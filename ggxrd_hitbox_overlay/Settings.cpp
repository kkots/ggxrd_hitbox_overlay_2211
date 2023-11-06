#include "pch.h"
#include "Settings.h"
#include "logging.h"

Settings settings;

bool Settings::onDllMain() {
	keys.push_back({ "Backspace", VK_BACK });
	keys.push_back({ "Tab", VK_TAB });
	keys.push_back({ "Enter", VK_RETURN });
	keys.push_back({ "PauseBreak", VK_PAUSE });
	keys.push_back({ "CapsLock", VK_CAPITAL });
	keys.push_back({ "Escape", VK_ESCAPE });
	keys.push_back({ "Space", VK_SPACE });
	keys.push_back({ "PageUp", VK_PRIOR });
	keys.push_back({ "PageDown", VK_NEXT });
	keys.push_back({ "End", VK_END });
	keys.push_back({ "Home", VK_HOME });
	keys.push_back({ "Left", VK_LEFT });
	keys.push_back({ "Up", VK_UP });
	keys.push_back({ "Right", VK_RIGHT });
	keys.push_back({ "Down", VK_DOWN });
	keys.push_back({ "PrintScreen", VK_SNAPSHOT });
	keys.push_back({ "Insert", VK_INSERT });
	keys.push_back({ "Delete", VK_DELETE });
	keys.push_back({ "Num0", VK_NUMPAD0 });
	keys.push_back({ "Num1", VK_NUMPAD1 });
	keys.push_back({ "Num2", VK_NUMPAD2 });
	keys.push_back({ "Num3", VK_NUMPAD3 });
	keys.push_back({ "Num4", VK_NUMPAD4 });
	keys.push_back({ "Num5", VK_NUMPAD5 });
	keys.push_back({ "Num6", VK_NUMPAD6 });
	keys.push_back({ "Num7", VK_NUMPAD7 });
	keys.push_back({ "Num8", VK_NUMPAD8 });
	keys.push_back({ "Num9", VK_NUMPAD9 });
	keys.push_back({ "NumMultiply", VK_MULTIPLY });
	keys.push_back({ "NumAdd", VK_ADD });
	keys.push_back({ "NumSubtract", VK_SUBTRACT });
	keys.push_back({ "NumDecimal", VK_DECIMAL });
	keys.push_back({ "NumDivide", VK_DIVIDE });
	keys.push_back({ "F1", VK_F1 });
	keys.push_back({ "F2", VK_F2 });
	keys.push_back({ "F3", VK_F3 });
	keys.push_back({ "F4", VK_F4 });
	keys.push_back({ "F5", VK_F5 });
	keys.push_back({ "F6", VK_F6 });
	keys.push_back({ "F7", VK_F7 });
	keys.push_back({ "F8", VK_F8 });
	keys.push_back({ "F9", VK_F9 });
	keys.push_back({ "F10", VK_F10 });
	keys.push_back({ "F11", VK_F11 });
	keys.push_back({ "F12", VK_F12 });
	keys.push_back({ "NumLock", VK_NUMLOCK });
	keys.push_back({ "ScrollLock", VK_SCROLL });
	keys.push_back({ "Colon", VK_OEM_1 });
	keys.push_back({ "Plus", VK_OEM_PLUS });
	keys.push_back({ "Minus", VK_OEM_MINUS });
	keys.push_back({ "Comma", VK_OEM_COMMA });
	keys.push_back({ "Period", VK_OEM_PERIOD });
	keys.push_back({ "Slash", VK_OEM_2 });
	keys.push_back({ "Tilde", VK_OEM_3 });
	keys.push_back({ "OpenSquareBracket", VK_OEM_4 });
	keys.push_back({ "Backslash", VK_OEM_5 });
	keys.push_back({ "CloseSquareBracket", VK_OEM_6 });
	keys.push_back({ "Quote", VK_OEM_7 });
	keys.push_back({ "Backslash2", VK_OEM_102 });

	addKeyRange('0', '9');
	addKeyRange('A', 'Z');

	keys.push_back({ "Shift", VK_SHIFT });
	keys.push_back({ "Ctrl", VK_CONTROL });
	keys.push_back({ "Alt", VK_MENU });

	readSettings();

	return true;
}

void Settings::addKeyRange(char start, char end) {
	extraNames.emplace_back();
	std::vector<std::string>& newVec = extraNames.back();
	newVec.reserve(end - start + 1);
	for (char c = start; c <= end; ++c) {
		std::string newName;
		newName.push_back(c);
		newVec.push_back(newName);
		const std::string& newNameRelocated = newVec.back();
		keys.push_back({ newNameRelocated.c_str(), c });
	}
}

