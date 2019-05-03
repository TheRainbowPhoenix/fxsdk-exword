//---
//	errors: Error message management
//
//	An attempt at producing elaborate error messages providing context from
//	multiple function calls, with a small amount of code.
//
//	Basically, caller functions define context and message parameters
//	before doing anything risky. When an error occurs in a sub-function,
//	the context is used to prepend relevant information and fill in the
//	error message where relevant.
//
//	This means that the caller functions must know it advance what kind of
//	messages can be emitted to accurately set the context. Although it
//	would work through library calls, it is not very suitable. It's rather
//	meant for the logic of applications.
//---

#ifndef FXOS_ERRORS
#define FXOS_ERRORS

/* Error levels */
#define ERR_CRIT   0x01    /* Aborts part or whole of the program's task */
#define ERR_ERR    0x02    /* Unexpected behavior */
#define ERR_WARN   0x03    /* Internal consistency issues */
#define ERR_LOG    0x04    /* Internal logging */
/* Error flags */
#define ERR_ERRNO  0x10

/* err_context(): Push one or more context elements
   @context  Context string, must live until it is popped by err_pop()
   @...      More contexts, same requirements as the first (NULL-terminated)
   Returns non-zero if the deepest context level (at least 15) is reached. */
int err_context(char const *context, ...);

/* err_pop(): Pop back context elements
   Pops back all context elements that were added by the previous call to
   err_context(), no matter their number. */
void err_pop(void);

/* errf(): Emit an error message
   Error message is printed via fprintf() and supports the same format. All the
   arguments are passed directly.

   Available flags are any combination of at most one error level:
     ERR_CRIT    -- Unrecoverable error
     ERR_ERR     -- Normal error
     ERR_WARN    -- Warning
     ERR_LOG     -- Logging
   and any of these boolean flags:
     ERR_ERRNO   -- Also print strerror(errno) as a suffix (as perror()).

   Error levels might be used later on for filtering. Messages without level
   will always be displayed.

   @flags  An OR-combination of the previous flags, or 0
   @str    Error messages
   @...    Arguments to format [str] through fprintf() */
void errf(int flags, char const *str, ...);

/* err(): Short error function */
#define err(str, ...) errf(0, str __VA_OPT__(,) __VA_ARGS__)

#endif /* FXOS_ERRORS */
