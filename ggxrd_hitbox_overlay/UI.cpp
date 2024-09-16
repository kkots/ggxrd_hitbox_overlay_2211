#include "pch.h"
#include "UI.h"
#include "Keyboard.h"
#include "logging.h"
#include "Settings.h"
#include "CustomWindowMessages.h"
#include "WinError.h"
#include "Detouring.h"

#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"

#include <commdlg.h>  // for GetOpenFileNameW

UI ui;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
const ImVec2 BTN_SIZE = ImVec2(60, 20);  // from BBCF Improvement Mod

static bool endsWithCaseInsensitive(std::wstring str, const wchar_t* endingPart) {
    unsigned int length = 0;
    const wchar_t* ptr = endingPart;
    while (*ptr != L'\0') {
        if (length == 0xFFFFFFFF) return false;
        ++ptr;
        ++length;
    }
    if (str.size() < length) return false;
    if (length == 0) return true;
    --ptr;
    auto it = str.begin() + (str.size() - 1);
    while (length) {
        if (towupper(*it) != towupper(*ptr)) return false;
        --length;
        --ptr;
        if (it == str.begin()) break;
        --it;
    }
    return length == 0;
}

static int findCharRevW(const wchar_t* buf, wchar_t c) {
    const wchar_t* ptr = buf;
    while (*ptr != '\0') {
        ++ptr;
    }
    while (ptr != buf) {
        --ptr;
        if (*ptr == c) return ptr - buf;
    }
    return -1;
}

bool UI::selectFile(std::wstring& path, HWND owner) {
    std::wstring szFile;
    szFile = lastSelectedPath;
    szFile.resize(MAX_PATH, L'\0');

    OPENFILENAMEW selectedFiles{ 0 };
    selectedFiles.lStructSize = sizeof(OPENFILENAMEW);
    selectedFiles.hwndOwner = owner;
    selectedFiles.lpstrFile = &szFile.front();
    selectedFiles.nMaxFile = (DWORD)szFile.size() + 1;
    selectedFiles.lpstrFilter = L"PNG file (*.png)\0*.PNG\0";
    selectedFiles.nFilterIndex = 1;
    selectedFiles.lpstrFileTitle = NULL;
    selectedFiles.nMaxFileTitle = 0;
    selectedFiles.lpstrInitialDir = NULL;
    selectedFiles.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

    if (!GetSaveFileNameW(&selectedFiles)) {
        DWORD errCode = CommDlgExtendedError();
        if (!errCode) {
            logwrap(fputs("The file selection dialog was closed by the user.\n", logfile));
        }
        else {
            logwrap(fprintf(logfile, "Error selecting file. Error code: %.8x\n", errCode));
        }
        return false;
    }
    szFile.resize(lstrlenW(szFile.c_str()));

    if (!endsWithCaseInsensitive(szFile, L".png")) {
        path = szFile + L".png";
        return true;
    }
    path = szFile;
    int pos = findCharRevW(path.c_str(), L'\\');
    if (pos == -1) {
        lastSelectedPath = path;
    } else {
        lastSelectedPath = path.c_str() + pos + 1;
    }
    return true;
}

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

bool UI::onDllMain() {
	orig_GetKeyState = GetKeyState;
	if (!detouring.attach(&(PVOID&)orig_GetKeyState,
		hook_GetKeyState,
		&orig_GetKeyStateMutex,
		"GetKeyState")) return false;
	return true;
}

void UI::onDllDetach() {
	if (!imguiInitialized) return;
	if (!keyboard.thisProcessWindow) timerId = NULL;
	if (timerId) {
		PostMessageW(keyboard.thisProcessWindow, WM_APP_NEED_KILL_TIMER, timerId, 0);
		int attempts = 0;
		while (true) {
			Sleep(100);
			if (timerId == 0) {
				break;
			}
			++attempts;
			if (attempts > 3) {
				logwrap(fprintf(logfile, "Trying to kill timer not from the WndProc\n"));
				if (!KillTimer(NULL, timerId)) {
					WinError winErr;
					logwrap(fprintf(logfile, "Failed to kill timer not from the WndProc: %ls\n", winErr.getMessage()));
				}
				timerId = 0;
				break;
			}
		}
	}
	RecursiveGuard guard(lock);
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    imguiInitialized = false;
}

