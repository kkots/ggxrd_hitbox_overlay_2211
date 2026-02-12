#pragma once
#include <vector>
#include <mutex>
#include <dinput.h>

enum KeyboardOwner {
	KEYBOARD_OWNER_NONE,
	KEYBOARD_OWNER_IMGUI
};

enum MultiplicationWhat {
	MULTIPLICATION_WHAT_NONE = -1,
	MULTIPLICATION_WHAT_MOUSE = 0,
	MULTIPLICATION_WHAT_KEYBOARD = 1,
	MULTIPLICATION_WHAT_WHEEL = 2,
	MULTIPLICATION_WHAT_LEFT_STICK = 3,
	MULTIPLICATION_WHAT_PS4_RIGHT_STICK = 4,
	MULTIPLICATION_WHAT_XBOX_TYPE_S_RIGHT_STICK = 5,
	MULTIPLICATION_WHAT_PS4_SHOULDER = 6,
	MULTIPLICATION_WHAT_DPAD = 7,
	MULTIPLICATION_WHAT_LAST = 8  // Must always be last
};

enum MultiplicationGoal {
	MULTIPLICATION_GOAL_LOOK,
	MULTIPLICATION_GOAL_CAMERA_MOVE,
	MULTIPLICATION_GOAL_ROLL,
	MULTIPLICATION_GOAL_CHANGE_FOV,
	MULTIPLICATION_GOAL_LAST  // Must always be last
};

class Keyboard
{
public:
	bool onDllMain();
	DWORD thisProcessId = 0;
	HWND thisProcessWindow = NULL;  // only set this once and never update it. Don't worry the window won't go anywhere
	DWORD windowThreadId = NULL;
	void updateKeyStatuses();
	bool gotPressed(const std::vector<int>& keyCodes);
	bool isHeld(const std::vector<int>& keyCodes);
	bool isHeldOmnidirectional(const std::vector<int>& keyCodes);
	float moveAmount(const std::vector<int>& keyCodes, MultiplicationGoal goal);
	void addNewKeyCodes(const std::vector<int>& keyCodes);
	void removeAllKeyCodes();
	void markAllKeyCodesUnused();
	void removeUnusedKeyCodes();
	inline bool isInitialized() const { return initialized; }
	void initialize();
	std::mutex mutex;
	bool mutexLockedFromOutside = false;
	struct MutexLockedFromOutsideGuard {
	public:
		MutexLockedFromOutsideGuard();
		~MutexLockedFromOutsideGuard();
	};
	
	bool screenSizeKnown = false;
	float screenWidth = 0.F;
	float screenHeight = 0.F;
	bool usePresentRect = false;
	int presentRectW = 0;
	int presentRectH = 0;
	
	POINT lastMousePos;
	bool firstTimeGettingMousePos = true;
	bool getMousePos(POINT* result);
	
