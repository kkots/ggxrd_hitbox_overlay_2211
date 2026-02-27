#include "pch.h"
#include "Direct3DVTable.h"
#include "memoryFunctions.h"
#include "logging.h"

Direct3DVTable direct3DVTable;

bool Direct3DVTable::findVtable() {
	
	if (deviceVtable) return true;
	if (getDevice() == nullptr) return false;
	char** idirect3Ddevice9Ptr = *(char***)(*d3dManager + 0x24);
	if (!idirect3Ddevice9Ptr) {
		// this is possible when injecting the mod by editing the BootGGXrd.bat to include the `start /MIN another.bat` line,
		// and that other .bat contains this line: `start /MIN ggxrd_hitbox_injector -force`, the injector and DLL are placed
		// in Xrd's Binaries\Win32 folder. Additionally, the game must launch directly into the fullscreen mode.
		// This causes the D3D9Device to be null during the mod's initialization.
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

bool Direct3DVTable::onDllMain() {
	return true;
}

Present_t Direct3DVTable::getPresent() {
	if (!findVtable()) return nullptr;
	return (Present_t)direct3DVTable.deviceVtable[17];
}

BeginScene_t Direct3DVTable::getBeginScene() {
	if (!findVtable()) return nullptr;
	return (BeginScene_t)direct3DVTable.deviceVtable[41];
}

// using this method as an alternative to the standard device->GetRenderTarget() eventually causes the application to freeze, I don't know why
IDirect3DSurface9* Direct3DVTable::getRenderTarget() {
	return *(IDirect3DSurface9**)(*(char**)(*d3dManager + 0x28) + 0x8);
}

IDirect3DDevice9* Direct3DVTable::getDevice() {
	// the reason we don't sigscan d3d9.dll directly is because on Linux underneath Steam Proton underneath Wine it is different and the sigscan is not found there
	
	// Big thanks to WorseThanYou for finding this variable
	d3dManager = (char**)sigscanOffset(
		GUILTY_GEAR_XRD_EXE,
		"e8 ?? ?? ?? ?? 89 3d ?? ?? ?? ?? 33 c9 a3 ?? ?? ?? ?? 8b 44 24 20 c7 44 24 40 ff ff ff ff 89 4c 24 28",
		{ 14, 0 },
		nullptr, "d3dManager");
	if (!d3dManager || !*d3dManager) {
		#ifdef LOG_PATH
		static bool reportedError = false;
		if (!reportedError) {
			reportedError = true;
			if (!d3dManager) {
				logwrap(fprintf(logfile, "d3dManager not found.\n"))
			} else {
				logwrap(fprintf(logfile, "*d3dManager is null.\n"))
			}
		}
		#endif
		return nullptr;
	}
		
	return *(IDirect3DDevice9**)(*d3dManager + 0x24);
}
