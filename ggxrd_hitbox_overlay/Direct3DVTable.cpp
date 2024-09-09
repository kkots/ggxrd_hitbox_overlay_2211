#include "pch.h"
#include "Direct3DVTable.h"
#include "memoryFunctions.h"
#include "logging.h"

Direct3DVTable direct3DVTable;

bool Direct3DVTable::onDllMain() {
	// ghidra sig: C7 06 ?? ?? ?? ?? 89 86 ?? ?? ?? ?? 89 86
	vTableAddr = (char**)sigscanOffset("d3d9.dll",
		"\xC7\x06\x00\x00\x00\x00\x89\x86\x00\x00\x00\x00\x89\x86",
		"xx????xx????xx",
		{2, 0},
		nullptr, "Direct3D vtable");
	if (vTableAddr) return true;

	// if we're here that means we're on Linux inside Wine or Steam Proton or something
	// luckily the game has a variable where it stores an IDirect3DDevice9*

	// Big thanks to WorseThanYou for finding this variable
	// ghidra sig: e8 ?? ?? ?? ?? 89 3d ?? ?? ?? ?? 33 c9 a3 ?? ?? ?? ?? 8b 44 24 20 c7 44 24 40 ff ff ff ff 89 4c 24 28
	char** d3dManager = (char**)sigscanOffset(
		"GuiltyGearXrd.exe",
		"\xe8\x00\x00\x00\x00\x89\x3d\x00\x00\x00\x00\x33\xc9\xa3\x00\x00\x00\x00\x8b\x44\x24\x20\xc7\x44\x24\x40\xff\xff\xff\xff\x89\x4c\x24\x28",
		"x????xx????xxx????xxxxxxxxxxxxxxxx",
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
