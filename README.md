# Kairo88

**Play your own PC-88 games on Android phones and handhelds.** Kairo88 is an Android emulator for the NEC PC-8801 series, built on the [Kairo shared frontend](https://github.com/MrJackSpade/Kairo): the same game library, controller and touch controls, on-screen keyboard, and per-game settings as [Kairo98](https://github.com/MrJackSpade/Kairo98) and [KairoDos](https://github.com/MrJackSpade/KairoDos). It is an independent project, not an official release of any emulator it builds on.

## Status

Early development. There is no release yet. The Android app runs [QUASI88](https://www.eonet.ne.jp/~showtime/quasi88/) with its sound output rebuilt on [ymfm](https://github.com/aaronsgiles/ymfm). [MAME](https://www.mamedev.org/)'s PC-8801 driver is also pinned for comparison and will be removed once the choice is settled.

## Games and firmware

Games, operating systems, and firmware are not supplied. The PC-8801 needs its system ROMs (N88-BASIC, N-BASIC, disk and kanji ROMs), which you provide from your own machine.

Kairo88 reads D88 floppy images (`.d88`, `.88d`), loose or inside ZIP and 7z archives. To import firmware, choose the folder holding your ROM files in the first-run setup or in Menu › Machine; Kairo88 copies the files it recognizes. N88-BASIC (`n88.rom`), its extension 0 (`n88_0.rom`) and the disk sub-system (`disk.rom`) are needed to start games; the YM2608 rhythm samples (`2608_*.wav`) play sound board II's drums.

## Documentation

- [Licensing and credits](docs/licensing.md): code license and third-party components
- [Source provenance](docs/source-import.md): where the emulator sources came from and what was copied
- [QUASI88 core](docs/quasi88-core.md): how the QUASI88 core is built, driven and tested

## Help and community

- **Chat** with other players on [Discord](https://discord.gg/ZkYQ3bPTTq).

### Related projects

- [Kairo98](https://github.com/MrJackSpade/Kairo98): PC-98 emulator for Android
- [KairoDos](https://github.com/MrJackSpade/KairoDos): DOS emulator for Android
- [Kairo68](https://github.com/MrJackSpade/Kairo68): X68000 emulator for Android
- [KairoTowns](https://github.com/MrJackSpade/KairoTowns): FM Towns emulator for Android
- [KairoMSX](https://github.com/MrJackSpade/KairoMSX): MSX emulator for Android
- [Kairo](https://github.com/MrJackSpade/Kairo): shared frontend used by every Kairo app

## Building from source

Clone with submodules to get the pinned shared frontend:

```sh
git clone --recurse-submodules https://github.com/MrJackSpade/Kairo88.git
```

Build with the pinned shared Gradle setup (see [Android build](shared/docs/android-build.md)), which needs JDK 17, the Android SDK and NDK, and Rust with the `aarch64-linux-android` target:

```sh
./gradlew :kairo88:assembleWithImagesDebug
```

On Windows, build from a drive letter mapped to the checkout (`subst`) when its path contains a space.

## License

First-party code is [GPL-2.0-or-later](LICENSE.md). Third-party components keep their own licenses; see [licensing and credits](docs/licensing.md).
