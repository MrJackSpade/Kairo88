package com.mrjackspade.kairo88

import com.mrjackspade.kairo.frontend.CatalogSetting
import com.mrjackspade.kairo.frontend.LayeredGameCatalog
import com.mrjackspade.kairo.frontend.MachineOption
import com.mrjackspade.kairo.frontend.CatalogGame
import com.mrjackspade.kairo.frontend.UiText
import org.json.JSONArray
import org.json.JSONObject

/**
 * Kairo88's catalog records beyond the common fields: the machine a game boots (`machine`:
 * `basic`, `clock` in MHz, `sound` board, joystick `port` device, disk `subcpu` sync), a second
 * floppy for drive 2 (media role `floppyB`), and the lines typed at BASIC's prompt to start the
 * game (`launch`: `commands`); a tape without any types its cassette load.
 */
object Pc88Catalog : LayeredGameCatalog.Schema {
    override val platform = "pc88"
    override val updateUrl = "https://raw.githubusercontent.com/MrJackSpade/Kairo88/main/catalog/core-v3.json"
    override val metadataUrl = "https://raw.githubusercontent.com/MrJackSpade/Kairo88/main/catalog/core-v3.meta.json"
    override val coreVersion = 3
    override val selfHostedArt = "/MrJackSpade/Kairo88/main/catalog/artwork/art/catalog/"
    override val contentId = Regex("sha256-(?:fd|tape)-v1:[0-9a-f]{64}")
    override val mediaExtensions = Regex("\\.(?:zip|7z|d88|88d|t88|cmt)$", RegexOption.IGNORE_CASE)
    override val settings: List<CatalogSetting> = Pc88Native.options.map(CatalogSetting::Choice) + listOf(
        CatalogSetting.Drive("floppyB", UiText.res(R.string.drive_2)) { it.startsWith(FLOPPY) },
        CatalogSetting.Lines(LAUNCH, COMMANDS, UiText.res(R.string.startup_commands),
            UiText.res(R.string.startup_commands_hint), Regex("[ -~]{1,40}"), 8) { game, options ->
            if (game.contentId.startsWith(TAPE_ID)) defaultLoader(options.getValue(Pc88Native.basic.id)) else emptyList()
        })
    override val fields = CatalogSetting.fields(settings)

    override fun validMedia(role: String, contentId: String) = role == "floppyB" && contentId.startsWith(FLOPPY)

    override fun validField(field: String, value: Any): Boolean =
        field in fields && CatalogSetting.validSection(settings, field, value)

    const val LAUNCH = "launch"
    const val COMMANDS = "commands"
    private const val FLOPPY = "sha256-fd-v1:"
    private const val TAPE_ID = "sha256-tape-v1:"
    /** How a tape without loader lines of its own loads in a BASIC mode: its cassette load, then RUN. */
    fun defaultLoader(basic: String) =
        listOf(if (basic == Pc88Native.Basic.N.id) "CLOAD\"{NAME}" else "LOAD\"CAS:\"", "RUN")

    override fun validBindings(bindings: JSONArray) = Pc88Keys.bindings.valid(bindings)
}

/** The lines typed to start this game in the machine [options] it boots: its own, or a tape's load. */
fun CatalogGame.startupCommands(options: Map<String, String>): List<String> =
    Pc88Catalog.settings.filterIsInstance<CatalogSetting.Lines>().single().value(this, options)
