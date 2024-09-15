#include "pch.h"
#include "UI.h"
#include "Keyboard.h"
#include "logging.h"
#include "Settings.h"
#include "CustomWindowMessages.h"

#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"

UI ui;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
const ImVec2 BTN_SIZE = ImVec2(60, 20);  // from BBCF Improvement Mod

static void HelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::BeginItemTooltip())
    {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void UI::onDllDetach() {
	if (!imguiInitialized) return;
	RecursiveGuard guard(lock);
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    imguiInitialized = false;
}

void UI::onEndScene(IDirect3DDevice9* device) {
	if (!visible) return;
	RecursiveGuard guard(lock);
	if (!imguiInitialized) initialize(device);
	
	std::unique_lock<std::mutex> keyboardGuard(keyboard.mutex);
	Keyboard::MutexLockedFromOutsideGuard keyboardOutsideGuard;
	std::unique_lock<std::mutex> settingsGuard(settings.keyCombosMutex);
	
	bool oldStateChanged = stateChanged;
	stateChanged = false;
	needWriteSettings = false;
	keyCombosChanged = false;
	
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGui::Begin("Mod UI Window", &visible);
	if (ImGui::CollapsingHeader("Hitbox settings")) {
		ImGui::Indent();
		stateChanged = stateChanged || ImGui::Checkbox("GIF Mode Enabled", &gifModeOn);
		ImGui::Unindent();
	}
	if (ImGui::CollapsingHeader("Keyboard shortcuts")) {
		ImGui::Indent();
		keyComboControl(settings.modWindowVisibilityToggle);
		keyComboControl(settings.gifModeToggle);
		keyComboControl(settings.gifModeToggleBackgroundOnly);
		ImGui::Unindent();
	}
	ImGui::End();
	ImGui::EndFrame();
	device->SetRenderState(D3DRS_ZENABLE, FALSE);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
	if (keyCombosChanged) {
		settings.onKeyCombosUpdated();
	}
	if (stateChanged || needWriteSettings) {
		logwrap(fprintf(logfile, "Posting WM_APP_UI_STATE_CHANGED: %d, %d\n", stateChanged, needWriteSettings));
		PostMessageW(keyboard.thisProcessWindow, WM_APP_UI_STATE_CHANGED, stateChanged, needWriteSettings);
	}
	stateChanged = oldStateChanged || stateChanged;
}

void UI::initialize(IDirect3DDevice9* device) {
	if (imguiInitialized || !visible) return;
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(keyboard.thisProcessWindow);
	ImGui_ImplDX9_Init(device);
	imguiInitialized = true;
}

void UI::handleResetBefore() {
	if (imguiInitialized) {
		RecursiveGuard guard(lock);
		ImGui_ImplDX9_InvalidateDeviceObjects();
	}
}

void UI::handleResetAfter() {
	if (imguiInitialized) {
		RecursiveGuard guard(lock);
		ImGui_ImplDX9_CreateDeviceObjects();
	}
}

LRESULT UI::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	if (imguiInitialized) {
		RecursiveGuard guard(lock);
		if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
			return TRUE;
	}
	return FALSE;
}

void UI::keyComboControl(std::vector<int>& keyCombo) {
	Settings::ComboInfo info;
	settings.getComboInfo(keyCombo, &info);
	
	bool keyComboChanged = false;
	
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(info.uiName);
	ImGui::SameLine();
	std::string idArena;
	std::vector<int> indicesToRemove;
	indicesToRemove.reserve(keyCombo.size());
	
	for (int i = 0; i < (int)keyCombo.size(); ++i) {
		if (i > 0) {
			ImGui::TextUnformatted(" + ");
			ImGui::SameLine();
		}
		idArena = "##";
		idArena += info.uiName;
		idArena += std::to_string(i);
		int currentlySelectedKey = keyCombo[i];
		const char* currentKeyStr = settings.getKeyRepresentation(currentlySelectedKey);
		ImGui::PushItemWidth(80);
		if (ImGui::BeginCombo(idArena.c_str(), currentKeyStr))
	    {
	    	ImGui::PushID(-1);
	    	if (ImGui::Selectable("Remove this key", false)) {
	    		indicesToRemove.push_back(i);
	    		needWriteSettings = true;
            	keyCombosChanged = true;
	    	}
            ImGui::PopID();
	        for (auto it = settings.keys.cbegin(); it != settings.keys.cend(); ++it)
	        {
	        	const Settings::Key& key = it->second;
	            ImGui::PushID((void*)&key);
	            if (ImGui::Selectable(key.uiName, currentlySelectedKey == key.code)) {
	            	needWriteSettings = true;
	            	keyCombosChanged = true;
        			keyCombo[i] = key.code;
	            }
	            ImGui::PopID();
	        }
	        ImGui::EndCombo();
	    }
	    ImGui::PopItemWidth();
		ImGui::SameLine();
	}
	if (!keyCombo.empty()) {
		ImGui::TextUnformatted(" + ");
		ImGui::SameLine();
	}
	idArena = "##";
	idArena += info.uiName;
	idArena += "New";
	ImGui::PushItemWidth(80);
	if (ImGui::BeginCombo(idArena.c_str(), ""))
    {
        for (auto it = settings.keys.cbegin(); it != settings.keys.cend(); ++it)
        {
        	const Settings::Key& key = it->second;
            ImGui::PushID((void*)&key);
            if (ImGui::Selectable(key.name, false)) {
            	needWriteSettings = true;
            	keyCombosChanged = true;
    			keyCombo.push_back(key.code);
            }
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
    int offset = 0;
    for (int indexToRemove : indicesToRemove) {
    	keyCombo.erase(keyCombo.begin() + indexToRemove - offset);
    	++offset;
    }
    
    ImGui::SameLine();
    HelpMarker(info.uiDescription);
}
