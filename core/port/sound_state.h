/* The sound driver's state in QUASI88 state files (sound/snddrv_ymfm.cpp). */
#ifndef KAIRO_SOUND_STATE_H
#define KAIRO_SOUND_STATE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The driver's whole state, valid until the next call. */
const uint8_t *kairo_sound_state(int *size);

/* A load: begin holds the driver's rendering, the system hook fills the buffer
 * with the state file's sound section, and end (after QUASI88 has finished
 * loading) puts it back and resumes rendering. Without a buffer, end only
 * resumes. end returns -1 when the section is not one this driver wrote. */
void kairo_sound_load_begin(void);
uint8_t *kairo_sound_load_buffer(int size);
int kairo_sound_load_end(void);

#ifdef __cplusplus
}
#endif

#endif
