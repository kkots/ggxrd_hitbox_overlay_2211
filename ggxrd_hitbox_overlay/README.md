# ggxrd_hitbox_overlay

## Credits

Original version created by Altimor: <http://www.dustloop.com/forums/index.php?/forums/topic/12495-xrd-pc-hitbox-overlay-mod/>
Created in 2016.
This version is adapted for Guilty Gear Xrd Rev2 version 2211.

Made with help from WorseThanYou.

## Build instructions for Windows

Open the `.sln` file, which is located in the parent folder from this readme, using Visual Studio (I'm using 2022 Community Edition). In the top bar select `Release` and `x86`.

Go to `Build` -> `Build Solution`.  
After building, the `.dll` file appears in `SOLUTION_ROOT\Release` folder (the `SOLUTION_ROOT` being where the `.sln` file is located).

## Build instructions for Linux with MinGW GCC

The thing when you build on one system for another is called "cross-compiling", and we're going to cross-compile the mod from Linux to Windows.
Install a MinGW GCC toolchain. For example, on Ubuntu I did `aptitude search mingw` and installed something random with mingw and gcc in its name, like `gcc-mingw-w64-i686`. That gave me tools like `i686-w64-mingw32-gcc` (the compiler, also able to call linker, generate a list of included header files by a .cpp file, and so on), `i686-w64-mingw32-windres` (compiles .rc file into an object file), `i686-w64-mingw32-ar` (creates static libraries, called archives on Linux), `i686-w64-mingw32-ranlib` (adds an index to archive files for faster lookup).
The portion before the `-gcc` I will call `CHOST` and I want you to specify that as an environment variable when launching `build.py`. `build.py` is a python script that you need `python3` to launch, and that script automates the build.
Launch it like this:

```bash
cd ggxrd_hitbox_overlay  # get inside the subproject directory
CHOST=i686-w64-mingw32 python3 build.py  # your CHOST may be different, it's whatever you installed
```

If there're errors during the build, you should be able to see them in red. Some of the common errors that you're very likely to have to fix:
1) Complains about min or max being undefined and suggests using std::min or std::max. min and max are defined on Microsoft C++ just fine. It's GCC's problem. But now it's your problem. You have to add a #define min at the top of the .cpp file:

```cpp
#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif
```

2) Complains about sizeof next to a type name. On Visual C++ if you use sizeof with a name of a type, it works fine, unless the expression is too confusing for it to understand, and then you must add parentheses around the type. On GCC you must always have parentheses around the type. My bad? Maybe?

3) Complains about floorf, ceilf. Mindblowingly, doesn't complain about roundf, ever. floorf and ceilf are functions that exist on Microsoft C++ in the std namespace, but do not on GCC. Simply rewrite floorf and ceilf to floor or ceil. Or remove the std:: namespace specifier, but then you may have to include some header, maybe cmath, math.h or something.

