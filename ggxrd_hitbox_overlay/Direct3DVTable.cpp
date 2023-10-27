#include "pch.h"
#include "Direct3DVTable.h"
#include "memoryFunctions.h"

Direct3DVTable direct3DVTable;

bool Direct3DVTable::onDllMain() {
	bool error = false;

	vTableAddr = (char**)sigscanOffset("d3d9.dll",
		"\xC7\x06\x00\x00\x00\x00\x89\x86\x00\x00\x00\x00\x89\x86",
		"xx????xx????xx",
		{2, 0},
		&error, "Direct3D vtable");

	return !error;
}

char** Direct3DVTable::getDirect3DVTable() const {
	return vTableAddr;
}
