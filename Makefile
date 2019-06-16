#! /usr/bin/make -f

# Require config file if not cleaning up
ifeq "$(filter clean distclean,$(MAKECMDGOALS))" ""
include Makefile.cfg
endif

# Compiler flags
cflags = -Wall -Wextra -std=c11 -g -I $(dir $<) -D_GNU_SOURCE \
         -DFXSDK_PREFIX='"$(PREFIX)"' $(CFLAGS)
# Linker flags
lflags = -lpng
# Dependency generation flags
dflags = -MT $@ -MMD -MP -MF $(@:%.o=%.d)

#
#  Main targets and symbolic targets
#  $TARGETS is provided by Makefile.cfg.
#

TARGETS := $(filter-out fxconv fxsdk,$(TARGETS))
bin = $(TARGETS:%=bin/%)

# fxconv has no sources files because it's written in Python, and fxsdk has no
# source either because it's written in Bash.
src = $(wildcard $1/*.c)
src-fxg1a  := $(call src,fxg1a)
src-fxos   := $(call src,fxos)
lex-fxos   := $(wildcard fxos/*.l)

obj = $($1:%=build/%.o)
lex = $($1:%.l=build/%.yy.c.o)
obj-fxg1a  := $(call obj,src-fxg1a)
obj-fxos   := $(call obj,src-fxos) $(call lex,lex-fxos)

# Symbolic targets

all: $(bin)

all-fxg1a: bin/fxg1a
all-fxos: bin/fxos

# Explicit targets

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

# Flex lexers for the fxos database
build/fxos/lexer-%.yy.c: fxos/lexer-%.l
	flex -o $@ -s $<
build/fxos/lexer-%.yy.c.o: build/fxos/lexer-%.yy.c
	gcc -c $< -o $@ $(cflags) -Wno-unused-function $(dflags) -I fxos

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

.PRECIOUS: build/fxos/lexer-%.yy.c

#
#  Installing
#

install: $(bin)
	install -d $(PREFIX)/bin
	install -d $(PREFIX)/share/fxsdk
	install $(bin) -m 755 $(PREFIX)/bin
	install fxos/*.txt -m 644 $(PREFIX)/share/fxsdk
	install -d $(PREFIX)/share/fxsdk/assets
	install fxsdk/assets/* -m 644 $(PREFIX)/share/fxsdk/assets
	install fxsdk/fxsdk.sh -m 755 $(PREFIX)/bin/fxsdk
	sed -i '/^PREFIX=\\$$/ a $(PREFIX)' $(PREFIX)/bin/fxsdk
	install fxconv/fxconv-main.py -m 755 $(PREFIX)/bin/fxconv
	install fxconv/fxconv.py -m 644 $(PREFIX)/bin

uninstall:
	rm -f $(PREFIX)/bin/{fxsdk,fxg1a,fxos,fxconv,fxconv.py}
	rm -rf $(PREFIX)/share/fxsdk

#
#  Cleaning
#

clean-fxg1a:
	@rm -rf build/fxg1a
clean-fxos:
	@rm -rf build/fxos

clean:
	@rm -rf build
distclean: clean
	@rm -rf bin
	@rm -f Makefile.cfg
