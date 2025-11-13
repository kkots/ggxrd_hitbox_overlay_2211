#pragma once
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <mutex>
#include "StringWithLength.h"
#include "HandleWrapper.h"
#include "characterTypes.h"
#include "Moves.h"
#include "PinnedWindowList.h"

const int MOUSE_MOVE_LEFT = 0x101;
const int MOUSE_MOVE_UP = 0x102;
const int MOUSE_MOVE_RIGHT = 0x103;
const int MOUSE_MOVE_DOWN = 0x104;
const int MOUSE_WHEEL_UP = 0x105;
const int MOUSE_WHEEL_DOWN = 0x106;
const int JOY_START = 0x107;
const int JOY_BTN_0 = 0x107;
const int JOY_BTN_1 = 0x108;
const int JOY_BTN_2 = 0x109;
const int JOY_BTN_3 = 0x10a;
const int JOY_BTN_4 = 0x10b;
const int JOY_BTN_5 = 0x10c;
const int JOY_BTN_6 = 0x10d;
const int JOY_BTN_7 = 0x10e;
const int JOY_BTN_8 = 0x10f;
const int JOY_BTN_9 = 0x110;
const int JOY_BTN_10 = 0x111;
const int JOY_BTN_11 = 0x112;
const int JOY_BTN_12 = 0x113;
const int JOY_BTN_13 = 0x114;
const int JOY_BTN_14 = 0x115;
const int JOY_BTN_15 = 0x116;
const int JOY_LEFT_STICK_LEFT = 0x117;  // lX, lY
const int JOY_LEFT_STICK_UP = 0x118;
const int JOY_LEFT_STICK_RIGHT = 0x119;
const int JOY_LEFT_STICK_DOWN = 0x11a;
const int JOY_DPAD_LEFT = 0x11b;
const int JOY_DPAD_UP = 0x11c;
const int JOY_DPAD_RIGHT = 0x11d;
const int JOY_DPAD_DOWN = 0x11e;
const int JOY_PS4_DUALSHOCK_RIGHT_STICK_LEFT = 0x11f;  // lZ, lRz
const int JOY_PS4_DUALSHOCK_RIGHT_STICK_UP = 0x120;
const int JOY_PS4_DUALSHOCK_RIGHT_STICK_RIGHT = 0x121;
const int JOY_PS4_DUALSHOCK_RIGHT_STICK_DOWN = 0x122;
const int JOY_XBOX_TYPE_S_RIGHT_STICK_LEFT = 0x123;  // lRx, lRy
const int JOY_XBOX_TYPE_S_RIGHT_STICK_UP = 0x124;
const int JOY_XBOX_TYPE_S_RIGHT_STICK_RIGHT = 0x125;
const int JOY_XBOX_TYPE_S_RIGHT_STICK_DOWN = 0x126;
// inclusive
const int JOY_END = 0x126;

struct MoveListPointer {
	CharacterType charType;
	const char* name;
	bool red = false;
	bool green = false;
	bool blue = false;
	inline MoveListPointer(CharacterType charType, const char* name, bool red, bool green, bool blue)
		: charType(charType), name(name), red(red), green(green), blue(blue) { }
};

struct MoveList {
	std::vector<MoveListPointer> pointers;
	unsigned int year = 0;
	unsigned int month = 0;
	unsigned int day = 0;
	unsigned int hour = 0;
	unsigned int minute = 0;
	unsigned int second = 0;
};

struct HitboxListElement {
	DWORD color;  // 0xAARRGGBB
	bool show;
};

struct HitboxList {
	HitboxListElement elements[17];
	HitboxList() = default;
	HitboxList(const char* str);
	// returns the number of items parsed
	size_t parse(const char* str);
	inline HitboxListElement& operator[](int index) { return elements[index]; }
	inline const HitboxListElement& operator[](int index) const { return elements[index]; }
};

