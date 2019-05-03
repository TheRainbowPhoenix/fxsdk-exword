#include <fxos.h>
#include <stdio.h>

/* analysis_short(): Print a one-line summary for an address */
void analysis_short(struct os const *os, uint32_t value)
{
	/* Find out whether it is a syscall address */
	int syscall = os_syscall_find(os, value);

	if(syscall != -1)
	{
		printf(" %%%03x", syscall);

		/* Also find out the syscall's name! */
		struct sys_call const *call = sys_find(syscall);
		if(call) printf(" %s", call->name);
	}

	/* Find out any peripheral register address */
	struct reg_address const *reg = reg_find(value);
	if(reg) printf(" %s", reg->name);
}
