/* The Kairo88 core API: QUASI88 started once, then run, reset and fed disks. */

#include <stdio.h>

#include "kairo88_core.h"

#include "quasi88.h"
#include "getconf.h"
#include "event.h"
#include "device.h"
#include "intr.h"
#include "suspend.h"
#include "initval.h"
#include "pc88main.h"
#include "sound_state.h"

static int started = 0;

int k88_init(int argc, char *argv[])
{
	if (started) {
		fprintf(stderr, "kairo88: k88_init called twice\n");
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

double k88_frame_rate(void)
{
	return vsync_freq_hz;
}

void k88_key(int key88, int pressed)
{
	quasi88_key(key88, pressed);
}

void k88_pad(int key88, int pressed)
{
	quasi88_pad(key88, pressed);
}

int k88_disk(int drive, const char *path)
{
	if (path == NULL) {
		quasi88_disk_eject(drive);
		return 0;
	}
	return quasi88_disk_insert(drive, path, 0, FALSE) ? 0 : -1;
}

int k88_boot(const k88_machine *machine, const char *disk_a, const char *disk_b)
{
	int failed = 0;
	T_RESET_CFG cfg;

	quasi88_disk_eject_all();
	if (disk_a != NULL && !quasi88_disk_insert(DRIVE_1, disk_a, 0, FALSE)) {
		fprintf(stderr, "kairo88: could not open %s\n", disk_a);
		failed = -1;
	}
	if (disk_b != NULL && !quasi88_disk_insert(DRIVE_2, disk_b, 0, FALSE)) {
		fprintf(stderr, "kairo88: could not open %s\n", disk_b);
		failed = -1;
	}
	quasi88_get_reset_cfg(&cfg);
	cfg.boot_basic = machine->basic;
	cfg.boot_clock_4mhz = machine->clock_8mhz ? FALSE : TRUE;
	cfg.sound_board = machine->sound == K88_SOUND_II ? SOUND_II : SOUND_I;
	cfg.boot_from_rom = disk_a == NULL ? TRUE : FALSE;
	cpu_clock_mhz = machine->clock_8mhz ? CONST_8MHZ_CLOCK : CONST_4MHZ_CLOCK;
	quasi88_reset(&cfg);
	k88_audio_clear();
	return failed;
}

void k88_reset(void)
{
	quasi88_reset(NULL);
}

int k88_state_save(const char *path)
{
	filename_set_state(path);
	return quasi88_statesave(-1) ? 0 : -1;
}

int k88_state_load(const char *path)
{
	int loaded;

	filename_set_state(path);
	kairo_sound_load_begin();
	loaded = quasi88_stateload(-1);
	if (kairo_sound_load_end() != 0) {
		return -1;
	}
	return loaded ? 0 : -1;
}

