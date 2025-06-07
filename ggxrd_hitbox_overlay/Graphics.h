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
#include "characterTypes.h"
#include "PlayerInfo.h"
#include "DrawData.h"
#include "CharInfo.h"
#include "HandleWrapper.h"

using UpdateD3DDeviceFromViewports_t = void(__thiscall*)(char* thisArg);
using FSuspendRenderingThread_t = void(__thiscall*)(char* thisArg, unsigned int InSuspendThreadFlags);
using FSuspendRenderingThreadDestructor_t = void(__thiscall*)(char* thisArg);
using BeginScene_t = HRESULT(__stdcall*)(IDirect3DDevice9* device);
using Present_t = HRESULT(__stdcall*)(IDirect3DDevice9* device, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion);

class Graphics
{
public:
	bool onDllMain(HMODULE hInstance);
	void onDllDetach();
	void onShutdown();
	void drawAllFromOutside(IDirect3DDevice9* device);
	void afterDraw();
	void takeScreenshotMain(IDirect3DDevice9* device, bool useSimpleVerion);
	void resetHook();
	IDirect3DSurface9* getOffscreenSurface(D3DSURFACE_DESC* renderTargetDescPtr = nullptr);
	IDirect3DTexture9* getFramesTexture(IDirect3DDevice9* device);
	// only returns a result once. Provides the error text to other classes
	void getShaderCompilationError(const std::string** result);
	void heartbeat(IDirect3DDevice9* device);
	
	DrawData drawDataUse;
	bool pauseMenuOpen = false;
	IDirect3DDevice9* device;
	DWORD graphicsThreadId = NULL;
	bool shutdown = false;
	bool onlyDrawPoints = false;  // drawing points may also draw inputs
	bool noNeedToDrawPoints = false;  // this will also affect drawing of inputs that are drawn together with points
	bool needDrawFramebarWithPoints = false;
	
	Present_t orig_present = nullptr;
	BeginScene_t orig_beginScene = nullptr;
	bool endSceneAndPresentHooked = false;
	bool obsStoppedCapturing = false;
	bool obsStoppedCapturingFromEndScenesPerspective = false;
	bool checkCanHookEndSceneAndPresent();
	bool imInDanger = false;;
	HandleWrapper responseToImInDanger = NULL;
	bool canDrawOnThisFrame() const;
	bool drawingPostponed() const;
	std::vector<BYTE> uiFramebarDrawData;
	std::vector<BYTE> uiDrawData;
	IDirect3DTexture9* uiTexture;
	IDirect3DTexture9* staticFontTexture;
	CharInfo staticFontOpenParenthesis;
	CharInfo staticFontCloseParenthesis;
	CharInfo staticFontDigit[10];
	// Draw boxes, without UI, and take a screenshot if needed
	// Runs on the graphics thread
	void executeBoxesRenderingCommand(IDirect3DDevice9* device);
	bool dontShowBoxes = false;
	IDirect3DTexture9* iconsTexture = nullptr;
	bool endSceneIsAwareOfDrawingPostponement = false;
	bool obsDisappeared = false;
	bool obsReappeared = false;
	bool onlyDrawInputHistory = false;
	bool inputHistoryIsSplitOut = false;  // if true, inputs must only be drawn using a dedicated FRenderCommand and nowhere else
	bool usePixelShader = true;
	static int getSin(int degrees);  // degrees - angle in degrees multiplied by 10 (for ex. 0-3600). Returns result from -1000 to 1000
	static int getCos(int degrees);  // degrees - angle in degrees multiplied by 10 (for ex. 0-3600). Returns result from -1000 to 1000
private:
	UpdateD3DDeviceFromViewports_t orig_UpdateD3DDeviceFromViewports = nullptr;
	FSuspendRenderingThread_t orig_FSuspendRenderingThread = nullptr;
	FSuspendRenderingThreadDestructor_t orig_FSuspendRenderingThreadDestructor = nullptr;
	class HookHelp {
		friend class Graphics;
		void UpdateD3DDeviceFromViewportsHook();
		void FSuspendRenderingThreadHook(unsigned int InSuspendThreadFlags);
		void FSuspendRenderingThreadDestructorHook();
	};
	struct Vertex {
		float x, y, z;
		DWORD color;
		Vertex() = default;
		Vertex(float x, float y, float z, DWORD color);
	};
	struct TextureVertex {
		float x, y, z;
		DWORD color;
		float u, v;
		TextureVertex() = default;
		TextureVertex(float x, float y, float z, float u, float v, DWORD color);
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
	unsigned int outlinesSectionHatchCount = 0;
	struct PreparedOutline {
		unsigned int linesSoFar = 0;
		unsigned int hatchesCount = 0;
		bool isOnePixelThick = false;
		bool isComplete = false;
		bool hasPadding = false;
		bool hatched = false;
		bool hatchesComplete = false;
	};
	std::vector<PreparedOutline> preparedOutlines;
	void prepareOutline(DrawOutlineCallParams& params);
	void drawOutlinesSection(bool preserveLastTwoVertices);

