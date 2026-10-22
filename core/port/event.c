/* QUASI88 event layer for Kairo88. Input arrives through the core API
 * (kairo88_core.c), which calls quasi88_key() and friends directly, so the
 * per-frame event pump has nothing to poll. */

#include "quasi88.h"
#include "device.h"
#include "event.h"

void event_init(void)
{
}

void event_update(void)
{
}

void event_exit(void)
{
}

void event_get_mouse_pos(int *x, int *y)
{
	*x = 0;
	*y = 0;
}

int event_keylayout_change(void)
{
	return FALSE;
}

void event_keylayout_revert(void)
{
}

void event_switch(void)
{
}

int event_get_joystick_num(void)
{
	return 0;
}
