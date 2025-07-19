#pragma once
// this file gets parsed by regenerate_ini_and_update_readme.ps1. Do not change its structure
#define keyEnum \
	keyEnumFunc("Backspace", "Backspace", VK_BACK) \
	keyEnumFunc("Tab", "Tab", VK_TAB) \
	keyEnumFunc("Enter", "Enter", VK_RETURN) \
	keyEnumFunc("PauseBreak", "PauseBreak", VK_PAUSE) \
	keyEnumFunc("CapsLock", "CapsLock", VK_CAPITAL) \
	keyEnumFunc("Escape", "Escape", VK_ESCAPE) \
	keyEnumFunc("Space", "Space", VK_SPACE) \
	keyEnumFunc("PageUp", "PageUp", VK_PRIOR) \
	keyEnumFunc("PageDown", "PadeDown", VK_NEXT) \
	keyEnumFunc("End", "End", VK_END) \
	keyEnumFunc("Home", "Home", VK_HOME) \
	keyEnumFunc("Left", "Arrow Left", VK_LEFT) \
	keyEnumFunc("Up", "Arrow Up", VK_UP) \
	keyEnumFunc("Right", "Arrow Right", VK_RIGHT) \
	keyEnumFunc("Down", "Arrow Down", VK_DOWN) \
	keyEnumFunc("PrintScreen", "PrintScreen", VK_SNAPSHOT) \
	keyEnumFunc("Insert", "Insert", VK_INSERT) \
	keyEnumFunc("Delete", "Delete", VK_DELETE) \
	keyEnumFunc("Num0", "Num0", VK_NUMPAD0) \
	keyEnumFunc("Num1", "Num1", VK_NUMPAD1) \
	keyEnumFunc("Num2", "Num2", VK_NUMPAD2) \
	keyEnumFunc("Num3", "Num3", VK_NUMPAD3) \
	keyEnumFunc("Num4", "Num4", VK_NUMPAD4) \
	keyEnumFunc("Num5", "Num5", VK_NUMPAD5) \
	keyEnumFunc("Num6", "Num6", VK_NUMPAD6) \
	keyEnumFunc("Num7", "Num7", VK_NUMPAD7) \
	keyEnumFunc("Num8", "Num8", VK_NUMPAD8) \
	keyEnumFunc("Num9", "Num9", VK_NUMPAD9) \
	keyEnumFunc("NumMultiply", "Num*", VK_MULTIPLY) \
	keyEnumFunc("NumAdd", "Num+", VK_ADD) \
	keyEnumFunc("NumSubtract", "Num-", VK_SUBTRACT) \
	keyEnumFunc("NumDecimal", "Num.", VK_DECIMAL) \
	keyEnumFunc("NumDivide", "Num/", VK_DIVIDE) \
	keyEnumFunc("F1", "F1", VK_F1) \
	keyEnumFunc("F2", "F2", VK_F2) \
	keyEnumFunc("F3", "F3", VK_F3) \
	keyEnumFunc("F4", "F4", VK_F4) \
	keyEnumFunc("F5", "F5", VK_F5) \
	keyEnumFunc("F6", "F6", VK_F6) \
	keyEnumFunc("F7", "F7", VK_F7) \
	keyEnumFunc("F8", "F8", VK_F8) \
	keyEnumFunc("F9", "F9", VK_F9) \
	keyEnumFunc("F10", "F10", VK_F10) \
	keyEnumFunc("F11", "F11", VK_F11) \
	keyEnumFunc("F12", "F12", VK_F12) \
	keyEnumFunc("NumLock", "NumLock", VK_NUMLOCK) \
	keyEnumFunc("ScrollLock", "ScrollLock", VK_SCROLL) \
	keyEnumFunc("Colon", ":", VK_OEM_1) \
	keyEnumFunc("Plus", "+", VK_OEM_PLUS) \
	keyEnumFunc("Minus", "-", VK_OEM_MINUS) \
	keyEnumFunc("Comma", ",", VK_OEM_COMMA) \
	keyEnumFunc("Period", ".", VK_OEM_PERIOD) \
	keyEnumFunc("Slash", "/", VK_OEM_2) \
	keyEnumFunc("Tilde", "~", VK_OEM_3) \
	keyEnumFunc("OpenSquareBracket", "[", VK_OEM_4) \
	keyEnumFunc("Backslash", "\\", VK_OEM_5) \
	keyEnumFunc("CloseSquareBracket", "]", VK_OEM_6) \
	keyEnumFunc("Quote", "\"", VK_OEM_7) \
	keyEnumFunc("Backslash2", "\\ (2)", VK_OEM_102) \
	keyEnumFunc_keyRange("0123456789") \
	keyEnumFunc_keyRange("ABCDEFGHIJKLMNOPQRSTUVWXYZ") \
	keyEnumFunc("Shift", "Shift", VK_SHIFT) \
	keyEnumFunc("Ctrl", "Ctrl", VK_CONTROL) \
	keyEnumFunc("Alt", "Alt", VK_MENU) \
	keyEnumFunc("JoystickBtn1", "JoystickBtn1", JOY_BTN_0) \
	keyEnumFunc("JoystickBtn2", "JoystickBtn2", JOY_BTN_1) \
	keyEnumFunc("JoystickBtn3", "JoystickBtn3", JOY_BTN_2) \
	keyEnumFunc("JoystickBtn4", "JoystickBtn4", JOY_BTN_3) \
	keyEnumFunc("JoystickLeftTrigger", "JoystickLeftTrigger", JOY_BTN_4) \
	keyEnumFunc("JoystickRightTrigger", "JoystickRightTrigger", JOY_BTN_5) \
	keyEnumFunc("JoystickLeftTrigger2", "JoystickLeftTrigger2", JOY_BTN_6) \
	keyEnumFunc("JoystickRightTrigger2", "JoystickRightTrigger2", JOY_BTN_7) \
	keyEnumFunc("JoystickBtn9", "JoystickBtn9", JOY_BTN_8) \
	keyEnumFunc("JoystickBtn10", "JoystickBtn10", JOY_BTN_9) \
	keyEnumFunc("JoystickBtn11", "JoystickBtn11", JOY_BTN_10) \
	keyEnumFunc("JoystickBtn12", "JoystickBtn12", JOY_BTN_11) \
	keyEnumFunc("JoystickBtn13", "JoystickBtn13", JOY_BTN_12) \
	keyEnumFunc("JoystickBtn14", "JoystickBtn14", JOY_BTN_13) \
	keyEnumFunc("JoystickBtn15", "JoystickBtn15", JOY_BTN_14) \
	keyEnumFunc("JoystickBtn16", "JoystickBtn16", JOY_BTN_15) \
	keyEnumFunc("LeftStickLeft", "LeftStickLeft", JOY_LEFT_STICK_LEFT) \
	keyEnumFunc("LeftStickUp", "LeftStickUp", JOY_LEFT_STICK_UP) \
	keyEnumFunc("LeftStickRight", "LeftStickRight", JOY_LEFT_STICK_RIGHT) \
	keyEnumFunc("LeftStickDown", "LeftStickDown", JOY_LEFT_STICK_DOWN) \
	keyEnumFunc("DPadLeft", "DPadLeft", JOY_DPAD_LEFT) \
	keyEnumFunc("DPadUp", "DPadUp", JOY_DPAD_UP) \
	keyEnumFunc("DPadRight", "DPadRight", JOY_DPAD_RIGHT) \
	keyEnumFunc("DPadDown", "DPadDown", JOY_DPAD_DOWN) \
	keyEnumFunc("PS4DualshockRightStickLeft", "PS4DualshockRightStickLeft", JOY_PS4_DUALSHOCK_RIGHT_STICK_LEFT) \
	keyEnumFunc("PS4DualshockRightStickUp", "PS4DualshockRightStickUp", JOY_PS4_DUALSHOCK_RIGHT_STICK_UP) \
	keyEnumFunc("PS4DualshockRightStickRight", "PS4DualshockRightStickRight", JOY_PS4_DUALSHOCK_RIGHT_STICK_RIGHT) \
	keyEnumFunc("PS4DualshockRightStickDown", "PS4DualshockRightStickDown", JOY_PS4_DUALSHOCK_RIGHT_STICK_DOWN) \
	keyEnumFunc("XboxTypeSRightStickLeft", "XboxTypeSRightStickLeft", JOY_XBOX_TYPE_S_RIGHT_STICK_LEFT) \
	keyEnumFunc("XboxTypeSRightStickUp", "XboxTypeSRightStickUp", JOY_XBOX_TYPE_S_RIGHT_STICK_UP) \
	keyEnumFunc("XboxTypeSRightStickRight", "XboxTypeSRightStickRight", JOY_XBOX_TYPE_S_RIGHT_STICK_RIGHT) \
	keyEnumFuncLast("XboxTypeSRightStickDown", "XboxTypeSRightStickDown", JOY_XBOX_TYPE_S_RIGHT_STICK_DOWN)