# Development

## Building

Build the app, and optionally install it, with the shared build script (see [Android build](../shared/docs/android-build.md)), which needs JDK 17, the Android SDK and NDK, and Rust with the `aarch64-linux-android` target:

```powershell
shared\tools\build.ps1 -Install <adb serial>
```

From a shell elsewhere, `./gradlew :kairo88:assembleWithImagesDebug` builds the same app.

## Emulator cores

The app runs QUASI88 with its sound output rebuilt on ymfm; [QUASI88 core](quasi88-core.md) covers how it is built, driven and tested on the host (`boot_test`, `restart_test`, `tape_test`). MAME's PC-8801 driver is also pinned in `third_party/mame` for comparison; the app does not use it.

<!-- kairo:ui-tests -->
## UI tests

Kairo88 has device tests in the style of Selenium tests, on the shared Kairo test framework: they
name the app's elements, act on them as a player would, and assert what the player would see
([how they work and how to run them](https://github.com/MrJackSpade/Kairo/blob/main/docs/ui-tests.md)). Run them from this
repository with a device connected over adb (Android 10 or later) and the `LOCAL_BUILD_TOKEN` key set:

```
shared\tools\uitest.ps1 -Device <serial>                                              every test
shared\tools\uitest.ps1 -Device <serial> -Filter 'MenuTests|ControllerTests'          by test name
shared\tools\uitest.ps1 -Device <serial> -Class com.mrjackspade.kairo88.uitest.<Class>   one class
shared\tools\uitest-all.ps1 -Device <serial>                                          every Kairo app
```

The tests are in `kairo88/src/androidTest/java/com/mrjackspade/kairo88/uitest/`. They run in a
build of their own, `com.loxifi.kairo88.uitest`, installed beside the app without touching its data, and
each run's logs, screenshots and summary land in `.build/uitest/<run id>/`.
<!-- /kairo:ui-tests -->
