package com.mrjackspade.kairo88

import com.mrjackspade.kairo.KanaLegendsTest
import com.mrjackspade.kairo.LaunchTablesTest
import java.io.File

/** What Kairo88 builds as it launches (Kairo#125, Kairo88#45). */
class Pc88LaunchTablesTest : LaunchTablesTest() {
    override fun tables() = LaunchTables(listOf(Pc88Media.format), listOf(Pc88Keys.layout),
        Pc88Firmware.sets(File("firmware")).map { it.second },
        mapOf("controller bindings" to { Pc88Keys.bindings }, "machine options" to { Pc88Native.options }))
}

/** The PC-88 keyboard's kana legends (Kairo#126). */
class Pc88KanaLegendsTest : KanaLegendsTest() {
    override val layouts = listOf(Pc88Keys.layout)
}
