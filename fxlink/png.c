#include "png.h"
#include "util.h"
#include <stdio.h>

/* fxlink_png_save(): Save a bitmap into a PNG file */
int fxlink_png_save(png_byte **row_pointers, int width, int height,
	char const *path)
{
	png_struct *png = png_create_write_struct(PNG_LIBPNG_VER_STRING,
		NULL, NULL, NULL);
	if(!png)
		return err("failed to write PNG: png_create_write_struct");

	png_infop info = png_create_info_struct(png);
	if(!info)
		return err("failed to write PNG: png_create_info_struct");

	FILE *fp = fopen(path, "wb");
	if(!fp) {
		png_destroy_write_struct(&png, &info);
		return err("failed to open '%s' to write PNG: %m", path);
	}

	if(setjmp(png_jmpbuf(png))) {
		fclose(fp);
		png_destroy_write_struct(&png, &info);
		return 1;
	}

	png_init_io(png, fp);
	png_set_IHDR(png, info,
		width, height, 8,
		PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	png_write_info(png, info);
	png_write_image(png, row_pointers);
	png_write_end(png, NULL);
	png_destroy_write_struct(&png, &info);
	fclose(fp);
	return 0;
}
