/* The Kairo88 core API over QUASI88's start / loop / stop sequence. */

#include <stdio.h>

#include "kairo88_core.h"

#include "quasi88.h"
#include "getconf.h"
#include "event.h"
#include "device.h"

static int started = 0;

int k88_start(int argc, char *argv[])
{
	if (started) {
		fprintf(stderr, "kairo88: k88_start called twice\n");
		return -1;
	}
	if (!config_init(argc, argv, NULL, NULL, NULL)) {
		fprintf(stderr, "kairo88: QUASI88 rejected the options\n");
		return -1;
	}
	started = 1;
	quasi88_start();
	return 0;
}

int k88_run_frame(void)
{
	for (;;) {
		switch (quasi88_loop()) {
		case QUASI88_LOOP_ONE:
			return 0;
		case QUASI88_LOOP_EXIT:
			return -1;
		default:
			break;
		}
	}
}

const uint32_t *k88_frame(int *width, int *height)
{
	*width = kairo_frame_width;
	*height = kairo_frame_height;
	return kairo_frame;
}

void k88_key(int key88, int pressed)
{
	quasi88_key(key88, pressed);
}

void k88_stop(void)
{
	if (started) {
		quasi88_stop();
		config_exit();
		started = 0;
	}
}
