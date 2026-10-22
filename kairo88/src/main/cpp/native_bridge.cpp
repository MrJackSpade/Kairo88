// The QUASI88 core (core/) run by the shared frame host, and its JNI surface
// for com.mrjackspade.kairo88.Pc88Native.

#include <jni.h>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <unistd.h>

#include <array>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "kairo/frame_machine.h"
#include "kairo88_core.h"

namespace {

// QUASI88 key codes for the lock keys and the joypad (pc88/keyboard.h).
constexpr int kCapsKey = 162;
constexpr int kKanaKey = 151;
constexpr int kPad1Up = 224;
constexpr int kPad1A = 228;
// The frontend's joystick controls, in JoystickInputRouter.DEFAULT_CONTROLS order.
constexpr std::array<int, 6> kJoystick = {kPad1Up, kPad1Up + 1, kPad1Up + 2, kPad1Up + 3, kPad1A, kPad1A + 1};
constexpr const char *kStateFile = "/state.q88";

// What a game boots with, from Pc88Native.nativeStart.
struct Launch {
    std::string firmware;
    std::string home;
    std::string disk_a;
    std::string disk_b;
    k88_machine machine;
};

// One game on the QUASI88 machine. QUASI88 cannot restart in a process, so the
// first session starts it and every session boots its game with k88_boot().
class Pc88Core {
public:
    static std::unique_ptr<Pc88Core> boot(const Launch &launch, std::string &error) {
        static bool initialized = false;
        if (!initialized) {
            // QUASI88 keeps its own folders under $HOME/.quasi88.
            setenv("HOME", launch.home.c_str(), 1);
            std::vector<std::string> args = {"kairo88", "-romdir", launch.firmware, "-notool", "-nostatus",
                                             "-noconfig"};
            std::vector<char *> argv;
            for (auto &arg : args) argv.push_back(arg.data());
            if (k88_init(static_cast<int>(argv.size()), argv.data()) != 0) {
                error = "QUASI88 did not start";
                return nullptr;
            }
            initialized = true;
        }
        if (k88_boot(&launch.machine, launch.disk_a.empty() ? nullptr : launch.disk_a.c_str(),
                     launch.disk_b.empty() ? nullptr : launch.disk_b.c_str()) != 0) {
            error = "A disk image could not be opened";
            return nullptr;
        }
        return std::unique_ptr<Pc88Core>(new Pc88Core());
    }

    ~Pc88Core() { release_inputs(); }

    bool run_frame() {
        if (k88_run_frame() != 0) {
            error_ = "QUASI88 stopped";
            return false;
        }
        return true;
    }

    std::string error() { return error_; }

    const uint8_t *frame(uint32_t &width, uint32_t &height) {
        int w = 0, h = 0;
        const uint32_t *pixels = k88_frame(&w, &h);
        width = static_cast<uint32_t>(w);
        height = static_cast<uint32_t>(h);
        return reinterpret_cast<const uint8_t *>(pixels);
    }

    double frame_seconds() { return 1.0 / k88_frame_rate(); }
    int32_t audio_rate() { return k88_audio_rate(); }

    uint32_t read_audio(int16_t *stereo, uint32_t max_frames) {
        return static_cast<uint32_t>(k88_audio_read(stereo, static_cast<int>(max_frames)));
    }

    void release_inputs() {
        for (int code = 0; code < static_cast<int>(keys_.size()); ++code) {
            if (keys_[code]) key(code, false);
        }
        for (int control = 0; control < static_cast<int>(kJoystick.size()); ++control) {
            if (pads_[control]) joystick(control, false);
        }
        for (int button = 1; button <= 2; ++button) {
            if (buttons_[button - 1]) mouse_button(button, false);
        }
    }

    void key(int code, bool down) {
        // Caps and Kana toggle on each press; they are never held.
        if (code != kCapsKey && code != kKanaKey) {
            if (keys_[code] == down) return;
            keys_[code] = down;
        }
        k88_key(code, down ? 1 : 0);
    }

