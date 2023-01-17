#include "config.h"
#include "fxlink.h"
#include "util.h"
#include "properties.h"
#include "filter.h"
#include "usb.h"

#include <libusb.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

int main_test(libusb_device *device, libusb_context *context);

static const char *help_string =
"usage: %1$s -l [options...]\n"
"       %1$s -b [options...]\n"
"       %1$s -s [options...] <FILES...>\n"
"       %1$s --test\n"
"\n"
"fxlink interacts with CASIO calculators of the fx-9860G and fx-CG 50 series\n"
"over the USB port, through mass storage and custom USB protocols. Depending\n"
"on the mode, fxlink uses libusb (for discovery and USB communication)or\n"
"the UDisks2 library (to mount and use Mass Storage devices).\n"
"\n"
"Operating modes:\n"
"  -l, --list          List detected calculators on the USB ports (libusb)\n"
"  -b, --blocks        List detected Mass Storage filesystems (udisks2)\n"
"  -s, --send          Send a file to a Mass Storage calculator (udisks2)\n"
"  -i, --interactive   Interactive messaging with a gint add-in (libusb)\n"
"  -p, --push          Push a .bin file to the Add-In Push app (libusb)\n"
"\n"
"General options:\n"
"  -w DELAY            Wait up to this many seconds for a calculator to\n"
"                      connect. If DELAY is unspecified, wait indefinitely.\n"
"  -f FILTER           Filter which calculators can be detected and used\n"
"  --libusb-log=LEVEL  libusb log level: NONE, ERROR, WARNING, INFO, DEBUG\n"
"  --fxlink-log[=FILE] Log fxlink text messages into FILE (append mode). If\n"
"                      FILE is missing, a timestamp-name will be generated.\n"
"  -q, --quiet         Quite mode (minimum verbosity)\n"
"  -u, --unmount       In -s mode, always unmount the disk, even if it was\n"
"                      mounted by someone else\n"
"  -r, --repeat        In interactive mode, reconnect infinitely if the\n"
"                      calculator disconnects (implies -w)\n"
"\n"
"Device filters:\n"
"  A device filter is a comma-separated list of properties that a device has\n"
"  to match in order to be listed or used, such as 'p7,serial=00000001'.\n"
"  Several filters can be separated with a semicolon, in which case a device\n"
"  will be considered as long as it matches one of the filters. For example,\n"
"  'p7 ; mass_storage,serial=IGQcGRe9'.\n"
"\n"
"  The following properties are defined; the libraries in which each can be\n"
"  detected and used is indicated in brackets.\n"
"    p7                Matches Protocol 7 calculators (all the FX models\n"
"                      except the G-III). [libusb, udisks2]\n"
"    mass_storage      Matches Mass Storage calculators (the CG series and\n"
"                      the G-III). [libusb, udisks2]\n"
"    series_cg         Matches CG-series calculators. [udisks2]\n"
"    series_g3         Matches G-III series calculators. [udisks2]\n"
"    serial_number=ID  Matches this specific serial number. Requires write\n"
"                      access to the device in libusb. [libusb, udisks2]\n";

/* Global options */
struct fxlink_options options;

