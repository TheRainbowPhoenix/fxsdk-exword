#! /usr/bin/make -f
#  Default Makefile for fxSDK add-ins. This file was probably copied there by
#  the [fxsdk] program.
#---

#
#  Configuration
#

include project.cfg

# Compiler flags
cf        := -mb -ffreestanding -nostdlib -Wall -Wextra \
             -fstrict-volatile-bitfields $(CFLAGS)
cf-fx     := $(cf) -m3 -DFX9860G
cf-cg     := $(cf) -m4-nofpu -DFXCG50

# Linker flags
lf-fx     := $(LDFLAGS) -Tfx9860g.ld -lgint-fx -lgcc -Wl,-Map=build-fx/map
lf-cg     := $(LDFLAGS) -Tfxcg50.ld  -lgint-cg -lgcc -Wl,-Map=build-cg/map

dflags     = -MMD -MT $@ -MF $(@:.o=.d) -MP
cpflags   := -R .bss -R .gint_bss

g1af      := -i "$(ICON_FX)" -n "$(NAME)" --internal="$(INTERNAL)"
g3af      := -n basic:"$(NAME)" -i uns:"$(ICON_CG_UNS)" -i sel:"$(ICON_CG_SEL)"

#
#  File listings
#

null      :=
filename  := $(subst $(null) $(null),-,$(NAME))

elf        = $(dir $<)$(filename).elf
bin        = $(dir $<)$(filename).bin
target-fx := $(filename).g1a
target-cg := $(filename).g3a

# Source files
src       := $(wildcard src/*.c)
assets-fx := $(wildcard assets-fx/**/*)
assets-cg := $(wildcard assets-cg/**/*)

# Object files
obj-fx  := $(src:%.c=build-fx/%.o) $(assets-fx:assets-fx/%=build-fx/assets/%.o)
obj-cg  := $(src:%.c=build-cg/%.o) $(assets-cg:assets-cg/%=build-cg/assets/%.o)

# Additional dependencies
deps-fx := $(ICON_FX)
deps-cg := $(ICON_CG_UNS) $(ICON_CG_SEL)

# All targets
all :=
ifneq "$(wildcard build-fx)" ""
all += all-fx
endif
ifneq "$(wildcard build-cg)" ""
all += all-cg
endif

#
#  Build rules
#

all: $(all)

all-fx: $(target-fx)
all-cg: $(target-cg)

$(target-fx): $(obj-fx) $(deps-fx)

	sh3eb-elf-gcc -o $(elf) $(obj-fx) $(cf-fx) $(lf-fx)
	sh3eb-elf-objcopy -O binary $(cpflags) $(elf) $(bin)
	fxg1a $(bin) -o $@ $(g1af)

$(target-cg): $(obj-cg) $(deps-cg)

	sh4eb-elf-gcc -o $(elf) $(obj-cg) $(cf-cg) $(lf-cg)
	sh4eb-elf-objcopy -O binary $(cpflags) $(elf) $(bin)
	mkg3a $(g3af) $(bin) $@

# C sources
build-fx/%.o: %.c
	@ mkdir -p $(dir $@)
	sh3eb-elf-gcc -c $< -o $@ $(cf-fx) $(dflags)
build-cg/%.o: %.c
	@ mkdir -p $(dir $@)
	sh4eb-elf-gcc -c $< -o $@ $(cf-cg) $(dflags)

# Images
build-fx/assets/img/%.o: assets-fx/img/%
	@ mkdir -p $(dir $@)
	fxconv -i $< -o $@ name:img_$(basename $*)

build-cg/assets/img/%.o: assets-cg/img/%
	@ echo -ne "\e[31;1mWARNING: image conversion for fxcg50 is not "
	@ echo -ne "supported yet\e[0m"
	@ mkdir -p $(dir $@)
	fxconv -i $< -o $@ name:img_$(basename $*)

# Fonts
build-fx/assets/fonts/%.o: assets-fx/fonts/%
	@ mkdir -p $(dir $@)
	fxconv -f $< -o $@ name:font_$(basename $*) $(FONT.$*)

build-cg/assets/fonts/%.o: assets-cg/fonts/%
	@ mkdir -p $(dir $@)
	fxconv -f $< -o $@ name:font_$(basename $*) $(FONT.$*)

#
#  Cleaning and utilities
#

# Dependency information
-include $(shell find build* -name *.d 2> /dev/null)
build-fx/%.d: ;
build-cg/%.d: ;
.PRECIOUS: build-fx build-cg build-fx/%.d build-cg/%.d %/

clean:
	@ rm -rf build*
distclean: clean
	@ rm -f $(target-fx) $(target-cg)

install-fx: $(target-fx)
	p7 send -f $<
install-cg: $(target-cg)
	@ while [[ ! -h /dev/Prizm1 ]]; do sleep 1; done
	@ mount /dev/Prizm1
	@ rm -f /mnt/prizm/$<
	@ cp $< /mnt/prizm
	@ umount /dev/Prizm1
	@- eject /dev/Prizm1

.PHONY: all all-fx all-cg clean distclean install-fx install-cg
