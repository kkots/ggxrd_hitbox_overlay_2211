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
static ImVec4 SLIGHTLY_GRAY = RGBToVec(0xc2c2c2);
static ImVec4 LIGHT_BLUE_COLOR = RGBToVec(0x72bcf2);
static ImVec4 P1_COLOR = RGBToVec(0xff944f);
static ImVec4 P1_OUTLINE_COLOR = RGBToVec(0xd73833);
static ImVec4 P2_COLOR = RGBToVec(0x78d6ff);
static ImVec4 P2_OUTLINE_COLOR = RGBToVec(0x525fdf);
static ImVec4* P1P2_COLOR[2] = { &P1_COLOR, &P2_COLOR };
static ImVec4* P1P2_OUTLINE_COLOR[2] = { &P1_OUTLINE_COLOR, &P2_OUTLINE_COLOR };
static char strbuf[512];
static std::string stringArena;
static char printdecimalbuf[512];
static int numDigits(int num);  // For negative numbers does not include the '-'
struct UVStartEnd { ImVec2 start; ImVec2 end; };
static UVStartEnd digitUVs[10];
struct FrameArt { FrameType type; const PngResource* resource; ImVec2 uvStart; ImVec2 uvEnd; const char* description; };
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
const float innerBorderThickness = 1.F;
const float innerBorderThicknessHalf = innerBorderThickness * 0.5F;
float drawFramebars_frameWidthScaled;
const char* thisHelpTextWillRepeat = "Show available gatlings, whiff cancels, and whether the jump and the special cancels are available,"
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
					"Available cancels may change between hit and whiff, and if the animation is canceled prematurely, not all cancels, that are still there in the move,"
					" may be displayed, because the information is being gathered from the player character directly every frame and not by reading"
					" the move's script (bbscript) ahead or in advance.\n"
					"\n"
					"The move names listed at the top might not match the names you may find when hovering your mouse over frames in the framebar to read their"
					" animation names, because the names here are only updated when a significant enough change in the animation happens.";
static std::string lastNameSuperfreeze;
static std::string lastNameAfterSuperfreeze;

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
static void drawOneLineOnCurrentLineAndTheRestBelow(float wrapWidth, const char* str);
static void printActiveWithMaxHit(const ActiveDataArray& active, const MaxHitInfo& maxHit, int hitOnFrame);
static void drawPlayerIconInWindowTitle(int playerIndex);
static void drawPlayerIconInWindowTitle(GGIcon& icon);
static void printAllCancels(const FrameCancelInfo& cancels,
	bool enableSpecialCancel,
	bool enableJumpCancel,
	bool enableSpecials,
	bool hitOccured,
	bool airborne,
	bool insertSeparators);
