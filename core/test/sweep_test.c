/* The boot sweep's runner (tools/boot_sweep.py): boots one game after another in one process, as
 * the app does between games, and reports how far each got.
 *
 * Usage: sweep_test <rom dir> <frames> <check every> <sound 0/1>
 *   Reads one game a line from stdin, tab-separated:
 *     <id> <basic 0-3> <8 MHz 0/1> <sound board 0/1> <sub-CPU 0-2> <disk A> <disk B or -> <snapshot or ->
 *     <lines to type at BASIC's prompt, separated by \x1f, or ->
 *   boots it and runs <frames> frames with no sound synthesized, composing the picture only every
 *   <check every> frames. Prints one line a game, tab-separated:
 *     @sweep <id> <lit pixels at each check, comma-separated> <1 when lines were left untyped, else 0>
 *            <the text screen at the end, | between rows>
 *   and writes the last picture as a binary PPM to <snapshot>. Sound off and drawing skipped
 *   change nothing the machine computes (kairo88_core.h), so a game boots here as in the app.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kairo88_core.h"

/* Starts each result line, which tools/boot_sweep.py reads; QUASI88's own notes go to stdout too. */
static const char kMark[] = "@sweep\t";

static long lit_pixels(void)
{
	int width, height;
	const uint32_t *pixels = k88_frame(&width, &height);
	long lit = 0;
	for (long i = 0; i < (long) width * height; i++)
		lit += (pixels[i] & 0xffffff) != 0;
	return lit;
}

static int write_ppm(const char *path)
{
	int width, height;
	const uint32_t *pixels = k88_frame(&width, &height);
	FILE *out = fopen(path, "wb");
	if (!out) {
		fprintf(stderr, "sweep_test: cannot write %s\n", path);
		return -1;
	}
	fprintf(out, "P6\n%d %d\n255\n", width, height);
	for (long i = 0; i < (long) width * height; i++) {
		const unsigned char rgb[3] = { (pixels[i] >> 16) & 0xff, (pixels[i] >> 8) & 0xff, pixels[i] & 0xff };
		fwrite(rgb, 1, 3, out);
	}
	return fclose(out) == 0 ? 0 : -1;
}

int main(int argc, char **argv)
{
	if (argc != 5) {
		fprintf(stderr, "usage: sweep_test <rom dir> <frames> <check every> <sound 0/1>\n");
		return 2;
	}
	const int frames = atoi(argv[2]), every = atoi(argv[3]);
	if (frames <= 0 || every <= 0 || frames % every != 0) {
		fprintf(stderr, "sweep_test: <frames> must be a positive multiple of <check every>\n");
		return 2;
	}
	char *init[] = { argv[0], "-romdir", argv[1], "-notool", "-nostatus", "-noconfig" };
	if (k88_init(6, init) != 0)
		return 1;
	k88_sound_output(atoi(argv[4]));

	char line[8192];
	static int16_t audio[4096 * 2];
	while (fgets(line, sizeof line, stdin)) {
		line[strcspn(line, "\r\n")] = '\0';
		char *field[9];
		int count = 0;
		for (char *part = strtok(line, "\t"); part && count < 9; part = strtok(NULL, "\t"))
			field[count++] = part;
		if (count != 9) {
			fprintf(stderr, "sweep_test: a game line needs 9 fields, not %d\n", count);
			return 2;
		}
		const k88_machine machine = { atoi(field[1]), atoi(field[2]), atoi(field[3]), K88_PORT_JOYSTICK, atoi(field[4]) };
		if (k88_boot(&machine, field[5], strcmp(field[6], "-") ? field[6] : NULL) != 0) {
			printf("%s%s\topen-failed\t\n", kMark, field[0]);
			fflush(stdout);
			continue;
		}
		/* The lines typed at BASIC's prompt, as the app's loader types them, unit separators between. */
		if (strcmp(field[8], "-")) {
			for (char *c = field[8]; *c; c++)
				if (*c == '\x1f') *c = '\n';
		}
		if (k88_tape(NULL) != 0 || k88_loader_start(strcmp(field[8], "-") ? field[8] : NULL) != 0) {
			fprintf(stderr, "sweep_test: %s: the loader cannot type its lines\n", field[0]);
			return 2;
		}
		/* The line is printed whole at the end: QUASI88 prints notes of its own to stdout. */
		char checks[2048] = "";
		size_t used = 0;
		for (int frame = 1; frame <= frames; frame++) {
			/* Compose the picture on the frames the checks look at. */
			k88_draw_every(frame % every == 0 ? 1 : 1000000);
			if (k88_run_frame() != 0 || k88_loader_frame() != 0) {
				fprintf(stderr, "sweep_test: QUASI88 stopped or its loader failed\n");
				return 1;
			}
			while (k88_audio_read(audio, 4096) > 0) {
			}
			if (frame % every == 0)
				used += (size_t) snprintf(checks + used, sizeof checks - used, "%s%ld", frame == every ? "" : ",",
				                          lit_pixels());
		}
		char text[4096] = "";
		k88_screen_text(text, sizeof text);
		for (char *c = text; *c; c++)
			if (*c == '\n' || *c == '\r' || *c == '\t') *c = '|';
		printf("%s%s\t%s\t%d\t%s\n", kMark, field[0], checks, k88_loader_active(), text);
		fflush(stdout);
		if (strcmp(field[7], "-") && write_ppm(field[7]) != 0)
			return 1;
	}
	return 0;
}
