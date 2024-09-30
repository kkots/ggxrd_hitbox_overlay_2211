#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <atlbase.h>
#include <vector>
#include "Entity.h"
#include <condition_variable>
#include <mutex>
#include "DrawTextWithIconsParams.h"
#include "PlayerInfo.h"

using USkeletalMeshComponent_UpdateTransform_t = void(__thiscall*)(char* thisArg);
using FUpdatePrimitiveTransformCommand_Apply_t = void(__thiscall*)(char* thisArg);
using drawTextWithIcons_t = void(*)(DrawTextWithIconsParams* param_1, int param_2, int param_3, int param_4, int param_5, int param_6);
using endSceneCaller_t = void(__thiscall*)(void* thisArg, int param1, int param2, int param3);
using BBScr_createObjectWithArg_t = void(__thiscall*)(void* pawn, const char* animName, unsigned int posType);
using BBScr_createParticleWithArg_t = void(__thiscall*)(void* pawn, const char* animName, unsigned int posType);
using setAnim_t = void(__thiscall*)(void* pawn, const char* animName);
using pawnInitialize_t = void(__thiscall*)(void* pawn, void* initializationParams);
using logicOnFrameAfterHit_t = void(__thiscall*)(void* pawn, int param1, int param2);
using BBScr_runOnObject_t = void(__thiscall*)(void* pawn, int entityReference);

LRESULT CALLBACK hook_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