static bool prevNamesControl(const PlayerInfo& player, bool includeTitle);
static void headerThatCanBeClickedForTooltip(const char* title, bool* windowVisibilityVar, bool makeTooltip);
void prepareLastNames(const char** lastNames, const PlayerInfo& player);

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
		"Blockstun that can be canceled into some specials: can't perform regular attacks.");
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
	inputNames[INPUT_624624] = { "624624", MOTION };
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
	inputNames[INPUT_646426] = { "646426", MOTION };
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
	imguiActiveTemp = false;
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
				
	if (visible) {
		static std::string windowTitle;
		if (windowTitle.empty()) {
			windowTitle = "ggxrd_hitbox_overlay v";
			windowTitle += VERSION;
		}
		ImGui::Begin(windowTitle.c_str(), &visible);
		
		if (ImGui::CollapsingHeader("Framedata", ImGuiTreeNodeFlags_DefaultOpen)) {
			if (endScene.isIGiveUp()) {
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
				CenterAlignedText("HP");
				AddTooltip("HP (x Guts) [x Defense Modifier]\n"
					"Technically you should divide HP by these values in order to get effective HP, because they're what all damage is multiplied by.");
				
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
						headerThatCanBeClickedForTooltip("Startup", &showStartupTooltip, false);
						if (ImGui::BeginItemTooltip()) {
							ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
							if (settings.dontShowMoveName) {
								if (prevNamesControl(player, true)) {
									printNoWordWrap
									ImGui::Separator();
								}
							}
							ImGui::TextUnformatted("Click the field for tooltip.");
							ImGui::PopTextWrapPos();
							ImGui::EndTooltip();
						}
					}
				}
				for (int i = 0; i < 2; ++i) {
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
						headerThatCanBeClickedForTooltip("Active", &showActiveTooltip, true);
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
						headerThatCanBeClickedForTooltip("Total", &showTotalTooltip, false);
						if (ImGui::BeginItemTooltip()) {
							ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
							if (settings.dontShowMoveName) {
								if (prevNamesControl(player, true)) {
									printNoWordWrap
									ImGui::Separator();
								}
							}
							ImGui::TextUnformatted("Click the field for tooltip.");
							ImGui::PopTextWrapPos();
							ImGui::EndTooltip();
						}
					}
				}
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					ImGui::TableNextColumn();
					player.printInvuls(strbuf, sizeof strbuf);
					printWithWordWrap
					
					if (i == 0) {
						ImGui::TableNextColumn();
						headerThatCanBeClickedForTooltip("Invul", &showInvulTooltip, true);
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
							" than the displayed value due to slowdown.\n"
							"If you land while in blockstun from an air block, instead of your blockstun decrementing by 1, like it"
							" normally would each frame, on the landing frame you instead gain +3 blockstun. So your blockstun is"
							" slightly prolonged when transitioning from air blockstun to ground blockstun.");
					}
				}
				
				const bool dontUsePreBlockstunTime = settings.frameAdvantage_dontUsePreBlockstunTime;
				bool oneWillIncludeParentheses = false;
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					int frameAdvantage = dontUsePreBlockstunTime ? player.frameAdvantageNoPreBlockstun : player.frameAdvantage;
					int landingFrameAdvantage = dontUsePreBlockstunTime ? player.landingFrameAdvantageNoPreBlockstun : player.landingFrameAdvantage;
					if (player.frameAdvantageValid && player.landingFrameAdvantageValid
							&& frameAdvantage != landingFrameAdvantage) {
						oneWillIncludeParentheses = true;
						break;
					}
				}
				
				for (int i = 0; i < 2; ++i) {
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
						headerThatCanBeClickedForTooltip("Frame Adv.", &showFrameAdvTooltip, !oneWillIncludeParentheses);
						if (oneWillIncludeParentheses) {
							AddTooltip(
								"Value in () means frame advantage after landing.\n"
								"\n"
								"Click the field for tooltip.");
						}
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
				if (!settings.dontShowMoveName) {
					for (int i = 0; i < 2; ++i) {
						PlayerInfo& player = endScene.players[i];
						ImGui::TableNextColumn();
						if (prevNamesControl(player, false)) {
							printWithWordWrap
						}
						
						if (i == 0) {
							ImGui::TableNextColumn();
							CenterAlignedText("Move");
							static std::string moveTooltip;
							if (moveTooltip.empty()) {
								moveTooltip = settings.convertToUiDescription("The last performed move, data of which is being displayed in the Startup/Active/Recovery/Total field."
									" If the 'Startup' or 'Total' field is showing multiplie numbers combined with + signs,"
									" all the moves that are included in those fields are listed here as well, combined with + signs or with *X appended to them,"
									" *X denoting how many times that move repeats.\n"
									"The move names might not match the names you may find when hovering your mouse over frames in the framebar to read their"
									" animation names, because the names here are only updated when a significant enough change in the animation happens.\n"
									"\n"
									"To hide this field you can use the \"dontShowMoveName\" setting. Then it will only be shown in the tooltip of 'Startup' and 'Total' fields.");
							}
							AddTooltip(moveTooltip.c_str());
						}
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
				const bool useSlang = settings.useSlangNames;
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					ImGui::TableNextColumn();
					const char* names[3] { nullptr };
					if (player.moveNonEmpty) {
						names[0] = useSlang ? player.move.getDisplayNameSlang(player.idle) : player.move.getDisplayName(player.idle);
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
							if (result != -1) {
								buf += result;
								bufSize -= result;
							}
						}
						isFirst = false;
						result = sprintf_s(buf, bufSize, "%s", name);
						if (result != -1) {
							buf += result;
							bufSize -= result;
						}
					}
					printWithWordWrap
					
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
					sprintf_s(strbuf, "%s", formatBoolean(player.grab));
					printNoWordWrap
					
					if (i == 0) {
						ImGui::TableNextColumn();
						CenterAlignedText("grab");
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
				Entity superflashInstigator = endScene.getLastNonZeroSuperflashInstigator();
				int slowdown = endScene.getRCSlowdownCounter();
				int slowdownMax = endScene.getRCSlowdownCounterMax();
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = endScene.players[i];
					ImGui::TableNextColumn();
					int flashCurrent = 0;
					int flashMax = 0;
					int slowCurrent = !player.immuneToRCSlowdown ? slowdown : 0;
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
						CenterAlignedText("Freeze+RC Slow");
						AddTooltip("Shows superfreeze current/maximum duration in frames, followed by +, followed by"
							" Roman Cancel slowdown duration current/maximum in frames."
							" Both the superfreeze and the Roman Cancel slowdown are always shown."
							" If either is not present at the moment, 0/0 or 0/X is shown in its place."
							" If a player is not affected by the superfreeze or Roman Cancel slowdown, 0/0 or 0/X is shown in the place of that.");
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
									sprintf_s(strbuf, "%d/%d", projectile.hitstop, projectile.hitstopMax);
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
									printActiveWithMaxHit(projectile.actives, projectile.maxHit,
										!projectile.maxHit.empty() && projectile.maxHit.maxUse <= 1 ? 0 : projectile.hitOnFrame);
									
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
			AddTooltip("Displays tension gained from combo and factors that affect tension gain.");
			ImGui::SameLine();
			if (ImGui::Button("Speed/Proration")) {
				showSpeedsData = !showSpeedsData;
			}
			AddTooltip("Display x,y, speed, pushback and protation of hitstun and pushback.");
			for (int i = 0; i < 2; ++i) {
				sprintf_s(strbuf, i == 0 ? "Character Specific (P%d)" : "... (P%d)", i + 1);
				if (i != 0) ImGui::SameLine();
				if (ImGui::Button(strbuf)) {
					showCharSpecific[i] = !showCharSpecific[i];
				}
				AddTooltip("Display of information specific to a character.");
			}
			if (ImGui::Button("Box Extents")) {
				showBoxExtents = !showBoxExtents;
			}
			AddTooltip("Shows the minimum and maximum Y (vertical) extents of hurtboxes and hitboxes of each player."
				" The units are not divided by 100 for viewability.");
			ImGui::SameLine();
			for (int i = 0; i < 2; ++i) {
				sprintf_s(strbuf, "Cancels (P%d)", i + 1);
				if (ImGui::Button(strbuf)) {
					showCancels[i] = !showCancels[i];
				}
				AddTooltip(thisHelpTextWillRepeat);
				if (i == 0) ImGui::SameLine();
			}
		}
		if (ImGui::CollapsingHeader("Hitboxes")) {
			
			booleanSettingPreset(settings.dontShowBoxes);
			
			stateChanged = ImGui::Checkbox("GIF Mode", &gifModeOn) || stateChanged;
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
			HelpMarker(GIFModeHelp.c_str());
			
			stateChanged = ImGui::Checkbox("Black Background", &gifModeToggleBackgroundOnly) || stateChanged;
			ImGui::SameLine();
			static std::string blackBackgroundHelp;
			if (blackBackgroundHelp.empty()) {
				blackBackgroundHelp = settings.convertToUiDescription(
					"Makes background black (and, for screenshotting purposes, - effectively transparent,"
					" if Post Effect is turned off in the game's graphics settings).\n"
					"You can use the \"gifModeToggleBackgroundOnly\" hotkey to toggle this setting.");
			}
			HelpMarker(blackBackgroundHelp.c_str());
			
			bool postEffectOn = game.postEffectOn() != 0;
			if (ImGui::Checkbox("Post-Effect On", &postEffectOn)) {
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
			HelpMarker(postEffectOnHelp.c_str());
			
			stateChanged = ImGui::Checkbox("Camera Center on Player", &gifModeToggleCameraCenterOnly) || stateChanged;
			ImGui::SameLine();
			static std::string cameraCenterHelp;
			if (cameraCenterHelp.empty()) {
				cameraCenterHelp = settings.convertToUiDescription(
					"Centers the camera on you.\n"
					"You can use the \"gifModeToggleCameraCenterOnly\" hotkey to toggle this setting.");
			}
			HelpMarker(cameraCenterHelp.c_str());
			
			stateChanged = ImGui::Checkbox("Camera Center on Opponent", &toggleCameraCenterOpponent) || stateChanged;
			ImGui::SameLine();
			static std::string cameraCenterOpponentHelp;
			if (cameraCenterOpponentHelp.empty()) {
				cameraCenterOpponentHelp = settings.convertToUiDescription(
					"Centers the camera on the opponent.\n"
					"You can use the \"toggleCameraCenterOpponent\" hotkey to toggle this setting.");
			}
			HelpMarker(cameraCenterOpponentHelp.c_str());
			
			stateChanged = ImGui::Checkbox("Hide Opponent", &gifModeToggleHideOpponentOnly) || stateChanged;
			ImGui::SameLine();
			static std::string hideOpponentHelp;
			if (hideOpponentHelp.empty()) {
				hideOpponentHelp = settings.convertToUiDescription(
					"Make the opponent invisible and invulnerable.\n"
					"You can use the \"gifModeToggleHideOpponentOnly\" hotkey to toggle this setting.");
			}
			HelpMarker(hideOpponentHelp.c_str());
			
			stateChanged = ImGui::Checkbox("Hide Player", &toggleHidePlayer) || stateChanged;
			ImGui::SameLine();
			static std::string hidePlayerHelp;
			if (hidePlayerHelp.empty()) {
				hidePlayerHelp = settings.convertToUiDescription(
					"Make the player invisible and invulnerable.\n"
					"You can use the \"toggleHidePlayer\" hotkey to toggle this setting.");
			}
			HelpMarker(hidePlayerHelp.c_str());
			
			stateChanged = ImGui::Checkbox("Hide HUD", &gifModeToggleHudOnly) || stateChanged;
			ImGui::SameLine();
			static std::string hideHUDHelp;
			if (hideHUDHelp.empty()) {
				hideHUDHelp = settings.convertToUiDescription(
					"Hides the HUD.\n"
					"You can use the \"gifModeToggleHudOnly\" hotkey to toggle this setting.");
			}
			HelpMarker(hideHUDHelp.c_str());
			
			stateChanged = ImGui::Checkbox("No Gravity", &noGravityOn) || stateChanged;
			ImGui::SameLine();
			static std::string noGravityHelp;
			if (noGravityHelp.empty()) {
				noGravityHelp = settings.convertToUiDescription(
					"Prevents you from falling, meaning you remain in the air as long as 'No Gravity Mode' is enabled.\n"
					"You can use the \"noGravityToggle\" hotkey to toggle this setting.");
			}
			HelpMarker(noGravityHelp.c_str());
			
			bool neverDisplayGrayHurtboxes = settings.neverDisplayGrayHurtboxes;
			if (ImGui::Checkbox("Disable Gray Hurtboxes", &neverDisplayGrayHurtboxes)) {
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
			HelpMarker(neverDisplayGrayHurtboxesHelp.c_str());
			
			stateChanged = ImGui::Checkbox("Freeze Game", &freezeGame) || stateChanged;
			ImGui::SameLine();
			if (ImGui::Button("Next Frame")) {
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
					" instead of pressing the button.");
			}
			HelpMarker(freezeGameHelp.c_str());
			
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
			static std::string slowmoHelp;
			if (slowmoHelp.empty()) {
				slowmoHelp = settings.convertToUiDescription(
					"Makes the game run slower, advancing only on every second, every third and so on frame, depending on 'Slow-Mo Factor' field.\n"
					"You can use the \"slowmoGameToggle\" shortcut to toggle slow-mo on and off.");
			}
			HelpMarker(slowmoHelp.c_str());
			
			ImGui::Button("Take Screenshot");
			if (ImGui::IsItemActivated()) {
				// Regular ImGui button 'press' (ImGui::Button(...) function returning true) happens when you RELEASE the button,
				// but to simulate the old keyboard behavior we need this to happen when you PRESS the button
				takeScreenshotPress = true;
				takeScreenshotTimer = 10;
			}
			takeScreenshotTemp = ImGui::IsItemActive();
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
			HelpMarker(screenshotHelp.c_str());
			
			stateChanged = ImGui::Checkbox("Continuous Screenshotting Mode", &continuousScreenshotToggle) || stateChanged;
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
			HelpMarker(continuousScreenshottingHelp.c_str());
			
		}
		if (ImGui::CollapsingHeader("Settings")) {
			if (ImGui::CollapsingHeader("Hitbox Settings")) {
				
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
				AddTooltip("Restores the default values for the four settings above.");
			}
			if (ImGui::CollapsingHeader("Framebar Settings")) {
				
				if (ImGui::Button("Framebar Help")) {
					showFramebarHelp = !showFramebarHelp;
				}
				AddTooltip("Shows the meaning of each frame color/graphic on the framebar.");
				
				booleanSettingPreset(settings.neverIgnoreHitstop);
				
				booleanSettingPreset(settings.ignoreHitstopForBlockingBaiken);
				
				booleanSettingPreset(settings.considerRunAndWalkNonIdle);
				
				booleanSettingPreset(settings.considerCrouchNonIdle);
				
				booleanSettingPreset(settings.considerKnockdownWakeupAndAirtechIdle);
				
				booleanSettingPreset(settings.considerIdleInvulIdle);
				
				booleanSettingPreset(settings.considerDummyPlaybackNonIdle);
				
				booleanSettingPreset(settings.useColorblindHelp);
				
				booleanSettingPreset(settings.showFramebar);
				
				booleanSettingPreset(settings.showFramebarInTrainingMode);
				
				booleanSettingPreset(settings.showFramebarInReplayMode);
				
				booleanSettingPreset(settings.showFramebarInOtherModes);
				
				booleanSettingPreset(settings.closingModWindowAlsoHidesFramebar);
				
				booleanSettingPreset(settings.showStrikeInvulOnFramebar);
				
				booleanSettingPreset(settings.showThrowInvulOnFramebar);
				
				booleanSettingPreset(settings.showSuperArmorOnFramebar);
				
				booleanSettingPreset(settings.showFirstFramesOnFramebar);
				
				booleanSettingPreset(settings.considerSimilarFrameTypesSameForFrameCounts);
				
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
				
				ImGui::PushID(1);
				lowProfileControl();
				ImGui::PopID();
				
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
				keyComboControl(settings.toggleNeverIgnoreHitstop);
			}
			if (ImGui::CollapsingHeader("General Settings")) {
				booleanSettingPreset(settings.modWindowVisibleOnStart);
				
				ImGui::PushID(1);
				booleanSettingPreset(settings.closingModWindowAlsoHidesFramebar);
				ImGui::PopID();
				
				booleanSettingPreset(settings.displayUIOnTopOfPauseMenu);
				
				lowProfileControl();
				
				booleanSettingPreset(settings.frameAdvantage_dontUsePreBlockstunTime);
				
				ImGui::PushID(1);
				booleanSettingPreset(settings.useSlangNames);
				ImGui::PopID();
				
				booleanSettingPreset(settings.dontShowMoveName);
				
			}
		}
		ImGui::End();
		if (showTensionData) {
			ImGui::Begin("Tension Data", &showTensionData);
			if (endScene.isIGiveUp()) {
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
			
			if (endScene.isIGiveUp()) {
				ImGui::TextUnformatted("Online non-observer match running.");
			} else
			if (ImGui::BeginTable("##TensionData",
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
					"Multiply base hitstun by the percentage using floating point math, then round down to get prorated hitstun.\n"
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
				sprintf_s(strbuf, "  Character Specific (P%d)", i + 1);
				ImGui::Begin(strbuf, showCharSpecific + i);
				const PlayerInfo& player = endScene.players[i];
				
				GGIcon scaledIcon;
				if (player.charType == CHARACTER_TYPE_SOL && player.playerval0) {
					scaledIcon = scaleGGIconToHeight(DISolIconRectangular, 14.F);
				} else {
					scaledIcon = scaleGGIconToHeight(getPlayerCharIcon(i), 14.F);
				}
				drawPlayerIconInWindowTitle(scaledIcon);
				
				if (!*aswEngine || !player.pawn) {
					ImGui::TextUnformatted("Match not running");
					ImGui::End();
					continue;
				} else if (endScene.isIGiveUp()) {
					ImGui::TextUnformatted("Online non-observer match running.");
					ImGui::End();
					continue;
				}
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
						printActiveWithMaxHit(player.eddie.actives, player.eddie.maxHit, player.eddie.hitOnFrame);
						float w = ImGui::CalcTextSize(strbuf).x;
						if (w > ImGui::GetContentRegionAvail().x) {
							ImGui::TextWrapped("%s", strbuf);
						} else {
							ImGui::TextUnformatted(strbuf);
						}
						
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
				} else if (player.charType == CHARACTER_TYPE_CHIPP) {
					if (player.playerval0) {
						printChippInvisibility(player.playerval0, player.maxDI);
					} else {
						ImGui::TextUnformatted("Not invisible");
					}
					if (player.move.caresAboutWall) {
						ImGui::Text("Wall time: %d/120", player.pawn.mem54());
					} else {
						ImGui::TextUnformatted("Not on a wall.");
					}
				} else if (player.charType == CHARACTER_TYPE_FAUST) {
					const PlayerInfo& otherPlayer = endScene.players[1 - player.index];
					if (!otherPlayer.poisonDuration) {
						ImGui::TextUnformatted("Opponent not poisoned.");
					} else {
						sprintf_s(strbuf, "Poison duration on opponent: %d/%d", otherPlayer.poisonDuration, otherPlayer.poisonDurationMax);
						ImGui::TextUnformatted(strbuf);
					}
				} else {
					ImGui::TextUnformatted("No character specific information to show.");
				}
				ImGui::End();
			}
		}
		if (showBoxExtents) {
			ImGui::Begin("Box Extents", &showBoxExtents);
			if (endScene.isIGiveUp()) {
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
							"The coordinates shown are relative to the global space.\n"
							"If the coordinates are not shown while an attack is out, that means that attack is a projectile."
							" To view projectiles' hitbox extents you can see 'Projectiles' in the main UI window.");
					}
				}
				ImGui::EndTable();
			}
			ImGui::End();
		}
		for (int i = 0; i < 2; ++i) {
			if (showCancels[i]) {
				sprintf_s(strbuf, "  Cancels (P%d)", i + 1);
				ImGui::SetNextWindowSize({
					ImGui::GetFontSize() * 35.F,
					150.F
				}, ImGuiCond_FirstUseEver);
				ImGui::Begin(strbuf, showCancels + i);
				drawPlayerIconInWindowTitle(i);
				
				const float wrapWidth = ImGui::GetContentRegionAvail().x;
				ImGui::PushTextWrapPos(wrapWidth);
				
				const PlayerInfo& player = endScene.players[i];
				
				const bool useSlang = settings.useSlangNames;
				const char* lastNames[2];
				prepareLastNames(lastNames, player);
				int animNamesCount = player.prevStartupsDisp.countOfNonEmptyUniqueNames(lastNames,
					player.superfreezeStartup ? 2 : 1,
					useSlang);
				ImGui::PushStyleVarX(ImGuiStyleVar_ItemSpacing, 0.F);
				textUnformattedColored(YELLOW_COLOR, animNamesCount ? "Anims: " : "Anim: ");
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
						cancels.hitOccured,
						cancels.airborne,
						false);
				}
				if (!printedSomething) {
					ImGui::TextUnformatted("No cancels available.");
				}
				
				ImGui::PopTextWrapPos();
				ImGui::PushID(i);
				GGIcon scaledIcon = scaleGGIconToHeight(tipsIcon, 14.F);
				drawGGIcon(scaledIcon);
				ImGui::PopID();
				AddTooltip(thisHelpTextWillRepeat);
				ImGui::End();
			}
		}
		if (showLowProfilePresets) {
			lowProfilePresetsWindow();
		}
		if (showFramebarHelp) {
			framebarHelpWindow();
		}
		if (showErrorDialog && errorDialogText && *errorDialogText != '\0') {
			ImGui::SetNextWindowPos(*(ImVec2*)errorDialogPos, ImGuiCond_Appearing);
			ImGui::Begin("Error", &showErrorDialog);
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			ImGui::TextUnformatted(errorDialogText);
			ImGui::PopTextWrapPos();
			ImGui::End();
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
				" (called frameAdvantage_dontUsePreBlockstunTime in the INI file).");
			ImGui::End();
		}
		if (showStartupTooltip) {
			ImGui::SetNextWindowSize({ 500.F, 0.F }, ImGuiCond_FirstUseEver);
			ImGui::Begin("'Startup' Field Help", &showStartupTooltip);
			ImGui::TextWrapped("%s",
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
				"6) Baiken canceling Azami into another Azami or the follow-ups, causes them to be displayed in addition to what happened"
				" before, over a + sign;\n"
				"7) Some other moves may get combined with the ones they were performed from as well, using the + sign.");
			ImGui::End();
		}
		if (showActiveTooltip) {
			ImGui::SetNextWindowSize({ 500.F, 0.F }, ImGuiCond_FirstUseEver);
			ImGui::Begin("'Active' Field Help", &showActiveTooltip);
			ImGui::TextWrapped("%s",
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
				" will display 0 active frames during superfreeze or on the frame after it.");
			ImGui::End();
		}
		if (showTotalTooltip) {
			ImGui::SetNextWindowSize({ 500.F, 0.F }, ImGuiCond_FirstUseEver);
			ImGui::Begin("'Total' Field Help", &showTotalTooltip);
			ImGui::TextWrapped("%s",
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
				"If you performed an air normal or similar air move without landing recovery, and it got canceled by"
				" landing, normally there's 1 frame upon landing during which normals can't be used but blocking is possible."
				" This frame is not included in the total frames as it is not considered part of the move.\n"
				"\n"
				"If the move recovery lets you attack first and then some times passes and then it lets you block, or vice versa"
				" the display will say either 'X can't block+Y can't attack' or 'X can't attack+Y can't block'. In this case"
				" the first part is the number of frames during which you were unable to block/attack and the second part is"
				" the number of frames during which you were unable to attack/block.\n"
				"\n"
				"If the move was jump canceled, the prejump frames and the jump are not included in neither the recovery nor 'Total'.\n"
				"\n"
				"If the move started up during superfreeze, the startup+active+recovery will be = total+1 (see tooltip of 'Active').");
			ImGui::End();
		}
		if (showInvulTooltip) {
			ImGui::SetNextWindowSize({ 500.F, 0.F }, ImGuiCond_FirstUseEver);
			ImGui::Begin("Invul Help", &showInvulTooltip);
			ImGui::TextWrapped("%s",
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
				"Low profile invul can be configured using Settings - General Settings - Low Profile Cut-Off Height");
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
	static const float inverse = 1.F / 255.F;
	return {
		(float)((color >> 16) & 0xff) * inverse,  // red
		(float)((color >> 8) & 0xff) * inverse,  // green
		(float)(color & 0xff) * inverse,  // blue
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

void UI::addFrameArt(HINSTANCE hModule, FrameType frameType, WORD resourceIdBothVersions, std::unique_ptr<PngResource>& resourceBothVersions, const char* description) {
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
                 WORD resourceIdNonColorblind, std::unique_ptr<PngResource>& resourceNonColorblind, const char* description) {
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
			if (result != -1) {
				buf += result;
				bufSize -= result;
			}
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
			if (result != -1) {
				buf += result;
				bufSize -= result;
			}
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
		if (result != -1) {
			buf += result;
			bufSize -= result;
		}
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
			if (result != -1) {
				buf += result;
				bufSize -= result;
			}
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
	if (settings.showFramebar && (!settings.closingModWindowAlsoHidesFramebar || visible)) {
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
		if (result != -1) {
			buf += result;
			bufSize -= result;
		}
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
				if (result != -1) {
					buf += result;
					bufSize -= result;
				}
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
			if (result != -1) {
				buf += result;
				bufSize -= result;
			}
			isFirstCondition = false;
		}
		if (cancel.move->minimumHeightRequirement) {
			ui.printDecimal(cancel.move->minimumHeightRequirement, 2, 0);
			result = sprintf_s(buf, bufSize, "%sminimum height requirement: %s",
				isFirstCondition ? " (" : ", ",
				printdecimalbuf);
			if (result != -1) {
				buf += result;
				bufSize -= result;
			}
			isFirstCondition = false;
		}
		if (!isFirstCondition) {
			result = sprintf_s(buf, bufSize, ")");
			if (result != -1) {
				buf += result;
				bufSize -= result;
			}
		}
		if (cancel.bufferTime && cancel.bufferTime != 3) {
			result = sprintf_s(buf, bufSize, " (%df buffer)", cancel.bufferTime);
			if (result != -1) {
				buf += result;
				bufSize -= result;
			}
		}
		ImGui::Text("%d) %s;", counter++, strbuf);
	}
	return counter - 1;
}

void UI::framebarHelpWindow() {
	ImGui::SetNextWindowSize({ ImGui::GetFontSize() * 35.0f + 16.F, 0.F }, ImGuiCond_FirstUseEver);
	ImGui::Begin("Framebar Help", &showFramebarHelp);
	float wordWrapWidth = ImGui::GetContentRegionAvail().x;
	ImGui::PushTextWrapPos(wordWrapWidth);
	
	static bool framebarHelpContentGenerated = false;
	struct FramebarHelpElement {
		const FrameArt* art[2];
		std::vector<const char*> meanings;
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
				newHelp.meanings.emplace_back(art.description);
			} else {
				found->meanings.emplace_back(art.description);
			}
		}
	}
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
		for (const char* description : elem.meanings) {
			if (count != 1) {
				ImGui::SetCursorPosX(cursorX);
			}
			if (theresOnlyOne) {
				ImGui::TextUnformatted(description);
			} else {
				ImGui::Text("%d) %s", count++, description);
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
	ImGui::TextUnformatted("A half-filled active frame means an attack's startup or active frame which first begins duing"
		" a superfreeze.");
	
	ImGui::Separator();
	
	const FrameMarkerArt* frameMarkerArtArray = settings.useColorblindHelp ? frameMarkerArtColorblind : frameMarkerArtNonColorblind;
	float frameMarkerVHeight = frameMarkerHeightOriginal / (float)strikeInvulFrame->height
		* (strikeInvulFrame->vEnd - strikeInvulFrame->vStart);
	
	struct MarkerHelpInfo {
		FrameMarkerType type;
		bool isOnTheBottom;
		const char* description;
	};
	MarkerHelpInfo markerHelps[] {
		{ MARKER_TYPE_STRIKE_INVUL, false, "Strike invulnerability." },
		{ MARKER_TYPE_THROW_INVUL, true, "Throw invulnerability." },
		{ MARKER_TYPE_SUPER_ARMOR, false, "Super armor, parry, azami,"
			" projectile-only invulnerability, reflect or flick, or a combination of those." },
		{ MARKER_TYPE_SUPER_ARMOR_FULL, false, "Red Blitz charge super armor or air Blitz super armor." }
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
		ImGui::TextUnformatted(info.description);
	}
	
	ImGui::Separator();
	
	ImGui::Image((ImTextureID)TEXID_FRAMES,
		{ frameWidthOriginal, firstFrameHeight },
		{ firstFrame->uStart, firstFrame->vStart },
		{ firstFrame->uEnd, firstFrame->vEnd });
	ImGui::SameLine();
	ImGui::TextUnformatted("A first frame, denoting the start of a new animation."
		" If the animation didn't change, may mean transition to some new state in the animation."
		" For blockstun and hitstun may mean leaving hitstop or re-entering hitstun/blockstun/hitstop.\n"
		"Some animation changes are intentionally not shown.");
	
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
			" The numbers do not reset (do not restart counting) when a \"next hit\" active frame (^img1;) is encountered."
			" The \"considerSimilarFrameTypesSameForFrameCounts\" setting may help broaden the range of situations where the numbers get reset (restart counting)."
			" By default it is set to true, meaning similar frame types of startup or recovery are counted as one range of frames."
			" When the number of frames is double or triple digit"
			" and does not fit, it may be broken up into 1-2 digit on one side + 1-2 digits on the other side. The way you should"
			" read this is as one whole number: the pieces on the right are the higher digits, and pieces on the left are the lower ones."
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
	imGuiDrawWrappedTextWithIcons(generalFramebarHelp.c_str(),
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
    /*
			ImGui::Image((ImTextureID)TEXID_FRAMES,
				{ frameWidthOriginal, frameHeightOriginal },
				newHitArt->uvStart,
				newHitArt->uvEnd);*/
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

void UI::lowProfilePresetsWindow() {
	
	static AttackValuePreset presets[] {
		{ "Sol c.S: 107000", 107000 },
		{ "Sol f.S: 133000", 133000 },
		{ "Sol DAA/6P: 129000", 129000 },
		{ "Sol Tyrant Rave (Normal/DI): 129000", 129000 },
		{ "Ky f.S: 175000", 175000 },
		{ "Ky DAA/6P: 150000", 150000 },
		{ "Ky SE: 184000", 184000 },
		{ "Ky CSE: 178000", 178000 },
		{ "Ky DCCSE Main Shaft: 171000", 171000 },
		{ "Ky DCCSE Base Ring: 136000", 136000 },
		{ "Ky Lowest Descending j.D: 160951", 160951 },
		{ "Ky tk j.D: 247226", 247226 },
		{ "May c.S: 110000", 110000 },
		{ "May f.S: 88000", 88000 },
		{ "May DAA/6P: 180000", 180000 },
		{ "May Ultimate Whiner: 179000", 179000 },
		{ "May Ultimate Spinning Whirlwind: 133000", 133000 },
		{ "May Mr. Dolphin Horizontal (S): 113025", 113025 },
		{ "May Mr. Dolphin Horizontal (H): 161500", 161500 },
		{ "Millia c.S: 98000", 98000 },
		{ "Millia f.S: 103000", 103000 },
		{ "Millia 6P: 126000", 126000 },
		{ "Millia 5H: 157000", 157000 },
		{ "Millia DAA: 194000", 194000 },
		{ "Millia Tandem Top (S): 225000", 225000 },
		{ "Millia Tandem Top (H) (Lowest Possible): 208900", 208900 },
		{ "Zato c.S: 184000", 184000 },
		{ "Zato f.S: 220000", 220000 },
		{ "Zato 6P: 166000", 166000 },
		{ "Zato DAA/2H: 217000", 217000 },
		{ "Potemkin c.S: 28000", 28000 },
		{ "Potemkin f.S: 220000", 220000 },
		{ "Potemkin 6P: 264000", 264000 },
		{ "Potemkin 5H: 133000", 133000 },
		{ "Potemkin 6H: 159000", 159000 },
		{ "Potemkin DAA/5D: 178000", 178000 },
		{ "Potemkin Hammerfall: 132000", 132000 },
		{ "Potemkin Giganter Kai: 73000", 73000 },
		{ "Potemkin F.D.B.: 165000", 165000 },
		{ "Potemkin F.D.B. Reflected Projectile: 152000", 152000 },
		{ "Chipp 5K: 140000", 140000 },
		{ "Chipp c.S: 123000", 123000 },
		{ "Chipp f.S: 143000", 143000 },
		{ "Chipp 5H/DAA: 202000", 202000 },
		{ "Chipp 6P: 161000", 161000 },
		{ "Chipp tk Alpha Blade: 51751", 51751 },
		{ "Chipp Zansei Rouga: 130250", 130250 },
		{ "Faust 5P: 215000", 215000 },
		{ "Faust f.S: 221000", 221000 },
		{ "Faust 5H: 205000", 205000 },
		{ "Faust 6P/DAA: 161000", 161000 },
		{ "Faust Re-re-re Thrust: 209000", 209000 },
		{ "Faust Drill (Lowest Possible): 182000", 182000 },
		{ "Axl 5K: 163000", 163000 },
		{ "Axl c.S: 107000", 107000 },
		{ "Axl f.S: 133000", 133000 },
		{ "Axl 6P/DAA: 244000", 244000 },
		{ "Venom 5K/DAA: 91000", 91000 },
		{ "Venom c.S: 137000", 137000 },
		{ "Venom f.S: 184000", 184000 },
		{ "Venom 5H: 185000", 185000 },
		{ "Venom 6P: 273000", 273000 },
		{ "Venom P Ball hit by 2P (Lowest Possible): 133875", 133875 },
		{ "Venom Stinger Aim (S/H): 183000", 183000 },
		{ "Slayer 5K: 184000", 184000 },
		{ "Slayer 6P/DAA: 85000", 85000 },
		{ "Slayer c.S: 121000", 121000 },
		{ "Slayer f.S: 149000", 149000 },
		{ "Slayer 5H: 78000", 78000 },
		{ "Slayer Mappa Hunch: 154000", 154000 },
		{ "Slayer Under Pressure: 100000", 100000 },
		{ "Slayer Pilebunker: 112000", 112000 },
		{ "Slayer Dead on Time: 126500", 126500 },
		{ "Slayer Eternal Wings: 107500", 107500 },
		{ "I-No 5K: 187000", 187000 },
		{ "I-No c.S: 116000", 116000 },
		{ "I-No f.S: 209000", 209000 },
		{ "I-No 6P/DAA: 147000", 147000 },
		{ "I-No Longing Desperation: 119000", 119000 },
		{ "I-No Chemical Love (Horizontal) (Lowest Possible): 299410", 299410 },
		{ "I-No Antidepressant Scale: 251000", 251000 },
		{ "Bedman 5K/DAA: 117000", 117000 },
		{ "Bedman 6P: 172000", 172000 },
		{ "Bedman c.S: 61000", 61000 },
		{ "Bedman f.S: 109000", 109000 },
		{ "Bedman Task A/A' (First Frame): 193000", 193000 },
		{ "Ramlethal DAA: 107000", 107000 },
		{ "Ramlethal 5P: 220000", 220000 },
		{ "Ramlethal 5K: 163000", 163000 },
		{ "Ramlethal c.S (With/Without Sword): 97000", 97000 },
		{ "Ramlethal f.S: 123820", 123820 },
		{ "Ramlethal 6P: 193000", 193000 },
		{ "Ramlethal f.S (No Sword): 174000", 174000 },
		{ "Ramlethal 5H (No Sword): 95000", 95000 },
		{ "Ramlethal Launch Greatsword (S): 92000", 92000 },
		{ "Ramlethal Launch Greatsword (H): 169000", 169000 },
		{ "Ramlethal Dauro: 127000", 127000 },
		{ "Sin 5K: 169000", 169000 },
		{ "Sin 6P: 224000", 224000 },
		{ "Sin f.S/DAA: 175000", 175000 },
		{ "Sin 5H (First Hit. Hard Whiffs on Crouching): 260000", 260000 },
		{ "Sin Beak Driver (Lowest Possible): 170000", 170000 },
		{ "Sin Beak Driver (Max Charge): 90000", 90000 },
		{ "Sin Voltec Dein: 218000", 218000 },
		{ "Elphelt c.S: 52000", 52000 },
		{ "Elphelt f.S: 159000", 159000 },
		{ "Elphelt 5H (Whiffs on Crouching): 244000", 244000 },
		{ "Elphelt Bridal Express: 52000", 52000 },
		{ "Elphelt sg.H (Not Max Charge): 41000", 41000 },
		{ "Elphelt sg.P: 112000", 112000 },
		{ "Elphelt sg.S: 47000", 47000 },
		{ "Elphelt DAA/6P: 212000", 212000 },
		{ "Elphelt j.D YRC (Lowest Possible): 138000", 138000 },
		{ "Leo 5K: 147000", 147000 },
		{ "Leo c.S: 148000", 148000 },
		{ "Leo f.S: 158000", 158000 },
		{ "Leo 5H: 145000", 145000 },
		{ "Leo 6P: 196000", 196000 },
		{ "Leo Kaltes Gest\xc3\xb6\x62\x65r Erst: 133000", 133000 },
		{ "Leo Kaltes Gest\xc3\xb6\x62\x65r Zweit: 121000", 121000 },
		{ "Leo bt.P: 142000", 142000 },
		{ "Leo bt.S: 108000", 108000 },
		{ "Leo bt.H: 112000", 112000 },
		{ "Leo Graviert W\xc3\xbcrde (S): 113000", 113000 },
		{ "Leo Graviert W\xc3\xbcrde (H): 115000", 115000 },
		{ "Leo Leidenschaft Dirigent: 188000", 188000 },
		{ "Leo DAA/5D: 131000", 131000 },
		{ "Johnny 5K: 180000", 180000 },
		{ "Johnny 6K: 189000", 189000 },
		{ "Johnny 6P/DAA: 222000", 222000 },
		{ "Johnny c.S: 66000", 66000 },
		{ "Johnny f.S: 191000", 191000 },
		{ "Johnny 5H: 56000", 56000 },
		{ "Johnny 6H: 97000", 97000 },
		{ "Johnny 2H: 44951", 44951 },
		{ "Johnny K Mist Finer (Lv1, Lv2): 210000", 210000 },
		{ "Johnny K Mist Finer (Lv3): 204000", 204000 },
		{ "Johnny That's My Name: 187000", 187000 },
		{ "Jack O' 5K: 173000", 173000 },
		{ "Jack O' c.S: 70000", 70000 },
		{ "Jack O' f.S: 106000", 106000 },
		{ "Jack O' 5H (First Frame of First Hit): 117000", 117000 },
		{ "Jack O' 4D: 152000", 152000 },
		{ "Jack O' 6P/DAA: 166000", 166000 },
		{ "Jack O' 5D: 72000", 72000 },
		{ "Jack O' 6K: 99000", 99000 },
		{ "Jam 5K: 61000", 61000 },
		{ "Jam c.S: 98000", 98000 },
		{ "Jam f.S/DAA: 134000", 134000 },
		{ "Jam 5H (First Hit): 164000", 164000 },
		{ "Jam 6H: 92000", 92000 },
		{ "Jam 2H (First Hit): 160000", 160000 },
		{ "Jam Hyappo Shinshou/Senri Shinshou: 81000", 81000 },
		{ "Jam Choukyaku Hou'oushou: 27000", 27000 },
		{ "Jam Bao Saishinshou: 89000", 89000 },
		{ "Haehyun 5K/DAA (Lower Hitbox): 89000", 89000 },
		{ "Haehyun 6P: 198000", 198000 },
		{ "Haehyun c.S: 56000", 56000 },
		{ "Haehyun f.S: 167000", 167000 },
		{ "Haehyun 6H: 84000", 84000 },
		{ "Haehyun Falcon Dive: 135000", 135000 },
		{ "Haehyun Four Tigers Sword (NOT Hold): 45000", 45000 },
		{ "Haehyun Four Tigers Sword (Reverse Ver.): 60000", 60000 },
		{ "Haehyun Tuning Ball (S/H): 203000", 203000 },
		{ "Raven 5K: 118000", 118000 },
		{ "Raven c.S: 89000", 89000 },
		{ "Raven f.S: 191000", 191000 },
		{ "Raven 2S: 78000", 78000 },
		{ "Raven 5H (First Hit): 151000", 151000 },
		{ "Raven 5H (Second Hit): 103000", 103000 },
		{ "Raven Schmerz Berg: 198600", 198600 },
		{ "Raven IAD: 209000", 209000 },
		{ "Raven IAD j.P: 206375", 206375 },
		{ "Raven IAD j.K: 123100", 123100 },
		{ "Raven IAD j.S: 177325", 177325 },
		{ "Raven IAD j.H: 145775", 145775 },
		{ "Raven Scharf Kugel: 296641", 296641 },
		{ "Raven tk Grausam Impuls: 213801", 213801 },
		{ "Raven DAA/6P: 183000", 183000 },
		{ "Dizzy 5K: 161000", 161000 },
		{ "Dizzy c.S: 91000", 91000 },
		{ "Dizzy f.S (Lowest Possible): 139000", 139000 },
		{ "Dizzy 5H: 89000", 89000 },
		{ "Dizyy 2H: 76000", 76000 },
		{ "Dizzy 6P/DAA: 160000", 160000 },
		{ "Dizzy The light was so small in the beginning (Lowest Possible): 139375", 139375 },
		{ "Dizzy For putting out the light...: 170600", 170600 },
		{ "Dizzy For searing cod... (First Descent, Lowest Possible): 167500", 167500 },
		{ "Dizzy For searing cod... (Second Descent, Lowest Possible): 150875", 150875 },
		{ "Baiken 5K: 109000", 109000 },
		{ "Baiken 6K: 110000", 110000 },
		{ "Baiken 2D (First Hit. Second Hit is 18000): 96000", 96000 },
		{ "Baiken 6P/DAA: 175000", 175000 },
		{ "Baiken f.S: 142000", 142000 },
		{ "Baiken 2S: 101000", 101000 },
		{ "Baiken 5H: 63000", 63000 },
		{ "Baiken Kabari (H): 154000", 154000 },
		{ "Baiken Kuchinashi: 210000", 210000 },
		{ "Baiken Sakura: 145000", 145000 },
		{ "Baiken Rokkon Sogi: 48000", 48000 },
		{ "Baiken Tetsuzan Sen: 74000", 74000 },
		{ "Baiken Tsuranu Sanzu-watashi (First Frame of First Hit): 84000", 84000 },
		{ "Answer 5K: 152000", 152000 },
		{ "Answer 6K: 301000", 301000 },
		{ "Answer c.S (First Hit): 147000", 147000 },
		{ "Answer c.S (Second Hit): 166000", 166000 },
		{ "Answer f.S: 115000", 115000 },
		{ "Answer 5H: 63000", 63000 },
		{ "Answer 2H: 145000", 145000 },
		{ "Answer 46P: 88000", 88000 },
		{ "Answer Low Scroll s.D Horizontal: 188001", 188001 }
	};
	
	ImGui::SetNextWindowSize({ 350.F, 0.F }, ImGuiCond_FirstUseEver);
	ImGui::Begin("Low Profile Presets", &showLowProfilePresets);
	ImGui::TextUnformatted("All these values represent the low extent of an attack's hitboxes.");
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
	ImGui::ListBox("##PresetsList", &currentSelectedLowProfilePreset, AttackValuePreset::getName, presets, _countof(presets), 12);
	if (ImGui::Button("Set##LowProfile")) {
		if (currentSelectedLowProfilePreset == -1) {
			showErrorDialog = true;
			errorDialogText = "Please first select a preset.";
			*(ImVec2*)errorDialogPos = ImGui::GetCursorScreenPos();
		} else {
			settings.lowProfileCutoffPoint = presets[currentSelectedLowProfilePreset].value;
			needWriteSettings = true;
		}
	}
	static std::string lowProfilePresetSetHelp;
	if (lowProfilePresetSetHelp.empty()) {
		lowProfilePresetSetHelp = settings.convertToUiDescription("Sets the selected item as the current setting for \"lowProfileCutoffPoint\".");
	}
	AddTooltip(lowProfilePresetSetHelp.c_str());
	ImGui::End();
}

void UI::lowProfileControl() {
	
	int lowProfileCutoffPoint = settings.lowProfileCutoffPoint;
	ImGui::SetNextItemWidth(200.F);
	if (ImGui::InputInt(settings.getOtherUIName(&settings.lowProfileCutoffPoint), &lowProfileCutoffPoint, 1000, 10000, 0)) {
		settings.lowProfileCutoffPoint = lowProfileCutoffPoint;
		needWriteSettings = true;
	}
	imguiActiveTemp = imguiActiveTemp || ImGui::IsItemActive();
	ImGui::SameLine();
	HelpMarker(settings.getOtherUIDescription(&settings.lowProfileCutoffPoint));
	if (ImGui::Button("Presets##LowProfile")) {
		showLowProfilePresets = !showLowProfilePresets;
	}
	AddTooltip("If you don't want to manually enter a value or use 'Box Extents' with frame freeze to determine the right value to enter,"
		" you may select a preset value from a list of character moves.");
}

struct FrameDims {
	float x;
	float width;
};

void drawPlayerFrameTooltipInfo(const PlayerFrame& frame, int playerIndex, float wrapWidth) {
	frame.printInvuls(strbuf, sizeof strbuf - 7);
	if (*strbuf != '\0') {
		ImGui::Separator();
		textUnformattedColored(YELLOW_COLOR, "Invul: ");
		drawOneLineOnCurrentLineAndTheRestBelow(wrapWidth, strbuf);
	}
	if (frame.crossupProtectionIsOdd || frame.crossupProtectionIsAbove1) {
		ImGui::Separator();
		textUnformattedColored(YELLOW_COLOR, "Crossup protection: ");
		ImGui::SameLine();
		sprintf_s(strbuf, "%d/3", frame.crossupProtectionIsAbove1 + frame.crossupProtectionIsAbove1 + frame.crossupProtectionIsOdd);
		ImGui::TextUnformatted(strbuf);
	}
	printAllCancels(frame.cancels,
			frame.enableSpecialCancel,
			frame.enableJumpCancel,
			frame.enableSpecials,
			frame.hitOccured,
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
	}
}

template<typename FramebarT, typename FrameT>
inline void drawFramebar(const FramebarT& framebar, FrameDims* preppedDims, int framebarPosition, ImU32 tintDarker, int playerIndex) {
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
			const char* description = frameArt->description;
			if (frame.newHit && frameTypeActive(frame.type)) {
				frameArt = &drawFramebars_frameArtArray[frame.type - FT_ACTIVE + FT_ACTIVE_NEW_HIT];
				description = frameArt->description;
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
							float currentHeight = ImGui::CalcTextSize(frameArtColorblind[p].description,
								nullptr, false, wrapWidthUse).y;
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
					ImGui::TextUnformatted(description);
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
						drawPlayerFrameTooltipInfo((const PlayerFrame&)frame, playerIndex, wrapWidth);
					}
					if (playerIndex != -1) {
						const PlayerFrame& playerFrame = (const PlayerFrame&)frame;
						if (playerFrame.hitstop || playerFrame.stop.isHitstun || playerFrame.stop.isBlockstun) {
							ImGui::Separator();
							printFameStop(strbuf, sizeof strbuf, &playerFrame.stop, playerFrame.hitstop, playerFrame.hitstopMax);
							ImGui::TextUnformatted(strbuf);
						}
					} else {
						if (frame.hitstop) {
							ImGui::Separator();
							printFameStop(strbuf, sizeof strbuf, nullptr, frame.hitstop, frame.hitstopMax);
							ImGui::TextUnformatted(strbuf);
						}
					}
					if (frame.skippedSuperfreeze || frame.skippedHitstop || frame.rcSlowdown || frame.hitConnected || frame.newHit) {
						ImGui::Separator();
						if (frame.skippedSuperfreeze && frame.skippedHitstop) {
							textUnformattedColored(YELLOW_COLOR, "Since previous displayed frame, skipped:");
							sprintf_s(strbuf, "%df superfreeze;", frame.skippedSuperfreeze);
							ImGui::TextUnformatted(strbuf);
							sprintf_s(strbuf, "%df hitstop.", frame.skippedHitstop);
							ImGui::TextUnformatted(strbuf);
						} else if (frame.skippedSuperfreeze || frame.skippedHitstop) {
							sprintf_s(strbuf, "Since previous displayed frame, skipped %df %s.",
								frame.skippedSuperfreeze + frame.skippedHitstop,
								frame.skippedSuperfreeze ? "superfreeze" : "hitstop");
							ImGui::TextUnformatted(strbuf);
							if (frameAssumesCanBlockButCantFDAfterSuperfreeze(frame.type)) {
								ImGui::PushStyleColor(ImGuiCol_Text, SLIGHTLY_GRAY);
								ImGui::TextUnformatted("Note that cancelling a dash into FD or covering a jump with FD or using FD in general,"
									" including to avoid chip damage, is impossible on this frame, because it immediately follows a superfreeze."
									" Generally doing anything except throw or normal block/IB is impossible in such situations.");
								ImGui::PopStyleColor();
							}
						}
						if (frame.newHit) {
							ImGui::TextUnformatted("A new (potential) hit starts on this frame.");
						}
						if (frame.hitConnected) {
							ImGui::TextUnformatted("A hit connected on this frame.");
						}
						if (frame.rcSlowdown) {
							textUnformattedColored(YELLOW_COLOR, "RC-slowed down: ");
							ImGui::SameLine();
							sprintf_s(strbuf, "%d/%d", frame.rcSlowdown, frame.rcSlowdownMax);
							ImGui::TextUnformatted(strbuf);
						}
					}
					ImGui::PopTextWrapPos();
					ImGui::PopStyleVar();
					ImGui::EndTooltip();
				}
			}
		}
	}
}

