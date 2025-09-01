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
#include "WError.h"
#include "UI.h"
#include "CustomWindowMessages.h"
#include "Hud.h"
#include "Moves.h"
#include "findMoveByName.h"
#include <mutex>
#include "InputsDrawing.h"
#include "InputNames.h"
#include "SpecificFramebarIds.h"
#include <chrono>
#include "NamePairManager.h"
#include <unordered_map>

EndScene endScene;
PlayerInfo emptyPlayer {0};
ProjectileInfo emptyProjectile;
findMoveByName_t findMoveByName = nullptr;


// Runs on the main thread
extern "C"
void __cdecl drawQuadExecHook(FVector2D* screenSize, REDDrawQuadCommand* item, void* canvas);  // defined here
extern "C"
void __cdecl increaseStunHook(Entity pawn, int stunAdd);  // defined here

// Runs on the main thread
extern "C"
void __cdecl drawQuadExecHookAsm(REDDrawQuadCommand* item, void* canvas);  // defined in asmhooks.asm

// Runs on the main thread
extern "C"
void __cdecl call_orig_drawQuadExec(void* orig_drawQuadExec, FVector2D *screenSize, REDDrawQuadCommand *item, void* canvas);  // defined in asmhooks.asm

// Runs on the main thread
extern "C"
void __cdecl increaseStunHookAsm();  // defined in asmhooks.asm

static int __cdecl LifeTimeCounterCompare(void const*, void const*);

extern "C" DWORD restoreDoubleJumps = 0;  // for use by jumpInstallNormalJumpHookAsm
extern "C" void jumpInstallNormalJumpHookAsm(void* pawn);  // defined in asmhooks.asm
extern "C" void __fastcall jumpInstallNormalJumpHook(void* pawn);  // defined here
extern "C" DWORD restoreAirDash = 0;  // for use by jumpInstallSuperJumpHookAsm
extern "C" void jumpInstallSuperJumpHookAsm(void* pawn);  // defined in asmhooks.asm
extern "C" void __fastcall jumpInstallSuperJumpHook(void* pawn);  // defined here

static inline bool isDizzyBubble(const char* name) {
	return (*(DWORD*)name & 0xffffff) == ('A' | ('w'<<8) | ('a'<<16))
		&& *(DWORD*)(name + 4) == ('O' | ('b'<<8) | ('j'<<16));
}
static inline bool isGrenadeBomb(const char* name) {
	return *(DWORD*)name == ('G' | ('r'<<8) | ('e'<<16) | ('n'<<24))
		&& *(DWORD*)(name + 4) == ('a' | ('d'<<8) | ('e'<<16) | ('B'<<24))
		&& *(DWORD*)(name + 8) == ('o' | ('m'<<8) | ('b'<<16));
}
static inline bool isVenomBall(const char* name) {
	return *(DWORD*)name == ('B' | ('a'<<8) | ('l'<<16) | ('l'<<24))
		&& *(BYTE*)(name + 4) == 0;
}
static void increaseFramesCountUnlimited(int& counterUnlimited, int incrBy, int displayedFrames);

template<typename T>
static inline const T& max_inline(const T& a, const T& b) {
	return a > b ? a : b;
}

template<typename T>
static inline const T& min_inline(const T& a, const T& b) {
	return a < b ? a : b;
}

