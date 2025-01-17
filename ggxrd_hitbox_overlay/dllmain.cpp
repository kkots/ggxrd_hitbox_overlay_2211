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
#include "WinError.h"
#include "Moves.h"

static void closeLog();
static bool initialized = false;
static HMODULE obsDll = NULL;

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
				WinError winErr;
				char winErrStr[1024];
				sprintf_s(winErrStr, "Failed to open log: %ls", winErr.getMessage());
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
		if (!detouring.beginTransaction(true)) terminate
		obsDll = GetModuleHandleA("graphics-hook32.dll");
		if (obsDll) graphics.imInDanger = true;
		if (!settings.onDllMain()) terminate
		if (!game.onDllMain()) terminate
		if (!camera.onDllMain()) terminate
		if (!entityManager.onDllMain()) terminate
		if (!direct3DVTable.onDllMain()) terminate
		if (!keyboard.onDllMain()) terminate
		if (!ui.onDllMain(hModule)) terminate
		if (!endScene.onDllMain()) terminate
		if (!hitDetector.onDllMain()) terminate
		if (!game.sigscanAfterHitDetector()) terminate
		if (!graphics.onDllMain(hModule)) terminate
		if (!altModes.onDllMain()) terminate
		if (!throws.onDllMain()) terminate
		if (!hud.onDllMain()) terminate
		if (!moves.onDllMain()) terminate
		if (!detouring.endTransaction()) terminate
		initialized = true;
		break;
	}
	case DLL_PROCESS_DETACH:
		logwrap(fputs("DLL_PROCESS_DETACH\n", logfile));
		logwrap(fprintf(logfile, "DllMain called from thread ID %d\n", GetCurrentThreadId()));
		ui.onDllDetachStage1_killTimer();
		settings.onDllDetach();
		
		// send signals to various hooked threads that are running continuously,
		// telling them to undo the changes they have made.
		// imGui looks like it needs graphics resources undone first (on the graphics thread),
		// then the whole rest of imGui (on the logic thread)
		
		graphics.shutdown = true;
		graphics.onDllDetach();
		
		camera.shutdown = true;
		game.shutdown = true;
		endScene.onDllDetach();
		
		closeLog();
		break;
	case DLL_THREAD_ATTACH:
		if (!obsDll) {
			obsDll = GetModuleHandleA("graphics-hook32.dll");
			if (obsDll) {
				// We need to stall the thread that hooks Present
				// This allows us to warn our own graphics thread that a hostile hook is coming in the near future
				// So that our friendly code can present its last frame unhooked
				// And, after that, notify us, to let us know we can let this thread go
				
				// If we don't stall this thread
				// It will hook Present in the middle of a frame
				// We will have no way of knowing if on that frame Present was already hooked or not
				// Because OBS hooks Present in a thread-unsafe way, from a thread that is not the graphics thread,
				// And without freezing all the threads of the process
				// So it's just poof and suddenly it's hooked in the middle of any instruction
				// We simply don't know
				// And that means that whatever we drew on that frame may be visible to OBS
				// Unless we somehow warn the graphics thread
				// But of course without stalling the thread that hooks Present this is all a race condition
				graphics.imInDanger = true;
				if (initialized) {
					WaitForSingleObject(graphics.responseToImInDanger, INFINITE);
				}
			}
		}
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
