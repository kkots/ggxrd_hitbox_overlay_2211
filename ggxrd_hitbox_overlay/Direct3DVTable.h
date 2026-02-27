#pragma once
#include "d3d9.h"

using BeginScene_t = HRESULT(__stdcall*)(IDirect3DDevice9* device);
using Present_t = HRESULT(__stdcall*)(IDirect3DDevice9* device, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion);

class Direct3DVTable
{
public:
	bool onDllMain();
	IDirect3DSurface9* getRenderTarget();
	char** deviceVtable = nullptr;
	char** d3dManager = nullptr;
	bool findVtable();
	Present_t getPresent();
	BeginScene_t getBeginScene();
	IDirect3DDevice9* getDevice();
};

extern Direct3DVTable direct3DVTable;
