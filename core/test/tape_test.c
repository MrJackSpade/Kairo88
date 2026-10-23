/* Boots N88-BASIC from ROM with a tape in the deck and drives it like a player at the keyboard.
 *
 * Usage: tape_test <rom-dir> <out-prefix> <basic 0-3> <tape> [--poll] <step>...
 *   Each step is one of:
 *     wait=<text>   run until the text screen shows <text> (at most 60 seconds of guest time)
 *     type=<line>   type <line> and Return; {CTRL-x} presses Ctrl with x, {F1}..{F5} a function key,
 *                   {QUOTE} a double quote
 *     load=<lines>  run the app's loader with these lines, separated by |, until it has typed them
 *     frames=<n>    run <n> frames
 *   The screen text is printed after each step, and <out-prefix>-<n>.ppm written for step n.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kairo88_core.h"

/* QUASI88's tape position (pc88/pc88main.c): bytes read and the image's size, both 0 at its end. */
int sio_tape_pos(long *cur, long *end);

enum { KEY_SHIFT = 152, KEY_CTRL = 153, KEY_RETURN = 145, KEY_F1 = 165 };

static int run(int frames)
{
	int16_t audio[4096 * 2];
	for (int i = 0; i < frames; ++i) {
		/* The app's loader, once a load= step starts it, advances every frame as in the app. */
		if (k88_run_frame() != 0 || k88_loader_frame() != 0) return -1;
		while (k88_audio_read(audio, 4096) > 0) {}
	}
	return 0;
}

static void write_ppm(const char *prefix, int index)
{
	int width = 0, height = 0;
	const uint32_t *pixels = k88_frame(&width, &height);
	char name[1024];
	snprintf(name, sizeof(name), "%s-%02d.ppm", prefix, index);
	FILE *out = fopen(name, "wb");
	if (!out) return;
	fprintf(out, "P6\n%d %d\n255\n", width, height);
	for (int i = 0; i < width * height; ++i) {
		const uint32_t p = pixels[i];
		const unsigned char rgb[3] = {(unsigned char) (p >> 16), (unsigned char) (p >> 8), (unsigned char) p};
		fwrite(rgb, 1, 3, out);
	}
	fclose(out);
}

/* A key held for five frames and released for five, with Shift held around it on frames of its own,
 * so BASIC's keyboard scan sees each change. */
static void tap(int key, int shifted)
{
	if (shifted) { k88_key(KEY_SHIFT, 1); run(2); }
	k88_key(key, 1);
	run(5);
	k88_key(key, 0);
	if (shifted) { run(2); k88_key(KEY_SHIFT, 0); }
	run(5);
}

/* The PC-88 key and Shift for a character of a command line. */
static int key_of(char c, int *shifted)
{
	static const char *const shifted_digits = "!\"#$%&'()";
	*shifted = 0;
	if (c >= 'a' && c <= 'z') c = (char) (c - 'a' + 'A');
	if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' ' || c == ':' || c == ',' || c == '.' ||
	    c == '-' || c == '/' || c == ';')
		return c;
	const char *digit = strchr(shifted_digits, c);
	if (digit) {
		*shifted = 1;
		return '1' + (int) (digit - shifted_digits);
	}
	if (c == '=') { *shifted = 1; return '-'; }
	fprintf(stderr, "tape_test: cannot type '%c'\n", c);
	exit(2);
}

static void type_line(const char *line)
{
	for (const char *c = line; *c; ++c) {
		if (*c == '{') {
			const char *end = strchr(c, '}');
			if (!end) { fprintf(stderr, "tape_test: unclosed {\n"); exit(2); }
			if (strncmp(c, "{CTRL-", 6) == 0) {
				k88_key(KEY_CTRL, 1);
				run(2);
				tap(c[6], 0);
				k88_key(KEY_CTRL, 0);
				run(2);
			} else if (c[1] == 'F' && c[2] >= '1' && c[2] <= '5') {
				tap(KEY_F1 + (c[2] - '1'), 0);
			} else if (strncmp(c, "{QUOTE}", 7) == 0) {
				tap('2', 1);
			}
			c = end;
			continue;
		}
		int shifted;
		const int key = key_of(*c, &shifted);
		tap(key, shifted);
	}
	tap(KEY_RETURN, 0);
}

int main(int argc, char **argv)
{
	if (argc < 6) {
		fprintf(stderr, "usage: tape_test <rom-dir> <out-prefix> <basic 0-3> <tape> <step>...\n");
		return 2;
	}
	/* --poll, before the steps, reads the tape by polling instead of interrupts (QUASI88's -cmt_poll). */
	int first = 5;
	char *args[] = {"tape_test", "-romdir", argv[1], "-notool", "-nostatus", "-noconfig", "-cmt_poll"};
	int count = 6;
	if (argc > 5 && strcmp(argv[5], "--poll") == 0) { count = 7; first = 6; }
	if (k88_init(count, args) != 0) return 1;
	k88_machine machine = {atoi(argv[3]), 0, 0, 0};
	if (k88_boot(&machine, NULL, NULL) != 0) return 1;
	if (k88_tape(argv[4]) != 0) return 1;
	static char text[8192];
	for (int i = first; i < argc; ++i) {
		const char *step = argv[i];
		if (strncmp(step, "wait=", 5) == 0) {
			int frames = 0;
			for (;;) {
				if (run(1) != 0) return 1;
				k88_screen_text(text, sizeof(text));
				if (strstr(text, step + 5)) break;
				if (++frames > 60 * 60) {
					printf("step %d: '%s' never appeared\n", i - first + 1, step + 5);
					break;
				}
			}
		} else if (strncmp(step, "type=", 5) == 0) {
			type_line(step + 5);
		} else if (strncmp(step, "load=", 5) == 0) {
			/* The app's loader (kairo88_loader.c), lines separated by |, until it has typed them all. */
			char script[512];
			snprintf(script, sizeof(script), "%s", step + 5);
			for (char *bar = script; (bar = strchr(bar, '|')); ) *bar = '\n';
			/* {QUOTE} stands for a double quote, which a shell may strip from an argument. */
			for (char *quote; (quote = strstr(script, "{QUOTE}")); ) {
				*quote = '"';
				memmove(quote + 1, quote + 7, strlen(quote + 7) + 1);
			}
			if (k88_loader_start(script) != 0) { fprintf(stderr, "tape_test: bad loader\n"); return 2; }
			/* Until it has typed every line; later steps keep it running if a load takes longer. */
			int frames = 0;
			while (k88_loader_active() && frames++ < 120 * 60)
				if (run(1) != 0) return 1;
		} else if (strncmp(step, "frames=", 7) == 0) {
			if (run(atoi(step + 7)) != 0) return 1;
		} else {
			fprintf(stderr, "tape_test: unknown step %s\n", step);
			return 2;
		}
		k88_screen_text(text, sizeof(text));
		long position, end;
		sio_tape_pos(&position, &end);
		printf("---- after step %d (%s), tape at %ld of %ld%s\n%s", i - first + 1, step, position, end,
		       k88_tape_running() ? ", running" : "", text);
		write_ppm(argv[2], i - first + 1);
	}
	return 0;
}
