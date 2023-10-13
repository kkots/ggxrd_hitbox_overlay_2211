# ggxrd_hitbox_overlay

## Credits

Original version created by Altimor: <http://www.dustloop.com/forums/index.php?/forums/topic/12495-xrd-pc-hitbox-overlay-mod/>
Created in 2016.
This version is adapted for Guilty Gear Xrd Rev2 version 2211 with 90% features removed as of yet (still trying to find where everything is).

Made with help from WorseThanYou.

## Build instructions

Open the `.sln` file using Visual Studio (I'm using 2022 Community Edition). In the top bar select `Release` and `x86`.

After this, make sure the project will be compiled with UNICODE unset. Right-click the project in `Solution Explorer` and go to `Properties` -> `Advanced` -> `Character Set`. Select `Use Multi-Byte Character Set`. This setting is set separately for each `Release` + `x86` combination so it might change after changing the target platform.

Go to `Build` -> `Build Solution`.  
After building, the `.dll` file appears in `Release` folder.

## Dependencies

The project depends on:

- Microsoft Detours library: <https://github.com/microsoft/Detours> Follow their instructions on how to build the `.lib` static library. You need to build the 32-bit (x86) version.
  In order to link the static library you must right click the project in `Solution Explorer`. Go to `Properties` -> `VC++ Directories`. Add the full path to the `Detours-main\include` folder in the `Include Directories` field.
  Add the full path to the `Detours-main\lib.X86` directory to the `Library Directories` field.
  In the project settings property pages go to `Linker` -> `Input` and add `detours.lib` into the `Additional Dependencies` field.

- `d3dx9.h` header file. If you don't have it you can get it from: <https://github.com/apitrace/dxsdk/blob/master/Include/d3dx9.h>
  Clone the full dxsdk repo. Right click your project in Visual Studio `Solution Explorer` and go to `Properties` -> `VC++ Directories`. Add the full path to the `dxsdk-master\Include` folder into the `Include Directories` folder.
