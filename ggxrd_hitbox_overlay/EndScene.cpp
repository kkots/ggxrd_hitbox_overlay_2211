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

EndScene endScene;
PlayerInfo emptyPlayer {0};
ProjectileInfo emptyProjectile;

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
			&orig_WndProcMutex,
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
				&orig_drawTrainingHudMutex,
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
		logwrap(fputs("Failed to find things needed for enqueueing render commands\n"));
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
			&orig_REDAnywhereDispDrawMutex,
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
			&orig_drawQuadExecMutex,
			"drawQuadExec")) return false;
	}
	
	superflashInstigatorOffset = sigscanOffset(
		"GuiltyGearXrd.exe",
		"8b 86 ?? ?? ?? ?? 3b c3 74 09 ff 48 24 89 9e ?? ?? ?? ?? 89 be ?? ?? ?? ?? ff 47 24 8b 8f bc 01 00 00 49 89 8e ?? ?? ?? ?? 8b 97 c0 01 00 00 4a",
		{ 37, 0 },
		NULL, "superflashInstigatorOffset");
	if (superflashInstigatorOffset) {
		superflashCounterAllOffset = superflashInstigatorOffset + 4;
		superflashCounterSelfOffset = superflashInstigatorOffset + 8;
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
				&orig_##name##Mutex, \
				#name)) return false; \
		}
	
	digUpBBScrFunctionAndHook(BBScr_createObjectWithArg_t, BBScr_createObjectWithArg, 445, (const char*, unsigned int))
	digUpBBScrFunctionAndHook(BBScr_createParticleWithArg_t, BBScr_createParticleWithArg, 449, (const char*, unsigned int))
	digUpBBScrFunctionAndHook(BBScr_runOnObject_t, BBScr_runOnObject, 41, (int))
	digUpBBScrFunctionAndHook(BBScr_sendSignal_t, BBScr_sendSignal, 1766, (int, int))
	digUpBBScrFunctionAndHook(BBScr_sendSignalToAction_t, BBScr_sendSignalToAction, 1771, (const char*, int))
	digUpBBScrFunction(BBScr_callSubroutine_t, BBScr_callSubroutine, 17)
	digUpBBScrFunctionAndHook(BBScr_setHitstop_t, BBScr_setHitstop, 2263, (int))
	digUpBBScrFunctionAndHook(BBScr_ignoreDeactivate_t, BBScr_ignoreDeactivate, 298, ())
	
	if (orig_BBScr_sendSignal) {
		getReferredEntity = (getReferredEntity_t)followRelativeCall((uintptr_t)orig_BBScr_sendSignal + 5);
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
		}
	}
	
	if (orig_setAnim) {
		void (HookHelp::*setAnimHookPtr)(const char*) = &HookHelp::setAnimHook;
		if (!detouring.attach(&(PVOID&)orig_setAnim,
			(PVOID&)setAnimHookPtr,
			&orig_setAnimMutex,
			"setAnim")) return false;
	}
	
	if (orig_pawnInitialize) {
		void (HookHelp::*pawnInitializeHookPtr)(void*) = &HookHelp::pawnInitializeHook;
		if (!detouring.attach(&(PVOID&)orig_pawnInitialize,
			(PVOID&)pawnInitializeHookPtr,
			&orig_pawnInitializeMutex,
			"pawnInitialize")) return false;
	}
	
	if (orig_handleUpon) {
		void (HookHelp::*handleUponHookPtr)(int) = &HookHelp::handleUponHook;
		if (!detouring.attach(&(PVOID&)orig_handleUpon,
			(PVOID&)handleUponHookPtr,
			&orig_handleUponMutex,
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
			&orig_logicOnFrameAfterHitMutex,
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
			&orig_backPushbackApplierMutex,
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
			&orig_pushbackStunOnBlockMutex,
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
			&orig_skillCheckPieceMutex,
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
			&orig_beginHitstopMutex,
			"beginHitstop")) return false;
	}
	
	return !error;
}

bool EndScene::onDllDetach() {
	logwrap(fputs("EndScene::onDllDetach() called\n", logfile));
	shutdown = true;
	if (!logicThreadId || logicThreadId == detouring.dllMainThreadId) return true;
	HANDLE logicThreadHandle = OpenThread(THREAD_QUERY_INFORMATION, FALSE, logicThreadId);
	if (!logicThreadHandle) {
		WinError winErr;
		logwrap(fprintf(logfile, "EndScene failed to open logic thread handle: %ls\n", winErr.getMessage()));
		hud.onDllDetach();
		ui.onDllDetachNonGraphics();
		return false;
	}
	if (GetProcessIdOfThread(logicThreadHandle) != GetCurrentProcessId()) {
		CloseHandle(logicThreadHandle);
		logwrap(fprintf(logfile, "EndScene freeing resources on DLL thread, because thread is no longer alive"));
		hud.onDllDetach();
		ui.onDllDetachNonGraphics();
		return true;
	}
	DWORD exitCode;
	bool stillActive = GetExitCodeThread(logicThreadHandle, &exitCode) && exitCode == STILL_ACTIVE;
	CloseHandle(logicThreadHandle);
	
	if (!stillActive) {
		logwrap(fprintf(logfile, "EndScene freeing resources on DLL thread, because thread is no longer alive (2)"));
		hud.onDllDetach();
		ui.onDllDetachNonGraphics();
		return true;
	}
	
	logwrap(fputs("EndScene calling WaitForSingleObject\n", logfile));
	if (WaitForSingleObject(shutdownFinishedEvent, 300) == WAIT_OBJECT_0) {
		logwrap(fprintf(logfile, "EndScene freed resources successfully\n"));
		return true;
	}
	logwrap(fprintf(logfile, "EndScene freeing resources on DLL thread, because WaitForSingleObject did not return success"));
	hud.onDllDetach();
	ui.onDllDetachNonGraphics();
	return true;
}