bool EndScene::onDllMain() {
	bool error = false;
	
	orig_WndProc = (WNDPROC)sigscanOffset(
		GUILTY_GEAR_XRD_EXE,
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
		&error, "TrainingEtc_OneDamage", nullptr);
	if (!error) {
		std::vector<char> sig;//[9] { "\xc7\x40\x28\x00\x00\x00\x00\xe8" };
		std::vector<char> mask;
		std::vector<char> maskForCaching;
		// ghidra sig: c7 40 28 ?? ?? ?? ?? e8
		byteSpecificationToSigMask("c7 40 28 rel(?? ?? ?? ??) e8", sig, mask, nullptr, 0, &maskForCaching);
		substituteWildcard(sig, mask, 0, (void*)TrainingEtc_OneDamage);
		
		uintptr_t drawTextWithIconsCall = sigscanBufOffset(
			GUILTY_GEAR_XRD_EXE,
			sig.data(),
			sig.size() - 1,
			{ 7 },
			&error, "drawTextWithIconsCall", maskForCaching.data());
		if (drawTextWithIconsCall) {
			drawTextWithIcons = (drawTextWithIcons_t)followRelativeCall(drawTextWithIconsCall);
			logwrap(fprintf(logfile, "drawTextWithIcons: %p\n", drawTextWithIcons));
			orig_drawTrainingHud = game.trainingHudTick;
			logwrap(fprintf(logfile, "orig_drawTrainingHud final location: %p\n", (void*)orig_drawTrainingHud));
		}
		if (orig_drawTrainingHud) {
			auto drawTrainingHudHookPtr = &HookHelp::drawTrainingHudHook;
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
		&error, "CanvasFlushSetupCommandString", nullptr);
	uintptr_t CanvasFlushSetupCommand_DescribeCommand = 0;
	if (CanvasFlushSetupCommandStringAddr) {
		std::vector<char> sig;
		std::vector<char> mask;
		std::vector<char> maskForCaching;
		// ghidra sig: b8 ?? ?? ?? ?? c3
		byteSpecificationToSigMask("b8 rel(?? ?? ?? ??) c3",
			sig, mask, nullptr, 0, &maskForCaching);
		substituteWildcard(sig, mask, 0, (void*)CanvasFlushSetupCommandStringAddr);
		CanvasFlushSetupCommand_DescribeCommand = sigscanOffset(
			GUILTY_GEAR_XRD_EXE,
			sig,
			mask,
			&error, "CanvasFlushSetupCommand_DescribeCommand", maskForCaching.data());
	}
	uintptr_t CanvasFlushSetupCommandVtable = 0;
	if (CanvasFlushSetupCommand_DescribeCommand) {
		CanvasFlushSetupCommandVtable = sigscanBufOffset(
			"GuiltyGearXrd.exe:.rdata",
			(const char*)&CanvasFlushSetupCommand_DescribeCommand,
			4,
			{ -8 },
			&error, "CanvasFlushSetupCommandVtable", "rel_GuiltyGearXrd.exe(????)");
	}
	uintptr_t CanvasFlushSetupCommandCreationPlace = 0;
	if (CanvasFlushSetupCommandVtable) {
		std::vector<char> sig;
		std::vector<char> mask;
		std::vector<char> maskForCaching;
		// ghidra sig: c7 ?? ?? ?? ?? ??
		byteSpecificationToSigMask("c7 ?? rel(?? ?? ?? ??)", sig, mask, nullptr, 0, &maskForCaching);
		strcpy(mask.data() + 2, "xxxx");
		memcpy(sig.data() + 2, &CanvasFlushSetupCommandVtable, 4);
		CanvasFlushSetupCommandCreationPlace = sigscanOffset(
			GUILTY_GEAR_XRD_EXE,
			sig,
			mask,
			&error, "CanvasFlushSetupCommandCreationPlace", maskForCaching.data());
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
	uintptr_t TryEnterCriticalSectionPtr = findImportedFunction(GUILTY_GEAR_XRD_EXE, "KERNEL32.DLL", "TryEnterCriticalSection");
	uintptr_t EnterCriticalSectionPtr = findImportedFunction(GUILTY_GEAR_XRD_EXE, "KERNEL32.DLL", "EnterCriticalSection");
	uintptr_t LeaveCriticalSectionPtr = findImportedFunction(GUILTY_GEAR_XRD_EXE, "KERNEL32.DLL", "LeaveCriticalSection");
	{
		std::vector<char> sig;
		std::vector<char> unused;
		std::vector<char> maskForCaching;
		size_t byteSpecPositions[3] { 0, 0, 0 };
		// pointers are in the game's image, not in kernel32.dll, even though they point to kernel32.dll
		byteSpecificationToSigMask("e8 ?? ?? ?? ?? 8b ?? >rel(?? ?? ?? ??) 8b ?? >rel(?? ?? ?? ??) 8b ?? >rel(?? ?? ?? ??)",
			sig, unused, byteSpecPositions, 0, &maskForCaching);
		memcpy(sig.data() + byteSpecPositions[0], &TryEnterCriticalSectionPtr, 4);
		memcpy(sig.data() + byteSpecPositions[1], &EnterCriticalSectionPtr, 4);
		memcpy(sig.data() + byteSpecPositions[2], &LeaveCriticalSectionPtr, 4);
		orig_REDAnywhereDispDraw = (REDAnywhereDispDraw_t)sigscanOffset(
			GUILTY_GEAR_XRD_EXE,
			sig.data(),
			"x????x?xxxxx?xxxxx?xxxx",
			{ -0x4 },
			&error, "REDAnywhereDispDraw", maskForCaching.data());
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
			drawQuadExecHookAsm,  // defined in asmhooks.asm
			"drawQuadExec")) return false;
	}
	
	uintptr_t superflashInstigatorUsage = sigscanOffset(
		GUILTY_GEAR_XRD_EXE,
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
		auto setSuperFreezeAndRCSlowdownFlagsHookPtr = &HookHelp::setSuperFreezeAndRCSlowdownFlagsHook;
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
		GUILTY_GEAR_XRD_EXE,
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
		
	#define digUpBBScrFunctionAndHook(type, name, index) \
		digUpBBScrFunction(type, orig_##name, index) \
		if (orig_##name) { \
			auto name##HookPtr = &HookHelp::name##Hook; \
			if (!detouring.attach(&(PVOID&)orig_##name, \
				(PVOID&)name##HookPtr, \
				#name)) return false; \
		}
	
	digUpBBScrFunctionAndHook(BBScr_createObjectWithArg_t, BBScr_createObjectWithArg, instr_createObjectWithArg)
	digUpBBScrFunctionAndHook(BBScr_createObjectWithArg_t, BBScr_createObject, instr_createObject)
	digUpBBScrFunctionAndHook(BBScr_createParticleWithArg_t, BBScr_createParticleWithArg, instr_createParticleWithArg)
	digUpBBScrFunctionAndHook(BBScr_linkParticle_t, BBScr_linkParticle, instr_linkParticle)
	digUpBBScrFunctionAndHook(BBScr_linkParticle_t, BBScr_linkParticleWithArg2, instr_linkParticleWithArg2)
	digUpBBScrFunctionAndHook(BBScr_runOnObject_t, BBScr_runOnObject, instr_runOnObject)
	digUpBBScrFunctionAndHook(BBScr_sendSignal_t, BBScr_sendSignal, instr_sendSignal)
	digUpBBScrFunctionAndHook(BBScr_sendSignalToAction_t, BBScr_sendSignalToAction, instr_sendSignalToAction)
	digUpBBScrFunction(BBScr_callSubroutine_t, BBScr_callSubroutine, instr_callSubroutine)
	uintptr_t BBScr_createArgHikitsukiVal = 0;
	digUpBBScrFunction(uintptr_t, BBScr_createArgHikitsukiVal, instr_createArgHikitsukiVal)
	uintptr_t BBScr_checkMoveCondition = 0;
	digUpBBScrFunction(uintptr_t, BBScr_checkMoveCondition, instr_checkMoveCondition)
	uintptr_t BBScr_whiffCancelOptionBufferTime = 0;
	digUpBBScrFunction(uintptr_t, BBScr_whiffCancelOptionBufferTime, instr_whiffCancelOptionBufferTime)
	digUpBBScrFunctionAndHook(BBScr_setHitstop_t, BBScr_setHitstop, instr_setHitstop)
	digUpBBScrFunctionAndHook(BBScr_ignoreDeactivate_t, BBScr_ignoreDeactivate, instr_ignoreDeactivate)
	digUpBBScrFunctionAndHook(BBScr_timeSlow_t, BBScr_timeSlow, instr_timeSlow)
	
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
			substituteWildcard(sig, mask, 0, game.gameDataPtr);
			ptr = sigscanForward(ptr, sig, mask, 0x40);
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
		substituteWildcard(sig, mask, 0, aswEngine);
		uintptr_t aswEngUsage = sigscanForward(entryPtr, sig, mask, 0x1e);
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
		} else {
			logwrap(fputs("Could not find findMoveByName: call place for it not found.\n", logfile));
			error = true;
		}
	} else {
		logwrap(fputs("Could not find findMoveByName: BBScr_whiffCancelOptionBufferTime not found.\n", logfile));
		error = true;
	}
	if (!findMoveByName) return false;
	
	uintptr_t PawnVtable = sigscanOffset(
		GUILTY_GEAR_XRD_EXE,
		"c7 ?? ?? ?? ?? ?? 66 0f ef c0 8d ?? ?? ?? ?? ?? ?? 66 0f d6 ?? ?? ?? ?? ?? ?? 89 ?? ?? ?? 66 0f d6 ?? ?? ?? ?? ?? e8",
		{ 2, 0 },
		&error, "PawnVtable");
	uintptr_t getAccessedValueAddr = 0;
	if (PawnVtable) {
		orig_setAnim = *(setAnim_t*)(PawnVtable + 0x44);
		orig_pawnInitialize = *(pawnInitialize_t*)(PawnVtable);
		orig_checkFirePerFrameUponsWrapper = *(checkFirePerFrameUponsWrapper_t*)(PawnVtable + 0x10);
		
		auto checkFirePerFrameUponsWrapperHookPtr = &HookHelp::checkFirePerFrameUponsWrapperHook;
		if (!detouring.attach(&(PVOID&)orig_checkFirePerFrameUponsWrapper,
			(PVOID&)checkFirePerFrameUponsWrapperHookPtr,
			"checkFirePerFrameUponsWrapper")) return false;
		
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
		getAccessedValueJumptable = *(uintptr_t*)(getAccessedValueImplJumptableUse + 3);
		uintptr_t interroundValueStorage2CodeStart = *(uintptr_t*)(getAccessedValueJumptable + BBSCRVAR_INTERROUND_VALUE_STORAGE2 * 4);
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
		
		uintptr_t IsBossCodeStart = *(uintptr_t*)(getAccessedValueJumptable + BBSCRVAR_IS_BOSS * 4);
		if (memcmp((void*)IsBossCodeStart, "\x8b\xce", 2) == 0) {
			IsBossCodeStart += 2;
		}
		orig_Pawn_ArcadeMode_IsBoss = (Pawn_ArcadeMode_IsBoss_t)followRelativeCall(IsBossCodeStart);
	}
	
	if (orig_setAnim) {
		auto setAnimHookPtr = &HookHelp::setAnimHook;
		if (!detouring.attach(&(PVOID&)orig_setAnim,
			(PVOID&)setAnimHookPtr,
			"setAnim")) return false;
	}
	
	if (orig_pawnInitialize) {
		auto pawnInitializeHookPtr = &HookHelp::pawnInitializeHook;
		if (!detouring.attach(&(PVOID&)orig_pawnInitialize,
			(PVOID&)pawnInitializeHookPtr,
			"pawnInitialize")) return false;
	}
	
	if (orig_handleUpon) {
		auto handleUponHookPtr = &HookHelp::handleUponHook;
		if (!detouring.attach(&(PVOID&)orig_handleUpon,
			(PVOID&)handleUponHookPtr,
			"handleUpon")) return false;
	}
	
	uintptr_t logicOnFrameAfterHitPiece = sigscanOffset(
		GUILTY_GEAR_XRD_EXE,
		"77 10 89 ?? b4 01 00 00 c7 ?? b8 01 00 00 02 00 00 00",
		&error, "logicOnFrameAfterHitPiece");
	if (logicOnFrameAfterHitPiece) {
		orig_logicOnFrameAfterHit = (logicOnFrameAfterHit_t)scrollUpToBytes((char*)logicOnFrameAfterHitPiece,
			"\x83\xec\x14", 3);
	}
	if (orig_logicOnFrameAfterHit) {
		auto logicOnFrameAfterHitHookPtr = &HookHelp::logicOnFrameAfterHitHook;
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
		&error, "iconStringAddr", nullptr);
	uintptr_t iconStringUsage = 0;
	if (iconStringAddr) {
		std::vector<char> sig;
		std::vector<char> mask;
		std::vector<char> maskForCaching;
		// ghidra sig: 68 ?? ?? ?? ?? e8
		byteSpecificationToSigMask("68 rel(?? ?? ?? ??) e8", sig, mask, nullptr, 0, &maskForCaching);
		substituteWildcard(sig, mask, 0, (void*)iconStringAddr);
		iconStringUsage = sigscanOffset(
			GUILTY_GEAR_XRD_EXE,
			sig,
			mask,
			&error, "iconStringUsage", maskForCaching.data());
	}
	if (iconStringUsage) {
		iconStringUsage = sigscanForward(iconStringUsage, "a3");
	}
	if (iconStringUsage) {
		iconTexture = *(void**)(iconStringUsage + 1);
	}
	if (!iconTexture) {
		error = true;
		logwrap(fputs("iconTexture not found.\n", logfile));
	}
	
	uintptr_t backPushbackApplierPiece = sigscanOffset(
		GUILTY_GEAR_XRD_EXE,
		"85 90 18 01 00 00 74 08 01 b0 30 03 00 00 eb 06 89 a8 30 03 00 00",
		nullptr, "backPushbackApplierPiece");
	if (backPushbackApplierPiece) {
		orig_backPushbackApplier = (backPushbackApplier_t)sigscanBackwards(backPushbackApplierPiece,
			"83 ec ?? 53 55 56 57");
	}
	if (orig_backPushbackApplier) {
		auto backPushbackApplierHookPtr = &HookHelp::backPushbackApplierHook;
		if (!detouring.attach(&(PVOID&)orig_backPushbackApplier,
			(PVOID&)backPushbackApplierHookPtr,
			"backPushbackApplier")) return false;
	}
	
	uintptr_t pushbackStunOnBlockPiece = sigscanOffset(
		GUILTY_GEAR_XRD_EXE,
		"f7 d8 89 83 1c 03 00 00 8b 86 08 07 00 00 8b 88 08 07 00 00",
		nullptr, "pushbackStunOnBlockPiece");
	if (pushbackStunOnBlockPiece) {
		isDummyPtr = (isDummy_t)followRelativeCall(pushbackStunOnBlockPiece - 0xd7);
		orig_pushbackStunOnBlock = (pushbackStunOnBlock_t)sigscanBackwards(pushbackStunOnBlockPiece,
			"83 ec ?? 53 55 56 57");
	}
	if (orig_pushbackStunOnBlock) {
		auto pushbackStunOnBlockHookPtr = &HookHelp::pushbackStunOnBlockHook;
		if (!detouring.attach(&(PVOID&)orig_pushbackStunOnBlock,
			(PVOID&)pushbackStunOnBlockHookPtr,
			"pushbackStunOnBlock")) return false;
	}
	
	uintptr_t OnPreSkillCheckStrAddr = sigscanStrOffset(
		"GuiltyGearXrd.exe:.rdata",
		"OnPreSkillCheck",
		&error, "OnPreSkillCheckStrAddr", nullptr);
	uintptr_t OnPreSkillCheckVar = 0;
	if (OnPreSkillCheckStrAddr) {
		std::vector<char> sig;
		std::vector<char> mask;
		std::vector<char> maskForCaching;
		// 68 ?? ?? ?? ??
		byteSpecificationToSigMask("68 rel(?? ?? ?? ??)", sig, mask, nullptr, 0, &maskForCaching);
		substituteWildcard(sig, mask, 0, (void*)OnPreSkillCheckStrAddr);
		OnPreSkillCheckVar = sigscanOffset(
			GUILTY_GEAR_XRD_EXE,
			sig,
			mask,
			{ 6, 0 },
			&error, "OnPreSkillCheckVar", maskForCaching.data());
	}
	if (OnPreSkillCheckVar) {
		std::vector<char> sig;
		std::vector<char> mask;
		std::vector<char> maskForCaching;
		// ghidra sig: 68 ?? ?? ?? ??
		byteSpecificationToSigMask("68 rel(?? ?? ?? ??)", sig, mask, nullptr, 0, &maskForCaching);
		substituteWildcard(sig, mask, 0, (void*)OnPreSkillCheckVar);
		uintptr_t jmpInstrAddr = sigscanOffset(
			GUILTY_GEAR_XRD_EXE,
			sig,
			mask,
			{ 0x1e },
			&error, "skillCheckPiece", maskForCaching.data());
		if (jmpInstrAddr) {
			orig_skillCheckPiece = (skillCheckPiece_t)followRelativeCall(jmpInstrAddr);
		}
	}
	if (orig_skillCheckPiece) {
		auto skillCheckPieceHookPtr = &HookHelp::skillCheckPieceHook;
		if (!detouring.attach(&(PVOID&)orig_skillCheckPiece,
			(PVOID&)skillCheckPieceHookPtr,
			"skillCheckPiece")) return false;
	}
	
	// To find this function you can just data breakpoint hitstop being set
	orig_beginHitstop = (beginHitstop_t)sigscanOffset(
		GUILTY_GEAR_XRD_EXE,
		"83 b9 b8 01 00 00 00 74 16 8b 81 b4 01 00 00 c7 81 b8 01 00 00 00 00 00 00 89 81 ac 01 00 00 33 c0 c3",
		&error,
		"beginHitstop");
	if (orig_beginHitstop) {
		auto beginHitstopHookPtr = &HookHelp::beginHitstopHook;
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
		GUILTY_GEAR_XRD_EXE,
		"8b 86 54 4d 00 00 8b ce 3b c5 7e 16 83 c0 04 89 86 54 4d 00 00",
		&error, "onCmnActXGuardLoop");
	if (onCmnActXGuardLoopPiece) {
		orig_onCmnActXGuardLoop = (onCmnActXGuardLoop_t)sigscanBackwards(onCmnActXGuardLoopPiece,
			"83 ec ?? a1 ?? ?? ?? ?? 33 c4", 0x616);
	}
	if (orig_onCmnActXGuardLoop) {
		auto onCmnActXGuardLoopHookPtr = &HookHelp::onCmnActXGuardLoopHook;
		if (!detouring.attach(&(PVOID&)orig_onCmnActXGuardLoop,
			(PVOID&)onCmnActXGuardLoopHookPtr,
			"onCmnActXGuardLoop")) return false;
	}
	
	uintptr_t atkIconNamesLoc = 0;
	static const char atkSig[] = "Atk1\x00\x00\x00\x00"
		"Atk2\x00\x00\x00\x00"
		"Atk3\x00\x00\x00\x00"
		"Atk4\x00\x00\x00\x00"
		"Atk5\x00\x00\x00\x00"
		"Atk6\x00\x00\x00\x00"
		"Atk7\x00\x00\x00\x00"
		"Atk8\x00\x00\x00\x00"
		"Atk9\x00\x00\x00\x00"
		"AtkP\x00\x00\x00\x00"
		"AtkK\x00\x00\x00\x00"
		"AtkS\x00\x00\x00\x00"
		"AtkH\x00\x00\x00\x00"
		"AtkD\x00\x00\x00\x00"
		"AtkC\x00\x00\x00\x00"
		"AtkO\x00\x00\x00\x00";
	
	atkIconNamesLoc = sigscan(
		"GuiltyGearXrd.exe:.rdata",
		atkSig,
		sizeof atkSig - 1,
		"atkIconNamesLoc", nullptr);
	
	uintptr_t atkIconNamesUsage = 0;
	if (atkIconNamesLoc) {
		std::vector<char> sig;
		std::vector<char> mask;
		std::vector<char> maskForCaching;
		// ghidra sig: c7 00 ?? ?? ?? ?? e8
		byteSpecificationToSigMask("c7 00 rel(?? ?? ?? ??) e8", sig, mask, nullptr, 0, &maskForCaching);
		substituteWildcard(sig, mask, 0, (void*)atkIconNamesLoc);
		atkIconNamesUsage = sigscanOffset(
			GUILTY_GEAR_XRD_EXE,
			sig,
			mask,
			nullptr, "AtkIconNamesUsage", maskForCaching.data());
	}
	if (atkIconNamesUsage) {
		uintptr_t instrPos = sigscanBackwards(atkIconNamesUsage,
			"55"
			" 8b ec"
			" 83 e4 f0"
			" 81 ec",
			0xc0);
		
		orig_drawTrainingHudInputHistory = (drawTrainingHudInputHistory_t)instrPos;
		
		gameModeFast = *(GameModeFast**)(instrPos + 0x1b);
		drawTrainingHudInputHistoryVal2 = *(int**)(instrPos + 0x2f);
		drawTrainingHudInputHistoryVal3 = *(int**)(instrPos + 0x3c);
		
	}
	if (orig_drawTrainingHudInputHistory) {
		auto drawTrainingHudInputHistoryHookPtr = &HookHelp::drawTrainingHudInputHistoryHook;
		if (!detouring.attach(&(PVOID&)orig_drawTrainingHudInputHistory,
			(PVOID&)drawTrainingHudInputHistoryHookPtr,
			"drawTrainingHudInputHistory")) return false;
	}
	
	uintptr_t hitDetectionHitOwnEffects = sigscanOffset(
		GUILTY_GEAR_XRD_EXE,
		"83 bf 3c 28 00 00 2c",
		{ -4 },
		nullptr, "hitDetectionHitOwnEffects");
	if (hitDetectionHitOwnEffects) {
		uintptr_t hitDetectionUsage = sigscanForward(hitDetectionHitOwnEffects, "6a 00 6a 00 6a 00 6a 01 56 8b cf e8 ?? ?? ?? ??", 0xa2);
		if (hitDetectionUsage) {
			hitDetectionFunc = (hitDetection_t)followRelativeCall(hitDetectionUsage + 11);
		}
	}
	
	uintptr_t stunIncrementPlace = sigscanOffset(
		GUILTY_GEAR_XRD_EXE,
		"01 86 c4 9f 00 00 8b 8e c8 9f 00 00",
		nullptr,
		"StunIncrementPlace");
	if (stunIncrementPlace) {
		void* increaseStunHookAsmPtr = (void*)increaseStunHookAsm;
		std::vector<char> newBytes;
		newBytes.resize(6);
		newBytes[0] = '\xe8';  // relative call
		int offset = calculateRelativeCallOffset(stunIncrementPlace, (uintptr_t)increaseStunHookAsm);
		memcpy(newBytes.data() + 1, &offset, 4);
		newBytes[5] = '\x90';  // NOP
		detouring.patchPlace(stunIncrementPlace, newBytes);
	}
	
	uintptr_t jumpInstallPlace = sigscanOffset(
		GUILTY_GEAR_XRD_EXE,
		// it could also be mov eax,dword [esp+0x6c] which 8b ?? 6c does not cover (it is 8B 44 24 6C) but I won't bother with that
		"8b ?? 6c 83 f8 08 74 6a 83 f8 09 74 65 83 f8 0a 74 60 83 f8 05 74 0a 83 f8 06 74 05 83 f8 07 75 74"
			" f7 86 24 4d 00 00 00 04 00 00 75 68",
		nullptr,
		"JumpInstallPlace");
	if (jumpInstallPlace) {
		uintptr_t jumpInstallPlaceOrig = jumpInstallPlace;
		jumpInstallPlace += 0x2d;
		Register dstReg;
		if (isMovInstructionFromRegToReg((BYTE*)jumpInstallPlace, &dstReg, nullptr) && dstReg == REGISTER_ECX
				&& *(BYTE*)(jumpInstallPlace + 2) == 0xe8) {
			jumpInstallPlace += 2;
			restoreDoubleJumps = followRelativeCall(jumpInstallPlace);
			
			std::vector<char> newBytes;
			newBytes.resize(5);
			newBytes[0] = '\xe8';  // relative call
			int offset = calculateRelativeCallOffset(jumpInstallPlace, (uintptr_t)jumpInstallNormalJumpHookAsm);
			memcpy(newBytes.data() + 1, &offset, 4);
			detouring.patchPlace(jumpInstallPlace, newBytes);
			
		}
		
		jumpInstallPlace = followSinglyByteJump(jumpInstallPlaceOrig + 6);
		if (isMovInstructionFromRegToReg((BYTE*)jumpInstallPlace, &dstReg, nullptr) && dstReg == REGISTER_ECX
				&& *(BYTE*)(jumpInstallPlace + 2) == 0xe8) {
			jumpInstallPlace += 2;
			restoreAirDash = followRelativeCall(jumpInstallPlace);
			
			std::vector<char> newBytes;
			newBytes.resize(5);
			newBytes[0] = '\xe8';  // relative call
			int offset = calculateRelativeCallOffset(jumpInstallPlace, (uintptr_t)jumpInstallSuperJumpHookAsm);
			memcpy(newBytes.data() + 1, &offset, 4);
			detouring.patchPlace(jumpInstallPlace, newBytes);
			
		}
	}
	
	uintptr_t speedYResetPlace = sigscanOffset(
		GUILTY_GEAR_XRD_EXE,
		"e8 ?? ?? ?? ?? 83 a6 24 4d 00 00 fd",
		nullptr,
		"SpeedYResetPlace");
	if (speedYResetPlace) {
		std::vector<char> bytes(4);
		auto ptr = &HookHelp::speedYReset;
		int offset = calculateRelativeCallOffset(speedYResetPlace, (uintptr_t)(PVOID&)ptr);
		memcpy(bytes.data(), &offset, 4);
		detouring.patchPlace(speedYResetPlace + 1, bytes);
	}
	
	if (!onPlayerIsBossChanged()) return false;
	
	uintptr_t isSignVer1_10OrHigherPlace = sigscanOffset(
		GUILTY_GEAR_XRD_EXE,
		"83 78 70 03 73 03 33 c0 c3 b8 01 00 00 00 c3",
		nullptr,
		"isSignVer1_10OrHigher");
	if (isSignVer1_10OrHigherPlace) {
		isSignVer1_10OrHigherPlace -= 14;
		if (*(BYTE*)(isSignVer1_10OrHigherPlace) == 0xe8 && (isSignVer1_10OrHigherPlace & 0xF) == 0) {
			isSignVer1_10OrHigher = (isSignVer1_10OrHigher_t)isSignVer1_10OrHigherPlace;
		}
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
		
		if (ui.needTestDelayStage2) {
			testDelay();
		}
		
		bool isRunning = (
				game.isMatchRunning()
				|| players[0].gotHitOnThisFrame  // fix for death by DoT
				|| players[1].gotHitOnThisFrame  // fix for death by DoT
			)
			|| altModes.roundendCameraFlybyType() != 8
			|| game.is0xa8PreparingCamera();
		if (!isRunning && !iGiveUp) {
			if (!settings.dontClearFramebarOnStageReset) {
				playerFramebars.clear();
				projectileFramebars.clear();
				combinedFramebars.clear();
			}
			startedNewRound = true;
			
			for (int i = 0; i < 2; ++i) {
				PlayerInfo& player = players[i];
				player.inputs.clear();
				player.prevInput = Input{0x0000};
				player.inputsOverflow = false;
				player.ramlethalForpeliMarteliDisabled = false;
			}
		}
		needDrawInputs = false;
		entityList.populate();
		if (requestedInputHistoryDraw) needDrawInputs = true;
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
		attackHitboxes.clear();
		hitDetector.clearAllBoxes();
		throws.clearAllBoxes();
		leoParries.clear();
	}
	// Camera values are updated separately, in a updateCameraHook call, which happens before this is called
}

// Runs on the main thread
void EndScene::prepareDrawData(bool* needClearHitDetection) {
	logOnce(fputs("prepareDrawData called\n", logfile));
	invisChipp.onEndSceneStart();
	drawnEntities.clear();
	
	bool isTheFirstFrameInTheRound = false;
	if (playerFramebars.empty() && !iGiveUp) {
		isTheFirstFrameInTheRound = true;
		playerFramebars.emplace_back();
		{
			PlayerFramebars& framebar = playerFramebars.back();
			framebar.playerIndex = 0;
		}
		
		playerFramebars.emplace_back();
		{
			PlayerFramebars& framebar = playerFramebars.back();
			framebar.playerIndex = 1;
		}
		
		framebarPosition = _countof(Framebar::frames) - 1;
		framebarTotalFramesUnlimited = 0;
		framebarPositionHitstop = _countof(Framebar::frames) - 1;
		framebarTotalFramesHitstopUnlimited = 0;
		framebarIdleFor = 0;
		framebarIdleHitstopFor = 0;
		ui.onFramebarReset();
	}
	if (startedNewRound) {
		startedNewRound = false;
		if (!iGiveUp) {
			isTheFirstFrameInTheRound = true;
		}
	}
	if (isTheFirstFrameInTheRound) {
		bool isTraining = game.isTrainingMode();
		if (isTraining && settings.startingTensionPulse) {
			int val = settings.startingTensionPulse;
			if (val > 25000) val = 25000;
			if (val < -25000) val = -25000;
			for (int i = 0; i < 2; ++i) {
				entityList.slots[i].tensionPulse() = settings.startingTensionPulse;
			}
		}
		if (isTraining && settings.clearInputHistoryOnStageResetInTrainingMode
				|| settings.clearInputHistoryOnStageReset) {
			game.clearInputHistory();
			clearInputHistory();
		}
		for (int i = 0; i < 2; ++i) {
			// on the first frame of a round people can't act. At all
			// without this fix, the mod thinks normals are enabled except on the very first of a match where it thinks they're not
			PlayerInfo& player = players[i];
			player.wasEnableNormals = false;
			player.wasPrevFrameEnableNormals = false;
			player.wasPrevFrameEnableWhiffCancels = false;
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
			player.wasCancels.deleteThatWhichWasNotFound();
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
			player.burstGainCounter = ent.burstGainCounter();
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
			player.speedY = player.lostSpeedYOnThisFrame ? player.speedYBeforeSpeedLost : ent.speedY();
			player.gravity = ent.gravity();
			player.comboTimer = ent.comboTimer();
			player.pushback = ent.pushback();
			
			player.prevFrameHadDangerousNonDisabledProjectiles = player.hasDangerousNonDisabledProjectiles;
			player.hasDangerousNonDisabledProjectiles = false;
			player.createdProjectiles.clear();
			
			player.inHitstunNowOrNextFrame = ent.inHitstun();
			player.inHitstun = ent.inHitstunThisFrame();
			bool needResetStun = false;
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
				if (player.inHitstunNowOrNextFrame) {
					needResetStun = true;
				}
			}
			// I'm checking both players because comboStarted means either player in being combo'd right now,
			// and I just want to check if a combo is going on right now
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
			
			if (player.inHitstunNowOrNextFrame) {
				int newStun;
				if (reachedMaxStun[i] != -1) {
					newStun = reachedMaxStun[i];
				} else {
					newStun = ent.stun();
				}
				if (newStun != -1 && (needResetStun || newStun > player.stunCombo)) {
					player.stunCombo = newStun;
				}
			}
			
			player.stun = ent.stun();
			player.stunThreshold = ent.stunThreshold();
			int prevFrameBlockstun = player.blockstun;
			player.blockstun = ent.blockstun();
			int prevFrameHitstun = player.hitstun;
			player.hitstun = ent.hitstun();
			player.tumble = 0;
			player.displayTumble = false;
			player.wallstick = 0;
			player.displayWallstick = false;
			player.knockdown = 0;
			player.displayKnockdown = false;
			CmnActIndex cmnActIndex = ent.cmnActIndex();
			if (cmnActIndex == CmnActBDownLoop
					|| cmnActIndex == CmnActFDownLoop
					|| cmnActIndex == CmnActVDownLoop
					|| cmnActIndex == CmnActBDown2Stand
					|| cmnActIndex == CmnActFDown2Stand
					|| cmnActIndex == CmnActWallHaritsukiLand
					|| cmnActIndex == CmnActWallHaritsukiGetUp
					|| cmnActIndex == CmnActKizetsu
					|| cmnActIndex == CmnActHajikareStand
					|| cmnActIndex == CmnActHajikareCrouch
					|| cmnActIndex == CmnActHajikareAir) {
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
			} else if (cmnActIndex == CmnActWallHaritsuki) {
				if (ent.bbscrvar2() == 0) {
					if (ent.currentAnimDuration() == 1 && !ent.isRCFrozen()) {
						player.wallstickContaminatedByRCSlowdown = false;
						player.wallstickMax = ent.wallstick() + 30 - 1;
						player.wallstickElapsed = 0;
					}
					player.displayWallstick = true;
					player.wallstick = ent.wallstick() + 1 - ent.bbscrvar();
				} else {
					player.wallstick = 0;
				}
			} else if (cmnActIndex == CmnActFDownLoop
					|| cmnActIndex == CmnActBDownLoop
					|| cmnActIndex == CmnActVDownLoop) {
				int knockdownDur;
				if (ent.hp() * 10000 / 420 > 0) {
					knockdownDur = 11;
					if (ent.infiniteKd()) {
						knockdownDur = -1;
					}
					int customKd = ent.knockdown();
					if (customKd != 0) {
						knockdownDur = customKd;
					}
					int frame = ent.currentAnimDuration();
					if (knockdownDur != -1) {
						if (ent.currentAnimDuration() == 1 && !ent.isRCFrozen()) {
							player.knockdownContaminatedByRCSlowdown = false;
							player.knockdownMax = knockdownDur;
							player.knockdownElapsed = 0;
						}
						player.displayKnockdown = true;
						player.knockdown = knockdownDur + 1 - frame;
					} else {
						player.knockdown = 0;
					}
				} else {
					player.knockdown = 0;
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
			
			bool jumpInstalledStage2 = player.jumpInstalledStage2;
			bool superJumpInstalledStage2 = player.superJumpInstalledStage2;
			player.jumpInstalledStage2 = false;
			player.superJumpInstalledStage2 = false;
			
			bool startedRunOrWalkOnThisFrame = false;
			if (player.changedAnimOnThisFrame) {
				
				bool oldIsRunning = player.isRunning;
				bool oldIsWalkingForward = player.isWalkingForward;
				bool oldIsWalkingBackward = player.isWalkingBackward;
				
				player.isRunning = false;
				player.isWalkingForward = false;
				player.isWalkingBackward = false;
				if (player.startedRunning) {
					player.isRunning = true;
				} else if (player.startedWalkingForward) {
					player.isWalkingForward = true;
				} else if (player.startedWalkingBackward) {
					player.isWalkingBackward = true;
				}
					
				if ((
						player.startedRunning && !oldIsRunning
						|| player.startedWalkingForward && !oldIsWalkingForward  // for combining Faust 3 and 6 walks
						|| player.startedWalkingBackward && !oldIsWalkingBackward
					) && otherEnt.inHitstun()
				) {
					startedRunOrWalkOnThisFrame = true;
					ui.comboRecipeUpdatedOnThisFrame[player.index] = true;
					player.comboRecipe.emplace_back();
					ComboRecipeElement& newComboElem = player.comboRecipe.back();
					newComboElem.name = &emptyNamePair;
					newComboElem.timestamp = aswEngineTickCount;
					newComboElem.framebarId = -1;
					newComboElem.dashDuration = 1;
					if (player.startedRunning) {
						// do nothing
					} else if (player.startedWalkingForward) {
						newComboElem.isWalkForward = true;
					} else if (player.startedWalkingBackward) {
						newComboElem.isWalkBackward = true;
					}
					
					if (!player.pawn.currentAnimData()->isPerformedRaw()) {
						const GatlingOrWhiffCancelInfo* foundCancel = nullptr;
						if (player.prevFrameCancels.hasCancel(player.pawn.currentMove()->name, &foundCancel)
								&& foundCancel->framesBeenAvailableForNotIncludingHitstopFreeze > 0
								&& !(
									foundCancel->framesBeenAvailableForNotIncludingHitstopFreeze == 1
									&& foundCancel->wasAddedDuringHitstopFreeze
								)
						) {
							newComboElem.cancelDelayedBy = foundCancel->framesBeenAvailableForNotIncludingHitstopFreeze;
						}
					} else {
						newComboElem.doneAfterIdle = true;
						newComboElem.cancelDelayedBy = player.timePassedPureIdle;
					}
				}
			}
			
			if ((player.isRunning || player.isWalkingForward || player.isWalkingBackward)
					&& !player.hitstop && !superflashInstigator && !startedRunOrWalkOnThisFrame && otherEnt.inHitstun()) {
				ComboRecipeElement* lastElem = player.findLastDash();
				if (lastElem) {
					bool ok = false;
					if (player.isRunning) {
						ok = !lastElem->isWalkForward && !lastElem->isWalkBackward;
					} else if (player.isWalkingForward) {
						ok = lastElem->isWalkForward;
					} else if (player.isWalkingBackward) {
						ok = lastElem->isWalkBackward;
					}
					if (ok) {
						++lastElem->dashDuration;
					}
				}
			}
			
			player.wallslumpLand = 0;
			player.wallslumpLandWithSlow = 0;
			player.rejection = 0;
			player.rejectionWithSlow = 0;
			
			const CmnActIndex entCmnActIndex = ent.cmnActIndex();
			if (entCmnActIndex == CmnActJitabataLoop) {
				int bbscrvar = ent.bbscrvar();
				if (player.changedAnimOnThisFrame) {
					player.staggerElapsed = 0;
					player.staggerMaxFixed = false;
					player.prevBbscrvar = 0;
					player.prevBbscrvar5 = ent.receivedAttack()->staggerDuration * 10;
				}
				if (!player.hitstop && !superflashInstigator) ++player.staggerElapsed;
				int staggerMax = player.prevBbscrvar5 / 10 + ent.thisIsMinusOneIfEnteredHitstunWithoutHitstop();
				int animDur = ent.currentAnimDuration();
				if (!bbscrvar) {
					player.staggerMax = staggerMax;
				} else if (!player.prevBbscrvar) {
					player.staggerMaxFixed = true;
					if (staggerMax - 4 < animDur) {
						staggerMax = animDur + 4;
					}
					player.staggerMax = staggerMax;
				}
				player.stagger = player.staggerMax - (animDur - 1) - (player.hitstop ? 1 : 0);
				player.prevBbscrvar = bbscrvar;
			} else if (entCmnActIndex == CmnActHajikareStand
					|| entCmnActIndex == CmnActHajikareCrouch
					|| entCmnActIndex == CmnActHajikareAir) {
				int animDur = ent.currentAnimDuration();
				if (animDur == 1 && !ent.isSuperFrozen() && !ent.isRCFrozen()) {
					player.rejectionElapsed = 0;
				}
				if (!superflashInstigator && !player.hitstop) {
					++player.rejectionElapsed;
				}
				int hitEffect = ent.currentHitEffect();
				if (hitEffect >= 22 && hitEffect <= 25) {
					player.rejectionMax = 30;
					player.rejection = 31 - animDur;
				} else {
					player.rejectionMax = 60;
					player.rejection = 61 - animDur;
				}
			} else if (entCmnActIndex == CmnActWallHaritsukiLand) {
				if (player.changedAnimOnThisFrame) {
					player.wallslumpLandElapsed = 0;
				}
				if (!player.hitstop && !superflashInstigator) ++player.wallslumpLandElapsed;
				int animDur = ent.currentAnimDuration();
				player.wallslumpLand = 26 - animDur + 1;
				player.wallslumpLandMax = 26;
				player.displayHitstop = false;
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
				&& ent.exKizetsu() <= 0
				|| player.move.canFaultlessDefend && player.move.canFaultlessDefend(player);
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
			player.crouching = false;
			
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
					&& strcmp(animName, "Semuke") == 0
					&& (
						strcmp(ent.gotoLabelRequests(), "SemukeEnd") == 0
						|| !ent.enableWhiffCancels()
					)) {
				if (strcmp(ent.gotoLabelRequests(), "SemukeEnd") == 0) {
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
			
			player.dustGatlingTimer = player.pawn.dustGatlingTimer();
			if (!player.dustGatlingTimer) player.dustGatlingTimer = player.pawn.dustPropulsion();
			if (!player.dustGatlingTimer) player.dustGatlingTimerMax = 0;
			if (player.dustGatlingTimer > player.dustGatlingTimerMax) player.dustGatlingTimerMax = player.dustGatlingTimer;
			
			if (idleNext != player.idle || player.wasIdle && !idleNext || (player.setBlockstunMax || player.setHitstunMax) && !idleNext) {
				player.idle = idleNext;
				if (!player.idle) {
					if (player.onTheDefensive) {
						player.startedDefending = true;
						if (player.cmnActIndex != CmnActUkemi && !player.baikenReturningToBlockstunAfterAzami) {
							player.hitstopMax = player.hitstop;
							player.hitstopElapsed = 0;
							if (player.setBlockstunMax && player.hitstop) {  // may enter blockstun without hitstop due to Mist Finer Bacchus Sigh-buffed airhit when it glitches out and does not become unblockable
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
			if (player.wallstick && !player.hitstop && !superflashInstigator) {
				++player.wallstickElapsed;
				if (player.rcSlowedDown) player.wallstickContaminatedByRCSlowdown = true;
			}
			if (player.knockdown && !player.hitstop && !superflashInstigator) {
				++player.knockdownElapsed;
				if (player.rcSlowedDown) player.knockdownContaminatedByRCSlowdown = true;
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
			PlayerInfo::calculateSlow(player.wallstickElapsed,
				player.wallstick,
				newSlow,
				&player.wallstickWithSlow,
				&player.wallstickMaxWithSlow,
				&unused);
			PlayerInfo::calculateSlow(player.knockdownElapsed,
				player.knockdown,
				newSlow,
				&player.knockdownWithSlow,
				&player.knockdownMaxWithSlow,
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
			if (player.cmnActIndex == CmnActHajikareStand
					|| player.cmnActIndex == CmnActHajikareCrouch
					|| player.cmnActIndex == CmnActHajikareAir) {
				PlayerInfo::calculateSlow(player.rejectionElapsed,
					player.rejection,
					newSlow,
					&player.rejectionWithSlow,
					&player.rejectionMaxWithSlow,
					&unused);
			}
			if (player.cmnActIndex == CmnActWallHaritsukiLand) {
				PlayerInfo::calculateSlow(player.wallslumpLandElapsed,
					player.wallslumpLand,
					newSlow,
					&player.wallslumpLandWithSlow,
					&player.wallslumpLandMaxWithSlow,
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
						&& player.pawn.justReachedSprite()) {
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
			if (player.changedAnimOnThisFrame
					&& !(player.cmnActIndex == CmnActJump && !player.canFaultlessDefense)) {
				player.sinHunger = false;
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
						&& strcmp(player.anim, "AntiAirAttack") != 0
						&& strcmp(animName, "AntiAir4Hasei") == 0
						|| player.charType == CHARACTER_TYPE_JOHNNY
						&& ent.mem54()  // when Mem(54) is set, MistFinerLoop/AirMistFinerLoop instantly transitions to a chosen Mist Finer. However, in Rev1, it takes one frame
						&& (
							strcmp(animName, "MistFinerLoop") == 0
							|| strcmp(animName, "AirMistFinerLoop") == 0
						)
					)
					|| player.charType == CHARACTER_TYPE_JAM
					&& strcmp(player.anim, "NeoHochihu") == 0  // 54625 causes NeoHochihu to cancel into itself
					&& strcmp(animName, "NeoHochihu") == 0
					&& !player.wasEnableWhiffCancels
					|| player.charType == CHARACTER_TYPE_JOHNNY
					&& ent.mem54()
					&& (
						strcmp(player.anim, "MistFinerLoop") == 0
						&& (
							strncmp(animName, "MistFinerALv", 12) == 0
							|| strncmp(animName, "MistFinerBLv", 12) == 0
							|| strncmp(animName, "MistFinerCLv", 12) == 0
						)
						|| strcmp(player.anim, "AirMistFinerLoop") == 0
						&& (
							strncmp(animName, "AirMistFinerALv", 15) == 0
							|| strncmp(animName, "AirMistFinerBLv", 15) == 0
							|| strncmp(animName, "AirMistFinerCLv", 15) == 0
						)
					)
				);
				if (!player.changedAnimFiltered) {
					player.determineMoveNameAndSlangName(&player.lastPerformedMoveName);
				}
				if (*player.prevAttackLockAction != '\0' && strcmp(animName, player.prevAttackLockAction) == 0
						&& !player.move.dontSkipGrab) {
					player.grab = true;  // this doesn't work on regular ground and air throws
					memcpy(player.grabAnimation, player.prevAttackLockAction, 32);
				}
				if (strcmp(animName, player.grabAnimation) != 0) {
					player.grab = false;
				}
				if (player.idle && (
						player.canBlock && player.canFaultlessDefense
						|| player.wasIdle  // needed for linking a normal with a forward dash
					)) {
					player.ignoreNextInabilityToBlockOrAttack = true;
					player.performingASuper = false;
					other.gettingHitBySuper = false;
					player.sinHunger = false;
				}
				if (
						!player.changedAnimFiltered
						&& !(
							player.charType == CHARACTER_TYPE_BEDMAN
							&& strcmp(player.anim, "Boomerang_B") != 0
							&& strcmp(player.anim, "Boomerang_B_Air") != 0
							&& strcmp(animName, "BWarp") == 0
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
					if (!player.isInFDWithoutBlockstun && player.lastPerformedMoveNameIsInComboRecipe && !player.comboRecipe.empty()) {
						ComboRecipeElement* lastElem = player.findLastNonProjectileComboElement();
						if (lastElem) {
							lastElem->name = player.lastPerformedMoveName;
						}
					}
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
						player.moveOriginatedInTheAir = player.airborne && player.wasAirborneOnAnimChange;
						
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
									strcmp(player.anim, "BlockingStand") == 0
									|| strcmp(player.anim, "BlockingCrouch") == 0
									|| strcmp(player.anim, "BlockingAir") == 0
								)
								&& (
									strcmp(animName, "BlockingStand") == 0
									|| strcmp(animName, "BlockingCrouch") == 0
									|| strcmp(animName, "BlockingAir") == 0
								)
								|| player.charType == CHARACTER_TYPE_ELPHELT
								&& !player.idle
								&& prevFrameCmnActIndex == CmnActRomanCancel
								&& strcmp(animName, "Rifle_Roman") == 0
								|| player.charType == CHARACTER_TYPE_JOHNNY
								&& !player.wasIdle
								&& (
									strcmp(animName, "MistFinerA") == 0
									|| strcmp(animName, "MistFinerB") == 0
									|| strcmp(animName, "MistFinerC") == 0
									|| strcmp(animName, "AirMistFinerA") == 0
									|| strcmp(animName, "AirMistFinerB") == 0
									|| strcmp(animName, "AirMistFinerC") == 0
								)
								|| player.charType == CHARACTER_TYPE_POTEMKIN
								&& !player.wasIdle
								&& (
									strcmp(animName, "HammerFall") == 0
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
									player.lastPerformedMoveName);
								player.total -= player.superfreezeStartup;  // needed for Elphelt Rifle RC
								player.superfreezeStartup = 0;
							}
							player.prevStartups.add(player.total,
								player.lastMoveIsPartOfStance,
								player.lastPerformedMoveName);
							if (player.charType == CHARACTER_TYPE_JOHNNY
									&& (
										strncmp(animName, "MistFinerALv", 12) == 0
										|| strncmp(animName, "MistFinerBLv", 12) == 0
										|| strncmp(animName, "MistFinerCLv", 12) == 0
										|| strncmp(animName, "AirMistFinerALv", 15) == 0
										|| strncmp(animName, "AirMistFinerBLv", 15) == 0
										|| strncmp(animName, "AirMistFinerCLv", 15) == 0
										|| strcmp(animName, "StepTreasureHunt") == 0
									)
							) {
								player.removeNonStancePrevStartups();
							}
						} else {
							player.onAnimReset();
						}
						
						memcpy(player.labelAtTheStartOfTheMove, player.pawn.gotoLabelRequests(), 32);
						player.airteched = player.cmnActIndex == CmnActUkemi;
						player.moveStartTime_aswEngineTick = player.startup;
						player.startup = 0;
						player.startedUp = false;
						player.actives.clear();
						player.maxHit.clear();
						player.recovery = 0;
						player.total = 0;
						player.totalForInvul = 0;
						player.stoppedMeasuringInvuls = false;
						player.timePassedInNonFrozenFramesSinceStartOfAnim = 0;
						player.lastMoveIsPartOfStance = player.move.partOfStance;
						player.determineMoveNameAndSlangName(&player.lastPerformedMoveName);
						player.lastPerformedMoveNameIsInComboRecipe = false;
						
						player.delayLastMoveWasCancelledIntoWith = 0;
						if (!player.pawn.currentAnimData()->isPerformedRaw()) {
							player.delayInTheLastMoveIsAfterIdle = false;
							if (!(
									player.charType == CHARACTER_TYPE_LEO
									&& strcmp(player.pawn.previousAnimName(), "CmnActFDash") == 0
								)) {
								const GatlingOrWhiffCancelInfo* foundCancel = nullptr;
								if (player.prevFrameCancels.hasCancel(player.pawn.currentMove()->name, &foundCancel)) {
									if (foundCancel->framesBeenAvailableForNotIncludingHitstopFreeze > 0
											&& !(
												foundCancel->framesBeenAvailableForNotIncludingHitstopFreeze == 1
												&& foundCancel->wasAddedDuringHitstopFreeze
											)) {
										player.delayLastMoveWasCancelledIntoWith = foundCancel->framesBeenAvailableForNotIncludingHitstopFreeze;
										player.delayInTheLastMoveIsAfterIdle = false;
									}
								} else if (
										(player.timeSinceWasEnableSpecialCancel || player.timeSinceWasEnableSpecials)
										&& player.pawn.dealtAttack()->type >= ATTACK_TYPE_EX
										&& player.timeSinceWasEnableSpecialCancel
								) {
									player.delayLastMoveWasCancelledIntoWith = player.timeSinceWasEnableSpecialCancel;
									player.delayInTheLastMoveIsAfterIdle = false;
								}
							}
						} else {
							player.delayLastMoveWasCancelledIntoWith = player.timePassedPureIdle;
							player.delayInTheLastMoveIsAfterIdle = true;
						}
						
						player.lastMoveWasJumpInstalled = jumpInstalledStage2;
						player.lastMoveWasSuperJumpInstalled = superJumpInstalledStage2;
						
						player.timeSinceWasEnableSpecialCancel = 0;
						player.timeSinceWasEnableSpecials = 0;
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
						player.sinHungerRecovery = 0;
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
						
						player.throwRangeValid = false;
						player.throwXValid = false;
						player.throwYValid = false;
						
						if (player.cmnActIndex != CmnActRomanCancel) {
							needDisableProjectiles = true;
						}
					} else if (conditionPartOne && !conditionPartTwo) {
						// Jam Hououshou final grounded leg kick RRC its startup - without this fix the frame after RRC superfreeze will be skipped
						// and the frame tooltip after will say "skipped: 1 super, 18 superfreeze, 1 super" which is very weird
						player.performingASuper = false;
						other.gettingHitBySuper = false;
						
						// and this is for combo recipe window to display RCs
						if (other.pawn.inHitstun()) {
							ui.comboRecipeUpdatedOnThisFrame[player.index] = true;
							player.comboRecipe.emplace_back();
							ComboRecipeElement& newComboElem = player.comboRecipe.back();
							player.determineMoveNameAndSlangName(&newComboElem.name);
							newComboElem.timestamp = aswEngineTickCount;
							newComboElem.framebarId = -1;
							player.lastPerformedMoveNameIsInComboRecipe = true;
						}
						
					}
				}
			}
			memcpy(player.anim, animName, 32);
			player.setMoveName(player.moveName, ent);
			
			if (other.pawn.inHitstun() && !startedRunOrWalkOnThisFrame
					
					// isRunning check needed to not register I-No's dash twice - it's already being registered in the run check,
					// and her dash is considered a move because she's not idle during it
					&& !player.isRunning) {
				
				// needed for Jack-O 4D1/2/3/4/6/7/8/9 - the name updates as the move goes on, without creating a new section or entering a new animation
				// a more universal mechanism will be added only if more characters need this
				if (player.charType == CHARACTER_TYPE_JACKO
						&& !player.idle && !player.isInFDWithoutBlockstun
						&& !player.comboRecipe.empty()
						&& (
							strcmp(player.anim, "IronballGenocideEx") == 0
							|| strcmp(player.anim, "AirIronballGenocideEx") == 0
						)) {
					ComboRecipeElement& lastElem = player.comboRecipe.back();
					player.determineMoveNameAndSlangName(&lastElem.name);
				}
				
				if (player.jumpCancelled
						|| player.superJumpCancelled
						|| player.jumpNonCancel
						|| player.superJumpNonCancel
						|| player.doubleJumped) {
					ui.comboRecipeUpdatedOnThisFrame[player.index] = true;
					player.comboRecipe.emplace_back();
					ComboRecipeElement& newComboElem = player.comboRecipe.back();
					
					if (player.superJumpCancelled) {
						newComboElem.name = assignName("Super Jump Cancel");
					} else if (player.jumpCancelled) {
						newComboElem.name = assignName("Jump Cancel");
					} else if (player.jumpNonCancel) {
						newComboElem.name = assignName("Jump");
					} else if (player.superJumpNonCancel) {
						newComboElem.name = assignName("Super Jump");
					} else if (player.doubleJumped) {
						newComboElem.name = assignName("Double Jump");
					}
					newComboElem.timestamp = aswEngineTickCount;
					newComboElem.framebarId = -1;
					newComboElem.artificial = true;
					newComboElem.isJump = true;
					
					// Jack-O 5H is not seen as a jump cancel, but as a 'Jump'
					if (player.timeSinceWasEnableJumpCancel) {
						newComboElem.cancelDelayedBy = player.timeSinceWasEnableJumpCancel;
					} else if (player.jumpNonCancel || player.superJumpNonCancel || player.doubleJumped) {
						newComboElem.doneAfterIdle = true;
						newComboElem.cancelDelayedBy = player.timePassedPureIdle;
					}
					
					player.jumpCancelled = false;
					player.superJumpCancelled = false;
					player.jumpNonCancel = false;
					player.superJumpNonCancel = false;
					player.doubleJumped = false;
					
				}
				
				// this should happen in the animation change registration above, but
				// we need an animFrame 3 check because things can be kara cancelled into FD, Blitz, specials, Burst, IK, supers (by completing motions)
				// so there are not many places we can go after changing animation, right?
				// what could happen inbetween that prevents us from reaching animFrame 3, besides kara cancelling and getting hit by something?
				// other than that animation starting outside of a combo, then ending, then combo starting and we hitting animFrame 3 on CmnActStand?
				// that we can filter with timePassedInNonFrozenFramesSinceStartOfAnim
				if ((
						player.animFrame == 3
						&& player.timePassedInNonFrozenFramesSinceStartOfAnim == 2
						// specials, supers and IKs cannot be kara cancelled at all
						|| player.animFrame == 1
						&& player.pawn.dealtAttack()->type > ATTACK_TYPE_NORMAL
						&& player.timePassedInNonFrozenFramesSinceStartOfAnim == 0
					)
					&& !player.lastPerformedMoveNameIsInComboRecipe
				) {
					
					bool isSJInstall = player.lastMoveWasSuperJumpInstalled
						&& !player.lastMoveWasJumpInstalled;
					
					bool outrightBanSJInstall = charDoesNotCareAboutSuperJumpInstalls[player.charType]
						&& isSJInstall;
					
					if (
							(player.lastMoveWasJumpInstalled || player.lastMoveWasSuperJumpInstalled)
							&& (
								!(
									player.move.ignoreJumpInstalls
									|| player.move.ignoreSuperJumpInstalls
									&& isSJInstall
								)
								// dust enables all normals to be cancelled, which means that jump installs on them could potentially matter,
								// even if normally advised otherwise. For example, Zato 6H is a dead end normal, but on a dust combo it can cancel
								// into 2D which jump cancels.
								|| player.pawn.dustGatlingTimer()
								&& player.pawn.dealtAttack()->type == ATTACK_TYPE_NORMAL
							)
							&& !outrightBanSJInstall
					) {
						// Here's why I don't think I should be worried about jump installs not getting registered in the
						// Combo Recipe panel due to the normal that was jump installed getting kara cancelled into a special,
						// if that normal was a 'dead end' normal that does not care about jump installs, or a normal that does not
						// care about superjumps, while the special does care about one of those two things:
						// 1) This can happen from neutral, or when the initial normal was done raw.
						//    You must be holding UP for this when the normal starts, there's simply no other way.
						//    Or else you have to jump install on an earlier move, that we'd register.
						//    When you're holding UP, there's no way in hell you're humanly capable of completing
						//    some motion.
						//    If you do manage to cancel the prejump into some special, the panel will show the 'Jump Cancel' then.
						//    So we're not losing any information.
						// 2) This can happen from another move that you cancel. Most jump cancels extend all the way
						//    across all of the active frames, or the gatling window. Which means that you can
						//    UP+X jump install a late gatling and that is a valid jump install if the initial normal
						//    was jump cancellable. You will then face the same problem of completing any motion at all
						//    from the UP direction. Sure, if it's a charge move with 46 input, you could probably do it,
						//    like May c.S > UP+5H~Horizontal Dolphin. But I don't expect people to do this that often.
						//    The other way you could jump cancel here is during hitstop and then try to complete a motion,
						//    but then right after hitstop you'd be in prejump, and even if you cancel that prejump into a special,
						//    the 'Jump Cancel' would get registered in the Combo Recipe. So we're not losing any information.
						player.comboRecipe.emplace_back();
						ComboRecipeElement& newElem = player.comboRecipe.back();
						newElem.artificial = true;
						newElem.name = player.lastMoveWasJumpInstalled ? assignName("Jump Install") : assignName("Super Jump Install");
						newElem.timestamp = aswEngineTickCount;
						if (player.lastMoveWasSuperJumpInstalled) {
							newElem.isSuperJumpInstall = true;
						}
					}
					
					ui.comboRecipeUpdatedOnThisFrame[player.index] = true;
					player.comboRecipe.emplace_back();
					ComboRecipeElement& newComboElem = player.comboRecipe.back();
					newComboElem.name = player.lastPerformedMoveName;
					newComboElem.whiffed = !player.hitSomething;
					newComboElem.timestamp = aswEngineTickCount;
					newComboElem.framebarId = -1;
					newComboElem.cancelDelayedBy = player.delayLastMoveWasCancelledIntoWith;
					newComboElem.doneAfterIdle = player.delayInTheLastMoveIsAfterIdle;
					player.lastPerformedMoveNameIsInComboRecipe = true;
				}
				
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
						projectileTo.creatorNamePtr = event.u.signal.creatorName;
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
						projectileTo.creatorNamePtr = event.u.signal.creatorName;
					}
				}
			}
			
			if (!player.hitstop
					&& getSuperflashInstigator() == nullptr
					&& !player.startedUp
					&& !player.inNewMoveSection
					&& !(
						player.charType == CHARACTER_TYPE_ELPHELT
						&& strcmp(player.anim, "Rifle_Roman") == 0
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
							player.lastPerformedMoveName);
						player.total -= player.superfreezeStartup;  // needed for Johnny Treasure Hunt
						player.superfreezeStartup = 0;
					}
					if (player.total) {
						player.prevStartups.add(player.total,
							player.lastMoveIsPartOfStance,
							player.lastPerformedMoveName);
					}
					player.startup = 0;
					player.determineMoveNameAndSlangName(&player.lastPerformedMoveName);
					if (player.lastPerformedMoveNameIsInComboRecipe && !player.comboRecipe.empty()) {
						ComboRecipeElement* lastElem = player.findLastNonProjectileComboElement();
						if (lastElem) {
							lastElem->name = player.lastPerformedMoveName;
						}
					}
					player.total = 0;
					player.totalForInvul = 0;
					player.stoppedMeasuringInvuls = false;
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
			
			if (!ent.isSuperFrozen() && !ent.isRCFrozen() && !player.hitstop
					&& player.timePassedInNonFrozenFramesSinceStartOfAnim != 0xFFFFFFFF) {
				++player.timePassedInNonFrozenFramesSinceStartOfAnim;
			}
			
			if (player.wasEnableSpecialCancel
					&& player.wasEnableGatlings
					&& player.wasAttackCollidedSoCanCancelNow
					// hitting stuff resets the delay of the cancel
					&& !player.pawn.hitSomethingOnThisFrame()) {
				if (!player.hitstop && !superflashInstigator) {
					++player.timeSinceWasEnableSpecialCancel;
				}
			} else {
				player.timeSinceWasEnableSpecialCancel = 0;
			}
			
			if (player.wasEnableSpecials) {
				if (!player.hitstop && !superflashInstigator) {
					++player.timeSinceWasEnableSpecials;
				}
			} else {
				player.timeSinceWasEnableSpecials = 0;
			}
			
			if (
				(
					player.wasEnableJumpCancel
					|| player.wasCancels.hasCancel("CmnVJump")
					|| player.wasCancels.hasCancel("CmnVAirJump")
				)
				&& !player.pawn.hitSomethingOnThisFrame()
			) {
				if (!player.hitstop && !superflashInstigator) {
					++player.timeSinceWasEnableJumpCancel;
				}
			} else {
				player.timeSinceWasEnableJumpCancel = 0;
			}
			
			handleMarteliForpeliSetting(player);
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
		
		checkVenomBallActivations();
		checkSelfProjectileHarmInflictions();
		
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
			bool hadHitboxes = projectile.landedHit || objHasAttackHitboxes(projectile.ptr);
			if (!found) {
				if (hadHitboxes || projectile.gotHitOnThisFrame) {
					projectile.ptr = nullptr;
					projectile.markActive = hadHitboxes;
					if (projectile.moveNonEmpty && (projectile.team == 0 || projectile.team == 1)) {
						PlayerInfo& player = players[projectile.team];
						player.registerCreatedProjectile(projectile);
					}
					++it;
					continue;
				}
				it = projectiles.erase(it);
			} else {
				projectile.fill(projectile.ptr, superflashInstigator, false);
				projectile.markActive = hadHitboxes;
				if (projectile.team == 0 || projectile.team == 1) {
					if (!projectile.moveNonEmpty) {
						projectile.isDangerous = false;
					} else {
						projectile.isDangerous = projectile.move.isDangerous && projectile.move.isDangerous(projectile.ptr);
						PlayerInfo& player = players[projectile.team];
						if (!projectile.disabled && projectile.isDangerous) {
							player.hasDangerousNonDisabledProjectiles = true;
						}
						player.registerCreatedProjectile(projectile);
					}
				}
				if (!projectile.inNewSection
						&& projectile.move.sectionSeparatorProjectile
						&& projectile.move.sectionSeparatorProjectile(projectile.ptr)) {
					projectile.inNewSection = true;
					if (projectile.total) {
						projectile.prevStartups.add(projectile.total, false, projectile.lastName);
					}
					projectile.determineMoveNameAndSlangName(&projectile.lastName);
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
					bool blockstunLinked = false;
					bool fallOnHitstun = false;
					bool recoilOnHitstun = false;
					bool isInvulnerable = false;
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
					"Undefined",  // ram_undefined
					"Teleporting",  // ram_teleport
					"Attack",  // ram_Attack
					"Hit Not Deployed",  // ram_koware_soubi
					"Hit Deployed",  // ram_koware_sonoba
					"Falling",  // ram_loop
					"Landing",  // ram_landing
					"Ramlethal Blocked",  // ram_koware_nokezori
					"Win"  // ram_Win
				};
				
				static const char* subAnims2[Moves::ram2_number_of_elements] {
					"Undefined",  // ram2_undefined
					"Teleporting",  // ram2_teleport
					"Attack",  // ram2_Attack
					"Win",  // ram2_Win
					"Hit",  // ram2_koware
					"Falling",  // ram2_loop
					"Landing",  // ram2_landing
					"Ramlethal Blocked"  // ram2_koware_nokezori
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
						bitInfo.blockstunLinked = p.hasUpon(BBSCREVENT_PLAYER_BLOCKED);
						bitInfo.fallOnHitstun = false;
						bitInfo.recoilOnHitstun = false;
						bitInfo.isInvulnerable = p.fullInvul() || p.strikeInvul() || p.hitboxCount(HITBOXTYPE_HURTBOX) == 0;
						anim = p.animationName();
						if (bitInfo.stackIndex == 0) {
							isTrance = strcmp(anim, "BitSpiral_NSpiral") == 0;
						} else {
							isTrance = strcmp(anim, "BitSpiral_FSpiral") == 0;
						}
						if (!isTrance) {
							if (bitInfo.stackIndex == 0) {
								isCalvados = strcmp(anim, "BitNLaser") == 0;
							} else {
								isCalvados = strcmp(anim, "BitFLaser") == 0;
							}
						}
						ProjectileInfo& projectile = endScene.findProjectile(p);
						if (projectile.ptr && projectile.move.displayName) {
							elapsed = projectile.ramlethalSwordElapsedTime;
							anim = projectile.move.displayName ? projectile.move.displayName->name : nullptr;
						}
						#define collectPlayerGotHitInfo(koware) \
							if (p.hasUpon(BBSCREVENT_PLAYER_GOT_HIT)) { \
								BYTE* instr = moves.skipInstr(p.uponStruct(BBSCREVENT_PLAYER_GOT_HIT)->uponInstrPtr); \
								if (moves.instrType(instr) == instr_gotoLabelRequests) { \
									const char* markerToGoTo = asInstr(instr, gotoLabelRequests)->name; \
									bitInfo.fallOnHitstun = strcmp(markerToGoTo, koware) == 0; \
									bitInfo.recoilOnHitstun = !bitInfo.fallOnHitstun; \
								} \
							}
						#define resetElapsedTimeIfOnFirstFrameOfState(frames) \
							if (offset == frames.front().offset \
									&& p.justReachedSprite()) { \
								projectile.ramlethalSwordElapsedTime = 0; \
								elapsed = 0; \
							}
						if (strcmp(p.animationName(), bitInfo.stateName.txt) == 0) {
							collectPlayerGotHitInfo("koware_sonoba")
							BYTE* func = p.bbscrCurrentFunc();
							if (!func) continue;
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
										if ((info.state == Moves::ram_koware_soubi || info.state == Moves::ram_koware_nokezori)) {
											resetElapsedTimeIfOnFirstFrameOfState(vec.frames)
										}
										timeLeft = vec.remainingTime(offset, p.spriteFrameCounter());
									} else if (info.state == Moves::ram_koware_sonoba || info.state == Moves::ram_loop) {
										if (info.state == Moves::ram_koware_sonoba) {
											resetElapsedTimeIfOnFirstFrameOfState(vec.frames)
										}
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
							collectPlayerGotHitInfo("koware")
							BYTE* func = p.bbscrCurrentFunc();
							if (!func) continue;
							moves.fillInRamlethalBitN6C_F6D(func, bitInfo.info2);
							int offset = p.bbscrCurrentInstr() - func;
							for (const Moves::RamlethalSwordInfo& info : bitInfo.info2) {
								const Moves::MayIrukasanRidingObjectInfo& vec = info.framesBunri;
								if (offset >= vec.frames.front().offset && offset <= vec.frames.back().offset) {
									subAnim = subAnims2[info.state];
									if (info.state == Moves::ram2_teleport) {
										timeLeft = vec.remainingTime(offset, p.spriteFrameCounter())
											+ bitInfo.info2[(int)Moves::ram2_Attack - 1].framesBunri.totalFrames;
									} else if (info.state == Moves::ram2_Attack
											|| info.state == Moves::ram2_landing
											|| info.state == Moves::ram2_koware_nokezori
											|| info.state == Moves::ram2_Win) {
										if (info.state == Moves::ram2_koware_nokezori) {
											resetElapsedTimeIfOnFirstFrameOfState(vec.frames)
										}
										timeLeft = vec.remainingTime(offset, p.spriteFrameCounter());
									} else if (info.state == Moves::ram2_koware || info.state == Moves::ram2_loop) {
										if (info.state == Moves::ram2_koware) {
											resetElapsedTimeIfOnFirstFrameOfState(vec.frames)
										}
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
						} else {
							bitInfo.recoilOnHitstun = p.hasUpon(BBSCREVENT_PLAYER_GOT_HIT);
						}
						#undef collectPlayerGotHitInfo
						#undef resetElapsedTimeIfOnFirstFrameOfState
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
				
				player.ramlethalSSwordBlockstunLinked = bitInfos[0].blockstunLinked;
				player.ramlethalSSwordFallOnHitstun = bitInfos[0].fallOnHitstun;
				player.ramlethalSSwordRecoilOnHitstun = bitInfos[0].recoilOnHitstun;
				player.ramlethalSSwordInvulnerable = bitInfos[0].isInvulnerable;
				
				player.ramlethalHSwordBlockstunLinked = bitInfos[1].blockstunLinked;
				player.ramlethalHSwordFallOnHitstun = bitInfos[1].fallOnHitstun;
				player.ramlethalHSwordRecoilOnHitstun = bitInfos[1].recoilOnHitstun;
				player.ramlethalHSwordInvulnerable = bitInfos[1].isInvulnerable;
				
			} else if (player.charType == CHARACTER_TYPE_ELPHELT) {
				
				bool grenadeFrozen = false;
				bool foundGrenadeInGeneral = false;
				bool foundGrenade = false;
				int grenadeSlowdown = 0;
				int grenadeTimeRemaining = 0;
				
				for (ProjectileInfo& projectile : projectiles) {
					if (projectile.team != player.index || !projectile.ptr) continue;
					if (strcmp(projectile.animName, "GrenadeBomb") == 0) {
						foundGrenadeInGeneral = true;
						if (!projectile.ptr.hasUpon(BBSCREVENT_PLAYER_GOT_HIT)) {
							foundGrenade = true;
							grenadeFrozen = projectile.ptr.isSuperFrozen();
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
							grenadeFrozen = projectile.ptr && projectile.ptr.isSuperFrozen();
						} else if (projectile.sprite.frameMax == 30) {
							grenadeTimeRemaining = 30 - projectile.sprite.frame;
							foundGrenade = true;
							grenadeFrozen = projectile.ptr && projectile.ptr.isSuperFrozen();
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
					
					if (!grenadeFrozen) ++player.elpheltGrenadeElapsed;
					
					if (result || hasForceDisableFlag) ++result;
					++resultMax;
					
					player.elpheltGrenadeRemainingWithSlow = result;
					player.elpheltGrenadeMaxWithSlow = resultMax;
				}
				
				player.elpheltRifle_AimMem46 = player.getElpheltRifle_AimMem46();
				
			} else if (player.charType == CHARACTER_TYPE_JOHNNY) {
				
				{
					bool johnnyMistFinerBuffed = player.pawn.dealtAttack()->enableGuardBreak()
						|| player.pawn.dealtAttack()->guardType == GUARD_TYPE_NONE;
					player.johnnyMistFinerBuffedOnThisFrame = !player.johnnyMistFinerBuffed && johnnyMistFinerBuffed;
					player.johnnyMistFinerBuffed = johnnyMistFinerBuffed;
					
					int playerval2 = player.wasPlayerval[2];
					int currentPlayerval2 = player.pawn.playerVal(2);
					
					int slowdown = 0;
					for (ProjectileInfo& projectile : projectiles) {
						if (projectile.team == player.index && strcmp(projectile.animName, "MistKuttsuku") == 0) {
							slowdown = projectile.rcSlowedDownCounter;
							if (projectile.lifeTimeCounter == 0) {
								player.johnnyMistKuttsukuElapsed = 0;
							} else if (!(projectile.ptr && projectile.ptr.isSuperFrozen())) {
								++player.johnnyMistKuttsukuElapsed;
							}
							break;
						}
					}
					
					int timeRemaining = currentPlayerval2;
					int unused;
					PlayerInfo::calculateSlow(
						player.johnnyMistKuttsukuElapsed + 1,
						timeRemaining,
						slowdown,
						&player.johnnyMistKuttsukuTimerWithSlow,
						&player.johnnyMistKuttsukuTimerMaxWithSlow,
						&unused);
					
					if (player.johnnyMistKuttsukuTimerMaxWithSlow <= 2) {
						player.johnnyMistKuttsukuTimerWithSlow = 0;
						player.johnnyMistKuttsukuTimerMaxWithSlow = 0;
					} else {
						++player.johnnyMistKuttsukuTimerMaxWithSlow;
						if (player.johnnyMistKuttsukuTimerWithSlow || !currentPlayerval2 && playerval2) {
							++player.johnnyMistKuttsukuTimerWithSlow;
						}
					}
				}
				
				{
					int maxTime = -1;
					int animFrame = -1;
					int slowdown = 0;
					bool isSuperFrozen = false;
					bool isFrozen = false;
					for (ProjectileInfo& projectile : projectiles) {
						if (projectile.team == player.index && projectile.ptr && strcmp(projectile.animName, "Mist") == 0) {
							animFrame = projectile.animFrame;
							isFrozen = projectile.ptr.isRCFrozen();
							isSuperFrozen = projectile.ptr.isSuperFrozen();
							slowdown = projectile.rcSlowedDownCounter;
							BYTE* func = projectile.ptr.bbscrCurrentFunc();
							if (!func) break;
							for (loopInstr(func)) {
								if (moves.instrType(instr) == instr_ifOperation
										&& asInstr(instr, ifOperation)->op == BBSCROP_IS_GREATER_OR_EQUAL
										&& asInstr(instr, ifOperation)->left == BBSCRVAR_FRAMES_PLAYED_IN_STATE
										&& asInstr(instr, ifOperation)->right == BBSCRTAG_VALUE) {
									maxTime = asInstr(instr, ifOperation)->right.value;
									break;
								}
							}
							break;
						}
					}
					
					if (maxTime == -1) {
						player.johnnyMistTimerWithSlow = 0;
					} else {
						
						if (animFrame == 1 && !isFrozen) {
							player.johnnyMistElapsed = 0;
						} else if (!isSuperFrozen) {
							++player.johnnyMistElapsed;
						}
						
						int timeRemaining = maxTime - animFrame;
						int unused;
						PlayerInfo::calculateSlow(
							player.johnnyMistElapsed + 1,
							timeRemaining,
							slowdown,
							&player.johnnyMistTimerWithSlow,
							&player.johnnyMistTimerMaxWithSlow,
							&unused);
						
					}
				}
				
			} else if (player.charType == CHARACTER_TYPE_JACKO) {
				
				int slowdown = 0;
				int timeRemaining = 0;
				Entity p = player.pawn.stackEntity(3);
				if (p && p.isActive()) {
					int frames = p.framesSinceRegisteringForTheIdlingSignal() - 1;
					if (moves.jackoAegisMax == 0) {
						BYTE* func = p.bbscrCurrentFunc();
						if (!func) continue;
						for (loopInstr(func)) {
							if (moves.instrType(instr) == instr_ifOperation
									&& asInstr(instr, ifOperation)->op == BBSCROP_IS_EQUAL
									&& asInstr(instr, ifOperation)->left == BBSCRVAR_FRAMES_SINCE_REGISTERING_FOR_THE_ANIMATION_FRAME_ADVANCED_SIGNAL
									&& asInstr(instr, ifOperation)->right == BBSCRTAG_VALUE) {
								moves.jackoAegisMax = asInstr(instr, ifOperation)->right.value;
								break;
							}
						}
					}
					if (p.lifeTimeCounter() == 0) player.jackoAegisElapsed = 0;
					else if (!p.isSuperFrozen()) {
						++player.jackoAegisElapsed;
					}
					ProjectileInfo& projectile = findProjectile(p);
					if (projectile.ptr) {
						slowdown = projectile.rcSlowedDownCounter;
					}
					timeRemaining = moves.jackoAegisMax - frames;
				}
				int unused;
				PlayerInfo::calculateSlow(
					player.jackoAegisElapsed + 1,
					timeRemaining,
					slowdown,
					&player.jackoAegisTimeWithSlow,
					&player.jackoAegisTimeMaxWithSlow,
					&unused);
				if (player.jackoAegisTimeMaxWithSlow <= 2) player.jackoAegisTimeMaxWithSlow = 0;
				
				player.jackoAegisActive = player.pawn.invulnForAegisField();
				player.jackoAegisReturningIn = INT_MIN;
				if (!player.pawn.inHitstunThisFrame() && !player.pawn.invulnForAegisField()) {
					for (int i = 2; i < entityList.count; ++i) {
						Entity ent = entityList.list[i];
						if (ent.isActive() && ent.team() == player.index && strcmp(ent.animationName(), "Aigisfield") == 0) {
							player.jackoAegisReturningIn = ent.mem45();
							break;
						}
					}
				}
				
				
			} else if (player.charType == CHARACTER_TYPE_HAEHYUN) {
				
				bool foundActualBall = false;
				bool foundProjectile = false;
				bool hasIndividualFlag = false;
				int projectileTimeLeft = 0;
				int slowdown = 0;
				bool createdOnThisFrame = false;
				int prevFoundAnimDur = -1;
				bool actualBallCreatedThisFrame = false;
				bool hitstop = false;
				Entity superBalls[10] { nullptr };
				int superBallsCount = 0;
				int unused;
				bool ballFrozen = false;
				bool actualBallFrozen = false;
				
				for (int j = 2; j < entityList.count; ++j) {
					ProjectileInfo* projectile = nullptr;
					bool triedFindProjectile = false;
					Entity p = entityList.list[j];
					if (p.isActive() && p.team() == player.index) {
						bool checkSlowdown = false;
						if (strcmp(p.animationName(), "yudodan_end") == 0) {
							if (prevFoundAnimDur == -1 || prevFoundAnimDur > (int)p.currentAnimDuration()) {
								prevFoundAnimDur = p.currentAnimDuration();
							} else {
								continue;
							}
							hasIndividualFlag = (p.forceDisableFlagsIndividual() & 0x1) != 0;
							if (!hasIndividualFlag) continue;
							foundProjectile = true;
							checkSlowdown = true;
							projectileTimeLeft = 7 + 14 - p.currentAnimDuration() + 1;
							createdOnThisFrame = p.currentAnimDuration() == 1 && !p.isRCFrozen();
							ballFrozen = p.isSuperFrozen();
						} else if (strcmp(p.animationName(), "EnergyBall") == 0) {
							actualBallCreatedThisFrame = p.currentAnimDuration() == 1 && !p.isRCFrozen();
							if (actualBallCreatedThisFrame) player.haehyunBallElapsed = 0;
							foundActualBall = true;
							checkSlowdown = true;
							projectileTimeLeft = 150 - p.framesSinceRegisteringForTheIdlingSignal() + 1;
							hitstop = p.hitstop() > 0 && !p.hitSomethingOnThisFrame();
							actualBallFrozen = p.isSuperFrozen();
						} else if (strcmp(p.animationName(), "SuperEnergyBall") == 0) {
							superBalls[superBallsCount++] = p;
							projectile = &findProjectile(p);
							triedFindProjectile = true;
							if (projectile && projectile->ptr) {
								projectile->haehyunCelestialTuningBall1 = superBallsCount == 1 || superBallsCount > 2;
								projectile->haehyunCelestialTuningBall2 = superBallsCount == 2 || superBallsCount > 2;
							}
						}
						if (checkSlowdown) {
							if (!triedFindProjectile) {
								projectile = &findProjectile(p);
							}
							if (projectile && projectile->ptr) slowdown = projectile->rcSlowedDownCounter;
						}
					}
				}
				
				if (superBallsCount) {
					qsort(superBalls, superBallsCount, sizeof Entity, LifeTimeCounterCompare);
					for (int j = 0; j < superBallsCount; ++j) {
						Entity superBall = superBalls[j];
						if (superBall.lifeTimeCounter() == 0) {
							player.haehyunSuperBallRemainingElapsed[j] = 0;
						} else if (!superBall.isSuperFrozen() && !(superBall.hitstop() && !superBall.hitSomethingOnThisFrame())) {
							++player.haehyunSuperBallRemainingElapsed[j];
						}
						
						int superBallSlowdown = 0;
						ProjectileInfo& projectile = findProjectile(superBall);
						if (projectile.ptr) {
							superBallSlowdown = projectile.rcSlowedDownCounter;
						}
						
						PlayerInfo::calculateSlow(
							player.haehyunSuperBallRemainingElapsed[j] + 1,
							240 - superBall.framesSinceRegisteringForTheIdlingSignal() + 1,
							superBallSlowdown,
							player.haehyunSuperBallRemainingTimeWithSlow + j,
							player.haehyunSuperBallRemainingTimeMaxWithSlow + j,
							&unused);
					}
				}
				
				if (superBallsCount != 10) {
					player.haehyunSuperBallRemainingTimeWithSlow[superBallsCount] = 0;
				}
				
				bool hasForceDisableFlag = (player.wasForceDisableFlags & 0x1) != 0;
				
				if (hasForceDisableFlag) {
					if (!ballFrozen && !foundActualBall && !createdOnThisFrame) {
					 	++player.haehyunBallElapsed;
					}
				} else {
					player.haehyunBallElapsed = 0;
				}
				
				if (!foundProjectile && !foundActualBall) {
					if (hasForceDisableFlag) {
						player.haehyunBallTimeWithSlow = 1;
					} else {
						player.haehyunBallTimeWithSlow = 0;
					}
					
					player.haehyunBallRemainingElapsed = 0;
					player.haehyunBallRemainingTimeWithSlow = 0;
				} else if (foundActualBall) {
					PlayerInfo::calculateSlow(
						1,
						7 + 14,
						slowdown,
						&unused,
						&player.haehyunBallTimeMaxWithSlow,
						&unused);
					player.haehyunBallTimeWithSlow = -1;
					++player.haehyunBallTimeMaxWithSlow;
					
					if (!actualBallCreatedThisFrame && !actualBallFrozen && !hitstop) {
						++player.haehyunBallRemainingElapsed;
					}
					
					PlayerInfo::calculateSlow(
						player.haehyunBallRemainingElapsed + 1,
						projectileTimeLeft,
						slowdown,
						&player.haehyunBallRemainingTimeWithSlow,
						&player.haehyunBallRemainingTimeMaxWithSlow,
						&unused);
				} else {
					
					if (foundActualBall) {
						projectileTimeLeft = 7 + 14;
					}
					
					int unused;
					PlayerInfo::calculateSlow(
						player.haehyunBallElapsed + 1,
						projectileTimeLeft,
						slowdown,
						&player.haehyunBallTimeWithSlow,
						&player.haehyunBallTimeMaxWithSlow,
						&unused);
					++player.haehyunBallTimeWithSlow;
					++player.haehyunBallTimeMaxWithSlow;
					
					player.haehyunBallRemainingElapsed = 0;
					player.haehyunBallRemainingTimeWithSlow = 0;
				}
				
			} else if (player.charType == CHARACTER_TYPE_RAVEN) {
				
				int timeRemaining = 0;
				int slowdown = 0;
				int needleTimeRemaining = 0;
				int needleSlowdown = 0;
				bool foundNeedleButNotNull = false;
				Entity needleObj = nullptr;
				for (int j = 2; j < entityList.count; ++j) {
					Entity p = entityList.list[j];
					if (p.isActive() && p.team() == player.index && !p.isPawn()) {
						bool checkNeedleSlowdown = false;
						if (strcmp(p.animationName(), "SlowEffect") == 0
								&& p.createArgHikitsukiVal1() == 0) {
							if (p.lifeTimeCounter() == 0) {
								player.slowTimeElapsed = 0;
							} else if (!p.isSuperFrozen()) {
								++player.slowTimeElapsed;
							}
							int timeMax;
							if (player.wasResource >= 6) {
								timeMax = 150;
							} else if (player.wasResource >= 3) {
								timeMax = 120;
							} else {
								timeMax = 90;
							}
							timeRemaining = timeMax - p.framesSinceRegisteringForTheIdlingSignal() + 1;
							
							ProjectileInfo& projectile = findProjectile(p);
							if (projectile.ptr) {
								slowdown = projectile.rcSlowedDownCounter;
							}
						} else if (strcmp(p.animationName(), "SlowNeeldeObjLand") == 0
								|| strcmp(p.animationName(), "SlowNeeldeObjAir") == 0) {
							if (strcmp(p.spriteName(), "null") == 0 && p.spriteFrameCounterMax() == 14) {
								needleTimeRemaining = p.spriteFrameCounterMax() - p.spriteFrameCounter();
								checkNeedleSlowdown = true;
							} else {
								foundNeedleButNotNull = true;
								needleTimeRemaining = 14;
							}
							needleObj = p;
						} else if (strcmp(p.animationName(), "LandSettingTypeNeedleObj") == 0
								|| strcmp(p.animationName(), "AirSettingTypeNeedleObj") == 0) {
							if (strcmp(p.spriteName(), "null") == 0 && p.spriteFrameCounterMax() == 15) {
								needleTimeRemaining = p.spriteFrameCounterMax() - p.spriteFrameCounter();
								checkNeedleSlowdown = true;
							} else {
								foundNeedleButNotNull = true;
								needleTimeRemaining = 15;
							}
							needleObj = p;
						}
						if (checkNeedleSlowdown) {
							ProjectileInfo& projectile = findProjectile(p);
							if (projectile.ptr) {
								needleSlowdown = projectile.rcSlowedDownCounter;
							}
						}
					}
				}
				
				int result;
				int resultMax;
				int unused;
				PlayerInfo::calculateSlow(
					player.slowTimeElapsed + 1,
					timeRemaining,
					slowdown,
					&result,
					&resultMax,
					&unused);
				RavenInfo& ri = player.ravenInfo;
				ri.slowTime = result;
				ri.slowTimeMax = resultMax > 1 ? resultMax : 0;
				ri.hasNeedle = hasLinkedProjectileOfType(player, "SlowNeeldeObjAir")
					|| hasLinkedProjectileOfType(player, "SlowNeeldeObjLand");
				ri.hasOrb = hasLinkedProjectileOfType(player, "AirSettingTypeNeedleObj")
					|| hasLinkedProjectileOfType(player, "LandSettingTypeNeedleObj");
				
				bool hasForceDisableFlag = (player.wasForceDisableFlags & 0x1) != 0;
				
				if (foundNeedleButNotNull) {
					player.ravenNeedleTime = needleTimeRemaining + 1;
					player.ravenNeedleTimeMax = -1;
					if (needleObj.lifeTimeCounter() == 0) {
						player.ravenNeedleElapsed = 0;
					}
				} else if (needleObj) {
					if (!needleObj.isSuperFrozen()) {
						++player.ravenNeedleElapsed;
					}
					PlayerInfo::calculateSlow(
						player.ravenNeedleElapsed,
						needleTimeRemaining,
						needleSlowdown,
						&player.ravenNeedleTime,
						&player.ravenNeedleTimeMax,
						&unused);
					if (player.ravenNeedleTimeMax <= 1) player.ravenNeedleTimeMax = 0;
					else ++player.ravenNeedleTimeMax;
					if (hasForceDisableFlag || player.ravenNeedleTime) ++player.ravenNeedleTime;
				} else if (hasForceDisableFlag) {
					player.ravenNeedleTime = 1;
				} else {
					player.ravenNeedleTime = 0;
				}
				
			} else if (player.charType == CHARACTER_TYPE_DIZZY) {
				
				{  // spears
					bool foundThing = false;
					bool thingIsBomb = false;
					int slowdown = 0;
					int timeRemaining = 0;
					bool hasForceDisableFlag = (player.wasForceDisableFlags & 4096) != 0;
					int fireSpearLifetime = -1;
					player.dizzySpearIsIce = false;
					player.dizzyFireSpearTimeMax = 0;
					bool frozen = false;
					for (int j = 2; j < entityList.count; ++j) {
						Entity p = entityList.list[j];
						if (p.isActive() && p.team() == player.index && !p.isPawn()) {
							bool checkSlowdown = false;
							const char* anim = p.animationName();
							if (strncmp(p.animationName(), "KinomiObj", 9) == 0) {
								if (anim[9] == 'N'
										&& anim[10] == 'e'
										&& anim[11] == 'c'
										&& anim[12] == 'r'
										&& anim[13] == 'o') {
									if (anim[14] == 'b' && anim[15] == 'o' && anim[16] == 'm' && anim[17] == 'b' && anim[18] == '\0') {
										BYTE* func = p.bbscrCurrentFunc();
										if (!func) continue;
										moves.fillDizzyKinomiNecrobomb(func);
										int newTime = moves.dizzyKinomiNecrobomb.remainingTime(p.bbscrCurrentInstr() - func, p.spriteFrameCounter());
										if (newTime > timeRemaining) {
											timeRemaining = newTime;
											checkSlowdown = true;
										}
										foundThing = true;
										thingIsBomb = true;
									} else if (!(foundThing && thingIsBomb)) {
										
										int index;
										if (anim[14] == '\0') index = 0;
										else index = anim[14] - '2' + 1;
										
										int* bombMarker = moves.dizzyKinomiNecroBombMarker + index;
										int* createBomb = moves.dizzyKinomiNecroCreateBomb + index;
										
										BYTE* func = p.bbscrCurrentFunc();
										if (!func) continue;
										moves.fillDizzyKinomiNecro(func, bombMarker, createBomb);
										
										foundThing = true;
										thingIsBomb = false;
										int offset = p.bbscrCurrentInstr() - func;
										if (offset > *bombMarker && offset < *createBomb && timeRemaining != -1) {
											int newTime = 2 - p.spriteFrameCounter();
											
											BYTE* func2 = p.findStateStart("KinomiObjNecrobomb");  // we need this
											moves.fillDizzyKinomiNecrobomb(func2);
											
											if (newTime > timeRemaining) {
												timeRemaining = newTime;
												checkSlowdown = true;
											}
										} else {
											timeRemaining = -1;
										}
										
										int lifetime = p.lifeTimeCounter();
										if (fireSpearLifetime == -1 || lifetime < fireSpearLifetime) {
											fireSpearLifetime = lifetime;
											player.dizzySpearX = p.x();
											player.dizzySpearY = p.y();
											player.dizzySpearSpeedX = p.speedX();
											player.dizzySpearSpeedY = p.speedY();
										}
										
									}
								} else {
									player.dizzySpearIsIce = true;
									player.dizzySpearX = p.x();
									player.dizzySpearY = p.y();
									player.dizzySpearSpeedX = p.speedX();
									player.dizzySpearSpeedY = p.speedY();
								}
							}
							if (checkSlowdown) {
								ProjectileInfo& projectile = findProjectile(p);
								if (projectile.ptr) {
									slowdown = projectile.rcSlowedDownCounter;
								}
								frozen = p.isSuperFrozen();
							}
						}
					}
					
					if (!hasForceDisableFlag) player.dizzyFireSpearElapsed = frozen ? 1 : 0;
					
					int unused;
					if (foundThing && !thingIsBomb) {
						if (timeRemaining == -1) {
							player.dizzyFireSpearTimeMax = -1;
						} else {
							timeRemaining += moves.dizzyKinomiNecrobomb.totalFrames;
							if (!frozen) ++player.dizzyFireSpearElapsed;
							PlayerInfo::calculateSlow(
								player.dizzyFireSpearElapsed,
								timeRemaining,
								slowdown,
								&player.dizzyFireSpearTime,
								&player.dizzyFireSpearTimeMax,
								&unused);
						}
					} else if (foundThing && thingIsBomb) {
						if (!frozen) ++player.dizzyFireSpearElapsed;
						PlayerInfo::calculateSlow(
							player.dizzyFireSpearElapsed,
							timeRemaining,
							slowdown,
							&player.dizzyFireSpearTime,
							&player.dizzyFireSpearTimeMax,
							&unused);
					} else {
						player.dizzyFireSpearTime = 0;
					}
					
					if (player.dizzyFireSpearTime || hasForceDisableFlag) {
						++player.dizzyFireSpearTime;
					}
					if (player.dizzyFireSpearTimeMax != -1) {
						++player.dizzyFireSpearTimeMax;
					}
				}  // spears
				
				{  // scythes
					bool found = false;
					int remainingTime = -1;
					int slowdown = 0;
					for (int j = 2; j < entityList.count; ++j) {
						Entity p = entityList.list[j];
						if (!(
								p.isActive() && p.team() == player.index && !p.isPawn()
								&& strcmp(p.animationName(), "AkariObj") == 0
						)) continue;
						
						found = true;
						
						BYTE* func = p.bbscrCurrentFunc();
						if (!func) continue;
						moves.fillDizzyAkari(func);
						
						int offset = p.bbscrCurrentInstr() - func;
						const Moves::MayIrukasanRidingObjectInfo* foundInfo = nullptr;
						
						// Index 0: Necro Startup
						// Index 1: Necro Loop
						// Index 2: Undine Startup (includes Undine travelling portion)
						// Index 3: Finish (can be entered into by timer from Necro or naturally from Undine. Is entered into on hit)
						int foundInfoIndex = -1;
						for (int k = 0; k < (int)moves.dizzyAkari.size(); ++k) {
							const Moves::MayIrukasanRidingObjectInfo& info = moves.dizzyAkari[k];
							if (offset >= info.frames.front().offset && offset <= info.frames.back().offset) {
								foundInfo = &info;
								foundInfoIndex = k;
								break;
							}
						}
						if (!foundInfo) continue;
						
						ProjectileInfo& projectile = findProjectile(p);
						if (!projectile.ptr) continue;
						
						int animDur = p.currentAnimDuration();
						bool isNecro = p.createArgHikitsukiVal1() == 0;
						bool isKoware = strcmp(p.gotoLabelRequests(), "koware") == 0;
						if (p.lifeTimeCounter() == 0) {
							player.dizzyScytheElapsed = 0;
						}
						
						if (!p.isSuperFrozen() || p.lifeTimeCounter() == 0) ++player.dizzyScytheElapsed;
						
						if (isNecro) {
							if (!isKoware && foundInfoIndex != 3) {
								remainingTime = 76 + moves.dizzyAkari[3].totalFrames - animDur + 1;
							} else if (isKoware) {
								remainingTime = 1 + moves.dizzyAkari[3].totalFrames;
							} else {
								remainingTime = moves.dizzyAkari[3].remainingTime(offset, p.spriteFrameCounter());
							}
						// Undine
						} else if (isKoware) {
							remainingTime = 1 + moves.dizzyAkari[3].totalFrames;
						} else if (foundInfoIndex == 3) {
							remainingTime = moves.dizzyAkari[3].remainingTime(offset, p.spriteFrameCounter());
						} else {
							remainingTime = 1  // one extra frame, because at the start it goes gotoLabelRequests: s32'Undine', which takes 1f to take effect
								+ moves.dizzyAkari[2].totalFrames + moves.dizzyAkari[3].totalFrames - animDur + 1;
						}
						
						slowdown = projectile.rcSlowedDownCounter;
					}
					
					bool hasForceDisableFlag = (player.wasForceDisableFlags & 0x800) != 0;
					if (!found) {
						player.dizzyScytheTime = hasForceDisableFlag ? 1 : 0;
					} else {
						int unused;
						PlayerInfo::calculateSlow(
							player.dizzyScytheElapsed,
							remainingTime,
							slowdown,
							&player.dizzyScytheTime,
							&player.dizzyScytheTimeMax,
							&unused);
						if (player.dizzyScytheTime || hasForceDisableFlag) {
							++player.dizzyScytheTime;
						}
						++player.dizzyScytheTimeMax;
					}
				}  // scythes
				
				{  // fish
					
					player.dizzyShieldFishSuperArmor = false;
					bool foundFish = false;
					bool fishEnding = false;
					int timeRemaining = -1;
					int slowdown = 0;
					bool frozen = false;
					for (int j = 2; j < entityList.count; ++j) {
						Entity p = entityList.list[j];
						if (!(
							p.isActive() && p.team() == player.index && !p.isPawn()
						)) continue;
						
						Moves::MayIrukasanRidingObjectInfo* fishData = nullptr;
						bool fireFish = false;
						int* normal = nullptr;
						int* alt = nullptr;
						BYTE* func;
						const char* animName = p.animationName();
						if (strncmp(animName, "Hanashi", 7) == 0) {
							if (strcmp(animName, "HanashiObjC") == 0) {
								normal = &moves.dizzySFishNormal;
								alt = &moves.dizzySFishAlt;
								fireFish = true;
							} else if (strcmp(animName, "HanashiObjD") == 0) {
								normal = &moves.dizzyHFishNormal;
								alt = &moves.dizzyHFishAlt;
								fireFish = true;
							} else if (strcmp(animName, "HanashiObjA") == 0) {
								fishData = &moves.dizzyPFishEnd;
							} else if (strcmp(animName, "HanashiObjB") == 0) {
								fishData = &moves.dizzyKFishEnd;
							} else if (strcmp(animName, "HanashiObjE") == 0) {
								fishData = &moves.dizzyDFishEnd;
								player.dizzyShieldFishSuperArmor = p.superArmorEnabled();
							} else if (strcmp(animName, "HanashiKoware") == 0) {
								func = p.bbscrCurrentFunc();
								if (!func) continue;
								
								int totalLength = 0;
								for (loopInstr(func)) {
									if (moves.instrType(instr) == instr_sprite) {
										totalLength += asInstr(instr, sprite)->duration;
									}
								}
								
								foundFish = true;
								fishEnding = true;
								int newTime = totalLength - p.currentAnimDuration() + 1;
								if (timeRemaining == -1 || newTime > timeRemaining) {
									ProjectileInfo& projectile = findProjectile(p);
									if (projectile.ptr) slowdown = projectile.rcSlowedDownCounter;
									else slowdown = 0;
									timeRemaining = newTime;
									frozen = p.isSuperFrozen();
								}
								continue;
							} else {
								continue;
							}
							
							foundFish = true;
							
							if (fireFish) {
								func = p.bbscrCurrentFunc();
								if (!func) continue;
								moves.fillDizzyLaserFish(func, normal, alt);
								
								bool isAlt = p.createArgHikitsukiVal1() != 0;
								fishEnding = true;
								int newTime = (isAlt ? *alt : *normal) - p.currentAnimDuration() + 1;
								if (timeRemaining == -1 || newTime > timeRemaining) {
									ProjectileInfo& projectile = findProjectile(p);
									if (projectile.ptr) slowdown = projectile.rcSlowedDownCounter;
									else slowdown = 0;
									timeRemaining = newTime;
									frozen = p.isSuperFrozen();
								}
								continue;
							}
							
							func = p.bbscrCurrentFunc();
							if (!func) continue;
							moves.fillDizzyFish(func, *fishData);
							
							int offset = p.bbscrCurrentInstr() - func;
							if (offset >= fishData->frames.front().offset) {
								fishEnding = true;
								int newTime = fishData->remainingTime(offset, p.spriteFrameCounter());
								if (timeRemaining == -1 || newTime > timeRemaining) {
									ProjectileInfo& projectile = findProjectile(p);
									if (projectile.ptr) slowdown = projectile.rcSlowedDownCounter;
									else slowdown = 0;
									timeRemaining = newTime;
									frozen = p.isSuperFrozen();
								}
							}
						}
						
					}
					
					bool hasForceDisableFlag = (player.wasForceDisableFlags & 0x400) != 0;
					if (!foundFish) {
						player.dizzyFishTime = hasForceDisableFlag ? 1 : 0;
						if (player.dizzyFishTimeMax == -1) player.dizzyFishTimeMax = 9999;
					} else if (fishEnding) {
						if (player.dizzyFishTime == 0) player.dizzyFishElapsed = frozen ? 1 : 0;
						if (!frozen) ++player.dizzyFishElapsed;
						
						int unused;
						PlayerInfo::calculateSlow(
							player.dizzyFishElapsed,
							timeRemaining,
							slowdown,
							&player.dizzyFishTime,
							&player.dizzyFishTimeMax,
							&unused);
						
						if (player.dizzyFishTime || hasForceDisableFlag) ++player.dizzyFishTime;
						
						if (player.dizzyFishTimeMax <= 2) player.dizzyFishTimeMax = 0;
						else ++player.dizzyFishTimeMax;
					} else {
						player.dizzyFishTimeMax = -1;
					}
					
				}  // fish
				
				{  // bubbles
					bool foundBubble = false;
					int timeRemaining = -1;
					int slowdown = 0;
					bool frozen = false;
					bool needIncrementTimeRemaining = false;
					for (int j = 2; j < entityList.count; ++j) {
						Entity p = entityList.list[j];
						if (!(
							p.isActive() && p.team() == player.index
						)) continue;
						
						const char* animName = p.animationName();
						int* koware = nullptr;
						Moves::MayIrukasanRidingObjectInfo* bomb = nullptr;
						BYTE* func = p.bbscrCurrentFunc();
						if (!func) continue;
						if (strcmp(animName, "AwaPObj") == 0) {
							koware = &moves.dizzyAwaPKoware;
							bomb = &moves.dizzyAwaPBomb;
						} else if (strcmp(animName, "AwaKObj") == 0) {
							koware = &moves.dizzyAwaKKoware;
							bomb = &moves.dizzyAwaKBomb;
						}
						if (koware) {
							if (p.lifeTimeCounter() == 0) player.dizzyBubbleElapsed = p.isSuperFrozen() ? 1 : 0;
							moves.fillDizzyAwaKoware(func, koware);
							moves.fillDizzyAwaBomb(func, *bomb);
							foundBubble = true;
							frozen = p.isSuperFrozen();
							int offset = p.bbscrCurrentInstr() - func;
							if (strcmp(p.gotoLabelRequests(), "bomb") == 0) {
								timeRemaining = 1 + bomb->totalFrames;
							} else if (offset >= bomb->frames.front().offset) {
								timeRemaining = bomb->remainingTime(offset, p.spriteFrameCounter());
							} else {
								timeRemaining = 160 + *koware - p.currentAnimDuration() + 1;
								// adding this because apparently we need to finish playing all the sprite and then advance one more "frame" at the end
								needIncrementTimeRemaining = true;
							}
							ProjectileInfo& projectile = findProjectile(p);
							if (projectile.ptr) {
								slowdown = projectile.rcSlowedDownCounter;
							}
						}
					}
					
					bool hasForceDisableFlag = (player.wasForceDisableFlags & 0x2000) != 0;
					if (!foundBubble) {
						player.dizzyBubbleTime = hasForceDisableFlag ? 1 : 0;
					} else {
						if (!frozen) ++player.dizzyBubbleElapsed;
						
						int unused;
						PlayerInfo::calculateSlow(
							player.dizzyBubbleElapsed != 0 && !timeRemaining
								? player.dizzyBubbleElapsed - 1
								: player.dizzyBubbleElapsed,
							timeRemaining,
							slowdown,
							&player.dizzyBubbleTime,
							&player.dizzyBubbleTimeMax,
							&unused);
						
						if (needIncrementTimeRemaining) {
							++player.dizzyBubbleTime;
							++player.dizzyBubbleTimeMax;
						}
						if (player.dizzyBubbleTime || hasForceDisableFlag) {
							++player.dizzyBubbleTime;
						}
						if (player.dizzyBubbleTimeMax <= 3) player.dizzyBubbleTimeMax = 0;
						else ++player.dizzyBubbleTimeMax;
					}
				}  // bubbles
				
			} else if (player.charType == CHARACTER_TYPE_ANSWER) {
				
				int timeRemaining = -1;
				int slowdown = 0;
				bool frozen = false;
				for (int j = 2; j < entityList.count; ++j) {
					Entity p = entityList.list[j];
					if (!(
						p.isActive() && p.team() == player.index && !p.isPawn()
							&& strcmp(p.animationName(), "Nin_Jitsu") == 0
					)) continue;
					
					frozen = p.isSuperFrozen();
					if (p.lifeTimeCounter() == 0) player.answerCantCardElapsed = frozen ? 1 : 0;
					
					timeRemaining = 43 - p.currentAnimDuration() + 1;
					ProjectileInfo& projectile = findProjectile(p);
					if (projectile.ptr) slowdown = projectile.rcSlowedDownCounter;
				}
				
				bool hasForceDisableFlag = (player.wasForceDisableFlags & 1) != 0;
				if (timeRemaining == -1) {
					player.answerCantCardTime = hasForceDisableFlag ? 1 : 0;
				} else {
					if (!frozen) ++player.answerCantCardElapsed;
					int unused;
					PlayerInfo::calculateSlow(
						player.answerCantCardElapsed,
						timeRemaining,
						slowdown,
						&player.answerCantCardTime,
						&player.answerCantCardTimeMax,
						&unused);
					if (hasForceDisableFlag || player.answerCantCardTime) ++player.answerCantCardTime;
					if (player.answerCantCardTimeMax <= 2) player.answerCantCardTimeMax = 0;
					else ++player.answerCantCardTimeMax;
				}
				
				
				bool hasRSFStart = false;
				if (strcmp(player.anim, "Royal_Straight_Flush") == 0) {
					if (player.animFrame > 4) {
						Entity prev = player.pawn.previousEntity();
						if (prev && strcmp(prev.animationName(), "RSF_Start") == 0) {
							Entity link = prev.linkObjectDestroyOnStateChange();
							if (!link) hasRSFStart = true;
						}
					}
				}
				if (player.answerPrevFrameRSFStart != hasRSFStart) {
					player.answerPrevFrameRSFStart = hasRSFStart;
					player.answerCreatedRSFStart = hasRSFStart;
				} else {
					player.answerCreatedRSFStart = false;
				}
				
			} else if (player.charType == CHARACTER_TYPE_MILLIA) {
				
				bool knifeExists = false;
				
				for (const ProjectileInfo& projectile : projectiles) {
					if (projectile.ptr && projectile.team == player.index && strcmp(projectile.animName, "SilentForceKnife") == 0) {
						knifeExists = true;
						break;
					}
				}
				
				player.pickedUpSilentForceKnifeOnThisFrame = !knifeExists && player.prevFrameSilentForceKnifeExisted;
				
				if (player.pickedUpSilentForceKnifeOnThisFrame && players[1 - player.index].inHitstunNowOrNextFrame) {
					ui.comboRecipeUpdatedOnThisFrame[player.index] = true;
					player.comboRecipe.emplace_back();
					ComboRecipeElement& newComboElem = player.comboRecipe.back();
					newComboElem.name = assignName("Pick Up Silent Force");
					newComboElem.timestamp = aswEngineTickCount;
					newComboElem.framebarId = -1;
					newComboElem.doneAfterIdle = true;
					newComboElem.cancelDelayedBy = player.timePassedPureIdle;
				}
				
				player.prevFrameSilentForceKnifeExisted = knifeExists;
				
			}
		}
		
		// This is a separate loop because it depends on another player's timePassedLanding, which I changed in the previous loop
		for (const PlayerInfo& player : players) {
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
	
	hitDetector.prepareDrawHits();  // may delete items from endScene.attackHitboxes
	
	for (AttackHitbox& attackHitbox : attackHitboxes) {
		attackHitbox.found = false;
	}
	
	// WARNING!
	// If the mod was injected in the middle of a round end or round start, player.pawn may be null here!!!
	// This will lead to a crash if you try to use it without checking for null.
	// Better use ent.playerEntity() instead.
	for (int i = 0; i < entityList.count; i++) {
		Entity ent = entityList.list[i];
		int team = ent.team();
		PlayerInfo& player = players[team];
		Entity playerEntity = ent.playerEntity();
		if (!ent.isActive() || isEntityAlreadyDrawn(ent)) continue;

		const bool active = isActiveFull(ent);
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
			lastIgnoredHitNum = &player.lastIgnoredHitNum;
		}
		int hitboxesCount = 0;
		DrawHitboxArrayCallParams hurtbox;
		
		EntityState entityState;
		
		bool useTheseValues = false;
		bool isSuperArmor;
		bool isFullInvul;
		if (i < 2 && (team == 0 || team == 1)) {
			isSuperArmor = player.wasSuperArmorEnabled;
			isFullInvul = player.wasFullInvul;
			useTheseValues = true;
		}
		size_t oldHitboxesSize = drawDataPrepared.hitboxes.size();
		
		int scaleX = INT_MAX;
		int scaleY = INT_MAX;
		if (gifMode.dontHideOpponentsBoxes && team != playerSide
				|| gifMode.dontHidePlayersBoxes && team == playerSide) {
			auto it = findHiddenEntity(ent);
			if (it != hiddenEntities.end()) {
				scaleX = it->scaleX;
				scaleY = it->scaleY;
			}
		}
		
		bool needCollectHitboxes = true;
		if (!ent.isPawn()) {
			for (AttackHitbox& attackHitbox : attackHitboxes) {
				if (ent == attackHitbox.ent) {
					needCollectHitboxes = false;
					drawDataPrepared.hitboxes.push_back(attackHitbox.hitbox);
					hitboxesCount = attackHitbox.count;
					attackHitbox.found = true;
					break;
				}
			}
		}
		
		collectHitboxes(ent,
			active,
			&hurtbox,
			needCollectHitboxes ? &drawDataPrepared.hitboxes : nullptr,
			&drawDataPrepared.points,
			&drawDataPrepared.lines,
			&drawDataPrepared.circles,
			&drawDataPrepared.pushboxes,
			&drawDataPrepared.interactionBoxes,
			needCollectHitboxes ? &hitboxesCount : nullptr,
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
				
				DrawLineCallParams* newLine;
				DrawPointCallParams* newPoint;
				DrawBoxCallParams* newBox;
				
				if (leftEdgeOfArenaOffset) {
					
					int wallX;
					int wallOff;
					int pnt = player.sinHawkBakerStartX;
					if (playerEntity.isFacingLeft()) {
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
		
		if (!ent.isPawn()
				&& (team == 0 || team == 1)) {
			if (player.charType == CHARACTER_TYPE_RAMLETHAL) {
				int bitNumber = 0;
				if (playerEntity.stackEntity(0) == ent) bitNumber = 1;
				if (playerEntity.stackEntity(1) == ent) bitNumber = 2;
				const char* animName = ent.animationName();
				bool animMatches = bitNumber == 1
								&& (
									strcmp(animName, "BitN6C") == 0
									&& ent.mem45()  // bunri only
									|| strcmp(animName, "BitN2C_Bunri") == 0
								)
								|| bitNumber == 2
								&& (
									strcmp(animName, "BitF6D") == 0
									&& ent.mem45()  // bunri only
									|| strcmp(animName, "BitF2D_Bunri") == 0
								);
				int animDuration = ent.currentAnimDuration();
				if (bitNumber && !(animMatches && animDuration < 20)) {
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
		if (!wasHitResult.wasHit || settings.neverDisplayGrayHurtboxes
				|| ent.isHidden()  // for when supers hide the defender. This is needed to not show hitboxes during a super cinematic
		) {
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
				player.hitboxesCount += hitboxesCount;
				if (
						player.hitboxesCount
						&& !player.comboRecipe.empty()
						&& player.lastPerformedMoveNameIsInComboRecipe
				) {
					ComboRecipeElement* lastElem = player.findLastNonProjectileComboElement();
					if (lastElem) {
						lastElem->isMeleeAttack = true;
					}
				}
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
				if (!entityState.strikeInvuln
						&& !player.airborne
						&& hurtbox.hitboxCount) {
					if (hurtboxBounds.bottom <= 88000) {
						player.superLowProfile.active = true;
					} else if (hurtboxBounds.bottom <= 159000) {
						player.lowProfile.active = true;
					} else if (hurtboxBounds.bottom < 175000) {
						player.somewhatLowProfile.active = true;
					} else if (hurtboxBounds.bottom <= 232000
							&& playerEntity.blockstun() == 0
							&& !player.inHitstunNowOrNextFrame) {
						player.upperBodyInvul.active = true;
					} else if (hurtboxBounds.bottom <= 274000
							&& playerEntity.blockstun() == 0
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
				if (player.y > 0) {
					if (hurtbox.hitboxCount && hurtboxBounds.top < 20000 && !entityState.strikeInvuln) {
						player.airborneButWontGoOverLows.active = true;
					} else {
						player.airborneInvul.active = true;
					}
				}
				if (playerEntity.damageToAir() && !player.airborne && !entityState.strikeInvuln) {
					player.consideredAirborne.active = true;
				}
				player.frontLegInvul.active = !entityState.strikeInvuln
					&& player.move.frontLegInvul
					&& player.move.frontLegInvul(player);
				player.superArmor.active = entityState.superArmorActive && !player.projectileOnlyInvul.active && !player.reflect.active;
				if (player.charType == CHARACTER_TYPE_LEO
						&& player.superArmor.active
						&& strcmp(player.anim, "Semuke5E") == 0) {  // Leo bt.D
					for (int projectileSearch = 2; projectileSearch < entityList.count; ++projectileSearch) {
						Entity projectileSearchPtr = entityList.list[projectileSearch];
						if (projectileSearchPtr.isActive()
								&& projectileSearchPtr.team() == player.index
								&& strcmp(projectileSearchPtr.animationName(), "Semuke5E_Reflect") == 0
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
				player.crouching = ent.crouching();
				
				if (drawDataPrepared.pushboxes.size() > pushboxesPrevSize) {
					DrawBoxCallParams& box = drawDataPrepared.pushboxes[pushboxesPrevSize];
					int w;
					w = box.right - box.left;
					player.pushboxWidth = (w < 0 ? -w : w);
					w = box.bottom - box.top;
					player.pushboxHeight = (w < 0 ? -w : w);
				} else {
					player.pushboxWidth = 0;
					player.pushboxHeight = 0;
				}
				
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
	
	for (const AttackHitbox& attackHitbox : attackHitboxes) {
		if (!attackHitbox.found && !invisChipp.needToHide(attackHitbox.team)) {
			drawDataPrepared.hitboxes.push_back(attackHitbox.hitbox);
		}
	}
	
	for (const LeoParry& parry : leoParries) {
		DrawBoxCallParams interactionBoxParams;
		interactionBoxParams.left = parry.x - 400000;
		interactionBoxParams.right = parry.x + 400000;
		interactionBoxParams.top = parry.y + 500000;
		interactionBoxParams.bottom = parry.y - 500000;
		if (parry.aswEngTick >= aswEngineTickCount - 1) {
			interactionBoxParams.fillColor = replaceAlpha(32, COLOR_INTERACTION);
		}
		interactionBoxParams.outlineColor = replaceAlpha(255, COLOR_INTERACTION);
		interactionBoxParams.thickness = THICKNESS_INTERACTION;
		drawDataPrepared.interactionBoxes.push_back(interactionBoxParams);
	}
	
	if (getSuperflashInstigator() == nullptr && frameHasChanged) {
		for (auto it = leoParries.begin(); it != leoParries.end(); ) {
			LeoParry& parry = *it;
			++parry.timer;
			if (parry.timer >= 12) {
				it = leoParries.erase(it);
			} else {
				++it;
			}
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
									&& !(
										player.charType == CHARACTER_TYPE_JAM
										&& strcmp(player.anim, "NeoHochihu") == 0
									)
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
							&& strcmp(player.anim, "Semuke") == 0
							&& (
								strcmp(player.pawn.gotoLabelRequests(), "SemukeFrontWalk") == 0
								|| strcmp(player.pawn.gotoLabelRequests(), "SemukeBackWalk") == 0
								|| player.pawn.speedX() != 0
							)
							|| (
								player.charType == CHARACTER_TYPE_FAUST
								|| player.charType == CHARACTER_TYPE_BEDMAN
							) && (
								strcmp(player.anim, "CrouchFWalk") == 0
								|| strcmp(player.anim, "CrouchBWalk") == 0
							)
							|| player.charType == CHARACTER_TYPE_HAEHYUN
							&& strcmp(player.anim, "CrouchFDash") == 0
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
								!player.elpheltWasPlayerval1
								|| !player.elpheltPrevFrameWasPlayerval1 && player.elpheltWasPlayerval1
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
			int displayedFrames = settings.framebarDisplayedFramesCount;
			if (displayedFrames < 1) {
				displayedFrames = 1;
			}
			int storedFrames = settings.framebarStoredFramesCount;
			if (storedFrames < 1) {
				storedFrames = 1;
			}
			if (storedFrames > _countof(Framebar::frames)) {
				storedFrames = _countof(Framebar::frames);
			}
			if (displayedFrames > storedFrames) {
				displayedFrames = storedFrames;
			}
			
			if (atLeastOneBusy || atLeastOneDangerousProjectilePresent) {
				if (framebarIdleHitstopFor > framebarIdleForLimit) {
					
					memset(skippedFramesHitstop.data(), 0, sizeof SkippedFramesInfo * _countof(Framebar::frames));
					memset(skippedFramesIdleHitstop.data(), 0, sizeof SkippedFramesInfo * _countof(Framebar::frames));
					
					nextSkippedFramesHitstop.clear();
					nextSkippedFramesIdleHitstop.clear();
					
					framebarPositionHitstop = 0;
					framebarTotalFramesHitstopUnlimited = 1;
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
					increaseFramesCountUnlimited(framebarTotalFramesHitstopUnlimited,
						1 + framebarIdleHitstopFor,
						displayedFrames);
					
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
						framebarTotalFramesUnlimited = 1;
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
						increaseFramesCountUnlimited(framebarTotalFramesUnlimited,
							1 + framebarIdleFor,
							displayedFrames);
						
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
					if (!settings.neverIgnoreHitstop) ui.onFramebarAdvanced();
				} else {  // if not  atLeastOneNotInHitstop
					nextSkippedFrames.addFrame(SKIPPED_FRAMES_HITSTOP);
					nextSkippedFramesIdle.addFrame(SKIPPED_FRAMES_HITSTOP);
				}
				if (settings.neverIgnoreHitstop) ui.onFramebarAdvanced();
			} else if (atLeastOneDoingGrab) {  // if not atLeastOneBusy || atLeastOneDangerousProjectilePresent
				nextSkippedFrames.addFrame(skippedType);
				nextSkippedFramesIdle.addFrame(skippedType);
				nextSkippedFramesHitstop.addFrame(skippedType);
				nextSkippedFramesIdleHitstop.addFrame(skippedType);
			} else {
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
			PlayerInfo& player = projectile.team == 0 || projectile.team == 1 ? players[projectile.team] : players[0];
			if (projectile.team == 0 || projectile.team == 1) {
				ignoreThisForPlayer = 
					player.charType == CHARACTER_TYPE_JACKO
					&& (
						strcmp(projectile.animName, "ServantA") == 0
						|| strcmp(projectile.animName, "ServantB") == 0
						|| strcmp(projectile.animName, "ServantC") == 0
						|| strcmp(projectile.animName, "magicAtkLv1") == 0
						|| strcmp(projectile.animName, "magicAtkLv2") == 0
						|| strcmp(projectile.animName, "magicAtkLv3") == 0
					);
			}
			
			if (!projectile.disabled && (projectile.team == 0 || projectile.team == 1)
					&& !projectile.prevStartups.empty()
					&& !ignoreThisForPlayer) {
				
				player.prevStartupsProj = projectile.prevStartups;
				for (int i = 0; i < player.prevStartupsProj.count; ++i) {
					player.prevStartupsProj[i].moveName = nullptr;
				}
			}
			
			bool projectileCanBeHit = false;
			if ((
					projectile.isRamlethalSword
					|| player.charType == CHARACTER_TYPE_ELPHELT
					&& strcmp(projectile.animName, "GrenadeBomb") == 0
					&& projectile.ptr
					&& projectile.ptr.hasUpon(BBSCREVENT_PLAYER_GOT_HIT)
					|| player.charType == CHARACTER_TYPE_JACKO
					&& projectile.ptr
					&& projectile.ptr.servant()
					|| player.charType == CHARACTER_TYPE_DIZZY
					&& strncmp(projectile.animName, "HanashiObj", 10) == 0
					// unfortunately, the code that finds Eddie happens later, so we have to re-find Eddie here
					|| player.charType == CHARACTER_TYPE_ZATO
					&& (
						strcmp(projectile.animName, "ChouDoriru") == 0
						|| player.pawn.playerVal(0)
						&& projectile.ptr
						&& projectile.ptr == getReferredEntity((void*)player.pawn.ent, ENT_STACK_0)
					)
				) && !projectile.strikeInvul
				|| projectile.gotHitOnThisFrame
				&& !isDizzyBubble(projectile.animName)
				&& !isVenomBall(projectile.animName)) {
				projectileCanBeHit = true;
			}
			bool isHouseInvul = player.charType == CHARACTER_TYPE_JACKO && projectile.ptr && projectile.ptr.ghost()
				&& projectile.strikeInvul && projectile.ptr.displayModel()
				&& projectile.ptr.mem45() != 0  // first-time create
				&& projectile.ptr.mem45() != 3;  // second-time create of a previously retrieved house
			bool isMist;
			bool isMistKuttsuku;
			if (player.charType == CHARACTER_TYPE_JOHNNY) {
				if (strcmp(projectile.animName, "Mist") == 0) {
					isMist = true;
					isMistKuttsuku = false;
				} else if (strcmp(projectile.animName, "MistKuttsuku") == 0) {
					isMist = false;
					isMistKuttsuku = projectile.animFrame == 1 && (!projectile.ptr || !projectile.ptr.isRCFrozen());
				} else {
					isMist = false;
					isMistKuttsuku = false;
				}
			} else {
				isMist = false;
				isMistKuttsuku = false;
			}
			
			ProjectileFramebar& entityFramebar = findProjectileFramebar(projectile,
				projectile.markActive
				|| projectile.gotHitOnThisFrame
				|| projectileCanBeHit
				|| isMist
				|| isMistKuttsuku
				|| isHouseInvul);
			entityFramebar.foundOnThisFrame = true;
			Framebar& framebar = entityFramebar.idleHitstop;
			Frame& currentFrame = framebar[framebarPos];
			
			FrameType defaultIdleFrame;
			if (isHouseInvul && !projectileCanBeHit
					|| projectile.gotHitOnThisFrame && (
						isDizzyBubble(projectile.animName) || isVenomBall(projectile.animName) || isGrenadeBomb(projectile.animName)
					)) {
				defaultIdleFrame = FT_IDLE_NO_DISPOSE;
			} else if (isMist || isMistKuttsuku) {
				defaultIdleFrame = FT_BACCHUS_SIGH;
			} else {
				defaultIdleFrame = projectileCanBeHit ? FT_IDLE_PROJECTILE_HITTABLE : FT_IDLE_PROJECTILE;
			}
			
			if (framebarAdvancedIdleHitstop
					|| (
						projectile.markActive
						&& !projectile.startedUp
						&& superflashInstigator
					)) {
				currentFrame.type = defaultIdleFrame;
				currentFrame.marker = isHouseInvul
						|| player.charType == CHARACTER_TYPE_DIZZY
						&& projectileCanBeHit
						&& player.dizzyShieldFishSuperArmor;
				if (projectile.ptr || !projectile.lastName) {
					projectile.determineMoveNameAndSlangName(&currentFrame.animName);
				} else {
					currentFrame.animName = projectile.lastName;
				}
				currentFrame.hitstop = projectile.hitstopWithSlow;
				currentFrame.hitstopMax = projectile.hitstopMaxWithSlow;
				currentFrame.hitConnected = projectile.hitConnectedForFramebar() || projectile.gotHitOnThisFrame
					|| isMistKuttsuku;
				currentFrame.rcSlowdown = projectile.rcSlowedDownCounter;
				currentFrame.rcSlowdownMax = projectile.rcSlowedDownMax;
				currentFrame.activeDuringSuperfreeze = false;
				currentFrame.powerup = projectile.move.projectilePowerup && projectile.move.projectilePowerup(projectile);
				currentFrame.next = nullptr;
				currentFrame.charSpecific1 = player.charType == CHARACTER_TYPE_RAMLETHAL
					&& projectile.ptr == player.pawn.stackEntity(0)
					|| player.charType == CHARACTER_TYPE_HAEHYUN
					&& projectile.haehyunCelestialTuningBall1;
				currentFrame.charSpecific2 = player.charType == CHARACTER_TYPE_RAMLETHAL
					&& projectile.ptr == player.pawn.stackEntity(1)
					|| player.charType == CHARACTER_TYPE_HAEHYUN
					&& projectile.haehyunCelestialTuningBall2;
				if (!projectile.dontReplaceFramebarTitle
						|| currentFrame.title.text == nullptr || *currentFrame.title.text->name == '\0') {
					currentFrame.title = projectile.framebarTitle;
				} else if (framebarPos != 0 || framebarTotalFramesHitstopUnlimited > 0 || framebarIdleHitstopFor > 0) {
					currentFrame.title = framebar[
						framebarPos == 0
							? _countof(Framebar::frames) - 1
							: framebarPos - 1
					].title;
				}
			}
			if (!framebarAdvancedIdleHitstop && superflashInstigator && projectile.gotHitOnThisFrame) {
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
					if (currentFrame.type == defaultIdleFrame || currentFrame.type == FT_NONE) {
						currentFrame.type = FT_IDLE_NO_DISPOSE;
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
				if (entityFramebar.foundOnThisFrame) continue;
				
				Framebar& framebar = entityFramebar.idleHitstop;
				Frame& currentFrame = framebar[framebarPos];
				
				currentFrame.type = FT_IDLE_PROJECTILE;
				currentFrame.activeDuringSuperfreeze = false;
				currentFrame.animName = nullptr;
				currentFrame.hitstop = 0;
				currentFrame.hitstopMax = 0;
				currentFrame.hitConnected = false;
				currentFrame.rcSlowdown = 0;
				currentFrame.rcSlowdownMax = 0;
				currentFrame.activeDuringSuperfreeze = false;
				currentFrame.powerup = false;
				currentFrame.next = nullptr;
				currentFrame.marker = false;
				currentFrame.charSpecific1 = false;
				currentFrame.charSpecific2 = false;
				
				if (framebarPos != 0 || framebarTotalFramesHitstopUnlimited > 0 || framebarIdleHitstopFor > 0) {
					currentFrame.title = framebar[
						framebarPos == 0
							? _countof(Framebar::frames) - 1
							: framebarPos - 1
					].title;
				}
				
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
					ent = getReferredEntity((void*)player.pawn.ent, ENT_STACK_0);
				}
				bool created = false;
				
				Entity landminePtr = nullptr;
				for (ProjectileInfo& projectile : projectiles) {
					if (strcmp(projectile.animName, "ChouDoriru") == 0
							&& projectile.ptr
							&& projectile.team == player.index) {
						landminePtr = projectile.ptr;
						if (!player.eddie.landminePtr) {
							projectile.creationTime_aswEngineTick = player.eddie.moveStartTime_aswEngineTick;
							projectile.startup = player.eddie.total;
							memset(projectile.creatorName, 0, 32);
							strcpy(projectile.creatorName, "Eddie");
							projectile.creator = player.eddie.ptr;
							projectile.creatorNamePtr = "Eddie";
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
						projectile.alreadyIncludedInComboRecipe = false;
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
				if (player.idle && !player.isRunning && !player.isWalkingForward && !player.isWalkingBackward) {
					if (!player.hitstop) {
						++player.timePassedPureIdle;
					}
				} else {
					player.timePassedPureIdle = 0;
				}
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
									p.mem45() && strcmp(p.gotoLabelRequests(), "hit") != 0
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
					if (blitzShieldCancellable(player, false)
							&& !player.hitstop
							&& isBlitzPostHitstopFrame_outsideTick(player)) {
						startupFrameType = FT_STARTUP_CAN_BLOCK;
					} else {
						startupFrameType = FT_STARTUP_CAN_BLOCK_AND_CANCEL;
					}
				} else {
					startupFrameType = FT_STARTUP_CAN_BLOCK;
				}
			} else {
				startupFrameType = FT_STARTUP;
			}
			
			Entity ent = entityList.slots[i];
			bool hasHitboxes = player.hitboxesCount > 0
				|| player.charType == CHARACTER_TYPE_ANSWER
				&& strcmp(player.anim, "Zaneiken") == 0
				&& player.hitstop
				&& hitDetector.hasHitboxThatHit(player.pawn);
			bool enableSpecialCancel = player.wasEnableSpecialCancel
					&& player.wasAttackCollidedSoCanCancelNow;
					//&& player.wasEnableGatlings;  // Jack-O 5H can special cancel even without this flag
					// I also checked the game's code and it does not use the gatlings flag for special cancels
			bool hitAlreadyHappened = player.pawn.hitAlreadyHappened() >= player.pawn.theValueHitAlreadyHappenedIsComparedAgainst()
					|| !player.pawn.currentHitNum();
			player.getInputs(game.getInputRingBuffers() + player.index, isTheFirstFrameInTheRound);
			
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
				
				currentFrame.canYrc = player.wasCanYrc;
				currentFrame.cantRc = player.wasCantRc;
				// for Ramlethal BitLaser, we redefine this in character-specific codes below
				currentFrame.canYrcProjectile = player.wasCanYrc && player.move.canYrcProjectile
					? player.move.canYrcProjectile(player) : nullptr;
				
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
					milliaInfo.hasPin = hasProjectileOfTypeAndHasNotExhausedHit(player, "SilentForceKnife");
					milliaInfo.hasSDisc = hasProjectileOfType(player, "TandemTopCRing");
					milliaInfo.hasHDisc = hasProjectileOfType(player, "TandemTopDRing");
					milliaInfo.hasEmeraldRain = hasAnyProjectileOfTypeStrNCmp(player, "EmeraldRainRing");
					milliaInfo.hasHitstunLinkedSecretGarden = hasLinkedProjectileOfType(player, "SecretGardenBall");
					milliaInfo.hasRose = hasAnyProjectileOfType(player, "RoseObj");
					currentFrame.u.milliaInfo = milliaInfo;
				} else if (player.charType == CHARACTER_TYPE_CHIPP) {
					ChippInfo& ci = currentFrame.u.chippInfo;
					ci.invis = player.playerval0;
					ci.wallTime = 0;
					if (player.move.caresAboutWall) {
						int wallTime = player.pawn.mem54();
						if (wallTime > USHRT_MAX || wallTime <= 0) wallTime = USHRT_MAX;
						ci.wallTime = wallTime;
					}
					ci.hasShuriken = hasAnyProjectileOfTypeStrNCmp(player, "ShurikenObj");  // is linked, but never unlinks, so we just check the projectile
					ci.hasKunaiWall = hasAnyProjectileOfType(player, "Kunai_Wall");
					ci.hasRyuuYanagi = hasAnyProjectileOfType(player, "Kunai");
				} else if (player.charType == CHARACTER_TYPE_SOL) {
					SolInfo& si = currentFrame.u.solInfo;
					si.currentDI = player.playerval1 < 0 ? USHRT_MAX : player.playerval1;
					si.maxDI = player.maxDI;
					
					bool gunflameDisappearsOnHit = false;
					bool gunflameComesOutLater = false;
					bool gunflameFirstWaveDisappearsOnHit = false;
					
					analyzeGunflame(player, &gunflameDisappearsOnHit,
						&gunflameComesOutLater, &gunflameFirstWaveDisappearsOnHit);
					
					si.gunflameDisappearsOnHit = gunflameDisappearsOnHit;
					si.gunflameComesOutLater = gunflameComesOutLater;
					si.gunflameFirstWaveDisappearsOnHit = gunflameFirstWaveDisappearsOnHit;
					
					si.hasTyrantRavePunch2 = hasProjectileOfType(player, "TyrantRavePunch2_DI");
				} else if (player.charType == CHARACTER_TYPE_KY) {
					KyInfo& ki = currentFrame.u.kyInfo;
					ki.stunEdgeWillDisappearOnHit = hasLinkedProjectileOfType(player, "StunEdgeObj");
					ki.hasChargedStunEdge = hasProjectileOfType(player, "ChargedStunEdgeObj");
					ki.hasSPChargedStunEdge = hasProjectileOfType(player, "SPChargedStunEdgeObj");
					ki.hasjD = hasProjectileOfType(player, "AirDustAttackObj");
				} else if (player.charType == CHARACTER_TYPE_ZATO) {
					ZatoInfo& zi = currentFrame.u.zatoInfo;
					zi.currentEddieGauge = player.pawn.exGaugeValue(0);
					zi.maxEddieGauge = 6000;
					// player.eddie.ptr is up to date here
					zi.hasGreatWhite = player.eddie.ptr && strcmp(player.eddie.anim, "EddieMegalithHead") == 0;
					zi.hasInviteHell = hasAnyProjectileOfTypeStrNCmp(player, "Drill");
					zi.hasEddie = player.eddie.ptr;
				} else if (player.charType == CHARACTER_TYPE_SLAYER) {
					SlayerInfo& si = currentFrame.u.slayerInfo;
					si.currentBloodsuckingUniverseBuff = player.wasPlayerval1Idling;
					si.maxBloodsuckingUniverseBuff = player.maxDI;
					si.hasRetro = hasProjectileOfType(player, "Retro");
				} else if (player.charType == CHARACTER_TYPE_INO) {
					InoInfo& ii = currentFrame.u.inoInfo;
					ii.airdashTimer = player.wasProhibitFDTimer;
					ii.noteTime = player.noteTimeWithSlow;
					ii.noteTimeMax = player.noteTimeWithSlowMax;
					ii.noteLevel = player.noteLevel;
					ii.hasChemicalLove = hasLinkedProjectileOfType(player, "BChemiLaser")
						|| hasLinkedProjectileOfType(player, "CChemiLaser");
					ii.hasNote = hasLinkedProjectileOfType(player, "Onpu");
					ii.has5DYRC = hasLinkedProjectileOfType(player, "DustObjShot");
				} else if (player.charType == CHARACTER_TYPE_BEDMAN) {
					fillInBedmanSealInfo(player);
					
					currentFrame.u.bedmanInfo = player.bedmanInfo;
				} else if (player.charType == CHARACTER_TYPE_RAMLETHAL) {
					fillRamlethalDisappearance(currentFrame, player);
				} else if (player.charType == CHARACTER_TYPE_ELPHELT) {
					ElpheltInfo& ei = currentFrame.u.elpheltInfo;
					ei.grenadeTimer = player.wasResource;
					ei.grenadeDisabledTimer = min(255, player.elpheltGrenadeRemainingWithSlow);
					ei.grenadeDisabledTimerMax = min(255, player.elpheltGrenadeMaxWithSlow);
					ei.hasGrenade = elpheltGrenadeExists(player);
					ei.hasJD = elpheltJDExists(player);
				} else if (player.charType == CHARACTER_TYPE_JOHNNY) {
					JohnnyInfo& ji = currentFrame.u.johnnyInfo;
					ji.mistTimer = max_inline(0, min_inline(1023, player.johnnyMistTimerWithSlow));
					ji.mistTimerMax = max_inline(0, min_inline(1023, player.johnnyMistTimerMaxWithSlow));
					ji.mistKuttsukuTimer = max_inline(0, min_inline(1023, player.johnnyMistKuttsukuTimerWithSlow));
					ji.mistKuttsukuTimerMax = max_inline(0, min_inline(1023, player.johnnyMistKuttsukuTimerMaxWithSlow));
					ji.hasMistKuttsuku = hasAnyProjectileOfType(player, "MistKuttsuku");
					ji.hasMist = hasLinkedProjectileOfType(player, "Mist");
				} else if (player.charType == CHARACTER_TYPE_RAVEN) {
					RavenInfo& ri = currentFrame.u.ravenInfo;
					ri = player.ravenInfo;
				} else if (player.charType == CHARACTER_TYPE_DIZZY) {
					fillDizzyInfo(player, currentFrame);
				} else if (player.charType == CHARACTER_TYPE_MAY) {
					MayInfo& mi = currentFrame.u.mayInfo;
					mi.hasDolphin = hasProjectileOfType(player, "IrukasanRidingObject");
					mi.hasBeachBall = hasProjectileOfTypeStrNCmp(player, "MayBall");
				} else if (player.charType == CHARACTER_TYPE_POTEMKIN) {
					currentFrame.u.potemkinInfo.hasBomb = hasLinkedProjectileOfType(player, "Bomb");  // Trishula
				} else if (player.charType == CHARACTER_TYPE_FAUST) {
					currentFrame.u.faustInfo.hasFlower = hasAnyProjectileOfType(player, "OreHana_Shot")
						|| hasAnyProjectileOfType(player, "OreHanaBig_Shot");
				} else if (player.charType == CHARACTER_TYPE_AXL) {
					AxlInfo& ai = currentFrame.u.axlInfo;
					ai.hasSpindleSpinner = hasProjectileOfType(player, "RashosenObj");
					ai.hasSickleFlash = hasAnyProjectileOfType(player, "RensengekiObj");
					ai.hasMelodyChain = hasAnyProjectileOfType(player, "KyokusagekiObj");
					ai.hasSickleStorm = hasAnyProjectileOfType(player, "ByakueObj");
				} else if (player.charType == CHARACTER_TYPE_VENOM) {
					VenomInfo& vi = currentFrame.u.venomInfo;
					static const char dubiousCurveName[] = "DubiousCurve";
					const char dubiousCurveLetter = player.anim[sizeof dubiousCurveName - 1];
					const bool dubiousCurve = strncmp(player.anim, dubiousCurveName, sizeof dubiousCurveName - 1) == 0;
					const bool hasChangedState = player.pawn.hasUpon(BBSCREVENT_PLAYER_CHANGED_STATE);
					const bool hasQVShockwave = hasProjectileOfType(player, "Debious_AttackBall");
					const bool isFirstFrameOfLackOfChangedState = dubiousCurve
						&& !hasChangedState
						&& player.pawn.bbscrCurrentFunc()
						&& dubiousCurveLetter >= 'A'
						&& dubiousCurveLetter <= 'D'
						&& player.pawn.bbscrCurrentInstr() ==
							player.pawn.bbscrCurrentFunc() + *moves.venomQvClearUponAfterExitOffsetArray[dubiousCurveLetter - 'A']
						&& player.pawn.spriteFrameCounter() == 0;
					
					vi.hasQV = hasQVShockwave
						&& dubiousCurve
						&& hasChangedState;
					
					vi.hasQVYRCOnly = hasQVShockwave && isFirstFrameOfLackOfChangedState;
					vi.hasHCarcassBall = hasHitstunTiedVenomBall(player);
					vi.performingQVA = false;
					vi.performingQVB = false;
					vi.performingQVC = false;
					vi.performingQVD = false;
					vi.performingQVAHitOnly = false;
					vi.performingQVBHitOnly = false;
					vi.performingQVCHitOnly = false;
					vi.performingQVDHitOnly = false;
					if (dubiousCurve && (
							hasChangedState
							|| isFirstFrameOfLackOfChangedState
						)
					) {
						switch (dubiousCurveLetter) {
							case 'A': vi.performingQVA = true; break;
							case 'B': vi.performingQVB = true; break;
							case 'C': vi.performingQVC = true; break;
							case 'D': vi.performingQVD = true; break;
							// here C# would complain about lack of a default case
						}
					}
					if (dubiousCurve && player.pawn.hasUpon(BBSCREVENT_PLAYER_GOT_HIT)) {
						switch (dubiousCurveLetter) {
							case 'A': vi.performingQVAHitOnly = true; break;
							case 'B': vi.performingQVBHitOnly = true; break;
							case 'C': vi.performingQVCHitOnly = true; break;
							case 'D': vi.performingQVDHitOnly = true; break;
							// here C# would complain about lack of a default case
						}
					}
				} else if (player.charType == CHARACTER_TYPE_LEO) {
					LeoInfo& li = currentFrame.u.leoInfo;
					li.hasEdgeyowai = hasLinkedProjectileOfType(player, "Edgeyowai");
					li.hasEdgetuyoi = hasLinkedProjectileOfType(player, "Edgetuyoi");
				} else if (player.charType == CHARACTER_TYPE_JACKO) {
					fillInJackoInfo(player, currentFrame);
				} else if (player.charType == CHARACTER_TYPE_HAEHYUN) {
					HaehyunInfo& hi = currentFrame.u.haehyunInfo;
					
					bool cantDoBall = (player.wasForceDisableFlags & 0x1) != 0 && player.haehyunBallTimeWithSlow != -1;
					hi.cantDoBall = cantDoBall;
					
					if (!cantDoBall) {
						hi.ballTime = min_inline(USHRT_MAX, max_inline(0, player.haehyunBallRemainingTimeWithSlow));
						hi.ballTimeMax = min_inline(USHRT_MAX, max_inline(0, player.haehyunBallRemainingTimeMaxWithSlow));
					} else {
						hi.ballTime = min_inline(USHRT_MAX, max_inline(0, player.haehyunBallTimeWithSlow));
						hi.ballTimeMax = min_inline(USHRT_MAX, max_inline(0, player.haehyunBallTimeMaxWithSlow));
					}
					
					bool ended = false;
					for (int i = 0; i < 2; ++i) {
						if (!ended && player.haehyunSuperBallRemainingTimeWithSlow[i]) {
							hi.superballTime[i].time = min_inline(USHRT_MAX, max_inline(0, player.haehyunSuperBallRemainingTimeWithSlow[i]));
							hi.superballTime[i].timeMax = min_inline(USHRT_MAX, max_inline(0, player.haehyunSuperBallRemainingTimeMaxWithSlow[i]));
						} else {
							ended = true;
							hi.superballTime[i].time = 0;
							hi.superballTime[i].timeMax = 0;
						}
					}
					
					hi.hasBall = hasLinkedProjectileOfType(player, "EnergyBall");
					hi.has5D = hasLinkedProjectileOfType(player, "kum_205shot");
				} else if (player.charType == CHARACTER_TYPE_BAIKEN) {
					BaikenInfo& bi = currentFrame.u.baikenInfo;
					bi.has5D = hasLinkedProjectileOfType(player, "NmlAtk5EShotObj");
					bi.hasJD = hasLinkedProjectileOfType(player, "NmlAtkAir5EShotObj");
					bi.hasTeppou = hasLinkedProjectileOfType(player, "TeppouObj");
					bi.hasTatami = hasLinkedProjectileOfType(player, "TatamiAirObj")
						|| hasLinkedProjectileOfType(player, "TatamiLandObj");
				} else if (player.charType == CHARACTER_TYPE_ANSWER) {
					AnswerInfo& ai = currentFrame.u.answerInfo;
					
					ai.hasCardDestroyOnDamage = false;
					ai.hasCardPlayerGotHit = false;
					ai.hasRSFStart = false;
					ai.hasClone = false;
					for (ProjectileInfo& projectile : projectiles) {
						if (projectile.team == player.index && projectile.isDangerous) {
							if (strcmp(projectile.animName, "Meishi") == 0) {
								if (projectile.ptr.hasUpon(BBSCREVENT_PLAYER_GOT_HIT)) {
									ai.hasCardPlayerGotHit = true;
								}
								if (projectile.ptr.linkObjectDestroyOnDamage() != nullptr) {
									ai.hasCardDestroyOnDamage = true;
								}
							} else if (strcmp(projectile.animName, "RSF_Start") == 0) {
								if (projectile.ptr.hasUpon(BBSCREVENT_PLAYER_GOT_HIT)) {
									ai.hasRSFStart = true;
								}
							} else if (strcmp(projectile.animName, "Nin_Jitsu") == 0) {
								if (projectile.ptr.linkObjectDestroyOnDamage() != nullptr) {
									ai.hasClone = true;
								}
							}
						}
					}
					
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
							strcmp(player.anim, "CounterGuardStand") == 0
							|| strcmp(player.anim, "CounterGuardCrouch") == 0
							|| strcmp(player.anim, "CounterGuardAir") == 0
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
				player.determineMoveNameAndSlangName(&currentFrame.animName);
				if (prevFrame.cancels && prevFrame.cancels->equalTruncated(player.wasCancels)) {
					currentFrame.cancels = prevFrame.cancels;
				} else if (player.wasCancels.gatlings.empty() && player.wasCancels.whiffCancels.empty() && !player.wasCancels.whiffCancelsNote) {
					currentFrame.cancels = nullptr;
				} else {
					if (!currentFrame.cancels || currentFrame.cancels.use_count() > 1) {
						ThreadUnsafeSharedResource<FrameCancelInfoStored>* newResource = new ThreadUnsafeSharedResource<FrameCancelInfoStored>();
						currentFrame.cancels = ThreadUnsafeSharedPtr<FrameCancelInfoStored>(newResource);
					}
					currentFrame.cancels->copyFromAnotherArray(player.wasCancels);
				}
				currentFrame.dustGatlingTimer = player.dustGatlingTimer;
				currentFrame.dustGatlingTimerMax = player.dustGatlingTimerMax;
				currentFrame.hitstop = player.hitstopWithSlow;
				currentFrame.hitstopMax = player.hitstopMaxWithSlow;
				if (player.stagger && player.cmnActIndex == CmnActJitabataLoop) {
					currentFrame.stop.isBlockstun = false;
					currentFrame.stop.isHitstun = false;
					currentFrame.stop.isStagger = true;
					currentFrame.stop.isWakeup = false;
					currentFrame.stop.isRejection = false;
					currentFrame.stop.value = min(player.staggerWithSlow, 8192);
					currentFrame.stop.valueMax = min(player.staggerMaxWithSlow, 2047);
					currentFrame.stop.valueMaxExtra = 0;
					currentFrame.stop.tumble = 0;
				} else if (player.blockstun) {
					currentFrame.stop.isBlockstun = true;
					currentFrame.stop.isHitstun = false;
					currentFrame.stop.isStagger = false;
					currentFrame.stop.isWakeup = false;
					currentFrame.stop.isRejection = false;
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
					currentFrame.stop.isRejection = false;
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
						currentFrame.stop.tumbleIsWallstick = false;
						currentFrame.stop.tumbleIsKnockdown = false;
					} else if (player.cmnActIndex == CmnActWallHaritsuki) {
						currentFrame.stop.tumble = min(player.wallstickWithSlow, 0xffff);
						currentFrame.stop.tumbleMax = min(player.wallstickMaxWithSlow, 0xffff);
						currentFrame.stop.tumbleIsWallstick = true;
						currentFrame.stop.tumbleIsKnockdown = false;
					} else if (player.cmnActIndex == CmnActFDownLoop
							|| player.cmnActIndex == CmnActBDownLoop
							|| player.cmnActIndex == CmnActVDownLoop) {
						currentFrame.stop.tumble = min(player.knockdownWithSlow, 0xffff);
						currentFrame.stop.tumbleMax = min(player.knockdownMaxWithSlow, 0xffff);
						currentFrame.stop.tumbleIsWallstick = false;
						currentFrame.stop.tumbleIsKnockdown = true;
					} else {
						currentFrame.stop.tumble = 0;
						currentFrame.stop.tumbleMax = 0;
					}
				} else if (player.wakeupTiming) {
					currentFrame.stop.isBlockstun = false;
					currentFrame.stop.isHitstun = false;
					currentFrame.stop.isStagger = false;
					currentFrame.stop.isWakeup = true;
					currentFrame.stop.isRejection = false;
					currentFrame.stop.value = min(player.wakeupTimingWithSlow, 8192);
					currentFrame.stop.valueMax = min(player.wakeupTimingMaxWithSlow, 2047);
					currentFrame.stop.valueMaxExtra = 0;
					currentFrame.stop.tumble = 0;
					currentFrame.stop.tumbleMax = 0;
				} else if (player.cmnActIndex == CmnActHajikareStand
						|| player.cmnActIndex == CmnActHajikareCrouch
						|| player.cmnActIndex == CmnActHajikareAir) {
					currentFrame.stop.isBlockstun = false;
					currentFrame.stop.isHitstun = false;
					currentFrame.stop.isStagger = false;
					currentFrame.stop.isWakeup = false;
					currentFrame.stop.isRejection = true;
					currentFrame.stop.value = min(player.rejectionWithSlow, 8192);
					currentFrame.stop.valueMax = min(player.rejectionMaxWithSlow, 2047);
					currentFrame.stop.valueMaxExtra = 0;
					currentFrame.stop.tumble = 0;
					currentFrame.stop.tumbleMax = 0;
				} else if (player.wallslumpLand && player.cmnActIndex == CmnActWallHaritsukiLand) {
					currentFrame.stop.isBlockstun = false;
					currentFrame.stop.isHitstun = true;
					currentFrame.stop.isStagger = false;
					currentFrame.stop.isWakeup = false;
					currentFrame.stop.isRejection = false;
					currentFrame.stop.value = min(player.wallslumpLandWithSlow, 8192);
					currentFrame.stop.valueMax = min(player.wallslumpLandMaxWithSlow, 2047);
					currentFrame.stop.valueMaxExtra = 0;
					currentFrame.stop.tumble = 0;
					currentFrame.stop.tumbleMax = 0;
				} else {
					currentFrame.stop.isBlockstun = false;
					currentFrame.stop.isHitstun = false;
					currentFrame.stop.isStagger = false;
					currentFrame.stop.isWakeup = false;
					currentFrame.stop.isRejection = false;
					if (player.cmnActIndex == CmnActKorogari) {
						currentFrame.stop.tumble = min(player.tumbleWithSlow, 0xffff);
						currentFrame.stop.tumbleMax = min(player.tumbleMaxWithSlow, 0xffff);
						currentFrame.stop.tumbleIsWallstick = false;
						currentFrame.stop.tumbleIsKnockdown = false;
					} else if (player.cmnActIndex == CmnActWallHaritsuki) {
						currentFrame.stop.tumble = min(player.wallstickWithSlow, 0xffff);
						currentFrame.stop.tumbleMax = min(player.wallstickMaxWithSlow, 0xffff);
						currentFrame.stop.tumbleIsWallstick = true;
						currentFrame.stop.tumbleIsKnockdown = false;
					} else if (player.cmnActIndex == CmnActFDownLoop
							|| player.cmnActIndex == CmnActBDownLoop
							|| player.cmnActIndex == CmnActVDownLoop) {
						currentFrame.stop.tumble = min(player.knockdownWithSlow, 0xffff);
						currentFrame.stop.tumbleMax = min(player.knockdownMaxWithSlow, 0xffff);
						currentFrame.stop.tumbleIsWallstick = false;
						currentFrame.stop.tumbleIsKnockdown = true;
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
					currentFrame.crossedUp = false;
				} else {
					currentFrame.crossupProtectionIsOdd = 0;
					currentFrame.crossupProtectionIsAbove1 = 0;
					currentFrame.crossedUp = player.pawn.crossupProtection() == 3;
				}
				currentFrame.rcSlowdown = player.rcSlowedDownCounter;
				currentFrame.rcSlowdownMax = player.rcSlowedDownMax;
				
				if (player.poisonDuration) {
					currentFrame.poisonDuration = player.poisonDuration;
					currentFrame.poisonMax = 360;
					currentFrame.poisonIsBacchusSigh = false;
					currentFrame.poisonIsRavenSlow = false;
				} else if (other.charType == CHARACTER_TYPE_JOHNNY) {
					currentFrame.poisonDuration = other.johnnyMistKuttsukuTimerWithSlow;
					currentFrame.poisonMax = other.johnnyMistKuttsukuTimerMaxWithSlow;
					currentFrame.poisonIsBacchusSigh = true;
					currentFrame.poisonIsRavenSlow = false;
				} else if (other.charType == CHARACTER_TYPE_RAVEN) {
					currentFrame.poisonDuration = other.ravenInfo.slowTime;
					currentFrame.poisonMax = other.ravenInfo.slowTimeMax;
					currentFrame.poisonIsBacchusSigh = false;
					currentFrame.poisonIsRavenSlow = true;
				} else {
					currentFrame.poisonDuration = 0;
					currentFrame.poisonMax = 0;
					currentFrame.poisonIsBacchusSigh = false;
					currentFrame.poisonIsRavenSlow = false;
				}
				
				currentFrame.needShowAirOptions = player.regainedAirOptions && player.airborne;
				currentFrame.doubleJumps = player.remainingDoubleJumps;
				currentFrame.airDashes = player.remainingAirDashes;
				currentFrame.activeDuringSuperfreeze = false;
				
				currentFrame.multipleInputs = player.inputs.size() != 1;
				if (currentFrame.multipleInputs) {
					if (!currentFrame.inputs || currentFrame.inputs.use_count() != 1) {
						currentFrame.inputs = new ThreadUnsafeSharedResource<std::vector<Input>>();
					}
					std::vector<Input>& currentFrameInputs = *currentFrame.inputs;
					currentFrameInputs = player.inputs;
				} else {
					currentFrame.input = player.inputs[0];
				}
				currentFrame.prevInput = player.prevInput;
				currentFrame.inputsOverflow = player.inputsOverflow;
				if (player.inputs.empty()) {
					player.prevInput = Input{0x0000};
				} else {
					player.prevInput = player.inputs.back();
				}
				player.inputsOverflow = false;
				player.inputs.clear();
				
				if (player.move.createdProjectile) {
					const CreatedProjectileStruct* proj = player.move.createdProjectile(player);
					if (proj) {
						if (!currentFrame.createdProjectiles || currentFrame.createdProjectiles.use_count() != 1) {
							currentFrame.createdProjectiles = new ThreadUnsafeSharedResource<std::vector<CreatedProjectileStruct>>();
						} else {
							currentFrame.createdProjectiles->clear();
						}
						currentFrame.createdProjectiles->push_back(*proj);
					} else if (currentFrame.createdProjectiles && !currentFrame.createdProjectiles->empty()) {
						if (currentFrame.createdProjectiles.use_count() != 1) {
							currentFrame.createdProjectiles = nullptr;
						} else {
							currentFrame.createdProjectiles->clear();
						}
					}
				} else if (player.createdProjectiles.empty()) {
					if (currentFrame.createdProjectiles && !currentFrame.createdProjectiles->empty()) {
						if (currentFrame.createdProjectiles.use_count() != 1) {
							currentFrame.createdProjectiles = nullptr;
						} else {
							currentFrame.createdProjectiles->clear();
						}
					}
				} else {
					if (!currentFrame.createdProjectiles || currentFrame.createdProjectiles.use_count() != 1) {
						currentFrame.createdProjectiles = new ThreadUnsafeSharedResource<std::vector<CreatedProjectileStruct>>();
					}
					*currentFrame.createdProjectiles = player.createdProjectiles;
				}
				
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
				
				static const StringWithLength elpheltShotgunFire = "Shotgun_Fire_";
				currentFrame.powerup = player.move.powerup ? player.move.powerup(player) : nullptr;
				if (currentFrame.powerup) {
					currentFrame.dontShowPowerupGraphic = player.move.dontShowPowerupGraphic ? player.move.dontShowPowerupGraphic(player) : false;
				} else if (player.charType == CHARACTER_TYPE_MILLIA && player.pickedUpSilentForceKnifeOnThisFrame) {
					currentFrame.powerup = "Picked up Silent Force";
					currentFrame.dontShowPowerupGraphic = false;
				} else if (player.charType == CHARACTER_TYPE_ELPHELT
						&& player.playerval0 && !player.elpheltPrevFrameWasPlayerval1 && player.elpheltWasPlayerval1
						&& (
							player.cmnActIndex == CmnActCrouch2Stand
							|| player.cmnActIndex == CmnActStand
							|| player.cmnActIndex == NotACmnAct
							&& strncmp(player.anim, elpheltShotgunFire.txt, elpheltShotgunFire.length) == 0
						)) {
					currentFrame.powerup = "Ms. Travailler reached maximum charge.";
					currentFrame.dontShowPowerupGraphic = false;
				} else {
					currentFrame.dontShowPowerupGraphic = false;
				}
				currentFrame.cantAirdash = player.airborne && player.wasCantAirdash && !player.inHitstun;
				currentFrame.airthrowDisabled = player.airborne && player.pawn.airthrowDisabled();
				currentFrame.running = player.pawn.running()
					|| player.cmnActIndex == CmnActFDash
					&& (
						player.pawn.isDisableThrow()  // fix for Johnny's dash
						|| player.charType == CHARACTER_TYPE_LEO  // Leo's is a special move with whiff cancels, but still we must make it very clear he cannot throw
						// For Bedman and Slayer it's kind of obvious they can't do anything at all during their dash, not just throw
					);
				currentFrame.cantBackdash = player.wasCantBackdashTimer != 0;
				if (player.x == player.prevPosX) {
					currentFrame.suddenlyTeleported = false;
				} else if (player.x > player.prevPosX) {
					currentFrame.suddenlyTeleported = player.x - player.prevPosX >= player.speedX + 122499;
				} else {
					currentFrame.suddenlyTeleported = player.prevPosX - player.x >= -player.speedX + 122499;
				}
				if (!currentFrame.suddenlyTeleported && player.y != player.prevPosY) {
					if (player.y > player.prevPosY) {
						currentFrame.suddenlyTeleported = player.y - player.prevPosY >= player.speedY + 122499;
					} else {
						currentFrame.suddenlyTeleported = player.prevPosY - player.y >= -player.speedY + 122499;
					}
				}
				
				currentFrame.counterhit = player.counterhit;
				currentFrame.crouching = player.crouching;
				
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
			
			if (player.charType == CHARACTER_TYPE_SIN && !player.sinHunger) {
				bool newVal = strcmp(player.pawn.gotoLabelRequests(), "Kuuhuku") == 0 && !player.pawn.isRCFrozen();
				if (newVal) {
					player.sinHunger = true;
					player.determineMoveNameAndSlangName(&currentFrame.animName);
					framebar.requestFirstFrame = true;
					currentFrame.isFirst = true;
				}
			}
			
			if (player.sinHunger) {
				currentFrame.type = FT_RECOVERY;
				++player.sinHungerRecovery;
			} else if (player.isInFDWithoutBlockstun) {
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
								&& strcmp(player.anim, "HammerFall") == 0) {
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
						// needed for whiff 5P to not show first recovery frame as cancellable (it is not)
						const FrameCancelInfoStored* cancelInfo = prevFrame.cancels.get();
						if (cancelInfo
								&& (
									!cancelInfo->gatlings.empty()
									|| !cancelInfo->whiffCancels.empty()
								)
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
				newCancelInfo.copyFromAnotherArray(player.wasCancels);
				newCancelInfo.enableSpecials = false;
				newCancelInfo.hitAlreadyHappened = hitAlreadyHappened;
				newCancelInfo.airborne = player.airborne;
				player.appendPlayerCancelInfo(newCancelInfo);
			}
			
			if (!(player.hitstop || superflashInstigator)) {
				if (measureInvuls) {
					if (!player.stoppedMeasuringInvuls) {
						++player.totalForInvul;
						int prevTotal = player.prevStartups.total() + player.totalForInvul;
						
						#define INVUL_TYPES_EXEC(enumName, stringDesc, fieldName) player.fieldName.addInvulFrame(prevTotal);
						INVUL_TYPES_TABLE
						#undef INVUL_TYPES_EXEC
					}
					
				} else {
					player.stoppedMeasuringInvuls = true;
				}
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
			
			PlayerFramebars& entityFramebar = playerFramebars[player.index];
			PlayerFramebar& framebar = entityFramebar.idleHitstop;
			PlayerFrame& currentFrame = framebar[framebarPos];
			
			int value;
			
			value = player.printStartupForFramebar();
			currentFrame.startup = value > SHRT_MAX ? SHRT_MAX : value;
			
			value = player.activesDisp.total();
			currentFrame.active = value > SHRT_MAX ? SHRT_MAX : value;
			
			value = player.printRecoveryForFramebar();
			currentFrame.recovery = value > SHRT_MAX ? SHRT_MAX : value;
			
			FrameAdvantageForFramebarResult advRes;
			player.calcFrameAdvantageForFramebar(&advRes);
			currentFrame.frameAdvantage = advRes.frameAdvantage;
			currentFrame.landingFrameAdvantage = advRes.landingFrameAdvantage;
			currentFrame.frameAdvantageNoPreBlockstun = advRes.frameAdvantageNoPreBlockstun;
			currentFrame.landingFrameAdvantageNoPreBlockstun = advRes.landingFrameAdvantageNoPreBlockstun;
			
			value = player.totalDisp + player.sinHungerRecovery;
			currentFrame.total = value > SHRT_MAX ? SHRT_MAX : value;
			
			copyIdleHitstopFrameToTheRestOfSubframebars(entityFramebar,
				framebarAdvanced,
				framebarAdvancedIdle,
				framebarAdvancedHitstop,
				framebarAdvancedIdleHitstop);
			
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
			} else if (player.rejection) {
				player.xStunDisplay = PlayerInfo::XSTUN_DISPLAY_REJECTION_WITH_SLOW;
			} else if (player.cmnActIndex == CmnActWallHaritsukiLand) {
				player.xStunDisplay = PlayerInfo::XSTUN_DISPLAY_WALLSLUMP_LAND;
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
			player.elpheltPrevFrameWasPlayerval1 = player.elpheltWasPlayerval1;
			player.prevFrameElpheltRifle_AimMem46 = player.elpheltRifle_AimMem46;
			player.prevFrameRomanCancelAvailability = player.pawn.romanCancelAvailability();
			player.prevBbscrvar5 = player.pawn.bbscrvar5();
			for (int k = 0; k < 4; ++k) {
				player.prevFrameResource[k] = player.pawn.exGaugeValue(k);
			}
			player.prevPosX = player.x;
			player.prevPosY = player.y;
			player.prevFrameCancels = player.wasCancels;
			player.prevFramePreviousEntityLinkObjectDestroyOnStateChangeWasEqualToPlayer = player.pawn.previousEntity()
				&& player.pawn.previousEntity().linkObjectDestroyOnStateChange() == player.pawn;
			
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
	bool eachProjectileOnSeparateFramebarChanged = false;
	#define trackSetting(name) \
		if (name != settings.name) { \
			name = settings.name; \
			combinedFramebarsSettingsChanged = true; \
		}
	trackSetting(combineProjectileFramebarsWhenPossible)
	if (eachProjectileOnSeparateFramebar != settings.eachProjectileOnSeparateFramebar) {
		eachProjectileOnSeparateFramebar = settings.eachProjectileOnSeparateFramebar;
		combinedFramebarsSettingsChanged = true;
		eachProjectileOnSeparateFramebarChanged = true;
	}
	trackSetting(condenseIntoOneProjectileFramebar)
	trackSetting(neverIgnoreHitstop)
	#undef trackSetting
	
	
	int newFramesCount = settings.framebarDisplayedFramesCount;
	int newStoredFramesCount = settings.framebarStoredFramesCount;
	if (newStoredFramesCount < 1) {
		newStoredFramesCount = 1;
	}
	if (newStoredFramesCount > _countof(Framebar::frames)) {
		newStoredFramesCount = _countof(Framebar::frames);
	}
	if (newFramesCount > newStoredFramesCount) {
		newFramesCount = newStoredFramesCount;
	}
	if (newFramesCount < 1) {
		newFramesCount = 1;
	}
	
	if (newFramesCount != framesCount) {
		framesCount = newFramesCount;
		combinedFramebarsSettingsChanged = true;
	}
	
	if (newStoredFramesCount != storedFramesCount) {
		storedFramesCount = newStoredFramesCount;
		combinedFramebarsSettingsChanged = true;
	}
	
	int framebarTotalFramesUnlimitedUse = neverIgnoreHitstop
		? endScene.getTotalFramesHitstopUnlimited()
		: endScene.getTotalFramesUnlimited();
	
	// Capped between 0 and framebarSettings.storedFramesCount, inclusive
	int framebarTotalFramesCapped;
	if (framebarTotalFramesUnlimitedUse > storedFramesCount) {
		framebarTotalFramesCapped = storedFramesCount;
	} else {
		framebarTotalFramesCapped = framebarTotalFramesUnlimitedUse;
	}
	
	bool newFramebarAutoScroll = ui.getFramebarAutoScroll();
	if (newFramebarAutoScroll != framebarAutoScroll) {
		framebarAutoScroll = newFramebarAutoScroll;
		combinedFramebarsSettingsChanged = true;
	}
	
	int newScrollXInFrames;
	if (framebarTotalFramesCapped > framesCount) {
		
		int totalScrollableFrames = framebarTotalFramesCapped  // total number of frames
			- framesCount;  // number of visible frames
		
		if (framebarAutoScroll) {
			newScrollXInFrames = 0;
		} else {
			newScrollXInFrames = std::lroundf(
				(float)(totalScrollableFrames + 1)  // adding one to give an even chance to frames that are on the edges
					* ui.getFramebarScrollX() / ui.getFramebarMaxScrollX()  // scroll ratio: from 0.F to 1.F
				- 0.5F
			);
			if (newScrollXInFrames < 0) {
				newScrollXInFrames = 0;
			}
			if (newScrollXInFrames > totalScrollableFrames) {
				newScrollXInFrames = totalScrollableFrames;
			}
		}
		
	} else {
		newScrollXInFrames = 0;
	}
	
	if (newScrollXInFrames != scrollXInFrames) {
		scrollXInFrames = newScrollXInFrames;
		combinedFramebarsSettingsChanged = true;
	}
	
	
	// Let UI know which settings we actually used, because UI may change them before drawing the framebar
	ui.framebarSettings.neverIgnoreHitstop = settings.neverIgnoreHitstop;
	ui.framebarSettings.eachProjectileOnSeparateFramebar = settings.eachProjectileOnSeparateFramebar;
	ui.framebarSettings.condenseIntoOneProjectileFramebar = settings.condenseIntoOneProjectileFramebar;
	ui.framebarSettings.framesCount = framesCount;
	ui.framebarSettings.storedFramesCount = storedFramesCount;
	ui.framebarSettings.scrollXInFrames = scrollXInFrames;
	
	if ((combinedFramebarsSettingsChanged || frameHasChanged) && !iGiveUp) {
		
		const bool recheckCompletelyEmpty = framesCount != _countof(Framebar::frames);
		int framebarPositionUse;
		if (neverIgnoreHitstop) {
			framebarPositionUse = framebarPositionHitstop;
		} else {
			framebarPositionUse = framebarPosition;
		}
		framebarPositionUse -= scrollXInFrames;
		if (framebarPositionUse < 0) {
			framebarPositionUse += _countof(Framebar::frames);
		}
		
		combinedFramebars.clear();
		if (!eachProjectileOnSeparateFramebar) {
			combinedFramebars.reserve(projectileFramebars.size());
			const bool combinedFramebarMustIncludeHitstop = neverIgnoreHitstop;
			for (ProjectileFramebar& source : projectileFramebars) {
				Framebar& from = combinedFramebarMustIncludeHitstop ? source.hitstop : source.main;
				if (!(from.completelyEmpty || recheckCompletelyEmpty && from.lastNFramesCompletelyEmpty(framebarPositionUse, framesCount))) {
					CombinedProjectileFramebar& entityFramebar = findCombinedFramebar(source, combinedFramebarMustIncludeHitstop);
					entityFramebar.combineFramebar(framebarPositionUse, from, &source);
				}
			}
			for (CombinedProjectileFramebar& entityFramebar : combinedFramebars) {
				entityFramebar.determineName(framebarPositionUse, combinedFramebarMustIncludeHitstop);
			}
		} else if (eachProjectileOnSeparateFramebarChanged) {
			for (ProjectileFramebar& source : projectileFramebars) {
				for (Frame& frame : source.hitstop.frames) {
					frame.next = nullptr;
				}
				for (Frame& frame : source.main.frames) {
					frame.next = nullptr;
				}
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
	if (keyboard.gotPressed(settings.disableModToggle)) {
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
	if (!gifMode.modDisabled && (keyboard.gotPressed(settings.toggleAllowCreateParticles))) {
		if (gifMode.allowCreateParticles == true) {
			gifMode.allowCreateParticles = false;
			logwrap(fputs("Allow create particles = false\n", logfile));
		} else {
			gifMode.allowCreateParticles = true;
			logwrap(fputs("Allow create particles = true\n", logfile));
		}
	}
	if (!gifMode.modDisabled && (keyboard.gotPressed(settings.clearInputHistory))) {
		logwrap(fputs("Clear input history\n", logfile));
		game.clearInputHistory();
		clearInputHistory();
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
				players[i].tensionGainLastCombo = 0;
			}
			if (ui.clearBurstGainMaxCombo[i]) {
				ui.clearBurstGainMaxCombo[i] = false;
				players[i].burstGainMaxCombo = 0;
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
			return 0;
		}
		switch (message) {
			case WM_KEYDOWN: {
				if (wParam == 13) {
					bool isExtended = (lParam & 0x1000000) != 0;
					if ((isExtended && settings.ignoreNumpadEnterKey
							|| !isExtended && settings.ignoreRegularEnterKey)
							&& shouldIgnoreEnterKey()) {
						return 0;  // 'Application should return 0 if it processes this message' - Microsoft
					}
				}
			}
			break;
			case WM_DESTROY: {
				keyboard.thisProcessWindow = NULL;
			}
			break;
			case WM_APP_SETTINGS_FILE_UPDATED: {
				settings.readSettings(false);
			}
			break;
			case WM_APP_UI_STATE_CHANGED: {
				if (wParam && ui.stateChanged) {
					processKeyStrokes();
				}
			}
			break;
			case WM_APP_TURN_OFF_POST_EFFECT_SETTING_CHANGED: {
				onGifModeBlackBackgroundChanged();
			}
			break;
			case WM_APP_HIDE_RANK_ICONS: {
				game.hideRankIcons();
			}
			break;
			case WM_APP_USE_POSITION_RESET_MOD_CHANGED: {
				game.onUsePositionResetChanged();
			}
			break;
			case WM_APP_PLAYER_IS_BOSS_CHANGED: {
				onPlayerIsBossChanged();
			}
			break;
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
	
	// do this even if 'give up'
	for (int i = 0; i < 2; ++i) {
		players[i].clear();
		reachedMaxStun[i] = -1;
	}
	measuringFrameAdvantage = false;
	measuringLandingFrameAdvantage = -1;
	memset(tensionGainOnLastHit, 0, sizeof tensionGainOnLastHit);
	memset(burstGainOnLastHit, 0, sizeof burstGainOnLastHit);
	memset(tensionGainOnLastHitUpdated, 0, sizeof tensionGainOnLastHitUpdated);
	memset(burstGainOnLastHitUpdated, 0, sizeof burstGainOnLastHitUpdated);
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
	bdashMoveIndex = -1;
	fdashMoveIndex = -1;
	maximumBurstMoveIndex = -1;
	cmnFAirDashMoveIndex = -1;
	cmnBAirDashMoveIndex = -1;
	fdStandMoveIndex = -1;
	fdCrouchMoveIndex = -1;
	fdAirMoveIndex = -1;
	rcMoveIndex = -1;
	clearInputHistory(true);
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
void EndScene::onHitDetectionStart(HitDetectionType hitDetectionType) {
	currentHitDetectionType = hitDetectionType;
	if (hitDetectionType == HIT_DETECTION_EASY_CLASH) {
		entityList.populate();
		if (!iGiveUp) {
			for (int i = 0; i < 2; ++i) {
				Entity ent = entityList.slots[i];
				tensionRecordedHit[i] = ent.tension();
				burstRecordedHit[i] = game.getBurst(i);
				tensionGainOnLastHit[i] = 0;
				burstGainOnLastHit[i] = 0;
				tensionGainOnLastHitUpdated[i] = false;
				burstGainOnLastHitUpdated[i] = false;
				reachedMaxStun[i] = -1;
				
				// moved this here from handleUponHook, signal 0x27 (PRE_DRAW), because Jam would not show super armor on the frame she parries.
				// Also the Blitz Shield reject would not show super armor on the frame the attacker's hit connects.
				PlayerInfo& player = findPlayer(ent);
				player.wasSuperArmorEnabled = ent.superArmorEnabled();
				player.wasFullInvul = ent.fullInvul();
			}
		}
		registeredHits.clear();
		
		attackHitboxes.clear();
	}
}

// We use this hook at the end of hit detection algorithm to measure some values after a hit.
// Runs on the main thread
void EndScene::onHitDetectionEnd(HitDetectionType hitDetectionType) {
	
	for (AttackHitbox& attackHitbox : attackHitboxes) {
		if (attackHitbox.clash && !attackHitbox.notClash && attackHitbox.hitbox.thickness > 1) {
			attackHitbox.hitbox.thickness = 1;
		}
	}
	
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
	if (hitDetectionType == HIT_DETECTION_CLASH) {
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
	if (hitResult == HIT_RESULT_ARMORED && attacker.isPawn() && defender.isPawn()
			&& defender.characterType() == CHARACTER_TYPE_LEO
			&& strcmp(defender.animationName(), "Semuke5E") == 0) {
		LeoParry newParry;
		newParry.x = defender.x();
		newParry.y = defender.y();
		newParry.timer = 0;
		newParry.aswEngTick = getAswEngineTick();
		leoParries.push_back(newParry);
	}
	if (!iGiveUp) {
		if (!attacker.isPawn()) {
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
	requestedInputHistoryDraw = false;
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
void EndScene::HookHelp::BBScr_createObjectWithArgHook(const char* animName, BBScrPosType posType) {
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
void EndScene::HookHelp::BBScr_createObjectHook(const char* animName, BBScrPosType posType) {
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
ProjectileInfo& EndScene::onObjectCreated(Entity pawn, Entity createdPawn, const char* animName, bool fillName) {
	for (auto it = projectiles.begin(); it != projectiles.end(); ++it) {
		if (it->ptr == createdPawn) {
			if (it->landedHit || it->gotHitOnThisFrame || objHasAttackHitboxes(it->ptr)) {
				it->ptr = nullptr;
			} else {
				projectiles.erase(it);
			}
			break;
		}
	}
	projectiles.emplace_back();
	ProjectileInfo& projectile = projectiles.back();
	projectile.fill(createdPawn, getSuperflashInstigator(), true, fillName);
	bool ownerFound = false;
	ProjectileInfo& creatorProjectile = findProjectile(pawn);
	if (creatorProjectile.ptr) {
		projectile.creationTime_aswEngineTick = creatorProjectile.creationTime_aswEngineTick;
		projectile.startup = creatorProjectile.total;
		memcpy(projectile.creatorName, creatorProjectile.ptr.animationName(), 32);
		creatorProjectile.determineCreatedName(&projectile.creatorNamePtr, true, true);
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
		projectile.creatorNamePtr.clear();
		projectile.creator = player.pawn;
	}
	return projectile;
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
void EndScene::registerJump(PlayerInfo& player, Entity pawn, const char* animName) {
	// for combo recipe - register a jump cancel
	// even air jump cancel starts with prejump
	if (strcmp(animName, "CmnActJumpPre") == 0) {
		
		struct RemoveFlagsOnExit {
			~RemoveFlagsOnExit() {
				player.jumpInstalled = false;
				player.superJumpInstalled = false;
			}
			PlayerInfo& player;
			RemoveFlagsOnExit(PlayerInfo& player) : player(player) { }
		} flagRemover(player);
		
		bool inAir = !(pawn.posY() == 0 && !pawn.ascending());
		bool isSuperJump = false;
		bool enableGatlings = pawn.enableGatlings() && pawn.attackCollidedSoCanCancelNow();
		if (!inAir) {
			const AddedMoveData* move = pawn.currentMove();
			if (move && strcmp(move->name + 4, "HighJump") == 0) {
				isSuperJump = true;
			}
			if (pawn.enemyEntity() && pawn.enemyEntity().inHitstun() && isSuperJump && player.jumpInstalled) {
				ui.comboRecipeUpdatedOnThisFrame[player.index] = true;
				player.comboRecipe.emplace_back();
				ComboRecipeElement& newElem = player.comboRecipe.back();
				newElem.artificial = true;
				newElem.name = assignName("Jump Install");
				newElem.timestamp = getAswEngineTick();
			}
			bool canJump = pawn.enableJump() || pawn.enableAirOptions();
			if (!canJump) {
				AddedMoveData* foundMove = (AddedMoveData*)findMoveByName((void*)pawn.ent, "CmnVJump", 0);
				if (foundMove) {
					canJump = foundMove->whiffCancelOption()
						|| foundMove->gatlingOption() && enableGatlings;  // Jack-O 5H jump cancel
				}
			}
			if (canJump) {
				if (isSuperJump) {
					if (!pawn.enableSpecials()) {
						player.superJumpCancelled = true;
					} else {
						player.superJumpNonCancel = true;
					}
				} else {
					// correction for Jack-O 5H jump cancel
					if (!pawn.enableSpecials()) {
						player.jumpCancelled = true;
					} else {
						player.jumpNonCancel = true;
					}
				}
				return;
			}
		} else {
			bool canJump = pawn.enableAirOptions();
			if (!canJump) {
				AddedMoveData* foundMove = (AddedMoveData*)findMoveByName((void*)pawn.ent, "CmnVAirJump", 0);
				if (foundMove) {
					canJump = foundMove->whiffCancelOption()
						|| foundMove->gatlingOption() && enableGatlings;  // anything similar to Jack-O 5H jump cancel
				}
			}
			if (canJump) {
				player.doubleJumped = true;
				return;
			}
		}
		
		if (pawn.enableJumpCancel()) {
			if (isSuperJump) {
				player.superJumpCancelled = true;
			} else {
				player.jumpCancelled = true;
			}
		}
		
	} else if (pawn.enemyEntity() && pawn.enemyEntity().inHitstun()) {
		
		if (player.jumpInstalled) {
			player.jumpInstalledStage2 = true;
			player.jumpInstalled = false;
			player.superJumpInstalled = false;
		} else if (player.superJumpInstalled) {
			player.superJumpInstalledStage2 = true;
			player.superJumpInstalled = false;
		}
		
	}
}

// Runs on the main thread
void EndScene::registerRun(PlayerInfo& player, Entity pawn, const char* animName) {
	if (strcmp(animName, "CmnActFDash") == 0) {
		player.startedRunning = true;
	} else if (strcmp(animName, "CrouchFWalk") == 0 || strcmp(animName, "CmnActFWalk") == 0) {
		player.startedWalkingForward = true;
	} else if (strcmp(animName, "CrouchBWalk") == 0 || strcmp(animName, "CmnActBWalk") == 0) {
		player.startedWalkingBackward = true;
		
	// Bedman is idle during the entirety of 1/2/3/4/6/7/8/9Move, and it can be cancelled into a normal or special immediately.
	// We wouldn't even notice it if we don't register it here.
	// This is needed only for the Combo Recipe panel.
	} else if (pawn.characterType() == CHARACTER_TYPE_BEDMAN
						&& animName[0] >= '1'
						&& animName[0] <= '9'
						&& strcmp(animName + 1, "Move") == 0) {
		ui.comboRecipeUpdatedOnThisFrame[player.index] = true;
		player.comboRecipe.emplace_back();
		ComboRecipeElement& newComboElem = player.comboRecipe.back();
		PlayerInfo::determineMoveNameAndSlangName(pawn, &newComboElem.name);
		newComboElem.timestamp = getAswEngineTick();
		newComboElem.framebarId = -1;
		
		const GatlingOrWhiffCancelInfo* foundCancel = nullptr;
		if (player.wasCancels.hasCancel(pawn.currentMove()->name, &foundCancel)) {
			int delay = foundCancel->framesBeenAvailableForNotIncludingHitstopFreeze;
			if (delay > 0) {
				// we're decrementing, because this hook runs after collecting frame cancels, and the 1/2/3/4/6/7/8/9Move is already in there
				--delay;
				if (
					delay > 0
					&& !(
						delay == 1
						&& foundCancel->wasAddedDuringHitstopFreeze
					)
				) {
					newComboElem.cancelDelayedBy = delay;
				}
			}
		}
	}
}

// Runs on the main thread
// This hook does not work on projectiles
void EndScene::setAnimHook(Entity pawn, const char* animName) {
	if (!shutdown && pawn.isPawn() && !gifMode.modDisabled && !iGiveUp) {
		PlayerInfo& player = findPlayer(pawn);
		if (player.pawn) {  // The hook may run on match load before we initialize our pawns,
			                // because we only do our pawn initialization after the end of the logic tick
			player.lastIgnoredHitNum = -1;
			player.wasAirborneOnAnimChange = player.airborne_insideTick();
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
			
			registerJump(player, pawn, animName);
			registerRun(player, pawn, animName);
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
void EndScene::HookHelp::BBScr_createParticleWithArgHook(const char* animName, BBScrPosType posType) {
	endScene.BBScr_createParticleWithArgHook(Entity{(char*)this}, animName, posType);
}

// Runs on the main thread
void EndScene::HookHelp::BBScr_linkParticleHook(const char* name) {
	endScene.BBScr_linkParticleHook(Entity{(char*)this}, name);
}

// Runs on the main thread
void EndScene::HookHelp::BBScr_linkParticleWithArg2Hook(const char* name) {
	endScene.BBScr_linkParticleWithArg2Hook(Entity{(char*)this}, name);
}

// Runs on the main thread
void EndScene::BBScr_createParticleWithArgHook(Entity pawn, const char* animName, BBScrPosType posType) {
	if (!gifMode.modDisabled && !gifMode.allowCreateParticles) {
		return;
	}
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

// Runs on the main thread
void EndScene::BBScr_linkParticleHook(Entity pawn, const char* name) {
	if (!gifMode.modDisabled && !gifMode.allowCreateParticles) {
		return;
	}
	orig_BBScr_linkParticle((void*)pawn, name);
}

// Runs on the main thread
void EndScene::BBScr_linkParticleWithArg2Hook(Entity pawn, const char* name) {
	if (!gifMode.modDisabled && !gifMode.allowCreateParticles) {
		return;
	}
	orig_BBScr_linkParticleWithArg2((void*)pawn, name);
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
		player.elpheltFirstWasPlayerval1Measurement = true;
		player.isFirstCheckFirePerFrameUponsWrapperOfTheFrame = true;
		player.jumpNonCancel = false;
		player.superJumpNonCancel = false;
		player.jumpCancelled = false;
		player.superJumpCancelled = false;
		player.doubleJumped = false;
		player.startedRunning = false;
		player.startedWalkingForward = false;
		player.startedWalkingBackward = false;
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
		player.wasPrevFrameEnableNormals = player.wasEnableNormals;
		player.wasPrevFrameEnableWhiffCancels = player.wasEnableWhiffCancels;
		player.wasPrevFrameForceDisableFlags = player.wasForceDisableFlags;
		player.wasEnableNormals = false;
		player.wasCantAirdash = true;
		player.wasCanYrc = false;
		player.wasCantRc = false;
		player.wasProhibitFDTimer = 1;
		player.wasAirdashHorizontallingTimer = 0;
		player.wasEnableWhiffCancels = false;
		player.wasPrevFrameEnableSpecials = player.wasEnableSpecials;
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
		player.wasCancels.unsetWasFoundOnThisFrame(true);
		player.receivedNewDmgCalcOnThisFrame = false;
		player.blockedAHitOnThisFrame = false;
		player.lostSpeedYOnThisFrame = false;
		player.wasAirborneOnAnimChange = false;
		player.bedmanInfo.sealAReceivedSignal5 = false;
		player.bedmanInfo.sealBReceivedSignal5 = false;
		player.bedmanInfo.sealCReceivedSignal5 = false;
		player.bedmanInfo.sealDReceivedSignal5 = false;
	}
	creatingObject = false;
	events.clear();
}

// Runs on the main thread
void EndScene::HookHelp::pawnInitializeHook(void* initializationParams) {
	endScene.pawnInitializeHook(Entity{(char*)this}, initializationParams);
}

// Runs on the main thread
void EndScene::HookHelp::handleUponHook(BBScrEvent signal) {
	endScene.handleUponHook(Entity{(char*)this}, signal);
}

// Runs on the main thread
// This is also for effects
void EndScene::pawnInitializeHook(Entity createdObj, void* initializationParams) { 
	Entity newProjectilePtr { nullptr };
	if (!shutdown && creatingObject) {
		creatingObject = false;
		if (!iGiveUp) {
			ProjectileInfo& newProjectileInfo = onObjectCreated(creatorOfCreatedObject, createdObj, createdObjectAnim, false);
			newProjectilePtr = newProjectileInfo.ptr;
			events.emplace_back();
			OccuredEvent& event = events.back();
			event.type = OccuredEvent::SIGNAL;
			event.u.signal.from = creatorOfCreatedObject;
			event.u.signal.to = createdObj;
			memcpy(event.u.signal.fromAnim, creatorOfCreatedObject.animationName(), 32);
			if (creatorOfCreatedObject.isPawn()) {
				event.u.signal.creatorName.clear();
			} else {
				ProjectileInfo::determineCreatedName(nullptr, creatorOfCreatedObject, nullptr, &event.u.signal.creatorName, true, true);
			}
		}
	}
	BOOL* advanceFramePtr = (BOOL*)((BYTE*)initializationParams + 0x38);
	if (*advanceFramePtr != 0 && newProjectilePtr) {
		BOOL oldVal = *advanceFramePtr;
		*advanceFramePtr = FALSE;
		endScene.orig_pawnInitialize(createdObj.ent, initializationParams);
		ProjectileInfo& projectile = findProjectile(newProjectilePtr);
		if (projectile.ptr) {
			projectile.fill(createdObj, getSuperflashInstigator(), true);
		}
		*advanceFramePtr = oldVal;
		orig_checkFirePerFrameUponsWrapper((void*)createdObj.ent);
	} else {
		endScene.orig_pawnInitialize(createdObj.ent, initializationParams);
		if (newProjectilePtr) {
			ProjectileInfo& projectile = findProjectile(newProjectilePtr);
			if (projectile.ptr) {
				projectile.fill(createdObj, getSuperflashInstigator(), true);
			}
		}
	}
}

// Runs on the main thread
void EndScene::handleUponHook(Entity pawn, BBScrEvent signal) { 
	if (!shutdown && !iGiveUp) {
		if (!sendSignalStack.empty()) {
			events.emplace_back();
			OccuredEvent& event = events.back();
			event.type = OccuredEvent::SIGNAL;
			Entity signalSender = sendSignalStack.back();
			event.u.signal.from = signalSender;
			event.u.signal.to = pawn;
			memcpy(event.u.signal.fromAnim, signalSender.animationName(), 32);
			if (signalSender.isPawn() || eventHandlerSendsIntoRecovery(pawn, signal)) {
				event.u.signal.creatorName.clear();
			} else {
				ProjectileInfo::determineCreatedName(nullptr, signalSender, nullptr, &event.u.signal.creatorName, true, true);
			}
		}
		if (signal == BBSCREVENT_CUSTOM_SIGNAL_5
				&& !pawn.isPawn() && strncmp(pawn.animationName(), "DejavIcon", 9) == 0) {
			PlayerInfo& player = findPlayer(pawn.playerEntity());
			const char* remainingName = pawn.animationName() + 9;
			if (strcmp(remainingName, "BoomerangA") == 0) {
				player.bedmanInfo.sealAReceivedSignal5 = true;
			} else if (strcmp(remainingName, "BoomerangB") == 0) {
				player.bedmanInfo.sealBReceivedSignal5 = true;
			} else if (strcmp(remainingName, "SpiralBed") == 0) {
				player.bedmanInfo.sealCReceivedSignal5 = true;
			} else if (strcmp(remainingName, "FlyingBed") == 0) {
				player.bedmanInfo.sealDReceivedSignal5 = true;
			}
		}
	}
	if (!shutdown) {
		if (signal == BBSCREVENT_ANIMATION_FRAME_ADVANCED
				&& pawn.isPawn()) {
			// Slayer's buff in PLAYERVAL_1 is checked when initiating a move, but decremented after the fact, in a FRAME_STEP handler
			// we need the original value
			PlayerInfo& player = findPlayer(pawn);
			player.wasPlayerval1Idling = pawn.playerVal(1);
			player.wasResource = pawn.exGaugeValue(0);
		}
		// Blitz Shield rejection changes super armor enabled and full invul flags at the end of a logic tick
		if (signal == BBSCREVENT_PRE_DRAW
				&& pawn.isPawn()) {
			PlayerInfo& player = findPlayer(pawn);
			// moved collection of wasSuperArmorEnabled and wasFullInvul from here to onHitDetectionStart,
			// because the frame when Jam parries would be shown as not having super armor in the framebar
			
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
		player.lastHitComboTimer = pawn.comboTimer();
		entityManager.calculateHitstunProration(
			pawn.receivedAttack()->noHitstunScaling(),
			isAirHit,
			player.lastHitComboTimer,
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
void EndScene::HookHelp::BBScr_runOnObjectHook(EntityReferenceType entityReference) {
	endScene.BBScr_runOnObjectHook(Entity{(char*)this}, entityReference);
}

// Runs on the main thread
void EndScene::BBScr_runOnObjectHook(Entity pawn, EntityReferenceType entityReference) {
	orig_BBScr_runOnObject((void*)pawn.ent, entityReference);
	if (!shutdown && pawn.isPawn() && !iGiveUp) {
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
					event.u.signal.creatorName.clear();
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
	if (!endScene.needDrawInputs && !endScene.requestedInputHistoryDraw) {
		for (int i = 0; i < 2; ++i) {
			drawData.inputsSize[i] = 0;
		}
	}
	camera.valuesPrepare.copyTo(cameraValues);
	noNeedToDrawPoints = endScene.willEnqueueAndDrawOriginPoints;  // drawing points may also draw inputs
	pauseMenuOpen = endScene.pauseMenuOpen;
	dontShowBoxes = settings.dontShowBoxes;
	inputHistoryIsSplitOut = endScene.requestedInputHistoryDraw;
	iconsUTexture2D = endScene.getIconsUTexture2D();
	endScene.fillInFontInfo(&staticFontTexture2D,
		&openParenthesis,
		&closeParenthesis,
		digit);
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

void DrawInputHistoryRenderCommand::Destructor(BOOL freeMem) noexcept {
	DrawInputHistoryRenderCommand::~DrawInputHistoryRenderCommand();
	FRenderCommand::Destructor(freeMem);
}
// Runs on the graphics thread
unsigned int DrawInputHistoryRenderCommand::Execute() {
	endScene.executeDrawInputHistoryRenderCommand(this);
	return sizeof(*this);
}
const wchar_t* DrawInputHistoryRenderCommand::DescribeCommand() noexcept {
	return L"DrawInputHistoryRenderCommand";
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
	inputHistoryIsSplitOut = endScene.requestedInputHistoryDraw;
	if (calledFromDrawOriginPointsRenderCommand && !endScene.needEnqueueUiWithPoints) return;
	iconsUTexture2D = endScene.getIconsUTexture2D();
	endScene.fillInFontInfo(&staticFontTexture2D,
		&openParenthesis,
		&closeParenthesis,
		digit);
	drawingPostponed = endScene.drawingPostponed();
	obsStoppedCapturing = endScene.obsStoppedCapturing;
	if (endScene.queueingFramebarDrawCommand && endScene.uiWillBeDrawnOnTopOfPauseMenu) {
		ui.getFramebarDrawData(drawData);
	} else {
		ui.copyDrawDataTo(drawData);
	}
	needUpdateFramebarTexture = false;
	if (endScene.needUpdateGraphicsFramebarTexture) {
		endScene.needUpdateGraphicsFramebarTexture = false;
		const PngResource* uiFramebarTexture = nullptr;
		const PackTextureSizes* uiSizes = nullptr;
		bool isColorblind = false;
		ui.getFramebarTexture(&uiFramebarTexture, &uiSizes, &isColorblind);
		if (uiFramebarTexture && uiSizes) {
			needUpdateFramebarTexture = true;
			framebarTexture = *uiFramebarTexture;
			framebarSizes = *uiSizes;
			framebarColorblind = isColorblind;
		}
	}
}

// Runs on the main thread
void EndScene::REDAnywhereDispDrawHook(void* canvas, FVector2D* screenSize) {
	// added gameDataPtr not null check, because was crashing on queueOriginPointDrawingDummyCommandAndInitializeIcon call (more specifically, REDGameCommon::GetStaticFont)
	// To reproduce the crash, remove the check and edit BootGGXrd.bat to add the following lines at the end:
	// GGXrdDisplayPingInjector -force
	// ggxrd_hitbox_injector -force
	// (GGXrdDisplayPingInjector mod is present) (is it really needed is not certain)
	// Then boot the game using Steam
	if (!shutdown && !graphics.shutdown && *game.gameDataPtr) {
		processKeyStrokes();
	}
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
	if (!shutdown && !graphics.shutdown && *game.gameDataPtr) {
		game.updateOnlineDelay();
		
		drawDataPrepared.gameModeFast = getGameModeFast();
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
			ui.needUpdateGraphicsFramebarTexture = false;
			ui.prepareDrawData();
			if (ui.needUpdateGraphicsFramebarTexture) {
				ui.needUpdateGraphicsFramebarTexture = false;
				needUpdateGraphicsFramebarTexture = true;
			}
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
							&& !requestedInputHistoryDraw
						) && !drawingPostponedLocal
				) {
					needEnqueueOriginPoints = true;
				}
				Entity instigator = getSuperflashInstigator();
				if (instigator != nullptr) {
					if (
						instigator.characterType() == CHARACTER_TYPE_RAMLETHAL
						&& strcmp(instigator.animationName(), "BitLaser") == 0
						&& instigator.currentAnimDuration() >= 8 && instigator.currentAnimDuration() < 139
						|| instigator.characterType() == CHARACTER_TYPE_INO
						&& strcmp(instigator.animationName(), "Madogiwa") == 0  // 632146H super
						&& instigator.currentAnimDuration() >= 8 && instigator.currentAnimDuration() < 64
						|| instigator.characterType() == CHARACTER_TYPE_SLAYER
						&& strcmp(instigator.animationName(), "DeadOnTime") == 0
						&& instigator.currentAnimDuration() >= 8 && instigator.currentAnimDuration() < 20
						|| instigator.characterType() == CHARACTER_TYPE_HAEHYUN
						&& strcmp(instigator.animationName(), "BlackHoleAttack") == 0  // 236236H super
						&& instigator.currentAnimDuration() >= 7 && instigator.currentAnimDuration() < 100
						|| instigator.characterType() == CHARACTER_TYPE_ANSWER
						&& strcmp(instigator.animationName(), "Royal_Straight_Flush") == 0  // 632146S super
						&& instigator.currentAnimDuration() >= 19 && instigator.currentAnimDuration() < 143
					) {
						drawDataPrepared.clearBoxes();
					}
				} else {
					for (int i = 0; i < 2 && i < entityList.count; ++i) {
						instigator = entityList.slots[i];
						if (instigator.characterType() == CHARACTER_TYPE_RAVEN
								&& strcmp(instigator.animationName(), "RevengeAttackEx") == 0  // 214214H super
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
	orig_REDAnywhereDispDraw(canvas, screenSize);  // calls asmhooks
	queueingFramebarDrawCommand = false;
	
	if (!shutdown && !graphics.shutdown && *game.gameDataPtr
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
	graphics.staticFontTexture = getTextureFromUTexture2D(command->staticFontTexture2D);
	graphics.staticFontOpenParenthesis = command->openParenthesis;
	graphics.staticFontCloseParenthesis = command->closeParenthesis;
	memcpy(graphics.staticFontDigit, command->digit, sizeof graphics.staticFontDigit);
	graphics.endSceneIsAwareOfDrawingPostponement = command->drawingPostponed;
	graphics.obsStoppedCapturingFromEndScenesPerspective = command->obsStoppedCapturing;
	graphics.inputHistoryIsSplitOut = command->inputHistoryIsSplitOut;
	if (command->drawingPostponed) return;
	if (graphics.drawingPostponed()) return;
	graphics.noNeedToDrawPoints = command->noNeedToDrawPoints;  // drawing points may also draw inputs
	graphics.executeBoxesRenderingCommand(getDevice());
	graphics.noNeedToDrawPoints = false;
}

// Runs on the graphics thread
void EndScene::executeDrawOriginPointsRenderCommand(DrawOriginPointsRenderCommand* command) {
	if (endScene.shutdown || graphics.shutdown) return;
	
	command->uiOrFramebarDrawData.applyFramebarTexture();
	
	bool hasFramebarDrawData = !command->uiOrFramebarDrawData.drawData.empty()
		&& !command->uiOrFramebarDrawData.drawingPostponed
		&& !graphics.drawingPostponed();
	
	if (settings.dontShowBoxes
			&& !(
				(graphics.drawDataUse.inputsSize[0] || graphics.drawDataUse.inputsSize[1])
				&& !command->uiOrFramebarDrawData.inputHistoryIsSplitOut
			)
			&& !hasFramebarDrawData) return;
	
	if (hasFramebarDrawData) {
		graphics.endSceneIsAwareOfDrawingPostponement = command->uiOrFramebarDrawData.drawingPostponed;
		graphics.obsStoppedCapturingFromEndScenesPerspective = command->uiOrFramebarDrawData.obsStoppedCapturing;
		graphics.uiFramebarDrawData = command->uiOrFramebarDrawData.drawData;
		graphics.uiTexture = getTextureFromUTexture2D(command->uiOrFramebarDrawData.iconsUTexture2D);
		graphics.staticFontTexture = getTextureFromUTexture2D(command->uiOrFramebarDrawData.staticFontTexture2D);
		graphics.staticFontOpenParenthesis = command->uiOrFramebarDrawData.openParenthesis;
		graphics.staticFontCloseParenthesis = command->uiOrFramebarDrawData.closeParenthesis;
		memcpy(graphics.staticFontDigit, command->uiOrFramebarDrawData.digit, sizeof graphics.staticFontDigit);
		graphics.needDrawFramebarWithPoints = true;
	}
	
	graphics.inputHistoryIsSplitOut = command->uiOrFramebarDrawData.inputHistoryIsSplitOut;
	graphics.onlyDrawPoints = true;  // drawing points may also draw inputs
	graphics.drawAllFromOutside(getDevice());
	graphics.onlyDrawPoints = false;  // drawing points may also draw inputs
	graphics.needDrawFramebarWithPoints = false;
}

// Runs on the graphics thread
void EndScene::executeDrawInputHistoryRenderCommand(DrawInputHistoryRenderCommand* command) {
	if (endScene.shutdown || graphics.shutdown) return;
	
	graphics.onlyDrawInputHistory = true;
	graphics.drawAllFromOutside(getDevice());
	graphics.onlyDrawInputHistory = false;
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
	if (item->count == 4 && (unsigned int&)item->vertices[0].x == getDummyCmdUInt(dummyInputHistory)) {  // avoid floating point comparison as it may be slower
		FCanvas_Flush(canvas, 0);
		enqueueRenderCommand<DrawInputHistoryRenderCommand>();
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
	
	command->uiOrFramebarDrawData.applyFramebarTexture();
	
	if (!graphics.canDrawOnThisFrame()) return;
	
	IDirect3DTexture9* tex = getTextureFromUTexture2D(command->uiOrFramebarDrawData.iconsUTexture2D);
	graphics.endSceneIsAwareOfDrawingPostponement = command->uiOrFramebarDrawData.drawingPostponed;
	graphics.obsStoppedCapturingFromEndScenesPerspective = command->uiOrFramebarDrawData.obsStoppedCapturing;
	if (command->uiOrFramebarDrawData.drawingPostponed) {
		graphics.uiTexture = tex;
		graphics.staticFontTexture = getTextureFromUTexture2D(command->uiOrFramebarDrawData.staticFontTexture2D);
		graphics.staticFontOpenParenthesis = command->uiOrFramebarDrawData.openParenthesis;
		graphics.staticFontCloseParenthesis = command->uiOrFramebarDrawData.closeParenthesis;
		memcpy(graphics.staticFontDigit, command->uiOrFramebarDrawData.digit, sizeof graphics.staticFontDigit);
		graphics.uiDrawData = std::move(command->uiOrFramebarDrawData.drawData);
		graphics.inputHistoryIsSplitOut = command->uiOrFramebarDrawData.inputHistoryIsSplitOut;
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
	graphics.heartbeat(getDevice());
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

BYTE* EndScene::getUTexture2DFromFont(BYTE* font) {
	if (*(int*)(font + 0x48 + 4) >= 1) {
		return **(BYTE***)(font + 0x48);  // element 0
	}
	return nullptr;
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

void EndScene::HookHelp::BBScr_sendSignalHook(EntityReferenceType referenceType, BBScrEvent signal) {
	endScene.BBScr_sendSignalHook(Entity{(char*)this}, referenceType, signal);
}

void EndScene::HookHelp::BBScr_sendSignalToActionHook(const char* searchAnim, BBScrEvent signal) {
	endScene.BBScr_sendSignalToActionHook(Entity{(char*)this}, searchAnim, signal);
}

void EndScene::BBScr_sendSignalHook(Entity pawn, EntityReferenceType referenceType, BBScrEvent signal) {
	if (!iGiveUp) {
		Entity referredEntity = getReferredEntity((void*)pawn.ent, referenceType);
		
		bool isDizzyBubblePopping = referredEntity == pawn && signal == BBSCREVENT_CUSTOM_SIGNAL_1
				&& isDizzyBubble(pawn.animationName());
		
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
				if (pawn.isPawn() || eventHandlerSendsIntoRecovery(projectile.ptr, signal)) {
					event.u.signal.creatorName.clear();
				} else {
					ProjectileInfo::determineCreatedName(nullptr, pawn, nullptr, &event.u.signal.creatorName, true, true);
				}
			}
		}
	}
	orig_BBScr_sendSignal((void*)pawn, referenceType, signal);
}

void EndScene::BBScr_sendSignalToActionHook(Entity pawn, const char* searchAnim, BBScrEvent signal) {
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
			if (player.elpheltFirstWasPlayerval1Measurement) {
				player.elpheltFirstWasPlayerval1Measurement = false;
				player.elpheltWasPlayerval1 = pawn.playerVal(1);
			}
			player.wasPlayerval[0] = pawn.playerVal(0);
			player.wasPlayerval[1] = pawn.playerVal(1);
			player.wasPlayerval[2] = pawn.playerVal(2);
			player.wasPlayerval[3] = pawn.playerVal(3);
			player.wasEnableNormals = pawn.isRCFrozen() ? player.wasPrevFrameEnableNormals : pawn.enableNormals();
			int speedY = pawn.speedY();
			int posY = pawn.posY();
			if (player.charType == CHARACTER_TYPE_BEDMAN) {
				bool doubleJumpHeightOk = speedY <= 0 || posY >= 122500;
				// AirStopCancelOnly does not have a minimum height requirement
				if (doubleJumpHeightOk && !player.wasCancels.gatlings.hasCancel("AirStopCancelOnly")) {
					const AddedMoveData* airStop = pawn.findAddedMove("AirStop");
					if (airStop && airStop->minimumHeightRequirement) {
						doubleJumpHeightOk = posY >= airStop->minimumHeightRequirement;
					}
				}
				player.wasCantAirdash = pawn.remainingAirDashes() && !doubleJumpHeightOk;
			} else {
				int airDashMinimumHeight = pawn.airDashMinimumHeight();
				bool airdashHeightOk;
				if (speedY > 0) {
					airdashHeightOk = posY > airDashMinimumHeight;
				} else {
					airdashHeightOk = posY > 70000 && posY > -speedY;
				}
				player.wasCantAirdash = pawn.remainingAirDashes() && !airdashHeightOk;
			}
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
			player.wasCantRc = player.pawn.romanCancelAvailability() == ROMAN_CANCEL_DISALLOWED_ON_WHIFF_WITH_X_MARK;
			player.wasProhibitFDTimer = min(127, pawn.prohibitFDTimer());
			player.wasAirdashHorizontallingTimer = min(127, pawn.airdashHorizontallingTimer());
			player.wasEnableGatlings = player.wasEnableGatlings && pawn.currentAnimDuration() != 1 || pawn.enableGatlings();
			player.wasEnableWhiffCancels = pawn.isRCFrozen()
				? player.wasPrevFrameEnableWhiffCancels
				: (
					player.wasEnableWhiffCancels && pawn.currentAnimDuration() != 1 || pawn.enableWhiffCancels()
				);
			player.wasEnableSpecials = pawn.isRCFrozen()
				? player.wasPrevFrameEnableSpecials
				: player.wasEnableSpecials && pawn.currentAnimDuration() != 1 || pawn.enableSpecials();
			player.wasEnableSpecialCancel = player.wasEnableSpecialCancel && pawn.currentAnimDuration() != 1 || pawn.enableSpecialCancel();
			player.wasEnableJumpCancel = player.wasEnableJumpCancel && pawn.currentAnimDuration() != 1 || pawn.enableJumpCancel() && pawn.attackCollidedSoCanJumpCancelNow();
			player.wasAttackCollidedSoCanCancelNow = player.wasAttackCollidedSoCanCancelNow && pawn.currentAnimDuration() != 1 || pawn.attackCollidedSoCanCancelNow();
			player.wasEnableAirtech = player.wasEnableAirtech && pawn.currentAnimDuration() != 1 || pawn.enableAirtech();
			player.wasForceDisableFlags = pawn.isRCFrozen() ? player.wasPrevFrameForceDisableFlags : pawn.forceDisableFlags();
			player.obtainedForceDisableFlags = true;
			if (pawn.currentAnimDuration() == 1) {
				player.wasCancels.unsetWasFoundOnThisFrame(false);
			}
			bool isBlitzShieldCancels = blitzShieldCancellable(player, true);
			collectFrameCancels(player, player.wasCancels, player.wasInHitstopFreezeDuringSkillCheck, isBlitzShieldCancels);
			if (isBlitzShieldCancels) {
				player.wasEnableGatlings = true;
				player.wasEnableSpecialCancel = true;
				player.wasEnableJumpCancel = true;
				player.wasAttackCollidedSoCanCancelNow = true;
			}
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
	const NamePair* name = nullptr;
	const NamePair* nameUncombined = nullptr;
	const char* nameFull = nullptr;
	bool dontReplaceTitle;
	
	if (projectile.moveNonEmpty) {
		
		const NamePair* framebarName;
		if (projectile.ptr) {
			framebarName = projectile.move.getFramebarName(projectile.ptr);
			if (!framebarName) framebarName = projectile.move.displayName;
		} else {
			framebarName = projectile.lastName;
		}
		
		if (framebarName) {
			name = framebarName;
			dontReplaceTitle = false;
		} else {
			name = &PROJECTILES_NAMEPAIR;
			dontReplaceTitle = true;
		}
		
		if (projectile.ptr) {
			nameUncombined = projectile.move.getFramebarNameUncombined(projectile.ptr);
		} else {
			nameUncombined = projectile.move.framebarNameUncombined;
		}
		
		if (projectile.move.framebarNameFull) {
			nameFull = projectile.move.framebarNameFull;
		}
	} else {
		name = &PROJECTILES_NAMEPAIR;
		nameUncombined = nullptr;
		nameFull = nullptr;
		dontReplaceTitle = true;
	}
	
	bool hitConnectedNow = projectile.hitConnectedForFramebar();
	if (!(projectile.titleIsFromAFrameThatHitSomething && !hitConnectedNow)
			|| projectile.framebarTitle.text == nullptr
			|| projectile.framebarTitle.text->name[0] == '\0') {
		// We will dump this into a frame
		projectile.titleIsFromAFrameThatHitSomething = hitConnectedNow;
		projectile.framebarTitle.text = name;
		projectile.framebarTitle.uncombined = nameUncombined;
		projectile.framebarTitle.full = nameFull;
		projectile.dontReplaceFramebarTitle = dontReplaceTitle;
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
	bar.moveFramebarId = projectile.move.framebarId;
	return bar;
}

CombinedProjectileFramebar& EndScene::findCombinedFramebar(const ProjectileFramebar& source, bool hitstop) {
	int id = source.idForCombinedFramebar();
	const bool combineProjectileFramebarsWhenPossible = settings.combineProjectileFramebarsWhenPossible;
	const bool condenseIntoOneProjectileFramebar = settings.condenseIntoOneProjectileFramebar;
	if (condenseIntoOneProjectileFramebar) {
		for (CombinedProjectileFramebar& bar : combinedFramebars) {
			if (bar.playerIndex == source.playerIndex) return bar;
		}
	} else {
		for (CombinedProjectileFramebar& bar : combinedFramebars) {
			if (
					bar.playerIndex == source.playerIndex
					&& (
						bar.id == id
						|| combineProjectileFramebarsWhenPossible
						&& bar.canBeCombined(hitstop ? source.hitstop : source.main, id)
					)
			) {
				if (!(bar.moveFramebarId != -1 && source.moveFramebarId == -1)) {
					bar.moveFramebarId = source.moveFramebarId;
				}
				return bar;
			}
		}
	}
	combinedFramebars.emplace_back();
	CombinedProjectileFramebar& bar = combinedFramebars.back();
	bar.playerIndex = source.playerIndex;
	bar.id = id;
	bar.isEddie = source.isEddie;
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
				|| needEnqueueUiWithPoints
			)
			&& !gifMode.modDisabled
			&& !drawingPostponed();
}

void EndScene::collectFrameCancelsPart(PlayerInfo& player, FixedArrayOfGatlingOrWhiffCancelInfos<GatlingOrWhiffCancelInfo>& vec, const AddedMoveData* move,
		int iterationIndex, bool inHitstopFreeze, bool blitzShield) {
	int vecPos = -1;
	for (GatlingOrWhiffCancelInfo& existingElem : vec) {
		if (existingElem.move == move) {
			if (!existingElem.foundOnThisFrame) {
				if (!existingElem.countersIncremented) {
					existingElem.countersIncremented = true;
					if (!inHitstopFreeze) {
						if (existingElem.wasAddedDuringHitstopFreeze) {
							existingElem.wasAddedDuringHitstopFreeze = false;
						} else {
							++existingElem.framesBeenAvailableForNotIncludingHitstopFreeze;
						}
					}
					++existingElem.framesBeenAvailableFor;
				}
				existingElem.foundOnThisFrame = true;
			}
			return;
		}
		if (existingElem.iterationIndex < iterationIndex) {
			vecPos = &existingElem - vec.data();
		}
	}
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
	cancel.foundOnThisFrame = true;
	cancel.framesBeenAvailableFor = 1;
	cancel.framesBeenAvailableForNotIncludingHitstopFreeze = 1;
	cancel.wasAddedDuringHitstopFreeze = inHitstopFreeze;
	cancel.iterationIndex = iterationIndex;
	MoveInfo obtainedInfo;
	bool moveNonEmpty = moves.getInfo(obtainedInfo, player.charType, move->name, move->stateName, false);
	// name selectors don't work properly when the player is not in the state that the move corresponds to
	if (!moveNonEmpty || !obtainedInfo.getDisplayNameNoScripts(player)) {
		cancel.name = NamePairManager::getPair(move->name);
		int lenTest = strnlen(move->name, 32);
		if (lenTest >= 32) {
			int somethingBad = 1;
		}
		cancel.nameIncludesInputs = false;
	} else {
		cancel.name = obtainedInfo.getDisplayNameNoScripts(player);
		cancel.nameIncludesInputs = obtainedInfo.nameIncludesInputs;
	}
	cancel.move = move;
	cancel.replacementInputs = obtainedInfo.replacementInputs;
	if (obtainedInfo.replacementBufferTime) {
		cancel.bufferTime = obtainedInfo.replacementBufferTime;
	} else {
		int bufferTime = move->bufferTime;
		if (move->type == MOVE_TYPE_BACKWARD_SUPER_JUMP
				|| move->type == MOVE_TYPE_FORWARD_SUPER_JUMP
				|| move->type == MOVE_TYPE_NEUTRAL_SUPER_JUMP) {
			bufferTime += 2;
		}
		if ((player.pawn.clashOrRCBufferTimer() != 0
				|| player.pawn.dustHomingJump1BufferTimer() != 0
				|| player.pawn.blitzShieldHitstopBuffer() != 0 && !blitzShield)
				&& move->type != MOVE_TYPE_FORWARD_WALK
				&& move->type != MOVE_TYPE_BACKWARD_WALK) {
			// we can read bufferTimeFromRC here, because if clashOrRCBufferTimer is 0, then it has already been reset to 0 (we run this hook after the original)
			bufferTime += player.pawn.bufferTimeFromRC() + 13;
		}
		if (player.pawn.ensureAtLeast3fBufferForNormalsWhenJumping() != 0
				&& move->type == MOVE_TYPE_NORMAL
				&& move->characterState == MOVE_CHARACTER_STATE_JUMPING
				&& bufferTime < 3) {
			bufferTime = 3;
		}
		
		int minBufTime = getMinBufferTime(move->inputs);
		if (minBufTime != -1) {
			bufferTime += minBufTime;
		} else {
			bufferTime += 1;
		}
		
		cancel.bufferTime = bufferTime;
	}
}

void EndScene::collectFrameCancels(PlayerInfo& player, FrameCancelInfoFull& frame, bool inHitstopFreeze, bool isBlitzShieldCancels) {
	if (player.moveNonEmpty) frame.whiffCancelsNote = player.move.whiffCancelsNote;
	const AddedMoveData* base = player.pawn.movesBase();
	int* indices = player.pawn.moveIndices();
	bool isStylish = game.isStylish(player.pawn);
	
	if (isBlitzShieldCancels) {
		collectBlitzShieldCancels(player, frame, inHitstopFreeze, isStylish);
		return;
	}
	
	if (player.charType == CHARACTER_TYPE_BAIKEN
			&& player.pawn.blockstun()
			&& player.pawn.playerVal(0)) {
		collectBaikenBlockCancels(player, frame, inHitstopFreeze, isStylish);
		return;
	}
	
	bool enableGatlings = player.wasEnableGatlings && player.wasAttackCollidedSoCanCancelNow;
	bool enableWhiffCancels = player.wasEnableWhiffCancels;
	for (int i = player.pawn.moveIndicesCount() - 1; i >= 0; --i) {
		const AddedMoveData* move = base + indices[i];
		bool isGatling = move->gatlingOption() && enableGatlings;
		bool isWhiffCancel = move->whiffCancelOption() && enableWhiffCancels;
		if ((isGatling || isWhiffCancel) && checkMoveConditions(player, move) && (isStylish || !requiresStylishInputs(move))) {
			if (isGatling) {
				collectFrameCancelsPart(player, frame.gatlings, move, i, inHitstopFreeze);
			}
			if (isWhiffCancel) {
				collectFrameCancelsPart(player, frame.whiffCancels, move, i, inHitstopFreeze);
			}
		}
	}
	if (player.moveNonEmpty && enableWhiffCancels
			&& player.move.onlyAddForceWhiffCancelsOnFirstFrameOfSprite
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
			collectFrameCancelsPart(player, frame.whiffCancels, move, moveIndex, inHitstopFreeze);
		}
	}
}

bool EndScene::checkMoveConditions(PlayerInfo& player, const AddedMoveData* move) {
	if (move->subcategory == MOVE_SUBCATEGORY_ITEM_USE
			// I don't know yet how to detect game mode in online. MOM can be selected offline as well as online,
			// and so far we only know how to determine the offline MOM mode.
			// For now we'll get rid of showing 'Item' as a gatling during dust combos in all modes.
			
			|| move->inputs[0] == INPUT_MOM_TAUNT
			
			// CmnStandForce has impossible inputs and comes up in the list of gatlings from 5D horizontal homing dash
			|| move->inputs[0] == INPUT_1
			&& move->inputs[1] == INPUT_2
			&& move->inputs[2] == INPUT_3
			&& move->inputs[3] == INPUT_4
			&& move->inputs[4] == INPUT_END
			) return false;
	if (move->forceDisableFlags && (move->forceDisableFlags & player.wasForceDisableFlags) != 0) return false;
	int posY = player.pawn.posY();
	bool isLand = !player.airborne_insideTick();  // you may ask, well what about the last airborne, prelanding frame? On that frame, your y == 1. And at the end of the tick it's 0. (Are they hardcoding aroung their own code?..)
	if (move->minimumHeightRequirement && posY < move->minimumHeightRequirement) return false;
	if (move->characterState == MOVE_CHARACTER_STATE_JUMPING && isLand
			|| (move->characterState == MOVE_CHARACTER_STATE_CROUCHING
				|| move->characterState == MOVE_CHARACTER_STATE_STANDING)
			&& !isLand) return false;
	for (int bitIndex : move->conditions) {
		if (bitIndex == -1) break;
		MoveCondition condition = (MoveCondition)bitIndex;
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
	}
	return true;
}

bool EndScene::requiresStylishInputs(const AddedMoveData* move) {
	const InputType* inputs = move->inputs;
	bool encounteredSpecial = false;
	bool encounteredNonSpecial = false;
	for (int i = 0; i < 16; ++i) {
		InputType inputType = inputs[i];
		if (inputType == INPUT_END) {
			break;
		} else if (inputType == INPUT_BOOLEAN_OR) {
			if (encounteredNonSpecial && !encounteredSpecial) return false;
			encounteredSpecial = false;
			encounteredNonSpecial = false;
		} else if (inputType == INPUT_SPECIAL_STRICT_PRESS
				|| inputType == INPUT_SPECIAL_STRICT_RELEASE
				|| inputType == INPUT_PRESS_SPECIAL
				|| inputType == INPUT_RELEASE_SPECIAL
				|| inputType == INPUT_HOLD_SPECIAL) {
			encounteredSpecial = true;
		} else {
			encounteredNonSpecial = true;
		}
	}
	return encounteredSpecial;
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
				maxCount = 100;
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
			attackerPlayer.determineMoveNameAndSlangName(&newCalc.attackName);
		} else {
			PlayerInfo::determineMoveNameAndSlangName(attackerPtr, &newCalc.attackName);
		}
		newCalc.nameFull = nullptr;
	} else {
		ProjectileInfo::determineMoveNameAndSlangName(attackerPtr, &newCalc.attackName, &newCalc.nameFull);
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
	PlayerInfo& attacker = findPlayer(attackerPtr.playerEntity());
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
	data.dangerTime = defenderPtr.receivedCounterHit() == COUNTER_HIT_ENTITY_VALUE_MORTAL_COUNTER || game.getDangerTime() != 0;
	
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
	PlayerInfo& attacker = findPlayer(attackerPtr.playerEntity());
	PlayerInfo& defender = findPlayer(defenderPtr);
	if (!defender.pawn || defender.dmgCalcs.empty() || defender.dmgCalcs.back().hitResult != HIT_RESULT_NORMAL) return;
	DmgCalc& dmgCalc = defender.dmgCalcs.back();
	DmgCalc::DmgCalcU::DmgCalcHit& data = dmgCalc.u.hit;
	const AttackData* attack = defenderPtr.receivedAttack();
	data.baseStun = attack->stun;
	data.comboCount = defenderPtr.comboCount();
	data.counterHit = defenderPtr.receivedCounterHit();
	data.tensionMode = defenderPtr.enemyEntity().tensionMode();
	data.oldStun = defenderPtr.stun();
	data.stunMax = defenderPtr.stunThreshold();
	
	ProjectileInfo* projectile = nullptr;
	MoveInfo* projectileMovePtr = nullptr;
	bool projectileMoveNonEmpty = false;
	int framebarId = -1;
	if (!attackerPtr.isPawn()) {
		projectile = &findProjectile(attackerPtr);
		if (projectile->ptr && projectile->moveNonEmpty) {
			projectileMoveNonEmpty = true;
			projectileMovePtr = &projectile->move;
			framebarId = projectile->move.framebarId;
		} else {
			static MoveInfo projectileMove;
			projectileMovePtr = &projectileMove;
			projectileMoveNonEmpty = moves.getInfo(projectileMove,
				attackerPtr.playerEntity().characterType(),
				nullptr,
				attackerPtr.animationName(),
				true);
			if (projectileMoveNonEmpty) {
				framebarId = projectileMove.framebarId;
			}
		}
	}
	// we check for hitstun and not for comboCount == 1, because some hits don't increment the combo count
	bool isFirstHit = !defenderPtr.inHitstun();
	if (isFirstHit) {
		attacker.comboRecipe.clear();
	}
	if (isFirstHit
			// Answer Firesale does an invisible hit after playing a portion of the superfreeze
			// we need to ignore that
			|| attackerPtr.dealtAttack()->isThrow()
			|| !attackerPtr.dealtAttack()->collisionForceExpand()
	) {
		if (attackerPtr.isPawn()) {
			const AttackData* dealtAttack = attackerPtr.dealtAttack();
			bool isNormalThrow = dealtAttack->isThrow()
					&& dealtAttack->type == ATTACK_TYPE_NORMAL
					&& attackerPtr.currentAnimDuration() == 1;
			// actually we could just check trial name NageTsukamiLand for normal ground throw or NageTsukamiAir for normal air throw
			if (
				// If we do Slayer 5H RRC walk ground throw, we get 5H 6H Ground Throw. It even erases RRC
				!isNormalThrow
				// I added this because I could not see ground throw as a starter in any of the combo recipes, it would just be nothing
				|| isFirstHit
			) {
				ComboRecipeElement* lastElem = nullptr;
				if (attacker.lastPerformedMoveNameIsInComboRecipe) {
					lastElem = attacker.findLastNonProjectileComboElement();
				}
				
				if (
						attacker.lastPerformedMoveNameIsInComboRecipe
						&& lastElem && lastElem->whiffed) {
					lastElem->player_markAsNotWhiff(attacker, dmgCalc, getAswEngineTick());
				} else if (!attacker.lastPerformedMoveNameIsInComboRecipe
						|| !lastElem
						|| !lastElem->whiffed
						&& attacker.moveNonEmpty
						&& attacker.move.showMultipleHitsFromOneAttack) {
					// for moves that can hit before animation frame 3
					// and also for moves that are allowed to register multiple hits
					ui.comboRecipeUpdatedOnThisFrame[attacker.index] = true;
					attacker.comboRecipe.emplace_back();
					ComboRecipeElement& newComboElem = attacker.comboRecipe.back();
					newComboElem.player_onFirstHitHappenedBeforeFrame3(attacker, dmgCalc, getAswEngineTick(), isNormalThrow);
				} else if (attacker.lastPerformedMoveNameIsInComboRecipe
						&& lastElem && lastElem->hitCount > 0) {
					++lastElem->hitCount;
					attacker.timeSinceWasEnableJumpCancel = 0;
					attacker.timeSinceWasEnableSpecialCancel = 0;
					attacker.timeSinceWasEnableSpecials = 0;
					attacker.wasCancels.onHit();
				}
			}
		} else {
			bool butAddAnyway = false;
			ComboRecipeElement* oldElem = nullptr;
			_ASSERTE(projectile);
			if (!isFirstHit && projectile->ptr) {
				if (projectile->alreadyIncludedInComboRecipe && !attacker.comboRecipe.empty()) {
					auto nextIt = attacker.comboRecipe.begin();
					for (auto it = attacker.comboRecipe.begin() + (attacker.comboRecipe.size() - 1);
						nextIt != attacker.comboRecipe.end();
						it = nextIt
					) {
						if (it == attacker.comboRecipe.begin()) {
							nextIt = attacker.comboRecipe.end();
						} else {
							nextIt = it - 1;
						}
						
						if (it->framebarId == projectile->alreadyIncludedInComboRecipeTime) {
							oldElem = &*it;
							break;
						}
						
					}
				}
				
				if (oldElem && (
					projectileMoveNonEmpty
					&& projectileMovePtr->showMultipleHitsFromOneAttack
					|| attacker.charType == CHARACTER_TYPE_VENOM
					&& strcmp(attackerPtr.dealtAttack()->trialName, "Ball_Gold") == 0
				)) {
					butAddAnyway = true;
				}
			}
			if (!oldElem || butAddAnyway) {
				if (
					(
						projectileMoveNonEmpty
						&& projectileMovePtr->combineHitsFromDifferentProjectiles
						|| attacker.charType == CHARACTER_TYPE_VENOM
						&& strcmp(attackerPtr.dealtAttack()->trialName, "Ball_RedHail") == 0
					)
					&& attacker.lastComboHitEqualsProjectile(attackerPtr)
					|| !attacker.comboRecipe.empty()
					&& attacker.comboRecipe.back().framebarId == framebarId
					&& combineHitsFromFramebarId(framebarId)
				) {
					oldElem = &attacker.comboRecipe.back();
					butAddAnyway = false;
				}
			}
			if (!oldElem || butAddAnyway) {
				if (projectile->ptr) projectile->alreadyIncludedInComboRecipe = true;
				ui.comboRecipeUpdatedOnThisFrame[attacker.index] = true;
				attacker.comboRecipe.emplace_back();
				ComboRecipeElement& newComboElem = attacker.comboRecipe.back();
				newComboElem.name = dmgCalc.attackName;
				newComboElem.whiffed = false;
				newComboElem.timestamp = getAswEngineTick();
				newComboElem.isProjectile = true;
				newComboElem.counterhit = data.counterHit;
				newComboElem.otg = dmgCalc.isOtg;
				newComboElem.framebarId = framebarId;
				newComboElem.hitCount = 1;
				memcpy(newComboElem.stateName, attackerPtr.animationName(), 32);
				memcpy(newComboElem.trialName, attackerPtr.dealtAttack()->trialName, 32);
			} else if (oldElem) {
				if (oldElem->hitCount == 0) oldElem->hitCount = 1;
				++oldElem->hitCount;
			}
		}
	}
}

bool EndScene::drawingPostponed() const {
	return settings.dodgeObsRecording && endSceneAndPresentHooked && !obsStoppedCapturing;
}

#ifdef LOG_PATH
bool loggedDrawingInputsOnce = false;
#endif
void EndScene::prepareInputs() {
	InputRingBuffer* sourceBuffers = game.getInputRingBuffers();
	if (!sourceBuffers) return;
	bool withDurations = settings.showDurationsInInputHistory;
	DWORD currentTime = getAswEngineTick();
	for (int i = 0; i < 2; ++i) {
		inputRingBuffersStored[i].update(sourceBuffers[i], prevInputRingBuffers[i], currentTime);
		std::vector<InputsDrawingCommandRow>& result = drawDataPrepared.inputs[i];
		if (result.size() != 100) result.resize(100);
		drawDataPrepared.inputsSize[i] = 0;
		memset(result.data(), 0, 100 * sizeof InputsDrawingCommandRow);
		inputsDrawing.produceData(inputRingBuffersStored[i], result.data(), drawDataPrepared.inputsSize + i, i == 1, withDurations);
		drawDataPrepared.inputsContainsDurations = withDurations;
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

void EndScene::HookHelp::onCmnActXGuardLoopHook(BBScrEvent signal, int type, int thisIs0) {
	endScene.onCmnActXGuardLoopHook(Entity{(void*)this}, signal, type, thisIs0);
}

void EndScene::onCmnActXGuardLoopHook(Entity pawn, BBScrEvent signal, int type, int thisIs0) {
	int oldBlockstun = pawn.blockstun();
	orig_onCmnActXGuardLoop((void*)pawn.ent, signal, type, thisIs0);
	int newBlockstun = pawn.blockstun();
	
	if (newBlockstun == oldBlockstun + 4) {
		PlayerInfo& player = findPlayer(pawn);
		player.blockstunMaxLandExtra = 4;
	}
}

void EndScene::fillInFontInfo(BYTE** staticFontTexture2D,
		CharInfo* openParenthesis,
		CharInfo* closeParenthesis,
		CharInfo* digit) {
	BYTE* staticFontVal = game.getStaticFont();
	*staticFontTexture2D = getUTexture2DFromFont(staticFontVal);
	BYTE* tArrayFontCharacter = (BYTE*)(staticFontVal + 0x3c);
	FontCharacter* arrayData = *(FontCharacter**)tArrayFontCharacter;
	int arrayNum = *(int*)(tArrayFontCharacter + 0x4);
	
	if (!obtainedStaticFontCharInfos) {
		obtainedStaticFontCharInfos = true;
		struct IndexToChar {
			int index = 0;
			CharInfo* info = nullptr;
			int offsetY = 0;
			int extraSpaceRight = 0;
		};
		IndexToChar infos[12];
		infos[0] = {
			4 + 8 - 1,
			&staticFontOpenParenthesis,
			0,
			0
		};
		infos[1] = {
			4 + 9 - 1,
			&staticFontCloseParenthesis,
			0,
			0
		};
		for (int i = 0; i < 10; ++i) {
			infos[2 + i] = { 4 + 16 + i - 1, staticFontDigit + i };
		}
		
		// 1
		infos[3].offsetY = 1;
		infos[3].extraSpaceRight = 2;
		
		// 2
		infos[4].extraSpaceRight = -1;
		
		// 4
		infos[6].offsetY = 2;
		infos[6].extraSpaceRight = -2;
		
		// 5
		infos[7].offsetY = 1;
		
		// 6
		infos[8].offsetY = 1;
		
		// 7
		infos[9].offsetY = 2;
		
		const float width = 4096.F;
		const float height = 2048.F;
		for (int i = 0; i < 12; ++i) {
			const IndexToChar& src = infos[i];
			if (src.index < arrayNum) {
				FontCharacter* fontCharacter = arrayData + src.index;
				CharInfo* dst = src.info;
				dst->uStart = (float)fontCharacter->StartU / width;
				dst->vStart = (float)fontCharacter->StartV / height;
				dst->uEnd = (float)(fontCharacter->StartU + fontCharacter->USize) / width;
				dst->vEnd = (float)(fontCharacter->StartV + fontCharacter->VSize) / height;
				dst->sizeX = fontCharacter->USize;
				dst->sizeY = fontCharacter->VSize;
				dst->offsetY = src.offsetY;
				dst->extraSpaceRight = src.extraSpaceRight;
			}
		}
	}
	
	*openParenthesis = staticFontOpenParenthesis;
	*closeParenthesis = staticFontCloseParenthesis;
	memcpy(digit, staticFontDigit, sizeof staticFontDigit);
	
}

void EndScene::HookHelp::drawTrainingHudInputHistoryHook(unsigned int layer) {
	endScene.drawTrainingHudInputHistoryHook((void*)this, layer);
}

void EndScene::drawTrainingHudInputHistoryHook(void* trainingHud, unsigned int layer) {
	GameModeFast gameMode = *gameModeFast;
	int DAT_01edb6f0 = *drawTrainingHudInputHistoryVal2;
	int DAT_01edb6ec = *drawTrainingHudInputHistoryVal3;
	int iVar3 = DAT_01edb6f0;
	bool willDraw = gameMode == GAME_MODE_FAST_CHALLENGE
			? DAT_01edb6f0 != 0
			: gameMode != GAME_MODE_FAST_KENTEI || DAT_01edb6ec;
	if (willDraw && settings.showDurationsInInputHistory) {
		char buf[9] = "+";
		queueDummyCommand(layer, dummyInputHistory, buf);
		requestedInputHistoryDraw = true;
	} else {
		orig_drawTrainingHudInputHistory(trainingHud, layer);
	}
}

GameModeFast EndScene::getGameModeFast() const {
	if (gameModeFast) return *gameModeFast;
	return GAME_MODE_FAST_NORMAL;
}

int EndScene::getMinBufferTime(const InputType* inputs) {
	int minLength = -1;
	int currentLength = -1;
	for (int i = 0; i < 16; ++i) {
		InputType inputType = inputs[i];
		if (inputType == INPUT_END) {
			if (currentLength != -1 && (minLength == -1 || currentLength < minLength)) return currentLength;
			return minLength;
		}
		if (inputType == INPUT_BOOLEAN_OR) {
			if (currentLength != -1 && (minLength == -1 || currentLength < minLength)) {
				minLength = currentLength;
			}
			currentLength = -1;
		}
		int bufferTime = inputNames[inputType].bufferTime;
		if (bufferTime != -1 && (currentLength == -1 || bufferTime < currentLength)) {
			currentLength = bufferTime;
		}
	}
	return minLength;
}

int __cdecl LifeTimeCounterCompare(void const* p1Ptr, void const* p2Ptr) {
	Entity p1Ent = *(Entity*)p1Ptr;
	Entity p2Ent = *(Entity*)p2Ptr;
	return p2Ent.lifeTimeCounter() - p1Ent.lifeTimeCounter();
}

// Kinda copy of handleVenomBalls function, with some filters which are explained in comments
// The original function runs at the start of a logic tick, but we run it at the end of the current tick, which is probably the exact same
// Runs on the main thread
void EndScene::checkVenomBallActivations() {
	if (!isSignVer1_10OrHigher || !hitDetectionFunc) return;
	bool isVer1_10 = isSignVer1_10OrHigher() != 0;
	for (int i = 0; i < entityList.count; ++i) {
		Entity attacker = entityList.list[i];
		if (!attacker.isActive() || !attacker.canTriggerBalls()) continue;
		
		for (int j = 0; j < entityList.count; ++j) {
			Entity defender = entityList.list[j];
			if (defender != attacker && !defender.isPawn() && (defender.venomBallFlags() & 0x4000) == 0) {
				bool orderOk = defender.venomBallArg2() < attacker.venomBallArg2();
				if (isVer1_10){
					bool attackerSpin = attacker.venomBallSpin();
					bool defenderSpin = defender.venomBallSpin();
					if (attackerSpin || defenderSpin) {
						orderOk = true;
						if (!attackerSpin && defenderSpin && attacker.canTriggerBalls() && defender.canTriggerBalls()) {
							continue;
						}
					}
				}
				if (
					(
						defender.canTriggerBalls() && orderOk && !attacker.isPawn() || (defender.venomBallFlags() & 2) != 0
					) && checkSameTeam(attacker, defender)
					&& (
						(defender.venomBallFlags() & 0x40) == 0
						|| attacker.isPawn()
					)
					&& ((attacker.venomBallFlags() & 0x80) == (defender.venomBallFlags() & 0x80))
				) {
					if (hitDetectionFunc((void*)attacker.ent, (void*)defender.ent, HITBOXTYPE_HITBOX, HITBOXTYPE_HURTBOX, nullptr, nullptr)) {
						ProjectileInfo& defenderProj = findProjectile(defender);
						if (!(  // this skip (entire 'if') is not in the original
							defenderProj.ptr && isVenomBall(defender.animationName())
						)) continue;
						
						defenderProj.gotHitOnThisFrame = true;
						if (attacker.isPawn()) {
							PlayerInfo& player = findPlayer(attacker);
							if (player.pawn) {
								const char* animName = attacker.animationName();
								if (strcmp(animName, "StingerAimC") != 0
										&& strcmp(animName, "SingerAimD") != 0
										&& strcmp(animName, "CarcassRaidC") != 0
										&& strcmp(animName, "CarcassRaidD") != 0) {
									player.hitSomething = true;
								}
							}
						} else {
							ProjectileInfo& attackerProj = findProjectile(attacker);
							if (attackerProj.ptr) {
								attackerProj.landedHit = true;
							}
						}
					}
				}
			}
		}
	}
}

// Kinda copy of hitDetectionHitOwnEffects function, with some filters which are explained in comments
// The original function runs at the start of a logic tick, but we run it at the end of the current tick, which is probably the exact same
// Runs on the main thread
void EndScene::checkSelfProjectileHarmInflictions() {
	if (!hitDetectionFunc) return;
	for (int i = 0; i < entityList.count; ++i) {
		Entity attacker = entityList.list[i];
		BBScrEvent event = attacker.signalToSendToYourOwnEffectsWhenHittingThem();
		if (event == BBSCREVENT_HIT_OWN_PROJECTILE_DEFAULT
				|| attacker.hitboxCount(HITBOXTYPE_HITBOX) == 0 && !attacker.effectLinkedCollision()) {
			continue;
		}
		for (int j = 0; j < entityList.count; ++j) {
			Entity defender = entityList.list[j];
			if (defender != attacker
					&& defender.naguriNagurareru()
					&& checkSameTeam(attacker, defender)  // this call is inlined in the real function
					// this check is not in the original function
					&& (
						defender.hasUpon(event)
						|| defender.needGoToStateUpon(event)
						|| defender.needGoToMarkerUpon(event)
					)
			) {
				ProjectileInfo& defenderProj = findProjectile(defender);
				if (hitDetectionFunc((void*)attacker.ent, (void*)defender.ent, HITBOXTYPE_HITBOX, HITBOXTYPE_HURTBOX, nullptr, nullptr)) {
					// prevent double collision for two frames in a row resulting from RC slowdown delaying the goto by 1 frame
					if (!(
						isGrenadeBomb(defender.animationName())
						&& strcmp(defender.gotoLabelRequests(), "explode2") == 0
						|| isDizzyBubble(defender.animationName())
						&& strcmp(defender.gotoLabelRequests(), "bomb") == 0
					)) {
						// in the original, the attacker.signalToSendToYourOwnEffectsWhenHittingThem() signal is sent instead
						defenderProj.gotHitOnThisFrame = true;
						ProjectileInfo& attackerProj = findProjectile(attacker);
						if (attackerProj.ptr) {
							PlayerInfo& player = findPlayer(attacker.playerEntity());
							if (player.pawn && player.pawn.effectLinkedCollision() == attacker) {  // Elphelt j.D fix
								player.hitSomething = true;
							} else {
								attackerProj.landedHit = true;
							}
						} else {
							PlayerInfo& player = findPlayer(attacker);
							if (player.pawn) {
								player.hitSomething = true;
							}
						}
					}
				}
			}
		}
	}
}

void increaseFramesCountUnlimited(int& counterUnlimited, int incrBy, int displayedFrames) {
	if (INT_MAX - counterUnlimited < incrBy) {
		int currentRemainder = counterUnlimited % displayedFrames;
		int remainder = _countof(Framebar::frames) % displayedFrames;
		counterUnlimited = _countof(Framebar::frames) + displayedFrames - remainder
			+ currentRemainder;
	}
	counterUnlimited += incrBy;
}

// Runs on the main thread. Called from _increaseStunHookAsm, declared in asmhooks.asm
void increaseStunHook(Entity pawn, int stunAdd) {
	endScene.increaseStunHook(pawn, stunAdd);
}

void EndScene::increaseStunHook(Entity pawn, int stunAdd) {
	if ((pawn.stun() + stunAdd) / 100 >= pawn.stunThreshold()) {
		reachedMaxStun[pawn.team()] = pawn.stun() + stunAdd;
	}
}

extern "C" void __fastcall jumpInstallNormalJumpHook(void* pawn) {
	endScene.jumpInstallNormalJumpHook(Entity{ pawn });
}

void EndScene::jumpInstallNormalJumpHook(Entity pawn) {
	PlayerInfo& player = findPlayer(pawn);
	if (!player.pawn
			|| pawn.remainingAirDashes() == pawn.maxAirdashes()
			&& pawn.remainingDoubleJumps() == pawn.maxDoubleJumps()
			|| pawn.characterType() == CHARACTER_TYPE_BEDMAN) return;
	player.jumpInstalled = true;
}

extern "C" void __fastcall jumpInstallSuperJumpHook(void* pawn) {
	endScene.jumpInstallSuperJumpHook(Entity{ pawn });
}

void EndScene::jumpInstallSuperJumpHook(Entity pawn) {
	PlayerInfo& player = findPlayer(pawn);
	if (!player.pawn
			|| pawn.remainingAirDashes() == pawn.maxAirdashes()
			|| pawn.characterType() == CHARACTER_TYPE_BEDMAN) return;
	player.superJumpInstalled = true;
}

bool EndScene::shouldIgnoreEnterKey() const {
	if (!*aswEngine) return false;
	GameMode gameMode = game.getGameMode();
	if (gameMode == GAME_MODE_NETWORK && game.getPlayerSide() == 2) return false;
	if (gameMode == GAME_MODE_REPLAY) return false;
	return true;
}

void EndScene::HookHelp::checkFirePerFrameUponsWrapperHook() {
	endScene.checkFirePerFrameUponsWrapperHook(Entity{(char*)this});
}

void EndScene::checkFirePerFrameUponsWrapperHook(Entity pawn) {
	if (pawn.isPawn()) {
		PlayerInfo& player = findPlayer(pawn);
		if (player.pawn) {
			if (player.isFirstCheckFirePerFrameUponsWrapperOfTheFrame) {
				player.wasInHitstopFreezeDuringSkillCheck = pawn.hitstop() != 0
					|| pawn.isSuperFrozen();
				player.isFirstCheckFirePerFrameUponsWrapperOfTheFrame = false;
				player.wasCancels.unsetWasFoundOnThisFrame(true);
			}
		}
	}
	orig_checkFirePerFrameUponsWrapper((void*)pawn);
}

void EndScene::HookHelp::speedYReset(int speedY) {
	endScene.speedYReset(Entity{(char*)this}, speedY);
}

void EndScene::speedYReset(Entity pawn, int speedY) {
	PlayerInfo& player = findPlayer(pawn);
	if (player.pawn) {
		player.lostSpeedYOnThisFrame = true;
		player.speedYBeforeSpeedLost = pawn.speedY();
	}
	pawn.speedY() = speedY;
}

// clears this mod's custom input history
void EndScene::clearInputHistory(bool resetClearTime) {
	if (!*aswEngine) return;
	InputRingBuffer* sourceBuffers = game.getInputRingBuffers();
	memcpy(prevInputRingBuffers, sourceBuffers, sizeof prevInputRingBuffers);
	DWORD currentTime = resetClearTime ? 0xFFFFFFFF : getAswEngineTick();
	for (int i = 0; i < 2; ++i) {
		inputRingBuffersStored[i].clear();
		inputRingBuffersStored[i].lastClearTime = currentTime;
	}
}

void EndScene::analyzeGunflame(PlayerInfo& player, bool* wholeGunflameDisappears,
		bool* firstWaveEntirelyDisappears, bool* firstWaveDisappearsDuringItsActiveFrames) {
	bool hasFirstWave = false;
	bool firstWaveActive = false;
	bool hasNonLinked = false;
	int team = player.index;
	for (const ProjectileInfo& projectile : projectiles) {
		if (projectile.team == team && strcmp(projectile.animName, "GunFlameHibashira") == 0 && projectile.ptr
				&& projectile.isDangerous) {
			Entity linkEntity = projectile.ptr.linkObjectDestroyOnDamage();
			if (linkEntity) {
				hasFirstWave = true;
				firstWaveActive = projectile.actives.count > 0;
			} else {
				hasNonLinked = true;
			}
		}
	}
	if (hasFirstWave && !hasNonLinked) {
		*wholeGunflameDisappears = true;
	} else if (hasFirstWave && !firstWaveActive && hasNonLinked) {
		*firstWaveEntirelyDisappears = true;
	} else if (hasFirstWave && firstWaveActive && hasNonLinked) {
		*firstWaveDisappearsDuringItsActiveFrames = true;
	}
}

bool EndScene::hasLinkedProjectileOfType(PlayerInfo& player, const char* name) {
	int team = player.index;
	for (const ProjectileInfo& projectile : projectiles) {
		Entity ptr = projectile.ptr;
		if (projectile.team == team && strcmp(projectile.animName, name) == 0 && ptr) {
			if (!projectile.isDangerous) {
				continue;
			}
			if (ptr.linkObjectDestroyOnDamage() != nullptr || ptr.destroyOnPlayerHitstun()) {
				return true;
			}
			// for Raven needle and orb
			if (ptr.hasUpon(BBSCREVENT_PLAYER_GOT_HIT)) {
				BYTE* instr = moves.skipInstr(ptr.uponStruct(BBSCREVENT_PLAYER_GOT_HIT)->uponInstrPtr);
				if (moves.instrType(instr) == instr_requestDestroy
						&& asInstr(instr, requestDestroy)->entity == ENT_SELF) {
					return true;
				}
			}
		}
	}
	return false;
}

bool EndScene::hasAnyProjectileOfTypeStrNCmp(PlayerInfo& player, const char* name) {
	int team = player.index;
	int strn = strlen(name);
	for (const ProjectileInfo& projectile : projectiles) {
		if (projectile.team == team && strncmp(projectile.animName, name, strn) == 0 && projectile.ptr
				&& projectile.isDangerous) {
			return true;
		}
	}
	return false;
}

bool EndScene::hasProjectileOfType(PlayerInfo& player, const char* name) {
	int team = player.index;
	for (const ProjectileInfo& projectile : projectiles) {
		if (projectile.team == team && strcmp(projectile.animName, name) == 0 && projectile.ptr) {
			return projectile.isDangerous;
		}
	}
	return false;
}

char EndScene::hasBoomerangHead(PlayerInfo& player) {
	int team = player.index;
	for (const ProjectileInfo& projectile : projectiles) {
		if (projectile.team == team
				&& strncmp(projectile.animName, "Boomerang_", 10) == 0
				&& projectile.animName[10] != '\0'
				&& strncmp(projectile.animName + 11, "_Head", 5) == 0  // includes _Head_Air
				&& projectile.ptr) {
			if (!projectile.isDangerous) return '\0';
			return projectile.animName[10];
		}
	}
	return false;
}

bool EndScene::hasAnyProjectileOfType(PlayerInfo& player, const char* name) {
	int team = player.index;
	for (const ProjectileInfo& projectile : projectiles) {
		if (projectile.team == team && strcmp(projectile.animName, name) == 0 && projectile.ptr
				&& projectile.isDangerous) {
			return true;
		}
	}
	return false;
}

bool EndScene::hasProjectileOfTypeAndHasNotExhausedHit(PlayerInfo& player, const char* name) {
	int team = player.index;
	for (const ProjectileInfo& projectile : projectiles) {
		if (projectile.team == team && strcmp(projectile.animName, name) == 0 && projectile.ptr) {
			return projectile.isDangerous
				&& (
					!projectile.ptr.hasActiveFlag()
					|| projectile.ptr.hitAlreadyHappened() < projectile.ptr.theValueHitAlreadyHappenedIsComparedAgainst()
				);
		}
	}
	return false;
}

bool EndScene::hasProjectileOfTypeStrNCmp(PlayerInfo& player, const char* name) {
	int team = player.index;
	int strn = strlen(name);
	for (const ProjectileInfo& projectile : projectiles) {
		if (projectile.team == team && strncmp(projectile.animName, name, strn) == 0 && projectile.ptr) {
			return projectile.isDangerous;
		}
	}
	return false;
}

void EndScene::removeAttackHitbox(Entity attackerPtr) {
	for (auto it = attackHitboxes.begin(); it != attackHitboxes.end(); ++it) {
		if (it->ent == attackerPtr) {
			attackHitboxes.erase(it);
			return;
		}
	}
}

bool EndScene::isActiveFull(Entity ent) const {
	return ent.isActiveFrames()
			&& (
				ent.isPawn()
					&& ent.characterType() == CHARACTER_TYPE_JAM
					&& strcmp(ent.animationName(), "Saishingeki") == 0
					&& ent.currentAnimDuration() > 100
				?
					ent.currentHitNum() > 1
				: true
			);
}

void EndScene::onHitDetectionAttackerParticipate(Entity ent) {
	if (
			(
				// easy clash only happens between players, and those can't lose their hitboxes at the end of a tick, so we don't need to register easy clashes
				currentHitDetectionType == HIT_DETECTION_NORMAL
				&& !ent.dealtAttack()->clashOnly()
				|| currentHitDetectionType == HIT_DETECTION_CLASH
			)
			&& !ent.isPawn()  // players can't lose their hitboxes at the end of a tick, so there's no point collecting them for them
			&& ent.hitboxCount(HITBOXTYPE_HITBOX) > 0
	) {
		static std::vector<DrawHitboxArrayCallParams> hitboxesArena;
		AttackHitbox attackBox;
		
		int count = 0;
		collectHitboxes(ent,
			isActiveFull(ent),
			nullptr,
			&hitboxesArena,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			&count);
		
		if (count) {
			attackBox.ent = ent;
			attackBox.team = ent.team();
			attackBox.notClash = currentHitDetectionType == HIT_DETECTION_NORMAL;
			attackBox.clash = currentHitDetectionType == HIT_DETECTION_CLASH;
			attackBox.count = count;
			attackBox.hitbox = hitboxesArena.back();
			
			bool found = false;
			for (AttackHitbox& existingBox : attackHitboxes) {
				if (existingBox.ent == ent && existingBox.hitbox == attackBox.hitbox) {
					existingBox.clash = existingBox.clash || attackBox.clash;
					existingBox.notClash = existingBox.notClash || attackBox.notClash;
					found = true;
					break;
				}
			}
			
			if (!found) {
				attackHitboxes.push_back(attackBox);
			}
			hitboxesArena.clear();
		}
	}
}

bool EndScene::objHasAttackHitboxes(Entity ent) const {
	if (!ent) return false;
	for (const AttackHitbox& attackHitbox : attackHitboxes) {
		if (attackHitbox.ent == ent) return true;
	}
	return false;
}


void EndScene::addPredefinedCancelsPart(PlayerInfo& player, std::vector<ForceAddedWhiffCancel>& cancels, FrameCancelInfoFull& frame, bool inHitstopFreeze,
		bool isStylish) {
	const AddedMoveData* base = player.pawn.movesBase();
	int* indices = player.pawn.moveIndices();
	for (ForceAddedWhiffCancel& cancel : cancels) {
		int moveIndex = cancel.getMoveIndex(player.pawn);
		const AddedMoveData* move = base + indices[moveIndex];
		if (checkMoveConditions(player, move) && (isStylish || !requiresStylishInputs(move))) {
			collectFrameCancelsPart(player, frame.whiffCancels, move, moveIndex, inHitstopFreeze);
		}
	}
}

void EndScene::initializePredefinedCancels(const char** array, size_t size, std::vector<ForceAddedWhiffCancel>& cancels) {
	if (cancels.empty()) {
		cancels.reserve(size);
		for (size_t i = 0; i < size; ++i) {
			cancels.emplace_back();
			ForceAddedWhiffCancel& newCancel = cancels.back();
			newCancel.name = array[i];
		}
	}
}

void EndScene::collectBaikenBlockCancels(PlayerInfo& player, FrameCancelInfoFull& frame, bool inHitstopFreeze, bool isStylish) {
	static const char* baikenBlockCancelsStr[] {
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
	addPredefinedCancels(player, baikenBlockCancelsStr, baikenBlockCancels, frame, inHitstopFreeze, isStylish);
}

bool EndScene::blitzShieldCancellable(PlayerInfo& player, bool insideTick) {
	
	int storage = player.pawn.storage(3);
	if (!(
		storage >= 12 && (
			player.pawn.hitstop()
			|| (
				insideTick
					? isBlitzPostHitstopFrame_insideTick(player)
					: isBlitzPostHitstopFrame_outsideTick(player)
			)
		)
	)) return false;
	
	const char* animName = player.pawn.animationName();
	
	if (strcmp(animName, "CounterGuardStand") == 0 || strcmp(animName, "CounterGuardCrouch") == 0) {
		if (Moves::getBlitzType(player) != BLITZTYPE_TAP) return false;
	} else if (strcmp(animName, "CounterGuardAir") != 0) return false;
	
	return true;
}

void EndScene::collectBlitzShieldCancels(PlayerInfo& player, FrameCancelInfoFull& frame, bool inHitstopFreeze, bool isStylish) {
	
	CheckAndCollectFrameCancelParams params {
		player,
		nullptr,
		nullptr,
		frame,
		inHitstopFreeze,
		isStylish,
		true
	};
	
	bool isAir = player.pawn.y() > 0;
	if (!isAir) {
		checkAndCollectFrameCancelHelper(&params, "CmnBDash", &bdashMoveIndex);
		checkAndCollectFrameCancelHelper(&params, "CmnFDash", &fdashMoveIndex);
		checkAndCollectFrameCancelHelper(&params, "CounterGuardStand", &player.counterGuardStandMoveIndex);
		checkAndCollectFrameCancelHelper(&params, "CounterGuardCrouch", &player.counterGuardCrouchMoveIndex);
		checkAndCollectFrameCancelHelper(&params, "FaultlessDefenceStand", &fdStandMoveIndex);
		checkAndCollectFrameCancelHelper(&params, "FaultlessDefenceCrouch", &fdCrouchMoveIndex);
	} else {
		checkAndCollectFrameCancelHelper(&params, "CmnFAirDash", &cmnFAirDashMoveIndex);
		checkAndCollectFrameCancelHelper(&params, "CmnBAirDash", &cmnBAirDashMoveIndex);
		checkAndCollectFrameCancelHelper(&params, "CounterGuardAir", &player.counterGuardAirMoveIndex);
		checkAndCollectFrameCancelHelper(&params, "FaultlessDefenceAir", &fdAirMoveIndex);
	}
	checkAndCollectFrameCancelHelper(&params, "CmnMaximumBurst", &maximumBurstMoveIndex);
	checkAndCollectFrameCancelHelper(&params, "RomanCancelHit", &rcMoveIndex);
	if (!isAir) {
		// you are currently in blitz, therefore IK is not activated or sent yet
		checkAndCollectFrameCancelHelper(&params, "IchigekiJunbi", &player.ikMoveIndex);
	}
	
	const AddedMoveData* base = player.pawn.movesBase();
	int* indices = player.pawn.moveIndices();
	for (int i = player.pawn.moveIndicesCount() - 1; i >= 0; --i) {
		int index = indices[i];
		const AddedMoveData* move = base + index;
		if (move->type == MOVE_TYPE_NORMAL && !move->isFollowupMove()
				&& checkMoveConditions(player, move) && (isStylish || !requiresStylishInputs(move))) {
			collectFrameCancelsPart(player, frame.gatlings, move, index, inHitstopFreeze, true);
		}
	}
}

void EndScene::checkAndCollectFrameCancel(const CheckAndCollectFrameCancelParams* params) {
	
	const AddedMoveData* move = nullptr;
	const int* indices = params->player.pawn.moveIndices();
	
	if (*params->moveIndex == -1) {
		move = (const AddedMoveData*)findMoveByName(params->player.pawn, params->name, 0);
		if (!move) return;
		
		int indexToFind = move->index;
		int i;
		for (i = params->player.pawn.moveIndicesCount() - 1; i >= 0; --i) {
			if (indices[i] == indexToFind) {
				*params->moveIndex = i;
				break;
			}
		}
		if (i < 0) return;
		
	} else {
		move = params->player.pawn.movesBase() + indices[*params->moveIndex];
	}
	
	if (checkMoveConditions(params->player, move) && (params->isStylish || !requiresStylishInputs(move))) {
		collectFrameCancelsPart(params->player, params->frame.gatlings, move, *params->moveIndex, params->inHitstopFreeze, params->blitzShield);
	}
}

bool EndScene::isBlitzPostHitstopFrame_insideTick(const PlayerInfo& player) {
	return player.pawn.mem45()
		&& (
			player.pawn.currentAnimDuration() == player.pawn.mem47() + 1
			|| player.pawn.isRCFrozen()
			&& player.pawn.enableNormals()
			&& !player.wasEnableNormals
		);
}

bool EndScene::isBlitzPostHitstopFrame_outsideTick(const PlayerInfo& player) {
	return player.pawn.mem45()
		&& player.pawn.mem51() <= 12
		&& !player.pawn.fullInvul()
		&& !player.wasEnableNormals
		&& player.pawn.enableNormals();
}

void EndScene::fillInBedmanSealInfo(PlayerInfo& player) {
	
	BedmanInfo& bi = player.bedmanInfo;
	
	struct SealInfo {
		Moves::MayIrukasanRidingObjectInfo& info;
		const char* stateName;
		BBScrEvent signal;
	};
	SealInfo seals[4] {
		{ moves.bedmanSealA, "BoomerangA" /* DejavIconBoomerangA */, BBSCREVENT_CUSTOM_SIGNAL_6 },
		{ moves.bedmanSealB, "BoomerangB" /* DejavIconBoomerangB */, BBSCREVENT_CUSTOM_SIGNAL_8 },
		{ moves.bedmanSealC, "SpiralBed" /* DejavIconSpiralBed */, BBSCREVENT_CUSTOM_SIGNAL_9 },
		{ moves.bedmanSealD, "FlyingBed" /* DejavIconFlyingBed */, BBSCREVENT_CUSTOM_SIGNAL_7 }
	};
	
	bi.sealA = 0;
	bi.sealB = 0;
	bi.sealC = 0;
	bi.sealD = 0;
	bi.sealAInvulnerable = 0;
	bi.sealBInvulnerable = 0;
	bi.sealCInvulnerable = 0;
	bi.sealDInvulnerable = 0;
	
	for (ProjectileInfo& projectile : projectiles) {
		
		if (!(
				projectile.team == player.index
				&& strncmp(projectile.animName, "DejavIcon", 9) == 0
		)) continue;
		
		for (int j = 0; j < 4; ++j) {
			SealInfo& seal = seals[j];
			if (strcmp(projectile.animName + 9, seal.stateName) == 0) {
				bool isInvul = projectile.ptr.fullInvul();
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
				
				unsigned short sealTimer = calculatedResult > 999 ? 999 : calculatedResult;
				unsigned short sealTimerMax = calculatedResultMax > 999 ? 999 : calculatedResultMax;
				
				if (j == 0) {
					bi.sealA = sealTimer;
					bi.sealAMax = sealTimerMax;
					bi.sealAInvulnerable = isInvul;
				} else if (j == 1) {
					bi.sealB = sealTimer;
					bi.sealBMax = sealTimerMax;
					bi.sealBInvulnerable = isInvul;
				} else if (j == 2) {
					bi.sealC = sealTimer;
					bi.sealCMax = sealTimerMax;
					bi.sealCInvulnerable = isInvul;
				} else if (j == 3) {
					bi.sealD = sealTimer;
					bi.sealDMax = sealTimerMax;
					bi.sealDInvulnerable = isInvul;
				}
				break;
			}
		}
	}
	
	// can only have one flying head at a time
	char headType = hasBoomerangHead(player);
	bi.hasBoomerangAHead = headType == 'A';
	bi.hasBoomerangBHead = headType == 'B';
	bi.hasDejavuAGhost = false;
	bi.hasDejavuBGhost = false;
	bi.dejavuAGhostAlreadyCreatedBoomerang = false;
	bi.dejavuBGhostAlreadyCreatedBoomerang = false;
	bi.hasDejavuCGhost = false;
	bi.hasDejavuDGhost = false;
	bi.dejavuCGhostInRecovery = false;
	bi.dejavuDGhostInRecovery = false;
	
	const ProjectileInfo* ghostAB = nullptr;
	const ProjectileInfo* ghostCD = nullptr;
	int team = player.index;
	for (const ProjectileInfo& projectile : projectiles) {
		if (projectile.team == team && projectile.ptr) {
			if (strncmp(projectile.animName, "Djavu_", 6) == 0 && strcmp(projectile.animName + 7, "_Ghost") == 0) {
				char letter = projectile.animName[6];
				if (letter == 'A' || letter == 'B') {
					ghostAB = &projectile;
				} else if (letter == 'C' || letter == 'D') {
					ghostCD = &projectile;
				}
			}
		}
	}
	if (ghostAB) {
		bool alreadyCreated = ghostAB->ptr.stackEntity(0) != nullptr;
		bool isValid = !ghostAB->ptr.destructionRequested();
		if (ghostAB->animName[6] == 'A') {
			bi.hasDejavuAGhost = isValid;
			bi.dejavuAGhostAlreadyCreatedBoomerang = alreadyCreated;
		} else {
			bi.hasDejavuBGhost = isValid;
			bi.dejavuBGhostAlreadyCreatedBoomerang = alreadyCreated;
		}
	}
	bi.hasDejavuBoomerangA = hasAnyProjectileOfTypeStrNCmp(player, "Boomerang_A_Djavu");
	bi.hasDejavuBoomerangB = hasAnyProjectileOfTypeStrNCmp(player, "Boomerang_B_Djavu");
	
	if (ghostCD != nullptr && !ghostCD->ptr.destructionRequested()) {
		char letter = ghostCD->animName[6];
		bi.hasDejavuCGhost = letter == 'C';
		bi.dejavuCGhostInRecovery = letter == 'C' && !ghostCD->isDangerous;
		bi.hasDejavuDGhost = letter == 'D';
		bi.dejavuDGhostInRecovery = letter == 'D' && !ghostCD->isDangerous;
	}
	
	bi.hasShockwaves = hasAnyProjectileOfTypeStrNCmp(player, "bomb");
	bi.hasOkkake = hasAnyProjectileOfTypeStrNCmp(player, "Okkake");
}

void UiOrFramebarDrawData::applyFramebarTexture() const {
	if (needUpdateFramebarTexture
			&& graphics.needUpdateFramebarTexture(&framebarSizes, framebarColorblind)) {
		graphics.updateFramebarTexture(&framebarSizes, framebarTexture, framebarColorblind);
	}
}

void EndScene::testDelay() {
	InputRingBuffer* inputRingBuffer = game.getInputRingBuffers() + game.getPlayerSide();
	if (inputRingBuffer->framesHeld[inputRingBuffer->index] == 1
			&& inputRingBuffer->inputs[inputRingBuffer->index].punch) {
		ui.needTestDelayStage2 = false;
		ui.hasTestDelayResult = true;
		unsigned long long currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		ui.testDelayResult = (DWORD)(currentTime - ui.testDelayStart);
	}
}

bool EndScene::hookPawnArcadeModeIsBoss() {
	if (!settings.player1IsBoss && !settings.player2IsBoss || Pawn_ArcadeMode_IsBossHooked) return true;
	
	if (!orig_Pawn_ArcadeMode_IsBoss) return false;
	
	// it is possible that we're already in transaction, as this function also gets called from onDllMain
	bool wasTransaction = detouring.isInTransaction();
	
	if (!wasTransaction) {
		// not freezing threads, because we moved UI.cpp::prepareDrawData to the main (logic) thread, and settings
		// changed by overwriting the mod's INI file directly post a window message, so on main thread as well,
		// and the function that we're about to hook only runs on the main thread
		detouring.beginTransaction(false);
	}
	
	auto Pawn_ArcadeMode_IsBossHookPtr = &HookHelp::Pawn_ArcadeMode_IsBossHook;
	if (!detouring.attach(&(PVOID&)orig_Pawn_ArcadeMode_IsBoss,
		(PVOID&)Pawn_ArcadeMode_IsBossHookPtr,
		"Pawn_ArcadeMode_IsBoss")) return false;
	
	if (!wasTransaction) {
		detouring.endTransaction();
	}
	
	Pawn_ArcadeMode_IsBossHooked = true;
	
	return true;
}

BOOL EndScene::HookHelp::Pawn_ArcadeMode_IsBossHook() {
	return endScene.Pawn_ArcadeMode_IsBossHook(Entity{(void*)this});
}

BOOL EndScene::Pawn_ArcadeMode_IsBossHook(Entity pawn) {
	GameMode gameMode = game.getGameMode();
	if (
		(
			gameMode == GAME_MODE_TRAINING
			|| gameMode == GAME_MODE_VERSUS
		)
		&& (
			settings.player1IsBoss && pawn.team() == 0
			|| settings.player2IsBoss && pawn.team() == 1
		)
	) {
		return TRUE;
	}
	return orig_Pawn_ArcadeMode_IsBoss((void*)pawn.ent);
}

bool EndScene::onPlayerIsBossChanged() {
	if (settings.player1IsBoss || settings.player2IsBoss) {
		return hookPawnArcadeModeIsBoss();
	}
	return true;
}

static void enableMove(PlayerInfo& player, const char* name, bool isEnabled) {
	AddedMoveData* addedMove = (AddedMoveData*)player.pawn.findAddedMove(name);
	if (addedMove) {
		if (!isEnabled) {
			addedMove->addCondition(MOVE_CONDITION_ALWAYS_FALSE);
		} else {
			addedMove->removeCondition(MOVE_CONDITION_ALWAYS_FALSE);
		}
	}
}

static void modify6SH_changeInput(PlayerInfo& player, const char* name, bool changeTo6SH) {
	AddedMoveData* addedMove = (AddedMoveData*)player.pawn.findAddedMove(name);
	if (addedMove) {
		if (changeTo6SH) {
			bool foundNotAnyDown = false;
			int index = 0;
			for (InputType& input : addedMove->inputs) {
				if (input == INPUT_214) {
					input = INPUT_ANYFORWARD;
				} else if (input == INPUT_NOTANYDOWN) {
					foundNotAnyDown = true;
				} else if (input == INPUT_END) {
					if (!foundNotAnyDown && index < _countof(addedMove->inputs) - 1) {
						input = INPUT_NOTANYDOWN;
						addedMove->inputs[index + 1] = INPUT_END;
					}
					break;
				}
				++index;
			}
		} else {
			InputType* foundPtr = nullptr;
			InputType* endPtr = nullptr;
			for (InputType& input : addedMove->inputs) {
				if (input == INPUT_ANYFORWARD) {
					input = INPUT_214;
				} else if (input == INPUT_NOTANYDOWN) {
					foundPtr = &input;
				} else if (input == INPUT_END) {
					endPtr = &input;
					break;
				}
			}
			if (foundPtr) {
				if (!endPtr) {
					endPtr = addedMove->inputs + _countof(addedMove->inputs) - 1;
					*endPtr = INPUT_END;
				}
				int elemsToMove = endPtr - foundPtr + 1;
				if (elemsToMove > 0) {
					memmove(foundPtr, foundPtr + 1, (endPtr - foundPtr + 1) * sizeof InputType);
				}
			}
		}
	}
}

void EndScene::handleMarteliForpeliSetting(PlayerInfo& player) {
	GameMode gameMode = game.getGameMode();
	if (gameMode != GAME_MODE_TRAINING && gameMode != GAME_MODE_VERSUS) return;
	const bool marteliForpeliSetting = player.index == 0 ? settings.p1RamlethalDisableMarteliForpeli : settings.p2RamlethalDisableMarteliForpeli;
	const bool _6shSetting = player.index == 0 ? settings.p1RamlethalUseBoss6SHSwordDeploy : settings.p2RamlethalUseBoss6SHSwordDeploy;
	if (marteliForpeliSetting != player.ramlethalForpeliMarteliDisabled && player.charType == CHARACTER_TYPE_RAMLETHAL) {
		player.ramlethalForpeliMarteliDisabled = marteliForpeliSetting;
		enableMove(player, "BitBlowC", !marteliForpeliSetting);
		enableMove(player, "BitBlowD", !marteliForpeliSetting);
	}
	if (_6shSetting != player.ramlethalBoss6SHInputsModified && player.charType == CHARACTER_TYPE_RAMLETHAL) {
		player.ramlethalBoss6SHInputsModified = _6shSetting;
		modify6SH_changeInput(player, "6CBunriShot", _6shSetting);
		modify6SH_changeInput(player, "6DBunriShot", _6shSetting);
		enableMove(player, "6C_Soubi_Land", !_6shSetting);
		enableMove(player, "6D_Soubi_Land", !_6shSetting);
	}
	
}

bool EndScene::hasHitstunTiedVenomBall(PlayerInfo& player) {
	int index = player.index;
	for (ProjectileInfo& projectile : projectiles) {
		if (projectile.team == index && strcmp(projectile.animName, "Ball") == 0
				&& projectile.ptr && projectile.ptr.destroyOnPlayerHitstun()) {
			return true;
		}
	}
	return false;
}

// fix for Elphelt "Close Shot created a Far Shot" when firing uncharged sg.H
bool EndScene::eventHandlerSendsIntoRecovery(Entity ptr, BBScrEvent signal) {
	return ptr.hasUpon(signal)
		&& moves.instrType(
			moves.skipInstr(
				ptr.uponStruct(signal)->uponInstrPtr
			)
		) == instr_recoveryState;
}

bool EndScene::checkSameTeam(Entity attacker, Entity defender) {
	TeamSwap attackerSwap = attacker.teamSwap();
	TeamSwap defenderSwap = defender.teamSwap();
	if (attackerSwap == TEAM_SWAP_NEITHER || defenderSwap == TEAM_SWAP_NEITHER) {
		return false;
	}
	
	int attackerTeam = attackerSwap == TEAM_SWAP_NORMAL ? attacker.team() : 1 - attacker.team();
	int defenderTeam = defenderSwap == TEAM_SWAP_NORMAL ? defender.team() : 1 - defender.team();
	return attackerTeam == defenderTeam;
}

static bool bitLaserMinionCreatedLaser(Entity ent) {
	BYTE* bitLaserMinionFunc = ent.bbscrCurrentFunc();
	BYTE* bitLaserMinionInstr = ent.bbscrCurrentInstr();
	moves.fillRamlethalBitLaserMinionStuff(bitLaserMinionFunc);
	
	int offset = bitLaserMinionInstr - bitLaserMinionFunc;
	bool bitLaserMinionIsBoss = bitLaserMinionInstr > bitLaserMinionFunc + moves.ramlethalBitLaserMinionBossStartMarker;
	int createLaserOffset = bitLaserMinionIsBoss ? moves.ramlethalBitLaserMinionBossCreateLaser : moves.ramlethalBitLaserMinionNonBossCreateLaser;
	return offset > createLaserOffset;
}

static Entity findYoungestBitLaserMinion(int team) {
	int time = INT_MAX;
	Entity found = nullptr;
	for (int i = 2; i < entityList.count; ++i) {
		Entity ent = entityList.list[i];
		if (ent.isActive() && ent.team() == team
				&& strcmp(ent.animationName(), "BitLaser_Minion") == 0) {
			int newTime = ent.lifeTimeCounter();
			if (newTime < time) {
				time = newTime;
				found = ent;
			}
		}
	}
	return found;
}

static int countBitLaserMinions(int team) {
	int count = 0;
	for (int i = 2; i < entityList.count; ++i) {
		Entity ent = entityList.list[i];
		if (ent.isActive() && ent.team() == team
				&& strcmp(ent.animationName(), "BitLaser_Minion") == 0) {
			if (!bitLaserMinionCreatedLaser(ent)) ++count;
		}
	}
	return count;
}

static Entity findBitLaserCreatedByMinion(Entity minion) {
	for (int i = 2; i < entityList.count; ++i) {
		Entity ent = entityList.list[i];
		if (ent.isActive() && ent.parentEntity() == minion) {
			return ent;
		}
	}
	return nullptr;
}

static int countBitLasers(int team) {
	int count = 0;
	for (int i = 2; i < entityList.count; ++i) {
		Entity ent = entityList.list[i];
		if (ent.isActive() && ent.team() == team && strcmp(ent.animationName(), "BitLaser") == 0
				&& ent.destroyOnPlayerHitstun()) {
			++count;
		}
	}
	return count;
}

void EndScene::fillRamlethalDisappearance(PlayerFrame& frame, PlayerInfo& player) {
	RamlethalInfo& ri = frame.u.ramlethalInfo;
	if (player.ramlethalSSwordTimerActive) {
		ri.sSwordTime = min(255, player.ramlethalSSwordTime);
		if (!player.ramlethalSSwordKowareSonoba) {
			ri.sSwordTimeMax = min(255, player.ramlethalSSwordTimeMax);
		} else {
			ri.sSwordTimeMax = 0;
		}
		ri.sSwordSubAnim = player.ramlethalSSwordSubanim;
	} else {
		ri.sSwordTime = 0;
		ri.sSwordTimeMax = 0;
		ri.sSwordSubAnim = nullptr;
	}
	
	if (player.ramlethalHSwordTimerActive) {
		ri.hSwordTime = min(255, player.ramlethalHSwordTime);
		if (!player.ramlethalHSwordKowareSonoba) {
			ri.hSwordTimeMax = min(255, player.ramlethalHSwordTimeMax);
		} else {
			ri.hSwordTimeMax = 0;
		}
		ri.hSwordSubAnim = player.ramlethalHSwordSubanim;
	} else {
		ri.hSwordTime = 0;
		ri.hSwordTimeMax = 0;
		ri.hSwordSubAnim = nullptr;
	}
	
	ri.sSwordBlockstunLinked = player.ramlethalSSwordBlockstunLinked;
	ri.sSwordFallOnHitstun = player.ramlethalSSwordFallOnHitstun;
	ri.sSwordRecoilOnHitstun = player.ramlethalSSwordRecoilOnHitstun;
	ri.sSwordInvulnerable = player.ramlethalSSwordInvulnerable;
	
	ri.hSwordBlockstunLinked = player.ramlethalHSwordBlockstunLinked;
	ri.hSwordFallOnHitstun = player.ramlethalHSwordFallOnHitstun;
	ri.hSwordRecoilOnHitstun = player.ramlethalHSwordRecoilOnHitstun;
	ri.hSwordInvulnerable = player.ramlethalHSwordInvulnerable;
	
	const int bitLasersCount = countBitLasers(player.index);
	ri.hasLaser = bitLasersCount > 0;
	ri.hasLaserSpawnerInStartup = false;
	
	const int bitLaserMinionCount = countBitLaserMinions(player.index);
	bool hitstunLinked = false;
	
	if (strcmp(player.pawn.animationName(), "BitLaser") == 0) {
		frame.canYrcProjectile = nullptr;
		
		bool stateLinked = player.pawn.hasUpon(BBSCREVENT_PLAYER_CHANGED_STATE);
		hitstunLinked = player.pawn.hasUpon(BBSCREVENT_GOT_HIT);
		
		// if someone mods the bbscript for BitLaser to not have destroyOnPlayerHitstun, we're going to miss the hitstun link going through the GOT_HIT event, but ok
		ri.hasLaserSpawnerInStartup = bitLaserMinionCount && hitstunLinked;
		
		BYTE* funcStart = player.pawn.bbscrCurrentFunc();
		moves.fillRamlethalCreateBitLaserMinion(funcStart);
		BYTE* currentInstr = player.pawn.bbscrCurrentInstr();
		int offset = currentInstr - funcStart;
		if (offset > moves.ramlethalCreateBitLaserMinion) {
			
			Entity bitLaserMinion = findYoungestBitLaserMinion(player.index);
			const bool bitLaserMinionAlreadyCreatedLaser = bitLaserMinion ? bitLaserMinionCreatedLaser(bitLaserMinion) : true;
			
			BYTE* spriteInstr = moves.findAnySprite(funcStart + moves.ramlethalCreateBitLaserMinion);
			
			bool justCreated = spriteInstr && currentInstr == spriteInstr
					&& player.pawn.justReachedSprite();
			
			frame.canYrcProjectile = player.wasCanYrc && (!justCreated && (!stateLinked || bitLaserMinionAlreadyCreatedLaser))
				? "Can YRC, and Calvados will stay" : nullptr;
			
		}
	}
	
	ri.hasLaserMinionInStartupAndHitstunNotTied = bitLaserMinionCount && !hitstunLinked;
	
	bool linkFound = false;
	for (int i = 2; i < entityList.count; ++i) {
		Entity ent = entityList.list[i];
		if (ent.isActive() && ent.team() == player.index) {
			const char* stateName = ent.animationName();
			bool isSword = strcmp(stateName, "BitSpiral_NSpiral") == 0;
			if (strcmp(stateName, "BitSpiral_Minion") == 0 || isSword) {
				ProjectileInfo& projectile = findProjectile(ent);
				if ((!isSword || projectile.isDangerous) && ent.hasUpon(BBSCREVENT_PLAYER_GOT_HIT)) {
					linkFound = true;
					break;
				}
			}
		}
	}
	
	ri.hasSpiral = linkFound;
}

bool EndScene::elpheltGrenadeExists(PlayerInfo& player) {
	for (int i = 2; i < entityList.count; ++i) {
		Entity ent = entityList.list[i];
		if (ent.isActive() && ent.team() == player.index && ent.hasUpon(BBSCREVENT_PLAYER_GOT_HIT)) {
			const char* animName = ent.animationName();
			if (strncmp(animName, "GrenadeBomb", 11) == 0 && (
					animName[11] == '\0'
					|| strcmp(animName + 11, "_Ready") == 0
					)) {
				return true;
			}
		}
	}
	return false;
}

bool EndScene::elpheltJDExists(PlayerInfo& player) {
	for (int i = 2; i < entityList.count; ++i) {
		Entity ent = entityList.list[i];
		if (ent.isActive() && ent.team() == player.index && ent.linkObjectDestroyOnDamage() != nullptr
				&& strcmp(ent.animationName(), "HandGun_air_shot") == 0 && !ent.isRecoveryState()) {
			return true;
		}
	}
	return false;
}

void EndScene::fillInJackoInfo(PlayerInfo& player, PlayerFrame& frame) {
	JackoInfo& ji = frame.u.jackoInfo;
	
	ji.hasAegisField = player.pawn.invulnForAegisField();
	
	ji.aegisFieldAvailableIn = JackoInfo::NO_AEGIS_FIELD;
	if (player.jackoAegisReturningIn != INT_MIN) {
		int val = player.jackoAegisReturningIn;
		if (val > (int)JackoInfo::AEGIS_FIELD_MAX) {
			ji.aegisFieldAvailableIn = JackoInfo::AEGIS_FIELD_MAX;
		} else if (val < 0) {
			ji.aegisFieldAvailableIn = 0;
		} else {
			ji.aegisFieldAvailableIn = (unsigned char)val;
		}
	}
	
	ji.hasServants = false;
	for (int i = 2; i < entityList.count; ++i) {
		Entity ent = entityList.list[i];
		if (ent.isActive() && ent.team() == player.index && strncmp(ent.animationName(), "Servant", 7) == 0 && ent.animationName()[8] == '\0') {
			const ProjectileInfo& projectile = findProjectile(ent);
			if (projectile.isDangerous) {
				ji.hasServants = true;
				break;
			}
		}
	}
	
	ji.hasMagicianProjectile = false;
	for (int i = 2; i < entityList.count; ++i) {
		Entity ent = entityList.list[i];
		if (ent.isActive() && ent.team() == player.index && strncmp(ent.animationName(), "magicAtkLv", 10) == 0) {
			ji.hasMagicianProjectile = true;
			break;
		}
	}
	
	ji.hasJD = hasLinkedProjectileOfType(player, "Fireball");
	
	ji.settingPGhost = false;
	ji.settingKGhost = false;
	ji.settingSGhost = false;
	ji.resettingPGhost = false;
	ji.resettingKGhost = false;
	ji.resettingSGhost = false;
	ji.retrievingPGhost = false;
	ji.retrievingKGhost = false;
	ji.retrievingSGhost = false;
	if (strncmp(player.anim, "SummonGhost", 11) == 0) {
		bool createdProjectile = player.move.createdProjectile && player.move.createdProjectile(player);
		bool canYrc = player.move.canYrcProjectile && player.move.canYrcProjectile(player);
		if (!createdProjectile && !canYrc) {
			switch (player.anim[11]) {
				case 'A':
					if (player.pawn.mem56()) {
						ji.resettingPGhost = true;
					} else {
						ji.settingPGhost = true;
					}
					break;
				case 'B':
					if (player.pawn.mem57()) {
						ji.resettingKGhost = true;
					} else {
						ji.settingKGhost = true;
					}
					break;
				case 'C':
					if (player.pawn.mem58()) {
						ji.resettingSGhost = true;
					} else {
						ji.settingSGhost = true;
					}
					break;
			}
		}
	} else if (strcmp(player.anim, "ReturnGhost") == 0) {
		bool createdProjectile = player.move.powerup && player.move.powerup(player);
		bool canYrc = player.move.canYrcProjectile && player.move.canYrcProjectile(player);
		if (!createdProjectile && !canYrc) {
			switch (player.pawn.mem59()) {
				case 1: ji.retrievingPGhost = true; break;
				case 2: ji.retrievingKGhost = true; break;
				case 3: ji.retrievingSGhost = true; break;
			}
		}
	}
	
	ji.carryingPGhost = false;
	ji.carryingKGhost = false;
	ji.carryingSGhost = false;
	if (player.playerval0) {
		switch (player.pawn.mem59()) {
			case 1: ji.carryingPGhost = true; break;
			case 2: ji.carryingKGhost = true; break;
			case 3: ji.carryingSGhost = true; break;
		}
	}
	
}

static void dizzyFunc_SakanaObj(DizzyInfo& di, ProjectileInfo& projectile) {
	if (
		(
			projectile.ptr.linkObjectDestroyOnDamage() != nullptr
			|| projectile.ptr.hasUpon(BBSCREVENT_PLAYER_GOT_HIT)
		) && projectile.isDangerous
	) {
		if (projectile.ptr.createArgHikitsukiVal1() == 1) {
			di.hasFirePillar = true;
		} else {
			di.hasIceSpike = true;
		}
	}
}

static void dizzyFunc_AkariObj(DizzyInfo& di, ProjectileInfo& projectile) {
	if (projectile.ptr.linkObjectDestroyOnDamage() != nullptr && projectile.isDangerous) {
		if (projectile.ptr.createArgHikitsukiVal1() == 1) {
			di.hasIceScythe = true;
		} else {
			di.hasFireScythe = true;
		}
	}
}

static void dizzyFunc_AwaPObj(DizzyInfo& di, ProjectileInfo& projectile) {
	if (projectile.ptr.linkObjectDestroyOnDamage() != nullptr) {
		di.hasBubble = true;
	}
}

static void dizzyFunc_AwaKObj(DizzyInfo& di, ProjectileInfo& projectile) {
	if (projectile.ptr.linkObjectDestroyOnDamage() != nullptr) {
		di.hasFireBubble = true;
	}
}

static void dizzyFunc_KinomiObj(DizzyInfo& di, ProjectileInfo& projectile) {
	if (projectile.ptr.linkObjectDestroyOnDamage() != nullptr && projectile.isDangerous) {
		di.hasIceSpear = true;
	}
}

static void dizzyFunc_KinomiObjNecro(DizzyInfo& di, ProjectileInfo& projectile) {
	if (!projectile.isDangerous) return;
	
	if (projectile.ptr.linkObjectDestroyOnDamage() != nullptr) {
		di.hasFireSpearHitstunLink = true;
	}
	if (projectile.ptr.hasUpon(BBSCREVENT_PLAYER_BLOCKED)) {
		BYTE* instr = moves.skipInstr(projectile.ptr.uponStruct(BBSCREVENT_PLAYER_BLOCKED)->uponInstrPtr);
		InstrType type = moves.instrType(instr);
		while (type != instr_endUpon) {
			if (type == instr_deactivateObj && asInstr(instr, deactivateObj)->entity == ENT_SELF) {
				static const char KinomiObjNecro[] = "KinomiObjNecro";
				char letter = projectile.animName[sizeof KinomiObjNecro - 1];
				if (letter == '\0') {
					di.hasFireSpear1BlockstunLink = true;
				} else if (letter == '2') {
					di.hasFireSpear2BlockstunLink = true;
				} else if (letter == '3') {
					di.hasFireSpear3BlockstunLink = true;
				}
				return;
			}
			instr = moves.skipInstr(instr);
			type = moves.instrType(instr);
		}
	}
}

static void dizzyFunc_KinomiObjNecrobomb(DizzyInfo& di, ProjectileInfo& projectile) {
	if(projectile.ptr.linkObjectDestroyOnDamage() != nullptr && projectile.isDangerous) {
		di.hasFireSpearExplosion = true;
	}
}

static void dizzyFunc_HanashiObjA(DizzyInfo& di, ProjectileInfo& projectile) {
	if(projectile.ptr.hasUpon(BBSCREVENT_PLAYER_GOT_HIT) && projectile.isDangerous) {
		di.hasPFish = true;
	}
}

static void dizzyFunc_HanashiObjB(DizzyInfo& di, ProjectileInfo& projectile) {
	if(projectile.ptr.hasUpon(BBSCREVENT_PLAYER_GOT_HIT) && projectile.isDangerous) {
		di.hasKFish = true;
	}
}

static void dizzyFunc_HanashiObjC(DizzyInfo& di, ProjectileInfo& projectile) {
	if(projectile.ptr.hasUpon(BBSCREVENT_PLAYER_GOT_HIT) && projectile.isDangerous) {
		di.hasSFish = true;
	}
}

static void dizzyFunc_HanashiObjD(DizzyInfo& di, ProjectileInfo& projectile) {
	if(projectile.ptr.hasUpon(BBSCREVENT_PLAYER_GOT_HIT) && projectile.isDangerous) {
		di.hasHFish = true;
	}
}

static void dizzyFunc_HanashiObjE(DizzyInfo& di, ProjectileInfo& projectile) {
	if(projectile.ptr.hasUpon(BBSCREVENT_PLAYER_GOT_HIT) && projectile.isDangerous) {
		di.hasDFish = true;
	}
}

static void dizzyFunc_Laser(DizzyInfo& di, ProjectileInfo& projectile) {
	if(projectile.ptr.linkObjectDestroyOnStateChange() != nullptr && projectile.isDangerous) {
		di.hasLaser = true;
	}
}

static void dizzyFunc_ImperialRayCreater(DizzyInfo& di, ProjectileInfo& projectile) {
	if(projectile.ptr.hasUpon(BBSCREVENT_PLAYER_GOT_HIT) && projectile.isDangerous) {
		di.hasBakuhatsuCreator = true;
	}
}

static void dizzyFunc_GammaRayLaser(DizzyInfo& di, ProjectileInfo& projectile) {
	if(projectile.ptr.linkObjectDestroyOnStateChange() != nullptr && projectile.isDangerous) {
		di.hasGammaRay = true;
	}
}

void EndScene::fillDizzyInfo(PlayerInfo& player, PlayerFrame& frame) {
	DizzyInfo& di = frame.u.dizzyInfo;
	
	di.hasIceSpike = false;  // hitstun only link, permanent
	di.hasFirePillar = false;  // hitstun only link, permanent
	di.hasIceScythe = false;  // hitstun only link, temporary
	di.hasFireScythe = false;  // hitstun only link, temporary
	di.hasBubble = false;  // hitstun only link, temporary
	di.hasFireBubble = false;  // hitstun only link, temporary
	di.hasIceSpear = false;  // hitstun only link, permanent
	// all 3 fire spears always disappear on hit, but on block they only disappear during the charge portion of each spear's animation
	di.hasFireSpearHitstunLink = false;
	di.hasFireSpear1BlockstunLink = false;
	di.hasFireSpear2BlockstunLink = false;
	di.hasFireSpear3BlockstunLink = false;
	di.hasFireSpearExplosion = false;  // explosions always disappear on hit
	di.hasPFish = false;  // hitstun only link, permanent
	di.hasKFish = false;  // hitstun only link, permanent
	di.hasSFish = false;  // hitstun only link, permanent
	di.hasHFish = false;  // hitstun only link, permanent
	di.hasDFish = false;  // hitstun only link, permanent
	di.hasLaser = false;  // permanently tied to state change of the parent fish
	di.hasBakuhatsuCreator = false;  // hitstun only link, permanent
	di.hasGammaRay = strcmp(player.anim, "GammaRay") == 0;  // tied to hitstun, permanently
	
	using dizzyCallback_t = void(*)(DizzyInfo& di, ProjectileInfo& projectile);
	
	struct MyHashFunction {
		inline std::size_t operator()(const char* k) const {
			return Entity::hashString(k);
		}
	};
	struct MyCompareFunction {
		inline bool operator()(const char* k, const char* other) const {
			return strcmp(k, other) == 0;
		}
	};
	
	static std::unordered_map<const char*, dizzyCallback_t, MyHashFunction, MyCompareFunction> dizzyMap;
	if (dizzyMap.empty()) {
		dizzyMap["SakanaObj"] = dizzyFunc_SakanaObj;
		dizzyMap["AkariObj"] = dizzyFunc_AkariObj;
		dizzyMap["AwaPObj"] = dizzyFunc_AwaPObj;
		dizzyMap["AwaKObj"] = dizzyFunc_AwaKObj;
		dizzyMap["KinomiObj"] = dizzyFunc_KinomiObj;
		dizzyMap["KinomiObjNecro"] = dizzyFunc_KinomiObjNecro;
		dizzyMap["KinomiObjNecro2"] = dizzyFunc_KinomiObjNecro;
		dizzyMap["KinomiObjNecro3"] = dizzyFunc_KinomiObjNecro;
		dizzyMap["KinomiObjNecrobomb"] = dizzyFunc_KinomiObjNecrobomb;
		dizzyMap["HanashiObjA"] = dizzyFunc_HanashiObjA;
		dizzyMap["HanashiObjB"] = dizzyFunc_HanashiObjB;
		dizzyMap["HanashiObjC"] = dizzyFunc_HanashiObjC;
		dizzyMap["HanashiObjD"] = dizzyFunc_HanashiObjD;
		dizzyMap["HanashiObjE"] = dizzyFunc_HanashiObjE;
		dizzyMap["Laser"] = dizzyFunc_Laser;
		dizzyMap["ImperialRayCreater"] = dizzyFunc_ImperialRayCreater;
		dizzyMap["GammaRayLaser"] = dizzyFunc_GammaRayLaser;
		dizzyMap["GammaRayLaserMax"] = dizzyFunc_GammaRayLaser;
	}
	
	for (ProjectileInfo& projectile : projectiles) {
		if (projectile.team == player.index) {
			const char* anim = projectile.ptr.animationName();
			auto found = dizzyMap.find(anim);
			if (found != dizzyMap.end()) {
				found->second(di, projectile);
			}
		}
	}
	
	di.shieldFishSuperArmor = player.dizzyShieldFishSuperArmor;
	
}
