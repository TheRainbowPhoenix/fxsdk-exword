#include <string.h>
#include <ctype.h>
#include <endianness.h>
#include <fxg1a.h>

/*
**  Public API
*/

/* checksum(): Sum of 8 big-endian shorts at 0x300 */
uint16_t checksum(struct g1a const *g1a, size_t size)
{
	uint16_t shorts[16] = { 0 };

	/* Extract 16 bytes from the file (maybe less are available) */
	int available = size - 0x300;
	if(available < 0) available = 0;
	if(available > 16) available = 16;
	memcpy(shorts, g1a->code + 0x100, available);

	/* Do the big-endian sum */
	uint16_t sum = 0;
	for(int i = 0; i < 8; i++) sum += htobe16(shorts[i]);

	return sum;
}

/* default_output(): Calculate default output file name */
void default_output(const char *name, const char *suffix, char *output)
{
	/* Check if there is a dot at the end of @name, before the last '/'.
	   The dot must also not be in first position (hidden files) */
	size_t end = strlen(name) - 1;
	while(end >= 1 && name[end] != '/' && name[end] != '.') end--;

	/* If we don't have a dot in the file name, append the extension */
	if(end < 1 || name[end] != '.')
	{
		strcpy(output, name);
		strcat(output, suffix);
	}

	/* If we found a dot before the last slash, replace the extension */
	else
	{
		memcpy(output, name, end);
		strcpy(output + end, suffix);
	}
}

/* default_internal(): Calculate default internal name */
void default_internal(const char *name, char *output)
{
	output[0] = '@';
	int i=1;

	for(int j=0; name[j] && i < 8; j++)
	{
		if(isalpha(name[j])) output[i++] = toupper(name[j]);
	}

	output[i] = 0;
}
