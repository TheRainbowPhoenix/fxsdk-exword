//---
//	fxos:fxos - Main interfaces
//---

#ifndef FX_FXOS
#define FX_FXOS

#include <stdint.h>
#include <stdlib.h>

/* Microprocessor platforms */
enum mpu {
	MPU_GUESS  = 0,
	MPU_SH7705 = 1,
	MPU_SH7305 = 2,
};


/*
**  Memory (memory.c)
*/

/* struct region: A valid memory region for at least one platform */
struct region
{
	uint32_t start;		/* Start address */
	uint32_t end;		/* End address */
	char const *name;	/* Name, used to identify RAM dump files */
	enum mpu platform;	/* Platform hint or MPU_GUESS */
};

/* memory_region(): Find the region where an address is located
   Returns NULL if the address points to no valid memory. This function rejects
   addresses of peripheral registers in P3 or P4 space and only heeds for
   contiguous RAM or ROM areas.

   @address  32-bit virtual memory address
   Returns a region description matching the address, NULL if none is known.*/
struct region const *memory_region(uint32_t address);

/*
**  General table storage (tables.c)
*/

/* struct table: Parametric data table */
struct table {
	char const *type;		/* Table type, set by user */
	char *name;			/* Table name, also set by user */
	void (*free_item)(void *);	/* Function to free individual items */

	int count;			/* Number of items in the table */
	size_t size;			/* Size of each item */
	void *items;			/* Table data */
};

/* table_available(): Whether a table can be allocated
   Returns non-zero if there is space left for a new table, zero if not. */
int table_available(void);

/* table_create(): Create a new table
   Allocates a new table inside the global storage. The created table can be
   searched immediately with table_find().

   @type       Table type, expectedly a string constant
   @name       Name string, this module takes ownership and will free() it
   @free_item  Function to use to destroy items in the future
   @count      Number of items
   @size       Size of each item
   @items      Full data array */
void table_create(char const *type, char *name, void (*free_item)(void *),
	int count, size_t size, void *items);

/* table_find(): Find matching entries in the database tables

   This function traverses all the tables of type @type and returns all the
   elements [e] such that [match(e)] is non-zero.

   The search starts with [next=0] and returns the first match; further calls
   with [next!=0] will return more matching elements until no more are found
   (in which case this function returns NULL) or this function is called again
   with [next=0] to start a new search.

   @type   Table filter by type
   @match  Match function
   @name   Set to matching table name, if not NULL
   @next   Set it to 0 on the first call, and non-zero after that
   Returns a match if one is found, NULL otherwise. */
void *table_find(char const *type, int (*match)(void *), char const **name,
	int next);


/*
**  Assembly tables (asm.c, lexer-asm.l)
**  These tables reference all assembler instructions used by fxos to
**  disassemble code. In case of conflict, fxos will disassemble the same
**  opcode several times.
*/

/* struct asm_insn: Entry of an instruction table */
struct asm_insn {
	uint16_t bits;		/* Opcode; arbitrary values for arguments */
	uint16_t arg_mask;	/* 1 for constant bits, 0 for arguments */

	/* Position of the arguments */
	uint8_t n_sh, m_sh, d_sh, i_sh;
	/* Masks indicating the length of arguments */
	uint16_t n_mask, m_mask, d_mask, i_mask;

	char *mnemonic;		/* NUL-terminated mnemonic */

	int arg1;		/* asm_arg member */
	int arg2;		/* asm_arg member */

	char *literal1;		/* When arg1 == LITERAL, argument string */
	char *literal2;		/* When arg2 == LITERAL, argument string */
};

/* enum asm_arg: Argument variants */
enum asm_arg {
	LITERAL=1,		/* Literal string, eg. "vbr" or "@(r0, gbr)" */
	IMM, RN, RM,		/* "#imm", "rn", "rm" */
	JUMP8, JUMP12,		/* Jump from displacement (PC + disp) */
	PCDISP,			/* PC-displacement with data */
	AT_RN, AT_RM,		/* "@rn", "@rm" */
	AT_RMP, AT_RNP, AT_MRN,	/* Post-increment and pre-decrement */
	AT_DRN, AT_DRM,		/* Displacement structure addressing */
	AT_R0RN, AT_R0RM,	/* r0 structure addressing */
	AT_DGBR,		/* GBR addressing */
};

