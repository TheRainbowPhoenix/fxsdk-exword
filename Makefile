#! /usr/bin/make -f

# Require config file if not cleaning up
ifeq "$(filter clean distclean,$(MAKECMDGOALS))" ""
include Makefile.cfg
endif

# Compiler flags
cflags = -Wall -Wextra -std=c11 -O2 -I $(dir $<) -D_GNU_SOURCE \
         -DFXSDK_PREFIX='"$(PREFIX)"' $(CFLAGS)
# Linker flags
lflags = -lpng
# Bison generation flags
# bflags = -L C --defines=$(@:.c=.h) --verbose
# Dependency generation flags
dflags = -MT $@ -MMD -MP -MF $(@:%.o=%.d)

#
#  Main targets and symbolic targets
#  $TARGETS is provided by Makefile.cfg.
#

TARGETS := $(filter-out fxconv,$(TARGETS))
bin = $(TARGETS:%=bin/%)

# fxconv has no sources files because it's written in Python
src = $(wildcard $1/*.c)
src-fxsdk  := $(call src,fxsdk)
src-fxg1a  := $(call src,fxg1a)
src-fxos   := $(call src,fxos)

obj = $(src-$1:%=build/%.o)
obj-fxsdk  := $(call obj,fxsdk)
obj-fxg1a  := $(call obj,fxg1a)
obj-fxos   := $(call obj,fxos)

# Symbolic targets

all: $(bin)

all-fxsdk: bin/fxsdk
all-fxg1a: bin/fxg1a
all-fxos: bin/fxos

# Explicit targets

bin/fxsdk: $(obj-fxsdk) | bin/
	gcc $^ -o $@ $(lflags)
bin/fxg1a: $(obj-fxg1a) | bin/
	gcc $^ -o $@ $(lflags)
bin/fxos: $(obj-fxos) | bin/
	gcc $^ -o $@ $(lflags)

bin/:
	mkdir -p $@

#
#  Source rules
#

build/%.c.o: %.c
	@mkdir -p $(dir $@)
	gcc -c $< -o $@ $(cflags) $(dflags)

# Flex lexers (unused since fxconv is written in Python)
# build/%/lexer.yy.c: %/lexer.l build/%/parser.tab.c
#	flex -o $@ -s $<
# build/%/lexer.yy.c.o: build/%/lexer.yy.c
#	gcc -c $< -o $@ $(cflags) -Wno-unused-function $(dflags) -I $*

# Bison parsers (unused since fxconv is written in Python)
# build/%/parser.tab.c: %/parser.y
#	bison $< -o $@ $(bflags)
# build/%/parser.tab.c.o: build/%/parser.tab.c
#	gcc -c $< -o $@ $(cflags) $(dflags) -I $*

#
#  Dependency system, misc.
#

include $(wildcard build/*/*.d)

# Dependency on configuration file
Makefile.cfg:
	@ if [[ ! -f Makefile.cfg ]]; then \
	  echo "error: Makefile.cfg is missing, did you ./configure?" >&2; \
	  false; \
	fi

.PHONY: all clean distclean

#
#  Installing
#

install: $(bin)
	install -d $(PREFIX)/bin
	install $(bin) -m 755 $(PREFIX)/bin
	install fxconv/fxconv-main.py -m 755 $(PREFIX)/bin/fxconv
	install fxconv/fxconv.py -m 644 $(PREFIX)/bin

#
#  Cleaning
#

clean-fxsdk:
	@rm -rf build/fxsdk
clean-fxconv:
	@rm -rf build/fxconv
clean-fxg1a:
	@rm -rf build/fxg1a
clean-fxos:
	@rm -rf build/fxos

clean:
	@rm -rf build
distclean: clean
	@rm -rf bin
	@rm -f Makefile.cfg
