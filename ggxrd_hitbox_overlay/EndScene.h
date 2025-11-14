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
#include "HandleWrapper.h"
#include "DrawHitboxArrayCallParams.h"
#include "HitDetectionType.h"
#include "PackTextureSizes.h"
#include "PngResource.h"
#include "trainingSettings.h"
#include "DrawBoxCallParams.h"

using drawTextWithIcons_t = void(*)(DrawTextWithIconsParams* param_1, int param_2, int param_3, int param_4, int param_5, int param_6);
using BBScr_createObjectWithArg_t = void(__thiscall*)(void* pawn, const char* animName, BBScrPosType posType);
using BBScr_createParticleWithArg_t = void(__thiscall*)(void* pawn, const char* animName, BBScrPosType posType);
using BBScr_linkParticle_t = void(__thiscall*)(void* pawn, const char* name);
using setAnim_t = void(__thiscall*)(void* pawn, const char* animName);
using pawnInitialize_t = void(__thiscall*)(void* pawn, void* initializationParams);
using logicOnFrameAfterHit_t = void(__thiscall*)(void* pawn, bool isAirHit, int param2);
using BBScr_runOnObject_t = void(__thiscall*)(void* pawn, EntityReferenceType entityReference);
using FCanvas_Flush_t = void(__thiscall*)(void* canvas, bool bForce);
using backPushbackApplier_t = void(__thiscall*)(void* thisArg);
using pushbackStunOnBlock_t = void(__thiscall*)(void* pawn, bool isAirAttack);
using isDummy_t = bool(__thiscall*)(void* trainingStruct, int team);
using BBScr_sendSignal_t = void(__thiscall*)(void* pawn, EntityReferenceType referenceType, BBScrEvent signal);
using BBScr_sendSignalToAction_t = void(__thiscall*)(void* pawn, const char* searchAnim, BBScrEvent signal);
using getReferredEntity_t = void*(__thiscall*)(void* pawn, EntityReferenceType referenceType);
using skillCheckPiece_t = BOOL(__thiscall*)(void* pawn);
using handleUpon_t = void(__thiscall*)(void* pawn, BBScrEvent signal);
using BBScr_callSubroutine_t = void(__thiscall*)(void* pawn, const char* funcName);
using BBScr_setHitstop_t = void(__thiscall*)(void* pawn, int hitstop);
using beginHitstop_t = void(__thiscall*)(void* pawn);
using BBScr_ignoreDeactivate_t = void(__thiscall*)(void* pawn);
using BBScr_getAccessedValueImpl_t = int(__thiscall*)(void* pawn, BBScrTag tag, BBScrVariable id);
using BBScr_checkMoveConditionImpl_t = int(__thiscall*)(void* pawn, MoveCondition condition);
using setSuperFreezeAndRCSlowdownFlags_t = void(__thiscall*)(void* asw_subengine);
using BBScr_timeSlow_t = void(__thiscall*)(void* pawn, int duration);
using onCmnActXGuardLoop_t = void(__thiscall*)(void* pawn, BBScrEvent signal, int type, int thisIs0);
using drawTrainingHudInputHistory_t = void(__thiscall*)(void* trainingHud, unsigned int layer);
using hitDetection_t = BOOL(__thiscall*)(void* attacker, void* defender, HitboxType hitboxIndex, HitboxType defenderHitboxIndex, int* intersectionXPtr, int* intersectionYPtr);
using checkFirePerFrameUponsWrapper_t = void(__thiscall*)(void* ent);
using Pawn_ArcadeMode_IsBoss_t = BOOL(__thiscall*)(void* pawn);
using isSignVer1_10OrHigher_t = BOOL(__cdecl*)(void);
using multiplySpeedX_t = void(__thiscall*)(void* ent, int percentage);
using pawnGetColor_t = DWORD(__thiscall*)(void* ent, DWORD* inColor);
using trainingModeAndNoOneInXStunOrThrowInvulFromStunOrAirborneOrAttacking_t = BOOL(__thiscall*)(void* ent);
using inHitstunBlockstunOrThrowProtectionOrDead_t = BOOL(__thiscall*)(void* ent);
using addBurst_t = void(__thiscall*)(void* ent, int amount);
using spriteImpl_t = void(__thiscall*)(void* ent, const char* name, int thisIs1);
using appFree_t = void(__cdecl*)(void* mem);
extern appFree_t appFree;
using execPreBeginPlay_Internal_t = void(__thiscall*)(void* thisArg, void* stack, void* result);

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
		memcpy(this, &other, sizeof(*this));
		other.AllocationStart = 0;
		return *this;
	}
};

