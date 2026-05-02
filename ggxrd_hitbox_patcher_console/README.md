# ggxrd_hitbox_patcher (console version)

## Build instructions on Windows

Open the `.sln` file using Visual Studio (I'm using 2022 Community Edition). In the top bar select `Release` and `x86`. Go to `Build` -> `Build Solution`.

After building, the `.exe` file appears in `Release` folder.

## Build instructions on Linux with MinGW GCC

Use CMake:

```
cd ggxrd_hitbox_patcher_console
cmake -DCMAKE_BUILD_TYPE=Release .
make
```

We're not cross-compiling, the patcher is supposed to run natively on Linux.

## Dependencies

None.

## Run instructions (Windows)

Double-click the patcher executable. It'll output some greeting. Press Enter. It will prompt you to select the GuiltyGearXrd.exe file. This file can be found in the game's installation directory's Binaries\Win32 folder. To locate the installation directory you can open Steam Library, right-click it, Manage - Browse local files. From there, go to Binaries\Win32 and you'll find GuiltyGearXrd.exe. Once handed the file, the patcher will patch it. Then, it is your job to place the ggxrd_hitbox_overlay.dll file into the game's Binaries\Win32 folder. If the DLL isn't there, and the game is patched, the game will work normally, but won't load the mod on startup.

## Run instructions (Linux)

The console version of the patcher runs natively on Linux:

```bash
./ggxrd_hitbox_patcher_console
```

After that, follow the Run instructions for Windows.

## Main README

The main README of the entire project is located in the root of the git repository.
