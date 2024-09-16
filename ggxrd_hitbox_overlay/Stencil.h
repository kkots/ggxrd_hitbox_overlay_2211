#pragma once
#include "atlbase.h"
#include <d3d9.h>
#include "BoundingRect.h"

class Stencil
{
public:
private:
	friend class Graphics;
	void onEndSceneStart();
	void initialize(IDirect3DDevice9* device);  // called on the first draw of the first box each frame
	void onEndSceneEnd(IDirect3DDevice9* device);
	void clearRegion(IDirect3DDevice9* device, const BoundingRect& boundingRect);
	void clear(IDirect3DDevice9* device);
	CComPtr<IDirect3DSurface9> surface = NULL;  // Thanks to WorseThanYou for telling to use CComPtr class
	bool initialized = false;
	bool direct3DSuccess = false;
	bool direct3DError = false;
};

