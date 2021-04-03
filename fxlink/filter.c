#include "filter.h"
#include "util.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

//---
// Property parser
//---

/* skip_spaces(): Skip spaces, returns true if end of string is reached */
bool skip_spaces(char const **input)
{
	while(isspace(**input)) (*input)++;
	return (**input == 0);
}
/* isword(): Identify valid word characters for the filter */
bool isword(int c)
{
	return c && !strchr(" \t\n,;=", c);
}
/* read_word(): Copy the next word in the string, assumes word is non-empty */
char *read_word(char const **input)
{
	char const *str = *input;
	while(**input && isword(**input)) (*input)++;
	return strndup(str, *input - str);
}

enum {
	T_END,          /* End of string */
	T_PROP,         /* Property; (*name) and (*value) are set */
	T_COMMA = ',',  /* Comma character (property separator) */
	T_SEMI  = ';',  /* Semicolon character (option separator) */
};

/* lex(): Read a token from the input source
   Returns the token type, updates (*input) and sets (*name) and (*value) to
   either NULL or some freshly-allocated copies of the name and (optional)
   value of the property (always NULL unless T_PROP is returned). The caller
   should free() both. */
static int lex(char const **input, char **name, char **value)
{
	*name = *value = NULL;
	if(skip_spaces(input)) return T_END;

	if(**input == ',' || **input == ';') {
		(*input)++;
		return (*input)[-1];
	}

	if(!isword(**input)) {
		wrn("expected property name in filter, skipping '%c'", **input);
		(*input)++;
		return lex(input, name, value);
	}
	*name = read_word(input);
	if(skip_spaces(input) || **input != '=')
		return T_PROP;

	(*input)++;
	if(skip_spaces(input))
		wrn("no value after '=' in filter property '%s'", *name);
	else if(!isword(**input))
		wrn("ignoring invalid value for filter property '%s'", *name);
	else
		*value = read_word(input);
	return T_PROP;
}

filter_t *filter_parse(char const *input)
{
	char *name=NULL, *value=NULL;
	int t;

	/* Create an initial filter with a single otion */
	filter_t *filter = malloc(sizeof *filter);
	if(!filter) return NULL;
	filter->options = calloc(1, sizeof(properties_t));
	if(!filter->options) {
		free(filter);
		return NULL;
	}
	filter->length = 1;

	/* Current option */
	properties_t *option = &filter->options[0];

	while((t = lex(&input, &name, &value)) != T_END) {
		/* Ignore property separators (tokens are already separated) */
		if(t == ',') continue;

		/* Add a new option in the filter */
		if(t == ';') {
			size_t new_size = (filter->length + 1) * sizeof(properties_t);
			properties_t *new_options = realloc(filter->options, new_size);
			if(!new_options) continue;

			filter->options = new_options;
			option = &filter->options[filter->length++];
			*option = (properties_t){ 0 };
			continue;
		}

		/* Add a new property to the current option */
		if(!strcmp(name, "p7") && !value)
			option->p7 = true;
		else if(!strcmp(name, "mass_storage") && !value)
			option->mass_storage = true;
		else if(!strcmp(name, "series_cg") && !value)
			option->series_cg = true;
		else if(!strcmp(name, "series_g3") && !value)
			option->series_g3 = true;
		else if(!strcmp(name, "serial_number") && value) {
			option->serial_number = strdup(value);
		}
		else wrn("ignoring invalid filter property: '%s' %s value", name,
			value ? "with" : "without");

		free(name);
		free(value);
	}

	return filter;
}

void filter_free(filter_t *filter)
{
	if(!filter) return;

	for(size_t i = 0; i < filter->length; i++)
		free(filter->options[i].serial_number);

	free(filter->options);
	free(filter);
}

//---
// Filtering API
//---

void filter_clean_libusb(filter_t *filter)
{
	if(!filter) return;

	for(size_t i = 0; i < filter->length; i++) {
		properties_t *prop = &filter->options[i];

		/* Suppress series_cg and series_g3, which are based off the USB Mass
		   Storage metadata provided only by UDisks2 */
		if(prop->series_cg) {
			wrn("ignoring series_cg in libusb filter (cannot be detected)");
			prop->series_cg = false;
		}
		if(prop->series_g3) {
			wrn("ignoring series_g3 in libusb filter (cannot be detected)");
			prop->series_g3 = false;
		}
	}
}

void filter_clean_udisks2(filter_t *filter)
{
	/* Every property can be used */
	(void)filter;
}

bool filter_match(properties_t const *props, filter_t const *filter)
{
	/* No filter is a pass-through */
	if(!filter || !filter->length)
		return true;

	for(size_t i = 0; i < filter->length; i++) {
		if(properties_match(props, &filter->options[i]))
			return true;
	}
	return false;
}

void filter_print(FILE *fp, filter_t const *filter)
{
	#define output(...) {           \
		if(sep) fprintf(fp, ", ");  \
		fprintf(fp, __VA_ARGS__);   \
		sep = true;                 \
	}

	for(size_t i = 0; i < filter->length; i++) {
		if(i > 0) printf("; ");
		properties_t *prop = &filter->options[i];
		properties_print(fp, prop);
	}
}
