# Kairo88

**Play your own PC-88 games on Android phones and handhelds.** Kairo88 is an Android emulator for the NEC PC-8801 series, built on the [Kairo shared frontend](https://github.com/MrJackSpade/Kairo): the same game library, controller and touch controls, on-screen keyboard, and per-game settings as [Kairo98](https://github.com/MrJackSpade/Kairo98) and [KairoDos](https://github.com/MrJackSpade/KairoDos). It is an independent project, not an official release of any emulator it builds on.

## Status

Early development. There is no release yet. Two emulator cores are pinned in this repository for evaluation on Android devices, and one will be chosen:

- [MAME](https://www.mamedev.org/)'s PC-8801 driver
- [QUASI88](https://www.eonet.ne.jp/~showtime/quasi88/), with its sound output to be rebuilt on [ymfm](https://github.com/aaronsgiles/ymfm)

## Games and firmware

Games, operating systems, and firmware are not supplied. The PC-8801 needs its system ROMs (N88-BASIC, N-BASIC, disk and kanji ROMs), which you provide from your own machine.

Both cores read D88 floppy images.

## Documentation

- [Licensing and credits](docs/licensing.md): code license and third-party components
- [Source provenance](docs/source-import.md): where the emulator sources came from and what was copied

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

The Android build is not set up yet.

## License

First-party code is [GPL-2.0-or-later](LICENSE.md). Third-party components keep their own licenses; see [licensing and credits](docs/licensing.md).
