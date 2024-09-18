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

#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"

#include <commdlg.h>  // for GetOpenFileNameW

UI ui;

static ImVec4 RGBToVec(DWORD color);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
const ImVec2 BTN_SIZE = ImVec2(60, 20);
static ImVec4 RED_COLOR = RGBToVec(0xEF5454);
static ImVec4 GREEN_COLOR = RGBToVec(0x5AE976);
static char strbuf[512];
static std::string stringArena;

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
	if (!visible || isSteamOverlayActive || gifMode.modDisabled) {
		GetKeyStateAllowedThread = 0;
		takeScreenshot = false;
		takeScreenshotPress = false;
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
	bool imguiActiveTemp = false;
	bool takeScreenshotTemp = false;
	
	decrementFlagTimer(allowNextFrameTimer, allowNextFrame);
	decrementFlagTimer(takeScreenshotTimer, takeScreenshotPress);
	
	ImGui_ImplDX9_NewFrame();
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
		    	PlayerInfo& player = graphics.drawDataUse.players[0];
		    	ImGui::TableNextColumn();
			    sprintf_s(strbuf, "[%d.%.2dx]",
			    	(player.defenseModifier + 0x100) / 0x100,
			    	(player.defenseModifier + 0x100) * 100 / 0x100 % 100);
			    stringArena = strbuf;
			    sprintf_s(strbuf, " (%d.%.2dx)",
			    	player.gutsPercentage / 100,
			    	player.gutsPercentage % 100);
			    stringArena += strbuf;
			    sprintf_s(strbuf, " %3d", player.hp);
			    stringArena += strbuf;
			    RightAlignedText(stringArena.c_str());
		    }
			
	    	ImGui::TableNextColumn();
			CenterAlignedText("HP");
			AddTooltip("HP (x Guts) [x Defense Modifier]");
			
			{
		    	PlayerInfo& player = graphics.drawDataUse.players[1];
			    ImGui::TableNextColumn();
			    sprintf_s(strbuf, "%-3d ", player.hp);
			    stringArena = strbuf;
			    sprintf_s(strbuf, " (x%d.%.2d)",
			    	player.gutsPercentage / 100,
			    	player.gutsPercentage % 100);
			    stringArena += strbuf;
			    sprintf_s(strbuf, "[x%d.%.2d]",
			    	(player.defenseModifier + 0x100) / 0x100,
			    	(player.defenseModifier + 0x100) * 100 / 0x100 % 100);
			    stringArena += strbuf;
		    	ImGui::TextUnformatted(stringArena.c_str());
			}
			
		    for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = graphics.drawDataUse.players[i];
			    ImGui::TableNextColumn();
			    sprintf_s(strbuf, "%d", player.tension / 100);
			    if (i == 0) RightAlignedText(strbuf);
			    else ImGui::TextUnformatted(strbuf);
		    	
		    	if (i == 0) {
			    	ImGui::TableNextColumn();
		    		CenterAlignedText("Meter");
					AddTooltip("Tension");
		    	}
		    }
		    for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = graphics.drawDataUse.players[i];
			    ImGui::TableNextColumn();
			    sprintf_s(strbuf, "%d", player.tensionPulse / 100);
			    if (i == 0) RightAlignedText(strbuf);
			    else ImGui::TextUnformatted(strbuf);
		    	
		    	if (i == 0) {
			    	ImGui::TableNextColumn();
		    		CenterAlignedText("Pulse");
					AddTooltip("Tension Pulse");
		    	}
		    }
		    for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = graphics.drawDataUse.players[i];
			    ImGui::TableNextColumn();
			    sprintf_s(strbuf, "%d", player.negativePenalty / 100);
			    if (i == 0) RightAlignedText(strbuf);
			    else ImGui::TextUnformatted(strbuf);
		    	
		    	if (i == 0) {
			    	ImGui::TableNextColumn();
		    		CenterAlignedText("Penalty");
					AddTooltip("Progress towards getting a Negative penalty");
		    	}
		    }
		    for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = graphics.drawDataUse.players[i];
			    ImGui::TableNextColumn();
			    sprintf_s(strbuf, "%d", player.burst / 100);
			    if (i == 0) RightAlignedText(strbuf);
			    else ImGui::TextUnformatted(strbuf);
		    	
		    	if (i == 0) {
			    	ImGui::TableNextColumn();
		    		CenterAlignedText("Burst");
		    	}
		    }
		    for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = graphics.drawDataUse.players[i];
			    ImGui::TableNextColumn();
			    sprintf_s(strbuf, "%d", player.risc / 100);
			    if (i == 0) RightAlignedText(strbuf);
			    else ImGui::TextUnformatted(strbuf);
		    	
		    	if (i == 0) {
			    	ImGui::TableNextColumn();
		    		CenterAlignedText("RISC");
		    	}
		    }
		    for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = graphics.drawDataUse.players[i];
			    ImGui::TableNextColumn();
			    sprintf_s(strbuf, "%4d / %4d", player.stun, player.stunThreshold * 100);
			    if (i == 0) RightAlignedText(strbuf);
			    else ImGui::TextUnformatted(strbuf);
		    	
		    	if (i == 0) {
			    	ImGui::TableNextColumn();
		    		CenterAlignedText("Stun");
		    	}
		    }
		    /*for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = graphics.drawDataUse.players[i];
			    ImGui::TableNextColumn();
			    if (player.startedUp) {
				    if (player.superfreezeStartup) {
				    	if (player.startedUp) {
				    		sprintf_s(strbuf, "%d+%d", player.superfreezeStartup, player.startup - player.superfreezeStartup);
				    	} else {
				    		sprintf_s(strbuf, "%d", player.superfreezeStartup);
				    	}
				    } else {
				    	sprintf_s(strbuf, "%d", player.startup);
				    }
			    } else {
			    	*strbuf = '\0';
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
		    	PlayerInfo& player = graphics.drawDataUse.players[i];
			    ImGui::TableNextColumn();
			    if (player.projectileStartedUp) {
			    	sprintf_s(strbuf, "%d", player.projectileStartup);
			    } else {
			    	*strbuf = '\0';
			    }
			    if (i == 0) RightAlignedText(strbuf);
			    else ImGui::TextUnformatted(strbuf);
		    	
		    	if (i == 0) {
			    	ImGui::TableNextColumn();
		    		CenterAlignedText("Startup (P)");
					AddTooltip("Startup (Projectile)\n"
						"The startup of a launched projectile in the last performed move. The last startup frame is also an active frame.");
		    	}
		    }
		    for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = graphics.drawDataUse.players[i];
			    ImGui::TableNextColumn();
			    if (player.startedUp) {
				    player.actives.print(strbuf, sizeof strbuf);
			    } else {
			    	*strbuf = '\0';
			    }
			    if (i == 0) RightAlignedText(strbuf);
			    else ImGui::TextUnformatted(strbuf);
		    	
		    	if (i == 0) {
			    	ImGui::TableNextColumn();
		    		CenterAlignedText("Active");
		    		AddTooltip("Number of active frames in the last performed move. Numbers in () mean non-active frames inbetween active frames."
		    			" So, for example, 1(2)3 would mean you were active for 1 frame, then were not active for 2 frames, then were active again for 3.");
		    	}
		    }
		    for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = graphics.drawDataUse.players[i];
			    ImGui::TableNextColumn();
			    if (player.projectileStartedUp) {
				    player.projectileActives.print(strbuf, sizeof strbuf);
			    } else {
			    	*strbuf = '\0';
			    }
			    if (i == 0) RightAlignedText(strbuf);
			    else ImGui::TextUnformatted(strbuf);
		    	
		    	if (i == 0) {
			    	ImGui::TableNextColumn();
		    		CenterAlignedText("Active (P)");
					AddTooltip("Active (Projectile).\n"
						"Number of active frames of a launched projectile in the last performed move. For a description of what numbers in () mean,"
		    			" read the tooltip of 'Active'.");
		    	}
		    }
		    for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = graphics.drawDataUse.players[i];
			    ImGui::TableNextColumn();
			    if (player.startedUp) {
				    sprintf_s(strbuf, "%d", player.recovery);
			    } else {
			    	*strbuf = '\0';
			    }
			    if (i == 0) RightAlignedText(strbuf);
			    else ImGui::TextUnformatted(strbuf);
		    	
		    	if (i == 0) {
			    	ImGui::TableNextColumn();
		    		CenterAlignedText("Recovery");
		    		AddTooltip("Number of recovery frames in the last performed move.");
		    	}
		    }*/
		    for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = graphics.drawDataUse.players[i];
			    ImGui::TableNextColumn();
			    if (player.idlePlus && player.total) {
			    	sprintf_s(strbuf, "%d", player.total);
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
		    
		    frameAdvantageControl("Frame Adv.", offsetof(PlayerInfo, frameAdvantage),
		    	offsetof(PlayerInfo, frameAdvantageValid),
		    	"Frame advantage of this player over the other player, in frames, after doing the last move. Frame advantage is who became able to 5P/j.P earlier."
		    	" Please note that players may become able to block earlier than they become able to attack.\n\n"
		    	" The value in () means frame advantage after landing. When () are present, the value that is not in () means frame advantage"
		    	" immediately after recovering in the air.");
		    
		    frameAdvantageControl("Frame Adv.L", offsetof(PlayerInfo, landingFrameAdvantage),
		    	offsetof(PlayerInfo, landingFrameAdvantageValid),
		    	"Eh");
		    
		    for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = graphics.drawDataUse.players[i];
			    ImGui::TableNextColumn();
			    sprintf_s(strbuf, "%s", formatBoolean(player.frameAdvantageValid));
			    if (i == 0) RightAlignedText(strbuf);
			    else ImGui::TextUnformatted(strbuf);
		    	
		    	if (i == 0) {
			    	ImGui::TableNextColumn();
		    		CenterAlignedText("Frame Adv. Valid");
		    	}
		    }
		    for (int i = 0; i < 2; ++i) {
		    	PlayerInfo& player = graphics.drawDataUse.players[i];
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
		    	PlayerInfo& player = graphics.drawDataUse.players[i];
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
		    	PlayerInfo& player = graphics.drawDataUse.players[i];
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
		    	PlayerInfo& player = graphics.drawDataUse.players[i];
			    ImGui::TableNextColumn();
			    sprintf_s(strbuf, "%s (%d) (%s)", formatBoolean(player.idleLanding), player.timePassedLanding, formatBoolean(player.needLand));
			    if (i == 0) RightAlignedText(strbuf);
			    else ImGui::TextUnformatted(strbuf);
		    	
		    	if (i == 0) {
			    	ImGui::TableNextColumn();
		    		CenterAlignedText("idleLanding");
		    	}
		    }
		    ImGui::EndTable();
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
	takeScreenshot = takeScreenshotTemp;
	imguiActive = imguiActiveTemp;
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
		PostMessageW(keyboard.thisProcessWindow, WM_APP_UI_STATE_CHANGED, stateChanged, needWriteSettings);
	}
	stateChanged = oldStateChanged || stateChanged;
}

