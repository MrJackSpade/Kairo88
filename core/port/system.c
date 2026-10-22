/* QUASI88 hooks that each system port supplies. Kairo88 adds the sound
 * driver's state to state files and shows no port-specific text in QUASI88's
 * own menu. */

#include "quasi88.h"
#include "suspend.h"
#include "menu.h"
#include "sound_state.h"

/* The sound state's length, then the state: QUASI88 blocks have fixed sizes. */
#define SID_SIZE	"KSNL"
#define SID_SOUND	"KSND"

int stateload_system(void)
{
	int size;
	int result = stateload_block(SID_SIZE, &size, sizeof(size));

	if (result == STATE_ERR_ID) {
		/* A state without it: QUASI88's register replay is all there is. */
		return TRUE;
	}
	if (result != STATE_OK || size <= 0) {
		return FALSE;
	}
	return stateload_block(SID_SOUND, kairo_sound_load_buffer(size), size) == STATE_OK;
}

int statesave_system(void)
{
	int size;
	const uint8_t *state = kairo_sound_state(&size);

	return statesave_block(SID_SIZE, &size, sizeof(size)) == STATE_OK &&
		statesave_block(SID_SOUND, (void *) state, size) == STATE_OK;
}

int menu_about_osd_msg(int req_japanese, int *result_code, const char *message[])
{
	(void) req_japanese;
	(void) result_code;
	(void) message;
	return FALSE;
}
