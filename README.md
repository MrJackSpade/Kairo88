<!-- kairo:discord -->
<p align="center">
  <a href="https://discord.gg/ZkYQ3bPTTq"><img src="https://raw.githubusercontent.com/MrJackSpade/Kairo/main/docs/assets/discord-banner.svg" alt="Join the Kairo Discord" width="49%"></a>
</p>
<!-- /kairo:discord -->

<!-- kairo:family -->
<h3 align="center">Other Kairo apps</h3>
<table align="center">
  <tr>
    <td align="center" width="33%"><a href="https://github.com/MrJackSpade/Kairo98"><img src="https://raw.githubusercontent.com/MrJackSpade/Kairo98/main/kairo98/src/main/res/drawable-nodpi/kairo98_icon_art.png" width="96" alt="Kairo98"><br><b>Kairo98</b></a><br>PC-98</td>
    <td align="center" width="33%"><a href="https://github.com/MrJackSpade/KairoDos"><img src="https://raw.githubusercontent.com/MrJackSpade/KairoDos/main/kairodos/src/main/res/drawable-nodpi/kairodos_icon_art.png" width="96" alt="KairoDos"><br><b>KairoDos</b></a><br>DOS</td>
    <td align="center" width="33%"><a href="https://github.com/MrJackSpade/Kairo68"><img src="https://raw.githubusercontent.com/MrJackSpade/Kairo68/main/kairo68/src/main/res/drawable-nodpi/product_icon_art.png" width="96" alt="Kairo68"><br><b>Kairo68</b></a><br>X68000</td>
  </tr>
  <tr>
    <td align="center" width="33%"><a href="https://github.com/MrJackSpade/KairoTowns"><img src="https://raw.githubusercontent.com/MrJackSpade/KairoTowns/main/kairotowns/src/main/res/drawable-nodpi/product_icon_art.png" width="96" alt="KairoTowns"><br><b>KairoTowns</b></a><br>FM Towns</td>
    <td align="center" width="33%"><a href="https://github.com/MrJackSpade/KairoMSX"><img src="https://raw.githubusercontent.com/MrJackSpade/KairoMSX/main/kairomsx/src/main/res/drawable-nodpi/product_icon_art.png" width="96" alt="KairoMSX"><br><b>KairoMSX</b></a><br>MSX</td>
  </tr>
</table>
<p align="center">Every Kairo app is built on <a href="https://github.com/MrJackSpade/Kairo">Kairo</a>, the shared frontend for the game library, controls, and settings.</p>
<!-- /kairo:family -->

# Kairo88

**Play your own PC-88 games on Android phones and handhelds.** Kairo88 is an Android emulator for the NEC PC-8801 series, built on the [Kairo shared frontend](https://github.com/MrJackSpade/Kairo): the same game library, controller and touch controls, on-screen keyboard, and per-game settings as [Kairo98](https://github.com/MrJackSpade/Kairo98) and [KairoDos](https://github.com/MrJackSpade/KairoDos). It is an independent project, not an official release of any emulator it builds on.

## Status

Early development. There is no release yet. The Android app runs [QUASI88](https://www.eonet.ne.jp/~showtime/quasi88/) with its sound output rebuilt on [ymfm](https://github.com/aaronsgiles/ymfm). [MAME](https://www.mamedev.org/)'s PC-8801 driver is also pinned for comparison and will be removed once the choice is settled.

## Games and firmware

Games and the PC-8801's BIOS are not supplied; you provide the BIOS from your own machine.

Kairo88 reads D88 floppy images (`.d88`, `.88d`), loose or inside ZIP and 7z archives. A game's disks show as one entry, and its second disk goes in drive 2 when it starts. Menu › Machine lists every BIOS file, each marked Required or Optional. Pick each file, or use **Import all from a folder…** to import every file the app recognizes in a folder you choose. N88-BASIC (`n88.rom`), its extension 0 (`n88_0.rom`) and the disk sub-system (`disk.rom`) are required; the YM2608 rhythm samples (`2608_*.wav`) play sound board II's drums.

## Documentation

- [Licensing and credits](docs/licensing.md): code license and third-party components
- [Source provenance](docs/source-import.md): where the emulator sources came from and what was copied
- [QUASI88 core](docs/quasi88-core.md): how the QUASI88 core is built, driven and tested

## Help and community

<!-- kairo:community -->
- **Report a game problem** in [Issues](https://github.com/MrJackSpade/Kairo88/issues). Include the title, file format, and what happened.
- **Chat** with other players on [Discord](https://discord.gg/ZkYQ3bPTTq).
<!-- /kairo:community -->

## Building from source

<!-- kairo:clone -->
Clone with submodules to get the pinned shared frontend:

```sh
git clone --recurse-submodules https://github.com/MrJackSpade/Kairo88.git
```
<!-- /kairo:clone -->

Build with the pinned shared Gradle setup (see [Android build](shared/docs/android-build.md)), which needs JDK 17, the Android SDK and NDK, and Rust with the `aarch64-linux-android` target:

```sh
./gradlew :kairo88:assembleWithImagesDebug
```

On Windows, build with `shared\tools\build.ps1`; it works from a path with spaces.

## License

<!-- kairo:license -->
First-party code is [GPL-2.0-or-later](LICENSE.md). The app as distributed is GPLv3 because it bundles GPLv3 components. Third-party components keep their own licenses; see [licensing and credits](docs/licensing.md) and [source provenance](docs/source-import.md).
<!-- /kairo:license -->
