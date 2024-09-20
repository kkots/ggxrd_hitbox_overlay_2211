#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include "DrawHitboxArrayCallParams.h"
#include "DrawOutlineCallParams.h"
#include "DrawPointCallParams.h"
#include "DrawBoxCallParams.h"
#include <vector>
#include "Stencil.h"
#include "rectCombiner.h"
#include "BoundingRect.h"
#include "ComplicatedHurtbox.h"
#include <mutex>
#include "characterTypes.h"
#include "Entity.h"
#include "PngResource.h"

using UpdateD3DDeviceFromViewports_t = void(__thiscall*)(char* thisArg);

struct ActiveData {
	int actives = 0;
	int nonActives = 0;
};

struct ActiveDataArray {
	int count = 0;
	ActiveData data[32] { 0 };
	inline void addNonActive(int n = 1) {
		if (count == 0) {
			return;
		}
		data[count - 1].nonActives += n;
	}
	void addActive(int n = 1);
	inline void clear() { count = 0; }
	void print(char* buf, size_t bufSize);
};

struct PlayerInfo {
	int hp = 0;
	int maxHp = 0;
	int defenseModifier = 0;  // dmg = dmg * ((256 + defenseModifier) / 256)
	int gutsRating = 0;
	int gutsPercentage = 0;
	int risc = 0;  // max 12800
	int tension = 0;  // max 10000
	int tensionPulse = 0;  // -25000 to 25000
	int negativePenaltyTimer = 0;  // time remaining until negative penalty wears off
	int negativePenalty = 0;  // 0 to 10000
	int tensionPulsePenalty = 0;  // 0 to 1800
	int cornerPenalty = 0;  // 0 to 960
	int tensionPulsePenaltyGainModifier_distanceModifier = 0;
	int tensionPulsePenaltyGainModifier_tensionPulseModifier = 0;
	int tensionGainModifier_distanceModifier = 0;
	int tensionGainModifier_negativePenaltyModifier = 0;
	int tensionGainModifier_tensionPulseModifier = 0;
	int extraTensionGainModifier = 0;
	int receivedComboCountTensionGainModifier = 0;
	int dealtComboCountTensionGainModifier = 0;
	int tensionGainOnLastHit = 0;
	int burstGainOnLastHit = 0;
	int tensionGainLastCombo = 0;
	int burstGainLastCombo = 0;
	int tensionGainMaxCombo = 0;
	int burstGainMaxCombo = 0;
	int stun = 0;
	int stunThreshold = 0;
	int blockstun = 0;
	int hitstun = 0;
	int hitstop = 0;
	int nextHitstop = 0;
	int burst = 0;  // max 15000
	int comboCountBurstGainModifier = 0;
	int frameAdvantage = 0;
	int landingFrameAdvantage = 0;
	int gaps[10] { 0 };
	int gapsCount = 0;
	int timeSinceLastGap = 0;
	int weight = 0;
	int wakeupTiming = 0;
	WakeupTimings wakeupTimings;
	int timePassed = 0;
	int timePassedLanding = 0;
	int hitboxesCount = 0;
	int activeProjectilesCount = 0;
	Entity activeProjectiles[32] { nullptr };
	bool hasNewActiveProjectiles = false;
	int superfreezeStartup = 0;
	int projectileStartup = 0;
	ActiveDataArray projectileActives { 0 };
	int timeSinceLastBusyStart = 0;
	int timeSinceLastActiveProjectile = 0;
	int startup = 0;
	ActiveDataArray actives { 0 };
	int recovery = 0;
	int total = 0;
	int landingOrPreJumpFrames = 0;
	int remainingDoubleJumps = 0;
	int remainingAirDashes = 0;
	char tensionPulsePenaltySeverity = 0;
	char cornerPenaltySeverity = 0;
	bool frameAdvantageValid = false;
	bool landingFrameAdvantageValid = false;
	bool idle:1;
	bool idleNext:1;
	bool idlePlus:1;
	bool idleLanding:1;
	bool startedUp:1;
	bool projectileStartedUp:1;
	bool onTheDefensive:1;
	bool landingOrPreJump:1;
	bool landed:1;
	bool isLanding:1;
	bool isLandingOrPreJump:1;
	bool needLand:1;
	bool airborne:1;
	bool inPain:1;
	bool wasCombod:1;
	bool gettingUp:1;
	CharacterType charType = CHARACTER_TYPE_SOL;
	char anim[32] { 0 };
	void removeActiveProjectile(int index);
	int findActiveProjectile(Entity ent);
	void addActiveProjectile(Entity ent);
	inline void clearGaps() { gapsCount = 0; }
	void addGap(int length = 1);
	void printGaps(char* buf, size_t bufSize);
};