4) Header file not found, especially a built-in one. On Windows, filenames are case-insensitive, meaning you can use upper- and lowercase letters interchangeably without problems. On Linux, filenames are case-sensitive, and MinGW-w64 has opted to make all names lowercase. So whenever you need to include a system header file, it has to be lowercase in order to be found on MinGW on Linux (MinGW also exists on Windows, and it probably won't care about case on Windows).

I'm not going to test every release and git commit for whether it builds fine on Linux, so I may eventually write some code that doesn't compile and it is now your problem. Sorry. Also, MinGW MSVC sounds like the most compatible toolchain in theory, because on Windows the project is compiled using the MSVC compiler, but an MSVC toolchain wasn't present on aptitude search on Unbuntu so I never actually tried it. There's also a Window 10 SDK downloader and extractor <https://github.com/mstorsjo/msvc-wine> but it got stuck on a broken hashsum and I tried looking into the manifest files it fetches and finding which downloads are supposed to contain the compiler and MSBuild system, but no matter how much I tried or how obvious the names of the things were in the manifest, I couldn't find the files I needed. You might just need to install Visual Studio on Wine through an official .exe installer with UI, select checkboxes and download through the UI to obtain MSVC with everything it depends on, which then would have to be run on Wine (slow). But these are all alternative approaches that you hopefully won't need if you just use MinGW GCC with my script `build.py`. Last and final alternative approaches are Windows Virtual Machine with Visual Studio and then dual-boot to Windows.

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
  You don't need the `.lib` static libraries from the Direct3D SDK because apparently they're a part of the Windows SDK, they load the d3d9.dll automatically when the mod is loaded.

- `libpng` - a PNG encoder library. This is needed for the transparent/not-transparent screenshotting functionality. It is included in the mod as a git submodule, pointing to a fork. We statically link libpng's 32-bit verion into this mod. libpng homepage: <http://www.libpng.org/pub/png/libpng.html>  

- `zlib` - a compression library used by libpng (<https://www.zlib.net/>). It is included in this mod as a git submodule, so you don't need to obtain it on your own.

In `ggxrd_hitbox_overlay` project, in `Properties`, in `VC++ Directories` the relative path to the `libpng` sourcetree is already included in the `Include Directories` field. The relative path to `libpng`'s `projects\vstudio\Release Library` directory is included into the `Library Directories` field. The `Linker` -> `Input` field already contains `libpng16.lib` and `zlib.lib` in the `Additional Dependencies` field.  
This will include `libpng` and `zlib` statically (meaning it's included into whatever you've built) into the `ggxrd_hitbox_overlay.dll` so you don't need to ship anything extra with the `ggxrd_hitbox_overlay.dll` to get PNG functionality working.

- `imgui` - a graphical user interface library. It's included in this mod as a git submodule.

- `D3DCompiler_43.dll` (optionally D3DCompiler_47.dll) - used for compiling a custom pixel shader using D3DCompile function. The header files for it are d3dcompiler.h and D3DX9Shader.h, from `dxsdk` repository mentioned earlier. A .LIB file for it, d3dcompiler.lib, is taken from Microsoft's legacy DirectX Software Development Kit and included in this project in the `d3d9` folder. It is used by the project by specifying it in ggxrd_hitbox_overlay's Project Settings - VC++ Directories - Library Directories - ..\d3d9, and Project Settings - Linker - Input - d3dcompiler.lib. The .LIB file is needed because it is not included in the Windows SDK and is not present in the modern DirectX SDK NuGet package. The .LIB file allows the functions from the DLL to be used directly, without LoadLibraryA and GetProcAddress, and the DLL itself gets loaded by the mod on startup automatically, because the .LIB file is a type of .LIB file meant for dynamically-loading the library instead of linking it statically. The DLL is shipped with Guilty Gear Xrd and resides in its Binaries\\Win32 folder and there is no need to ship it yourself with the mod. Optionally, the mod will try to load the more modern library - D3DCompiler_47.dll - and use its D3DCompile function, but if it doesn't find it, it will use D3DCompiler_43.dll.

- `D3DX9_43.dll` - used for matrix math using D3DXMatrixMultiply and similar. It is used by the project the exact same way as D3DCompiler_43.dll, and its .LIB file is d3dx9.lib and header file is d3dx9math.h from `dxsdk` repository mentioned earlier.

- `JWasm` - this repository <https://github.com/JWasm/JWasm> gets cloned by the build.py script used for building the project on Linux. This x86 assembler is used to build the asmhooks.asm file, written in MASM syntax. MASM syntax differs from NASM, despite it having extensions and options to support it more fully, and it especially differs from GCC Aseembler syntax even with it has been passed -masm=intel commandline option.

Please refrain from modifying anything that's a git submodule, because there is no way to push those changes back into the repository, because for the submodules you would be pushing them not here but to the corresponding submodule repository (so a different git repository that isn't ours). Don't modify libpng either because the changes that were made to it are only limited to retargeting the projects since I didn't have the older build tools and was getting errors.

## Main README

The main README of the entire solution is located in the root of the git repository.
