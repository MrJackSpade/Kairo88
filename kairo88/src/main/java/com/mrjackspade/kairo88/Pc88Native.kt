package com.mrjackspade.kairo88

import com.mrjackspade.kairo.frontend.MachineBridge
import android.view.Surface

/** The QUASI88 machine in libkairo88 (src/main/cpp/native_bridge.cpp). */
object Pc88Native : MachineBridge {
    init { System.loadLibrary("kairo88") }

    /** What a game boots: BASIC mode, CPU clock, sound board, and the joystick port's device. */
    data class Machine(val basic: Basic, val clock8Mhz: Boolean, val sound: Sound, val port: Port)

    enum class Basic(val core: Int, val id: String, val label: String) {
        N(0, "n", "N-BASIC"), V1S(1, "v1s", "N88 V1S"), V1H(2, "v1h", "N88 V1H"), V2(3, "v2", "N88 V2")
    }
    enum class Sound(val core: Int, val id: String, val label: String) {
        I(0, "1", "Sound board"), II(1, "2", "Sound board II")
    }
    enum class Port(val core: Int, val id: String, val label: String) {
        JOYSTICK(0, "joystick", "Joystick"), MOUSE(1, "mouse", "Mouse")
    }

    /** Starts the machine with its disks; false while one is already running. */
    fun start(firmware: String, home: String, diskA: String?, diskB: String?, machine: Machine): Boolean =
        nativeStart(firmware, home, diskA, diskB, machine.basic.core, machine.clock8Mhz, machine.sound.core,
            machine.port.core)

    /** Inserts an image in drive 0 or 1, or ejects it for null; false when it could not be opened. */
    fun disk(drive: Int, path: String?): Boolean = nativeDisk(drive, path) == 0

    /** The images in both drives, "" for an empty one; null when no machine runs. */
    fun diskPaths(): List<String>? = nativeDiskPaths()?.toList()

    override fun stop() = nativeStop()
    override fun pause(paused: Boolean) = nativePause(paused)
    override fun reset() = nativeReset()
    override fun key(code: Int, down: Boolean) = nativeKey(code, down)
    override fun joystick(control: Int, down: Boolean) = nativeJoystick(control, down)
    override fun mouseMove(dx: Int, dy: Int) = nativeMouseMove(dx, dy)
    override fun mouseButton(button: Int, down: Boolean) = nativeMouseButton(button, down)
    override fun saveState(directory: String) = nativeSaveState(directory)
    override fun loadState(directory: String) = nativeLoadState(directory)
    override fun lockedKeys(): Set<Int> {
        val locks = nativeLocks()
        check(locks >= 0) { "The machine stopped before reporting its lock keys" }
        return setOfNotNull(Pc88Keys.CAPS.takeIf { locks and 1 != 0 }, Pc88Keys.KANA.takeIf { locks and 2 != 0 })
    }
    override fun status(): String = nativeStatus()
    override fun setSurface(surface: Surface?) = nativeSetSurface(surface)
    override fun setShaderPreset(path: String?) = nativeSetShaderPreset(path)
    override fun setMuted(muted: Boolean) = nativeSetMuted(muted)
    override fun setFastForward(enabled: Boolean) = nativeSetFastForward(enabled)

    private external fun nativeStart(firmware: String, home: String, diskA: String?, diskB: String?, basic: Int,
                                     clock8Mhz: Boolean, sound: Int, port: Int): Boolean
    private external fun nativeStop()
    private external fun nativePause(paused: Boolean)
    private external fun nativeReset()
    private external fun nativeKey(code: Int, down: Boolean)
    private external fun nativeJoystick(control: Int, down: Boolean)
    private external fun nativeMouseMove(dx: Int, dy: Int)
    private external fun nativeMouseButton(button: Int, down: Boolean)
    private external fun nativeDisk(drive: Int, path: String?): Int
    private external fun nativeDiskPaths(): Array<String>?
    private external fun nativeSaveState(directory: String): Int
    private external fun nativeLoadState(directory: String): Int
    private external fun nativeLocks(): Int
    private external fun nativeStatus(): String
    private external fun nativeSetSurface(surface: Surface?)
    private external fun nativeSetShaderPreset(path: String?)
    private external fun nativeSetMuted(muted: Boolean)
    private external fun nativeSetFastForward(enabled: Boolean)
}
