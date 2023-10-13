# ggxrd_hitbox_injector

## Credits

Original version created by Altimor: <http://www.dustloop.com/forums/index.php?/forums/topic/12495-xrd-pc-hitbox-overlay-mod/>
Created in 2016.
This version is adapted for Guilty Gear Xrd Rev2 version 2211 with 90% features removed as of yet (still trying to find where everything is).

Made with help from WorseThanYou.

## Build instructions

Open the `.sln` file using Visual Studio (I'm using 2022 Community Edition). In the top bar select `Release` and `x86`.

After this, make sure the project will be compiled with UNICODE unset. Right-click the project in `Solution Explorer` and go to `Properties` -> `Advanced` -> `Character Set`. Select `Use Multi-Byte Character Set`. This setting is set separately for each `Release` + `x86` combination so it might change after changing the target platform.

Go to `Build` -> `Build Solution`.

After building, the `.exe` file appears in `Release` folder.

## Dependencies

None/I don't know yet

## Run instructions

This program needs the `ggxrd_hitbox_overlay.dll` in order to function correctly (this dll is built by the other project, check it out). If you're launching through Visual Studio's F5 or Debug or similar functionality, place the `.dll` file directly into the solution's folder (where the `.sln` file is).
If you're launching the program by double-clicking it or similar method, place the `.dll` file into the same folder with it.
