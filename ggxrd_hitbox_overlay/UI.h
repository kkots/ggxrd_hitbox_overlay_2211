#pragma once
#include "pch.h"
#include <d3d9.h>
#include "RecursiveLock.h"
#include <vector>

class UI
{
public:
	void onDllDetach();
	void onEndScene(IDirect3DDevice9* device);
	LRESULT WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void handleResetBefore();
	void handleResetAfter();
	
	bool visible = true;
	bool stateChanged = false;
	bool gifModeOn = false;
	RecursiveLock lock;
private:
	void initialize(IDirect3DDevice9* device);
	bool imguiInitialized = false;
	void keyComboControl(std::vector<int>& keyCombo);
	bool needWriteSettings = false;
	bool keyCombosChanged = false;
};

extern UI ui;
