"""Kairo88's catalog settings: PC-8801 media, artwork references and the platform's slugs."""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "shared/tools"))
import content_ids
from launchbox_artwork import References

APP = "kairo88"
PLATFORMS = ("pc88",)
ARTWORK_CODE = 88
ARTWORK = References("Kairo88", (ARTWORK_CODE,))

FLOPPY = {".d88", ".88d"}
TAPE = {".t88", ".cmt"}
LIMITS = {"fd": 64 * 1024**2, "tape": 16 * 1024**2}

KINDS = set(LIMITS)


def kind(name):
    suffix = Path(name).suffix.lower()
    return "fd" if suffix in FLOPPY else "tape" if suffix in TAPE else None


# Neo Kobe distributes PC-88 sets as .7z archives.
MEDIA = content_ids.Media(kind, content_ids.same_file, LIMITS.__getitem__, 1, seven_zip=True)

MOBYGAMES = "pc88"
# Neo Kobe PC-8801 (2016-02-25): one folder per game, holding its .7z releases.
SCANS = {"neokobe": (".downloads/scan/neokobe-pc88.json", ("pc88",))}


def GROUP(path):
    return path.split("/", 1)[0]


# The machine settings a game can ask for, as Pc88Native's options name them.
MACHINE_OPTIONS = {"basic": {"n", "v1s", "v1h", "v2"}, "clock": {"8", "4"}, "sound": {"1", "2"},
                   "port": {"joystick", "mouse"}}
_MODES = {"v1 mode": "v1s", "v1s mode": "v1s", "v1h mode": "v1h", "n mode": "n", "boot n mode": "n"}


# A tape's loading instructions in Neo Kobe's notes, word by word: monitor commands, Ctrl-B back to
# BASIC, LOAD CAS for BASIC's cassette load, and RUN.
_LOADER_WORD = re.compile(r"MON|R|L|RDATA|G[0-9A-F]{4}|RUN|CTRL[-+]B|LOAD CAS", re.I)


def _loader(part):
    """The lines a loading instruction types, such as "MON R G8F00": MON, R, G8F00; None for a note
    that is not one (a key to press in the game, a password)."""
    words = re.findall(r"LOAD CAS|\S+", part, re.I)
    if not words or not all(_LOADER_WORD.fullmatch(word) for word in words):
        return None
    lines = []
    for word in words:
        word = word.upper()
        lines.append('LOAD"CAS:"' if word == "LOAD CAS" else "{CTRL-B}" if word.startswith("CTRL") else word)
    return lines


def MACHINE(name, set_name, content_id):
    """Neo Kobe writes the mode and clock a disk needs in braces: {V1 mode}, {V1H mode}, {4MHz},
    {V1 mode, RUN MPD1}. A tape's notes may also say how to load it ({V1 mode, MON R G8F00}): that
    becomes its "loader", the lines typed at the prompt. A tape without one loads with LOAD"CAS:"
    and RUN in the app."""
    settings = {}
    for note in re.findall(r"\{([^}]*)\}", name):
        for part in (part.strip() for part in note.split(",")):
            lower = part.lower()
            if lower in _MODES:
                settings["basic"] = _MODES[lower]
            elif lower.startswith("boot n mode"):
                settings["basic"] = "n"
                rest = part[len("boot n mode"):].strip()
                if rest and content_id.startswith("sha256-tape-v1:") and _loader(rest):
                    settings["loader"] = _loader(rest)
            elif lower == "4mhz":
                settings["clock"] = "4"
            elif content_id.startswith("sha256-tape-v1:") and _loader(part):
                settings["loader"] = _loader(part)
    # A note that ends with a BASIC load leaves running it to the player ("F5 to start": F5 is RUN).
    if settings.get("loader", [None])[-1] == 'LOAD"CAS:"':
        settings["loader"].append("RUN")
    return settings


def NOT_GAME(path):
    """Neo Kobe keeps utilities, system disks and firmware dumps in [Utility], [OS] and [BIOS] folders."""
    return path.startswith(("[Utility]", "[OS]", "[BIOS]"))

# Disk sets (shared/tools/disk_sets.py): a game boots from its first floppy, with its second in drive 2,
# or from its tape when it has no floppy.
BOOT_KINDS = ["fd", "tape"]
COMPANION = {"fd": ("fd", "floppyB", False)}
