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
#include "pi.h"
#include "ReadWholeFile.h"
#include "EndSceneRepeatingStuff.h"

EndScene endScene;
PlayerInfo emptyPlayer {0};
ProjectileInfo emptyProjectile;
findMoveByName_t findMoveByName = nullptr;
appFree_t appFree = nullptr;

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

extern "C" DWORD restoreDoubleJumps = 0;  // for use by jumpInstallNormalJumpHookAsm
extern "C" void jumpInstallNormalJumpHookAsm(void* pawn);  // defined in asmhooks.asm
extern "C" void __fastcall jumpInstallNormalJumpHook(void* pawn);  // defined here
extern "C" DWORD restoreAirDash = 0;  // for use by jumpInstallSuperJumpHookAsm
extern "C" void jumpInstallSuperJumpHookAsm(void* pawn);  // defined in asmhooks.asm
extern "C" void __fastcall jumpInstallSuperJumpHook(void* pawn);  // defined here
extern "C" void activeFrameHitReflectHookAsm(void* ent, int percentage);  // defined in asmhooks.asm
extern "C" void __cdecl activeFrameHitReflectHook(void* attacker, void* defender, int percentage);  // defined here
extern "C" void obtainingOfCounterhitTrainingSettingHookAsm(void* trainingStruct /* this argument */,
		TrainingSettingId settingId, BOOL outsideTraining);  // defined in asmhooks.asm
// the defender here is typecast into Player. It is 0 for Projectiles
extern "C" int __cdecl obtainingOfCounterhitTrainingSettingHook(void* defender, void* trainingStruct,
		TrainingSettingId settingId, BOOL outsideTraining);  // defined here
extern "C" DWORD orig_isGameModeNetwork = 0;
extern "C" BOOL __cdecl isGameModeNetworkHookWhenDecidingStepCountHookAsm();  // defined in asmhooks.asm
// returns ticks to perform
extern "C" int __cdecl isGameModeNetworkHookWhenDecidingStepCountHook();  // defined here
extern "C" DWORD orig_replayPauseControlTick = 0;
// this function is actually a __thiscall! But they have mangled names that I don't remember/want to look up how to write so I denoted this as __cdecl for convenience
extern "C" void __cdecl replayPauseControlTickHookAsm();  // defined in asmhooks.asm

const unsigned char greenHighlights[] { 255, 255, 255, 255, 255, 242, 228, 200, 165, 125, 82, 67, 55, 44, 33, 22, 17, 13, 9, 7, 4 };

const PunishFrame punishAnim[] { { -10, 0x00FFFFFF }, { -10, 0x11FFFFFF }, { -10, 0x33FFFFFF }, { -9, 0x66FFFFFF }, { -9, 0x99FFFFFF },
	{ -9, 0xCCFFFFFF }, { -8, 0xDDFFFFFF }, { -8, 0xEEFFFFFF }, { -7, 0xFFFFFFFF }, { -7, 0xFFFFFFEE }, { -6, 0xFFFFFFDD }, { -6, 0xFFFFFFBB },
	{ -5, 0xFFFFFF99 }, { -4, 0xFFFFFF66 }, { -4, 0xFFFFFF3A }, { -4, 0xFFFFFF28 }, { -3, 0xFFFFFF1A }, { -3, 0xFFFFFF0F }, { -3, 0xFFFFFF00 },
	{ -2, 0xFFFFFF00 }, { -2, 0xFFFFFF00 }, { -2, 0xFFFFFF00 }, { -2, 0xFFFFFF00 }, { -1, 0xFFFFFF00 }, { -1, 0xFFFFFF00 }, { -1, 0xFFFFFF00 },
	{ -1, 0xFFFFFF00 }, { -1, 0xFFFFFF00 }, { 0, 0xFFFFFF00, 60 * 3 }, { 0, 0xF0FFFF00 }, { 0, 0xE0FFFF00 }, { 0, 0xD4FFFF00 }, { 0, 0xC0FFFF00 },
	{ 0, 0xAFFFFF00 }, { 0, 0x90FFFF00 }, { 0, 0x70FFFF00 }, { 0, 0x50FFFF00 }, { 0, 0x40FFFF00 }, { 0, 0x30FFFF00 }, { 0, 0x24FFFF00 },
	{ 0, 0x18FFFF00 }, { 0, 0x14FFFF00 }, { 0, 0x10FFFF00 }, { 0, 0x0CFFFF00 }, { 0, 0x08FFFF00 }, { 0, 0x04FFFF00 } };

ProjectileFramebar defaultFramebar { -1, INT_MAX };

