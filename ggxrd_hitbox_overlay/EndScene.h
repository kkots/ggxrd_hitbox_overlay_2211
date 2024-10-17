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
#include "DrawData.h"

using drawTextWithIcons_t = void(*)(DrawTextWithIconsParams* param_1, int param_2, int param_3, int param_4, int param_5, int param_6);
using BBScr_createObjectWithArg_t = void(__thiscall*)(void* pawn, const char* animName, unsigned int posType);
using BBScr_createParticleWithArg_t = void(__thiscall*)(void* pawn, const char* animName, unsigned int posType);
using setAnim_t = void(__thiscall*)(void* pawn, const char* animName);
using pawnInitialize_t = void(__thiscall*)(void* pawn, void* initializationParams);
using logicOnFrameAfterHit_t = void(__thiscall*)(void* pawn, bool isAirHit, int param2);
using BBScr_runOnObject_t = void(__thiscall*)(void* pawn, int entityReference);
using FCanvas_Flush_t = void(__thiscall*)(void* canvas, bool bForce);
using backPushbackApplier_t = void(__thiscall*)(void* thisArg);
using pushbackStunOnBlock_t = void(__thiscall*)(void* pawn, bool isAirAttack);
using isDummy_t = bool(__thiscall*)(void* trainingStruct, int team);
using BBScr_sendSignal_t = void(__thiscall*)(void* pawn, int referenceType, int signal);
using BBScr_sendSignalToAction_t = void(__thiscall*)(void* pawn, const char* searchAnim, int signal);
using getReferredEntity_t = void*(__thiscall*)(void* pawn, int referenceType);
using skillCheckPiece_t = BOOL(__thiscall*)(void* pawn);
using handleUpon_t = void(__thiscall*)(void* pawn, int signal);
using BBScr_callSubroutine_t = void(__thiscall*)(void* pawn, const char* funcName);
using BBScr_setHitstop_t = void(__thiscall*)(void* pawn, int hitstop);
using beginHitstop_t = void(__thiscall*)(void* pawn);
using BBScr_ignoreDeactivate_t = void(__thiscall*)(void* pawn);

struct FVector2D {
	float X;
	float Y;
};

struct FRingBuffer_AllocationContext {
	void* RingBuffer = nullptr;
	BYTE* AllocationStart = nullptr;
	BYTE* AllocationEnd = nullptr;
	FRingBuffer_AllocationContext(void* InRingBuffer, unsigned int InAllocationSize);
	inline int GetAllocatedSize() { return AllocationEnd - AllocationStart; }
	void Commit();
	inline ~FRingBuffer_AllocationContext() { Commit(); }
	FRingBuffer_AllocationContext& operator=(const FRingBuffer_AllocationContext& other) = delete;
	FRingBuffer_AllocationContext& operator=(FRingBuffer_AllocationContext&& other) noexcept {
		memmove(this, &other, sizeof(*this));
		other.AllocationStart = 0;
		return *this;
	}
};

struct FRenderCommand {
	virtual void Destructor(BOOL freeMem) noexcept;
	virtual unsigned int Execute() = 0;  // Runs on the graphics thread
	virtual const wchar_t* DescribeCommand() noexcept = 0;
};

struct FSkipRenderCommand : FRenderCommand {
	FSkipRenderCommand(unsigned int InNumSkipBytes);  // Runs on the main thread
	virtual unsigned int Execute() override;  // Runs on the graphics thread
	virtual const wchar_t* DescribeCommand() noexcept override;
	unsigned int NumSkipBytes;
};

struct DrawBoxesRenderCommand : FRenderCommand {
	virtual void Destructor(BOOL freeMem) noexcept override;
	virtual unsigned int Execute() override;  // Runs on the graphics thread
	virtual const wchar_t* DescribeCommand() noexcept override;
	DrawBoxesRenderCommand();  // Runs on the main thread
	DrawData drawData;
	CameraValues cameraValues;
};

struct DrawOriginPointsRenderCommand : FRenderCommand {
	virtual unsigned int Execute() override;  // Runs on the graphics thread
	virtual const wchar_t* DescribeCommand() noexcept override;
};

struct DrawImGuiRenderCommand : FRenderCommand {
	virtual void Destructor(BOOL freeMem) noexcept override;
	virtual unsigned int Execute() override;  // Runs on the graphics thread
	virtual const wchar_t* DescribeCommand() noexcept override;
	DrawImGuiRenderCommand();  // Runs on the main thread
	std::vector<BYTE> drawData;
	BYTE* iconsUTexture2D = nullptr;
};

struct ShutdownRenderCommand : FRenderCommand {
	virtual unsigned int Execute() override;  // Runs on the graphics thread
	virtual const wchar_t* DescribeCommand() noexcept override;
};

