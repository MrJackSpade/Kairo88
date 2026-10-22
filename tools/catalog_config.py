"""Kairo88's catalog settings: PC-8801 media, artwork references and the platform's slugs."""
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


def IGNORE(path):
    """Neo Kobe keeps BIOS and kanji ROM dumps in [BIOS] folders."""
    return path.startswith("[BIOS]")
