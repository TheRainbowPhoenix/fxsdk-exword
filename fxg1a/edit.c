#include <fxg1a.h>
#include <stdio.h>
#include <string.h>
#include <endianness.h>

/* sign(): Sign header by filling fixed fields and checksums */
void sign(struct g1a *g1a, size_t size)
{
	struct header *header = &g1a->header;

	/* Fixed elements */

	memcpy(header->magic, "USBPower", 8);
	header->mcs_type = 0xf3;
	memcpy(header->seq1, "\x00\x10\x00\x10\x00", 5);
	header->seq2 = 0x01;

	header->filesize_be1 = htobe32(size);
	header->filesize_be2 = htobe32(size);

	/* Control bytes and checksums */

	header->control1 = size + 0x41;
	header->control2 = size + 0xb8;
	header->checksum = htobe16(checksum(g1a, size));
}

/* edit_name(): Set application name */
void edit_name(struct g1a *g1a, const char *name)
{
	memset(g1a->header.name, 0, 8);
	if(!name) return;

	for(int i = 0; name[i] && i < 8; i++)
		g1a->header.name[i] = name[i];
}

/* edit_internal(): Set internal name */
void edit_internal(struct g1a *g1a, const char *internal)
{
	memset(g1a->header.internal, 0, 8);
	if(!internal) return;

	for(int i = 0; internal[i] && i < 8; i++)
		g1a->header.internal[i] = internal[i];
}

/* edit_version(): Set version string */
void edit_version(struct g1a *g1a, const char *version)
{
	memset(g1a->header.version, 0, 10);
	if(!version) return;

	for(int i = 0; version[i] && i < 10; i++)
		g1a->header.version[i] = version[i];
}

/* edit_date(): Set build date */
void edit_date(struct g1a *g1a, const char *date)
{
	memset(g1a->header.date, 0, 14);
	if(!date) return;

	for(int i = 0; date[i] && i < 14; i++)
		g1a->header.date[i] = date[i];
}

/* edit_icon(): Set icon from monochrome bitmap */
void edit_icon(struct g1a *g1a, uint8_t const *mono)
{
	memcpy(g1a->header.icon, mono, 68);
}