int main(int argc, char **argv)
{
	int rc=1, mode=0, error=0, option=0, loglevel=LIBUSB_LOG_LEVEL_ERROR;
	delay_t delay = delay_seconds(0);
	filter_t *filter = NULL;
	bool repeat = false;

	options.quiet = false;
	options.force_unmount = false;
	options.log_file = NULL;

	//---
	// Command-line argument parsing
	//---

	enum { LIBUSB_LOG=1, LOG_TO_FILE=2 };
	const struct option longs[] = {
		{ "help",        no_argument,       NULL, 'h' },
		{ "list",        no_argument,       NULL, 'l' },
		{ "blocks",      no_argument,       NULL, 'b' },
		{ "send",        no_argument,       NULL, 's' },
		{ "interactive", no_argument,       NULL, 'i' },
		{ "push",        no_argument,       NULL, 'p' },
		{ "libusb-log",  required_argument, NULL, LIBUSB_LOG },
		{ "quiet",       no_argument,       NULL, 'q' },
		{ "fxlink-log",  optional_argument, NULL, LOG_TO_FILE },
		{ "unmount",     no_argument,       NULL, 'u' },
		{ "repeat",      no_argument,       NULL, 'r' },
	};

	while(option >= 0 && option != '?')
	switch((option = getopt_long(argc, argv, "hlbsipquf:w::r", longs, NULL)))
	{
	case 'h':
		fprintf(stderr, help_string, argv[0]);
		return 0;
	case 'l':
	case 'b':
	case 's':
	case 'i':
	case 'p':
		mode = option;
		break;
	case LIBUSB_LOG:
		if(!strcmp(optarg, "NONE"))
			loglevel = LIBUSB_LOG_LEVEL_NONE;
		else if(!strcmp(optarg, "ERROR"))
			loglevel = LIBUSB_LOG_LEVEL_ERROR;
		else if(!strcmp(optarg, "WARNING"))
			loglevel = LIBUSB_LOG_LEVEL_WARNING;
		else if(!strcmp(optarg, "INFO"))
			loglevel = LIBUSB_LOG_LEVEL_INFO;
		else if(!strcmp(optarg, "DEBUG"))
			loglevel = LIBUSB_LOG_LEVEL_DEBUG;
		else fprintf(stderr, "warning: ignoring log level '%s'; should be "
				"NONE, ERROR, WARNING, INFO or DEBUG\n", optarg);
		break;
	case 'q':
		options.quiet = true;
		break;
	case 'u':
		options.force_unmount = true;
		break;
	case 'r':
		repeat = true;
		delay = delay_infinite();
		break;
	case LOG_TO_FILE:
		if(optarg)
			options.log_file = fopen(optarg, "a");
		else {
			char *name = gen_file_name(".", "logfile", "log");
			printf("--fxlink-log will output in '%s'\n", name);
			options.log_file = fopen(name, "a");
			free(name);
		}
		break;
	case 'w':
		if(!optarg) {
			delay = delay_infinite();
			break;
		}
		char *end;
		int seconds = strtol(optarg, &end, 10);
		if(seconds < 0 || *end != 0) {
			error = err("invalid delay '%s'\n", optarg);
			break;
		}
		delay = delay_seconds(seconds);
		break;
	case 'f':
		filter = filter_parse(optarg);
		break;
	case '?':
		error = 1;
	}

	if(mode == 's' && optind == argc)
		error = err("send mode requires additional arguments (file names)");

	if(mode == 'p' && optind == argc)
		error = err("push mode requires a file name");
	if(mode == 'p' && optind < argc-1)
		error = err("push mode only accepts one file name");

	/* No arguments or bad arguments */
	if(error)
		return 1;
	if(!mode) {
		fprintf(stderr, help_string, argv[0]);
		return 1;
	}

	//---
	// libusb initialization
	//---

	libusb_context *context = NULL;

	/* Initialize libusb for corresponding modes */
	if(mode == 'l' || mode == 'i' || mode == 'p') {
		if((rc = libusb_init(&context)))
			return libusb_err(rc, "error initializing libusb");
		libusb_set_option(context, LIBUSB_OPTION_LOG_LEVEL, loglevel);
	}

	//---
	// Main functions
	//---

	if(mode == 'l') {
		rc = main_list(filter, &delay, context);
	}
	else if(mode == 'b') {
		#ifndef FXLINK_DISABLE_UDISKS2
		rc = main_blocks(filter, &delay);
		#else
		rc = err("this fxlink was built without UDisks2; -b is disabled");
		#endif
	}
	else if(mode == 's') {
		#ifndef FXLINK_DISABLE_UDISKS2
		rc = main_send(filter, &delay, argv + optind);
		#else
		rc = err("this fxlink was built without UDisks2; -s is disabled");
		#endif
	}
	else if(mode == 'i') {
		do {
			rc = main_interactive(filter, &delay, context);
		}
		while(repeat);
	}
	else if(mode == 'p') {
		rc = main_push(filter, &delay, context, argv + optind);
	}

	if(context)
		libusb_exit(context);
	if(options.log_file)
		fclose(options.log_file);
	return rc;
}

//---
// Device list
//---

int main_list(filter_t *filter, delay_t *delay, libusb_context *context)
{
	/* Wait for a device to be connected */
	filter_clean_libusb(filter);
	usb_unique_wait(filter, delay, context, NULL);

	int total_devices = 0;
	bool error;

	for_libusb_devices(it, context, &error) {
		if(!filter_match(&it.props, filter)) continue;

		if(total_devices > 0) printf("\n");

		if(it.dc.idProduct == 0x6101)
			printf("fx-9860G series (Protocol 7) calculator\n");
		else if(it.dc.idProduct == 0x6102)
			printf("fx-CG or G-III series (USB Mass Storage) calculator\n");
		else
			printf("Unknown calculator (idProduct: %04x)\n", it.dc.idProduct);

		printf("     Device location:  Bus %d, Port %d, Device %d\n",
			libusb_get_bus_number(it.dev),
			libusb_get_port_number(it.dev),
			libusb_get_device_address(it.dev));
		printf("      Identification:  idVendor: %04x, idProduct: %04x\n",
			it.dc.idVendor, it.dc.idProduct);
		/* FIXME: This assumes a short path (no hub or dual-device) */
		printf("  Guessed sysfs path:  /sys/bus/usb/devices/%d-%d/\n",
			libusb_get_bus_number(it.dev),
			libusb_get_port_number(it.dev));

		char *serial = it.dh ? usb_serial_number(it.dh) : NULL;
		if(serial)
			printf("       Serial number:  %s\n", serial);
		free(serial);

		printf("          Properties:  ");
		properties_print(stdout, &it.props);
		printf("\n");

		total_devices++;
	}

	if(!error && !total_devices)
		printf("No%s device found.\n", filter ? " matching" : "");
	return 0;
}

//---
// libudev tests; work but not useful yet
//---

#if 0
#include <libudev.h>
int main_udev_test(libusb_device *dev)
{
	struct udev *udev = NULL;
	struct udev_device *udev_device = NULL;

	udev = udev_new();
	if(!udev) return err("cannot create udev context");

	static char sys_path[128];
	sprintf(sys_path, "/sys/bus/usb/devices/%d-%d",
		libusb_get_bus_number(dev),
		libusb_get_port_number(dev));

	udev_device = udev_device_new_from_syspath(udev, sys_path);
	if(!udev_device) {
		udev_unref(udev);
		return err("cannot get udev device for %s", sys_path);
	}

	printf("Device number: %ld\n", udev_device_get_devnum(udev_device));
	printf("Device devnode: %s\n", udev_device_get_devnode(udev_device));

	if(udev) udev_unref(udev);
	return 0;
}
#endif
