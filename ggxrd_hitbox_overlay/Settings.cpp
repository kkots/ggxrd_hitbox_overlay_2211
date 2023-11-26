#include "pch.h"
#include "Settings.h"
#include "logging.h"
#include "Keyboard.h"
#include "GifMode.h"
#include "Detouring.h"
#include "WinError.h"

Settings settings;

bool Settings::onDllMain() {
	addKey("Backspace", VK_BACK);
	addKey("Tab", VK_TAB);
	addKey("Enter", VK_RETURN);
	addKey("PauseBreak", VK_PAUSE);
	addKey("CapsLock", VK_CAPITAL);
	addKey("Escape", VK_ESCAPE);
	addKey("Space", VK_SPACE);
	addKey("PageUp", VK_PRIOR);
	addKey("PageDown", VK_NEXT);
	addKey("End", VK_END);
	addKey("Home", VK_HOME);
	addKey("Left", VK_LEFT);
	addKey("Up", VK_UP);
	addKey("Right", VK_RIGHT);
	addKey("Down", VK_DOWN);
	addKey("PrintScreen", VK_SNAPSHOT);
	addKey("Insert", VK_INSERT);
	addKey("Delete", VK_DELETE);
	addKey("Num0", VK_NUMPAD0);
	addKey("Num1", VK_NUMPAD1);
	addKey("Num2", VK_NUMPAD2);
	addKey("Num3", VK_NUMPAD3);
	addKey("Num4", VK_NUMPAD4);
	addKey("Num5", VK_NUMPAD5);
	addKey("Num6", VK_NUMPAD6);
	addKey("Num7", VK_NUMPAD7);
	addKey("Num8", VK_NUMPAD8);
	addKey("Num9", VK_NUMPAD9);
	addKey("NumMultiply", VK_MULTIPLY);
	addKey("NumAdd", VK_ADD);
	addKey("NumSubtract", VK_SUBTRACT);
	addKey("NumDecimal", VK_DECIMAL);
	addKey("NumDivide", VK_DIVIDE);
	addKey("F1", VK_F1);
	addKey("F2", VK_F2);
	addKey("F3", VK_F3);
	addKey("F4", VK_F4);
	addKey("F5", VK_F5);
	addKey("F6", VK_F6);
	addKey("F7", VK_F7);
	addKey("F8", VK_F8);
	addKey("F9", VK_F9);
	addKey("F10", VK_F10);
	addKey("F11", VK_F11);
	addKey("F12", VK_F12);
	addKey("NumLock", VK_NUMLOCK);
	addKey("ScrollLock", VK_SCROLL);
	addKey("Colon", VK_OEM_1);
	addKey("Plus", VK_OEM_PLUS);
	addKey("Minus", VK_OEM_MINUS);
	addKey("Comma", VK_OEM_COMMA);
	addKey("Period", VK_OEM_PERIOD);
	addKey("Slash", VK_OEM_2);
	addKey("Tilde", VK_OEM_3);
	addKey("OpenSquareBracket", VK_OEM_4);
	addKey("Backslash", VK_OEM_5);
	addKey("CloseSquareBracket", VK_OEM_6);
	addKey("Quote", VK_OEM_7);
	addKey("Backslash2", VK_OEM_102);

	addKeyRange('0', '9');
	addKeyRange('A', 'Z');

	addKey("Shift", VK_SHIFT);
	addKey("Ctrl", VK_CONTROL);
	addKey("Alt", VK_MENU);


	std::wstring currentDir = getCurrentDirectory();
	settingsPath = currentDir + L"\\ggxrd_hitbox_overlay.ini";
	logwrap(fprintf(logfile, "INI file path: %ls\n", settingsPath.c_str()));


	directoryChangeHandle = FindFirstChangeNotificationW(
		currentDir.c_str(), // directory to watch 
		FALSE,              // do not watch subtree 
		FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE); // watch file name changes and last write date changes
	if (directoryChangeHandle == INVALID_HANDLE_VALUE || !directoryChangeHandle) {
		WinError winErr;
		logwrap(fprintf(logfile, "FindFirstChangeNotificationW failed: %s\n", winErr.getMessage()));
		directoryChangeHandle = NULL;
	}

	readSettings();

	return true;
}

