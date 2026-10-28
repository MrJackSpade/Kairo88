/* The Kairo88 core API: QUASI88 started once, then run, reset and fed disks. */

#include <stdio.h>
#include <string.h>

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
#include "memory.h"
#include "keyboard.h"
#include "drive.h"
#include "fdc.h"
#include "fname.h"
#include "crtcdmac.h"
#include "screen.h"
#include "emu.h"

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
	/* A 200-line mode is drawn once per line, on the buffer's even lines; k88_frame hands those
	 * over for the host to repeat, instead of QUASI88 copying every line to the next (Kairo#50). */
	use_interlace = SCREEN_INTERLACE_SKIP;
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

const uint32_t *k88_frame(int *width, int *height, int *stride, int *repeat)
{
	*repeat = screen_drawn_400 ? 1 : 2;
	*width = kairo_frame_width;
	*height = kairo_frame_height / *repeat;
	*stride = kairo_frame_width * *repeat;
	return kairo_frame;
}

double k88_frame_rate(void)
{
	return vsync_freq_hz;
}

void k88_key(int key88, int pressed)
{
	if (key88 == KEY88_CAPS) {
		if (pressed) {
			quasi88_tgl_key_caps();
		}
	} else if (key88 == KEY88_KANA) {
		if (pressed) {
			quasi88_tgl_key_kana();
		}
	} else {
		quasi88_key(key88, pressed);
	}
}

int k88_locks(void)
{
	return (quasi88_now_key_caps() ? 1 : 0) | (quasi88_now_key_kana() ? 2 : 0);
}

void k88_mouse_move(int dx, int dy)
{
	quasi88_mouse_moved_rel(dx, dy);
}

void k88_mouse_button(int button, int pressed)
{
	quasi88_mouse(button == 2 ? KEY88_MOUSE_R : KEY88_MOUSE_L, pressed);
}

const char *k88_disk_path(int unit)
{
	return disk_image_exist(unit) ? file_disk[unit] : "";
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

static char tape_path[1024];

int k88_tape(const char *path)
{
	if (path == NULL) {
		sio_close_tapeload();
		tape_path[0] = '\0';
		return 0;
	}
	if (strlen(path) >= sizeof(tape_path) || !sio_open_tapeload(path)) {
		fprintf(stderr, "kairo88: could not open tape %s\n", path);
		return -1;
	}
	strcpy(tape_path, path);
	return 0;
}

const char *k88_tape_path(void)
{
	return tape_path;
}

int k88_tape_running(void)
{
	long position, end;
	/* OUT 30h bit 3 is the cassette motor relay; at the tape's end QUASI88 reports position 0 of 0. */
	return tape_path[0] && (sys_ctrl & 0x08) && sio_tape_pos(&position, &end) && end > 0;
}

int k88_disk_busy(void)
{
	int active = kairo88_fdc_active;
	kairo88_fdc_active = 0;
	return active;
}

int k88_tape_position(long *read, long *size)
{
	return tape_path[0] && sio_tape_pos(read, size) && *size > 0 ? 0 : -1;
}

int k88_screen_text(char *out, int size)
{
	/* The DMA controller's text channel points at the rows the CRTC shows. In 40-column mode (OUT 30h
	 * bit 0 off) each character is every other byte of the row. */
	const int step = (sys_ctrl & SYS_CTRL_80) ? 1 : 2;
	int written = 0;
	int row, column;
	for (row = 0; row < crtc_sz_lines && written < size - 1; ++row) {
		const byte *line = &main_ram[(word)(text_dma_addr.W + row * crtc_byte_per_line)];
		int end = crtc_sz_columns;
		while (end >= step && (line[end - step] == ' ' || line[end - step] == 0)) end -= step;
		for (column = 0; column < end && written < size - 1; column += step) {
			const byte c = line[column];
			out[written++] = (c >= 0x20 && c < 0x7f) ? (char) c : '?';
		}
		if (written < size - 1) out[written++] = '\n';
	}
	out[written] = '\0';
	return written;
}

int k88_boot(const k88_machine *machine, const char *disk_a, const char *disk_b)
{
	int failed = 0;
	T_RESET_CFG cfg;

	quasi88_disk_eject_all();
	/* ROM files may have changed since the last boot. */
	memory_load_rom();
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
	cpu_timing = machine->sub_cpu;
	mouse_mode = machine->port == K88_PORT_MOUSE ? MOUSE_MOUSE : MOUSE_JOYSTICK;
	quasi88_reset(&cfg);
	/* A boot is a power-on, not a reset: QUASI88's reset keeps memory as the machine's reset button
	 * does, so the last game's RAM and VRAM would be what the next one finds. Memory gets its power-on
	 * contents (sub-system RAM already did, in memory_load_rom) and the keyboard its power-on state. */
	memset(main_vram, 0, sizeof(byte) * 0x4000 * 4);
	if (ext_ram != NULL) {
		memset(ext_ram, 0, sizeof(byte) * 0x8000 * 4 * use_extram);
	}
	power_on_ram_init();
	keyboard_reset();
	/* The key and joystick assignments follow the port's device. */
	keyboard_switch();
	k88_audio_clear();
	return failed;
}

void k88_draw_every(int frames)
{
	quasi88_cfg_set_frameskip_rate(frames);
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

