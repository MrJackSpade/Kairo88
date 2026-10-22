package com.mrjackspade.kairo88

import com.mrjackspade.kairo.frontend.LayeredGameCatalog
import com.mrjackspade.kairo.frontend.MachineOption
import org.json.JSONArray

/**
 * Kairo88's catalog records beyond the common fields: the machine a game boots (`machine`:
 * `basic`, `clock` in MHz, `sound` board, joystick `port` device) and a second floppy for drive 2
 * (media role `floppyB`).
 */
object Pc88Catalog : LayeredGameCatalog.Schema {
    override val platform = "pc88"
    override val updateUrl = "https://raw.githubusercontent.com/MrJackSpade/Kairo88/main/catalog/core-v3.json"
    override val metadataUrl = "https://raw.githubusercontent.com/MrJackSpade/Kairo88/main/catalog/core-v3.meta.json"
    override val coreVersion = 3
    override val selfHostedArt = "/MrJackSpade/Kairo88/main/catalog/artwork/art/catalog/"
    override val contentId = Regex("sha256-(?:fd|tape)-v1:[0-9a-f]{64}")
    override val mediaExtensions = Regex("\\.(?:zip|7z|d88|88d|t88|cmt)$", RegexOption.IGNORE_CASE)
    override val fields: Map<String, Set<String>?> = mapOf(MachineOption.SECTION to Pc88Native.options.map { it.id }.toSet())
    override val singleMediaRoles = setOf("floppyB")

    override fun validMedia(role: String, contentId: String) = role == "floppyB" && contentId.startsWith("sha256-fd-v1:")

    override fun validField(field: String, value: Any): Boolean =
        field == MachineOption.SECTION && MachineOption.validSection(Pc88Native.options, value)

    override fun validBindings(bindings: JSONArray) = Pc88Keys.bindings.valid(bindings)
}
