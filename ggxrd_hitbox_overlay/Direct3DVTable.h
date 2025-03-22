#pragma once
#include "d3d9.h"

class Direct3DVTable
{
public:
	bool onDllMain();
	IDirect3DSurface9* getRenderTarget();
	char** deviceVtable = nullptr;
	char** d3dManager = nullptr;
};

extern Direct3DVTable direct3DVTable;