struct REDDrawCommand {
    int commandType;  // 4 is quad
    unsigned int layer;  // ordered by layer, lowest to highest, lowest drawn first
    float z;  // ordered by z within same layer, lowest to highest
    REDDrawCommand* prevItem;  // prev item within the same layer
    REDDrawCommand* nextItem;  // next item within the same layer
    REDDrawCommand* prevLayer;  // only valid for layer-items, i.e. items that are the first in a given layer
    REDDrawCommand* nextLayer;  // only valid for layer-items, i.e. items that are the first in a given layer
};

struct REDQuadVertex {
    float x;
    float y;
    float u;
    float v;
    D3DCOLOR color;
};

struct REDDrawQuadCommand : REDDrawCommand {
    void* texture;  // UTexture2D*
    int field2_0x20;
    int count;  // vertex count
    int field4_0x28;
    REDQuadVertex* vertices;
    int field6_0x30;
    void* field7_0x34;
    int field8_0x38;
    int field9_0x3c;
};

using FRingBuffer_AllocationContext_Constructor_t = FRingBuffer_AllocationContext*(__thiscall*)(FRingBuffer_AllocationContext* thisArg,
	void* InRingBuffer, unsigned int InAllocationSize);
using FRingBuffer_AllocationContext_Commit_t = void(__thiscall*)(FRingBuffer_AllocationContext* thisArg);
using REDAnywhereDispDraw_t = void(__cdecl*)(void* canvas, FVector2D* screenSize);  // canvas is FCanvas*
using FRenderCommandDestructor_t = void(__thiscall*)(void* thisArg, BOOL freeMem);

LRESULT CALLBACK hook_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

