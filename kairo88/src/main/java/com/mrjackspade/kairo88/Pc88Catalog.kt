package com.mrjackspade.kairo88

import com.mrjackspade.kairo.frontend.LayeredGameCatalog
import com.mrjackspade.kairo.frontend.MachineOption
import com.mrjackspade.kairo.frontend.CatalogGame
import org.json.JSONArray
import org.json.JSONObject

/**
 * Kairo88's catalog records beyond the common fields: the machine a game boots (`machine`:
 * `basic`, `clock` in MHz, `sound` board, joystick `port` device), a second floppy for drive 2
 * (media role `floppyB`), and for a tape the lines its loader types (`tape`: `loader`).
 */
object Pc88Catalog : LayeredGameCatalog.Schema {
    override val platform = "pc88"
    override val updateUrl = "https://raw.githubusercontent.com/MrJackSpade/Kairo88/main/catalog/core-v3.json"
    override val metadataUrl = "https://raw.githubusercontent.com/MrJackSpade/Kairo88/main/catalog/core-v3.meta.json"
    override val coreVersion = 3
    override val selfHostedArt = "/MrJackSpade/Kairo88/main/catalog/artwork/art/catalog/"
    override val contentId = Regex("sha256-(?:fd|tape)-v1:[0-9a-f]{64}")
    override val mediaExtensions = Regex("\\.(?:zip|7z|d88|88d|t88|cmt)$", RegexOption.IGNORE_CASE)
    override val fields: Map<String, Set<String>?> = mapOf(MachineOption.SECTION to Pc88Native.options.map { it.id }.toSet(),
        TAPE to setOf(LOADER))
    override val singleMediaRoles = setOf("floppyB")

    override fun validMedia(role: String, contentId: String) = role == "floppyB" && contentId.startsWith("sha256-fd-v1:")

    override fun validField(field: String, value: Any): Boolean = when (field) {
        MachineOption.SECTION -> MachineOption.validSection(Pc88Native.options, value)
        TAPE -> value is JSONObject && value.keys().asSequence().toList() == listOf(LOADER) &&
            value.optJSONArray(LOADER)?.let { lines ->
                lines.length() in 1..8 && (0 until lines.length()).all { (lines.opt(it) as? String)?.matches(LOADER_LINE) == true }
            } == true
        else -> false
    }

    const val TAPE = "tape"
    const val LOADER = "loader"
    private val LOADER_LINE = Regex("[ -~]{1,40}")
    /** How a tape without loader lines of its own loads in a BASIC mode: its cassette load, then RUN. */
    fun defaultLoader(basic: String) =
        listOf(if (basic == Pc88Native.Basic.N.id) "CLOAD\"{NAME}" else "LOAD\"CAS:\"", "RUN")

    override fun validBindings(bindings: JSONArray) = Pc88Keys.bindings.valid(bindings)
}

/** The lines this tape's loader types at the prompts of [basic] mode; its default when the catalog names none. */
fun CatalogGame.tapeLoader(basic: String): List<String> =
    section(Pc88Catalog.TAPE)?.optJSONArray(Pc88Catalog.LOADER)?.let { lines ->
        (0 until lines.length()).map(lines::getString)
    } ?: Pc88Catalog.defaultLoader(basic)
