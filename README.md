# Console-Friendly File Manager
(indev)

CFFM is a minimalist terminal-based file manager. Heavily inspired by similar terminal based file managers such as `ranger` and `lf`, CFFM seeks to provide a clean, snappy, and customizable file managing experience.

## What's New [v0.4.0]
-Execute shell commands in the current directory with `:`.

-Various bug fixes

-Fixed all memory leaks.

## Download
You can download a static "just works" binary from the releases section when CFFM 1.0 releases.

## Build and Install
Dependencies:

'libncurses-dev' (Debian)

'ncurses' (Arch)

Compiling:

`make` for a slim, dynamically linked binary.

`make release` for a static binary.

Installing:

`make install`

## Uninstall CFFM
`sudo make uninstall`

# Configuration
You can modify any `#define` value in `config.h` to your liking. When done, simply re-compile and run.
