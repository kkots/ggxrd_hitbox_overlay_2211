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
#include "EndScene.h"
#include "memoryFunctions.h"
#include "EntityList.h"

#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"

#include <commdlg.h>  // for GetOpenFileNameW
#include "resource.h"
#include "Graphics.h"

UI ui;

static ImVec4 RGBToVec(DWORD color);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
const ImVec2 BTN_SIZE = ImVec2(60, 20);
static ImVec4 RED_COLOR = RGBToVec(0xEF5454);
static ImVec4 YELLOW_COLOR = RGBToVec(0xF9EA6C);
static ImVec4 GREEN_COLOR = RGBToVec(0x5AE976);
static ImVec4 BLACK_COLOR = RGBToVec(0);
static char strbuf[512];
static std::string stringArena;
static char printdecimalbuf[512];
static int numDigits(int num);  // For negative numbers does not include the '-'
struct UVStartEnd { ImVec2 start; ImVec2 end; };
static UVStartEnd digitUVs[10];
struct FrameArt { FrameType type; const PngResource* resource; ImVec2 uvStart; ImVec2 uvEnd; const char* description; };
static FrameArt frameArtNonColorblind[FRAME_TYPE_LAST + 1];
static FrameArt frameArtColorblind[FRAME_TYPE_LAST + 1];
struct FrameMarkerArt { FrameMarkerType type; const PngResource* resource; ImVec2 uvStart; ImVec2 uvEnd; };
static FrameMarkerArt frameMarkerArtNonColorblind[MARKER_TYPE_LAST + 1];
static FrameMarkerArt frameMarkerArtColorblind[MARKER_TYPE_LAST + 1];

struct CustomImDrawList {
	ImVector<ImDrawCmd> CmdBuffer;
	ImVector<ImDrawIdx> IdxBuffer;
	ImVector<ImDrawVert> VtxBuffer;
};

struct GGIcon {
	ImVec2 size;
	ImVec2 uvStart;
	ImVec2 uvEnd;
};

static GGIcon coordsToGGIcon(int x, int y, int w, int h);
static GGIcon questionMarkIcon = coordsToGGIcon(1104, 302, 20, 28);
static GGIcon tipsIcon = coordsToGGIcon(253, 1093, 62, 41);
static GGIcon characterIcons[25];
static GGIcon characterIconsBorderless[25];
static void drawGGIcon(const GGIcon& icon);
static GGIcon scaleGGIconToHeight(const GGIcon& icon, float height);
static CharacterType getPlayerCharacter(int playerSide);
static void drawPlayerIconWithTooltip(int playerSide);
static bool endsWithCaseInsensitive(std::wstring str, const wchar_t* endingPart);
static int findCharRev(const char* buf, char c);
static int findCharRevW(const wchar_t* buf, wchar_t c);
static void AddTooltip(const char* desc);
static void HelpMarker(const char* desc);
static void RightAlign(float w);
static void RightAlignedText(const char* txt);
static void RightAlignedColoredText(const ImVec4& color, const char* txt);
static void CenterAlign(float w);
static void CenterAlignedText(const char* txt);
static const GGIcon& getCharIcon(CharacterType charType);
static const GGIcon& getPlayerCharIcon(int playerSide);
ImVec4 RGBToVec(DWORD color);  // color = 0xRRGGBB
static const char* formatBoolean(bool value);
static void pushZeroItemSpacingStyle();
static float getItemSpacing();
static GGIcon DISolIcon = coordsToGGIcon(172, 1096, 56, 35);
static bool SelectionRect(ImVec2* start_pos, ImVec2* end_pos, ImGuiMouseButton mouse_button, bool* isDragging);
static void outlinedText(ImVec2 pos, const char* text);

