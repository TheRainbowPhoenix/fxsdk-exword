#! /usr/bin/env bash

set -e

# Note: this line is edited at compile time to insert the install folder
PREFIX=\

usage_string=$(cat << EOF
usage: fxsdk new <folder>
       fxsdk (build|build-fx|build-cg) [-s]
       fxsdk (send|send-fx|send-cg)

This program is a command-line helper for the fxSDK, a set of tools used in
conjunction with gint to develop add-ins for CASIO fx-9860G and fx-CG 50.

Project creation:
  fxsdk new <folder>

  Creates a new project in the specified folder. Project info is input
  interactively. Creates <folder name> and populates it with default project
  data.

Compilation:
  fxsdk build [-s]
  fxsdk build-fx [-s]
  fxsdk build-cg [-s]

  Compiles the current project for fx-9860G (.g1a target) or fx-CG 50 (.g3a
  target). With 'fxsdk build', compiles every existing build folder, and ask
  interactively if none is found.

  With '-s', also sends the resulting program to the calculator.

Installation:
  fxsdk send
  fxsdk send-fx
  fxsdk send-cg

  Sends the target file to the calculator. Uses p7 (which must be installed
  externally) for fx-9860G. Currently not implemented for fx-CG 50, as it
  requires detecting and mounting the calculator (same for the Graph 35+E II).

Project update:
  fxsdk update

  Copies the latest version of the Makefile to your project. *This will
  discard any changes made to your Makefile.* If you have edited your
  Makefile, make a backup and merge the changes after updating.
EOF
)

# Project creation flags
#  -n <name>, --name=<name>        Project name
#  -@ <name>, --internal=<name>    Internal project name

usage() {
  echo "$usage_string"
  exit ${1:-1}
}

error() {
  echo -n "error: " >&2
  echo "$@" >&2
  echo "Try 'fxsdk --help' for more information." >&2
}

status() {
    echo -ne "\n\e[34;1m::\e[39;1m "
    echo -n "$@"
    echo -e "\e[0m\n"
}

# TODO: Finish this interface
fxsdk_new_project_cli() {
  internal=

  while [[ "$@" ]]; do
    case "$1" in
      -h|--help|-\?)
        usage 0;;

      -@|--internal)
        [[ -z "$2" ]] && error "$1 requires an argument" && return 1
        internal="$2"
        shift;;

      --internal=*)
        internal="${1#*=}";;
    esac

    shift
  done

  echo "internal=$internal"
}

fxsdk_new_project_interactive() {
  [[ -z "$1" ]] && error "please specify the project folder" && return 1
  [[ -e "$1" ]] && error "'$1' exists, I don't dare touch it" && return 1

  echo -e "Creating a new project in folder '$1'.\n"

  echo -ne "Full project name ? (at most 8 characters)\n> "
  read NAME

  echo "Internal name ? ('@' followed by at most 7 uppercase letters)"
  echo -ne "(Add-in might not appear on calc if format is wrong)\n> "
  read INTERNAL

  mkdir -p "$1"/{,src,assets-fx,assets-cg}
  fxsdk_create_config > "$1/project.cfg"

  echo -e "\nYour project '$NAME' has been created.\n"
  echo "Type 'fxsdk build-fx' or 'fxsdk build-cg' to compile the program."

  assets="$PREFIX/share/fxsdk/assets"
  cp "$assets"/Makefile "$1"
  cp "$assets"/main.c "$1"/src
  cp "$assets"/icon-fx.png "$1"/assets-fx
  cp "$assets"/icon-cg-uns.png "$1"/assets-cg
  cp "$assets"/icon-cg-sel.png "$1"/assets-cg
}

fxsdk_load_config() {
  grep -E '^ *[a-zA-Z0-9_]+ *=' project.cfg \
  | sed -E 's/^([A-Z_]+)\s*=\s*(.*)/\1="\2"/' \
  | source /dev/stdin
}

