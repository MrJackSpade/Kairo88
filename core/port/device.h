#ifndef DEVICE_H_INCLUDED
#define DEVICE_H_INCLUDED

/* Shared state between the QUASI88 system layer (graph.c, event.c) and the
 * Kairo88 core API. */

#include <stdint.h>

/* The emulator's frame buffer, written by QUASI88 through graph_setup(). */
extern uint32_t *kairo_frame;
extern int kairo_frame_width;
extern int kairo_frame_height;

#endif /* DEVICE_H_INCLUDED */
