"""
Convert data files into gint formats or object files
"""

import os
import tempfile
import subprocess
import collections
import re

from PIL import Image

__all__ = [
	# Color names
	"FX_BLACK", "FX_DARK", "FX_LIGHT", "FX_WHITE", "FX_ALPHA",
	# Conversion mechanisms
	"ObjectData", "u8", "u16", "u32", "ref", "sym",
	# Functions
	"quantize", "convert", "elf",
	# Reusable classes
	"Area", "Grid",
	# Reusable converters
	"convert_bopti_fx", "convert_bopti_cg",
	"convert_topti",
	"convert_libimg_fx", "convert_libimg_cg",
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

# Libimg flags
LIBIMG_FLAG_OWN = 1
LIBIMG_FLAG_RO  = 2

#
#  Character sets
#

FX_CHARSETS = {
	# Digits 0...9
	"numeric": [ (ord('0'), 10) ],
	# Uppercase letters A...Z
	"upper":   [ (ord('A'), 26) ],
	# Upper and lowercase letters A..Z, a..z
	"alpha":   [ (ord('A'), 26), (ord('a'), 26) ],
	# Letters and digits A..Z, a..z, 0..9
	"alnum":   [ (ord('A'), 26), (ord('a'), 26), (ord('0'), 10) ],
	# All printable characters from 0x20 to 0x7e
	"print":   [ (0x20, 95) ],
	# All 128 ASII characters
	"ascii":   [ (0x00, 128) ],
	# Custom Unicode block intervals
	"unicode": [],
}

#
# Conversion mechanisms
#

def u8(x):
    return bytes([ x & 255 ])
def u16(x):
    return bytes([ (x >> 8) & 255, x & 255 ])
def u32(x):
    return bytes([ (x >> 24) & 255, (x >> 16) & 255, (x >> 8) & 255, x & 255 ])

def ref(base, offset=None, padding=None):
	if isinstance(base, bytearray):
		base = bytes(base)

	if isinstance(base, bytes):
		assert offset is None
		if padding and len(base) % padding != 0:
			base += bytes(padding - len(base) % padding)
		return Ref(base, "", 0)
	elif isinstance(base, str):
		assert padding is None
		return Ref(b"", base, offset or 0)

	raise FxconvError(f"invalid type {type(base)} for ref()")

ptr = ref

def sym(name):
	return Sym("_" + name)

Ref = collections.namedtuple("Ref", ["data", "name", "offset"])
Sym = collections.namedtuple("Sym", ["name"])

class ObjectData:
	"""
	A sequence of bytes that can contain pointers to external variables or
	other data generated along the output structure.
	"""

	def __init__(self):
		"""Construct an empty ObjectData sequence."""
		self.elements = []
		self.static_data = bytes()

	def __add__(self, other):
		if isinstance(other, bytes):
			self.elements.append(other)
		elif isinstance(other, bytearray):
			self.elements.append(bytes(other))
		elif isinstance(other, Ref) and other.name:
			self.elements.append((other.name, other.offset))
		elif isinstance(other, Ref) and other.data:
			self.elements.append(("", len(self.static_data)))
			self.static_data += other.data
		elif isinstance(other, ObjectData):
			# Shift all offsets into other.static_data by len(self.static_data)
			el = other.elements
			for i in range(len(el)):
				if not isinstance(el[i], bytes):
					name, offset = el
					if not name:
						el[i] = (name, offset + len(self.static_data))
			# Extend elements and data
			self.elements += el
			self.static_data += static_data
		elif isinstance(other, Sym):
			self.elements.append(other)
		return self

	def data(self):
		return self.static_data

	def assembly(self, data_symbol):
		assembly = ""
		for el in self.elements:
			if isinstance(el, bytes):
				assembly += ".byte " + ",".join(hex(x) for x in el) + "\n"
			elif isinstance(el, Sym):
				assembly += f".global {el.name}\n"
				assembly += f"{el.name}:\n"
			else:
				name, offset = el
				name = "_" + name if name != "" else data_symbol
				assembly += f".long {name} + {offset}\n"
		return assembly

Structure = ObjectData

#
#  Area specifications
#

class Area:
	"""
	A subrectangle of an image, typically used for pre-conversion cropping.
	"""

	def __init__(self, area, img):
		"""
		Construct an Area object from a dict specification. The following keys
		may be used to specific the position and size of the rectangle:

		* "x", "y"          (int strings, default to 0)
		* "width", "height" (int strings, default to image dimensions)
		* "size"            ("WxH" where W and H are the width and height)

		The Area objects has attributes "x", "y", "w" and "h". Both positions
		default to 0 and both sizes to the corresponding image dimensions.
		"""

		self.x = int(area.get("x", 0))
		self.y = int(area.get("y", 0))
		self.w = int(area.get("width", img.width))
		self.h = int(area.get("height", img.height))

		if "size" in area:
			self.w, self.h = map(int, area["size"].split("x"))

	def tuple(self):
		"""Return the tuple representation (x,y,w,h), suitable for .crop(). """
		return (self.x, self.y, self.w, self.h)

#
#  Grid specifications
#

class Grid:
	"""
	A grid over an image, used to isolate glyphs in fonts and tiles in maps.
	Supports several types of spacing. To apply an outer border, use crop
	through an Area before using the Grid.
	"""

	def __init__(self, grid):
		"""
		Construct a Grid object from a dict specification. The following keys
		may be used to specify the dimension and spacing of the cells:

		* "border"           (int string, defaults to 0)
		* "padding"          (int string, defaults to 0)
		* "width", "height"  (int strings, mandatory if "size" not set)
		* "size"             ("WxH" where W and H are the cell width/height)

		The Grid object has attributes "border", "padding", "w" and "h". Each
		cell is of size "(w,h)" and has "padding" pixels of proper padding
		around it. Additionally, cells are separated by a border of size
		"border"; this includes an outer border.
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
		"""Yields subrectangles of the grid as tuples (x,y,w,h)."""
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

def convert_binary(input, params):
	return open(input, "rb").read()

#
# Image conversion for fx-9860G
#

def convert_bopti_fx(input, params):
	if isinstance(input, Image.Image):
		img = input.copy()
	else:
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
	return header + data

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

def convert_bopti_cg(input, params):
	if isinstance(input, Image.Image):
		img = input.copy()
	else:
		img = Image.open(input)
	if img.width >= 65536 or img.height >= 65536:
		raise FxconvError(f"'{input}' is too large (max. 65535x65535)")

	# Crop image to key "area"
	area = Area(params.get("area", {}), img)
	img = img.crop(area.tuple())

	# If no profile is specified, fall back to r5g6b5 or r5g6b5a later on
	name = params.get("profile", None)
	if name is not None:
		profile = CgProfile.find(name)

	if name in [ "r5g6b5", "r5g6b5a", None ]:
		# Encode the image into the 16-bit format
		encoded, alpha = r5g6b5(img)

		name = "r5g6b5" if alpha is None else "r5g6b5a"
		profile = CgProfile.find(name)

	elif name in [ "p4", "p8" ]:
		# Encoded the image into 16-bit with a palette of 16 or 256 entries
		color_count = 1 << int(name[1])
		encoded, palette, alpha = r5g6b5(img, color_count=color_count)

		encoded = palette + encoded

	else:
		raise FxconvError(f"unknown color profile '{name}'")

	if alpha is not None and not profile.supports_alpha:
		raise FxconvError(f"'{input}' has transparency; use r5g6b5a, p8 or p4")

	w, h, a = img.width, img.height, alpha or 0x0000

	header = bytearray([
		0x00, profile.id,		# Profile identification
		a >> 8, a & 0xff,		# Alpha color
		w >> 8, w & 0xff,		# Width
		h >> 8, h & 0xff,		# Height
	])

	return header + encoded

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

def _blockstart(name):
	m = re.match(r'(?:U\+)?([0-9A-Fa-f]+)\.', name)

	if m is None:
		return None
	try:
		return int(m[1], base=16)
	except Exception as e:
		return None

def convert_topti(input, params):

	#--
	# Character set
	#--

	if "charset" not in params:
		raise FxconvError("'charset' attribute is required and missing")

	charset = params["charset"]
	blocks = FX_CHARSETS.get(charset, None)
	if blocks is None:
		raise FxconvError(f"unknown character set '{charset}'")

	# Will be recomputed later for Unicode fonts
	glyph_count = sum(length for start, length in blocks)

	#--
	# Image input
	#--

	grid = Grid(params.get("grid", {}))

	# When using predefined charsets with a single image, apply the area and
	# check that the number of glyphs is correct
	if charset != "unicode":
		if isinstance(input, Image.Image):
			img = input.copy()
		else:
			img = Image.open(input)
		area = Area(params.get("area", {}), img)
		img = img.crop(area.tuple())

		# Quantize it (only black pixels will be encoded into glyphs)
		img = quantize(img, dither=False)

		if glyph_count > grid.size(img):
			raise FxconvError(
				f"not enough elements in grid (got {grid.size(img)}, "+
				f"need {glyph_count} for '{charset}')")

		inputs = [ img ]

	# In Unicode mode, load images for the provided directory, but don't apply
	# the area (this makes no sense since the sizes are different)
	else:
		try:
			files = os.listdir(input)
		except Exception as e:
			raise FxconvError(
				f"cannot scan directory '{input}' to discover blocks for the"+
				f"unicode charset: {str(e)}")

		# Keep only files with basenames like "<hexa>" or "U+<hexa>" and sort
		# them by code point order (for consistency)
		files = [e for e in files if _blockstart(e) is not None]
		files = sorted(files, key=_blockstart)

		# Open all images and guess the block size
		inputs = []
		for file in files:
			img = Image.open(os.path.join(input, file))
			img = quantize(img, dither=False)
			inputs.append(img)

		blocks = [(_blockstart(e), grid.size(img))
		          for e, img in zip(files, inputs)]

		# Recompute the total glyph count
		glyph_count = sum(length for start, length in blocks)


	#--
	# Proportionality and metadata
	#--

	proportional = (params.get("proportional", "false") == "true")
	title = bytes(params.get("title", ""), "utf-8") + bytes([0])

	flags = set(params.get("flags", "").split(","))
	flags.remove("")
	flags_std = { "bold", "italic", "serif", "mono" }

	if flags - flags_std:
		raise FxconvError(f"unknown flags: {', '.join(flags - flags_std)}")

	bold   = int("bold" in flags)
	italic = int("italic" in flags)
	serif  = int("serif" in flags)
	mono   = int("mono" in flags)

	flags = (bold << 7) | (italic << 6) | (serif << 5) | (mono << 4) \
			| int(proportional)
	# Default line height to glyph height
	line_height = int(params.get("height", grid.h))

	# Default character spacing to 1
	char_spacing = params.get("char-spacing", 1)

	#--
	# Encoding blocks
	#---

	def encode_block(b):
		start, length = b
		return u32((start << 12) | length)

	data_blocks = b''.join(encode_block(b) for b in blocks)

	#--
	# Encoding glyphs
	#--

	data_glyphs = []
	total_glyphs = 0
	data_width = bytearray()
	data_index  = bytearray()

	for img in inputs:
		for (number, region) in enumerate(grid.iter(img)):
			# Upate index
			if not (number % 8):
				idx = total_glyphs // 4
				data_index += u16(idx)

			# Get glyph area
			glyph = img.crop(region)
			if proportional:
				glyph = _trim(glyph)
				data_width.append(glyph.width)

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

	# Base data: always put the raw data and blocks first since they are
	# 4-aligned, to preserve alignment on the rest of the references.
	o = ObjectData()

	# Make the title pointer NULL if no title is specified
	if len(title) > 1:
		o += ref(title, padding=4)
	else:
		o += u32(0)

	o += u8(flags) + u8(line_height) + u8(grid.h) + u8(len(blocks))
	o += u32(glyph_count)
	o += u8(char_spacing) + bytes(3)
	o += ref(data_blocks)
	o += ref(data_glyphs)

	# For proportional fonts, add the index (2-aligned) then the glyph size
	# array (1-aligned).
	if proportional:
		o += ref(data_index)
		o += ref(data_width)
	# For fixed-width fonts, add more metrics
	else:
		o += u16(grid.w)
		o += u16((grid.w * grid.h + 31) >> 5)

	return o

#
# libimg conversion for fx-9860G
#

def convert_libimg_fx(input, params):
	if isinstance(input, Image.Image):
		img = input.copy()
	else:
		img = Image.open(input)
	if img.width >= 65536 or img.height >= 65536:
		raise FxconvError(f"'{input}' is too large (max. 65535x65535)")

	# Crop image to area
	area = Area(params.get("area", {}), img)
	img = img.crop(area.tuple())

	# Quantize the image. We don't need to check if there is gray; the VRAM
	# rendering function for mono output will adjust at runetime
	img = quantize(img, dither=False)
	code = { FX_WHITE: 0, FX_LIGHT: 1, FX_DARK: 2, FX_BLACK: 3, FX_ALPHA: 4 }

	# Encode image as a plain series of pixels
	data = bytearray(img.width * img.height)
	im = img.load()
	i = 0

	for y in range(img.height):
		for x in range(img.width):
			data[i] = code[im[x, y]]
			i += 1

	o = ObjectData()
	o += u16(img.width) + u16(img.height)
	o += u16(img.width) + u8(LIBIMG_FLAG_RO) + bytes(1)
	o += ref(data)

	return o


#
# libimg conversion for fx-CG 50
#

def convert_libimg_cg(input, params):
	if isinstance(input, Image.Image):
		img = input.copy()
	else:
		img = Image.open(input)
	if img.width >= 65536 or img.height >= 65536:
		raise FxconvError(f"'{input}' is too large (max. 65535x65535)")

	# Crop image to key "area"
	area = Area(params.get("area", {}), img)
	img = img.crop(area.tuple())

	# Encode the image into 16-bit format and force the alpha to 0x0001
	encoded, alpha = r5g6b5(img, alpha=(0x0001,0x0000))

	o = ObjectData()
	o += u16(img.width) + u16(img.height)
	o += u16(img.width) + u8(LIBIMG_FLAG_RO) + bytes(1)
	o += ref(encoded)

	return o

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

	# Make the image RGBA in case it was indexed so that transparent pixels are
	# represented in an alpha channel
	if img.mode == "P":
		img = img.convert("RGBA")

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

def r5g6b5(img, color_count=0, alpha=None):
	"""
	Convert a PIL.Image.Image into an R5G6B5 byte stream. If there are
	transparent pixels, chooses a color to implement alpha and replaces them
	with this color.

	Returns the converted image as a bytearray and the alpha value, or None if
	no alpha value was used.

	If color_count is provided, it should be either 16 or 256. The image is
	encoded with a palette of this size. Returns the converted image as a
	bytearray, the palette as a bytearray, and the alpha value (None if there
	were no transparent pixels).

	If alpha is provided, it should be a pair (alpha value, replacement).
	Trandarpent pixels will be encoded with the specified alpha value and
	pixels with the value will be encoded with the replacement.
	"""

	def rgb24to16(r, g, b):
		r = (r & 0xff) >> 3
		g = (g & 0xff) >> 2
		b = (b & 0xff) >> 3
		return (r << 11) | (g << 5) | b

	# Save the alpha channel and make it 1-bit
	try:
		alpha_channel = img.getchannel("A").convert("1", dither=Image.NONE)
		alpha_levels = { t[1]: t[0] for t in alpha_channel.getcolors() }
		has_alpha = 0 in alpha_levels
		replacement = None

		if has_alpha:
			alpha_pixels = alpha_channel.load()

	except ValueError:
		has_alpha = False

	# Convert the input image to RGB
	img = img.convert("RGB")

	# Optionally convert to palette
	if color_count:
		palette_size = color_count - int(has_alpha)
		img = img.convert("P", dither=Image.NONE, palette=Image.ADAPTIVE,
			colors=palette_size)
		palette = img.getpalette()

	pixels = img.load()

	# Choose an alpha color

	if alpha is not None:
		alpha, replacement = alpha

	elif color_count > 0:
		# Transparency is mapped to the last palette element, if there are no
		# transparent pixels then select an index out of bounds.
		alpha = color_count - 1 if has_alpha else 0xffff

	elif has_alpha:
		# Compute the set of all used R5G6B5 colors
		colormap = set()

		for y in range(img.height):
			for x in range(img.width):
				if alpha_pixels[x, y] > 0:
					colormap.add(rgb24to16(*pixels[x, y]))

		# Choose an alpha color among the unused ones
		available = set(range(65536)) - colormap

		if not available:
			raise FxconvError("image uses all 65536 colors and alpha")
		alpha = available.pop()

	else:
		alpha = None

	def alpha_encoding(color, a):
		if a > 0:
			if color == alpha:
				return replacement
			else:
				return color
		else:
			return alpha

	# Create a byte array with all encoded pixels

	pixel_count = img.width * img.height

	if not color_count:
		size = pixel_count * 2
	elif color_count == 256:
		size = pixel_count
	elif color_count == 16:
		size = (pixel_count + 1) // 2

	# Result of encoding
	encoded = bytearray(size)
	# Number of pixels encoded so far
	entries = 0
	# Offset into the array
	offset = 0

	for y in range(img.height):
		for x in range(img.width):
			a = alpha_pixels[x, y] if has_alpha else 0xff

			if not color_count:
				c = alpha_encoding(rgb24to16(*pixels[x, y]), a)
				encoded[offset]   = c >> 8
				encoded[offset+1] = c & 0xff
				offset += 2

			elif color_count == 16:
				c = alpha_encoding(pixels[x, y], a)

				# Aligned pixels: left 4 bits = high 4 bits of current byte
				if (entries % 2) == 0:
					encoded[offset] |= (c << 4)
				# Unaligned pixels: right 4 bits of current byte
				else:
					encoded[offset] |= c
					offset += 1

			elif color_count == 256:
				c = alpha_encoding(pixels[x, y], a)
				encoded[offset] = c
				offset += 1

			entries += 1

	if not color_count:
		return encoded, alpha

	# Encode the palette as R5G6B5

	encoded_palette = bytearray(2 * color_count)

	for c in range(color_count - int(has_alpha)):
		r, g, b = palette[3*c], palette[3*c+1], palette[3*c+2]
		rgb16 = rgb24to16(r, g, b)

		encoded_palette[2*c]   = rgb16 >> 8
		encoded_palette[2*c+1] = rgb16 & 0xff

	return encoded, encoded_palette, alpha

def convert(input, params, target, output=None, model=None, custom=None):
	"""
	Convert a data file into an object that exports the following symbols:
	* _<varname>
	* _<varname>_end
	* _<varname>_size
	The variable name is obtained from the parameter dictionary <params>.

	Arguments:
	input   -- Input file path
	params  -- Parameter dictionary
	target  -- String dictionary keys 'toolchain', 'arch' and 'section'
	output  -- Output file name [default: <input> with suffix '.o']
	model   -- 'fx' or 'cg'  (some conversions require this) [default: None]
	custom  -- Custom conversion function

	Produces an output file and returns nothing.
	"""

	if output is None:
		output = os.path.splitext(input)[0] + ".o"

	if "name" in params:
		pass
	elif "name_regex" in params:
		params["name"] = re.sub(*params["name_regex"], os.path.basename(input))
	else:
		raise FxconvError(f"no name specified for conversion '{input}'")

	if target["arch"] is None:
		target["arch"] = model

	if "custom-type" in params:
		t = params["custom-type"]
		# Also copy it in "type" for older converters (this is legacy)
		params["type"] = t
	elif "type" in params:
		t = params["type"]
	else:
		raise FxconvError(f"missing type in conversion '{input}'")

	if t == "binary":
		o = convert_binary(input, params)
	elif t == "bopti-image" and model in [ "fx", None ]:
		o = convert_bopti_fx(input, params)
	elif t == "bopti-image" and model == "cg":
		o = convert_bopti_cg(input, params)
	elif t == "font":
		o = convert_topti(input, params)
	elif t == "libimg-image" and model in [ "fx", None ]:
		o = convert_libimg_fx(input, params)
	elif t == "libimg-image" and model == "cg":
		o = convert_libimg_cg(input, params)
	elif custom is not None:
		for converter in custom:
			if converter(input, output, params, target) == 0:
				return
		raise FxconvError(f'unknown custom resource type \'{t}\'')
	else:
		raise FxconvError(f'unknown resource type \'{t}\'')

	# Standard conversions: save to ELF in the natural way
	elf(o, output, "_" + params["name"], **target)

def elf(data, output, symbol, toolchain=None, arch=None, section=None,
	assembly=None):
	"""
	Call objcopy to create an object file from the specified data. The object
	file will export three symbols:
	* <symbol>
	* <symbol>_end
	* <symbol>_size

	The symbol name must have a leading underscore if it is to be declared and
	used from a C program.

    The toolchain can be any target triplet for which the compiler is
    available. The architecture is deduced from some typical triplets;
    otherwise it can be set, usually as "sh3" or "sh4-nofpu". This affects the
	--binary-architecture flag of objcopy. If arch is set to "fx" or "cg", this
	function tries to be smart and:

	* Uses the name of the compiler if it contains a full architecture name
	  such as "sh3", "sh4" or "sh4-nofpu";
	* Uses "sh3" for fx9860g and "sh4-nofpu" for fxcg50 if the toolchain is
	  "sh-elf", which is a custom set;
	* Fails otherwise.

	The section name can be specified, along with its flags. A typical example
	would be section=".rodata,contents,alloc,load,readonly,data", which is the
	default.

	If assembly is set to a non-empty assembly program, this function also
	generates a temporary ELF file by assembling this piece of code, and merges
	it into the original one.

	Arguments:
	data       -- A bytes-like or ObjectData object to embed into the output
	output     -- Name of output file
	symbol     -- Chosen symbol name
	toolchain  -- Target triplet [default: "sh3eb-elf"]
	arch       -- Target architecture [default: try to guess]
	section    -- Target section [default: above variation of .rodata]
	assembly   -- Additional assembly code [default: None]

	Produces an output file and returns nothing.
	"""

	# Unfold ObjectData into data and assembly
	if isinstance(data, ObjectData):
		assembly = (assembly or "") + f"""
			.section .rodata
			.global {symbol}
			{symbol}:
		""" + data.assembly(symbol + "_staticdata")

		symbol = symbol + "_staticdata"
		data = data.data()

	if data is None and assembly is None:
		raise fxconv.FxconvError("elf() but no data and no assembly")

	# Toolchain parameters

	if toolchain is None:
		toolchain = "sh3eb-elf"
	if section is None:
		section = ".rodata,contents,alloc,load,readonly,data"

	if arch in ["fx", "cg", None] and toolchain in ["sh3eb-elf", "sh4eb-elf",
		"sh4eb-nofpu-elf"]:
		arch = toolchain.replace("eb-", "-")[:-4]

	elif arch == "fx" and toolchain == "sh-elf":
		arch = "sh3"
	elif arch == "cg" and toolchain == "sh-elf":
		arch = "sh4-nofpu"

	elif arch in ["fx", "cg", None]:
		raise FxconvError(f"non-trivial architecture for {toolchain} must be "+
			"specified")

	# Generate data - in <output> directly if there is no assembly

	if data:
		fp_obj = tempfile.NamedTemporaryFile()
		fp_obj.write(data)
		fp_obj.flush()

		sybl = "_binary_" + fp_obj.name.replace("/", "_")
		objcopy_args = [
			f"{toolchain}-objcopy", "-I", "binary", "-O", "elf32-sh",
			"--binary-architecture", arch, "--file-alignment", "4",
			"--rename-section", f".data={section}",
			"--redefine-sym", f"{sybl}_start={symbol}",
			"--redefine-sym", f"{sybl}_end={symbol}_end",
			"--redefine-sym", f"{sybl}_size={symbol}_size",
			fp_obj.name, output if not assembly else fp_obj.name + ".o" ]

		proc = subprocess.run(objcopy_args)
		if proc.returncode != 0:
			raise FxconvError(f"objcopy returned {proc.returncode}")

	# Generate assembly - in <output> directly if there is no data

	if assembly:
		fp_asm = tempfile.NamedTemporaryFile()
		fp_asm.write(assembly.encode('utf-8'))
		fp_asm.flush()

		as_args = [
			f"{toolchain}-as", "-c", fp_asm.name, "-o",
			output if not data else fp_asm.name + ".o" ]

		proc = subprocess.run(as_args)
		if proc.returncode != 0:
			raise FxconvError(f"as returned {proc.returncode}")

	# If both data and assembly are specified, merge everyone

	if data and assembly:
		ld_args = [
			f"{toolchain}-ld", "-r", fp_obj.name + ".o", fp_asm.name + ".o",
			"-o", output ]

		proc = subprocess.run(ld_args)
		if proc.returncode != 0:
			raise FxconvError(f"ld returned {proc.returncode}")

		os.remove(fp_obj.name + ".o")
		os.remove(fp_asm.name + ".o")

	if data:
		fp_obj.close()
	if assembly:
		fp_asm.close()
