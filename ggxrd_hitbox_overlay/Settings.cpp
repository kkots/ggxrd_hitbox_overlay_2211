#include "pch.h"
#include "Settings.h"
#include "logging.h"
#include "Keyboard.h"
#include "GifMode.h"
#include "Detouring.h"
#include "WinError.h"
#include <list>
#include "UI.h"
#include "CustomWindowMessages.h"
#include <unordered_map>
#include <functional>

Settings settings;

bool Settings::onDllMain() {
	addKey("Backspace", "Backspace", VK_BACK);
	addKey("Tab", "Tab", VK_TAB);
	addKey("Enter", "Enter", VK_RETURN);
	addKey("PauseBreak", "PauseBreak", VK_PAUSE);
	addKey("CapsLock", "CapsLock", VK_CAPITAL);
	addKey("Escape", "Escape", VK_ESCAPE);
	addKey("Space", "Space", VK_SPACE);
	addKey("PageUp", "PageUp", VK_PRIOR);
	addKey("PageDown", "PadeDown", VK_NEXT);
	addKey("End", "End", VK_END);
	addKey("Home", "Home", VK_HOME);
	addKey("Left", "Arrow Left", VK_LEFT);
	addKey("Up", "Arrow Up", VK_UP);
	addKey("Right", "Arrow Right", VK_RIGHT);
	addKey("Down", "Arrow Down", VK_DOWN);
	addKey("PrintScreen", "PrintScreen", VK_SNAPSHOT);
	addKey("Insert", "Insert", VK_INSERT);
	addKey("Delete", "Delete", VK_DELETE);
	addKey("Num0", "Num0", VK_NUMPAD0);
	addKey("Num1", "Num1", VK_NUMPAD1);
	addKey("Num2", "Num2", VK_NUMPAD2);
	addKey("Num3", "Num3", VK_NUMPAD3);
	addKey("Num4", "Num4", VK_NUMPAD4);
	addKey("Num5", "Num5", VK_NUMPAD5);
	addKey("Num6", "Num6", VK_NUMPAD6);
	addKey("Num7", "Num7", VK_NUMPAD7);
	addKey("Num8", "Num8", VK_NUMPAD8);
	addKey("Num9", "Num9", VK_NUMPAD9);
	addKey("NumMultiply", "Num*", VK_MULTIPLY);
	addKey("NumAdd", "Num+", VK_ADD);
	addKey("NumSubtract", "Num-", VK_SUBTRACT);
	addKey("NumDecimal", "Num.", VK_DECIMAL);
	addKey("NumDivide", "Num/", VK_DIVIDE);
	addKey("F1", "F1", VK_F1);
	addKey("F2", "F2", VK_F2);
	addKey("F3", "F3", VK_F3);
	addKey("F4", "F4", VK_F4);
	addKey("F5", "F5", VK_F5);
	addKey("F6", "F6", VK_F6);
	addKey("F7", "F7", VK_F7);
	addKey("F8", "F8", VK_F8);
	addKey("F9", "F9", VK_F9);
	addKey("F10", "F10", VK_F10);
	addKey("F11", "F11", VK_F11);
	addKey("F12", "F12", VK_F12);
	addKey("NumLock", "NumLock", VK_NUMLOCK);
	addKey("ScrollLock", "ScrollLock", VK_SCROLL);
	addKey("Colon", ":", VK_OEM_1);
	addKey("Plus", "+", VK_OEM_PLUS);
	addKey("Minus", "-", VK_OEM_MINUS);
	addKey("Comma", ",", VK_OEM_COMMA);
	addKey("Period", ".", VK_OEM_PERIOD);
	addKey("Slash", "/", VK_OEM_2);
	addKey("Tilde", "~", VK_OEM_3);
	addKey("OpenSquareBracket", "[", VK_OEM_4);
	addKey("Backslash", "\\", VK_OEM_5);
	addKey("CloseSquareBracket", "]", VK_OEM_6);
	addKey("Quote", "\"", VK_OEM_7);
	addKey("Backslash2", "\\ (2)", VK_OEM_102);

	addKeyRange('0', '9');
	addKeyRange('A', 'Z');

	addKey("Shift", "Shift", VK_SHIFT);
	addKey("Ctrl", "Ctrl", VK_CONTROL);
	addKey("Alt", "Alt", VK_MENU);
	
	for (auto it = keys.begin(); it != keys.end(); ++it) {
		reverseKeys.insert({it->second.code, &it->second});
	}
	
	insertKeyComboToParse("gifModeToggle", "GIF Mode Toggle", &gifModeToggle, "F1",
		"; A keyboard shortcut to toggle GIF mode.\n"
		"; GIF mode is:\n"
		"; 1) Background becomes black\n"
		"; 2) Camera is centered on you\n"
		"; 3) Opponent is invisible and invulnerable\n"
		"; 4) Hide HUD");
	insertKeyComboToParse("noGravityToggle", "No Gravity Toggle", &noGravityToggle, "F2",
		"; A keyboard shortcut to toggle No gravity mode\n"
		"; No gravity mode is you can't fall basically");
	insertKeyComboToParse("freezeGameToggle", "Freeze Game Toggle", &freezeGameToggle, "F3",
		"; A keyboard shortcut to freeze the game");
	insertKeyComboToParse("slowmoGameToggle", "Slow-mo Game Toggle", &slowmoGameToggle, "F4",
		"; A keyboard shortcut to play the game in slow motion.\n"
		"; Please specify by how many times to slow the game down in \"slowmoTimes\"");
	insertKeyComboToParse("allowNextFrameKeyCombo", "Allow Next Frame", &allowNextFrameKeyCombo, "F5",
		"; A keyboard shortcut. Only works while the game is frozen using freezeGameToggle.\n"
		"; Advances the game forward one frame");
	insertKeyComboToParse("disableModToggle", "Disable Mod Toggle", &disableModKeyCombo, "F6",
		"; A keyboard shortcut to enable/disable the mod without having to load/unload it");
	insertKeyComboToParse("disableHitboxDisplayToggle", "Disable Hitbox Display Toggle", &disableHitboxDisplayToggle, "F7",
		"; A keyboard shortcut to enable/disable only the mod hitbox drawing feature:\n"
		"; the GIF mode and no gravity, etc will keep working");
	insertKeyComboToParse("screenshotBtn", "Take Screenshot", &screenshotBtn, "F8", "; A keyboard shortcut.\n"
		"; Takes a screenshot and saves it at screenshotPath path\n"
		"; To take screenshots over a transparent background you need to go to the game's\n"
		"; Display Settings and turn off Post-Effects, then use GIF mode (make background dark).\n"
		"; Then screenshots will film character over transparent background.\n"
		"; If the dontUseScreenshotTransparency setting is true, screenshot will be without\n"
		"; transparency anyway");
	insertKeyComboToParse("continuousScreenshotToggle", "Continuous Screenshot Toggle", &continuousScreenshotToggle, "",
		"; A keyboard shortcut.\n"
		"; This toggle can be used same way as screenshotBtn (when it's combined with\n"
		"; allowContinuousScreenshotting = true), except it's a separate key combination and when you\n"
		"; press it, it toggles the continuous screenshot taking every game logical frame. This\n"
		"; toggle does not require allowContinuousScreenshotting to be set to true,\n"
		"; or screenshotBtn to be set to anything.");
	insertKeyComboToParse("gifModeToggleBackgroundOnly", "GIF Mode Toggle (Background Only)", &gifModeToggleBackgroundOnly, "",
		"; A keyboard shortcut to only toggle the \"background becomes black\" part of the gifModeToggle.\n"
		"; Empty by default, which means no hotkey is assigned. Assign your desired hotkey manually here.\n"
		"; This option can be combined with the other \"only\" options, by sharing the same key binding for example");
	insertKeyComboToParse("gifModeToggleCameraCenterOnly", "GIF Mode Toggle (Camera Only)", &gifModeToggleCameraCenterOnly, "",
		"; A keyboard shortcut to only toggle the \"Camera is centered on you\" part of the gifModeToggle.\n"
		"; Empty by default, which means no hotkey is assigned. Assign your desired hotkey manually here.\n"
		"; This option can be combined with the other \"only\" options, by sharing the same key binding for example");
	insertKeyComboToParse("gifModeToggleHideOpponentOnly", "GIF Mode Toggle (Hide Opponent Only)", &gifModeToggleHideOpponentOnly, "",
		"; A keyboard shortcut to only toggle the \"Opponent is invisible and invulnerable\" part of the gifModeToggle.\n"
		"; Empty by default, which means no hotkey is assigned. Assign your desired hotkey manually here.\n"
		"; This option can be combined with the other \"only\" options, by sharing the same key binding for example");
	insertKeyComboToParse("gifModeToggleHudOnly", "GIF Mode Toggle (HUD Only)", &gifModeToggleHudOnly, "",
		"; A keyboard shortcut to only toggle the \"hide hud\" part of the gifModeToggle.\n"
		"; Empty by default, which means no hotkey is assigned. Assign your desired hotkey manually here.\n"
		"; This option can be combined with the other \"only\" options, by sharing the same key binding for example");
	insertKeyComboToParse("modWindowVisibilityToggle", "Hide UI Toggle", &modWindowVisibilityToggle, "",
		"; A keyboard shortcut.\n"
		"; Pressing this shortcut will show/hide the mod's UI window.");
	
	registerOtherDescription(&slowmoTimes, "; A number.\n"
			"; This works in conjunction with slowmoGameToggle. Only round numbers greater than 1 allowed.\n"
			"; Specifies by how many times to slow the game down");
	registerOtherDescription(&allowContinuousScreenshotting, "; Specify true or false.\n"
			"; When this is true that means screenshots are being taken every game loop logical frame as\n"
			"; long as the screenshotBtn is being held. Game loop logical frame means that if the game is\n"
			"; paused or the actual animations are not playing for whatever reason, screenshot won't be taken.\n"
			"; A new screenshot is only taken when animation frames change on the player characters.\n"
			"; Be cautions not to run out of disk space if you're low. This option doesn't\n"
			"; work if screenshotPath is empty, it's not allowed to work outside of training mode or when\n"
			"; a match (training session) isn't currently running (for example on character selection screen).");
	registerOtherDescription(&startDisabled, "; Specify true or false.\n"
			"; When true, starts the mod in a disabled state: it doesn't draw boxes or affect anything");
	registerOtherDescription(&screenshotPath, "; A path to a file or a directory.\n"
			"; It specifies where screenshots will be saved.\n"
			"; If you provided a file path, it must be with .png extension, and if such name already exists, a\n"
			"; number will be appended to it, increasing from 1 to infinity consecutively so that it's unique,\n"
			"; so that new screenshots will never overwrite old ones.\n"
			"; If you provided a directory path, it must already exist, and \"screen.png\" will be appended to\n"
			"; it with an increasing number at the end in case the filename is not unique.\n"
			"; The provided path must be without quotes.\n"
			"; If you want the path to be multilingual you need to save this file in UTF-8.\n"
			"; On Ubuntu/Linux running Guilty Gear Xrd under Steam Proton you need to specify paths with\n"
			"; the Z:\\ drive, path separator is backslash (\\), not forward slash (/). Example: Z:\\home\\yourUserName\\ggscreen.png\n"
			"; If the path is not specified or is empty, the screenshot will be saved into your clipboard so\n"
			"; it can be pasted into any image editing program. For example, GIMP will recognize the PNG\n"
			"; format and paste that, with transparency. This would work even on Ubuntu/Linux.\n"
			"; Only PNG format is supported.");
	registerOtherDescription(&dontUseScreenshotTransparency, "; Specify true or false.\n"
			"; Setting this to true will produce screenshots without transparency");
	registerOtherDescription(&drawPushboxCheckSeparately, "; Specify true or false.\n"
			"; Setting this to true will make throw boxes show in an opponent-character-independent way:\n"
			"; The part of the throw box that checks for pushboxes proximity will be shown in blue,\n"
			"; while the part of the throw box that checks x or y of the origin point will be shown in purple\n"
			"; Setting this to false will combine both the checks of the throw so that you only see the final box\n"
			"; in blue which only checks the opponent's origin point. Be warned, such a throw box\n"
			"; is affected by the width of the opponent's pushbox. Say, on Potemkin, for example,\n"
			"; all ground throw ranges should be higher.");
	registerOtherDescription(&modWindowVisibleOnStart, "; Specify true or false.\n"
			"; If this is false, when this mod starts, the mod's UI window will be invisible.");
	
	std::wstring currentDir = getCurrentDirectory();
	settingsPath = currentDir + L"\\ggxrd_hitbox_overlay.ini";
	logwrap(fprintf(logfile, "INI file path: %ls\n", settingsPath.c_str()));
	
	registerListenerForChanges();

	readSettings(false);

	return true;
}

