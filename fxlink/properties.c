#include "properties.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

bool properties_match(properties_t const *props, properties_t const *option)
{
	if(option->p7 && !props->p7)
		return false;
	if(option->mass_storage && !props->mass_storage)
		return false;
	if(option->series_cg && !props->series_cg)
		return false;
	if(option->series_g3 && !props->series_g3)
		return false;
	if(option->serial_number && (!props->serial_number ||
			strcmp(option->serial_number, props->serial_number)))
		return false;

	return true;
}

void properties_print(FILE *fp, properties_t const *props)
{
	#define output(...) {           \
		if(sep) fprintf(fp, ", ");  \
		fprintf(fp, __VA_ARGS__);   \
		sep = true;                 \
	}

	bool sep = false;
	if(props->p7)
		output("p7");
	if(props->mass_storage)
		output("mass_storage");
	if(props->series_cg)
		output("series_cg");
	if(props->series_g3)
		output("series_g3");
	if(props->serial_number)
		output("serial_number=%s", props->serial_number);
}
