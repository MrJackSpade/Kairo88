package com.mrjackspade.kairo88

import com.mrjackspade.kairo.frontend.ControllerBinding
import com.mrjackspade.kairo.frontend.ControllerBindingsCodec
import com.mrjackspade.kairo.frontend.ControllerLayout
import com.mrjackspade.kairo.frontend.GuestKeyboardLayout
import com.mrjackspade.kairo.frontend.GuestKeyboardPage
import com.mrjackspade.kairo.frontend.JisKana
import com.mrjackspade.kairo.frontend.JoystickInputRouter
import com.mrjackspade.kairo.frontend.KeyboardKey
import android.view.KeyEvent

/**
 * The PC-8801 keyboard as QUASI88 key codes (pc88/keyboard.h): a character key's code is its
 * unshifted character, and each code is one position in the keyboard matrix, so a shifted symbol
 * is its key with Shift.
 */
object Pc88Keys {
    const val SPACE = 32
    const val YEN = 92
    const val UNDERSCORE = 95
    const val KP_0 = 128
    const val KP_MULTIPLY = 138
    const val KP_ADD = 139
    const val KP_EQUAL = 140
    const val KP_COMMA = 141
    const val KP_PERIOD = 142
    const val KP_SUB = 143
    const val KP_DIVIDE = 144
    const val RETURN = 145
    const val HOME = 146
    const val UP = 147
    const val RIGHT = 148
    const val INS_DEL = 149
    const val GRAPH = 150
    const val KANA = 151
    const val SHIFT = 152
    const val CTRL = 153
    const val STOP = 154
    const val ESC = 156
    const val TAB = 157
    const val DOWN = 158
    const val LEFT = 159
    const val HELP = 160
    const val COPY = 161
    const val CAPS = 162
    const val ROLL_UP = 163
    const val ROLL_DOWN = 164
    const val F1 = 165
    const val F6 = 180
    const val BS = 185
    const val INS = 186
    const val DEL = 187
    const val HENKAN = 188
    const val KETTEI = 189
    const val MAX_CODE = 255

    /** A character key's code: its unshifted character, with letters upper case. */
    private fun code(char: Char): Int = when (char) {
        '¥' -> YEN
        in 'a'..'z' -> char.uppercaseChar().code
        else -> char.code
    }

    private fun key(char: Char, shifted: String? = null) = JisKana.key(char, code(char), shifted = shifted)

    val layout = GuestKeyboardLayout(
        brand = "KAIRO88",
        pages = listOf(
            GuestKeyboardPage("ABC", alphabetRows()),
            GuestKeyboardPage("?123", symbolRows(), pickerLabel = "Symbols"),
            GuestKeyboardPage("PC-88", pc88Rows())
        ),
        modifiers = setOf(SHIFT, CTRL, GRAPH, CAPS, KANA),
        shiftCodes = setOf(SHIFT),
        capsCode = CAPS,
        chordShiftCode = SHIFT,
        kanaCode = KANA
    )

    // The alphabet page follows the PC-8801 rows, so every kana legend has its key.
    private fun alphabetRows(): List<List<KeyboardKey>> = listOf(
        listOf(KeyboardKey("Esc", ESC, 1.3f)) +
            listOf("!", "\"", "#", "$", "%", "&", "'", "(", ")", null).mapIndexed { index, shifted ->
                key("1234567890"[index], shifted)
            } +
            listOf(key('-', "="), key('^', "`"), key('¥', "|"), KeyboardKey("Del", INS_DEL, 1.5f, "Ins")),
        listOf(KeyboardKey("Tab", TAB, 1.3f)) +
            "qwertyuiop".map { key(it) } +
            listOf(key('@', "~"), key('[', "{"), KeyboardKey("Return", RETURN, 1.5f)),
        listOf(KeyboardKey("Caps", CAPS, 1.4f)) +
            "asdfghjkl".map { key(it) } +
            listOf(key(';', "+"), key(':', "*"), key(']', "}")),
        listOf(KeyboardKey("Shift", SHIFT, 1.5f)) +
            "zxcvbnm".map { key(it) } +
            listOf(key(',', "<"), key('.', ">"), key('/', "?"), key('_'),
                KeyboardKey("Shift", SHIFT, 1.5f)),
        listOf(KeyboardKey("Ctrl", CTRL), KeyboardKey("Graph", GRAPH), KeyboardKey("Kana", KANA),
            KeyboardKey("Space", SPACE, 5f), KeyboardKey("Stop", STOP), KeyboardKey("Help", HELP))
    )

