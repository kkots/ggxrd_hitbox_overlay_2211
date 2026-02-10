#include "pch.h"
#include "Keyboard.h"
#include "logging.h"
#include "Settings.h"
#include "memoryFunctions.h"
#include "KeyDefinitions.h"
#include "Detouring.h"

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
			keyboard.windowThreadId = windsThreadId;
		}
	}
	return TRUE;
}

extern "C" void getJoyStateHookAsm();  // defined in asmhooks.asm
extern "C" HRESULT __cdecl getJoyStateHook(void* getDeviceStatePtr, void* directInputDevice, size_t size, DIJOYSTATE2* joyState) {  // defined here
	typedef HRESULT (__stdcall*GetDeviceState_t)(void*,size_t,void*);
	HRESULT result = ((GetDeviceState_t)getDeviceStatePtr)(directInputDevice, size, joyState);
	memcpy(&keyboard.joy, joyState, sizeof DIJOYSTATE2);
	if (keyboard.captureJoyInput) {
		Keyboard::resetJoyStruct(joyState);
	}
	return result;
}

bool Keyboard::onDllMain() {
	thisProcessId = GetCurrentProcessId();
	EnumWindows(EnumWindowsFindMyself, NULL);
	if (!thisProcessWindow) {
		logwrap(fputs("Could not find this process' window\n", logfile));
	}
	
	if (!isInitialized()) {
		initialize();
	}
	
	HMODULE xrd = GetModuleHandleA("GuiltyGearXrd.exe");
	uintptr_t patchPlace = 0x00d4845a - 0x400000 + (uintptr_t)xrd;
	std::vector<char> newBytes(9);
	memcpy(newBytes.data(), "\xe8\x00\x00\x00\x00\x90\x90\x90\x90", 9);
	int offset = calculateRelativeCallOffset(patchPlace, (uintptr_t)getJoyStateHookAsm);
	memcpy(newBytes.data() + 1, &offset, 4);
	detouring.patchPlace(patchPlace, newBytes);
	
	return true;
}

void Keyboard::initialize() {
	initialized = true;
	
	if (settings.getMaxKeyCode() != INT_MIN
			&& settings.getMaxKeyCode() >= 0) {
		size_t newSize = settings.getMaxKeyCode() + 1;
		codeToStatus.resize(newSize);
		codeToMovable.resize(newSize);
		
		#define keyEnumFunc(identifier, userFriendlyName, virtualKeyCode, movable) \
			codeToMovable[virtualKeyCode] = movable;
		#define keyEnumFuncLast(identifier, userFriendlyName, virtualKeyCode, movable) keyEnumFunc(identifier, userFriendlyName, virtualKeyCode, movable);
		#define keyEnumFunc_keyRange(str) \
			for (const char* c = str; *c != '\0'; ++c) { \
				codeToMovable[*c] = MULTIPLICATION_WHAT_KEYBOARD; \
			}
		keyEnum
		#undef keyEnumFunc
		#undef keyEnumFuncLast
		#undef keyEnumFunc_keyRange
	}
}

