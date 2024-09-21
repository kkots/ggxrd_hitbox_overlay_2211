#pragma once
#include "pch.h"
#include <d3d9.h>
#include <vector>
#include <mutex>

class UI
{
public:
	bool onDllMain();
	void onDllDetach();
	void prepareDrawData();
	void onEndScene(IDirect3DDevice9* device);
	LRESULT WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void handleResetBefore();
	void handleResetAfter();
	
	bool visible = true;
	bool stateChanged = false;
	bool gifModeOn = false;
	bool gifModeToggleBackgroundOnly = false;
	bool gifModeToggleCameraCenterOnly = false;
	bool gifModeToggleHideOpponentOnly = false;
	bool gifModeToggleHudOnly = false;
	bool noGravityOn = false;
	bool freezeGame = false;
	bool hitboxDisplayDisabled = false;
	bool allowNextFrame = false;
	int allowNextFrameTimer = 0;
	bool takeScreenshot = false;
	bool takeScreenshotPress = false;
	int takeScreenshotTimer = 0;
	bool clearTensionGainMaxCombo[2] { false };
	int clearTensionGainMaxComboTimer[2] { 0 };
	bool slowmoGame = false;
	bool continuousScreenshotToggle = false;
	std::mutex lock;
	bool imguiActive = false;
	void* drawData = nullptr;
	bool needInitFont = false;
private:
	void initialize();
	bool imguiInitialized = false;
	bool imguiD3DInitialized = false;
	void keyComboControl(std::vector<int>& keyCombo);
	bool needWriteSettings = false;
	bool keyCombosChanged = false;
	char screenshotsPathBuf[MAX_PATH] { 0 };
	UINT_PTR timerId = 0;
	bool selectFile(std::wstring& path, HWND owner);
	std::wstring lastSelectedPath;
	static void __stdcall Timerproc(HWND unnamedParam1, UINT unnamedParam2, UINT_PTR unnamedParam3, DWORD unnamedParam4);
	SHORT (WINAPI *orig_GetKeyState) (int nVirtKey);
	std::mutex orig_GetKeyStateMutex;
	static SHORT WINAPI hook_GetKeyState(int nVirtKey);
	DWORD GetKeyStateAllowedThread = 0;
	void decrementFlagTimer(int& timer, bool& flag);
	void frameAdvantageControl();
	void frameAdvantageTextFormat(int frameAdv, char* buf, size_t bufSize);
	void frameAdvantageText(int frameAdv);
	char* printDecimal(int num, int numAfterPoint, int padding, bool percentage = false);
	bool showTensionData = false;
};

extern UI ui;
