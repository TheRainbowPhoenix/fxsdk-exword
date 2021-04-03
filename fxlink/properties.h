//---
// fxlink:properties - Detected models and properties of devices
//---

#ifndef FXLINK_PROPERTIES_H
#define FXLINK_PROPERTIES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

/* properties_t: Type of properties that can be detected on a device

   This structure lists all the properties that fxlink can detect on connected
   devices. These properties help identify the devices in interactive use, and
   accurately specify which calculators to interact with when several models
   are connected simultaneously or when the same command in run against
   different models in a script.

   Depending on the backend and access privileges, not all properties can be
   detected. The backends supporting each property are listed in brackets at
   the end of each description.

   An instance of this structure can also be used as a filter. In order for the
   semantics of filtering to work out, every attribute needs to have a "total
   information order", meaning that two values can be compared for how specific
   they are. A device will match a filter if and only if all of its properties
   are more specific than the values provided in the filter.

   Currently the order is "true more specific than false" for all booleans and
   "any value more specific than NULL" for the serial number. Properties that
   cannot be detected by back-ends are reset to their least specific value (ie.
   ignored). */
typedef struct {

	/* The calculator is a Protocol 7 calculator (idProduct: 0x6101). This
	   makes no sense in UDisks2 as a P7 calculator has no disks, therefore
	   this property is always false in UDisks2. [libusb, udisks2] */
	 bool p7;
	 /* The calculator is a Mass Storage calculator (idProduct: 0x6102). All
	    devices detected in UDisks2 are of this type. [libusb, udisks2] */
	 bool mass_storage;
	 /* The calculator is an fx-CG series. [udisks2] */
	 bool series_cg;
	 /* The calculator is a G-III series. [udisks2] */
	 bool series_g3;

	 /* Serial number. This can only be obtained in libusb if the user has
	    write access to the device, because libusb needs to send a request for
	    the STRING descriptor holding the serial number. [libusb, udisks2] */
	 char *serial_number;

} properties_t;

/* properties_match(): Check whether some properties match a given option

   Returns true if (props) is more specific than (option), meaning that every
   property mentioned in (option) is indeed set in (props). This is a building
   block for filter_match() and probably doesn't need to be used directly. */
bool properties_match(properties_t const *props, properties_t const *option);

/* properties_print(): Print a property set (one-line) */
void properties_print(FILE *fp, properties_t const *props);

#endif /* FXLINK_PROPERTIES_H */
