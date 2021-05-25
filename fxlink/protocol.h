//---
// fxlink:protocol - Custom fxlink protocol
//---

#ifndef FXLINK_PROTOCOL_H
#define FXLINK_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

/* See the gint source for details on the protocol */
typedef struct
{
	uint32_t version;
	uint32_t size;
	uint32_t transfer_size;

	char application[16];
	char type[16];

} usb_fxlink_header_t;

/* Subheader for the fxlink built-in "image" type */
typedef struct
{
	uint32_t width;
	uint32_t height;
	int pixel_format;

} usb_fxlink_image_t;

/* Pixel formats */
typedef enum
{
	/* Image is an array of *big-endian* uint16_t with RGB565 format */
	USB_FXLINK_IMAGE_RGB565 = 0,
	/* Image is an array of bits in mono format */
	USB_FXLINK_IMAGE_MONO,
	/* Image is two consecutive mono arrays, one for light, one for dark */
	USB_FXLINK_IMAGE_GRAY,

} usb_fxlink_image_format_t;

//---
// Utilities in this implementation
//---

/* Message currently being transferred */
typedef struct
{
	usb_fxlink_header_t header;
	/* Valid when we are reading a message */
	bool valid;
	/* Data already read in this message */
	uint32_t size_read;
	/* Data buffer */
	char *output;

} message_t;

/* fxlink_protocol_decode_image(): Decode an image into RGB888 format

   This function decodes the message into an RGB888 image and returns an array
   of row pointers with the image data (free the array and each element of the
   array after use).

   If there are not enough bytes in the input, it pads with zeros, and if there
   are extra bytes, they are dropped; in both cases a warning is printed. */
uint8_t **fxlink_protocol_decode_image(message_t *msg);

#endif /* FXLINK_PROTOCOL_H */
