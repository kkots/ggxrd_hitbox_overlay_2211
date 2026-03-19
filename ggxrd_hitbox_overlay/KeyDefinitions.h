#pragma once
// this file gets parsed by regenerate_ini_and_update_readme.ps1. Do not change its structure
#define keyEnum \
	keyEnumFunc("Backspace", "Backspace", VK_BACK, MULTIPLICATION_WHAT_KEYBOARD, true) \
	keyEnumFunc("Tab", "Tab", VK_TAB, MULTIPLICATION_WHAT_KEYBOARD, true) \
	keyEnumFunc("Enter", "Enter", VK_RETURN, MULTIPLICATION_WHAT_KEYBOARD, true) \
	keyEnumFunc("PauseBreak", "PauseBreak", VK_PAUSE, MULTIPLICATION_WHAT_KEYBOARD, false) \
	keyEnumFunc("CapsLock", "CapsLock", VK_CAPITAL, MULTIPLICATION_WHAT_KEYBOARD, true) \
	keyEnumFunc("Escape", "Escape", VK_ESCAPE, MULTIPLICATION_WHAT_KEYBOARD, false) \
	keyEnumFunc("Space", "Space", VK_SPACE, MULTIPLICATION_WHAT_KEYBOARD, true) \
	keyEnumFunc("PageUp", "PageUp", VK_PRIOR, MULTIPLICATION_WHAT_KEYBOARD, false) \
	keyEnumFunc("PageDown", "PadeDown", VK_NEXT, MULTIPLICATION_WHAT_KEYBOARD, false) \
	keyEnumFunc("End", "End", VK_END, MULTIPLICATION_WHAT_KEYBOARD, true) \
	keyEnumFunc("Home", "Home", VK_HOME, MULTIPLICATION_WHAT_KEYBOARD, true) \
	keyEnumFunc("Left", "Arrow Left", VK_LEFT, MULTIPLICATION_WHAT_KEYBOARD, true) \
	keyEnumFunc("Up", "Arrow Up", VK_UP, MULTIPLICATION_WHAT_KEYBOARD, true) \
	keyEnumFunc("Right", "Arrow Right", VK_RIGHT, MULTIPLICATION_WHAT_KEYBOARD, true) \
	keyEnumFunc("Down", "Arrow Down", VK_DOWN, MULTIPLICATION_WHAT_KEYBOARD, true) \
	keyEnumFunc("PrintScreen", "PrintScreen", VK_SNAPSHOT, MULTIPLICATION_WHAT_KEYBOARD, false) \
	keyEnumFunc("Insert", "Insert", VK_INSERT, MULTIPLICATION_WHAT_KEYBOARD, false) \
	keyEnumFunc("Delete", "Delete", VK_DELETE, MULTIPLICATION_WHAT_KEYBOARD, true) \
	keyEnumFunc("Num0", "Num0", VK_NUMPAD0, MULTIPLICATION_WHAT_KEYBOARD, true) \
	keyEnumFunc("Num1", "Num1", VK_NUMPAD1, MULTIPLICATION_WHAT_KEYBOARD, true) \
	keyEnumFunc("Num2", "Num2", VK_NUMPAD2, MULTIPLICATION_WHAT_KEYBOARD, true) \
	keyEnumFunc("Num3", "Num3", VK_NUMPAD3, MULTIPLICATION_WHAT_KEYBOARD, true) \
	keyEnumFunc("Num4", "Num4", VK_NUMPAD4, MULTIPLICATION_WHAT_KEYBOARD, true) \
	keyEnumFunc("Num5", "Num5", VK_NUMPAD5, MULTIPLICATION_WHAT_KEYBOARD, true) \
	keyEnumFunc("Num6", "Num6", VK_NUMPAD6, MULTIPLICATION_WHAT_KEYBOARD, true) \
	keyEnumFunc("Num7", "Num7", VK_NUMPAD7, MULTIPLICATION_WHAT_KEYBOARD, true) \
	keyEnumFunc("Num8", "Num8", VK_NUMPAD8, MULTIPLICATION_WHAT_KEYBOARD, true) \
	keyEnumFunc("Num9", "Num9", VK_NUMPAD9, MULTIPLICATION_WHAT_KEYBOARD, true) \
	keyEnumFunc("NumMultiply", "Num*", VK_MULTIPLY, MULTIPLICATION_WHAT_KEYBOARD, true) \
	keyEnumFunc("NumAdd", "Num+", VK_ADD, MULTIPLICATION_WHAT_KEYBOARD, true) \
	keyEnumFunc("NumSubtract", "Num-", VK_SUBTRACT, MULTIPLICATION_WHAT_KEYBOARD, true) \
	keyEnumFunc("NumDecimal", "Num.", VK_DECIMAL, MULTIPLICATION_WHAT_KEYBOARD, true) \
	keyEnumFunc("NumDivide", "Num/", VK_DIVIDE, MULTIPLICATION_WHAT_KEYBOARD, true) \
	keyEnumFunc("F1", "F1", VK_F1, MULTIPLICATION_WHAT_KEYBOARD, false) \
	keyEnumFunc("F2", "F2", VK_F2, MULTIPLICATION_WHAT_KEYBOARD, false) \
	keyEnumFunc("F3", "F3", VK_F3, MULTIPLICATION_WHAT_KEYBOARD, false) \
	keyEnumFunc("F4", "F4", VK_F4, MULTIPLICATION_WHAT_KEYBOARD, false) \
	keyEnumFunc("F5", "F5", VK_F5, MULTIPLICATION_WHAT_KEYBOARD, false) \
	keyEnumFunc("F6", "F6", VK_F6, MULTIPLICATION_WHAT_KEYBOARD, false) \
	keyEnumFunc("F7", "F7", VK_F7, MULTIPLICATION_WHAT_KEYBOARD, false) \
	keyEnumFunc("F8", "F8", VK_F8, MULTIPLICATION_WHAT_KEYBOARD, false) \
	keyEnumFunc("F9", "F9", VK_F9, MULTIPLICATION_WHAT_KEYBOARD, false) \
	keyEnumFunc("F10", "F10", VK_F10, MULTIPLICATION_WHAT_KEYBOARD, false) \
	keyEnumFunc("F11", "F11", VK_F11, MULTIPLICATION_WHAT_KEYBOARD, false) \
	keyEnumFunc("F12", "F12", VK_F12, MULTIPLICATION_WHAT_KEYBOARD, false) \
	keyEnumFunc("NumLock", "NumLock", VK_NUMLOCK, MULTIPLICATION_WHAT_KEYBOARD, true) \
	keyEnumFunc("ScrollLock", "ScrollLock", VK_SCROLL, MULTIPLICATION_WHAT_KEYBOARD, false) \
	keyEnumFunc("Colon", ":", VK_OEM_1, MULTIPLICATION_WHAT_KEYBOARD, true) \
	keyEnumFunc("Plus", "+", VK_OEM_PLUS, MULTIPLICATION_WHAT_KEYBOARD, true) \
	keyEnumFunc("Minus", "-", VK_OEM_MINUS, MULTIPLICATION_WHAT_KEYBOARD, true) \
	keyEnumFunc("Comma", ",", VK_OEM_COMMA, MULTIPLICATION_WHAT_KEYBOARD, true) \
	keyEnumFunc("Period", ".", VK_OEM_PERIOD, MULTIPLICATION_WHAT_KEYBOARD, true) \
	keyEnumFunc("Slash", "/", VK_OEM_2, MULTIPLICATION_WHAT_KEYBOARD, true) \
	keyEnumFunc("Tilde", "~", VK_OEM_3, MULTIPLICATION_WHAT_KEYBOARD, true) \
	keyEnumFunc("OpenSquareBracket", "[", VK_OEM_4, MULTIPLICATION_WHAT_KEYBOARD, true) \
	keyEnumFunc("Backslash", "\\", VK_OEM_5, MULTIPLICATION_WHAT_KEYBOARD, true) \
	keyEnumFunc("CloseSquareBracket", "]", VK_OEM_6, MULTIPLICATION_WHAT_KEYBOARD, true) \
	keyEnumFunc("Quote", "\"", VK_OEM_7, MULTIPLICATION_WHAT_KEYBOARD, true) \
	keyEnumFunc("Backslash2", "\\ (2)", VK_OEM_102, MULTIPLICATION_WHAT_KEYBOARD, true) \
	keyEnumFunc_keyRange("0123456789") \
	keyEnumFunc_keyRange("ABCDEFGHIJKLMNOPQRSTUVWXYZ") \
	keyEnumFunc("Shift", "Shift", VK_SHIFT, MULTIPLICATION_WHAT_KEYBOARD, false) \
	keyEnumFunc("Ctrl", "Ctrl", VK_CONTROL, MULTIPLICATION_WHAT_KEYBOARD, false) \
	keyEnumFunc("Alt", "Alt", VK_MENU, MULTIPLICATION_WHAT_KEYBOARD, false) \
	keyEnumFunc("LeftClick", "Left Click", VK_LBUTTON, MULTIPLICATION_WHAT_KEYBOARD, false) \
	keyEnumFunc("RightClick", "Right Click", VK_RBUTTON, MULTIPLICATION_WHAT_KEYBOARD, false) \
	keyEnumFunc("MiddleClick", "Middle Click", VK_MBUTTON, MULTIPLICATION_WHAT_KEYBOARD, false) \
	keyEnumFunc("ThumbMouseClick", "Thumb Mouse Click", VK_XBUTTON1, MULTIPLICATION_WHAT_KEYBOARD, false) \
	keyEnumFunc("ThumbMouseClick2", "Thumb Mouse Click 2", VK_XBUTTON2, MULTIPLICATION_WHAT_KEYBOARD, false) \
	keyEnumFunc("MouseMoveLeft", "Mouse Move Left", MOUSE_MOVE_LEFT, MULTIPLICATION_WHAT_MOUSE, false) \
	keyEnumFunc("MouseMoveUp", "Mouse Move Up", MOUSE_MOVE_UP, MULTIPLICATION_WHAT_MOUSE, false) \
	keyEnumFunc("MouseMoveRight", "Mouse Move Right", MOUSE_MOVE_RIGHT, MULTIPLICATION_WHAT_MOUSE, false) \
	keyEnumFunc("MouseMoveDown", "Mouse Move Down", MOUSE_MOVE_DOWN, MULTIPLICATION_WHAT_MOUSE, false) \
	keyEnumFunc("MouseWheelUp", "Mouse Wheel Up", MOUSE_WHEEL_UP, MULTIPLICATION_WHAT_WHEEL, false) \
	keyEnumFunc("MouseWheelDown", "Mouse Wheel Down", MOUSE_WHEEL_DOWN, MULTIPLICATION_WHAT_WHEEL, false) \
	keyEnumFunc("JoystickBtn1", "JoystickBtn1", JOY_BTN_0, MULTIPLICATION_WHAT_KEYBOARD, false) \
	keyEnumFunc("JoystickBtn2", "JoystickBtn2", JOY_BTN_1, MULTIPLICATION_WHAT_KEYBOARD, false) \
	keyEnumFunc("JoystickBtn3", "JoystickBtn3", JOY_BTN_2, MULTIPLICATION_WHAT_KEYBOARD, false) \
	keyEnumFunc("JoystickBtn4", "JoystickBtn4", JOY_BTN_3, MULTIPLICATION_WHAT_KEYBOARD, false) \
	keyEnumFunc("JoystickLeftTrigger", "JoystickLeftTrigger", JOY_BTN_4, MULTIPLICATION_WHAT_KEYBOARD, false) \
	keyEnumFunc("JoystickRightTrigger", "JoystickRightTrigger", JOY_BTN_5, MULTIPLICATION_WHAT_KEYBOARD, false) \
	keyEnumFunc("JoystickLeftTrigger2", "JoystickLeftTrigger2", JOY_BTN_6, MULTIPLICATION_WHAT_KEYBOARD, false) \
	keyEnumFunc("JoystickRightTrigger2", "JoystickRightTrigger2", JOY_BTN_7, MULTIPLICATION_WHAT_KEYBOARD, false) \
	keyEnumFunc("JoystickBtn9", "JoystickBtn9", JOY_BTN_8, MULTIPLICATION_WHAT_KEYBOARD, false) \
	keyEnumFunc("JoystickBtn10", "JoystickBtn10", JOY_BTN_9, MULTIPLICATION_WHAT_KEYBOARD, false) \
	keyEnumFunc("JoystickBtn11", "JoystickBtn11", JOY_BTN_10, MULTIPLICATION_WHAT_KEYBOARD, false) \
	keyEnumFunc("JoystickBtn12", "JoystickBtn12", JOY_BTN_11, MULTIPLICATION_WHAT_KEYBOARD, false) \
	keyEnumFunc("JoystickBtn13", "JoystickBtn13", JOY_BTN_12, MULTIPLICATION_WHAT_KEYBOARD, false) \
	keyEnumFunc("JoystickBtn14", "JoystickBtn14", JOY_BTN_13, MULTIPLICATION_WHAT_KEYBOARD, false) \
	keyEnumFunc("JoystickBtn15", "JoystickBtn15", JOY_BTN_14, MULTIPLICATION_WHAT_KEYBOARD, false) \
	keyEnumFunc("JoystickBtn16", "JoystickBtn16", JOY_BTN_15, MULTIPLICATION_WHAT_KEYBOARD, false) \
	keyEnumFunc("LeftStickLeft", "LeftStickLeft", JOY_LEFT_STICK_LEFT, MULTIPLICATION_WHAT_LEFT_STICK, false) \
	keyEnumFunc("LeftStickUp", "LeftStickUp", JOY_LEFT_STICK_UP, MULTIPLICATION_WHAT_LEFT_STICK, false) \
	keyEnumFunc("LeftStickRight", "LeftStickRight", JOY_LEFT_STICK_RIGHT, MULTIPLICATION_WHAT_LEFT_STICK, false) \
	keyEnumFunc("LeftStickDown", "LeftStickDown", JOY_LEFT_STICK_DOWN, MULTIPLICATION_WHAT_LEFT_STICK, false) \
	keyEnumFunc("DPadLeft", "DPadLeft", JOY_DPAD_LEFT, MULTIPLICATION_WHAT_DPAD, false) \
	keyEnumFunc("DPadUp", "DPadUp", JOY_DPAD_UP, MULTIPLICATION_WHAT_DPAD, false) \
	keyEnumFunc("DPadRight", "DPadRight", JOY_DPAD_RIGHT, MULTIPLICATION_WHAT_DPAD, false) \
	keyEnumFunc("DPadDown", "DPadDown", JOY_DPAD_DOWN, MULTIPLICATION_WHAT_DPAD, false) \
	keyEnumFunc("PS4DualshockRightStickLeft", "PS4DualshockRightStickLeft", JOY_PS4_DUALSHOCK_RIGHT_STICK_LEFT, MULTIPLICATION_WHAT_PS4_RIGHT_STICK, false) \
	keyEnumFunc("PS4DualshockRightStickUp", "PS4DualshockRightStickUp", JOY_PS4_DUALSHOCK_RIGHT_STICK_UP, MULTIPLICATION_WHAT_PS4_RIGHT_STICK, false) \
	keyEnumFunc("PS4DualshockRightStickRight", "PS4DualshockRightStickRight", JOY_PS4_DUALSHOCK_RIGHT_STICK_RIGHT, MULTIPLICATION_WHAT_PS4_RIGHT_STICK, false) \
	keyEnumFunc("PS4DualshockRightStickDown", "PS4DualshockRightStickDown", JOY_PS4_DUALSHOCK_RIGHT_STICK_DOWN, MULTIPLICATION_WHAT_PS4_RIGHT_STICK, false) \
	keyEnumFunc("XboxTypeSRightStickLeft", "XboxTypeSRightStickLeft", JOY_XBOX_TYPE_S_RIGHT_STICK_LEFT, MULTIPLICATION_WHAT_XBOX_TYPE_S_RIGHT_STICK, false) \
	keyEnumFunc("XboxTypeSRightStickUp", "XboxTypeSRightStickUp", JOY_XBOX_TYPE_S_RIGHT_STICK_UP, MULTIPLICATION_WHAT_XBOX_TYPE_S_RIGHT_STICK, false) \
	keyEnumFunc("XboxTypeSRightStickRight", "XboxTypeSRightStickRight", JOY_XBOX_TYPE_S_RIGHT_STICK_RIGHT, MULTIPLICATION_WHAT_XBOX_TYPE_S_RIGHT_STICK, false) \
	keyEnumFuncLast("XboxTypeSRightStickDown", "XboxTypeSRightStickDown", JOY_XBOX_TYPE_S_RIGHT_STICK_DOWN, MULTIPLICATION_WHAT_XBOX_TYPE_S_RIGHT_STICK, false)