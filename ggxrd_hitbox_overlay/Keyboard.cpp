#include "pch.h"
#include "Keyboard.h"
#include "logging.h"

Keyboard keyboard;

BOOL CALLBACK EnumWindowsFindMyself(HWND hwnd, LPARAM lParam) {
	DWORD windsProcId = 0;
	DWORD windsThreadId = GetWindowThreadProcessId(hwnd, &windsProcId);
	if (windsProcId == keyboard.thisProcessId) {
		char className[1024] = "";
		GetClassName(hwnd, className, _countof(className));
		// we got multiple windows on Linux, need to disambiguate
		if (strcmp(className, "LaunchUnrealUWindowsClient") == 0) {
			keyboard.thisProcessWindow = hwnd;
		}
	}
	return TRUE;
}

bool Keyboard::onDllMain() {
	thisProcessId = GetCurrentProcessId();
	EnumWindows(EnumWindowsFindMyself, NULL);
	if (!thisProcessWindow) {
		logwrap(fputs("Could not find this process' window\n", logfile));
	}
	return true;
}

void Keyboard::updateKeyStatuses() {
	const bool windowActive = isWindowActive();
	for (KeyStatus& status : statuses) {
		status.gotPressed = false;
		const bool isPressed = isKeyCodePressed(status.code);
		if (isPressed && !status.isPressed) {
			if (windowActive) {
				status.gotPressed = true;
			}
		}
		status.isPressed = isPressed;
	}
}

void Keyboard::addNewKeyCodes(const std::vector<int>& keyCodes) {
	for (int code : keyCodes) {
		auto found = statuses.end();
		for (auto it = statuses.begin(); it != statuses.end(); ++it) {
			if (it->code == code) {
				found = it;
				break;
			}
		}
		if (found == statuses.end()) {
			statuses.push_back(KeyStatus{ code, isKeyCodePressed(code), false });
		}
	}
}

bool Keyboard::gotPressed(const std::vector<int>& keyCodes) {

	bool hasNonModifierKeys = false;
	for (int code : keyCodes) {
		if (!isModifierKey(code)) {
			hasNonModifierKeys = true;
			break;
		}
	}
	
	bool atLeastOneGotPressed = false;

	if (hasNonModifierKeys) {
		for (int code : keyCodes) {
			KeyStatus* status = getStatus(code);
			if (!status) return false;
			if (!status->isPressed) return false;
			if (!isModifierKey(code)) {
				if (status->gotPressed) atLeastOneGotPressed = true;
			}
		}
	} else {
		for (int code : keyCodes) {
			KeyStatus* status = getStatus(code);
			if (!status) return false;
			if (!status->isPressed) return false;
			if (status->gotPressed) atLeastOneGotPressed = true;
		}
	}

	return atLeastOneGotPressed;
}

bool Keyboard::isHeld(const std::vector<int>& keyCodes) {
	if (keyCodes.empty()) return false;
	for (int code : keyCodes) {
		KeyStatus* status = getStatus(code);
		if (!status) return false;
		if (!status->isPressed) return false;
	}
	return true;
}

bool Keyboard::isKeyCodePressed(int code) const {
	return (GetKeyState(code) & 0x8000) != 0;
}

bool Keyboard::isWindowActive() const {
	return GetForegroundWindow() == thisProcessWindow;
}

bool Keyboard::isModifierKey(int code) const {
	return code == VK_SHIFT
		|| code == VK_CONTROL
		|| code == VK_MENU;
}

Keyboard::KeyStatus* Keyboard::getStatus(int code) {
	for (KeyStatus& status : statuses) {
		if (status.code == code) {
			return &status;
		}
	}
	return nullptr;
}