	unsigned int numberOfPointsPrepared = 0;
	void preparePoint(const DrawPointCallParams& params);
	unsigned int numberOfSmallPointsPrepared = 0;
	void prepareSmallPoint(const DrawPointCallParams& params);

	bool worldToScreen(const D3DXVECTOR3& vec, D3DXVECTOR3* out);
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
	CComPtr<IDirect3DSurface9> whateverOldRenderTarget = nullptr;  // just to put it back after we're done, we don't know what this is, it might be some weird smaller sized render target
	
	enum RenderStateDrawingWhat {
		RENDER_STATE_DRAWING_NOTHING,
		RENDER_STATE_DRAWING_ARRAYBOXES,
		RENDER_STATE_DRAWING_BOXES,
		RENDER_STATE_DRAWING_OUTLINES,
		RENDER_STATE_DRAWING_POINTS,
		RENDER_STATE_DRAWING_TEXTURES,
		RENDER_STATE_DRAWING_TEXT,
		RENDER_STATE_DRAWING_HOW_MANY_ENUMS_ARE_THERE  // must be last
	} drawingWhat;
	void advanceRenderState(RenderStateDrawingWhat newState);
	bool needClearStencil;
	unsigned int lastComplicatedHurtboxId = ~0;
	unsigned int lastArrayboxId = ~0;
	BoundingRect lastBoundingRect;

	bool failedToCreateVertexBuffers = false;
	bool initializeVertexBuffers();

	std::vector<BYTE> vertexArena;
	CComPtr<IDirect3DVertexBuffer9> vertexBuffer;
	const unsigned int vertexBufferSize = 6 * 200;
	const unsigned int textureVertexBufferSize = vertexBufferSize * sizeof (Vertex) / sizeof (TextureVertex);
	unsigned int vertexBufferRemainingSize = 0;
	unsigned int vertexBufferLength = 0;
	unsigned int textureVertexBufferRemainingSize = 0;
	unsigned int textureVertexBufferLength = 0;
	inline void consumeVertexBufferSpace(int verticesCount) {
		vertexBufferLength += verticesCount;
		vertexBufferRemainingSize -= verticesCount;
	}
	inline void consumeTextureVertexBufferSpace(int verticesCount) {
		textureVertexBufferLength += verticesCount;
		textureVertexBufferRemainingSize -= verticesCount;
	}
	bool preparingTextureVertexBuffer = false;
	bool renderingTextureVertices = false;
	void startPreparingTextureVertexBuffer();
	void stopPreparingTextureVertexBuffer();
	void switchToRenderingTextureVertices();
	void switchToRenderingNonTextureVertices();
	Vertex* vertexIt = nullptr;
	TextureVertex* textureVertexIt = nullptr;
	unsigned int vertexBufferPosition = 0;
	unsigned int textureVertexBufferPosition = 0;
	bool vertexBufferSent = false;