void UI::onEndScene(IDirect3DDevice9* device) {
	if (!visible || isSteamOverlayActive) {
		GetKeyStateAllowedThread = 0;
		imguiActive = false;
		return;
	}
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
		stateChanged = stateChanged || ImGui::Checkbox("GIF Mode Enabled", &gifModeOn);
		{
			std::unique_lock<std::mutex> screenshotGuard(settings.screenshotPathMutex);
			size_t newLen = settings.screenshotPath.size();
			if (newLen > MAX_PATH - 1) {
				newLen = MAX_PATH - 1;
			}
			memcpy(screenshotsPathBuf, settings.screenshotPath.c_str(), newLen);
			screenshotsPathBuf[newLen] = '\0';
		}
		ImGui::Text("Screenshots path");
		ImGui::SameLine();
        float w = ImGui::GetContentRegionAvail().x * 0.85f - BTN_SIZE.x;
        ImGui::SetNextItemWidth(w);
		if (ImGui::InputText("##Screenshots path", screenshotsPathBuf, MAX_PATH, 0, nullptr, nullptr)) {
			{
				std::unique_lock<std::mutex> screenshotGuard(settings.screenshotPathMutex);
				settings.screenshotPath = screenshotsPathBuf;
			}
			if (keyboard.thisProcessWindow) {
				logwrap(fputs("Posting message 'WM_APP_SCREENSHOT_PATH_UPDATED'\n", logfile));
				PostMessageW(keyboard.thisProcessWindow, WM_APP_SCREENSHOT_PATH_UPDATED, 0, 0);
			}
		}
		imguiActive = ImGui::IsItemActive();
		ImGui::SameLine();
		if (ImGui::Button("Select", BTN_SIZE) && keyboard.thisProcessWindow) {
			PostMessageW(keyboard.thisProcessWindow, WM_APP_OPEN_FILE_SELECTION, 0, 0);
		}
		ImGui::SameLine();
		HelpMarker(settings.getOtherUIDescription(&settings.screenshotPath));
	} else {
		imguiActive = false;
	}
	if (ImGui::CollapsingHeader("Keyboard shortcuts")) {
		keyComboControl(settings.modWindowVisibilityToggle);
		keyComboControl(settings.gifModeToggle);
		keyComboControl(settings.gifModeToggleBackgroundOnly);
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
	if ((stateChanged || needWriteSettings) && keyboard.thisProcessWindow) {
		logwrap(fprintf(logfile, "Posting WM_APP_UI_STATE_CHANGED: %d, %d\n", stateChanged, needWriteSettings));
		PostMessageW(keyboard.thisProcessWindow, WM_APP_UI_STATE_CHANGED, stateChanged, needWriteSettings);
	}
	stateChanged = oldStateChanged || stateChanged;
}

