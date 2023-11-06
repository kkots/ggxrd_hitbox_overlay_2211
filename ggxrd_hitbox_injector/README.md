# ggxrd_hitbox_injector

## Credits

Original version created by Altimor: <http://www.dustloop.com/forums/index.php?/forums/topic/12495-xrd-pc-hitbox-overlay-mod/>
Created in 2016.
This version is adapted for Guilty Gear Xrd Rev2 version 2211.

Made with help from WorseThanYou.

## Build instructions

Open the `.sln` file using Visual Studio (I'm using 2022 Community Edition). In the top bar select `Release` and `x86`. Go to `Build` -> `Build Solution`.

After building, the `.exe` file appears in `Release` folder.

## Dependencies

None/I don't know yet

## Run instructions

This program needs the `ggxrd_hitbox_overlay.dll` in order to function correctly (this dll is built by the other project, check it out).  
You should be opening both projects as a single colution in Visual Studio, and when you launch the program either through Visual Studio's F5 or Debug or similar functionality, or when you launch the executable directly by double-clicking it, it should search for the `.dll` file in the `SOLUTION_ROOT\Release` or `SOLUTION_ROOT\Debug` folder (`SOLUTION_ROOT` being where the `.sln` file is located), and when building the solution it should place both `.exe` and `.dll` in that folder automatically.

## Main README

The main README of the entire project is located in the root of the git repository.
