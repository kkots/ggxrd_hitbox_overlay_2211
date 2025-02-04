#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <atlbase.h>
#include <vector>
#include "Entity.h"
#include "DrawTextWithIconsParams.h"
#include "PlayerInfo.h"
#include "DrawData.h"
#include <unordered_set>
#include <memory>
#include "InputRingBuffer.h"
#include "InputRingBufferStored.h"
#include "CharInfo.h"

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
using BBScr_getAccessedValueImpl_t = int(__thiscall*)(void* pawn, int tag, int id);
using BBScr_checkMoveConditionImpl_t = int(__thiscall*)(void* pawn, MoveCondition condition);
using setSuperFreezeAndRCSlowdownFlags_t = void(__thiscall*)(void* asw_subengine);
using BBScr_timeSlow_t = void(__thiscall*)(void* pawn, int duration);
using onCmnActXGuardLoop_t = void(__thiscall*)(void* pawn, int signal, int type, int thisIs0);
using drawTrainingHudInputHistory_t = void(__thiscall*)(void* trainingHud, unsigned int layer);
using hitDetection_t = BOOL(__thiscall*)(void* attacker, void* defender, HitboxType hitboxIndex, HitboxType defenderHitboxIndex, int* intersectionXPtr, int* intersectionYPtr);

struct FVector2D {
	float X;
	float Y;
};

struct FontCharacter {
	int StartU;
	int StartV;
	int USize;
	int VSize;
	unsigned char TextureIndex;
	char padding[3];
	int VerticalOffset;
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
	bool drawingPostponed;
	bool obsStoppedCapturing;
	DrawData drawData;
	CameraValues cameraValues;
	bool noNeedToDrawPoints;
	bool pauseMenuOpen;
	bool dontShowBoxes;
	bool inputHistoryIsSplitOut;
	BYTE* iconsUTexture2D = nullptr;
	BYTE* staticFontTexture2D = nullptr;
	CharInfo openParenthesis;
	CharInfo closeParenthesis;
	CharInfo digit[10];
};

struct UiOrFramebarDrawData {
	UiOrFramebarDrawData(bool calledFromDrawOriginPointsRenderCommand);
	std::vector<BYTE> drawData;
	bool drawingPostponed = false;
	bool obsStoppedCapturing = false;
	BYTE* iconsUTexture2D = nullptr;
	BYTE* staticFontTexture2D = nullptr;
	CharInfo openParenthesis;
	CharInfo closeParenthesis;
	CharInfo digit[10];
	bool inputHistoryIsSplitOut;
};

struct DrawOriginPointsRenderCommand : FRenderCommand {
	virtual void Destructor(BOOL freeMem) noexcept override;
	virtual unsigned int Execute() override;  // Runs on the graphics thread
	virtual const wchar_t* DescribeCommand() noexcept override;
	UiOrFramebarDrawData uiOrFramebarDrawData{true};
};

struct DrawInputHistoryRenderCommand : FRenderCommand {
	virtual void Destructor(BOOL freeMem) noexcept override;
	virtual unsigned int Execute() override;  // Runs on the graphics thread
	virtual const wchar_t* DescribeCommand() noexcept override;
};

struct DrawImGuiRenderCommand : FRenderCommand {
	virtual void Destructor(BOOL freeMem) noexcept override;
	virtual unsigned int Execute() override;  // Runs on the graphics thread
	virtual const wchar_t* DescribeCommand() noexcept override;
	UiOrFramebarDrawData uiOrFramebarDrawData{false};
};

struct ShutdownRenderCommand : FRenderCommand {
	virtual unsigned int Execute() override;  // Runs on the graphics thread
	virtual const wchar_t* DescribeCommand() noexcept override;
};

