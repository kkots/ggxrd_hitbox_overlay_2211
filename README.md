# ggxrd_hitbox_overlay_2211

## Description

Adds hitboxes overlaid on top of characters/projectiles for Guilty Gear Xrd Rev2 version 2211 (as of 27'th October 2023).

## Credits

Original version created by Altimor: <http://www.dustloop.com/forums/index.php?/forums/topic/12495-xrd-pc-hitbox-overlay-mod/>  
Created in 2016.  
This version is adapted for Guilty Gear Xrd Rev2 version 2211 with half the features removed as of yet (still trying to find where everything is).

Many thanks to WorseThanYou, without whose help this wouldn't have been possible.

## System requirements

Intel processor architecture x86 (32-bit) or x64 (64-bit) (AMD will work). Windows operating system.

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

## Development dependencies

Dependencies are better described in each project's README.md. Short version is, the project depends on:

- Microsoft Detours library: <https://github.com/microsoft/Detours> Follow their instructions on how to build the `.lib` static library. You need to build the 32-bit (x86) version.

- `d3dx9.h` header file. If you don't have it you can get it from: <https://github.com/apitrace/dxsdk/blob/master/Include/d3dx9.h>

## Missing features list

- OTG detection. It's supposed to count as throw invulnerability;
- Throw hitboxes/some throw invulnerability checks. For example, when backdashing you don't show as throw invulnerable;
- GIF mode and "no gravity" mode;

## Changelog

- 2023 October 13: Now hitboxes belonging to the same group may be displayed as a single shape with one combined outline;
- 2023 October 16: Added Unicode support to the injector, meaning you should be able to include any non-english characters in the path to the directory in which the injector and the .dll reside;  
                   Tweaked hitbox drawing so that outlines always draw on top of all hitboxes, hitboxes always draw on top of all hurtboxes.  
                   Restricted the hitbox drawing to only non-online matches until I figure out a way to tell if Chipp is doing the invisibility thing in an online match, in which case his boxes should not display.
- 2023 October 21: Now boxes won't be drawn for Chipp & his projectiles in online mode if he's invisible. The mod in all other cases will display boxes in online mode.  
                   Thanks to WorseThanYou's help, added counterhit state to hurtboxes. Your hurtbox will be blue if, should you get hit, you will be in a counter hit state.  
                   Boxes now don't show if a menu is open or an Instant Kill cutscene is currently playing.
                   Now hitboxes show only during active frames, i.e. there's no longer a problem of them showing during recovery. Hitboxes never show as transparent, only filled in.  
                   Hitboxes keep showing even after an attack connects, as long as the attack's active frames are going on.  
                   Millia's Tandem Top and Sol's Gunflame now show hitbox for a brief period after hitting a target so you could actually see the hitbox on hit.  
                   Added invulnerability check related to startup of supers, such as May's temper tantrum, and possibly many other moves.  
                   Added invulnerability check related to throws but it's incomplete yet, Slayer still shows up as vulnerable for the remainder of his ground throw.  
                   You can now unload the dll after it has been loaded, reverting the game back to normal without having to restart it.  
                   Added proper hitbox display for attached entities like Ky's, I-No's and Millia's Dust (5D) attacks.
- 2023 October 27: Major refactoring.  
                   Fixed hitboxes display for rotated projectiles like Ky's Stun Edge. Previously it was showing as rotated boxes which is not how the game actually checks if it hits. Now it's showing as a box ladder which is correct.  
                   Now while doing a throw you display as strike invulnerable throughout the entire duration of the throw, not just part of it.  
                   Made counterhit state not display if you're strike invulnerable.  
                   Made counterhit state not display on summons or anyone but the main player entities.  
                   Added pushboxes.  
                   Fixed an issue when all boxes were always drawn twice per frame, making them more opaque. Now they should be more transparent.  
                   Moved the binaries into Github's Releases section of this project.

## TODO

- Display throw invulnerability when backdashing.
- Throw boxes
- Missing OTG flag
- Show counterhit for longer after a hit connects in counterhit state
- When a hitbox connects, the one who got hit changes their hurtbox very fast and we can't see what exactly caused the hit.  
  Make it so that the previous hitbox still displays with the same stencil buffer and its outlines are drawn fully together with the current
  box's outlines. All outlines without stencil. Use gray color and gray outline for the old box, always filled (ignoring if the
  player was strike invul at the moment of getting hit). The gray box must be drawn behind the green/blue box with stencil.
- Show Ky Grinders and similar summons, if any such exist in the game, as invulnerable, if they have a hurtbox but can't actually be hit by the other player.  
  Jack-O houses and minions, Bedman's deja'vus (i.e. all summons that can be a lot of and that can be hit) extra transparent and thinner outlines.  
  Summons like Dizzy's fish, Elphelt's pineberry, Eddie that cannot be a lot of and can be hit - show normally.
- Show Jack-O Aegis Field as extra transparent.
- Don't display invulnerability if it's from a Dust homing dash cinematic or overdrive super freeze (if overdrive doesn't actually give any invulnerability).
- Find is_push_active function.
- Not show boxes on Episode and single player MOM mode victory/defeat screen
- Not show boxes during episode mode in-game dialogue cutscenes
- (impossible) EndScene and Present get called in a different thread from where the game logic is happening,
  hence sometimes there are artifacts when boxes are drawn twice onto a frame in different states or the boxes are one frame ahead of what's on the frame.
