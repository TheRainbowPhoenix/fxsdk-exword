#include "protocol.h"
#include "util.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

//---
// Image format
//---

static int img_bytes_per_row(int format, int width)
{
	if(format == USB_FXLINK_IMAGE_RGB565)
		return 2 * width;
	if(format == USB_FXLINK_IMAGE_MONO)
		return (width + 7) >> 3;
	if(format == USB_FXLINK_IMAGE_GRAY)
		return 2 * ((width + 7) >> 3);

	return 0;
}

static void decode_rgb565(void *pixels, int width, int height, int size,
	uint8_t **row_pointers)
{
	int bpr = img_bytes_per_row(USB_FXLINK_IMAGE_RGB565, width);

	for(int y = 0; y < height; y++) {
		void *row = pixels + y * bpr;

		for(int x = 0; x < width; x++) {
			/* Don't read past the read buffer if the image is incomplete */
			void *input = row + 2 * x;
			uint16_t color = 0;
			if(input - pixels + 2 <= size) color = *(uint16_t *)input;

			color = ((color & 0xff00) >> 8) | ((color & 0x00ff) << 8);

			row_pointers[y][3*x+0] = (color >> 11) << 3;
			row_pointers[y][3*x+1] = ((color >> 5) & 0x3f) << 2;
			row_pointers[y][3*x+2] = (color & 0x1f) << 3;
		}
	}
}

static void decode_mono(void *pixels, int width, int height, int size,
	uint8_t **row_pointers)
{
	int bpr = img_bytes_per_row(USB_FXLINK_IMAGE_MONO, width);

	for(int y = 0; y < height; y++) {
		void *row = pixels + y * bpr;

		for(int x = 0; x < width; x++) {
			/* Don't read past the read buffer if the image is incomplete */
			void *input = row + (x >> 3);
			int byte = 0;
			if(input - pixels + 1 <= size) byte = *(uint8_t *)input;
			int color = (byte & (0x80 >> (x & 7))) ? 0 : 255;

			row_pointers[y][3*x+0] = color;
			row_pointers[y][3*x+1] = color;
			row_pointers[y][3*x+2] = color;
		}
	}
}

static void decode_gray(void *pixels, int width, int height, int size,
	uint8_t **row_pointers)
{
	int bpr = img_bytes_per_row(USB_FXLINK_IMAGE_MONO, width);

	for(int k = 0; k < 2 * height; k++) {
		void *row = pixels + k * bpr;
		int y = k % height;

		for(int x = 0; x < width; x++) {
			/* Don't read past the read buffer if the image is incomplete */
			void *input = row + (x >> 3);
			int byte = 0;
			if(input - pixels + 1 <= size) byte = *(uint8_t *)input;

			int color = (byte & (0x80 >> (x & 7)));
			/* Everything is inverted */
			if(!color) color = (k >= height) ? 0xaa : 0x55;
			else color = 0x00;

			row_pointers[y][3*x+0] += color;
			row_pointers[y][3*x+1] += color;
			row_pointers[y][3*x+2] += color;
		}
	}
}

uint8_t **fxlink_protocol_decode_image(message_t *msg)
{
	usb_fxlink_image_t *img = (void *)msg->output;
	void *pixels = msg->output + sizeof *img;

	/* Compute the amount of data for the specified image format and size */
	int bytes_per_row = img_bytes_per_row(img->pixel_format, img->width);
	int expected_size = img->height * bytes_per_row;

	/* Check that the correct amount of data was sent */
	int size = msg->size_read - sizeof *img;
	if(size < expected_size)
		printf("warning: got %d bytes but needed %d, image will be "
			"incomplete\n", size, expected_size);
	if(size > expected_size)
		printf("warning: got %d bytes but needed %d for image, dropping extra"
			"\n", size, expected_size);

	/* Allocate row pointers */
	uint8_t **row_pointers = malloc(img->height*sizeof *row_pointers);
	if(!row_pointers) {
		err("failed to write allocate memory to decode image");
		return NULL;
	}

	for(size_t y = 0; y < img->height; y++) {
		row_pointers[y] = calloc(img->width, 3);
		if(!row_pointers[y]) {
			err("failed to write allocate memory to decode image");
			for(size_t i = 0 ; i < y; i++) free(row_pointers[i]);
			free(row_pointers);
			return NULL;
		}
	}

	/* Decode image */
	if(img->pixel_format == USB_FXLINK_IMAGE_RGB565)
		decode_rgb565(pixels, img->width, img->height, size, row_pointers);
	if(img->pixel_format == USB_FXLINK_IMAGE_MONO)
		decode_mono(pixels, img->width, img->height, size, row_pointers);
	if(img->pixel_format == USB_FXLINK_IMAGE_GRAY)
		decode_gray(pixels, img->width, img->height, size, row_pointers);

	return row_pointers;
}
