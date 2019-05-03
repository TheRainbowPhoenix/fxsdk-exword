#include <fxos.h>

/* Shared by all platforms (though ROM is 4M on most) */

static struct region ROM = { 0x80000000, 0x807fffff, "ROM", MPU_GUESS };
static struct region RAM = { 0x88000000, 0x88040000, "RAM", MPU_GUESS };

static struct region P2_ROM = { 0xa0000000, 0xa07fffff, "P2 ROM", MPU_GUESS };
static struct region P2_RAM = { 0xa8000000, 0xa8040000, "P2 RAM", MPU_GUESS };

/* SH7305 only */

static struct region RS   = { 0xfd800000, 0xfd8007ff, "RS",   MPU_SH7305 };
static struct region IL   = { 0xe5200000, 0xe5203fff, "IL",   MPU_SH7305 };
static struct region XRAM = { 0xe5007000, 0xe5008fff, "XRAM", MPU_SH7305 };
static struct region YRAM = { 0xe5017000, 0xe5018fff, "YRAM", MPU_SH7305 };

/* A summary */
static struct region *regions[] = {
	&ROM, &RAM, &P2_ROM, &P2_RAM, &IL, &RS, NULL,
};

/* memory_in(): Check if an address if inside a given region */
static int memory_in(uint32_t address, struct region const *reg)
{
	return reg && address >= reg->start && address <= reg->end;
}

/* memory_region(): Find the region where an address is located */
struct region const *memory_region(uint32_t address)
{
	for(int i = 0; regions[i]; i++)
	{
		if(memory_in(address, regions[i])) return regions[i];
	}

	return NULL;
}
