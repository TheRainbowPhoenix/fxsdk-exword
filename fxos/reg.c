#include <fxos.h>
#include <errors.h>
#include <string.h>
#include <util.h>

/* reg_free_item(): Free a peripheral register item */
static void reg_free_item(void *item)
{
	struct reg_address *reg = item;
	free(reg->name);
}

/* reg_load(): Load a peripheral register table */
void reg_load(char const *file)
{
	err_context("register", file, NULL);
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

	/* If the file is named "reg-x.txt", use "x" as the table name */
	char *name = match_table_name(file, "reg", ".txt");

	/* Parse the contents */
	int count;
	struct reg_address *regs = lex_reg(data, size, &count);

	table_create("reg", name, reg_free_item, count, sizeof *regs, regs);

	unmap(data, fd, size);
	err_pop();
}

/* Global storage for reg_match() */
static uint32_t reg_match_address;

/* reg_match(): Search routine for reg_find() */
static int reg_match(void *item)
{
	struct reg_address *reg = item;
	return reg->address == reg_match_address;
}

/* reg_find(): Find information on a given peripheral register address */
struct reg_address const *reg_find(uint32_t address)
{
	reg_match_address = address;
	return table_find("reg", reg_match, NULL, 0);
}
