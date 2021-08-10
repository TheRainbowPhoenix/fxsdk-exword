#include "config.h"
#include "fxlink.h"
#include "util.h"
#include "properties.h"
#include "filter.h"
#include "protocol.h"
#include "usb.h"
#include "png.h"
#include "sdl2.h"

#include <libusb.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <png.h>

static char *output_file(char const *path,char const *type,char const *suffix)
{
	char *filename = NULL;
	int counter = 1;

	time_t time_raw;
	struct tm time_bd;
	time(&time_raw);
	localtime_r(&time_raw, &time_bd);

	while(1) {
		asprintf(&filename, "%s/fxlink-%.16s-%04d.%02d.%02d-%02dh%02d-%d.%s",
			path, type, time_bd.tm_year + 1900, time_bd.tm_mon,
			time_bd.tm_mday, time_bd.tm_hour, time_bd.tm_min, counter, suffix);
		if(!filename) continue;

		/* Try to find a name for a file that doesn't exist */
		if(access(filename, F_OK) == -1) break;

		free(filename);
		counter++;
	}

	return filename;
}

static bool message_new(message_t *msg, usb_fxlink_header_t const *h)
{
	int version_major = (h->version >> 8) & 0xff;
	int version_minor = (h->version) & 0xff;

	fprintf(stderr, "New message (v%d.%d): application '%.16s', type '%.16s', "
		"size %d bytes\n", version_major, version_minor, h->application,
		h->type, h->size);

	msg->output = malloc(h->size);
	if(!msg->output) {
		err("cannot allocate memory for message of %d bytes", h->size);
		return false;
	}

	msg->header = *h;
	msg->size_read = 0;
	msg->valid = true;
	return true;
}

static void message_finish(message_t *msg)
{
	char const *path = ".";

	if(!strncmp(msg->header.application, "fxlink", 16)) {
		if(!strncmp(msg->header.type, "image", 16)) {
			usb_fxlink_image_t *img = (void *)msg->output;
			char *filename = output_file(path, msg->header.type, "png");

			uint8_t **row_pointers = fxlink_protocol_decode_image(msg);
			fxlink_png_save(row_pointers, img->width, img->height, filename);

			printf("Saved image (%dx%d, format=%d) to '%s'\n",
				img->width, img->height, img->pixel_format, filename);
			free(row_pointers);
			free(filename);
			return;
		}

		if(!strncmp(msg->header.type, "text", 16)) {
			printf("------------------\n");
			fwrite(msg->output, 1, msg->header.size, stdout);
			if(msg->output[msg->header.size - 1] != '\n') printf("\n");
			printf("------------------\n");
			return;
		}

		if(!strncmp(msg->header.type, "video", 16)) {
			usb_fxlink_image_t *img = (void *)msg->output;
			uint8_t **row_pointers = fxlink_protocol_decode_image(msg);

#ifndef FXLINK_DISABLE_SDL2
			sdl2_stream(row_pointers, img->width, img->height);
#else
			warn("SDL2 support disabled, skipping video frame!");
#endif
			return;
		}
	}

	/* Default to saving to a blob */
	char *filename = output_file(path, "blob", "bin");
	FILE *fp = fopen(filename, "wb");
	if(!fp) {
		err("could not save to '%s': %m", filename);
		return;
	}

	fwrite(msg->output, 1, msg->header.size, fp);
	fclose(fp);
	fprintf(stderr, "Saved as blob to '%s'\n", filename);
	free(filename);
}

static void message_output(message_t *msg, void *buffer, int size)
{
	int data_left = msg->header.size - msg->size_read;

	if(size > data_left) {
		err("Too much data in message, dropping %d bytes", size - data_left);
		size = data_left;
	}

	memcpy(msg->output + msg->size_read, buffer, size);

	msg->size_read += size;
	if(msg->size_read >= msg->header.size) {
		fprintf(stderr, "Successfully read %d bytes\n", msg->size_read);
		message_finish(msg);
		msg->valid = false;
	}
}

int main_interactive(filter_t *filter, delay_t *delay, libusb_context *context)
{
	libusb_device *dev = NULL;
	libusb_device_handle *dh = NULL;

	/* Wait for a device to be connected */
	filter_clean_libusb(filter);
	int rc = usb_unique_wait(filter, delay, context, &dev);

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

	printf("Connected to %s, starting test.\n", usb_id(dev));

	/* This buffer is used to receive messages; if the header is not complete
	   it is left in the buffer, hence the extra room */
	__attribute__((aligned(4)))
	static uint8_t buffer[2048 + sizeof(usb_fxlink_header_t)] = { 0 };
	/* Amount of data in the buffer */
	int buffer_size = 0;

	/* Current message */
	message_t msg = { 0 };

	while(1)
	{
#ifndef FXLINK_DISABLE_SDL2
		sdl2_tick();
#endif

		int transferred = -1;
		rc = libusb_bulk_transfer(dh, 0x81, buffer + buffer_size, 2048,
			&transferred, 500);

		if(rc == LIBUSB_ERROR_NO_DEVICE) {
			printf("Disconnected, leaving.\n");
			break;
		}
		else if(rc && rc != LIBUSB_ERROR_TIMEOUT) {
			rc = libusb_err(rc, "bulk transfer failed on %s", usb_id(dev));
			continue;
		}
		if(transferred <= 0) continue;

		buffer_size += transferred;

		/* If there is an unfinished message, continue working on it */
		if(msg.valid) {
			message_output(&msg, buffer, buffer_size);
			buffer_size = 0;
		}

		/* If the header is not yet fully transmitted, wait */
		usb_fxlink_header_t *h = (void *)buffer;
		if(buffer_size < (int)sizeof *h) continue;

		/* Handle a new message */
		if(h->version == 0x00000100) {
			int data_size = buffer_size - sizeof *h;

			if(!message_new(&msg, h))
				printf("dropping %d bytes\n", data_size);
			else
				message_output(&msg, buffer + sizeof *h, data_size);

			buffer_size = 0;
			continue;
		}
		else {
			err("invalid header, dropping %d bytes", transferred);
			buffer_size = 0;
		}
	}

	/* Save last unfinished message */
	if(buffer_size > 0) {
		printf("%d bytes not collected dropped\n", buffer_size);
	}
	rc = 0;

end:
	if(dh) {
		libusb_release_interface(dh, 0);
		libusb_close(dh);
	}
	if(dev) libusb_unref_device(dev);
	return rc;
}
