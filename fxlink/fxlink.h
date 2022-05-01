//---
// fxlink:fxlink - Application logic
//---

#ifndef FXLINK_FXLINK_H
#define FXLINK_FXLINK_H

#include <libusb.h>
#include "filter.h"
#include "util.h"

struct fxlink_options
{
	bool quiet;
	bool force_unmount;
	FILE *log_file;
};

extern struct fxlink_options options;

/* Main function for -l */
int main_list(filter_t *filter, delay_t *delay, libusb_context *context);

/* Main function for -b */
int main_blocks(filter_t *filter, delay_t *delay);

/* Main function for -s */
int main_send(filter_t *filter, delay_t *delay, char **files);

/* Main function for -i */
int main_interactive(filter_t *filter,delay_t *delay,libusb_context *context);

#endif /* FXLINK_FXLINK_H */
