#include <fxos.h>
#include <errors.h>
#include <util.h>
#include <string.h>

/* sys_free_item(): Free a syscall description item */
static void sys_free_item(void *item)
{
	struct sys_call *call = item;
	free(call->name);
	free(call->descr);
}

/* sys_load(): Load a syscall table */
void sys_load(char const *file)
{
	err_context("syscall", file, NULL);
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

	/* If the file is named "sys-x.txt", use "x" as the table name */
	char *name = match_table_name(file, "sys", ".txt");

	/* Parse the contents */
	int count;
	struct sys_call *calls = lex_sys(data, size, &count);

	table_create("sys", name, sys_free_item, count, sizeof *calls, calls);

	unmap(data, fd, size);
	err_pop();
}

/* Global storage for the sys_match() function */
static uint32_t sys_match_number;

/* sys_match(): Search routine for sys_find() */
static int sys_match(void *item)
{
	struct sys_call *call = item;
	return call->number == sys_match_number;
}

/* sys_find(): Find information on a given syscall number */
struct sys_call const *sys_find(uint32_t number)
{
	sys_match_number = number;
	return table_find("sys", sys_match, NULL, 0);
}