struct HeartbeatRenderCommand : FRenderCommand {
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

enum SkippedFramesType : char {
	SKIPPED_FRAMES_SUPERFREEZE,
	SKIPPED_FRAMES_HITSTOP,
	SKIPPED_FRAMES_GRAB,
	SKIPPED_FRAMES_SUPER
};

struct SkippedFramesElement {
	SkippedFramesType type;
	unsigned short count;
};

struct SkippedFramesInfo {
	SkippedFramesElement elements[4];
	char count:7;
	char overflow:1;
	void addFrame(SkippedFramesType type);
	void clear();
	void transitionToOverflow();
	void print(bool canBlockButNotFD) const;
};

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
	void registerHit(HitResult hitResult, bool hasHitbox, Entity attacker, Entity defender);
	bool didHit(Entity attacker);
	void onTickActors_FDeferredTickList_FGlobalActorIteratorBegin(bool isFrozen);
	void onGifModeBlackBackgroundChanged();
	void onAfterAttackCopy(Entity defenderPtr, Entity attackerPtr);
	void onDealHit(Entity defenderPtr, Entity attackerPtr);
	void onAfterDealHit(Entity defenderPtr, Entity attackerPtr);
	WNDPROC orig_WndProc = nullptr;
	void(__thiscall* orig_drawTrainingHud)(char* thisArg) = nullptr;  // type is defined in Game.h: trainingHudTick_t
	BBScr_createObjectWithArg_t orig_BBScr_createObjectWithArg = nullptr;
	BBScr_createObjectWithArg_t orig_BBScr_createObject = nullptr;
	BBScr_createParticleWithArg_t orig_BBScr_createParticleWithArg = nullptr;
	setAnim_t orig_setAnim = nullptr;
	pawnInitialize_t orig_pawnInitialize = nullptr;
	logicOnFrameAfterHit_t orig_logicOnFrameAfterHit = nullptr;
	BBScr_runOnObject_t orig_BBScr_runOnObject = nullptr;
	REDAnywhereDispDraw_t orig_REDAnywhereDispDraw = nullptr;
	FCanvas_Flush_t FCanvas_Flush = nullptr;
	DrawData drawDataPrepared;
	void* orig_drawQuadExec = nullptr;  // weird calling convention
	backPushbackApplier_t orig_backPushbackApplier = nullptr;
	pushbackStunOnBlock_t orig_pushbackStunOnBlock = nullptr;
	isDummy_t isDummyPtr = nullptr;
	BBScr_sendSignal_t orig_BBScr_sendSignal = nullptr;
	BBScr_sendSignalToAction_t orig_BBScr_sendSignalToAction = nullptr;
	getReferredEntity_t getReferredEntity = nullptr;
	skillCheckPiece_t orig_skillCheckPiece = nullptr;
	handleUpon_t orig_handleUpon = nullptr;
	BBScr_callSubroutine_t BBScr_callSubroutine = nullptr;
	BBScr_setHitstop_t orig_BBScr_setHitstop = nullptr;
	beginHitstop_t orig_beginHitstop = nullptr;
	BBScr_ignoreDeactivate_t orig_BBScr_ignoreDeactivate = nullptr;
	bool pauseMenuOpen = false;
	BBScr_timeSlow_t orig_BBScr_timeSlow = nullptr;
	onCmnActXGuardLoop_t orig_onCmnActXGuardLoop = nullptr;
	
	std::vector<PlayerInfo> players{2};
	std::vector<ProjectileInfo> projectiles;
	std::vector<PlayerFramebars> playerFramebars;
	std::vector<ProjectileFramebar> projectileFramebars;
	inline int totalFramebarCount() { return playerFramebars.size() + projectileFramebars.size(); }
	EntityFramebar& getFramebar(int totalIndex);
	std::vector<CombinedProjectileFramebar> combinedFramebars;  // does not contain player framebars
	
