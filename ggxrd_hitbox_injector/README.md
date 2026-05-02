# ggxrd_hitbox_injector (GUI version)

## Credits

Original version created by Altimor: <http://www.dustloop.com/forums/index.php?/forums/topic/12495-xrd-pc-hitbox-overlay-mod/>
Created in 2016.
This version is adapted for Guilty Gear Xrd Rev2 version 2211.

Made with help from WorseThanYou.

## Build instructions on Windows

Open the `.sln` file using Visual Studio (I'm using 2022 Community Edition). In the top bar select `Release` and `x86`. Go to `Build` -> `Build Solution`.

After building, the `.exe` file appears in `Release` folder.

## Build instructions on Linux with MinGW GCC

Use the GNUmakefile by running:

```
cd ggxrd_hitbox_injector
make CHOST=stuff
```

For example, if your installed MinGW GCC toolchain is i686-w64-mingw32-gcc, note there're a bunch of other tools starting with i686-w64-mingw32, so the CHOST must be set to `i686-w64-mingw32`.

## Dependencies

Comctl32.lib for SetWindowSubclass, DefSubclassProc.

## Run instructions

This program needs the `ggxrd_hitbox_overlay.dll` in order to function correctly (this dll is built by the other project, check it out).  
On Windows, you should be opening both projects as a single solution in Visual Studio, and when you launch the program either through Visual Studio's F5 or Debug or similar functionality, or when you launch the executable directly by double-clicking it, it should search for the `.dll` file in the `SOLUTION_ROOT\Release` or `SOLUTION_ROOT\Debug` folder (`SOLUTION_ROOT` being where the `.sln` file is located), and when building the solution it should place both `.exe` and `.dll` in that folder automatically.

On Linux, build the DLL part using build.py residing in that project's folder. Keep in mind, that wherever the injector is, it will look for the DLL in the current working directory at the time of launching the injector, so if you launched the injector in some crooked way like:
```bash
WINEPREFIX=stuff WINEFSYNC=1 subdir/ggxrd_hitbox_injector.exe
```
, the injector will look not in subdir, but in what the value of your pwd was when you launched it.  
For an explanation of WINEPREFIX, see launch_ggxrd_hitbox_injector_linux.sh.

## Main README

The main README of the entire project is located in the root of the git repository.
