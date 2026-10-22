package com.mrjackspade.kairo88

import com.mrjackspade.kairo.frontend.CatalogGame
import com.mrjackspade.kairo.frontend.ChoiceGroup
import com.mrjackspade.kairo.frontend.ChoiceOption
import com.mrjackspade.kairo.frontend.ChoiceSpec
import com.mrjackspade.kairo.frontend.ControllerLayout
import com.mrjackspade.kairo.frontend.ControllerGuestSpec
import com.mrjackspade.kairo.frontend.JoystickInputRouter
import com.mrjackspade.kairo.frontend.LibraryStrings
import com.mrjackspade.kairo.frontend.DiskImageEntry
import com.mrjackspade.kairo.frontend.DiskImageLibrary
import com.mrjackspade.kairo.frontend.FirmwareSet
import com.mrjackspade.kairo.frontend.GameSettingsRows
import com.mrjackspade.kairo.frontend.GameSource
import com.mrjackspade.kairo.frontend.GroupsPage
import com.mrjackspade.kairo.frontend.LayeredGameCatalog
import com.mrjackspade.kairo.frontend.MachineActivity
import com.mrjackspade.kairo.frontend.MachineFirstRun
import com.mrjackspade.kairo.frontend.MachineStatus
import com.mrjackspade.kairo.frontend.RowGroup
import com.mrjackspade.kairo.frontend.SettingsRow
import com.mrjackspade.kairo.frontend.ValueTone
import com.mrjackspade.kairo.frontend.sameSource

