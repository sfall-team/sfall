# sfall

[![License](https://img.shields.io/badge/License-GPL--3.0-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Dev Build](https://github.com/phobos2077/sfall/actions/workflows/build.yml/badge.svg?branch=develop)](https://github.com/phobos2077/sfall/actions/workflows/build.yml)
[![GitHub Pages](https://github.com/phobos2077/sfall/actions/workflows/gh-pages.yml/badge.svg)](https://github.com/phobos2077/sfall/actions/workflows/gh-pages.yml)

A set of engine modifications for the classic game Fallout 2 in the form of a DLL, which modifies executable in memory without changing anything in EXE file itself.

**Engine modifications include:**
- Better support for modern operating systems
- Externalizing many settings like starting map and game time, skills, perks, critical hit tables, books, etc.
- Bug fixes
- Many additional features for users, such as item highlight button, party member control, etc.
- Extended scripting capabilities for modders (many new opcodes to control sfall features as well as previously unavailable vanilla engine functions)

Original author: **Timeslip**

Original description: A set of engine modifications for the classic game Fallout 2 by Interplay. Includes fixes for bugs in the original engine, allows Fallout to run correctly on modern operating systems, and adds additional features for modders.

## Installation

- Download `sfall_*.7z` from the [release archive](https://sourceforge.net/projects/sfall/files/).

- Extract `ddraw.dll`, `ddraw.ini`, `sfall.dat`, and the `mods` folder to Fallout's base directory (i.e. the one that contains `fallout2.exe`). Also, remove `gl_highlighting.int` and `gl_partycontrol.int` from Fallout's `data\scripts\` directory if you have them.

- __Important Note:__\
  If you are using a mod that already included sfall (e.g. killap's [Unofficial Patch](https://github.com/BGforgeNet/Fallout2_Unofficial_Patch) or [Restoration Project](https://github.com/BGforgeNet/Fallout2_Restoration_Project)), then that mod has probably included a custom modified `ddraw.ini`. In that case, overwriting it with sfall's vanilla `ddraw.ini` will be likely break your game. Instead, only overwrite `ddraw.dll`, and keep the mod's existing copy of `ddraw.ini`. (Or, if you know what you're doing, you can merge them together by hand.)

- The folder `translations` contains translations of some of the strings that sfall displays in the game. To use a translation, copy this folder to Fallout's base directory too, and then in `ddraw.ini` change the __TranslationsINI__ setting to `.\translations\<your language>.ini`.

## Uninstallation

Delete `ddraw.dll`, `ddraw.ini`, and `sfall.dat` from your Fallout directory, and delete `sfall-mods.ini` from the `mods` folder.

## Usage

This mod is configured via the `ddraw.ini` and `sfall-mods.ini` files, which can be opened with any text editor. Details of every configerable option are included in those files. Where a comment refers to a DX scancode, the complete list of codes can be found at the link below:\
https://kippykip.com/b3ddocs/commands/scancodes.htm

In a default installation using an unmodified copy of `ddraw.ini`, the middle mouse button will be set to switch between weapons and the mouse wheel will be set to scroll through any menus that respond to the up/down arrow keys. Holding Ctrl and hitting numpad keys 0 to 6 (with Num Lock off) will adjust the game speed. Holding left Ctrl will let you move a whole stack of items at once. Pressing left Shift will highlight items on the ground. The script extender and any engine fixes are also enabled. Most of the options that change gameplay in some way not originally intended by the developers are disabled.

For [__Wine__](https://www.winehq.org/) users:\
You need to set DLL overrides for `ddraw.dll` to __"native, builtin"__ in `winecfg` or use `WINEDLLOVERRIDES="ddraw=n,b"` to run Fallout from the command line. If you want to play alternative sound files, you'll also need to install GStreamer Good 32-bit plugins.

## Additional info

* [Changelog](CHANGELOG.md)
* [Scripting Documentation](https://sfall-team.github.io/sfall/)
* Fallout Engine IDA Database: [for IDA Pro 6.8](https://www.dropbox.com/s/tm0nyx0lnk4yui0/Fallout_1_and_2_IDA68.rar?dl=1 "Download from Dropbox") | [for IDA Pro 7.0](https://www.dropbox.com/s/61srq09pn8grfpu/Fallout_1_and_2_IDA70.rar?dl=1 "Download from Dropbox") (comments are in Russian)
* [Fallout 2 Reference Edition](https://github.com/alexbatalov/fallout2-re)
