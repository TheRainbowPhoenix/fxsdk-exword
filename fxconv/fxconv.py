"""
fxconv: Convert data files into gint formats or object files
"""

import os
import tempfile
import subprocess

from PIL import Image

#
# Color quantification
#

# Colors
FX_BLACK  = (  0,   0,   0, 255)
FX_DARK   = ( 85,  85,  85, 255)
FX_LIGHT  = (170, 170, 170, 255)
FX_WHITE  = (255, 255, 255, 255)
FX_ALPHA  = (  0,   0,   0,   0)

# Profiles

FX_PROFILES = [
	{ # Usual black-and-white bitmaps without transparency, as in MonochromeLib
	  "name":   "mono",
	  "gray":   False,
	  "colors": { FX_BLACK, FX_WHITE },
	  "layers": [ lambda c: (c == FX_BLACK) ]
	},
    { # Black-and-white with transparency, equivalent of two bitmaps in ML
	  "name":   "mono_alpha",
	  "gray":   False,
	  "colors": { FX_BLACK, FX_WHITE, FX_ALPHA },
	  "layers": [ lambda c: (c != FX_ALPHA),
	              lambda c: (c == FX_BLACK) ]
	},
	{ # Gray engine bitmaps, reference could have been Eiyeron's Gray Lib
	  "name":   "gray",
	  "gray":   True,
	  "colors": { FX_BLACK, FX_DARK, FX_LIGHT, FX_WHITE },
	  "layers": [ lambda c: (c in [FX_BLACK, FX_LIGHT]),
	              lambda c: (c in [FX_BLACK, FX_DARK]) ]
	},
	{ # Gray images with transparency, unfortunately 3 layers since 5 colors
	  "name":   "gray_alpha",
	  "gray":   True,
	  "colors": { FX_BLACK, FX_DARK, FX_LIGHT, FX_WHITE, FX_ALPHA },
	  "layers": [ lambda c: (c != FX_ALPHA),
	              lambda c: (c in [FX_BLACK, FX_LIGHT]),
	              lambda c: (c in [FX_BLACK, FX_DARK]) ]
	},
]

#
# Character sets
#

class _Charset:
	def __init__(self, id, name, count):
		self.id = id
		self.name = name
		self.count = count

FX_CHARSETS = [
	# Digits 0...9
	_Charset(0x0, "numeric", 10),
	# Uppercase letters A...Z
	_Charset(0x1, "upper", 26),
	# Upper and lowercase letters A..Z, a..z
	_Charset(0x2, "alpha", 52),
	# Letters and digits A..Z, a..z, 0..9
	_Charset(0x3, "alnum", 62),
	# All printable characters from 0x20 to 0x7e
	_Charset(0x4, "print", 95),
	# All 128 ASII characters
	_Charset(0x5, "ascii", 128),
]

#
# Internal routines
#

# normalize_area(): Expand area.size and set defaults for all values.
def _normalize_area(area, img):
	default = { "x": 0, "y": 0, "width": img.width, "height": img.height }
	if area is None:
		area = default
	else:
		if "size" in area:
			area["width"], area["height"] = area["size"].split("x")
		area = { **default, **area }

	return (int(area[key]) for key in "x y width height".split())

class _Grid:
	# [grid] is a dictionary of parameters. Relevant keys:
	#   "border", "padding", "width", "height", "size"
	def __init__(self, grid):
		self.border  = int(grid.get("border",  1))
		self.padding = int(grid.get("padding", 0))

		self.w = int(grid.get("width", "-1"))
		self.h = int(grid.get("height", "-1"))

		if "size" in grid:
			self.w, self.h = map(int, grid["size"].split("x"))

		if self.w <= 0 or self.h <= 0:
			raise FxconvError("size of grid unspecified or invalid")

	# size(): Number of elements in the grid
	def size(self, img):
		b, p, w, h =  self.border, self.padding, self.w, self.h

		# Padding-extended parameters
		W = w + 2 * p
		H = h + 2 * p

		columns = (img.width  - b) // (W + b)
		rows    = (img.height - b) // (H + b)
		return columns * rows


	# iter(): Iterator on all rectangles of the grid
	def iter(self, img):
		b, p, w, h =  self.border, self.padding, self.w, self.h

		# Padding-extended parameters
		W = w + 2 * p
		H = h + 2 * p

		columns = (img.width  - b) // (W + b)
		rows    = (img.height - b) // (H + b)

		for r in range(rows):
			for c in range(columns):
				x = b + c * (W + b) + p
				y = b + r * (H + b) + p
				yield (x, y, x + w, y + h)

#
# Binary conversion
#

def _convert_binary(input, output, params):
	raise FxconvError("TODO: binary mode x_x")

#
# Image conversion
#

def _profile_find(name):
	gen = ((i,pr) for (i,pr) in enumerate(FX_PROFILES) if pr["name"] == name)
	return next(gen, (None,None))

