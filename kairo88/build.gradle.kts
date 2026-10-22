plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

extra["kairoProduct"] = "kairo88"
extra["kairoLabel"] = "Kairo88"
apply(from = rootProject.file("shared/gradle/kairo-machine-app.gradle"))
