#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include <fxos.h>
#include <errors.h>
#include <util.h>

static char const *help_string =
"usage: %1$s info    (<os file> | -b <binary file>)\n"
"       %1$s disasm  <os file> (-a <address> | -s <syscall id>) [options...]\n"
"       %1$s disasm  -b <binary file> [options...]\n"
"       %1$s analyze [-f] [-s] [-a] [-r] <number> <os file> [options...]\n"
"\n"
"fxos is a reverse-engineering tool to disassemble and analyze fx9860g-like\n"
"OS dumps, providing efficient annotations through an editable database.\n"
"\n"
"Commands:\n"
"  info     Identify an OS image: version, platform, date, checksums...\n"
"           Identify the architecture of a binary file.\n"
"  disasm   Disassemble and annotate code with relative address targets,\n"
"           syscall invocations and hints about memory structure.\n"
"  analyze  Dig an address or syscall number, finding syscall references,\n"
"           4-aligned occurrences, memory region and probable role.\n"
"\n"
"General options:\n"
"  -b                   Work with an arbitrary binary file, not an OS\n"
"\n"
"Table extensions:\n"
"  --table-asm <file>   Use instruction codes and mnemonics from <file>\n"
"  --table-sys <file>   Use syscall prototypes and descriptions from <file>\n"
"  --table-reg <file>   Use peripheral register addresses from <file>\n"
"\n"
"Disassembly options:\n"
"  -a <address>         Start disassembling at this address\n"
"  -s <syscall id>      Start disassembling at this syscall's address\n"
"  -l <length>          Length of region\n"
"  -3, --sh3            Assume SH3 OS and platform (default: guess)\n"
"  -4, --sh4            Assume SH4 OS and platform (default: guess)\n"
"\n"
"Analysis modes:\n"
"  -f, --full           Run all analysis passes on <number> (same as -sar)\n"
"  -s, --syscall        Run syscall ID analysis\n"
"  -a, --address        Run code/data address analyis\n"
"  -r, --register       Run peripheral register analysis\n"
"\n"
"Analysis options:\n"
"  --occurrences <num>  Show at most <num> occurrences (integer or \"all\")\n"
"\n"
"All numbers support base prefixes '0' (octal) and '0x' (hexadecimal).\n";

#define OPT_ASM 1
#define OPT_SYS 2
#define OPT_REG 3
#define OPT_OCC 4

/*
**  "info" command
*/

int main_info(int argc, char **argv)
{
	int error=0, option=0, binary=0;

	struct option const longs[] = {
		{ "help",        no_argument,       NULL, 'h'     },
	};

	while(option >= 0 && option != '?')
	switch((option = getopt_long(argc, argv, "hb", longs, NULL)))
	{
	case 'h':
		err(help_string, argv[0]);
		break;
	case 'b':
		binary = 1;
		break;
	case '?':
		error = 1;
	}

	if(error) return 1;
	char const *path = argv[optind + 1];

	if(!path)
	{
		err(help_string, argv[0]);
		return 1;
	}

	err_context(path, NULL);

	struct os os;
	if(os_load(path, &os)) { err_pop(); return 1; }

	if(binary)
	{
		err("TODO: Binary file info x_x");
	}
	else
	{
		info_os(&os);
	}

	os_free(&os);
	err_pop();
	return 0;
}

/*
**  "disasm" command
*/

