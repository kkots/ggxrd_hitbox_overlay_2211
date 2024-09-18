#include "pch.h"
#include "Direct3DVTable.h"
#include "memoryFunctions.h"
#include "logging.h"

Direct3DVTable direct3DVTable;

bool Direct3DVTable::onDllMain() {
	vTableAddr = (char**)sigscanOffset("d3d9.dll",
		"C7 06 ?? ?? ?? ?? 89 86 ?? ?? ?? ?? 89 86",
		{2, 0},
		nullptr, "Direct3D vtable");
	if (vTableAddr) return true;

	// if we're here that means we're on Linux inside Wine or Steam Proton or something
	// luckily the game has a variable where it stores an IDirect3DDevice9*

	// Big thanks to WorseThanYou for finding this variable
	char** d3dManager = (char**)sigscanOffset(
		"GuiltyGearXrd.exe",
		"e8 ?? ?? ?? ?? 89 3d ?? ?? ?? ?? 33 c9 a3 ?? ?? ?? ?? 8b 44 24 20 c7 44 24 40 ff ff ff ff 89 4c 24 28",
		{ 14, 0 },
		nullptr, "d3dManager");
	if (!d3dManager) return false;
	if (!*d3dManager) {
		logwrap(fputs("d3dManager contains null\n", logfile));
		return false;
	}
	char** idirect3Ddevice9Ptr = *(char***)(*d3dManager + 0x24);
	if (!idirect3Ddevice9Ptr) {
		logwrap(fputs("*d3dManager + 0x24 contains null\n", logfile));
		return false;
	}
	void** deviceVtable = *(void***)(idirect3Ddevice9Ptr);
	if (!deviceVtable) {
		logwrap(fputs("*(*d3dManager + 0x24) contains null\n", logfile));
		return false;
	}
	vTableAddr = (char**)deviceVtable;
	logwrap(fprintf(logfile, "Found Direct3D vtable using alternative method: %p\n", vTableAddr));
	return true;
}

char** Direct3DVTable::getDirect3DVTable() const {
	return vTableAddr;
}
