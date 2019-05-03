#include <errors.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define MAX 32

/* Array of context strings. */
static char const *context[MAX] = { NULL };
/* Number of active elements. */
static int elements = 0;

/* push(): Push an element in the context buffer
   @el  New element
   Returns non-zero if the buffer is full. */
static int push(char const *el)
{
	if(elements >= MAX - 2) return 1;
	context[elements++] = el;
	return 0;
}

/* err_context(): Push one or more context elements */
int err_context(char const *context, ...)
{
	va_list args;
	va_start(args, context);
	int x = 0;

	/* Delimiter between calls to err_context(), for err_pop() */
	x |= push(NULL);
	x |= push(context);

	while(1)
	{
		char const *el = va_arg(args, char const *);
		if(!el) break;
		x |= push(el);
	}

	va_end(args);
	return x;
}

/* err_pop(): Pop back context elements */
void err_pop(void)
{
	/* Clear everything until the last NULL delimiter */
	while(--elements >= 0 && context[elements])
	{
		context[elements] = NULL;
	}
}

void err_debug(void)
{
	for(int i = 0; i < MAX; i++) fprintf(stderr, "%s: ", context[i]);
	fprintf(stderr, "\n");
}

/* errf(): Emit an error message */
void errf(int flags, char const *str, ...)
{
	int saved_errno = errno;

	va_list args;
	va_start(args, str);

	for(int i = 0; i <= MAX-2 && (context[i] || context[i+1]); i++)
	{
		if(context[i]) fprintf(stderr, "%s: ", context[i]);
	}

	vfprintf(stderr, str, args);

	if(flags & ERR_ERRNO)
	{
		fprintf(stderr, ": %s", strerror(saved_errno));
	}

	fprintf(stderr, "\n");
	va_end(args);
}
