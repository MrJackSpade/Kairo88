// The QUASI88 core (core/) as the MachineCore the shared frame host runs
// (shared/frontend/src/main/cpp/kairo/machine_core.h).

#include <array>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#include "kairo/machine_core.h"
#include "kairo/relative_warp.h"
#include "kairo88_core.h"
#include "kairo88_typing.h"

namespace {

// QUASI88 key codes for the lock keys and the joypad (pc88/keyboard.h).
constexpr int kCapsKey = 162;
constexpr int kKanaKey = 151;
constexpr int kPad1Up = 224;
constexpr int kPad1A = 228;
// The frontend's joystick controls, in JoystickInputRouter.DEFAULT_CONTROLS order.
constexpr std::array<int, 6> kJoystick = {kPad1Up, kPad1Up + 1, kPad1Up + 2, kPad1Up + 3, kPad1A, kPad1A + 1};
constexpr const char *kStateFile = "/state.q88";
// MainActivity's media slots: drives 1 and 2, then the cassette deck.
constexpr size_t kTapeSlot = 2;

// One game on the QUASI88 machine. QUASI88 cannot restart in a process, so the
// first session starts it and every session boots its game with k88_boot().
class Pc88Core : public kairo::MachineCore {
public:
    static std::unique_ptr<Pc88Core> boot(const kairo::Launch &launch, std::string &error) {
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
        // The settings Pc88Native.Machine names; every launch passes all of them.
        k88_machine machine{std::stoi(launch.settings.at("basic")), launch.settings.at("clock") == "8" ? 1 : 0,
                            std::stoi(launch.settings.at("sound")), std::stoi(launch.settings.at("port")),
                            std::stoi(launch.settings.at("subcpu"))};
        const std::string disk_a = launch.media.size() > 0 ? launch.media[0] : "";
        const std::string disk_b = launch.media.size() > 1 ? launch.media[1] : "";
        if (k88_boot(&machine, disk_a.empty() ? nullptr : disk_a.c_str(),
                     disk_b.empty() ? nullptr : disk_b.c_str()) != 0) {
            error = "A disk image could not be opened";
            return nullptr;
        }
        // After the drives, the tape. The shared frame host types the startup commands at BASIC's
        // prompt (kairo/startup_typer.h), with this core's prompts and keys (kairo88_typing.h).
        const std::string tape = launch.media.size() > kTapeSlot ? launch.media[kTapeSlot] : "";
        if (k88_tape(tape.empty() ? nullptr : tape.c_str()) != 0) {
            error = "The tape image could not be opened";
            return nullptr;
        }
        return std::unique_ptr<Pc88Core>(new Pc88Core());
    }

    ~Pc88Core() override { release_inputs(); }

    bool run_frame() override {
        if (warp_.running()) {
            int dx, dy;
            warp_.step(k88_mouse_idle() != 0, dx, dy);
            if (dx || dy) k88_mouse_move(dx, dy);
        }
        if (k88_run_frame() != 0) {
            error_ = "QUASI88 stopped";
            return false;
        }
        return true;
    }

    // While the tape is read, the frame host runs the machine as fast as it can: a cassette loads in
    // minutes at its real speed. (It does so while a startup command goes in, too.)
    // Flat out while the tape runs or a disk loads (Kairo#82).
    bool loading() override { return k88_tape_running() != 0 || k88_disk_busy() != 0; }

    // The startup commands go in at BASIC's prompts (kairo88_typing.h).
    kairo::Prompt prompt() override { return typing_.prompt(); }
    bool typed_key(char c, int &code, bool &shift) override { return typing_.typed_key(c, code, shift); }
    int special_key(kairo::SpecialKey key) override { return typing_.special_key(key); }
    bool expand_command(std::string &line) override { return typing_.expand_command(line); }

    // The tape's progress ("tape=read/size"), so a test can tell a slow load from a stalled one.
    std::string details() override {
        long read = 0, size = 0;
        if (k88_tape_position(&read, &size) != 0) return "";
        return "tape=" + std::to_string(read) + "/" + std::to_string(size);
    }

    std::string error() override { return error_; }

    const uint8_t *frame(uint32_t &width, uint32_t &height) override {
        int w = 0, h = 0, stride = 0, repeat = 0;
        const uint32_t *pixels = k88_frame(&w, &h, &stride, &repeat);
        width = static_cast<uint32_t>(w);
        height = static_cast<uint32_t>(h);
        stride_ = static_cast<uint32_t>(stride) * 4;
        repeat_ = static_cast<uint32_t>(repeat);
        return reinterpret_cast<const uint8_t *>(pixels);
    }

