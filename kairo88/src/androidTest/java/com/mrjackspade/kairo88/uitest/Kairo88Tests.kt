package com.mrjackspade.kairo88.uitest

import com.mrjackspade.kairo.uitest.AppUnderTest
import com.mrjackspade.kairo.uitest.AppUnderTest.BiosFile
import com.mrjackspade.kairo.uitest.AppUnderTest.MediaSwap
import com.mrjackspade.kairo.uitest.KairoUiTest
import com.mrjackspade.kairo.uitest.suites.FirstRunTests
import com.mrjackspade.kairo.uitest.suites.GameTests
import com.mrjackspade.kairo.uitest.suites.LibraryTests
import com.mrjackspade.kairo.uitest.suites.SearchTests
import com.mrjackspade.kairo.uitest.suites.WordingTests
import com.mrjackspade.kairo.uitest.suites.MediaTests
import com.mrjackspade.kairo.uitest.suites.MenuTests
import com.mrjackspade.kairo.uitest.suites.ControllerTests
import com.mrjackspade.kairo.uitest.suites.TouchTests
import com.mrjackspade.kairo.uitest.suites.LaunchTests
import com.mrjackspade.kairo.uitest.suites.LibraryManagementTests
import com.mrjackspade.kairo.uitest.suites.SettingsTests
import com.mrjackspade.kairo.uitest.suites.StateTests
import org.junit.Test

/**
 * Kairo88 as the shared UI tests see it. Its test resources (uitest/resources.json) are Neo Kobe
 * archives: Thunder Force's whole folder (V1 and SR versions, dump sets, a tape and extras),
 * Angelus (two disks and alternate dumps), the first two 100 Yen Disks and a utility, with the
 * PC-88 BIOS and rhythm samples.
 */
object Kairo88App : AppUnderTest() {
    override val name = "Kairo88"
    override val brand = "KAIRO88"
    override val biosTitle = "PC-88 BIOS"
    override val biosFiles = listOf(
        BiosFile("n88.rom", true), BiosFile("n88_0.rom", true), BiosFile("n88_1.rom", false),
        BiosFile("n88_2.rom", false), BiosFile("n88_3.rom", false), BiosFile("n80.rom", false),
        BiosFile("disk.rom", true), BiosFile("kanji1.rom", false), BiosFile("kanji2.rom", false),
        BiosFile("jisyo.rom", false), BiosFile("font.rom", false),
        BiosFile("2608_bd.wav", true), BiosFile("2608_sd.wav", true), BiosFile("2608_top.wav", true),
        BiosFile("2608_hh.wav", true), BiosFile("2608_tom.wav", true), BiosFile("2608_rim.wav", true))
    override val bootGame = "Thunder Force"
    override val singleEntryGames = listOf("Thunder Force", "Angelus: Akuma no Fukuin")
    override val separateGames = listOf("100 Yen Disk No. 1", "100 Yen Disk No. 2")
    override val hiddenFiles = listOf("[Utility] Magic Copy II", "Thunder Force [CT]", "Thunder Force [extras]")
    override val searchTerm = "Thunder"
    override val bootGameFile = "games/Thunder Force (Tecno Soft)/Thunder Force [FD].7z"
    override val otherGame = "100 Yen Disk No. 1"
    override val defaultOption = "sound"
    override val mediaSwap = MediaSwap("Angelus: Akuma no Fukuin", "Drive 2", "(Disk C)")
}

class Kairo88FirstRunTests : FirstRunTests() { override val app = Kairo88App }
class Kairo88LibraryTests : LibraryTests() { override val app = Kairo88App }
class Kairo88SearchTests : SearchTests() { override val app = Kairo88App }
class Kairo88GameTests : GameTests() { override val app = Kairo88App }
class Kairo88WordingTests : WordingTests() { override val app = Kairo88App }
class Kairo88MediaTests : MediaTests() { override val app = Kairo88App }
class Kairo88MenuTests : MenuTests() { override val app = Kairo88App }
class Kairo88ControllerTests : ControllerTests() { override val app = Kairo88App }
class Kairo88TouchTests : TouchTests() { override val app = Kairo88App }
class Kairo88LaunchTests : LaunchTests() { override val app = Kairo88App }
class Kairo88LibraryManagementTests : LibraryManagementTests() { override val app = Kairo88App }
class Kairo88SettingsTests : SettingsTests() { override val app = Kairo88App }
class Kairo88StateTests : StateTests() { override val app = Kairo88App }

/** The PC-88's own behavior: the machine mode a disk asks for, and a second disk at launch. */
class Pc88Tests : KairoUiTest() {
    override val app = Kairo88App

    @Test
    fun aV1ModeGameStartsInV1Mode() {
        screens.completeFirstRun().open("Thunder Force").play()
        val probe = driver.probe()
        expect.equal("Thunder Force's BASIC mode setting", "v1s", probe.details["option.basic"])
        // Pc88Native.Basic.V1S is core mode 1.
        expect.equal("the BASIC mode the machine started in", "1", probe.details["machine.basic"])
    }

    @Test
    fun aTwoDiskGameStartsWithDiskBInDrive2() {
        screens.completeFirstRun().open("Angelus: Akuma no Fukuin").play()
        val probe = driver.probe()
        expect.that("drive 1 holds Disk A", probe.details["media.Drive 1"].orEmpty().contains("(Disk A)")) { probe.details["media.Drive 1"].orEmpty() }
        expect.that("drive 2 holds Disk B", probe.details["media.Drive 2"].orEmpty().contains("(Disk B)")) { probe.details["media.Drive 2"].orEmpty() }
    }

    @Test
    fun aDisksOwnModeOutranksTheDefault() {
        screens.completeFirstRun().waitForGame("Thunder Force")
        screens.choose(screens.openMachinePage(), "default-basic", "v2")
        driver.press(android.view.KeyEvent.KEYCODE_BUTTON_MODE)
        screens.library.open("Thunder Force").play()
        expect.equal("Thunder Force's mode with V2 as the default", "v1s", driver.probe().details["option.basic"])
    }

    @Test
    fun aGameWithoutAModeStartsInTheDefault() {
        screens.completeFirstRun().waitForGame("100 Yen Disk No. 1")
        val picked = screens.choose(screens.openMachinePage(), "default-basic", "v1h")
        driver.press(android.view.KeyEvent.KEYCODE_BUTTON_MODE)
        screens.library.open("100 Yen Disk No. 1").play()
        expect.equal("a game without its own mode starts in the default", picked, driver.probe().details["option.basic"])
    }

    @Test
    fun theMainReleaseStartsNotItsSrVersion() {
        screens.completeFirstRun().open("Thunder Force").play()
        val disk = driver.probe().details["media.Drive 1"].orEmpty()
        expect.that("Thunder Force starts from its V1 disk, not the SR version", disk.contains("{V1 mode}") && !disk.contains("(SR)")) { disk }
    }
}