class EndScene
{
public:
	bool onDllMain();
	bool onDllDetach();
	LRESULT WndProcHook(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void logic();
	void assignNextId(bool acquireLock = false);
	void onAswEngineDestroyed();
	void onHitDetectionStart(int hitDetectionType);
	void onHitDetectionEnd(int hitDetectionType);
	void onUWorld_TickBegin();
	void onUWorld_Tick();
	void endSceneHook(IDirect3DDevice9* device);
	void registerHit(HitResult hitResult, bool hasHitbox, Entity attacker, Entity defender);
	bool didHit(Entity attacker);
	void onTickActors_FDeferredTickList_FGlobalActorIteratorBegin(bool isFrozen);
	USkeletalMeshComponent_UpdateTransform_t orig_USkeletalMeshComponent_UpdateTransform = nullptr;
	std::mutex orig_USkeletalMeshComponent_UpdateTransformMutex;
	bool orig_USkeletalMeshComponent_UpdateTransformMutexLocked = false;
	DWORD orig_USkeletalMeshComponent_UpdateTransformMutexThreadId = NULL;
	FUpdatePrimitiveTransformCommand_Apply_t orig_FUpdatePrimitiveTransformCommand_Apply = nullptr;
	std::mutex orig_FUpdatePrimitiveTransformCommand_ApplyMutex;
	WNDPROC orig_WndProc = nullptr;
	std::mutex orig_WndProcMutex;
	bool orig_WndProcMutexLockedByWndProc = false;
	bool butDontPrepareBoxData = false;
	void(__thiscall* orig_drawTrainingHud)(char* thisArg) = nullptr;  // type is defined in Game.h: trainingHudTick_t
	std::mutex orig_drawTrainingHudMutex;
	endSceneCaller_t orig_endSceneCaller = nullptr;
	std::mutex orig_endSceneCallerMutex;
	BBScr_createObjectWithArg_t orig_BBScr_createObjectWithArg = nullptr;
	std::mutex orig_BBScr_createObjectWithArgMutex;
	BBScr_createParticleWithArg_t orig_BBScr_createParticleWithArg = nullptr;
	std::mutex orig_BBScr_createParticleWithArgMutex;
	setAnim_t orig_setAnim = nullptr;
	std::mutex orig_setAnimMutex;
	pawnInitialize_t orig_pawnInitialize = nullptr;
	std::mutex orig_pawnInitializeMutex;
	logicOnFrameAfterHit_t orig_logicOnFrameAfterHit = nullptr;
	std::mutex orig_logicOnFrameAfterHitMutex;
	BBScr_runOnObject_t orig_BBScr_runOnObject = nullptr;
	std::mutex orig_BBScr_runOnObjectMutex;
	
	PlayerInfo players[2] { 0 };
	std::vector<ProjectileInfo> projectiles;
	DWORD logicThreadId = NULL;
	Entity getSuperflashInstigator();
	int getSuperflashCounterAll();
	int getSuperflashCounterSelf();
private:
	void processKeyStrokes();
	void clearContinuousScreenshotMode();
	void actUponKeyStrokesThatAlreadyHappened();
	class HookHelp {
		friend class EndScene;
		void USkeletalMeshComponent_UpdateTransformHook();
		void FUpdatePrimitiveTransformCommand_ApplyHook();
		void drawTrainingHudHook();
		void endSceneCallerHook(int param1, int param2, int param3);
		void BBScr_createObjectWithArgHook(const char* animName, unsigned int posType);
		void BBScr_createParticleWithArgHook(const char* animName, unsigned int posType);
		void setAnimHook(const char* animName);
		void pawnInitializeHook(void* initializationParams);
		void logicOnFrameAfterHitHook(int param1, int param2);
		void BBScr_runOnObjectHook(int entityReference);
	};
	void USkeletalMeshComponent_UpdateTransformHook(char* thisArg);
	void FUpdatePrimitiveTransformCommand_ApplyHook(char* thisArg);
	void drawTrainingHudHook(char* thisArg);
	void BBScr_createParticleWithArgHook(Entity pawn, const char* animName, unsigned int posType);
	void onObjectCreated(Entity pawn, Entity createdPawn, const char* animName);
	void setAnimHook(Entity pawn, const char* animName);
	void pawnInitializeHook(Entity createdObj, void* initializationParams);
	void logicOnFrameAfterHitHook(Entity pawn, int param1, int param2);
	void BBScr_runOnObjectHook(Entity pawn, int entityReference);
	void prepareDrawData(bool* needClearHitDetection);
	struct HiddenEntity {
		Entity ent{ nullptr };
		int scaleX = 0;
		int scaleY = 0;
		int scaleZ = 0;
		int scaleDefault = 0;
		int scaleForParticles = 0;
		bool wasFoundOnThisFrame = false;
	};
	bool isEntityAlreadyDrawn(const Entity& ent) const;
	void noGravGifMode();
	void hideEntity(Entity ent);
	std::vector<HiddenEntity>::iterator findHiddenEntity(const Entity& ent);
	bool needToTakeScreenshot = false;

	std::vector<Entity> drawnEntities;

	std::vector<HiddenEntity> hiddenEntities;
	unsigned int allowNextFrameBeenHeldFor = 0;
	unsigned int allowNextFrameCounter = 0;
	bool freezeGame = false;
	bool continuousScreenshotMode = false;
	bool needContinuouslyTakeScreens = false;
	unsigned int previousTimeOfTakingScreen = ~0;
	
	drawTextWithIcons_t drawTextWithIcons = nullptr;
	
	bool needToRunNoGravGifMode = false;
	void drawTexts();
	
	uintptr_t superflashInstigatorOffset = 0;
	uintptr_t superflashCounterAllOffset = 0;
	uintptr_t superflashCounterSelfOffset = 0;
	
	bool measuringFrameAdvantage = false;
	int measuringLandingFrameAdvantage = -1;
	void restartMeasuringFrameAdvantage(int index);
	void restartMeasuringLandingFrameAdvantage(int index);
	
	int tensionRecordedHit[2] { 0 };
	int burstRecordedHit[2] { 0 };
	int tensionGainOnLastHit[2] { 0 };
	bool tensionGainOnLastHitUpdated[2] { 0 };
	int burstGainOnLastHit[2] { 0 };
	bool burstGainOnLastHitUpdated[2] { 0 };
	
	DWORD prevAswEngineTickCount = 0;
	bool drawDataPrepared = false;
	bool shutdown = false;
	HANDLE shutdownFinishedEvent = NULL;
	struct RegisteredHit {
		ProjectileInfo projectile;
		HitResult hitResult;
		Entity attacker;
		Entity defender;
		bool hasHitbox;
		bool isPawn;
	};
	std::vector<RegisteredHit> registeredHits;
	
	PlayerInfo& findPlayer(Entity ent);
	ProjectileInfo& findProjectile(Entity ent);
	void initializePawn(PlayerInfo& player, Entity ent);
	bool needFrameCleanup = false;
	void frameCleanup();
	bool creatingObject = false;
	Entity creatorOfCreatedObject = nullptr;
	const char* createdObjectAnim = nullptr;
};

extern EndScene endScene;