void Settings::onDllDetach() {
	if (directoryChangeHandle) {
		HANDLE temp = directoryChangeHandle;
		directoryChangeHandle = NULL;
		FindCloseChangeNotification(temp);
	}
	if (changesListener) {
		changesListenerWakeType = WAKE_TYPE_EXIT;
		// We're calling WaitForSingleObject on the changeListenerExitedEvent, instead of on the
		// changesListener, because this call happens in DllMain, which happens under an internal
		// system lock, called the "loader" lock.
		// "The loader lock is taken by any function that needs to access the list of DLLs loaded into the process." - Raymond Chen.
		// When a thread exits, even if you used DisableThreadLibraryCalls(hModule), it calls DllMain with the loader lock, and this
		// happens before its object is signaled.
		// As such, you cannot wait on a thread inside DllMain.
		
		// On normal exit the thread is already killed by something
		bool eventIsSet = false;
		while (true) {
			DWORD exitCode;
			if (GetExitCodeThread(changesListener, &exitCode)) {
				if (exitCode != STILL_ACTIVE) {
					break;
				}
			}
			if (!eventIsSet) {
				eventIsSet = true;
				SetEvent(changesListenerWakeEvent);
			}
			DWORD waitResult = WaitForSingleObject(changeListenerExitedEvent, 100);
			if (waitResult == WAIT_OBJECT_0) break;
		}
		CloseHandle(changeListenerExitedEvent);
		CloseHandle(changesListener);
		CloseHandle(changesListenerWakeEvent);
		changesListener = NULL;
	}
}

