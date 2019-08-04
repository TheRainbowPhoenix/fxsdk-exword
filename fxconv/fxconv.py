"""
Convert data files into gint formats or object files
"""

import os
import tempfile
import subprocess

from PIL import Image

__all__ = [
	# Color names
	"FX_BLACK", "FX_DARK", "FX_LIGHT", "FX_WHITE", "FX_ALPHA",
	# Functions
	"quantize", "convert", "elf",
]

#
#  Constants
#

# Colors
FX_BLACK  = (  0,   0,   0, 255)
FX_DARK   = ( 85,  85,  85, 255)
FX_LIGHT  = (170, 170, 170, 255)
FX_WHITE  = (255, 255, 255, 255)
FX_ALPHA  = (  0,   0,   0,   0)

# fx-9860G profiles
class FxProfile:
	def __init__(self, id, name, colors, layers):
		"""
		Construct an FxProfile object.
		* [id] is the profile ID in bopti
		* [name] is the profile's name as seen in the "profile" key
		* [colors] is the set of supported colors
		* [layers] is a list of layer functions
		"""

		self.id = id
		self.name = name
		self.gray = FX_LIGHT in colors or FX_DARK in colors
		self.colors = colors
		self.layers = layers

	@staticmethod
	def find(name):
		"""Find a profile by name."""
		for profile in FX_PROFILES:
			if profile.name == name:
				return profile
		return None

FX_PROFILES = [
	# Usual black-and-white bitmaps without transparency, as in MonochromeLib
	FxProfile(0x0, "mono", { FX_BLACK, FX_WHITE }, [
		lambda c: (c == FX_BLACK),
	]),
    # Black-and-white with transparency, equivalent of two bitmaps in ML
	FxProfile(0x1, "mono_alpha", { FX_BLACK, FX_WHITE, FX_ALPHA }, [
		lambda c: (c != FX_ALPHA),
		lambda c: (c == FX_BLACK),
	]),
	# Gray engine bitmaps, reference could have been Eiyeron's Gray Lib
	FxProfile(0x2, "gray", { FX_BLACK, FX_DARK, FX_LIGHT, FX_WHITE }, [
		lambda c: (c in [FX_BLACK, FX_LIGHT]),
		lambda c: (c in [FX_BLACK, FX_DARK]),
	]),
	# Gray images with transparency, unfortunately 3 layers since 5 colors
	FxProfile(0x3, "gray_alpha",
		{ FX_BLACK, FX_DARK, FX_LIGHT, FX_WHITE, FX_ALPHA }, [
		lambda c: (c != FX_ALPHA),
		lambda c: (c in [FX_BLACK, FX_LIGHT]),
		lambda c: (c in [FX_BLACK, FX_DARK]),
	]),
]

# fx-CG 50 profiles
class CgProfile:
	def __init__(self, id, name, alpha):
		"""
		Construct a CgProfile object.
		* [id] is the profile ID in bopti
		* [name] is the profile name as found in the specification key
		* [alpha] is True if this profile supports alpha, False otherwise
		"""

		self.id = id
		self.name = name
		self.supports_alpha = alpha

	@staticmethod
	def find(name):
		"""Find a profile by name."""
		for profile in CG_PROFILES:
			if profile.name == name:
				return profile
		return None

CG_PROFILES = [
	# 16-bit R5G6B5
	CgProfile(0x0, "r5g6b5", False),
	# 16-bit R5G6B5 with alpha
	CgProfile(0x1, "r5g6b5a", True),
	# 8-bit palette
	CgProfile(0x2, "p8", True),
	# 4-bit palette
	CgProfile(0x3, "p4", True),
]

#
#  Character sets
#

class Charset:
	def __init__(self, id, name, count):
		self.id = id
		self.name = name
		self.count = count

	@staticmethod
	def find(name):
		"""Find a charset by name."""
		for charset in FX_CHARSETS:
			if charset.name == name:
				return charset
		return None

FX_CHARSETS = [
	# Digits 0...9
	Charset(0x0, "numeric", 10),
	# Uppercase letters A...Z
	Charset(0x1, "upper", 26),
	# Upper and lowercase letters A..Z, a..z
	Charset(0x2, "alpha", 52),
	# Letters and digits A..Z, a..z, 0..9
	Charset(0x3, "alnum", 62),
	# All printable characters from 0x20 to 0x7e
	Charset(0x4, "print", 95),
	# All 128 ASII characters
	Charset(0x5, "ascii", 128),
]

#
#  Area specifications
#

