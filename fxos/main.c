#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include <fxos.h>

static const char *help_string =
"usage: %1$s info    (<os file> | -b <binary file>)\n"
"       %1$s disasm  <os file> (-a <address> | -s <syscall id>) [options...]\n"
"       %1$s disasm  -b <binary file> [options...]\n"
"       %1$s analyze [-f|-s|-a|-r] <number> <os file> [options...]\n"
"\n"
"fxos disassembles or analyzes binary and OS files for efficient reverse-\n"
"engineering. It currently only supports fx9860g binaries.\n"
"\n"
"Commands:\n"
"  info     Identify an OS image: version, platform, date, checksums...\n"
"           Identify the architecture of a binary file.\n"
"  disasm   Disassemble and annotate code with relative address targets,\n"
"           syscall invocations and hints about memory structure.\n"
"  analyze  Dig an address or syscall number, finding syscall call graph,\n"
"           4-aligned occurrences, memory region and probable role.\n"
"\n"
"General options:\n"
"  -b                   Disassemble any binary file, not an OS file\n"
"  -3, --sh3            Assume SH3 OS and platform (default: guess)\n"
"  -4, --sh4            Assume SH4 OS and platform (default: guess)\n"
"  --ram <folder>       Read RAM dumps from <folder>\n"
"  --table-asm <file>   Read more instruction patterns in <file>\n"
"  --table-call <file>  Read more syscall prototypes in <file>\n"
"\n"
"Disassembly region options:\n"
"  -a <address>         Start disassembling at this address\n"
"  -s <syscall id>      Start disassembling at this syscall's address\n"
"  -l <length>          Length of region\n"
"\n"
"Analysis modes:\n"
"  -f, --full           Find everything that can be known about <number>\n"
"  -s, --syscall        <number> is a syscall ID\n"
"  -a, --address        <number> is a code/data address in ROM or RAM\n"
"  -r, --register       <number> is a register or peripheral address\n"
"\n"
"Analysis options:\n"
"  --occurrences <num>  Show at most <num> occurrences (integer or \"all\")\n";

/* "Low-level" command-line option set */
struct options
{
	const char *input;

	int target;
	const char *ram;

	const char *a;
	const char *s;
	size_t l;

	int f;
	int r;
	const char *occ;
};

int main(int argc, char **argv)
{
	int command = 0, error = 0;
	struct options opt = { 0 };
	/* For string -> int conversions, first non-int character */


	/* Get command name */
	if(argc >= 2)
	{
		if(!strcmp(argv[1], "info")) command = 'i';
		if(!strcmp(argv[1], "disasm")) command = 'd';
		if(!strcmp(argv[1], "analyze")) command = 'a';

		if(!command && argv[1][0] != '-')
		{
			fprintf(stderr, "invalid operation: '%s'\n", argv[1]);
			fprintf(stderr, "Try '%s --help'.\n", argv[0]);
			return 1;
		}

		if(command) argv[1] = "";
	}

	enum {
		OPT_RAM  = 1,
		OPT_ASM  = 2,
		OPT_CALL = 3,
		OPT_OCC  = 4,
	};
	const struct option longs[] = {
		{ "help",        no_argument,       NULL, 'h'      },
		{ "sh3",         no_argument,       NULL, '3'      },
		{ "sh4",         no_argument,       NULL, '4'      },
		{ "ram",         required_argument, NULL, OPT_RAM  },
		{ "table-asm",   required_argument, NULL, OPT_ASM  },
		{ "table-call",  required_argument, NULL, OPT_CALL },
		{ "full",        no_argument,       NULL, 'f'      },
		{ "syscall",     no_argument,       NULL, 's'      },
		{ "address",     no_argument,       NULL, 'a'      },
		{ "register",    no_argument,       NULL, 'r'      },
		{ "occurrences", required_argument, NULL, OPT_OCC  },
	};

	int option = 0;
	while(option >= 0 && option != '?')
	switch((option = getopt_long(argc, argv, "hb34a::s::l:fr",longs,NULL)))
	{
	case 'h':
		fprintf(stderr, help_string, argv[0]);
		return 0;
	case '3':
	case '4':
		opt.target = option;
		break;
	case OPT_RAM:
		opt.ram = optarg;
		break;
	case OPT_ASM:
		tables_add_asm(optarg);
		break;
	case OPT_CALL:
		tables_add_syscall(optarg);
		break;
	case 'f':
		opt.f = 1;
		break;
	case 's':
		opt.s = optarg;
		break;
	case 'a':
		opt.a = optarg;
		break;
	case 'l':
		opt.l = integer(optarg, &error);
		break;
	case 'r':
		opt.r = 1;
		break;
	case OPT_OCC:
		opt.occ = optarg;
		break;
	case '?':
		error = 1;
		break;
	}

	if(error) return 1;
	opt.input = argv[optind];

	if(!opt.input)
	{
		fprintf(stderr, help_string, argv[0]);
		return 1;
	}

	/* Load default tables (user-specified tables will override them) */
	tables_add_asm    (FXSDK_PREFIX "/share/assembly-sh3.txt");
	tables_add_syscall(FXSDK_PREFIX "/share/syscalls.txt");

	/* Change interpretation of arguments depending on mode */

	printf("Operation is '%c'\n", command);
	printf("  input=%s\n", opt.input);
	printf("  target='%c'\n", opt.target);
	printf("  ram=%s\n", opt.ram);
	printf("  a=%s\n", opt.a);
	printf("  s=%s\n", opt.s);
	printf("  l=%zu\n", opt.l);
	printf("  f=%d\n", opt.f);
	printf("  r=%d\n", opt.r);
	printf("  occ=%s\n", opt.occ);

	/* TODO: Execution procedure:
	   TODO: 1. Identify architecture
	   TODO: 2. Load RAM data, syscall tables, peripheral modules, etc
	   TODO: 3. Execute command */

	return 0;
}
