#pragma once
#include <d3d9.h>

class UI
{
public:
	void onDllDetach();
	void onEndScene(IDirect3DDevice9* device);
	LRESULT WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void handleResetBefore();
	void handleResetAfter();
private:
	void initialize(IDirect3DDevice9* device);
	bool imguiInitialized = false;
	bool checkbox1 = false;
	bool checkbox2 = false;
};

extern UI ui;
