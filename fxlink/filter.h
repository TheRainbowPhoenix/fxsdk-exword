//---
// fxlink:filter - Property-based device filtering
//---

#ifndef FXLINK_FILTER_H
#define FXLINK_FILTER_H

#include "properties.h"
#include <stddef.h>
#include <stdio.h>

/* filter_t: An OR-combination of property filters

   Attributes of properties_t objects have AND-semantics when used as filters;
   all of them must match. For more flexibility, the command-line allows the
   user to specify an OR-combination of such filters, called "options". */
typedef struct {
	/* Array of options to be matched against; not terminated. */
	properties_t *options;
	/* Length of (options). */
	size_t length;

} filter_t;

/* Return values for backend-specific matching functions */
enum {
	FILTER_UNIQUE    = 0,
	FILTER_NONE      = 1,
	FILTER_MULTIPLE  = 2,
	FILTER_ERROR     = 3,
};

/* filter_parse(): Parse a filter string */
filter_t *filter_parse(char const *specification);
/* filter_free(): Free a created by filter_parse() */
void filter_free(filter_t *filter);

/* filter_clean_libusb(): Disable filter properties unsupported for libusb */
void filter_clean_libusb(filter_t *filter);
/* filter_clean_udisks2(): Disable filter properties unsupported for udisks2 */
void filter_clean_udisks2(filter_t *filter);

/* filter_match(): Check whether some properties match the supplied filter */
bool filter_match(properties_t const *props, filter_t const *filter);

/* filter_print(): Print a parser filter (one-line; for debugging) */
void filter_print(FILE *fp, filter_t const *filter);

#endif /* FXLINK_FILTER_H */