void Settings::onDllDetach() {
	if (directoryChangeHandle) {
		FindCloseChangeNotification(directoryChangeHandle);
	}
}

void Settings::readSettingsIfChanged() {
	if (!directoryChangeHandle) return;
	DWORD dwWaitStatus;
	if (lastCallFailedToGetTime) {
		dwWaitStatus = WAIT_OBJECT_0;
	} else {
		dwWaitStatus = WaitForSingleObject(directoryChangeHandle, 0);
	}
	if (dwWaitStatus == WAIT_OBJECT_0) {
		FILETIME newTime;
		if (!getLastWriteTime(settingsPath, &newTime)) {
			lastCallFailedToGetTime = true;
			return;
		}
		lastCallFailedToGetTime = false;
		if (newTime.dwLowDateTime != lastSettingsWriteTime.dwLowDateTime
			|| newTime.dwHighDateTime != lastSettingsWriteTime.dwHighDateTime) {
			readSettings();
		}
		if (!FindNextChangeNotification(directoryChangeHandle)) {
			WinError winErr;
			logwrap(fprintf(logfile, "FindNextChangeNotification failed: %s\n", winErr.getMessage()));
			FindCloseChangeNotification(directoryChangeHandle);
			directoryChangeHandle = NULL;
			return;
		}
	}
}

void Settings::addKey(const char* name, int code) {
	keys.insert({toUppercase(name), {name, code}});
}

void Settings::addKeyRange(char start, char end) {
	for (char c = start; c <= end; ++c) {
		char newStr[2] = " ";
		newStr[0] = c;
		addKey(newStr, c);
	}
}

void Settings::insertKeyComboToParse(std::map<std::string, KeyComboToParse>& keyCombosToParse, const char* name, std::vector<int>* keyCombo, const char* defaultValue) {
	keyCombosToParse.insert({ toUppercase(name), { name, keyCombo, defaultValue } });
}

