#include "usb.h"
#include "fxlink.h"
#include "util.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>

char const *usb_id(libusb_device *dev)
{
	static char id[32];
	sprintf(id, "%d:%d",
		libusb_get_bus_number(dev),
		libusb_get_device_address(dev));
	return id;
}

char *usb_serial_number(libusb_device_handle *dh)
{
	struct libusb_device_descriptor dc;
	libusb_device *dev = libusb_get_device(dh);

	if(libusb_get_device_descriptor(dev, &dc))
		return NULL;
	if(!dc.iSerialNumber)
		return NULL;

	char serial[256];
	int length = libusb_get_string_descriptor_ascii(dh, dc.iSerialNumber,
		(unsigned char *)serial, 256);
	if(length < 0)
		return NULL;

	/* LINK sends a 12-byte serial number with four leading 0. Remove them */
	int start = (length == 12 && !strncmp(serial, "0000", 4)) ? 4 : 0;
	return strndup(serial + start, length - start);
}

properties_t usb_properties(struct libusb_device_descriptor *dc,
	libusb_device_handle *dh)
{
	properties_t props = { 0 };

	/* Type of calculator based on USB behavior, detected by idProduct */
	if(dc->idProduct == 0x6101)
		props.p7 = true;
	if(dc->idProduct == 0x6102)
		props.mass_storage = true;
	if(dh)
		props.serial_number = usb_serial_number(dh);

	return props;
}

int usb_unique_matching(filter_t const *filter, libusb_context *context,
	libusb_device **dev)
{
	libusb_device *unique = NULL;
	int status = FILTER_NONE;
	bool error;

	for_libusb_devices(it, context, &error) {
		if(!filter_match(&it.props, filter)) continue;

		/* Already found a device before */
		if(unique) {
			status = FILTER_MULTIPLE;
			libusb_unref_device(unique);
			unique = NULL;
			break;
		}

		/* First device: record it */
		unique = libusb_ref_device(it.dev);
		status = FILTER_UNIQUE;
	}
	if(error)
		return FILTER_ERROR;

	/* Don't keep the reference to the device if we're not returning it */
	if(unique && !dev)
		libusb_unref_device(unique);
	if(unique && dev)
		*dev = unique;

	return status;
}

int usb_unique_wait(filter_t const *filter, delay_t *delay,
	libusb_context *context, libusb_device **dev)
{
	while(true) {
		int rc = usb_unique_matching(filter, context, dev);

		/* If a device is found, multiple devices are found, or an error
		   occurs, forward the result; wait only if nothing was found */
		if(rc != FILTER_NONE) return rc;
		if(delay_cycle(delay)) return FILTER_NONE;
	}
}

//---
// Iteration on libusb devices
//---

usb_iterator_t usb_iter_start(libusb_context *context, bool *error)
{
	usb_iterator_t it = { 0 };

	it.device_count = libusb_get_device_list(context, &it.devices);
	if(it.device_count < 0) {
		libusb_err(it.device_count, "cannot get libusb device list");
		it.done = true;
		if(error) *error = true;
		return it;
	}

	it.index = -1;
	usb_iter_next(&it);
	if(error) *error = false;
	return it;
}

void usb_iter_next(usb_iterator_t *it)
{
	if(it->done == true) return;
	int rc;

	/* Free the resources from the previous iteration */
	if(it->dh)
		libusb_close(it->dh);
	it->dev = NULL;
	it->dh = NULL;

	/* Load the next device */
	if(++it->index >= it->device_count) {
		it->done = true;
	}
	else {
		it->dev = it->devices[it->index];

		if((rc = libusb_get_device_descriptor(it->dev, &it->dc))) {
			libusb_err(rc, "cannot get descriptor for device %s",
				usb_id(it->dev));
			return usb_iter_next(it);
		}

		/* Ignore non-CASIO devices */
		if(it->dc.idVendor != 0x07cf)
			return usb_iter_next(it);

		if((rc = libusb_open(it->dev, &it->dh)))
			libusb_wrn(rc, "cannot open device %s", usb_id(it->dev));

		it->props = usb_properties(&it->dc, it->dh);
	}

	if(it->done)
		libusb_free_device_list(it->devices, true);
}
