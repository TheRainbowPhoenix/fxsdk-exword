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

/* Main functions for each mdoe */
int main_list(filter_t *filter, delay_t *delay, libusb_context *context);
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
"the UDisks2 library (to mount and use Mass Storage devics).\n"
"\n"
"Operating modes:\n"
"  -l, --list          List detected calculators on the USB ports (libusb)\n"
"  -b, --blocks        List detected Mass Storage filesystems (udisks2)\n"
"  -s, --send          Send a file to a Mass Storage calculator (udisks2)\n"
"  --test              Communication tests by Lephe (libusb) [WIP!]\n"
"\n"
"General options:\n"
"  -w DELAY            Wait up to this many seconds for a calculator to\n"
"                      connect. If DELAY is unspecified, wait indefinitely.\n"
"  -f FILTER           Filter which calculators can be detected and used\n"
"  --libusb-log=LEVEL  libusb log level: NONE, ERROR, WARNING, INFO, DEBUG\n"
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

int main(int argc, char **argv)
{
	int rc=1, mode=0, error=0, option=0, loglevel=LIBUSB_LOG_LEVEL_ERROR;
	delay_t delay = delay_seconds(0);
	filter_t *filter = NULL;

	//---
	// Command-line argument parsing
	//---

	enum { TEST=1, LIBUSB_LOG };
	const struct option longs[] = {
		{ "help",       no_argument,       NULL,       'h' },
		{ "list",       no_argument,       NULL,       'l' },
		{ "blocks",     no_argument,       NULL,       'b' },
		{ "send",       no_argument,       NULL,       's' },
		{ "test",       no_argument,       NULL,      TEST },
		{ "libusb-log", required_argument, NULL, LIBUSB_LOG },
	};

	while(option >= 0 && option != '?')
	switch((option = getopt_long(argc, argv, "hlbsf:w::", longs, NULL)))
	{
	case 'h':
		fprintf(stderr, help_string, argv[0]);
		return 0;
	case 'l':
	case 'b':
	case 's':
	case TEST:
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
	if(mode == 'l' || mode == TEST) {
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
	else if(mode == TEST) {
		libusb_device *dev = NULL;
		int rc = usb_unique_wait(filter, &delay, context, &dev);

		if(rc == FILTER_NONE)
			printf("No device found.\n");
		else if(rc == FILTER_MULTIPLE)
			printf("Multiple devices found, ambiguous!\n");
		else if(rc == FILTER_UNIQUE) {
			rc = main_test(dev, context);
			libusb_unref_device(dev);
		}
	}

	if(context) libusb_exit(context);
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
// WIP tests
//---

int main_test(libusb_device *dev, libusb_context *context)
{
	libusb_device_handle *dh;
	int rc;
	(void)context;

	if((rc = libusb_open(dev, &dh)))
		return libusb_err(rc, "cannot open device %s", usb_id(dev));

	printf("driver active: %d\n", libusb_kernel_driver_active(dh, 0));

	if((rc = libusb_claim_interface(dh, 0))) {
		libusb_close(dh);
		return libusb_err(rc, "cannot claim interface on %s", usb_id(dev));
	}

	uint8_t buffer[512];
	int transferred;

	rc = libusb_bulk_transfer(dh, 0x81, buffer, 512, &transferred, 2000);

	if(rc)
		rc = libusb_err(rc, "cannot perform bulk transfer on %s", usb_id(dev));
	else {
		printf("Got some data!\n");
		fwrite(buffer, 1, transferred, stdout);
		rc = 0;
	}

	libusb_release_interface(dh, 0);
	libusb_close(dh);
	return rc;
}

/* libudev tests, work but not useful yet */
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
