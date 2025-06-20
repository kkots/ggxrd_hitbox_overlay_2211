#include "pch.h"
#include "UI.h"
#include "Keyboard.h"
#include "logging.h"
#include "Settings.h"
#include "CustomWindowMessages.h"
#include "WError.h"
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
#include "findMoveByName.h"
#include "Hardcode.h"
#include "InputNames.h"
#include "ImGuiCorrecter.h"
#include <array>
#include "SpecificFramebarIds.h"

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
static char strbuf2[512];
static char* strbufs[2] { strbuf, strbuf2 };
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
static const float powerupWidthOriginal = 7.F;
static const float powerupHeightOriginal = 7.F;
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
// The total number of frames that can be displayed
int drawFramebars_framesCount;
// The framebar position with horizontal scrolling already applied to it
// Is in [0;_countof(Framebar::frames)] coordinate space, its range of possible values is [0;_countof(Framebar::frames)-1]
int drawFramebars_framebarPosition;
// Is in [0;drawFramebars_framesCount] coordinate space, its range of possible values is [0;drawFramebars_framesCount-1]
// It is the result of converting drawFramebars_framebarPosition from [0;_countof(Framebar::frames)] coordinate space to [0;drawFramebars_framesCount] coordinate space
int drawFramebars_framebarPositionDisplay;
static int inline iterateVisualFramesFrom0_getInitialInternalInd() {
	int result = drawFramebars_framebarPosition - drawFramebars_framebarPositionDisplay;
	if (result < 0) {
		return result + _countof(Framebar::frames);
	} else {
		return result;
	}
}
static void inline incrementInternalInd(int& internalInd) {
	if (internalInd == drawFramebars_framebarPosition) {
		internalInd = drawFramebars_framebarPosition - drawFramebars_framesCount + 1;
		if (internalInd < 0) {
			internalInd += _countof(Framebar::frames);
		}
	} else if (internalInd == _countof(Framebar::frames) - 1) {
		internalInd = 0;
	} else {
		++internalInd;
	}
}
const char thisHelpTextWillRepeat[] = "Shows available gatlings, whiff cancels, and whether the jump and the special cancels are available,"
					" per range of frames for this player.\n"
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
static std::array<std::vector<NameDuration>, 2> nameParts;

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
static GGIcon cogwheelIcon = coordsToGGIcon(1, 379, 41, 41);
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
static float getItemSpacing();
static GGIcon DISolIcon = coordsToGGIcon(172, 1096, 56, 35);
static GGIcon DISolIconRectangular = coordsToGGIcon(179, 1095, 37, 37);
static void outlinedText(ImVec2 pos, const char* text, ImVec4* color = nullptr, ImVec4* outlineColor = nullptr, bool highQuality = false);
static void outlinedTextJustTheOutline(ImVec2 pos, const char* text, ImVec4* outlineColor = nullptr, bool highQuality = false);
static void outlinedTextRaw(ImDrawList* drawList, ImVec2 pos, const char* text, ImVec4* color = nullptr, ImVec4* outlineColor = nullptr, bool highQuality = false);
static int printCancels(const FixedArrayOfGatlingOrWhiffCancelInfos<30>& cancels, float maxY);
static int printInputs(char* buf, size_t bufSize, const InputType* inputs);
static void printInputs(char*&buf, size_t& bufSize, InputName** motions, int motionCount, InputName** buttons, int buttonsCount);
static void printChippInvisibility(int current, int max);
static void textUnformattedColored(ImVec4 color, const char* str, const char* strEnd = nullptr);
static void yellowText(const char* str, const char* strEnd = nullptr);
static void drawOneLineOnCurrentLineAndTheRestBelow(float wrapWidth,
		const char* str,
		const char* strEnd = nullptr,
		bool needSameLine = true,
		bool needManualMultilineOutput = false,
		bool isLastLine = true);
static void printActiveWithMaxHit(const ActiveDataArray& active, const MaxHitInfo& maxHit, int hitOnFrame);
static void drawPlayerIconInWindowTitle(int playerIndex);
static void drawPlayerIconInWindowTitle(GGIcon& icon);
static void drawTextInWindowTitle(const char* txt);
static bool printMoveFieldTooltip(const PlayerInfo& player);
static bool printMoveField(const PlayerInfo& player);
static void headerThatCanBeClickedForTooltip(const char* title, bool* windowVisibilityVar, bool makeTooltip);
static void prepareLastNames(const char** lastName, const PlayerInfo& player, bool disableSlang,
							int* lastNameDuration);
