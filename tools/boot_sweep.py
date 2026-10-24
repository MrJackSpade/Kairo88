#!/usr/bin/env python3
"""Boots every PC-88 game in the library on the host and reports which ones fail to start.

Usage: boot_sweep.py <work dir> --label <pass> [--from <earlier pass>] [--set key=value ...]
                     [--runner <sweep_test>] [--romdir <bios>] [--jobs N] [--frames N] [--check N]
                     [--only <id,...>]

The first pass, without --from, runs every game and finds the ones that do not start. Each later
pass runs only the games still not starting after <earlier pass>, with one candidate fix (--set),
so the games left can only shrink. A game a pass fixes is recorded in <work dir>/fixes.json with
the settings that fixed it (the catalog takes them from there); the rest go on to the next pass.

Each library entry is resolved as the app resolves it (catalog/core-v3.json): the entry disk in
drive 1, its companion ("media" floppyB) in drive 2, and the machine settings its catalog record
names over the app's defaults (Pc88Native: V2, 8 MHz, sound board II). --set overrides a setting
for every game, such as --set subcpu=disk to try the disk unit's CPU in step during disk access. Disks are read from the
Neo Kobe set the catalog was built from (.downloads/scan/neokobe-pc88.json names their archives)
and extracted once into <work dir>/disks.

core/test/sweep_test runs the games, several processes at once, each booting one game after
another as the app does. It runs <frames> frames with no sound synthesized and the picture
composed only every <check> frames; both change nothing the machine computes. A game is:
  black    fewer lit pixels at the last check than a line of text
  basic    it ended in BASIC: its prompt, an error in a program line, the "How many files" prompt or
           BASIC's function-key row in one colour
  mode     it asks for another BASIC mode ("V2 MODE ONLY")
  untyped  the pass had commands to type, but the game never stopped at a prompt for them
  rejected a screen the review of the snapshots found no started game (--reject records it)
  mono     any other screen of one colour on black (a hung loader's bar, an error, a monochrome game)
  picture  a screen of three colours or more, or one the review found a game running (--accept
           records it): the only outcome counted started
A colourful screen is not proof a game started (a disk error, garbage), and a game in text may be
waiting for the player, so each pass's fixes and the games left are reviewed by eye; passes from a
changed verdict's pass on are run again.
Writes <work dir>/<label>/results.json and summary.txt, and every game's last picture at its
native size as <work dir>/<label>/snap/<id>.png.
"""
import argparse
import hashlib
import json
import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SEVEN_ZIP = Path(r"C:\Program Files\7-Zip\7z.exe")
# Pc88Native's defaults and the core's numbers for each setting (Pc88Native.settings).
DEFAULTS = {"basic": "v2", "clock": "8", "sound": "2", "subcpu": "off"}
BASIC = {"n": 0, "v1s": 1, "v1h": 2, "v2": 3}
SOUND = {"1": 0, "2": 1}
SUBCPU = {"off": 0, "disk": 1, "always": 2}
BASIC_ROW = ('load "', "auto", "go to", "list", "run")
BASIC_TEXT = ("How many files", "Bytes free", "File not found", "Disk I/O error", "Syntax error")
# BASIC's prompt (with whatever the cursor leaves after it) and its errors in a program line.
BASIC_LINE = re.compile(r"^Ok\W?$|\b(?:error|overflow) in \d+", re.I)
# A game telling the player to switch the machine to another BASIC mode.
WRONG_MODE = re.compile(r"\bV[12][ -]?mode\b|mode only|boot by v[12]|requests? v[12]", re.I)
# Fewer lit pixels than a line of text: a stray line or a cursor on a blank screen.
BLANK = 1000
# The review of the snapshots: screens it found no started game (a disk error, garbage, a
# message: "notStarted") and screens it found a game running though the rules above would not say
# so (a menu or a prompt in text: "started"), each {hash of the picture: the disk it was seen for}.
# The emulator is deterministic, so the same screen comes back for the same game and settings and
# gets the same verdict. A game whose opening ends on a black screen at the last check has no
# picture to name it by: "startedGames" are the games the review saw running all the same, each
# {content ID: disk}.
REVIEW = ROOT / "catalog/research/boot-sweep-review-v1.json"
VERDICTS = ("notStarted", "started")


