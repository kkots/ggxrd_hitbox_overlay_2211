#pragma once
#include <vector>
#include <string>
#include <map>
#include <mutex>
#include <atomic>
#include <thread>

BOOL CALLBACK EnumWindowsFindMyself(HWND hwnd, LPARAM lParam);

class Settings
{
public:
	bool onDllMain();
	void onDllDetach();
	void readSettings();
	void readSettingsIfChanged();
	struct Key {
		const char* name = nullptr;
		int code = 0;
	};
	std::mutex keyCombosMutex;
	std::map<std::string, Key> keys;
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
	std::vector<int> screenshotBtn;
	std:: mutex screenshotPathMutex;
	std::string screenshotPath;
	std::atomic_bool allowContinuousScreenshotting = false;
	std::atomic_bool dontUseScreenshotTransparency = false;
	std::atomic_int slowmoTimes = 3;
private:
	void addKey(const char* name, int code);
	int findMinCommentPos(const char* buf) const;
	std::string parseKeyName(const char* buf) const;
	std::string getKeyValue(const char* buf) const;
	void addKeyRange(char start, char end);
	int findChar(const char* buf, char c) const;
	void trim(std::string& str) const;
	std::vector<std::string> split(const std::string& str, char c) const;
	bool parseKeys(const char* keyName, std::string keyValue, std::vector<int>& keyCodes);
	bool parseInteger(const char* keyName, std::string keyValue, std::atomic_int& integer);
	bool parseBoolean(const char* keyName, std::string keyValue, std::atomic_bool& aBooleanValue);
	std::wstring getCurrentDirectory();
	bool getLastWriteTime(const std::wstring& path, FILETIME* fileTime);
	std::wstring settingsPath;
	bool firstSettingsParse = true;
	FILETIME lastSettingsWriteTime{0};
	HANDLE directoryChangeHandle = NULL;
	bool lastCallFailedToGetTime = false;
};

extern Settings settings;
