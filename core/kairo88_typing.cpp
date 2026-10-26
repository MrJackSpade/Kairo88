// The shared typer for C callers (kairo88_core.h k88_typer_*), as the app's frame host runs it.

#include <cstdio>
#include <string>

#include "kairo88_typing.h"

namespace {

kairo::StartupTyper typer;
Pc88Typing machine;

} // namespace

extern "C" int k88_typer_start(const char *script) {
    typer.start(script ? script : "");
    return 0;
}

extern "C" int k88_typer_frame(void) {
    std::string error;
    if (typer.frame(machine, error)) return 0;
    std::fprintf(stderr, "kairo88: %s\n", error.c_str());
    return -1;
}

extern "C" int k88_typer_active(void) { return typer.active() ? 1 : 0; }

extern "C" int k88_typer_typing(void) { return typer.typing() ? 1 : 0; }