	enum LastThingInVertexBuffer {
		LAST_THING_IN_VERTEX_BUFFER_NOTHING,
		LAST_THING_IN_VERTEX_BUFFER_END_OF_COMPLICATED_HURTBOX,
		LAST_THING_IN_VERTEX_BUFFER_END_OF_ARRAYBOX,
		LAST_THING_IN_VERTEX_BUFFER_END_OF_BOX,
		LAST_THING_IN_VERTEX_BUFFER_END_OF_THINLINE,
		LAST_THING_IN_VERTEX_BUFFER_END_OF_THICKLINE,
		LAST_THING_IN_VERTEX_BUFFER_END_OF_TEXTUREBOX,
		LAST_THING_IN_VERTEX_BUFFER_END_OF_SMALL_POINT,
		LAST_THING_IN_VERTEX_BUFFER_POINT,
		LAST_THING_IN_VERTEX_BUFFER_HATCH
	} lastThingInVertexBuffer = LAST_THING_IN_VERTEX_BUFFER_NOTHING;
	bool lastTextureIsFont = false;

	unsigned int preparedBoxesCount = 0;
	bool prepareBox(const DrawBoxCallParams& params, BoundingRect* const boundingRect = nullptr, bool ignoreFill = false, bool ignoreOutline = false);
	void resetVertexBuffer();

	void sendAllPreparedVertices();

	bool drawAllArrayboxes();
	void drawAllBoxes();
	bool drawAllOutlines();
	void drawAllPoints();
	void drawAllSmallPoints();
	void drawAllTextureBoxes();

	void drawAllPrepared();

	bool drawIfOutOfSpace(unsigned int verticesCountRequired);
	bool textureDrawIfOutOfSpace(unsigned int verticesCountRequired);

	bool loggedDrawingOperationsOnce = false;
	
	enum ScreenshotStage {
		SCREENSHOT_STAGE_NONE,
		SCREENSHOT_STAGE_BASE_COLOR,
		SCREENSHOT_STAGE_FINAL,
		SCREENSHOT_STAGE_HOW_MANY_ENUMS_ARE_THERE  // must be last
	} screenshotStage = SCREENSHOT_STAGE_NONE;
	
	CComPtr<IDirect3DSurface9> altRenderTarget;
	int altRenderTargetLifeRemaining = 0;
	
	HandleWrapper shutdownFinishedEvent = NULL;
	DWORD suspenderThreadId = NULL;
	
	CComPtr<IDirect3DTexture9> framesTexture = nullptr;
	bool failedToCreateFramesTexture = false;
	
	IDirect3DTexture9* getOutlinesRTSamplingTexture(IDirect3DDevice9* device);
	CComPtr<IDirect3DTexture9> outlinesRTSamplingTexture = nullptr;
	bool failedToCreateOutlinesRTSamplingTexture = false;
	
	HMODULE hInstance = NULL;
	void compilePixelShader();
	bool failedToCompilePixelShader = false;
	std::vector<char> pixelShaderCode;
	
	std::string shaderCompilationError;
	
	bool failedToCreatePixelShader = false;
	CComPtr<IDirect3DPixelShader9> pixelShader;
	IDirect3DPixelShader9* getPixelShader(IDirect3DDevice9* device);
	
	void preparePixelShader(IDirect3DDevice9* device);
	
	void cpuPixelBlenderSimple(void* gameImage, const void* boxesImage, int width, int height);
	void cpuPixelBlenderComplex(void* gameImage, const void* boxesImage, int width, int height);
	
	const int hatchesDist = 15000;
	
