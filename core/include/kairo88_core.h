#ifndef KAIRO88_CORE_H
#define KAIRO88_CORE_H

/* The Kairo88 PC-8801 core: QUASI88 driven one frame at a time.
 *
 * QUASI88 keeps its state in globals and cannot be shut down and started again,
 * so there is one machine per process: k88_init() starts it once, and each game
 * is a k88_boot() that inserts its disks and resets the machine. */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Starts QUASI88 once per process. `argv` is a QUASI88 command line (argv[0] is
 * the program name), for example { "kairo88", "-romdir", "/path", "-notool",
 * "-nostatus" }. Returns 0 on success, or -1 when it was already started, or
 * QUASI88 rejected the options or failed to start; QUASI88 has already printed
 * why to stderr. */
int k88_init(int argc, char *argv[]);

/* The machine a game boots: its BASIC mode, CPU clock, sound board, and what
 * is plugged into the joystick port. */
enum k88_basic { K88_BASIC_N = 0, K88_BASIC_V1S = 1, K88_BASIC_V1H = 2, K88_BASIC_V2 = 3 };
enum k88_sound { K88_SOUND_I = 0, K88_SOUND_II = 1 };
enum k88_port { K88_PORT_JOYSTICK = 0, K88_PORT_MOUSE = 1 };
typedef struct {
	int basic;      /* enum k88_basic */
	int clock_8mhz; /* nonzero for 8 MHz, else 4 MHz */
	int sound;      /* enum k88_sound */
	int port;       /* enum k88_port */
} k88_machine;

/* Ejects both drives, reads the ROM files again (they may have changed since
 * k88_init), inserts `disk_a` and `disk_b` (either may be NULL), and resets
 * into `machine`. With no disk in drive A it boots from ROM BASIC.
 * Pending audio is discarded. Returns 0, or -1 when a disk could not be opened
 * (the machine then runs with what was inserted). */
int k88_boot(const k88_machine *machine, const char *disk_a, const char *disk_b);

/* Runs the emulator until it completes one video frame. Returns 0, or -1 once
 * QUASI88 has quit. */
int k88_run_frame(void);

/* The current frame, `*width` x `*height` pixels with no row padding, each
 * RGBA8 in memory: 0xAABBGGRR read as a little-endian uint32_t. */
const uint32_t *k88_frame(int *width, int *height);

/* The machine's frame rate (its vsync frequency) in Hz. */
double k88_frame_rate(void);

/* A PC-8801 key (QUASI88 KEY88_* code) going down (pressed != 0) or up. Caps
 * and Kana lock mechanically on the real keyboard: each press toggles them,
 * and their release does nothing. */
void k88_key(int key88, int pressed);

/* The lock keys that are on: bit 0 Caps, bit 1 Kana. */
int k88_locks(void);

/* A joypad control (QUASI88 KEY88_PAD1_* or KEY88_PAD2_* code) going down or up.
 * The machine reads it while a joystick is in its port. */
void k88_pad(int key88, int pressed);

/* Mouse movement in PC-8801 mouse counts, and button 1 (left) or 2 (right).
 * The machine reads them while a mouse is in its port. */
void k88_mouse_move(int dx, int dy);
void k88_mouse_button(int button, int pressed);

/* The image in drive 0 or 1, or "" when it is empty. */
const char *k88_disk_path(int drive);

/* Inserts a disk image into drive 0 or 1, or ejects the drive when `path` is
 * NULL. Returns 0, or -1 when QUASI88 could not open the image. */
int k88_disk(int drive, const char *path);

/* Puts a T88 or CMT tape image in the cassette deck, rewound, or empties it for NULL; 0 when done. */
int k88_tape(const char *path);
/* The tape in the deck; "" when it is empty. */
const char *k88_tape_path(void);
/* Whether the machine is reading the tape: its motor is on and the tape has not reached its end. */
int k88_tape_running(void);
/* The text screen as the machine shows it now: its rows, newline-separated, each character as its
 * byte (0x20 to 0x7e as ASCII), trailing spaces trimmed. Writes at most size bytes with the
 * terminating NUL and returns the number written without it. */
int k88_screen_text(char *out, int size);

/* A tape's loader (kairo88_loader.c): after k88_boot, types its newline-separated lines one at a
 * time, each when the screen shows a prompt (BASIC's Ok or the monitor's ] or *), answering BASIC's
 * "How many files" with Return. {CTRL-x} in a line is Ctrl with x, {F1} to {F5} a function key,
 * {NAME} the name of the tape's first BASIC program (insert the tape with k88_tape first).
 * NULL or "" for none; -1 for a script it cannot type. */
int k88_loader_start(const char *script);
/* Whether the loader still has lines to type. */
int k88_loader_active(void);
/* Advances the loader by one frame, after k88_run_frame; -1 when a line cannot be typed. */
int k88_loader_frame(void);

/* Resets the machine, keeping its disks and configuration. */
void k88_reset(void);

/* Saves or loads the whole machine to or from `path`. A loaded state reopens the
 * disk images it was saved with, by their paths. Returns 0 or -1. */
int k88_state_save(const char *path);
int k88_state_load(const char *path);

/* Audio: stereo 16-bit frames at k88_audio_rate() Hz, produced as frames run.
 * k88_audio_read copies up to `max_frames` frames into `stereo` (interleaved)
 * and returns how many it copied. Unread audio is kept for one second; older
 * frames are dropped. */
int k88_audio_rate(void);
int k88_audio_available(void);
int k88_audio_read(int16_t *stereo, int max_frames);

/* Discards the audio not yet read. */
void k88_audio_clear(void);

#ifdef __cplusplus
}
#endif

#endif /* KAIRO88_CORE_H */
