<!-- kairo:discord -->
<p align="center">
  <a href="https://discord.gg/ZkYQ3bPTTq"><img src="https://raw.githubusercontent.com/MrJackSpade/Kairo/main/docs/assets/discord-banner.svg" alt="Join the Kairo Discord" width="49%"></a>
  <a href="https://play.google.com/store/apps/details?id=com.loxifi.kairo88"><img src="https://raw.githubusercontent.com/MrJackSpade/Kairo/main/docs/assets/donate-banner.svg" alt="Donate: buying Kairo88 on Google Play is the donation; it adds no features" width="49%"></a>
</p>
<!-- /kairo:discord -->

<!-- kairo:family -->
<h3 align="center">Other Kairo apps</h3>
<table align="center">
  <tr>
    <td align="center" width="33%"><a href="https://github.com/MrJackSpade/Kairo98"><img src="https://raw.githubusercontent.com/MrJackSpade/Kairo98/main/kairo98/src/main/res/drawable-nodpi/product_icon_art.png" width="96" alt="Kairo98"><br><b>Kairo98</b></a><br>PC-98</td>
    <td align="center" width="33%"><a href="https://github.com/MrJackSpade/KairoDos"><img src="https://raw.githubusercontent.com/MrJackSpade/KairoDos/main/kairodos/src/main/res/drawable-nodpi/product_icon_art.png" width="96" alt="KairoDos"><br><b>KairoDos</b></a><br>DOS</td>
    <td align="center" width="33%"><a href="https://github.com/MrJackSpade/Kairo68"><img src="https://raw.githubusercontent.com/MrJackSpade/Kairo68/main/kairo68/src/main/res/drawable-nodpi/product_icon_art.png" width="96" alt="Kairo68"><br><b>Kairo68</b></a><br>X68000</td>
  </tr>
  <tr>
    <td align="center" width="33%"><a href="https://github.com/MrJackSpade/KairoTowns"><img src="https://raw.githubusercontent.com/MrJackSpade/KairoTowns/main/kairotowns/src/main/res/drawable-nodpi/product_icon_art.png" width="96" alt="KairoTowns"><br><b>KairoTowns</b></a><br>FM Towns</td>
    <td align="center" width="33%"><a href="https://github.com/MrJackSpade/KairoMSX"><img src="https://raw.githubusercontent.com/MrJackSpade/KairoMSX/main/kairomsx/src/main/res/drawable-nodpi/product_icon_art.png" width="96" alt="KairoMSX"><br><b>KairoMSX</b></a><br>MSX</td>
    <td align="center" width="33%"><a href="https://github.com/MrJackSpade/KairoST"><img src="https://raw.githubusercontent.com/MrJackSpade/KairoST/main/kairost/src/main/res/drawable-nodpi/product_icon_art.png" width="96" alt="KairoST"><br><b>KairoST</b></a><br>Atari ST</td>
  </tr>
</table>
<p align="center">Every Kairo app is built on <a href="https://github.com/MrJackSpade/Kairo">Kairo</a>, the shared frontend for the game library, controls, and settings.</p>
<!-- /kairo:family -->

# Kairo88

**Play your own PC-88 games on Android phones and handhelds.** Kairo88 is an Android emulator for the NEC PC-8801 series, built on the [Kairo shared frontend](https://github.com/MrJackSpade/Kairo): the same game library with artwork, controller and touch controls, on-screen keyboard, save states, and per-game settings as [Kairo98](https://github.com/MrJackSpade/Kairo98) and [KairoDos](https://github.com/MrJackSpade/KairoDos). It runs [QUASI88](https://www.eonet.ne.jp/~showtime/quasi88/) with its FM sound rebuilt on [ymfm](https://github.com/aaronsgiles/ymfm). It is an independent project, not an official release of either.

## Download

<!-- kairo:download -->
- **[GitHub Releases](https://github.com/MrJackSpade/Kairo88/releases)** (free): install the APK yourself. The larger `-with-images` APK also bundles catalog artwork.

Requires an ARM64 device running Android 8.0 or newer.
<!-- /kairo:download -->

## Quick start

1. Install Kairo88 from [GitHub Releases](https://github.com/MrJackSpade/Kairo88/releases).
2. Open Kairo88 and choose the folder that holds your PC-88 games. You can change it later in **Menu › Manage library**.
3. Import the PC-8801 BIOS from your own machine in **Menu › Machine** (see [Firmware](#firmware)).
4. Select a game and tap **Play**.

Games and the PC-8801's BIOS are not supplied.

## Supported formats

| Type | Formats |
| --- | --- |
| Floppies | D88, 88D |
| Cassette tapes | T88, CMT |
| Archives | Any of the above inside ZIP or 7z archives |

A game's disks show as one library entry, and its second disk goes in drive 2 when it starts. A tape game boots BASIC and types the commands that load the tape for you; a game with several tapes shows as one entry, with its other tapes ready to insert. **Menu › Disks** changes disks and tapes during play.

## Firmware

Kairo88 needs N88-BASIC (`n88.rom`), its extension 0 (`n88_0.rom`) and the disk sub-system (`disk.rom`) from your own machine. The YM2608 rhythm samples (`2608_*.wav`) are optional; they play sound board II's drums. **Menu › Machine** lists every BIOS file, each marked Required or Optional. Pick each file, or use **Import all from a folder…** to import every file the app recognizes in a folder you choose.

## Machine settings

**Menu › Machine** sets these for all games; each game can override them in its **Game settings**, and the catalog sets them for games that need something else.

- **BASIC mode:** N88 V2 (the default), N88 V1H, N88 V1S or N-BASIC.
- **CPU clock:** 8 MHz (the default) or 4 MHz.
- **Sound:** Sound board II (the default) or the original sound board.
- **Port:** a joystick (the default) or a mouse in the joystick port.

## Documentation

- [Licensing and credits](docs/licensing.md): code license and third-party components
- [Source provenance](docs/source-import.md): where the emulator sources came from and what was copied

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

Build the app, and optionally install it, with the shared build script:

```powershell
shared\tools\build.ps1 -Install <adb serial>
```

How the QUASI88 core is built, driven and tested, host tests and UI tests: see [development](docs/development.md).

## License

<!-- kairo:license -->
First-party code is [GPL-2.0-or-later](LICENSE.md). The app as distributed is GPLv3 because it bundles GPLv3 components. Third-party components keep their own licenses; see [licensing and credits](docs/licensing.md) and [source provenance](docs/source-import.md).
<!-- /kairo:license -->