void UI::initialize(IDirect3DDevice9* device) {
	if (imguiInitialized || !visible || !keyboard.thisProcessWindow) return;
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

void __stdcall UI::Timerproc(HWND unnamedParam1, UINT unnamedParam2, UINT_PTR unnamedParam3, DWORD unnamedParam4) {
	if (ui.timerId == 0) return;
	logwrap(fprintf(logfile, "Timerproc called for timerId: %d\n", ui.timerId));
	settings.writeSettings();
	if (!KillTimer(NULL, ui.timerId)) {
		WinError winErr;
		logwrap(fprintf(logfile, "Failed to kill timer: %ls\n", winErr.getMessage()));
	}
	ui.timerId = 0;
}

LRESULT UI::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	if (imguiInitialized) {
		RecursiveGuard guard(lock);
		static int nestingLevel = 0;
		if (nestingLevel == 0) {
			if (imguiActive) {
				GetKeyStateAllowedThread = GetCurrentThreadId();
			} else {
				GetKeyStateAllowedThread = 0;
			}
		}
		++nestingLevel;
		LRESULT imguiResult = ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam);
		--nestingLevel;
		if (nestingLevel == 0) {
			if (imguiActive) {
				GetKeyStateAllowedThread = -1;
			} else {
				GetKeyStateAllowedThread = 0;
			}
		}
		if (imguiResult) return TRUE;
		switch (message) {
		case WM_APP_SCREENSHOT_PATH_UPDATED: {
			logwrap(fputs("Received message 'WM_APP_SCREENSHOT_PATH_UPDATED'\n", logfile));
			if (timerId == 0) {
				timerId = SetTimer(NULL, 0, 1000, Timerproc);
				logwrap(fprintf(logfile, "Calling SetTimer for new timer, timerId: %d\n", timerId));
			} else {
				SetTimer(NULL, timerId, 1000, Timerproc);
				logwrap(fprintf(logfile, "Calling SetTimer for old timer, timerId: %d\n", timerId));
			}
			return TRUE;
		}
		case WM_APP_UI_STATE_CHANGED: {
			if (lParam) {
				if (timerId) {
					logwrap(fprintf(logfile, "'WM_APP_UI_STATE_CHANGED': Killing timer. timerId: %d\n", timerId));
					KillTimer(NULL, timerId);
					timerId = 0;
				}
				settings.writeSettings();
				if (!wParam) return TRUE;
			}
			break;
		}
		case WM_APP_NEED_KILL_TIMER: {
			if (wParam == timerId && ui.timerId != 0) {
				logwrap(fprintf(logfile, "'WM_APP_NEED_KILL_TIMER': Killing timer. timerId: %d\n", timerId));
				KillTimer(NULL, timerId);
				timerId = 0;
			}
			return TRUE;
		}
		case WM_APP_OPEN_FILE_SELECTION: {
			std::wstring selectedPath;
			{
				std::unique_lock<std::mutex> screenshotGuard(settings.screenshotPathMutex);
				if (!settings.screenshotPath.empty()) {
					selectedPath.assign(settings.screenshotPath.size() + 1, L'\0');
					MultiByteToWideChar(CP_UTF8, 0, settings.screenshotPath.c_str(), -1, &selectedPath.front(), selectedPath.size());
					selectedPath.resize(wcslen(selectedPath.c_str()));
					if (!selectedPath.empty()) {
						lastSelectedPath = selectedPath;
					}
				}
			}
			ShowCursor(TRUE);
			if (selectFile(selectedPath, hWnd)) {
				{
					std::unique_lock<std::mutex> screenshotGuard(settings.screenshotPathMutex);
					settings.screenshotPath.assign(selectedPath.size() * 4 + 1, '\0');
					WideCharToMultiByte(CP_UTF8, 0, selectedPath.c_str(), -1, &settings.screenshotPath.front(), settings.screenshotPath.size(), NULL, NULL);
					logwrap(fprintf(logfile, "From selection dialog set screenshot path to: %s\n", settings.screenshotPath.c_str()));
					settings.screenshotPath.resize(strlen(settings.screenshotPath.c_str()));
				}
				if (timerId) {
					logwrap(fprintf(logfile, "'WM_APP_OPEN_FILE_SELECTION': Killing timer. timerId: %d\n", timerId));
					KillTimer(NULL, timerId);
					timerId = 0;
				}
				settings.writeSettings();
			}
			ShowCursor(FALSE);
			return TRUE;
		}
		}
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

void UI::OnGameOverlayActivated(GameOverlayActivated_t* pParam) {
	isSteamOverlayActive = pParam->m_bActive;
	if (isSteamOverlayActive) GetKeyStateAllowedThread = 0;
	else if (imguiActive && !GetKeyStateAllowedThread) GetKeyStateAllowedThread = -1;
	else if (!imguiActive) GetKeyStateAllowedThread = 0;
}

SHORT WINAPI UI::hook_GetKeyState(int nVirtKey) {
	++detouring.hooksCounter;
	if (ui.GetKeyStateAllowedThread == 0 || GetCurrentThreadId() == ui.GetKeyStateAllowedThread) {
		std::unique_lock<std::mutex> guard(ui.orig_GetKeyStateMutex);
		SHORT result = ui.orig_GetKeyState(nVirtKey);
		--detouring.hooksCounter;
		return result;
	}
	--detouring.hooksCounter;
	return 0;
}
