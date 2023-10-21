# ggxrd_hitbox_overlay_2211

## Description

Adds hitboxes overlaid on top of characters/projectiles for Guilty Gear Xrd Rev2 version 2211 (as of 8'th October 2023).

## Credits

Original version created by Altimor: <http://www.dustloop.com/forums/index.php?/forums/topic/12495-xrd-pc-hitbox-overlay-mod/>  
Created in 2016.  
This version is adapted for Guilty Gear Xrd Rev2 version 2211 with half the features removed as of yet (still trying to find where everything is).

Many thanks to WorseThanYou, without whose help this wouldn't have been possible.

## System requirements

Intel processor architecture x86 (32-bit) or x64 (64-bit) (AMD not supported). Windows operating system.

## Quickstart

1. Launch the game. Or you can do the next step at any point while the game is running.

2. Go to `bin` folder and launch `ggxrd_hitbox_injector.exe`.

3. Start a match. Hitboxes should display.

To turn off the mod you can launch `ggxrd_hitbox_injector.exe` again.

The mod may show up as a virus. I swear this is not a virus, check the source code, compile this yourself if still doubting. Check commit history of this repo to see no one else but me modified this. Add this to whatever antivirus exceptions necessary and run as administrator if necessary.

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

- Some invulnerability checks are still missing;
- OTG detection is missing;
- Missing pushboxes, they don't even display (not found yet) (note: they seem to play an important role in throw hit detection);
- Missing throw hitboxes/throw invulnerability checks/all of the throw stuff;
- Missing GIF mode and "no gravity" mode;

## Changelog

- 2023 October 13: Now hitboxes belonging to the same group may be displayed as a single shape with one combined outline;
- 2023 October 16: Added Unicode support to the injector, meaning you should be able to include any non-english characters in the path to the directory in which the injector and the .dll reside;  
                   Tweaked hitbox drawing so that outlines always draw on top of all hitboxes, hitboxes always draw on top of all hurtboxes.  
                   Restricted the hitbox drawing to only non-online matches until I figure out a way to tell if Chipp is doing the invisibility thing in an online match, in which case his boxes should not display.
- 2023 October 21: Now boxes won't be drawn for Chipp & his projectiles in online mode if he's invisible.  
                   Thanks to WorseThanYou's help, added counterhit state to hurtboxes. Your hurtbox will be blue if, should you get hit, you will be in a counter hit state.  
                   Boxes now don't show if a menu is open in training and versus modes (online, episode and single player MOM mode not yet tested).
                   Boxes now don't show if an Instant Kill cutscene is currently playing.
                   Now hitboxes show only during active frames, i.e. there's no longer a problem of them showing during recovery. Hitboxes never show as transparent, only filled in.  
                   Hitboxes keep showing even after an attack connects, as long as the attack's active frames are going on.  
                   Millia's Tandem Top and Sol's Gunflame now show hitbox for a brief period after hitting a target so you could actually see the hitbox on hit.  
                   Added invulnerability check related to startup of supers, such as May's temper tantrum, and possibly many other moves.  
                   Added invulnerability check related to throws but it's incomplete yet, Slayer still shows up as vulnerable for the remainder of his ground throw.  
                   You can now unload the dll after it has been loaded, reverting the game back to normal without having to restart it.  
                   Added proper hitbox display for attached entities like Ky's, I-No's and Millia's Dust (5D) attacks.