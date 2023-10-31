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

using Reset_t = HRESULT(__stdcall*)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS* pPresentationParameters);

HRESULT __stdcall hook_Reset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* pPresentationParameters);

class Graphics
{
public:
	bool onDllMain();
	void onDllDetach();
	void onEndSceneStart(IDirect3DDevice9* device);
	void drawAll();
	void resetHook();
	Reset_t orig_Reset;
	std::vector<ComplicatedHurtbox> hurtboxes;
	std::vector<DrawHitboxArrayCallParams> hitboxes;
	std::vector<DrawBoxCallParams> pushboxes;
	std::vector<DrawPointCallParams> points;
	std::vector<DrawBoxCallParams> throwBoxes;
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

	void drawBox(const DrawBoxCallParams& params, BoundingRect* const boundingRect = nullptr, bool useStencil = false);
	void drawHitboxArray(const DrawHitboxArrayCallParams& params, BoundingRect* boundingRect = nullptr, bool clearStencil = true,
		std::vector<DrawOutlineCallParams>* outlinesOverride = nullptr);
	void drawOutline(const DrawOutlineCallParams& params);
	void drawPoint(const DrawPointCallParams& params);
	void worldToScreen(const D3DXVECTOR3& vec, D3DXVECTOR3* out);
};

extern Graphics graphics;
