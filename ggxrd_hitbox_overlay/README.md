# ggxrd_hitbox_overlay

## Credits

Original version created by Altimor: <http://www.dustloop.com/forums/index.php?/forums/topic/12495-xrd-pc-hitbox-overlay-mod/>
Created in 2016.
This version is adapted for Guilty Gear Xrd Rev2 version 2211.

Made with help from WorseThanYou.

## Build instructions

Open the `.sln` file, which is located in the parent folder from this readme, using Visual Studio (I'm using 2022 Community Edition). In the top bar select `Release` and `x86`.

Go to `Build` -> `Build Solution`.  
After building, the `.dll` file appears in `SOLUTION_ROOT\Release` folder (the `SOLUTION_ROOT` being where the `.sln` file is located).

## Dependencies

The project depends on:

- Microsoft Detours library: <https://github.com/microsoft/Detours> It is included in this repository as a git submodule, you need not obtain it on your own. Our goal is to build the `.lib` static library of it and statically link it with the program, so there's no need to ship any extra DLL with the EXE. You need to build the 32-bit (x86) version, but again, all settings should already be prepared for you and you just need to hit Build.  
  If you right click the `ggxrd_hitbox_overlay` project in `Solution Explorer` and look into `Properties` -> `VC++ Directories` you will notice a path to `..\Detours\include` folder in the `Include Directories` field.  
  And if you look at the `Library Directories` field you'll notice the `..\Detours\lib.X86` directory already being there.  
  In the project settings property pages, in `Linker` -> `Input` the `detours.lib` is already added into the `Additional Dependencies` field.  
  If you run into the problem that Detours rebuilds itself and all its samples each time you try to build the ggxrd_hitbox_overlay project, try replacing the Makefile located in SOLUTION_ROOT/Detours/Makefile with an empty file. Do not commit this change to the Detours repository, either locally or remotely.

- `d3dx9.h` header file. It should be inside the <https://github.com/apitrace/dxsdk> repository that's included in this repository as a git submodule. You need not obtain it on your own.  
  If you right click this project in Visual Studio `Solution Explorer` and go to `Properties` -> `VC++ Directories`, you will notice that `..\dxsdk\Include` folder is already added into the `Include Directories` folder.  
  The correct place to get this header would be from Microsoft DirectX SDK (June 2010) (for DirectX 9) which can be downloaded freely from Microsoft. But it cannot be included as a git submodule and the license for its distribution by anyone other than Microsoft is ???, so I stuck with the dxsdk repo.  
  You don't need the `.lib` static libraries from the Direct3D SDK because you (the DLL) will get them from the Guilty Gear executable.

- `libpng` - a PNG encoder library. This is needed for the transparent/not-transparent screenshotting functionality. The full sources of libpng are included in this mod's sources. You need not obtain them on your own. We statically link libpng's 32-bit verion into this mod. libpng homepage: <http://www.libpng.org/pub/png/libpng.html>  

- `zlib` - a compression library used by libpng (<https://www.zlib.net/>). It is included in this mod as a git submodule, so you don't need to obtain it on your own.

In `ggxrd_hitbox_overlay` project, in `Properties`, in `VC++ Directories` the relative path to the `libpng` sourcetree is already included in the `Include Directories` field. The relative path to `libpng`'s `projects\vstudio\Release Library` directory is included into the `Library Directories` field. The `Linker` -> `Input` field already contains `libpng16.lib` and `zlib.lib` in the `Additional Dependencies` field.  
This will include `libpng` and `zlib` statically (meaning it's included into whatever you've built) into the `ggxrd_hitbox_overlay.dll` so you don't need to ship anything extra with the `ggxrd_hitbox_overlay.dll` to get PNG functionality working.

- `imgui` - a graphical user interface library. Its included in this mod as a git submodule.

- `D3DCompiler_47.dll` - Direct3D compiler for shaders and effects. It is used to compile a pixel shader which changes a hitbox's outline to black if it's being drawn over a pixel of the same color as that hitbox. This DLL should be present on the end user's Windows machine, in the C:\\Windows\\SysWOW64\\ (is machine/OS is 64-bit) or C:\\Windows\\System32\\ (if machine/OS is 32-bit) folder (the DLL we need is always 32-bit). Distributing it on your own may constitute a violation of copyright. This DLL is made a dependency of this project as a consequence of adding `d3dcompiler.lib` to ggxrd_hitbox_overlay's Project Settings - Linker - Input - Additional Dependencies, and is needed to call the D3DCompile function. If problems arise with this DLL, change the code to LoadLibraryA an older verion of it: 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43 - all named D3DCompiler_VERSIONNUMBER.dll, and then use GetProcAddress to find the D3DCompile function and call it. See usage in Graphics.cpp. Another way is to use fxc to precompile the shader (but I have no idea how many architectures are out there and how to target them with fxc, and how to apply the correct one at run-time), or make the feature optional (again, by using LoadLibraryA, if the DLL's not found just bail), or to get rid of this feature altogether - it is not critical.

Please refrain from modifying anything that's a git submodule, because there is no way to push those changes back into the repository, because for the submodules you would be pushing them not here but to the corresponding submodule repository (so a different git repository that isn't ours). Don't modify libpng either because the changes that were made to it are only limited to retargeting the projects since I didn't have the older build tools and was getting errors.

## Main README

The main README of the entire solution is located in the root of the git repository.