    private fun symbolRows(): List<List<KeyboardKey>> = listOf(
        listOf(KeyboardKey("Esc", ESC, 1.3f)) + "1234567890".map { KeyboardKey("$it", it.code) } +
            KeyboardKey("Del", INS_DEL, 1.5f),
        listOf("!", "\"", "#", "$", "%", "&", "'", "(", ")")
            .mapIndexed { index, label -> KeyboardKey(label, '1'.code + index, chordShift = true) } +
            KeyboardKey("Return", RETURN, 1.5f),
        "-^¥@[];:,./".map { char -> KeyboardKey("$char", code(char)) },
        listOf("=" to '-', "`" to '^', "|" to '¥', "~" to '@', "{" to '[', "}" to ']', "+" to ';',
            "*" to ':', "<" to ',', ">" to '.', "?" to '/')
            .map { (label, char) -> KeyboardKey(label, code(char), chordShift = true) } +
            KeyboardKey("_", UNDERSCORE),
        listOf(KeyboardKey("Tab", TAB), KeyboardKey("Space", SPACE, 5f), KeyboardKey("Return", RETURN, 1.5f))
    )

    private fun pc88Rows(): List<List<KeyboardKey>> = listOf(
        listOf(KeyboardKey("Stop", STOP, 1.4f), KeyboardKey("Copy", COPY, 1.4f)) +
            (0..4).map { KeyboardKey("F${it + 1}", F1 + it, shifted = "F${it + 6}") },
        listOf(KeyboardKey("Roll↑", ROLL_UP), KeyboardKey("Roll↓", ROLL_DOWN), KeyboardKey("Home", HOME, shifted = "Clr"),
            KeyboardKey("Help", HELP), KeyboardKey("Ins", INS), KeyboardKey("Del", DEL)),
        listOf(KeyboardKey("7", KP_0 + 7), KeyboardKey("8", KP_0 + 8), KeyboardKey("9", KP_0 + 9),
            KeyboardKey("*", KP_MULTIPLY), KeyboardKey("/", KP_DIVIDE), KeyboardKey("-", KP_SUB)),
        listOf(KeyboardKey("4", KP_0 + 4), KeyboardKey("5", KP_0 + 5), KeyboardKey("6", KP_0 + 6),
            KeyboardKey("+", KP_ADD), KeyboardKey("=", KP_EQUAL), KeyboardKey("↑", UP)),
        listOf(KeyboardKey("1", KP_0 + 1), KeyboardKey("2", KP_0 + 2), KeyboardKey("3", KP_0 + 3),
            KeyboardKey("0", KP_0), KeyboardKey(",", KP_COMMA), KeyboardKey(".", KP_PERIOD),
            KeyboardKey("←", LEFT), KeyboardKey("↓", DOWN), KeyboardKey("→", RIGHT))
    )

    /** Every key a controller binding can send. */
    val bindableCodes: List<Int> = ((SPACE..95) + (KP_0..KP_DIVIDE) + (RETURN..ROLL_DOWN) + (F1..F1 + 4) +
        (F6..KETTEI)).filter { it != 155 }

