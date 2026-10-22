apply(from = "shared/gradle/android-settings.gradle")

rootProject.name = "Kairo88"
include(":kairo88", ":frontend")
project(":frontend").projectDir = file("shared/frontend")
