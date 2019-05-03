#include <fxos.h>
#include <errors.h>

#include <stdio.h>
#include <string.h>
#include <endian.h>

char const *info_str =
"Header information:\n"
"  Bootcode timestamp (DateA)                 (0x8000ffb0)  :  %s\n"
"  Serial number                              (0x8000ffd0)  :  %s\n"
"  Bootcode checksum                          (0x8000fffc)  :  0x%s\n"
"  OS version                                 (0x80010020)  :  %s\n";

char const *footer_str =
"\nFooter information:\n"
"  Detected footer address                                  :  0x8%07x\n"
"  Langdata entries found                                   :  %d\n"
"  OS date (DateO)                            (0x8%07x)" "  :  %s\n"
"  OS checksum                                (0x8%07x)" "  :  0x%s\n";

char const *syscall_str =
"\nSyscall information:\n"
"  Syscall table address                      (0x8001007c)  :  0x%08x\n"
"  Entries that point to valid memory                       :  0x%x\n"
"  First seemingly invalid entry                            :  0x%08x\n"
"  Syscall entries outside ROM:\n";

char const *syscall_nonrom_str =
"    %%%04x -> 0x%08x (%s memory)\n";

/* extract_str(): Extract a string from the OS file
   This function checks offsets and bounds and writes "(eof)" if the provided
   file is too small for the tested offset. */
static void extract_str(struct os const * os, uint32_t address, size_t size,
	char *dst)
{
	if(address + size > os->len)
	{
		strcpy(dst, "(eof)");
		return;
	}

	memcpy(dst, os->data + address, size);
	dst[size] = 0;
}

/* extract_int(): Extract a string from the OS file */
static void extract_int(struct os const *os, uint32_t address, char *dst)
{
	if(address + 4 > os->len)
	{
		strcpy(dst, "(eof)");
		return;
	}

	uint32_t x;
	memcpy(&x, os->data + address, 4);
	x = be32toh(x);
	sprintf(dst, "%08x", x);
}

/* info_os(): Print general information on an OS file */
void info_os(struct os const *os)
{
	void *data = os->data;
	size_t len = os->len;

	char bios_timestamp[15];
	extract_str(os, 0xffb0, 14, bios_timestamp);

	char serial[9];
	extract_str(os, 0xffd0, 8, serial);

	char bios_checksum[9];
	extract_int(os, 0xfffc, bios_checksum);

	printf(info_str, bios_timestamp, serial, bios_checksum, os->version);

	if(os->footer == (uint32_t)-1)
	{
		printf("\nFooter could not be found.\n");
	}
	else
	{
		uint32_t addr = os->footer + 8;
		int langdata = 0;

		while(addr + 8 < len && !memcmp(data + addr, "Langdata", 8))
		{
			langdata++;
			addr += 0x30;
		}

		char os_timestamp[15];
		extract_str(os, addr, 14, os_timestamp);

		char os_checksum[9];
		extract_int(os, addr + 0x18, os_checksum);

		printf(footer_str, os->footer, langdata, addr, os_timestamp,
			addr + 0x18, os_checksum);
	}

	printf(syscall_str, os->syscall_table, os->syscalls,
		os_syscall(os, os->syscalls));

	int total = 0;
	for(int i = 0; i < os->syscalls; i++)
	{
		uint32_t e = os_syscall(os, i);
		struct region const *r = memory_region(e);
		if(!r || !strcmp(r->name, "ROM")) continue;

		printf(syscall_nonrom_str, i, e, r->name);
		total++;
	}

	if(!total) printf("    (none)\n");
}
