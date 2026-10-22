package com.mrjackspade.kairo88

import com.mrjackspade.kairo.frontend.CatalogGame
import com.mrjackspade.kairo.frontend.LayeredGameCatalog
import org.json.JSONArray
import org.json.JSONObject

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
    override val fields: Map<String, Set<String>?> = mapOf("machine" to setOf("basic", "clock", "sound", "port"))
    override val singleMediaRoles = setOf("floppyB")

    override fun validMedia(role: String, contentId: String) = role == "floppyB" && contentId.startsWith("sha256-fd-v1:")

    override fun validField(field: String, value: Any): Boolean = field == "machine" && value is JSONObject &&
        value.keys().asSequence().all { it in fields.getValue("machine")!! } &&
        (!value.has("basic") || Pc88Native.Basic.entries.any { it.id == value.opt("basic") }) &&
        (!value.has("clock") || value.opt("clock").let { it is Int && it in CLOCKS }) &&
        (!value.has("sound") || Pc88Native.Sound.entries.any { it.id == value.opt("sound") }) &&
        (!value.has("port") || Pc88Native.Port.entries.any { it.id == value.opt("port") })

    override fun validBindings(bindings: JSONArray) = Pc88Keys.bindings.valid(bindings)

    val CLOCKS = setOf(4, 8)

    fun basic(game: CatalogGame): Pc88Native.Basic? =
        game.section("machine")?.optString("basic")?.let { id -> Pc88Native.Basic.entries.firstOrNull { it.id == id } }
    fun clock(game: CatalogGame): Int? =
        game.section("machine")?.opt("clock")?.let { it as? Int }?.takeIf { it in CLOCKS }
    fun sound(game: CatalogGame): Pc88Native.Sound? =
        game.section("machine")?.optString("sound")?.let { id -> Pc88Native.Sound.entries.firstOrNull { it.id == id } }
    fun port(game: CatalogGame): Pc88Native.Port? =
        game.section("machine")?.optString("port")?.let { id -> Pc88Native.Port.entries.firstOrNull { it.id == id } }
}
