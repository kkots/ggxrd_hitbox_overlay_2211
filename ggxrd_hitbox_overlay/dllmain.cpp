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
#include "imgui.h"
#include "UI.h"

static void closeLog();

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH: {
		DisableThreadLibraryCalls(hModule);
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
		#define terminate { \
    		detouring.cancelTransaction(); \
			closeLog(); \
			return FALSE; \
		}
		if (!IMGUI_CHECKVERSION()) {
			logwrap(fputs("IMGUI_CHECKVERSION() returned false\n", logfile));
			terminate
		}
        if (!detouring.beginTransaction()) terminate
        if (!settings.onDllMain()) terminate
        if (!game.onDllMain()) terminate
        if (!camera.onDllMain()) terminate
        if (!entityManager.onDllMain()) terminate
        if (!direct3DVTable.onDllMain()) terminate
        if (!keyboard.onDllMain()) terminate
        if (!ui.onDllMain()) terminate
        if (!endScene.onDllMain()) terminate
        if (!hitDetector.onDllMain()) terminate
        if (!graphics.onDllMain()) terminate
        if (!altModes.onDllMain()) terminate
        if (!throws.onDllMain()) terminate
        if (!hud.onDllMain()) terminate
        if (!detouring.endTransaction()) terminate
        break;
    }
    case DLL_PROCESS_DETACH:
        logwrap(fputs("DLL_PROCESS_DETACH\n", logfile));
        ui.onDllDetach();
        settings.onDllDetach();
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
    	detouring.cancelTransaction();
    	closeLog();
        break;
    }
    return TRUE;
}

void closeLog() {
#ifdef LOG_PATH
    if (logfile) {
        fflush(logfile);
        fclose(logfile);
        logfile = NULL;
    }
#endif
}
