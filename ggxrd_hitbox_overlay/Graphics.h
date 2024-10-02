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
#include "PngResource.h"
#include "PlayerInfo.h"
#include "DrawData.h"

using UpdateD3DDeviceFromViewports_t = void(__thiscall*)(char* thisArg);
using FSuspendRenderingThread_t = void(__thiscall*)(char* thisArg, unsigned int InSuspendThreadFlags);
using FSuspendRenderingThreadDestructor_t = void(__thiscall*)(char* thisArg);

class Graphics
{
public:
	bool onDllMain(HMODULE hMod);
	void onDllDetach();
	void onEndSceneStart(IDirect3DDevice9* device);
	void drawAll();
	void takeScreenshotMain(IDirect3DDevice9* device, bool useSimpleVerion);
	void resetHook();
	IDirect3DSurface9* getOffscreenSurface(D3DSURFACE_DESC* renderTargetDescPtr = nullptr);
	void* getTexture();
	
	DrawData drawDataUse;
	IDirect3DDevice9* device;
	DWORD graphicsThreadId = NULL;
	bool shutdown = false;
	bool onlyDrawPoints = false;

private:
	UpdateD3DDeviceFromViewports_t orig_UpdateD3DDeviceFromViewports = nullptr;
	std::mutex orig_UpdateD3DDeviceFromViewportsMutex;
	FSuspendRenderingThread_t orig_FSuspendRenderingThread = nullptr;
	std::mutex orig_FSuspendRenderingThreadMutex;
	FSuspendRenderingThreadDestructor_t orig_FSuspendRenderingThreadDestructor = nullptr;
	std::mutex orig_FSuspendRenderingThreadDestructorMutex;
	class HookHelp {
		friend class Graphics;
		void UpdateD3DDeviceFromViewportsHook();
		void FSuspendRenderingThreadHook(unsigned int InSuspendThreadFlags);
		void FSuspendRenderingThreadDestructorHook();
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
	HANDLE shutdownFinishedEvent = NULL;
	DWORD suspenderThreadId = NULL;
};

extern Graphics graphics;
