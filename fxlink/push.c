#include "config.h"
#include "fxlink.h"
#include "util.h"
#include "properties.h"
#include "filter.h"
#include "usb.h"

#include <libusb.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int main_push(filter_t *filter, delay_t *delay, libusb_context *context, char** files)
{
	int rc = 1;
	libusb_device *dev = NULL;
	libusb_device_handle *dh = NULL;

	FILE *fp = fopen(files[0], "rb");
	if (!fp) {
		printf("error: Unable to open file %s\n", files[0]);
		goto end;
	}

	fseek(fp, 0, SEEK_END);
	long fsize = ftell(fp);
	// If more than 6MB, abort
	if (fsize > 6 * 1024 * 1024) {
		printf("error: File is too large (max 6MB)\n");
		goto end;
	}
	fseek(fp, 0, SEEK_SET);
	uint8_t *filebuf = malloc(fsize);
	fread(filebuf, fsize, 1, fp);
	fclose(fp);

	/* Wait for a device to be connected */
	filter_clean_libusb(filter);
	rc = usb_unique_wait(filter, delay, context, &dev);

	if(rc == FILTER_NONE) {
		printf("No device found.\n");
		return 1;
	}
	else if(rc == FILTER_MULTIPLE) {
		printf("Multiple devices found, ambiguous!\n");
		return 1;
	}

	if((rc = libusb_open(dev, &dh))) {
		rc = libusb_err(rc, "cannot open device %s", usb_id(dev));
		goto end;
	}

	/* Don't detach kernel drivers to avoid breaking the Mass Storage
	   communications if fxlink is ever started while the native LINK
	   application is running! */
	libusb_set_auto_detach_kernel_driver(dh, false);

	if((rc = libusb_claim_interface(dh, 0))) {
		rc = libusb_err(rc, "cannot claim interface on %s", usb_id(dev));
		goto end;
	}

	printf("Connected to %s\n", usb_id(dev));

	// Wait to receive "USB loader ready" over USB bulk transfer
	uint8_t buf[18];
	while (1)
	{
		int actual_length;
		rc = libusb_bulk_transfer(dh, 0x82, buf, sizeof(buf) - 1, &actual_length, 0);
		buf[sizeof(buf) - 1] = 0;
		// if (rc == LIBUSB_ERROR_TIMEOUT) continue;
		if (rc) {
			rc = libusb_err(rc, "cannot receive data: %s", usb_id(dev));
			goto end;
		}
		if (actual_length == 0) continue;
		if (actual_length != 17) {
			printf("error: Received %d bytes, expected 17\n", actual_length);
			goto end;
		}
		// See if it's the "USB loader ready" message with strcmp
		if (strcmp((char*) buf, "USB loader ready") == 0) {
			printf("Ready to send!\n");
			break;
		} else {
			printf("error: Unknown message received: %s\n", buf);
			goto end;
		}
	}

	// Send the contents of the passed file over USB bulk transfer

	// First send the size of the file
	uint8_t sizebuf[4];
	sizebuf[0] = (fsize >> 24) & 0xFF;
	sizebuf[1] = (fsize >> 16) & 0xFF;
	sizebuf[2] = (fsize >> 8) & 0xFF;
	sizebuf[3] = fsize & 0xFF;
	rc = libusb_bulk_transfer(dh, 0x01, sizebuf, sizeof(sizebuf), NULL, 0);
	if (rc) {
		rc = libusb_err(rc, "cannot send size: %s", usb_id(dev));
		goto end;
	}

	// Then send the file contents
	printf("Sending %ld bytes\n", fsize);
	int sent = 0;
	while (sent < fsize) {
		int actual_length;
		rc = libusb_bulk_transfer(dh, 0x01, filebuf + sent, fsize - sent, &actual_length, 0);
		if (rc) {
			rc = libusb_err(rc, "cannot send data: %s", usb_id(dev));
			goto end;
		}
		sent += actual_length;
	}
	printf("Sent %d bytes\n", sent);

end:
	if(dh) {
		libusb_release_interface(dh, 0);
		libusb_close(dh);
	}
	if(dev) libusb_unref_device(dev);
	return rc;
}
