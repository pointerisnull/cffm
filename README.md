# Console-Friendly File Manager
(indev)

CFFM is a minimalist terminal-based file manager. Heavily inspired by similar terminal based file managers such as `ranger` and `lf`, CFFM seeks to provide a clean, snappy, and customizable file managing experience.

## Download
You can download a static "just works" binary from the releases section when CFFM 1.0 releases.

## Build and Install
Dependencies:
-libncurses-dev (Debian)
-ncurses (Arch/Gentoo)

Compiling:
`make` for a slim, dynamically linked binary.
`make release` for a static binary.

Installing:
    `make install`

## Uninstall CFFM
    `sudo make uninstall`

# Configuration 
## Using a Config File (Default)
`/home/USER/.config/cffm.conf`
If any option is deleted, not present, or invalid, CFFM will automatically use a hard-coded default.

## Configure Without an External Config File
If you would prefer to not use a config file and rather hard-code your preferences into CFFM, you can simply change the constant USECONFIGFILE, located in `config.h` from 1 to 0. Then set whatever values you would like to the below constants, and compile CFFM.
