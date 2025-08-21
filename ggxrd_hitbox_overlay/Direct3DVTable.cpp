#include "pch.h"
#include "Direct3DVTable.h"
#include "memoryFunctions.h"
#include "logging.h"

Direct3DVTable direct3DVTable;

bool Direct3DVTable::onDllMain() {
	
	// the reason we don't sigscan d3d9.dll directly is because on Linux underneath Steam Proton underneath Wine it is different and the sigscan is not found there
	
	// Big thanks to WorseThanYou for finding this variable
	d3dManager = (char**)sigscanOffset(
		GUILTY_GEAR_XRD_EXE,
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
	deviceVtable = *(char***)(idirect3Ddevice9Ptr);
	if (!deviceVtable) {
		logwrap(fputs("*(*d3dManager + 0x24) contains null\n", logfile));
		return false;
	}
	return true;
}

// using this method as an alternative to the standard device->GetRenderTarget() eventually causes the application to freeze, I don't know why
IDirect3DSurface9* Direct3DVTable::getRenderTarget() {
	return *(IDirect3DSurface9**)(*(char**)(*d3dManager + 0x28) + 0x8);
}
