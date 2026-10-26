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
import com.mrjackspade.kairo.frontend.mediaFor

import java.io.File
import java.util.concurrent.atomic.AtomicBoolean

/** Kairo88: the PC-8801 on QUASI88, in the shared Kairo frontend. */
class MainActivity : MachineActivity<DiskImageEntry>() {
    override val identity = Identity("KAIRO88", 0xffFB981A.toInt(), "Kairo88", "PC-88", "kairo88", LibraryStrings("KAIRO88", "Kairo88"),
        "Kairo88 uses QUASI88 and ymfm. Source and provenance: github.com/MrJackSpade/Kairo88.",
        "Choose a games folder to find disk images and the ZIP and 7z archives holding them",
        "Folder access expired. Select the games folder again.",
        MachineFirstRun.Folder("Choose a games folder", "Select the folder containing your PC-88 games. Kairo88 will " +
            "scan its disk images and the ZIP and 7z archives holding them.", "Choose games folder…"),
        "SAVE STATES", "MrJackSpade/Kairo88")
    override val machine = Machine(640, 400, 640.0 / 400.0, "state.q88")
    override val bridge get() = Pc88Native.machine
    override val keyboard = Pc88Keys.layout
    override val maxKeyCode = Pc88Keys.MAX_CODE
    override fun guestKey(androidKey: Int) = Pc88Keys.fromAndroid(androidKey)
    override val controllerSpec = ControllerGuestSpec("PC-88", Pc88Keys.bindableCodes, Pc88Keys::label,
        Pc88Keys.layout.modifiers, JoystickInputRouter.DEFAULT_CONTROLS.zip(Pc88Keys.joystickLabels),
        "Uses the joystick port, with a joystick in it: the game's Port setting.",
        ControllerGuestSpec.STANDARD_ACTIONS, Pc88Keys.layout)
    override val bindingsCodec = Pc88Keys.bindings
    override val artworkDownloadsEnabled: Boolean get() = resources.getBoolean(R.bool.catalog_art_download_enabled)

    private val format = DiskImageLibrary.Format(mapOf("d88" to "fd", "88d" to "fd", "t88" to "tape", "cmt" to "tape"),
        mapOf("fd" to "Floppy disk", "tape" to "Cassette tape"), mapOf("fd" to 64L * 1024 * 1024, "tape" to 16L * 1024 * 1024),
        "PC-88")
    private val disks by lazy { DiskImageLibrary(this, format) }
    override fun createCatalog() = LayeredGameCatalog(this, Pc88Catalog)
    override fun createLibrary(catalog: LayeredGameCatalog) = disks

    override val machineOptions = Pc88Native.options
    private fun isDisk(entry: DiskImageEntry) = format.kind(entry.displayName) == "fd"
    private fun isTape(entry: DiskImageEntry) = format.kind(entry.displayName) == "tape"

    override val mediaSlots = listOf(MediaSlot("Drive 1", ::isDisk), MediaSlot("Drive 2", ::isDisk),
        MediaSlot("Cassette", ::isTape))

    private val firmwareDir by lazy { File(filesDir, "firmware") }
    override val firmwareSets by lazy {
        fun rom(name: String, label: String, description: String, required: Boolean, sizes: Set<Int>,
                vararg aliases: String) = FirmwareSet.Part(name, label, "$name · $description", setOf(name) + aliases,
            required, sizes.map(Int::toLong).toSet())
        listOf("PC-88 BIOS" to FirmwareSet(firmwareDir, listOf(
            rom("n88.rom", "N88-BASIC", "the BIOS and BASIC that start the machine", true, setOf(0x8000)),
            rom("n88_0.rom", "N88 extension 0", "the first extended BASIC bank, needed to boot", true, setOf(0x2000),
                "n88ext0.rom"),
            rom("n88_1.rom", "N88 extension 1", "an extended BASIC bank some games use", false, setOf(0x2000), "n88ext1.rom"),
            rom("n88_2.rom", "N88 extension 2", "an extended BASIC bank some games use", false, setOf(0x2000), "n88ext2.rom"),
            rom("n88_3.rom", "N88 extension 3", "an extended BASIC bank some games use", false, setOf(0x2000), "n88ext3.rom"),
            rom("n80.rom", "N-BASIC", "BASIC for the oldest PC-8001 style games", false, setOf(0x8000), "n88n.rom"),
            rom("disk.rom", "Disk sub-system", "runs the floppy drives", true, setOf(0x800, 0x2000), "n88sub.rom"),
            rom("kanji1.rom", "Kanji font 1", "everyday Japanese characters", false, setOf(0x20000), "n88knj1.rom"),
            rom("kanji2.rom", "Kanji font 2", "rarer Japanese characters", false, setOf(0x20000), "n88knj2.rom"),
            rom("jisyo.rom", "Dictionary", "kana-to-kanji dictionary for Japanese text entry", false, setOf(0x80000),
                "n88jisho.rom"),
            rom("font.rom", "Character font", "replaces the built-in text font", false, setOf(0x800, 0x1000)))),
            "Rhythm samples" to FirmwareSet(firmwareDir, listOf("bd" to "Bass drum", "sd" to "Snare drum",
                "top" to "Top cymbal", "hh" to "Hi-hat", "tom" to "Tom", "rim" to "Rim shot").map { (name, label) ->
                FirmwareSet.Part("2608_$name.wav", label, "2608_$name.wav · a drum sound for sound board II's music",
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
        val second = game.mediaId("floppyB")?.let { id ->
            mediaFor(id, entry, libraryEntries) ?: error("The second disk this game needs is not in the library")
        }
        val diskA = disks.prepare(entry, cancelled, progress)
        val diskB = second?.let { disks.prepare(it, cancelled, progress) }
        check(Pc88Native.machine.start(firmwareDir.absolutePath, home.absolutePath,
            listOf(diskA.absolutePath, diskB?.absolutePath), settings)) {
            "Another machine is still running"
        }
        awaitRunning()
        mediaStarted(listOf(entry, second))
    }

    override fun stateError(code: Int) = when (code) {
        2 -> "the emulator could not read or write the state"
        else -> "code $code"
    }
}
