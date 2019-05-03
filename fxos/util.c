#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <fxos.h>
#include <errors.h>

/* integer(): Convert base 8, 10 or 16 into integers */
long long integer(char const *str, int *error)
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

/* match_table_name(): Some specific matching on filenames */
char *match_table_name(char const *path, char const *type, char const *suffix)
{
	char const *base = strrchr(path, '/');
	base = (base) ? (base + 1) : (path);

	size_t len   = strlen(base);
	size_t len_t = strlen(type);
	size_t len_s = strlen(suffix);

	if(len > len_t + len_s + 1
		&& !strncmp(type, base, len_t)
		&& base[len_t] == '-'
		&& !strcmp(suffix, base + len - len_s))
	{
		return strndup(base + len_t + 1, len - len_t - len_s - 1);
	}

	return strdup(base);
}

/* map(): Map a file to memory */
void *map(char const *path, int *fd, size_t *size)
{
	int x;
	struct stat statbuf;

	*fd = open(path, O_RDONLY);
	if(*fd < 0)
	{
		errf(ERR_ERRNO, "cannot open file");
		return NULL;
	}

	x = fstat(*fd, &statbuf);
	if(x < 0)
	{
		errf(ERR_ERRNO, "cannot stat file");
		close(*fd);
		return NULL;
	}

	*size = statbuf.st_size;

	void *data = mmap(NULL, *size, PROT_READ, MAP_PRIVATE, *fd, 0);
	if(data == MAP_FAILED)
	{
		errf(ERR_ERRNO, "cannot map file");
		close(*fd);
	}
	return data;
}

/* unmap(): Unmap a file loaded with map() */
void unmap(void *data, int fd, size_t size)
{
	munmap(data, size);
	close(fd);
}
