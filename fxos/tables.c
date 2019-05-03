#include <fxos.h>
#include <errors.h>
#include <string.h>

/* Up to 128 tables can be defined */
#define MAX 128
static struct table db[MAX] = { 0 };
/* Number of tables currently used */
static int db_size = 0;

/* table_free(): Free a table and its contents
   @t  Table pointer */
static void table_free(struct table *t)
{
	if(!t) return;
	free(t->name);

	if(t->items) for(int i = 0; i < t->count; i++)
	{
		t->free_item(t->items + i * t->size);
	}
	free(t->items);
}

/* table_quit(): Destroy all allocated tables and release memory */
__attribute__((destructor))
static void table_quit(void)
{
	for(int i = 0; i < db_size; i++) table_free(&db[i]);
}

/* table_available(): Whether a table can be allocated */
int table_available(void)
{
	return db_size < MAX;
}

/* table_create(): Create a new table */
void table_create(char const *type, char *name, void (*free_item)(void *),
	int count, size_t size, void *items)
{
	if(!table_available()) return;
	struct table *t = &db[db_size++];

	t->type = type;
	t->name = name;
	t->free_item = free_item;
	t->count = count;
	t->size = size;
	t->items = items;
}

/* table_find(): Find matching entries in the database tables */
void *table_find(char const *type, int (*match)(void *), char const **name,
	int next)
{
	static int table = 0;
	static int index = 0;
	if(!next) table = index = 0;

	while(table < db_size)
	{
		struct table const *t = &db[table];

		if(!strcmp(t->type, type))
		while(index < t->count)
		{
			void *entry = t->items + index * t->size;
			index++;
			if(match(entry))
			{
				if(name) *name = t->name;
				return entry;
			}
		}

		table++;
		index = 0;
	}

	return NULL;
}