struct DrawData {
	std::vector<ComplicatedHurtbox> hurtboxes;
	std::vector<DrawHitboxArrayCallParams> hitboxes;
	std::vector<DrawBoxCallParams> pushboxes;
	std::vector<DrawPointCallParams> points;
	std::vector<DrawBoxCallParams> throwBoxes;
	PlayerInfo players[2] { 0 };
	void clear();
	void clearPlayers();
	void clearWithoutPlayers();
	void copyTo(DrawData* destination);
	bool empty = false;
	bool needTakeScreenshot = false;
	unsigned int id = 0;
};

class Graphics
{
public:
	bool onDllMain(HMODULE hMod);
	void onUnload();
	void onEndSceneStart(IDirect3DDevice9* device);
	void drawAll();
	void takeScreenshotMain(IDirect3DDevice9* device, bool useSimpleVerion);
	void resetHook();
	IDirect3DSurface9* getOffscreenSurface(D3DSURFACE_DESC* renderTargetDescPtr = nullptr);
	void* getTexture();
	
	UpdateD3DDeviceFromViewports_t orig_UpdateD3DDeviceFromViewports = nullptr;
	std::mutex orig_UpdateD3DDeviceFromViewportsMutex;
	DrawData drawDataPrepared;
	std::mutex drawDataPreparedMutex;
	DrawData drawDataUse;
	bool needNewDrawData = true;
	bool needNewCameraData = true;
	std::mutex specialScreenshotFlagMutex;
	bool specialScreenshotFlag = false;
	IDirect3DDevice9* device;

private:
	class HookHelp {
		friend class Graphics;
		void UpdateD3DDeviceFromViewportsHook();
	};
	struct Vertex {
		float x, y, z, rhw;
		DWORD color;
		Vertex() = default;
		Vertex(float x, float y, float z, float rhw, DWORD color);
	};
	Stencil stencil;
	std::vector<DrawOutlineCallParams> outlines;
	std::vector<RectCombiner::Polygon> rectCombinerInputBoxes;
	std::vector<std::vector<RectCombiner::PathElement>> rectCombinerOutlines;
	CComPtr<IDirect3DSurface9> offscreenSurface;
	unsigned int offscreenSurfaceWidth = 0;
	unsigned int offscreenSurfaceHeight = 0;
	
	void prepareComplicatedHurtbox(const ComplicatedHurtbox& pairOfBoxesOrOneBox);

	unsigned int preparedArrayboxIdCounter = 0;
	struct PreparedArraybox {
		unsigned int id = ~0;
		unsigned int boxesPreparedSoFar = 0;
		BoundingRect boundingRect;
		bool isComplete = false;
	};
	std::vector<PreparedArraybox> preparedArrayboxes;
	std::vector<DrawOutlineCallParams> outlinesOverrideArena;
	void prepareArraybox(const DrawHitboxArrayCallParams& params, bool isComplicatedHurtbox,
						BoundingRect* boundingRect = nullptr, std::vector<DrawOutlineCallParams>* outlinesOverride = nullptr);

	unsigned int outlinesSectionOutlineCount = 0;
	unsigned int outlinesSectionTotalLineCount = 0;
	struct PreparedOutline {
		unsigned int linesSoFar = 0;
		bool isOnePixelThick = false;
		bool isComplete = false;
		bool hasPadding = false;
	};
	std::vector<PreparedOutline> preparedOutlines;
	void prepareOutline(const DrawOutlineCallParams& params);
	void drawOutlinesSection(bool preserveLastTwoVertices);

