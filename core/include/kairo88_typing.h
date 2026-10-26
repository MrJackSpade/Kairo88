// The PC-88 as the shared typer's machine (shared/frontend/src/main/cpp/kairo/startup_typer.h):
// its prompts, keys and tape names (kairo88_loader.c), for the app's core and the host tests.

#pragma once

#include <string>
#include <vector>

#include "kairo/startup_typer.h"
#include "kairo88_core.h"

struct Pc88Typing {
    kairo::Prompt prompt() {
        switch (k88_prompt()) {
        case K88_PROMPT_LINE: return kairo::Prompt::Line;
        case K88_PROMPT_RETURN: return kairo::Prompt::Return;
        default: return kairo::Prompt::None;
        }
    }

    bool typed_key(char c, int &code, bool &shift) {
        int shifted = 0;
        const int key = k88_typed_key(c, &shifted);
        if (key < 0) return false;
        code = key;
        shift = shifted != 0;
        return true;
    }

    int special_key(kairo::SpecialKey key) {
        switch (key) {
        case kairo::SpecialKey::Return: return K88_KEY_RETURN;
        case kairo::SpecialKey::Shift: return K88_KEY_SHIFT;
        case kairo::SpecialKey::Ctrl: return K88_KEY_CTRL;
        case kairo::SpecialKey::F1:
        case kairo::SpecialKey::F2:
        case kairo::SpecialKey::F3:
        case kairo::SpecialKey::F4:
        case kairo::SpecialKey::F5:
            return K88_KEY_F1 + (static_cast<int>(key) - static_cast<int>(kairo::SpecialKey::F1));
        }
        return -1;
    }

    // {NAME}: the tape's first BASIC program.
    bool expand_command(std::string &line) {
        std::vector<char> buffer(line.begin(), line.end());
        buffer.resize(line.size() + 64, '\0');
        if (k88_expand_name(buffer.data(), static_cast<int>(buffer.size())) != 0) return false;
        line = buffer.data();
        return true;
    }

    void key(int code, bool down) { k88_key(code, down ? 1 : 0); }
};
