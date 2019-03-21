//---
//	fxg1a:fxg1a - Main interfaces
//---

#ifndef FX_FXG1A
#define FX_FXG1A

#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <g1a.h>

/*
**  Header dumping (dump.c)
*/

/* dump(): Print the detailed header fields of a g1a file
   This function takes as argument the full file loaded into memory and the
   size of the file. It does various printing to stdout as main job.

   @g1a   Full file data
   @size  Size of g1a file */
void dump(struct g1a const *g1a, size_t size);


/*
**  Header manipulation (edit.c)
*/

/* sign(): Sign header by filling fixed fields and checksums
   This function fills the fixed fields and various checksums of a g1a file. To
   do this it accesses some of the binary data. To set the user-customizable
   field, use the edit_*() functions. (The value of the customizable fields
   does not influence the checksums so it's okay to not call this function
   afterwards.)

   @g1a   Header to sign
   @size  Size of raw file data */
void sign(struct g1a *g1a, size_t size);

/* edit_*(): Set various fields of a g1a header */

void edit_name     (struct g1a *g1a, const char *name);
void edit_internal (struct g1a *g1a, const char *internal);
void edit_version  (struct g1a *g1a, const char *version);
void edit_date     (struct g1a *g1a, const char *date);

/* edit_icon(): Set monochrome icon of a g1a header
   The icon parameter must be loaded in 1-bit bitmap format. */
void edit_icon(struct g1a *header, uint8_t const *mono);


/*
**  Utility functions (util.c)
*/

/* checksum(): Sum of 8 big-endian shorts at 0x300
   Computes the third checksum by summing bytes from the data part of the file.

   @g1a   Add-in file whose checksum is requested
   @size  Size of file */
uint16_t checksum(struct g1a const *g1a, size_t size);

/* default_output(): Calculate default output file name
   This function computes a default file name by replacing the extension of
   @name (if it exists) or adding one. The extension is specified as a suffix,
   usually in the form ".ext".

   The resulting string might be as long as the length of @name plus that of
   @suffix (plus one NUL byte); the provided buffer must point to a suitably-
   large allocated space.

   @name    Input file name
   @suffix  Suffix to add or replace @name's extension with
   @output  Output file name */
void default_output(const char *name, const char *suffix, char *output);


/*
**  File manipulation (file.c)
*/

/* load_g1a(): Load a g1a file into memory
   This function loads @filename into a dynamically-allocated buffer and
   returns the address of that buffer; it must be free()'d after use. When
   loading the file, if @size is not NULL, it receives the size of the file.
   On error, load() prints a message an stderr and returns NULL. The header
   is inverted before this function returns.

   @filename  File to load
   @size      If non-NULL, receives the file size
   Returns a pointer to a buffer with loaded data, or NULL on error. */
struct g1a *load_g1a(const char *filename, size_t *size);

/* load_binary(): Load a binary file into memory
   This function operates like load_g1a() but reserves space for an empty
   header. The header is initialized with all zeros.

   @filename  File to load
   @size      If non-NULL, receives the file size
   Returns a pointer to a buffer with loaded data, or NULL on error. */
struct g1a *load_binary(const char *filename, size_t *size);

/* save_g1a(): Save a g1a file to disk
   This functions creates @filename, then writes a g1a header and a chunk of
   raw data to it. Since it temporarily inverts the header to comply with
   Casio's obfuscated format, it needs write access to @g1a. Returns non-zero
   on error.

   @filename  File to write (it will be overridden if it exists)
   @g1a       G1A data to write
   @size      Size of data
   Returns zero on success and a nonzero error code otherwise. */
int save_g1a(const char *filename, struct g1a *g1a, size_t size);


/*
**  Icon management (icon.c)
*/

/* icon_print(): Show a monochrome 30*17 icon on stdout
   The buffer should point to a 68-byte array. */
void icon_print(uint8_t const *icon);

/* icon_load(): Load a monochrome PNG image into an array
   This function loads a PNG image into a 1-bit buffer; each row is represented
   by a fixed number of bytes, each byte being 8 pixels. Rows are loaded from
   top to bottom, and from left to right.

   If the image is not a PNG image or a reading error occurs, this functions
   prints an error message on stderr and returns NULL.

   @filename  PNG file to load
   @width     If non-NULL, receives image width
   @height    If non-NULL, receives image height
   Returns a pointer to a free()able buffer with loaded data, NULL on error. */
uint8_t *icon_load(const char *filename, size_t *width, size_t *height);

/* icon_save(): Save an 8-bit array to a PNG image
   Assumes 8-bit GRAY format.

   @filename  Target filename
   @input     An 8-bit GRAY image
   @width     Width of input, should be equal to stride
   @height    Height of input
   Returns non-zero on error. */
int icon_save(const char *filename, uint8_t *input, size_t width,
	size_t height);

/* icon_conv_8to1(): Convert an 8-bit icon to 1-bit
   The returned 1-bit icon is always of size 30*17, if the input size does not
   match it is adjusted.

   @input   8-bi data
   @width   Width of input image
   @height  Height of input image
   Returns a free()able buffer with a 1-bit icon on success, NULL on error. */
uint8_t *icon_conv_8to1(uint8_t const *input, size_t width, size_t height);

/* icon_conv_1to8(): Convert an 1-bit icon to 8-bit
   Input 1-bit is assumed to be 30*17 in size, this function returns an 8-bit
   buffer with the same dimensions.

   @mono  Input monochrome icon (from a g1a header, for instance)
   Returns a free()able buffer, or NULL on error. */
uint8_t *icon_conv_1to8(uint8_t const *mono);

#endif /* FX_FXG1A */