fxsdk_create_config() {
  cat << EOF
#---
# fxSDK project configuration file for $NAME
#---

# Project name, should be at most 8 bytes long.
# (You can also specify NAME_G1A or NAME_G3A to override individually.)
NAME := $NAME

# Internal name, should be '@' followed by at most 7 uppercase letters.
# WARNING: If this convention is not followed, the add-in might not appear in
#          the main menu of the calculator!
INTERNAL := $INTERNAL

# Output file name. The default is to take <NAME>, replace spaces with dashes,
# and add .g1a (or .g3a). You can specify a different folder if you want.
TARGET_FX :=
TARGET_CG :=

# fx-9860G icon location
ICON_FX = assets-fx/icon-fx.png
# fx-CG 50 icon locations
ICON_CG_UNS = assets-cg/icon-cg-uns.png
ICON_CG_SEL = assets-cg/icon-cg-sel.png

#---
# Toolchain selection
#---

# Toolchain for fx9860g. Please see also CFLAGS_FX below.
TOOLCHAIN_FX := sh-elf

# Toolchain for fxcg50. Please see also CFLAGS_CG below.
TOOLCHAIN_CG := sh-elf

#---
# Compiler flags
#---

# Base compiler flags for the fxSDK, you usually want to keep these.
CFLAGS := -mb -ffreestanding -nostdlib -fstrict-volatile-bitfields

# Platform-specific compiler flags.
# <> If you are using sh3eb-elf, use -m3. (You can do this on both FX and CG.)
# <> If you are using sh4eb-elf, use -m4-nofpu. (Not ideal on FX but works.)
# <> If you are using sh4eb-nofpu-elf, then your compiler will likely use the
#    FPU and cause problems on the calculator. Consider another configuration.
# <> If you are using an sh-elf with several targets, specify whichever you
#    support. I recommend -m3 on FX and -m4-nofpu on CG.
# Please see also TOOLCHAIN_FX and TOOLCHAIN_CG above.
CFLAGS_FX := -D FX9860G -m3
CFLAGS_CG := -D FXCG50  -m4-nofpu

# Additional compiler flags, change to your own taste!
CFLAGS += -Wall -Wextra -Os

# Include paths. Add one -I option for each folder from which you want to
# be able to include files with #include<>.
INCLUDE := -I include

# Libraries. Add one -l option for each library you are using, and also
# suitable -L options if you have library files in custom folders. To use
# fxlib, add libfx.a to the project directory and use "-L . -lfx".
LIBS :=

# Base linker flags for the fxSDK, you usually want to keep these.
LDFLAGS_FX := -T fx9860g.ld -lgint-fx \$(LIBS) -lgint-fx -lgcc
LDFLAGS_CG := -T fxcg50.ld  -lgint-cg \$(LIBS) -lgint-cg -lgcc

# Additional linker flags, if you need any.
LDFLAGS :=

# Additional platform-specific linker flags.
LDFLAGS_FX += -Wl,-Map=build-fx/map
LDFLAGS_CG += -Wl,-Map=build-cg/map

#---
# File conversion parameters
#---

# Here you can add fxconv options for each converted file, individually.
# The syntax is "<type>.<file>". For example, to specify the parameters for a
# font named "hexa.png", you might write:
#
#   FONT.hexa.png = charset:print grid.size:3x5 grid.padding:1
EOF
}

fxsdk_build() {
  built=

  if [[ -e build-fx ]]; then
    status "Making into build-fx"
    make all-fx
    built=1
  fi

  if [[ -e build-cg ]]; then
    status "Making into build-cg"
    make all-cg
    built=1
  fi

  [[ $built ]] && return

  echo "No build files currently exist. Which platform do I compile for?"
  echo ""
  echo '"fx": fx-9860G II, Graph 35+ USB/E/E II, Graph 75+'
  echo '"cg": fx-CG 10/20/50, Graph 90+E'
  echo ""
  echo "Leave blank to cancel."

  platform=
  while true; do
    echo -n '> '
    read platform
    [[ -z $platform ]] && return

    if [[ $platform == "fx" ]]; then
      status "Making into build-fx"
      make all-fx
      return
    fi

    if [[ $platform == "cg" ]]; then
      status "Making into build-fx"
      make all-cg
      return
    fi

    echo -e 'Unknown platform (valid names are "fx" and "cg")!'
  done
}

fxsdk_build_fx() {
  status "Making into build-fx"
  make all-fx
}

fxsdk_build_cg() {
  status "Making into build-cg"
  make all-cg
}

fxsdk_send() {
  if [[ -e "build-fx" && ! -e "build-cg" ]]; then
    fxsdk_send_fx
  fi

  if [[ -e "build-cg" && ! -e "build-fx" ]]; then
    fxsdk_send_cg
  fi

  echo "either no or several platforms are targeted, use 'fxsdk send-fx' or"
  echo "fxsdk 'send-cg' to specify which calculator to send to."
}

fxsdk_send_fx() {
  status "Installing for fx9860g using p7"
  make install-fx
}

fxsdk_send_cg() {
  # TODO
  echo "error: this is tricky and not implemented yet, sorry x_x"
}

fxsdk_update() {
  if [[ ! -e "project.cfg" ]]; then
    echo "No file 'project.cfg' was found. This does not look like an fxSDK"
    echo "project folder. (Nothing done.)"
  fi

  assets="$PREFIX/share/fxsdk/assets"
  cp "$assets"/Makefile .
  echo "Succesfully copied $assets/Makefile to the current directory."
}

# Parse command name

case $1 in

  # Project creation
  "new")
    shift
    fxsdk_new_project_interactive "$@";;

  # Project compilation
  "build"|"b")
    fxsdk_build
    [[ $1 == "-s" ]] && fxsdk_send;;
  "build-fx"|"bf"|"bfx")
    fxsdk_build_fx
    [[ $1 == "-s" ]] && fxsdk_send_fx;;
  "build-cg"|"bc"|"bcg")
    fxsdk_build_cg
    [[ $1 == "-s" ]] && fxsdk_send_cg;;

  # Install
  "send"|"s")
    fxsdk_send;;
  "send-fx"|"sf"|"sfx")
    fxsdk_send_fx;;
  "send-cg"|"sc"|"scg")
    fxsdk_send_cg;;

  # Project update
  "update")
    fxsdk_update;;

  # Misc
  -h|--help|-\?)
    usage 0;;
  ?*)
    error "unknown command '$1'"
    exit 1;;
  *)
    usage 0;;
esac
