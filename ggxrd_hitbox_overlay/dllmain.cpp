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
#include "imgui.h"
#include "UI.h"
#include "WError.h"
#include "Moves.h"
#include "InputNames.h"

#ifdef LOG_PATH
#include <io.h>     // for _open_osfhandle
#include <fcntl.h>  // for _O_APPEND
static void closeLog();
#else
#define closeLog()
#endif
static bool initialized = false;
static HMODULE obsDll = NULL;
HMODULE hInst = NULL;


unsigned int* ue3EngineTick = nullptr;
unsigned int getUE3EngineTick() {
	if (!ue3EngineTick) {
		ue3EngineTick = (unsigned int*)sigscanOffset(GUILTY_GEAR_XRD_EXE,
			"75 05 83 f8 5 76 14 f2 0f 10 47 10", { -4, 0 }, nullptr, "EngineTickCount");
	}
	if (!ue3EngineTick) {
		return 0;
	} else {
		return *ue3EngineTick;
	}
}

DWORD threadIdThatCalledInitializeTheMod = 0;
bool UWorld_Tick_doingNestedCall = false;

bool modStillBeingExecutedOnTheThreadThatCalledInitializeTheMod() {
	if (UWorld_Tick_doingNestedCall) return true;
	if (!threadIdThatCalledInitializeTheMod) return false;
	if (GetCurrentThreadId() == threadIdThatCalledInitializeTheMod) return false;
	HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT, FALSE, threadIdThatCalledInitializeTheMod);
	if (!hThread) return false;
	
	struct CloseHandleOnExit {
		~CloseHandleOnExit() {
			if (handle) {
				if (unsuspend) {
					ResumeThread(handle);
				}
				CloseHandle(handle);
			}
		}
		HANDLE handle;
		bool unsuspend = false;
	} closeHandleOnExit{hThread};
	
	if (SuspendThread(hThread) == (DWORD)-1) return false;
	closeHandleOnExit.unsuspend = true;
	
	CONTEXT ctx;
	ctx.ContextFlags = CONTEXT_CONTROL;
	if (!GetThreadContext(hThread, &ctx)) return false;
	
	uintptr_t start, end, wholeModuleBegin;
	getModuleBounds(GUILTY_GEAR_XRD_EXE, "all", &start, &end, &wholeModuleBegin);
	
	return ctx.Eip >= start && ctx.Eip < end;
	
}

// runs in its own thread
DWORD WINAPI selfUnload(LPVOID lpThreadParameter) {
	while (modStillBeingExecutedOnTheThreadThatCalledInitializeTheMod()) {
		Sleep(16);
	}
	FreeLibraryAndExitThread(hInst, 0);
}

// runs on the main thread
BOOL initializeTheMod() {
	threadIdThatCalledInitializeTheMod = GetCurrentThreadId();
	if (!detouring.beginTransaction(false)) {
		detouring.undoPatches();  // in case if detouring.beganTransaction is false, detouring.cancelTransaction() won't actually undo the patches. Make sure they're undone
		goto fail;
	}
	if (!game.onDllMain()) goto fail;
	if (!camera.onDllMain()) goto fail;
	if (!entityManager.onDllMain()) goto fail;
	if (!direct3DVTable.onDllMain()) goto fail;
	if (!keyboard.onDllMain()) goto fail;
	if (!ui.onDllMain()) goto fail;
	if (!endScene.onDllMain()) goto fail;
	if (!hitDetector.onDllMain()) goto fail;
	if (!game.sigscanAfterHitDetector()) goto fail;
	if (!endScene.sigscanAfterHitDetector()) goto fail;
	if (!graphics.onDllMain()) goto fail;
	if (!altModes.onDllMain()) goto fail;
	if (!throws.onDllMain()) goto fail;
	if (!hud.onDllMain()) goto fail;
	if (!detouring.endTransaction()) goto fail;
	fillInInputNames();
	finishedSigscanning();
	return TRUE;
	
	// yes, throw shit at me, I'm using labels now, after reading 100000 articles about how it makes bad spaghetti code and not a single article that says anything good about it.
	// This is better than having code in a define, because define code requires a \ at the end of each line and frankly I don't see a difference.
	// You get spaghetti code if you use a subfunction or std::function, too, or even a struct DoThisUponExit with a ~DoThisUponExit.
	// The only way to solve spaghetti cleanup code on exit is to use resource wrappers, i.e. classes that wrap resource objects and free them on destruction.
	// But if you define these classes in the function itself, the spaghetti is still there and you haven't solved anything.
	// Another way could be to run a subfunction, check its exit status, and if it's an error, do cleanup and propagate the 'false' result.
	// Really all you're doing is writing labels in a different way.
	// The real problem with labels starts when there're too many of them and it's really confusing to keep track of.
	fail:
	detouring.cancelTransaction();  // undoes the UWorld_Tick patch
	CreateThread(NULL, 0, selfUnload, NULL, 0, NULL);
	return FALSE;
}

