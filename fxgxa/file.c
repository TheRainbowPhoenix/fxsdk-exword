#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fxg1a.h>

/* invert_header(): Bit-invert a standard header
   Part of the header is stored inverted in files for obfuscation purposes. */
static void invert_header(struct g1a *g1a)
{
	uint8_t *data = (void *)&g1a->header;
	for(size_t i = 0; i < 0x20; i++) data[i] = ~data[i];
}

#define fail(msg, ...) {						\
	fprintf(stderr, "error: " msg ": %m\n", ##__VA_ARGS__);		\
	close(fd);							\
	free(data);							\
	return NULL;							\
}

/* load(): Fully load a file into memory
   Allocates a buffer with @prepend leading bytes initialized to zero. */
static void *load(const char *filename, size_t *size, size_t prepend)
{
	int fd;
	struct stat statbuf;
	void *data = NULL;
	size_t filesize;

	fd = open(filename, O_RDONLY);
	if(fd < 0) fail("cannot open %s", filename);

	int x = fstat(fd, &statbuf);
	if(x > 0) fail("cannot stat %s", filename);

	filesize = statbuf.st_size;
	data = malloc(prepend + filesize);
	if(!data) fail("cannot load %s", filename);

	size_t remaining = filesize;
	while(remaining > 0)
	{
		size_t offset = prepend + filesize - remaining;
		ssize_t y = read(fd, data + offset, remaining);

		if(y < 0) fail("cannot read from %s", filename);
		remaining -= y;
	}
	close(fd);

	memset(data, 0, prepend);

	if(size) *size = prepend + filesize;
	return data;
}

/* load_g1a(): Load a g1a file into memory */
struct g1a *load_g1a(const char *filename, size_t *size)
{
	struct g1a *ret = load(filename, size, 0);
	if(ret) invert_header(ret);
	return ret;
}

/* load_binary(): Load a binary file into memory */
struct g1a *load_binary(const char *filename, size_t *size)
{
	struct g1a *ret = load(filename, size, 0x200);
	if(ret) memset(ret, 0xff, 0x20);
	return ret;
}

#undef fail
#define fail(msg, ...) {						\
	fprintf(stderr, "error: " msg ": %m\n", ##__VA_ARGS__);		\
	close(fd);							\
	invert_header(g1a);						\
	return 1;							\
}

/* save_g1a(): Save a g1a file to disk */
int save_g1a(const char *filename, struct g1a *g1a, size_t size)
{
	/* Invert header before saving */
	invert_header(g1a);

	int fd = creat(filename, 0644);
	if(fd < 0) fail("cannot open %s", filename);

	void const *raw = g1a;
	ssize_t status;

	size_t written = 0;
	while(written < size)
	{
		status = write(fd, raw + written, size - written);
		if(status < 0) fail("cannot write to %s", filename);
		written += status;
	}
	close(fd);

	/* Before returning, re-invert header for further use */
	invert_header(g1a);
	return 0;
}