class Area:
	def __init__(self, area, img):
		"""
		Construct an Area object from a dict specification. The following keys
		may be used:

		* "x", "y"          (int strings, default to 0)
		* "width", "height" (int strings, default to image dimensions)
		* "size"            ("WxH" where W and H are the width and height)

		The Area objects has attributes "x", "y", "w" and "h".
		"""

		self.x = int(area.get("x", 0))
		self.y = int(area.get("y", 0))
		self.w = int(area.get("width", img.width))
		self.h = int(area.get("height", img.height))

		if "size" in area:
			self.w, self.h = map(int, area["size"].split("x"))

	def tuple(self):
		"""Return the tuple representation (x,y,w,h)."""
		return (self.x, self.y, self.w, self.h)

#
#  Grid specifications
#

class Grid:
	def __init__(self, grid):
		"""
		Construct a Grid object from a dict specification. The following keys
		may be used:

		* "border"           (int string, defaults to 0)
		* "padding"          (int string, defaults to 0)
		* "width", "height"  (int strings, mandatory if "size" not set)
		* "size"             ("WxH" where W and H are the cell width/height)

		The Grid object has attributes "border", "padding", "w" and "h".
		"""

		self.border = int(grid.get("border", 0))
		self.padding = int(grid.get("padding", 0))

		self.w = int(grid.get("width", -1))
		self.h = int(grid.get("height", -1))

		if "size" in grid:
			self.w, self.h = map(int, grid["size"].split("x"))

		if self.w <= 0 or self.h <= 0:
			raise FxconvError("size of grid unspecified or invalid")

	def size(self, img):
		"""Count the number of elements in the grid."""
		b, p, w, h =  self.border, self.padding, self.w, self.h

		# Padding-extended parameters
		W = w + 2 * p
		H = h + 2 * p

		columns = (img.width  - b) // (W + b)
		rows    = (img.height - b) // (H + b)
		return columns * rows


	def iter(self, img):
		"""Build an iterator on all subrectangles of the grid."""
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

def convert_binary(input, output, params):
	data = open(input, "rb").read()
	elf(data, output, "_" + params["name"])

#
# Image conversion for fx-9860G
#

def convert_bopti_fx(input, output, params):
	img = Image.open(input)
	if img.width >= 4096 or img.height >= 4096:
		raise FxconvError(f"'{input}' is too large (max. 4095x4095)")

	# Expand area.size and get the defaults. Crop image to resulting area.
	area = Area(params.get("area", {}), img)
	img = img.crop(area.tuple())

	# Quantize the image and check the profile
	img = quantize(img, dither=False)

	# If profile is provided, check its validity, otherwise use the smallest
	# compatible profile

	colors = { y for (x,y) in img.getcolors() }

	if "profile" in params:
		name = params["profile"]
		p = FxProfile.find(name)

		if p is None:
			raise FxconvError(f"unknown profile {name} in '{input}'")
		if colors - p.colors:
			raise FxconvError(f"{name} has too few colors for '{input}'")
	else:
		name = "gray" if FX_LIGHT in colors or FX_DARK in colors else "mono"
		if FX_ALPHA in colors: name += "_alpha"
		p = FxProfile.find(name)

	# Make the image header

	header = bytes ([(0x80 if p.gray else 0) + p.id])
	encode24bit = lambda x: bytes([ x >> 16, (x & 0xff00) >> 8, x & 0xff ])
	header += encode24bit((img.size[0] << 12) + img.size[1])

	# Split the image into layers depending on the profile and zip them all

	layers = [ _image_project(img, layer) for layer in p.layers ]
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
# Image conversion for fx-CG 50
#

def convert_bopti_cg(input, output, params):
	img = Image.open(input)
	if img.width >= 65536 or img.height >= 65536:
		raise FxconvError(f"'{input}' is too large (max. 65535x65535)")

	# Crop image to key "area"
	area = Area(params.get("area", {}), img)
	img = img.crop(area.tuple())

	# Encode the image into the 16-bit format
	encoded, alpha = r5g6b5(img)

	# If no profile is specified, fall back to R5G6B5 or R5G6B5A as needed
	name = params.get("profile", "r5g6b5" if alpha is None else "r5g6b5a")
	profile = CgProfile.find(name)

	if name in [ "r5g6b5", "r5g6b5a" ]:

		if alpha is not None and not profile.supports_alpha:
			raise FxconvError(f"'{input}' has transparency; use r5g6b5a")

		w, h, a = img.width, img.height, (0x00 if alpha is None else alpha)

		header = bytearray([
			0x00, profile.id,		# Profile identification
			a >> 8, a & 0xff,		# Alpha color
			w >> 8, w & 0xff,		# Width
			h >> 8, h & 0xff,		# Height
		])

		elf(header + encoded, output, "_" + params["name"])

#
# Font conversion
#