bool UI::onDllMain(HMODULE hModule) {
	
	for (int i = 0; i < 25; ++i) {
		characterIcons[i] = coordsToGGIcon(1 + 42 * i, 1135, 41, 41);
		characterIconsBorderless[i] = coordsToGGIcon(3 + 42 * i, 1137, 37, 37);
	}
	
	uintptr_t GetKeyStateRData = findImportedFunction("GuiltyGearXrd.exe", "USER32.DLL", "GetKeyState");
	if (GetKeyStateRData) {
		std::vector<char> sig;
		std::vector<char> mask;
		byteSpecificationToSigMask("8b 3d ?? ?? ?? ?? 52 ff d7", sig, mask);
		substituteWildcard(sig, mask, 0, (void*)GetKeyStateRData);
		uintptr_t GetKeyStateCallPlace = sigscanOffset(
			"GuiltyGearXrd.exe",
			sig,
			mask,
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
	addFrameArt(hModule, FT_ACTIVE,
		IDB_ACTIVE_FRAME, activeFrame,
		IDB_ACTIVE_FRAME_NON_COLORBLIND, activeFrameNonColorblind,
		"Active: an attack is currently active. Cannot perform another attack,"
		" but may or may not be able to block or FD - this information is not displayed in the frame color graphic. In general should be unable to block/FD.");
	addImage(hModule, IDB_DIGIT_0, digitFrame[0]);
	addImage(hModule, IDB_DIGIT_1, digitFrame[1]);
	addImage(hModule, IDB_DIGIT_2, digitFrame[2]);
	addImage(hModule, IDB_DIGIT_3, digitFrame[3]);
	addImage(hModule, IDB_DIGIT_4, digitFrame[4]);
	addImage(hModule, IDB_DIGIT_5, digitFrame[5]);
	addImage(hModule, IDB_DIGIT_6, digitFrame[6]);
	addImage(hModule, IDB_DIGIT_7, digitFrame[7]);
	addImage(hModule, IDB_DIGIT_8, digitFrame[8]);
	addImage(hModule, IDB_DIGIT_9, digitFrame[9]);
	addImage(hModule, IDB_FIRST_FRAME, firstFrame);
	addFrameArt(hModule, FT_IDLE, IDB_IDLE_FRAME, idleFrame,
		"Idle: can attack, block and FD.");
	addFrameArt(hModule, FT_IDLE_CANT_BLOCK,
		IDB_IDLE_FRAME_CANT_BLOCK, idleFrameCantBlock,
		IDB_IDLE_FRAME_CANT_BLOCK_NON_COLORBLIND, idleFrameCantBlockNonColorblind,
		"Idle, but can't block: can only attack.");
	addFrameArt(hModule, FT_IDLE_CANT_FD,
		IDB_IDLE_FRAME_CANT_FD, idleFrameCantFD,
		IDB_IDLE_FRAME_CANT_FD_NON_COLORBLIND, idleFrameCantFDNonColorblind,
		"Idle, but can't FD: can only attack and regular block.");
	addFrameArt(hModule, FT_IDLE_ELPHELT_RIFLE,
		IDB_IDLE_FRAME_ELPHELT_RIFLE, idleFrameElpheltRifle,
		IDB_IDLE_FRAME_ELPHELT_RIFLE_NON_COLORBLIND, idleFrameElpheltRifleNonColorblind,
		"Idle: For Elphelt: can cancel stance with specials, but not fire yet. For everyone else: are in stance and able to attack from it. Everyone: Can't block or FD.");
	addFrameArt(hModule, FT_LANDING_RECOVERY,
		IDB_LANDING_RECOVERY_FRAME, landingRecoveryFrame,
		IDB_LANDING_RECOVERY_FRAME_NON_COLORBLIND, landingRecoveryFrameNonColorblind,
		"Landing recovery: can't perform another attack,"
		" but may or may not be able to block or FD - this information is not displayed in the frame color graphic. In general should be unable to block/FD.");
	addFrameArt(hModule, FT_NON_ACTIVE,
		IDB_NON_ACTIVE_FRAME, nonActiveFrame,
		IDB_NON_ACTIVE_FRAME_NON_COLORBLIND, nonActiveFrameNonColorblind,
		"Non-active: a frame inbetween active frames of an attack."
		" Cannot perform another attack,"
		" but may or may not be able to block or FD - this information is not displayed in the frame color graphic. In general should be unable to block/FD.");
	addFrameArt(hModule, FT_PROJECTILE,
		IDB_PROJECTILE_FRAME, projectileFrame,
		IDB_PROJECTILE_FRAME_NON_COLORBLIND, projectileFrameNonColorblind,
		"Projectile: a projectile's attack is active. Can't perform another attack,"
		" but may or may not be able to block or FD - this information is not displayed in the frame color graphic. In general should be unable to block/FD.");
	addFrameArt(hModule, FT_RECOVERY,
		IDB_RECOVERY_FRAME, recoveryFrame,
		IDB_RECOVERY_FRAME_NON_COLORBLIND, recoveryFrameNonColorblind,
		"Recovery: an attack's active frames are already over. Can't perform another attack,"
		" but may or may not be able to block or FD - this information is not displayed in the frame color graphic. In general should be unable to block/FD.");
	addFrameArt(hModule, FT_RECOVERY_HAS_GATLINGS,
		IDB_RECOVERY_FRAME_HAS_GATLINGS, recoveryFrameHasGatlings,
		IDB_RECOVERY_FRAME_HAS_GATLINGS_NON_COLORBLIND, recoveryFrameHasGatlingsNonColorblind,
		"Recovery, but can gatling or cancels: an attack's active frames are already over but can gatling or cancel into some other attacks."
		" May or may not be able to block or FD - this information is not displayed in the frame color graphic. In general should be unable to block/FD.");
	addFrameArt(hModule, FT_RECOVERY_CAN_ACT,
		IDB_RECOVERY_FRAME_CAN_ACT, recoveryFrameCanAct,
		IDB_RECOVERY_FRAME_CAN_ACT_NON_COLORBLIND, recoveryFrameCanActNonColorblind,
		"Recovery, but can act: an attack's active frames are already over but can gatling into some other attacks or jump or airdash."
		" May or may not be able to block or FD - this information is not displayed in the frame color graphic. In general should be unable to block/FD.");
	addFrameArt(hModule, FT_STARTUP,
		IDB_STARTUP_FRAME, startupFrame,
		IDB_STARTUP_FRAME_NON_COLORBLIND, startupFrameNonColorblind,
		"Startup: an attack's active frames have not yet started, or this is not an attack."
		" Can't perform another attack, can't block and can't FD.");
	addFrameArt(hModule, FT_STARTUP_CAN_BLOCK,
		IDB_STARTUP_FRAME_CAN_BLOCK, startupFrameCanBlock,
		IDB_STARTUP_FRAME_CAN_BLOCK_NON_COLORBLIND, startupFrameCanBlockNonColorblind,
		"Startup, but can block:"
		" an attack's active frames have not yet started, or this is not an attack."
		" Can't perform another attack, but can block and/or maybe FD.");
	addFrameMarkerArt(hModule, MARKER_TYPE_STRIKE_INVUL, IDB_STRIKE_INVUL, strikeInvulFrame);
	addFrameMarkerArt(hModule, MARKER_TYPE_SUPER_ARMOR,
		IDB_SUPER_ARMOR_ACTIVE, superArmorFrame,
		IDB_SUPER_ARMOR_ACTIVE_NON_COLORBLIND, superArmorFrameNonColorblind);
	addFrameMarkerArt(hModule, MARKER_TYPE_SUPER_ARMOR_FULL,
		IDB_SUPER_ARMOR_ACTIVE_FULL, superArmorFrameFull,
		IDB_SUPER_ARMOR_ACTIVE_FULL_NON_COLORBLIND, superArmorFrameFullNonColorblind);
	addFrameMarkerArt(hModule, MARKER_TYPE_THROW_INVUL, IDB_THROW_INVUL, throwInvulFrame);
	addFrameArt(hModule, FT_XSTUN,
		IDB_XSTUN_FRAME, xstunFrame,
		IDB_XSTUN_FRAME_NON_COLORBLIND, xstunFrameNonColorblind,
		"Blockstun, hitstun or holding FD: can't perform another attack, or, if in hitstun, then block/FD.");
	
	packedTexture = texturePacker.getTexture();
	
	FrameArt* arrays[2] = { frameArtColorblind, frameArtNonColorblind };
	FrameMarkerArt* arraysMarkers[2] = { frameMarkerArtColorblind, frameMarkerArtNonColorblind };
	for (int i = 0; i < 2; ++i) {
		arrays[i][FT_HITSTOP] = {
			FT_HITSTOP,
			&idleFrame,
			ImVec2{},
			ImVec2{},
			"Hitstop: the time is stopped for this player due to a hit or blocking."
		};
	}
	for (int i = 0; i < 2; ++i) {
		FrameArt* theArray = arrays[i];
		
		FrameArt& startupAnytimeNow = theArray[FT_STARTUP_ANYTIME_NOW];
		startupAnytimeNow = arrays[i][FT_IDLE_ELPHELT_RIFLE];
		startupAnytimeNow.description = "Startup of a chargeable move: can release the button any time to attack. Can't block or FD or perform normal attacks.";
		
		FrameArt& startupStance = theArray[FT_STARTUP_STANCE];
		startupStance = arrays[i][FT_IDLE_ELPHELT_RIFLE];
		startupStance.description = "Being in some form of stance: can cancel into one or more specials. Can't block or FD or perform normal attacks.";
		
		for (int j = 1; j < _countof(frameArtNonColorblind); ++j) {
			FrameArt& art = theArray[j];
			if (!art.resource) {
				MessageBoxW(NULL, L"Null frame art.", L"Error", MB_OK);
				return false;
			}
			art.uvStart = { art.resource->uStart, art.resource->vStart };
			art.uvEnd = { art.resource->uEnd, art.resource->vEnd };
		}
	}
	for (int i = 0; i < 2; ++i) {
		for (int j = 0; j < _countof(frameMarkerArtNonColorblind); ++j) {
			FrameMarkerArt& art = arraysMarkers[i][j];
			if (!art.resource) {
				MessageBoxW(NULL, L"Null frame marker art.", L"Error", MB_OK);
				return false;
			}
			art.uvStart = { art.resource->uStart, art.resource->vStart };
			art.uvEnd = { art.resource->uEnd, art.resource->vEnd };
		}
	}
	for (int i = 0; i < 10; ++i) {
		const PngResource& res = digitFrame[i];
		digitUVs[i] = {
			{ res.uStart, res.vStart },
			{ res.uEnd, res.vEnd }
		};
	}
	
	return true;
}

// Stops queueing new timer events on the window (main) thread. KillTimer does not remove events that have already been queued
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

// Destroys only the graphical resources of imGui
void UI::onDllDetachGraphics() {
	std::unique_lock<std::mutex> guard(lock);
	if (imguiD3DInitialized) {
		logwrap(fputs("imgui freeing D3D resources\n", logfile));
		imguiD3DInitialized = false;
		ImGui_ImplDX9_Shutdown();
	}
}

// Destroys the entire rest of imGui
void UI::onDllDetachNonGraphics() {
	if (imguiD3DInitialized) {
		logwrap(fputs("imgui calling onDllDetachNonGraphics from onDllDetachNonGraphics\n", logfile));
		// this shouldn't happen
		onDllDetachNonGraphics();
	}
	std::unique_lock<std::mutex> guard(lock);
	if (imguiInitialized) {
		logwrap(fputs("imgui freeing non-D3D resources\n", logfile));
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
		imguiInitialized = false;
	}
}

// Runs on the main thread
void UI::prepareDrawData() {
	if (!visible && !needShowFramebar() || gifMode.modDisabled) {
		takeScreenshot = false;
		takeScreenshotPress = false;
		imguiActive = false;
		
		if (imguiInitialized) {
			// When the window is hidden for very long it may become temporarily unresponsive after showing again,
			// and then start quickly processing all the input that was accumulated during the unresponsiveness period (over a span of several frames).
			// However, the thread does not get hung up and both the game and ImGui display fine,
			// it's just that ImGui does not respond to user input for a while.
			// The cause of this is unknown, I haven't tried reproducing it yet and I think it may be related to not doing an ImGui frame every frame.
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();
			ImGui::EndFrame();
		}
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
	
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	
	#define printWithWordWrap \
				float w = ImGui::CalcTextSize(strbuf).x; \
				if (w > ImGui::GetContentRegionAvail().x) { \
					ImGui::TextWrapped(strbuf); \
				} else { \
					if (i == 0) RightAlign(w); \
					ImGui::TextUnformatted(strbuf); \
				}
				
	#define printWithWordWrapArg(a) \
				float w = ImGui::CalcTextSize(a).x; \
				if (w > ImGui::GetContentRegionAvail().x) { \
					ImGui::TextWrapped(a); \
				} else { \
					if (i == 0) RightAlign(w); \
					ImGui::TextUnformatted(a); \
				}
				
	#define printNoWordWrap \
				if (i == 0) RightAlignedText(strbuf); \
				else ImGui::TextUnformatted(strbuf);
				
	#define printNoWordWrapArg(a) \
				if (i == 0) RightAlignedText(a); \
				else ImGui::TextUnformatted(a);
				
	if (visible) {
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
				GGIcon scaledIcon = scaleGGIconToHeight(getPlayerCharIcon(0), 14.F);
				float w = ImGui::CalcTextSize("P1").x + getItemSpacing() + scaledIcon.size.x;
				RightAlign(w);
				drawPlayerIconWithTooltip(0);
				ImGui::SameLine();
				ImGui::TextUnformatted("P1");
				ImGui::TableNextColumn();
				scaledIcon = scaleGGIconToHeight(tipsIcon, 14.F);
				CenterAlign(scaledIcon.size.x);
				drawGGIcon(scaledIcon);
				AddTooltip("Hover your mouse cursor over individual row titles to see their corresponding tooltips.\n"
					"If this is an online match affected by rollback, displayed framedata might be incorrect.");
				ImGui::TableNextColumn();
				ImGui::TextUnformatted("P2");
				ImGui::SameLine();
				drawPlayerIconWithTooltip(1);
				
				{
					PlayerInfo& player = endScene.players[0];
					ImGui::TableNextColumn();
					sprintf_s(strbuf, "[x%s]", printDecimal((player.defenseModifier + 0x100) * 100 / 0x100, 2, 0));
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
					sprintf_s(strbuf, "[x%s]", printDecimal((player.defenseModifier + 0x100) * 100 / 0x100, 2, 0));
					stringArena += strbuf;
					ImGui::TextUnformatted(stringArena.c_str());
				}
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					ImGui::TableNextColumn();
					sprintf_s(strbuf, "%s", printDecimal(player.tension, 2, 0));
					printNoWordWrap
					
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
					printNoWordWrap
					
					if (i == 0) {
						ImGui::TableNextColumn();
						CenterAlignedText("Burst");
					}
				}
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					ImGui::TableNextColumn();
					sprintf_s(strbuf, "%s", printDecimal(player.risc, 2, 0));
					printNoWordWrap
					
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
					printNoWordWrap
					
					if (i == 0) {
						ImGui::TableNextColumn();
						CenterAlignedText("Stun");
					}
				}
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					ImGui::TableNextColumn();
					printDecimal(player.x, 2, 0);
					sprintf_s(strbuf, "%s; ", printdecimalbuf);
					printDecimal(player.y, 2, 0);
					sprintf_s(strbuf + strlen(strbuf), sizeof strbuf - strlen(strbuf), "%s", printdecimalbuf);
					printNoWordWrap
					
					if (i == 0) {
						ImGui::TableNextColumn();
						CenterAlignedText("X; Y");
						AddTooltip("Position X; Y in the arena. Divided by 100 for viewability.");
					}
				}
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					ImGui::TableNextColumn();
					player.printStartup(strbuf, sizeof strbuf);
					printWithWordWrap
					
					if (i == 0) {
						ImGui::TableNextColumn();
						CenterAlignedText("Startup");
						AddTooltip("The startup of the last performed move. The last startup frame is also an active frame.\n"
							"For moves that cause a superfreeze, such as RC, the startup of the superfreeze is displayed.\n"
							"The startup of the move may consist of multiple numbers, separated by +. In that case:\n"
							"1) If the move caused a superfreeze, and that superfreeze occured before active frames,"
							" it's displayed as the startup of the superfreeze + startup after superfreeze;\n"
							"2) If the move only caused a superfreeze and no attack (for example, it's RC), then only the startup of the superfreeze"
							" is displayed;\n"
							"3) If the move can be held, such as Blitz Shield, May 6P, May 6H, Johnny Mist Finer, etc, then the startup is displayed"
							" as everything up to releasing the button + startup after releasing the button. Except Johnny Mist Finer and many"
							" other moves additionally"
							" break up into: the number of frames before the move enters the portion that can be held"
							" + the amount of frames you held after that + startup after you released the button. Elphelt Ms. Confille breaks up"
							" into frames it takes to become able to do other moves, and then, over a +, extra frames it takes to become able to"
							" fire. Other moves may break up similarly;\n"
							"4) If a move was RC'd, the move's frames are shown first, then +, then RC's frames;\n"
							"5) If a move is a follow-up move, the first move's frames are shown, then the follow-up's. Not all follow-ups are"
							" displayed like this - they reset the entire display instead, by restarting the startup/total from 1, without + sign;\n"
							"6) Baiken canceling Azami into another Azami or the follow-ups, causes them to be displayed in addition to what happened"
							" before, over a + sign;\n"
							"7) Some other moves may get combined with the ones they were performed from as well, using the + sign.");
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
					printWithWordWrap
					
					if (i == 0) {
						ImGui::TableNextColumn();
						CenterAlignedText("Active");
						AddTooltip("Number of active frames in the last performed move.\n"
							"Numbers in (), when surrounded by other numbers, mean non-active frames inbetween active frames."
							" So, for example, 1(2)3 would mean you were active for 1 frame, then were not active for 2 frames, then were active again for 3.\n"
							"Numbers separated by a , symbol mean active frames of separate distinct hits, between which there is no gap of non-active frames."
							" For example, 1,4 would mean that the move is active for 5 frames, and the first frame is hit one, while frames 2-5 are hit two."
							" The attack need not actually land those hits, and some moves may be limited by the max number of hits they can deal, which means"
							" the displayed number of hits might not represent the actual number of hits dealt.\n"
							"Sometimes, when the number of hits is too great, an alternative representation of active frames will be displayed over a / sign."
							" For example: 13 / 1,1,1,1,1,1,1,1,1,1,1,1,1. Means there're 13 active frames, and over the /, each individual hit's active frames"
							" are shown.\n"
							"If the move spawned one or more projectiles, and the hits of projectiles overlap with each other or with the player's hits, then the"
							" individual hits' information is discarded in the overlapping spans and they're shown as one hit. For example,"
							" Sol DI Ground Viper spawns vertical pillars of fire in its path, as Sol himself is hitting from below. The hits of the pillars are"
							" out of sync with Sol's hits and so everything is displayed as just one number representing the total duration of all active frames.\n"
							"If the move spawned one or more projectiles, or Eddie, and that projectile entered hitstop due to the opponent blocking it or"
							" getting hit by it, then the displayed number of active frames may be larger than it is on whiff, because the hitstop gets added"
							" to it.\n"
							"If, while the move was happening, some projectile unrelated to the move had active frames, those are not included in the move's"
							" active frames.\n"
							"If active frames start during superfreeze, the active frames will include the frame that happened during superfreeze and"
							" all the active frames that were after the superfreeze. For example, a move has superfreeze startup 1"
							" (meaning superfreeze starts in 1 frame), +0 startup after superfreeze (which means that it starts during the superfreeze),"
							" and 2 active frames after the superfreeze. The displayed result for active frames will be: 3.");
					}
				}
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					ImGui::TableNextColumn();
					player.printRecovery(strbuf, sizeof strbuf);
					printWithWordWrap
					
					if (i == 0) {
						ImGui::TableNextColumn();
						CenterAlignedText("Recovery");
						AddTooltip("Number of recovery frames in the last performed move."
							" If the move spawned a projectile that lasted beyond the boundaries of the move, its recovery is 0.\n"
							"See the tooltip for the 'Total' field for more details.");
					}
				}
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					ImGui::TableNextColumn();
					player.printTotal(strbuf, sizeof strbuf);
					printWithWordWrap
					
					if (i == 0) {
						ImGui::TableNextColumn();
						CenterAlignedText("Total");
						AddTooltip("Total number of frames in the last performed move during which you've been unable to act.\n"
							"If the move spawned a projectiled that lasted beyond the boundaries of the move, this field will display"
							" only the amount of frames it took to recover, from the start of the move."
							" So for example, if a move's startup is 1, it creates a projectile that lasts 100 frames and recovers instantly,"
							" on frame 2, then its total frames will be 1, its startup will be 1 and its actives will be 100.\n"
							"If you performed an air move that has landing recovery, the"
							" landing recovery is included in the display as '+X landing'.\n"
							"If you performed an air move and recovered in the air, then the time you spent in the air idle is not included"
							" in the recovery or 'Total' frames. Even if such move had landing recovery, it will display only the frames,"
							" during which you were 'busy', will not include the idle time spent in the air, and will add a '+X landing'"
							" to the recovery.\n"
							"If you performed an air normal or similar air move without landing recovery, and it got canceled by"
							" landing, normally there's 1 frame upon landing during which normals can't be used but blocking is possible."
							" This frame is not included in the total frames as it is not considered part of the move.\n"
							"If the move recovery lets you attack first and then some times passes and then it lets you block, or vice versa"
							" the display will say either 'X can't block+Y can't attack' or 'X can't attack+Y can't block'. In this case"
							" the first part is the number of frames during which you were unable to block/attack and the second part is"
							" the number of frames during which you were unable to attack/block.\n"
							"If the move was jump canceled, the prejump frames and the jump are not included in neither the recovery nor 'Total'.");
					}
				}
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					ImGui::TableNextColumn();
					player.printInvuls(strbuf, sizeof strbuf);
					printWithWordWrap
					
					if (i == 0) {
						ImGui::TableNextColumn();
						CenterAlignedText("Invul");
						AddTooltip("Strike invul: invulnerable to strike and projectiles.\n"
							"Throw invul: invulnerable to throws.\n"
							"Projectile-only invul: only vulnerable to direct player strikes or throws.\n"
							"Super armor: parry or super armor.\n"
							"Reflect: able to reflect certain types of projectiles.\n\n"
							"All given frame ranges begin from the moment you started performing the move or, if"
							" you performed multiple moves and their display got combined with a + sign in"
							" the 'Startup' field, then from the start of the first move. If the moves are combined"
							" in the 'Total' field, but not the 'Startup' field, then the ranges begin from the start"
							" of the last move.\n\n"
							"List of moves that are unblockable:\n"
							"*) Answer Taunt;\n"
							"*) Axl Haitaka Stance max charge;\n"
							"*) Bedman Hemi Jack;\n"
							"*) Dizzy Taunt;\n"
							"*) Elphelt Ms. Confille maximum charge;\n"
							"*) Faust Platform;\n"
							"*) Faust 100-ton Weight;\n"
							"*) Faust 10,000 Ton Weight;\n"
							"*) Faust Hack'n'Slash if the opponent is grounded;\n"
							"*) I-No Sterilization Method;\n"
							"*) Johnny Bacchus Sigh + Mist Finer if the opponent is airborne;\n"
							"*) Johnny Treasure Hunt max charge;\n"
							"*) Kum Enlightened 3000 Palm Strike max charge;\n"
							"*) Potemkin Slidehead shockwave;\n"
							"*) Potemkin Heat Knuckle;\n"
							"*) Potemkin Heavenly Potemkin Buster;\n"
							"*) Raven S Wachen Zweig;\n"
							"*) Slayer Undertow.\n\n"
							"Low profile invul can be configured using Settings - General Settings - Low Profile Cut-Off Height");
					}
				}
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					ImGui::TableNextColumn();
					*strbuf = '\0';
					if (player.displayHitstop) {
						sprintf_s(strbuf, "%d/%d", player.hitstop, player.hitstopMax);
					}
					char* ptrNext = strbuf;
					int ptrNextSize = sizeof strbuf;
					if (*strbuf) {
						ptrNext += strlen(strbuf) + strlen(" + ");
						*ptrNext = '\0';
						ptrNextSize -= (ptrNext - strbuf);
					}
					size_t ptrNextSizeCap = ptrNextSize < 0 ? 0 : (size_t)ptrNextSize;
					if (player.xStunDisplay == PlayerInfo::XSTUN_DISPLAY_HIT) {
						sprintf_s(ptrNext, ptrNextSizeCap, "%d/%d", player.hitstun - (player.hitstop ? 1 : 0), player.hitstunMax);
					} else if (player.xStunDisplay == PlayerInfo::XSTUN_DISPLAY_BLOCK) {
						sprintf_s(ptrNext, ptrNextSizeCap, "%d/%d", player.blockstun - (player.hitstop ? 1 : 0), player.blockstunMax);
					}
					if (strbuf != ptrNext && *strbuf && *ptrNext) {
						ptrNext = strbuf + strlen(strbuf);
						memcpy(ptrNext, " + ", 3);
					}
					printNoWordWrap
					
					if (i == 0) {
						ImGui::TableNextColumn();
						CenterAlignedText("Hitstop+X-stun");
						AddTooltip("Displays current hitstop/max hitstop + current hitstun or blockstun /"
							" max hitstun or blockstun. When there's no + sign, the displayed values could"
							" either be hitstop, or hitstun or blockstun, but if both are displayed, hitstop is always on the left,"
							" and the other are on the right.\n"
							"During Roman Cancel or Mortal Counter slowdown, the actual hitstop and hitstun/etc duration may be longer"
							" than the displayed value due to slowdown.");
					}
				}
				
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					
					ImGui::TableNextColumn();
					frameAdvantageControl(
						player.frameAdvantage,
						player.landingFrameAdvantage,
						player.frameAdvantageValid,
						player.landingFrameAdvantageValid,
						i == 0);
					
					if (i == 0) {
						ImGui::TableNextColumn();
						const char* txt = "Frame Adv.";
						CenterAlign(ImGui::CalcTextSize(txt).x);
						ImGui::PushStyleColor(ImGuiCol_HeaderHovered, { 0.F, 0.F, 0.F, 0.F });
						if (ImGui::Selectable(txt)) {
							showFrameAdvTooltip = !showFrameAdvTooltip;
						}
						ImGui::PopStyleColor();
						AddTooltip("Click the field for tooltip.");
					}
				}
				
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					ImGui::TableNextColumn();
					player.printGaps(strbuf, sizeof strbuf);
					printWithWordWrap
					
					if (i == 0) {
						ImGui::TableNextColumn();
						CenterAlignedText("Gaps");
						AddTooltip("Each gap is the number of frames from when the opponent left blockstun to when they entered blockstun again.");
					}
				}
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					ImGui::TableNextColumn();
					sprintf_s(strbuf, "%s", formatBoolean(player.pawn ? player.pawn.inBlockstunNextFrame() : false));
					printNoWordWrap
					
					if (i == 0) {
						ImGui::TableNextColumn();
						CenterAlignedText("inBlockstunNextFrame");
					}
				}
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					ImGui::TableNextColumn();
					sprintf_s(strbuf, "%s", formatBoolean(player.pawn ? player.pawn.successfulIB() : false));
					printNoWordWrap
					
					if (i == 0) {
						ImGui::TableNextColumn();
						CenterAlignedText("successfulIB");
					}
				}
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					ImGui::TableNextColumn();
					sprintf_s(strbuf, "%s", formatBoolean(player.pawn ? player.pawn.holdingFD() : false));
					printNoWordWrap
					
					if (i == 0) {
						ImGui::TableNextColumn();
						CenterAlignedText("holdingFD");
					}
				}
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					ImGui::TableNextColumn();
					sprintf_s(strbuf, "%s", formatBoolean(player.idle));
					printNoWordWrap
					
					if (i == 0) {
						ImGui::TableNextColumn();
						CenterAlignedText("idle");
					}
				}
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					ImGui::TableNextColumn();
					sprintf_s(strbuf, "%s", formatBoolean(player.canBlock));
					printNoWordWrap
					
					if (i == 0) {
						ImGui::TableNextColumn();
						CenterAlignedText("canBlock");
					}
				}
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					ImGui::TableNextColumn();
					sprintf_s(strbuf, "%s", formatBoolean(player.canFaultlessDefense));
					printNoWordWrap
					
					if (i == 0) {
						ImGui::TableNextColumn();
						CenterAlignedText("canFaultlessDefense");
					}
				}
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					ImGui::TableNextColumn();
					sprintf_s(strbuf, "%s (%d)", formatBoolean(player.idlePlus), player.timePassed);
					printNoWordWrap
					
					if (i == 0) {
						ImGui::TableNextColumn();
						CenterAlignedText("idlePlus");
					}
				}
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					ImGui::TableNextColumn();
					sprintf_s(strbuf, "%s (%d)", formatBoolean(player.idleLanding), player.timePassedLanding);
					printNoWordWrap
					
					if (i == 0) {
						ImGui::TableNextColumn();
						CenterAlignedText("idleLanding");
					}
				}
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					ImGui::TableNextColumn();
					if (!player.move || !player.move->displayName) {
						printNoWordWrapArg(player.anim)
					} else {
						sprintf_s(strbuf, "%s (%s)", player.move->displayName, player.anim);
						printWithWordWrap
					}
					
					if (i == 0) {
						ImGui::TableNextColumn();
						CenterAlignedText("anim");
					}
				}
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					ImGui::TableNextColumn();
					sprintf_s(strbuf, "%d", player.animFrame);
					printNoWordWrap
					
					if (i == 0) {
						ImGui::TableNextColumn();
						CenterAlignedText("animFrame");
					}
				}
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					ImGui::TableNextColumn();
					player.sprite.print(strbuf, sizeof strbuf);
					printNoWordWrap
					
					if (i == 0) {
						ImGui::TableNextColumn();
						CenterAlignedText("sprite");
					}
				}
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					ImGui::TableNextColumn();
					sprintf_s(strbuf, "%d", player.startup);
					printNoWordWrap
					
					if (i == 0) {
						ImGui::TableNextColumn();
						CenterAlignedText("startup");
					}
				}
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					ImGui::TableNextColumn();
					player.actives.print(strbuf, sizeof strbuf);
					printWithWordWrap
					
					if (i == 0) {
						ImGui::TableNextColumn();
						CenterAlignedText("active");
					}
				}
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					ImGui::TableNextColumn();
					sprintf_s(strbuf, "%d", player.recovery);
					printNoWordWrap
					
					if (i == 0) {
						ImGui::TableNextColumn();
						CenterAlignedText("recovery");
					}
				}
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					ImGui::TableNextColumn();
					sprintf_s(strbuf, "%d", player.total);
					printNoWordWrap
					
					if (i == 0) {
						ImGui::TableNextColumn();
						CenterAlignedText("total");
					}
				}
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					ImGui::TableNextColumn();
					sprintf_s(strbuf, "%d", player.startupProj);
					printNoWordWrap
					
					if (i == 0) {
						ImGui::TableNextColumn();
						CenterAlignedText("startupProj");
					}
				}
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					ImGui::TableNextColumn();
					player.activesProj.print(strbuf, sizeof strbuf);
					printWithWordWrap
					
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
						sprintf_s(strbuf, "true (%d)", endScene.getSuperflashCounterAllied());
					} else if (superflashInstigator) {
						sprintf_s(strbuf, "true (%d)", endScene.getSuperflashCounterOpponent());
					} else {
						strcpy(strbuf, "false");
					}
					printNoWordWrap
					
					if (i == 0) {
						ImGui::TableNextColumn();
						CenterAlignedText("superfreeze");
					}
				}
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					ImGui::TableNextColumn();
					if (player.hurtboxTopBottomValid) {
						sprintf_s(strbuf, "from %d to %d",
							player.hurtboxTopY,
							player.hurtboxBottomY);
						printNoWordWrap
					}
					
					if (i == 0) {
						ImGui::TableNextColumn();
						CenterAlignedText("Hurtbox Y");
						AddTooltip("These values display either the current or last valid value and change each frame."
							" They do not show the total combined bounding box."
							" To view these values for each frame more easily you could use the frame freeze mode,"
							" available in the Hitboxes section.\n"
							"The coordinates shown are relative to the global space.");
					}
				}
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					ImGui::TableNextColumn();
					if (player.hitboxTopBottomValid) {
						sprintf_s(strbuf, "from %d to %d",
							player.hitboxTopY,
							player.hitboxBottomY);
						printNoWordWrap
					}
					
					if (i == 0) {
						ImGui::TableNextColumn();
						CenterAlignedText("Hitbox Y");
						AddTooltip("These values display either the current or last valid value and change each frame."
							" They do not show the total combined bounding box."
							" To view these values for each frame more easily you could use the frame freeze mode,"
							" available in the Hitboxes section.\n"
							"The coordinates shown are relative to the global space.");
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
									printNoWordWrap
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
									printNoWordWrap
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
									printNoWordWrapArg(projectile.creatorName)
								}
								
								if (i == 0) {
									ImGui::TableNextColumn();
									CenterAlignedText("creator");
								}
							}
							for (int i = 0; i < 2; ++i) {
								ImGui::TableNextColumn();
								if (row.side[i]) {
									ProjectileInfo& projectile = *row.side[i];
									printNoWordWrapArg(projectile.animName)
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
									printNoWordWrap
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
									projectile.sprite.print(strbuf, sizeof strbuf);
									printNoWordWrap
								}
								
								if (i == 0) {
									ImGui::TableNextColumn();
									CenterAlignedText("sprite");
								}
							}
							for (int i = 0; i < 2; ++i) {
								ImGui::TableNextColumn();
								if (row.side[i]) {
									ProjectileInfo& projectile = *row.side[i];
									sprintf_s(strbuf, "%d", projectile.hitstop);
									printNoWordWrap
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
									
									printWithWordWrap
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
									projectile.printStartup(strbuf, sizeof strbuf);
									printNoWordWrap
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
									projectile.printTotal(strbuf, sizeof strbuf);
									printNoWordWrap
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
									printNoWordWrap
								}
								
								if (i == 0) {
									ImGui::TableNextColumn();
									CenterAlignedText("disabled");
								}
							}
							for (int i = 0; i < 2; ++i) {
								ImGui::TableNextColumn();
								if (row.side[i]) {
									ProjectileInfo& projectile = *row.side[i];
									if (projectile.hitboxTopBottomValid) {
										sprintf_s(strbuf, "from %d to %d",
											projectile.hitboxTopY,
											projectile.hitboxBottomY);
										printNoWordWrap
									}
								}
								
								if (i == 0) {
									ImGui::TableNextColumn();
									CenterAlignedText("Hitbox Y");
									AddTooltip("These values display either the current or last valid value and change each frame."
										" They do not show the total combined bounding box."
										" To view these values for each frame more easily you could use the frame freeze mode,"
										" available in the Hitboxes section.\n"
										"The coordinates shown are relative to the global space.");
								}
							}
						}
						ImGui::EndTable();
					}
				}
			}
			if (ImGui::Button("Show Tension Values")) {
				showTensionData = !showTensionData;
			}
			ImGui::SameLine();
			if (ImGui::Button("Speed/Proration")) {
				showSpeedsData = !showSpeedsData;
			}
			for (int i = 0; i < 2; ++i) {
				sprintf_s(strbuf, i == 0 ? "Character Specific (P%d)" : "... (P%d)", i + 1);
				if (i != 0) ImGui::SameLine();
				if (ImGui::Button(strbuf)) {
					showCharSpecific[i] = !showCharSpecific[i];
				}
			}
		}
		if (ImGui::CollapsingHeader("Hitboxes")) {
			stateChanged = ImGui::Checkbox("GIF Mode", &gifModeOn) || stateChanged;
			ImGui::SameLine();
			HelpMarker("GIF mode is:\n"
				"; 1) Background becomes black\n"
				"; 2) Camera is centered on you\n"
				"; 3) Opponent is invisible and invulnerable\n"
				"; 4) Hide HUD");
			
			stateChanged = ImGui::Checkbox("Black Background", &gifModeToggleBackgroundOnly) || stateChanged;
			ImGui::SameLine();
			HelpMarker("Makes background black (and, for screenshotting purposes, - effectively transparent, if Post Effect is turned off in the game's graphics settings).");
			
			bool postEffectOn = game.postEffectOn() != 0;
			if (ImGui::Checkbox("Post-Effect On", &postEffectOn)) {
				game.postEffectOn() = (BOOL)postEffectOn;
			}
			ImGui::SameLine();
			HelpMarker("Toggles the game's Settings - Display Settings - Post-Effect. Changing it this way does not"
				" require the current match to be restarted.\n"
				"Alternatively, you could tick the 'Turn Off Post-Effect When Making Background Black' checkbox"
				" (in this mod's Settings - Hitbox Settings),"
				" so that whenever you enter either the GIF mode or the GIF mode (black background only), the Post-Effect are"
				" turned off automatically, and when you leave those modes, it gets turned back on.\n"
				"Or, alternatively, you could use the manual keyboard toggle, set in this mod's Settings - Keyboard settings"
				" - Toggle Post-Effect On/Off.");
			
			stateChanged = ImGui::Checkbox("Camera Center on Player", &gifModeToggleCameraCenterOnly) || stateChanged;
			ImGui::SameLine();
			HelpMarker("Centers the camera on you.");
			
			stateChanged = ImGui::Checkbox("Camera Center on Opponent", &toggleCameraCenterOpponent) || stateChanged;
			ImGui::SameLine();
			HelpMarker("Centers the camera on the opponent.");
			
			stateChanged = ImGui::Checkbox("Hide Opponent", &gifModeToggleHideOpponentOnly) || stateChanged;
			ImGui::SameLine();
			HelpMarker("Make the opponent invisible and invulnerable.");
			
			stateChanged = ImGui::Checkbox("Hide Player", &toggleHidePlayer) || stateChanged;
			ImGui::SameLine();
			HelpMarker("Make the player invisible and invulnerable.");
			
			stateChanged = ImGui::Checkbox("Hide HUD", &gifModeToggleHudOnly) || stateChanged;
			ImGui::SameLine();
			HelpMarker("Hides the HUD.");
			
			stateChanged = ImGui::Checkbox("No Gravity", &noGravityOn) || stateChanged;
			ImGui::SameLine();
			HelpMarker("Prevents you from falling, meaning you remain in the air as long as 'No Gravity Mode' is enabled.");
			
			bool neverDisplayGrayHurtboxes = settings.neverDisplayGrayHurtboxes;
			if (ImGui::Checkbox("Disable Gray Hurtboxes", &neverDisplayGrayHurtboxes)) {
				settings.neverDisplayGrayHurtboxes = neverDisplayGrayHurtboxes;
				needWriteSettings = true;
			}
			ImGui::SameLine();
			HelpMarker("Disables/enables the display of residual hurtboxes that appear on hit/block and show"
				" the defender's hurtbox at the moment of impact. These hurtboxes display for only a brief time on impacts but"
				" they can get in the way when trying to do certain stuff such as take screenshots of hurtboxes.");
			
			stateChanged = ImGui::Checkbox("Freeze Game", &freezeGame) || stateChanged;
			ImGui::SameLine();
			if (ImGui::Button("Next Frame")) {
				allowNextFrame = true;
				allowNextFrameTimer = 10;
			}
			ImGui::SameLine();
			HelpMarker("Freezes the current frame of the game and stops gameplay from advancing. You can advance gameplay to the next frame using the 'Next Frame' button."
				" It is way more convenient to use this feature with shortcuts, which you can configure in the 'Keyboard shortcuts' section below.");
			
			stateChanged = ImGui::Checkbox("Slow-Mo Mode", &slowmoGame) || stateChanged;
			ImGui::SameLine();
			int slowmoTimes = settings.slowmoTimes;
			ImGui::SetNextItemWidth(80.F);
			if (ImGui::InputInt("Slow-Mo Factor", &slowmoTimes, 1, 1, 0)) {
				if (slowmoTimes <= 0) {
					slowmoTimes = 1;
				}
				settings.slowmoTimes = slowmoTimes;
				needWriteSettings = true;
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
				" If you make background black using 'GIF Mode Enabled' and set Post Effect to off in the game's graphics settings"
				" (or use 'Settings - Keyboard Shortcuts - Toggle Post-Effect On/Off' or 'Settings - Hitbox Settings - Turn Off Post-Effect' When Making Background Black)"
				", you will be able to take screenshots with transparency. Screenshots are copied to clipboard by default, but if 'Screenshots path' is set"
				" in the 'Hitbox settings', they're saved there instead.\n"
				"A hotkey can be configured to take screenshots with, in 'Settings - Keyboard Shortcuts - Take Screenshot'.");
			stateChanged = ImGui::Checkbox("Continuous Screenshotting Mode", &continuousScreenshotToggle) || stateChanged;
			ImGui::SameLine();
			HelpMarker("When this option is enabled, screenshots will be taken every frame, unless the game is frozen, in which case"
				" a new screenshot is taken only when the frame advances. You can run out of disk space pretty fast with this and it slows"
				" the game down significantly. Continuous screenshotting is only allowed in training mode.");
			
		}
		if (ImGui::CollapsingHeader("Settings")) {
			if (ImGui::CollapsingHeader("Hitbox Settings")) {
				
				bool dontShowBoxes = settings.dontShowBoxes;
				if (ImGui::Checkbox(settings.getOtherUIName(&settings.dontShowBoxes), &dontShowBoxes)) {
					settings.dontShowBoxes = dontShowBoxes;
					needWriteSettings = true;
				}
				ImGui::SameLine();
				HelpMarker(settings.getOtherUIDescription(&settings.dontShowBoxes));
				
				bool drawPushboxCheckSeparately = settings.drawPushboxCheckSeparately;
				if (ImGui::Checkbox(settings.getOtherUIName(&settings.drawPushboxCheckSeparately), &drawPushboxCheckSeparately)) {
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
				
				ImGui::Text(settings.getOtherUIName(&settings.screenshotPath));
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
				if (ImGui::Checkbox(settings.getOtherUIName(&settings.allowContinuousScreenshotting), &allowContinuousScreenshotting)) {
					settings.allowContinuousScreenshotting = allowContinuousScreenshotting;
					needWriteSettings = true;
				}
				ImGui::SameLine();
				HelpMarker(settings.getOtherUIDescription(&settings.allowContinuousScreenshotting));
				
				bool dontUseScreenshotTransparency = settings.dontUseScreenshotTransparency;
				if (ImGui::Checkbox(settings.getOtherUIName(&settings.dontUseScreenshotTransparency), &dontUseScreenshotTransparency)) {
					settings.dontUseScreenshotTransparency = dontUseScreenshotTransparency;
				}
				ImGui::SameLine();
				HelpMarker(settings.getOtherUIDescription(&settings.dontUseScreenshotTransparency));
				
				bool useSimplePixelBlender = settings.useSimplePixelBlender;
				if (ImGui::Checkbox(settings.getOtherUIName(&settings.useSimplePixelBlender), &useSimplePixelBlender)) {
					settings.useSimplePixelBlender = useSimplePixelBlender;
					needWriteSettings = true;
				}
				ImGui::SameLine();
				HelpMarker(settings.getOtherUIDescription(&settings.useSimplePixelBlender));
				
				bool turnOffPostEffectWhenMakingBackgroundBlack = settings.turnOffPostEffectWhenMakingBackgroundBlack;
				if (ImGui::Checkbox(settings.getOtherUIName(&settings.turnOffPostEffectWhenMakingBackgroundBlack), &turnOffPostEffectWhenMakingBackgroundBlack)) {
					settings.turnOffPostEffectWhenMakingBackgroundBlack = turnOffPostEffectWhenMakingBackgroundBlack;
					if (turnOffPostEffectWhenMakingBackgroundBlack) {
						endScene.onGifModeBlackBackgroundChanged();
					}
					needWriteSettings = true;
				}
				ImGui::SameLine();
				HelpMarker(settings.getOtherUIDescription(&settings.turnOffPostEffectWhenMakingBackgroundBlack));
				
			}
			if (ImGui::CollapsingHeader("Framebar Settings")) {
				bool neverIgnoreHitstop = settings.neverIgnoreHitstop;
				if (ImGui::Checkbox(settings.getOtherUIName(&settings.neverIgnoreHitstop), &neverIgnoreHitstop)) {
					settings.neverIgnoreHitstop = neverIgnoreHitstop;
					needWriteSettings = true;
				}
				ImGui::SameLine();
				HelpMarker(settings.getOtherUIDescription(&settings.neverIgnoreHitstop));
				
				bool considerRunAndWalkNonIdle = settings.considerRunAndWalkNonIdle;
				if (ImGui::Checkbox(settings.getOtherUIName(&settings.considerRunAndWalkNonIdle), &considerRunAndWalkNonIdle)) {
					settings.considerRunAndWalkNonIdle = considerRunAndWalkNonIdle;
					needWriteSettings = true;
				}
				ImGui::SameLine();
				HelpMarker(settings.getOtherUIDescription(&settings.considerRunAndWalkNonIdle));
				
				bool considerCrouchNonIdle = settings.considerCrouchNonIdle;
				if (ImGui::Checkbox(settings.getOtherUIName(&settings.considerCrouchNonIdle), &considerCrouchNonIdle)) {
					settings.considerCrouchNonIdle = considerCrouchNonIdle;
					needWriteSettings = true;
				}
				ImGui::SameLine();
				HelpMarker(settings.getOtherUIDescription(&settings.considerCrouchNonIdle));
				
				bool considerKnockdownWakeupAndAirtechIdle = settings.considerKnockdownWakeupAndAirtechIdle;
				if (ImGui::Checkbox(settings.getOtherUIName(&settings.considerKnockdownWakeupAndAirtechIdle), &considerKnockdownWakeupAndAirtechIdle)) {
					settings.considerKnockdownWakeupAndAirtechIdle = considerKnockdownWakeupAndAirtechIdle;
					needWriteSettings = true;
				}
				ImGui::SameLine();
				HelpMarker(settings.getOtherUIDescription(&settings.considerKnockdownWakeupAndAirtechIdle));
				
				bool useColorblindHelp = settings.useColorblindHelp;
				if (ImGui::Checkbox(settings.getOtherUIName(&settings.useColorblindHelp), &useColorblindHelp)) {
					settings.useColorblindHelp = useColorblindHelp;
					needWriteSettings = true;
				}
				ImGui::SameLine();
				HelpMarker(settings.getOtherUIDescription(&settings.useColorblindHelp));
				
				bool showFramebar = settings.showFramebar;
				if (ImGui::Checkbox(settings.getOtherUIName(&settings.showFramebar), &showFramebar)) {
					settings.showFramebar = showFramebar;
					needWriteSettings = true;
				}
				ImGui::SameLine();
				HelpMarker(settings.getOtherUIDescription(&settings.showFramebar));
				
				bool showFramebarInTrainingMode = settings.showFramebarInTrainingMode;
				if (ImGui::Checkbox(settings.getOtherUIName(&settings.showFramebarInTrainingMode), &showFramebarInTrainingMode)) {
					settings.showFramebarInTrainingMode = showFramebarInTrainingMode;
					needWriteSettings = true;
				}
				ImGui::SameLine();
				HelpMarker(settings.getOtherUIDescription(&settings.showFramebarInTrainingMode));
				
				bool showFramebarInReplayMode = settings.showFramebarInReplayMode;
				if (ImGui::Checkbox(settings.getOtherUIName(&settings.showFramebarInReplayMode), &showFramebarInReplayMode)) {
					settings.showFramebarInReplayMode = showFramebarInReplayMode;
					needWriteSettings = true;
				}
				ImGui::SameLine();
				HelpMarker(settings.getOtherUIDescription(&settings.showFramebarInReplayMode));
				
				bool showFramebarInOtherModes = settings.showFramebarInOtherModes;
				if (ImGui::Checkbox(settings.getOtherUIName(&settings.showFramebarInOtherModes), &showFramebarInOtherModes)) {
					settings.showFramebarInOtherModes = showFramebarInOtherModes;
					needWriteSettings = true;
				}
				ImGui::SameLine();
				HelpMarker(settings.getOtherUIDescription(&settings.showFramebarInOtherModes));
				
				bool closingModWindowAlsoHidesFramebar = settings.closingModWindowAlsoHidesFramebar;
				if (ImGui::Checkbox(settings.getOtherUIName(&settings.closingModWindowAlsoHidesFramebar), &closingModWindowAlsoHidesFramebar)) {
					settings.closingModWindowAlsoHidesFramebar = closingModWindowAlsoHidesFramebar;
					needWriteSettings = true;
				}
				ImGui::SameLine();
				HelpMarker(settings.getOtherUIDescription(&settings.closingModWindowAlsoHidesFramebar));
				
				bool showStrikeInvulOnFramebar = settings.showStrikeInvulOnFramebar;
				if (ImGui::Checkbox(settings.getOtherUIName(&settings.showStrikeInvulOnFramebar), &showStrikeInvulOnFramebar)) {
					settings.showStrikeInvulOnFramebar = showStrikeInvulOnFramebar;
					needWriteSettings = true;
				}
				ImGui::SameLine();
				HelpMarker(settings.getOtherUIDescription(&settings.showStrikeInvulOnFramebar));
				
				bool showThrowInvulOnFramebar = settings.showThrowInvulOnFramebar;
				if (ImGui::Checkbox(settings.getOtherUIName(&settings.showThrowInvulOnFramebar), &showThrowInvulOnFramebar)) {
					settings.showThrowInvulOnFramebar = showThrowInvulOnFramebar;
					needWriteSettings = true;
				}
				ImGui::SameLine();
				HelpMarker(settings.getOtherUIDescription(&settings.showThrowInvulOnFramebar));
				
				bool showSuperArmorOnFramebar = settings.showSuperArmorOnFramebar;
				if (ImGui::Checkbox(settings.getOtherUIName(&settings.showSuperArmorOnFramebar), &showSuperArmorOnFramebar)) {
					settings.showSuperArmorOnFramebar = showSuperArmorOnFramebar;
					needWriteSettings = true;
				}
				ImGui::SameLine();
				HelpMarker(settings.getOtherUIDescription(&settings.showSuperArmorOnFramebar));
				
				bool showFirstFramesOnFramebar = settings.showFirstFramesOnFramebar;
				if (ImGui::Checkbox(settings.getOtherUIName(&settings.showFirstFramesOnFramebar), &showFirstFramesOnFramebar)) {
					settings.showFirstFramesOnFramebar = showFirstFramesOnFramebar;
					needWriteSettings = true;
				}
				ImGui::SameLine();
				HelpMarker(settings.getOtherUIDescription(&settings.showFirstFramesOnFramebar));
				
				int framebarHeight = settings.framebarHeight;
				ImGui::SetNextItemWidth(80.F);
				if (ImGui::InputInt(settings.getOtherUIName(&settings.framebarHeight), &framebarHeight, 1, 1, 0)) {
					if (framebarHeight <= 0) {
						framebarHeight = 1;
					}
					settings.framebarHeight = framebarHeight;
					needWriteSettings = true;
				}
				imguiActiveTemp = imguiActiveTemp || ImGui::IsItemActive();
				ImGui::SameLine();
				HelpMarker(settings.getOtherUIDescription(&settings.framebarHeight));
				
				int lowProfileCutoffPoint = settings.lowProfileCutoffPoint;
				ImGui::SetNextItemWidth(200.F);
				if (ImGui::InputInt(settings.getOtherUIName(&settings.lowProfileCutoffPoint), &lowProfileCutoffPoint, 1000, 10000, 0)) {
					settings.lowProfileCutoffPoint = lowProfileCutoffPoint;
					needWriteSettings = true;
				}
				imguiActiveTemp = imguiActiveTemp || ImGui::IsItemActive();
				ImGui::SameLine();
				HelpMarker(settings.getOtherUIDescription(&settings.lowProfileCutoffPoint));
				
			}
			if (ImGui::CollapsingHeader("Keyboard Shortcuts")) {
				keyComboControl(settings.modWindowVisibilityToggle);
				keyComboControl(settings.framebarVisibilityToggle);
				keyComboControl(settings.gifModeToggle);
				keyComboControl(settings.gifModeToggleBackgroundOnly);
				keyComboControl(settings.togglePostEffectOnOff);
				keyComboControl(settings.gifModeToggleCameraCenterOnly);
				keyComboControl(settings.toggleCameraCenterOpponent);
				keyComboControl(settings.gifModeToggleHideOpponentOnly);
				keyComboControl(settings.toggleHidePlayer);
				keyComboControl(settings.gifModeToggleHudOnly);
				keyComboControl(settings.noGravityToggle);
				keyComboControl(settings.freezeGameToggle);
				keyComboControl(settings.slowmoGameToggle);
				keyComboControl(settings.allowNextFrameKeyCombo);
				keyComboControl(settings.disableHitboxDisplayToggle);
				keyComboControl(settings.screenshotBtn);
				keyComboControl(settings.continuousScreenshotToggle);
				keyComboControl(settings.toggleDisableGrayHurtboxes);
			}
			if (ImGui::CollapsingHeader("General Settings")) {
				bool modWindowVisibleOnStart = settings.modWindowVisibleOnStart;
				if (ImGui::Checkbox(settings.getOtherUIName(&settings.modWindowVisibleOnStart), &modWindowVisibleOnStart)) {
					settings.modWindowVisibleOnStart = modWindowVisibleOnStart;
					needWriteSettings = true;
				}
				ImGui::SameLine();
				HelpMarker(settings.getOtherUIDescription(&settings.modWindowVisibleOnStart));
				
				bool closingModWindowAlsoHidesFramebar = settings.closingModWindowAlsoHidesFramebar;
				if (ImGui::Checkbox(settings.getOtherUIName(&settings.closingModWindowAlsoHidesFramebar), &closingModWindowAlsoHidesFramebar)) {
					settings.closingModWindowAlsoHidesFramebar = closingModWindowAlsoHidesFramebar;
					needWriteSettings = true;
				}
				ImGui::SameLine();
				HelpMarker(settings.getOtherUIDescription(&settings.closingModWindowAlsoHidesFramebar));
				
				bool displayUIOnTopOfPauseMenu = settings.displayUIOnTopOfPauseMenu;
				if (ImGui::Checkbox(settings.getOtherUIName(&settings.displayUIOnTopOfPauseMenu), &displayUIOnTopOfPauseMenu)) {
					settings.displayUIOnTopOfPauseMenu = displayUIOnTopOfPauseMenu;
					needWriteSettings = true;
				}
				ImGui::SameLine();
				HelpMarker(settings.getOtherUIDescription(&settings.displayUIOnTopOfPauseMenu));
				
				int lowProfileCutoffPoint = settings.lowProfileCutoffPoint;
				ImGui::SetNextItemWidth(200.F);
				if (ImGui::InputInt(settings.getOtherUIName(&settings.lowProfileCutoffPoint), &lowProfileCutoffPoint, 1000, 10000, 0)) {
					settings.lowProfileCutoffPoint = lowProfileCutoffPoint;
					needWriteSettings = true;
				}
				imguiActiveTemp = imguiActiveTemp || ImGui::IsItemActive();
				ImGui::SameLine();
				HelpMarker(settings.getOtherUIDescription(&settings.lowProfileCutoffPoint));
				
			}
		}
		ImGui::End();
		if (showTensionData) {
			ImGui::Begin("Tension Data", &showTensionData);
			if (ImGui::BeginTable("##TensionData", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_NoPadOuterX)) {
				ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 220.f);
				ImGui::TableSetupColumn("P1", ImGuiTableColumnFlags_WidthStretch, 0.5f);
				ImGui::TableSetupColumn("P2", ImGuiTableColumnFlags_WidthStretch, 0.5f);
				
				ImGui::TableNextColumn();
				ImGui::TextUnformatted("Name");
				ImGui::TableNextColumn();
				drawPlayerIconWithTooltip(0);
				ImGui::SameLine();
				ImGui::TextUnformatted("P1");
				ImGui::TableNextColumn();
				drawPlayerIconWithTooltip(1);
				ImGui::SameLine();
				ImGui::TextUnformatted("P2");
				
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
					printDecimal(player.negativePenaltyTimer * 100 / 60, 2, 0);
					sprintf_s(strbuf, "%s sec", printdecimalbuf);
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
					if (endScene.players[i].inHitstun) {
						comboHappening = true;
						break;
					}
				}
				
				ImGui::TableNextColumn();
				ImGui::TextUnformatted("Tension Gain On Attack");
				AddTooltip("Affects how fast you gain Tension when performing attacks or combos.\n"
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
						if (!player.inHitstun) {
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
				ImGui::TextUnformatted("Tension Gain On Defense");
				AddTooltip("Affects how fast you gain Tension when getting hit by attacks or combos.\n"
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
						if (player.inHitstun) {
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
		if (showSpeedsData) {
			ImGui::Begin("Speed/Proration", &showSpeedsData);
			
			if (ImGui::BeginTable("##TensionData", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_NoPadOuterX)) {
				ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 140.f);
				ImGui::TableSetupColumn("P1", ImGuiTableColumnFlags_WidthStretch, 0.5f);
				ImGui::TableSetupColumn("P2", ImGuiTableColumnFlags_WidthStretch, 0.5f);
				
				ImGui::TableNextColumn();
				ImGui::TextUnformatted("Name");
				ImGui::TableNextColumn();
				drawPlayerIconWithTooltip(0);
				ImGui::SameLine();
				ImGui::TextUnformatted("P1");
				ImGui::TableNextColumn();
				drawPlayerIconWithTooltip(1);
				ImGui::SameLine();
				ImGui::TextUnformatted("P2");
				
				ImGui::TableNextColumn();
				ImGui::TextUnformatted("X; Y");
				AddTooltip("Position X; Y in the arena. Divided by 100 for viewability.");
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					ImGui::TableNextColumn();
					printDecimal(player.x, 2, 0);
					sprintf_s(strbuf, "%s; ", printdecimalbuf);
					printDecimal(player.y, 2, 0);
					sprintf_s(strbuf + strlen(strbuf), sizeof strbuf - strlen(strbuf), "%s", printdecimalbuf);
					ImGui::TextUnformatted(strbuf);
				}
				
				ImGui::TableNextColumn();
				ImGui::TextUnformatted("Speed X; Y");
				AddTooltip("Speed X; Y in the arena. Divided by 100 for viewability.");
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					ImGui::TableNextColumn();
					printDecimal(player.speedX, 2, 0);
					sprintf_s(strbuf, "%s; ", printdecimalbuf);
					printDecimal(player.speedY, 2, 0);
					sprintf_s(strbuf + strlen(strbuf), sizeof strbuf - strlen(strbuf), "%s", printdecimalbuf);
					ImGui::TextUnformatted(strbuf);
				}
				
				ImGui::TableNextColumn();
				ImGui::TextUnformatted("Gravity");
				AddTooltip("Gravity. Divided by 100 for viewability.");
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					ImGui::TableNextColumn();
					printDecimal(player.gravity, 2, 0);
					ImGui::TextUnformatted(printdecimalbuf);
				}
				
				ImGui::TableNextColumn();
				ImGui::TextUnformatted("Combo Timer");
				AddTooltip("The time, in seconds, of the current combo's duration.");
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					ImGui::TableNextColumn();
					printDecimal(player.comboTimer * 100 / 60, 2, 0);
					sprintf_s(strbuf, "%s sec (%df)", printdecimalbuf, player.comboTimer);
					ImGui::TextUnformatted(strbuf);
				}
				
				ImGui::TableNextColumn();
				ImGui::TextUnformatted("Pushback");
				AddTooltip("Pushback. Divided by 100 for viewability.\n"
					"Format: Current pushback / (Max pushback + FD pushback) (FD pushback base value, FD pushback percentage modifier%).\n"
					"If last hit was not FD'd, FD values are not displayed.\n"
					"When you attack an opponent, the opponent has this 'Pushback' value and you only get pushed back if they're in the corner (or close to it)."
					" If the opponent utilized FD, you will be pushed back regardless of whether the opponent is in the corner."
					" If they're in the corner and they also FD, the pushback from FD and the pushback from them being in the corner get added together."
					" The FD pushback base value and modifier % will be listed in (). The base value depends on distance between players. If it's less"
					" than 385000, the base is 900, otherwise it's 500. The modifier is 130% if the defender was touching the wall, otherwise 100%."
					" Multiply the base value by the modifier and 175 / 10, round down to get resulting pushback, divide by 100 for viewability.");
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					ImGui::TableNextColumn();
					printDecimal(player.pushback, 2, 0);
					sprintf_s(strbuf, !player.baseFdPushback ? "%s / " : "%s / (", printdecimalbuf);
					printDecimal(player.pushbackMax * 175 / 10, 2, 0);
					sprintf_s(strbuf + strlen(strbuf), sizeof strbuf - strlen(strbuf), "%s", printdecimalbuf);
					if (player.baseFdPushback) {
						printDecimal(player.fdPushbackMax * 175 / 10, 2, 0);
						sprintf_s(strbuf + strlen(strbuf), sizeof strbuf - strlen(strbuf), " + %s)", printdecimalbuf);
						sprintf_s(strbuf + strlen(strbuf), sizeof strbuf - strlen(strbuf), " (%d, %d%c)",
							player.baseFdPushback, player.oppoWasTouchingWallOnFD ? 130 : 100, '%');
					}
					ImGui::TextWrapped("%s", strbuf);
				}
				
				ImGui::TableNextColumn();
				ImGui::TextUnformatted("Base pushback");
				AddTooltip("Base pushback on hit/block. Pushback = floor(Base pushback * 175 / 10) (divide by 100 for viewability).\n"
					"The base values of pushback are, per attack level:\n"
					"Ground block: 1250, 1375, 1500, 1750, 2000, 2400, 3000;\n"
					"Air block: 800,  850,  900,  950, 1000, 2400, 3000;\n"
					"Hit: 1300, 1400, 1500, 1750, 2000, 2400, 3000;\n");
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					ImGui::TableNextColumn();
					sprintf_s(strbuf, "%d", player.basePushback);
					ImGui::TextUnformatted(strbuf);
				}
				
				ImGui::TableNextColumn();
				ImGui::TextUnformatted("Pushback Modifier");
				AddTooltip("Pushback modifier. Equals Attack pushback modifier % * Attack pushback modifier on hit % * Combo timer modifier %"
					" * IB modifier %.\n"
					"Attack pushback modifier depends on the performed move."
					" Attack pushback modifier on hit depends on the performed move and should only be non-100 when the opponent is in hitstun."
					" Combo timer modifier depends on combo timer, in frames: >= 480 --> 200%, >= 420 --> 184%, >= 360 --> 166%, >= 300 --> 150%"
					", >= 240 --> 136%, >= 180 --> 124%, >= 120 --> 114%, >= 60 --> 106%."
					" IB modifier is non-100 on IB: air IB 90%, ground IB 10%.");
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					ImGui::TableNextColumn();
					sprintf_s(strbuf, "%d%c * %d%c * %d%c * %d%c = %d%c", player.attackPushbackModifier, '%', player.hitstunPushbackModifier, '%',
						player.comboTimerPushbackModifier, '%', player.ibPushbackModifier, '%',
						player.attackPushbackModifier * player.hitstunPushbackModifier / 100
						* player.comboTimerPushbackModifier / 100
						* player.ibPushbackModifier / 100, '%');
					ImGui::TextWrapped("%s", strbuf);
				}
				
				ImGui::TableNextColumn();
				ImGui::TextUnformatted("Received Speed Y");
				AddTooltip("This is updated only when a hit happens.\n"
					"Format: Base speed Y * (Weight % * Combo Proration % = Resulting Modifier %) = Resulting speed Y. Base and Final speeds divided by 100 for viewability."
					" On block, something more is going on, and it's not studied yet, so gained speed Y cannot be displayed."
					" Modifiers on received speed Y are weight and combo proration, displayed in that order.\n"
					"The combo proration depends on the number of hits so far at the moment of getting hit, not including the ongoing hit:\n"
					"6 hits so far -> 59/60 * 100% proration,\n"
					"5 hits -> 58 / 60 * 100% and so on, up to 30 / 60 * 100%. The rounding of the final speed Y is up.\n"
					"Some moves could theoretically ignore weight or combo proration, or both. When that happens, 100% will be displayed in place"
					" of the ignored parameter.");
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					ImGui::TableNextColumn();
					if (!player.receivedSpeedYValid) {
						ImGui::TextUnformatted("???");
					} else {
						int mod = player.receivedSpeedYWeight * player.receivedSpeedYComboProration / 100;
						printDecimal(player.receivedSpeedY * 100 / mod, 2, 0);
						sprintf_s(strbuf, "%s", printdecimalbuf);
						printDecimal(player.receivedSpeedY, 2, 0);
						sprintf_s(strbuf + strlen(strbuf), sizeof strbuf - strlen(strbuf), " * (%d%c * %d%c = %d%c) = %s",
							player.receivedSpeedYWeight,
							'%', player.receivedSpeedYComboProration, '%',
							mod, '%',
							printdecimalbuf);
						ImGui::TextWrapped("%s", strbuf);
					}
				}
				
				ImGui::TableNextColumn();
				ImGui::TextUnformatted("Hitstun Proration");
				AddTooltip("This is updated only when a hit happens."
					" Hitstun proration depends on the duration of an air or ground combo, in frames. For air combo:\n"
					">= 1080 --> 10%\n"
					">= 840  --> 60%\n"
					">= 600  --> 70%\n"
					">= 420  --> 80%\n"
					">= 300  --> 90%\n"
					">= 180  --> 95%\n"
					"For ground combo it's just:\n"
					">= 1080 --> 50%\n"
					"Some attacks could theoretically ignore hitstun proration. When that happens, 100% is displayed.");
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					ImGui::TableNextColumn();
					if (!player.hitstunProrationValid) {
						ImGui::TextUnformatted("--");
					} else {
						sprintf_s(strbuf, "%d%c", player.hitstunProration, '%');
						ImGui::TextUnformatted(strbuf);
					}
				}
				
				ImGui::TableNextColumn();
				ImGui::TextUnformatted("Wakeup");
				AddTooltip("Displays wakeup timing or time until able to act after air recovery (airtech)."
					" Format: Time remaining until able to act / Total wakeup or airtech time.");
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					ImGui::TableNextColumn();
					
					int remaining = 0;
					if (player.wakeupTiming) {
						remaining = player.wakeupTiming - player.animFrame + 1;
					}
					sprintf_s(strbuf, "%d/%d", remaining, player.wakeupTiming);
					ImGui::TextUnformatted(strbuf);
				}
				
				ImGui::EndTable();
			}
			ImGui::End();
		}
		for (int i = 0; i < 2; ++i) {
			if (showCharSpecific[i]) {
				sprintf_s(strbuf, "Character Specific (P%d)", i + 1);
				ImGui::Begin(strbuf, showCharSpecific + i);
				if (!*aswEngine) {
					ImGui::TextUnformatted("Match not running");
					ImGui::End();
					continue;
				}
				const PlayerInfo& player = endScene.players[i];
				if (player.charType == CHARACTER_TYPE_SOL) {
					if (player.playerval0) {
						GGIcon scaledIcon = scaleGGIconToHeight(DISolIcon, 14.F);
						drawGGIcon(scaledIcon);
						ImGui::SameLine();
						ImGui::TextUnformatted("In Dragon Install");
						ImGui::Text("Time remaining: %d/%df", player.playerval1, player.maxDI);
					} else {
						GGIcon scaledIcon = scaleGGIconToHeight(getCharIcon(CHARACTER_TYPE_SOL), 14.F);
						drawGGIcon(scaledIcon);
						ImGui::SameLine();
						ImGui::TextUnformatted("Not in Dragon Install");
					}
				} else if (player.charType == CHARACTER_TYPE_KY) {
					GGIcon scaledIcon = scaleGGIconToHeight(getPlayerCharIcon(i), 14.F);
					drawGGIcon(scaledIcon);
					ImGui::SameLine();
					if (!endScene.interRoundValueStorage2Offset) {
						ImGui::TextUnformatted("Error");
					} else {
						DWORD& theValue = *(DWORD*)(*aswEngine + endScene.interRoundValueStorage2Offset + i * 4);
						if (theValue) {
							ImGui::TextUnformatted("Hair down");
						} else {
							ImGui::TextUnformatted("Hair not down");
						}
						if (game.isTrainingMode() && ImGui::Button("Toggle Hair Down")) {
							theValue = 1 - theValue;
							endScene.BBScr_callSubroutine((void*)player.pawn.ent, "PonyMeshSetCheck");
						}
					}
				} else if (player.charType == CHARACTER_TYPE_ZATO) {
					Entity eddie = nullptr;
					bool isSummoned = player.pawn.playerVal(0);
					if (isSummoned) {
						eddie = endScene.getReferredEntity((void*)player.pawn.ent, 4);
					}
					
					GGIcon scaledIcon = scaleGGIconToHeight(getPlayerCharIcon(i), 14.F);
					drawGGIcon(scaledIcon);
					ImGui::SameLine();
					ImGui::Text("Eddie Values");
					sprintf_s(strbuf, "##Zato_P%d", i);
					if (ImGui::BeginTable(strbuf, 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_NoPadOuterX)) {
						ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0.5f);
						ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.5f);
						
						ImGui::TableHeadersRow();
						
						ImGui::TableNextColumn();
						ImGui::Text("Eddie Gauge");
						AddTooltip("Divided by 10 for readability.");
						ImGui::TableNextColumn();
						ImGui::Text("%-3d/%d", player.pawn.exGaugeValue(0) / 10, player.pawn.exGaugeMaxValue(0) / 10);
						
						ImGui::TableNextColumn();
						ImGui::TextUnformatted("Is Summoned");
						ImGui::TableNextColumn();
						if (!isSummoned) {
							ImGui::TextUnformatted("no");
						} else {
							ImGui::TextUnformatted("yes");
						}
						
						ImGui::TableNextColumn();
						ImGui::TextUnformatted("Shadow Puddle X");
						ImGui::TableNextColumn();
						int shadowPuddleX = player.pawn.playerVal(3);
						if (shadowPuddleX != 3000000) {
							printDecimal(shadowPuddleX, 2, 0);
							ImGui::TextUnformatted(printdecimalbuf);
						}
						
						if (!eddie && player.eddie.landminePtr) {
							eddie = Entity{player.eddie.landminePtr};
						}
						
						ImGui::TableNextColumn();
						ImGui::TextUnformatted("Startup");
						ImGui::TableNextColumn();
						ImGui::Text("%d", player.eddie.startup);
						
						ImGui::TableNextColumn();
						ImGui::TextUnformatted("Active");
						ImGui::TableNextColumn();
						player.eddie.actives.print(strbuf, sizeof strbuf);
						ImGui::TextUnformatted(strbuf);
						
						ImGui::TableNextColumn();
						ImGui::TextUnformatted("Recovery");
						ImGui::TableNextColumn();
						ImGui::Text("%d", player.eddie.recovery);
						
						ImGui::TableNextColumn();
						ImGui::TextUnformatted("Total");
						ImGui::TableNextColumn();
						ImGui::Text("%d", player.eddie.total);
						
						ImGui::TableNextColumn();
						ImGui::TextUnformatted("Frame Adv.");
						ImGui::TableNextColumn();
						frameAdvantageControl(
							player.eddie.frameAdvantage,
							player.eddie.landingFrameAdvantage,
							player.eddie.frameAdvantageValid,
							player.eddie.landingFrameAdvantageValid,
							false);
						
						ImGui::TableNextColumn();
						ImGui::TextUnformatted("Hitstop");
						ImGui::TableNextColumn();
						ImGui::Text("%d/%d", player.eddie.hitstop, player.eddie.hitstopMax);
						
						ImGui::TableNextColumn();
						ImGui::TextUnformatted("Last Consumed Eddie Gauge Amount");
						AddTooltip("Divided by 10 for readability. The amount of consumed Eddie Gauge of the last attack.");
						ImGui::TableNextColumn();
						ImGui::Text("%d", player.eddie.consumedResource / 10);
						
						ImGui::TableNextColumn();
						ImGui::TextUnformatted("X");
						ImGui::TableNextColumn();
						if (eddie) {
							printDecimal(eddie.x(), 2, 0);
							ImGui::TextUnformatted(printdecimalbuf);
						}
						
						ImGui::TableNextColumn();
						ImGui::TextUnformatted("Anim");
						ImGui::TableNextColumn();
						if (eddie) {
							ImGui::TextUnformatted(eddie.animationName());
						}
						
						ImGui::TableNextColumn();
						ImGui::TextUnformatted("Frame");
						ImGui::TableNextColumn();
						if (eddie) {
							ImGui::Text("%d", eddie.currentAnimDuration());
						}
						
						ImGui::TableNextColumn();
						ImGui::TextUnformatted("Sprite");
						ImGui::TableNextColumn();
						if (eddie) {
							ImGui::Text("%s (%d/%d)", eddie.spriteName(), eddie.spriteFrameCounter(), eddie.spriteFrameCounterMax());
						}
						
						ImGui::EndTable();
					}
				} else {
					GGIcon scaledIcon = scaleGGIconToHeight(getPlayerCharIcon(i), 14.F);
					drawGGIcon(scaledIcon);
					ImGui::SameLine();
					ImGui::TextUnformatted("No character specific information to show.");
				}
				ImGui::End();
			}
		}
		if (showFrameAdvTooltip) {
			ImGui::SetNextWindowSize({ 500.F, 0.F }, ImGuiCond_FirstUseEver);
			ImGui::Begin("Frame Advantage Help", &showFrameAdvTooltip);
			ImGui::TextWrapped("%s",
				"Frame advantage of this player over the other player, in frames, after doing the last move. Frame advantage is who became able to 5P/j.P earlier"
				" (or, for stance moves such as Leo backturn, it also includes the ability to do a stance move from such stance)."
				" Please note that players may become able to block earlier than they become able to attack. This difference will not be displayed, and only the time"
				" when players become able to attack will be considered for frame advantage calculation.\n\n"
				"The value in () means frame advantage after yours or your opponent's landing, whatever happened last."
				" The landing frame advantage is measured against the other player's becoming able to attack after landing or,"
				" if they never jumped, after them just becoming able to attack."
				" For example, two players jump one after each other with 1f delay. Both players whiff a j.P in the air and recover in the air, then land"
				" one after another with 1f delay. The player who landed first will have a +1 'landing' frame advantage and the displayed result will be:"
				" ??? (+1), where ??? is the 'air' frame advantage (read below), and +1 is the 'landing' frame advantage.\n"
				"The other value (not in ()) means ('air') frame advantage"
				" immediately after recovering in the air or on the ground, whichever happened earlier."
				" 'Air' frame advantage is measured against the other player becoming able to attack in the air or"
				" on the ground, whichever happened earlier. For example, you do a move in the air and recover on frame 1 in the air. On the next frame, opponent recovers"
				" as well, but it takes you 100 frames to fall back down. Then you're +1 advantage in the air, but upon landing you're -99, so the displayed result is:"
				" +1 (-99). If both you and the opponent jumped, and you recovered in the air 1f before they did, but after they recovered"
				" it took you 100 frames to fall back down"
				" and them - only one, then the displayed result for you will be: +1 (-99), and for them: -1 (+99). So, 'air' frame advantage is measured"
				" against recovering in the air/on the ground, 'landing' frame advantage is measured against recovering on the ground only.\n"
				"\n"
				"Frame advantage is only updated when both players are in \"not idle\" state simultaneously or one started blocking, or if a player lands from a jump.\n"
				"Frame advantage may go into the past and use time from before the opponent entered blockstun and add that time to your frame advantage,"
				" if during that time you were idle. For example, if Ky uses j.D, he recovers in the air before it goes active, then opponent gets put into blockstun,"
				" then all the time that Ky was idle in the air immediately gets included in the frame advantage. For example, if you did a move that let you recover"
				" in one frame, but it caused the opponent to enter blockstun 100 frames after you started your move, and the opponent spent 1 frame in blockstun,"
				" you're considered +100 instead of +1. If you do not want to include attacker's pre-blockstun idle time as part of frame advantage,"
				" then you may calculate frame advantage manually by adding defender's hitstop + blockstun together. They're displayed above in the 'Hitstop+X-stun' field.");
			ImGui::End();
		}
		
		if (!shaderCompilationError) {
			graphics.getShaderCompilationError(&shaderCompilationError);
		}
		if (shaderCompilationError && showShaderCompilationError) {
			ImGui::SetNextWindowSize({ 500.F, 0.F }, ImGuiCond_FirstUseEver);
			ImGui::Begin("Shader compilation error", &showShaderCompilationError);
			ImGui::TextWrapped("%s", shaderCompilationError->c_str());
			ImGui::End();
		}
	}
	
	if (needShowFramebar()) {
		drawFramebars();
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

// Runs on the graphics thread
void UI::onEndScene(IDirect3DDevice9* device, void* drawData, IDirect3DTexture9* iconTexture) {
	if (!visible && !needShowFramebar() || !imguiInitialized || gifMode.modDisabled || !drawData) {
		return;
	}
	std::unique_lock<std::mutex> uiGuard(lock);
	initializeD3D(device);
	
	substituteTextureIDs(device, drawData, iconTexture);
	ImGui_ImplDX9_RenderDrawData((ImDrawData*)drawData);
}

// Runs on the main thread
// Must be performed while holding the -lock- mutex.
void UI::initialize() {
	if (imguiInitialized || !visible && !needShowFramebar() || !keyboard.thisProcessWindow || gifMode.modDisabled) return;
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(keyboard.thisProcessWindow);
	
	ImGuiIO& io = ImGui::GetIO();
	BYTE* unused;
	io.Fonts->GetTexDataAsRGBA32(&unused, nullptr, nullptr);  // imGui complains if we don't call this before preparing its draw data
	io.Fonts->SetTexID((ImTextureID)TEXID_IMGUIFONT);  // I use fake wishy-washy IDs instead of real textures, because they're created on the
												 // the graphics thread and this code is running on the main thread.
												 // I cannot create our textures on the main thread, because I inject our DLL in the middle
												 // of the app already running and I don't know if Direct3D 9 is fully thread-safe during resource creation.
												 // Even if it was, device may be lost when calling IDirect3DDevice9->Present(), which is called on the graphics
												 // thread, but the IDirect3DDevice9->Reset() is called on the main thread, which creates a delay between losing
												 // a device and resetting it, during which I don't want to create new resources.
												 // For these reasons, I create all Direct3D resources only on the graphics thread
												 // and use imaginary numbers for tex IDs in imGui.
												 // I also made imGui a submodule instead of copying all its files directly, so I can't modify it now,
												 // i can't submit a Pull Request, because using IDirect3DTexture9* as tex IDs lies at the core philosophy
												 // of not only the current imGui D3D9 implementation, but the whole imGui, so it's unchangeable.
												 // I made imGui a submodule because it's easier to keep track of changes to it and update it, and also
												 // give it credit by making it visible in some git tools that it's a link to their repo.
												 // For now, we keep the current imGui D3D9 implementation which stores pointers in tex IDs.
												 // So we must swap out the pointers every time imGui D3D9 implementation interacts with them.
	
	imguiInitialized = true;
}

// Runs on the main thread while the graphics thread is suspended
void UI::handleResetBefore() {
	if (imguiD3DInitialized) {
		ImGui_ImplDX9_InvalidateDeviceObjects();
	}
}

// Runs on the main thread while the graphics thread is suspended
void UI::handleResetAfter() {
	if (imguiD3DInitialized) {
		ImGui_ImplDX9_CreateDeviceObjects();
		onImGuiMessWithFontTexID();
	}
}

// Runs on the main thread
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

// Runs on the main thread. Called from EndScene::WndProcHook
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

// Runs on the main thread
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

// Runs on the main thread. Called hundreds of times each frame
SHORT WINAPI UI::hook_GetKeyState(int nVirtKey) {
	//HookGuard hookGuard("GetKeyState");
	SHORT result;
	if (ui.imguiActive) {
		result = 0;
	} else {
		result = GetKeyState(nVirtKey);
	}
	return result;
}

// Runs on the main thread
void UI::decrementFlagTimer(int& timer, bool& flag) {
	if (timer > 0 && flag) {
		--timer;
		if (timer == 0) flag = false;
	}
}

// Runs on the main thread
void UI::frameAdvantageControl(int frameAdvantage, int landingFrameAdvantage, bool frameAdvantageValid, bool landingFrameAdvantageValid, bool rightAlign) {
	if (frameAdvantageValid && landingFrameAdvantageValid && frameAdvantage != landingFrameAdvantage) {
		frameAdvantageTextFormat(frameAdvantage, strbuf, sizeof strbuf);
		strcat(strbuf, " (");
		frameAdvantageTextFormat(landingFrameAdvantage, strbuf + strlen(strbuf), sizeof strbuf - strlen(strbuf));
		strcat(strbuf, ")");
		if (rightAlign) RightAlign(ImGui::CalcTextSize(strbuf).x);
		pushZeroItemSpacingStyle();
		frameAdvantageText(frameAdvantage);
		ImGui::SameLine();
		ImGui::TextUnformatted(" (");
		ImGui::SameLine();
		frameAdvantageText(landingFrameAdvantage);
		ImGui::SameLine();
		ImGui::TextUnformatted(")");
		ImGui::PopStyleVar();
	} else if (frameAdvantageValid || landingFrameAdvantageValid) {
		int frameAdvantageLocal = frameAdvantageValid ? frameAdvantage : landingFrameAdvantage;
		frameAdvantageTextFormat(frameAdvantageLocal, strbuf, sizeof strbuf);
		if (rightAlign) RightAlign(ImGui::CalcTextSize(strbuf).x);
		frameAdvantageText(frameAdvantageLocal);
	}
}

// Runs on the main thread
void UI::frameAdvantageTextFormat(int frameAdv, char* buf, size_t bufSize) {
	if (frameAdv > 0) {
		sprintf_s(buf, bufSize, "+%d", frameAdv);
	} else {
		sprintf_s(buf, bufSize, "%d", frameAdv);
	}
}

// Runs on the main thread
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

// Runs on the main thread
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

// Avoid copying the vector - only move or pass by pointer/reference
// Runs on the main thread
void UI::copyDrawDataTo(std::vector<BYTE>& destinationBuffer) {
	destinationBuffer.clear();
	if (!drawData) return;
	
	ImDrawData* oldData = (ImDrawData*)drawData;
	if (!oldData->Valid) return;
	
	size_t requiredSize = sizeof ImDrawData
		+ sizeof (CustomImDrawList*) * oldData->CmdListsCount
		+ sizeof CustomImDrawList * oldData->CmdListsCount;
	
	for (int i = 0; i < oldData->CmdListsCount; ++i) {
		const ImDrawList* cmdList = oldData->CmdLists[i];
		requiredSize += cmdList->CmdBuffer.Size * sizeof ImDrawCmd
			+ cmdList->IdxBuffer.Size * sizeof ImDrawIdx
			+ cmdList->VtxBuffer.Size * sizeof ImDrawVert
		;
	}
	
	destinationBuffer.resize(requiredSize);
	BYTE* p = destinationBuffer.data();
	
	memcpy(p, oldData, sizeof ImDrawData);
	ImDrawData* newData = (ImDrawData*)p;
	p += sizeof ImDrawData;
	
	newData->CmdLists.Data = (ImDrawList**)p;
	
	p += sizeof (CustomImDrawList*) * oldData->CmdListsCount;
	
	for (int i = 0; i < oldData->CmdListsCount; ++i) {
		newData->CmdLists.Data[i] = (ImDrawList*)p;
		
		CustomImDrawList* newCmdList = (CustomImDrawList*)p;
		const ImDrawList* oldCmdList = oldData->CmdLists[i];
		
		newCmdList->CmdBuffer.Size = oldCmdList->CmdBuffer.Size;
		newCmdList->IdxBuffer.Size = oldCmdList->IdxBuffer.Size;
		newCmdList->VtxBuffer.Size = oldCmdList->VtxBuffer.Size;
		p += sizeof CustomImDrawList;
		
		newCmdList->CmdBuffer.Data = (ImDrawCmd*)p;
		size_t cmdsSize = oldCmdList->CmdBuffer.Size * sizeof ImDrawCmd;
		memcpy(p, oldCmdList->CmdBuffer.Data, cmdsSize);
		p += cmdsSize;
		
		newCmdList->IdxBuffer.Data = (ImDrawIdx*)p;
		size_t idxSize = oldCmdList->IdxBuffer.Size * sizeof ImDrawIdx;
		memcpy(p, oldCmdList->IdxBuffer.Data, idxSize);
		p += idxSize;
		
		newCmdList->VtxBuffer.Data = (ImDrawVert*)p;
		size_t vtxSize = oldCmdList->VtxBuffer.Size * sizeof ImDrawVert;
		memcpy(p, oldCmdList->VtxBuffer.Data, vtxSize);
		p += vtxSize;
	}
	
}

// Runs on the graphics thread
void UI::substituteTextureIDs(IDirect3DDevice9* device, void* drawData, IDirect3DTexture9* iconTexture) {
	ImDrawData* d = (ImDrawData*)drawData;
	for (int i = 0; i < d->CmdListsCount; ++i) {
		ImDrawList* drawList = d->CmdLists[i];
		for (int j = 0; j < drawList->CmdBuffer.Size; ++j) {
			ImDrawCmd& cmd = drawList->CmdBuffer[j];
			if (cmd.TextureId == (ImTextureID)TEXID_IMGUIFONT) {
				cmd.TextureId = imguiFont;
			} else if (cmd.TextureId == (ImTextureID)TEXID_GGICON) {
				cmd.TextureId = iconTexture;
			} else if (cmd.TextureId == (ImTextureID)TEXID_FRAMES) {
				cmd.TextureId = graphics.getFramesTexture(device);
			}
		}
	}
}

// Must be performed while holding the -lock- mutex
// Runs on the graphics thread
void UI::initializeD3D(IDirect3DDevice9* device) {
	if (!imguiD3DInitialized) {
		imguiD3DInitialized = true;
		ImGui_ImplDX9_Init(device);
		ImGui_ImplDX9_NewFrame();
		onImGuiMessWithFontTexID();
	}
}

// Runs on the graphics thread
void UI::onImGuiMessWithFontTexID() {
	ImGuiIO& io = ImGui::GetIO();
	IDirect3DTexture9* tmp = (IDirect3DTexture9*)io.Fonts->TexID;  // has SetTexID() but no GetTexID()... says it will pass it back to me if I draw something. No thx
	if (tmp != (IDirect3DTexture9*)TEXID_IMGUIFONT) {
		imguiFont = tmp;
		io.Fonts->SetTexID((ImTextureID)TEXID_IMGUIFONT);  // undo the TexID replacement ImGui_ImplDX9_NewFrame has done. You can read more about this stupid gig we're doing in UI::initialize()
	}
}

const char* characterNames[25] {
	"Sol",       // 0
	"Ky",        // 1
	"May",       // 2
	"Millia",    // 3
	"Zato=1",    // 4
	"Potemkin",  // 5
	"Chipp",     // 6
	"Faust",     // 7
	"Axl",       // 8
	"Venom",     // 9
	"Slayer",    // 10
	"I-No",      // 11
	"Bedman",    // 12
	"Ramlethal", // 13
	"Sin",       // 14
	"Elphelt",   // 15
	"Leo",       // 16
	"Johnny",    // 17
	"Jack O'",   // 18
	"Jam",       // 19
	"Kum",       // 20
	"Raven",     // 21
	"Dizzy",     // 22
	"Baiken",    // 23
	"Answer"     // 24
};

const char* characterNamesFull[25] {
	"Sol Badguy",	  // 0
	"Ky Kiske",		  // 1
	"May",			  // 2
	"Millia Rage",	  // 3
	"Zato=1",		  // 4
	"Potemkin",		  // 5
	"Chipp Zanuff",	  // 6
	"Faust",		  // 7
	"Axl Low",		  // 8
	"Venom",		  // 9
	"Slayer",		  // 10
	"I-No",           // 11
	"Bedman",         // 12
	"Ramlethal",      // 13
	"Sin Kiske",      // 14
	"Elphelt",        // 15
	"Leo Whitefang",  // 16
	"Johnny",         // 17
	"Jack O'",        // 18
	"Jam Kuradoberi", // 19
	"Kum Haehyun",    // 20
	"Raven",          // 21
	"Dizzy",          // 22
	"Baiken",         // 23
	"Answer"          // 24
};

GGIcon coordsToGGIcon(int x, int y, int w, int h) {
	GGIcon result;
	result.size = ImVec2{ (float)w, (float)h };
	result.uvStart = ImVec2{ (float)x / 1536.F, (float)y / 1536.F };
	result.uvEnd = ImVec2{ (float)(x + w) / 1536.F, (float)(y + h) / 1536.F };
	return result;
}

void drawGGIcon(const GGIcon& icon) {
	ImGui::Image((ImTextureID)TEXID_GGICON, icon.size, icon.uvStart, icon.uvEnd);
}

GGIcon scaleGGIconToHeight(const GGIcon& icon, float height) {
	GGIcon result;
	result.size = ImVec2{ icon.size.x * height / icon.size.y, height };
	result.uvStart = icon.uvStart;
	result.uvEnd = icon.uvEnd;
	return result;
}

CharacterType getPlayerCharacter(int playerSide) {
	if (!*aswEngine || playerSide != 0 && playerSide != 1) return (CharacterType)-1;
	entityList.populate();
	Entity ent = entityList.slots[playerSide];
	if (!ent) return (CharacterType)-1;
	return ent.characterType();
}

void drawPlayerIconWithTooltip(int playerSide) {
	CharacterType charType = getPlayerCharacter(playerSide);
	GGIcon scaledIcon = scaleGGIconToHeight(getCharIcon(charType), 14.F);
	drawGGIcon(scaledIcon);
	if (charType != -1) {
		AddTooltip(characterNamesFull[charType]);
	}
}

bool endsWithCaseInsensitive(std::wstring str, const wchar_t* endingPart) {
	unsigned int length = 0;
	const wchar_t* ptr = endingPart;
	while (*ptr != L'\0') {
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

int findCharRev(const char* buf, char c) {
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

int findCharRevW(const wchar_t* buf, wchar_t c) {
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

void AddTooltip(const char* desc) {
	if (ImGui::BeginItemTooltip()) {
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

void HelpMarker(const char* desc) {
	ImGui::TextDisabled("(?)");
	AddTooltip(desc);
}

void RightAlign(float w) {
	const auto rightEdge = ImGui::GetCursorPosX() + ImGui::GetColumnWidth();
	const auto posX = (rightEdge - w);
	ImGui::SetCursorPosX(posX);
}

void RightAlignedText(const char* txt) {
	RightAlign(ImGui::CalcTextSize(txt).x);
	ImGui::TextUnformatted(txt);
}

void RightAlignedColoredText(const ImVec4& color, const char* txt) {
	RightAlign(ImGui::CalcTextSize(txt).x);
	ImGui::TextColored(color, txt);
}

void CenterAlign(float w) {
	const auto rightEdge = ImGui::GetCursorPosX() + ImGui::GetColumnWidth() / 2;
	const auto posX = (rightEdge - w / 2);
	ImGui::SetCursorPosX(posX);
}

void CenterAlignedText(const char* txt) {
	CenterAlign(ImGui::CalcTextSize(txt).x);
	ImGui::TextUnformatted(txt);
}

const GGIcon& getCharIcon(CharacterType charType) {
	if (charType >= 0 && charType < 25) {
		return characterIconsBorderless[charType];
	}
	return questionMarkIcon;
}

const GGIcon& getPlayerCharIcon(int playerSide) {
	return getCharIcon(getPlayerCharacter(playerSide));
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

const char* formatBoolean(bool value) {
	static const char* trueStr = "true";
	static const char* falseStr = "false";
	return value ? trueStr : falseStr;
}

void pushZeroItemSpacingStyle() {
	ImGuiStyle& style = ImGui::GetStyle();  // it's a reference
	ImVec2 itemSpacing = style.ItemSpacing;
	itemSpacing.x = 0;
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, itemSpacing);
}

float getItemSpacing() {
	return ImGui::GetStyle().ItemSpacing.x;
}

bool SelectionRect(ImVec2* start_pos, ImVec2* end_pos, ImGuiMouseButton mouse_button, bool* isDragging)
{
	/*IM_ASSERT(start_pos != NULL);
	IM_ASSERT(end_pos != NULL);
	if (ImGui::IsMouseClicked(mouse_button))
		*start_pos = ImGui::GetMousePos();
	if (ImGui::IsMouseDown(mouse_button))
	{
		*isDragging = true;
		*end_pos = ImGui::GetMousePos();
		ImDrawList* draw_list = ImGui::GetForegroundDrawList(); //ImGui::GetWindowDrawList();
		draw_list->AddRect(*start_pos, *end_pos, ImGui::GetColorU32(IM_COL32(0, 130, 216, 255)));   // Border
		draw_list->AddRectFilled(*start_pos, *end_pos, ImGui::GetColorU32(IM_COL32(0, 130, 216, 50)));	// Background
	}
	return ImGui::IsMouseReleased(mouse_button);*/
	return false;
}

bool UI::addImage(HMODULE hModule, WORD resourceId, PngResource& resource) {
	if (!loadPngResource(hModule, resourceId, resource)) return false;
	texturePacker.addImage(resource);
	return true;
}

void outlinedText(ImVec2 pos, const char* text) {
	ImGui::SetCursorPos({ pos.x, pos.y - 1.F });
	ImGui::TextColored(BLACK_COLOR, "%s", text);
	
	ImGui::SetCursorPos({ pos.x, pos.y + 1.F });
	ImGui::TextColored(BLACK_COLOR, "%s", text);
	
	ImGui::SetCursorPos({ pos.x - 1.F, pos.y - 1.F });
	ImGui::TextColored(BLACK_COLOR, "%s", text);
	
	ImGui::SetCursorPos({ pos.x + 1.F, pos.y - 1.F });
	ImGui::TextColored(BLACK_COLOR, "%s", text);
	
	ImGui::SetCursorPos({ pos.x - 1.F, pos.y + 1.F });
	ImGui::TextColored(BLACK_COLOR, "%s", text);
	
	ImGui::SetCursorPos({ pos.x + 1.F, pos.y + 1.F });
	ImGui::TextColored(BLACK_COLOR, "%s", text);
	
	ImGui::SetCursorPos({ pos.x, pos.y });
	ImGui::TextUnformatted(text);
}

const PngResource& UI::getPackedFramesTexture() const {
	return packedTexture;
}

int numDigits(int num) {
	int answer = 1;
	num /= 10;
	while (num) {
		++answer;
		num /= 10;
	}
	return answer;
}

void UI::addFrameArt(HINSTANCE hModule, FrameType frameType, WORD resourceIdBothVersions, PngResource& resourceBothVersions, const char* description) {
	addImage(hModule, resourceIdBothVersions, resourceBothVersions);
	frameArtNonColorblind[frameType] = frameArtColorblind[frameType] = {
		frameType,
		&resourceBothVersions,
		ImVec2{},
		ImVec2{},
		description
	};
}

void UI::addFrameArt(HINSTANCE hModule, FrameType frameType, WORD resourceIdColorblind, PngResource& resourceColorblind,
                 WORD resourceIdNonColorblind, PngResource& resourceNonColorblind, const char* description) {
	assert(&resourceIdColorblind != &resourceIdNonColorblind);
	assert(&resourceColorblind != &resourceNonColorblind);
	addImage(hModule, resourceIdColorblind, resourceColorblind);
	addImage(hModule, resourceIdNonColorblind, resourceNonColorblind);
	frameArtColorblind[frameType] = {
		frameType,
		&resourceColorblind,
		ImVec2{},
		ImVec2{},
		description
	};
	frameArtNonColorblind[frameType] = {
		frameType,
		&resourceNonColorblind,
		ImVec2{},
		ImVec2{},
		description
	};
	
}

void UI::addFrameMarkerArt(HINSTANCE hModule, FrameMarkerType markerType, WORD resourceIdBothVersions, PngResource& resourceBothVersions) {
	addImage(hModule, resourceIdBothVersions, resourceBothVersions);
	frameMarkerArtNonColorblind[markerType] = frameMarkerArtColorblind[markerType] = {
		markerType,
		&resourceBothVersions,
		ImVec2{},
		ImVec2{}
	};
}

void UI::addFrameMarkerArt(HINSTANCE hModule, FrameMarkerType markerType, WORD resourceIdColorblind, PngResource& resourceColorblind,
                 WORD resourceIdNonColorblind, PngResource& resourceNonColorblind) {
	assert(&resourceIdColorblind != &resourceIdNonColorblind);
	assert(&resourceColorblind != &resourceNonColorblind);
	addImage(hModule, resourceIdColorblind, resourceColorblind);
	addImage(hModule, resourceIdNonColorblind, resourceNonColorblind);
	frameMarkerArtColorblind[markerType] = {
		markerType,
		&resourceColorblind,
		ImVec2{},
		ImVec2{}
	};
	frameMarkerArtNonColorblind[markerType] = {
		markerType,
		&resourceNonColorblind,
		ImVec2{},
		ImVec2{}
	};
	
}

void UI::drawFramebars() {
	static ImVec2 selStart { 0.F, 0.F };
	static ImVec2 selEnd { 0.F, 0.F };
	static bool isDragging = false;
	SelectionRect(&selStart, &selEnd, ImGuiMouseButton_Left, &isDragging);
	
	const bool showFirstFrames = settings.showFirstFramesOnFramebar;
	const bool showStrikeInvulOnFramebar = settings.showStrikeInvulOnFramebar;
	const bool showSuperArmorOnFramebar = settings.showSuperArmorOnFramebar;
	const bool showThrowInvulOnFramebar = settings.showThrowInvulOnFramebar;
	float settingsFramebarHeight = (float)settings.framebarHeight;
	if (settingsFramebarHeight < 5.F) {
		settingsFramebarHeight = 5.F;
	}
	
	static const float outerBorderThickness = 2.F;
	const float frameItselfHeight = settingsFramebarHeight - outerBorderThickness - outerBorderThickness;
	static const float frameWidthOriginal = 9.F;
	static const float frameHeightOriginal = 15.F;
	static const float frameNumberHeightOriginal = 11.F;
	static const float firstFrameHeight = 19.F;
	static const float frameMarkerWidthOriginal = 11.F;
	static const float frameMarkerWidthOffset = (frameMarkerWidthOriginal - frameWidthOriginal) * 0.5F;
	static const float frameMarkerHeightOriginal = 6.F;
	static const float frameMarkerSideHeightOriginal = 3.F;
	float frameMarkerHeight = frameMarkerHeightOriginal * frameItselfHeight / frameHeightOriginal;
	if (frameMarkerHeight < 5.F) {
		frameMarkerHeight = 5.F;
	}
	float frameMarkerSideHeight = frameMarkerSideHeightOriginal * frameMarkerHeight / frameMarkerHeightOriginal;
	static const float markerPaddingHeight = -1.F;
	static const float paddingBetweenFramebarsOriginal = 5.F;
	static const float paddingBetweenFramebarsMin = 3.F;
	static const float innerBorderThickness = 1.F;
	static const float paddingBetweenTextAndFramebar = 5.F;
	static const float textStartX = 1.F;
	static const float firstFrameHeightDiff = firstFrameHeight - frameHeightOriginal;
	static const float frameWidthScaled = frameWidthOriginal * frameItselfHeight / frameHeightOriginal;
	static const float frameNumberPaddingY = 2.F;
	static const float highlighterWidth = 2.F;
	
	float maxTopPadding;
	if (!showFirstFrames) {
		maxTopPadding = 0.F;
	} else {
		maxTopPadding = firstFrameHeightDiff - outerBorderThickness;
	}
	const float otherTopPadding = -outerBorderThickness + markerPaddingHeight + frameMarkerHeight;
	if (otherTopPadding > maxTopPadding && (showStrikeInvulOnFramebar || showSuperArmorOnFramebar)) {
		maxTopPadding = otherTopPadding;
	}
	float bottomPadding = -outerBorderThickness + markerPaddingHeight + frameMarkerHeight;
	if (bottomPadding < 0.F || !showThrowInvulOnFramebar) {
		bottomPadding = 0.F;
	}
	
	float paddingBetweenFramebars = paddingBetweenFramebarsOriginal;
	if (paddingBetweenFramebars - (maxTopPadding + bottomPadding) < paddingBetweenFramebarsMin) {
		paddingBetweenFramebars = paddingBetweenFramebarsMin + (maxTopPadding + bottomPadding);
	}
	
	const float oneFramebarHeight = outerBorderThickness
		+ frameItselfHeight
		+ outerBorderThickness;
	
	std::vector<const EntityFramebar*> framebars(2 + endScene.combinedFramebars.size(), nullptr);
	int framebarsCount = 0;
	int playerFramebarLimit = 2;
	for (int i = 0; i < 2; ++i) {
		for (const EntityFramebar& entityFramebar : endScene.framebars) {
			if (entityFramebar.playerIndex == i && entityFramebar.belongsToPlayer() && playerFramebarLimit) {
				framebars[framebarsCount++] = &entityFramebar;
				--playerFramebarLimit;
				break;
			}
		}
		for (const EntityFramebar& entityFramebar : endScene.combinedFramebars) {
			if (entityFramebar.playerIndex == i && entityFramebar.belongsToProjectile()) {
				framebars[framebarsCount++] = &entityFramebar;
			}
		}
	}
	for (const EntityFramebar& entityFramebar : endScene.combinedFramebars) {
		if (entityFramebar.playerIndex != 0 && entityFramebar.playerIndex != 1) {
			framebars[framebarsCount++] = &entityFramebar;
		}
	}
	if (framebarsCount != framebars.size()) {
		framebars.erase(framebars.begin() + framebarsCount, framebars.end());
	}
	
	float framebarsPaddingYTotal = 0.F;
	if (framebarsCount) {
		framebarsPaddingYTotal = (float)(framebarsCount - 1) * paddingBetweenFramebars;
	}
	ImGui::SetNextWindowContentSize({ 0.F,
		1.F
		+ maxTopPadding
		+ oneFramebarHeight
		* (float)framebarsCount
		+ framebarsPaddingYTotal
		+ bottomPadding
		+ 1.F
	});
	ImGui::Begin("Framebar", nullptr,
		ImGuiWindowFlags_NoBackground
		| ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoTitleBar
		| (isDragging ? ImGuiWindowFlags_NoMove : 0));
	
	const ImVec2 windowPos = ImGui::GetWindowPos();
	const float windowWidth = ImGui::GetWindowWidth();
	const float windowHeight = ImGui::GetWindowHeight();
	
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	if (ImGui::GetScrollMaxY() > 0.001F) {
		drawList->AddLine({windowPos.x, windowPos.y + 1.F},
			{windowPos.x + windowWidth, windowPos.y + 1.F},
			ImGui::GetColorU32(IM_COL32(0, 0, 0, 255)),
			1.F);
		drawList->AddLine({windowPos.x, windowPos.y + windowHeight - 2.F},
			{windowPos.x + windowWidth, windowPos.y + windowHeight - 2.F},
			ImGui::GetColorU32(IM_COL32(0, 0, 0, 255)),
			1.F);
	}
	
	char maxText[EntityFramebar::titleShortCharsCountMax + 1];
	memset(maxText, 'W', sizeof maxText);  // I know it's monospace
	maxText[sizeof maxText - 1] = '\0';
	ImVec2 maxTextSize = ImGui::CalcTextSize(maxText);
	
	const float framesX = windowPos.x
		+ textStartX
		+ maxTextSize.x 
		+ paddingBetweenTextAndFramebar
		+ outerBorderThickness;
	const float framesXEnd = windowPos.x
		+ ImGui::GetContentRegionMax().x
		+ 5.F  // window padding?
		- outerBorderThickness
		+ innerBorderThickness;
	
	float y = windowPos.y 
		+ 1.F
		+ maxTopPadding
		+ outerBorderThickness;
	
	static const float framesCountFloat = (float)_countof(Framebar::frames);
	const int framebarPosition = settings.neverIgnoreHitstop ? endScene.getFramebarPositionHitstop() : endScene.getFramebarPosition();
	ImU32 tintDarker = ImGui::GetColorU32(IM_COL32(128, 128, 128, 255));
	
	
	float frameNumberHeightDiff = frameHeightOriginal - frameItselfHeight;
	float frameNumberPaddingYUse = frameNumberPaddingY;
	if (frameNumberHeightDiff >= 0.F) {
		if (frameNumberHeightDiff >= frameNumberPaddingY + frameNumberPaddingY) {
			frameNumberPaddingYUse = 0.F;
		} else {
			frameNumberPaddingYUse = frameNumberPaddingY - frameNumberHeightDiff * 0.5F;
		}
	} else {
		frameNumberPaddingYUse = frameNumberPaddingY;
	}
	const float frameNumberHeight = frameItselfHeight - frameNumberPaddingYUse - frameNumberPaddingYUse;
	
	struct FrameDims {
		float x;
		float width;
	};
	FrameDims preppedDims[_countof(Framebar::frames)];
	float highlighterStartX;
	float highlighterEndX;
	
	if (framesXEnd > framesX) {
		float x = framesX;
		for (int i = 0; i < _countof(Framebar::frames); ++i) {
			float thisFrameXEnd = std::round((framesXEnd - framesX) * (float)(i + 1) / framesCountFloat + framesX);
			float thisFrameWidth = thisFrameXEnd - x;
			float thisFrameWidthWithoutOutline = thisFrameWidth - 1.F;
			
			if (thisFrameWidth < 0.99F) {
				thisFrameWidth = 1.F;
				thisFrameWidthWithoutOutline = 1.F;
			}
			
			FrameDims& dims = preppedDims[i];
			dims.x = x;
			dims.width = thisFrameWidthWithoutOutline;
			
			x = thisFrameXEnd;
		}
		
		highlighterStartX = preppedDims[EntityFramebar::confinePos(framebarPosition + 1)].x - innerBorderThickness;
		highlighterEndX = highlighterStartX + highlighterWidth;
		
	}
	
	const FrameArt* frameArtArray = settings.useColorblindHelp ? frameArtColorblind : frameArtNonColorblind;
	const FrameMarkerArt* frameMarkerArtArray = settings.useColorblindHelp ? frameMarkerArtColorblind : frameMarkerArtNonColorblind;
	const FrameMarkerArt& strikeInvulMarker = frameMarkerArtArray[MARKER_TYPE_STRIKE_INVUL];
	const FrameMarkerArt& throwInvulMarker = frameMarkerArtArray[MARKER_TYPE_THROW_INVUL];
	
	for (const EntityFramebar* entityFramebarPtr : framebars) {
		const EntityFramebar& entityFramebar = *entityFramebarPtr;
		const Framebar& framebar = settings.neverIgnoreHitstop ? entityFramebar.hitstop : entityFramebar.main;
		
		if (framesXEnd > framesX) {
			drawList->AddRectFilled(
				{ framesX - outerBorderThickness, y - outerBorderThickness },
				{ framesXEnd, y - outerBorderThickness + oneFramebarHeight },
				ImGui::GetColorU32(IM_COL32(0, 0, 0, 255)));
		}
		
		const char* title;
		if (entityFramebar.titleShort) {
			title = entityFramebar.titleShort;
		} else {
			title = "???";
		}
		ImVec2 textSize = ImGui::CalcTextSize(title);
		outlinedText({
				textStartX + maxTextSize.x - textSize.x,
				y - outerBorderThickness + (oneFramebarHeight - textSize.y) * 0.5F - windowPos.y
			}, title);
		if (!entityFramebar.titleFull.empty()) {
			AddTooltip(entityFramebar.titleFull.c_str());
		}
		
		if (framesXEnd > framesX) {
			char invisibleButtonImGuiId[] = "invisibleButtonImGuiId79";
			
			for (int i = 0; i < _countof(Framebar::frames); ++i) {
				const Frame& frame = framebar[i];
				const FrameDims& dims = preppedDims[i];
				
				ImU32 tint = -1;
				if (i > framebarPosition) {
					tint = tintDarker;
				}
				
				
				if (frame.type != FT_NONE) {
					ImVec2 frameStartVec { dims.x, y };
					ImVec2 frameEndVec { dims.x + dims.width, y + frameItselfHeight };
					ImVec2 frameEndVecForTooltip;
					if (i < _countof(Framebar::frames) - 1) {
						frameEndVecForTooltip = { preppedDims[i + 1].x, frameEndVec.y };
					} else {
						frameEndVecForTooltip = frameEndVec;
					}
					
					const FrameArt& frameArt = frameArtArray[frame.type];
					drawList->AddImage((ImTextureID)TEXID_FRAMES,
						frameStartVec,
						frameEndVec,
						frameArt.uvStart,
						frameArt.uvEnd,
						tint);
					ImGui::SetCursorPos({
						{ frameStartVec.x - windowPos.x },
						{ frameStartVec.y - windowPos.y }
					});
					
					invisibleButtonImGuiId[sizeof invisibleButtonImGuiId - 2] = '0' + (i / 10);
					invisibleButtonImGuiId[sizeof invisibleButtonImGuiId - 1] = '0' + (i % 10);
					
					ImGui::InvisibleButton(invisibleButtonImGuiId, {
						frameEndVecForTooltip.x - frameStartVec.x,
						frameEndVecForTooltip.y - frameStartVec.y
					}, 0);
					if (ImGui::IsItemHovered()) {
						if (ImGui::BeginTooltip()) {
							ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
							ImGui::TextUnformatted(frameArt.description);
							ImGui::PopTextWrapPos();
							ImGui::Separator();
							bool hasFull = frame.strikeInvul && frame.throwInvul;
							ImGui::EndTooltip();
						}
					}
				}
			}
			
			{
				FrameType lastFrameType = framebar.preFrame;
				int sameFrameTypeCount = framebar.preFrameLength;
				int visualFrameCount = 0;
				
				float frameNumberYTop;
				float frameNumberYBottom;
				{
					float frameNumberYTopTemp = y + frameNumberPaddingYUse;
					frameNumberYBottom = frameNumberYTopTemp + frameNumberHeight;
					if (frameNumberPaddingYUse > 0.01F) {
						frameNumberYTop = std::floor(frameNumberYTopTemp);
						frameNumberYBottom -= frameNumberYTopTemp - frameNumberYTop;
					} else {
						frameNumberYTop = frameNumberYTopTemp;
					}
				}
				
				for (int i = 0; i < _countof(Framebar::frames); ++i) {
					int ind = (framebarPosition + 1 + i) % _countof(Framebar::frames);
					const Frame& frame = framebar[ind];
					
					enum DivisionType {
						DIVISION_TYPE_NONE,
						DIVISION_TYPE_DIFFERENT_TYPES,
						DIVISION_TYPE_REACHED_END
					} divisionType = DIVISION_TYPE_NONE;
					
					int displayPos = ind - 1;
					
					if (frame.type == lastFrameType
							&& !(frame.isFirst && showFirstFrames)
							&& i == _countof(Framebar::frames) - 1
							&& lastFrameType != FT_NONE) {
						divisionType = DIVISION_TYPE_REACHED_END;
						++displayPos;
						++sameFrameTypeCount;
						++visualFrameCount;
					} else if (!(frame.type == lastFrameType && !(frame.isFirst && showFirstFrames))
							&& i != 0
							&& lastFrameType != FT_NONE) {
						divisionType = DIVISION_TYPE_DIFFERENT_TYPES;
					}
					
					if (
							divisionType != DIVISION_TYPE_NONE
							&& (sameFrameTypeCount > 3 || sameFrameTypeCount > 1 && i == 0 && divisionType == DIVISION_TYPE_DIFFERENT_TYPES)
							&& numDigits(sameFrameTypeCount) <= visualFrameCount
						) {
						
						displayPos = EntityFramebar::confinePos(displayPos);
						
						int prevIndCounter = 0;
						int sameFrameTypeCountModif = sameFrameTypeCount;
						while (sameFrameTypeCountModif) {
							
							int remainder = sameFrameTypeCountModif % 10;
							sameFrameTypeCountModif /= 10;
							
							const UVStartEnd& digitImg = digitUVs[remainder];
							
							const FrameDims& prevDim = preppedDims[EntityFramebar::confinePos(displayPos - prevIndCounter)];
							float digitX = prevDim.x;
							float digitWidth = prevDim.width;
							
							if (digitWidth > frameWidthScaled) {
								digitX += (digitWidth - frameWidthScaled) * 0.5F;
								digitWidth = frameWidthScaled;
							}
							
							drawList->AddImage((ImTextureID)TEXID_FRAMES,
								{ digitX, frameNumberYTop },
								{ digitX + digitWidth, frameNumberYBottom },
								digitImg.start,
								digitImg.end);
							
							++prevIndCounter;
						}
					}
					
					if (frame.type == lastFrameType && !(frame.isFirst && showFirstFrames)) {
						++sameFrameTypeCount;
						++visualFrameCount;
					} else {
						lastFrameType = frame.type;
						sameFrameTypeCount = 1;
						visualFrameCount = 1;
					}
				}
			}
			
			if (showStrikeInvulOnFramebar || showSuperArmorOnFramebar || showThrowInvulOnFramebar) {
				
				const float yTopRow = y - markerPaddingHeight - frameMarkerHeight;
				const float markerEndY = yTopRow + frameMarkerHeight;
				const float thisMarkerWidthPremult = frameMarkerWidthOriginal / frameWidthOriginal;
				
				for (int i = 0; i < _countof(Framebar::frames); ++i) {
					const Frame& frame = framebar[i];
					const FrameDims& dims = preppedDims[i];
					
					ImU32 tint = -1;
					if (i > framebarPosition) {
						tint = tintDarker;
					}
					
					float thisMarkerWidth = thisMarkerWidthPremult * dims.width;
					float thisMarkerWidthOffset = (thisMarkerWidth - dims.width) * 0.5F;
					ImVec2 markerStart { dims.x - thisMarkerWidthOffset, yTopRow };
					ImVec2 markerEnd { dims.x + dims.width + thisMarkerWidthOffset, markerEndY };
					
					if (frame.strikeInvulInGeneral && showStrikeInvulOnFramebar) {
						drawList->AddImage((ImTextureID)TEXID_FRAMES,
							markerStart,
							markerEnd,
							strikeInvulMarker.uvStart,
							strikeInvulMarker.uvEnd,
							tint);
						
						markerStart.y += frameMarkerSideHeight;
						markerEnd.y += frameMarkerSideHeight;
					}
					if (frame.superArmorActiveInGeneral && showSuperArmorOnFramebar) {
						const FrameMarkerArt& markerArt = frameMarkerArtArray[frame.superArmorActiveInGeneral_IsFull ? MARKER_TYPE_SUPER_ARMOR_FULL : MARKER_TYPE_SUPER_ARMOR];
						drawList->AddImage((ImTextureID)TEXID_FRAMES,
							markerStart,
							markerEnd,
							markerArt.uvStart,
							markerArt.uvEnd,
							tint);
					}
					
					if (frame.throwInvulInGeneral && showThrowInvulOnFramebar) {
						
						markerStart.y = y + frameItselfHeight + markerPaddingHeight;
						markerEnd.y = markerStart.y + frameMarkerHeight;
						
						drawList->AddImage((ImTextureID)TEXID_FRAMES,
							markerStart,
							markerEnd,
							throwInvulMarker.uvStart,
							throwInvulMarker.uvEnd,
							tint);
					}
				}
			}
			
			if (showFirstFrames) {
				
				const float firstFrameTopY = y - firstFrameHeightDiff;
				const float firstFrameBottomY = y + firstFrameHeight;
				const float innerBorderThicknessHalf = innerBorderThickness * 0.5F;
				const ImVec2 firstFrameUVStart = { firstFrame.uStart, firstFrame.vStart };
				const ImVec2 firstFrameUVEnd = { firstFrame.uEnd, firstFrame.vEnd };
				
				for (int i = 0; i < _countof(Framebar::frames); ++i) {
					const Frame& frame = framebar[i];
					const FrameDims& dims = preppedDims[i];
					
					if (frame.isFirst) {
						drawList->AddImage((ImTextureID)TEXID_FRAMES,
							{
								dims.x - innerBorderThicknessHalf - dims.width * 0.5F,
								firstFrameTopY
							},
							{
								dims.x - innerBorderThicknessHalf + dims.width * 0.5F,
								firstFrameBottomY
							},
							firstFrameUVStart,
							firstFrameUVEnd,
							-1);
					}
				}
			}
			
			{
				const float yStart = y
					- outerBorderThickness;
				
				drawList->AddRectFilled(
					{ highlighterStartX, yStart },
					{ highlighterEndX, yStart + oneFramebarHeight },
					ImGui::GetColorU32(IM_COL32(255, 255, 255, 255)));
			}
		}
		
		y += oneFramebarHeight + paddingBetweenFramebars;
	}
	
	ImGui::End();
}

bool UI::needShowFramebar() const {
	if (settings.showFramebar && (!settings.closingModWindowAlsoHidesFramebar || visible)) {
		GameMode mode = game.getGameMode();
		if (mode == GAME_MODE_TRAINING) {
			return settings.showFramebarInTrainingMode;
		} else if (mode == GAME_MODE_REPLAY) {
			return settings.showFramebarInReplayMode;
		} else {
			return settings.showFramebarInOtherModes;
		}
	} else {
		return false;
	}
}
