/* The current frame as the picture the player sees: k88_frame's rows, each repeated as many
 * times as it stands for (a 200-line mode's 200 rows become 400), with no row padding. Valid
 * until the next call. */

#ifndef KAIRO88_TEST_PICTURE_H
#define KAIRO88_TEST_PICTURE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kairo88_core.h"

static const uint32_t *k88_picture(int *width, int *height)
{
	static uint32_t *picture = NULL;
	static size_t capacity = 0;
	int stride, repeat;
	const uint32_t *frame = k88_frame(width, height, &stride, &repeat);
	const size_t size = (size_t) *width * *height * repeat;
	if (size > capacity) {
		uint32_t *grown = realloc(picture, size * sizeof *picture);
		if (grown == NULL) {
			fprintf(stderr, "k88_picture: out of memory for %zu pixels\n", size);
			abort();
		}
		picture = grown;
		capacity = size;
	}
	for (int y = 0; y < *height * repeat; y++) {
		memcpy(picture + (size_t) y * *width, frame + (size_t) (y / repeat) * stride, sizeof *picture * *width);
	}
	*height *= repeat;
	return picture;
}

#endif