    fun label(code: Int): String = when (code) {
        SPACE -> "Space"
        YEN -> "¥"
        in 33..126 -> code.toChar().toString()
        in KP_0..KP_0 + 9 -> "Numpad ${code - KP_0}"
        KP_MULTIPLY -> "Numpad *"
        KP_ADD -> "Numpad +"
        KP_EQUAL -> "Numpad ="
        KP_COMMA -> "Numpad ,"
        KP_PERIOD -> "Numpad ."
        KP_SUB -> "Numpad -"
        KP_DIVIDE -> "Numpad /"
        RETURN -> "Return"
        HOME -> "Home"
        UP -> "Up"
        RIGHT -> "Right"
        INS_DEL -> "Del"
        GRAPH -> "Graph"
        KANA -> "Kana"
        SHIFT -> "Shift"
        CTRL -> "Ctrl"
        STOP -> "Stop"
        ESC -> "Esc"
        TAB -> "Tab"
        DOWN -> "Down"
        LEFT -> "Left"
        HELP -> "Help"
        COPY -> "Copy"
        CAPS -> "Caps"
        ROLL_UP -> "Roll up"
        ROLL_DOWN -> "Roll down"
        in F1..F1 + 4 -> "F${code - F1 + 1}"
        in F6..F6 + 4 -> "F${code - F6 + 6}"
        BS -> "BS"
        INS -> "Ins"
        DEL -> "Delete"
        HENKAN -> "Henkan"
        KETTEI -> "Kettei"
        else -> "Key $code"
    }

    /** The PC-8801 key an Android key types: US punctuation by position on the JIS rows. */
    fun fromAndroid(key: Int): Int? {
        if (key in KeyEvent.KEYCODE_A..KeyEvent.KEYCODE_Z) return 'A'.code + key - KeyEvent.KEYCODE_A
        if (key in KeyEvent.KEYCODE_0..KeyEvent.KEYCODE_9) return '0'.code + key - KeyEvent.KEYCODE_0
        if (key in KeyEvent.KEYCODE_F1..KeyEvent.KEYCODE_F5) return F1 + key - KeyEvent.KEYCODE_F1
        if (key in KeyEvent.KEYCODE_F6..KeyEvent.KEYCODE_F10) return F6 + key - KeyEvent.KEYCODE_F6
        if (key in KeyEvent.KEYCODE_NUMPAD_0..KeyEvent.KEYCODE_NUMPAD_9) return KP_0 + key - KeyEvent.KEYCODE_NUMPAD_0
        return when (key) {
            KeyEvent.KEYCODE_ESCAPE -> ESC
            KeyEvent.KEYCODE_DEL -> INS_DEL
            KeyEvent.KEYCODE_FORWARD_DEL -> DEL
            KeyEvent.KEYCODE_INSERT -> INS
            KeyEvent.KEYCODE_TAB -> TAB
            KeyEvent.KEYCODE_ENTER, KeyEvent.KEYCODE_NUMPAD_ENTER -> RETURN
            KeyEvent.KEYCODE_SPACE -> SPACE
            KeyEvent.KEYCODE_DPAD_UP -> UP
            KeyEvent.KEYCODE_DPAD_DOWN -> DOWN
            KeyEvent.KEYCODE_DPAD_LEFT -> LEFT
            KeyEvent.KEYCODE_DPAD_RIGHT -> RIGHT
            KeyEvent.KEYCODE_MOVE_HOME -> HOME
            KeyEvent.KEYCODE_MOVE_END -> HELP
            KeyEvent.KEYCODE_PAGE_UP -> ROLL_UP
            KeyEvent.KEYCODE_PAGE_DOWN -> ROLL_DOWN
            KeyEvent.KEYCODE_BREAK -> STOP
            KeyEvent.KEYCODE_SHIFT_LEFT, KeyEvent.KEYCODE_SHIFT_RIGHT -> SHIFT
            KeyEvent.KEYCODE_CTRL_LEFT, KeyEvent.KEYCODE_CTRL_RIGHT -> CTRL
            KeyEvent.KEYCODE_ALT_LEFT, KeyEvent.KEYCODE_ALT_RIGHT -> GRAPH
            KeyEvent.KEYCODE_CAPS_LOCK -> CAPS
            KeyEvent.KEYCODE_KATAKANA_HIRAGANA -> KANA
            KeyEvent.KEYCODE_HENKAN -> HENKAN
            KeyEvent.KEYCODE_MINUS -> '-'.code
            KeyEvent.KEYCODE_EQUALS -> '^'.code
            KeyEvent.KEYCODE_BACKSLASH, KeyEvent.KEYCODE_YEN -> YEN
            KeyEvent.KEYCODE_LEFT_BRACKET -> '@'.code
            KeyEvent.KEYCODE_RIGHT_BRACKET -> '['.code
            KeyEvent.KEYCODE_SEMICOLON -> ';'.code
            KeyEvent.KEYCODE_APOSTROPHE -> ':'.code
            KeyEvent.KEYCODE_COMMA -> ','.code
            KeyEvent.KEYCODE_PERIOD -> '.'.code
            KeyEvent.KEYCODE_SLASH -> '/'.code
            KeyEvent.KEYCODE_RO -> UNDERSCORE
            KeyEvent.KEYCODE_NUMPAD_MULTIPLY -> KP_MULTIPLY
            KeyEvent.KEYCODE_NUMPAD_ADD -> KP_ADD
            KeyEvent.KEYCODE_NUMPAD_EQUALS -> KP_EQUAL
            KeyEvent.KEYCODE_NUMPAD_COMMA -> KP_COMMA
            KeyEvent.KEYCODE_NUMPAD_DOT -> KP_PERIOD
            KeyEvent.KEYCODE_NUMPAD_SUBTRACT -> KP_SUB
            KeyEvent.KEYCODE_NUMPAD_DIVIDE -> KP_DIVIDE
            else -> null
        }
    }

