#include <stdio.h>
#include <string.h>
#include <time.h>
#include <getopt.h>

#include <fxg1a.h>
#include <g1a.h>

static const char *help_string =
"usage: %1$s [-g] <binary file> [options...]\n"
"       %1$s  -e  <g1a file> [options...]\n"
"       %1$s  -d  <g1a file>\n"
"       %1$s  -r  <g1a file> [-o <g1a file>]\n"
"       %1$s  -x  <g1a file> [-o <png file>]\n"
"\n"
"fxg1a creates or edits g1a files (add-in applications for Casio fx9860g\n"
"calculator series) that consist of a g1a header followed by binary code.\n"
"\n"
"Operating modes:\n"
"  -g, --g1a             Generate a g1a file (default)\n"
"  -e, --edit            Edit header of an existing g1a file\n"
"  -d, --dump            Dump header of an existing g1a file\n"
"  -r, --repair          Recalculate control bytes and checksums\n"
"  -x, --extract         Extract icon into a PNG file\n"
"\n"
"General options:\n"
"  -o, --output=<file>   Output file (default: input file with .g1a suffix\n"
"                        [-g]; with .png suffix [-x]; input file [-e, -r])\n"
"\n"
"Generation and edition options:\n"
"  -i, --icon=<png>      Program icon, in PNG format (default: blank icon)\n"
"  -n, --name=<name>     Add-in name, 8 bytes (default: output file name)\n"
"  --version=<text>      Program version, MM.mm.pppp format (default: empty)\n"
"  --internal=<name>     Internal name, eg. '@NAME' (default: empty)\n"
"  --date=<date>         Date of build, yyyy.MMdd.hhmm (default: now)\n";

/*
**  Field customization
*/

/* A set of user-defined fields, often taken on the command-line
   Default values are NULL and indicate "no value" (-g) or "no change" (-e). */
struct fields
{
	/* New values for basic fields */
	const char *name;
	const char *version;
	const char *internal;
	const char *date;
	/* Icon file name */
	const char *icon;
};

/* fields_edit(): Set the value of some fields altogether
   @header  Header to edit, is assumed checksumed and filled
   @fields  New values for fields, any members can be NULL */
void fields_edit(struct g1a *header, struct fields const *fields)
{
	/* For easy fields, just call the appropriate edition function */
	if(fields->name)     edit_name(header, fields->name);
	if(fields->version)  edit_version(header, fields->version);
	if(fields->internal) edit_internal(header, fields->internal);
	if(fields->date)     edit_date(header, fields->date);

	/* Load icon from PNG file */
	if(fields->icon)
	{
		size_t width, height;
		uint8_t *data = icon_load(fields->icon, &width, &height);
		if(!data) return;

		uint8_t *mono = icon_conv_8to1(data, width, height);
		free(data);

		if(!mono) return;
		edit_icon(header, mono);
	}
}

/*
**  Tool implementation
*/

