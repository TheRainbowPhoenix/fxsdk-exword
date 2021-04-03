//---
// fxlink:usb - libusb functions
//---

#ifndef FXLINK_USB_H
#define FXLINK_USB_H

#include "util.h"
#include "properties.h"
#include "filter.h"
#include <libusb.h>

/* usb_properties(): Determine as many properties of the device as possible

   If the device can be opened, an open handle should be supplied as (dh). This
   is used to determine the serial number; if the device cannot be opened, the
   serial number is omitted from the device properties.

   @dc  Device descriptor
   @dh  Open handle if the device can be opened, or NULL
   -> Returns detected properties of the device. */
properties_t usb_properties(struct libusb_device_descriptor *dc,
	libusb_device_handle *dh);

/* usb_unique_matching(): Device that matches the provided filter, if unique

   This function runs through the list of devices provided by libusb and
   determines whether there is exactly one device matching the filter. If so,
   a pointer to this device is set in (*dev) and FILTER_UNIQUE is returned. The
   device is referenced and should be un-referenced after use for the data to
   be freed. If (dev) is NULL, the pointer is not recorded and not referenced.

   If there are no devices matching the filter, (*dev) is unchanged and this
   function returns FILTER_NONE. If several devices match the filter, (*dev) is
   unchanged and FILTER_MULTIPLE is returned. If an error occurs and the
   function cannot complete, an error is printed and FILTER_ERROR is returned.

   @filter   Device filter to refine the search
   @context  Previously-initialized libusb context
   @dev      Output: unique device matching the filter (may be NULL)
   -> Returns one of FILTER_{UNIQUE,NONE,MULTIPLE,ERROR}. */
int usb_unique_matching(filter_t const *filter, libusb_context *context,
	libusb_device **dev);

/* usb_unique_wait(): Wait for a device matching the provided filter to connect

   This function waits up to the provided delay for a device matching the
   specified filter to be connected. It calls usb_unique_matching() several
   times per second to check for new devices being attached and initialized.

   If several devices are connected when usb_unique_wait() is first called, or
   several devices are connected between two calls to usb_unique_matching(),
   this function returns FILTER_MULTIPLE. As soon as a unique matching device
   is found, the pointer is referenced and set in (*dev) if (dev) is not NULL,
   and FILTER_UNIQUE is returned, regardless of whether other matching devices
   are attached before the end of the wait period.

   If no matching device is attached during the specified period, this function
   returns FILTER_NONE. If an error occurs during scanning, it returns
   FILTER_ERROR.

   @filter   Device filter to refine the search
   @delay    Time resource to use delay from
   @context  Previously-initialized libusb context
   @dev      Output: unique device matching the filter (can be NULL)
   -> Returns one of FILTER_{UNIQUE,NONE,MULTIPLE,ERROR}. */
int usb_unique_wait(filter_t const *filter, delay_t *delay,
	libusb_context *context, libusb_device **dev);

//---
// Iteration on libusb devices
//---

typedef struct {
	/* Current device and its device descriptor */
	libusb_device *dev;
	struct libusb_device_descriptor dc;
	/* If the device can be opened, its open handle, otherwise NULL */
	libusb_device_handle *dh;
	/* Device properties */
	properties_t props;
	/* Whether the iteration has finished */
	bool done;

	/* Internal indicators: list of devices and current index */
	libusb_device **devices;
	int device_count;
	int index;

} usb_iterator_t;

/* usb_iter_start(): Start an iteration on libusb devices
   If the first step fails, returns an iterator with (done = true) and sets
   (*error) to true; otherwise, sets (*error) to false. */
usb_iterator_t usb_iter_start(libusb_context *context, bool *error);

/* usb_iter_next(): Iterate to the next libusb device */
void usb_iter_next(usb_iterator_t *it);

/* Convenience for-loop macro for iteration */
#define for_libusb_devices(NAME, context, error) \
	for(usb_iterator_t NAME = usb_iter_start(context, error); \
		!NAME.done; usb_iter_next(&NAME)) if(!NAME.done)

//---
// Miscellaneous
//---

/* usb_id(): Printable address-based identifier for error messages
   This function is used in error messages to describe the device on which an
   error occurred in a useful way. The pointer returned is to a static buffer
   that changes at every call to this function, and should only be used briefly
   to generate messages. */
char const *usb_id(libusb_device *dev);

/* usb_serial_number(): Serial number advertised by the device

   This function returns the serial number (as presented with iSerialNumber in
   the device descriptor) of the provided device, which may or may not be
   present.

   Serial numbers for CASIO calculators normally have 8 letters. The LINK
   application presents a 12-character code with "0000" prepended. This
   function detects this quirk and only returns the last 8 characters. gint's
   driver doesn't send to "0000" prefix.

   This function requires the device to be open in order to send the request
   for the STRING descriptor, and cannot be used if the process user doesn't
   have write access to the device.

   @dh  Open device handle
   -> Returns a freshly-allocated copy of the serial number string, to be
      free()'d after use, or NULL if the serial number is unspecified or cannot
      be retrieved. */
char *usb_serial_number(libusb_device_handle *dh);

#endif /* FXLINK_USB_H */