def _convert_image(input, output, params):
	img = Image.open(input)
	if img.width >= 4096 or img.height >= 4096:
		raise FxconvError(f"'{input}' is too large (max. 4095*4095)")

	# Expand area.size and get the defaults. Crop image to resulting area.
	params["area"] = _normalize_area(params.get("area", None), img)
	img = img.crop(params["area"])

	# Quantize the image and check the profile
	img = quantize(img, dither=False)

	# If profile is provided, check its validity, otherwise use the smallest
	# compatible profile

	colors = { y for (x,y) in img.getcolors() }

	if "profile" in params:
		p = params["profile"]
		pid, p = _profile_find(p)
		if p is None:
			raise FxconvError(f"unknown profile {p} in conversion '{input}'")
		if colors - profiles[p]:
			raise FxconvError(f"'{input}' has more colors than profile '{p}'")
	else:
		p = "gray" if FX_LIGHT in colors or FX_DARK in colors else "mono"
		if FX_ALPHA in colors: p += "_alpha"
		pid, p = _profile_find(p)

	# Make the image header

	header = bytes ([(0x80 if p["gray"] else 0) + pid])
	encode24bit = lambda x: bytes([ x >> 16, (x & 0xff00) >> 8, x & 0xff ])
	header += encode24bit((img.size[0] << 12) + img.size[1])

	# Split the image into layers depending on the profile and zip them all

	layers = [ _image_project(img, layer) for layer in p["layers"] ]
	count  = len(layers)
	size   = len(layers[0])

	data = bytearray(count * size)
	n = 0

	for longword in range(size // 4):
		for layer in layers:
			for i in range(4):
				data[n] = layer[4 * longword + i]
				n += 1

	# Generate the object file

	elf(header + data, output, "_" + params["name"])

def _image_project(img, f):
	# New width and height
	w = (img.size[0] + 31) // 32
	h = (img.size[1])

	data = bytearray(4 * w * h)
	im = img.load()

	# Now generate a 32-bit byte sequence
	for y in range(img.size[1]):
		for x in range(img.size[0]):
			bit = int(f(im[x, y]))
			data[4 * y * w + (x >> 3)] |= (bit << (~x & 7))

	return data

#
# Font conversion
#

def _charset_find(name):
	gen = (cs for cs in FX_CHARSETS if cs.name == name)
	return next(gen, None)

def _convert_font(input, output, params):

	#--
	# Image area and grid
	#--

	img = Image.open(input)
	params["area"] = _normalize_area(params.get("area", None), img)
	img = img.crop(params["area"])

	grid = _Grid(params.get("grid", {}))

	# Quantize image (any profile will do)
	img = quantize(img, dither=False)

	#--
	# Character set
	#--

	if "charset" not in params:
		raise FxconvError("'charset' attribute is required and missing")

	charset = _charset_find(params["charset"])
	if charset is None:
		raise FxconvError(f"unknown character set '{charset}'")
	if charset.count > grid.size(img):
		raise FxconvError(f"not enough elements in grid (got {grid.size(img)}, "+
			f"need {charset.count} for '{charset.name}')")

	#--
	# Proportionality and metadata
	#--

	proportional = (params.get("proportional", "false") == "true")

	title = params.get("title", "")
	if len(title) > 31:
		raise FxconvError(f"font title {title} is too long (max. 31 bytes)")
	# Pad title to 4 bytes
	title = bytes(title, "utf-8") + bytes(((4 - len(title) % 4) % 4) * [0])

	flags = set(params.get("flags", "").split(","))
	flags.remove("")
	flags_std = { "bold", "italic", "serif", "mono" }

	if flags - flags_std:
		raise FxconvError(f"unknown flags: {', '.join(flags - flags_std)}")

	bold   = int("bold" in flags)
	italic = int("italic" in flags)
	serif  = int("serif" in flags)
	mono   = int("mono" in flags)
	header = bytes([
		(len(title) << 3) | (bold << 2) | (italic << 1) | serif,
		(mono << 7) | (int(proportional) << 6) | (charset.id & 0xf),
		params.get("height", grid.h),
		grid.h,
	])

	encode16bit = lambda x: bytes([ x >> 8, x & 255 ])
	fixed_header = encode16bit(grid.w) + encode16bit((grid.w*grid.h + 31) >> 5)

	#--
	# Encoding glyphs
	#--

	data_glyphs = []
	data_widths = bytearray()
	data_index  = bytearray()

	for (number, region) in enumerate(grid.iter(img)):
		# Upate index
		if not (number % 8):
			idx = len(data_glyphs) // 4
			data_index += encode16bit(idx)

		# Get glyph area
		glyph = img.crop(region)
		if proportional:
			glyph = _trim(glyph)
			data_widths.append(glyph.width)

		length = 4 * ((glyph.width * glyph.height + 31) >> 5)
		bits = bytearray(length)
		offset = 0
		px = glyph.load()

		for y in range(glyph.size[1]):
			for x in range(glyph.size[0]):
				color = (px[x,y] == FX_BLACK)
				bits[offset >> 3] |= ((color * 0x80) >> (offset & 7))
				offset += 1

		data_glyphs.append(bits)

	data_glyphs = b''.join(data_glyphs)

	#---
	#  Object file generation
	#---

	if proportional:
		data = header + data_index + data_widths + data_glyphs + title
	else:
		data = header + fixed_header + data_glyphs + title

	elf(data, output, "_" + params["name"])

#
# Exceptions
#

FxconvError = Exception

#
# API
#

def quantize(img, dither=False):
	"""
	Convert a PIL.Image.Image into an RGBA image whose only colors are:
	* FX_BLACK  = (  0,   0,   0, 255)
	* FX_DARK   = ( 85,  85,  85, 255)
	* FX_LIGHT  = (170, 170, 170, 255)
	* FX_WHITE  = (255, 255, 255, 255)
	* FX_ALPHA  = (  0,   0,   0,   0)

	The alpha channel is first flattened to either opaque of full transparent,
	then all colors are quantized into the 4-shade scale. Floyd-Steinberg
	dithering can be used, although most applications will prefer nearest-
	neighbor coloring.

	Arguments:
	img     -- Input image, in any format
	dither  -- Enable Floyd-Steinberg dithering [default: False]

	Returns a quantized PIL.Image.Image.
	"""

	# Our palette will have only 4 colors for the gray engine
	colors = [ FX_BLACK, FX_DARK, FX_LIGHT, FX_WHITE ]

	# Create the palette
	palette = Image.new("RGBA", (len(colors), 1))
	for (i, c) in enumerate(colors):
		palette.putpixel((i, 0), c)
	palette = palette.convert("P")

	# Save the alpha channel, and make it either full transparent or opaque
	try:
		alpha_channel = img.getchannel("A").convert("1", dither=Image.NONE)
	except:
		alpha_channel = Image.new("L", img.size, 255)

	# Apply the palette to the original image (transparency removed)
	img = img.convert("RGB")

	# Let's do an equivalent of the following, but with a dithering setting:
	# img = img.quantize(palette=palette)

	img.load()
	palette.load()
	im = img.im.convert("P", int(dither), palette.im)
	img = img._new(im).convert("RGB")

	# Put back the alpha channel
	img.putalpha(alpha_channel)

	# Premultiply alpha
	pixels = img.load()
	for y in range(img.size[1]):
		for x in range(img.size[0]):
			r, g, b, a = pixels[x, y]
			if a == 0:
				r, g, b, = 0, 0, 0
			pixels[x, y] = (r, g, b, a)

	return img

def convert(input, params, output=None):
	"""
	Convert a data file into an object that exports the following symbols:
	* _<varname>
	* _<varname>_end
	* _<varname>_size
	The variable name is obtained from the parameter dictionary <params>.

	Arguments:
	input   -- Input file path
	params  -- Parameter dictionary
	output  -- Output file name [default: <input> with suffix '.o']

	Produces an output file and returns nothing.
	"""

	if output is None:
		output = os.path.splitext(input)[0] + '.o'

	if "name" not in params:
		raise FxconvError(f"no name specified for conversion '{input}'")

	if "type" not in params:
		raise FxconvError(f"missing type in conversion '{input}'")
	elif params["type"] == "binary":
		_convert_binary(input, output, params)
	elif params["type"] == "image":
		_convert_image(input, output, params)
	elif params["type"] == "font":
		_convert_font(input, output, params)

def elf(data, output, symbol, section=None, arch="sh3"):
	"""
	Call objcopy to create an object file from the specified data. The object
	file will export three symbols:
	* <symbol>
	* <symbol>_end
	* <symbol>_size

	The symbol name must have a leading underscore if it is to be declared and
	used from a C program.

	The section name can be specified, along with its flags. A typical example
	would be section=".rodata,contents,alloc,load,readonly,data", which is the
	default.

	The architecture can be either "sh3" or "sh4". This affects the choice of
	the toolchain (sh3eb-elf-objcopy versus sh4eb-nofpu-elf-objcopy) and the
	--binary-architecture flag of objcopy.

	Arguments:
	data     -- A bytes-like object with data to embed into the object file
	output   -- Name of output file
	symbol   -- Chosen symbol name
	section  -- Target section [default: above variation of .rodata]
	arch     -- Target architecture: "sh3" or "sh4" [default: "sh3"]

	Produces an output file and returns nothing.
	"""

	toolchain = { "sh3": "sh3eb-elf", "sh4": "sh4eb-nofpu-elf" }[arch]
	if section is None:
		section = ".rodata,contents,alloc,load,readonly,data"

	with tempfile.NamedTemporaryFile() as fp:
		fp.write(data)
		fp.flush()

		sybl = "_binary_" + fp.name.replace("/", "_")

		objcopy_args = [
			f"{toolchain}-objcopy", "-I", "binary", "-O", "elf32-sh",
			"--binary-architecture", arch, "--file-alignment", "4",
			"--rename-section", f".data={section}",
			"--redefine-sym", f"{sybl}_start={symbol}",
			"--redefine-sym", f"{sybl}_end={symbol}_end",
			"--redefine-sym", f"{sybl}_size={symbol}_size",
			fp.name, output ]

		proc = subprocess.run(objcopy_args)
		if proc.returncode != 0:
			raise FxconvError(f"objcopy returned {proc.returncode}")
