package com.mrjackspade.kairo88

import com.mrjackspade.kairo.frontend.FirmwareSet
import com.mrjackspade.kairo.frontend.R as FrontendR
import com.mrjackspade.kairo.frontend.UiText
import java.io.File

/** The firmware the app's Machine page lists, as its launch builds it (Kairo#125). */
object Pc88Firmware {
    /** Each set, titled, with its files in [directory]. */
    fun sets(directory: File): List<Pair<UiText, FirmwareSet>> {
        fun description(name: String, what: UiText) = UiText.res(FrontendR.string.kairo_firmware_part, name, what)
        fun rom(name: String, label: UiText, description: UiText, required: Boolean, sizes: Set<Int>,
                vararg aliases: String) = FirmwareSet.Part(name, label, description(name, description), setOf(name) + aliases,
            required, sizes.map(Int::toLong).toSet())
        fun rom(name: String, label: Int, description: Int, required: Boolean, sizes: Set<Int>, vararg aliases: String) =
            rom(name, UiText.res(label), UiText.res(description), required, sizes, *aliases)
        return listOf(UiText.res(FrontendR.string.kairo_firmware_title, "PC-88") to FirmwareSet(directory, listOf(
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
            UiText.res(R.string.rhythm_samples) to FirmwareSet(directory, listOf("bd" to R.string.rhythm_bass_drum,
                "sd" to R.string.rhythm_snare_drum, "top" to R.string.rhythm_top_cymbal, "hh" to R.string.rhythm_hi_hat,
                "tom" to R.string.rhythm_tom, "rim" to R.string.rhythm_rim_shot).map { (name, label) ->
                FirmwareSet.Part("2608_$name.wav", UiText.res(label), description("2608_$name.wav", UiText.res(R.string.rhythm_description)),
                    setOf("2608_$name.wav"), true)
            }))
    }
}
