# QUASI88 core

[← Kairo88](../README.md)

`core/` builds QUASI88 (`third_party/quasi88/`, with the state file changes listed in [source-import.md](source-import.md#local-changes)) as a library that the app drives one frame at a time. The same build runs on the host, so the core can be tested without a device.

## Layout

| Path | Contents |
| --- | --- |
| `core/include/kairo88_core.h` | The core API: start once, boot a game, run a frame, read the frame and audio, send keys and pads, change disks, reset, save and load states. |
| `core/kairo88_core.c` | The API over QUASI88's `quasi88_start()` / `quasi88_loop()`, its disk, reset and state functions. |
| `core/port/` | The QUASI88 system layer (`config.h`, `device.h`, graphics, events, wait, state and menu hooks). It stands in for one of QUASI88's `sysdepend/` ports. |
| `core/sound/snddrv_ymfm.cpp` | QUASI88's sound driver interface (`snddrv.h`) on ymfm. |
| `core/test/boot_test.c` | A headless host program that boots a machine and writes frames and audio. |
| `core/test/restart_test.c` | Boots several games in one process and checks that a save state round trip reproduces the picture and audio exactly. |

QUASI88's own OS layer for Unix (`osdepend/unix/file-op.c`) is used as it is.

## One machine per process

QUASI88 keeps its state in globals and cannot be shut down and started again in one process. `k88_init()` starts it once; each game is a `k88_boot()`, which ejects both drives, inserts the game's disks and resets into the game's BASIC mode, CPU clock and sound board. With no disk in drive A the machine boots ROM BASIC.

## Save states

`k88_state_save()` and `k88_state_load()` use QUASI88's state files, which name the disk images they were saved with: a loaded state reopens them by path. The port adds the sound driver's whole state (sections `KSNL` and `KSND`): QUASI88 itself only replays the sound registers after a load, which restarts held notes. A load holds the driver's rendering until QUASI88 has finished, then puts the chips back exactly.

## Frames

The port's graphics layer gives QUASI88 a 32-bit buffer of RGBA8 pixels (`0xAABBGGRR` as a little-endian `uint32_t`), the layout of an Android `RGBA_8888` window and an OpenGL RGBA texture. With `-notool -nostatus` QUASI88 draws only the machine's 640×400 screen. The wait layer never sleeps; the host paces frames.

## Sound

QUASI88 emulates the sound boards' timers, status flags and ADPCM RAM itself and only sends the sound driver its register writes. The driver feeds them to ymfm: a YM2203 at 3.9936 MHz for sound board I, a YM2608 at 7.9872 MHz for sound board II. Each write is applied at the CPU cycle it happened, counted from QUASI88's vsync count and CPU state counters, by rendering the chip up to that moment first.

The YM2608's rhythm instruments need the chip's internal ROM, which is not distributed. The driver plays them from the user's `2608_bd.wav`, `2608_sd.wav`, `2608_top.wav`, `2608_hh.wav`, `2608_tom.wav` and `2608_rim.wav` in the ROM folder, using the rhythm registers' key-on, levels and panning.

The beeper (port 0x40: bit 5 gates a 2.4 kHz tone, bit 7 drives the speaker directly) is mixed in. The PCG-8100 sound board and drive-noise samples are not emulated.

Output is 16-bit stereo at 48 kHz. The host reads it with `k88_audio_read()`; one second is buffered.

## Host test

`boot_test` builds wherever CMake and a C/C++ compiler are available. On Windows the build used [llvm-mingw](https://github.com/mstorsjo/llvm-mingw):

```sh
cmake -S core -B .build/host -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release
cmake --build .build/host
.build/host/boot_test 1800 300 .build/out/game -- -romdir bios/ -notool -nostatus -sd2 game.d88
```

Arguments after `--` are QUASI88 options. It writes `game-NNNNN.ppm` every 300 frames and `game.wav` with all audio.

```sh
.build/host/restart_test 900 .build/out/restart .build/out/restart.sta bios/ v2-game.d88 v1-game.d88
```

boots the first disk (V2, 8 MHz, sound board II), the second (V1S, 4 MHz, sound board I) and the first again, then saves a state, runs on, loads it and runs the same frames again. It fails unless both runs end on the same picture with the same audio.

```sh
.build/host/tape_test bios/ .build/out/tape 1 "Apploon.t88" "load=MON|R|G8F00" frames=36000
```

boots N88-BASIC (V1S here; 0 N-BASIC, 2 V1H, 3 V2) with the tape in the deck and runs its steps: `load=` runs the app's loader (`kairo88_loader.c`) with those lines, `wait=`, `type=` and `frames=` drive the machine directly. It prints the text screen and the tape position after each step and writes `tape-NN.ppm`.

## Tapes

A tape game boots ROM BASIC with its T88 or CMT image in the deck, and its loader types the commands that load it, each at the prompt it waits for (BASIC's `Ok`, the monitor's `]` or `*`), answering V1/V2's `How many files` with Return. The catalog gives a tape's loader when its source names one (Neo Kobe's `{V1 mode, MON R G8F00}`, parsed in `tools/catalog_config.py`); otherwise it is `LOAD"CAS:"` and `RUN`, or `CLOAD"<name>` and `RUN` in N-BASIC, which needs the program's name. The machine runs as fast as the device can while the loader types and while the cassette motor is on.

## Known problems

- Silpheed (Game Arts) does not boot: every dump tried falls back to N88-BASIC. Hydlide (V1 mode) and Sorcerian (V2 mode) boot and play.
