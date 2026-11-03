package com.mrjackspade.kairo88

import com.mrjackspade.kairo.frontend.DiskImageLibrary
import com.mrjackspade.kairo.frontend.R as FrontendR
import com.mrjackspade.kairo.frontend.UiText

/** The library's media format, as the app's launch builds it (Kairo#125). */
object Pc88Media {
    val format = DiskImageLibrary.Format(mapOf("d88" to "fd", "88d" to "fd", "t88" to "tape", "cmt" to "tape"),
        mapOf("fd" to UiText.res(FrontendR.string.kairo_floppy_disk), "tape" to UiText.res(R.string.cassette_tape)), mapOf("fd" to 64L * 1024 * 1024, "tape" to 16L * 1024 * 1024),
        "PC-88")
}
