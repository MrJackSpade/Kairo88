"""Kairo88's local changes to the QUASI88 snapshot in third_party/quasi88 (ymfm and the MAME subset are unchanged), which shared/tools/local_patches.py applies and checks. Recorded from the snapshot as committed; docs/source-import.md describes each change."""

# One group per changed file: (path under third_party/, old text, new text) in order.
PATCHES = {
    'quasi88/src/control.c': [
        ('quasi88/src/control.c',
         '''
#include "emu.h"
#include "drive.h"
#include "event.h"
#include "snddrv.h"
#include "wait.h"
''',
         '''
#include "emu.h"
#include "drive.h"
#include "fdc.h"
#include "event.h"
#include "snddrv.h"
#include "wait.h"
'''),
        ('quasi88/src/control.c',
         '''#include "menu.h"
#include "intr.h"
#include "toolbar.h"


/* リセット時に要求すべき設定 */
''',
         '''#include "menu.h"
#include "intr.h"
#include "toolbar.h"
#include "kairo/state_paths.h"	/* Kairo88 */


/* リセット時に要求すべき設定 */
'''),
        ('quasi88/src/control.c',
         '''	/* ステートロード実行 */
	success = stateload();

	if (now_board != sound_board) {
		/* サウンドボードが変わったら サウンドドライバの再初期化 */
		quasi88_sound_restart(FALSE);
''',
         '''	/* ステートロード実行 */
	success = stateload();

	/* Kairo88: a state records its disks and tapes by path; each reopens where it is now
	 * (kairo/state_paths.h), as a working copy can have another name since the state was saved. */
	if (success) {
		int i;
		char *recorded[] = { file_disk[0], file_disk[1], file_tape[0], file_tape[1] };
		for (i = 0; i < 4; i++) {
			const char *now = kairo_state_path(recorded[i]);
			if (now == recorded[i]) continue;
			if (strlen(now) >= QUASI88_MAX_FILENAME) { success = FALSE; break; }
			strcpy(recorded[i], now);
		}
	}

	if (now_board != sound_board) {
		/* サウンドボードが変わったら サウンドドライバの再初期化 */
		quasi88_sound_restart(FALSE);
'''),
        ('quasi88/src/control.c',
         '''
		/* イメージファイルを全て開く*/
		imagefile_all_open(TRUE);

		pc88main_init(INIT_STATELOAD);
		pc88sub_init(INIT_STATELOAD);
''',
         '''
		/* イメージファイルを全て開く*/
		imagefile_all_open(TRUE);
		drive_stateload_position();	/* Kairo88 */

		pc88main_init(INIT_STATELOAD);
		pc88sub_init(INIT_STATELOAD);
'''),
    ],
    'quasi88/src/osdepend/unix/file-op.c': [
        ('quasi88/src/osdepend/unix/file-op.c',
         '''	struct stat sb;						/* 開いたファイルの状態 */
	int         type;					/* ファイル種別 */
	char        mode[4];				/* 開いた際の、モード */

};

''',
         '''	struct stat sb;						/* 開いたファイルの状態 */
	int         type;					/* ファイル種別 */
	char        mode[4];				/* 開いた際の、モード */
	char        path[1024];				/* Kairo88: the path it was opened by */

};

'''),
        ('quasi88/src/osdepend/unix/file-op.c',
         '''			/* すでに開いているファイルかどうかをチェックする */
			for (i = 0; i < MAX_STREAM; i++) {
				if (osd_stream[i].fp) {
					if (osd_stream[i].sb.st_dev == sb.st_dev &&
						osd_stream[i].sb.st_ino == sb.st_ino) {

						/* DISKの場合かつ同じモードならばそれを返す */
						if (type == FTYPE_DISK                   &&
''',
         '''			/* すでに開いているファイルかどうかをチェックする */
			for (i = 0; i < MAX_STREAM; i++) {
				if (osd_stream[i].fp) {
					/* Kairo88: a system without inode numbers (Windows: st_ino is 0)
					 * names a file by its path, or every disk would be the first. */
					if (osd_stream[i].sb.st_dev == sb.st_dev &&
						osd_stream[i].sb.st_ino == sb.st_ino &&
						(sb.st_ino != 0 || strcmp(osd_stream[i].path, path) == 0)) {

						/* DISKの場合かつ同じモードならばそれを返す */
						if (type == FTYPE_DISK                   &&
'''),
        ('quasi88/src/osdepend/unix/file-op.c',
         '''			st->type = type;
			st->sb   = sb;
			strncpy(st->mode, mode, sizeof(st->mode));
			return st;

		} else {
''',
         '''			st->type = type;
			st->sb   = sb;
			strncpy(st->mode, mode, sizeof(st->mode));
			snprintf(st->path, sizeof(st->path), "%s", path);
			return st;

		} else {
'''),
    ],
    'quasi88/src/pc88/fdc.c': [
        ('quasi88/src/pc88/fdc.c',
         ''' * ステートロード／ステートセーブ
 ************************************************************************/

#define SID				"FDC "
#define SID_DATA		"FDC0"
#define SID2			"FDC2"

static T_SUSPEND_W suspend_fdc_work[] = {
	{ TYPE_STR,   &file_disk[0][0],       },
	{ TYPE_STR,   &file_disk[1][0],       },
	{ TYPE_INT,   &image_disk[0],         },
	{ TYPE_INT,   &image_disk[1],         },
''',
         ''' * ステートロード／ステートセーブ
 ************************************************************************/

#define SID				"FDC "
#define SID_DATA		"FDC0"
#define SID2			"FDC2"
#define SID3			"FDC3"

static T_SUSPEND_W suspend_fdc_work[] = {
	{ TYPE_STR,   &file_disk[0][0],       },
	{ TYPE_STR,   &file_disk[1][0],       },
	{ TYPE_INT,   &image_disk[0],         },
	{ TYPE_INT,   &image_disk[1],         },
'''),
        ('quasi88/src/pc88/fdc.c',
         '''	{ TYPE_CHAR,  &fdc.intr_unit          },

	{ TYPE_END,   0                       },
};


int statesave_fdc(void)
{
	image_disk[0] = drive[0].selected_image;
	image_disk[1] = drive[1].selected_image;

	if (statesave_table(SID, suspend_fdc_work) != STATE_OK) {
		return FALSE;
	}
''',
         '''	{ TYPE_CHAR,  &fdc.intr_unit          },

	{ TYPE_END,   0                       },
};


/* Kairo88: each drive's head position within its track and the sector the FDC
 * last read, which the tables above leave out. A drive reopened after a load
 * starts at sector 0 of its cylinder, so a game that reads sector IDs would see
 * different sectors than it would have without the save. */
static int  saved_track[ NR_DRIVE ], saved_sec[ NR_DRIVE ], saved_sec_nr[ NR_DRIVE ];
static long saved_sec_pos[ NR_DRIVE ], saved_track_top[ NR_DRIVE ];
static int  saved_position = FALSE;
static struct {
	Uchar   c, h, r, n, density, deleted, status;
	int     sec_nr, size, drv;
} saved_sec_buf;

static T_SUSPEND_W suspend_fdc_work3[] = {
	{ TYPE_INT,   &saved_track[0],        },
	{ TYPE_INT,   &saved_track[1],        },
	{ TYPE_INT,   &saved_sec[0],          },
	{ TYPE_INT,   &saved_sec[1],          },
	{ TYPE_INT,   &saved_sec_nr[0],       },
	{ TYPE_INT,   &saved_sec_nr[1],       },
	{ TYPE_LONG,  &saved_sec_pos[0],      },
	{ TYPE_LONG,  &saved_sec_pos[1],      },
	{ TYPE_LONG,  &saved_track_top[0],    },
	{ TYPE_LONG,  &saved_track_top[1],    },

	{ TYPE_BYTE,  &saved_sec_buf.c,             },
	{ TYPE_BYTE,  &saved_sec_buf.h,             },
	{ TYPE_BYTE,  &saved_sec_buf.r,             },
	{ TYPE_BYTE,  &saved_sec_buf.n,             },
	{ TYPE_BYTE,  &saved_sec_buf.density,       },
	{ TYPE_BYTE,  &saved_sec_buf.deleted,       },
	{ TYPE_BYTE,  &saved_sec_buf.status,        },
	{ TYPE_INT,   &saved_sec_buf.sec_nr,        },
	{ TYPE_INT,   &saved_sec_buf.size,          },
	{ TYPE_INT,   &saved_sec_buf.drv,           },
	{ TYPE_END,   0                       },
};

/* Kairo88: puts back the positions a state load read, once the disk images
 * are open again. A state without them keeps the reopened positions. */
void drive_stateload_position(void)
{
	int i;

	if (saved_position == FALSE) {
		return;
	}
	for (i = 0; i < NR_DRIVE; i++) {
		if (drive[ i ].fp) {
			drive[ i ].track     = saved_track[ i ];
			drive[ i ].sec       = saved_sec[ i ];
			drive[ i ].sec_nr    = saved_sec_nr[ i ];
			drive[ i ].sec_pos   = saved_sec_pos[ i ];
			drive[ i ].track_top = saved_track_top[ i ];
		}
	}
	sec_buf.c       = saved_sec_buf.c;
	sec_buf.h       = saved_sec_buf.h;
	sec_buf.r       = saved_sec_buf.r;
	sec_buf.n       = saved_sec_buf.n;
	sec_buf.density = saved_sec_buf.density;
	sec_buf.deleted = saved_sec_buf.deleted;
	sec_buf.status  = saved_sec_buf.status;
	sec_buf.sec_nr  = saved_sec_buf.sec_nr;
	sec_buf.size    = saved_sec_buf.size;
	sec_buf.drv     = saved_sec_buf.drv;
	saved_position = FALSE;
}

int statesave_fdc(void)
{
	int i;

	image_disk[0] = drive[0].selected_image;
	image_disk[1] = drive[1].selected_image;

	if (statesave_table(SID, suspend_fdc_work) != STATE_OK) {
		return FALSE;
	}
'''),
        ('quasi88/src/pc88/fdc.c',
         '''	}

	if (statesave_table(SID2, suspend_fdc_work2) != STATE_OK) {
		return FALSE;
	}

	return TRUE;
}

int stateload_fdc(void)
{
	if (stateload_table(SID, suspend_fdc_work) != STATE_OK) {
''',
         '''	}

	if (statesave_table(SID2, suspend_fdc_work2) != STATE_OK) {
		return FALSE;
	}

	for (i = 0; i < NR_DRIVE; i++) {
		saved_track[ i ]     = drive[ i ].track;
		saved_sec[ i ]       = drive[ i ].sec;
		saved_sec_nr[ i ]    = drive[ i ].sec_nr;
		saved_sec_pos[ i ]   = drive[ i ].sec_pos;
		saved_track_top[ i ] = drive[ i ].track_top;
	}
	saved_sec_buf.c       = sec_buf.c;
	saved_sec_buf.h       = sec_buf.h;
	saved_sec_buf.r       = sec_buf.r;
	saved_sec_buf.n       = sec_buf.n;
	saved_sec_buf.density = sec_buf.density;
	saved_sec_buf.deleted = sec_buf.deleted;
	saved_sec_buf.status  = sec_buf.status;
	saved_sec_buf.sec_nr  = sec_buf.sec_nr;
	saved_sec_buf.size    = sec_buf.size;
	saved_sec_buf.drv     = sec_buf.drv;
	if (statesave_table(SID3, suspend_fdc_work3) != STATE_OK) {
		return FALSE;
	}

	return TRUE;
}

int stateload_fdc(void)
{
	if (stateload_table(SID, suspend_fdc_work) != STATE_OK) {
'''),
        ('quasi88/src/pc88/fdc.c',
         '''		fdc.intr_unit = 4;  /* non exist unit */

		return TRUE;

	}

	return TRUE;
}




''',
         '''		fdc.intr_unit = 4;  /* non exist unit */

		return TRUE;

	}

	saved_position = (stateload_table(SID3, suspend_fdc_work3) == STATE_OK);

	return TRUE;
}




'''),
    ],
    'quasi88/src/pc88/fdc.h': [
        ('quasi88/src/pc88/fdc.h',
         '''byte fdc_read(void);
byte fdc_status(void);
void fdc_TC(void);


void pc88fdc_break_point(void);
''',
         '''byte fdc_read(void);
byte fdc_status(void);
void fdc_TC(void);
void drive_stateload_position(void);


void pc88fdc_break_point(void);
'''),
    ],
    'quasi88/src/pc88/memory.c': [
        ('quasi88/src/pc88/memory.c',
         '''
#define FONT_SZ (8*256*1)

int memory_allocate(void)
{
	int   size;


	/* 標準メモリを確保 */

	mem_alloc_start("Allocating memory for standard ROM/RAM...");
	{
		main_rom    = (byte *)           mem_alloc(sizeof(byte) *  0x8000);
		main_rom_ext = (byte(*)[0x2000]) mem_alloc(sizeof(byte) *  0x2000 * 4);
		main_rom_n  = (byte *)           mem_alloc(sizeof(byte) *  0x8000);
		sub_romram  = (byte *)           mem_alloc(sizeof(byte) *  0x8000);

		main_ram    = (byte *)           mem_alloc(sizeof(byte) * 0x10000);
		main_high_ram = (byte *)         mem_alloc(sizeof(byte) *  0x1000);
		main_vram   = (byte(*)[0x4])     mem_alloc(sizeof(byte) *  0x4000 * 4);

		kanji_rom   = (byte(*)[65536][2])mem_alloc(sizeof(byte) * 2 * 65536 * 2);

		font_pcg    = (byte *)           mem_alloc(sizeof(byte) * 8 * 256 * 2);
		font_mem    = (byte *)           mem_alloc(sizeof(byte) * 8 * 256 * 2);
		font_mem2   = (byte *)           mem_alloc(sizeof(byte) * 8 * 256 * 2);
	}
	if (mem_alloc_finish() == FALSE) {
		return 0;
	}




	/* ROMイメージをファイルから読み込む */

	if (file_compatrom == NULL) {
''',
         '''
#define FONT_SZ (8*256*1)

/* Kairo88: memory_allocate() から分離。確保済みのメモリにROMイメージを読み込む。
 * Reads every ROM image into the memory memory_allocate() allocated, so a
 * host can load ROM files that changed after QUASI88 started; a reset must
 * follow. */
void memory_load_rom(void)
{
	int   size;

	/* ROMイメージをファイルから読み込む */

	if (file_compatrom == NULL) {
'''),
        ('quasi88/src/pc88/memory.c',
         '''	/* ROMのバージョンを保存 */
	rom_version = ROM_VERSION;

	/* オプショナルなメモリを確保 */

	if (memory_allocate_additional() == FALSE) {
''',
         '''	/* ROMのバージョンを保存 */
	rom_version = ROM_VERSION;

	/* 辞書ROMは確保済みなら読み込み直す */
	if (use_jisho_rom && jisho_rom) {
		load_rom(rom_list[ JISHO_ROM ], jisho_rom[0], 0x4000 * 32, DISP_RESULT);
	}
}



int memory_allocate(void)
{
	int   size;


	/* 標準メモリを確保 */

	mem_alloc_start("Allocating memory for standard ROM/RAM...");
	{
		main_rom    = (byte *)           mem_alloc(sizeof(byte) *  0x8000);
		main_rom_ext = (byte(*)[0x2000]) mem_alloc(sizeof(byte) *  0x2000 * 4);
		main_rom_n  = (byte *)           mem_alloc(sizeof(byte) *  0x8000);
		sub_romram  = (byte *)           mem_alloc(sizeof(byte) *  0x8000);

		main_ram    = (byte *)           mem_alloc(sizeof(byte) * 0x10000);
		main_high_ram = (byte *)         mem_alloc(sizeof(byte) *  0x1000);
		main_vram   = (byte(*)[0x4])     mem_alloc(sizeof(byte) *  0x4000 * 4);

		kanji_rom   = (byte(*)[65536][2])mem_alloc(sizeof(byte) * 2 * 65536 * 2);

		font_pcg    = (byte *)           mem_alloc(sizeof(byte) * 8 * 256 * 2);
		font_mem    = (byte *)           mem_alloc(sizeof(byte) * 8 * 256 * 2);
		font_mem2   = (byte *)           mem_alloc(sizeof(byte) * 8 * 256 * 2);
	}
	if (mem_alloc_finish() == FALSE) {
		return 0;
	}




	/* Kairo88: ROMイメージの読み込みは memory_load_rom() に分離 */
	memory_load_rom();

	/* オプショナルなメモリを確保 */

	if (memory_allocate_additional() == FALSE) {
'''),
    ],
    'quasi88/src/pc88/memory.h': [
        ('quasi88/src/pc88/memory.h',
         '''

int  memory_allocate(void);
void memory_free(void);

void memory_reset_font(void);
''',
         '''

int  memory_allocate(void);
void memory_load_rom(void);	/* Kairo88 */
void memory_free(void);

void memory_reset_font(void);
'''),
    ],
    'quasi88/src/pc88/pc88sub.c': [
        ('quasi88/src/pc88/pc88sub.c',
         ''' * 割り込みを生成する。と同時に、次の割り込みまでの、最小 state も計算
 *      帰り値は、Z80処理強制終了のフラグ(TRUE/FALSE)
 *----------------------------------------------------------------------*/
void sub_INT_update(void)
{
	static int sub_total_state = 0;		/* サブCPUが処理した命令数 */
	int icount;

	icount = fdc_ctrl(z80sub_cpu.state0);
''',
         ''' * 割り込みを生成する。と同時に、次の割り込みまでの、最小 state も計算
 *      帰り値は、Z80処理強制終了のフラグ(TRUE/FALSE)
 *----------------------------------------------------------------------*/
/* Kairo88: at file scope so state files save it; it decides when the sub CPU
 * next breaks off for a frame update. */
static int sub_total_state = 0;		/* サブCPUが処理した命令数 */

void sub_INT_update(void)
{
	int icount;

	icount = fdc_ctrl(z80sub_cpu.state0);
'''),
        ('quasi88/src/pc88/pc88sub.c',
         ''' ************************************************************************/

#define SID		"SUB "

static T_SUSPEND_W suspend_pc88sub_work[] = {
	{ TYPE_PAIR,  &z80sub_cpu.AF,         },
''',
         ''' ************************************************************************/

#define SID		"SUB "
#define SID2	"SUB2"

static T_SUSPEND_W suspend_pc88sub_work[] = {
	{ TYPE_PAIR,  &z80sub_cpu.AF,         },
'''),
        ('quasi88/src/pc88/pc88sub.c',
         '''};


int statesave_pc88sub(void)
{
	if (statesave_table(SID, suspend_pc88sub_work) == STATE_OK) {
		return TRUE;
	} else {
		return FALSE;
''',
         '''};


/* Kairo88: a section of its own, so state files without it still load. */
static T_SUSPEND_W suspend_pc88sub_work2[] = {
	{ TYPE_INT,   &sub_total_state,       },
	{ TYPE_END,   0                       },
};

int statesave_pc88sub(void)
{
	if (statesave_table(SID, suspend_pc88sub_work) == STATE_OK &&
		statesave_table(SID2, suspend_pc88sub_work2) == STATE_OK) {
		return TRUE;
	} else {
		return FALSE;
'''),
        ('quasi88/src/pc88/pc88sub.c',
         '''int stateload_pc88sub(void)
{
	if (stateload_table(SID, suspend_pc88sub_work) == STATE_OK) {
		return TRUE;
	} else {
		return FALSE;
''',
         '''int stateload_pc88sub(void)
{
	if (stateload_table(SID, suspend_pc88sub_work) == STATE_OK) {
		if (stateload_table(SID2, suspend_pc88sub_work2) != STATE_OK) {
			sub_total_state = 0;
		}
		return TRUE;
	} else {
		return FALSE;
'''),
    ],
    'quasi88/src/quasi88.c': [
        ('quasi88/src/quasi88.c',
         '''
#include "emu.h"
#include "drive.h"
#include "event.h"
#include "keyboard.h"
#include "monitor.h"
''',
         '''
#include "emu.h"
#include "drive.h"
#include "fdc.h"
#include "event.h"
#include "keyboard.h"
#include "monitor.h"
'''),
        ('quasi88/src/quasi88.c',
         '''
						set_signal();						/* INTシグナルの処理を設定 */
						imagefile_all_open(resume_flag);	/* イメージファイルを全て開く */

						/* エミュ用ワークを順次初期化   */
						pc88main_cpu_init();
''',
         '''
						set_signal();						/* INTシグナルの処理を設定 */
						imagefile_all_open(resume_flag);	/* イメージファイルを全て開く */
						if (resume_flag) {	/* Kairo88 */
							drive_stateload_position();
						}

						/* エミュ用ワークを順次初期化   */
						pc88main_cpu_init();
'''),
    ],
    'quasi88/src/screen/putimage.c': [
        ('quasi88/src/screen/putimage.c',
         ''' *      この関数は、 bpp ・ サイズ ・ エフェクト の変更時に呼び出す。
 *----------------------------------------------------------------------*/
static  int (*vram2screen_list[4][4][2])(T_RECTANGLE *updated_area);
static  void (*screen_buf_init_p)(void);

static  int (*menu2screen_p)(T_RECTANGLE *updated_area);
''',
         ''' *      この関数は、 bpp ・ サイズ ・ エフェクト の変更時に呼び出す。
 *----------------------------------------------------------------------*/
static  int (*vram2screen_list[4][4][2])(T_RECTANGLE *updated_area);
/* Kairo88: whether the last VRAM transfer drew 400 lines (V_HIRESO); otherwise
   it drew 200, which the host reads from every other buffer line with -skipline. */
int screen_drawn_400 = FALSE;
static  void (*screen_buf_init_p)(void);

static  int (*menu2screen_p)(T_RECTANGLE *updated_area);
'''),
        ('quasi88/src/screen/putimage.c',
         '''	}
#endif

	return (vram2screen_list[ vram_mode ][ text_mode ][ method ])(updated_area);
}

''',
         '''	}
#endif

	screen_drawn_400 = (vram_mode == V_HIRESO);	/* Kairo88 */
	return (vram2screen_list[ vram_mode ][ text_mode ][ method ])(updated_area);
}

'''),
    ],
    'quasi88/src/screen/screen.h': [
        ('quasi88/src/screen/screen.h',
         '''	SCREEN_INTERLACE_YES = 1,				/* インターレス表示する */
	SCREEN_INTERLACE_SKIP = -1				/* 1ラインおき表示する */
};
extern int use_interlace;					/* インターレース表示 */

extern int use_half_interp;					/* 画面サイズ半分時、色補間する */
''',
         '''	SCREEN_INTERLACE_YES = 1,				/* インターレス表示する */
	SCREEN_INTERLACE_SKIP = -1				/* 1ラインおき表示する */
};
extern int screen_drawn_400;				/* Kairo88: last VRAM transfer drew 400 lines */
extern int use_interlace;					/* インターレース表示 */

extern int use_half_interp;					/* 画面サイズ半分時、色補間する */
'''),
    ],
}