import android.content.Intent
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
    override val bridge = Pc88Native
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

    private val firmwareDir by lazy { File(filesDir, "firmware") }
    private val roms by lazy {
        fun rom(name: String, label: String, required: Boolean, sizes: Set<Int>, vararg aliases: String) =
            FirmwareSet.Part(name, label, setOf(name) + aliases, required, sizes.map(Int::toLong).toSet())
        FirmwareSet(firmwareDir, listOf(
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
            rom("font.rom", "Font ROM", false, setOf(0x800, 0x1000))))
    }
    private val rhythm by lazy {
        FirmwareSet(firmwareDir, listOf("bd" to "Bass drum", "sd" to "Snare drum", "top" to "Top cymbal",
            "hh" to "Hi-hat", "tom" to "Tom", "rim" to "Rim shot").map { (name, label) ->
            FirmwareSet.Part("2608_$name.wav", label, setOf("2608_$name.wav"), true)
        })
    }
    @Volatile private var importing = false

    override val firmware: MachineFirstRun.Firmware get() = MachineFirstRun.Firmware("PC-88 firmware",
        "Choose the folder holding your PC-88 ROM files. Kairo88 copies the files it recognizes.",
        { firmwareRows() },
        "N88-BASIC, extension 0 and the disk sub-system are needed to start games. The YM2608 rhythm " +
            "samples (2608_*.wav) play the drums of sound board II.", { importing })

    private fun firmwareRows() = listOf(
        roms.row("roms", "PC-88 ROMs", { importing }, ::firmwareConfigurable, { RETURN_FOR_FIRMWARE }, ::chooseFirmware),
        rhythm.row("rhythm", "Rhythm samples", { importing }, ::firmwareConfigurable, { RETURN_FOR_FIRMWARE },
            ::chooseFirmware))

    /** The machine reads its ROMs when a game boots, so they change only in the library. */
    private fun firmwareConfigurable() = currentEntry == null && !transitioning

    private fun chooseFirmware() = roms.chooseFolder(this, FIRMWARE_REQUEST, ::toast)

    override fun handleProductResult(requestCode: Int, resultCode: Int, data: Intent?): Boolean {
        if (requestCode != FIRMWARE_REQUEST) return false
        val tree = data?.data?.takeIf { resultCode == RESULT_OK } ?: return true
        importing = true
        firmwareChanged()
        Thread {
            val message = try {
                val cancelled = AtomicBoolean(false)
                val imported = roms.importFolder(contentResolver, tree, cancelled) +
                    rhythm.importFolder(contentResolver, tree, cancelled)
                if (imported.isEmpty()) "No PC-88 firmware files in that folder"
                else "Imported ${imported.size} firmware ${if (imported.size == 1) "file" else "files"}"
            } catch (error: Exception) {
                "Firmware import failed: ${error.message ?: error.javaClass.simpleName}"
            }
            runOnUiThread {
                importing = false
                firmwareChanged()
                toast(message)
            }
        }.apply { name = "Kairo88-firmware"; start() }
        return true
    }

    // The machine a game boots: its own settings, else the defaults for all games.
    private fun defaultBasic() = preferences.getString("basic", null)
        ?.let { id -> Pc88Native.Basic.entries.firstOrNull { it.id == id } } ?: Pc88Native.Basic.V2
    private fun defaultClock() = preferences.getInt("clock", 8).takeIf { it in Pc88Catalog.CLOCKS } ?: 8
    private fun defaultSound() = preferences.getString("sound", null)
        ?.let { id -> Pc88Native.Sound.entries.firstOrNull { it.id == id } } ?: Pc88Native.Sound.II
    private fun defaultPort() = preferences.getString("port", null)
        ?.let { id -> Pc88Native.Port.entries.firstOrNull { it.id == id } } ?: Pc88Native.Port.JOYSTICK

    private fun machineFor(game: CatalogGame) = Pc88Native.Machine(
        Pc88Catalog.basic(game) ?: defaultBasic(), (Pc88Catalog.clock(game) ?: defaultClock()) == 8,
        Pc88Catalog.sound(game) ?: defaultSound(), Pc88Catalog.port(game) ?: defaultPort())

    /** What each drive holds, by library entry; null for an empty drive or a disk the library lacks. */
    private val mounted = arrayOfNulls<DiskImageEntry>(2)
    @Volatile private var diskBusy = false

    override fun startGame(entry: DiskImageEntry, game: CatalogGame, cancelled: AtomicBoolean,
                           progress: (String) -> Unit) {
        roms.missingRequired.takeIf { it.isNotEmpty() }?.let { missing ->
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
        check(Pc88Native.start(firmwareDir.absolutePath, home.absolutePath, diskA.absolutePath, diskB?.absolutePath,
            machineFor(game))) { "Another machine is still running" }
        // The machine leaves Starting once it has booted or failed.
        while (true) {
            val status = MachineStatus(Pc88Native.status())
            if (status.state == "Error") error(status.error)
            if (status.running) break
            Thread.sleep(20)
        }
        runOnUiThread {
            mounted[0] = entry
            mounted[1] = second
        }
    }

    override fun sessionEnded() {
        mounted.fill(null)
    }

    override fun stateLoaded() {
        val paths = Pc88Native.diskPaths() ?: return
        for (drive in 0..1) mounted[drive] = disks.entryForWorkingImage(paths[drive], libraryEntries)
        refreshSettingValues()
    }

    override fun stateError(code: Int) = when (code) {
        2 -> "the emulator could not read or write the state"
        else -> "code $code"
    }

    override fun machineRows() = listOf(SettingsRow.nav("f-disks", "Disks") { pages.push(disksPage()) })

    private fun disksPage() = GroupsPage("Disks", { currentGame?.title }, {
        listOf(RowGroup("DRIVES", listOf(driveRow(0), driveRow(1)),
            "Disks from the same archive or folder as the game. A disk keeps what the game writes to it."))
    })

    private fun driveRow(drive: Int) = SettingsRow.choice("drive$drive", "Drive ${drive + 1}", driveChoice(drive),
        value = { mounted[drive]?.displayName ?: "Empty" },
        tone = { if (mounted[drive] == null) ValueTone.EMPTY else ValueTone.CURRENT },
        enabled = { machineRunning && !diskBusy },
        disabledReason = { if (diskBusy) "Wait for the disk change to finish" else "Start a game before changing disks" })

    /** Empty ejects; the others are the disks from the game's own archive or folder. */
    private fun driveChoice(drive: Int): ChoiceSpec {
        fun candidates(): List<DiskImageEntry> {
            val current = currentEntry ?: return emptyList()
            return libraryEntries.filter { it.playable && sameSource(current, it) }
        }
        return ChoiceSpec("Drive ${drive + 1}", { currentGame?.title }, {
            val found = candidates()
            listOfNotNull(ChoiceGroup(null, listOf(ChoiceOption("empty", "Empty"))),
                found.takeIf { it.isNotEmpty() }?.let { items ->
                    ChoiceGroup("IN THIS GAME’S FOLDER", items.map { ChoiceOption(it.id, it.displayName) })
                })
        }, { mounted[drive]?.id ?: "empty" }, { id ->
            if (id == "empty") changeDisk(drive, null)
            else candidates().firstOrNull { it.id == id }?.let { changeDisk(drive, it) }
        }, cycles = false)
    }

    private fun changeDisk(drive: Int, entry: DiskImageEntry?) {
        if (diskBusy) return
        diskBusy = true
        refreshSettingValues()
        Thread {
            val failure = try {
                val image = entry?.let { disks.prepare(it, AtomicBoolean(false)) { } }
                check(Pc88Native.disk(drive, image?.absolutePath)) { "The disk could not be inserted" }
                null
            } catch (error: Exception) { error.message ?: "Disk change failed" }
            runOnUiThread {
                diskBusy = false
                if (failure == null) mounted[drive] = entry
                toast(failure ?: "Drive ${drive + 1}: ${entry?.displayName ?: "Empty"}")
                refreshSettingValues()
            }
        }.apply { name = "Kairo88-disk"; start() }
    }

    override fun machineGroup(entry: DiskImageEntry, enabled: () -> Boolean): RowGroup {
        val scope = { titleOf(entry) }
        val noHash = { GameSettingsRows.NO_HASH }
        return RowGroup("MACHINE", listOf(
            GameSettingsRows.choice("basic", "BASIC mode", scope,
                Pc88Native.Basic.entries.map { ChoiceOption(it.id, it.label) },
                machineValue(entry, "machine", "basic", { Pc88Catalog.basic(it)?.id }, { defaultBasic().id },
                    GameSource.ALL_GAMES), enabled, noHash),
            GameSettingsRows.choice("clock", "CPU clock", scope, CLOCK_OPTIONS,
                machineValue(entry, "machine", "clock", { Pc88Catalog.clock(it)?.toString() },
                    { defaultClock().toString() }, GameSource.ALL_GAMES) { it.toInt() }, enabled, noHash),
            GameSettingsRows.choice("sound", "Sound", scope,
                Pc88Native.Sound.entries.map { ChoiceOption(it.id, it.label) },
                machineValue(entry, "machine", "sound", { Pc88Catalog.sound(it)?.id }, { defaultSound().id },
                    GameSource.ALL_GAMES), enabled, noHash),
            GameSettingsRows.choice("port", "Port", scope,
                Pc88Native.Port.entries.map { ChoiceOption(it.id, it.label) },
                machineValue(entry, "machine", "port", { Pc88Catalog.port(it)?.id }, { defaultPort().id },
                    GameSource.ALL_GAMES), enabled, noHash)),
            "Machine changes apply on the next launch or restart.")
    }

    override fun machinePage() = GroupsPage("Machine", { GameSource.ALL_GAMES }, {
        listOf(
            RowGroup("ALL GAMES", listOf(
                defaultRow("basic", "BASIC mode", Pc88Native.Basic.entries.map { ChoiceOption(it.id, it.label) },
                    { defaultBasic().id }) { preferences.edit().putString("basic", it).apply() },
                defaultRow("clock", "CPU clock", CLOCK_OPTIONS, { defaultClock().toString() }) {
                    preferences.edit().putInt("clock", it.toInt()).apply()
                },
                defaultRow("sound", "Sound", Pc88Native.Sound.entries.map { ChoiceOption(it.id, it.label) },
                    { defaultSound().id }) { preferences.edit().putString("sound", it).apply() },
                defaultRow("port", "Port", Pc88Native.Port.entries.map { ChoiceOption(it.id, it.label) },
                    { defaultPort().id }) { preferences.edit().putString("port", it).apply() }),
                "A game's own settings come first. Changes apply on the next launch or restart."),
            RowGroup("FIRMWARE", firmwareRows(),
                if (firmwareConfigurable()) firmware.footnote else RETURN_FOR_FIRMWARE))
    })

    private fun defaultRow(id: String, label: String, options: List<ChoiceOption>, current: () -> String,
                           save: (String) -> Unit) = SettingsRow.choice("default-$id", label,
        ChoiceSpec(label, { GameSource.ALL_GAMES }, { listOf(ChoiceGroup(null, options)) }, current,
            select = { value -> save(value); refreshSettingValues() }))

    companion object {
        private const val FIRMWARE_REQUEST = FIRST_PRODUCT_REQUEST
        private const val RETURN_FOR_FIRMWARE = "Return to the library to change firmware."
        private val CLOCK_OPTIONS = listOf(ChoiceOption("8", "8 MHz"), ChoiceOption("4", "4 MHz"))
    }
}