def screen_hash(snapshot):
    return hashlib.sha1(Image.open(snapshot).convert("RGB").tobytes()).hexdigest()


def reviewed_screens():
    """{verdict: {hash: disk}} and {"startedGames": {content ID: disk}}, from the review file."""
    document = json.loads(REVIEW.read_text(encoding="utf-8"))
    return {verdict: document[verdict] for verdict in (*VERDICTS, "startedGames")}


def write_review(review):
    REVIEW.write_text(json.dumps({"schemaVersion": 1, **{name: dict(sorted(entries.items()))
                                                         for name, entries in review.items()}},
                                 indent=1, ensure_ascii=False) + "\n", encoding="utf-8")


def outcome_of(result, snapshot, review):
    """A result's outcome: started for a game the review saw running, else as its screen says."""
    if result["contentId"] in review["startedGames"]:
        return "picture"
    return classify(result["lit"], result["text"], snapshot, review)


def library(catalog):
    """{entry content ID: (record, drive-2 content ID or None)} for every disk the library lists."""
    hidden = set()
    for content_id, record in catalog.items():
        hidden.update(record.get("setDisks", []))
        if record.get("hidden"):
            hidden.add(content_id)
    entries = {}
    for content_id, record in catalog.items():
        if content_id in hidden:
            continue
        second = next((m["contentId"] for m in record.get("media", []) if m["role"] == "floppyB"), None)
        entries[content_id] = (record, second)
    return entries


def short_id(content_id):
    return content_id.split(":")[1][:16]


def extract(scan, work, wanted):
    """Extracts each wanted disk once: {content ID: path}."""
    disks = work / "disks"
    disks.mkdir(parents=True, exist_ok=True)
    root = Path(scan["root"])
    where = {}
    for archive in scan["archives"]:
        for disk in archive.get("disks", []):
            where.setdefault(disk["contentId"], (archive["path"], disk["entry"]))
    paths, missing = {}, []
    for content_id in sorted(wanted):
        if content_id not in where:
            missing.append(content_id)
            continue
        archive, member = where[content_id]
        suffix = Path(member).suffix.lower()
        target = disks / f"{short_id(content_id)}{suffix}"
        if not target.exists():
            data = subprocess.run([str(SEVEN_ZIP), "e", "-so", str(root / archive), member],
                                  check=True, capture_output=True).stdout
            if not data:
                raise SystemExit(f"7-Zip gave nothing for {member} in {archive}")
            # Written whole under another name first, so a stopped run never leaves a cut-off disk.
            partial = target.with_name(target.name + ".partial")
            partial.write_bytes(data)
            partial.replace(target)
        paths[content_id] = target
    return paths, missing, where


# Names a disk's starting program usually has, most likely first.
START_NAMES = ("START", "AUTO", "MAIN", "MENU", "GAME", "OPENING", "TITLE", "LOADER", "BOOT", "INIT")


def directory_program(disk):
    """The BASIC program (saved as tokens) a disk most likely starts with: one named like a start
    program, else the first in its N88-DISK BASIC directory; None for a disk without one. Only
    names the loader can type count: one in kana would need the kana keys."""
    from d88_files import BASIC, MACHINE_CODE, files
    programs = [name + ("." + extension if extension else "") for name, extension, attribute in files(disk)
                if attribute & BASIC and not attribute & MACHINE_CODE]
    programs = [program for program in programs if all(" " <= c <= "~" and c != '"' for c in program)]
    if not programs:
        return None
    return next((program for start in START_NAMES for program in programs
                 if program.upper().split(".")[0] == start), programs[0])


