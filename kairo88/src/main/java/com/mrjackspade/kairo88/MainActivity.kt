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

    private val format = Pc88Media.format
    private val disks by lazy { DiskImageLibrary(this, format) }
    override fun createCatalog() = LayeredGameCatalog(this, Pc88Catalog)
    override fun createLibrary(catalog: LayeredGameCatalog) = disks

    override val machineOptions = Pc88Native.options
    // Unformatted, as a new floppy was: a game's own menu makes it a user disk, or N88-DISK BASIC formats it.
    override val blankDisks = listOf(
        coreBlankDisk("2d", UiText.res(FrontendR.string.kairo_blank_floppy, "320 KB · 2D"), "d88"))
    private fun isDisk(entry: DiskImageEntry) = entry.kind == "fd"
    private fun isTape(entry: DiskImageEntry) = entry.kind == "tape"

    override val mediaSlots = listOf(MediaSlot(UiText.res(R.string.drive_1), ::isDisk),
        MediaSlot(UiText.res(FrontendR.string.kairo_drive_2), ::isDisk), MediaSlot(UiText.res(FrontendR.string.kairo_cassette), ::isTape))

    private val firmwareDir by lazy { File(filesDir, "firmware") }
    override val firmwareSets by lazy { Pc88Firmware.sets(firmwareDir) }

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

    // N88-BASIC's prompt: with drive 1 empty the machine boots ROM BASIC (k88_boot), and the game's
    // disks go in once it runs; a tape waits in the deck until it is loaded.
    override val commandLine = true

    override fun startAtPrompt(entry: DiskImageEntry, game: CatalogGame, cancelled: AtomicBoolean,
                               progress: (String) -> Unit) {
        requireFirmware(listOf(firmwareSets.first().second))
        val home = File(filesDir, "quasi88").apply { mkdirs() }
        val tape = entry.takeIf(::isTape)?.let { disks.prepare(it, cancelled, progress).absolutePath }
        check(Pc88Native.machine.start(firmwareDir.absolutePath, home.absolutePath, listOf(null, null, tape),
            Pc88Native.settings(optionValues(game)))) { "Another machine is still running" }
        awaitRunning()
        if (isTape(entry)) mediaStarted(listOf(null, null, entry))
        else insertMedia(listOf(entry, companionFor(game.mediaId("floppyB"), entry, launchEntries,
            UiText.res(FrontendR.string.kairo_second_disk_missing), ::isDisk)), cancelled, progress)
    }
}