// INI file must be placed next the the game's executable at SteamLibrary\steamapps\common\GUILTY GEAR Xrd -REVELATOR-\Binaries\Win32\ggxrd_hitbox_overlay.ini
// Example INI file content:
// gifModeToggle = Ctrl+F3
void Settings::readSettings() {
	
	std::map<std::string, KeyComboToParse> keyCombosToParse;
	insertKeyComboToParse(keyCombosToParse, "gifModeToggle", &gifModeToggle, "F1");
	insertKeyComboToParse(keyCombosToParse, "noGravityToggle", &noGravityToggle, "F2");
	insertKeyComboToParse(keyCombosToParse, "freezeGameToggle", &freezeGameToggle, "F3");
	insertKeyComboToParse(keyCombosToParse, "slowmoGameToggle", &slowmoGameToggle, "F4");
	insertKeyComboToParse(keyCombosToParse, "allowNextFrameKeyCombo", &allowNextFrameKeyCombo, "F5");
	insertKeyComboToParse(keyCombosToParse, "disableModToggle", &disableModKeyCombo, "F6");
	insertKeyComboToParse(keyCombosToParse, "disableHitboxDisplayToggle", &disableHitboxDisplayToggle, "F7");
	insertKeyComboToParse(keyCombosToParse, "screenshotBtn", &screenshotBtn, "F8");
	insertKeyComboToParse(keyCombosToParse, "continuousScreenshotToggle", &continuousScreenshotToggle, "");
	insertKeyComboToParse(keyCombosToParse, "gifModeToggleBackgroundOnly", &gifModeToggleBackgroundOnly, "");
	insertKeyComboToParse(keyCombosToParse, "gifModeToggleCameraCenterOnly", &gifModeToggleCameraCenterOnly, "");
	insertKeyComboToParse(keyCombosToParse, "gifModeToggleHideOpponentOnly", &gifModeToggleHideOpponentOnly, "");
	

	for (auto it = keyCombosToParse.begin(); it != keyCombosToParse.end(); ++it) {
		std::unique_lock<std::mutex> guard(keyCombosMutex);
		it->second.keyCombo->clear();
	}
	keyboard.removeAllKeyCodes();


	slowmoTimes = 3;
	bool slowmoTimesParsed = false;

	// startDisabled stores its result not here, applies effects right after parsing
	bool startDisabledParsed = false;

	{
		std::unique_lock<std::mutex> guard(screenshotPathMutex);
		screenshotPath.clear();
	}
	bool screenshotPathParsed = false;

	allowContinuousScreenshotting = false;
	bool allowContinuousScreenshottingParsed = false;

	dontUseScreenshotTransparency = false;
	bool dontUseScreenshotTransparencyParsed = false;

	drawPushboxCheckSeparately = true;
	bool drawPushboxCheckSeparatelyParsed = false;

	char errorString[500];
	char buf[128];
	FILE* file = NULL;
	if (_wfopen_s(&file, settingsPath.c_str(), L"rt") || !file) {
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
			std::string keyName = parseKeyName(buf);
			std::string keyNameUpper = toUppercase(keyName);
			std::string keyValue = getKeyValue(buf);
			auto found = keyCombosToParse.find(keyNameUpper);
			if (found != keyCombosToParse.end()) {
				found->second.isParsed = parseKeys(found->second.name.c_str(), keyValue, *found->second.keyCombo);
			}
			if (!slowmoTimesParsed && _stricmp(keyName.c_str(), "slowmoTimes") == 0) {
				slowmoTimesParsed = parseInteger("slowmoTimes", keyValue, slowmoTimes);
			}
			if (!allowContinuousScreenshottingParsed && _stricmp(keyName.c_str(), "allowContinuousScreenshotting") == 0) {
				allowContinuousScreenshottingParsed = parseBoolean("allowContinuousScreenshotting", keyValue, allowContinuousScreenshotting);
			}
			if (firstSettingsParse && !startDisabledParsed && _stricmp(keyName.c_str(), "startDisabled") == 0) {
				std::atomic_bool startDisabled = false;
				startDisabledParsed = parseBoolean("startDisabled", keyValue, startDisabled);
				if (startDisabled) {
					gifMode.modDisabled = true;
				}
			}
			if (!screenshotPathParsed && _stricmp(keyName.c_str(), "screenshotPath") == 0) {
				screenshotPathParsed = true;
				{
					std::unique_lock<std::mutex> guard(screenshotPathMutex);
					screenshotPath = keyValue;  // in UTF-8
				}
				logwrap(fprintf(logfile, "Parsed screenshotPath (UTF8): %s\n", keyValue.c_str()));
			}
			if (!dontUseScreenshotTransparencyParsed && _stricmp(keyName.c_str(), "dontUseScreenshotTransparency") == 0) {
				dontUseScreenshotTransparencyParsed = parseBoolean("dontUseScreenshotTransparency", keyValue, dontUseScreenshotTransparency);
			}
			if (!drawPushboxCheckSeparatelyParsed && _stricmp(keyName.c_str(), "drawPushboxCheckSeparately") == 0) {
				drawPushboxCheckSeparatelyParsed = parseBoolean("drawPushboxCheckSeparately", keyValue, drawPushboxCheckSeparately);
			}
			if (feof(file)) break;
		}
		fclose(file);
	}

	for (auto it = keyCombosToParse.begin(); it != keyCombosToParse.end(); ++it) {
		if (!it->second.isParsed) {
			parseKeys(it->first.c_str(), it->second.defaultValue, *it->second.keyCombo);
		}
		keyboard.addNewKeyCodes(*it->second.keyCombo);
	}

	firstSettingsParse = false;
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

