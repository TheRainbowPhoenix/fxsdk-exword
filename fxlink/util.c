#include "util.h"
#include <time.h>
#include <errno.h>

delay_t delay_none(void)
{
	return 0;
}
delay_t delay_seconds(int seconds)
{
	return seconds * 4;
}
delay_t delay_infinite(void)
{
	return -1;
}

bool delay_cycle(delay_t *delay)
{
	if(*delay == 0) return true;

	struct timespec spec = { .tv_sec=0, .tv_nsec=250000000 };
	int rc;

	/* Account for interrupts in the nanosleep(2) call */
	struct timespec req = spec;
	do rc = nanosleep(&req, &req);
	while(rc == -1 && errno == EINTR);

	if(*delay > 0) (*delay)--;
	return false;
}
