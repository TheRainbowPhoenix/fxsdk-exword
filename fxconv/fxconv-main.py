#! /usr/bin/python3

import getopt
import sys
import os
import fxconv

help_string = """
usage: fxconv [-s] <python script> [files...]
       fxconv  -b  <bin file> -o <object file> [parameters...]
       fxconv  -i  <png file> -o <object file> [parameters...]
       fxconv  -f  <png file> -o <object file> [parameters...]

fxconv converts data files such as images and fonts into gint formats
optimized for fast execution, or into object files.

Operating modes:
  -s, --script   Expose the fxconv module and run this Python script
  -b, --binary   Turn data into an object file, no conversion
  -i, --image    Convert to gint's image format
  -f, --font     Convert to gint's font format

When using -s, additional arguments are stored in the [fxconv.args] variable of
the module. This is intended to be a restricted list of file names specified by
a Makefile, used to convert only a subset of the files in the script.

The -b, -i and -f modes are shortcuts to convert single files without a script.
They accept parameters with a "category.key:value" syntax, for example:
  fxconv -f myfont.png -o myfont.o charset:ascii grid.padding:1 height:7
""".strip()

# Simple error-warnings system
def err(msg):
	print("error:", msg, file=sys.stderr)
def warn(msg):
	print("warning:", msg, file=sys.stderr)

def main():
	# Default execution mode is to run a Python script for conversion
	modes = "script binary image font"
	mode = "s"
	output = None

	# Parse command-line arguments

	if len(sys.argv) == 1:
		print(help_string, file=sys.stderr)
		sys.exit(1)

	try:
		opts, args = getopt.gnu_getopt(sys.argv[1:], "hsbifo:",
			("help output="+modes).split())
	except getopt.GetoptError as error:
		err(error)
		sys.exit(1)

	for name, value in opts:
		# Print usage
		if name == "--help":
			err(help_string, file=sys.stderr)
			sys.exit(0)
		# TODO: fxconv: verbose mode
		elif name == "--verbose":
			pass
		elif name in [ "-o", "--output" ]:
			output = value
		# Other names are modes
		else:
			mode = name[1] if len(name)==2 else name[2]

	# Remaining arguments
	if args == []:
		err(f"execution mode -{mode} expects an input file")
		sys.exit(1)
	input = args.pop(0)

	# In --script mode, run the Python script with an augmented PYTHONPATH

	if mode == "s":
		if output is not None:
			warn("option --output is ignored in script mode")
		args = None if args == [] else args

		err("script mode not currently implemented (TODO) x_x")
		sys.exit(1)

	# In shortcut conversion modes, read parameters from the command-line

	else:
		def check(arg):
			if ':' not in arg:
				warn(f"argument {arg} is not a valid parameter (ignored)")
			return ':' in arg

		def insert(params, path, value):
			if len(path) == 1:
				params[path[0]] = value
				return
			if not path[0] in params:
				params[path[0]] = {}
			insert(params[path[0]], path[1:], value)

		args = [ arg.split(':', 1) for arg in args if check(arg) ]
		params = {}
		for (name, value) in args:
			insert(params, name.split("."), value)

		params["type"] = { "b": "binary", "i": "image", "f": "font" }[mode]

		fxconv.convert(input, params, output)

main()
