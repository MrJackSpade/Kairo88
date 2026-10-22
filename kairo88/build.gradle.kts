plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

apply(from = rootProject.file("shared/gradle/android-module.gradle"))
apply(from = rootProject.file("shared/gradle/catalog-binary.gradle"))
@Suppress("UNCHECKED_CAST")
val compileCatalogParts = extra["compileCatalogParts"] as groovy.lang.Closure<Unit>

android {
    androidResources { noCompress += listOf("json", "idx", "bin") }
    namespace = "com.mrjackspade.kairo88"

    defaultConfig {
        applicationId = providers.gradleProperty("kairo88ApplicationId").orNull ?: "com.loxifi.kairo88"
        versionCode = providers.gradleProperty("kairo88VersionCode").orNull?.toInt() ?: 1
        versionName = providers.gradleProperty("kairo88VersionName").orNull ?: "0.1.0"
        ndk {
            abiFilters += "arm64-v8a"
        }
        externalNativeBuild {
            cmake {
                arguments += "-DANDROID_STL=c++_shared"
            }
        }
    }

    signingConfigs {
        create("beta") {
            val keystore = providers.environmentVariable("KAIRO88_BETA_KEYSTORE").orNull
            val password = providers.environmentVariable("KAIRO88_BETA_PASSWORD").orNull
            if (!keystore.isNullOrBlank() && !password.isNullOrBlank()) {
                storeFile = file(keystore)
                storePassword = password
                keyAlias = "kairo88-beta"
                keyPassword = password
            }
        }
    }

    buildTypes {
        getByName("debug") {
            if (!providers.environmentVariable("KAIRO88_BETA_KEYSTORE").orNull.isNullOrBlank()) {
                signingConfig = signingConfigs.getByName("beta")
            }
        }
        release {
            isMinifyEnabled = false
            if (!providers.environmentVariable("KAIRO88_BETA_KEYSTORE").orNull.isNullOrBlank()) {
                signingConfig = signingConfigs.getByName("beta")
            }
        }
    }

    flavorDimensions += "artwork"
    productFlavors {
        create("withImages") {
            dimension = "artwork"
            resValue("bool", "catalog_art_download_enabled", "true")
        }
        create("withoutImages") {
            dimension = "artwork"
            resValue("bool", "catalog_art_download_enabled", "true")
        }
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
        }
    }
}

android.sourceSets.getByName("main").assets.setSrcDirs(listOf(layout.buildDirectory.dir("generated/catalog-assets")))
android.sourceSets.getByName("withImages").assets.setSrcDirs(listOf(rootProject.file("catalog/artwork")))
val catalogAssets by tasks.registering(Sync::class) {
    from("src/main/assets") { exclude("catalog/**") }
    from(rootProject.file("catalog/parts")) {
        into("catalog")
        include("*.idx")
        if (providers.gradleProperty("kairoDistribution").orNull == "play") exclude("art.nsfw.*")
    }
    inputs.dir(rootProject.file("catalog/parts"))
    into(layout.buildDirectory.dir("generated/catalog-assets"))
    doLast {
        compileCatalogParts.call(rootProject.file("catalog/parts"),
            layout.buildDirectory.dir("generated/catalog-assets/catalog").get().asFile)
    }
}
tasks.named("preBuild") { dependsOn(catalogAssets) }

dependencies {
    implementation(project(":frontend"))
}
