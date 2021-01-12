#! /usr/bin/env python3

import getopt
import sys
import os
import re
import fnmatch
import fxconv

help_string = f"""
usage: fxconv [<TYPE>] <INPUT> -o <OUTPUT> [--fx|--cg] [<PARAMETERS>...]

fxconv converts resources such as images and fonts into binary formats for
fxSDK applications, using gint and custom conversion formats.

When no TYPE is specified (automated mode), fxconv looks for type and
parameters in an fxconv-metadata.txt file in the same folder as the input. This
is normally the default for add-ins.

When TYPE is specified (one-shot conversion), it should be one of:
  -b, --binary    Turn data into an object file without conversion
  -f, --font      Convert to gint's topti font format
  --bopti-image   Convert to gint's bopti image format
  --libimg-image  Convert to the libimg image format
  --custom        Use converters.py; you might want to specify an explicit type
                  by adding a parameter type:your_custom_type (see below)

During one-shot conversions, parameters can be specified with a "NAME:VALUE"
syntax (names can contain dots). For example:
  fxconv -f myfont.png -o myfont.o charset:ascii grid.padding:1 height:7

Some formats differ between platforms so you should specify it when possible:
  --fx, --fx9860G  Casio fx-9860G family (black-and-white calculators)
  --cg, --fxCG50   Casio fx-CG 50 family (16-bit color calculators)
""".strip()

# Simple error-warnings system
FxconvError = fxconv.FxconvError

def err(msg):
	print("\x1b[31;1merror:\x1b[0m", msg, file=sys.stderr)
	return 1
def warn(msg):
	print("\x1b[33;1mwarning:\x1b[0m", msg, file=sys.stderr)

# "converters" module from the user project... if it exists
try:
	import converters
except ImportError:
	converters = None

def parse_parameters(params):
	"""Parse parameters of the form "NAME:VALUE" into a dictionary."""
	d = dict()

	def insert(d, path, value):
		if len(path) == 1:
			d[path[0]] = value
		else:
			if not path[0] in d:
				d[path[0]] = dict()
			insert(d[path[0]], path[1:], value)

	for decl in params:
		if ":" not in decl:
			raise FxconvError(f"invalid parameter {decl}, ignoring")
		else:
			name, value = decl.split(":", 1)
			insert(d, name.split("."), value.strip())

	return d

def parse_parameters_metadata(contents):
	"""Parse parameters from a metadata file contents."""

	RE_COMMENT = re.compile(r'#.*$', re.MULTILINE)
	contents = re.sub(RE_COMMENT, "", contents)

	RE_WILDCARD = re.compile(r'^(\S(?:[^:\s]|\\:|\\ )*)\s*:\s*$', re.MULTILINE)
	lead, *elements = [ s.strip() for s in re.split(RE_WILDCARD, contents) ]

	if lead:
		raise FxconvError(f"invalid metadata: {lead} appears before wildcard")

	# Group elements by pairs (left: wildcard, right: list of properties)
	elements = list(zip(elements[::2], elements[1::2]))

	metadata = []
	for (wildcard, params) in elements:
		params = [ s.strip() for s in params.split("\n") if s.strip() ]
		metadata.append((wildcard, parse_parameters(params)))

	return metadata

def main():
	types = "binary image font bopti-image libimg-image custom"
	mode = ""
	output = None
	model = None
	target = { 'toolchain': None, 'arch': None, 'section': None }
	use_custom = False

	# Parse command-line arguments

	if len(sys.argv) == 1:
		print(help_string, file=sys.stderr)
		sys.exit(1)

	try:
		longs = f"help output= toolchain= arch= section= fx cg {types}"
		opts, args = getopt.gnu_getopt(sys.argv[1:], "hsbifo:", longs.split())
	except getopt.GetoptError as error:
		return err(error)

	for name, value in opts:
		# Print usage
		if name == "--help":
			print(help_string, file=sys.stderr)
			return 0
		elif name in [ "-o", "--output" ]:
			output = value
		elif name in [ "--fx", "--cg" ]:
			model = name[2:]
		elif name == "--toolchain":
			target['toolchain'] = value
		elif name == "--arch":
			target['arch'] = value
		elif name == "--section":
			target['section'] = value
		elif name == "--custom":
			use_custom = True
			mode = "custom"
		# Other names are modes
		else:
			mode = name[1] if len(name)==2 else name[2:]

	# Remaining arguments
	if args == []:
		err(f"no input file")
		sys.exit(1)
	input = args.pop(0)

	# In automatic mode, look for information in fxconv-metadata.txt
	if mode == "":
		metadata_file = os.path.dirname(input) + "/fxconv-metadata.txt"
		basename = os.path.basename(input)

		if not os.path.exists(metadata_file):
			return err(f"using auto mode but {metadata_file} does not exist")

		with open(metadata_file, "r") as fp:
			metadata = parse_parameters_metadata(fp.read())

			params = dict()
			for (wildcard, p) in metadata:
				if fnmatch.fnmatchcase(basename, wildcard):
					params.update(**p)

	# In manual conversion modes, read parameters from the command-line
	else:
		params = parse_parameters(args)

		if "type" in params:
			pass
		elif len(mode) == 1:
			params["type"] = { "b": "binary", "i": "image", "f": "font" }[mode]
		else:
			params["type"] = mode

		# Will be deprecated in the future
		if params["type"] == "image":
			warn("type 'image' is deprecated, use 'bopti-image' instead")
			params["type"] = "bopti-image"

	# Use the custom module
	custom = None
	if use_custom:
		if converters is None:
			return err("--custom specified but no [converters] module")
		custom = converters.convert

	fxconv.convert(input, params, target, output, model, custom)

if __name__ == "__main__":
	try:
		sys.exit(main())
	except fxconv.FxconvError as e:
		sys.exit(err(e))
