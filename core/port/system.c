/* QUASI88 hooks that each system port supplies. Kairo88 adds nothing to state
 * files and shows no port-specific text in QUASI88's own menu. */

#include "quasi88.h"
#include "suspend.h"
#include "menu.h"

int stateload_system(void)
{
	return TRUE;
}

int statesave_system(void)
{
	return TRUE;
}

int menu_about_osd_msg(int req_japanese, int *result_code, const char *message[])
{
	(void) req_japanese;
	(void) result_code;
	(void) message;
	return FALSE;
}
