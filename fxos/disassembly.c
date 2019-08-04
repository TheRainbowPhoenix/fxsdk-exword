#include <fxos.h>
#include <errors.h>

#include <stdio.h>
#include <string.h>

/* Architecture we are disassembling for */
static enum mpu mpu = MPU_GUESS;
/* Current program counter */
static uint32_t pc = 0;
/* Data chunk under disassembly (whole file) */
static uint8_t *data;
static size_t len;
/* Non-NULL when disassembling an OS */
static struct os const *os = NULL;

/* pcdisp(): Compute the address of a PC-relative displacement
   @pc    Current PC value
   @disp  Displacement value (from opcode)
   @unit  Number of bytes per step (1, 2 or 4), depends on instruction
   Returns the pointed location. */
static uint32_t pcdisp(uint32_t pc, uint32_t disp, int unit)
{
	pc += 4;
	if(unit == 4) pc &= ~3;

	return pc + disp * unit;
}

/* pcread(): Read data pointed through a PC-relative displacement
   @pc    Current PC value
   @disp  Displacement value (from opcode)
   @unit  Number of bytes per set (also number of bytes read)
   @out   If non-NULL, set to 1 if pointed address is out of the file
   Returns the value pointed at, an undefined value if out of bounds. */
static uint32_t pcread(uint32_t pc, uint32_t disp, int unit, int *out)
{
	/* Take physical addresses */
	uint32_t addr = pcdisp(pc, disp, unit) & 0x1fffffff;
	if(out) *out = (addr + unit > len);

	uint32_t value = 0;
	while(unit--) value = (value << 8) | data[addr++];
	return value;
}

/* matches(): Count number of matches for a single instruction
   @opcode  16-bit opcode value
   Returns the number of matching instructions in the database. */
static int matches(uint16_t opcode)
{
	struct asm_match match;
	int count = 0;

	while(!asm_decode(opcode, &match, count))
	{
		count++;
	}
	return count;
}

static void arg_output(int type, char const *literal,
	struct asm_match const *match, int opsize)
{
	int n = match->n;
	int m = match->m;
	int d = match->d;
	int i = match->i;

	/* Sign extensions of d to 8 and 12 bits */
	int32_t d8  = (int8_t)d;
	int32_t d12 = (d & 0x800) ? (int32_t)(d | 0xfffff000) : (d);
	/* Sign extension of i to 8 bits */
	int32_t i8  = (int8_t)i;

	int out_of_bounds;
	uint32_t addr;
	uint32_t value;

	switch(type)
	{
	case LITERAL:
		printf("%s", literal);
		break;
	case IMM:
		printf("#%d", i8);
		break;
	case RN:
		printf("r%d", n);
		break;
	case RM:
		printf("r%d", m);
		break;
	case JUMP8:
		value = pcdisp(pc, d8, 2);
		printf("<%x", value);
		if(os) analysis_short(os, value | 0x80000000);
		printf(">");
		break;
	case JUMP12:
		value = pcdisp(pc, d12, 2);
		printf("<%x", value);
		if(os) analysis_short(os, value | 0x80000000);
		printf(">");
		break;
	case PCDISP:
		addr  = pcdisp(pc, d, opsize);
		value = pcread(pc, d, opsize, &out_of_bounds);

		printf("<%x>", addr);
		if(out_of_bounds) printf("(out of bounds)");
		else
		{
			printf("(#0x%0*x", opsize * 2, value);
			if(os) analysis_short(os, value);
			printf(")");
		}

		break;
	case AT_RN:
		printf("@r%d", n);
		break;
	case AT_RM:
		printf("@r%d", m);
		break;
	case AT_RMP:
		printf("@r%d+", m);
		break;
	case AT_RNP:
		printf("@r%d+", n);
		break;
	case AT_MRN:
		printf("@-r%d", n);
		break;
	case AT_DRN:
		printf("@(%d, r%d)", d * opsize, n);
		break;
	case AT_DRM:
		printf("@(%d, r%d)", d * opsize, m);
		break;
	case AT_R0RN:
		printf("@(r0, r%d)", n);
		break;
	case AT_R0RM:
		printf("@(r0, r%d)", m);
		break;
	case AT_DGBR:
		printf("@(%d, gbr)", d * opsize);
		break;
	}
}

static void instruction_output(uint16_t opcode, struct asm_match const *match)
{
	char const *mnemonic = match->insn->mnemonic;
	printf(" %5x:  %04x   %s", pc, opcode, mnemonic);

	/* Find out operation size */

	size_t n = strlen(mnemonic);
	int opsize = 0;

	if(n >= 3 && mnemonic[n-2] == '.')
	{
		int c = mnemonic[n-1];
		opsize = (c == 'b') + 2 * (c == 'w') + 4 * (c == 'l');
	}

	/* Output arguments */

	if(!match->insn->arg1) return;
	printf("%*s", (int)(8-n), "");
	arg_output(match->insn->arg1, match->insn->literal1, match, opsize);

	if(!match->insn->arg2) return;
	printf(", ");
	arg_output(match->insn->arg2, match->insn->literal2, match, opsize);
}

static void instruction_single(uint16_t opcode)
{
	struct asm_match match;
	asm_decode(opcode, &match, 0);
	instruction_output(opcode, &match);
	printf("\n");
}

static void instruction_conflicts(uint16_t opcode, int count)
{
	struct asm_match match;
	printf("\n # conflicts[%d] on <%x>(%04x)\n", count, pc, opcode);

	for(int i = 0; i < count; i++)
	{
		asm_decode(opcode, &match, i);
		instruction_output(opcode, &match);
		printf("    # table '%s'\n", match.table);
	}

	printf("\n");
}

/* disassembly_os(): Disassemble an address or a syscall */
void disassembly_os(struct os const *src, struct disassembly const *opt)
{
	/* Initialize this file's global state */
	mpu  = src->mpu;
	pc   = opt->start;
	data = src->data;
	len  = src->len;
	os   = src;

	/* Override MPU guesses if user requested a specific platform */
	if(opt->mpu != MPU_GUESS) mpu = opt->mpu;

	/* Handle situations where the start address is a syscall */
	if(opt->syscall)
	{
		pc = os_syscall(os, opt->start);
		if(pc == (uint32_t)-1)
		{
			err("syscall 0x%04x does not exist", opt->start);
			return;
		}
	}

	/* Take physical addresses */
	pc &= 0x1fffffff;

	/* Cut the length if it reaches past the end of the file */
	uint32_t limit = (pc + opt->len) & ~1;
	if(limit > os->len) limit = os->len;

	/* Enforce alignment of PC */
	if(pc & 1)
	{
		err("address is not 2-aligned, skipping 1 byte");
		pc += 1;
	}

	uint8_t *data = os->data;

	while(pc < limit)
	{
		if(os)
		{
			int syscall = os_syscall_find(os, pc | 0x80000000);
			if(syscall == -1)
				syscall = os_syscall_find(os, pc | 0xa0000000);
			if(syscall != -1)
			{
				struct sys_call const *s = sys_find(syscall);
				printf("\n<%x %%%03x", pc, syscall);
				if(s) printf(" %s", s->name);
				printf(">\n");
			}
		}

		uint16_t opcode = (data[pc] << 8) | data[pc + 1];
		int count = matches(opcode);

		if(count == 0) printf(" %5x:  %04x\n", pc, opcode);
		else if(count == 1) instruction_single(opcode);
		else instruction_conflicts(opcode, count);

		pc += 2;
	}
}
