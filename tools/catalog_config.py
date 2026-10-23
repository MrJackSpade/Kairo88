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


def MACHINE(name, set_name, content_id):
    """Neo Kobe writes the mode and clock a disk needs in braces: {V1 mode}, {V1H mode}, {4MHz},
    {V1 mode, RUN MPD1}."""
    settings = {}
    for note in re.findall(r"\{([^}]*)\}", name):
        for part in (part.strip().lower() for part in note.split(",")):
            if part in _MODES:
                settings["basic"] = _MODES[part]
            elif part.startswith("boot n mode"):
                settings["basic"] = "n"
            elif part == "4mhz":
                settings["clock"] = "4"
    return settings


def NOT_GAME(path):
    """Neo Kobe keeps utilities, system disks and firmware dumps in [Utility], [OS] and [BIOS] folders."""
    return path.startswith(("[Utility]", "[OS]", "[BIOS]"))

# Disk sets (shared/tools/disk_sets.py): a game boots from its first floppy, with its second in drive 2.
BOOT_KINDS = ["fd"]
COMPANION = {"fd": ("fd", "floppyB", False)}
