package com.mrjackspade.kairo88

import com.mrjackspade.kairo.frontend.CatalogGame
import com.mrjackspade.kairo.frontend.ControllerGuestSpec
import com.mrjackspade.kairo.frontend.ControllerLayout
import com.mrjackspade.kairo.frontend.DiskImageEntry
import com.mrjackspade.kairo.frontend.DiskImageLibrary
import com.mrjackspade.kairo.frontend.FirmwareSet
import com.mrjackspade.kairo.frontend.JoystickInputRouter
import com.mrjackspade.kairo.frontend.LayeredGameCatalog
import com.mrjackspade.kairo.frontend.LibraryStrings
import com.mrjackspade.kairo.frontend.MachineActivity
import com.mrjackspade.kairo.frontend.MachineFirstRun
import com.mrjackspade.kairo.frontend.StartupCommands
import com.mrjackspade.kairo.frontend.UiText
import com.mrjackspade.kairo.frontend.R as FrontendR
import com.mrjackspade.kairo.frontend.companionFor

import java.io.File
import java.util.concurrent.atomic.AtomicBoolean

/** Kairo88: the PC-8801 on QUASI88, in the shared Kairo frontend. */
class MainActivity : MachineActivity<DiskImageEntry>() {
    override val identity = Identity("KAIRO88", 0xffFB981A.toInt(), "Kairo88", "PC-88", "kairo88", LibraryStrings("KAIRO88", "Kairo88"),
        UiText.res(R.string.credits, "Kairo88", "QUASI88", "ymfm", "MrJackSpade/Kairo88"),
        UiText.res(R.string.library_empty), UiText.res(FrontendR.string.kairo_library_expired),
        MachineFirstRun.Folder(UiText.res(FrontendR.string.kairo_folder_title), UiText.res(R.string.folder_description, "PC-88", "Kairo88"),
            UiText.res(FrontendR.string.kairo_folder_button)),
        UiText.res(FrontendR.string.kairo_saves_group), "MrJackSpade/Kairo88")
    override val machine = Machine(640, 400, 640.0 / 400.0, "state.q88")
    override val bridge get() = Pc88Native.machine
    override val keyboard = Pc88Keys.layout
    override val maxKeyCode = Pc88Keys.MAX_CODE
    override fun guestKey(androidKey: Int) = Pc88Keys.fromAndroid(androidKey)
    override val controllerSpec = ControllerGuestSpec("PC-88", Pc88Keys.bindableCodes, Pc88Keys::label,
        Pc88Keys.layout.modifiers, JoystickInputRouter.DEFAULT_CONTROLS.zip(Pc88Keys.joystickLabels),
        UiText.res(R.string.joystick_help),
        ControllerGuestSpec.STANDARD_ACTIONS, Pc88Keys.layout)
    override val bindingsCodec = Pc88Keys.bindings
    override val artworkDownloadsEnabled: Boolean get() = resources.getBoolean(R.bool.catalog_art_download_enabled)

    private val format = DiskImageLibrary.Format(mapOf("d88" to "fd", "88d" to "fd", "t88" to "tape", "cmt" to "tape"),
        mapOf("fd" to UiText.res(FrontendR.string.kairo_floppy_disk), "tape" to UiText.res(R.string.cassette_tape)), mapOf("fd" to 64L * 1024 * 1024, "tape" to 16L * 1024 * 1024),
        "PC-88")
    private val disks by lazy { DiskImageLibrary(this, format) }
    override fun createCatalog() = LayeredGameCatalog(this, Pc88Catalog)
    override fun createLibrary(catalog: LayeredGameCatalog) = disks

    override val machineOptions = Pc88Native.options
    private fun isDisk(entry: DiskImageEntry) = format.kind(entry.displayName) == "fd"
    private fun isTape(entry: DiskImageEntry) = format.kind(entry.displayName) == "tape"

    override val mediaSlots = listOf(MediaSlot(UiText.res(R.string.drive_1), ::isDisk),
        MediaSlot(UiText.res(FrontendR.string.kairo_drive_2), ::isDisk), MediaSlot(UiText.res(FrontendR.string.kairo_cassette), ::isTape))

