# fxSDK

The fxSDK is a development kit for Casio graphing calculators of the family of
the fx-9860G and fx-CG 50. It's a set of command-line compilation and project
tools used to facilitate development with the
[gint kernel](/Lephenixnoir/gint).

This kit is free software; you may use it for any purpose, share it, and modify
it as long as you share your changes. No credit required, but please let me
know!

The fxSDK is compatible with Linux and has been successfully built on Mac OS.
If there are compatibility issues, I am willing to try and port stuff to your
favorite operating system.

## Tool description

### Project management (_fxsdk_)

`fxsdk` lets you set up projects almost instantly with a default folder
structure and a working Makefile linking against gint for both fx-9860G and
fx-CG 50. You can then use it to build and transfer your add-ins to a
calculator.

Most project parameters can be changed just by editing a text file annotated
with the role and description of each option.

`fxsdk` only writes files at project creation time, so you keep control over
your build system and configuration - it just helps you get started faster.

### G1A file generation (_fxg1a_)

`fxg1a` is a versatile g1a file editor that creates, edits and dumps the header
of fx-9860G add-ins files. It is used to build a g1a file out of a binary
program.

It supports PNG icons, checking the validity and checksums of the header,
repairing broken headers and dumping both the application data and icon.

### Data conversion (_fxconv_)

`fxconv` is a tool that interacts specifically with gint. It converts data
files such as images and fonts into gint-specific format and embeds the result
into object files that expose a single variable.

### OS disassembly and study (_fxos_)

`fxos` is a powerful disassembler and OS fiddling tool. It knows the structure
of OS files and can extract metadata, language information and syscalls through
a simple command-line interface.

Its key feature is the production of annotated assembler listings where
syscalls invocations and peripheral registers are identified and named. This
makes it a lot easier to climb back abstraction levels from the code.

## Build instructions

The fxSDK is platform-agnostic; a single install will cover any target
platforms. Here are the dependencies:

* libpng ≥ 1.6
* Python ≥ 3.7 (might work in 3.6)
* The Pillow library for Python 3

First configure; you can specify the install folder with `--prefix`, which
defaults to your local home folder. You can also enable or disable tools.
Broadly, you will need:

* `fxsdk` if you want to benefit from the automated project setup
* `fxg1a` if you want to compile add-ins for fx-9860G
* `fxconv` if you want to compile gint or develop add-ins with it
* `fxos` if you want to disassemble or study OS dumps

Each tool can be enabled or disabled with `--enable-<tool>` and
`--disable-<tool>`. For a default build you need no arguments:

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

## Usage instructions

Most details are covered in the help of each individual tool, which you can get
with `-h`, `--help` or by invoking each tool without arguments.
