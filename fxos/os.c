#include <endianness.h>
#include <string.h>

#include <fxos.h>
#include <errors.h>
#include <util.h>

/* os_load(): Load an OS file and find out basic properties */
int os_load(char const *path, struct os *os)
{
	if(!path || !os) return 1;

	os->data = map(path, &os->fd, &os->len);
	if(!os->data) return 1;
	if(os->len < 0x10080)
	{
		err("too small for the OS format I know (min. 0x10080 bytes)",
			os->len);
		return 1;
	}

	/* Get the OS version */

	memcpy(os->version, os->data + 0x10020, 14);
	os->version[14] = 0;

	/* Guess the MPU type (or assume SH7305). Until version 03.00, the last
	   digit of the version number indicated the presence of an SH7305 */

	os->mpu = MPU_SH7305;
	if(!strncmp(os->version, "02.", 3) && os->version[13] == '0')
		os->mpu = MPU_SH7705;

	/* Find the syscall table address */

	memcpy(&os->syscall_table, os->data + 0x1007c, 4);
	os->syscall_table = be32toh(os->syscall_table);

	/* Count the number of valid syscall entries */

	os->syscalls = 0;
	while(memory_region(os_syscall(os, os->syscalls))) os->syscalls++;

	/* Find the footer address (last occurrence of "CASIOABSLangdata") */

	char const *signature = "CASIOABSLangdata";
	void *occ = NULL, *next = memmem(os->data, os->len, signature, 16);
	void *end = os->data + os->len;

	while(next)
	{
		occ = next;
		next = memmem(next + 1, end - (next + 1), signature, 16);
	}

	os->footer = (occ) ? (occ - os->data) : (uint32_t)-1;

	return 0;
}

/* os_syscall(): Get the address of a syscall entry */
uint32_t os_syscall(struct os const *os, int syscall)
{
	uint32_t table_entry = (os->syscall_table & 0x1fffffff) + 4 * syscall;
	if(table_entry + 4 > os->len) return (uint32_t)-1;

	uint32_t address;
	memcpy(&address, os->data + table_entry, 4);
	address = be32toh(address);

	return address;
}

/* os_syscall_find(): Find a syscall which points to an address */
int os_syscall_find(struct os const *os, uint32_t entry)
{
	/* Try to make this slightly quicker */
	uint32_t table_entry = (os->syscall_table & 0x1fffffff);
	entry = htobe32(entry);

	uint32_t *entries = os->data + table_entry;
	for(int i = 0; i < os->syscalls; i++) if(entries[i] == entry) return i;

	return -1;
}

/* os_free(): Free an OS file opened with os_load() */
void os_free(struct os const *os)
{
	if(os && os->data) unmap(os->data, os->fd, os->len);
}
