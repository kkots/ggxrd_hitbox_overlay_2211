#pragma once
#include <vector>

BOOL CALLBACK EnumWindowsFindMyself(HWND hwnd, LPARAM lParam);

class Keyboard
{
public:
	bool onDllMain();
	DWORD thisProcessId = 0;
	HWND thisProcessWindow = NULL;
	void updateKeyStatuses();
	bool gotPressed(const std::vector<int>& keyCodes);
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
};

extern Keyboard keyboard;
