/* Boots several games in one process, as the app does between games, and
 * checks a save state round trip.
 *
 * Usage: restart_test <frames> <out-prefix> <state-path> <rom-dir> <disk-a> <disk-b>
 *   Boots <disk-a> (V2, 8 MHz, sound board II), <disk-b> (V1S, 4 MHz,
 *   sound board I), then <disk-a> again, each for <frames> frames, writing
 *   each one's last frame as <out-prefix>-<n>.ppm. Each must light pixels and
 *   deliver audio, and <disk-a> must make sound in those frames, so its second
 *   boot shows sound surviving a reboot. Then it saves a state, runs <frames> more frames, loads
 *   the state and runs the same frames again: both runs must end on the same
 *   picture and produce the same audio. Exits non-zero on the first failure.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kairo88_core.h"

/* When set, run() appends the audio it reads here. */
static int16_t *recording;
static long recorded;

static int run(int frames, long *audio_frames, long *nonzero)
{
	int16_t buffer[2048 * 2];
	for (int frame = 0; frame < frames; frame++) {
		if (k88_run_frame() != 0) {
			fprintf(stderr, "QUASI88 quit at frame %d\n", frame);
			return -1;
		}
		int got;
		while ((got = k88_audio_read(buffer, 2048)) > 0) {
			*audio_frames += got;
			if (recording != NULL) {
				memcpy(recording + recorded * 2, buffer, (size_t) got * 2 * sizeof *buffer);
				recorded += got;
			}
			for (int i = 0; i < got * 2; i++) {
				*nonzero += buffer[i] != 0;
			}
		}
	}
	return 0;
}

static int write_ppm(const char *prefix, int n)
{
	int width, height;
	const uint32_t *pixels = k88_frame(&width, &height);
	char path[1024];
	snprintf(path, sizeof path, "%s-%d.ppm", prefix, n);
	FILE *out = fopen(path, "wb");
	if (out == NULL) {
		perror(path);
		return -1;
	}
	fprintf(out, "P6\n%d %d\n255\n", width, height);
	for (int i = 0; i < width * height; i++) {
		unsigned char rgb[3] = { (unsigned char) pixels[i], (unsigned char) (pixels[i] >> 8), (unsigned char) (pixels[i] >> 16) };
		fwrite(rgb, 1, 3, out);
	}
	fclose(out);
	printf("  -> %s\n", path);
	return 0;
}

static uint32_t *copy_frame(size_t *count)
{
	int width, height;
	const uint32_t *pixels = k88_frame(&width, &height);
	*count = (size_t) width * height;
	uint32_t *copy = malloc(*count * sizeof *copy);
	memcpy(copy, pixels, *count * sizeof *copy);
	return copy;
}

int main(int argc, char *argv[])
{
	if (argc != 7) {
		fprintf(stderr, "usage: %s <frames> <out-prefix> <state-path> <rom-dir> <disk-a> <disk-b>\n", argv[0]);
		return 2;
	}
	int frames = atoi(argv[1]);
	const char *prefix = argv[2];
	const char *state = argv[3];
	char *init_argv[] = { argv[0], "-romdir", argv[4], "-notool", "-nostatus" };
	if (k88_init(5, init_argv) != 0) {
		return 1;
	}
	const k88_machine v2 = { K88_BASIC_V2, 1, K88_SOUND_II, K88_PORT_JOYSTICK };
	const k88_machine v1s = { K88_BASIC_V1S, 0, K88_SOUND_I, K88_PORT_JOYSTICK };
	const k88_machine *machines[3] = { &v2, &v1s, &v2 };
	const char *disks[3] = { argv[5], argv[6], argv[5] };
	for (int n = 1; n <= 3; n++) {
		if (k88_boot(machines[n - 1], disks[n - 1], NULL) != 0) {
			return 1;
		}
		long audio_frames = 0, nonzero = 0;
		if (run(frames, &audio_frames, &nonzero) != 0) {
			return 1;
		}
		size_t count;
		uint32_t *pixels = copy_frame(&count);
		long lit = 0;
		for (size_t i = 0; i < count; i++) {
			lit += (pixels[i] & 0xffffff) != 0;
		}
		free(pixels);
		printf("boot %d (%s): %ld lit pixels, %ld audio frames, %ld non-zero samples\n",
			n, disks[n - 1], lit, audio_frames, nonzero);
		if (write_ppm(prefix, n) != 0 || lit == 0 || audio_frames == 0 ||
			(disks[n - 1] == argv[5] && nonzero == 0)) {
			return 1;
		}
	}

	if (k88_state_save(state) != 0) {
		fprintf(stderr, "state save failed\n");
		return 1;
	}
	long audio_frames = 0, nonzero = 0;
	/* The frames' audio at the slowest frame rate, with room to spare. */
	const long capacity = (long) frames * k88_audio_rate() / 40;
	int16_t *first_audio = malloc((size_t) capacity * 2 * sizeof *first_audio);
	int16_t *second_audio = malloc((size_t) capacity * 2 * sizeof *second_audio);
	k88_audio_clear();
	recording = first_audio;
	recorded = 0;
	if (run(frames, &audio_frames, &nonzero) != 0) {
		return 1;
	}
	const long first_recorded = recorded;
	size_t first_count;
	uint32_t *first = copy_frame(&first_count);
	if (k88_state_load(state) != 0) {
		fprintf(stderr, "state load failed\n");
		return 1;
	}
	k88_audio_clear();
	recording = second_audio;
	recorded = 0;
	if (run(frames, &audio_frames, &nonzero) != 0) {
		return 1;
	}
	recording = NULL;
	if (recorded != first_recorded || memcmp(first_audio, second_audio, (size_t) recorded * 2 * sizeof *first_audio) != 0) {
		long at = 0;
		while (at < recorded && at < first_recorded &&
			memcmp(first_audio + at * 2, second_audio + at * 2, 2 * sizeof *first_audio) == 0) {
			at++;
		}
		fprintf(stderr, "the audio after loading the state differs from the audio after saving it "
			"(%ld and %ld frames, first difference at frame %ld)\n", first_recorded, recorded, at);
		return 1;
	}
	size_t second_count;
	uint32_t *second = copy_frame(&second_count);
	if (first_count != second_count || memcmp(first, second, first_count * sizeof *first) != 0) {
		fprintf(stderr, "the frame after loading the state differs from the frame after saving it\n");
		write_ppm(prefix, 4);
		return 1;
	}
	printf("state round trip: picture and %ld audio frames identical after %d frames\n", recorded, frames);
	return 0;
}
