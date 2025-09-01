# 🧱 KMAK Build System

KMAK (Komodore Make) is a lightweight and minimalist build system written in C.<br>
It is inspired by Makefile but much simpler with custom tasks support.<br>
![Build](https://img.shields.io/badge/build-kmak-lightgrey?style=flat-square&logo=c) ![Lang](https://img.shields.io/badge/language-C-blue?style=flat-square&logo=c) ![License](https://img.shields.io/badge/license-GPL-green?style=flat-square) ![Status](https://img.shields.io/badge/status-WIP-orange?style=flat-square)

## Building
To build KMAK, run
```
kmak.exe make.kmk compile
```
from the main directory. It will produce a `kmak0.exe`/`kmak0` file that you can then rename `kmak.exe`/`kmak` to use.

## Starting
The example below is the one for the Windows version, but building on Linux is the same.

```
usage: kmak InputFile <Task>
```

For example with `make.kmk` in `example`:
```
# Variables
CC = gcc
SRC = game.c
TARGET = game.exe

# Tasks
task clean
	print Cleaning...
	cmd del $(TARGET)

task run
	print Running...
	cmd $(TARGET)

task make
	print Making project...
	cmd $(CC) -o $(TARGET) $(SRC)
```

You can execute:
```
kmak make.kmk make
```
or...:
```
kmak make.kmk run
```
To execute game.exe.