	DWORD logicThreadId = NULL;
	FRingBuffer_AllocationContext_Constructor_t FRingBuffer_AllocationContext_Constructor = nullptr;
	FRenderCommandDestructor_t FRenderCommandDestructor = nullptr;
	FRingBuffer_AllocationContext_Commit_t FRingBuffer_AllocationContext_Commit = nullptr;
	void* GRenderCommandBuffer = nullptr;  // an FRingBuffer*
	bool* GIsThreadedRendering = 0;
	void executeDrawBoxesRenderCommand(DrawBoxesRenderCommand* command);
	void executeDrawOriginPointsRenderCommand(DrawOriginPointsRenderCommand* command);
	void executeDrawInputHistoryRenderCommand(DrawInputHistoryRenderCommand* command);
	IDirect3DDevice9* getDevice();
	FVector2D lastScreenSize { 0.F, 0.F };
	void drawQuadExecHook(FVector2D* screenSize, REDDrawQuadCommand* item, void* canvas);
	BYTE* getIconsUTexture2D();
	void fillInFontInfo(BYTE** staticFontTexture2D,
		CharInfo* openParenthesis,
		CharInfo* closeParenthesis,
		CharInfo* digit);
	void executeDrawImGuiRenderCommand(DrawImGuiRenderCommand* command);
	void* iconTexture = nullptr;
	IDirect3DTexture9* getTextureFromUTexture2D(BYTE* uTex2D);
	BYTE* getUTexture2DFromFont(BYTE* font);
	void executeShutdownRenderCommand();
	void executeHeartbeatRenderCommand();
	PlayerInfo& findPlayer(Entity ent);
	ProjectileInfo& findProjectile(Entity ent);
	DWORD interRoundValueStorage1Offset = 0;
	DWORD interRoundValueStorage2Offset = 0;
	bool isEntityHidden(const Entity& ent);
	int getFramebarPosition() const;
	int getFramebarPositionHitstop() const;
	bool willEnqueueAndDrawOriginPoints = false;
	bool endSceneAndPresentHooked = false;
	BBScr_getAccessedValueImpl_t BBScr_getAccessedValueImpl = nullptr;
	BBScr_checkMoveConditionImpl_t BBScr_checkMoveConditionImpl = nullptr;
	bool wasPlayerHadGatling(int playerIndex, const char* name);
	bool wasPlayerHadWhiffCancel(int playerIndex, const char* name);
	bool wasPlayerHadGatlings(int playerIndex);
	bool wasPlayerHadWhiffCancels(int playerIndex);
	int getSuperflashCounterOpponentCached();
	int getSuperflashCounterAlliedCached();
	int getSuperflashCounterOpponentMax();
	int getSuperflashCounterAlliedMax();
	Entity getLastNonZeroSuperflashInstigator();
	inline bool isIGiveUp() const { return iGiveUp; }
	bool needDrawInputs = false;
	const std::vector<SkippedFramesInfo>& getSkippedFrames(bool hitstop) const;
	bool queueingFramebarDrawCommand = false;
	bool drawingPostponed() const;
	bool uiWillBeDrawnOnTopOfPauseMenu = false;
	bool obsStoppedCapturing = false;
	setSuperFreezeAndRCSlowdownFlags_t orig_setSuperFreezeAndRCSlowdownFlags = nullptr;
	bool needEnqueueUiWithPoints = false;
	drawTrainingHudInputHistory_t orig_drawTrainingHudInputHistory = nullptr;
	GameModeFast getGameModeFast() const;
	bool requestedInputHistoryDraw = false;  // if true, inputs must only be drawn using a dedicated FRenderCommand and nowhere else
private:
	void onDllDetachPiece();
	void processKeyStrokes();
	void clearContinuousScreenshotMode();
	void actUponKeyStrokesThatAlreadyHappened();
	class HookHelp {
		friend class EndScene;
		void drawTrainingHudHook();
		void BBScr_createObjectHook(const char* animName, unsigned int posType);
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
		void BBScr_createObjectHook_piece();
		void setSuperFreezeAndRCSlowdownFlagsHook();
		void BBScr_timeSlowHook(int duration);
		void onCmnActXGuardLoopHook(int signal, int type, int thisIs0);
		void drawTrainingHudInputHistoryHook(unsigned int layer);
	};
	void drawTrainingHudInputHistoryHook(void* trainingHud, unsigned int layer);
	void setSuperFreezeAndRCSlowdownFlagsHook(char* asw_subengine);
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
	void BBScr_timeSlowHook(Entity pawn, int duration);
	void beginHitstopHook(Entity pawn);
	void onCmnActXGuardLoopHook(Entity pawn, int signal, int type, int thisIs0);
	
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
	uintptr_t superflashCounterOpponentOffset = 0;
	uintptr_t superflashCounterAlliedOffset = 0;
	
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
	DWORD prevAswEngineTickCountForInputs = 0;
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
	
	
	#define getDummyCmdUInt(name) name##UInt
	#define makeDummyCmdConst(name, floatVal) \
		const float name = floatVal; \
		const float name##OneGreater = name + 1.F; \
		const unsigned int getDummyCmdUInt(name) = *(unsigned int*)&name##OneGreater;
		
	makeDummyCmdConst(dummyOriginPointX, -615530.F)
	makeDummyCmdConst(dummyInputHistory, -615529.F)
	
