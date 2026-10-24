plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

extra["kairoProduct"] = "kairo88"
extra["kairoLabel"] = "Kairo88"
extra["kairoCatalogRules"] = "com.mrjackspade.kairo88.Pc88Catalog"
apply(from = rootProject.file("shared/gradle/kairo-app.gradle"))