class Settings
{
public:
	bool onDllMain();
	void onDllDetach();
	struct Key {
		const char* name = nullptr;
		const char* uiName = nullptr;
		int code = 0;
	};
	std::mutex keyCombosMutex;
	struct StringView {
		StringView() = default;
		StringView(const char* start, const char* end) : start(start), end(end) { }
		StringView(const StringWithLength& other) : start(other.txt), end(other.txt + other.length) { }
		StringView(const std::string& str) :
			start(str.empty() ? nullptr : str.c_str()),
			end(str.empty() ? nullptr : str.c_str() + str.size()) { }
		template<size_t size> inline StringView(const char(&array)[size]) : start(array), end(array + size - 1) { }
		const char* start = nullptr;
		const char* end = nullptr;
		inline int length() const { return end - start; }
	};
	struct StringViewHash {
		inline std::size_t operator()(const StringView& k) const {
			return hashString(k.start, k.end);
		}
	};
	struct StringViewCompare {
		inline bool operator()(const StringView& k, const StringView& other) const {
			if (k.end - k.start != other.end - other.start) return false;
			return _strnicmp(k.start, other.start, k.end - k.start) == 0;
		}
	};
	struct StringViewLess {
		inline bool operator()(const StringView& k, const StringView& other) const {
			const char* kPtr = k.start;
			const char* otherPtr = other.start;
			while (true) {
				if (kPtr >= k.end) {
					if (otherPtr >= other.end) return false;  // strings are equal
					return true;
				} else if (otherPtr >= other.end) {
					return false;
				} else {
					int diff = toupper(*kPtr) - toupper(*otherPtr);
					if (diff < 0) {
						return true;
					} else if (diff > 0) {
						return false;
					} else {  // diff == 0
						++kPtr;
						++otherPtr;
						continue;
					}
				}
						
			}
		}
	};
	std::map<StringView, Key, StringViewLess> keys;  // string to int code
	std::vector<Key*> reverseKeys; // int code to string
	static const char* const SETTINGS_HITBOX;
	static const char* const SETTINGS_HITBOX_SETTINGS;
	static const char* const SETTINGS_GENERAL;
	static const char* const SETTINGS_FRAMEBAR;
	static const char* const SETTINGS_COMBO_RECIPE;
	static const char* const SETTINGS_CHARACTER_SPECIFIC;
	static const char* const SETTINGS_HITBOX_EDITOR;
	bool keyCombosBegin;
	#define settingsKeyCombo(name, displayName, defaultValue, description) std::vector<int> name;
	#define settingsField(type, name, defaultValue, displayName, section, description, inlineComment)
	#include "SettingsDefinitions.h"
	#undef settingsField
	#undef settingsKeyCombo
	bool keyCombosEnd;
	
