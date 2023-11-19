#pragma once
#include <vector>
#include <mutex>

BOOL CALLBACK EnumWindowsFindMyself(HWND hwnd, LPARAM lParam);

class Keyboard
{
public:
	bool onDllMain();
	DWORD thisProcessId = 0;
	HWND thisProcessWindow = NULL;
	void updateKeyStatuses();
	bool gotPressed(const std::vector<int>& keyCodes);
	bool isHeld(const std::vector<int>& keyCodes);
	void addNewKeyCodes(const std::vector<int>& keyCodes);
	void removeAllKeyCodes();
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
	std::mutex mutex;
};

extern Keyboard keyboard;
