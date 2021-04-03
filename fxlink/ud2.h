//---
// fxlink:ud2 - UDisks2 functions
//---

#ifndef FXLINK_UD2_H
#define FXLINK_UD2_H

#ifndef FXLINK_DISABLE_UDISKS2

#include <udisks/udisks.h>
#include "config.h"
#include "properties.h"
#include "filter.h"
#include "util.h"

/* ud2_properties(): Determine properties of a UDisks2 USB drive */
properties_t ud2_properties(UDisksDrive *drive);

/* ud2_unique_matching(): Device matching the provided filter, if unique
   Similar to usb_unique_matching(), please refer to "usb.h" for details.
   There are just many more inputs and outputs. */
int ud2_unique_matching(filter_t const *filter, UDisksClient *udc,
	UDisksManager *udm, UDisksBlock **block, UDisksDrive **drive,
	UDisksFilesystem **fs);

/* ud2_unique_wait(): Wait for a device matching the provided filter to connect
   Like usb_unique_wait(), please see "usb.h" for details. */
int ud2_unique_wait(filter_t const *filter, delay_t *delay, UDisksClient *udc,
	UDisksManager *udm, UDisksBlock **block, UDisksDrive **drive,
	UDisksFilesystem **fs);

//---
// Iteration on UDisks2 devices
//---

typedef struct {
	/* Current block, associated drive and filesystem */
	UDisksBlock *block;
	UDisksDrive *drive;
	UDisksFilesystem *fs;
	/* Device properties */
	properties_t props;
	/* Whether the iteration has finished */
	bool done;

	/* Internal indicators: list of devices and current index */
	gchar **devices;
	int index;
	/* Client for object queries */
	UDisksClient *udc;

} ud2_iterator_t;

/* ud2_iter_start(): Start an iteration on UDisks2 devices
   If the first step fails, returns an iterator with (done = true) and sets
   (*error) to true; otherwise, sets (*error) to false. */
ud2_iterator_t ud2_iter_start(UDisksClient *udc, UDisksManager *udm,
	bool *error);

/* ud2_iter_next(): Iterate to the next UDisks2 device */
void ud2_iter_next(ud2_iterator_t *it);

/* Convenience for-loop macro for iteration */
#define for_udisks2_devices(NAME, udc, udm, error) \
	for(ud2_iterator_t NAME = ud2_iter_start(udc, udm, error); \
		!NAME.done; ud2_iter_next(&NAME)) if(!NAME.done)

#endif /* FXLINK_DISABLE_UDISKS2 */

#endif /* FXLINK_UD2_H */
