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
#include "InputsIcon.h"

#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"

#include <commdlg.h>  // for GetOpenFileNameW
#include "resource.h"
#include "Graphics.h"
#ifdef PERFORMANCE_MEASUREMENT
#include <chrono>
#endif
#include "colors.h"

UI ui;

static ImVec4 RGBToVec(DWORD color);
static ImVec4 ARGBToVec(DWORD color);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
// The following functions are from imgui_internal.h
IMGUI_API int           ImTextCharFromUtf8(unsigned int* out_char, const char* in_text, const char* in_text_end);               // read one character. return input UTF-8 bytes count
static inline bool      ImCharIsBlankW(unsigned int c)  { return c == ' ' || c == '\t' || c == 0x3000; }


struct imGuiDrawWrappedTextWithIcons_Icon {
	ImTextureID texId;
	ImVec2 size;
	ImVec2 uvStart;
	ImVec2 uvEnd;
};
// You must call PopTextWrapPos if you set wrap width with PushTextWrapPos before calling this function
static void imGuiDrawWrappedTextWithIcons(const char* textStart, const char* textEnd, float wrap_width, const imGuiDrawWrappedTextWithIcons_Icon* icons, int iconsCount);
static const char* imGuiDrawWrappedTextWithIcons_CalcWordWrapPositionA(float scale,
	const char* text,
	const char* text_end,
	float wrap_width,
	const imGuiDrawWrappedTextWithIcons_Icon* icons, int iconsCount,
	const imGuiDrawWrappedTextWithIcons_Icon** icon);
const ImVec2 BTN_SIZE = ImVec2(60, 20);
static ImVec4 RED_COLOR = RGBToVec(0xEF5454);
static ImVec4 YELLOW_COLOR = RGBToVec(0xF9EA6C);
static ImVec4 GREEN_COLOR = RGBToVec(0x5AE976);
static ImVec4 BLACK_COLOR = RGBToVec(0);
static ImVec4 WHITE_COLOR = RGBToVec(0xFFFFFF);
static ImVec4 SLIGHTLY_GRAY = RGBToVec(0xc2c2c2);  // it reads slightly "GRAY"
static ImVec4 LIGHT_BLUE_COLOR = RGBToVec(0x72bcf2);
static ImVec4 P1_COLOR = RGBToVec(0xff944f);
static ImVec4 P1_OUTLINE_COLOR = RGBToVec(0xd73833);
static ImVec4 P2_COLOR = RGBToVec(0x78d6ff);
static ImVec4 P2_OUTLINE_COLOR = RGBToVec(0x525fdf);
static ImVec4* P1P2_COLOR[2] = { &P1_COLOR, &P2_COLOR };
static ImVec4* P1P2_OUTLINE_COLOR[2] = { &P1_OUTLINE_COLOR, &P2_OUTLINE_COLOR };
static ImVec4 inputsDark = RGBToVec(0xa0a0a0);
static char strbuf[512];
static std::string stringArena;
static char printdecimalbuf[512];
static int numDigits(int num);  // For negative numbers does not include the '-'
struct UVStartEnd { ImVec2 start; ImVec2 end; };
static UVStartEnd digitUVs[10];
struct FrameArt { FrameType type; const PngResource* resource; ImVec2 uvStart; ImVec2 uvEnd; StringWithLength description; };
static FrameArt frameArtNonColorblind[FT_LAST];
static FrameArt frameArtColorblind[FT_LAST];
struct FrameMarkerArt { FrameMarkerType type; const PngResource* resource; ImVec2 uvStart; ImVec2 uvEnd; };
static FrameMarkerArt frameMarkerArtNonColorblind[MARKER_TYPE_LAST];
static FrameMarkerArt frameMarkerArtColorblind[MARKER_TYPE_LAST];
static const float frameWidthOriginal = 9.F;
static const float frameHeightOriginal = 15.F;
static const float frameMarkerWidthOriginal = 11.F;
static const float frameMarkerHeightOriginal = 6.F;
static const float firstFrameHeight = 19.F;
float drawFramebars_frameItselfHeight;
const FrameArt* drawFramebars_frameArtArray;
ImDrawList* drawFramebars_drawList;
ImVec2 drawFramebars_windowPos;
int drawFramebars_hoveredFrameIndex;
float drawFramebars_hoveredFrameY;
float drawFramebars_y;
const float innerBorderThicknessUnscaled = 1.F;
float drawFramebars_innerBorderThickness;
float drawFramebars_innerBorderThicknessHalf;
float drawFramebars_frameWidthScaled;
const char thisHelpTextWillRepeat[] = "Show available gatlings, whiff cancels, and whether the jump and the special cancels are available,"
					" per range of frame for this player.\n"
					"\n"
					"The frame numbers start from 1, and start from the first frame of the animation. So, for example, if the"
					" move has 2f startup (so 1f of pure startup), 1 active frame (so it becomes active on f2),"
					" and the cancels become available during the active frame,"
					" then the first group of cancels will begin on frame 2.\n"
					"\n"
					"If the move is made out of several animations, which is signaled by the main UI window showing a + sign in"
					" its 'Startup' field, then the countdown for the display of cancels in this window begins from the first move from which the chain of"
					" + added moves start. For example, a Mist Finer Cancel always breaks up into two parts with a + sign inbetween them in the main UI window."
					" It does not have cancels, but let's imagine it did. On Lv1 Johnny MFC is 9+4 frames total. If it had cancels in the second portion on its f2,"
					" the cancels in this window would show that they start on f11, because it counts from the first move (the one that is 9 total in the "
					"overall total 9+4).\n"
					"\n"
					"Available cancels may change between hit and whiff, and if the animation is canceled prematurely, not all cancells, that are still there in the move,"
					" may be displayed, because the information is being gathered from the player character directly every frame and not by reading"
					" the move's script (bbscript) ahead or in advance.\n"
					"\n"
					"The move names listed at the top might not match the names you may find when hovering your mouse over frames in the framebar to read their"
					" animation names, because the names here are only updated when a significant enough change in the animation happens.";
static std::string lastNameSuperfreeze;
static std::string lastNameAfterSuperfreeze;
#define ARGB_to_ABGR(clr) ((clr & 0xff00ff00) | ((clr & 0xff) << 16) | ((clr & 0xffffff) >> 16))
static ImVec4 COLOR_PUSHBOX_IMGUI = RGBToVec((DWORD)COLOR_PUSHBOX);
static ImVec4 COLOR_HURTBOX_IMGUI = RGBToVec((DWORD)COLOR_HURTBOX);
static ImVec4 COLOR_HURTBOX_COUNTERHIT_IMGUI = RGBToVec((DWORD)COLOR_HURTBOX_COUNTERHIT);
static ImVec4 COLOR_HURTBOX_OLD_IMGUI = RGBToVec((DWORD)COLOR_HURTBOX_OLD);
static ImVec4 COLOR_HITBOX_IMGUI = RGBToVec((DWORD)COLOR_HITBOX);
static ImVec4 COLOR_THROW_IMGUI = RGBToVec((DWORD)COLOR_THROW);
static ImVec4 COLOR_THROW_PUSHBOX_IMGUI = RGBToVec((DWORD)COLOR_THROW_PUSHBOX);
static ImVec4 COLOR_THROW_XYORIGIN_IMGUI = RGBToVec((DWORD)COLOR_THROW_XYORIGIN);
static ImVec4 COLOR_REJECTION_IMGUI = RGBToVec((DWORD)COLOR_REJECTION);
static ImVec4 COLOR_INTERACTION_IMGUI = RGBToVec((DWORD)COLOR_INTERACTION);

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
static GGIcon DISolIconRectangular = coordsToGGIcon(179, 1095, 37, 37);
static bool SelectionRect(ImVec2* start_pos, ImVec2* end_pos, ImGuiMouseButton mouse_button, bool* isDragging);
static void outlinedText(ImVec2 pos, const char* text, ImVec4* color = nullptr, ImVec4* outlineColor = nullptr);
static int printCancels(const std::vector<GatlingOrWhiffCancelInfo>& cancels);
static int printInputs(char* buf, size_t bufSize, const InputType* inputs);
static void printInputs(char*&buf, size_t& bufSize, UI::InputName** motions, int motionCount, UI::InputName** buttons, int buttonsCount);
static void printChippInvisibility(int current, int max);
static void textUnformattedColored(ImVec4 color, const char* str);
static void drawOneLineOnCurrentLineAndTheRestBelow(float wrapWidth,
		const char* str,
		const char* strEnd = nullptr,
		bool needSameLine = true,
		bool needManualMultilineOutput = false,
		bool isLastLine = true);
static void printActiveWithMaxHit(const ActiveDataArray& active, const MaxHitInfo& maxHit, int hitOnFrame);
static void drawPlayerIconInWindowTitle(int playerIndex);
static void drawPlayerIconInWindowTitle(GGIcon& icon);
static bool prevNamesControl(const PlayerInfo& player, bool includeTitle, bool disableSlang);
static void headerThatCanBeClickedForTooltip(const char* title, bool* windowVisibilityVar, bool makeTooltip);
static void prepareLastNames(const char** lastNames, const PlayerInfo& player, bool disableSlang);
static const char* formatHitResult(HitResult hitResult);
static const char* formatBlockType(BlockType blockType);
static int printChipDamageCalculation(int x, int baseDamage, int attackKezuri, int attackKezuriStandard);
static int printDamageGutsCalculation(int x, int defenseModifier, int gutsRating, int guts, int gutsLevel);
static int printScaleDmgBasic(int x, int playerIndex, int damageScale, bool isProjectile, int projectileDamageScale, HitResult hitResult, int superArmorDamagePercent);
static const char* formatAttackType(AttackType attackType);
static const char* formatGuardType(GuardType guardType);
struct ImDrawListBackup {
	std::vector<ImDrawCmd> CmdBuffer;
    std::vector<ImDrawIdx> IdxBuffer;
    std::vector<ImDrawVert> VtxBuffer;
};
static void copyDrawList(ImDrawListBackup& destination, const ImDrawList* drawList);
static void makeRenderDataFromDrawLists(std::vector<BYTE>& destination, const ImDrawData* referenceDrawData, ImDrawListBackup** drawLists, int drawListsCount);
static void printExtraHitstunTooltip(int amount);
static void printExtraHitstunText(int amount);
static void printExtraBlockstunTooltip(int amount);
static void printExtraBlockstunText(int amount);
static const char* comborepr(std::vector<int>& combo);

#define zerohspacing ImGui::PushStyleVarX(ImGuiStyleVar_ItemSpacing, 0.F);
#define _zerohspacing ImGui::PopStyleVar();
#define printWithWordWrap \
			float w = ImGui::CalcTextSize(strbuf).x; \
			if (w > ImGui::GetContentRegionAvail().x) { \
				ImGui::TextWrapped("%s", strbuf); \
			} else { \
				if (i == 0) RightAlign(w); \
				ImGui::TextUnformatted(strbuf); \
			}
			
#define printWithWordWrapArg(a) \
			float w = ImGui::CalcTextSize(a).x; \
			if (w > ImGui::GetContentRegionAvail().x) { \
				ImGui::TextWrapped("%s", a); \
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
#define advanceBuf if (result != -1) { buf += result; bufSize -= result; }


// THIS PERFORMANCE MEASUREMENT IS NOT THREAD SAFE
#ifdef PERFORMANCE_MEASUREMENT
static std::chrono::time_point<std::chrono::system_clock> performanceMeasurementStart;
#define PERFORMANCE_MEASUREMENT_DECLARE(name) \
	static unsigned long long performanceMeasurement_##name##_sum = 0; \
	static unsigned long long performanceMeasurement_##name##_count = 0; \
	static unsigned long long performanceMeasurement_##name##_average = 0;
#define PERFORMANCE_MEASUREMENT_ON_EXIT(name) \
	PerformanceMeasurementEnder performanceMeasurementEnder_##name(performanceMeasurement_##name##_sum, \
		performanceMeasurement_##name##_count, \
		performanceMeasurement_##name##_average, \
		#name);
#define PERFORMANCE_MEASUREMENT_START performanceMeasurementStart = std::chrono::system_clock::now();
#define PERFORMANCE_MEASUREMENT_END(name) \
	{ \
		PERFORMANCE_MEASUREMENT_ON_EXIT(name) \
	}
		
PERFORMANCE_MEASUREMENT_DECLARE(search)

struct PerformanceMeasurementEnder {
	PerformanceMeasurementEnder(unsigned long long& sum,
		unsigned long long& count,
		unsigned long long& average,
		const char* name) : sum(sum), count(count), average(average), name(name) { }
	~PerformanceMeasurementEnder() {
		unsigned long long duration = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now() - performanceMeasurementStart).count();
		sum += duration;
		++count;
		average = sum / count;
		logwrap(fprintf(logfile, "%s took %llu nanoseconds; average: %llu; count: %llu\n", name, duration, average, count));
	}
	unsigned long long& sum;
	unsigned long long& count;
	unsigned long long& average;
	const char* name;
};
#else
#define PERFORMANCE_MEASUREMENT_ON_EXIT(name) 
#define PERFORMANCE_MEASUREMENT_START 
#define PERFORMANCE_MEASUREMENT_END(name) 
#endif

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
	#define blockFDNotice " May or may not be able to block or FD - this information is not displayed in the frame color graphic. In general should be unable to block/FD."
	addFrameArt(hModule, FT_ACTIVE,
		IDB_ACTIVE_FRAME, activeFrame,
		IDB_ACTIVE_FRAME_NON_COLORBLIND, activeFrameNonColorblind,
		"Active: an attack is currently active. Cannot perform another attack."
		blockFDNotice);
	addFrameArt(hModule, FT_ACTIVE_NEW_HIT,
		IDB_ACTIVE_FRAME_NEW_HIT, activeFrameNewHit,
		IDB_ACTIVE_FRAME_NEW_HIT_NON_COLORBLIND, activeFrameNewHitNonColorblind,
		"Active, new (potential) hit has started: an attack is currently active. The black shadow on the left side of the frame"
		" denotes the start of a new (potential) hit. They attack may be capable of doing fewer hits than 1+\"new hits\" displayed,"
		" and the first actual hit may occur on any active frame independent of \"new hit\" frames."
		" \"New hit\" frame merely means that the hit #2, #3 and so on can only happen after a \"new hit\" frame,"
		" and, between the first hit and the next \"new hit\", the attack is inactive (even though it is displayed as active)."
		" When the second hit happens the situation resets and you need another \"new hit\" frame to do an actual hit and so on."
		" Cannot perform another attack."
		blockFDNotice);
	addFrameArt(hModule, FT_ACTIVE_HITSTOP,
		IDB_ACTIVE_FRAME_HITSTOP, activeFrameHitstop,
		IDB_ACTIVE_FRAME_HITSTOP_NON_COLORBLIND, activeFrameHitstopNonColorblind,
		"Active: an attack is currently active and the attacking player is in hitstop. Cannot perform another attack."
		blockFDNotice);
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
	addImage(hModule, IDB_HIT_CONNECTED_FRAME, hitConnectedFrame);
	addImage(hModule, IDB_HIT_CONNECTED_FRAME_BLACK, hitConnectedFrameBlack);
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
		"Idle: can cancel stance with specials, but not fire yet. Can't block or FD.");
	addFrameArt(hModule, FT_STARTUP_STANCE_CAN_STOP_HOLDING,
		IDB_IDLE_FRAME_ELPHELT_RIFLE_CAN_STOP_HOLDING, idleFrameElpheltRifleCanStopHolding,
		IDB_IDLE_FRAME_ELPHELT_RIFLE_CAN_STOP_HOLDING_NON_COLORBLIND, idleFrameElpheltRifleCanStopHoldingNonColorblind,
		"Being in some form of stance: can cancel into one or more specials. Can release the button to cancel stance. Can't block or FD or perform normal attacks.");
	addFrameArt(hModule, FT_LANDING_RECOVERY,
		IDB_LANDING_RECOVERY_FRAME, landingRecoveryFrame,
		IDB_LANDING_RECOVERY_FRAME_NON_COLORBLIND, landingRecoveryFrameNonColorblind,
		"Landing recovery: can't perform another attack."
		blockFDNotice);
	addFrameArt(hModule, FT_LANDING_RECOVERY_CAN_CANCEL,
		IDB_LANDING_RECOVERY_FRAME_CAN_CANCEL, landingRecoveryFrameCanCancel,
		IDB_LANDING_RECOVERY_FRAME_CAN_CANCEL_NON_COLORBLIND, landingRecoveryFrameCanCancelNonColorblind,
		"Landing recovery: can't perform a regular attack, but can cancel into certain moves."
		blockFDNotice);
	addFrameArt(hModule, FT_NON_ACTIVE,
		IDB_NON_ACTIVE_FRAME, nonActiveFrame,
		IDB_NON_ACTIVE_FRAME_NON_COLORBLIND, nonActiveFrameNonColorblind,
		"Non-active: a frame inbetween active frames of an attack."
		" Cannot perform another attack."
		blockFDNotice);
	addFrameArt(hModule, FT_PROJECTILE,
		IDB_PROJECTILE_FRAME, projectileFrame,
		IDB_PROJECTILE_FRAME_NON_COLORBLIND, projectileFrameNonColorblind,
		"Projectile: a projectile's attack is active. Can't perform another attack."
		blockFDNotice);
	addFrameArt(hModule, FT_RECOVERY,
		IDB_RECOVERY_FRAME, recoveryFrame,
		IDB_RECOVERY_FRAME_NON_COLORBLIND, recoveryFrameNonColorblind,
		"Recovery: an attack's active frames are already over. Can't perform another attack."
		blockFDNotice);
	addFrameArt(hModule, FT_RECOVERY_HAS_GATLINGS,
		IDB_RECOVERY_FRAME_HAS_GATLINGS, recoveryFrameHasGatlings,
		IDB_RECOVERY_FRAME_HAS_GATLINGS_NON_COLORBLIND, recoveryFrameHasGatlingsNonColorblind,
		"Recovery, but can gatling or cancel or release: an attack's active frames are already over but can gatling or cancel into some other attacks"
		" or release the button to end the attack sooner."
		blockFDNotice);
	addFrameArt(hModule, FT_RECOVERY_CAN_ACT,
		IDB_RECOVERY_FRAME_CAN_ACT, recoveryFrameCanAct,
		IDB_RECOVERY_FRAME_CAN_ACT_NON_COLORBLIND, recoveryFrameCanActNonColorblind,
		"Recovery, but can gatling or cancel or more: an attack's active frames are already over but can gatling into some other attacks or do other actions."
		blockFDNotice);
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
		"Blockstun, hitstun, holding FD, wakeup or airtech: can't perform an attack, and, if in hitstun/wakeup/tech, then block/FD.");
	addFrameArt(hModule, FT_XSTUN_CAN_CANCEL,
		IDB_XSTUN_FRAME_CAN_CANCEL, xstunFrameCanCancel,
		IDB_XSTUN_FRAME_CAN_CANCEL_NON_COLORBLIND, xstunFrameCanCancelNonColorblind,
		"Blockstun that can be cancelled into some specials: can't perform regular attacks.");
	addFrameArt(hModule, FT_XSTUN_HITSTOP,
		IDB_XSTUN_FRAME_HITSTOP, xstunFrameHitstop,
		IDB_XSTUN_FRAME_HITSTOP_NON_COLORBLIND, xstunFrameHitstopNonColorblind,
		"Blockstun, hitstun, holding FD while in hitstop: can't perform an attack, and, if in hitstun, then block/FD.");
	addFrameArt(hModule, FT_GRAYBEAT_AIR_HITSTUN,
		IDB_GRAYBEAT_AIR_HITSTUN, graybeatAirHitstunFrame,
		IDB_GRAYBEAT_AIR_HITSTUN_NON_COLORBLIND, graybeatAirHitstunFrameNonColorblind,
		"In air hitstun, but can airtech: this is a graybeat combo. Can't perform an attack or block or FD.");
	addFrameArt(hModule, FT_ZATO_BREAK_THE_LAW_STAGE2,
		IDB_ZATO_BREAK_THE_LAW_STAGE2, zatoBreakTheLawStage2Frame,
		IDB_ZATO_BREAK_THE_LAW_STAGE2_NON_COLORBLIND, zatoBreakTheLawStage2FrameNonColorblind,
		"Performing a move that can be held: can release the button any time to cancel the move."
			" The move was held long enough to have longer recovery upon release."
			" Can't block or FD or perform attacks.");
	addFrameArt(hModule, FT_ZATO_BREAK_THE_LAW_STAGE3,
		IDB_ZATO_BREAK_THE_LAW_STAGE3, zatoBreakTheLawStage3Frame,
		IDB_ZATO_BREAK_THE_LAW_STAGE3_NON_COLORBLIND, zatoBreakTheLawStage3FrameNonColorblind,
		"Performing a move that can be held: can release the button any time to cancel the move."
			" The move was held long enough to have an even longer recovery upon release."
			" Can't block or FD or perform attacks.");
	addFrameArt(hModule, FT_ZATO_BREAK_THE_LAW_STAGE2_RELEASED,
		IDB_ZATO_BREAK_THE_LAW_STAGE2_RELEASED, zatoBreakTheLawStage2ReleasedFrame,
		IDB_ZATO_BREAK_THE_LAW_STAGE2_RELEASED_NON_COLORBLIND, zatoBreakTheLawStage2ReleasedFrameNonColorblind,
		"Performing a move that can be held: the move had been held long enough to have longer recovery upon release."
			" Can't block or FD or perform attacks.");
	addFrameArt(hModule, FT_ZATO_BREAK_THE_LAW_STAGE3_RELEASED,
		IDB_ZATO_BREAK_THE_LAW_STAGE3_RELEASED, zatoBreakTheLawStage3ReleasedFrame,
		IDB_ZATO_BREAK_THE_LAW_STAGE3_RELEASED_NON_COLORBLIND, zatoBreakTheLawStage3ReleasedFrameNonColorblind,
		"Performing a move that can be held: the move had been held long enough to have an even longer recovery upon release."
			" Can't block or FD or perform attacks.");
	
	packedTexture = std::make_unique<PngResource>();
	*packedTexture = texturePacker.getTexture();
	
	FrameArt* arrays[2] = { frameArtColorblind, frameArtNonColorblind };
	FrameMarkerArt* arraysMarkers[2] = { frameMarkerArtColorblind, frameMarkerArtNonColorblind };
	for (int i = 0; i < 2; ++i) {
		arrays[i][FT_HITSTOP] = {
			FT_HITSTOP,
			idleFrame.get(),
			ImVec2{},
			ImVec2{},
			"Hitstop: the time is stopped for this player due to a hit or blocking."
		};
	}
	for (int i = 0; i < 2; ++i) {
		FrameArt* theArray = arrays[i];
		
		FrameArt& idleProjectile = theArray[FT_IDLE_PROJECTILE];
		idleProjectile = arrays[i][FT_IDLE];
		idleProjectile.description = "Projectile is not active.";
		
		FrameArt& idleSuperfreeze = theArray[FT_IDLE_ACTIVE_IN_SUPERFREEZE];
		idleSuperfreeze = arrays[i][FT_IDLE];
		idleSuperfreeze.description = "Projectile is not active.";
		
		FrameArt& activeProjectile = theArray[FT_ACTIVE_PROJECTILE];
		activeProjectile = arrays[i][FT_ACTIVE];
		activeProjectile.description = "Projectile is active.";
		
		FrameArt& activeProjectileNewHit = theArray[FT_ACTIVE_NEW_HIT_PROJECTILE];
		activeProjectileNewHit = arrays[i][FT_ACTIVE_NEW_HIT];
		activeProjectileNewHit.description = "Projectile is active, new (potential) hit has started:"
			" The black shadow on the left side of the frame denotes the start of a new (potential) hit."
			" The projectile may be capable of doing fewer hits than 1+the number of \"new hits\" displayed,"
			" and the first actual hit may occur on any active frame independent of \"new hit\" frames."
			" \"New hit\" frame merely means that the hit #2, #3 and so on can only happen after a \"new hit\" frame,"
			" and, between the first hit and the next \"new hit\", projectile is inactive (even though it is displayed as active)."
			" When the second hit happens the situation resets and you need another \"new hit\" frame to do an actual hit and so on.";
		
		FrameArt& activeProjectileHitstop = theArray[FT_ACTIVE_HITSTOP_PROJECTILE];
		activeProjectileHitstop = arrays[i][FT_ACTIVE_HITSTOP];
		activeProjectileHitstop.description = "Projectile is active, and in hitstop: while the projectile is in hitstop, time doesn't advance for it and"
			" it doesn't hit enemies.";
		
		FrameArt& nonActiveProjectile = theArray[FT_NON_ACTIVE_PROJECTILE];
		nonActiveProjectile = arrays[i][FT_NON_ACTIVE];
		nonActiveProjectile.description = "A frame inbetweeen the active frames of a projectile.";
		
		FrameArt& startupAnytimeNow = theArray[FT_STARTUP_ANYTIME_NOW];
		startupAnytimeNow = arrays[i][FT_IDLE_ELPHELT_RIFLE];
		startupAnytimeNow.description = "Startup of a chargeable move: can release the button any time to either attack or cancel the move."
			" Can't block or FD or perform normal attacks.";
		
		FrameArt& startupProgramSecretGarden = theArray[FT_STARTUP_CAN_PROGRAM_SECRET_GARDEN];
		startupProgramSecretGarden = arrays[i][FT_IDLE_ELPHELT_RIFLE];
		startupProgramSecretGarden.description = "Startup: can program Secret Garden."
			" Can't block or FD or perform normal attacks.";
		
		FrameArt& startupStance = theArray[FT_STARTUP_STANCE];
		startupStance = arrays[i][FT_IDLE_ELPHELT_RIFLE];
		startupStance.description = "Being in some form of stance: can cancel into one or more specials. Can't block or FD or perform normal attacks.";
		
		FrameArt& elpheltRifleReady = theArray[FT_IDLE_ELPHELT_RIFLE_READY];
		elpheltRifleReady = arrays[i][FT_STARTUP_STANCE_CAN_STOP_HOLDING];
		elpheltRifleReady.description = "Idle: Can cancel the stance into specials or fire the rifle. Can't block or FD or perform normal attacks.";
		
		FrameArt& canBlockAndCancel = theArray[FT_STARTUP_CAN_BLOCK_AND_CANCEL];
		canBlockAndCancel = arrays[i][FT_STARTUP_STANCE_CAN_STOP_HOLDING];
		canBlockAndCancel.description = "Startup, but can block and cancel into certain moves:"
		" an attack's active frames have not yet started, or this is not an attack."
		" Can block and/or maybe FD, possibly can't switch block, and can cancel into certain moves.";
		
		FrameArt& rifleReload = theArray[FT_RECOVERY_CAN_RELOAD];
		rifleReload = arrays[i][FT_RECOVERY_HAS_GATLINGS];
		rifleReload.description = "Recovery, but can reload. Can't attack, block or FD.";
		
		FrameArt& startupAnytimeNowCanAct = theArray[FT_STARTUP_ANYTIME_NOW_CAN_ACT];
		startupAnytimeNowCanAct = arrays[i][FT_IDLE_CANT_BLOCK];
		startupAnytimeNowCanAct.description = "Startup of a chargeable move: can release the button any time to either attack or cancel the move,"
			" and can also cancel into other moves."
			" Can't block or FD or perform normal attacks.";
		
		FrameArt& airborneIdleCanGroundBlock = theArray[FT_IDLE_AIRBORNE_BUT_CAN_GROUND_BLOCK];
		airborneIdleCanGroundBlock = arrays[i][FT_IDLE_CANT_FD];
		airborneIdleCanGroundBlock.description = "Idle while airborne on the pre-landing (last airborne) frame:"
			" can block grounded on this frame and regular (without FD) block ground attacks that require FD to be blocked in the air."
			" Can attack, block and FD.";
		
		for (int j = 1; j < _countof(frameArtNonColorblind); ++j) {
			FrameArt& art = theArray[j];
			#ifdef _DEBUG
			if (!art.resource) {
				MessageBoxW(NULL, L"Null frame art.", L"Error", MB_OK);
				return false;
			}
			#endif
			art.uvStart = { art.resource->uStart, art.resource->vStart };
			art.uvEnd = { art.resource->uEnd, art.resource->vEnd };
		}
	}
	#undef blockFDNotice
	for (int i = 0; i < 2; ++i) {
		for (int j = 0; j < _countof(frameMarkerArtNonColorblind); ++j) {
			FrameMarkerArt& art = arraysMarkers[i][j];
			#ifdef _DEBUG
			if (!art.resource) {
				MessageBoxW(NULL, L"Null frame marker art.", L"Error", MB_OK);
				return false;
			}
			#endif
			art.uvStart = { art.resource->uStart, art.resource->vStart };
			art.uvEnd = { art.resource->uEnd, art.resource->vEnd };
		}
	}
	for (int i = 0; i < 10; ++i) {
		const PngResource& res = *digitFrame[i];
		digitUVs[i] = {
			{ res.uStart, res.vStart },
			{ res.uEnd, res.vEnd }
		};
	}
	
	inputNames[INPUT_HOLD_P] = { "[P]", BUTTON };
	inputNames[INPUT_P_STRICT_PRESS] = { "S (no buffer)",  BUTTON };
	inputNames[INPUT_P_STRICT_RELEASE] = { "]S[ (no buffer)",  BUTTON };
	inputNames[INPUT_PRESS_P] = { "P",  BUTTON };
	inputNames[INPUT_NOT_HOLD_P] = { "don't hold P",  MULTIWORD_BUTTON };
	inputNames[INPUT_RELEASE_P] = { "]P[",  BUTTON };
	inputNames[INPUT_HOLD_K] = { "[K]", BUTTON };
	inputNames[INPUT_K_STRICT_PRESS] = { "K (no buffer)", BUTTON };
	inputNames[INPUT_K_STRICT_RELEASE] = { "]K[ (no buffer)", BUTTON };
	inputNames[INPUT_PRESS_K] = { "K", BUTTON };
	inputNames[INPUT_NOT_HOLD_K] = { "don't hold K", MULTIWORD_BUTTON };
	inputNames[INPUT_RELEASE_K] = { "]K[", BUTTON };
	inputNames[INPUT_HOLD_S] = { "[S]", BUTTON };
	inputNames[INPUT_S_STRICT_PRESS] = { "S (no buffer)", BUTTON };
	inputNames[INPUT_S_STRICT_RELEASE] = { "]S[ (no buffer)", BUTTON };
	inputNames[INPUT_PRESS_S] = { "S", BUTTON };
	inputNames[INPUT_NOT_HOLD_S] = { "don't hold S", MULTIWORD_BUTTON };
	inputNames[INPUT_RELEASE_S] = { "]S[", BUTTON };
	inputNames[INPUT_HOLD_H] = { "[H]", BUTTON };
	inputNames[INPUT_H_STRICT_PRESS] = { "H (no buffer)", BUTTON };
	inputNames[INPUT_H_STRICT_RELEASE] = { "]H[ (no buffer)", BUTTON };
	inputNames[INPUT_PRESS_H] = { "H", BUTTON };
	inputNames[INPUT_NOT_HOLD_H] = { "don't hold H", MULTIWORD_BUTTON };
	inputNames[INPUT_RELEASE_H] = { "]H[", BUTTON };
	inputNames[INPUT_HOLD_D] = { "[D]", BUTTON };
	inputNames[INPUT_D_STRICT_PRESS] = { "D (no buffer)", BUTTON };
	inputNames[INPUT_D_STRICT_RELEASE] = { "]D[ (no buffer)", BUTTON };
	inputNames[INPUT_PRESS_D] = { "D", BUTTON };
	inputNames[INPUT_NOT_HOLD_D] = { "don't hold D", MULTIWORD_BUTTON };
	inputNames[INPUT_RELEASE_D] = { "]D[", BUTTON };
	inputNames[INPUT_HOLD_TAUNT] = { "[Taunt]", BUTTON };
	inputNames[INPUT_TAUNT_STRICT_PRESS] = { "Taunt (no buffer)", BUTTON };
	inputNames[INPUT_TAUNT_STRICT_RELEASE] = { "]Taunt[ (no buffer)", BUTTON };
	inputNames[INPUT_PRESS_TAUNT] = { "Taunt", BUTTON };
	inputNames[INPUT_NOT_HOLD_TAUNT] = { "don't hold Taunt", MULTIWORD_BUTTON };
	inputNames[INPUT_RELEASE_TAUNT] = { "Taunt", BUTTON };
	inputNames[INPUT_1] = { "1", MOTION };
	inputNames[INPUT_4_OR_1_OR_2] = { "4/1/2", MOTION };
	inputNames[INPUT_NOT_1] = { "don't hold 1", MULTIWORD_MOTION };
	inputNames[INPUT_NOT_4_OR_1_OR_2] = { "don't hold 4/1/2", MULTIWORD_MOTION };
	inputNames[INPUT_2] = { "2", MOTION };
	inputNames[INPUT_ANYDOWN] = { "1/2/3", MOTION };
	inputNames[INPUT_ANYDOWN_STRICT_PRESS] = { "press 1/2/3 (no buffer)", MULTIWORD_BUTTON };
	inputNames[INPUT_NOT_2] = { "don't hold 2", MULTIWORD_MOTION };
	inputNames[INPUT_NOTANYDOWN] = { "don't hold 1/2/3", MULTIWORD_MOTION };
	inputNames[INPUT_3] = { "3", MOTION };
	inputNames[INPUT_6_OR_3_OR_2] = { "6/3/2", MOTION };
	inputNames[INPUT_NOT_3] = { "don't hold 3", MULTIWORD_MOTION };
	inputNames[INPUT_NOT_6_OR_3_OR_2] = { "don't hold 6/3/2", MULTIWORD_MOTION };
	inputNames[INPUT_4] = { "4", MOTION };
	inputNames[INPUT_ANYBACK] = { "7/4/1", MOTION };
	inputNames[INPUT_ANYBACK_STRICT_PRESS] = { "press 7/4/1 (no buffer)", MULTIWORD_BUTTON };
	inputNames[INPUT_NOT_4] = { "don't hold 4", MULTIWORD_MOTION };
	inputNames[INPUT_NOTANYBACK] = { "don't hold 7/4/1", MULTIWORD_MOTION };
	inputNames[INPUT_5] = { "5", MOTION };
	inputNames[INPUT_ALWAYS_TRUE_DUPLICATE] = { "nullptr", MULTIWORD_MOTION };
	inputNames[INPUT_NOT_5] = { "don't hold 5", MULTIWORD_MOTION };
	inputNames[INPUT_6] = { "6", MOTION };
	inputNames[INPUT_ANYFORWARD] = { "9/6/3", MOTION };
	inputNames[INPUT_ANYFORWARD_STRICT_PRESS] = { "press 9/6/3 (no buffer)", MULTIWORD_BUTTON };
	inputNames[INPUT_NOT_6] = { "don't hold 6", MULTIWORD_MOTION };
	inputNames[INPUT_NOTANYFORWARD] = { "don't hold 9/6/3", MULTIWORD_MOTION };
	inputNames[INPUT_7] = { "7", MOTION };
	inputNames[INPUT_4_OR_7_OR_8] = { "4/7/8", MOTION };
	inputNames[INPUT_NOT_7] = { "don't hold 7", MULTIWORD_MOTION };
	inputNames[INPUT_NOT_4_OR_7_OR_8] = { "don't hold 4/7/8", MULTIWORD_MOTION };
	inputNames[INPUT_8] = { "8", MOTION };
	inputNames[INPUT_ANYUP] = { "7/8/9", MOTION };
	inputNames[INPUT_ANYUP_STRICT_PRESS] = { "press 7/8/9 (no buffer)", MULTIWORD_BUTTON };
	inputNames[INPUT_NOT_8] = { "don't hold 8", MULTIWORD_MOTION };
	inputNames[INPUT_NOTANYUP] = { "don't hold 7/8/9", MULTIWORD_MOTION };
	inputNames[INPUT_9] = { "9", MOTION };
	inputNames[INPUT_6_OR_9_OR_8] = { "6/9/8", MOTION };
	inputNames[INPUT_NOT_9] = { "don't hold 9", MULTIWORD_MOTION };
	inputNames[INPUT_NOT_6_OR_9_OR_8] = { "don't hold 6/9/8", MULTIWORD_MOTION };
	inputNames[INPUT_236] = { "236", MOTION };
	inputNames[INPUT_623] = { "623", MOTION };
	inputNames[INPUT_214] = { "214", MOTION };
	inputNames[INPUT_41236] = { "41236", MOTION };
	inputNames[INPUT_421] = { "421", MOTION };
	inputNames[INPUT_63214] = { "63214", MOTION };
	inputNames[INPUT_236236] = { "236236", MOTION };
	inputNames[INPUT_214214] = { "214214", MOTION };
	inputNames[INPUT_4123641236] = { "4123641236", MOTION };
	inputNames[INPUT_6321463214] = { "6321463214", MOTION };
	inputNames[INPUT_632146] = { "632146", MOTION };
	inputNames[INPUT_641236] = { "641236", MOTION };
	inputNames[INPUT_2141236] = { "2141236", MOTION };
	inputNames[INPUT_2363214] = { "2363214", MOTION };
	inputNames[INPUT_22] = { "22", MOTION };
	inputNames[INPUT_46] = { "46", MOTION };
	inputNames[INPUT_CHARGE_BACK_FORWARD_30F] = { "charge back 30f -> forward", MULTIWORD_MOTION };
	inputNames[INPUT_CHARGE_DOWN_UP_30F] = { "charge down 30f -> up", MULTIWORD_MOTION };
	inputNames[INPUT_6428] = { "6428", MOTION };
	inputNames[INPUT_CHARGE_BACK_UP_30F] = { "charge back 30f -> up", MULTIWORD_MOTION };
	inputNames[INPUT_64641236] = { "64641236", MOTION };
	inputNames[INPUT_342646] = { "342646", MOTION };
	inputNames[INPUT_28] = { "28", MOTION };
	inputNames[INPUT_646] = { "646", MOTION };
	inputNames[INPUT_P_MASH] = { "mash (press 5 times) P", MULTIWORD_BUTTON };
	inputNames[INPUT_K_MASH] = { "mash (press 5 times) K", MULTIWORD_BUTTON };
	inputNames[INPUT_S_MASH] = { "mash (press 5 times) S", MULTIWORD_BUTTON };
	inputNames[INPUT_H_MASH] = { "mash (press 5 times) H", MULTIWORD_BUTTON };
	inputNames[INPUT_D_MASH] = { "mash (press 5 times) D", MULTIWORD_BUTTON };
	inputNames[INPUT_CIRCLE] = { "circle", MULTIWORD_MOTION };
	inputNames[INPUT_222] = { "222", MOTION };
	inputNames[INPUT_2222] = { "2222", MOTION };
	inputNames[INPUT_236236_STRICTER] = { "236236", MOTION };
	inputNames[INPUT_PRESS_D_DUPLICATE] = { "D", BUTTON };
	inputNames[INPUT_HOLD_6_OR_3_AND_PRESS_TWO_OF_PKSH] = { "6/3+any two of PKSH", MULTIWORD_BUTTON };
	inputNames[INPUT_ROMAN_CANCEL] = { "Roman Cancel", MULTIWORD_BUTTON };
	inputNames[INPUT_SUPERJUMP] = { "Superjump", BUTTON };
	inputNames[INPUT_ANYUP_STRICT_PRESS_DUPLICATE] = { "press 7/8/9 (no buffer)", MULTIWORD_BUTTON };
	inputNames[INPUT_FORWARD_DASH] = { "66", MOTION };
	inputNames[INPUT_BACKDASH] = { "44", MOTION };
	inputNames[INPUT_BLOCK_WITH_CROSSUP_PROTECTION] = { "hold block (when crossup immunity (3f) is in effect, both directions of block will suffice or even not blocking"
		" at all and simply having held block within the past 6f."
		" If in the air and the opponent is behind you, you may hold block in either direction)", MULTIWORD_MOTION };
	inputNames[INPUT_BLOCK_OR_CROSSUP_AIR_BLOCK] = { "hold block (or if in the air and the opponent is behind you, then you may hold block in either direction)", MULTIWORD_MOTION };
	inputNames[INPUT_151] = { "151", MOTION };
	inputNames[INPUT_252] = { "252", MOTION };
	inputNames[INPUT_353] = { "353", MOTION };
	inputNames[INPUT_454] = { "454", MOTION };
	inputNames[INPUT_66_QUICK] = { "66", MOTION };
	inputNames[INPUT_757] = { "757", MOTION };
	inputNames[INPUT_858] = { "858", MOTION };
	inputNames[INPUT_959] = { "959", MOTION };
	inputNames[INPUT_ALWAYS_TRUE_DUPLICATE2] = { "nullptr", MULTIWORD_MOTION };
	inputNames[INPUT_ALWAYS_FALSE_DUPLICATE] = { "nullptr", MULTIWORD_MOTION };
	inputNames[INPUT_623_LENIENT] = { "623", MOTION };
	inputNames[INPUT_22_LENIENT] = { "22", MOTION };
	inputNames[INPUT_5_OR_4_OR_ANY_UP] = { "5/4/7/8/9", MOTION };
	inputNames[INPUT_5_OR_ANY_UP] = { "5/7/8/9", MOTION };
	inputNames[INPUT_5_OR_6_OR_ANY_UP] = { "5/6/7/8/9", MOTION };
	inputNames[INPUT_5_OR_4_OR_7_OR_8] = { "5/4/7/8", MOTION };
	inputNames[INPUT_421_LENIENT] = { "421", MOTION };
	inputNames[INPUT_16243] = { "16243", MOTION };
	inputNames[INPUT_546] = { "546", MOTION };
	inputNames[INPUT_5_ANYBACK_ANYFORWARD_STRICTER] = { "5(7/4/1)(9/6/3)", MOTION };
	inputNames[INPUT_5_ANYFORWARD_ANYBACK] = { "5(9/6/3)(7/4/1)", MOTION };
	inputNames[INPUT_CHARGE_BACK_FORWARD_40F] = { "charge back 40f -> forward", MULTIWORD_MOTION };
	inputNames[INPUT_CHARGE_DOWN_UP_40F] = { "charge down 40f -> up", MULTIWORD_MOTION };
	inputNames[INPUT_CHARGE_BACK_FORWARD_45F] = { "charge back 45f -> forward", MULTIWORD_MOTION };
	inputNames[INPUT_CHARGE_DOWN_UP_45F] = { "charge down 45f -> up", MULTIWORD_MOTION };
	inputNames[INPUT_236236236] = { "236236236", MOTION };
	inputNames[INPUT_623_WITHIN_LAST_3F] = { "623 (3f buffer)", MULTIWORD_MOTION };
	inputNames[INPUT_5_ANYBACK_ANYFORWARD_WITHIN_LAST_2F] = { "5(7/4/1)(9/6/3) (2f buffer)", MULTIWORD_MOTION };
	inputNames[INPUT_NOTANYDOWN_2] = { "(not 1/2/3) -> 2", MULTIWORD_MOTION };
	inputNames[INPUT_46_WITHIN_LAST_1F] = { "46 (no buffer)", MULTIWORD_MOTION };
	inputNames[INPUT_CHARGE_DOWN_10F] = { "charge down 10f", MULTIWORD_MOTION };
	inputNames[INPUT_546_BUTNOT_54_ANYDOWN_6] = { "(546, but not 54(1/2/3)6)", MOTION };
	inputNames[INPUT_5_ANYBACK_ANYFORWARD_LENIENT] = { "5(7/4/1)(9/6/3)", MOTION };
	inputNames[INPUT_BURST] = { "Burst", BUTTON };
	inputNames[INPUT_HOLD_TWO_OR_MORE_OF_PKSH] = { "hold two or more of PKSH", MULTIWORD_BUTTON };
	inputNames[INPUT_PRESS_TWO_OR_MORE_OF_PKSH_GLITCHED] = { "press two or more of PKSH (obsolete)", MULTIWORD_BUTTON };  // why it's glitched? I think it relies on a 2f buttonpress buffer. Rn it's 3. So it gets its flag set on f1 of press and then on f2 it's unset and on f3 it's set again and then you need to press the button again to continue it switching on and off each frame. Activating twice from one press on frames 1 and 3 is a glitch thank you for reading this far I hope you're happy and all is well
	inputNames[INPUT_PRESS_ANYBACK_WITHIN_LAST_8F_NO_MASH_ALLOWED] = { "7/4/1 within last 8f no mash allowed", MULTIWORD_BUTTON };
	inputNames[INPUT_P_OR_K_OR_S_OR_H] = { "P/K/S/H", BUTTON };
	inputNames[INPUT_BOOLEAN_OR] = { "or", BUTTON };
	inputNames[INPUT_HAS_PRECEDING_5] = { "preceding 5", MULTIWORD_MOTION };
	inputNames[INPUT_ITEM] = { "Item", BUTTON };
	inputNames[INPUT_HOLD_SPECIAL] = { "[Sp]", BUTTON };
	inputNames[INPUT_SPECIAL_STRICT_PRESS] = { "Sp (no buffer)", BUTTON };
	inputNames[INPUT_SPECIAL_STRICT_RELEASE] = { "]Sp[ (no buffer)", BUTTON };
	inputNames[INPUT_PRESS_SPECIAL] = { "Sp", BUTTON };
	inputNames[INPUT_NOT_HOLD_SPECIAL] = { "don't hold Sp", MULTIWORD_BUTTON };
	inputNames[INPUT_RELEASE_SPECIAL] = { "]Sp[", BUTTON };
	inputNames[INPUT_ANY_TWO_OF_PKSH] = { "any two of PKSH", MULTIWORD_BUTTON };
	inputNames[INPUT_ROMAN_CANCEL_DUPLICATE] = { "Roman Cancel", MULTIWORD_BUTTON };
	inputNames[INPUT_MOM_TAUNT] = { "MOM Taunt", MULTIWORD_BUTTON };
	inputNames[INPUT_FORWARD_DASH_WITHIN_LAST_2F] = { "66 (2f buffer)", MULTIWORD_MOTION };
	inputNames[INPUT_BACKDASH_WITHIN_LAST_2F] = { "44 (2f buffer)", MULTIWORD_MOTION };
	inputNames[INPUT_P_OR_K_OR_S_OR_H_OR_D_STRICT_PRESS] = { "P/K/S/H/D (no buffer)", BUTTON };
	inputNames[INPUT_ALWAYS_FALSE] = { "nullptr", MULTIWORD_MOTION };
	inputNames[INPUT_PRESS_TAUNT_DUPLICATE] = { "Taunt", BUTTON };
	inputNames[INPUT_HOLD_ONE_OR_MORE_OF_PKSH] = { "hold one or more of P/K/S/H", MULTIWORD_BUTTON };
	inputNames[INPUT_HOLD_ONE_OR_MORE_OF_PKSH_OR_D] = { "hold one or more of P/K/S/H/D", MULTIWORD_BUTTON };
	
	errorDialogPos = new ImVec2();
	
	return true;
}

// Stops queueing new timer events on the window (main) thread. KillTimer does not remove events that have already been queued
void UI::onDllDetachStage1_killTimer() {
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
				// we won't call KillTimer from a non-WndProc thread
				logwrap(fprintf(logfile, "Failed to kill timer not from the WndProc\n"));
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

// Runs on the main thread
void UI::prepareDrawData() {
	drewFramebar = false;
	drewFrameTooltip = false;
	if (!visible && !needShowFramebarCached || gifMode.modDisabled) {
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
	imguiActiveTemp = false;
	takeScreenshotTemp = false;
	
	decrementFlagTimer(allowNextFrameTimer, allowNextFrame);
	decrementFlagTimer(takeScreenshotTimer, takeScreenshotPress);
	for (int i = 0; i < 2; ++i) {
		decrementFlagTimer(clearTensionGainMaxComboTimer[i], clearTensionGainMaxCombo[i]);
	}
	
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	
	if (visible) {
		
		drawSearchableWindows();
		
		if (showErrorDialog && errorDialogText && *errorDialogText != '\0') {
			ImGui::SetNextWindowPos(*(ImVec2*)errorDialogPos, ImGuiCond_Appearing);
			ImGui::Begin("Error", &showErrorDialog);
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			ImGui::TextUnformatted(errorDialogText);
			ImGui::PopTextWrapPos();
			ImGui::End();
		}
		
		if (!shaderCompilationError) {
			graphics.getShaderCompilationError(&shaderCompilationError);
		}
		if (shaderCompilationError && showShaderCompilationError) {
			ImGui::SetNextWindowSize({ 500.F, 0.F }, ImGuiCond_FirstUseEver);
			ImGui::Begin("Shader compilation error", &showShaderCompilationError);
			ImGui::PushTextWrapPos(0.F);
			ImGui::TextUnformatted(shaderCompilationError->c_str());
			ImGui::PopTextWrapPos();
			ImGui::End();
		}
		if (showSearch) {
			searchWindow();
		}
	}
	
	if (needShowFramebarCached) {
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

void UI::drawSearchableWindows() {
	static std::string windowTitle;
	if (windowTitle.empty()) {
		windowTitle = "ggxrd_hitbox_overlay v";
		windowTitle += VERSION;
	}
	if (searching) {
		two = 1;
		ImGui::SetNextWindowPos({ 100000.F, 100000.F }, ImGuiCond_Always);
		ImGui::PushID(34789572);
	} else {
		two = 2;
	}
	ImGui::Begin(searching ? "search_main" : windowTitle.c_str(), &visible, searching ? ImGuiWindowFlags_NoSavedSettings : 0);
	pushSearchStack("Main UI Window");
	
	if (ImGui::CollapsingHeader(searchCollapsibleSection("Framedata"), ImGuiTreeNodeFlags_DefaultOpen) || searching) {
		if (endScene.isIGiveUp() && !searching) {
			ImGui::TextUnformatted("Online non-observer match running.");
		} else
		if (ImGui::BeginTable("##PayerData",
					3,
					ImGuiTableFlags_Borders
					| ImGuiTableFlags_RowBg
					| ImGuiTableFlags_NoSavedSettings
					| ImGuiTableFlags_NoPadOuterX)
		) {
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
			AddTooltip("Hover your mouse cursor over individual row titles to see their corresponding tooltips.");
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
			CenterAlignedText(searchFieldTitle("HP"));
			AddTooltip(searchTooltip("HP (x Guts) [x Defense Modifier]\n"
				"Technically you should divide HP by these values in order to get effective HP, because they're what all damage is multiplied by."));
			
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
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				sprintf_s(strbuf, "%s", printDecimal(player.tension, 2, 0));
				printNoWordWrap
				
				if (i == 0) {
					ImGui::TableNextColumn();
					CenterAlignedText(searchFieldTitle("Meter"));
					AddTooltip(searchTooltip("Tension"));
				}
			}
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				sprintf_s(strbuf, "%s", printDecimal(player.burst, 2, 0));
				printNoWordWrap
				
				if (i == 0) {
					ImGui::TableNextColumn();
					CenterAlignedText(searchFieldTitle("Burst"));
				}
			}
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				sprintf_s(strbuf, "%s", printDecimal(player.risc, 2, 0));
				printNoWordWrap
				
				if (i == 0) {
					ImGui::TableNextColumn();
					CenterAlignedText(searchFieldTitle("RISC"));
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
					CenterAlignedText(searchFieldTitle("Stun"));
				}
			}
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				printDecimal(player.x, 2, 0);
				sprintf_s(strbuf, "%s; ", printdecimalbuf);
				printDecimal(player.y, 2, 0);
				sprintf_s(strbuf + strlen(strbuf), sizeof strbuf - strlen(strbuf), "%s", printdecimalbuf);
				printNoWordWrap
				
				if (i == 0) {
					ImGui::TableNextColumn();
					CenterAlignedText(searchFieldTitle("X; Y"));
					AddTooltip(searchTooltip("Position X; Y in the arena. Divided by 100 for viewability."));
				}
			}
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				player.printStartup(strbuf, sizeof strbuf);
				printWithWordWrap
				
				if (i == 0) {
					ImGui::TableNextColumn();
					headerThatCanBeClickedForTooltip(searchFieldTitle("Startup"), &showStartupTooltip, false);
					if (ImGui::BeginItemTooltip()) {
						ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
						if (prevNamesControl(player, true, false)) {
							searchFieldValue(strbuf, nullptr);
							printNoWordWrap
							ImGui::Separator();
						}
						ImGui::TextUnformatted("Click the field for tooltip.");
						ImGui::PopTextWrapPos();
						ImGui::EndTooltip();
					}
				}
			}
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				if (player.startedUp || player.startupProj) {
					printActiveWithMaxHit(player.activesDisp, player.maxHitDisp, player.hitOnFrameDisp);
				} else {
					*strbuf = '\0';
				}
				printWithWordWrap
				
				if (i == 0) {
					ImGui::TableNextColumn();
					headerThatCanBeClickedForTooltip(searchFieldTitle("Active"), &showActiveTooltip, true);
				}
			}
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				player.printRecovery(strbuf, sizeof strbuf);
				printWithWordWrap
				
				if (i == 0) {
					ImGui::TableNextColumn();
					CenterAlignedText(searchFieldTitle("Recovery"));
					AddTooltip(searchTooltip("Number of recovery frames in the last performed move."
						" If the move spawned a projectile that lasted beyond the boundaries of the move, its recovery is 0.\n"
						"See the tooltip for the 'Total' field for more details."));
				}
			}
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				player.printTotal(strbuf, sizeof strbuf);
				printWithWordWrap
				
				if (i == 0) {
					ImGui::TableNextColumn();
					headerThatCanBeClickedForTooltip(searchFieldTitle("Total"), &showTotalTooltip, false);
					if (ImGui::BeginItemTooltip()) {
						ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
						if (prevNamesControl(player, true, false)) {
							searchFieldValue(strbuf, nullptr);
							printNoWordWrap
							ImGui::Separator();
						}
						ImGui::TextUnformatted("Click the field for tooltip.");
						ImGui::PopTextWrapPos();
						ImGui::EndTooltip();
					}
				}
			}
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				player.printInvuls(strbuf, sizeof strbuf);
				searchFieldValue(strbuf, nullptr);
				float w = ImGui::CalcTextSize(strbuf).x;
				const char* cantPos = strstr(strbuf, "can't armor unblockables");
				float wrapWidth = ImGui::GetContentRegionAvail().x;
				if (w > wrapWidth) {
					wrapWidth += ImGui::GetCursorPosX();
					if (cantPos) {
						zerohspacing
						drawOneLineOnCurrentLineAndTheRestBelow(wrapWidth, strbuf, cantPos, false, true, false);
						ImGui::PushStyleColor(ImGuiCol_Text, RED_COLOR);
						drawOneLineOnCurrentLineAndTheRestBelow(wrapWidth, "can't", nullptr, true, true, false);
						ImGui::PopStyleColor();
						drawOneLineOnCurrentLineAndTheRestBelow(wrapWidth, cantPos + 5, nullptr, true, true, true);
						_zerohspacing
					} else {
						ImGui::TextWrapped("%s", strbuf);
					}
				} else {
					if (i == 0) RightAlign(w);
					if (cantPos) {
						zerohspacing
						ImGui::TextUnformatted(strbuf, cantPos);
						ImGui::SameLine();
						textUnformattedColored(RED_COLOR, "can't");
						ImGui::SameLine();
						ImGui::TextUnformatted(cantPos + 5);
						_zerohspacing
					} else {
						ImGui::TextUnformatted(strbuf);
					}
					
				}
				
				if (i == 0) {
					ImGui::TableNextColumn();
					headerThatCanBeClickedForTooltip(searchFieldTitle("Invul"), &showInvulTooltip, true);
				}
			}
			struct {
				bool displayFloorbounceQuestionMark = false;
				int displayFloorbounceQuestionMarkValue = 0;
				bool displayBlockstunLandQuestionMark = false;
				int displayBlockstunLandQuestionMarkValue = 0;
			} playerBlocks[2];
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				if (player.xStunDisplay == PlayerInfo::XSTUN_DISPLAY_HIT) {
					if (player.hitstunMaxFloorbounceExtra) {
						playerBlocks[i].displayFloorbounceQuestionMarkValue = player.hitstunMaxFloorbounceExtra;
						playerBlocks[i].displayFloorbounceQuestionMark = true;
					}
				} else if (player.xStunDisplay == PlayerInfo::XSTUN_DISPLAY_BLOCK) {
					if (player.blockstunMaxLandExtra) {
						playerBlocks[i].displayBlockstunLandQuestionMarkValue = player.blockstunMaxLandExtra;
						playerBlocks[i].displayBlockstunLandQuestionMark = true;
					}
				}
			}
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				*strbuf = '\0';
				int strbufLen = 0;
				if (player.displayHitstop) {
					strbufLen = sprintf_s(strbuf, "%d/%d", player.hitstopWithSlow, player.hitstopMaxWithSlow);
				}
				char* ptrNext = strbuf;
				int ptrNextSize = sizeof strbuf;
				if (*strbuf) {
					ptrNext += strbufLen + 3;  // strlen(" + ")
					*ptrNext = '\0';
					ptrNextSize -= (ptrNext - strbuf);
				}
				size_t ptrNextSizeCap = ptrNextSize < 0 ? 0 : (size_t)ptrNextSize;
				ptrNextSize = 0;
				const char* blockType;
				if (player.lastBlockWasIB) {
					blockType = " (IB)";
				} else if (player.lastBlockWasFD) {
					blockType = " (FD)";
				} else {
					blockType = "";
				}
				if (player.xStunDisplay == PlayerInfo::XSTUN_DISPLAY_STAGGER) {
					ptrNextSize = sprintf_s(ptrNext, ptrNextSizeCap, "%d/%d",
						player.cmnActIndex == CmnActJitabataLoop
							? player.stagger
							: 0,
						player.staggerMax);
				} else if (player.xStunDisplay == PlayerInfo::XSTUN_DISPLAY_STAGGER_WITH_SLOW) {
					ptrNextSize = sprintf_s(ptrNext, ptrNextSizeCap, "%d/%d",
						player.cmnActIndex == CmnActJitabataLoop
							? player.staggerWithSlow
							: 0,
						player.staggerMaxWithSlow);
				} else if (player.xStunDisplay == PlayerInfo::XSTUN_DISPLAY_HIT) {
					int currentHitstun = player.inHitstun
							? player.hitstun - (player.hitstop ? 1 : 0)
							: 0;
					if (player.hitstunMaxFloorbounceExtra) {
						ptrNextSize = sprintf_s(ptrNext, ptrNextSizeCap, "%d/(%d+%d)",
							currentHitstun,
							player.hitstunMax,
							player.hitstunMaxFloorbounceExtra);
					} else {
						ptrNextSize = sprintf_s(ptrNext, ptrNextSizeCap, "%d/%d",
							currentHitstun,
							player.hitstunMax);
					}
				} else if (player.xStunDisplay == PlayerInfo::XSTUN_DISPLAY_HIT_WITH_SLOW) {
					ptrNextSize = sprintf_s(ptrNext, ptrNextSizeCap, "%d/%d",
						player.hitstunWithSlow,
						player.hitstunMaxWithSlow);
				} else if (player.xStunDisplay == PlayerInfo::XSTUN_DISPLAY_BLOCK) {
					int currentBlockstun = player.blockstun - (player.hitstop ? 1 : 0);
					if (player.blockstunMaxLandExtra) {
						ptrNextSize = sprintf_s(ptrNext, ptrNextSizeCap, "%d/(%d+%d)%s",
							currentBlockstun,
							player.blockstunMax,
							player.blockstunMaxLandExtra,
							blockType);
					} else {
						ptrNextSize = sprintf_s(ptrNext, ptrNextSizeCap, "%d/%d%s",
							currentBlockstun,
							player.blockstunMax,
							blockType);
					}
				} else if (player.xStunDisplay == PlayerInfo::XSTUN_DISPLAY_BLOCK_WITH_SLOW) {
					ptrNextSize = sprintf_s(ptrNext, ptrNextSizeCap, "%d/%d%s",
						player.blockstunWithSlow,
						player.blockstunMaxWithSlow,
						blockType);
				}
				
				if (strbuf != ptrNext && ptrNextSize > 0 && ptrNextSizeCap) {
					memcpy(strbuf + strbufLen, " + ", 3);
					strbufLen += 3 + ptrNextSize;
				} else if (ptrNextSize > 0) {
					strbufLen = ptrNextSize;
				}
				printWithWordWrap
				if (playerBlocks[i].displayFloorbounceQuestionMark) {
					printExtraHitstunTooltip(playerBlocks[i].displayFloorbounceQuestionMarkValue);
				} else if (playerBlocks[i].displayBlockstunLandQuestionMark) {
					printExtraBlockstunTooltip(playerBlocks[i].displayBlockstunLandQuestionMarkValue);
				}
				
				if (i == 0) {
					ImGui::TableNextColumn();
					CenterAlignedText(searchFieldTitle("Hitstop+X-stun"));
					if (ImGui::BeginItemTooltip()) {
						ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
						bool needSeparator = false;
						for (int j = 0; j < 2; ++j) {
							if (playerBlocks[j].displayFloorbounceQuestionMark) {
								zerohspacing
								sprintf_s(strbuf, "Player %d: ", j + 1);
								ImGui::TextUnformatted(strbuf);
								ImGui::SameLine();
								printExtraHitstunText(playerBlocks[j].displayFloorbounceQuestionMarkValue);
								_zerohspacing
								needSeparator = true;
							} else if (playerBlocks[j].displayBlockstunLandQuestionMark) {
								zerohspacing
								sprintf_s(strbuf, "Player %d: ", j + 1);
								ImGui::TextUnformatted(strbuf);
								ImGui::SameLine();
								printExtraBlockstunText(playerBlocks[j].displayBlockstunLandQuestionMarkValue);
								_zerohspacing
								needSeparator = true;
							}
						}
						if (needSeparator) ImGui::Separator();
						ImGui::TextUnformatted(searchTooltip("Displays current hitstop/max hitstop + current hitstun or blockstun /"
							" max hitstun or blockstun. When there's no + sign, the displayed values could"
							" either be hitstop, or hitstun or blockstun, but if both are displayed, hitstop is always on the left,"
							" and the other are on the right.\n"
							"If you land while in blockstun from an air block, instead of your blockstun decrementing by 1, like it"
							" normally would each frame, on the landing frame you instead gain +3 blockstun. So your blockstun is"
							" slightly prolonged when transitioning from air blockstun to ground blockstun."));
						ImGui::PopTextWrapPos();
						ImGui::EndTooltip();
					}
				}
			}
			
			const bool dontUsePreBlockstunTime = settings.frameAdvantage_dontUsePreBlockstunTime;
			bool oneWillIncludeParentheses = false;
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				int frameAdvantage = dontUsePreBlockstunTime ? player.frameAdvantageNoPreBlockstun : player.frameAdvantage;
				int landingFrameAdvantage = dontUsePreBlockstunTime ? player.landingFrameAdvantageNoPreBlockstun : player.landingFrameAdvantage;
				if (player.frameAdvantageValid && player.landingFrameAdvantageValid
						&& frameAdvantage != landingFrameAdvantage) {
					oneWillIncludeParentheses = true;
					break;
				}
			}
			
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				
				ImGui::TableNextColumn();
				frameAdvantageControl(
					dontUsePreBlockstunTime ? player.frameAdvantageNoPreBlockstun : player.frameAdvantage,
					dontUsePreBlockstunTime ? player.landingFrameAdvantageNoPreBlockstun : player.landingFrameAdvantage,
					player.frameAdvantageValid,
					player.landingFrameAdvantageValid,
					i == 0);
				
				if (i == 0) {
					ImGui::TableNextColumn();
					headerThatCanBeClickedForTooltip(searchFieldTitle("Frame Adv."), &showFrameAdvTooltip, !oneWillIncludeParentheses);
					if (oneWillIncludeParentheses) {
						AddTooltip(
							searchTooltip("Value in () means frame advantage after landing.\n"
							"\n"
							"Click the field for tooltip."));
					}
				}
			}
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				player.printGaps(strbuf, sizeof strbuf);
				printWithWordWrap
				
				if (i == 0) {
					ImGui::TableNextColumn();
					CenterAlignedText(searchFieldTitle("Gaps"));
					AddTooltip(searchTooltip("Each gap is the number of frames from when the opponent left blockstun to when they entered blockstun again."));
				}
			}
			if (!settings.dontShowMoveName) {
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					ImGui::TableNextColumn();
					if (prevNamesControl(player, false, false)) {
						printWithWordWrap
						if (settings.useSlangNames) {
							if (ImGui::BeginItemTooltip()) {
								ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
								prevNamesControl(player, false, true);
								ImGui::TextUnformatted(strbuf);
								ImGui::PopTextWrapPos();
								ImGui::EndTooltip();
							}
						}
					}
					
					if (i == 0) {
						ImGui::TableNextColumn();
						CenterAlignedText(searchFieldTitle("Move"));
						static std::string moveTooltip;
						if (moveTooltip.empty()) {
							moveTooltip = settings.convertToUiDescription(
								"The last performed move, data of which is being displayed in the Startup/Active/Recovery/Total field."
								" If the 'Startup' or 'Total' field is showing multiplie numbers combined with + signs,"
								" all the moves that are included in those fields are listed here as well, combined with + signs or with *X appended to them,"
								" *X denoting how many times that move repeats.\n"
								"The move names might not match the names you may find when hovering your mouse over frames in the framebar to read their"
								" animation names, because the names here are only updated when a significant enough change in the animation happens.\n"
								"\n"
								"To hide this field you can use the \"dontShowMoveName\" setting. Then it will only be shown in the tooltip of 'Startup' and 'Total' fields.");
						}
						AddTooltip(searchTooltip(moveTooltip.c_str(), nullptr));
					}
				}
			}
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				sprintf_s(strbuf, "%d", player.hitstunElapsed);
				printNoWordWrap
				
				if (i == 0) {
					ImGui::TableNextColumn();
					CenterAlignedText("hitstunElapsed");
				}
			}
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				sprintf_s(strbuf, "%d", player.blockstunElapsed);
				printNoWordWrap
				
				if (i == 0) {
					ImGui::TableNextColumn();
					CenterAlignedText("blockstunElapsed");
				}
			}
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				sprintf_s(strbuf, "%d", player.staggerElapsed);
				printNoWordWrap
				
				if (i == 0) {
					ImGui::TableNextColumn();
					CenterAlignedText("staggerElapsed");
				}
			}
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				sprintf_s(strbuf, "%s", formatBoolean(player.idle));
				printNoWordWrap
				
				if (i == 0) {
					ImGui::TableNextColumn();
					CenterAlignedText("idle");
				}
			}
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				sprintf_s(strbuf, "%s", formatBoolean(player.canBlock));
				printNoWordWrap
				
				if (i == 0) {
					ImGui::TableNextColumn();
					CenterAlignedText("canBlock");
				}
			}
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				sprintf_s(strbuf, "%s", formatBoolean(player.canFaultlessDefense));
				printNoWordWrap
				
				if (i == 0) {
					ImGui::TableNextColumn();
					CenterAlignedText("canFaultlessDefense");
				}
			}
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				sprintf_s(strbuf, "%s (%d)", formatBoolean(player.idlePlus), player.timePassed);
				printNoWordWrap
				
				if (i == 0) {
					ImGui::TableNextColumn();
					CenterAlignedText("idlePlus");
				}
			}
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				sprintf_s(strbuf, "%s (%d)", formatBoolean(player.idleLanding), player.timePassedLanding);
				printNoWordWrap
				
				if (i == 0) {
					ImGui::TableNextColumn();
					CenterAlignedText("idleLanding");
				}
			}
			const bool useSlang = settings.useSlangNames;
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				const char* names[3] { nullptr };
				if (player.moveNonEmpty) {
					names[0] = useSlang ? player.move.getDisplayNameSlang(player) : player.move.getDisplayName(player);
				}
				if (player.moveName[0] != '\0') {
					names[1] = player.moveName;
				}
				if (player.anim[0] != '\0') {
					names[2] = player.anim;
				}
				bool isFirst = true;
				char* buf = strbuf;
				size_t bufSize = sizeof strbuf;
				int result;
				for (int j = 0; j < 3; ++j) {
					const char* name = names[j];
					if (!name) continue;
					
					bool alreadyIncluded = false;
					for (int k = 0; k < j; ++k) {
						if (names[k] && strcmp(names[k], name) == 0) {
							alreadyIncluded = true;
							break;
						}
					}
					
					if (alreadyIncluded) continue;
					
					if (!isFirst) {
						result = sprintf_s(buf, bufSize, " / ");
						advanceBuf
					}
					isFirst = false;
					result = sprintf_s(buf, bufSize, "%s", name);
					advanceBuf
				}
				printWithWordWrap
				
				if (i == 0) {
					ImGui::TableNextColumn();
					CenterAlignedText("anim");
				}
			}
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				sprintf_s(strbuf, "%d", player.animFrame);
				printNoWordWrap
				
				if (i == 0) {
					ImGui::TableNextColumn();
					CenterAlignedText("animFrame");
				}
			}
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				player.sprite.print(strbuf, sizeof strbuf);
				printNoWordWrap
				
				if (i == 0) {
					ImGui::TableNextColumn();
					CenterAlignedText("sprite");
				}
			}
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				sprintf_s(strbuf, "%d", player.startup);
				printNoWordWrap
				
				if (i == 0) {
					ImGui::TableNextColumn();
					CenterAlignedText("startup");
				}
			}
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				player.actives.print(strbuf, sizeof strbuf);
				printWithWordWrap
				
				if (i == 0) {
					ImGui::TableNextColumn();
					CenterAlignedText("active");
				}
			}
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				sprintf_s(strbuf, "%d", player.recovery);
				printNoWordWrap
				
				if (i == 0) {
					ImGui::TableNextColumn();
					CenterAlignedText("recovery");
				}
			}
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				sprintf_s(strbuf, "%d", player.total);
				printNoWordWrap
				
				if (i == 0) {
					ImGui::TableNextColumn();
					CenterAlignedText("total");
				}
			}
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				sprintf_s(strbuf, "%d", player.startupProj);
				printNoWordWrap
				
				if (i == 0) {
					ImGui::TableNextColumn();
					CenterAlignedText("startupProj");
				}
			}
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				player.activesProj.print(strbuf, sizeof strbuf);
				printWithWordWrap
				
				if (i == 0) {
					ImGui::TableNextColumn();
					CenterAlignedText("activesProj");
				}
			}
			Entity superflashInstigator = endScene.getLastNonZeroSuperflashInstigator();
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				int flashCurrent = 0;
				int flashMax = 0;
				int slowCurrent = player.rcSlowdownCounter;
				int slowMax = player.rcSlowdownMax;
				if (superflashInstigator == player.pawn) {
					flashCurrent = endScene.getSuperflashCounterAlliedCached();
					flashMax = endScene.getSuperflashCounterAlliedMax();
				} else if (superflashInstigator) {
					flashCurrent = endScene.getSuperflashCounterOpponentCached();
					flashMax = endScene.getSuperflashCounterOpponentMax();
				}
				if (flashCurrent + flashMax + slowCurrent + slowMax == 0) {
					strbuf[0] = '\0';
				} else {
					sprintf_s(strbuf, "%d/%d+%d/%d", flashCurrent, flashMax, slowCurrent, slowMax);
				}
				printNoWordWrap
				
				if (i == 0) {
					ImGui::TableNextColumn();
					CenterAlignedText(searchFieldTitle("Freeze+RC Slow"));
					AddTooltip(searchTooltip("Shows superfreeze current/maximum duration in frames, followed by +, followed by"
						" Roman Cancel slowdown duration current/maximum in frames."
						" Both the superfreeze and the Roman Cancel slowdown are always shown."
						" If either is not present at the moment, 0/0 or 0/X is shown in its place."
						" If a player is not affected by the superfreeze or Roman Cancel slowdown, 0/0 or 0/X is shown in the place of that."));
				}
			}
			ImGui::EndTable();
		}
	}
	popSearchStack();
	if (ImGui::CollapsingHeader(searchCollapsibleSection("Projectiles")) || searching) {
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
				for (int i = 0; i < two; ++i) {
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
				for (int i = 0; i < two; ++i) {
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
				for (int i = 0; i < two; ++i) {
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
				for (int i = 0; i < two; ++i) {
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
				for (int i = 0; i < two; ++i) {
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
				for (int i = 0; i < two; ++i) {
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
				for (int i = 0; i < two; ++i) {
					ImGui::TableNextColumn();
					if (row.side[i]) {
						ProjectileInfo& projectile = *row.side[i];
						sprintf_s(strbuf, "%d/%d", projectile.hitstopWithSlow, projectile.hitstopMaxWithSlow);
						printNoWordWrap
					}
					
					if (i == 0) {
						ImGui::TableNextColumn();
						CenterAlignedText("hitstop");
					}
				}
				for (int i = 0; i < two; ++i) {
					ImGui::TableNextColumn();
					if (row.side[i]) {
						ProjectileInfo& projectile = *row.side[i];
						printActiveWithMaxHit(projectile.actives, projectile.maxHit,
							!projectile.maxHit.empty() && projectile.maxHit.maxUse <= 1 ? 0 : projectile.hitOnFrame);
						
						printWithWordWrap
					}
					
					if (i == 0) {
						ImGui::TableNextColumn();
						CenterAlignedText("active");
					}
				}
				for (int i = 0; i < two; ++i) {
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
				for (int i = 0; i < two; ++i) {
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
				for (int i = 0; i < two; ++i) {
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
				for (int i = 0; i < two; ++i) {
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
				if (searching) break;
			}
			ImGui::EndTable();
		}
	}
	popSearchStack();
	if (ImGui::Button(searchFieldTitle("Show Tension Values"))) {
		showTensionData = !showTensionData;
	}
	AddTooltip(searchTooltip("Displays tension gained from combo and factors that affect tension gain."));
	ImGui::SameLine();
	if (ImGui::Button(searchFieldTitle("Burst Gain"))) {
		showBurstGain = !showBurstGain;
	}
	AddTooltip(searchTooltip("Displays burst gained from combo or last hit."));
	
	if (ImGui::Button(searchFieldTitle("Speed/Hitstun Proration/Pushback/Wakeup"))) {
		showSpeedsData = !showSpeedsData;
	}
	AddTooltip(searchTooltip("Display x,y, speed, pushback and protation of hitstun and pushback."));
	for (int i = 0; i < two; ++i) {
		ImGui::PushID(searchFieldTitle("Character Specific"));
		ImGui::PushID(i);
		sprintf_s(strbuf, i == 0 ? "Character Specific (P%d)" : "... (P%d)", i + 1);
		if (i != 0) ImGui::SameLine();
		if (ImGui::Button(strbuf)) {
			showCharSpecific[i] = !showCharSpecific[i];
		}
		AddTooltip(searchTooltip("Display of information specific to a character."));
		ImGui::PopID();
		ImGui::PopID();
	}
	if (ImGui::Button(searchFieldTitle("Box Extents"))) {
		showBoxExtents = !showBoxExtents;
	}
	AddTooltip(searchTooltip("Shows the minimum and maximum Y (vertical) extents of hurtboxes and hitboxes of each player."
		" The units are not divided by 100 for viewability."));
	
	ImGui::SameLine();
	for (int i = 0; i < two; ++i) {
		searchFieldTitle("Cancels");
		sprintf_s(strbuf, "Cancels (P%d)", i + 1);
		if (ImGui::Button(strbuf)) {
			showCancels[i] = !showCancels[i];
		}
		AddTooltip(searchTooltip(thisHelpTextWillRepeat));
		if (i == 0) ImGui::SameLine();
	}
	
	for (int i = 0; i < two; ++i) {
		ImGui::PushID(searchFieldTitle("Damage/RISC Calculation"));
		ImGui::PushID(i);
		sprintf_s(strbuf, i == 0 ? "Damage/RISC Calculation (P1)" : "... (P2)");
		if (ImGui::Button(strbuf)) {
			showDamageCalculation[i] = !showDamageCalculation[i];
		}
		AddTooltip(searchTooltip("For the defending player this shows damage and RISC calculation from the last hit and current combo proration."));
		ImGui::PopID();
		ImGui::PopID();
		if (i == 0) ImGui::SameLine();
	}
	
	for (int i = 0; i < two; ++i) {
		ImGui::PushID(searchFieldTitle("Stun/Stagger Mash"));
		ImGui::PushID(i);
		sprintf_s(strbuf, i == 0 ? "Stun/Stagger Mash (P1)" : "... (P2)");
		if (ImGui::Button(strbuf)) {
			showStunmash[i] = !showStunmash[i];
		}
		if (ImGui::BeginItemTooltip()) {
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			sprintf_s(strbuf, "The progress on your stun or stagger mash."
				" It might be too difficult to use this window in real-time, so please consider additionally using"
				" the Hitboxes - Freeze Game checkbox (Hotkey: %s) and the Next Frame button next to it (Hotkey: %s).",
				comborepr(settings.freezeGameToggle),
				comborepr(settings.allowNextFrameKeyCombo));
			ImGui::TextUnformatted(searchTooltip(strbuf));
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}
		ImGui::PopID();
		ImGui::PopID();
		if (i == 0) ImGui::SameLine();
	}
	
	if (ImGui::CollapsingHeader(searchCollapsibleSection("Hitboxes")) || searching) {
		
		booleanSettingPresetWithHotkey(settings.dontShowBoxes, settings.disableHitboxDisplayToggle);
		
		stateChanged = ImGui::Checkbox(searchFieldTitle("GIF Mode"), &gifModeOn) || stateChanged;
		ImGui::SameLine();
		static std::string GIFModeHelp;
		if (GIFModeHelp.empty()) {
			GIFModeHelp = settings.convertToUiDescription("GIF mode is:\n"
				"1) Background becomes black\n"
				"2) Camera is centered on you\n"
				"3) Opponent is invisible and invulnerable\n"
				"4) Hide HUD\n"
				"A hotkey can be configured for entering and leaving GIF Mode at \"gifModeToggle\".");
		}
		HelpMarkerWithHotkey(GIFModeHelp, settings.gifModeToggle);
		
		stateChanged = ImGui::Checkbox(searchFieldTitle("Black Background"), &gifModeToggleBackgroundOnly) || stateChanged;
		ImGui::SameLine();
		static std::string blackBackgroundHelp;
		if (blackBackgroundHelp.empty()) {
			blackBackgroundHelp = settings.convertToUiDescription(
				"Makes background black (and, for screenshotting purposes, - effectively transparent,"
				" if Post Effect is turned off in the game's graphics settings).\n"
				"You can use the \"gifModeToggleBackgroundOnly\" hotkey to toggle this setting.");
		}
		HelpMarkerWithHotkey(blackBackgroundHelp, settings.gifModeToggleBackgroundOnly);
		
		bool postEffectOn = game.postEffectOn() != 0;
		if (ImGui::Checkbox(searchFieldTitle("Post-Effect On"), &postEffectOn)) {
			game.postEffectOn() = (BOOL)postEffectOn;
		}
		ImGui::SameLine();
		static std::string postEffectOnHelp;
		if (postEffectOnHelp.empty()) {
			postEffectOnHelp = settings.convertToUiDescription("Toggles the game's 'Settings - Display Settings - Post-Effect'. Changing it this way does not"
			" require the current match to be restarted.\n"
			"Alternatively, you could tick the \"turnOffPostEffectWhenMakingBackgroundBlack\" checkbox,"
			" so that whenever you enter either the GIF mode or the GIF mode (black background only), the Post-Effect is"
			" turned off automatically, and when you leave those modes, it gets turned back on.\n"
			"Or, alternatively, you could use the manual keyboard toggle, set in this mod's \"togglePostEffectOnOff\".");
		}
		HelpMarkerWithHotkey(postEffectOnHelp, settings.togglePostEffectOnOff);
		
		stateChanged = ImGui::Checkbox(searchFieldTitle("Camera Center On Player"), &gifModeToggleCameraCenterOnly) || stateChanged;
		ImGui::SameLine();
		static std::string cameraCenterHelp;
		if (cameraCenterHelp.empty()) {
			cameraCenterHelp = settings.convertToUiDescription(
				"Centers the camera on you.\n"
				"You can use the \"gifModeToggleCameraCenterOnly\" hotkey to toggle this setting.");
		}
		HelpMarkerWithHotkey(cameraCenterHelp, settings.gifModeToggleCameraCenterOnly);
		
		stateChanged = ImGui::Checkbox(searchFieldTitle("Camera Center on Opponent"), &toggleCameraCenterOpponent) || stateChanged;
		ImGui::SameLine();
		static std::string cameraCenterOpponentHelp;
		if (cameraCenterOpponentHelp.empty()) {
			cameraCenterOpponentHelp = settings.convertToUiDescription(
				"Centers the camera on the opponent.\n"
				"You can use the \"toggleCameraCenterOpponent\" hotkey to toggle this setting.");
		}
		HelpMarkerWithHotkey(cameraCenterOpponentHelp, settings.toggleCameraCenterOpponent);
		
		stateChanged = ImGui::Checkbox(searchFieldTitle("Hide Opponent"), &gifModeToggleHideOpponentOnly) || stateChanged;
		ImGui::SameLine();
		static std::string hideOpponentHelp;
		if (hideOpponentHelp.empty()) {
			hideOpponentHelp = settings.convertToUiDescription(
				"Make the opponent invisible and invulnerable.\n"
				"You can use the \"gifModeToggleHideOpponentOnly\" hotkey to toggle this setting.");
		}
		HelpMarkerWithHotkey(hideOpponentHelp, settings.gifModeToggleHideOpponentOnly);
		
		stateChanged = ImGui::Checkbox(searchFieldTitle("Hide Player"), &toggleHidePlayer) || stateChanged;
		ImGui::SameLine();
		static std::string hidePlayerHelp;
		if (hidePlayerHelp.empty()) {
			hidePlayerHelp = settings.convertToUiDescription(
				"Make the player invisible and invulnerable.\n"
				"You can use the \"toggleHidePlayer\" hotkey to toggle this setting.");
		}
		HelpMarkerWithHotkey(hidePlayerHelp, settings.toggleHidePlayer);
		
		stateChanged = ImGui::Checkbox(searchFieldTitle("Hide HUD"), &gifModeToggleHudOnly) || stateChanged;
		ImGui::SameLine();
		static std::string hideHUDHelp;
		if (hideHUDHelp.empty()) {
			hideHUDHelp = settings.convertToUiDescription(
				"Hides the HUD (interface).\n"
				"You can use the \"gifModeToggleHudOnly\" hotkey to toggle this setting.");
		}
		HelpMarkerWithHotkey(hideHUDHelp, settings.gifModeToggleHudOnly);
		
		stateChanged = ImGui::Checkbox(searchFieldTitle("No Gravity"), &noGravityOn) || stateChanged;
		ImGui::SameLine();
		static std::string noGravityHelp;
		if (noGravityHelp.empty()) {
			noGravityHelp = settings.convertToUiDescription(
				"Prevents you from falling, meaning you remain in the air as long as 'No Gravity Mode' is enabled.\n"
				"You can use the \"noGravityToggle\" hotkey to toggle this setting.");
		}
		HelpMarkerWithHotkey(noGravityHelp, settings.noGravityToggle);
		
		bool neverDisplayGrayHurtboxes = settings.neverDisplayGrayHurtboxes;
		if (ImGui::Checkbox(searchFieldTitle("Disable Gray Hurtboxes"), &neverDisplayGrayHurtboxes)) {
			settings.neverDisplayGrayHurtboxes = neverDisplayGrayHurtboxes;
			needWriteSettings = true;
		}
		ImGui::SameLine();
		static std::string neverDisplayGrayHurtboxesHelp;
		if (neverDisplayGrayHurtboxesHelp.empty()) {
			neverDisplayGrayHurtboxesHelp = settings.convertToUiDescription(
				"Disables/enables the display of residual hurtboxes that appear on hit/block and show"
				" the defender's hurtbox at the moment of impact. These hurtboxes display for only a brief time on impacts but"
				" they can get in the way when trying to do certain stuff such as take screenshots of hurtboxes.\n"
				"You can use the \"toggleDisableGrayHurtboxes\" hotkey to toggle this setting.");
		}
		HelpMarkerWithHotkey(neverDisplayGrayHurtboxesHelp, settings.toggleDisableGrayHurtboxes);
		
		stateChanged = ImGui::Checkbox(searchFieldTitle("Freeze Game"), &freezeGame) || stateChanged;
		ImGui::SameLine();
		if (ImGui::Button(searchFieldTitle("Next Frame"))) {
			allowNextFrame = true;
			allowNextFrameTimer = 10;
		}
		ImGui::SameLine();
		static std::string freezeGameHelp;
		if (freezeGameHelp.empty()) {
			freezeGameHelp = settings.convertToUiDescription(
				"Freezes the current frame of the game and stops gameplay from advancing."
				" You can advance gameplay to the next frame using the 'Next Frame' button."
				" It is way more convenient to use this feature with the \"allowNextFrameKeyCombo\" shortcut"
				" instead of pressing the button, and freezing and unfreezing the game can be achieved with"
				" the \"freezeGameToggle\" shortcut.");
		}
		ImGui::TextDisabled("(?)");
		if (ImGui::BeginItemTooltip()) {
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			
			sprintf_s(strbuf, "Freeze Game Hotkey: %s", comborepr(settings.freezeGameToggle));
			ImGui::TextUnformatted(searchTooltip(strbuf, nullptr));
			
			sprintf_s(strbuf, "Allow Next Frame Hotkey: %s", comborepr(settings.allowNextFrameKeyCombo));
			ImGui::TextUnformatted(searchTooltip(strbuf, nullptr));
			
			ImGui::Separator();
			ImGui::TextUnformatted(searchTooltipStr(freezeGameHelp));
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}
		
		stateChanged = ImGui::Checkbox(searchFieldTitle("Slow-Mo Mode"), &slowmoGame) || stateChanged;
		ImGui::SameLine();
		int slowmoTimes = settings.slowmoTimes;
		ImGui::SetNextItemWidth(80.F);
		if (ImGui::InputInt(searchFieldTitle("Slow-Mo Factor"), &slowmoTimes, 1, 1, 0)) {
			if (slowmoTimes <= 0) {
				slowmoTimes = 1;
			}
			settings.slowmoTimes = slowmoTimes;
			needWriteSettings = true;
		}
		imguiActiveTemp = imguiActiveTemp || ImGui::IsItemActive();
		ImGui::SameLine();
		static std::string slowmoHelp;
		if (slowmoHelp.empty()) {
			slowmoHelp = settings.convertToUiDescription(
				"Makes the game run slower, advancing only on every second, every third and so on frame, depending on 'Slow-Mo Factor' field.\n"
				"You can use the \"slowmoGameToggle\" shortcut to toggle slow-mo on and off.");
		}
		HelpMarkerWithHotkey(slowmoHelp, settings.slowmoGameToggle);
		
		ImGui::Button(searchFieldTitle("Take Screenshot"));
		if (ImGui::IsItemActivated()) {
			// Regular ImGui button 'press' (ImGui::Button(...) function returning true) happens when you RELEASE the button,
			// but to simulate the old keyboard behavior we need this to happen when you PRESS the button
			takeScreenshotPress = true;
			takeScreenshotTimer = 10;
		}
		if (!searching) {
			takeScreenshotTemp = ImGui::IsItemActive();
		}
		ImGui::SameLine();
		static std::string screenshotHelp;
		if (screenshotHelp.empty()) {
			screenshotHelp = settings.convertToUiDescription(
				"Takes a screenshot. This only works during a match, so it won't work, for example, on character select screen or on some menu."
				" If you make background black using 'GIF Mode Enabled' and set Post Effect to off in the game's graphics settings"
				" (or use \"togglePostEffectOnOff\" or \"turnOffPostEffectWhenMakingBackgroundBlack\")"
				", you will be able to take screenshots with transparency. Screenshots are copied to clipboard by default, but if 'Screenshots path' is set"
				" in the 'Hitbox settings', they're saved there instead.\n"
				"A hotkey can be configured to take screenshots with, in \"screenshotBtn\".");
		}
		HelpMarkerWithHotkey(screenshotHelp, settings.screenshotBtn);
		
		stateChanged = ImGui::Checkbox(searchFieldTitle("Continuous Screenshotting Mode"), &continuousScreenshotToggle) || stateChanged;
		ImGui::SameLine();
		static std::string continuousScreenshottingHelp;
		if (continuousScreenshottingHelp.empty()) {
			continuousScreenshottingHelp = settings.convertToUiDescription(
				"When this option is enabled, screenshots will be taken every frame,"
				" as long as the mod's screenshot button is being help, unless the game is frozen, in which case"
				" a new screenshot is taken only when the frame advances. You can run out of disk space pretty fast with this and it slows"
				" the game down significantly. Continuous screenshotting is only allowed in training mode.\n"
				"Alternatively, you can use \"continuousScreenshotToggle\" shortcut to toggle a mode where you don't have to hold"
				" the screenshot button, and screenshots get taken every non frozen (advancing) frame automatically.");
		}
		HelpMarkerWithHotkey(continuousScreenshottingHelp, settings.continuousScreenshotToggle);
		
	}
	popSearchStack();
	if (ImGui::CollapsingHeader(searchCollapsibleSection("Settings")) || searching) {
		if (ImGui::CollapsingHeader(searchCollapsibleSection("Hitbox Settings")) || searching) {
			
			if (ImGui::Button(searchFieldTitle("Hitboxes Help"))) {
				showBoxesHelp = !showBoxesHelp;
			}
			
			booleanSettingPreset(settings.drawPushboxCheckSeparately);
			
			{
				std::unique_lock<std::mutex> screenshotGuard(settings.screenshotPathMutex);
				size_t newLen = settings.screenshotPath.size();
				if (newLen > MAX_PATH - 1) {
					newLen = MAX_PATH - 1;
				}
				memcpy(screenshotsPathBuf, settings.screenshotPath.c_str(), newLen);
				screenshotsPathBuf[newLen] = '\0';
			}
			
			ImGui::Text(searchFieldTitle(settings.getOtherUINameWithLength(&settings.screenshotPath)));
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
			
			booleanSettingPreset(settings.allowContinuousScreenshotting);
			
			booleanSettingPreset(settings.dontUseScreenshotTransparency);
			
			booleanSettingPreset(settings.useSimplePixelBlender);
			
			if (booleanSettingPreset(settings.turnOffPostEffectWhenMakingBackgroundBlack)) {
				endScene.onGifModeBlackBackgroundChanged();
			}
			
			booleanSettingPreset(settings.forceZeroPitchDuringCameraCentering);
			
			ImGui::PushItemWidth(120.F);
			float4SettingPreset(settings.cameraCenterOffsetY);
			
			float4SettingPreset(settings.cameraCenterOffsetY_WhenForcePitch0);
			
			float4SettingPreset(settings.cameraCenterOffsetZ);
			ImGui::PopItemWidth();
			
			if (ImGui::Button("Restore Defaults")) {
				settings.cameraCenterOffsetX = settings.cameraCenterOffsetX_defaultValue;
				settings.cameraCenterOffsetY = settings.cameraCenterOffsetY_defaultValue;
				settings.cameraCenterOffsetY_WhenForcePitch0 = settings.cameraCenterOffsetY_WhenForcePitch0_defaultValue;
				settings.cameraCenterOffsetZ = settings.cameraCenterOffsetZ_defaultValue;
				needWriteSettings = true;
			}
			AddTooltip(searchTooltip("Restores the default values for the four settings above."));
		}
		popSearchStack();
		if (ImGui::CollapsingHeader(searchCollapsibleSection("Framebar Settings")) || searching) {
			
			if (ImGui::Button(searchFieldTitle("Framebar Help"))) {
				showFramebarHelp = !showFramebarHelp;
			}
			AddTooltip(searchTooltip("Shows the meaning of each frame color/graphic on the framebar."));
			
			booleanSettingPresetWithHotkey(settings.neverIgnoreHitstop, settings.toggleNeverIgnoreHitstop);
			
			booleanSettingPreset(settings.ignoreHitstopForBlockingBaiken);
			
			booleanSettingPreset(settings.considerRunAndWalkNonIdle);
			
			booleanSettingPreset(settings.considerCrouchNonIdle);
			
			booleanSettingPreset(settings.considerKnockdownWakeupAndAirtechIdle);
			
			booleanSettingPreset(settings.considerIdleInvulIdle);
			
			booleanSettingPreset(settings.considerDummyPlaybackNonIdle);
			
			booleanSettingPreset(settings.useColorblindHelp);
			
			booleanSettingPresetWithHotkey(settings.showFramebar, settings.framebarVisibilityToggle);
			
			booleanSettingPreset(settings.showFramebarInTrainingMode);
			
			booleanSettingPreset(settings.showFramebarInReplayMode);
			
			booleanSettingPreset(settings.showFramebarInOtherModes);
			
			booleanSettingPreset(settings.closingModWindowAlsoHidesFramebar);
			
			booleanSettingPreset(settings.showStrikeInvulOnFramebar);
			
			booleanSettingPreset(settings.showThrowInvulOnFramebar);
			
			booleanSettingPreset(settings.showSuperArmorOnFramebar);
			
			booleanSettingPreset(settings.showFirstFramesOnFramebar);
			
			booleanSettingPreset(settings.considerSimilarFrameTypesSameForFrameCounts);
			
			booleanSettingPreset(settings.considerSimilarIdleFramesSameForFrameCounts);
			
			booleanSettingPreset(settings.skipGrabsInFramebar);
			
			if (booleanSettingPreset(settings.combineProjectileFramebarsWhenPossible)) {
				if (settings.combineProjectileFramebarsWhenPossible) {
					settings.eachProjectileOnSeparateFramebar = false;
				}
			}
			
			if (booleanSettingPreset(settings.eachProjectileOnSeparateFramebar)) {
				if (settings.eachProjectileOnSeparateFramebar) {
					settings.combineProjectileFramebarsWhenPossible = false;
				}
			}
			
			booleanSettingPreset(settings.dontClearFramebarOnStageReset);
			
			booleanSettingPreset(settings.dontTruncateFramebarTitles);
			
			booleanSettingPreset(settings.useSlangNames);
			
			booleanSettingPreset(settings.allFramebarTitlesDisplayToTheLeft);
			
			booleanSettingPreset(settings.showPlayerInFramebarTitle);
			
			intSettingPreset(settings.framebarTitleCharsMax, 0);
			
			intSettingPreset(settings.framebarHeight, 1);
			
		}
		popSearchStack();
		if (ImGui::CollapsingHeader(searchCollapsibleSection("Keyboard Shortcuts")) || searching) {
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
			keyComboControl(settings.toggleNeverIgnoreHitstop);
			keyComboControl(settings.toggleShowInputHistory);
		}
		popSearchStack();
		if (ImGui::CollapsingHeader(searchCollapsibleSection("General Settings")) || searching) {
			booleanSettingPreset(settings.modWindowVisibleOnStart);
			
			ImGui::PushID(1);
			booleanSettingPreset(settings.closingModWindowAlsoHidesFramebar);
			ImGui::PopID();
			
			booleanSettingPreset(settings.displayUIOnTopOfPauseMenu);
			
			booleanSettingPreset(settings.dodgeObsRecording);
			
			booleanSettingPreset(settings.frameAdvantage_dontUsePreBlockstunTime);
			
			ImGui::PushID(1);
			booleanSettingPreset(settings.useSlangNames);
			ImGui::PopID();
			
			booleanSettingPreset(settings.dontShowMoveName);
			
			booleanSettingPreset(settings.showComboProrationInRiscGauge);
			
			booleanSettingPresetWithHotkey(settings.displayInputHistoryWhenObserving, settings.toggleShowInputHistory);
			
			booleanSettingPresetWithHotkey(settings.displayInputHistoryInSomeOfflineModes, settings.toggleShowInputHistory);
			
			booleanSettingPreset(settings.dontShowMayInteractionChecks);
			
		}
		popSearchStack();
	}
	popSearchStack();
	if (!searching) {
		if (ImGui::Button("Search")) {
			showSearch = !showSearch;
		}
		AddTooltip("Searches the UI field names and tooltips for text.");
	}
	ImGui::End();
	popSearchStack();
	searchCollapsibleSection("Tension Data");
	if (showTensionData || searching) {
		if (searching) {
			ImGui::SetNextWindowPos({ 100000.F, 100000.F }, ImGuiCond_Always);
		}
		ImGui::Begin(searching ? "search_tension" : "Tension Data", &showTensionData, searching ? ImGuiWindowFlags_NoSavedSettings : 0);
		if (endScene.isIGiveUp() && !searching) {
			ImGui::TextUnformatted("Online non-observer match running.");
		} else
		if (ImGui::BeginTable("##TensionData",
					3,
					ImGuiTableFlags_Borders
					| ImGuiTableFlags_RowBg
					| ImGuiTableFlags_NoSavedSettings
					| ImGuiTableFlags_NoPadOuterX)
		) {
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
			ImGui::TextUnformatted(searchFieldTitle("Tension"));
			AddTooltip(searchTooltip("Meter"));
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				printDecimal(player.tension, 2, 0);
				ImGui::TextUnformatted(printdecimalbuf);
			}
			
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(searchFieldTitle("Tension Pulse"));
			AddTooltip(searchTooltip("Affects how fast you gain tension. Gained on IB, landing attacks, moving forward. Lost when moving back. Decays on its own slowly towards 0."
				" Tension Pulse Penalty and Corner Penalty may decrease Tension Pulse over time."));
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				sprintf_s(strbuf, "%-6d / %d", player.tensionPulse, player.tensionPulse < 0 ? -25000 : 25000);
				ImGui::TextUnformatted(strbuf);
			}
			
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(searchFieldTitle("Negative Penalty Active"));
			AddTooltip(searchTooltip("When Negative Penalty is active, you receive only 20% of the tension you would without it when attacking or moving."));
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(player.negativePenaltyTimer ? "Yes" : "No");
			}
			
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(searchFieldTitle("Negative Penalty Time Left"));
			AddTooltip(searchTooltip("Timer that counts down how much time is remaining until Negative Penalty wears off."));
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				printDecimal(player.negativePenaltyTimer * 100 / 60, 2, 0);
				sprintf_s(strbuf, "%s sec", printdecimalbuf);
				ImGui::TextUnformatted(strbuf);
			}
			
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(searchFieldTitle("Negative Penalty Buildup"));
			AddTooltip(searchTooltip("Tracks your progress towards reaching Negative Penalty. Negative Penalty is built up by Tension Pulse Penalty being red"
				" or Corner Penalty being red or by moving back."));
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				sprintf_s(strbuf, "%s / 100.00", printDecimal(player.negativePenalty, 2, -6));
				ImGui::TextUnformatted(strbuf);
			}
			
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(searchFieldTitle("Tension Pulse Penalty"));
			AddTooltip(searchTooltip("Reduces Tension Pulse (yellow) and at large enough values (red) increases Negative Penalty Buildup."
				" Increases constantly by 1. Gets reduced when getting hit, landing hits, getting your attack blocked or moving forward."));
			for (int i = 0; i < two; ++i) {
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
			ImGui::TextUnformatted(searchFieldTitle("Corner Penalty"));
			AddTooltip(searchTooltip("Penalty for being in touch with the screen or the wall. Reduces Tension Pulse (yellow) and increases Negative Penalty (red)."
				" Slowly decays when not in corner and gets reduced when getting hit."));
			for (int i = 0; i < two; ++i) {
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
			ImGui::TextUnformatted(searchFieldTitle("Negative Penalty Gain"));
			AddTooltip(searchTooltip("Negative Penalty Buildup gain modifier. Affects how fast Negative Penalty Buildup increases.\n"
				"Negative Penalty Gain Modifier = Distance-Based Modifier * Tension Pulse-Based Modifier.\n"
				"Distance-Based Modifier - depends on distance to the opponent.\n"
				"Tension Pulse-Based Modifier - depends on Tension Pulse."));
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				sprintf_s(strbuf, "%s * %d%c", printDecimal(player.tensionPulsePenaltyGainModifier_distanceModifier, 0, -3, true),
					player.tensionPulsePenaltyGainModifier_tensionPulseModifier, '%');
				ImGui::TextUnformatted(strbuf);
			}
			
			bool comboHappening = false;
			for (int i = 0; i < two; ++i) {
				if (endScene.players[i].inHitstun) {
					comboHappening = true;
					break;
				}
			}
			
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(searchFieldTitle("Tension Gain On Attack"));
			AddTooltip(searchTooltip("Affects how fast you gain Tension when performing attacks or combos.\n"
				"Tension Gain Modifier = Distance-Based Modifier * Negative Penalty Modifier * Tension Pulse-Based Modifier.\n"
				"Distance-Based Modifier - depends on distance to the opponent.\n"
				"Negative Penalty Modifier - if a Negative Penalty is active, the modifier is 20%, otherwise it's 100%.\n"
				"Tension Pulse-Based Modifier - depends on Tension Pulse.\n"
				"\n"
				"A fourth modifier may be displayed, which is an extra tension modifier. "
				"It may be present if you use Stylish mode or playing MOM mode. It will be highlighted in yellow.\n"
				"\n"
				"A fourth or fifth modifier may be displayed, which is a combo hit count-dependent modifier. "
				"It affects how fast you gain Tension from performing a combo.\n"
				"\n"
				"Tension gain on attack depends on 'tensionGainOnConnect' for each move. The standard values are:\n"
				"Throws: 40 tension gain on connect;\n"
				"Projectiles: 10;\n"
				"Non-projectile specials: 60\n"
				"Attack level 0 normal: 12\n"
				"Attack level 1 normal: 22\n"
				"Attack levels 2-4 normal: 32\n"
				"Overdrives: 0\n"
				"\nThese values get multiplied by 12 before use.\n"
				"\n"
				"If the opponent blocked or armored the move, the tension gain on attack is halved."));
			for (int i = 0; i < two; ++i) {
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
			ImGui::TextUnformatted(searchFieldTitle("Tension Gain On Defense"));
			AddTooltip(searchTooltip("Affects how fast you gain Tension when getting hit by attacks or combos.\n"
				"Tension Gain Modifier = Distance-Based Modifier * Negative Penalty Modifier * Tension Pulse-Based Modifier.\n"
				"Distance-Based Modifier - depends on distance to the opponent.\n"
				"Negative Penalty Modifier - if a Negative Penalty is active, the modifier is 20%, otherwise it's 100%.\n"
				"Tension Pulse-Based Modifier - depends on Tension Pulse.\n\n"
				"A fourth modifer may be displayed, which happens when you are getting combo'd. It affects how much tension you gain from getting hid by a combo"
				" and depends on the number of hits.\n"
				"\n"
				"Tension gain when being combo'd uses damage * 4 as the base. The damage taken is the base damage, before guts or combo proration scaling.\n"
				"When blocking, the tension gained is the damage / 3, round down, then * 3. The damage is the base damage, prior to chip damage calculation,"
				" and doesn't depend on guts."));
			for (int i = 0; i < two; ++i) {
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
			ImGui::TextUnformatted(searchFieldTitle("Tension Gain On Last Hit"));
			AddTooltip(searchTooltip("How much Tension was gained on a single last hit (either inflicting it or getting hit by it)."));
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				printDecimal(player.tensionGainOnLastHit, 2, 0);
				ImGui::TextUnformatted(printdecimalbuf);
			}
			
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(searchFieldTitle("Tension Gain Last Combo"));
			AddTooltip(searchTooltip("How much Tension was gained on the entire last performed combo (either inflicting it or getting hit by it)."));
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				printDecimal(player.tensionGainLastCombo, 2, 0);
				ImGui::TextUnformatted(printdecimalbuf);
			}
			
			float offsets[2];
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(searchFieldTitle("Tension Gain Max Combo"));
			AddTooltip(searchTooltip("The maximum amount of Tension that was gained on an entire performed combo during this training session"
				" (either inflicting it or getting hit by it).\n"
				"You can clear this value by pressing a button below this table."));
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				offsets[i] = ImGui::GetCursorPosX();
				printDecimal(player.tensionGainMaxCombo, 2, 0);
				ImGui::TextUnformatted(printdecimalbuf);
			}
			
			ImGui::EndTable();
			
			for (int i = 0; i < two; ++i) {
				ImGui::SetCursorPosX(offsets[i]);
				ImGui::PushID(i);
				if (ImGui::Button(searchFieldTitle("Clear Max Combo"))) {
					clearTensionGainMaxCombo[i] = true;
					clearTensionGainMaxComboTimer[i] = 10;
					stateChanged = true;
				}
				AddTooltip(searchTooltip("Clear max combo's Tension and Burst gain."));
				if (i == 0) ImGui::SameLine();
				ImGui::PopID();
			}
		}
		ImGui::End();
	}
	popSearchStack();
	searchCollapsibleSection("Burst Gain");
	if (showBurstGain || searching) {
		if (searching) {
			ImGui::SetNextWindowPos({ 100000.F, 100000.F }, ImGuiCond_Always);
		}
		ImGui::Begin(searching ? "search_burst" : "BurstGain", &showBurstGain, searching ? ImGuiWindowFlags_NoSavedSettings : 0);
		if (endScene.isIGiveUp() && !searching) {
			ImGui::TextUnformatted("Online non-observer match running.");
		} else
		if (ImGui::BeginTable("##BurstGain",
					3,
					ImGuiTableFlags_Borders
					| ImGuiTableFlags_RowBg
					| ImGuiTableFlags_NoSavedSettings
					| ImGuiTableFlags_NoPadOuterX)
		) {
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
			ImGui::TextUnformatted(searchFieldTitle("Burst Gain Modifier"));
			AddTooltip(searchFieldTitle("Burst gain depends on the current combo hit count. The more hits the combo has, the faster the defender gains burst.\n"
				" The amount gained from each hit is based on (damage * 3 + 100) * Burst Gain Modifier.\n"
				" There are two or three percentages displayed. The first depends on combo count (combo count + 32) / 32, the second depends on some unknown thing"
				" that scales burst gain by 20%. The third percentage is displayed when you're playing Stylish and is the stylish burst gain modifier."));
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				char* buf = strbuf;
				size_t bufSize = sizeof strbuf;
				int result = sprintf_s(strbuf, "%s", printDecimal(player.burstGainModifier, 0, -3, true));
				advanceBuf
				result = sprintf_s(buf, bufSize, " * %s", printDecimal(player.burstGainOnly20Percent ? 20 : 100, 0, -3, true));
				advanceBuf
				if (player.stylishBurstGainModifier != 100) {
					result = sprintf_s(buf, bufSize, " * %s", printDecimal(player.stylishBurstGainModifier, 0, -3, true));
					advanceBuf
				}
				sprintf_s(buf, bufSize, " = %s", printDecimal(
					player.burstGainModifier * 100
						* (player.burstGainOnly20Percent ? 20 : 100) / 100
						* player.stylishBurstGainModifier / 10000,
					0, 3, true));
				ImGui::TextUnformatted(strbuf);
			}
			
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(searchFieldTitle("Burst Gain On Last Hit"));
			AddTooltip(searchFieldTitle("How much Burst was gained on a single last hit (either inflicting it or getting hit by it)."));
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				printDecimal(player.burstGainOnLastHit, 2, 0);
				ImGui::TextUnformatted(printdecimalbuf);
			}
			
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(searchFieldTitle("Burst Gain Last Combo"));
			AddTooltip(searchTooltip("How much Burst was gained on the entire last performed combo (either inflicting it or getting hit by it)."));
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				printDecimal(player.burstGainLastCombo, 2, 0);
				ImGui::TextUnformatted(printdecimalbuf);
			}
			
			
			
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(searchFieldTitle("Burst Gain Max Combo"));
			AddTooltip(searchTooltip("The maximum amount of Burst that was gained on an entire performed combo during this training session"
				" (either inflicting it or getting hit by it).\n"
				"You can clear this value by pressing a button below this table."));
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				printDecimal(player.burstGainMaxCombo, 2, 0);
				ImGui::TextUnformatted(printdecimalbuf);
			}
			
			ImGui::EndTable();
		}
		ImGui::End();
	}
	popSearchStack();
	searchCollapsibleSection("Speed/Hitstun Proration/...");
	if (showSpeedsData || searching) {
		if (searching) {
			ImGui::SetNextWindowPos({ 100000.F, 100000.F }, ImGuiCond_Always);
		}
		ImGui::Begin(searching ? "search_speed" : "Speed/Hitstun Proration/...", &showSpeedsData, searching ? ImGuiWindowFlags_NoSavedSettings : 0);
		
		if (endScene.isIGiveUp() && !searching) {
			ImGui::TextUnformatted("Online non-observer match running.");
		} else
		if (ImGui::BeginTable("##SpeedHitstunProrationDotDotDot",
					3,
					ImGuiTableFlags_Borders
					| ImGuiTableFlags_RowBg
					| ImGuiTableFlags_NoSavedSettings
					| ImGuiTableFlags_NoPadOuterX)
		) {
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
			ImGui::TextUnformatted(searchFieldTitle("X; Y"));
			AddTooltip(searchTooltip("Position X; Y in the arena. Divided by 100 for viewability."));
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				printDecimal(player.x, 2, 0);
				sprintf_s(strbuf, "%s; ", printdecimalbuf);
				printDecimal(player.y, 2, 0);
				sprintf_s(strbuf + strlen(strbuf), sizeof strbuf - strlen(strbuf), "%s", printdecimalbuf);
				ImGui::TextUnformatted(strbuf);
			}
			
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(searchFieldTitle("Speed X; Y"));
			AddTooltip(searchTooltip("Speed X; Y in the arena. Divided by 100 for viewability."));
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				printDecimal(player.speedX, 2, 0);
				sprintf_s(strbuf, "%s; ", printdecimalbuf);
				printDecimal(player.speedY, 2, 0);
				sprintf_s(strbuf + strlen(strbuf), sizeof strbuf - strlen(strbuf), "%s", printdecimalbuf);
				ImGui::TextUnformatted(strbuf);
			}
			
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(searchFieldTitle("Gravity"));
			AddTooltip(searchTooltip("Gravity. Divided by 100 for viewability."));
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				printDecimal(player.gravity, 2, 0);
				ImGui::TextUnformatted(printdecimalbuf);
			}
			
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(searchFieldTitle("Combo Timer"));
			AddTooltip(searchTooltip("The time, in seconds, of the current combo's duration."));
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				printDecimal(player.comboTimer * 100 / 60, 2, 0);
				sprintf_s(strbuf, "%s sec (%df)", printdecimalbuf, player.comboTimer);
				ImGui::TextUnformatted(strbuf);
			}
			
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(searchFieldTitle("Pushback"));
			AddTooltip(searchTooltip("Pushback. Divided by 100 for viewability.\n"
				"Format: Current pushback / (Max pushback + FD pushback) (FD pushback base value, FD pushback percentage modifier%).\n"
				"If last hit was not FD'd, FD values are not displayed.\n"
				"When you attack an opponent, the opponent has this 'Pushback' value and you only get pushed back if they're in the corner (or close to it)."
				" If the opponent utilized FD, you will be pushed back regardless of whether the opponent is in the corner."
				" If they're in the corner and they also FD, the pushback from FD and the pushback from them being in the corner get added together."
				" The FD pushback base value and modifier % will be listed in (). The base value depends on distance between players. If it's less"
				" than 385000, the base is 900, otherwise it's 500. The modifier is 130% if the defender was touching the wall, otherwise 100%."
				" Multiply the base value by the modifier and 175 / 10, round down to get resulting pushback, divide by 100 for viewability."));
			for (int i = 0; i < two; ++i) {
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
			ImGui::TextUnformatted(searchFieldTitle("Base Pushback"));
			AddTooltip(searchTooltip("Base pushback on hit/block. Pushback = floor(Base pushback * 175 / 10) (divide by 100 for viewability).\n"
				"The base values of pushback are, per attack level:\n"
				"Ground block: 1250, 1375, 1500, 1750, 2000, 2400, 3000;\n"
				"Air block: 800,  850,  900,  950, 1000, 2400, 3000;\n"
				"Hit: 1300, 1400, 1500, 1750, 2000, 2400, 3000;\n"));
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				sprintf_s(strbuf, "%d", player.basePushback);
				ImGui::TextUnformatted(strbuf);
			}
			
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(searchFieldTitle("Pushback Modifier"));
			AddTooltip(searchTooltip("Pushback modifier. Equals Attack pushback modifier % * Attack pushback modifier on hit % * Combo timer modifier %"
				" * IB modifier %.\n"
				"Attack pushback modifier depends on the performed move."
				" Attack pushback modifier on hit depends on the performed move and should only be non-100 when the opponent is in hitstun."
				" Combo timer modifier depends on combo timer, in frames: >= 480 --> 200%, >= 420 --> 184%, >= 360 --> 166%, >= 300 --> 150%"
				", >= 240 --> 136%, >= 180 --> 124%, >= 120 --> 114%, >= 60 --> 106%."
				" IB modifier is non-100 on IB: air IB 10%, ground IB 90%."));
			for (int i = 0; i < two; ++i) {
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
			ImGui::TextUnformatted(searchFieldTitle("Received Speed Y"));
			AddTooltip(searchTooltip("This is updated only when a hit happens.\n"
				"Format: Base speed Y * (Weight % * Combo Proration % = Resulting Modifier %) = Resulting speed Y. Base and Final speeds divided by 100 for viewability."
				" On block, something more is going on, and it's not studied yet, so gained speed Y cannot be displayed."
				" Modifiers on received speed Y are weight and combo proration, displayed in that order.\n"
				"The combo proration depends on the number of hits so far at the moment of getting hit, not including the ongoing hit:\n"
				"6 hits so far -> 59/60 * 100% proration,\n"
				"5 hits -> 58 / 60 * 100% and so on, up to 30 / 60 * 100%. The rounding of the final speed Y is up.\n"
				"Some moves could theoretically ignore weight or combo proration, or both. When that happens, 100% will be displayed in place"
				" of the ignored parameter."));
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				if (!player.receivedSpeedYValid) {
					ImGui::TextUnformatted("???");
				} else {
					int mod = player.receivedSpeedYWeight * player.receivedSpeedYComboProration / 100;
					if (mod == 0) {
						printDecimal(0, 2, 0);
					} else {
						printDecimal(player.receivedSpeedY * 100 / mod, 2, 0);  // program crashed here
					}
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
			ImGui::TextUnformatted(searchFieldTitle("Hitstun Proration"));
			AddTooltip(searchTooltip("This is updated only when a hit happens."
				" Hitstun proration depends on the duration of an air or ground combo, in frames. For air combo:\n"
				">= 1080 --> 10%\n"
				">= 840  --> 60%\n"
				">= 600  --> 70%\n"
				">= 420  --> 80%\n"
				">= 300  --> 90%\n"
				">= 180  --> 95%\n"
				"For ground combo it's just:\n"
				">= 1080 --> 50%\n"
				"Multiply base hitstun by the percentage using floating point math, then round down to get prorated hitstun.\n"
				"Some attacks could theoretically ignore hitstun proration. When that happens, 100% is displayed."));
			for (int i = 0; i < two; ++i) {
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
			ImGui::TextUnformatted(searchFieldTitle("Wakeup"));
			AddTooltip(searchTooltip("Displays wakeup timing or time until able to act after air recovery (airtech)."
				" Format: Time remaining until able to act / Total wakeup or airtech time."));
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				sprintf_s(strbuf, "%2d/%2d", player.wakeupTiming ? player.wakeupTimingWithSlow : 0, player.wakeupTimingMaxWithSlow);
				ImGui::TextUnformatted(strbuf);
			}
			
			ImGui::EndTable();
		}
		ImGui::End();
	}
	popSearchStack();
	searchCollapsibleSection("Character Specific");
	for (int i = 0; i < two; ++i) {
		if (showCharSpecific[i] || searching) {
			ImGui::PushID(i);
			sprintf_s(strbuf, searching ? "search_char%d" : "  Character Specific (P%d)", i + 1);
			if (searching) {
				ImGui::SetNextWindowPos({ 100000.F, 100000.F }, ImGuiCond_Always);
			}
			ImGui::Begin(strbuf, showCharSpecific + i, searching ? ImGuiWindowFlags_NoSavedSettings : 0);
			PlayerInfo& player = endScene.players[i];
			
			GGIcon scaledIcon;
			if (player.charType == CHARACTER_TYPE_SOL && player.playerval0) {
				scaledIcon = scaleGGIconToHeight(DISolIconRectangular, 14.F);
			} else {
				scaledIcon = scaleGGIconToHeight(getPlayerCharIcon(i), 14.F);
			}
			drawPlayerIconInWindowTitle(scaledIcon);
			
			if (!*aswEngine || !player.pawn) {
				ImGui::TextUnformatted("Match not running");
			} else if (endScene.isIGiveUp() && !searching) {
				ImGui::TextUnformatted("Online non-observer match running.");
			} else
			if (player.charType == CHARACTER_TYPE_SOL) {
				if (player.playerval0) {
					GGIcon scaledIcon = scaleGGIconToHeight(DISolIcon, 14.F);
					drawGGIcon(scaledIcon);
					ImGui::SameLine();
					ImGui::TextUnformatted(searchFieldTitle("In Dragon Install"));
					searchFieldValue("Time remaining:");
					ImGui::Text("Time remaining: %d/%df", player.playerval1, player.maxDI);
				} else {
					GGIcon scaledIcon = scaleGGIconToHeight(getCharIcon(CHARACTER_TYPE_SOL), 14.F);
					drawGGIcon(scaledIcon);
					ImGui::SameLine();
					ImGui::TextUnformatted(searchFieldTitle("Not in Dragon Install"));
				}
				int timeRemainingMax = 0;
				int slowMax = 0;
				for (int j = 0; j < entityList.count; ++j) {
					Entity ent = entityList.list[j];
					if (ent.isActive() && ent.team() == i && strcmp(ent.animationName(), "GunFlameHibashira") == 0) {
						int timeRemaining = 0;
						
						if (player.gunflameParams.totalSpriteLengthUntilCreation == 0) {
							BYTE* func = ent.bbscrCurrentFunc();
							BYTE* foundPos = moves.findSpriteNonNull(func);
							if (!foundPos) break;
							bool created = false;
							Moves::InstructionType lastType = Moves::instr_sprite;
							int lastSpriteLength = 0;
							for (BYTE* instr = foundPos; lastType != Moves::instr_endState; ) {
								if (lastType == Moves::instr_sprite) {
									lastSpriteLength = *(int*)(instr + 4 + 32);
									if (!created) {
										player.gunflameParams.totalSpriteLengthUntilCreation += lastSpriteLength;
									}
									player.gunflameParams.totalSpriteLengthUntilReenabled += lastSpriteLength;
								} else if (lastType == Moves::instr_createObjectWithArg) {
									if (!created) {
										player.gunflameParams.totalSpriteLengthUntilCreation =
											player.gunflameParams.totalSpriteLengthUntilCreation - lastSpriteLength + 1;
										created = true;
									}
								} else if (lastType == Moves::instr_deleteMoveForceDisableFlag) {
									player.gunflameParams.totalSpriteLengthUntilReenabled =
										player.gunflameParams.totalSpriteLengthUntilReenabled - lastSpriteLength + 1;
									break;
								}
								instr = moves.skipInstruction(instr);
								lastType = moves.instructionType(instr);
							}
						}
						if (player.gunflameParams.totalSpriteLengthUntilCreation == 0) break;
						
						bool canCreateNewOne = ent.createArgHikitsukiVal1() <= 3
							&& (int)ent.currentAnimDuration() < player.gunflameParams.totalSpriteLengthUntilCreation
							&& !ent.mem46();
						if (canCreateNewOne) {
							timeRemaining = player.gunflameParams.totalSpriteLengthUntilCreation - 1 - ent.currentAnimDuration()
								+ (3 - ent.createArgHikitsukiVal1()) * (player.gunflameParams.totalSpriteLengthUntilCreation - 1)
								+ player.gunflameParams.totalSpriteLengthUntilReenabled + 1;
						} else {
							timeRemaining = player.gunflameParams.totalSpriteLengthUntilReenabled - ent.currentAnimDuration() + 1;
						}
						if (timeRemaining > timeRemainingMax) timeRemainingMax = timeRemaining;
						
						ProjectileInfo& projectile = endScene.findProjectile(ent);
						if (projectile.ptr) {
							if (projectile.rcSlowedDownCounter > slowMax) {
								slowMax = projectile.rcSlowedDownCounter;
							}
						}
					}
				}
				textUnformattedColored(YELLOW_COLOR, "Time until can do another gunflame: ");
				int unused;
				PlayerInfo::calculateSlow(0, timeRemainingMax, slowMax, &timeRemainingMax, &unused, &unused);
				sprintf_s(strbuf, "%df or until hits/gets blocked/gets erased", timeRemainingMax);
				ImGui::TextUnformatted(strbuf);
			} else if (player.charType == CHARACTER_TYPE_KY) {
				if (!endScene.interRoundValueStorage2Offset) {
					ImGui::TextUnformatted("Error");
				} else {
					DWORD& theValue1 = *(DWORD*)(*aswEngine + endScene.interRoundValueStorage1Offset + i * 4);
					DWORD& theValue2 = *(DWORD*)(*aswEngine + endScene.interRoundValueStorage2Offset + i * 4);
					if (theValue2) {
						ImGui::TextUnformatted(searchFieldTitle("Hair down"));
					} else {
						ImGui::TextUnformatted(searchFieldTitle("Hair not down"));
					}
					if (game.isTrainingMode() && ImGui::Button(searchFieldTitle("Toggle Hair Down"))) {
						theValue2 = 1 - theValue2;
						endScene.BBScr_callSubroutine((void*)player.pawn.ent, "PonyMeshSetCheck");
					}
					if (game.isTrainingMode() && ImGui::Button(
							theValue1 >= 6
								? searchFieldTitle("Allow Hair Falling Down")
								: searchFieldTitle("Prevent Hair Falling Down"))) {
						if (theValue1 >= 6) {
							theValue1 = 0;
						} else {
							theValue1 = 100;
						}
					}
					AddTooltip(searchTooltip("Hair falls down on the 6'th knockdown if on it HP is >= 252."));
				}
				zerohspacing
				textUnformattedColored(YELLOW_COLOR, searchFieldTitle("Time until can do another stun edge: "));
				ImGui::SameLine();
				bool hasForceDisableFlag1 = (player.wasForceDisableFlags & 0x1) != 0;
				if (!hasForceDisableFlag1) {
					ImGui::TextUnformatted("0f");
				} else {
					bool hasStunEdge = false;
					bool hasChargedStunEdge = false;
					bool hasSPChargedStunEdge = false;
					int stunEdgeDeleteFrame = -1;
					int stunEdgeDeleteSlow = 0;
					int spChargedStunEdgeKowareFrame = -1;
					int spChargedStunEdgeKowareFrameMax = -1;
					int spChargedStunEdgeKowareSlow = 0;
					for (int j = 0; j < entityList.count; ++j) {
						Entity p = entityList.list[j];
						if (p.isActive() && p.team() == i && !p.isPawn()) {
							if (strcmp(p.animationName(), "StunEdgeObj") == 0) {
								hasStunEdge = true;
								if (moves.stunEdgeDeleteSpriteSum == 0) {
									BYTE* funcStart = p.findFunctionStart("StunEdgeDelete");
									if (funcStart) {
										for (BYTE* instr = moves.skipInstruction(funcStart);
												moves.instructionType(instr) != Moves::instr_endState;
												instr = moves.skipInstruction(instr)) {
											if (moves.instructionType(instr) == Moves::instr_sprite) {
												moves.stunEdgeDeleteSpriteSum += *(int*)(instr + 4 + 32);
											}
										}
									}
								}
							} else if (strcmp(p.animationName(), "ChargedStunEdgeObj") == 0) {
								hasChargedStunEdge = true;
							} else if (strcmp(p.animationName(), "SPChargedStunEdgeObj") == 0) {
								if (moves.instructionType(p.bbscrCurrentInstr()) == Moves::instr_endState) {
									spChargedStunEdgeKowareFrame = p.spriteFrameCounter();
									spChargedStunEdgeKowareFrameMax = p.spriteFrameCounterMax();
									ProjectileInfo& projectile = endScene.findProjectile(p);
									if (projectile.ptr) spChargedStunEdgeKowareSlow = projectile.rcSlowedDownCounter;
								} else {
									hasSPChargedStunEdge = true;
									if (moves.spChargedStunEdgeKowareSpriteDuration == 0) {
										BYTE* lastSprite = nullptr;
										BYTE* instr = p.bbscrCurrentInstr();
										do {
											if (moves.instructionType(instr) == Moves::instr_sprite) {
												lastSprite = instr;
											}
											instr = moves.skipInstruction(instr);
										} while (moves.instructionType(instr) != Moves::instr_endState);
										if (lastSprite) {
											moves.spChargedStunEdgeKowareSpriteDuration = *(int*)(lastSprite + 4 + 32);
										}
									}
								}
							} else if (strcmp(p.animationName(), "StunEdgeDelete") == 0) {
								stunEdgeDeleteFrame = p.currentAnimDuration();
								ProjectileInfo& projectile = endScene.findProjectile(p);
								if (projectile.ptr) stunEdgeDeleteSlow = projectile.rcSlowedDownCounter;
							}
						}
					}
					
					int waitTime = 0;
					int unused;
					if (hasStunEdge) {
						sprintf_s(strbuf, "???+%df", moves.stunEdgeDeleteSpriteSum + 1);
						ImGui::TextUnformatted(strbuf);
					} else if (hasChargedStunEdge) {
						ImGui::TextUnformatted("???f");
					} else if (hasSPChargedStunEdge) {
						sprintf_s(strbuf, "???+%df", moves.spChargedStunEdgeKowareSpriteDuration + 1);
						ImGui::TextUnformatted(strbuf);
					} else if (stunEdgeDeleteFrame != -1) {
						PlayerInfo::calculateSlow(stunEdgeDeleteFrame,
							moves.stunEdgeDeleteSpriteSum - stunEdgeDeleteFrame + 1,
							stunEdgeDeleteSlow,
							&waitTime,
							&unused,
							&unused);
						++waitTime;
					} else if (spChargedStunEdgeKowareFrame != -1) {
						PlayerInfo::calculateSlow(spChargedStunEdgeKowareFrame + 1,
							spChargedStunEdgeKowareFrameMax - spChargedStunEdgeKowareFrame,
							spChargedStunEdgeKowareSlow,
							&waitTime,
							&unused,
							&unused);
						++waitTime;
					} else {
						// nothing found? Projectile is probably already destroyed. But force disable flags will only apply on the next frame
						ImGui::TextUnformatted("1f");
					}
					if (waitTime) {
						sprintf_s(strbuf, "%df", waitTime);
						ImGui::TextUnformatted(strbuf);
					}
				}
				_zerohspacing
				
				textUnformattedColored(YELLOW_COLOR, searchFieldTitle("Has used j.D:"));
				const char* tooltip = searchTooltip("You can only use j.D once in the air. j.D gets reenabled when you land.");
				AddTooltip(tooltip);
				ImGui::SameLine();
				bool hasForceDisableFlag2 = (player.wasForceDisableFlags & 0x2) != 0;
				ImGui::TextUnformatted(hasForceDisableFlag2 ? "Yes" : "No");
				AddTooltip(tooltip);
			} else if (player.charType == CHARACTER_TYPE_MAY) {
				bool hasForceDisableFlag2 = (player.wasForceDisableFlags & 0x2) != 0;
				if (!hasForceDisableFlag2) {
					textUnformattedColored(YELLOW_COLOR, "Time until can do Beach Ball:");
					ImGui::SameLine();
					ImGui::TextUnformatted("0f");
				} else {
					bool foundBeachBall = false;
					for (int j = 0; j < entityList.count; ++j) {
						Entity p = entityList.list[j];
						if (p.isActive() && p.team() == i && !p.isPawn()
								&& (
									strcmp(p.animationName(), "MayBallA") == 0
									|| strcmp(p.animationName(), "MayBallB") == 0
								)) {
							foundBeachBall = true;
							textUnformattedColored(YELLOW_COLOR, "Beach Ball bounces left:");
							ImGui::SameLine();
							sprintf_s(strbuf, "%d/3", 3 - p.mem45());
							ImGui::TextUnformatted(strbuf);
							break;
						}
					}
					if (!foundBeachBall) {
						// Nothing found, but still can't do a Beach Ball on this frame?
						// Probably, a Ball has just been deleted, and the lack of its forceDisableFlags will only take effect on the next frame...
						textUnformattedColored(YELLOW_COLOR, "Time until can do Beach Ball:");
						ImGui::SameLine();
						ImGui::TextUnformatted("1f");
					}
				}
				
				booleanSettingPreset(settings.dontShowMayInteractionChecks);
				
			} else if (player.charType == CHARACTER_TYPE_ZATO) {
				Entity eddie = nullptr;
				bool isSummoned = player.pawn.playerVal(0);
				if (isSummoned) {
					eddie = endScene.getReferredEntity((void*)player.pawn.ent, 4);
				}
				
				ImGui::TextUnformatted(searchFieldTitle("Eddie Values"));
				sprintf_s(strbuf, "##Zato_P%d", i);
				if (ImGui::BeginTable(strbuf, 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_NoPadOuterX)) {
					ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0.5f);
					ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.5f);
					
					ImGui::TableHeadersRow();
					
					ImGui::TableNextColumn();
					ImGui::Text(searchFieldTitle("Eddie Gauge"));
					AddTooltip(searchTooltip("Divided by 10 for readability."));
					ImGui::TableNextColumn();
					ImGui::Text("%-3d/%d", player.pawn.exGaugeValue(0) / 10, player.pawn.exGaugeMaxValue(0) / 10);
					
					ImGui::TableNextColumn();
					ImGui::TextUnformatted(searchFieldTitle("Is Summoned"));
					ImGui::TableNextColumn();
					if (!isSummoned) {
						ImGui::TextUnformatted("No");
					} else {
						ImGui::TextUnformatted("Yes");
					}
					
					ImGui::TableNextColumn();
					ImGui::TextUnformatted(searchFieldTitle("Shadow Puddle X"));
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
					ImGui::TextUnformatted(searchFieldTitle("Startup"));
					ImGui::TableNextColumn();
					ImGui::Text("%d", player.eddie.startup);
					
					ImGui::TableNextColumn();
					ImGui::TextUnformatted(searchFieldTitle("Active"));
					ImGui::TableNextColumn();
					printActiveWithMaxHit(player.eddie.actives, player.eddie.maxHit, player.eddie.hitOnFrame);
					float w = ImGui::CalcTextSize(strbuf).x;
					if (w > ImGui::GetContentRegionAvail().x) {
						ImGui::TextWrapped("%s", strbuf);
					} else {
						ImGui::TextUnformatted(strbuf);
					}
					
					ImGui::TableNextColumn();
					ImGui::TextUnformatted(searchFieldTitle("Recovery"));
					ImGui::TableNextColumn();
					ImGui::Text("%d", player.eddie.recovery);
					
					ImGui::TableNextColumn();
					ImGui::TextUnformatted(searchFieldTitle("Total"));
					ImGui::TableNextColumn();
					ImGui::Text("%d", player.eddie.total);
					
					ImGui::TableNextColumn();
					ImGui::TextUnformatted(searchFieldTitle("Frame Adv."));
					ImGui::TableNextColumn();
					frameAdvantageControl(
						player.eddie.frameAdvantage,
						player.eddie.landingFrameAdvantage,
						player.eddie.frameAdvantageValid,
						player.eddie.landingFrameAdvantageValid,
						false);
					
					ImGui::TableNextColumn();
					ImGui::TextUnformatted(searchFieldTitle("Hitstop"));
					ImGui::TableNextColumn();
					ImGui::Text("%d/%d", player.eddie.hitstop, player.eddie.hitstopMax);
					
					ImGui::TableNextColumn();
					ImGui::TextUnformatted(searchFieldTitle("Last Consumed Eddie Gauge Amount"));
					AddTooltip(searchTooltip("Divided by 10 for readability. The amount of consumed Eddie Gauge of the last attack."));
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
					ImGui::TextUnformatted(searchFieldTitle("Anim"));
					ImGui::TableNextColumn();
					if (eddie) {
						ImGui::TextUnformatted(eddie.animationName());
					}
					
					ImGui::TableNextColumn();
					ImGui::TextUnformatted(searchFieldTitle("Frame"));
					ImGui::TableNextColumn();
					if (eddie) {
						ImGui::Text("%d", eddie.currentAnimDuration());
					}
					
					ImGui::TableNextColumn();
					ImGui::TextUnformatted(searchFieldTitle("Sprite"));
					ImGui::TableNextColumn();
					if (eddie) {
						ImGui::Text("%s (%d/%d)", eddie.spriteName(), eddie.spriteFrameCounter(), eddie.spriteFrameCounterMax());
					}
					
					ImGui::EndTable();
				}
			} else if (player.charType == CHARACTER_TYPE_CHIPP) {
				if (player.playerval0) {
					printChippInvisibility(player.playerval0, player.maxDI);
				} else {
					ImGui::TextUnformatted(searchFieldTitle("Not invisible"));
				}
				if (player.move.caresAboutWall) {
					searchFieldTitle("Wall time:");
					ImGui::Text("Wall time: %d/120", player.pawn.mem54());
				} else {
					ImGui::TextUnformatted(searchFieldTitle("Not on a wall."));
				}
			} else if (player.charType == CHARACTER_TYPE_FAUST) {
				const PlayerInfo& otherPlayer = endScene.players[1 - player.index];
				if (!otherPlayer.poisonDuration) {
					ImGui::TextUnformatted(searchFieldTitle("Opponent not poisoned."));
				} else {
					sprintf_s(strbuf, "Poison duration on opponent: %d/%d", otherPlayer.poisonDuration, otherPlayer.poisonDurationMax);
					ImGui::TextUnformatted(searchFieldTitle(strbuf, nullptr));
				}
			} else {
				ImGui::TextUnformatted(searchFieldTitle("No character specific information to show."));
			}
			ImGui::End();
			ImGui::PopID();
		}
	}
	popSearchStack();
	searchCollapsibleSection("Box Extents");
	if (showBoxExtents || searching) {
		if (searching) {
			ImGui::SetNextWindowPos({ 100000.F, 100000.F }, ImGuiCond_Always);
		}
		ImGui::Begin(searching ? "search_box" : "Box Extents", &showBoxExtents, searching ? ImGuiWindowFlags_NoSavedSettings : 0);
		if (endScene.isIGiveUp() && !searching) {
			ImGui::TextUnformatted("Online non-observer match running.");
		} else
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
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("P2");
			ImGui::SameLine();
			drawPlayerIconWithTooltip(1);
			
			for (int i = 0; i < two; ++i) {
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
					CenterAlignedText(searchFieldTitle("Hurtbox Y"));
					AddTooltip(searchTooltip("These values display either the current or last valid value and change each frame."
						" They do not show the total combined bounding box."
						" To view these values for each frame more easily you could use the frame freeze mode,"
						" available in the Hitboxes section.\n"
						"The coordinates shown are relative to the global space."));
				}
			}
			for (int i = 0; i < two; ++i) {
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
					CenterAlignedText(searchFieldTitle("Hitbox Y"));
					AddTooltip(searchTooltip("These values display either the current or last valid value and change each frame."
						" They do not show the total combined bounding box."
						" To view these values for each frame more easily you could use the frame freeze mode,"
						" available in the Hitboxes section.\n"
						"The coordinates shown are relative to the global space.\n"
						"If the coordinates are not shown while an attack is out, that means that attack is a projectile."
						" To view projectiles' hitbox extents you can see 'Projectiles' in the main UI window."));
				}
			}
			ImGui::EndTable();
		}
		ImGui::End();
	}
	popSearchStack();
	searchCollapsibleSection("Cancels");
	for (int i = 0; i < two; ++i) {
		if (showCancels[i] || searching) {
			ImGui::PushID(i);
			sprintf_s(strbuf, searching ? "search_cancels%d" : "  Cancels (P%d)", i + 1);
			ImGui::SetNextWindowSize({
				ImGui::GetFontSize() * 35.F,
				150.F
			}, ImGuiCond_FirstUseEver);
			if (searching) {
				ImGui::SetNextWindowPos({ 100000.F, 100000.F }, ImGuiCond_Always);
			}
			ImGui::Begin(strbuf, showCancels + i, searching ? ImGuiWindowFlags_NoSavedSettings : 0);
			drawPlayerIconInWindowTitle(i);
			
			const float wrapWidth = ImGui::GetContentRegionAvail().x;
			ImGui::PushTextWrapPos(wrapWidth);
			
			const PlayerInfo& player = endScene.players[i];
			
			const bool useSlang = settings.useSlangNames;
			const char* lastNames[2];
			prepareLastNames(lastNames, player, false);
			int animNamesCount = player.prevStartupsDisp.countOfNonEmptyUniqueNames(lastNames,
				lastNames[1] ? 2 : 1,
				useSlang);
			ImGui::PushStyleVarX(ImGuiStyleVar_ItemSpacing, 0.F);
			textUnformattedColored(YELLOW_COLOR, searchFieldTitle(animNamesCount ? "Anims: " : "Anim: ", nullptr));
			char* buf = strbuf;
			size_t bufSize = sizeof strbuf;
			player.prevStartupsDisp.printNames(buf, bufSize, lastNames,
				player.superfreezeStartup ? 2 : 1,
				useSlang,
				false,
				animNamesCount > 1);
			drawOneLineOnCurrentLineAndTheRestBelow(wrapWidth, strbuf);
			ImGui::PopStyleVar();
			
			if (player.cancelsOverflow) {
				ImGui::TextUnformatted("...Some initial frames skipped...");
				ImGui::Separator();
			}
			bool printedSomething = false;
			for (int i = 0; i < player.cancelsCount; ++i) {
				const PlayerCancelInfo& cancels = player.cancels[i];
				if (cancels.isCompletelyEmpty()) continue;
				if (printedSomething) {
					ImGui::Separator();
				}
				printedSomething = true;
				sprintf_s(strbuf, "Frames %d-%d:", cancels.start, cancels.end);
				textUnformattedColored(LIGHT_BLUE_COLOR, strbuf);
				printAllCancels(cancels.cancels,
					cancels.enableSpecialCancel,
					cancels.enableJumpCancel,
					cancels.enableSpecials,
					cancels.hitAlreadyHappened,
					cancels.airborne,
					false);
			}
			if (!printedSomething) {
				ImGui::TextUnformatted("No cancels available.");
			}
			
			ImGui::PopTextWrapPos();
			GGIcon scaledIcon = scaleGGIconToHeight(tipsIcon, 14.F);
			drawGGIcon(scaledIcon);
			AddTooltip(thisHelpTextWillRepeat);
			ImGui::End();
			ImGui::PopID();
		}
	}
	popSearchStack();
	searchCollapsibleSection("Damage/RISC Calculation");
	for (int i = 0; i < two; ++i) {
		if (showDamageCalculation[i] || searching) {
			ImGui::PushID(i);
			sprintf_s(strbuf, searching ? "search_damage" : "  Damage/RISC Calculation (P%d)", i + 1);
			ImGui::SetNextWindowSize({
				ImGui::GetFontSize() * 35.F,
				150.F
			}, ImGuiCond_FirstUseEver);
			if (searching) {
				ImGui::SetNextWindowPos({ 100000.F, 100000.F }, ImGuiCond_Always);
			}
			ImGui::Begin(strbuf, showDamageCalculation + i, searching ? ImGuiWindowFlags_NoSavedSettings : 0);
			drawPlayerIconInWindowTitle(i);
			
			const PlayerInfo& player = endScene.players[i];
			
			struct ComboProration {
				const char* name;
				int val;
			};
			ComboProration comboProrations[] {
				{ "Normal/Special", player.comboProrationNormal },
				{ "Overdrive", player.comboProrationOverdrive }
			};
			
			ImGui::PushStyleVarX(ImGuiStyleVar_ItemSpacing, 0.F);
			for (int comboProrationI = 0; comboProrationI < 2; ++comboProrationI) {
				const ComboProration& currentProration = comboProrations[comboProrationI];
				
				searchFieldTitle("Current proration");
				sprintf_s(strbuf, "Current proration (%s): ", currentProration.name);
				textUnformattedColored(YELLOW_COLOR, strbuf);
				AddTooltip(searchTooltip("Here we display only Dust proration * Proration from initial/forced proration * RISC Damage Scale * Guts * Defense Modifier * RC Proration"));
				
				int totalProration = 10000;
				totalProration = totalProration * player.dustProration1 / 100;
				totalProration = totalProration * player.dustProration2 / 100;
				totalProration = totalProration * player.proration / 100;
				totalProration = totalProration * currentProration.val / 256;
				totalProration = totalProration * player.gutsPercentage / 100;
				totalProration = totalProration * (player.defenseModifier + 256) / 256;
				if (player.rcProration) totalProration = totalProration * 80 / 100;
				
				sprintf_s(strbuf, "%3d%c", player.dustProration1 * player.dustProration2 / 100, '%');
				ImGui::TextUnformatted(strbuf);
				AddTooltip(searchTooltip("Dust proration"));
				ImGui::SameLine();
				
				ImGui::TextUnformatted(" * ");
				ImGui::SameLine();
				
				sprintf_s(strbuf, "%3d%c", player.proration, '%');
				ImGui::TextUnformatted(strbuf);
				AddTooltip(searchTooltip("Initial/forced proration"));
				ImGui::SameLine();
				
				ImGui::TextUnformatted(" * ");
				ImGui::SameLine();
				
				sprintf_s(strbuf, "%3d%c", currentProration.val * 100 / 256, '%');
				ImGui::TextUnformatted(strbuf);
				AddTooltip(searchTooltip("RISC Damage Scale"));
				ImGui::SameLine();
				
				ImGui::TextUnformatted(" * ");
				ImGui::SameLine();
				
				sprintf_s(strbuf, "%3d%c", player.gutsPercentage, '%');
				ImGui::TextUnformatted(strbuf);
				AddTooltip(searchTooltip("Guts"));
				ImGui::SameLine();
				
				ImGui::TextUnformatted(" * ");
				ImGui::SameLine();
				
				sprintf_s(strbuf, "%3d%c", (player.defenseModifier + 256) * 100 / 256, '%');
				ImGui::TextUnformatted(strbuf);
				AddTooltip(searchTooltip("Defense Modifier"));
				ImGui::SameLine();
				
				ImGui::TextUnformatted(" * ");
				ImGui::SameLine();
				
				ImGui::TextUnformatted(player.rcProration ? " 80%" : "100%");
				AddTooltip(searchTooltip("Roman Cancel Damage Modifier. Applied during Roman Cancel slowdown."));
				ImGui::SameLine();
				
				ImGui::TextUnformatted(" = ");
				ImGui::SameLine();
				
				sprintf_s(strbuf, "%3d%c", totalProration / 100, '%');
				ImGui::TextUnformatted(strbuf);
				AddTooltip(searchTooltip("Total proration"));
				
			}
				
			ImGui::PopStyleVar();
			ImGui::Separator();
			
			static std::vector<DmgCalc> searchDmgCalcs;
			if (searching && searchDmgCalcs.empty()) {
				DmgCalc newCalc;
				newCalc.hitResult = HIT_RESULT_BLOCKED;
				newCalc.blockType = BLOCK_TYPE_NORMAL;
				searchDmgCalcs.push_back(newCalc);
				
				newCalc.hitResult = HIT_RESULT_ARMORED;
				searchDmgCalcs.push_back(newCalc);
				
				newCalc.hitResult = HIT_RESULT_NORMAL;
				newCalc.u.hit.extraInverseProration = 80;
				newCalc.u.hit.stylishDamageInverseModifier = 80;
				newCalc.u.hit.handicap = 156;
				newCalc.u.hit.needReduceRisc = true;
				searchDmgCalcs.push_back(newCalc);
			}
			const std::vector<DmgCalc>& dmgCalcsUse = searching ? searchDmgCalcs : player.dmgCalcs;
			
			if (!dmgCalcsUse.empty()) {
				
				bool isFirst = true;
				bool needPrintHitNumbers = dmgCalcsUse.size() > 1;
				bool useSlang = settings.useSlangNames;
				int hitCounter = (searching ? 0 : player.dmgCalcsSkippedHits) + dmgCalcsUse.size() - 1;
				for (auto it = dmgCalcsUse.begin() + (dmgCalcsUse.size() - 1); ; ) {
					const DmgCalc& dmgCalc = *it;
					
					if (!isFirst) ImGui::Separator();
					isFirst = false;
					
					if (needPrintHitNumbers) {
						ImGui::PushStyleVarX(ImGuiStyleVar_ItemSpacing, 0.F);
						textUnformattedColored(YELLOW_COLOR, searchFieldTitle("Hit Number: "));
						ImGui::SameLine();
						sprintf_s(strbuf, "%d", hitCounter + 1);
						ImGui::TextUnformatted(strbuf);
						ImGui::PopStyleVar();
					}
					--hitCounter;
					
					ImGui::PushStyleVarX(ImGuiStyleVar_ItemSpacing, 0.F);
					
					textUnformattedColored(YELLOW_COLOR, searchFieldTitle("Attack Name: "));
					ImGui::SameLine();
					ImGui::TextUnformatted(useSlang && dmgCalc.attackSlangName ? dmgCalc.attackSlangName : dmgCalc.attackName);
					if (dmgCalc.nameFull || useSlang && dmgCalc.attackSlangName && dmgCalc.attackName) {
						AddTooltip(dmgCalc.nameFull ? dmgCalc.nameFull : dmgCalc.attackName);
					}
					
					textUnformattedColored(YELLOW_COLOR, searchFieldTitle("Is Projectile: "));
					ImGui::SameLine();
					ImGui::TextUnformatted(dmgCalc.isProjectile ? "Yes" : "No");
					
					textUnformattedColored(YELLOW_COLOR, searchFieldTitle("Guard Type: "));
					ImGui::SameLine();
					const char* guardTypeStr;
					if (dmgCalc.isThrow) {
						guardTypeStr = "Throw";
					} else {
						guardTypeStr = formatGuardType(dmgCalc.guardType);
					}
					ImGui::TextUnformatted(guardTypeStr);
					
					textUnformattedColored(YELLOW_COLOR, searchFieldTitle("Air Blockable: "));
					AddTooltip(searchTooltip("Is air blockable - if not, then requires Faultless Defense to be blocked in the air."));
					ImGui::SameLine();
					ImGui::TextUnformatted(dmgCalc.airUnblockable ? "No" : "Yes");
					
					if (dmgCalc.guardCrush || searching) {
						textUnformattedColored(YELLOW_COLOR, searchFieldTitle("Guard Crush: "));
						AddTooltip(searchTooltip("Guard break. When blocked, this attack causes the defender to enter hitstun on the next frame."));
						ImGui::SameLine();
						ImGui::TextUnformatted(dmgCalc.guardCrush ? "Yes" : "No");
					}
					
					ImGui::PopStyleVar();
					
					zerohspacing
					textUnformattedColored(YELLOW_COLOR, searchFieldTitle("Last Hit Result: "));
					ImGui::SameLine();
					ImGui::TextUnformatted(formatHitResult(dmgCalc.hitResult));
					_zerohspacing
					
					if (dmgCalc.hitResult == HIT_RESULT_BLOCKED) {
						zerohspacing
						textUnformattedColored(YELLOW_COLOR, searchFieldTitle("Block Type: "));
						ImGui::SameLine();
						ImGui::TextUnformatted(formatBlockType(dmgCalc.blockType));
						_zerohspacing
						if (dmgCalc.blockType != BLOCK_TYPE_FAULTLESS) {
							const DmgCalc::DmgCalcU::DmgCalcBlock& data = dmgCalc.u.block;
							if (ImGui::BeginTable("##DmgCalc", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_NoPadOuterX)) {
								ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0.5f);
								ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.5f);
								ImGui::TableHeadersRow();
								
								printAttackLevel(dmgCalc);
								
								ImGui::TableNextColumn();
								textUnformattedColored(LIGHT_BLUE_COLOR, "RISC+");
								AddTooltip(searchTooltip("The base value for determining how much RISC this attack adds on block."));
								ImGui::TableNextColumn();
								sprintf_s(strbuf, "%d", data.riscPlusBase);
								const char* needHelp = nullptr;
								textUnformattedColored(LIGHT_BLUE_COLOR, strbuf);
								ImVec4* color = &RED_COLOR;
								if (data.riscPlusBase > data.riscPlusBaseStandard) {
									needHelp = "higher";
								} else if (data.riscPlusBase < data.riscPlusBaseStandard) {
									needHelp = "lower";
									color = &LIGHT_BLUE_COLOR;
								}
								if (needHelp) {
									ImGui::SameLine();
									textUnformattedColored(*color, "(!)");
									if (ImGui::BeginItemTooltip()) {
										ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
										sprintf_s(strbuf, "This attack's RISC+ (%d) is %s than the standard RISC+ (%d) for its attack level %s(%d).",
											data.riscPlusBase,
											needHelp,
											data.riscPlusBaseStandard,
											dmgCalc.attackOriginalAttackLevel == dmgCalc.attackLevelForGuard ? "" : "on block/armor",
											dmgCalc.attackLevelForGuard);
										ImGui::TextUnformatted(strbuf);
										ImGui::PopTextWrapPos();
										ImGui::EndTooltip();
									}
								}
								
								ImGui::TableNextColumn();
								ImGui::TextUnformatted(searchFieldTitle("RISC Gain Rate"));
								AddTooltip(searchTooltip("A per-character constant value that alters incoming values of RISC+."
									" Depends on the defending player's character."));
								ImGui::TableNextColumn();
								printDecimal(data.guardBalanceDefence * 100 / 32, 0, 0, true);
								sprintf_s(strbuf, "%d (%s)", data.guardBalanceDefence, printdecimalbuf);
								ImGui::TextUnformatted(strbuf);
								
								int x = data.riscPlusBase * 100 * data.guardBalanceDefence / 32;
								ImGui::TableNextColumn();
								zerohspacing
								textUnformattedColored(LIGHT_BLUE_COLOR, "RISC+");
								ImGui::SameLine();
								ImGui::TextUnformatted(" * Gain Rate");
								_zerohspacing
								ImGui::TableNextColumn();
								sprintf_s(strbuf, "%d * %s = ", data.riscPlusBase, printdecimalbuf);
								zerohspacing
								ImGui::TextUnformatted(strbuf);
								ImGui::SameLine();
								sprintf_s(strbuf, "%s", printDecimal(x, 2, 0, false));
								textUnformattedColored(LIGHT_BLUE_COLOR, strbuf);
								_zerohspacing
								
								ImGui::TableNextColumn();
								ImGui::TextUnformatted(searchFieldTitle("Grounded and Overhead/Low"));
								AddTooltip(searchTooltip("The defender was on the ground and the attack was either an overhead or a low."
									" If yes, the modifier is 75%, otherwise RISC+ is unchanged."));
								ImGui::TableNextColumn();
								int oldX = x;
								if (data.groundedAndOverheadOrLow) {
									ImGui::TextUnformatted("Yes: 75% modifier");
									x -= x / 4;
								} else {
									ImGui::TextUnformatted("No: 100% (no) modifier");
								}
								
								ImGui::TableNextColumn();
								zerohspacing
								textUnformattedColored(LIGHT_BLUE_COLOR, "RISC+");
								ImGui::SameLine();
								ImGui::TextUnformatted(" * Overhead/Low");
								_zerohspacing
								ImGui::TableNextColumn();
								zerohspacing
								sprintf_s(strbuf, "%s * %s = ", printdecimalbuf, data.groundedAndOverheadOrLow ? "75%" : "100%");
								ImGui::TextUnformatted(strbuf);
								ImGui::SameLine();
								textUnformattedColored(LIGHT_BLUE_COLOR, printDecimal(x, 2, 0, false));
								_zerohspacing
								
								ImGui::TableNextColumn();
								ImGui::TextUnformatted(searchFieldTitle("Was In Blockstun"));
								AddTooltip(searchTooltip("The defender was already in blockstun at the time of attack."
									" If yes, the modifier is 50%, otherwise RISC+ is unchanged."));
								ImGui::TableNextColumn();
								oldX = x;
								if (data.wasInBlockstun) {
									ImGui::TextUnformatted("Yes: 50% modifier");
									x /= 2;
								} else {
									ImGui::TextUnformatted("No: 100% (no) modifier");
								}
								
								ImGui::TableNextColumn();
								searchFieldTitle("RISC+");
								const char* tooltip = searchTooltip("This is the final RISC value that gets added to the RISC gauge.");
								zerohspacing
								textUnformattedColored(LIGHT_BLUE_COLOR, "RISC+");
								AddTooltip(tooltip);
								ImGui::SameLine();
								ImGui::TextUnformatted(" * Was In Blockstun");
								AddTooltip(tooltip);
								_zerohspacing
								ImGui::TableNextColumn();
								zerohspacing
								sprintf_s(strbuf, "%s * %s = ", printdecimalbuf, data.wasInBlockstun ? "50%" : "100%");
								ImGui::TextUnformatted(strbuf);
								ImGui::SameLine();
								textUnformattedColored(LIGHT_BLUE_COLOR, printDecimal(x, 2, 0, false));
								_zerohspacing
								
								ImGui::TableNextColumn();
								ImGui::TextUnformatted("RISC");
								AddTooltip(searchTooltip("Final value for RISC, without bounds check for [-128.00; 128.00] is"
									" the old value + change = final value."));
								ImGui::TableNextColumn();
								
								char* buf = strbuf;
								size_t bufSize = sizeof strbuf;
								int result = sprintf_s(buf, bufSize, "%s + ", printDecimal(data.defenderRisc, 2, 0, false));
								advanceBuf
								
								result = sprintf_s(buf, bufSize, "%s = ", printDecimal(x, 2, 0, false));
								advanceBuf
								
								sprintf_s(buf, bufSize, "%s", printDecimal(data.defenderRisc + x, 2, 0, false));
								
								ImGui::TextUnformatted(strbuf);
								
								x = printBaseDamageCalc(dmgCalc, nullptr);
								
								x = printChipDamageCalculation(x, data.baseDamage, data.attackKezuri, data.attackKezuriStandard);
								
								ImGui::TableNextColumn();
								ImGui::TextUnformatted("HP");
								ImGui::TableNextColumn();
								sprintf_s(strbuf, "%d - %d = %d", dmgCalc.oldHp, x, dmgCalc.oldHp - x);
								ImGui::TextUnformatted(strbuf);
								
								ImGui::EndTable();
							}
						}
					} else if (dmgCalc.hitResult == HIT_RESULT_ARMORED || dmgCalc.hitResult == HIT_RESULT_ARMORED_BUT_NO_DMG_REDUCTION) {
						const DmgCalc::DmgCalcU::DmgCalcArmor& data = dmgCalc.u.armor;
						if (ImGui::BeginTable("##DmgCalc", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_NoPadOuterX)) {
							ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0.5f);
							ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.5f);
							ImGui::TableHeadersRow();
							
							printAttackLevel(dmgCalc);
							
							int x = printBaseDamageCalc(dmgCalc, nullptr);
							int baseDamage = x;
							
							x = printScaleDmgBasic(x, i, data.damageScale, data.isProjectile, data.projectileDamageScale, dmgCalc.hitResult, data.superArmorDamagePercent);
							
							ImGui::TableNextColumn();
							ImGui::TextUnformatted(searchFieldTitle("Armor Is Like Block?"));
							AddTooltip(searchTooltip("If the armor behaves like blocking when tanking hits, it won't use guts calculation and will use the"
								" same chip damage calculation as blocking uses. Otherwise it will apply guts and take that as damage."));
							ImGui::TableNextColumn();
							ImGui::TextUnformatted(data.superArmorHeadAttribute ? "Yes" : "No");
							
							if (data.superArmorHeadAttribute) {
								x = printChipDamageCalculation(x, baseDamage, data.attackKezuri, data.attackKezuriStandard);;
							} else {
								x = printDamageGutsCalculation(x, data.defenseModifier, data.gutsRating, data.guts, data.gutsLevel);
							}
							
							ImGui::TableNextColumn();
							ImGui::TextUnformatted("HP");
							ImGui::TableNextColumn();
							sprintf_s(strbuf, "%d - %d = %d", dmgCalc.oldHp, x, dmgCalc.oldHp - x);
							ImGui::TextUnformatted(strbuf);
							
							ImGui::EndTable();
						}
					} else if (dmgCalc.hitResult == HIT_RESULT_NORMAL) {
						const DmgCalc::DmgCalcU::DmgCalcHit& data = dmgCalc.u.hit;
						if (ImGui::BeginTable("##DmgCalc", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_NoPadOuterX)) {
							ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0.5f);
							ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.5f);
							ImGui::TableHeadersRow();
							
							printAttackLevel(dmgCalc);
							
							int damageAfterHpScaling;
							int x = printBaseDamageCalc(dmgCalc, &damageAfterHpScaling);
							int baseDamage = x;
							
							int oldX = x;
							if (data.increaseDmgBy50Percent) {
								x = x * 150 / 100;
								ImGui::TableNextColumn();
								const char* tooltip = searchTooltip("Maybe Dustloop or someone knows what this is.");
								zerohspacing
								textUnformattedColored(YELLOW_COLOR, "Dmg");
								AddTooltip(tooltip);
								ImGui::SameLine();
								ImGui::TextUnformatted(" * 150%");
								AddTooltip(tooltip);
								_zerohspacing
								ImGui::TableNextColumn();
								zerohspacing
								sprintf_s(strbuf, "%d * 150%c = ", oldX, '%');
								ImGui::TextUnformatted(strbuf);
								ImGui::SameLine();
								sprintf_s(strbuf, "%d", x);
								textUnformattedColored(YELLOW_COLOR, strbuf);
								_zerohspacing
							}
							
							oldX = x;
							if (data.extraInverseProration != 100 && data.extraInverseProration != 0) {
								x = x * 100 / data.extraInverseProration;
								ImGui::TableNextColumn();
								ImGui::TextUnformatted(searchFieldTitle("Extra Inverse Modif"));
								AddTooltip("Damage = Damage * 100 / Extra Inverse Modif");
								ImGui::TableNextColumn();
								sprintf_s(strbuf, "%d%c", data.extraInverseProration, '%');
								ImGui::TextUnformatted(strbuf);
								
								ImGui::TableNextColumn();
								const char* tooltip = "Damage = Damage * 100 / Extra Inverse Modif";
								zerohspacing
								textUnformattedColored(YELLOW_COLOR, "Dmg");
								AddTooltip(tooltip);
								ImGui::SameLine();
								ImGui::TextUnformatted(" / Extra Inv. Modif");
								AddTooltip(tooltip);
								_zerohspacing
								ImGui::TableNextColumn();
								zerohspacing
								sprintf_s(strbuf, "%d / %d%c = ", oldX, data.extraInverseProration, '%');
								ImGui::TextUnformatted(strbuf);
								ImGui::SameLine();
								sprintf_s(strbuf, "%d", x);
								textUnformattedColored(YELLOW_COLOR, strbuf);
								_zerohspacing
							}
							
							oldX = x;
							if (data.isStylish && data.stylishDamageInverseModifier != 0) {
								x = x * 100 / data.stylishDamageInverseModifier;
								ImGui::TableNextColumn();
								ImGui::TextUnformatted(searchFieldTitle("Stylish Inverse Modifier"));
								AddTooltip(searchTooltip("Inverse modifier applied to the damage dealt to the defender for defender using the Stylish mode."
									" Depends on the defender using the Stylish mode."));
								ImGui::TableNextColumn();
								sprintf_s(strbuf, "%d%c", data.stylishDamageInverseModifier, '%');
								ImGui::TextUnformatted(strbuf);
								
								ImGui::TableNextColumn();
								zerohspacing
								textUnformattedColored(YELLOW_COLOR, "Dmg");
								ImGui::SameLine();
								ImGui::TextUnformatted(" / Stylish");
								_zerohspacing
								ImGui::TableNextColumn();
								zerohspacing
								sprintf_s(strbuf, "%d / %d%c = ", oldX, data.stylishDamageInverseModifier, '%');
								ImGui::TextUnformatted(strbuf);
								ImGui::SameLine();
								sprintf_s(strbuf, "%d", x);
								textUnformattedColored(YELLOW_COLOR, strbuf);
								_zerohspacing
							}
							
							oldX = x;
							if (data.handicap != 100) {
								x = x * data.handicap / 100;
								ImGui::TableNextColumn();
								ImGui::TextUnformatted(searchFieldTitle("Handicap"));
								AddTooltip(searchTooltip("Handicap used by the defender modifies their incoming damage."
									" Handicap levels:\n"
									"1) 156%;\n"
									"2) 125%;\n"
									"3) 100%;\n"
									"4) 80%;\n"
									"5) 64%;"));
								ImGui::TableNextColumn();
								sprintf_s(strbuf, "%d%c", data.handicap, '%');
								ImGui::TextUnformatted(strbuf);
								
								ImGui::TableNextColumn();
								zerohspacing
								textUnformattedColored(YELLOW_COLOR, "Dmg");
								ImGui::SameLine();
								ImGui::TextUnformatted(" * Handicap");
								_zerohspacing
								ImGui::TableNextColumn();
								zerohspacing
								sprintf_s(strbuf, "%d * %d%c = ", oldX, data.handicap, '%');
								ImGui::TextUnformatted(strbuf);
								ImGui::SameLine();
								sprintf_s(strbuf, "%d", x);
								textUnformattedColored(YELLOW_COLOR, strbuf);
								_zerohspacing
							}
							
							x = printScaleDmgBasic(x, i, data.damageScale, data.isProjectile, data.projectileDamageScale, HIT_RESULT_NORMAL, 100);
							
							oldX = x;
							if (data.dustProration1 != 100) {
								x = x * data.dustProration1 / 100;
								ImGui::TableNextColumn();
								if (data.dustProration2 != 100) {
									ImGui::TextUnformatted(searchFieldTitle("Dust Proration #1"));
								} else {
									ImGui::TextUnformatted(searchFieldTitle("Dust Proration"));
								}
								sprintf_s(strbuf, "%d%c", data.dustProration1, '%');
								ImGui::TextUnformatted(strbuf);
								
								ImGui::TableNextColumn();
								zerohspacing
								textUnformattedColored(YELLOW_COLOR, "Dmg");
								ImGui::SameLine();
								if (data.dustProration2 != 100) {
									ImGui::TextUnformatted(" * Dust Proration #1");
								} else {
									ImGui::TextUnformatted(" * Dust Proration");
								}
								_zerohspacing
								ImGui::TableNextColumn();
								zerohspacing
								sprintf_s(strbuf, "%d * %d%c = ", oldX, data.dustProration1, '%');
								ImGui::TextUnformatted(strbuf);
								ImGui::SameLine();
								sprintf_s(strbuf, "%d", x);
								textUnformattedColored(YELLOW_COLOR, strbuf);
								_zerohspacing
							}
							
							oldX = x;
							x = x * data.dustProration2 / 100;
							ImGui::TableNextColumn();
							if (data.dustProration1 != 100) {
								ImGui::TextUnformatted(searchFieldTitle("Dust Proration #2"));
							} else {
								ImGui::TextUnformatted(searchFieldTitle("Dust Proration"));
							}
							ImGui::TableNextColumn();
							sprintf_s(strbuf, "%d%c", data.dustProration2, '%');
							ImGui::TextUnformatted(strbuf);
							
							ImGui::TableNextColumn();
							zerohspacing
							textUnformattedColored(YELLOW_COLOR, "Dmg");
							ImGui::SameLine();
							if (data.dustProration1 != 100) {
								ImGui::TextUnformatted(" * Dust Proration #2");
							} else {
								ImGui::TextUnformatted(" * Dust Proration");
							}
							_zerohspacing
							ImGui::TableNextColumn();
							zerohspacing
							sprintf_s(strbuf, "%d * %d%c = ", oldX, data.dustProration2, '%');
							ImGui::TextUnformatted(strbuf);
							ImGui::SameLine();
							sprintf_s(strbuf, "%d", x);
							textUnformattedColored(YELLOW_COLOR, strbuf);
							_zerohspacing
							
							bool hellfire = data.attackerHellfireState && data.attackerHpLessThan10Percent && data.attackHasHellfireEnabled;
							ImGui::TableNextColumn();
							ImGui::TextUnformatted(searchFieldTitle("Hellfire"));
							AddTooltip(searchTooltip("To gain 20% damage bonus, the attacker must have hellfire state enabled, they must have <= 10% HP (<= 42 HP),"
								" and the attack must be Hellfire-enabled. All overdrives should be Hellfire-enabled."));
							ImGui::TableNextColumn();
							if (hellfire) {
								ImGui::TextUnformatted("Yes (120%)");
							} else if (data.attackerHellfireState && !data.attackerHpLessThan10Percent) {
								ImGui::TextUnformatted("No, hp>10% (hp>42) (100%)");
							} else if (data.attackerHellfireState && data.attackerHpLessThan10Percent && !data.attackHasHellfireEnabled) {
								if (dmgCalc.attackType == ATTACK_TYPE_OVERDRIVE) {
									ImGui::TextUnformatted("No, attack lacks hellfire attribute (100%)");
								} else {
									ImGui::TextUnformatted("No, not a super (100%)");
								}
							} else {
								ImGui::TextUnformatted("No (100%)");
							}
							
							oldX = x;
							ImGui::TableNextColumn();
							zerohspacing
							textUnformattedColored(YELLOW_COLOR, "Dmg");
							ImGui::SameLine();
							ImGui::TextUnformatted(" * Hellfire");
							_zerohspacing
							ImGui::TableNextColumn();
							zerohspacing
							if (hellfire) {
								x = x * 120 / 100;
								sprintf_s(strbuf, "%d * 120%c = ", oldX, '%');
							} else {
								sprintf_s(strbuf, "%d * 100%c = ", oldX, '%');
							}
							ImGui::TextUnformatted(strbuf);
							ImGui::SameLine();
							sprintf_s(strbuf, "%d", x);
							textUnformattedColored(YELLOW_COLOR, strbuf);
							_zerohspacing
							
							if (data.trainingSettingIsForceCounterHit) {
								ImGui::TableNextColumn();
								ImGui::TextUnformatted(searchFieldTitle("Attack Can't Counter Hit"));
								AddTooltip(searchTooltip("Some attacks cannot be counter hits even when the 'Counter Hit' training setting is set to 'Forced' or 'Forced Mortal Counter'."
									" However, triggering Danger Time normally and doing the attack still produces the Mortal Counter and gives the 20% damage boost."));
								ImGui::TableNextColumn();
								ImGui::TextUnformatted(
									data.attackCounterHitType == COUNTERHIT_TYPE_NO_COUNTER
										? "Yes"
										: "No"
								);
							}
							
							oldX = x;
							ImGui::TableNextColumn();
							ImGui::TextUnformatted(searchFieldTitle("Danger Time"));
							ImGui::TableNextColumn();
							if (data.dangerTime) {
								x = x * 120 / 100;
								ImGui::TextUnformatted("Yes (120%)");
							} else {
								ImGui::TextUnformatted("No (100%)");
							}
							
							ImGui::TableNextColumn();
							zerohspacing
							textUnformattedColored(YELLOW_COLOR, "Dmg");
							ImGui::SameLine();
							ImGui::TextUnformatted(" * Danger Time");
							_zerohspacing
							ImGui::TableNextColumn();
							zerohspacing
							if (data.dangerTime) {
								sprintf_s(strbuf, "%d * 120%c = ", oldX, '%');
							} else {
								sprintf_s(strbuf, "%d * 100%c = ", oldX, '%');
							}
							ImGui::TextUnformatted(strbuf);
							ImGui::SameLine();
							sprintf_s(strbuf, "%d", x);
							textUnformattedColored(YELLOW_COLOR, strbuf);
							_zerohspacing
							
							bool rcProration = data.rcDmgProration || data.wasHitDuringRc;
							
							ImGui::TableNextColumn();
							ImGui::TextUnformatted(searchFieldTitle("Current Proration"));
							AddTooltip(searchTooltip("Forced/initial proration that was at the moment of impact. Stored in the defender."));
							ImGui::TableNextColumn();
							sprintf_s(strbuf, "%d%c", data.proration, '%');
							ImGui::TextUnformatted(strbuf);
							
							ImGui::TableNextColumn();
							ImGui::TextUnformatted(searchFieldTitle("RISC"));
							AddTooltip(searchTooltip("RISC that was at the moment of impact."));
							ImGui::TableNextColumn();
							ImGui::TextUnformatted(printDecimal(data.risc, 2, 0, false));
							
							ImGui::TableNextColumn();
							ImGui::TextUnformatted("Is First Hit");
							ImGui::TableNextColumn();
							ImGui::TextUnformatted(data.isFirstHit ? "Yes" : "No");
							
							ImGui::TableNextColumn();
							ImGui::TextUnformatted(searchFieldTitle("Initial Proration"));
							AddTooltip(searchTooltip("Depends on the attack. May only be applied on first hit."));
							ImGui::TableNextColumn();
							int nextProration = data.proration;
							const char* nextProrationWhich = nullptr;
							if (data.isFirstHit) {
								if (data.initialProration == INT_MAX) {
									ImGui::TextUnformatted("None (100%)");
								} else {
									nextProration = data.initialProration;
									nextProrationWhich = "initial";
									sprintf_s(strbuf, "%d%c", data.initialProration, '%');
									ImGui::TextUnformatted(strbuf);
								}
							} else {
								ImGui::TextUnformatted("Doesn't apply (100%)");
							}
							
							ImGui::TableNextColumn();
							ImGui::TextUnformatted(searchFieldTitle("Forced Proration"));
							AddTooltip(searchTooltip("Depends on the attack."));
							ImGui::TableNextColumn();
							if (data.forcedProration == INT_MAX) {
								ImGui::TextUnformatted("None (100%)");
							} else {
								if (data.forcedProration < nextProration) {
									nextProration = data.forcedProration;
									nextProrationWhich = "forced";
								}
								sprintf_s(strbuf, "%d%c", data.forcedProration, '%');
								ImGui::TextUnformatted(strbuf);
							}
							
							ImGui::TableNextColumn();
							ImGui::TextUnformatted(searchFieldTitle("Next Proration"));
							AddTooltip(searchTooltip("The proration that is chosen out of initial or forced prorations that will apply to the consecutive combo."));
							ImGui::TableNextColumn();
							if (!nextProrationWhich) {
								sprintf_s(strbuf, "Unchanged (%d%c)", data.proration, '%');
							} else {
								sprintf_s(strbuf, "%d%c (%s)", nextProration, '%', nextProrationWhich);
							}
							ImGui::TextUnformatted(strbuf);
							
							if (!data.needReduceRisc) {
								ImGui::TableNextColumn();
								ImGui::TextUnformatted("Attack Reduces RISC");
								ImGui::TableNextColumn();
								ImGui::TextUnformatted("No");
							} else {
								int riscMinusTotal = 0;
								
								ImGui::TableNextColumn();
								searchFieldTitle("RISC- Initial");
								const char* tooltip = searchTooltip("This RISC- is applied on first hit only. Depends on the attack.");
								zerohspacing
								textUnformattedColored(LIGHT_BLUE_COLOR, "RISC-");
								AddTooltip(tooltip);
								ImGui::SameLine();
								ImGui::TextUnformatted(" Initial");
								AddTooltip(tooltip);
								_zerohspacing
								ImGui::TableNextColumn();
								if (!data.isFirstHit) {
									zerohspacing
									ImGui::TextUnformatted("Not first hit (");
									ImGui::SameLine();
									textUnformattedColored(LIGHT_BLUE_COLOR, "0");
									ImGui::SameLine();
									ImGui::TextUnformatted(")");
									_zerohspacing
								} else {
									riscMinusTotal = data.riscMinusStarter * 100;
									sprintf_s(strbuf, "%d", data.riscMinusStarter);
									textUnformattedColored(LIGHT_BLUE_COLOR, strbuf);
								}
								
								ImGui::TableNextColumn();
								textUnformattedColored(LIGHT_BLUE_COLOR, searchFieldTitle("RISC-"));
								AddTooltip(searchTooltip("Depends on the attack."));
								ImGui::TableNextColumn();
								riscMinusTotal += data.riscMinus * 100;
								sprintf_s(strbuf, "%d", data.riscMinus);
								textUnformattedColored(LIGHT_BLUE_COLOR, strbuf);
								
								ImGui::TableNextColumn();
								searchFieldTitle("RISC- Once");
								tooltip = searchTooltip("This RISC- may only be applied once. Depends on the attack.");
								zerohspacing
								textUnformattedColored(LIGHT_BLUE_COLOR, "RISC-");
								AddTooltip(tooltip);
								ImGui::SameLine();
								ImGui::TextUnformatted(" Once");
								AddTooltip(tooltip);
								_zerohspacing
								ImGui::TableNextColumn();
								if (data.riscMinusOnceUsed) {
									zerohspacing
									ImGui::TextUnformatted("Already applied (");
									ImGui::SameLine();
									textUnformattedColored(LIGHT_BLUE_COLOR, "0");
									ImGui::SameLine();
									ImGui::TextUnformatted(")");
									_zerohspacing
								} else if (data.riscMinusOnce == INT_MAX) {
									zerohspacing
									ImGui::TextUnformatted("None (");
									ImGui::SameLine();
									textUnformattedColored(LIGHT_BLUE_COLOR, "0");
									ImGui::SameLine();
									ImGui::TextUnformatted(")");
									_zerohspacing
								} else {
									riscMinusTotal += data.riscMinusOnce * 100;
									sprintf_s(strbuf, "%d", data.riscMinusOnce);
									textUnformattedColored(LIGHT_BLUE_COLOR, strbuf);
								}
								
								ImGui::TableNextColumn();
								ImGui::TextUnformatted("RISC > 0 ?");
								AddTooltip("RISC reduces by 25% extra on each hit when it is positive.");
								ImGui::TableNextColumn();
								int riscReductionExtra = 0;
								if (data.risc > 0) {
									riscReductionExtra = data.risc >> 3;
									riscMinusTotal += riscReductionExtra;
									zerohspacing
									ImGui::TextUnformatted("Yes (extra 'RISC-' = ");
									ImGui::SameLine();
									textUnformattedColored(LIGHT_BLUE_COLOR, printDecimal(riscReductionExtra, 2, 0, false));
									ImGui::SameLine();
									ImGui::TextUnformatted(")");
									_zerohspacing
								} else {
									zerohspacing
									ImGui::TextUnformatted("No (extra 'RISC-' = ");
									ImGui::SameLine();
									textUnformattedColored(LIGHT_BLUE_COLOR, "0");
									ImGui::SameLine();
									ImGui::TextUnformatted(")");
									_zerohspacing
								}
								
								ImGui::TableNextColumn();
								zerohspacing
								textUnformattedColored(LIGHT_BLUE_COLOR, searchFieldTitle("RISC-"));
								ImGui::SameLine();
								ImGui::TextUnformatted(" Total");
								_zerohspacing
								ImGui::TableNextColumn();
								sprintf_s(strbuf, "%d + %d + %d + %s = ",
									data.isFirstHit ? data.riscMinusStarter : 0,
									data.riscMinus,
									!data.riscMinusOnceUsed && data.riscMinusOnce != INT_MAX ? data.riscMinusOnce : 0,
									printDecimal(riscReductionExtra, 2, 0, false));
								zerohspacing
								ImGui::TextUnformatted(strbuf);
								ImGui::SameLine();
								textUnformattedColored(LIGHT_BLUE_COLOR, printDecimal(riscMinusTotal, 2, 0, false));
								_zerohspacing
								
								ImGui::TableNextColumn();
								ImGui::TextUnformatted("RISC");
								ImGui::TableNextColumn();
								char* buf = strbuf;
								size_t bufSize = sizeof strbuf;
								int result = sprintf_s(strbuf, "%s - ", printDecimal(data.risc, 2, 0, false));
								advanceBuf
								result = sprintf_s(buf, bufSize, "%s = ", printDecimal(riscMinusTotal, 2, 0, false));
								advanceBuf
								result = sprintf_s(buf, bufSize, "%s", printDecimal(data.risc - riscMinusTotal, 2, 0, false));
								advanceBuf
								ImGui::TextUnformatted(strbuf);
								
							}
							
							ImGui::TableNextColumn();
							ImGui::TextUnformatted(searchFieldTitle("Attack Type"));
							AddTooltip(searchTooltip("Attack type affects which RISC Damage Scaling table is used. Overdrives prorate differently from normals and specials.\n"
								"More info in the tooltip of the field below."));
							ImGui::TableNextColumn();
							ImGui::TextUnformatted(formatAttackType(dmgCalc.attackType));
							
							ImGui::TableNextColumn();
							ImGui::TextUnformatted(searchFieldTitle("RISC Dmg Scaling"));
							AddTooltip(searchTooltip("This damage scaling depends on the defender's RISC that was at the moment of impact"
								" and the attacker's attack type.\n"
								"Normal and special attacks use this table:\n"
								"256, 200, 152, 112, 80, 48, 32, 16, 8, 8, 8;\n"
								"Overdrives use this table:\n"
								"256, 176, 128, 96, 80, 48, 40, 32, 24, 16, 16;\n"
								"X = -(RISC/100) - 1; Round the division down. The RISC here is [-12800; +12800].\n"
								"If RISC >= 0, the table is not used and RISC Damage Scaling is 256 (100%).\n"
								"Index into the table = X / 16; Round down. Index starts from 0.\n"
								"M = remainder of division of X by 16. From 0 to 15.\n"
								"RISC Dmg Scaling (%) = (table[index] * 16 - (table[index] - table[index + 1]) * M) / 16 * 100% / 256;"
								" Round down both divisions.\n"));
							ImGui::TableNextColumn();
							sprintf_s(strbuf, "%d%c", data.comboProration * 100 / 256, '%');
							ImGui::TextUnformatted(strbuf);
							
							int proration = data.proration * data.comboProration / 100;
							ImGui::TableNextColumn();
							ImGui::TextUnformatted(searchFieldTitle("Current Pror. * RISC Scaling"));
							AddTooltip(searchTooltip("Current Proration, which is forced/initial proration that was at the moment of impact,"
								" * RISC Damage Scaling"));
							ImGui::TableNextColumn();
							if (data.noDamageScaling) {
								proration = 256;
								ImGui::TextUnformatted("No Damage Scaling (100%)");
								ImGui::SameLine();
								HelpMarker("Depends on the attack. Attack ignores initial/forced proration and RISC Damage Scaling.");
							} else {
								sprintf_s(strbuf, "%d%c * %d%c = %d%c",
									data.proration,
									'%',
									data.comboProration * 100 / 256,
									'%',
									proration * 100 / 256,
									'%');
								ImGui::TextUnformatted(strbuf);
							}
							
							int damagePriorToProration = x;
							oldX = x;
							x = x * proration / 256;
							ImGui::TableNextColumn();
							const char* tooltip = "Damage * Current proration * RISC Damage Scaling."
								" Current proration is forced/initial proration that was at the moment of impact.";
							zerohspacing
							textUnformattedColored(YELLOW_COLOR, "Dmg");
							AddTooltip(tooltip);
							ImGui::SameLine();
							ImGui::TextUnformatted(" * Pror. * Scaling");
							AddTooltip(tooltip);
							_zerohspacing
							ImGui::TableNextColumn();
							zerohspacing
							sprintf_s(strbuf, "%d * %d%c = ",
									oldX,
									proration * 100 / 256,
									'%');
							ImGui::TextUnformatted(strbuf);
							ImGui::SameLine();
							sprintf_s(strbuf, "%d", x);
							textUnformattedColored(YELLOW_COLOR, strbuf);
							_zerohspacing
							
							ImGui::TableNextColumn();
							ImGui::TextUnformatted(searchFieldTitle("Roman Cancel"));
							AddTooltip(searchTooltip("Proration resulting from landing a hit during Roman Cancel slowdown."));
							ImGui::TableNextColumn();
							if (rcProration) {
								ImGui::TextUnformatted("Yes (80%)");
							} else {
								ImGui::TextUnformatted("No (100%)");
							}
							
							oldX = x;
							ImGui::TableNextColumn();
							tooltip = "Damage * Roman Cancel proration";
							zerohspacing
							textUnformattedColored(YELLOW_COLOR, "Damage");
							AddTooltip(tooltip);
							ImGui::SameLine();
							ImGui::TextUnformatted(" * RC");
							AddTooltip(tooltip);
							_zerohspacing
							ImGui::TableNextColumn();
							zerohspacing
							if (rcProration) {
								x = x * 80 / 100;
								sprintf_s(strbuf, "%d * 80%c = ", oldX, '%');
								ImGui::TextUnformatted(strbuf);
							} else {
								ImGui::TextUnformatted("Doesn't apply (");
							}
							ImGui::SameLine();
							sprintf_s(strbuf, "%d", x);
							textUnformattedColored(YELLOW_COLOR, strbuf);
							if (!rcProration) {
								ImGui::SameLine();
								ImGui::TextUnformatted(")");
							}
							_zerohspacing
							
							if (damagePriorToProration > 0 && x < 1) {
								x = 1;
							}
							
							ImGui::TableNextColumn();
							ImGui::TextUnformatted(searchFieldTitle("Minimum Dmg %"));
							AddTooltip(searchTooltip("Minimum Damage Percent. Calculated from Base Damage. Depends on the attack."));
							ImGui::TableNextColumn();
							if (data.minimumDamagePercent == 0) {
								ImGui::TextUnformatted("None (0%)");
							} else {
								sprintf_s(strbuf, "%d%c", data.minimumDamagePercent, '%');
								ImGui::TextUnformatted(strbuf);
							}
							
							ImGui::TableNextColumn();
							ImGui::TextUnformatted(searchFieldTitle("Base Dmg * Min Dmg %"));
							ImGui::TableNextColumn();
							int minDmg = 0;
							if (data.minimumDamagePercent == 0) {
								sprintf_s(strbuf, "Doesn't apply (0)");
							} else {
								minDmg = baseDamage * data.minimumDamagePercent / 100;
								sprintf_s(strbuf, "%d * %d%c = %d", baseDamage, data.minimumDamagePercent, '%', minDmg);
							}
							ImGui::TextUnformatted(strbuf);
							
							ImGui::TableNextColumn();
							zerohspacing
							textUnformattedColored(YELLOW_COLOR, "Damage");
							ImGui::SameLine();
							ImGui::TextUnformatted(" or Min ");
							ImGui::SameLine();
							textUnformattedColored(YELLOW_COLOR, "Dmg");
							_zerohspacing
							ImGui::TableNextColumn();
							if (data.minimumDamagePercent != 0 && x < minDmg) x = minDmg;
							sprintf_s(strbuf, "%d", x);
							textUnformattedColored(YELLOW_COLOR, strbuf);
							
							x = printDamageGutsCalculation(x, data.defenseModifier, data.gutsRating, data.guts, data.gutsLevel);
							
							ImGui::TableNextColumn();
							ImGui::TextUnformatted(searchFieldTitle("HP<=Dmg and HP>=30% MaxHP"));
							AddTooltip(searchTooltip("When HP at the moment of hit is less than or equal to the damage, and HP is greater than or equal to max HP * 30% (HP>=126),"
								" the damage gets changed to:\n"
								"Damage = HP - Max HP * 5% or, in other words, Damage = HP - 21"));
							ImGui::TableNextColumn();
							bool attackIsTooOP = dmgCalc.oldHp <= x && dmgCalc.oldHp >= dmgCalc.maxHp * 30 / 100;
							ImGui::TextUnformatted(attackIsTooOP ? "Yes" : "No");
							
							ImGui::TableNextColumn();
							tooltip = "Damage after change due to the condition above.";
							zerohspacing
							textUnformattedColored(YELLOW_COLOR, "Damage");
							AddTooltip(tooltip);
							if (attackIsTooOP) {
								ImGui::SameLine();
								ImGui::TextUnformatted(" = HP - 21");
								AddTooltip(tooltip);
							}
							_zerohspacing
							ImGui::TableNextColumn();
							zerohspacing
							if (attackIsTooOP) {
								x = dmgCalc.oldHp - dmgCalc.maxHp * 5 / 100;
								sprintf_s(strbuf, "%d - 21 = ", dmgCalc.oldHp);
								ImGui::TextUnformatted(strbuf);
								ImGui::SameLine();
							}
							sprintf_s(strbuf, "%d", x);
							textUnformattedColored(YELLOW_COLOR, strbuf);
							_zerohspacing
							
							
							ImGui::TableNextColumn();
							ImGui::TextUnformatted(searchFieldTitle("Attack Is Kill"));
							AddTooltip(searchTooltip("This was observed to not happen immediately when landing an IK, but it does happen during the IK cinematic."
								" When an attack is a kill, it always deals damage equal to the entire remaining health of the defender no matter what."));
							ImGui::TableNextColumn();
							ImGui::TextUnformatted(data.kill ? "Yes" : "No");
							
							ImGui::TableNextColumn();
							textUnformattedColored(YELLOW_COLOR, "Damage");
							AddTooltip("Damage after change due to the condition above.");
							ImGui::TableNextColumn();
							if (data.kill) x = dmgCalc.oldHp;
							sprintf_s(strbuf, "%d", x);
							textUnformattedColored(YELLOW_COLOR, strbuf);
							
							ImGui::TableNextColumn();
							ImGui::TextUnformatted("HP");
							ImGui::TableNextColumn();
							sprintf_s(strbuf, "%d - %d = %d", dmgCalc.oldHp, x, dmgCalc.oldHp - x);
							ImGui::TextUnformatted(strbuf);
							
							ImGui::TableNextColumn();
							searchFieldTitle("Base Stun");
							tooltip = searchTooltip("The base stun value that the attack deals. Stun can only be dealt on hit, not on block or armor.");
							zerohspacing
							ImGui::TextUnformatted("Base ");
							AddTooltip(tooltip);
							ImGui::SameLine();
							textUnformattedColored(RED_COLOR, "Stun");
							AddTooltip(tooltip);
							_zerohspacing
							ImGui::TableNextColumn();
							
							int defaultStun = damageAfterHpScaling;
							if (data.throwLockExecute) defaultStun /= 2;
							
							if (data.baseStun != defaultStun) {
								sprintf_s(strbuf, "%d", data.baseStun);
								textUnformattedColored(RED_COLOR, strbuf);
								
								const char* isLessGreater;
								ImVec4* color;
								if (data.baseStun < defaultStun) {
									isLessGreater = "less";
									color = &LIGHT_BLUE_COLOR;
								} else {
									isLessGreater = "greater";
									color = &RED_COLOR;
								}
								ImGui::SameLine();
								textUnformattedColored(*color, "(!)");
								if (ImGui::BeginItemTooltip()) {
									ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
									char* buf = strbuf;
									size_t bufSize = sizeof strbuf;
									int result = sprintf_s(strbuf, "This attack's base stun value (%d) is %s than the default value (= damage%s (%d)",
										data.baseStun,
										isLessGreater,
										dmgCalc.scaleDamageWithHp ? " after HP Damage Scale" : "",
										damageAfterHpScaling);
									advanceBuf
									if (data.throwLockExecute) {
										sprintf_s(buf, bufSize, "%s", " * 50% (due to throw animation) ).");
									} else {
										sprintf_s(buf, bufSize, " ).");
									}
									ImGui::TextUnformatted(strbuf);
									ImGui::PopTextWrapPos();
									ImGui::EndTooltip();
								}
							} else {
								zerohspacing
								sprintf_s(strbuf, "%d", data.baseStun);
								textUnformattedColored(RED_COLOR, strbuf);
								ImGui::SameLine();
								sprintf_s(strbuf, " = Base Damage%s%s",
									dmgCalc.scaleDamageWithHp ? " After HP Scale" : "",
									data.throwLockExecute ? " * 50%" : "");
								ImGui::TextUnformatted(strbuf);
								_zerohspacing
								AddTooltip("The 50% modifier is applied probably due to it being part of a throw animation.");
							}
							
							ImGui::TableNextColumn();
							ImGui::TextUnformatted(searchFieldTitle("Combo Count"));
							AddTooltip(searchTooltip("The current number of hits in a combo, including this very hit."));
							ImGui::TableNextColumn();
							sprintf_s(strbuf, "%d", data.comboCount);
							ImGui::TextUnformatted(strbuf);
							
							ImGui::TableNextColumn();
							ImGui::TextUnformatted(searchFieldTitle("Combo Stun Multiplier"));
							AddTooltip(searchTooltip("This multiplier is only applied when combo count is > 1. The limit for combo count is 12.\n"
								"X = (combo count + 2) * base stun;\n"
								"new stun = base stun - X / 16, round down."));
							
							ImGui::TableNextColumn();
							int comboCountLimited = data.comboCount;
							int somePercentage;
							if (data.comboCount <= 1) {
								ImGui::TextUnformatted("100%");
								somePercentage = 100;
							} else {
								if (data.comboCount > 12) comboCountLimited = 12;
								somePercentage = 100 - (comboCountLimited + 2) * 100 / 16;
								sprintf_s(strbuf, "%d%c", somePercentage, '%');
								ImGui::TextUnformatted(strbuf);
							}
							
							ImGui::TableNextColumn();
							zerohspacing
							searchFieldTitle("Attack Stun");
							tooltip = searchTooltip("Attack's stun after multiplication by the combo stun multiplier.");
							ImGui::TextUnformatted("Attack ");
							AddTooltip(tooltip);
							ImGui::SameLine();
							textUnformattedColored(RED_COLOR, "Stun");
							AddTooltip(tooltip);
							_zerohspacing
							ImGui::TableNextColumn();
							x = data.baseStun;
							if (data.comboCount > 1) {
								x -= (comboCountLimited + 2) * x / 16;
							}
							zerohspacing
							sprintf_s(strbuf, "%d * %d%c = ", data.baseStun, somePercentage, '%');
							ImGui::TextUnformatted(strbuf);
							ImGui::SameLine();
							sprintf_s(strbuf, "%d", x);
							textUnformattedColored(RED_COLOR, strbuf);
							_zerohspacing
							
							ImGui::TableNextColumn();
							ImGui::TextUnformatted(searchFieldTitle("OTG"));
							AddTooltip(searchTooltip("OTG (off the ground) hits reduce attack's stun to 25% of its value."));
							ImGui::TableNextColumn();
							if (dmgCalc.isOtg) {
								ImGui::TextUnformatted("Yes (25%)");
								somePercentage = 25;
							} else {
								ImGui::TextUnformatted("No (100%)");
								somePercentage = 100;
							}
							
							ImGui::TableNextColumn();
							zerohspacing
							searchFieldTitle("Attack Stun");
							tooltip = searchTooltip("Attack's stun after multiplication by the OTG modifier.");
							ImGui::TextUnformatted("Attack ");
							AddTooltip(tooltip);
							ImGui::SameLine();
							textUnformattedColored(RED_COLOR, "Stun");
							AddTooltip(tooltip);
							_zerohspacing
							ImGui::TableNextColumn();
							oldX = x;
							if (dmgCalc.isOtg) {
								x >>= 2;
							}
							sprintf_s(strbuf, "%d * %d%c = ", oldX, somePercentage, '%');
							zerohspacing
							ImGui::TextUnformatted(strbuf);
							ImGui::SameLine();
							sprintf_s(strbuf, "%d", x);
							textUnformattedColored(RED_COLOR, strbuf);
							_zerohspacing
							
							ImGui::TableNextColumn();
							ImGui::TextUnformatted(searchFieldTitle("Attack Can't Counter Hit"));
							AddTooltip(searchTooltip("Some attacks cannot be counter hits even when the 'Counter Hit' training setting is set to 'Forced' or 'Forced Mortal Counter'."));
							ImGui::TableNextColumn();
							ImGui::TextUnformatted(
								data.attackCounterHitType == COUNTERHIT_TYPE_NO_COUNTER
									? "Yes"
									: "No"
							);
							
							ImGui::TableNextColumn();
							ImGui::TextUnformatted(searchFieldTitle("Counter Hit"));
							AddTooltip(searchTooltip("Counter hits cause twice the stun."));
							ImGui::TableNextColumn();
							if (data.counterHit != COUNTER_HIT_ENTITY_VALUE_NO_COUNTERHIT) {
								ImGui::TextUnformatted("Yes (200%)");
								somePercentage = 200;
							} else {
								ImGui::TextUnformatted("No (100%)");
								somePercentage = 100;
							}
							
							ImGui::TableNextColumn();
							zerohspacing
							searchFieldTitle("Attack Stun");
							tooltip = searchTooltip("Attack's stun after multiplication by the counter hit modifier.");
							ImGui::TextUnformatted("Attack ");
							AddTooltip(tooltip);
							ImGui::SameLine();
							textUnformattedColored(RED_COLOR, "Stun");
							AddTooltip(tooltip);
							_zerohspacing
							ImGui::TableNextColumn();
							oldX = x;
							if (data.counterHit != COUNTER_HIT_ENTITY_VALUE_NO_COUNTERHIT) {
								x *= 2;
							}
							sprintf_s(strbuf, "%d * %d%c = ", oldX, somePercentage, '%');
							zerohspacing
							ImGui::TextUnformatted(strbuf);
							ImGui::SameLine();
							sprintf_s(strbuf, "%d", x);
							textUnformattedColored(RED_COLOR, strbuf);
							_zerohspacing
							
							ImGui::TableNextColumn();
							zerohspacing
							searchFieldTitle("Attack Stun - 5");
							tooltip = searchTooltip("Attack's stun is unconditionally (always) reduced by 5 on this step of the calculation.");
							ImGui::TextUnformatted("Attack ");
							AddTooltip(tooltip);
							ImGui::SameLine();
							textUnformattedColored(RED_COLOR, "Stun");
							AddTooltip(tooltip);
							ImGui::SameLine();
							ImGui::TextUnformatted(" - 5");
							AddTooltip(tooltip);
							_zerohspacing
							ImGui::TableNextColumn();
							oldX = x;
							x -= 5;
							if (x < 0) x = 0;
							zerohspacing
							sprintf_s(strbuf, "%d - 5 = ", oldX);
							ImGui::TextUnformatted(strbuf);
							ImGui::SameLine();
							sprintf_s(strbuf, "%d", x);
							textUnformattedColored(RED_COLOR, strbuf);
							_zerohspacing
							
							ImGui::TableNextColumn();
							ImGui::TextUnformatted(searchFieldTitle("IK Active"));
							AddTooltip(searchTooltip("When IK is active, attack's stun is reduced by half."));
							ImGui::TableNextColumn();
							oldX = x;
							if (data.tensionMode == TENSION_MODE_RED_IK_ACTIVE || data.tensionMode == TENSION_MODE_GOLD_IK_ACTIVE) {
								x /= 2;
								ImGui::TextUnformatted("Yes (50%)");
								somePercentage = 50;
							} else {
								ImGui::TextUnformatted("No (100%)");
								somePercentage = 100;
							}
							
							ImGui::TableNextColumn();
							zerohspacing
							searchFieldTitle("Attack Stun");
							tooltip = searchTooltip("Attack's stun after multiplication by IK modifier.");
							ImGui::TextUnformatted("Attack ");
							AddTooltip(tooltip);
							ImGui::SameLine();
							textUnformattedColored(RED_COLOR, "Stun");
							AddTooltip(tooltip);
							_zerohspacing
							ImGui::TableNextColumn();
							zerohspacing
							sprintf_s(strbuf, "%d * %d%c = ", oldX, somePercentage, '%');
							ImGui::TextUnformatted(strbuf);
							ImGui::SameLine();
							sprintf_s(strbuf, "%d", x);
							textUnformattedColored(RED_COLOR, strbuf);
							_zerohspacing
							
							ImGui::TableNextColumn();
							zerohspacing
							searchFieldTitle("Attack Stun * 17.25");
							tooltip = searchTooltip("This multiplier is fixed and is always applied. Attack's stun is limited in [0; 15000].");
							ImGui::TextUnformatted("Attack ");
							AddTooltip(tooltip);
							ImGui::SameLine();
							textUnformattedColored(RED_COLOR, "Stun");
							AddTooltip(tooltip);
							ImGui::SameLine();
							ImGui::TextUnformatted(" * 17.25");
							AddTooltip(tooltip);
							_zerohspacing
							ImGui::TableNextColumn();
							oldX = x;
							x = x * 1500 / 100 * 115 / 100;
							zerohspacing
							sprintf_s(strbuf, "%d * 17.25 = ", oldX);
							ImGui::TextUnformatted(strbuf);
							ImGui::SameLine();
							if (x < 0) x = 0;
							if (x > 15000) x = 15000;
							sprintf_s(strbuf, "%d", x);
							textUnformattedColored(RED_COLOR, strbuf);
							_zerohspacing
							
							ImGui::TableNextColumn();
							ImGui::TextUnformatted(searchFieldTitle("Stun"));
							AddTooltip("The defender's new stun value is equal to old stun value + attack's final stun. If it reached the maximum stun,"
								" defender is dizzied and next maximum stun is increased by 25% up to a maximum of 12000.");
							ImGui::TableNextColumn();
							sprintf_s(strbuf, "%d + %d = %d out of %d", data.oldStun, x, data.oldStun + x, data.stunMax * 100);
							ImGui::TextUnformatted(strbuf);
							
							ImGui::EndTable();
						}
					}
					
					if (it == dmgCalcsUse.begin()) break;
					--it;
				}
				
				if (searching ? 0 : player.dmgCalcsSkippedHits) {
					ImGui::Separator();
					sprintf_s(strbuf, "Skipped %d hit%s...", player.dmgCalcsSkippedHits, player.dmgCalcsSkippedHits == 1 ? "" : "s");
					ImGui::TextUnformatted(strbuf);
				}
				
			} else {
				
				ImGui::PushStyleVarX(ImGuiStyleVar_ItemSpacing, 0.F);
				textUnformattedColored(YELLOW_COLOR, "Last hit result: ");
				ImGui::SameLine();
				ImGui::TextUnformatted(formatHitResult(HIT_RESULT_NONE));
				ImGui::PopStyleVar();
				
				if (!endScene.players[1 - i].dmgCalcs.empty()
					&& !showDamageCalculation[1 - i]) {
					ImGui::TextUnformatted("The other player has info in their corresponding window.\n"
						"You might want to look over there.");
				}
			}
			
			GGIcon scaledIcon = scaleGGIconToHeight(tipsIcon, 14.F);
			drawGGIcon(scaledIcon);
			AddTooltip("Hover your mouse over individual field titles or field values (depends on each field or even sometimes current"
				" field value) to see their tooltips.");
			
			ImGui::End();
			ImGui::PopID();
		}
	}
	popSearchStack();
	searchCollapsibleSection("Stun/Stagger Mash");
	bool useAlternativeStaggerMashProgressDisplayUse = settings.useAlternativeStaggerMashProgressDisplay;
	for (int i = 0; i < two; ++i) {
		if (showStunmash[i] || searching) {
			ImGui::PushID(i);
			sprintf_s(strbuf, searching ? "search_stunmash" : "  Stun/Stagger Mash (P%d)", i + 1);
			ImGui::SetNextWindowSize({
				ImGui::GetFontSize() * 35.F,
				180.F
			}, ImGuiCond_FirstUseEver);
			if (searching) {
				ImGui::SetNextWindowPos({ 100000.F, 100000.F }, ImGuiCond_Always);
			}
			ImGui::Begin(strbuf, showStunmash + i, searching ? ImGuiWindowFlags_NoSavedSettings : 0);
			drawPlayerIconInWindowTitle(i);
			
			const PlayerInfo& player = endScene.players[i];
			
			bool kizetsu = player.pawn.dizzyMashAmountLeft() > 0 || player.cmnActIndex == CmnActKizetsu;
			if (endScene.isIGiveUp() && !searching) {
				ImGui::TextUnformatted("Online non-observer match running.");
			} else if (!player.pawn) {
				ImGui::TextUnformatted("A match isn't currently running");
			} else if (player.cmnActIndex != CmnActJitabataLoop && !kizetsu) {
				ImGui::TextUnformatted(searchFieldTitle("Not in stagger/stun"));
			} else if (kizetsu) {
				zerohspacing
				textUnformattedColored(YELLOW_COLOR, searchFieldTitle("In hitstop: "));
				ImGui::SameLine();
				ImGui::TextUnformatted(player.hitstop ? "Yes (1/3 multiplier)" : "No");
				
				textUnformattedColored(YELLOW_COLOR, searchFieldTitle("Stunmash Remaining: "));
				AddTooltip(searchTooltip("For every left/right direction press you get a 15 point reduction."
					" For every frame that any of PKSHD buttons is pressed you get a 15 point reduction."
					" Pressing a direction AND a button on the same frame combines these reductions."
					" If you're in hitstop, in both cases you get a 5 reduction instead of 15, and"
					" you can still combine both direction and a button press to get 5+5=10 reduction on that frame."
					" When not in hitstop, the stunmash remaining automatically decreases by 10 each frame and this can be combined"
					" with your own input of direction and button presses for a maximum of 40 reduction per frame."
					" Pressing multiple buttons on the same frame does not yield any extra bonuses than pressing one"
					" button on that frame."
					" The direction and button presses cannot be buffered. Starting a mash before faint begins"
					" does not translate to having a headstart on the mash progress."));
				ImGui::SameLine();
				sprintf_s(strbuf, "%d/%d", player.pawn.dizzyMashAmountLeft(), player.pawn.dizzyMashAmountMax());
				ImGui::TextUnformatted(strbuf);
				
				BYTE* funcStart = player.pawn.bbscrCurrentFunc();
				if (strcmp((const char*)(funcStart + 4), "CmnActKizetsu") == 0) {
					BYTE* markerPos = moves.findSetMarker(funcStart, "_End");
					if (markerPos) {
						BYTE* currentInst = player.pawn.bbscrCurrentInstr();
						int currentDuration = 0;
						int totalDuration = 0;
						int lastDuration = 0;
						BYTE* instIt = moves.skipInstruction(markerPos);
						while (moves.instructionType(instIt) != Moves::instr_endState) {
							lastDuration = *(int*)(instIt + 4 + 32);
							totalDuration += lastDuration;
							if (instIt < currentInst) {
								currentDuration += lastDuration;
							}
							instIt = moves.skipInstruction(instIt);
						}
						
						textUnformattedColored(YELLOW_COLOR, searchFieldTitle("Recovery Animation: "));
						ImGui::SameLine();
						int recoveryElapsed = 0;
						if (currentInst > markerPos) {
							recoveryElapsed = currentDuration - lastDuration + player.sprite.frame + 1;
						}
						sprintf_s(strbuf, "%d/%d", recoveryElapsed, totalDuration > 6 ? 6 : totalDuration);
						ImGui::TextUnformatted(strbuf);
						ImGui::SameLine();
						ImGui::TextUnformatted(" (Fully actionable)");
						AddTooltip(searchTooltip("Recovery from faint is always fully actionable."));
					}
				}
				_zerohspacing
				
			} else {
				zerohspacing
				int mashMax = player.pawn.receivedAttack()->staggerDuration;
				int mashedAmount = mashMax - player.pawn.bbscrvar5() / 10;
				int mashedAmountPrev = mashMax - player.prevBbscrvar5 / 10;
				
				textUnformattedColored(YELLOW_COLOR, searchFieldTitle("Stagger Duration: "));
				AddTooltip(searchTooltip("The original amount of stagger inflicted by the attack."));
				ImGui::SameLine();
				sprintf_s(strbuf, "%d", mashMax);
				ImGui::TextUnformatted(strbuf);
				
				textUnformattedColored(YELLOW_COLOR, searchFieldTitle("Mashed: "));
				AddTooltip(searchTooltip("How much you've mashed vs how much you can possibly mash. Mashing above the limit achieves no extra stagger reduction."));
				ImGui::SameLine();
				sprintf_s(strbuf, "%d/%d", mashedAmount, player.pawn.bbscrvar3());
				ImGui::TextUnformatted(strbuf);
				
				textUnformattedColored(YELLOW_COLOR, searchFieldTitle("Animation Duration: "));
				AddTooltip(searchTooltip("The current stagger animation's duration."));
				ImGui::SameLine();
				sprintf_s(strbuf, "%d%s", player.animFrame, player.hitstop ? " (is in hitstop)" : "");
				ImGui::TextUnformatted(strbuf);
				
				float cursorY = ImGui::GetCursorPosY();
				ImGuiStyle& style = ImGui::GetStyle();
				ImGui::SetCursorPosY(cursorY + style.FramePadding.y);
				textUnformattedColored(YELLOW_COLOR, searchFieldTitle("Progress Previous: "));
				AddTooltip(searchTooltip("The progress 'previous' includes both the current duration of the stagger animation, as it is on THIS frame,"
					" and a snapshot or a saved value of how much you've mashed so far, as it was on the PREVIOUS frame.\n"
					" The value that the game checks for whether to decide if you should enter the stagger recovery animation or not,"
					" is the value of 'Progress Previous'.\n"
					" Note that stagger animation does not progress during hitstop, so it is possible to start mashing before the stagger animation"
					" goes past its first frame."));
				ImGui::SameLine();
				int progress;
				int progressMax;
				if (useAlternativeStaggerMashProgressDisplayUse) {
					progress = player.pawn.currentAnimDuration();
					progressMax = mashMax - 4
								+ player.pawn.thisIsMinusOneIfEnteredHitstunWithoutHitstop()
								- mashedAmountPrev;
				} else {
					progress = mashedAmountPrev + player.pawn.currentAnimDuration();
					progressMax = mashMax - 4
								+ player.pawn.thisIsMinusOneIfEnteredHitstunWithoutHitstop();
				}
				sprintf_s(strbuf, "%d/%d", progress > progressMax ? progressMax : progress, progressMax);
				ImGui::TextUnformatted(strbuf);
				
				_zerohspacing
				ImGui::SameLine();
				
				static std::string stunmashSetting;
				if (stunmashSetting.empty()) {
					stunmashSetting += settings.getOtherUIName(&settings.useAlternativeStaggerMashProgressDisplay);
					stunmashSetting += '\n';
					stunmashSetting += settings.getOtherUIDescription(&settings.useAlternativeStaggerMashProgressDisplay);
				}
				
				searchFieldTitle(settings.getOtherUINameWithLength(&settings.useAlternativeStaggerMashProgressDisplay));
				searchTooltip(settings.getOtherUIDescriptionWithLength(&settings.useAlternativeStaggerMashProgressDisplay));
				
				ImGui::SetCursorPosY(cursorY);
				bool useAlternativeStaggerMashProgressDisplay = settings.useAlternativeStaggerMashProgressDisplay;
				if (ImGui::Checkbox("##useAlternativeStaggerMashProgressDisplay", &useAlternativeStaggerMashProgressDisplay)) {
					settings.useAlternativeStaggerMashProgressDisplay = useAlternativeStaggerMashProgressDisplay;
					needWriteSettings = true;
				}
				AddTooltip(stunmashSetting.c_str());
				zerohspacing
				
				textUnformattedColored(YELLOW_COLOR, searchFieldTitle("Progress Now: "));
				AddTooltip(searchTooltip("The progress 'now' includes both the current duration of the stagger animation, as it is on THIS frame,"
					" and how much you've mashed so far, including THIS frame's delta-progress.\n"
					" The value that the game checks for whether to decide if you should enter the stagger recovery animation or not,"
					" is the value of 'Progress Previous'.\n"
					" Note that stagger animation does not progress during hitstop, so it is possible to start mashing before the stagger animation"
					" goes past its first frame."));
				ImGui::SameLine();
				if (useAlternativeStaggerMashProgressDisplayUse) {
					progress = player.pawn.currentAnimDuration();
					progressMax = mashMax - 4
								+ player.pawn.thisIsMinusOneIfEnteredHitstunWithoutHitstop()
								- mashedAmount;
				} else {
					progress = mashedAmount + player.pawn.currentAnimDuration();
					progressMax = mashMax - 4
								+ player.pawn.thisIsMinusOneIfEnteredHitstunWithoutHitstop();
				}
				sprintf_s(strbuf, "%d/%d", progress > progressMax ? progressMax : progress, progressMax);
				ImGui::TextUnformatted(strbuf);
				
				textUnformattedColored(YELLOW_COLOR, searchFieldTitle("Started Recovery: "));
				AddTooltip(searchTooltip("Has the 4f stagger recovery animation started?"));
				ImGui::SameLine();
				ImGui::TextUnformatted(player.pawn.bbscrvar() ? "Yes" : "No");
				
				textUnformattedColored(YELLOW_COLOR, searchFieldTitle("Recovery Animation: "));
				ImGui::SameLine();
				sprintf_s(strbuf, "%d/4", !player.pawn.bbscrvar() ? 0 : player.pawn.bbscrvar2() - 1);
				ImGui::TextUnformatted(strbuf);
				
				drawGGIcon(scaleGGIconToHeight(tipsIcon, 14.F));
				AddTooltip(searchTooltip("Every frame that a PKSHD button is pressed, Progress and Mashed increase by 3."
					" Progress also increases by an extra 1 each non-hitstop/superfreeze/RC-slow-eaten frame,"
					" and that means, if the attack applied hitstop to you, it is possible to start mashing before the stagger"
					" animation goes past its first frame. After hitstop is over, progress will increase by 4 on every frame you"
					" pressed a button, and by 1 on every frame you didn't."
					" You can't increase Mashed by more than half the original stagger duration."
					" That means there's a minimum amount of stagger animation you have to play,"
					" and that amount can be almost doubled by RC slowdown."
					" Progress goes up to Stagger Duration - 4 - 1 (the - 1 is applied only if stagger was entered into without hitstop)."
					" When it reaches that, you enter a 4f stagger recovery that cannot be sped up by mash and has fixed length.\n"
					"The mash cannot be buffered. Starting to mash before stagger begins does not start it at a reduced value."
					" Pressing multiple buttons on the same frame does not yield any extra bonuses than pressing one"
					" button on that frame."));
				
				_zerohspacing
			}
			
			ImGui::End();
			ImGui::PopID();
		}
	}
	popSearchStack();
	searchCollapsibleSection("Framebar Help");
	if (showFramebarHelp || searching) {
		framebarHelpWindow();
	}
	popSearchStack();
	searchCollapsibleSection("Hitboxes Help");
	if (showBoxesHelp || searching) {
		hitboxesHelpWindow();
	}
	popSearchStack();
	searchCollapsibleSection("Frame Advantage Help");
	if (showFrameAdvTooltip || searching) {
		ImGui::SetNextWindowSize({ 500.F, 0.F }, ImGuiCond_FirstUseEver);
		if (searching) {
			ImGui::SetNextWindowPos({ 100000.F, 100000.F }, ImGuiCond_Always);
		}
		ImGui::Begin(searching ? "search_frame" : "Frame Advantage Help", &showFrameAdvTooltip, searching ? ImGuiWindowFlags_NoSavedSettings : 0);
		ImGui::PushTextWrapPos(0.F);
		searchFieldTitle("Help Contents");
		ImGui::TextUnformatted(searchTooltip(
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
			"The other value (not in ()) means 'air' frame advantage"
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
			" then you may untick the 'Settings - General Settings - Frame Advantage: Don't Use Pre-Blockstun Time' checkbox"
			" (called frameAdvantage_dontUsePreBlockstunTime in the INI file)."));
		ImGui::PopTextWrapPos();
		ImGui::End();
	}
	popSearchStack();
	searchCollapsibleSection("Startup Field's Help");
	if (showStartupTooltip || searching) {
		ImGui::SetNextWindowSize({ 500.F, 0.F }, ImGuiCond_FirstUseEver);
		if (searching) {
			ImGui::SetNextWindowPos({ 100000.F, 100000.F }, ImGuiCond_Always);
		}
		ImGui::Begin(searching ? "search_startup" : "'Startup' Field Help", &showStartupTooltip, searching ? ImGuiWindowFlags_NoSavedSettings : 0);
		ImGui::PushTextWrapPos(0.F);
		searchFieldTitle("Help Contents");
		ImGui::TextUnformatted(searchTooltip(
			"The startup of the last performed move. The last startup frame is also an active frame.\n"
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
			"6) Baiken cancelling Azami into another Azami or the follow-ups, causes them to be displayed in addition to what happened"
			" before, over a + sign;\n"
			"7) Some other moves may get combined with the ones they were performed from as well, using the + sign."));
		ImGui::PopTextWrapPos();
		ImGui::End();
	}
	popSearchStack();
	searchCollapsibleSection("Active Field's Help");
	if (showActiveTooltip || searching) {
		ImGui::SetNextWindowSize({ 500.F, 0.F }, ImGuiCond_FirstUseEver);
		if (searching) {
			ImGui::SetNextWindowPos({ 100000.F, 100000.F }, ImGuiCond_Always);
		}
		ImGui::Begin(searching ? "search_active" : "'Active' Field Help", &showActiveTooltip, searching ? ImGuiWindowFlags_NoSavedSettings : 0);
		searchFieldTitle("Help Contents");
		ImGui::PushTextWrapPos(0.F);
		ImGui::TextUnformatted(searchTooltip(
			"Number of active frames in the last performed move.\n"
			"\n"
			"Numbers in (), when surrounded by other numbers, mean non-active frames inbetween active frames."
			" So, for example, 1(2)3 would mean you were active for 1 frame, then were not active for 2 frames, then were active again for 3.\n"
			"\n"
			"Numbers separated by a , symbol mean active frames of separate distinct hits, between which there is no gap of non-active frames."
			" For example, 1,4 would mean that the move is active for 5 frames, and the first frame is hit one, while frames 2-5 are hit two."
			" The attack need not actually land those hits, and some moves may be limited by the max number of hits they can deal, which means"
			" the displayed number of hits might not represent the actual number of hits dealt.\n"
			"\n"
			"(max hits X) may be displayed next to active frames and shows the maximum number of hits the attack can deal."
			" If the attack has projectiles which are also limited by the number of hits, then there's a max hit number limit conflict,"
			" and no information about max hits is displayed.\n"
			"\n"
			"Sometimes, when the number of hits is too great, an alternative representation of active frames will be displayed over a / sign."
			" For example: 13 / 1,1,1,1,1,1,1,1,1,1,1,1,1. Means there're 13 active frames, and over the /, each individual hit's active frames"
			" are shown.\n"
			"\n"
			"If the move spawned one or more projectiles, and the hits of projectiles overlap with each other or with the player's hits, then the"
			" individual hits' information is discarded in only those spans that overlap, and those spans get combined and shown as one hit. For example,"
			" Sol DI Ground Viper spawns vertical pillars of fire in its path, as Sol himself is hitting from below. The hits of the pillars are"
			" out of sync with Sol's hits and so everything is displayed as just one number representing the total duration of all active frames.\n"
			"\n"
			"If the move spawned one or more projectiles, or Eddie, and that projectile entered hitstop due to the opponent blocking it or"
			" getting hit by it, then the displayed number of active frames may be larger than it is on whiff, because the hitstop gets added"
			" to it. When both the player and the projectile enter hitstop, like with Axl Benten, this does not happen and active frames display normally.\n"
			"\n"
			"If, while the move was happening, some projectile unrelated to the move had active frames, those are not included in the move's"
			" active frames.\n"
			"\n"
			"If active frames start during superfreeze, the active frames will be 1 greater than real frames to include the frame that happened during"
			" the superfreeze. For example, a move has superfreeze startup 1"
			" (meaning superfreeze starts in 1 frame), +0 startup after superfreeze (which means that it starts during the superfreeze),"
			" and 2 active frames after the superfreeze. The displayed result for active frames will be: 3. If we don't do this, Venom Red Hail hit up close"
			" will display 0 active frames during superfreeze or on the frame after it."));
		ImGui::PopTextWrapPos();
		ImGui::End();
	}
	popSearchStack();
	searchCollapsibleSection("Total Field's Help");
	if (showTotalTooltip || searching) {
		ImGui::SetNextWindowSize({ 500.F, 0.F }, ImGuiCond_FirstUseEver);
		if (searching) {
			ImGui::SetNextWindowPos({ 100000.F, 100000.F }, ImGuiCond_Always);
		}
		ImGui::Begin(searching ? "search_total" : "'Total' Field Help", &showTotalTooltip, searching ? ImGuiWindowFlags_NoSavedSettings : 0);
		searchFieldTitle("Help Contents");
		ImGui::PushTextWrapPos(0.F);
		ImGui::TextUnformatted(searchTooltip(
			"Total number of frames in the last performed move during which you've been unable to act.\n"
			"\n"
			"If the move spawned a projectiled that lasted beyond the boundaries of the move, this field will display"
			" only the amount of frames it took to recover, from the start of the move."
			" So for example, if a move's startup is 1, it creates a projectile that lasts 100 frames and recovers instantly,"
			" on frame 2, then its total frames will be 1, its startup will be 1 and its actives will be 100.\n"
			"\n"
			"If you performed an air move that has landing recovery, the"
			" landing recovery is included in the display as '+X landing'.\n"
			"\n"
			"If you performed an air move and recovered in the air, then the time you spent in the air idle is not included"
			" in the recovery or 'Total' frames. Even if such move had landing recovery, it will display only the frames,"
			" during which you were 'busy', will not include the idle time spent in the air, and will add a '+X landing'"
			" to the recovery.\n"
			"\n"
			"If you performed an air normal or similar air move without landing recovery, and it got cancelled by"
			" landing, normally there's 1 frame upon landing during which normals can't be used but blocking is possible."
			" This frame is not included in the total frames as it is not considered part of the move.\n"
			"\n"
			"If the move recovery lets you attack first and then some times passes and then it lets you block, or vice versa"
			" the display will say either 'X can't block+Y can't attack' or 'X can't attack+Y can't block'. In this case"
			" the first part is the number of frames during which you were unable to block/attack and the second part is"
			" the number of frames during which you were unable to attack/block.\n"
			"\n"
			"If the move was jump cancelled, the prejump frames and the jump are not included in neither the recovery nor 'Total'.\n"
			"\n"
			"If the move started up during superfreeze, the startup+active+recovery will be = total+1 (see tooltip of 'Active')."));
		ImGui::PopTextWrapPos();
		ImGui::End();
	}
	popSearchStack();
	searchCollapsibleSection("Invul Help");
	if (showInvulTooltip || searching) {
		ImGui::SetNextWindowSize({ 500.F, 0.F }, ImGuiCond_FirstUseEver);
		if (searching) {
			ImGui::SetNextWindowPos({ 100000.F, 100000.F }, ImGuiCond_Always);
		}
		ImGui::Begin(searching ? "search_invul" : "Invul Help", &showInvulTooltip, searching ? ImGuiWindowFlags_NoSavedSettings : 0);
		searchFieldTitle("Help Contents");
		ImGui::PushTextWrapPos(0.F);
		ImGui::TextUnformatted(searchTooltip(
			"Strike invul: invulnerable to strike and projectiles.\n"
			"Throw invul: invulnerable to throws.\n"
			"Low profile: low profiles first active frame of Ky f.S"
			" (read the bottom for how to configure maximum height that determines what 'low profile' is).\n"
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
			"*) Slayer Undertow.\n"
			"NOTE: If the super armor properties say it can armor unblockables, but does not mention overdrives, then that means it can't"
			" armor overdrive unblockables such as Kum Enlightened 3000 Palm Strike max charge.\n"
			"Some moves such as Kum max charge Falcon Dive cause guard crush and are not \"unblockables\".\n\n"
			"Low profile invul can be configured using Settings - General Settings - Low Profile Cut-Off Height"));
		ImGui::End();
	}
	popSearchStack();
	if (searching) {
		ImGui::PopID();
	}
}

// Runs on the graphics thread
void UI::onEndScene(IDirect3DDevice9* device, void* drawData, IDirect3DTexture9* iconTexture) {
	if (!imguiInitialized || gifMode.modDisabled || !drawData) {
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
	if (imguiInitialized || !visible && !needShowFramebarCached || !keyboard.thisProcessWindow || gifMode.modDisabled) return;
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
	
	
	framebarWindowDrawDataCopy.resize(sizeof ImDrawListBackup);
	framebarTooltipDrawDataCopy.resize(sizeof ImDrawListBackup);
	new (framebarWindowDrawDataCopy.data()) ImDrawListBackup();
	new (framebarTooltipDrawDataCopy.data()) ImDrawListBackup();
	
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
	if (!ui.timerDisabled) {
		logwrap(fprintf(logfile, "Timerproc called for timerId: %d\n", ui.timerId));
		settings.writeSettings();
	} else {
		logwrap(fprintf(logfile, "Killing timerId: %d\n", ui.timerId));
	}
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
				// remember, killing a timer does not remove its messages from the queue. The only way to ensure it is dead is to set it and wait for its arrival
				SetTimer(NULL, timerId, 1, Timerproc);
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
	ImGui::TextUnformatted(searchFieldTitle(info.uiNameWithLength));
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
		if (ImGui::BeginCombo(idArena.c_str(), searchFieldValue(currentKeyStr, nullptr)))
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
	HelpMarker(searchTooltip(info.uiDescriptionWithLength));
}

// Runs on the main thread. Called hundreds of times each frame
SHORT WINAPI UI::hook_GetKeyState(int nVirtKey) {
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
	int len;
	bool insertedPercentage = false;
	if (numAfterPoint == 0) {
		if (percentage) {
			len = sprintf_s(printdecimalbuf, "%d%c", num, '%');
		} else {
			len = sprintf_s(printdecimalbuf, "%d", num);
		}
	} else {
		if (numAfterPoint < 0 || numAfterPoint > 99) {
			*printdecimalbuf = '\0';
			return printdecimalbuf;
		}
		sprintf_s(fmtbuf + 5, sizeof fmtbuf - 5, "%dd", numAfterPoint);
		if (num >= 0) {
			len = sprintf_s(printdecimalbuf, fmtbuf, num / divideBy, num % divideBy);
		} else {
			num = -num;
			len = sprintf_s(printdecimalbuf + 1, sizeof printdecimalbuf - 1, fmtbuf, num / divideBy, num % divideBy);
			if (len != -1) ++len;
			*printdecimalbuf = '-';
		}
		if (percentage) {
			if (len != -1 && len + 1 < sizeof printdecimalbuf) {
				printdecimalbuf[len] = '%';
				printdecimalbuf[len + 1] = '\0';
				insertedPercentage = true;
			}
		}
	}
	if (len == -1) {
		*printdecimalbuf = '\0';
		return printdecimalbuf;
	}
	int absPadding = padding;
	if (absPadding < 0) absPadding = -absPadding;
	if (len < absPadding) {
		int padSize = absPadding - len;
		if (insertedPercentage) ++len;
		if (((int)(sizeof printdecimalbuf) - len - 1) < padSize) {
			padSize = ((int)(sizeof printdecimalbuf) - len - 1);
		}
		if (padSize == 0) {
			return printdecimalbuf;
		}
		if (padding > 0) {
			memmove(printdecimalbuf + padSize, printdecimalbuf, len);
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
		+ (sizeof (CustomImDrawList*) + sizeof CustomImDrawList) * oldData->CmdListsCount;
	
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
		
		const ImDrawList* oldDrawList = oldData->CmdLists[i];
		CustomImDrawList* newCmdList = (CustomImDrawList*)p;
		
		newCmdList->CmdBuffer.Size = oldDrawList->CmdBuffer.Size;
		newCmdList->IdxBuffer.Size = oldDrawList->IdxBuffer.Size;
		newCmdList->VtxBuffer.Size = oldDrawList->VtxBuffer.Size;
		p += sizeof CustomImDrawList;
		
		newCmdList->CmdBuffer.Data = (ImDrawCmd*)p;
		size_t cmdsSize = oldDrawList->CmdBuffer.Size * sizeof ImDrawCmd;
		memcpy(p, oldDrawList->CmdBuffer.Data, cmdsSize);
		p += cmdsSize;
		
		newCmdList->IdxBuffer.Data = (ImDrawIdx*)p;
		size_t idxSize = oldDrawList->IdxBuffer.Size * sizeof ImDrawIdx;
		memcpy(p, oldDrawList->IdxBuffer.Data, idxSize);
		p += idxSize;
		
		newCmdList->VtxBuffer.Data = (ImDrawVert*)p;
		size_t vtxSize = oldDrawList->VtxBuffer.Size * sizeof ImDrawVert;
		memcpy(p, oldDrawList->VtxBuffer.Data, vtxSize);
		p += vtxSize;
	}
	
}

void makeRenderDataFromDrawLists(std::vector<BYTE>& destination, const ImDrawData* referenceDrawData, ImDrawListBackup** drawLists, int drawListsCount) {
	
	if (!referenceDrawData->Valid) return;
	
	size_t requiredSize = sizeof ImDrawData
		+ (sizeof (CustomImDrawList*) + sizeof CustomImDrawList) * drawListsCount;
	
	int totalIdxCount = 0;
	int totalVtxCount = 0;
	
	for (int i = 0; i < drawListsCount; ++i) {
		const ImDrawListBackup* cmdList = drawLists[i];
		totalIdxCount += cmdList->IdxBuffer.size();
		totalVtxCount += cmdList->VtxBuffer.size();
	}
	
	destination.resize(requiredSize);
	BYTE* p = destination.data();
	
	memcpy(p, referenceDrawData, sizeof ImDrawData);
	ImDrawData* newData = (ImDrawData*)p;
	p += sizeof ImDrawData;
	
	newData->CmdListsCount = drawListsCount;
	newData->CmdLists.Size = drawListsCount;
	newData->TotalIdxCount = totalIdxCount;
	newData->TotalVtxCount = totalIdxCount;
	newData->CmdLists.Data = (ImDrawList**)p;
	p += sizeof (CustomImDrawList*) * drawListsCount;
	
	for (int i = 0; i < drawListsCount; ++i) {
		newData->CmdLists.Data[i] = (ImDrawList*)p;
		
		ImDrawListBackup* drawList = drawLists[i];
		CustomImDrawList* newCmdList = (CustomImDrawList*)p;
		
		newCmdList->CmdBuffer.Size = drawList->CmdBuffer.size();
		newCmdList->IdxBuffer.Size = drawList->IdxBuffer.size();
		newCmdList->VtxBuffer.Size = drawList->VtxBuffer.size();
		newCmdList->CmdBuffer.Data = drawList->CmdBuffer.data();
		newCmdList->IdxBuffer.Data = drawList->IdxBuffer.data();
		newCmdList->VtxBuffer.Data = drawList->VtxBuffer.data();
		p += sizeof CustomImDrawList;
	}
	
}

void copyDrawList(ImDrawListBackup& destination, const ImDrawList* drawList) {
	destination.CmdBuffer.resize(drawList->CmdBuffer.Size);
	destination.IdxBuffer.resize(drawList->IdxBuffer.Size);
	destination.VtxBuffer.resize(drawList->VtxBuffer.Size);
	memcpy(destination.CmdBuffer.data(), drawList->CmdBuffer.Data, sizeof ImDrawCmd * drawList->CmdBuffer.Size);
	memcpy(destination.IdxBuffer.data(), drawList->IdxBuffer.Data, sizeof ImDrawIdx * drawList->IdxBuffer.Size);
	memcpy(destination.VtxBuffer.data(), drawList->VtxBuffer.Data, sizeof ImDrawVert * drawList->VtxBuffer.Size);
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
	"Ramlethal Valentine", // 13
	"Sin Kiske",      // 14
	"Elphelt Valentine", // 15
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

void UI::HelpMarkerWithHotkey(const char* desc, const char* descEnd, std::vector<int>& hotkey) {
	ImGui::TextDisabled("(?)");
	if (searching || ImGui::BeginItemTooltip()) {
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		int result = sprintf_s(strbuf, "Hotkey: %s", comborepr(hotkey));
		if (result != -1) {
			ImGui::TextUnformatted(searchTooltip(strbuf, strbuf + result), strbuf + result);
		}
		ImGui::Separator();
		ImGui::TextUnformatted(searchTooltip(desc, descEnd));
		ImGui::PopTextWrapPos();
		if (!searching) ImGui::EndTooltip();
	}
}

void RightAlign(float w) {
	const float rightEdge = ImGui::GetCursorPosX() + ImGui::GetColumnWidth();
	const float posX = (rightEdge - w);
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

static const float inverse_255 = 1.F / 255.F;
// color = 0xRRGGBB
ImVec4 RGBToVec(DWORD color) {
	// they also wrote it as r, g, b, a... just in struct form
	return {
		(float)((color >> 16) & 0xff) * inverse_255,  // red
		(float)((color >> 8) & 0xff) * inverse_255,  // green
		(float)(color & 0xff) * inverse_255,  // blue
		1.F  // alpha
	};
}

// color = 0xAARRGGBB
ImVec4 ARGBToVec(DWORD color) {
	// they also wrote it as r, g, b, a... just in struct form
	return {
		(float)((color >> 16) & 0xff) * inverse_255,  // red
		(float)((color >> 8) & 0xff) * inverse_255,  // green
		(float)(color & 0xff) * inverse_255,  // blue
		(float)(color >> 24) * inverse_255  // alpha
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

bool UI::addImage(HMODULE hModule, WORD resourceId, std::unique_ptr<PngResource>& resource) {
	if (!resource) resource = std::make_unique<PngResource>();
	if (!loadPngResource(hModule, resourceId, *resource)) return false;
	texturePacker.addImage(*resource);
	return true;
}

void outlinedText(ImVec2 pos, const char* text, ImVec4* color, ImVec4* outlineColor) {
	if (!color) color = &WHITE_COLOR;
	if (!outlineColor) outlineColor = &BLACK_COLOR;
	
    ImGui::PushStyleColor(ImGuiCol_Text, *outlineColor);
    
	ImGui::SetCursorPos({ pos.x, pos.y - 1.F });
	ImGui::TextUnformatted(text);
	
	ImGui::SetCursorPos({ pos.x, pos.y + 1.F });
	ImGui::TextUnformatted(text);
	
	ImGui::SetCursorPos({ pos.x - 1.F, pos.y - 1.F });
	ImGui::TextUnformatted(text);
	
	ImGui::SetCursorPos({ pos.x + 1.F, pos.y - 1.F });
	ImGui::TextUnformatted(text);
	
	ImGui::SetCursorPos({ pos.x - 1.F, pos.y + 1.F });
	ImGui::TextUnformatted(text);
	
	ImGui::SetCursorPos({ pos.x + 1.F, pos.y + 1.F });
	ImGui::TextUnformatted(text);
	
    ImGui::PopStyleColor();
	
	ImGui::SetCursorPos({ pos.x, pos.y });
	if (!color) {
		ImGui::TextUnformatted(text);
	} else {
		ImGui::PushStyleColor(ImGuiCol_Text, *color);
		ImGui::TextUnformatted(text);
    	ImGui::PopStyleColor();
	}
}

const PngResource& UI::getPackedFramesTexture() const {
	return *packedTexture;
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

void UI::addFrameArt(HINSTANCE hModule, FrameType frameType, WORD resourceIdBothVersions, std::unique_ptr<PngResource>& resourceBothVersions, StringWithLength description) {
	if (!resourceBothVersions) resourceBothVersions = std::make_unique<PngResource>();
	addImage(hModule, resourceIdBothVersions, resourceBothVersions);
	frameArtNonColorblind[frameType] = frameArtColorblind[frameType] = {
		frameType,
		resourceBothVersions.get(),
		ImVec2{},
		ImVec2{},
		description
	};
}

void UI::addFrameArt(HINSTANCE hModule, FrameType frameType, WORD resourceIdColorblind, std::unique_ptr<PngResource>& resourceColorblind,
                 WORD resourceIdNonColorblind, std::unique_ptr<PngResource>& resourceNonColorblind, StringWithLength description) {
	if (!resourceColorblind) resourceColorblind = std::make_unique<PngResource>();
	if (!resourceNonColorblind) resourceNonColorblind = std::make_unique<PngResource>();
	assert(&resourceIdColorblind != &resourceIdNonColorblind);
	assert(&resourceColorblind != &resourceNonColorblind);
	addImage(hModule, resourceIdColorblind, resourceColorblind);
	addImage(hModule, resourceIdNonColorblind, resourceNonColorblind);
	frameArtColorblind[frameType] = {
		frameType,
		resourceColorblind.get(),
		ImVec2{},
		ImVec2{},
		description
	};
	frameArtNonColorblind[frameType] = {
		frameType,
		resourceNonColorblind.get(),
		ImVec2{},
		ImVec2{},
		description
	};
	
}

void UI::addFrameMarkerArt(HINSTANCE hModule, FrameMarkerType markerType, WORD resourceIdBothVersions, std::unique_ptr<PngResource>& resourceBothVersions) {
	if (!resourceBothVersions) resourceBothVersions = std::make_unique<PngResource>();
	addImage(hModule, resourceIdBothVersions, resourceBothVersions);
	frameMarkerArtNonColorblind[markerType] = frameMarkerArtColorblind[markerType] = {
		markerType,
		resourceBothVersions.get(),
		ImVec2{},
		ImVec2{}
	};
}

void UI::addFrameMarkerArt(HINSTANCE hModule, FrameMarkerType markerType, WORD resourceIdColorblind, std::unique_ptr<PngResource>& resourceColorblind,
                 WORD resourceIdNonColorblind, std::unique_ptr<PngResource>& resourceNonColorblind) {
	if (!resourceColorblind) resourceColorblind = std::make_unique<PngResource>();
	if (!resourceNonColorblind) resourceNonColorblind = std::make_unique<PngResource>();
	assert(&resourceIdColorblind != &resourceIdNonColorblind);
	assert(&resourceColorblind != &resourceNonColorblind);
	addImage(hModule, resourceIdColorblind, resourceColorblind);
	addImage(hModule, resourceIdNonColorblind, resourceNonColorblind);
	frameMarkerArtColorblind[markerType] = {
		markerType,
		resourceColorblind.get(),
		ImVec2{},
		ImVec2{}
	};
	frameMarkerArtNonColorblind[markerType] = {
		markerType,
		resourceNonColorblind.get(),
		ImVec2{},
		ImVec2{}
	};
	
}

void printInputs(char*& buf, size_t& bufSize, UI::InputName** motions, int motionCount, UI::InputName** buttons, int buttonsCount) {
	char* bufOrig = buf;
	int result;
	bool needSpace = false;
	for (int i = 0; i < motionCount; ++i) {
		UI::InputName* desc = motions[i];
		if (desc->type == UI::InputNameType::MULTIWORD_MOTION) {
			result = sprintf_s(buf, bufSize, "%s%s", needSpace ? ", " : "", desc->name);
			advanceBuf
			needSpace = true;
		}
	}
	bool needPlus = false;
	for (int i = 0; i < motionCount; ++i) {
		UI::InputName* desc = motions[i];
		if (desc->type == UI::InputNameType::MOTION) {
			result = sprintf_s(buf, bufSize, "%s%s",
				needSpace
					? ", "
					: needPlus
						? "+" : ""
				, desc->name);
			advanceBuf
			needSpace = false;
			needPlus = true;
		}
	}
	needPlus = false;
	for (int i = 0; i < buttonsCount; ++i) {
		UI::InputName* desc = buttons[i];
		result = sprintf_s(buf, bufSize, "%s%s",
			needPlus
				? "+"
				: (desc->type == UI::InputNameType::MULTIWORD_BUTTON || needSpace) && bufOrig != buf
					? " " : ""
			, desc->name);
		advanceBuf
		needSpace = false;
		needPlus = true;
	}
}

int printInputs(char* buf, size_t bufSize, const InputType* inputs) {
	if (!bufSize) return 0;
	*buf = '\0';
	char* origBuf = buf;
	UI::InputName* motions[16] { nullptr };
	int motionCount = 0;
	UI::InputName* buttons[16] { nullptr };
	int buttonsCount = 0;
	int result;
	for (int i = 0; i < 16; ++i) {
		InputType inputType = inputs[i];
		if (inputType == INPUT_END) {
			break;
		}
		if (inputType == INPUT_BOOLEAN_OR) {
			printInputs(buf, bufSize, motions, motionCount, buttons, buttonsCount);
			result = sprintf_s(buf, bufSize, " or ");
			advanceBuf
			motionCount = 0;
			buttonsCount = 0;
			continue;
		}
		UI::InputName& info = ui.inputNames[inputType];
		if (info.type == UI::InputNameType::MOTION || info.type == UI::InputNameType::MULTIWORD_MOTION) {
			motions[motionCount++] = &info;
		}
		if (info.type == UI::InputNameType::BUTTON || info.type == UI::InputNameType::MULTIWORD_BUTTON) {
			buttons[buttonsCount++] = &info;
		}
	}
	printInputs(buf, bufSize, motions, motionCount, buttons, buttonsCount);
	return buf - origBuf;
}

bool UI::needShowFramebar() const {
	if (settings.showFramebar
			&& (!settings.closingModWindowAlsoHidesFramebar || visible)
			&& !(drawingPostponed && pauseMenuOpen)
			&& !gifMode.gifModeToggleHudOnly && !gifMode.gifModeOn) {
		GameMode mode = game.getGameMode();
		if (mode == GAME_MODE_TRAINING) {
			return settings.showFramebarInTrainingMode;
		} else if (mode == GAME_MODE_REPLAY) {
			return settings.showFramebarInReplayMode;
		} else if (!(mode == GAME_MODE_NETWORK && game.getPlayerSide() != 2)) {  // if (!(you're playing online as a non-observer)) {
			return settings.showFramebarInOtherModes;
		} else {
			return false;
		}
	} else {
		return false;
	}
}

int printCancels(const std::vector<GatlingOrWhiffCancelInfo>& cancels) {
	struct Requirement {
		MoveCondition condition;
		const char* description;
		int index;
		int mask;
	};
	static Requirement requirements[] {
		{ MOVE_CONDITION_REQUIRES_25_TENSION, "requires 25 meter", 0, 0x20 },
		{ MOVE_CONDITION_REQUIRES_50_TENSION, "requires 50 meter", 0, 0x8 },
		{ MOVE_CONDITION_REQUIRES_100_TENSION, "requires 100 meter", 0, 0x10 },
		{ MOVE_CONDITION_IS_TOUCHING_LEFT_SCREEN_EDGE, "must touch left screen edge", 2, 0x10 },
		{ MOVE_CONDITION_IS_TOUCHING_RIGHT_SCREEN_EDGE, "must touch right screen edge", 2, 0x20 },
		{ MOVE_CONDITION_IS_TOUCHING_WALL, "must touch arena's wall", 1, 0x10000000 }
	};
	int counter = 1;
	for (const GatlingOrWhiffCancelInfo& cancel : cancels) {
		char* buf = strbuf;
		size_t bufSize = sizeof strbuf;
		int result;
		result = sprintf_s(buf, bufSize, "%s", cancel.name);
		advanceBuf
		if (cancel.move->inputs[0] != INPUT_END && bufSize >= 2 && !cancel.nameIncludesInputs) {
			if (cancel.replacementInputs) {
				result = sprintf_s(buf + 2, bufSize - 2, "%s", cancel.replacementInputs);
				if (result == -1) result = 0;
			} else {
				result = printInputs(buf + 2, bufSize - 2, cancel.move->inputs);
			}
			if (result) {
				*buf = ' ';
				*(buf + 1) = '(';
			}
			buf += result + 2;
			bufSize -= result + 2;
			if (bufSize >= 2) {
				*buf = ')';
				*(buf + 1) = '\0';
				++buf;
				--bufSize;
			}
		}
		bool isFirstCondition = true;
		bool alreadySaidTheWordRequires = false;
		for (int i = 0; i < _countof(requirements); ++i) {
			Requirement& req = requirements[i];
			if ((cancel.move->conditions[req.index] & req.mask) != 0) {
				result = sprintf_s(buf, bufSize, isFirstCondition ? " (%s" : ", %s", req.description);
				advanceBuf
				isFirstCondition = false;
				alreadySaidTheWordRequires = req.condition == MOVE_CONDITION_REQUIRES_25_TENSION
					|| req.condition == MOVE_CONDITION_REQUIRES_50_TENSION
					|| req.condition == MOVE_CONDITION_REQUIRES_100_TENSION;
			}
		}
		if (cancel.move->subcategory == MOVE_SUBCATEGORY_BURST_OVERDRIVE) {
			if (alreadySaidTheWordRequires) {
				result = sprintf_s(buf, bufSize, "%s", " and burst");
			} else {
				result = sprintf_s(buf, bufSize, isFirstCondition ? " (%s" : ", %s", "requires burst");
			}
			advanceBuf
			isFirstCondition = false;
		}
		if (cancel.move->minimumHeightRequirement) {
			ui.printDecimal(cancel.move->minimumHeightRequirement, 2, 0);
			result = sprintf_s(buf, bufSize, "%sminimum height requirement: %s",
				isFirstCondition ? " (" : ", ",
				printdecimalbuf);
			advanceBuf
			isFirstCondition = false;
		}
		if (!isFirstCondition) {
			result = sprintf_s(buf, bufSize, ")");
			advanceBuf
		}
		if (cancel.bufferTime && cancel.bufferTime != 3) {
			result = sprintf_s(buf, bufSize, " (%df buffer)", cancel.bufferTime);
			advanceBuf
		}
		ImGui::Text("%d) %s;", counter++, strbuf);
	}
	return counter - 1;
}

void UI::hitboxesHelpWindow() {
	ImGui::SetNextWindowSize({ ImGui::GetFontSize() * 35.0f + 16.F, 0.F }, ImGuiCond_FirstUseEver);
	if (searching) {
		ImGui::SetNextWindowPos({ 100000.F, 100000.F }, ImGuiCond_Always);
	}
	ImGui::Begin(searching ? "search_hitboxeshelp" : "Hitboxes Help", &showBoxesHelp, searching ? ImGuiWindowFlags_NoSavedSettings : 0);
	ImGui::PushTextWrapPos(0.F);
	static std::string boxesDesc1;
	if (boxesDesc1.empty()) {
		boxesDesc1 = settings.convertToUiDescription("Boxes are only displayed when \"dontShowBoxes\" is disabled.");
	}
	ImGui::TextUnformatted(boxesDesc1.c_str(), boxesDesc1.c_str() + boxesDesc1.size());
	textUnformattedColored(YELLOW_COLOR, "Box colors and what they mean:");
	ImGui::Separator();
	
	textUnformattedColored(COLOR_HITBOX_IMGUI, "Red: ");
	ImGui::SameLine();
	ImGui::TextUnformatted("Hitboxes.");
	ImGui::TextUnformatted("Strike and projectile non-throw hitboxes are shown in red."
		" Clash-only hitboxes are shown in more transparent red with thinner outline."
		" Hitboxes' fills may never be absent.");
	ImGui::Separator();
	
	textUnformattedColored(COLOR_HURTBOX_IMGUI, "Green: ");
	ImGui::SameLine();
	ImGui::TextUnformatted("Hurtboxes.");
	ImGui::TextUnformatted("Normally hurtboxes display in green."
		" The rules in general are such, that when a hitbox (red) makes contact with hurtbox, a hit occurs.\n"
		" If a hurtbox is displayed fully transparent (i.e. shows outline only), that means strike invulnerability."
		" If a hurtbox is hatched with diagonal lines, that means some form of super armor is active:"
		" it could be parry (Jam 46P), projectile reflection (Zato Drunkard Shade), super armor"
		" (Potemkin Hammerfall), or projectile-only invulnerability (Ky Ride the Lightning),"
		" or direct player attacks-only invulnerability (Ky j.D). This visual hatching effect"
		" can be combined with the hurtbox being not filled in, meaning strike"
		" invulerability + super armor/other.\n"
		"For Jack O's and Bedman's summons the hurtbox's outline may be thin to create less clutter on the screen.");
	ImGui::Separator();
	
	textUnformattedColored(COLOR_HURTBOX_COUNTERHIT_IMGUI, "Light-blue: ");
	ImGui::SameLine();
	ImGui::TextUnformatted("Would-be counterhit hurtboxes.");
	ImGui::TextUnformatted("If your hurtbox is displaying light blue, that means,"
		" should you get hit, you would enter counterhit state. It means that moves"
		" that have light blue hurtbox on recovery are more punishable.\n"
		"If a hurtbox is hatched with diagonal lines, that means some form of super armor is active:"
		" it could be parry (Jam 46P), projectile reflection (Zato Drunkard Shade), super armor"
		" (Potemkin Hammerfall), or projectile-only invulnerability (Ky Ride the Lightning),"
		" or direct player attacks-only invulnerability (Ky j.D).");
	ImGui::Separator();
	
	textUnformattedColored(COLOR_HURTBOX_OLD_IMGUI, "Gray: ");
	ImGui::SameLine();
	ImGui::TextUnformatted("Pre-hit hurtboxes.");
	ImGui::TextUnformatted("When you get hit a gray outline appears on top"
		" of your current hurtbox. This outline represents the previous state of your hurtbox,"
		" before you got hit. Its purpose is to make it easier to see how or why you got hit.");
	static std::string turnOffGrayBoxes;
	if (turnOffGrayBoxes.empty()) {
		turnOffGrayBoxes = settings.convertToUiDescription("The display of gray hurtboxes can be disabled using the"
			" \"neverDisplayGrayHurtboxes\" setting.");
	}
	ImGui::TextUnformatted(turnOffGrayBoxes.c_str(), turnOffGrayBoxes.c_str() + turnOffGrayBoxes.size());
	ImGui::Separator();
	
	textUnformattedColored(COLOR_PUSHBOX_IMGUI, "Yellow: ");
	ImGui::SameLine();
	ImGui::TextUnformatted("Pushboxes.");
	ImGui::TextUnformatted("Each player has a pushbox. When two pushboxes collide,"
		" the players get pushed apart until their pushboxes no longer collide."
		" Pushbox widths also affect throw range - more on that in next section(s).\n"
		"If a pushbox is displayed fully transparent (i.e. shows outline only), that means throw invulnerability."
		" Pushbox outline is always thin.");
	ImGui::Separator();
	
	ImGui::TextUnformatted("+ (Point/cross): ");
	ImGui::SameLine();
	ImGui::TextUnformatted("Origin point.");
	ImGui::TextUnformatted("Each player has an origin point which is shown as"
		" a black-white cross on the ground between their feet. When players jump, the origin"
		" point tracks their location. Origin points play a key role in throw hit detection.");
	ImGui::Separator();
	
	ImGui::TextUnformatted(". (Tiny black & white dot): ");
	ImGui::SameLine();
	ImGui::TextUnformatted("Projectile origin point.");
	ImGui::TextUnformatted("Projectiles whose origin points were deemed to be important enough"
		" to be shown will display them as black and white tiny square points. They visualize"
		" the projectiles' position which may be used in some range or interaction checks.\n"
		"\n"
		"Player center of body or other point. These points may be important in some kind"
		" of interactions, but they're not the origin point, and to distinguish that they're"
		" drawn using the style of projectile origin points so that they look smaller and less important.");
	ImGui::Separator();
	
	textUnformattedColored(COLOR_REJECTION_IMGUI, "Blue: ");
	ImGui::SameLine();
	ImGui::TextUnformatted("(Blue) Rejection boxes.");
	ImGui::TextUnformatted("When a Blitz Shield meets a projectile it displays"
		" a square blue box around the rejecting player, and if the opponent's origin point"
		" is within that box the opponent enters rejected state. The box does not show when"
		" rejecting normal, melee attacks because those cause rejection no matter the distance."
		" Pushboxes and their sizes do not affect the distance check in any way, i.e. only the"
		" X and Y distances between the players' origin points are checked.");
	ImGui::Separator();
	
	if (settings.drawPushboxCheckSeparately) {
		
		textUnformattedColored(COLOR_THROW_PUSHBOX_IMGUI, "Blue: ");
		ImGui::SameLine();
		static std::string throwBoxSeparatePushbox;
		if (throwBoxSeparatePushbox.empty()) {
			throwBoxSeparatePushbox = settings.convertToUiDescription("(Blue) Throw box pushbox check (when \"drawPushboxCheckSeparately\" is checked - the default).");
		}
		ImGui::TextUnformatted(throwBoxSeparatePushbox.c_str(), throwBoxSeparatePushbox.c_str() + throwBoxSeparatePushbox.size());
		ImGui::TextUnformatted(
			"When a player does a throw he displays a throw box which may either be represented as"
			" one blue box, one purple box or one blue and one purple box. Throw boxes are usually"
			" only active for one frame (that's when they display semi-transparent)."
			" This period is so brief throw boxes have to show for a few extra frames, but"
			" during those frames they're no longer active and so they display fully transparent (outline only).\n"
			" The blue part of the box shows the pushbox-checking throw box. It is never limited vertically,"
			" because it only checks the horizontal distance between pushboxes. The rule is if this blue"
			" throw box touches the yellow pushbox of the opponent, the pushbox-checking part of the throw is satisfied.\n"
			"But there may be another part of the throw that checks the X and/or Y of the opponent's origin point (shown in purple)."
			" If such a part of the throw box is present, it must also be satisfied, or else the throw won't connect.");
		ImGui::Separator();
		
		textUnformattedColored(COLOR_THROW_XYORIGIN_IMGUI, "Purple: ");
		ImGui::SameLine();
		static std::string throwBoxSeparateOriginPoint;
		if (throwBoxSeparateOriginPoint.empty()) {
			throwBoxSeparateOriginPoint = settings.convertToUiDescription("Throw box origin point check (when \"drawPushboxCheckSeparately\" is checked - the default).");
		}
		ImGui::TextUnformatted(throwBoxSeparateOriginPoint.c_str(), throwBoxSeparateOriginPoint.c_str() + throwBoxSeparateOriginPoint.size());
		ImGui::TextUnformatted(
			"The purple part of the throw box displays the region where the opponent's origin point must be"
			" in order for this part of the throw check to connect.\n"
			"If there's a pushbox-proximity-checking part of the throw (blue), then satisfying the origin point"
			" check (purple) is not enough - the pushbox check must be passed as well."
			" The pushbox-checking part of the throw displays in blue and is described in the section above.");
		ImGui::Separator();
		
	} else {
		
		textUnformattedColored(COLOR_THROW_IMGUI, "Blue: ");
		ImGui::SameLine();
		static std::string throwBox;
		if (throwBox.empty()) {
			throwBox = settings.convertToUiDescription("(Blue) Throw boxes (when \"drawPushboxCheckSeparately\" = false).");
		}
		ImGui::TextUnformatted(throwBox.c_str(), throwBox.c_str() + throwBox.size());
		ImGui::TextUnformatted(
			"Combined throw box which only checks the opponent's origin point. This box may depend on the width"
			" of the opponent's pushbox, so beware! (Try make your opponent crouch and see your throw box get bigger."
			" An alternative way of displaying throwboxes using the setting mentioned earlier avoid this problem.)\n"
			"Throw boxes are usually only active for one frame (that's when they display semi-transparent)."
			" This period is so brief throw boxes have to show for a few extra frames,"
			" but during those frames they're no longer active and so they display fully transparent (outline only).");
		ImGui::Separator();
		
	}
	
	textUnformattedColored(COLOR_INTERACTION_IMGUI, "White: ");
	ImGui::SameLine();
	ImGui::TextUnformatted("Interaction boxes/circles");
	ImGui::TextUnformatted(
		"Boxes or circles like this are displayed when a move is checking ranges."
		" They may be checking distance to a player's origin point or to their 'center',"
		" depending on the type of move or projectile. All types of displayed interactions will be listed here down below.");
	
	textUnformattedColored(YELLOW_COLOR, "Ky Stun Edge, Charged Stun Edge and Sacred Edge:");
	ImGui::TextUnformatted("The box shows the area in which Ciel's origin point must be in order for the projectile to become Fortified.");
	
	textUnformattedColored(YELLOW_COLOR, "May Beach Ball:");
	static std::string mayBeachBall;
	if (mayBeachBall.empty()) {
		mayBeachBall = settings.convertToUiDescription("The circle shows the range in which May's center of body must be in order to jump on the ball."
			" May's center of body is additionally displayed as a smaller point, instead of like a cross, like her origin point."
			" Now, this may be a bit much, but a white line is also displayed connecting May's center of body point to the ball's"
			" point that is at the center of the circle. This line serves no purpose other than to remind the user that the range"
			" check of the circle is done against the center of body point of May, not her origin point.\n"
			"The display of all this can be disabled with \"dontShowMayInteractionChecks\".");
	}
	ImGui::TextUnformatted(mayBeachBall.c_str());
	
	ImGui::Separator();
	
	textUnformattedColored(YELLOW_COLOR, "Outlines lie within their boxes/on the edge");
	ImGui::TextUnformatted("If a box's outline is thick, it lies within that box's bounds,"
		" meaning that two boxes intersect if either their fills or outlines touch or both. This"
		" is relevant for throwboxes too.\n"
		"If a box's outline is thin, like that of a pushbox or a clash-only hitbox for example,"
		" then that outline lies on the edge of the box. For Jack O's and Bedman's summons"
		" the hurtbox's outline may be thin to create less clutter on the screen.");
	ImGui::Separator();
	
	textUnformattedColored(YELLOW_COLOR, "General notes about throw boxes");
	ImGui::TextUnformatted("If a move (like Riot Stamp) has a throw box as well as hitbox - both the hitbox and the throw boxes must connect.");
	
	ImGui::PopTextWrapPos();
	ImGui::End();
}
	
void UI::framebarHelpWindow() {
	ImGui::SetNextWindowSize({ ImGui::GetFontSize() * 35.0f + 16.F, 0.F }, ImGuiCond_FirstUseEver);
	if (searching) {
		ImGui::SetNextWindowPos({ 100000.F, 100000.F }, ImGuiCond_Always);
	}
	ImGui::Begin(searching ? "search_framebarhelp" : "Framebar Help", &showFramebarHelp, searching ? ImGuiWindowFlags_NoSavedSettings : 0);
	float wordWrapWidth = ImGui::GetContentRegionAvail().x;
	ImGui::PushTextWrapPos(wordWrapWidth);
	
	static bool framebarHelpContentGenerated = false;
	struct FramebarHelpElement {
		const FrameArt* art[2];
		std::vector<StringWithLength*> meanings;
	};
	static std::vector<FramebarHelpElement> framebarHelpContent;
	
	if (!framebarHelpContentGenerated) {
		framebarHelpContentGenerated = true;
		framebarHelpContent.reserve(_countof(frameArtNonColorblind));
		for (int i = 0; i < _countof(frameArtNonColorblind); ++i) {
			FrameArt& art = frameArtNonColorblind[i];
			if (art.type == FT_PROJECTILE
					|| art.type == FT_NONE) {
				continue;
			}
			
			FramebarHelpElement* found = nullptr;
			for (FramebarHelpElement& elem : framebarHelpContent) {
				if (elem.art[0]->resource == art.resource) {
					found = &elem;
					break;
				}
			}
			
			if (!found) {
				framebarHelpContent.emplace_back();
				FramebarHelpElement& newHelp = framebarHelpContent.back();
				newHelp.art[0] = &art;
				newHelp.art[1] = &frameArtColorblind[i];
				newHelp.meanings.emplace_back(&art.description);
			} else {
				found->meanings.emplace_back(&art.description);
			}
		}
	}
	searchFieldTitle("A frame type's description");
	int index = settings.useColorblindHelp ? 1 : 0;
	bool needSeparator = false;
	for (const FramebarHelpElement& elem : framebarHelpContent) {
		const FrameArt* art = elem.art[index];
		
		if (needSeparator) {
			ImGui::Separator();
		}
		
		ImGui::Image((ImTextureID)TEXID_FRAMES,
			{ frameWidthOriginal, frameHeightOriginal },
			art->uvStart,
			art->uvEnd,
			ImVec4(1, 1, 1, 1),
			ImVec4(1, 1, 1, 1));
		
		ImGui::SameLine();
		float cursorX = ImGui::GetCursorPosX();
		
		int count = 1;
		bool theresOnlyOne = elem.meanings.size() == 1;
		for (StringWithLength* description : elem.meanings) {
			if (count != 1) {
				ImGui::SetCursorPosX(cursorX);
			}
			if (theresOnlyOne) {
				ImGui::TextUnformatted(searchTooltip(*description));
			} else {
				ImGui::Text("%d) %s", count++, searchTooltip(*description));
			}
		}
		
		needSeparator = true;
	}
	
	ImGui::Separator();
	
	const FrameArt* frameArtArray = settings.useColorblindHelp ? frameArtColorblind : frameArtNonColorblind;
	ImVec2 cursorPos = ImGui::GetCursorPos();
	ImGui::Image((ImTextureID)TEXID_FRAMES,
		{ frameWidthOriginal, frameHeightOriginal },
		frameArtArray[FT_STARTUP].uvStart,
		frameArtArray[FT_STARTUP].uvEnd,
		ImVec4(1, 1, 1, 1),
		ImVec4(1, 1, 1, 1));
	ImGui::SetCursorPos({
		cursorPos.x + frameWidthOriginal * 0.5F
			+ 1.F,  // include the white border
		cursorPos.y
			+ 1.F  // include the white border
	});
	ImGui::Image((ImTextureID)TEXID_FRAMES,
		{ frameWidthOriginal * 0.5F, frameHeightOriginal },
		{
			(frameArtArray[FT_ACTIVE].uvStart.x + frameArtArray[FT_ACTIVE].uvEnd.x) * 0.5F,
			frameArtArray[FT_ACTIVE].uvStart.y
		},
		frameArtArray[FT_ACTIVE].uvEnd,
		ImVec4(1, 1, 1, 1));
	
	ImGui::SameLine();
	ImGui::TextUnformatted(searchTooltip("A half-filled active frame means an attack's startup or active frame which first begins duing"
		" a superfreeze."));
	
	ImGui::Separator();
	
	const FrameMarkerArt* frameMarkerArtArray = settings.useColorblindHelp ? frameMarkerArtColorblind : frameMarkerArtNonColorblind;
	float frameMarkerVHeight = frameMarkerHeightOriginal / (float)strikeInvulFrame->height
		* (strikeInvulFrame->vEnd - strikeInvulFrame->vStart);
	
	struct MarkerHelpInfo {
		FrameMarkerType type;
		bool isOnTheBottom;
		StringWithLength description;
	};
	MarkerHelpInfo markerHelps[] {
		{ MARKER_TYPE_STRIKE_INVUL, false, "Strike invulnerability." },
		{ MARKER_TYPE_THROW_INVUL, true, "Throw invulnerability." },
		{ MARKER_TYPE_SUPER_ARMOR, false, "Super armor, parry, azami,"
			" projectile-only invulnerability, reflect or flick, or a combination of those." },
		{ MARKER_TYPE_SUPER_ARMOR_FULL, false, "Red Blitz charge super armor or air Blitz super armor."
			" Or Faust 5D homerun reflect frames." }
	};
	
	for (int i = 0; i < _countof(markerHelps); ++i) {
		MarkerHelpInfo& info = markerHelps[i];
		float cursorY = ImGui::GetCursorPosY();
		ImGui::SetCursorPosY(cursorY + (ImGui::GetTextLineHeightWithSpacing() - frameMarkerHeightOriginal) * 0.5F);
		ImGui::Image((ImTextureID)TEXID_FRAMES,
			{ frameMarkerWidthOriginal, frameMarkerHeightOriginal },
			!info.isOnTheBottom
				? frameMarkerArtArray[info.type].uvStart
				: ImVec2{
					frameMarkerArtArray[info.type].uvStart.x,
					frameMarkerArtArray[info.type].uvEnd.y - frameMarkerVHeight
				},
			!info.isOnTheBottom
				? ImVec2{
					frameMarkerArtArray[info.type].uvEnd.x,
					frameMarkerArtArray[info.type].uvStart.y + frameMarkerVHeight
				}
				: frameMarkerArtArray[info.type].uvEnd
			);
		ImGui::SameLine();
		ImGui::SetCursorPosY(cursorY);
		ImGui::TextUnformatted(searchTooltip(info.description));
	}
	
	ImGui::Separator();
	
	ImGui::Image((ImTextureID)TEXID_FRAMES,
		{ frameWidthOriginal, firstFrameHeight },
		{ firstFrame->uStart, firstFrame->vStart },
		{ firstFrame->uEnd, firstFrame->vEnd });
	ImGui::SameLine();
	ImGui::TextUnformatted(searchTooltip("A first frame, denoting the start of a new animation."
		" If the animation didn't change, may mean transition to some new state in the animation."
		" For blockstun and hitstun may mean leaving hitstop or re-entering hitstun/blockstun/hitstop.\n"
		"Some animation changes are intentionally not shown."));
	
	ImGui::Separator();
	
	ImGui::Image((ImTextureID)TEXID_FRAMES,
		{ frameWidthOriginal, firstFrameHeight },
		{ hitConnectedFrame->uStart, hitConnectedFrame->vStart },
		{ hitConnectedFrame->uEnd, hitConnectedFrame->vEnd });
	ImGui::SameLine();
	ImGui::TextUnformatted(searchTooltip("A frame on which an attack connected and a hit occured."
		" In order to improve visibility, this white outline is instead made black when drawn on top"
		" of frames that are non-dark yellow"));
	
	ImGui::Separator();
	
	static std::string generalFramebarHelp;
	if (generalFramebarHelp.empty()) {
		generalFramebarHelp = settings.convertToUiDescription(
			"Note: due to rollback, framebar cannot function properly in online play and is disabled there.\n"
			"\n"
			"The game runs at 60FPS. If the computer can't handle the game's required processing at the required FPS,"
			" frames do not get skipped. Instead, the game slows down, and every frame is always displayed on the screen without skipping."
			" If you see frames skipping in online play, that's due to rollback frames, not the game compensating for poor CPU or graphical performance."
			" If the performance is low, the game just slows down, even in online. Point is, every frame some game logic runs and all time and everything"
			" is measured in frames.\n"
			"\n"
			"The framebar shows individual frames that record some information of what happened on that frame and display it using color,"
			" or extra graphics around the frames.\n"
			"\n"
			"When both players stand still, the framebar doesn't display any new information."
			" The framebar only advances forward when either player is not 'idle', or is strike/throw/etc invul,"
			" or a projectile is present on the arena."
			" \"considerRunAndWalkNonIdle\", \"considerCrouchNonIdle\", \"considerDummyPlaybackNonIdle\", \"considerIdleInvulIdle\","
			" \"considerKnockdownWakeupAndAirtechIdle\" settings affect"
			" whether a player is considered 'idle'.\n"
			"\n"
			"A framebar displays 80 frames maximum. When the framebar gets full, it circles back and the old frames are tinted darker. A white vertical line"
			" denotes the current position in the framebar. All sub-framebars always have the same position of the white vertical line.\n"
			"\n"
			"Hitstop in general is skipped, unless a projectile or a blocking Baiken (see \"ignoreHitstopForBlockingBaiken\")"
			" is present. The \"neverIgnoreHitstop\" setting can be used to not skip hitstop.\n"
			"\n"
			"Superfreeze is always skipped in the framebar.\n"
			"\n"
			"When a projectile appears, a new sub-framebar is created for it. It may not be visible if it doesn't fit in the 'framebar window'."
			" If there are more sub-framebars than the framebar window can show, a vertical scrollbar will appear on its right side that can be used"
			" to scroll it with the mouse wheel or by dragging the scrollbar with the mouse."
			" The framebar window has an invisible border which can be resized using the mouse. Try to find where the border is by following towards the top"
			" from the 'Player 1' title text. You can resize the framebar so that it is large enough to fit all its sub-framebars in it without having to scroll."
			" Framebar can be dragged by clicking anywhere on it with the mouse, holding and moving.\n"
			"Similar projectiles may be combined into single sub-framebars. The \"combineProjectileFramebarsWhenPossible\" and \"eachProjectileOnSeparateFramebar\""
			" settings control how projectile sub-framebars are combined.\n"
			"\n"
			"You can hover your mouse over individual frames in the framebar to reveal additional information about them.\n"
			"\n"
			"Numbers on the framebar mean the number of same-type frames in a row, until a 'first frame' marker (^img0;) is encountered."
			" The numbers do not reset (do not restart counting) when a \"next hit\" active frame (^img1;) is encountered or on frames"
			" where an attack connected with an opponent."
			" The \"considerSimilarFrameTypesSameForFrameCounts\" and \"considerSimilarIdleFramesSameForFrameCounts\" settings"
			" may help narrow the range of situations where the numbers get reset (restart counting)."
			" By default \"considerSimilarFrameTypesSameForFrameCounts\" is set to true and \"considerSimilarIdleFramesSameForFrameCounts\" is set to false,"
			" meaning similar frame types of startup or recovery are counted as one range of frames, but breaks in ranges of idle frames are counted"
			"as different frame ranges.\n"
			"\n"
			"When the number of frames is double or triple digit"
			" and does not fit, it may be broken up into 1-2 digit on one side + 1-2 digits on the other side. The way you should"
			" read this is as one whole number: the pieces on the right are the higher digits, and pieces on the left are the lower ones.\n"
			"\n"
			"When using \"skipGrabsInFramebar\", which is the default, white stitches will be displayed on the framebar in places where"
			" a grab or a super animation was skipped.\n"
			"\n"
			"Inside a frame's tooltip that appears on mouse hover there may be a display of input history."
			" If that frame follows some skipped hitstop, superfreeze, grab/super or roundstart, there may be more than"
			" one frame of inputs. In this case, frame numbers are displayed next to rows of inputs."
			" The frame number next to a row means the duration of that input row in frames."
			" The row shows directions and buttons in either normal or dark color. If the color is dark,"
			" that means that that direction was not changed by that row of input, or that button was not pressed"
			" on that row of input. If the color is not dark, then for a direction that means it was changed"
			" by that row of input from some previous one that was different, and for a button that means it was"
			" pressed on that row of input."
		);
	}
	
	FrameArt* newHitArt;
	if (settings.useColorblindHelp) {
		newHitArt = &frameArtColorblind[FT_ACTIVE_NEW_HIT];
	} else {
		newHitArt = &frameArtNonColorblind[FT_ACTIVE_NEW_HIT];
	}
	imGuiDrawWrappedTextWithIcons_Icon icons[] {
		{
			(ImTextureID)TEXID_FRAMES,
			{ frameWidthOriginal, firstFrameHeight },
			{ firstFrame->uStart, firstFrame->vStart },
			{ firstFrame->uEnd, firstFrame->vEnd }
		},
		{
			(ImTextureID)TEXID_FRAMES,
			{ frameWidthOriginal, frameHeightOriginal },
			newHitArt->uvStart,
			newHitArt->uvEnd
		}
	};
	ImGui::PopTextWrapPos();
	imGuiDrawWrappedTextWithIcons(searchTooltipStr(generalFramebarHelp),
		generalFramebarHelp.c_str() + generalFramebarHelp.size(),
		wordWrapWidth,
		icons,
		_countof(icons));
	ImGui::End();
}

// This is from imgui_draw.cpp::ImFont::CalcWordWrapPositionA, modified version. Added icons
const char* imGuiDrawWrappedTextWithIcons_CalcWordWrapPositionA(float scale,
		const char* text,
		const char* text_end,
		float wrap_width,
		const imGuiDrawWrappedTextWithIcons_Icon* icons,
		int iconsCount,
		const imGuiDrawWrappedTextWithIcons_Icon** icon)
{
	if (icon) *icon = nullptr;
	ImFont* font = ImGui::GetFont();
    // For references, possible wrap point marked with ^
    //  "aaa bbb, ccc,ddd. eee   fff. ggg!"
    //      ^    ^    ^   ^   ^__    ^    ^

    // List of hardcoded separators: .,;!?'"

    // Skip extra blanks after a line returns (that includes not counting them in width computation)
    // e.g. "Hello    world" --> "Hello" "World"

    // Cut words that cannot possibly fit within one line.
    // e.g.: "The tropical fish" with ~5 characters worth of width --> "The tr" "opical" "fish"
    float line_width = 0.0f;
    float word_width = 0.0f;
    float blank_width = 0.0f;
    wrap_width /= scale; // We work with unscaled widths to avoid scaling every characters

    const char* word_end = text;
    const char* prev_word_end = NULL;
    bool inside_word = true;

    const char* s = text;
    IM_ASSERT(text_end != NULL);
    while (s < text_end)
    {
        unsigned int c = (unsigned int)*s;
        const char* next_s;
        if (c < 0x80)
            next_s = s + 1;
        else
            next_s = s + ImTextCharFromUtf8(&c, s, text_end);

        if (c < 32)
        {
            if (c == '\n')
            {
                return s;
            }
            if (c == '\r')
            {
                s = next_s;
                continue;
            }
        }

        float char_width = ((int)c < font->IndexAdvanceX.Size ? font->IndexAdvanceX.Data[c] : font->FallbackAdvanceX);
        if (ImCharIsBlankW(c))
        {
            if (inside_word)
            {
                line_width += blank_width;
                blank_width = 0.0f;
                word_end = s;
            }
            blank_width += char_width;
            inside_word = false;
        }
        else
        {
        	if (c == '^' && strncmp(s + 1, "img", 3) == 0) {
        		int digitValue = 0;
        		const char* digitPtr = s + 4;
        		while (digitPtr < text_end) {
        			char digit = *digitPtr;
        			if (digit >= '0' && digit <= '9') {
        				digitValue = digitValue * 10 + digit - '0';
        			} else {
        				if (digitPtr != s + 4 && digit == ';' && digitValue < iconsCount) {
        					const imGuiDrawWrappedTextWithIcons_Icon& iconRef = icons[digitValue];
        					if (icon) *icon = &iconRef;
        					char_width = iconRef.size.x;
        					next_s = digitPtr + 1;
        					c = 'A';
        					return s;
        				}
        				break;
        			}
        			++digitPtr;
        		}
        	}
            word_width += char_width;
            if (inside_word)
            {
                word_end = next_s;
            }
            else
            {
                prev_word_end = word_end;
                line_width += word_width + blank_width;
                word_width = blank_width = 0.0f;
            }

            // Allow wrapping after punctuation.
            inside_word = (c != '.' && c != ',' && c != ';' && c != '!' && c != '?' && c != '\"');
        }

        // We ignore blank width at the end of the line (they can be skipped)
        if (line_width + word_width > wrap_width)
        {
            // Words that cannot possibly fit within an entire line will be cut anywhere.
            if (word_width < wrap_width)
                s = prev_word_end ? prev_word_end : word_end;
            break;
        }

        s = next_s;
    }
	
    // Wrap_width is too small to fit anything. Force displaying 1 character to minimize the height discontinuity.
    // +1 may not be a character start point in UTF-8 but it's ok because caller loops use (text >= word_wrap_eol).
    if (s == text && text < text_end)
        return s + 1;
    return s;
}

// You must call PopTextWrapPos if you set wrap width with PushTextWrapPos before calling this function
void imGuiDrawWrappedTextWithIcons(const char* textStart,
		const char* textEnd,
		float wrap_width,
		const imGuiDrawWrappedTextWithIcons_Icon* icons,
		int iconsCount) {
	if (textEnd == nullptr) textEnd = textStart + strlen(textStart);
	const char* lineStart = textStart;
	
	ImFont* font = ImGui::GetFont();
	float lineHeight = ImGui::GetTextLineHeight();
	float cursorX = ImGui::GetCursorPosX();
	ImVec2 windowPos = ImGui::GetWindowPos();
	float wordWrapWidthUse = wrap_width - cursorX;
	float wordWrapWidthUseOrig = wordWrapWidthUse;
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	float scrollY = ImGui::GetScrollY();
	float textPosOrigX = cursorX + windowPos.x;
	ImVec2 currentTextPos{
		textPosOrigX,
		ImGui::GetCursorPosY() + windowPos.y - scrollY
	};
	float yStart = currentTextPos.y;
	const imGuiDrawWrappedTextWithIcons_Icon* icon = nullptr;
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0.F, 0.F });
	char gianttextid[15] = "##gianttext";
	int invisButtonCount = 0;
	bool prevWasIcon = false;
	while (true) {
		const char* lineEnd = imGuiDrawWrappedTextWithIcons_CalcWordWrapPositionA(1.F, lineStart, textEnd, wordWrapWidthUse, icons, iconsCount, &icon);
		if (!icon) {
			drawList->AddText(currentTextPos, -1, lineStart, lineEnd);
			lineStart = lineEnd;
			if (prevWasIcon) {
				ImGui::NewLine();
				wordWrapWidthUse = wordWrapWidthUseOrig;
				currentTextPos.x = textPosOrigX;
				currentTextPos.y = ImGui::GetCursorPosY() + windowPos.y - scrollY;
				yStart = currentTextPos.y;
			} else {
				currentTextPos.y += lineHeight;
			}
			prevWasIcon = false;
		} else {
			if (!prevWasIcon && currentTextPos.y != yStart) {
				sprintf_s(gianttextid + 11, sizeof gianttextid - 11, "%d", invisButtonCount++);
				ImGui::InvisibleButton(gianttextid, { wordWrapWidthUseOrig, currentTextPos.y - yStart }, ImGuiButtonFlags_None);
			}
			bool needSameLine = prevWasIcon;
			if (lineEnd != lineStart) {
				if (needSameLine) ImGui::SameLine();
				ImGui::TextUnformatted(lineStart, lineEnd);
				ImGui::SameLine();
				needSameLine = false;
			}
			if (needSameLine) ImGui::SameLine();
			ImGui::Image(icon->texId,
				icon->size,
				icon->uvStart,
				icon->uvEnd);
			const char* iconEnd = textEnd;
			if (textEnd > lineEnd) iconEnd = (const char*)memchr(lineEnd, ';', textEnd - lineEnd);
			if (iconEnd == nullptr) break;
			if (iconEnd != textEnd) ++iconEnd;
			
			ImGui::SameLine();
			cursorX = ImGui::GetCursorPosX();
			wordWrapWidthUse = wrap_width - cursorX;
			currentTextPos.x = cursorX + windowPos.x;
			prevWasIcon = true;
			
			lineStart = iconEnd;
		}
		bool isFirstLineBreak = true;
		while (lineStart < textEnd && *lineStart <= 32) {
			if (*lineStart == '\n') {
				if (!isFirstLineBreak) {
					currentTextPos.y += lineHeight;
				} else if (icon) {
					wordWrapWidthUse = wordWrapWidthUseOrig;
					currentTextPos.x = textPosOrigX;
					prevWasIcon = false;
					ImGui::NewLine();
					
					currentTextPos.y = ImGui::GetCursorPosY() + windowPos.y - scrollY;
					yStart = currentTextPos.y;
				}
					
				isFirstLineBreak = false;
			}
			++lineStart;
		}
		if (lineStart >= textEnd) break;
	}
	if (currentTextPos.y != yStart) {
		sprintf_s(gianttextid + 11, sizeof gianttextid - 11, "%d", invisButtonCount++);
		ImGui::InvisibleButton(gianttextid, { wordWrapWidthUseOrig, currentTextPos.y - yStart }, ImGuiButtonFlags_None);
	}
	ImGui::PopStyleVar();
}

struct FrameDims {
	float x;
	float width;
};

#define PRINT_INPUT_BUTTON(name, ENUM_NAME) \
	if (row.name) { \
		const InputsIcon& icon = inputsIcon[ENUM_NAME]; \
		drawList->AddImage((ImTextureID)TEXID_GGICON, \
			{ x, y }, \
			{ x + framebarTooltipInputIconSize.x, y + framebarTooltipInputIconSize.y }, \
			{ icon.uStart, icon.vStart }, \
			{ icon.uEnd, icon.vEnd }, \
			prevRow.name ? darkTint : -1); \
		x += framebarTooltipInputIconSize.x + spacing; \
	}

static inline const InputsIcon* determineDirectionIcon(Input row) {
	if (row.right) {
		if (row.up) {
			return inputsIcon + INPUT_ICON_UPRIGHT;
		} else if (row.down) {
			return inputsIcon + INPUT_ICON_DOWNRIGHT;
		} else {
			return inputsIcon + INPUT_ICON_RIGHT;
		}
	} else if (row.left) {
		if (row.up) {
			return inputsIcon + INPUT_ICON_UPLEFT;
		} else if (row.down) {
			return inputsIcon + INPUT_ICON_DOWNLEFT;
		} else {
			return inputsIcon + INPUT_ICON_LEFT;
		}
	} else if (row.up) {
		return inputsIcon + INPUT_ICON_UP;
	} else if (row.down) {
		return inputsIcon + INPUT_ICON_DOWN;
	} else {
		return nullptr;
	}
}

static inline void drawDirectionIcon(ImDrawList* drawList, float& x, float y,
		float spacing, Input row, Input prevRow, ImVec2& framebarTooltipInputIconSize,
		ImU32 darkTint) {
	const InputsIcon* rowDirection = determineDirectionIcon(row);
	if (rowDirection) {
		drawList->AddImage((ImTextureID)TEXID_GGICON,
			{ x, y },
			{ x + framebarTooltipInputIconSize.x, y + framebarTooltipInputIconSize.y },
			{ rowDirection->uStart, rowDirection->vStart },
			{ rowDirection->uEnd, rowDirection->vEnd },
			(((unsigned short)row & 0xf) == ((unsigned short)prevRow & 0xf))
				? darkTint : -1);
		x += framebarTooltipInputIconSize.x + spacing;
	}
}

static inline void printInputsRowP1(ImDrawList* drawList, float x, float y,
			float spacing, int frameCount, Input row, Input prevRow, ImVec2& framebarTooltipInputIconSize,
			float textPaddingY, ImU32 darkTint, bool printFrameCounts, float maxTextSize) {
	int textLength;
	if (printFrameCounts) {
		textLength = sprintf_s(strbuf, "(%d)", frameCount);
		if (textLength != -1) {
			drawList->AddText({ x, y + textPaddingY }, -1, strbuf, strbuf + textLength);
		}
		x += maxTextSize + spacing;
	}
	
	drawDirectionIcon(drawList, x, y, spacing, row, prevRow, framebarTooltipInputIconSize, darkTint);
	PRINT_INPUT_BUTTON(punch, INPUT_ICON_PUNCH)
	PRINT_INPUT_BUTTON(kick, INPUT_ICON_KICK)
	PRINT_INPUT_BUTTON(slash, INPUT_ICON_SLASH)
	PRINT_INPUT_BUTTON(heavySlash, INPUT_ICON_HEAVYSLASH)
	PRINT_INPUT_BUTTON(dust, INPUT_ICON_DUST)
	PRINT_INPUT_BUTTON(taunt, INPUT_ICON_TAUNT)
	PRINT_INPUT_BUTTON(special, INPUT_ICON_SPECIAL)
}

static inline void printInputsRowP2(ImDrawList* drawList, float x, float y,
		float spacing, int frameCount, Input row, Input prevRow, ImVec2& framebarTooltipInputIconSize,
		float textPaddingY, ImU32 darkTint, bool printFrameCounts, float maxTextSize) {
	
	int count = 0;
	if (((unsigned short)row & 0xf) != 0) ++count;
	if (row.punch) ++count;
	if (row.kick) ++count;
	if (row.slash) ++count;
	if (row.heavySlash) ++count;
	if (row.dust) ++count;
	if (row.taunt) ++count;
	if (row.special) ++count;
	
	x = x + ImGui::GetContentRegionAvail().x
		- (
			count == 0
				? 0
				: printFrameCounts
					? count * (framebarTooltipInputIconSize.x + spacing)
					: count * framebarTooltipInputIconSize.x + (count - 1) * spacing
		)
		- maxTextSize;
	
	PRINT_INPUT_BUTTON(special, INPUT_ICON_SPECIAL)
	PRINT_INPUT_BUTTON(taunt, INPUT_ICON_TAUNT)
	PRINT_INPUT_BUTTON(dust, INPUT_ICON_DUST)
	PRINT_INPUT_BUTTON(heavySlash, INPUT_ICON_HEAVYSLASH)
	PRINT_INPUT_BUTTON(slash, INPUT_ICON_SLASH)
	PRINT_INPUT_BUTTON(kick, INPUT_ICON_KICK)
	PRINT_INPUT_BUTTON(punch, INPUT_ICON_PUNCH)
	drawDirectionIcon(drawList, x, y, spacing, row, prevRow, framebarTooltipInputIconSize, darkTint);
	
	if (printFrameCounts) {
		int textLength = sprintf_s(strbuf, "(%d)", frameCount);
		if (textLength != -1) {
			drawList->AddText({ x, y + textPaddingY }, -1, strbuf, strbuf + textLength);
		}
	}
	
}

#undef PRINT_INPUT_BUTTON

void UI::drawPlayerFrameInputsInTooltip(const PlayerFrame& frame, int playerIndex) {
	const std::vector<Input>& inputs = frame.inputs;
	if (!inputs.empty() && !(inputs.size() == 1 && inputs[0] == Input{0x0000})) {
		ImGui::Separator();
		
		float framebarTooltipInputIconSizeFloat = 20.F;
		ImVec2 framebarTooltipInputIconSize{ framebarTooltipInputIconSizeFloat, framebarTooltipInputIconSizeFloat };
		
		int frameCount = 1;
		const Input* it = inputs.data() + (inputs.size() - 1);
		Input currentInput = *it;
		
		float spacing = 1.F;
		
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		float textPaddingY = (framebarTooltipInputIconSizeFloat - ImGui::GetFontSize()) * 0.5F;
		ImVec2 windowPos = ImGui::GetWindowPos();
		ImVec2 cursorPos = ImGui::GetCursorPos();
		float oneLineHeight = ImGui::GetTextLineHeightWithSpacing() + 2.F;
		float x = windowPos.x + cursorPos.x;
		float y = windowPos.y + cursorPos.y;
		ImU32 darkTint = ImGui::GetColorU32(inputsDark);
		int rowCount = 0;
		const int rowLimit = 12;
		bool overflow = false;
		
		bool printFrameCounts = inputs.size() > 1;
		
		float maxTextSize;
		bool allInputsAreJustOneEmptyRow = false;  // with possible overflow
		int allInputsAreJustOneEmptyRow_frameCount = 0;
		if (printFrameCounts) {
			int maxFrameCount = 0;
			if (inputs.size() != 1) {
				--it;
				const Input* startIt = inputs.data() - 1;
				for (; it != startIt; --it) {
					Input nextInput = *it;
					if (nextInput != currentInput) {
						if (rowCount >= rowLimit) {
							overflow = true;
							break;
						}
						if (frameCount > maxFrameCount) maxFrameCount = frameCount;
						++rowCount;
						currentInput = nextInput;
						frameCount = 1;
					} else {
						++frameCount;
					}
				}
			}
			if (!overflow && rowCount < rowLimit) {
				if (frameCount > maxFrameCount) maxFrameCount = frameCount;
				++rowCount;
			}
			
			allInputsAreJustOneEmptyRow = rowCount == 1 && (unsigned int)inputs.back() == 0;
			if (allInputsAreJustOneEmptyRow) {
				allInputsAreJustOneEmptyRow_frameCount = frameCount;
			}
			
			frameCount = 1;
			rowCount = 0;
			overflow = false;
			it = inputs.data() + (inputs.size() - 1);
			currentInput = *it;
			
			if (!allInputsAreJustOneEmptyRow) {
				char* buf = strbuf;
				size_t bufSize = sizeof strbuf;
				if (bufSize >= 2) {
					*strbuf = '(';
					++buf;
					--bufSize;
				}
				do {
					if (bufSize < 2) break;
					*buf = '9';
					++buf;
					--bufSize;
					maxFrameCount /= 10;
				} while (maxFrameCount);
				if (bufSize >= 2) {
					*buf = ')';
					++buf;
					--bufSize;
				}
				if (bufSize) *buf = '\0';
				
				if (buf == strbuf) {
					maxTextSize = 0.F;
				} else {
					maxTextSize = ImGui::CalcTextSize(strbuf, buf).x;
				}
			} else {
				maxTextSize = 0.F;
			}
		} else {
			maxTextSize = 0.F;
		}
		
		if (!allInputsAreJustOneEmptyRow) {
			float startY = y;
			
			#define piece(funcname) \
				if (inputs.size() != 1) { \
					--it; \
					const Input* startIt = inputs.data() - 1; \
					for (; it != startIt; --it) { \
						Input nextInput = *it; \
						if (nextInput != currentInput) { \
							if (rowCount >= rowLimit) { \
								overflow = true; \
								break; \
							} \
							funcname(drawList, x, y, spacing, frameCount, currentInput, nextInput, framebarTooltipInputIconSize, textPaddingY, darkTint, printFrameCounts, maxTextSize); \
							y += oneLineHeight; \
							++rowCount; \
							currentInput = nextInput; \
							frameCount = 1; \
						} else { \
							++frameCount; \
						} \
					} \
				} \
				if (!overflow) { \
					if (rowCount >= rowLimit) { \
						overflow = true; \
					} else { \
						funcname(drawList, x, y, spacing, frameCount, currentInput, frame.prevInput, framebarTooltipInputIconSize, textPaddingY, darkTint, printFrameCounts, maxTextSize); \
						y += oneLineHeight; \
						++rowCount; \
					} \
				}
				
			
			if (playerIndex == 0) {
				piece(printInputsRowP1)
			} else {
				piece(printInputsRowP2)
			}
			#undef piece
		
			if (y != startY) {
				ImGui::InvisibleButton("##PlayerInputsRender",
					{
						1.F,
						y - startY
					});
			}
		} else {
			sprintf_s(strbuf, "(%d) <no inputs>", allInputsAreJustOneEmptyRow_frameCount);
			ImGui::TextUnformatted(strbuf);
		}
		
		if (overflow || frame.inputsOverflow) {
			static StringWithLength txt = "Remaining inputs not shown...";
			if (playerIndex == 1) {
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(txt.txt, txt.txt + txt.length).x);
			}
			ImGui::TextUnformatted(txt.txt, txt.txt + txt.length);
		}
	}
	
}

void UI::drawPlayerFrameTooltipInfo(const PlayerFrame& frame, int playerIndex, float wrapWidth) {
	frame.printInvuls(strbuf, sizeof strbuf - 7);
	if (*strbuf != '\0') {
		ImGui::Separator();
		textUnformattedColored(YELLOW_COLOR, "Invul: ");
		const char* cantPos = strstr(strbuf, "can't armor unblockables");
		if (cantPos) {
			zerohspacing
			drawOneLineOnCurrentLineAndTheRestBelow(wrapWidth, strbuf, cantPos, true, true, false);
			ImGui::PushStyleColor(ImGuiCol_Text, RED_COLOR);
			drawOneLineOnCurrentLineAndTheRestBelow(wrapWidth, "can't", nullptr, true, true, false);
			ImGui::PopStyleColor();
			drawOneLineOnCurrentLineAndTheRestBelow(wrapWidth, cantPos + 5, nullptr, true, true, true);
			_zerohspacing
		} else {
			drawOneLineOnCurrentLineAndTheRestBelow(wrapWidth, strbuf);
		}
	}
	if (frame.crossupProtectionIsOdd || frame.crossupProtectionIsAbove1) {
		ImGui::Separator();
		textUnformattedColored(YELLOW_COLOR, "Crossup protection: ");
		ImGui::SameLine();
		sprintf_s(strbuf, "%d/3", frame.crossupProtectionIsAbove1 + frame.crossupProtectionIsAbove1 + frame.crossupProtectionIsOdd);
		ImGui::TextUnformatted(strbuf);
	}
	if (frame.canYrc || frame.canYrcProjectile || frame.createdDangerousProjectile) {
		ImGui::Separator();
		if (frame.canYrcProjectile) {
			ImGui::TextUnformatted("Can YRC, and projectile/powerup will stay");
		} else if (frame.canYrc) {
			ImGui::TextUnformatted("Can YRC");
		}
		if (frame.createdDangerousProjectile) {
			ImGui::TextUnformatted("Created a projectile/powerup on this frame");
		}
	}
	printAllCancels(frame.cancels,
			frame.enableSpecialCancel,
			frame.enableJumpCancel,
			frame.enableSpecials,
			frame.hitAlreadyHappened,
			frame.airborne,
			true);
	if (frame.needShowAirOptions) {
		ImGui::Separator();
		const PlayerInfo& player = endScene.players[playerIndex];
		Entity pawn = player.pawn;
		sprintf_s(strbuf, "Double jumps: %d/%d; Airdashes: %d/%d;",
			frame.doubleJumps,
			pawn ? pawn.maxDoubleJumps() : 0,
			frame.airDashes,
			pawn ? pawn.maxAirdashes() : 0);
		ImGui::TextUnformatted(strbuf);
	}
	if (frame.poisonDuration) {
		ImGui::Separator();
		textUnformattedColored(YELLOW_COLOR, "Poison duration: ");
		ImGui::SameLine();
		sprintf_s(strbuf, "%d/360", frame.poisonDuration);
		ImGui::TextUnformatted(strbuf);
	}
	CharacterType charType = endScene.players[playerIndex].charType;
	if (charType == CHARACTER_TYPE_SOL) {
		if (frame.u.diInfo.current) {
			ImGui::Separator();
			textUnformattedColored(YELLOW_COLOR, "Dragon Install: ");
			ImGui::SameLine();
			if (frame.u.diInfo.current == USHRT_MAX) {
				ImGui::Text("overdue/%d", frame.u.diInfo.max);
			} else {
				ImGui::Text("%d/%d", frame.u.diInfo.current, frame.u.diInfo.max);
			}
		}
	} else if (charType == CHARACTER_TYPE_MILLIA) {
		if (frame.u.canProgramSecretGarden.can || frame.u.canProgramSecretGarden.inputs) {
			ImGui::Separator();
			ImGui::Text("%sInputs %d/%d",
				frame.u.canProgramSecretGarden.can ? "Can program Secret Garden. " : "Secret Garden ",
				frame.u.canProgramSecretGarden.inputs,
				frame.u.canProgramSecretGarden.inputsMax);
		}
	} else if (charType == CHARACTER_TYPE_CHIPP) {
		if (frame.u.chippInfo.invis || frame.u.chippInfo.wallTime) {
			ImGui::Separator();
			if (frame.u.chippInfo.invis) {
				printChippInvisibility(frame.u.chippInfo.invis, endScene.players[playerIndex].maxDI);
			}
			if (frame.u.chippInfo.wallTime) {
				textUnformattedColored(YELLOW_COLOR, "Wall time: ");
				ImGui::SameLine();
				if (frame.u.chippInfo.wallTime == USHRT_MAX) {
					ImGui::TextUnformatted("0/120");
				} else {
					ImGui::Text("%d/120", frame.u.chippInfo.wallTime);
				}
			}
		}
	} else if (charType == CHARACTER_TYPE_FAUST) {
		if (frame.superArmorActiveInGeneral_IsFull && strcmp(frame.animName, "5D") == 0) {
			ImGui::Separator();
			ImGui::TextUnformatted("If Faust gets hit by a reflectable projectile on this frame,"
				" the reflection will be a homerun.");
		}
	}
}

template<typename FramebarT, typename FrameT>
inline void drawFramebar(const FramebarT& framebar, FrameDims* preppedDims, int framebarPosition, ImU32 tintDarker, int playerIndex,
			const std::vector<SkippedFramesInfo>& skippedFrames) {
	const bool useSlang = settings.useSlangNames;
	for (int i = 0; i < _countof(Framebar::frames); ++i) {
		const FrameT& frame = framebar[i];
		const FrameDims& dims = preppedDims[i];
		
		ImU32 tint = -1;
		if (i > framebarPosition) {
			tint = tintDarker;
		}
		
		if (frame.type != FT_NONE) {
			ImVec2 frameStartVec { dims.x, drawFramebars_y };
			ImVec2 frameEndVec { dims.x + dims.width, drawFramebars_y + drawFramebars_frameItselfHeight };
			ImVec2 frameEndVecForTooltip;
			if (i < _countof(Framebar::frames) - 1) {
				frameEndVecForTooltip = { preppedDims[i + 1].x, frameEndVec.y };
			} else {
				frameEndVecForTooltip = frameEndVec;
			}
			
			const FrameArt* frameArt = &drawFramebars_frameArtArray[frame.type];
			const StringWithLength* description = &frameArt->description;
			if (frame.newHit && frameTypeActive(frame.type)) {
				frameArt = &drawFramebars_frameArtArray[frame.type - FT_ACTIVE + FT_ACTIVE_NEW_HIT];
				description = &frameArt->description;
			}
			drawFramebars_drawList->AddImage((ImTextureID)TEXID_FRAMES,
				frameStartVec,
				frameEndVec,
				frameArt->uvStart,
				frameArt->uvEnd,
				tint);
			
			if (frame.activeDuringSuperfreeze) {
				const FrameArt& superfreezeActiveArt = drawFramebars_frameArtArray[playerIndex == -1 ? FT_ACTIVE_PROJECTILE : FT_ACTIVE];
				drawFramebars_drawList->AddImage((ImTextureID)TEXID_FRAMES,
					{
						frameStartVec.x + dims.width * 0.5F,
						frameStartVec.y
					},
					frameEndVec,
					{
						(superfreezeActiveArt.uvStart.x + superfreezeActiveArt.uvEnd.x) * 0.5F,
						superfreezeActiveArt.uvStart.y
					},
					superfreezeActiveArt.uvEnd,
					tint);
			}
			
			if (frame.hitConnected) {
				const PngResource* art;
				if (frame.type == FT_XSTUN
						|| frame.type == FT_XSTUN_CAN_CANCEL
						|| frame.type == FT_GRAYBEAT_AIR_HITSTUN) {
					art = ui.hitConnectedFrameBlack.get();
				} else {
					art = ui.hitConnectedFrame.get();
				}
				drawFramebars_drawList->AddImage((ImTextureID)TEXID_FRAMES,
					frameStartVec,
					frameEndVec,
					{ art->uStart, art->vStart },
					{ art->uEnd, art->vEnd },
					tint);
			}
			
			ImVec2 mouseRectStart{
				drawFramebars_windowPos.x + frameStartVec.x - drawFramebars_windowPos.x,
				drawFramebars_windowPos.y + frameStartVec.y - drawFramebars_windowPos.y
			};
			
			ImVec2 mouseRectEnd{
				mouseRectStart.x + frameEndVecForTooltip.x - frameStartVec.x,
				mouseRectStart.y + frameEndVecForTooltip.y - frameStartVec.y
			};
			
			if (drawFramebars_hoveredFrameIndex == -1 && ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(mouseRectStart, mouseRectEnd, true)) {
				drawFramebars_hoveredFrameIndex = i;
				drawFramebars_hoveredFrameY = drawFramebars_y;
				if (ImGui::BeginTooltip()) {
					ImGui::PushStyleVarX(ImGuiStyleVar_ItemSpacing, 0.F);
					float wrapWidth = ImGui::GetFontSize() * 35.0f;
					static float descriptionHeights[FT_LAST] { 0 };
					static bool descriptionHeightsInitialized = false;
					static float maxDescriptionHeight = 0;
					static float maxDescriptionHeightProjectiles = 0;
					if (!descriptionHeightsInitialized) {
						descriptionHeightsInitialized = true;
						float wrapWidthUse = wrapWidth - ImGui::GetCursorPosX();
						for (int p = 1; p < FT_LAST; ++p) {
							const StringWithLength* sws = &frameArtColorblind[p].description;
							float currentHeight = ImGui::CalcTextSize(sws->txt, sws->txt + sws->length, false, wrapWidthUse).y;
							descriptionHeights[p] = currentHeight;
							if (p != FT_ACTIVE_NEW_HIT && p != FT_ACTIVE_NEW_HIT_PROJECTILE) {
								maxDescriptionHeight = max(maxDescriptionHeight, currentHeight);
							}
						}
						for (int p = 0; p < _countof(projectileFrameTypes); ++p) {
							FrameType ptype = projectileFrameTypes[p];
							float currentHeight = descriptionHeights[ptype];
							if (ptype != FT_ACTIVE_NEW_HIT && ptype != FT_ACTIVE_NEW_HIT_PROJECTILE) {
								maxDescriptionHeightProjectiles = max(maxDescriptionHeightProjectiles, currentHeight);
							}
						}
					}
					ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
					float oldCursorPos = ImGui::GetCursorPosY();
					ImGui::TextUnformatted(description->txt, description->txt + description->length);
					float newCursorPos = ImGui::GetCursorPosY();
					float textHeight = newCursorPos - oldCursorPos;
					float requiredHeight;
					if (playerIndex != -1) {
						requiredHeight = maxDescriptionHeight;
					} else {
						requiredHeight = maxDescriptionHeightProjectiles;
					}
					if (textHeight < requiredHeight) {
						ImGui::SetCursorPosY(newCursorPos + requiredHeight - textHeight);
					}
					const char* name;
					if (useSlang && frame.animSlangName && *frame.animSlangName != '\0') {
						name = frame.animSlangName;
					} else {
						name = frame.animName;
					}
					if (name && *name != '\0') {
						ImGui::Separator();
						textUnformattedColored(YELLOW_COLOR, "Anim: ");
						ImGui::SameLine();
						ImGui::TextUnformatted(name);
					}
					if (frame.activeDuringSuperfreeze) {
						ImGui::Separator();
						ImGui::TextUnformatted("After this frame the attack becomes active during superfreeze."
							" In order to block that attack, it must be blocked on this frame, in advance.");
					}
					if (playerIndex != -1) {
						ui.drawPlayerFrameTooltipInfo((const PlayerFrame&)frame, playerIndex, wrapWidth);
					}
					if (playerIndex != -1) {
						const PlayerFrame& playerFrame = (const PlayerFrame&)frame;
						if (playerFrame.hitstop
								|| playerFrame.stop.isHitstun
								|| playerFrame.stop.isBlockstun
								|| playerFrame.stop.isStagger
								|| playerFrame.stop.isWakeup) {
							ImGui::Separator();
							printFameStop(strbuf, sizeof strbuf, &playerFrame.stop, playerFrame.hitstop, playerFrame.hitstopMax, playerFrame.lastBlockWasIB, playerFrame.lastBlockWasFD);
							ImGui::TextUnformatted(strbuf);
						}
					} else {
						if (frame.hitstop) {
							ImGui::Separator();
							printFameStop(strbuf, sizeof strbuf, nullptr, frame.hitstop, frame.hitstopMax, false, false);
							ImGui::TextUnformatted(strbuf);
						}
					}
					const SkippedFramesInfo& skippedFramesElem = skippedFrames[i];
					if (skippedFramesElem.count || frame.rcSlowdown || frame.hitConnected || frame.newHit) {
						ImGui::Separator();
						if (skippedFramesElem.count) {
							skippedFramesElem.print(frameAssumesCanBlockButCantFDAfterSuperfreeze(frame.type));
						}
						if (frame.newHit) {
							ImGui::TextUnformatted("A new (potential) hit starts on this frame.");
						}
						if (playerIndex != -1 && ((const PlayerFrame&)frame).blockedOnThisFrame) {
							const PlayerFrame& playerFrame = (const PlayerFrame&)frame;
							if (playerFrame.lastBlockWasIB) {
								ImGui::TextUnformatted("IB'd a hit on this frame");
							} else if (playerFrame.lastBlockWasFD) {
								ImGui::TextUnformatted("FD'd a hit on this frame");
							} else {
								ImGui::TextUnformatted("Blocked a hit on this frame");
							}
						} else if (frame.hitConnected) {
							ImGui::TextUnformatted("A hit connected on this frame.");
						}
						if (frame.rcSlowdown) {
							textUnformattedColored(YELLOW_COLOR, "RC-slowed down: ");
							ImGui::SameLine();
							sprintf_s(strbuf, "%d/%d", frame.rcSlowdown, frame.rcSlowdownMax);
							ImGui::TextUnformatted(strbuf);
						}
					}
					if (playerIndex != -1) {
						ui.drawPlayerFrameInputsInTooltip((const PlayerFrame&)frame, playerIndex);
					}
					ImGui::PopTextWrapPos();
					ImGui::PopStyleVar();
					ImDrawList* drawList = ImGui::GetWindowDrawList();
					ImGui::EndTooltip();
					if (ui.needSplitFramebar) {
						copyDrawList(*(ImDrawListBackup*)ui.framebarTooltipDrawDataCopy.data(), drawList);
						drawList->CmdBuffer.clear();
						drawList->IdxBuffer.clear();
						drawList->VtxBuffer.clear();
					}
				}
			}
		}
	}
}

void drawPlayerFramebar(const PlayerFramebar& framebar, FrameDims* preppedDims, int framebarPosition, ImU32 tintDarker, int playerIndex,
			const std::vector<SkippedFramesInfo>& skippedFrames) {
	drawFramebar<PlayerFramebar, PlayerFrame>(framebar, preppedDims, framebarPosition, tintDarker, playerIndex, skippedFrames);
}

void drawProjectileFramebar(const Framebar& framebar, FrameDims* preppedDims, int framebarPosition, ImU32 tintDarker,
			const std::vector<SkippedFramesInfo>& skippedFrames) {
	drawFramebar<Framebar, Frame>(framebar, preppedDims, framebarPosition, tintDarker, -1, skippedFrames);
}

template<typename FramebarT, typename FrameT>
void drawFirstFrames(const FramebarT& framebar, int framebarPosition, FrameDims* preppedDims, float firstFrameTopY, float firstFrameBottomY) {
	const bool considerSimilarFrameTypesSameForFrameCounts = settings.considerSimilarFrameTypesSameForFrameCounts;
	const bool considerSimilarIdleFramesSameForFrameCounts = settings.considerSimilarIdleFramesSameForFrameCounts;
	const ImVec2 firstFrameUVStart = { ui.firstFrame->uStart, ui.firstFrame->vStart };
	const ImVec2 firstFrameUVEnd = { ui.firstFrame->uEnd, ui.firstFrame->vEnd };
	for (int i = 0; i < _countof(Framebar::frames); ++i) {
		const FrameT& frame = framebar[i];
		const FrameDims& dims = preppedDims[i];
		
		if (frame.isFirst
				&& !(
					considerSimilarIdleFramesSameForFrameCounts
						?
							i == framebarPosition
								?
									framebar.preFrame != FT_NONE
									&& frameMap(frame.type) == frameMap(framebar.preFrame)
									&& frameMap(frame.type) == FT_IDLE
								:
									frameMap(frame.type) == frameMap(framebar[i == 0 ? _countof(Framebar::frames) - 1 : i - 1].type)
									&& frameMap(frame.type) == FT_IDLE
						:
							false
				)) {
			drawFramebars_drawList->AddImage((ImTextureID)TEXID_FRAMES,
				{
					dims.x - drawFramebars_innerBorderThicknessHalf - dims.width * 0.5F,
					firstFrameTopY
				},
				{
					dims.x - drawFramebars_innerBorderThicknessHalf + dims.width * 0.5F,
					firstFrameBottomY
				},
				firstFrameUVStart,
				firstFrameUVEnd,
				-1);
		}
	}
}

template<typename FramebarT, typename FrameT>
void drawDigits(const FramebarT& framebar, int framebarPosition, FrameDims* preppedDims, float frameNumberYTop, float frameNumberYBottom) {
	
	const bool showFirstFrames = settings.showFirstFramesOnFramebar;
	const bool considerSimilarFrameTypesSameForFrameCounts = settings.considerSimilarFrameTypesSameForFrameCounts;
	const bool considerSimilarIdleFramesSameForFrameCounts = settings.considerSimilarIdleFramesSameForFrameCounts;
	
	FrameType lastFrameType = framebar.preFrame;
	if (considerSimilarFrameTypesSameForFrameCounts) {
		lastFrameType = considerSimilarIdleFramesSameForFrameCounts ? frameMap(lastFrameType) : frameMapNoIdle(lastFrameType);
	}
	int sameFrameTypeCount = framebar.preFrameLength;
	int visualFrameCount = 0;
	
	for (int i = 0; i < _countof(Framebar::frames); ++i) {
		int ind = (framebarPosition + 1 + i) % _countof(Framebar::frames);
		const FrameT& frame = framebar[ind];
		
		enum DivisionType {
			DIVISION_TYPE_NONE,
			DIVISION_TYPE_DIFFERENT_TYPES,
			DIVISION_TYPE_REACHED_END
		} divisionType = DIVISION_TYPE_NONE;
		
		int displayPos = ind - 1;
		
		FrameType currentType = frame.type;
		if (considerSimilarFrameTypesSameForFrameCounts) {
			currentType = considerSimilarIdleFramesSameForFrameCounts ? frameMap(currentType) : frameMapNoIdle(currentType);
		} else if (currentType == FT_IDLE_ACTIVE_IN_SUPERFREEZE) {
			currentType = FT_IDLE_PROJECTILE;
		}
		
		bool isFirst;
		if (showFirstFrames) {
			isFirst = frame.isFirst
				&& !(
					considerSimilarIdleFramesSameForFrameCounts
						?
							i == 0
								?
									framebar.preFrame != FT_NONE
									&& frameMap(frame.type) == frameMap(framebar.preFrame)
									&& frameMap(frame.type) == FT_IDLE
								:
									frameMap(frame.type) == frameMap(framebar[ind == 0 ? _countof(Framebar::frames) - 1 : ind - 1].type)
									&& frameMap(frame.type) == FT_IDLE
						:
							false
				);
		} else {
			isFirst = false;
		}
		
		if (currentType == lastFrameType
				&& !isFirst
				&& i == _countof(Framebar::frames) - 1
				&& lastFrameType != FT_NONE) {
			divisionType = DIVISION_TYPE_REACHED_END;
			++displayPos;
			++sameFrameTypeCount;
			++visualFrameCount;
		} else if (!(currentType == lastFrameType && !isFirst)
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
				
				if (digitWidth > drawFramebars_frameWidthScaled) {
					digitX += (digitWidth - drawFramebars_frameWidthScaled) * 0.5F;
					digitWidth = drawFramebars_frameWidthScaled;
				}
				
				drawFramebars_drawList->AddImage((ImTextureID)TEXID_FRAMES,
					{ digitX, frameNumberYTop },
					{ digitX + digitWidth, frameNumberYBottom },
					digitImg.start,
					digitImg.end);
				
				++prevIndCounter;
			}
		}
		
		if (currentType == lastFrameType && !isFirst) {
			++sameFrameTypeCount;
			++visualFrameCount;
		} else {
			lastFrameType = currentType;
			sameFrameTypeCount = 1;
			visualFrameCount = 1;
		}
	}
}

void printChippInvisibility(int current, int max) {
	int percentage = current % 120;
	if (percentage <= 65) {
		if (percentage > 55) {
			percentage = 55;
		}
	} else {
		percentage = 120 - percentage;
	}
	percentage = 100 * percentage / 55;
	textUnformattedColored(YELLOW_COLOR, "Invisibility: ");
	ImGui::SameLine();
	ImGui::Text("%s (%d/%d)",
		ui.printDecimal(percentage, 0, 3, true),
		current,
		max);
}

void textUnformattedColored(ImVec4 color, const char* str) {
	ImGui::PushStyleColor(ImGuiCol_Text, color);
	ImGui::TextUnformatted(str);
	ImGui::PopStyleColor();
}

void drawOneLineOnCurrentLineAndTheRestBelow(float wrapWidth, const char* str, const char* strEnd, bool needSameLine, bool needManualMultilineOutput, bool isLastLine) {
	if (needSameLine) ImGui::SameLine();
	ImFont* font = ImGui::GetFont();
	const char* textEnd = strEnd ? strEnd : str + strlen(str);
	const char* newlinePos = (const char*)memchr(str, '\n', textEnd - str);
	if (newlinePos == nullptr) newlinePos = textEnd;
	float wrapWidthUse = wrapWidth - ImGui::GetCursorPosX();
	const char* wrapPos = font->CalcWordWrapPositionA(1.F, str, textEnd, wrapWidthUse);
	if (wrapPos != textEnd && (wrapPos == str || wrapPos == str + 1 && *wrapPos > 32) && needSameLine) {
		ImGui::PushStyleVarY(ImGuiStyleVar_ItemSpacing, 0.F);
		ImGui::NewLine();
		ImGui::PopStyleVar();
		float wrapWidthNew = wrapWidth - ImGui::GetCursorPosX();
		wrapWidthUse = wrapWidthNew;
		wrapPos = font->CalcWordWrapPositionA(1.F, str, textEnd, wrapWidthUse);
	}
	if (wrapPos == textEnd) {
		ImGui::TextUnformatted(str, textEnd);
		return;
	} else {
		float itemSpacing = ImGui::GetStyle().ItemSpacing.y;
		ImGui::PushStyleVarY(ImGuiStyleVar_ItemSpacing, 0.F);
		ImGui::TextUnformatted(str, wrapPos);
		while (wrapPos < textEnd && *wrapPos <= 32) {
			++wrapPos;
		}
		if (needManualMultilineOutput) {
			wrapWidthUse = wrapWidth - ImGui::GetCursorPosX();
			while (wrapPos != textEnd) {
				str = wrapPos;
				wrapPos = font->CalcWordWrapPositionA(1.F, str, textEnd, wrapWidthUse);
				ImGui::TextUnformatted(str, wrapPos);
				while (wrapPos < textEnd && *wrapPos <= 32) {
					++wrapPos;
				}
			}
		} else if (wrapPos != textEnd) {
			ImGui::TextUnformatted(wrapPos, textEnd);
		}
		ImGui::PopStyleVar();
		if (isLastLine) {
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + itemSpacing);
		}
	}
}

static void printActiveWithMaxHit(const ActiveDataArray& active, const MaxHitInfo& maxHit, int hitOnFrame) {
	char* buf = strbuf;
	size_t bufSize = sizeof strbuf;
	int result;
	result = active.print(buf, bufSize);
	advanceBuf
	if (!maxHit.empty()
			&& strbuf[0] != '\0'
			&& !(
				active.count == maxHit.maxUse
				&& maxHit.maxUse <= 2  // for Ky Air stun edge and Jack O' j.H
				|| active.count < maxHit.maxUse
			)) {
		result = sprintf_s(buf, bufSize, " (%d hit%s max)", maxHit.maxUse, maxHit.maxUse == 1 ? "" : "s");
		advanceBuf
	}
	if (hitOnFrame) {
		const char* ordinalWordEnding;
		const int remainder = hitOnFrame % 10;
		if (remainder == 1 && hitOnFrame != 11) {
			ordinalWordEnding = "st";
		} else if (remainder == 2 && hitOnFrame != 12) {
			ordinalWordEnding = "nd";
		} else if (remainder == 3 && hitOnFrame != 13) {
			ordinalWordEnding = "rd";
		} else {
			ordinalWordEnding = "th";
		}
		
		const char* earliestNote;
		if (active.count <= 1) {
			earliestNote = "hit";
		} else {
			earliestNote = "earliest hit";
		}
		
		if (hitOnFrame == 1) {
			//sprintf_s(buf, bufSize, " (%s on first active frame)",
			//	earliestNote);
			// this is way too annoying
		} else {
			sprintf_s(buf, bufSize, " (%s on %d%s active frame)",
				earliestNote,
				hitOnFrame,
				ordinalWordEnding);
		}
	}
}

bool UI::booleanSettingPresetWithHotkey(std::atomic_bool& settingsRef, std::vector<int>& hotkey) {
	bool itHappened = false;
	bool boolValue = settingsRef;
	if (ImGui::Checkbox(searchFieldTitle(settings.getOtherUINameWithLength(&settingsRef)), &boolValue)) {
		settingsRef = boolValue;
		needWriteSettings = true;
		itHappened = true;
	}
	ImGui::SameLine();
	HelpMarkerWithHotkey(settings.getOtherUIDescriptionWithLength(&settingsRef), hotkey);
	return itHappened;
}

bool UI::booleanSettingPreset(std::atomic_bool& settingsRef) {
	bool itHappened = false;
	bool boolValue = settingsRef;
	if (ImGui::Checkbox(searchFieldTitle(settings.getOtherUINameWithLength(&settingsRef)), &boolValue)) {
		settingsRef = boolValue;
		needWriteSettings = true;
		itHappened = true;
	}
	ImGui::SameLine();
	HelpMarker(searchTooltip(settings.getOtherUIDescriptionWithLength(&settingsRef)));
	return itHappened;
}

bool UI::float4SettingPreset(float& settingsPtr) {
	bool attentionPossiblyNeeded = false;
	float floatValue = settingsPtr;
	if (ImGui::InputFloat(searchFieldTitle(settings.getOtherUINameWithLength(&settingsPtr)), &floatValue, 1.F, 10.F, "%.4f")) {
		settingsPtr = floatValue;
		needWriteSettings = true;
		attentionPossiblyNeeded = true;
	}
	imguiActiveTemp = imguiActiveTemp || ImGui::IsItemActive();
	ImGui::SameLine();
	HelpMarker(searchTooltip(settings.getOtherUIDescriptionWithLength(&settingsPtr)));
	return attentionPossiblyNeeded;
}

bool UI::intSettingPreset(std::atomic_int& settingsPtr, int minValue) {
	bool isChange = false;
	int intValue = settingsPtr;
	ImGui::SetNextItemWidth(80.F);
	if (ImGui::InputInt(searchFieldTitle(settings.getOtherUINameWithLength(&settingsPtr)), &intValue, 1, 1, 0)) {
		if (intValue < minValue) {
			intValue = minValue;
		}
		settingsPtr = intValue;
		needWriteSettings = true;
		isChange = true;
	}
	imguiActiveTemp = imguiActiveTemp || ImGui::IsItemActive();
	ImGui::SameLine();
	HelpMarker(searchTooltip(settings.getOtherUIDescriptionWithLength(&settingsPtr)));
	return isChange;
}

void drawPlayerIconInWindowTitle(int playerIndex) {
	GGIcon scaledIcon = scaleGGIconToHeight(getPlayerCharIcon(playerIndex), 14.F);
	drawPlayerIconInWindowTitle(scaledIcon);
}

void drawPlayerIconInWindowTitle(GGIcon& icon) {
	ImVec2 windowPos = ImGui::GetWindowPos();
	float windowWidth = ImGui::GetWindowWidth();
	ImGuiStyle& style = ImGui::GetStyle();
	const float fontSize = ImGui::GetFontSize();
	ImVec2 startPos {
		windowPos.x + style.FramePadding.x + fontSize + style.ItemInnerSpacing.x,
		windowPos.y + style.FramePadding.y
	};
	ImVec2 clipEnd {
		windowPos.x + windowWidth - style.FramePadding.x - fontSize - style.ItemInnerSpacing.x,
		startPos.y + icon.size.y
	};
	if (clipEnd.x > startPos.x) {
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->PushClipRect(startPos,
			clipEnd,
			false);
		int alpha = ImGui::IsWindowCollapsed() ? 128 : 255;
		drawList->AddImage((ImTextureID)TEXID_GGICON,
			startPos,
			{
				startPos.x + icon.size.x,
				startPos.y + icon.size.y
			},
			icon.uvStart,
			icon.uvEnd,
			ImGui::GetColorU32(IM_COL32(255, 255, 255, alpha)));
		drawList->PopClipRect();
	}
}

void UI::printAllCancels(const FrameCancelInfo& cancels,
		bool enableSpecialCancel,
		bool enableJumpCancel,
		bool enableSpecials,
		bool hitAlreadyHappened,
		bool airborne,
		bool insertSeparators) {
	bool needUnpush = false;
	if (ImGui::GetStyle().ItemSpacing.x != 0.F) {
		needUnpush = true;
		ImGui::PushStyleVarX(ImGuiStyleVar_ItemSpacing, 0.F);
	}
	searchFieldTitle("Gatlings");
	searchFieldTitle("Whiff Cancels");
	searchFieldTitle("Late Cancels");
	searchFieldTitle("Jump cancel");
	searchFieldTitle("Specials");
	if (!cancels.gatlings.empty() || enableSpecialCancel || enableJumpCancel) {
		if (insertSeparators) ImGui::Separator();
		textUnformattedColored(YELLOW_COLOR, "Gatlings:");
		int count = 1;
		if (!cancels.gatlings.empty()) {
			count += printCancels(cancels.gatlings);
		}
		if (enableSpecialCancel) {
			ImGui::Text("%d) Specials", count);
		}
		if (enableJumpCancel) {
			ImGui::Text("%d) Jump cancel", count);
		}
	}
	if (!cancels.whiffCancels.empty() || enableSpecials) {
		if (insertSeparators) ImGui::Separator();
		if (hitAlreadyHappened) {
			textUnformattedColored(YELLOW_COLOR, "Late cancels:");
		} else {
			textUnformattedColored(YELLOW_COLOR, "Whiff cancels:");
		}
		int count = 1;
		if (!cancels.whiffCancels.empty()) {
			count += printCancels(cancels.whiffCancels);
		}
		if (enableSpecials) {
			if (airborne) {
				ImGui::Text("%d) Specials (note: some specials have a minimum height limit and might be unavailable at this time)", count);
			} else {
				ImGui::Text("%d) Specials", count);
			}
		}
	}
	if (cancels.gatlings.empty() && !enableSpecialCancel
			&& cancels.whiffCancels.empty() && !enableSpecials
			&& cancels.whiffCancelsNote) {
		if (insertSeparators) ImGui::Separator();
		if (hitAlreadyHappened) {
			textUnformattedColored(YELLOW_COLOR, "Late cancels:");
		} else {
			textUnformattedColored(YELLOW_COLOR, "Whiff cancels:");
		}
		ImGui::TextUnformatted(cancels.whiffCancelsNote);
	}
	if (needUnpush) {
		ImGui::PopStyleVar();
	}
}

bool prevNamesControl(const PlayerInfo& player, bool includeTitle, bool disableSlang) {
	if (player.canPrintTotal() || player.startupType() != -1) {
		*strbuf = '\0';
		char* buf = strbuf;
		size_t bufSize = sizeof strbuf;
		if (includeTitle) {
			int result = sprintf_s(buf, bufSize, "Move: ");
			advanceBuf
		}
		const char* lastNames[2];
		prepareLastNames(lastNames, player, disableSlang);
		player.prevStartupsDisp.printNames(buf, bufSize, lastNames, lastNames[1] ? 2 : 1, disableSlang ? false : settings.useSlangNames.load());
		return true;
	}
	return false;
}

void headerThatCanBeClickedForTooltip(const char* title, bool* windowVisibilityVar, bool makeTooltip) {
	CenterAlign(ImGui::CalcTextSize(title).x);
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, { 0.F, 0.F, 0.F, 0.F });
	if (ImGui::Selectable(title)) {
		*windowVisibilityVar = !*windowVisibilityVar;
	}
	ImGui::PopStyleColor();
	if (makeTooltip) {
		AddTooltip("Click the field for tooltip.");
	}
}

void prepareLastNames(const char** lastNames, const PlayerInfo& player, bool disableSlang) {
	const char* lastName;
	if (!disableSlang && settings.useSlangNames && player.lastPerformedMoveSlangName) {
		lastName = player.lastPerformedMoveSlangName;
	} else {
		lastName = player.lastPerformedMoveName;
	}
	if (player.startupType() == 0) {
		lastNameSuperfreeze = lastName;
		lastNameSuperfreeze += " Superfreeze Startup";
		lastNameAfterSuperfreeze = lastName;
		lastNameAfterSuperfreeze += " After Superfreeze";
		lastNames[0] = lastNameSuperfreeze.c_str();
		lastNames[1] = lastNameAfterSuperfreeze.c_str();
	} else {
		lastNames[0] = lastName;
		lastNames[1] = nullptr;
	}
}

const char* formatHitResult(HitResult hitResult) {
	switch (hitResult) {
		case HIT_RESULT_NONE: return "No Hit";
		case HIT_RESULT_NORMAL: return "Hit";
		case HIT_RESULT_BLOCKED: return "Blocked";
		case HIT_RESULT_IGNORED: return "Ignored";
		case HIT_RESULT_ARMORED: return "Armored";
		case HIT_RESULT_ARMORED_BUT_NO_DMG_REDUCTION: return "Armored, but without Armor Damage Reduction";
		default: return "Unknown";
	}
}

const char* formatBlockType(BlockType blockType) {
	switch (blockType) {
		case BLOCK_TYPE_NORMAL: return "Normal";
		case BLOCK_TYPE_FAULTLESS: return "FD";
		case BLOCK_TYPE_INSTANT: return "IB";
		default: return "Unknown";
	}
}

int printDamageGutsCalculation(int x, int defenseModifier, int gutsRating, int guts, int gutsLevel) {
	ImGui::TableNextColumn();
	ImGui::TextUnformatted("Defense Modifier");
	AddTooltip("Defender's defense modifier, constant per character. Used to scale incoming damage.\n"
		"The scale = (defense modifier + 256) / 256. First number shown is the defense modifier, without"
		" addition to 256. The second number is the scale.");
	ImGui::TableNextColumn();
	const char* defenseModifierStr = ui.printDecimal((defenseModifier + 256) * 100 / 256, 0, 0, true);
	sprintf_s(strbuf, "%d (%s)", defenseModifier, defenseModifierStr);
	ImGui::TextUnformatted(strbuf);
	
	ImGui::TableNextColumn();
	ImGui::TextUnformatted("Guts Rating");
	static const char* gutsHelp = "Guts rating, fixed constant per character. Determines how fast damage scales when HP drops to certain thresholds."
		"Guts rating table:\n"
		"Guts Rating 0: 100, 90, 76, 60, 50, 40;\n"
		"Guts Rating 1: 100, 87, 72, 58, 48, 40;\n"
		"Guts Rating 2: 100, 84, 68, 56, 46, 38;\n"
		"Guts Rating 3: 100, 81, 66, 54, 44, 38;\n"
		"Guts Rating 4: 100, 78, 64, 50, 42, 38;\n"
		"Guts Rating 5: 100, 75, 60, 48, 40, 36;\n"
		"Answer, Bedman, Elphelt, Faust, Zato: Guts Rating 0.\n"
		"Axl, I-No, Ramlethal, Sin, Slayer, Sol, Venom, Dizzy: Guts Rating 1.\n"
		"Ky, Haehyun, Jack-O': Guts Rating 2.\n"
		"Johnny, Leo, May, Millia, Potemkin, Jam: Guts Rating 3.\n"
		"Baiken, Chipp: Guts Rating 4.\n"
		"Raven: Guts Rating 5.\n"
		"\n"
		"HP thresholds: >50% (>210), <=50% (<=210), <=40% (<=168), <=30% (<=126), <=20% (<=84), <=10% (<=42).";
	AddTooltip(gutsHelp);
	ImGui::TableNextColumn();
	sprintf_s(strbuf, "%d", gutsRating);
	ImGui::TextUnformatted(strbuf);
	
	ImGui::TableNextColumn();
	ImGui::TextUnformatted("Guts");
	AddTooltip(gutsHelp);
	ImGui::TableNextColumn();
	if (gutsLevel == 0) {
		sprintf_s(strbuf, "%d%c (HP > 50%c)", guts, '%', '%');
	} else {
		sprintf_s(strbuf, "%d%c (HP <= %d%c)", guts, '%',
			50 - (gutsLevel - 1) * 10,
			'%');
	}
	ImGui::TextUnformatted(strbuf);
	
	ImGui::TableNextColumn();
	ImGui::TextUnformatted("Def.Mod. * Guts");
	AddTooltip("This equals (defense modifier + 256) * guts / 100. The percentage is shown after division by 256.");
	ImGui::TableNextColumn();
	char* buf = strbuf;
	size_t bufSize = sizeof strbuf;
	int result = sprintf_s(buf, bufSize, "%s * %d%c = ", defenseModifierStr, guts, '%');
	advanceBuf
	const char* totalGutsStr = ui.printDecimal((defenseModifier + 256) * guts / 256, 0, 0, true);
	sprintf_s(buf, bufSize, "%s", totalGutsStr);
	ImGui::TextUnformatted(strbuf);
	
	ImGui::TableNextColumn();
	const char* tooltip = "This equals (defense modifier + 256) * guts / 100 * damage / 256.";
	zerohspacing
	textUnformattedColored(YELLOW_COLOR, "Damage");
	AddTooltip(tooltip);
	ImGui::SameLine();
	ImGui::TextUnformatted(" * (Def.Mod. * Guts)");
	AddTooltip(tooltip);
	_zerohspacing
	ImGui::TableNextColumn();
	int oldX = x;
	x = (defenseModifier + 256) * guts / 100 * x / 256;
	zerohspacing
	sprintf_s(strbuf, "%d * %s = ", oldX, totalGutsStr);
	ImGui::TextUnformatted(strbuf);
	ImGui::SameLine();
	sprintf_s(strbuf, "%d", x);
	textUnformattedColored(YELLOW_COLOR, strbuf);
	_zerohspacing
	
	return x;
}

int printChipDamageCalculation(int x, int baseDamage, int attackKezuri, int attackKezuriStandard) {
	ImGui::TableNextColumn();
	ImGui::TextUnformatted("Chip Damage Modif");
	AddTooltip("Chip damage modifier specifies how much of the base damage is applied as chip damage on block."
		" The standard value for supers and specials is 16 and that amounts to 12.5% and goes linearly up or down from there.");
	ImGui::TableNextColumn();
	const char* chipModifStr = ui.printDecimal(attackKezuri * 10000 / 128, 2, 0, true);
	sprintf_s(strbuf, "%d (%s)", attackKezuri, chipModifStr);
	
	const char* needHelp = nullptr;
	ImGui::TextUnformatted(strbuf);
	ImVec4* color = &RED_COLOR;
	if (attackKezuri > attackKezuriStandard) {
		needHelp = "higher";
	} else if (attackKezuri < attackKezuriStandard) {
		needHelp = "lower";
		color = &LIGHT_BLUE_COLOR;
	}
	
	if (attackKezuri != attackKezuriStandard) {
		ImGui::SameLine();
		textUnformattedColored(*color, "(!)");
		if (ImGui::BeginItemTooltip()) {
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			sprintf_s(strbuf, "This attack's chip damage modifier (%d) is %s than the standard chip damage modifier (%d) for all specials and overdrives.",
				attackKezuri,
				needHelp,
				attackKezuriStandard);
			ImGui::TextUnformatted(strbuf);
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}
	}
	
	ImGui::TableNextColumn();
	const char* tooltip = "Final damage can't be less than 1, if base damage was greater than 0 and chip damage modifier was greater than 0.";
	zerohspacing
	ImGui::TextUnformatted("Chip ");
	AddTooltip(tooltip);
	ImGui::SameLine();
	textUnformattedColored(YELLOW_COLOR, "Damage");
	AddTooltip(tooltip);
	_zerohspacing
	ImGui::TableNextColumn();
	int oldX = x;
	x = x * attackKezuri / 128;
	if (x < 1 && attackKezuri > 0 && baseDamage > 0) x = 1;
	zerohspacing
	sprintf_s(strbuf, "%d * %s = ", oldX, chipModifStr);
	ImGui::TextUnformatted(strbuf);
	ImGui::SameLine();
	sprintf_s(strbuf, "%d", x);
	textUnformattedColored(YELLOW_COLOR, strbuf);
	_zerohspacing
	
	return x;
}

int printScaleDmgBasic(int x, int playerIndex, int damageScale, bool isProjectile, int projectileDamageScale, HitResult hitResult, int superArmorDamagePercent) {
	x = x * 10;
	int oldX = x;
	CharacterType otherChar = endScene.players[1 - playerIndex].charType;
	if (otherChar == CHARACTER_TYPE_RAVEN
			|| endScene.players[playerIndex].charType == CHARACTER_TYPE_RAVEN
			&& (
				otherChar == CHARACTER_TYPE_DIZZY
				|| otherChar == CHARACTER_TYPE_ZATO
				|| otherChar == CHARACTER_TYPE_LEO
			)) {
		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Damage Scale");
		AddTooltip("This damage scale is used by Raven when he has excitement.");
		ImGui::TableNextColumn();
		sprintf_s(strbuf, "%d%c", damageScale, '%');
		ImGui::TextUnformatted(strbuf);
		
		x = x * damageScale / 100;
		ImGui::TableNextColumn();
		zerohspacing
		textUnformattedColored(YELLOW_COLOR, "Damage");
		ImGui::SameLine();
		ImGui::TextUnformatted(" * Scale");
		_zerohspacing
		ImGui::TableNextColumn();
		
		zerohspacing
		sprintf_s(strbuf, "%s * %d%c = ", ui.printDecimal(oldX, 1, 0, false), damageScale, '%');
		ImGui::TextUnformatted(strbuf);
		ImGui::SameLine();
		textUnformattedColored(YELLOW_COLOR, ui.printDecimal(x, 1, 0, false));
		_zerohspacing
	}
	
	ImGui::TableNextColumn();
	ImGui::TextUnformatted("Is Projectile");
	AddTooltip("If the attack is a projectile, the defender's projectile damage scale is applied below.");
	ImGui::TableNextColumn();
	ImGui::TextUnformatted(isProjectile ? "Yes" : "No");
	
	ImGui::TableNextColumn();
	ImGui::TextUnformatted("Projectile Damage Scale");
	AddTooltip("Defender's projectile damage scale.");
	ImGui::TableNextColumn();
	oldX = x;
	if (!isProjectile) {
		ImGui::TextUnformatted("Doesn't apply (100%)");
	} else {
		x = x * projectileDamageScale / 100;
		sprintf_s(strbuf, "%d%c", projectileDamageScale, '%');
		ImGui::TextUnformatted(strbuf);
	}
	
	ImGui::TableNextColumn();
	const char* tooltip = "Damage * Defender's projectile damage scale";
	zerohspacing
	textUnformattedColored(YELLOW_COLOR, "Damage");
	AddTooltip(tooltip);
	ImGui::SameLine();
	ImGui::TextUnformatted(" * Proj.Dmg.Scale");
	AddTooltip(tooltip);
	_zerohspacing
	ImGui::TableNextColumn();
	if (!isProjectile) {
		zerohspacing
		ImGui::TextUnformatted("Doesn't apply (");
		ImGui::SameLine();
		textUnformattedColored(YELLOW_COLOR, ui.printDecimal(x, 1, 0, false));
		ImGui::SameLine();
		ImGui::TextUnformatted(")");
		_zerohspacing
	} else {
		zerohspacing
		sprintf_s(strbuf, "%s * %d%c = ", ui.printDecimal(oldX, 1, 0, false), projectileDamageScale, '%');
		ImGui::TextUnformatted("Doesn't apply (");
		ImGui::SameLine();
		textUnformattedColored(YELLOW_COLOR, ui.printDecimal(x, 1, 0, false));
		ImGui::SameLine();
		ImGui::TextUnformatted(")");
		_zerohspacing
	}
	
	oldX = x;
	if (hitResult == HIT_RESULT_ARMORED || hitResult == HIT_RESULT_ARMORED_BUT_NO_DMG_REDUCTION) {
		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Armor Dmg Scale");
		AddTooltip("Defender's armor damage scale gets applied to the incoming damage.");
		ImGui::TableNextColumn();
		if (hitResult == HIT_RESULT_ARMORED) {
			x = x * superArmorDamagePercent / 100;
			sprintf_s(strbuf, "%d%c", superArmorDamagePercent, '%');
			ImGui::TextUnformatted(strbuf);
		} else {
			ImGui::TextUnformatted("Doesn't apply (100%)");
		}
	}
	x /= 10;
	
	if (hitResult == HIT_RESULT_ARMORED || hitResult == HIT_RESULT_ARMORED_BUT_NO_DMG_REDUCTION) {
		ImGui::TableNextColumn();
		zerohspacing
		textUnformattedColored(YELLOW_COLOR, "Damage");
		ImGui::SameLine();
		ImGui::TextUnformatted(" * Armor Scale");
		_zerohspacing
		ImGui::TableNextColumn();
		zerohspacing
		if (hitResult == HIT_RESULT_ARMORED) {
			sprintf_s(strbuf, "%s * %d%c = ", ui.printDecimal(oldX, 1, 0, false), superArmorDamagePercent, '%');
			ImGui::TextUnformatted(strbuf);
		} else {
			ImGui::TextUnformatted("Doesn't apply (");
		}
		ImGui::SameLine();
		sprintf_s(strbuf, "%d", x);
		textUnformattedColored(YELLOW_COLOR, strbuf);
		if (hitResult != HIT_RESULT_ARMORED) {
			ImGui::SameLine();
			ImGui::TextUnformatted(")");
		}
		_zerohspacing
	}
	
	return x;
}

const char* formatAttackType(AttackType attackType) {
	switch (attackType) {
		case ATTACK_TYPE_NONE: return "None";
		case ATTACK_TYPE_NORMAL: return "Normal";
		case ATTACK_TYPE_EX: return "Special";
		case ATTACK_TYPE_OVERDRIVE: return "Overdrive";
		case ATTACK_TYPE_IK: return "Instant Kill";
		default: return "Unknown";
	}
}

const char* formatGuardType(GuardType guardType) {
	switch (guardType) {
		case GUARD_TYPE_ANY: return "Any";
		case GUARD_TYPE_HIGH: return "Overhead";
		case GUARD_TYPE_LOW: return "Low";
		case GUARD_TYPE_NONE: return "Unblockable";
		default: return "Unknown";
	}
}

const char* UI::searchCollapsibleSection(const char* collapsibleHeaderName, const char* textEnd) {
	if (!searching) return collapsibleHeaderName;
	searchFieldTitle(collapsibleHeaderName, textEnd);
	pushSearchStack(collapsibleHeaderName);
	return collapsibleHeaderName;
}

void UI::pushSearchStack(const char* name) {
	if (!searching) return;
	searchStack[searchStackCount++] = name;
}

void UI::popSearchStack() {
	if (!searching) return;
	--searchStackCount;
}

static void replaceNewLinesWithSpaces(std::string& str) {
	for (auto it = str.begin(); it != str.end(); ++it) {
		if (*it == '\n') {
			*it = ' ';
		}
	}
}

void UI::searchRawTextMultiResult(const char* txt, const char* txtEnd) {
	const char* txtStart = txt;
	do {
		txt = searchRawText(txt, txtStart, &txtEnd);
		if (!txt) return;
		SearchResult newResult;
		newResult.field = searchField;
		newResult.foundLeft = lastFoundTextLeft;
		replaceNewLinesWithSpaces(newResult.foundLeft);
		newResult.foundMid = lastFoundTextMid;
		newResult.foundRight = lastFoundTextRight;
		replaceNewLinesWithSpaces(newResult.foundRight);
		for (int i = 0; i < searchStackCount; ++i) {
			newResult.searchStack[i] = searchStack[i];
		}
		newResult.searchStackCount = searchStackCount;
		searchResults.push_back(newResult);
		txt += searchStringLen;
	} while (txt != txtEnd);
}

const char* UI::searchRawText(const char* txt, const char* txtStart, const char** txtEnd) {
	const char* result;
	if (*txtEnd == nullptr) {
		result = txt;
		int matchCount = 0;
		do {
			char sourceChar = *result;
			if (sourceChar >= 'A' && sourceChar <= 'Z') sourceChar = 'a' + sourceChar - 'A';
			if (sourceChar == searchString[matchCount]) {
				++matchCount;
				if (matchCount == searchStringLen) {
					result -= searchStringLen - 1;
					break;
				}
			} else {
				matchCount = 0;
			}
			++result;
		} while (*result != '\0');
		if (*result == '\0') result = nullptr;
	} else {
		result = (const char*)sigscanCaseInsensitive((uintptr_t)txt, (uintptr_t)*txtEnd, searchString, searchStringLen, searchStep);
	}
	if (result) {
		if (*txt == '\0') return nullptr;
		
		lastFoundTextRight.clear();
		
		size_t len = 0;
		if (*txtEnd) len = *txtEnd - txt;
		const int showAheadOrBehindLimit = 15;
		
		if (result != txtStart) {
			int limit = showAheadOrBehindLimit;
			const char* ptr = result;
			do {
				--limit;
				ptr = rewindToNextUtf8CharStart(ptr, txtStart);
			} while (limit && ptr != txtStart);
			lastFoundTextLeft.assign(ptr, result);
		} else {
			lastFoundTextLeft.clear();
		}
		
		lastFoundTextMid.assign(result, result + searchStringLen);
		
		if (!*txtEnd || result - txt != len - searchStringLen) {
			int limit = showAheadOrBehindLimit;
			const char* ptr = result + searchStringLen;
			if (!*txtEnd) {
				while (*ptr != '\0' && limit) {
					--limit;
					ptr = skipToNextUtf8CharStart(ptr);
				}
				if (*ptr == '\0') {
					*txtEnd = ptr;
					lastFoundTextRight = result + searchStringLen;
				} else {
					lastFoundTextRight.assign(result + searchStringLen, ptr);
				}
			} else {
				const char* const textEnd = txt + len;
				do {
					--limit;
					ptr = skipToNextUtf8CharStart(ptr, textEnd);
				} while (limit && ptr != textEnd);
				if (ptr != textEnd) {
					lastFoundTextRight.assign(result + searchStringLen, ptr);
				} else {
					lastFoundTextRight = result + searchStringLen;
				}
			}
		} else {
			lastFoundTextRight.clear();
		}
		return result;
	}
	return nullptr;
}

const char* UI::rewindToNextUtf8CharStart(const char* ptr, const char* textStart) {
	while (ptr != textStart) {
		--ptr;
		if ((*ptr & 0b11000000) != 0b10000000) {
			return ptr;
		}
	}
	return ptr;
}

const char* UI::skipToNextUtf8CharStart(const char* ptr) {
	while (true) {
		++ptr;
		if (*ptr == '\0') return ptr;
		if ((*ptr & 0b11000000) != 0b10000000) {
			return ptr;
		}
	}
}

const char* UI::skipToNextUtf8CharStart(const char* ptr, const char* textEnd) {
	while (ptr != textEnd) {
		++ptr;
		if ((*ptr & 0b11000000) != 0b10000000) {
			return ptr;
		}
	}
	return ptr;
}

const char* UI::searchFieldTitle(const char* fieldTitle, const char* textEnd) {
	if (!searching) return fieldTitle;
	searchField = fieldTitle;
	searchRawTextMultiResult(fieldTitle, textEnd);
	return fieldTitle;
}

const char* UI::searchTooltip(const char* tooltip, const char* textEnd) {
	if (!searching) return tooltip;
	searchRawTextMultiResult(tooltip, textEnd);
	return tooltip;
}

const char* UI::searchFieldValue(const char* value, const char* textEnd) {
	if (!searching) return value;
	searchRawTextMultiResult(value, textEnd);
	return value;
}

void UI::searchWindow() {
	ImGui::Begin("Search", &showSearch);
	if (ImGui::InputText("##Search string", searchStringOriginal, sizeof searchStringOriginal, 0, nullptr, nullptr)) {
		showTooFewCharactersError = false;
		int totalCharacters = 0;
		const char* c;
		const char* firstNonWhitespace = nullptr;
		const char* lastNonWhitespace = nullptr;
		char* destination = searchString;
		for (c = searchStringOriginal; *c != '\0'; ++c) {
			if (*c > 32) {
				if (firstNonWhitespace == nullptr) firstNonWhitespace = c;
				lastNonWhitespace = c;
				++totalCharacters;
			}
			char cVal = *c;
			if (cVal >= 'A' && cVal <= 'Z') cVal = 'a' + cVal - 'A';
			*destination = cVal;
			++destination;
		}
		*destination = '\0';
		if (firstNonWhitespace) {
			size_t newStrLen = lastNonWhitespace - firstNonWhitespace + 1;
			if (firstNonWhitespace != searchStringOriginal) {
				memmove(searchString, searchString + (firstNonWhitespace - searchStringOriginal), newStrLen);
				searchString[newStrLen] = '\0';
			}
			searchStringLen = newStrLen;
		} else {
			searchStringLen = 0;
		}
		searchStringOk = totalCharacters > 1;
	}
	imguiActiveTemp = imguiActiveTemp || ImGui::IsItemActive();
	ImGui::SameLine();
	if (ImGui::Button("Search##TheActualButton")) {
		if (!searchStringOk) {
			showTooFewCharactersError = true;
		} else {
			showTooFewCharactersError = false;
			searching = true;
			searchResults.clear();
			sigscanCaseInsensitivePrepare(searchString, searchStringLen, searchStep);
			PERFORMANCE_MEASUREMENT_START
			drawSearchableWindows();
			PERFORMANCE_MEASUREMENT_END(search)
			searching = false;
		}
	}
	if (showTooFewCharactersError) {
		ImGui::PushTextWrapPos(0.F);
		ImGui::TextUnformatted("The search text contains too few characters.");
		ImGui::PopTextWrapPos();
	}
	if (ImGui::BeginTable("##SearchResults",
					3,
					ImGuiTableFlags_Borders
					| ImGuiTableFlags_RowBg
					| ImGuiTableFlags_NoPadOuterX
					| ImGuiTableFlags_Resizable)
	) {
		ImGui::TableSetupColumn("Where", ImGuiTableColumnFlags_WidthStretch, 0.26f);
		ImGui::TableSetupColumn("Field Name", ImGuiTableColumnFlags_WidthStretch, 0.26f);
		ImGui::TableSetupColumn("Context", ImGuiTableColumnFlags_WidthStretch, 0.48f);
		
		ImGui::TableHeadersRow();
		
		ImGui::PushTextWrapPos(0.0f);
		for (const SearchResult& searchResult : searchResults) {
			
			ImGui::TableNextColumn();
			
			char* buf = strbuf;
			size_t bufSize = sizeof strbuf;
			int result;
			*strbuf = '\0';
			for (int i = 0; i < searchResult.searchStackCount; ++i) {
				if (i > 0) {
					result = sprintf_s(buf, bufSize, " - ");
					advanceBuf
				}
				result = sprintf_s(buf, bufSize, "%s", searchResult.searchStack[i].c_str());
				advanceBuf
			}
			if (*strbuf != '\0') {
				ImGui::TextUnformatted(strbuf);
			}
			
			ImGui::TableNextColumn();
			zerohspacing
			if (!searchResult.field.empty()) {
				ImGui::TextUnformatted(searchResult.field.c_str());
			}
			
			ImGui::TableNextColumn();
			const float wrapWidth = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x;
			bool needSameLine = false;
			if (!searchResult.foundLeft.empty()) {
				ImGui::TextUnformatted(searchResult.foundLeft.c_str());
				ImGui::PushStyleColor(ImGuiCol_Text, RED_COLOR);
				drawOneLineOnCurrentLineAndTheRestBelow(wrapWidth, searchResult.foundMid.c_str());
				ImGui::PopStyleColor();
			} else {
				textUnformattedColored(RED_COLOR, searchResult.foundMid.c_str());
			}
			if (!searchResult.foundRight.empty()) {
				drawOneLineOnCurrentLineAndTheRestBelow(wrapWidth, searchResult.foundRight.c_str());
			}
			_zerohspacing
		}
		ImGui::EndTable();
		ImGui::PopTextWrapPos();
	}
	ImGui::End();
}

void UI::printAttackLevel(const DmgCalc& dmgCalc) {
	
	ImGui::TableNextColumn();
	ImGui::TextUnformatted(searchFieldTitle("Attack Level"));
	ImGui::TableNextColumn();
	sprintf_s(strbuf, "%d", dmgCalc.attackLevel);
	ImGui::TextUnformatted(strbuf);
	if (dmgCalc.attackOriginalAttackLevel != dmgCalc.attackLevelForGuard) {
		ImGui::SameLine();
		ImVec4* color;
		int theOtherAttackLevel;
		if (dmgCalc.hitResult == HIT_RESULT_NORMAL) {
			theOtherAttackLevel = dmgCalc.attackLevelForGuard;
		} else {
			theOtherAttackLevel = dmgCalc.attackOriginalAttackLevel;
		}
		if (dmgCalc.attackLevel > theOtherAttackLevel) {
			color = &RED_COLOR;
		} else {
			color = &LIGHT_BLUE_COLOR;
		}
		textUnformattedColored(*color, "(!)");
		if (ImGui::BeginItemTooltip()) {
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			sprintf_s(strbuf, "This attack's level (%d) %s is %s than its attack level (%d) %s.",
				dmgCalc.attackLevel,
				dmgCalc.hitResult == HIT_RESULT_NORMAL ? "on hit" : "on block/armor",
				dmgCalc.attackLevel > theOtherAttackLevel ? "higher" : "lower",
				theOtherAttackLevel,
				dmgCalc.hitResult == HIT_RESULT_NORMAL ? "on block/armor" : "on hit");
			ImGui::TextUnformatted(strbuf);
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}
	}
	
}

int UI::printBaseDamageCalc(const DmgCalc& dmgCalc, int* dmgWithHpScale) {
	// attack level on hit/guard
	// if -1 set to on hit
	// standard damage from attack level
	// but if overriden, use override
	// scale damage with hp
	// base stun gets its value
	// add 5 damage in some dust situation
	// damage 30% scale on OTG
	
	ImGui::TableNextColumn();
	searchFieldTitle("Base Damage");
	zerohspacing
	ImGui::TextUnformatted("Base ");
	ImGui::SameLine();
	textUnformattedColored(YELLOW_COLOR, "Damage");
	_zerohspacing
	
	ImGui::TableNextColumn();
	sprintf_s(strbuf, "%d", dmgCalc.dealtOriginalDamage);
	textUnformattedColored(YELLOW_COLOR, strbuf);
	if (dmgCalc.dealtOriginalDamage != dmgCalc.standardDamage) {
		ImGui::SameLine();
		ImVec4* color;
		if (dmgCalc.dealtOriginalDamage > dmgCalc.standardDamage) {
			color = &RED_COLOR;
		} else {
			color = &LIGHT_BLUE_COLOR;
		}
		textUnformattedColored(*color, "(!)");
		if (ImGui::BeginItemTooltip()) {
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			sprintf_s(strbuf, "This attack's base damage (%d) is %s than the standard base damage (%d) for its attack level (%d)%s.",
				dmgCalc.dealtOriginalDamage,
				dmgCalc.dealtOriginalDamage > dmgCalc.standardDamage ? "higher" : "less",
				dmgCalc.standardDamage,
				dmgCalc.attackLevel,
				dmgCalc.attackOriginalAttackLevel != dmgCalc.attackLevelForGuard
					?
						dmgCalc.hitResult == HIT_RESULT_NORMAL ? " on hit" : " on block/armor"
					:
						"");
			ImGui::TextUnformatted(strbuf);
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}
	}
	
	int x = dmgCalc.dealtOriginalDamage;
	
	if (dmgCalc.scaleDamageWithHp) {
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(searchFieldTitle("HP Damage Scale"));
		const char* tooltip = searchTooltip("This attack scales its damage with the defender's HP. Reaching certain HP thresholds activates more severe scaling.\n"
			"HP < 1/8th max HP: 0% damage;\n"
			"HP < 1/4th max HP: 25% damage;\n"
			"HP < 1/2 max HP: 50% damage.\n"
			"The scaling is only applied to certain attacks.");
		AddTooltip(tooltip);
		
		ImGui::TableNextColumn();
		int oldX = x;
		int scale;
		if (dmgCalc.oldHp < dmgCalc.maxHp / 8) {
			scale = 0;
			x = 0;
		} else if (dmgCalc.oldHp < dmgCalc.maxHp / 4) {
			scale = 25;
			x /= 4;
		} else if (dmgCalc.oldHp < dmgCalc.maxHp / 2) {
			scale = 50;
			x /= 2;
		} else {
			scale = 100;
		}
		sprintf_s(strbuf, "%d%c", scale, '%');
		ImGui::TextUnformatted(strbuf);
		
		ImGui::TableNextColumn();
		zerohspacing
		textUnformattedColored(YELLOW_COLOR, "Damage");
		AddTooltip(tooltip);
		ImGui::SameLine();
		ImGui::TextUnformatted(" * HP Scale");
		AddTooltip(tooltip);
		_zerohspacing
		
		ImGui::TableNextColumn();
		zerohspacing
		sprintf_s(strbuf, "%d * %d%c = ", oldX, scale, '%');
		ImGui::TextUnformatted(strbuf);
		ImGui::SameLine();
		sprintf_s(strbuf, "%d", x);
		textUnformattedColored(YELLOW_COLOR, strbuf);
		_zerohspacing
	}
	
	if (dmgWithHpScale) *dmgWithHpScale = x;
	
	if (dmgCalc.adds5Dmg) {
		ImGui::TableNextColumn();
		searchFieldTitle("Damage + 5");
		zerohspacing
		const char* tooltip = searchTooltip("In certain dust situations, the attack gains 5 extra damage.");
		textUnformattedColored(YELLOW_COLOR, "Damage");
		AddTooltip(tooltip);
		ImGui::SameLine();
		ImGui::TextUnformatted(" + 5");
		AddTooltip(tooltip);
		_zerohspacing
		ImGui::TableNextColumn();
		zerohspacing
		sprintf_s(strbuf, "%d + 5 = ", x);
		ImGui::TextUnformatted(strbuf);
		ImGui::SameLine();
		x += 5;
		sprintf_s(strbuf, "%d", x);
		textUnformattedColored(YELLOW_COLOR, strbuf);
	}
	
	if (dmgCalc.hitResult == HIT_RESULT_NORMAL) {
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(searchFieldTitle("OTG"));
		AddTooltip(searchTooltip("OTG multiplies damage by 30%. Some attacks ignore OTG."));
		ImGui::TableNextColumn();
		int scale;
		int oldX = x;
		const char* yesno;
		if (dmgCalc.isOtg && !dmgCalc.ignoreOtg) {
			scale = 30;
			x = x * 30 / 100;
			yesno = "Yes";
		} else {
			scale = 100;
			yesno = "No";
		}
		sprintf_s(strbuf, "%s (%d%c)", yesno, scale, '%');
		ImGui::TextUnformatted(strbuf);
		
		ImGui::TableNextColumn();
		searchFieldTitle("Damage * OTG");
		zerohspacing
		textUnformattedColored(YELLOW_COLOR, "Damage");
		ImGui::SameLine();
		ImGui::TextUnformatted(" * OTG");
		_zerohspacing
		ImGui::TableNextColumn();
		zerohspacing
		sprintf_s(strbuf, "%d * %d%c = ", oldX, scale, '%');
		ImGui::TextUnformatted(strbuf);
		ImGui::SameLine();
		sprintf_s(strbuf, "%d", x);
		textUnformattedColored(YELLOW_COLOR, strbuf);
		_zerohspacing
	}
	
	return x;
}

static const char* skippedFramesTypeToString(SkippedFramesType type) {
	switch (type) {
		case SKIPPED_FRAMES_SUPERFREEZE: return "superfreeze";
		case SKIPPED_FRAMES_HITSTOP: return "hitstop";
		case SKIPPED_FRAMES_GRAB: return "grab anim";
		case SKIPPED_FRAMES_SUPER: return "super";
		default: return "something";
	}
}

void SkippedFramesInfo::print(bool canBlockButNotFD) const {
	if (overflow) {
		sprintf_s(strbuf, "Since previous displayed frame, skipped %df", elements[0].count);
		ImGui::TextUnformatted(strbuf);
		return;
	}
	if (count == 1) {
		sprintf_s(strbuf, "Since previous displayed frame, skipped %df %s", elements[0].count, skippedFramesTypeToString(elements[0].type));
		ImGui::TextUnformatted(strbuf);
	} else {
		textUnformattedColored(YELLOW_COLOR, "Since previous displayed frame, skipped:");
		for (int i = 0; i < count; ++i) {
			sprintf_s(strbuf, "%df %s;", elements[i].count, skippedFramesTypeToString(elements[i].type));
			ImGui::TextUnformatted(strbuf);
		}
	}
	if (elements[count - 1].type == SKIPPED_FRAMES_SUPERFREEZE && canBlockButNotFD) {
		ImGui::PushStyleColor(ImGuiCol_Text, SLIGHTLY_GRAY);
		ImGui::TextUnformatted("Note that cancelling a dash into FD or covering a jump with FD or using FD in general,"
			" including to avoid chip damage, is impossible on this frame, because it immediately follows a superfreeze."
			" Generally doing anything except throw or normal block/IB is impossible in such situations.");
		ImGui::PopStyleColor();
	}
}

void UI::getFramebarDrawData(std::vector<BYTE>& dData) {
	dData.clear();
	if (!drawData) return;
	
	ImDrawListBackup* lists[2] { nullptr };
	int listsCount = 0;
	if (drewFramebar) {
		lists[listsCount++] = (ImDrawListBackup*)framebarWindowDrawDataCopy.data();
	}
	if (drewFrameTooltip) {
		lists[listsCount++] = (ImDrawListBackup*)framebarTooltipDrawDataCopy.data();
	};
	if (!listsCount) return;
	
	makeRenderDataFromDrawLists(dData, (const ImDrawData*)drawData, lists, listsCount);
}

void printExtraHitstunTooltip(int amount) {
	if (ImGui::BeginItemTooltip()) {
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		printExtraHitstunText(amount);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

void printExtraHitstunText(int amount) {
	sprintf_s(strbuf, "The extra %d hitstun is applied from a floor bounce.", amount);
	ImGui::TextUnformatted(strbuf);
}

void printExtraBlockstunTooltip(int amount) {
	if (ImGui::BeginItemTooltip()) {
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		printExtraBlockstunText(amount);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

void printExtraBlockstunText(int amount) {
	sprintf_s(strbuf, "The extra %d blockstun is applied from landing while in blockstun.", amount);
	ImGui::TextUnformatted(strbuf);
}

const char* comborepr(std::vector<int>& combo) {
	const char* repr = settings.getComboRepresentation(combo);
	if (repr[0] == '\0') return "<not set>";
	return repr;
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
	ImGuiIO& io = ImGui::GetIO();
	float settingsFramebarHeight = (float)settings.framebarHeight * io.DisplaySize.y / 720.F;
	if (settingsFramebarHeight < 5.F) {
		settingsFramebarHeight = 5.F;
	}
	const float scale = settingsFramebarHeight / 19.F;
	
	static const float outerBorderThicknessUnscaled = 2.F;
	float outerBorderThickness = outerBorderThicknessUnscaled * scale;
	if (outerBorderThickness < 2.F) outerBorderThickness = 2.F;
	drawFramebars_frameItselfHeight = settingsFramebarHeight - outerBorderThickness - outerBorderThickness;
	static const float frameNumberHeightOriginal = 11.F;
	static const float frameMarkerSideHeightOriginal = 3.F;
	float frameMarkerHeight = frameMarkerHeightOriginal * drawFramebars_frameItselfHeight / frameHeightOriginal;
	if (frameMarkerHeight < 5.F) {
		frameMarkerHeight = 5.F;
	}
	float frameMarkerSideHeight = frameMarkerSideHeightOriginal * frameMarkerHeight / frameMarkerHeightOriginal;
	static const float markerPaddingHeightUnscaled = -1.F;
	const float markerPaddingHeight = markerPaddingHeightUnscaled * scale;
	static const float paddingBetweenFramebarsOriginalUnscaled = 5.F;
	const float paddingBetweenFramebarsOriginal = paddingBetweenFramebarsOriginalUnscaled * scale;
	static const float paddingBetweenFramebarsMinUnscaled = 3.F;
	const float paddingBetweenFramebarsMin = paddingBetweenFramebarsMinUnscaled * scale;
	static const float paddingBetweenTextAndFramebarUnscaled = 5.F;
	const float paddingBetweenTextAndFramebar = paddingBetweenTextAndFramebarUnscaled * scale;
	static const float textPaddingUnscaled = 2.F;
	const float textPadding = textPaddingUnscaled * scale;
	const float firstFrameHeightScaled = firstFrameHeight * scale;
	static const float firstFrameHeightDiff = firstFrameHeightScaled - drawFramebars_frameItselfHeight;
	drawFramebars_frameWidthScaled = frameWidthOriginal * drawFramebars_frameItselfHeight / frameHeightOriginal;
	static const float frameNumberPaddingY = 2.F;
	static const float highlighterWidthUnscaled = 2.F;
	const float highlighterWidth = highlighterWidthUnscaled * scale;
	static const float hoveredFrameHighlightPaddingXUnscaled = 3.F;
	const float hoveredFrameHighlightPaddingX = hoveredFrameHighlightPaddingXUnscaled * scale;
	static const float hoveredFrameHighlightPaddingYUnscaled = 3.F;
	const float hoveredFrameHighlightPaddingY = hoveredFrameHighlightPaddingYUnscaled * scale;
	static const float framebarCurrentPositionHighlighterStickoutDistanceUnscaled = 2.F;
	const float framebarCurrentPositionHighlighterStickoutDistance = framebarCurrentPositionHighlighterStickoutDistanceUnscaled * scale;
	drawFramebars_innerBorderThickness = innerBorderThicknessUnscaled * scale;
	if (drawFramebars_innerBorderThickness < 1.F) drawFramebars_innerBorderThickness = 1.F;
	drawFramebars_innerBorderThicknessHalf = drawFramebars_innerBorderThickness * 0.5F;
	
	float maxTopPadding;
	if (!showFirstFrames) {
		maxTopPadding = 0.F;
	} else {
		maxTopPadding = firstFrameHeightDiff * 0.5F;
		if (maxTopPadding < 0.F) maxTopPadding = 0.F;
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
		+ drawFramebars_frameItselfHeight
		+ outerBorderThickness;
	
	std::vector<const EntityFramebar*> framebars;
	const bool eachProjectileOnSeparateFramebar = settings.eachProjectileOnSeparateFramebar;
	size_t playerFramebarsCount = endScene.playerFramebars.size();
	if (playerFramebarsCount > 2) playerFramebarsCount = 2;
	if (eachProjectileOnSeparateFramebar) {
		framebars.resize(playerFramebarsCount + endScene.projectileFramebars.size(), nullptr);
	} else {
		framebars.resize(playerFramebarsCount + endScene.combinedFramebars.size(), nullptr);
	}
	if (framebars.empty()) return;
	int framebarsCount = 0;
	int playerFramebarLimit = 2;
	for (const PlayerFramebars& entityFramebar : endScene.playerFramebars) {
		if (!playerFramebarLimit) break;
		framebars[framebarsCount++] = (const EntityFramebar*)&entityFramebar;
		--playerFramebarLimit;
	}
	for (int i = 0; i < 2; ++i) {
		if (eachProjectileOnSeparateFramebar) {
			for (const ProjectileFramebar& entityFramebar : endScene.projectileFramebars) {
				if (entityFramebar.playerIndex == i) {
					framebars[framebarsCount++] = (const EntityFramebar*)&entityFramebar;
				}
			}
		} else {
			for (const CombinedProjectileFramebar& entityFramebar : endScene.combinedFramebars) {
				if (entityFramebar.playerIndex == i) {
					framebars[framebarsCount++] = (const EntityFramebar*)&entityFramebar;
				}
			}
		}
	}
	if (eachProjectileOnSeparateFramebar) {
		for (const ProjectileFramebar& entityFramebar : endScene.projectileFramebars) {
			if (entityFramebar.playerIndex != 0 && entityFramebar.playerIndex != 1) {
				framebars[framebarsCount++] = (const EntityFramebar*)&entityFramebar;
			}
		}
	} else {
		for (const CombinedProjectileFramebar& entityFramebar : endScene.combinedFramebars) {
			if (entityFramebar.playerIndex != 0 && entityFramebar.playerIndex != 1) {
				framebars[framebarsCount++] = (const EntityFramebar*)&entityFramebar;
			}
		}
	}
	if (framebarsCount != framebars.size()) {
		framebars.erase(framebars.begin() + framebarsCount, framebars.end());
	}
	
	float framebarsPaddingYTotal = 0.F;
	if (framebarsCount) {
		framebarsPaddingYTotal = (float)(framebarsCount - 1) * paddingBetweenFramebars;
	}
	ImGui::SetNextWindowSize({ 880.F / 1280.F * io.DisplaySize.x,
		2.F  // imgui window padding
		+ 1.F
		+ maxTopPadding
		+ oneFramebarHeight
		* 2.F
		+ (framebarsCount <= 1 ? 0.F : paddingBetweenFramebars)
		+ bottomPadding
		+ 1.F
		+ 2.F  // imgui window padding
	}, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowContentSize({ 0.F,
		// don't add imgui window padding here
		+ 1.F
		+ maxTopPadding
		+ oneFramebarHeight
		* (float)framebarsCount
		+ framebarsPaddingYTotal
		+ bottomPadding
		+ 1.F
		// don't add imgui window padding here
	});
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 8.F, 2.F });
	drewFramebar = true;
	ImGui::Begin("Framebar", nullptr,
		ImGuiWindowFlags_NoBackground
		| ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoTitleBar
		| (isDragging ? ImGuiWindowFlags_NoMove : 0)
		| ImGuiWindowFlags_NoBringToFrontOnFocus
		| ImGuiWindowFlags_NoFocusOnAppearing);
	bool drawFullBorder = ImGui::IsMouseDown(ImGuiMouseButton_Left)
		&& ImGui::IsMouseHoveringRect({ 0.F, 0.F }, { FLT_MAX, FLT_MAX }, true)
		&& ImGui::IsWindowFocused();
	ImGui::PopStyleVar();
	if (scale > 1.F) {
		ImGui::SetWindowFontScale(scale);
	} else {
		ImGui::SetWindowFontScale(1.F);
	}
	drawFramebars_windowPos = ImGui::GetWindowPos();
	const float windowWidth = ImGui::GetWindowWidth();
	const float windowHeight = ImGui::GetWindowHeight();
	
	drawFramebars_drawList = ImGui::GetWindowDrawList();
	
	if (ImGui::GetScrollMaxY() > 0.001F || drawFullBorder) {
		drawFramebars_drawList->AddLine({ drawFramebars_windowPos.x, drawFramebars_windowPos.y + 1.F },
			{ drawFramebars_windowPos.x + windowWidth, drawFramebars_windowPos.y + 1.F },
			ImGui::GetColorU32(IM_COL32(0, 0, 0, 255)),
			1.F);
		drawFramebars_drawList->AddLine({ drawFramebars_windowPos.x, drawFramebars_windowPos.y + windowHeight - 2.F },
			{ drawFramebars_windowPos.x + windowWidth, drawFramebars_windowPos.y + windowHeight - 2.F },
			ImGui::GetColorU32(IM_COL32(0, 0, 0, 255)),
			1.F);
		if (drawFullBorder) {
			drawFramebars_drawList->AddLine({ drawFramebars_windowPos.x + 1.F, drawFramebars_windowPos.y + 1.F },
				{ drawFramebars_windowPos.x + 1.F, drawFramebars_windowPos.y + windowHeight - 2.F },
				ImGui::GetColorU32(IM_COL32(0, 0, 0, 255)),
				1.F);
			drawFramebars_drawList->AddLine({ drawFramebars_windowPos.x + windowWidth - 2.F, drawFramebars_windowPos.y + 1.F },
				{ drawFramebars_windowPos.x + windowWidth - 2.F, drawFramebars_windowPos.y + windowHeight - 2.F },
				ImGui::GetColorU32(IM_COL32(0, 0, 0, 255)),
				1.F);
		}
	}
	
	const float framesX = drawFramebars_windowPos.x
		+ paddingBetweenTextAndFramebar
		+ outerBorderThickness;
	const float framesXEnd = drawFramebars_windowPos.x
		+ ImGui::GetContentRegionMax().x
		+ 5.F  // window padding? (it was 8. close enough)
		- outerBorderThickness
		+ drawFramebars_innerBorderThickness;
	
	const float scrollY = ImGui::GetScrollY();
	drawFramebars_y = drawFramebars_windowPos.y 
		+ 2.F  // imgui window padding
		+ 1.F
		+ maxTopPadding
		+ outerBorderThickness
		- scrollY;
	
	static const float framesCountFloat = (float)_countof(Framebar::frames);
	const int framebarPosition = settings.neverIgnoreHitstop ? endScene.getFramebarPositionHitstop() : endScene.getFramebarPosition();
	ImU32 tintDarker = ImGui::GetColorU32(IM_COL32(128, 128, 128, 255));
	
	
	float frameNumberHeightDiff = frameHeightOriginal - drawFramebars_frameItselfHeight;
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
	const float frameNumberHeight = drawFramebars_frameItselfHeight - frameNumberPaddingYUse - frameNumberPaddingYUse;
	
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
		
		highlighterStartX = preppedDims[EntityFramebar::confinePos(framebarPosition + 1)].x - drawFramebars_innerBorderThickness;
		highlighterEndX = highlighterStartX + highlighterWidth;
		
	}
	
	static float P1P2TextSizeWithSpace;
	static float P1P2TextSize;
	static bool P1P2TextSizeCalculated = false;
	if (!P1P2TextSizeCalculated) {
		P1P2TextSizeCalculated = true;
		P1P2TextSizeWithSpace = ImGui::CalcTextSize("P1 ").x;
		P1P2TextSize = ImGui::CalcTextSize("P1").x;
	}
	
	drawFramebars_frameArtArray = settings.useColorblindHelp ? frameArtColorblind : frameArtNonColorblind;
	const FrameMarkerArt* frameMarkerArtArray = settings.useColorblindHelp ? frameMarkerArtColorblind : frameMarkerArtNonColorblind;
	const FrameMarkerArt& strikeInvulMarker = frameMarkerArtArray[MARKER_TYPE_STRIKE_INVUL];
	const FrameMarkerArt& throwInvulMarker = frameMarkerArtArray[MARKER_TYPE_THROW_INVUL];
	
	drawFramebars_hoveredFrameIndex = -1;
	const float currentPositionHighlighterStartY = drawFramebars_y;
	
	const bool showPlayerInFramebarTitle = settings.showPlayerInFramebarTitle;
	const int framebarTitleCharsMax = settings.framebarTitleCharsMax;
	const std::vector<SkippedFramesInfo>& skippedFrames = endScene.getSkippedFrames(settings.neverIgnoreHitstop);
	
	if (showPlayerInFramebarTitle || framebarTitleCharsMax > 0) {
		ImGui::PushClipRect(
			{
				0.F,
				drawFramebars_windowPos.y
			},
			{
				200000.F,
				drawFramebars_windowPos.y + windowHeight
			}, false);
		float titleY = drawFramebars_y + scrollY;
		const float lineHeight = ImGui::GetTextLineHeightWithSpacing();
		const float textPaddingY = (oneFramebarHeight - lineHeight) * 0.5F;
		const bool dontTruncateFramebarTitles = settings.dontTruncateFramebarTitles;
		const bool allFramebarTitlesDisplayToTheLeft = settings.allFramebarTitlesDisplayToTheLeft;
		const bool useSlang = settings.useSlangNames;
		for (const EntityFramebar* entityFramebarPtr : framebars) {
			const EntityFramebar& entityFramebar = *entityFramebarPtr;
			const char* title = nullptr;
			const char* titleFull = nullptr;
			static std::string titleShortStr;
			if (framebarTitleCharsMax <= 0) {
				title = nullptr;
			} else {
				const char* selectedTitle = nullptr;
				if (eachProjectileOnSeparateFramebar) {
					if (useSlang) {
						selectedTitle = entityFramebar.titleSlangUncombined;
					}
					if (!selectedTitle) selectedTitle = entityFramebar.titleUncombined;
					titleFull = entityFramebar.titleUncombined;
				}
				if (!selectedTitle) {
					if (useSlang) {
						selectedTitle = entityFramebar.titleSlang;
					}
					if (!selectedTitle) selectedTitle = entityFramebar.titleShort;
				}
				if (!titleFull && useSlang) {
					titleFull = entityFramebar.titleShort;
				}
				if (selectedTitle) {
					if (!dontTruncateFramebarTitles) {
						int len;
						int bytesUpToMax;
						int cpsTotal;
						EntityFramebar::utf8len(selectedTitle, &len, &cpsTotal, framebarTitleCharsMax, &bytesUpToMax);
						if (bytesUpToMax != len) {
							if (!titleFull) titleFull = selectedTitle;
							titleShortStr.assign(selectedTitle, bytesUpToMax);
							title = titleShortStr.c_str();
						} else {
							title = selectedTitle;
						}
					} else {
						title = selectedTitle;
					}
				}
			}
			if (!title) {
				title = "???";
			}
			if (entityFramebar.titleFull && *entityFramebar.titleFull != '\0') titleFull = entityFramebar.titleFull;
			float textX;
			bool isOnTheLeft = true;
			ImVec2 textSize;
			if (entityFramebar.playerIndex == 0 || allFramebarTitlesDisplayToTheLeft) {
				if (!title) {
					textSize.x = 0.F;
				} else {
					textSize = ImGui::CalcTextSize(title);
				}
				textX = -textPadding;
			} else {
				textX = windowWidth + textPadding;
				isOnTheLeft = false;
			}
			
			const float textY = titleY
				- outerBorderThickness
				+ textPaddingY
				- drawFramebars_windowPos.y;
			
			bool hoveredExtra = false;
			
			if (showPlayerInFramebarTitle) {
				
				ImVec4* P1P2Clr = P1P2_COLOR[entityFramebar.playerIndex];
				ImVec4* P1P2OutlineClr = P1P2_OUTLINE_COLOR[entityFramebar.playerIndex];
				
				if (!entityFramebar.belongsToPlayer()) {
					const char* P1P2Str;
					if (isOnTheLeft) {
						if (entityFramebar.playerIndex == 0) {
							P1P2Str = " P1";
						} else {
							P1P2Str = " P2";
						}
					} else {
						if (entityFramebar.playerIndex == 0) {
							P1P2Str = "P1 ";
						} else {
							P1P2Str = "P2 ";
						}
					}
					outlinedText({
							isOnTheLeft
								? textX - P1P2TextSizeWithSpace
								: textX,
							textY
						},
						P1P2Str,
						P1P2Clr,
						P1P2OutlineClr);
					
					if (title) {
						if (titleFull) {
							hoveredExtra = ImGui::IsItemHovered(ImGuiHoveredFlags_RectOnly);
						}
						
						outlinedText({
								(
									isOnTheLeft
										? textX - P1P2TextSizeWithSpace - textSize.x
										: textX + P1P2TextSizeWithSpace
								),
								textY
							}, title);
					}
				} else {
					const char* P1P2Str;
					if (entityFramebar.playerIndex == 0) {
						P1P2Str = "P1";
					} else {
						P1P2Str = "P2";
					}
					
					outlinedText({
							isOnTheLeft
								? textX - P1P2TextSize
								: textX,
							textY
						},
						P1P2Str,
						P1P2Clr,
						P1P2OutlineClr);
				}
			} else if (title) {
				outlinedText({
					isOnTheLeft
						? textX - textSize.x
						: textX,
					textY
				},
				title);
			}
			if (titleFull
					&& (ImGui::IsItemHovered(ImGuiHoveredFlags_RectOnly) || hoveredExtra)
					&& !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)
					&& ImGui::BeginTooltip()) {
				ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
				if (entityFramebar.belongsToPlayer()) {
					ImGui::TextUnformatted(titleFull);
				} else {
					ImGui::Text("Player %d's %s", entityFramebar.playerIndex + 1, titleFull);
				}
				ImGui::PopTextWrapPos();
				ImGui::EndTooltip();
			}
			titleY += oneFramebarHeight + paddingBetweenFramebars;
		}
		ImGui::PopClipRect();
	}
	
	for (const EntityFramebar* entityFramebarPtr : framebars) {
		const EntityFramebar& entityFramebar = *entityFramebarPtr;
		const FramebarBase& framebar = settings.neverIgnoreHitstop ? entityFramebar.getHitstop() : entityFramebar.getMain();
		
		if (framesXEnd > framesX) {
			drawFramebars_drawList->AddRectFilled(
				{ framesX - outerBorderThickness, drawFramebars_y - outerBorderThickness },
				{ framesXEnd, drawFramebars_y - outerBorderThickness + oneFramebarHeight },
				ImGui::GetColorU32(IM_COL32(0, 0, 0, 255)));
		}
		
		if (framesXEnd > framesX) {
			
			if (entityFramebar.belongsToPlayer()) {
				drawPlayerFramebar((const PlayerFramebar&)framebar, preppedDims, framebarPosition, tintDarker, entityFramebar.playerIndex, skippedFrames);
			} else {
				drawProjectileFramebar((const Framebar&)framebar, preppedDims, framebarPosition, tintDarker, skippedFrames);
			}
			
			{
				
				float frameNumberYTop;
				float frameNumberYBottom;
				{
					float frameNumberYTopTemp = drawFramebars_y + frameNumberPaddingYUse;
					frameNumberYBottom = frameNumberYTopTemp + frameNumberHeight;
					if (frameNumberPaddingYUse > 0.01F) {
						frameNumberYTop = std::floor(frameNumberYTopTemp);
						frameNumberYBottom -= frameNumberYTopTemp - frameNumberYTop;
					} else {
						frameNumberYTop = frameNumberYTopTemp;
					}
				}
				
				if (entityFramebar.belongsToPlayer()) {
					drawDigits<PlayerFramebar, PlayerFrame>((const PlayerFramebar&)framebar, framebarPosition, preppedDims, frameNumberYTop, frameNumberYBottom);
				} else {
					drawDigits<Framebar, Frame>((const Framebar&)framebar, framebarPosition, preppedDims, frameNumberYTop, frameNumberYBottom);
				}
			}
			
			if (showStrikeInvulOnFramebar || showSuperArmorOnFramebar || showThrowInvulOnFramebar) {
				
				const float yTopRow = drawFramebars_y - markerPaddingHeight - frameMarkerHeight;
				const float markerEndY = yTopRow + frameMarkerHeight;
				const float thisMarkerWidthPremult = frameMarkerWidthOriginal / frameWidthOriginal;
				
				if (entityFramebar.belongsToPlayer()) {
					const PlayerFramebar& framebarCast = (const PlayerFramebar&)framebar;
					for (int i = 0; i < _countof(Framebar::frames); ++i) {
						const PlayerFrame& frame = framebarCast[i];
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
							drawFramebars_drawList->AddImage((ImTextureID)TEXID_FRAMES,
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
							drawFramebars_drawList->AddImage((ImTextureID)TEXID_FRAMES,
								markerStart,
								markerEnd,
								markerArt.uvStart,
								markerArt.uvEnd,
								tint);
						}
						
						if (frame.throwInvulInGeneral && showThrowInvulOnFramebar) {
							
							markerStart.y = drawFramebars_y + drawFramebars_frameItselfHeight + markerPaddingHeight;
							markerEnd.y = markerStart.y + frameMarkerHeight;
							
							drawFramebars_drawList->AddImage((ImTextureID)TEXID_FRAMES,
								markerStart,
								markerEnd,
								throwInvulMarker.uvStart,
								throwInvulMarker.uvEnd,
								tint);
						}
					}
				}
			}
			
			if (showFirstFrames) {
				
				const float firstFrameTopY = drawFramebars_y - outerBorderThickness - firstFrameHeightDiff * 0.5F;
				const float firstFrameBottomY = firstFrameTopY + firstFrameHeightScaled;
				
				if (entityFramebar.belongsToPlayer()) {
					drawFirstFrames<PlayerFramebar, PlayerFrame>((const PlayerFramebar&)framebar, framebarPosition, preppedDims, firstFrameTopY, firstFrameBottomY);
				} else {
					drawFirstFrames<Framebar, Frame>((const Framebar&)framebar, framebarPosition, preppedDims, firstFrameTopY, firstFrameBottomY);
				}
			}
		}
		
		drawFramebars_y += oneFramebarHeight + paddingBetweenFramebars;
	}
	
	if (framesXEnd > framesX) {
		float highlighterStartY = currentPositionHighlighterStartY
					- outerBorderThickness
					- framebarCurrentPositionHighlighterStickoutDistance;
		
		float highlighterEndY = drawFramebars_y
					- outerBorderThickness
					- paddingBetweenFramebars
					+ framebarCurrentPositionHighlighterStickoutDistance;
		
		drawFramebars_drawList->AddRectFilled(
			{
				highlighterStartX,
				highlighterStartY
			},
			{
				highlighterEndX,
				highlighterEndY
			},
			ImGui::GetColorU32(IM_COL32(255, 255, 255, 255)));
		
		bool needInitStitchParams = true;
		static const float distanceBetweenStitches = 4.F;
		static const float stitchSize = 3.F;
		static int visiblePreviousStitchesAtTheTopOfAStitch = -1;
		static const float stitchThickness = 1.F;
		float stitchStartYWithWindowClipping;
		int stitchCount;
		
		for (int i = 0; i < _countof(Framebar::frames); ++i) {
			
			const SkippedFramesInfo& skippedInfo = skippedFrames[i];
			if (!skippedInfo.count) {
				continue;
			}
			SkippedFramesType skippedType = skippedInfo.elements[skippedInfo.count - 1].type;
			if (!(skippedInfo.overflow || skippedType == SKIPPED_FRAMES_GRAB || skippedType == SKIPPED_FRAMES_SUPER)) {
				continue;
			}
			if (needInitStitchParams) {
				if (visiblePreviousStitchesAtTheTopOfAStitch == -1) {
					if (distanceBetweenStitches >= stitchSize) {
						visiblePreviousStitchesAtTheTopOfAStitch = 0;
					} else {
						visiblePreviousStitchesAtTheTopOfAStitch = (int)std::floor(stitchSize / distanceBetweenStitches);
					}
				}
				needInitStitchParams = false;
				float windowViewableRegionStartY = drawFramebars_windowPos.y;
				float windowViewableRegionEndY = drawFramebars_windowPos.y + windowHeight;
				stitchStartYWithWindowClipping = highlighterStartY;
				float stitchEndYWithWindowClipping = highlighterEndY;
				if (stitchStartYWithWindowClipping < windowViewableRegionStartY) {
					float countFitIn = std::floor((windowViewableRegionStartY - stitchStartYWithWindowClipping) / distanceBetweenStitches);
					stitchStartYWithWindowClipping += countFitIn * distanceBetweenStitches;
				}
				if (stitchEndYWithWindowClipping > windowViewableRegionEndY) {
					stitchEndYWithWindowClipping = windowViewableRegionEndY;
				}
				stitchCount = (int)std::ceil((stitchEndYWithWindowClipping - stitchStartYWithWindowClipping) / distanceBetweenStitches);
				if (stitchCount <= 0) break;
				stitchStartYWithWindowClipping -= (float)visiblePreviousStitchesAtTheTopOfAStitch * distanceBetweenStitches;
				stitchCount += visiblePreviousStitchesAtTheTopOfAStitch;
			}
			float stitchY = stitchStartYWithWindowClipping;
			float stitchX = preppedDims[i].x - stitchSize * 0.5F;
			float stitchEndX = preppedDims[i].x + stitchSize * 0.5F;
			for (int j = 0; j < stitchCount; ++j) {
				drawFramebars_drawList->AddLine(
					{
						stitchX,
						stitchY
					},
					{
						stitchX + stitchSize,
						stitchY + stitchSize
					},
					ImGui::GetColorU32(IM_COL32(255, 255, 255, 255)),
					stitchThickness);
				stitchY += distanceBetweenStitches;
			}
		}
	}
	
	if (drawFramebars_hoveredFrameIndex != -1) {
		drewFrameTooltip = true;
		const FrameDims& dims = preppedDims[drawFramebars_hoveredFrameIndex];
		drawFramebars_drawList->PushClipRect(ImVec2{ 0.F, 0.F }, ImVec2{ 10000.F, 10000.F }, false);
		drawFramebars_drawList->AddRectFilled(
			{
				dims.x - hoveredFrameHighlightPaddingX,
				drawFramebars_hoveredFrameY - hoveredFrameHighlightPaddingY
			},
			{
				dims.x + dims.width + hoveredFrameHighlightPaddingX,
				drawFramebars_hoveredFrameY + oneFramebarHeight + hoveredFrameHighlightPaddingY - 1.F
			},
			ImGui::GetColorU32(IM_COL32(255, 255, 255, 60)));
		drawFramebars_drawList->PopClipRect();
	}
	ImGui::End();
	if (needSplitFramebar) {
		copyDrawList(*(ImDrawListBackup*)framebarWindowDrawDataCopy.data(), drawFramebars_drawList);
		drawFramebars_drawList->CmdBuffer.clear();
		drawFramebars_drawList->IdxBuffer.clear();
		drawFramebars_drawList->VtxBuffer.clear();
	}
}
