#!/usr/bin/env python3
"""Writes the shared PC boot test's PC-88 mouse program (Kairo88#49) into Kairo88's table: Z80 code
that N88-BASIC is given as typed lines (it pokes the bytes at C000h and runs them with USR). The
code reads the mouse on the OPN's general-purpose port itself (port 40h bit 6 strobes it, register
0Eh gives its four nibbles of -dx and -dy, register 0Fh bit 0 the left button, 0 pressed), with
interrupts off, so nothing but it sees the motion. It moves its pointer, an asterisk in text VRAM
(F3C8h, 120 bytes a row, 80 characters), a pixel per count over 640 x 200 (stopping at the edges, a
text cell per 8 x 8 pixels), once per frame (port 40h bit 5, the vertical retrace), and on a press
of the left button fills the text screen with a letter from where the pointer is: A + x / 128 +
5 * (y / 64). It never ends.

    make_pc88_mouse_basic.py <boot-tests.json>
"""
import json
import sys
from pathlib import Path

ORIGIN = 0xC000
TEXT = 0xF3C8
TEST = "PC-88 mouse program"
# Hex digits a typed line carries: N88-BASIC takes a line of 255 characters.
LINE_HEX = 160


class Assembler:
    """Z80 bytes with labels: relative (JR, DJNZ) and absolute 16-bit references."""

    def __init__(self):
        self.code = bytearray()
        self.labels = {}
        self.fixups = []  # (offset, label, kind): "rel8" or "abs16"

    def label(self, name):
        if name in self.labels:
            raise SystemExit(f"label {name} twice")
        self.labels[name] = ORIGIN + len(self.code)

    def b(self, *values):
        self.code += bytes(values)

    def w(self, value):
        if isinstance(value, str):
            self.fixups.append((len(self.code), value, "abs16"))
            self.code += b"\0\0"
        else:
            self.code += (value & 0xFFFF).to_bytes(2, "little")

    def jr(self, opcode, label):
        self.b(opcode, 0)
        self.fixups.append((len(self.code) - 1, label, "rel8"))

    def build(self):
        for offset, label, kind in self.fixups:
            target = self.labels[label]
            if kind == "abs16":
                self.code[offset:offset + 2] = target.to_bytes(2, "little")
            else:
                distance = target - (ORIGIN + offset + 1)
                if not -128 <= distance <= 127:
                    raise SystemExit(f"the jump to {label} is {distance} bytes")
                self.code[offset] = distance & 0xFF
        return bytes(self.code)


JR, JR_NZ, JR_Z, JR_NC, DJNZ = 0x18, 0x20, 0x28, 0x30, 0x10