struct FRenderCommand {
	// suspected heavily to run on the graphics thread
	virtual void Destructor(BOOL freeMem) noexcept;
	virtual unsigned int Execute() = 0;  // Runs on the graphics thread
	virtual const wchar_t* DescribeCommand() noexcept = 0;
	FRenderCommand();
	static LONG volatile totalCountOfCommandsInCirculation;  // use InterlockedIncrement, InterlockedDecrement as this is ++ on main thread and -- on graphics
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
	bool needUpdateFramebarTexture = false;
	PngResource framebarTexture;
	PackTextureSizes framebarSizes;
	bool framebarColorblind;
	void applyFramebarTexture() const;
	bool needsFramesTextureFramebar = false;
	bool needsFramesTextureHelp = false;
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
	void print(bool canBlockButNotFD_ASSUMPTION) const;
};

class EndScene
{
public:
	bool onDllMain();
	bool onDllDetach();
	bool sigscanAfterHitDetector();
	LRESULT WndProcHook(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void logic();
	void onAswEngineDestroyed();
	void onHitDetectionStart(HitDetectionType hitDetectionType);
	void onHitDetectionEnd(HitDetectionType hitDetectionType);
	void onUWorld_TickBegin();
	void onUWorld_Tick();
	void registerHit(HitResult hitResult, bool hasHitbox, Entity attacker, Entity defender);
	bool didHit(Entity attacker);
	void onTickActors_FDeferredTickList_FGlobalActorIteratorBegin(bool isFrozen);
	void onGifModeBlackBackgroundChanged();
	void onAfterAttackCopy(Entity defenderPtr, Entity attackerPtr);
	void onDealHit(Entity defenderPtr, Entity attackerPtr);
	void onAfterDealHit(Entity defenderPtr, Entity attackerPtr);
	void removeAttackHitbox(Entity attackerPtr);
	WNDPROC orig_WndProc = nullptr;
	void(__thiscall* orig_drawTrainingHud)(char* thisArg) = nullptr;  // type is defined in Game.h: trainingHudTick_t
	BBScr_createObjectWithArg_t orig_BBScr_createObjectWithArg = nullptr;
	BBScr_createObjectWithArg_t orig_BBScr_createObject = nullptr;
	BBScr_createParticleWithArg_t orig_BBScr_createParticleWithArg = nullptr;
	BBScr_linkParticle_t orig_BBScr_linkParticle = nullptr;
	BBScr_linkParticle_t orig_BBScr_linkParticleWithArg2 = nullptr;
	setAnim_t orig_setAnim = nullptr;
	pawnInitialize_t orig_pawnInitialize = nullptr;
	checkFirePerFrameUponsWrapper_t orig_checkFirePerFrameUponsWrapper = nullptr;
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
	int getTotalFramesUnlimited() const { return framebarTotalFramesUnlimited; }
	int getTotalFramesHitstopUnlimited() const { return framebarTotalFramesHitstopUnlimited; }
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
	DWORD leftEdgeOfArenaOffset = 0;
	DWORD rightEdgeOfArenaOffset = 0;
	void increaseStunHook(Entity pawn, int stunAdd);
	void jumpInstallNormalJumpHook(Entity pawn);
	void jumpInstallSuperJumpHook(Entity pawn);
	void clearInputHistory(bool resetClearTime = false);
	void onHitDetectionAttackerParticipate(Entity ent);
	bool needUpdateGraphicsFramebarTexture = false;
	bool onPlayerIsBossChanged();
	isSignVer1_10OrHigher_t isSignVer1_10OrHigher = nullptr;
	BOOL clashHitDetectionCallHook(Entity attacker, Entity defender, HitboxType hitboxIndex, HitboxType defenderHitboxIndex, int* intersectionXPtr, int* intersectionYPtr);
	void activeFrameHitReflectMultiplySpeedXHook(Entity attacker, Entity defender, int percentage);
	bool highlightGreenWhenBecomingIdleChanged();
	void highlightSettingsChanged();
	bool needsFramesTextureFramebar = false;
	bool needsFramesTextureHelp = false;
	bool onDontResetBurstAndTensionGaugesWhenInStunOrFaintChanged();
	bool onDontResetRiscWhenInBurstOrFaintChanged();
	bool onOnlyApplyCounterhitSettingWhenDefenderNotInBurstOrFaintOrHitstunChanged();
	bool onStartingBurstGaugeChanged();
	getTrainingSetting_t orig_getTrainingSetting = nullptr;
	spriteImpl_t spriteImpl = nullptr;
	inline void forceFeedHitboxes(const DrawHitboxArrayCallParams& params) { drawDataPrepared.hitboxes.push_back(params); }
	inline void forceFeedOneHitbox(const DrawBoxCallParams& params) { drawDataPrepared.interactionBoxes.push_back(params); }
	// explained in UI::startHitboxEditMode
	struct HitboxEditorCameraValues {
		float cam_y;
		float fov;
		float t;
		float vw;
		float vh;
		
		// all ArcSys points must be first scaled by m to get them in UE3 coordinate space's scale
		float m;
		
		// multiply ArcSys displacements by this to get screen (1;-1) displacements
		float xCoeff;
		float yCoeff;
		
		float screenWidthASW;
		float screenHeightASW;
		
		float cam_xASW;
		float cam_zASW;
		
		bool prepare(Entity ent);
		bool ready = false;
	} hitboxEditorCameraValues;
	bool hitboxEditorCameraValuesReady = false;
	bool onEnableModsChanged();
private:
	void onDllDetachPiece();
	void processKeyStrokes();
	void clearContinuousScreenshotMode();
	void actUponKeyStrokesThatAlreadyHappened();
	class HookHelp {
		friend class EndScene;
		void drawTrainingHudHook();
		void BBScr_createObjectHook(const char* animName, BBScrPosType posType);
		void BBScr_createObjectWithArgHook(const char* animName, BBScrPosType posType);
		void BBScr_createParticleWithArgHook(const char* animName, BBScrPosType posType);
		void BBScr_linkParticleHook(const char* name);
		void BBScr_linkParticleWithArg2Hook(const char* name);
		void setAnimHook(const char* animName);
		void pawnInitializeHook(void* initializationParams);
		void logicOnFrameAfterHitHook(bool isAirHit, int param2);
		void BBScr_runOnObjectHook(EntityReferenceType entityReference);
		void backPushbackApplierHook();
		void pushbackStunOnBlockHook(bool isAirAttack);
		void BBScr_sendSignalHook(EntityReferenceType referenceType, BBScrEvent signal);
		void BBScr_sendSignalToActionHook(const char* searchAnim, BBScrEvent signal);
		BOOL skillCheckPieceHook();
		void handleUponHook(BBScrEvent signal);
		void BBScr_setHitstopHook(int hitstop);
		void BBScr_ignoreDeactivateHook();
		void beginHitstopHook();
		void BBScr_createObjectHook_piece();
		void setSuperFreezeAndRCSlowdownFlagsHook();
		void BBScr_timeSlowHook(int duration);
		void onCmnActXGuardLoopHook(BBScrEvent signal, int type, int thisIs0);
		void drawTrainingHudInputHistoryHook(unsigned int layer);
		void checkFirePerFrameUponsWrapperHook();
		void speedYReset(int speedY);
		BOOL Pawn_ArcadeMode_IsBossHook();
		DWORD pawnGetColorHook(DWORD* inColor);
		BOOL trainingModeAndNoOneInXStunOrThrowInvulFromStunOrAirborneOrAttackingHook();
		BOOL inHitstunBlockstunOrThrowProtectionOrDeadHook();
		void resetBurstHook(int amount);
		void execPreBeginPlay_InternalHook(void* stack, void* result);
	};
	void drawTrainingHudInputHistoryHook(void* trainingHud, unsigned int layer);
	void setSuperFreezeAndRCSlowdownFlagsHook(char* asw_subengine);
	void drawTrainingHudHook(char* thisArg);
	void BBScr_createParticleWithArgHook(Entity pawn, const char* animName, BBScrPosType posType);
	void BBScr_linkParticleHook(Entity pawn, const char* name);
	void BBScr_linkParticleWithArg2Hook(Entity pawn, const char* name);
	ProjectileInfo& onObjectCreated(Entity pawn, Entity createdPawn, const char* animName, bool fillName, bool calledFromInsideTick);
	void setAnimHook(Entity pawn, const char* animName);
	void pawnInitializeHook(Entity createdObj, void* initializationParams);
	void logicOnFrameAfterHitHook(Entity pawn, bool isAirHit, int param2);
	void BBScr_runOnObjectHook(Entity pawn, EntityReferenceType entityReference);
	static void REDAnywhereDispDrawHookStatic(void* canvas, FVector2D* screenSize);
	void REDAnywhereDispDrawHook(void* canvas, FVector2D* screenSize);
	void backPushbackApplierHook(char* thisArg);
	void pushbackStunOnBlockHook(Entity pawn, bool isAirAttack);
	void BBScr_sendSignalHook(Entity pawn, EntityReferenceType referenceType, BBScrEvent signal);
	void BBScr_sendSignalToActionHook(Entity pawn, const char* searchAnim, BBScrEvent signal);
	BOOL skillCheckPieceHook(Entity pawn);
	void handleUponHook(Entity pawn, BBScrEvent signal);
	void BBScr_setHitstopHook(Entity pawn, int hitstop);
	void BBScr_ignoreDeactivateHook(Entity pawn);
	void BBScr_timeSlowHook(Entity pawn, int duration);
	void beginHitstopHook(Entity pawn);
	void onCmnActXGuardLoopHook(Entity pawn, BBScrEvent signal, int type, int thisIs0);
	void checkFirePerFrameUponsWrapperHook(Entity pawn);
	void speedYReset(Entity pawn, int speedY);
	BOOL Pawn_ArcadeMode_IsBossHook(Entity pawn);
	DWORD pawnGetColorHook(Entity pawn, DWORD* inColor);
	BOOL trainingModeAndNoOneInXStunOrThrowInvulFromStunOrAirborneOrAttackingHook(Entity pawn);
	BOOL inHitstunBlockstunOrThrowProtectionOrDeadHook(Entity pawn);
	void resetBurstHook(Entity pawn, int amount);
	void execPreBeginPlay_InternalHook(UObject* thisArg, void* stack, void* result);
	
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
	HandleWrapper shutdownFinishedEvent = NULL;
	struct RegisteredHit {
		ProjectileInfo projectile;
		HitResult hitResult;
		Entity attacker;
		Entity defender;
		bool hasHitbox;
		bool isPawn;
	};
	std::vector<RegisteredHit> registeredHits;
	struct LeoParry {
		int x;
		int y;
		int timer;
		DWORD aswEngTick;
	};
	std::vector<LeoParry> leoParries;
	
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
				CreatedProjectileStruct creatorName;
			} signal;
			inline OccuredEventUnion() { }
		} u;
	};
	std::vector<OccuredEvent> events;
	std::vector<Entity> sendSignalStack;
	int framebarPosition = 0;
	int framebarTotalFramesUnlimited = 0;
	int framebarPositionHitstop = 0;
	int framebarTotalFramesHitstopUnlimited = 0;
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
	void collectFrameCancels(PlayerInfo& player, FrameCancelInfoFull& frame, bool inHitstopFreeze, bool isBlitzShieldCancels);
	void collectFrameCancelsPart(PlayerInfo& player, FixedArrayOfGatlingOrWhiffCancelInfos<GatlingOrWhiffCancelInfo>& vec, const AddedMoveData* move,
								int iterationIndex, bool inHitstopFreeze, bool blitzShield = false);
	bool checkMoveConditions(PlayerInfo& player, const AddedMoveData* move);
	bool requiresStylishInputs(const AddedMoveData* move);
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
	bool condenseIntoOneProjectileFramebar = false;
	int framesCount = -1;
	int storedFramesCount = -1;
	int scrollXInFrames = 0;
	bool framebarAutoScroll = true;
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
	CharInfo staticFontOpenParenthesis;
	CharInfo staticFontCloseParenthesis;
	CharInfo staticFontDigit[10];
	bool obtainedStaticFontCharInfos = false;
	GameModeFast* gameModeFast = nullptr;
	int* drawTrainingHudInputHistoryVal2 = nullptr;
	int* drawTrainingHudInputHistoryVal3 = nullptr;
	int getMinBufferTime(const InputType* inputs);
	hitDetection_t hitDetectionFunc = nullptr;
	void checkVenomBallActivations();
	void checkSelfProjectileHarmInflictions();
	int reachedMaxStun[2] { -1, -1 };  // on this frame
	void registerJump(PlayerInfo& player, Entity pawn, const char* animName);
	void registerRun(PlayerInfo& player, Entity pawn, const char* animName);
	bool shouldIgnoreEnterKey() const;
	void analyzeGunflame(PlayerInfo& player, bool* wholeGunflameDisappears,
		bool* firstWaveEntirelyDisappears, bool* firstWaveDisappearsDuringItsActiveFrames);
	bool hasLinkedProjectileOfType(PlayerInfo& player, const char* name);
	bool hasAnyProjectileOfTypeStrNCmp(PlayerInfo& player, const char* name);
	bool hasAnyProjectileOfType(PlayerInfo& player, const char* name);
	// this function is useless if you can have multiple of these projectiles and some can be dangerous while others aren't
	bool hasProjectileOfType(PlayerInfo& player, const char* name);
	// this function is useless if you can have multiple of these projectiles and some can be dangerous while others aren't
	bool hasProjectileOfTypeAndHasNotExhausedHit(PlayerInfo& player, const char* name);
	// this function is useless if you can have multiple of these projectiles and some can be dangerous while others aren't
	bool hasProjectileOfTypeStrNCmp(PlayerInfo& player, const char* name);
	char hasBoomerangHead(PlayerInfo& player);
	struct AttackHitbox {
		DrawHitboxArrayCallParams hitbox;
		bool notClash = false;
		bool clash = false;
		int count = 0;
		Entity ent { nullptr };
		bool found = false;
		int team = 2;
	};
	// this is needed to display hitboxes of projectiles that disappear when their player is hit, on the very frame their player gets hit.
	// Without this, those projectiles would get deleted by the end of the tick and we would not see their hitboxes.
	// While in reality, projectiles run hit collision before players do, and there may be some order between different projectiles,
	// so such projectiles could potentially deal hits on that very frame.
	std::vector<AttackHitbox> attackHitboxes;
	HitDetectionType currentHitDetectionType = HIT_DETECTION_EASY_CLASH;
	bool objHasAttackHitboxes(Entity ent) const;
	template<size_t size>
	inline void addPredefinedCancels(PlayerInfo& player, const char* (&array)[size],
	                                 std::vector<ForceAddedWhiffCancel>& cancels, FrameCancelInfoFull& frame,
	                                 bool inHitstopFreeze, bool isStylish) {
		initializePredefinedCancels(array, cancels);
		addPredefinedCancelsPart(player, cancels, frame, inHitstopFreeze, isStylish);
	}
	void addPredefinedCancelsPart(PlayerInfo& player, std::vector<ForceAddedWhiffCancel>& cancels, FrameCancelInfoFull& frame, bool inHitstopFreeze, bool isStylish);
	template<size_t size> inline static void initializePredefinedCancels(const char* (&array)[size], std::vector<ForceAddedWhiffCancel>& cancels) {
		initializePredefinedCancels(array, size, cancels);
	}
	static void initializePredefinedCancels(const char** array, size_t size, std::vector<ForceAddedWhiffCancel>& cancels);
	bool blitzShieldCancellable(PlayerInfo& player, bool insideTick);
	void collectBlitzShieldCancels(PlayerInfo& player, FrameCancelInfoFull& frame, bool inHitstopFreeze, bool isStylish);
	void collectBaikenBlockCancels(PlayerInfo& player, FrameCancelInfoFull& frame, bool inHitstopFreeze, bool isStylish);
	// these are common for both players, because these move are in cmn_ef
	int bdashMoveIndex = -1;
	int fdashMoveIndex = -1;
	int maximumBurstMoveIndex = -1;
	int cmnFAirDashMoveIndex = -1;
	int cmnBAirDashMoveIndex = -1;
	int fdStandMoveIndex = -1;
	int fdCrouchMoveIndex = -1;
	int fdAirMoveIndex = -1;
	int rcMoveIndex = -1;
	struct CheckAndCollectFrameCancelParams {
		PlayerInfo& player;
		int* moveIndex;
		const char* name;
		FrameCancelInfoFull& frame;
		bool inHitstopFreeze;
		bool isStylish;
		bool blitzShield;
	};
	void checkAndCollectFrameCancel(const CheckAndCollectFrameCancelParams* params);
	inline void checkAndCollectFrameCancelHelper(CheckAndCollectFrameCancelParams* params, const char* name, int* moveIndex) {
		params->name = name;
		params->moveIndex = moveIndex;
		checkAndCollectFrameCancel(params);
	}
	static bool isBlitzPostHitstopFrame_insideTick(const PlayerInfo& player);
	static bool isBlitzPostHitstopFrame_outsideTick(const PlayerInfo& player);
	void fillInBedmanSealInfo(PlayerInfo& player);
	void testDelay();
	uintptr_t getAccessedValueJumptable = 0;
	Pawn_ArcadeMode_IsBoss_t orig_Pawn_ArcadeMode_IsBoss = nullptr;
	bool hookPawnArcadeModeIsBoss();
	bool Pawn_ArcadeMode_IsBossHooked = false;
	void handleMarteliForpeliSetting(PlayerInfo& player);
	bool hasHitstunTiedVenomBall(PlayerInfo& player);
	// fix for Elphelt "Close Shot created a Far Shot" when firing uncharged sg.H
	static bool eventHandlerSendsIntoRecovery(Entity ptr, BBScrEvent signal);
	static bool checkSameTeam(Entity attacker, Entity defender);
	void fillRamlethalDisappearance(PlayerFrame& frame, PlayerInfo& player);
	bool elpheltGrenadeExists(PlayerInfo& player);
	bool elpheltJDExists(PlayerInfo& player);
	void fillInJackoInfo(PlayerInfo& player, PlayerFrame& frame);
	void fillDizzyInfo(PlayerInfo& player, PlayerFrame& frame);
	multiplySpeedX_t multiplySpeedX = nullptr;
	pawnGetColor_t orig_pawnGetColor = nullptr;
	bool hasCancelUnlocked(CharacterType charType, const FixedArrayOfGatlingOrWhiffCancelInfos<GatlingOrWhiffCancelInfo>& array, std::vector<const AddedMoveData*>& markedMoves,
		bool* redPtr, bool* greenPtr, bool* bluePtr);
	void processColor(PlayerInfo& player);
	struct HighlightMoveCacheEntry {
		bool needHighlight = false;
		bool red = false;
		bool green = false;
		bool blue = false;
	};
	std::unordered_map<const AddedMoveData*, HighlightMoveCacheEntry> highlightMoveCache;
	bool burstGaugeResettingHookAttempted = false;
	trainingModeAndNoOneInXStunOrThrowInvulFromStunOrAirborneOrAttacking_t orig_trainingModeAndNoOneInXStunOrThrowInvulFromStunOrAirborneOrAttacking = nullptr;
	inHitstunBlockstunOrThrowProtectionOrDead_t orig_inHitstunBlockstunOrThrowProtectionOrDead = nullptr;
	bool riscGaugeResettingHookAttempted = false;
	bool attemptedToHookTheObtainingOfCounterhitTrainingSetting = false;
	addBurst_t addBurst = nullptr;
	bool attemptedToHookBurstGaugeReset = false;
	// meant for use outside of onDllMain, but may be called from onDllMain as well
	bool overwriteCall(uintptr_t callInstr, int newOffset);
	// meant for use outside of onDllMain, but may be called from onDllMain as well
	bool attach(PVOID* ppPointer, PVOID pDetour, const char* name);
	bool lastRoundendContainedADeath = false;
	void drawHitboxEditorHitboxes();
	void drawHitboxEditorHitboxesForEntity(Entity ent);
	void drawHitboxEditorPushbox(Entity ent, int posY);
	struct PunishMessageTimer {
		int animFrame = -1;
		int recheckTimer = -1;
		int animFrameRepeatCount = 0;
		float yOff = 0.F;
		float xOff = 0.F;
	};
	std::array<std::vector<PunishMessageTimer>, 2> punishMessageTimers;
	void drawPunishMessage(float x, float y, DrawTextWithIconsAlignment alignment, DWORD textColor);
	bool attackerInRecoveryAfterBlock[2] { false, false };
	bool attackerInRecoveryAfterCreatingProjectile[2][75] { false };
	int aswOneScreenPixelWidth = 0;
	int aswOneScreenPixelHeight = 0;
	int aswOneScreenPixelDiameter = 0;
	void hitboxEditorDrawBox(DrawHitboxArrayCallParams& projector, int posX, int posY,
					Hitbox* hitbox, DWORD color, bool selected, BoxPart highlightedPart);
	void hitboxEditorDrawBox(const RECT* bounds, int posX, int posY, DWORD fillColor, DWORD outlineColor,
		bool selected, BoxPart highlightedPart, bool dashed);
	void hitboxEditorDrawPushbox(Entity ent, int posX, int posY, DWORD color);
	execPreBeginPlay_Internal_t orig_execPreBeginPlay_Internal = nullptr;
	char* determineCharaName(REDAssetCharaScript* script);
	// includes trailing slash
	std::wstring modsFolder;
	// includes trailing slash
	const std::wstring& getModsFolder();
	void replaceAsset(WCHAR (&basePathWithFileNameButWithoutExtensionOrDot)[MAX_PATH], int basePathLen, const wchar_t* extensionWithDot, REDAssetBase<BYTE>* asset,
		std::vector<BYTE>& data, bool* pathLengthError);
	DWORD fillBBScrInfosWrapperVtblOffset = 0;
	bool attemptedHookPreBeginPlay_Internal = false;
};

extern EndScene endScene;