// INI file must be placed next the the game's executable at SteamLibrary\steamapps\common\GUILTY GEAR Xrd -REVELATOR-\Binaries\Win32\ggxrd_hitbox_overlay.ini
// Example INI file content:
// gifModeToggle = Ctrl+F3
void Settings::readSettings() {

	bool gifModeToggleParsed = false;
	bool noGravityToggleParsed = false;

	char errorString[500];
	char buf[128];
	wchar_t currentPath[MAX_PATH];
	GetCurrentDirectoryW(_countof(currentPath), currentPath);
	wcscat_s(currentPath, L"\\ggxrd_hitbox_overlay.ini");
	logwrap(fprintf(logfile, "INI file path: %ls\n", currentPath));
	FILE* file = NULL;
	if (_wfopen_s(&file, currentPath, L"rt") || !file) {
		strerror_s(errorString, errno);
		logwrap(fprintf(logfile, "Could not open INI file: %s\n", errorString));
	} else {
		while (true) {
			if (!fgets(buf, 127, file)) {
				if (ferror(file)) {
					strerror_s(errorString, errno);
					logwrap(fprintf(logfile, "Error reading INI file: %s\n", errorString));
					break;
				}
				break;
			}
			if (!gifModeToggleParsed) {
				gifModeToggleParsed = parseKeys("gifModeToggle", buf, gifModeToggle);
			}
			if (!noGravityToggleParsed) {
				noGravityToggleParsed = parseKeys("noGravityToggle", buf, noGravityToggle);
			}
			if (feof(file)) break;
		}
		fclose(file);
	}

	if (!gifModeToggleParsed) {
		logwrap(fputs("gifModeToggle not parsed, using default value\n", logfile));
		parseKeys("gifModeToggle", "gifModeToggle = F1", gifModeToggle);
	}
	if (!noGravityToggleParsed) {
		logwrap(fputs("noGravityToggle not parsed, using default value\n", logfile));
		parseKeys("noGravityToggle", "noGravityToggle = F2", noGravityToggle);
	}
}

int Settings::findChar(const char* buf, char c) const {
	const char* ptr = buf;
	while (*ptr != '\0') {
		if (*ptr == c) return ptr - buf;
		++ptr;
	}
	return -1;
}

void Settings::trim(std::string& str) const {
	if (str.empty()) return;
	const char* strStart = &str.front();
	const char* c = strStart;
	while (*c <= 32 && *c != '\0') {
		++c;
	}
	if (*c == '\0') {
		str.clear();
		return;
	}

	const char* cEnd = strStart + str.size() - 1;
	while (cEnd >= c && *cEnd <= 32) {
		--cEnd;
	}
	if (cEnd < c) {
		str.clear();
		return;
	}

	str = std::string(c, cEnd - c + 1);
}

std::vector<std::string> Settings::split(const std::string& str, char c) const {
	std::vector<std::string> result;
	const char* strStart = &str.front();
	const char* strEnd = strStart + str.size();
	const char* prevPtr = strStart;
	const char* ptr = strStart;
	while (*ptr != '\0') {
		if (*ptr == c) {
			if (ptr > prevPtr) {
				result.emplace_back(prevPtr, ptr - prevPtr);
			} else if (ptr == prevPtr) {
				result.emplace_back();
			}
			prevPtr = ptr + 1;
		}
		++ptr;
	}
	if (prevPtr < strEnd) {
		result.emplace_back(prevPtr, strEnd - prevPtr);
	}
	return result;
}

bool Settings::parseKeys(const char* keyName, const char* buf, std::vector<int>& keyCodes) {
	int equalSignPos = findChar(buf, '=');

	if (equalSignPos == -1) return false;

	std::string keyNameStr(buf, equalSignPos);
	trim(keyNameStr);

	if (keyNameStr != keyName) return false;

	const char* bufPos = buf + equalSignPos + 1;
	size_t bufLength = strlen(buf);
	std::string keyValue(bufPos, buf + bufLength - bufPos);
	trim(keyValue);

	std::vector<std::string> keyNames = split(keyValue, '+');
	for (std::string& str : keyNames) {
		trim(str);
		for (Key& key : keys) {
			if (_stricmp(str.c_str(), key.name) == 0) {
				keyCodes.push_back(key.code);
			}
		}
	}
	if (!keyCodes.empty()) {
		logwrap(fprintf(logfile, "Parsed key codes for %s: %s\n", keyName, buf));
		return true;
	}
	return false;
}