def name_commands(name):
    """The commands a disk's name says to type ({V1 mode, RUN TANTEI}: RUN TANTEI), by
    catalog_config's loader rules; None when it gives none."""
    sys.path.insert(0, str(ROOT / "tools"))
    import catalog_config
    lines = []
    for note in re.findall(r"\{([^}]*)\}", name):
        for part in (part.strip() for part in note.split(",")):
            if part.lower() in catalog_config._MODES or part.lower() == "4mhz":
                continue
            loader = catalog_config._loader(part)
            if loader:
                lines += loader
            elif re.fullmatch(r"(?:RUN|LOAD)\s+\S+", part, re.I):
                # A program by name: BASIC wants it quoted (RUN"TANTEI").
                verb, program = part.split(None, 1)
                lines.append(f'{verb.upper()}"{program}"')
    # A note that ends with a load leaves running it to the player, as a tape's does.
    if lines and lines[-1].upper().startswith("LOAD"):
        lines.append("RUN")
    return lines or None


def classify(checks, text, snapshot, review):
    """black, basic, mode, rejected, mono or picture; the review's verdict on the screen comes
    first (rejected, or picture for one it found running). Text memory keeps BASIC's function-key row
    after a game has drawn over the screen, so BASIC's words alone do not mean BASIC: only a picture
    that is text and nothing else (at most two colours) with them is BASIC. A screen black at the
    last check is black even if something showed before: a blinking BASIC prompt, a logo before a
    hang; a game that is only between scenes there costs a retry, where calling a broken game
    started would hide it."""
    seen = screen_hash(snapshot)
    if seen in review["notStarted"]:
        return "rejected"
    if seen in review["started"]:
        return "picture"
    if not checks or checks[-1] < BLANK:
        return "black"
    # BASIC at its prompt whatever colour its text: an Ok line, or what it prints when a load fails.
    rows = [row.strip() for row in text.split("|")]
    if any(BASIC_LINE.search(row) for row in rows) or any(phrase.lower() in text.lower() for phrase in BASIC_TEXT):
        return "basic"
    if WRONG_MODE.search(text):
        return "mode"
    colours = Image.open(snapshot).convert("RGB").getcolors(1 << 20)
    if colours is not None and len(colours) <= 2:
        # A screen of one colour on black: BASIC, a hung loader's bar or cursor, an error message. A
        # monochrome game is one too; it is never counted started, so no pass records a fix for it,
        # and it stays for the review of what is left.
        lower = text.lower()
        if "how many files" in lower or sum(word in lower for word in BASIC_ROW) >= 4:
            return "basic"
        return "mono"
    return "picture"


def record_fix(label, result):
    """What fixes.json keeps of a game [label] started: the pass, its change, and its disks."""
    return {"pass": label, "change": result["change"], "game": result["game"], "disk": result["disk"],
            "second": result["second"]}


FIXES = ROOT / "catalog/research/boot-fixes-v1.json"
# A sweep setting's value as the catalog stores it (Pc88Native: the clock is a number).
CATALOG_VALUE = {"clock": int}


