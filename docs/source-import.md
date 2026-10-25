# Source import policy

[← Kairo88](../README.md)

This repository is independent. Its only Git remote is `origin`, pointing to this project's repository. The first-party Kairo frontend is pinned at `shared/` as a submodule. Do not add an `upstream` remote, fork relationship, other submodule, subtree sync, or automated upstream merge. Third-party source is copied in as pinned snapshots under `third_party/`.

Two PC-8801 cores are pinned while they are compared on Android devices. The one not chosen will be removed.

## MAME snapshot

`third_party/mame/` holds the part of MAME 0.289 that the PC-8801 driver needs.

| | |
| --- | --- |
| Release | `mame0289`, tag commit `f34f02505e32c1993c6a782b6814232cbfc74e36` (2026-07-29) |
| Archive | <https://codeload.github.com/mamedev/mame/tar.gz/refs/tags/mame0289> |
| Archive SHA-256 | `0929cc749afabcef892900e10dd90bd8b05f94a7dde8f367ac6a5d2082589f84` |
| Driver | `src/mame/nec/pc8801.cpp` |
| Imported | 2026-10-21 |

[`tools/import_mame.py`](../tools/import_mame.py) chose the files from the extracted archive:

1. MAME's own `scripts/build/makedep.py sourcesproject` (the logic behind `make SOURCES=...`) lists the driver's sources and the CPU, sound, machine, video, bus and format options it uses.
2. Those options are resolved to files through MAME's `scripts/src/*.lua` file lists, and the always-present shared floppy format code is added.
3. `src/emu`, `src/lib/util` and the top level of `src/osd` are copied whole.
4. The set is closed over `#include`: each included header is added, along with the source file next to any device, driver or format header, until nothing new is reached. Generated layout and font inputs are added from their source files.
5. Every 3rdparty library that a selected file includes, plus the libraries the core always links (expat, zlib, lzma, zstd, utf8proc, flac, softfloat3, rapidjson, asio), is copied whole so its license files stay with it.
6. MAME's `COPYING`, `README.md` and `docs/legal/` are copied.

The import produced 6,127 files. `third_party/mame/KAIRO_IMPORT.txt` records the driver, the 3rdparty libraries, and the full `makedep.py` output. To repeat the import, run `tools/import_mame.py <extracted-mame-root> <new-destination> src/mame/nec/pc8801.cpp` and compare with the committed tree.

After copying, nested Git metadata (`.gitignore`, `.gitattributes`, `.gitmodules`, `.github/`) was removed so that every copied file is tracked. One symbolic link in zstd's CLI tests (`tests/cli-tests/bin/unzstd`) could not be created on Windows and is absent.

There are no local changes to the MAME sources.

## QUASI88 snapshot

`third_party/quasi88/` holds QUASI88 0.7.4 by Showzoh Fukunaga, from the author's [download page](https://www.eonet.ne.jp/~showtime/quasi88/download.html).

| | |
| --- | --- |
| Archive | <https://www.eonet.ne.jp/~showtime/quasi88/release/quasi88-0.7.4.zip> |
| Archive SHA-256 | `e61f110ae2adc3ee10b0e6718f58e60f796a272c25309fd0950e47e70c2844e1` |
| Released | 2026-05-01 |
| Imported | 2026-10-21 |

The archive was extracted unchanged except that QUASI88's sound output section was removed: `src/snddrv/fmgen/` (fmgen), `src/snddrv/xmame/` and `src/snddrv/xmame-old/` (MAME/XMAME sound code). fmgen requires the author's consent for commercial use, and the MAME/XMAME code is under the pre-2016 MAME license, which forbids sale; neither may ship in Kairo88. QUASI88's own `src/snddrv/snddrv.h` interface remains. Other QUASI88 sources still refer to the removed code through build switches and `#include`s; the Android build will provide the sound output through ymfm instead.

### Local changes

Two kinds of machine state that QUASI88 leaves out of its state files are saved in new sections, so a loaded state runs exactly as the saved machine would have. A state file without these sections still loads as before. ROM loading is split out so it can run again.

| File | Change |
| --- | --- |
| `src/pc88/pc88sub.c` | `sub_total_state`, the sub CPU's count toward its next frame-update break, moves from a static local in `sub_INT_update()` to file scope and is saved in section `SUB2`. Without it the CPUs' scheduling after a load differs, and games diverge within seconds. |
| `src/pc88/fdc.c`, `src/pc88/fdc.h` | Each drive's head position within its track and the last sector ID the FDC read are saved in section `FDC3`. `drive_stateload_position()` puts them back once the disk images are reopened, which otherwise leaves both drives at sector 0. |
| `src/control.c`, `src/quasi88.c` | Call `drive_stateload_position()` after the disk images reopen on a state load. |
| `src/pc88/memory.c`, `src/pc88/memory.h` | ROM loading moves out of `memory_allocate()` into `memory_load_rom()`, which reads every ROM image into the memory already allocated. QUASI88 cannot restart in a process, so the core calls it on every boot to pick up firmware imported after QUASI88 started. |
| `src/screen/putimage.c`, `src/screen/screen.h` | `screen_drawn_400` records whether the last VRAM transfer drew 400 lines (`V_HIRESO`). The core runs QUASI88 with `-skipline` drawing (each 200-line row once, on the buffer's even lines) and hands those 200 rows over with `line_repeat()` 2; this says which kind of frame the buffer holds (Kairo#50). |
| `src/osdepend/unix/file-op.c` | A file already open is found by its device and inode, and where the system has no inode numbers (Windows, where `st_ino` is 0) also by the path it was opened by. Without the path, every disk on a Windows host is taken for the one already open, so drive 2 reads drive 1's disk. |

Each change is marked with a `Kairo88` comment.

## ymfm snapshot

`third_party/ymfm/` holds [ymfm](https://github.com/aaronsgiles/ymfm) by Aaron Giles at commit `81aec25ccbb98f4873a255f7551ac4dadac59b4a`, the same commit Kairo98 pins. It will supply the YM2203 and YM2608 sound for the QUASI88 core.

| | |
| --- | --- |
| Archive | <https://codeload.github.com/aaronsgiles/ymfm/tar.gz/81aec25ccbb98f4873a255f7551ac4dadac59b4a> |
| Archive SHA-256 | `9bcf001b30f53754c49cf2fb864792a64702b8aeebb105baa908ad5dd33f443f` |
| Imported | 2026-10-21 |

Nested `.gitignore` files were omitted; the `LICENSE` file, source, and examples are preserved.