void drawPlayerFramebar(const PlayerFramebar& framebar, FrameDims* preppedDims, int framebarPosition, ImU32 tintDarker, int playerIndex) {
	drawFramebar<PlayerFramebar, PlayerFrame>(framebar, preppedDims, framebarPosition, tintDarker, playerIndex);
}

void drawProjectileFramebar(const Framebar& framebar, FrameDims* preppedDims, int framebarPosition, ImU32 tintDarker) {
	drawFramebar<Framebar, Frame>(framebar, preppedDims, framebarPosition, tintDarker, -1);
}

template<typename FramebarT, typename FrameT>
void drawFirstFrames(const FramebarT& framebar, int framebarPosition, FrameDims* preppedDims, float firstFrameTopY, float firstFrameBottomY) {
	const bool considerSimilarFrameTypesSameForFrameCounts = settings.considerSimilarFrameTypesSameForFrameCounts;
	const ImVec2 firstFrameUVStart = { ui.firstFrame->uStart, ui.firstFrame->vStart };
	const ImVec2 firstFrameUVEnd = { ui.firstFrame->uEnd, ui.firstFrame->vEnd };
	for (int i = 0; i < _countof(Framebar::frames); ++i) {
		const FrameT& frame = framebar[i];
		const FrameDims& dims = preppedDims[i];
		
		if (frame.isFirst
				&& !(
					considerSimilarFrameTypesSameForFrameCounts
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

template<typename FramebarT, typename FrameT>
void drawDigits(const FramebarT& framebar, int framebarPosition, FrameDims* preppedDims, float frameNumberYTop, float frameNumberYBottom) {
	
	const bool showFirstFrames = settings.showFirstFramesOnFramebar;
	const bool considerSimilarFrameTypesSameForFrameCounts = settings.considerSimilarFrameTypesSameForFrameCounts;
	
	FrameType lastFrameType = framebar.preFrame;
	if (considerSimilarFrameTypesSameForFrameCounts) {
		lastFrameType = frameMap(lastFrameType);
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
			currentType = frameMap(currentType);
		} else if (currentType == FT_IDLE_ACTIVE_IN_SUPERFREEZE) {
			currentType = FT_IDLE_PROJECTILE;
		}
		
		bool isFirst;
		if (showFirstFrames) {
			isFirst = frame.isFirst
				&& !(
					considerSimilarFrameTypesSameForFrameCounts
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

void drawOneLineOnCurrentLineAndTheRestBelow(float wrapWidth, const char* str) {
	ImGui::SameLine();
	ImFont* font = ImGui::GetFont();
	const char* textEnd = str + strlen(str);
	const char* newlinePos = (const char*)memchr(str, '\n', textEnd - str);
	if (newlinePos == nullptr) newlinePos = textEnd;
	const char* wrapPos = font->CalcWordWrapPositionA(1.F, str, textEnd, wrapWidth - ImGui::GetCursorPosX());
	if (wrapPos == textEnd) {
		ImGui::TextUnformatted(str);
		return;
	} else {
		float itemSpacing = ImGui::GetStyle().ItemSpacing.y;
		ImGui::PushStyleVarY(ImGuiStyleVar_ItemSpacing, 0.F);
		ImGui::TextUnformatted(str, wrapPos);
		while (wrapPos < textEnd && *wrapPos <= 32) {
			++wrapPos;
		}
		if (wrapPos != textEnd) {
			ImGui::TextUnformatted(wrapPos, textEnd);
		}
		ImGui::PopStyleVar();
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + itemSpacing);
	}
}

static void printActiveWithMaxHit(const ActiveDataArray& active, const MaxHitInfo& maxHit, int hitOnFrame) {
	char* buf = strbuf;
	size_t bufSize = sizeof strbuf;
	int result;
	result = active.print(buf, bufSize);
	buf += result;
	bufSize -= result;
	if (!maxHit.empty()
			&& strbuf[0] != '\0'
			&& !(
				active.count == maxHit.maxUse
				&& maxHit.maxUse <= 2  // for Ky Air stun edge and Jack O' j.H
				|| active.count < maxHit.maxUse
			)) {
		result = sprintf_s(buf, bufSize, " (%d hit%s max)", maxHit.maxUse, maxHit.maxUse == 1 ? "" : "s");
		if (result != -1) {
			buf += result;
			bufSize -= result;
		}
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

bool UI::booleanSettingPreset(std::atomic_bool& settingsPtr) {
	bool itHappened = false;
	bool boolValue = settingsPtr;
	if (ImGui::Checkbox(settings.getOtherUIName(&settingsPtr), &boolValue)) {
		settingsPtr = boolValue;
		needWriteSettings = true;
		itHappened = true;
	}
	ImGui::SameLine();
	HelpMarker(settings.getOtherUIDescription(&settingsPtr));
	return itHappened;
}

bool UI::float4SettingPreset(float& settingsPtr) {
	bool attentionPossiblyNeeded = false;
	float floatValue = settingsPtr;
	if (ImGui::InputFloat(settings.getOtherUIName(&settingsPtr), &floatValue, 1.F, 10.F, "%.4f")) {
		settingsPtr = floatValue;
		needWriteSettings = true;
		attentionPossiblyNeeded = true;
	}
	imguiActiveTemp = imguiActiveTemp || ImGui::IsItemActive();
	ImGui::SameLine();
	HelpMarker(settings.getOtherUIDescription(&settingsPtr));
	return attentionPossiblyNeeded;
}

bool UI::intSettingPreset(std::atomic_int& settingsPtr, int minValue) {
	bool isChange = false;
	int intValue = settingsPtr;
	ImGui::SetNextItemWidth(80.F);
	if (ImGui::InputInt(settings.getOtherUIName(&settingsPtr), &intValue, 1, 1, 0)) {
		if (intValue < minValue) {
			intValue = minValue;
		}
		settingsPtr = intValue;
		needWriteSettings = true;
		isChange = true;
	}
	imguiActiveTemp = imguiActiveTemp || ImGui::IsItemActive();
	ImGui::SameLine();
	HelpMarker(settings.getOtherUIDescription(&settingsPtr));
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

static void printAllCancels(const FrameCancelInfo& cancels,
		bool enableSpecialCancel,
		bool enableJumpCancel,
		bool enableSpecials,
		bool hitOccured,
		bool airborne,
		bool insertSeparators) {
	bool needUnpush = false;
	if (ImGui::GetStyle().ItemSpacing.x != 0.F) {
		needUnpush = true;
		ImGui::PushStyleVarX(ImGuiStyleVar_ItemSpacing, 0.F);
	}
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
		if (hitOccured) {
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
		if (hitOccured) {
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

bool prevNamesControl(const PlayerInfo& player, bool includeTitle) {
	if (player.canPrintTotal() || player.startupType() != -1) {
		*strbuf = '\0';
		char* buf = strbuf;
		size_t bufSize = sizeof strbuf;
		if (includeTitle) {
			int result = sprintf_s(buf, bufSize, "Move: ");
			if (result != -1) {
				buf += result;
				bufSize -= result;
			}
		}
		const char* lastNames[2];
		prepareLastNames(lastNames, player);
		player.prevStartupsDisp.printNames(buf, bufSize, lastNames, player.superfreezeStartup ? 2 : 1, settings.useSlangNames);
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

void prepareLastNames(const char** lastNames, const PlayerInfo& player) {
	const char* lastName = settings.useSlangNames && player.lastPerformedMoveSlangName ? player.lastPerformedMoveSlangName : player.lastPerformedMoveName;
	if (player.superfreezeStartup) {
		lastNameSuperfreeze = lastName;
		lastNameSuperfreeze += " Superfreeze Startup";
		lastNameAfterSuperfreeze = lastName;
		lastNameAfterSuperfreeze += " After Superfreeze";
		lastNames[0] = lastNameSuperfreeze.c_str();
		lastNames[1] = lastNameAfterSuperfreeze.c_str();
	} else {
		lastNames[0] = lastName;
		lastNames[1] = lastName;
	}
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
	drawFramebars_frameItselfHeight = settingsFramebarHeight - outerBorderThickness - outerBorderThickness;
	static const float frameNumberHeightOriginal = 11.F;
	static const float frameMarkerWidthOffset = (frameMarkerWidthOriginal - frameWidthOriginal) * 0.5F;
	static const float frameMarkerSideHeightOriginal = 3.F;
	float frameMarkerHeight = frameMarkerHeightOriginal * drawFramebars_frameItselfHeight / frameHeightOriginal;
	if (frameMarkerHeight < 5.F) {
		frameMarkerHeight = 5.F;
	}
	float frameMarkerSideHeight = frameMarkerSideHeightOriginal * frameMarkerHeight / frameMarkerHeightOriginal;
	static const float markerPaddingHeight = -1.F;
	static const float paddingBetweenFramebarsOriginal = 5.F;
	static const float paddingBetweenFramebarsMin = 3.F;
	static const float paddingBetweenTextAndFramebar = 5.F;
	static const float textPadding = 2.F;
	static const float firstFrameHeightDiff = firstFrameHeight - frameHeightOriginal;
	drawFramebars_frameWidthScaled = frameWidthOriginal * drawFramebars_frameItselfHeight / frameHeightOriginal;
	static const float frameNumberPaddingY = 2.F;
	static const float highlighterWidth = 2.F;
	static const float hoveredFrameHighlightPaddingX = 3.F;
	static const float hoveredFrameHighlightPaddingY = 3.F;
	static const float framebarCurrentPositionHighlighterStickoutDistance = 2.F;
	
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
		+ drawFramebars_frameItselfHeight
		+ outerBorderThickness;
	
	std::vector<const EntityFramebar*> framebars;
	const bool eachProjectileOnSeparateFramebar = settings.eachProjectileOnSeparateFramebar;
	if (eachProjectileOnSeparateFramebar) {
		framebars.resize(2 + endScene.projectileFramebars.size(), nullptr);
	} else {
		framebars.resize(2 + endScene.combinedFramebars.size(), nullptr);
	}
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
	ImGuiIO& io = ImGui::GetIO();
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
	
	drawFramebars_windowPos = ImGui::GetWindowPos();
	const float windowWidth = ImGui::GetWindowWidth();
	const float windowHeight = ImGui::GetWindowHeight();
	
	drawFramebars_drawList = ImGui::GetWindowDrawList();
	
	ImDrawList* foregroundFrawList = ImGui::GetForegroundDrawList();
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
		+ innerBorderThickness;
	
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
		
		highlighterStartX = preppedDims[EntityFramebar::confinePos(framebarPosition + 1)].x - innerBorderThickness;
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
				drawPlayerFramebar((const PlayerFramebar&)framebar, preppedDims, framebarPosition, tintDarker, entityFramebar.playerIndex);
			} else {
				drawProjectileFramebar((const Framebar&)framebar, preppedDims, framebarPosition, tintDarker);
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
				
				const float firstFrameTopY = drawFramebars_y - firstFrameHeightDiff;
				const float firstFrameBottomY = drawFramebars_y + firstFrameHeight;
				
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
		drawFramebars_drawList->AddRectFilled(
			{
				highlighterStartX,
				currentPositionHighlighterStartY
					- outerBorderThickness
					- framebarCurrentPositionHighlighterStickoutDistance
			},
			{
				highlighterEndX,
				drawFramebars_y
					- outerBorderThickness
					- paddingBetweenFramebars
					+ framebarCurrentPositionHighlighterStickoutDistance
			},
			ImGui::GetColorU32(IM_COL32(255, 255, 255, 255)));
	}
	
	if (drawFramebars_hoveredFrameIndex != -1) {
		const FrameDims& dims = preppedDims[drawFramebars_hoveredFrameIndex];
		foregroundFrawList->AddRectFilled(
			{
				dims.x - hoveredFrameHighlightPaddingX,
				drawFramebars_hoveredFrameY - hoveredFrameHighlightPaddingY
			},
			{
				dims.x + dims.width + hoveredFrameHighlightPaddingX,
				drawFramebars_hoveredFrameY + oneFramebarHeight + hoveredFrameHighlightPaddingY - 1.F
			},
			ImGui::GetColorU32(IM_COL32(255, 255, 255, 60)));
	}
	
	ImGui::End();
}
