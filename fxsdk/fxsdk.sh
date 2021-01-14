#! /usr/bin/env bash

set -e

# Note: this line is edited at compile time to insert the install folder
PREFIX=\

R=$(printf "\e[31;1m")
g=$(printf "\e[32m\e[3m")
n=$(printf "\e[0m")

usage_string=$(cat << EOF
usage: ${R}fxsdk${n} (${R}new${n}|${R}build${n}|${R}build-fx${n}|${R}build-\
cg${n}|${R}send${n}|${R}send-fx${n}|${R}send-cg${n}) [${g}ARGUMENTS${n}...]

This program is a command-line helper for the fxSDK, a set of tools used in
conjunction with gint to develop add-ins for CASIO fx-9860G and fx-CG 50.

${R}fxsdk new${n} ${g}<FOLDER>${n} [${R}--makefile${n}|${R}--cmake${n}] \
[${g}<NAME>${n}]
  Create a new project in the specified folder. The default build system is
  CMake. Project name can be specified now or in the project files later.

${R}fxsdk build${n} [${g}-s${n}]
${R}fxsdk build-fx${n} [${g}-s${n}]
${R}fxsdk build-cg${n} [${g}-s${n}]
  Build the current project for fx-9860G (.g1a target) or fx-CG 50 (.g3a
  target). With 'fxsdk build', compiles every existing build folder, and ask
  interactively if none is found.

  With '-s', also sends the resulting program to the calculator.

${R}fxsdk send${n}
${R}fxsdk send-fx${n}
${R}fxsdk send-cg${n}
  Sends the target file to the calculator. Uses p7 (which must be installed
  externally) for fx-9860G. Currently not implemented for fx-CG 50, as it
  requires detecting and mounting the calculator (same for the Graph 35+E II).

${R}fxsdk update${n}
  Copies the latest version of the Makefile to your project. *This will
  discard any changes made to your Makefile.* If you have edited your
  Makefile, make a backup and merge the changes after updating.
EOF
)

usage() {
  echo "$usage_string"
  exit ${1:-1}
}

error() {
  echo "error:" "$@" >&2
  exit 1
}

status() {
    echo -ne "\n\e[34;1m::\e[39;1m "
    echo -n "$@"
    echo -e "\e[0m\n"
}

fxsdk_new_project() {
  # Generator to use, output folder and project name
  generator="CMake"
  folder=""
  name=""

  # Parse options, then skip to positional arguments
  TEMP=$(getopt -o "" -l "makefile,cmake" -n "$0" -- "$@")
  eval set -- "$TEMP"
  for arg; do case "$arg" in
    "--makefile") generator="Makefile";;
    "--cmake") generator="CMake";;
    *) break;;
  esac; done
  while [[ "$1" != "--" ]]; do shift; done; shift

  if [[ -z "$1" ]]; then
    usage 1
  fi
  if [[ -e "$1" && "$1" != "." ]]; then
    error "$1 exists, I don't dare touch it"
  fi

  # Determine name and internal name
  if [[ ! -z "$2" ]]; then
    NAME=${2::8}
    upper=${2^^}
  else
    cap=${1^}
    NAME=${cap::8}
    upper=${1^^}
  fi
  INTERNAL=@${upper::7}

  # Copy initial files to project folder
  assets="$PREFIX/share/fxsdk/assets"
  mkdir -p "$1"/{,src,assets-fx,assets-cg}

  case "$generator" in
    "Makefile")
      sed -e "s/@NAME@/$NAME/g" -e "s/@INTERNAL@/$INTERNAL/g" \
        "$assets/project.cfg" > "$1/project.cfg"
      cp "$assets/Makefile" "$1";;
    "CMake")
      cp "$assets/CMakeLists.txt" "$1";;
  esac

  cp "$assets"/gitignore "$1"/.gitignore
  cp "$assets"/main.c "$1"/src
  cp "$assets"/icon-fx.png "$1"/assets-fx/icon.png
  cp "$assets"/icon-cg-uns.png "$1"/assets-cg/icon-uns.png
  cp "$assets"/icon-cg-sel.png "$1"/assets-cg/icon-sel.png
  cp -r "$assets"/assets-fx "$1"/
  cp -r "$assets"/assets-cg "$1"/

  echo "Created a new project $NAME (build system: $generator)."
  echo "Type 'fxsdk build-fx' or 'fxsdk build-cg' to compile the program."
}

fxsdk_load_config() {
  grep -E '^ *[a-zA-Z0-9_]+ *=' project.cfg \
  | sed -E 's/^([A-Z_]+)\s*=\s*(.*)/\1="\2"/' \
  | source /dev/stdin
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
      fxsdk_build_fx
      return
    fi

    if [[ $platform == "cg" ]]; then
      fxsdk_build_ch
      return
    fi

    echo -e 'Unknown platform (valid names are "fx" and "cg")!'
  done
}

fxsdk_build_fx() {
  # CMake version; automatically configure
  if [[ -e "CMakeLists.txt" ]]; then
    if [[ ! -e "build-fx" ]]; then
      cmake -B build-fx \
        -DCMAKE_MODULE_PATH="$PREFIX/lib/cmake/fxsdk" \
        -DCMAKE_TOOLCHAIN_FILE="$PREFIX/lib/cmake/fxsdk/FX9860G.cmake"
    fi
    make --no-print-directory -C build-fx
  # Makefile version
  else
    make all-fx
  fi
}

fxsdk_build_cg() {
  # CMake version; automatically configure
  if [[ -e "CMakeLists.txt" ]]; then
    if [[ ! -e "build-cg" ]]; then
      cmake -B build-cg \
        -DCMAKE_MODULE_PATH="$PREFIX/lib/cmake/fxsdk" \
        -DCMAKE_TOOLCHAIN_FILE="$PREFIX/lib/cmake/fxsdk/FXCG50.cmake"
    fi
    make --no-print-directory -C build-cg
  # Makefile version
  else
    make all-cg
  fi
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

case "$1" in
  # Project creation
  "new")
    fxsdk_new_project "${@:2}";;

  # Project compilation
  "build"|"b")
    fxsdk_build
    if [[ "$2" == "-s" ]]; then
      fxsdk_send
    fi;;
  "build-fx"|"bf"|"bfx")
    fxsdk_build_fx
    if [[ "$2" == "-s" ]]; then
      fxsdk_send_fx
    fi;;
  "build-cg"|"bc"|"bcg")
    fxsdk_build_cg
    if [[ "$2" == "-s" ]]; then
      fxsdk_send_cg
    fi;;

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