int main(int argc, char **argv)
{
	/* Result of option parsing */
	int mode = 'g', error = 0;
	struct fields fields = { 0 };
	const char *output = NULL;

	const struct option longs[] = {
		{ "help",     no_argument,       NULL, 'h' },
		{ "g1a",      no_argument,       NULL, 'g' },
		{ "edit",     no_argument,       NULL, 'e' },
		{ "dump",     no_argument,       NULL, 'd' },
		{ "repair",   no_argument,       NULL, 'r' },
		{ "extract",  no_argument,       NULL, 'x' },
		{ "output",   required_argument, NULL, 'o' },
		{ "icon",     required_argument, NULL, 'i' },
		{ "name",     required_argument, NULL, 'n' },
		{ "version",  required_argument, NULL, 'v' },
		{ "internal", required_argument, NULL, 't' },
		{ "date",     required_argument, NULL, 'a' },
		{ NULL,       0,                 NULL, 0   },
	};

	int option = 0;
	while(option >= 0 && option != '?')
	switch((option = getopt_long(argc, argv, "hgedrxo:i:n:", longs, NULL)))
	{
	case 'h':
		fprintf(stderr, help_string, argv[0]);
		return 0;
	case 'g':
	case 'e':
	case 'd':
	case 'r':
	case 'x':
		mode = option;
		break;
	case 'o':
		output = optarg;
		break;
	case 'i':
		fields.icon = optarg;
		break;
	case 'n':
		fields.name = optarg;
		break;
	case 'v':
		fields.version = optarg;
		break;
	case 't':
		fields.internal = optarg;
		break;
	case 'a':
		fields.date = optarg;
		break;
	case '?':
		error = 1;
		break;
	}

	if(error) return 1;

	if(argv[optind] == NULL)
	{
		fprintf(stderr, help_string, argv[0]);
		return 1;
	}
	if(mode == 'g')
	{
		/* Load binary file into memory */
		size_t size;
		struct g1a *g1a = load_binary(argv[optind], &size);
		if(!g1a) return 1;

		/* If [output] is set, use it, otherwise compute a default */
		char *alloc = NULL;
		if(!output)
		{
			alloc = malloc(strlen(argv[optind]) + 5);
			if(!alloc) {fprintf(stderr, "error: %m\n"); return 1;}
			default_output(argv[optind], ".g1a", alloc);
		}

		/* Start with output file name as application name */
		edit_name(g1a, output ? output : alloc);

		/* Start with "now" as build date */
		char date[15];
		time_t t = time(NULL);
		struct tm *now = localtime(&t);
		strftime(date, 15, "%Y.%m%d.%H%M", now);
		edit_date(g1a, date);

		/* Edit the fields with user-customized values */
		fields_edit(g1a, &fields);

		/* Set fixed fields and calculate checksums */
		sign(g1a, size);

		save_g1a(output ? output : alloc, g1a, size);
		free(alloc);

		/* Write output file */
		free(g1a);
	}
	if(mode == 'e')
	{
		/* Load g1a file into memory */
		size_t size;
		struct g1a *g1a = load_g1a(argv[optind], &size);
		if(!g1a) return 1;

		/* Edit the fields with user-customized values */
		fields_edit(g1a, &fields);

		/* We don't reset fixed fields or recalculate checksums because
		   we only want to edit what was requested by the user.
		   Besides, the control bytes and checksums do *not* depend on
		   the value of user-customizable fields. */

		/* Regenerate input file, or output somewhere else */
		if(!output) output = argv[optind];
		save_g1a(output, g1a, size);
		free(g1a);
	}
	if(mode == 'd')
	{
		/* Load and dump the g1a */
		size_t size;
		struct g1a *g1a = load_g1a(argv[optind], &size);
		if(!g1a) return 1;

		dump(g1a, size);
		free(g1a);
	}
	if(mode == 'r')
	{
		/* Load g1a file into memory */
		size_t size;
		struct g1a *g1a = load_g1a(argv[optind], &size);
		if(!g1a) return 1;

		/* Repair file by recalculating fixed fields and checksums */
		sign(g1a, size);

		/* Regenerate input file, or output somewhere else */
		if(!output) output = argv[optind];
		save_g1a(output, g1a, size);
		free(g1a);
	}
	if(mode == 'x')
	{
		/* Load g1a file into memory */
		size_t size;
		struct g1a *g1a = load_g1a(argv[optind], &size);
		if(!g1a) return 1;

		/* Generate 8-bit icon from g1a 1-bit */
		uint8_t *data = icon_conv_1to8(g1a->header.icon);
		if(!data)
		{
			fprintf(stderr, "error: %m\n");
			return 1;
		}

		/* Calculate a default output name if none is provided */
		if(output)
		{
			icon_save(output, data, 30, 17);
		}
		else
		{
			char *alloc = malloc(strlen(argv[optind]) + 5);
			if(!alloc) {fprintf(stderr, "error: %m\n"); return 1;}
			default_output(argv[optind], ".png", alloc);

			icon_save(alloc, data, 30, 17);
			free(alloc);
		}
	}

	return 0;
}