    private val firmwareDir by lazy { File(filesDir, "firmware") }
    override val firmwareSets by lazy {
        fun description(name: String, what: UiText) = UiText.res(FrontendR.string.kairo_firmware_part, name, what)
        fun rom(name: String, label: UiText, description: UiText, required: Boolean, sizes: Set<Int>,
                vararg aliases: String) = FirmwareSet.Part(name, label, description(name, description), setOf(name) + aliases,
            required, sizes.map(Int::toLong).toSet())
        fun rom(name: String, label: Int, description: Int, required: Boolean, sizes: Set<Int>, vararg aliases: String) =
            rom(name, UiText.res(label), UiText.res(description), required, sizes, *aliases)
        listOf(UiText.res(FrontendR.string.kairo_firmware_title, "PC-88") to FirmwareSet(firmwareDir, listOf(
            rom("n88.rom", UiText.fixed("N88-BASIC"), UiText.res(R.string.firmware_n88_description), true, setOf(0x8000)),
            rom("n88_0.rom", R.string.firmware_n88_extension_0, R.string.firmware_n88_0_description, true, setOf(0x2000),
                "n88ext0.rom"),
            rom("n88_1.rom", R.string.firmware_n88_extension_1, R.string.firmware_n88_extension_description, false,
                setOf(0x2000), "n88ext1.rom"),
            rom("n88_2.rom", R.string.firmware_n88_extension_2, R.string.firmware_n88_extension_description, false,
                setOf(0x2000), "n88ext2.rom"),
            rom("n88_3.rom", R.string.firmware_n88_extension_3, R.string.firmware_n88_extension_description, false,
                setOf(0x2000), "n88ext3.rom"),
            rom("n80.rom", UiText.fixed("N-BASIC"), UiText.res(R.string.firmware_n80_description, "PC-8001"), false,
                setOf(0x8000), "n88n.rom"),
            rom("disk.rom", R.string.firmware_disk, R.string.firmware_disk_description, true, setOf(0x800, 0x2000),
                "n88sub.rom"),
            rom("kanji1.rom", R.string.firmware_kanji_1, R.string.firmware_kanji_1_description, false, setOf(0x20000),
                "n88knj1.rom"),
            rom("kanji2.rom", R.string.firmware_kanji_2, R.string.firmware_kanji_2_description, false, setOf(0x20000),
                "n88knj2.rom"),
            rom("jisyo.rom", FrontendR.string.kairo_firmware_dictionary, R.string.firmware_dictionary_description, false,
                setOf(0x80000), "n88jisho.rom"),
            rom("font.rom", R.string.firmware_font, R.string.firmware_font_description, false, setOf(0x800, 0x1000)))),
            UiText.res(R.string.rhythm_samples) to FirmwareSet(firmwareDir, listOf("bd" to R.string.rhythm_bass_drum,
                "sd" to R.string.rhythm_snare_drum, "top" to R.string.rhythm_top_cymbal, "hh" to R.string.rhythm_hi_hat,
                "tom" to R.string.rhythm_tom, "rim" to R.string.rhythm_rim_shot).map { (name, label) ->
                FirmwareSet.Part("2608_$name.wav", UiText.res(label), description("2608_$name.wav", UiText.res(R.string.rhythm_description)),
                    setOf("2608_$name.wav"), true)
            }))
    }

    override fun startGame(entry: DiskImageEntry, game: CatalogGame, cancelled: AtomicBoolean,
                           progress: (String) -> Unit) {
        // The machine boots without the rhythm samples; only sound board II's drums need them.
        requireFirmware(listOf(firmwareSets.first().second))
        val home = File(filesDir, "quasi88").apply { mkdirs() }
        val options = optionValues(game)
        // The lines typed at BASIC's prompt to start the game: a tape's load, or a disk's own commands.
        val settings = Pc88Native.settings(options) + StartupCommands.setting(game.startupCommands(options))
        if (isTape(entry)) {
            // A tape game boots BASIC with the tape in the deck.
            val tape = disks.prepare(entry, cancelled, progress)
            check(Pc88Native.machine.start(firmwareDir.absolutePath, home.absolutePath,
                listOf(null, null, tape.absolutePath), settings)) {
                "Another machine is still running"
            }
            awaitRunning()
            mediaStarted(listOf(null, null, entry))
            return
        }
        val second = companionFor(game.mediaId("floppyB"), entry, launchEntries,
            UiText.res(FrontendR.string.kairo_second_disk_missing), ::isDisk)
        val diskA = disks.prepare(entry, cancelled, progress)
        val diskB = second?.let { disks.prepare(it, cancelled, progress) }
        check(Pc88Native.machine.start(firmwareDir.absolutePath, home.absolutePath,
            listOf(diskA.absolutePath, diskB?.absolutePath), settings)) {
            "Another machine is still running"
        }
        awaitRunning()
        mediaStarted(listOf(entry, second))
    }
}