static bool printNameParts(int playerIndex, std::vector<NameDuration>& elems, char* buf, size_t bufSize);
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
			{ \
				const char* aStr = (a); \
				float w = ImGui::CalcTextSize(aStr).x; \
				if (w > ImGui::GetContentRegionAvail().x) { \
					ImGui::TextWrapped("%s", aStr); \
				} else { \
					if (i == 0) RightAlign(w); \
					ImGui::TextUnformatted(aStr); \
				} \
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
			hook_GetKeyStatePtr = hook_GetKeyState;
			void** hook_GetKeyStatePtrPtr = &hook_GetKeyStatePtr;
			std::vector<char> newBytes;
			newBytes.resize(4);
			memcpy(newBytes.data(), &hook_GetKeyStatePtrPtr, 4);
			detouring.patchPlace(GetKeyStateCallPlace, newBytes);
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
		"Recovery: an attack's active frames are already over or projectile active frames have started. Can't perform another attack."
		blockFDNotice);
	addFrameArt(hModule, FT_RECOVERY_HAS_GATLINGS,
		IDB_RECOVERY_FRAME_HAS_GATLINGS, recoveryFrameHasGatlings,
		IDB_RECOVERY_FRAME_HAS_GATLINGS_NON_COLORBLIND, recoveryFrameHasGatlingsNonColorblind,
		"Recovery, but can gatling or cancel or release: an attack's active frames are already over 9or projectile active frames have started),"
		" but can gatling or cancel into some other attacks"
		" or release the button to end the attack sooner."
		blockFDNotice);
	addFrameArt(hModule, FT_RECOVERY_CAN_ACT,
		IDB_RECOVERY_FRAME_CAN_ACT, recoveryFrameCanAct,
		IDB_RECOVERY_FRAME_CAN_ACT_NON_COLORBLIND, recoveryFrameCanActNonColorblind,
		"Recovery, but can gatling or cancel or more: an attack's active frames are already over (or projectile active frames have started),"
		" but can gatling into some other attacks or do other actions."
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
	addFrameMarkerArt(hModule, MARKER_TYPE_OTG, IDB_OTG, OTGFrame);
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
	addFrameArt(hModule, FT_EDDIE_IDLE,
		IDB_EDDIE_IDLE_FRAME, eddieIdleFrame,
		IDB_EDDIE_IDLE_FRAME_NON_COLORBLIND, eddieIdleFrameNonColorblind,
		"Eddie is idle.");
	addFrameArt(hModule, FT_BACCHUS_SIGH,
		IDB_BACCHUS_SIGH_FRAME, bacchusSighFrame,
		IDB_BACCHUS_SIGH_FRAME_NON_COLORBLIND, bacchusSighFrameNonColorblind,
		"Bacchus Sigh is ready to hit the opponent when in range.");
	
	addImage(hModule, IDB_POWERUP, powerupFrame);
	
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
		
		FrameArt& idleSuperfreeze = theArray[FT_IDLE_NO_DISPOSE];
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
		startupAnytimeNow.description = "Startup of a holdable move: can release the button any time to either attack or cancel the move."
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
		startupAnytimeNowCanAct.description = "Startup of a holdable move: can release the button any time to either attack or cancel the move,"
			" and can also cancel into other moves."
			" Can't block or FD or perform normal attacks.";
		
		FrameArt& airborneIdleCanGroundBlock = theArray[FT_IDLE_AIRBORNE_BUT_CAN_GROUND_BLOCK];
		airborneIdleCanGroundBlock = arrays[i][FT_IDLE_CANT_FD];
		airborneIdleCanGroundBlock.description = "Idle while airborne on the pre-landing (last airborne) frame:"
			" can block grounded on this frame and regular (without FD) block ground attacks that require FD to be blocked in the air."
			" Can attack, block and FD.";
		
		FrameArt& eddieStartup = theArray[FT_EDDIE_STARTUP];
		eddieStartup = arrays[i][FT_STARTUP];
		eddieStartup.description = "Eddie's attack is in startup.";
		
		FrameArt& eddieActive = theArray[FT_EDDIE_ACTIVE];
		eddieActive = arrays[i][FT_ACTIVE];
		eddieActive.description = "Eddie's attack is active.";
		
		FrameArt& eddieActiveHitstop = theArray[FT_EDDIE_ACTIVE_HITSTOP];
		eddieActiveHitstop = arrays[i][FT_ACTIVE_HITSTOP];
		eddieActiveHitstop.description = "Eddie's attack is active, but Eddie is in hitstop:"
			" during it, his attack can't hurt anybody and Eddie doesn't move.";
		
		FrameArt& eddieActiveNewHit = theArray[FT_EDDIE_ACTIVE_NEW_HIT];
		eddieActiveNewHit = arrays[i][FT_ACTIVE_NEW_HIT];
		eddieActiveNewHit.description = "Eddie's attack is active, and a new (potential) hit starts on this frame."
			" The black shadow on the left side of the frame denotes the start of a new (potential) hit."
			" Eddie may be capable of doing fewer hits than 1+the number of \"new hits\" displayed,"
			" and the first actual hit may occur on any active frame independent of \"new hit\" frames."
			" \"New hit\" frame merely means that the hit #2, #3 and so on can only happen after a \"new hit\" frame,"
			" and, between the first hit and the next \"new hit\", Eddie is inactive (even though he's displayed as active)."
			" When the second hit happens the situation resets and you need another \"new hit\" frame to do an actual hit and so on.";
		
		FrameArt& eddieRecovery = theArray[FT_EDDIE_RECOVERY];
		eddieRecovery = arrays[i][FT_RECOVERY];
		eddieRecovery.description = "Eddie's attack is in recovery.";
		
		FrameArt& hittableProjectileIdle = theArray[FT_IDLE_PROJECTILE_HITTABLE];
		hittableProjectileIdle = arrays[i][FT_EDDIE_IDLE];
		hittableProjectileIdle.description = "Projectile is not active and can be hit by attacks.";
		
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
	
	jamPantyPtr = (BYTE*)sigscanOffset(
		"GuiltyGearXrd.exe",
		"8d 4a ff 85 c0 79 04 33 c0 eb 06 3b c1 7c 02 8b c1",
		{ -10, 0 },
		nullptr, "JamPanty");
	
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
	if (!screenSizeKnown) return;
	dontUsePreBlockstunTime = settings.frameAdvantage_dontUsePreBlockstunTime;
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
			interjectIntoImGui();
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
		decrementFlagTimer(clearBurstGainMaxComboTimer[i], clearBurstGainMaxCombo[i]);
	}
	
	ImGui_ImplWin32_NewFrame();
	interjectIntoImGui();
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
	} else {
		resetFrameSelection();
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
	
	ImGui::Begin(searching ? "search_main" : "##ggxrd_hitbox_overlay_main_window", &visible, searching ? ImGuiWindowFlags_NoSavedSettings : 0);
	drawTextInWindowTitle(windowTitle.c_str());
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
			drawRightAlignedP1TitleWithCharIcon();
			ImGui::TableNextColumn();
			GGIcon scaledIcon = scaleGGIconToHeight(tipsIcon, 14.F);
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
					AddTooltip("When not in hitstun, not in OTG state and not waking up, stun decays each frame by 4.\n"
						"When in OTG state or waking up, stun decays each frame by 10.\n"
						"Stun does not decay when in hitstun.\n"
						"Stun decay happens even when in hitstop or superfreeze, and is unaffected by RC slowdown.\n"
						"When an unknown condition is met, and not in hitstun, OTG or wakeup, stun decreases by 8 each frame, instead of 4."
						" It was never observed.");
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
				player.printStartup(strbufs[i], sizeof strbuf, &nameParts[i]);
			}
			startupOrTotal(two, "Startup", &showStartupTooltip);
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
				player.printTotal(strbufs[i], sizeof strbuf, &nameParts[i]);
			}
			startupOrTotal(two, "Total", &showTotalTooltip);
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
				} else if (player.xStunDisplay == PlayerInfo::XSTUN_DISPLAY_REJECTION) {
					ptrNextSize = sprintf_s(ptrNext, ptrNextSizeCap, "%d/%d",
						player.rejection,
						player.rejectionMax);
				} else if (player.xStunDisplay == PlayerInfo::XSTUN_DISPLAY_REJECTION_WITH_SLOW) {
					ptrNextSize = sprintf_s(ptrNext, ptrNextSizeCap, "%d/%d",
						player.rejectionWithSlow,
						player.rejectionMaxWithSlow);
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
				if (player.displayTumble && (
						player.xStunDisplay == PlayerInfo::XSTUN_DISPLAY_HIT
						|| player.xStunDisplay == PlayerInfo::XSTUN_DISPLAY_HIT_WITH_SLOW
				)) {
					sprintf_s(strbuf, "%d/%d tumble", player.tumbleWithSlow, player.tumbleMaxWithSlow);
					printNoWordWrap
				}
				if (player.displayWallstick && (
						player.xStunDisplay == PlayerInfo::XSTUN_DISPLAY_HIT
						|| player.xStunDisplay == PlayerInfo::XSTUN_DISPLAY_HIT_WITH_SLOW
				)) {
					sprintf_s(strbuf, "%d/%d wallstick", player.wallstickWithSlow, player.wallstickMaxWithSlow);
					printNoWordWrap
				}
				if (player.displayKnockdown && (
						player.xStunDisplay == PlayerInfo::XSTUN_DISPLAY_HIT
						|| player.xStunDisplay == PlayerInfo::XSTUN_DISPLAY_HIT_WITH_SLOW
				)) {
					sprintf_s(strbuf, "%d/%d knockdown", player.knockdownWithSlow, player.knockdownMaxWithSlow);
					printNoWordWrap
				}
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
					if (printMoveField(player)) {
						printWithWordWrap
						if (settings.useSlangNames) {
							if (ImGui::BeginItemTooltip()) {
								ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
								printMoveFieldTooltip(player);
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
								"\n"
								"Notes:\n"
								"1) If one of the moves is a super or caused a superfreeze, it may be shown as one move, while in the Startup/Total field"
								" it is split into parts using + sign - this is one possible discrepancy.\n"
								"2) The move names might not match the names you may find when hovering your mouse over frames in the framebar to read their"
								" animation names, because the names here are only updated when a significant enough change in the animation happens.\n"
								"\n"
								"To hide this field you can use the \"dontShowMoveName\" setting."
								" Then it will only be shown in the tooltip of 'Startup' and 'Total' fields.");
						}
						AddTooltip(searchTooltip(moveTooltip.c_str(), nullptr));
					}
				}
			}
			if (settings.showDebugFields) {
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
	if (ImGui::Button(searchFieldTitle("Show Tension Values"))) {
		showTensionData = !showTensionData;
	}
	AddTooltip(searchTooltip("Displays tension gained from combo and factors that affect tension gain."));
	ImGui::SameLine();
	if (ImGui::Button(searchFieldTitle("Burst Gain"))) {
		showBurstGain = !showBurstGain;
	}
	AddTooltip(searchTooltip("Displays burst gained from combo or last hit."));
	
	if (ImGui::Button(searchFieldTitle("Combo Damage & Combo Stun (P1)"))) {
		showComboDamage[0] = !showComboDamage[0];
	}
	AddTooltip(searchTooltip("Displays combo damage and maximum total stun achieved during the last performed combo for P1.\n"
		"Also shows the total tension gained during last combo by you and the total burst gained during last combo by the opponent."));
	ImGui::SameLine();
	if (ImGui::Button(searchFieldTitle(".. (P2)"))) {
		showComboDamage[1] = !showComboDamage[1];
	}
	AddTooltip(searchTooltip("...for P2."));
	
	if (ImGui::Button(searchFieldTitle("Speed/Hitstun Proration/Pushback/Wakeup"))) {
		showSpeedsData = !showSpeedsData;
	}
	AddTooltip(searchTooltip("Display x,y, speed, pushback and protation of hitstun and pushback."));
	
	if (ImGui::Button(searchFieldTitle("Projectiles"))) {
		showProjectiles = !showProjectiles;
	}
	AddTooltip(searchTooltip("Display the list of current projectiles present on the current frame, for both players."));
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
		ImGui::PushID(searchFieldTitle("Damage/RISC/Stun Calculation"));
		ImGui::PushID(i);
		sprintf_s(strbuf, i == 0 ? "Damage/RISC/Stun Calculation (P1)" : "... (P2)");
		if (ImGui::Button(strbuf)) {
			showDamageCalculation[i] = !showDamageCalculation[i];
		}
		AddTooltip(searchTooltip("For the attacking player this shows damage, RISC and stun calculation from the last hit and current combo proration."));
		ImGui::PopID();
		ImGui::PopID();
		if (i == 0) ImGui::SameLine();
	}
	
	for (int i = 0; i < two; ++i) {
		int strbufLen = sprintf_s(strbuf, "Combo Recipe (P%d)", i + 1);
		if (ImGui::Button(searchFieldTitle(strbuf, strbuf + strbufLen))) {
			showComboRecipe[i] = !showComboRecipe[i];
		}
		AddTooltip(searchTooltip("Displays actions performed by this player as the attacker during the last combo."));
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
			int strbufLength = sprintf_s(strbuf, "The progress on your stun or stagger mash."
				" It might be too difficult to use this window in real-time, so please consider additionally using"
				" the Hitboxes - Freeze Game checkbox (Hotkey: %s) and the Next Frame button next to it (Hotkey: %s).",
				comborepr(settings.freezeGameToggle),
				comborepr(settings.allowNextFrameKeyCombo));
			ImGui::TextUnformatted(searchTooltip(strbuf, strbuf + strbufLength));
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}
		ImGui::PopID();
		ImGui::PopID();
		if (i == 0) ImGui::SameLine();
	}
	
	if (ImGui::Button(searchFieldTitle("Clear Input History"))) {
		game.clearInputHistory();
		endScene.clearInputHistory();
	}
	static std::string clearInputHistoryHelp;
	if (clearInputHistoryHelp.empty()) {
		clearInputHistoryHelp = settings.convertToUiDescription(
			"Clears input history. For example, in training mode, when input history display is enabled.\n"
			"You can use the \"clearInputHistory\" hotkey to toggle this setting.\n"
			"\n"
			"Alternatively, you can use the \"clearInputHistoryOnStageReset\" boolean setting to"
			" make the game clear input history when you reset positions in training mode or when"
			" round restarts in any game mode.\n"
			"Alternatively, you can use the \"clearInputHistoryOnStageResetInTrainingMode\" boolean setting to"
			" make the game clear input history when you reset positions in training mode only.");
	}
	AddTooltipWithHotkey(clearInputHistoryHelp.c_str(), clearInputHistoryHelp.c_str() + clearInputHistoryHelp.size(), settings.clearInputHistory);
	
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
		
		ImGui::PushStyleColor(ImGuiCol_Text, SLIGHTLY_GRAY);
		ImGui::PushTextWrapPos(0.F);
		ImGui::TextUnformatted("You can take screenshots with transparency as long as GIF Mode or Black Background"
			" is enabled, using the 'Take Screenshot' button in this section below.");
		ImGui::PopTextWrapPos();
		ImGui::PopStyleColor();
		
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
		
		bool dontHideOpponentsEffects = gifMode.dontHideOpponentsEffects;
		if (ImGui::Checkbox(searchFieldTitle("Don't Hide Opponent's Effects"), &dontHideOpponentsEffects)) {
			gifMode.dontHideOpponentsEffects = dontHideOpponentsEffects;
		}
		ImGui::SameLine();
		HelpMarker("If 'Hide Opponent' is used, don't hide their effects, which is everything except the player's character model.");
		
		bool dontHideOpponentsBoxes = gifMode.dontHideOpponentsBoxes;
		if (ImGui::Checkbox(searchFieldTitle("Don't Hide Opponent's Boxes"), &dontHideOpponentsBoxes)) {
			gifMode.dontHideOpponentsBoxes = dontHideOpponentsBoxes;
		}
		ImGui::SameLine();
		HelpMarker("Similar to 'Hide Opponent' in that makes the opponent player invulnerable to all attacks,"
			" except that this option does not hide them and allows them to land attacks of their own.");
		
		bool makeFullInvul = gifMode.makeOpponentFullInvul;
		if (ImGui::Checkbox(searchFieldTitle("Make Opponent Full Invul (Without Hiding)"), &makeFullInvul)) {
			gifMode.makeOpponentFullInvul = makeFullInvul;
		}
		ImGui::SameLine();
		HelpMarker("If 'Hide Opponent' is used, don't hide their hitboxes, pushboxes, hurtboxes, etc, on either the character model or the effects.");
		
		stateChanged = ImGui::Checkbox(searchFieldTitle("Hide Player"), &toggleHidePlayer) || stateChanged;
		ImGui::SameLine();
		static std::string hidePlayerHelp;
		if (hidePlayerHelp.empty()) {
			hidePlayerHelp = settings.convertToUiDescription(
				"Make the player invisible and invulnerable.\n"
				"You can use the \"toggleHidePlayer\" hotkey to toggle this setting.");
		}
		HelpMarkerWithHotkey(hidePlayerHelp, settings.toggleHidePlayer);
		
		bool dontHidePlayersEffects = gifMode.dontHidePlayersEffects;
		if (ImGui::Checkbox(searchFieldTitle("Don't Hide Player's Effects"), &dontHidePlayersEffects)) {
			gifMode.dontHidePlayersEffects = dontHidePlayersEffects;
		}
		ImGui::SameLine();
		HelpMarker("If 'Hide Player' is used, don't hide their effects, which is everything except the player's character model.");
		
		bool dontHidePlayersBoxes = gifMode.dontHidePlayersBoxes;
		if (ImGui::Checkbox(searchFieldTitle("Don't Hide Player's Boxes"), &dontHidePlayersBoxes)) {
			gifMode.dontHidePlayersBoxes = dontHidePlayersBoxes;
		}
		ImGui::SameLine();
		HelpMarker("If 'Hide Player' is used, don't hide their hitboxes, pushboxes, hurtboxes, etc, on either the character model or the effects.");
		
		makeFullInvul = gifMode.makePlayerFullInvul;
		if (ImGui::Checkbox(searchFieldTitle("Make Player Full Invul (Without Hiding)"), &makeFullInvul)) {
			gifMode.makePlayerFullInvul = makeFullInvul;
		}
		ImGui::SameLine();
		HelpMarker("Similar to 'Hide Player' in that makes your player invulnerable to all attacks,"
			" except that this option does not hide you and allows you to land attacks of your own.");
		
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
		
		ImGui::PushID(1);
		booleanSettingPreset(settings.ignoreScreenshotPathAndSaveToClipboard);
		ImGui::PopID();
		
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
		
		bool allowCreateParticles = gifMode.allowCreateParticles;
		if (ImGui::Checkbox(searchFieldTitle("Allow Creation Of Particles"), &allowCreateParticles)) {
			gifMode.allowCreateParticles = allowCreateParticles;
		}
		ImGui::SameLine();
		static std::string allowCreateParticlesHelp;
		if (allowCreateParticlesHelp.empty()) {
			allowCreateParticlesHelp = settings.convertToUiDescription(
				"When this option is enabled, particle effects such as superfreeze flash, can be created."
				" Turning this option on or off does not remove particles that have already been created,"
				" or make appear those particles which have already not been created.\n"
				"You can use the \"toggleAllowCreateParticles\" shortcut to toggle this option.");
		}
		HelpMarkerWithHotkey(allowCreateParticlesHelp, settings.toggleAllowCreateParticles);
		
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
			
			booleanSettingPreset(settings.ignoreScreenshotPathAndSaveToClipboard);
			
			booleanSettingPreset(settings.allowContinuousScreenshotting);
			
			booleanSettingPreset(settings.dontUseScreenshotTransparency);
			
			booleanSettingPreset(settings.useSimplePixelBlender);
			
			booleanSettingPreset(settings.usePixelShader);
			if (graphics.failedToCreatePixelShader) {
				if (!pixelShaderFailReasonObtained) {
					pixelShaderFailReasonObtained = true;
					pixelShaderFailReason = graphics.getFailedToCreatePixelShaderReason();
				}
				if (!pixelShaderFailReason.empty()) {
					ImGui::PushStyleColor(ImGuiCol_Text, SLIGHTLY_GRAY);
					ImGui::PushTextWrapPos(0.F);
					char initialText[] = "Pixel shader was disabled automatically: ";
					char finalText[] = " The setting will have no effect.";
					if ((sizeof initialText - 1) + pixelShaderFailReason.size() + (sizeof finalText - 1) + 1 > sizeof strbuf) {
						pixelShaderFailReason[sizeof strbuf - (sizeof initialText - 1) - (sizeof finalText - 1) - 1] = '\0';
					}
					sprintf_s(strbuf, "%s%s%s", initialText, pixelShaderFailReason.c_str(), finalText);
					ImGui::TextUnformatted(strbuf);
					ImGui::PopTextWrapPos();
					ImGui::PopStyleColor();
				}
			}
			
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
			
			booleanSettingPreset(settings.showOTGOnFramebar);
			
			booleanSettingPreset(settings.showSuperArmorOnFramebar);
			
			booleanSettingPreset(settings.showFirstFramesOnFramebar);
			
			booleanSettingPreset(settings.considerSimilarFrameTypesSameForFrameCounts);
			
			booleanSettingPreset(settings.considerSimilarIdleFramesSameForFrameCounts);
			
			booleanSettingPreset(settings.skipGrabsInFramebar);
			
			booleanSettingPreset(settings.showFramebarHatchedLineWhenSkippingGrab);
			
			booleanSettingPreset(settings.showFramebarHatchedLineWhenSkippingHitstop);
			
			booleanSettingPreset(settings.showFramebarHatchedLineWhenSkippingSuperfreeze);
			
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
			
			booleanSettingPreset(settings.showP1FramedataInFramebar);
			
			booleanSettingPreset(settings.showP2FramedataInFramebar);
			
			if (intSettingPreset(settings.framebarStoredFramesCount, 1, 1, 1, 80.F, _countof(Framebar::frames))) {
				if (settings.framebarDisplayedFramesCount.load() > settings.framebarStoredFramesCount.load()) {
					settings.framebarDisplayedFramesCount = settings.framebarStoredFramesCount.load();
				}
			}
			
			intSettingPreset(settings.framebarDisplayedFramesCount, 1, 1, 1, 80.F, settings.framebarStoredFramesCount);
			
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
			keyComboControl(settings.toggleAllowCreateParticles);
			keyComboControl(settings.clearInputHistory);
			keyComboControl(settings.disableModKeyCombo);
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
			
			booleanSettingPreset(settings.showDurationsInInputHistory);
			
			booleanSettingPreset(settings.usePositionResetMod);
			
			intSettingPreset(settings.positionResetDistBetweenPlayers, 0, 1000, 10000, 120.F);
			intSettingPreset(settings.positionResetDistFromCorner, 0, 1000, 10000, 120.F);
			
			booleanSettingPreset(settings.showDebugFields);
			
			booleanSettingPreset(settings.ignoreNumpadEnterKey);
			booleanSettingPreset(settings.ignoreRegularEnterKey);
			
			intSettingPreset(settings.startingTensionPulse, -25000, 100, 1000, 120.F, 25000);
			
			booleanSettingPreset(settings.clearInputHistoryOnStageReset);
			booleanSettingPreset(settings.clearInputHistoryOnStageResetInTrainingMode);
			
			booleanSettingPreset(settings.hideWins);
			booleanSettingPreset(settings.hideWinsDirectParticipantOnly);
			intSettingPreset(settings.hideWinsExceptOnWins, INT_MIN, 1, 5, 80.F);
			
			ImGui::PushStyleColor(ImGuiCol_Text, SLIGHTLY_GRAY);
			ImGui::PushTextWrapPos(0.F);
			ImGui::TextUnformatted(searchFieldTitle("Some character-specific settings are only found in \"Character Specific\" menus (see buttons above).\n"
				"Combo Recipe settings are only found in the cogwheel on the Combo Recipe panel."));
			ImGui::PopTextWrapPos();
			ImGui::PopStyleColor();
			
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
				"Distance-Based Modifier - depends on distance to the opponent. Can only be 60% (distance >= 1312500),"
				" 80% (distance >= 875000) or 100%.\n"
				"Negative Penalty Modifier - if a Negative Penalty is active, the modifier is 20%, otherwise it's 100%.\n"
				"Tension Pulse-Based Modifier - depends on Tension Pulse. The values are listed immediately below:\n"
				"Tension Pulse < -12500 ==> 25%\n"
				"Tension Pulse < -7500 ==> 50%\n"
				"Tension Pulse < -3750 ==> 75%\n"
				"Tension Pulse < -1250 ==> 90%\n"
				"Tension Pulse < 1250 ==> 100%\n"
				"Tension Pulse < 5000 ==> 125%\n"
				"Tension Pulse >= 5000 ==> 150%\n"
				"\n"
				"A fourth modifier may be displayed, which is an extra tension modifier. "
				"It may be present if you use Stylish mode or playing MOM mode. It will be highlighted in yellow.\n"
				"\n"
				"A fourth or fifth modifier may be displayed, which is a combo hit count-dependent modifier. "
				"It affects how fast you gain Tension from performing a combo.\n"
				"\n"
				"The exact amount of tension gained on attack depends on 'tensionGainOnConnect' for each move. The standard values are:\n"
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
					zerohspacing
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
					_zerohspacing
				}
			}
			
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(searchFieldTitle("Tension Gain On Defense"));
			AddTooltip(searchTooltip("Affects how fast you gain Tension when getting hit by attacks or combos.\n"
				"Tension Gain Modifier = Distance-Based Modifier * Negative Penalty Modifier * Tension Pulse-Based Modifier.\n"
				"Distance-Based Modifier - depends on distance to the opponent. (The value ranges are the same as for 'Tension Gain On Attack'.)\n"
				"Negative Penalty Modifier - if a Negative Penalty is active, the modifier is 20%, otherwise it's 100%.\n"
				"Tension Pulse-Based Modifier - depends on Tension Pulse. (The value ranges are the same as for 'Tension Gain On Attack'.)\n\n"
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
			
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(searchFieldTitle("Tension Gain Max Combo"));
			AddTooltip(searchTooltip("The maximum amount of Tension that was gained on an entire performed combo during this training session"
				" (either inflicting it or getting hit by it).\n"
				"You can clear this value by pressing a button below this table."));
			float offsets[2];
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
				AddTooltip(searchTooltip("Clear max combo's Tension gain."));
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
		ImGui::Begin(searching ? "search_burst" : "Burst Gain", &showBurstGain, searching ? ImGuiWindowFlags_NoSavedSettings : 0);
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
			float offsets[2];
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				offsets[i] = ImGui::GetCursorPosX();
				printDecimal(player.burstGainMaxCombo, 2, 0);
				ImGui::TextUnformatted(printdecimalbuf);
			}
			
			ImGui::EndTable();
			
			for (int i = 0; i < two; ++i) {
				ImGui::SetCursorPosX(offsets[i]);
				ImGui::PushID(i);
				if (ImGui::Button(searchFieldTitle("Clear Max Combo##Burst"))) {
					clearBurstGainMaxCombo[i] = true;
					clearBurstGainMaxComboTimer[i] = 10;
					stateChanged = true;
				}
				AddTooltip(searchTooltip("Clear max combo's Burst gain."));
				if (i == 0) ImGui::SameLine();
				ImGui::PopID();
			}
		}
		ImGui::End();
	}
	popSearchStack();
	searchCollapsibleSection("Combo Damage & Combo Stun");
	for (int i = 0; i < two; ++i) {
		if (showComboDamage[i] || searching) {
			ImGui::PushID(i);
			sprintf_s(strbuf, searching ? "search_combodmg%d" : "  Combo Damage & Combo Stun (P%d)", i + 1);
			if (searching) {
				ImGui::SetNextWindowPos({ 100000.F, 100000.F }, ImGuiCond_Always);
			}
			ImGui::Begin(strbuf, showComboDamage + i, searching ? ImGuiWindowFlags_NoSavedSettings : ImGuiWindowFlags_NoBackground);
			PlayerInfo& player = endScene.players[i];
			PlayerInfo& opponent = endScene.players[1 - i];
			
			drawPlayerIconInWindowTitle(i);
			
			if (!*aswEngine) {
				ImGui::TextUnformatted("Match isn't running.");
			} else
			if (ImGui::BeginTable("##ComboDmgStun",
						2,
						ImGuiTableFlags_Borders
						| ImGuiTableFlags_RowBg
						| ImGuiTableFlags_NoSavedSettings
						| ImGuiTableFlags_NoPadOuterX)
			) {
				ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0.5F);
				ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.5F);
				
				ImGui::TableNextColumn();
				outlinedText(ImGui::GetCursorPos(), searchFieldTitle("Combo Damage"), nullptr, nullptr, true);
				AddTooltip(searchFieldTitle("Total damage done by this player as the attacker during the last combo."));
				ImGui::TableNextColumn();
				if (opponent.pawn) {
					sprintf_s(strbuf, "%d", opponent.pawn.TrainingEtc_ComboDamage());
					outlinedText(ImGui::GetCursorPos(), strbuf, nullptr, nullptr, true);
				}
				
				ImGui::TableNextColumn();
				outlinedText(ImGui::GetCursorPos(), searchFieldTitle("Combo Stun"), nullptr, nullptr, true);
				AddTooltip(searchFieldTitle("Maximum total stun reached by the opponent during the last combo that was done by this player as the attacker."));
				ImGui::TableNextColumn();
				sprintf_s(strbuf, "%d", opponent.stunCombo);
				outlinedText(ImGui::GetCursorPos(), strbuf, nullptr, nullptr, true);
				
				ImGui::TableNextColumn();
				outlinedText(ImGui::GetCursorPos(), searchFieldTitle("Tension Gained Last Combo"), nullptr, nullptr, true);
				AddTooltip(searchFieldTitle("The total amount of tension gained during the last combo by this player as the attacker.\n"
					"This value is in units from 0.00 (no tension) to 100.00 (full tension)."));
				ImGui::TableNextColumn();
				printDecimal(player.tensionGainLastCombo, 2, 0);
				outlinedText(ImGui::GetCursorPos(), printdecimalbuf, nullptr, nullptr, true);
				
				ImGui::TableNextColumn();
				outlinedText(ImGui::GetCursorPos(), searchFieldTitle("Burst Gained Last Combo"), nullptr, nullptr, true);
				AddTooltip(searchFieldTitle("The total amount of burst gained during the last combo by the opponent as the defender.\n"
					"This value is in units from 0.00 (no burst) to 150.00 (full burst)."));
				ImGui::TableNextColumn();
				printDecimal(opponent.burstGainLastCombo, 2, 0);
				outlinedText(ImGui::GetCursorPos(), printdecimalbuf, nullptr, nullptr, true);
				
				ImGui::EndTable();
			}
			ImGui::End();
			ImGui::PopID();
		}
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
			AddTooltip(searchTooltip("Speed X; Y. Divided by 100 for viewability."));
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
			ImGui::TextUnformatted(searchFieldTitle("Dash Speed"));
			AddTooltip(searchTooltip("Dash Speed X. Divided by 100 for viewability."));
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				printDecimal(player.pawn ? player.pawn.dashSpeed() : 0, 2, 0);
				sprintf_s(strbuf, "%s", printdecimalbuf);
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
			ImGui::TextUnformatted(searchFieldTitle("Weight"));
			AddTooltip(searchTooltip("Weight is the percentage multiplier applied to received speed Y."));
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				sprintf_s(strbuf, "%d", player.weight);
				ImGui::TextUnformatted(strbuf);
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
				"'Attack pushback modifier' depends on the performed move."
				" 'Attack pushback modifier on hit' depends on the performed move and should only be non-100 when the opponent is in hitstun."
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
				"5 hits and below -> no proration,\n"
				"6 hits so far -> 59/60 * 100% proration,\n"
				"7 hits -> 58 / 60 * 100% and so on, up to 30 / 60 * 100%. The rounding of the final speed Y is up.\n"
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
			ImGui::TextUnformatted(searchFieldTitle("Last Hit Combo Timer"));
			AddTooltip(searchTooltip("This is updated only when a hit happens."
				"Combo timer at the time of the hit affects hitstun proration (see Hitstun Proration's tooltip for info)."));
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				printDecimal(player.lastHitComboTimer * 100 / 60, 2, 0);
				sprintf_s(strbuf, "%s sec (%df)", printdecimalbuf, player.lastHitComboTimer);
				ImGui::TextUnformatted(strbuf);
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
			ImGui::TextUnformatted(searchFieldTitle("Double Jumps"));
			AddTooltip(searchTooltip("Available double jumps."));
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				if (*aswEngine && player.pawn) {
					sprintf_s(strbuf, "%d/%d", player.pawn.remainingDoubleJumps(), player.pawn.maxDoubleJumps());
					ImGui::TextUnformatted(strbuf);
				}
			}
			
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(searchFieldTitle("Airdashes"));
			AddTooltip(searchTooltip("Available airdashes."));
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				if (*aswEngine && player.pawn) {
					sprintf_s(strbuf, "%d/%d", player.pawn.remainingAirDashes(), player.pawn.maxAirdashes());
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
	searchCollapsibleSection("Projectiles");
	if (showProjectiles || searching) {
		if (searching) {
			ImGui::SetNextWindowPos({ 100000.F, 100000.F }, ImGuiCond_Always);
		}
		ImGui::Begin(searching ? "search_projectiles" : "Projectiles", &showProjectiles, searching ? ImGuiWindowFlags_NoSavedSettings : 0);
		
		if (ImGui::BeginTable("##Projectiles", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_NoPadOuterX)) {
			ImGui::TableSetupColumn("P1", ImGuiTableColumnFlags_WidthStretch, 0.4f);
			ImGui::TableSetupColumn("##FieldTitle", ImGuiTableColumnFlags_WidthStretch, 0.2f);
			ImGui::TableSetupColumn("P2", ImGuiTableColumnFlags_WidthStretch, 0.4f);
			
			ImGui::TableNextColumn();
			drawRightAlignedP1TitleWithCharIcon();
			ImGui::SameLine();
			RightAlignedText("P1");
			ImGui::TableNextColumn();
			ImGui::TableNextColumn();
			drawPlayerIconWithTooltip(1);
			ImGui::SameLine();
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
				if (settings.showDebugFields) {
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
						CenterAlignedText("Lifetime Counter");
						AddTooltip("Time, in frames, since creation. Does not reset when switching states.");
					}
				}
				for (int i = 0; i < two; ++i) {
					ImGui::TableNextColumn();
					if (row.side[i]) {
						ProjectileInfo& projectile = *row.side[i];
						char* buf = strbuf;
						size_t bufSize = sizeof strbuf;
						printDecimal(projectile.x, 2, 0, false);
						int result = sprintf_s(strbuf, "%s; ", printdecimalbuf);
						advanceBuf
						printDecimal(projectile.y, 2, 0, false);
						sprintf_s(buf, bufSize, "%s", printdecimalbuf);
						printNoWordWrap
					}
					
					if (i == 0) {
						ImGui::TableNextColumn();
						CenterAlignedText("Pos");
						AddTooltip("Position X; Y");
					}
				}
				if (settings.showDebugFields) {
					for (int i = 0; i < two; ++i) {
						ImGui::TableNextColumn();
						if (row.side[i]) {
							ProjectileInfo& projectile = *row.side[i];
							printNoWordWrapArg(projectile.creatorName)
						}
						
						if (i == 0) {
							ImGui::TableNextColumn();
							CenterAlignedText("Creator");
							AddTooltip("The name of state that created this projectile.");
						}
					}
				}
				for (int i = 0; i < two; ++i) {
					ImGui::TableNextColumn();
					if (row.side[i]) {
						ProjectileInfo& projectile = *row.side[i];
						if (projectile.trialName[0] == '\0') {
							printNoWordWrapArg(projectile.animName)
						} else {
							sprintf_s(strbuf, "%s (%s)", projectile.animName, projectile.trialName);
							printNoWordWrap
						}
					}
					
					if (i == 0) {
						ImGui::TableNextColumn();
						CenterAlignedText("Anim");
						AddTooltip("The name of the current state of this projectile. If an alternative name is given"
							" in parentheses, that is a 'trial' name.");
					}
				}
				if (settings.showDebugFields) {
					for (int i = 0; i < two; ++i) {
						ImGui::TableNextColumn();
						if (row.side[i]) {
							ProjectileInfo& projectile = *row.side[i];
							printNoWordWrapArg(projectile.ptr ? projectile.ptr.gotoLabelRequest() : "")
						}
						
						if (i == 0) {
							ImGui::TableNextColumn();
							CenterAlignedText("gotoLabelRequest");
							AddTooltip("On the next frame, this projectile will change to a sub-state of the given name.");
						}
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
						CenterAlignedText("Anim Frame");
						AddTooltip("Current time spent in the current state, in frames, not including hitstop and superfreeze. Starts from 1 when entering a new state.");
					}
				}
				if (settings.showDebugFields) {
					for (int i = 0; i < two; ++i) {
						ImGui::TableNextColumn();
						if (row.side[i]) {
							ProjectileInfo& projectile = *row.side[i];
							projectile.sprite.print(strbuf, sizeof strbuf);
							printNoWordWrap
						}
						
						if (i == 0) {
							ImGui::TableNextColumn();
							CenterAlignedText("Sprite");
						}
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
						CenterAlignedText("Hitstop");
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
						CenterAlignedText("Active");
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
						CenterAlignedText("Startup");
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
						CenterAlignedText("Total");
					}
				}
				if (settings.showDebugFields) {
					for (int i = 0; i < two; ++i) {
						ImGui::TableNextColumn();
						if (row.side[i]) {
							ProjectileInfo& projectile = *row.side[i];
							sprintf_s(strbuf, "%s", formatBoolean(projectile.disabled));
							printNoWordWrap
						}
						
						if (i == 0) {
							ImGui::TableNextColumn();
							CenterAlignedText("Ignored");
							AddTooltip("This means that the projectile does not count towards the display of 'Active' frames in the mod's main UI window for the player.");
						}
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
		ImGui::End();
	}
	popSearchStack();
	searchCollapsibleSection("Character Specific");
	for (int i = 0; i < 2; ++i) {
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
					
					ImGui::PushStyleColor(ImGuiCol_Text, SLIGHTLY_GRAY);
					ImGui::PushTextWrapPos(0.F);
					ImGui::TextUnformatted("This value doesn't decrease during hitstop and superfreeze and decreases"
						" at half the speed when slowed down by opponent's RC.");
					ImGui::PopTextWrapPos();
					ImGui::PopStyleColor();
					
				} else {
					GGIcon scaledIcon = scaleGGIconToHeight(getCharIcon(CHARACTER_TYPE_SOL), 14.F);
					drawGGIcon(scaledIcon);
					ImGui::SameLine();
					ImGui::TextUnformatted(searchFieldTitle("Not in Dragon Install"));
				}
				bool hasForceDisableFlag1 = (player.wasForceDisableFlags & 0x1) != 0;
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
									player.gunflameParams.totalSpriteLength += lastSpriteLength;
								} else if (lastType == Moves::instr_createObjectWithArg) {
									if (!created) {
										player.gunflameParams.totalSpriteLengthUntilCreation -= lastSpriteLength;
										created = true;
									}
								} else if (lastType == Moves::instr_deleteMoveForceDisableFlag) {
									player.gunflameParams.totalSpriteLength -= lastSpriteLength;
									break;
								}
								instr = moves.skipInstruction(instr);
								lastType = moves.instructionType(instr);
							}
						}
						if (player.gunflameParams.totalSpriteLengthUntilCreation == 0) break;
						
						bool canCreateNewOne = ent.createArgHikitsukiVal1() <= 3
							&& (int)ent.currentAnimDuration() <= player.gunflameParams.totalSpriteLengthUntilCreation
							&& !ent.mem46();
						if (canCreateNewOne) {
							timeRemaining = player.gunflameParams.totalSpriteLengthUntilCreation - ent.currentAnimDuration() + 1
								+ (3 - ent.createArgHikitsukiVal1()) * player.gunflameParams.totalSpriteLengthUntilCreation
								+ player.gunflameParams.totalSpriteLength;
						} else {
							timeRemaining = player.gunflameParams.totalSpriteLength - ent.currentAnimDuration() + 1;
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
				yellowText(searchFieldTitle("Time until can do another gunflame:"));
				if (!hasForceDisableFlag1) {
					ImGui::TextUnformatted("0f");
				} else {
					int unused;
					PlayerInfo::calculateSlow(0, timeRemainingMax, slowMax, &timeRemainingMax, &unused, &unused);
					++timeRemainingMax;
					sprintf_s(strbuf, "%df or until hits/gets blocked/gets erased", timeRemainingMax);
					ImGui::TextUnformatted(strbuf);
				}
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
				yellowText(searchFieldTitle("Time until can do another stun edge: "));
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
									BYTE* funcStart = p.findStateStart("StunEdgeDelete");
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
				
				yellowText(searchFieldTitle("Has used j.D:"));
				const char* tooltip = searchTooltip("You can only use j.D once in the air. j.D gets reenabled when you land.");
				AddTooltip(tooltip);
				ImGui::SameLine();
				bool hasForceDisableFlag2 = (player.wasForceDisableFlags & 0x2) != 0;
				ImGui::TextUnformatted(hasForceDisableFlag2 ? "Yes" : "No");
				AddTooltip(tooltip);
				
				StringWithLength mahojinNames[2] { "Ground Ciel Timer:", "Air Ciel Timer:" };
				for (int j = 1; j >= 0; --j) {
					yellowText(searchFieldTitle(mahojinNames[j]));
					ImGui::SameLine();
					Entity p = player.pawn.stackEntity(1 + j);
					if (p && p.isActive() && strcmp(p.animationName(), "Mahojin") == 0) {
						BYTE* func = p.bbscrCurrentFunc();
						moves.fillInKyMahojin(func);
						int timer = moves.kyMahojin.remainingTime(p.bbscrCurrentInstr() - func, p.spriteFrameCounter());
						int slowdown = 0;
						int elapsed = 0;
						ProjectileInfo& projectile = endScene.findProjectile(p);
						if (projectile.ptr) {
							slowdown = projectile.rcSlowedDownCounter;
							elapsed = projectile.elapsedTime;
						}
						int result;
						int resultMax;
						int unused;
						PlayerInfo::calculateSlow(
							elapsed + 1,
							timer,
							slowdown,
							&result,
							&resultMax,
							&unused);
						sprintf_s(strbuf, "%d/%d", result, resultMax);
						ImGui::TextUnformatted(strbuf);
					} else {
						ImGui::TextUnformatted("Not set");
					}
				}
				
			} else if (player.charType == CHARACTER_TYPE_MAY) {
				bool hasForceDisableFlag2 = (player.wasForceDisableFlags & 0x2) != 0;
				if (!hasForceDisableFlag2) {
					yellowText(searchFieldTitle("Time until can do Beach Ball:"));
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
							yellowText(searchFieldTitle("Beach Ball bounces left:"));
							ImGui::SameLine();
							sprintf_s(strbuf, "%d/3", 3 - p.mem45());
							ImGui::TextUnformatted(strbuf);
							break;
						}
					}
					if (!foundBeachBall) {
						// Nothing found, but still can't do a Beach Ball on this frame?
						// Probably, a Ball has just been deleted, and the lack of its forceDisableFlags will only take effect on the next frame...
						yellowText(searchFieldTitle("Time until can do Beach Ball:"));
						ImGui::SameLine();
						ImGui::TextUnformatted("1f");
					}
				}
				
				booleanSettingPreset(settings.dontShowMayInteractionChecks);
				
				searchFieldTitle("Jump on Beach Ball range explanation");
				ImGui::PushTextWrapPos(0.F);
				ImGui::TextUnformatted(searchTooltip("When entering the valid Beach Ball jump range, on the next frame you can't do the Ball Jump,"
					" and on the next frame you become able to do it."));
				ImGui::TextUnformatted(searchTooltip("Enter Range -> Can't Balljump Yet -> Can Balljump"));
				ImGui::PopTextWrapPos();
				
				bool hasForceDisableFlag1 = (player.wasForceDisableFlags & 0x1) != 0;
				if (!hasForceDisableFlag1) {
					yellowText(searchFieldTitle("Time until can do Hoop:"));
					ImGui::SameLine();
					ImGui::TextUnformatted("0f");
				} else {
					Entity dolphin = player.pawn.stackEntity(0);
					if (dolphin && dolphin.isActive()) {
						ProjectileInfo& projectile = endScene.findProjectile(dolphin);
						BYTE* currentInstr = dolphin.bbscrCurrentInstr();
						BYTE* func = dolphin.bbscrCurrentFunc();
						int currentOffset = currentInstr - func;
						Moves::MayIrukasanRidingObjectInfo* ar[4] {
							&moves.mayIrukasanRidingObjectYokoA,
							&moves.mayIrukasanRidingObjectYokoB,
							&moves.mayIrukasanRidingObjectTateA,
							&moves.mayIrukasanRidingObjectTateB
						};
						int arInd = 0;
						if (moves.mayIrukasanRidingObjectYokoA.offset == 0) {
							BYTE* pos = moves.findSetMarker(func, "moveYokoA");
							if (pos) {
								BYTE* instr = moves.skipInstruction(pos);
								moves.mayIrukasanRidingObjectYokoA.offset = pos - func;
								int totalSoFar = 0;
								bool lastSpriteWasNull = false;
								while (moves.instructionType(instr) != Moves::instr_endState) {
									Moves::InstructionType type = moves.instructionType(instr);
									if (type == Moves::instr_sprite) {
										if (!ar[arInd]->frames.empty()) {
											ar[arInd]->frames.back().offset = instr - func;
										}
										if (strcmp((const char*)(instr + 4), "null") == 0) {
											lastSpriteWasNull = true;
										} else {
											int spriteLength = *(int*)(instr + 4 + 32);
											ar[arInd]->frames.emplace_back();
											Moves::MayIrukasanRidingObjectFrames& newObj = ar[arInd]->frames.back();
											newObj.offset = instr - func;
											newObj.frames = totalSoFar;
											totalSoFar += spriteLength;
										}
									} else if (type == Moves::instr_exitState && !lastSpriteWasNull && !ar[arInd]->frames.empty()) {
										ar[arInd]->frames.back().offset = instr - func;
									} else if (type == Moves::instr_setMarker) {
										ar[arInd]->totalFrames = totalSoFar;
										if (arInd == 3) break;
										++arInd;
										totalSoFar = 0;
										lastSpriteWasNull = false;
										ar[arInd]->offset = instr - func;
									}
									instr = moves.skipInstruction(instr);
								}
								arInd = 0;
							}
						}
						if (moves.mayIrukasanRidingObjectYokoA.offset != 0) {
							for ( ; arInd < 4; ++arInd) {
								if (currentOffset > ar[3 - arInd]->offset) {
									arInd = 3 - arInd;
									break;
								}
							}
						} else {
							arInd = 4;
						}
						int frames = 0;
						int totalFrames = 0;
						if (arInd != 4) {
							const Moves::MayIrukasanRidingObjectInfo& info = *ar[arInd];
							totalFrames = info.totalFrames;
							for (const Moves::MayIrukasanRidingObjectFrames& framesElem : info.frames) {
								if (framesElem.offset == currentOffset) {
									frames = framesElem.frames;
									break;
								}
							}
						}
						yellowText(searchFieldTitle("Time until can do Hoop:"));
						ImGui::SameLine();
						if (frames) {
							int timeRemaining = totalFrames - frames - dolphin.spriteFrameCounter();
							if (projectile.ptr) {
								int unused;
								PlayerInfo::calculateSlow(
									0,
									timeRemaining,
									projectile.rcSlowedDownCounter,
									&timeRemaining,
									&unused,
									&unused);
							}
							if (timeRemaining || hasForceDisableFlag1) ++timeRemaining;
							sprintf_s(strbuf, "%df", timeRemaining);
							ImGui::TextUnformatted(strbuf);
						} else {
							ImGui::TextUnformatted("???");
						}
					} else {
						yellowText(searchFieldTitle("Time until can do Hoop:"));
						ImGui::SameLine();
						
						bool foundIrukasanTsubureru_tama = false;
						for (int j = 0; j < entityList.count; ++j) {
							Entity p = entityList.list[j];
							if (p.isActive() && p.team() == i && !p.isPawn()
									&& strcmp(p.animationName(), "IrukasanTsubureru_tama") == 0) {
								foundIrukasanTsubureru_tama = true;
								if (p.y() >= 0 || p.speedY() >= 0) {
									ImGui::TextUnformatted("Until Dolphin lands+2f");
								} else {
									ImGui::TextUnformatted("2-1f");
								}
								break;
							}
						}
						if (!foundIrukasanTsubureru_tama) {
							ImGui::TextUnformatted("1f");
						}
					}
				}
				
				printChargeInCharSpecific(i, true, true, 30);
			
			} else if (player.charType == CHARACTER_TYPE_MILLIA) {
				searchFieldTitle("Pin pickup range explanation");
				ImGui::PushTextWrapPos(0.F);
				ImGui::TextUnformatted(searchTooltip("To pick up the knife you must be in either of the animations:"));
				ImGui::PopTextWrapPos();
				ImGui::TextUnformatted("*) Stand transitioning to Crouch;");
				ImGui::TextUnformatted("*) Crouch;");
				bool isRev2 = findMoveByName((void*)player.pawn.ent, "SilentForce2", 0) != nullptr;
				if (isRev2) {
					ImGui::TextUnformatted("*) Crouch Turn;");
					ImGui::TextUnformatted("*) Roll;");
					ImGui::TextUnformatted("*) Doubleroll.");
				}
				ImGui::PushTextWrapPos(0.F);
				ImGui::TextUnformatted(searchTooltip("Displayed Pin range checks for your origin point, not the pushbox."));
				ImGui::PopTextWrapPos();
				
				yellowText(searchFieldTitle("Chroming Rose:"));
				ImGui::SameLine();
				sprintf_s(strbuf, "%d/%df", player.milliaChromingRoseTimeLeft, player.maxDI);
				ImGui::TextUnformatted(strbuf);
				
				ImGui::PushStyleColor(ImGuiCol_Text, SLIGHTLY_GRAY);
				ImGui::PushTextWrapPos(0.F);
				ImGui::TextUnformatted(searchTooltip("This value doesn't decrease in hitstop and superfreeze and decreases"
					" at half the speed when slowed down by opponent's RC."));
				ImGui::PopTextWrapPos();
				ImGui::PopStyleColor();
				
				if (isRev2) {
					booleanSettingPreset(settings.showMilliaBadMoonBuffHeight);
				}
				
				yellowText(searchFieldTitle("Can do S/H Disc or Garden:"));
				ImGui::SameLine();
				bool hasForceDisableFlag = (player.wasForceDisableFlags & 0x1) != 0;
				ImGui::TextUnformatted(hasForceDisableFlag ? "No" : "Yes");
			
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
					ImGui::TableNextColumn();
					ImGui::Text("%-4d/%d", player.pawn.exGaugeValue(0), player.pawn.exGaugeMaxValue(0));
					
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
					
					ImGui::TableNextColumn();
					ImGui::TextUnformatted(searchFieldTitle("Shadow Puddle Timer"));
					ImGui::TableNextColumn();
					bool foundShadowPuddle = false;
					for (int j = 2; j < entityList.count; ++j) {
						Entity p = entityList.list[j];
						if (p.isActive() && p.team() == i && !p.isPawn() && strcmp(p.animationName(), "KageDamari") == 0) {
							int timeRemaining = 301 - (p.currentAnimDuration() - 1);
							int elapsed = 0;
							int slowdown = 0;
							ProjectileInfo& projectile = endScene.findProjectile(p);
							if (projectile.ptr) {
								elapsed = projectile.elapsedTime;
								slowdown = projectile.rcSlowedDownCounter;
							}
							int result;
							int resultMax;
							int unused;
							PlayerInfo::calculateSlow(
								elapsed + 1,
								timeRemaining,
								slowdown,
								&result,
								&resultMax,
								&unused);
							sprintf_s(strbuf, "%d/%d", result, resultMax);
							ImGui::TextUnformatted(strbuf);
							foundShadowPuddle = true;
							break;
						}
					}
					if (!foundShadowPuddle) {
						ImGui::TextUnformatted("Not set");
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
					AddTooltip(searchTooltip("The amount of consumed Eddie Gauge by the last attack."));
					ImGui::TableNextColumn();
					ImGui::Text("%d", player.eddie.consumedResource);
					
					ImGui::TableNextColumn();
					ImGui::TextUnformatted("X");
					ImGui::TableNextColumn();
					if (eddie) {
						printDecimal(eddie.x(), 2, 0);
						ImGui::TextUnformatted(printdecimalbuf);
					}
					
					ImGui::TableNextColumn();
					ImGui::TextUnformatted(searchFieldTitle("Anim"));
					AddTooltip(searchTooltip("This is the current animation, and might not be the same animation that"
						" is shown in Startup/Active/Recovery/Total fields."));
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
				
				yellowText(searchFieldTitle("Can perform S Drill:"));
				ImGui::SameLine();
				ImGui::TextUnformatted((player.wasForceDisableFlags & 0x1) == 0 ? "Yes" : "No");
				
				yellowText(searchFieldTitle("Can perform H Drill:"));
				ImGui::SameLine();
				ImGui::TextUnformatted((player.wasForceDisableFlags & 0x2) == 0 ? "Yes" : "No");
				
			} else if (player.charType == CHARACTER_TYPE_CHIPP) {
				if (player.playerval0) {
					printChippInvisibility(player.playerval0, player.maxDI);
				} else {
					ImGui::TextUnformatted(searchFieldTitle("Not invisible"));
				}
				if (player.move.caresAboutWall) {
					searchFieldTitle("Wall time:");
					ImGui::Text("Wall time: %d/120", player.pawn.mem54());
					ImGui::PushStyleColor(ImGuiCol_Text, SLIGHTLY_GRAY);
					ImGui::PushTextWrapPos(0.F);
					ImGui::TextUnformatted("This value increases slower when opponent slows you down with RC.");
					ImGui::PopTextWrapPos();
					ImGui::PopStyleColor();
				} else {
					ImGui::TextUnformatted(searchFieldTitle("Not on a wall."));
				}
				
				yellowText(searchFieldTitle("Can perform Gamma Blade:"));
				ImGui::SameLine();
				ImGui::TextUnformatted((player.wasForceDisableFlags & 0x1) == 0 ? "Yes" : "No");
				
			} else if (player.charType == CHARACTER_TYPE_POTEMKIN) {
				printChargeInCharSpecific(i, true, false, 30);
			} else if (player.charType == CHARACTER_TYPE_FAUST) {
				const PlayerInfo& otherPlayer = endScene.players[1 - player.index];
				if (!otherPlayer.poisonDuration) {
					ImGui::TextUnformatted(searchFieldTitle("Opponent not poisoned."));
				} else {
					yellowText(searchFieldTitle("Poison Duration On Opponent:"));
					ImGui::SameLine();
					sprintf_s(strbuf, "%d/%d", otherPlayer.poisonDuration, otherPlayer.poisonDurationMax);
					ImGui::TextUnformatted(strbuf);
				}
				
				yellowText(searchFieldTitle("Can throw item:"));
				ImGui::SameLine();
				ImGui::TextUnformatted((player.wasForceDisableFlags & 0x1) == 0 ? "Yes" : "No");
				
				yellowText(searchFieldTitle("Can throw Love:"));
				ImGui::SameLine();
				ImGui::TextUnformatted((player.wasForceDisableFlags & 0x2) == 0 ? "Yes" : "No");
				
				if (ImGui::Button(searchFieldTitle("How Flicking Works"))) {
					showHowFlickingWorks[i] = !showHowFlickingWorks[i];
				}
				if (showHowFlickingWorks[i] || searching) {
					ImGui::PushTextWrapPos(0.F);
					ImGui::TextUnformatted(searchTooltip("The mechanics are different when flicking your own thrown item and when flicking"
						" an enemy's projectile."));
					ImGui::TextUnformatted(searchTooltip("To flick an enemy projectile, it must come in contact with your hurtbox while"
						" you have super armor. If it hits you on the first two frames of super armor, colored red in the framebar,"
						" the reflect will be a homerun. Otherwise it will be a non-homerun reflect.\n"
						"Flicking enemy projectiles was added in Rev2."));
					ImGui::TextUnformatted(searchTooltip("When reflecting your own throw item, there's a point somewhere in front of you,"
						" and when a particular frame of animation is played, a signal is sent to the thrown item to check"
						" how far it is from that point. If it is < 100000 range, the hit is a homerun."
						" If the range is 300000, the hit is not a homerun."));
					ImGui::PopTextWrapPos();
				}
				
				booleanSettingPreset(settings.showFaustOwnFlickRanges);
				
				if (settings.showFaustOwnFlickRanges) {
					ImGui::PushTextWrapPos(0.F);
					yellowText(searchFieldTitle("Faust 5D:"));
					if (faust5D.empty()) {
						faust5D = settings.convertToUiDescription(faust5DHelp);
					}
					ImGui::TextUnformatted(searchTooltip(faust5D.c_str(), faust5D.c_str() + faust5D.size()));
					ImGui::PopTextWrapPos();
				}	
				
			} else if (player.charType == CHARACTER_TYPE_AXL) {
				printChargeInCharSpecific(i, true, false, 30);
			} else if (player.charType == CHARACTER_TYPE_VENOM) {
				printChargeInCharSpecific(i, true, true, 40);
				
				bool hasForceDisableFlag = (player.wasForceDisableFlags & 0x2) != 0;
				yellowText(searchFieldTitle("Can do Bishop Runout:"));
				ImGui::SameLine();
				ImGui::TextUnformatted(hasForceDisableFlag ? "No" : "Yes");
				
				struct VenomBallInfo {
					StringWithLength title;
					int stackIndex;
					bool isBishop;
				};
				VenomBallInfo venomBalls[] {
					{ "P Ball:", 0, false },
					{ "K Ball:", 1, false },
					{ "S Ball:", 2, false },
					{ "H Ball:", 3, false },
					{ "Last Stinger Ball:", 4, false },
					{ "Bishop:", 7, true }
				};
				for (int i = 0; i < _countof(venomBalls); ++i) {
					VenomBallInfo& info = venomBalls[i];
					Entity p = player.pawn.stackEntity(info.stackIndex);
					if (p && p.isActive()) {
						yellowText(searchFieldTitle(info.title));
						ImGui::Indent();
						if (!info.isBishop) {
							textUnformattedColored(LIGHT_BLUE_COLOR, searchFieldTitle("Level:"));
							ImGui::SameLine();
							sprintf_s(strbuf, "%d/4", p.storage(0));
							ImGui::TextUnformatted(strbuf);
						}
						if (info.isBishop) {
							textUnformattedColored(LIGHT_BLUE_COLOR, searchFieldTitle("Bishop Level:"));
							ImGui::SameLine();
							sprintf_s(strbuf, "%d/5", p.storage(1));
							ImGui::TextUnformatted(strbuf);
						}
						
						int elapsedTime = 0;
						int slowdown = 0;
						ProjectileInfo& projectile = endScene.findProjectile(p);
						if (projectile.ptr) {
							slowdown = projectile.rcSlowedDownCounter;
							elapsedTime = projectile.elapsedTime;
						}
						
						int timerOrig = p.mem47();
						int timer = timerOrig;
						int unused;
						int result;
						int resultMax;
						if (!info.isBishop) {
							PlayerInfo::calculateSlow(
								elapsedTime + 1,
								timer,
								slowdown,
								&result,
								&resultMax,
								&unused);
							textUnformattedColored(LIGHT_BLUE_COLOR, searchFieldTitle("Timer:"));
							ImGui::SameLine();
							if (timerOrig) {
								sprintf_s(strbuf, "%d/%d", result, resultMax);
								ImGui::TextUnformatted(strbuf);
							} else {
								ImGui::TextUnformatted("Active");
							}
						}
						
						if (info.isBishop) {
							timer = p.mem53();
							PlayerInfo::calculateSlow(
								elapsedTime + 1,
								timer,
								slowdown,
								&result,
								&resultMax,
								&unused);
							textUnformattedColored(LIGHT_BLUE_COLOR, searchFieldTitle("Bishop Timer:"));
							const char* tooltip = searchTooltip("While the text is grayed out, Bishop cannot be destroyed even if Bishop Timer ran out."
								" After Bishop stops being active, though, it can still be destroyed.");
							AddTooltip(tooltip);
							ImGui::SameLine();
							bool isGrayedOut = p.mem54() == 0;
							if (isGrayedOut) {
								ImGui::PushStyleColor(ImGuiCol_Text, SLIGHTLY_GRAY);
							}
							sprintf_s(strbuf, "%d/%d", result, resultMax);
							ImGui::TextUnformatted(strbuf);
							AddTooltip(tooltip);
							if (isGrayedOut) {
								ImGui::PopStyleColor();
							}
						}
						ImGui::Unindent();
					}
				}
				
			} else if (player.charType == CHARACTER_TYPE_SLAYER) {
				
				yellowText(searchFieldTitle("Bloodsucking Universe Buff (Rev2 only):"));
				const char* tooltip = searchTooltip("Bloodsucking Universe makes the next special or super guaranteed to do a counterhit.");
				AddTooltip(tooltip);
				sprintf_s(strbuf, "%d/%df", player.wasPlayerval1Idling, player.maxDI);
				ImGui::TextUnformatted(strbuf);
				
				if (ImGui::Button(searchFieldTitle("Show Buffed Moves"))) {
					printSlayerBuffedMoves[i] = !printSlayerBuffedMoves[i];
				}
				AddTooltip(tooltip);
				
				if (printSlayerBuffedMoves[i] || searching) {
					static StringWithLength buffedMoveNames[] {
						"*) P Mappa;",
						"*) K Mappa;",
						"*) Pilebunker;",
						"*) Crosswise Heel;",
						"*) Under Pressure;",
						"*) It's Late;",
						"*) Helter Skelter;",
						"*) Footloose Journey;",
						"*) Undertow;",
						"*) Dead on Time;",
						"*) Eternal Wings;",
						"*) Straight-Down Dandy."
					};
					for (int j = 0; j < _countof(buffedMoveNames); ++j) {
						ImGui::TextUnformatted(searchTooltip(buffedMoveNames[j]));
					}
				}
			} else if (player.charType == CHARACTER_TYPE_INO) {
				
				ImGui::PushStyleColor(ImGuiCol_Text, SLIGHTLY_GRAY);
				ImGui::PushTextWrapPos(0.F);
				ImGui::TextUnformatted("Hover-3 and S-CL having lower speed Y only applies to Rev2.");
				ImGui::PopTextWrapPos();
				ImGui::PopStyleColor();
				
				yellowText(searchFieldTitle("Hoverdashed down:"));
				const char* tooltip = searchTooltip("Pressed 3 during hoverdash."
					" Doing so replaces your airdash (66) with a downward hover. However, it's possible"
					" under right circumstances to still get an airdash, and not hoverdown. This happens because the"
					" height and speed requirements for an airdash are such:\n"
					"If your speed Y is directed upwards, your Y must be > airdash minimum height, which is 105000 for I-No.\n"
					"If your speed Y is <= 0, your Y must be > 70000 and > -speedY. Also note that movement occurs after"
					" deciding what move should be performed, so on the screen you always see positions that are not what"
					" they were when deciding if an airdash is possible. Let's now look at downhover's height requirement.\n"
					"The hoverdown's height requirement is always 105000 (note that this height requirement is the same as for an"
					" \"ascending\" airdash), regardless of whether your speed Y is directed up or down.\n"
					"This makes it possible to airdash even after pressing 3 during hoverdash, if you started going down and are below"
					" the \"ascending\" minimum airdash height.");
				AddTooltip(tooltip);
				ImGui::SameLine();
				ImGui::TextUnformatted(player.playerval0 ? "Yes" : "No");
				AddTooltip(tooltip);
				
				yellowText(searchFieldTitle("Airdash active timer:"));
				tooltip = searchTooltip("Some people call this state 'fast fall'."
					" This timer is set when initiaing an airdash, and it counts the time"
					" during which it continuously sets speed Y to 0. When this timer runs out,"
					" airdash no longer sets speed Y to 0 each frame. This setting speed Y to 0 beats out"
					" and takes priority over any move that would like to change speed Y to something else."
					" Meaning that airdashing and performing some move that would lift you upwards or "
					" pull you downwards would not move you up or down when this timer is still active.\n"
					"The Airdash timer is not specific or exclusive to I-No: all characters, except Potemkin, Bedman"
					" and Raven on his forward dash, have it.\n"
					"The airdash timer decrements not by 1, but by 2 on the last frame of the airdash, and this"
					" is not a bug, it is intentional. Performing a move from the airdash and not letting the airdash"
					" animation simply play to that frame prevents it. That's right: you get 1 less frame of airtime"
					" from an airdash if you don't press any buttons during it. I am very sorry if this caused"
					" any confusion for you.");
				AddTooltip(tooltip);
				ImGui::SameLine();
				sprintf_s(strbuf, "%df", player.wasProhibitFDTimer);
				ImGui::TextUnformatted(strbuf);
				AddTooltip(tooltip);
				
				yellowText(searchFieldTitle("Airdash becoming horizontal timer:"));
				tooltip = searchTooltip("While this timer is active, airdash sets speed not exactly to 0, but:\n"
					"New Speed Y = Old Speed Y - Old Speed Y / 8;\n"
					"So it's bringing it gradually to 0."
					" After this timer becomes 0, the speed will be set to 0 as long as 'Airdash active timer'"
					" is active.");
				AddTooltip(tooltip);
				ImGui::SameLine();
				sprintf_s(strbuf, "%df", player.wasAirdashHorizontallingTimer);
				ImGui::TextUnformatted(strbuf);
				AddTooltip(tooltip);
				
				yellowText(searchFieldTitle("S-CL will have reduced speed Y:"));
				tooltip = searchTooltip("Some people call this state 'fast fall'"
					" If performing S Chemical Love from a hoverdash-3-66, it won't bring you up as much."
					" Additionally, if performing S Chemical Love after a hoverdash-3-66 + j.S (first few frames),"
					" it will achieve the same effect of reducing S Chemical Love's upwards momentum.\n"
					"If both this 'Fast Fall' state, and the 'Airdash active timer' are active,"
					" then the airdash timer takes priority and the resulting speed Y of"
					" S Chemical Love is 0.");
				AddTooltip(tooltip);
				ImGui::SameLine();
				
				const char* txt = "No";
				bool wellThen_IsItFastFallState_QuestionMark = false;
				if (strcmp(player.anim, "NmlAtkAir5C") == 0) {
					if (player.pawn.mem53()) {
						txt = "Yes";
					} else {
						txt = "No";
					}
				} else if (strcmp(player.anim, "AirFDash_Under") == 0) {
					txt = "Yes";
				}
				if (player.wasProhibitFDTimer) {
					if (player.wasAirdashHorizontallingTimer) {
						txt = "Yes, inches to 0";
					} else {
						txt = "Yes, 0";
					}
				}
					
				ImGui::TextUnformatted(txt);
				AddTooltip(tooltip);
				
				bool hasForceDisableFlag = (player.wasForceDisableFlags & 0x1) != 0;
				yellowText(searchFieldTitle("Can cast Note:"));
				ImGui::SameLine();
				ImGui::TextUnformatted(hasForceDisableFlag ? "No" : "Yes");
				
				int timeDuration = player.noteTimeWithSlow;
				
				yellowText(searchFieldTitle("Note time elapsed:"));
				tooltip = searchTooltip("Format: How much time must elapse since the creation of the Note /"
					" How much time must elapse since the creation of the Note in order to reach the next level.");
				AddTooltip(tooltip);
				ImGui::SameLine();
				
				if (player.noteLevel == 5) {
					txt = "Reached max";
				} else {
					txt = printDecimal(player.noteTimeWithSlowMax, 0, 0, false);
				}
				
				sprintf_s(strbuf, "%d/%s (%d hits)", timeDuration, txt, player.noteLevel);
				ImGui::TextUnformatted(strbuf);
				AddTooltip(tooltip);
				
			} else if (player.charType == CHARACTER_TYPE_BEDMAN) {
				
				bool hasForceDisableFlag = (player.wasForceDisableFlags & 0x1) != 0;
				yellowText(searchFieldTitle("Can throw head:"));
				const char* tooltip = searchTooltip("If you have the head, you can perform the following moves:\n"
					"*) Task A;\n"
					"*) Task A';\n"
					"*) D\xc3\xa9j\xc3\xa0 Vu A;\n"
					"*) D\xc3\xa9j\xc3\xa0 Vu A'.\n"
					"If you don't have the head you can't perform them.");
				AddTooltip(tooltip);
				ImGui::SameLine();
				ImGui::TextUnformatted(hasForceDisableFlag ? "No" : "Yes");
				AddTooltip(tooltip);
				
				hasForceDisableFlag = (player.wasForceDisableFlags & 0x2) != 0;
				yellowText(searchFieldTitle("Can do D\xc3\xa9j\xc3\xa0 Vu B or C:"));
				tooltip = searchTooltip("This controls whether you can perform the following moves:\n"
					"*) D\xc3\xa9j\xc3\xa0 Vu B;\n"
					"*) D\xc3\xa9j\xc3\xa0 Vu C.\n"
					"Only one of these can be performed at a time.");
				AddTooltip(tooltip);
				ImGui::SameLine();
				ImGui::TextUnformatted(hasForceDisableFlag ? "No" : "Yes");
				AddTooltip(tooltip);
				
				hasForceDisableFlag = (player.wasForceDisableFlags & 0x4) != 0;
				yellowText(searchFieldTitle("Can do Sheep:"));
				tooltip = searchTooltip("This controls whether you can perform the following moves:\n"
					"*) Hemi Jack.");
				AddTooltip(tooltip);
				ImGui::SameLine();
				ImGui::TextUnformatted(hasForceDisableFlag ? "No" : "Yes");
				AddTooltip(tooltip);
				
				booleanSettingPreset(settings.showBedmanTaskCHeightBuffY);
				
				struct SealInfo {
					const char* uiName;
					unsigned short& timer;
					unsigned short& timerMax;
				};
				SealInfo seals[4] {
					{ searchFieldTitle("Task A Seal Timer:"), player.bedmanInfo.sealA, player.bedmanInfo.sealAMax },
					{ searchFieldTitle("Task A' Seal Timer:"), player.bedmanInfo.sealB, player.bedmanInfo.sealBMax },
					{ searchFieldTitle("Task B Seal Timer:"), player.bedmanInfo.sealC, player.bedmanInfo.sealCMax },
					{ searchFieldTitle("Task C Seal Timer:"), player.bedmanInfo.sealD, player.bedmanInfo.sealDMax }
				};
				for (int j = 0; j < 4; ++j) {
					SealInfo& seal = seals[j];
					yellowText(seal.uiName);
					ImGui::SameLine();
					sprintf_s(strbuf, "%d/%d", seal.timer, seal.timerMax);
					ImGui::TextUnformatted(strbuf);
				}
			} else if (player.charType == CHARACTER_TYPE_RAMLETHAL) {
				
				bool hasForceDisableFlag = (player.wasForceDisableFlags & 0x4) != 0;
				int playerval0 = player.wasPlayerval[0];
				int playerval1 = player.wasPlayerval[1];
				int playerval2 = player.wasPlayerval[2];
				int playerval3 = player.wasPlayerval[3];
				
				yellowText(searchFieldTitle("Can do Calvados:"));
				const char* tooltip = searchTooltip("You can do Calvados if you have both swords equipped.");
				AddTooltip(tooltip);
				ImGui::SameLine();
				ImGui::TextUnformatted(!hasForceDisableFlag && playerval0 && playerval2 ? "Yes" : "No");
				AddTooltip(tooltip);
				
				hasForceDisableFlag = (player.wasForceDisableFlags & 0x10) != 0;
				yellowText(searchFieldTitle("Can do Trance:"));
				tooltip = searchTooltip("You can do Trance if you don't have any of the swords, even if they're not fully deployed yet.");
				AddTooltip(tooltip);
				ImGui::SameLine();
				ImGui::TextUnformatted(!hasForceDisableFlag && !playerval0 && !playerval2 ? "Yes" : "No");
				AddTooltip(tooltip);
				
				hasForceDisableFlag = (player.wasForceDisableFlags & 0x8) != 0;
				yellowText(searchFieldTitle("Can do Cassius:"));
				ImGui::SameLine();
				ImGui::TextUnformatted(!hasForceDisableFlag ? "Yes" : "No");
				
				struct BitInfo {
					StringWithLength hasSwordTitle;
					StringWithLength hasSwordWhatItMeans;
					int hasSword;
					StringWithLength swordDeployedTitle;
					StringWithLength deployedSwordWhatItMeans;
					int swordDeployed;
					StringWithLength animTitle;
					bool kowareSonoba;
					bool timerActive;
					int time;
					int timeMax;
					const char* subAnim;
					const char* anim;
				};
				BitInfo bitInfos[2] {
					{
						"Has S Sword:",
						"This controls the availability of the following moves:\n"
							"*) f.S with Sword;\n"
							"*) f.S without Sword;\n"
							"*) 2S with Sword;\n"
							"*) 6S Summon (makes recovery shorter);\n"
							"*) j.S with Sword;\n"
							"*) j.S without Sword;\n"
							"*) j.6S Summon (makes recovery shorter, airstalls more);\n"
							"*) Marteli (makes startup faster);\n"
							"*) Air Marteli (makes startup faster);\n"
							"*) Calvados (if you also have H Sword);\n"
							"*) Trance (if you don't have H Sword either).",
						playerval0,
						"S Sword Deployed:",
						"This controls the availability of the following moves:\n"
							"*) 6S Summon (maker recovery longer)\n;"
							"*) 2S Summon\n;"
							"*) 4S Recall\n;"
							"*) j.6S Summon (makes recovery longer, airstalls less)\n;"
							"*) j.2S Summon\n;"
							"*) j.4S Recall\n;"
							"*) Marteli (makes startup slower)\n;"
							"*) Air Marteli (makes startup slower).",
						playerval1,
						"S Sword Anim:",
						player.ramlethalSSwordKowareSonoba,
						player.ramlethalSSwordTimerActive,
						player.ramlethalSSwordTime,
						player.ramlethalSSwordTimeMax,
						player.ramlethalSSwordSubanim,
						player.ramlethalSSwordAnim
					},
					{
						"Has H Sword:",
						"This controls the availability of the following moves:\n"
							"*) 5H with Sword;\n"
							"*) 5H without Sword;\n"
							"*) 6H Summon (maker recovery shorter);\n"
							"*) 2H with Sword;\n"
							"*) j.H (with Sword);\n"
							"*) j.H (without Sword);\n"
							"*) j.6H Summon (makes recovery shorter, airstalls more);\n"
							"*) Forpeli (makes startup faster);\n"
							"*) Air Forpeli (makes startup faster);\n"
							"*) Calvados (if you also have S Sword);\n"
							"*) Trance (if you don't have S Sword either).",
						playerval2,
						"H Sword Deployed:",
						"This controls the availability of the following moves:\n"
							"*) 6H Summon (maker recovery longer);\n"
							"*) 4H Recall;\n"
							"*) 2H Summon;\n"
							"*) j.6H Summon (makes recovery longer, airstalls less);\n"
							"*) j.2H Summon;\n"
							"*) j.4H Recall;\n"
							"*) Forpeli (makes startup slower);\n"
							"*) Air Forpeli (makes startup slower).",
						playerval3,
						"H Sword Anim:",
						player.ramlethalHSwordKowareSonoba,
						player.ramlethalHSwordTimerActive,
						player.ramlethalHSwordTime,
						player.ramlethalHSwordTimeMax,
						player.ramlethalHSwordSubanim,
						player.ramlethalHSwordAnim
					}
				};
				
				for (int j = 0; j < 2; ++j) {
					BitInfo& bitInfo = bitInfos[j];
					
					ImGui::Separator();
					
					yellowText(searchFieldTitle(bitInfo.hasSwordTitle));
					tooltip = searchTooltip(bitInfo.hasSwordWhatItMeans);
					AddTooltip(tooltip);
					ImGui::SameLine();
					ImGui::TextUnformatted(bitInfo.hasSword ? "Yes" : "No");
					AddTooltip(tooltip);
					
					yellowText(searchFieldTitle(bitInfo.swordDeployedTitle));
					tooltip = searchTooltip(bitInfo.deployedSwordWhatItMeans);
					AddTooltip(tooltip);
					ImGui::SameLine();
					ImGui::TextUnformatted(bitInfo.swordDeployed ? "Yes" : "No");
					AddTooltip(tooltip);
					
					yellowText(searchFieldTitle(bitInfo.animTitle));
					ImGui::SameLine();
					
					if (bitInfo.subAnim) {
						sprintf_s(strbuf, "%s:%s", bitInfo.anim, bitInfo.subAnim);
						ImGui::TextUnformatted(strbuf);
					} else {
						ImGui::TextUnformatted(bitInfo.anim);
					}
					if (bitInfo.timerActive) {
						ImGui::SameLine();
						if (bitInfo.kowareSonoba) {
							sprintf_s(strbuf, "(until landing + %d)", bitInfo.time);
						} else {
							sprintf_s(strbuf, "(%d/%d)", bitInfo.time, bitInfo.timeMax);
						}
						ImGui::TextUnformatted(strbuf);
					}
					
				}
				
			} else if (player.charType == CHARACTER_TYPE_SIN) {
				
				yellowText(searchFieldTitle("Calorie Gauge:"));
				ImGui::SameLine();
				sprintf_s(strbuf, "%d/16000", player.pawn.exGaugeValue(0));
				ImGui::TextUnformatted(strbuf);
				
				ImGui::PushStyleColor(ImGuiCol_Text, SLIGHTLY_GRAY);
				ImGui::PushTextWrapPos(0.F);
				ImGui::TextUnformatted("When doing Hawk Baker, the horizontal line displayed leads to the wall"
					" in front of you. There's a separator on that line. If your origin point is between that"
					" separator and the wall then you're close enough to the wall to get a \"red\" hit."
					" If you're not close to the wall, then opponent's origin point must be within the"
					" infinite vertical box around you, in order to get the \"red\" hit.");
				ImGui::PopTextWrapPos();
				ImGui::PopStyleColor();
				
			} else if (player.charType == CHARACTER_TYPE_ELPHELT) {
				
				yellowText(searchFieldTitle("Berry Timer:"));
				ImGui::SameLine();
				sprintf_s(strbuf, "%d/180", player.wasResource);
				ImGui::TextUnformatted(strbuf);
				
				yellowText(searchFieldTitle("Can pull Berry in:"));
				ImGui::SameLine();
				char* buf = strbuf;
				size_t bufSize = sizeof strbuf;
				int result;
				if (player.elpheltGrenadeRemainingWithSlow == 255) {
					result = sprintf_s(buf, bufSize, "%s", "???");
				} else {
					result = sprintf_s(buf, bufSize, "%d", player.elpheltGrenadeRemainingWithSlow);
				}
				advanceBuf
				if (player.elpheltGrenadeMaxWithSlow == 255) {
					result = sprintf_s(buf, bufSize, "/%s", "???");
				} else {
					result = sprintf_s(buf, bufSize, "/%d", player.elpheltGrenadeMaxWithSlow);
				}
				ImGui::TextUnformatted(strbuf);
				
				if (ImGui::Button(searchFieldTitle("Ms. Travailler Powerup Explanation"))) {
					printElpheltShotgunPowerup[i] = !printElpheltShotgunPowerup[i];
				}
				if (printElpheltShotgunPowerup[i]) {
					ImGui::PushTextWrapPos(0.F);
					ImGui::PushStyleColor(ImGuiCol_Text, SLIGHTLY_GRAY);
					ImGui::TextUnformatted(searchTooltip("There're three levels of powerup of Elphelt's Max Charge Shotgun:\n"
						"1) The attack has two hitboxes. When only the far hitbox connects, and the close one doesn't, the hit has:\n"
						"1.a) Attack level 4;\n"
						"1.b) The shot is unflickable;\n"
						"1.c) 35 damage;\n"
						"1.d) 50% as chip damage;\n"
						"1.e) Blows away on counterhit;\n"
						"1.f) 300.00 speed X to the opponent;\n"
						"1.g) 160.00 speed Y to the opponent;\n"
						"1.h) 33f untechable time;\n"
						"1.i) The close shot's hitbox is multihit, it can hit up to 10 things on the same frame,"
						" in addition to the far hitbox hitting something;\n"
						"2) When the close hitbox connects, no matter if the far one connected or not, the hit additionally gets:\n"
						"2.) Nothing;\n"
						"3) If the opponent's origin point is within the infinite vertical white box displayed around Elphelt"
						" at the moment of the shot, the hit changes some properties to the following:\n"
						"3.c) 55 damage;\n"
						"3.h) 36f untechable time;\n"
						"New properties:\n"
						"3.j) Prorates combo more: RISC- becomes 9, instead of 6;\n"
						"3.k) Deals 86 more stun (example calculated on the first hit of a combo, this value is presented here"
						" for comparison purposes only): 40 base stun value instead of 35 base stun value;\n"
						"3.l) 38f wallstick;\n"
						"3.m) 76f tumble on counterhit;\n"));
					ImGui::PopStyleColor();
					ImGui::PopTextWrapPos();
				}
				
				yellowText(searchFieldTitle("Ms. Travailler Stance:"));
				ImGui::SameLine();
				ImGui::TextUnformatted(player.playerval0 ? "Yes" : "No");
				
				yellowText(searchFieldTitle("Ms. Travailler Max Charge:"));
				ImGui::SameLine();
				ImGui::TextUnformatted(player.playerval1 ? "Yes" : "No");
				
			} else if (player.charType == CHARACTER_TYPE_LEO) {
				
				printChargeInCharSpecific(i, true, true, 40);
				
				bool hasForceDisableFlag = (player.wasForceDisableFlags & 0x1) != 0;
				yellowText(searchFieldTitle("Can do Graviert W\xc3\xbcrde:"));
				ImGui::SameLine();
				ImGui::TextUnformatted(hasForceDisableFlag ? "No" : "Yes");
				
			} else if (player.charType == CHARACTER_TYPE_JOHNNY) {
				
				yellowText(searchFieldTitle("Mist Finer Level:"));
				ImGui::SameLine();
				sprintf_s(strbuf, "%d", player.pawn.playerVal(1) + 1);
				ImGui::TextUnformatted(strbuf);
				
				yellowText(searchFieldTitle("Bacchus Projectile Timer:"));
				AddTooltip(searchTooltip("You can't perform another Bacchus Sigh until this timer expires, plus one extra frame"
					" after it expires.\n"
					"This timer counts down the time until the Bacchus Sigh projectile stops existing. The 'projectile' means"
					" it has not made contact with the opponent yet."));
				ImGui::SameLine();
				sprintf_s(strbuf, "%d/%d", player.johnnyMistTimerWithSlow, player.johnnyMistTimerMaxWithSlow);
				ImGui::TextUnformatted(strbuf);
				
				yellowText(searchFieldTitle("Bacchus Debuff On Opponent:"));
				AddTooltip(searchTooltip("This timer counts down the time while the opponent has the Bacchus Sigh effect on them."
					" Mist Finer checks this effect shortly before its active frames start and then re-checks it every frame."
					" However, if Mist Finer caught this effect once, it becomes a Guard Break or an unblockable, and when"
					" the next time it checks it and the effect is not there, neither the Guard Break nor unblockable"
					" properties are removed. They merely stop updating. If the opponent is airborne while the"
					" effect is on, Mist Finer becomes an unblockable, and if the opponent is on the ground,"
					" Mist Finer is Guard Break. This can change every frame as long as the effect is on, except that"
					" the Guard Break property cannot be removed, only the unblockable one."));
				ImGui::SameLine();
				sprintf_s(strbuf, "%d/%d", player.johnnyMistKuttsukuTimerWithSlow, player.johnnyMistKuttsukuTimerMaxWithSlow);
				ImGui::TextUnformatted(strbuf);
				
				yellowText(searchFieldTitle("Mist Finer is unblockable:"));
				AddTooltip(searchTooltip("Mist Finer is a true unblockable. "
					" While Bacchus Debuff Timer debuff is active and Johnny is in Mist Finer animation, every frame he checks"
					" if the opponent is in the air or not, and if yes, Mist Finer's attack is set to be a true unblockable,"
					" but if the opponent is on the ground, Mist Finer's attack is either an overhead, a mid or a low, depending"
					" its type, and a Guard Break property is applied instead to the attack.\n"
					" When Bacchus Debuff Timer runs out, Johnny stops updating the properties of his Mist Finer,"
					" and it stays on the last values that were set."
					" It means, that if Mist Finer becomes a true unblockable, and then Bacchus Debuff Timer runs out,"
					" then, even if the opponent lands, Mist Finer will continue to be an unblockable, and not a"
					" Guard Break."));
				ImGui::SameLine();
				ImGui::TextUnformatted(player.pawn.dealtAttack()->guardType == GUARD_TYPE_NONE ? "Yes" : "No");
				
				yellowText(searchFieldTitle("Mist Finer is Guard Break:"));
				AddTooltip(searchTooltip("The Guard Break can only be applied to non-airborne opponents.\n"
					"While Bacchus Debuff Timer debuff is active and Johnny is in Mist Finer animation, every frame he checks"
					" if the opponent is in the air or not, and if yes, Mist Finer's attack is set to be a true unblockable,"
					" but if the opponent is on the ground, Mist Finer's attack is either an overhead, a mid or a low, depending"
					" its type, and a Guard Break property is applied instead to the attack.\n"
					" When Bacchus Debuff Timer runs out, Johnny stops updating the properties of his Mist Finer,"
					" and it stays on the last values that were set."
					" It means, that if Mist Finer acquires the Guard Break property, and then Bacchus Debuff Timer runs out,"
					" then, even if the opponent jumps, Mist Finer will not be an unblockable, and the opponent will be"
					" able to FD it in the air, get hit by Guard Break anyway, but since there's no air stagger animation,"
					" opponent will quickly be able to tech."));
				ImGui::SameLine();
				ImGui::TextUnformatted(player.pawn.dealtAttack()->enableGuardBreak() ? "Yes" : "No");
				
			} else if (player.charType == CHARACTER_TYPE_JACKO) {
				
				booleanSettingPreset(settings.showJackoGhostPickupRange);
				
				booleanSettingPreset(settings.showJackoSummonsPushboxes);
				
				booleanSettingPreset(settings.showJackoAegisFieldRange);
				
				booleanSettingPreset(settings.showJackoServantAttackRange);
				
				bool hasForceDisableFlag = (player.wasForceDisableFlags & 0x1) != 0;
				yellowText(searchFieldTitle("Can do j.D:"));
				ImGui::SameLine();
				ImGui::TextUnformatted(hasForceDisableFlag ? "No" : "Yes");
				
				const char* gaugeNames[4] {
					searchFieldTitle("Organ P Cooldown:"),
					searchFieldTitle("Organ K Cooldown:"),
					searchFieldTitle("Organ S Cooldown:"),
					searchFieldTitle("Organ H Cooldown:")
				};
				
				for (int j = 0; j < 4; ++j) {
					textUnformattedColored(YELLOW_COLOR, gaugeNames[j]);
					ImGui::SameLine();
					sprintf_s(strbuf, "%d/%d", player.pawn.exGaugeValue(j), player.pawn.exGaugeMaxValue(j));
					ImGui::TextUnformatted(strbuf);
				}
				
				yellowText(searchFieldTitle("Aegis Field:"));
				ImGui::SameLine();
				sprintf_s(strbuf, "%d/%d", player.jackoAegisTimeWithSlow, player.jackoAegisTimeMaxWithSlow);
				ImGui::TextUnformatted(strbuf);
				
				yellowText(searchFieldTitle("Holding Ghost:"));
				AddTooltip(searchTooltip("If a cutscene is not currently playing, you gain 0.01 meter per frame for holding a Ghost."));
				ImGui::SameLine();
				ImGui::TextUnformatted(player.pawn.playerVal(0) ? "Yes" : "No");
				
				yellowText(searchFieldTitle("Ghost Nearby:"));
				AddTooltip(searchTooltip("Means you can pick up a Ghost."
					" If in addition to being in range you're not airborne and a cutscene is not currently playing, you gain 0.03 meter per frame."));
				ImGui::SameLine();
				ImGui::TextUnformatted(player.pawn.playerVal(1) ? "Yes" : "No");
				
				struct GhostInfo {
					std::vector<int>& offsets;
					StringWithLength title;
					DWORD maskExplode;
					DWORD maskClockUp;
					int* exp;
					int* creationTimer;
					int* healingTimer;
					const char* dummyName;
					int* dummyTotalFrames;
					const char* servantCooldownName;
					int* servantCooldownTimes;
				};
				GhostInfo ghosts[3] {
					{
						moves.ghostAStateOffsets,
						"P Ghost:",
						2048,
						16384,
						moves.jackoGhostAExp,
						moves.jackoGhostACreationTimer,
						moves.jackoGhostAHealingTimer,
						"GhostADummy",
						&moves.ghostADummyTotalFrames,
						"ServantCoolTimeA",
						moves.servantCooldownA
					},
					{
						moves.ghostBStateOffsets,
						"K Ghost:",
						4096,
						32768,
						moves.jackoGhostBExp,
						moves.jackoGhostBCreationTimer,
						moves.jackoGhostBHealingTimer,
						"GhostBDummy",
						&moves.ghostBDummyTotalFrames,
						"ServantCoolTimeB",
						moves.servantCooldownB
					},
					{
						moves.ghostCStateOffsets,
						"S Ghost:",
						8192,
						65536,
						moves.jackoGhostCExp,
						moves.jackoGhostCCreationTimer,
						moves.jackoGhostCHealingTimer,
						"GhostCDummy",
						&moves.ghostCDummyTotalFrames,
						"ServantCoolTimeC",
						moves.servantCooldownC
					}
				};
				static const char* ghostStateNames[] {
					"Appear",
					"Land",
					"Reappear",
					"Idle",
					"Create",
					"Create",
					"Pick Up",
					"Hold",
					"Put",
					"Throw",
					"Throw",
					"Drop",
					"Damage"
				};
				DWORD playerval2 = (DWORD)player.pawn.playerVal(2);
				for (int j = 0; j < 3; ++j) {
					GhostInfo& info = ghosts[j];
					textUnformattedColored(LIGHT_BLUE_COLOR, searchFieldTitle(info.title));
					Entity p = player.pawn.stackEntity(j);
					if (p && p.isActive()) {
						yellowText(searchFieldTitle("HP:"));
						ImGui::SameLine();
						int maxHits = p.numberOfHits();
						sprintf_s(strbuf, "%d/%d", maxHits - p.numberOfHitsTaken(), maxHits);
						ImGui::TextUnformatted(strbuf);
						
						yellowText(searchFieldTitle("Level:"));
						ImGui::SameLine();
						moves.fillJackoGhostExp(p.bbscrCurrentFunc(), info.exp);
						if (p.mem53() != 2) {
							sprintf_s(strbuf, "%d/3 (EXP: %d/%d)",
								1 + p.mem53(),
								p.mem46(),
								info.exp[p.mem53()]);
							ImGui::TextUnformatted(strbuf);
						} else {
							ImGui::TextUnformatted("3/3");
						}
						
						yellowText(searchFieldTitle("Production Timer:"));
						ImGui::SameLine();
						moves.fillJackoGhostCreationTimer(p.bbscrCurrentFunc(), info.creationTimer);
						int minVal = info.creationTimer[0];
						bool isFast = false;
						if (p.mem60()) {
							if (info.creationTimer[1] < minVal) minVal = info.creationTimer[1];
							isFast = true;
						}
						if (p.mem56()) {
							if (info.creationTimer[2] < minVal) minVal = info.creationTimer[2];
							isFast = true;
						}
						sprintf_s(strbuf, "%d/%d%s", p.mem57(), minVal, isFast ? " (Fast Production)" : "");
						ImGui::TextUnformatted(strbuf);
						
						yellowText(searchFieldTitle("Servants:"));
						ImGui::SameLine();
						sprintf_s(strbuf, "%d/2", p.mem47());
						ImGui::TextUnformatted(strbuf);
						
						int maxAnimFrame = 0;
						int maxRemainingTime = 0;
						int maxTotalCooldown = 0;
						for (int k = 2; k < entityList.count; ++k) {
							Entity serv = entityList.list[k];
							if (serv.isActive() && serv.team() == i && !serv.isPawn()
									&& strcmp(serv.animationName(), info.servantCooldownName) == 0) {
								moves.fillServantCooldown(serv.bbscrCurrentFunc(), info.servantCooldownTimes);
								int totalCooldown = info.servantCooldownTimes[0]
									+ (
										serv.createArgHikitsukiVal1() != 1
											? info.servantCooldownTimes[1]
											: 0
									);
								int animFrame = serv.currentAnimDuration();
								if (totalCooldown - animFrame > maxRemainingTime) {
									maxRemainingTime = totalCooldown - animFrame;
									maxAnimFrame = animFrame;
									maxTotalCooldown = totalCooldown;
								}
							}
						}
						if (maxRemainingTime && maxAnimFrame <= maxTotalCooldown) {
							bool isGray = p.mem47() < 2;
							if (isGray) {
								ImGui::PushStyleColor(ImGuiCol_Text, SLIGHTLY_GRAY);
								ImGui::TextUnformatted(searchFieldTitle("Production Cooldown:"));
							} else {
								yellowText(searchFieldTitle("Production Cooldown:"));
							}
							const char* tooltip = searchTooltip("This Production Cooldown timer only matters if the Ghost has 2 Servants.");
							AddTooltip(tooltip);
							ImGui::SameLine();
							sprintf_s(strbuf, "%d/%d", maxAnimFrame, maxTotalCooldown);
							ImGui::TextUnformatted(strbuf);
							AddTooltip(tooltip);
							if (isGray) {
								ImGui::PopStyleColor();
							}
						}
						
						moves.fillJackoGhostHealingTimer(p.bbscrCurrentFunc(), info.healingTimer);
						if (info.healingTimer[0] != -1  // if -1, then it's Rev1, which doesn't have Healing Timer
								&& p.mem55()
								&& player.playerval0) {
							yellowText(searchFieldTitle("Healing Timer:"));
							ImGui::SameLine();
							int nextTimer = 0;
							int mem48 = p.mem48();
							for (int k = 0; k < 6; ++k) {
								if (mem48 <= info.healingTimer[k]) {
									nextTimer = info.healingTimer[k];
									break;
								}
							}
							if (!nextTimer) {
								sprintf_s(strbuf, "%d/Never", mem48);
							} else {
								sprintf_s(strbuf, "%d/%d", mem48, nextTimer);
							}
							ImGui::TextUnformatted(strbuf);
						}
						
						if ((playerval2 & info.maskClockUp) != 0) {
							moves.fillJackoGhostBuffTimer(p.bbscrCurrentFunc());
							yellowText(searchFieldTitle("Clock Up Timer:"));
							ImGui::SameLine();
							sprintf_s(strbuf, "%d/%d", p.mem52(), moves.jackoGhostBuffTimer);
							ImGui::TextUnformatted(strbuf);
						}
						
						if ((playerval2 & info.maskExplode) != 0) {
							moves.fillJackoGhostExplodeTimer(p.bbscrCurrentFunc());
							yellowText(searchFieldTitle("Explode Timer:"));
							ImGui::SameLine();
							sprintf_s(strbuf, "%d/%d", p.mem49(), moves.jackoGhostExplodeTimer);
							ImGui::TextUnformatted(strbuf);
						}
						
						yellowText("Anim:");
						ImGui::SameLine();
						BYTE* func = p.bbscrCurrentFunc();
						moves.fillGhostStateOffsets(func, info.offsets);
						int stateInd = moves.findGhostState(p.bbscrCurrentInstr() - func, info.offsets);
						if (stateInd >= 0 && stateInd < _countof(ghostStateNames)) {
							ImGui::TextUnformatted(ghostStateNames[stateInd]);
						} else {
							ImGui::TextUnformatted("???");
						}
						
					} else {
						int duration = 0;
						for (int k = 2; k < entityList.count; ++k) {
							p = entityList.list[k];
							if (p.isActive() && p.team() == i && !p.isPawn()
									&& strcmp(p.animationName(), info.dummyName) == 0) {
								if (*info.dummyTotalFrames == 0) {
									BYTE* func = p.bbscrCurrentFunc();
									BYTE* instr;
									for (
											instr = moves.skipInstruction(func);
											moves.instructionType(instr) != Moves::instr_endState;
											instr = moves.skipInstruction(instr)
									) {
										if (moves.instructionType(instr) == Moves::instr_sprite) {
											(*info.dummyTotalFrames) += *(int*)(instr + 4 + 32);
										}
									}
								}
								duration = p.currentAnimDuration();
								break;
							}
						}
						if (duration) {
							yellowText(searchFieldTitle("Set a Ghost Cooldown:"));
							ImGui::SameLine();
							sprintf_s(strbuf, "%d/%d", duration - 1, *info.dummyTotalFrames - 1);
							ImGui::TextUnformatted(strbuf);
						} else {
							ImGui::TextUnformatted("Not set");
						}
					}
					
				}
				
				struct ServantInfo {
					const char* servantTitle[2];
					DWORD mask;
					std::vector<int>& offsets;
					const char** stateNames;
					int stateNamesCount;
					Moves::MayIrukasanRidingObjectInfo* servantAtk;
				};
				static const char* servantStateNames[] {
					"Spawn",
					"Move",
					"Move",
					"Turn",
					"Attack",
					"Attack",
					"Attack",
					"Damage",
					"Death",
					"Death",
					"Wave Goodbye",
					"Waiting",
					"Lose",
					"Win"
				};
				static const char* servantStateNamesSpearman[] {
					"Spawn",
					"Move",
					"Move",
					"Turn",
					"Attack",
					"Attack",
					"Attack",
					"Damage",
					"Damage",
					"Damage",
					"Death",
					"Death",
					"Wave Goodbye",
					"Waiting",
					"Lose",
					"Win"
				};
				ServantInfo servants[3] {
					{
						{ searchFieldTitle("Knight #1:"), searchFieldTitle("Knight #2:") },
						0x800000,
						moves.servantAStateOffsets,
						servantStateNames,
						_countof(servantStateNames),
						moves.servantAAtk
					},
					{
						{ searchFieldTitle("Spearman #1:"), searchFieldTitle("Spearman #2:") },
						0x1000000,
						moves.servantBStateOffsets,
						servantStateNamesSpearman,
						_countof(servantStateNamesSpearman),
						moves.servantBAtk
					},
					{
						{ searchFieldTitle("Magician #1:"), searchFieldTitle("Magician #2:") },
						0x2000000,
						moves.servantCStateOffsets,
						servantStateNames,
						_countof(servantStateNames),
						moves.servantCAtk
					}
				};
				for (int j = 0; j < 3; ++j) {
					ServantInfo& servant = servants[j];
					for (int k = 1; k <= 2; ++k) {
						ProjectileInfo* projectile = nullptr;
						for (ProjectileInfo& iter : endScene.projectiles) {
							if (iter.ptr && iter.team == i && (*(DWORD*)(iter.ptr + 0x120) & servant.mask) != 0
									&& iter.ptr.mem47() == k) {
								projectile = &iter;
								break;
							}
						}
						if (!projectile) continue;
						
						textUnformattedColored(LIGHT_BLUE_COLOR, servant.servantTitle[k - 1]);
						
						yellowText(searchFieldTitle("HP:"));
						ImGui::SameLine();
						int maxHits = projectile->ptr.numberOfHits();
						sprintf_s(strbuf, "%d/%d", maxHits - projectile->ptr.numberOfHitsTaken(), maxHits);
						ImGui::TextUnformatted(strbuf);
						
						yellowText(searchFieldTitle("Level:"));
						ImGui::SameLine();
						sprintf_s(strbuf, "%d", projectile->ptr.createArgHikitsukiVal1() + 1);
						ImGui::TextUnformatted(strbuf);
						
						BYTE* func = projectile->ptr.bbscrCurrentFunc();
						moves.fillServantTimeoutTimer(func);
						yellowText(searchFieldTitle("Timeout Timer:"));
						ImGui::SameLine();
						sprintf_s(strbuf, "%d/%d", projectile->ptr.framesSinceRegisteringForTheIdlingSignal(), moves.servantTimeoutTimer);
						ImGui::TextUnformatted(strbuf);
						
						if (projectile->ptr.mem48()) {
							moves.fillServantClockUpTimer(func);
							yellowText(searchFieldTitle("Clock Up Timer:"));
							ImGui::SameLine();
							sprintf_s(strbuf, "%d/%d", projectile->ptr.mem49(), moves.servantClockUpTimer);
							ImGui::TextUnformatted(strbuf);
						}
						
						if (projectile->ptr.mem53()) {
							moves.fillServantExplosionTimer(func);
							yellowText(searchFieldTitle("Explosion Timer:"));
							ImGui::SameLine();
							sprintf_s(strbuf, "%d/%d", projectile->ptr.mem54(), moves.servantExplosionTimer);
							ImGui::TextUnformatted(strbuf);
						}
						
						moves.fillGhostStateOffsets(func, servant.offsets);
						int state = moves.findGhostState(projectile->ptr.bbscrCurrentInstr() - func, servant.offsets);
						if (state >= 0 && state < servant.stateNamesCount) {
							yellowText("Anim:");
							ImGui::SameLine();
							ImGui::TextUnformatted(servant.stateNames[state]);
							if (state >= 4 && state <= 6) {
								moves.fillServantAtk(func, servant.servantAtk);
								const Moves::MayIrukasanRidingObjectInfo& frames = servant.servantAtk[2 * (state - 4)];
								const Moves::MayIrukasanRidingObjectInfo& framesExtra = servant.servantAtk[2 * (state - 4) + 1];
								int time;
								int totalTime;
								int offset = projectile->ptr.bbscrCurrentInstr() - func;
								if (offset >= framesExtra.frames.front().offset) {
									time = framesExtra.remainingTime(offset, projectile->ptr.spriteFrameCounter());
									totalTime = frames.totalFrames + framesExtra.totalFrames;
								} else if (!projectile->ptr.mem48()) {
									time = frames.remainingTime(offset, projectile->ptr.spriteFrameCounter()) + framesExtra.totalFrames;
									totalTime = frames.totalFrames + framesExtra.totalFrames;
								} else {
									time = frames.remainingTime(offset, projectile->ptr.spriteFrameCounter());
									totalTime = frames.totalFrames;
								}
								ImGui::SameLine();
								sprintf_s(strbuf, "(%d/%d)", totalTime - time, totalTime);
								ImGui::TextUnformatted(strbuf);
							}
						}
						
					}
				}
				
			} else if (player.charType == CHARACTER_TYPE_JAM && jamPantyPtr) {
				
				ImGui::PushItemWidth(80);
				sprintf_s(strbuf, "%d", *jamPantyPtr);
				if (ImGui::BeginCombo(searchFieldTitle("Panty"), strbuf)) {
					for (int i = 0; i <= 7; ++i) {
						ImGui::PushID(i);
						sprintf_s(strbuf, "%d", i);
						if (ImGui::Selectable(strbuf, i == *jamPantyPtr)) {
							*jamPantyPtr = i;
						}
						ImGui::PopID();
					}
					ImGui::EndCombo();
				}
				ImGui::PopItemWidth();
				
			} else if (player.charType == CHARACTER_TYPE_HAEHYUN) {
				
				yellowText(searchFieldTitle("Ball Time Remaining:"));
				const char* tooltip = searchTooltip("This timer does not decrement during superfreeze"
					" or when the ball is in hitstop, from clashing or from hitting someone.");
				AddTooltip(tooltip);
				ImGui::SameLine();
				sprintf_s(strbuf, "%d/%df", player.haehyunBallRemainingTimeWithSlow, player.haehyunBallRemainingTimeMaxWithSlow);
				ImGui::TextUnformatted(strbuf);
				AddTooltip(tooltip);
				
				yellowText(searchFieldTitle("Can Do Ball In:"));
				ImGui::SameLine();
				if ((player.wasForceDisableFlags & 0x1) == 0) {
					ImGui::TextUnformatted("0f");
				} else if (player.haehyunBallTimeWithSlow == -1) {
					sprintf_s(strbuf, "until destroyed+%d/%df", player.haehyunBallTimeMaxWithSlow, player.haehyunBallTimeMaxWithSlow);
					ImGui::TextUnformatted(strbuf);
				} else {
					sprintf_s(strbuf, "%d/%df", player.haehyunBallTimeWithSlow, player.haehyunBallTimeMaxWithSlow);
					ImGui::TextUnformatted(strbuf);
				}
				
				for (int j = 0; j < 10; ++j) {
					if (player.haehyunSuperBallRemainingTimeWithSlow[j] == 0) break;
					sprintf_s(strbuf, "Super Ball #%d Time Remaining:", j + 1);
					yellowText(strbuf);
					AddTooltip(tooltip);
					ImGui::SameLine();
					sprintf_s(strbuf, "%d/%df", player.haehyunSuperBallRemainingTimeWithSlow[j], player.haehyunSuperBallRemainingTimeMaxWithSlow[j]);
					ImGui::TextUnformatted(strbuf);
					AddTooltip(tooltip);
				}
				
			} else if (player.charType == CHARACTER_TYPE_RAVEN) {
				
				yellowText(searchFieldTitle("Excitement:"));
				sprintf_s(strbuf, "%d ticks", player.wasResource);
				ImGui::SameLine();
				ImGui::TextUnformatted(strbuf);
				
				yellowText(searchFieldTitle("Excitement Timer:"));
				AddTooltip(searchTooltip("Excitement timer counts time until you start losing excitement."
					" Excitement is lost only in Rev2. Once it reaches 600, you lose excitement every 20 frames."
					" When in hitstun, Excitement Timer is paused. The Timer keeps counting even during hitstop or superfreeze."));
				int mem55 = player.pawn.mem55();
				int memMax;
				if (mem55 <= 600) memMax = 600;
				else if (mem55 <= 620) memMax = 620;
				else if (mem55 <= 640) memMax = 640;
				else if (mem55 <= 660) memMax = 660;
				else if (mem55 <= 680) memMax = 680;
				else if (mem55 <= 700) memMax = 700;
				else if (mem55 <= 720) memMax = 720;
				else if (mem55 <= 740) memMax = 740;
				else if (mem55 <= 760) memMax = 760;
				else if (mem55 <= 780) memMax = 780;
				else if (mem55 <= 800) memMax = 800;
				sprintf_s(strbuf, "%d/%d", mem55, memMax);
				ImGui::SameLine();
				ImGui::TextUnformatted(strbuf);
				
				yellowText(searchFieldTitle("Damage Multiplier:"));
				sprintf_s(strbuf, "%d%c", player.pawn.damageScale(), '%');
				ImGui::SameLine();
				ImGui::TextUnformatted(strbuf);
				
				yellowText(searchFieldTitle("Slow Time On Opponent:"));
				ImGui::SameLine();
				sprintf_s(strbuf, "%d/%d", player.ravenInfo.slowTime, player.ravenInfo.slowTimeMax);
				ImGui::TextUnformatted(strbuf);
				
				yellowText(searchFieldTitle("Can Do Needle/Orb In:"));
				ImGui::SameLine();
				if (player.ravenNeedleTimeMax == -1) {
					sprintf_s(strbuf, "Until destroyed+%df", player.ravenNeedleTime);
				} else {
					sprintf_s(strbuf, "%d/%df", player.ravenNeedleTime, player.ravenNeedleTimeMax);
				}
				ImGui::TextUnformatted(strbuf);
				
				if (ImGui::Button(searchFieldTitle("Moves Buffed By Excitement"))) {
					printRavenBuffedMoves[i] = !printRavenBuffedMoves[i];
				}
				
				if (printRavenBuffedMoves[i]) {
					yellowText("At 3 or 6 ticks of Excitement:");
					ImGui::TextUnformatted("Ground and Air Scratch;\n"
						"Ground and Air Command Grabs;\n"
						"Needle Slowdown Duration;\n"
						"Orb Number of Hits.\n");
					yellowText("At 3, 6 or other amount of ticks:");
					ImGui::TextUnformatted("Supers\n"
						"Damage of all attacks.");
				}
				
			} else if (player.charType == CHARACTER_TYPE_DIZZY) {
				
				bool hasForceDisableFlag = (player.wasForceDisableFlags & 4096) != 0;
				bool showSpearParameters = player.dizzySpearIsIce || player.dizzyFireSpearTimeMax == -1;
				yellowText(searchFieldTitle("Can Do Ice/Fire Spear:"));
				ImGui::SameLine();
				if (hasForceDisableFlag) {
					if (!showSpearParameters && player.dizzyFireSpearElapsed) {
						sprintf_s(strbuf, "%d/%d", player.dizzyFireSpearTime, player.dizzyFireSpearTimeMax);
						ImGui::TextUnformatted(strbuf);
					} else {
						ImGui::TextUnformatted("After leaves arena");
					}
				} else {
					ImGui::TextUnformatted("Yes");
				}
				
				struct StringTableElem {
					const char* x;
					const char* y;
					const char* speed;
				};
				StringTableElem stringTbl[2] {
					{
						searchFieldTitle("Ice Spear X:"),
						searchFieldTitle("Ice Spear Y:"),
						searchFieldTitle("Ice Spear Speed:")
					},
					{
						searchFieldTitle("Fire Spear X:"),
						searchFieldTitle("Fire Spear Y:"),
						searchFieldTitle("Fire Spear Speed")
					}
				};
				
				StringTableElem& elem = stringTbl[player.dizzySpearIsIce ? 0 : 1];
				
				const char* tooltip;
				if (showSpearParameters) {
					
					yellowText(elem.x);
					ImGui::SameLine();
					printDecimal(player.dizzySpearX, 2, 0, false);
					sprintf_s(strbuf, "%s/%s", printdecimalbuf, player.dizzySpearSpeedX < 0 ? "-17000.00" : "17000.00");
					ImGui::TextUnformatted(strbuf);
					
					yellowText(elem.y);
					ImGui::SameLine();
					printDecimal(player.dizzySpearY, 2, 0, false);
					sprintf_s(strbuf, "%s/%s", printdecimalbuf, player.dizzySpearSpeedY < 0 ? "-1000.00" : "5000.00");
					ImGui::TextUnformatted(strbuf);
					
					yellowText(elem.speed);
					tooltip = "Speed X; Speed Y";
					AddTooltip(tooltip);
					ImGui::SameLine();
					printDecimal(player.dizzySpearSpeedX, 2, 0, false);
					char* buf = strbuf;
					size_t bufSize = sizeof strbuf;
					int result = sprintf_s(buf, bufSize, "%s", printdecimalbuf);
					advanceBuf
					printDecimal(player.dizzySpearSpeedY, 2, 0, false);
					sprintf_s(buf, bufSize, "; %s", printdecimalbuf);
					ImGui::TextUnformatted(strbuf);
					AddTooltip(tooltip);
					
				}
				
				yellowText(searchFieldTitle("Can Do Ice/Fire Scythe:"));
				tooltip = searchTooltip("Time remaining, in frames, until you can perform 'For putting out the light...'"
					" or 'The light was so small in the beginning'.");
				AddTooltip(tooltip);
				ImGui::SameLine();
				sprintf_s(strbuf, "%d/%df", player.dizzyScytheTime, player.dizzyScytheTimeMax);
				ImGui::TextUnformatted(strbuf);
				AddTooltip(tooltip);
				
				yellowText(searchFieldTitle("Can Do Fish:"));
				tooltip = searchTooltip("Time remaining, in frames, until you can perform 'We talked a lot together'"
					" or 'We fought a lot together'. If it says 'No' then idk how long it might take and it will become clearer once"
					" the fish enters a portion of the animation that is more predetermined and depends on fewer variables.");
				AddTooltip(tooltip);
				ImGui::SameLine();
				if (player.dizzyFishTimeMax == -1) {
					ImGui::TextUnformatted("No");
				} else if (player.dizzyFishTimeMax == 9999) {
					sprintf_s(strbuf, "%d/???f", player.dizzyFishTime);
					ImGui::TextUnformatted(strbuf);
				} else {
					sprintf_s(strbuf, "%d/%df", player.dizzyFishTime, player.dizzyFishTimeMax);
					ImGui::TextUnformatted(strbuf);
				}
				AddTooltip(tooltip);
				
				yellowText(searchFieldTitle("Can Do Bubble:"));
				tooltip = searchTooltip("Time remaining, in frames, until you can perform 'Please, leave me alone'"
					" or 'What happens when I'm TOO alone'.");
				AddTooltip(tooltip);
				ImGui::SameLine();
				sprintf_s(strbuf, "%d/%df", player.dizzyBubbleTime, player.dizzyBubbleTimeMax);
				ImGui::TextUnformatted(strbuf);
				AddTooltip(tooltip);
				
				if (game.isTrainingMode()) {
					if (ImGui::Button(searchFieldTitle("Costume"))) {
						DWORD& theValue = *(DWORD*)(*aswEngine + endScene.interRoundValueStorage1Offset + i * 4);
						theValue = 1 - theValue;
						endScene.BBScr_callSubroutine((void*)player.pawn.ent, "UndressCheck");
					}
					AddTooltip("Innocuous button.");
				}
				
			} else if (player.charType == CHARACTER_TYPE_BAIKEN) {
				
				yellowText(searchFieldTitle("Successful Azami:"));
				ImGui::SameLine();
				ImGui::TextUnformatted(player.wasPlayerval[1] ? "Yes" : "No");
				
				yellowText(searchFieldTitle("Azami Follow-Ups Enabled:"));
				ImGui::SameLine();
				ImGui::TextUnformatted(
					player.wasCancels.hasCancel("SakuraGuard")
						|| player.wasCancels.hasCancel("Sakura")
					? "Yes" : "No");
				
				if (ImGui::Button(searchFieldTitle("How Azami Works"))) {
					printHowAzamiWorks[i] = !printHowAzamiWorks[i];
				}
				
				if (printHowAzamiWorks[i]) {
					ImGui::PushTextWrapPos(0.F);
					ImGui::TextUnformatted("Pressing S+H performs a parry move called Azami.\n"
						"5/8/4/7 S+H is Standing Azami.\n"
						"2/1 S+H is Crouching Azami.\n"
						"5/7/4/1 S+H in the air is Aerial Azami.\n"
						"\n"
						"The super armor from Azami is instant, meaning it can be used as a reversal."
						" If you tap Azami, you get super armor for 12f and a recovery of 22f (24 for Aerial Azami)."
						" If you hold Azami, you get super armor for up to 100f and a recovery of 22f (24 for Aerial Azami)."
						" If you held Azami for more than 11f, after you release the button, you have super armor"
						" for one more frame and then it's recovery."
						" The recovery is always the same duration."
						" Aerial Azami has an extra, landing recovery of 6f. It is possible to recover before landing and use that opportunity to"
						" double jump or airdash. If you double jumped, there's no landing recovery when you land. If you airdash,"
						" the landing recovery is still there.\n"
						"\n"
						"If you successfully parry a hit, you lose super armor, gain ability to do Azami follow-ups (starting on the next frame - see Note),"
						" and enter hitstop, and for the duration of the hitstop you are throw invulnerable."
						" You cannot parry again unless you re-enter Azami. If, during hitstop, you fail to re-enter Azami and another hit connects,"
						" you will simply block it automatically, meaning you don't necessarily have to hold block in order to block it."
						" You can switch block between high and low during hitstop and after it and you can block low by just holding 2 (not just 1).\n"
						"If you fail to re-enter Azami by the time hitstop ends, you enter blockstun for the duration you normally would"
						" had you not parried, and you remain throw invulnerable and can still do Azami follow-ups"
						" or re-enter Azami for the entire duration of the blockstun.\n"
						"\n"
						"Note about how quickly Azami follow-ups become available:\n"
						"If you successfully parry a hit, and immediately on the next frame attempt to re-enter Azami"
						" by holding 8/4/7/1 S+H (or another method) and simultaneously try to do a follow-up by pressing P, K or D on the same frame (or buffering the button press),"
						" the Azami re-entry takes priority over the follow-up for that frame (the frame that is immediately after the hit connects),"
						" and the follow-up comes out on the NEXT frame thanks to its button press still remaining in the buffer."
						" So the follow-up CAN be delayed by 1f if you were holding Azami with 8/4/7/1. This does not happen if you let"
						" yourself re-enter Azami first, and attempt the follow-up on a later frame.\n"
						"\n"
						"There's a number of ways you can re-enter Azami.\n"
						"*) During hitstop.\n"
						"   If you're in hitstop, you can re-enter Azami by:\n"
						"  *) Hold 8/4/7 + one or more of P/K/S/H/D for Standing Azami;\n"
						"  *) Hold 1 + one or more of P/K/S/H/D for Crouching Azami;\n"
						"  *) Hold 7/4/1 + one or more of P/K/S/H/D for Aerial Azami;\n"
						"  *) 5/8/4/7 + PRESS S+H for Standing Azami;\n"
						"  *) 2/1 + PRESS S+H for Crouching Azami;\n"
						"  *) 5/7/4/1 + PRESS S+H for Aerial Azami;\n"
						"*) After hitstop.\n"
						"   After hitstop ends, you gain extra ways to re-enter Azami:\n"
						"  *) Hold 5 + one or more of P/K/S/H/D for Standing Azami;\n"
						"  *) Hold 2 + one or more of P/K/S/H/D for Crouching Azami;\n"
						"  *) Hold 5 + one or more of P/K/S/H/D for Aerial Azami;\n"
						"\n"
						"This means it is possible to get hit a second time during hitstop and not parry that hit, if you were not holding 8/4/7/1 + P/K/S/H/D"
						" and were instead holding, for example, 5 P/K/S/H/D."
						" Certain projectiles can hit you during hitstop, and projectile YRC can create situations where you get hit during hitstop.\n"
						"\n"
						"As soon as you re-enter Azami, you:\n"
						"*) Lose throw invul (see Note below);\n"
						"*) Regain super armor.\n"
						"\n"
						"Note: If you enter or re-enter Azami from blockstun, then you have the standard 5f throw protection from leaving blockstun."
						" Similarly, if you enter Azami after hitstun or wakeup, you have the standard throw protection that lasts"
						" from the moment you leave hitstun/wakeup, which is 6f for hitstun and 9f for wakeup. Azami does not disrupt (or extend) that."
						" This throw invulnerability time may be extended by hitstop. For example, if you parry a hit on wakeup, you would enter hitstop,"
						" and during that hitstop you'd be throw invulnerable, but also after the hitstop you'd still be throw invulnerable for 8 frames,"
						" assuming you re-entered Azami after hitstop. If, however, you re-enter Azami during hitstop by holding 4SH, you would leave"
						" hitstop sooner and start losing throw invul sooner.\n"
						"\n"
						"If you re-enter Azami by pressing S+H, then you:\n"
						"*) Lose ability to do Azami follow-ups.\n"
						"This only happens if you use inputs that involve pressing S+H, such as 5/8/4/7 + press S+H, and does not happen with inputs that involve holding S+H.\n"
						"\n"
						"Red Azami.\n"
						"If you're in blockstun, and that blockstun is not caused by leaving another Azami, then cancelling said blockstun"
						" into an Azami produces a Red Azami, with a distinctive colored flash. You can cancel into a Red Azami even during hitstop,"
						" but for some reason, the first 2f of initial hitstop cannot be cancelled into Red Azami (the button press gets buffered and carries"
						" over to the 3rd frame where you can cancel, so no worries).\n"
						"Tapping or holding Red Azami for 1-3f gives you 3f super armor (does not depend on how long you tapped/held for,"
						" as long as the tap/hold is in 1-3f range), followed by 43f recovery.\n"
						"Holding Red Azami for 4-5f gives 6f super armor, followed by 40f recovery.\n"
						"Holding Red Azami for 6f gives 7f super armor, followed by 40f recovery.\n"
						"Holding Red Azami for 7f gives 8f super armor, followed by 40f recovery.\n"
						"Holding Red Azami for 8f gives 8f super armor, followed by 41f recovery.\n"
						"You cannot hold Red Azami for more than 8f. 8f is the maximum duration of super armor you can get.\n"
						"Successfully parrying a hit with Red Azami produces the same result as parrying with a non-Red Azami.");
					ImGui::PopTextWrapPos();
				}
			
			} else if (player.charType == CHARACTER_TYPE_ANSWER) {
				
				Entity p;
				
				const char* scrollTitles[8] {
					searchFieldTitle("Scroll 1:"),
					searchFieldTitle("Scroll 2:"),
					searchFieldTitle("Scroll 3:"),
					searchFieldTitle("Scroll 4:"),
					searchFieldTitle("Scroll 5:"),
					searchFieldTitle("Scroll 6:"),
					searchFieldTitle("Scroll 7:"),
					searchFieldTitle("Scroll 8:")
				};
				const char* tooltip;
				
				tooltip = searchTooltip("Time remaining until scroll disappears.");
				for (int j = 0; j < 8; ++j) {
					p = player.pawn.stackEntity(j);
					if (p && p.isActive() && !p.mem45()) {
						yellowText(scrollTitles[j]);
						AddTooltip(tooltip);
						ImGui::SameLine();
						sprintf_s(strbuf, "%d/720f", p.currentAnimDuration());
						ImGui::TextUnformatted(strbuf);
						AddTooltip(tooltip);
					}
				}
				
				yellowText(searchFieldTitle("Scroll Cling Ends In:"));
				ImGui::SameLine();
				if (strcmp(player.anim, "Ami_Hold") == 0 && player.pawn.hasUpon(3)) {
					sprintf_s(strbuf, "%d/120f", player.animFrame);
					ImGui::TextUnformatted(strbuf);
				} else {
					ImGui::TextUnformatted("Not on scroll");
				}
				
				yellowText(searchFieldTitle("Can Do Card In:"));
				ImGui::SameLine();
				sprintf_s(strbuf, "%d/%df", player.answerCantCardTime, player.answerCantCardTimeMax);
				ImGui::TextUnformatted(strbuf);
				
				struct CardInfo {
					StringWithLength title;
					int timer;
				};
				CardInfo cards[2] {
					{
						"S Card:",
						-1
					},
					{
						"H Card:",
						-1
					}
				};
				for (int j = 2; j < entityList.count; ++j) {
					p = entityList.list[j];
					if (p.isActive() && p.team() == i && !p.isPawn() && strcmp(p.animationName(), "Meishi") == 0 && p.spriteFrameCounterMax() == 720) {
						cards[p.createArgHikitsukiVal2()].timer = p.currentAnimDuration();
					}
				}
				
				for (int j = 0; j < 2; ++j) {
					yellowText(searchFieldTitle(cards[j].title));
					ImGui::SameLine();
					if (cards[j].timer == -1) {
						ImGui::TextUnformatted("Not present");
					} else {
						sprintf_s(strbuf, "%d/720f", cards[j].timer);
						ImGui::TextUnformatted(strbuf);
					}
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
			
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				if (player.throwRangeValid) {
					sprintf_s(strbuf, "%d", player.throwRange);
					printNoWordWrap
				} else {
					printNoWordWrapArg("--")
				}
				
				if (i == 0) {
					ImGui::TableNextColumn();
					CenterAlignedText(searchFieldTitle("Throw Range"));
					AddTooltip(searchTooltip("These values correspond to the ones used by the last throw performed.\n"
						"In particular, this value is from the part of the throw that checks for the opponent's pushbox to touch the boundaries determined by player's pushbox +- the throw range specified.\n"
						"The range is relative to the player's own pushbox boundaries."));
				}
			}
			
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				if (player.throwXValid) {
					sprintf_s(strbuf, "from %d to %d",
						player.throwMinX,
						player.throwMaxX);
					printNoWordWrap
				} else {
					printNoWordWrapArg("--")
				}
				
				if (i == 0) {
					ImGui::TableNextColumn();
					CenterAlignedText(searchFieldTitle("Throw Box X"));
					AddTooltip(searchTooltip("These values correspond to the ones used by the last throw performed.\n"
						"In particular, these are values from the part of the throw that checks for the opponent's origin point to be within the boundaries outlined by these coordinates.\n"
						"The coordinates shown are relative to the global space."));
				}
			}
			
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				if (player.throwYValid) {
					sprintf_s(strbuf, "from %d to %d",
						player.throwMinY,
						player.throwMaxY);
					printNoWordWrap
				} else {
					printNoWordWrapArg("--")
				}
				
				if (i == 0) {
					ImGui::TableNextColumn();
					CenterAlignedText(searchFieldTitle("Throw Box Y"));
					AddTooltip(searchTooltip("These values correspond to the ones used by the last throw performed.\n"
						"In particular, these are values from the part of the throw that checks for the opponent's origin point to be within the boundaries outlined by these coordinates.\n"
						"The coordinates shown are relative to the global space."));
				}
			}
			
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				sprintf_s(strbuf, "%d; %d", player.pushboxWidth, player.pushboxHeight);
				printNoWordWrap
				
				if (i == 0) {
					ImGui::TableNextColumn();
					CenterAlignedText(searchFieldTitle("Pushbox W; H"));
					AddTooltip(searchTooltip("Current pushbox width and height in the format \"Width; Height\"."));
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
			const char* lastName = nullptr;
			int lastNameDuration = 0;
			prepareLastNames(&lastName, player, false, &lastNameDuration);
			int animNamesCount = player.prevStartupsDisp.countOfNonEmptyUniqueNames(&lastName, 1, useSlang);
			ImGui::PushStyleVarX(ImGuiStyleVar_ItemSpacing, 0.F);
			yellowText(searchFieldTitle(animNamesCount > 1 ? "Anims: " : "Anim: ", nullptr));
			char* buf = strbuf;
			size_t bufSize = sizeof strbuf;
			player.prevStartupsDisp.printNames(buf, bufSize, &lastName,
				1,
				useSlang,
				false,
				animNamesCount > 1,
				&lastNameDuration);
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
					false,
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
	searchCollapsibleSection("Damage/RISC/Stun Calculation");
	for (int i = 0; i < two; ++i) {
		if (showDamageCalculation[i] || searching) {
			ImGui::PushID(i);
			sprintf_s(strbuf, searching ? "search_damage" : "  Damage/RISC/Stun Calculation (P%d)", i + 1);
			ImGui::SetNextWindowSize({
				ImGui::GetFontSize() * 35.F,
				150.F
			}, ImGuiCond_FirstUseEver);
			if (searching) {
				ImGui::SetNextWindowPos({ 100000.F, 100000.F }, ImGuiCond_Always);
			}
			ImGui::Begin(strbuf, showDamageCalculation + i, searching ? ImGuiWindowFlags_NoSavedSettings : 0);
			drawPlayerIconInWindowTitle(i);
			
			const PlayerInfo& player = endScene.players[1 - i];
			
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
				yellowText(strbuf);
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
						yellowText(searchFieldTitle("Hit Number: "));
						ImGui::SameLine();
						sprintf_s(strbuf, "%d", hitCounter + 1);
						ImGui::TextUnformatted(strbuf);
						ImGui::PopStyleVar();
					}
					--hitCounter;
					
					ImGui::PushStyleVarX(ImGuiStyleVar_ItemSpacing, 0.F);
					
					yellowText(searchFieldTitle("Attack Name: "));
					ImGui::SameLine();
					ImGui::TextUnformatted(useSlang && dmgCalc.attackSlangName ? dmgCalc.attackSlangName : dmgCalc.attackName);
					if (dmgCalc.nameFull || useSlang && dmgCalc.attackSlangName && dmgCalc.attackName) {
						AddTooltip(dmgCalc.nameFull ? dmgCalc.nameFull : dmgCalc.attackName);
					}
					
					printAttackLevel(dmgCalc);
					
					yellowText(searchFieldTitle("Is Projectile: "));
					ImGui::SameLine();
					ImGui::TextUnformatted(dmgCalc.isProjectile ? "Yes" : "No");
					
					yellowText(searchFieldTitle("Guard Type: "));
					ImGui::SameLine();
					const char* guardTypeStr;
					if (dmgCalc.isThrow) {
						guardTypeStr = "Throw";
					} else {
						guardTypeStr = formatGuardType(dmgCalc.guardType);
					}
					ImGui::TextUnformatted(guardTypeStr);
					
					yellowText(searchFieldTitle("Air Blockable: "));
					AddTooltip(searchTooltip("Is air blockable - if not, then requires Faultless Defense to be blocked in the air."));
					ImGui::SameLine();
					ImGui::TextUnformatted(dmgCalc.airUnblockable ? "No" : "Yes");
					
					if (dmgCalc.guardCrush || searching) {
						yellowText(searchFieldTitle("Guard Crush: "));
						AddTooltip(searchTooltip("Guard break. When blocked, this attack causes the defender to enter hitstun on the next frame."));
						ImGui::SameLine();
						ImGui::TextUnformatted(dmgCalc.guardCrush ? "Yes" : "No");
					}
					
					ImGui::PopStyleVar();
					
					zerohspacing
					yellowText(searchFieldTitle("Last Hit Result: "));
					ImGui::SameLine();
					ImGui::TextUnformatted(formatHitResult(dmgCalc.hitResult));
					_zerohspacing
					
					if (dmgCalc.hitResult == HIT_RESULT_BLOCKED) {
						zerohspacing
						yellowText(searchFieldTitle("Block Type: "));
						ImGui::SameLine();
						ImGui::TextUnformatted(formatBlockType(dmgCalc.blockType));
						_zerohspacing
						if (dmgCalc.blockType != BLOCK_TYPE_FAULTLESS) {
							const DmgCalc::DmgCalcU::DmgCalcBlock& data = dmgCalc.u.block;
							if (ImGui::BeginTable("##DmgCalc", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_NoPadOuterX)) {
								ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0.5f);
								ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.5f);
								ImGui::TableHeadersRow();
								
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
							
							int damageAfterHpScaling;
							int x = printBaseDamageCalc(dmgCalc, &damageAfterHpScaling);
							int baseDamage = x;
							
							int oldX = x;
							if (data.increaseDmgBy50Percent) {
								x = x * 150 / 100;
								ImGui::TableNextColumn();
								const char* tooltip = searchTooltip("Maybe Dustloop or someone knows what this is.");
								zerohspacing
								yellowText("Dmg");
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
								yellowText(strbuf);
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
								yellowText("Dmg");
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
								yellowText(strbuf);
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
								yellowText("Dmg");
								ImGui::SameLine();
								ImGui::TextUnformatted(" / Stylish");
								_zerohspacing
								ImGui::TableNextColumn();
								zerohspacing
								sprintf_s(strbuf, "%d / %d%c = ", oldX, data.stylishDamageInverseModifier, '%');
								ImGui::TextUnformatted(strbuf);
								ImGui::SameLine();
								sprintf_s(strbuf, "%d", x);
								yellowText(strbuf);
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
								yellowText("Dmg");
								ImGui::SameLine();
								ImGui::TextUnformatted(" * Handicap");
								_zerohspacing
								ImGui::TableNextColumn();
								zerohspacing
								sprintf_s(strbuf, "%d * %d%c = ", oldX, data.handicap, '%');
								ImGui::TextUnformatted(strbuf);
								ImGui::SameLine();
								sprintf_s(strbuf, "%d", x);
								yellowText(strbuf);
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
								yellowText("Dmg");
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
								yellowText(strbuf);
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
							yellowText("Dmg");
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
							yellowText(strbuf);
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
							yellowText("Dmg");
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
							yellowText(strbuf);
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
							yellowText("Dmg");
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
							yellowText(strbuf);
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
							yellowText("Dmg");
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
							yellowText(strbuf);
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
							yellowText("Damage");
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
							yellowText(strbuf);
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
							yellowText("Damage");
							ImGui::SameLine();
							ImGui::TextUnformatted(" or Min ");
							ImGui::SameLine();
							yellowText("Dmg");
							_zerohspacing
							ImGui::TableNextColumn();
							if (data.minimumDamagePercent != 0 && x < minDmg) x = minDmg;
							sprintf_s(strbuf, "%d", x);
							yellowText(strbuf);
							
							x = printDamageGutsCalculation(x, data.defenseModifier, data.gutsRating, data.guts, data.gutsLevel);
							
							ImGui::TableNextColumn();
							ImGui::TextUnformatted(searchFieldTitle("HP<=Dmg and HP>=30% MaxHP"));
							AddTooltip(searchTooltip("When HP at the moment of hit is less than or equal to the damage,"
								" and HP is greater than or equal to max HP * 30% (i.e. HP>=126),"
								" the damage gets changed to:\n"
								"Damage = HP - Max HP * 5% or, in other words, Damage = HP - 21"));
							ImGui::TableNextColumn();
							bool attackIsTooOP = dmgCalc.oldHp <= x && dmgCalc.oldHp >= dmgCalc.maxHp * 30 / 100;
							ImGui::TextUnformatted(attackIsTooOP ? "Yes" : "No");
							
							ImGui::TableNextColumn();
							tooltip = "Damage after change due to the condition above.";
							zerohspacing
							yellowText("Damage");
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
							yellowText(strbuf);
							_zerohspacing
							
							
							ImGui::TableNextColumn();
							ImGui::TextUnformatted(searchFieldTitle("Attack Is Kill"));
							AddTooltip(searchTooltip("This was observed to not happen immediately when landing an IK, but it does happen during the IK cinematic."
								" When an attack is a kill, it always deals damage equal to the entire remaining health of the defender no matter what."));
							ImGui::TableNextColumn();
							ImGui::TextUnformatted(data.kill ? "Yes" : "No");
							
							ImGui::TableNextColumn();
							yellowText("Damage");
							AddTooltip("Damage after change due to the condition above.");
							ImGui::TableNextColumn();
							if (data.kill) x = dmgCalc.oldHp;
							sprintf_s(strbuf, "%d", x);
							yellowText(strbuf);
							
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
							tooltip = searchTooltip("This multiplier is fixed and is always applied. Attack's stun must be within [0; 15000].");
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
				yellowText("Last hit result: ");
				ImGui::SameLine();
				ImGui::TextUnformatted(formatHitResult(HIT_RESULT_NONE));
				ImGui::PopStyleVar();
				
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
			
			if (player.pawn) {
				bool kizetsu = player.pawn.dizzyMashAmountLeft() > 0 || player.cmnActIndex == CmnActKizetsu;
				if (endScene.isIGiveUp() && !searching) {
					ImGui::TextUnformatted("Online non-observer match running.");
				} else if (!player.pawn) {
					ImGui::TextUnformatted("A match isn't currently running");
				} else if (player.cmnActIndex != CmnActJitabataLoop && !kizetsu) {
					ImGui::TextUnformatted(searchFieldTitle("Not in stagger/stun"));
				} else if (kizetsu) {
					zerohspacing
					yellowText(searchFieldTitle("In hitstop: "));
					ImGui::SameLine();
					ImGui::TextUnformatted(player.hitstop ? "Yes (1/3 multiplier)" : "No");
					
					yellowText(searchFieldTitle("Stunmash Remaining: "));
					AddTooltip(searchTooltip("For every left/right direction press you get a 15 point reduction."
						" For every frame, if on that frame you pressed any of PKSHD buttons, you get a 15 point reduction."
						" Pressing a direction AND a button on the same frame combines these reductions."
						" Pressing more than one of PKSHD on the same frame is still a 15 point reduction (no increase from multiple buttons)."
						" If you're in hitstop, in both cases you get a 5 reduction instead of 15, and"
						" you can still combine both direction and a button press to get 5+5=10 reduction on that frame."
						" When not in hitstop, the stunmash remaining automatically decreases by 10 each frame and this can be combined"
						" with your own input of direction and button presses for a maximum of 40 reduction per frame."
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
							
							yellowText(searchFieldTitle("Recovery Animation: "));
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
					
					yellowText(searchFieldTitle("Stagger Duration: "));
					AddTooltip(searchTooltip("The original amount of stagger inflicted by the attack."));
					ImGui::SameLine();
					sprintf_s(strbuf, "%d", mashMax);
					ImGui::TextUnformatted(strbuf);
					
					yellowText(searchFieldTitle("Mashed: "));
					AddTooltip(searchTooltip("How much you've mashed vs how much you can possibly mash. Mashing above the limit achieves no extra stagger reduction."));
					ImGui::SameLine();
					sprintf_s(strbuf, "%d/%d", mashedAmount, player.pawn.bbscrvar3());
					ImGui::TextUnformatted(strbuf);
					
					yellowText(searchFieldTitle("Animation Duration: "));
					AddTooltip(searchTooltip("The current stagger animation's duration. Does not advance during hitstop."
						" Something of note is that it always starts on 1. So when you leave hitstop, it's already on 2."
						" That means you get one free frame of stagger progress, which means that stagger will always last"
						" 1 frame less than what is formally declared in the attack, even if you don't mash."
						" And if you entered stagger without hitstop, like on Bacchus Sigh-buffed Mist Finer blocked hit,"
						" the game forcibly deducts 1 frame anyway. So it's always 1 frame less than declared, with"
						" or without hitstop."));
					ImGui::SameLine();
					sprintf_s(strbuf, "%d%s", player.animFrame, player.hitstop ? " (is in hitstop)" : "");
					ImGui::TextUnformatted(strbuf);
					
					float cursorY = ImGui::GetCursorPosY();
					ImGuiStyle& style = ImGui::GetStyle();
					ImGui::SetCursorPosY(cursorY + style.FramePadding.y);
					yellowText(searchFieldTitle("Progress Previous: "));
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
					
					yellowText(searchFieldTitle("Progress Now: "));
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
					
					yellowText(searchFieldTitle("Started Recovery: "));
					AddTooltip(searchTooltip("Has the 4f stagger recovery animation started?"));
					ImGui::SameLine();
					ImGui::TextUnformatted(player.pawn.bbscrvar() ? "Yes" : "No");
					
					yellowText(searchFieldTitle("Recovery Animation: "));
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
			} else {
				ImGui::TextUnformatted("Match isn't running.");
			}
			
			ImGui::End();
			ImGui::PopID();
		}
	}
	popSearchStack();
	searchCollapsibleSection("Combo Recipe");
	for (int i = 0; i < two; ++i) {
		if (showComboRecipe[i] || searching) {
			ImGui::PushID(i);
			sprintf_s(strbuf, searching ? "search_comborecipe%d" : "  Combo Recipe (P%d)", i + 1);
			if (searching) {
				ImGui::SetNextWindowPos({ 100000.F, 100000.F }, ImGuiCond_Always);
			}
			ImGui::SetNextWindowSize({ 300.F, 300.F }, ImGuiCond_FirstUseEver);
			ImGui::Begin(strbuf, showComboRecipe + i, searching ? ImGuiWindowFlags_NoSavedSettings
				: settings.comboRecipe_transparentBackground
					? ImGuiWindowFlags_NoBackground
					: 0);
			PlayerInfo& player = endScene.players[i];
			
			drawPlayerIconInWindowTitle(i);
			
			ImVec2 cursorPosStart = ImGui::GetCursorPos();
			
			GGIcon scaledIcon = scaleGGIconToHeight(cogwheelIcon, 16.F);
			const ImVec2& itemSpacing = ImGui::GetStyle().ItemSpacing;
			const bool showingSettings = showComboRecipeSettings[i];
			const bool transparentBackground = settings.comboRecipe_transparentBackground;
			if (showingSettings) {
				ImGui::SetCursorPosY(cursorPosStart.y + scaledIcon.size.y + itemSpacing.y * 3.F);
				settingsPresetsUseOutlinedText = transparentBackground;
				booleanSettingPreset(settings.comboRecipe_showDelaysBetweenCancels);
				booleanSettingPreset(settings.comboRecipe_showIdleTimeBetweenMoves);
				booleanSettingPreset(settings.comboRecipe_showDashes);
				booleanSettingPreset(settings.comboRecipe_showWalks);
				booleanSettingPreset(settings.comboRecipe_showSuperJumpInstalls);
				booleanSettingPreset(settings.comboRecipe_transparentBackground);
				settingsPresetsUseOutlinedText = false;
			}
			
			ImVec2 cursorPosForTable = ImGui::GetCursorPos();
			
			ImVec2 buttonCursorPos = {
				cursorPosStart.x + (
					showingSettings
						? 0.F
						: (ImGui::GetContentRegionAvail().x - scaledIcon.size.x - itemSpacing.x * 2.F)
				),
				cursorPosStart.y
			};
			ImGui::SetCursorPos(buttonCursorPos);
			if (transparentBackground) {
				ImGui::PushStyleColor(ImGuiCol_Button, { 0.F, 0.F, 0.F, 0.F });
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 1.F, 1.F, 1.F, 0.25F });
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 1.F, 1.F, 1.F, 1.F });
			}
			
			if (ImGui::Button("##ComboRecipeCogwheel",
				{
					scaledIcon.size.x + itemSpacing.x * 2.F,
					scaledIcon.size.y + itemSpacing.y * 2.F
				})) {
				showComboRecipeSettings[i] = !showComboRecipeSettings[i];
			}
			
			if (transparentBackground) {
				ImGui::PopStyleColor(3);
			}
			
			AddTooltip("Settings for the Combo Recipe panel.\n"
				"The settings are shared between P1 and P2.\n"
				"\n"
				"Extra Info: The '>' symbol at the end of a move means that the next move is being cancelled from that move.\n"
				"The ',' symbol at the end of a move means that the next move is being linked or done after some idle time after that move.");
			bool isHovered = ImGui::IsItemHovered();
			bool mousePressed = ImGui::IsMouseDown(ImGuiMouseButton_Left);
			ImGui::SetCursorPos({
				buttonCursorPos.x + itemSpacing.x,
				buttonCursorPos.y + itemSpacing.y + (isHovered && mousePressed ? 1.F : 0.F)
			});
			ImVec4 tint { 1.F, 1.F, 1.F, 1.F };
			if (isHovered && mousePressed) {
				tint = { 0.5F, 0.5F, 0.5F, 1.F };
			} else if (isHovered) {
				tint = { 0.75F, 0.75F, 1.F, 1.F };
			}
			ImGui::Image((ImTextureID)TEXID_GGICON, scaledIcon.size, scaledIcon.uvStart, scaledIcon.uvEnd, tint);
			
			ImGui::SetCursorPos(cursorPosForTable);
			
			if (ImGui::BeginTable("##ComboRecipe",
						1,
						ImGuiTableFlags_Borders
						| ImGuiTableFlags_RowBg
						| ImGuiTableFlags_NoSavedSettings
						| ImGuiTableFlags_NoPadOuterX)
			) {
				ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch, 1.F);
				
				int rowCount = 1;
				size_t nextJ = 0;
				size_t comboRecipeSize = player.comboRecipe.size();
				
				enum IsLinkEnum {
					IS_LINK_UNKNOWN,
					IS_LINK_NO,
					IS_LINK_YES
				} isLink = IS_LINK_UNKNOWN;
				
				bool lastElemIsDelayed = false;
				
				for (size_t j = 0; j < comboRecipeSize; ++j) {
					const ComboRecipeElement& elem = player.comboRecipe[j];
					
					if (j == nextJ) {
						isLink = IS_LINK_UNKNOWN;
						for (++nextJ; nextJ != comboRecipeSize; ++nextJ) {
							const ComboRecipeElement& nextElem = player.comboRecipe[nextJ];
							if (!nextElem.isProjectile && (!nextElem.artificial || nextElem.isJump)) {
								isLink = nextElem.doneAfterIdle ? IS_LINK_YES : IS_LINK_NO;
								break;
							}
						}
					}
					
					if (elem.cancelDelayedBy
							&& (
								elem.doneAfterIdle
									? settings.comboRecipe_showIdleTimeBetweenMoves
									: settings.comboRecipe_showDelaysBetweenCancels
							)) {
						
						int correctedCancelDelayedBy = elem.cancelDelayedBy;
						if (lastElemIsDelayed) {
							const ComboRecipeElement& lastElem = player.comboRecipe[j - 1];
							int timeDifference = elem.timestamp - lastElem.timestamp;
							if (elem.cancelDelayedBy + 1 > timeDifference) {
								correctedCancelDelayedBy = timeDifference - 1;
							}
						}
						
						lastElemIsDelayed = correctedCancelDelayedBy > 0;
						
						if (lastElemIsDelayed) {
							
							ImGui::TableNextColumn();
							sprintf_s(strbuf, "%u)", rowCount++);
							if (transparentBackground) {
								outlinedText(ImGui::GetCursorPos(), strbuf, &YELLOW_COLOR, nullptr, true);
							} else {
								yellowText(strbuf);
							}
							ImGui::SameLine();
							
							ImGui::PushStyleColor(ImGuiCol_Text, SLIGHTLY_GRAY);
							if (elem.doneAfterIdle) {
								sprintf_s(strbuf, "(Idle %df)", elem.cancelDelayedBy);
							} else {
								sprintf_s(strbuf, "(Delay %df)", elem.cancelDelayedBy);
							}
							if (transparentBackground) {
								outlinedText(ImGui::GetCursorPos(), strbuf, nullptr, nullptr, true);
							} else {
								ImGui::TextUnformatted(strbuf);
							}
							ImGui::PopStyleColor();
							
						}
					} else {
						lastElemIsDelayed = false;
					}
					
					if (elem.dashDuration) {
						if (elem.isWalkForward || elem.isWalkBackward) {
							if (!settings.comboRecipe_showWalks) continue;
						} else if (!settings.comboRecipe_showDashes) continue;
					}
					
					if (elem.isSuperJumpInstall && !settings.comboRecipe_showSuperJumpInstalls) continue;
					
					const char* chosenName;
					if (settings.useSlangNames && elem.slangName) {
						chosenName = elem.slangName;
					} else {
						chosenName = elem.name;
					}
					
					ImGui::TableNextColumn();
					sprintf_s(strbuf, "%u)", rowCount++);
					if (transparentBackground) {
						outlinedText(ImGui::GetCursorPos(), strbuf, &YELLOW_COLOR, nullptr, true);
					} else {
						yellowText(strbuf);
					}
					ImGui::SameLine();
					
					const char* linkText = "";
					if (!elem.isProjectile && (!elem.artificial || elem.isJump) && isLink != IS_LINK_UNKNOWN) {
						if (isLink == IS_LINK_YES) {
							linkText = ",";
						} else {
							linkText = " >";
						}
					}
					
					if (!elem.dashDuration) {
						const char* lastSuffix = "";
						if (elem.whiffed && elem.isMeleeAttack) {
							lastSuffix = " (Whiff)";
						} else if (elem.otg) {
							lastSuffix = " (OTG)";
						} else if (elem.counterhit) {
							lastSuffix = " (Counterhit)";
						}
						sprintf_s(strbuf, "%s%s%s%s",
							chosenName,
							elem.isProjectile ? " (Hit)" : "",
							lastSuffix,
							linkText);
					} else if (elem.isWalkForward) {
						sprintf_s(strbuf, "%df %s%s",
							elem.dashDuration,
							elem.dashDuration >= 10 ? "Walk" : "Microwalk",
							linkText);
					} else if (elem.isWalkBackward) {
						sprintf_s(strbuf, "%df %s%s",
							elem.dashDuration,
							elem.dashDuration >= 10 ? "Walk Back" : "Microwalk Back",
							linkText);
					} else {
						sprintf_s(strbuf, "%df %s%s",
							elem.dashDuration,
							elem.dashDuration >= 10 ? "Dash" : "Microdash",
							linkText);
					}
					
					if (elem.isProjectile) {
						if (transparentBackground) {
							outlinedText(ImGui::GetCursorPos(), strbuf, &LIGHT_BLUE_COLOR, nullptr, true);
						} else {
							textUnformattedColored(LIGHT_BLUE_COLOR, strbuf);
						}
					} else {
						if (transparentBackground) {
							outlinedText(ImGui::GetCursorPos(), strbuf, nullptr, nullptr, true);
						} else {
							ImGui::TextUnformatted(strbuf);
						}
					}
				}
				
				ImGui::EndTable();
			}
			
			float totalViewableArea = ImGui::GetWindowHeight() - ImGui::GetStyle().FramePadding.y * 2 - ImGui::GetFontSize();
			float totalContentSize = ImGui::GetCursorPosY();
			if (comboRecipeUpdatedOnThisFrame[i]) {
				comboRecipeUpdatedOnThisFrame[i] = false;
				// simply calling ImGui::GetScrollY(ImGui::GetScrollMaxY()) made it scroll to the penultimate line
				// probably because ImGui::GetScrollMaxY() returns the value from the ImGui::Begin call so it can be compared to ImGui::GetScrollY(),
				// also from that call
				if (totalContentSize > totalViewableArea) {
					ImGui::SetScrollY(totalContentSize - totalViewableArea);
				}
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
	
	
	framebarHorizontalScrollbarDrawDataCopy.resize(sizeof ImDrawListBackup);
	framebarWindowDrawDataCopy.resize(sizeof ImDrawListBackup);
	framebarTooltipDrawDataCopy.resize(sizeof ImDrawListBackup);
	new (framebarHorizontalScrollbarDrawDataCopy.data()) ImDrawListBackup();
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
		if (*currentKeyStr != '\0') {
			AddTooltip(currentKeyStr);
		}
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
		zerohspacing
		frameAdvantageText(frameAdvantage);
		ImGui::SameLine();
		ImGui::TextUnformatted(" (");
		ImGui::SameLine();
		frameAdvantageText(landingFrameAdvantage);
		ImGui::SameLine();
		ImGui::TextUnformatted(")");
		_zerohspacing
	} else if (frameAdvantageValid || landingFrameAdvantageValid) {
		int frameAdvantageLocal = frameAdvantageValid ? frameAdvantage : landingFrameAdvantage;
		frameAdvantageTextFormat(frameAdvantageLocal, strbuf, sizeof strbuf);
		if (rightAlign) RightAlign(ImGui::CalcTextSize(strbuf).x);
		frameAdvantageText(frameAdvantageLocal);
	}
}

// Runs on the main thread
int UI::frameAdvantageTextFormat(int frameAdv, char* buf, size_t bufSize) {
	if (frameAdv > 0) {
		return sprintf_s(buf, bufSize, "+%d", frameAdv);
	} else {
		return sprintf_s(buf, bufSize, "%d", frameAdv);
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
	AddTooltipWithHotkey(desc, descEnd, hotkey);
}

void UI::AddTooltipWithHotkey(const char* desc, const char* descEnd, std::vector<int>& hotkey) {
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

float getItemSpacing() {
	return ImGui::GetStyle().ItemSpacing.x;
}

bool UI::addImage(HMODULE hModule, WORD resourceId, std::unique_ptr<PngResource>& resource) {
	if (!resource) resource = std::make_unique<PngResource>();
	if (!loadPngResource(hModule, resourceId, *resource)) return false;
	texturePacker.addImage(*resource);
	return true;
}

void outlinedText(ImVec2 pos, const char* text, ImVec4* color, ImVec4* outlineColor, bool highQuality) {
	if (!color) color = &WHITE_COLOR;
	outlinedTextJustTheOutline(pos, text, outlineColor, highQuality);
	ImGui::SetCursorPos({ pos.x, pos.y });
	if (!color) {
		ImGui::TextUnformatted(text);
	} else {
		ImGui::PushStyleColor(ImGuiCol_Text, *color);
		ImGui::TextUnformatted(text);
    	ImGui::PopStyleColor();
	}
}

void outlinedTextJustTheOutline(ImVec2 pos, const char* text, ImVec4* outlineColor, bool highQuality) {
	if (!outlineColor) outlineColor = &BLACK_COLOR;
	
    ImGui::PushStyleColor(ImGuiCol_Text, *outlineColor);
    
	ImGui::SetCursorPos({ pos.x, pos.y - 1.F });
	ImGui::TextUnformatted(text);
	
	ImGui::SetCursorPos({ pos.x, pos.y + 1.F });
	ImGui::TextUnformatted(text);
	
	if (highQuality) {
		
		ImGui::SetCursorPos({ pos.x + 1.F, pos.y });
		ImGui::TextUnformatted(text);
		
		ImGui::SetCursorPos({ pos.x - 1.F, pos.y });
		ImGui::TextUnformatted(text);
		
	}
	
	ImGui::SetCursorPos({ pos.x - 1.F, pos.y - 1.F });
	ImGui::TextUnformatted(text);
	
	ImGui::SetCursorPos({ pos.x + 1.F, pos.y - 1.F });
	ImGui::TextUnformatted(text);
	
	ImGui::SetCursorPos({ pos.x - 1.F, pos.y + 1.F });
	ImGui::TextUnformatted(text);
	
	ImGui::SetCursorPos({ pos.x + 1.F, pos.y + 1.F });
	ImGui::TextUnformatted(text);
	
    ImGui::PopStyleColor();
}

void outlinedTextRaw(ImDrawList* drawList, ImVec2 pos, const char* text, ImVec4* color, ImVec4* outlineColor, bool highQuality) {
	if (!color) color = &WHITE_COLOR;
	if (!outlineColor) outlineColor = &BLACK_COLOR;
	
	ImU32 clr = ImGui::GetColorU32(*color);
	ImU32 outlineClr = ImGui::GetColorU32(*outlineColor);
	
	drawList->AddText({ pos.x, pos.y - 1.F }, outlineClr, text);
	drawList->AddText({ pos.x, pos.y + 1.F }, outlineClr, text);
	if (highQuality) {
		drawList->AddText({ pos.x - 1.F, pos.y }, outlineClr, text);
		drawList->AddText({ pos.x + 1.F, pos.y }, outlineClr, text);
	}
	drawList->AddText({ pos.x - 1.F, pos.y - 1.F }, outlineClr, text);
	drawList->AddText({ pos.x + 1.F, pos.y - 1.F }, outlineClr, text);
	drawList->AddText({ pos.x - 1.F, pos.y + 1.F }, outlineClr, text);
	drawList->AddText({ pos.x + 1.F, pos.y + 1.F }, outlineClr, text);
	drawList->AddText(pos, clr, text);
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

void printInputs(char*& buf, size_t& bufSize, InputName** motions, int motionCount, InputName** buttons, int buttonsCount) {
	char* bufOrig = buf;
	int result;
	bool needSpace = false;
	for (int i = 0; i < motionCount; ++i) {
		InputName* desc = motions[i];
		if (strstr(desc->name, "don't") == nullptr && desc->type == InputNameType::MULTIWORD_MOTION) {
			result = sprintf_s(buf, bufSize, "%s%s", needSpace ? ", " : "", desc->name);
			advanceBuf
			needSpace = true;
		}
	}
	bool needPlus = false;
	for (int i = 0; i < motionCount; ++i) {
		InputName* desc = motions[i];
		if (strstr(desc->name, "don't") == nullptr && desc->type == InputNameType::MOTION) {
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
		InputName* desc = buttons[i];
		if (strstr(desc->name, "don't") == nullptr) {
			result = sprintf_s(buf, bufSize, "%s%s",
				needPlus
					? "+"
					: desc->type == InputNameType::MULTIWORD_BUTTON && bufOrig != buf
						? ", "
						: needSpace
							? " "
							: ""
				, desc->name);
			advanceBuf
			needSpace = false;
			needPlus = true;
		}
	}
	bool madeOne = false;
	for (int i = 0; i < motionCount; ++i) {
		InputName* desc = motions[i];
		if (strstr(desc->name, "don't") != nullptr && desc->type == InputNameType::MULTIWORD_MOTION) {
			if (buf != bufOrig) { needPlus = false; needSpace = true; }
			result = sprintf_s(buf, bufSize, "%s%s", needSpace ? ", " : "", desc->name);
			advanceBuf
			needSpace = true;
			madeOne = true;
		}
	}
	for (int i = 0; i < motionCount; ++i) {
		InputName* desc = motions[i];
		if (strstr(desc->name, "don't") != nullptr && desc->type == InputNameType::MOTION) {
			if (buf != bufOrig) { needPlus = false; needSpace = true; }
			result = sprintf_s(buf, bufSize, "%s%s",
				needSpace
					? ", "
					: needPlus
						? "+" : ""
				, desc->name);
			advanceBuf
			needSpace = false;
			needPlus = true;
			madeOne = true;
		}
	}
	if (madeOne) {
		needPlus = false;
	}
	for (int i = 0; i < buttonsCount; ++i) {
		InputName* desc = buttons[i];
		if (strstr(desc->name, "don't") != nullptr) {
			if (buf != bufOrig) { needPlus = false; needSpace = true; }
			result = sprintf_s(buf, bufSize, "%s%s",
				needPlus
					? "+"
					: (desc->type == InputNameType::MULTIWORD_BUTTON || needSpace) && bufOrig != buf
						? ", " : ""
				, desc->name);
			advanceBuf
			needSpace = false;
			needPlus = true;
		}
	}
}

int printInputs(char* buf, size_t bufSize, const InputType* inputs) {
	if (!bufSize) return 0;
	*buf = '\0';
	char* origBuf = buf;
	InputName* motions[16] { nullptr };
	int motionCount = 0;
	InputName* buttons[16] { nullptr };
	int buttonsCount = 0;
	int result;
	char* lastOrPrint = nullptr;
	for (int i = 0; i < 16; ++i) {
		InputType inputType = inputs[i];
		if (inputType == INPUT_END) {
			break;
		}
		if (inputType == INPUT_BOOLEAN_OR) {
			lastOrPrint = buf;
			printInputs(buf, bufSize, motions, motionCount, buttons, buttonsCount);
			if (bufSize > 2) {
				memmove(lastOrPrint + 1, lastOrPrint, buf - lastOrPrint);
				*lastOrPrint = '{';
				*(buf + 1) = '}';
				buf += 2;
				bufSize -= 2;
			}
			result = sprintf_s(buf, bufSize, " or ");
			advanceBuf
			motionCount = 0;
			buttonsCount = 0;
			continue;
		}
		InputName& info = inputNames[inputType];
		if (info.type == InputNameType::MOTION || info.type == InputNameType::MULTIWORD_MOTION) {
			motions[motionCount++] = &info;
		}
		if (info.type == InputNameType::BUTTON || info.type == InputNameType::MULTIWORD_BUTTON) {
			buttons[buttonsCount++] = &info;
		}
	}
	char* oldBuf = buf;
	printInputs(buf, bufSize, motions, motionCount, buttons, buttonsCount);
	if (bufSize > 2 && lastOrPrint) {
		memmove(oldBuf + 1, oldBuf, buf - oldBuf);
		*oldBuf = '{';
		*(buf + 1) = '}';
		buf += 2;
		bufSize -= 2;
	}
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
		} else if (
			!(
				mode == GAME_MODE_NETWORK
				&& game.getPlayerSide() != 2  // 2 means observer
			) && !(
				mode == GAME_MODE_VERSUS
				&& game.bothPlayersHuman()
			)
		) {
			return settings.showFramebarInOtherModes;
		} else {
			return false;
		}
	} else {
		return false;
	}
}

int printCancels(const FixedArrayOfGatlingOrWhiffCancelInfos<30>& cancels, float maxY) {
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
	bool useSlang = settings.useSlangNames;
	for (size_t i = 0; i < cancels.size(); ++i) {
		const GatlingOrWhiffCancelInfo& cancel = cancels[i];
		
		if (i != cancels.size() - 1 && ImGui::GetCursorPosY() >= maxY) {
			ImGui::Text("%d) Skipping %d items...", counter++, cancels.size() - i);
			break;
		}
		
		char* buf = strbuf;
		size_t bufSize = sizeof strbuf;
		int result;
		result = sprintf_s(buf, bufSize, "%s", useSlang && cancel.slangName ? cancel.slangName : cancel.name);
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
		result = sprintf_s(buf, bufSize, " (%df buffer)", cancel.bufferTime);
		advanceBuf
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
	yellowText("Box colors and what they mean:");
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
	
	// these notes are also repeated in README.md in the solution's root directory
	textUnformattedColored(COLOR_INTERACTION_IMGUI, "White: ");
	ImGui::SameLine();
	ImGui::TextUnformatted("Interaction boxes/circles");
	ImGui::TextUnformatted(
		"Boxes or circles like this are displayed when a move is checking ranges."
		" They may be checking distance to a player's origin point or to their 'center',"
		" depending on the type of move or projectile. All types of displayed interactions will be listed here down below.");
	
	yellowText("Ky Stun Edge, Charged Stun Edge and Sacred Edge:");
	ImGui::TextUnformatted("The box shows the area in which Ciel's origin point must be in order for the projectile to become Fortified.");
	
	yellowText("May Beach Ball:");
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
	
	yellowText("May Dolphin:");
	static std::string mayDolphin;
	if (mayDolphin.empty()) {
		mayDolphin = settings.convertToUiDescription("The circle shows the range in which May's behind the body point must be"
			" in order to ride the Dolphin. The point behind May's body depends on May's facing, and not the Dolphin's."
			" Additionally, a line connecting May's behind the body point and the origin point of the Dolphin is shown."
			" It serves no purpose other than to remind the user that the distance check is performed against May's"
			" behind the body point, and not her origin point.\n"
			"The display of all this can be disabled with \"dontShowMayInteractionChecks\".");
	}
	ImGui::TextUnformatted(mayDolphin.c_str());
	
	yellowText("Millia Pin:");
	ImGui::TextUnformatted("The infinite vertical box shows the range in which Millia's origin point must be in order"
		" for the Pin to be picked up. Millia must be in either of the following animations:");
  	ImGui::TextUnformatted("*) Stand to Crouch;");
  	ImGui::TextUnformatted("*) Crouch;");
  	ImGui::TextUnformatted("*) Crouch Turn (as of Rev2);");
  	ImGui::TextUnformatted("*) Roll (as of Rev2);");
  	ImGui::TextUnformatted("*) Doubleroll (as of Rev2).");
  	
	yellowText("Millia Bad Moon Buff Height:");
	static std::string milliaBadMoonHeightBuff;
	if (milliaBadMoonHeightBuff.empty()) {
		milliaBadMoonHeightBuff = settings.convertToUiDescription(
			"The infinite horizontal line shows the height above which Millia's origin point must be in order for Bad Moon"
			" to get some attack powerup. However, Bad Moon is limited by the maximum number of hits it can deal,"
			" which increases with height (the buff height is 500000, next buffs are at 650000, 800000, 950000, 1100000, 1250000, 1400000)."
			" The display of the height line must be enabled with \"showMilliaBadMoonBuffHeight\" setting.");
	}
	ImGui::TextUnformatted(milliaBadMoonHeightBuff.c_str());
	
	yellowText("Faust 5D:");
	if (faust5D.empty()) {
		faust5D = settings.convertToUiDescription(faust5DHelp);
	}
	ImGui::TextUnformatted(faust5D.c_str());
	
	yellowText("Faust Food Items:");
	ImGui::TextUnformatted("The white box shows where a player's origin point must be in order to pick up the food item.");
	
	yellowText("Faust Helium:");
	ImGui::TextUnformatted("The circle shows the range in which opponent's or Faust's origin point must be in order to pick up the Helium.");
	
	yellowText("Bedman Task C Height Buff:");
	static std::string bedmanTaskCHeightBuff;
	if (bedmanTaskCHeightBuff.empty()) {
		bedmanTaskCHeightBuff = settings.convertToUiDescription(
			"The infinite horizontal line shows the height above which Bedman's origin point must be in order for Task C"
			" to gain a powerup that makes it slam the ground harder. That height is 700000."
			" The display of the height line must be enabled with \"showBedmanTaskCHeightBuffY\" setting.");
	}
	ImGui::TextUnformatted(bedmanTaskCHeightBuff.c_str());
	
	yellowText("Ramlethal Sword Re-Deploy No-Teleport Distance:");
	static std::string ramlethalSword;
	if (ramlethalSword.empty()) {
		ramlethalSword = settings.convertToUiDescription(
			"The infinite vertical boxes around the swords show the distance in which the opponent's"
			" origin point must be in order for the sword to not spend extra time teleporting to the"
			" opponent when you re-deploy it.");
	}
	ImGui::TextUnformatted(ramlethalSword.c_str());
	
	yellowText("Sin Hawk Baker Red Hit Box:");
	static std::string sinHawkBaker;
	if (sinHawkBaker.empty()) {
		sinHawkBaker = settings.convertToUiDescription(
			"The line connecting Sin to the wall that he's facing has a marking on it denoting the maximum"
			" distance he must be from the wall, as one of two possible ways to get a red hit."
			" The distance marker checks for Sin's origin point, not his pushbox. If Sin is too far from"
			" the wall that he is facing, the other way to get a red hit is for the opponent's origin point"
			" to be close to Sin, inside the infinite white vertical box drawn around him. If Sin is neither"
			" close to the wall he's facing nor close enough to the opponent, the hit is blue.");
	}
	ImGui::TextUnformatted(sinHawkBaker.c_str());
	
	yellowText("Elphelt Max Charge Shotgun:");
	static std::string elpheltShotgun;
	if (elpheltShotgun.empty()) {
		elpheltShotgun = settings.convertToUiDescription(
			"If the opponent's origin point is within the infinite white vertical box around Elphelt,"
			" then the shotgun shot gets the maximum possible powerup.");
	}
	ImGui::TextUnformatted(elpheltShotgun.c_str());
	
	yellowText("Johnny Bacchus Sigh:");
	ImGui::TextUnformatted("If the opponent's center of body (marked with an extra small point)"
		" is within the circle, Bacchus Sigh will connect on the next frame.");
	
	yellowText("Jack-O' Ghost Pickup Range:");
	static std::string jackoGhostPickup;
	if (jackoGhostPickup.empty()) {
		jackoGhostPickup = settings.convertToUiDescription("The displayed vertical box around each Ghost"
			" (house) shows the range in which Jack-O's origin point must be in order to pick up the"
			" Ghost or gain Tension from it.\n"
			"This is only displayed when the \"showJackoGhostPickupRange\" setting is on.");
	}
	ImGui::TextUnformatted(jackoGhostPickup.c_str());
	
	yellowText("Jack-O' Aegis Field Range:");
	static std::string jackoAegisField;
	if (jackoAegisField.empty()) {
		jackoAegisField = settings.convertToUiDescription("The white circle shows the range where the Ghosts'"
			" or Servants' origin points must be in order for them to receive protection of the Field.\n"
			"This is only displayed when the \"showJackoAegisFieldRange\" setting is on.");
	}
	ImGui::TextUnformatted(jackoAegisField.c_str());
	
	yellowText("Jack-O' Servant Attack Range:");
	static std::string jackoServantAttack;
	if (jackoServantAttack.empty()) {
		jackoServantAttack = settings.convertToUiDescription("The white box around each Servant shows the area where"
			" the opponent's player's origin point must be in order for the Servant to initiate an attack.\n"
			"This is only displayed when the \"showJackoServantAttackRange\" setting is on.");
	}
	ImGui::TextUnformatted(jackoServantAttack.c_str());
	
	yellowText("Jam Bao Saishinshou:");
	ImGui::TextUnformatted("The white box shows the vertical range where the opponent's origin point be upon"
		" the hit connecting in order to be vacuumed by the attack.");
	
	yellowText("Haehyun Enlightened 3000 Palm Strike:");
	ImGui::TextUnformatted("The giant circle shows where the opponent's center of body point has to be in order to be vacuumed."
		" The center of body points of both players are shown and a line connecting them is shown as a guide for what exact"
		" distance is being measured by the game.");
	
	yellowText("Baiken Metsudo Kushodo:");
	ImGui::TextUnformatted("The white box shows the area where opponent's origin point must be at the moment the move is performed,"
		" in order for Baiken to shoot out ropes and travel to the opposite wall.");
	
	yellowText("Answer Scroll:");
	ImGui::TextUnformatted("The white box around the scroll shows the area where Answer's origin point must be in order to cling to the scroll."
		" After entering the area, you are able to cling only on the frame after the next one. So Enter range -> Wait -> Cling.");
	
	yellowText("Answer Card:");
	ImGui::TextUnformatted("The infinite vertical white box around the card shows the area where the opponent's origin point must be"
		" in order for the Clone to track to their position.");
	
	yellowText("Leo bt.D successful parry:");
	ImGui::TextUnformatted("The white box around Leo shows where the origin point of the opponent must be in order to get vaccuumed.");
	
	yellowText("Baiken Tsurane Sanzu-watashi:");
	ImGui::TextUnformatted("The white box in front of Baiken shows where the origin point of the opponent must be at the moment"
			" of the second hit in order to trigger the secondary super cinematic. An extra condition for the cinematic to be"
			" triggered is also that the first hit of the super connected, but the distance on it is not checked.");
	
	ImGui::Separator();
	
	yellowText("Outlines lie within their boxes/on the edge");
	ImGui::TextUnformatted("If a box's outline is thick, it lies within that box's bounds,"
		" meaning that two boxes intersect if either their fills or outlines touch or both. This"
		" is relevant for throwboxes too.\n"
		"If a box's outline is thin, like that of a pushbox or a clash-only hitbox for example,"
		" then that outline lies on the edge of the box. For Jack O's and Bedman's summons"
		" the hurtbox's outline may be thin to create less clutter on the screen.");
	ImGui::Separator();
	
	yellowText("General notes about throw boxes");
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
	ImGui::TextUnformatted(searchTooltip("A half-filled active frame means an attack's startup or active frame which first begins during"
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
		{ MARKER_TYPE_OTG, true, "OTG state - getting hit in this state reduces hitstun, damage and stun." },
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
	
	float cursorY = ImGui::GetCursorPosY();
	ImGui::SetCursorPosY(cursorY + (ImGui::GetTextLineHeightWithSpacing() - powerupHeightOriginal) * 0.5F);
	ImGui::Image((ImTextureID)TEXID_FRAMES,
		{ powerupWidthOriginal, powerupHeightOriginal },
		{
			powerupFrame->uStart,
			powerupFrame->vStart
		},
		ImVec2{
			powerupFrame->uEnd,
			powerupFrame->vEnd
		}
	);
	ImGui::SameLine();
	ImGui::SetCursorPosY(cursorY);
	ImGui::TextUnformatted(searchTooltip("The move reached some kind of powerup on this frame. For example, for May 6P it means that,"
		" starting from this frame, it deals more stun and has more pushback, while for Venom QV it means the ball has become"
		" bigger, and so on.\n"));
		
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
			"Note: due to rollback, framebar cannot function properly in online play and is disabled there (but it works when observing a match).\n"
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
			" The framebar window has an invisible border which can be resized using the mouse. You can see the border when you drag the framebar window."
			" You can resize the framebar so that it is large enough to fit all its sub-framebars in it without having to scroll.\n"
			"\n"
			"Framebar can be dragged by clicking anywhere on it with the mouse, holding the mouse button and moving the mouse.\n"
			"\n"
			"The framebar can actually hold more frames than displayed. When that happens, a horizontal scrollbar appear on top"
			" of the framebar. Scrolling it to the right by either dragging it with the mouse or using Shift + Mouse Wheel,"
			" has the framebar travel into the past to remember one of its older states and revert to that."
			" The framebar can only be horizontally scrolled when both players are idle or the game is paused or the match is already over."
			" Horizontal scrollbar can be disabled by changing \"framebarStoredFramesCount\" and \"framebarDisplayedFramesCount\" settings"
			" so that they are equal.\n"
			"\n"
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
			"Pressing the left mouse button over a frame on the framebar, holding it and then dragging selects a range of frames, and a text is displayed"
			" telling the count of selected frames. Even though multiple rows at once are being selected, the displayed count includes"
			" only the horizontal spaces shared among all rows.\n"
			"\n"
			"Startup/Active/Recovery/Total/Advantage text on top of and below the players' framebars can be disabled using"
			"\"showP1FramedataInFramebar\" and \"showP2FramedataInFramebar\" settings.\n"
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
				ImGui::PushStyleVarY(ImGuiStyleVar_ItemSpacing, 0.F);
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + currentTextPos.y - yStart - 1.F);
				ImGui::InvisibleButton(gianttextid, { 1.F, 1.F });
				ImGui::PopStyleVar();
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
		ImGui::PushStyleVarY(ImGuiStyleVar_ItemSpacing, 0.F);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + currentTextPos.y - yStart - 1.F);
		ImGui::InvisibleButton(gianttextid, { 1.F, 1.F });
		ImGui::PopStyleVar();
	}
	ImGui::PopStyleVar();
}

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
	CharacterType charType = endScene.players[playerIndex].charType;
	
	printAllCancels(frame.cancels,
			frame.enableSpecialCancel,
			frame.enableJumpCancel,
			frame.enableSpecials,
			frame.hitAlreadyHappened,
			frame.airborne,
			true,
			true);
	
	bool showHorizCharge = false;
	int horizChargeMax = 0;
	bool showVertCharge = false;
	int vertChargeMax = 0;
	
	if (charType == CHARACTER_TYPE_LEO || charType == CHARACTER_TYPE_VENOM) {
		showHorizCharge = true;
		horizChargeMax = 40;
		showVertCharge = true;
		vertChargeMax = 40;
	} else if (charType == CHARACTER_TYPE_MAY) {
		showHorizCharge = true;
		horizChargeMax = 30;
		showVertCharge = true;
		vertChargeMax = 30;
	} else if (charType == CHARACTER_TYPE_POTEMKIN || charType == CHARACTER_TYPE_AXL) {
		showHorizCharge = true;
		horizChargeMax = 30;
	}
	
	if (showHorizCharge || showVertCharge) {
		ImGui::Separator();
		zerohspacing
		if (showHorizCharge) {
			printChargeInFrameTooltip("Charge (left): ", frame.chargeLeft, horizChargeMax, frame.chargeLeftLast);
			printChargeInFrameTooltip("Charge (right): ", frame.chargeRight, horizChargeMax, frame.chargeRightLast);
		}
		if (showVertCharge) {
			printChargeInFrameTooltip("Charge (down): ", frame.chargeDown, vertChargeMax, frame.chargeDownLast);
		}
		_zerohspacing
	}
	
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
			int result = sprintf_s(strbuf, "(%d) <no inputs>", allInputsAreJustOneEmptyRow_frameCount);
			if (result != -1) {
				if (playerIndex == 1) {
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(strbuf, strbuf + result).x);
				}
				ImGui::TextUnformatted(strbuf);
			}
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
	if (*strbuf != '\0' || frame.OTGInGeneral || frame.counterhit) {
		ImGui::Separator();
		if (*strbuf != '\0') {
			yellowText("Invul: ");
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
		if (frame.OTGInGeneral) {
			ImGui::TextUnformatted("OTG state.");
		}
		if (frame.counterhit) {
			ImGui::TextUnformatted("Counterhit state.");
		}
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
		
		const char* poisonTitle;
		if (frame.poisonIsBacchusSigh) {
			poisonTitle = "Bacchus Debuff On You: ";
		} else if (frame.poisonIsRavenSlow) {
			poisonTitle = "Slow Timer: ";
		} else {
			poisonTitle = "Poison Duration: ";
		}
		yellowText(poisonTitle);
		ImGui::SameLine();
		sprintf_s(strbuf, "%d/%d", frame.poisonDuration, frame.poisonMax);
		ImGui::TextUnformatted(strbuf);
		
	}
	CharacterType charType = endScene.players[playerIndex].charType;
	if (frame.powerup) {
		ImGui::Separator();
		if (frame.powerupExplanation && *frame.powerupExplanation != '\0') {
			const char* newlinePos = nullptr;
			static const char titleOverride[] = "//Title override: ";
			if (strncmp(frame.powerupExplanation, titleOverride, sizeof titleOverride - 1) == 0) {
				newlinePos = strchr(frame.powerupExplanation, '\n');
			}
			if (newlinePos) {
				if (newlinePos != frame.powerupExplanation + sizeof titleOverride - 1) {
					yellowText(frame.powerupExplanation + sizeof titleOverride - 1, newlinePos);
				}
				ImGui::TextUnformatted(newlinePos + 1);
			} else {
				yellowText("Reached powerup:");
				ImGui::TextUnformatted(frame.powerupExplanation);
			}
		} else {
			ImGui::TextUnformatted("The move reached some kind of powerup on this frame.");
		}
	}
	if (frame.airthrowDisabled || frame.running || frame.cantBackdash || frame.cantAirdash) {
		ImGui::Separator();
		if (frame.airthrowDisabled) {
			ImGui::TextUnformatted("Airthrow disabled.");
			ImGui::PushStyleColor(ImGuiCol_Text, SLIGHTLY_GRAY);
			if (endScene.players[playerIndex].charType == CHARACTER_TYPE_BEDMAN) {
				ImGui::TextUnformatted("Hover disables airthrow for the remainder of being in the air.");
			} else {
				ImGui::TextUnformatted("Airdashing disables airthrow for the remainder of being in the air.");
			}
			ImGui::PopStyleColor();
		}
		if (frame.running) {
			ImGui::TextUnformatted("Throw disabled.");
			ImGui::PushStyleColor(ImGuiCol_Text, SLIGHTLY_GRAY);
			ImGui::TextUnformatted("Cannot throw while running.");
			ImGui::PopStyleColor();
		}
		if (frame.cantBackdash) {
			ImGui::TextUnformatted("Backdash disabled.");
			ImGui::PushStyleColor(ImGuiCol_Text, SLIGHTLY_GRAY);
			ImGui::TextUnformatted("Can't backdash again for 4f after previous backdash is over.");
			ImGui::PopStyleColor();
		}
		if (frame.cantAirdash) {
			ImGui::TextUnformatted("Can't airdash due to minimum height requirement.");
		}
	}
	if (charType == CHARACTER_TYPE_SOL) {
		if (frame.u.solInfo.currentDI || frame.u.solInfo.gunflameDisappearsOnHit) {
			ImGui::Separator();
			if (frame.u.solInfo.currentDI) {
				yellowText("Dragon Install: ");
				ImGui::SameLine();
				if (frame.u.solInfo.currentDI == USHRT_MAX) {
					ImGui::Text("overdue/%d", frame.u.solInfo.maxDI);
				} else {
					ImGui::Text("%d/%d", frame.u.solInfo.currentDI, frame.u.solInfo.maxDI);
				}
				
				ImGui::PushStyleColor(ImGuiCol_Text, SLIGHTLY_GRAY);
				ImGui::TextUnformatted("This value doesn't decrease in hitstop and superfreeze and decreases"
					" at half the speed when slowed down by opponent's RC.");
				ImGui::PopStyleColor();
				
			}
			if (frame.u.solInfo.gunflameDisappearsOnHit) {
				ImGui::TextUnformatted("The Gunflame will disappear if Sol is hit (non-blocked hit) on this frame.");
			}
		}
	} else if (charType == CHARACTER_TYPE_KY) {
		if (frame.u.kyInfo.stunEdgeWillDisappearOnHit || frame.u.kyInfo.hasChargedStunEdge || frame.u.kyInfo.hasSPChargedStunEdge) {
			ImGui::Separator();
			if (frame.u.kyInfo.stunEdgeWillDisappearOnHit) {
				ImGui::TextUnformatted("The Stun Edge will disappear if Ky is hit (non-blocked hit) on this frame.");
			}
			if (frame.u.kyInfo.hasChargedStunEdge) {
				ImGui::TextUnformatted("The Charged Stun Edge will disappear if Ky is hit (non-blocked hit) at any time.");
			}
			if (frame.u.kyInfo.hasSPChargedStunEdge) {
				ImGui::TextUnformatted("The Fortified Charged Stun Edge will disappear if Ky is hit (non-blocked hit) at any time.");
			}
		}
	} else if (charType == CHARACTER_TYPE_MILLIA) {
		bool insertedSeparator = false;
		if (frame.u.milliaInfo.canProgramSecretGarden || frame.u.milliaInfo.SGInputs) {
			ImGui::Separator();
			insertedSeparator = true;
			ImGui::Text("%sInputs %d/%d",
				frame.u.milliaInfo.canProgramSecretGarden ? "Can program Secret Garden. " : "Secret Garden ",
				frame.u.milliaInfo.SGInputs,
				frame.u.milliaInfo.SGInputsMax);
		}
		if (frame.u.milliaInfo.chromingRose) {
			if (!insertedSeparator) {
				ImGui::Separator();
			}
			yellowText("Chroming Rose: ");
			ImGui::SameLine();
			ImGui::Text("%d/%d", frame.u.milliaInfo.chromingRose, frame.u.milliaInfo.chromingRoseMax);
			
			ImGui::PushStyleColor(ImGuiCol_Text, SLIGHTLY_GRAY);
			ImGui::TextUnformatted("This value doesn't decrease in hitstop and superfreeze and decreases"
				" at half the speed when slowed down by opponent's RC.");
			ImGui::PopStyleColor();
			
		}
	} else if (charType == CHARACTER_TYPE_CHIPP) {
		if (frame.u.chippInfo.invis || frame.u.chippInfo.wallTime) {
			ImGui::Separator();
			if (frame.u.chippInfo.invis) {
				printChippInvisibility(frame.u.chippInfo.invis, endScene.players[playerIndex].maxDI);
			}
			if (frame.u.chippInfo.wallTime) {
				yellowText("Wall time: ");
				ImGui::SameLine();
				if (frame.u.chippInfo.wallTime == USHRT_MAX) {
					ImGui::TextUnformatted("0/120");
				} else {
					ImGui::Text("%d/120", frame.u.chippInfo.wallTime);
				}
				ImGui::PushStyleColor(ImGuiCol_Text, SLIGHTLY_GRAY);
				ImGui::PushTextWrapPos(0.F);
				ImGui::TextUnformatted("This value increases slower when opponent slows you down with RC.");
				ImGui::PopTextWrapPos();
				ImGui::PopStyleColor();
			}
		}
	} else if (charType == CHARACTER_TYPE_ZATO) {
		ImGui::Separator();
		yellowText("Eddie Gauge: ");
		ImGui::SameLine();
		ImGui::Text("%d/6000", frame.u.currentTotalInfo.current);
	} else if (charType == CHARACTER_TYPE_FAUST) {
		if (frame.superArmorActiveInGeneral_IsFull && strcmp(frame.animName, "5D") == 0) {
			ImGui::Separator();
			ImGui::TextUnformatted("If Faust gets hit by a reflectable projectile on this frame,"
				" the reflection will be a homerun.");
		}
	} else if (charType == CHARACTER_TYPE_SLAYER) {
		if (frame.u.currentTotalInfo.current) {
			ImGui::Separator();
			yellowText("Bloodsucking Universe Buff: ");
			ImGui::SameLine();
			ImGui::Text("%d/%d", frame.u.currentTotalInfo.current, frame.u.currentTotalInfo.max);
		}
	} else if (charType == CHARACTER_TYPE_INO) {
		if (frame.u.inoInfo.airdashTimer) {
			ImGui::Separator();
			yellowText("Airdash active timer: ");
			ImGui::SameLine();
			ImGui::Text("%d", frame.u.inoInfo.airdashTimer);
			ImGui::PushStyleColor(ImGuiCol_Text, SLIGHTLY_GRAY);
			ImGui::TextUnformatted("While airdash is active, speed Y is continuously set to 0.");
			ImGui::PopStyleColor();
		}
	} else if (charType == CHARACTER_TYPE_BEDMAN) {
		if (frame.u.bedmanInfo.sealA || frame.u.bedmanInfo.sealB || frame.u.bedmanInfo.sealC || frame.u.bedmanInfo.sealD) {
			ImGui::Separator();
			struct SealInfo {
				const char* txt;
				const unsigned short& timer;
				const unsigned short& timerMax;
			};
			SealInfo seals[4] {
				{ "Task A Seal: ", frame.u.bedmanInfo.sealA, frame.u.bedmanInfo.sealAMax },
				{ "Task A' Seal: ", frame.u.bedmanInfo.sealB, frame.u.bedmanInfo.sealBMax },
				{ "Task B Seal: ", frame.u.bedmanInfo.sealC, frame.u.bedmanInfo.sealCMax },
				{ "Task C Seal: ", frame.u.bedmanInfo.sealD, frame.u.bedmanInfo.sealDMax }
			};
			for (int i = 0; i < 4; ++i) {
				SealInfo& seal = seals[i];
				if (seal.timer) {
					yellowText(seal.txt);
					ImGui::SameLine();
					sprintf_s(strbuf, "%d/%d", seal.timer, seal.timerMax);
					ImGui::TextUnformatted(strbuf);
				}
			}
		}
	} else if (charType == CHARACTER_TYPE_RAMLETHAL) {
		if (frame.u.ramlethalInfo.sSwordTime || frame.u.ramlethalInfo.hSwordTime) {
			ImGui::Separator();
			struct BitInfo {
				const char* title;
				int time;
				int timeMax;
			};
			BitInfo bitInfos[2] {
				{
					"S Sword: ",
					frame.u.ramlethalInfo.sSwordTime,
					frame.u.ramlethalInfo.sSwordTimeMax
				},
				{
					"H Sword: ",
					frame.u.ramlethalInfo.hSwordTime,
					frame.u.ramlethalInfo.hSwordTimeMax
				}
			};
			for (int i = 0; i < 2; ++i) {
				BitInfo& bitInfo = bitInfos[i];
				if (bitInfo.time) {
					yellowText(bitInfo.title);
					ImGui::SameLine();
					if (bitInfo.timeMax) {
						sprintf_s(strbuf, "%d/%d", bitInfo.time, bitInfo.timeMax);
					} else {
						sprintf_s(strbuf, "until landing + %d", bitInfo.time);
					}
					ImGui::TextUnformatted(strbuf);
				}
			}
		}
	} else if (charType == CHARACTER_TYPE_ELPHELT) {
		if (frame.u.elpheltInfo.grenadeTimer || frame.u.elpheltInfo.grenadeDisabledTimer) {
			ImGui::Separator();
		}
		if (frame.u.elpheltInfo.grenadeTimer) {
			yellowText("Berry Timer: ");
			ImGui::SameLine();
			sprintf_s(strbuf, "%d/180", frame.u.elpheltInfo.grenadeTimer);
			ImGui::TextUnformatted(strbuf);
		}
		if (frame.u.elpheltInfo.grenadeDisabledTimer) {
			yellowText("Can pull Berry in: ");
			ImGui::SameLine();
			char* buf = strbuf;
			size_t bufSize = sizeof strbuf;
			int result;
			if (frame.u.elpheltInfo.grenadeDisabledTimer == 255) {
				result = sprintf_s(buf, bufSize, "%s", "???");
			} else {
				result = sprintf_s(buf, bufSize, "%d", frame.u.elpheltInfo.grenadeDisabledTimer);
			}
			advanceBuf
			if (frame.u.elpheltInfo.grenadeDisabledTimerMax == 255) {
				sprintf_s(buf, bufSize, "/%s", "???");
			} else {
				sprintf_s(buf, bufSize, "/%d", frame.u.elpheltInfo.grenadeDisabledTimerMax);
			}
			ImGui::TextUnformatted(strbuf);
		}
	} else if (charType == CHARACTER_TYPE_JOHNNY) {
		if (frame.u.johnnyInfo.mistTimer || frame.u.johnnyInfo.mistTimerMax) {
			ImGui::Separator();
		}
		if (frame.u.johnnyInfo.mistTimer) {
			yellowText("Bacchus Sigh Projectile Timer: ");
			sprintf_s(strbuf, "%d/%d", frame.u.johnnyInfo.mistTimer, frame.u.johnnyInfo.mistTimerMax);
			ImGui::TextUnformatted(strbuf);
		}
		
		if (frame.u.johnnyInfo.mistKuttsukuTimer) {
			yellowText("Bacchus Sigh Debuff On Opponent: ");
			sprintf_s(strbuf, "%d/%d", frame.u.johnnyInfo.mistKuttsukuTimer, frame.u.johnnyInfo.mistKuttsukuTimerMax);
			ImGui::TextUnformatted(strbuf);
		}
	} else if (charType == CHARACTER_TYPE_RAVEN) {
		if (frame.u.ravenInfo.slowTime) {
			ImGui::Separator();
			yellowText("Slow Timer On Opponent: ");
			ImGui::SameLine();
			sprintf_s(strbuf, "%d/%d", frame.u.ravenInfo.slowTime, frame.u.ravenInfo.slowTimeMax);
			ImGui::TextUnformatted(strbuf);
		}
		
	}
	
	if (frame.suddenlyTeleported) {
		ImGui::Separator();
		ImGui::TextUnformatted("Suddenly teleported.");
	}
	if (frame.crossupProtectionIsOdd || frame.crossupProtectionIsAbove1 || frame.crossedUp) {
		ImGui::Separator();
		if (frame.crossedUp) {
			ImGui::TextUnformatted("Crossed up");
		} else {
			yellowText("Crossup protection: ");
			ImGui::SameLine();
			sprintf_s(strbuf, "%d/3", frame.crossupProtectionIsAbove1 + frame.crossupProtectionIsAbove1 + frame.crossupProtectionIsOdd);
			ImGui::TextUnformatted(strbuf);
		}
	}
	if (frame.dustGatlingTimer) {
		ImGui::Separator();
		yellowText("Dust Gatling Timer: ");
		ImGui::SameLine();
		sprintf_s(strbuf, "%d/%d", frame.dustGatlingTimer, frame.dustGatlingTimerMax);
		ImGui::TextUnformatted(strbuf);
		ImGui::PushStyleColor(ImGuiCol_Text, SLIGHTLY_GRAY);
		ImGui::TextUnformatted("If a normal attack connects while this timer is still counting, all player's non-followup non-super non-IK moves"
			" get enabled as gatlings from it.");
		ImGui::PopStyleColor();
		
	}
}

/// <summary>
/// Draws backgrounds of frames - the base frame graphics. Also registers mouse hovering over a frame and draws the frame tooltip window.
/// </summary>
/// <typeparam name="FramebarT">Possible values: PlayerFramebar, Framebar</typeparam>
/// <typeparam name="FrameT">Possible values: PlayerFrame, Frame</typeparam>
/// <param name="framebar">Either main or hitstop framebar</param>
/// <param name="preppedDims">X positions and widths of each on-screen frame</param>
/// <param name="tintDarker">Color to be used as the tint for darkened, older frames that are behind drawFramebars_framebarPosition</param>
/// <param name="playerIndex">Index of the player. 0 or 1. For projectiles it is -1</param>
/// <param name="skippedFrames">Contains _countof(Framebar::frames) elements. For each frame, describes whether hitstop/superfreeze/etc was skipped and how many frames were skipped</param>
/// <param name="correspondingPlayersFramebar">If this is a projectile framebar, then framebar of the player that corresponds to or owns this projectile is given</param>
/// <param name="owningPlayerCharType">If this is a projectile, then this is the character type of the corresponding or owner player</param>
template<typename FramebarT, typename FrameT>
inline void drawFramebar(const FramebarT& framebar, UI::FrameDims* preppedDims, ImU32 tintDarker, int playerIndex,
			const std::vector<SkippedFramesInfo>& skippedFrames, const PlayerFramebar& correspondingPlayersFramebar, CharacterType owningPlayerCharType) {
	const bool useSlang = settings.useSlangNames;
	const int framesCount = settings.framebarDisplayedFramesCount;
	
	int internalINext = iterateVisualFramesFrom0_getInitialInternalInd();
	int internalI;
	
	for (int visualI = 0; visualI < drawFramebars_framesCount; ++visualI) {
		
		internalI = internalINext;
		incrementInternalInd(internalINext);
		
		const FrameT& frame = framebar[internalI];
		const Frame& projectileFrame = (const Frame&)frame;
		const PlayerFrame& correspondingPlayersFrame = correspondingPlayersFramebar[internalI];
		const UI::FrameDims& dims = preppedDims[visualI];
		
		ImU32 tint = -1;
		if (visualI > drawFramebars_framebarPositionDisplay) {
			tint = tintDarker;
		}
		
		if (frame.type != FT_NONE) {
			ImVec2 frameStartVec { dims.x, drawFramebars_y };
			ImVec2 frameEndVec { dims.x + dims.width, drawFramebars_y + drawFramebars_frameItselfHeight };
			ImVec2 frameEndVecForTooltip;
			if (visualI < drawFramebars_framesCount - 1) {
				frameEndVecForTooltip = { preppedDims[visualI + 1].x, frameEndVec.y };
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
				drawFramebars_hoveredFrameIndex = visualI;
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
							if (!isNewHitType((FrameType)p)) {
								maxDescriptionHeight = max(maxDescriptionHeight, currentHeight);
							}
						}
						for (int p = 0; p < _countof(projectileFrameTypes); ++p) {
							FrameType ptype = projectileFrameTypes[p];
							float currentHeight = descriptionHeights[ptype];
							if (!isNewHitType(ptype)) {
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
					
					int ramlethalTime = 0;
					int ramlethalTimeMax = 0;
					const char* ramlethalSubAnim = nullptr;
					
					if (playerIndex == -1
							&& owningPlayerCharType == CHARACTER_TYPE_RAMLETHAL
							&& projectileFrame.animName) {
						if (strcmp(projectileFrame.animName, "S Sword") == 0) {
							ramlethalTime = correspondingPlayersFrame.u.ramlethalInfo.sSwordTime;
							ramlethalTimeMax = correspondingPlayersFrame.u.ramlethalInfo.sSwordTimeMax;
							ramlethalSubAnim = correspondingPlayersFrame.u.ramlethalInfo.sSwordSubAnim;
						} else if (strcmp(projectileFrame.animName, "H Sword") == 0) {
							ramlethalTime = correspondingPlayersFrame.u.ramlethalInfo.hSwordTime;
							ramlethalTimeMax = correspondingPlayersFrame.u.ramlethalInfo.hSwordTimeMax;
							ramlethalSubAnim = correspondingPlayersFrame.u.ramlethalInfo.hSwordSubAnim;
						}
					}
					
					if (name && *name != '\0') {
						ImGui::Separator();
						yellowText("Anim: ");
						ImGui::SameLine();
						if (ramlethalSubAnim) {
							sprintf_s(strbuf, "%s:%s", name, ramlethalSubAnim);
							ImGui::TextUnformatted(strbuf);
						} else {
							ImGui::TextUnformatted(name);
						}
					}
					if (frame.activeDuringSuperfreeze) {
						ImGui::Separator();
						ImGui::TextUnformatted("After this frame the attack becomes active during superfreeze."
							" In order to block that attack, it must be blocked on this frame, in advance.");
					}
					if (playerIndex != -1) {
						ui.drawPlayerFrameTooltipInfo((const PlayerFrame&)frame, playerIndex, wrapWidth);
					} else if (frame.powerup) {
						ImGui::Separator();
						if (owningPlayerCharType == CHARACTER_TYPE_INO) {
							ImGui::TextUnformatted("The note reached the next level on this frame: it will deal one more hit.");
						} else {
							ImGui::TextUnformatted("The projectile reached some kind of powerup on this frame.");
						}
					}
					if (playerIndex == -1) {
						if (owningPlayerCharType == CHARACTER_TYPE_INO
								&& projectileFrame.animSlangName
								&& strcmp(projectileFrame.animSlangName, MOVE_NAME_NOTE) == 0) {
							// I am a dirty scumbar
							ImGui::Separator();
							yellowText("Note elapsed time: ");
							ImGui::SameLine();
							int time = correspondingPlayersFrame.u.inoInfo.noteTime;
							int timeMax = correspondingPlayersFrame.u.inoInfo.noteTimeMax;
							const char* txt;
							if (correspondingPlayersFrame.u.inoInfo.noteLevel == 5) {
								txt = "Reached max";
							} else {
								txt = ui.printDecimal(correspondingPlayersFrame.u.inoInfo.noteTimeMax, 0, 0, false);
							}
							sprintf_s(strbuf, "%d/%s (%d hits)", time, txt, correspondingPlayersFrame.u.inoInfo.noteLevel);
							ImGui::TextUnformatted(strbuf);
						} else if (ramlethalTime) {
							ImGui::Separator();
							yellowText("Time Remaining: ");
							ImGui::SameLine();
							if (ramlethalTimeMax) {
								sprintf_s(strbuf, "%d/%d", ramlethalTime, ramlethalTimeMax);
							} else {
								sprintf_s(strbuf, "until landing + %d", ramlethalTime);
							}
							ImGui::TextUnformatted(strbuf);
						} else if (owningPlayerCharType == CHARACTER_TYPE_ELPHELT
								&& projectileFrame.animSlangName
								&& strcmp(projectileFrame.animSlangName, PROJECTILE_NAME_BERRY) == 0) {
							
							ImGui::Separator();
							yellowText("Berry Timer: ");
							ImGui::SameLine();
							sprintf_s(strbuf, "%d/180", correspondingPlayersFrame.u.elpheltInfo.grenadeTimer);
							ImGui::TextUnformatted(strbuf);
							
						} else if (owningPlayerCharType == CHARACTER_TYPE_JOHNNY
								&& projectileFrame.animSlangName
								&& strcmp(projectileFrame.animSlangName, PROJECTILE_NAME_BACCHUS) == 0) {
							
							ImGui::Separator();
							yellowText("Bacchus Sigh Projectile Timer: ");
							ImGui::SameLine();
							sprintf_s(strbuf, "%d/%d", correspondingPlayersFrame.u.johnnyInfo.mistTimer,
								correspondingPlayersFrame.u.johnnyInfo.mistTimerMax);
							ImGui::TextUnformatted(strbuf);
							
						} else if (frame.type == FT_IDLE_NO_DISPOSE
										&& owningPlayerCharType == CHARACTER_TYPE_JACKO
										&& strcmp(projectileFrame.animName, PROJECTILE_NAME_GHOST) == 0) {
							ImGui::Separator();
							ImGui::TextUnformatted("The Ghost is strike invulnerable.");
							
						} else if (frame.type == FT_IDLE_PROJECTILE_HITTABLE
										&& owningPlayerCharType == CHARACTER_TYPE_DIZZY
										&& correspondingPlayersFrame.u.dizzyInfo.shieldFishSuperArmor) {
							ImGui::Separator();
							ImGui::TextUnformatted("Shield Fish super armor active.");
						}
					}
					if (playerIndex != -1) {
						const PlayerFrame& playerFrame = (const PlayerFrame&)frame;
						if (playerFrame.hitstop
								|| playerFrame.stop.isHitstun
								|| playerFrame.stop.isBlockstun
								|| playerFrame.stop.isStagger
								|| playerFrame.stop.isWakeup
								|| playerFrame.stop.isRejection
								|| playerFrame.stop.tumble) {
							ImGui::Separator();
							if (playerFrame.hitstop
									|| playerFrame.stop.isHitstun
									|| playerFrame.stop.isBlockstun
									|| playerFrame.stop.isStagger
									|| playerFrame.stop.isWakeup
									|| playerFrame.stop.isRejection) {
								printFameStop(strbuf,
										sizeof strbuf,
										&playerFrame.stop,
										playerFrame.hitstop,
										playerFrame.hitstopMax,
										playerFrame.lastBlockWasIB,
										playerFrame.lastBlockWasFD);
								ImGui::TextUnformatted(strbuf);
							}
							if (playerFrame.stop.tumble) {
								const char* tumbleName;
								if (playerFrame.stop.tumbleIsWallstick) {
									tumbleName = "wallstick";
								} else if (playerFrame.stop.tumbleIsKnockdown) {
									tumbleName = "knockdown";
								} else {
									tumbleName = "tumble";
								}
								sprintf_s(strbuf, "%d/%d %s", playerFrame.stop.tumble, playerFrame.stop.tumbleMax, tumbleName);
								ImGui::TextUnformatted(strbuf);
							}
						}
					} else {
						if (frame.hitstop) {
							ImGui::Separator();
							printFameStop(strbuf, sizeof strbuf, nullptr, frame.hitstop, frame.hitstopMax, false, false);
							ImGui::TextUnformatted(strbuf);
						}
					}
					const SkippedFramesInfo& skippedFramesElem = skippedFrames[internalI];
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
							yellowText("RC-slowed down: ");
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

void drawPlayerFramebar(const PlayerFramebar& framebar, UI::FrameDims* preppedDims, ImU32 tintDarker, int playerIndex,
			const std::vector<SkippedFramesInfo>& skippedFrames, CharacterType charType) {
	drawFramebar<PlayerFramebar, PlayerFrame>(framebar, preppedDims, tintDarker, playerIndex, skippedFrames, framebar, charType);
}

void drawProjectileFramebar(const Framebar& framebar, UI::FrameDims* preppedDims, ImU32 tintDarker,
			const std::vector<SkippedFramesInfo>& skippedFrames, const PlayerFramebar& correspondingPlayersFramebar, CharacterType owningPlayerCharType) {
	drawFramebar<Framebar, Frame>(framebar, preppedDims, tintDarker, -1, skippedFrames, correspondingPlayersFramebar, owningPlayerCharType);
}

template<typename FramebarT, typename FrameT>
void drawFirstFrames(const FramebarT& framebar, UI::FrameDims* preppedDims, float firstFrameTopY, float firstFrameBottomY) {
	const bool considerSimilarFrameTypesSameForFrameCounts = settings.considerSimilarFrameTypesSameForFrameCounts;
	const bool considerSimilarIdleFramesSameForFrameCounts = settings.considerSimilarIdleFramesSameForFrameCounts;
	const ImVec2 firstFrameUVStart = { ui.firstFrame->uStart, ui.firstFrame->vStart };
	const ImVec2 firstFrameUVEnd = { ui.firstFrame->uEnd, ui.firstFrame->vEnd };
	const int startFrame = drawFramebars_framebarPosition == _countof(Framebar::frames) - 1
					? 0
					: drawFramebars_framebarPosition + 1;
	int internalIndNext = iterateVisualFramesFrom0_getInitialInternalInd();
	int internalInd;
	for (int visualInd = 0; visualInd < drawFramebars_framesCount; ++visualInd) {
		
		internalInd = internalIndNext;
		incrementInternalInd(internalIndNext);
		
		const FrameT& frame = framebar[internalInd];
		const UI::FrameDims& dims = preppedDims[visualInd];
		
		bool isFirst = frame.isFirst;
		if (isFirst
				&& considerSimilarFrameTypesSameForFrameCounts
				&& considerSimilarIdleFramesSameForFrameCounts
				&& frameMap(frame.type) == FT_IDLE) {
			if (internalInd == startFrame) {
				isFirst = framebar.preFrameMapped != FT_IDLE;
			} else {
				isFirst = frameMap(framebar[internalInd == 0 ? _countof(Framebar::frames) - 1 : internalInd - 1].type) != FT_IDLE;
			}
		}
		if (isFirst) {
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

// Draws frame counts of contiguous groups of similarly-typed frames, on top of the frames
template<typename FramebarT, typename FrameT>
void drawDigits(const FramebarT& framebar, UI::FrameDims* preppedDims, float frameNumberYTop, float frameNumberYBottom) {
	
	const bool showFirstFrames = settings.showFirstFramesOnFramebar;
	const bool considerSimilarFrameTypesSameForFrameCounts = settings.considerSimilarFrameTypesSameForFrameCounts;
	const bool considerSimilarIdleFramesSameForFrameCounts = settings.considerSimilarIdleFramesSameForFrameCounts;
	
	FrameType lastFrameType;
	int sameFrameTypeCount;
	if (considerSimilarFrameTypesSameForFrameCounts) {
		if (considerSimilarIdleFramesSameForFrameCounts) {
			lastFrameType = framebar.preFrameMapped;
			sameFrameTypeCount = framebar.preFrameMappedLength;
		} else {
			lastFrameType = framebar.preFrameMappedNoIdle;
			sameFrameTypeCount = framebar.preFrameMappedNoIdleLength;
		}
	} else {
		lastFrameType = framebar.preFrame;
		sameFrameTypeCount = framebar.preFrameLength;
	}
	int visualFrameCount = 0;
	bool indInView = false;
	int visualInd;
	int internalIndNext = drawFramebars_framebarPosition == _countof(Framebar::frames) - 1
		? 0
		: drawFramebars_framebarPosition + 1;
	int internalInd;
	bool prevIndInView = false;
	int prevVisualInd;
	
	for (int i = 0; i < _countof(Framebar::frames); ++i) {
		
		internalInd = internalIndNext;
		if (internalIndNext == _countof(Framebar::frames) - 1) {
			internalIndNext = 0;
		} else {
			++internalIndNext;
		}
		
		prevIndInView = indInView;
		if (indInView) {
			prevVisualInd = visualInd;
		}
		
		if (drawFramebars_framebarPosition >= drawFramebars_framesCount - 1) {
			if (internalInd <= drawFramebars_framebarPosition) {
				indInView = internalInd >= drawFramebars_framebarPosition - drawFramebars_framesCount + 1;
				if (indInView) {
					visualInd = drawFramebars_framebarPositionDisplay - (drawFramebars_framebarPosition - internalInd);
				}
			} else {
				indInView = false;
			}
		} else if (internalInd <= drawFramebars_framebarPosition) {
			indInView = true;
			visualInd = drawFramebars_framebarPositionDisplay - (drawFramebars_framebarPosition - internalInd);
		} else {
			int startInd = drawFramebars_framebarPosition - drawFramebars_framesCount + 1 + _countof(Framebar::frames);
			indInView = internalInd >= startInd;
			if (indInView) {
				visualInd = drawFramebars_framebarPositionDisplay - drawFramebars_framesCount + 1
					+ (internalInd - startInd);
			}
		}
		
		if (indInView && visualInd < 0) {
			visualInd += drawFramebars_framesCount;
		}
		
		const FrameT& frame = framebar[internalInd];
		
		enum DivisionType {
			DIVISION_TYPE_NONE,
			DIVISION_TYPE_DIFFERENT_TYPES,
			DIVISION_TYPE_REACHED_END
		} divisionType = DIVISION_TYPE_NONE;
		
		FrameType currentType = frame.type;
		if (considerSimilarFrameTypesSameForFrameCounts) {
			currentType = considerSimilarIdleFramesSameForFrameCounts ? frameMap(currentType) : frameMapNoIdle(currentType);
		} else if (currentType == FT_IDLE_NO_DISPOSE) {
			currentType = FT_IDLE_PROJECTILE;
		}
		
		bool isFirst;
		if (showFirstFrames) {
			isFirst = frame.isFirst;
			if (isFirst && considerSimilarFrameTypesSameForFrameCounts && considerSimilarIdleFramesSameForFrameCounts) {
				if (i == 0) {
					isFirst = !(
						framebar.preFrameMapped != FT_NONE
						&& frameMap(frame.type) == framebar.preFrameMapped
						&& framebar.preFrameMapped == FT_IDLE
					);
				} else {
					FrameType frameTypeMapped = frameMap(frame.type);
					isFirst = !(
						frameTypeMapped == frameMap(framebar[internalInd == 0 ? _countof(Framebar::frames) - 1 : internalInd - 1].type)
						&& frameTypeMapped == FT_IDLE
					);
				}
			}
		} else {
			isFirst = false;
		}
		
		int displayPos = -1;
		if (prevIndInView) {
			displayPos = prevVisualInd;
		}
		
		if (currentType == lastFrameType
				&& !isFirst
				&& i == _countof(Framebar::frames) - 1
				&& lastFrameType != FT_NONE) {
			divisionType = DIVISION_TYPE_REACHED_END;
			displayPos = visualInd;
			++sameFrameTypeCount;
			++visualFrameCount;
		} else if (!(currentType == lastFrameType && !isFirst)
				&& i != 0
				&& lastFrameType != FT_NONE) {
			divisionType = DIVISION_TYPE_DIFFERENT_TYPES;
		}
		
		if (
				divisionType != DIVISION_TYPE_NONE
				&& sameFrameTypeCount > 3
				&& numDigits(sameFrameTypeCount) <= visualFrameCount
				&& displayPos != -1
			) {
			
			int displayPosIter = displayPos;
			int prevIndCounter = 0;
			int sameFrameTypeCountModif = sameFrameTypeCount;
			while (sameFrameTypeCountModif) {
				
				int remainder = sameFrameTypeCountModif % 10;
				sameFrameTypeCountModif /= 10;
				
				const UVStartEnd& digitImg = digitUVs[remainder];
				
				const UI::FrameDims& prevDim = preppedDims[displayPosIter];
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
				if (displayPosIter == 0) {
					displayPosIter = drawFramebars_framesCount - 1;
				} else {
					--displayPosIter;
				}
			}
		}
		
		if (currentType == lastFrameType && !isFirst) {
			++sameFrameTypeCount;
			if (prevIndInView) {
				++visualFrameCount;
			}
		} else {
			lastFrameType = currentType;
			sameFrameTypeCount = 1;
			visualFrameCount = prevIndInView ? 1 : 0;
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
	yellowText("Invisibility: ");
	ImGui::SameLine();
	ImGui::Text("%s (%d/%d)",
		ui.printDecimal(percentage, 0, 3, true),
		current,
		max);
}

void textUnformattedColored(ImVec4 color, const char* str, const char* strEnd) {
	ImGui::PushStyleColor(ImGuiCol_Text, color);
	ImGui::TextUnformatted(str, strEnd);
	ImGui::PopStyleColor();
}

void yellowText(const char* str, const char* strEnd) {
	ImGui::PushStyleColor(ImGuiCol_Text, YELLOW_COLOR);
	ImGui::TextUnformatted(str, strEnd);
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
	if (wrapPos != textEnd && (wrapPos == str || (wrapPos <= str + 3) && *wrapPos > 32) && needSameLine) {
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

void UI::startupOrTotal(int two, StringWithLength title, bool* showTooltipFlag) {
	for (int i = 0; i < two; ++i) {
		PlayerInfo& player = endScene.players[i];
		ImGui::TableNextColumn();
		printWithWordWrapArg(strbufs[i])
		if (ImGui::BeginItemTooltip()) {
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			if (printNameParts(-1, nameParts[i], strbuf, sizeof strbuf)) {
				searchFieldValue(strbuf, nullptr);
				ImGui::TextUnformatted(strbuf);
			}
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}
		
		if (i == 0) {
			ImGui::TableNextColumn();
			headerThatCanBeClickedForTooltip(searchFieldTitle(title), showTooltipFlag, false);
			if (ImGui::BeginItemTooltip()) {
				ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
				for (int j = 0; j < 2; ++j) {
					if (!nameParts[j].empty()) {
						char* buf = strbuf;
						size_t bufSize = sizeof strbuf;
						int result = sprintf_s(buf, bufSize, "Player %d Move: ", j + 1);
						advanceBuf
						printNameParts(j, nameParts[j], buf, bufSize);
						searchFieldValue(strbuf, nullptr);
						ImGui::TextUnformatted(strbuf);
					}
				}
				ImGui::Separator();
				ImGui::TextUnformatted("Click the field for tooltip.");
				ImGui::PopTextWrapPos();
				ImGui::EndTooltip();
			}
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
	StringWithLength text = settings.getOtherUINameWithLength(&settingsRef);
	if (settingsPresetsUseOutlinedText) {
		float squareSize = ImGui::GetFrameHeight();
		ImGuiStyle& style = ImGui::GetStyle();
		ImVec2 cursor = ImGui::GetCursorPos();
		ImVec2 newPos = {cursor.x + squareSize + style.ItemInnerSpacing.x, cursor.y + style.FramePadding.y};
		outlinedTextJustTheOutline(newPos, text.txt, nullptr, true);
		ImGui::SetCursorPos(cursor);
	}
	if (ImGui::Checkbox(searchFieldTitle(text), &boolValue)) {
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

bool UI::intSettingPreset(std::atomic_int& settingsPtr, int minValue, int step, int stepFast, float fieldWidth, int maxValue) {
	bool isChange = false;
	int intValue = settingsPtr;
	ImGui::SetNextItemWidth(fieldWidth);
	if (ImGui::InputInt(searchFieldTitle(settings.getOtherUINameWithLength(&settingsPtr)), &intValue, step, stepFast, 0)) {
		if (intValue < minValue) {
			intValue = minValue;
		}
		if (intValue > maxValue) {
			intValue = maxValue;
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

void drawTextInWindowTitle(const char* txt) {
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
		startPos.y + 1000.F  // ImGui::CalcTextSize(txt).y produces height that was too small, and tails of y's were cut off
	};
	if (clipEnd.x > startPos.x) {
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->PushClipRect(startPos,
			clipEnd,
			false);
		drawList->AddText(startPos,
			ImGui::GetColorU32(IM_COL32(255, 255, 255, 255)),
			txt);
		drawList->PopClipRect();
	}
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

void UI::printAllCancels(const FrameCancelInfo<30>& cancels,
		bool enableSpecialCancel,
		bool enableJumpCancel,
		bool enableSpecials,
		bool hitAlreadyHappened,
		bool airborne,
		bool insertSeparators,
		bool useMaxY) {
	
	const ImGuiStyle& style = ImGui::GetStyle();
	float maxY;
	if (useMaxY) {
		maxY = ImGui::GetIO().DisplaySize.y - style.FramePadding.y * 4.F - style.ItemSpacing.y * 6.F
			- ImGui::GetTextLineHeightWithSpacing() * 4.F;
	} else {
		maxY = FLT_MAX;
	}
	
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
		yellowText("Gatlings:");
		int count = 1;
		if (!cancels.gatlings.empty()) {
			count += printCancels(cancels.gatlings, maxY);
		}
		if (enableSpecialCancel && ImGui::GetCursorPosY() < maxY) {
			ImGui::Text("%d) Specials", count);
			++count;
		}
		if (enableJumpCancel && ImGui::GetCursorPosY() < maxY) {
			ImGui::Text("%d) Jump cancel", count);
		}
	}
	if ((!cancels.whiffCancels.empty() || enableSpecials) && ImGui::GetCursorPosY() < maxY) {
		if (insertSeparators) ImGui::Separator();
		if (hitAlreadyHappened) {
			yellowText("Late cancels:");
		} else {
			yellowText("Whiff cancels:");
		}
		maxY += ImGui::GetTextLineHeightWithSpacing();
		int count = 1;
		if (!cancels.whiffCancels.empty()) {
			count += printCancels(cancels.whiffCancels, maxY);
		}
		if (enableSpecials && ImGui::GetCursorPosY() < maxY) {
			if (airborne) {
				ImGui::Text("%d) Specials (note: some specials have a minimum height limit and might be unavailable at this time)", count);
			} else {
				ImGui::Text("%d) Specials", count);
			}
		}
	}
	if (cancels.gatlings.empty() && !enableSpecialCancel
			&& cancels.whiffCancels.empty() && !enableSpecials
			&& cancels.whiffCancelsNote && ImGui::GetCursorPosY() < maxY) {
		if (insertSeparators) ImGui::Separator();
		if (hitAlreadyHappened) {
			yellowText("Late cancels:");
		} else {
			yellowText("Whiff cancels:");
		}
		ImGui::TextUnformatted(cancels.whiffCancelsNote);
	}
	if (needUnpush) {
		ImGui::PopStyleVar();
	}
}

bool printMoveFieldTooltip(const PlayerInfo& player) {
	if (player.canPrintTotal() || player.startupType() != -1) {
		*strbuf = '\0';
		char* buf = strbuf;
		size_t bufSize = sizeof strbuf;
		const char* lastName = nullptr;
		int lastNameDuration = 0;
		prepareLastNames(&lastName, player, true, &lastNameDuration);
		player.prevStartupsDisp.printNames(buf, bufSize, &lastName,
				1,
				false,
				true,
				&lastNameDuration);
		return true;
	}
	return false;
}

bool printMoveField(const PlayerInfo& player) {
	if (player.canPrintTotal() || player.startupType() != -1) {
		*strbuf = '\0';
		char* buf = strbuf;
		size_t bufSize = sizeof strbuf;
		const char* lastName = nullptr;
		int lastNameDuration = 0;
		prepareLastNames(&lastName, player, false, &lastNameDuration);
		player.prevStartupsDisp.printNames(buf, bufSize, &lastName,
				1,
				settings.useSlangNames.load(),
				true,
				false,
				&lastNameDuration);
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

void prepareLastNames(const char** lastName, const PlayerInfo& player, bool disableSlang,
						int* lastNameDuration) {
	*lastName = player.getLastPerformedMoveName(disableSlang);
	int startupType = player.startupType();
	if (startupType == 1) {
		*lastNameDuration = player.superfreezeStartup;
	} else if (startupType == 0 || startupType == 2) {
		
		int lowestStartup = INT_MAX;
		if (player.startedUp) {
			lowestStartup = player.startupDisp - player.superfreezeStartup;
		}
		if (player.startupProj && player.startupProj < lowestStartup) {
			lowestStartup = player.startupProj;
		}
		
		if (lowestStartup != INT_MAX) {
			*lastNameDuration = lowestStartup;
		} else {
			*lastNameDuration = 0;
		}
	} else {
		*lastNameDuration = 0;
	}
}

static bool printNameParts(int playerIndex, std::vector<NameDuration>& elems, char* buf, size_t bufSize) {
	int result;
	bool isFirst = true;
	for (const NameDuration& elem : elems) {
		result = sprintf_s(buf, bufSize, "%s%df (%s)",
			isFirst ? "" : "+",
			elem.duration,
			elem.name);
		advanceBuf
		isFirst = false;
	}
	return !isFirst;
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
		" Guts rating table:\n"
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
	yellowText("Damage");
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
	yellowText(strbuf);
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
	yellowText("Damage");
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
	yellowText(strbuf);
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
		yellowText("Damage");
		ImGui::SameLine();
		ImGui::TextUnformatted(" * Scale");
		_zerohspacing
		ImGui::TableNextColumn();
		
		zerohspacing
		sprintf_s(strbuf, "%s * %d%c = ", ui.printDecimal(oldX, 1, 0, false), damageScale, '%');
		ImGui::TextUnformatted(strbuf);
		ImGui::SameLine();
		yellowText(ui.printDecimal(x, 1, 0, false));
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
	yellowText("Damage");
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
		yellowText(ui.printDecimal(x, 1, 0, false));
		ImGui::SameLine();
		ImGui::TextUnformatted(")");
		_zerohspacing
	} else {
		zerohspacing
		sprintf_s(strbuf, "%s * %d%c = ", ui.printDecimal(oldX, 1, 0, false), projectileDamageScale, '%');
		ImGui::TextUnformatted("Doesn't apply (");
		ImGui::SameLine();
		yellowText(ui.printDecimal(x, 1, 0, false));
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
		yellowText("Damage");
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
		yellowText(strbuf);
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
	
	yellowText(searchFieldTitle("Attack Level: "));
	ImGui::SameLine();
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
	yellowText("Damage");
	_zerohspacing
	
	ImGui::TableNextColumn();
	sprintf_s(strbuf, "%d", dmgCalc.dealtOriginalDamage);
	yellowText(strbuf);
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
		yellowText("Damage");
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
		yellowText(strbuf);
		_zerohspacing
	}
	
	if (dmgWithHpScale) *dmgWithHpScale = x;
	
	if (dmgCalc.adds5Dmg) {
		ImGui::TableNextColumn();
		searchFieldTitle("Damage + 5");
		zerohspacing
		const char* tooltip = searchTooltip("In certain dust situations, the attack gains 5 extra damage.");
		yellowText("Damage");
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
		yellowText(strbuf);
		_zerohspacing
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
		yellowText("Damage");
		ImGui::SameLine();
		ImGui::TextUnformatted(" * OTG");
		_zerohspacing
		ImGui::TableNextColumn();
		zerohspacing
		sprintf_s(strbuf, "%d * %d%c = ", oldX, scale, '%');
		ImGui::TextUnformatted(strbuf);
		ImGui::SameLine();
		sprintf_s(strbuf, "%d", x);
		yellowText(strbuf);
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
		yellowText("Since previous displayed frame, skipped:");
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
	
	ImDrawListBackup* lists[3] { nullptr };
	int listsCount = 0;
	if (drewFramebar) {
		lists[listsCount++] = (ImDrawListBackup*)framebarHorizontalScrollbarDrawDataCopy.data();
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

void UI::printChargeInFrameTooltip(const char* title, unsigned char value, unsigned char valueMax, unsigned char valueLast) {
	yellowText(title);
	ImGui::SameLine();
	if (value == 255) {
		sprintf_s(strbuf, "254+/%d", valueMax);
	} else if (value == 0) {
		sprintf_s(strbuf, "0/%d (last %d)", valueMax, valueLast);
	} else {
		sprintf_s(strbuf, "%d/%d", value, valueMax);
	}
	ImGui::TextUnformatted(strbuf);
}

void UI::printChargeInCharSpecific(int playerIndex, bool showHoriz, bool showVert, int maxCharge) {
	const InputRingBuffer* ringBuffer = game.getInputRingBuffers();
	if (ringBuffer) {
		ringBuffer += playerIndex;
		if (showHoriz) {
			yellowText(searchFieldTitle("Charge (left):"));
			ImGui::SameLine();
			int charge = ringBuffer->parseCharge(InputRingBuffer::CHARGE_TYPE_HORIZONTAL, false);
			if (charge != 0) {
				sprintf_s(strbuf, "%2d/%d", charge, maxCharge);
			} else {
				sprintf_s(strbuf, " 0/%d (last %d)", maxCharge, endScene.players[playerIndex].chargeLeftLast);
			}
			ImGui::TextUnformatted(strbuf);
			
			yellowText(searchFieldTitle("Charge (right):"));
			ImGui::SameLine();
			charge = ringBuffer->parseCharge(InputRingBuffer::CHARGE_TYPE_HORIZONTAL, true);
			if (charge != 0) {
				sprintf_s(strbuf, "%2d/%d", charge, maxCharge);
			} else {
				sprintf_s(strbuf, " 0/%d (last %d)", maxCharge, endScene.players[playerIndex].chargeRightLast);
			}
			ImGui::TextUnformatted(strbuf);
		}
		
		if (showVert) {
			yellowText(searchFieldTitle("Charge (down):"));
			ImGui::SameLine();
			int charge = ringBuffer->parseCharge(InputRingBuffer::CHARGE_TYPE_VERTICAL, false);
			if (charge != 0) {
				sprintf_s(strbuf, "%2d/%d", charge, maxCharge);
			} else {
				sprintf_s(strbuf, " 0/%d (last %d)", maxCharge, endScene.players[playerIndex].chargeDownLast);
			}
			ImGui::TextUnformatted(strbuf);
		}
	}
}

void UI::resetFrameSelection() {
	selectingFrames = false;
	selectedFrameStart = -1;
	selectedFrameEnd = -1;
}

int UI::findHoveredFrame(float x, FrameDims* dims) {
	if (drawFramebars_framesCount <= 1) return 0;
	if (x < dims[1].x) {
		return 0;
	} else if (x >= dims[drawFramebars_framesCount - 1].x) {
		return drawFramebars_framesCount - 1;
	} else {
		int start, end, mid;
		start = 0;
		end = drawFramebars_framesCount - 1;
		do {
			mid = (start + end) >> 1;
			if (x < dims[mid].x) {
				end = mid - 1;
			} else {
				start = mid;
			}
		} while (end - start > 1);
		if (start == end || x < dims[end].x) return start;
		return end;
	}
}

void UI::drawRightAlignedP1TitleWithCharIcon() {
	GGIcon scaledIcon = scaleGGIconToHeight(getPlayerCharIcon(0), 14.F);
	float w = ImGui::CalcTextSize("P1").x + getItemSpacing() + scaledIcon.size.x;
	RightAlign(w);
	drawPlayerIconWithTooltip(0);
	ImGui::SameLine();
	ImGui::TextUnformatted("P1");
}

void UI::onFramebarReset() {
	onFramebarAdvanced();
	framebarAutoScroll = true;
}

void UI::onFramebarAdvanced() {
	resetFrameSelection();
	framebarScrollX = 0.F;
	framebarAutoScroll = true;
}

void UI::drawFramebars() {
	
	const bool showFirstFrames = settings.showFirstFramesOnFramebar;
	const bool showStrikeInvulOnFramebar = settings.showStrikeInvulOnFramebar;
	const bool showSuperArmorOnFramebar = settings.showSuperArmorOnFramebar;
	const bool showThrowInvulOnFramebar = settings.showThrowInvulOnFramebar;
	const bool showOTGOnFramebar = settings.showOTGOnFramebar;
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
	float powerupHeight = powerupHeightOriginal * drawFramebars_frameItselfHeight / frameHeightOriginal;
	if (powerupHeight < 4.F) {
		powerupHeight = 4.F;
	}
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
	static const float framedataBottomPadding = 2.F;
	drawFramebars_innerBorderThickness = innerBorderThicknessUnscaled * scale;
	if (drawFramebars_innerBorderThickness < 1.F) drawFramebars_innerBorderThickness = 1.F;
	drawFramebars_innerBorderThicknessHalf = drawFramebars_innerBorderThickness * 0.5F;
	
	// Space reserved on top of a framebar for top markers and first frame indicator
	float maxTopPadding;
	if (!showFirstFrames) {
		maxTopPadding = 0.F;
	} else {
		maxTopPadding = firstFrameHeightDiff * 0.5F;
		if (maxTopPadding < 0.F) maxTopPadding = 0.F;
	}
	if (showStrikeInvulOnFramebar || showSuperArmorOnFramebar) {
		const float otherTopPadding = -outerBorderThickness + markerPaddingHeight + frameMarkerHeight;
		if (otherTopPadding > maxTopPadding) {
			maxTopPadding = otherTopPadding;
		}
	}
	// Space reserved under a framebar for bottom markers
	float bottomPadding = -outerBorderThickness + markerPaddingHeight + frameMarkerHeight;
	if (bottomPadding < 0.F || !showThrowInvulOnFramebar && !showOTGOnFramebar) {
		bottomPadding = 0.F;
	}
	
	float paddingBetweenFramebars = paddingBetweenFramebarsOriginal;
	const float minPaddingBetweenFramebars = paddingBetweenFramebarsMin + (maxTopPadding + bottomPadding);
	if (paddingBetweenFramebars < minPaddingBetweenFramebars) {
		paddingBetweenFramebars = minPaddingBetweenFramebars;
	}
	
	const float oneFramebarHeight = outerBorderThickness
		+ drawFramebars_frameItselfHeight
		+ outerBorderThickness;
	
	float framebarFrameDataHeight = 0.F;
	bool needShowFramebarFrameDataP1 = settings.showP1FramedataInFramebar;
	bool needShowFramebarFrameDataP2 = settings.showP2FramedataInFramebar;
	if (needShowFramebarFrameDataP1 || needShowFramebarFrameDataP2) {
		framebarFrameDataHeight = ImGui::GetTextLineHeight() + 3.F;  // for whatever reason it's missing ~3px for the tails of letters like y and g
		// also we're going to add a 1px outline all around the text, so that adds 2 more pixels
	}
	
	drawFramebars_framesCount = framebarSettings.framesCount;
	
	int framebarTotalFramesUnlimited = framebarSettings.neverIgnoreHitstop
		? endScene.getTotalFramesHitstopUnlimited()
		: endScene.getTotalFramesUnlimited();
	
	// Capped between 0 and framebarSettings.storedFramesCount, inclusive
	int framebarTotalFramesCapped;
	if (framebarTotalFramesUnlimited > framebarSettings.storedFramesCount) {
		framebarTotalFramesCapped = framebarSettings.storedFramesCount;
	} else {
		framebarTotalFramesCapped = framebarTotalFramesUnlimited;
	}
	
	
	const float framesCountFloat = (float)drawFramebars_framesCount;
	
	// this value is in [0;_countof(Framebar::frames)] coordinate space, its possible range of values is [0;_countof(Framebar::frames)-1]
	// It does not have horizontal scrolling applied to it
	const int framebarPosition = framebarSettings.neverIgnoreHitstop ? endScene.getFramebarPositionHitstop() : endScene.getFramebarPosition();
	
	int scrollXInFrames = framebarSettings.scrollXInFrames;
	
	drawFramebars_framebarPosition = framebarPosition - scrollXInFrames;
	if (drawFramebars_framebarPosition < 0) {
		drawFramebars_framebarPosition += _countof(Framebar::frames);
	}
	
	int framebarTotalFramesUnlimited_withScroll;
	if (framebarTotalFramesUnlimited < scrollXInFrames) {
		framebarTotalFramesUnlimited_withScroll = 0;
	} else {
		framebarTotalFramesUnlimited_withScroll = framebarTotalFramesUnlimited - scrollXInFrames;
	}
	
	drawFramebars_framebarPositionDisplay = framebarTotalFramesUnlimited_withScroll == 0
		? 0
		: (framebarTotalFramesUnlimited_withScroll - 1) % drawFramebars_framesCount;
	
	bool recheckCompletelyEmpty = drawFramebars_framesCount != _countof(Framebar::frames);
	const bool eachProjectileOnSeparateFramebar = framebarSettings.eachProjectileOnSeparateFramebar;
	
	std::vector<bool> framebarsCompletelyEmpty;
	if (recheckCompletelyEmpty) {
		if (eachProjectileOnSeparateFramebar) {
			framebarsCompletelyEmpty.resize(endScene.projectileFramebars.size());
			int index = 0;
			for (const ProjectileFramebar& entityFramebar : endScene.projectileFramebars) {
				const FramebarBase& framebar = framebarSettings.neverIgnoreHitstop ? entityFramebar.getHitstop() : entityFramebar.getMain();
				framebarsCompletelyEmpty[index++] = framebar.lastNFramesCompletelyEmpty(drawFramebars_framebarPosition, drawFramebars_framesCount);
			}
		} else {
			framebarsCompletelyEmpty.resize(endScene.combinedFramebars.size());
			int index = 0;
			for (const CombinedProjectileFramebar& entityFramebar : endScene.combinedFramebars) {
				const FramebarBase& framebar = framebarSettings.neverIgnoreHitstop ? entityFramebar.getHitstop() : entityFramebar.getMain();
				framebarsCompletelyEmpty[index++] = framebar.lastNFramesCompletelyEmpty(drawFramebars_framebarPosition, drawFramebars_framesCount);
			}
		}
	}
	
	std::vector<const EntityFramebar*> framebars;
	size_t playerFramebarsCount = endScene.playerFramebars.size();
	if (playerFramebarsCount > 2) playerFramebarsCount = 2;
	if (eachProjectileOnSeparateFramebar) {
		size_t nonEmptyCount = 0;
		int index = 0;
		for (const ProjectileFramebar& entityFramebar : endScene.projectileFramebars) {
			const FramebarBase& framebar = framebarSettings.neverIgnoreHitstop ? entityFramebar.getHitstop() : entityFramebar.getMain();
			if (!(
					framebar.completelyEmpty || recheckCompletelyEmpty && framebarsCompletelyEmpty[index]
			)) {
				++nonEmptyCount;
			}
			++index;
		}
		framebars.resize(playerFramebarsCount + nonEmptyCount, nullptr);
	} else if (recheckCompletelyEmpty) {
		size_t nonEmptyCount = 0;
		int index = 0;
		for (const CombinedProjectileFramebar& entityFramebar : endScene.combinedFramebars) {
			const FramebarBase& framebar = framebarSettings.neverIgnoreHitstop ? entityFramebar.getHitstop() : entityFramebar.getMain();
			if (!framebarsCompletelyEmpty[index]) {
				++nonEmptyCount;
			}
			++index;
		}
		framebars.resize(playerFramebarsCount + nonEmptyCount, nullptr);
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
			int index = 0;
			for (const ProjectileFramebar& entityFramebar : endScene.projectileFramebars) {
				const FramebarBase& framebar = framebarSettings.neverIgnoreHitstop ? entityFramebar.getHitstop() : entityFramebar.getMain();
				if (entityFramebar.playerIndex == i && !(
						framebar.completelyEmpty || recheckCompletelyEmpty && framebarsCompletelyEmpty[index]
				)) {
					framebars[framebarsCount++] = (const EntityFramebar*)&entityFramebar;
				}
				++index;
			}
		} else {
			int index = 0;
			for (const CombinedProjectileFramebar& entityFramebar : endScene.combinedFramebars) {
				if (entityFramebar.playerIndex == i && !(recheckCompletelyEmpty && framebarsCompletelyEmpty[index])) {
					framebars[framebarsCount++] = (const EntityFramebar*)&entityFramebar;
				}
				++index;
			}
		}
	}
	if (eachProjectileOnSeparateFramebar) {
		int index = 0;
		for (const ProjectileFramebar& entityFramebar : endScene.projectileFramebars) {
			const FramebarBase& framebar = framebarSettings.neverIgnoreHitstop ? entityFramebar.getHitstop() : entityFramebar.getMain();
			if (entityFramebar.playerIndex != 0 && entityFramebar.playerIndex != 1 && !(
					framebar.completelyEmpty || recheckCompletelyEmpty && framebarsCompletelyEmpty[index]
			)) {
				framebars[framebarsCount++] = (const EntityFramebar*)&entityFramebar;
			}
			++index;
		}
	} else {
		int index = 0;
		for (const CombinedProjectileFramebar& entityFramebar : endScene.combinedFramebars) {
			if (entityFramebar.playerIndex != 0 && entityFramebar.playerIndex != 1
					&& !(recheckCompletelyEmpty && framebarsCompletelyEmpty[index])) {
				framebars[framebarsCount++] = (const EntityFramebar*)&entityFramebar;
			}
			++index;
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
		+ oneFramebarHeight * 2.F
		+ (needShowFramebarFrameDataP1 ? framebarFrameDataHeight : 0.F)
		+ (needShowFramebarFrameDataP2 ? framebarFrameDataHeight : 0.F)
		+ (framebarsCount <= 1 ? 0.F : paddingBetweenFramebars)
		+ bottomPadding
		+ 1.F
		+ 2.F  // imgui window padding
	}, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowContentSize({ 0.F,
		// don't add imgui window padding here
		+ 1.F
		+ maxTopPadding
		+ oneFramebarHeight * (float)framebarsCount
		+ (needShowFramebarFrameDataP1 && playerFramebarsCount > 0 ? framebarFrameDataHeight : 0.F)
		+ (needShowFramebarFrameDataP2 && playerFramebarsCount > 1 ? framebarFrameDataHeight : 0.F)
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
		| ImGuiWindowFlags_NoBringToFrontOnFocus
		| ImGuiWindowFlags_NoFocusOnAppearing);
	bool drawFullBorder = ImGui::IsMouseDown(ImGuiMouseButton_Left)
		&& ImGui::IsMouseHoveringRect({ 0.F, 0.F }, { FLT_MAX, FLT_MAX }, true)
		&& ImGui::IsWindowFocused();
	ImGui::PopStyleVar();
	bool scaledText = scale > 1.F;
	if (scaledText) {
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
	
	const float framesXMask = framesX - outerBorderThickness;
	const float framesXEndMask = framesXEnd - drawFramebars_innerBorderThickness + outerBorderThickness;
	
	const float scrollY = ImGui::GetScrollY();
	drawFramebars_y = drawFramebars_windowPos.y 
		+ 2.F  // imgui window padding
		+ 1.F
		+ maxTopPadding
		+ outerBorderThickness
		- scrollY
		+ (needShowFramebarFrameDataP1 && playerFramebarsCount > 0 ? framebarFrameDataHeight : 0.F);
	
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
	
	FrameDims preppedDims[_countof(Framebar::frames)];  // we're only going to use drawFramebars_framesCount of these
	float highlighterStartX[2] { 0.F, 0.F };
	int highlighterCount = 0;
	
	if (framesXEnd > framesX) {
		FrameDims* dims;
		float x = framesX;
		const float totalVisibleFramesWidth = framesXEnd - framesX;
		
		for (int i = 0; i < drawFramebars_framesCount - 1; ++i) {
			
			float thisFrameXEnd = std::round(totalVisibleFramesWidth * (float)(i + 1) / framesCountFloat + framesX);
			float thisFrameWidth = thisFrameXEnd - x;
			float thisFrameWidthWithoutOutline = thisFrameWidth - drawFramebars_innerBorderThickness;
			
			if (thisFrameWidth < 0.99F) {
				thisFrameWidth = 1.F;
				thisFrameWidthWithoutOutline = 1.F;
			}
			
			dims = preppedDims + i;
			dims->x = x;
			dims->width = thisFrameWidthWithoutOutline;
			
			x = thisFrameXEnd;
		}
		
		dims = preppedDims + (drawFramebars_framesCount - 1);
		dims->x = x;
		x = framesXEnd;
		dims->width = x - drawFramebars_innerBorderThickness - dims->x;
		
		int highlighterPos = (
			drawFramebars_framebarPositionDisplay == drawFramebars_framesCount - 1
				? 0
				: drawFramebars_framebarPositionDisplay + 1
			);
		highlighterStartX[0] = preppedDims[highlighterPos].x - drawFramebars_innerBorderThickness;
		++highlighterCount;
		
		if (drawFramebars_framebarPositionDisplay == drawFramebars_framesCount - 1) {
			highlighterStartX[1] = framesXEnd - drawFramebars_innerBorderThickness;
			++highlighterCount;
		}
		
	}
	
	static float P1P2TextSizeWithSpace;
	static float P1P2TextSize;
	static bool P1P2TextSizeCalculated = false;
	static float P1P2TextScale = 1.F;
	if (!P1P2TextSizeCalculated || P1P2TextScale != scale) {
		P1P2TextSizeCalculated = true;
		P1P2TextScale = scale;
		P1P2TextSizeWithSpace = ImGui::CalcTextSize("P1 ").x;
		P1P2TextSize = ImGui::CalcTextSize("P1").x;
	}
	
	drawFramebars_frameArtArray = settings.useColorblindHelp ? frameArtColorblind : frameArtNonColorblind;
	const FrameMarkerArt* frameMarkerArtArray = settings.useColorblindHelp ? frameMarkerArtColorblind : frameMarkerArtNonColorblind;
	const FrameMarkerArt& strikeInvulMarker = frameMarkerArtArray[MARKER_TYPE_STRIKE_INVUL];
	const FrameMarkerArt& throwInvulMarker = frameMarkerArtArray[MARKER_TYPE_THROW_INVUL];
	const FrameMarkerArt& OTGMarker = frameMarkerArtArray[MARKER_TYPE_OTG];
	
	drawFramebars_hoveredFrameIndex = -1;
	const float currentPositionHighlighter_Strip1_StartY = drawFramebars_y;
	float currentPositionHighlighter_Strip1_EndY = drawFramebars_y;
	float currentPositionHighlighter_Strip2_StartY = drawFramebars_y;
	
	const bool showPlayerInFramebarTitle = settings.showPlayerInFramebarTitle;
	const int framebarTitleCharsMax = settings.framebarTitleCharsMax;
	const std::vector<SkippedFramesInfo>& skippedFrames = endScene.getSkippedFrames(framebarSettings.neverIgnoreHitstop);
	
	// we're positioned after outerBorderThickness of the next framebar
	const float offsetAfterP2Framedata =
		// imaginary (we don't actually do this): -outerBorderThickness
		-paddingBetweenFramebars
		+ bottomPadding
		+ framebarFrameDataHeight
		+ framedataBottomPadding
		+ maxTopPadding
		// imaginary: + outerBorderThickness   ; drawFramebars_y must always point to the top Y of frame's graphical art, not it's outer border
		;
	
	bool lastWasP2Framedata = false;
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
			const Frame* frame = nullptr;  // I initialize this variable because of compiler warning C4703 'potentially uninitialized variable used' which can't be removed even with const bool check in the if
			
			if (lastWasP2Framedata) {
				titleY += offsetAfterP2Framedata;
				lastWasP2Framedata = false;
			}
			
			const char* title = nullptr;
			const char* titleFull = nullptr;
			static std::string titleShortStr;
			
			const bool hasTitleInFrame = framebarTitleCharsMax > 0 && !entityFramebar.belongsToPlayer();
			if (!hasTitleInFrame) {
				title = nullptr;
			} else {
				const Framebar* framebar;
				if (eachProjectileOnSeparateFramebar) {
					const ProjectileFramebar& projectileFramebar = (const ProjectileFramebar&)entityFramebar;
					framebar = framebarSettings.neverIgnoreHitstop ? &projectileFramebar.hitstop : &projectileFramebar.main;
				} else {
					const CombinedProjectileFramebar& combinedProjectileFramebar = (const CombinedProjectileFramebar&)entityFramebar;
					framebar = &combinedProjectileFramebar.main;
				}
				frame = &(*framebar)[drawFramebars_framebarPosition];
				const char* selectedTitle = nullptr;
				if (eachProjectileOnSeparateFramebar) {
					if (useSlang) {
						selectedTitle = frame->title.slangUncombined;
					}
					if (!selectedTitle) selectedTitle = frame->title.uncombined;
					titleFull = frame->title.uncombined;
				}
				if (!selectedTitle) {
					if (useSlang) {
						selectedTitle = frame->title.slang;
					}
					if (!selectedTitle) selectedTitle = frame->title.text;
				}
				if (!titleFull && useSlang) {
					titleFull = frame->title.text;
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
			if (hasTitleInFrame
					&& frame->title.full
					&& *frame->title.full != '\0') {
				titleFull = frame->title.full;
			}
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
			
			bool drewTitle = false;
			if (showPlayerInFramebarTitle && (entityFramebar.playerIndex == 0 || entityFramebar.playerIndex == 1)) {
				
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
						titleFull = "Player 1";
					} else {
						P1P2Str = "P2";
						titleFull = "Player 2";
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
				drewTitle = true;
			} else if (!entityFramebar.belongsToPlayer() && title) {
				outlinedText({
					isOnTheLeft
						? textX - textSize.x
						: textX,
					textY
				},
				title);
				drewTitle = true;
			}
			if (titleFull
					&& (drewTitle && ImGui::IsItemHovered(ImGuiHoveredFlags_RectOnly) || hoveredExtra)
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
			if (needShowFramebarFrameDataP2
					&& entityFramebar.belongsToPlayer()
					&& entityFramebar.playerIndex == 1) {
				lastWasP2Framedata = true;
			}
		}
		ImGui::PopClipRect();
	}
	
	#define pushFramesClipRect(intersect_with_current_clip_rect) \
		drawFramebars_drawList->PushClipRect({ \
				framesXMask, \
				-10000.F \
			}, \
			{ \
				framesXEndMask, \
				10000.F \
			}, \
			intersect_with_current_clip_rect);
	
	pushFramesClipRect(true)
	
	bool has2StripsOfCurrentPositionHighlighter = false;
	lastWasP2Framedata = false;
	for (const EntityFramebar* entityFramebarPtr : framebars) {
		const EntityFramebar& entityFramebar = *entityFramebarPtr;
		const FramebarBase& framebar = framebarSettings.neverIgnoreHitstop ? entityFramebar.getHitstop() : entityFramebar.getMain();
		
		if (lastWasP2Framedata) {
			drawFramebars_y += offsetAfterP2Framedata;
			currentPositionHighlighter_Strip2_StartY = drawFramebars_y;
			has2StripsOfCurrentPositionHighlighter = true;
			lastWasP2Framedata = false;
		}
		
		if ((
					needShowFramebarFrameDataP1
					&& entityFramebar.playerIndex == 0
					|| needShowFramebarFrameDataP2
					&& entityFramebar.playerIndex == 1
				)
				&& entityFramebar.belongsToPlayer()) {
			const PlayerFramebar& playerFramebar = (const PlayerFramebar&)framebar;
			const PlayerFrame& playerFrame = playerFramebar[drawFramebars_framebarPosition];
			sprintf_s(strbuf, "Startup: %d, Active: %d, Recovery: %d, Total: %d, Advantage: ",
				playerFrame.startup,
				playerFrame.active,
				playerFrame.recovery,
				playerFrame.total);
			if (scaledText) {
				ImGui::SetWindowFontScale(1.F);
			}
			ImVec2 textSize = ImGui::CalcTextSize(strbuf);
			ImVec2 textPos;
			textPos.x = framesXMask;
			if (entityFramebar.playerIndex == 0) {
				textPos.y = drawFramebars_y - outerBorderThickness - framebarFrameDataHeight - maxTopPadding;
			} else {
				textPos.y = drawFramebars_y - outerBorderThickness + oneFramebarHeight + bottomPadding;
			}
			outlinedTextRaw(drawFramebars_drawList, textPos, strbuf, nullptr, nullptr, true);
			textPos.x += textSize.x;
			
			short frameAdvantage;
			short landingFrameAdvantage;
			if (scrollXInFrames == 0) {
				// get the current frame advantage if the framebar playhead position is the latest one
				FrameAdvantageForFramebarResult advRes;
				endScene.players[entityFramebar.playerIndex].calcFrameAdvantageForFramebar(&advRes);
				
				frameAdvantage = dontUsePreBlockstunTime
					? advRes.frameAdvantageNoPreBlockstun
					: advRes.frameAdvantage;
					
				landingFrameAdvantage = dontUsePreBlockstunTime
					? advRes.landingFrameAdvantageNoPreBlockstun
					: advRes.landingFrameAdvantage;
				
			} else {
				frameAdvantage = dontUsePreBlockstunTime
					? playerFrame.frameAdvantageNoPreBlockstun
					: playerFrame.frameAdvantage;
					
				landingFrameAdvantage = dontUsePreBlockstunTime
					? playerFrame.landingFrameAdvantageNoPreBlockstun
					: playerFrame.landingFrameAdvantage;
			}
			
			if (frameAdvantage == SHRT_MIN) {
				outlinedTextRaw(drawFramebars_drawList, textPos, "?", nullptr, nullptr, true);
			} else {
				int result = frameAdvantageTextFormat(frameAdvantage, strbuf, sizeof strbuf);
				if (landingFrameAdvantage != SHRT_MIN) {
					strbuf[result] = ' ';
					strbuf[result + 1] = '\0';
				}
				ImVec4* color;
				if (frameAdvantage > 0) {
					color = &GREEN_COLOR;
				} else if (frameAdvantage < 0) {
					color = &RED_COLOR;
				} else {
					color = nullptr;
				}
				outlinedTextRaw(drawFramebars_drawList, textPos, strbuf, color, nullptr, true);
				if (landingFrameAdvantage != SHRT_MIN) {
					textSize = ImGui::CalcTextSize(strbuf);
					textPos.x += textSize.x;
					outlinedTextRaw(drawFramebars_drawList, textPos, "(", nullptr, nullptr, true);
					textSize = ImGui::CalcTextSize("(");
					textPos.x += textSize.x;
					frameAdvantageTextFormat(landingFrameAdvantage, strbuf, sizeof strbuf);
					if (landingFrameAdvantage > 0) {
						color = &GREEN_COLOR;
					} else if (landingFrameAdvantage < 0) {
						color = &RED_COLOR;
					} else {
						color = nullptr;
					}
					outlinedTextRaw(drawFramebars_drawList, textPos, strbuf, color, nullptr, true);
					textSize = ImGui::CalcTextSize(strbuf);
					textPos.x += textSize.x;
					outlinedTextRaw(drawFramebars_drawList, textPos, ")", nullptr, nullptr, true);
				}
			}
			if (scaledText) {
				ImGui::SetWindowFontScale(scale);
			}
		}
		
		if (framesXEndMask > framesXMask && framesXEnd > framesX) {
			drawFramebars_drawList->AddRectFilled(
				{ framesXMask, drawFramebars_y - outerBorderThickness },
				{ framesXEndMask, drawFramebars_y - outerBorderThickness + oneFramebarHeight },
				ImGui::GetColorU32(IM_COL32(0, 0, 0, 255)));
		}
		
		if (framesXEnd > framesX) {
			
			if (entityFramebar.belongsToPlayer()) {
				drawPlayerFramebar((const PlayerFramebar&)framebar, preppedDims, tintDarker, entityFramebar.playerIndex, skippedFrames,
					entityFramebar.playerIndex == 0 || entityFramebar.playerIndex == 1
						? endScene.players[entityFramebar.playerIndex].charType
						: (CharacterType)-1);
			} else {
				const PlayerFramebars& correspondingPlayersFramebars = endScene.playerFramebars[entityFramebar.playerIndex];
				
				drawProjectileFramebar((const Framebar&)framebar, preppedDims, tintDarker, skippedFrames,
					framebarSettings.neverIgnoreHitstop
						? (const PlayerFramebar&)correspondingPlayersFramebars.getHitstop()
						: (const PlayerFramebar&)correspondingPlayersFramebars.getMain(),
					entityFramebar.playerIndex == 0 || entityFramebar.playerIndex == 1
						? endScene.players[entityFramebar.playerIndex].charType
						: (CharacterType)-1);
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
					drawDigits<PlayerFramebar, PlayerFrame>((const PlayerFramebar&)framebar, preppedDims, frameNumberYTop, frameNumberYBottom);
				} else {
					drawDigits<Framebar, Frame>((const Framebar&)framebar, preppedDims, frameNumberYTop, frameNumberYBottom);
				}
			}
			
			if (showStrikeInvulOnFramebar || showSuperArmorOnFramebar || showThrowInvulOnFramebar || showOTGOnFramebar) {
				
				const float yTopRow = drawFramebars_y - markerPaddingHeight - frameMarkerHeight;
				const float markerEndY = yTopRow + frameMarkerHeight;
				const float yBottomRow = drawFramebars_y + drawFramebars_frameItselfHeight + markerPaddingHeight;
				const float markerBottomEndY = yBottomRow + frameMarkerHeight;
				const float thisMarkerWidthPremult = frameMarkerWidthOriginal / frameWidthOriginal;
				const float powerupWidthPremult = powerupWidthOriginal / frameWidthOriginal;
				
				bool isPlayer = entityFramebar.belongsToPlayer();
				{
					const PlayerFramebar& framebarPlayer = (const PlayerFramebar&)framebar;
					const Framebar& framebarProjectile = (const Framebar&)framebar;
					int internalIndNext = iterateVisualFramesFrom0_getInitialInternalInd();
					int internalInd;
					for (int visualInd = 0; visualInd < drawFramebars_framesCount; ++visualInd) {
						
						internalInd = internalIndNext;
						incrementInternalInd(internalIndNext);
						
						const PlayerFrame& playerFrame = framebarPlayer[internalInd];
						const Frame& projectileFrame = framebarProjectile[internalInd];
						const FrameDims& dims = preppedDims[visualInd];
						
						ImU32 tint = -1;
						if (visualInd > drawFramebars_framebarPositionDisplay) {
							tint = tintDarker;
						}
						
						float thisMarkerWidth = thisMarkerWidthPremult * dims.width;
						float thisMarkerWidthOffset = (thisMarkerWidth - dims.width) * 0.5F;
						ImVec2 markerStart { dims.x - thisMarkerWidthOffset, yTopRow };
						ImVec2 markerEnd { dims.x + dims.width + thisMarkerWidthOffset, markerEndY };
						ImVec2 bottomBarkerStart { markerStart.x, yBottomRow };
						ImVec2 bottomBarkerEnd { markerEnd.x, markerBottomEndY };
						
						if (
								(
									isPlayer
										? playerFrame.strikeInvulInGeneral
										: projectileFrame.type == FT_IDLE_NO_DISPOSE
											&& (entityFramebar.playerIndex == 0 || entityFramebar.playerIndex == 1)
											&& endScene.players[entityFramebar.playerIndex].charType == CHARACTER_TYPE_JACKO
											&& strcmp(projectileFrame.animName, PROJECTILE_NAME_GHOST) == 0
								) && showStrikeInvulOnFramebar
						) {
							drawFramebars_drawList->AddImage((ImTextureID)TEXID_FRAMES,
								markerStart,
								markerEnd,
								strikeInvulMarker.uvStart,
								strikeInvulMarker.uvEnd,
								tint);
							
							markerStart.y += frameMarkerSideHeight;
							markerEnd.y += frameMarkerSideHeight;
						}
						if (isPlayer) {
							if (playerFrame.superArmorActiveInGeneral && showSuperArmorOnFramebar) {
								const FrameMarkerArt& markerArt = frameMarkerArtArray[
										playerFrame.superArmorActiveInGeneral_IsFull
											? MARKER_TYPE_SUPER_ARMOR_FULL
											: MARKER_TYPE_SUPER_ARMOR
								];
								drawFramebars_drawList->AddImage((ImTextureID)TEXID_FRAMES,
									markerStart,
									markerEnd,
									markerArt.uvStart,
									markerArt.uvEnd,
									tint);
							}
							
							if (playerFrame.throwInvulInGeneral && showThrowInvulOnFramebar) {
								
								drawFramebars_drawList->AddImage((ImTextureID)TEXID_FRAMES,
									bottomBarkerStart,
									bottomBarkerEnd,
									throwInvulMarker.uvStart,
									throwInvulMarker.uvEnd,
									tint);
								
								bottomBarkerStart.y -= frameMarkerSideHeight;
								bottomBarkerEnd.y -= frameMarkerSideHeight;
							}
							
							if (playerFrame.OTGInGeneral && showOTGOnFramebar) {
								
								drawFramebars_drawList->AddImage((ImTextureID)TEXID_FRAMES,
									bottomBarkerStart,
									bottomBarkerEnd,
									OTGMarker.uvStart,
									OTGMarker.uvEnd,
									tint);
							}
						} else if (projectileFrame.type == FT_IDLE_PROJECTILE_HITTABLE
								&& (entityFramebar.playerIndex == 0 || entityFramebar.playerIndex == 1)
								&& endScene.players[entityFramebar.playerIndex].charType == CHARACTER_TYPE_DIZZY
								&& showSuperArmorOnFramebar) {
							
							const PlayerFramebars& correspondingPlayersFramebars = endScene.playerFramebars[entityFramebar.playerIndex];
							const PlayerFramebar& correspondingPlayersFramebar =
								framebarSettings.neverIgnoreHitstop
									? (const PlayerFramebar&)correspondingPlayersFramebars.getHitstop()
									: (const PlayerFramebar&)correspondingPlayersFramebars.getMain();
									
							const PlayerFrame& correspondingPlayerFrame = correspondingPlayersFramebar[internalInd];
							if (correspondingPlayerFrame.u.dizzyInfo.shieldFishSuperArmor) {
								const FrameMarkerArt& markerArt = frameMarkerArtArray[MARKER_TYPE_SUPER_ARMOR];
								drawFramebars_drawList->AddImage((ImTextureID)TEXID_FRAMES,
									markerStart,
									markerEnd,
									markerArt.uvStart,
									markerArt.uvEnd,
									tint);
							}
						}
						
						if (isPlayer ? playerFrame.powerup && !playerFrame.dontShowPowerupGraphic : projectileFrame.powerup) {
							float powerupWidthOffset = (powerupWidthPremult * dims.width - dims.width) * 0.5F;
							drawFramebars_drawList->AddImage((ImTextureID)TEXID_FRAMES,
								{
									dims.x - powerupWidthOffset,
									isPlayer && playerFrame.superArmorActiveInGeneral ? yTopRow + frameMarkerSideHeight : yTopRow
								},
								{
									dims.x + dims.width + powerupWidthOffset,
									isPlayer && playerFrame.superArmorActiveInGeneral ? yTopRow + powerupHeight + frameMarkerSideHeight : yTopRow + powerupHeight
								},
								{
									powerupFrame->uStart,
									powerupFrame->vStart
									
								},
								{
									powerupFrame->uEnd,
									powerupFrame->vEnd
								},
								tint);
						}
						
					}
				}
			}
			
			if (showFirstFrames) {
				
				const float firstFrameTopY = drawFramebars_y - outerBorderThickness - firstFrameHeightDiff * 0.5F;
				const float firstFrameBottomY = firstFrameTopY + firstFrameHeightScaled;
				
				if (entityFramebar.belongsToPlayer()) {
					drawFirstFrames<PlayerFramebar, PlayerFrame>((const PlayerFramebar&)framebar, preppedDims, firstFrameTopY, firstFrameBottomY);
				} else {
					drawFirstFrames<Framebar, Frame>((const Framebar&)framebar, preppedDims, firstFrameTopY, firstFrameBottomY);
				}
			}
		}
		
		drawFramebars_y += oneFramebarHeight + paddingBetweenFramebars;
		if (needShowFramebarFrameDataP2
				&& entityFramebar.belongsToPlayer()
				&& entityFramebar.playerIndex == 1) {
			lastWasP2Framedata = true;
			currentPositionHighlighter_Strip1_EndY = drawFramebars_y;  // leave paddingBetweenFramebars in
		}
	}
	
	const int highlighterStripCount = has2StripsOfCurrentPositionHighlighter ? 2 : 1;
	struct HighlighterStartEnd {
		float start;
		float end;
	};
	HighlighterStartEnd curPosStripRaw[2];
	if (!has2StripsOfCurrentPositionHighlighter) {
		curPosStripRaw[0] = {
			currentPositionHighlighter_Strip1_StartY,
			drawFramebars_y
		};
	} else {
		curPosStripRaw[0] = {
			currentPositionHighlighter_Strip1_StartY,
			currentPositionHighlighter_Strip1_EndY
		};
		curPosStripRaw[1] = {
			currentPositionHighlighter_Strip2_StartY,
			drawFramebars_y
		};
	}
	
	HighlighterStartEnd highlighterStartEnd[2];
	for (int i = 0; i < highlighterStripCount; ++i) {
		highlighterStartEnd[i].start = curPosStripRaw[i].start
					- outerBorderThickness
					- framebarCurrentPositionHighlighterStickoutDistance;
		
		highlighterStartEnd[i].end = curPosStripRaw[i].end
					- outerBorderThickness
					- paddingBetweenFramebars
					+ framebarCurrentPositionHighlighterStickoutDistance;
	}
	
	float windowViewableRegionStartY = drawFramebars_windowPos.y;
	float windowViewableRegionEndY = drawFramebars_windowPos.y + windowHeight;
	
	if (framesXEnd > framesX) {
		
		for (int i = 0; i < highlighterCount; ++i) {
			for (int j = 0; j < highlighterStripCount; ++j) {
				drawFramebars_drawList->AddRectFilled(
					{
						highlighterStartX[i],
						highlighterStartEnd[j].start
					},
					{
						highlighterStartX[i] + highlighterWidth,
						highlighterStartEnd[j].end
					},
					ImGui::GetColorU32(IM_COL32(255, 255, 255, 255)));
			}
		}
		
		bool needInitStitchParams = true;
		static const float distanceBetweenStitches = 4.F;
		static const float stitchSize = 3.F;
		static int visiblePreviousStitchesAtTheTopOfAStitch = -1;
		static const float stitchThickness = 1.F;
		struct StitchParams {
			float startY;
			int count;
		};
		StitchParams stitchParams[2];
		bool showFramebarHatchedLineWhenSkippingGrab = settings.showFramebarHatchedLineWhenSkippingGrab;
		bool showFramebarHatchedLineWhenSkippingHitstop = settings.showFramebarHatchedLineWhenSkippingHitstop;
		bool showFramebarHatchedLineWhenSkippingSuperfreeze = settings.showFramebarHatchedLineWhenSkippingSuperfreeze;
		
		int internalIndNext = iterateVisualFramesFrom0_getInitialInternalInd();
		int internalInd;
		
		for (int visualInd = 0; visualInd < drawFramebars_framesCount; ++visualInd) {
			
			internalInd = internalIndNext;
			incrementInternalInd(internalIndNext);
			
			const SkippedFramesInfo& skippedInfo = skippedFrames[internalInd];
			if (!skippedInfo.count) {
				continue;
			}
			SkippedFramesType skippedType = skippedInfo.elements[skippedInfo.count - 1].type;
			if (!(
				skippedInfo.overflow || (
					skippedType == SKIPPED_FRAMES_GRAB || skippedType == SKIPPED_FRAMES_SUPER
				)
				&& showFramebarHatchedLineWhenSkippingGrab
				|| skippedType == SKIPPED_FRAMES_HITSTOP
				&& showFramebarHatchedLineWhenSkippingHitstop
				|| skippedType == SKIPPED_FRAMES_SUPERFREEZE
				&& showFramebarHatchedLineWhenSkippingSuperfreeze
			)) {
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
				for (int j = 0; j < highlighterStripCount; ++j) {
					StitchParams& params = stitchParams[j];
					params.startY = highlighterStartEnd[j].start;
					float stitchEndYWithWindowClipping = highlighterStartEnd[j].end;
					if (params.startY < windowViewableRegionStartY) {
						float countFitIn = std::floor((windowViewableRegionStartY - params.startY) / distanceBetweenStitches);
						params.startY += countFitIn * distanceBetweenStitches;
					}
					if (stitchEndYWithWindowClipping > windowViewableRegionEndY) {
						stitchEndYWithWindowClipping = windowViewableRegionEndY;
					}
					params.count = (int)std::ceil((stitchEndYWithWindowClipping - params.startY) / distanceBetweenStitches);
					if (params.count <= 0) continue;
					params.startY -= (float)visiblePreviousStitchesAtTheTopOfAStitch * distanceBetweenStitches;
					params.count += visiblePreviousStitchesAtTheTopOfAStitch;
				}
			}
			for (int j = 0; j < highlighterStripCount; ++j) {
				StitchParams& params = stitchParams[j];
				float stitchY = params.startY;
				float stitchX = preppedDims[visualInd].x - stitchSize * 0.5F;
				float stitchEndX = preppedDims[visualInd].x + stitchSize * 0.5F;
				for (int k = 0; k < params.count; ++k) {
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
	}
	drawFramebars_drawList->PopClipRect();
	
	bool clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
	bool mouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
	if (drawFramebars_hoveredFrameIndex != -1) {
		if (!selectingFrames) {
			if (clicked) {
				// This needs to be done to stop the window from getting drag-moved
				io.MouseClicked[0] = false;  // *shaking* I feel violated. This is not good
				selectingFrames = true;
				selectedFrameStart = drawFramebars_hoveredFrameIndex;
				selectedFrameEnd = drawFramebars_hoveredFrameIndex;
			}
		} else if (mouseDown) {
			selectedFrameEnd = drawFramebars_hoveredFrameIndex;
		}
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
		
	} else if (clicked || !(framesXEnd > framesX)) {
		resetFrameSelection();
	} else if (mouseDown && selectingFrames) {
		ImVec2 mousePos = ImGui::GetMousePos();
		selectedFrameEnd = findHoveredFrame(mousePos.x, preppedDims);
	}
	if (!mouseDown && selectingFrames) {
		selectingFrames = false;
	}
	
	if (selectedFrameStart != -1 && selectedFrameEnd != -1) {
		ImVec2 startPos;
		ImVec2 endPos;
		
		float selectionBoxStartY = highlighterStartEnd[0].start + framebarCurrentPositionHighlighterStickoutDistance;
		float selectionBoxEndY = (
				has2StripsOfCurrentPositionHighlighter
					? highlighterStartEnd[1].end
					: highlighterStartEnd[0].end
			) - framebarCurrentPositionHighlighterStickoutDistance;
		
		int selFrameStart;
		int selFrameEnd;
		if (selectedFrameStart <= selectedFrameEnd) {
			selFrameStart = selectedFrameStart;
			selFrameEnd = selectedFrameEnd;
		} else {
			selFrameEnd = selectedFrameStart;
			selFrameStart = selectedFrameEnd;
		}
		
		#pragma warning(suppress:6001)
		if (selFrameStart == 0) {
			startPos.x = framesXMask;
		} else {
			FrameDims& dims = preppedDims[selFrameStart - 1];
			startPos.x = dims.x + dims.width;
		}
		
		if (selectionBoxStartY < drawFramebars_windowPos.y) {
			startPos.y = drawFramebars_windowPos.y;
		} else {
			startPos.y = selectionBoxStartY;
		}
		
		if (selFrameEnd == drawFramebars_framesCount - 1) {
			endPos.x = framesXEndMask;
		} else {
			FrameDims& dims = preppedDims[selFrameEnd + 1];
			endPos.x = dims.x;
		}
		if (selectionBoxEndY > windowViewableRegionEndY) {
			endPos.y = windowViewableRegionEndY;
		} else {
			endPos.y = selectionBoxEndY;
		}
		
		drawFramebars_drawList->PushClipRect(ImVec2{ 0.F, 0.F }, ImVec2{ 10000.F, 10000.F }, false);
		drawFramebars_drawList->AddRect(startPos, endPos, ImGui::GetColorU32(IM_COL32(0, 130, 216, 255)));   // Border
		drawFramebars_drawList->AddRect({ startPos.x - 1.F, startPos.y - 1.F},
			{ endPos.x + 1.F, endPos.y + 1.F},
			ImGui::GetColorU32(IM_COL32(255, 255, 255, 255)));   // Border
		drawFramebars_drawList->AddRect({ startPos.x - 2.F, startPos.y - 2.F},
			{ endPos.x + 2.F, endPos.y + 2.F},
			ImGui::GetColorU32(IM_COL32(0, 130, 216, 255)));   // Border
		drawFramebars_drawList->AddRectFilled(startPos, endPos, ImGui::GetColorU32(IM_COL32(0, 130, 216, 50)));	// Background
		
		ImGui::SetWindowFontScale(1.F);
		const char* txt;
		if (selFrameEnd == selFrameStart) {
			txt = "1 frame selected";
		} else {
			sprintf_s(strbuf, "%d frames selected", selFrameEnd - selFrameStart + 1);
			txt = strbuf;
		}
		ImVec2 textSize = ImGui::CalcTextSize(txt);
		ImVec2 textPos;
		textPos.x = preppedDims[0].x;
		if (drawFramebars_windowPos.y < textSize.y) {
			textPos.y = selectionBoxEndY;
		} else {
			textPos.y = drawFramebars_windowPos.y - textSize.y;
		}
		
		outlinedTextRaw(drawFramebars_drawList, textPos, txt, nullptr, nullptr, true);
		
		drawFramebars_drawList->PopClipRect();
	}
	bool transferHorizScroll = ImGui::IsWindowHovered()
			&& io.MouseWheel != 0.F
			&& io.KeyShift;
	ImGui::End();
	if (needSplitFramebar) {
		copyDrawList(*(ImDrawListBackup*)framebarWindowDrawDataCopy.data(), drawFramebars_drawList);
		drawFramebars_drawList->CmdBuffer.clear();
		drawFramebars_drawList->IdxBuffer.clear();
		drawFramebars_drawList->VtxBuffer.clear();
	}
	
	ImGui::SetNextWindowPos({
			drawFramebars_windowPos.x,
			drawFramebars_windowPos.y - 30.F
		},
		ImGuiCond_Always);
	ImGui::SetNextWindowSize({
			windowWidth,
			30.F
		},
		ImGuiCond_Always);
	ImGui::SetNextWindowContentSize({
			(windowWidth - 16.F)  // this is the maximum content width where we don't get a horizontal scrollbar
				* (float)framebarTotalFramesCapped / framesCountFloat,
			0.F
		});
	float prevScroll = framebarScrollX;
	if (framebarAutoScroll) {
		ImGui::SetNextWindowScroll({
			0.F,
			0.F
		});
	}
	ImGui::Begin("Framebar Horizontal Scrollbar",nullptr,
		ImGuiWindowFlags_NoBackground
		| ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoTitleBar
		| ImGuiWindowFlags_NoBringToFrontOnFocus
		| ImGuiWindowFlags_NoFocusOnAppearing
		| ImGuiWindowFlags_NoSavedSettings
		| ImGuiWindowFlags_HorizontalScrollbar
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoResize);
	framebarScrollX = ImGui::GetScrollX();
	if (framebarAutoScroll && framebarScrollX != prevScroll) {
		framebarAutoScroll = false;
	}
	framebarMaxScrollX = ImGui::GetScrollMaxX();
	if (transferHorizScroll) {
		// this function does not clamp the scroll value, so we can't just grab the new scrollX, we have to wait for the next frame
		ImGui::SetScrollX(ImGui::GetScrollX() - io.MouseWheel * 2 * ImGui::GetFontSize());
		framebarAutoScroll = false;  // we can however makes ourselves aware of user's intention to manually scroll
	}
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	ImGui::End();
	if (needSplitFramebar) {
		copyDrawList(*(ImDrawListBackup*)framebarHorizontalScrollbarDrawDataCopy.data(), drawList);
		drawList->CmdBuffer.clear();
		drawList->IdxBuffer.clear();
		drawList->VtxBuffer.clear();
	}
	#undef pushFramesClipRect
}

// Interject between Win32 backend and ImGui::NewFrame
void UI::interjectIntoImGui() {
	imGuiCorrecter.interjectIntoImGui(screenWidth, screenHeight,
		usePresentRect, presentRectW, presentRectH);
}