	#undef makeDummyCmdConst
	
	
	void queueOriginPointDrawingDummyCommandAndInitializeIcon();
	void queueDummyCommand(int layer, float x, char* txt);
	struct OccuredEvent {
		enum OccuredEventType {
			SET_ANIM,
			SIGNAL
		} type;
		union OccuredEventUnion {
			struct OccuredEventSetAnim {
				Entity pawn;
				char fromAnim[32];
			} setAnim;
			struct OccuredEventSignal {
				Entity from;
				Entity to;
				char fromAnim[32];  // this is needed from Bedman 236H's bomb1 being created by Flying_bomb1.
									// Flying_bomb1 disappears on that very frame and is never actually visible,
									// and we get a different animation string ("423wind") when reading from its pointer
			} signal;
			inline OccuredEventUnion() { }
		} u;
	};
	std::vector<OccuredEvent> events;
	std::vector<Entity> sendSignalStack;
	int framebarPosition = 0;
	int framebarPositionHitstop = 0;
	int framebarIdleFor = 0;
	int framebarIdleHitstopFor = 0;
	const int framebarIdleForLimit = 30;
	DWORD getAswEngineTick();
	
	int nextFramebarId = 0;
	void incrementNextFramebarIdDirectly();
	void incrementNextFramebarIdSmartly();
	ProjectileFramebar& findProjectileFramebar(ProjectileInfo& projectile, bool needCreate);
	CombinedProjectileFramebar& findCombinedFramebar(const ProjectileFramebar& source, bool hitstop);
	void copyIdleHitstopFrameToTheRestOfSubframebars(EntityFramebar& entityFramebar,
		bool framebarAdvanced,
		bool framebarAdvancedIdle,
		bool framebarAdvancedHitstop,
		bool framebarAdvancedIdleHitstop);
	
	bool misterPlayerIsManuallyCrouching(const PlayerInfo& player);
	
	bool isInDarkenModePlusTurnOffPostEffect = false;
	bool postEffectWasOnWhenEnteringDarkenModePlusTurnOffPostEffect = false;
	bool willDrawOriginPoints();
	void collectFrameCancels(PlayerInfo& player, FrameCancelInfo& frame);
	void collectFrameCancelsPart(PlayerInfo& player, std::vector<GatlingOrWhiffCancelInfo>& vec, const AddedMoveData* move, int iterationIndex);
	bool checkMoveConditions(PlayerInfo& player, const AddedMoveData* move);
	bool whiffCancelIntoFDEnabled(PlayerInfo& player);
	Entity getSuperflashInstigator();
	int getSuperflashCounterOpponent();
	int getSuperflashCounterAllied();
	int superfreezeHasBeenGoingFor = 0;
	int superflashCounterAllied = 0;
	int superflashCounterAlliedMax = 0;
	int superflashCounterOpponent = 0;
	int superflashCounterOpponentMax = 0;
	void onProjectileHit(Entity ent);
	Entity lastNonZeroSuperflashInstigator;
	bool iGiveUp = false;
	std::vector<ForceAddedWhiffCancel> baikenBlockCancels;
	bool neverIgnoreHitstop = false;
	bool combineProjectileFramebarsWhenPossible = false;
	bool eachProjectileOnSeparateFramebar = false;
	bool isHoldingFD(const PlayerInfo& player) const;
	bool isHoldingFD(Entity pawn) const;
	void prepareInputs();
	InputRingBuffer prevInputRingBuffers[2] { };
	InputRingBufferStored inputRingBuffersStored[2] { };
	std::vector<SkippedFramesInfo> skippedFrames;
	std::vector<SkippedFramesInfo> skippedFramesIdle;
	std::vector<SkippedFramesInfo> skippedFramesHitstop;
	std::vector<SkippedFramesInfo> skippedFramesIdleHitstop;
	SkippedFramesInfo nextSkippedFrames;
	SkippedFramesInfo nextSkippedFramesIdle;
	SkippedFramesInfo nextSkippedFramesHitstop;
	SkippedFramesInfo nextSkippedFramesIdleHitstop;
	bool isFirstTickOfAMatch = false;
	bool startedNewRound = false;
	DWORD leftEdgeOfArenaOffset = 0;
	DWORD rightEdgeOfArenaOffset = 0;
	CharInfo staticFontOpenParenthesis;
	CharInfo staticFontCloseParenthesis;
	CharInfo staticFontDigit[10];
	bool obtainedStaticFontCharInfos = false;
	GameModeFast* gameModeFast = nullptr;
	int* drawTrainingHudInputHistoryVal2 = nullptr;
	int* drawTrainingHudInputHistoryVal3 = nullptr;
	int getMinBufferTime(const InputType* inputs);
	hitDetection_t hitDetectionFunc = nullptr;
	void checkDizzyBubblePops();
};

extern EndScene endScene;