bool EndScene::onDllMain() {
	bool error = false;
	
	stateRingBuffer.resize(1);
	currentState = stateRingBuffer.data();
	defaultFramebar.stateHead = defaultFramebar.states;
	defaultFramebar.main.stateHead = defaultFramebar.main.states;
	defaultFramebar.hitstop.stateHead = defaultFramebar.hitstop.states;
	defaultFramebar.idle.stateHead = defaultFramebar.idle.states;
	defaultFramebar.idleHitstop.stateHead = defaultFramebar.idleHitstop.states;
	
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
			/*orig_drawTrainingHud = game.trainingHudTick;
			logwrap(fprintf(logfile, "orig_drawTrainingHud final location: %p\n", (void*)orig_drawTrainingHud));
		}
		if (orig_drawTrainingHud) {
			auto drawTrainingHudHookPtr = &HookHelp::drawTrainingHudHook;
			if (!detouring.attach(&(PVOID&)orig_drawTrainingHud,
				(PVOID&)drawTrainingHudHookPtr,
				"drawTrainingHud")) return false;*/
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
		uintptr_t appFreeCallPlace = sigscanForward((uintptr_t)FRenderCommandDestructor,
			"f6 44 24 04 01 56 8b f1 c7 06 ?? ?? ?? ?? 74 09 56 >e8", 0x20, nullptr);
		if (appFreeCallPlace) {
			appFree = (appFree_t)followRelativeCall(appFreeCallPlace);
		}
	}
	if (!GIsThreadedRendering
			|| !GRenderCommandBuffer
			|| !FRingBuffer_AllocationContext_Constructor
			|| !FRingBuffer_AllocationContext_Commit
			|| !appFree) {
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
	uintptr_t BBScr_sprite = 0;
	digUpBBScrFunction(uintptr_t, BBScr_sprite, instr_sprite)
	
	if (BBScr_sprite) {
		uintptr_t pos = sigscanForward(BBScr_sprite, "c2 08 00 >e9", 0x50);
		if (pos) {
			uintptr_t slightlyMoreInnerSprite = followRelativeCall(pos);
			if (slightlyMoreInnerSprite) {
				pos = sigscanForward(slightlyMoreInnerSprite, "89 be 68 0b 00 00 >e8", 0xb0);
				if (pos) {
					spriteImpl = (spriteImpl_t)followRelativeCall(pos);
				}
			}
		}
	}
	
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
		skippedF.resize(_countof(PlayerFramebar::frames)); \
		memset(skippedF.data(), 0, _countof(PlayerFramebar::frames) * sizeof SkippedFramesInfo);
		
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
	if (!hitDetectionFunc) {
		logwrap(fprintf(logfile, "Could not find hitDetectionFunc.\n"));
		error = true;
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
		
	if (!highlightGreenWhenBecomingIdleChanged()) return false;
	
	if (!onDontResetBurstAndTensionGaugesWhenInStunOrFaintChanged()) return false;
	
	if (!onDontResetRiscWhenInBurstOrFaintChanged()) return false;
	
	if (!onOnlyApplyCounterhitSettingWhenDefenderNotInBurstOrFaintOrHitstunChanged()) return false;
	
	if (!onStartingBurstGaugeChanged()) return false;
	
	if (!onEnableModsChanged()) return false;
	
	if (!onSpeedUpReplayChanged()) return false;
	
	return !error;
}

bool EndScene::sigscanAfterHitDetector() {
	if (!hitDetector.activeFrameHit) return true;
	uintptr_t activeFrameHitReflect = sigscanForward((uintptr_t)hitDetector.activeFrameHit,
		// ghidra sig: 89 93 48 02 00 00 e8 ?? ?? ?? ?? 8b 83 ac 26 00 00 85 c0 75
		"89 93 48 02 00 00 >e8 ?? ?? ?? ?? 8b 83 ac 26 00 00 85 c0 75",
		0x300);
	if (activeFrameHitReflect) {
		multiplySpeedX = (multiplySpeedX_t)followRelativeCall(activeFrameHitReflect);
		std::vector<char> newBytes(4);
		int offset = calculateRelativeCallOffset(activeFrameHitReflect, (uintptr_t)activeFrameHitReflectHookAsm);
		memcpy(newBytes.data(), &offset, 4);
		detouring.patchPlace(activeFrameHitReflect + 1, newBytes);
	}
	
	return true;
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

/*
// Runs on the main thread
void EndScene::HookHelp::drawTrainingHudHook() {
	endScene.drawTrainingHudHook((char*)this);
}
*/

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
		
		bool isRunning = EndScene::isRunning();
		entityList.populate();
		needDrawInputs = false;
		if (requestedInputHistoryDraw) needDrawInputs = true;
		if (gifMode.showInputHistory && !gifMode.gifModeToggleHudOnly && !gifMode.gifModeOn) {
			if (game.getGameMode() == GAME_MODE_NETWORK
					&& (
						settings.displayInputHistoryWhenObserving
						&& game.getPlayerSide() == 2
						|| settings.displayInputHistoryInOnline
					)) {
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
		if (!gifMode.gifModeToggleHudOnly && !gifMode.gifModeOn) {
			for (int i = 0; i < 2; ++i) {
				std::vector<EndSceneStoredState::PunishMessageTimer>& vec = currentState->punishMessageTimers[i];
				for (EndSceneStoredState::PunishMessageTimer& timer : vec) {
					if (timer.currentAnimFrame >= 0 && timer.currentAnimFrame < (int)_countof(punishAnim)) {
						int attackerSide = 1 - i;
						const PunishFrame& frameData = punishAnim[timer.currentAnimFrame];
						float x = (
							attackerSide == 0
								? 120.F
								: 1280.F - 120.F
						);
						int sign = (attackerSide == 0 ? 1 : -1);
						x += (float)(sign * frameData.xOffset + timer.xOff);
						float y = 490.F + timer.yOff;
						drawPunishMessage(x, y, attackerSide == 0 ? ALIGN_LEFT : ALIGN_RIGHT, frameData.color);
					}
				}
			}
		}
		if (!*aswEngine || currentState->startedNewRound && gifMode.editHitboxes) {
			ui.stopHitboxEditMode();
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
		if (currentState->prevAswEngineTickCountForInputs != aswEngineTickCount) {
			prepareInputs();
			currentState->prevAswEngineTickCountForInputs = aswEngineTickCount;
		}
		
		drawHitboxEditorHitboxes();
	}
	if (needToClearHitDetection) {
		currentState->attackHitboxes.clear();
		hitDetector.clearAllBoxes();
		throws.clearAllBoxes();
		currentState->leoParries.clear();
	}
	// Camera values are updated separately, in a updateCameraHook call, which happens before this is called
}

// Runs on the main thread
void EndScene::prepareDrawData(bool* needClearHitDetection) {
	logOnce(fputs("prepareDrawData called\n", logfile));
	invisChipp.onEndSceneStart();
	EndSceneStoredState* cs = currentState;
	
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
	bool frameHasChanged = prevAswEngineTickCountMain != aswEngineTickCount && !game.isRoundend();
	prevAswEngineTickCountMain = aswEngineTickCount;
	
	if (frameHasChanged) {
		
		Entity superflashInstigator = getSuperflashInstigator();
		
		for (int i = 0; i < 2; ++i) {
			PlayerInfo& player = cs->players[i];
			handleMarteliForpeliSetting(player);
		}
		
	}
	
	int playerSide = 2;
	if (gifMode.dontHideOpponentsBoxes || gifMode.dontHidePlayersBoxes) {
		playerSide = game.getPlayerSide();
	}
	
	// WARNING!
	// If the mod was injected in the middle of a round end or round start, player.pawn may be null here!!!
	// This will lead to a crash if you try to use it without checking for null.
	// Better use ent.playerEntity() instead.
	for (int i = 0; i < entityList.count; i++) {
		Entity ent = entityList.list[i];
		int team = ent.team();
		PlayerInfo& player = cs->players[team];
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
		if (i < 2 && (team == 0 || team == 1)) {
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
			for (EndSceneStoredState::AttackHitbox& attackHitbox : cs->attackHitboxes) {
				if (ent == attackHitbox.ent) {
					needCollectHitboxes = false;
					drawDataPrepared.hitboxes.push_back(attackHitbox.hitbox);
					hitboxesCount = attackHitbox.count;
					attackHitbox.found = true;
					break;
				}
			}
		}
		
		// need entityState
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
			drawDataPrepared.hurtboxes.emplace_back( hurtbox );
		}
		else {
			drawDataPrepared.hurtboxes.emplace_back( hurtbox, wasHitResult.hurtbox );
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
			drawDataPrepared.hurtboxes.emplace_back( attachedHurtbox );
			drawnEntities.push_back(attached);
		}
		if (team == 0 || team == 1) {
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
	drawnEntities.clear();
	
	for (const EndSceneStoredState::AttackHitbox& attackHitbox : cs->attackHitboxes) {
		if (!attackHitbox.found && !invisChipp.needToHide(attackHitbox.team)) {
			drawDataPrepared.hitboxes.push_back(attackHitbox.hitbox);
		}
	}
	
	for (const EndSceneStoredState::LeoParry& parry : cs->leoParries) {
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
	
	logOnce(fputs("got past the entity loop\n", logfile));
	hitDetector.drawHits();
	logOnce(fputs("hitDetector.drawDetected() call successful\n", logfile));
	throws.drawThrows();
	logOnce(fputs("throws.drawThrows() call successful\n", logfile));
	
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
	if (newStoredFramesCount > (int)FRAMES_MAX) {
		newStoredFramesCount = FRAMES_MAX;
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
	
	if (combinedFramebarsSettingsChanged || frameHasChanged) {
		
		int framebarPositionUse;
		int framesTotalUse;
		if (neverIgnoreHitstop) {
			framebarPositionUse = cs->framebarPositionHitstop;
			framesTotalUse = cs->framebarTotalFramesHitstopUnlimited;
		} else {
			framebarPositionUse = cs->framebarPosition;
			framesTotalUse = cs->framebarTotalFramesUnlimited;
		}
		int framebarPositionUseWithoutScroll = framebarPositionUse;
		framebarPositionUse -= scrollXInFrames;
		if (framebarPositionUse < 0) {
			framebarPositionUse += _countof(PlayerFramebar::frames);
		}
		framesTotalUse -= scrollXInFrames;
		int lastNFramesToCheck = min(framesCount, framesTotalUse);
		const bool recheckCompletelyEmpty = lastNFramesToCheck != FRAMES_MAX;
		
		combinedFramebars.clear();
		if (!eachProjectileOnSeparateFramebar) {
			combinedFramebars.reserve(projectileFramebars.size());
			const bool combinedFramebarMustIncludeHitstop = neverIgnoreHitstop;
			for (ThreadUnsafeSharedPtr<ProjectileFramebar>& source : projectileFramebars) {
				if (aswEngineTickCount >= source->creationTick && aswEngineTickCount < source->deletionTick) {
					Framebar& from = combinedFramebarMustIncludeHitstop ? source->hitstop : source->main;
					if (!(from.stateHead->completelyEmpty || recheckCompletelyEmpty && from.lastNFramesCompletelyEmpty(framebarPositionUse, lastNFramesToCheck))) {
						CombinedProjectileFramebar& entityFramebar = findCombinedFramebar(
							*source, combinedFramebarMustIncludeHitstop,
							scrollXInFrames, framebarPositionUse, framesTotalUse);
						entityFramebar.combineFramebar(framebarPositionUse, framebarPositionUseWithoutScroll, scrollXInFrames, framesTotalUse,
							from, &*source);
					}
				}
			}
			for (CombinedProjectileFramebar& entityFramebar : combinedFramebars) {
				entityFramebar.determineName(framebarPositionUse, scrollXInFrames, combinedFramebarMustIncludeHitstop);
			}
		} else if (eachProjectileOnSeparateFramebarChanged) {
			for (ThreadUnsafeSharedPtr<ProjectileFramebar>& source : projectileFramebars) {
				int iEnd = source->hitstop.stateHead->framesCount;
				for (int i = 0; i < iEnd; ++i) {
					source->hitstop.frames[i].next = nullptr;
				}
				iEnd = source->main.stateHead->framesCount;
				for (int i = 0; i < iEnd; ++i) {
					source->main.frames[i].next = nullptr;
				}
			}
		}
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
		camera.frozen = freezeGame;
		ui.freezeGame = freezeGame;
	}
	if (!gifMode.modDisabled && keyboard.gotPressed(settings.slowmoGameToggle)) {
		if (std::abs(gifMode.fps - 60.F) < 0.001F) {
			if (settings.slowmoFps < 1.F) settings.slowmoFps = 1.F;
			if (settings.slowmoFps > 999.F) settings.slowmoFps = 999.F;
			if (*game.gameDataPtr && (game.isTrainingMode() || game.getGameMode() == GAME_MODE_REPLAY) && *aswEngine) {
				logwrap(fputs("Changing FPS to a custom one from a hotkey press\n", logfile));
				gifMode.fps = settings.slowmoFps;
				game.onFPSChanged();
			} else {
				logwrap(fputs("Declined changing FPS to a custom one from a hotkey press\n", logfile));
			}
		} else {
			gifMode.fps = 60.F;
			logwrap(fputs("Changed FPS back to 60 from a hotkey press\n", logfile));
		}
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
		ui.onVisibilityToggleKeyboardShortcutPressed();
		logwrap(fputs("UI display toggled\n", logfile));
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
				for (int stateIndex = 0; stateIndex < stateCount; ++stateIndex) {
					stateRingBuffer[stateIndex].players[i].tensionGainMaxCombo = 0;
					stateRingBuffer[stateIndex].players[i].tensionGainLastCombo = 0;
				}
			}
			if (ui.clearBurstGainMaxCombo[i]) {
				ui.clearBurstGainMaxCombo[i] = false;
				for (int stateIndex = 0; stateIndex < stateCount; ++stateIndex) {
					stateRingBuffer[stateIndex].players[i].burstGainMaxCombo = 0;
					stateRingBuffer[stateIndex].players[i].burstGainLastCombo = 0;
				}
			}
		}
	}
	if (!gifMode.modDisabled && keyboard.gotPressed(settings.hitboxEditModeToggle)) {
		ui.onToggleHitboxEditMode();
		logwrap(fputs("Toggled hitbox edit mode with a keyboard shortcut\n", logfile));
	}
	if (gifMode.editHitboxes && gifMode.editHitboxesEntity && *aswEngine) {
		ui.editHitboxesProcessControls();
	}
	gifMode.speedUpReplay = !gifMode.modDisabled && keyboard.isHeld(settings.fastForwardReplay);
	if (gifMode.speedUpReplay) {
		onSpeedUpReplayChanged();
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
	camera.frozen = game.freezeGame;
	if (!trainingMode || gifMode.modDisabled) {
		gifMode.gifModeOn = false;
		ui.gifModeOn = false;
		gifMode.noGravityOn = false;
		ui.noGravityOn = false;
		gifMode.fps = 60.F;
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
		ui.stopHitboxEditMode();
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
			case WM_APP_CONNECTION_TIER_CHANGED: {
				game.onConnectionTierChanged();
			}
			break;
			case WM_APP_HIGHLIGHTED_MOVES_CHANGED: {
				ui.highlightedMovesChanged();
				highlightSettingsChanged();
			}
			// omission of 'break' intentional
			case WM_APP_HIGHLIGHT_GREEN_WHEN_BECOMING_IDLE_CHANGED: {
				endScene.highlightGreenWhenBecomingIdleChanged();
			}
			break;
			case WM_APP_PINNED_WINDOWS_CHANGED: {
				ui.pinnedWindowsChanged();
			}
			break;
			case WM_APP_DISABLE_PIN_BUTTON_CHANGED: {
				ui.onDisablePinButtonChanged(true);
			}
			break;
			case WM_APP_DONT_RESET_BURST_GAUGE_WHEN_IN_STUN_OR_FAINT_CHANGED: {
				onDontResetBurstAndTensionGaugesWhenInStunOrFaintChanged();
			}
			break;
			case WM_APP_DONT_RESET_RISC_WHEN_IN_BURST_OR_FAINT_CHANGED: {
				onDontResetRiscWhenInBurstOrFaintChanged();
			}
			break;
			case WM_APP_ONLY_APPLY_COUNTERHIT_SETTING_WHEN_DEFENDER_NOT_IN_BURST_OR_FAINT_OR_HITSTUN_CHANGED: {
				onOnlyApplyCounterhitSettingWhenDefenderNotInBurstOrFaintOrHitstunChanged();
			}
			break;
			case WM_APP_STARTING_BURST_GAUGE_CHANGED: {
				onStartingBurstGaugeChanged();
			}
			break;
		}
	}
	
	LRESULT result = orig_WndProc(hWnd, message, wParam, lParam);
	return result;
}

/*
// The info provided here and the sigscanning we do to find this are correct, we just ended up not needing this
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
	#if 0
	if (!shutdown && !graphics.shutdown && false) {
		drawTexts();
	}
	#endif
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
		s.textColor = -1;
		s.field164_0x114 = 0;
		s.field165_0x118 = 0;
		s.field166_0x11c = -1;
		s.outlineColor = 0xff000000;
		s.dropShadowColor = 0xff000000;
		s.x = 100;
		s.y = 185.0 + 34 * 3;
		s.alignment = ALIGN_LEFT;
		s.text = HelloWorld;
		s.flags1 = 0x210;
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
		s.textColor = -1;
		s.field164_0x114 = 0;
		s.field165_0x118 = 0;
		s.field166_0x11c = -1;
		s.outlineColor = 0xff000000;
		s.dropShadowColor = 0xff000000;
		s.x = 460;
		s.y = 185.0 + 34 * 3;
		s.alignment = ALIGN_CENTER;
		s.text = HelloWorld;
		s.flags1 = 0x210;
		drawTextWithIcons(&s,0x0,1,4,0,0);
	}
	#endif
}
*/

// Called when switching characters, exiting the match.
// Runs on the main thread
void EndScene::onAswEngineDestroyed() {
	stateRingBuffer.resize(1);
	stateCount = 1;
	currentState = stateRingBuffer.data();
	
	currentState->superfreezeHasBeenGoingFor = 0;
	currentState->lastNonZeroSuperflashInstigator = nullptr;
	currentState->superfreezeHasBeenGoingFor = 0;
	currentState->superflashCounterAllied = 0;
	currentState->superflashCounterAlliedMax = 0;
	currentState->superflashCounterOpponent = 0;
	currentState->superflashCounterOpponentMax = 0;

	// do this even if 'give up'
	for (int i = 0; i < 2; ++i) {
		currentState->players[i].clear();
		currentState->reachedMaxStun[i] = -1;
		currentState->punishMessageTimers[i].clear();
		currentState->attackerInRecoveryAfterBlock[i] = false;
	}
	memset(currentState->attackerInRecoveryAfterCreatingProjectile, 0, sizeof currentState->attackerInRecoveryAfterCreatingProjectile);
	currentState->measuringFrameAdvantage = false;
	currentState->measuringLandingFrameAdvantage = -1;
	memset(currentState->tensionGainOnLastHit, 0, sizeof currentState->tensionGainOnLastHit);
	memset(currentState->burstGainOnLastHit, 0, sizeof currentState->burstGainOnLastHit);
	memset(currentState->tensionGainOnLastHitUpdated, 0, sizeof currentState->tensionGainOnLastHitUpdated);
	memset(currentState->burstGainOnLastHitUpdated, 0, sizeof currentState->burstGainOnLastHitUpdated);
	currentState->projectiles.clear();
	sendSignalStack.clear();
	currentState->events.clear();

	currentState->registeredHits.clear();

	currentState->prevAswEngineTickCount = 0xffffffff;
	currentState->prevAswEngineTickCountForInputs = 0xffffffff;
	
	drawDataPrepared.clear();
	clearContinuousScreenshotMode();
	creatingObject = false;
	ui.onAswEngineDestroyed();
	moves.onAswEngineDestroyed();
	highlightMoveCache.clear();
	
	playerFramebars.clear();
	projectileFramebars.clear();
	combinedFramebars.clear();
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
	
	prevAswEngineTickCountMain = 0xffffffff;
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
	PlayerInfo& player = currentState->players[index];
	PlayerInfo& other = currentState->players[1 - index];
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
	currentState->measuringFrameAdvantage = true;
}

// Runs on the main thread
void EndScene::restartMeasuringLandingFrameAdvantage(int index) {
	PlayerInfo& player = currentState->players[index];
	PlayerInfo& other = currentState->players[1 - index];
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
	currentState->measuringLandingFrameAdvantage = 1 - index;
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
		for (int i = 0; i < 2; ++i) {
			Entity ent = entityList.slots[i];
			currentState->tensionRecordedHit[i] = ent.tension();
			currentState->burstRecordedHit[i] = game.getBurst(i);
			currentState->tensionGainOnLastHit[i] = 0;
			currentState->burstGainOnLastHit[i] = 0;
			currentState->tensionGainOnLastHitUpdated[i] = false;
			currentState->burstGainOnLastHitUpdated[i] = false;
			currentState->reachedMaxStun[i] = -1;
			
			// moved this here from handleUponHook, signal 0x27 (PRE_DRAW), because Jam would not show super armor on the frame she parries.
			// Also the Blitz Shield reject would not show super armor on the frame the attacker's hit connects.
			PlayerInfo& player = findPlayer(ent);
			player.wasSuperArmorEnabled = ent.superArmorEnabled();
			player.wasFullInvul = ent.fullInvul();
		}
		currentState->registeredHits.clear();
		
		currentState->attackHitboxes.clear();
	}
}

// We use this hook at the end of hit detection algorithm to measure some values after a hit.
// Runs on the main thread
void EndScene::onHitDetectionEnd(HitDetectionType hitDetectionType) {
	
	for (EndSceneStoredState::AttackHitbox& attackHitbox : currentState->attackHitboxes) {
		if (attackHitbox.clash && !attackHitbox.notClash && attackHitbox.hitbox.thickness > 1) {
			attackHitbox.hitbox.thickness = 1;
		}
	}
	
	entityList.populate();
	for (int i = 0; i < 2; ++i) {
		Entity ent = entityList.slots[i];
		int tension = ent.tension();
		int tensionBefore = currentState->tensionRecordedHit[i];
		if (tension != tensionBefore) {
			currentState->tensionGainOnLastHit[i] += tension - tensionBefore;
			currentState->tensionGainOnLastHitUpdated[i] = true;
		}
		currentState->tensionRecordedHit[i] = tension;
		int burst = game.getBurst(i);
		int burstBefore = currentState->burstRecordedHit[i];
		if (burst != burstBefore) {
			currentState->burstGainOnLastHit[i] += burst - burstBefore;
			currentState->burstGainOnLastHitUpdated[i] = true;
		}
		currentState->burstRecordedHit[i] = burst;
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
	currentState->registeredHits.emplace_back();
	EndSceneStoredState::RegisteredHit& hit = currentState->registeredHits.back();
	if (defender.isPawn()) {
		int attackerIndex = 1 - defender.team();
		if (attacker.isPawn()) {
			currentState->attackerInRecoveryAfterBlock[attackerIndex] = true;
		} else {
			currentState->attackerInRecoveryAfterBlock[attackerIndex] = currentState->attackerInRecoveryAfterBlock[attackerIndex]
				|| currentState->attackerInRecoveryAfterCreatingProjectile[attackerIndex][attacker.getEffectIndex()];
		}
	}
	if (hitResult == HIT_RESULT_ARMORED && attacker.isPawn() && defender.isPawn()
			&& defender.characterType() == CHARACTER_TYPE_LEO
			&& strcmp(defender.animationName(), "Semuke5E") == 0) {
		EndSceneStoredState::LeoParry newParry;
		newParry.x = defender.x();
		newParry.y = defender.y();
		newParry.timer = 0;
		newParry.aswEngTick = getAswEngineTick() + 1;  // game's timestamp gets incremented at the end of the tick
		currentState->leoParries.push_back(newParry);
	}
	if (!attacker.isPawn()) {
		hit.projectile.fill(attacker, getSuperflashInstigator(), false);
	}
	hit.isPawn = attacker.isPawn();
	hit.hitResult = hitResult;
	hit.hasHitbox = hasHitbox;
	hit.attacker = attacker;
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

// Called at the start of an UE3 engine tick.
// This tick runs even when paused or not in a match.
// Runs on the main thread
void EndScene::onUWorld_TickBegin() {
	logicThreadId = GetCurrentThreadId();
	drewExGaugeHud = false;
	camera.grabbedValues = false;
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
ProjectileInfo& EndScene::onObjectCreated(Entity pawn, Entity createdPawn, const char* animName, bool fillName, bool calledFromInsideTick) {
	std::vector<ProjectileInfo>& projectiles = currentState->projectiles;
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
		projectile.creationTime_aswEngineTick = getAswEngineTick() + calledFromInsideTick;  // game's timestamp gets incremented at the end of the tick
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
	auto itEnd = currentState->registeredHits.end();
	for (auto it = currentState->registeredHits.begin(); it != itEnd; ++it) {
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
				newElem.timestamp = getAswEngineTick() + 1;  // game's timestamp gets incremented at the end of the tick
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
	// need to reset these to null because Jam Hochifu cancels work through walk cancelling
	player.startedRunning = false;
	player.startedWalkingForward = false;
	player.startedWalkingBackward = false;
	if (strcmp(animName, "CmnActFDash") == 0) {
		player.startedRunning = true;
	} else if (strcmp(animName, "CrouchFWalk") == 0 || strcmp(animName, "CmnActFWalk") == 0
			|| strcmp(animName, "MistFinerFWalk") == 0) {
		player.startedWalkingForward = true;
	} else if (strcmp(animName, "CrouchBWalk") == 0 || strcmp(animName, "CmnActBWalk") == 0
			|| strcmp(animName, "MistFinerBWalk") == 0) {
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
		newComboElem.timestamp = getAswEngineTick() + 1;  // game's timestamp gets incremented at the end of the tick
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
	if (!shutdown && pawn.isPawn() && !gifMode.modDisabled) {
		if (strcmp(animName, "CmnActLandingStiff") != 0) {
			currentState->attackerInRecoveryAfterBlock[pawn.team()] = false;
			memset(currentState->attackerInRecoveryAfterCreatingProjectile[pawn.team()], 
				false,
				_countof(currentState->attackerInRecoveryAfterCreatingProjectile[0]));
		}
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
			currentState->events.emplace_back();
			EndSceneStoredState::OccurredEvent& event = currentState->events.back();
			event.type = EndSceneStoredState::OccurredEvent::SET_ANIM;
			event.u.setAnim.pawn = pawn;
			memcpy(event.u.setAnim.fromAnim, pawn.animationName(), 32);
			
			registerJump(player, pawn, animName);
			registerRun(player, pawn, animName);
		}
	}
	orig_setAnim((void*)pawn, animName);
	if (!shutdown && pawn.isPawn() && !gifMode.modDisabled) {
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
		if (currentState->players[i].pawn == ent) {
			return currentState->players[i];
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
	// wow ok
}

// Stuff that runs at the start of a tick
// Runs on the main thread
void EndScene::frameCleanup() {
	entityList.populate();
	std::vector<ProjectileInfo>& projectiles = currentState->projectiles;
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
	for (PlayerInfo& player : currentState->players) {
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
		player.wasClashCancelTimer = false;
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
	currentState->events.clear();
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
		if (endScene.creatorOfCreatedObject) {
			int team = endScene.creatorOfCreatedObject.team();
			int effectIndex = createdObj.getEffectIndex();
			if (endScene.creatorOfCreatedObject.isPawn()) {
				currentState->attackerInRecoveryAfterCreatingProjectile[team][effectIndex] = true;
			} else {
				currentState->attackerInRecoveryAfterCreatingProjectile[team][effectIndex] =
					currentState->attackerInRecoveryAfterCreatingProjectile[team][endScene.creatorOfCreatedObject.getEffectIndex()];
			}
		}
		
		ProjectileInfo& newProjectileInfo = onObjectCreated(creatorOfCreatedObject, createdObj, createdObjectAnim, false, true);
		newProjectilePtr = newProjectileInfo.ptr;
		currentState->events.emplace_back();
		EndSceneStoredState::OccurredEvent& event = currentState->events.back();
		event.type = EndSceneStoredState::OccurredEvent::SIGNAL;
		event.u.signal.from = creatorOfCreatedObject;
		event.u.signal.to = createdObj;
		memcpy(event.u.signal.fromAnim, creatorOfCreatedObject.animationName(), 32);
		if (creatorOfCreatedObject.isPawn()) {
			event.u.signal.creatorName.clear();
		} else {
			ProjectileInfo::determineCreatedName(nullptr, creatorOfCreatedObject, nullptr, &event.u.signal.creatorName, true, true);
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
	if (!shutdown) {
		if (!sendSignalStack.empty()) {
			currentState->events.emplace_back();
			EndSceneStoredState::OccurredEvent& event = currentState->events.back();
			event.type = EndSceneStoredState::OccurredEvent::SIGNAL;
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
	if (!shutdown && !gifMode.modDisabled) {
		if (signal == BBSCREVENT_ANIMATION_FRAME_ADVANCED
				&& pawn.isPawn()) {
			// Slayer's buff in PLAYERVAL_1 is checked when initiating a move, but decremented after the fact, in a FRAME_STEP handler
			// we need the original value
			PlayerInfo& player = findPlayer(pawn);
			player.wasPlayerval1Idling = pawn.playerVal(1);
			player.wasResource = pawn.exGaugeValue(0);
		}
		// Blitz Shield rejection changes super armor enabled and full invul flags at the end of a logic tick
		if (signal == BBSCREVENT_PRE_DRAW) {
			entityList.populate();
			if (pawn.isPawn()) {
				PlayerInfo& player = findPlayer(pawn);
				// moved collection of wasSuperArmorEnabled and wasFullInvul from here to onHitDetectionStart,
				// because the frame when Jam parries would be shown as not having super armor in the framebar
				
				// I put this here of all places because the other places are disabled by iGiveUp (an old lockdown mechanism that prevented the mod
				// from displaying or collecting framedata in non-observer online modes due to fear of rollback), and I want this to always work no matter what.
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
				
				if (game.isTrainingMode()) {
					// the color highlights get processed here, because doing them at REDAnywhereDispDraw causes highlights to get delayed by 1 frame
					int controllingSide = 2;
					if (
						(
							settings.highlightRedWhenBecomingIdle
							|| settings.highlightGreenWhenBecomingIdle
							|| settings.highlightBlueWhenBecomingIdle
						)
					) {
						controllingSide = game.currentPlayerControllingSide();
						if (player.index == controllingSide
								&& !(
									pawn.cmnActIndex() == CmnActJump
									&& pawn.currentAnimDuration() == 2
									&& !player.moveOriginatedInTheAir
								)
								&& !(
									pawn.cmnActIndex() == CmnActJumpLanding
									&& pawn.currentAnimDuration() == 2
									&& strcmp(pawn.previousAnimName(), "CmnActJump") == 0
								)
								&& !player.idle
								&& player.pawn) {
							static MoveInfo moveInfo;
							if (moves.getInfo(moveInfo, player.charType, pawn.currentMoveIndex() == -1
									? (const char*)pawn.bbscrCurrentFunc() + 4 : pawn.currentMove()->name, false)) {
								if (moveInfo.isIdle && moveInfo.isIdle(player)) {
									if (settings.highlightRedWhenBecomingIdle) {
										player.redHighlightTimer = sizeof greenHighlights;
									}
									if (settings.highlightGreenWhenBecomingIdle) {
										player.greenHighlightTimer = sizeof greenHighlights;
									}
									if (settings.highlightBlueWhenBecomingIdle) {
										player.blueHighlightTimer = sizeof greenHighlights;
									}
								}
							}
						}
					}
					if (!settings.highlightWhenCancelsIntoMovesAvailable.pointers.empty()) {
						if (controllingSide == 2) {
							controllingSide = game.currentPlayerControllingSide();
						}
						if (player.index == controllingSide) {
							bool red = false;
							bool green = false;
							bool blue = false;
							std::vector<const AddedMoveData*> markedMoves;
							if (hasCancelUnlocked(pawn.characterType(), player.wasCancels.gatlings, markedMoves, &red, &green, &blue)
									|| hasCancelUnlocked(pawn.characterType(), player.wasCancels.whiffCancels, markedMoves, &red, &green, &blue)) {
								if (red) {
									player.redHighlightTimer = sizeof greenHighlights;
								}
								if (green) {
									player.greenHighlightTimer = sizeof greenHighlights;
								}
								if (blue) {
									player.blueHighlightTimer = sizeof greenHighlights;
								}
							}
						}
					}
					processColor(player);
				}
			}
			if (pawn == entityList.list[entityList.count - 1]) {
				logicInside();
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
	if (pawn.isPawn() && !functionWillNotDoAnything) {
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
		player.receivedSpeedY = pawn.received()->impulseY;
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
	for (ProjectileInfo& projectile : currentState->projectiles) {
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
	if (!shutdown && pawn.isPawn()) {
		Entity objectBeingRunOn = pawn.currentRunOnObject();
		if (objectBeingRunOn) {
			if (!objectBeingRunOn.isPawn()) {
				ProjectileInfo& projectile = findProjectile(objectBeingRunOn);
				if (projectile.ptr) {
					currentState->events.emplace_back();
					EndSceneStoredState::OccurredEvent& event = currentState->events.back();
					event.type = EndSceneStoredState::OccurredEvent::SIGNAL;
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
	} else if (game.getGameMode() == GAME_MODE_NETWORK) {
		int playerSide = game.getPlayerSide();
		if (playerSide != 2 && !game.isOnlineTrainingMode_Part()) {
			drawData.inputsSize[1 - playerSide] = 0;
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
	needsFramesTextureFramebar = endScene.needsFramesTextureFramebar;
	needsFramesTextureHelp = endScene.needsFramesTextureHelp;
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
		if (!game.isTrainingMode() && game.getGameMode() != GAME_MODE_REPLAY || !*aswEngine) {
			gifMode.fps = 60.F;
		}
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
			uiWillBeDrawnOnTopOfPauseMenu = isFading || settings.displayUIOnTopOfPauseMenu && pauseMenuOpen && ui.isVisible();
		} else {
			if (gifMode.editHitboxes) {
				ui.stopHitboxEditMode();
			}
			uiWillBeDrawnOnTopOfPauseMenu = true;
		}
		drawingPostponedLocal = drawingPostponed();
		if (!shutdown && !graphics.shutdown) {
			ui.drawData = nullptr;
			ui.pauseMenuOpen = pauseMenuOpen;
			ui.drawingPostponed = drawingPostponedLocal;
			ui.needSplitFramebar = uiWillBeDrawnOnTopOfPauseMenu && !drawingPostponedLocal && pauseMenuOpen && ui.isVisible();
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
		finishedSigscanning();
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
	InterlockedDecrement(&totalCountOfCommandsInCirculation);
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
		graphics.uiNeedsFramesTextureFramebar = command->uiOrFramebarDrawData.needsFramesTextureFramebar;
		graphics.uiNeedsFramesTextureHelp = command->uiOrFramebarDrawData.needsFramesTextureHelp;
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
	s.textColor = -1;
	s.field164_0x114 = 0;
	s.field165_0x118 = 0;
	s.field166_0x11c = -1;
	s.outlineColor = 0xff000000;
	s.dropShadowColor = 0xff000000;
	s.x = x;
	s.y = 0.F;
	s.alignment = ALIGN_LEFT;
	s.text = txt;
	s.flags1 = 0x010;
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
		graphics.uiNeedsFramesTextureFramebar = command->uiOrFramebarDrawData.needsFramesTextureFramebar;
		graphics.uiNeedsFramesTextureHelp = command->uiOrFramebarDrawData.needsFramesTextureHelp;
		graphics.inputHistoryIsSplitOut = command->uiOrFramebarDrawData.inputHistoryIsSplitOut;
		return;
	}
	if (graphics.drawingPostponed()) {
		return;
	}
	ui.onEndScene(getDevice(), command->uiOrFramebarDrawData.drawData.data(), tex,
		command->uiOrFramebarDrawData.needsFramesTextureFramebar,
		command->uiOrFramebarDrawData.needsFramesTextureHelp);
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
	entityList.populate();
	for (int i = 0; i < 2; ++i) {
		PlayerInfo& player = currentState->players[i];
		player.fdPushback = entityList.slots[i].fdPushback();
		// it changes after it was used, so if we don't do this, at the end of a frame we will see a decremented value
	}
	orig_backPushbackApplier((void*)thisArg);
}

void EndScene::HookHelp::pushbackStunOnBlockHook(bool isAirHit) {
	endScene.pushbackStunOnBlockHook(Entity{(char*)this}, isAirHit);
}

void EndScene::pushbackStunOnBlockHook(Entity pawn, bool isAirHit) {
	orig_pushbackStunOnBlock((void*)pawn.ent, isAirHit);
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

void EndScene::HookHelp::BBScr_sendSignalHook(EntityReferenceType referenceType, BBScrEvent signal) {
	endScene.BBScr_sendSignalHook(Entity{(char*)this}, referenceType, signal);
}

void EndScene::HookHelp::BBScr_sendSignalToActionHook(const char* searchAnim, BBScrEvent signal) {
	endScene.BBScr_sendSignalToActionHook(Entity{(char*)this}, searchAnim, signal);
}

void EndScene::BBScr_sendSignalHook(Entity pawn, EntityReferenceType referenceType, BBScrEvent signal) {
	Entity referredEntity = getReferredEntity((void*)pawn.ent, referenceType);
	
	bool isDizzyBubblePopping = referredEntity == pawn && signal == BBSCREVENT_CUSTOM_SIGNAL_1
			&& isDizzyBubble(pawn.animationName());
	
	if (!shutdown && referredEntity && !isDizzyBubblePopping) {
		ProjectileInfo& projectile = findProjectile(referredEntity);
		int team = pawn.team();
		if (projectile.ptr && (team == 0 || team == 1)) {
			currentState->events.emplace_back();
			EndSceneStoredState::OccurredEvent& event = currentState->events.back();
			event.type = EndSceneStoredState::OccurredEvent::SIGNAL;
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
	orig_BBScr_sendSignal((void*)pawn, referenceType, signal);
}

void EndScene::BBScr_sendSignalToActionHook(Entity pawn, const char* searchAnim, BBScrEvent signal) {
	if (!shutdown) {
		sendSignalStack.push_back(pawn);
	}
	orig_BBScr_sendSignalToAction((void*)pawn, searchAnim, signal);
	if (!shutdown) {
		sendSignalStack.pop_back();
	}
}

BOOL EndScene::HookHelp::skillCheckPieceHook() {
	return endScene.skillCheckPieceHook(Entity{(char*)this});
}

BOOL EndScene::skillCheckPieceHook(Entity pawn) {
	BOOL result = orig_skillCheckPiece((void*)pawn.ent);
	PlayerInfo& player = findPlayer(pawn);
	if (player.pawn) {
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
			if (doubleJumpHeightOk && !player.wasCancels.hasCancel(player.wasCancels.gatlings, "AirStopCancelOnly")) {
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
		player.wasClashCancelTimer = player.wasClashCancelTimer && pawn.currentAnimDuration() != 1 || pawn.clashCancelTimer() > 0;
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
	return result;
}

void EndScene::HookHelp::BBScr_setHitstopHook(int hitstop) {
	endScene.BBScr_setHitstopHook(Entity{(char*)this}, hitstop);
}

void EndScene::BBScr_setHitstopHook(Entity pawn, int hitstop) {
	if (!shutdown) {
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
	if (pawn.needSetHitstop()) {
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
	PlayerInfo& player = findPlayer(pawn);
	player.baikenReturningToBlockstunAfterAzami = true;
}

bool EndScene::isEntityHidden(const Entity& ent) {
	return findHiddenEntity(ent) != hiddenEntities.end();
}

int EndScene::getFramebarPosition() const {
	return currentState->framebarPosition;
}

int EndScene::getFramebarPositionHitstop() const {
	return currentState->framebarPositionHitstop;
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
		for (const ThreadUnsafeSharedPtr<ProjectileFramebar>& f: projectileFramebars) {
			if (f->id == nextFramebarId) {
				incrementNextFramebarIdDirectly();
				restart = true;
				break;
			}
		}
	} while(restart);
}

ProjectileFramebar& EndScene::findProjectileFramebar(ProjectileInfo& projectile, bool needCreate) {
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
	
	DWORD currentTick = getAswEngineTick();
	for (ThreadUnsafeSharedPtr<ProjectileFramebar>& bar : projectileFramebars) {
		if (currentTick >= bar->creationTick && currentTick < bar->deletionTick
				&& bar->playerIndex == projectile.team
				&& (
					projectile.move.isEddie
						? bar->isEddie
						:
							!bar->isEddie
							&& bar->id == projectile.framebarId
				)
			) {
			if (!(bar->stateHead->moveFramebarId != -1 && projectile.move.framebarId == -1)) {
				bar->stateHead->moveFramebarId = projectile.move.framebarId;
			}
			return *bar;
		}
	}
	if (!needCreate) {
		return defaultFramebar;
	}
	projectileFramebars.emplace_back(new ThreadUnsafeSharedResource<ProjectileFramebar>());
	ThreadUnsafeSharedPtr<ProjectileFramebar>& bar = projectileFramebars.back();
	bar->creationTick = currentTick;
	bar->deletionTick = 0xFFFFFFFF;
	bar->playerIndex = projectile.team;
	bar->id = nextFramebarId;
	bar->isEddie = projectile.move.isEddie;
	projectile.framebarId = nextFramebarId;
	incrementNextFramebarIdSmartly();
	bar->stateHead = bar->states;
	bar->stateHead->moveFramebarId = projectile.move.framebarId;
	bar->main.stateHead = bar->main.states;
	bar->idle.stateHead = bar->idle.states;
	bar->hitstop.stateHead = bar->hitstop.states;
	bar->idleHitstop.stateHead = bar->idleHitstop.states;
	return *bar;
}

CombinedProjectileFramebar& EndScene::findCombinedFramebar(const ProjectileFramebar& source, bool hitstop,
				int scrollX, int framebarPosition, int framesTotal) {
	int id = source.idForCombinedFramebar();
	const bool combineProjectileFramebarsWhenPossible = settings.combineProjectileFramebarsWhenPossible;
	const bool condenseIntoOneProjectileFramebar = settings.condenseIntoOneProjectileFramebar;
	if (condenseIntoOneProjectileFramebar) {
		for (CombinedProjectileFramebar& bar : combinedFramebars) {
			if (bar.playerIndex == source.playerIndex) return bar;
		}
	} else {
		bool tooManyBars = combinedFramebars.size() > 10;
		for (CombinedProjectileFramebar& bar : combinedFramebars) {
			if (
					bar.playerIndex == source.playerIndex
					&& (
						tooManyBars
						|| bar.id == id
						|| combineProjectileFramebarsWhenPossible
						&& bar.canBeCombined(hitstop ? source.hitstop : source.main, id,
							scrollX, framebarPosition, framesTotal)
					)
			) {
				return bar;
			}
		}
	}
	combinedFramebars.emplace_back();
	CombinedProjectileFramebar& bar = combinedFramebars.back();
	bar.playerIndex = source.playerIndex;
	bar.id = id;
	bar.isEddie = source.isEddie;
	return bar;
}

void EndScene::copyIdleHitstopFrameToTheRestOfSubframebars(PlayerFramebars& entityFramebar,
		bool framebarAdvanced,
		bool framebarAdvancedIdle,
		bool framebarAdvancedHitstop,
		bool framebarAdvancedIdleHitstop) {
	
	PlayerFramebar& framebar = entityFramebar.idleHitstop;
	const int framebarPos = EntityFramebar::confinePos(currentState->framebarPositionHitstop + currentState->framebarIdleHitstopFor);
	PlayerFrame& currentFrame = framebar.frames[framebarPos];
	PlayerFrame* destinationFrame;
	
	
	destinationFrame = &entityFramebar.hitstop.frames[currentState->framebarPositionHitstop];
	if (framebarAdvancedHitstop) {
		entityFramebar.copyFrame(*destinationFrame, currentFrame);
		entityFramebar.hitstop.processRequests(*destinationFrame);
	} else {
		if (!framebarAdvancedIdleHitstop) {
			entityFramebar.copyActiveDuringSuperfreeze(*destinationFrame, currentFrame);
		}
		entityFramebar.hitstop.collectRequests(framebar, framebarAdvancedIdleHitstop, currentFrame);
	}
	int idlePos = EntityFramebar::confinePos(currentState->framebarPosition + currentState->framebarIdleFor);
	destinationFrame = &entityFramebar.idle.frames[idlePos];
	if (framebarAdvancedIdle) {
		entityFramebar.copyFrame(*destinationFrame, currentFrame);
		entityFramebar.idle.processRequests(*destinationFrame);
	} else {
		if (!framebarAdvancedIdleHitstop) {
			entityFramebar.copyActiveDuringSuperfreeze(*destinationFrame, currentFrame);
		}
		entityFramebar.idle.collectRequests(framebar, framebarAdvancedIdleHitstop, currentFrame);
	}
	destinationFrame = &entityFramebar.main.frames[currentState->framebarPosition];
	if (framebarAdvanced) {
		entityFramebar.copyFrame(*destinationFrame, currentFrame);
		entityFramebar.main.processRequests(*destinationFrame);
	} else {
		if (!framebarAdvancedIdleHitstop) {
			entityFramebar.copyActiveDuringSuperfreeze(*destinationFrame, currentFrame);
		}
		entityFramebar.main.collectRequests(framebar, framebarAdvancedIdleHitstop, currentFrame);
	}
	if (framebarAdvancedIdleHitstop) {
		framebar.clearRequests();
	}
}

void EndScene::copyIdleHitstopFrameToTheRestOfSubframebars(ProjectileFramebar& entityFramebar,
		bool framebarAdvanced,
		bool framebarAdvancedIdle,
		bool framebarAdvancedHitstop,
		bool framebarAdvancedIdleHitstop) {
	
	Framebar& framebar = entityFramebar.idleHitstop;
	const int framebarPos = EntityFramebar::confinePos(currentState->framebarPositionHitstop + currentState->framebarIdleHitstopFor);
	Frame& currentFrame = (Frame&)framebar.getFrame(framebarPos);  // the frame should exist
	Frame* destinationFrame;
	
	if (framebarAdvancedHitstop) {
		if (entityFramebar.hitstop.stateHead->idleTime) {
			entityFramebar.hitstop.convertIdleTimeToFrames(currentState->framebarPositionHitstop,
				currentState->framebarTotalFramesHitstopUnlimited - 1);
		}
		entityFramebar.hitstop.advance(currentState->framebarPositionHitstop, currentState->framebarTotalFramesHitstopUnlimited);
		destinationFrame = &entityFramebar.hitstop.makeSureFrameExists(currentState->framebarPositionHitstop);
		entityFramebar.copyFrame(*destinationFrame, currentFrame);
		entityFramebar.hitstop.processRequests(*destinationFrame);
	} else {
		destinationFrame = &entityFramebar.hitstop.makeSureFrameExists(currentState->framebarPositionHitstop);
		if (!framebarAdvancedIdleHitstop) {
			entityFramebar.copyActiveDuringSuperfreeze(*destinationFrame, currentFrame);
		}
		entityFramebar.hitstop.collectRequests(framebar, framebarAdvancedIdleHitstop, currentFrame);
	}
	int idlePos = EntityFramebar::confinePos(currentState->framebarPosition + currentState->framebarIdleFor);
	if (framebarAdvancedIdle) {
		if (entityFramebar.idle.stateHead->idleTime) {
			entityFramebar.idle.convertIdleTimeToFrames(idlePos,
				currentState->framebarTotalFramesUnlimited + currentState->framebarIdleFor - 1);
		}
		entityFramebar.idle.advance(idlePos, currentState->framebarTotalFramesUnlimited + currentState->framebarIdleFor);
		destinationFrame = &entityFramebar.idle.makeSureFrameExists(idlePos);
		entityFramebar.copyFrame(*destinationFrame, currentFrame);
		entityFramebar.idle.processRequests(*destinationFrame);
	} else {
		destinationFrame = &entityFramebar.idle.makeSureFrameExists(idlePos);
		if (!framebarAdvancedIdleHitstop) {
			entityFramebar.copyActiveDuringSuperfreeze(*destinationFrame, currentFrame);
		}
		entityFramebar.idle.collectRequests(framebar, framebarAdvancedIdleHitstop, currentFrame);
	}
	if (framebarAdvanced) {
		if (entityFramebar.main.stateHead->idleTime) {
			entityFramebar.main.convertIdleTimeToFrames(currentState->framebarPosition,
				currentState->framebarTotalFramesUnlimited - 1);
		}
		entityFramebar.main.advance(currentState->framebarPosition, currentState->framebarTotalFramesUnlimited);
		destinationFrame = &entityFramebar.main.makeSureFrameExists(currentState->framebarPosition);
		entityFramebar.copyFrame(*destinationFrame, currentFrame);
		entityFramebar.main.processRequests(*destinationFrame);
	} else {
		destinationFrame = &entityFramebar.main.makeSureFrameExists(currentState->framebarPosition);
		if (!framebarAdvancedIdleHitstop) {
			entityFramebar.copyActiveDuringSuperfreeze(*destinationFrame, currentFrame);
		}
		entityFramebar.main.collectRequests(framebar, framebarAdvancedIdleHitstop, currentFrame);
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

void EndScene::collectFrameCancelsPart(PlayerInfo& player, std::vector<GatlingOrWhiffCancelInfo>& vec, const AddedMoveData* move,
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
	for (const GatlingOrWhiffCancelInfo& cancel : currentState->players[playerIndex].wasCancels.gatlings) {
		if (strcmp(cancel.move->name, name) == 0) {
			return true;
		}
	}
	return false;
}

bool EndScene::wasPlayerHadWhiffCancel(int playerIndex, const char* name) {
	for (const GatlingOrWhiffCancelInfo& cancel : currentState->players[playerIndex].wasCancels.whiffCancels) {
		if (strcmp(cancel.move->name, name) == 0) {
			return true;
		}
	}
	return false;
}

bool EndScene::wasPlayerHadGatlings(int playerIndex) {
	return !currentState->players[playerIndex].wasCancels.gatlings.empty();
}

bool EndScene::wasPlayerHadWhiffCancels(int playerIndex) {
	return !currentState->players[playerIndex].wasCancels.whiffCancels.empty();
}

EntityFramebar& EndScene::getFramebar(int totalIndex) {
	if (totalIndex >= (int)playerFramebars.size()) {
		return *projectileFramebars[totalIndex - playerFramebars.size()];
	} else {
		return playerFramebars[totalIndex];
	}
}

int EndScene::getSuperflashCounterOpponentCached() {
	return currentState->superflashCounterOpponent;
}

int EndScene::getSuperflashCounterAlliedCached() {
	return currentState->superflashCounterAllied;
}

int EndScene::getSuperflashCounterOpponentMax() {
	return currentState->superflashCounterOpponentMax;
}

int EndScene::getSuperflashCounterAlliedMax() {
	return currentState->superflashCounterAlliedMax;
}

Entity EndScene::getLastNonZeroSuperflashInstigator() {
	return currentState->lastNonZeroSuperflashInstigator;
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
	if (!defenderPtr.isPawn()) return;
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
	if (!defenderPtr.isPawn()) return;
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
	if (!defenderPtr.isPawn()) return;
	if (!defenderPtr.enableBlock() && !defenderPtr.inHitstun()
			&& (
				settings.showPunishMessageOnWhiff
				&& (
					defenderPtr.currentHitNum() > 0  // non-0 after any 'hit' instruction
					&& (
						defenderPtr.isRecoveryState()
						|| defenderPtr.hitboxes()->count[HITBOXTYPE_HITBOX] == 0
						|| !defenderPtr.isActiveFrames()  // can be turned off using 'attackOff'
						|| defenderPtr.hitAlreadyHappened() == defenderPtr.theValueHitAlreadyHappenedIsComparedAgainst()  // already landed the hit
						&& !defenderPtr.hitSomethingOnThisFrame()  // but landed the hit not on this frame, to avoid showing Punish on trades
					)
					|| defenderPtr.cmnActIndex() == CmnActJumpLanding
					|| defenderPtr.cmnActIndex() == CmnActLandingStiff
					|| defenderPtr.currentAnimDuration() > 15
					|| defenderPtr.isRecoveryState()  // some non-attack moves signal recovery state explicitly
				)
				&& defenderPtr.cmnActIndex() != CmnActKizetsu
				&& strcmp(defenderPtr.animationName(), "ExKizetsu") != 0
				|| settings.showPunishMessageOnBlock
				&& currentState->attackerInRecoveryAfterBlock[defenderPtr.team()]
			)
			// exclude Blitz rejection
			&& !(
				attackerPtr.isPawn()
				&& strncmp(attackerPtr.animationName(), "CounterGuard", 12) == 0
				&& attackerPtr.dealtAttack()->collisionForceExpand()
			)
			// exclude dashing, backdashing, airdashing
			&& !(
				defenderPtr.cmnActIndex() == CmnActFDash
				|| defenderPtr.cmnActIndex() == CmnActBDash
				|| defenderPtr.cmnActIndex() == CmnActAirFDash
				|| defenderPtr.cmnActIndex() == CmnActAirBDash
			)
			// exclude Leo Backturn idle, except the 22 (return from Backturn to neutral). Walk left/right is part of Backturn.
			// Backturn backdash and forward dash are separate animations.
			&& !leoHasbtDAvailable(defenderPtr)
	) {
		std::vector<EndSceneStoredState::PunishMessageTimer>& vec = currentState->punishMessageTimers[defenderPtr.team()];
		vec.emplace_back();
		EndSceneStoredState::PunishMessageTimer& newTimer = vec.back();
		newTimer.animFrame = 0;
		newTimer.animFrameRepeatCount = punishAnim[0].repeatCount;
	}
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
					attacker.wasCancels.clearDelays();
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
					newComboElem.player_onFirstHitHappenedBeforeFrame3(attacker, dmgCalc, getAswEngineTick(),
						isNormalThrow);
					attacker.wasCancels.clearDelays();  // for Combo Recipe panel Haehyun shishinken: otherwise displays that there's a delay between the shi and the shinken
				} else if (attacker.lastPerformedMoveNameIsInComboRecipe
						&& lastElem && lastElem->hitCount > 0) {
					++lastElem->hitCount;
					attacker.timeSinceWasEnableJumpCancel = 0;
					attacker.timeSinceWasEnableSpecialCancel = 0;
					attacker.timeSinceWasEnableSpecials = 0;
					attacker.wasCancels.clearDelays();
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
		inputRingBuffersStored[i].update(sourceBuffers[i], currentState->prevInputRingBuffers[i], currentTime);
		std::vector<InputsDrawingCommandRow>& result = drawDataPrepared.inputs[i];
		if (result.size() != 100) result.resize(100);
		drawDataPrepared.inputsSize[i] = 0;
		memset(result.data(), 0, 100 * sizeof InputsDrawingCommandRow);
		inputsDrawing.produceData(inputRingBuffersStored[i], result.data(), drawDataPrepared.inputsSize + i, i == 1, withDurations);
		drawDataPrepared.inputsContainsDurations = withDurations;
	}
	memcpy(currentState->prevInputRingBuffers, sourceBuffers, sizeof currentState->prevInputRingBuffers);
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
	
	frameCleanup();
	// since this function is one of the first things that run in a game logic tick,
	// we are also using it as the starting point of the logic tick in general,
	// where we advance our state pointer, clone states and register rollback.
	if (game.getGameMode() == GAME_MODE_NETWORK && game.getPlayerSide() != 2) {
		int currentStateInd = currentState - stateRingBuffer.data();
		if (stateRingBuffer.size() < ROLLBACK_MAX) {
			stateRingBuffer.resize(ROLLBACK_MAX);
		}
		currentState = stateRingBuffer.data() + currentStateInd;
		DWORD aswTick = getAswEngineTick();
		if (aswTick <= currentState->prevAswEngineTickCount) {
			loadState(findState(aswTick - 1));
		}
		EndSceneStoredState* oldCurrentState = currentState;
		currentState = incrementStatePointer(currentState);
		cloneState(currentState, oldCurrentState);
		++stateCount;
		if (stateCount > (int)stateRingBuffer.size()) {
			stateCount = stateRingBuffer.size();
		}
	}
	
	for (PlayerInfo& player : currentState->players) {
		player.rcSlowedDown = false;
		player.rcSlowedDownCounter = 0;
		player.rcSlowedDownMax = 0;
	}
	for (ProjectileInfo& projectile : currentState->projectiles) {
		projectile.rcSlowedDown = false;
		projectile.rcSlowedDownCounter = 0;
		projectile.rcSlowedDownMax = 0;
	}
	
	entityList.populate();
	Entity superflashInstigator = getSuperflashInstigator();
	for (int i = 0; i < 2; ++i) {
		PlayerInfo& player = currentState->players[i];
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

// Kinda copy of handleVenomBalls function, with some filters which are explained in comments
// The original function runs at the start of a logic tick, but we run it at the end of the current tick, which is probably the exact same
// Runs on the main thread
void EndScene::checkVenomBallActivations() {
	if (!isSignVer1_10OrHigher) return;
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
	for (int i = 0; i < entityList.count; ++i) {
		Entity attacker = entityList.list[i];
		BBScrEvent event = attacker.signalToSendToYourOwnEffectsWhenHittingThem();
		if (event == BBSCREVENT_HIT_OWN_PROJECTILE_DEFAULT
				|| attacker.hitboxes()->count[HITBOXTYPE_HITBOX] == 0 && !attacker.effectLinkedCollision()) {
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
		int remainder = _countof(PlayerFramebar::frames) % displayedFrames;
		counterUnlimited = _countof(PlayerFramebar::frames) + displayedFrames - remainder
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
		currentState->reachedMaxStun[pawn.team()] = pawn.stun() + stunAdd;
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
	if (*aswEngine) {
		InputRingBuffer* sourceBuffers = game.getInputRingBuffers();
		memcpy(currentState->prevInputRingBuffers, sourceBuffers, sizeof currentState->prevInputRingBuffers);
		DWORD currentTime = resetClearTime ? 0xFFFFFFFF : getAswEngineTick();
		for (int i = 0; i < 2; ++i) {
			inputRingBuffersStored[i].clear();
			inputRingBuffersStored[i].lastClearTime = currentTime;
		}
	} else {
		for (int i = 0; i < 2; ++i) {
			inputRingBuffersStored[i].clear();
			inputRingBuffersStored[i].lastClearTime = 0xFFFFFFFF;
		}
	}
}

void EndScene::analyzeGunflame(PlayerInfo& player, bool* wholeGunflameDisappears,
		bool* firstWaveEntirelyDisappears, bool* firstWaveDisappearsDuringItsActiveFrames) {
	bool hasFirstWave = false;
	bool firstWaveActive = false;
	bool hasNonLinked = false;
	int team = player.index;
	for (const ProjectileInfo& projectile : currentState->projectiles) {
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
	for (const ProjectileInfo& projectile : currentState->projectiles) {
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
	for (const ProjectileInfo& projectile : currentState->projectiles) {
		if (projectile.team == team && strncmp(projectile.animName, name, strn) == 0 && projectile.ptr
				&& projectile.isDangerous) {
			return true;
		}
	}
	return false;
}

bool EndScene::hasProjectileOfType(PlayerInfo& player, const char* name) {
	int team = player.index;
	for (const ProjectileInfo& projectile : currentState->projectiles) {
		if (projectile.team == team && strcmp(projectile.animName, name) == 0 && projectile.ptr) {
			return projectile.isDangerous;
		}
	}
	return false;
}

char EndScene::hasBoomerangHead(PlayerInfo& player) {
	int team = player.index;
	for (const ProjectileInfo& projectile : currentState->projectiles) {
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
	for (const ProjectileInfo& projectile : currentState->projectiles) {
		if (projectile.team == team && strcmp(projectile.animName, name) == 0 && projectile.ptr
				&& projectile.isDangerous) {
			return true;
		}
	}
	return false;
}

bool EndScene::hasProjectileOfTypeAndHasNotExhausedHit(PlayerInfo& player, const char* name) {
	int team = player.index;
	for (const ProjectileInfo& projectile : currentState->projectiles) {
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
	for (const ProjectileInfo& projectile : currentState->projectiles) {
		if (projectile.team == team && strncmp(projectile.animName, name, strn) == 0 && projectile.ptr) {
			return projectile.isDangerous;
		}
	}
	return false;
}

void EndScene::removeAttackHitbox(Entity attackerPtr) {
	auto itEnd = currentState->attackHitboxes.end();
	for (auto it = currentState->attackHitboxes.begin(); it != itEnd; ++it) {
		if (it->ent == attackerPtr) {
			currentState->attackHitboxes.erase(it);
			return;
		}
	}
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
			&& ent.hitboxes()->count[HITBOXTYPE_HITBOX] > 0
	) {
		static std::vector<DrawHitboxArrayCallParams> hitboxesArena;
		EndSceneStoredState::AttackHitbox attackBox;
		
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
			for (EndSceneStoredState::AttackHitbox& existingBox : currentState->attackHitboxes) {
				if (existingBox.ent == ent && existingBox.hitbox == attackBox.hitbox) {
					existingBox.clash = existingBox.clash || attackBox.clash;
					existingBox.notClash = existingBox.notClash || attackBox.notClash;
					found = true;
					break;
				}
			}
			
			if (!found) {
				currentState->attackHitboxes.push_back(attackBox);
			}
		}
		hitboxesArena.clear();
	}
}

bool EndScene::objHasAttackHitboxes(Entity ent) const {
	if (!ent) return false;
	for (const EndSceneStoredState::AttackHitbox& attackHitbox : currentState->attackHitboxes) {
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
	
	for (ProjectileInfo& projectile : currentState->projectiles) {
		
		if (!(
				projectile.team == player.index
				&& strncmp(projectile.animName, "DejavIcon", 9) == 0
				&& projectile.ptr
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
	for (const ProjectileInfo& projectile : currentState->projectiles) {
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
	
	bool wasInTransaction = detouring.isInTransaction();
	if (!wasInTransaction) detouring.beginTransaction(false);
	
	auto Pawn_ArcadeMode_IsBossHookPtr = &HookHelp::Pawn_ArcadeMode_IsBossHook;
	if (!attach(&(PVOID&)orig_Pawn_ArcadeMode_IsBoss,
		(PVOID&)Pawn_ArcadeMode_IsBossHookPtr,
		"Pawn_ArcadeMode_IsBoss")) return false;
	
	if (!wasInTransaction) detouring.endTransaction();
	
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
	for (ProjectileInfo& projectile : currentState->projectiles) {
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
	
	for (ProjectileInfo& projectile : currentState->projectiles) {
		if (projectile.team == player.index && projectile.ptr) {
			const char* anim = projectile.ptr.animationName();
			auto found = dizzyMap.find(anim);
			if (found != dizzyMap.end()) {
				found->second(di, projectile);
			}
		}
	}
	
	di.shieldFishSuperArmor = player.dizzyShieldFishSuperArmor;
	
}

BOOL EndScene::clashHitDetectionCallHook(Entity attacker, Entity defender, HitboxType hitboxIndex, HitboxType defenderHitboxIndex, int* intersectionXPtr, int* intersectionYPtr) {
	BOOL result = hitDetectionFunc((void*)attacker.ent, (void*)defender.ent, hitboxIndex, defenderHitboxIndex, intersectionXPtr, intersectionYPtr);
	if (result) {
		int attackerLvl = attacker.dealtAttack()->projectileLvl;
		int defenderLvl = defender.dealtAttack()->projectileLvl;
		if (attackerLvl > 0 && defenderLvl > 0
				&& (attackerLvl < 2 || defenderLvl < 2)) {
			Entity victim;
			if (attackerLvl == defenderLvl) {
				onProjectileHit(attacker);
				onProjectileHit(defender);
			} else if (attackerLvl < defenderLvl) {
				onProjectileHit(defender);
				victim = attacker;
			} else {
				onProjectileHit(attacker);
				victim = defender;
			}
			if (victim) {
				ProjectileInfo& victimProjectile = findProjectile(victim);
				if (victimProjectile.ptr) {
					victimProjectile.fill(victim, getSuperflashInstigator(), false);
					victimProjectile.gotHitOnThisFrame = true;
				}
			}
		}
	}
	return result;
}

void __cdecl activeFrameHitReflectHook(void* attacker, void* defender, int percentage) {
	endScene.activeFrameHitReflectMultiplySpeedXHook(Entity{attacker}, Entity{defender}, percentage);
}

void EndScene::activeFrameHitReflectMultiplySpeedXHook(Entity attacker, Entity defender, int percentage) {
	if (!attacker.isPawn()) {
		onProjectileHit(attacker);
	}
	if (defender.isPawn()) {
		PlayerInfo& player = findPlayer(defender);
		if (player.pawn) {
			player.gotHitOnThisFrame = true;
		}
	} else {
		ProjectileInfo& defenderProjectile = findProjectile(defender);
		if (defenderProjectile.ptr) {
			defenderProjectile.fill(defender, getSuperflashInstigator(), false);
			defenderProjectile.gotHitOnThisFrame = true;
		}
	}
	multiplySpeedX((void*)attacker.ent, percentage);
}

bool EndScene::highlightGreenWhenBecomingIdleChanged() {
	if (!settings.highlightRedWhenBecomingIdle
			&& !settings.highlightGreenWhenBecomingIdle
			&& !settings.highlightBlueWhenBecomingIdle
			&& settings.highlightWhenCancelsIntoMovesAvailable.pointers.empty()) return true;
	static bool conductedSearch = false;
	if (!conductedSearch) {
		conductedSearch = true;
		uintptr_t place = sigscanOffset(GUILTY_GEAR_XRD_EXE,
				"8b f1 8b 5e 10 8b 08 8a 86 00 04 00 00 f7 db",
				nullptr, "pawnGetColor");
		if (!place) return false;
		orig_pawnGetColor = (pawnGetColor_t)sigscanBackwards16ByteAligned(place, "83 ec");
		if (!detouring.isInTransaction()) finishedSigscanning();
	}
	if (!orig_pawnGetColor) return false;
	
	bool wasInTransaction = detouring.isInTransaction();
	if (!wasInTransaction) detouring.beginTransaction(false);
	
	auto pawnGetColorHookPtr = &HookHelp::pawnGetColorHook;
	bool result = attach(&(PVOID&)orig_pawnGetColor,
		(PVOID&)pawnGetColorHookPtr,
		"pawnGetColor");
	
	if (!wasInTransaction) detouring.endTransaction();
	return result;
}

DWORD EndScene::HookHelp::pawnGetColorHook(DWORD* inColor) {
	return endScene.pawnGetColorHook(Entity{(void*)this}, inColor);
}

DWORD EndScene::pawnGetColorHook(Entity pawn, DWORD* inColor) {
	if (*game.gameDataPtr && game.isTrainingMode()) {
		PlayerInfo& player = findPlayer(pawn);
		if (player.pawn && player.overrideColor) {
			*inColor = 0xFFFFFFFF;
			return player.colorOverride;
		}
	}
	return orig_pawnGetColor((void*)pawn.ent, inColor);
}

bool EndScene::hasCancelUnlocked(CharacterType charType, const std::vector<GatlingOrWhiffCancelInfo>& array, std::vector<const AddedMoveData*>& markedMoves,
		bool* redPtr, bool* greenPtr, bool* bluePtr) {
	for (const GatlingOrWhiffCancelInfo& elem : array) {
		bool isIncluded = false;
		for (const AddedMoveData* ptr : markedMoves) {
			if (elem.move == ptr) {
				isIncluded = true;
				break;
			}
		}
		if (!isIncluded) {
			markedMoves.push_back(elem.move);
			if (elem.framesBeenAvailableFor <= 1 && elem.foundOnThisFrame) {
				auto it = highlightMoveCache.find(elem.move);
				if (it == highlightMoveCache.end()) {
					for (const MoveListPointer& ptr : settings.highlightWhenCancelsIntoMovesAvailable.pointers) {
						if (ptr.charType == charType
								&& strcmp(ptr.name, elem.move->name) == 0) {
							highlightMoveCache[elem.move] = { true, ptr.red, ptr.green, ptr.blue };
							*redPtr = ptr.red;
							*greenPtr = ptr.green;
							*bluePtr = ptr.blue;
							return true;
						}
					}
					highlightMoveCache[elem.move] = { false };
				} else if (it->second.needHighlight) {
					*redPtr = it->second.red;
					*greenPtr = it->second.green;
					*bluePtr = it->second.blue;
					return true;
				}
			}
		}
	}
	return false;
}

void EndScene::processColor(PlayerInfo& player) {
	DWORD color = 0xFF000000;
	if (player.redHighlightTimer) {
		player.redHighlight = greenHighlights[sizeof greenHighlights - player.redHighlightTimer];
		--player.redHighlightTimer;
		color |= player.redHighlight << 16;
	}
	if (player.greenHighlightTimer) {
		player.greenHighlight = greenHighlights[sizeof greenHighlights - player.greenHighlightTimer] >> 1;  // green goes quite a bit harder than blue for some reason
		--player.greenHighlightTimer;
		color |= player.greenHighlight << 8;
	}
	if (player.blueHighlightTimer) {
		player.blueHighlight = greenHighlights[sizeof greenHighlights - player.blueHighlightTimer];
		--player.blueHighlightTimer;
		color |= player.blueHighlight;
	}
	if (player.redHighlightTimer || player.greenHighlightTimer || player.blueHighlightTimer) {
		player.overrideColor = true;
		player.colorOverride = color;
	} else {
		player.overrideColor = false;
	}
}

void EndScene::highlightSettingsChanged() {
	highlightMoveCache.clear();
}

LONG volatile FRenderCommand::totalCountOfCommandsInCirculation = 0;

FRenderCommand::FRenderCommand() {
	InterlockedIncrement(&totalCountOfCommandsInCirculation);
}

// only return critical error as false, the rest as true
bool EndScene::onDontResetBurstAndTensionGaugesWhenInStunOrFaintChanged() {
	if (!settings.dontResetBurstAndTensionGaugesWhenInStunOrFaint
			|| burstGaugeResettingHookAttempted) return true;
	burstGaugeResettingHookAttempted = true;
	uintptr_t beforeFrameStepPlaceWhereTheyResetGauges = sigscanOffset(
		GUILTY_GEAR_XRD_EXE,
		"8b 7e 40 6a 00 6a 08 e8",
		nullptr, "beforeFrameStepPlaceWhereTheyResetGauges");
	if (!beforeFrameStepPlaceWhereTheyResetGauges) return true;
	std::vector<char> sig;
	std::vector<char> mask;
	std::vector<char> maskForCaching;
	byteSpecificationToSigMask("e8 ?? ?? ?? ?? 85 c0 0f 84 ?? ?? ?? ??", sig, mask, nullptr, 0, &maskForCaching);
	uintptr_t trainingModeAndNoOneInXStunOrThrowInvulFromStunOrAirborneOrAttackingCallPlace = sigscan(
			beforeFrameStepPlaceWhereTheyResetGauges - 13,
			beforeFrameStepPlaceWhereTheyResetGauges,
			sig.data(),
			mask.data(),
			"trainingModeAndNoOneInXStunOrThrowInvulFromStunOrAirborneOrAttacking",
			maskForCaching.data());
	if (!trainingModeAndNoOneInXStunOrThrowInvulFromStunOrAirborneOrAttackingCallPlace) return true;
	// the orig_trainingModeAndNoOneInXStunOrThrowInvulFromStunOrAirborneOrAttacking function checks if both players are 'idle', but the check is incomplete
	orig_trainingModeAndNoOneInXStunOrThrowInvulFromStunOrAirborneOrAttacking =
		(trainingModeAndNoOneInXStunOrThrowInvulFromStunOrAirborneOrAttacking_t)followRelativeCall(
			trainingModeAndNoOneInXStunOrThrowInvulFromStunOrAirborneOrAttackingCallPlace);
	auto trainingModeAndNoOneInXStunOrThrowInvulFromStunOrAirborneOrAttackingHookPtr =
		&HookHelp::trainingModeAndNoOneInXStunOrThrowInvulFromStunOrAirborneOrAttackingHook;
	bool wasInTransaction = detouring.isInTransaction();
	if (!wasInTransaction) {
		finishedSigscanning();
		detouring.beginTransaction(false);
	}
	bool result = attach(&(PVOID&)orig_trainingModeAndNoOneInXStunOrThrowInvulFromStunOrAirborneOrAttacking,
		(PVOID&)trainingModeAndNoOneInXStunOrThrowInvulFromStunOrAirborneOrAttackingHookPtr,
		"trainingModeAndNoOneInXStunOrThrowInvulFromStunOrAirborneOrAttacking");
	if (!wasInTransaction) detouring.endTransaction();
	return result;
}

BOOL EndScene::HookHelp::trainingModeAndNoOneInXStunOrThrowInvulFromStunOrAirborneOrAttackingHook() {
	return endScene.trainingModeAndNoOneInXStunOrThrowInvulFromStunOrAirborneOrAttackingHook(Entity{(void*)this});
}

BOOL EndScene::trainingModeAndNoOneInXStunOrThrowInvulFromStunOrAirborneOrAttackingHook(Entity ent) {
	if (game.isTrainingMode() && settings.dontResetBurstAndTensionGaugesWhenInStunOrFaint) {
		if (!findPlayer(ent).isIdle() || !findPlayer(ent.enemyEntity()).isIdle()) {
			return FALSE;
		}
	}
	return orig_trainingModeAndNoOneInXStunOrThrowInvulFromStunOrAirborneOrAttacking((void*)ent.ent);
}

// only return critical error as false, the rest as true
bool EndScene::onDontResetRiscWhenInBurstOrFaintChanged() {
	if (!settings.dontResetRISCWhenInBurstOrFaint
			|| riscGaugeResettingHookAttempted) return true;
	riscGaugeResettingHookAttempted = true;
	orig_inHitstunBlockstunOrThrowProtectionOrDead = (inHitstunBlockstunOrThrowProtectionOrDead_t)sigscanOffset(
		GUILTY_GEAR_XRD_EXE,
		"8b 81 3c 02 00 00 a8 04 75 20 83 b9 54 4d 00 00 00 7f 17 83 b9 e4 9f 00 00 00 7f 0e a8 02 75 0a",
		nullptr, "inHitstunBlockstunOrThrowProtectionOrDead");
	if (!orig_inHitstunBlockstunOrThrowProtectionOrDead) return true;
	bool wasInTransaction = detouring.isInTransaction();
	if (!wasInTransaction) {
		finishedSigscanning();
		detouring.beginTransaction(false);
	}
	auto inHitstunBlockstunOrThrowProtectionOrDeadHookPtr = &HookHelp::inHitstunBlockstunOrThrowProtectionOrDeadHook;
	bool result = attach(&(PVOID&)orig_inHitstunBlockstunOrThrowProtectionOrDead,
		(PVOID&)inHitstunBlockstunOrThrowProtectionOrDeadHookPtr,
		"inHitstunBlockstunOrThrowProtectionOrDead");
	if (!wasInTransaction) detouring.endTransaction();
	return result;
}

BOOL EndScene::HookHelp::inHitstunBlockstunOrThrowProtectionOrDeadHook() {
	return endScene.inHitstunBlockstunOrThrowProtectionOrDeadHook(Entity{(void*)this});
}

// only runs in training mode. Only used to decide whether to reset RISC in Training Mode
BOOL EndScene::inHitstunBlockstunOrThrowProtectionOrDeadHook(Entity ent) {
	if (!gifMode.modDisabled && settings.dontResetRISCWhenInBurstOrFaint && !findPlayer(ent).isIdle()) {
		return TRUE;
	}
	return orig_inHitstunBlockstunOrThrowProtectionOrDead((void*)ent.ent);
}

// only return critical error as false, the rest as true
bool EndScene::onOnlyApplyCounterhitSettingWhenDefenderNotInBurstOrFaintOrHitstunChanged() {
	if (!settings.onlyApplyCounterhitSettingWhenDefenderNotInBurstOrFaintOrHitstun
			|| attemptedToHookTheObtainingOfCounterhitTrainingSetting) return true;
	attemptedToHookTheObtainingOfCounterhitTrainingSetting = true;
	uintptr_t callPlace = sigscanOffset(
		GUILTY_GEAR_XRD_EXE,
		"55 6a 0b 89 6c 24 30 e8 ?? ?? ?? ?? 8b c8 >e8",
		nullptr, "obtainingOfCounterhitTrainingSetting");
	if (!callPlace) return true;
	if (!detouring.isInTransaction()) finishedSigscanning();
	getTrainingSetting_t whatIsThere = (getTrainingSetting_t)followRelativeCall(callPlace);
	if (!thisIsOurFunction((uintptr_t)whatIsThere)) {
		orig_getTrainingSetting = whatIsThere;
	}
	
	int offset = calculateRelativeCallOffset(callPlace, (uintptr_t)obtainingOfCounterhitTrainingSettingHookAsm);
	return overwriteCall(callPlace, offset);
}

int __cdecl obtainingOfCounterhitTrainingSettingHook(void* defender, void* trainingStruct, TrainingSettingId settingId, BOOL outsideTraining) {
	Entity defenderEnt { defender };
	if (defenderEnt && settingId == TRAINING_SETTING_COUNTER_HIT && !outsideTraining
			&& settings.onlyApplyCounterhitSettingWhenDefenderNotInBurstOrFaintOrHitstun
			&& (
				defenderEnt.cmnActIndex() == CmnActBurst
				|| defenderEnt.cmnActIndex() == CmnActKizetsu
			)
			&& !gifMode.modDisabled) {
		return 0;
	}
	return game.getTrainingSetting(settingId);
}

// only return critical error as false, the rest as true
bool EndScene::onStartingBurstGaugeChanged() {
	int valP1 = minmax(0, 15000, settings.startingBurstGaugeP1);
	int valP2 = minmax(0, 15000, settings.startingBurstGaugeP1);
	if (valP1 == 15000
			&& valP2 == 15000
			|| attemptedToHookBurstGaugeReset) return true;
	attemptedToHookBurstGaugeReset = true;
	uintptr_t callPlace = sigscanOffset(
		GUILTY_GEAR_XRD_EXE,
		"85 c0 74 0c 68 98 3a 00 00 8b ce >e8",
		nullptr, "burstGaugeReset");
	if (!callPlace) return true;
	if (!detouring.isInTransaction()) finishedSigscanning();
	addBurst = (addBurst_t)followRelativeCall(callPlace);
	auto resetBurstHookPtr = &HookHelp::resetBurstHook;
	int offset = calculateRelativeCallOffset(callPlace, (uintptr_t&)resetBurstHookPtr);
	return overwriteCall(callPlace, offset);
}

void EndScene::HookHelp::resetBurstHook(int amount) {
	endScene.resetBurstHook(Entity{(void*)this}, amount);
}

void EndScene::resetBurstHook(Entity pawn, int amout) {
	
	if (*gameModeFast != GAME_MODE_FAST_NORMAL || gifMode.modDisabled) {
		addBurst((void*)pawn.ent, 15000);
		return;
	}
		
	int team = pawn.team();
	int needed = team == 0 ? settings.startingBurstGaugeP1 : settings.startingBurstGaugeP2;
	needed = minmax(0, 15000, needed);
	if (needed == 15000) {
		addBurst((void*)pawn.ent, 15000);
		return;
	}
	int currentBurst = game.getBurst(team);
	int diff = needed - currentBurst;
	if (diff <= 0) return;
	addBurst((void*)pawn.ent, diff);
}

bool EndScene::overwriteCall(uintptr_t callInstr, int newOffset) {
	std::vector<char> newBytes(4);
	memcpy(newBytes.data(), &newOffset, 4);
	if (!detouring.patchPlace(callInstr + 1, newBytes)) {
		return false;
	}
	return true;
}

bool EndScene::attach(PVOID* ppPointer, PVOID pDetour, const char* name) {
	// it is possible that we're already in transaction, as this function also gets called from onDllMain
	bool wasTransaction = detouring.isInTransaction();
	
	// not freezing threads, because we moved UI.cpp::prepareDrawData to the main (logic) thread, and settings
	// changed by overwriting the mod's INI file directly post a window message, so on main thread as well,
	// and the function that we're about to hook only runs on the main thread
	if (!wasTransaction) {
		if (!detouring.beginTransaction(false)) return false;
	}
	if (!detouring.attach(ppPointer,
			pDetour,
			name)) {
		if (!wasTransaction) detouring.cancelTransaction();
		return false;
	}
	if (!wasTransaction) {
		detouring.endTransaction();
	}
	return true;
}

void EndScene::drawHitboxEditorHitboxes() {
	if (!gifMode.editHitboxes || !gifMode.editHitboxesEntity) return;
	drawDataPrepared.hurtboxes.clear();
	drawDataPrepared.hitboxes.clear();
	drawDataPrepared.circles.clear();
	drawDataPrepared.interactionBoxes.clear();
	drawDataPrepared.lines.clear();
	drawDataPrepared.points.clear();
	drawDataPrepared.pushboxes.clear();
	drawDataPrepared.throwBoxes.clear();
	
	aswOneScreenPixelWidth = 0;
	aswOneScreenPixelHeight = 0;
	HitboxEditorCameraValues& vals = hitboxEditorCameraValues;
	if (vals.prepare(Entity{gifMode.editHitboxesEntity})) {
		aswOneScreenPixelWidth = (int)std::ceilf(1.F / vals.vw * 2.F / vals.xCoeff);
		aswOneScreenPixelHeight = (int)std::ceilf(1.F / vals.vh * 2.F / vals.yCoeff);
	}
	aswOneScreenPixelDiameter = max(aswOneScreenPixelWidth, aswOneScreenPixelHeight);
	
	if (settings.hitboxEditShowHitboxesOfEntitiesOtherThanTheOneBeingEdited) {
		for (int i = 0; i < entityList.count; ++i) {
			Entity ent = entityList.list[i];
			drawHitboxEditorHitboxesForEntity(ent);
		}
	} else {
		drawHitboxEditorHitboxesForEntity(gifMode.editHitboxesEntity);
	}
	
	if (settings.hitboxEditShowFloorline) {
		drawDataPrepared.lines.emplace_back();
		DrawLineCallParams& newLine = drawDataPrepared.lines.back();
		newLine.color = 0xFFFFFFFF;
		newLine.posX1 = -10000000;
		newLine.posX2 =  10000000;
		newLine.posY1 = 0;
		newLine.posY2 = 0;
	}
	
}

void EndScene::drawHitboxEditorPushbox(Entity ent, int posY) {
	int pushboxTop;
	int pushboxBottom;
	int pushboxLeft;
	int pushboxRight;
	ent.pushboxDimensions(&pushboxLeft, &pushboxTop, &pushboxRight, &pushboxBottom);
	
	if (!ent.isPawn() && pushboxTop - posY == 100 && pushboxBottom == posY) {
		pushboxTop = 20000;
	}
	
	drawDataPrepared.pushboxes.emplace_back();
	DrawBoxCallParams& newPushbox = drawDataPrepared.pushboxes.back();
	newPushbox.left = pushboxLeft;
	newPushbox.right = pushboxRight;
	newPushbox.top = pushboxTop;
	newPushbox.bottom = pushboxBottom;
	newPushbox.fillColor = replaceAlpha(64, COLOR_PUSHBOX);
	newPushbox.outlineColor = replaceAlpha(255, COLOR_PUSHBOX);
	newPushbox.thickness = 1;
	newPushbox.hatched = false;
}

void EndScene::drawHitboxEditorHitboxesForEntity(Entity ent) {
	if (ent.scaleDefault() == 0 && ent != gifMode.editHitboxesEntity) return;
	
	int posX = ent.posX();
	int posY = ent.posY();
	
	if (settings.hitboxEditShowOriginPoints && ent.isPawn()) {
		drawDataPrepared.points.emplace_back();
		DrawPointCallParams& newPoint = drawDataPrepared.points.back();
		newPoint.posX = posX;
		newPoint.posY = posY;
	}
	
	if (ent.scaleDefault() == 0) return;
	
	DrawHitboxArrayCallParams projector;
	
	DrawHitboxArrayParams& params = projector.params;
	params.angle = ent.pitch();
	params.flip = ent.isFacingLeft() ? 1 : -1;
	params.posX = posX;
	params.posY = posY;
	params.scaleX = ent.scaleX();
	params.scaleY = ent.scaleY();
	params.transformCenterX = ent.transformCenterX();
	params.transformCenterY = ent.transformCenterY();
	
	bool showPushbox = ent.showPushbox();
	
	LayerIterator layerIterator = ui.getEntityLayers(ent);
	
	bool isEditEntity = ent.ent == (char*)gifMode.editHitboxesEntity;
	
	while (layerIterator.getNext()) {
		HitboxListElement& setting = settings.hitboxList[layerIterator.type];
		if (!setting.show) continue;
		if (layerIterator.isPushbox) {
			if (showPushbox) {
				hitboxEditorDrawPushbox(ent, posX, posY, setting.color);
			}
		} else {
			hitboxEditorDrawBox(projector, posX, posY, layerIterator.ptr, setting.color,
				isEditEntity ? ui.hitboxIsSelectedForEndScene(layerIterator.originalIndex) : false,
				isEditEntity ? ui.hitboxHoveredPart(layerIterator.originalIndex) : BOXPART_NONE);
		}
	}
	
	if (isEditEntity) {
		RECT overallBounds;
		BoxPart overallHoverPart;
		if (ui.hasOverallSelectionBox(&overallBounds, &overallHoverPart)) {
			hitboxEditorDrawBox(&overallBounds, posX, posY, 0, 0xFF000000, true, overallHoverPart, true);
		}
	}
	
}

void EndScene::drawPunishMessage(float x, float y, DrawTextWithIconsAlignment alignment, DWORD textColor) {
	char message[] = "PUNISH";
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
	s.textColor = textColor;  // text color
	s.field164_0x114 = 0;
	s.field165_0x118 = 0;
	s.field166_0x11c = -1;
	BYTE blue = textColor & 0xFF;
	BYTE alpha = (BYTE)((textColor & 0xFF000000) >> 24);
	s.outlineColor = ((DWORD)alpha << 24) | ((DWORD)blue << 16) | ((DWORD)blue << 8) | blue;
	s.dropShadowColor = s.outlineColor;
	s.x = x;
	s.y = y;
	s.alignment = alignment;
	s.text = message;
	s.flags1 = 0x2612;
	drawTextWithIcons(&s,0x0,1,4,0,0);
}

bool EndScene::HitboxEditorCameraValues::prepare(Entity ent) {
	
	cam_y = camera.editHitboxesViewDistance;
	if (cam_y > 0.001F || cam_y < -0.001F) {
		
		fov = camera.valuesUse.fov;
		t = 1.F / tanf(fov / 360.F * PI);
		vw = graphics.viewportW;
		vh = graphics.viewportH;
		
		// all ArcSys points must be first scaled by m to get them in UE3 coordinate space's scale
		m = camera.valuesUse.coordCoefficient / 1000.F;
		
		// multiply ArcSys displacements by this to get screen (1;-1) displacements
		xCoeff = t/cam_y*m;
		yCoeff = t*vw/vh/cam_y*m;
		
		screenWidthASW = 2.F / xCoeff;
		screenHeightASW = 2.F / yCoeff;
		
		cam_xASW = ent.posX() + camera.editHitboxesOffsetX / m;
		cam_zASW = ent.posY() + camera.editHitboxesOffsetY / m;
		
		ready = true;
	} else {
		ready = false;
	}
	
	return ready;
}

bool colorIsLess(DWORD colorProtagonist, DWORD colorToCompareAgainst) {
	BYTE colorProtOne = colorProtagonist & 0xFF;
	BYTE colorProtTwo = (colorProtagonist >> 8) & 0xFF;
	BYTE colorProtThree = (colorProtagonist >> 16) & 0xFF;
	BYTE colorAgstOne = colorToCompareAgainst & 0xFF;
	BYTE colorAgstTwo = (colorToCompareAgainst >> 8) & 0xFF;
	BYTE colorAgstThree = (colorToCompareAgainst >> 16) & 0xFF;
	return colorProtOne < colorAgstOne
		&& colorProtTwo < colorAgstTwo
		&& colorProtThree < colorAgstThree;
}

bool leoHasbtDAvailable(Entity pawn) {
	if (pawn.characterType() != CHARACTER_TYPE_LEO) return false;
	if (strcmp(pawn.animationName(), "Semuke") != 0) return false;
	const AddedMoveData* move = (const AddedMoveData*)findMoveByName(pawn, "Semuke5E", 0);
	if (!move) return false;
	return move->whiffCancelOption();
}

void EndScene::hitboxEditorDrawBox(DrawHitboxArrayCallParams& projector, int posX, int posY,
				Hitbox* hitbox, DWORD color, bool selected, BoxPart highlightedPart) {
	
	RECT bounds = projector.getWorldBounds(*hitbox);
	hitboxEditorDrawBox(&bounds, posX, posY, replaceAlpha(64, color), replaceAlpha(255, color), selected, highlightedPart, false);
}

void EndScene::hitboxEditorDrawBox(const RECT* bounds, int posX, int posY, DWORD fillColor, DWORD outlineColor,
			bool selected, BoxPart highlightedPart, bool dashed) {
	drawDataPrepared.pushboxes.emplace_back();
	DrawBoxCallParams& newElem = drawDataPrepared.pushboxes.back();
	
	newElem.fillColor = fillColor;
	newElem.hatched = false;
	
	newElem.left = bounds->left;
	newElem.top = bounds->top;
	newElem.right = bounds->right;
	newElem.bottom = bounds->bottom;
	
	newElem.outlineColor = selected ? 0 : outlineColor;
	newElem.thickness = 1;
	newElem.originX = posX;
	newElem.originY = posY;
	
	newElem.dashed = dashed;
	
	if (newElem.dashed && selected) {
		
		drawDataPrepared.pushboxes.emplace_back();
		DrawBoxCallParams& newElem = drawDataPrepared.pushboxes.back();
		
		newElem.fillColor = 0;
		newElem.hatched = false;
		
		newElem.left = bounds->left - aswOneScreenPixelDiameter;
		newElem.top = bounds->top - aswOneScreenPixelDiameter;
		newElem.right = bounds->right - aswOneScreenPixelDiameter;
		newElem.bottom = bounds->bottom - aswOneScreenPixelDiameter;
		
		newElem.outlineColor = (~outlineColor & 0xFFFFFF) | (outlineColor & 0xFF000000);
		newElem.thickness = 1;
		newElem.originX = posX;
		newElem.originY = posY;
		
		newElem.dashed = dashed;
		
	} else if (selected) {
		
		drawDataPrepared.pushboxes.emplace_back();
		DrawBoxCallParams& thickestOutline = drawDataPrepared.pushboxes.back();
		thickestOutline = newElem;
		thickestOutline.fillColor = 0;
		
		thickestOutline.outlineColor = outlineColor;
		thickestOutline.thickness = aswOneScreenPixelDiameter * 3;
		
		drawDataPrepared.pushboxes.emplace_back();
		DrawBoxCallParams& midThickOutline = drawDataPrepared.pushboxes.back();
		midThickOutline = newElem;
		midThickOutline.fillColor = 0;
		
		midThickOutline.outlineColor = colorIsLess(0xFFFFFF - (newElem.outlineColor & 0xFFFFFF), 0x191919)
			? 0xFF000000 : 0xFFFFFFFF;
		midThickOutline.thickness = aswOneScreenPixelDiameter * 2;
		
		drawDataPrepared.pushboxes.emplace_back();
		DrawBoxCallParams& thinOutline = drawDataPrepared.pushboxes.back();
		thinOutline = newElem;
		thinOutline.fillColor = 0;
		
		thinOutline.outlineColor = outlineColor;
		thinOutline.thickness = 1;
		
	}
	
	if (bounds->right - bounds->left < 1500 || bounds->bottom - bounds->top < 1500) {
		drawDataPrepared.points.emplace_back();
		DrawPointCallParams& newPoint = drawDataPrepared.points.back();
		newPoint.posX = (bounds->left + bounds->right) >> 1;
		newPoint.posY = (bounds->top + bounds->bottom) >> 1;
		newPoint.isProjectile = true;
		newPoint.selected = selected;
		newPoint.fillColor = colorIsLess(0xFFFFFF - (outlineColor & 0xFFFFFF), 0x191919)
			? 0xFF000000 : 0xFFFFFFFF;
		newPoint.outlineColor = outlineColor;
	}
	
	if (highlightedPart != BOXPART_NONE) {
		enum MyHighlightType {
			MYHIGHLIGHTTYPE_NONE,
			MYHIGHLIGHTTYPE_POINT,
			MYHIGHLIGHTTYPE_LINE
		} myHighlightType = MYHIGHLIGHTTYPE_NONE;
		union {
			struct MyHighlightLine {
				int xStart;
				int yStart;
				int xEnd;
				int yEnd;
			} line;
			struct MyHighlightPoint {
				int x;
				int y;
			} point;
		} myHighlightUnion;
		switch (highlightedPart) {
			case BOXPART_TOPLEFT:
				myHighlightType = MYHIGHLIGHTTYPE_POINT;
				myHighlightUnion.point.x = newElem.left;
				myHighlightUnion.point.y = newElem.top;
				break;
			case BOXPART_TOPRIGHT:
				myHighlightType = MYHIGHLIGHTTYPE_POINT;
				myHighlightUnion.point.x = newElem.right;
				myHighlightUnion.point.y = newElem.top;
				break;
			case BOXPART_BOTTOMLEFT:
				myHighlightType = MYHIGHLIGHTTYPE_POINT;
				myHighlightUnion.point.x = newElem.left;
				myHighlightUnion.point.y = newElem.bottom;
				break;
			case BOXPART_BOTTOMRIGHT:
				myHighlightType = MYHIGHLIGHTTYPE_POINT;
				myHighlightUnion.point.x = newElem.right;
				myHighlightUnion.point.y = newElem.bottom;
				break;
			case BOXPART_TOP:
				myHighlightType = MYHIGHLIGHTTYPE_LINE;
				myHighlightUnion.line.xStart = newElem.left;
				myHighlightUnion.line.yStart = newElem.top;
				myHighlightUnion.line.xEnd = newElem.right;
				myHighlightUnion.line.yEnd = newElem.top;
				break;
			case BOXPART_BOTTOM:
				myHighlightType = MYHIGHLIGHTTYPE_LINE;
				myHighlightUnion.line.xStart = newElem.left;
				myHighlightUnion.line.yStart = newElem.bottom;
				myHighlightUnion.line.xEnd = newElem.right;
				myHighlightUnion.line.yEnd = newElem.bottom;
				break;
			case BOXPART_LEFT:
				myHighlightType = MYHIGHLIGHTTYPE_LINE;
				myHighlightUnion.line.xStart = newElem.left;
				myHighlightUnion.line.yStart = newElem.top;
				myHighlightUnion.line.xEnd = newElem.left;
				myHighlightUnion.line.yEnd = newElem.bottom;
				break;
			case BOXPART_RIGHT:
				myHighlightType = MYHIGHLIGHTTYPE_LINE;
				myHighlightUnion.line.xStart = newElem.right;
				myHighlightUnion.line.yStart = newElem.top;
				myHighlightUnion.line.xEnd = newElem.right;
				myHighlightUnion.line.yEnd = newElem.bottom;
				break;
		}
		
		if (myHighlightType == MYHIGHLIGHTTYPE_POINT) {
			drawDataPrepared.circles.emplace_back();
			DrawCircleCallParams& newCircle = drawDataPrepared.circles.back();
			newCircle.posX = myHighlightUnion.point.x;
			newCircle.posY = myHighlightUnion.point.y;
			newCircle.radius = aswOneScreenPixelDiameter * 6;
			newCircle.fillColor = colorIsLess(0xFFFFFF - (outlineColor & 0xFFFFFF), 0x191919)
				? 0x66000000 : 0x66FFFFFF;
			newCircle.outlineColor = 0;
			newCircle.hashKey = 1;
		} else if (myHighlightType == MYHIGHLIGHTTYPE_LINE) {
			drawDataPrepared.interactionBoxes.emplace_back();
			DrawBoxCallParams& newBox = drawDataPrepared.interactionBoxes.back();
			newBox.left = myHighlightUnion.line.xStart - aswOneScreenPixelDiameter * 3;
			newBox.top = myHighlightUnion.line.yStart - aswOneScreenPixelDiameter * 3;
			newBox.right = myHighlightUnion.line.xEnd + aswOneScreenPixelDiameter * 3;
			newBox.bottom = myHighlightUnion.line.yEnd + aswOneScreenPixelDiameter * 3;
			newBox.fillColor = colorIsLess(0xFFFFFF - (outlineColor & 0xFFFFFF), 0x191919)
				? 0x66000000 : 0x66FFFFFF;
			newBox.outlineColor = 0;
			newBox.thickness = 0;
			newBox.hatched = false;
		}
	}
	
}

void EndScene::hitboxEditorDrawPushbox(Entity ent, int posX, int posY, DWORD color) {
	drawDataPrepared.pushboxes.emplace_back();
	DrawBoxCallParams& newElem = drawDataPrepared.pushboxes.back();
	
	newElem.fillColor = replaceAlpha(64, color);
	newElem.hatched = false;
	
	ent.pushboxDimensions(&newElem.left, &newElem.top, &newElem.right, &newElem.bottom);
	
	newElem.outlineColor = replaceAlpha(255, color);
	newElem.thickness = 1;
	newElem.originX = posX;
	newElem.originY = posY;
}

bool EndScene::onEnableModsChanged() {
	if (!settings.enableScriptMods) return true;
	if (attemptedHookPreBeginPlay_Internal) return true;
	attemptedHookPreBeginPlay_Internal = true;
	uintptr_t stringLocation = sigscanStrOffset("GuiltyGearXrd.exe:.rdata",
		"AREDGameInfo_BattleexecPreBeginPlay_Internal",
		nullptr, "execPreBeginPlay_Internal", nullptr);
	if (!stringLocation) return true;
	uintptr_t stringMentionLocation = sigscanBufOffset("GuiltyGearXrd.exe:.data",
		(char*)&stringLocation,
		4,
		nullptr, "execPreBeginPlay_InternalMention", "rel_GuiltyGearXrd.exe(????)");
	if (!stringMentionLocation) return true;
	if (!detouring.isInTransaction()) finishedSigscanning();
	stringMentionLocation += 4;
	orig_execPreBeginPlay_Internal = *(execPreBeginPlay_Internal_t*)stringMentionLocation;
	uintptr_t callPlace = sigscanForward((uintptr_t)orig_execPreBeginPlay_Internal,
		"8b 06 8b 90 >?? ?? ?? ?? 8b ce ff d2", 0x30);
	if (!callPlace) return true;
	fillBBScrInfosWrapperVtblOffset = *(DWORD*)(callPlace);
	auto ptr = &HookHelp::execPreBeginPlay_InternalHook;
	if (!attach(&(PVOID&)orig_execPreBeginPlay_Internal,
		(PVOID&)ptr,
		"execPreBeginPlay_Internal")) return false;
	return true;
}

void EndScene::HookHelp::execPreBeginPlay_InternalHook(void* stack, void* result) {
	// actually, 'this' is .. well, it could be many things, due to compiler optimization settings, but it is a UObject definitely
	endScene.execPreBeginPlay_InternalHook((UObject*)this, stack, result);
}

void EndScene::execPreBeginPlay_InternalHook(UObject* thisArg, void* stack, void* result) {
	std::vector<BYTE> data;
	
	if (settings.enableScriptMods) {
		game.sigscanFNamesAndAppRealloc();
		bool isWide;
		void* eitherNarrowOrWideStr = (void*)game.readFName(thisArg->Name.low, &isWide);
		int strCmpResult;
		if (isWide) {
			strCmpResult = wcscmp((wchar_t*)eitherNarrowOrWideStr, L"REDGameInfo_Battle");
		} else {
			strCmpResult = strcmp((char*)eitherNarrowOrWideStr, "REDGameInfo_Battle");
		}
		// they set the compiler optimization setting so high it crushed multiple separate functions that have same code together.
		// Luckily, this function is only used by things inheriting from UObject
		if (strCmpResult == 0) {
			PANGAEA_MOD_VERSION pangaeaVer = pangaeaVer = getPangaeaModVersion();
			
			static Moves::TriBool pangaeasModDefinitelyPresent = Moves::TRIBOOL_DUNNO;
			if (pangaeasModDefinitelyPresent == Moves::TRIBOOL_DUNNO) {
				pangaeasModDefinitelyPresent = Moves::TRIBOOL_FALSE;
				BYTE* vtable = *(BYTE**)thisArg;
				uintptr_t fillBBScrInfosWrapper = *(uintptr_t*)(vtable + fillBBScrInfosWrapperVtblOffset);
				uintptr_t callPlace = sigscanForward(fillBBScrInfosWrapper, "8b cf 89 97 ?? ?? ?? ?? >e8", 0x50);
				if (callPlace) {
					uintptr_t fillBBScrInfos = followRelativeCall(callPlace);
					uintptr_t BBScr_fillInfoCall = sigscanForward(fillBBScrInfos,
						"8d 4f e8 >e8", 0x70);
					if (BBScr_fillInfoCall) {
						uintptr_t BBScr_fillInfo = followRelativeCall(BBScr_fillInfoCall);
						if (*(BYTE*)BBScr_fillInfo == 0xE9 && belongsToPangaea((BYTE*)followRelativeCall(BBScr_fillInfo))) {
							pangaeasModDefinitelyPresent = Moves::TRIBOOL_TRUE;
						}
					}
				}
			}
			if (pangaeasModDefinitelyPresent == Moves::TRIBOOL_FALSE) {
				pangaeaVer = PANGAEA_MOD_NOT_PRESENT;
			}
			
			// I didn't account for the fact that Pangaea's mod has a checkbox. Loading mods in it is optional!
			// Ok, let's waste CPU/harddrive time and do double work per round init
			bool loadBBScript = true;//pangaeaVer == PANGAEA_MOD_NOT_PRESENT;
			bool loadCollision = true;//pangaeaVer == PANGAEA_MOD_NOT_PRESENT || pangaeaVer == PANGAEA_MOD_PRE_COLLISION;
			if (loadBBScript || loadCollision) {
				WCHAR pathBase[MAX_PATH];
				bool pathLengthError = false;
				if (getModsFolder().size() + 1 > MAX_PATH) {  // what if they're == ? We'll find out soon
					pathLengthError = true;
				} else {
					memcpy(pathBase, getModsFolder().c_str(), (getModsFolder().size() + 1) * sizeof (WCHAR));
					
					REDPawn_Player** players = (REDPawn_Player**)((BYTE*)thisArg + 0x490);
					for (int playerIndex = 0; playerIndex < 3; ++playerIndex) {
						REDPawn_Player* player = players[playerIndex];
						const char* charaName;
						if (playerIndex < 2) {
							charaName = determineCharaName(player->CharaScript());
						} else {
							charaName = "cmn";
						}
						if (charaName && !(
							playerIndex == 1
							&& players[0]->Collision() == players[1]->Collision()  // if this matches, everything matches
						)) {
							const char* charaNameIter;
							int destCharIndex;
							for (charaNameIter = charaName; *charaNameIter != '\0'; ++charaNameIter) {
								destCharIndex = (int)getModsFolder().size() + (charaNameIter - charaName);
								if (destCharIndex >= MAX_PATH) {
									pathLengthError = true;
								} else {
									pathBase[destCharIndex] = (wchar_t)*charaNameIter;
								}
							}
							destCharIndex = (int)getModsFolder().size() + (charaNameIter - charaName);
							if (destCharIndex >= MAX_PATH) {
								pathLengthError = true;
							} else {
								pathBase[destCharIndex] = L'\0';
								if (loadBBScript) {
									replaceAsset(pathBase, destCharIndex,
											L".bbscript",
											(REDAssetBase<BYTE>*)(player->CharaScript()),
											data,
											&pathLengthError);
								}
								pathBase[destCharIndex] = L'\0';
								if (loadCollision) {
									replaceAsset(pathBase, destCharIndex,
											L".collision",
											(REDAssetBase<BYTE>*)(player->Collision()),
											data,
											&pathLengthError);
								}
							}
							
						}
					}
				}
				if (pathLengthError) {
					ui.showErrorDlg("Can't load script mods because the game is installed in a path that exceeds 260 characters or is very close to it.", true);
				}
			}
		}
	}
	orig_execPreBeginPlay_Internal(thisArg, stack, result);
}

char* EndScene::determineCharaName(REDAssetCharaScript* script) {
	BYTE* ptr = script->TopData;
	int entryCount = *(int*)ptr;
	BYTE* dataEnd = ptr + script->DataSize;
	for (
		BYTE* dataStart = ptr + 4 + entryCount * sizeof (BBScriptLookupEntry);
		dataStart < dataEnd;
		dataStart = moves.skipInstr(dataStart)
	) {
		if (*dataStart == instr_beginSubroutine && strcmp(asInstr(dataStart, beginSubroutine)->name, "PreInit") == 0) {
			for (loopInstr(dataStart)) {
				if (moves.instrType(instr) == instr_charaName) {
					return asInstr(instr, charaName)->name;
				}
			}
			break;
		}
	}
	return nullptr;
}

// includes trailing slash
const std::wstring& EndScene::getModsFolder() {
	if (modsFolder.empty()) {
		modsFolder = settings.getSettingsPath();
		if (modsFolder.size() <= 2) {
			modsFolder.clear();
			return modsFolder;
		}
		modsFolder.resize(modsFolder.size() - 1);
		int count = 2;
		auto it = modsFolder.begin() + (modsFolder.size() - 1);
		for (; true; ) {
			if (*it == L'\\') {
				--count;
				if (count == 0) break;
			}
			if (it == modsFolder.begin()) break;
			--it;
		}
		if (count != 0) {
			modsFolder.clear();
			return modsFolder;
		}
		modsFolder.resize(it - modsFolder.begin() + 1);
		modsFolder += L"Mods\\";
	}
	return modsFolder;
}

void EndScene::replaceAsset(WCHAR (&basePathWithFileNameButWithoutExtensionOrDot)[MAX_PATH], int basePathLen,
			const wchar_t* extensionWithDot, REDAssetBase<BYTE>* asset,
			std::vector<BYTE>& data, bool* pathLengthError) {
	char errorBuf[1024];
	struct MyErrorHandler {
		static void showError(const char* errorMsg) {
			ui.showErrorDlg(errorMsg, true);
		}
	};
	
	WCHAR* destPtr = basePathWithFileNameButWithoutExtensionOrDot + basePathLen;
	int spaceLeft = MAX_PATH - (destPtr - basePathWithFileNameButWithoutExtensionOrDot);
	if (spaceLeft < (int)wcslen(extensionWithDot) + 1) {
		*pathLengthError = true;
		return;
	}
	wcscpy(destPtr, extensionWithDot);
	const WCHAR* const path = basePathWithFileNameButWithoutExtensionOrDot;
	
	HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (!file || file == INVALID_HANDLE_VALUE) {
		return;
	}
	
	try {
		if (readWholeFile(data, file, false, errorBuf, MyErrorHandler::showError, path)) {
			if (data.size() > asset->DataSize) {
				void* newMem = appRealloc(asset->TopData, data.size(), 8);
				if (newMem) {
					asset->TopData = (BYTE*)newMem;
					memcpy(newMem, data.data(), data.size());
					asset->DataSize = data.size();
				}
			} else {
				memcpy(asset->TopData, data.data(), data.size());
				asset->DataSize = data.size();
			}
		}
	} catch (std::exception& e) {
		std::vector<char> err;
		// WANNA CALL VSNPRINTF MAYBE??
		static const char msg1[] { "Failed to read file '" };
		static const char msg2[] { "': " };
		int sizeNeeded /* will include null due to passing -1 */ = WideCharToMultiByte(CP_UTF8, 0, path, -1, NULL, 0, NULL, NULL);
		if (sizeNeeded > 0) {
			err.resize(sizeof (msg1) - 1 + sizeNeeded - 1 + sizeof (msg2) - 1 + strlen(e.what())
				+ 1  // null terminator
			);
			char* errPtr = err.data();
			
			memcpy(errPtr, msg1, sizeof (msg1) - 1);
			errPtr += sizeof (msg1) - 1;
			
			WideCharToMultiByte(CP_UTF8, 0, path, -1, errPtr, err.size() - (errPtr - err.data()), NULL, NULL);
			errPtr += sizeNeeded - 1;
			
			memcpy(errPtr, msg2, sizeof (msg2) - 1);
			errPtr += sizeof (msg2) - 1;
			
			strcpy_s(errPtr, err.size() - (errPtr - err.data()), e.what());
			
			// strcpy should've put the null terminator
			
			ui.showErrorDlg(err.data(), true);
		}
		if (file) CloseHandle(file);
		file = NULL;
	}
	
	if (file) CloseHandle(file);
	file = NULL;
}

EndSceneStoredState* EndScene::incrementStatePointer(EndSceneStoredState* ptr) {
	++ptr;
	if (ptr >= stateRingBuffer.data() + stateRingBuffer.size()) {
		ptr = stateRingBuffer.data();
	}
	return ptr;
}

EndSceneStoredState* EndScene::findState(DWORD tickCount) {
	EndSceneStoredState* ptr = currentState;
	int i = stateCount;
	while (i > 0) {
		if (ptr->prevAswEngineTickCount == tickCount) {
			return ptr;
		}
		--i;
		--ptr;
		if (ptr < stateRingBuffer.data()) {
			ptr = &stateRingBuffer.back();
		}
	}
	return nullptr;
}

void EndScene::loadState(EndSceneStoredState* ptr) {
	if (ptr == nullptr) return;
	int tickDiff = (int)ptr->prevAswEngineTickCount - currentState->prevAswEngineTickCount;
	currentState = ptr;
	if (tickDiff != 0) {
		DWORD currentTick = currentState->prevAswEngineTickCount;
		int idlePos = EntityFramebar::confinePos(currentState->framebarPosition + currentState->framebarIdleFor);
		int idleHitstopPos = EntityFramebar::confinePos(currentState->framebarPositionHitstop + currentState->framebarIdleHitstopFor);
		for (PlayerFramebars& framebars : playerFramebars) {
			#define piece(framebarName, pos) { \
				int currentInd = framebars.framebarName.stateHead - framebars.framebarName.states; \
				currentInd += tickDiff; \
				if (currentInd < 0) { \
					currentInd += _countof(framebars.framebarName.states); \
				} else if (currentInd >= _countof(framebars.framebarName.states)) { \
					currentInd -= _countof(framebars.framebarName.states); \
				} \
				framebars.framebarName.stateHead = &framebars.framebarName.states[currentInd]; \
				framebars.framebarName.frames[pos] = framebars.framebarName.stateHead->currentFrame; \
			}
			piece(main, currentState->framebarPosition)
			piece(hitstop, currentState->framebarPositionHitstop)
			piece(idle, idlePos)
			piece(idleHitstop, idleHitstopPos)
			#undef piece
		}
		for (ThreadUnsafeSharedPtr<ProjectileFramebar>& framebarsPtr : projectileFramebars) {
			#define piece(stateHeadHolder) { \
				int currentInd = stateHeadHolder.stateHead - stateHeadHolder.states; \
				currentInd += tickDiff; \
				if (currentInd < 0) { \
					currentInd += _countof(stateHeadHolder.states); \
				} else if (currentInd >= _countof(stateHeadHolder.states)) { \
					currentInd -= _countof(stateHeadHolder.states); \
				} \
				stateHeadHolder.stateHead = &stateHeadHolder.states[currentInd]; \
			}
			ProjectileFramebar& framebars = *framebarsPtr;
			piece(framebars)
			piece(framebars.main)
			piece(framebars.hitstop)
			piece(framebars.idle)
			piece(framebars.idleHitstop)
			#undef piece
			if (currentTick >= framebars.creationTick && currentTick < framebars.deletionTick) {
				#define piece(framebarName, pos) { \
					if (framebars.framebarName.stateHead->idleTime == 0) { \
						int relPos = framebars.framebarName.toRelative(pos); \
						if (relPos < framebars.framebarName.stateHead->framesCount) { \
							framebars.framebarName.frames[relPos] = framebars.framebarName.stateHead->currentFrame; \
						} \
					} \
				}
				piece(main, currentState->framebarPosition)
				piece(hitstop, currentState->framebarPositionHitstop)
				piece(idle, idlePos)
				piece(idleHitstop, idleHitstopPos)
				#undef piece
			}
		}
	}
}

void EndScene::cloneState(EndSceneStoredState* dest, EndSceneStoredState* src) {
	*dest = *src;
	// framebars leave trace states, since they clone a new state out each time they advance
}

bool EndScene::isRunning() {
	return (
		game.isMatchRunning()
		|| currentState->players[0].gotHitOnThisFrame  // fix for death by DoT
		|| currentState->players[1].gotHitOnThisFrame  // fix for death by DoT
	)
	|| altModes.roundendCameraFlybyType() != 8
	|| game.is0xa8PreparingCamera();
}

bool EndScene::onSpeedUpReplayChanged() {
	if (!gifMode.speedUpReplay) return true;
	if (!hookLogicTickStepCount()) return false;
	return true;
}

bool EndScene::hookLogicTickStepCount() {
	if (attemptedHookLogicTickStepCount) return true;
	attemptedHookLogicTickStepCount = true;
	static const wchar_t IsSpecialCamera[] = L"IsSpecialCamera";
	uintptr_t stringLocation = sigscanOffsetMain("GuiltyGearXrd.exe:.rdata",
		(const char*)IsSpecialCamera,
		sizeof IsSpecialCamera,
		nullptr,
		std::initializer_list<int>{},
		nullptr,
		"IsSpecialCamera",
		nullptr);
	if (!stringLocation) return true;
	
	std::vector<char> sig;
	std::vector<char> mask;
	std::vector<char> maskForCaching;
	// 68 ?? ?? ?? ?? 8d 4c 24 0c 89 15 ?? ?? ?? ?? e8 ?? ?? ?? ?? 8b 08 6a 01 89 0d
	//                              string loc                        prev         FName::operator=                    our FName
	byteSpecificationToSigMask("68 rel(?? ?? ?? ??) 8d 4c 24 0c 89 15 ?? ?? ?? ?? e8 ?? ?? ?? ?? 8b 08 6a 01 89 0d",  // ?? ?? ?? ??
		sig, mask, nullptr, 0, &maskForCaching);
	memcpy(sig.data() + 1, &stringLocation, 4);
	uintptr_t stringMentionLocation = sigscanOffset(
		GUILTY_GEAR_XRD_EXE,
		sig.data(), mask.data(), nullptr, "IsSpecialCameraAssignment", maskForCaching.data());
	if (!stringMentionLocation) return true;
	
	stringMentionLocation += sig.size();
	uintptr_t IsSpecialCameraFName = *(uintptr_t*)stringMentionLocation;
	
	// 39 99 ?? ?? ?? ?? 74 ?? 8b b9 ?? ?? ?? ?? 8b 0d 88 f9 e3 01 8b 15 84 f9 e3 01
	byteSpecificationToSigMask("39 99 ?? ?? ?? ?? 74 ?? 8b b9 ?? ?? ?? ?? 8b 0d rel(?? ?? ?? ??) 8b 15 rel(?? ?? ?? ??)",
		sig, mask, nullptr, 0, &maskForCaching);
	DWORD IsSpecialCameraFNameHigh = IsSpecialCameraFName + 4;
	memcpy(sig.data() + 16, &IsSpecialCameraFNameHigh, 4);
	memcpy(sig.data() + 22, &IsSpecialCameraFName, 4);
	uintptr_t somewhereCloseToWhereWeReallyWant = sigscanOffset(
		GUILTY_GEAR_XRD_EXE,
		sig.data(), mask.data(), nullptr, "IsSpecialCameraUsageInOfflineVerOfTheLogicTick", maskForCaching.data());
	if (!somewhereCloseToWhereWeReallyWant) return true;
	
	uintptr_t isGameModeNetworkCall = sigscanForward(somewhereCloseToWhereWeReallyWant,
		//  isGameModeNetwork          observerNeedsCatchUp    MOV dword ptr [ESP + 0x24],0x2
		"e8 ?? ?? ?? ?? 85 c0 74 11 e8 ?? ?? ?? ?? 85 c0 74 08 c7 44 24 24 02 00 00 00", 0x60);
	if (!isGameModeNetworkCall) return true;
	
	// 8b 0d ?? ?? ?? ?? 81 c1 ?? ?? ?? ?? e8
	size_t pos;
	byteSpecificationToSigMask("8b 0d rel(?? ?? ?? ??) 81 c1 ?? ?? ?? ?? >e8",
		sig, mask, &pos, 1, &maskForCaching);
	memcpy(sig.data() + 2, &aswEngine, 4);
	uintptr_t replayPauseControlTickCall = sigscanForward(isGameModeNetworkCall,
		sig.data(), mask.data(), 0x600);
	if (!replayPauseControlTickCall) return true;
	replayPauseControlTickCall += pos;
	
	if (!detouring.isInTransaction()) finishedSigscanning();
	
	orig_isGameModeNetwork = (DWORD)followRelativeCall(isGameModeNetworkCall);
	int offset = calculateRelativeCallOffset(isGameModeNetworkCall, (uintptr_t)isGameModeNetworkHookWhenDecidingStepCountHookAsm);
	overwriteCall(isGameModeNetworkCall, offset);
	
	// why this hook?
	// when the replay is not paused and you want to pause it while holding the speedUpReplay button, it double-toggles, resulting in single frame step
	// this is because the replay toggles code is inside the loop that runs the logic ticks
	orig_replayPauseControlTick = (DWORD)followRelativeCall(replayPauseControlTickCall);
	offset = calculateRelativeCallOffset(replayPauseControlTickCall, (uintptr_t)replayPauseControlTickHookAsm);
	overwriteCall(replayPauseControlTickCall, offset);
	
	return true;
}

// returns ticks to perform
int isGameModeNetworkHookWhenDecidingStepCountHook() {
	return endScene.isGameModeNetworkHookWhenDecidingStepCountHook();
}

// returns ticks to perform
int EndScene::isGameModeNetworkHookWhenDecidingStepCountHook() {
	if (game.getGameMode() == GAME_MODE_REPLAY && gifMode.speedUpReplay
	) {
		return settings.fastForwardReplayFactor;
	}
	return 1;
}
