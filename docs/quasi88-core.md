# QUASI88 core

[← Kairo88](../README.md)

`core/` builds QUASI88 (`third_party/quasi88/`, unmodified) as a library that the app drives one frame at a time. The same build runs on the host, so the core can be tested without a device.

## Layout

| Path | Contents |
| --- | --- |
| `core/include/kairo88_core.h` | The core API: start, run a frame, read the frame and audio, send keys, stop. |
| `core/kairo88_core.c` | The API over QUASI88's `quasi88_start()` / `quasi88_loop()` / `quasi88_stop()`. |
| `core/port/` | The QUASI88 system layer (`config.h`, `device.h`, graphics, events, wait, state and menu hooks). It stands in for one of QUASI88's `sysdepend/` ports. |
| `core/sound/snddrv_ymfm.cpp` | QUASI88's sound driver interface (`snddrv.h`) on ymfm. |
| `core/test/boot_test.c` | A headless host program that boots a machine and writes frames and audio. |

QUASI88's own OS layer for Unix (`osdepend/unix/file-op.c`) is used as it is.

## Frames

The port's graphics layer gives QUASI88 a 32-bit buffer of `0xAARRGGBB` pixels. With `-notool -nostatus` QUASI88 draws only the machine's 640×400 screen. The wait layer never sleeps; the host paces frames.

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

## Known problems

- Silpheed (Game Arts) does not boot: every dump tried falls back to N88-BASIC. Hydlide (V1 mode) and Sorcerian (V2 mode) boot and play.