def program():
    a = Assembler()
    b, w = a.b, a.w

    def opn(register):
        b(0x3E, register, 0xD3, 0x44)              # ld a, register; out (44h), a

    def strobe(level):
        b(0x3E, level, 0xD3, 0x40)                 # ld a, level; out (40h), a

    def read_byte(reg):
        """Two strobes, two nibbles: reg = -(the high nibble << 4 | the low one)."""
        strobe(0x40)
        b(0xDB, 0x45, 0x07, 0x07, 0x07, 0x07, 0xE6, 0xF0, 0x47)  # in a,(45h); rlca x4; and 0f0h; ld b,a
        strobe(0x00)
        b(0xDB, 0x45, 0xE6, 0x0F, 0xB0, 0xED, 0x44, reg)         # in a,(45h); and 0fh; or b; neg; ld reg,a

    def add_clamped(var, source, limit, done):
        """var (16-bit) += the signed byte in source, kept in 0..limit."""
        b(0x2A); w(var)                            # ld hl, (var)
        b(source, 0x4F, 0x87, 0x9F, 0x47)          # ld a, source; ld c, a; add a, a; sbc a, a; ld b, a
        b(0x09)                                    # add hl, bc
        b(0xCB, 0x7C); a.jr(JR_Z, done + "_low")   # bit 7, h
        b(0x21); w(0)                              # ld hl, 0
        a.label(done + "_low")
        b(0xE5, 0x01); w(-(limit + 1))             # push hl; ld bc, -(limit + 1)
        b(0x09, 0xE1); a.jr(JR_NC, done)           # add hl, bc; pop hl: no carry, below the limit
        b(0x21); w(limit)                          # ld hl, limit
        a.label(done)
        b(0x22); w(var)                            # ld (var), hl

    b(0xF3)                                        # di
    opn(0x07); b(0x3E, 0x3F, 0xD3, 0x45)           # register 7: tone and noise off, both ports in
    strobe(0x00)
    b(0x21); w(320); b(0x22); w("x")
    b(0x21); w(100); b(0x22); w("y")
    b(0x21); w(0); b(0x22); w("prev")              # no pointer drawn yet
    b(0x3E, 0x01, 0x32); w("button")               # released

    a.label("frame")
    a.label("leave")
    b(0xDB, 0x40, 0xE6, 0x20); a.jr(JR_NZ, "leave")  # out of the retrace
    a.label("enter")
    b(0xDB, 0x40, 0xE6, 0x20); a.jr(JR_Z, "enter")   # into it

    opn(0x0E)
    read_byte(0x4F)                                # c = dx
    b(0x79, 0x32); w("dx")                         # ld a, c; ld (dx), a
    read_byte(0x57)                                # d = dy
    opn(0x0F); b(0xDB, 0x45, 0x32); w("now")       # in a, (45h); ld (now), a
    b(0x3A); w("dx"); b(0x5F)                      # ld a, (dx); ld e, a
    add_clamped("x", 0x7B, 639, "x_done")          # ld a, e
    add_clamped("y", 0x7A, 199, "y_done")          # ld a, d

    # The pointer's cell: TEXT + (y / 8) * 120 + x / 8.
    b(0x3A); w("y"); b(0xCB, 0x3F, 0xCB, 0x3F, 0xCB, 0x3F)   # ld a, (y); srl a x3
    b(0x6F, 0x26, 0x00, 0x29, 0x29, 0x29, 0x54, 0x5D)        # ld l,a; ld h,0; add hl,hl x3; ld d,h; ld e,l
    b(0x29, 0x29, 0x29, 0x29, 0xB7, 0xED, 0x52)              # add hl,hl x4; or a; sbc hl,de: row * 120
    b(0x11); w(TEXT); b(0x19, 0xEB)                          # ld de, TEXT; add hl, de; ex de, hl
    b(0x2A); w("x"); b(0xCB, 0x3C, 0xCB, 0x1D, 0xCB, 0x3C, 0xCB, 0x1D, 0xCB, 0x3C, 0xCB, 0x1D)  # x >> 3
    b(0x19)                                                  # add hl, de
    b(0xED, 0x5B); w("prev")                                 # ld de, (prev)
    b(0xB7, 0xED, 0x52, 0x19); a.jr(JR_Z, "drawn")           # or a; sbc hl,de; add hl,de
    b(0x7A, 0xB3); a.jr(JR_Z, "unsaved")                     # ld a,d; or e
    b(0x3A); w("saved"); b(0x12)                             # ld a, (saved); ld (de), a
    a.label("unsaved")
    b(0x7E, 0x32); w("saved")                                # ld a, (hl); ld (saved), a
    b(0x36, 0x2A)                                            # ld (hl), '*'
    b(0x22); w("prev")
    a.label("drawn")

    # A press of the left button: the screen filled with the pointer's letter.
    b(0x3A); w("now"); b(0xE6, 0x01, 0x47)                   # ld a,(now); and 1; ld b,a
    b(0x3A); w("button"); b(0xB8); a.jr(JR_Z, "next")        # ld a,(button); cp b
    b(0x78, 0x32); w("button"); b(0xB7); a.jr(JR_NZ, "next") # ld a,b; ld (button),a; or a: released
    b(0x2A); w("x"); b(0x29, 0x4C)                           # ld hl,(x); add hl,hl; ld c,h: x >> 7
    b(0x3A); w("y"); b(0x07, 0x07, 0xE6, 0x03, 0x47)         # ld a,(y); rlca x2; and 3; ld b,a: y >> 6
    b(0x87, 0x87, 0x80, 0x81, 0xC6, 0x41)                    # add a,a x2; add a,b; add a,c; add a,'A'
    b(0x32); w("saved"); b(0x4F)                             # ld (saved), a; ld c, a
    b(0x21); w(TEXT); b(0x11); w(40); b(0x06, 25)            # ld hl,TEXT; ld de,40; ld b,25
    a.label("row")
    b(0xC5, 0x06, 80)                                        # push bc; ld b, 80
    a.label("cell")
    b(0x71, 0x23); a.jr(DJNZ, "cell")                        # ld (hl),c; inc hl
    b(0x19, 0xC1); a.jr(DJNZ, "row")                         # add hl,de; pop bc
    b(0x2A); w("prev"); b(0x36, 0x2A)                        # the pointer over it again
    a.label("next")
    b(0xC3); w("frame")

    for name, size in (("x", 2), ("y", 2), ("prev", 2), ("saved", 1), ("button", 1), ("now", 1), ("dx", 1)):
        a.label(name)
        b(*([0] * size))
    # The variables are set before they are read, so only the code is typed.
    return a.build()[:a.labels["x"] - ORIGIN]


def commands(code):
    """The typed lines: 25 text rows, memory above BFFFh kept from BASIC, the bytes poked a line at a time, then run."""
    digits = code.hex().upper()
    # 25 rows of 8 lines: BASIC starts with 20 of 10.
    lines = ["WIDTH 80,25", "CLEAR 300,&HBFFF", f"P=&H{ORIGIN:X}"]
    for at in range(0, len(digits), LINE_HEX):
        part = digits[at:at + LINE_HEX]
        lines.append(f'A$="{part}":FOR I=1 TO LEN(A$) STEP 2:POKE P,VAL("&H"+MID$(A$,I,2)):P=P+1:NEXT')
    lines.append(f"DEF USR=&H{ORIGIN:X}:A=USR(0)")
    for line in lines:
        if len(line) > 250:
            raise SystemExit(f"a typed line of {len(line)} characters is too long for N88-BASIC")
    return "\n".join(lines)


def main(argv):
    if len(argv) != 2:
        raise SystemExit(__doc__)
    table = Path(argv[1])
    code = program()
    data = json.loads(table.read_text(encoding="utf-8"))
    entries = [t for t in data["tests"] if t["name"] == TEST]
    if len(entries) != 1:
        raise SystemExit(f"{len(entries)} tests named {TEST} in {table}")
    entries[0]["checks"]["directTap"]["commands"] = commands(code)
    table.write_text(json.dumps(data, indent=2, ensure_ascii=False) + "\n", encoding="utf-8", newline="\n")
    print(f"{table}: {TEST}: {len(code)} bytes of Z80 code")


if __name__ == "__main__":
    main(sys.argv)
