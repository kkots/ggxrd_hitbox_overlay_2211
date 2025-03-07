#include "pch.h"
#include "ggxrd_hitbox_injector.h"
#include "ConsoleEmulator.h"
#include "ggxrd_hitbox_injector_common.h"
#include "InjectorVersion.h"  // I included this file so that when it changes, it triggers the Pre-Build event and updates versions in the .rc file

bool forceAllowed = true;
UINT windowAppTitleResourceId = IDS_APP_TITLE;
UINT windowClassNameResourceId = IDC_GGXRDHITBOXINJECTOR;
LPCWSTR windowIconId = MAKEINTRESOURCEW(IDI_GGXRDHITBOXINJECTOR);
LPCWSTR windowMenuName = MAKEINTRESOURCEW(IDC_GGXRDHITBOXINJECTOR);
LPCWSTR windowAcceleratorId = MAKEINTRESOURCEW(IDC_GGXRDHITBOXINJECTOR);

bool parseArgs(int argc, LPWSTR* argv, int* exitCode) {
	for (int i = 0; i < argc; ++i) {
		if (_wcsicmp(argv[i], L"/?") == 0
				|| _wcsicmp(argv[i], L"--help") == 0
				|| _wcsicmp(argv[i], L"-help") == 0) {
			MessageBoxA(NULL,
				"Injector for ggxrd_hitbox_overlay for Guilty Gear Xrd Rev2 version 2211."
				" Arguments:\n"
				" <None> - launch a window in interactive mode.\n"
				" -force - attempt to inject silently.",
				"ggxrd_hitbox_injector " INJECTOR_VERSION,
				MB_OK);
			*exitCode = 0;
			return false;
		} else if (_wcsicmp(argv[i], L"-force") == 0) {
			force = true;
		}
	}
	return true;
}

unsigned long __stdcall taskThreadProc(LPVOID unused) {
	unsigned long result = injectorMain();
	PostMessageW(mainWindow, WM_TASK_ENDED, 0, 0);
	return result;
}
