#pragma once
#include <list>
#include <vector>
#include <string>

BOOL CALLBACK EnumWindowsFindMyself(HWND hwnd, LPARAM lParam);

class Settings
{
public:
	bool onDllMain();
	struct Key {
		const char* name = nullptr;
		int code = 0;
	};
	std::vector<Key> keys;
	std::list<std::vector<std::string>> extraNames;
	std::vector<int> gifModeToggle;
	std::vector<int> noGravityToggle;
private:
	void addKeyRange(char start, char end);
	void readSettings();
	int findChar(const char* buf, char c) const;
	void trim(std::string& str) const;
	std::vector<std::string> split(const std::string& str, char c) const;
	bool parseKeys(const char* keyName, const char* buf, std::vector<int>& keyCodes);
};

extern Settings settings;
