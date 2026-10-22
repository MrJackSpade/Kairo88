/* QUASI88 graphics layer for Kairo88: a 32-bit frame buffer that the host
 * reads after each frame. Pixels are RGBA8 in memory (0xAABBGGRR as a
 * little-endian uint32_t), the layout of an Android RGBA_8888 window and an
 * OpenGL RGBA texture, so frames are shown without conversion. */

#include <stdlib.h>

#include "quasi88.h"
#include "graph.h"
#include "device.h"

uint32_t *kairo_frame = NULL;
int kairo_frame_width = 0;
int kairo_frame_height = 0;

static T_GRAPH_SPEC graph_spec;
static T_GRAPH_INFO graph_info;

const T_GRAPH_SPEC *graph_init(void)
{
	graph_spec.window_max_width = 640;
	graph_spec.window_max_height = 480;
	graph_spec.fullscreen_max_width = 0;
	graph_spec.fullscreen_max_height = 0;
	graph_spec.touchkey_window = FALSE;
	graph_spec.touchkey_fullscreen = FALSE;
	graph_spec.touchkey_notify = NULL;
	graph_spec.forbid_half = FALSE;
	return &graph_spec;
}

const T_GRAPH_INFO *graph_setup(int width, int height, int fullscreen, double aspect)
{
	(void) fullscreen;
	(void) aspect;
	if (kairo_frame == NULL || width != kairo_frame_width || height != kairo_frame_height) {
		uint32_t *frame = calloc((size_t) width * height, sizeof(uint32_t));
		if (frame == NULL) {
			return NULL;
		}
		free(kairo_frame);
		kairo_frame = frame;
		kairo_frame_width = width;
		kairo_frame_height = height;
	}
	graph_info.fullscreen = FALSE;
	graph_info.width = width;
	graph_info.height = height;
	graph_info.byte_per_pixel = 4;
	graph_info.byte_per_line = width * 4;
	graph_info.buffer = kairo_frame;
	graph_info.nr_color = 255;
	graph_info.write_only = FALSE;
	graph_info.broken_mouse = FALSE;
	graph_info.draw_start = NULL;
	graph_info.draw_finish = NULL;
	graph_info.dont_frameskip = NULL;
	return &graph_info;
}

void graph_exit(void)
{
	free(kairo_frame);
	kairo_frame = NULL;
	kairo_frame_width = 0;
	kairo_frame_height = 0;
}

void graph_add_color(const PC88_PALETTE_T color[], int nr_color, unsigned long pixel[])
{
	for (int i = 0; i < nr_color; i++) {
		pixel[i] = 0xff000000ul | ((unsigned long) color[i].blue << 16) |
			((unsigned long) color[i].green << 8) | (unsigned long) color[i].red;
	}
}

void graph_remove_color(int nr_pixel, unsigned long pixel[])
{
	(void) nr_pixel;
	(void) pixel;
}

void graph_update(int nr_rect, T_GRAPH_RECT rect[])
{
	(void) nr_rect;
	(void) rect;
}

void graph_set_window_title(const char *title)
{
	(void) title;
}

void graph_set_attribute(int mouse_show, int grab, int keyrepeat_on, int *result_show, int *result_grab)
{
	(void) mouse_show;
	(void) grab;
	(void) keyrepeat_on;
	*result_show = TRUE;
	*result_grab = FALSE;
}