void UI::initialize(IDirect3DDevice9* device) {
	if (imguiInitialized || !visible || !keyboard.thisProcessWindow || gifMode.modDisabled) return;
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
			if (timerId == 0) {
				timerId = SetTimer(NULL, 0, 1000, Timerproc);
			} else {
				SetTimer(NULL, timerId, 1000, Timerproc);
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

void UI::OnGameOverlayActivated(GameOverlayActivated_t* pParam) {
	isSteamOverlayActive = pParam->m_bActive;
	logwrap(fprintf(logfile, "isSteamOverlayActive: %d\n", isSteamOverlayActive));
	if (isSteamOverlayActive) GetKeyStateAllowedThread = 0;
	else if (imguiActive && !GetKeyStateAllowedThread) GetKeyStateAllowedThread = -1;
	else if (!imguiActive) GetKeyStateAllowedThread = 0;
}

SHORT WINAPI UI::hook_GetKeyState(int nVirtKey) {
	++detouring.hooksCounter;
	detouring.markHookRunning("GetKeyState", true);
	if (ui.GetKeyStateAllowedThread == 0 || GetCurrentThreadId() == ui.GetKeyStateAllowedThread) {
		std::unique_lock<std::mutex> guard(ui.orig_GetKeyStateMutex);
		SHORT result = ui.orig_GetKeyState(nVirtKey);
		detouring.markHookRunning("GetKeyState", false);
		--detouring.hooksCounter;
		return result;
	}
	detouring.markHookRunning("GetKeyState", false);
	--detouring.hooksCounter;
	return 0;
}

void UI::decrementFlagTimer(int& timer, bool& flag) {
	if (timer > 0 && flag) {
		--timer;
		if (timer == 0) flag = false;
	}
}

void UI::frameAdvantageControl(const char* columnName, DWORD frameAdvOff, DWORD frameAdvValidOff, const char* description) {
    for (int i = 0; i < 2; ++i) {
    	PlayerInfo& player = graphics.drawDataUse.players[i];
	    ImGui::TableNextColumn();
	    
	    int frameAdvantage = *(int*)((char*)&player + frameAdvOff);
	    bool valid = *(bool*)((char*)&player + frameAdvValidOff);
	    //if (player.frameAdvantageValid) {
	    if (frameAdvantage == 0) {
	    	*strbuf = '0';
	    	*(strbuf + 1) = '\0';
	    } else if (frameAdvantage > 0) {
			sprintf_s(strbuf, "+%d", frameAdvantage);
	    } else {
		    sprintf_s(strbuf, "%d", frameAdvantage);
	    }
	    stringArena = strbuf;
	    sprintf_s(strbuf + stringArena.size(), sizeof strbuf - stringArena.size(), " (%s)", formatBoolean(valid));
	    if (i == 0) {
	    	RightAlign(ImGui::CalcTextSize(strbuf).x + ImGui::GetStyle().ItemSpacing.x);
	    }
	    if (frameAdvantage == 0) {
		    ImGui::TextUnformatted("0");
	    } else if (frameAdvantage > 0) {
		    ImGui::TextColored(GREEN_COLOR, stringArena.c_str());
	    } else {
		    ImGui::TextColored(RED_COLOR, stringArena.c_str());
	    }
	    ImGui::SameLine();
	    ImGui::TextUnformatted(strbuf + stringArena.size());
	    //} else {
	    //	ImGui::TextUnformatted("");
	    //}
    	
    	if (i == 0) {
	    	ImGui::TableNextColumn();
    		CenterAlignedText(columnName);
    		AddTooltip(description);
    	}
    }
}
