/* Boots the Kairo88 core headlessly and writes frames as binary PPM files.
 *
 * Usage: boot_test <frames> <every> <out-prefix> -- <QUASI88 options...>
 *   Runs <frames> frames and writes <out-prefix>-NNNNN.ppm every <every> frames
 *   and after the last one, and all audio to <out-prefix>.wav. Everything after
 *   "--" is passed to QUASI88, e.g.
 *   boot_test 600 120 out/game -- -romdir bios game.d88
 *   It reports how many frames the floppy controller was busy (k88_disk_busy), which the app runs
 *   flat out.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kairo88_core.h"
#include "picture.h"

static FILE *wav;
static uint32_t wav_frames;

static void put_le(FILE *out, uint32_t value, int bytes)
{
	for (int i = 0; i < bytes; i++) {
		fputc((int) (value >> (8 * i)) & 0xff, out);
	}
}

/* Writes (or rewrites) the 44-byte header of a 16-bit stereo PCM WAV. */
static void wav_header(FILE *out, uint32_t frames, int rate)
{
	fseek(out, 0, SEEK_SET);
	fwrite("RIFF", 1, 4, out);
	put_le(out, 36 + frames * 4, 4);
	fwrite("WAVEfmt ", 1, 8, out);
	put_le(out, 16, 4);
	put_le(out, 1, 2);
	put_le(out, 2, 2);
	put_le(out, (uint32_t) rate, 4);
	put_le(out, (uint32_t) rate * 4, 4);
	put_le(out, 4, 2);
	put_le(out, 16, 2);
	fwrite("data", 1, 4, out);
	put_le(out, frames * 4, 4);
	fseek(out, 0, SEEK_END);
}

static void drain_audio(void)
{
	int16_t buffer[2048 * 2];
	int got;
	while ((got = k88_audio_read(buffer, 2048)) > 0) {
		fwrite(buffer, sizeof(int16_t) * 2, (size_t) got, wav);
		wav_frames += (uint32_t) got;
	}
}

static int write_ppm(const char *prefix, int frame)
{
	int width, height;
	const uint32_t *pixels = k88_picture(&width, &height);
	char path[1024];
	snprintf(path, sizeof path, "%s-%05d.ppm", prefix, frame);
	FILE *out = fopen(path, "wb");
	if (out == NULL) {
		perror(path);
		return -1;
	}
	fprintf(out, "P6\n%d %d\n255\n", width, height);
	for (int i = 0; i < width * height; i++) {
		unsigned char rgb[3] = {
			(unsigned char) pixels[i], (unsigned char) (pixels[i] >> 8), (unsigned char) (pixels[i] >> 16)
		};
		fwrite(rgb, 1, 3, out);
	}
	if (fclose(out) != 0) {
		perror(path);
		return -1;
	}
	printf("%s\n", path);
	return 0;
}

int main(int argc, char *argv[])
{
	int split = 0;
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--") == 0) {
			split = i;
			break;
		}
	}
	if (split != 4) {
		fprintf(stderr, "usage: %s <frames> <every> <out-prefix> -- <QUASI88 options...>\n", argv[0]);
		return 2;
	}
	int frames = atoi(argv[1]);
	int every = atoi(argv[2]);
	const char *prefix = argv[3];

	argv[split] = argv[0];
	if (k88_init(argc - split, argv + split) != 0) {
		return 1;
	}
	char wav_path[1024];
	snprintf(wav_path, sizeof wav_path, "%s.wav", prefix);
	wav = fopen(wav_path, "wb");
	if (wav == NULL) {
		perror(wav_path);
		return 1;
	}
	wav_header(wav, 0, k88_audio_rate());
	int busy = 0, last_busy = 0;
	for (int frame = 1; frame <= frames; frame++) {
		if (k88_disk_busy()) {
			busy++;
			last_busy = frame;
		}
		if (k88_run_frame() != 0) {
			fprintf(stderr, "QUASI88 quit at frame %d\n", frame);
			return 1;
		}
		drain_audio();
		if ((every > 0 && frame % every == 0) || frame == frames) {
			if (write_ppm(prefix, frame) != 0) {
				return 1;
			}
		}
	}
	wav_header(wav, wav_frames, k88_audio_rate());
	if (fclose(wav) != 0) {
		perror(wav_path);
		return 1;
	}
	printf("%s (%u frames)\n", wav_path, wav_frames);
	printf("disk busy in %d of %d frames, the last at frame %d\n", busy, frames, last_busy);
	return 0;
}
