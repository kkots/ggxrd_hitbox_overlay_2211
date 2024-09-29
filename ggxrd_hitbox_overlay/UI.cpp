#include "pch.h"
#include "UI.h"
#include "Keyboard.h"
#include "logging.h"
#include "Settings.h"
#include "CustomWindowMessages.h"
#include "WinError.h"
#include "Detouring.h"
#include "GifMode.h"
#include "Game.h"
#include "Version.h"
#include "Graphics.h"
#include "EndScene.h"
#include "memoryFunctions.h"

#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"

#include <commdlg.h>  // for GetOpenFileNameW

UI ui;

static ImVec4 RGBToVec(DWORD color);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
const ImVec2 BTN_SIZE = ImVec2(60, 20);
static ImVec4 RED_COLOR = RGBToVec(0xEF5454);
static ImVec4 YELLOW_COLOR = RGBToVec(0xF9EA6C);
static ImVec4 GREEN_COLOR = RGBToVec(0x5AE976);
static char strbuf[512];
static std::string stringArena;
static char printdecimalbuf[512];

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

static int findCharRev(const char* buf, char c) {
    const char* ptr = buf;
    while (*ptr != '\0') {
        ++ptr;
    }
    while (ptr != buf) {
        --ptr;
        if (*ptr == c) return ptr - buf;
    }
    return -1;
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

static void AddTooltip(const char* desc) {
    if (ImGui::BeginItemTooltip()) {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

static void HelpMarker(const char* desc) {
    ImGui::TextDisabled("(?)");
    AddTooltip(desc);
}

static void RightAlign(float w) {
	const auto rightEdge = ImGui::GetCursorPosX() + ImGui::GetColumnWidth();
    const auto posX = (rightEdge - w);
    ImGui::SetCursorPosX(posX);
}

static void RightAlignedText(const char* txt) {
	RightAlign(ImGui::CalcTextSize(txt).x);
	ImGui::TextUnformatted(txt);
}

static void RightAlignedColoredText(const ImVec4& color, const char* txt) {
	RightAlign(ImGui::CalcTextSize(txt).x);
    ImGui::TextColored(color, txt);
}

static void CenterAlign(float w) {
	const auto rightEdge = ImGui::GetCursorPosX() + ImGui::GetColumnWidth() / 2;
    const auto posX = (rightEdge - w / 2);
    ImGui::SetCursorPosX(posX);
}

static void CenterAlignedText(const char* txt) {
	CenterAlign(ImGui::CalcTextSize(txt).x);
	ImGui::TextUnformatted(txt);
}

// color = 0xRRGGBB
ImVec4 RGBToVec(DWORD color) {
	// they also wrote it as r, g, b, a... just in struct form
	return {
		((color >> 16) & 0xff) * (1.F / 255.F),  // red
		((color >> 8) & 0xff) * (1.F / 255.F),  // green
		(color & 0xff) * (1.F / 255.F),  // blue
		1.F  // alpha
	};
}

static const char* formatBoolean(bool value) {
	static const char* trueStr = "true";
	static const char* falseStr = "false";
	return value ? trueStr : falseStr;
}

static void pushZeroItemSpacingStyle() {
	ImGuiStyle& style = ImGui::GetStyle();  // it's a reference
	ImVec2 itemSpacing = style.ItemSpacing;
	itemSpacing.x = 0;
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, itemSpacing);
}

bool UI::onDllMain() {
	uintptr_t GetKeyStateRData = findImportedFunction("GuiltyGearXrd.exe", "USER32.DLL", "GetKeyState");
	if (GetKeyStateRData) {
		std::vector<char> sig;
		std::vector<char> mask;
		byteSpecificationToSigMask("8b 3d ?? ?? ?? ?? 52 ff d7", sig, mask);
		substituteWildcard(sig.data(), mask.data(), 0, (void*)GetKeyStateRData);
		uintptr_t GetKeyStateCallPlace = sigscanOffset(
			"GuiltyGearXrd.exe",
			sig.data(),
			mask.data(),
			{ 2 },
			nullptr, "GetKeyStateCallPlace");
		if (GetKeyStateCallPlace) {
			std::vector<char> origBytes;
			origBytes.resize(4);
			memcpy(origBytes.data(), &GetKeyStateRData, 4);
			detouring.addInstructionToReplace(GetKeyStateCallPlace, origBytes);
			DWORD oldProtect;
			VirtualProtect((void*)GetKeyStateCallPlace, 4, PAGE_EXECUTE_READWRITE, &oldProtect);
			hook_GetKeyStatePtr = hook_GetKeyState;
			void** hook_GetKeyStatePtrPtr = &hook_GetKeyStatePtr;
			memcpy((void*)GetKeyStateCallPlace, &hook_GetKeyStatePtrPtr, 4);
			DWORD unused;
			VirtualProtect((void*)GetKeyStateCallPlace, 4, oldProtect, &unused);
			FlushInstructionCache(GetCurrentProcess(), (void*)GetKeyStateCallPlace, 4);
		}
	}
	return true;
}

void UI::onDllDetachStage1() {
	timerDisabled = true;
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
}

void UI::onDllDetachGraphics() {
	std::unique_lock<std::mutex> guard(lock);
	shutdownGraphics = true;
	if (imguiD3DInitialized) {
		logwrap(fputs("imgui freeing D3D resources\n", logfile));
    	imguiD3DInitialized = false;
    	ImGui_ImplDX9_Shutdown();
	}
}

void UI::onDllDetachNonGraphics() {
	if (imguiD3DInitialized) {
		logwrap(fputs("imgui calling onDllDetachGraphics from onDllDetachNonGraphics\n", logfile));
		// this shouldn't happen
		onDllDetachGraphics();
	}
	std::unique_lock<std::mutex> guard(lock);
	if (imguiInitialized) {
		logwrap(fputs("imgui freeing non-D3D resources\n", logfile));
	    ImGui_ImplWin32_Shutdown();
	    ImGui::DestroyContext();
	    imguiInitialized = false;
	}
}

void UI::prepareDrawData() {
	if (!visible || gifMode.modDisabled) {
		takeScreenshot = false;
		takeScreenshotPress = false;
		imguiActive = false;
		return;
	}
	std::unique_lock<std::mutex> uiGuard(lock);
	initialize();
	std::unique_lock<std::mutex> keyboardGuard(keyboard.mutex);
	Keyboard::MutexLockedFromOutsideGuard keyboardOutsideGuard;
	std::unique_lock<std::mutex> settingsGuard(settings.keyCombosMutex);
	
	bool oldStateChanged = stateChanged;
	stateChanged = false;
	needWriteSettings = false;
	keyCombosChanged = false;
	bool imguiActiveTemp = false;
	bool takeScreenshotTemp = false;
	
	decrementFlagTimer(allowNextFrameTimer, allowNextFrame);
	decrementFlagTimer(takeScreenshotTimer, takeScreenshotPress);
	for (int i = 0; i < 2; ++i) {
		decrementFlagTimer(clearTensionGainMaxComboTimer[i], clearTensionGainMaxCombo[i]);
	}
	
	if (!imguiD3DInitialized) {
		needInitFont = true;
		return;
	}
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	static std::string windowTitle;
	if (windowTitle.empty()) {
		windowTitle = "ggxrd_hitbox_overlay v";
		windowTitle += VERSION;
	}
	ImGui::Begin(windowTitle.c_str(), &visible);
	if (ImGui::CollapsingHeader("Framedata", ImGuiTreeNodeFlags_DefaultOpen)) {
		if (ImGui::BeginTable("##PayerData", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_NoPadOuterX)) {
			ImGui::TableSetupColumn("P1", ImGuiTableColumnFlags_WidthStretch, 0.37f);
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 0.26f);
			ImGui::TableSetupColumn("P2", ImGuiTableColumnFlags_WidthStretch, 0.37f);
			
			ImGui::TableNextColumn();
			RightAlignedText("P1");
			ImGui::TableNextColumn();
			void* texId = graphics.getTexture();
			if (texId) {
				CenterAlign(14.F);
				ImGui::Image(texId, ImVec2(14.F, 14.F));
				AddTooltip("Hover your mouse cursor over individual row titles to see their corresponding tooltips.");
			}
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("P2");
	    	
		    {
		    	PlayerInfo& player = endScene.players[0];
		    	ImGui::TableNextColumn();
			    sprintf_s(strbuf, "[x%s]", printDecimal((player.defenseModifier + 0x100) / 0x100, 2, 0));
			    stringArena = strbuf;
			    sprintf_s(strbuf, " (x%s)", printDecimal(player.gutsPercentage, 2, 0));
			    stringArena += strbuf;
			    sprintf_s(strbuf, " %3d", player.hp);
			    stringArena += strbuf;
			    RightAlignedText(stringArena.c_str());
		    }
			
	    	ImGui::TableNextColumn();
			CenterAlignedText("HP");
			AddTooltip("HP (x Guts) [x Defense Modifier]");
			
			{
		    	PlayerInfo& player = endScene.players[1];
			    ImGui::TableNextColumn();
			    sprintf_s(strbuf, "%-3d ", player.hp);
			    stringArena = strbuf;
			    sprintf_s(strbuf, "(x%s) ", printDecimal(player.gutsPercentage, 2, 0));
			    stringArena += strbuf;
			    sprintf_s(strbuf, "[x%s]", printDecimal((player.defenseModifier + 0x100) / 0x100, 2, 0));
			    stringArena += strbuf;
		    	ImGui::TextUnformatted(stringArena.c_str());
			}
			
		    for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = endScene.players[i];
			    ImGui::TableNextColumn();
			    sprintf_s(strbuf, "%s", printDecimal(player.tension, 2, 0));
			    if (i == 0) RightAlignedText(strbuf);
			    else ImGui::TextUnformatted(strbuf);
		    	
		    	if (i == 0) {
			    	ImGui::TableNextColumn();
		    		CenterAlignedText("Meter");
					AddTooltip("Tension");
		    	}
		    }
		    for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = endScene.players[i];
			    ImGui::TableNextColumn();
			    sprintf_s(strbuf, "%s", printDecimal(player.burst, 2, 0));
			    if (i == 0) RightAlignedText(strbuf);
			    else ImGui::TextUnformatted(strbuf);
		    	
		    	if (i == 0) {
			    	ImGui::TableNextColumn();
		    		CenterAlignedText("Burst");
		    	}
		    }
		    for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = endScene.players[i];
			    ImGui::TableNextColumn();
			    sprintf_s(strbuf, "%s", printDecimal(player.risc, 2, 0));
			    if (i == 0) RightAlignedText(strbuf);
			    else ImGui::TextUnformatted(strbuf);
		    	
		    	if (i == 0) {
			    	ImGui::TableNextColumn();
		    		CenterAlignedText("RISC");
		    	}
		    }
		    for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = endScene.players[i];
			    ImGui::TableNextColumn();
			    const char* formatString;
			    if (i == 0) {
			    	formatString = "%4d / %4d";
			    } else {
			    	formatString = "%-4d / %-4d";
			    }
			    sprintf_s(strbuf, formatString, player.stun, player.stunThreshold * 100);
			    if (i == 0) RightAlignedText(strbuf);
			    else ImGui::TextUnformatted(strbuf);
		    	
		    	if (i == 0) {
			    	ImGui::TableNextColumn();
		    		CenterAlignedText("Stun");
		    	}
		    }
		    for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = endScene.players[i];
			    ImGui::TableNextColumn();
			    //	*strbuf = '\0';
			    if (player.superfreezeStartup && player.superfreezeStartup <= player.startupDisp && (player.startedUp || player.startupProj)) {
		    		sprintf_s(strbuf, "%d+%d", player.superfreezeStartup, player.startupDisp - player.superfreezeStartup);
			    } else if (player.superfreezeStartup && !(player.startedUp || player.startupProj)) {
			    	sprintf_s(strbuf, "%d", player.superfreezeStartup);
			    } else { //if (player.startedUp || player.startupProj) {
			    	sprintf_s(strbuf, "%d", player.startupDisp);
			    }
			    if (i == 0) RightAlignedText(strbuf);
			    else ImGui::TextUnformatted(strbuf);
		    	
		    	if (i == 0) {
			    	ImGui::TableNextColumn();
		    		CenterAlignedText("Startup");
		    		AddTooltip("The startup of the last performed move. The last startup frame is also an active frame.");
		    	}
		    }
		    for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = endScene.players[i];
			    ImGui::TableNextColumn();
			    if (player.startedUp || player.startupProj) {
				    player.activesDisp.print(strbuf, sizeof strbuf);
			    } else {
			    	*strbuf = '\0';
			    }
			    float w = ImGui::CalcTextSize(strbuf).x;
			    if (w > ImGui::GetContentRegionAvail().x) {
				    ImGui::TextWrapped(strbuf);
			    } else {
				    if (i == 0) RightAlign(w);
				    ImGui::TextUnformatted(strbuf);
			    }
		    	
		    	if (i == 0) {
			    	ImGui::TableNextColumn();
		    		CenterAlignedText("Active");
		    		AddTooltip("Number of active frames in the last performed move.\n"
		    			"Numbers in (), when surrounded by other numbers, mean non-active frames inbetween active frames."
		    			" So, for example, 1(2)3 would mean you were active for 1 frame, then were not active for 2 frames, then were active again for 3.\n"
		    			"Numbers separated by a , symbol mean active frames of separate distinct hits, between which there is no gap of non-active frames."
		    			" For example, 1,4 would mean that the move is active for 5 frames, and the first frame is hit one, while frames 2-5 are hit two.\n"
		    			"Sometimes, when the number of hits is too great, an alternative representation of active frames will be displayed over a / sign."
		    			" In this representation, separate hits are not shown, only active frames and gaps between active frames, in ().");
		    	}
		    }
		    for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = endScene.players[i];
			    ImGui::TableNextColumn();
			    if ((player.startedUp || player.startupProj) && !(player.recoveryDisp == 0 && player.landingRecovery)) {
				    sprintf_s(strbuf, "%d", player.recoveryDisp);
			    } else {
			    	*strbuf = '\0';
			    }
			    if (player.landingRecovery) {
			    	if (*strbuf != '\0') {
			    		strcat(strbuf, "+");
			    	}
			    	sprintf_s(strbuf + strlen(strbuf), sizeof strbuf - strlen(strbuf), "%d landing", player.landingRecovery);
			    }
			    if (i == 0) RightAlignedText(strbuf);
			    else ImGui::TextUnformatted(strbuf);
		    	
		    	if (i == 0) {
			    	ImGui::TableNextColumn();
		    		CenterAlignedText("Recovery");
		    		AddTooltip("Number of recovery frames in the last performed move."
		    			" If the move spawned a projectile that lasts beyond the boundaries of the move, its recovery is 0.");
		    	}
		    }
		    for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = endScene.players[i];
			    ImGui::TableNextColumn();
			    if (player.idlePlus && player.totalDisp) {
			    	sprintf_s(strbuf, "%d", player.totalDisp);
			    } else {
			    	*strbuf = '\0';
			    }
			    if (i == 0) RightAlignedText(strbuf);
			    else ImGui::TextUnformatted(strbuf);
		    	
		    	if (i == 0) {
			    	ImGui::TableNextColumn();
		    		CenterAlignedText("Total");
		    		AddTooltip("Total number of frames in the last performed move.");
		    	}
		    }
		    
		    frameAdvantageControl();
		    
		    for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = endScene.players[i];
			    ImGui::TableNextColumn();
			    player.printGaps(strbuf, sizeof strbuf);
			    if (i == 0) RightAlignedText(strbuf);
			    else ImGui::TextUnformatted(strbuf);
		    	
		    	if (i == 0) {
			    	ImGui::TableNextColumn();
		    		CenterAlignedText("Gaps");
		    		AddTooltip("Each gap is the number of frames from when the opponent left blockstun to when they entered blockstun again.");
		    	}
		    }
		    for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = endScene.players[i];
			    ImGui::TableNextColumn();
			    sprintf_s(strbuf, "%s", formatBoolean(player.idle));
			    if (i == 0) RightAlignedText(strbuf);
			    else ImGui::TextUnformatted(strbuf);
		    	
		    	if (i == 0) {
			    	ImGui::TableNextColumn();
		    		CenterAlignedText("idle");
		    	}
		    }
		    for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = endScene.players[i];
			    ImGui::TableNextColumn();
			    sprintf_s(strbuf, "%s (%d)", formatBoolean(player.idlePlus), player.timePassed);
			    if (i == 0) RightAlignedText(strbuf);
			    else ImGui::TextUnformatted(strbuf);
		    	
		    	if (i == 0) {
			    	ImGui::TableNextColumn();
		    		CenterAlignedText("idlePlus");
		    	}
		    }
		    for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = endScene.players[i];
			    ImGui::TableNextColumn();
			    sprintf_s(strbuf, "%s (%d) (%s)", formatBoolean(player.idleLanding), player.timePassedLanding, formatBoolean(player.needLand));
			    if (i == 0) RightAlignedText(strbuf);
			    else ImGui::TextUnformatted(strbuf);
		    	
		    	if (i == 0) {
			    	ImGui::TableNextColumn();
		    		CenterAlignedText("idleLanding");
		    	}
		    }
		    for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = endScene.players[i];
			    ImGui::TableNextColumn();
			    sprintf_s(strbuf, "%d", player.hitstop);
			    if (i == 0) RightAlignedText(strbuf);
			    else ImGui::TextUnformatted(strbuf);
		    	
		    	if (i == 0) {
			    	ImGui::TableNextColumn();
		    		CenterAlignedText("hitstop");
		    	}
		    }
		    for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = endScene.players[i];
			    ImGui::TableNextColumn();
			    if (player.hitstun) {
			    	sprintf_s(strbuf, "%d", player.hitstun);
			    } else if (player.blockstun) {
			    	sprintf_s(strbuf, "%d", player.blockstun);
			    } else if (player.wakeupTiming) {
			    	sprintf_s(strbuf, "%d", player.wakeupTiming - player.animFrame + 1);
			    } else {
			    	*strbuf = '\0';
			    }
			    if (i == 0) RightAlignedText(strbuf);
			    else ImGui::TextUnformatted(strbuf);
		    	
		    	if (i == 0) {
			    	ImGui::TableNextColumn();
		    		CenterAlignedText("hitstun/...");
		    	}
		    }
		    for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = endScene.players[i];
			    ImGui::TableNextColumn();
			    if (i == 0) RightAlignedText(player.anim);
			    else ImGui::TextUnformatted(player.anim);
		    	
		    	if (i == 0) {
			    	ImGui::TableNextColumn();
		    		CenterAlignedText("anim");
		    	}
		    }
		    for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = endScene.players[i];
			    ImGui::TableNextColumn();
			    sprintf_s(strbuf, "%d", player.animFrame);
			    if (i == 0) RightAlignedText(strbuf);
			    else ImGui::TextUnformatted(strbuf);
		    	
		    	if (i == 0) {
			    	ImGui::TableNextColumn();
		    		CenterAlignedText("animFrame");
		    	}
		    }
		    for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = endScene.players[i];
			    ImGui::TableNextColumn();
			    sprintf_s(strbuf, "%d", player.startup);
			    if (i == 0) RightAlignedText(strbuf);
			    else ImGui::TextUnformatted(strbuf);
		    	
		    	if (i == 0) {
			    	ImGui::TableNextColumn();
		    		CenterAlignedText("startup");
		    	}
		    }
		    for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = endScene.players[i];
			    ImGui::TableNextColumn();
			    player.actives.print(strbuf, sizeof strbuf);
			    if (i == 0) RightAlignedText(strbuf);
			    else ImGui::TextUnformatted(strbuf);
		    	
		    	if (i == 0) {
			    	ImGui::TableNextColumn();
		    		CenterAlignedText("active");
		    	}
		    }
		    for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = endScene.players[i];
			    ImGui::TableNextColumn();
			    sprintf_s(strbuf, "%d", player.recovery);
			    if (i == 0) RightAlignedText(strbuf);
			    else ImGui::TextUnformatted(strbuf);
		    	
		    	if (i == 0) {
			    	ImGui::TableNextColumn();
		    		CenterAlignedText("recovery");
		    	}
		    }
		    for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = endScene.players[i];
			    ImGui::TableNextColumn();
			    sprintf_s(strbuf, "%d", player.total);
			    if (i == 0) RightAlignedText(strbuf);
			    else ImGui::TextUnformatted(strbuf);
		    	
		    	if (i == 0) {
			    	ImGui::TableNextColumn();
		    		CenterAlignedText("total");
		    	}
		    }
		    for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = endScene.players[i];
			    ImGui::TableNextColumn();
			    sprintf_s(strbuf, "%d", player.startupProj);
			    if (i == 0) RightAlignedText(strbuf);
			    else ImGui::TextUnformatted(strbuf);
		    	
		    	if (i == 0) {
			    	ImGui::TableNextColumn();
		    		CenterAlignedText("startupProj");
		    	}
		    }
		    for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = endScene.players[i];
			    ImGui::TableNextColumn();
			    player.activesProj.print(strbuf, sizeof strbuf);
			    if (i == 0) RightAlignedText(strbuf);
			    else ImGui::TextUnformatted(strbuf);
		    	
		    	if (i == 0) {
			    	ImGui::TableNextColumn();
		    		CenterAlignedText("activesProj");
		    	}
		    }
		    Entity superflashInstigator = endScene.getSuperflashInstigator();
		    for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = endScene.players[i];
			    ImGui::TableNextColumn();
			    if (superflashInstigator == player.pawn) {
			    	sprintf_s(strbuf, "true (%d)", endScene.getSuperflashCounterSelf());
			    } else if (superflashInstigator) {
			    	sprintf_s(strbuf, "true (%d)", endScene.getSuperflashCounterAll());
			    } else {
			    	strcpy(strbuf, "false");
			    }
			    if (i == 0) RightAlignedText(strbuf);
			    else ImGui::TextUnformatted(strbuf);
		    	
		    	if (i == 0) {
			    	ImGui::TableNextColumn();
		    		CenterAlignedText("superfreeze");
		    	}
		    }
		    ImGui::EndTable();
	    	ImGui::Spacing();
	    	if (ImGui::CollapsingHeader("Projectiles")) {
	    		if (ImGui::BeginTable("##Projectiles", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_NoPadOuterX)) {
					ImGui::TableSetupColumn("P1", ImGuiTableColumnFlags_WidthStretch, 0.4f);
					ImGui::TableSetupColumn("##FieldTitle", ImGuiTableColumnFlags_WidthStretch, 0.2f);
					ImGui::TableSetupColumn("P2", ImGuiTableColumnFlags_WidthStretch, 0.4f);
					
					ImGui::TableNextColumn();
					RightAlignedText("P1");
					ImGui::TableNextColumn();
					ImGui::TableNextColumn();
					ImGui::TextUnformatted("P2");
					
		    		struct Row {
		    			ProjectileInfo* side[2] { nullptr };
		    		};
		    		std::vector<Row> rows;
			    	for (ProjectileInfo& projectile : endScene.projectiles) {
	    				bool found = false;
		    			for (Row& row : rows) {
		    				if (!row.side[projectile.team]) {
		    					row.side[projectile.team] = &projectile;
		    					found = true;
		    					break;
		    				}
		    			}
		    			if (!found) {
		    				rows.emplace_back();
		    				Row& row = rows.back();
		    				row.side[projectile.team] = &projectile;
		    			}
			    	}
			    	bool isFirst = true;
			    	for (Row& row : rows) {
			    		if (!isFirst) {
			    			ImGui::TableNextColumn();
			    			ImGui::TableNextColumn();
			    			ImGui::TableNextColumn();
			    		}
			    		isFirst = false;
					    for (int i = 0; i < 2; ++i) {
						    ImGui::TableNextColumn();
					    	if (row.side[i]) {
						    	ProjectileInfo& projectile = *row.side[i];
							    sprintf_s(strbuf, "%p", (void*)projectile.ptr);
							    if (i == 0) RightAlignedText(strbuf);
							    else ImGui::TextUnformatted(strbuf);
					    	}
					    	
					    	if (i == 0) {
						    	ImGui::TableNextColumn();
					    		CenterAlignedText("ptr");
					    	}
					    }
					    for (int i = 0; i < 2; ++i) {
						    ImGui::TableNextColumn();
					    	if (row.side[i]) {
						    	ProjectileInfo& projectile = *row.side[i];
							    sprintf_s(strbuf, "%d", projectile.lifeTimeCounter);
							    if (i == 0) RightAlignedText(strbuf);
							    else ImGui::TextUnformatted(strbuf);
					    	}
					    	
					    	if (i == 0) {
						    	ImGui::TableNextColumn();
					    		CenterAlignedText("lifeTimeCounter");
					    	}
					    }
					    for (int i = 0; i < 2; ++i) {
						    ImGui::TableNextColumn();
					    	if (row.side[i]) {
						    	ProjectileInfo& projectile = *row.side[i];
							    if (i == 0) RightAlignedText(projectile.animName);
							    else ImGui::TextUnformatted(projectile.animName);
					    	}
					    	
					    	if (i == 0) {
						    	ImGui::TableNextColumn();
					    		CenterAlignedText("anim");
					    	}
					    }
					    for (int i = 0; i < 2; ++i) {
						    ImGui::TableNextColumn();
					    	if (row.side[i]) {
						    	ProjectileInfo& projectile = *row.side[i];
							    sprintf_s(strbuf, "%d", projectile.animFrame);
							    if (i == 0) RightAlignedText(strbuf);
							    else ImGui::TextUnformatted(strbuf);
					    	}
					    	
					    	if (i == 0) {
						    	ImGui::TableNextColumn();
					    		CenterAlignedText("animFrame");
					    	}
					    }
					    for (int i = 0; i < 2; ++i) {
						    ImGui::TableNextColumn();
					    	if (row.side[i]) {
						    	ProjectileInfo& projectile = *row.side[i];
							    sprintf_s(strbuf, "%d", projectile.hitstop);
							    if (i == 0) RightAlignedText(strbuf);
							    else ImGui::TextUnformatted(strbuf);
					    	}
					    	
					    	if (i == 0) {
						    	ImGui::TableNextColumn();
					    		CenterAlignedText("hitstop");
					    	}
					    }
					    for (int i = 0; i < 2; ++i) {
						    ImGui::TableNextColumn();
					    	if (row.side[i]) {
						    	ProjectileInfo& projectile = *row.side[i];
							    projectile.actives.print(strbuf, sizeof strbuf);
							    
							    float w = ImGui::CalcTextSize(strbuf).x;
							    if (w > ImGui::GetContentRegionAvail().x) {
								    ImGui::TextWrapped(strbuf);
							    } else {
								    if (i == 0) RightAlign(w);
								    ImGui::TextUnformatted(strbuf);
							    }
					    	}
					    	
					    	if (i == 0) {
						    	ImGui::TableNextColumn();
					    		CenterAlignedText("active");
					    	}
					    }
					    for (int i = 0; i < 2; ++i) {
						    ImGui::TableNextColumn();
					    	if (row.side[i]) {
						    	ProjectileInfo& projectile = *row.side[i];
							    sprintf_s(strbuf, "%d", projectile.startup);
							    if (i == 0) RightAlignedText(strbuf);
							    else ImGui::TextUnformatted(strbuf);
					    	}
					    	
					    	if (i == 0) {
						    	ImGui::TableNextColumn();
					    		CenterAlignedText("startup");
					    	}
					    }
					    for (int i = 0; i < 2; ++i) {
						    ImGui::TableNextColumn();
					    	if (row.side[i]) {
						    	ProjectileInfo& projectile = *row.side[i];
							    sprintf_s(strbuf, "%d", projectile.total);
							    if (i == 0) RightAlignedText(strbuf);
							    else ImGui::TextUnformatted(strbuf);
					    	}
					    	
					    	if (i == 0) {
						    	ImGui::TableNextColumn();
					    		CenterAlignedText("total");
					    	}
					    }
					    for (int i = 0; i < 2; ++i) {
						    ImGui::TableNextColumn();
					    	if (row.side[i]) {
						    	ProjectileInfo& projectile = *row.side[i];
							    sprintf_s(strbuf, "%s", formatBoolean(projectile.disabled));
							    if (i == 0) RightAlignedText(strbuf);
							    else ImGui::TextUnformatted(strbuf);
					    	}
					    	
					    	if (i == 0) {
						    	ImGui::TableNextColumn();
					    		CenterAlignedText("disabled");
					    	}
					    }
			    	}
			    	ImGui::EndTable();
	    		}
	    	}
		}
		if (ImGui::Button("Show Tension Values")) {
			showTensionData = true;
		}
	}
	if (ImGui::CollapsingHeader("Hitboxes")) {
		stateChanged = stateChanged || ImGui::Checkbox("GIF Mode", &gifModeOn);
		ImGui::SameLine();
		HelpMarker("GIF mode is:\n"
			"; 1) Background becomes black\n"
			"; 2) Camera is centered on you\n"
			"; 3) Opponent is invisible and invulnerable\n"
			"; 4) Hide HUD");
		stateChanged = stateChanged || ImGui::Checkbox("GIF Mode (Black Background Only)", &gifModeToggleBackgroundOnly);
		ImGui::SameLine();
		HelpMarker("Makes background black (and, for screenshotting purposes, - effectively transparent, if Post Effect is turned off in the game's graphics settings).");
		stateChanged = stateChanged || ImGui::Checkbox("GIF Mode (Camera Center Only)", &gifModeToggleCameraCenterOnly);
		ImGui::SameLine();
		HelpMarker("Centers the camera on you.");
		stateChanged = stateChanged || ImGui::Checkbox("GIF Mode (Hide Opponent Only)", &gifModeToggleHideOpponentOnly);
		ImGui::SameLine();
		HelpMarker("Make the opponent invisible and invulnerable.");
		stateChanged = stateChanged || ImGui::Checkbox("GIF Mode (Hide HUD Only)", &gifModeToggleHudOnly);
		ImGui::SameLine();
		HelpMarker("Hides the HUD.");
		stateChanged = stateChanged || ImGui::Checkbox("No Gravity", &noGravityOn);
		ImGui::SameLine();
		HelpMarker("Prevents you from falling, meaning you remain in the air as long as 'No Gravity Mode' is enabled.");
		stateChanged = stateChanged || ImGui::Checkbox("Freeze Game", &freezeGame);
		ImGui::SameLine();
		if (ImGui::Button("Next Frame")) {
			allowNextFrame = true;
			allowNextFrameTimer = 10;
		}
		ImGui::SameLine();
		HelpMarker("Freezes the current frame of the game and stops gameplay from advancing. You can advance gameplay to the next frame using the 'Next Frame' button."
			" It is way more convenient to use this feature with shortcuts, which you can configure in the 'Keyboard shortcuts' section below.");
		stateChanged = stateChanged || ImGui::Checkbox("Slow-Mo Mode", &slowmoGame);
		ImGui::SameLine();
		int slowmoTimes = settings.slowmoTimes;
		ImGui::SetNextItemWidth(80.F);
		if (ImGui::InputInt("Slow-Mo Factor", &slowmoTimes, 1, 1, 0)) {
			if (slowmoTimes <= 0) {
				slowmoTimes = 1;
			}
			settings.slowmoTimes = slowmoTimes;
		}
		imguiActiveTemp = imguiActiveTemp || ImGui::IsItemActive();
		ImGui::SameLine();
		HelpMarker("Makes the game run slower, advancing only on every second, every third and so on frame, depending on 'Slow-Mo Factor' field.");
		ImGui::Button("Take Screenshot");
		if (ImGui::IsItemActivated()) {
			// Regular ImGui button 'press' (ImGui::Button(...) function returning true) happens when you RELEASE the button,
			// but to simulate the old keyboard behavior we need this to happen when you PRESS the button
			takeScreenshotPress = true;
			takeScreenshotTimer = 10;
		}
		takeScreenshotTemp = ImGui::IsItemActive();
		ImGui::SameLine();
		HelpMarker("Takes a screenshot. This only works during a match, so it won't work, for example, on character select screen or on some menu."
			" If you make background black using 'GIF Mode Enabled' and set Post Effect to off in the game's graphics settings, you"
			" will be able to take screenshots with transparency. Screenshots are copied to clipboard by default, but if 'Screenshots path' is set,"
			" they're saved there instead.");
		stateChanged = stateChanged || ImGui::Checkbox("Continuous Screenshotting Mode", &continuousScreenshotToggle);
		ImGui::SameLine();
		HelpMarker("When this option is enabled, screenshots will be taken every frame, unless the game is frozen, in which case"
			" a new screenshot is taken only when the frame advances. You can run out of disk space pretty fast with this and it slows"
			" the game down significantly. Continuous screenshotting is only allowed in training mode.");
	}
	if (ImGui::CollapsingHeader("Settings")) {
		if (ImGui::CollapsingHeader("Hitbox settings")) {
			stateChanged = stateChanged || ImGui::Checkbox("Disable Hitbox Drawing", &hitboxDisplayDisabled);
			ImGui::SameLine();
			HelpMarker("Disables display of hitboxes/boxes. All other features of the mod continue to work normally.");
			bool drawPushboxCheckSeparately = settings.drawPushboxCheckSeparately;
			if (ImGui::Checkbox("Draw Pushbox Check Separately", &drawPushboxCheckSeparately)) {
				settings.drawPushboxCheckSeparately = drawPushboxCheckSeparately;
				needWriteSettings = true;
			}
			ImGui::SameLine();
			HelpMarker(settings.getOtherUIDescription(&settings.drawPushboxCheckSeparately));
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
			imguiActiveTemp = imguiActiveTemp || ImGui::IsItemActive();
			ImGui::SameLine();
			if (ImGui::Button("Select", BTN_SIZE) && keyboard.thisProcessWindow) {
				PostMessageW(keyboard.thisProcessWindow, WM_APP_OPEN_FILE_SELECTION, 0, 0);
			}
			ImGui::SameLine();
			HelpMarker(settings.getOtherUIDescription(&settings.screenshotPath));
			bool allowContinuousScreenshotting = settings.allowContinuousScreenshotting;
			if (ImGui::Checkbox("Allow Continuous Screenshotting When Button Is Held", &allowContinuousScreenshotting)) {
				settings.allowContinuousScreenshotting = allowContinuousScreenshotting;
				needWriteSettings = true;
			}
			ImGui::SameLine();
			HelpMarker(settings.getOtherUIDescription(&settings.allowContinuousScreenshotting));
			bool dontUseScreenshotTransparency = settings.dontUseScreenshotTransparency;
			if (ImGui::Checkbox("Take Screenshots Without Transparency", &dontUseScreenshotTransparency)) {
				settings.dontUseScreenshotTransparency = dontUseScreenshotTransparency;
			}
			ImGui::SameLine();
			HelpMarker(settings.getOtherUIDescription(&settings.dontUseScreenshotTransparency));
		}
		if (ImGui::CollapsingHeader("Keyboard shortcuts")) {
			keyComboControl(settings.modWindowVisibilityToggle);
			keyComboControl(settings.gifModeToggle);
			keyComboControl(settings.gifModeToggleBackgroundOnly);
			keyComboControl(settings.gifModeToggleCameraCenterOnly);
			keyComboControl(settings.gifModeToggleHideOpponentOnly);
			keyComboControl(settings.gifModeToggleHudOnly);
			keyComboControl(settings.noGravityToggle);
			keyComboControl(settings.freezeGameToggle);
			keyComboControl(settings.slowmoGameToggle);
			keyComboControl(settings.allowNextFrameKeyCombo);
			keyComboControl(settings.disableHitboxDisplayToggle);
			keyComboControl(settings.screenshotBtn);
			keyComboControl(settings.continuousScreenshotToggle);
		}
		if (ImGui::CollapsingHeader("General settings")) {
			bool modWindowVisibleOnStart = settings.modWindowVisibleOnStart;
			if (ImGui::Checkbox("Mod Window Visible On Start", &modWindowVisibleOnStart)) {
				settings.modWindowVisibleOnStart = modWindowVisibleOnStart;
				needWriteSettings = true;
			}
			ImGui::SameLine();
			HelpMarker(settings.getOtherUIDescription(&settings.modWindowVisibleOnStart));
		}
	}
	ImGui::End();
	if (showTensionData) {
		ImGui::Begin("Tension Data", &showTensionData);
		if (ImGui::BeginTable("##TensionData", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_NoPadOuterX)) {
			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 220.f);
			ImGui::TableSetupColumn("P1", ImGuiTableColumnFlags_WidthStretch, 0.5f);
			ImGui::TableSetupColumn("P2", ImGuiTableColumnFlags_WidthStretch, 0.5f);
			ImGui::TableHeadersRow();
			
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Tension");
			AddTooltip("Meter");
			for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = endScene.players[i];
			    ImGui::TableNextColumn();
			    printDecimal(player.tension, 2, 0);
			    ImGui::TextUnformatted(printdecimalbuf);
			}
			
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Tension Pulse");
			AddTooltip("Affects how fast you gain tension. Gained on IB, landing attacks, moving forward. Lost when moving back. Decays on its own slowly towards 0."
				" Tension Pulse Penalty and Corner Penalty may decrease Tension Pulse over time.");
			for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = endScene.players[i];
			    ImGui::TableNextColumn();
			    sprintf_s(strbuf, "%-6d / %d", player.tensionPulse, player.tensionPulse < 0 ? -25000 : 25000);
			    ImGui::TextUnformatted(strbuf);
			}
			
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Negative Penalty Active");
			AddTooltip("When Negative Penalty is active, you receive only 20% of the tension you would without it when attacking or moving.");
			for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = endScene.players[i];
			    ImGui::TableNextColumn();
			    ImGui::TextUnformatted(player.negativePenaltyTimer ? "yes" : "no");
			}
			
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Negative Penalty Time Left");
			AddTooltip("Timer that counts down how much time is remaining until Negative Penalty wears off.");
			for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = endScene.players[i];
			    ImGui::TableNextColumn();
			    sprintf_s(strbuf, "%.2f sec", (float)player.negativePenaltyTimer / 60.F);
			    ImGui::TextUnformatted(strbuf);
			}
			
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Negative Penalty Buildup");
			AddTooltip("Tracks your progress towards reaching Negative Penalty. Negative Penalty is built up by Tension Pulse Penalty being red"
				" or Corner Penalty being red or by moving back.");
			for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = endScene.players[i];
			    ImGui::TableNextColumn();
			    sprintf_s(strbuf, "%s / 100.00", printDecimal(player.negativePenalty, 2, -6));
			    ImGui::TextUnformatted(strbuf);
			}
			
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Tension Pulse Penalty");
			AddTooltip("Reduces Tension Pulse (yellow) and at large enough values (red) increases Negative Penalty Buildup."
				" Increases constantly by 1. Gets reduced when getting hit, landing hits, getting your attack blocked or moving forward.");
			for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = endScene.players[i];
			    ImGui::TableNextColumn();
			    sprintf_s(strbuf, "%-4d / 1800", player.tensionPulsePenalty);
			    if (player.tensionPulsePenaltySeverity == 0) {
			    	ImGui::TextUnformatted(strbuf);
			    } else if (player.tensionPulsePenaltySeverity == 1) {
			    	ImGui::TextColored(YELLOW_COLOR, strbuf);
			    } else if (player.tensionPulsePenaltySeverity == 2) {
			    	ImGui::TextColored(RED_COLOR, strbuf);
			    }
			}
			
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Corner Penalty");
			AddTooltip("Penalty for being in touch with the screen or the wall. Reduces Tension Pulse (yellow) and increases Negative Penalty (red)."
				" Slowly decays when not in corner and gets reduced when getting hit.");
			for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = endScene.players[i];
			    ImGui::TableNextColumn();
			    sprintf_s(strbuf, "%-3d / 960", player.cornerPenalty);
			    if (player.cornerPenaltySeverity == 0) {
			    	ImGui::TextUnformatted(strbuf);
			    } else if (player.cornerPenaltySeverity == 1) {
			    	ImGui::TextColored(YELLOW_COLOR, strbuf);
			    } else if (player.cornerPenaltySeverity == 2) {
			    	ImGui::TextColored(RED_COLOR, strbuf);
			    }
			}
			
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Negative Penalty Gain");
			AddTooltip("Negative Penalty Buildup gain modifier. Affects how fast Negative Penalty Buildup increases.\n"
				"Negative Penalty Gain Modifier = Distance-Based Modifier * Tension Pulse-Based Modifier.\n"
				"Distance-Based Modifier - depends on distance to the opponent.\n"
				"Tension Pulse-Based Modifier - depends on Tension Pulse.");
			for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = endScene.players[i];
			    ImGui::TableNextColumn();
			    sprintf_s(strbuf, "%s * %d%c", printDecimal(player.tensionPulsePenaltyGainModifier_distanceModifier, 0, -3, true),
			    	player.tensionPulsePenaltyGainModifier_tensionPulseModifier, '%');
			    ImGui::TextUnformatted(strbuf);
			}
			
			bool comboHappening = false;
			for (int i = 0; i < 2; ++i) {
				if (endScene.players[i].inPain) {
					comboHappening = true;
					break;
				}
			}
			
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Tension Gain On Attack");
			AddTooltip("Affects how fast you gain Tension increases when performing attacks or combos.\n"
				"Tension Gain Modifier = Distance-Based Modifier * Negative Penalty Modifier * Tension Pulse-Based Modifier.\n"
				"Distance-Based Modifier - depends on distance to the opponent.\n"
				"Negative Penalty Modifier - if a Negative Penalty is active, the modifier is 20%, otherwise it's 100%.\n"
				"Tension Pulse-Based Modifier - depends on Tension Pulse.\n\n"
				"A fourth modifier may be displayed, which is an extra tension modifier. "
				"It may be present if you use Stylish mode or playing MOM mode. It will be highlighted in yellow.\n\n"
				"A fourth or fifth modifier may be displayed, which is a combo hit count-dependent modifier. "
				"It affects how fast you gain Tension from performing a combo.");
			for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = endScene.players[i];
			    ImGui::TableNextColumn();
			    sprintf_s(strbuf, "%s", printDecimal(player.tensionGainModifier_distanceModifier, 0, -3, true));
			    sprintf_s(strbuf + strlen(strbuf), sizeof strbuf - strlen(strbuf), " * %s", printDecimal(player.tensionGainModifier_negativePenaltyModifier, 0, -3, true));
			    sprintf_s(strbuf + strlen(strbuf), sizeof strbuf - strlen(strbuf), " * %s", printDecimal(player.tensionGainModifier_tensionPulseModifier, 0, -3, true));
			    bool needPop = false;
			    if (player.extraTensionGainModifier != 100) {
			    	needPop = true;
			    	pushZeroItemSpacingStyle();
			    	strcat(strbuf, " * ");
			    	ImGui::TextUnformatted(strbuf);
			    	sprintf_s(strbuf, "%s", printDecimal(player.extraTensionGainModifier, 0, -3, true));
			    	ImGui::SameLine();
    				ImGui::PushStyleColor(ImGuiCol_Text, YELLOW_COLOR);
			    	ImGui::TextUnformatted(strbuf);
			    	ImGui::PopStyleColor();
			    	*strbuf = '\0';
			    	ImGui::SameLine();
			    }
		    	int total = player.tensionGainModifier_distanceModifier
			    	* player.tensionGainModifier_negativePenaltyModifier;
		    	total /= 100;
		    	total *= player.tensionGainModifier_tensionPulseModifier
			    	* player.extraTensionGainModifier;
		    	total /= 10000;
			    if (comboHappening) {
			    	if (!player.inPain) {
			    		sprintf_s(strbuf + strlen(strbuf), sizeof strbuf - strlen(strbuf), " * %s",
			    			printDecimal(player.dealtComboCountTensionGainModifier, 0, -3, true));
			    		total *= player.dealtComboCountTensionGainModifier;
			    		total /= 100;
			    	}
		    	}
		    	sprintf_s(strbuf + strlen(strbuf), sizeof strbuf - strlen(strbuf), " = %d%c", total, '%');
			    ImGui::TextUnformatted(strbuf);
			    if (needPop) {
			    	ImGui::PopStyleVar();
			    }
			}
			
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Tension Gain");
			AddTooltip("Affects how fast you gain Tension increases when performing attacks or combos.\n"
				"Tension Gain Modifier = Distance-Based Modifier * Negative Penalty Modifier * Tension Pulse-Based Modifier.\n"
				"Distance-Based Modifier - depends on distance to the opponent.\n"
				"Negative Penalty Modifier - if a Negative Penalty is active, the modifier is 20%, otherwise it's 100%.\n"
				"Tension Pulse-Based Modifier - depends on Tension Pulse.\n\n"
				"A fourth modifer may be displayed, which happens when you are getting combo'd. It affects how much tension you gain from getting hid by a combo"
				" and depends on the number of hits.");
			for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = endScene.players[i];
			    ImGui::TableNextColumn();
			    sprintf_s(strbuf, "%s", printDecimal(player.tensionGainModifier_distanceModifier, 0, -3, true));
			    sprintf_s(strbuf + strlen(strbuf), sizeof strbuf - strlen(strbuf),
			    	" * %s", printDecimal(player.tensionGainModifier_negativePenaltyModifier, 0, -3, true));
			    sprintf_s(strbuf + strlen(strbuf), sizeof strbuf - strlen(strbuf),
			    	" * %s", printDecimal(player.tensionGainModifier_tensionPulseModifier, 0, -3, true));
		    	int total = player.tensionGainModifier_distanceModifier
			    	* player.tensionGainModifier_negativePenaltyModifier
			    	* player.tensionGainModifier_tensionPulseModifier;
		    	total /= 10000;
			    if (comboHappening) {
			    	if (player.inPain) {
			    		sprintf_s(strbuf + strlen(strbuf), sizeof strbuf - strlen(strbuf), " * %s",
			    			printDecimal(player.receivedComboCountTensionGainModifier, 0, -3, true));
			    		total *= player.receivedComboCountTensionGainModifier;
			    		total /= 100;
			    	}
		    	}
		    	sprintf_s(strbuf + strlen(strbuf), sizeof strbuf - strlen(strbuf), " = %d%c", total, '%');
			    ImGui::TextUnformatted(strbuf);
			}
			
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Tension Gain On Last Hit");
			AddTooltip("How much Tension was gained on a single last hit (either inflicting it or getting hit by it).");
			for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = endScene.players[i];
			    ImGui::TableNextColumn();
			    printDecimal(player.tensionGainOnLastHit, 2, 0);
			    ImGui::TextUnformatted(printdecimalbuf);
			}
			
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Tension Gain Last Combo");
			AddTooltip("How much Tension was gained on the entire last performed combo (either inflicting it or getting hit by it).");
			for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = endScene.players[i];
			    ImGui::TableNextColumn();
			    printDecimal(player.tensionGainLastCombo, 2, 0);
			    ImGui::TextUnformatted(printdecimalbuf);
			}
			
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Tension Gain Max Combo");
			AddTooltip("The maximum amount of Tension that was gained on an entire performed combo during this training session"
				" (either inflicting it or getting hit by it).\n"
				"You can clear this value by pressing a button below this table.");
			for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = endScene.players[i];
			    ImGui::TableNextColumn();
			    printDecimal(player.tensionGainMaxCombo, 2, 0);
			    ImGui::TextUnformatted(printdecimalbuf);
			}
			
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Burst Gain On Last Hit");
			AddTooltip("How much Burst was gained on a single last hit (either inflicting it or getting hit by it).");
			for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = endScene.players[i];
			    ImGui::TableNextColumn();
			    printDecimal(player.burstGainOnLastHit, 2, 0);
			    ImGui::TextUnformatted(printdecimalbuf);
			}
			
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Burst Gain Last Combo");
			AddTooltip("How much Burst was gained on the entire last performed combo (either inflicting it or getting hit by it).");
			for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = endScene.players[i];
			    ImGui::TableNextColumn();
			    printDecimal(player.burstGainLastCombo, 2, 0);
			    ImGui::TextUnformatted(printdecimalbuf);
			}
			
			float offsets[2];
			
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Burst Gain Max Combo");
			AddTooltip("The maximum amount of Burst that was gained on an entire performed combo during this training session"
				" (either inflicting it or getting hit by it).\n"
				"You can clear this value by pressing a button below this table.");
			for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = endScene.players[i];
			    ImGui::TableNextColumn();
				offsets[i] = ImGui::GetCursorPosX();
			    printDecimal(player.burstGainMaxCombo, 2, 0);
			    ImGui::TextUnformatted(printdecimalbuf);
			}
			
			
		    ImGui::EndTable();
		    
		    for (int i = 0; i < 2; ++i) {
			    ImGui::SetCursorPosX(offsets[i]);
		    	ImGui::PushID(i);
			    if (ImGui::Button("Clear Max Combo")) {
			    	clearTensionGainMaxCombo[i] = true;
			    	clearTensionGainMaxComboTimer[i] = 10;
			    	stateChanged = true;
			    }
			    AddTooltip("Clear max combo's Tension and Burst gain.");
			    if (i == 0) ImGui::SameLine();
			    ImGui::PopID();
			}
		}
		ImGui::End();
	}
	takeScreenshot = takeScreenshotTemp;
	imguiActive = imguiActiveTemp;
	ImGui::EndFrame();
	ImGui::Render();
	drawData = ImGui::GetDrawData();
	if (keyCombosChanged) {
		settings.onKeyCombosUpdated();
	}
	if ((stateChanged || needWriteSettings) && keyboard.thisProcessWindow) {
		PostMessageW(keyboard.thisProcessWindow, WM_APP_UI_STATE_CHANGED, stateChanged, needWriteSettings);
	}
	stateChanged = oldStateChanged || stateChanged;
}

