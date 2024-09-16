#pragma once
#include <vector>
#include <string>
#include <map>
#include <mutex>
#include <atomic>

class Settings
{
public:
	bool onDllMain();
	void onDllDetach();
	void readSettingsIfChanged();
	struct Key {
		const char* name = nullptr;
		const char* uiName = nullptr;
		int code = 0;
	};
	std::mutex keyCombosMutex;
	std::map<std::string, Key> keys;  // string to int code
	std::map<int, Key*> reverseKeys; // int code to string
	std::vector<int> gifModeToggle;
	std::vector<int> noGravityToggle;
	std::vector<int> freezeGameToggle;
	std::vector<int> slowmoGameToggle;
	std::vector<int> allowNextFrameKeyCombo;
	std::vector<int> disableModKeyCombo;
	std::vector<int> disableHitboxDisplayToggle;
	std::vector<int> continuousScreenshotToggle;
	std::vector<int> gifModeToggleBackgroundOnly;
	std::vector<int> gifModeToggleCameraCenterOnly;
	std::vector<int> gifModeToggleHideOpponentOnly;
	std::vector<int> gifModeToggleHudOnly;
	std::vector<int> screenshotBtn;
	std::vector<int> modWindowVisibilityToggle;
	std:: mutex screenshotPathMutex;
	std::string screenshotPath;
	std::atomic_bool allowContinuousScreenshotting = false;
	std::atomic_bool dontUseScreenshotTransparency = false;
	std::atomic_int slowmoTimes = 3;
	std::atomic_bool startDisabled = false;
	std::atomic_bool drawPushboxCheckSeparately = true;
	std::atomic_bool modWindowVisibleOnStart = true;
	const char* getKeyRepresentation(int code);
	void readSettings(bool dontReadIfDoesntExist);
	void writeSettings();
	struct ComboInfo {
		const char* uiName = nullptr;
		const char* uiDescription = nullptr;
	};
	void onKeyCombosUpdated();
	void getComboInfo(std::vector<int>& keyCombo, ComboInfo* info);
	const char* getOtherUIDescription(void* ptr);
	const char* getOtherINIDescription(void* ptr);
private:
	struct KeyComboToParse {
		const char* name = nullptr;
		const char* uiName = nullptr;
		std::vector<int>* keyCombo = nullptr;
		const char* defaultValue = nullptr;
		const char* iniDescription = nullptr;
		std::string uiDescription;
		bool isParsed = false;
		bool representationGenerated = false;
		std::string representation;
		void generateRepresentation();
	};
	std::map<std::string, KeyComboToParse> keyCombosToParse;
	void insertKeyComboToParse(const char* name, const char* uiName, std::vector<int>* keyCombo, const char* defaultValue, const char* iniDescription);
	void addKey(const char* name, const char* uiName, int code);
	int findMinCommentPos(const char* buf) const;
	std::string parseKeyName(const char* buf) const;
	std::string getKeyValue(const char* buf) const;
	void addKeyRange(char start, char end);
	int findChar(const char* buf, char c, int startingPos = 0) const;
	std::pair<int, int> trim(std::string& str) const; // Trims left and right in-place. Returns how many chars were cut off from left (.first) and from right (.second).
	std::string toUppercase(const std::string& str) const;
	std::vector<std::string> split(const std::string& str, char c) const;
	bool parseKeys(const char* keyName, const std::string& keyValue, std::vector<int>& keyCodes);
	bool parseInteger(const char* keyName, const std::string& keyValue, std::atomic_int& integer);
	std::string formatInteger(int value);
	bool parseBoolean(const char* keyName, const std::string& keyValue, std::atomic_bool& aBooleanValue);
	const char* formatBoolean(bool value);
	std::wstring getCurrentDirectory();
	void registerListenerForChanges();
	std::wstring settingsPath;
	bool firstSettingsParse = true;
	HANDLE directoryChangeHandle = NULL;
	bool lastCallFailedToGetTime = false;
	HANDLE changesListener = NULL;
	HANDLE changesListenerWakeEvent = NULL;
	HANDLE changeListenerExitedEvent = NULL;
	enum ChangesListenerWakeType {
		WAKE_TYPE_EXIT,
		WAKE_TYPE_WRITING_FILE
	} changesListenerWakeType;
	static DWORD WINAPI changesListenerLoop(LPVOID lpThreadParameter);
	void writeSettingsMain();
	bool isWhitespace(const char* str);
	int compareKeyCombos(const std::vector<int>& left, const std::vector<int>& right);
	const char* getComboRepresentation(std::vector<int>& toggle);
	const char* getKeyTxtName(int code);
	void trashComboRepresentation(std::vector<int>& toggle);
	struct OtherDescription {
		void* ptr = nullptr;
		const char* iniDescription = nullptr;
		std::string uiDescription;
	};
	std::vector<OtherDescription> otherDescriptions;
	void registerOtherDescription(void* ptr, const char* iniDescription);
	std::string convertToUiDescription(const char* iniDescription);
};

extern Settings settings;