class EndScene
{
public:
	bool onDllMain();
	bool onDllDetach();
	LRESULT WndProcHook(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void logic();
	void onAswEngineDestroyed();
	void onHitDetectionStart(int hitDetectionType);
	void onHitDetectionEnd(int hitDetectionType);
	void onUWorld_TickBegin();
	void onUWorld_Tick();
	void endSceneHook(IDirect3DDevice9* device);
	void registerHit(HitResult hitResult, bool hasHitbox, Entity attacker, Entity defender);
	bool didHit(Entity attacker);
	void onTickActors_FDeferredTickList_FGlobalActorIteratorBegin(bool isFrozen);
	WNDPROC orig_WndProc = nullptr;
	std::mutex orig_WndProcMutex;
	bool orig_WndProcMutexLockedByWndProc = false;
	void(__thiscall* orig_drawTrainingHud)(char* thisArg) = nullptr;  // type is defined in Game.h: trainingHudTick_t
	std::mutex orig_drawTrainingHudMutex;
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
	REDAnywhereDispDraw_t orig_REDAnywhereDispDraw = nullptr;
	std::mutex orig_REDAnywhereDispDrawMutex;
	FCanvas_Flush_t FCanvas_Flush = nullptr;
	DrawData drawDataPrepared;
	void* orig_drawQuadExec = nullptr;  // weird calling convention
	std::mutex orig_drawQuadExecMutex;
	backPushbackApplier_t orig_backPushbackApplier = nullptr;
	std::mutex orig_backPushbackApplierMutex;
	pushbackStunOnBlock_t orig_pushbackStunOnBlock = nullptr;
	std::mutex orig_pushbackStunOnBlockMutex;
	isDummy_t isDummyPtr = nullptr;
	BBScr_sendSignal_t orig_BBScr_sendSignal = nullptr;
	std::mutex orig_BBScr_sendSignalMutex;
	BBScr_sendSignalToAction_t orig_BBScr_sendSignalToAction = nullptr;
	std::mutex orig_BBScr_sendSignalToActionMutex;
	getReferredEntity_t getReferredEntity = nullptr;
	skillCheckPiece_t orig_skillCheckPiece = nullptr;
	std::mutex orig_skillCheckPieceMutex;
	handleUpon_t orig_handleUpon = nullptr;
	std::mutex orig_handleUponMutex;
	BBScr_callSubroutine_t BBScr_callSubroutine = nullptr;
	BBScr_setHitstop_t orig_BBScr_setHitstop = nullptr;
	std::mutex orig_BBScr_setHitstopMutex;
	beginHitstop_t orig_beginHitstop = nullptr;
	std::mutex orig_beginHitstopMutex;
	BBScr_ignoreDeactivate_t orig_BBScr_ignoreDeactivate = nullptr;
	std::mutex orig_BBScr_ignoreDeactivateMutex;
	
	PlayerInfo players[2] { 0 };
	std::vector<ProjectileInfo> projectiles;
	DWORD logicThreadId = NULL;
	Entity getSuperflashInstigator();
	int getSuperflashCounterAll();
	int getSuperflashCounterSelf();
	FRingBuffer_AllocationContext_Constructor_t FRingBuffer_AllocationContext_Constructor = nullptr;
	FRenderCommandDestructor_t FRenderCommandDestructor = nullptr;
	FRingBuffer_AllocationContext_Commit_t FRingBuffer_AllocationContext_Commit = nullptr;
	void* GRenderCommandBuffer = nullptr;  // an FRingBuffer*
	bool* GIsThreadedRendering = 0;
	void executeDrawBoxesRenderCommand(DrawBoxesRenderCommand* command);
	void executeDrawOriginPointsRenderCommand(DrawOriginPointsRenderCommand* command);
	IDirect3DDevice9* getDevice();
	FVector2D lastScreenSize { 0.F, 0.F };
	void drawQuadExecHook(FVector2D* screenSize, REDDrawQuadCommand* item, void* canvas);
	BYTE* getIconsUTexture2D();
	void executeDrawImGuiRenderCommand(DrawImGuiRenderCommand* command);
	void* iconTexture = nullptr;
	IDirect3DTexture9* getTextureFromUTexture2D(BYTE* uTex2D);
	void executeShutdownRenderCommand();
	PlayerInfo& findPlayer(Entity ent);
	ProjectileInfo& findProjectile(Entity ent);
	DWORD interRoundValueStorage2Offset = 0;
private:
	void processKeyStrokes();
	void clearContinuousScreenshotMode();
	void actUponKeyStrokesThatAlreadyHappened();
	class HookHelp {
		friend class EndScene;
		void drawTrainingHudHook();
		void BBScr_createObjectWithArgHook(const char* animName, unsigned int posType);
		void BBScr_createParticleWithArgHook(const char* animName, unsigned int posType);
		void setAnimHook(const char* animName);
		void pawnInitializeHook(void* initializationParams);
		void logicOnFrameAfterHitHook(bool isAirHit, int param2);
		void BBScr_runOnObjectHook(int entityReference);
		void backPushbackApplierHook();
		void pushbackStunOnBlockHook(bool isAirAttack);
		void BBScr_sendSignalHook(int referenceType, int signal);
		void BBScr_sendSignalToActionHook(const char* searchAnim, int signal);
		BOOL skillCheckPieceHook();
		void handleUponHook(int signal);
		void BBScr_setHitstopHook(int hitstop);
		void BBScr_ignoreDeactivateHook();
		void beginHitstopHook();
	};
	void drawTrainingHudHook(char* thisArg);
	void BBScr_createParticleWithArgHook(Entity pawn, const char* animName, unsigned int posType);
	void onObjectCreated(Entity pawn, Entity createdPawn, const char* animName);
	void setAnimHook(Entity pawn, const char* animName);
	void pawnInitializeHook(Entity createdObj, void* initializationParams);
	void logicOnFrameAfterHitHook(Entity pawn, bool isAirHit, int param2);
	void BBScr_runOnObjectHook(Entity pawn, int entityReference);
	static void REDAnywhereDispDrawHookStatic(void* canvas, FVector2D* screenSize);
	void REDAnywhereDispDrawHook(void* canvas, FVector2D* screenSize);
	void backPushbackApplierHook(char* thisArg);
	void pushbackStunOnBlockHook(Entity pawn, bool isAirAttack);
	void BBScr_sendSignalHook(Entity pawn, int referenceType, int signal);
	void BBScr_sendSignalToActionHook(Entity pawn, const char* searchAnim, int signal);
	BOOL skillCheckPieceHook(Entity pawn);
	void handleUponHook(Entity pawn, int signal);
	void BBScr_setHitstopHook(Entity pawn, int hitstop);
	void BBScr_ignoreDeactivateHook(Entity pawn);
	void beginHitstopHook(Entity pawn);
	
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
	int measuringLandingFrameAdvantage = -1;  // index of the player who is in the air and needs to land
	void restartMeasuringFrameAdvantage(int index);
	void restartMeasuringLandingFrameAdvantage(int index);
	
	int tensionRecordedHit[2] { 0 };
	int burstRecordedHit[2] { 0 };
	int tensionGainOnLastHit[2] { 0 };
	bool tensionGainOnLastHitUpdated[2] { 0 };
	int burstGainOnLastHit[2] { 0 };
	bool burstGainOnLastHitUpdated[2] { 0 };
	
	DWORD prevAswEngineTickCount = 0;
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
	
	void initializePawn(PlayerInfo& player, Entity ent);
	bool needFrameCleanup = false;
	void frameCleanup();
	bool creatingObject = false;
	Entity creatorOfCreatedObject = nullptr;
	const char* createdObjectAnim = nullptr;
	void* FRenderCommandVtable = nullptr;
	void* FSkipRenderCommandVtable = nullptr;
	bool drewExGaugeHud = false;
	void queueOriginPointDrawingDummyCommandAndInitializeIcon();
	struct OccuredEvent {
		enum OccuredEventType {
			SET_ANIM,
			SIGNAL
		} type;
		union OccuredEventUnion {
			struct OccuredEventSetAnim {
				Entity pawn;
			} setAnim;
			struct OccuredEventSignal {
				Entity from;
				Entity to;
			} signal;
			inline OccuredEventUnion() { }
		} u;
	};
	std::vector<OccuredEvent> events;
	std::vector<Entity> sendSignalStack;
};

extern EndScene endScene;