    /** One label per JoystickInputRouter.DEFAULT_CONTROLS control: the joystick port's lines. */
    val joystickLabels = listOf("Up", "Down", "Left", "Right", "Trigger 1", "Trigger 2")

    val bindings = ControllerBindingsCodec(::defaults, { it in bindableCodes }, JoystickInputRouter.DEFAULT_CONTROLS,
        setOf("menu", "pause", "restart", "exit", "fastForward"))

    fun defaults(layout: ControllerLayout): List<ControllerBinding> =
        if (layout == ControllerLayout.WITH_STICKS) defaults()
        else defaults().filterNot { it.input.startsWith("virtual:ls") || it.input.startsWith("virtual:rs") }

    /**
     * PC-88 action games read the numeric keypad far more often than the joystick port: the D-pad
     * is the keypad's arrows, the left stick the joystick, and the right stick the cursor keys.
     */
    fun defaults() = listOf(
        ControllerBinding("virtual:a", listOf('Z'.code)),
        ControllerBinding("virtual:b", listOf('X'.code)),
        ControllerBinding("virtual:x", listOf('C'.code)),
        ControllerBinding("virtual:y", listOf(SPACE)),
        ControllerBinding("virtual:start", listOf(RETURN)),
        ControllerBinding("virtual:select", listOf(ESC)),
        ControllerBinding("virtual:menu", action = "menu"),
        ControllerBinding("virtual:up", listOf(KP_0 + 8)),
        ControllerBinding("virtual:down", listOf(KP_0 + 2)),
        ControllerBinding("virtual:left", listOf(KP_0 + 4)),
        ControllerBinding("virtual:right", listOf(KP_0 + 6)),
        ControllerBinding("virtual:lsup", joystick = "up"),
        ControllerBinding("virtual:lsdown", joystick = "down"),
        ControllerBinding("virtual:lsleft", joystick = "left"),
        ControllerBinding("virtual:lsright", joystick = "right"),
        ControllerBinding("virtual:rsup", listOf(UP)),
        ControllerBinding("virtual:rsdown", listOf(DOWN)),
        ControllerBinding("virtual:rsleft", listOf(LEFT)),
        ControllerBinding("virtual:rsright", listOf(RIGHT))
    )
}