    // A 200-line mode's 200 rows are every other line of QUASI88's buffer (k88_frame).
    uint32_t frame_stride() override { return stride_; }
    uint32_t line_repeat() override { return repeat_; }

    double frame_seconds() override { return 1.0 / k88_frame_rate(); }
    int32_t audio_rate() override { return k88_audio_rate(); }

    uint32_t read_audio(int16_t *stereo, uint32_t max_frames) override {
        return static_cast<uint32_t>(k88_audio_read(stereo, static_cast<int>(max_frames)));
    }

    void release_inputs() override {
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

    void key(int code, bool down) override {
        if (code < 0 || code >= static_cast<int>(keys_.size())) return;
        // Caps and Kana toggle on each press; they are never held.
        if (code != kCapsKey && code != kKanaKey) {
            if (keys_[code] == down) return;
            keys_[code] = down;
        }
        k88_key(code, down ? 1 : 0);
    }

    void joystick(int control, bool down) override {
        if (control < 0 || control >= static_cast<int>(kJoystick.size())) return;
        pads_[control] = down;
        k88_pad(kJoystick[control], down ? 1 : 0);
    }

    void mouse_move(int dx, int dy) override {
        // A drag after a direct tap goes with the warp's counts until they are given.
        if (!warp_.hold(dx, dy)) k88_mouse_move(dx, dy);
    }
    // Direct touch: a push past the screen's top-left corner and the counts from there, in the
    // picture's own pixels (kairo::RelativeWarp), for a game that moves its pointer a pixel a count.
    void mouse_warp(int x, int y) override {
        uint32_t width = 0, height = 0;
        if (!frame(width, height)) return;
        warp_.start(x, y, int(width), int(height));
    }
    bool mouse_warp_done() override { return warp_.done(); }

    void mouse_button(int button, bool down) override {
        buttons_[button - 1] = down;
        k88_mouse_button(button, down ? 1 : 0);
    }

    int media(int slot, const std::string &path) override {
        if (slot == static_cast<int>(kTapeSlot)) return k88_tape(path.empty() ? nullptr : path.c_str());
        if (slot < 0 || slot > 1) return kUnsupported;
        return k88_disk(slot, path.empty() ? nullptr : path.c_str());
    }

    std::vector<std::string> media_paths() override { return {k88_disk_path(0), k88_disk_path(1), k88_tape_path()}; }

    // Caps is bit 0 and Kana bit 1, as Pc88Native lists them.
    int locks() override { return k88_locks(); }
    // Auto touch mode (Kairo#78): the guest clocking a mouse's data out is the mouse; with no
    // keyboard signal of its own, the keyboard stays the default.
    bool guest_input(GuestInput &input) override {
        input.mouse_reads = k88_mouse_strobes();
        return true;
    }

    void reset() override {
        release_inputs();
        k88_reset();
    }

    int save_state(const std::string &directory) override {
        return k88_state_save((directory + kStateFile).c_str()) == 0 ? 0 : kFailed;
    }

    int load_state(const std::string &directory) override {
        release_inputs();
        return k88_state_load((directory + kStateFile).c_str()) == 0 ? 0 : kFailed;
    }

    // MainActivity.stateError reads these.

private:
    Pc88Core() = default;

    std::string error_;
    Pc88Typing typing_;
    uint32_t stride_ = 0, repeat_ = 1;
    std::array<bool, 256> keys_{};
    std::array<bool, kJoystick.size()> pads_{};
    std::array<bool, 2> buttons_{};
    // Direct touch (mouse_warp).
    kairo::RelativeWarp warp_;
};

} // namespace

std::unique_ptr<kairo::MachineCore> kairo::create_core(const kairo::Launch &launch, std::string &error) {
    return Pc88Core::boot(launch, error);
}

// The blank disks MainActivity.blankDisks lists: "2d", a 320 KB floppy as QUASI88 makes one
// (32-byte header, 164 track offsets, 84 tracks of 0x1600 bytes).
std::string kairo::create_blank_disk(const std::string &kind, const std::string &path) {
    if (kind != "2d") return "Kairo88 makes no blank disk " + kind;
    if (k88_create_blank_disk(path.c_str()) != 0) return "QUASI88 could not write a blank disk at " + path;
    FILE *file = std::fopen(path.c_str(), "rb");
    if (!file) return "The blank disk " + path + " is not there after QUASI88 wrote it";
    std::fseek(file, 0, SEEK_END);
    const long size = std::ftell(file);
    std::fclose(file);
    constexpr long kSize = 32 + 164 * 4 + 84 * 0x1600;
    if (size != kSize)
        return "QUASI88 wrote " + std::to_string(size) + " bytes for a blank 2D disk, not " + std::to_string(kSize);
    return "";
}
