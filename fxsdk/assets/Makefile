#! /usr/bin/make -f
#  Default Makefile for fxSDK add-ins. This file was probably copied there by
#  the [fxsdk] program.
#---

#
#  Configuration
#

include project.cfg

# Compiler flags
CFLAGSFX := $(CFLAGS) $(CFLAGS_FX) $(INCLUDE)
CFLAGSCG := $(CFLAGS) $(CFLAGS_CG) $(INCLUDE)

# Linker flags
LDFLAGSFX := $(LDFLAGS) $(LDFLAGS_FX)
LDFLAGSCG := $(LDFLAGS) $(LDFLAGS_CG)

# Dependency list generation flags
depflags = -MMD -MT $@ -MF $(@:.o=.d) -MP
# ELF to binary flags
BINFLAGS := -R .bss -R .gint_bss

# G1A and G3A generation flags
NAME_G1A ?= $(NAME)
NAME_G3A ?= $(NAME)
G1AF := -i "$(ICON_FX)" -n "$(NAME_G1A)" --internal="$(INTERNAL)"
G3AF := -n basic:"$(NAME_G3A)" -i uns:"$(ICON_CG_UNS)" -i sel:"$(ICON_CG_SEL)"

ifeq "$(TOOLCHAIN_FX)" ""
TOOLCHAIN_FX := sh3eb-elf
endif

ifeq "$(TOOLCHAIN_CG)" ""
TOOLCHAIN_CG := sh4eb-elf
endif

# fxconv flags
FXCONVFX := --fx --toolchain=$(TOOLCHAIN_FX)
FXCONVCG := --cg --toolchain=$(TOOLCHAIN_CG)

#
#  File listings
#

ifeq "$(TARGET)" ""
NULL   :=
TARGET := $(subst $(NULL) $(NULL),-,$(NAME))
endif

elf       = $(dir $<)$(TARGET).elf
bin       = $(dir $<)$(TARGET).bin
TARGETFX := $(TARGET).g1a
TARGETCG := $(TARGET).g3a

# Source files
src       := $(wildcard src/*.[csS] \
                        src/*/*.[csS] \
                        src/*/*/*.[csS] \
                        src/*/*/*/*.[csS])
assets-fx := $(wildcard assets-fx/*/*)
assets-cg := $(wildcard assets-cg/*/*)

# Object files
obj-fx  := $(src:%=build-fx/%.o) \
           $(assets-fx:assets-fx/%=build-fx/assets/%.o)
obj-cg  := $(src:%=build-cg/%.o) \
           $(assets-cg:assets-cg/%=build-cg/assets/%.o)

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

all-fx: $(TARGETFX)
all-cg: $(TARGETCG)

$(TARGETFX): $(obj-fx) $(deps-fx)
	$(TOOLCHAIN_FX)-gcc -o $(elf) $(obj-fx) $(CFLAGSFX) $(LDFLAGSFX)
	$(TOOLCHAIN_FX)-objcopy -O binary $(BINFLAGS) $(elf) $(bin)
	fxg1a $(bin) -o $@ $(G1AF)

$(TARGETCG): $(obj-cg) $(deps-cg)
	$(TOOLCHAIN_CG)-gcc -o $(elf) $(obj-cg) $(CFLAGSCG) $(LDFLAGSCG)
	$(TOOLCHAIN_CG)-objcopy -O binary $(COPY) $(elf) $(bin)
	mkg3a $(G3AF) $(bin) $@

# C sources
build-fx/%.c.o: %.c
	@ mkdir -p $(dir $@)
	$(TOOLCHAIN_FX)-gcc -c $< -o $@ $(CFLAGSFX) $(depflags)
build-cg/%.c.o: %.c
	@ mkdir -p $(dir $@)
	$(TOOLCHAIN_CG)-gcc -c $< -o $@ $(CFLAGSCG) $(depflags)

# Assembler sources
build-fx/%.s.o: %.s
	@ mkdir -p $(dir $@)
	$(TOOLCHAIN_FX)-gcc -c $< -o $@
build-cg/%.s.o: %.s
	@ mkdir -p $(dir $@)
	$(TOOLCHAIN_CG)-gcc -c $< -o $@

# Preprocessed assembler sources
build-fx/%.S.o: %.S
	@ mkdir -p $(dir $@)
	$(TOOLCHAIN_FX)-gcc -c $< -o $@ $(INCLUDE)
build-cg/%.S.o: %.S
	@ mkdir -p $(dir $@)
	$(TOOLCHAIN_CG)-gcc -c $< -o $@ $(INCLUDE)

# Images
build-fx/assets/img/%.o: assets-fx/img/%
	@ mkdir -p $(dir $@)
	fxconv -i $< -o $@ $(FXCONVFX) name:img_$(basename $*) $(IMG.$*)
build-cg/assets/img/%.o: assets-cg/img/%
	@ mkdir -p $(dir $@)
	fxconv -i $< -o $@ $(FXCONVCG) name:img_$(basename $*) $(IMG.$*)

# Fonts
build-fx/assets/fonts/%.o: assets-fx/fonts/%
	@ mkdir -p $(dir $@)
	fxconv -f $< -o $@ $(FXCONVFX) name:font_$(basename $*) $(FONT.$*)
build-cg/assets/fonts/%.o: assets-cg/fonts/%
	@ mkdir -p $(dir $@)
	fxconv -f $< -o $@ $(FXCONVCG) name:font_$(basename $*) $(FONT.$*)

# Binaries
build-fx/assets/bin/%.o: assets-fx/bin/%
	@ mkdir -p $(dir $@)
	fxconv -b $< -o $@ $(FXCONVFX) name:bin_$(basename $*) $(BIN.$*)
build-cg/assets/bin/%.o: assets-cg/bin/%
	@ mkdir -p $(dir $@)
	fxconv -b $< -o $@ $(FXCONVCG) name:bin_$(basename $*) $(BIN.$*)

#
#  Cleaning and utilities
#

# Dependency information
-include $(shell find build* -name *.d 2> /dev/null)
build-fx/%.d: ;
build-cg/%.d: ;
.PRECIOUS: build-fx build-cg build-fx/%.d build-cg/%.d %/

clean-fx:
	@ rm -rf build-fx/
clean-cg:
	@ rm -rf build-cg/

distclean-fx: clean-fx
	@ rm -f $(TARGETFX)
distclean-cg: clean-cg
	@ rm -f $(TARGETCG)

clean: clean-fx clean-cg

distclean: distclean-fx distclean-cg

install-fx: $(TARGETFX)
	p7 send -f $<
install-cg: $(TARGETCG)
	@ while [[ ! -h /dev/Prizm1 ]]; do sleep 0.25; done
	@ while ! mount /dev/Prizm1; do sleep 0.25; done
	@ rm -f /mnt/prizm/$<
	@ cp $< /mnt/prizm
	@ umount /dev/Prizm1
	@- eject /dev/Prizm1

.PHONY: all all-fx all-cg clean distclean install-fx install-cg
