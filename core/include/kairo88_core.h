#ifndef KAIRO88_CORE_H
#define KAIRO88_CORE_H

/* The Kairo88 PC-8801 core: QUASI88 driven one frame at a time.
 *
 * QUASI88 keeps its state in globals, so there is one machine per process.
 * Call k88_start() once; it fails if called again. */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Starts the machine. `argv` is a QUASI88 command line (argv[0] is the program
 * name), for example { "kairo88", "-romdir", "/path", "game.d88" }.
 * Returns 0 on success, or -1 when QUASI88 rejected the options or failed to
 * start; QUASI88 has already printed why to stderr. */
int k88_start(int argc, char *argv[]);

/* Runs the emulator until it completes one video frame. Returns 0, or -1 once
 * QUASI88 has quit. */
int k88_run_frame(void);

/* The current frame, 0xAARRGGBB pixels, `*width` x `*height`, no row padding. */
const uint32_t *k88_frame(int *width, int *height);

/* A PC-8801 key (QUASI88 KEY88_* code) going down (pressed != 0) or up. */
void k88_key(int key88, int pressed);

/* Audio: stereo 16-bit frames at k88_audio_rate() Hz, produced as frames run.
 * k88_audio_read copies up to `max_frames` frames into `stereo` (interleaved)
 * and returns how many it copied. Unread audio is kept for one second; older
 * frames are dropped. */
int k88_audio_rate(void);
int k88_audio_available(void);
int k88_audio_read(int16_t *stereo, int max_frames);

/* Shuts the machine down. */
void k88_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* KAIRO88_CORE_H */
