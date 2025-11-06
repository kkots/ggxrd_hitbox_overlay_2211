#include "pch.h"
#include "SettingsTopCommentDefinition.h"
#include "KeyDefinitions.h"
// this file gets parsed by regenerate_ini_and_update_readme.ps1. Do not change its structure
#define keyEnumFunc(identifier, userFriendlyName, virtualKeyCode, movable) identifier ", "
#define keyEnumFuncLast(identifier, userFriendlyName, virtualKeyCode, movable) identifier
#define keyEnumFunc_keyRange(str) str ", "
const char* settingsTopComment = "; Place this file into the game folder containing 'GuiltyGearXrd.exe' so that it gets seen by the mod. Allowed key names: "
		keyEnum
		".\n"
		"\n"
		"; Key combinations can be specified by separating key names with '+' sign.\n"
		"; The '-' sign means that that key must not be held.\n"
		"; You can assign same key to multiple features - it will toggle/set in motion all of them simultaneously.\n"
		"; You don't need to reload the mod when you change this file - it re-reads this settings file automatically when it changes.\n"
		"\n"
		"; All of these settings can be changed using the mod's UI which can be seen in the game if you press ESC (default hotkey, can be configured in modWindowVisibilityToggle).";
#undef keyEnumFunc
#undef keyEnumFuncLast
#undef keyEnumFunc