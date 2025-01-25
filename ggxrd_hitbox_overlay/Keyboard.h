#pragma once
#include <vector>
#include <mutex>
#include <dinput.h>

class Keyboard
{
public:
	bool onDllMain();
	DWORD thisProcessId = 0;
	HWND thisProcessWindow = NULL;  // only set this once and never update it. Don't worry the window won't go anywhere
	void updateKeyStatuses();
	bool gotPressed(const std::vector<int>& keyCodes);
	bool isHeld(const std::vector<int>& keyCodes);
	void addNewKeyCodes(const std::vector<int>& keyCodes);
	void removeAllKeyCodes();
	std::mutex mutex;
	bool mutexLockedFromOutside = false;
	struct MutexLockedFromOutsideGuard {
	public:
		MutexLockedFromOutsideGuard();
		~MutexLockedFromOutsideGuard();
	};
private:
	struct KeyStatus {
		int code = 0;
		bool isPressed = false;
		bool gotPressed = false;
	};
	std::vector<KeyStatus> statuses;
	bool isKeyCodePressed(int code) const;
	bool isWindowActive() const;
	bool isModifierKey(int code) const;
	KeyStatus* getStatus(int code);
	BYTE* UWindowsClient_Joysticks = nullptr;
	void getJoyState(DIJOYSTATE2* state) const;
};

extern Keyboard keyboard;