// DLL_PROCESS_ATTACH may run from the main thread, if patcher was used, or from an injected thread, if injector was used.
// DLL_PROCESS_DETACH runs from the main thread when the game is crashing (this one is definite, seen it with my own eyes) or shutting down normally (probably, not sure),
// and runs from an injected thread when the mod is being unloaded by the injector application.
// DLL_THREAD_ATTACH and DLL_THREAD_DETACH may run from any thread.
BOOL APIENTRY DllMain( HMODULE hModule,
					   DWORD  ul_reason_for_call,
					   LPVOID lpReserved
					 )
{
	hInst = hModule;
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
			closeLog(); \
			return FALSE; \
		}
		if (!IMGUI_CHECKVERSION()) {
			logwrap(fputs("IMGUI_CHECKVERSION() returned false\n", logfile));
			terminate
		}
		
		obsDll = GetModuleHandleA("graphics-hook32.dll");
		//if (obsDll) graphics.imInDanger = true;
		if (obsDll) graphics.obsModuleSpotted = true;
		
		// the list of moves is needed for the settings reader. moves.onDllMain() must not perform sigscan
		if (!moves.onDllMain()) terminate
		// settings are read to determine the value of settings.useSigscanCaching. settings does not perform sigscans
		if (!settings.onDllMain()) terminate
		// now moves is allowed to sigscan. sigscan depends on the value of settings.useSigscanCaching.
		// settings.onDllMain() also sets up settingsPathFolder where the sigscan cache is located
		if (!moves.performSigscans()) terminate
		
		// vvvvvvvvvvvv HOLY FUCKING SHIT THIS IS A LOT OF TEXT vvvvvvvvvvvv
		
		// we should not hook multiple functions here at once, as the DLL may be loaded from an injected thread
		// (I will explain why hooking multiple functions from a rogue thread is a bad idea shortly).
		// Even if we try to use the Detours library on even one function, we would have to suspend the main
		// thread (the one we hook) first, because Detours is very tidy and clean and relocates EIP between the trampoline and
		// the original code. This is good and we want that. Problem with suspending threads at all is that one of the
		// suspended threads may happen to be holding some heap memory lock, which, if we try to allocate some heap
		// memory of our own, may cause a deadlock, and this has already actually happened to me and I've confirmed
		// via debugging that this was the case.
		// The lock is internal to Win API and we can't easily check if it's already locked or not.
		// The way Detours is written, it does allocate memory in pretty much all of its functions that we interact with.
		// Our own code that interacts with detours also allocated dynamic memory.
		// Even if we get rid of all memory allocations during detour transactions, and make each DetourAttach be in a separate transaction,
		// that's just so many transactions, because we hook so, so many functions. Each transaction messes with
		// page protections for the detour regions (pages of allocated memory for trampolines).
		// And we'd have to suspend the main thread separately in each transaction as well, and each detour attach
		// operation would call GetThreadContext on that thread. This may be slow, therefore it's better to do just one
		// big transaction, and preferably without suspending any threads at all.
		// The way we achieve that is by patching an instruction that is known to be 5 bytes into another instruction
		// that is also 5 bytes. This does not require a suspension, because no EIP would have to be moved.
		// The new instruction is a call to our function, which will happen on the main thread, which is the one we hook,
		// which means if we proceed to hook all the rest of the functions from there, no suspension will be needed at all.
		// Which means we won't run into the heap memory deadlock.
		// Another interesting problem that may happen if you try to hook many functions from a separate thread using
		// a separate Detours transaction for each hook, is that the hooks may start running before they're all done,
		// which means that now in the hook code you must consider such possibility that other hooks may not be in place yet.
		// And when they're all in place, that may happen at any point during execution of the game, so what you really need
		// to wait for then is the end of the Unreal Engine tick. And that hints at the idea that maybe you should just install
		// all hooks in the Unreal Engine tick.
		
		uintptr_t UWorld_TickCallPlace = sigscanOffset(
			GUILTY_GEAR_XRD_EXE,
			"89 9e f4 04 00 00 8b 0d ?? ?? ?? ?? f3 0f 11 04 24 6a 02 e8 ?? ?? ?? ?? 33 ff 39 9e d8 04 00 00 7e 37",
			{ 19 },
			nullptr, "UWorld_TickCallPlace");
		finishedSigscanning();
		if (!UWorld_TickCallPlace) return false;
		
		game.orig_UWorld_Tick = (UWorld_Tick_t)followRelativeCall(UWorld_TickCallPlace);
		auto UWorld_TickHookPtr = &Game::HookHelp::UWorld_TickHook;
		int offset = calculateRelativeCallOffset(UWorld_TickCallPlace, (uintptr_t&)UWorld_TickHookPtr);
		std::vector<char> newBytes(4);
		*(int*)newBytes.data() = offset;
		detouring.patchPlace(UWorld_TickCallPlace + 1, newBytes);
		initialized = true;
		break;
	}
	// you may not detach the DLL on the main thread unless the process is shutting down.
	// When it is shutting down, all threads are dead but one.
	// If not, and you try to detach on the main thread, the code that calls this detach would be very likely foreign to the game
	// (you added such code to the game via hooking),
	// so would probably reside within the mod, and that means after the FreeLibrary call you would return
	// to the return pointer that still points to somewhere in the DLL, which no longer exists, which means crash.
	// And if you patch the game to unload the mod, that is better, but then you leave that instruction in,
	// and it unloads each time the host code runs, which is ugly.
	// The only function that lets you free a library and stop executing the calling function is
	// FreeLibraryAndExitThread, and you don't want to terminate the main thread.
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
		
		// there may still be a bit of a race condition, because after signalling an event
		// the hooks may still be executing
		
		if (endScene.logicThreadId && endScene.logicThreadId != GetCurrentThreadId()) {
			// thanks to Worse Than You for finding this
			// the tick is 8 bytes, but it takes 28 months for it to overflow into the high dword
			unsigned int startingTick = getUE3EngineTick();
			if (ue3EngineTick) {
				const bool graphicsThreadStillExists = graphics.graphicsThreadStillExists();
				while (getUE3EngineTick() - startingTick < 2 || graphicsThreadStillExists && FRenderCommand::totalCountOfCommandsInCirculation > 0) {
					Sleep(16);
				}
			}
		} else if (graphics.graphicsThreadStillExists()) {
			while (FRenderCommand::totalCountOfCommandsInCirculation > 0) {
				Sleep(16);
			}
		}
		
		closeLog();
		break;
	case DLL_THREAD_ATTACH:
		if (!obsDll) {
			obsDll = GetModuleHandleA("graphics-hook32.dll");
			if (obsDll) {
				// EDIT: THIS SOLUTION CAUSES FREEZES
				// when the game is launching into fullscreen.
				// Debugging shows that the main logic thread is stuck inside PeekMessage called from
				// GameOverlayRenderer.dll (steam's DLL), called from one of the game's functions.
				// Below is the original comment with the idea of what I was trying to do, for historical context.
				
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
				//graphics.imInDanger = true;
				graphics.obsModuleSpotted = true;
				//if (initialized) {
				//	WaitForSingleObject(graphics.responseToImInDanger, INFINITE);
				//}
			}
		}
		break;
	}
	return TRUE;
}

#ifdef LOG_PATH
void closeLog() {
	if (logfile) {
		fflush(logfile);
		fclose(logfile);
		logfile = NULL;
	}
}
#endif
