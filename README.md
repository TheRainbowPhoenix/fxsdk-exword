# fxSDK

The fxSDK is a development kit for CASIO graphing calculators in the fx-9860G
and fx-CG 50 families. It provides command-line helper tools and build systems
for add-ins and libraries, and is commonly used to develop add-ins running the
[gint kernel](/Lephenixnoir/gint).

The fxSDK is free software; you may use it for any purpose, share it, modify
modify it and share your changes. No credit required, but please let me know!

The fxSDK is compatible with Linux and has been successfully built on Mac OS.
If there are compatibility issues, I am willing to try and port stuff to your
favorite operating system. Windows users have good support with WSL.

## Basic install and use

The simplest way to install the fxSDK is to use
[GiteaPC](/Lephenixnoir/GiteaPC), an automation tool that builds and installs
repositories from Planète Casio's Gitea forge.

```bash
% giteapc install Lephenixnoir/fxsdk
```

The fxSDK depends on the [`sh-elf-gcc` compiler](/Lephenixnoir/sh-elf-gcc) so
GiteaPC might build it too as a dependency. You will also need the PIL library
for Python:

```bash
# On Debian, Ubuntu, WSL and the like:
% sudo apt install python3-pil
# On Arch Linux, Manjaro and the like:
% sudo apt install python-pillow
```

Use the `fxsdk` command to manage projects. You can create an empty add-in
project with `fxsdk create` and a name for a new folder:

```bash
% fxsdk create MyAddin
```

From that folder, you can build the add-in with the build commands:

```bash
# Build the add-in for fx-9860G (.g1a):
% fxsdk build-fx
# Build the add-in for fx-CG 50 (.g3a):
% fxsdk build-cg
```

## Tools in the fxSDK

A tool called *fxos* used to live here and has now moved to [its own
repository](/Lephenixnoir/fxos).

**Project management** with `fxsdk`

`fxsdk` lets you set up projects almost instantly with a default folder
structure and a build system for both fx-9860G and fx-CG 50. The default build
system is CMake since version 2.3, and a bare-bones Makefile is also supported.

`fxsdk` only writes files at project creation time, so you keep control over
your build system and configuration - it just helps you get started faster.

Summary of commands (`fxsdk --help` for details):

* `fxsdk new`: Create a new project
* `fxsdk build/build-fx/build-cg`: Configure and compile add-ins and libraries
* `fxsdk send/send-fx/send-cg`: Install files to the calculator (WIP)

**G1A file generation** with `fxg1a`

`fxg1a` is a versatile g1a file editor that creates, edits and dumps the header
of fx-9860G add-ins files. It is used to build a g1a file out of a binary
program.

It supports PNG icons, checking the validity and checksums of the header,
repairing broken headers and dumping both the application data and icon.

`fxg1a` is called automatically by the build system in your add-in, so you
don't need to worry about it, but here are the main commands:

* `fxg1a -g`: Generate g1a files
* `fxg1a -e`: Edit g1a files
* `fxg1a -d`: Dump metadata, checksum, and icon
* `fxg1a -r`: Repair control bytes and checksums for broken files
* `fxg1a -x`: Extract icon into a PNG file

**Asset conversion** with `fxconv`

`fxconv` is a programmable asset converter that converts images, fonts and
other common asset types into data structures usable directly in add-ins. The
built-in formats include gint images and fonts, [libimg](/Lephenixnoir/libimg)
images, and binary blobs.

Projects can extend the support to custom types for maps, dialogs, GUI
descriptions, or other application-specific assets. Extensions to `fxconv` are
implemented in Python within the project.

`fxconv` can be used directly on the command-line but normally you specify
parameters in `fxconv-metadata.txt` and let the build system do the magic.

## Manual build instructions

The fxSDK is platform-agnostic; a single install will cover any target
platforms. Here are the dependencies:

* libpng ≥ 1.6
* Python ≥ 3.7 (might work in 3.6)
* The Pillow library for Python 3

First configure; you can specify the install folder with `--prefix`, which
defaults to your local home folder. You can also enable or disable tools,
although normally all of them are needed for add-in development. See
`./configure --help` for details.

```sh
% ./configure
```

Then make and install as usual.

```sh
% make
% make install
```

If you selected an install folder for which you don't have write access (which
apparently includes the default folder on Mac OS), you will need `sudo` to
install.
