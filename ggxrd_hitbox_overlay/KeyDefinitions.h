#pragma once
// this file gets parsed by regenerate_ini_and_update_readme.ps1. Do not change its structure
#define keyEnum \
	keyEnumFunc("Backspace", "Backspace", VK_BACK, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("Tab", "Tab", VK_TAB, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("Enter", "Enter", VK_RETURN, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("PauseBreak", "PauseBreak", VK_PAUSE, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("CapsLock", "CapsLock", VK_CAPITAL, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("Escape", "Escape", VK_ESCAPE, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("Space", "Space", VK_SPACE, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("PageUp", "PageUp", VK_PRIOR, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("PageDown", "PadeDown", VK_NEXT, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("End", "End", VK_END, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("Home", "Home", VK_HOME, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("Left", "Arrow Left", VK_LEFT, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("Up", "Arrow Up", VK_UP, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("Right", "Arrow Right", VK_RIGHT, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("Down", "Arrow Down", VK_DOWN, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("PrintScreen", "PrintScreen", VK_SNAPSHOT, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("Insert", "Insert", VK_INSERT, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("Delete", "Delete", VK_DELETE, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("Num0", "Num0", VK_NUMPAD0, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("Num1", "Num1", VK_NUMPAD1, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("Num2", "Num2", VK_NUMPAD2, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("Num3", "Num3", VK_NUMPAD3, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("Num4", "Num4", VK_NUMPAD4, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("Num5", "Num5", VK_NUMPAD5, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("Num6", "Num6", VK_NUMPAD6, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("Num7", "Num7", VK_NUMPAD7, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("Num8", "Num8", VK_NUMPAD8, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("Num9", "Num9", VK_NUMPAD9, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("NumMultiply", "Num*", VK_MULTIPLY, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("NumAdd", "Num+", VK_ADD, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("NumSubtract", "Num-", VK_SUBTRACT, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("NumDecimal", "Num.", VK_DECIMAL, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("NumDivide", "Num/", VK_DIVIDE, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("F1", "F1", VK_F1, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("F2", "F2", VK_F2, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("F3", "F3", VK_F3, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("F4", "F4", VK_F4, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("F5", "F5", VK_F5, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("F6", "F6", VK_F6, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("F7", "F7", VK_F7, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("F8", "F8", VK_F8, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("F9", "F9", VK_F9, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("F10", "F10", VK_F10, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("F11", "F11", VK_F11, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("F12", "F12", VK_F12, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("NumLock", "NumLock", VK_NUMLOCK, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("ScrollLock", "ScrollLock", VK_SCROLL, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("Colon", ":", VK_OEM_1, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("Plus", "+", VK_OEM_PLUS, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("Minus", "-", VK_OEM_MINUS, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("Comma", ",", VK_OEM_COMMA, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("Period", ".", VK_OEM_PERIOD, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("Slash", "/", VK_OEM_2, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("Tilde", "~", VK_OEM_3, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("OpenSquareBracket", "[", VK_OEM_4, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("Backslash", "\\", VK_OEM_5, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("CloseSquareBracket", "]", VK_OEM_6, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("Quote", "\"", VK_OEM_7, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("Backslash2", "\\ (2)", VK_OEM_102, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc_keyRange("0123456789") \
	keyEnumFunc_keyRange("ABCDEFGHIJKLMNOPQRSTUVWXYZ") \
	keyEnumFunc("Shift", "Shift", VK_SHIFT, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("Ctrl", "Ctrl", VK_CONTROL, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("Alt", "Alt", VK_MENU, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("LeftClick", "Left Click", VK_LBUTTON, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("RightClick", "Right Click", VK_RBUTTON, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("MiddleClick", "Middle Click", VK_MBUTTON, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("ThumbMouseClick", "Thumb Mouse Click", VK_XBUTTON1, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("ThumbMouseClick2", "Thumb Mouse Click 2", VK_XBUTTON2, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("MouseMoveLeft", "Mouse Move Left", MOUSE_MOVE_LEFT, MULTIPLICATION_WHAT_MOUSE) \
	keyEnumFunc("MouseMoveUp", "Mouse Move Up", MOUSE_MOVE_UP, MULTIPLICATION_WHAT_MOUSE) \
	keyEnumFunc("MouseMoveRight", "Mouse Move Right", MOUSE_MOVE_RIGHT, MULTIPLICATION_WHAT_MOUSE) \
	keyEnumFunc("MouseMoveDown", "Mouse Move Down", MOUSE_MOVE_DOWN, MULTIPLICATION_WHAT_MOUSE) \
	keyEnumFunc("MouseWheelUp", "Mouse Wheel Up", MOUSE_WHEEL_UP, MULTIPLICATION_WHAT_WHEEL) \
	keyEnumFunc("MouseWheelDown", "Mouse Wheel Down", MOUSE_WHEEL_DOWN, MULTIPLICATION_WHAT_WHEEL) \
	keyEnumFunc("JoystickBtn1", "JoystickBtn1", JOY_BTN_0, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("JoystickBtn2", "JoystickBtn2", JOY_BTN_1, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("JoystickBtn3", "JoystickBtn3", JOY_BTN_2, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("JoystickBtn4", "JoystickBtn4", JOY_BTN_3, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("JoystickLeftTrigger", "JoystickLeftTrigger", JOY_BTN_4, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("JoystickRightTrigger", "JoystickRightTrigger", JOY_BTN_5, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("JoystickLeftTrigger2", "JoystickLeftTrigger2", JOY_BTN_6, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("JoystickRightTrigger2", "JoystickRightTrigger2", JOY_BTN_7, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("JoystickBtn9", "JoystickBtn9", JOY_BTN_8, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("JoystickBtn10", "JoystickBtn10", JOY_BTN_9, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("JoystickBtn11", "JoystickBtn11", JOY_BTN_10, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("JoystickBtn12", "JoystickBtn12", JOY_BTN_11, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("JoystickBtn13", "JoystickBtn13", JOY_BTN_12, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("JoystickBtn14", "JoystickBtn14", JOY_BTN_13, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("JoystickBtn15", "JoystickBtn15", JOY_BTN_14, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("JoystickBtn16", "JoystickBtn16", JOY_BTN_15, MULTIPLICATION_WHAT_KEYBOARD) \
	keyEnumFunc("LeftStickLeft", "LeftStickLeft", JOY_LEFT_STICK_LEFT, MULTIPLICATION_WHAT_LEFT_STICK) \
	keyEnumFunc("LeftStickUp", "LeftStickUp", JOY_LEFT_STICK_UP, MULTIPLICATION_WHAT_LEFT_STICK) \
	keyEnumFunc("LeftStickRight", "LeftStickRight", JOY_LEFT_STICK_RIGHT, MULTIPLICATION_WHAT_LEFT_STICK) \
	keyEnumFunc("LeftStickDown", "LeftStickDown", JOY_LEFT_STICK_DOWN, MULTIPLICATION_WHAT_LEFT_STICK) \
	keyEnumFunc("DPadLeft", "DPadLeft", JOY_DPAD_LEFT, MULTIPLICATION_WHAT_DPAD) \
	keyEnumFunc("DPadUp", "DPadUp", JOY_DPAD_UP, MULTIPLICATION_WHAT_DPAD) \
	keyEnumFunc("DPadRight", "DPadRight", JOY_DPAD_RIGHT, MULTIPLICATION_WHAT_DPAD) \
	keyEnumFunc("DPadDown", "DPadDown", JOY_DPAD_DOWN, MULTIPLICATION_WHAT_DPAD) \
	keyEnumFunc("PS4DualshockRightStickLeft", "PS4DualshockRightStickLeft", JOY_PS4_DUALSHOCK_RIGHT_STICK_LEFT, MULTIPLICATION_WHAT_PS4_RIGHT_STICK) \
	keyEnumFunc("PS4DualshockRightStickUp", "PS4DualshockRightStickUp", JOY_PS4_DUALSHOCK_RIGHT_STICK_UP, MULTIPLICATION_WHAT_PS4_RIGHT_STICK) \
	keyEnumFunc("PS4DualshockRightStickRight", "PS4DualshockRightStickRight", JOY_PS4_DUALSHOCK_RIGHT_STICK_RIGHT, MULTIPLICATION_WHAT_PS4_RIGHT_STICK) \
	keyEnumFunc("PS4DualshockRightStickDown", "PS4DualshockRightStickDown", JOY_PS4_DUALSHOCK_RIGHT_STICK_DOWN, MULTIPLICATION_WHAT_PS4_RIGHT_STICK) \
	keyEnumFunc("XboxTypeSRightStickLeft", "XboxTypeSRightStickLeft", JOY_XBOX_TYPE_S_RIGHT_STICK_LEFT, MULTIPLICATION_WHAT_XBOX_TYPE_S_RIGHT_STICK) \
	keyEnumFunc("XboxTypeSRightStickUp", "XboxTypeSRightStickUp", JOY_XBOX_TYPE_S_RIGHT_STICK_UP, MULTIPLICATION_WHAT_XBOX_TYPE_S_RIGHT_STICK) \
	keyEnumFunc("XboxTypeSRightStickRight", "XboxTypeSRightStickRight", JOY_XBOX_TYPE_S_RIGHT_STICK_RIGHT, MULTIPLICATION_WHAT_XBOX_TYPE_S_RIGHT_STICK) \
	keyEnumFuncLast("XboxTypeSRightStickDown", "XboxTypeSRightStickDown", JOY_XBOX_TYPE_S_RIGHT_STICK_DOWN, MULTIPLICATION_WHAT_XBOX_TYPE_S_RIGHT_STICK)