int main_disassembly(int argc, char **argv)
{
	int error=0, option=0;

	/* First load some of fxos' default resources */
	asm_load(FXSDK_PREFIX "/share/fxsdk/asm-sh3.txt");
	asm_load(FXSDK_PREFIX "/share/fxsdk/asm-sh4.txt");
	sys_load(FXSDK_PREFIX "/share/fxsdk/sys-simlo.txt");
	sys_load(FXSDK_PREFIX "/share/fxsdk/sys-lephe.txt");
	reg_load(FXSDK_PREFIX "/share/fxsdk/reg-sh7305.txt");
	reg_load(FXSDK_PREFIX "/share/fxsdk/reg-simlo.txt");

	struct disassembly opt = { 0 };
	opt.mpu = MPU_GUESS;
	opt.len = 0x20;

	struct option const longs[] = {
		{ "help",        no_argument,       NULL, 'h'     },
		{ "sh3",         no_argument,       NULL, '3'     },
		{ "sh4",         no_argument,       NULL, '4'     },
		{ "table-asm",   required_argument, NULL, OPT_ASM },
		{ "table-sys",   required_argument, NULL, OPT_SYS },
		{ "table-reg",   required_argument, NULL, OPT_REG },
	};

	while(option >= 0 && option != '?')
	switch((option = getopt_long(argc, argv, "hb34a:s:l:", longs, NULL)))
	{
	case 'h':
		err(help_string, argv[0]);
		break;
	case 'b':
		opt.binary = 1;
		break;
	case '3':
		opt.mpu = MPU_SH7705;
		break;
	case '4':
		opt.mpu = MPU_SH7305;
		break;
	case OPT_ASM:
		asm_load(optarg);
		break;
	case OPT_SYS:
		printf("TODO: Load additional syscall tables x_x\n");
		break;
	case OPT_REG:
		printf("TODO: Load additional register tables x_x\n");
		break;
	case 'a':
	case 's':
		opt.start = integer(optarg, &error);
		opt.syscall = (option == 's');
		break;
	case 'l':
		opt.len = integer(optarg, &error);
		break;
	case '?':
		error = 1;
	}

	if(error) return 1;
	char const *path = argv[optind + 1];

	if(!path)
	{
		err(help_string, argv[0]);
		return 1;
	}

	err_context(path, NULL);

	if(opt.binary)
	{
		printf("TODO: Disassembly binary x_x");
		err_pop();
		return 1;
	}

	struct os os;
	if(os_load(path, &os)) { err_pop(); return 1; }

	disassembly_os(&os, &opt);

	os_free(&os);
	err_pop();
	return 0;
}

/*
**  "analyze" command
*/

int main_analyze(int argc, char **argv)
{
	int error=0, option=0;
	struct analysis opt = { 0 };

	struct option const longs[] = {
		{ "help",        no_argument,       NULL, 'h'     },
		{ "table-asm",   required_argument, NULL, OPT_ASM },
		{ "table-sys",   required_argument, NULL, OPT_SYS },
		{ "table-reg",   required_argument, NULL, OPT_REG },
		{ "full",        no_argument,       NULL, 'f'     },
		{ "syscall",     no_argument,       NULL, 's'     },
		{ "address",     no_argument,       NULL, 'a'     },
		{ "register",    no_argument,       NULL, 'r'     },
		{ "occurrences", required_argument, NULL, OPT_OCC },
	};

	while(option >= 0 && option != '?')
	switch((option = getopt_long(argc,argv,"hfsar", longs, NULL)))
	{
	case 'h':
		fprintf(stderr, help_string, argv[0]);
		return 0;
	case OPT_ASM:
		asm_load(optarg);
		break;
	case OPT_SYS:
//		sys_load(optarg);
		break;
	case OPT_REG:
//		reg_load(optarg);
		break;
	case 'f':
		opt.type = ANALYSIS_FULL;
		break;
	case 's':
		opt.type |= ANALYSIS_SYSCALL;
		break;
	case 'a':
		opt.type |= ANALYSIS_ADDRESS;
		break;
	case 'r':
		opt.type |= ANALYSIS_REGISTER;
		break;
	case OPT_OCC:
		opt.occurrences = integer(optarg, &error);
		break;
	case '?':
		error = 1;
		break;
	}

	if(error) return 1;

	/* If no analysis mode was specified, do everything */
	if(!opt.type) opt.type = ANALYSIS_FULL;

	return 0;
}

/*
**  Program entry
*/

int main(int argc, char **argv)
{
	if(argc < 2)
	{
		err(help_string, argv[0]);
		return 1;
	}

	char *cmd = argv[1];
	argv[1] = "";

	/* Switch on command name */
	if(!strcmp(cmd, "info"))
		return main_info(argc, argv);
	else if(!strcmp(cmd, "disasm"))
		return main_disassembly(argc, argv);
	else if(!strcmp(cmd, "analyze"))
		return main_analyze(argc, argv);

	else if(!strcmp(cmd, "-?") || !strcmp(cmd, "-h") ||
		!strcmp(cmd, "--help"))
	{
		err(help_string, argv[0]);
		return 0;
	}

	err("invalid operation '%s'", cmd);
	err("Try '%s --help'.", argv[0]);
	return 1;
}
