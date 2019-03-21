//---
//	fxos:fxos - Main interfaces
//---

#ifndef FX_FXOS
#define FX_FXOS

#include <stdint.h>
#include <stddef.h>

/* Microprocessor platforms */
enum mpu
{
	mpu_unknown = 0,
	mpu_sh7705  = 1,
	mpu_sh7305  = 2,
};


/*
**  Data tables (tables.c)
*/

/* tables_add_asm(): Append an instruction table to the table list
   This function adds a new instruction table to fetch instructions from; it
   will be consulted if searching any of the previously-declared tables fails.

   @filename  Name of instruction table file
   Returns non-zero on error (and prints a message on stderr) */
int tables_add_asm(const char *filename);

/* tables_add_syscall(): Append a syscall table to the table list
   This function adds a new syscall table to fetch syscalls information from;
   if will be consulted if searching any of the previously-declared tables
   fails.

   @filename  Name of instruction table file
   Returns non-zero on error (and prints a message on stderr) */
int tables_add_syscall(const char *filename);


/*
**  RAM dumps (ram.c)
*/

/* Region for a single RAM dump */
struct region
{
	uint32_t start;
	uint32_t length;
	void *data;
};

/* RAM dump */
struct ram_sh7705
{
	struct region RAM;	/* Usual RAM (256k) */
};
struct ram_sh7305
{
	struct region RAM;	/* Usual RAM (512k) */
	struct region IL;	/* On-chip instruction storage (16k) */
	struct region RS;	/* On-chip generic storage (2k) */
};


/*
**  File identification (info.c)
*/

/* Info options */
struct info
{
	/* OS file (0) or binary file (1) */
	int binary;
	/* Force underlying architecture */
	enum mpu mpu;

	/* RAM dumps, if any */
	union {
		struct ram_sh7705 ram3;
		struct ram_sh7305 ram4;
	};
};


/*
**  Disassembling (disassembly.c)
*/

/* Disassembly options */
struct disassembly
{
	/* OS file (0) or binary file (1) */
	int binary;
	/* Force underlying architecture */
	enum mpu mpu;

	/* RAM dumps, if any */
	union {
		struct ram_sh7705 ram3;
		struct ram_sh7305 ram4;
	};

	/* Start address */
	uint32_t start;
	/* Length of disassembled region */
	uint32_t length;
};


/*
**  Blind analysis (analysis.c)
*/

/* Analysis options */
struct analysis
{
	/* Force underlying architecture */
	enum mpu mpu;

	/* RAM dumps, if any */
	union {
		struct ram_sh7705 ram3;
		struct ram_sh7305 ram4;
	};

	/* Analysis mode */
	enum {
		ANALYSIS_FULL = 0,
		ANALYSIS_SYSCALL = 1,
		ANALYSIS_ADDRESS = 2,
		ANALYSIS_REGISTER = 3,
	} type;

	/* Max number of printed occurrences */
	int occurrences;
};


/*
**  Utility functions (util.c)
*/

/* integer(): Convert base 8, 10 or 16 into integers
   Prints an error message and sets *error to 1 in case of conversion error or
   overflow.

   @str    Original integer representation ("10", "0x1f", "07")
   @error  Set to 1 on error, otherwise unchanged (can be NULL)
   Returns result of conversion (valid if *error is not 1) */
long long integer(const char *str, int *error);

#endif /* FX_FXOS */