// Runs on the main thread
void EndScene::HookHelp::drawTrainingHudHook() {
	HookGuard hookGuard("drawTrainingHud");
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
		bool isRunning = game.isMatchRunning() || altModes.roundendCameraFlybyType() != 8;
		entityList.populate();
		bool areAnimationsNormal = entityList.areAnimationsNormal();
		if (isNormalMode) {
			if (!isRunning || !areAnimationsNormal) {
				needToClearHitDetection = true;
			}
			else {
				prepareDrawData(&needToClearHitDetection);
			}
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
	logOnce(fputs("endSceneHook called\n", logfile));
	invisChipp.onEndSceneStart();
	drawnEntities.clear();

	noGravGifMode();

	logOnce(fprintf(logfile, "entity count: %d\n", entityList.count));

	DWORD aswEngineTickCount = *(DWORD*)(*aswEngine + 4 + game.aswEngineTickCountOffset);
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
	bool frameHasChanged = prevAswEngineTickCount != aswEngineTickCount;
	prevAswEngineTickCount = aswEngineTickCount;
	
	if (frameHasChanged) {
		bool needCatchEntities = false;
		for (int i = 0; i < 2; ++i) {
			PlayerInfo& player = players[i];
			if (!player.pawn) {
				player.index = i;
				initializePawn(player, entityList.slots[i]);
				needCatchEntities = true;
			}
		}
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
			if (ent.inPain() && !player.inPain) {
				comboStarted = true;
				break;
			}
		}
		
		for (int i = 0; i < 2; ++i) {
			PlayerInfo& player = players[i];
			PlayerInfo& other = players[1 - i];
			Entity ent = player.pawn;
			Entity otherEnt = other.pawn;
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
			player.inPain = ent.inPain();
			if (player.inPain || otherEnt.inPain()) {
				player.tensionGainLastCombo += tensionGain;
				player.burstGainLastCombo += burstGain;
			}
			if (player.tensionGainLastCombo > player.tensionGainMaxCombo) {
				player.tensionGainMaxCombo = player.tensionGainLastCombo;
			}
			if (player.burstGainLastCombo > player.burstGainMaxCombo) {
				player.burstGainMaxCombo = player.burstGainLastCombo;
			}
			player.receivedComboCountTensionGainModifier = entityManager.calculateReceivedComboCountTensionGainModifier(
				player.inPain,
				ent.comboCount());
			player.dealtComboCountTensionGainModifier = entityManager.calculateDealtComboCountTensionGainModifier(
				otherEnt.inPain(),
				otherEnt.comboCount());
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
			player.hitstun = ent.hitstun();
			int hitstop = ent.hitstop();
			int clashHitstop = ent.clashHitstop();
			if (!player.clashHitstop && clashHitstop) {
				player.hitstopMax = 0;
			}
			if (player.clashHitstop) {
				int val = clashHitstop + hitstop;
				if (val > player.hitstopMax) {
					player.hitstopMax = val;
				}
			}
			player.clashHitstop = clashHitstop;
			if (ent.hitSomethingOnThisFrame()) {
				player.hitstop = 0;
				player.hitstopMax = hitstop + clashHitstop - 1;
				if (player.hitstopMax < 0) player.hitstopMax = 0;
			} else if (player.armoredHitOnThisFrame && !player.gotHitOnThisFrame && player.setHitstopMaxSuperArmor) {
				player.hitstopMax = player.hitstopMaxSuperArmor - 1;
			} else {
				player.hitstop = hitstop + clashHitstop;
			}
			CmnActIndex cmnActIndex = ent.cmnActIndex();
			player.airborne = !(ent.y() == 0 && player.speedY == 0);
			player.wakeupTiming = 0;
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
			if (player.cmnActIndex == CmnActKirimomiUpper && ent.currentAnimDuration() == 2) {
				player.hitstunMax = player.hitstun;  // 5D6 hitstun
			}
			player.inBlockstunNextFrame = ent.inBlockstunNextFrame();
			const char* animName = ent.animationName();
			player.onTheDefensive = player.blockstun
				|| player.inPain
				|| cmnActIndex == CmnActUkemi
				|| ent.gettingUp();
			if (!player.obtainedForceDisableFlags) {
				player.wasForceDisableFlags = ent.forceDisableFlags();
			}
			player.enableBlock = ent.enableBlock();
			const MoveInfo& move = moves.getInfo(player.charType, ent.animationName(), false);
			bool idleNext = move.isIdle(player);
			bool prevFrameIdle = player.idle;
			bool prevFrameignoreNextInabilityToBlockOrAttack = player.ignoreNextInabilityToBlockOrAttack;
			bool prevFrameCanBlock = player.canBlock;
			player.canBlock = move.canBlock(player);
			
			if (!player.changedAnimOnThisFrame
					&& prevFrameIdle
					&& (
						// Leaving Ms. Confille stance after spending too much time in it doing nothing
						player.charType == CHARACTER_TYPE_ELPHELT
						&& (
							strcmp(player.anim, "Rifle_Start") == 0
							&& strcmp(animName, "Rifle_Start") == 0
							|| strcmp(player.anim, "Rifle_Reload") == 0
							&& strcmp(animName, "Rifle_Reload") == 0
							|| strcmp(player.anim, "Rifle_Reload_Perfect") == 0
							&& strcmp(animName, "Rifle_Reload_Perfect") == 0
							|| strcmp(player.anim, "Rifle_Roman") == 0
							&& strcmp(animName, "Rifle_Roman") == 0
						)
						&& !player.idle
					)
				) {
				player.changedAnimOnThisFrame = true;
			}
			if (!player.changedAnimOnThisFrame
					&& player.charType == CHARACTER_TYPE_LEO
					&& idleNext
					&& strcmp(player.anim, "Semuke") == 0
					&& strcmp(ent.gotoLabelRequest(), "SemukeEnd") == 0) {
				player.changedAnimOnThisFrame = true;
				player.canBlock = false;
				idleNext = false;
			}
			
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
			memcpy(player.attackLockAction, ent.attackLockAction(), 32);
			player.animFrame = ent.currentAnimDuration();
			player.hitboxesCount = 0;
			
			if (idleNext != player.idle || player.wasIdle && !idleNext || (player.setBlockstunMax || player.setHitstunMax) && !idleNext) {
				player.idle = idleNext;
				if (!player.idle) {
					if (player.onTheDefensive) {
						player.startedDefending = true;
						if (player.cmnActIndex != CmnActUkemi && !player.baikenReturningToBlockstunAfterAzami) {
							player.hitstopMax = player.hitstop;
							if (player.setBlockstunMax) {
								--player.blockstunMax;  // this is the line PlayerInfo.h:PlayerInfo::inBlockstunNextFrame refers to
							}
							if (player.setHitstunMax) {
								player.hitstunMax = ent.hitstun() - (ent.hitstop() ? 1 : 0);
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
			int playerval0 = ent.playerVal(0);
			int playerval1 = ent.playerVal(1);
			if (!player.playerval0 && playerval0) {
				player.maxDI = playerval1;
			}
			player.playerval1 = playerval1;
			player.playerval0 = playerval0;
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
					|| !player.canBlock  // block check also needed because 5D6 cannot be immediately FD canceled
					&& !player.ignoreNextInabilityToBlockOrAttack  // this check must always follow the canBlock check
				);
			bool needDisableProjectiles = false;
			if (player.changedAnimOnThisFrame) {
				player.move = &move;
				if (!move.preservesNewSection) {
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
				if (
						move.combineWithPreviousMove
						&& !move.usePlusSignInCombination
						&& !(
							player.charType == CHARACTER_TYPE_BEDMAN
							&& strcmp(player.anim, "Boomerang_B") != 0
							&& strcmp(player.anim, "Boomerang_B_Air") != 0
							&& strcmp(animName, "BWarp") == 0
						)
						&& !(
							player.charType == CHARACTER_TYPE_HAEHYUN
							&& strcmp(player.anim, "AntiAirAttack") != 0
							&& strcmp(animName, "AntiAir4Hasei") == 0
						)
						|| player.move == &moves.defaultMove
						&& *player.prevAttackLockAction != '\0'
						&& strcmp(animName, player.prevAttackLockAction) == 0
						|| player.charType == CHARACTER_TYPE_JAM
						&& strcmp(player.anim, "NeoHochihu") == 0  // 54625 causes NeoHochihu to cancel into itself
						&& strcmp(animName, "NeoHochihu") == 0
						|| player.isInFDWithoutBlockstun  // this check is here and not down below because I don't want air button +
						                                  // + recover in air + short FD + keep falling = to mess up the moveOriginatedInTheAir flag
						&& (
							!prevFrameIdle
							|| !prevFrameCanBlock
							&& !prevFrameignoreNextInabilityToBlockOrAttack
						)
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
					if (!player.isLandingOrPreJump
							&& player.cmnActIndex != CmnActJump
							&& !player.idlePlus
							&& !(
								!player.wasIdle
								&& player.cmnActIndex == CmnActRomanCancel
							)) {
						if (!player.baikenReturningToBlockstunAfterAzami) {
							player.displayHitstop = false;  // technically Baiken changes animation when she puts herself into blockstun from a successful Azami with no followup,
							                                // and normally we stop displaying old hitstop when the animation changes, but in this case we should keep it
						}
						if (!player.onTheDefensive) {
							player.xStunDisplay = PlayerInfo::XSTUN_DISPLAY_NONE;
						}
						player.moveOriginatedInTheAir = player.airborne;
						
						player.startupProj = 0;
						player.activesProj.clear();
						player.prevStartupsProj.clear();
						
						if (move.usePlusSignInCombination
								|| player.charType == CHARACTER_TYPE_BAIKEN
								&& strcmp(player.anim, "Suzuran") == 0
								&& (
									strcmp(animName, "Youshijin") == 0  // P followup
									|| strcmp(animName, "Mawarikomi") == 0  // K followup
									|| strcmp(animName, "Sakura") == 0  // S followup
									|| strcmp(animName, "Issen") == 0  // H followup
									|| strcmp(animName, "Teppou") == 0  // D followup
								)) {
							player.prevStartups.add(player.total);
						} else {
							player.prevStartups.clear();
						}
						
						player.startup = 0;
						player.startedUp = false;
						player.actives.clear();
						player.recovery = 0;
						player.total = 0;
						player.totalFD = 0;
						player.totalCanBlock = 0;
						player.ignoreNextInabilityToBlockOrAttack = false;
						
						player.dontRestartTheNonLandingFrameAdvantageCountdownUponNextLanding = false;
						
						player.landingRecovery = 0;
						player.superfreezeStartup = 0;
						if (player.cmnActIndex != CmnActRomanCancel) {
							needDisableProjectiles = true;
						}
					}
				}
				
				memcpy(player.anim, animName, 32);
				
			}
			
			if (player.idle && player.canBlock) {
				player.ignoreNextInabilityToBlockOrAttack = true;
			}
			
			// This is needed for animations that create projectiles on frame 1
			for (OccuredEvent& event : events) {
				if (event.type == OccuredEvent::SET_ANIM
						&& needDisableProjectiles
						&& event.u.setAnim.pawn == ent) {
					for (ProjectileInfo& projectile : projectiles) {
						if (projectile.team == player.index && projectile.lifeTimeCounter > 0) {
							projectile.disabled = true;
							projectile.startedUp = false;
							projectile.actives.clear();
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
						projectileTo.disabled = false;
						projectileTo.startup = player.total + player.prevStartups.total();  // + prevStartups.total() needed for Venom QV
						projectileTo.total = player.total + player.prevStartups.total();  // + prevStartups.total() needed for Venom QV
					} else if (projectileFrom.ptr
							&& projectileTo.ptr
							&& projectileFrom.ptr
							&& projectileFrom.team == player.index
							&& projectileTo.team == player.index) {
						projectileTo.disabled = false;
						projectileTo.startup = projectileFrom.total;
						projectileTo.total = projectileFrom.total;
					}
				}
			}
			
			if (!player.hitstop
					&& getSuperflashInstigator() == nullptr
					&& !player.startedUp
					&& !player.inNewMoveSection
					&& player.move
					&& player.move->sectionSeparator
					&& player.move->sectionSeparator(ent)) {
				player.inNewMoveSection = true;
				player.timeInNewSection = 0;
				if (!player.startedUp && player.total) {
					if (player.superfreezeStartup) {
						player.prevStartups.add(player.superfreezeStartup);
						player.total -= player.superfreezeStartup;  // needed for Johnny Treasure Hunt
						player.superfreezeStartup = 0;
					}
					if (player.total) {
						player.prevStartups.add(player.total);
					}
					player.startup = 0;
					player.total = 0;
					player.totalCanBlock = 0;
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
					}
					if (!other.frameAdvantageIncludesIdlenessInNewSection) {
						other.frameAdvantageIncludesIdlenessInNewSection = true;
						other.frameAdvantage -= player.timeInNewSection;
					}
					if (!other.eddie.frameAdvantageIncludesIdlenessInNewSection) {
						other.eddie.frameAdvantageIncludesIdlenessInNewSection = true;
						other.eddie.frameAdvantage -= player.timeInNewSection;
					}
					if (!player.airborne) {
						if (!player.landingFrameAdvantageIncludesIdlenessInNewSection) {
							player.landingFrameAdvantageIncludesIdlenessInNewSection = true;
							player.landingFrameAdvantage += player.timeInNewSection;
						}
						if (!other.landingFrameAdvantageIncludesIdlenessInNewSection) {
							other.landingFrameAdvantageIncludesIdlenessInNewSection = true;
							other.landingFrameAdvantage -= player.timeInNewSection;
						}
						if (!other.eddie.landingFrameAdvantageIncludesIdlenessInNewSection) {
							other.eddie.landingFrameAdvantageIncludesIdlenessInNewSection = true;
							other.eddie.landingFrameAdvantage -= player.timeInNewSection;
						}
					}
				}
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
				if (projectile.landedHit) {
					projectile.ptr = nullptr;
					projectile.markActive = true;
					++it;
					continue;
				}
				it = projectiles.erase(it);
			} else {
				projectile.fill(projectile.ptr);
				projectile.markActive = projectile.landedHit;
				if (projectile.team == 0 || projectile.team == 1) {
					projectile.move = &moves.getInfo(entityList.slots[projectile.team].characterType(), projectile.animName, true);
				} else {
					projectile.move = nullptr;
				}
				if (!projectile.inNewSection
						&& projectile.move
						&& projectile.move->sectionSeparator
						&& projectile.move->sectionSeparator(projectile.ptr)) {
					projectile.inNewSection = true;
					if (projectile.total) {
						projectile.prevStartups.add(projectile.total);
					}
					projectile.startup = 0;
					projectile.total = 0;
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
						} else {
							player.landingFrameAdvantage = -other.timePassedLanding;
						}
						other.landingFrameAdvantage = -player.landingFrameAdvantage;
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
						other.eddie.landingFrameAdvantageValid = false;
					}
				}
				if (player.idleInNewSection) {
					player.timePassedLanding = player.timeInNewSection;
				} else {
					player.timePassedLanding = 0;
				}
			}
		}
		
		// This is a separate loop because it depends on another player's timePassedLanding, which I changed in the previous loop
		for (PlayerInfo& player : players) {
			if (player.startedDefending) {
				restartMeasuringFrameAdvantage(player.index);
				restartMeasuringLandingFrameAdvantage(player.index);
			}
		}
		
		Entity superflashInstigator = getSuperflashInstigator();
		if (!superflashInstigator) {
			for (ProjectileInfo& projectile : projectiles) {
				if (!projectile.startedUp) {
					++projectile.startup;
				}
				++projectile.total;
			}
		}
	}
	
	for (int i = 0; i < entityList.count; i++) {
		Entity ent = entityList.list[i];
		int team = ent.team();
		if (!ent.isActive() || isEntityAlreadyDrawn(ent)) continue;

		bool active = ent.isActiveFrames();
		logOnce(fprintf(logfile, "drawing entity # %d. active: %d\n", i, (int)active));
		
		size_t hurtboxesPrevSize = drawDataPrepared.hurtboxes.size();
		size_t hitboxesPrevSize = drawDataPrepared.hitboxes.size();
		size_t pointsPrevSize = drawDataPrepared.points.size();
		size_t pushboxesPrevSize = drawDataPrepared.pushboxes.size();
		
		int* lastIgnoredHitNum = nullptr;
		if (i < 2 && (team == 0 || team == 1)) {
			PlayerInfo& player = players[team];
			lastIgnoredHitNum = &player.lastIgnoredHitNum;
		}
		int hitboxesCount = 0;
		DrawHitboxArrayCallParams hurtbox;
		collectHitboxes(ent, active, &hurtbox, &drawDataPrepared.hitboxes, &drawDataPrepared.points, &drawDataPrepared.pushboxes, &hitboxesCount, lastIgnoredHitNum);
		HitDetector::WasHitInfo wasHitResult = hitDetector.wasThisHitPreviously(ent, hurtbox);
		if (!wasHitResult.wasHit) {
			drawDataPrepared.hurtboxes.push_back({ false, hurtbox });
		}
		else {
			drawDataPrepared.hurtboxes.push_back({ true, hurtbox, wasHitResult.hurtbox });
		}
		logOnce(fputs("collectHitboxes(...) call successful\n", logfile));
		drawnEntities.push_back(ent);
		logOnce(fputs("drawnEntities.push_back(...) call successful\n", logfile));

		// Attached entities like dusts
		Entity attached = *(Entity*)(ent + 0x204);
		if (attached != nullptr) {
			logOnce(fprintf(logfile, "Attached entity: %p\n", attached.ent));
			collectHitboxes(attached, active, &hurtbox, &drawDataPrepared.hitboxes, &drawDataPrepared.points, &drawDataPrepared.pushboxes, &hitboxesCount, lastIgnoredHitNum);
			drawDataPrepared.hurtboxes.push_back({ false, hurtbox });
			drawnEntities.push_back(attached);
		}
		if ((team == 0 || team == 1) && frameHasChanged) {
			if (!hitboxesCount && i < 2) {
				PlayerInfo& player = players[team];
				if (player.hitSomething) ++hitboxesCount;
			}
			if (hitboxesCount) {
				if (i < 2) {
					PlayerInfo& player = players[team];
					player.hitboxesCount += hitboxesCount;
				} else if (frameHasChanged) {
					ProjectileInfo& projectile = findProjectile(ent);
					if (projectile.ptr) {
						projectile.markActive = true;
					}
				}
			}
		}
		if (invisChipp.needToHide(ent)) {
			drawDataPrepared.hurtboxes.resize(hurtboxesPrevSize);
			drawDataPrepared.hitboxes.resize(hitboxesPrevSize);
			drawDataPrepared.points.resize(pointsPrevSize);
			drawDataPrepared.pushboxes.resize(pushboxesPrevSize);
		}
	}
	
	logOnce(fputs("got past the entity loop\n", logfile));
	hitDetector.drawHits();
	logOnce(fputs("hitDetector.drawDetected() call successful\n", logfile));
	throws.drawThrows();
	logOnce(fputs("throws.drawThrows() call successful\n", logfile));
	
	if (frameHasChanged) {
		Entity superflashInstigator = getSuperflashInstigator();
		int instigatorTeam = -1;
		if (superflashInstigator) {
			instigatorTeam = superflashInstigator.team();
		}
		if (!superflashInstigator) {
			for (PlayerInfo& player : players) {
				if (!player.hitstop) {  // needed for Axl DP
					player.activesProj.beginMergeFrame();
				}
			}
		}
		for (ProjectileInfo& projectile : projectiles) {
			bool ignoreThisForPlayer = false;
			if (projectile.team == 0 || projectile.team == 1) {
				PlayerInfo& player = players[projectile.team];
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
				PlayerInfo& player = players[projectile.team];
				player.prevStartupsProj = projectile.prevStartups;
			}
			if (!projectile.markActive) {
				if (!superflashInstigator) {
					projectile.actives.addNonActive();
				}
			} else if (!superflashInstigator || !projectile.startedUp) {
				if (!projectile.startedUp) {
					if (!projectile.disabled && (projectile.team == 0 || projectile.team == 1) && !ignoreThisForPlayer) {
						PlayerInfo& player = players[projectile.team];
						if (!player.startupProj) {
							player.startupProj = projectile.startup;
						}
					}
					projectile.startedUp = true;
				}
				projectile.actives.addActive(projectile.hitNumber, 1);
				if (!projectile.disabled && (projectile.team == 0 || projectile.team == 1) && !ignoreThisForPlayer) {
					PlayerInfo& player = players[projectile.team];
					if (!player.hitstop) {
						if (superflashInstigator) {
							player.activesProj.addSuperfreezeActive(projectile.hitNumber);
						} else {
							player.activesProj.addActive(projectile.hitNumber, 1);
						}
					}
				}
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
					if (strcmp(projectile.animName, "ChouDoriru") == 0
							&& projectile.ptr
							&& projectile.team == player.index) {
						landminePtr = projectile.ptr;
						if (!player.eddie.landminePtr) {
							projectile.startup = player.eddie.total;
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
				if (created || strcmp(player.eddie.anim, projectile.animName) != 0) {
					memcpy(player.eddie.anim, projectile.animName, 32);
					
					if (created || !idleNext && player.eddie.ptr) {
						player.eddie.total = 0;
						player.eddie.startup = 0;
						player.eddie.startedUp = false;
						player.eddie.actives.clear();
						player.eddie.recovery = 0;
						player.eddie.frameAdvantageValid = false;
						player.eddie.landingFrameAdvantageValid = false;
						player.eddie.frameAdvantageIncludesIdlenessInNewSection = false;
						player.eddie.landingFrameAdvantageIncludesIdlenessInNewSection = false;
					}
					
					if (created) {
						player.eddie.hitstopMax = 0;
						player.eddie.startup = player.total;
						player.eddie.total = player.total;
						player.eddie.prevEnemyIdle = enemy.idlePlus;
						player.eddie.prevEnemyIdleLanding = enemy.idleLanding;
					}
				}
				
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
						player.eddie.actives.addActive(projectile.hitNumber, 1);
					}
					++player.eddie.total;
				}
				
				++player.eddie.timePassed;
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
					}
				} else if (!player.idlePlus && other.idlePlus) {
					if (measuringFrameAdvantage) {
						--player.frameAdvantage;
					}
				} else if (!player.idlePlus && !other.idlePlus) {
					if (measuringLandingFrameAdvantage == -1) {
						player.landingFrameAdvantageValid = false;
						other.landingFrameAdvantageValid = false;
					}
					player.frameAdvantage = 0;
					other.frameAdvantage = 0;
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
					} else if (!player.idleLanding && other.idleLanding) {
						--player.landingFrameAdvantage;
						++other.landingFrameAdvantage;
					} else if (!player.idleLanding && !other.idleLanding) {
						player.landingFrameAdvantage = 0;
						other.landingFrameAdvantage = 0;
					} else if (player.idleLanding && other.idleLanding) {
						measuringLandingFrameAdvantage = -1;
						player.landingFrameAdvantageValid = true;
						other.landingFrameAdvantageValid = true;
					}
				}
				
			}
			Entity ent = entityList.slots[i];
			bool hasHitboxes = player.hitboxesCount > 0;
			if (player.isInFDWithoutBlockstun) {
				++player.totalFD;
			} else if (!player.hitstop
					&& !(superflashInstigator && superflashInstigator != ent)
					&& (
						player.cmnActIndex == CmnActLandingStiff && !player.idle  // Ramlethal j.8D becomes "idle" on f6 of stiff landing
						|| !player.idle
						&& !player.airborne
						&& player.moveOriginatedInTheAir
					)
					&& !hasHitboxes
					&& !player.onTheDefensive  // Potemkin F.D.B. may cause a situation where, on hit, opponent is put into CmnActLockWait that begins airborne,
					                           // then transitions to the ground and, if this check wasn't here, it would be seen as landing animation (lol)
				) {
				++player.landingRecovery;
				player.totalFD = 0;
			} else if (!player.hitstop
					&& !(superflashInstigator && superflashInstigator != ent)
					&& player.cmnActIndex != CmnActJump
					&& player.cmnActIndex != CmnActJumpPre
					&& !player.isLanding) {
				if (
						(
							(!player.idlePlus
							|| player.idleInNewSection
							|| !player.canBlock)
							&& !player.ignoreNextInabilityToBlockOrAttack
						)
						&& !player.startedUp
						&& !superflashInstigator
					) {
					if (!player.idlePlus || player.idleInNewSection) {
						if (player.landingRecovery) {  // needed for Zato Shadow Gallery. Removing airborne check for Answer s.S
							player.startup += player.landingRecovery;
							player.total += player.landingRecovery;
							player.totalCanBlock += player.landingRecovery;
							player.landingRecovery = 0;
						}
						++player.startup;
						++player.total;
					}
					if (!player.canBlock) {
						++player.totalCanBlock;
					}
				}
				if (superflashInstigator == ent && !player.superfreezeStartup) {
					player.superfreezeStartup = player.total;
				}
				if (hasHitboxes) {
					if (!player.startedUp) {
						player.totalCanBlock = player.total;  // needed for Raven glide
						player.startedUp = true;
						player.inNewMoveSection = false;
						player.actives.addActive(ent.currentHitNum());
					} else if (!superflashInstigator) {
						if (player.landingRecovery) {
							player.recovery += player.landingRecovery;
							player.total += player.landingRecovery;
							player.totalCanBlock += player.landingRecovery;
							player.landingRecovery = 0;
						}
						if (player.recovery + player.landingRecovery) {  // needed for Venom H Mad Struggle
							player.actives.addNonActive(player.recovery + player.landingRecovery);
							player.recovery = 0;
							player.landingRecovery = 0;
						}
						player.actives.addActive(ent.currentHitNum());
						++player.total;
						if (!player.canBlock && !player.ignoreNextInabilityToBlockOrAttack) {
							++player.totalCanBlock;
						}
					}
				} else if (
						(
							(!player.idlePlus
							|| !player.canBlock)
							&& !player.ignoreNextInabilityToBlockOrAttack
						)
						&& !superflashInstigator
						&& player.startedUp
					) {
					if (player.landingRecovery && !player.idlePlus) {
						player.recovery += player.landingRecovery;
						player.total += player.landingRecovery;
						player.totalCanBlock += player.landingRecovery;
						player.landingRecovery = 0;
					}
					if (!player.idlePlus) {
						++player.total;
						++player.recovery;
					}
					if (!player.canBlock) {
						++player.totalCanBlock;
					}
				}
			}
		}
		
		for (PlayerInfo& player : players) {
			player.startupDisp = 0;
			player.activesDisp.clear();
			player.recoveryDisp = 0;
			player.totalDisp = 0;
			player.prevStartupsDisp.clear();
			player.prevStartupsTotalDisp.clear();
			if (player.startedUp && !player.startupProj) {
				player.prevStartupsDisp = player.prevStartups;
				player.prevStartupsTotalDisp = player.prevStartups;
				player.startupDisp = player.startup;
				player.activesDisp = player.actives;
				player.recoveryDisp = player.recovery;
				player.totalDisp = player.total;
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
				player.activesDisp = player.activesProj;
				int activesDispTotal = player.activesDisp.total();
				if (endOfActivesRelativeToPlayerTotalCountdownStart >= player.total) {
					player.recoveryDisp = 0;
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
			if (player.hitstun) {
				player.xStunDisplay = PlayerInfo::XSTUN_DISPLAY_HIT;
			} else if (player.blockstun) {
				player.xStunDisplay = PlayerInfo::XSTUN_DISPLAY_BLOCK;
			}
		}
		
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

// Draw boxes, without UI, and take a screenshot if needed
// Runs on the graphics thread
void EndScene::endSceneHook(IDirect3DDevice9* device) {
	graphics.graphicsThreadId = GetCurrentThreadId();
	graphics.onEndSceneStart(device);
	drawOutlineCallParamsManager.onEndSceneStart();
	camera.onEndSceneStart();

	bool doYourThing = !gifMode.hitboxDisplayDisabled;

	if (!*aswEngine) {
		// since we store pointers to hitbox data instead of copies of it, when aswEngine disappears those are gone and we get a crash if we try to read them
		graphics.drawDataUse.clear();
	}

	if (doYourThing) {
		if (graphics.drawDataUse.needTakeScreenshot && !settings.dontUseScreenshotTransparency) {
			logwrap(fputs("Running the branch with if (needToTakeScreenshot)\n", logfile));
			graphics.takeScreenshotMain(device, false);
		}
		graphics.drawAll();
		if (graphics.drawDataUse.needTakeScreenshot && settings.dontUseScreenshotTransparency) {
			graphics.takeScreenshotMain(device, true);
		}

	} else if (graphics.drawDataUse.needTakeScreenshot) {
		graphics.takeScreenshotMain(device, true);
	}
	graphics.drawDataUse.needTakeScreenshot = false;
}

// Runs on the main thread. Is called from WndProc hook
void EndScene::processKeyStrokes() {
	bool trainingMode = game.isTrainingMode();
	keyboard.updateKeyStatuses();
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
	if (!gifMode.modDisabled && (keyboard.gotPressed(settings.disableHitboxDisplayToggle) || stateChanged && ui.hitboxDisplayDisabled != gifMode.hitboxDisplayDisabled)) {
		if (gifMode.hitboxDisplayDisabled == true) {
			gifMode.hitboxDisplayDisabled = false;
			logwrap(fputs("Hitbox display enabled\n", logfile));
		} else {
			gifMode.hitboxDisplayDisabled = true;
			logwrap(fputs("Hitbox display disabled\n", logfile));
		}
		ui.hitboxDisplayDisabled = gifMode.hitboxDisplayDisabled;
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
	{
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
	game.freezeGame = (allowNextFrameIsHeld || freezeGame) && trainingMode && !gifMode.modDisabled;
	if (!trainingMode || gifMode.modDisabled) {
		gifMode.gifModeOn = false;
		ui.gifModeOn = false;
		gifMode.noGravityOn = false;
		ui.noGravityOn = false;
		game.slowmoGame = false;
		ui.slowmoGame = false;
		gifMode.gifModeToggleBackgroundOnly = false;
		ui.gifModeToggleBackgroundOnly = false;
		gifMode.gifModeToggleCameraCenterOnly = false;
		ui.gifModeToggleCameraCenterOnly = false;
		gifMode.gifModeToggleHideOpponentOnly = false;
		ui.gifModeToggleHideOpponentOnly = false;
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

// Runs on the main thread
void EndScene::noGravGifMode() {
	char playerIndex;
	playerIndex = game.getPlayerSide();
	if (playerIndex == 2) playerIndex = 0;

	bool useGifMode = (gifMode.gifModeOn || gifMode.gifModeToggleHideOpponentOnly) && game.isTrainingMode();
	if (useGifMode) {
		for (auto it = hiddenEntities.begin(); it != hiddenEntities.end(); ++it) {
			it->wasFoundOnThisFrame = false;
		}
		for (int i = 0; i < entityList.count; ++i) {
			Entity ent = entityList.list[i];
			if (ent.team() != playerIndex) {
				hideEntity(ent);
			}
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
	} else {
		for (int i = 0; i < entityList.count; ++i) {
			Entity ent = entityList.list[i];
			auto found = findHiddenEntity(ent);
			if (found != hiddenEntities.end()) {
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
			}
		}
		hiddenEntities.clear();
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
	HookGuard hookGuard("WndProc");
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
	}
	
	bool iLockedTheMutex = false;
	if (!orig_WndProcMutexLockedByWndProc) {
		orig_WndProcMutex.lock();
		orig_WndProcMutexLockedByWndProc = true;
		iLockedTheMutex = true;
	}
	LRESULT result = orig_WndProc(hWnd, message, wParam, lParam);
	if (iLockedTheMutex) {
		orig_WndProcMutex.unlock();
		orig_WndProcMutexLockedByWndProc = false;
	}
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
	{
		std::unique_lock<std::mutex> guard(orig_drawTrainingHudMutex);
		orig_drawTrainingHud(thisArg);
	}
	if (!shutdown && !graphics.shutdown) {
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
	drawDataPrepared.clear();
	clearContinuousScreenshotMode();
	measuringFrameAdvantage = false;
	measuringLandingFrameAdvantage = -1;
	memset(tensionGainOnLastHit, 0, sizeof tensionGainOnLastHit);
	memset(burstGainOnLastHit, 0, sizeof burstGainOnLastHit);
	memset(tensionGainOnLastHitUpdated, 0, sizeof tensionGainOnLastHitUpdated);
	memset(burstGainOnLastHitUpdated, 0, sizeof burstGainOnLastHitUpdated);
	registeredHits.clear();
	for (int i = 0; i < 2; ++i) {
		players[i].clear();
	}
	projectiles.clear();
	needFrameCleanup = false;
	creatingObject = false;
	sendSignalStack.clear();
	events.clear();
}

// When someone YRCs, PRCs, RRCs or does a super, the address of their entity is written into the
// *aswEngine + superflashInstigatorOffset variable for the duration of the superfreeze.
// Runs on the main thread
Entity EndScene::getSuperflashInstigator() {
	if (!superflashInstigatorOffset || !*aswEngine) return nullptr;
	return Entity{ *(char**)(*aswEngine + superflashInstigatorOffset) };
}

// Runs on the main thread
int EndScene::getSuperflashCounterAll() {
	if (!superflashCounterAllOffset || !*aswEngine) return 0;
	return *(int*)(*aswEngine + superflashCounterAllOffset);
}

// Runs on the main thread
int EndScene::getSuperflashCounterSelf() {
	if (!superflashCounterSelfOffset || !*aswEngine) return 0;
	return *(int*)(*aswEngine + superflashCounterSelfOffset);
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
	}
	player.landingFrameAdvantageIncludesIdlenessInNewSection = false;
	other.landingFrameAdvantageIncludesIdlenessInNewSection = false;
	measuringLandingFrameAdvantage = 1 - index;
}

// There're three hit detection calls: with hitDetectionType 0, 1 and 2, called
// one right after the other each logic tick.
// A single hit detection does a loop in a loop and tries to find which two entities hit each other.
// We use this hook at the start of hit detection algorithm to measure some values before a hit.
// Runs on the main thread
void EndScene::onHitDetectionStart(int hitDetectionType) {
	if (hitDetectionType == 0) {
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
		registeredHits.clear();
	}
}

// We use this hook at the end of hit detection algorithm to measure some values after a hit.
// Runs on the main thread
void EndScene::onHitDetectionEnd(int hitDetectionType) {
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
}

// Called by a hook inside hit detection when a hit was detected.
// Runs on the main thread
void EndScene::registerHit(HitResult hitResult, bool hasHitbox, Entity attacker, Entity defender) {
	registeredHits.emplace_back();
	RegisteredHit& hit = registeredHits.back();
	hit.projectile.fill(attacker);
	hit.isPawn = attacker.isPawn();
	hit.hitResult = hitResult;
	hit.hasHitbox = hasHitbox;
	hit.attacker = attacker;
	hit.defender = defender;
	if (defender.isPawn()) {
		PlayerInfo& defenderPlayer = findPlayer(defender);
		defenderPlayer.xStunDisplay = PlayerInfo::XSTUN_DISPLAY_NONE;
		if (hitResult == HIT_RESULT_ARMORED || hitResult == HIT_RESULT_5) {
			defenderPlayer.armoredHitOnThisFrame = true;
		} else if (hitResult == HIT_RESULT_NORMAL || hitResult == HIT_RESULT_BLOCKED) {
			defenderPlayer.gotHitOnThisFrame = true;
		}
	}
	if (attacker.isPawn() && hasHitbox) {
		PlayerInfo& attackerPlayer = findPlayer(attacker);
		attackerPlayer.hitSomething = true;
	}
	if (hasHitbox) {
		ProjectileInfo& projectile = findProjectile(attacker);
		if (projectile.ptr) {
			projectile.fill(attacker);
			projectile.hitstop = attacker.hitstop();
			projectile.landedHit = true;
			projectile.markActive = true;
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
			if (needToCallNoGravGifMode && game.isTrainingMode()) {
				entityList.populate();
				noGravGifMode();
			}
		}
		hud.onDllDetach();
		ui.onDllDetachNonGraphics();
		SetEvent(shutdownFinishedEvent);
	}
}

// Runs on the main thread
void EndScene::HookHelp::BBScr_createObjectWithArgHook(const char* animName, unsigned int posType) {
	HookGuard hookGuard("BBScr_createObjectWithArg");
	static bool insideObjectCreation = false;
	bool needUnset = !insideObjectCreation;
	if (!endScene.shutdown) {
		insideObjectCreation = true;
		endScene.creatingObject = true;
		endScene.createdObjectAnim = animName;
		endScene.creatorOfCreatedObject = Entity{(char*)this};
	}
	{
		static bool imHoldingTheLock = false;
		bool needUnlock = false;
		if (!imHoldingTheLock) {
			needUnlock = true;
			imHoldingTheLock = true;
			endScene.orig_BBScr_createObjectWithArgMutex.lock();
		}
		endScene.orig_BBScr_createObjectWithArg(this, animName, posType);
		if (needUnlock) {
			imHoldingTheLock = false;
			endScene.orig_BBScr_createObjectWithArgMutex.unlock();
		}
	}
	if (!endScene.shutdown) {
		if (needUnset) {
			insideObjectCreation = false;
			endScene.creatingObject = false;
		}
		if (!gifMode.modDisabled && (gifMode.gifModeToggleHideOpponentOnly || gifMode.gifModeOn) && game.isTrainingMode()) {
			int playerSide = game.getPlayerSide();
			if (playerSide == 2) playerSide = 0;
			Entity createdPawn = Entity{(char*)this}.previousEntity();
			if (createdPawn && createdPawn.team() != playerSide) {
				endScene.hideEntity(createdPawn);
			}
		}
	}
}

// Runs on the main thread
void EndScene::onObjectCreated(Entity pawn, Entity createdPawn, const char* animName) {
	for (auto it = projectiles.begin(); it != projectiles.end(); ++it) {
		if (it->ptr == createdPawn) {
			if (it->landedHit) {
				it->ptr = nullptr;
			} else {
				projectiles.erase(it);
			}
			break;
		}
	}
	projectiles.emplace_back();
	ProjectileInfo& projectile = projectiles.back();
	projectile.fill(createdPawn);
	bool ownerFound = false;
	ProjectileInfo& creatorProjectile = findProjectile(pawn);
	if (creatorProjectile.ptr) {
		projectile.startup = creatorProjectile.total;
		projectile.total = creatorProjectile.total;
		projectile.disabled = creatorProjectile.disabled;
		ownerFound = true;
	}
	if (!ownerFound && pawn.isPawn()) {
		entityList.populate();
		PlayerInfo& player = findPlayer(pawn);
		projectile.startup = player.total + player.prevStartups.total();  // + prevStartups.total() needed for Venom QV
		projectile.total = player.total + player.prevStartups.total();  // + prevStartups.total() needed for Venom QV
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
	HookGuard hookGuard("setAnim");
	endScene.setAnimHook(Entity{(char*)this}, animName);
}

// Runs on the main thread
// This hook does not work on projectiles
void EndScene::setAnimHook(Entity pawn, const char* animName) {
	if (!shutdown && pawn.isPawn() && !gifMode.modDisabled) {
		PlayerInfo& player = findPlayer(pawn);
		if (player.pawn) {  // The hook may run on match load before we initialize our pawns,
			                // because we only do our pawn initialization after the end of the logic tick
			player.lastIgnoredHitNum = -1;
			player.changedAnimOnThisFrame = true;
			const MoveInfo& move = moves.getInfo(player.charType, player.animIntraFrame, false);
			memcpy(player.animIntraFrame, animName, 32);
			if (move.isIdle(player)) player.wasIdle = true;
			events.emplace_back();
			OccuredEvent& event = events.back();
			event.type = OccuredEvent::SET_ANIM;
			event.u.setAnim.pawn = pawn;
		}
	}
	{
		std::unique_lock<std::mutex> guard(orig_setAnimMutex);
		orig_setAnim((void*)pawn, animName);
	}
	if (!shutdown && pawn.isPawn() && !gifMode.modDisabled) {
		PlayerInfo& player = findPlayer(pawn);
		int blockstun = pawn.blockstun();
		if (blockstun && (player.inBlockstunNextFrame || player.baikenReturningToBlockstunAfterAzami)) {
			// defender was observed to not be in hitstop at this point, but having blockstun nonetheless
			player.blockstunMax = blockstun;
			player.setBlockstunMax = true;
		}
	}
}

// Runs on the main thread
void EndScene::initializePawn(PlayerInfo& player, Entity ent) {
	player.pawn = ent;
	player.charType = ent.characterType();
	ent.getWakeupTimings(&player.wakeupTimings);
	player.idle = moves.getInfo(player.charType, ent.animationName(), false).isIdle(player);
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
	HookGuard hookGuard("BBScr_createParticleWithArg");
	endScene.BBScr_createParticleWithArgHook(Entity{(char*)this}, animName, posType);
}

// Runs on the main thread
void EndScene::BBScr_createParticleWithArgHook(Entity pawn, const char* animName, unsigned int posType) {
	if (!gifMode.modDisabled && (gifMode.gifModeToggleHideOpponentOnly || gifMode.gifModeOn) && game.isTrainingMode() && pawn.isPawn()) {
		int playerSide = game.getPlayerSide();
		if (playerSide == 2) playerSide = 0;
		if (pawn.team() != playerSide) {
			pawn.scaleForParticles() = 0;
		}
	}
	{
		std::unique_lock<std::mutex> guard(orig_BBScr_createParticleWithArgMutex);
		orig_BBScr_createParticleWithArg((void*)pawn, animName, posType);
	}
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
		player.setHitstopMax = false;
		player.setHitstopMaxSuperArmor = false;
		player.setHitstunMax = false;
		player.setBlockstunMax = false;
		player.wasIdle = false;
		player.startedDefending = false;
		player.hitSomething = false;
		player.changedAnimOnThisFrame = false;
		player.wasEnableGatlings = false;
		player.wasEnableNormals = false;
		player.wasEnableWhiffCancels = false;
		player.obtainedForceDisableFlags = false;
		player.armoredHitOnThisFrame = false;
		player.gotHitOnThisFrame = false;
		player.baikenReturningToBlockstunAfterAzami = false;
		memcpy(player.animIntraFrame, player.anim, 32);
	}
	creatingObject = false;
	events.clear();
}

// Runs on the main thread
void EndScene::HookHelp::pawnInitializeHook(void* initializationParams) {
	HookGuard hookGuard("pawnInitialize");
	endScene.pawnInitializeHook(Entity{(char*)this}, initializationParams);
}

// Runs on the main thread
void EndScene::HookHelp::handleUponHook(int signal) {
	HookGuard hookGuard("handleUpon");
	endScene.handleUponHook(Entity{(char*)this}, signal);
}

// Runs on the main thread
void EndScene::pawnInitializeHook(Entity createdObj, void* initializationParams) { 
	if (!shutdown && creatingObject) {
		creatingObject = false;
		onObjectCreated(creatorOfCreatedObject, createdObj, createdObjectAnim);
		events.emplace_back();
		OccuredEvent& event = events.back();
		event.type = OccuredEvent::SIGNAL;
		event.u.signal.from = creatorOfCreatedObject;
		event.u.signal.to = createdObj;
	}
	{
		static bool imHoldingTheLock = false;
		bool needUnlock = false;
		if (!imHoldingTheLock) {
			needUnlock = true;
			imHoldingTheLock = true;
			endScene.orig_pawnInitializeMutex.lock();
		}
		endScene.orig_pawnInitialize(createdObj.ent, initializationParams);
		if (needUnlock) {
			imHoldingTheLock = false;
			endScene.orig_pawnInitializeMutex.unlock();
		}
	}
}

// Runs on the main thread
void EndScene::handleUponHook(Entity pawn, int signal) { 
	if (!shutdown) {
		if (!sendSignalStack.empty()) {
			events.emplace_back();
			OccuredEvent& event = events.back();
			event.type = OccuredEvent::SIGNAL;
			event.u.signal.from = sendSignalStack.back();
			event.u.signal.to = pawn;
		}
	}
	{
		static bool imHoldingTheLock = false;
		bool needUnlock = false;
		if (!imHoldingTheLock) {
			needUnlock = true;
			imHoldingTheLock = true;
			endScene.orig_handleUponMutex.lock();
		}
		endScene.orig_handleUpon((void*)pawn.ent, signal);
		if (needUnlock) {
			imHoldingTheLock = false;
			endScene.orig_handleUponMutex.unlock();
		}
	}
}

// Runs on the main thread
void EndScene::HookHelp::logicOnFrameAfterHitHook(bool isAirHit, int param2) {
	HookGuard hookGuard("logicOnFrameAfterHit");
	endScene.logicOnFrameAfterHitHook(Entity{(char*)this}, isAirHit, param2);
}

// Runs on the main thread
void EndScene::logicOnFrameAfterHitHook(Entity pawn, bool isAirHit, int param2) {
	bool functionWillNotDoAnything = !pawn.inPainNextFrame() && (!pawn.enableGuardBreak() || !pawn.inBlockstunNextFrame());
	{
		std::unique_lock<std::mutex> guard(orig_logicOnFrameAfterHitMutex);
		orig_logicOnFrameAfterHit((void*)pawn.ent, isAirHit, param2);
	}
	if (pawn.isPawn() && !functionWillNotDoAnything) {
		PlayerInfo& player = findPlayer(pawn);
		player.hitstopMax = pawn.startingHitstop();
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
			pawn.ignoreWeight(),
			pawn.dontUseComboTimerForSpeedY(),
			&player.receivedSpeedYWeight,
			&player.receivedSpeedYComboProration);
		entityManager.calculateHitstunProration(
			pawn.noHitstunScaling(),
			isAirHit,
			pawn.comboTimer(),
			&player.hitstunProration);
		entityManager.calculatePushback(
			pawn.receivedAttackLevel(),
			pawn.comboTimer(),
			pawn.dontUseComboTimerForPushback(),
			pawn.ascending(),
			pawn.y(),
			pawn.pushbackModifier(),
			pawn.airPushbackModifier(),
			true,
			pawn.pushbackModifierDuringPain(),
			&player.basePushback,
			&player.attackPushbackModifier,
			&player.painPushbackModifier,
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
	HookGuard hookGuard("BBScr_runOnObject");
	endScene.BBScr_runOnObjectHook(Entity{(char*)this}, entityReference);
}

// Runs on the main thread
void EndScene::BBScr_runOnObjectHook(Entity pawn, int entityReference) {
	{
		std::unique_lock<std::mutex> guard(orig_BBScr_runOnObjectMutex);
		orig_BBScr_runOnObject((void*)pawn.ent, entityReference);
	}
	if (!shutdown && pawn.isPawn()) {
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
				}
			}
		}
	}
}

// Runs on the main thread
void EndScene::REDAnywhereDispDrawHookStatic(void* canvas, FVector2D* screenSize) {
	HookGuard hookGuard("REDAnywhereDispDraw");
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
	drawData.~DrawData();
	FRenderCommand::Destructor(freeMem);
}
// Runs on the main thread
DrawBoxesRenderCommand::DrawBoxesRenderCommand() {
	drawData.clear();
	endScene.drawDataPrepared.copyTo(&drawData);
	camera.valuesPrepare.copyTo(cameraValues);
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
	drawData.~vector<BYTE>();
	FRenderCommand::Destructor(freeMem);
}
// Runs on the main thread
DrawImGuiRenderCommand::DrawImGuiRenderCommand() {
	ui.copyDrawDataTo(drawData);
	iconsUTexture2D = endScene.getIconsUTexture2D();
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
	if (!shutdown && !graphics.shutdown) {
		drawDataPrepared.clear();
		lastScreenSize = *screenSize;
		if (*aswEngine) {
			logic();
			if (!gifMode.modDisabled) {
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
						drawDataPrepared.clear();
					}
				} else {
					for (int i = 0; i < 2 && i < entityList.count; ++i) {
						instigator = entityList.slots[i];
						if (instigator.characterType() == CHARACTER_TYPE_RAVEN
								&& strcmp(instigator.animationName(), "RevengeAttackEx") == 0  // 214214H super
								&& instigator.currentAnimDuration() >= 19 && instigator.currentAnimDuration() < 139) {
							drawDataPrepared.clear();
							break;
						}
					}
				}
				if (!camera.grabbedValues) {
					camera.grabValues();
				}
			}
		}
		enqueueRenderCommand<DrawBoxesRenderCommand>();
		if (!gifMode.modDisabled && (
				!gifMode.hitboxDisplayDisabled && !drawDataPrepared.points.empty()
				|| !getIconsUTexture2D())) {
			queueOriginPointDrawingDummyCommandAndInitializeIcon();
		}
	}
	{
		std::unique_lock<std::mutex> guard;
		orig_REDAnywhereDispDraw(canvas, screenSize);  // calls drawQuadExecHook
	}
	
	if (!shutdown && !graphics.shutdown) {
		ui.drawData = nullptr;
		ui.prepareDrawData();
		if (ui.drawData) {
			FCanvas_Flush(canvas, 0);  // for things to be drawn on top of anything drawn so far, need to flush canvas, otherwise some
			                           // items might still be drawn on top of yours
			enqueueRenderCommand<DrawImGuiRenderCommand>();
		}
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
void EndScene::executeDrawBoxesRenderCommand(DrawBoxesRenderCommand* command) {
	if (endScene.shutdown || graphics.shutdown) return;
	graphics.drawDataUse.clear();
	command->drawData.copyTo(&graphics.drawDataUse);
	command->cameraValues.copyTo(camera.valuesUse);
	endSceneHook(getDevice());
}

// Runs on the graphics thread
void EndScene::executeDrawOriginPointsRenderCommand(DrawOriginPointsRenderCommand* command) {
	if (endScene.shutdown || graphics.shutdown) return;
	if (gifMode.hitboxDisplayDisabled) return;
	graphics.onlyDrawPoints = true;
	graphics.drawAll();
	graphics.onlyDrawPoints = false;
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
    s.x = -615530.F;  // gets converted into -615529.0F, or 0xC9164690
    s.y = 0.F;
    s.alignment = ALIGN_LEFT;
    s.text = plusSign;
    s.field156_0xf4 = 0x010;
    drawTextWithIcons(&s,0x0,1,4,0,0);
}

// Runs on the main thread. Called from orig_REDAnywhereDispDraw
void drawQuadExecHook(FVector2D* screenSize, REDDrawQuadCommand* item, void* canvas) {
	endScene.drawQuadExecHook(screenSize, item, canvas);
}

// Runs on the main thread. Called from ::drawQuadExecHook
void EndScene::drawQuadExecHook(FVector2D* screenSize, REDDrawQuadCommand* item, void* canvas) {
	if (item->count == 4 && (unsigned int&)item->vertices[0].x == 0xC9164690) {  // avoid floating point comparison as it may be slower
		if (!shutdown
				&& !graphics.shutdown
				&& !drawDataPrepared.points.empty()
				&& !gifMode.hitboxDisplayDisabled
				&& !gifMode.modDisabled) {
			FCanvas_Flush(canvas, 0);
			enqueueRenderCommand<DrawOriginPointsRenderCommand>();
		}
		return;  // can safely omit items
	}
	{
		std::unique_lock<std::mutex> guard(orig_drawQuadExecMutex);
		call_orig_drawQuadExec(orig_drawQuadExec, screenSize, item, canvas);
	}
}

// Runs on the main thread
BYTE* EndScene::getIconsUTexture2D() {
	return *(BYTE**)iconTexture;
}

// Runs on the graphics thread
void EndScene::executeDrawImGuiRenderCommand(DrawImGuiRenderCommand* command) {
	if (shutdown || graphics.shutdown) return;
	IDirect3DTexture9* tex = getTextureFromUTexture2D(command->iconsUTexture2D);
	ui.onEndScene(getDevice(), command->drawData.data(), tex);
}

// Runs on the graphics thread
void EndScene::executeShutdownRenderCommand() {
	if (!graphics.shutdown) return;
	graphics.onShutdown();
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
	HookGuard hookGuard("backPushbackApplier");
	endScene.backPushbackApplierHook((char*)this);
}

void EndScene::backPushbackApplierHook(char* thisArg) {
	entityList.populate();
	for (int i = 0; i < 2; ++i) {
		PlayerInfo& player = players[i];
		player.fdPushback = entityList.slots[i].fdPushback();
		// it changes after it was used, so if we don't do this, at the end of a frame we will see a decremented value
	}
	{
		std::unique_lock<std::mutex> guard(orig_backPushbackApplierMutex);
		orig_backPushbackApplier((void*)thisArg);
	}
}

void EndScene::HookHelp::pushbackStunOnBlockHook(bool isAirHit) {
	HookGuard hookGuard("pushbackStunOnBlock");
	endScene.pushbackStunOnBlockHook(Entity{(char*)this}, isAirHit);
}

void EndScene::pushbackStunOnBlockHook(Entity pawn, bool isAirHit) {
	{
		std::unique_lock<std::mutex> guard(orig_pushbackStunOnBlockMutex);
		orig_pushbackStunOnBlock((void*)pawn.ent, isAirHit);
	}
	PlayerInfo& player = findPlayer(pawn);
	player.ibPushbackModifier = 100;
	char* trainingStruct = game.getTrainingHud();
	bool isDummy = isDummyPtr(trainingStruct, pawn.team());
	bool fd = !isDummy ? pawn.holdingFD() : *(DWORD*)(trainingStruct + 0x670 + 4 * pawn.team()) == 2;
	Entity attacker = pawn.attacker();
	PlayerInfo& attackerPlayer = findPlayer(attacker);
	if (attacker) {
		attackerPlayer.baseFdPushback = 0;
	}
	if (fd) {
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
		pawn.receivedAttackLevel(),
		pawn.comboTimer(),
		pawn.dontUseComboTimerForPushback(),
		pawn.ascending(),
		pawn.y(),
		pawn.pushbackModifier(),
		pawn.airPushbackModifier(),
		pawn.inPain(),
		pawn.pushbackModifierDuringPain(),
		&player.basePushback,
		&player.attackPushbackModifier,
		&player.painPushbackModifier,
		&player.comboTimerPushbackModifier);
	player.hitstunProrationValid = false;
}

void EndScene::HookHelp::BBScr_sendSignalHook(int referenceType, int signal) {
	HookGuard hookGuard("BBScr_sendSignal");
	endScene.BBScr_sendSignalHook(Entity{(char*)this}, referenceType, signal);
}

void EndScene::HookHelp::BBScr_sendSignalToActionHook(const char* searchAnim, int signal) {
	HookGuard hookGuard("BBScr_sendSignalToAction");
	endScene.BBScr_sendSignalToActionHook(Entity{(char*)this}, searchAnim, signal);
}

void EndScene::BBScr_sendSignalHook(Entity pawn, int referenceType, int signal) {
	Entity referredEntity = getReferredEntity((void*)pawn.ent, referenceType);
	
	bool isDizzyBubblePopping = referredEntity == pawn && signal == 0x17  // HIT_OR_BLOCK
			&& (strcmp(pawn.animationName(), "AwaPObj") == 0  // Dizzy bubble
				|| strcmp(pawn.animationName(), "AwaKObj") == 0);
	
	if (!shutdown && referredEntity && !isDizzyBubblePopping) {
		ProjectileInfo& projectile = findProjectile(referredEntity);
		int team = pawn.team();
		if (projectile.ptr && (team == 0 || team == 1)) {
			events.emplace_back();
			OccuredEvent& event = events.back();
			event.type = OccuredEvent::SIGNAL;
			event.u.signal.from = pawn;
			event.u.signal.to = projectile.ptr;
		}
	}
	{
		static bool isLocked = false;
		bool needUnlock = !isLocked;
		isLocked = true;
		std::unique_lock<std::mutex> guard;
		if (needUnlock) {
			guard = std::unique_lock<std::mutex>(orig_BBScr_sendSignalMutex);
		}
		orig_BBScr_sendSignal((void*)pawn, referenceType, signal);
		if (needUnlock) {
			isLocked = false;
		}
	}
}

void EndScene::BBScr_sendSignalToActionHook(Entity pawn, const char* searchAnim, int signal) {
	if (!shutdown) {
		sendSignalStack.push_back(pawn);
	}
	{
		static bool isLocked = false;
		bool needUnlock = !isLocked;
		isLocked = true;
		std::unique_lock<std::mutex> guard;
		if (needUnlock) {
			guard = std::unique_lock<std::mutex>(orig_BBScr_sendSignalToActionMutex);
		}
		orig_BBScr_sendSignalToAction((void*)pawn, searchAnim, signal);
		if (needUnlock) {
			isLocked = false;
		}
	}
	if (!shutdown) {
		sendSignalStack.pop_back();
	}
}

BOOL EndScene::HookHelp::skillCheckPieceHook() {
	HookGuard hookGuard("skillCheckPiece");
	return endScene.skillCheckPieceHook(Entity{(char*)this});
}

BOOL EndScene::skillCheckPieceHook(Entity pawn) {
	BOOL result;
	{
		std::unique_lock<std::mutex> guard(orig_skillCheckPieceMutex);
		result = orig_skillCheckPiece((void*)pawn.ent);
	}
	PlayerInfo& player = findPlayer(pawn);
	player.wasEnableNormals = pawn.enableNormals();
	player.wasEnableGatlings = pawn.enableGatlings();
	player.wasEnableWhiffCancels = pawn.enableWhiffCancels();
	player.wasForceDisableFlags = pawn.forceDisableFlags();
	player.obtainedForceDisableFlags = true;
	return result;
}

void EndScene::HookHelp::BBScr_setHitstopHook(int hitstop) {
	HookGuard hookGuard("BBScr_setHitstop");
	endScene.BBScr_setHitstopHook(Entity{(char*)this}, hitstop);
}

void EndScene::BBScr_setHitstopHook(Entity pawn, int hitstop) {
	if (!shutdown) {
		PlayerInfo& player = findPlayer(pawn);
		player.hitstopMax = hitstop;
		player.setHitstopMax = true;
		player.setHitstopMaxSuperArmor = false;
	}
	{
		std::unique_lock<std::mutex> guard(orig_BBScr_setHitstopMutex);
		orig_BBScr_setHitstop((void*)pawn.ent, hitstop);
	}
}

void EndScene::HookHelp::beginHitstopHook() {
	HookGuard hookGuard("beginHitstop");
	endScene.beginHitstopHook(Entity{(char*)this});
}

void EndScene::beginHitstopHook(Entity pawn) {
	if (pawn.needSetHitstop() && pawn.startingHitstop() != 0) {
		PlayerInfo& player = findPlayer(pawn);
		player.hitstopMaxSuperArmor = pawn.startingHitstop();
		player.setHitstopMaxSuperArmor = true;
	}
	{
		std::unique_lock<std::mutex> guard(orig_beginHitstopMutex);
		orig_beginHitstop((void*)pawn.ent);
	}
}

void EndScene::HookHelp::BBScr_ignoreDeactivateHook() {
	HookGuard hookGuard("BBScr_ignoreDeactivate");
	endScene.BBScr_ignoreDeactivateHook(Entity{(char*)this});
}

void EndScene::BBScr_ignoreDeactivateHook(Entity pawn) {
	{
		std::unique_lock<std::mutex> guard(orig_BBScr_ignoreDeactivateMutex);
		orig_BBScr_ignoreDeactivate((void*)pawn.ent);
	}
	PlayerInfo& player = findPlayer(pawn);
	player.baikenReturningToBlockstunAfterAzami = true;
}

void EndScene::onBubblePop(Entity bubble) {
	endScene.events.emplace_back();
	OccuredEvent& event = events.back();
	event.type = OccuredEvent::SIGNAL;
	event.u.signal.from = players[bubble.team()].pawn;
	event.u.signal.to = bubble;
}
