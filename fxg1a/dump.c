#include <stdio.h>
#include <string.h>
#include <endian.h>

#include <fxg1a.h>
#include <g1a.h>

/* check(): Check validity of a g1a control or fixed field

   This function checks a single field of a g1a header (depending on the value
   of @test, from 0 up) and returns:
   * 0 if the field is valid
   * 1 if there is a minor error (wrong fixed-byte entry)
   * 2 if there is a major error (like not a g1a, bad checksum, etc)
   * -1 if the value of @test is out of bounds

   It produces a description of the check in @status (even if the test is
   passed); the string should have room for at least 81 bytes.

   @test    Test number
   @g1a     G1A file being manipulated
   @size    File size
   @status  Array row, at least 81 bytes free */
static int check(int test, struct g1a const *g1a, size_t size, char *status)
{
	#define m(msg, ...) sprintf(status, msg, ##__VA_ARGS__)

	struct header const *h = &g1a->header;
	uint8_t const *raw = (void *)h;

	uint16_t sum;
	uint8_t ctrl;

	switch(test)
	{
	case 0:
		m("Signature    \"USBPower\"    \"########\"");
		strncpy(status + 28, h->magic, 8);
		return strncmp(h->magic, "USBPower", 8) ? 2:0;
	case 1:
		m("MCS Type     0xf3          0x%02x", h->mcs_type);
		return (h->mcs_type != 0xf3) ? 2:0;
	case 2:
		m("Sequence 1   0x0010001000  0x%02x%02x%02x%02x%02x",
		h->seq1[0], h->seq1[1], h->seq1[2], h->seq1[3], h->seq1[4]);
		return strncmp((const char *)h->seq1, "\x00\x01\x00\x01\x00",
			5) ? 1:0;
	case 3:
		ctrl = raw[0x13] + 0x41;
		m("Control 1    0x%02x          0x%02x", ctrl, h->control1);
		return (h->control1 != ctrl) ? 2:0;
	case 4:
		m("Sequence 2   0x01          0x%02x", h->seq2);
		return (h->seq2 != 0x01) ? 1:0;
	case 5:
		m("File size 1  %-8zu      %u", size,
			be32toh(h->filesize_be1));
		return (be32toh(h->filesize_be1) != size) ? 2:0;
	case 6:
		ctrl = raw[0x13] + 0xb8;
		m("Control 2    0x%02x          0x%02x", ctrl, h->control2);
		return (h->control2 != ctrl) ? 2:0;
	case 7:
		sum = checksum(g1a, size);
		m("Checksum     0x%02x        0x%02x", sum,
			be16toh(h->checksum));
		return (be16toh(h->checksum) != sum) ? 2:0;
	case 8:
		m("File size 2  %-8zu      %u", size,
			be32toh(h->filesize_be2));
		return (be32toh(h->filesize_be2) != size) ? 2:0;
	default:
		return -1;
	}
}

/* unknown(): Print an unknown field
   @data    Address of field
   @offset  Offset of field in header
   @size    Number of consecutive unknown bytes */
static void unknown(uint8_t const *data, size_t offset, size_t size)
{
	printf("  0x%03zx   %-4zd  0x", offset, size);
	for(size_t i = 0; i < size; i++) printf("%02x", data[offset + i]);
	printf("\n");
}

/* field(): Print a text field with limited size
   @field  Address of text field
   @size   Maximum number of bytes to print */
static void field(const char *field, size_t size)
{
	for(size_t i = 0; i < size && field[i]; i++) putchar(field[i]);
	printf("\n");
}

/* dump(): Print the detailed header fields of a g1a file */
void dump(struct g1a const *g1a, size_t size)
{
	struct header const *header = &g1a->header;
	uint8_t const *raw = (void *)header;

	/* Checks for g1a files */
	char status[81];
	int ret = 0;
	int passed = 0;

	printf("G1A signature checks:\n\n");
	printf("  Sta.  Field        Expected      Value\n");

	for(int test = 0; ret >= 0; test++)
	{
		ret = check(test, g1a, size, status);
		passed += !ret;
		if(ret < 0) break;

		printf("  %s  %s\n", ret ? "FAIL" : "OK  ", status);
	}

	printf("\nFields with unknown meanings:\n\n");
	printf("  Offset  Size  Value\n");

	unknown(raw, 0x015, 1);
	unknown(raw, 0x018, 6);
	unknown(raw, 0x028, 3);
	unknown(raw, 0x02c, 4);
	unknown(raw, 0x03a, 2);
	unknown(raw, 0x04a, 2);
	unknown(raw, 0x1d0, 4);
	unknown(raw, 0x1dc, 20);
	unknown(raw, 0x1f4, 12);

	printf("\nApplication metadata:\n\n");

	printf("  Program name:   ");
	field(header->name, 8);
	printf("  Internal name:  ");
	field(header->internal, 8);
	printf("  Version:        ");
	field(header->version, 10);
	printf("  Build date:     ");
	field(header->date, 14);

	printf("\nProgram icon:\n\n");
	icon_print(header->icon);
}