	enum CurrentTransformSet {
		CURRENT_TRANSFORM_DEFAULT,
		CURRENT_TRANSFORM_3D_PROJECTION,
		CURRENT_TRANSFORM_2D_PROJECTION
	} currentTransformSet;
	int currentWorld3DMatrixWorldShiftX = 0;
	int currentWorld3DMatrixWorldShiftY = 0;
	void rememberTransforms(IDirect3DDevice9* device);
	void bringBackOldTransform(IDirect3DDevice9* device);
	D3DMATRIX prevWorld;
	D3DMATRIX prevView;
	D3DMATRIX prevProjection;
	void setTransformMatrices3DProjection(IDirect3DDevice9* device);
	void setTransformMatricesPlain2D(IDirect3DDevice9* device);
	float viewportW = 0.F;
	float viewportH = 0.F;
	#define RenderStateType(name) RENDER_STATE_TYPE_##name
	#define RenderStateValue(settingName, value) RENDER_STATE_VALUE_##settingName##__##value
	#define RenderStateHandler(name) RenderStateValueHandler_##name
	enum TypeOfRenderState {
		RenderStateType(D3DRS_STENCILENABLE),
		RenderStateType(D3DRS_ALPHABLENDENABLE),
		RenderStateType(PIXEL_SHADER),
		RenderStateType(TRANSFORM_MATRICES),
		RenderStateType(D3DRS_SRCBLEND),
		RenderStateType(D3DRS_DESTBLEND),
		RenderStateType(D3DRS_SRCBLENDALPHA),
		RenderStateType(D3DRS_DESTBLENDALPHA),
		RenderStateType(VERTEX),
		RenderStateType(TEXTURE),
		RENDER_STATE_TYPE_LAST  // must always be last
	};
	enum RenderStateValue {
		RenderStateValue(D3DRS_STENCILENABLE, FALSE),
		RenderStateValue(D3DRS_STENCILENABLE, TRUE),
		RenderStateValue(D3DRS_ALPHABLENDENABLE, FALSE),
		RenderStateValue(D3DRS_ALPHABLENDENABLE, TRUE),
		RenderStateValue(PIXEL_SHADER, NONE),
		RenderStateValue(PIXEL_SHADER, CUSTOM_PIXEL_SHADER),
		RenderStateValue(PIXEL_SHADER, NO_PIXEL_SHADER),
		RenderStateValue(TRANSFORM_MATRICES, NONE),
		RenderStateValue(TRANSFORM_MATRICES, 3D),
		RenderStateValue(TRANSFORM_MATRICES, 2D),
		RenderStateValue(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA),
		RenderStateValue(D3DRS_SRCBLEND, D3DBLEND_ONE),
		RenderStateValue(D3DRS_DESTBLEND, D3DBLEND_ZERO),
		RenderStateValue(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA),
		RenderStateValue(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE),
		RenderStateValue(D3DRS_SRCBLENDALPHA, D3DBLEND_ZERO),
		RenderStateValue(D3DRS_DESTBLENDALPHA, D3DBLEND_INVSRCALPHA),
		RenderStateValue(D3DRS_DESTBLENDALPHA, D3DBLEND_ZERO),
		RenderStateValue(VERTEX, NONTEXTURE),
		RenderStateValue(VERTEX, TEXTURE),
		RenderStateValue(TEXTURE, NONE),
		RenderStateValue(TEXTURE, FOR_PIXEL_SHADER),
		RenderStateValue(TEXTURE, ICONS),
		RenderStateValue(TEXTURE, FONT)
	};
	RenderStateValue renderStateValues[RENDER_STATE_TYPE_LAST];
	class RenderStateValueHandler {
	public:
		inline RenderStateValueHandler(Graphics* graphics) : graphics(*graphics) {}
		virtual void handleChange(RenderStateValue newValue) = 0;
		Graphics& graphics;
	};
	#define inheritConstructor(name) name(Graphics* graphics) : RenderStateValueHandler(graphics) {}
	class RenderStateHandler(D3DRS_STENCILENABLE) : public RenderStateValueHandler {
	public:
		inheritConstructor(RenderStateHandler(D3DRS_STENCILENABLE))
		void handleChange(RenderStateValue newValue) override;
	};
	class RenderStateHandler(D3DRS_ALPHABLENDENABLE) : public RenderStateValueHandler {
	public:
		inheritConstructor(RenderStateHandler(D3DRS_ALPHABLENDENABLE))
		void handleChange(RenderStateValue newValue) override;
	};
	class RenderStateHandler(PIXEL_SHADER) : public RenderStateValueHandler {
	public:
		inheritConstructor(RenderStateHandler(PIXEL_SHADER))
		void handleChange(RenderStateValue newValue) override;
	};
	class RenderStateHandler(TRANSFORM_MATRICES) : public RenderStateValueHandler {
	public:
		inheritConstructor(RenderStateHandler(TRANSFORM_MATRICES))
		void handleChange(RenderStateValue newValue) override;
	};
	class RenderStateHandler(D3DRS_SRCBLEND) : public RenderStateValueHandler {
	public:
		inheritConstructor(RenderStateHandler(D3DRS_SRCBLEND))
		void handleChange(RenderStateValue newValue) override;
	};
	class RenderStateHandler(D3DRS_DESTBLEND) : public RenderStateValueHandler {
	public:
		inheritConstructor(RenderStateHandler(D3DRS_DESTBLEND))
		void handleChange(RenderStateValue newValue) override;
	};
	class RenderStateHandler(D3DRS_SRCBLENDALPHA) : public RenderStateValueHandler {
	public:
		inheritConstructor(RenderStateHandler(D3DRS_SRCBLENDALPHA))
		void handleChange(RenderStateValue newValue) override;
	};
	class RenderStateHandler(D3DRS_DESTBLENDALPHA) : public RenderStateValueHandler {
	public:
		inheritConstructor(RenderStateHandler(D3DRS_DESTBLENDALPHA))
		void handleChange(RenderStateValue newValue) override;
	};
	class RenderStateHandler(VERTEX) : public RenderStateValueHandler {
	public:
		inheritConstructor(RenderStateHandler(VERTEX))
		void handleChange(RenderStateValue newValue) override;
	};
	class RenderStateHandler(TEXTURE) : public RenderStateValueHandler {
	public:
		inheritConstructor(RenderStateHandler(TEXTURE))
		void handleChange(RenderStateValue newValue) override;
	};
	#undef inheritConstructor
	RenderStateValueHandler* renderStateValueHandlers[RENDER_STATE_TYPE_LAST];
	struct RenderStateValueStack {
		RenderStateValue values[RENDER_STATE_TYPE_LAST];
		inline RenderStateValueStack* operator=(const RenderStateValueStack* other) {
			memcpy(this, other, sizeof *this);
		}
		inline RenderStateValue& operator[](int index) { return values[index]; }
	};
	RenderStateValueStack requiredRenderState[SCREENSHOT_STAGE_HOW_MANY_ENUMS_ARE_THERE][RENDER_STATE_DRAWING_HOW_MANY_ENUMS_ARE_THERE];
	HRESULT static __stdcall beginSceneHookStatic(IDirect3DDevice9* device);
	void beginSceneHook(IDirect3DDevice9* device);
	HRESULT static __stdcall presentHook(IDirect3DDevice9* device, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion);
	bool mayRunBeginSceneHook = false;
	bool checkAndHookBeginSceneAndPresent(bool transactionActive);
	bool imInDangerReceived = false;
	void receiveDanger();
	void dllDetachPiece();
	bool runningOwnBeginScene = false;
	struct TextureBoxParams {
		float xStart;
		float yStart;
		float xEnd;
		float yEnd;
		float uStart;
		float vStart;
		float uEnd;
		float vEnd;
		DWORD color;
	};
	unsigned int preparedTextureBoxesCount = 0;
	unsigned int preparedTextureBoxesCountWithFont = 0;
	void prepareTextureBox(const TextureBoxParams& box, bool isFont);
	void prepareDrawInputs();
	int calculateStartingTextureVertexBufferLength();
	int calculateStartingVertexBufferLength();
	bool framebarDrawDataPrepared = false;
	inline void prepareFramebarDrawData() { framebarDrawDataPrepared = true; }
	void drawAllFramebarDrawData();
	bool needDrawWholeUiWithPoints = false;
	int linesPrepared = 0;
	void prepareLine(const DrawLineCallParams& params);
	void drawAllLines();
	float screenSizeConstant[4] { 0.F, 0.F, 0.F, 0.F };
	bool preparedPixelShaderOnThisFrame = false;
	struct PreparedCircle {
		int x;
		int y;
		int verticesCount;
	};
	std::vector<PreparedCircle> preparedCircles;
	void prepareCircle(const DrawCircleCallParams& params);
	void drawAllCircles();
	std::vector<PreparedCircle> preparedCircleOutlines;
	void prepareCircleOutline(const DrawCircleCallParams& params);
	void drawAllCircleOutlines();
	struct CircleCacheElement {
		int radius = 0;
		D3DCOLOR fillColor = 0;
		D3DCOLOR outlineColor = 0;
		std::vector<Vertex> vertices;
		std::vector<Vertex> outlineVertices;
		int next = 0;
	};
	std::vector<int> circleCacheHashmap;
	std::vector<CircleCacheElement> circleCache;
	const int* sinTable = nullptr;
	int setupCircle(int radius, D3DCOLOR fillColor, D3DCOLOR outlineColor);
	bool worldMatrixHasShiftedWorldCenter = false;
	void ensureWorldMatrixWorldCenterIsZero();
	void setWorld3DMatrix(int worldCenterShiftX = 0, int worldCenterShiftY = 0);
	
