# ggxrd_hitbox_overlay

## Credits

Original version created by Altimor: <http://www.dustloop.com/forums/index.php?/forums/topic/12495-xrd-pc-hitbox-overlay-mod/>
Created in 2016.
This version is adapted for Guilty Gear Xrd Rev2 version 2211.

Made with help from WorseThanYou.

## Build instructions

Open the `.sln` file, which is located in the parent folder from this readme, using Visual Studio (I'm using 2022 Community Edition). In the top bar select `Release` and `x86`.

After this, make sure the project will be compiled with UNICODE unset. Right-click the `ggxrd_hitbox_overlay` project in `Solution Explorer` and go to `Properties` -> `Advanced` -> `Character Set`. Select `Use Multi-Byte Character Set`. This setting is set separately for each `Release` + `x86` combination so it might change after changing the target platform.

Go to `Build` -> `Build Solution`.  
After building, the `.dll` file appears in `SOLUTION_ROOT\Release` folder (the `SOLUTION_ROOT` being where the `.sln` file is located).

## Dependencies

The project depends on:

- Microsoft Detours library: <https://github.com/microsoft/Detours> Follow their instructions on how to build the `.lib` static library. You need to build the 32-bit (x86) version.  
  In order to link the static library you must right click the project in `Solution Explorer`. Go to `Properties` -> `VC++ Directories`. Add the full path to the `Detours-main\include` folder in the `Include Directories` field.  
  Add the full path to the `Detours-main\lib.X86` directory to the `Library Directories` field.  
  In the project settings property pages go to `Linker` -> `Input` and add `detours.lib` into the `Additional Dependencies` field.  
  This project uses version 4.0.1 of Detours dated Apr 16, 2018.

- `d3dx9.h` header file. If you don't have it you can get it from: <https://github.com/apitrace/dxsdk/blob/master/Include/d3dx9.h>  
  Clone the full dxsdk repo. Right click your project in Visual Studio `Solution Explorer` and go to `Properties` -> `VC++ Directories`. Add the full path to the `dxsdk-master\Include` folder into the `Include Directories` folder.  
  The correct place to get this header would be from Microsoft DirectX SDK (June 2010) (for DirectX 9) which can be downloaded freely from Microsoft.  
  Add the full path to the `SDK_ROOT\Include` folder into the `Include Directories` folder. You don't need the `.lib` static libraries because you will get them from the Guilty Gear executable.

- `libpng` - a PNG encoder library. This is needed for the transparent/not-transparent screenshotting functionality. You should statically link its 32-bit verion into this mod. Its sources are not included in the mod in any way, you must download and build it yourself. libpng homepage: <http://www.libpng.org/pub/png/libpng.html>  
After you download the sources, make sure to also download the `zlib` (<https://www.zlib.net/>) sources and put the `zlib` sourcetree directory in the parent directory relative to `libpng` sourcetree and rename the `zlib` sourcetree directory to `zlib` if it's not already named that. Just to recap, the directory tree should look like this:

- The directory containing both `libpng` and `zlib`:
  - `libpng` sourcetree directory - can be named anything
  - `zlib` sourcetree directory - must be named exactly `zlib`

You don't need to compile `zlib` separately because it will get included as a subproject in `libpng` and will get compiled when you build the `libpng` solution.

In the `libpng` folder go to `projects\vstudio` and open the `vstudio.sln` solution file in Visual Studio. If Visual Studio asks to Retarget Projects, agree.  
In the top bar of Visual Studio choose build configuration `Release Library` (just `Release` seems to compile a DLL (with a `.lib` file, but that `.lib` requires the DLL actually still), while `Release Library` compiles a static `.lib` library) and `Win32`.  
Then go to `Build` -> `Build Solution`. Everything should succeed. The build files will be written to `projects\vstudio\Release Library`. We need the `libpng16.lib` and the `zlib.lib` files.  
Open the `ggxrd_hitbox` solution, right-click the `ggxrd_hitbox_overlay` project and select `Properties`. Navigate to `VC++ Directories` and add the full path to the `libpng` sourcetree into the `Include Directories`. Add the full path to `libpng`'s `projects\vstudio\Release Library` directory into the `Library Directories` field. Navigate to `Linker` -> `Input` and add `libpng16.lib` and `zlib.lib` into the `Additional Dependencies` field.  
This will include `libpng` and `zlib` statically (meaning it's included into whatever you've built) into the `ggxrd_hitbox_overlay.dll` so you don't need to ship anything extra with the `ggxrd_hitbox_overlay.dll` to get PNG functionality working.

## Main README

The main README of the entire solution is located in the root of the git repository.
