#include <fxos.h>
#include <errors.h>
#include <util.h>
#include <string.h>

/* asm_free_item(): Free an assembly instruction details */
static void asm_free_item(void *item)
{
	struct asm_insn *insn = item;
	free(insn->mnemonic);
	free(insn->literal1);
	free(insn->literal2);
}

/* asm_load(): Load an assembly table */
void asm_load(char const *file)
{
	err_context("assembly", file, NULL);
	if(!table_available())
	{
		err("too many tables, skipping");
		err_pop();
		return;
	}

	/* Map the file to memory */
	int fd;
	size_t size;
	void *data = map(file, &fd, &size);
	if(!data) { err_pop(); return; }

	/* If the file is named "asm-x.txt", use "x" as the table name */
	char *name = match_table_name(file, "asm", ".txt");

	/* Parse the contents; the lexer in is [lexer-asm.l] and the parser is
	   implemented as an automaton with basic error correction. */
	int count;
	struct asm_insn *ins = lex_asm(data, size, &count);

	table_create("asm", name, asm_free_item, count, sizeof *ins, ins);

	unmap(data, fd, size);
	err_pop();
}

/* Global storage for asm_match() */
static uint16_t asm_match_opcode;

/* asm_match(): Check if an instruction matches a pattern */
int asm_match(void *item)
{
	struct asm_insn *insn = item;
	return ((insn->bits ^ asm_match_opcode) & insn->arg_mask) == 0;
}

/* asm_decode(): Match a 16-bit opcode against the assembly database */
int asm_decode(uint16_t opcode, struct asm_match *match, int next)
{
	asm_match_opcode = opcode;
	char const *table;
	struct asm_insn *insn = table_find("asm", asm_match, &table, next);
	if(!insn) return 1;

	/* Set match details */
	match->insn = insn;
	match->table = table;

	match->n = (opcode >> insn->n_sh) & insn->n_mask;
	match->m = (opcode >> insn->m_sh) & insn->m_mask;
	match->d = (opcode >> insn->d_sh) & insn->d_mask;
	match->i = (opcode >> insn->i_sh) & insn->i_mask;

	return 0;
}