void UI::onEndScene(IDirect3DDevice9* device) {
	if (!visible || !imguiInitialized || gifMode.modDisabled) {
		return;
	}
	std::unique_lock<std::mutex> uiGuard(lock);
	if (!imguiD3DInitialized) {
		imguiD3DInitialized = true;
		ImGui_ImplDX9_Init(device);
		ImGui_ImplDX9_NewFrame();
		graphics.getTexture();
	}
	if (needInitFont) {
		ImGui_ImplDX9_NewFrame();
		needInitFont = false;
	}
	if (drawData) {
		ImGui_ImplDX9_RenderDrawData((ImDrawData*)drawData);
	}
}

void UI::initialize() {
	if (imguiInitialized || !visible || !keyboard.thisProcessWindow || gifMode.modDisabled) return;
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(keyboard.thisProcessWindow);
	imguiInitialized = true;
}

void UI::handleResetBefore() {
	if (imguiD3DInitialized) {
		ImGui_ImplDX9_InvalidateDeviceObjects();
	}
}

void UI::handleResetAfter() {
	if (imguiD3DInitialized) {
		ImGui_ImplDX9_CreateDeviceObjects();
		graphics.getTexture();
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
		if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
			return TRUE;
		
		switch (message) {
		case WM_APP_SCREENSHOT_PATH_UPDATED: {
			if (!timerDisabled) {
				if (timerId == 0) {
					timerId = SetTimer(NULL, 0, 1000, Timerproc);
				} else {
					SetTimer(NULL, timerId, 1000, Timerproc);
				}
			}
			return TRUE;
		}
		case WM_APP_UI_STATE_CHANGED: {
			if (lParam) {
				if (timerId) {
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

SHORT WINAPI UI::hook_GetKeyState(int nVirtKey) {
	HookGuard hookGuard("GetKeyState");
	SHORT result;
	if (ui.imguiActive) {
		result = 0;
	} else {
		result = GetKeyState(nVirtKey);
	}
	return result;
}

void UI::decrementFlagTimer(int& timer, bool& flag) {
	if (timer > 0 && flag) {
		--timer;
		if (timer == 0) flag = false;
	}
}

void UI::frameAdvantageControl() {
    for (int i = 0; i < 2; ++i) {
    	PlayerInfo& player = endScene.players[i];
	    ImGui::TableNextColumn();
    	if (player.frameAdvantageValid && player.landingFrameAdvantageValid && player.frameAdvantage != player.landingFrameAdvantage) {
    		frameAdvantageTextFormat(player.frameAdvantage, strbuf, sizeof strbuf);
    		strcat(strbuf, " (");
    		frameAdvantageTextFormat(player.landingFrameAdvantage, strbuf + strlen(strbuf), sizeof strbuf - strlen(strbuf));
    		strcat(strbuf, ")");
    		if (i == 0) RightAlign(ImGui::CalcTextSize(strbuf).x);
			pushZeroItemSpacingStyle();
	    	frameAdvantageText(player.frameAdvantage);
	    	ImGui::SameLine();
			ImGui::TextUnformatted(" (");
			ImGui::SameLine();
			frameAdvantageText(player.landingFrameAdvantage);
			ImGui::SameLine();
			ImGui::TextUnformatted(")");
			ImGui::PopStyleVar();
	    } else if (player.frameAdvantageValid || player.landingFrameAdvantageValid) {
	    	int frameAdvantage = player.frameAdvantageValid ? player.frameAdvantage : player.landingFrameAdvantage;
	    	frameAdvantageTextFormat(frameAdvantage, strbuf, sizeof strbuf);
	    	if (i == 0) RightAlign(ImGui::CalcTextSize(strbuf).x);
	    	frameAdvantageText(frameAdvantage);
	    }
    	
    	if (i == 0) {
	    	ImGui::TableNextColumn();
    		CenterAlignedText("Frame Adv.");
    		AddTooltip("Frame advantage of this player over the other player, in frames, after doing the last move. Frame advantage is who became able to 5P/j.P earlier."
    			" (For certain stances it means whether you can do a move out of your stance.)"
		    	" Please note that players may become able to block earlier than they become able to attack.\n\n"
		    	"The value in () means frame advantage after yours or your opponent's landing, whatever happened last. The other value means frame advantage"
		    	" immediately after recovering in the air.");
    	}
    }
}

void UI::frameAdvantageTextFormat(int frameAdv, char* buf, size_t bufSize) {
	if (frameAdv > 0) {
		sprintf_s(buf, bufSize, "+%d", frameAdv);
	} else {
		sprintf_s(buf, bufSize, "%d", frameAdv);
	}
}

void UI::frameAdvantageText(int frameAdv) {
	frameAdvantageTextFormat(frameAdv, strbuf, sizeof strbuf);
	if (frameAdv > 0) {
		ImGui::TextColored(GREEN_COLOR, strbuf);
	} else {
		if (frameAdv == 0) {
			ImGui::TextUnformatted(strbuf);
		} else {
			ImGui::TextColored(RED_COLOR, strbuf);
		}
	}
}

char* UI::printDecimal(int num, int numAfterPoint, int padding, bool percentage) {
	int divideBy = 1;
	for (int i = 0; i < numAfterPoint; ++i) {
		divideBy *= 10;
	}
	char fmtbuf[9] = "%d.%.";
	if (numAfterPoint == 0) {
		if (percentage) {
			sprintf_s(printdecimalbuf, "%d%c", num, '%');
		} else {
			sprintf_s(printdecimalbuf, "%d", num);
		}
	} else {
		if (numAfterPoint < 0 || numAfterPoint > 99) {
			*printdecimalbuf = '\0';
			return printdecimalbuf;
		}
		sprintf_s(fmtbuf + 5, sizeof fmtbuf - 5, "%dd", numAfterPoint);
		if (num >= 0) {
			sprintf_s(printdecimalbuf, fmtbuf, num / divideBy, num % divideBy);
		} else {
			num = -num;
			sprintf_s(printdecimalbuf + 1, sizeof printdecimalbuf - 1, fmtbuf, num / divideBy, num % divideBy);
			*printdecimalbuf = '-';
		}
		if (percentage) {
			size_t n = strlen(printdecimalbuf);
			if (n < sizeof printdecimalbuf - 1) {
				printdecimalbuf[n] = '%';
			}
			if (n + 1 < sizeof printdecimalbuf) {
				printdecimalbuf[n + 1] = '\0';
			}
		}
	}
	size_t len = strlen(printdecimalbuf);
	int absPadding = padding;
	if (absPadding < 0) absPadding = -absPadding;
	if (len < (size_t)absPadding) {
		size_t padSize = (size_t)absPadding - len;
		if ((size_t)(sizeof printdecimalbuf - len - 1) < padSize) {
			padSize = (size_t)(sizeof printdecimalbuf - len - 1);
		}
		if (padSize == 0) {
			return printdecimalbuf;
		}
		if (padding > 0) {
			memmove(printdecimalbuf, printdecimalbuf + padSize, len);
			memset(printdecimalbuf, ' ', padSize);
		} else {
			memset(printdecimalbuf + len, ' ', padSize);
		}
		printdecimalbuf[len + padSize] = '\0';
	}
	return printdecimalbuf;
}