std::string Settings::toUppercase(std::string str) const {
	std::string result;
	result.reserve(str.size());
	for (char c : str) {
		result.push_back(toupper(c));
	}
	return result;
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

bool Settings::parseKeys(const char* keyName, const std::string& keyValue, std::vector<int>& keyCodes) {
	if (!keyValue.empty()) {
		std::string keyValueUppercase = toUppercase(keyValue);
		std::vector<std::string> keyNames = split(keyValueUppercase, '+');
		for (std::string& str : keyNames) {
			trim(str);
			auto found = keys.find(str);
			if (found != keys.end()) {
				keyCodes.push_back(found->second.code);
			} else {
				logwrap(fprintf(logfile, "Key combo parsing error: key not found %s\n", str));
				return false;
			}
		}
		logwrap(fprintf(logfile, "Parsed key codes for %s: %s\n", keyName, keyValue.c_str()));
	} else {
		logwrap(fprintf(logfile, "Parsed that key codes are empty for %s\n", keyName));
	}
	return true;
}

bool Settings::parseInteger(const char* keyName, const std::string& keyValue, std::atomic_int& integer) {
	for (auto it = keyValue.begin(); it != keyValue.end(); ++it) {
		if (!(*it >= '0' && *it <= '9')) return false;  // apparently atoi doesn't do this check
	}
	int result = std::atoi(keyValue.c_str());
	if (result == 0 && keyValue != "0") return false;
	integer = result;
	logwrap(fprintf(logfile, "Parsed integer for %s: %d\n", keyName, integer.load()));
	return true;
}

bool Settings::parseBoolean(const char* keyName, const std::string& keyValue, std::atomic_bool& aBooleanValue) {
	if (_stricmp(keyValue.c_str(), "true") == 0) {
		logwrap(fprintf(logfile, "Parsed boolean for %s: %d\n", keyName, 1));
		aBooleanValue = true;
		return true;
	}
	if (_stricmp(keyValue.c_str(), "false") == 0) {
		logwrap(fprintf(logfile, "Parsed boolean for %s: %d\n", keyName, 0));
		aBooleanValue = false;
		return true;
	}
	return false;
}

int Settings::findMinCommentPos(const char* buf) const {
	int colonPos = findChar(buf, ';');
	int hashtagPos = findChar(buf, '#');
	int minCommentPos = -1;
	if (colonPos != -1) minCommentPos = colonPos;
	if (minCommentPos == -1 || hashtagPos != -1 && minCommentPos != -1 && hashtagPos < minCommentPos) minCommentPos = hashtagPos;
	return minCommentPos;
}

std::string Settings::parseKeyName(const char* buf) const {

	int minCommentPos = findMinCommentPos(buf);

	int equalSignPos = findChar(buf, '=');

	if (equalSignPos == -1 || minCommentPos != -1 && equalSignPos != -1 && equalSignPos > minCommentPos) return std::string{};

	std::string keyNameStr(buf, equalSignPos);
	trim(keyNameStr);

	return keyNameStr;
}

std::string Settings::getKeyValue(const char* buf) const {
	int minCommentPos = findMinCommentPos(buf);
	int equalSignPos = findChar(buf, '=');

	if (equalSignPos == -1 || minCommentPos != -1 && equalSignPos != -1 && equalSignPos > minCommentPos) return std::string{};

	const char* bufPos = buf + equalSignPos + 1;
	size_t bufLength = strlen(buf);
	if (minCommentPos != -1) bufLength -= (bufLength - minCommentPos);
	int lengthFromBufPos = buf + bufLength - bufPos;
	if (lengthFromBufPos == 0) return std::string{};
	std::string keyValue(bufPos, lengthFromBufPos);
	trim(keyValue);

	return keyValue;
}

std::wstring Settings::getCurrentDirectory() {
	DWORD requiredSize = GetCurrentDirectoryW(0, NULL);
	if (!requiredSize) {
		WinError winErr;
		logwrap(fprintf(logfile, "GetCurrentDirectoryW failed: %s\n", winErr.getMessage()));
		return std::wstring{};
	}
	std::wstring currentDir;
	currentDir.resize(requiredSize - 1);
	if (!GetCurrentDirectoryW(currentDir.size() + 1, &currentDir.front())) {
		WinError winErr;
		logwrap(fprintf(logfile, "GetCurrentDirectoryW (second call) failed: %s\n", winErr.getMessage()));
		return std::wstring{};
	}
	return currentDir;
}

bool Settings::getLastWriteTime(const std::wstring& path, FILETIME* fileTime) {
	HANDLE hFile = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (!hFile || hFile == INVALID_HANDLE_VALUE) {
		WinError winErr;
		logwrap(fprintf(logfile, "CreateFileW failed: %s. %.8x\n", winErr.getMessage(), winErr.code));;
		return false;
	}
	FILETIME creationTime{ 0 };
	FILETIME lastAccessTime{ 0 };
	FILETIME lastWriteTime{ 0 };
	if (!GetFileTime(hFile, &creationTime, &lastAccessTime, &lastWriteTime)) {
		WinError winErr;
		logwrap(fprintf(logfile, "GetFileTime failed: %s\n", winErr.getMessage()));
		CloseHandle(hFile);
		return false;
	}
	CloseHandle(hFile);
	*fileTime = lastWriteTime;
	return true;
}
