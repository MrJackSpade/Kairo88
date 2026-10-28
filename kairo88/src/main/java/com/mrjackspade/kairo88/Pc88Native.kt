package com.mrjackspade.kairo88

import com.mrjackspade.kairo.frontend.ChoiceOption
import com.mrjackspade.kairo.frontend.MachineOption
import com.mrjackspade.kairo.frontend.NativeMachine
import com.mrjackspade.kairo.frontend.UiText
import com.mrjackspade.kairo.frontend.R as FrontendR

/** The QUASI88 machine in libkairo88 (src/main/cpp/pc88_core.cpp) and the choices a game boots with. */
object Pc88Native {
    val machine by lazy { NativeMachine("kairo88", listOf(Pc88Keys.CAPS, Pc88Keys.KANA)) }

    /** The BASIC modes, by their names on the machine. */
    enum class Basic(val core: Int, val id: String, val label: String) {
        N(0, "n", "N-BASIC"), V1S(1, "v1s", "N88 V1S"), V1H(2, "v1h", "N88 V1H"), V2(3, "v2", "N88 V2")
    }
    enum class Sound(val core: Int, val id: String, val label: UiText) {
        I(0, "1", UiText.res(R.string.sound_board)), II(1, "2", UiText.res(R.string.sound_board_ii))
    }
    enum class Port(val core: Int, val id: String, val label: UiText) {
        JOYSTICK(0, "joystick", UiText.res(FrontendR.string.kairo_joystick)),
        MOUSE(1, "mouse", UiText.res(FrontendR.string.kairo_mouse))
    }
    /** How closely the disk unit's CPU runs with the main CPU (kairo88_core.h, k88_machine.sub_cpu). */
    enum class SubCpu(val core: Int, val id: String, val label: UiText) {
        OFF(0, "off", UiText.res(FrontendR.string.kairo_off)), DISK(1, "disk", UiText.res(R.string.disk_cpu_sync_disk)),
        ALWAYS(2, "always", UiText.res(R.string.disk_cpu_sync_always))
    }

    val basic = MachineOption("basic", UiText.res(R.string.option_basic), Basic.entries.map { ChoiceOption(it.id, it.label) },
        Basic.V2.id)
    val clock = MachineOption("clock", UiText.res(R.string.option_clock),
        listOf("8", "4").map { ChoiceOption(it, UiText.res(R.string.clock_mhz, it)) }, "8", numeric = true)
    val sound = MachineOption("sound", UiText.res(R.string.option_sound), Sound.entries.map { ChoiceOption(it.id, it.label) },
        Sound.II.id)
    val port = MachineOption("port", UiText.res(R.string.option_port), Port.entries.map { ChoiceOption(it.id, it.label) },
        Port.JOYSTICK.id)
    // Copy-protected loaders such as Thexder's need the disk unit's CPU in step during disk access.
    val subCpu = MachineOption("subcpu", UiText.res(R.string.option_disk_cpu_sync),
        SubCpu.entries.map { ChoiceOption(it.id, it.label) }, SubCpu.OFF.id)
    val options = listOf(basic, clock, sound, port, subCpu)

    /** The settings pc88_core.cpp boots with, from the options' values. */
    fun settings(values: Map<String, String>) = mapOf(
        "basic" to Basic.entries.first { it.id == values.getValue("basic") }.core.toString(),
        "clock" to values.getValue("clock"),
        "sound" to Sound.entries.first { it.id == values.getValue("sound") }.core.toString(),
        "port" to Port.entries.first { it.id == values.getValue("port") }.core.toString(),
        "subcpu" to SubCpu.entries.first { it.id == values.getValue("subcpu") }.core.toString())
}