	std::mutex screenshotPathMutex;
	bool settingsMembersStart = false;  // make sure all settings are contained between this and settingsMembersEnd
	typedef std::string ScreenshotPath;
	#define settingsKeyCombo(name, displayName, defaultValue, description) 
	#define settingsField(type, name, defaultValue, displayName, section, description, inlineComment) type name = defaultValue;
	#include "SettingsDefinitions.h"
	#undef settingsField
	#undef settingsKeyCombo
	bool settingsMembersEnd = false;
	const float cameraCenterOffsetX_defaultValue = cameraCenterOffsetX;
	const float cameraCenterOffsetY_defaultValue = cameraCenterOffsetY;
	const float cameraCenterOffsetY_WhenForcePitch0_defaultValue = cameraCenterOffsetY_WhenForcePitch0;
	const float cameraCenterOffsetZ_defaultValue = cameraCenterOffsetZ;
	const char* getKeyRepresentation(int code);
	void readSettings(bool isFirstEverRead);
	void writeSettings();
	struct ComboInfo {
		const char* uiName = nullptr;
		StringWithLength uiNameWithLength;
		const char* uiDescription = nullptr;
		StringWithLength uiDescriptionWithLength;
	};
	void onKeyCombosUpdated();
	void getComboInfo(const std::vector<int>& keyCombo, ComboInfo* info) const;
	const char* getOtherUIName(void* ptr);
	StringWithLength getOtherUINameWithLength(void* ptr);
	const char* getOtherUIFullName(void* ptr);
	const char* getOtherUIDescription(void* ptr);
	StringWithLength getOtherUIDescriptionWithLength(void* ptr);
	StringWithLength getOtherINIDescription(void* ptr);
	std::string convertToUiDescription(const char* iniDescription);
	StringWithLength getComboRepresentation(const std::vector<int>& toggle);
	StringWithLength getComboRepresentationUserFriendly(const std::vector<int>& toggle);
	inline int getMaxKeyCode() const {
		return maxKeyCode;
	}
	// includes trailing slash
	const std::wstring& getSettingsPath() const { return settingsPathFolder; }
	void setupSettingsPathFolder();
private:
	struct KeyComboToParse {
		size_t keyLength = 0;
		const char* name = nullptr;
		const char* uiName = nullptr;
		std::string uiFullName;
		std::vector<int>* keyCombo = nullptr;
		StringWithLength defaultValue;
		StringWithLength iniDescription;
		std::string uiDescription;
		bool isParsed = false;
		bool representationGenerated = false;
		bool representationUserFriendlyGenerated = false;
		std::string representation;
		std::string representationUserFriendly;
		void generateRepresentation();
		void generateRepresentationUserFriendly();
		KeyComboToParse(size_t keyLength, const char* name, const char* uiName, std::vector<int>* keyCombo, const StringWithLength& defaultValue, const StringWithLength& iniDescription);
	};
	std::unordered_map<StringView, KeyComboToParse, StringViewHash, StringViewCompare> keyCombosToParse;
	std::vector<KeyComboToParse*> offsetToKeyComboToParse;
	inline size_t offsetToKeyComboIndex(const std::vector<int>& data) const {
		return offsetToKeyComboIndex((const void*)&data);
	}
	inline size_t offsetToKeyComboIndex(const void* data) const {
		return (
			(BYTE*)data - (BYTE*)&keyCombosBegin
		) / sizeof (std::vector<int>);
	}
	inline KeyComboToParse& getKeyComboToParseByOffset(const std::vector<int>& data) {
		return *offsetToKeyComboToParse[offsetToKeyComboIndex(data)];
	}
	inline const KeyComboToParse& getKeyComboToParseByOffset(const std::vector<int>& data) const {
		return *offsetToKeyComboToParse[offsetToKeyComboIndex(data)];
	}
	struct MyKey {
		const char* str;
	};
	static int hashString(const char* str, int startingHash = 0);
	static int hashString(const char* strStart, const char* strEnd, int startingHash = 0);
	struct MyHashFunction {
		inline std::size_t operator()(const char* key) const {
			return hashString(key);
		}
	};
	struct IniNameToUiNameMapElement {
		const char* fullName;
		const char* name;
	};
	struct MyCompareFunction {
		inline bool operator()(const char* key, const char* other) const {
			return key == other || _stricmp(key, other) == 0;
		}
	};
	std::unordered_map<const char*, IniNameToUiNameMapElement, MyHashFunction, MyCompareFunction> iniNameToUiNameMap;
	void insertKeyComboToParse(const char* name, const char* uiName, std::vector<int>* keyCombo, const StringWithLength& defaultValue, const StringWithLength& iniDescription);
	void addKey(const char* name, const char* uiName, int code);
	static int findMinCommentPos(const char* buf);
	static int findMinCommentPos(const char* bufStart, const char* bufEnd);
	static void parseKeyName(const char* buf, StringView& result);
	static void getKeyValue(const char* buf, std::string& result);
	void addKeyRange(const char* str);
	static int findChar(const char* buf, char c, int startingPos = 0);
	static int findChar(const char* bufStart, const char* bufEnd, char c, int startingPos = 0);
	static int findCharRev(const char* buf, char c);
	static int findCharRevW(const wchar_t* buf, wchar_t c);
	static std::pair<int, int> trim(std::string& str); // Trims left and right in-place. Returns how many chars were cut off from left (.first) and from right (.second).
	static std::vector<std::string> split(const std::string& str, char c);
	inline bool parseKeys(const char* keyName, const StringWithLength& keyValue, std::vector<int>& keyCodes) {
		return parseKeys(keyName, keyValue.txt, keyValue.txt + keyValue.length, keyCodes);
	}
	inline bool parseKeys(const char* keyName, const std::string& keyValue, std::vector<int>& keyCodes) {
		return parseKeys(keyName, keyValue.c_str(), keyValue.c_str() + keyValue.size(), keyCodes);
	}
	bool parseKeys(const char* keyName, const char* keyValueStart, const char* keyValueEnd, std::vector<int>& keyCodes);
	static bool parseInteger(const char* keyName, const std::string& keyValue, int& integer);
	static bool parseBoolean(const char* keyName, const std::string& keyValue, bool& aBooleanValue);
	static const StringWithLength& formatBoolean(bool value);
	static bool parseFloat(const char* keyName, const std::string& keyValue, float& floatValue);
	static float parseFloat(const char* inputString, bool* error = nullptr);
	static bool parseMoveList(const char* keyName, const std::string& keyValue, MoveList& listValue);
	static bool parseHitboxList(const char* keyName, const std::string& keyValue, HitboxList& listValue);
	static bool parsePinnedWindowList(const char* keyName, const std::string& keyValue, PinnedWindowList& listValue);
	static void formatFloat(float f, std::string& result);
	static void formatInteger(int f, std::string& result);
	static void formatMoveList(const MoveList& moveList, std::string& result);
	static void formatHitboxList(const HitboxList& hitboxList, std::string& result);
	static void formatPinnedWindowList(const PinnedWindowList& moveList, std::string& result);
	void registerListenerForChanges();
	std::wstring settingsPathFolder;
	std::wstring settingsPath;
	bool firstSettingsParse = true;
	HANDLE directoryChangeHandle = NULL;
	bool lastCallFailedToGetTime = false;
	HandleWrapper changesListener = NULL;
	bool changesListenerStarted = false;
	HandleWrapper changesListenerWakeEvent = NULL;
	HandleWrapper changeListenerExitedEvent = NULL;
	enum ChangesListenerWakeType {
		WAKE_TYPE_EXIT,
		WAKE_TYPE_WRITING_FILE
	} changesListenerWakeType;
	static DWORD WINAPI changesListenerLoop(LPVOID lpThreadParameter);
	void writeSettingsMain();
	static bool isWhitespace(const char* str);
	static bool isWhitespace(const char* strStart, const char* strEnd);
	static int compareKeyCombos(const std::vector<int>& left, const std::vector<int>& right);
	const char* getKeyTxtName(int code);
	void trashComboRepresentation(std::vector<int>& toggle);
	struct OtherDescription {
		void* ptr = nullptr;
		const char* iniName = nullptr;
		const char* uiName = nullptr;
		StringWithLength uiNameWithLength;
		std::string uiFullPath;
		StringWithLength iniDescription;
		std::string uiDescription;
	};
	std::vector<OtherDescription> otherDescriptions;
	void registerOtherDescription(void* ptr, const char* iniName, const char* uiName, const char* uiPath, StringWithLength iniDescription);
	std::vector<OtherDescription*> pointerIntoSettingsIntoDescription;
	std::unordered_map<StringView, DWORD, StringViewHash, StringViewCompare> settingNameToOffset;  // case-insensitive
	static inline const char* rewindWhitespace(const char* ptr, const char* strStart) {
		while (ptr != strStart && *ptr <= 32) --ptr;
		return ptr;
	}
	static inline const char* skipWhitespace(const char* ptr, const char* strEnd) {
		while (ptr != strEnd && *ptr <= 32) ++ptr;
		return ptr;
	}
	static inline char* rewindWhitespace(char* ptr, char* strStart) {
		while (ptr != strStart && *ptr <= 32) --ptr;
		return ptr;
	}
	static inline char* skipWhitespace(char* ptr, char* strEnd) {
		while (ptr != strEnd && *ptr <= 32) ++ptr;
		return ptr;
	}
	int maxKeyCode = INT_MIN;
};

extern Settings settings;