/* struct asm_match: Matching of a 16-bit code against an instruction
   Specifies the source instruction and the value of the parameters. The value
   of [m], [n], [d] or [i] is unspecified for parameters that are not used in
   the matched instruction's opcode. */
struct asm_match {
	/* Matching instruction */
	struct asm_insn const *insn;

	char const *table;	/* Table name */
	int m, n, d, i;		/* Parameter assignment */
};

/* asm_load(): Load an assembly table
   Loads all instructions described by @file into a table named "x" is @file's
   basename is on the form "asm-x.txt" and the whole basename otherwise. The
   resulting table is available immediately to use with asm_match().

   Skips every row that does not conform to the file's syntax after printing a
   message to stderr.

   @file  Input file path */
void asm_load(char const *file);

/* asm_decode(): Match a 16-bit opcode against the assembly database
   This function searches matches of a 16-bit instruction code inside the
   instruction database. Depending on the database files currently loaded,
   there can be several matches; this function uses static variables to
   maintain state information through several calls.

   First call this function with [next] set to 0. If there is no match, the
   call will return non-zero and [*match] will be left unchanged. Otherwise,
   the first matching instruction will be described in [*match], the call will
   return 0 and internal static variables will be reset.

   Repeatedly call this function with [next != 0] to get further matches. The
   search ends when there are no more matches, in which case this function
   returns non-zero and [*match] is left unchanged. Any non-zero value for
   [next] is suitable.

   The name of the table providing the match is set in [match->table]. Please
   bear in mind, though, that table names do not uniquely identify tables.

   @opcode  16-bit opcode to be matched against the database
   @match   Set to a description of the matching instruction (must not be NULL)
   @next    Set it to 0 on the first call, and non-zero after that
   Returns 0 if a match is found, non-zero otherwise. */
int asm_decode(uint16_t opcode, struct asm_match *match, int next);

/* asm_quit(): Unload all assembly tables
   Releases all memory held by the assembly table database. */
void asm_quit(void);

/* lex_asm(): Assembly table lexer and parser
   Lexes and parses string @data of length @length, allocating and filling an
   instruction array whose size is stored in [*count] if @count is non-NULL.
   Prints messages to stderr for every syntax error in the file.

   @data    Input memory, not NUL-terminated (typically a memory-mapped file)
   @length  Length of input string
   @count   Set to number of successfully decoded instructions if non-NULL
   Returns a free()able array of decoded instructions. */
struct asm_insn *lex_asm(void *data, size_t length, int *count);


/*
**  Syscall tables (sys.c)
*/

/* struct sys_call: Entry of a syscall table */
struct sys_call {
	uint32_t number;	/* Syscall number */
	char *name;		/* Symbol or function name */
	char *descr;		/* Prototype or description */
};

/* sys_load(): Load a syscall table
   Loads a syscall description table. If @file is named "sys-x.txt" then the
   table name is set to "x", otherwise @file's basename.
   Prints syntax errors and skips invalid lines.

   @file  Input file path */
void sys_load(char const *path);

/* sys_find(): Find information on a given syscall number
   Traverse the syscall tables currently loaded and returns the first entry
   matching the provided syscall number, if any is found.

   @number  Syscall number
   Returns a description of the syscall, NULL if none was found. */
struct sys_call const *sys_find(uint32_t number);

/* sys_quit(): Release memory held by the syscall tables */
void sys_quit(void);

/* lex_sys(): Syscall table lexer and parser
   Lexes and parses string @data of length @len, allocating and filling a
   syscall array whose size is stored in [*count] if @count is not NULL.
   Prints syntax errors on stderr.

   @data   Input memory (typically memory-mapped file)
   @len    Length of input
   @count  Set to number of decoded entries if not NULL
   Returns a free()able table of decoded syscall entries. */
struct sys_call *lex_sys(void *data, size_t len, int *count);


/*
**  Peripheral register tables (reg.c)
*/

/* struct reg_address: Entry of a peripheral register table */
struct reg_address {
	uint32_t address;	/* Register address */
	char *name;		/* Typically an upper-case dotted specifier */
};


/* reg_load(): Load a peripheral register table
   Loads a peripheral register listing. If @file is named "reg-x.txt" then the
   table name is set to "x", otherwise @file's basename.
   Prints syntax errors and skips invalid lines. Loaded data is available
   immediately through reg_find().

   @file  Input file path */
void reg_load(char const *path);

/* reg_find(): Find information on a given peripheral register address
   Looks up the loaded tables and returns the first entry matching the given
   address (if any).

   @address  Any input address
   Returns a pointer to the matching register, NULL if none was found. */
