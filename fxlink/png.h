//---
// fxlink:png - Tools to output PNG images with libpng
//---

#ifndef FXLINK_PNG_H
#define FXLINK_PNG_H

#include <png.h>

/* fxlink_png_save(): Save a bitmap into a PNG file */
int fxlink_png_save(png_byte **row_pointers, int width, int height,
	char const *path);

#endif /* FXLINK_PNG_H */
