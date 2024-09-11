#include "pch.h"
#include "UI.h"
#include "Keyboard.h"

#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"

UI ui;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void UI::onDllDetach() {
	if (!imguiInitialized) return;
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    imguiInitialized = false;
}

void UI::onEndScene(IDirect3DDevice9* device) {
	if (!imguiInitialized) initialize(device);
	
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.
	ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
	ImGui::Checkbox("Demo Window", &checkbox1);      // Edit bools storing our window open/close state
	ImGui::Checkbox("Another Window", &checkbox2);
	ImGui::End();
	ImGui::EndFrame();
	device->SetRenderState(D3DRS_ZENABLE, FALSE);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}

void UI::initialize(IDirect3DDevice9* device) {
	if (imguiInitialized) return;
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(keyboard.thisProcessWindow);
	ImGui_ImplDX9_Init(device);
	imguiInitialized = true;
}

void UI::handleResetBefore() {
    ImGui_ImplDX9_InvalidateDeviceObjects();
}

void UI::handleResetAfter() {
    ImGui_ImplDX9_CreateDeviceObjects();
}

LRESULT UI::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	if (imguiInitialized && ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam)) {
		return TRUE;
	}
	return FALSE;
}
