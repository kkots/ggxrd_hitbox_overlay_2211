#include "pch.h"
#include "EndScene.h"
#include "Direct3DVTable.h"
#include "Detouring.h"
#include "DrawOutlineCallParams.h"
#include "AltModes.h"
#include "HitDetector.h"
#include "logging.h"
#include "Game.h"
#include "EntityList.h"
#include "InvisChipp.h"
#include "Graphics.h"
#include "Camera.h"
#include <algorithm>
#include "collectHitboxes.h"
#include "Throws.h"
#include "colors.h"
#include "Settings.h"
#include "Keyboard.h"
#include "GifMode.h"
#include "memoryFunctions.h"
#include "WinError.h"
#include "UI.h"
#include "CustomWindowMessages.h"
#include "Hud.h"
#include "Moves.h"
#include <intrin.h>
#include "findMoveByName.h"
#include "Hardcode.h"
#include <mutex>
#include "InputsDrawing.h"

EndScene endScene;
PlayerInfo emptyPlayer {0};
ProjectileInfo emptyProjectile;
findMoveByName_t findMoveByName = nullptr;


// Runs on the main thread
extern "C"
void __cdecl drawQuadExecHook(FVector2D* screenSize, REDDrawQuadCommand* item, void* canvas);  // defined here

// Runs on the main thread
extern "C"
void __cdecl drawQuadExecHookAsm(REDDrawQuadCommand* item, void* canvas);  // defined in drawQuadExecHook.asm

// Runs on the main thread
extern "C"
void __cdecl call_orig_drawQuadExec(void* orig_drawQuadExec, FVector2D *screenSize, REDDrawQuadCommand *item, void* canvas);  // defined in drawQuadExecHook.asm

bool EndScene::onDllMain() {
	bool error = false;
	
	orig_WndProc = (WNDPROC)sigscanOffset(
		"GuiltyGearXrd.exe",
		"81 fb 18 02 00 00 75 0f 83 fd 01 77 28 5d b8 44 51 4d 42 5b c2 10 00",
		{ -0xA },
		&error, "WndProc");
	// why not use orig_WndProc = (WNDPROC)SetWindowLongPtrW(keyboard.thisProcessWindow, GWLP_WNDPROC, (LONG_PTR)hook_WndProc);?
	// because if you use the patcher, it will load the DLL on startup, the DLL will swap out the WndProc,
	// but then the game swaps out the WndProc with its own afterwards. So clearly, we need to load the DLL
	// a bit later, and that needs to be fixed in the patcher, but a quick solution is to hook the
	// WndProc directly.

	if (orig_WndProc) {
		if (!detouring.attach(&(PVOID&)orig_WndProc,
			hook_WndProc,
			"WndProc")) return false;
	}
	
	uintptr_t TrainingEtc_OneDamage = sigscanStrOffset(
		"GuiltyGearXrd.exe:.rdata",
		"TrainingEtc_OneDamage",
		&error, "TrainingEtc_OneDamage");
	if (!error) {
		std::vector<char> sig;//[9] { "\xc7\x40\x28\x00\x00\x00\x00\xe8" };
		std::vector<char> mask;
		byteSpecificationToSigMask("c7 40 28 ?? ?? ?? ?? e8", sig, mask);
		substituteWildcard(sig, mask, 0, (void*)TrainingEtc_OneDamage);
		
		uintptr_t drawTextWithIconsCall = sigscanBufOffset(
			"GuiltyGearXrd.exe",
			sig.data(),
			sig.size() - 1,
			{ 7 },
			&error, "drawTextWithIconsCall");
		if (drawTextWithIconsCall) {
			drawTextWithIcons = (drawTextWithIcons_t)followRelativeCall(drawTextWithIconsCall);
			logwrap(fprintf(logfile, "drawTextWithIcons: %p\n", drawTextWithIcons));
			orig_drawTrainingHud = game.trainingHudTick;
			logwrap(fprintf(logfile, "orig_drawTrainingHud final location: %p\n", (void*)orig_drawTrainingHud));
		}
		if (orig_drawTrainingHud) {
			void (HookHelp::*drawTrainingHudHookPtr)(void) = &HookHelp::drawTrainingHudHook;
			if (!detouring.attach(&(PVOID&)orig_drawTrainingHud,
				(PVOID&)drawTrainingHudHookPtr,
				"drawTrainingHud")) return false;
		}
		if (!drawTextWithIcons) {
			error = true;
		}
	}
	
	wchar_t CanvasFlushSetupCommandString[] = L"_CanvasFlushSetupCommand";
	CanvasFlushSetupCommandString[0] = L'\0';
	uintptr_t CanvasFlushSetupCommandStringAddr = sigscanBufOffset(
		"GuiltyGearXrd.exe:.rdata",
		(const char*)CanvasFlushSetupCommandString,
		sizeof CanvasFlushSetupCommandString,
		{ 2 },
		&error, "CanvasFlushSetupCommandString");
	uintptr_t CanvasFlushSetupCommand_DescribeCommand = 0;
	if (CanvasFlushSetupCommandStringAddr) {
		std::vector<char> sig;
		std::vector<char> mask;
		byteSpecificationToSigMask("b8 ?? ?? ?? ?? c3",
			sig, mask);
		substituteWildcard(sig, mask, 0, (void*)CanvasFlushSetupCommandStringAddr);
		CanvasFlushSetupCommand_DescribeCommand = sigscanOffset(
			"GuiltyGearXrd.exe",
			sig,
			mask,
			&error, "CanvasFlushSetupCommand_DescribeCommand");
	}
	uintptr_t CanvasFlushSetupCommandVtable = 0;
	if (CanvasFlushSetupCommand_DescribeCommand) {
		CanvasFlushSetupCommandVtable = sigscanBufOffset(
			"GuiltyGearXrd.exe:.rdata",
			(const char*)&CanvasFlushSetupCommand_DescribeCommand,
			4,
			{ -8 },
			&error, "CanvasFlushSetupCommandVtable");
	}
	uintptr_t CanvasFlushSetupCommandCreationPlace = 0;
	if (CanvasFlushSetupCommandVtable) {
		std::vector<char> sig;
		std::vector<char> mask;
		byteSpecificationToSigMask("c7 ?? ?? ?? ?? ??", sig, mask);
		strcpy(mask.data() + 2, "xxxx");
		memcpy(sig.data() + 2, &CanvasFlushSetupCommandVtable, 4);
		CanvasFlushSetupCommandCreationPlace = sigscanOffset(
			"GuiltyGearXrd.exe",
			sig,
			mask,
			&error, "CanvasFlushSetupCommandCreationPlace");
	}
	uintptr_t GIsThreadedRenderingJzInstr = 0;
	if (CanvasFlushSetupCommandCreationPlace) {
		GIsThreadedRenderingJzInstr = sigscanBackwards(CanvasFlushSetupCommandCreationPlace - 1,
			"0f 84 ?? 00 00 00 6a 14");
	}
	uintptr_t commandVtableAssignment = 0;
	uintptr_t FRingBuffer_AllocationContext_ConstructorCallPlace = 0;
	if (GIsThreadedRenderingJzInstr) {
		// assume it's 39 3d __ __ __ __ CMP [GIsRenderedThreading], reg32  ; reg32 stores 0
		GIsThreadedRendering = *(bool**)(GIsThreadedRenderingJzInstr - 4);
		// 8d 4c 24 __ LEA ECX,[ESP+__]
		// e8 __ __ __ __ CALL ________
		FRingBuffer_AllocationContext_ConstructorCallPlace = sigscanForward(GIsThreadedRenderingJzInstr,
			"8d 4c 24 ?? e8");
		commandVtableAssignment = sigscanForward(GIsThreadedRenderingJzInstr,
			"c7 00 ?? ?? ?? ?? c7 00 ?? ?? ?? ??");
	}
	if (FRingBuffer_AllocationContext_ConstructorCallPlace) {
		FRingBuffer_AllocationContext_Constructor = (FRingBuffer_AllocationContext_Constructor_t)followRelativeCall(FRingBuffer_AllocationContext_ConstructorCallPlace + 4);
		GRenderCommandBuffer = *(void**)(FRingBuffer_AllocationContext_ConstructorCallPlace - 4);
	}
	if (commandVtableAssignment) {
		FRingBuffer_AllocationContext_Commit = (FRingBuffer_AllocationContext_Commit_t)followRelativeCall(commandVtableAssignment + 19);
		FRenderCommandVtable = *(void**)(commandVtableAssignment + 2);
		FRenderCommandDestructor = *(FRenderCommandDestructor_t*)FRenderCommandVtable;
		FSkipRenderCommandVtable = *(void**)(commandVtableAssignment + 8);
	}
	if (!GIsThreadedRendering
			|| !GRenderCommandBuffer
			|| !FRingBuffer_AllocationContext_Constructor
			|| !FRingBuffer_AllocationContext_Commit) {
		logwrap(fputs("Failed to find things needed for enqueueing render commands\n", logfile));
		error = true;
	}
	
	// Another way to find this is search for L"PostEvent" string. It will be used to create an FName global var
	// (FName is basically 2 ints). You can then search for uses of that global var to find a really big function
	// that calls a function with 2 arguments, with PostEvent FName being used after the call (is unrelated to the call).
	// PostName FName is used in 6 places, 3 of which are the same function - that is the big function I'm referring to,
	// and the call I'm looking for happens just prior to the third usage.
	// Or you could take one of the last called functions inside drawTextWithIcons, find where it stores data,
	// then the function that reads that data, and its calling function's calling function will be the one I want.
	uintptr_t TryEnterCriticalSectionPtr = findImportedFunction("GuiltyGearXrd.exe", "KERNEL32.DLL", "TryEnterCriticalSection");
	uintptr_t EnterCriticalSectionPtr = findImportedFunction("GuiltyGearXrd.exe", "KERNEL32.DLL", "EnterCriticalSection");
	uintptr_t LeaveCriticalSectionPtr = findImportedFunction("GuiltyGearXrd.exe", "KERNEL32.DLL", "LeaveCriticalSection");
	{
		std::vector<char> sig;
		std::vector<char> unused;
		byteSpecificationToSigMask("e8 ?? ?? ?? ?? 8b ?? ?? ?? ?? ?? 8b ?? ?? ?? ?? ?? 8b ?? ?? ?? ?? ??",
			sig, unused);
		memcpy(sig.data() + 7, &TryEnterCriticalSectionPtr, 4);
		memcpy(sig.data() + 13, &EnterCriticalSectionPtr, 4);
		memcpy(sig.data() + 19, &LeaveCriticalSectionPtr, 4);
		orig_REDAnywhereDispDraw = (REDAnywhereDispDraw_t)sigscanOffset(
			"GuiltyGearXrd.exe",
			sig.data(),
			"x????x?xxxxx?xxxxx?xxxx",
			{ -0x4 },
			&error, "REDAnywhereDispDraw");
	}
	
	if (orig_REDAnywhereDispDraw) {
		uintptr_t anotherCallWith2ArgsCdecl = followRelativeCall((uintptr_t)orig_REDAnywhereDispDraw + 0x5f);  // a cdecl call with 2 arguments
		FCanvas_Flush = (FCanvas_Flush_t)followRelativeCall(anotherCallWith2ArgsCdecl + 0x16f);  // it's in switch's case 7
		if (!detouring.attach(&(PVOID&)orig_REDAnywhereDispDraw,
			REDAnywhereDispDrawHookStatic,
			"REDAnywhereDispDraw")) return false;
		orig_drawQuadExec = (void*)followRelativeCall(anotherCallWith2ArgsCdecl + 0x148);  // it's in switch's case 4
	}
	
	if (orig_drawQuadExec) {
		// This function has a weird calling convention:
		// It does not pop stack arguments, similar to cdecl
		// It expects a pointer argument in ECX
		// It expects two more arguments on the stack, in ESP+4 and ESP+8
		// 
		// It cannot be hooked solely by C, because none of the standard calling conventions satisfy these requirements, so we hook it with a function written in asm
		if (!detouring.attach(&(PVOID&)orig_drawQuadExec,
			drawQuadExecHookAsm,  // defined in drawQuadExecHook.asm
			"drawQuadExec")) return false;
	}
	
	uintptr_t superflashInstigatorUsage = sigscanOffset(
		"GuiltyGearXrd.exe",
		"8b 86 ?? ?? ?? ?? 3b c3 74 09 ff 48 24 89 9e ?? ?? ?? ?? 89 be ?? ?? ?? ?? ff 47 24 8b 8f bc 01 00 00 49 89 8e ?? ?? ?? ?? 8b 97 c0 01 00 00 4a",
		NULL, "superflashInstigatorOffset");
	if (superflashInstigatorUsage) {
		superflashInstigatorOffset = *(DWORD*)(superflashInstigatorUsage + 37);
		superflashCounterOpponentOffset = superflashInstigatorOffset + 4;
		superflashCounterAlliedOffset = superflashInstigatorOffset + 8;
		uintptr_t setSuperFreezeAndRCSlowdownFlagsUsage = sigscanForward(superflashInstigatorUsage, "8b ce e8", 0x7f + 3);
		if (setSuperFreezeAndRCSlowdownFlagsUsage) {
			orig_setSuperFreezeAndRCSlowdownFlags = (setSuperFreezeAndRCSlowdownFlags_t)followRelativeCall(setSuperFreezeAndRCSlowdownFlagsUsage + 2);
		}
	}
	
	if (orig_setSuperFreezeAndRCSlowdownFlags) {
		void (HookHelp::*setSuperFreezeAndRCSlowdownFlagsHookPtr)() = &HookHelp::setSuperFreezeAndRCSlowdownFlagsHook;
		if (!detouring.attach(&(PVOID&)orig_setSuperFreezeAndRCSlowdownFlags,
			(PVOID&)setSuperFreezeAndRCSlowdownFlagsHookPtr,
			"setSuperFreezeAndRCSlowdownFlags")) return false;
	}
	
	shutdownFinishedEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
	if (!shutdownFinishedEvent || shutdownFinishedEvent == INVALID_HANDLE_VALUE) {
		WinError winErr;
		logwrap(fprintf(logfile, "Failed to create event: %ls\n", winErr.getMessage()));
		error = true;
	}
	
	uintptr_t bbscrJumptable = sigscanOffset(
		"GuiltyGearXrd.exe",
		"83 ?? 04 85 ?? 74 07 83 ?? 28 74 02 8b ?? 81 ?? 88 09 00 00 0f 87 ?? ?? ?? ?? ff 24",
		{ 29, 0 },
		&error, "bbscrJumptable");
	
	#define digUpBBScrFunction(type, name, index) \
		uintptr_t name##CallPlace = 0; \
		uintptr_t name##Call = 0; \
		if (bbscrJumptable) { \
			name##CallPlace = *(uintptr_t*)(bbscrJumptable + index*4); \
		} \
		if (name##CallPlace) { \
			name##Call = sigscanForward(name##CallPlace, "e8"); \
		} \
		if (name##Call) { \
			name = (type)followRelativeCall(name##Call); \
		}
		
	#define digUpBBScrFunctionAndHook(type, name, index, signature_in_parentheses) \
		digUpBBScrFunction(type, orig_##name, index) \
		if (orig_##name) { \
			void (HookHelp::*name##HookPtr)signature_in_parentheses = &HookHelp::name##Hook; \
			if (!detouring.attach(&(PVOID&)orig_##name, \
				(PVOID&)name##HookPtr, \
				#name)) return false; \
		}
	
	digUpBBScrFunctionAndHook(BBScr_createObjectWithArg_t, BBScr_createObjectWithArg, 445, (const char*, unsigned int))
	digUpBBScrFunctionAndHook(BBScr_createObjectWithArg_t, BBScr_createObject, 446, (const char*, unsigned int))
	digUpBBScrFunctionAndHook(BBScr_createParticleWithArg_t, BBScr_createParticleWithArg, 449, (const char*, unsigned int))
	digUpBBScrFunctionAndHook(BBScr_runOnObject_t, BBScr_runOnObject, 41, (int))
	digUpBBScrFunctionAndHook(BBScr_sendSignal_t, BBScr_sendSignal, 1766, (int, int))
	digUpBBScrFunctionAndHook(BBScr_sendSignalToAction_t, BBScr_sendSignalToAction, 1771, (const char*, int))
	digUpBBScrFunction(BBScr_callSubroutine_t, BBScr_callSubroutine, 17)
	uintptr_t BBScr_createArgHikitsukiVal = 0;
	digUpBBScrFunction(uintptr_t, BBScr_createArgHikitsukiVal, 2247)
	uintptr_t BBScr_checkMoveCondition = 0;
	digUpBBScrFunction(uintptr_t, BBScr_checkMoveCondition, 49)
	uintptr_t BBScr_whiffCancelOptionBufferTime = 0;
	digUpBBScrFunction(uintptr_t, BBScr_whiffCancelOptionBufferTime, 1630)
	digUpBBScrFunctionAndHook(BBScr_setHitstop_t, BBScr_setHitstop, 2263, (int))
	digUpBBScrFunctionAndHook(BBScr_ignoreDeactivate_t, BBScr_ignoreDeactivate, 298, ())
	digUpBBScrFunctionAndHook(BBScr_timeSlow_t, BBScr_timeSlow, 2201, (int))
	
	if (orig_BBScr_sendSignal) {
		getReferredEntity = (getReferredEntity_t)followRelativeCall((uintptr_t)orig_BBScr_sendSignal + 5);
	}
	
	if (BBScr_createArgHikitsukiVal) {
		BBScr_getAccessedValueImpl = (BBScr_getAccessedValueImpl_t)followRelativeCall(BBScr_createArgHikitsukiVal + 15);
	}
	uintptr_t BBScr_getAccessedValueImplJumptable = 0;
	if (BBScr_getAccessedValueImpl) {
		std::vector<char> sig;
		std::vector<char> mask;
		byteSpecificationToSigMask("83 ec 1c", sig, mask);
		uintptr_t ptr = (uintptr_t)BBScr_getAccessedValueImpl;
		bool ok = memcmp((void*)ptr, sig.data(), 3) == 0;
		ptr += 3;
		if (ok) {
			ok = *(BYTE*)ptr == 0xa1;
			++ptr;
		}
		if (ok) {
			byteSpecificationToSigMask("8b 0d ?? ?? ?? ?? 3d ec 00 00 00 0f 87", sig, mask);
			substituteWildcard(sig.data(), mask.data(), 0, game.gameDataPtr);
			ptr = sigscanForward(ptr, sig.data(), mask.data(), 0x40);
			ok = ptr != 0;
			ptr += 13;
		}
		if (ok) {
			ptr = sigscanForward(ptr, "ff 24 85", 0x40);
			ok = ptr != 0;
		}
		if (ok) {
			BBScr_getAccessedValueImplJumptable = *(uintptr_t*)(ptr + 3);
		}
	}
	if (BBScr_getAccessedValueImplJumptable) {
		uintptr_t entryPtr = *(uintptr_t*)(BBScr_getAccessedValueImplJumptable + 4 * 0x14);  // DISTANCE_TO_WALL_IN_FRONT
		std::vector<char> sig;
		std::vector<char> mask;
		byteSpecificationToSigMask("8b 0d ?? ?? ?? ?? 8b 91 ?? ?? ?? ??", sig, mask);
		substituteWildcard(sig.data(), mask.data(), 0, aswEngine);
		uintptr_t aswEngUsage = sigscanForward(entryPtr, sig.data(), mask.data(), 0x1e);
		if (aswEngUsage) {
			leftEdgeOfArenaOffset = *(DWORD*)(aswEngUsage + 8);
			rightEdgeOfArenaOffset = leftEdgeOfArenaOffset + 4;
		}
	}
	if (BBScr_checkMoveCondition) {
		BBScr_checkMoveConditionImpl = (BBScr_checkMoveConditionImpl_t)followRelativeCall(BBScr_checkMoveCondition + 14);
	}
	if (BBScr_whiffCancelOptionBufferTime) {
		uintptr_t findMoveByNameCallPlace = sigscanForward(BBScr_whiffCancelOptionBufferTime, "e8", 20);
		if (findMoveByNameCallPlace) {
			findMoveByName = (findMoveByName_t)followRelativeCall(findMoveByNameCallPlace);
		}
	}
	
	std::vector<char> sig;
	std::vector<char> mask;
	byteSpecificationToSigMask("c7 ?? ?? ?? ?? ?? 66 0f ef c0 8d ?? ?? ?? ?? ?? ?? 66 0f d6 ?? ?? ?? ?? ?? ?? 89 ?? ?? ?? 66 0f d6 ?? ?? ?? ?? ?? e8",
		sig, mask);
	uintptr_t PawnVtable = sigscanOffset(
		"GuiltyGearXrd.exe",
		sig,
		mask,
		{ 2, 0 },
		&error, "PawnVtable");
	uintptr_t getAccessedValueAddr = 0;
	if (PawnVtable) {
		orig_setAnim = *(setAnim_t*)(PawnVtable + 0x44);
		orig_pawnInitialize = *(pawnInitialize_t*)(PawnVtable);
		orig_handleUpon = *(handleUpon_t*)(PawnVtable + 0x3c);
		getAccessedValueAddr = *(uintptr_t*)(PawnVtable + 0x4c);
	}
	uintptr_t getAccessedValueImplCallPlace = 0;
	if (getAccessedValueAddr) {
		getAccessedValueImplCallPlace = sigscanForward(getAccessedValueAddr, "e8");
	}
	uintptr_t getAccessedValueImpl = 0;
	if (getAccessedValueImplCallPlace) {
		getAccessedValueImpl = followRelativeCall(getAccessedValueImplCallPlace);
	}
	uintptr_t getAccessedValueImplJumptableUse = 0;
	if (getAccessedValueImpl) {
		getAccessedValueImplJumptableUse = sigscanForward(getAccessedValueImpl, "ff 24 85");
	}
	if (getAccessedValueImplJumptableUse) {
		uintptr_t getAccessedValueJumptable = *(uintptr_t*)(getAccessedValueImplJumptableUse + 3);
		uintptr_t interroundValueStorage2CodeStart = *(uintptr_t*)(getAccessedValueJumptable + 124 * 4);
		std::vector<char> sig;
		std::vector<char> mask;
		byteSpecificationToSigMask("8b ?? ?? ?? ?? ?? 8b", sig, mask);
		*(char***)(sig.data() + 2) = aswEngine;
		memcpy(mask.data() + 2, "xxxx", 4);
		uintptr_t interroundValueStorage2Use = sigscanForward(interroundValueStorage2CodeStart, sig, mask);
		if (interroundValueStorage2Use) {
			interRoundValueStorage2Offset = *(DWORD*)(interroundValueStorage2Use + 9);
			interRoundValueStorage1Offset = interRoundValueStorage2Offset - 0x8;
		}
	}
	
	if (orig_setAnim) {
		void (HookHelp::*setAnimHookPtr)(const char*) = &HookHelp::setAnimHook;
		if (!detouring.attach(&(PVOID&)orig_setAnim,
			(PVOID&)setAnimHookPtr,
			"setAnim")) return false;
	}
	
	if (orig_pawnInitialize) {
		void (HookHelp::*pawnInitializeHookPtr)(void*) = &HookHelp::pawnInitializeHook;
		if (!detouring.attach(&(PVOID&)orig_pawnInitialize,
			(PVOID&)pawnInitializeHookPtr,
			"pawnInitialize")) return false;
	}
	
	if (orig_handleUpon) {
		void (HookHelp::*handleUponHookPtr)(int) = &HookHelp::handleUponHook;
		if (!detouring.attach(&(PVOID&)orig_handleUpon,
			(PVOID&)handleUponHookPtr,
			"handleUpon")) return false;
	}
	
	uintptr_t logicOnFrameAfterHitPiece = sigscanOffset(
		"GuiltyGearXrd.exe",
		"77 10 89 ?? b4 01 00 00 c7 ?? b8 01 00 00 02 00 00 00",
		&error, "logicOnFrameAfterHitPiece");
	if (logicOnFrameAfterHitPiece) {
		orig_logicOnFrameAfterHit = (logicOnFrameAfterHit_t)scrollUpToBytes((char*)logicOnFrameAfterHitPiece,
			"\x83\xec\x14", 3);
	}
	if (orig_logicOnFrameAfterHit) {
		void (HookHelp::*logicOnFrameAfterHitHookPtr)(bool, int) = &HookHelp::logicOnFrameAfterHitHook;
		if (!detouring.attach(&(PVOID&)orig_logicOnFrameAfterHit,
			(PVOID&)logicOnFrameAfterHitHookPtr,
			"logicOnFrameAfterHit")) return false;
	}
	
	wchar_t iconStringW[] = L"icon";
	char iconStringA[1 + sizeof iconStringW];
	iconStringA[0] = '\0';
	memcpy(iconStringA + 1, iconStringW, sizeof iconStringW);
	uintptr_t iconStringAddr = sigscanBufOffset(
		"GuiltyGearXrd.exe:.rdata",
		iconStringA,
		sizeof iconStringA,
		{ 1 },
		&error, "iconStringAddr");
	uintptr_t iconStringUsage = 0;
	if (iconStringAddr) {
		std::vector<char> sig;
		std::vector<char> mask;
		byteSpecificationToSigMask("68 ?? ?? ?? ?? e8", sig, mask);
		substituteWildcard(sig.data(), mask.data(), 0, (void*)iconStringAddr);
		iconStringUsage = sigscanOffset(
			"GuiltyGearXrd.exe",
			sig,
			mask,
			&error, "iconStringUsage");
	}
	if (iconStringUsage) {
		iconStringUsage = sigscanForward(iconStringUsage, "a3");
	}
	if (iconStringUsage) {
		iconTexture = *(void**)(iconStringUsage + 1);
	}
	
	uintptr_t backPushbackApplierPiece = sigscanOffset(
		"GuiltyGearXrd.exe",
		"85 90 18 01 00 00 74 08 01 b0 30 03 00 00 eb 06 89 a8 30 03 00 00",
		nullptr, "backPushbackApplierPiece");
	if (backPushbackApplierPiece) {
		orig_backPushbackApplier = (backPushbackApplier_t)sigscanBackwards(backPushbackApplierPiece,
			"83 ec ?? 53 55 56 57");
	}
	if (orig_backPushbackApplier) {
		void (HookHelp::*backPushbackApplierHookPtr)() = &HookHelp::backPushbackApplierHook;
		if (!detouring.attach(&(PVOID&)orig_backPushbackApplier,
			(PVOID&)backPushbackApplierHookPtr,
			"backPushbackApplier")) return false;
	}
	
	uintptr_t pushbackStunOnBlockPiece = sigscanOffset(
		"GuiltyGearXrd.exe",
		"f7 d8 89 83 1c 03 00 00 8b 86 08 07 00 00 8b 88 08 07 00 00",
		nullptr, "pushbackStunOnBlockPiece");
	if (pushbackStunOnBlockPiece) {
		isDummyPtr = (isDummy_t)followRelativeCall(pushbackStunOnBlockPiece - 0xd7);
		orig_pushbackStunOnBlock = (pushbackStunOnBlock_t)sigscanBackwards(pushbackStunOnBlockPiece,
			"83 ec ?? 53 55 56 57");
	}
	if (orig_pushbackStunOnBlock) {
		void (HookHelp::*pushbackStunOnBlockHookPtr)(bool isAirHit) = &HookHelp::pushbackStunOnBlockHook;
		if (!detouring.attach(&(PVOID&)orig_pushbackStunOnBlock,
			(PVOID&)pushbackStunOnBlockHookPtr,
			"pushbackStunOnBlock")) return false;
	}
	
	uintptr_t OnPreSkillCheckStrAddr = sigscanStrOffset(
		"GuiltyGearXrd.exe:.rdata",
		"OnPreSkillCheck",
		&error, "OnPreSkillCheckStrAddr");
	uintptr_t OnPreSkillCheckVar = 0;
	if (OnPreSkillCheckStrAddr) {
		std::vector<char> sig;
		std::vector<char> mask;
		byteSpecificationToSigMask("68 ?? ?? ?? ??", sig, mask);
		substituteWildcard(sig, mask, 0, (void*)OnPreSkillCheckStrAddr);
		OnPreSkillCheckVar = sigscanOffset(
			"GuiltyGearXrd.exe",
			sig,
			mask,
			{ 6, 0 },
			&error, "OnPreSkillCheckVar");
	}
	if (OnPreSkillCheckVar) {
		std::vector<char> sig;
		std::vector<char> mask;
		byteSpecificationToSigMask("68 ?? ?? ?? ??", sig, mask);
		substituteWildcard(sig, mask, 0, (void*)OnPreSkillCheckVar);
		uintptr_t jmpInstrAddr = sigscanOffset(
			"GuiltyGearXrd.exe",
			sig,
			mask,
			{ 0x1e },
			&error, "skillCheckPiece");
		if (jmpInstrAddr) {
			orig_skillCheckPiece = (skillCheckPiece_t)followRelativeCall(jmpInstrAddr);
		}
	}
	if (orig_skillCheckPiece) {
		BOOL (HookHelp::*skillCheckPieceHookPtr)() = &HookHelp::skillCheckPieceHook;
		if (!detouring.attach(&(PVOID&)orig_skillCheckPiece,
			(PVOID&)skillCheckPieceHookPtr,
			"skillCheckPiece")) return false;
	}
	
	// To find this function you can just data breakpoint hitstop being set
	orig_beginHitstop = (beginHitstop_t)sigscanOffset(
		"GuiltyGearXrd.exe",
		"83 b9 b8 01 00 00 00 74 16 8b 81 b4 01 00 00 c7 81 b8 01 00 00 00 00 00 00 89 81 ac 01 00 00 33 c0 c3",
		&error,
		"beginHitstop");
	if (orig_beginHitstop) {
		void (HookHelp::*beginHitstopHookPtr)() = &HookHelp::beginHitstopHook;
		if (!detouring.attach(&(PVOID&)orig_beginHitstop,
			(PVOID&)beginHitstopHookPtr,
			"beginHitstop")) return false;
	}
	
	for (int i = 0; i < 2; ++i) {
		inputRingBuffersStored[i].resize(100);
	}
	
	#define initializeSkippedFrames(skippedF) \
		skippedF.resize(_countof(Framebar::frames)); \
		memset(skippedF.data(), 0, _countof(Framebar::frames) * sizeof SkippedFramesInfo);
		
	initializeSkippedFrames(skippedFrames)
	initializeSkippedFrames(skippedFramesIdle)
	initializeSkippedFrames(skippedFramesHitstop)
	initializeSkippedFrames(skippedFramesIdleHitstop)
	#undef initializeSkippedFrames
	
	uintptr_t onCmnActXGuardLoopPiece = sigscanOffset(
		"GuiltyGearXrd.exe",
		"8b 86 54 4d 00 00 8b ce 3b c5 7e 16 83 c0 04 89 86 54 4d 00 00",
		&error, "onCmnActXGuardLoop");
	if (onCmnActXGuardLoopPiece) {
		orig_onCmnActXGuardLoop = (onCmnActXGuardLoop_t)sigscanBackwards(onCmnActXGuardLoopPiece,
			"83 ec ?? a1 ?? ?? ?? ?? 33 c4", 0x616);
	}
	if (orig_onCmnActXGuardLoop) {
		void (HookHelp::*onCmnActXGuardLoopHookPtr)(int signal, int type, int thisIs0) = &HookHelp::onCmnActXGuardLoopHook;
		if (!detouring.attach(&(PVOID&)orig_onCmnActXGuardLoop,
			(PVOID&)onCmnActXGuardLoopHookPtr,
			"onCmnActXGuardLoop")) return false;
	}
	
	return !error;
}

void EndScene::onDllDetachPiece() {
	hud.onDllDetach();
	ui.onDllDetachNonGraphics();
	detouring.detachAll(true);
	detouring.cancelTransaction();
}

bool EndScene::onDllDetach() {
	logwrap(fputs("EndScene::onDllDetach() called\n", logfile));
	shutdown = true;
	if (!logicThreadId) return true;
	HANDLE logicThreadHandle = OpenThread(THREAD_QUERY_INFORMATION, FALSE, logicThreadId);
	if (!logicThreadHandle) {
		WinError winErr;
		logwrap(fprintf(logfile, "EndScene failed to open logic thread handle: %ls\n", winErr.getMessage()));
		onDllDetachPiece();
		return false;
	}
	if (GetProcessIdOfThread(logicThreadHandle) != GetCurrentProcessId()) {
		CloseHandle(logicThreadHandle);
		logwrap(fprintf(logfile, "EndScene freeing resources on DLL thread, because thread is no longer alive"));
		onDllDetachPiece();
		return true;
	}
	DWORD exitCode;
	bool stillActive = GetExitCodeThread(logicThreadHandle, &exitCode) && exitCode == STILL_ACTIVE;
	CloseHandle(logicThreadHandle);
	
	if (!stillActive) {
		logwrap(fprintf(logfile, "EndScene freeing resources on DLL thread, because thread is no longer alive (2)"));
		onDllDetachPiece();
		return true;
	}
	
	logwrap(fputs("EndScene calling WaitForSingleObject\n", logfile));
	if (WaitForSingleObject(shutdownFinishedEvent, 300) == WAIT_OBJECT_0) {
		logwrap(fprintf(logfile, "EndScene freed resources successfully\n"));
		return true;
	}
	logwrap(fprintf(logfile, "EndScene freeing resources on DLL thread, because WaitForSingleObject did not return success"));
	onDllDetachPiece();
	return true;
}

// Runs on the main thread
void EndScene::HookHelp::drawTrainingHudHook() {
	endScene.drawTrainingHudHook((char*)this);
}

// Runs on the main thread. Called once every frame when a match is running, including freeze mode or when Pause menu is open
void EndScene::logic() {
	actUponKeyStrokesThatAlreadyHappened();

	bool needToClearHitDetection = false;
	if (gifMode.modDisabled) {
		needToClearHitDetection = true;
	}
	else {
		bool isPauseMenu;
		bool isNormalMode = altModes.isGameInNormalMode(&needToClearHitDetection, &isPauseMenu) || isPauseMenu;
		pauseMenuOpen = isPauseMenu;
		bool isRunning = game.isMatchRunning() || altModes.roundendCameraFlybyType() != 8;
		if (!isRunning && !iGiveUp) {
			if (!settings.dontClearFramebarOnStageReset) {
				playerFramebars.clear();
				projectileFramebars.clear();
				combinedFramebars.clear();
			}
			startedNewRound = true;
		}
		if (!isRunning && !iGiveUp) {
			for (int i = 0; i < 2; ++i) {
				PlayerInfo& player = players[i];
				player.inputs.clear();
				player.prevInput = Input{0x0000};
				player.inputsOverflow = false;
			}
		}
		needDrawInputs = false;
		entityList.populate();
		if (gifMode.showInputHistory && !gifMode.gifModeToggleHudOnly && !gifMode.gifModeOn) {
			if (settings.displayInputHistoryWhenObserving
					&& game.currentModeIsOnline()
					&& game.getPlayerSide() == 2) {
				needDrawInputs = true;
			} else if (settings.displayInputHistoryInSomeOfflineModes) {
				GameMode gameMode = game.getGameMode();
				if (gameMode == GAME_MODE_ARCADE
						|| gameMode == GAME_MODE_KENTEI
						|| gameMode == GAME_MODE_TUTORIAL
						|| (
							gameMode == GAME_MODE_MOM
							|| gameMode == GAME_MODE_VERSUS
						) && (
							entityList.count >= 1
							&& entityList.slots[0].isCpu()
							|| entityList.count >= 2
							&& entityList.slots[1].isCpu()
						)) {
					needDrawInputs = true;
				}
			}
		}
		DWORD aswEngineTickCount = getAswEngineTick();
		bool areAnimationsNormal = entityList.areAnimationsNormal();
		if (isNormalMode) {
			if (!isRunning || !areAnimationsNormal) {
				needToClearHitDetection = true;
			}
			else {
				prepareDrawData(&needToClearHitDetection);
			}
		}
		if (prevAswEngineTickCountForInputs != aswEngineTickCount) {
			prepareInputs();
			prevAswEngineTickCountForInputs = aswEngineTickCount;
		}
	}
	if (needToClearHitDetection) {
		hitDetector.clearAllBoxes();
		throws.clearAllBoxes();
	}
	// Camera values are updated separately, in a updateCameraHook call, which happens before this is called
}

// Runs on the main thread
void EndScene::prepareDrawData(bool* needClearHitDetection) {
	logOnce(fputs("prepareDrawData called\n", logfile));
	invisChipp.onEndSceneStart();
	drawnEntities.clear();
	
	bool isTheFirstFrameInTheMatch = false;
	if (playerFramebars.empty() && !iGiveUp) {
		isTheFirstFrameInTheMatch = true;
		playerFramebars.emplace_back();
		{
			PlayerFramebars& framebar = playerFramebars.back();
			framebar.playerIndex = 0;
			framebar.setTitle("Player 1");
		}
		
		playerFramebars.emplace_back();
		{
			PlayerFramebars& framebar = playerFramebars.back();
			framebar.playerIndex = 1;
			framebar.setTitle("Player 2");
		}
		
		framebarPosition = _countof(Framebar::frames) - 1;
		framebarPositionHitstop = _countof(Framebar::frames) - 1;
		framebarIdleFor = 0;
		framebarIdleHitstopFor = 0;
	}
	if (startedNewRound) {
		startedNewRound = false;
		if (!iGiveUp) {
			isTheFirstFrameInTheMatch = true;
		}
	}
	if (isTheFirstFrameInTheMatch) {
		for (int i = 0; i < 2; ++i) {
			// on the first frame of a round people can't act. At all
			// without this fix, the mod thinks normals are enabled except on the very first of a match where it thinks they're not
			PlayerInfo& player = players[i];
			player.wasEnableNormals = false;
		}
	}
	bool framebarAdvanced = false;
	bool framebarAdvancedHitstop = false;
	bool framebarAdvancedIdle = false;
	bool framebarAdvancedIdleHitstop = false;
	
	noGravGifMode();

	logOnce(fprintf(logfile, "entity count: %d\n", entityList.count));

	DWORD aswEngineTickCount = getAswEngineTick();
	bool frameHasChangedForScreenshot = previousTimeOfTakingScreen != aswEngineTickCount;
	if (needContinuouslyTakeScreens) {
		if (frameHasChangedForScreenshot) {
			drawDataPrepared.needTakeScreenshot = true;
		}
		previousTimeOfTakingScreen = aswEngineTickCount;
	}
	else if (frameHasChangedForScreenshot) {
		previousTimeOfTakingScreen = ~0;
	}
	bool frameHasChanged = prevAswEngineTickCount != aswEngineTickCount && !game.isRoundend();
	prevAswEngineTickCount = aswEngineTickCount;
	
	bool needCatchEntities = false;
	if (frameHasChanged) {
		for (int i = 0; i < 2; ++i) {
			PlayerInfo& player = players[i];
			if (!player.pawn) {
				player.index = i;
				initializePawn(player, entityList.slots[i]);
				needCatchEntities = true;
				projectiles.clear();
			}
		}
	}
	if (frameHasChanged && !iGiveUp) {
		if (needCatchEntities && projectiles.empty()) {
			// probably the mod was loaded in mid-game
			for (int i = 2; i < entityList.count; ++i) {
				onObjectCreated(entityList.slots[i], entityList.list[i], entityList.list[i].animationName());
			}
		}
		
		int distanceBetweenPlayers = entityList.slots[0].posX() - entityList.slots[1].posX();
		if (distanceBetweenPlayers < 0) distanceBetweenPlayers = -distanceBetweenPlayers;
		
		bool comboStarted = false;
		for (int i = 0; i < 2; ++i) {
			Entity ent = entityList.slots[i];
			PlayerInfo& player = players[i];
			if (ent.inHitstun() && !player.inHitstunNowOrNextFrame) {
				comboStarted = true;
				break;
			}
		}
		
		Entity superflashInstigator = getSuperflashInstigator();
		
		for (int i = 0; i < 2; ++i) {
			PlayerInfo& player = players[i];
			PlayerInfo& other = players[1 - i];
			Entity ent = player.pawn;
			Entity otherEnt = other.pawn;
			player.prevHp = player.hp;
			player.hp = ent.hp();
			player.gutsRating = ent.gutsRating();
			player.gutsPercentage = ent.calculateGuts();
			player.risc = ent.risc();
			int tension = ent.tension();
			int tensionGain = tension - player.tension;
			player.tension = tension;
			int burst = game.getBurst(i);
			int burstGain = burst - player.burst;
			player.burst = burst;
			player.tensionPulse = ent.tensionPulse();
			player.negativePenaltyTimer = ent.negativePenaltyTimer();
			player.negativePenalty = ent.negativePenalty();
			player.tensionPulsePenalty = ent.tensionPulsePenalty();
			player.cornerPenalty = ent.cornerPenalty();
			entityManager.calculateTensionPulsePenaltyGainModifier(
				distanceBetweenPlayers,
				player.tensionPulse,
				&player.tensionPulsePenaltyGainModifier_distanceModifier,
				&player.tensionPulsePenaltyGainModifier_tensionPulseModifier);
			entityManager.calculateTensionGainModifier(
				distanceBetweenPlayers,
				player.negativePenaltyTimer,
				player.tensionPulse,
				&player.tensionGainModifier_distanceModifier,
				&player.tensionGainModifier_negativePenaltyModifier,
				&player.tensionGainModifier_tensionPulseModifier);
			player.extraTensionGainModifier = entityManager.calculateExtraTensionGainModifier(ent);
			
			player.x = ent.posX();
			player.y = ent.posY();
			player.speedX = ent.speedX();
			player.speedY = ent.speedY();
			player.gravity = ent.gravity();
			player.comboTimer = ent.comboTimer();
			player.pushback = ent.pushback();
			
			player.prevFrameHadDangerousNonDisabledProjectiles = player.hasDangerousNonDisabledProjectiles;
			player.prevFramePreviousEntityLinkObjectDestroyOnStateChangeWasEqualToPlayer = player.pawn.previousEntity()
				&& player.pawn.previousEntity().linkObjectDestroyOnStateChange() == player.pawn;
			player.hasDangerousNonDisabledProjectiles = false;
			player.createdDangerousProjectile = false;
			player.createdProjectileThatSometimesCanBeDangerous = false;
			
			if (comboStarted) {
				if (tensionGainOnLastHitUpdated[i]) {
					player.tensionGainLastCombo = tensionGainOnLastHit[i];
					tensionGain = player.tension - tensionRecordedHit[i];
				} else {
					player.tensionGainLastCombo = 0;
				}
				if (burstGainOnLastHitUpdated[i]) {
					player.burstGainLastCombo = burstGainOnLastHit[i];
					burstGain = player.burst - burstRecordedHit[i];
				} else {
					player.burstGainLastCombo = 0;
				}
			}
			player.inHitstunNowOrNextFrame = ent.inHitstun();
			player.inHitstun = ent.inHitstunThisFrame();
			if (player.inHitstunNowOrNextFrame || otherEnt.inHitstun()) {
				player.tensionGainLastCombo += tensionGain;
				player.burstGainLastCombo += burstGain;
			}
			if (player.tensionGainLastCombo > player.tensionGainMaxCombo) {
				player.tensionGainMaxCombo = player.tensionGainLastCombo;
			}
			if (player.burstGainLastCombo > player.burstGainMaxCombo) {
				player.burstGainMaxCombo = player.burstGainLastCombo;
			}
			player.tensionPulsePenaltySeverity = entityManager.calculateTensionPulsePenaltySeverity(player.tensionPulsePenalty);
			player.cornerPenaltySeverity = entityManager.calculateCornerPenaltySeverity(player.cornerPenalty);
			
			if (tensionGainOnLastHitUpdated[i]) {
				player.tensionGainOnLastHit = tensionGainOnLastHit[i];
			}
			if (burstGainOnLastHitUpdated[i]) {
				player.burstGainOnLastHit = burstGainOnLastHit[i];
			}
			tensionGainOnLastHitUpdated[i] = false;
			burstGainOnLastHitUpdated[i] = false;
	
			player.stun = ent.stun();
			player.stunThreshold = ent.stunThreshold();
			int prevFrameBlockstun = player.blockstun;
			player.blockstun = ent.blockstun();
			int prevFrameHitstun = player.hitstun;
			player.hitstun = ent.hitstun();
			player.tumble = 0;
			player.displayTumble = false;
			CmnActIndex cmnActIndex = ent.cmnActIndex();
			if (cmnActIndex == CmnActBDownLoop
					|| cmnActIndex == CmnActFDownLoop
					|| cmnActIndex == CmnActVDownLoop
					|| cmnActIndex == CmnActBDown2Stand
					|| cmnActIndex == CmnActFDown2Stand
					|| cmnActIndex == CmnActWallHaritsukiLand
					|| cmnActIndex == CmnActWallHaritsukiGetUp
					|| cmnActIndex == CmnActKizetsu) {
				player.hitstun = 0;
			}
			if (cmnActIndex == CmnActKorogari) {
				if (ent.currentAnimDuration() == 1 && !ent.isRCFrozen()) {
					player.tumbleContaminatedByRCSlowdown = false;
					player.tumbleMax = ent.tumble() + 30 - 1;
					player.tumbleStartedInHitstop = ent.hitstop() > 0;
					player.tumbleElapsed = 0;
				}
				player.displayTumble = true;
				if (ent.bbscrvar() == 0) {
					int animDur = ent.currentAnimDuration();
					player.tumble = ent.tumble() - (animDur == 1 ? 1 : player.tumbleStartedInHitstop ? animDur - 1 : animDur) + 30
						+ (!player.tumbleStartedInHitstop ? 1 : 0);
				} else {
					player.tumble = 30 - ent.bbscrvar2() + 1;
				}
			}
			if (player.hitstun == prevFrameHitstun + 9 && (
					cmnActIndex == CmnActBDownBound
					|| cmnActIndex == CmnActFDownBound
					|| cmnActIndex == CmnActVDownBound)) {
				player.hitstunMaxFloorbounceExtra += 10;  // some code in CmnActFDownBound, CmnActBDownBound and CmnActVDownBound increments hitstun by 10
				                                          // try this with Sol Kudakero airhit floorbounce. Potemkin ICPM ground hit may also do this
				                                          // Greed Sever airhit does this
			}
			int hitstop = ent.hitstop();
			int clashHitstop = ent.clashHitstop();
			if (!player.clashHitstop && clashHitstop) {
				player.hitstopMax = 0;
				player.hitstopElapsed = 0;
			}
			if (player.clashHitstop) {
				int val = clashHitstop + hitstop;
				if (val > player.hitstopMax) {
					player.hitstopMax = val;
					player.hitstopElapsed = 0;
				}
			}
			player.clashHitstop = clashHitstop;
			if (ent.hitSomethingOnThisFrame()) {
				player.hitstop = 0;
				player.hitstopMax = hitstop + clashHitstop - 1;
				if (player.hitstopMax < 0) player.hitstopMax = 0;
				player.hitstopElapsed = 0;
			} else if (player.armoredHitOnThisFrame && !player.gotHitOnThisFrame && player.setHitstopMaxSuperArmor) {
				player.hitstopMax = player.hitstopMaxSuperArmor - 1;
				player.hitstopElapsed = 0;
			} else {
				player.hitstop = hitstop + clashHitstop;
				if (!player.hitstop) {
					player.hitstop = player.lastHitstopBeforeWipe;
				}
			}
			if (player.blockstun && !player.hitstop && !superflashInstigator) {
				++player.blockstunElapsed;
				if (player.rcSlowedDown) player.blockstunContaminatedByRCSlowdown = true;
			}
			
			if (ent.cmnActIndex() == CmnActJitabataLoop) {
				int bbscrvar = player.pawn.bbscrvar();
				if (player.changedAnimOnThisFrame) {
					player.staggerElapsed = 0;
					player.staggerMaxFixed = false;
					player.prevBbscrvar = 0;
					player.prevBbscrvar5 = player.pawn.receivedAttack()->staggerDuration * 10;
				}
				if (!player.hitstop && !superflashInstigator) ++player.staggerElapsed;
				int staggerMax = player.prevBbscrvar5 / 10 + player.pawn.thisIsMinusOneIfEnteredHitstunWithoutHitstop();
				int animDur = player.pawn.currentAnimDuration();
				if (!bbscrvar) {
					player.staggerMax = staggerMax;
				} else if (!player.prevBbscrvar) {
					player.staggerMaxFixed = true;
					if (staggerMax - 4 < animDur) {
						staggerMax = animDur + 4;
					}
					player.staggerMax = staggerMax;
				}
				player.stagger = player.staggerMax - (player.pawn.currentAnimDuration() - 1);
				player.prevBbscrvar = bbscrvar;
			}
			
			player.airborne = player.airborne_afterTick();
			int prevFrameWakeupTiming = player.wakeupTiming;
			player.wakeupTiming = 0;
			CmnActIndex prevFrameCmnActIndex = player.cmnActIndex;
			player.cmnActIndex = cmnActIndex;
			if (cmnActIndex == CmnActFDown2Stand) {
				player.wakeupTiming = player.wakeupTimings.faceDown;
			} else if (cmnActIndex == CmnActBDown2Stand) {
				player.wakeupTiming = player.wakeupTimings.faceUp;
			} else if (cmnActIndex == CmnActWallHaritsukiGetUp) {
				player.wakeupTiming = 15;
			} else if (cmnActIndex == CmnActUkemi) {
				player.wakeupTiming = 9;
			}
			if (!prevFrameWakeupTiming && player.wakeupTiming) {
				player.wakeupTimingElapsed = 0;
			}
			if (player.wakeupTiming && !superflashInstigator) ++player.wakeupTimingElapsed;
			if (player.cmnActIndex == CmnActKirimomiUpper && ent.currentAnimDuration() == 2 && !ent.isRCFrozen()) {
				player.hitstunMax = player.hitstun;  // 5D6 hitstun
				player.hitstunElapsed = 0;
				player.hitstunContaminatedByRCSlowdown = false;
				player.hitstunMaxFloorbounceExtra = 0;
			}
			player.inBlockstunNextFrame = ent.inBlockstunNextFrame();
			if (player.inBlockstunNextFrame) {
				player.lastBlockWasFD = isHoldingFD(ent);
				player.lastBlockWasIB = !player.lastBlockWasFD && ent.successfulIB();
			}
			const char* animName = ent.animationName();
			player.onTheDefensive = player.blockstun
				|| player.inHitstun
				|| cmnActIndex == CmnActUkemi
				|| ent.gettingUp();
			if (!player.obtainedForceDisableFlags) {
				player.wasForceDisableFlags = ent.forceDisableFlags();
			}
			player.enableBlock = ent.enableBlock();
			bool prevFrameCanFaultlessDefense = player.canFaultlessDefense;
			// I gave up trying to understand how moves work. Hardcode/make shit up time!
			player.canFaultlessDefense = (player.enableBlock || player.wasEnableNormals || whiffCancelIntoFDEnabled(player))
				&& ent.fdNegativeCheck() == 0
				&& player.wasProhibitFDTimer == 0
				&& ent.dizzyMashAmountLeft() <= 0
				&& ent.exKizetsu() <= 0;
			player.fillInMove();
			bool idleNext = player.move.isIdle(player);
			if (player.airborne && player.move.forceLandingRecovery) player.moveOriginatedInTheAir = true;
			bool prevFrameIdle = player.idle;
			bool prevFrameIgnoreNextInabilityToBlockOrAttack = player.ignoreNextInabilityToBlockOrAttack;
			bool prevFrameCanBlock = player.canBlock;
			player.canBlock = player.move.canBlock(player);
			
			#define INVUL_TYPES_EXEC(enumName, stringDesc, fieldName) player.fieldName.active = false;
			INVUL_TYPES_TABLE
			#undef INVUL_TYPES_EXEC
			
			player.projectileOnlyInvul.active = !ent.strikeInvul()
				&& ent.strikeInvulnFrames() <= 0
				&& !player.wasFullInvul
				&& (
					player.wasSuperArmorEnabled
					&& ent.superArmorType() == SUPER_ARMOR_DODGE
					&& !ent.superArmorHontaiAttacck()
					&& !ent.superArmorForReflect()
				);
			player.reflect.active = !player.wasFullInvul
				&& (
					(
						player.wasSuperArmorEnabled
						|| ent.strikeInvul()
						|| ent.strikeInvulnFrames() > 0
					)
					&& ent.superArmorType() == SUPER_ARMOR_DODGE
					&& ent.superArmorForReflect()
				);
			
			player.counterhit = false;
			
			if (player.changedAnimOnThisFrame) {
				player.maxHit.clear();
				player.hitNumber = 0;
			}
			
			if (!player.leftBlockstunHitstun
					&& !player.receivedNewDmgCalcOnThisFrame
					&& (
						player.cmnActIndex == -1
						|| player.wakeupTiming
						|| !player.inHitstun
						&& !player.blockstun
						&& !player.pawn.blockCount()
						&& player.cmnActIndex != CmnActAirGuardLoop
						&& player.cmnActIndex != CmnActAirGuardEnd
						&& player.cmnActIndex != CmnActCrouchGuardLoop
						&& player.cmnActIndex != CmnActCrouchGuardEnd
						&& player.cmnActIndex != CmnActHighGuardLoop
						&& player.cmnActIndex != CmnActHighGuardEnd
						&& player.cmnActIndex != CmnActMidGuardLoop
						&& player.cmnActIndex != CmnActMidGuardEnd
					)) {
				player.leftBlockstunHitstun = true;
			}
			player.proration = ent.proration();
			player.dustProration1 = ent.dustProration1();
			player.dustProration2 = ent.dustProration2();
			player.comboProrationNormal = entityManager.calculateComboProration(ent.risc(), ATTACK_TYPE_NORMAL);
			player.comboProrationOverdrive = entityManager.calculateComboProration(ent.risc(), ATTACK_TYPE_OVERDRIVE);
			player.rcProration = other.pawn.rcSlowdownCounter() != 0 && !other.pawn.ignoreRcProration();
			
			if (!player.changedAnimOnThisFrame
					&& prevFrameIdle
					// Leaving Rifle or Pogo stance after spending too much time in it
					&& player.move.canBeUnableToBlockIndefinitelyOrForVeryLongTime
					&& !idleNext
					&& player.charType != CHARACTER_TYPE_LEO
				) {
				player.changedAnimOnThisFrame = true;
			}
			player.forceBusy = false;
			if (player.charType == CHARACTER_TYPE_LEO
					&& strcmp(animName, "Semuke"_hardcode) == 0
					&& (
						strcmp(ent.gotoLabelRequest(), "SemukeEnd"_hardcode) == 0
						|| !ent.enableWhiffCancels()
					)) {
				if (strcmp(ent.gotoLabelRequest(), "SemukeEnd"_hardcode) == 0) {
					player.changedAnimOnThisFrame = true;
				}
				player.canBlock = false;
				player.forceBusy = true;  // if I just set idleNext to false here, the framebar will display Leo as busy on that frame, but that's not true - he can act
			}
			if (player.changedAnimOnThisFrame
					&& (
						prevFrameCmnActIndex == CmnActBDownUpper
						&& player.cmnActIndex == CmnActBDownDown
						|| prevFrameCmnActIndex == CmnActFDownUpper
						&& player.cmnActIndex == CmnActFDownDown
						|| prevFrameCmnActIndex == CmnActVDownUpper
						&& player.cmnActIndex == CmnActVDownDown
						|| prevFrameCmnActIndex == CmnActJump
						&& player.cmnActIndex == CmnActJump
						&& ent.remainingDoubleJumps() != player.remainingDoubleJumps - 1
						|| !prevFrameBlockstun  // proximity blocking - for displaying gaps in framebar
						&& !player.blockstun
						&& idleNext  // exclude voluntary FD
						&& (
							(
								prevFrameCmnActIndex == CmnActMidGuardEnd
								|| prevFrameCmnActIndex == CmnActCrouchGuardEnd
								|| prevFrameCmnActIndex == CmnActHighGuardEnd
								|| prevFrameCmnActIndex == CmnActAirGuardEnd
								|| prevFrameCmnActIndex == CmnActMidGuardLoop
								|| prevFrameCmnActIndex == CmnActCrouchGuardLoop
								|| prevFrameCmnActIndex == CmnActHighGuardLoop
								|| prevFrameCmnActIndex == CmnActAirGuardLoop
							)
							|| prevFrameIdle
							&& (
								cmnActIndex == CmnActMidGuardEnd
								|| cmnActIndex == CmnActCrouchGuardEnd
								|| cmnActIndex == CmnActHighGuardEnd
								|| cmnActIndex == CmnActAirGuardEnd
								|| cmnActIndex == CmnActMidGuardLoop
								|| cmnActIndex == CmnActCrouchGuardLoop
								|| cmnActIndex == CmnActHighGuardLoop
								|| cmnActIndex == CmnActAirGuardLoop
							)
						)
						// for clearer display of frame advantage in the framebar, but show turning around and crouching
						|| prevFrameIdle
						&& idleNext
						&& (
							prevFrameCmnActIndex == CmnActStand2Crouch
							&& cmnActIndex == CmnActCrouch
							|| prevFrameCmnActIndex == CmnActCrouch2Stand
							&& cmnActIndex == CmnActStand
							|| prevFrameCmnActIndex == CmnActAirTurn
							&& cmnActIndex == CmnActJump
							|| prevFrameCmnActIndex == CmnActStandTurn
							&& cmnActIndex == CmnActStand
							|| prevFrameCmnActIndex == CmnActCrouchTurn
							&& cmnActIndex == CmnActCrouch
						)
					)) {
				player.changedAnimOnThisFrame = false;
			}
			int remainingDoubleJumps = ent.remainingDoubleJumps();
			int remainingAirDashes = ent.remainingAirDashes();
			if ((remainingDoubleJumps > player.remainingDoubleJumps
					|| remainingAirDashes > player.remainingAirDashes)
					&& player.airborne) {
				player.regainedAirOptions = true;
			}
			player.remainingDoubleJumps = ent.remainingDoubleJumps();
			player.remainingAirDashes = ent.remainingAirDashes();
			
			// for CmnActBDownUpper changing to CmnActBDownDown losing hitstop display. Try Sol j.D Kudakero airborne hit
			bool dontResetHitstopMax = player.changedAnimOnThisFrame
					&& player.onTheDefensive
					&& !(player.setBlockstunMax || player.setHitstunMax)
					&& !player.wakeupTiming;
			
			player.isLanding = cmnActIndex == CmnActJumpLanding
				|| cmnActIndex == CmnActLandingStiff
				|| player.theAnimationIsNotOverYetLolConsiderBusyNonAirborneFramesAsLandingAnimation
				&& !player.airborne
				&& !idleNext
				&& !player.onTheDefensive;
			player.isLandingOrPreJump = player.isLanding || cmnActIndex == CmnActJumpPre;
			if (cmnActIndex == CmnActJumpPre || player.airborne && !player.idle && !player.idleInNewSection) {  // second check needed for Venom Ball teleport
				player.frameAdvantageValid = false;
				other.frameAdvantageValid = false;
				player.landingFrameAdvantageValid = false;
				other.landingFrameAdvantageValid = false;
				player.frameAdvantageIncludesIdlenessInNewSection = false;
				other.frameAdvantageIncludesIdlenessInNewSection = false;
				other.eddie.frameAdvantageIncludesIdlenessInNewSection = false;
				player.landingFrameAdvantageIncludesIdlenessInNewSection = false;
				other.landingFrameAdvantageIncludesIdlenessInNewSection = false;
				other.eddie.landingFrameAdvantageIncludesIdlenessInNewSection = false;
				player.dontRestartTheNonLandingFrameAdvantageCountdownUponNextLanding = false;
			}
			memcpy(player.prevAttackLockAction, player.attackLockAction, 32);
			memcpy(player.attackLockAction, ent.dealtAttack()->attackLockAction, 32);
			player.animFrame = ent.currentAnimDuration();
			player.hitboxesCount = 0;
			
			if (idleNext != player.idle || player.wasIdle && !idleNext || (player.setBlockstunMax || player.setHitstunMax) && !idleNext) {
				player.idle = idleNext;
				if (!player.idle) {
					if (player.onTheDefensive) {
						player.startedDefending = true;
						if (player.cmnActIndex != CmnActUkemi && !player.baikenReturningToBlockstunAfterAzami) {
							player.hitstopMax = player.hitstop;
							player.hitstopElapsed = 0;
							if (player.setBlockstunMax) {
								--player.blockstunMax;  // this is the line PlayerInfo.h:PlayerInfo::inBlockstunNextFrame refers to
							}
							if (player.setHitstunMax) {
								player.hitstunMax = ent.hitstun();
								player.hitstunElapsed = 0;
								player.hitstunContaminatedByRCSlowdown = false;
								player.hitstunMaxFloorbounceExtra = 0;
								if (ent.inHitstunNextFrame() && player.lastHitstopBeforeWipe
										|| ent.hitstop()) {
									--player.hitstunMax;
								}
							}
						}
						
						if (other.timeSinceLastGap > 45) {
							other.clearGaps();
						} else {
							other.addGap(other.timeSinceLastGap);
						}
						other.timeSinceLastGap = 0;
					}
				} else if (player.airborne) {
					player.dontRestartTheNonLandingFrameAdvantageCountdownUponNextLanding = true;
					player.theAnimationIsNotOverYetLolConsiderBusyNonAirborneFramesAsLandingAnimation = true;
					player.landingFrameAdvantageValid = false;
					other.landingFrameAdvantageValid = false;
					player.landingFrameAdvantageIncludesIdlenessInNewSection = false;
					other.landingFrameAdvantageIncludesIdlenessInNewSection = false;
					other.landingFrameAdvantageIncludesIdlenessInNewSection = false;
				}
			}
			if (player.hitstun && !player.hitstop && !superflashInstigator) {
				++player.hitstunElapsed;
				if (player.rcSlowedDown) player.hitstunContaminatedByRCSlowdown = true;
			}
			if (player.tumble && !player.hitstop && !superflashInstigator) {
				++player.tumbleElapsed;
				if (player.rcSlowedDown) player.tumbleContaminatedByRCSlowdown = true;
			}
			if (player.hitstop && !superflashInstigator) ++player.hitstopElapsed;
			int newSlow;
			int unused;
			PlayerInfo::calculateSlow(player.hitstopElapsed,
				player.hitstop,
				player.rcSlowedDownCounter,
				&player.hitstopWithSlow,
				&player.hitstopMaxWithSlow,
				&newSlow);
			PlayerInfo::calculateSlow(player.hitstunElapsed,
				player.inHitstun
					? (
						player.hitstun - (player.hitstop ? 1 : 0)
					) : 0,
				newSlow,
				&player.hitstunWithSlow,
				&player.hitstunMaxWithSlow,
				&unused);
			PlayerInfo::calculateSlow(player.tumbleElapsed,
				player.tumble,
				newSlow,
				&player.tumbleWithSlow,
				&player.tumbleMaxWithSlow,
				&unused);
			PlayerInfo::calculateSlow(player.blockstunElapsed,
				player.blockstun - (player.hitstop ? 1 : 0),
				newSlow,
				&player.blockstunWithSlow,
				&player.blockstunMaxWithSlow,
				&unused);
			if (player.cmnActIndex == CmnActJitabataLoop) {
				PlayerInfo::calculateSlow(player.staggerElapsed,
					player.stagger,
					newSlow,
					&player.staggerWithSlow,
					&player.staggerMaxWithSlow,
					&unused);
			}
			if (player.wakeupTiming) {
				PlayerInfo::calculateSlow(player.wakeupTimingElapsed,
					player.wakeupTiming - player.animFrame + 1,
					player.rcSlowedDownCounter,
					&player.wakeupTimingWithSlow,
					&player.wakeupTimingMaxWithSlow,
					&unused);
			}
			int playerval0 = ent.playerVal(0);
			int playerval1 = ent.playerVal(1);
			const char* playervalSetterName = nullptr;
			int playervalNum = 1;
			int maxDI = 0;
			if (player.charType == CHARACTER_TYPE_SOL) {
				playervalSetterName = "DragonInstall";
				maxDI = player.wasPlayerval1Idling;
			} else if (player.charType == CHARACTER_TYPE_CHIPP) {
				playervalSetterName = "Meisai";
				playervalNum = 0;
				maxDI = player.pawn.playerVal(0);
			} else if (player.charType == CHARACTER_TYPE_MILLIA) {
				playervalSetterName = "ChromingRose";
				maxDI = player.wasPlayerval1Idling + 10;
			} else if (player.charType == CHARACTER_TYPE_SLAYER) {
				playervalSetterName = "ChiwosuuUchuuExe";
				maxDI = player.wasPlayerval1Idling;
			}
			if (playervalSetterName && strcmp(animName, playervalSetterName) == 0) {
				player.fillInPlayervalSetter(playervalNum);
				if (player.playervalSetterOffset != -1
						&& player.pawn.bbscrCurrentInstr() - player.pawn.bbscrCurrentFunc() == player.playervalSetterOffset
						&& !player.pawn.isRCFrozen() && player.pawn.spriteFrameCounter() == 0) {
					player.maxDI = maxDI;
				}
			}
			player.playerval1 = playerval1;
			player.playerval0 = playerval0;
			
			int poisonDuration = ent.poisonDuration();
			if (!player.poisonDuration) {
				player.poisonDurationMax = poisonDuration;
			} else if (poisonDuration > player.poisonDuration) {
				player.poisonDurationMax = poisonDuration;
			}
			player.poisonDuration = poisonDuration;
			
			player.maxHit.fill(ent, ent.currentHitNum());
			
			bool isHitOnThisFrame = player.pawn.inHitstunNextFrame();
			player.wasHitOnPreviousFrame = player.wasHitOnThisFrame;
			player.wasHitOnThisFrame = isHitOnThisFrame;
			
			bool clearSuperStuff = false;
			if (player.inHitstunNowOrNextFrame && !(player.wasEnableAirtech && player.hitstun == 0)) {
				if (!player.gettingHitBySuper
						&& other.move.forceSuperHitAnyway
						&& other.move.forceSuperHitAnyway(other)) {
					player.gettingHitBySuper = true;
				} else if (player.gettingHitBySuper
						&& (
							other.recovery
							&& other.move.iKnowExactlyWhenTheRecoveryOfThisMoveIs
							&& other.move.iKnowExactlyWhenTheRecoveryOfThisMoveIs(other)
							|| other.landingRecovery
						)) {
					clearSuperStuff = true;
				}
			} else {
				clearSuperStuff = true;
			}
			if (clearSuperStuff) {
				player.gettingHitBySuper = false;
				player.startedSuperWhenComboing = false;
			}
			
			player.gettingUp = ent.gettingUp();
			bool idlePlus = player.idle
				|| player.idleInNewSection
				|| player.dontRestartTheNonLandingFrameAdvantageCountdownUponNextLanding
				&& player.isLanding;
			if (idlePlus != player.idlePlus) {
				player.timePassed = 0;
				player.idlePlus = idlePlus;
			}
			player.sprite.fill(ent);
			player.isInFDWithoutBlockstun = (
					player.cmnActIndex == CmnActCrouchGuardLoop
					|| player.cmnActIndex == CmnActMidGuardLoop
					|| player.cmnActIndex == CmnActAirGuardLoop
					|| player.cmnActIndex == CmnActHighGuardLoop
				)
				&& player.blockstun == 0
				&& (
					!player.idle  // idle check needed because proxyguard also triggers these animations
					|| !player.canBlock  // block check also needed because 5D6 cannot be immediately FD cancelled
					&& !player.ignoreNextInabilityToBlockOrAttack  // this check must always follow the canBlock check
				);
			bool needDisableProjectiles = false;
			player.changedAnimFiltered = false;
			bool changedAnim = player.changedAnimOnThisFrame;
			if (changedAnim) {
				if (player.cmnActIndex == CmnActJump && !player.canFaultlessDefense) {
					changedAnim = false;
				}
			} else if (player.cmnActIndex == CmnActJump
					&& prevFrameCmnActIndex == CmnActJump
					&& !prevFrameCanFaultlessDefense
					&& player.canFaultlessDefense) {
			}
			if (changedAnim) {
				player.wokeUp = prevFrameWakeupTiming && !player.wakeupTiming && idleNext;
				if (!player.move.preservesNewSection) {
					player.inNewMoveSection = false;
					if (!measuringFrameAdvantage) {
						player.frameAdvantageIncludesIdlenessInNewSection = true;
						other.frameAdvantageIncludesIdlenessInNewSection = true;
						other.eddie.frameAdvantageIncludesIdlenessInNewSection = true;
					}
					if (measuringLandingFrameAdvantage == -1) {
						player.landingFrameAdvantageIncludesIdlenessInNewSection = true;
						other.landingFrameAdvantageIncludesIdlenessInNewSection = true;
						other.eddie.landingFrameAdvantageIncludesIdlenessInNewSection = true;
					}
					if (player.idleInNewSection) {
						player.idleInNewSection = false;
						player.idlePlus = false;
						player.timePassed = 0;
					}
				}
				player.changedAnimFiltered = !(
					player.move.combineWithPreviousMove
					&& !player.move.usePlusSignInCombination
					&& !(
						player.charType == CHARACTER_TYPE_HAEHYUN
						&& strcmp(player.anim, "AntiAirAttack"_hardcode) != 0
						&& strcmp(animName, "AntiAir4Hasei"_hardcode) == 0
						|| player.charType == CHARACTER_TYPE_JOHNNY
						&& ent.mem54()  // when Mem(54) is set, MistFinerLoop instantly transitions to a chosen Mist Finer. However, in Rev1, it takes one frame
						&& (
							strcmp(animName, "MistFinerLoop"_hardcode) == 0
							|| strcmp(animName, "AirMistFinerLoop"_hardcode) == 0
						)
					)
					|| player.charType == CHARACTER_TYPE_JAM
					&& strcmp(player.anim, "NeoHochihu"_hardcode) == 0  // 54625 causes NeoHochihu to cancel into itself
					&& strcmp(animName, "NeoHochihu"_hardcode) == 0
					&& !player.wasEnableWhiffCancels
					|| player.charType == CHARACTER_TYPE_JOHNNY
					&& ent.mem54()
					&& (
						strcmp(player.anim, "MistFinerLoop"_hardcode) == 0
						&& (
							strncmp(animName, "MistFinerALv"_hardcode, 12) == 0
							|| strncmp(animName, "MistFinerBLv"_hardcode, 12) == 0
							|| strncmp(animName, "MistFinerCLv"_hardcode, 12) == 0
						)
						|| strcmp(player.anim, "AirMistFinerLoop"_hardcode) == 0
						&& (
							strncmp(animName, "AirMistFinerALv"_hardcode, 15) == 0
							|| strncmp(animName, "AirMistFinerBLv"_hardcode, 15) == 0
							|| strncmp(animName, "AirMistFinerCLv"_hardcode, 15) == 0
						)
					)
				);
				if (!player.changedAnimFiltered) {
					player.determineMoveNameAndSlangName(&player.lastPerformedMoveName, &player.lastPerformedMoveSlangName);
				}
				if (*player.prevAttackLockAction != '\0' && strcmp(animName, player.prevAttackLockAction) == 0
						&& !player.move.dontSkipGrab) {
					player.grab = true;  // this doesn't work on regular ground and air throws
					memcpy(player.grabAnimation, player.prevAttackLockAction, 32);
				}
				if (strcmp(animName, player.grabAnimation) != 0) {
					player.grab = false;
				}
				if (
						!player.changedAnimFiltered
						&& !(
							player.charType == CHARACTER_TYPE_BEDMAN
							&& strcmp(player.anim, "Boomerang_B"_hardcode) != 0
							&& strcmp(player.anim, "Boomerang_B_Air"_hardcode) != 0
							&& strcmp(animName, "BWarp"_hardcode) == 0
						)
						|| !player.moveNonEmpty
						&& *player.prevAttackLockAction != '\0'
						&& strcmp(animName, player.prevAttackLockAction) == 0
						|| player.isInFDWithoutBlockstun  // this check is here and not down below because I don't want air button +
						                                  // + recover in air + short FD + keep falling = to mess up the moveOriginatedInTheAir flag
						&& (
							!prevFrameIdle
							|| !prevFrameCanBlock
							&& !prevFrameIgnoreNextInabilityToBlockOrAttack
						)
						&& !player.prejumped
					) {
					// do nothing
				} else {
					player.moveOriginatedInTheAir = false;
					player.theAnimationIsNotOverYetLolConsiderBusyNonAirborneFramesAsLandingAnimation = false;  // this assignment is here and not down below,
					            // because it's for custom landing animations that are part of moves like May Hop on Dolphin or CounterGuardAir (Air Blitz whiff),
					            // and those animations don't change to another animation all the way until the landing recovery is over, so I want to know
					            // that the animation changed to whatever other one and be sure that the player being busy on the ground now is not
					            // part of a custom landing animation 100%.
					            // This assignment is not higher above because some animations I just consider an inseparable part of another animation.
		            bool conditionPartOne = !(
		            			player.isLandingOrPreJump
		            			&& !(
									player.charType == CHARACTER_TYPE_SLAYER
				        			&& cmnActIndex == CmnActJumpPre
				        			&& prevFrameCmnActIndex == CmnActBDash
			        			)
		            		)
							&& player.cmnActIndex != CmnActJump
							&& (!player.idlePlus || player.forceBusy);
		            bool conditionPartTwo = !(
								!player.wasIdle
								&& player.cmnActIndex == CmnActRomanCancel
								&& player.startedUp
							);
					if (conditionPartOne && conditionPartTwo) {
						
						if (!player.baikenReturningToBlockstunAfterAzami  // technically Baiken changes animation when she puts herself into blockstun from a successful Azami with no followup,
						                                                  // and normally we stop displaying old hitstop when the animation changes, but in this case we should keep it
								&& !dontResetHitstopMax) {  // for CmnActBDownUpper changing to CmnActBDownDown losing hitstop display
							player.displayHitstop = false;
						}
						if (!player.onTheDefensive) {
							player.xStunDisplay = PlayerInfo::XSTUN_DISPLAY_NONE;
						}
						player.moveOriginatedInTheAir = player.airborne;
						
						player.startupProj = 0;
						player.hitOnFrameProj = 0;
						player.startupProjIgnoredForFramebar = 0;
						player.activesProj.clear();
						player.maxHitUse.clear();
						player.prevStartupsProj.clear();
						player.maxHitProj.clear();
						player.maxHitProjConflict = false;
						player.maxHitProjLastPtr = nullptr;
						
						if (player.move.usePlusSignInCombination
								|| player.charType == CHARACTER_TYPE_BAIKEN
								&& (
									strcmp(player.anim, "BlockingStand"_hardcode) == 0
									|| strcmp(player.anim, "BlockingCrouch"_hardcode) == 0
									|| strcmp(player.anim, "BlockingAir"_hardcode) == 0
								)
								&& (
									strcmp(animName, "BlockingStand"_hardcode) == 0
									|| strcmp(animName, "BlockingCrouch"_hardcode) == 0
									|| strcmp(animName, "BlockingAir"_hardcode) == 0
								)
								|| player.charType == CHARACTER_TYPE_ELPHELT
								&& !player.idle
								&& prevFrameCmnActIndex == CmnActRomanCancel
								&& strcmp(animName, "Rifle_Roman"_hardcode) == 0
								|| player.charType == CHARACTER_TYPE_JOHNNY
								&& !player.wasIdle
								&& (
									strcmp(animName, "MistFinerA"_hardcode) == 0
									|| strcmp(animName, "MistFinerB"_hardcode) == 0
									|| strcmp(animName, "MistFinerC"_hardcode) == 0
									|| strcmp(animName, "AirMistFinerA"_hardcode) == 0
									|| strcmp(animName, "AirMistFinerB"_hardcode) == 0
									|| strcmp(animName, "AirMistFinerC"_hardcode) == 0
								)
								|| player.charType == CHARACTER_TYPE_POTEMKIN
								&& !player.wasIdle
								&& (
									strcmp(animName, "HammerFall"_hardcode) == 0
								)
								|| !player.wasIdle
								&& player.cmnActIndex == CmnActRomanCancel
								&& !player.startedUp
								|| player.charType == CHARACTER_TYPE_SLAYER
			        			&& (
			        				player.performingBDC
			        				&& prevFrameCmnActIndex == CmnActJumpPre
			        				&& !player.airborne
			        				|| prevFrameCmnActIndex == CmnActBDash
		        				)
		        				&& !idleNext
		        				&& !player.inHitstunNowOrNextFrame) {
							if (player.superfreezeStartup) {
								player.prevStartups.add(player.superfreezeStartup,
									player.lastMoveIsPartOfStance,
									player.lastPerformedMoveName,
									player.lastPerformedMoveSlangName);
								player.total -= player.superfreezeStartup;  // needed for Elphelt Rifle RC
								player.superfreezeStartup = 0;
							}
							player.prevStartups.add(player.total,
								player.lastMoveIsPartOfStance,
								player.lastPerformedMoveName,
								player.lastPerformedMoveSlangName);
							if (player.charType == CHARACTER_TYPE_JOHNNY
									&& (
										strncmp(animName, "MistFinerALv"_hardcode, 12) == 0
										|| strncmp(animName, "MistFinerBLv"_hardcode, 12) == 0
										|| strncmp(animName, "MistFinerCLv"_hardcode, 12) == 0
										|| strncmp(animName, "AirMistFinerALv"_hardcode, 15) == 0
										|| strncmp(animName, "AirMistFinerBLv"_hardcode, 15) == 0
										|| strncmp(animName, "AirMistFinerCLv"_hardcode, 15) == 0
										|| strcmp(animName, "StepTreasureHunt"_hardcode) == 0
									)
							) {
								player.removeNonStancePrevStartups();
							}
						} else {
							player.onAnimReset();
						}
						
						player.airteched = player.cmnActIndex == CmnActUkemi;
						player.moveStartTime_aswEngineTick = player.startup;
						player.startup = 0;
						player.startedUp = false;
						player.actives.clear();
						player.maxHit.clear();
						player.recovery = 0;
						player.total = 0;
						player.lastMoveIsPartOfStance = player.move.partOfStance;
						player.determineMoveNameAndSlangName(&player.lastPerformedMoveName, &player.lastPerformedMoveSlangName);
						player.hitOnFrame = 0;
						player.totalFD = 0;
						player.totalCanBlock = 0;
						player.totalCanFD = 0;
						player.ignoreNextInabilityToBlockOrAttack = false;
						player.performingASuper = player.pawn.dealtAttack()->type == ATTACK_TYPE_OVERDRIVE
							&& !player.move.dontSkipSuper;
						if (player.performingASuper) {
							player.startedSuperWhenComboing = other.pawn.comboCount() > 1;
						} else {
							player.startedSuperWhenComboing = false;
						}
						other.gettingHitBySuper = false;
						
						player.dontRestartTheNonLandingFrameAdvantageCountdownUponNextLanding = false;
						
						player.landingRecovery = 0;
						player.superfreezeStartup = 0;
						
						player.hitboxTopBottomValid = false;
						player.hurtboxTopBottomValid = false;
						player.prejumped = false;
						player.performingBDC = player.charType == CHARACTER_TYPE_SLAYER
		        			&& cmnActIndex == CmnActJumpPre
		        			&& prevFrameCmnActIndex == CmnActBDash;
						
						if (player.animFrame == 1) {
							int actualMoveIndex = player.pawn.currentMoveIndex();
							if (actualMoveIndex == -1) {
								player.regainedAirOptions = false;
							} else if (!player.wasIdle && player.cmnActIndex == CmnActRomanCancel && player.airborne) {
								player.regainedAirOptions = true;
							} else {
								const AddedMoveData* actualMove = player.pawn.movesBase() + actualMoveIndex;
								if (
										actualMove->type == MOVE_TYPE_SPECIAL
										&& !(
											actualMove->characterState == MOVE_CHARACTER_STATE_JUMPING
											|| actualMove->characterState == MOVE_CHARACTER_STATE_NONE
											&& player.airborne
										)
										&& (
											player.remainingDoubleJumps != 0
											|| player.remainingAirDashes != 0
										)
								) {
									player.regainedAirOptions = true;
								} else {
									player.regainedAirOptions = false;
								}
							}
						}
						
						if (player.cmnActIndex != CmnActRomanCancel) {
							needDisableProjectiles = true;
						}
					} else if (conditionPartOne && !conditionPartTwo) {
						// Jam Hououshou final grounded leg kick RRC its startup - without this fix the frame after RRC superfreeze will be skipped
						// and the frame tooltip after will say "skipped: 1 super, 18 superfreeze, 1 super" which is very weird
						player.performingASuper = false;
						other.gettingHitBySuper = false;
					}
				}
			}
			memcpy(player.anim, animName, 32);
			player.setMoveName(player.moveName, ent);
			
			if (player.idle && (
					player.canBlock && player.canFaultlessDefense
					|| player.wasIdle && changedAnim  // needed for linking a normal with a forward dash
				)) {
				player.ignoreNextInabilityToBlockOrAttack = true;
				player.performingASuper = false;
				other.gettingHitBySuper = false;
			}
			if (player.cmnActIndex == CmnActJumpPre && !player.performingBDC) {
				player.prejumped = true;
			}
			
			// This is needed for animations that create projectiles on frame 1
			for (OccuredEvent& event : events) {
				if (event.type == OccuredEvent::SET_ANIM
						&& needDisableProjectiles
						&& event.u.setAnim.pawn == ent) {
					for (ProjectileInfo& projectile : projectiles) {
						bool parentDisabled = false;
						if (projectile.creator && projectile.creator != players[0].pawn && projectile.creator != players[1].pawn) {
							for (ProjectileInfo& seekParent : projectiles) {
								if (seekParent.ptr && seekParent.ptr != projectile.ptr && seekParent.ptr == projectile.creator) {
									parentDisabled = seekParent.disabled;
									break;
								}
							}
						}
						if (projectile.team == player.index && (projectile.lifeTimeCounter > 0 || parentDisabled)) {
							projectile.disabled = true;
							projectile.startedUp = false;
							projectile.actives.clear();
							projectile.hitOnFrame = 0;
							projectile.maxHit.clear();
						}
					}
				} else if (event.type == OccuredEvent::SIGNAL) {
					PlayerInfo& playerFrom = findPlayer(event.u.signal.from);
					ProjectileInfo& projectileFrom = findProjectile(event.u.signal.from);
					ProjectileInfo& projectileTo = findProjectile(event.u.signal.to);
					if (playerFrom.pawn
							&& projectileTo.ptr
							&& playerFrom.pawn == ent
							&& projectileTo.team == player.index) {
						if (projectileTo.disabled) {
							projectileTo.creationTime_aswEngineTick = aswEngineTickCount;
						}
						projectileTo.disabled = false;
						projectileTo.startup = player.total + player.prevStartups.total();  // + prevStartups.total() needed for Venom QV
						if (player.totalCanBlock > player.total) {  // needed for Answer Taunt
							projectileTo.startup += player.totalCanBlock - player.total;
						}
						projectileTo.total = projectileTo.startup;
						memcpy(projectileTo.creatorName, event.u.signal.fromAnim, 32);
						projectileTo.creator = event.u.signal.from;
					} else if (projectileFrom.ptr
							&& projectileTo.ptr
							&& projectileFrom.team == player.index
							&& projectileTo.team == player.index) {
						if (projectileTo.disabled) {
							projectileTo.creationTime_aswEngineTick = aswEngineTickCount;
						}
						projectileTo.disabled = projectileFrom.disabled;  // Sol Gunflame YRC delay 1f 5P. This prevents it from thinking Gunflame active frames are part of 5P
						projectileTo.startup = projectileFrom.total;
						projectileTo.total = projectileFrom.total;
						memcpy(projectileTo.creatorName, event.u.signal.fromAnim, 32);
						projectileTo.creator = event.u.signal.from;
					}
				}
			}
			
			if (!player.hitstop
					&& getSuperflashInstigator() == nullptr
					&& !player.startedUp
					&& !player.inNewMoveSection
					&& !(
						player.charType == CHARACTER_TYPE_ELPHELT
						&& strcmp(player.anim, "Rifle_Roman"_hardcode) == 0
						&& player.idle
						&& player.canBlock
					)
					&& player.move.sectionSeparator
					&& player.move.sectionSeparator(player)) {
				player.inNewMoveSection = true;
				player.changedAnimOnThisFrame = true;  // for framebar
				player.changedAnimFiltered = true;  // for framebar
				player.timeInNewSection = 0;
				if (!player.startedUp && player.total) {
					if (player.superfreezeStartup) {
						player.prevStartups.add(player.superfreezeStartup,
							player.lastMoveIsPartOfStance,
							player.lastPerformedMoveName,
							player.lastPerformedMoveSlangName);
						player.total -= player.superfreezeStartup;  // needed for Johnny Treasure Hunt
						player.superfreezeStartup = 0;
					}
					if (player.total) {
						player.prevStartups.add(player.total,
							player.lastMoveIsPartOfStance,
							player.lastPerformedMoveName,
							player.lastPerformedMoveSlangName);
					}
					player.startup = 0;
					player.determineMoveNameAndSlangName(&player.lastPerformedMoveName, &player.lastPerformedMoveSlangName);
					player.total = 0;
					player.hitOnFrame = 0;
					player.totalCanBlock = 0;
					player.totalCanFD = 0;
				}
			}
			if (!player.idleInNewSection && player.isIdleInNewSection()) {
				player.idleInNewSection = true;
				if (!player.idlePlus) {
					player.idlePlus = true;
					player.timePassed = player.timeInNewSection;
					if (!player.frameAdvantageIncludesIdlenessInNewSection) {
						player.frameAdvantageIncludesIdlenessInNewSection = true;
						player.frameAdvantage += player.timeInNewSection;
						player.frameAdvantageNoPreBlockstun += player.timeInNewSection;
					}
					if (!other.frameAdvantageIncludesIdlenessInNewSection) {
						other.frameAdvantageIncludesIdlenessInNewSection = true;
						other.frameAdvantage -= player.timeInNewSection;
						other.frameAdvantageNoPreBlockstun -= player.timeInNewSection;
					}
					if (!other.eddie.frameAdvantageIncludesIdlenessInNewSection) {
						other.eddie.frameAdvantageIncludesIdlenessInNewSection = true;
						other.eddie.frameAdvantage -= player.timeInNewSection;
					}
					if (!player.airborne) {
						if (!player.landingFrameAdvantageIncludesIdlenessInNewSection) {
							player.landingFrameAdvantageIncludesIdlenessInNewSection = true;
							player.landingFrameAdvantage += player.timeInNewSection;
							player.landingFrameAdvantageNoPreBlockstun += player.timeInNewSection;
						}
						if (!other.landingFrameAdvantageIncludesIdlenessInNewSection) {
							other.landingFrameAdvantageIncludesIdlenessInNewSection = true;
							other.landingFrameAdvantage -= player.timeInNewSection;
							other.landingFrameAdvantageNoPreBlockstun -= player.timeInNewSection;
						}
						if (!other.eddie.landingFrameAdvantageIncludesIdlenessInNewSection) {
							other.eddie.landingFrameAdvantageIncludesIdlenessInNewSection = true;
							other.eddie.landingFrameAdvantage -= player.timeInNewSection;
						}
					}
				}
			}
		}
		for (int i = 0; i < 2; ++i) {
			PlayerInfo& player = players[i];
			
			player.receivedComboCountTensionGainModifier = entityManager.calculateReceivedComboCountTensionGainModifier(
				player.inHitstunNowOrNextFrame,
				player.pawn.comboCount());
			player.dealtComboCountTensionGainModifier = entityManager.calculateDealtComboCountTensionGainModifier(
				players[1 - i].inHitstunNowOrNextFrame,
				players[1 - i].pawn.comboCount());
			player.burstGainOnly20Percent = player.pawn.burstGainOnly20Percent();
			player.burstGainModifier = (player.pawn.comboCount() + 32) * 100 / 32;
			if (game.isStylish(player.pawn)) {
				player.stylishBurstGainModifier = game.getStylishBurstGainModifier();
			} else {
				player.stylishBurstGainModifier = 100;
			}
		}
		
		// This is down here because throughout the logic tick in various hooks we gather
		// events that refer to objects via pointer, and some objects like May beachball
		// can get deleted on the same frame they receive a signal if they also happen
		// to hit something on that frame, so we process events before setting
		// deleted projectiles' ptr to null
		for (auto it = projectiles.begin(); it != projectiles.end();) {
			bool found = false;
			ProjectileInfo& projectile = *it;
			if (projectile.ptr) {
				for (int i = 0; i < entityList.count; ++i) {
					Entity ent = entityList.list[i];
					if (ent.isActive() && projectile.ptr == ent) {
						found = true;
						break;
					}
				}
			}
			if (!found) {
				if (projectile.landedHit || projectile.gotHitOnThisFrame) {
					projectile.ptr = nullptr;
					projectile.markActive = projectile.landedHit;
					++it;
					continue;
				}
				it = projectiles.erase(it);
			} else {
				projectile.fill(projectile.ptr, superflashInstigator, false);
				projectile.markActive = projectile.landedHit;
				if (projectile.team == 0 || projectile.team == 1) {
					projectile.fillInMove();
					if (!projectile.moveNonEmpty) {
						projectile.isDangerous = false;
					} else {
						projectile.isDangerous = projectile.move.isDangerous && projectile.move.isDangerous(projectile.ptr);
						if (!projectile.disabled) {
							PlayerInfo& player = players[projectile.team];
							if (projectile.isDangerous) {
								player.hasDangerousNonDisabledProjectiles = true;
							}
							if (projectile.lifeTimeCounter == 0
									//&& projectile.creator == player.pawn  // May Beach Ball is not directly created by the player
									&& !player.idle) {
								if (projectile.isDangerous) {
									player.createdDangerousProjectile = true;
								} else {
									player.createdProjectileThatSometimesCanBeDangerous = true;
								}
							}
						}
					}
				}
				if (!projectile.inNewSection
						&& projectile.move.sectionSeparatorProjectile
						&& projectile.move.sectionSeparatorProjectile(projectile.ptr)) {
					projectile.inNewSection = true;
					if (projectile.total) {
						projectile.prevStartups.add(projectile.total, false, projectile.lastName, projectile.lastSlangName);
					}
					projectile.determineMoveNameAndSlangName(&projectile.lastName, &projectile.lastSlangName);
					projectile.startup = 0;
					projectile.total = 0;
					projectile.hitOnFrame = 0;
				}
				++it;
			}
		}
		
		// This is a separate loop because it depends on another player's idlePlus, which I changed in the previous loop
		for (PlayerInfo& player : players) {
			PlayerInfo& other = players[1 - player.index];
			
			bool actualIdle = player.idle || player.idleInNewSection;
			
			if (player.idleLanding != actualIdle
					&& !(!player.idleLanding && player.airborne)) {  // we can't change idleLanding from false to true while player is airborne
				player.idleLanding = actualIdle;
				if (player.idleLanding && player.isLanding) {
					if (other.idleLanding) {  // no need to worry about the value of other.idleLanding being outdated here,
						                      // because if they're landing on the same frame, they will enter the if branch
						                      // immediately below and reset measuringLandingFrameAdvantage to -1
						measuringLandingFrameAdvantage = -1;
						if (other.timePassedLanding > player.timePassedLanding) {
							player.landingFrameAdvantage = -player.timePassedLanding;
							player.landingFrameAdvantageNoPreBlockstun = -player.timePassedLanding;
						} else {
							player.landingFrameAdvantage = -other.timePassedLanding;
							player.landingFrameAdvantageNoPreBlockstun = -other.timePassedLanding;
						}
						other.landingFrameAdvantage = -player.landingFrameAdvantage;
						other.landingFrameAdvantageNoPreBlockstun = -player.landingFrameAdvantage;
						player.landingFrameAdvantageValid = true;
						other.landingFrameAdvantageValid = true;
					} else if (measuringLandingFrameAdvantage == -1) {
						other.landingFrameAdvantageIncludesIdlenessInNewSection = false;
						player.landingFrameAdvantageIncludesIdlenessInNewSection = false;
						measuringLandingFrameAdvantage = player.index;
						player.landingFrameAdvantageValid = false;
						other.landingFrameAdvantageValid = false;
						player.landingFrameAdvantage = 0;
						other.landingFrameAdvantage = 0;
						player.landingFrameAdvantageNoPreBlockstun = 0;
						other.landingFrameAdvantageNoPreBlockstun = 0;
						other.eddie.landingFrameAdvantageValid = false;
					}
				}
				if (player.idleInNewSection) {
					player.timePassedLanding = player.timeInNewSection;
				} else {
					player.timePassedLanding = 0;
				}
				if (player.idleLanding) {
					player.airteched = false;
					player.regainedAirOptions = false;
				}
			}
			
			if (player.charType == CHARACTER_TYPE_RAMLETHAL) {
				
				int playerval0 = player.wasPlayerval[0];
				int playerval1 = player.wasPlayerval[1];
				int playerval2 = player.wasPlayerval[2];
				int playerval3 = player.wasPlayerval[3];
				
				const char* anim;
				const char* subAnim;
				int timeLeft;
				int slowdown;
				bool isKowareSonoba;
				int elapsed;
				bool isTrance;
				bool isCalvados;
				
				struct BitInfo {
					StringWithLength stateName;
					StringWithLength stateName2;
					StringWithLength stateName3;
					int hasSword;
					int swordDeployed;
					int stackIndex;
					std::vector<Moves::RamlethalSwordInfo>& info;
					std::vector<Moves::RamlethalSwordInfo>& info2;
					const char** anim;
					const char** subanim;
					int* time;
					int* timeMax;
				};
				BitInfo bitInfos[2] {
					{
						"BitN6C",
						"BitN2C_Bunri",
						"Bit4C",
						playerval0,
						playerval1,
						0,
						moves.ramlethalBitN6C,
						moves.ramlethalBitN2C,
						&player.ramlethalSSwordAnim,
						&player.ramlethalSSwordSubanim,
						&player.ramlethalSSwordTime,
						&player.ramlethalSSwordTimeMax
					},
					{
						"BitF6D",
						"BitF2D_Bunri",
						"Bit4D",
						playerval2,
						playerval3,
						1,
						moves.ramlethalBitF6D,
						moves.ramlethalBitF2D,
						&player.ramlethalHSwordAnim,
						&player.ramlethalHSwordSubanim,
						&player.ramlethalHSwordTime,
						&player.ramlethalHSwordTimeMax
					}
				};
				
				static const char* subAnims[Moves::ram_number_of_elements] {
					"Undefined",
					"Teleporting",
					"Attack",
					"Hit Not Deployed",
					"Hit Deployed",
					"Falling",
					"Landing",
					"Ramlethal Blocked",
					"Win"
				};
				
				static const char* subAnims2[Moves::ram2_number_of_elements] {
					"Undefined",
					"Teleporting",
					"Attack",
					"Win",
					"Hit",
					"Falling",
					"Landing",
					"Ramlethal Blocked"
				};
				
				for (int j = 0; j < 2; ++j) {
					BitInfo& bitInfo = bitInfos[j];
					
					anim = "???";
					subAnim = nullptr;
					timeLeft = 0;
					slowdown = 0;
					isKowareSonoba = false;
					elapsed = 0;
					isTrance = false;
					isCalvados = false;
					
					Entity p = player.pawn.stackEntity(bitInfo.stackIndex);
					
					if (p && p.isActive()) {
						anim = p.animationName();
						if (bitInfo.stackIndex == 0) {
							isTrance = strcmp(anim, "BitSpiral_NSpiral") == 0;
						} else {
							isTrance = strcmp(anim, "BitSpiral_FSpiral") == 0;
						}
						if (!isTrance) {
							if (bitInfo.stackIndex == 0) {
								isTrance = strcmp(anim, "BitSpiral_NSpiral") == 0;
							} else {
								isTrance = strcmp(anim, "BitSpiral_FSpiral") == 0;
							}
						}
						ProjectileInfo& projectile = endScene.findProjectile(p);
						if (projectile.ptr && projectile.move.displayName) {
							elapsed = projectile.ramlethalSwordElapsedTime;
							anim = projectile.move.displayName;
						}
						if (strcmp(p.animationName(), bitInfo.stateName.txt) == 0) {
							BYTE* func = p.bbscrCurrentFunc();
							moves.fillInRamlethalBitN6C_F6D(func, bitInfo.info);
							int offset = p.bbscrCurrentInstr() - func;
							bool mem45 = p.mem45();
							for (const Moves::RamlethalSwordInfo& info : bitInfo.info) {
								const Moves::MayIrukasanRidingObjectInfo& vec = mem45 ? info.framesBunri : info.framesSoubi;
								if (offset >= vec.frames.front().offset && offset <= vec.frames.back().offset) {
									subAnim = subAnims[info.state];
									if (info.state == Moves::ram_teleport) {
										timeLeft = vec.remainingTime(offset, p.spriteFrameCounter())
											+ bitInfo.info[(int)Moves::ram_Attack - 1].select(mem45).totalFrames;
									} else if (info.state == Moves::ram_Attack
											|| info.state == Moves::ram_koware_soubi
											|| info.state == Moves::ram_landing
											|| info.state == Moves::ram_koware_nokezori
											|| info.state == Moves::ram_Win) {
										timeLeft = vec.remainingTime(offset, p.spriteFrameCounter());
									} else if (info.state == Moves::ram_koware_sonoba || info.state == Moves::ram_loop) {
										isKowareSonoba = true;
										timeLeft = bitInfo.info[(int)Moves::ram_landing - 1].select(mem45).totalFrames;
									}
									if (projectile.ptr) {
										slowdown = projectile.rcSlowedDownCounter;
									}
									break;
								}
							}
						} else if (strcmp(p.animationName(), bitInfo.stateName2.txt) == 0) {
							BYTE* func = p.bbscrCurrentFunc();
							moves.fillInRamlethalBitN6C_F6D(func, bitInfo.info2);
							int offset = p.bbscrCurrentInstr() - func;
							for (const Moves::RamlethalSwordInfo& info : bitInfo.info2) {
								if (offset >= info.framesBunri.frames.front().offset && offset <= info.framesBunri.frames.back().offset) {
									subAnim = subAnims2[info.state];
									if (info.state == Moves::ram2_teleport) {
										timeLeft = info.framesBunri.remainingTime(offset, p.spriteFrameCounter())
											+ bitInfo.info2[(int)Moves::ram2_Attack - 1].framesBunri.totalFrames;
									} else if (info.state == Moves::ram2_Attack
											|| info.state == Moves::ram2_landing
											|| info.state == Moves::ram2_koware_nokezori
											|| info.state == Moves::ram2_Win) {
										timeLeft = info.framesBunri.remainingTime(offset, p.spriteFrameCounter());
									} else if (info.state == Moves::ram2_koware || info.state == Moves::ram2_loop) {
										isKowareSonoba = true;
										timeLeft = bitInfo.info2[(int)Moves::ram2_landing - 1].framesBunri.totalFrames;
									}
									if (projectile.ptr) {
										slowdown = projectile.rcSlowedDownCounter;
									}
									break;
								}
							}
						} else if (strcmp(p.animationName(), bitInfo.stateName3.txt) == 0) {
							timeLeft = 7 - p.currentAnimDuration() + 1;
							if (projectile.ptr) {
								slowdown = projectile.rcSlowedDownCounter;
							}
						}
					}
					
					*bitInfo.subanim = subAnim;
					*bitInfo.anim = anim;
					
					bool timerActive = timeLeft
							|| !(bitInfo.swordDeployed || bitInfo.hasSword)
							&& !isTrance
							&& !isCalvados;
					
					if (bitInfo.stackIndex == 0) {
						player.ramlethalSSwordTimerActive = timerActive;
					} else {
						player.ramlethalHSwordTimerActive = timerActive;
					}
					
					if (timerActive) {  // it takes an extra frame for the change in PLAYERVAL to affect the player
						int result;
						int resultMax;
						int unused;
						PlayerInfo::calculateSlow(
							elapsed + 1,
							timeLeft,
							slowdown,
							&result,
							&resultMax,
							&unused);
						if (bitInfo.stackIndex == 0) {
							player.ramlethalSSwordKowareSonoba = isKowareSonoba;
						} else {
							player.ramlethalHSwordKowareSonoba = isKowareSonoba;
						}
						*bitInfo.time = result + 1;
						*bitInfo.timeMax = resultMax + 1;
					}
					
				}
				
			} else if (player.charType == CHARACTER_TYPE_ELPHELT) {
				
				bool foundGrenadeInGeneral = false;
				bool foundGrenade = false;
				int grenadeSlowdown = 0;
				int grenadeTimeRemaining = 0;
				
				for (ProjectileInfo& projectile : projectiles) {
					if (projectile.team != player.index || !projectile.ptr) continue;
					if (strcmp(projectile.animName, "GrenadeBomb") == 0) {
						foundGrenadeInGeneral = true;
						if (!projectile.ptr.hasUpon(35)) {
							foundGrenade = true;
							grenadeSlowdown = projectile.rcSlowedDownCounter;
							if (projectile.sprite.frameMax == 1) {
								grenadeTimeRemaining = 30;
							} else {
								grenadeTimeRemaining = 29 - projectile.sprite.frame;
							}
						}
					} else if (strcmp(projectile.animName, "GrenadeBomb_Ready") == 0) {
						foundGrenadeInGeneral = true;
						if (projectile.sprite.frameMax == 1) {
							grenadeTimeRemaining = 31;
							foundGrenade = true;
						} else if (projectile.sprite.frameMax == 30) {
							grenadeTimeRemaining = 30 - projectile.sprite.frame;
							foundGrenade = true;
						}
					}
				}
				
				bool hasForceDisableFlag = (player.wasForceDisableFlags & 0x1) != 0;
				if (!foundGrenade) {
					if (foundGrenadeInGeneral && hasForceDisableFlag) {
						player.elpheltGrenadeRemainingWithSlow = 255;
						player.elpheltGrenadeMaxWithSlow = 255;
					} else if (hasForceDisableFlag) {
						player.elpheltGrenadeRemainingWithSlow = 1;
					} else {
						player.elpheltGrenadeRemainingWithSlow = 0;
					}
					player.elpheltGrenadeElapsed = 0;
				} else {
					int result;
					int resultMax;
					int unused;
					PlayerInfo::calculateSlow(
						player.elpheltGrenadeElapsed + 1,
						grenadeTimeRemaining,
						grenadeSlowdown,
						&result,
						&resultMax,
						&unused);
					
					if (superflashInstigator == nullptr) ++player.elpheltGrenadeElapsed;
					
					if (result || hasForceDisableFlag) ++result;
					++resultMax;
					
					player.elpheltGrenadeRemainingWithSlow = result;
					player.elpheltGrenadeMaxWithSlow = resultMax;
				}
				
				player.elpheltRifle_AimMem46 = player.getElpheltRifle_AimMem46();
				
			}
		}
		
		// This is a separate loop because it depends on another player's timePassedLanding, which I changed in the previous loop
		for (PlayerInfo& player : players) {
			if (player.startedDefending) {
				restartMeasuringFrameAdvantage(player.index);
				restartMeasuringLandingFrameAdvantage(player.index);
			}
		}
		
		if (!superflashInstigator) {
			for (ProjectileInfo& projectile : projectiles) {
				if (!projectile.startedUp) {
					++projectile.startup;
				}
				++projectile.total;
				projectile.strikeInvul = true;
			}
		}
	}
	
	int playerSide = 2;
	if (gifMode.dontHideOpponentsBoxes || gifMode.dontHidePlayersBoxes) {
		playerSide = game.getPlayerSide();
	}
	
	for (int i = 0; i < entityList.count; i++) {
		Entity ent = entityList.list[i];
		int team = ent.team();
		if (!ent.isActive() || isEntityAlreadyDrawn(ent)) continue;

		bool active = ent.isActiveFrames()
			&& (
				i < 2
					&& ent.characterType() == CHARACTER_TYPE_JAM
					&& strcmp(ent.animationName(), "Saishingeki") == 0
					&& ent.currentAnimDuration() > 100
				?
					ent.currentHitNum() > 1
				: true
			);
		logOnce(fprintf(logfile, "drawing entity # %d. active: %d\n", i, (int)active));
		
		size_t hurtboxesPrevSize = drawDataPrepared.hurtboxes.size();
		size_t hitboxesPrevSize = drawDataPrepared.hitboxes.size();
		size_t pointsPrevSize = drawDataPrepared.points.size();
		size_t linesPrevSize = drawDataPrepared.lines.size();
		size_t circlesPrevSize = drawDataPrepared.circles.size();
		size_t pushboxesPrevSize = drawDataPrepared.pushboxes.size();
		size_t interactionBoxesPrevSize = drawDataPrepared.interactionBoxes.size();
		
		int* lastIgnoredHitNum = nullptr;
		if (!iGiveUp && i < 2 && (team == 0 || team == 1)) {
			PlayerInfo& player = players[team];
			lastIgnoredHitNum = &player.lastIgnoredHitNum;
		}
		int hitboxesCount = 0;
		DrawHitboxArrayCallParams hurtbox;
		
		EntityState entityState;
		
		bool useTheseValues = false;
		bool isSuperArmor;
		bool isFullInvul;
		if (i < 2 && (team == 0 || team == 1)) {
			const PlayerInfo& player = players[team];
			isSuperArmor = player.wasSuperArmorEnabled;
			isFullInvul = player.wasFullInvul;
			useTheseValues = true;
		}
		size_t oldHitboxesSize = drawDataPrepared.hitboxes.size();
		
		int scaleX = INT_MAX;
		int scaleY = INT_MAX;
		if (gifMode.dontHideOpponentsBoxes && ent.team() != playerSide
				|| gifMode.dontHidePlayersBoxes && ent.team() == playerSide) {
			auto it = findHiddenEntity(ent);
			if (it != hiddenEntities.end()) {
				scaleX = it->scaleX;
				scaleY = it->scaleY;
			}
		}
		
		collectHitboxes(ent,
			active,
			&hurtbox,
			&drawDataPrepared.hitboxes,
			&drawDataPrepared.points,
			&drawDataPrepared.lines,
			&drawDataPrepared.circles,
			&drawDataPrepared.pushboxes,
			&drawDataPrepared.interactionBoxes,
			&hitboxesCount,
			lastIgnoredHitNum,
			&entityState,
			useTheseValues ? &isSuperArmor : nullptr,
			useTheseValues ? &isFullInvul : nullptr,
			scaleX,
			scaleY);
		
		if (ent.isPawn()
				&& ent.characterType() == CHARACTER_TYPE_SIN
				&& strcmp(ent.animationName(), "UkaseWaza") == 0) {
			int animFrame = ent.currentAnimDuration();
			if (animFrame >= 7 && animFrame < 27) {
				
				PlayerInfo& player = players[ent.team()];
				DrawLineCallParams* newLine;
				DrawPointCallParams* newPoint;
				DrawBoxCallParams* newBox;
				
				if (leftEdgeOfArenaOffset) {
					
					int wallX;
					int wallOff;
					int pnt = player.sinHawkBakerStartX;
					if (player.pawn.isFacingLeft()) {
						wallX = *(int*)(*aswEngine + leftEdgeOfArenaOffset) * 1000;
						wallOff = wallX + 360000;
						if (pnt < wallOff) pnt = wallOff;
						wallX += 20000;
					} else {
						wallX = *(int*)(*aswEngine + rightEdgeOfArenaOffset) * 1000;
						wallOff = wallX - 360000;
						if (pnt > wallOff) pnt = wallOff;
						wallX -= 20000;
					}
					
					int centerOffsetY = ent.getCenterOffsetY();
					int centerY = entityState.posY + centerOffsetY;
					int sinBottomY = entityState.posY - 30000;
					int sinTopY = entityState.posY + centerOffsetY + centerOffsetY + 30000;
					int wallBottomY = centerY - 20000;
					int wallTopY = centerY + 20000;
					
					// Sin vertical line
					drawDataPrepared.lines.emplace_back();
					newLine = &drawDataPrepared.lines.back();
					newLine->posX1 = player.sinHawkBakerStartX;
					newLine->posY1 = sinBottomY;
					newLine->posX2 = player.sinHawkBakerStartX;
					newLine->posY2 = sinTopY;
					
					// Wall vertical line
					drawDataPrepared.lines.emplace_back();
					newLine = &drawDataPrepared.lines.back();
					newLine->posX1 = wallX;
					newLine->posY1 = wallBottomY;
					newLine->posX2 = wallX;
					newLine->posY2 = wallTopY;
					
					// Wall +- 360000 vertical line
					drawDataPrepared.lines.emplace_back();
					newLine = &drawDataPrepared.lines.back();
					newLine->posX1 = wallOff;
					newLine->posY1 = wallBottomY;
					newLine->posX2 = wallOff;
					newLine->posY2 = wallTopY;
					
					// Sin center point
					drawDataPrepared.points.emplace_back();
					newPoint = &drawDataPrepared.points.back();
					newPoint->isProjectile = true;
					newPoint->posX = player.sinHawkBakerStartX;
					newPoint->posY = centerY;
					
					// Wall center point
					drawDataPrepared.points.emplace_back();
					newPoint = &drawDataPrepared.points.back();
					newPoint->isProjectile = true;
					newPoint->posX = wallX;
					newPoint->posY = centerY;
					
					// Line from Wall center to either Sin center or, if he's too close, to wall X +- 360000
					drawDataPrepared.lines.emplace_back();
					newLine = &drawDataPrepared.lines.back();
					newLine->posX1 = wallX;
					newLine->posY1 = centerY;
					newLine->posX2 = pnt;
					newLine->posY2 = centerY;
					
					drawDataPrepared.interactionBoxes.emplace_back();
					newBox = &drawDataPrepared.interactionBoxes.back();
					newBox->bottom = -1000000;
					newBox->top = 10000000;
					newBox->left = player.sinHawkBakerStartX - 200000;
					newBox->right = player.sinHawkBakerStartX + 200000;
					if (animFrame == 7 && !ent.isRCFrozen()) {
						newBox->fillColor = D3DCOLOR_ARGB(64, 255, 255, 255);
					} else {
						newBox->fillColor = 0;
					}
					newBox->outlineColor = D3DCOLOR_ARGB(255, 255, 255, 255);
					newBox->thickness = 1;
					
				}
			}
		}
		
		if (ent.isPawn() && ent.characterType() == CHARACTER_TYPE_ELPHELT
				&& strcmp(ent.animationName(), "Shotgun_Fire_MAX") == 0) {
			PlayerInfo& player = players[ent.team()];
			drawDataPrepared.interactionBoxes.emplace_back();
			DrawBoxCallParams& newBox = drawDataPrepared.interactionBoxes.back();
			newBox.bottom = -1000000;
			newBox.top = 10000000;
			newBox.left = player.elpheltShotgunX - 300000;
			newBox.right = player.elpheltShotgunX + 300000;
			newBox.thickness = 1;
			if (ent.currentAnimDuration() == 1 && !ent.isRCFrozen()) {
				newBox.fillColor = D3DCOLOR_ARGB(64, 255, 255, 255);
			} else {
				newBox.fillColor = 0;
			}
			newBox.outlineColor = D3DCOLOR_ARGB(255, 255, 255, 255);
		}
		
		if (settings.showRamlethalSwordRedeployNoTeleportDistance
				&& !ent.isPawn()
				&& (ent.team() == 0 || ent.team() == 1)) {
			PlayerInfo& player = players[ent.team()];
			if (player.charType == CHARACTER_TYPE_RAMLETHAL) {
				int bitNumber = 0;
				if (player.pawn.stackEntity(0) == ent) bitNumber = 1;
				if (player.pawn.stackEntity(1) == ent) bitNumber = 2;
				const char* animName = ent.animationName();
				bool animMatches = bitNumber == 1
								&& (
									strcmp(animName, "BitN6C") == 0
									|| strcmp(animName, "BitN2C_Bunri") == 0
								)
								|| bitNumber == 2
								&& (
									strcmp(animName, "BitF6D") == 0
									|| strcmp(animName, "BitF2D_Bunri") == 0
								);
				int animDuration = ent.currentAnimDuration();
				if (bitNumber && settings.onlyShowRamlethalSwordRedeployDistanceWhenRedeploying) {
					if (!(animMatches && animDuration < 20)) {
						bitNumber = 0;
					}
				}
				if (bitNumber
						&& (
							!settings.showRamlethalSwordRedeployDistanceForP1
							&& ent.team() == 0
							|| !settings.showRamlethalSwordRedeployDistanceForP2
							&& ent.team() == 1
						)) {
					bitNumber = 0;
				}
				if (bitNumber) {
					DrawBoxCallParams interactionBoxParams;
					int entX = ent.posX();
					int entY = ent.posY();
					if (animMatches && animDuration < 20) {
						if (bitNumber == 1) {
							entX = player.ramlethalBitNStartPos;
						} else {
							entX = player.ramlethalBitFStartPos;
						}
					}
					int checkDistance = bitNumber == 1 ? 500000 : strcmp(animName, "BitF6D") == 0 ? 200000 : 400000;
					interactionBoxParams.left = entX - checkDistance;
					interactionBoxParams.right = entX + checkDistance;
					interactionBoxParams.top = 10000000;
					interactionBoxParams.bottom = -1000000;
					if (animMatches && animDuration == 1 && !ent.isRCFrozen()) {
						interactionBoxParams.fillColor = replaceAlpha(32, COLOR_INTERACTION);
					} else {
						interactionBoxParams.fillColor = replaceAlpha(16, COLOR_INTERACTION);
					}
					interactionBoxParams.outlineColor = replaceAlpha(255, COLOR_INTERACTION);
					interactionBoxParams.thickness = THICKNESS_INTERACTION;
					drawDataPrepared.interactionBoxes.push_back(interactionBoxParams);
				}
			}
		}
		
		HitDetector::WasHitInfo wasHitResult = hitDetector.wasThisHitPreviously(ent, hurtbox);
		if (!wasHitResult.wasHit || settings.neverDisplayGrayHurtboxes) {
			drawDataPrepared.hurtboxes.push_back({ false, hurtbox });
		}
		else {
			drawDataPrepared.hurtboxes.push_back({ true, hurtbox, wasHitResult.hurtbox });
		}
		logOnce(fputs("collectHitboxes(...) call successful\n", logfile));
		drawnEntities.push_back(ent);
		logOnce(fputs("drawnEntities.push_back(...) call successful\n", logfile));

		// Attached entities like dusts
		Entity attached = ent.effectLinkedCollision();
		if (attached != nullptr) {
			DrawHitboxArrayCallParams attachedHurtbox;
			logOnce(fprintf(logfile, "Attached entity: %p\n", attached.ent));
			collectHitboxes(attached,
				active,
				&attachedHurtbox,
				&drawDataPrepared.hitboxes,
				&drawDataPrepared.points,
				&drawDataPrepared.lines,
				&drawDataPrepared.circles,
				&drawDataPrepared.pushboxes,
				&drawDataPrepared.interactionBoxes,
				&hitboxesCount,
				lastIgnoredHitNum,
				nullptr,
				nullptr,
				nullptr,
				scaleX,
				scaleY);
			drawDataPrepared.hurtboxes.push_back({ false, attachedHurtbox });
			drawnEntities.push_back(attached);
		}
		if (!iGiveUp && (team == 0 || team == 1)) {
			if (!hitboxesCount && i < 2) {
				PlayerInfo& player = players[team];
				if (player.hitSomething) ++hitboxesCount;
			}
			RECT hitboxesBoundsTotal;
			if (hitboxesCount && oldHitboxesSize != drawDataPrepared.hitboxes.size()) {
				bool isFirstHitbox = true;
				for (auto it = drawDataPrepared.hitboxes.begin() + oldHitboxesSize;
						it != drawDataPrepared.hitboxes.end();
						++it) {
					if (isFirstHitbox) {
						hitboxesBoundsTotal = it->getWorldBounds();
					} else {
						combineBounds(hitboxesBoundsTotal, it->getWorldBounds());
					}
					isFirstHitbox = false;
				}
			}
			if (i < 2) {
				PlayerInfo& player = players[team];
				player.hitboxesCount += hitboxesCount;
				if (hitboxesCount && oldHitboxesSize != drawDataPrepared.hitboxes.size()) {
					player.hitboxTopY = hitboxesBoundsTotal.top;
					player.hitboxBottomY = hitboxesBoundsTotal.bottom;
					player.hitboxTopBottomValid = true;
				}
				RECT hurtboxBounds;
				if (hurtbox.hitboxCount) {
					hurtboxBounds = hurtbox.getWorldBounds();
					player.hurtboxTopY = hurtboxBounds.top;
					player.hurtboxBottomY = hurtboxBounds.bottom;
					player.hurtboxTopBottomValid = true;
				}
				player.strikeInvul.active = entityState.strikeInvuln;
				player.throwInvul.active = entityState.throwInvuln;
				if (!player.strikeInvul.active
						&& !player.airborne
						&& hurtbox.hitboxCount) {
					if (hurtboxBounds.bottom <= 88000) {
						player.superLowProfile.active = true;
					} else if (hurtboxBounds.bottom <= 159000) {
						player.lowProfile.active = true;
					} else if (hurtboxBounds.bottom < 175000) {
						player.somewhatLowProfile.active = true;
					} else if (hurtboxBounds.bottom <= 232000
							&& !player.pawn.crouching()
							&& player.pawn.blockstun() == 0
							&& !player.inHitstunNowOrNextFrame) {
						player.upperBodyInvul.active = true;
					} else if (hurtboxBounds.bottom <= 274000
							&& !player.pawn.crouching()
							&& player.pawn.blockstun() == 0
							&& !player.inHitstunNowOrNextFrame) {
						player.aboveWaistInvul.active = true;
					}
					if (hurtboxBounds.top >= 174000) {
						player.legInvul.active = true;
					} else if (hurtboxBounds.top >= 84000) {
						player.footInvul.active = true;
					} else if (hurtboxBounds.top >= 20000) {
						player.toeInvul.active = true;
					}
				}
				if (player.airborne && !player.strikeInvul.active && hurtbox.hitboxCount && !player.moveOriginatedInTheAir) {
					if (hurtboxBounds.top < 20000) {
						player.airborneButWontGoOverLows.active = true;
					} else {
						player.airborneInvul.active = true;
					}
				}
				if (player.pawn.damageToAir() && !player.airborne && !player.strikeInvul.active) {
					player.consideredAirborne.active = true;
				}
				player.frontLegInvul.active = !player.strikeInvul.active
					&& player.move.frontLegInvul
					&& player.move.frontLegInvul(player);
				player.superArmor.active = entityState.superArmorActive && !player.projectileOnlyInvul.active && !player.reflect.active;
				if (player.charType == CHARACTER_TYPE_LEO
						&& player.superArmor.active
						&& strcmp(player.anim, "Semuke5E"_hardcode) == 0) {  // Leo bt.D
					for (int projectileSearch = 2; projectileSearch < entityList.count; ++projectileSearch) {
						Entity projectileSearchPtr = entityList.list[projectileSearch];
						if (projectileSearchPtr.isActive()
								&& projectileSearchPtr.team() == player.index
								&& strcmp(projectileSearchPtr.animationName(), "Semuke5E_Reflect"_hardcode) == 0
								&& projectileSearchPtr.superArmorEnabled()) {
							player.reflect.active = true;
						}
					}
				}
				if (player.superArmor.active) {
					player.superArmorThrow.active = ent.superArmorThrow();
					player.superArmorBurst.active = ent.superArmorBurst();
					player.superArmorMid.active = ent.superArmorMid();
					player.superArmorOverhead.active = ent.superArmorOverhead();
					player.superArmorLow.active = ent.superArmorLow();
					player.superArmorGuardImpossible.active = ent.superArmorGuardImpossible();
					player.superArmorObjectAttacck.active = ent.superArmorObjectAttacck();
					player.superArmorHontaiAttacck.active = ent.superArmorHontaiAttacck();
					player.superArmorProjectileLevel0.active = ent.superArmorProjectileLevel0();
					player.superArmorOverdrive.active = ent.superArmorOverdrive();
					player.superArmorBlitzBreak.active = ent.superArmorBlitzBreak();
				}
				player.counterhit = entityState.counterhit;
			} else if (frameHasChanged) {
				ProjectileInfo& projectile = findProjectile(ent);
				if (projectile.ptr) {
					projectile.strikeInvul = entityState.strikeInvuln;
					if (hitboxesCount) {
						projectile.markActive = true;
						if (oldHitboxesSize != drawDataPrepared.hitboxes.size()) {
							projectile.hitboxTopY = hitboxesBoundsTotal.top;
							projectile.hitboxBottomY = hitboxesBoundsTotal.bottom;
							projectile.hitboxTopBottomValid = true;
						}
					}
					projectile.superArmorActive = entityState.superArmorActive;
				}
			}
		}
		if (invisChipp.needToHide(ent)) {
			drawDataPrepared.hurtboxes.resize(hurtboxesPrevSize);
			drawDataPrepared.hitboxes.resize(hitboxesPrevSize);
			drawDataPrepared.points.resize(pointsPrevSize);
			drawDataPrepared.lines.resize(linesPrevSize);
			drawDataPrepared.circles.resize(circlesPrevSize);
			drawDataPrepared.pushboxes.resize(pushboxesPrevSize);
			drawDataPrepared.interactionBoxes.resize(interactionBoxesPrevSize);
		}
	}
	
	logOnce(fputs("got past the entity loop\n", logfile));
	hitDetector.drawHits();
	logOnce(fputs("hitDetector.drawDetected() call successful\n", logfile));
	throws.drawThrows();
	logOnce(fputs("throws.drawThrows() call successful\n", logfile));
	
	if (frameHasChanged && !iGiveUp) {
		
		bool matchTimerZero = game.getMatchTimer() == 0;
		bool atLeastOneDiedThisFrame = false;
		bool atLeastOneDead = false;
		bool atLeastOneNotInHitstop = false;
		bool atLeastOneBusy = false;
		bool atLeastOneDangerousProjectilePresent = false;
		bool atLeastOneDoingGrab = false;
		bool atLeastOneGettingHitBySuper = false;
		bool atLeastOneStartedGettingHitBySuperOnThisFrame = false;
		bool hasDangerousProjectiles[2] { false };
		
		for (ProjectileInfo& projectile : projectiles) {
			if (projectile.team != 0 && projectile.team != 1) {
				continue;
			}
			
			bool needUseHitstop = false;
			bool isDangerous = false;
			
			if (projectile.markActive
					|| projectile.gotHitOnThisFrame
					|| projectile.move.isDangerous
					&& projectile.move.isDangerous(projectile.ptr)) {
				isDangerous = true;
				needUseHitstop = true;
			}
			
			if (isDangerous) {
				hasDangerousProjectiles[projectile.team] = true;
				atLeastOneDangerousProjectilePresent = true;
			}
			
			if (needUseHitstop && !projectile.hitstop) {
				atLeastOneNotInHitstop = true;
			}
		}
		
		bool playerCantRc[2] { false };
		
		for (int i = 0; i < 2; ++i) {
			PlayerInfo& player = players[i];
			if (player.pawn.performingThrow()) {
				playerCantRc[i] = true;
			} else if (player.pawn.romanCancelAvailability() == ROMAN_CANCEL_DISALLOWED) {
				playerCantRc[i] = true;
			}
		}
		
		for (int i = 0; i < 2; ++i) {
			PlayerInfo& player = players[i];
			atLeastOneGettingHitBySuper = atLeastOneGettingHitBySuper
				|| playerCantRc[1 - i]
				&& player.gettingHitBySuper
				&& !hasDangerousProjectiles[i];
			atLeastOneStartedGettingHitBySuperOnThisFrame = atLeastOneStartedGettingHitBySuperOnThisFrame
				|| player.gettingHitBySuper
				&& !player.prevGettingHitBySuper;
			atLeastOneDiedThisFrame = atLeastOneDiedThisFrame || player.hp == 0 && player.prevHp != 0;
			atLeastOneDead = atLeastOneDead || player.hp == 0;
			if (
					(
						player.grab
						|| player.move.isGrab && !(player.move.forceLandingRecovery && player.landingRecovery)
					)
					&& !(
						!player.wasCancels.gatlings.empty()
						|| !player.wasCancels.whiffCancels.empty()
					)
					&& playerCantRc[i]
			) {
				if (player.pawn.dealtAttack()->type == ATTACK_TYPE_OVERDRIVE) {
					atLeastOneGettingHitBySuper = true;
				} else {
					atLeastOneDoingGrab = true;
				}
			}
			if (!player.hitstop
					|| player.charType == CHARACTER_TYPE_BAIKEN
					&& (
						!settings.ignoreHitstopForBlockingBaiken && player.blockstun > 0
						|| player.move.ignoresHitstop
					)) {
				atLeastOneNotInHitstop = true;
			}
			if (
					(
						(
							!(
								!settings.considerKnockdownWakeupAndAirtechIdle
								&& (
									player.idleLanding
									|| player.idle
									&& player.move.canBeUnableToBlockIndefinitelyOrForVeryLongTime  // Chipp Wall Climb
								)
								|| settings.considerKnockdownWakeupAndAirtechIdle
								&& (player.airteched ? player.idlePlus : player.idleLanding)
							)
							|| (!player.canBlock || !player.canFaultlessDefense)
							&& !player.ignoreNextInabilityToBlockOrAttack
							&& !player.move.canBeUnableToBlockIndefinitelyOrForVeryLongTime
						) && !(
							!settings.considerRunAndWalkNonIdle
							&& player.charType == CHARACTER_TYPE_LEO
							&& player.cmnActIndex == CmnActFDash
						)
						|| (
							(
								player.strikeInvul.active
								|| player.throwInvul.active
							) && (
								!settings.considerKnockdownWakeupAndAirtechIdle
								|| !player.wokeUp
								&& (player.airteched ? player.idlePlus : player.idleLanding)
							)
							|| player.projectileOnlyInvul.active
							|| player.superArmor.active
							|| player.reflect.active
						)
						&& !settings.considerIdleInvulIdle
						|| settings.considerRunAndWalkNonIdle
						&& (
							player.cmnActIndex == CmnActFWalk
							|| player.cmnActIndex == CmnActBWalk
							|| player.cmnActIndex == CmnActFDash
							|| player.cmnActIndex == CmnActFDashStop
							|| player.charType == CHARACTER_TYPE_LEO
							&& strcmp(player.anim, "Semuke"_hardcode) == 0
							&& (
								strcmp(player.pawn.gotoLabelRequest(), "SemukeFrontWalk"_hardcode) == 0
								|| strcmp(player.pawn.gotoLabelRequest(), "SemukeBackWalk"_hardcode) == 0
								|| player.pawn.speedX() != 0
							)
							|| (
								player.charType == CHARACTER_TYPE_FAUST
								|| player.charType == CHARACTER_TYPE_BEDMAN
							) && (
								strcmp(player.anim, "CrouchFWalk"_hardcode) == 0
								|| strcmp(player.anim, "CrouchBWalk"_hardcode) == 0
							)
							|| player.charType == CHARACTER_TYPE_HAEHYUN
							&& strcmp(player.anim, "CrouchFDash"_hardcode) == 0
						)
						|| settings.considerCrouchNonIdle
						&& misterPlayerIsManuallyCrouching(player)
						|| player.forceBusy
						|| settings.considerDummyPlaybackNonIdle
						&& game.isTrainingMode() && game.getDummyRecordingMode() == DUMMY_MODE_PLAYING_BACK
						|| player.charType == CHARACTER_TYPE_ELPHELT
						&& (
							player.playerval0
							&& (
								!player.playerval1
								|| player.playerval1 && !player.prevFramePlayerval1
							)
							&& (
								player.cmnActIndex == CmnActStand
								|| player.cmnActIndex == CmnActCrouch2Stand
							)
							|| player.move.canBeUnableToBlockIndefinitelyOrForVeryLongTime  // rifle stance
							&& (
								!player.elpheltRifle_AimMem46
								|| player.elpheltRifle_AimMem46
								&& !player.prevFrameElpheltRifle_AimMem46
							)
						)
					)
					&& (
						!settings.considerKnockdownWakeupAndAirtechIdle
						|| !(
							(
								player.cmnActIndex == CmnActBDownBound
								|| player.cmnActIndex == CmnActFDownBound
								|| player.cmnActIndex == CmnActVDownBound
							)
							&& player.pawn.isOtg()
							|| (
								player.cmnActIndex == CmnActBDownLoop
								|| player.cmnActIndex == CmnActFDownLoop
								|| player.cmnActIndex == CmnActVDownLoop
							)
							|| player.wakeupTiming  // CmnActFDown2Stand, CmnActBDown2Stand, CmnActWallHaritsukiGetUp, CmnActUkemi
						)
					)) {
				atLeastOneBusy = true;
			}
		}
		
		SkippedFramesType skippedType = SKIPPED_FRAMES_HITSTOP;
		SkippedFramesType skippedGrabType = SKIPPED_FRAMES_GRAB;
		if (atLeastOneGettingHitBySuper && !atLeastOneStartedGettingHitBySuperOnThisFrame) {
			atLeastOneDoingGrab = true;
			skippedGrabType = SKIPPED_FRAMES_SUPER;
		}
		if (atLeastOneDoingGrab && settings.skipGrabsInFramebar) {
			skippedType = skippedGrabType;
			atLeastOneBusy = false;
			atLeastOneNotInHitstop = false;
			atLeastOneDangerousProjectilePresent = false;
		}
		
		if ((atLeastOneDead || matchTimerZero) && !atLeastOneDiedThisFrame) {
			// do nothing
		} else if (getSuperflashInstigator() == nullptr) {
			if (atLeastOneBusy || atLeastOneDangerousProjectilePresent) {
				if (framebarIdleHitstopFor > framebarIdleForLimit) {
					
					memset(skippedFramesHitstop.data(), 0, sizeof SkippedFramesInfo * _countof(Framebar::frames));
					memset(skippedFramesIdleHitstop.data(), 0, sizeof SkippedFramesInfo * _countof(Framebar::frames));
					
					nextSkippedFramesHitstop.clear();
					nextSkippedFramesIdleHitstop.clear();
					
					framebarPositionHitstop = 0;
					for (int totalFramebarIndex = 0; totalFramebarIndex < totalFramebarCount(); ++totalFramebarIndex) {
						EntityFramebar& entityFramebar = getFramebar(totalFramebarIndex);
						entityFramebar.getHitstop().clear();
						entityFramebar.getIdleHitstop().clear();
						if (entityFramebar.belongsToPlayer()) {
							PlayerFramebars& framebarCast = (PlayerFramebars&)entityFramebar;
							framebarCast.hitstop.clearCancels();
							framebarCast.idleHitstop.clearCancels();
						}
					}
				} else {
					if (framebarIdleHitstopFor) {
						
						int ind = EntityFramebar::posPlusOne(framebarPositionHitstop);
						for (int i = 1; i <= framebarIdleHitstopFor; ++i) {
							skippedFramesHitstop[ind] = skippedFramesIdleHitstop[ind];
							EntityFramebar::incrementPos(ind);
						}
						nextSkippedFramesHitstop = nextSkippedFramesIdleHitstop;
						
						for (int totalFramebarIndex = 0; totalFramebarIndex < totalFramebarCount(); ++totalFramebarIndex) {
							EntityFramebar& entityFramebar = getFramebar(totalFramebarIndex);
							bool isPlayer = entityFramebar.belongsToPlayer();
							entityFramebar.getHitstop().catchUpToIdle(entityFramebar.getIdleHitstop(), framebarPositionHitstop, framebarIdleHitstopFor);
						}
						framebarPositionHitstop += framebarIdleHitstopFor;
						framebarPositionHitstop %= _countof(Framebar::frames);
					}
					EntityFramebar::incrementPos(framebarPositionHitstop);
					
					skippedFramesHitstop[framebarPositionHitstop] = nextSkippedFramesHitstop;
					nextSkippedFramesHitstop.clear();
					skippedFramesIdleHitstop[framebarPositionHitstop] = nextSkippedFramesIdleHitstop;
					nextSkippedFramesIdleHitstop.clear();
					
					for (int totalFramebarIndex = 0; totalFramebarIndex < totalFramebarCount(); ++totalFramebarIndex) {
						EntityFramebar& entityFramebar = getFramebar(totalFramebarIndex);
						entityFramebar.getHitstop().soakUpIntoPreFrame(entityFramebar.getHitstop().getFrame(framebarPositionHitstop));
						entityFramebar.getIdleHitstop().soakUpIntoPreFrame(entityFramebar.getIdleHitstop().getFrame(framebarPositionHitstop));
						if (entityFramebar.belongsToPlayer()) {
							PlayerFramebars& framebarCast = (PlayerFramebars&)entityFramebar;
							framebarCast.hitstop.clearCancels(framebarPositionHitstop);
							framebarCast.idleHitstop.clearCancels(framebarPositionHitstop);
						}
					}
					
				}
				framebarIdleHitstopFor = 0;
				framebarAdvancedHitstop = true;
				framebarAdvancedIdleHitstop = true;
				
				if (atLeastOneNotInHitstop) {
					if (framebarIdleFor > framebarIdleForLimit) {
						
						memset(skippedFrames.data(), 0, sizeof SkippedFramesInfo * _countof(Framebar::frames));
						memset(skippedFramesIdle.data(), 0, sizeof SkippedFramesInfo * _countof(Framebar::frames));
						
						nextSkippedFrames.clear();
						nextSkippedFramesIdle.clear();
						
						framebarPosition = 0;
						for (int totalFramebarIndex = 0; totalFramebarIndex < totalFramebarCount(); ++totalFramebarIndex) {
							EntityFramebar& entityFramebar = getFramebar(totalFramebarIndex);
							entityFramebar.getMain().clear();
							entityFramebar.getIdle().clear();
							if (entityFramebar.belongsToPlayer()) {
								PlayerFramebars& framebarCast = (PlayerFramebars&)entityFramebar;
								framebarCast.main.clearCancels();
								framebarCast.idle.clearCancels();
							}
						}
					} else {
						if (framebarIdleFor) {
							
							int ind = EntityFramebar::posPlusOne(framebarPosition);
							for (int i = 1; i <= framebarIdleFor; ++i) {
								skippedFrames[ind] = skippedFramesIdle[ind];
								EntityFramebar::incrementPos(ind);
							}
							nextSkippedFrames = nextSkippedFramesIdle;
							
							for (int totalFramebarIndex = 0; totalFramebarIndex < totalFramebarCount(); ++totalFramebarIndex) {
								EntityFramebar& entityFramebar = getFramebar(totalFramebarIndex);
								bool isPlayer = entityFramebar.belongsToPlayer();
								entityFramebar.getMain().catchUpToIdle(entityFramebar.getIdle(), framebarPosition, framebarIdleFor);
							}
							framebarPosition += framebarIdleFor;
							framebarPosition %= _countof(Framebar::frames);
						}
						EntityFramebar::incrementPos(framebarPosition);
						
						skippedFrames[framebarPosition] = nextSkippedFrames;
						nextSkippedFrames.clear();
						skippedFramesIdle[framebarPosition] = nextSkippedFramesIdle;
						nextSkippedFramesIdle.clear();
						
						for (int totalFramebarIndex = 0; totalFramebarIndex < totalFramebarCount(); ++totalFramebarIndex) {
							EntityFramebar& entityFramebar = getFramebar(totalFramebarIndex);
							FrameBase& mainFrame = entityFramebar.getMain().getFrame(framebarPosition);
							FrameBase& idleFrame = entityFramebar.getMain().getFrame(framebarPosition);
							entityFramebar.getMain().soakUpIntoPreFrame(mainFrame);
							entityFramebar.getIdle().soakUpIntoPreFrame(idleFrame);
							if (entityFramebar.belongsToPlayer()) {
								PlayerFramebars& framebarCast = (PlayerFramebars&)entityFramebar;
								framebarCast.main.clearCancels(framebarPosition);
								framebarCast.idle.clearCancels(framebarPosition);
							}
						}
					}
					framebarIdleFor = 0;
					framebarAdvanced = true;
					framebarAdvancedIdle = true;
				} else {  // if not  atLeastOneNotInHitstop
					nextSkippedFrames.addFrame(SKIPPED_FRAMES_HITSTOP);
					nextSkippedFramesIdle.addFrame(SKIPPED_FRAMES_HITSTOP);
				}
			} else {  // if not atLeastOneBusy || atLeastOneDangerousProjectilePresent
				framebarAdvancedIdleHitstop = true;
				++framebarIdleHitstopFor;
				int confinedPos = EntityFramebar::confinePos(framebarPositionHitstop + framebarIdleHitstopFor);
				skippedFramesIdleHitstop[confinedPos] = nextSkippedFramesIdleHitstop;
				nextSkippedFramesIdleHitstop.clear();
				for (int totalFramebarIndex = 0; totalFramebarIndex < totalFramebarCount(); ++totalFramebarIndex) {
					EntityFramebar& entityFramebar = getFramebar(totalFramebarIndex);
					entityFramebar.getIdleHitstop().soakUpIntoPreFrame(entityFramebar.getIdleHitstop().getFrame(confinedPos));
					if (entityFramebar.belongsToPlayer()) {
						PlayerFramebars& framebarCast = (PlayerFramebars&)entityFramebar;
						framebarCast.idleHitstop.clearCancels(confinedPos);
					}
				}
				if (atLeastOneNotInHitstop) {
					framebarAdvancedIdle = true;
					++framebarIdleFor;
					confinedPos = EntityFramebar::confinePos(framebarPosition + framebarIdleFor);
					skippedFramesIdle[confinedPos] = nextSkippedFramesIdle;
					nextSkippedFramesIdle.clear();
					for (int totalFramebarIndex = 0; totalFramebarIndex < totalFramebarCount(); ++totalFramebarIndex) {
						EntityFramebar& entityFramebar = getFramebar(totalFramebarIndex);
						FrameBase& frame = entityFramebar.getIdle().getFrame(confinedPos);
						entityFramebar.getIdle().soakUpIntoPreFrame(frame);
						if (entityFramebar.belongsToPlayer()) {
							PlayerFramebars& framebarCast = (PlayerFramebars&)entityFramebar;
							framebarCast.idle.clearCancels(confinedPos);
						}
					}
				} else {  // if not atLeastOneNotInHitstop
					nextSkippedFrames.addFrame(skippedType);
					nextSkippedFramesIdle.addFrame(skippedType);
				}
			}
		} else {  // if not getSuperflashInstigator() == nullptr
			nextSkippedFrames.addFrame(SKIPPED_FRAMES_SUPERFREEZE);
			nextSkippedFramesIdle.addFrame(SKIPPED_FRAMES_SUPERFREEZE);
			nextSkippedFramesHitstop.addFrame(SKIPPED_FRAMES_SUPERFREEZE);
			nextSkippedFramesIdleHitstop.addFrame(SKIPPED_FRAMES_SUPERFREEZE);
		}
		
		Entity superflashInstigator = getSuperflashInstigator();
		
		if (!superflashInstigator) {
			for (PlayerInfo& player : players) {
				if (!player.hitstop) {  // needed for Axl DP
					player.activesProj.beginMergeFrame();
				}
			}
		}
		
		int instigatorTeam = -1;
		if (superflashInstigator) {
			instigatorTeam = superflashInstigator.team();
			
			int newValue;
			
			newValue = getSuperflashCounterAllied();
			if (newValue > superflashCounterAllied) {
				superflashCounterAlliedMax = newValue;
			}
			superflashCounterAllied = newValue;
			
			newValue = getSuperflashCounterOpponent();
			if (newValue > superflashCounterOpponent) {
				superflashCounterOpponentMax = newValue;
			}
			superflashCounterOpponent = newValue;
			
		} else {
			superflashCounterAllied = 0;
			superflashCounterOpponent = 0;
		}
		
		for (int totalFramebarIndex = 0; totalFramebarIndex < totalFramebarCount(); ++totalFramebarIndex) {
			EntityFramebar& entityFramebar = getFramebar(totalFramebarIndex);
			entityFramebar.foundOnThisFrame = false;
		}
		const int framebarPos = (framebarPositionHitstop + framebarIdleHitstopFor) % _countof(Framebar::frames);
		int framebarHitstopSearchPos = framebarPositionHitstop;
		if (framebarAdvancedHitstop) EntityFramebar::decrementPos(framebarHitstopSearchPos);
		int framebarIdleSearchPos = (framebarPosition + framebarIdleFor) % _countof(Framebar::frames);
		if (framebarAdvancedIdle) EntityFramebar::decrementPos(framebarIdleSearchPos);
		int framebarSearchPos = framebarPosition;
		if (framebarAdvanced) EntityFramebar::decrementPos(framebarSearchPos);
			
		for (ProjectileInfo& projectile : projectiles) {
			bool ignoreThisForPlayer = false;
			if (projectile.team == 0 || projectile.team == 1) {
				PlayerInfo& player = players[projectile.team];
				ignoreThisForPlayer = 
					player.charType == CHARACTER_TYPE_JACKO
					&& (
						strcmp(projectile.animName, "ServantA"_hardcode) == 0
						|| strcmp(projectile.animName, "ServantB"_hardcode) == 0
						|| strcmp(projectile.animName, "ServantC"_hardcode) == 0
						|| strcmp(projectile.animName, "magicAtkLv1"_hardcode) == 0
						|| strcmp(projectile.animName, "magicAtkLv2"_hardcode) == 0
						|| strcmp(projectile.animName, "magicAtkLv3"_hardcode) == 0
					);
			}
			
			PlayerInfo& player = players[projectile.team == 0 || projectile.team == 1 ? projectile.team : 0];
			if (!projectile.disabled && (projectile.team == 0 || projectile.team == 1)
					&& !projectile.prevStartups.empty()
					&& !ignoreThisForPlayer) {
				
				player.prevStartupsProj = projectile.prevStartups;
				for (int i = 0; i < player.prevStartupsProj.count; ++i) {
					player.prevStartupsProj[i].moveName = nullptr;
					player.prevStartupsProj[i].moveSlangName = nullptr;
				}
			}
			
			bool projectileCanBeHit = false;
			if ((
					projectile.isRamlethalSword
					|| player.charType == CHARACTER_TYPE_ELPHELT
					&& strcmp(projectile.animName, "GrenadeBomb") == 0
				) && !projectile.strikeInvul
				|| projectile.gotHitOnThisFrame) {
				projectileCanBeHit = true;
			}
			ProjectileFramebar& entityFramebar = findProjectileFramebar(projectile,
				projectile.markActive
				|| projectile.gotHitOnThisFrame
				|| projectileCanBeHit);
			entityFramebar.foundOnThisFrame = true;
			Framebar& framebar = entityFramebar.idleHitstop;
			Frame& currentFrame = framebar[framebarPos];
			
			FrameType defaultIdleFrame = projectileCanBeHit ? FT_IDLE_PROJECTILE_HITTABLE : FT_IDLE_PROJECTILE;
			
			if (framebarAdvancedIdleHitstop) {
				currentFrame.type = defaultIdleFrame;
				projectile.determineMoveNameAndSlangName(&currentFrame.animName, &currentFrame.animSlangName);
				currentFrame.hitstop = projectile.hitstop;
				currentFrame.hitstopMax = projectile.hitstopMax;
				currentFrame.hitConnected = projectile.hitConnectedForFramebar() || projectile.gotHitOnThisFrame;
				currentFrame.rcSlowdown = projectile.rcSlowedDownCounter;
				currentFrame.rcSlowdownMax = projectile.rcSlowedDownMax;
				currentFrame.activeDuringSuperfreeze = false;
				currentFrame.powerup = projectile.move.projectilePowerup && projectile.move.projectilePowerup(projectile);
			} else if (superflashInstigator && projectile.gotHitOnThisFrame) {
				currentFrame.hitConnected = true;
				if (currentFrame.type == FT_NONE) {
					currentFrame.type = defaultIdleFrame;
				}
			}
			
			if (!projectile.markActive) {
				if (!superflashInstigator) {
					projectile.actives.addNonActive();
					if (framebarAdvancedIdleHitstop) {
						currentFrame.type = defaultIdleFrame;
					}
				}
			} else if (!superflashInstigator || !projectile.startedUp) {
				if (!projectile.startedUp) {
					if (!projectile.disabled && (projectile.team == 0 || projectile.team == 1) && !ignoreThisForPlayer) {
						if (!player.startupProj) {
							player.startupProj = projectile.startup;
						}
					}
					projectile.startedUp = true;
				}
				if (projectile.actives.count) {
					int lastNonActives = projectile.actives.data[projectile.actives.count - 1].nonActives;
					if (lastNonActives) {
						entityFramebar.changePreviousFramesOneType(defaultIdleFrame,
							FT_NON_ACTIVE_PROJECTILE,
							framebarPos - 1,
							framebarHitstopSearchPos,
							framebarIdleSearchPos,
							framebarSearchPos,
							lastNonActives);
					}
				}
				if (projectile.actives.count
						&& !projectile.actives.data[projectile.actives.count - 1].nonActives
						&& projectile.hitNumber != projectile.actives.prevHitNum
						&& !(projectile.ptr ? projectile.ptr.isRCFrozen() : false)) {
					framebar.requestNextHit = true;
				}
				projectile.actives.addActive(projectile.hitNumber);
				if (!projectile.hitOnFrame && projectile.hitConnectedForFramebar()) {
					projectile.hitOnFrame = projectile.actives.total();
					if (!projectile.disabled && (projectile.team == 0 || projectile.team == 1) && !ignoreThisForPlayer) {
						if (!player.hitOnFrameProj) {
							player.hitOnFrameProj = projectile.hitOnFrame;
						}
					}
				}
				if (framebarAdvancedIdleHitstop) {
					currentFrame.type = projectile.hitstop ? FT_ACTIVE_HITSTOP_PROJECTILE : FT_ACTIVE_PROJECTILE;
				}
				if (superflashInstigator && !framebarAdvancedIdleHitstop) {
					currentFrame.activeDuringSuperfreeze = true;
					if (projectile.hitConnectedForFramebar()) {
						currentFrame.hitConnected = true;
					}
					if (currentFrame.type == defaultIdleFrame || currentFrame.type == FT_NONE) {
						currentFrame.type = FT_IDLE_ACTIVE_IN_SUPERFREEZE;
					}
				}
				if (!projectile.disabled && (projectile.team == 0 || projectile.team == 1) && !ignoreThisForPlayer) {
					if (!player.hitstop) {
						if (superflashInstigator) {
							player.activesProj.addSuperfreezeActive(projectile.hitNumber);
						} else {
							player.activesProj.addActive(projectile.hitNumber);
						}
						if (player.maxHitProj.empty()) {
							if (!player.maxHitProjConflict) {
								player.maxHitProj = projectile.maxHit;
								player.maxHitProjLastPtr = projectile.ptr;
							}
						} else if (!projectile.maxHit.empty()) {
							if (projectile.ptr == player.maxHitProjLastPtr) {
								if (projectile.ptr) {
									player.maxHitProj = projectile.maxHit;
								} else if (player.maxHitProj != projectile.maxHit) {
									player.maxHitProjConflict = true;
									player.maxHitProj.clear();
								}
							} else if (!projectile.ptr ? player.maxHitProj != projectile.maxHit : true) {
								player.maxHitProjConflict = true;
								player.maxHitProj.clear();
							}
						}
					}
				}
			}
			
			if (framebarAdvancedIdleHitstop) {
				currentFrame.newHit = framebar.requestNextHit;
			}
			
			copyIdleHitstopFrameToTheRestOfSubframebars(entityFramebar,
				framebarAdvanced,
				framebarAdvancedIdle,
				framebarAdvancedHitstop,
				framebarAdvancedIdleHitstop);
		}
		if (framebarAdvancedIdleHitstop) {
			for (ProjectileFramebar& entityFramebar : projectileFramebars) {
				if(entityFramebar.foundOnThisFrame) continue;
				
				Framebar& framebar = entityFramebar.idleHitstop;
				Frame& currentFrame = framebar[framebarPos];
				
				currentFrame.type = FT_IDLE_PROJECTILE;
				currentFrame.activeDuringSuperfreeze = false;
				currentFrame.animName = nullptr;
				currentFrame.animSlangName = nullptr;
				currentFrame.hitstop = 0;
				currentFrame.hitstopMax = 0;
				currentFrame.hitConnected = false;
				currentFrame.rcSlowdown = 0;
				currentFrame.rcSlowdownMax = 0;
				currentFrame.activeDuringSuperfreeze = false;
				currentFrame.powerup = false;
				
				copyIdleHitstopFrameToTheRestOfSubframebars(entityFramebar,
					framebarAdvanced,
					framebarAdvancedIdle,
					framebarAdvancedHitstop,
					framebarAdvancedIdleHitstop);
			}
		}
		
		if (!superflashInstigator) {
			for (PlayerInfo& player : players) {
				if (!player.hitstop) {
					player.activesProj.endMergeFrame();
				}
				
				if (player.charType != CHARACTER_TYPE_ZATO) continue;
				
				int resource = player.pawn.exGaugeValue(0);
				int prevResource = player.eddie.prevResource;
				player.eddie.prevResource = resource;
				PlayerInfo& enemy = players[1 - player.index];
				Entity ent = nullptr;
				if (player.pawn.playerVal(0)) {
					ent = getReferredEntity((void*)player.pawn.ent, 4);
				}
				bool created = false;
				
				Entity landminePtr = nullptr;
				for (ProjectileInfo& projectile : projectiles) {
					if (strcmp(projectile.animName, "ChouDoriru"_hardcode) == 0
							&& projectile.ptr
							&& projectile.team == player.index) {
						landminePtr = projectile.ptr;
						if (!player.eddie.landminePtr) {
							projectile.creationTime_aswEngineTick = player.eddie.moveStartTime_aswEngineTick;
							projectile.startup = player.eddie.total;
							memset(projectile.creatorName, 0, 32);
							strcpy(projectile.creatorName, "Eddie"_hardcode);
							projectile.creator = player.eddie.ptr;
							projectile.total = player.eddie.total;
						}
						break;
					}
				}
				player.eddie.landminePtr = landminePtr;
				
				if (player.eddie.ptr != ent) {
					player.eddie.ptr = ent;
					created = !player.eddie.landminePtr;
				}
				if (!player.eddie.ptr && !landminePtr) continue;
				
				int diff = prevResource - resource;
				if (diff > 10) {  // 10 is the amount you lose each frame
					player.eddie.consumedResource = diff;
				}
				
				bool idleNext = !player.eddie.ptr ? false : !player.pawn.playerVal(2);
				ProjectileInfo& projectile = !player.eddie.ptr ? findProjectile(landminePtr) : findProjectile(player.eddie.ptr);
				if (created || strcmp(player.eddie.anim, projectile.animName) != 0 || !idleNext && projectile.animFrame < player.eddie.prevAnimFrame) {
					memcpy(player.eddie.anim, projectile.animName, 32);
					
					if (created || !idleNext && player.eddie.ptr) {
						player.eddie.total = 0;
						player.eddie.hitOnFrame = 0;
						player.eddie.moveStartTime_aswEngineTick = aswEngineTickCount;
						player.eddie.startup = 0;
						player.eddie.startedUp = false;
						player.eddie.actives.clear();
						player.eddie.maxHit.clear();
						player.eddie.recovery = 0;
						player.eddie.frameAdvantageValid = false;
						player.eddie.landingFrameAdvantageValid = false;
						player.eddie.frameAdvantageIncludesIdlenessInNewSection = false;
						player.eddie.landingFrameAdvantageIncludesIdlenessInNewSection = false;
					}
					
					if (created) {
						player.eddie.hitstopMax = 0;
						player.eddie.moveStartTime_aswEngineTick = player.moveStartTime_aswEngineTick;
						player.eddie.startup = player.total;
						player.eddie.total = player.total;
						player.eddie.prevEnemyIdle = enemy.idlePlus;
						player.eddie.prevEnemyIdleLanding = enemy.idleLanding;
					}
				}
				player.eddie.prevAnimFrame = projectile.animFrame;
				
				if (created || player.eddie.idle != idleNext) {
					player.eddie.idle = idleNext;
					player.eddie.timePassed = 0;
					if (idleNext && enemy.idlePlus) {
						player.eddie.frameAdvantage = player.eddie.timePassed - enemy.timePassed;
						if (enemy.timePassed < 999) {
							player.eddie.frameAdvantageValid = true;
						}
					}
					if (idleNext && enemy.idleLanding) {
						player.eddie.landingFrameAdvantage = player.eddie.timePassed - enemy.timePassedLanding;
						if (enemy.timePassedLanding < 999) {
							player.eddie.landingFrameAdvantageValid = true;
						}
					}
				}
				if (enemy.idlePlus != player.eddie.prevEnemyIdle) {
					player.eddie.prevEnemyIdle = enemy.idlePlus;
					if (enemy.idlePlus && player.eddie.idle) {
						player.eddie.frameAdvantage = player.eddie.timePassed - enemy.timePassed;
						player.eddie.frameAdvantageValid = true;
					}
				}
				if (enemy.idleLanding != player.eddie.prevEnemyIdleLanding) {
					player.eddie.prevEnemyIdleLanding = enemy.idleLanding;
					if (enemy.idleLanding && player.eddie.idle) {
						player.eddie.landingFrameAdvantage = player.eddie.timePassed - enemy.timePassedLanding;
						player.eddie.landingFrameAdvantageValid = true;
					}
				}
				
				int hitstop = player.eddie.hitstop;
				player.eddie.hitstop = projectile.hitstop;
				if (!hitstop && projectile.hitstop) {
					player.eddie.hitstopMax = projectile.hitstop;
				}
				
				if (!projectile.hitstop && !player.eddie.idle) {
					if (!player.eddie.startedUp) {
						++player.eddie.startup;
					}
					if (!projectile.markActive) {
						if (player.eddie.startedUp) {
							++player.eddie.recovery;
						}
					} else {
						player.eddie.startedUp = true;
						if (player.eddie.recovery) {
							player.eddie.actives.addNonActive(player.eddie.recovery);
							player.eddie.recovery = 0;
						}
						if (!player.eddie.hitOnFrame && projectile.hitConnectedForFramebar()) {
							player.eddie.hitOnFrame = player.eddie.total - player.eddie.startup + 1;
						}
						player.eddie.maxHit = projectile.maxHit;
						player.eddie.actives.addActive(projectile.hitNumber, 1);
					}
					++player.eddie.total;
				}
				
				++player.eddie.timePassed;
				
				ProjectileFramebar& entityFramebar = findProjectileFramebar(projectile, true);
				Framebar& framebar = entityFramebar.idleHitstop;
				Frame& currentFrame = framebar[framebarPos];
				if (framebarAdvancedIdleHitstop) {
					FrameType oldType = currentFrame.type;
					if (idleNext) {
						currentFrame.type = FT_EDDIE_IDLE;
					} else if (!player.eddie.actives.count) {
						currentFrame.type = FT_EDDIE_STARTUP;
					} else if (!player.eddie.recovery) {
						currentFrame.type = player.eddie.hitstop ? FT_EDDIE_ACTIVE_HITSTOP : FT_EDDIE_ACTIVE;
					} else {
						currentFrame.type = FT_EDDIE_RECOVERY;
					}
					if (oldType != currentFrame.type) {
						if (oldType == FT_NONE) {
							// the framebar was created by us, the Eddie-specific code
							copyIdleHitstopFrameToTheRestOfSubframebars(entityFramebar,
								framebarAdvanced,
								framebarAdvancedIdle,
								framebarAdvancedHitstop,
								framebarAdvancedIdleHitstop);
						} else {
							// the framebar was created by projectile-handling code, and all framebars are already caught up.
							// copyIdleHitstopFrameToTheRestOfSubframebars function should not be called more than once per frame
							entityFramebar.hitstop.modifyFrame(framebarPositionHitstop, currentFrame.aswEngineTick, currentFrame.type);
							entityFramebar.idle.modifyFrame(
								EntityFramebar::confinePos(framebarPosition + framebarIdleFor),
								currentFrame.aswEngineTick, currentFrame.type);
							entityFramebar.main.modifyFrame(framebarPosition, currentFrame.aswEngineTick, currentFrame.type);
						}
					}
				}
				
			}
		}
		for (int i = 0; i < 2; ++i) {
			PlayerInfo& player = players[i];
			PlayerInfo& other = players[1 - i];
			
			if (!superflashInstigator) {
				++player.timePassed;
				++player.timePassedLanding;
				if (player.inNewMoveSection) {
					++player.timeInNewSection;
				}
				if (player.timeSinceLastGap == 0) {
					if (other.idle || !other.onTheDefensive) {
						player.timeSinceLastGap = 1;
					}
				} else {
					++player.timeSinceLastGap;
				}
				
				if (player.idlePlus && !other.idlePlus) {
					if (measuringFrameAdvantage) {
						++player.frameAdvantage;
						++player.frameAdvantageNoPreBlockstun;
					}
				} else if (!player.idlePlus && other.idlePlus) {
					if (measuringFrameAdvantage) {
						--player.frameAdvantage;
						--player.frameAdvantageNoPreBlockstun;
					}
				} else if (!player.idlePlus && !other.idlePlus) {
					if (measuringLandingFrameAdvantage == -1) {
						player.landingFrameAdvantageValid = false;
						other.landingFrameAdvantageValid = false;
					}
					player.frameAdvantage = 0;
					other.frameAdvantage = 0;
					player.frameAdvantageNoPreBlockstun = 0;
					other.frameAdvantageNoPreBlockstun = 0;
					measuringFrameAdvantage = true;
					player.frameAdvantageValid = false;
					other.frameAdvantageValid = false;
					player.frameAdvantageIncludesIdlenessInNewSection = false;
					other.frameAdvantageIncludesIdlenessInNewSection = false;
				} else if (player.idlePlus && other.idlePlus) {
					if (measuringFrameAdvantage) {
						measuringFrameAdvantage = false;
						player.frameAdvantageValid = true;
						other.frameAdvantageValid = true;
					}
				}
				
				if (i == measuringLandingFrameAdvantage) {
					if (player.idleLanding && !other.idleLanding) {
						++player.landingFrameAdvantage;
						--other.landingFrameAdvantage;
						++player.landingFrameAdvantageNoPreBlockstun;
						--other.landingFrameAdvantageNoPreBlockstun;
					} else if (!player.idleLanding && other.idleLanding) {
						--player.landingFrameAdvantage;
						++other.landingFrameAdvantage;
						--player.landingFrameAdvantageNoPreBlockstun;
						++other.landingFrameAdvantageNoPreBlockstun;
					} else if (!player.idleLanding && !other.idleLanding) {
						player.landingFrameAdvantage = 0;
						other.landingFrameAdvantage = 0;
						player.landingFrameAdvantageNoPreBlockstun = 0;
						other.landingFrameAdvantageNoPreBlockstun = 0;
					} else if (player.idleLanding && other.idleLanding) {
						measuringLandingFrameAdvantage = -1;
						player.landingFrameAdvantageValid = true;
						other.landingFrameAdvantageValid = true;
					}
				}
				
			}
			
			if (player.charType == CHARACTER_TYPE_INO) {
				player.noteTime = -1;
				int noteElapsedTime = 0;
				int noteSlowdown = 0;
				for (int j = 2; j < entityList.count; ++j) {
					Entity p = entityList.list[j];
					if (p.isActive() && p.team() == player.index && !p.isPawn() && strcmp(p.animationName(), "Onpu") == 0) {
						if (!(
									p.mem45() && strcmp(p.gotoLabelRequest(), "hit") != 0
						)) {
							player.noteTime = p.currentAnimDuration();
							ProjectileInfo& projectile = findProjectile(p);
							if (projectile.ptr) {
								noteElapsedTime = projectile.elapsedTime + 1;
								noteSlowdown = projectile.rcSlowedDownCounter;
							}
						}
						break;
					}
				}
				if (player.noteTime != -1) {
					player.lastNoteTime = player.noteTime;
					if (player.noteTime >= 68) {
						player.noteLevel = 5;
						player.noteTimeMax = -1;
					} else if (player.noteTime >= 56) {
						player.noteLevel = 4;
						player.noteTimeMax = 68;
					} else if (player.noteTime >= 44) {
						player.noteLevel = 3;
						player.noteTimeMax = 56;
					} else if (player.noteTime >= 32) {
						player.noteLevel = 2;
						player.noteTimeMax = 44;
					} else {
						player.noteLevel = 1;
						player.noteTimeMax = 32;
					}
					
					if (player.noteTimeMax != -1) {
						int result;
						int unused;
						PlayerInfo::calculateSlow(
							noteElapsedTime,
							player.noteTimeMax - player.noteTime,
							noteSlowdown,
							&result,
							&unused,
							&unused);
						player.noteTimeWithSlow = noteElapsedTime;
						player.noteTimeWithSlowMax = noteElapsedTime + result;
					} else {
						player.noteTimeWithSlow = noteElapsedTime;
						player.noteTimeWithSlowMax = -1;
					}
				}
			}
			
			PlayerFramebars& entityFramebar = playerFramebars[player.index];
			PlayerFramebar& framebar = entityFramebar.idleHitstop;
			
			bool inXStun = player.inHitstun
				|| player.blockstun > 0
				|| player.wakeupTiming;
			if (player.changedAnimOnThisFrame
					&& (
						player.startup == 0
						|| player.changedAnimFiltered
						|| player.cmnActIndex == CmnActJumpPre
						&& player.animFrame == 1
						|| player.cmnActIndex == CmnActJump
						&& player.animFrame == 1
						|| player.cmnActIndex == CmnActJumpLanding
						&& player.animFrame == 1
						|| player.cmnActIndex == CmnActLandingStiff
						&& player.animFrame == 1
					)
					&& !player.pawn.isRCFrozen()) {
				framebar.requestFirstFrame = true;
			}
			
			PlayerFrame& prevFrame = framebar[EntityFramebar::posMinusOne(framebarPos)];
			if (player.wasEnableNormals
					&& !player.canBlock) {
				if (prevFrame.type != FT_NONE && !prevFrame.canBlock && !prevFrame.enableNormals) {
					framebar.requestFirstFrame = true;
				}
			}
			
			FrameType hasCancelsRecoveryFrameType = FT_NONE;
			bool hasCancelsOverridePrevRecovery = false;
			bool hasCancels = false;
			if (player.move.isRecoveryCanReload && player.move.isRecoveryCanReload(player)) {
				hasCancelsRecoveryFrameType = FT_RECOVERY_CAN_RELOAD;
				hasCancels = true;
			} else if (player.move.isRecoveryCanAct && player.move.isRecoveryCanAct(player)) {
				hasCancelsRecoveryFrameType = FT_RECOVERY_CAN_ACT;
				hasCancels = true;
			} else if (player.move.isRecoveryHasGatlings
					&& player.move.isRecoveryHasGatlings(player)) {
				hasCancelsRecoveryFrameType = FT_RECOVERY_HAS_GATLINGS;
				hasCancels = true;
				
			// Automatic version of isRecoveryHasGatlings
			} else if (!(
						player.move.isRecoveryHasGatlings
						|| player.move.isRecoveryCanAct
						|| player.move.isRecoveryCanReload
					)
					&& (
						player.wasEnableGatlings
						&& player.wasAttackCollidedSoCanCancelNow
						&& (
							!player.wasCancels.gatlings.empty()
							|| player.wasEnableSpecialCancel
						)
						|| player.wasEnableWhiffCancels
						&& !player.wasCancels.whiffCancels.empty()
					)) {
				if (player.recovery >= 1) {
					hasCancelsRecoveryFrameType = FT_RECOVERY_HAS_GATLINGS;
					hasCancelsOverridePrevRecovery = true;
				}
				hasCancels = true;
			}
			
			PlayerFrame& currentFrame = framebar[framebarPos];
			FrameType defaultStartupFrame = FT_STARTUP;
			FrameType defaultActiveFrame = player.hitstop ? FT_ACTIVE_HITSTOP : FT_ACTIVE;
			FrameType defaultRecoveryFrameType = FT_RECOVERY;
			bool overrideStartupFrame = false;
			bool isAzami = player.move.ignoresHitstop;
			FrameType standardXStun;
			if (player.wasEnableAirtech && player.hitstun == 0) {
				standardXStun = FT_GRAYBEAT_AIR_HITSTUN;
			} else if (player.hitstop) {
				standardXStun = FT_XSTUN_HITSTOP;
			} else if (player.charType == CHARACTER_TYPE_BAIKEN
					&& player.blockstun
					&& !player.wasCancels.whiffCancels.empty()) {
				standardXStun = FT_XSTUN_CAN_CANCEL;
			} else {
				standardXStun = FT_XSTUN;
			}
			
			FrameType startupFrameType;
			if (player.canBlock) {
				if (hasCancels) {
					startupFrameType = FT_STARTUP_CAN_BLOCK_AND_CANCEL;
				} else {
					startupFrameType = FT_STARTUP_CAN_BLOCK;
				}
			} else {
				startupFrameType = FT_STARTUP;
			}
			
			Entity ent = entityList.slots[i];
			bool hasHitboxes = player.hitboxesCount > 0;
			bool enableSpecialCancel = player.wasEnableSpecialCancel
					&& player.wasAttackCollidedSoCanCancelNow
					&& player.wasEnableGatlings;
			bool hitAlreadyHappened = player.pawn.hitAlreadyHappened() >= player.pawn.theValueHitAlreadyHappenedIsComparedAgainst()
					|| !player.pawn.currentHitNum();
			player.getInputs(game.getInputRingBuffers() + player.index, isTheFirstFrameInTheMatch);
			
			if (framebarAdvancedIdleHitstop) {
				
				FrameType zatoFrameType = FT_NONE;
				if (player.move.zatoHoldLevel && !player.idle) {
					DWORD zatoHoldLevel = player.move.zatoHoldLevel(player);
					if ((zatoHoldLevel & 3) >= 2) {
						if ((zatoHoldLevel & 4) != 0) {
							zatoFrameType = (FrameType)((char)FT_ZATO_BREAK_THE_LAW_STAGE2_RELEASED + (zatoHoldLevel & 3) - 2);
						} else {
							zatoFrameType = (FrameType)((char)FT_ZATO_BREAK_THE_LAW_STAGE2 + (zatoHoldLevel & 3) - 2);
						}
					}
				}
				
				MilliaInfo milliaInfo { 0 };
				if (player.charType == CHARACTER_TYPE_MILLIA) {
					player.milliaChromingRoseTimeLeft = player.wasPlayerval1Idling;
					if (player.wasPlayerval1Idling) {
						player.milliaChromingRoseTimeLeft += 10;
					} else {
						for (int j = 2; j < entityList.count; ++j) {
							Entity p = entityList.list[j];
							if (p.isActive() && p.team() == i && !p.isPawn()
									&& strcmp(p.animationName(), "RoseBody") == 0
									&& strcmp(p.spriteName(), "null") == 0) {
								if (p.spriteFrameCounterMax() == 10) {
									player.milliaChromingRoseTimeLeft += 11 - p.spriteFrameCounter();
								} else {
									++player.milliaChromingRoseTimeLeft;
								}
							}
						}
					}
					milliaInfo = player.canProgramSecretGarden();
					milliaInfo.chromingRose = player.milliaChromingRoseTimeLeft;
					milliaInfo.chromingRoseMax = player.maxDI;
					currentFrame.u.milliaInfo = milliaInfo;
				} else if (player.charType == CHARACTER_TYPE_CHIPP) {
					currentFrame.u.chippInfo.invis = player.playerval0;
					currentFrame.u.chippInfo.wallTime = 0;
					if (player.move.caresAboutWall) {
						int wallTime = player.pawn.mem54();
						if (wallTime > USHRT_MAX || wallTime <= 0) wallTime = USHRT_MAX;
						currentFrame.u.chippInfo.wallTime = wallTime;
					}
				} else if (player.charType == CHARACTER_TYPE_SOL) {
					currentFrame.u.diInfo.current = player.playerval1 < 0 ? USHRT_MAX : player.playerval1;
					currentFrame.u.diInfo.max = player.maxDI;
				} else if (player.charType == CHARACTER_TYPE_ZATO) {
					currentFrame.u.diInfo.current = player.pawn.exGaugeValue(0);
					currentFrame.u.diInfo.max = 6000;
				} else if (player.charType == CHARACTER_TYPE_SLAYER) {
					currentFrame.u.diInfo.current = player.wasPlayerval1Idling;
					currentFrame.u.diInfo.max = player.maxDI;
				} else if (player.charType == CHARACTER_TYPE_INO) {
					currentFrame.u.inoInfo.airdashTimer = player.wasProhibitFDTimer;
					currentFrame.u.inoInfo.noteTime = player.noteTimeWithSlow;
					currentFrame.u.inoInfo.noteTimeMax = player.noteTimeWithSlowMax;
					currentFrame.u.inoInfo.noteLevel = player.noteLevel;
				} else if (player.charType == CHARACTER_TYPE_BEDMAN) {
					struct SealInfo {
						Moves::MayIrukasanRidingObjectInfo& info;
						unsigned short& sealTimer;
						unsigned short& sealTimerMax;
						const char* stateName;
						int signal;
					};
					SealInfo seals[4] {
						{ moves.bedmanSealA, player.bedmanInfo.sealA, player.bedmanInfo.sealAMax, "DejavIconBoomerangA", 29 },
						{ moves.bedmanSealB, player.bedmanInfo.sealB, player.bedmanInfo.sealBMax, "DejavIconBoomerangB", 31 },
						{ moves.bedmanSealC, player.bedmanInfo.sealC, player.bedmanInfo.sealCMax, "DejavIconSpiralBed", 32 },
						{ moves.bedmanSealD, player.bedmanInfo.sealD, player.bedmanInfo.sealDMax, "DejavIconFlyingBed", 30 }
					};
					for (int j = 0; j < 4; ++j) {
						seals[j].sealTimer = 0;
					}
					int n = 4;
					for (ProjectileInfo& projectile : projectiles) {
						if (projectile.team != player.index) continue;
						for (int j = 0; j < n; ++j) {
							SealInfo& seal = seals[j];
							if (strcmp(projectile.animName, seal.stateName) == 0) {
								bool isFrameAfter = false;
								int remainingFrames = moves.getBedmanSealRemainingFrames(projectile, seal.info, seal.signal, &isFrameAfter);
								int calculatedResult;
								int calculatedResultMax;
								int unused;
								PlayerInfo::calculateSlow(
									projectile.bedmanSealElapsedTime + 1,
									remainingFrames,
									projectile.rcSlowedDownCounter,
									&calculatedResult,
									&calculatedResultMax,
									&unused);
								if (calculatedResult || isFrameAfter) {
									++calculatedResult;
								}
								++calculatedResultMax;
								seal.sealTimer = calculatedResult > 0xffff ? 0xffff : calculatedResult;
								seal.sealTimerMax = calculatedResultMax > 0xffff ? 0xffff : calculatedResultMax;
								if (j + 1 < n) {
									memmove(seals + j, seals + j + 1, (n - j - 1) * sizeof SealInfo);
								}
								--n;
								break;
							}
						}
						if (n == 0) break;
					}
					currentFrame.u.bedmanInfo = player.bedmanInfo;
				} else if (player.charType == CHARACTER_TYPE_RAMLETHAL) {
					if (player.ramlethalSSwordTimerActive) {
						currentFrame.u.ramlethalInfo.sSwordTime = player.ramlethalSSwordTime;
						if (!player.ramlethalSSwordKowareSonoba) {
							currentFrame.u.ramlethalInfo.sSwordTimeMax = player.ramlethalSSwordTimeMax;
						} else {
							currentFrame.u.ramlethalInfo.sSwordTimeMax = 0;
						}
						currentFrame.u.ramlethalInfo.sSwordSubAnim = player.ramlethalSSwordSubanim;
					} else {
						currentFrame.u.ramlethalInfo.sSwordTime = 0;
						currentFrame.u.ramlethalInfo.sSwordTimeMax = 0;
						currentFrame.u.ramlethalInfo.sSwordSubAnim = nullptr;
					}
					
					if (player.ramlethalHSwordTimerActive) {
						currentFrame.u.ramlethalInfo.hSwordTime = player.ramlethalHSwordTime;
						if (!player.ramlethalHSwordKowareSonoba) {
							currentFrame.u.ramlethalInfo.hSwordTimeMax = player.ramlethalHSwordTimeMax;
						} else {
							currentFrame.u.ramlethalInfo.hSwordTimeMax = 0;
						}
						currentFrame.u.ramlethalInfo.hSwordSubAnim = player.ramlethalHSwordSubanim;
					} else {
						currentFrame.u.ramlethalInfo.hSwordTime = 0;
						currentFrame.u.ramlethalInfo.hSwordTimeMax = 0;
						currentFrame.u.ramlethalInfo.hSwordSubAnim = nullptr;
					}
				} else if (player.charType == CHARACTER_TYPE_ELPHELT) {
					currentFrame.u.elpheltInfo.grenadeTimer = player.wasResource;
					currentFrame.u.elpheltInfo.grenadeDisabledTimer = min(255, player.elpheltGrenadeRemainingWithSlow);
					currentFrame.u.elpheltInfo.grenadeDisabledTimerMax = min(255, player.elpheltGrenadeMaxWithSlow);
				} else {
					currentFrame.u.milliaInfo = milliaInfo;
				}
				
				if (player.move.butForFramebarDontCombineWithPreviousMove
						&& !player.startedUp
						&& player.startupProj) {
					if (!player.startupProjIgnoredForFramebar
							&& player.animFrame == 1
							&& !player.pawn.isRCFrozen()
							&& !superflashInstigator) {
						if (!player.activesProj.count || player.activesProj.last().nonActives) {
							player.startupProjIgnoredForFramebar = player.activesProj.count;
						}
						framebar.requestFirstFrame = true;
					} else if (player.startupProjIgnoredForFramebar != player.activesProj.count) {
						player.startupProjIgnoredForFramebar = 0;
					}
				}
				if (player.wasHitOnPreviousFrame && inXStun && !player.wakeupTiming) {
					framebar.requestFirstFrame = true;
				}
				
				currentFrame.aswEngineTick = aswEngineTickCount;
				if (inXStun) {
					currentFrame.type = standardXStun;
				} else if (!player.idle && player.hitstop && !isAzami) {
					currentFrame.type = hasHitboxes ? FT_ACTIVE_HITSTOP : FT_HITSTOP;
				} else if (!player.idle) {
					if (player.inNewMoveSection && player.move.considerNewSectionAsBeingInElpheltRifleStateBeforeBeingAbleToShoot) {
						overrideStartupFrame = true;
						defaultStartupFrame = FT_IDLE_ELPHELT_RIFLE;
					} else {
						bool isInVariableStartupSection = player.isInVariableStartupSection();
						bool isInASectionBeforeVariableStartup = false;
						if (isInVariableStartupSection) {
							overrideStartupFrame = true;
							defaultRecoveryFrameType = FT_RECOVERY_HAS_GATLINGS;
							if (player.move.considerVariableStartupAsStanceForFramebar) {
								if (player.move.aSectionBeforeVariableStartup) {
									defaultStartupFrame = FT_STARTUP_ANYTIME_NOW_CAN_ACT;
								} else if (player.move.canStopHolding && player.move.canStopHolding(player)) {
									defaultStartupFrame = FT_STARTUP_STANCE_CAN_STOP_HOLDING;
								} else {
									defaultStartupFrame = FT_STARTUP_STANCE;
								}
							} else if (zatoFrameType != FT_NONE) {
								defaultStartupFrame = zatoFrameType;
							} else {
								defaultStartupFrame = FT_STARTUP_ANYTIME_NOW;
							}
						} else if (player.move.aSectionBeforeVariableStartup) {
							isInASectionBeforeVariableStartup = player.move.aSectionBeforeVariableStartup(player);
							if (isInASectionBeforeVariableStartup) {
								defaultStartupFrame = FT_STARTUP_ANYTIME_NOW;
							}
						} else if (zatoFrameType != FT_NONE) {
							defaultStartupFrame = zatoFrameType;
							overrideStartupFrame = true;
						} else if (milliaInfo.canProgramSecretGarden) {
							overrideStartupFrame = true;
							defaultStartupFrame = FT_STARTUP_CAN_PROGRAM_SECRET_GARDEN;
						}
						if (!isInVariableStartupSection && !isInASectionBeforeVariableStartup && !milliaInfo.canProgramSecretGarden && zatoFrameType == FT_NONE) {
							defaultStartupFrame = startupFrameType;
						}
					}
					currentFrame.type = defaultStartupFrame;
				} else if (!player.canBlock) {
					if (player.move.canBeUnableToBlockIndefinitelyOrForVeryLongTime) {
						if (player.inNewMoveSection && player.move.considerNewSectionAsBeingInElpheltRifleStateBeforeBeingAbleToShoot) {
							defaultStartupFrame = FT_IDLE_ELPHELT_RIFLE_READY;
						} else if (player.move.faustPogo) {
							defaultStartupFrame = FT_IDLE_CANT_BLOCK;
						} else {
							defaultStartupFrame = FT_STARTUP_STANCE;
						}
						overrideStartupFrame = true;
						currentFrame.type = defaultStartupFrame;
					} else {
						currentFrame.type = FT_IDLE_CANT_BLOCK;
					}
				} else if (!player.canFaultlessDefense) {
					currentFrame.type = FT_IDLE_CANT_FD;
				} else if (player.airborne
						&& player.idle
						&& player.canBlock
						&& player.y == 0
						&& player.speedY != 0
						&& !player.pawn.ascending()) {  // the last check idk if I need it
					currentFrame.type = FT_IDLE_AIRBORNE_BUT_CAN_GROUND_BLOCK;
				} else {
					currentFrame.type = FT_IDLE;
				}
				currentFrame.isFirst = framebar.requestFirstFrame;
				currentFrame.newHit = framebar.requestNextHit;
				currentFrame.enableNormals = player.wasEnableNormals;
				currentFrame.canBlock = player.canBlock;
				currentFrame.strikeInvulInGeneral = player.strikeInvul.active;
				currentFrame.throwInvulInGeneral = player.throwInvul.active;
				currentFrame.OTGInGeneral = player.wasOtg;
				currentFrame.superArmorActiveInGeneral = player.superArmor.active || player.projectileOnlyInvul.active || player.reflect.active;
				
				#define INVUL_TYPES_EXEC(enumName, stringDesc, fieldName) currentFrame.fieldName = player.fieldName.active;
				INVUL_TYPES_TABLE
				#undef INVUL_TYPES_EXEC
				
				currentFrame.superArmorActiveInGeneral_IsFull =
					currentFrame.superArmorActiveInGeneral
					&& (
						player.superArmorMid.active
						&& player.superArmorLow.active
						&& player.superArmorOverhead.active
						&& (
							strcmp(player.anim, "CounterGuardStand"_hardcode) == 0
							|| strcmp(player.anim, "CounterGuardCrouch"_hardcode) == 0
							|| strcmp(player.anim, "CounterGuardAir"_hardcode) == 0
						)
						&& player.superArmorHontaiAttacck.active
						&& !player.projectileOnlyInvul.active
						&& !player.reflect.active
						|| player.charType == CHARACTER_TYPE_FAUST
						&& strcmp(player.anim, "NmlAtk5E") == 0
						&& strcmp(player.sprite.name, "fau205_05") == 0
					);
				
				currentFrame.airborne = player.airborne;
				currentFrame.hitAlreadyHappened = hitAlreadyHappened;
				currentFrame.hitConnected = (
						player.pawn.hitSomethingOnThisFrame()
						|| player.hitSomething
					)
					|| player.pawn.inBlockstunNextFrame()
					|| player.pawn.inHitstunNextFrame()
					|| player.armoredHitOnThisFrame
					|| player.gotHitOnThisFrame;
				currentFrame.enableSpecialCancel = enableSpecialCancel;
				currentFrame.enableJumpCancel = player.wasEnableJumpCancel;
				currentFrame.enableSpecials = false;
				player.determineMoveNameAndSlangName(&currentFrame.animName, &currentFrame.animSlangName);
				currentFrame.cancels = player.wasCancels;
				currentFrame.hitstop = player.hitstop;
				currentFrame.hitstopMax = player.hitstopMax;
				if (player.stagger && player.cmnActIndex == CmnActJitabataLoop) {
					currentFrame.stop.isBlockstun = false;
					currentFrame.stop.isHitstun = false;
					currentFrame.stop.isStagger = true;
					currentFrame.stop.isWakeup = false;
					currentFrame.stop.value = min(player.staggerWithSlow, 8192);
					currentFrame.stop.valueMax = min(player.staggerMaxWithSlow, 2047);
					currentFrame.stop.valueMaxExtra = 0;
					currentFrame.stop.tumble = 0;
				} else if (player.blockstun) {
					currentFrame.stop.isBlockstun = true;
					currentFrame.stop.isHitstun = false;
					currentFrame.stop.isStagger = false;
					currentFrame.stop.isWakeup = false;
					if (player.blockstunContaminatedByRCSlowdown) {
						currentFrame.stop.value = min(player.blockstunWithSlow, 8192);
						currentFrame.stop.valueMax = min(player.blockstunMaxWithSlow, 2047);
						currentFrame.stop.valueMaxExtra = 0;
					} else {
						currentFrame.stop.value = min(player.blockstun, 8192);
						currentFrame.stop.valueMax = min(player.blockstunMax, 2047);
						currentFrame.stop.valueMaxExtra = min(player.blockstunMaxLandExtra, 15);
					}
					currentFrame.stop.tumble = 0;
				} else if (player.hitstun && player.inHitstun) {
					currentFrame.stop.isBlockstun = false;
					currentFrame.stop.isHitstun = true;
					currentFrame.stop.isStagger = false;
					currentFrame.stop.isWakeup = false;
					if (player.hitstunContaminatedByRCSlowdown) {
						currentFrame.stop.value = min(player.hitstunWithSlow, 8192);
						currentFrame.stop.valueMax = min(player.hitstunMaxWithSlow, 2047);
						currentFrame.stop.valueMaxExtra = 0;
					} else {
						currentFrame.stop.value = min(player.hitstun, 8192);
						currentFrame.stop.valueMax = min(player.hitstunMax, 2047);
						currentFrame.stop.valueMaxExtra = min(player.hitstunMaxFloorbounceExtra, 15);
					}
					if (player.cmnActIndex == CmnActKorogari) {
						currentFrame.stop.tumble = min(player.tumbleWithSlow, 0xffff);
						currentFrame.stop.tumbleMax = min(player.tumbleMaxWithSlow, 0xffff);
					} else {
						currentFrame.stop.tumble = 0;
						currentFrame.stop.tumbleMax = 0;
					}
				} else if (player.wakeupTiming) {
					currentFrame.stop.isBlockstun = false;
					currentFrame.stop.isHitstun = false;
					currentFrame.stop.isStagger = false;
					currentFrame.stop.isWakeup = true;
					currentFrame.stop.value = min(player.wakeupTimingWithSlow, 8192);
					currentFrame.stop.valueMax = min(player.wakeupTimingMaxWithSlow, 2047);
					currentFrame.stop.valueMaxExtra = 0;
					currentFrame.stop.tumble = 0;
					currentFrame.stop.tumbleMax = 0;
				} else {
					currentFrame.stop.isBlockstun = false;
					currentFrame.stop.isHitstun = false;
					currentFrame.stop.isStagger = false;
					currentFrame.stop.isWakeup = false;
					if (player.cmnActIndex == CmnActKorogari) {
						currentFrame.stop.tumble = min(player.tumbleWithSlow, 0xffff);
						currentFrame.stop.tumbleMax = min(player.tumbleMaxWithSlow, 0xffff);
					} else {
						currentFrame.stop.tumble = 0;
						currentFrame.stop.tumbleMax = 0;
					}
				}
				currentFrame.IBdOnThisFrame = player.inBlockstunNextFrame
					&& player.lastBlockWasIB;
				currentFrame.FDdOnThisFrame = player.inBlockstunNextFrame
					&& player.lastBlockWasFD;
				currentFrame.blockedOnThisFrame = player.inBlockstunNextFrame;
				currentFrame.lastBlockWasIB = player.lastBlockWasIB;
				currentFrame.lastBlockWasFD = player.lastBlockWasFD;
				
				if (!player.airborne || player.airborne && player.y == 0 && player.speedY != 0 && !player.pawn.ascending()) {
					int crossupProtection = player.pawn.crossupProtection();
					currentFrame.crossupProtectionIsOdd = (crossupProtection & 1);
					currentFrame.crossupProtectionIsAbove1 = (crossupProtection & 2) >> 1;
				} else {
					currentFrame.crossupProtectionIsOdd = 0;
					currentFrame.crossupProtectionIsAbove1 = 0;
				}
				currentFrame.rcSlowdown = player.rcSlowedDownCounter;
				currentFrame.rcSlowdownMax = player.rcSlowedDownMax;
				
				currentFrame.poisonDuration = player.poisonDuration;
				
				currentFrame.needShowAirOptions = player.regainedAirOptions;
				currentFrame.doubleJumps = player.remainingDoubleJumps;
				currentFrame.airDashes = player.remainingAirDashes;
				currentFrame.activeDuringSuperfreeze = false;
				
				currentFrame.inputs = player.inputs;
				currentFrame.prevInput = player.prevInput;
				currentFrame.inputsOverflow = player.inputsOverflow;
				if (player.inputs.empty()) {
					player.prevInput = Input{0x0000};
				} else {
					player.prevInput = player.inputs.back();
				}
				player.inputsOverflow = false;
				player.inputs.clear();
				
				currentFrame.canYrc = player.wasCanYrc;
				currentFrame.canYrcProjectile = player.wasCanYrc
					&& player.move.canYrcProjectile
					&& player.move.canYrcProjectile(player);
				currentFrame.createdDangerousProjectile = player.createdDangerousProjectile
					|| player.createdProjectileThatSometimesCanBeDangerous  // for Ky 5D
					|| player.move.createdProjectile
					&& player.move.createdProjectile(player);
				
				const InputRingBuffer* ringBuffer = game.getInputRingBuffers() + player.index;
				int charge = ringBuffer->parseCharge(InputRingBuffer::CHARGE_TYPE_HORIZONTAL, false);
				currentFrame.chargeLeft = charge > 254 ? 255 : charge;
				if (charge) player.chargeLeftLast = charge;
				currentFrame.chargeLeftLast = player.chargeLeftLast;
				charge = ringBuffer->parseCharge(InputRingBuffer::CHARGE_TYPE_HORIZONTAL, true);
				currentFrame.chargeRight = charge > 254 ? 255 : charge;
				currentFrame.chargeRightLast = player.chargeRightLast;
				if (charge) player.chargeRightLast = charge;
				charge = ringBuffer->parseCharge(InputRingBuffer::CHARGE_TYPE_VERTICAL, false);
				currentFrame.chargeDown = charge > 254 ? 255 : charge;
				if (charge) player.chargeDownLast = charge;
				currentFrame.chargeDownLast = player.chargeDownLast;
				
				currentFrame.powerup = player.move.powerup && player.move.powerup(player);
				if (currentFrame.powerup) {
					currentFrame.powerupExplanation = player.move.powerupExplanation ? player.move.powerupExplanation(player) : nullptr;
					currentFrame.dontShowPowerupGraphic = player.move.dontShowPowerupGraphic ? player.move.dontShowPowerupGraphic(player) : false;
				} else {
					currentFrame.powerupExplanation = nullptr;
					currentFrame.dontShowPowerupGraphic = false;
				}
				currentFrame.airthrowDisabled = player.airborne && player.pawn.airthrowDisabled();
				currentFrame.running = player.pawn.running();
				currentFrame.cantBackdash = player.wasCantBackdashTimer != 0;
				if (player.x == player.prevPosX) {
					currentFrame.suddenlyTeleported = false;
				} else if (player.x > player.prevPosX) {
					currentFrame.suddenlyTeleported = player.x - player.prevPosX >= player.speedX + 122499;
				} else {
					currentFrame.suddenlyTeleported = player.prevPosX - player.x >= -player.speedX + 122499;
				}
				if (player.y != player.prevPosY) {
					if (player.y > player.prevPosY) {
						currentFrame.suddenlyTeleported = currentFrame.suddenlyTeleported || (
							player.y - player.prevPosY >= player.speedY + 122499
						);
					} else {
						currentFrame.suddenlyTeleported = currentFrame.suddenlyTeleported || (
							player.prevPosY - player.y >= -player.speedY + 122499
						);
					}
				}
			} else if (superflashInstigator && player.gotHitOnThisFrame) {
				currentFrame.hitConnected = true;
			}
			
			FrameType recoveryFrameType = defaultRecoveryFrameType;
			bool overridePrevRecoveryFrames = false;
			if (hasCancelsRecoveryFrameType != FT_NONE) {
				recoveryFrameType = hasCancelsRecoveryFrameType;
				overridePrevRecoveryFrames = hasCancelsOverridePrevRecovery;
			}
			
			if (player.cmnActIndex == CmnActRomanCancel && !player.superfreezeStartup) {
				recoveryFrameType = FT_STARTUP;
			} else if (player.move.secondaryStartup && player.move.secondaryStartup(player)) {
				recoveryFrameType = FT_STARTUP;
				if (prevFrame.type != FT_NONE && prevFrame.type == FT_RECOVERY_HAS_GATLINGS) {
					framebar.requestFirstFrame = true;
					if (framebarAdvancedIdleHitstop) {
						currentFrame.isFirst = true;
					}
				}
			}
			
			if (inXStun) {
				startupFrameType = standardXStun;
			} else if (!player.startedUp
					&& (player.startupProj && !player.startupProjIgnoredForFramebar)
					&& !(
						player.cmnActIndex == CmnActRomanCancel
						&& !player.superfreezeStartup
					)) {
				if (hasCancels && recoveryFrameType == FT_RECOVERY) {
					startupFrameType = FT_RECOVERY_HAS_GATLINGS;
				} else {
					startupFrameType = recoveryFrameType;
				}
			} else if (player.cmnActIndex == CmnActRomanCancel && player.superfreezeStartup) {
				startupFrameType = recoveryFrameType;
			} else if (overrideStartupFrame) {
				startupFrameType = defaultStartupFrame;
			}
			
			bool measureInvuls = !(player.hitstop && !isAzami)
					&& !superflashInstigator
					&& (player.strikeInvul.active
						|| player.throwInvul.active && !player.move.canBeUnableToBlockIndefinitelyOrForVeryLongTime  // anti Faust-Pogo infinite throw invul
						|| player.projectileOnlyInvul.active
						|| player.superArmor.active
						|| player.reflect.active);
			
			int playerMaxThing = player.total;
			if (player.totalCanBlock > playerMaxThing) playerMaxThing = player.totalCanBlock;
			if (player.totalCanFD > playerMaxThing) playerMaxThing = player.totalCanFD;
			
			if (player.isInFDWithoutBlockstun) {
				++player.totalFD;
				currentFrame.type = standardXStun;
			} else if (!(player.hitstop && !isAzami)
					&& !superflashInstigator
					&& (
						player.cmnActIndex == CmnActLandingStiff && !player.idle  // Ramlethal j.8D becomes "idle" on f6 of stiff landing
						|| !player.idle
						&& !player.airborne
						&& player.moveOriginatedInTheAir
					)
					&& !hasHitboxes
					&& !player.onTheDefensive  // Potemkin I.C.P.M may cause a situation where, on hit, opponent is put into CmnActLockWait that begins airborne,
					                           // then transitions to the ground and, if this check wasn't here, it would be seen as landing animation (lol)
					&& player.cmnActIndex != CmnActRomanCancel  // needed for Elphelt j.D YRC
				) {
				currentFrame.type = hasCancels ? FT_LANDING_RECOVERY_CAN_CANCEL : FT_LANDING_RECOVERY;
				++player.landingRecovery;
				player.totalFD = 0;
				measureInvuls = player.total
					&& !player.theAnimationIsNotOverYetLolConsiderBusyNonAirborneFramesAsLandingAnimation
					&& !player.ignoreNextInabilityToBlockOrAttack;
			} else if (!(player.hitstop && !isAzami)
					&& !(superflashInstigator && superflashInstigator != ent)
					&& !(player.cmnActIndex == CmnActJump && player.canFaultlessDefense && !player.performingBDC)
					&& (player.cmnActIndex != CmnActJumpPre || player.performingBDC)
					&& !player.isLanding) {
				if (
						(
							(
								!player.idlePlus
								|| player.idleInNewSection
								|| player.forceBusy
								|| !(player.canBlock && player.canFaultlessDefense || player.move.canBeUnableToBlockIndefinitelyOrForVeryLongTime)
							)
							&& !player.ignoreNextInabilityToBlockOrAttack
						)
						&& !player.startedUp
						&& !superflashInstigator
					) {
					if (!player.idlePlus || player.idleInNewSection || player.forceBusy) {
						if (player.landingRecovery) {  // needed for Zato Shadow Gallery. Removing airborne check for Answer s.S
							entityFramebar.changePreviousFrames(landingRecoveryTypes,
								_countof(landingRecoveryTypes),
								startupFrameType,
								framebarPos - 1,
								framebarHitstopSearchPos,
								framebarIdleSearchPos,
								framebarSearchPos,
								player.landingRecovery);
							player.startup += player.landingRecovery;
							player.total = playerMaxThing;
							player.totalCanBlock = playerMaxThing;
							player.totalCanFD = playerMaxThing;
							player.total += player.landingRecovery;
							player.totalCanBlock += player.landingRecovery;
							player.totalCanFD += player.landingRecovery;
							playerMaxThing += player.landingRecovery;
							player.landingRecovery = 0;
						}
						++player.startup;
						if (player.total < playerMaxThing) player.total = playerMaxThing;
						++player.total;
						measureInvuls = true;
					}
					if (!player.canBlock && !player.ignoreNextInabilityToBlockOrAttack
							&& !(player.move.canBeUnableToBlockIndefinitelyOrForVeryLongTime)) {
						if (player.totalCanBlock < playerMaxThing) player.totalCanBlock = playerMaxThing;
						++player.totalCanBlock;
					}
					if ((!player.canBlock || !player.canFaultlessDefense) && !player.ignoreNextInabilityToBlockOrAttack
							&& !(player.move.canBeUnableToBlockIndefinitelyOrForVeryLongTime)) {
						if (player.totalCanFD < playerMaxThing) player.totalCanFD = playerMaxThing;
						++player.totalCanFD;
					}
					if (!player.idlePlus || player.idleInNewSection || player.forceBusy
							|| (
								player.canBlock && player.canFaultlessDefense
								|| player.move.canBeUnableToBlockIndefinitelyOrForVeryLongTime
							)) {
						currentFrame.type = startupFrameType;
					}
				}
				if (superflashInstigator == ent && !player.superfreezeStartup) {
					player.superfreezeStartup = player.total;
				}
				if (hasHitboxes) {
					if (!player.startedUp) {
						player.totalCanBlock = player.total;  // needed for Raven glide
						player.totalCanFD = player.total;  // needed for Raven glide
						if (player.startupProj) {
							entityFramebar.changePreviousFramesOneType(FT_RECOVERY,
								startupFrameType,
								framebarPos - 1,
								framebarHitstopSearchPos,
								framebarIdleSearchPos,
								framebarSearchPos,
								player.startup - player.startupProj,
								true);
						}
						player.startedUp = true;
						player.inNewMoveSection = false;
						if (player.charType == CHARACTER_TYPE_POTEMKIN
								&& strcmp(player.anim, "HammerFall"_hardcode) == 0) {
							player.removeNonStancePrevStartups();
						}
						player.addActiveFrame(ent, framebar);
						player.maxHitUse = player.maxHit;
						currentFrame.type = defaultActiveFrame;
						if (superflashInstigator && !framebarAdvancedIdleHitstop && currentFrame.type != FT_NONE) {
							currentFrame.activeDuringSuperfreeze = true;
							if (player.pawn.hitSomethingOnThisFrame()
									|| player.pawn.dealtAttack()->attackMultiHit() && player.hitSomething) {
								currentFrame.hitConnected = true;
							}
						}
					} else if (!superflashInstigator) {
						if (player.recovery + player.landingRecovery) {
							if (player.landingRecovery) {
								entityFramebar.changePreviousFrames(landingRecoveryTypes,  // needed for Venom H Mad Struggle
									_countof(landingRecoveryTypes),
									FT_NON_ACTIVE,
									framebarPos - 1,
									framebarHitstopSearchPos,
									framebarIdleSearchPos,
									framebarSearchPos,
									player.landingRecovery);
								player.recovery += player.landingRecovery;
								player.total = playerMaxThing;
								player.totalCanBlock = playerMaxThing;
								player.totalCanFD = playerMaxThing;
								player.total += player.landingRecovery;
								player.totalCanBlock += player.landingRecovery;
								player.totalCanFD += player.landingRecovery;
								playerMaxThing += player.landingRecovery;
								player.landingRecovery = 0;
							}
							
							entityFramebar.changePreviousFrames(recoveryFrameTypes,
								_countof(recoveryFrameTypes),
								FT_NON_ACTIVE,
								framebarPos - 1 - player.landingRecovery,
								framebarHitstopSearchPos,
								framebarIdleSearchPos,
								framebarSearchPos,
								player.recovery);
							
							player.actives.addNonActive(player.recovery + player.landingRecovery);
							player.recovery = 0;
							player.landingRecovery = 0;
						}
						player.addActiveFrame(ent, framebar);
						player.maxHitUse = player.maxHit;
						currentFrame.type = defaultActiveFrame;
						if (player.total < playerMaxThing) player.total = playerMaxThing;
						++player.total;
						if (!player.canBlock && !player.ignoreNextInabilityToBlockOrAttack
								&& !(player.move.canBeUnableToBlockIndefinitelyOrForVeryLongTime)) {
							if (player.totalCanBlock < playerMaxThing) player.totalCanBlock = playerMaxThing;
							++player.totalCanBlock;
						}
						if ((!player.canBlock || !player.canFaultlessDefense) && !player.ignoreNextInabilityToBlockOrAttack
								&& !(player.move.canBeUnableToBlockIndefinitelyOrForVeryLongTime)) {
							if (player.totalCanFD < playerMaxThing) player.totalCanFD = playerMaxThing;
							++player.totalCanFD;
						}
						measureInvuls = true;
					}
				} else if (
						(
							(
								!player.idlePlus
								|| !(player.canBlock && player.canFaultlessDefense || player.move.canBeUnableToBlockIndefinitelyOrForVeryLongTime)
							) && !player.ignoreNextInabilityToBlockOrAttack
						)
						&& !superflashInstigator
						&& player.startedUp
					) {
					if (player.landingRecovery && !player.idlePlus) {
						entityFramebar.changePreviousFrames(landingRecoveryTypes,
							_countof(landingRecoveryTypes),
							recoveryFrameType,
							framebarPos - 1,
							framebarHitstopSearchPos,
							framebarIdleSearchPos,
							framebarSearchPos,
							player.landingRecovery);
						player.recovery += player.landingRecovery;
						player.total += player.landingRecovery;
						player.totalCanBlock += player.landingRecovery;
						player.totalCanFD += player.landingRecovery;
						player.landingRecovery = 0;
					}
					DWORD recoveryMode = 0;
					if (!player.idlePlus) {
						recoveryMode |= 1;
					}
					if (!player.canBlock && !player.ignoreNextInabilityToBlockOrAttack
							&& !(player.move.canBeUnableToBlockIndefinitelyOrForVeryLongTime)) {
						recoveryMode |= 2;
					}
					if ((!player.canBlock || !player.canFaultlessDefense) && !player.ignoreNextInabilityToBlockOrAttack
							&& !(player.move.canBeUnableToBlockIndefinitelyOrForVeryLongTime)) {
						recoveryMode |= 4;
					}
					if (recoveryMode
							&& overridePrevRecoveryFrames
							&& player.recovery == 1) {
						const FrameCancelInfo& cancelInfo = prevFrame.cancels;
						// needed for whiff 5P to not show first recovery frame as cancelable (it is not)
						if (!cancelInfo.gatlings.empty()
								|| !cancelInfo.whiffCancels.empty()
								|| prevFrame.enableSpecialCancel  // for Ky 3H
								|| prevFrame.enableSpecials) {
							entityFramebar.changePreviousFramesOneType(FT_RECOVERY,
								recoveryFrameType,
								framebarPos - 1,
								framebarHitstopSearchPos,
								framebarIdleSearchPos,
								framebarSearchPos,
								1);
						}
					}
					if ((recoveryMode & 1) != 0) {
						if (player.total < playerMaxThing) player.total = playerMaxThing;
						++player.total;
						++player.recovery;
						currentFrame.type = recoveryFrameType;
						measureInvuls = true;
					}
					if ((recoveryMode & 2) != 0) {
						currentFrame.type = recoveryFrameType;
						if (player.totalCanBlock < playerMaxThing) player.totalCanBlock = playerMaxThing;
						++player.totalCanBlock;
					}
					if ((recoveryMode & 4) != 0) {
						currentFrame.type = recoveryFrameType;
						if (player.totalCanFD < playerMaxThing) player.totalCanFD = playerMaxThing;
						++player.totalCanFD;
					}
				}
			}
			if (framebarAdvancedIdleHitstop && (
					frameTypeAssumesCantAttack(currentFrame.type)
					|| player.move.canBeUnableToBlockIndefinitelyOrForVeryLongTime
				)) {
				currentFrame.enableSpecials = player.wasEnableSpecials;
			}
			if (framebarAdvancedIdleHitstop) {
				currentFrame.newHit = framebar.requestNextHit;
			}
			if (!(player.hitstop && !isAzami)
					&& !superflashInstigator
					&& !(player.cmnActIndex == CmnActJump && player.canFaultlessDefense && !player.performingBDC)
					&& (player.cmnActIndex != CmnActJumpPre || player.performingBDC)
					&& !player.isLanding
					&& (!player.ignoreNextInabilityToBlockOrAttack || player.move.canBeUnableToBlockIndefinitelyOrForVeryLongTime)) {
				PlayerCancelInfo newCancelInfo;
				++player.cancelsTimer;
				newCancelInfo.start = player.cancelsTimer;
				newCancelInfo.end = player.cancelsTimer;
				newCancelInfo.enableSpecialCancel = enableSpecialCancel;
				newCancelInfo.enableJumpCancel = player.wasEnableJumpCancel;
				newCancelInfo.cancels = player.wasCancels;
				newCancelInfo.enableSpecials = false;
				newCancelInfo.hitAlreadyHappened = hitAlreadyHappened;
				newCancelInfo.airborne = player.airborne;
				player.appendPlayerCancelInfo(newCancelInfo);
			}
			
			copyIdleHitstopFrameToTheRestOfSubframebars(entityFramebar,
				framebarAdvanced,
				framebarAdvancedIdle,
				framebarAdvancedHitstop,
				framebarAdvancedIdleHitstop);
			
			if (measureInvuls) {
				int prevTotal = player.prevStartups.total() + player.total;
				
				#define INVUL_TYPES_EXEC(enumName, stringDesc, fieldName) player.fieldName.addInvulFrame(prevTotal);
				INVUL_TYPES_TABLE
				#undef INVUL_TYPES_EXEC
				
			}
		}
		
		for (PlayerInfo& player : players) {
			player.startupDisp = 0;
			player.activesDisp.clear();
			player.maxHitDisp.clear();
			player.recoveryDisp = 0;
			player.recoveryDispCanBlock = -1;
			player.totalDisp = 0;
			player.prevStartupsDisp.clear();
			player.prevStartupsTotalDisp.clear();
			player.hitOnFrameDisp;
			if (player.startedUp && !player.startupProj) {
				player.prevStartupsDisp = player.prevStartups;
				player.prevStartupsTotalDisp = player.prevStartups;
				player.startupDisp = player.startup;
				player.activesDisp = player.actives;
				player.maxHitDisp = player.maxHitUse;
				player.recoveryDisp = player.recovery;
				player.totalDisp = player.total;
				player.hitOnFrameDisp = player.hitOnFrame;
			} else if (player.startupProj && !player.startedUp) {
				int endOfActivesRelativeToPlayerTotalCountdownStart;
				if (!player.prevStartupsProj.empty()) {
					player.startupDisp = player.startupProj;
					player.prevStartupsDisp = player.prevStartupsProj;
					endOfActivesRelativeToPlayerTotalCountdownStart = player.prevStartupsProj.total() + player.startupProj + player.activesProj.total() - 1;
				} else {
					int prevStartupsTotal = player.prevStartups.total();
					if (player.startupProj < prevStartupsTotal) {  // needed for Dizzy 421H
						player.startupDisp = player.startupProj;
					} else {
						player.startupDisp = player.startupProj - prevStartupsTotal;
						player.prevStartupsDisp = player.prevStartups;
					}
					player.prevStartupsTotalDisp = player.prevStartups;
					endOfActivesRelativeToPlayerTotalCountdownStart = player.startupDisp + player.activesProj.total() - 1;
				}
				player.maxHitDisp = player.maxHitProj; 
				player.hitOnFrameDisp = !player.maxHitProj.empty() && player.maxHitProj.maxUse <= 1 && !player.maxHitProjConflict ? 0 : player.hitOnFrameProj;
				player.activesDisp = player.activesProj;
				int activesDispTotal = player.activesDisp.total();
				if (endOfActivesRelativeToPlayerTotalCountdownStart >= player.total) {
					player.recoveryDisp = 0;
					if (player.totalCanBlock > player.total && endOfActivesRelativeToPlayerTotalCountdownStart < player.totalCanBlock) {
						// needed for Answer Taunt
						player.recoveryDispCanBlock = player.totalCanBlock - endOfActivesRelativeToPlayerTotalCountdownStart;
					}
				} else {
					player.recoveryDisp = player.total - endOfActivesRelativeToPlayerTotalCountdownStart;
				}
				player.totalDisp = player.total;
			} else if (player.startedUp && player.startupProj) {
				if (!player.prevStartupsProj.empty()) {
					int projPrevTotal = player.prevStartupsProj.total();
					player.startupProj += projPrevTotal;
					player.prevStartupsProj.clear();
				}
				if (player.hitOnFrame && player.hitOnFrameProj) {
					if (player.hitOnFrame < player.hitOnFrameProj) {
						player.hitOnFrameDisp = player.hitOnFrame;
					} else {
						player.hitOnFrameDisp = player.hitOnFrameProj;
					}
				} else if (player.hitOnFrame) {
					player.hitOnFrameDisp = player.hitOnFrame;
				} else if (player.hitOnFrameProj) {
					player.hitOnFrameDisp = player.hitOnFrameProj;
				}
				player.prevStartupsDisp = player.prevStartups;
				player.prevStartupsTotalDisp = player.prevStartups;
				if (player.startup <= player.startupProj) {
					player.startupDisp = player.startup;
					player.activesDisp = player.actives;
					player.activesDisp.mergeTimeline(player.startupProj - player.startup + 1 - player.prevStartupsDisp.total(), player.activesProj);
				} else {
					player.startupDisp = player.startupProj;
					player.activesDisp = player.activesProj;
					player.activesDisp.mergeTimeline(player.startup - player.startupProj + 1 - player.prevStartupsDisp.total(), player.actives);
				}
				int activesDispTotal = player.activesDisp.total();
				int totalAbsoluteStartup = player.startupDisp + activesDispTotal - 1;
				if (totalAbsoluteStartup >= player.total) {
					player.recoveryDisp = 0;
				} else {
					player.recoveryDisp = player.total - totalAbsoluteStartup;
				}
				player.totalDisp = player.total;
			} else {
				player.prevStartupsDisp = player.prevStartups;
				player.prevStartupsTotalDisp = player.prevStartups;
				player.totalDisp = player.total;
			}
			
			if (player.hitstop) {
				player.displayHitstop = true;
			}
			if (player.stagger && player.cmnActIndex == CmnActJitabataLoop) {
				player.xStunDisplay = PlayerInfo::XSTUN_DISPLAY_STAGGER_WITH_SLOW;
			} else if (player.hitstun && player.inHitstun) {
				if (player.rcSlowedDown || player.hitstunContaminatedByRCSlowdown) {
					player.xStunDisplay = PlayerInfo::XSTUN_DISPLAY_HIT_WITH_SLOW;
				} else {
					player.xStunDisplay = PlayerInfo::XSTUN_DISPLAY_HIT;
				}
			} else if (player.blockstun) {
				if (player.rcSlowedDown || player.blockstunContaminatedByRCSlowdown) {
					player.xStunDisplay = PlayerInfo::XSTUN_DISPLAY_BLOCK_WITH_SLOW;
				} else {
					player.xStunDisplay = PlayerInfo::XSTUN_DISPLAY_BLOCK;
				}
			}
			player.prevGettingHitBySuper = player.gettingHitBySuper;
			player.prevFrameStunValue = player.pawn.dealtAttack()->stun;
			player.prevFrameMem45 = player.pawn.mem45();
			player.prevFrameMem46 = player.pawn.mem46();
			player.prevFrameGroundHitEffect = player.pawn.groundHitEffect();
			player.prevFrameGroundBounceCount = player.pawn.groundBounceCount();
			player.prevFrameTumbleDuration = player.pawn.tumbleDuration();
			player.prevFrameMaxHit = player.pawn.maxHit();
			player.prevFramePlayerval0 = player.playerval0;
			player.prevFramePlayerval1 = player.playerval1;
			player.prevFrameElpheltRifle_AimMem46 = player.elpheltRifle_AimMem46;
			player.prevPosX = player.x;
			player.prevPosY = player.y;
			
		}
		
		for (auto it = projectileFramebars.begin(); it != projectileFramebars.end(); ) {
			ProjectileFramebar& entityFramebar = *it;
			entityFramebar.main.completelyEmpty = true;
			entityFramebar.hitstop.completelyEmpty = true;
			
			{
				int i = 0;
				for (; i < _countof(Framebar::frames); ++i) {
					FrameType ft = entityFramebar.main[i].type;
					if (!frameTypeDiscardable(ft)) {
						entityFramebar.main.completelyEmpty = false;
						break;
					}
					ft = entityFramebar.hitstop[i].type;
					if (!frameTypeDiscardable(ft)) {
						entityFramebar.hitstop.completelyEmpty = false;
						break;
					}
				}
				if (i != _countof(Framebar::frames)) {
					if (entityFramebar.main.completelyEmpty) {
						for (++i; i < _countof(Framebar::frames); ++i) {
							FrameType ft = entityFramebar.main[i].type;
							if (!frameTypeDiscardable(ft)) {
								entityFramebar.main.completelyEmpty = false;
								break;
							}
						}
					} else {
						for (; i < _countof(Framebar::frames); ++i) {
							FrameType ft = entityFramebar.hitstop[i].type;
							if (!frameTypeDiscardable(ft)) {
								entityFramebar.hitstop.completelyEmpty = false;
								break;
							}
						}
					}
				}
			}
			
			if (entityFramebar.main.completelyEmpty && entityFramebar.hitstop.completelyEmpty) {
				it = projectileFramebars.erase(it);
			} else {
				++it;
			}
		}
	}
	bool combinedFramebarsSettingsChanged = false;
	if (combineProjectileFramebarsWhenPossible != settings.combineProjectileFramebarsWhenPossible) {
		combineProjectileFramebarsWhenPossible = settings.combineProjectileFramebarsWhenPossible;
		combinedFramebarsSettingsChanged = true;
	}
	if (eachProjectileOnSeparateFramebar != settings.eachProjectileOnSeparateFramebar) {
		eachProjectileOnSeparateFramebar = settings.eachProjectileOnSeparateFramebar;
		combinedFramebarsSettingsChanged = true;
	}
	if (neverIgnoreHitstop != settings.neverIgnoreHitstop) {
		neverIgnoreHitstop = settings.neverIgnoreHitstop;
		combinedFramebarsSettingsChanged = true;
	}
	if ((combinedFramebarsSettingsChanged || frameHasChanged) && !iGiveUp) {
		combinedFramebars.clear();
		combinedFramebars.reserve(projectileFramebars.size());
		if (!eachProjectileOnSeparateFramebar) {
			const bool combinedFramebarMustIncludeHitstop = neverIgnoreHitstop ? true : false;
			for (const ProjectileFramebar& source : projectileFramebars) {
				const Framebar& from = combinedFramebarMustIncludeHitstop ? source.hitstop : source.main;
				if (!from.completelyEmpty) {
					CombinedProjectileFramebar& entityFramebar = findCombinedFramebar(source, combinedFramebarMustIncludeHitstop);
					entityFramebar.combineFramebar(from, &source);
				}
			}
			for (CombinedProjectileFramebar& entityFramebar : combinedFramebars) {
				entityFramebar.determineName();
			}
		}
	}
	if (frameHasChanged && !iGiveUp) {
		Entity superflashInstigator = getSuperflashInstigator();
		if (!superflashInstigator) {
			superfreezeHasBeenGoingFor = 0;
		} else {
			lastNonZeroSuperflashInstigator = superflashInstigator;
			++superfreezeHasBeenGoingFor;
		}
	}
	if (frameHasChanged) {
		needFrameCleanup = true;
	}
	
#ifdef LOG_PATH
	didWriteOnce = true;
#endif
}

// Runs on the main thread
bool EndScene::isEntityAlreadyDrawn(const Entity& ent) const {
	return std::find(drawnEntities.cbegin(), drawnEntities.cend(), ent) != drawnEntities.cend();
}

// Runs on the main thread. Is called from WndProc hook
void EndScene::processKeyStrokes() {
	bool trainingMode = game.isTrainingMode();
	keyboard.updateKeyStatuses();
	bool needWriteSettings = false;
	bool stateChanged;
	{
		stateChanged = ui.stateChanged;
		ui.stateChanged = false;
	}
	if (!gifMode.modDisabled && (keyboard.gotPressed(settings.gifModeToggle) || stateChanged && ui.gifModeOn != gifMode.gifModeOn)) {
		// idk how atomic_bool reacts to ! and operator bool(), so we do it the arduous way
		if (gifMode.gifModeOn == true) {
			gifMode.gifModeOn = false;
			logwrap(fputs("GIF mode turned off\n", logfile));
			needToRunNoGravGifMode = needToRunNoGravGifMode || (*aswEngine != nullptr);
		} else if (trainingMode) {
			gifMode.gifModeOn = true;
			logwrap(fputs("GIF mode turned on\n", logfile));
		}
		ui.gifModeOn = gifMode.gifModeOn;
		onGifModeBlackBackgroundChanged();
	}
	if (!gifMode.modDisabled && (keyboard.gotPressed(settings.gifModeToggleBackgroundOnly) || stateChanged && ui.gifModeToggleBackgroundOnly != gifMode.gifModeToggleBackgroundOnly)) {
		if (gifMode.gifModeToggleBackgroundOnly == true) {
			gifMode.gifModeToggleBackgroundOnly = false;
			logwrap(fputs("GIF mode (darken background only) turned off\n", logfile));
		}
		else if (trainingMode) {
			gifMode.gifModeToggleBackgroundOnly = true;
			logwrap(fputs("GIF mode (darken background only) turned on\n", logfile));
		}
		ui.gifModeToggleBackgroundOnly = gifMode.gifModeToggleBackgroundOnly;
		onGifModeBlackBackgroundChanged();
	}
	if (!gifMode.modDisabled && keyboard.gotPressed(settings.togglePostEffectOnOff)) {
		BOOL theValue = !game.postEffectOn();
		game.postEffectOn() = theValue;
		if (theValue) {
			logwrap(fputs("Post Effect turned on\n", logfile));
		}
		else {
			logwrap(fputs("Post Effect turned off\n", logfile));
		}
	}
	if (!gifMode.modDisabled && (keyboard.gotPressed(settings.gifModeToggleCameraCenterOnly) || stateChanged && ui.gifModeToggleCameraCenterOnly != gifMode.gifModeToggleCameraCenterOnly)) {
		if (gifMode.gifModeToggleCameraCenterOnly == true) {
			gifMode.gifModeToggleCameraCenterOnly = false;
			logwrap(fputs("GIF mode (center camera only) turned off\n", logfile));
		}
		else if (trainingMode) {
			gifMode.gifModeToggleCameraCenterOnly = true;
			logwrap(fputs("GIF mode (center camera only) turned on\n", logfile));
		}
		ui.gifModeToggleCameraCenterOnly = gifMode.gifModeToggleCameraCenterOnly;
	}
	if (!gifMode.modDisabled && (keyboard.gotPressed(settings.toggleCameraCenterOpponent) || stateChanged && ui.toggleCameraCenterOpponent != gifMode.toggleCameraCenterOpponent)) {
		if (gifMode.toggleCameraCenterOpponent == true) {
			gifMode.toggleCameraCenterOpponent = false;
			logwrap(fputs("Center camera on opponent turned off\n", logfile));
		}
		else if (trainingMode) {
			gifMode.toggleCameraCenterOpponent = true;
			logwrap(fputs("Center camera on opponent turned on\n", logfile));
		}
		ui.toggleCameraCenterOpponent = gifMode.toggleCameraCenterOpponent;
	}
	if (!gifMode.modDisabled && (keyboard.gotPressed(settings.gifModeToggleHideOpponentOnly) || stateChanged && ui.gifModeToggleHideOpponentOnly != gifMode.gifModeToggleHideOpponentOnly)) {
		if (gifMode.gifModeToggleHideOpponentOnly == true) {
			gifMode.gifModeToggleHideOpponentOnly = false;
			logwrap(fputs("GIF mode (hide opponent only) turned off\n", logfile));
			needToRunNoGravGifMode = true;
		}
		else if (trainingMode) {
			gifMode.gifModeToggleHideOpponentOnly = true;
			logwrap(fputs("GIF mode (hide opponent only) turned on\n", logfile));
		}
		ui.gifModeToggleHideOpponentOnly = gifMode.gifModeToggleHideOpponentOnly;
	}
	if (!gifMode.modDisabled && (keyboard.gotPressed(settings.toggleHidePlayer) || stateChanged && ui.toggleHidePlayer != gifMode.toggleHidePlayer)) {
		if (gifMode.toggleHidePlayer == true) {
			gifMode.toggleHidePlayer = false;
			logwrap(fputs("Hide player turned off\n", logfile));
			needToRunNoGravGifMode = true;
		}
		else if (trainingMode) {
			gifMode.toggleHidePlayer = true;
			logwrap(fputs("Hide player turned on\n", logfile));
		}
		ui.toggleHidePlayer = gifMode.toggleHidePlayer;
	}
	if (!gifMode.modDisabled && (keyboard.gotPressed(settings.gifModeToggleHudOnly) || stateChanged && ui.gifModeToggleHudOnly != gifMode.gifModeToggleHudOnly)) {
		if (gifMode.gifModeToggleHudOnly == true) {
			gifMode.gifModeToggleHudOnly = false;
			logwrap(fputs("GIF mode (hide hud only) turned off\n", logfile));
		}
		else if (trainingMode) {
			gifMode.gifModeToggleHudOnly = true;
			logwrap(fputs("GIF mode (hide hud only) turned on\n", logfile));
		}
		ui.gifModeToggleHudOnly = gifMode.gifModeToggleHudOnly;
	}
	if (!gifMode.modDisabled && (keyboard.gotPressed(settings.noGravityToggle) || stateChanged && ui.noGravityOn != gifMode.noGravityOn)) {
		if (gifMode.noGravityOn == true) {
			gifMode.noGravityOn = false;
			logwrap(fputs("No gravity mode turned off\n", logfile));
		}
		else if (trainingMode) {
			gifMode.noGravityOn = true;
			logwrap(fputs("No gravity mode turned on\n", logfile));
		}
		ui.noGravityOn = gifMode.noGravityOn;
	}
	if (!gifMode.modDisabled && (keyboard.gotPressed(settings.freezeGameToggle) || stateChanged && ui.freezeGame != freezeGame)) {
		if (freezeGame == true) {
			freezeGame = false;
			logwrap(fputs("Freeze game turned off\n", logfile));
		}
		else if (trainingMode) {
			freezeGame = true;
			logwrap(fputs("Freeze game turned on\n", logfile));
		}
		ui.freezeGame = freezeGame;
	}
	if (!gifMode.modDisabled && (keyboard.gotPressed(settings.slowmoGameToggle) || stateChanged && ui.slowmoGame != game.slowmoGame)) {
		if (game.slowmoGame == true) {
			game.slowmoGame = false;
			logwrap(fputs("Slowmo game turned off\n", logfile));
		}
		else if (trainingMode) {
			game.slowmoGame = true;
			logwrap(fputs("Slowmo game turned on\n", logfile));
		}
		ui.slowmoGame = game.slowmoGame;
	}
	if (keyboard.gotPressed(settings.disableModKeyCombo)) {
		if (gifMode.modDisabled == true) {
			gifMode.modDisabled = false;
			logwrap(fputs("Mod enabled\n", logfile));
		} else {
			gifMode.modDisabled = true;
			logwrap(fputs("Mod disabled\n", logfile));
			needToRunNoGravGifMode = needToRunNoGravGifMode || (*aswEngine != nullptr);
		}
	}
	if (!gifMode.modDisabled && (keyboard.gotPressed(settings.disableHitboxDisplayToggle))) {
		if (settings.dontShowBoxes == true) {
			settings.dontShowBoxes = false;
			logwrap(fputs("Hitbox display enabled\n", logfile));
		} else {
			settings.dontShowBoxes = true;
			logwrap(fputs("Hitbox display disabled\n", logfile));
		}
		needWriteSettings = true;
	}
	if (!gifMode.modDisabled && keyboard.gotPressed(settings.modWindowVisibilityToggle)) {
		if (ui.visible == true) {
			ui.visible = false;
			logwrap(fputs("UI display disabled\n", logfile));
		} else {
			ui.visible = true;
			logwrap(fputs("UI display enabled\n", logfile));
		}
	}
	if (!gifMode.modDisabled && keyboard.gotPressed(settings.framebarVisibilityToggle)) {
		if (settings.showFramebar == true) {
			settings.showFramebar = false;
			logwrap(fputs("Framebar display disabled\n", logfile));
		} else {
			settings.showFramebar = true;
			logwrap(fputs("Framebar display enabled\n", logfile));
		}
		needWriteSettings = true;
	}
	if (!gifMode.modDisabled && (keyboard.gotPressed(settings.toggleDisableGrayHurtboxes))) {
		if (settings.neverDisplayGrayHurtboxes == true) {
			settings.neverDisplayGrayHurtboxes = false;
			logwrap(fputs("Enabling gray hurtboxes\n", logfile));
		} else {
			settings.neverDisplayGrayHurtboxes = true;
			logwrap(fputs("Disabling gray hurtboxes\n", logfile));
		}
		needWriteSettings = true;
	}
	if (!gifMode.modDisabled && (keyboard.gotPressed(settings.toggleNeverIgnoreHitstop))) {
		if (settings.neverIgnoreHitstop == true) {
			settings.neverIgnoreHitstop = false;
			logwrap(fputs("Never ignore hitstop = false\n", logfile));
		} else {
			settings.neverIgnoreHitstop = true;
			logwrap(fputs("Never ignore hitstop = true\n", logfile));
		}
		needWriteSettings = true;
	}
	if (!gifMode.modDisabled && (keyboard.gotPressed(settings.toggleShowInputHistory))) {
		if (gifMode.showInputHistory == true) {
			gifMode.showInputHistory = false;
			logwrap(fputs("Show input history = false\n", logfile));
		} else {
			gifMode.showInputHistory = true;
			logwrap(fputs("Show input history = true\n", logfile));
		}
	}
	if (!gifMode.modDisabled && (keyboard.gotPressed(settings.continuousScreenshotToggle) || stateChanged && ui.continuousScreenshotToggle != continuousScreenshotMode)) {
		if (continuousScreenshotMode) {
			continuousScreenshotMode = false;
			logwrap(fputs("Continuous screenshot mode off\n", logfile));
		} else if (trainingMode) {
			continuousScreenshotMode = true;
			logwrap(fputs("Continuous screenshot mode on\n", logfile));
		}
		ui.continuousScreenshotToggle = continuousScreenshotMode;
	}
	if (!gifMode.modDisabled && keyboard.gotPressed(settings.screenshotBtn)) {
		ui.takeScreenshotPress = true;
		ui.takeScreenshotTimer = 10;
	}
	if (!gifMode.modDisabled) {
		for (int i = 0; i < 2; ++i) {
			if (ui.clearTensionGainMaxCombo[i]) {
				ui.clearTensionGainMaxCombo[i] = false;
				players[i].tensionGainMaxCombo = 0;
				players[i].burstGainMaxCombo = 0;
				players[i].tensionGainLastCombo = 0;
				players[i].burstGainLastCombo = 0;
			}
		}
	}
	if (needWriteSettings && keyboard.thisProcessWindow) {
		PostMessageW(keyboard.thisProcessWindow, WM_APP_UI_STATE_CHANGED, FALSE, TRUE);
	}
}

// Runs on the main thread. Called once every frame when a match is running, including freeze mode or when Pause menu is open
void EndScene::actUponKeyStrokesThatAlreadyHappened() {
	bool trainingMode = game.isTrainingMode();
	bool allowNextFrameIsHeld = false;
	if (!gifMode.modDisabled) {
		allowNextFrameIsHeld = keyboard.isHeld(settings.allowNextFrameKeyCombo);
	}
	if (ui.allowNextFrame && !gifMode.modDisabled) {
		allowNextFrameIsHeld = true;
		ui.allowNextFrame = false;
	}
	if (allowNextFrameIsHeld) {
		if (!freezeGame || allowNextFrameBeenHeldFor == -1) {
			allowNextFrameBeenHeldFor = -1;
		} else {
			bool allowPress = false;
			if (allowNextFrameBeenHeldFor == 0) {
				allowPress = true;
			} else if (allowNextFrameBeenHeldFor >= 40) {
				allowNextFrameBeenHeldFor = 40;
				++allowNextFrameCounter;
				if (allowNextFrameCounter >= 10) {
					allowPress = true;
					allowNextFrameCounter = 0;
				}
			}
			if (trainingMode && allowPress) {
				game.allowNextFrame = true;
				logwrap(fputs("allowNextFrame set to true\n", logfile));
			}
			++allowNextFrameBeenHeldFor;
		}
	} else {
		allowNextFrameBeenHeldFor = 0;
		allowNextFrameCounter = 0;
	}
	drawDataPrepared.needTakeScreenshot = false;
	if (!gifMode.modDisabled && ui.takeScreenshotPress) {
		ui.takeScreenshotPress = false;
		drawDataPrepared.needTakeScreenshot = true;
	}
	bool screenshotPathEmpty = false;
	if (settings.ignoreScreenshotPathAndSaveToClipboard) {
		screenshotPathEmpty = true;
	} else {
		std::unique_lock<std::mutex> guard(settings.screenshotPathMutex);
		screenshotPathEmpty = settings.screenshotPath.empty();
	}
	needContinuouslyTakeScreens = false;
	if (!gifMode.modDisabled
			&& ((keyboard.isHeld(settings.screenshotBtn) || ui.takeScreenshot) && settings.allowContinuousScreenshotting || continuousScreenshotMode)
			&& trainingMode
			&& !screenshotPathEmpty) {
		needContinuouslyTakeScreens = true;
	}
	game.freezeGame = freezeGame && trainingMode && !gifMode.modDisabled;
	if (!trainingMode || gifMode.modDisabled) {
		gifMode.gifModeOn = false;
		ui.gifModeOn = false;
		gifMode.noGravityOn = false;
		ui.noGravityOn = false;
		game.slowmoGame = false;
		ui.slowmoGame = false;
		gifMode.gifModeToggleBackgroundOnly = false;
		onGifModeBlackBackgroundChanged();
		ui.gifModeToggleBackgroundOnly = false;
		gifMode.gifModeToggleCameraCenterOnly = false;
		ui.gifModeToggleCameraCenterOnly = false;
		ui.toggleCameraCenterOpponent = false;
		gifMode.gifModeToggleHideOpponentOnly = false;
		ui.gifModeToggleHideOpponentOnly = false;
		ui.toggleHidePlayer = false;
		gifMode.gifModeToggleHudOnly = false;
		ui.gifModeToggleHudOnly = false;
		clearContinuousScreenshotMode();
	}
	if (needToRunNoGravGifMode) {
		entityList.populate();
		noGravGifMode();
	}
	needToRunNoGravGifMode = false;
	
}

static bool getNeedHide(const int* ar, const bool* arHideEffects, int intToFind, bool* needHideEffects) {
	for (int i = 0; i < 2; ++i) {
		if (ar[i] == intToFind) {
			*needHideEffects = arHideEffects[i];
			return true;
		}
	}
	return false;
}

// Runs on the main thread
void EndScene::noGravGifMode() {
	char playerIndex;
	playerIndex = game.getPlayerSide();
	if (playerIndex == 2) playerIndex = 0;
	
	int teamsToHide[2] { -1, -1 };
	bool hideEffects[2] { true, true };
	
	if ((gifMode.gifModeOn || gifMode.gifModeToggleHideOpponentOnly) && game.isTrainingMode()) {
		teamsToHide[0] = 1 - playerIndex;
		hideEffects[0] = !gifMode.dontHideOpponentsEffects;
	}
	if (gifMode.toggleHidePlayer && game.isTrainingMode()) {
		teamsToHide[1] = playerIndex;
		hideEffects[1] = !gifMode.dontHidePlayersEffects;
	}
	
	for (auto it = hiddenEntities.begin(); it != hiddenEntities.end(); ++it) {
		it->wasFoundOnThisFrame = false;
	}
	for (int i = 0; i < entityList.count; ++i) {
		Entity ent = entityList.list[i];
		
		bool needHideEffects = false;
		if (getNeedHide(teamsToHide, hideEffects, ent.team(), &needHideEffects) && (needHideEffects || ent.isPawn())) {
			hideEntity(ent);
		}
	}
	
	for (int i = 0; i < entityList.count; ++i) {
		Entity ent = entityList.list[i];
		auto found = findHiddenEntity(ent);
		if (found == hiddenEntities.end()) {
			continue;
		}
		bool needHideEffects = false;
		if (getNeedHide(teamsToHide, hideEffects, ent.team(), &needHideEffects) && (needHideEffects || ent.isPawn())) {
			continue;
		}
		const int currentScaleX = ent.scaleX();
		const int currentScaleY = ent.scaleY();
		const int currentScaleZ = ent.scaleZ();
		const int currentScaleDefault = ent.scaleDefault();
		const int currentScaleForParticles = ent.scaleForParticles();

		if (currentScaleX == 0) {
			ent.scaleX() = found->scaleX;
		}
		if (currentScaleY == 0) {
			ent.scaleY() = found->scaleY;
		}
		if (currentScaleZ == 0) {
			ent.scaleZ() = found->scaleZ;
		}
		if (currentScaleDefault == 0) {
			ent.scaleDefault() = found->scaleDefault;
			ent.scaleDefault2() = found->scaleDefault;
		}
		if (currentScaleForParticles == 0) {
			ent.scaleForParticles() = found->scaleForParticles;
		}
		hiddenEntities.erase(found);
	}
	
	auto it = hiddenEntities.begin();
	while (it != hiddenEntities.end()) {
		if (!it->wasFoundOnThisFrame) {
			it = hiddenEntities.erase(it);
		}
		else {
			++it;
		}
	}

	bool useNoGravMode = gifMode.noGravityOn && game.isTrainingMode();
	if (useNoGravMode) {
		entityList.slots[playerIndex].speedY() = 0;
	}
}

// Runs on the main thread
void EndScene::clearContinuousScreenshotMode() {
	continuousScreenshotMode = false;
	ui.continuousScreenshotToggle = false;
	previousTimeOfTakingScreen = ~0;
}

// Runs on the main thread
std::vector<EndScene::HiddenEntity>::iterator EndScene::findHiddenEntity(const Entity& ent) {
	for (auto it = hiddenEntities.begin(); it != hiddenEntities.end(); ++it) {
		if (it->ent == ent) {
			return it;
		}
	}
	return hiddenEntities.end();
}

// Runs on the main thread
LRESULT CALLBACK hook_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	return endScene.WndProcHook(hWnd, message, wParam, lParam);
}

// The game logic thread runs WndProc, by calling DispatchMessageW inbetween ticks.
// Runs on the main thread
LRESULT EndScene::WndProcHook(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	if (!shutdown) {
		if (ui.WndProc(hWnd, message, wParam, lParam)) {
			return TRUE;
		}
		
		if (message == WM_DESTROY) {
			keyboard.thisProcessWindow = NULL;
		}
		
		if (message == WM_APP_SETTINGS_FILE_UPDATED) {
			settings.readSettings(true);
		}
		
		if (message == WM_KEYDOWN
				|| message == WM_KEYUP
				|| message == WM_SYSKEYDOWN
				|| message == WM_SYSKEYUP
				|| message == WM_APP_UI_STATE_CHANGED && wParam && ui.stateChanged) {
			processKeyStrokes();
		}
		if (message == WM_APP_TURN_OFF_POST_EFFECT_SETTING_CHANGED) {
			onGifModeBlackBackgroundChanged();
		}
	}
	
	LRESULT result = orig_WndProc(hWnd, message, wParam, lParam);
	return result;
}

// Training HUD here means:
// 1) The texts displaying hit dmg, combo dmg, max combo dmg.
// 2) Input history
// 3) Virtual sticks
// 4) Dummy status
// All drawn in this one function depending on in-game settings. This function hooks that.
// Runs on the main thread
void EndScene::drawTrainingHudHook(char* thisArg) {
	if (!shutdown && !graphics.shutdown) {
		if (gifMode.gifModeToggleHudOnly || gifMode.gifModeOn) return;
	}
	orig_drawTrainingHud(thisArg);
	if (!shutdown && !graphics.shutdown && !iGiveUp) {
		drawTexts();
	}
}

// See drawTrainingHudHook
// Runs on the main thread
void EndScene::drawTexts() {
	return;
	#if 0
	// Example code
	{
		char HelloWorld[] = "oig^mAtk1;";
		DrawTextWithIconsParams s;
		s.field159_0x100 = 36.0;
		s.layer = 177;
		s.field160_0x104 = -1.0;
		s.field4_0x10 = -1.0;
		s.field155_0xf0 = 1;
		s.field157_0xf8 = -1;
		s.field158_0xfc = -1;
		s.field161_0x108 = 0;
		s.field162_0x10c = 0;
		s.field163_0x110 = -1;
		s.field164_0x114 = 0;
		s.field165_0x118 = 0;
		s.field166_0x11c = -1;
		s.outlineColor = 0xff000000;
		s.flags2 = 0xff000000;
		s.x = 100;
		s.y = 185.0 + 34 * 3;
		s.alignment = ALIGN_LEFT;
		s.text = HelloWorld;
		s.field156_0xf4 = 0x210;
		drawTextWithIcons(&s,0x0,1,4,0,0);
	}
	return;
	{
		char HelloWorld[] = "-123765";
		DrawTextWithIconsParams s;
		s.field159_0x100 = 36.0;
		s.layer = 177;
		s.field160_0x104 = -1.0;
		s.field4_0x10 = -1.0;
		s.field155_0xf0 = 1;
		s.field157_0xf8 = -1;
		s.field158_0xfc = -1;
		s.field161_0x108 = 0;
		s.field162_0x10c = 0;
		s.field163_0x110 = -1;
		s.field164_0x114 = 0;
		s.field165_0x118 = 0;
		s.field166_0x11c = -1;
		s.outlineColor = 0xff000000;
		s.flags2 = 0xff000000;
		s.x = 460;
		s.y = 185.0 + 34 * 3;
		s.alignment = ALIGN_CENTER;
		s.text = HelloWorld;
		s.field156_0xf4 = 0x210;
		drawTextWithIcons(&s,0x0,1,4,0,0);
	}
	#endif
}

// Called when switching characters, exiting the match.
// Runs on the main thread
void EndScene::onAswEngineDestroyed() {
	prevAswEngineTickCount = 0xffffffff;
	prevAswEngineTickCountForInputs = 0xffffffff;
	drawDataPrepared.clear();
	clearContinuousScreenshotMode();
	registeredHits.clear();
	needFrameCleanup = false;
	creatingObject = false;
	moves.onAswEngineDestroyed();
	if (!iGiveUp) {
		measuringFrameAdvantage = false;
		measuringLandingFrameAdvantage = -1;
		memset(tensionGainOnLastHit, 0, sizeof tensionGainOnLastHit);
		memset(burstGainOnLastHit, 0, sizeof burstGainOnLastHit);
		memset(tensionGainOnLastHitUpdated, 0, sizeof tensionGainOnLastHitUpdated);
		memset(burstGainOnLastHitUpdated, 0, sizeof burstGainOnLastHitUpdated);
		for (int i = 0; i < 2; ++i) {
			players[i].clear();
		}
		projectiles.clear();
		sendSignalStack.clear();
		events.clear();
		playerFramebars.clear();
		projectileFramebars.clear();
		combinedFramebars.clear();
		superfreezeHasBeenGoingFor = 0;
		lastNonZeroSuperflashInstigator = nullptr;
		superfreezeHasBeenGoingFor = 0;
		superflashCounterAllied = 0;
		superflashCounterAlliedMax = 0;
		superflashCounterOpponent = 0;
		superflashCounterOpponentMax = 0;
		baikenBlockCancels.clear();
	}
	memset(prevInputRingBuffers, 0, sizeof prevInputRingBuffers);
	for (int i = 0; i < 2; ++i) {
		inputRingBuffersStored[i].clear();
	}
}

// When someone YRCs, PRCs, RRCs or does a super, the address of their entity is written into the
// *aswEngine + superflashInstigatorOffset variable for the duration of the superfreeze.
// Runs on the main thread
Entity EndScene::getSuperflashInstigator() {
	if (!superflashInstigatorOffset || !*aswEngine) return nullptr;
	return Entity{ *(char**)(*aswEngine + superflashInstigatorOffset) };
}

// Runs on the main thread
int EndScene::getSuperflashCounterOpponent() {
	if (!superflashCounterOpponentOffset || !*aswEngine) return 0;
	return *(int*)(*aswEngine + superflashCounterOpponentOffset);
}

// Runs on the main thread
int EndScene::getSuperflashCounterAllied() {
	if (!superflashCounterAlliedOffset || !*aswEngine) return 0;
	return *(int*)(*aswEngine + superflashCounterAlliedOffset);
}

// Runs on the main thread
void EndScene::restartMeasuringFrameAdvantage(int index) {
	PlayerInfo& player = players[index];
	PlayerInfo& other = players[1 - index];
	player.frameAdvantageValid = false;
	other.frameAdvantageValid = false;
	if (other.idlePlus) {
		player.frameAdvantage = -other.timePassed;
		other.frameAdvantage = other.timePassed;
		player.frameAdvantageNoPreBlockstun = 0;
		other.frameAdvantageNoPreBlockstun = 0;
	}
	player.frameAdvantageIncludesIdlenessInNewSection = false;
	other.frameAdvantageIncludesIdlenessInNewSection = false;
	measuringFrameAdvantage = true;
}

// Runs on the main thread
void EndScene::restartMeasuringLandingFrameAdvantage(int index) {
	PlayerInfo& player = players[index];
	PlayerInfo& other = players[1 - index];
	player.landingFrameAdvantageValid = false;
	other.landingFrameAdvantageValid = false;
	if (other.idleLanding) {
		player.landingFrameAdvantage = -other.timePassedLanding;
		other.landingFrameAdvantage = other.timePassedLanding;
		player.landingFrameAdvantageNoPreBlockstun = 0;
		other.landingFrameAdvantageNoPreBlockstun = 0;
	}
	player.landingFrameAdvantageIncludesIdlenessInNewSection = false;
	other.landingFrameAdvantageIncludesIdlenessInNewSection = false;
	measuringLandingFrameAdvantage = 1 - index;
}

// There're three hit detection calls: with hitDetectionType 0, 1 and 2, called
// one right after the other each logic tick.
// 0 - easy clash only (see easyClash in bbscript)
// 1 - regular hitbox vs hurtbox interaction
// 2 - clash only (excluding easy clash)
// A single hit detection does a loop in a loop and tries to find which two entities hit each other.
// We use this hook at the start of hit detection algorithm to measure some values before a hit.
// Runs on the main thread
void EndScene::onHitDetectionStart(int hitDetectionType) {
	if (hitDetectionType == 0) {
		if (!iGiveUp) {
			entityList.populate();
			for (int i = 0; i < 2; ++i) {
				Entity ent = entityList.slots[i];
				tensionRecordedHit[i] = ent.tension();
				burstRecordedHit[i] = game.getBurst(i);
				tensionGainOnLastHit[i] = 0;
				burstGainOnLastHit[i] = 0;
				tensionGainOnLastHitUpdated[i] = false;
				burstGainOnLastHitUpdated[i] = false;
			}
		}
		registeredHits.clear();
	}
}

// We use this hook at the end of hit detection algorithm to measure some values after a hit.
// Runs on the main thread
void EndScene::onHitDetectionEnd(int hitDetectionType) {
	if (iGiveUp) return;
	entityList.populate();
	for (int i = 0; i < 2; ++i) {
		Entity ent = entityList.slots[i];
		int tension = ent.tension();
		int tensionBefore = tensionRecordedHit[i];
		if (tension != tensionBefore) {
			tensionGainOnLastHit[i] += tension - tensionBefore;
			tensionGainOnLastHitUpdated[i] = true;
		}
		tensionRecordedHit[i] = tension;
		int burst = game.getBurst(i);
		int burstBefore = burstRecordedHit[i];
		if (burst != burstBefore) {
			burstGainOnLastHit[i] += burst - burstBefore;
			burstGainOnLastHitUpdated[i] = true;
		}
		burstRecordedHit[i] = burst;
	}
	if (hitDetectionType == 0 || hitDetectionType == 2) {
		for (int i = 2; i < entityList.count; ++i) {
			Entity ent { entityList.list[i] };
			if (!ent.isActive() || ent.isPawn() || !ent.receivedProjectileClashSignal()) continue;
			onProjectileHit(ent);
		}
	}
}

// Runs on the main thread
void EndScene::onProjectileHit(Entity ent) {
	ProjectileInfo& projectile = findProjectile(ent);
	if (projectile.ptr) {
		projectile.fill(ent, getSuperflashInstigator(), false);
		projectile.hitstop = ent.hitstop() + ent.clashHitstop();
		projectile.landedHit = true;
		projectile.markActive = true;
	}
}

// Called by a hook inside hit detection when a hit was detected.
// Runs on the main thread
void EndScene::registerHit(HitResult hitResult, bool hasHitbox, Entity attacker, Entity defender) {
	registeredHits.emplace_back();
	RegisteredHit& hit = registeredHits.back();
	if (!iGiveUp) {
		if (attacker.isPawn()) {
			hit.projectile.fill(attacker, getSuperflashInstigator(), false);
		}
		hit.isPawn = attacker.isPawn();
		hit.hitResult = hitResult;
		hit.hasHitbox = hasHitbox;
	}
	hit.attacker = attacker;
	if (!iGiveUp) {
		hit.defender = defender;
		if (defender.isPawn()) {
			PlayerInfo& defenderPlayer = findPlayer(defender);
			defenderPlayer.xStunDisplay = PlayerInfo::XSTUN_DISPLAY_NONE;
			if (hitResult == HIT_RESULT_ARMORED || hitResult == HIT_RESULT_ARMORED_BUT_NO_DMG_REDUCTION) {
				defenderPlayer.armoredHitOnThisFrame = true;
			} else if (hitResult == HIT_RESULT_NORMAL || hitResult == HIT_RESULT_BLOCKED) {
				defenderPlayer.gotHitOnThisFrame = true;
			}
		} else {
			ProjectileInfo& defendingProjectile = findProjectile(defender);
			if (defendingProjectile.ptr) {
				defendingProjectile.gotHitOnThisFrame = true;
			}
		}
		if (attacker.isPawn() && hasHitbox) {
			PlayerInfo& attackerPlayer = findPlayer(attacker);
			attackerPlayer.hitSomething = true;
		}
		if (hasHitbox) {
			onProjectileHit(attacker);
		}
	}
}

// Called at the start of an UE3 engine tick.
// This tick runs even when paused or not in a match.
// Runs on the main thread
void EndScene::onUWorld_TickBegin() {
	logicThreadId = GetCurrentThreadId();
	drewExGaugeHud = false;
	camera.grabbedValues = false;
	iGiveUp = game.getGameMode() == GAME_MODE_NETWORK && game.getPlayerSide() != 2;
}

// Called at the end of an UE3 engine tick.
// This tick runs even when paused or not in a match.
// Runs on the main thread
void EndScene::onUWorld_Tick() {
	if (shutdown) {
		if (*aswEngine) {
			bool needToCallNoGravGifMode = gifMode.gifModeOn
				|| gifMode.gifModeToggleHideOpponentOnly
				|| gifMode.noGravityOn;
			gifMode.gifModeOn = false;
			gifMode.noGravityOn = false;
			gifMode.gifModeToggleHideOpponentOnly = false;
			onGifModeBlackBackgroundChanged();
			gifMode.toggleHidePlayer = false;
			if (needToCallNoGravGifMode && game.isTrainingMode()) {
				entityList.populate();
				noGravGifMode();
			}
		}
		hud.onDllDetach();
		ui.onDllDetachNonGraphics();
		SetEvent(shutdownFinishedEvent);
		detouring.detachAll(false);
		detouring.cancelTransaction();
	}
}

// Runs on the main thread
void EndScene::HookHelp::BBScr_createObjectWithArgHook(const char* animName, unsigned int posType) {
	static bool insideObjectCreation = false;
	bool needUnset = !insideObjectCreation;
	if (!endScene.shutdown) {
		insideObjectCreation = true;
		endScene.creatingObject = true;
		endScene.createdObjectAnim = animName;
		endScene.creatorOfCreatedObject = Entity{(char*)this};
	}
	endScene.orig_BBScr_createObjectWithArg(this, animName, posType);
	if (!endScene.shutdown) {
		if (needUnset) {
			insideObjectCreation = false;
			endScene.creatingObject = false;
		}
		BBScr_createObjectHook_piece();
	}
}

// Runs on the main thread
void EndScene::HookHelp::BBScr_createObjectHook(const char* animName, unsigned int posType) {
	static bool insideObjectCreation = false;
	bool needUnset = !insideObjectCreation;
	if (!endScene.shutdown) {
		insideObjectCreation = true;
		endScene.creatingObject = true;
		endScene.createdObjectAnim = animName;
		endScene.creatorOfCreatedObject = Entity{(char*)this};
	}
	endScene.orig_BBScr_createObject(this, animName, posType);
	if (!endScene.shutdown) {
		if (needUnset) {
			insideObjectCreation = false;
			endScene.creatingObject = false;
		}
		BBScr_createObjectHook_piece();
	}
}

// Runs on the main thread
void EndScene::HookHelp::BBScr_createObjectHook_piece() {
	if (!gifMode.modDisabled && game.isTrainingMode()) {
		int playerSide = game.getPlayerSide();
		if (playerSide == 2) playerSide = 0;
		Entity createdPawn = Entity{(char*)this}.previousEntity();
		if (createdPawn
				&& (
					(gifMode.gifModeToggleHideOpponentOnly || gifMode.gifModeOn) && (!gifMode.dontHideOpponentsEffects || createdPawn.isPawn())
					&& createdPawn.team() != playerSide
					|| gifMode.toggleHidePlayer && (!gifMode.dontHidePlayersEffects || createdPawn.isPawn())
					&& createdPawn.team() == playerSide
				)) {
			endScene.hideEntity(createdPawn);
		}
	}
}

// Runs on the main thread
void EndScene::onObjectCreated(Entity pawn, Entity createdPawn, const char* animName) {
	for (auto it = projectiles.begin(); it != projectiles.end(); ++it) {
		if (it->ptr == createdPawn) {
			if (it->landedHit || it->gotHitOnThisFrame) {
				it->ptr = nullptr;
			} else {
				projectiles.erase(it);
			}
			break;
		}
	}
	projectiles.emplace_back();
	ProjectileInfo& projectile = projectiles.back();
	projectile.fill(createdPawn, getSuperflashInstigator(), true);
	bool ownerFound = false;
	ProjectileInfo& creatorProjectile = findProjectile(pawn);
	if (creatorProjectile.ptr) {
		projectile.creationTime_aswEngineTick = creatorProjectile.creationTime_aswEngineTick;
		projectile.startup = creatorProjectile.total;
		memcpy(projectile.creatorName, creatorProjectile.ptr.animationName(), 32);
		projectile.creator = creatorProjectile.ptr;
		projectile.total = creatorProjectile.total;
		projectile.disabled = creatorProjectile.disabled;
		ownerFound = true;
	}
	if (!ownerFound && pawn.isPawn()) {
		entityList.populate();
		PlayerInfo& player = findPlayer(pawn);
		projectile.creationTime_aswEngineTick = getAswEngineTick();
		projectile.startup = player.total + player.prevStartups.total();  // + prevStartups.total() needed for Venom QV
		if (player.totalCanBlock > player.total) {  // needed for Answer Taunt
			projectile.startup += player.totalCanBlock - player.total;
		}
		projectile.total = projectile.startup;
		memcpy(projectile.creatorName, player.anim, 32);
		projectile.creator = player.pawn;
	}
}

// Runs on the main thread
void EndScene::hideEntity(Entity ent) {
	const int currentScaleX = ent.scaleX();
	const int currentScaleY = ent.scaleY();
	const int currentScaleZ = ent.scaleZ();
	const int currentScaleDefault = ent.scaleDefault();  // 0x2664 is another default scaling
	const int currentScaleForParticles = ent.scaleForParticles();  // 0x2618 is another default scaling used for created particles

	auto found = findHiddenEntity(ent);
	if (found == hiddenEntities.end()) {
		hiddenEntities.emplace_back();
		HiddenEntity& hiddenEntity = hiddenEntities.back();
		hiddenEntity.ent = ent;
		hiddenEntity.scaleX = currentScaleX;
		hiddenEntity.scaleY = currentScaleY;
		hiddenEntity.scaleZ = currentScaleZ;
		hiddenEntity.scaleDefault = currentScaleDefault;
		hiddenEntity.scaleForParticles = currentScaleForParticles;
		hiddenEntity.wasFoundOnThisFrame = true;
	} else {
		HiddenEntity& hiddenEntity = *found;
		if (currentScaleX != 0) {
			hiddenEntity.scaleX = currentScaleX;
		}
		if (currentScaleY != 0) {
			hiddenEntity.scaleY = currentScaleY;
		}
		if (currentScaleZ != 0) {
			hiddenEntity.scaleZ = currentScaleZ;
		}
		if (currentScaleDefault != 0) {
			hiddenEntity.scaleDefault = currentScaleDefault;
		}
		if (currentScaleForParticles != 0) {
			hiddenEntity.scaleForParticles = currentScaleForParticles;
		}
		hiddenEntity.wasFoundOnThisFrame = true;
	}
	ent.scaleX() = 0;
	ent.scaleY() = 0;
	ent.scaleZ() = 0;
	ent.scaleDefault() = 0;
	ent.scaleDefault2() = 0;
	ent.scaleForParticles() = 0;
}

// Runs on the main thread
bool EndScene::didHit(Entity attacker) {
	for (auto it = registeredHits.begin(); it != registeredHits.end(); ++it) {
		if (it->attacker == attacker) {
			return true;
		}
	}
	return false;
}

// Runs on the main thread
// This hook does not work on projectiles
void EndScene::HookHelp::setAnimHook(const char* animName) {
	endScene.setAnimHook(Entity{(char*)this}, animName);
}

// Runs on the main thread
// This hook does not work on projectiles
void EndScene::setAnimHook(Entity pawn, const char* animName) {
	if (!shutdown && pawn.isPawn() && !gifMode.modDisabled && !iGiveUp) {
		PlayerInfo& player = findPlayer(pawn);
		if (player.pawn) {  // The hook may run on match load before we initialize our pawns,
			                // because we only do our pawn initialization after the end of the logic tick
			player.lastIgnoredHitNum = -1;
			player.changedAnimOnThisFrame = true;
			static MoveInfo moveJustSitsThere;
			bool moveIsFound = moves.getInfo(moveJustSitsThere, player.charType, player.moveNameIntraFrame, player.animIntraFrame, false);
			memcpy(player.animIntraFrame, animName, 32);
			player.setMoveName(player.moveNameIntraFrame, pawn);
			if (moveJustSitsThere.isIdle(player)) player.wasIdle = true;
			events.emplace_back();
			OccuredEvent& event = events.back();
			event.type = OccuredEvent::SET_ANIM;
			event.u.setAnim.pawn = pawn;
			memcpy(event.u.setAnim.fromAnim, pawn.animationName(), 32);
		}
	}
	orig_setAnim((void*)pawn, animName);
	if (!shutdown && pawn.isPawn() && !gifMode.modDisabled && !iGiveUp) {
		PlayerInfo& player = findPlayer(pawn);
		int blockstun = pawn.blockstun();
		if (blockstun && (player.inBlockstunNextFrame || player.baikenReturningToBlockstunAfterAzami)) {
			// defender was observed to not be in hitstop at this point, but having blockstun nonetheless
			player.blockstunMax = blockstun;
			player.blockstunElapsed = 0;
			player.blockstunContaminatedByRCSlowdown = 0;
			player.blockstunMaxLandExtra = 0;
			player.setBlockstunMax = true;
		}
	}
}

// Runs on the main thread
void EndScene::initializePawn(PlayerInfo& player, Entity ent) {
	player.pawn = ent;
	player.charType = ent.characterType();
	ent.getWakeupTimings(&player.wakeupTimings);
	static MoveInfo moveJustThere;
	moves.getInfo(moveJustThere,
		player.charType,
		ent.currentMoveIndex() == -1 ? nullptr : ent.currentMove()->name,
		ent.animationName(),
		false);
	player.idle = moveJustThere.isIdle(player);
	player.weight = ent.weight();
	player.maxHp = ent.maxHp();
	player.defenseModifier = ent.defenseModifier();
	player.lastIgnoredHitNum = -1;
	player.changedAnimOnThisFrame = true;
}

// Runs on the main thread
PlayerInfo& EndScene::findPlayer(Entity ent) {
	for (int i = 0; i < 2; ++i) {
		if (players[i].pawn == ent) {
			return players[i];
		}
	}
	return emptyPlayer;
}

// Runs on the main thread
void EndScene::HookHelp::BBScr_createParticleWithArgHook(const char* animName, unsigned int posType) {
	endScene.BBScr_createParticleWithArgHook(Entity{(char*)this}, animName, posType);
}

// Runs on the main thread
void EndScene::BBScr_createParticleWithArgHook(Entity pawn, const char* animName, unsigned int posType) {
	if (!gifMode.modDisabled && game.isTrainingMode() && pawn.isPawn()) {
		int playerSide = game.getPlayerSide();
		if (playerSide == 2) playerSide = 0;
		if (
				(gifMode.gifModeToggleHideOpponentOnly || gifMode.gifModeOn) && (!gifMode.dontHideOpponentsEffects || pawn.isPawn())
				&& pawn.team() != playerSide
				|| gifMode.toggleHidePlayer && (!gifMode.dontHidePlayersEffects || pawn.isPawn())
				&& pawn.team() == playerSide
			) {
			pawn.scaleForParticles() = 0;
		}
	}
	orig_BBScr_createParticleWithArg((void*)pawn, animName, posType);
}

// Called before a logic tick happens. Doesn't run when the pause menu is open or a match is not running.
// Runs on the main thread
void EndScene::onTickActors_FDeferredTickList_FGlobalActorIteratorBegin(bool isFrozen) {
	if (!isFrozen) {
		if (needFrameCleanup) {
			frameCleanup();
		}
		needFrameCleanup = false;
	}
}

// Stuff that runs at the start of a tick
// Runs on the main thread
void EndScene::frameCleanup() {
	entityList.populate();
	for (auto it = projectiles.begin(); it != projectiles.end();) {
		ProjectileInfo& projectile = *it;
		projectile.landedHit = false;
		projectile.gotHitOnThisFrame = false;
		bool found = false;
		if (projectile.ptr) {
			for (int i = 2; i < entityList.count; ++i) {
				if (entityList.list[i] == projectile.ptr) {
					found = true;
					break;
				}
			}
		}
		if (!found) {
			it = projectiles.erase(it);
			continue;
		}
		++it;
	}
	for (PlayerInfo& player : players) {
		player.prevBbscrvar5 = player.pawn ? player.pawn.bbscrvar5() : 0;
		player.setHitstopMax = false;
		player.setHitstopMaxSuperArmor = false;
		player.setHitstunMax = false;
		player.setBlockstunMax = false;
		player.lastHitstopBeforeWipe = 0;
		player.wasIdle = false;
		player.startedDefending = false;
		player.hitSomething = false;
		player.changedAnimOnThisFrame = false;
		player.wasEnableGatlings = false;
		player.wasEnableAirtech = false;
		player.wasAttackCollidedSoCanCancelNow = false;
		player.wasEnableNormals = false;
		player.wasCanYrc = false;
		player.wasProhibitFDTimer = 1;
		player.wasAirdashHorizontallingTimer = 0;
		player.wasEnableWhiffCancels = false;
		player.wasEnableSpecials = false;
		player.wasEnableSpecialCancel = false;
		player.wasEnableJumpCancel = false;
		player.wasSuperArmorEnabled = false;
		player.wasFullInvul = false;
		player.obtainedForceDisableFlags = false;
		player.armoredHitOnThisFrame = false;
		player.gotHitOnThisFrame = false;
		player.baikenReturningToBlockstunAfterAzami = false;
		memcpy(player.animIntraFrame, player.anim, 32);
		player.wasCancels.clear();
		player.receivedNewDmgCalcOnThisFrame = false;
		player.blockedAHitOnThisFrame = false;
	}
	creatingObject = false;
	events.clear();
}

// Runs on the main thread
void EndScene::HookHelp::pawnInitializeHook(void* initializationParams) {
	endScene.pawnInitializeHook(Entity{(char*)this}, initializationParams);
}

// Runs on the main thread
void EndScene::HookHelp::handleUponHook(int signal) {
	endScene.handleUponHook(Entity{(char*)this}, signal);
}

// Runs on the main thread
// This is also for effects
void EndScene::pawnInitializeHook(Entity createdObj, void* initializationParams) { 
	if (!shutdown && creatingObject) {
		creatingObject = false;
		if (!iGiveUp) {
			onObjectCreated(creatorOfCreatedObject, createdObj, createdObjectAnim);
			events.emplace_back();
			OccuredEvent& event = events.back();
			event.type = OccuredEvent::SIGNAL;
			event.u.signal.from = creatorOfCreatedObject;
			event.u.signal.to = createdObj;
			memcpy(event.u.signal.fromAnim, creatorOfCreatedObject.animationName(), 32);
		}
	}
	endScene.orig_pawnInitialize(createdObj.ent, initializationParams);
}

// Runs on the main thread
void EndScene::handleUponHook(Entity pawn, int signal) { 
	if (!shutdown && !iGiveUp) {
		if (!sendSignalStack.empty()) {
			events.emplace_back();
			OccuredEvent& event = events.back();
			event.type = OccuredEvent::SIGNAL;
			event.u.signal.from = sendSignalStack.back();
			event.u.signal.to = pawn;
			memcpy(event.u.signal.fromAnim, sendSignalStack.back().animationName(), 32);
		}
	}
	if (!shutdown) {
		if (signal == 3  // IDLING
				&& pawn.isPawn()) {
			// Slayer's buff in PLAYERVAL_1 is checked when initiating a move, but decremented after the fact, in a FRAME_STEP handler
			// we need the original value
			PlayerInfo& player = findPlayer(pawn);
			player.wasPlayerval1Idling = pawn.playerVal(1);
			player.wasResource = pawn.exGaugeValue(0);
		}
		// Blitz Shield rejection changes super armor enabled and full invul flags at the end of a logic tick
		if (signal == 0x27  // PRE_DRAW
				&& pawn.isPawn()) {
			PlayerInfo& player = findPlayer(pawn);
			player.wasSuperArmorEnabled = pawn.superArmorEnabled();
			player.wasFullInvul = pawn.fullInvul();
			
			// I put this here of all places because the other places are disabled by iGiveUp, and I want this to always work no matter what.
			// Using this approach we pull the value from the animation start, even if rollback happened and the animation start was skipped,
			// we never saw it on the screen
			if (pawn.characterType() == CHARACTER_TYPE_RAMLETHAL) {
				Entity p = pawn.stackEntity(0);
				if (p && p.isActive()) {
					const char* animName = p.animationName();
					if (
							p.currentAnimDuration() == 1
							&& (
								strcmp(animName, "BitN6C") == 0
								|| strcmp(animName, "BitN2C_Bunri") == 0
							)
					) {
						player.ramlethalBitNStartPos = p.posX();
					}
				}
				p = pawn.stackEntity(1);
				if (p && p.isActive()) {
					const char* animName = p.animationName();
					if (
							p.currentAnimDuration() == 1
							&& (
								strcmp(animName, "BitF6D") == 0
								|| strcmp(animName, "BitF2D_Bunri") == 0
							)
					) {
						player.ramlethalBitFStartPos = p.posX();
					}
				}
			} else if (pawn.characterType() == CHARACTER_TYPE_SIN) {
				if (strcmp(pawn.animationName(), "UkaseWaza") == 0
						&& pawn.currentAnimDuration() == 7 && !pawn.isRCFrozen()) {
					player.sinHawkBakerStartX = pawn.posX();
				}
			} else if (pawn.characterType() == CHARACTER_TYPE_ELPHELT) {
				if (strcmp(pawn.animationName(), "Shotgun_Fire_MAX") == 0 && pawn.currentAnimDuration() == 1 && !pawn.isRCFrozen()) {
					player.elpheltShotgunX = pawn.posX();
				}
			}
		}
	}
	endScene.orig_handleUpon((void*)pawn.ent, signal);
}

// Runs on the main thread
void EndScene::HookHelp::logicOnFrameAfterHitHook(bool isAirHit, int param2) {
	endScene.logicOnFrameAfterHitHook(Entity{(char*)this}, isAirHit, param2);
}

// Runs on the main thread
void EndScene::logicOnFrameAfterHitHook(Entity pawn, bool isAirHit, int param2) {
	bool functionWillNotDoAnything = !pawn.inHitstunNextFrame() && (!pawn.receivedAttack()->enableGuardBreak() || !pawn.inBlockstunNextFrame());
	orig_logicOnFrameAfterHit((void*)pawn.ent, isAirHit, param2);
	if (pawn.isPawn() && !functionWillNotDoAnything && !iGiveUp) {
		PlayerInfo& player = findPlayer(pawn);
		player.hitstopMax = pawn.startingHitstop();
		player.hitstopElapsed = 0;
		player.lastHitstopBeforeWipe = player.hitstopMax;  // try Sol Gunflame YRC delay 1f 5P:
			// on connection frame, inHitstunNextFrame flag is set. On the connection frame attacker gains hitstop instantly and defender doesn't.
			// On the frame after, this function fires and sets startingHitstop for the defender.
			// On the same frame, beginHitstop fires for the defending pawn, current hitstop being 0 and starting hitstop same as here,
			// and sets current hitstop to non-zero and we also get it from there.
			// On the same frame, the hit detection successful hit fires and erases hitstop (if a hit connected on this frame).
			// On the same frame, beginHitstop fires for the defending pawn, with current hitstop non-0, and startingHitstop 0,
			// and erases current hitstop.
		// after this function, the enclosing function may additionally increase hitstun
		// try this on Potemkin ICPM: it sets hitstun in this hook first, but just outside the hook adds 10 to it
		player.setHitstopMax = true;
		player.setHitstopMaxSuperArmor = false;
		player.setHitstunMax = true;
		player.receivedSpeedYValid = isAirHit;
		player.receivedSpeedY = pawn.receivedSpeedY();
		Entity attacker = pawn.attacker();
		player.pushbackMax = pawn.pendingPushback();
		player.baseFdPushback = 0;
		if (attacker) {
			PlayerInfo& attackerPlayer = findPlayer(attacker);
			attackerPlayer.baseFdPushback = 0;
		}
		entityManager.calculateSpeedYProration(
			pawn.comboCount(),
			pawn.weight(),
			pawn.receivedAttack()->ignoreWeight(),
			pawn.receivedAttack()->dontUseComboTimerForSpeedY(),
			&player.receivedSpeedYWeight,
			&player.receivedSpeedYComboProration);
		entityManager.calculateHitstunProration(
			pawn.receivedAttack()->noHitstunScaling(),
			isAirHit,
			pawn.comboTimer(),
			&player.hitstunProration);
		entityManager.calculatePushback(
			pawn.receivedAttack()->level,
			pawn.comboTimer(),
			pawn.receivedAttack()->dontUseComboTimerForPushback(),
			pawn.ascending(),
			pawn.y(),
			pawn.receivedAttack()->pushbackModifier,
			pawn.receivedAttack()->airPushbackModifier,
			true,
			pawn.receivedAttack()->pushbackModifierOnHitstun,
			&player.basePushback,
			&player.attackPushbackModifier,
			&player.hitstunPushbackModifier,
			&player.comboTimerPushbackModifier);
		player.hitstunProrationValid = true;
		
		player.ibPushbackModifier = 100;
	}
}

// Runs on the main thread
ProjectileInfo& EndScene::findProjectile(Entity pawn) {
	for (ProjectileInfo& projectile : projectiles) {
		if (projectile.ptr == pawn) {
			return projectile;
		}
	}
	return emptyProjectile;
}

// Runs on the main thread
void EndScene::HookHelp::BBScr_runOnObjectHook(int entityReference) {
	endScene.BBScr_runOnObjectHook(Entity{(char*)this}, entityReference);
}

// Runs on the main thread
void EndScene::BBScr_runOnObjectHook(Entity pawn, int entityReference) {
	orig_BBScr_runOnObject((void*)pawn.ent, entityReference);
	if (!shutdown && pawn.isPawn() && !iGiveUp) {
		PlayerInfo& player = findPlayer(pawn);
		Entity objectBeingRunOn = pawn.currentRunOnObject();
		if (objectBeingRunOn) {
			if (!objectBeingRunOn.isPawn()) {
				ProjectileInfo& projectile = findProjectile(objectBeingRunOn);
				if (projectile.ptr) {
					events.emplace_back();
					OccuredEvent& event = events.back();
					event.type = OccuredEvent::SIGNAL;
					event.u.signal.from = pawn;
					event.u.signal.to = projectile.ptr;
					memcpy(event.u.signal.fromAnim, pawn.animationName(), 32);
				}
			}
		}
	}
}

// Runs on the main thread
void EndScene::REDAnywhereDispDrawHookStatic(void* canvas, FVector2D* screenSize) {
	endScene.REDAnywhereDispDrawHook(canvas, screenSize);
}

// Runs on the main thread
template<typename T>
void enqueueRenderCommand() {
	if (*endScene.GIsThreadedRendering) {
		FRingBuffer_AllocationContext allocationContext(endScene.GRenderCommandBuffer, sizeof(T));
		if (allocationContext.GetAllocatedSize() < sizeof(T)) {
			if (allocationContext.AllocationStart) {
				new (allocationContext.AllocationStart) FSkipRenderCommand(allocationContext.GetAllocatedSize());
			}
			allocationContext.Commit();
			allocationContext = FRingBuffer_AllocationContext(endScene.GRenderCommandBuffer, sizeof(T));
		}
		if (allocationContext.AllocationStart) {
			new (allocationContext.AllocationStart) T();
		}
			
	} else {
		T MyCommand_2348623486;
		MyCommand_2348623486.Execute();
	}
}

// Runs on the graphics thread
IDirect3DDevice9* EndScene::getDevice() {
	return *(IDirect3DDevice9**)(*direct3DVTable.d3dManager + 0x24);
}

// Runs on the graphics thread
unsigned int DrawBoxesRenderCommand::Execute() {
	endScene.executeDrawBoxesRenderCommand(this);
	return sizeof(*this);
}
const wchar_t* DrawBoxesRenderCommand::DescribeCommand() noexcept {
	return L"DrawBoxesRenderCommand";
}
void DrawBoxesRenderCommand::Destructor(BOOL freeMem) noexcept {
	DrawBoxesRenderCommand::~DrawBoxesRenderCommand();
	FRenderCommand::Destructor(freeMem);
}
// Runs on the main thread
DrawBoxesRenderCommand::DrawBoxesRenderCommand() {
	drawData.clear();
	drawingPostponed = endScene.drawingPostponed();
	obsStoppedCapturing = endScene.obsStoppedCapturing;
	endScene.drawDataPrepared.copyTo(&drawData);
	if (!endScene.needDrawInputs) {
		for (int i = 0; i < 2; ++i) {
			drawData.inputsSize[i] = 0;
		}
	}
	camera.valuesPrepare.copyTo(cameraValues);
	noNeedToDrawPoints = endScene.willEnqueueAndDrawOriginPoints;
	pauseMenuOpen = endScene.pauseMenuOpen;
	dontShowBoxes = settings.dontShowBoxes;
	iconsUTexture2D = endScene.getIconsUTexture2D();
}

void DrawOriginPointsRenderCommand::Destructor(BOOL freeMem) noexcept {
	DrawOriginPointsRenderCommand::~DrawOriginPointsRenderCommand();
	FRenderCommand::Destructor(freeMem);
}
// Runs on the graphics thread
unsigned int DrawOriginPointsRenderCommand::Execute() {
	endScene.executeDrawOriginPointsRenderCommand(this);
	return sizeof(*this);
}
const wchar_t* DrawOriginPointsRenderCommand::DescribeCommand() noexcept {
	return L"DrawOriginPointsRenderCommand";
}

// Runs on the graphics thread
unsigned int DrawImGuiRenderCommand::Execute() {
	endScene.executeDrawImGuiRenderCommand(this);
	return sizeof(*this);
}
const wchar_t* DrawImGuiRenderCommand::DescribeCommand() noexcept {
	return L"DrawImGuiRenderCommand";
}
void DrawImGuiRenderCommand::Destructor(BOOL freeMem) noexcept {
	DrawImGuiRenderCommand::~DrawImGuiRenderCommand();
	FRenderCommand::Destructor(freeMem);
}
// Runs on the main thread
UiOrFramebarDrawData::UiOrFramebarDrawData(bool calledFromDrawOriginPointsRenderCommand) {
	if (calledFromDrawOriginPointsRenderCommand && !endScene.needEnqueueUiWithPoints) return;
	iconsUTexture2D = endScene.getIconsUTexture2D();
	drawingPostponed = endScene.drawingPostponed();
	obsStoppedCapturing = endScene.obsStoppedCapturing;
	if (endScene.queueingFramebarDrawCommand && endScene.uiWillBeDrawnOnTopOfPauseMenu) {
		ui.getFramebarDrawData(drawData);
	} else {
		ui.copyDrawDataTo(drawData);
	}
}

// Runs on the main thread
void EndScene::REDAnywhereDispDrawHook(void* canvas, FVector2D* screenSize) {
	if (graphics.shutdown) {
		static bool graphicsOnShutdownCalled = false;
		if (!graphicsOnShutdownCalled) {
			graphicsOnShutdownCalled = true;
			enqueueRenderCommand<ShutdownRenderCommand>();
		}
	}
	bool isFading = false;
	bool drawBoxesEnqueued = false;
	bool needEnqueueOriginPoints = false;
	willEnqueueAndDrawOriginPoints = false;
	endSceneAndPresentHooked = graphics.endSceneAndPresentHooked;
	obsStoppedCapturing = graphics.obsStoppedCapturing;
	pauseMenuOpen = false;
	uiWillBeDrawnOnTopOfPauseMenu = false;
	bool drawingPostponedLocal;
	needEnqueueUiWithPoints = false;
	if (!shutdown && !graphics.shutdown) {
		drawDataPrepared.clearBoxes();
		lastScreenSize = *screenSize;
		
		if (!gifMode.modDisabled) {
			if (!getIconsUTexture2D()) {
				needEnqueueOriginPoints = true;
			}
		}
		
		if (*aswEngine) {
			isFading = game.isFading();
			logic();
			uiWillBeDrawnOnTopOfPauseMenu = isFading || settings.displayUIOnTopOfPauseMenu && pauseMenuOpen && ui.visible;
		} else {
			uiWillBeDrawnOnTopOfPauseMenu = true;
		}
		drawingPostponedLocal = drawingPostponed();
		if (!shutdown && !graphics.shutdown) {
			ui.drawData = nullptr;
			ui.pauseMenuOpen = pauseMenuOpen;
			ui.drawingPostponed = drawingPostponedLocal;
			ui.needSplitFramebar = uiWillBeDrawnOnTopOfPauseMenu && !drawingPostponedLocal && pauseMenuOpen && ui.visible;
			ui.needShowFramebarCached = ui.needShowFramebar();
			ui.prepareDrawData();
			needEnqueueUiWithPoints = *aswEngine
				&& ui.drawData
				&& (!uiWillBeDrawnOnTopOfPauseMenu || ui.drewFramebar && ui.needSplitFramebar)
				&& !drawingPostponedLocal;
		}
		if (needEnqueueUiWithPoints) {
			needEnqueueOriginPoints = true;
		}
		if (*aswEngine) {
			if (!gifMode.modDisabled) {
				if (
						(
							!settings.dontShowBoxes && !drawDataPrepared.points.empty()
							|| (drawDataPrepared.inputsSize[0] || drawDataPrepared.inputsSize[1])
						) && !drawingPostponedLocal
				) {
					needEnqueueOriginPoints = true;
				}
				Entity instigator = getSuperflashInstigator();
				if (instigator != nullptr) {
					if (
						instigator.characterType() == CHARACTER_TYPE_RAMLETHAL
						&& strcmp(instigator.animationName(), "BitLaser"_hardcode) == 0
						&& instigator.currentAnimDuration() >= 8 && instigator.currentAnimDuration() < 139
						|| instigator.characterType() == CHARACTER_TYPE_INO
						&& strcmp(instigator.animationName(), "Madogiwa"_hardcode) == 0  // 632146H super
						&& instigator.currentAnimDuration() >= 8 && instigator.currentAnimDuration() < 64
						|| instigator.characterType() == CHARACTER_TYPE_SLAYER
						&& strcmp(instigator.animationName(), "DeadOnTime"_hardcode) == 0
						&& instigator.currentAnimDuration() >= 8 && instigator.currentAnimDuration() < 20
						|| instigator.characterType() == CHARACTER_TYPE_HAEHYUN
						&& strcmp(instigator.animationName(), "BlackHoleAttack"_hardcode) == 0  // 236236H super
						&& instigator.currentAnimDuration() >= 7 && instigator.currentAnimDuration() < 100
						|| instigator.characterType() == CHARACTER_TYPE_ANSWER
						&& strcmp(instigator.animationName(), "Royal_Straight_Flush"_hardcode) == 0  // 632146S super
						&& instigator.currentAnimDuration() >= 19 && instigator.currentAnimDuration() < 143
					) {
						drawDataPrepared.clearBoxes();
					}
				} else {
					for (int i = 0; i < 2 && i < entityList.count; ++i) {
						instigator = entityList.slots[i];
						if (instigator.characterType() == CHARACTER_TYPE_RAVEN
								&& strcmp(instigator.animationName(), "RevengeAttackEx"_hardcode) == 0  // 214214H super
								&& instigator.currentAnimDuration() >= 19 && instigator.currentAnimDuration() < 139) {
							drawDataPrepared.clearBoxes();
							break;
						}
					}
				}
				if (!camera.grabbedValues) {
					camera.grabValues();
				}
			}
			
			willEnqueueAndDrawOriginPoints = needEnqueueOriginPoints && willDrawOriginPoints();
			
			enqueueRenderCommand<DrawBoxesRenderCommand>();
			drawBoxesEnqueued = true;
			
		}
		if (needEnqueueOriginPoints) {
			queueOriginPointDrawingDummyCommandAndInitializeIcon();
		}
	} else {
		drawingPostponedLocal = drawingPostponed();
	}
	queueingFramebarDrawCommand = false;
	orig_REDAnywhereDispDraw(canvas, screenSize);  // calls drawQuadExecHook
	queueingFramebarDrawCommand = false;
	
	if (!shutdown && !graphics.shutdown
			&& (uiWillBeDrawnOnTopOfPauseMenu || drawingPostponedLocal)) {
		FCanvas_Flush(canvas, 0);  // for things to be drawn on top of anything drawn so far, need to flush canvas, otherwise some
		                           // items might still be drawn on top of yours
		enqueueRenderCommand<DrawImGuiRenderCommand>();
	}
	if (!shutdown && !graphics.shutdown && !drawBoxesEnqueued) {
		enqueueRenderCommand<HeartbeatRenderCommand>();
	}
}

// Runs on the main thread
FRingBuffer_AllocationContext::FRingBuffer_AllocationContext(void* InRingBuffer, unsigned int InAllocationSize) {
	endScene.FRingBuffer_AllocationContext_Constructor(this, InRingBuffer, InAllocationSize);
}

// Runs on the main thread
void FRingBuffer_AllocationContext::Commit() {
	endScene.FRingBuffer_AllocationContext_Commit(this);
}

void FRenderCommand::Destructor(BOOL freeMem) noexcept {
	endScene.FRenderCommandDestructor((void*)this, freeMem);
}

// Runs on the main thread
FSkipRenderCommand::FSkipRenderCommand(unsigned int InNumSkipBytes) : NumSkipBytes(InNumSkipBytes) { }

// Runs on the graphics thread
unsigned int FSkipRenderCommand::Execute() {
	return NumSkipBytes;
}
const wchar_t* FSkipRenderCommand::DescribeCommand() noexcept {
	return L"FSkipRenderCommand";
}

// Runs on the graphics thread
unsigned int ShutdownRenderCommand::Execute() {
	endScene.executeShutdownRenderCommand();
	return sizeof(*this);
}
const wchar_t* ShutdownRenderCommand::DescribeCommand() noexcept {
	return L"ShutdownRenderCommand";
}

// Runs on the graphics thread
unsigned int HeartbeatRenderCommand::Execute() {
	endScene.executeHeartbeatRenderCommand();
	return sizeof(*this);
}
const wchar_t* HeartbeatRenderCommand::DescribeCommand() noexcept {
	return L"HeartbeatRenderCommand";
}

// Runs on the graphics thread
void EndScene::executeDrawBoxesRenderCommand(DrawBoxesRenderCommand* command) {
	if (endScene.shutdown || graphics.shutdown) return;
	graphics.drawDataUse.clear();
	command->drawData.copyTo(&graphics.drawDataUse);
	command->cameraValues.copyTo(camera.valuesUse);
	graphics.dontShowBoxes = command->dontShowBoxes;
	graphics.pauseMenuOpen = command->pauseMenuOpen;
	IDirect3DTexture9* tex = getTextureFromUTexture2D(command->iconsUTexture2D);
	graphics.iconsTexture = tex;
	graphics.endSceneIsAwareOfDrawingPostponement = command->drawingPostponed;
	graphics.obsStoppedCapturingFromEndScenesPerspective = command->obsStoppedCapturing;
	if (command->drawingPostponed) return;
	if (graphics.drawingPostponed()) return;
	graphics.noNeedToDrawPoints = command->noNeedToDrawPoints;
	graphics.executeBoxesRenderingCommand(getDevice());
	graphics.noNeedToDrawPoints = false;
}

// Runs on the graphics thread
void EndScene::executeDrawOriginPointsRenderCommand(DrawOriginPointsRenderCommand* command) {
	if (endScene.shutdown || graphics.shutdown) return;
	
	bool hasFramebarDrawData = !command->uiOrFramebarDrawData.drawData.empty()
		&& !command->uiOrFramebarDrawData.drawingPostponed
		&& !graphics.drawingPostponed();
	
	if (settings.dontShowBoxes
			&& !(
				(graphics.drawDataUse.inputsSize[0] || graphics.drawDataUse.inputsSize[1])
			)
			&& !hasFramebarDrawData) return;
	
	if (hasFramebarDrawData) {
		IDirect3DTexture9* tex = getTextureFromUTexture2D(command->uiOrFramebarDrawData.iconsUTexture2D);
		graphics.endSceneIsAwareOfDrawingPostponement = command->uiOrFramebarDrawData.drawingPostponed;
		graphics.obsStoppedCapturingFromEndScenesPerspective = command->uiOrFramebarDrawData.obsStoppedCapturing;
		graphics.uiFramebarDrawData = command->uiOrFramebarDrawData.drawData;
		graphics.uiTexture = tex;
		graphics.needDrawFramebarWithPoints = true;
	}
	
	graphics.onlyDrawPoints = true;
	graphics.drawAll();
	graphics.onlyDrawPoints = false;
	graphics.needDrawFramebarWithPoints = false;
}

// Runs on the main thread
void EndScene::queueOriginPointDrawingDummyCommandAndInitializeIcon() {
	// 6,15 offset to center of "+"
	// full size: 12,22
	
	// Queue a dummy text item for drawing - later we catch it in our drawQuadExecHook and draw players' origin points
	// The item is queued to be drawn on top of HUD but underneath the Pause menu, so that's where the players' origin points will show up.
	char plusSign[9] = "+";
	if (!getIconsUTexture2D()) {
		// Draw a dummy icon so that the icons texture variable gets initialized
		// this code runs when the icon is lost due to switching menus
		strcat(plusSign, "^mAtk1;");
	}
	queueDummyCommand(177, dummyOriginPointX, plusSign);
}

void EndScene::queueDummyCommand(int layer, float x, char* txt) {
	DrawTextWithIconsParams s;
	s.field159_0x100 = 36.0;
	s.layer = layer;
	s.field160_0x104 = -1.0;
	s.field4_0x10 = -1.0;
	s.field155_0xf0 = 1;
	s.field157_0xf8 = -1;
	s.field158_0xfc = -1;
	s.field161_0x108 = 0;
	s.field162_0x10c = 0;
	s.field163_0x110 = -1;
	s.field164_0x114 = 0;
	s.field165_0x118 = 0;
	s.field166_0x11c = -1;
	s.outlineColor = 0xff000000;
	s.flags2 = 0xff000000;
	s.x = x;
	s.y = 0.F;
	s.alignment = ALIGN_LEFT;
	s.text = txt;
	s.field156_0xf4 = 0x010;
	drawTextWithIcons(&s,0x0,1,4,0,0);
}

// Runs on the main thread. Called from orig_REDAnywhereDispDraw
void drawQuadExecHook(FVector2D* screenSize, REDDrawQuadCommand* item, void* canvas) {
	endScene.drawQuadExecHook(screenSize, item, canvas);
}

// Runs on the main thread. Called from ::drawQuadExecHook
void EndScene::drawQuadExecHook(FVector2D* screenSize, REDDrawQuadCommand* item, void* canvas) {
	if (item->count == 4 && (unsigned int&)item->vertices[0].x == getDummyCmdUInt(dummyOriginPointX)) {  // avoid floating point comparison as it may be slower
		if (willDrawOriginPoints()) {
			FCanvas_Flush(canvas, 0);
			if (needEnqueueUiWithPoints) {
				queueingFramebarDrawCommand = true;
			}
			enqueueRenderCommand<DrawOriginPointsRenderCommand>();
			needEnqueueUiWithPoints = false;
		}
		return;  // can safely omit items
	}
	call_orig_drawQuadExec(orig_drawQuadExec, screenSize, item, canvas);
}

// Runs on the main thread
BYTE* EndScene::getIconsUTexture2D() {
	return *(BYTE**)iconTexture;
}

// Runs on the graphics thread
void EndScene::executeDrawImGuiRenderCommand(DrawImGuiRenderCommand* command) {
	if (shutdown || graphics.shutdown) return;
	if (!graphics.canDrawOnThisFrame()) return;
	IDirect3DTexture9* tex = getTextureFromUTexture2D(command->uiOrFramebarDrawData.iconsUTexture2D);
	graphics.endSceneIsAwareOfDrawingPostponement = command->uiOrFramebarDrawData.drawingPostponed;
	graphics.obsStoppedCapturingFromEndScenesPerspective = command->uiOrFramebarDrawData.obsStoppedCapturing;
	if (command->uiOrFramebarDrawData.drawingPostponed) {
		graphics.uiTexture = tex;
		graphics.uiDrawData = std::move(command->uiOrFramebarDrawData.drawData);
		return;
	}
	if (graphics.drawingPostponed()) {
		return;
	}
	ui.onEndScene(getDevice(), command->uiOrFramebarDrawData.drawData.data(), tex);
}

// Runs on the graphics thread
void EndScene::executeShutdownRenderCommand() {
	if (!graphics.shutdown) return;
	graphics.onShutdown();
}

// Runs on the graphics thread
void EndScene::executeHeartbeatRenderCommand() {
	if (graphics.shutdown) return;
	graphics.heartbeat();
}

// Runs on the graphics thread
IDirect3DTexture9* EndScene::getTextureFromUTexture2D(BYTE* uTex2D) {
	if (!uTex2D) return nullptr;
	BYTE* next = *(BYTE**)(uTex2D + 0x3c + 0x84);  // read FTextureResouruce* Resource
	if (!next) return nullptr;
	next = *(BYTE**)(next + 0x14);  // read FTextureRHIRef TextureRHI
	if (!next) return nullptr;
	
	//  FTextureRHIRef is a typedef for TDynamicRHIResourceReference<RRT_Texture>.
	//  TDynamicRHIResourceReference<RRT_Texture> is a:
	//  struct TDynamicRHIResourceReference<RRT_Texture> {
	//      TDynamicRHIResource<RRT_Texture>* Reference;
	//  };
	//  Pointers to TDynamicRHIResource<RRT_Texture> type are actually pointing at offset 0xc
	//  into a TD3D9Texture<IDirect3DBaseTexture9,RRT_Texture> type.
	//  template<typename D3DResourceType,...>
	//  class TD3D9Texture : public FRefCountedObject, public TRefCountPtr<D3DResourceType> ...
	//  The TRefCountPtr<D3DResourceType> is at offset 0x8 of TD3D9Texture and is actually a:
	//  template<typename ReferencedType>
	//  class TRefCountPtr {
	//      ReferencedType* Reference;
	//  };
	return *(IDirect3DTexture9**)(next + -0xc + 0x8);
}

void EndScene::HookHelp::backPushbackApplierHook() {
	endScene.backPushbackApplierHook((char*)this);
}

void EndScene::backPushbackApplierHook(char* thisArg) {
	if (!iGiveUp) {
		entityList.populate();
		for (int i = 0; i < 2; ++i) {
			PlayerInfo& player = players[i];
			player.fdPushback = entityList.slots[i].fdPushback();
			// it changes after it was used, so if we don't do this, at the end of a frame we will see a decremented value
		}
	}
	orig_backPushbackApplier((void*)thisArg);
}

void EndScene::HookHelp::pushbackStunOnBlockHook(bool isAirHit) {
	endScene.pushbackStunOnBlockHook(Entity{(char*)this}, isAirHit);
}

void EndScene::pushbackStunOnBlockHook(Entity pawn, bool isAirHit) {
	orig_pushbackStunOnBlock((void*)pawn.ent, isAirHit);
	if (!iGiveUp) {
		PlayerInfo& player = findPlayer(pawn);
		player.ibPushbackModifier = 100;
		Entity attacker = pawn.attacker();
		PlayerInfo& attackerPlayer = findPlayer(attacker);
		if (attacker) {
			attackerPlayer.baseFdPushback = 0;
		}
		if (isHoldingFD(pawn)) {
			attackerPlayer.fdPushbackMax = attacker.fdPushback();
			int dist = pawn.posX() - attacker.posX();
			if (dist < 0) dist = -dist;
			attackerPlayer.oppoWasTouchingWallOnFD = pawn.isTouchingLeftWall() || pawn.isTouchingRightWall();
			if (dist >= 385000) {
				attackerPlayer.baseFdPushback = 500;
			} else {
				attackerPlayer.baseFdPushback = 900;
			}
		} else if (pawn.successfulIB() && pawn.lastHitResult() == HIT_RESULT_BLOCKED) {
			player.ibPushbackModifier = isAirHit ? 10 : 90;
		}
		player.receivedSpeedYValid = false;
		player.pushbackMax = pawn.pendingPushback();
		entityManager.calculatePushback(
			pawn.receivedAttack()->level,
			pawn.comboTimer(),
			pawn.receivedAttack()->dontUseComboTimerForPushback(),
			pawn.ascending(),
			pawn.y(),
			pawn.receivedAttack()->pushbackModifier,
			pawn.receivedAttack()->airPushbackModifier,
			pawn.inHitstun(),
			pawn.receivedAttack()->pushbackModifierOnHitstun,
			&player.basePushback,
			&player.attackPushbackModifier,
			&player.hitstunPushbackModifier,
			&player.comboTimerPushbackModifier);
		player.hitstunProrationValid = false;
	}
}

void EndScene::HookHelp::BBScr_sendSignalHook(int referenceType, int signal) {
	endScene.BBScr_sendSignalHook(Entity{(char*)this}, referenceType, signal);
}

void EndScene::HookHelp::BBScr_sendSignalToActionHook(const char* searchAnim, int signal) {
	endScene.BBScr_sendSignalToActionHook(Entity{(char*)this}, searchAnim, signal);
}

void EndScene::BBScr_sendSignalHook(Entity pawn, int referenceType, int signal) {
	if (!iGiveUp) {
		Entity referredEntity = getReferredEntity((void*)pawn.ent, referenceType);
		
		bool isDizzyBubblePopping = referredEntity == pawn && signal == 0x17  // HIT_OR_BLOCK
				&& (strcmp(pawn.animationName(), "AwaPObj"_hardcode) == 0  // Dizzy bubble
					|| strcmp(pawn.animationName(), "AwaKObj"_hardcode) == 0);
		
		if (!shutdown && referredEntity && !isDizzyBubblePopping) {
			ProjectileInfo& projectile = findProjectile(referredEntity);
			int team = pawn.team();
			if (projectile.ptr && (team == 0 || team == 1)) {
				events.emplace_back();
				OccuredEvent& event = events.back();
				event.type = OccuredEvent::SIGNAL;
				event.u.signal.from = pawn;
				event.u.signal.to = projectile.ptr;
				memcpy(event.u.signal.fromAnim, pawn.animationName(), 32);
			}
		}
	}
	orig_BBScr_sendSignal((void*)pawn, referenceType, signal);
}

void EndScene::BBScr_sendSignalToActionHook(Entity pawn, const char* searchAnim, int signal) {
	if (!shutdown && !iGiveUp) {
		sendSignalStack.push_back(pawn);
	}
	orig_BBScr_sendSignalToAction((void*)pawn, searchAnim, signal);
	if (!shutdown && !iGiveUp) {
		sendSignalStack.pop_back();
	}
}

BOOL EndScene::HookHelp::skillCheckPieceHook() {
	return endScene.skillCheckPieceHook(Entity{(char*)this});
}

BOOL EndScene::skillCheckPieceHook(Entity pawn) {
	BOOL result = orig_skillCheckPiece((void*)pawn.ent);
	if (!iGiveUp) {
		PlayerInfo& player = findPlayer(pawn);
		if (player.pawn) {
			player.wasPlayerval[0] = pawn.playerVal(0);
			player.wasPlayerval[1] = pawn.playerVal(1);
			player.wasPlayerval[2] = pawn.playerVal(2);
			player.wasPlayerval[3] = pawn.playerVal(3);
			player.wasEnableNormals = pawn.enableNormals();
			entityList.populate();
			Entity other = entityList.slots[1 - player.index];
			player.wasCanYrc = player.pawn.romanCancelAvailability() == ROMAN_CANCEL_ALLOWED
				&& !pawn.defaultYrcWindowOver()
				&& !pawn.overridenYrcWindowOver()
				&& !(
					other.inHitstun()
					|| other.blockstun() > 0
					|| other.inBlockstunNextFrame()
					|| other.hitstunOrBlockstunTypeKindOfState()
					|| (
						other.currentMoveIndex() != -1
						&& other.movesBase()[other.currentMoveIndex()].type == MOVE_TYPE_BLUE_BURST
					)
				);
			player.wasProhibitFDTimer = min(127, pawn.prohibitFDTimer());
			player.wasAirdashHorizontallingTimer = min(127, pawn.airdashHorizontallingTimer());
			player.wasEnableGatlings = player.wasEnableGatlings && pawn.currentAnimDuration() != 1 || pawn.enableGatlings();
			player.wasEnableWhiffCancels = player.wasEnableWhiffCancels && pawn.currentAnimDuration() != 1 || pawn.enableWhiffCancels();
			player.wasEnableSpecials = player.wasEnableSpecials && pawn.currentAnimDuration() != 1 || pawn.enableSpecials();
			player.wasEnableSpecialCancel = player.wasEnableSpecialCancel && pawn.currentAnimDuration() != 1 || pawn.enableSpecialCancel();
			player.wasEnableJumpCancel = player.wasEnableJumpCancel && pawn.currentAnimDuration() != 1 || pawn.enableJumpCancel() && pawn.attackCollidedSoCanJumpCancelNow();
			player.wasAttackCollidedSoCanCancelNow = player.wasAttackCollidedSoCanCancelNow && pawn.currentAnimDuration() != 1 || pawn.attackCollidedSoCanCancelNow();
			player.wasEnableAirtech = player.wasEnableAirtech && pawn.currentAnimDuration() != 1 || pawn.enableAirtech();
			player.wasForceDisableFlags = pawn.forceDisableFlags();
			player.obtainedForceDisableFlags = true;
			if (pawn.currentAnimDuration() == 1) {
				player.wasCancels.clear();
			}
			collectFrameCancels(player, player.wasCancels);
			player.wasCantBackdashTimer = pawn.cantBackdashTimer();
			player.wasOtg = pawn.isOtg();
		}
	}
	return result;
}

void EndScene::HookHelp::BBScr_setHitstopHook(int hitstop) {
	endScene.BBScr_setHitstopHook(Entity{(char*)this}, hitstop);
}

void EndScene::BBScr_setHitstopHook(Entity pawn, int hitstop) {
	if (!shutdown && !iGiveUp) {
		PlayerInfo& player = findPlayer(pawn);
		player.hitstopMax = hitstop;
		player.hitstopElapsed = 0;
		player.setHitstopMax = true;
		player.setHitstopMaxSuperArmor = false;
	}
	orig_BBScr_setHitstop((void*)pawn.ent, hitstop);
}

void EndScene::HookHelp::beginHitstopHook() {
	endScene.beginHitstopHook(Entity{(char*)this});
}

void EndScene::beginHitstopHook(Entity pawn) {
	if (!iGiveUp && pawn.needSetHitstop()) {
		PlayerInfo& player = findPlayer(pawn);
		if (pawn.startingHitstop() == 0) {
			player.lastHitstopBeforeWipe = pawn.hitstop();
		} else {
			player.hitstopMaxSuperArmor = pawn.startingHitstop();
		}
		player.setHitstopMaxSuperArmor = true;
	}
	orig_beginHitstop((void*)pawn.ent);
}

void EndScene::HookHelp::BBScr_ignoreDeactivateHook() {
	endScene.BBScr_ignoreDeactivateHook(Entity{(char*)this});
}

void EndScene::BBScr_ignoreDeactivateHook(Entity pawn) {
	orig_BBScr_ignoreDeactivate((void*)pawn.ent);
	if (!iGiveUp) {
		PlayerInfo& player = findPlayer(pawn);
		player.baikenReturningToBlockstunAfterAzami = true;
	}
}

bool EndScene::isEntityHidden(const Entity& ent) {
	return findHiddenEntity(ent) != hiddenEntities.end();
}

int EndScene::getFramebarPosition() const {
	return framebarPosition;
}

int EndScene::getFramebarPositionHitstop() const {
	return framebarPositionHitstop;
}

DWORD EndScene::getAswEngineTick() {
	return *(DWORD*)(*aswEngine + 4 + game.aswEngineTickCountOffset);
}

void EndScene::incrementNextFramebarIdDirectly() { 
	if (nextFramebarId == INT_MAX) {
		nextFramebarId = 0;
	} else {
		++nextFramebarId;
	}
}

void EndScene::incrementNextFramebarIdSmartly() { 
	incrementNextFramebarIdDirectly();
	bool restart;
	do {
		restart = false;
		for (const ProjectileFramebar& f: projectileFramebars) {
			if (f.id == nextFramebarId) {
				incrementNextFramebarIdDirectly();
				restart = true;
				break;
			}
		}
	} while(restart);
}

ProjectileFramebar& EndScene::findProjectileFramebar(ProjectileInfo& projectile, bool needCreate) {
	static ProjectileFramebar defaultFramebar { -1, INT_MAX };
	const char* name = nullptr;
	const char* slangName = nullptr;
	const char* nameUncombined = nullptr;
	const char* slangNameUncombined = nullptr;
	const char* nameFull = nullptr;
	bool dontReplaceTitle;
	
	if (projectile.moveNonEmpty) {
		
		const char* framebarName = projectile.move.getFramebarName(projectile.ptr);
		
		if (framebarName) {
			name = framebarName;
			dontReplaceTitle = false;
		} else {
			name = "Projectiles";
			dontReplaceTitle = true;
		}
		
		slangName = projectile.move.framebarSlangNameSelector ? projectile.move.framebarSlangNameSelector(projectile.ptr) : projectile.move.framebarSlangName;
		nameUncombined = projectile.move.framebarNameUncombined;
		slangNameUncombined = projectile.move.framebarSlangNameUncombined;
		
		if (projectile.move.framebarNameFull) {
			nameFull = projectile.move.framebarNameFull;
		}
	} else {
		name = "Projectiles";
		slangName = nullptr;
		nameUncombined = nullptr;
		slangNameUncombined = nullptr;
		nameFull = nullptr;
		dontReplaceTitle = true;
	}
	for (ProjectileFramebar& bar : projectileFramebars) {
		if (bar.playerIndex == projectile.team
				&& (
					projectile.move.isEddie
						? bar.isEddie
						:
							!bar.isEddie
							&& bar.id == projectile.framebarId
				)
			) {
			if (!(bar.moveFramebarId != -1 && projectile.move.framebarId == -1)) {
				bar.moveFramebarId = projectile.move.framebarId;
			}
			if (!(
					*bar.titleShort != '\0'
					&& (
						name == nullptr
						|| *name == '\0'
						|| dontReplaceTitle
					)
				)
				&& !(
					bar.titleLandedHit
					&& !projectile.landedHit
				)) {
				bar.setTitle(name, slangName, nameUncombined, slangNameUncombined, nameFull, projectile.landedHit);
			}
			return bar;
		}
	}
	if (!needCreate) {
		return defaultFramebar;
	}
	projectileFramebars.emplace_back();
	ProjectileFramebar& bar = projectileFramebars.back();
	bar.playerIndex = projectile.team;
	bar.id = nextFramebarId;
	bar.isEddie = projectile.move.isEddie;
	projectile.framebarId = nextFramebarId;
	incrementNextFramebarIdSmartly();
	bar.setTitle(name, slangName, nameUncombined, slangNameUncombined, nameFull, projectile.landedHit);
	bar.moveFramebarId = projectile.move.framebarId;
	return bar;
}

CombinedProjectileFramebar& EndScene::findCombinedFramebar(const ProjectileFramebar& source, bool hitstop) {
	int id;
	if (source.moveFramebarId != -1) {
		id = source.moveFramebarId;
	} else {
		id = INT_MAX - 1 + source.playerIndex;
	}
	const bool combineProjectileFramebarsWhenPossible = settings.combineProjectileFramebarsWhenPossible;
	for (CombinedProjectileFramebar& bar : combinedFramebars) {
		if (
				bar.playerIndex == source.playerIndex
				&& (
					(
						combineProjectileFramebarsWhenPossible
							? bar.canBeCombined(hitstop ? source.hitstop : source.main)
							: false
					)
					|| bar.id == id
				)
		) {
			if (!(bar.moveFramebarId != -1 && source.moveFramebarId == -1)) {
				bar.moveFramebarId = source.moveFramebarId;
			}
			if (!(
					*bar.titleShort != '\0'
					&& bar.moveFramebarId != -1
					&& (
						*source.titleShort == '\0'
						|| source.moveFramebarId == -1
					)
				)
				&& !(
					bar.titleLandedHit
					&& !source.titleLandedHit
				)
			) {
				bar.copyTitle(source);
			}
			return bar;
		}
	}
	combinedFramebars.emplace_back();
	CombinedProjectileFramebar& bar = combinedFramebars.back();
	bar.playerIndex = source.playerIndex;
	bar.id = id;
	bar.isEddie = source.isEddie;
	bar.copyTitle(source);
	bar.moveFramebarId = source.moveFramebarId;
	return bar;
}

void EndScene::copyIdleHitstopFrameToTheRestOfSubframebars(EntityFramebar& entityFramebar,
		bool framebarAdvanced,
		bool framebarAdvancedIdle,
		bool framebarAdvancedHitstop,
		bool framebarAdvancedIdleHitstop) {
	
	bool isPlayer = entityFramebar.belongsToPlayer();
	FramebarBase& framebar = entityFramebar.getIdleHitstop();
	const int framebarPos = (framebarPositionHitstop + framebarIdleHitstopFor) % _countof(Framebar::frames);
	FrameBase& currentFrame = framebar.getFrame(framebarPos);
	FrameBase* destinationFrame;
	
	
	destinationFrame = &entityFramebar.getHitstop().getFrame(framebarPositionHitstop);
	if (framebarAdvancedHitstop) {
		entityFramebar.copyFrame(*destinationFrame, currentFrame);
		entityFramebar.getHitstop().processRequests(*destinationFrame);
	} else {
		if (!framebarAdvancedIdleHitstop) {
			entityFramebar.copyActiveDuringSuperfreeze(*destinationFrame, currentFrame);
		}
		entityFramebar.getHitstop().collectRequests(framebar, framebarAdvancedIdleHitstop, currentFrame);
	}
	int idlePos = (framebarPosition + framebarIdleFor) % _countof(Framebar::frames);
	destinationFrame = &entityFramebar.getIdle().getFrame(idlePos);
	if (framebarAdvancedIdle) {
		entityFramebar.copyFrame(*destinationFrame, currentFrame);
		entityFramebar.getIdle().processRequests(*destinationFrame);
	} else {
		if (!framebarAdvancedIdleHitstop) {
			entityFramebar.copyActiveDuringSuperfreeze(*destinationFrame, currentFrame);
		}
		entityFramebar.getIdle().collectRequests(framebar, framebarAdvancedIdleHitstop, currentFrame);
	}
	destinationFrame = &entityFramebar.getMain().getFrame(framebarPosition);
	if (framebarAdvanced) {
		entityFramebar.copyFrame(*destinationFrame, currentFrame);
		entityFramebar.getMain().processRequests(*destinationFrame);
	} else {
		if (!framebarAdvancedIdleHitstop) {
			entityFramebar.copyActiveDuringSuperfreeze(*destinationFrame, currentFrame);
		}
		entityFramebar.getMain().collectRequests(framebar, framebarAdvancedIdleHitstop, currentFrame);
	}
	if (framebarAdvancedIdle) {
		entityFramebar.getIdle().clearRequests();
	}
	if (framebarAdvancedIdleHitstop) {
		framebar.clearRequests();
	}
	if (framebarAdvancedHitstop) {
		entityFramebar.getHitstop().clearRequests();
	}
}

bool EndScene::misterPlayerIsManuallyCrouching(const PlayerInfo& player) {
	if (player.cmnActIndex == CmnActCrouch
			|| player.cmnActIndex == CmnActCrouchTurn
			|| player.cmnActIndex == CmnActStand2Crouch) {
		if (!game.isTrainingMode()) return true;
		int playerSide = game.getPlayerSide();
		DummyRecordingMode mode = game.getDummyRecordingMode();
		if (mode == DUMMY_MODE_CONTROLLING || mode == DUMMY_MODE_RECORDING) {
			playerSide = 1 - playerSide;
		}
		bool autoCrouch = (mode == DUMMY_MODE_IDLE || mode == DUMMY_MODE_CONTROLLING || mode == DUMMY_MODE_RECORDING);
		if (autoCrouch && playerSide != player.index) {
			return false;
		} else {
			return true;
		}
	} else {
		return false;
	}
}

void EndScene::onGifModeBlackBackgroundChanged() {
	bool newIsInMode = (gifMode.gifModeOn || gifMode.gifModeToggleBackgroundOnly) && settings.turnOffPostEffectWhenMakingBackgroundBlack;
	if (newIsInMode != isInDarkenModePlusTurnOffPostEffect) {
		isInDarkenModePlusTurnOffPostEffect = newIsInMode;
		if (isInDarkenModePlusTurnOffPostEffect) {
			postEffectWasOnWhenEnteringDarkenModePlusTurnOffPostEffect = game.postEffectOn() != 0;
			game.postEffectOn() = false;
		} else if (postEffectWasOnWhenEnteringDarkenModePlusTurnOffPostEffect) {
			game.postEffectOn() = true;
		}
	}
}

bool EndScene::willDrawOriginPoints() {
	return !shutdown
			&& !graphics.shutdown
			&& (
				!drawDataPrepared.points.empty()
				&& !settings.dontShowBoxes
				|| (drawDataPrepared.inputsSize[0] || drawDataPrepared.inputsSize[1])
			)
			&& !gifMode.modDisabled
			&& !drawingPostponed();
}

void EndScene::collectFrameCancelsPart(PlayerInfo& player, std::vector<GatlingOrWhiffCancelInfo>& vec, const AddedMoveData* move, int iterationIndex) {
	bool foundTheMove = false;
	int vecPos = -1;
	for (const GatlingOrWhiffCancelInfo& existingElem : vec) {
		if (existingElem.move == move) {
			foundTheMove = true;
		}
		if (existingElem.iterationIndex < iterationIndex) {
			vecPos = &existingElem - vec.data();
		}
	}
	if (foundTheMove) return;
	GatlingOrWhiffCancelInfo* ptr = nullptr;
	if (vecPos == -1 && !vec.empty()) {
		vec.emplace(vec.begin());
		ptr = &vec.front();
	} else if (vecPos == -1 && vec.empty() || vecPos == (int)vec.size() - 1) {
		vec.emplace_back();
		ptr = &vec.back();
	} else {
		vec.emplace(vec.begin() + vecPos + 1);
		ptr = &vec[vecPos + 1];
	}
	GatlingOrWhiffCancelInfo& cancel = *ptr;
	cancel.iterationIndex = iterationIndex;
	MoveInfo obtainedInfo;
	bool moveNonEmpty = moves.getInfo(obtainedInfo, player.charType, move->name, move->stateName, false);
	if (!moveNonEmpty || !obtainedInfo.getDisplayName(player)) {
		cancel.name = move->name;
		cancel.slangName = nullptr;
		int lenTest = strnlen(move->name, 32);
		if (lenTest >= 32) {
			int somethingBad = 1;
		}
		cancel.nameIncludesInputs = false;
	} else {
		cancel.name = obtainedInfo.getDisplayName(player);
		cancel.slangName = obtainedInfo.getDisplayNameSlang(player);
		cancel.nameIncludesInputs = obtainedInfo.nameIncludesInputs;
	}
	cancel.move = move;
	cancel.replacementInputs = obtainedInfo.replacementInputs;
	cancel.bufferTime = obtainedInfo.replacementBufferTime ? obtainedInfo.replacementBufferTime : move->bufferTime;
}

void EndScene::collectFrameCancels(PlayerInfo& player, FrameCancelInfo& frame) {
	if (player.moveNonEmpty) frame.whiffCancelsNote = player.move.whiffCancelsNote;
	const AddedMoveData* base = player.pawn.movesBase();
	int* indices = player.pawn.moveIndices();
	if (player.charType == CHARACTER_TYPE_BAIKEN
			&& player.pawn.blockstun()
			&& player.pawn.playerVal(0)) {
		if (baikenBlockCancels.empty()) {
			const char* baikenBlockCancelsStrs[] {
				"DeadAngleAttack",  // standing
				// Blue Burst, obviously
				// FD, obviously
				"BlockingStandGuard",  // standing
				"BlockingCrouchGuard",  // crouching
				"YoushijinGuard",  // standing
				"MawarikomiGuard", // standing
				"SakuraGuard", // standing
				"IssenGuard",  // standing
				"TeppouGuard",  // standing
				"AirGCAntiAirAGuard",  // jumping
				"AirGCAntiAirBGuard",  // jumping
				"AirGCAntiGroundCGuard",  // jumping
				"AirGCAntiGroundDGuard",  // jumping
				"BlockingKakuseiGuard"  // none
			};
			baikenBlockCancels.reserve(_countof(baikenBlockCancelsStrs));
			for (int i = 0; i < _countof(baikenBlockCancelsStrs); ++i) {
				baikenBlockCancels.emplace_back();
				ForceAddedWhiffCancel& newCancel = baikenBlockCancels.back();
				newCancel.name = baikenBlockCancelsStrs[i];
			}
		}
		for (ForceAddedWhiffCancel& cancel : baikenBlockCancels) {
			int moveIndex = cancel.getMoveIndex(player.pawn);
			const AddedMoveData* move = base + indices[moveIndex];
			if (checkMoveConditions(player, move)) {
				collectFrameCancelsPart(player, frame.whiffCancels, move, moveIndex);
			}
		}
		return;
	}
	bool enableGatlings = player.wasEnableGatlings && player.wasAttackCollidedSoCanCancelNow;
	bool enableWhiffCancels = player.wasEnableWhiffCancels;
	for (int i = player.pawn.moveIndicesCount() - 1; i >= 0; --i) {
		const AddedMoveData* move = base + indices[i];
		bool isGatling = move->gatlingOption() && enableGatlings;
		bool isWhiffCancel = move->whiffCancelOption() && enableWhiffCancels;
		if ((isGatling || isWhiffCancel) && checkMoveConditions(player, move)) {
			if (isGatling) {
				collectFrameCancelsPart(player, frame.gatlings, move, i);
			}
			if (isWhiffCancel) {
				collectFrameCancelsPart(player, frame.whiffCancels, move, i);
			}
		}
	}
	if (player.moveNonEmpty && enableWhiffCancels) {
		if (player.move.onlyAddForceWhiffCancelsOnFirstFrameOfSprite
				?
					!player.pawn.isRCFrozen()
					&& player.sprite.frame == 0
					&& strcmp(player.sprite.name, player.move.onlyAddForceWhiffCancelsOnFirstFrameOfSprite) == 0
				:
					true
				&& player.move.conditionForAddingWhiffCancels
					? player.move.conditionForAddingWhiffCancels(player)
					: true
		) {
			for (int i = 0; i < player.move.forceAddWhiffCancelsCount; ++i) {
				ForceAddedWhiffCancel* cancel = player.move.getForceAddWhiffCancel(i);
				int moveIndex = cancel->getMoveIndex(player.pawn);
				const AddedMoveData* move = base + indices[moveIndex];
				collectFrameCancelsPart(player, frame.whiffCancels, move, moveIndex);
			}
		}
	}
}

bool EndScene::checkMoveConditions(PlayerInfo& player, const AddedMoveData* move) {
	if (move->forceDisableFlags && (move->forceDisableFlags & player.wasForceDisableFlags) != 0) return false;
	int posY = player.pawn.posY();
	bool isLand = !player.airborne_insideTick();  // you may ask, well what about the last airborne, prelanding frame? On that frame, your y == 1. And at the end of the tick it's 0. (Are they hardcoding aroung their own code?..)
	if (move->minimumHeightRequirement && posY < move->minimumHeightRequirement) return false;
	if (move->characterState == MOVE_CHARACTER_STATE_JUMPING && isLand
			|| (move->characterState == MOVE_CHARACTER_STATE_CROUCHING
				|| move->characterState == MOVE_CHARACTER_STATE_STANDING)
			&& !isLand) return false;
	DWORD conditions[5];
	memcpy(conditions, move->conditions, 4*5);
	for (int i = 0; i < 5; ++i) {
		DWORD dword = conditions[i];
		while (dword) {
			DWORD bitIndex;  // 0 is LSB
			if (!_BitScanForward(&bitIndex, dword)) {
				break;
			}
			MoveCondition condition = (MoveCondition)(i * 32 + bitIndex);
			if (condition != MOVE_CONDITION_REQUIRES_25_TENSION
					&& condition != MOVE_CONDITION_REQUIRES_50_TENSION
					&& condition != MOVE_CONDITION_REQUIRES_100_TENSION
					&& condition != MOVE_CONDITION_IS_TOUCHING_LEFT_SCREEN_EDGE
					&& condition != MOVE_CONDITION_IS_TOUCHING_RIGHT_SCREEN_EDGE
					&& condition != MOVE_CONDITION_IS_TOUCHING_WALL
					&& condition != MOVE_CONDITION_CLOSE_SLASH
					&& condition != MOVE_CONDITION_FAR_SLASH) {
				if (BBScr_checkMoveConditionImpl && !BBScr_checkMoveConditionImpl((void*)player.pawn.ent, condition)) {
					return false;
				}
			}
			dword &= ~(1 << bitIndex);
		}
	}
	return true;
}

bool EndScene::whiffCancelIntoFDEnabled(PlayerInfo& player) {
	if (!findMoveByName || !player.wasEnableWhiffCancels) return false;
	if (!player.standingFDMove) {
		player.standingFDMove = player.findMoveByName("FaultlessDefenceStand");
	}
	if (!player.crouchingFDMove) {
		player.crouchingFDMove = player.findMoveByName("FaultlessDefenceCrouch");
	}
	if (!player.standingFDMove || !player.crouchingFDMove) return false;
	for (GatlingOrWhiffCancelInfo& cancel : player.wasCancels.whiffCancels) {
		if (player.standingFDMove == cancel.move || player.crouchingFDMove == cancel.move) {
			return true;
		}
	}
	return false;
}

bool EndScene::wasPlayerHadGatling(int playerIndex, const char* name) {
	for (const GatlingOrWhiffCancelInfo& cancel : players[playerIndex].wasCancels.gatlings) {
		if (strcmp(cancel.move->name, name) == 0) {
			return true;
		}
	}
	return false;
}

bool EndScene::wasPlayerHadWhiffCancel(int playerIndex, const char* name) {
	for (const GatlingOrWhiffCancelInfo& cancel : players[playerIndex].wasCancels.whiffCancels) {
		if (strcmp(cancel.move->name, name) == 0) {
			return true;
		}
	}
	return false;
}

bool EndScene::wasPlayerHadGatlings(int playerIndex) {
	return !players[playerIndex].wasCancels.gatlings.empty();
}

bool EndScene::wasPlayerHadWhiffCancels(int playerIndex) {
	return !players[playerIndex].wasCancels.whiffCancels.empty();
}

EntityFramebar& EndScene::getFramebar(int totalIndex) {
	if (totalIndex >= (int)playerFramebars.size()) {
		return projectileFramebars[totalIndex - playerFramebars.size()];
	} else {
		return playerFramebars[totalIndex];
	}
}

int EndScene::getSuperflashCounterOpponentCached() {
	return superflashCounterOpponent;
}

int EndScene::getSuperflashCounterAlliedCached() {
	return superflashCounterAllied;
}

int EndScene::getSuperflashCounterOpponentMax() {
	return superflashCounterOpponentMax;
}

int EndScene::getSuperflashCounterAlliedMax() {
	return superflashCounterAlliedMax;
}

Entity EndScene::getLastNonZeroSuperflashInstigator() {
	return lastNonZeroSuperflashInstigator;
}

bool EndScene::isHoldingFD(const PlayerInfo& player) const {
	return isHoldingFD(player.pawn);
}

bool EndScene::isHoldingFD(Entity pawn) const {
	char* trainingStruct = game.getTrainingHud();
	bool isDummy = isDummyPtr(trainingStruct, pawn.team());
	return !isDummy ? pawn.holdingFD() : *(DWORD*)(trainingStruct + 0x670 + 4 * pawn.team()) == 2;
}

void EndScene::onAfterAttackCopy(Entity defenderPtr, Entity attackerPtr) {
	if (iGiveUp || !defenderPtr.isPawn()) return;
	PlayerInfo& defender = findPlayer(defenderPtr);
	if (!defender.pawn) return;
	DWORD tickCount = getAswEngineTick();
	if (!defender.dmgCalcs.empty()) {
		if (defender.leftBlockstunHitstun) {
			defender.dmgCalcs.clear();
			defender.dmgCalcsSkippedHits = 0;
		} else {
			size_t maxCount;
			if (tickCount - defender.dmgCalcs.back().aswEngineCounter > 3) {
				maxCount = 10;
			} else {
				maxCount = 100;
			}
			if (defender.dmgCalcs.size() >= maxCount) {
				int countToErase = defender.dmgCalcs.size() - maxCount + 1;
				defender.dmgCalcsSkippedHits += countToErase;
				defender.dmgCalcs.erase(defender.dmgCalcs.begin(), defender.dmgCalcs.begin() + countToErase);
			}
		}
	}
	defender.leftBlockstunHitstun = false;
	defender.receivedNewDmgCalcOnThisFrame = true;
	defender.dmgCalcs.emplace_back();
	DmgCalc& newCalc = defender.dmgCalcs.back();
	newCalc.aswEngineCounter = tickCount;
	newCalc.hitResult = defenderPtr.lastHitResult();
	const AttackData* dealtAttack = attackerPtr.dealtAttack();
	const AttackData* attack = defenderPtr.receivedAttack();
	newCalc.isProjectile = attack->projectileLvl > 0;
	newCalc.guardType = attack->guardType;
	newCalc.airUnblockable = attack->airUnblockable();
	newCalc.guardCrush = attack->enableGuardBreak();
	newCalc.isThrow = attack->isThrow();
	if (newCalc.hitResult == HIT_RESULT_BLOCKED) {
		if (isHoldingFD(defenderPtr)) {
			newCalc.blockType = BLOCK_TYPE_FAULTLESS;
		} else if (defenderPtr.successfulIB()) {
			newCalc.blockType = BLOCK_TYPE_INSTANT;
		} else {
			newCalc.blockType = BLOCK_TYPE_NORMAL;
		}
	}
	
	defender.lastHitResult = newCalc.hitResult;
	defender.blockType = newCalc.blockType;
	
	if (attackerPtr.isPawn()) {
		PlayerInfo& attackerPlayer = findPlayer(attackerPtr);
		if (attackerPlayer.pawn) {
			attackerPlayer.fillInMove();
			attackerPlayer.determineMoveNameAndSlangName(&newCalc.attackName, &newCalc.attackSlangName);
		} else {
			PlayerInfo::determineMoveNameAndSlangName(attackerPtr, &newCalc.attackName, &newCalc.attackSlangName);
		}
		newCalc.nameFull = nullptr;
	} else {
		ProjectileInfo::determineMoveNameAndSlangName(attackerPtr, &newCalc.attackName, &newCalc.attackSlangName, &newCalc.nameFull);
	}
	if (newCalc.hitResult == HIT_RESULT_BLOCKED) {
		defender.blockedAHitOnThisFrame = true;
	}
	
	
	newCalc.attackType = attack->type;
	newCalc.attackLevel = attack->level;
	newCalc.attackOriginalAttackLevel = dealtAttack->level;
	newCalc.dealtOriginalDamage = dealtAttack->damage;
	static int standardDamages[] { 8, 18, 28, 40, 50, 55 };
	newCalc.standardDamage = standardDamages[newCalc.attackLevel];
	if (newCalc.dealtOriginalDamage == INT_MAX) {
		newCalc.dealtOriginalDamage = newCalc.standardDamage;
	}
	newCalc.scaleDamageWithHp = attack->scaleDamageWithHp();
	newCalc.isOtg = defenderPtr.isOtg();
	newCalc.ignoreOtg = attack->ignoreOtg();
	newCalc.oldHp = defenderPtr.hp();
	newCalc.maxHp = defenderPtr.maxHp();
	newCalc.adds5Dmg = (!newCalc.isOtg || newCalc.ignoreOtg)
		&& !attack->noDustScaling()
		&& (attackerPtr.dustPropulsion() || attackerPtr.unknownField1())
		&& newCalc.attackType == ATTACK_TYPE_NORMAL;
	newCalc.attackLevelForGuard = attack->atkLevelOnBlockOrArmor;
	if (newCalc.attackLevelForGuard == -1) {
		newCalc.attackLevelForGuard = newCalc.attackOriginalAttackLevel;
	}
	
	if (newCalc.hitResult == HIT_RESULT_BLOCKED && newCalc.blockType != BLOCK_TYPE_FAULTLESS) {
		DmgCalc::DmgCalcU::DmgCalcBlock& data = newCalc.u.block;
		data.defenderRisc = defenderPtr.risc();
		data.riscPlusBase = attack->riscPlus;
		data.attackLevel = dealtAttack->level;
		static int riscPlusStandard[] = { 3, 6, 10, 14, 20 };
		data.riscPlusBaseStandard = riscPlusStandard[newCalc.attackLevelForGuard];
		data.guardBalanceDefence = defenderPtr.guardBalanceDefence();
		defender.pawn = defenderPtr;
		GuardType guardType = attack->guardType;
		data.groundedAndOverheadOrLow = !defender.airborne_insideTick()
			&& (guardType == GUARD_TYPE_HIGH || guardType == GUARD_TYPE_LOW);
		data.wasInBlockstun = defenderPtr.blockstun() != 0;
		
		data.baseDamage = attack->damage;
		data.attackKezuri = attack->attackKezuri;
		data.attackKezuriStandard = 0;
		if (newCalc.attackType == ATTACK_TYPE_EX || newCalc.attackType == ATTACK_TYPE_OVERDRIVE) {
			data.attackKezuriStandard = 16;
		}
	} else if (newCalc.hitResult == HIT_RESULT_ARMORED || newCalc.hitResult == HIT_RESULT_ARMORED_BUT_NO_DMG_REDUCTION) {
		DmgCalc::DmgCalcU::DmgCalcArmor& data = newCalc.u.armor;
		data.baseDamage = attack->damage;
		data.damageScale = attackerPtr.playerEntity().damageScale();
		data.isProjectile = attack->projectileLvl > 0;
		data.projectileDamageScale = defenderPtr.projectileDamageScale();
		data.superArmorDamagePercent = defenderPtr.superArmorDamagePercent();
		data.superArmorHeadAttribute = (defenderPtr.lastHitResultFlags() & 0x400) != 0;
		
		data.defenseModifier = defenderPtr.defenseModifier();
		data.gutsRating = defenderPtr.gutsRating();
		data.guts = defenderPtr.calculateGuts(&data.gutsLevel);
		
		data.attackKezuri = attack->attackKezuri;
		data.attackKezuriStandard = 0;
		if (newCalc.attackType == ATTACK_TYPE_EX || newCalc.attackType == ATTACK_TYPE_OVERDRIVE) {
			data.attackKezuriStandard = 16;
		}
	}
}

void EndScene::onDealHit(Entity defenderPtr, Entity attackerPtr) {
	if (iGiveUp || !defenderPtr.isPawn()) return;
	PlayerInfo& defender = findPlayer(defenderPtr);
	if (!defender.pawn || defender.dmgCalcs.empty() || defender.dmgCalcs.back().hitResult != HIT_RESULT_NORMAL) return;
	DmgCalc& dmgCalc = defender.dmgCalcs.back();
	DmgCalc::DmgCalcU::DmgCalcHit& data = dmgCalc.u.hit;
	const AttackData* attack = defenderPtr.receivedAttack();
	data.baseDamage = attack->damage;
	data.increaseDmgBy50Percent = defenderPtr.increaseDmgBy50Percent();
	data.extraInverseProration = defenderPtr.extraInverseProration();
	data.isStylish = game.isStylish(defenderPtr);
	data.stylishDamageInverseModifier = game.getStylishDefenseInverseModifier();
	data.handicap = game.getHandicap(defenderPtr.team());
	switch (data.handicap) {
		case 0: data.handicap = 156; break;
		case 1: data.handicap = 125; break;
		case 2: data.handicap = 100; break;
		case 3: data.handicap = 80; break;
		case 4: data.handicap = 64; break;
		default: data.handicap = 100;
	}
	
	if (attack->type == ATTACK_TYPE_OVERDRIVE && !dmgCalc.isOtg) {
		bool superHasBeenGoingOnForTooLong = false;
		bool superCanIgnoreBeingTooLong = false;
		
		bool attackerPerformingSuper = false;
		PlayerInfo& attacker = findPlayer(attackerPtr.playerEntity());
		if (attacker.pawn) {
			attackerPerformingSuper = attacker.performingASuper;
			superHasBeenGoingOnForTooLong = attacker.activesDisp.count > 2 || attacker.activesDisp.total() > 14 || attacker.recovery;
			superCanIgnoreBeingTooLong = attacker.startedSuperWhenComboing;
		}
		
		bool isADisabledProjectile = false;
		if (!attackerPtr.isPawn()) {
			ProjectileInfo& projectile = findProjectile(attackerPtr);
			if (projectile.ptr && projectile.disabled && attackerPtr.lifeTimeCounter() != 0) {
				isADisabledProjectile = true;
			}
		}
		if (!isADisabledProjectile
				&& attackerPerformingSuper
				&& !(
					superHasBeenGoingOnForTooLong
					&& !superCanIgnoreBeingTooLong)) {
			defender.gettingHitBySuper = true;
		}
	}
	data.damageScale = attackerPtr.playerEntity().damageScale();
	data.isProjectile = attack->projectileLvl > 0;
	data.projectileDamageScale = defenderPtr.projectileDamageScale();
	
	data.dustProration1 = defenderPtr.dustProration1();
	data.dustProration2 = defenderPtr.dustProration2();
	
	data.attackerHellfireState = attackerPtr.playerEntity().hellfireState();
	data.attackerHpLessThan10Percent = attackerPtr.playerEntity().hp() * 10000 / 420 <= 1000;
	data.attackHasHellfireEnabled = attack->hellfire();
	
	data.attackCounterHitType = attack->counterHitType;
	data.trainingSettingIsForceCounterHit = false;
	if (game.isTrainingMode()) {
		TrainingSettingValue_CounterHit settingValue = (TrainingSettingValue_CounterHit)game.getTrainingSetting(TRAINING_SETTING_COUNTER_HIT);
		if (settingValue == TRAINING_SETTING_VALUE_COUNTER_HIT_FORCED_MORTAL_COUNTER) {
			data.trainingSettingIsForceCounterHit = true;
		}
	}
	data.dangerTime = defenderPtr.counterHit() == COUNTER_HIT_ENTITY_VALUE_MORTAL_COUNTER || game.getDangerTime() != 0;
	
	data.wasHitDuringRc = attack->wasHitDuringRc();
	data.rcDmgProration = defenderPtr.rcDmgProration() != 0;
	data.proration = defenderPtr.proration();
	data.initialProration = attack->initialProration;
	data.forcedProration = attack->forcedProration;
	data.needReduceRisc = attackerPtr.defendersRisc() == INT_MAX || attack->prorationTandan();
	if (data.needReduceRisc) {
		data.risc = defenderPtr.risc();
		data.guardBreakInitialProrationApplied = defenderPtr.guardBreakInitialProrationApplied();
		data.isFirstHit = defenderPtr.comboCount() == 0 && !data.guardBreakInitialProrationApplied;
		data.riscMinusStarter = attack->riscMinusStarter;
		data.riscMinusOnceUsed = defenderPtr.riscMinusOnceUsed() != 0;
		data.riscMinusOnce = attack->riscMinusOnce;
		data.riscMinus = attack->riscMinus;
	} else {
		data.risc = attackerPtr.defendersRisc();
	}
	data.comboProration = entityManager.calculateComboProration(data.risc, attack->type);
	data.noDamageScaling = attack->noDamageScaling();
	data.minimumDamagePercent = attack->minimumDamagePercent;
	
	data.defenseModifier = defenderPtr.defenseModifier();
	data.gutsRating = defenderPtr.gutsRating();
	data.guts = defenderPtr.calculateGuts(&data.gutsLevel);
	
	data.kill = attack->killType == KILL_TYPE_KILL
		|| attack->killType == KILL_TYPE_KILL_ALLY && attackerPtr.team() == defenderPtr.team();
	
	data.originalAttackStun = attackerPtr.dealtAttack()->stun;
	data.throwLockExecute = attack->throwLockExecute();
}

void EndScene::onAfterDealHit(Entity defenderPtr, Entity attackerPtr) {
	if (iGiveUp || !defenderPtr.isPawn()) return;
	PlayerInfo& defender = findPlayer(defenderPtr);
	if (!defender.pawn || defender.dmgCalcs.empty() || defender.dmgCalcs.back().hitResult != HIT_RESULT_NORMAL) return;
	DmgCalc::DmgCalcU::DmgCalcHit& data = defender.dmgCalcs.back().u.hit;
	const AttackData* attack = defenderPtr.receivedAttack();
	data.baseStun = attack->stun;
	data.comboCount = defenderPtr.comboCount();
	data.counterHit = defenderPtr.counterHit();
	data.tensionMode = defenderPtr.enemyEntity().tensionMode();
	data.oldStun = defenderPtr.stun();
	data.stunMax = defenderPtr.stunThreshold();
}

bool EndScene::drawingPostponed() const {
	return settings.dodgeObsRecording && endSceneAndPresentHooked && !obsStoppedCapturing;
}

#ifdef LOG_PATH
bool loggedDrawingInputsOnce
#endif
void EndScene::prepareInputs() {
	InputRingBuffer* sourceBuffers = game.getInputRingBuffers();
	if (!sourceBuffers) return;
	for (int i = 0; i < 2; ++i) {
		inputRingBuffersStored[i].update(sourceBuffers[i], prevInputRingBuffers[i]);
		std::vector<InputsDrawingCommandRow>& result = drawDataPrepared.inputs[i];
		if (result.size() != 100) result.resize(100);
		drawDataPrepared.inputsSize[i] = 0;
		memset(result.data(), 0, 100 * sizeof InputsDrawingCommandRow);
		inputsDrawing.produceData(inputRingBuffersStored[i], result.data(), drawDataPrepared.inputsSize + i, i == 1);
	}
	memcpy(prevInputRingBuffers, sourceBuffers, sizeof prevInputRingBuffers);
	#ifdef LOG_PATH
	loggedDrawingInputsOnce = true;
	#endif
}

const std::vector<SkippedFramesInfo>& EndScene::getSkippedFrames(bool hitstop) const {
	return hitstop ? skippedFramesHitstop : skippedFrames;
}

void SkippedFramesInfo::addFrame(SkippedFramesType type) {
	if (overflow) {
		++elements[0].count;
		return;
	}
	if (count == _countof(elements)) {
		if (elements[_countof(elements) - 1].type == type) {
			++elements[_countof(elements) - 1].count;
		} else {
			transitionToOverflow();
			addFrame(type);
			return;
		}
	} else if (count && elements[count - 1].type == type) {
		++elements[count - 1].count;
	} else {
		elements[count].type = type;
		elements[count].count = 1;
		++count;
	}
}

void SkippedFramesInfo::clear() {
	count = 0;
	overflow = false;
}

void SkippedFramesInfo::transitionToOverflow() {
	overflow = true;
	for (int i = 1; i < count; ++i) {
		elements[0].count += elements[i].count;
	}
}

void EndScene::HookHelp::setSuperFreezeAndRCSlowdownFlagsHook() {
	endScene.setSuperFreezeAndRCSlowdownFlagsHook((char*)this);
}

// This function determines if you're superfrozen or slowed down by RC
// It runs before hitDetectionMain
// On hit RC slowdown from mortal counter may be applied, or RC slowdown may be cancelled by starting a super or IK
// Initially RC is set after an RC animation finishes its superfreeze and does an idling event
// But if both players YRC at 1f offset from each other, it may take more than 1f after the superfreeze is over for the idling event to occur and initial RC to be set
// So, some RC changes happen after RC slowdown is already decided
// Even if not interrupted by anything, RC counter decrements by 1 after this function is already over
// We're seeing obsolete values at the end of a tick that don't matter anymore
// And there're too many ways for those values to change
// That's why we need these hooks
void EndScene::setSuperFreezeAndRCSlowdownFlagsHook(char* asw_subengine) {
	
	for (PlayerInfo& player : players) {
		player.rcSlowedDown = false;
		player.rcSlowedDownCounter = 0;
		player.rcSlowedDownMax = 0;
	}
	for (ProjectileInfo& projectile : projectiles) {
		projectile.rcSlowedDown = false;
		projectile.rcSlowedDownCounter = 0;
		projectile.rcSlowedDownMax = 0;
	}
	
	entityList.populate();
	Entity superflashInstigator = getSuperflashInstigator();
	for (int i = 0; i < 2; ++i) {
		PlayerInfo& player = players[i];
		Entity pawn = entityList.slots[i];
		player.rcSlowdownCounter = pawn.rcSlowdownCounter();
		player.rcSlowdownMax = player.rcSlowdownMaxLastSet;
		if (player.rcSlowdownCounter) {
			for (int j = 0; j < entityList.count; ++j) {
				Entity ent = entityList.list[j];
				if (ent != pawn && ent != superflashInstigator && !ent.immuneToRCSlowdown()) {
					ProjectileInfo& foundProjectile = findProjectile(ent);
					if (foundProjectile.ptr) {
						foundProjectile.rcSlowedDown = true;
						foundProjectile.rcSlowedDownCounter = max(foundProjectile.rcSlowedDownCounter, player.rcSlowdownCounter);
						foundProjectile.rcSlowedDownMax = max(foundProjectile.rcSlowedDownMax, player.rcSlowdownMax);
					} else {
						PlayerInfo& foundPlayer = findPlayer(ent);
						if (foundPlayer.pawn) {
							foundPlayer.rcSlowedDown = true;
							foundPlayer.rcSlowedDownCounter = max(foundProjectile.rcSlowedDownCounter, player.rcSlowdownCounter);
							foundPlayer.rcSlowedDownMax = max(foundProjectile.rcSlowedDownMax, player.rcSlowdownMax);
						}
					}
				}
			}
		}
	}
	orig_setSuperFreezeAndRCSlowdownFlags(asw_subengine);
}

void EndScene::HookHelp::BBScr_timeSlowHook(int duration) {
	endScene.BBScr_timeSlowHook(Entity{(void*)this}, duration);
}

void EndScene::BBScr_timeSlowHook(Entity pawn, int duration) {
	if (duration != 0) {
		PlayerInfo& player = findPlayer(pawn);
		player.rcSlowdownMaxLastSet = duration;
	}
	orig_BBScr_timeSlow((void*)pawn.ent, duration);
}

void EndScene::HookHelp::onCmnActXGuardLoopHook(int signal, int type, int thisIs0) {
	endScene.onCmnActXGuardLoopHook(Entity{(void*)this}, signal, type, thisIs0);
}

void EndScene::onCmnActXGuardLoopHook(Entity pawn, int signal, int type, int thisIs0) {
	int oldBlockstun = pawn.blockstun();
	orig_onCmnActXGuardLoop((void*)pawn.ent, signal, type, thisIs0);
	int newBlockstun = pawn.blockstun();
	
	if (newBlockstun == oldBlockstun + 4) {
		PlayerInfo& player = findPlayer(pawn);
		player.blockstunMaxLandExtra = 4;
	}
}