void Keyboard::updateKeyStatuses() {
	std::unique_lock<std::mutex> guard;
	if (!mutexLockedFromOutside)
		guard = std::unique_lock<std::mutex>(mutex);
	
	int mouseMoveX = 0;
	int mouseMoveY = 0;
	int wheel = wheelDelta;
	wheelDelta = 0;
	if (imguiHovered) wheel = 0;
	
	if (keyboard.thisProcessWindow && screenSizeKnown) {
		if (firstTimeGettingMousePos || imguiHovered) {
			if (getMousePos(&lastMousePos)) {
				firstTimeGettingMousePos = false;
			}
		} else {
			POINT currentMousePos;
			if (getMousePos(&currentMousePos)) {
				
				mouseMoveX = currentMousePos.x - lastMousePos.x;
				mouseMoveY = currentMousePos.y - lastMousePos.y;
				
				lastMousePos = currentMousePos;
			}
		}
	}
	
	const bool windowActive = isWindowActive();
	for (KeyStatus& status : statuses) {
		status.moveAmount = 0;
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
					status.moveAmount = isPressed ? 1 : 0;
					break;
				case JOY_LEFT_STICK_LEFT:
					isPressed = joy.lX < 32767 - 3000;
					status.moveAmount = 32767 - joy.lX;
					break;
				case JOY_LEFT_STICK_UP:
					isPressed = joy.lY < 32767 - 3000;
					status.moveAmount = 32767 - joy.lY;
					break;
				case JOY_LEFT_STICK_RIGHT:
					isPressed = joy.lX > 32767 + 3000;
					status.moveAmount = joy.lX - 32767;
					break;
				case JOY_LEFT_STICK_DOWN:
					isPressed = joy.lY > 32767 + 3000;
					status.moveAmount = joy.lY - 32767;
					break;
				case JOY_DPAD_LEFT:
					isPressed = joy.rgdwPOV[0] == 31500
						|| joy.rgdwPOV[0] == 27000
						|| joy.rgdwPOV[0] == 22500;
					status.moveAmount = isPressed ? 1 : 0;
					break;
				case JOY_DPAD_UP:
					isPressed = joy.rgdwPOV[0] == 0
						|| joy.rgdwPOV[0] == 31500
						|| joy.rgdwPOV[0] == 4500;
					status.moveAmount = isPressed ? 1 : 0;
					break;
				case JOY_DPAD_RIGHT:
					isPressed = joy.rgdwPOV[0] == 4500
						|| joy.rgdwPOV[0] == 9000
						|| joy.rgdwPOV[0] == 13500;
					status.moveAmount = isPressed ? 1 : 0;
					break;
				case JOY_DPAD_DOWN:
					isPressed = joy.rgdwPOV[0] == 22500
						|| joy.rgdwPOV[0] == 18000
						|| joy.rgdwPOV[0] == 13500;
					status.moveAmount = isPressed ? 1 : 0;
					break;
				case JOY_PS4_DUALSHOCK_RIGHT_STICK_LEFT:
					isPressed = joy.lZ < 32767 - 3000;
					status.moveAmount = 32767 - joy.lZ;
					break;
				case JOY_PS4_DUALSHOCK_RIGHT_STICK_UP:
					isPressed = joy.lRz < 32767 - 3000;
					status.moveAmount = 32767 - joy.lRz;
					break;
				case JOY_PS4_DUALSHOCK_RIGHT_STICK_RIGHT:
					isPressed = joy.lZ > 32767 + 3000;
					status.moveAmount = joy.lZ - 32767;
					break;
				case JOY_PS4_DUALSHOCK_RIGHT_STICK_DOWN:
					isPressed = joy.lRz > 32767 + 3000;
					status.moveAmount = joy.lRz - 32767;
					break;
				case JOY_XBOX_TYPE_S_RIGHT_STICK_LEFT:
					isPressed = joy.lRx < 32767 - 3000;
					status.moveAmount = 32767 - joy.lRx;
					break;
				case JOY_XBOX_TYPE_S_RIGHT_STICK_UP:
					isPressed = joy.lRy < 32767 - 3000;
					status.moveAmount = 32767 - joy.lRy;
					break;
				case JOY_XBOX_TYPE_S_RIGHT_STICK_RIGHT:
					isPressed = joy.lRx > 32767 + 3000;
					status.moveAmount = joy.lRx - 32767;
					break;
				case JOY_XBOX_TYPE_S_RIGHT_STICK_DOWN:
					isPressed = joy.lRy > 32767 + 3000;
					status.moveAmount = joy.lRy - 32767;
					break;
				case MOUSE_MOVE_LEFT:
					isPressed = mouseMoveX < 0;
					status.moveAmount = -mouseMoveX;
					break;
				case MOUSE_MOVE_RIGHT:
					isPressed = mouseMoveX > 0;
					status.moveAmount = mouseMoveX;
					break;
				case MOUSE_MOVE_UP:
					isPressed = mouseMoveY < 0;
					status.moveAmount = -mouseMoveY;
					break;
				case MOUSE_MOVE_DOWN:
					isPressed = mouseMoveY > 0;
					status.moveAmount = mouseMoveY;
					break;
				case MOUSE_WHEEL_UP:
					isPressed = wheel > 0;
					status.moveAmount = wheel;
					break;
				case MOUSE_WHEEL_DOWN:
					isPressed = wheel < 0;
					status.moveAmount = -wheel;
					break;
				case VK_UP:
				case VK_DOWN:
				case VK_LEFT:
				case VK_RIGHT:
					isPressed = !imguiContextMenuOpen && isKeyCodePressed(status.code);
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

void Keyboard::markAllKeyCodesUnused() {
	// only settings.cpp is going to use this, and other actors can't add or remove elements, so no need for mutexes
	for (KeyStatus& status : statuses) {
		status.unused = true;
	}
}

void Keyboard::removeAllKeyCodes() {
	std::unique_lock<std::mutex> guard;
	if (!mutexLockedFromOutside)
		guard = std::unique_lock<std::mutex>(mutex);
	
	for (const KeyStatus& status : statuses) {
		codeToStatus[status.code] = 0;
	}
	statuses.clear();
}

void Keyboard::removeUnusedKeyCodes() {
	std::unique_lock<std::mutex> guard;
	if (!mutexLockedFromOutside)
		guard = std::unique_lock<std::mutex>(mutex);
	
	auto it = statuses.begin();
	auto endIt = statuses.end();
	auto lastIt = endIt;
	for (; it != statuses.end(); ++it) {
		KeyStatus& status = *it;
		if (status.unused) {
			if (lastIt == endIt) {
				lastIt = it;
			}
			codeToStatus[status.code] = 0;
		} else {
			if (lastIt != endIt) {
				int itIndex = it - statuses.begin();
				int erasedCount = it - lastIt;
				statuses.erase(lastIt, it);
				it = statuses.begin() + itIndex - erasedCount;
				endIt = statuses.end();
				lastIt = endIt;
			}
			codeToStatus[status.code] = it - statuses.begin() + 1;
		}
	}
	if (lastIt != endIt) {
		statuses.erase(lastIt, endIt);
	}
}

void Keyboard::addNewKeyCodes(const std::vector<int>& keyCodes) {
	std::unique_lock<std::mutex> guard;
	if (!mutexLockedFromOutside)
		guard = std::unique_lock<std::mutex>(mutex);
	
	for (int code : keyCodes) {
		int codeToStatusIndex = code;
		bool negate = codeToStatusIndex < 0;
		if (negate) {
			codeToStatusIndex = -codeToStatusIndex;
		}
		if ((size_t)codeToStatusIndex >= codeToStatus.size()) return;
		if (codeToStatus[codeToStatusIndex] == 0) {
			codeToStatus[codeToStatusIndex] = (int)statuses.size() + 1;
			
			statuses.push_back(KeyStatus{ codeToStatusIndex, false, false, codeToMovable[codeToStatusIndex] });
		} else {
			statuses[codeToStatus[codeToStatusIndex] - 1].unused = false;
		}
	}
}

bool Keyboard::gotPressed(const std::vector<int>& keyCodes) {
	if (imguiOwner && imguiOwner != owner) return false;
	std::unique_lock<std::mutex> guard;
	std::unique_lock<std::mutex> guardSettings;
	// reading from the main thread does not require locking, as the data won't be modified
	if (!mutexLockedFromOutside && GetCurrentThreadId() != windowThreadId) {
		guard = std::unique_lock<std::mutex>(mutex);
		guardSettings = std::unique_lock<std::mutex>(settings.keyCombosMutex);
	}
	bool hasNonModifierKeys = false;
	for (int code : keyCodes) {
		if (!isModifierKey(code < 0 ? -code : code)) {
			hasNonModifierKeys = true;
			break;
		}
	}
	
	bool atLeastOneGotPressed = false;

	if (hasNonModifierKeys) {
		for (int code : keyCodes) {
			int absCode = code < 0 ? -code : code;
			KeyStatus* status = getStatus(absCode);
			if (!status) return false;
			if (status->isPressed != (code >= 0)) return false;
			if (!isModifierKey(absCode) && code >= 0) {
				if (status->gotPressed) atLeastOneGotPressed = true;
			}
		}
	} else {
		for (int code : keyCodes) {
			KeyStatus* status = getStatus(code < 0 ? -code : code);
			if (!status) return false;
			if (status->isPressed != (code >= 0)) return false;
			if (status->gotPressed && code >= 0) atLeastOneGotPressed = true;
		}
	}

	return atLeastOneGotPressed;
}

bool Keyboard::isHeld(const std::vector<int>& keyCodes) {
	if (imguiOwner && imguiOwner != owner) return false;
	std::unique_lock<std::mutex> guard;
	std::unique_lock<std::mutex> guardSettings;
	// reading from the main thread does not require locking the mutex, as the data won't be modified
	if (!mutexLockedFromOutside && GetCurrentThreadId() != windowThreadId) {
		guard = std::unique_lock<std::mutex>(mutex);
		guardSettings = std::unique_lock<std::mutex>(settings.keyCombosMutex);
	}
	if (keyCodes.empty()) return false;
	for (int code : keyCodes) {
		KeyStatus* status = getStatus(code < 0 ? -code : code);
		if (!status || status->isPressed != (code >= 0)) return false;
	}
	return true;
}

float Keyboard::moveAmount(const std::vector<int>& keyCodes, MultiplicationGoal goal) {
	if (imguiOwner && imguiOwner != owner) return 0.F;
	std::unique_lock<std::mutex> guard;
	std::unique_lock<std::mutex> guardSettings;
	// reading from the main thread does not require locking, as the data won't be modified
	if (!mutexLockedFromOutside && GetCurrentThreadId() != windowThreadId) {
		guard = std::unique_lock<std::mutex>(mutex);
		guardSettings = std::unique_lock<std::mutex>(settings.keyCombosMutex);
	}
	if (keyCodes.empty()) return 0.F;
	float largestMoveAbsolute = 0.F;
	float largestMove = 0.F;
	for (int code : keyCodes) {
		KeyStatus* status = getStatus(code < 0 ? -code : code);
		if (!status || status->isPressed != (code >= 0)) return false;
		float moveAmount = (float)status->moveAmount * multiplicationTable[status->movable][goal];
		float moveAmountAbs = moveAmount < 0 ? -moveAmount : moveAmount;
		if (moveAmountAbs > largestMoveAbsolute) {
			largestMoveAbsolute = moveAmountAbs;
			largestMove = moveAmount;
		}
	}
	return largestMove;
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
	if (code < 0) return nullptr;
	if ((size_t)code >= codeToStatus.size()) return nullptr;
	int statusesIndex = codeToStatus[code];
	if (statusesIndex == 0) return nullptr;
	return &statuses[statusesIndex - 1];
}

Keyboard::MutexLockedFromOutsideGuard::MutexLockedFromOutsideGuard() {
	keyboard.mutexLockedFromOutside = true;
}

Keyboard::MutexLockedFromOutsideGuard::~MutexLockedFromOutsideGuard() {
	keyboard.mutexLockedFromOutside = false;
}

void Keyboard::getJoyState(DIJOYSTATE2* state) {
	
	resetJoyStruct(state);
	
	if (!UWindowsClient_Joysticks) {
		if (UWindowsClient_Joysticks_HookAttempted) {
			return;
		}
		UWindowsClient_Joysticks_HookAttempted = true;
		
		UWindowsClient_Joysticks = (BYTE*)sigscanOffset(
			GUILTY_GEAR_XRD_EXE,
			"69 ff fc 01 00 00 03 3d ?? ?? ?? ?? 68 10 01 00 00",
			{ 8, 0 },
			nullptr, "UWindowsClient_Joysticks");
		if (!UWindowsClient_Joysticks) {
			return;
		}
		finishedSigscanning();
	}
	
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

void Keyboard::clearJoyState() {
	
	if (!UWindowsClient_Joysticks) return;
	
	int ArrayNum = *(int*)(UWindowsClient_Joysticks + 4);
	BYTE* FJoystickInfo = *(BYTE**)UWindowsClient_Joysticks;
	while (ArrayNum >= 0) {
		if (*(void**)FJoystickInfo != nullptr  // LPDIRECTINPUTDEVICE8W DirectInput8Joystick
				&& *(BOOL*)(FJoystickInfo + 0x1f0)) {  // BOOL bIsConnected
			memset(FJoystickInfo + 0xc, 0, 4*16);
		}
		
		FJoystickInfo += 0x1fc;
		--ArrayNum;
	}
	
}


bool Keyboard::getMousePos(POINT* result) {
	if (!GetCursorPos(result)) return false;
	if (!ScreenToClient(thisProcessWindow, result)) return false;
	if (usePresentRect) {
		float v;
		
		v = (float)result->x;
		v = v * screenWidth / (float)presentRectW;
		result->x = (LONG)std::roundf(v);
		
		v = (float)result->y;
		v = v * screenHeight / (float)presentRectH;
		result->y = (LONG)std::roundf(v);
		
	}
	return true;
}

void Keyboard::resetJoyStruct(DIJOYSTATE2* ptr) {
	memset(ptr, 0, sizeof DIJOYSTATE2);
	ptr->lX = 32767;
	ptr->lY = 32767;
	ptr->rgdwPOV[0] = -1;
}
