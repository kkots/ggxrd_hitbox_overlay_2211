#pragma once

class Direct3DVTable
{
public:
	bool onDllMain();
	char** d3dManager = nullptr;
};

extern Direct3DVTable direct3DVTable;
