#include "pch.h"
#include "Settings.h"
#include "logging.h"
#include "Keyboard.h"
#include "GifMode.h"
#include "Detouring.h"
#include "WError.h"
#include <list>
#include "UI.h"
#include "CustomWindowMessages.h"
#include <unordered_map>
#include <functional>
#include "PlayerInfo.h"
#include "LineReaderFromString.h"
#include "SplitStringIterator.h"
#include <vector>
#include "KeyDefinitions.h"
#include "SettingsTopCommentDefinition.h"

Settings settings;

static const char* rn = "\r\n";
const char* const Settings::SETTINGS_HITBOX = "Hitboxes";
const char* const Settings::SETTINGS_HITBOX_SETTINGS = "Settings - Hitbox Settings";
const char* const Settings::SETTINGS_GENERAL = "Settings - General Settings";
const char* const Settings::SETTINGS_FRAMEBAR = "Settings - Framebar Settings";
const char* const Settings::SETTINGS_COMBO_RECIPE = "Combo Recipe";
const char* const Settings::SETTINGS_CHARACTER_SPECIFIC = "UI - Character specific";

bool Settings::onDllMain() {
	#define keyEnumFunc(identifier, userFriendlyName, virtualKeyCode) addKey(identifier, userFriendlyName, virtualKeyCode);
	#define keyEnumFuncLast(identifier, userFriendlyName, virtualKeyCode) addKey(identifier, userFriendlyName, virtualKeyCode);
	#define keyEnumFunc_keyRange(str) addKeyRange(str);
	keyEnum
	#undef keyEnumFunc
	#undef keyEnumFuncLast
	#undef keyEnumFunc_keyRange
	
	for (auto it = keys.begin(); it != keys.end(); ++it) {
		reverseKeys.insert({it->second.code, &it->second});
	}
	
	#define settingsKeyCombo(name, displayName, defaultValue, description) insertKeyComboToParse(#name, displayName, &name, defaultValue, description);
	#define settingsField(type, name, defaultValue, displayName, section, description, inlineComment) \
		registerOtherDescription(&name, #name, displayName, section, description);
	#include "SettingsDefinitions.h"
	#undef settingsField
	#undef settingsKeyCombo
	
	pointerIntoSettingsIntoDescription.resize(offsetof(Settings, settingsMembersEnd) - offsetof(Settings, settingsMembersStart));
	for (OtherDescription& desc : otherDescriptions) {
		pointerIntoSettingsIntoDescription[(BYTE*)desc.ptr - (BYTE*)this - offsetof(Settings, settingsMembersStart)] = &desc;
		settingNameToOffset[{ desc.iniName, desc.iniName + strlen(desc.iniName) }] = (BYTE*)desc.ptr - (BYTE*)this;
		iniNameToUiNameMap.insert(
			{
				desc.iniName,
				{ desc.uiFullPath.c_str(), desc.uiName }
			});
	}
	
	offsetToKeyComboToParse.resize(offsetToKeyComboIndex(&keyCombosEnd));
	for (auto it = keyCombosToParse.begin(); it != keyCombosToParse.end(); ++it) {
		KeyComboToParse& keyCombo = it->second;
		
		offsetToKeyComboToParse[offsetToKeyComboIndex(keyCombo.keyCombo)] = &keyCombo;
		
		std::string& strRef = keyCombo.uiFullName;
		static const StringWithLength prefix = "'Settings - Keyboard Shortcuts - ";
		strRef.reserve(prefix.length + strlen(keyCombo.uiName) + 1);
		strRef = prefix.txt;
		strRef += keyCombo.uiName;
		strRef += '\'';
		iniNameToUiNameMap.insert(
			{
				it->first.start,
				{ strRef.c_str(), keyCombo.uiName }
			});
		
	}
	for (auto it = keyCombosToParse.begin(); it != keyCombosToParse.end(); ++it) {
		KeyComboToParse& keyCombo = it->second;
		keyCombo.uiDescription = convertToUiDescription(keyCombo.iniDescription.txt);
	}
	for (OtherDescription& desc : otherDescriptions) {
		desc.uiDescription = convertToUiDescription(desc.iniDescription.txt);
	}
	
	std::wstring currentDir = getCurrentDirectory();
	settingsPath = currentDir + L"\\ggxrd_hitbox_overlay.ini";
	logwrap(fprintf(logfile, "INI file path: %ls\n", settingsPath.c_str()));
	
	registerListenerForChanges();
// todo: elphelt make shotgun charge display 1f later
	readSettings(true);

	return true;
}

void Settings::onDllDetach() {
	if (directoryChangeHandle) {
		HANDLE temp = directoryChangeHandle;
		directoryChangeHandle = NULL;
		FindCloseChangeNotification(temp);
	}
	if (changesListener) {
		if (!changesListenerStarted) {
			// When the DLL returns FALSE in DllMain upon DLL_PROCESS_ATTACH, and
			// the changesListener thread has already been created,
			// it will not have started yet by the time this code runs.
			// We need to terminate it because there's absolutely no way to interact with it,
			// since its waiting on a lock that DllMain holds.
			TerminateThread(changesListener, 0);
		} else {
			changesListenerWakeType = WAKE_TYPE_EXIT;
			// We're calling WaitForSingleObject on the changeListenerExitedEvent, instead of on the
			// changesListener, because this call happens in DllMain, which happens under an internal
			// system lock, called the "loader" lock.
			// "The loader lock is taken by any function that needs to access the list of DLLs loaded into the process." - Raymond Chen.
			// When a thread exits, even if you used DisableThreadLibraryCalls(hModule), it calls DllMain with the loader lock, and this
			// happens before its object is signaled.
			// As such, you cannot wait on a thread inside DllMain.
			
			bool eventIsSet = false;
			while (true) {
				DWORD exitCode;
				if (GetExitCodeThread(changesListener, &exitCode)) {
					// On normal exit the thread is already killed by something
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
		}
		changeListenerExitedEvent.close();
		changesListener.close();
		changesListenerWakeEvent.close();
	}
}

void Settings::addKey(const char* name, const char* uiName, int code) {
	keys.insert({{ name, name + strlen(name) }, {name, uiName, code}});
	if (code < minKeyCode) minKeyCode = code;
	if (code > maxKeyCode) maxKeyCode = code;
}

void Settings::addKeyRange(const char* str) {
	static std::list<std::string> mem;
	for (const char* c = str; *c != '\0'; ++c) {
		mem.emplace_back(1, ' ');
		std::string& newMem = mem.back();
		newMem[0] = *c;
		addKey(newMem.c_str(), newMem.c_str(), *c);
	}
}

void Settings::insertKeyComboToParse(const char* name, const char* uiName, std::vector<int>* keyCombo, const StringWithLength& defaultValue, const StringWithLength& iniDescription) {
	keyCombosToParse.insert(
		{
			// key: StringView
			{ name, name + strlen(name) },
			
			// value
			KeyComboToParse {
				strlen(name),
				name,
				uiName,
				keyCombo,
				defaultValue,
				iniDescription
			}
		}
	);
}

// INI file must be placed next to the game's executable at SteamLibrary\steamapps\common\GUILTY GEAR Xrd -REVELATOR-\Binaries\Win32\ggxrd_hitbox_overlay.ini
// Example INI file content:
// gifModeToggle = Ctrl+F3
void Settings::readSettings(bool isFirstEverRead) {
	logwrap(fputs("Reading settings\n", logfile));
	
	char errorString[500];
	FILE* file = NULL;
	if (_wfopen_s(&file, settingsPath.c_str(), L"rt") || !file) {
		strerror_s(errorString, errno);
		logwrap(fprintf(logfile, "Could not open INI file: %s\n", errorString));
		file = NULL;
	}
	if (!isFirstEverRead && !file) {
		return;
	}
	std::unique_lock<std::mutex> keyboardGuard(keyboard.mutex);
	Keyboard::MutexLockedFromOutsideGuard keyboardOutsideGuard;
	std::unique_lock<std::mutex> guard(keyCombosMutex);
	std::unique_lock<std::mutex> screenshotGuard(screenshotPathMutex);
	
	for (auto it = keyCombosToParse.begin(); it != keyCombosToParse.end(); ++it) {
		it->second.keyCombo->clear();
		it->second.isParsed = false;
	}
	if (!keyboard.isInitialized()) keyboard.initialize();
	keyboard.removeAllKeyCodes();
	
	
	#define settingsKeyCombo(name, displayName, defaultValue, description)
	#define settingsField(type, name, defaultValue, displayName, section, description, inlineComment) bool name##Parsed = false;
	#include "SettingsDefinitions.h"
	#undef settingsField
	#undef settingsKeyCombo
	
	screenshotPath.clear();  // we can clear this in advance because it is protected by screenshotPathMutex
	// keyboard shortcuts are protected by keyCombosMutex
	// the other settings are exposed bare
	bool oldTurnOffPostEffectWhenMakingBackgroundBlack = turnOffPostEffectWhenMakingBackgroundBlack;
	bool oldHideRankIcons = hideRankIcons;
	bool oldUsePositionResetMod = usePositionResetMod;
	bool oldPlayer1IsBoss = player1IsBoss;
	bool oldPlayer2IsBoss = player2IsBoss;
	bool oldconnectionTierToPretendAs = connectionTierToPretendAs;
	int oldoverrideYourConnectionTierForFilter = overrideYourConnectionTierForFilter;

	std::string accum;
	char buf[129];
	std::string keyValue;
	StringView keyNameStartEnd;
	if (file) {
		while (true) {
			accum.clear();
			bool exitOuterLoop = false;
			bool exitOuterLoopIfEmpty = false;
			while (true) {
				buf[0] = '\0';
				if (!fgets(buf, 129, file)) {
					if (ferror(file)) {
						exitOuterLoop = true;
						strerror_s(errorString, errno);
						logwrap(fprintf(logfile, "Error reading INI file: %s\n", errorString));
					}
					exitOuterLoopIfEmpty = true;
					break;
				}
				accum += buf;
				size_t len = strlen(buf);
				if (len == 0 || buf[len - 1] == '\n') break;
			}
			if (exitOuterLoop) {
				break;
			}
			if (exitOuterLoopIfEmpty && accum.empty()) {
				break;
			}
			if (!accum.empty()) {
				parseKeyName(accum.c_str(), keyNameStartEnd);
				if (keyNameStartEnd.start != keyNameStartEnd.end) {
					getKeyValue(accum.c_str(), keyValue);
					auto found = keyCombosToParse.find(keyNameStartEnd);
					if (found != keyCombosToParse.end()) {
						found->second.isParsed = parseKeys(found->second.name, keyValue, *found->second.keyCombo);
					} else {
						auto foundOffsetIt = settingNameToOffset.find(keyNameStartEnd);
						DWORD foundOffset = 0xffffffff;
						if (foundOffsetIt != settingNameToOffset.end()) {
							foundOffset = foundOffsetIt->second;
							switch (foundOffset) {
								#define booleanPreset(name) \
									case offsetof(Settings, name): \
										if (!name##Parsed) { \
											name##Parsed = parseBoolean(#name, keyValue, name); \
										} \
										break;
										
								#define integerPreset(name) \
									case offsetof(Settings, name): \
									if (!name##Parsed) { \
										name##Parsed = parseInteger(#name, keyValue, name); \
									} \
									break;
										
								#define floatPreset(name) \
									case offsetof(Settings, name): \
									if (!name##Parsed) { \
										name##Parsed = parseFloat(#name, keyValue, name); \
									} \
									break;
										
								#define screenshotPathPreset(name) \
									case offsetof(Settings, name): \
									if (!name##Parsed) { \
										name##Parsed = true; \
										name = keyValue;  /* in UTF-8 */ \
										logwrap(fprintf(logfile, "Parsed " #name " (UTF8): %s\n", keyValue.c_str())); \
									} \
									break;
								
								#define int integerPreset
								#define bool booleanPreset
								#define ScreenshotPath screenshotPathPreset
								#define float floatPreset
								#define settingsKeyCombo(name, displayName, defaultValue, description)
								#define settingsField(type, name, defaultValue, displayName, section, description, inlineComment) type(name)
								#include "SettingsDefinitions.h"
								#undef settingsField
								#undef settingsKeyCombo
								#undef float
								#undef ScreenshotPath
								#undef bool
								#undef int
								#undef screenshotPathPreset
								#undef floatPreset
								#undef integerPreset
								#undef booleanPreset
							}
						}
					}
				}
			}
			if (feof(file)) break;
		}
		fclose(file);
	}
	
	#define settingsKeyCombo(name, displayName, defaultValue, description)
	#define settingsField(type, name, defaultValue, displayName, section, description, inlineComment) \
		if (!name##Parsed) { \
			name = defaultValue; \
		}
	#include "SettingsDefinitions.h"
	#undef settingsField
	#undef settingsKeyCombo
	
	// when delivering the 7.0.0 update to existing users of 6.X.X, they probably got used to framebars not being condensed
	if (!condenseIntoOneProjectileFramebarParsed && file) {
		condenseIntoOneProjectileFramebar = false;
	}
	
	if (framebarStoredFramesCount < 1) {
		framebarStoredFramesCount = 1;
	}
	if (framebarStoredFramesCount > _countof(Framebar::frames)) {
		framebarStoredFramesCount = _countof(Framebar::frames);
	}
	
	if (framebarDisplayedFramesCount < 1) {
		framebarDisplayedFramesCount = 1;
	}
	if (framebarDisplayedFramesCount.load() > framebarStoredFramesCount.load()) {
		framebarDisplayedFramesCount = framebarStoredFramesCount.load();
	}
	
	if (firstSettingsParse) {
		ui.visible = modWindowVisibleOnStart;
		if (startDisabled) {
			gifMode.modDisabled = true;
		}
		// keyboard.thisProcessWindow is NULL at this stage of DLL initialization
	} else {
		if (turnOffPostEffectWhenMakingBackgroundBlack != oldTurnOffPostEffectWhenMakingBackgroundBlack && keyboard.thisProcessWindow) {
			PostMessageW(keyboard.thisProcessWindow, WM_APP_TURN_OFF_POST_EFFECT_SETTING_CHANGED, FALSE, TRUE);
		}
		if (hideRankIcons != oldHideRankIcons && hideRankIcons && keyboard.thisProcessWindow) {
			PostMessageW(keyboard.thisProcessWindow, WM_APP_HIDE_RANK_ICONS, FALSE, FALSE);
		}
		if (usePositionResetMod != oldUsePositionResetMod && usePositionResetMod) {
			PostMessageW(keyboard.thisProcessWindow, WM_APP_USE_POSITION_RESET_MOD_CHANGED, FALSE, FALSE);
		}
		if (player1IsBoss != oldPlayer1IsBoss && player1IsBoss
				|| player2IsBoss != oldPlayer2IsBoss && player2IsBoss) {
			PostMessageW(keyboard.thisProcessWindow, WM_APP_PLAYER_IS_BOSS_CHANGED, FALSE, FALSE);
		}
		if (
			(
				connectionTierToPretendAs != oldconnectionTierToPretendAs
				|| overrideYourConnectionTierForFilter != oldoverrideYourConnectionTierForFilter
			)
		 	&& connectionTierToPretendAs
	 	) {
			PostMessageW(keyboard.thisProcessWindow, WM_APP_CONNECTION_TIER_CHANGED, FALSE, FALSE);
		}
	}
	
	for (auto it = keyCombosToParse.begin(); it != keyCombosToParse.end(); ++it) {
		if (!it->second.isParsed) {
			parseKeys(it->first.start, it->second.defaultValue, *it->second.keyCombo);
		}
		keyboard.addNewKeyCodes(*it->second.keyCombo);
	}
	
	firstSettingsParse = false;
}

int Settings::findChar(const char* buf, char c, int startingPos) {
	const char* ptr = strchr(buf + startingPos, c);
	if (!ptr) return -1;
	return ptr - buf;
}

int Settings::findChar(const char* bufStart, const char* bufEnd, char c, int startingPos) {
	const char* searchStart = bufStart + startingPos;
	const char* ptr = (const char*)memchr(searchStart, c, bufEnd - searchStart);
	if (!ptr) return -1;
	return ptr - bufStart;
}

std::pair<int, int> Settings::trim(std::string& str) {
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

std::vector<std::string> Settings::split(const std::string& str, char c) {
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

bool Settings::parseKeys(const char* keyName, const char* keyValueStart, const char* keyValueEnd, std::vector<int>& keyCodes) {
	if (keyValueEnd != keyValueStart) {
		SplitStringIterator splitter(keyValueStart, keyValueEnd, '+');
		const char* partStart;
		const char* partEnd;
		while (splitter.getNext(&partStart, &partEnd)) {
			if (partStart == partEnd) {
				#ifdef LOG_PATH
				std::string buf;
				buf.assign(keyValueStart, keyValueEnd - keyValueStart);
				logwrap(fprintf(logfile, "Key combo parsing error: key is empty %s\n", buf.c_str()));
				#endif
				return false;
			}
			StringView partTrimmed = { skipWhitespace(partStart, partEnd), rewindWhitespace(partEnd - 1, partStart) };
			if (partTrimmed.end < partTrimmed.start) {
				#ifdef LOG_PATH
				std::string buf;
				buf.assign(keyValueStart, keyValueEnd - keyValueStart);
				logwrap(fprintf(logfile, "Key combo parsing error: key is empty %s\n", buf.c_str()));
				#endif
				return false;
			}
			++partTrimmed.end;
			auto found = keys.find(partTrimmed);
			if (found != keys.end()) {
				keyCodes.push_back(found->second.code);
			} else {
				#ifdef LOG_PATH
				std::string buf;
				buf.assign(partTrimmed.start, partTrimmed.end - partTrimmed.start);
				logwrap(fprintf(logfile, "Key combo parsing error: key not found %s\n", buf.c_str()));
				#endif
				return false;
			}
		}
		#ifdef LOG_PATH
		std::string buf;
		buf.assign(keyValueStart, keyValueEnd - keyValueStart);
		logwrap(fprintf(logfile, "Parsed key codes for %s: %s\n", keyName, buf.c_str()));
		#endif
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

bool Settings::parseFloat(const char* keyName, const std::string& keyValue, float& floatValue) {
	bool isError;
	floatValue = parseFloat(keyValue.c_str(), &isError);
	if (isError) {
		return false;
	}
	logwrap(fprintf(logfile, "Parsed float for %s: %d\n", keyName, floatValue));
	return true;
}

StringWithLength Settings::formatBoolean(bool value) {
	static const StringWithLength trueStr = "true";
	static const StringWithLength falseStr = "false";
	return value ? trueStr : falseStr;
}

int Settings::findMinCommentPos(const char* buf) {
	int colonPos = findChar(buf, ';');
	int hashtagPos = findChar(buf, '#');
	int minCommentPos = -1;
	if (colonPos != -1) minCommentPos = colonPos;
	if (minCommentPos == -1 || hashtagPos != -1 && minCommentPos != -1 && hashtagPos < minCommentPos) minCommentPos = hashtagPos;
	return minCommentPos;
}

int Settings::findMinCommentPos(const char* bufStart, const char* bufEnd) {
	int colonPos = findChar(bufStart, bufEnd, ';');
	int hashtagPos = findChar(bufStart, bufEnd, '#');
	int minCommentPos = -1;
	if (colonPos != -1) minCommentPos = colonPos;
	if (minCommentPos == -1 || hashtagPos != -1 && minCommentPos != -1 && hashtagPos < minCommentPos) minCommentPos = hashtagPos;
	return minCommentPos;
}

void Settings::parseKeyName(const char* buf, StringView& result) {

	int minCommentPos = findMinCommentPos(buf);

	int equalSignPos = findChar(buf, '=');

	if (equalSignPos == -1 || minCommentPos != -1 && equalSignPos != -1 && equalSignPos > minCommentPos) {
		result.start = nullptr;
		result.end = nullptr;
		return;
	}
	
	if (equalSignPos == 0) {
		result.start = nullptr;
		result.end = nullptr;
		return;
	}
	
	result.start = skipWhitespace(buf, buf + equalSignPos);
	result.end = rewindWhitespace(buf + equalSignPos - 1, buf);
	if (result.start > result.end) {
		result.start = nullptr;
		result.end = nullptr;
		return;
	}
	++result.end;
}

void Settings::getKeyValue(const char* buf, std::string& result) {
	int minCommentPos = findMinCommentPos(buf);
	int equalSignPos = findChar(buf, '=');

	if (equalSignPos == -1 || minCommentPos != -1 && equalSignPos != -1 && equalSignPos > minCommentPos) {
		result.clear();
		return;
	}

	const char* bufPos = buf + equalSignPos + 1;
	size_t bufLength = strlen(buf);
	if (minCommentPos != -1) bufLength -= (bufLength - minCommentPos);
	int lengthFromBufPos = buf + bufLength - bufPos;
	if (lengthFromBufPos == 0) {
		result.clear();
		return;
	}
	result.assign(bufPos, lengthFromBufPos);
	trim(result);
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

StringWithLength Settings::getComboRepresentation(const std::vector<int>& toggle) {
	KeyComboToParse& combo = getKeyComboToParseByOffset(toggle);
	if (!combo.representationGenerated) combo.generateRepresentation();
	if (combo.representation.empty()) return "";
	return combo.representation;
}

void Settings::trashComboRepresentation(std::vector<int>& toggle) {
	KeyComboToParse& combo = getKeyComboToParseByOffset(toggle);
	combo.representationGenerated = false;
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
	auto found = reverseKeys.find(code);
	if (found != reverseKeys.end()) {
		return found->second->name;
	}
	return "";
}

const char* Settings::getKeyRepresentation(int code) {
	auto found = reverseKeys.find(code);
	if (found != reverseKeys.end()) {
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
	struct {
		StringView line;
		StringView key;
		const char* equalSignPtr = nullptr;
		StringView value;
		StringView comment;
		bool modified = false;
		StringWithLength newValuePtr;
		HANDLE file;
		void compareAndUpdateValue(const StringWithLength& otherPtr) {
			bool otherEmpty = !otherPtr.txt || otherPtr.length == 0;
			if (!value.start && otherEmpty) return;
			if (!value.start) {
				modified = true;
				newValuePtr = otherPtr;
			} else if (otherEmpty) {
				modified = true;
				newValuePtr = otherPtr;
			} else if (value.end - value.start != otherPtr.length
					|| _strnicmp(value.start, otherPtr.txt, value.end - value.start) != 0) {
				modified = true;
				newValuePtr = otherPtr;
			}
		}
		void compareAndUpdateValue(const std::string& other) {
			if (!value.start && other.empty()) return;
			if (!value.start) {
				modified = true;
				newValuePtr = other;
			} else if (other.empty()) {
				modified = true;
				newValuePtr.txt = nullptr;
			// can't have a newValue at this point
			} else if (value.end - value.start != other.size()
					|| _strnicmp(value.start, other.c_str(), value.end - value.start) != 0) {
				modified = true;
				newValuePtr = other;
			}
		}
		void outputWithNewValueIntoFile() {
			DWORD bytesWritten;
			if (value.start) {
				WriteFile(file, line.start, value.start - line.start, &bytesWritten, NULL);
				if (newValuePtr.txt) {
					WriteFile(file, newValuePtr.txt, newValuePtr.length, &bytesWritten, NULL);
				}
				if (line.end > value.end) {
					WriteFile(file, value.end, line.end - value.end, &bytesWritten, NULL);
				}
			} else {
				const char* whereWeEndedSoFar = line.start;
				if (equalSignPtr) {
					WriteFile(file, line.start, equalSignPtr + 1 - line.start, &bytesWritten, NULL);
					char c = ' ';
					WriteFile(file, &c, 1, &bytesWritten, NULL);
					if (newValuePtr.txt) {
						WriteFile(file, newValuePtr.txt, newValuePtr.length, &bytesWritten, NULL);
					}
					whereWeEndedSoFar = equalSignPtr + 1;
				}
				if (comment.start) {
					char c = ' ';
					WriteFile(file, &c, 1, &bytesWritten, NULL);
					WriteFile(file, comment.start, line.end - comment.start, &bytesWritten, NULL);
				} else if (line.end > whereWeEndedSoFar) {
					WriteFile(file, whereWeEndedSoFar, line.end - whereWeEndedSoFar, &bytesWritten, NULL);
				}
			}
		}
	} li;
	li.file = file;
	
	for (auto& it : keyCombosToParse) {
		it.second.isParsed = false;
	}
	
	static bool fieldFoundInFile[offsetof(Settings, settingsMembersEnd) - offsetof(Settings,settingsMembersStart)];
	memset(fieldFoundInFile, 0, sizeof fieldFoundInFile);
	
	enum FieldType {
		FieldType_Boolean,
		FieldType_Int,
		FieldType_ScreenshotPath,
		FieldType_Float
	};
	struct FieldInfo {
		FieldType type;
		bool* fieldFoundInFilePtr;
		void* ptr;
	};
	static std::unordered_map<StringView, FieldInfo, StringViewHash, StringViewCompare> fieldNameToIsFound;
	static bool fieldNameToIsFoundInitialized = false;
	if (!fieldNameToIsFoundInitialized) {
		fieldNameToIsFoundInitialized = true;
		#define bool FieldType_Boolean
		#define int FieldType_Int
		#define ScreenshotPath FieldType_ScreenshotPath
		#define float FieldType_Float
		#define settingsKeyCombo(name, displayName, defaultValue, description) 
		#define settingsField(type, name, defaultValue, displayName, section, description, inlineComment) \
			fieldNameToIsFound[#name] = { \
				type, \
				fieldFoundInFile + offsetof(Settings, name) - offsetof(Settings, settingsMembersStart), \
				(void*)&name \
			};
		#include "SettingsDefinitions.h"
		#undef settingsField
		#undef settingsKeyCombo
		#undef float
		#undef ScreenshotPath
		#undef int
		#undef bool
	}
	
	size_t fieldsFound = 0;
	size_t keyCombosFound = 0;
	
	bool rChar = false;  // even on Windows, by default, we generate the file with \n line breaks
	DWORD fileSize = GetFileSize(file, NULL);
	std::vector<char> wholeFile;
	std::string fieldValue;
	size_t lineCount = 0;
	bool lastLinesEmpty[2] { false };
	bool lastLineEmpty = false;
	
	#define writeNewline WriteFile(file, rn + (1 - rChar), 1 + rChar, &bytesWritten, NULL);
	
	if (fileSize > 0) {
		
		const size_t readSize = 1024;
		wholeFile.resize(fileSize + 1);
		char* wholeFilePtr = wholeFile.data();
		size_t totalBytesRead = 0;
		
		while (true) {
			
			DWORD bytesRead = 0;
			
			size_t bytesToRead;
			// totalBytesRead cannot be greater than fileSize
			if (totalBytesRead == fileSize) break;
			else bytesToRead = fileSize - totalBytesRead;
			
			if (bytesToRead > readSize) bytesToRead = readSize;
			
			if (!ReadFile(file, wholeFilePtr, bytesToRead, &bytesRead, NULL)) {
				WinError winErr;
				logwrap(fprintf(logfile, "Error reading INI file: %ls\n", winErr.getMessage()));
				CloseHandle(file);
				return;
			}
			// bytesRead cannot be greater than bytesToRead
			
			wholeFilePtr += bytesRead;
			totalBytesRead += bytesRead;
			
			if (bytesRead < bytesToRead) {
				break;
			}
		}
		wholeFile[totalBytesRead] = '\0';
		
		SetFilePointer(file, 0, NULL, FILE_BEGIN);
		
		LineReaderFromString lineReader { wholeFile.data() };
		rChar = lineReader.rCharDetected;
		std::vector<int> parsedCombo;
		
		const char* lineStart = nullptr;
		const char* lineEnd = nullptr;
		while (lineReader.readLine(&lineStart, &lineEnd)) {
			++lineCount;
			
			li.key.start = nullptr;
			li.equalSignPtr = nullptr;
			li.newValuePtr.txt = nullptr;
			li.value.start = nullptr;
			li.comment.start = nullptr;
			li.modified = false;
			li.line = { lineStart, lineEnd };
			
			DWORD bytesWritten;
			
			bool skip = false;
			
			if (lineStart == lineEnd) {
				skip = true;
			}
			
			int equalSignPos = -1;
			int commentPos;
			
			if (!skip) {
				commentPos = findMinCommentPos(lineStart, lineEnd);
				equalSignPos = findChar(lineStart, lineEnd, '=');
				if (equalSignPos == -1 || equalSignPos == 0 || commentPos != -1 && equalSignPos != -1 && equalSignPos > commentPos) {
					skip = true;
				}
			}
			
			const char* equalSignPtr = nullptr;
			const char* keyStart = nullptr;
			const char* keyEnd = nullptr;
			
			if (!skip) {
				equalSignPtr = lineStart + equalSignPos;
				keyStart = skipWhitespace(lineStart, equalSignPtr);
				keyEnd = rewindWhitespace(equalSignPtr - 1, lineStart);
				
				if (keyStart > keyEnd) {
					skip = true;
				}
			}
			
			if (!skip) {
				li.equalSignPtr = equalSignPtr;
				li.key = { keyStart, keyEnd + 1 };
				
				const char* valueSearchEnd;
				
				if (commentPos != -1) {
					const char* commentPtr = lineStart + commentPos;
					li.comment.start = commentPtr;
					li.comment.end = lineEnd;
					
					valueSearchEnd = commentPtr;
				} else {
					valueSearchEnd = lineEnd;
				}
				
				if (equalSignPtr < valueSearchEnd - 1) {
					const char* valueStart = skipWhitespace(equalSignPtr + 1, valueSearchEnd);
					const char* valueEnd = rewindWhitespace(valueSearchEnd - 1, equalSignPtr + 1);
					if (valueStart <= valueEnd) {
						li.value.start = valueStart;
						li.value.end = valueEnd + 1;
					}
				}
				
				auto foundKeyCombo = keyCombosToParse.find(li.key);
				if (foundKeyCombo != keyCombosToParse.end()) {
					KeyComboToParse& keyCombo = foundKeyCombo->second;
					if (!keyCombo.isParsed) {
						keyCombo.isParsed = true;
						++keyCombosFound;
					} else {
						li.modified = false;
						li.newValuePtr.txt = nullptr;
					}
					parsedCombo.clear();
					if (li.value.start && !parseKeys(keyCombo.name, li.value.start, li.value.end, parsedCombo)) {
						li.modified = true;
					} else if (compareKeyCombos(*keyCombo.keyCombo, parsedCombo) != 0) {
						li.modified = true;
					}
					if (li.modified) {
						li.newValuePtr = getComboRepresentation(*keyCombo.keyCombo);
						#ifdef LOG_PATH
						std::string keyStr;
						keyStr.assign(li.key.start, li.key.end - li.key.start);
						std::string valueStr;
						valueStr.assign(li.value.start, li.value.end - li.value.start);
						logwrap(fprintf(logfile, "Combo representation for %s: %s\n", keyStr.c_str(), valueStr.c_str()));
						logwrap(fputs("Combo contains codes: ", logfile));
						for (int code : *keyCombo.keyCombo) {
							logwrap(fprintf(logfile, "%d ", code));
						}
						logwrap(fprintf(logfile, "\n"));
						#endif
					}
				} else {
					auto foundField = fieldNameToIsFound.find(li.key);
					if (foundField != fieldNameToIsFound.end()) {
						FieldInfo& fi = foundField->second;
						if (!*fi.fieldFoundInFilePtr) {
							*fi.fieldFoundInFilePtr = true;
							++fieldsFound;
						} else {
							li.modified = false;
							li.newValuePtr.txt = nullptr;
						}
						const std::string* fieldValuePtr = nullptr;
						switch (fi.type) {
							case FieldType_Boolean: li.compareAndUpdateValue(formatBoolean(*(std::atomic_bool*)fi.ptr)); break;
							case FieldType_Int: formatInteger(*(std::atomic_int*)fi.ptr, fieldValue); fieldValuePtr = &fieldValue; break;
							case FieldType_ScreenshotPath: fieldValuePtr = &screenshotPath; break;
							case FieldType_Float: formatFloat(*(float*)fi.ptr, fieldValue); fieldValuePtr = &fieldValue; break;
						}
						if (fieldValuePtr) {
							li.compareAndUpdateValue(*fieldValuePtr);
						}
					}
				}
			}
			
			if (lineCount != 1) writeNewline
			if (li.modified) {
				li.outputWithNewValueIntoFile();
			} else {
				WriteFile(file, lineStart, lineEnd - lineStart, &bytesWritten, NULL);
			}
			lastLineEmpty = isWhitespace(lineStart, lineEnd);
			lastLinesEmpty[0] = lastLinesEmpty[1];
			lastLinesEmpty[1] = lastLineEmpty;
		}
		
	}
	
	if (!lineCount) {
		DWORD bytesWritten;
		SplitStringIterator splitIter(settingsTopComment, settingsTopComment + strlen(settingsTopComment), '\n');
		StringView line;
		while (splitIter.getNext(&line.start, &line.end)) {
			WriteFile(file, line.start, line.end - line.start, &bytesWritten, NULL);
			writeNewline
		}
		writeNewline
	}
	
	if ((keyCombosFound < keyCombosToParse.size() || fieldsFound < fieldNameToIsFound.size())
			&& lineCount && (!lastLinesEmpty[0] || !lastLinesEmpty[1])) {
		DWORD bytesWritten;
		if (!lastLinesEmpty[0]) writeNewline
		if (!lastLinesEmpty[1]) writeNewline
	}
	
	struct {
		StringWithLength name;
		StringWithLength newValuePtr;
		StringWithLength iniDescription;
		StringWithLength inlineComment;
		bool rChar = false;
		HANDLE file = NULL;
		bool isFirst = true;
		std::vector<std::string> printedNames;
		void outputToFile() {
			std::string nameAsString(name.txt, name.length);
			printedNames.push_back(nameAsString);
			DWORD bytesWritten;
			
			if (!isFirst) {
				if (rChar) {
					WriteFile(file, "\r\n\r\n", 4, &bytesWritten, NULL);
				} else {
					WriteFile(file, "\n\n", 2, &bytesWritten, NULL);
				}
			} else {
				isFirst = false;
			}
			
			if (iniDescription.txt) {
				SplitStringIterator splitter(iniDescription.txt, iniDescription.txt + iniDescription.length, '\n');
				StringView line;
				while (splitter.getNext(&line.start, &line.end)) {
					WriteFile(file, line.start, line.end - line.start, &bytesWritten, NULL);
					WriteFile(file, rn + 1 - rChar, 1 + rChar, &bytesWritten, NULL);
				}
			}
			WriteFile(file, name.txt, name.length, &bytesWritten, NULL);
			WriteFile(file, " = ", 3, &bytesWritten, NULL);
			if (newValuePtr.txt) {
				WriteFile(file, newValuePtr.txt, newValuePtr.length, &bytesWritten, NULL);
			}
			if (inlineComment.txt) {
				if (newValuePtr.txt) {
					char c = ' ';
					WriteFile(file, &c, 1, &bytesWritten, NULL);
				}
				WriteFile(file, inlineComment.txt, inlineComment.length, &bytesWritten, NULL);
			}
		}
	} nl;
	nl.rChar = rChar;
	nl.file = file;
	
	KeyComboToParse* keyCombo;
	
	#define screenshotPathPreset(fieldName, fieldInlineComment) \
		if (!fieldFoundInFile[offsetof(Settings, fieldName) - offsetof(Settings, settingsMembersStart)]) { \
			std::string fieldName##Cpy; \
			{ \
				std::unique_lock<std::mutex> screenshotGuard(screenshotPathMutex); \
				fieldName##Cpy = screenshotPath; \
			} \
			logwrap(fprintf(logfile, "Writing screenshot path: %s\n", screenshotPath.c_str())); \
			nl.name = StringWithLength { #fieldName }; \
			nl.newValuePtr = fieldName##Cpy; \
			nl.iniDescription = getOtherINIDescription(&fieldName); \
			nl.inlineComment = StringWithLength { fieldInlineComment }; \
			nl.outputToFile(); \
			nl.inlineComment.txt = nullptr; \
		}
	#define keyComboPreset(keyComboName) \
		keyCombo = &getKeyComboToParseByOffset(keyComboName); \
		if (!keyCombo->isParsed) { \
			nl.name = StringWithLength { #keyComboName }; \
			nl.newValuePtr = getComboRepresentation(keyComboName); \
			nl.iniDescription = keyCombo->iniDescription; \
			nl.outputToFile(); \
		}
	#define booleanPreset(fieldName, fieldInlineComment) \
		if (!fieldFoundInFile[offsetof(Settings, fieldName) - offsetof(Settings, settingsMembersStart)]) { \
			nl.name = StringWithLength { #fieldName }; \
			nl.newValuePtr = formatBoolean(fieldName); \
			nl.iniDescription = getOtherINIDescription(&fieldName); \
			nl.outputToFile(); \
		}
	#define integerPreset(fieldName, fieldInlineComment) \
		if (!fieldFoundInFile[offsetof(Settings, fieldName) - offsetof(Settings, settingsMembersStart)]) { \
			nl.name = StringWithLength { #fieldName }; \
			formatInteger(fieldName, fieldValue); \
			nl.newValuePtr = fieldValue; \
			nl.iniDescription = getOtherINIDescription(&fieldName); \
			nl.outputToFile(); \
		}
	#define floatPreset(fieldName, fieldInlineComment) \
		if (!fieldFoundInFile[offsetof(Settings, fieldName) - offsetof(Settings, settingsMembersStart)]) { \
			nl.name = StringWithLength { #fieldName }; \
			formatFloat(fieldName, fieldValue); \
			nl.newValuePtr = fieldValue; \
			nl.iniDescription = getOtherINIDescription(&fieldName); \
			nl.outputToFile(); \
		}
	#define int integerPreset
	#define float floatPreset
	#define ScreenshotPath screenshotPathPreset
	#define bool booleanPreset
	#define settingsKeyCombo(name, displayName, defaultValue, description) keyComboPreset(name)
	#define settingsField(type, fieldName, defaultValue, displayName, section, description, inlineComment) type(fieldName, inlineComment)
	#include "SettingsDefinitions.h"
	#undef settingsField
	#undef settingsKeyCombo
	#undef bool
	#undef ScreenshotPath
	#undef float
	#undef int
	#undef floatPreset
	#undef integerPreset
	#undef booleanPreset
	#undef keyComboPreset
	#undef screenshotPathPreset
	
	if (!nl.isFirst) {
		DWORD bytesWritten;
		WriteFile(file, rn + 1 - rChar, 1 + rChar, &bytesWritten, NULL);
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
	settings.changesListenerStarted = true;
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

bool Settings::isWhitespace(const char* strStart, const char* strEnd) {
	for (const char* c = strStart; c != strEnd; ++c) {
		if (*c > 32) return false;
	}
	return true;
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

void Settings::getComboInfo(const std::vector<int>& keyCombo, ComboInfo* info) const {
	for (auto it = keyCombosToParse.cbegin(); it != keyCombosToParse.cend(); ++it) {
		const KeyComboToParse& combo = it->second;
		if (combo.keyCombo == &keyCombo) {
			info->uiName = combo.uiName;
			info->uiNameWithLength = { info->uiName, strlen(info->uiName) };
			info->uiDescription = combo.uiDescription.c_str();
			info->uiDescriptionWithLength = { info->uiDescription, combo.uiDescription.size() };
			return;
		}
	}
	info->uiName = info->uiDescription = "";
}

const char* Settings::getOtherUIName(void* ptr) {
	return pointerIntoSettingsIntoDescription[(BYTE*)ptr - (BYTE*)this - offsetof(Settings, settingsMembersStart)]->uiName;
}

StringWithLength Settings::getOtherUINameWithLength(void* ptr) {
	return pointerIntoSettingsIntoDescription[(BYTE*)ptr - (BYTE*)this - offsetof(Settings, settingsMembersStart)]->uiNameWithLength;
}

const char* Settings::getOtherUIFullName(void* ptr) {
	return pointerIntoSettingsIntoDescription[(BYTE*)ptr - (BYTE*)this - offsetof(Settings, settingsMembersStart)]->uiFullPath.c_str();
}

const char* Settings::getOtherUIDescription(void* ptr) {
	return pointerIntoSettingsIntoDescription[(BYTE*)ptr - (BYTE*)this - offsetof(Settings, settingsMembersStart)]->uiDescription.c_str();
}

StringWithLength Settings::getOtherUIDescriptionWithLength(void* ptr) {
	const std::string& str = pointerIntoSettingsIntoDescription[(BYTE*)ptr - (BYTE*)this - offsetof(Settings, settingsMembersStart)]->uiDescription;
	return { str.c_str(), str.size() };
}

StringWithLength Settings::getOtherINIDescription(void* ptr) {
	return pointerIntoSettingsIntoDescription[(BYTE*)ptr - (BYTE*)this - offsetof(Settings, settingsMembersStart)]->iniDescription;
}

void Settings::registerOtherDescription(void* ptr, const char* iniName, const char* uiName, const char* uiPath, StringWithLength iniDescription) {
	otherDescriptions.emplace_back();
	OtherDescription& desc = otherDescriptions.back();
	desc.ptr = ptr;
	desc.iniName = iniName;
	desc.uiName = uiName;
	desc.uiNameWithLength = { uiName, strlen(uiName) };
	desc.uiFullPath.reserve(1 + strlen(uiPath) + 3 + strlen(uiName) + 1);
	desc.uiFullPath = '\'';
	desc.uiFullPath += uiPath;
	desc.uiFullPath += " - ";
	desc.uiFullPath += uiName;
	desc.uiFullPath += '\'';
	desc.iniDescription = iniDescription;
}

static const char* convertToUiDescription_lookaheadPart(const char* mainPtr) {
	const char* ptr = mainPtr + 1;
	while (*ptr == '\n' && *ptr != '\0') {
		++ptr;
	}
	if (*ptr != ';') return nullptr;
	if (*++ptr != ' ') return nullptr;
	char c = *++ptr;
	if (!(c >= 'a' && c <= 'z')) return nullptr;
	return ptr - 1;
}

std::string Settings::convertToUiDescription(const char* iniDescription) {
	std::string result;
	if (*iniDescription != '\0') {
		
		const char* const iniDescriptionInitialValue = iniDescription;
		std::string iniNameLookup;
		std::vector<const char*> mentionedFullPaths;
		result.reserve(strlen(iniDescription));
		
		enum ParseMode {
			PARSING_NEWLINE,
			PARSING_SEMICOLON,
			PARSING_WHITESPACE,
			PARSING_QUOTATION
		} parsingMode = PARSING_NEWLINE;
		
		const char* quotationStart = nullptr;
		
		for (; ; ++iniDescription) {
			char cVal = *iniDescription;
			if (cVal == '\0') break;
			if (cVal == ';') {
				if (parsingMode == PARSING_NEWLINE) {
					parsingMode = PARSING_SEMICOLON;
					continue;
				} else if (parsingMode == PARSING_QUOTATION) {
					result.append(quotationStart, iniDescription - quotationStart);
				}
				parsingMode = PARSING_WHITESPACE;
			} else if (cVal == '\n') {
				if (parsingMode == PARSING_QUOTATION) {
					result.append(quotationStart, iniDescription - quotationStart);
				}
				parsingMode = PARSING_NEWLINE;
				if (iniDescription != iniDescriptionInitialValue) {
					char p = *(iniDescription - 1);
					if (p >= 'a' && p <= 'z'
							|| p == ','
							|| p >= '0' && p <= '9') {
						const char* reply = convertToUiDescription_lookaheadPart(iniDescription);
						if (reply) {
							iniDescription = reply;
							cVal = *reply;
							parsingMode = PARSING_WHITESPACE;
						}
					}
				}
			} else if (cVal <= 32) {
				if (parsingMode == PARSING_SEMICOLON) {
					parsingMode = PARSING_WHITESPACE;
					continue;
				} else if (parsingMode == PARSING_QUOTATION) {
					result.append(quotationStart, iniDescription - quotationStart);
				}
				parsingMode = PARSING_WHITESPACE;
			} else if (cVal == '"') {
				if (parsingMode == PARSING_WHITESPACE || parsingMode == PARSING_NEWLINE) {
					parsingMode = PARSING_QUOTATION;
					quotationStart = iniDescription;
					continue;
				} else if (parsingMode == PARSING_QUOTATION) {
					iniNameLookup.assign(quotationStart + 1, iniDescription - quotationStart - 1);
					auto found = iniNameToUiNameMap.find(iniNameLookup.c_str());
					if (found == iniNameToUiNameMap.end()) {
						result.append(quotationStart, iniDescription - quotationStart + 1);
					} else {
						if (std::find(mentionedFullPaths.begin(), mentionedFullPaths.end(), found->first) != mentionedFullPaths.end()) {
							result += '\'';
							result += found->second.name;
							result += '\'';
						} else {
							result += found->second.fullName;
							mentionedFullPaths.push_back(found->first);
						}
					}
					parsingMode = PARSING_WHITESPACE;
					continue;
				}
				parsingMode = PARSING_WHITESPACE;
			} else if (parsingMode == PARSING_QUOTATION) {
				continue;
			} else {
				parsingMode = PARSING_WHITESPACE;
			}
			result += cVal;
		}
		if (parsingMode == PARSING_QUOTATION) {
			result += quotationStart;
		}
	}
	return result;
}

int Settings::hashString(const char* str, int startingHash) {
	for (const char* c = str; *c != '\0'; ++c) {
		char lowercaseChar = tolower(*c);
		startingHash = startingHash * 0x89 + lowercaseChar;
	}
	return startingHash;
}

int Settings::hashString(const char* strStart, const char* strEnd, int startingHash) {
	for (const char* c = strStart; c != strEnd; ++c) {
		char lowercaseChar = tolower(*c);
		startingHash = startingHash * 0x89 + lowercaseChar;
	}
	return startingHash;
}

Settings::KeyComboToParse::KeyComboToParse(size_t keyLength, const char* name, const char* uiName, std::vector<int>* keyCombo, const StringWithLength& defaultValue, const StringWithLength& iniDescription)
	: keyLength(keyLength), name(name), uiName(uiName), keyCombo(keyCombo), defaultValue(defaultValue), iniDescription(iniDescription) { }

int Settings::findCharRev(const char* buf, char c) {
	const char* ptr = buf;
	while (*ptr != '\0') {
		++ptr;
	}
	while (ptr != buf) {
		--ptr;
		if (*ptr == c) return ptr - buf;
	}
	return -1;
}

float Settings::parseFloat(const char* inputString, bool* error) {
	if (error) *error = false;
	float result;
	int returnValue = sscanf(inputString, "%f", &result);
	if (returnValue == EOF || returnValue == 0 || returnValue == -1) {
		if (error) *error = true;
		return 0.F;
	}
	return result;
}

void Settings::formatInteger(int d, std::string& result) {
	static const char* formatString = "%d";
	
	result.assign(
		snprintf(nullptr, 0, formatString, d),
		'\0');
	
	if (result.empty()) {
		return;
	}
	
	snprintf(&result.front(), result.size() + 1, formatString, d);
}

void Settings::formatFloat(float f, std::string& result) {
	static const char* formatString = "%.4f";
	
	result.assign(
		snprintf(nullptr, 0, formatString, f),
		'\0');
	
	if (result.empty()) {
		return;
	}
	
	auto firstDotPos = result.end();
	auto firstNonZeroPos = result.end();
	const auto lastCharPos = result.begin() + (result.size() - 1);
	
	snprintf(&result.front(), result.size() + 1, formatString, f);
	
	// goal: remove repeating '0's after '.': 6.00000000000000000
	for (auto it = lastCharPos; ; ) {
		char c = *it;
		
		if (c == '.') {
			firstDotPos = it;
			break;
		}
		if (c != '0' && firstNonZeroPos == result.end()) {
			if (it == lastCharPos) return;
			firstNonZeroPos = it;
		}
		if (it == result.begin()) break;
		--it;
	}
	if (firstDotPos == result.end()) {
		return;
	}
	std::string::iterator firstZeroPos;
	if (firstNonZeroPos == result.end()) {
		firstZeroPos = firstDotPos + 1;
	} else {
		firstZeroPos = firstNonZeroPos + 1;
	}
	if (firstZeroPos == firstDotPos + 1) {
		if (firstZeroPos == lastCharPos) {
			return;
		}
		++firstZeroPos;
	}
	result.erase(firstZeroPos, result.end());
}