struct reg_address const *reg_find(uint32_t address);

/* reg_quit(): Release memory held by the peripheral register tables */
void reg_quit(void);

/* lex_reg(): Peripheral register table lexer and parser
   Lexes and parses @data (of length @length). Allocates and fills a register
   description array and stores its size in [*count] if @count is not NULL.
   Prints messages on stderr if there are syntax errors.

   @data   Input string (needs not be NUL-terminated)
   @len    Length of input
   @count  Set to the number of decoded register addresses, if not NULL
   Returns a free()able table with the decoded data. */
struct reg_address *lex_reg(void *data, size_t len, int *count);


/*
**  General OS operations (os.c)
*/

/* struct os: Basic OS information */
struct os {
	void *data;		/* Operating system dump */
	size_t len;		/* File length */
	int fd;			/* Underlying file descriptor */

	char version[15];	/* NUL-terminated OS version string */
	enum mpu mpu;		/* User-provided or guessed MPU type */

	uint32_t syscall_table;	/* Syscall table address */
	int syscalls;		/* Number of valid syscalls found */

	uint32_t footer;	/* Footer address (-1 if not found) */
};

/* os_load(): Load an OS file and find out basic properties
   Guesses the MPU type, finds the syscall table and its size, finds the footer
   address.

   @path  File path
   @os    Will be filled with loaded data and information
   Returns non-zero in case of loading error or file format error. */
int os_load(char const *path, struct os *os);

/* os_syscall(): Get the address of a syscall entry
   Does not check bounds, only returns (uint32_t)-1 if the requested entry of
   the table is past the end of the file.

   @os       Source OS
   @syscall  Syscall entry number
   Returns the syscall address. */
uint32_t os_syscall(struct os const *os, int syscall);

/* os_syscall_find(): Find a syscall which points to an address
   This function looks for a syscall entry (among the ones that point to valid
   memory) whose value is @entry.

   @os     Loaded OS structure
   @entry  Researched value
   Returns a syscall ID if some is found, -1 otherwise. */
int os_syscall_find(struct os const *os, uint32_t entry);

/* os_free(): Free an OS file opened with os_load()
   @os  Loaded OS structure */
void os_free(struct os const *os);


/*
**  File identification (info.c)
*/

/* info_os(): Print general information on an OS file

   This function prints the OS metadata, traverses the syscall table, and
   shows a few details of known binary regions such as the footer.

   @os   Input OS file */
void info_os(struct os const *os);

/* info_binary(): Print general information on a binary file

   This function tries to determine the platform by looking for SH4-only
   instructions or SH7705 and SH7305-specific registers addresses. The analysis
   results are printed on stdout.

   @data  Input file data (memory-mapped)
   @len   Length of input */
void info_binary(void *data, size_t len);


/*
**  Disassembling (disassembly.c)
*/

/* struct disassembly: Disassembly options */
struct disassembly
{
	int binary;		/* OS file (0) or binary file (1) */
	enum mpu mpu;		/* Force architecture (or MPU_GUESS) */

	uint32_t start;		/* Start address or syscall ID */
	int syscall;		/* Non-zero if [start] is a syscall ID */
	uint32_t len;		/* Length of disassembled region */
};

/* disassembly_os(): Disassemble an address or a syscall
   Produces a disassembly listing of the program on stdout, annotated with
   every piece of information that can be extracted from the OS.

   @os   Operating system image to disassemble
   @opt  Disassembly region and options */
void disassembly_os(struct os const *os, struct disassembly const *opt);


/*
**  Blind analysis (analysis.c)
*/

/* analysis_short(): Print a one-line summary for an address
   Prints a list of space-separated elements summarizing the information that
   can be found about the provided value (typically an address). This summary
   is often inserted in disassembled code as annotation.

   @os     Source OS
   @value  Analyzed value, often an address */
void analysis_short(struct os const *os, uint32_t value);

/* struct analysis: In-depth analysis options */
struct analysis
{
	/* Force underlying architecture */
	enum mpu mpu;

	/* Analysis mode */
	enum {
		ANALYSIS_SYSCALL  = 0x01,
		ANALYSIS_ADDRESS  = 0x02,
		ANALYSIS_REGISTER = 0x04,

		ANALYSIS_FULL     = 0x07,
	} type;

	/* Max number of printed occurrences */
	int occurrences;
};

#endif /* FX_FXOS */
