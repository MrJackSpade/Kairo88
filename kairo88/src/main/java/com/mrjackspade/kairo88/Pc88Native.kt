package com.mrjackspade.kairo88

import com.mrjackspade.kairo.frontend.ChoiceOption
import com.mrjackspade.kairo.frontend.MachineOption
import com.mrjackspade.kairo.frontend.NativeMachine

/** The QUASI88 machine in libkairo88 (src/main/cpp/pc88_core.cpp) and the choices a game boots with. */
object Pc88Native {
    val machine by lazy { NativeMachine("kairo88", listOf(Pc88Keys.CAPS, Pc88Keys.KANA)) }

    enum class Basic(val core: Int, val id: String, val label: String) {
        N(0, "n", "N-BASIC"), V1S(1, "v1s", "N88 V1S"), V1H(2, "v1h", "N88 V1H"), V2(3, "v2", "N88 V2")
    }
    enum class Sound(val core: Int, val id: String, val label: String) {
        I(0, "1", "Sound board"), II(1, "2", "Sound board II")
    }
    enum class Port(val core: Int, val id: String, val label: String) {
        JOYSTICK(0, "joystick", "Joystick"), MOUSE(1, "mouse", "Mouse")
    }

    val basic = MachineOption("basic", "BASIC mode", Basic.entries.map { ChoiceOption(it.id, it.label) }, Basic.V2.id)
    val clock = MachineOption("clock", "CPU clock", listOf(ChoiceOption("8", "8 MHz"), ChoiceOption("4", "4 MHz")), "8",
        numeric = true)
    val sound = MachineOption("sound", "Sound", Sound.entries.map { ChoiceOption(it.id, it.label) }, Sound.II.id)
    val port = MachineOption("port", "Port", Port.entries.map { ChoiceOption(it.id, it.label) }, Port.JOYSTICK.id)
    val options = listOf(basic, clock, sound, port)

    /** The settings pc88_core.cpp boots with, from the options' values. */
    fun settings(values: Map<String, String>) = mapOf(
        "basic" to Basic.entries.first { it.id == values.getValue("basic") }.core.toString(),
        "clock" to values.getValue("clock"),
        "sound" to Sound.entries.first { it.id == values.getValue("sound") }.core.toString(),
        "port" to Port.entries.first { it.id == values.getValue("port") }.core.toString())
}