void Settings::addKey(const char* name, const char* uiName, int code) {
	keys.insert({toUppercase(name), {name, uiName, code}});
}

void Settings::addKeyRange(char start, char end) {
	static std::list<std::string> mem;
	for (char c = start; c <= end; ++c) {
		mem.emplace_back(2, '\0');
		std::string& newMem = mem.back();
		newMem[0] = c;
		addKey(newMem.c_str(), newMem.c_str(), c);
	}
}

void Settings::insertKeyComboToParse(const char* name, const char* uiName, std::vector<int>* keyCombo, const char* defaultValue, const char* iniDescription) {
	auto newIt = keyCombosToParse.insert({ toUppercase(name), { name, uiName, keyCombo, defaultValue, iniDescription } });
	KeyComboToParse& k = newIt.first->second;
	k.uiDescription = convertToUiDescription(iniDescription);
}

// INI file must be placed next the the game's executable at SteamLibrary\steamapps\common\GUILTY GEAR Xrd -REVELATOR-\Binaries\Win32\ggxrd_hitbox_overlay.ini
// Example INI file content:
// gifModeToggle = Ctrl+F3
void Settings::readSettings(bool dontReadIfDoesntExist) {
	logwrap(fputs("Reading settings\n", logfile));
	
	char errorString[500];
	FILE* file = NULL;
	if (_wfopen_s(&file, settingsPath.c_str(), L"rt") || !file) {
		strerror_s(errorString, errno);
		logwrap(fprintf(logfile, "Could not open INI file: %s\n", errorString));
		file = NULL;
	}
	if (dontReadIfDoesntExist && !file) {
		return;
	}
	std::unique_lock<std::mutex> keyboardGuard(keyboard.mutex);
	Keyboard::MutexLockedFromOutsideGuard keyboardOutsideGuard;
	std::unique_lock<std::mutex> guard(keyCombosMutex);
	std::unique_lock<std::mutex> screenshotGuard(screenshotPathMutex);
	
	for (auto it = keyCombosToParse.begin(); it != keyCombosToParse.end(); ++it) {
		it->second.keyCombo->clear();
	}
	keyboard.removeAllKeyCodes();


	slowmoTimes = 3;
	bool slowmoTimesParsed = false;

	startDisabled = false;
	bool startDisabledParsed = false;

	screenshotPath.clear();
	bool screenshotPathParsed = false;

	allowContinuousScreenshotting = false;
	bool allowContinuousScreenshottingParsed = false;

	dontUseScreenshotTransparency = false;
	bool dontUseScreenshotTransparencyParsed = false;

	drawPushboxCheckSeparately = true;
	bool drawPushboxCheckSeparatelyParsed = false;
	
	modWindowVisibleOnStart = true;
	bool modWindowVisibleOnStartParsed = false;

	std::string accum;
	char buf[128];
	if (file) {
		while (true) {
			accum.clear();
			bool exitOuterLoop = false;
			bool exitOuterLoopIfEmpty = false;
			while (true) {
				if (!fgets(buf, 127, file)) {
					if (ferror(file)) {
						exitOuterLoop = true;
						strerror_s(errorString, errno);
						logwrap(fprintf(logfile, "Error reading INI file: %s\n", errorString));
					}
					exitOuterLoopIfEmpty = true;
					break;
				}
				buf[127] = '\0';
				accum += buf;
				if (buf[strlen(buf) - 1] == '\n') break;
			}
			if (exitOuterLoop) {
				break;
			}
			if (exitOuterLoopIfEmpty && accum.empty()) {
				break;
			}
			std::string keyName = parseKeyName(accum.c_str());
			std::string keyNameUpper = toUppercase(keyName);
			std::string keyValue = getKeyValue(accum.c_str());
			auto found = keyCombosToParse.find(keyNameUpper);
			if (found != keyCombosToParse.end()) {
				found->second.isParsed = parseKeys(found->second.name, keyValue, *found->second.keyCombo);
			}
			if (!slowmoTimesParsed && _stricmp(keyName.c_str(), "slowmoTimes") == 0) {
				slowmoTimesParsed = parseInteger("slowmoTimes", keyValue, slowmoTimes);
			}
			if (!allowContinuousScreenshottingParsed && _stricmp(keyName.c_str(), "allowContinuousScreenshotting") == 0) {
				allowContinuousScreenshottingParsed = parseBoolean("allowContinuousScreenshotting", keyValue, allowContinuousScreenshotting);
			}
			if (firstSettingsParse && !startDisabledParsed && _stricmp(keyName.c_str(), "startDisabled") == 0) {
				startDisabledParsed = parseBoolean("startDisabled", keyValue, startDisabled);
				if (startDisabled) {
					gifMode.modDisabled = true;
				}
			}
			if (!screenshotPathParsed && _stricmp(keyName.c_str(), "screenshotPath") == 0) {
				screenshotPathParsed = true;
				screenshotPath = keyValue;  // in UTF-8
				logwrap(fprintf(logfile, "Parsed screenshotPath (UTF8): %s\n", keyValue.c_str()));
			}
			if (!dontUseScreenshotTransparencyParsed && _stricmp(keyName.c_str(), "dontUseScreenshotTransparency") == 0) {
				dontUseScreenshotTransparencyParsed = parseBoolean("dontUseScreenshotTransparency", keyValue, dontUseScreenshotTransparency);
			}
			if (!drawPushboxCheckSeparatelyParsed && _stricmp(keyName.c_str(), "drawPushboxCheckSeparately") == 0) {
				drawPushboxCheckSeparatelyParsed = parseBoolean("drawPushboxCheckSeparately", keyValue, drawPushboxCheckSeparately);
			}
			if (!modWindowVisibleOnStartParsed && _stricmp(keyName.c_str(), "modWindowVisibleOnStart") == 0) {
				modWindowVisibleOnStartParsed = parseBoolean("modWindowVisibleOnStart", keyValue, modWindowVisibleOnStart);
				if (firstSettingsParse && modWindowVisibleOnStartParsed) {
					ui.visible = modWindowVisibleOnStart;
				}
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

int Settings::findChar(const char* buf, char c, int startingPos) const {
	const char* ptr = buf + startingPos;
	while (*ptr != '\0') {
		if (*ptr == c) return ptr - buf;
		++ptr;
	}
	return -1;
}

std::pair<int, int> Settings::trim(std::string& str) const {
	if (str.empty()) return {0,0};
	const char* strStart = &str.front();
	const char* c = strStart;
	while (*c <= 32 && *c != '\0') {
		++c;
	}
	if (*c == '\0') {
		str.clear();
		return {c - strStart, 0};
	}

	const char* cEnd = strStart + str.size() - 1;
	while (cEnd >= c && *cEnd <= 32) {
		--cEnd;
	}
	if (cEnd < c) {
		str.clear();
		return {c - strStart, strStart + str.size() - 1 - cEnd};
	}

	str = std::string(c, cEnd - c + 1);
	return {c - strStart, strStart + str.size() - 1 - cEnd};
}

std::string Settings::toUppercase(const std::string& str) const {
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
				logwrap(fprintf(logfile, "Key combo parsing error: key not found %s\n", str.c_str()));
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

const char* Settings::formatBoolean(bool value) {
	static const char* trueStr = "true";
	static const char* falseStr = "false";
	return value ? trueStr : falseStr;
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
		logwrap(fprintf(logfile, "GetCurrentDirectoryW failed: %ls\n", winErr.getMessage()));
		return std::wstring{};
	}
	std::wstring currentDir;
	currentDir.resize(requiredSize - 1);
	if (!GetCurrentDirectoryW(currentDir.size() + 1, &currentDir.front())) {
		WinError winErr;
		logwrap(fprintf(logfile, "GetCurrentDirectoryW (second call) failed: %ls\n", winErr.getMessage()));
		return std::wstring{};
	}
	return currentDir;
}

const char* Settings::getComboRepresentation(std::vector<int>& toggle) {
	for (auto it = keyCombosToParse.begin(); it != keyCombosToParse.end(); ++it) {
		KeyComboToParse& combo = it->second;
		if (combo.keyCombo == &toggle) {
			if (!combo.representationGenerated) combo.generateRepresentation();
			if (combo.representation.empty()) return "";
			return combo.representation.c_str();
		}
	}
	return "";
}

void Settings::trashComboRepresentation(std::vector<int>& toggle) {
	for (auto it = keyCombosToParse.begin(); it != keyCombosToParse.end(); ++it) {
		KeyComboToParse& combo = it->second;
		if (combo.keyCombo == &toggle) {
			combo.representationGenerated = false;
			return;
		}
	}
}

void Settings::KeyComboToParse::generateRepresentation() {
	representation.clear();
	representationGenerated = true;
	std::vector<int>& kCombo = *keyCombo;
	bool isFirst = true;
	for (int code : kCombo) {
		const char* keyStr = settings.getKeyTxtName(code);
		if (!isFirst) {
			representation += "+";
		}
		isFirst = false;
		representation += keyStr;
	}
}

const char* Settings::getKeyTxtName(int code) {
	auto found = settings.reverseKeys.find(code);
	if (found != settings.reverseKeys.end()) {
		return found->second->name;
	}
	return "";
}

const char* Settings::getKeyRepresentation(int code) {
	auto found = settings.reverseKeys.find(code);
	if (found != settings.reverseKeys.end()) {
		return found->second->uiName;
	}
	return "";
}

void Settings::writeSettings() {
	HANDLE temp = directoryChangeHandle;
	directoryChangeHandle = NULL;
	changesListenerWakeType = WAKE_TYPE_WRITING_FILE;
	SetEvent(changesListenerWakeEvent);
	FindCloseChangeNotification(temp);
	
	writeSettingsMain();
	
	registerListenerForChanges();
}

void Settings::writeSettingsMain() {
	logwrap(fputs("Writing settings\n", logfile));
	HANDLE file = CreateFileW(settingsPath.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		WinError winErr;
		logwrap(fprintf(logfile, "Could not open INI file: %ls\n", winErr.getMessage()));
		return;
	}
	struct LineInfo {
		int lineNumber = -1;
		int keyPos = -1;
		int equalSignPos = -1;
		int valuePos = -1;
		int commentPos = -1;
		std::string line;
		std::string key;
		std::string keyUpper;
		std::string value;
		std::string comment;
		bool needReform = false;
	};
	std::unordered_map<std::string, LineInfo*> keyToLine;
	int lineNumber = 0;
	std::list<LineInfo> lines;
	std::string accum;
	char buf[128];
	buf[0] = '\0';
	int bufContentSize = 0;
	bool reachedEnd = false;
	bool rCharDetected = false;
	while (true) {
		accum.clear();
		++lineNumber;
		int pos = findChar(buf, '\n');
		bool needToReadMore = true;
		if (pos != -1) {
			needToReadMore = false;
			*(buf + pos) = '\0';
			if (pos > 0 && *(buf + pos - 1) == '\r') {
				rCharDetected = true;
				*(buf + pos - 1) = '\0';
			}
			if (pos == 0 && !accum.empty() && accum.back() == '\r') {
				rCharDetected = true;
				accum.resize(accum.size() - 1);
			}
			accum.append(buf);
			if (pos + 1 >= bufContentSize) {
				bufContentSize = 0;
				buf[0] = '\0';
			} else {
				bufContentSize = bufContentSize - (pos + 1);
				memmove(buf, buf + pos + 1, bufContentSize + 1);
			}
		}
		while (needToReadMore) {
			if (bufContentSize) {
				accum += buf;
				// can have \r character at the end of buf here, will deal with it in the branch above
				bufContentSize = 0;
				buf[0] = '\0';
			}
			if (reachedEnd) {
				needToReadMore = false;
				break;
			}
			needToReadMore = true;
			DWORD bytesRead;
			if (!ReadFile(file, buf, sizeof buf - 1, &bytesRead, NULL)) {
				WinError winErr;
				logwrap(fprintf(logfile, "Error reading INI file: %ls\n", winErr.getMessage()));
				CloseHandle(file);
				return;
			}
			bufContentSize = bytesRead;
			buf[bytesRead] = '\0';
			char* bufPtr = buf;
			pos = findChar(buf, '\n');
			if (pos != -1) {
				*(buf + pos) = '\0';
				if (pos > 0 && *(buf + pos - 1) == '\r') {
					rCharDetected = true;
					*(buf + pos - 1) = '\0';
				}
				needToReadMore = false;
				if (pos == 0 && !accum.empty() && accum.back() == '\r') {
					rCharDetected = true;
					accum.resize(accum.size() - 1);
				}
				accum.append(buf);
				if (pos + 1 >= bufContentSize) {
					bufContentSize = 0;
					buf[0] = '\0';
				} else {
					bufContentSize = bufContentSize - (pos + 1);
					memmove(buf, buf + pos + 1, bufContentSize + 1);
				}
			}
			if (bytesRead < sizeof buf - 1) {
				if (needToReadMore && bytesRead > 0) {
					accum += buf;
					// can't have \r character here
					bufContentSize = 0;
					buf[0] = '\0';
				}
				needToReadMore = false;
				reachedEnd = true;
				break;
			}
		}
		if (reachedEnd && accum.empty()) {
			break;
		}
		
		int commentPos = findMinCommentPos(accum.c_str());
	
		int equalSignPos = findChar(accum.c_str(), '=');
		
		lines.emplace_back();
		LineInfo& li = lines.back();
		li.lineNumber = lineNumber;
		li.line = accum;
		
		if (equalSignPos == -1 || commentPos != -1 && equalSignPos != -1 && equalSignPos > commentPos) {
			li.comment = accum;
			li.commentPos = 0;
			continue;
		}
		
		std::string keyStr(accum.begin(), accum.begin() + equalSignPos);
		std::pair<int, int> trimResult = trim(keyStr);
		
		li.keyPos = trimResult.first;
		li.key = keyStr;
		li.keyUpper = toUppercase(keyStr);
		li.commentPos = commentPos;
		if (commentPos != -1) {
			li.comment.assign(accum.begin() + commentPos, accum.end());
		}
		li.equalSignPos = equalSignPos;
		
		const char* bufPos = &accum.front() + equalSignPos + 1;
		auto accumEnd = commentPos == -1 ? accum.end() : accum.begin() + commentPos;
		li.value.assign(accum.begin() + equalSignPos + 1, accumEnd);
		trimResult = trim(li.value);
		li.valuePos = equalSignPos + 1 + trimResult.first;
		
		auto found = keyCombosToParse.find(li.keyUpper);
		if (found != keyCombosToParse.end()) {
			std::vector<int> parsedCombo;
			if (!parseKeys(found->second.name, li.value, parsedCombo)) {
				li.needReform = true;
			} else if (compareKeyCombos(*found->second.keyCombo, parsedCombo) != 0) {
				li.needReform = true;
			}
			if (li.needReform) {
				li.value = getComboRepresentation(*found->second.keyCombo);
				logwrap(fprintf(logfile, "Combo representation for %s: %s\n", li.key.c_str(), li.value.c_str()));
				logwrap(fputs("Combo contains codes: ", logfile));
				for (int code : *found->second.keyCombo) {
					logwrap(fprintf(logfile, "%d ", code));
				}
				logwrap(fprintf(logfile, "\n"));
			}
		} else {
			li.needReform = true;
		}
		auto ktl = keyToLine.find(li.keyUpper);
		if (ktl != keyToLine.end()) {
			ktl->second = &li;
		} else {
			keyToLine.insert({li.keyUpper, &li});
		}
	}
	
	std::function<LineInfo&(const char*, const char*, const char*)> appendLine = [&](
		const char* name, const char* value, const char* iniDescription
	) -> LineInfo& {
		if (!lines.empty() && !isWhitespace(lines.back().line.c_str())) {
			lines.emplace_back();
			LineInfo& li = lines.back();
			li.commentPos = 0;
			li.lineNumber = lines.size();
		}
		for (const std::string& piece : split(iniDescription, '\n')) {
			lines.emplace_back();
			LineInfo& li = lines.back();
			li.lineNumber = lines.size();
			li.commentPos = 0;
			li.comment = piece;
			li.needReform = true;
		}
		lines.emplace_back();
		LineInfo& li = lines.back();
		li.lineNumber = lines.size();
		li.keyPos = 0;
		li.key = name;
		li.keyUpper = toUppercase(li.key);
		li.equalSignPos = li.key.size() + 1;
		li.valuePos = li.equalSignPos + 2;
		li.value = value;
		li.needReform = true;
		return li;
	};
	
	for (auto it = keyCombosToParse.begin(); it != keyCombosToParse.end(); ++it) {
		KeyComboToParse& k = it->second;
		auto found = keyToLine.find(it->first);
		if (found == keyToLine.end()) {
			appendLine(k.name, getComboRepresentation(*k.keyCombo), k.iniDescription);
		}
	}
	
	std::function<LineInfo&(const char*, const char*, const char*)> replaceOrAddSetting = [&](
		const char* name, const char* value, const char* description
	) -> LineInfo& {
		auto found = keyToLine.find(toUppercase(name));
		if (found == keyToLine.end()) {
			return appendLine(name, value, description);
		} else {
			found->second->needReform = true;
			found->second->value = value;
			return *found->second;
		}
	};
	
	replaceOrAddSetting("slowmoTimes", std::to_string(slowmoTimes).c_str(), getOtherINIDescription(&slowmoTimes));
	replaceOrAddSetting("allowContinuousScreenshotting", formatBoolean(allowContinuousScreenshotting), getOtherINIDescription(&allowContinuousScreenshotting));
	replaceOrAddSetting("startDisabled", formatBoolean(startDisabled), getOtherINIDescription(&startDisabled));
	std::string scrPathCpy;
	{
		std::unique_lock<std::mutex> screenshotGuard(screenshotPathMutex);
		scrPathCpy = screenshotPath;
	}
	logwrap(fprintf(logfile, "Writing screenshot path: %s\n", screenshotPath.c_str()));
	LineInfo& li = replaceOrAddSetting("screenshotPath", scrPathCpy.c_str(), getOtherINIDescription(&screenshotPath));
	if (li.value.empty()) {
		li.commentPos = li.equalSignPos + 2;
		li.comment = ";C:\\Users\\yourUser\\Desktop\\test screenshot name.png   don't forget to uncomment (; is a comment)";
	}
	
	replaceOrAddSetting("dontUseScreenshotTransparency", formatBoolean(dontUseScreenshotTransparency), getOtherINIDescription(&dontUseScreenshotTransparency));
	replaceOrAddSetting("drawPushboxCheckSeparately", formatBoolean(drawPushboxCheckSeparately), getOtherINIDescription(&drawPushboxCheckSeparately));
	replaceOrAddSetting("modWindowVisibleOnStart", formatBoolean(modWindowVisibleOnStart), getOtherINIDescription(&modWindowVisibleOnStart));
	
	SetFilePointer(file, 0, NULL, FILE_BEGIN);
	
	std::string lineStr;
	for (LineInfo& li : lines) {
		lineStr.clear();
		DWORD bytesWritten;
		if (!li.needReform) {
			WriteFile(file, li.line.c_str(), li.line.size(), &bytesWritten, NULL);
		} else {
			if (li.keyPos != -1) {
				lineStr.append(li.keyPos, ' ');
				lineStr += li.key;
			}
			if (li.equalSignPos != -1) {
				if (li.equalSignPos > (int)lineStr.size()) {
					lineStr.append(li.equalSignPos - lineStr.size(), ' ');
				}
				lineStr += '=';
			}
			if (li.valuePos != -1) {
				if (li.valuePos > (int)lineStr.size()) {
					lineStr.append(li.valuePos - lineStr.size(), ' ');
				}
				lineStr += li.value;
			}
			if (li.commentPos != -1) {
				if (li.commentPos > (int)lineStr.size()) {
					lineStr.append(li.commentPos - lineStr.size(), ' ');
				}
				lineStr += li.comment;
			}
			WriteFile(file, lineStr.c_str(), lineStr.size(), &bytesWritten, NULL);
		}
		if (rCharDetected) {
			WriteFile(file, "\r\n", 1, &bytesWritten, NULL);
		} else {
			WriteFile(file, "\n", 1, &bytesWritten, NULL);
		}
	}
	
	SetEndOfFile(file);
	
	CloseHandle(file);
}

void Settings::registerListenerForChanges() {
	std::wstring currentDir = settingsPath;
	if (!currentDir.empty()) {
		for (auto it = currentDir.begin() + (currentDir.size() - 1); ; --it) {
			if (*it == L'\\') {
				currentDir.resize(it - currentDir.begin());
				break;
			}
			if (it == currentDir.begin()) break;
		}
	}
	logwrap(fprintf(logfile, "registerListenerForChanges currentDir: %ls\n", currentDir.c_str()));
	
	if (!currentDir.empty()) {
		directoryChangeHandle = FindFirstChangeNotificationW(
			currentDir.c_str(), // directory to watch 
			FALSE,              // do not watch subtree 
			FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE); // watch file name changes and last write date changes
		if (directoryChangeHandle == INVALID_HANDLE_VALUE || !directoryChangeHandle) {
			WinError winErr;
			logwrap(fprintf(logfile, "FindFirstChangeNotificationW failed: %ls\n", winErr.getMessage()));
			directoryChangeHandle = NULL;
		} else if (!changesListener) {
			changesListenerWakeEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
			changeListenerExitedEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
			DWORD changesListenerId = 0;
			changesListener = CreateThread(NULL, 0, changesListenerLoop, NULL, 0, &changesListenerId);
			logwrap(fprintf(logfile, "changesListenerId: %d\n", changesListenerId));
		} else {
			changesListenerWakeType = WAKE_TYPE_WRITING_FILE;
			SetEvent(changesListenerWakeEvent);
		}
	} else {
		logwrap(fputs("registerListenerForChanges: the current directory is empty\n", logfile));
	}
}

DWORD WINAPI Settings::changesListenerLoop(LPVOID lpThreadParameter) {
	struct SetEventOnExit {
	public:
		~SetEventOnExit() {
			SetEvent(settings.changeListenerExitedEvent);
		}
	} setEventOnExit;
	while (true) {
		HANDLE handles[2] { 0 };
		int handlesCount = 0;
		if (settings.directoryChangeHandle) {
			handles[handlesCount++] = settings.directoryChangeHandle;
		}
		handles[handlesCount++] = settings.changesListenerWakeEvent;
		DWORD result = WaitForMultipleObjects(handlesCount, handles, FALSE, INFINITE);
		if (result == WAIT_OBJECT_0 && handlesCount == 2) {
			if (settings.directoryChangeHandle) {
				if (!settings.firstSettingsParse && keyboard.thisProcessWindow) {
					PostMessageW(keyboard.thisProcessWindow, WM_APP_SETTINGS_FILE_UPDATED, 0, 0);
				}
				if (!FindNextChangeNotification(settings.directoryChangeHandle)) {
					WinError winErr;
					logwrap(fprintf(logfile, "FindNextChangeNotification failed: %ls\n", winErr.getMessage()));
					FindCloseChangeNotification(settings.directoryChangeHandle);
					settings.directoryChangeHandle = NULL;
					return 0;
				}
			}
		} else if (result == WAIT_OBJECT_0 + (handlesCount == 2 ? 1 : 0)) {
			if (settings.changesListenerWakeType == WAKE_TYPE_EXIT) {
				return 0;
			}
		}
	}
}

bool Settings::isWhitespace(const char* str) {
	for (const char* c = str; ; ++c) {
		char cVal = *c;
		if (cVal == '\0') return true;
		if (cVal > 32) return false;
	}
}

int Settings::compareKeyCombos(const std::vector<int>& left, const std::vector<int>& right) {
	auto itLeft = left.cbegin();
	auto itLeftEnd = left.cend();
	auto itRight = right.cbegin();
	auto itRightEnd = right.cend();
	while (true) {
		if (itLeft == itLeftEnd) {
			if (itRight == itRightEnd) return 0;
			return -1;
		} else if (itRight == itRightEnd) return 1;
		if (*itLeft != *itRight) {
			if (*itLeft < *itRight) return -1;
			return 1;
		}
		++itLeft;
		++itRight;
	}
}

void Settings::onKeyCombosUpdated() {
	keyboard.removeAllKeyCodes();
	for (auto it = keyCombosToParse.begin(); it != keyCombosToParse.end(); ++it) {
		KeyComboToParse& k = it->second;
		k.representationGenerated = false;
		keyboard.addNewKeyCodes(*k.keyCombo);
	}
}

void Settings::getComboInfo(std::vector<int>& keyCombo, ComboInfo* info) {
	for (auto it = keyCombosToParse.begin(); it != keyCombosToParse.end(); ++it) {
		KeyComboToParse& combo = it->second;
		if (combo.keyCombo == &keyCombo) {
			info->uiName = combo.uiName;
			info->uiDescription = combo.uiDescription.c_str();
			return;
		}
	}
	info->uiName = info->uiDescription = "";
}

const char* Settings::getOtherUIDescription(void* ptr) {
	for (const OtherDescription& desc : otherDescriptions) {
		if (desc.ptr == ptr) return desc.uiDescription.c_str();
	}
	return "";
}

const char* Settings::getOtherINIDescription(void* ptr) {
	for (const OtherDescription& desc : otherDescriptions) {
		if (desc.ptr == ptr) return desc.iniDescription;
	}
	return "";
}

void Settings::registerOtherDescription(void* ptr, const char* iniDescription) {
	otherDescriptions.emplace_back();
	OtherDescription& desc = otherDescriptions.back();
	desc.ptr = ptr;
	desc.iniDescription = iniDescription;
	desc.uiDescription = convertToUiDescription(iniDescription);
}

std::string Settings::convertToUiDescription(const char* iniDescription) {
	std::string result;
	if (*iniDescription != '\0') {
		result.reserve(strlen(iniDescription));
		enum ParseMode {
			PARSING_NEWLINE,
			PARSING_SEMICOLON,
			PARSING_WHITESPACE
		} parsingMode = PARSING_NEWLINE;
		for (; ; ++iniDescription) {
			char cVal = *iniDescription;
			if (cVal == '\0') break;
			if (cVal == ';' && parsingMode == PARSING_NEWLINE) {
				parsingMode = PARSING_SEMICOLON;
				continue;
			}
			if (cVal == '\n') {
				parsingMode = PARSING_NEWLINE;
				result += '\n';
				continue;
			}
			if (cVal <= 32 && parsingMode == PARSING_SEMICOLON) {
				parsingMode = PARSING_WHITESPACE;
				continue;
			}
			result += cVal;
		}
	}
	return result;
}
