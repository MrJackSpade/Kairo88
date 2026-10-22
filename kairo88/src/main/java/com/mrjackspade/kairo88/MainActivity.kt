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
import com.mrjackspade.kairo.frontend.sameSource

import java.io.File
import java.util.concurrent.atomic.AtomicBoolean

/** Kairo88: the PC-8801 on QUASI88, in the shared Kairo frontend. */
class MainActivity : MachineActivity<DiskImageEntry>() {
    override val identity = Identity("KAIRO88", "Kairo88", "PC-88", "kairo88", LibraryStrings("KAIRO88", "Kairo88"),
        "Kairo88 uses QUASI88 and ymfm. Source and provenance: github.com/MrJackSpade/Kairo88.",
        "Choose a ROM folder to find disk images and the ZIP and 7z archives holding them",
        "Folder access expired. Select the ROM folder again.",
        MachineFirstRun.Folder("Choose a ROM folder", "Select the folder containing your PC-88 games. Kairo88 will " +
            "scan its disk images and the ZIP and 7z archives holding them.", "Choose ROM folder…"),
        "SAVE STATES")
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
    override fun defaultBindings(layout: ControllerLayout) = Pc88Keys.defaults(layout)
    override val artworkDownloadsEnabled: Boolean get() = resources.getBoolean(R.bool.catalog_art_download_enabled)

    private val disks by lazy {
        DiskImageLibrary(this, DiskImageLibrary.Format(mapOf("d88" to "fd", "88d" to "fd"),
            mapOf("fd" to "Floppy disk"), mapOf("fd" to 64L * 1024 * 1024), "PC-88"))
    }
    override fun createCatalog() = LayeredGameCatalog(this, Pc88Catalog)
    override fun createLibrary(catalog: LayeredGameCatalog) = disks

    override val machineOptions = Pc88Native.options
    override val mediaSlots = listOf(MediaSlot<DiskImageEntry>("Drive 1"), MediaSlot("Drive 2"))

    private val firmwareDir by lazy { File(filesDir, "firmware") }
    override val firmwareSets by lazy {
        fun rom(name: String, label: String, required: Boolean, sizes: Set<Int>, vararg aliases: String) =
            FirmwareSet.Part(name, label, setOf(name) + aliases, required, sizes.map(Int::toLong).toSet())
        listOf("PC-88 ROMs" to FirmwareSet(firmwareDir, listOf(
            rom("n88.rom", "N88-BASIC", true, setOf(0x8000)),
            rom("n88_0.rom", "N88 extension 0", true, setOf(0x2000), "n88ext0.rom"),
            rom("n88_1.rom", "N88 extension 1", false, setOf(0x2000), "n88ext1.rom"),
            rom("n88_2.rom", "N88 extension 2", false, setOf(0x2000), "n88ext2.rom"),
            rom("n88_3.rom", "N88 extension 3", false, setOf(0x2000), "n88ext3.rom"),
            rom("n80.rom", "N-BASIC", false, setOf(0x8000), "n88n.rom"),
            rom("disk.rom", "Disk sub-system", true, setOf(0x800, 0x2000), "n88sub.rom"),
            rom("kanji1.rom", "Kanji ROM 1", false, setOf(0x20000), "n88knj1.rom"),
            rom("kanji2.rom", "Kanji ROM 2", false, setOf(0x20000), "n88knj2.rom"),
            rom("jisyo.rom", "Dictionary ROM", false, setOf(0x80000), "n88jisho.rom"),
            rom("font.rom", "Font ROM", false, setOf(0x800, 0x1000)))),
            "Rhythm samples" to FirmwareSet(firmwareDir, listOf("bd" to "Bass drum", "sd" to "Snare drum",
                "top" to "Top cymbal", "hh" to "Hi-hat", "tom" to "Tom", "rim" to "Rim shot").map { (name, label) ->
                FirmwareSet.Part("2608_$name.wav", label, setOf("2608_$name.wav"), true)
            }))
    }
    override val firmwareText get() = Triple("PC-88 firmware",
        "Choose the folder holding your PC-88 ROM files. Kairo88 copies the files it recognizes.",
        "N88-BASIC, extension 0 and the disk sub-system are needed to start games. The YM2608 rhythm " +
            "samples (2608_*.wav) play the drums of sound board II.")

    override fun startGame(entry: DiskImageEntry, game: CatalogGame, cancelled: AtomicBoolean,
                           progress: (String) -> Unit) {
        firmwareSets.first().second.missingRequired.takeIf { it.isNotEmpty() }?.let { missing ->
            error("Import the PC-88 ROMs in Menu › Machine first (missing ${missing.joinToString(", ") { it.label }})")
        }
        val second = game.mediaId("floppyB")?.let { id ->
            val candidates = libraryEntries.filter { it.playable && (it.contentId == id || it.catalogId == id) }
            candidates.firstOrNull { sameSource(entry, it) } ?: candidates.firstOrNull()
                ?: error("The second disk this game needs is not in the library")
        }
        val diskA = disks.prepare(entry, cancelled, progress)
        val diskB = second?.let { disks.prepare(it, cancelled, progress) }
        val home = File(filesDir, "quasi88").apply { mkdirs() }
        check(Pc88Native.machine.start(firmwareDir.absolutePath, home.absolutePath,
            listOf(diskA.absolutePath, diskB?.absolutePath), Pc88Native.settings(optionValues(game)))) {
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
