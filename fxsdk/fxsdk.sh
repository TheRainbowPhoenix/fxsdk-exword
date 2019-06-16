#! /usr/bin/env bash

# Note: this line is edited at install time to insert the install folder
PREFIX=\

usage_string=$(cat << EOF
usage: fxsdk new [<folder name>] [options...]
       fxsdk build [fx|cg]
       fxsdk send [fx|cg]

This program is a command-line helper for the fxSDK, a set of tools used in
conjunction with gint to develop add-ins for CASIO fx-9860G and fx-CG 50.

Project creation:
  fxsdk new [<folder name>] [options...]
  -@ <name>, --internal=<name>    Internal project name

EOF
)

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

  echo -ne "Internal name ? ('@' followed by at most 7 uppercase letters)\n> "
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
  sed -E 's/^([A-Z_]+)\s*=\s*(.*)/\1="\2"' project.cfg | source /dev/stdin
}

fxsdk_create_config() {
  cat << EOF
#---
# fxSDK project configuration file for $NAME
#---

# Project name, should be at most 8 bytes long.
NAME = $NAME
# Internal name, should be '@' followed by at most 7 uppercase letters.
INTERNAL = $INTERNAL

# fx-9860G icon location
ICON_FX = assets-fx/icon-fx.png
# fx-CG 50 icon locations
ICON_CG_UNS = assets-cg/icon-cg-uns.png
ICON_CG_SEL = assets-cg/icon-cg-sel.png

# Additional compiler flags
CFLAGS = -std=c11 -Os
# Additional linker flags
LDFLAGS =
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

    echo -e 'Unknown platform (valid names are "fx" and "cg")\n'
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

fxsdk_install() {
  if [[ -e "build-fx" && ! -e "build-cg" ]]; then
    fxsdk_install_fx
  fi

  if [[ -e "build-cg" && ! -e "build-fx" ]]; then
    fxsdk_install_cg
  fi

  echo "either no or several platforms are targeted, use 'fxsdk install-fx' or"
  echo "fxsdk 'install-cg' to specify which calculator to install to."
}

fxsdk_install_fx() {
  status "Installing for fx9860g using p7"
  make install-fx
}

fxsdk_install_cg() {
  # TODO
  echo "error: this is tricky and not implemented yet, sorry x_x"
}

# Parse command name

case $1 in

  # Project creation
  "new")
    shift
    fxsdk_new_project_interactive "$@";;

  # Project compilation
  "build")
    fxsdk_build
    [[ $1 == "-s" ]] && fxsdk_install;;
  "build-fx")
    fxsdk_build_fx
    [[ $1 == "-s" ]] && fxsdk_install;;
  "build-cg")
    fxsdk_build_cg
    [[ $1 == "-s" ]] && fxsdk_install;;

  # Install
  "install")
    fxsdk_install;;
  "install-fx")
    fxsdk_install_fx;;
  "install-cg")
    fxsdk_install_cg;;

  # Misc
  -h|--help|-\?)
    usage 0;;
  ?*)
    error "unknown command '$1'"
    exit 1;;
  *)
    usage 0;;
esac
