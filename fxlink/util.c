#include "util.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>

char *gen_file_name(char const *path, char const *name, char const *suffix)
{
	char *filename = NULL;
	int counter = 1;

	time_t time_raw;
	struct tm time_bd;
	time(&time_raw);
	localtime_r(&time_raw, &time_bd);

	while(1) {
		asprintf(&filename, "%s/fxlink-%.16s-%04d.%02d.%02d-%02dh%02d-%d.%s",
			path, name, time_bd.tm_year + 1900, time_bd.tm_mon + 1,
			time_bd.tm_mday, time_bd.tm_hour, time_bd.tm_min, counter, suffix);
		if(!filename) continue;

		/* Try to find a name for a file that doesn't exist */
		if(access(filename, F_OK) == -1) break;

		free(filename);
		counter++;
	}

	return filename;
}

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
