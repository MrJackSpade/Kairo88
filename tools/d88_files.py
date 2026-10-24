#!/usr/bin/env python3
"""The files on a PC-88 disk image (.d88) written by N88-DISK BASIC.

Usage: d88_files.py <disk.d88> ...   (prints each disk's files: name, extension, attribute)

A D88 image is a header (name, protection, media type, size) with 164 track offsets, each track
its sectors in order: C, H, R, N, sector count, density, deleted mark, status, 5 reserved bytes,
data size, then the data. N88-DISK BASIC keeps its directory on cylinder 18, side 1 (track 37):
sectors 1 to 12 of 16-byte entries, each a 6-character name, a 3-character extension, an
attribute (0x01 BASIC program saved as tokens, 0x80 machine code, 0x10 write protected, 0x00
ASCII), the file's first cluster, then 0xff. An entry starting with 0x00 was deleted and one
starting with 0xff ends the directory.
"""
import struct
import sys
from pathlib import Path

HEADER = 0x2B0
DIRECTORY_TRACK = 37
BASIC = 0x01
MACHINE_CODE = 0x80


def sectors(image, track):
    """{sector number R: data} of one track of a D88 image."""
    offset = struct.unpack_from("<I", image, 0x20 + track * 4)[0]
    if offset == 0 or offset >= len(image):
        return {}
    found = {}
    count = struct.unpack_from("<H", image, offset + 4)[0]
    for _ in range(count):
        r = image[offset + 2]
        size = struct.unpack_from("<H", image, offset + 14)[0]
        found[r] = image[offset + 16:offset + 16 + size]
        offset += 16 + size
    return found


def files(path):
    """[(name, extension, attribute)] of the disk at [path]; empty when it has no N88 directory."""
    image = Path(path).read_bytes()
    track = sectors(image, DIRECTORY_TRACK)
    listed = []
    for r in range(1, 13):
        data = track.get(r, b"")
        for at in range(0, len(data) - 15, 16):
            entry = data[at:at + 16]
            if entry[0] == 0xFF:
                return listed
            if entry[0] == 0x00:
                continue
            name, extension = entry[:6], entry[6:9]
            if not all(0x20 <= c < 0x7F or 0xA1 <= c <= 0xDF for c in name + extension):
                # Not a directory: a game's own format on this track.
                return []
            listed.append((name.decode("latin-1").rstrip(), extension.decode("latin-1").rstrip(), entry[9]))
    return listed


if __name__ == "__main__":
    for argument in sys.argv[1:]:
        print(argument)
        for name, extension, attribute in files(argument):
            print(f"  {name}.{extension}  {attribute:#04x}")
