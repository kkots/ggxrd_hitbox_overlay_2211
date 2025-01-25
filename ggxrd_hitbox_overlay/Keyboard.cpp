#include "pch.h"
#include "Keyboard.h"
#include "logging.h"
#include "Settings.h"
#include "memoryFunctions.h"

Keyboard keyboard;

BOOL CALLBACK EnumWindowsFindMyself(HWND hwnd, LPARAM lParam) {
	DWORD windsProcId = 0;
	DWORD windsThreadId = GetWindowThreadProcessId(hwnd, &windsProcId);
	if (windsProcId == keyboard.thisProcessId) {
		char className[1024] = "";
		GetClassNameA(hwnd, className, _countof(className));
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
	
	UWindowsClient_Joysticks = (BYTE*)sigscanOffset(
		"GuiltyGearXrd.exe",
		"69 ff fc 01 00 00 03 3d ?? ?? ?? ?? 68 10 01 00 00",
		{ 8, 0 },
		nullptr, "UWindowsClient_Joysticks");
	
	return true;
}

void Keyboard::updateKeyStatuses() {
	std::unique_lock<std::mutex> guard;
	if (!mutexLockedFromOutside)
		guard = std::unique_lock<std::mutex>(mutex);
	
	const bool windowActive = isWindowActive();
	DIJOYSTATE2 joy;
	if (windowActive && !statuses.empty()) {
		getJoyState(&joy);
	}
	for (KeyStatus& status : statuses) {
		status.gotPressed = false;
		bool isPressed;
		if (!windowActive) {
			isPressed = false;
		} else {
			switch (status.code) {
				case JOY_BTN_0:
				case JOY_BTN_1:
				case JOY_BTN_2:
				case JOY_BTN_3:
				case JOY_BTN_4:
				case JOY_BTN_5:
				case JOY_BTN_6:
				case JOY_BTN_7:
				case JOY_BTN_8:
				case JOY_BTN_9:
				case JOY_BTN_10:
				case JOY_BTN_11:
				case JOY_BTN_12:
				case JOY_BTN_13:
				case JOY_BTN_14:
				case JOY_BTN_15:
					isPressed = joy.rgbButtons[status.code - JOY_BTN_0];
					break;
				case JOY_LEFT_STICK_LEFT:
					isPressed = joy.lX < 32767 - 3000;
					break;
				case JOY_LEFT_STICK_UP:
					isPressed = joy.lY < 32767 - 3000;
					break;
				case JOY_LEFT_STICK_RIGHT:
					isPressed = joy.lX > 32767 + 3000;
					break;
				case JOY_LEFT_STICK_DOWN:
					isPressed = joy.lY > 32767 + 3000;
					break;
				case JOY_DPAD_LEFT:
					isPressed = joy.rgdwPOV[0] == 31500
						|| joy.rgdwPOV[0] == 27000
						|| joy.rgdwPOV[0] == 22500;
					break;
				case JOY_DPAD_UP:
					isPressed = joy.rgdwPOV[0] == 0
						|| joy.rgdwPOV[0] == 31500
						|| joy.rgdwPOV[0] == 4500;
					break;
				case JOY_DPAD_RIGHT:
					isPressed = joy.rgdwPOV[0] == 4500
						|| joy.rgdwPOV[0] == 9000
						|| joy.rgdwPOV[0] == 13500;
					break;
				case JOY_DPAD_DOWN:
					isPressed = joy.rgdwPOV[0] == 22500
						|| joy.rgdwPOV[0] == 18000
						|| joy.rgdwPOV[0] == 13500;
					break;
				case JOY_PS4_DUALSHOCK_RIGHT_STICK_LEFT:
					isPressed = joy.lZ < 32767 - 3000;
					break;
				case JOY_PS4_DUALSHOCK_RIGHT_STICK_UP:
					isPressed = joy.lRz < 32767 - 3000;
					break;
				case JOY_PS4_DUALSHOCK_RIGHT_STICK_RIGHT:
					isPressed = joy.lZ > 32767 + 3000;
					break;
				case JOY_PS4_DUALSHOCK_RIGHT_STICK_DOWN:
					isPressed = joy.lRz > 32767 + 3000;
					break;
				case JOY_XBOX_TYPE_S_RIGHT_STICK_LEFT:
					isPressed = joy.lRx < 32767 - 3000;
					break;
				case JOY_XBOX_TYPE_S_RIGHT_STICK_UP:
					isPressed = joy.lRy < 32767 - 3000;
					break;
				case JOY_XBOX_TYPE_S_RIGHT_STICK_RIGHT:
					isPressed = joy.lRx > 32767 + 3000;
					break;
				case JOY_XBOX_TYPE_S_RIGHT_STICK_DOWN:
					isPressed = joy.lRy > 32767 + 3000;
					break;
				default:
					isPressed = isKeyCodePressed(status.code);
					break;
			}
		}
		if (isPressed && !status.isPressed) {
			status.gotPressed = true;
		}
		status.isPressed = isPressed;
	}
}

void Keyboard::removeAllKeyCodes() {
	std::unique_lock<std::mutex> guard;
	if (!mutexLockedFromOutside)
		guard = std::unique_lock<std::mutex>(mutex);
	
	statuses.clear();
}

void Keyboard::addNewKeyCodes(const std::vector<int>& keyCodes) {
	std::unique_lock<std::mutex> guard;
	if (!mutexLockedFromOutside)
		guard = std::unique_lock<std::mutex>(mutex);
	
	for (int code : keyCodes) {
		auto found = statuses.end();
		for (auto it = statuses.begin(); it != statuses.end(); ++it) {
			if (it->code == code) {
				found = it;
				break;
			}
		}
		if (found == statuses.end()) {
			statuses.push_back(KeyStatus{ code, false, false });
		}
	}
}

bool Keyboard::gotPressed(const std::vector<int>& keyCodes) {
	std::unique_lock<std::mutex> guard;
	std::unique_lock<std::mutex> guardSettings;
	if (!mutexLockedFromOutside) {
		guard = std::unique_lock<std::mutex>(mutex);
		guardSettings = std::unique_lock<std::mutex>(settings.keyCombosMutex);
	}
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
	std::unique_lock<std::mutex> guard;
	std::unique_lock<std::mutex> guardSettings;
	if (!mutexLockedFromOutside) {
		guard = std::unique_lock<std::mutex>(mutex);
		guardSettings = std::unique_lock<std::mutex>(settings.keyCombosMutex);
	}
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
	return GetForegroundWindow() == thisProcessWindow && thisProcessWindow != NULL;
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

Keyboard::MutexLockedFromOutsideGuard::MutexLockedFromOutsideGuard() {
	keyboard.mutexLockedFromOutside = true;
}

Keyboard::MutexLockedFromOutsideGuard::~MutexLockedFromOutsideGuard() {
	keyboard.mutexLockedFromOutside = false;
}

void Keyboard::getJoyState(DIJOYSTATE2* state) const {
	
	memset(state, 0, sizeof DIJOYSTATE2);
	state->lX = 32767;
	state->lY = 32767;
	state->rgdwPOV[0] = -1;
	
	if (!UWindowsClient_Joysticks) return;
	
	int ArrayNum = *(int*)(UWindowsClient_Joysticks + 4);
	BYTE* FJoystickInfo = *(BYTE**)UWindowsClient_Joysticks;
	while (ArrayNum >= 0) {
		if (*(void**)FJoystickInfo != nullptr  // LPDIRECTINPUTDEVICE8W DirectInput8Joystick
				&& *(BOOL*)(FJoystickInfo + 0x1f0)) {  // BOOL bIsConnected
			*state = *(DIJOYSTATE2*)(FJoystickInfo + 0xd0);
			return;
		}
		
		FJoystickInfo += 0x1fc;
		--ArrayNum;
	}
	
}