	float multiplicationTable[MULTIPLICATION_WHAT_LAST][MULTIPLICATION_GOAL_LAST] {
		// MULTIPLICATION_WHAT_MOUSE
		{
			/* MULTIPLICATION_GOAL_LOOK*/        35.F,
			/* MULTIPLICATION_GOAL_CAMERA_MOVE*/ 1.F,
			/* MULTIPLICATION_GOAL_ROLL*/        35.F,
			/* MULTIPLICATION_GOAL_CHANGE_FOV*/  0.1F
		},
		// MULTIPLICATION_WHAT_KEYBOARD
		{
			/* MULTIPLICATION_GOAL_LOOK*/        100.F,
			/* MULTIPLICATION_GOAL_CAMERA_MOVE*/ 2.F,
			/* MULTIPLICATION_GOAL_ROLL*/        200.F,
			/* MULTIPLICATION_GOAL_CHANGE_FOV*/  0.5F
		},
		// MULTIPLICATION_WHAT_WHEEL
		{
			/* MULTIPLICATION_GOAL_LOOK*/        3.F,
			/* MULTIPLICATION_GOAL_CAMERA_MOVE*/ 0.5F,
			/* MULTIPLICATION_GOAL_ROLL*/        3.F,
			/* MULTIPLICATION_GOAL_CHANGE_FOV*/  0.04F
		},
		// MULTIPLICATION_WHAT_LEFT_STICK
		{
			/* MULTIPLICATION_GOAL_LOOK*/        0.01F,
			/* MULTIPLICATION_GOAL_CAMERA_MOVE*/ 0.00025F,
			/* MULTIPLICATION_GOAL_ROLL*/        0.02F,
			/* MULTIPLICATION_GOAL_CHANGE_FOV*/  0.00005F
		},
		// MULTIPLICATION_WHAT_PS4_RIGHT_STICK
		{
			/* MULTIPLICATION_GOAL_LOOK*/        0.01F,
			/* MULTIPLICATION_GOAL_CAMERA_MOVE*/ 0.00025F,
			/* MULTIPLICATION_GOAL_ROLL*/        0.02F,
			/* MULTIPLICATION_GOAL_CHANGE_FOV*/  0.00005F
		},
		// MULTIPLICATION_WHAT_XBOX_TYPE_S_RIGHT_STICK
		{
			/* MULTIPLICATION_GOAL_LOOK*/        0.01F,
			/* MULTIPLICATION_GOAL_CAMERA_MOVE*/ 0.00025F,
			/* MULTIPLICATION_GOAL_ROLL*/        0.02F,
			/* MULTIPLICATION_GOAL_CHANGE_FOV*/  0.00005F
		},
		// MULTIPLICATION_WHAT_PS4_SHOULDER
		{
			/* MULTIPLICATION_GOAL_LOOK*/        0.005F,
			/* MULTIPLICATION_GOAL_CAMERA_MOVE*/ 0.000125F,
			/* MULTIPLICATION_GOAL_ROLL*/        0.01F,
			/* MULTIPLICATION_GOAL_CHANGE_FOV*/  0.000025F
		},
		// MULTIPLICATION_WHAT_DPAD
		{
			/* MULTIPLICATION_GOAL_LOOK*/        100.F,
			/* MULTIPLICATION_GOAL_CAMERA_MOVE*/ 2.F,
			/* MULTIPLICATION_GOAL_ROLL*/        200.F,
			/* MULTIPLICATION_GOAL_CHANGE_FOV*/  0.5F
		}
	};
	
	inline void addWheelDelta(int newDelta) { wheelDelta += newDelta; }
	bool imguiHovered = false;
	bool imguiContextMenuOpen = false;
	bool imguiActive = false;
	KeyboardOwner imguiOwner = KEYBOARD_OWNER_NONE;
	DIJOYSTATE2 joy;
	bool captureJoyInput = false;
	static void resetJoyStruct(DIJOYSTATE2* ptr);
	KeyboardOwner owner = KEYBOARD_OWNER_NONE;
private:
	struct KeyStatus {
		int code = 0;
		bool isPressed = false;
		bool isPressedOmnidirectional = false;
		bool gotPressed = false;
		MultiplicationWhat movable = MULTIPLICATION_WHAT_NONE;
		int moveAmount = 0;
		bool unused = false;
	};
	std::vector<KeyStatus> statuses;
	bool isKeyCodePressed(int code) const;
	bool isWindowActive() const;
	bool isModifierKey(int code) const;
	KeyStatus* getStatus(int code);
	BYTE* UWindowsClient_Joysticks = nullptr;
	bool UWindowsClient_Joysticks_HookAttempted = false;
	void getJoyState(DIJOYSTATE2* state);
	void clearJoyState();
	std::vector<int> codeToStatus;
	std::vector<MultiplicationWhat> codeToMovable;
	bool initialized = false;
	
	int wheelDelta = 0;
};

extern Keyboard keyboard;

// use this to temporarily set the owner in a statement block ({})
struct KeyboardOwnerGuard {
	bool isSet = false;
	inline KeyboardOwnerGuard() { }
	inline KeyboardOwnerGuard(KeyboardOwner owner) {
		isSet = true;
		keyboard.owner = owner;
	}
	inline ~KeyboardOwnerGuard() {
		if (isSet) {
			keyboard.owner = KEYBOARD_OWNER_NONE;
		}
	}
	inline KeyboardOwnerGuard& operator=(const KeyboardOwnerGuard& other) = delete;
	inline KeyboardOwnerGuard& operator=(KeyboardOwnerGuard&& other) noexcept {
		if (isSet && !other.isSet) keyboard.owner = KEYBOARD_OWNER_NONE;
		isSet = other.isSet;
		other.isSet = false;
		return *this;
	}
};

// use this to temporarily ignore any presence of an imguiOwner in a statement block ({})
struct KeyboardIgnoreOwnerGuard {
	KeyboardOwner oldImguiOwner;
	inline KeyboardIgnoreOwnerGuard() : oldImguiOwner(keyboard.imguiOwner) { }
	inline ~KeyboardIgnoreOwnerGuard() {
		keyboard.imguiOwner = oldImguiOwner;
	}
};