def _trim(img):
	def blank(x):
		return all(px[x,y] == FX_WHITE for y in range(img.height))

	left = 0
	right = img.width
	px = img.load()

	while left + 1 < right and blank(left):
		left += 1
	while right - 1 > left and blank(right - 1):
		right -= 1

	return img.crop((left, 0, right, img.height))

def _align(seq, align):
	n = (align - len(seq)) % align
	return seq + bytearray(n)

def _pad(seq, length):
	n = max(0, length - len(seq))
	return seq + bytearray(n)

def convert_topti(input, output, params):

	#--
	# Image area and grid
	#--

	img = Image.open(input)
	area = Area(params.get("area", {}), img)
	img = img.crop(area.tuple())

	grid = Grid(params.get("grid", {}))

	# Quantize image. (Profile doesn't matter here; only black pixels will be
	# encoded into glyphs. White pixels are used to separate entries and gray
	# pixels can be used to forcefully insert spacing on the sides.)
	img = quantize(img, dither=False)

	#--
	# Character set
	#--

	if "charset" not in params:
		raise FxconvError("'charset' attribute is required and missing")

	charset = Charset.find(params["charset"])
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
	total_glyphs = 0
	data_widths = bytearray()
	data_index  = bytearray()

	for (number, region) in enumerate(grid.iter(img)):
		# Upate index
		if not (number % 8):
			idx = total_glyphs // 4
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
		total_glyphs += length

	data_glyphs = b''.join(data_glyphs)

	#---
	#  Object file generation
	#---

	if proportional:
		data_index = _pad(data_index, 32)
		data_widths = _align(data_widths, 4)
		data = header + data_index + data_widths + data_glyphs + title
	else:
		data = header + fixed_header + data_glyphs + title

	elf(data, output, "_" + params["name"])

#
# Exceptions
#

class FxconvError(Exception):
	pass

#
# API
#

def quantize(img, dither=False):
	"""
	Convert a PIL.Image.Image into an RGBA image with only these colors:
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

def r5g6b5(img):
	"""
	Convert a PIL.Image.Image into an R5G6B5 byte stream. If there are
	transparent pixels, chooses a color to implement alpha and replaces them
	with this color.

	Returns the converted image as a bytearray and the alpha value, or None if
	no alpha value was used.
	"""

	def rgb24to16(r, g, b):
		r = (r & 0xff) >> 3
		g = (g & 0xff) >> 2
		b = (b & 0xff) >> 3
		return (r << 11) | (g << 5) | b

	# Save the alpha channel and make it 1-bit
	try:
		alpha_channel = img.getchannel("A").convert("1", dither=Image.NONE)
	except:
		alpha_channel = Image.new("L", img.size, 255)

	# Convert the input image to RGB and put back the alpha channel
	img = img.convert("RGB")
	img.putalpha(alpha_channel)

	# Gather a list of R5G6B5 colors

	colors = set()
	has_alpha = False

	pixels = img.load()
	for y in range(img.height):
		for x in range(img.width):
			r, g, b, a = pixels[x, y]

			if a == 0:
				has_alpha = True
			else:
				colors.add(rgb24to16(r, g, b))

	# Choose a color for the alpha if needed

	if has_alpha:
		palette = set(range(65536))
		available = palette - colors

		if not available:
			raise FxconvError("image uses all 65536 colors and alpha")
		alpha = available.pop()
	else:
		alpha = None

	# Create a byte array with all encoded pixels

	encoded = bytearray(img.width * img.height * 2)
	offset = 0

	for y in range(img.height):
		for x in range(img.width):
			r, g, b, a = pixels[x, y]

			if a == 0:
				encoded[offset]   = alpha >> 8
				encoded[offset+1] = alpha & 0xff
			else:
				rgb16 = rgb24to16(r, g, b)
				encoded[offset]   = rgb16 >> 8
				encoded[offset+1] = rgb16 & 0xff

			offset += 2

	return encoded, alpha

def convert(input, params, output=None, target=None):
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
	target  -- 'fx' or 'cg'  (some conversions require this) [default: None]

	Produces an output file and returns nothing.
	"""

	if output is None:
		output = os.path.splitext(input)[0] + ".o"

	if "name" not in params:
		raise FxconvError(f"no name specified for conversion '{input}'")

	if "type" not in params:
		raise FxconvError(f"missing type in conversion '{input}'")
	elif params["type"] == "binary":
		convert_binary(input, output, params)
	elif params["type"] == "image" and target in [ "fx", None ]:
		convert_bopti_fx(input, output, params)
	elif params["type"] == "image" and target == "cg":
		convert_bopti_cg(input, output, params)
	elif params["type"] == "font":
		convert_topti(input, output, params)

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