    void joystick(int control, bool down) {
        pads_[control] = down;
        k88_pad(kJoystick[control], down ? 1 : 0);
    }

    void mouse_move(int dx, int dy) { k88_mouse_move(dx, dy); }

    void mouse_button(int button, bool down) {
        buttons_[button - 1] = down;
        k88_mouse_button(button, down ? 1 : 0);
    }

    int disk(int drive, const std::string &path) {
        return k88_disk(drive, path.empty() ? nullptr : path.c_str());
    }

    void reset() {
        release_inputs();
        k88_reset();
    }

    int save_state(const std::string &directory) {
        return k88_state_save((directory + kStateFile).c_str()) == 0 ? 0 : kStateFailed;
    }

    int load_state(const std::string &directory) {
        release_inputs();
        return k88_state_load((directory + kStateFile).c_str()) == 0 ? 0 : kStateFailed;
    }

    // MainActivity.stateError reads these.
    static constexpr int kStateFailed = 2;

private:
    Pc88Core() = default;

    std::string error_;
    std::array<bool, 256> keys_{};
    std::array<bool, kJoystick.size()> pads_{};
    std::array<bool, 2> buttons_{};
};

kairo::FrameMachine<Pc88Core> machine;

std::string utf8(JNIEnv *env, jstring value) {
    if (!value) return "";
    const char *chars = env->GetStringUTFChars(value, nullptr);
    std::string result = chars ? chars : "";
    if (chars) env->ReleaseStringUTFChars(value, chars);
    return result;
}

// QUASI88 reports problems on stdout and stderr, which Android discards.
void forward_output_to_log() {
    static int pipe_fds[2];
    if (pipe(pipe_fds) != 0) return;
    dup2(pipe_fds[1], STDOUT_FILENO);
    dup2(pipe_fds[1], STDERR_FILENO);
    setvbuf(stdout, nullptr, _IOLBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);
    std::thread([] {
        char buffer[512];
        std::string line;
        ssize_t count;
        while ((count = read(pipe_fds[0], buffer, sizeof(buffer))) > 0) {
            for (ssize_t i = 0; i < count; ++i) {
                if (buffer[i] == '\n') {
                    __android_log_write(ANDROID_LOG_WARN, "QUASI88", line.c_str());
                    line.clear();
                } else {
                    line += buffer[i];
                }
            }
        }
    }).detach();
}

} // namespace

extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM *, void *) {
    forward_output_to_log();
    return JNI_VERSION_1_6;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_mrjackspade_kairo88_Pc88Native_nativeStart(JNIEnv *env, jobject, jstring firmware, jstring home,
                                                    jstring disk_a, jstring disk_b, jint basic, jboolean clock_8mhz,
                                                    jint sound, jint port) {
    Launch launch{utf8(env, firmware), utf8(env, home), utf8(env, disk_a), utf8(env, disk_b),
                  {basic, clock_8mhz == JNI_TRUE ? 1 : 0, sound, port}};
    return machine.start([launch](std::string &error) { return Pc88Core::boot(launch, error); }) ? JNI_TRUE
                                                                                                  : JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_mrjackspade_kairo88_Pc88Native_nativeStop(JNIEnv *, jobject) {
    machine.stop();
}

extern "C" JNIEXPORT void JNICALL
Java_com_mrjackspade_kairo88_Pc88Native_nativePause(JNIEnv *, jobject, jboolean paused) {
    machine.pause(paused == JNI_TRUE);
}

extern "C" JNIEXPORT void JNICALL
Java_com_mrjackspade_kairo88_Pc88Native_nativeReset(JNIEnv *, jobject) {
    machine.post([](Pc88Core &core) { core.reset(); });
}

extern "C" JNIEXPORT void JNICALL
Java_com_mrjackspade_kairo88_Pc88Native_nativeKey(JNIEnv *, jobject, jint code, jboolean down) {
    if (code < 0 || code > 255) return;
    machine.post([code, down](Pc88Core &core) { core.key(code, down == JNI_TRUE); });
}

extern "C" JNIEXPORT void JNICALL
Java_com_mrjackspade_kairo88_Pc88Native_nativeJoystick(JNIEnv *, jobject, jint control, jboolean down) {
    if (control < 0 || control >= static_cast<jint>(kJoystick.size())) return;
    machine.post([control, down](Pc88Core &core) { core.joystick(control, down == JNI_TRUE); });
}

extern "C" JNIEXPORT void JNICALL
Java_com_mrjackspade_kairo88_Pc88Native_nativeMouseMove(JNIEnv *, jobject, jint dx, jint dy) {
    machine.post([dx, dy](Pc88Core &core) { core.mouse_move(dx, dy); });
}

extern "C" JNIEXPORT void JNICALL
Java_com_mrjackspade_kairo88_Pc88Native_nativeMouseButton(JNIEnv *, jobject, jint button, jboolean down) {
    if (button != 1 && button != 2) return;
    machine.post([button, down](Pc88Core &core) { core.mouse_button(button, down == JNI_TRUE); });
}

extern "C" JNIEXPORT jint JNICALL
Java_com_mrjackspade_kairo88_Pc88Native_nativeDisk(JNIEnv *env, jobject, jint drive, jstring path) {
    if (drive < 0 || drive > 1) return -1;
    std::string image = utf8(env, path);
    return machine.call([drive, image](Pc88Core &core) { return core.disk(drive, image); });
}

extern "C" JNIEXPORT jobjectArray JNICALL
Java_com_mrjackspade_kairo88_Pc88Native_nativeDiskPaths(JNIEnv *env, jobject) {
    std::array<std::string, 2> paths;
    const int result = machine.call([&paths](Pc88Core &) {
        paths = {k88_disk_path(0), k88_disk_path(1)};
        return 0;
    });
    if (result != 0) return nullptr;
    jobjectArray array = env->NewObjectArray(2, env->FindClass("java/lang/String"), nullptr);
    for (int drive = 0; drive < 2; ++drive)
        env->SetObjectArrayElement(array, drive, env->NewStringUTF(paths[drive].c_str()));
    return array;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_mrjackspade_kairo88_Pc88Native_nativeSaveState(JNIEnv *env, jobject, jstring directory) {
    std::string path = utf8(env, directory);
    return machine.call([path](Pc88Core &core) { return core.save_state(path); });
}

extern "C" JNIEXPORT jint JNICALL
Java_com_mrjackspade_kairo88_Pc88Native_nativeLoadState(JNIEnv *env, jobject, jstring directory) {
    std::string path = utf8(env, directory);
    return machine.call([path](Pc88Core &core) { return core.load_state(path); });
}

extern "C" JNIEXPORT jint JNICALL
Java_com_mrjackspade_kairo88_Pc88Native_nativeLocks(JNIEnv *, jobject) {
    return machine.call([](Pc88Core &) { return k88_locks(); });
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_mrjackspade_kairo88_Pc88Native_nativeStatus(JNIEnv *env, jobject) {
    return env->NewStringUTF(machine.status().c_str());
}

extern "C" JNIEXPORT void JNICALL
Java_com_mrjackspade_kairo88_Pc88Native_nativeSetSurface(JNIEnv *env, jobject, jobject surface) {
    machine.set_window(surface ? ANativeWindow_fromSurface(env, surface) : nullptr);
}

extern "C" JNIEXPORT void JNICALL
Java_com_mrjackspade_kairo88_Pc88Native_nativeSetShaderPreset(JNIEnv *env, jobject, jstring path) {
    machine.set_shader_preset(utf8(env, path));
}

extern "C" JNIEXPORT void JNICALL
Java_com_mrjackspade_kairo88_Pc88Native_nativeSetMuted(JNIEnv *, jobject, jboolean muted) {
    machine.set_muted(muted == JNI_TRUE);
}

extern "C" JNIEXPORT void JNICALL
Java_com_mrjackspade_kairo88_Pc88Native_nativeSetFastForward(JNIEnv *, jobject, jboolean enabled) {
    machine.set_fast_forward(enabled == JNI_TRUE);
}