	unsigned int numberOfPointsPrepared = 0;
	void preparePoint(const DrawPointCallParams& params);

	void worldToScreen(const D3DXVECTOR3& vec, D3DXVECTOR3* out);
	bool takeScreenshotBegin(IDirect3DDevice9* device);
	void takeScreenshotDebug(IDirect3DDevice9* device, const wchar_t* filename);
	void takeScreenshotEnd(IDirect3DDevice9* device);
	bool getFramebufferData(IDirect3DDevice9* device,
		std::vector<unsigned char>& buffer,
		IDirect3DSurface9* renderTarget = nullptr,
		D3DSURFACE_DESC* renderTargetDescPtr = nullptr,
		unsigned int* widthPtr = nullptr,
		unsigned int* heightPtr = nullptr);
	void takeScreenshotSimple(IDirect3DDevice9* device);
	CComPtr<IDirect3DSurface9> gamesRenderTarget = nullptr;
	
	enum RenderStateDrawingWhat {
		RENDER_STATE_DRAWING_ARRAYBOXES,
		RENDER_STATE_DRAWING_BOXES,
		RENDER_STATE_DRAWING_OUTLINES,
		RENDER_STATE_DRAWING_POINTS
	} drawingWhat;
	void advanceRenderState(RenderStateDrawingWhat newState);
	bool needClearStencil;
	unsigned int lastComplicatedHurtboxId = ~0;
	unsigned int lastArrayboxId = ~0;
	BoundingRect lastBoundingRect;

	bool failedToCreateVertexBuffers = false;
	bool initializeVertexBuffers();

	std::vector<Vertex> vertexArena;
	CComPtr<IDirect3DVertexBuffer9> vertexBuffer;
	const unsigned int vertexBufferSize = 6 * 200;
	unsigned int vertexBufferRemainingSize = 0;
	unsigned int vertexBufferLength = 0;
	std::vector<Vertex>::iterator vertexIt;
	unsigned int vertexBufferPosition = 0;
	bool vertexBufferSent = false;

	enum LastThingInVertexBuffer {
		LAST_THING_IN_VERTEX_BUFFER_NOTHING,
		LAST_THING_IN_VERTEX_BUFFER_END_OF_COMPLICATED_HURTBOX,
		LAST_THING_IN_VERTEX_BUFFER_END_OF_ARRAYBOX,
		LAST_THING_IN_VERTEX_BUFFER_END_OF_BOX,
		LAST_THING_IN_VERTEX_BUFFER_END_OF_THINLINE,
		LAST_THING_IN_VERTEX_BUFFER_END_OF_THICKLINE
	} lastThingInVertexBuffer = LAST_THING_IN_VERTEX_BUFFER_NOTHING;

	unsigned int preparedBoxesCount = 0;
	bool prepareBox(const DrawBoxCallParams& params, BoundingRect* const boundingRect = nullptr, bool ignoreFill = false, bool ignoreOutline = false);
	void resetVertexBuffer();

	void sendAllPreparedVertices();

	bool drawAllArrayboxes();
	void drawAllBoxes();
	bool drawAllOutlines();
	void drawAllPoints();

	void drawAllPrepared();

	bool drawIfOutOfSpace(unsigned int verticesCountRequired, unsigned int texturedVerticesCountRequired);

	CComPtr<IDirect3DTexture9> packedTexture;
	bool failedToCreatePackedTexture = false;
	bool loggedDrawingOperationsOnce = false;
	
	enum ScreenshotStage {
		SCREENSHOT_STAGE_NONE,
		SCREENSHOT_STAGE_BASE_COLOR,
		SCREENSHOT_STAGE_FINAL
	} screenshotStage = SCREENSHOT_STAGE_NONE;
	
	bool failedToCreateTexture = false;
	bool initializeTexture();
	CComPtr<IDirect3DTexture9> texture = nullptr;
	PngResource questionMarkRes;
	HMODULE hMod = NULL;
};

extern Graphics graphics;
