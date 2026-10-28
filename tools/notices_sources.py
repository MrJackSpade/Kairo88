"""Kairo88's third-party notices: the sources shared/tools/notices.py writes
kairo88/src/main/assets/THIRD_PARTY_NOTICES.txt from. Run it again after an import."""
from notices import excerpt, text

OUT = 'kairo88/src/main/assets/THIRD_PARTY_NOTICES.txt'

HEADER = '''Kairo88 third-party notices
These notices apply to the emulator code, libraries, and C++ runtime in this package.'''

QUASI88_TERMS = '''QUASI88, except its sound output section, is free software under terms following the modified
BSD license. Kairo88 does not include the sound output section (MAME/XMAME code and fmgen); its
sound comes from ymfm. Some of QUASI88's built-in images use ICOOON MONO icons and the Shinonome
font, which remain under their authors' copyrights.'''

# (title, source, [text]) in the order shown.
SECTIONS = [
    ('QUASI88 0.7.4 by Showzoh Fukunaga', 'third_party/quasi88/doc/manual.txt (著作権／免責)',
     [QUASI88_TERMS, excerpt('quasi88/doc/manual.txt', '著作権／免責', 'これらの画像、フォントの著作権は各著作者が有します。')]),
    ('ymfm', 'third_party/ymfm/LICENSE', [text('ymfm/LICENSE')]),
    # The license is EUC-JP, as the font project ships it.
    ('Shinonome 16-dot bitmap fonts (public domain), in QUASI88\'s built-in images', 'third_party/shinonome/LICENSE',
     [text('shinonome/LICENSE', 'euc_jp')]),
]
