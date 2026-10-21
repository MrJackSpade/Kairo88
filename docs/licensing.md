# Licensing and credits

[← Kairo88](../README.md)

Kairo88 first-party code is [GPL-2.0-or-later](../LICENSE.md). The pinned [Kairo frontend](https://github.com/MrJackSpade/Kairo) is also GPL-2.0-or-later. Third-party components keep their own notices.

## MAME

The MAME PC-8801 core comes from [MAME](https://www.mamedev.org/) 0.289. MAME as a whole is distributed under the GPL; individual files carry their own license in a `// license:` header. In the imported subset those headers are BSD-3-Clause, GPL-2.0+, LGPL-2.1+, MIT, or public domain. The import contains no GPL-2.0-only file. The PC-8801 driver files (`src/mame/nec/pc8801*`, `pc88*`) are BSD-3-Clause. MAME's license texts are in `third_party/mame/COPYING` and `third_party/mame/docs/legal/`.

The bundled MAME 3rdparty libraries keep their own licenses:

| Library | License |
| --- | --- |
| asio | Boost Software License 1.0 |
| asmjit | Zlib |
| expat | MIT |
| flac | libFLAC is BSD-3-Clause (Xiph). The directory also holds the `flac`/`metaflac` programs (GPL) and documentation (GFDL), which are not built. |
| lzma (LZMA SDK) | Public domain |
| rapidjson | MIT (the JSON-licensed `bin/jsonchecker/` is not included) |
| softfloat3 | BSD-3-Clause |
| utf8proc | MIT, with Unicode data under the Unicode license |
| ymfm | BSD-3-Clause |
| zlib | Zlib |
| zstd | BSD-3-Clause (dual-licensed with GPL-2.0) |

## QUASI88

[QUASI88](https://www.eonet.ne.jp/~showtime/quasi88/) is by Showzoh Fukunaga. Its manual (`third_party/quasi88/doc/manual.txt`, 著作権／免責) states that QUASI88, except its sound output section, is free software under terms following the modified BSD license. The sound output section, which used MAME/XMAME code and fmgen, was removed from this snapshot; see [source import](source-import.md). The manual also notes that some built-in images use ICOOON MONO icons and the Shinonome font, which remain under their authors' copyrights.

## ymfm

[ymfm](https://github.com/aaronsgiles/ymfm) is BSD-3-Clause; its license is in `third_party/ymfm/LICENSE`. fmgen is not used.

## Shared frontend

The shared RetroArch Slang shader runtime uses librashader under its GPL-3.0-only option, so a shader-enabled Kairo88 build is distributed under GPLv3 terms using first-party "or later" permission. See [shared licensing](../shared/docs/licensing.md).

## Firmware and games

Kairo88 does not include PC-8801 ROMs, operating systems, or games. Users supply any firmware and games they need.
