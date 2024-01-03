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

using Reset_t = HRESULT(__stdcall*)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS* pPresentationParameters);

HRESULT __stdcall hook_Reset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* pPresentationParameters);

struct DrawData {
	std::vector<ComplicatedHurtbox> hurtboxes;
	std::vector<DrawHitboxArrayCallParams> hitboxes;
	std::vector<DrawBoxCallParams> pushboxes;
	std::vector<DrawPointCallParams> points;
	std::vector<DrawBoxCallParams> throwBoxes;
	void clear();
	void copyTo(DrawData* destination);
	bool empty = false;
	bool needTakeScreenshot = false;
	unsigned int id = 0;
};

class Graphics
{
public:
	bool onDllMain();
	void onUnload();
	void onEndSceneStart(IDirect3DDevice9* device);
	void drawAll();
	void takeScreenshotMain(IDirect3DDevice9* device, bool useSimpleVerion);
	void resetHook();
	IDirect3DSurface9* getOffscreenSurface(D3DSURFACE_DESC* renderTargetDescPtr = nullptr);
	
	Reset_t orig_Reset = nullptr;
	std::mutex orig_ResetMutex;
	DrawData drawDataPrepared;
	std::mutex drawDataPreparedMutex;
	DrawData drawDataUse;
	bool needNewDrawData = true;
	bool needNewCameraData = true;
	bool screenshotMode = false;
	bool allowedToDrawFills = true;
	bool allowedToDrawOutlines = true;
	bool allowedToUseStencil = true;
	bool allowedToDrawPoints = true;
	std::mutex specialScreenshotFlagMutex;
	bool specialScreenshotFlag = false;

private:
	struct Vertex {
		float x, y, z, rhw;
		DWORD color;
		Vertex(float x, float y, float z, float rhw, DWORD color);
	};
	Stencil stencil;
	std::vector<DrawOutlineCallParams> outlines;
	IDirect3DDevice9* device;
	std::vector<Vertex> vertexArena;
	std::vector<RectCombiner::Polygon> rectCombinerInputBoxes;
	std::vector<std::vector<RectCombiner::PathElement>> rectCombinerOutlines;
	CComPtr<IDirect3DSurface9> offscreenSurface;
	unsigned int offscreenSurfaceWidth = 0;
	unsigned int offscreenSurfaceHeight = 0;

	void drawBox(const DrawBoxCallParams& params, BoundingRect* const boundingRect = nullptr, bool useStencil = false);
	void drawHitboxArray(const DrawHitboxArrayCallParams& params, BoundingRect* boundingRect = nullptr, bool clearStencil = true,
		std::vector<DrawOutlineCallParams>* outlinesOverride = nullptr);
	void drawOutline(const DrawOutlineCallParams& params);
	void drawPoint(const DrawPointCallParams& params);
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
	CComPtr<IDirect3DStateBlock9> oldState;
};

extern Graphics graphics;
