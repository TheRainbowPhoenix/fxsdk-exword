#include <fxos.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

/* integer(): Convert base 8, 10 or 16 into integers */
long long integer(const char *str, int *error)
{
	char *end;
	errno = 0;

	long long ll = strtoll(str, &end, 0);
	if(errno == ERANGE)
	{
		fprintf(stderr, "error: integer is too large: '%s'\n", str);
		if(error) *error = 1;

		/* In my situation this is often better than LLONG_MIN/MAX */
		return 0;
	}

	if(*end)
	{
		fprintf(stderr, "invalid integer: '%s'\n", str);
		if(error) *error = 1;
		return 0;
	}

	return ll;
}
