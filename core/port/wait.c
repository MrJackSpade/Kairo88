/* QUASI88 wait layer for Kairo88. The host paces frames (display refresh and
 * audio), so QUASI88 never sleeps: every vsync reports that it is already due. */

#include "quasi88.h"
#include "device.h"
#include "wait.h"

int wait_vsync_init(void)
{
	return TRUE;
}

void wait_vsync_exit(void)
{
}

void wait_vsync_setup(long vsync_cycle_us, int do_sleep)
{
	(void) vsync_cycle_us;
	(void) do_sleep;
}

int wait_vsync_update(void)
{
	return WAIT_JUST;
}

void wait_get_current_time(union wait_time *time)
{
	(void) time;
}

int wait_calc_elasped_time_ms(const union wait_time *got_time)
{
	(void) got_time;
	return (int) (1000 / 55.4);
}
