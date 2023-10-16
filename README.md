# ggxrd_hitbox_overlay_2211

## Description

Adds hitboxes overlaid on top of characters/projectiles for Guilty Gear Xrd Rev2 version 2211 (as of 8'th October 2023).

## Credits

Original version created by Altimor: <http://www.dustloop.com/forums/index.php?/forums/topic/12495-xrd-pc-hitbox-overlay-mod/>  
Created in 2016.  
This version is adapted for Guilty Gear Xrd Rev2 version 2211 with 90% features removed as of yet (still trying to find where everything is).

Made with help from WorseThanYou.

## System requirements

Intel processor architecture x86 (32-bit). Windows operating system.

## Quickstart

1. Launch the game. Or you can do the next step at any point while the game is running.

2. Go to `bin` folder and launch `ggxrd_hitbox_injector.exe`.

3. Start a match. Hitboxes should display.

To turn off the mod the only way is to restart the game.  
You should not launch `ggxrd_hitbox_injector.exe` more than once during the game.

## Developing

There are two separate projects in the `src` directory.

The `ggxrd_hitbox_injector` project builds an application that will inject a dll into the process and exit. The main action will then take place in the dll, in the target process' address space.

The `ggxrd_hitbox_overlay` project builds the dll that's responsible for drawing the hitboxes.

Each project should have its own separate README.md.

You need to build the `Release` version for `x86` (32-bit) platform (not x64) (it's specified at the top of Visual Studio window). Place the `ggxrd_hitbox_overlay.dll` and `ggxrd_hitbox_injector.exe` both in the same directory in `bin` folder, start the game and launch `ggxrd_hitbox_injector.exe`.

## Development dependencies

The project depends on:

- Microsoft Detours library: <https://github.com/microsoft/Detours> Follow their instructions on how to build the `.lib` static library. You need to build the 32-bit (x86) version.

- `d3dx9.h` header file. If you don't have it you can get it from: <https://github.com/apitrace/dxsdk/blob/master/Include/d3dx9.h>

## Missing features list

- Some invulnerability checks are missing;
- OTG detection is missing;
- Some entities will be drawn even though they are inactive (missing check);
- Missing pushboxes, they don't even display (not found yet) (note: they seem to play an important role in throw hit detection);
- Missing "attached" entities like dust (I don't even know what that is, it seems important);
- Missing throw hitboxes/throw invulnerability checks/all of the throw stuff;
- Missing counterhit detection;
- Missing GIF mode and "no gravity" mode;

## Changelog

- 2023 October 13: Now hitboxes belonging to the same group may be displayed as a single shape with one combined outline;
- 2023 October 16: Added Unicode support to the injector, meaning you should be able to include any non-english characters in the path to the directory in which the injector and the .dll reside;
                   Tweaked hitbox drawing so that outlines always draw on top of all hitboxes, hitboxes always draw on top of all hurtboxes.
                   Restricted the hitbox drawing to only non-online matches until I figure out a way to tell if Chipp is doing the invisibility thing in an online match, in which case his boxes should not display.
