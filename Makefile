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

INSTALL := $(TARGETS:%=install-%)
TARGETS := $(TARGETS:%=all-%)

# fxconv has no sources files because it's written in Python, and fxsdk has no
# source either because it's written in Bash.
src-fxg1a := $(wildcard fxg1a/*.c)
obj-fxg1a := $(src-fxg1a:%=build/%.o)

# Sed command to copy install path to fxsdk.sh. On Mac OS, BSD sed is used so
# we need to do it a bit differently with a printf helper to insert a literal
# newline into the command.
sed  := -E -e '/^PREFIX=.?.?.?\\$$/ a \$(PREFIX)'
ifeq "$(shell uname)" "Darwin"
sed  := -e "$$(printf '/^PREFIX=.?.?.?/ a \\\n$(PREFIX)')"
endif

# Symbolic targets

all: $(TARGETS)

all-fxsdk: bin/fxsdk.sh
all-fxg1a: bin/fxg1a
all-fxconv:

# Explicit targets

bin/fxsdk.sh: fxsdk/fxsdk.sh | bin/
	sed $(sed) $< > $@
bin/fxg1a: $(obj-fxg1a) | bin/
	gcc $^ -o $@ $(lflags)
bin/:
	mkdir -p $@

#
#  Source rules
#

build/%.c.o: %.c
	@ mkdir -p $(dir $@)
	gcc -c $< -o $@ $(cflags) $(dflags)

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

m644 := -m 644
m755 := -m 755

# Disable -m on Mac OS
ifeq "$(shell uname)" "Darwin"
m644 :=
m755 :=
endif

install: $(INSTALL)

install-fxsdk: all-fxsdk
	install -d $(PREFIX)/bin
	install bin/fxsdk.sh $(PREFIX)/bin/fxsdk
	install -d $(PREFIX)/share/fxsdk/assets
	install fxsdk/assets/* $(m644) $(PREFIX)/share/fxsdk/assets
	install bin/fxsdk.sh $(m755) $(PREFIX)/bin/fxsdk
	install -d $(PREFIX)/lib/cmake/fxsdk
	install fxsdk/cmake/* $(m644) $(PREFIX)/lib/cmake/fxsdk

install-fxg1a: all-fxg1a
	install -d $(PREFIX)/bin
	install bin/fxg1a $(m755) $(PREFIX)/bin

install-fxconv: all-fxconv
	install -d $(PREFIX)/bin
	install fxconv/fxconv-main.py $(m755) $(PREFIX)/bin/fxconv
	install fxconv/fxconv.py $(m644) $(PREFIX)/bin

uninstall:
	rm -f $(PREFIX)/bin/{fxsdk,fxg1a,fxconv,fxconv.py}
	rm -rf $(PREFIX)/share/fxsdk
	rm -rf $(PREFIX)/lib/cmake/fxsdk

#
#  Cleaning
#

clean-fxg1a:
	@rm -rf build/fxg1a

clean:
	@rm -rf build
distclean: clean
	@rm -rf bin
	@rm -f Makefile.cfg