	const float charInfoOffsetScale = 1.52F;
	void calcTextSize(const char* txt, float coef, float textScale, bool outline, float* sizeX, float* sizeY);
	/// <summary>
	/// Prepares texture vertices that contain data needed to draw the requested text.
	/// The text will be drawn with a black outline.
	/// </summary>
	/// <param name="x">Position of the top left corner of the text. Must be already multiplied by coefW.</param>
	/// <param name="y">Position of the top left corner of the text. Must be already multiplied by coefH and include extraH.</param>
	/// <param name="txt">The text to draw.</param>
	/// <param name="coef">The scaling factor for the text and outline size. Scaling of 1.F is used when screen width is 1280.F.</param>
	/// <param name="textScale">The scaling factor for text only. Gets combined with coef.</param>
	void printTextWithOutline(float x, float y, const char* txt, float coef, float textScale);
	
	/// <summary>
	/// Prepares texture vertices that contain data needed to draw the requested text.
	/// The text will be drawn with a black outline.
	/// </summary>
	/// <param name="x">Position of the top left corner of the text. Must be already multiplied by coefW.</param>
	/// <param name="y">Position of the top left corner of the text. Must be already multiplied by coefH and include extraH.</param>
	/// <param name="txt">The text to draw.</param>
	/// <param name="coef">The scaling factor for the text and outline size. Scaling of 1.F is used when screen width is 1280.F.</param>
	/// <param name="textScale">The scaling factor for text only. Gets combined with coef.</param>
	/// <param name="color">The tint of the text.</param>
	void printText(float x, float y, const char* txt, float coef, float textScale, DWORD color);
	void drawAll();
	void drawAllInit(IDirect3DDevice9* device);
	void onEndSceneStart(IDirect3DDevice9* device);
	void fillInScreenSize(IDirect3DDevice9* device);
	void initViewport(IDirect3DDevice9* device);
	
	// These things are added by ArcSys into the UE3 source code
	// They call IDirect3DDevice9::Present with a second argument - the destination rectangle
	// That argument tells on which portion of the window to draw the graphics, and it is in window client coordinates
	// Normally they provide the result of GetClientRect call into the second argument of Present, but
	// if *usePresentRectPtr is TRUE, they replace the W and H of that rect with *presentRectW/HPtr
	BOOL* usePresentRectPtr = nullptr;
	int* presentRectWPtr = nullptr;
	int* presentRectHPtr = nullptr;
	
};

extern Graphics graphics;
