// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "logging.h"
#include "Detouring.h"
#include "Graphics.h"
#include "Camera.h"
#include "Game.h"
#include "Direct3DVTable.h"
#include "EndScene.h"
#include "HitDetector.h"
#include "Entity.h"
#include "AltModes.h"
#include "Throws.h"
#include "Settings.h"
#include "Keyboard.h"
#include "Hud.h"
#include "memoryFunctions.h"
#include <io.h>     // for _open_osfhandle
#include <fcntl.h>  // for _O_APPEND
#include "..\imgui\imgui.h"

static void closeLog();

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH: {
#ifdef LOG_PATH
        {
            HANDLE fileHandle = CreateFileW(
				LOG_PATH,
				GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
				FILE_ATTRIBUTE_NORMAL, NULL);
			if (fileHandle == INVALID_HANDLE_VALUE) {
				return FALSE;
			}
			int fileDesc = _open_osfhandle((intptr_t)fileHandle, _O_APPEND | _O_TEXT);
			logfile = _fdopen(fileDesc, "at");
			fputs("DllMain called with fdwReason DLL_PROCESS_ATTACH\n", logfile);
			fflush(logfile);
		}
#endif
		if (!IMGUI_CHECKVERSION()) {
			logwrap(fputs("IMGUI_CHECKVERSION() returned false\n", logfile));
			closeLog();
			return FALSE;
		}
        if (!detouring.beginTransaction()) break;
        if (!settings.onDllMain()) break;
        if (!game.onDllMain()) break;
        if (!camera.onDllMain()) break;
        if (!entityManager.onDllMain()) break;
        if (!direct3DVTable.onDllMain()) break;
        if (!endScene.onDllMain()) break;
        if (!hitDetector.onDllMain()) break;
        if (!graphics.onDllMain()) break;
        if (!altModes.onDllMain()) break;
        if (!throws.onDllMain()) break;
        if (!keyboard.onDllMain()) break;
        if (!hud.onDllMain()) break;
        if (!detouring.endTransaction()) break;
        break;
    }
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        logwrap(fputs("DLL_PROCESS_DETACH\n", logfile));
        detouring.dllMainThreadId = GetCurrentThreadId();
        logwrap(fprintf(logfile, "DllMain called from thread ID %d\n", GetCurrentThreadId()));
        detouring.detachAll();
        Sleep(100);
        while (detouring.someThreadsAreExecutingThisModule(hModule)) Sleep(100);

        graphics.onUnload();  // here's how we cope with this being unsafe: between unhooking all functions
        // and this line of code we may miss an IDirect3DDevice9::Reset() call, in which we have to null the stencil
        // and the offscreenSurface. If we don't unhook Reset, we can't really wait for all hooks to finish executing,
        // because immediately after the wait, Reset may get called.
        // What solves all this is that Reset only gets called when the user decides to change resolutions or go
        // to fullscreen mode/exit fullscreen mode. So we just hope he doesn't do it while uninjecting the DLL.
        endScene.onDllDetach();
        hud.onDllDetach();
        settings.onDllDetach();
        break;
    }
    detouring.cancelTransaction();
    closeLog();
    return TRUE;
}

void closeLog() {
#ifdef LOG_PATH
    if (ul_reason_for_call == DLL_PROCESS_DETACH && logfile) {
        fflush(logfile);
        fclose(logfile);
        logfile = NULL;
    }
#endif
}
