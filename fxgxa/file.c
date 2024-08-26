#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fxgxa.h>

/* invert_header(): Bit-invert a standard header
   Part of the header is stored inverted in files for obfuscation purposes. */
static void invert_header(void *gxa)
{
	uint8_t *data = gxa;
	for(size_t i = 0; i < 0x20; i++) data[i] = ~data[i];
}

#define fail(msg, ...) {						\
	fprintf(stderr, "error: " msg ": %m\n", ##__VA_ARGS__);		\
	fclose(fp);							\
	free(data);							\
	return NULL;							\
}

/* load(): Fully load a file into memory
   Allocates a buffer with @prepend leading bytes initialized to zero. */
static void *load(const char *filename, size_t *size, int header, int footer)
{
	FILE *fp;
	void *data = NULL;
	long filesize;

	fp = fopen(filename, "rb");
	if(!fp) fail("cannot open %s", filename);

	fseek(fp, 0, SEEK_END);
	filesize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	if(ftell(fp) != 0) fail("cannod rewind %s", filename);
	data = malloc(header + filesize + footer);
	if(!data) fail("cannot load %s", filename);

	long remaining = filesize;
	while(remaining > 0)
	{
		long offset = header + filesize - remaining;
		long y = fread(data + offset, 1, remaining, fp);

		if(y <= 0) fail("cannot read from %s at offset %ld",
			filename, offset);
		remaining -= y;
	}
	fclose(fp);

	memset(data, 0, header);
	memset(data + header + filesize, 0, footer);

	if(size) *size = header + filesize + footer;
	return data;
}

void *load_gxa(const char *filename, size_t *size)
{
	void *ret = load(filename, size, 0, 0);
	if(ret) invert_header(ret);
	return ret;
}

void *load_binary(const char *filename, size_t *size, int header, int footer)
{
	void *ret = load(filename, size, header, footer);
	if(ret) invert_header(ret);
	return ret;
}

#undef fail
#define fail(msg, ...) {						\
	fprintf(stderr, "error: " msg ": %m\n", ##__VA_ARGS__);		\
	rc = 1;								\
	goto end;							\
}

int save_gxa(const char *filename, void *gxa, size_t size)
{
	/* Invert header before saving */
	invert_header(gxa);

	int rc = 0;
	int fd = creat(filename, 0644);
	if(fd < 0) fail("cannot open %s", filename);

	ssize_t status;
	size_t written = 0;
	while(written < size)
	{
		status = write(fd, gxa + written, size - written);
		if(status < 0) fail("cannot write to %s", filename);
		written += status;
	}

end:
	/* Before returning, re-invert header for further use */
	if(fd >= 0) close(fd);
	invert_header(gxa);
	return rc;
}
