//---
// fxlink:util - Utility functions and error reporting mechanisms
//---

#ifndef FXLINK_UTIL_H
#define FXLINK_UTIL_H

#include <stdio.h>
#include <stdbool.h>

/* Literal error message printed to stderr, evaluates to 1 for a combined
   return/exit() call */
#define err(fmt, ...) ({ \
	fprintf(stderr, "error: " fmt "\n" __VA_OPT__(,) __VA_ARGS__); \
	1; \
})
/* Fatal error that includes a libusb error message */
#define libusb_err(rc, fmt, ...) ({ \
	fprintf(stderr, "error: " fmt ": %s\n" __VA_OPT__(,) __VA_ARGS__, \
		libusb_strerror(rc)); \
	1; \
})
/* Warning message */
#define wrn(fmt, ...) \
	fprintf(stderr, "warning: " fmt "\n" __VA_OPT__(,) __VA_ARGS__)

/* Warning that includes a libusb error message */
#define libusb_wrn(rc, fmt, ...) \
	fprintf(stderr, "error: " fmt ": %s\n" __VA_OPT__(,) __VA_ARGS__, \
		libusb_strerror(rc))

//---
// Delay
//---

/* delay_t: An expandable allocated time used to wait for devices */
typedef int delay_t;

/* delay_none(): No delay allowed */
delay_t delay_none(void);
/* delay_seconds(): Initial delay from a duration in seconds */
delay_t delay_seconds(int seconds);
/* delay_infinite(): Delay that can run through delay_cycle() indefinitely */
delay_t delay_infinite(void);

/* delay_cycle(): Wait for a short cycle

   This function returns (true) if the delay has expired; otherwise, it waits
   for a short while (250 ms), decreases the supplied delay pointer, and
   returns (false).

   Not returning (true) after waiting, even if the delay expires then, allows
   the caller to perform the task they were waiting on one last time before
   giving up.

   @delay  Input-output: Delay resource to take time from
   -> Return (true) if (*delay) has expired, or (false) after waiting. */
bool delay_cycle(delay_t *delay);

#endif /* FXLINK_UTIL_H */
