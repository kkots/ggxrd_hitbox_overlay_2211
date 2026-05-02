# ggxrd_hitbox_patcher (GUI version)

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

## Run instructions (Windows)

Double-click the patcher executable. It'll output some greeting. Press Enter. It will prompt you to select the GuiltyGearXrd.exe file. This file can be found in the game's installation directory's Binaries\Win32 folder. To locate the installation directory you can open Steam Library, right-click it, Manage - Browse local files. From there, go to Binaries\Win32 and you'll find GuiltyGearXrd.exe. Once handed the file, the patcher will patch it. Then, it is your job to place the ggxrd_hitbox_overlay.dll file into the game's Binaries\Win32 folder. If the DLL isn't there, and the game is patched, the game will work normally, but won't load the mod on startup.

## Run instructions (Linux)

To start off, on Linux you probably want to compile the console version of the patcher (ggxrd_hitbox_patcher_console), as that may run natively, without Wine. But if you want the GUI, it must run on Wine.

Put the compiled patcher and launch_ggxrd_hitbox_injector_linux.sh together into one folder and cd into it. Then launch the .sh script with an argument containing the patcher's name:

```bash
./launch_ggxrd_hitbox_injector_linux.sh ggxrd_hitbox_patcher.exe
```

The argument makes it launch that program, instead of the injector (the default).

After that, follow the Run instructions for Windows.

## Main README

The main README of the entire project is located in the root of the git repository.