def export(work):
    """The fixes the review kept, as catalog settings: machine options and startup commands. A fix
    that is only another disk to boot or put in drive 2 is a rule of the disk sets, not a setting
    of the game, and one with no change started given more time; neither is written."""
    fixes = json.loads((work / "fixes.json").read_text(encoding="utf-8"))
    written, skipped = {}, []
    for content_id, fix in sorted(fixes.items()):
        change = {key: value for key, value in fix["change"].items() if key not in ("boot", "second")}
        if not change:
            skipped.append(f"{fix['pass']}: {fix['disk']} ({', '.join(fix['change']) or 'more time'})")
            continue
        written[content_id] = {"pass": fix["pass"], "disk": fix["disk"],
                               "settings": {key: CATALOG_VALUE.get(key, str)(value) if key != "commands" else value
                                            for key, value in change.items()}}
    FIXES.write_text(json.dumps({"schemaVersion": 1, "fixes": written}, indent=1, ensure_ascii=False) + "\n",
                     encoding="utf-8")
    print(f"{len(written)} games' settings written to {FIXES.relative_to(ROOT)}")
    for line in skipped:
        print(f"  not a setting: {line}")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("work", type=Path)
    parser.add_argument("--label", help="the pass (required except with --export)")
    parser.add_argument("--from", dest="earlier", help="run only the games this pass left not starting")
    parser.add_argument("--runner", type=Path, default=ROOT / ".build/host/sweep_test.exe")
    parser.add_argument("--romdir", type=Path, default=ROOT / "bios/MA")
    parser.add_argument("--jobs", type=int, default=max(1, (os.cpu_count() or 2) - 2))
    parser.add_argument("--frames", type=int, default=1800)
    parser.add_argument("--check", type=int, default=300)
    parser.add_argument("--only", default="", help="comma-separated short IDs")
    parser.add_argument("--set", action="append", default=[], help="key=value over every game's settings")
    parser.add_argument("--type-from-name", action="store_true",
                        help="type the commands the boot disk's name gives at BASIC's prompt, as "
                             "{V1 mode, RUN TANTEI} (catalog_config's tape loader rules, for a disk)")
    parser.add_argument("--commands", help="lines to type at BASIC's prompt for every game, separated by ;")
    parser.add_argument("--type-from-directory", action="store_true",
                        help="type RUN for the BASIC program the boot disk's N88-DISK BASIC directory most likely "
                             "starts with (directory_program)")
    parser.add_argument("--second-next-disk", action="store_true",
                        help="put the disk after the entry in its archive (Disk A, then Disk B) in drive 2")
    parser.add_argument("--boot-from-start-disk", action="store_true",
                        help="boot from the disk of the game's set whose name marks it as the one a game starts "
                             "from (disk_sets' start roles: program, boot, system...), the entry disk in drive 2")
    parser.add_argument("--reclassify", action="store_true",
                        help="score <label>'s saved results again from its snapshots, without running anything")
    parser.add_argument("--reject", default="",
                        help="comma-separated short IDs whose <label> snapshots the review found no started game: "
                             "their screens are recorded as not started, and <label> is scored again")
    parser.add_argument("--accept-game", default="",
                        help="comma-separated short IDs of <label>'s games the review saw running though their "
                             "last screen is black (an opening between scenes): they are recorded as started")
    parser.add_argument("--accept", default="",
                        help="comma-separated short IDs whose <label> snapshots the review found a game running: "
                             "their screens are recorded as started, and <label> is scored again")
    parser.add_argument("--export", action="store_true",
                        help="write the reviewed fixes (<work dir>/fixes.json) to the catalog's research "
                             "(catalog/research/boot-fixes-v1.json), as the settings catalog_config gives each game")
    args = parser.parse_args()

    if args.export:
        export(args.work)
        return 0
    if not args.label:
        parser.error("--label is required")

    review = reviewed_screens()
    if args.reject or args.accept or args.accept_game:
        results = json.loads((args.work / args.label / "results.json").read_text(encoding="utf-8"))
        for sid in filter(None, args.accept_game.split(",")):
            review["startedGames"][results[sid]["contentId"]] = results[sid]["disk"]
        for verdict, sids in (("notStarted", args.reject), ("started", args.accept)):
            for sid in filter(None, sids.split(",")):
                seen = screen_hash(args.work / args.label / "snap" / f"{sid}.png")
                for other in VERDICTS:
                    review[other].pop(seen, None)
                review[verdict][seen] = results[sid]["disk"]
        write_review(review)
        args.reclassify = True

    if args.reclassify:
        path = args.work / args.label / "results.json"
        results = json.loads(path.read_text(encoding="utf-8"))
        for sid, result in results.items():
            # A game this pass did not try carries an earlier pass's result, scored there.
            if result["outcome"] not in ("open-failed", "untyped") and not result.get("untried"):
                result["outcome"] = outcome_of(result, args.work / args.label / "snap" / f"{sid}.png", review)
        path.write_text(json.dumps(results, indent=1, ensure_ascii=False), encoding="utf-8")
        # This pass's fixes as the screens are now scored: a game it started that it no longer counts
        # started is no fix, and one the review found running is. The first pass has no fixes.
        fixes_path = args.work / "fixes.json"
        if fixes_path.exists() and args.label != "base":
            fixes = json.loads(fixes_path.read_text(encoding="utf-8"))
            for result in results.values():
                fix = fixes.get(result["contentId"])
                if fix and fix["pass"] == args.label and result["outcome"] != "picture":
                    del fixes[result["contentId"]]
                elif result["outcome"] == "picture" and not result.get("untried") and fix is None:
                    fixes[result["contentId"]] = record_fix(args.label, result)
            fixes_path.write_text(json.dumps(fixes, indent=1, ensure_ascii=False), encoding="utf-8")
        counts = {}
        for result in results.values():
            counts[result["outcome"]] = counts.get(result["outcome"], 0) + 1
        print(f"{args.label}: " + ", ".join(f"{name} {count}" for name, count in sorted(counts.items())))
        return 0

    catalog =json.loads((ROOT / "catalog/core-v3.json").read_text(encoding="utf-8"))["games"]
    scan = json.loads((ROOT / ".downloads/scan/neokobe-pc88.json").read_text(encoding="utf-8"))
    entries = library(catalog)
    tapes = {cid for cid in entries if cid.startswith("sha256-tape-")}
    floppies = {cid: entry for cid, entry in entries.items() if cid not in tapes}
    if args.only:
        only = set(args.only.split(","))
        floppies = {cid: entry for cid, entry in floppies.items() if short_id(cid) in only}
    earlier = None
    if args.earlier:
        earlier = json.loads((args.work / args.earlier / "results.json").read_text(encoding="utf-8"))
        left = {sid for sid, result in earlier.items() if result["outcome"] != "picture"}
        floppies = {cid: entry for cid, entry in floppies.items() if short_id(cid) in left}
    forced = dict(item.split("=", 1) for item in args.set)
    wanted = set(floppies) | {second for _, second in floppies.values() if second}
    paths, missing, where = extract(scan, args.work, wanted)

    out = args.work / args.label
    (out / "snap").mkdir(parents=True, exist_ok=True)
    jobs, info, untried = [], {}, {}
    if args.boot_from_start_disk or args.second_next_disk:
        # The other disks of the entry's own archive, by name.
        sys.path.insert(0, str(ROOT / "shared/tools"))
        from disk_sets import DISK_TAG, _order, _role
        archive_disks = {}
        for archive in scan["archives"]:
            for disk in archive.get("disks", []):
                archive_disks.setdefault(archive["path"], []).append(disk)
    for content_id, (record, second) in sorted(floppies.items()):
        if content_id not in paths or (second and second not in paths):
            continue
        # A game is its library entry (sid); the disks it boots with may be others of its set.
        sid = short_id(content_id)
        boot, change = content_id, {}
        if args.boot_from_start_disk:
            members = archive_disks[where[content_id][0]]
            start = next((disk["contentId"] for disk in members if disk["contentId"] != content_id and
                          not DISK_TAG.search(disk["entry"]) and _role(disk["entry"]) == 0), None)
            if start is None:
                untried[sid] = content_id
                continue
            # The start disk in drive 1, the entry disk beside it in drive 2.
            paths.update(extract(scan, args.work, {start})[0])
            boot, second = start, content_id
            change = {"boot": start, "second": content_id}
        if args.second_next_disk:
            # The disk after the entry in its archive by disk tag (Disk A, then Disk B) in drive 2.
            members = sorted(archive_disks[where[content_id][0]], key=lambda disk: _order(disk["entry"]))
            ids = [disk["contentId"] for disk in members]
            following = ids[ids.index(content_id) + 1] if content_id in ids[:-1] else None
            if following is None or following == second:
                untried[sid] = content_id
                continue
            paths.update(extract(scan, args.work, {following})[0])
            second = following
            change = {"second": following}
        settings = {**DEFAULTS, **record.get("machine", {}), **forced}
        lines = None
        if args.type_from_name:
            lines = name_commands(where[boot][1])
            if not lines:
                untried[sid] = content_id
                continue
            change = {**change, "commands": lines}
        if args.commands:
            lines = [line.strip() for line in args.commands.split(";")]
            change = {**change, "commands": lines}
        if args.type_from_directory:
            program = directory_program(paths[boot])
            if program is None:
                untried[sid] = content_id
                continue
            lines = [f'RUN"{program}"']
            change = {**change, "commands": lines}
        ppm = out / "snap" / f"{sid}.ppm"
        jobs.append("\t".join([sid, str(BASIC[settings["basic"]]), "1" if str(settings["clock"]) == "8" else "0",
                               str(SOUND[settings["sound"]]), str(SUBCPU[settings["subcpu"]]), paths[boot].as_posix(),
                               paths[second].as_posix() if second else "-", ppm.as_posix(),
                               "\x1f".join(lines) if lines else "-"]))
        info[sid] = {"contentId": content_id, "game": record["game"], "archive": where[content_id][0],
                     "disk": where[boot][1], "second": where[second][1] if second else None,
                     "settings": settings, "change": {**forced, **change}}
    chunks = [jobs[i::args.jobs] for i in range(args.jobs)]
    env = {**os.environ, "HOME": "/kairo88-sweep", "MSYS2_ENV_CONV_EXCL": "HOME"}

    def run(chunk):
        if not chunk:
            return ""
        done = subprocess.run([str(args.runner), args.romdir.as_posix() + "/", str(args.frames), str(args.check), "0"],
                              input="\n".join(chunk) + "\n", capture_output=True, text=True, env=env,
                              encoding="utf-8", errors="replace")
        if done.returncode != 0:
            raise SystemExit(f"sweep_test failed ({done.returncode}): {done.stderr[-2000:]}")
        return done.stdout

    with ThreadPoolExecutor(args.jobs) as pool:
        lines = "".join(pool.map(run, chunks)).splitlines()
    results = {}
    for line in lines:
        # The runner's result lines; QUASI88 prints notes of its own ("FDC : Buffer overflow") too.
        if not line.startswith("@sweep\t"):
            continue
        sid, checks, untyped, text = (line.split("\t")[1:] + ["", "", ""])[:4]
        ppm = out / "snap" / f"{sid}.ppm"
        png = out / "snap" / f"{sid}.png"
        if ppm.exists():
            Image.open(ppm).save(png)
            ppm.unlink()
        if checks == "open-failed":
            outcome, numbers = "open-failed", []
        else:
            numbers = [int(n) for n in checks.split(",")]
            outcome = outcome_of({"contentId": info[sid]["contentId"], "lit": numbers, "text": text}, png, review)
            # Commands never typed (the game went on without stopping at BASIC) are no fix of its,
            # and in the app they would wait for a prompt that never comes.
            if untyped == "1":
                outcome = "untyped"
        # A game is left for the next pass unless it reached a picture with this pass's settings.
        results[sid] = {**info[sid], "outcome": outcome, "lit": numbers, "text": text}
    if len(results) != len(jobs):
        raise SystemExit(f"{len(jobs)} games ran but {len(results)} reported")
    # Games this pass had no candidate fix for carry their earlier result to the next pass.
    for sid in untried:
        results[sid] = {**earlier[sid], "untried": True}
    (out / "results.json").write_text(json.dumps(results, indent=1, ensure_ascii=False), encoding="utf-8")
    counts = {}
    for result in results.values():
        counts[result["outcome"]] = counts.get(result["outcome"], 0) + 1
    summary = [f"{len(results)} games; {len(tapes)} tape games not run; {len(missing)} disks not in the scan",
               *(f"{name}: {count}" for name, count in sorted(counts.items()))]
    if earlier is not None:
        fixed = {sid: result for sid, result in results.items() if result["outcome"] == "picture"}
        # The settings that fixed each game, kept across passes for the catalog.
        fixes_path = args.work / "fixes.json"
        fixes = json.loads(fixes_path.read_text(encoding="utf-8")) if fixes_path.exists() else {}
        for sid, result in fixed.items():
            fixes[result["contentId"]] = record_fix(args.label, result)
        fixes_path.write_text(json.dumps(fixes, indent=1, ensure_ascii=False), encoding="utf-8")
        before = sum(1 for result in earlier.values() if result["outcome"] != "picture")
        after = len(results) - len(fixed)
        if after > before:
            raise SystemExit(f"{args.label} left {after} games not starting, more than {args.earlier}'s {before}")
        summary.append(f"{args.earlier} left {before}; {args.label} fixed {len(fixed)}, leaving {after}")
    (out / "summary.txt").write_text("\n".join(summary) + "\n", encoding="utf-8")
    print("\n".join(summary))


if __name__ == "__main__":
    sys.exit(main())
