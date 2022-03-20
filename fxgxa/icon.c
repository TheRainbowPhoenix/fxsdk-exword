#include <stdio.h>
#include <string.h>

#include <fxg1a.h>
#include <png.h>

/* icon_print(): Show a monochrome 30*17 icon on stdout */
void icon_print(uint8_t const *icon)
{
	for(int y = 0; y < 17; y++)
	{
		for(int x = 0; x < 30; x++)
		{
			int v = icon[(y << 2) + (x >> 3)] & (0x80 >> (x & 7));
			putchar(v ? '#' : ' ');
			putchar(v ? '#' : ' ');
		}

		putchar('\n');
	}
}

/* icon_load(): Load a monochrome PNG image into an array */
uint8_t *icon_load(const char *filename, size_t *width, size_t *height)
{
	png_image img;
	memset(&img, 0, sizeof img);
	img.opaque = NULL;
	img.version = PNG_IMAGE_VERSION;

	png_image_begin_read_from_file(&img, filename);
	if(img.warning_or_error)
	{
		fprintf(stderr, "libpng %s: %s\n", img.warning_or_error == 1
			? "warning": "error", img.message);
		if(img.warning_or_error > 1)
		{
			png_image_free(&img);
			return NULL;
		}
	}

	img.format = PNG_FORMAT_GRAY;

	void *buffer = calloc(img.width * img.height, 1);
	if(!buffer)
	{
		fprintf(stderr, "error: cannot read %s: %m\n", filename);
		png_image_free(&img);
		return NULL;
	}

	png_image_finish_read(&img, NULL, buffer, img.width, NULL);
	if(width) *width = img.width;
	if(height) *height = img.height;

	png_image_free(&img);
	return buffer;
}

/* icon_save(): Save an 8-bit array to a PNG image */
int icon_save(const char *filename, uint8_t *input, size_t width,
	size_t height)
{
	png_image img;
	memset(&img, 0, sizeof img);

	img.version = PNG_IMAGE_VERSION;
	img.width = width;
	img.height = height;
	img.format = PNG_FORMAT_GRAY;

	png_image_write_to_file(&img, filename, 0, input, 0, NULL);
	png_image_free(&img);

	if(img.warning_or_error)
	{
		fprintf(stderr, "libpng %s: %s\n", img.warning_or_error == 1
			? "warning": "error", img.message);
		if(img.warning_or_error > 1) return 1;
	}

	return 0;
}

/* icon_conv_8to1(): Convert an 8-bit icon to 1-bit */
uint8_t *icon_conv_8to1(uint8_t const *input, size_t width, size_t height)
{
	if(!input) return NULL;
	uint8_t *mono = calloc(68, 1);
	if(!mono) return NULL;
	size_t stride = width;

	/* If the image is wider than 30 pixels, ignore columns at the right */
	if(width > 30) width = 30;

	/* Skip the first line if there is enough rows, because in standard
	   30*19 icons, the first and last lines are skipped */
	if(height > 17) input += stride, height--;

	/* If height is still larger than 17, ignore rows at the bottom */
	if(height > 17) height = 17;

	/* Then copy individual pixels on the currently-blank image */
	for(size_t y = 0; y < height; y++)
	for(size_t x = 0; x < width; x++)
	{
		int offset = (y << 2) + (x >> 3);
		int color = input[y * stride + x] < 128;
		uint8_t mask = color << (~x & 7);
		mono[offset] |= mask;
	}

	return mono;
}

/* icon_conv_1to8(): Convert an 1-bit icon to 8-bit */
uint8_t *icon_conv_1to8(uint8_t const *mono)
{
	uint8_t *data = calloc(30 * 17, 1);
	if(!data) return NULL;

	for(int y = 0; y < 17; y++)
	for(int x = 0; x < 30; x++)
	{
		int offset = (y << 2) + (x >> 3);
		int bit = mono[offset] & (0x80 >> (x & 7));
		data[y * 30 + x] = (bit ? 0x00 : 0xff);
	}

	return data;
}
