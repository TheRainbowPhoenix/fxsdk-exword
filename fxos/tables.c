#include <stdlib.h>
#include <fxos.h>

/* A linked list of pointers (either file names or tables) */
struct element
{
	void *data;
	struct element *next;
};

/* list_append(): Append a pointer to a linked list
   Returns a pointer to the new node, NULL on error. */
struct element *list_append(struct element **head, void *data)
{
	struct element *el = malloc(sizeof *el);
	if(!el) return NULL;
	el->data = data;
	el->next = NULL;

	while(*head) head = &((*head)->next);
	*head = el;

	return el;
}

/* list_free(): Free a linked list */
void list_free(struct element *head, void (*destructor)(void *))
{
	struct element *next;
	while(head)
	{
		destructor(head->data);
		next = head->next;
		free(head);
		head = next;
	}
}

/* Table of assembly instruction listings */
static struct element *tasm = NULL;
/* Table of system call information listings */
static struct element *tcall = NULL;


/*
**  Public API
*/

/* tables_add_asm(): Append an instruction table to the table list */
int tables_add_asm(const char *filename)
{
	return !list_append(&tasm, (void *)filename);
}

/* tables_add_syscall(): Append a syscall table to the table list */
int tables_add_syscall(const char *filename)
{
	return !list_append(&tcall, (void *)filename);
}
