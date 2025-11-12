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
#include <d3d9.h>
#include <chrono>
#include "colors.h"
#include "findMoveByName.h"
#include "InputNames.h"
#include "ImGuiCorrecter.h"
#include "SpecificFramebarIds.h"
#include "texids.h"
#include "Camera.h"
#include "pi.h"
#include "PinAtlas.h"
#include <list>
#include "JSON.h"

UI ui;

void showErrorDlgS(const char* error) {
	ui.showErrorDlg(error, true);
}

static ImVec4 RGBToVec(DWORD color);
static const float inverse_255 = 1.F / 255.F;
static ImVec4 ARGBToVec(DWORD color);
static DWORD VecToRGB(const ImVec4& vec);
static DWORD ARGBToABGR(DWORD color) { return (color & 0xFF00FF00) | ((color & 0xFF0000) >> 16) | ((color & 0xFF) << 16); }
static DWORD multiplyColor(DWORD colorLeft, DWORD colorRight);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
// The following two functions are from imgui_internal.h
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
static ImVec4 VERY_DARK_GRAY = RGBToVec(0x333333);
static ImVec4 PURPLE_COLOR = RGBToVec(0xec3fbd);
static ImVec4 FRAME_SKIPPED_COLOR = RGBToVec(0xc3a1e3);
static ImVec4 FRAME_ADVANCED_COLOR = RGBToVec(0xd5e9f6);
static ImVec4 COUNTERHIT_TEXT_COLOR = RGBToVec(0x9ddef3);
static ImVec4 CROUCHING_TEXT_COLOR = RGBToVec(0x96e7cc);
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
int numDigits(int num);  // For negative numbers does not include the '-'
struct UVStartEnd {
	ImVec2 start;
	ImVec2 end;
	int width;
	int height;
	ImVec2 size;
};
struct DigitUVs {
	UVStartEnd help;
	UVStartEnd framebar;
};
static DigitUVs digitUVs[10];
static DigitUVs digitUVsMini[10];
struct FrameArt {
	FrameType type;
	PngResource* resource;
	StringWithLength description;
	UVStartEnd help;
	UVStartEnd framebar;
};
struct FrameAddon {
	UVStartEnd help;
	UVStartEnd framebar;
};
static FrameAddon firstFrameArt;
static FrameAddon hitConnectedFrameArt;
static FrameAddon hitConnectedFrameBlackArt;
static FrameAddon hitConnectedFrameArtAlt;  // either wider or narrower than hitConnectedFrameArt
static FrameAddon hitConnectedFrameBlackArtAlt;  // either wider or narrower than hitConnectedFrameBlackArt
static FrameAddon hitConnectedFrameArtMini;
static FrameAddon hitConnectedFrameBlackArtMini;
static FrameAddon hitConnectedFrameArtAltMini;  // either wider or narrower than hitConnectedFrameArt
static FrameAddon hitConnectedFrameBlackArtAltMini;  // either wider or narrower than hitConnectedFrameBlackArtMini
static FrameAddon powerupFrameArt;
static FrameAddon newHitFrameArt;
static FrameAddon newHitFrameArtMini;
static FrameArt frameArtNonColorblind[FT_LAST];
static FrameArt frameArtColorblind[FT_LAST];
static FrameArt frameArtNonColorblindMini[FT_LAST];
static FrameArt frameArtColorblindMini[FT_LAST];
struct FrameMarkerArt {
	FrameMarkerType type;
	PngResource* resource;
	DWORD outlineColor;  // ARGB color
	bool hasMiddleLine;
	UVStartEnd help;
	UVStartEnd framebar;
	bool equal(const FrameMarkerArt& other) const {
		return type == other.type
			&& resource == other.resource
			&& outlineColor == other.outlineColor
			&& hasMiddleLine == other.hasMiddleLine;
	}
};
static FrameMarkerArt frameMarkerArtNonColorblind[MARKER_TYPE_LAST];
static FrameMarkerArt frameMarkerArtColorblind[MARKER_TYPE_LAST];
static const float frameWidthOriginal = 9.F;
static const float frameHeightOriginal = 15.F;
static const float frameMarkerWidthOriginal = 11.F;
static const float frameMarkerHeightOriginal = 4.F;  // not including the outline
static const float powerupWidthOriginal = 7.F;
static const float powerupHeightOriginal = 7.F;
static const float firstFrameHeight = 19.F;
static float drawFramebars_frameItselfHeight;
static float drawFramebars_frameItselfHeightProjectile;
static const FrameArt* drawFramebars_frameArtArray;
static const FrameArt* drawFramebars_frameArtArrayMini;
static ImDrawList* drawFramebars_drawList;
static ImVec2 drawFramebars_windowPos;
static int drawFramebars_hoveredFrameIndex;
static float drawFramebars_hoveredFrameY;
static float drawFramebars_hoveredFrameHeight;
static float drawFramebars_y;
static const float innerBorderThicknessUnscaled = 1.F;
static float drawFramebars_innerBorderThickness;
static float drawFramebars_innerBorderThicknessHalf;
static float drawFramebars_frameWidthScaled;
// The total number of frames that can be displayed
static int drawFramebars_framesCount;
// The framebar position with horizontal scrolling already applied to it
// Is in [0;_countof(Framebar::frames)] coordinate space, its range of possible values is [0;_countof(Framebar::frames)-1]
static int drawFramebars_framebarPosition;
// Is in [0;drawFramebars_framesCount] coordinate space, its range of possible values is [0;drawFramebars_framesCount-1]
// It is the result of converting drawFramebars_framebarPosition from [0;_countof(Framebar::frames)] coordinate space to [0;drawFramebars_framesCount] coordinate space
static int drawFramebars_framebarPositionDisplay;
// the internal index that is being referred to here is the index pointing into the [0;_countof(Framebar::frames)] coordinate space
static int inline iterateVisualFramesFrom0_getInitialInternalInd() {
	int result = drawFramebars_framebarPosition - drawFramebars_framebarPositionDisplay;
	if (result < 0) {
		return result + _countof(Framebar::frames);
	} else {
		return result;
	}
}
// the internal index that is being referred to here is the index pointing into the [0;_countof(Framebar::frames)] coordinate space
static void inline iterateVisualFrames_incrementInternalInd(int& internalInd) {
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
#define makeIconAndBoolean(name, origName, size) \
	static GGIcon name; \
	static bool name##Prepared = false; \
	static GGIcon* get_##name() { \
		if (!name##Prepared) { \
			name##Prepared = true; \
			name = scaleGGIconToHeight(origName, size); \
		} \
		return &name; \
	}
makeIconAndBoolean(cogwheelIconScaled16, cogwheelIcon, 16.F)
makeIconAndBoolean(tipsIconScaled14, tipsIcon, 14.F)
static CharacterType getPlayerCharacter(int playerSide);
static void drawPlayerIconWithTooltip(int playerSide);
static void drawFontSizedPlayerIconWithCharacterName(CharacterType charType);
static void drawFontSizedPlayerIconWithText(CharacterType charType, const char* text);
static bool endsWithCaseInsensitive(std::wstring str, const wchar_t* endingPart);
static int findCharRev(const char* buf, char c);
static int findCharRevW(const wchar_t* buf, wchar_t c);
static void AddTooltip(const char* desc);
static void AddTooltipNoSharedDelay(const char* desc);
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
template<typename T>
static int printCancels(const T& cancels, float maxY);
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
static void drawTextButParenthesesInGrayColor(const char* str);
static void printActiveWithMaxHit(const ActiveDataArray& active, const MaxHitInfo& maxHit, int hitOnFrame);
static void drawPlayerIconInWindowTitle(int playerIndex);
static void drawPlayerIconInWindowTitle(GGIcon& icon);
static void drawTextInWindowTitle(const char* txt);
static bool printMoveFieldTooltip(const PlayerInfo& player);
static bool printMoveField(const PlayerInfo& player);
static void headerThatCanBeClickedForTooltip(const char* title, PinnedWindowEnum windowIndex, bool makeTooltip);
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
static float truncfTowardsZero(float value);
struct HitConnectedArtSelector {
	const FrameAddon* hitConnected;
	const FrameAddon* hitConnectedAlt;
	const FrameAddon* hitConnectedBlack;
	const FrameAddon* hitConnectedBlackAlt;
};
static void initializeLetters(bool* letters, bool* lettersStandalone);
static void printReflectableProjectilesList();
static void setOutlinedText(bool isOutlined);
static void pushOutlinedText(bool isOutlined);
static void popOutlinedText();
static bool outlinedTextStack[10] { false };
static bool* outlinedTextHead = outlinedTextStack;
static const ImGuiTableFlags tableFlags = ImGuiTableFlags_Borders
					| ImGuiTableFlags_RowBg
					| ImGuiTableFlags_NoSavedSettings
					| ImGuiTableFlags_NoPadOuterX;
static bool settingOutlineText = false;
static ImGuiWindowFlags windowFlags = (ImGuiWindowFlags)0;
static ImVec4 windowColor { 0.F, 0.F, 0.F, 1.F };
static bool overrideWindowColor = false;
#define customBeginPair(name, i) \
	customBegin(i == 0 ? name##_1 : name##_2)
	
#define toggleOpenManuallyPair(name, i) \
	toggleOpenManually(i == 0 ? name##_1 : name##_2)
	
#define needDrawPair(name, i) \
	needDraw(i == 0 ? name##_1 : name##_2)

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

static const char* getEditEntityRepr(Entity ent);

const char* hitboxTypeName[17] {
	"Hurtbox",
	"Hitbox",
	"ExPnt",
	"ExPntExt",
	"Type4",
	"Pushbox",
	"Type6",
	"Neck",
	"Abdomen",
	"RLeg",
	"LLeg",
	"Priv0",
	"Priv1",
	"Priv2",
	"Priv3",
	"Type15",
	"Type16"
};

typedef GGIcon PinIcon;
static const ImVec2 drawIconButton_minSize = { PinAtlas::add_size, PinAtlas::add_size };
static bool drawIconButton(const char* buttonName, const PinIcon* icon, bool toolActive = false, bool flipHoriz = false, bool flipVert = false, bool disabled = false);

#define makePinIcon(name) \
	static PinIcon name##Btn { \
		{ PinAtlas::name##_width, PinAtlas::name##_height }, \
		{ PinAtlas::name##_x / PinAtlas::pin_texture_width, PinAtlas::name##_y / PinAtlas::pin_texture_height }, \
		{ \
			(PinAtlas::name##_x + PinAtlas::name##_width) / PinAtlas::pin_texture_width, \
			(PinAtlas::name##_y + PinAtlas::name##_height) / PinAtlas::pin_texture_height \
		} \
	};

makePinIcon(add)
makePinIcon(delete)
makePinIcon(select)
makePinIcon(rect)
makePinIcon(undo)
makePinIcon(rename)
makePinIcon(rect_delete)
makePinIcon(eye_open)
makePinIcon(eye_closed)

float UI::dragNDropInterpolationAnim[dragNDropInterpolationTimerMax] { 0.F };
const int UI::dragNDropInterpolationAnimRaw[dragNDropInterpolationTimerMax] {
	3,3,4,6,9,13,16,21,22,22,22,22,21,16,13,9,6,4,3,3
};
static bool dragNDropInterpolationAnimReady = false;

struct P1P2CommonTable {
	ImVec2 tableCursorPos;
	P1P2CommonTable();
	~P1P2CommonTable();
	static const char* tableHeaderTexts[3];
	static float tableHeaderTextWidths[3];
	static bool tableHeaderTextWidthsKnown;
};
const char* P1P2CommonTable::tableHeaderTexts[3] {
	"P1",
	"P2",
	"Common"
};
float P1P2CommonTable::tableHeaderTextWidths[3];
bool P1P2CommonTable::tableHeaderTextWidthsKnown = false;
	

struct CogwheelButtonContext {
	ImVec2 cogwheelPos;  // where the cogwheel should be when settings are displayed
	const GGIcon* scaledIcon;
	const ImVec2* itemSpacing;
	const char* elementName;
	bool* cogwheelTogglePtr;
	bool isOnTransparentBackground;
	const char* description;
	bool showSettings;
	CogwheelButtonContext(
			const char* elementName,
			bool* cogwheelTogglePtr,
			bool isOnTransparentBackground,
			const char* description
	) : elementName(elementName),
		cogwheelTogglePtr(cogwheelTogglePtr),
		isOnTransparentBackground(isOnTransparentBackground),
		description(description),
		showSettings(*cogwheelTogglePtr)
	{
		cogwheelPos = ImGui::GetCursorPos();
		scaledIcon = get_cogwheelIconScaled16();
		itemSpacing = &ImGui::GetStyle().ItemSpacing;
		ImGui::SetCursorPosY(cogwheelPos.y + scaledIcon->size.y + itemSpacing->y * 3.F);
	}
	bool needShowSettings() {
		bool retVal = showSettings;
		showSettings = false;
		return retVal;
	}
	// then draw settings, if needed
	~CogwheelButtonContext() {
		
		ImVec2 contentPos = ImGui::GetCursorPos();
		
		const bool settingsShown = *cogwheelTogglePtr;
		
		ImVec2 buttonCursorPos = {
			cogwheelPos.x + (
				settingsShown
					? 0.F
					: (ImGui::GetContentRegionAvail().x - scaledIcon->size.x - itemSpacing->x * 2.F)
			),
			cogwheelPos.y
		};
		ImGui::SetCursorPos(buttonCursorPos);
		if (isOnTransparentBackground) {
			ImGui::PushStyleColor(ImGuiCol_Button, { 0.F, 0.F, 0.F, 0.F });
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 1.F, 1.F, 1.F, 0.25F });
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 1.F, 1.F, 1.F, 1.F });
		}
		
		if (ImGui::Button(elementName,
			{
				scaledIcon->size.x + itemSpacing->x * 2.F,
				scaledIcon->size.y + itemSpacing->y * 2.F
			})) {
			*cogwheelTogglePtr = !*cogwheelTogglePtr;
		}
		
		if (isOnTransparentBackground) {
			ImGui::PopStyleColor(3);
		}
		
		if (description) AddTooltip(description);
		
		bool isHovered = ImGui::IsItemHovered();
		bool mousePressed = ImGui::IsMouseDown(ImGuiMouseButton_Left);
		ImGui::SetCursorPos({
			buttonCursorPos.x + itemSpacing->x,
			buttonCursorPos.y + itemSpacing->y + (isHovered && mousePressed ? 1.F : 0.F)
		});
		ImVec4 tint { 1.F, 1.F, 1.F, 1.F };
		if (isHovered && mousePressed) {
			tint = { 0.5F, 0.5F, 0.5F, 1.F };
		} else if (isHovered) {
			tint = { 0.75F, 0.75F, 1.F, 1.F };
		}
		ImGui::Image(TEXID_GGICON, scaledIcon->size, scaledIcon->uvStart, scaledIcon->uvEnd, tint);
		
		if (settingsShown) {
			ImGui::SetCursorPos(contentPos);
		} else {
			ImGui::SetCursorPos(cogwheelPos);
		}
		
	}
};

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

// runs on the thread that is loading the DLL
bool UI::onDllMain() {
	
	for (int i = 0; i < 25; ++i) {
		characterIcons[i] = coordsToGGIcon(1 + 42 * i, 1135, 41, 41);
		characterIconsBorderless[i] = coordsToGGIcon(3 + 42 * i, 1137, 37, 37);
	}
	
	uintptr_t GetKeyStateRData = findImportedFunction(GUILTY_GEAR_XRD_EXE, "USER32.DLL", "GetKeyState");
	if (GetKeyStateRData) {
		std::vector<char> sig;
		std::vector<char> mask;
		std::vector<char> maskForCaching;
		// ghidra sig: 8b 3d ?? ?? ?? ?? 52 ff d7
		byteSpecificationToSigMask("8b 3d rel(?? ?? ?? ??) 52 ff d7", sig, mask, nullptr, 0, &maskForCaching);
		substituteWildcard(sig, mask, 0, (void*)GetKeyStateRData);
		uintptr_t GetKeyStateCallPlace = sigscanOffset(
			GUILTY_GEAR_XRD_EXE,
			sig,
			mask,
			{ 2 },
			nullptr, "GetKeyStateCallPlace", maskForCaching.data());
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
	addFrameArt(FT_ACTIVE,
		IDB_ACTIVE_FRAME, activeFrame,
		IDB_ACTIVE_FRAME_NON_COLORBLIND, activeFrameNonColorblind,
		"Active: an attack is currently active. Cannot perform another attack."
		blockFDNotice);
	addFrameArt(FT_ACTIVE_NEW_HIT,
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
	addFrameArt(FT_ACTIVE_HITSTOP,
		IDB_ACTIVE_FRAME_HITSTOP, activeFrameHitstop,
		IDB_ACTIVE_FRAME_HITSTOP_NON_COLORBLIND, activeFrameHitstopNonColorblind,
		"Active: an attack is currently active and the attacking player is in hitstop. Cannot perform another attack."
		blockFDNotice);
	#define addDigitMacro(n) addDigit(IDB_DIGIT_##n, IDB_DIGIT_##n##_THICKNESS_1, digitFrame[n])
	addDigitMacro(0);
	addDigitMacro(1);
	addDigitMacro(2);
	addDigitMacro(3);
	addDigitMacro(4);
	addDigitMacro(5);
	addDigitMacro(6);
	addDigitMacro(7);
	addDigitMacro(8);
	addDigitMacro(9);
	#undef addDigitMacro
	addFrameArt(FT_IDLE, IDB_IDLE_FRAME, idleFrame,
		"Idle: can attack, block and FD.");
	addFrameArt(FT_IDLE_CANT_BLOCK,
		IDB_IDLE_FRAME_CANT_BLOCK, idleFrameCantBlock,
		IDB_IDLE_FRAME_CANT_BLOCK_NON_COLORBLIND, idleFrameCantBlockNonColorblind,
		"Idle, but can't block: can only attack.");
	addFrameArt(FT_IDLE_CANT_FD,
		IDB_IDLE_FRAME_CANT_FD, idleFrameCantFD,
		IDB_IDLE_FRAME_CANT_FD_NON_COLORBLIND, idleFrameCantFDNonColorblind,
		"Idle, but can't FD: can only attack and regular block.");
	addFrameArt(FT_IDLE_ELPHELT_RIFLE,
		IDB_IDLE_FRAME_ELPHELT_RIFLE, idleFrameElpheltRifle,
		IDB_IDLE_FRAME_ELPHELT_RIFLE_NON_COLORBLIND, idleFrameElpheltRifleNonColorblind,
		"Idle: can cancel stance with specials, but not fire yet. Can't block or FD.");
	addFrameArt(FT_STARTUP_STANCE_CAN_STOP_HOLDING,
		IDB_IDLE_FRAME_ELPHELT_RIFLE_CAN_STOP_HOLDING, idleFrameElpheltRifleCanStopHolding,
		IDB_IDLE_FRAME_ELPHELT_RIFLE_CAN_STOP_HOLDING_NON_COLORBLIND, idleFrameElpheltRifleCanStopHoldingNonColorblind,
		"Being in some form of stance: can cancel into one or more specials. Can release the button to cancel stance. Can't block or FD or perform normal attacks.");
	addFrameArt(FT_LANDING_RECOVERY,
		IDB_LANDING_RECOVERY_FRAME, landingRecoveryFrame,
		IDB_LANDING_RECOVERY_FRAME_NON_COLORBLIND, landingRecoveryFrameNonColorblind,
		"Landing recovery: can't perform another attack."
		blockFDNotice);
	addFrameArt(FT_LANDING_RECOVERY_CAN_CANCEL,
		IDB_LANDING_RECOVERY_FRAME_CAN_CANCEL, landingRecoveryFrameCanCancel,
		IDB_LANDING_RECOVERY_FRAME_CAN_CANCEL_NON_COLORBLIND, landingRecoveryFrameCanCancelNonColorblind,
		"Landing recovery: can't perform a regular attack, but can cancel into certain moves."
		blockFDNotice);
	addFrameArt(FT_NON_ACTIVE,
		IDB_NON_ACTIVE_FRAME, nonActiveFrame,
		IDB_NON_ACTIVE_FRAME_NON_COLORBLIND, nonActiveFrameNonColorblind,
		"Non-active: a frame inbetween active frames of an attack."
		" Cannot perform another attack."
		blockFDNotice);
	addFrameArt(FT_PROJECTILE,
		IDB_PROJECTILE_FRAME, projectileFrame,
		IDB_PROJECTILE_FRAME_NON_COLORBLIND, projectileFrameNonColorblind,
		"Projectile: a projectile's attack is active. Can't perform another attack."
		blockFDNotice);
	addFrameArt(FT_RECOVERY,
		IDB_RECOVERY_FRAME, recoveryFrame,
		IDB_RECOVERY_FRAME_NON_COLORBLIND, recoveryFrameNonColorblind,
		"Recovery: an attack's active frames are already over or projectile active frames have started. Can't perform another attack."
		blockFDNotice);
	addFrameArt(FT_RECOVERY_HAS_GATLINGS,
		IDB_RECOVERY_FRAME_HAS_GATLINGS, recoveryFrameHasGatlings,
		IDB_RECOVERY_FRAME_HAS_GATLINGS_NON_COLORBLIND, recoveryFrameHasGatlingsNonColorblind,
		"Recovery, but can gatling or cancel or release: an attack's active frames are already over 9or projectile active frames have started),"
		" but can gatling or cancel into some other attacks"
		" or release the button to end the attack sooner."
		blockFDNotice);
	addFrameArt(FT_RECOVERY_CAN_ACT,
		IDB_RECOVERY_FRAME_CAN_ACT, recoveryFrameCanAct,
		IDB_RECOVERY_FRAME_CAN_ACT_NON_COLORBLIND, recoveryFrameCanActNonColorblind,
		"Recovery, but can gatling or cancel or more: an attack's active frames are already over (or projectile active frames have started),"
		" but can gatling into some other attacks or do other actions."
		blockFDNotice);
	#undef blockFDNotice
	addFrameArt(FT_STARTUP,
		IDB_STARTUP_FRAME, startupFrame,
		IDB_STARTUP_FRAME_NON_COLORBLIND, startupFrameNonColorblind,
		"Startup: an attack's active frames have not yet started, or this is not an attack."
		" Can't perform another attack, can't block and can't FD.");
	addFrameArt(FT_STARTUP_CAN_BLOCK,
		IDB_STARTUP_FRAME_CAN_BLOCK, startupFrameCanBlock,
		IDB_STARTUP_FRAME_CAN_BLOCK_NON_COLORBLIND, startupFrameCanBlockNonColorblind,
		"Startup, but can block:"
		" an attack's active frames have not yet started, or this is not an attack."
		" Can't perform another attack, but can block and/or maybe FD.");
	addFrameMarkerArt(MARKER_TYPE_STRIKE_INVUL, IDB_STRIKE_INVUL, strikeInvulFrame, 0, 0, false, false);
	addFrameMarkerArt(MARKER_TYPE_SUPER_ARMOR, IDB_SUPER_ARMOR_ACTIVE, superArmorFrame, 0, 0xFFFFFFFF, false, false);
	addFrameMarkerArt(MARKER_TYPE_SUPER_ARMOR_FULL, IDB_SUPER_ARMOR_ACTIVE_FULL, superArmorFrameFull, 0, 0xfff0a847, false, true);
	addFrameMarkerArt(MARKER_TYPE_THROW_INVUL, IDB_THROW_INVUL, throwInvulFrame, 0, 0, false, false);
	addFrameMarkerArt(MARKER_TYPE_OTG, IDB_OTG, OTGFrame, 0, 0, false, false);
	addFrameArt(FT_XSTUN,
		IDB_XSTUN_FRAME, xstunFrame,
		IDB_XSTUN_FRAME_NON_COLORBLIND, xstunFrameNonColorblind,
		"Blockstun, hitstun, holding FD, wakeup or airtech: can't perform an attack, and, if in hitstun/wakeup/tech, then block/FD.");
	addFrameArt(FT_XSTUN_CAN_CANCEL,
		IDB_XSTUN_FRAME_CAN_CANCEL, xstunFrameCanCancel,
		IDB_XSTUN_FRAME_CAN_CANCEL_NON_COLORBLIND, xstunFrameCanCancelNonColorblind,
		"Blockstun that can be cancelled into some specials: can't perform regular attacks.");
	addFrameArt(FT_XSTUN_HITSTOP,
		IDB_XSTUN_FRAME_HITSTOP, xstunFrameHitstop,
		IDB_XSTUN_FRAME_HITSTOP_NON_COLORBLIND, xstunFrameHitstopNonColorblind,
		"Blockstun, hitstun, holding FD while in hitstop: can't perform an attack, and, if in hitstun, then block/FD.");
	addFrameArt(FT_GRAYBEAT_AIR_HITSTUN,
		IDB_GRAYBEAT_AIR_HITSTUN, graybeatAirHitstunFrame,
		IDB_GRAYBEAT_AIR_HITSTUN_NON_COLORBLIND, graybeatAirHitstunFrameNonColorblind,
		"In air hitstun, but can airtech: this is a graybeat combo. Can't perform an attack or block or FD.");
	addFrameArt(FT_ZATO_BREAK_THE_LAW_STAGE2,
		IDB_ZATO_BREAK_THE_LAW_STAGE2, zatoBreakTheLawStage2Frame,
		IDB_ZATO_BREAK_THE_LAW_STAGE2_NON_COLORBLIND, zatoBreakTheLawStage2FrameNonColorblind,
		"Performing a move that can be held: can release the button any time to cancel the move."
			" The move was held long enough to have longer recovery upon release."
			" Can't block or FD or perform attacks.");
	addFrameArt(FT_ZATO_BREAK_THE_LAW_STAGE3,
		IDB_ZATO_BREAK_THE_LAW_STAGE3, zatoBreakTheLawStage3Frame,
		IDB_ZATO_BREAK_THE_LAW_STAGE3_NON_COLORBLIND, zatoBreakTheLawStage3FrameNonColorblind,
		"Performing a move that can be held: can release the button any time to cancel the move."
			" The move was held long enough to have an even longer recovery upon release."
			" Can't block or FD or perform attacks.");
	addFrameArt(FT_ZATO_BREAK_THE_LAW_STAGE2_RELEASED,
		IDB_ZATO_BREAK_THE_LAW_STAGE2_RELEASED, zatoBreakTheLawStage2ReleasedFrame,
		IDB_ZATO_BREAK_THE_LAW_STAGE2_RELEASED_NON_COLORBLIND, zatoBreakTheLawStage2ReleasedFrameNonColorblind,
		"Performing a move that can be held: the move had been held long enough to have longer recovery upon release."
			" Can't block or FD or perform attacks.");
	addFrameArt(FT_ZATO_BREAK_THE_LAW_STAGE3_RELEASED,
		IDB_ZATO_BREAK_THE_LAW_STAGE3_RELEASED, zatoBreakTheLawStage3ReleasedFrame,
		IDB_ZATO_BREAK_THE_LAW_STAGE3_RELEASED_NON_COLORBLIND, zatoBreakTheLawStage3ReleasedFrameNonColorblind,
		"Performing a move that can be held: the move had been held long enough to have an even longer recovery upon release."
			" Can't block or FD or perform attacks.");
	addFrameArt(FT_EDDIE_IDLE,
		IDB_EDDIE_IDLE_FRAME, eddieIdleFrame,
		IDB_EDDIE_IDLE_FRAME_NON_COLORBLIND, eddieIdleFrameNonColorblind,
		"Eddie is idle.");
	addFrameArt(FT_BACCHUS_SIGH,
		IDB_BACCHUS_SIGH_FRAME, bacchusSighFrame,
		IDB_BACCHUS_SIGH_FRAME_NON_COLORBLIND, bacchusSighFrameNonColorblind,
		"Bacchus Sigh is ready to hit the opponent when in range.");
	addFrameArt(FT_BACKDASH_RECOVERY,
		IDB_BACKDASH_RECOVERY_FRAME, backdashRecoveryFrame,
		"Backdash recovery: can attack, block and FD, but can't backdash."
		" This is not cancelable by proximity blocking.");
	addFrameArt(FT_NORMAL_LANDING_RECOVERY,
		IDB_NORMAL_LANDING_RECOVERY_FRAME, normalLandingRecoveryFrame,
		"Normal landing recovery: can attack, block and FD, but can't walk, dash, backdash, crouch, jump or superjump."
		" This is cancelable by proximity blocking, meaning, if an attack is incoming, you can do all of those things anyway.");
	
	prepareSecondaryFrameArts(UITEX_HELP);  // need to call this to prevent null resource pointers in array indices > 0, so that packTexture doesn't trip over them
	
	#ifdef _DEBUG
	FrameArt* arrays[2] = { frameArtColorblind, frameArtNonColorblind };
	FrameMarkerArt* arraysMarkers[2] = { frameMarkerArtColorblind, frameMarkerArtNonColorblind };
	
	for (int i = 0; i < 2; ++i) {
		for (int j = 1; j < _countof(frameArtNonColorblind); ++j) {
			if (!arrays[i][j].resource) {
				MessageBoxW(NULL, L"Null frame art.", L"Error", MB_OK);
				return false;
			}
		}
	}
	
	for (int i = 0; i < 2; ++i) {
		for (int j = 0; j < _countof(frameMarkerArtNonColorblind); ++j) {
			if (!arraysMarkers[i][j].resource) {
				MessageBoxW(NULL, L"Null frame marker art.", L"Error", MB_OK);
				return false;
			}
		}
	}
	#endif
	
	jamPantyPtr = (BYTE*)sigscanOffset(
		GUILTY_GEAR_XRD_EXE,
		"8d 4a ff 85 c0 79 04 33 c0 eb 06 3b c1 7c 02 8b c1",
		{ -10, 0 },
		nullptr, "JamPanty");
	
	addImage(IDB_PIN, pinResource);
	
	for (int i = 0; i < PinnedWindowEnum_Last; ++i) {
		windows[i].init();
	}
	
	bool somePinnedWindowsWillOpenOnStartup = settings.openPinnedWindowsOnStartup && hasAtLeastOnePinnedWindowThatIsNotTheMainWindow();
	if (!somePinnedWindowsWillOpenOnStartup || settings.modWindowVisibleOnStart) {
		windows[PinnedWindowEnum_MainWindow].setOpen(true, false);
	}
	
	windowShowMode = settings.modWindowVisibleOnStart || somePinnedWindowsWillOpenOnStartup
			? WindowShowMode_All
			: WindowShowMode_None;
	
	for (int i = 0; i < 3; ++i) {
		hitboxEditorFPACSecondaryData[i].bbscrIndexInAswEng = i;
	}
	
	return true;
}

// Stops queueing new timer events on the window (main) thread. KillTimer does not remove events that have already been queued
// runs on the thread that is unloading the DLL
void UI::onDllDetachStage1_killTimer() {
	timerDisabled = true;
	if (!imguiInitialized) return;
	if (!keyboard.thisProcessWindow) timerId = NULL;
	if (timerId) {
		if (endScene.logicThreadId && endScene.logicThreadId == GetCurrentThreadId()) {
			// no one is probably going to process the queue anymore, just chill
			return;
		}
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
// may either run on the thread that is unloading the DLL, or on the main thread, or on the graphics thread, but normally it should run on the graphics thread
void UI::onDllDetachGraphics() {
	if (imguiD3DInitialized) {
		logwrap(fputs("imgui freeing D3D resources\n", logfile));
		imguiD3DInitialized = false;
		ImGui_ImplDX9_Shutdown();
		clearSecondaryTextures();
	}
}

// Destroys the entire rest of imGui
// may either run on the thread that is unloading the DLL or on the main thread
void UI::onDllDetachNonGraphics() {
	if (imguiD3DInitialized) {
		logwrap(fputs("imgui calling onDllDetachNonGraphics from onDllDetachNonGraphics\n", logfile));
		// this shouldn't happen
		onDllDetachGraphics();
	}
	if (imguiInitialized) {
		logwrap(fputs("imgui freeing non-D3D resources\n", logfile));
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
		imguiInitialized = false;
	}
	detachFPAC();
}

// Runs on the main thread
void UI::prepareDrawData() {
	if (!keyboard.screenSizeKnown) return;
	convertedBoxesPrepared = false;
	lastOverallSelectionBoxReady = false;
	dontUsePreBlockstunTime = settings.frameAdvantage_dontUsePreBlockstunTime;
	drewFramebar = false;
	drewFrameTooltip = false;
	popupsOpen = false;
	if (!isVisible() && !needShowFramebarCached && !boxMouseDown || gifMode.modDisabled) {
		takeScreenshot = false;
		takeScreenshotPress = false;
		imguiActive = false;
		keyboard.imguiActive = false;
		needToDivertCodeInGetKeyState = needTestDelay;
		
		if (imguiInitialized) {
			// When the window is hidden for very long it may become temporarily unresponsive after showing again,
			// and then start quickly processing all the input that was accumulated during the unresponsiveness period (over a span of several frames).
			// However, the thread does not get hung up and both the game and ImGui display fine,
			// it's just that ImGui does not respond to user input for a while.
			// The cause of this is unknown, I haven't tried reproducing it yet and I think it may be related to not doing an ImGui frame every frame.
			imguiContextMenuOpen = false;
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();
			hitboxEditorCheckEntityStillAlive();
			ImGuiIO& imIO = ImGui::GetIO();
			if (imIO.MouseClicked[0]) {
				lastClickPosX = imIO.MousePos.x;
				lastClickPosY = imIO.MousePos.y;
			}
			prepareOutlinedFont();
			adjustMousePosition();
			ImGui::EndFrame();
			endDragNDrop();
			dragNDropInterpolationTimer = 0;
			hitboxEditorBoxSelect();
			hitboxEditProcessBackground();
			if (popupsOpen) {
				ImGui::Render();
				drawData = ImGui::GetDrawData();
				
				keyboard.imguiHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow
					| ImGuiHoveredFlags_AllowWhenBlockedByActiveItem
					| ImGuiHoveredFlags_AllowWhenBlockedByPopup);
				
			} else {
				keyboard.imguiHovered = false;
			}
			keyboard.imguiContextMenuOpen = imguiContextMenuOpen;
		}
		return;
	}
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
	imguiContextMenuOpen = false;
	
	decrementFlagTimer(allowNextFrameTimer, allowNextFrame);
	decrementFlagTimer(takeScreenshotTimer, takeScreenshotPress);
	for (int i = 0; i < 2; ++i) {
		decrementFlagTimer(clearTensionGainMaxComboTimer[i], clearTensionGainMaxCombo[i]);
		decrementFlagTimer(clearBurstGainMaxComboTimer[i], clearBurstGainMaxCombo[i]);
	}
	
	ImGui_ImplWin32_NewFrame();
	adjustMousePosition();
	ImGui::NewFrame();
	hitboxEditorCheckEntityStillAlive();
	ImGuiIO& imIO = ImGui::GetIO();
	if (imIO.MouseClicked[0]) {
		lastClickPosX = imIO.MousePos.x;
		lastClickPosY = imIO.MousePos.y;
	}
	prepareOutlinedFont();
	
	drawSearchableWindows();
	
	if (errorDialogText && *errorDialogText != '\0' && needDraw(PinnedWindowEnum_Error)) {
		ImGui::SetNextWindowPos(ImVec2 { errorDialogPos[0], errorDialogPos[1] }, ImGuiCond_Appearing);
		customBegin(PinnedWindowEnum_Error);
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(errorDialogText);
		ImGui::PopTextWrapPos();
		customEnd();
	}
	
	if (!shaderCompilationError) {
		graphics.getShaderCompilationError(&shaderCompilationError);
	}
	if (shaderCompilationError && needDraw(PinnedWindowEnum_ShaderCompilationError)) {
		ImGui::SetNextWindowSize({ 500.F, 0.F }, ImGuiCond_FirstUseEver);
		customBegin(PinnedWindowEnum_ShaderCompilationError);
		ImGui::PushTextWrapPos(0.F);
		ImGui::TextUnformatted(shaderCompilationError->c_str());
		ImGui::PopTextWrapPos();
		customEnd();
	}
	if (needDraw(PinnedWindowEnum_RankIconDrawingHookError)) {
		ImGui::SetNextWindowSize({ 650.F, 0.F }, ImGuiCond_FirstUseEver);
		customBegin(PinnedWindowEnum_RankIconDrawingHookError);
		ImGui::PushTextWrapPos(0.F);
		if (!game.drawRankInLobbyOverPlayersHeads
				&& !game.drawRankInLobbySearchMemberList
				&& !game.drawRankInLobbyMemberList_NonCircle
				&& !game.drawRankInLobbyMemberList_Circle) {
			ImGui::TextUnformatted("Failed to find any of the code that draws rank icons!");
		} else {
			ImGui::TextUnformatted("Failed to find some of the code that draws rank icons! In those places, icons won't be hidden.");
		}
		ImGui::PopTextWrapPos();
		printLineOfResultOfHookingRankIcons("Over Players' Heads", game.drawRankInLobbyOverPlayersHeads != 0);
		printLineOfResultOfHookingRankIcons("Lobby Search's Member List", game.drawRankInLobbySearchMemberList != 0);
		printLineOfResultOfHookingRankIcons("Inside Lobby - Pause Menu - Display Members - Rank Icons, Besides The Circle", game.drawRankInLobbyMemberList_NonCircle != 0);
		printLineOfResultOfHookingRankIcons("Inside Lobby - Pause Menu - Display Members - The Circle Rank Icon", game.drawRankInLobbyMemberList_Circle != 0);
		customEnd();
	}
	if (needDraw(PinnedWindowEnum_Search)) {
		searchWindow();
	}
	
	if (needShowFramebarCached) {
		drawFramebars();
	} else {
		resetFrameSelection();
	}
	keyboard.imguiHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow
		| ImGuiHoveredFlags_AllowWhenBlockedByActiveItem
		| ImGuiHoveredFlags_AllowWhenBlockedByPopup);
	
	takeScreenshot = takeScreenshotTemp;
	imguiActive = imguiActiveTemp;
	keyboard.imguiActive = imguiActiveTemp;
	keyboard.imguiContextMenuOpen = imguiContextMenuOpen;
	needToDivertCodeInGetKeyState = imguiActive || needTestDelay;
	ImGui::EndFrame();
	ImGui::Render();
	drawData = ImGui::GetDrawData();
	if (keyCombosChanged) {
		settings.onKeyCombosUpdated();
	}
	if (needWriteSettings && keyboard.thisProcessWindow) {
		PostMessageW(keyboard.thisProcessWindow, WM_APP_WIND_UP_TIMER_FOR_DEFERRED_SETTINGS_WRITE, stateChanged, needWriteSettings);
	}
	stateChanged = oldStateChanged || stateChanged;
}

// runs on the main thread
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
	
	windowColor = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
	overrideWindowColor = !searching && settings.globalWindowTransparency != 0 && settings.globalWindowTransparency != 100;
	if (overrideWindowColor) {
		windowColor = { windowColor.x, windowColor.y, windowColor.z, windowColor.w * (float)settings.globalWindowTransparency / 100.F };
	}
	settingOutlineText = settings.outlineAllWindowText;
	
	windowFlags = searching
				? ImGuiWindowFlags_NoSavedSettings
				: (
					settings.globalWindowTransparency == 0
						? ImGuiWindowFlags_NoBackground
						: 0
				) | (settings.disablePinButton ? 0 : ImGuiWindowFlags_HasPinButton);
	
	if (needDraw(PinnedWindowEnum_MainWindow)) {
		customBegin(PinnedWindowEnum_MainWindow);
		drawTextInWindowTitle(windowTitle.c_str());
		pushSearchStack("Main UI Window");
		
		if (ImGui::CollapsingHeader(searchCollapsibleSection("Framedata"), ImGuiTreeNodeFlags_DefaultOpen) || searching) {
			if (endScene.isIGiveUp() && !searching) {
				ImGui::TextUnformatted("Online non-observer match running.");
			} else
			if (ImGui::BeginTable("##PayerData", 3, tableFlags)) {
				ImGui::TableSetupColumn("P1", ImGuiTableColumnFlags_WidthStretch, 0.37f);
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 0.26f);
				ImGui::TableSetupColumn("P2", ImGuiTableColumnFlags_WidthStretch, 0.37f);
				
				ImGui::TableNextColumn();
				drawRightAlignedP1TitleWithCharIcon();
				ImGui::TableNextColumn();
				const GGIcon* scaledIcon = get_tipsIconScaled14();
				CenterAlign(scaledIcon->size.x);
				drawGGIcon(*scaledIcon);
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
				startupOrTotal(two, "Startup", PinnedWindowEnum_StartupFieldHelp);
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
						headerThatCanBeClickedForTooltip(searchFieldTitle("Active"), PinnedWindowEnum_ActiveFieldHelp, true);
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
				startupOrTotal(two, "Total", PinnedWindowEnum_TotalFieldHelp);
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
						headerThatCanBeClickedForTooltip(searchFieldTitle("Invul"), PinnedWindowEnum_InvulFieldHelp, true);
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
					} else if (player.xStunDisplay == PlayerInfo::XSTUN_DISPLAY_WALLSLUMP_LAND) {
						ptrNextSize = sprintf_s(ptrNext, ptrNextSizeCap, "%d/%d",
							player.wallslumpLandWithSlow,
							player.wallslumpLandMaxWithSlow);
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
						headerThatCanBeClickedForTooltip(searchFieldTitle("Frame Adv."), PinnedWindowEnum_FrameAdvantageHelp, !oneWillIncludeParentheses);
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
							const NamePair* displayName = player.move.getDisplayName(player);
							names[0] = useSlang ? displayName->slang : displayName->name;
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
					if (settings.showDebugFields) {
						for (int i = 0; i < two; ++i) {
							PlayerInfo& player = endScene.players[i];
							ImGui::TableNextColumn();
							sprintf_s(strbuf, "%d", player.pawn && *aswEngine ? player.pawn.animFrameStepCounter() : 0);
							printNoWordWrap
							
							if (i == 0) {
								ImGui::TableNextColumn();
								CenterAlignedText("frameSteps");
							}
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
						if (*aswEngine && player.pawn) {
							printNoWordWrapArg(player.pawn.gotoLabelRequests())
						}
						
						if (i == 0) {
							ImGui::TableNextColumn();
							CenterAlignedText("gotoLabelRequests");
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
					for (int i = 0; i < two; ++i) {
						PlayerInfo& player = endScene.players[i];
						ImGui::TableNextColumn();
						if (player.charType == CHARACTER_TYPE_ELPHELT) {
							if (player.cmnActIndex == CmnActStand || player.cmnActIndex == CmnActCrouch2Stand) {
								sprintf_s(strbuf, "%d/%d",
									player.elpheltShotgunCharge.current + player.elpheltShotgunCharge.elpheltShotgunChargeSkippedFrames,
									player.elpheltShotgunCharge.max);
							} else if (player.cmnActIndex == NotACmnAct
									&& strncmp(player.anim, "CounterGuard", 12) == 0
									&& player.y == 0) {
								sprintf_s(strbuf, "shield: %d/%d, shotgun: %d/%d",
									player.charge.current,
									player.charge.max,
									player.elpheltShotgunCharge.current + player.elpheltShotgunCharge.elpheltShotgunChargeSkippedFrames,
									player.elpheltShotgunCharge.max);
							} else {
								sprintf_s(strbuf, "%d/%d",
									player.charge.current, player.charge.max);
							}
						} else {
							sprintf_s(strbuf, "%d/%d", player.charge.current, player.charge.max);
						}
						printWithWordWrap
						
						if (i == 0) {
							ImGui::TableNextColumn();
							CenterAlignedText("charge");
						}
					}
					for (int i = 0; i < two; ++i) {
						PlayerInfo& player = endScene.players[i];
						ImGui::TableNextColumn();
						if (player.pawn && *aswEngine) {
							void* pawnWorld = *(void**)(player.pawn + 0x27a8);
							if (pawnWorld) {
								void* collision = *(void**)((BYTE*)pawnWorld + 0x4a84);
								if (collision) {
									void* top = *(void**)((BYTE*)collision + 0x3c);
									if (top) {
										DWORD flags = *(DWORD*)((BYTE*)top + 0x10);
										sprintf_s(strbuf, "%.8x", flags);
										printNoWordWrap
									}
								}
							}
						}
						
						if (i == 0) {
							ImGui::TableNextColumn();
							CenterAlignedText("collisionFlags");
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
			toggleOpenManually(PinnedWindowEnum_TensionData);
		}
		AddTooltip(searchTooltip("Displays tension gained from combo and factors that affect tension gain."));
		ImGui::SameLine();
		if (ImGui::Button(searchFieldTitle("Burst Gain"))) {
			toggleOpenManually(PinnedWindowEnum_BurstGain);
		}
		AddTooltip(searchTooltip("Displays burst gained from combo or last hit."));
		
		if (ImGui::Button(searchFieldTitle("Combo Damage & Combo Stun (P1)"))) {
			toggleOpenManually(PinnedWindowEnum_ComboDamage_1);
		}
		AddTooltip(searchTooltip("Displays combo damage and maximum total stun achieved during the last performed combo for P1.\n"
			"Also shows the total tension gained during last combo by you and the total burst gained during last combo by the opponent.\n"
			"Note: this window will always have a transparent background, and this cannot be configured."));
		ImGui::SameLine();
		if (ImGui::Button(searchFieldTitle("... (P2)"))) {
			toggleOpenManually(PinnedWindowEnum_ComboDamage_2);
		}
		AddTooltip(searchTooltip("...for P2."));
		
		if (ImGui::Button(searchFieldTitle("Speed/Hitstun Proration/Pushback/Wakeup"))) {
			toggleOpenManually(PinnedWindowEnum_SpeedHitstunProration);
		}
		AddTooltip(searchTooltip("Display x,y, speed, pushback and protation of hitstun and pushback."));
		
		if (ImGui::Button(searchFieldTitle("Projectiles"))) {
			toggleOpenManually(PinnedWindowEnum_Projectiles);
		}
		AddTooltip(searchTooltip("Display the list of current projectiles present on the current frame, for both players."));
		for (int i = 0; i < two; ++i) {
			ImGui::PushID(searchFieldTitle("Character Specific"));
			ImGui::PushID(i);
			sprintf_s(strbuf, i == 0 ? "Character Specific (P%d)" : "... (P%d)", i + 1);
			if (i != 0) ImGui::SameLine();
			if (ImGui::Button(strbuf)) {
				toggleOpenManuallyPair(PinnedWindowEnum_CharSpecific, i);
			}
			AddTooltip(searchTooltip("Display of information specific to a character."));
			ImGui::PopID();
			ImGui::PopID();
		}
		if (ImGui::Button(searchFieldTitle("Box Extents"))) {
			toggleOpenManually(PinnedWindowEnum_BoxExtents);
		}
		AddTooltip(searchTooltip("Shows the minimum and maximum Y (vertical) extents of hurtboxes and hitboxes of each player."
			" The units are not divided by 100 for viewability."));
		
		ImGui::SameLine();
		for (int i = 0; i < two; ++i) {
			searchFieldTitle("Cancels");
			sprintf_s(strbuf, "Cancels (P%d)", i + 1);
			if (ImGui::Button(strbuf)) {
				toggleOpenManuallyPair(PinnedWindowEnum_Cancels, i);
			}
			AddTooltip(searchTooltip(thisHelpTextWillRepeat));
			if (i == 0) ImGui::SameLine();
		}
		
		for (int i = 0; i < two; ++i) {
			ImGui::PushID(searchFieldTitle("Damage/RISC/Stun Calculation"));
			ImGui::PushID(i);
			sprintf_s(strbuf, i == 0 ? "Damage/RISC/Stun Calculation (P1)" : "... (P2)");
			if (ImGui::Button(strbuf)) {
				toggleOpenManuallyPair(PinnedWindowEnum_DamageCalculation, i);
			}
			AddTooltip(searchTooltip("For the attacking player this shows damage, RISC and stun calculation from the last hit and current combo proration."));
			ImGui::PopID();
			ImGui::PopID();
			if (i == 0) ImGui::SameLine();
		}
		
		for (int i = 0; i < two; ++i) {
			int strbufLen = sprintf_s(strbuf, "Combo Recipe (P%d)", i + 1);
			if (ImGui::Button(searchFieldTitle(strbuf, strbuf + strbufLen))) {
				toggleOpenManuallyPair(PinnedWindowEnum_ComboRecipe, i);
			}
			AddTooltip(searchTooltip("Displays actions performed by this player as the attacker during the last combo."));
			if (i == 0) ImGui::SameLine();
		}
		
		for (int i = 0; i < two; ++i) {
			ImGui::PushID(searchFieldTitle("Stun/Stagger Mash"));
			ImGui::PushID(i);
			sprintf_s(strbuf, i == 0 ? "Stun/Stagger Mash (P1)" : "... (P2)");
			if (ImGui::Button(strbuf)) {
				toggleOpenManuallyPair(PinnedWindowEnum_StunMash, i);
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
			
			freezeGameAndAllowNextFrameControls();
			
			bool slowmoGame = std::abs(gifMode.fps - 60.F) > 0.001F;
			if (ImGui::Checkbox(searchFieldTitle("Slow-Mo Mode"), &slowmoGame)) {
				if (!*game.gameDataPtr || !game.isTrainingMode() && !*aswEngine) {
					slowmoGame = false;
				}
				if (slowmoGame) {
					gifMode.fps = settings.slowmoFps;
					game.onFPSChanged();
				} else {
					gifMode.fps = 60.F;
				}
			}
			ImGui::SameLine();
			float slowmoFps = settings.slowmoFps;
			ImGui::SetNextItemWidth(80.F);
			if (ImGui::InputFloat(searchFieldTitle("Slow-Mo FPS"), &slowmoFps, 1.F, 5.F, "%.2f")) {
				if (slowmoFps < 1.F) slowmoFps = 1.F;
				if (slowmoFps > 999.F) slowmoFps = 999.F;
				settings.slowmoFps = slowmoFps;
				if (slowmoGame) {
					gifMode.fps = settings.slowmoFps;
					game.onFPSChanged();
				}
				needWriteSettings = true;
			}
			imguiActiveTemp = imguiActiveTemp || ImGui::IsItemActive();
			ImGui::SameLine();
			static std::string slowmoHelp;
			if (slowmoHelp.empty()) {
				slowmoHelp = settings.convertToUiDescription(
					"You can activate the Slow-mo mode to set the game's FPS to a different value, specified in 'Slow-Mo FPS' field.\n"
					"You can use the \"slowmoGameToggle\" shortcut to toggle slow-mo on and off.\n"
					"Slow-mo mode only works in Training Mode.");
			}
			HelpMarkerWithHotkey(slowmoHelp, settings.slowmoGameToggle);
			
			float fpsValue = gifMode.fps;
			ImGui::SetNextItemWidth(80.F);
			if (ImGui::InputFloat(searchFieldTitle("FPS"), &fpsValue, 1.F, 5.F, "%.2f")) {
				if (fpsValue < 1.F) fpsValue = 1.F;
				if (fpsValue > 999.F) fpsValue = 999.F;
			}
			if (!game.isTrainingMode() || !*aswEngine) {
				fpsValue = 60.F;
			}
			if (std::abs(fpsValue - gifMode.fps) > 0.001F) {
				game.onFPSChanged();
				gifMode.fps = fpsValue;
			}
			imguiActiveTemp = imguiActiveTemp || ImGui::IsItemActive();
			ImGui::SameLine();
			HelpMarker(searchTooltip("Changes FPS (frames per second) of the whole game. Works only in Training Mode."));
			
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
			
			if (ImGui::Button(searchFieldTitle("Hitbox Editor"))) {
				toggleOpenManually(PinnedWindowEnum_HitboxEditor);
			}
			ImGui::SameLine();
			HelpMarker(searchTooltip("Shows or hides the 'Hitbox Editor' menu. The edited data gets lost on character change or"
				" after exiting training mode, unless saved into a .collision file."
				" But keep in mind, when loading back from a .collision file, it still won't let you delete sprites that are"
				" already in the game. Sprite deletion must go hand-in-hand with bbscript edits. Use Pangaea's ggxrd-mod to load"
				" the right bbscript and .collision files on the loading screen: https://github.com/super-continent/ggxrd-mod."));
			
		}
		popSearchStack();
		if (ImGui::CollapsingHeader(searchCollapsibleSection("Settings")) || searching) {
			if (ImGui::CollapsingHeader(searchCollapsibleSection("Hitbox Settings")) || searching) {
				
				if (ImGui::Button(searchFieldTitle("Hitboxes Help"))) {
					toggleOpenManually(PinnedWindowEnum_HitboxesHelp);
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
					needWriteSettings = true;
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
				
				booleanSettingPreset(settings.showIndividualHitboxOutlines);
				
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
					toggleOpenManually(PinnedWindowEnum_FramebarHelp);
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
						settings.condenseIntoOneProjectileFramebar = false;
					}
				}
				
				if (booleanSettingPreset(settings.condenseIntoOneProjectileFramebar)) {
					if (settings.condenseIntoOneProjectileFramebar) {
						settings.eachProjectileOnSeparateFramebar = false;
					}
				}
				
				booleanSettingPreset(settings.dontClearFramebarOnStageReset);
				
				booleanSettingPreset(settings.dontTruncateFramebarTitles);
				
				booleanSettingPreset(settings.useSlangNames);
				
				booleanSettingPreset(settings.allFramebarTitlesDisplayToTheLeft);
				
				booleanSettingPreset(settings.showPlayerInFramebarTitle);
				
				intSettingPreset(settings.framebarTitleCharsMax, 0);
				
				intSettingPreset(settings.playerFramebarHeight, 1);
				
				intSettingPreset(settings.projectileFramebarHeight, 1);
				
				intSettingPreset(settings.distanceBetweenPlayerFramebars, INT_MIN);
				
				intSettingPreset(settings.distanceBetweenProjectileFramebars, INT_MIN);
				
				intSettingPreset(settings.digitThickness, 1, 1, 1, 80.F, 2);
				
				booleanSettingPreset(settings.drawDigits);
				
				booleanSettingPreset(settings.showP1FramedataInFramebar);
				
				booleanSettingPreset(settings.showP2FramedataInFramebar);
				
				float4SettingPreset(settings.framedataInFramebarScale, 0.F, FLT_MAX, 0.1F, 0.2F, 100.F);
				
				if (intSettingPreset(settings.framebarStoredFramesCount, 1, 1, 1, 80.F, _countof(Framebar::frames))) {
					if (settings.framebarDisplayedFramesCount > settings.framebarStoredFramesCount) {
						settings.framebarDisplayedFramesCount = settings.framebarStoredFramesCount;
					}
				}
				
				intSettingPreset(settings.framebarDisplayedFramesCount, 1, 1, 1, 80.F, settings.framebarStoredFramesCount);
				
				booleanSettingPreset(settings.clearFrameSelectionWhenFramebarAdvances);
				
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
				keyComboControl(settings.disableModToggle);
				keyComboControl(settings.hitboxEditModeToggle);
				keyComboControl(settings.hitboxEditMoveCameraUp);
				keyComboControl(settings.hitboxEditMoveCameraDown);
				keyComboControl(settings.hitboxEditMoveCameraLeft);
				keyComboControl(settings.hitboxEditMoveCameraRight);
				keyComboControl(settings.hitboxEditMoveCameraBack);
				keyComboControl(settings.hitboxEditMoveCameraForward);
			}
			popSearchStack();
			if (ImGui::CollapsingHeader(searchCollapsibleSection("General Settings")) || searching) {
				booleanSettingPreset(settings.modWindowVisibleOnStart);
				booleanSettingPreset(settings.openPinnedWindowsOnStartup);
				if (booleanSettingPreset(settings.disablePinButton)) {
					onDisablePinButtonChanged(false);
				}
				
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
				
				if (booleanSettingPreset(settings.usePositionResetMod)) {
					if (settings.usePositionResetMod) {
						game.onUsePositionResetChanged();
					}
				}
				
				intSettingPreset(settings.positionResetDistBetweenPlayers, 0, 1000, 10000, 120.F);
				intSettingPreset(settings.positionResetDistFromCorner, 0, 1000, 10000, 120.F);
				
				booleanSettingPreset(settings.showDebugFields);
				
				booleanSettingPreset(settings.ignoreNumpadEnterKey);
				booleanSettingPreset(settings.ignoreRegularEnterKey);
				
				intSettingPreset(settings.startingTensionPulseP1, -25000, 100, 1000, 120.F, 25000);
				intSettingPreset(settings.startingTensionPulseP2, -25000, 100, 1000, 120.F, 25000);
				
				bool startingBurstGaugeChanged = false;
				if (intSettingPreset(settings.startingBurstGaugeP1, 0, 100, 1000, 120.F, 15000)) {
					startingBurstGaugeChanged = true;
				}
				if (intSettingPreset(settings.startingBurstGaugeP2, 0, 100, 1000, 120.F, 15000)) {
					startingBurstGaugeChanged = true;
				}
				if (startingBurstGaugeChanged) {
					endScene.onStartingBurstGaugeChanged();
				}
				
				booleanSettingPreset(settings.clearInputHistoryOnStageReset);
				booleanSettingPreset(settings.clearInputHistoryOnStageResetInTrainingMode);
				
				booleanSettingPreset(settings.hideWins);
				booleanSettingPreset(settings.hideWinsDirectParticipantOnly);
				intSettingPreset(settings.hideWinsExceptOnWins, INT_MIN, 1, 5, 80.F);
				
				if (booleanSettingPreset(settings.hideRankIcons)) {
					if (settings.hideRankIcons) {
						game.hideRankIcons();
						if (!game.drawRankInLobbyOverPlayersHeads
								|| !game.drawRankInLobbySearchMemberList
								|| !game.drawRankInLobbyMemberList_NonCircle
								|| !game.drawRankInLobbyMemberList_Circle) {
							setOpen(PinnedWindowEnum_RankIconDrawingHookError, true, true);
						}
					}
				}
				
				booleanSettingPreset(settings.overrideOnlineInputDelay);
				intSettingPreset(settings.onlineInputDelayFullscreen, 0, 1, 1, 80.F, 4);
				intSettingPreset(settings.onlineInputDelayWindowed, 0, 1, 1, 80.F, 4);
				
				if (needTestDelayOnNextUiFrame) {
					needTestDelayOnNextUiFrame = false;
					testDelay();
				}
				if (ImGui::Button(searchFieldTitle("Test Delay"))) {
					if (!sigscannedButtonSettings) {
						needTestDelayOnNextUiFrame = true;  // sigscanning takes ~7ms
						sigscannedButtonSettings = true;
						buttonSettings = (ButtonSettings*)sigscanOffset(GUILTY_GEAR_XRD_EXE,
							"41 83 f9 0e 7c c4 8d 43 54",
							{ 10, 0 },
							nullptr, "ButtonSettings");
						finishedSigscanning();
					} else {
						testDelay();
					}
				}
				ImGui::SameLine();
				HelpMarker(searchTooltip("Send a PUNCH key press into the game and monitor how long it takes for it to arrive into the input ring buffer.\n"
					" The result, in milliseconds, will be displayed below."));
				zerohspacing
				textUnformattedColored(SLIGHTLY_GRAY, "Result: ");
				ImGui::SameLine();
				if (idiotPressedTestDelayButtonOutsideBattle) {
					ImGui::TextUnformatted("<Must press button during battle>");
				} else if (hasTestDelayResult) {
					unsigned long long currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
					unsigned long long timeDiff = currentTime - testDelayStart;
					sprintf_s(strbuf, "%u ms", testDelayResult);
					ImGui::TextUnformatted(strbuf);
					if (timeDiff > 30000ULL) {  // 30 seconds
						ImGui::SameLine();
						if (timeDiff > 120000ULL) {  // 2 minutes
							if (timeDiff > 7200000ULL) {  // 2 hours
								unsigned long remainder = timeDiff % 3600000U;
								unsigned long long divisionResult = timeDiff / 3600000U;
								sprintf_s(strbuf, " (Last measured %llu.%u hours ago)",
									divisionResult,
									remainder * 10 / 3600000U);
							} else {
								sprintf_s(strbuf, " (Last measured %llu minutes ago)", timeDiff / 60000U);
							}
						} else {
							sprintf_s(strbuf, " (Last measured %llu seconds ago)", timeDiff / 1000U);
						}
						textUnformattedColored(SLIGHTLY_GRAY, strbuf);
					}
				} else {
					ImGui::TextUnformatted("<Not measured yet>");
				}
				_zerohspacing
				
				if (booleanSettingPreset(settings.player1IsBoss) && settings.player1IsBoss) {
					endScene.onPlayerIsBossChanged();
				}
				if (booleanSettingPreset(settings.player2IsBoss) && settings.player2IsBoss) {
					endScene.onPlayerIsBossChanged();
				}
				
				booleanSettingPreset(settings.useSigscanCaching);
				
				bool connectionTierChanged = false;
				if (booleanSettingPreset(settings.overrideYourConnectionTierForFilter)) {
					connectionTierChanged = true;
				}
				if (intSettingPreset(settings.connectionTierToPretendAs, 0, 1, 1, 80.F, 4, !settings.overrideYourConnectionTierForFilter)) {
					connectionTierChanged = true;
				}
				if (connectionTierChanged) {
					game.onConnectionTierChanged();
				}
				
				bool highlightGreenBlueChanged = false;
				if (booleanSettingPreset(settings.highlightRedWhenBecomingIdle)) {
					highlightGreenBlueChanged = true;
				}
				if (booleanSettingPreset(settings.highlightGreenWhenBecomingIdle)) {
					highlightGreenBlueChanged = true;
				}
				if (booleanSettingPreset(settings.highlightBlueWhenBecomingIdle)) {
					highlightGreenBlueChanged = true;
				}
				if (ImGui::Button(searchFieldTitle(settings.getOtherUINameWithLength(&settings.highlightWhenCancelsIntoMovesAvailable)))) {
					toggleOpenManually(PinnedWindowEnum_HighlightedCancels);
				}
				ImGui::SameLine();
				HelpMarker(searchTooltip(settings.getOtherUIDescriptionWithLength(&settings.highlightWhenCancelsIntoMovesAvailable)));
				if (highlightGreenBlueChanged) {
					endScene.highlightGreenWhenBecomingIdleChanged();
				}
				
				intSettingPreset(settings.globalWindowTransparency, 0, 5, 20, 80.F, 100);
				booleanSettingPreset(settings.outlineAllWindowText);
				
				booleanSettingPreset(settings.showYrcWindowsInCancelsPanel);
				
				if (booleanSettingPreset(settings.dontResetBurstAndTensionGaugesWhenInStunOrFaint)) {
					endScene.onDontResetBurstAndTensionGaugesWhenInStunOrFaintChanged();
				}
				
				if (booleanSettingPreset(settings.dontResetRISCWhenInBurstOrFaint)) {
					endScene.onDontResetRiscWhenInBurstOrFaintChanged();
				}
				
				if (booleanSettingPreset(settings.onlyApplyCounterhitSettingWhenDefenderNotInBurstOrFaintOrHitstun)) {
					endScene.onOnlyApplyCounterhitSettingWhenDefenderNotInBurstOrFaintOrHitstunChanged();
				}
				
				booleanSettingPreset(settings.showPunishMessageOnWhiff);
				
				booleanSettingPreset(settings.showPunishMessageOnBlock);
				
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
				toggleOpenManually(PinnedWindowEnum_Search);
			}
			AddTooltip("Searches the UI field names and tooltips for text.");
		}
		customEnd();
	}
	popSearchStack();
	searchCollapsibleSection(PinnedWindowEnum_HitboxEditor);
	if (needDraw(PinnedWindowEnum_HitboxEditor) || searching) {
		customBegin(PinnedWindowEnum_HitboxEditor);
		if (!*aswEngine || !game.isTrainingMode()) {
			ImGui::PushTextWrapPos(0.F);
			ImGui::TextUnformatted("Editing hitboxes is only possible in training mode.");
			ImGui::PopTextWrapPos();
		} else {
			drawHitboxEditor();
		}
		customEnd();
	} else {
		endDragNDrop();
		dragNDropInterpolationTimer = 0;
	}
	hitboxEditProcessBackground();
	hitboxEditorBoxSelect();
	popSearchStack();
	searchCollapsibleSection(PinnedWindowEnum_TensionData);
	if (needDraw(PinnedWindowEnum_TensionData) || searching) {
		customBegin(PinnedWindowEnum_TensionData);
		if (endScene.isIGiveUp() && !searching) {
			ImGui::TextUnformatted("Online non-observer match running.");
		} else
		if (ImGui::BeginTable("##TensionData", 3, tableFlags)) {
			
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
		customEnd();
	}
	popSearchStack();
	searchCollapsibleSection(PinnedWindowEnum_BurstGain);
	if (needDraw(PinnedWindowEnum_BurstGain) || searching) {
		customBegin(PinnedWindowEnum_BurstGain);
		if (endScene.isIGiveUp() && !searching) {
			ImGui::TextUnformatted("Online non-observer match running.");
		} else
		if (ImGui::BeginTable("##BurstGain", 3, tableFlags)
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
				" The amount gained from each hit is based on (damage * 3 + 100) * Burst Gain Modifier. The damage used here is the base damage of the attack.\n"
				" There are two or three percentages displayed. The first depends on combo count (combo count + 32) / 32, the second depends on some unknown thing"
				" that scales burst gain by 20%. The third percentage is displayed when you're playing Stylish and is the stylish burst gain modifier.\n"));
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
			
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(searchFieldTitle("Burst Gain Per Second"));
			AddTooltip(searchTooltip("Burst is also gained each 60 frames. The timer for this is shown in parentheses,"
				" and increments each frame spent in hitstop or in \"normal\" timeflow."
				" \"Normal\" timeflow is when it is not hitstop, not superfreeze and not skipping a frame due to RC slowdown"
				" (this means that the \"normal\" timeflow is halved during RC slowdown of affected players and projectiles).\n"
				"The amount of burst gained depends on HP:\n"
				"211-420 (50%-100%): 60 burst;\n"
				"106-210 (25%-50%): 120 burst;\n"
				"1-105 (0%-25%): 180 burst."));
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				int burstGainedPerSecond;
				if (player.hp > 210) {
					burstGainedPerSecond = 60;
				} else if (player.hp < 106) {
					burstGainedPerSecond = 180;
				} else {
					burstGainedPerSecond = 120;
				}
				
				sprintf_s(strbuf, "%d (%d/60)", burstGainedPerSecond, player.burstGainCounter % 60);
				ImGui::TextUnformatted(strbuf);
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
		customEnd();
	}
	popSearchStack();
	searchCollapsibleSection("Combo Damage & Combo Stun");
	for (int i = 0; i < two; ++i) {
		if (needDrawPair(PinnedWindowEnum_ComboDamage, i) || searching) {
			ImGui::PushID(i);
			customBeginPair(PinnedWindowEnum_ComboDamage, i);
			PlayerInfo& player = endScene.players[i];
			PlayerInfo& opponent = endScene.players[1 - i];
			
			drawPlayerIconInWindowTitle(i);
			
			if (!*aswEngine) {
				ImGui::TextUnformatted("Match isn't running.");
			} else
			if (ImGui::BeginTable("##ComboDmgStun", 2, tableFlags)) {
				ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0.5F);
				ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.5F);
				
				ImGui::TableNextColumn();
				pushOutlinedText(true);
				ImGui::TextUnformatted(searchFieldTitle("Combo Damage"));
				AddTooltip(searchFieldTitle("Total damage done by this player as the attacker during the last combo."));
				ImGui::TableNextColumn();
				if (opponent.pawn) {
					sprintf_s(strbuf, "%d", opponent.pawn.TrainingEtc_ComboDamage());
					ImGui::TextUnformatted(strbuf);
				}
				
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(searchFieldTitle("Combo Stun"));
				AddTooltip(searchFieldTitle("Maximum total stun reached by the opponent during the last combo that was done by this player as the attacker."));
				ImGui::TableNextColumn();
				sprintf_s(strbuf, "%d", opponent.stunCombo);
				ImGui::TextUnformatted(strbuf);
				
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(searchFieldTitle("Tension Gained Last Combo"));
				AddTooltip(searchFieldTitle("The total amount of tension gained during the last combo by this player as the attacker.\n"
					"This value is in units from 0.00 (no tension) to 100.00 (full tension)."));
				ImGui::TableNextColumn();
				printDecimal(player.tensionGainLastCombo, 2, 0);
				ImGui::TextUnformatted(printdecimalbuf);
				
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(searchFieldTitle("Burst Gained Last Combo"));
				AddTooltip(searchFieldTitle("The total amount of burst gained during the last combo by the opponent as the defender.\n"
					"This value is in units from 0.00 (no burst) to 150.00 (full burst)."));
				ImGui::TableNextColumn();
				printDecimal(opponent.burstGainLastCombo, 2, 0);
				ImGui::TextUnformatted(printdecimalbuf);
				popOutlinedText();
				
				ImGui::EndTable();
			}
			customEnd();
			ImGui::PopID();
		}
	}
	popSearchStack();
	searchCollapsibleSection(PinnedWindowEnum_SpeedHitstunProration);
	if (needDraw(PinnedWindowEnum_SpeedHitstunProration) || searching) {
		customBegin(PinnedWindowEnum_SpeedHitstunProration);
		
		if (endScene.isIGiveUp() && !searching) {
			ImGui::TextUnformatted("Online non-observer match running.");
		} else
		if (ImGui::BeginTable("##SpeedHitstunProrationDotDotDot", 3, tableFlags)) {
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
			AddTooltip(searchTooltip("The time, in seconds, of the current combo's duration. Keeps counting during hitstop. Pauses during superfreeze"
				" and increases at half the rate when the one being comboed is affected by RC slowdown."));
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
				"Ground block: 1250, 1375, 1500, 1750, 2000;\n"
				"Air block: 800,  850,  900,  950, 1000;\n"
				"Hit: 1300, 1400, 1500, 1750, 2000;\n"));
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
				" Combo timer modifier depends on combo timer (keeps counting during hitstop; pauses during superfreeze"
				" and increases at half the rate when the one being comboed is affected by RC slowdown),"
				" in frames: >= 480 --> 200%, >= 420 --> 184%, >= 360 --> 166%, >= 300 --> 150%"
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
				" of the ignored parameter.\n\n"
				"Base speed Y by default depends on the attack damage and air launch effect, however, some attacks override Base speed Y.\n"
				"If the attack launched the opponent from a ground hit, Base speed Y is set"
				" to a preset value, according to this table (all speeds provided in units that are not divided by 100):\n"
				"1) Attack launches vertically (face down landing) - 35000;\n"
				"2) Attack launches you forward (face down landing) - 14000;\n"
				"3) Attack launches you backward (face up landing) or any other way - 17500;\n"
				"\n"
				"If the attack launches you vertically, but you got hit airborne - 28000.\n"
				"For everything else: ((Base Damage + 240) * 875) / 10, rounded down.\n"
				"Again, some attacks redefine their Base speed Y, these are merely the default values.\n"
				"Hitting the opponent with a move that deals higher damage, but is slower, might not launch them higher"
				" than a weaker, but fast move that hits them earlier, due to the opponent constantly descending down."));
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
				"Some attacks could theoretically ignore hitstun proration. When that happens, 100% is displayed.\n"
				"Combo timer keeps counting during hitstop, pauses during superfreeze"
				" and increases at half the rate when the one being comboed is affected by RC slowdown."));
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
			ImGui::TextUnformatted(searchFieldTitle("Attack Y"));
			AddTooltip(searchTooltip("The Y of the last connected attack. This value is divided by 100 for viewability."
				" If the attack connected at Y < 1750.00, the hitstun animation is low, otherwise it is high."));
			for (int i = 0; i < two; ++i) {
				PlayerInfo& player = endScene.players[i];
				ImGui::TableNextColumn();
				if (*aswEngine && player.pawn) {
					printDecimal((int)player.pawn.attackY(), 2, 0);
					ImGui::TextUnformatted(printdecimalbuf);
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
		customEnd();
	}
	popSearchStack();
	searchCollapsibleSection(PinnedWindowEnum_Projectiles);
	if (needDraw(PinnedWindowEnum_Projectiles) || searching) {
		customBegin(PinnedWindowEnum_Projectiles);
		
		if (ImGui::BeginTable("##Projectiles", 3, tableFlags)) {
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
							if (projectile.ptr) {
								sprintf_s(strbuf, "%p (%d)", (void*)projectile.ptr, projectile.ptr.getEffectIndex());
							} else {
								sprintf_s(strbuf, "%p", (void*)projectile.ptr);
							}
							printNoWordWrap
						}
						
						if (i == 0) {
							ImGui::TableNextColumn();
							CenterAlignedText("ptr");
							AddTooltip("Displayes the pointer to the projectile's data and, in parentheses,"
								" its index in the game's Effects[75] array.");
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
							printNoWordWrapArg(projectile.ptr ? projectile.ptr.gotoLabelRequests() : "")
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
							sprintf_s(strbuf, "%d", projectile.ptr && *aswEngine ? projectile.ptr.animFrameStepCounter() : 0);
							printNoWordWrap
						}
						
						if (i == 0) {
							ImGui::TableNextColumn();
							CenterAlignedText("Frame Steps");
							AddTooltip("Current time spent in the current state, in frames, including frozen frames. Starts from 1 when entering a new state.");
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
		customEnd();
	}
	popSearchStack();
	searchCollapsibleSection("Character Specific");
	for (int i = 0; i < 2; ++i) {
		if (needDrawPair(PinnedWindowEnum_CharSpecific, i) || searching) {
			ImGui::PushID(i);
			customBeginPair(PinnedWindowEnum_CharSpecific, i);
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
							if (func) {
								BYTE* foundPos = moves.findSpriteNonNull(func);
								if (!foundPos) break;
								bool created = false;
								InstrType lastType = instr_sprite;
								int lastSpriteLength = 0;
								for (BYTE* instr = foundPos; lastType != instr_endState; ) {
									if (lastType == instr_sprite) {
										lastSpriteLength = asInstr(instr, sprite)->duration;
										if (!created) {
											player.gunflameParams.totalSpriteLengthUntilCreation += lastSpriteLength;
										}
										player.gunflameParams.totalSpriteLength += lastSpriteLength;
									} else if (lastType == instr_createObjectWithArg) {
										if (!created) {
											player.gunflameParams.totalSpriteLengthUntilCreation -= lastSpriteLength;
											created = true;
										}
									} else if (lastType == instr_deleteMoveForceDisableFlag) {
										player.gunflameParams.totalSpriteLength -= lastSpriteLength;
										break;
									}
									instr = moves.skipInstr(instr);
									lastType = moves.instrType(instr);
								}
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
										for (BYTE* instr = moves.skipInstr(funcStart);
												moves.instrType(instr) != instr_endState;
												instr = moves.skipInstr(instr)) {
											if (moves.instrType(instr) == instr_sprite) {
												moves.stunEdgeDeleteSpriteSum += asInstr(instr, sprite)->duration;
											}
										}
									}
								}
							} else if (strcmp(p.animationName(), "ChargedStunEdgeObj") == 0) {
								hasChargedStunEdge = true;
							} else if (strcmp(p.animationName(), "SPChargedStunEdgeObj") == 0) {
								if (moves.instrType(p.bbscrCurrentInstr()) == instr_endState) {
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
											if (moves.instrType(instr) == instr_sprite) {
												lastSprite = instr;
											}
											instr = moves.skipInstr(instr);
										} while (moves.instrType(instr) != instr_endState);
										if (lastSprite) {
											moves.spChargedStunEdgeKowareSpriteDuration = asInstr(lastSprite, sprite)->duration;
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
					if (p && p.isActive() && strcmp(p.animationName(), "Mahojin") == 0 && p.bbscrCurrentFunc()) {
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
					if (dolphin && dolphin.isActive() && dolphin.bbscrCurrentFunc()) {
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
								BYTE* instr = moves.skipInstr(pos);
								moves.mayIrukasanRidingObjectYokoA.offset = pos - func;
								int totalSoFar = 0;
								bool lastSpriteWasNull = false;
								while (moves.instrType(instr) != instr_endState) {
									InstrType type = moves.instrType(instr);
									if (type == instr_sprite) {
										if (!ar[arInd]->frames.empty()) {
											ar[arInd]->frames.back().offset = instr - func;
										}
										if (strcmp(asInstr(instr, sprite)->name, "null") == 0) {
											lastSpriteWasNull = true;
										} else {
											int spriteLength = asInstr(instr, sprite)->duration;
											ar[arInd]->frames.emplace_back();
											Moves::MayIrukasanRidingObjectFrames& newObj = ar[arInd]->frames.back();
											newObj.offset = instr - func;
											newObj.frames = totalSoFar;
											totalSoFar += spriteLength;
										}
									} else if (type == instr_exitState && !lastSpriteWasNull && !ar[arInd]->frames.empty()) {
										ar[arInd]->frames.back().offset = instr - func;
									} else if (type == instr_setMarker) {
										ar[arInd]->totalFrames = totalSoFar;
										if (arInd == 3) break;
										++arInd;
										totalSoFar = 0;
										lastSpriteWasNull = false;
										ar[arInd]->offset = instr - func;
									}
									instr = moves.skipInstr(instr);
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
					eddie = endScene.getReferredEntity((void*)player.pawn.ent, ENT_STACK_0);
				}
				
				ImGui::TextUnformatted(searchFieldTitle("Eddie Values"));
				sprintf_s(strbuf, "##Zato_P%d", i);
				if (ImGui::BeginTable(strbuf, 2, tableFlags)) {
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
				
				if (ImGui::Button("Show Reflectable Projectiles")) {
					showZatoReflectableProjectiles[i] = !showZatoReflectableProjectiles[i];
				}
				if (showZatoReflectableProjectiles[i]) {
					printReflectableProjectilesList();
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
					ImGui::PushStyleColor(ImGuiCol_Text, SLIGHTLY_GRAY);
					ImGui::PushTextWrapPos(0.F);
					ImGui::TextUnformatted("This value increases slower when opponent slows you down with RC, and stops increasing during superfreezes.");
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
				if (ImGui::Button("Show Flickable Projectiles")) {
					showPotCanFlick[i] = !showPotCanFlick[i];
				}
				ImGui::SameLine();
				if (ImGui::Button("Show Unflickable Projectiles")) {
					showPotCantFlick[i] = !showPotCantFlick[i];
				}
				ImGui::PushTextWrapPos(0.F);
				if (showPotCanFlick[i]) {
					yellowText("Flickable Projectiles (Rev2)");
					ImGui::Separator();
					ImGui::TextUnformatted("Whenever some projectile that is possible to reflect could potentially cause problems for you,"
						" like staying active and still being a threat, I will add a (!) marker next to it.\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_SOL);
					drawTextButParenthesesInGrayColor(
						"Gunflame\n"
						"DI Gunflame\n"
						"Break Explosion\n"
						"DI Break Explosion\n"
						"DI Riot Stamp Explosion (but you will be in blockstun from the initial leg hit)\n"
						"DI Ground Viper Fire Pillar (but you will connect with Sol's unflickable direct hit first)\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_KY);
					drawTextButParenthesesInGrayColor(
						"Stun Edge\n"
						"Fortified Stun Edge\n"
						"Charged Stun Edge\n"
						"Fortified Charged Stun Edge (!)\n"
						"j.D\n"
						"5D Projectile\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_MAY);
					drawTextButParenthesesInGrayColor(
						"Applause for the Victim (except when ridden by May)\n"
						"Beach Ball\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_MILLIA);
					drawTextButParenthesesInGrayColor(
						"Silent Force\n"
						"Tandem Top\n"
						"Secret Garden (!)\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_ZATO);
					drawTextButParenthesesInGrayColor(
						"Drill Special (!)\n"
						"Drill\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_POTEMKIN);
					drawTextButParenthesesInGrayColor(
						"F.D.B Fireball\n"
						"Slide Head Shockwave\n"
						"Trishula\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_CHIPP);
					drawTextButParenthesesInGrayColor(
						"Gamma Blade\n"
						"Shuriken\n"
						"Kunai (Wall-K)\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_FAUST);
					drawTextButParenthesesInGrayColor(
						"Bomb\n"
						"Love (the bag, but not the explosion)\n"
						"Oil Fire\n"
						"Meteor Shower\n"
						"Hammer\n"
						"Mini-Faust (after it has landed or when it's walking, but not when it's in the air)\n"
						"Huge Faust (after it has landed or when it's walking, but not when it's in the air)\n"
						"Poison Cloud\n"
						"Platform\n"
						"100-ton Weight (both as it's falling and the shockwave)\n"
						"10,000-ton Weight (both as it's falling and the shockwave)\n"
						"Fireworks\n"
						"Massive Meteor (!)\n"
						"Golden Hammer\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_AXL);
					drawTextButParenthesesInGrayColor(
						"Spindle Spinner\n"
						"Sickle Flash (Rensen)\n"
						"Melody Chain (Rensen-8)\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_VENOM);
					drawTextButParenthesesInGrayColor(
						"Ball Level 1 (including Stinger Aim and Carcass Raid)\n"
						"Ball Level 2 (including Stinger Aim)\n"
						"Ball Level 3-4 (including Stinger Aim) (!)\n"
						"QV Shockwave\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_SLAYER);
					drawTextButParenthesesInGrayColor(
						"Helter-Skelter Shockwave\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_INO);
					drawTextButParenthesesInGrayColor(
						"(Horizontal) Chemical Love\n"
						"Chemical Love Follow-up (but you can't get out of blockstun from the first hit in time)\n"
						"Vertical Chemical Love\n"
						"Antidepressant Scale Level 1-4\n"
						"Antidepressant Scale Level 5 (!)\n"
						"5D Projectile\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_BEDMAN);
					drawTextButParenthesesInGrayColor(
						"Task A Boomerang Head\n"
						"Task A' Boomerang Head (Bedman teleports in front of you and gets hit by the fireball)\n"
						"Deja Vu Task A Boomerang Head\n"
						"Deja Vu Task A' Boomerang Head (Bedman teleports in front of you and gets hit by the fireball)\n"
						"Deja Vu Task B\n"
						"Task C Shockwave\n"
						"Task C Big Shockwave (from big height)\n"
						"Deja Vu Task C\n"
						"Air Deja Vu Task C (!)\n"
						"Air Deja Vu Task C Shockwave\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_RAMLETHAL);
					drawTextButParenthesesInGrayColor(
						"6S Sword Deploy/Redeploy\n"
						"6H Sword Deploy/Redeploy\n"
						"2S Sword Redeploy\n"
						"2H Sword Redeploy\n"
						"j.6S Sword Deploy/Redeploy\n"
						"j.6H Sword Deploy/Redeploy\n"
						"j.2S Sword Redeploy\n"
						"j.2H Sword Redeploy\n"
						"Cassius\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_ELPHELT);
					drawTextButParenthesesInGrayColor(
						"Grenade\n"
						"Grenade Explosion\n"
						"Ms. Confille\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_LEO);
					drawTextButParenthesesInGrayColor(
						"Graviert Wurde\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_JOHNNY);
					drawTextButParenthesesInGrayColor(
						"Coin\n"
						"Zweihander Fire Pillar\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_JACKO);
					drawTextButParenthesesInGrayColor(
						"Knight\n"
						"Lancer\n"
						"Magician\n"
						"j.D\n"
						"Thrown Ghost\n"
						"Self-Detonate Explosion\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_HAEHYUN);
					drawTextButParenthesesInGrayColor(
						"S Tuning Ball\n"
						"H Tuning Ball (!)\n"
						"5D Projectile\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_RAVEN);
					drawTextButParenthesesInGrayColor(
						"Schmerz Berg\n"
						"Grebechlich Licht\n"
						"Scharf Kugel 0-2 Ticks of Excitement\n"
						"Scharf Kugel 3+ Ticks of Excitement (!)\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_DIZZY);
					drawTextButParenthesesInGrayColor(
						"Ice Spike\n"
						"Fire Pillar\n"
						"Ice Scythe\n"
						"Fire Scythe\n"
						"Bubble\n"
						"Fire Bubble\n"
						"Ice Spear\n"
						"1-2 Fire Spears\n"
						"3 Fire Spears (!)\n"
						"Fire Spear Explosion\n"
						"Blue Fish (fish instantly eats the fireball and dies)\n"
						"Laser Fish (fish may eat the fireball and die)\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_BAIKEN);
					drawTextButParenthesesInGrayColor(
						"5D Projectile\n"
						"Yasha Gatana\n"
						"Tatami Gaeshi\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_ANSWER);
					drawTextButParenthesesInGrayColor(
						"Card\n"
						"Clone");
				}
				if (showPotCantFlick[i]) {
					const ImGuiStyle& style = ImGui::GetStyle();
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() + style.FramePadding.y);
					yellowText("Unflickable Projectiles (Rev2)");
					ImGui::SameLine();
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() - style.FramePadding.y);
					ImGui::Checkbox("List Supers", showPotCantFlickIncludeSupers + i);
					ImGui::Separator();
					ImGui::TextUnformatted("You can't reflect overdrives."
						" Some non-overdrive projectiles too.\n");
					if (showPotCantFlickIncludeSupers[i]) {
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_SOL);
						drawTextButParenthesesInGrayColor(
							"Tyrant Rave second punch\n"
							"DI Tyrant Rave Laser\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_KY);
						drawTextButParenthesesInGrayColor(
							"Sacred Edge\n"
							"Fortified Sacred Edge\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_MAY);
						drawTextButParenthesesInGrayColor(
							"Dolphin when it's being ridden by May\n"
							"Deluxe Goshogawara Bomber\n"
							"Great Yamada Attack\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_MILLIA);
						drawTextButParenthesesInGrayColor(
							"Emerald Rain\n"
							"Roses\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_ZATO);
						drawTextButParenthesesInGrayColor(
							"Great White\n"
							"Eddie P\n"
							"Mawaru\n"
							"Nobiru\n"
							"Deadman's Hand\n"
							"Amorphous\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_POTEMKIN);
						drawTextButParenthesesInGrayColor(
							"Giganter Kai\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_CHIPP);
						drawTextButParenthesesInGrayColor(
							"Ryuu Yanagi\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_FAUST);
						drawTextButParenthesesInGrayColor(
							"Flower\n"
							"Love Explosion (but can reflect the bag while it's still flying)\n"
							"Airborne Mini-Faust\n"
							"Airborne Huge Faust\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_AXL);
						drawTextButParenthesesInGrayColor(
							"Sickle Storm\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_VENOM);
						drawTextButParenthesesInGrayColor(
							"Bishop Runout\n"
							"Red Hail\n"
							"Dark Angel\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_SLAYER);
						drawTextButParenthesesInGrayColor(
							"Straight-Down Dandy Backthrusts\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_INO);
						drawTextButParenthesesInGrayColor(
							"Ultimate Fortissimo\n"
							"Longing Desperation\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_BEDMAN);
						drawTextButParenthesesInGrayColor(
							"Sinusoidal Helios\n"
							"Hemi Jack (it's above you, and can't connect with you unless you jump)\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_RAMLETHAL);
						drawTextButParenthesesInGrayColor(
							"Calvados\n"
							"Trance\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_SIN);
						drawTextButParenthesesInGrayColor(
							"Voltec Dein\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_ELPHELT);
						drawTextButParenthesesInGrayColor(
							"j.D\n"
							"Ms. Travailler\n"
							"Genoverse\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_LEO);
						drawTextButParenthesesInGrayColor(
							"Stahl Wirbel\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_JOHNNY);
						drawTextButParenthesesInGrayColor(
							"Bacchus Sigh\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_JACKO);
						drawTextButParenthesesInGrayColor(
							"Calvados\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_JAM);
						drawTextButParenthesesInGrayColor(
							"Renhoukyaku\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_HAEHYUN);
						drawTextButParenthesesInGrayColor(
							"Celestial Tuning Ball\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_DIZZY);
						drawTextButParenthesesInGrayColor(
							"Shield Fish\n"
							"Imperial Ray\n"
							"Gamma Ray\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_BAIKEN);
						drawTextButParenthesesInGrayColor(
							"j.D\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_ANSWER);
						drawTextButParenthesesInGrayColor(
							"Air Firesale\n"
							"Firesale");
					} else {
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_MAY);
						drawTextButParenthesesInGrayColor(
							"Dolphin when it's being ridden by May\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_ZATO);
						drawTextButParenthesesInGrayColor(
							"Eddie P\n"
							"Mawaru\n"
							"Nobiru\n"
							"Deadman's Hand\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_FAUST);
						drawTextButParenthesesInGrayColor(
							"Flower\n"
							"Love Explosion (but can reflect the bag while it's still flying)\n"
							"Airborne Mini-Faust\n"
							"Airborne Huge Faust\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_ELPHELT);
						drawTextButParenthesesInGrayColor(
							"j.D\n"
							"Ms. Travailler\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_JOHNNY);
						drawTextButParenthesesInGrayColor(
							"Bacchus Sigh\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_DIZZY);
						drawTextButParenthesesInGrayColor(
							"Shield Fish\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_BAIKEN);
						drawTextButParenthesesInGrayColor(
							"j.D\n");
					}
				}
				ImGui::PopTextWrapPos();
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
				
				
				if (ImGui::Button("Show Flickable Projectiles")) {
					showFaustCanFlick[i] = !showFaustCanFlick[i];
				}
				ImGui::SameLine();
				if (ImGui::Button("Show Unflickable Projectiles")) {
					showFaustCantFlick[i] = !showFaustCantFlick[i];
				}
				ImGui::PushTextWrapPos(0.F);
				if (showFaustCanFlick[i]) {
					yellowText("Flickable Projectiles (Rev2)");
					ImGui::Separator();
					ImGui::TextUnformatted("Whenever the projectile may still hit you upon reflecting, I will add a (!) marker.\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_SOL);
					drawTextButParenthesesInGrayColor(
						"Gunflame\n"
						"DI Gunflame\n"
						"Break Explosion\n"
						"DI Break Explosion\n"
						"DI Riot Stamp Explosion (!)\n"
						"DI Ground Viper Fire Pillars (! Sol himself hits you)\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_KY);
					drawTextButParenthesesInGrayColor(
						"Stun Edge\n"
						"Fortified Stun Edge\n"
						"Charged Stun Edge (!)\n"
						"Fortified Charged Stun Edge (!)\n"
						"j.D\n"
						"5D Projectile\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_MAY);
					drawTextButParenthesesInGrayColor(
						"Applause for the Victim (except when ridden by May)\n"
						"Beach Ball\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_MILLIA);
					drawTextButParenthesesInGrayColor(
						"Silent Force\n"
						"Tandem Top\n"
						"Secret Garden (!)\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_ZATO);
					drawTextButParenthesesInGrayColor(
						"Drill Special (!)\n"
						"Drill\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_POTEMKIN);
					drawTextButParenthesesInGrayColor(
						"F.D.B Fireball\n"
						"Trishula\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_CHIPP);
					drawTextButParenthesesInGrayColor(
						"Gamma Blade\n"
						"Shuriken\n"
						"Kunai (Wall-K)\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_FAUST);
					drawTextButParenthesesInGrayColor(
						"Your own Item Toss: Oil, Bomb, Black Hole, Helium Gas, Hammer, Mini-Faust, Poison Flask, Chocolate, Donut, Platform, 100-t Weight (need to jump afterwards), Fireworks, Golden Hammer, Big Faust, Valentine's Chocolate, Box of Donuts, 10,000-ton Weight (need to jump afterwards)\n"
						"Other Faust's Oil set on Fire (can reflect, but the baseball instantly clashes with the oil fire and is harmless)\n"
						"Other Faust's Bomb Explosion (can reflect, but the baseball instantly clashes with the bomb explosion and is harmless)\n"
						"Other Faust's Hammer\n"
						"Other Faust's Mini-Faust after it has landed\n"
						"Other Faust's Poison Cloud\n"
						"Other Faust's 100-t Weight (! but instantly get hit by its shockwave. Can't reflect the shockwave)\n"
						"Other Faust's 10,000-t Weight (! but instantly get hit by its shockwave. Can't reflect the shockwave)\n"
						"Other Faust's Fireworks Explosion (but the baseball instantly clashes with the explosion and is harmless)\n"
						"Other Faust's Golden Hammer\n"
						"Other Faust's Huge Faust when it's on the ground (but not when it's airborne)\n"
						"Other Faust's Love (the bag, but not the explosion)\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_AXL);
					drawTextButParenthesesInGrayColor(
						"Melody Chain (Rensen-8) (it should be impossible to avoid the Sickle Flash that comes before this, though)\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_VENOM);
					drawTextButParenthesesInGrayColor(
						"Ball Lvl 1\n"
						"Ball Lvl 2, Lvl 3, Lvl 4 (you do generate a baseball and it does not clash with Venom's ball and is harmful to Venom, but you get hit by the next hits of Venom's ball)\n"
						"QV Shockwave\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_SLAYER);
					drawTextButParenthesesInGrayColor(
						"Helter Skelter Shockwave\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_INO);
					drawTextButParenthesesInGrayColor(
						"(Horizontal) Chemical Love\n"
						"Vertical Chemical Love\n"
						"Antidepressant Scale Lvl 1 (1 hit)\n"
						"Antidepressant Scale Lvl 2, Lvl 3, Lvl 4, Lvl 5 (2, 3, 4, 5 hits) (you do generate a baseball, but you get hit anyway by the second hit of the note)\n"
						"5D Projectile\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_BEDMAN);
					drawTextButParenthesesInGrayColor(
						"Task A Boomerang Head\n"
						"Task A' Boomerang Head (baseball is created but Bedman doesn't get hit by it and teleports to you anyway)\n"
						"Task C Shockwave\n"
						"Deja Vu Task A Boomerang Head\n"
						"Deja Vu Task A' Boomerang Head (baseball is created but Bedman doesn't get hit by it and teleports to you anyway)\n"
						"Deja Vu Task B (baseball is created successfully, but you get hit by the beyblade anyway)\n"
						"Deja Vu Task C First Hit (you reflect the first hit, baseball gets flung, and the bed spirit just flies away; there is no second hit)\n"
						"Deja Vu Task C Second Hit (baseball gets created, but you get hit by the shockwave anyway)\n"
						"Deja Vu Task C Shockwave\n"
						"Deja Vu Air Task C\n"
						"Deja Vu Air Task C Shockwave\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_RAMLETHAL);
					drawTextButParenthesesInGrayColor(
						"Baseball can hit the Sword and not become harmless, because hitting the Sword does not waste the baseball's hit\n"
						"6S Sword Deploy\n"
						"6H Sword Deploy\n"
						"j.6S Sword Deploy\n"
						"j.6H Sword Deploy\n"
						"2S Sword Redeploy\n"
						"2H Sword Redeploy\n"
						"j.2S Sword Redeploy\n"
						"j.2H Sword Redeploy\n"
						"6S Sword Redeploy\n"
						"6H Sword Redeploy\n"
						"j.6S Sword Redeploy\n"
						"j.6H Sword Redeploy\n"
						"Cassius\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_ELPHELT);
					drawTextButParenthesesInGrayColor(
						"Grenade\n"
						"Grenade Explosion\n"
						"Ms. Confille (but not max charge)\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_LEO);
					drawTextButParenthesesInGrayColor(
						"Graviert Wurde (creates baseball, but you get hit anyway by the next hits)\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_JOHNNY);
					drawTextButParenthesesInGrayColor(
						"Coin\n"
						"Zweihander Fire Pillar\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_JACKO);
					drawTextButParenthesesInGrayColor(
						"Knight\n"
						"Lancer\n"
						"j.D\n"
						"Thrown Ghost\n"
						"Self-Detonate Explosion\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_HAEHYUN);
					drawTextButParenthesesInGrayColor(
						"S Tuning Ball\n"
						"H Tuning Ball (!)\n"
						"5D Projectile\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_RAVEN);
					drawTextButParenthesesInGrayColor(
						"Schmerz Berg\n"
						"Grebechlich Licht\n"
						"Scharf Kugel (!)\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_DIZZY);
					drawTextButParenthesesInGrayColor(
						"Ice Spike\n"
						"Fire Pillar\n"
						"Ice Scythe\n"
						"Fire Scythe (baseball clashes with the scythe)\n"
						"Bubble Explosion\n"
						"Fire Bubble Explosion\n"
						"Ice Spear\n"
						"1 Fire Spear\n"
						"2-3 Fire Spears (! you get hit by the next spears)\n"
						"Fire Spear Explosion\n"
						"Blue Fish\n"
						"Laser Fish\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_BAIKEN);
					drawTextButParenthesesInGrayColor(
						"5D Projectile\n"
						"Yasha Gatana\n"
						"Tatami Gaeshi\n");
					drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_ANSWER);
					drawTextButParenthesesInGrayColor(
						"Card\n"
						"Clone\n");
				}
				if (showFaustCantFlick[i]) {
					const ImGuiStyle& style = ImGui::GetStyle();
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() + style.FramePadding.y);
					yellowText("Unflickable Projectiles (Rev2)");
					ImGui::SameLine();
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() - style.FramePadding.y);
					ImGui::Checkbox("List Supers", showFaustCantFlickIncludeSupers + i);
					ImGui::Separator();
					ImGui::TextUnformatted("Can't reflect overdrives, unblockables and some other exceptional projectiles.\n");
					if (showFaustCantFlickIncludeSupers[i]) {
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_SOL);
						drawTextButParenthesesInGrayColor(
							"Tyrant Rave second punch\n"
							"DI Tyrant Rave Laser\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_KY);
						drawTextButParenthesesInGrayColor(
							"Sacred Edge\n"
							"Fortified Sacred Edge\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_MAY);
						drawTextButParenthesesInGrayColor(
							"Dolphin when it's being ridden by May\n"
							"Great Yamada Attack\n"
							"Deluxe Goshogawara Bomber\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_MILLIA);
						drawTextButParenthesesInGrayColor(
							"Emerald Rain\n"
							"Roses\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_ZATO);
						drawTextButParenthesesInGrayColor(
							"Eddie P\n"
							"Mawaru\n"
							"Nobiru\n"
							"Great White\n"
							"Amorphous\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_POTEMKIN);
						drawTextButParenthesesInGrayColor(
							"Slide Head\n"
							"Giganter\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_CHIPP);
						drawTextButParenthesesInGrayColor(
							"Ryuu Yanagi\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_FAUST);
						drawTextButParenthesesInGrayColor(
							"Own Oil set on fire\n"
							"Own Bomb Explosion\n"
							"Own Platform when it's already on the ground\n"
							"Own 100-t Weight Shockwave\n"
							"Own Fireworks Explosion\n"
							"Own 10,000-ton Weight Shockwave\n"
							"Own Love Explosion\n"
							"Flower\n"
							"You can't reflect other Faust's thrown items, unless they have a hitbox\n"
							"Other Faust's Meteor Shower (baseball instantly clashes with another meteor, and you get hit by a third meteor)\n"
							"Other Faust's Massive Meteor (baseball clashes with the next hit of the Meteor, and you get hit by that same next hit anyway)\n"
							"Other Faust's airborne Mini-Faust, but can after it has landed\n"
							"Other Faust's Platform\n"
							"Other Faust's 100-t Weight Shockwave\n"
							"Other Faust's 10,000-t Weight Shockwave\n"
							"Other Faust's Huge Faust when it's Airborne\n"
							"Other Faust's Love Explosion (but can reflect the bag while it's still flying)\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_AXL);
						drawTextButParenthesesInGrayColor(
							"Spindle Spinner\n"
							"Sickle Flash (baseball clashes with the projectile instantly, and you get hit on the next frame by another hit)\n"
							"Sickle Storm\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_VENOM);
						drawTextButParenthesesInGrayColor(
							"Bishop Runout\n"
							"Red Hail\n"
							"Dark Angel\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_SLAYER);
						drawTextButParenthesesInGrayColor(
							"Straight-Down Dandy Backthrusts\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_INO);
						drawTextButParenthesesInGrayColor(
							"Ultimate Fortissimo\n"
							"Longing Desperation\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_BEDMAN);
						drawTextButParenthesesInGrayColor(
							"Sinusoidal Helios\n"
							"Hemi Jack\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_RAMLETHAL);
						drawTextButParenthesesInGrayColor(
							"Calvados\n"
							"Trance\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_SIN);
						drawTextButParenthesesInGrayColor(
							"Voltec Dein\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_ELPHELT);
						drawTextButParenthesesInGrayColor(
							"Ms. Travailler\n"
							"Ms. Confille Max Charge\n"
							"Genoverse\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_JOHNNY);
						drawTextButParenthesesInGrayColor(
							"Bacchus Sigh\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_JACKO);
						drawTextButParenthesesInGrayColor(
							"Calvados\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_JAM);
						drawTextButParenthesesInGrayColor(
							"Renhoukyaku\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_HAEHYUN);
						drawTextButParenthesesInGrayColor(
							"Celestial Tuning Ball\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_DIZZY);
						drawTextButParenthesesInGrayColor(
							"Shield Fish\n"
							"Imperial Ray\n"
							"Gamma Ray\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_BAIKEN);
						drawTextButParenthesesInGrayColor(
							"j.D\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_ANSWER);
						drawTextButParenthesesInGrayColor(
							"Air Firesale\n"
							"Firesale\n");
					} else {
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_MAY);
						drawTextButParenthesesInGrayColor(
							"Dolphin when it's being ridden by May\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_ZATO);
						drawTextButParenthesesInGrayColor(
							"Eddie P\n"
							"Mawaru\n"
							"Nobiru\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_POTEMKIN);
						drawTextButParenthesesInGrayColor(
							"Slide Head\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_FAUST);
						drawTextButParenthesesInGrayColor(
							"Own Oil set on fire\n"
							"Own Bomb Explosion\n"
							"Own Platform when it's already on the ground\n"
							"Own 100-t Weight Shockwave\n"
							"Own Fireworks Explosion\n"
							"Own 10,000-ton Weight Shockwave\n"
							"Own Love Explosion\n"
							"Flower\n"
							"You can't reflect other Faust's thrown items, unless they have a hitbox\n"
							"Other Faust's Meteor Shower (baseball instantly clashes with another meteor, and you get hit by a third meteor)\n"
							"Other Faust's Massive Meteor (baseball clashes with the next hit of the Meteor, and you get hit by that same next hit anyway)\n"
							"Other Faust's airborne Mini-Faust, but can after it has landed\n"
							"Other Faust's Platform\n"
							"Other Faust's 100-t Weight Shockwave\n"
							"Other Faust's 10,000-t Weight Shockwave\n"
							"Other Faust's Huge Faust when it's Airborne\n"
							"Other Faust's Love Explosion (but can reflect the bag while it's still flying)\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_AXL);
						drawTextButParenthesesInGrayColor(
							"Spindle Spinner\n"
							"Sickle Flash (baseball clashes with the projectile instantly, and you get hit on the next frame by another hit)\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_ELPHELT);
						drawTextButParenthesesInGrayColor(
							"Ms. Travailler\n"
							"Ms. Confille Max Charge\n"
							"Genoverse\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_JOHNNY);
						drawTextButParenthesesInGrayColor(
							"Bacchus Sigh\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_DIZZY);
						drawTextButParenthesesInGrayColor(
							"Shield Fish\n");
						drawFontSizedPlayerIconWithCharacterName(CHARACTER_TYPE_BAIKEN);
						drawTextButParenthesesInGrayColor(
							"j.D\n");
					}
				}
				ImGui::PopTextWrapPos();
				
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
							sprintf_s(strbuf, "%d/4", p.storage(1));
							ImGui::TextUnformatted(strbuf);
						}
						if (info.isBishop) {
							textUnformattedColored(LIGHT_BLUE_COLOR, searchFieldTitle("Bishop Level:"));
							ImGui::SameLine();
							sprintf_s(strbuf, "%d/5", p.storage(2));
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
					txt = "68";
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
				
				printBedmanSeals(player.bedmanInfo, false);
			} else if (player.charType == CHARACTER_TYPE_RAMLETHAL) {
				
				if (player.index == 0) {
					booleanSettingPreset(settings.p1RamlethalDisableMarteliForpeli);
				} else {
					booleanSettingPreset(settings.p2RamlethalDisableMarteliForpeli);
				}
				
				if (player.index == 0) {
					booleanSettingPreset(settings.p1RamlethalUseBoss6SHSwordDeploy);
				} else {
					booleanSettingPreset(settings.p2RamlethalUseBoss6SHSwordDeploy);
				}
				
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
					bool isInvulnerable;
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
						player.ramlethalSSwordAnim,
						player.ramlethalSSwordInvulnerable
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
						player.ramlethalHSwordAnim,
						player.ramlethalHSwordInvulnerable
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
							sprintf_s(strbuf, "(until landing + %d)%s", bitInfo.time,
								bitInfo.isInvulnerable ? " (Invulnerable)" : "");
						} else {
							sprintf_s(strbuf, "(%d/%d)%s", bitInfo.time, bitInfo.timeMax,
								bitInfo.isInvulnerable ? " (Invulnerable)" : "");
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
				
				if (ImGui::Button("Show Reflectable Projectiles")) {
					showLeoReflectableProjectiles[i] = !showLeoReflectableProjectiles[i];
				}
				if (showLeoReflectableProjectiles[i]) {
					printReflectableProjectilesList();
				}
				
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
				const char* aegisPresent = player.jackoAegisActive ? "active" : "inactive";
				if (player.jackoAegisReturningIn != INT_MIN) {
					if (player.jackoAegisReturningIn > 60) {
						sprintf_s(strbuf, "%d/%d (%s) (returns in <never>)", player.jackoAegisTimeWithSlow, player.jackoAegisTimeMaxWithSlow,
							aegisPresent);
					} else {
						sprintf_s(strbuf, "%d/%d (%s) (returns in %d/60)", player.jackoAegisTimeWithSlow, player.jackoAegisTimeMaxWithSlow,
							aegisPresent, 60 - player.jackoAegisReturningIn);
					}
				} else {
					sprintf_s(strbuf, "%d/%d (%s)", player.jackoAegisTimeWithSlow, player.jackoAegisTimeMaxWithSlow,
						aegisPresent);
				}
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
				DWORD playerval2 = (DWORD)player.pawn.playerVal(2);
				for (int j = 0; j < 3; ++j) {
					GhostInfo& info = ghosts[j];
					textUnformattedColored(LIGHT_BLUE_COLOR, searchFieldTitle(info.title));
					Entity p = player.pawn.stackEntity(j);
					if (p && p.isActive() && p.bbscrCurrentFunc()) {
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
									&& strcmp(serv.animationName(), info.servantCooldownName) == 0
									&& serv.bbscrCurrentFunc()) {
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
							ImGui::TextUnformatted(ghostStateNames[stateInd].name);
						} else {
							ImGui::TextUnformatted("???");
						}
						
					} else {
						int duration = 0;
						for (int k = 2; k < entityList.count; ++k) {
							p = entityList.list[k];
							if (p.isActive() && p.team() == i && !p.isPawn()
									&& strcmp(p.animationName(), info.dummyName) == 0
									&& p.bbscrCurrentFunc()) {
								if (*info.dummyTotalFrames == 0) {
									BYTE* func = p.bbscrCurrentFunc();
									BYTE* instr;
									for (
											instr = moves.skipInstr(func);
											moves.instrType(instr) != instr_endState;
											instr = moves.skipInstr(instr)
									) {
										if (moves.instrType(instr) == instr_sprite) {
											*info.dummyTotalFrames += asInstr(instr, sprite)->duration;
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
					const ServantState* stateNames;
					int stateNamesCount;
					Moves::MayIrukasanRidingObjectInfo* servantAtk;
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
						if (!projectile || !projectile->ptr || !projectile->ptr.bbscrCurrentFunc()) continue;
						
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
							ImGui::TextUnformatted(servant.stateNames[state].name);
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
					imguiContextMenuOpen = true;
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
				
				textUnformattedColored(PURPLE_COLOR, searchFieldTitle("Purple Health:"));
				if (player.pawn && *aswEngine) {
					sprintf_s(strbuf, "%d(%d+%d)/%d (%df)",
						player.pawn.hp() + player.pawn.purpleHealth(),
						player.pawn.hp(),
						player.pawn.purpleHealth(),
						player.pawn.maxHp(),
						player.pawn.purpleHealthTimer());
					ImGui::SameLine();
					ImGui::TextUnformatted(strbuf);
				}
				
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
				
				if (ImGui::Button("Show Reflectable Projectiles")) {
					showDizzyReflectableProjectiles[i] = !showDizzyReflectableProjectiles[i];
				}
				if (showDizzyReflectableProjectiles[i]) {
					printReflectableProjectilesList();
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
				if (strcmp(player.anim, "Ami_Hold") == 0 && player.pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED)) {
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
			customEnd();
			ImGui::PopID();
		}
	}
	popSearchStack();
	searchCollapsibleSection(PinnedWindowEnum_BoxExtents);
	if (needDraw(PinnedWindowEnum_BoxExtents) || searching) {
		customBegin(PinnedWindowEnum_BoxExtents);
		if (endScene.isIGiveUp() && !searching) {
			ImGui::TextUnformatted("Online non-observer match running.");
		} else
		if (ImGui::BeginTable("##PlayerData", 3, tableFlags)) {
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
		customEnd();
	}
	popSearchStack();
	searchCollapsibleSection("Cancels");
	for (int i = 0; i < two; ++i) {
		if (needDrawPair(PinnedWindowEnum_Cancels, i) || searching) {
			ImGui::PushID(i);
			ImGui::SetNextWindowSize({
				ImGui::GetFontSize() * 35.F,
				150.F
			}, ImGuiCond_FirstUseEver);
			customBeginPair(PinnedWindowEnum_Cancels, i);
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
				printAllCancels(cancels,
					cancels.enableSpecialCancel,
					cancels.clashCancelTimer,
					cancels.enableJumpCancel,
					cancels.enableSpecials,
					cancels.hitAlreadyHappened,
					cancels.airborne,
					cancels.canYrc,
					false,
					false);
			}
			if (!printedSomething) {
				ImGui::TextUnformatted("No cancels available.");
			}
			
			ImGui::PopTextWrapPos();
			const GGIcon* scaledIcon = get_tipsIconScaled14();
			drawGGIcon(*scaledIcon);
			AddTooltip(thisHelpTextWillRepeat);
			customEnd();
			ImGui::PopID();
		}
	}
	popSearchStack();
	searchCollapsibleSection("Damage/RISC/Stun Calculation");
	for (int i = 0; i < two; ++i) {
		if (needDrawPair(PinnedWindowEnum_DamageCalculation, i) || searching) {
			ImGui::PushID(i);
			ImGui::SetNextWindowSize({
				ImGui::GetFontSize() * 35.F,
				150.F
			}, ImGuiCond_FirstUseEver);
			customBeginPair(PinnedWindowEnum_DamageCalculation, i);
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
					if (dmgCalc.attackName) {
						ImGui::TextUnformatted(useSlang && dmgCalc.attackName->slang ? dmgCalc.attackName->slang : dmgCalc.attackName->name);
						if (dmgCalc.nameFull || useSlang && dmgCalc.attackName->slang && dmgCalc.attackName->name) {
							AddTooltip(dmgCalc.nameFull ? dmgCalc.nameFull : dmgCalc.attackName->name);
						}
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
							if (ImGui::BeginTable("##DmgCalc", 2, tableFlags)) {
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
						if (ImGui::BeginTable("##DmgCalc", 2, tableFlags)) {
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
						if (ImGui::BeginTable("##DmgCalc", 2, tableFlags)) {
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
							
							bool hellfire = (data.attackerHellfireState || data.attackerHpLessThan10Percent) && data.attackHasHellfireEnabled;
							ImGui::TableNextColumn();
							ImGui::TextUnformatted(searchFieldTitle("Hellfire"));
							AddTooltip(searchTooltip("To gain 20% damage bonus, the attacker must have hellfire state enabled, they must have <= 10% HP (<= 42 HP),"
								" and the attack must be Hellfire-enabled. All overdrives should be Hellfire-enabled."));
							ImGui::TableNextColumn();
							if (hellfire) {
								ImGui::TextUnformatted("Yes (120%)");
							} else if (data.attackHasHellfireEnabled) {
								ImGui::TextUnformatted("No (100%)");
							} else if (dmgCalc.attackType == ATTACK_TYPE_OVERDRIVE) {
								ImGui::TextUnformatted("No, attack does not allow hellfire (100%)");
							} else {
								ImGui::TextUnformatted("No, not a super (100%)");
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
							AddTooltip(searchTooltip("Depends on the attack."
								" May only be applied on first hit."
								" Does not prorate the current hit, only the consecutive hits."));
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
								sprintf_s(strbuf, "Not first hit (would be %d%c otherwise)", data.initialProration, '%');
								ImGui::TextUnformatted(strbuf);
							}
							
							ImGui::TableNextColumn();
							ImGui::TextUnformatted(searchFieldTitle("Forced Proration"));
							AddTooltip(searchTooltip("Depends on the attack."
								" May be applied at any point in the combo, not just on first hit."
								" Only gets applied if lower than the 'Current Proration'."
								" Does not prorate the current hit, only the consecutive hits."));
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
							AddTooltip(searchTooltip("The proration that is chosen out of initial or forced prorations (the lowest is chosen)"
								" that will apply to the consecutive combo."));
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
								tooltip = "Total RISC- that will be applied by the current hit is"
									" RISC- Initial + RISC- + RISC- Once + Extra RISC-.";
								textUnformattedColored(LIGHT_BLUE_COLOR, searchFieldTitle("RISC-"));
								AddTooltip(tooltip);
								ImGui::SameLine();
								ImGui::TextUnformatted(" Total");
								AddTooltip(tooltip);
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
								ImGui::TextUnformatted("Next RISC");
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
							ImGui::TextUnformatted(" * (Pror. * Scaling)");
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
							AddTooltip(searchTooltip("Minimum Damage Percent. Depends on the attack."));
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
			
			const GGIcon* scaledIcon = get_tipsIconScaled14();
			drawGGIcon(*scaledIcon);
			AddTooltip("Hover your mouse over individual field titles or field values (depends on each field or even sometimes current"
				" field value) to see their tooltips.");
			
			customEnd();
			ImGui::PopID();
		}
	}
	popSearchStack();
	searchCollapsibleSection("Stun/Stagger Mash");
	bool useAlternativeStaggerMashProgressDisplayUse = settings.useAlternativeStaggerMashProgressDisplay;
	for (int i = 0; i < two; ++i) {
		if (needDrawPair(PinnedWindowEnum_StunMash, i) || searching) {
			ImGui::PushID(i);
			ImGui::SetNextWindowSize({
				ImGui::GetFontSize() * 35.F,
				180.F
			}, ImGuiCond_FirstUseEver);
			customBeginPair(PinnedWindowEnum_StunMash, i);
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
					if (funcStart && strcmp(asInstr(funcStart, beginState)->name, "CmnActKizetsu") == 0) {
						BYTE* markerPos = moves.findSetMarker(funcStart, "_End");
						if (markerPos) {
							BYTE* currentInst = player.pawn.bbscrCurrentInstr();
							int currentDuration = 0;
							int totalDuration = 0;
							int lastDuration = 0;
							BYTE* instIt = moves.skipInstr(markerPos);
							while (moves.instrType(instIt) != instr_endState) {
								lastDuration = asInstr(instIt, sprite)->duration;
								totalDuration += lastDuration;
								if (instIt < currentInst) {
									currentDuration += lastDuration;
								}
								instIt = moves.skipInstr(instIt);
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
					
					drawGGIcon(*get_tipsIconScaled14());
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
			
			customEnd();
			ImGui::PopID();
		}
	}
	popSearchStack();
	searchCollapsibleSection("Combo Recipe");
	for (int i = 0; i < two; ++i) {
		if (needDrawPair(PinnedWindowEnum_ComboRecipe, i) || searching) {
			ImGui::PushID(i);
			ImGui::SetNextWindowSize({ 300.F, 300.F }, ImGuiCond_FirstUseEver);
			customBeginPair(PinnedWindowEnum_ComboRecipe, i);
			PlayerInfo& player = endScene.players[i];
			
			drawPlayerIconInWindowTitle(i);
			
			const bool transparentBackground = settings.comboRecipe_transparentBackground;
			for (
					CogwheelButtonContext cogwheel(
						"##ComboRecipeCogwheel",
						showComboRecipeSettings +i,
						transparentBackground,
						"Settings for the Combo Recipe panel.\n"
							"The settings are shared between P1 and P2.\n"
							"\n"
							"Extra Info: The '>' symbol at the end of a move means that the next move is being cancelled from that move.\n"
							"The ',' symbol at the end of a move means that the next move is being linked or done after some idle time after that move."
					);
					cogwheel.needShowSettings(); ) {
				settingsPresetsUseOutlinedText = transparentBackground;
				booleanSettingPreset(settings.comboRecipe_showDelaysBetweenCancels);
				booleanSettingPreset(settings.comboRecipe_showIdleTimeBetweenMoves);
				booleanSettingPreset(settings.comboRecipe_showDashes);
				booleanSettingPreset(settings.comboRecipe_showWalks);
				booleanSettingPreset(settings.comboRecipe_showSuperJumpInstalls);
				booleanSettingPreset(settings.comboRecipe_showNumberOfHits);
				booleanSettingPreset(settings.comboRecipe_showCharge);
				booleanSettingPreset(settings.comboRecipe_clearOnPositionReset);
				booleanSettingPreset(settings.comboRecipe_transparentBackground);
				settingsPresetsUseOutlinedText = false;
			}
			
			if (ImGui::BeginTable("##ComboRecipe", 1, tableFlags)) {
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
				
				if (transparentBackground) {
					pushOutlinedText(true);
				}
				
				int idleTimeAdd = 0;
				
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
					
					if (!elem.doneAfterIdle) {
						idleTimeAdd = 0;
					}
					int delayModif = elem.cancelDelayedBy + idleTimeAdd;
					
					bool goingToShowTheElement;
					if (elem.dashDuration) {
						if (elem.isWalkForward || elem.isWalkBackward) {
							goingToShowTheElement = settings.comboRecipe_showWalks;
						} else {
							goingToShowTheElement = settings.comboRecipe_showDashes;
						}
					} else if (elem.isSuperJumpInstall) {
						goingToShowTheElement = settings.comboRecipe_showSuperJumpInstalls;
					} else {
						goingToShowTheElement = true;
					}
					
					if (delayModif
							&& (
								elem.doneAfterIdle
									? settings.comboRecipe_showIdleTimeBetweenMoves
									: settings.comboRecipe_showDelaysBetweenCancels
							)) {
						
						int correctedCancelDelayedBy = delayModif;
						if (lastElemIsDelayed) {
							const ComboRecipeElement& lastElem = player.comboRecipe[j - 1];
							int timeDifference = (int)elem.timestamp - (int)lastElem.timestamp;
							if (elem.cancelDelayedBy + 1 > timeDifference) {
								correctedCancelDelayedBy = timeDifference - 1 + idleTimeAdd;
							}
						}
						
						lastElemIsDelayed = correctedCancelDelayedBy > 0;
						
						if (lastElemIsDelayed) {
							
							if (goingToShowTheElement) {
								
								ImGui::TableNextColumn();
								sprintf_s(strbuf, "%u)", rowCount++);
								yellowText(strbuf);
								ImGui::SameLine();
								
								ImGui::PushStyleColor(ImGuiCol_Text, SLIGHTLY_GRAY);
								if (elem.doneAfterIdle) {
									if (elem.shotgunMaxCharge && elem.shotgunChargeSkippedFrames != 255) {
										if (!elem.shotgunChargeSkippedFrames) {
											// the displayed max charge doesn't account for RC slowdown, while the displayed current charge does
											// this doesn't matter because the one performing the combo can't be slowed down by RC
											sprintf_s(strbuf, "(Idle %d/%df)", correctedCancelDelayedBy, elem.shotgunMaxCharge);
										} else {
											sprintf_s(strbuf, "(Idle %df) (Shotgun Charge: %d/%df)", correctedCancelDelayedBy,
												(int)correctedCancelDelayedBy + (int)elem.shotgunChargeSkippedFrames,
												elem.shotgunMaxCharge);
										}
									} else {
										sprintf_s(strbuf, "(Idle %df)", correctedCancelDelayedBy);
									}
									idleTimeAdd = 0;
								} else {
									sprintf_s(strbuf, "(Delay %df)", elem.cancelDelayedBy);
								}
								ImGui::TextUnformatted(strbuf);
								ImGui::PopStyleColor();
							} else if (elem.doneAfterIdle) {
								idleTimeAdd += elem.cancelDelayedBy;
							}
							
						}
					} else {
						lastElemIsDelayed = false;
					}
					
					if (!goingToShowTheElement) {
						if (elem.dashDuration) {
							idleTimeAdd += elem.dashDuration;
						}
						continue;
					}
					
					const char* chosenName;
					if (elem.name) {
						if (settings.useSlangNames && elem.name->slang) {
							chosenName = elem.name->slang;
						} else {
							chosenName = elem.name->name;
						}
					} else {
						chosenName = nullptr;
					}
					
					idleTimeAdd = 0;
					ImGui::TableNextColumn();
					sprintf_s(strbuf, "%u)", rowCount++);
					yellowText(strbuf);
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
						char* buf = strbuf;
						size_t bufSize = sizeof strbuf;
						int result = sprintf_s(strbuf, "%s", chosenName);
						advanceBuf
						if (elem.hitCount > 1 && settings.comboRecipe_showNumberOfHits) {
							result = sprintf_s(buf, bufSize, "(%d)", elem.hitCount);
							advanceBuf
						}
						if ((elem.charge || elem.maxCharge) && settings.comboRecipe_showCharge) {
							if (elem.maxCharge) {
								result = sprintf_s(buf, bufSize, " (Held: %d/%df)", elem.charge, elem.maxCharge);
							} else {
								result = sprintf_s(buf, bufSize, " (Held: %df)", elem.charge);
							}
							advanceBuf
						}
						sprintf_s(buf, bufSize, "%s%s%s",
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
						textUnformattedColored(LIGHT_BLUE_COLOR, strbuf);
					} else {
						ImGui::TextUnformatted(strbuf);
					}
				}
				
				if (transparentBackground) {
					popOutlinedText();
				}
				ImGui::EndTable();
			}
			
			float totalViewableArea = ImGui::GetWindowHeight() - ImGui::GetStyle().FramePadding.y * 2 - ImGui::GetFontSize();
			float totalContentSize = ImGui::GetCursorPosY();
			if (comboRecipeUpdatedOnThisFrame[i]) {
				comboRecipeUpdatedOnThisFrame[i] = false;
				// simply calling ImGui::SetScrollY(ImGui::GetScrollMaxY()) made it scroll to the penultimate line
				// probably because ImGui::GetScrollMaxY() returns the value from the ImGui::Begin call so it can be compared to ImGui::GetScrollY(),
				// also from that call
				if (totalContentSize > totalViewableArea) {
					ImGui::SetScrollY(totalContentSize - totalViewableArea);
				}
			}
			
			customEnd();
			ImGui::PopID();
		}
	}
	popSearchStack();
	searchCollapsibleSection(PinnedWindowEnum_HighlightedCancels);
	if (needDraw(PinnedWindowEnum_HighlightedCancels) || searching) {
		ImGui::SetNextWindowSize({ 300.F, 300.F }, ImGuiCond_FirstUseEver);
		customBegin(PinnedWindowEnum_HighlightedCancels);
		if (sortedMovesRedoPending || sortedMovesRedoPendingWhenAswEngingExists && *aswEngine) {
			sortedMovesRedoPending = false;
			if (*aswEngine) sortedMovesRedoPendingWhenAswEngingExists = false;
			for (std::vector<SortedMovesEntry>& vec : sortedMoves) {
				vec.clear();
			}
			MoveInfo moveInfo;
			if (*aswEngine) {
				entityList.populate();
				for (int playerInd = 0; playerInd < 2; ++playerInd) {
					if (playerInd == 1 && entityList.slots[0].characterType() == entityList.slots[1].characterType()) break;
					Entity pawn = entityList.slots[playerInd];
					CharacterType charType = pawn.characterType();
					std::vector<SortedMovesEntry>& vec = sortedMoves[charType];
					const int movesCount = pawn.movesCount();
					const AddedMoveData* addedMove = pawn.movesBase();
					for (int moveInd = 0; moveInd < movesCount; ++moveInd) {
						bool isStylish = false;
						for (int inputInd = 0; inputInd < _countof(addedMove->inputs); ++inputInd) {
							if (addedMove->inputs[inputInd] == INPUT_PRESS_SPECIAL) {
								isStylish = true;
								break;
							}
						}
						if (!isStylish) {
							const char* name = nullptr;
							void const* sortValue = nullptr;
							if (moves.getInfoWithName(moveInfo, charType, addedMove->name, addedMove->stateName, false, &name, &sortValue)
									&& moveInfo.isMove) {
								vec.emplace_back(name, moveInfo.displayName, -1, sortValue);
							}
						}
						++addedMove;
					}
				}
			}
			int index = 0;
			for (const MoveListPointer& ptr : settings.highlightWhenCancelsIntoMovesAvailable.pointers) {
				bool alreadyIncluded = false;
				std::vector<SortedMovesEntry>& vec = sortedMoves[ptr.charType];
				for (SortedMovesEntry& iter : vec) {
					if (iter.name == ptr.name) {
						iter.index = index;
						alreadyIncluded = true;
						break;
					}
				}
				if (!alreadyIncluded) {
					const NamePair* displayName = nullptr;
					const char* name = nullptr;
					void const* sortValue = nullptr;
					if (moves.getInfoWithName(moveInfo, ptr.charType, ptr.name, false, &name, &sortValue)) {
						displayName = moveInfo.displayName;
					}
					vec.emplace_back(ptr.name, displayName, index, sortValue);
				}
				++index;
			}
			for (std::vector<SortedMovesEntry>& vec : sortedMoves) {
				if (vec.empty()) continue;
				qsort(vec.data(), vec.size(), sizeof SortedMovesEntry, CompareMoveInfo);
			}
		}
		
		if (!*aswEngine) {
			ImGui::PushStyleColor(ImGuiCol_Text, SLIGHTLY_GRAY);
			ImGui::TextUnformatted("Load a match to see more moves.");
			ImGui::PopStyleColor();
		}
		
		std::vector<MoveListPointer>& jesusFuckingChrist = settings.highlightWhenCancelsIntoMovesAvailable.pointers;
		const bool slang = settings.useSlangNames;
		for (int charIter = CHARACTER_TYPE_SOL; charIter <= CHARACTER_TYPE_ANSWER; ++charIter) {
			std::vector<SortedMovesEntry>& vec = sortedMoves[charIter];
			if (vec.empty()) continue;
			ImGui::SeparatorText(characterNames[charIter]);
			sprintf_s(strbuf, "##HighlightedMoves%d", charIter);
			if (ImGui::BeginTable(strbuf, 4, tableFlags)) {
				ImGui::TableSetupColumn("Move", ImGuiTableColumnFlags_WidthStretch, 1.F);
				ImGui::TableSetupColumn("Red", ImGuiTableColumnFlags_WidthStretch, 0.11F);
				ImGui::TableSetupColumn("Green", ImGuiTableColumnFlags_WidthStretch, 0.11F);
				ImGui::TableSetupColumn("Blue", ImGuiTableColumnFlags_WidthStretch, 0.11F);
				ImGui::TableHeadersRow();
				int row = -1;
				for (SortedMovesEntry& iter : vec) {
					++row;
					ImGui::TableNextColumn();
					if (iter.displayName) {
						ImGui::TextUnformatted(slang && iter.displayName->slang ? iter.displayName->slang : iter.displayName->name);
						if (slang && iter.displayName->slang) {
							AddTooltip(iter.displayName->name);
						}
					}
					
					bool red = false;
					bool green = false;
					bool blue = false;
					if (iter.index != -1) {
						const MoveListPointer& ptr = jesusFuckingChrist[iter.index];
						red = ptr.red;
						green = ptr.green;
						blue = ptr.blue;
					}
					bool changedColor = false;
					ImGui::TableNextColumn();
					sprintf_s(strbuf, "##HighlightedMovesRed%d_%d", charIter, row);
					if (ImGui::Checkbox(strbuf, &red)) {
						changedColor = true;
					}
					ImGui::TableNextColumn();
					sprintf_s(strbuf, "##HighlightedMovesGreen%d_%d", charIter, row);
					if (ImGui::Checkbox(strbuf, &green)) {
						changedColor = true;
					}
					ImGui::TableNextColumn();
					sprintf_s(strbuf, "##HighlightedMovesBlue%d_%d", charIter, row);
					if (ImGui::Checkbox(strbuf, &blue)) {
						changedColor = true;
					}
					if (changedColor) {
						if (iter.index != -1) {
							if (iter.index >= 0 && iter.index < (int)jesusFuckingChrist.size()) {
								jesusFuckingChrist.erase(jesusFuckingChrist.begin() + iter.index);
								for (SortedMovesEntry& iterModif : vec) {
									if (iterModif.index > iter.index) {
										--iterModif.index;
									}
								}
							}
							iter.index = -1;
						}
						if (red || green || blue) {
							int satisfactionLevel = 0;
							int foundIndex = 0;
							for (int listInd = 0; listInd < (int)jesusFuckingChrist.size(); ++listInd) {
								MoveListPointer& ptr = jesusFuckingChrist[listInd];
								int currentSatisfaction = 0;
								if (ptr.charType == charIter) currentSatisfaction += 10;
								if (ptr.red == red && ptr.green == green && ptr.blue == blue) ++currentSatisfaction;
								if (currentSatisfaction > satisfactionLevel) {
									foundIndex = listInd;
									satisfactionLevel = currentSatisfaction;
									if (satisfactionLevel == 11) break;
								}
							}
							if (jesusFuckingChrist.empty()) {
								jesusFuckingChrist.emplace_back((CharacterType)charIter, iter.name, red, green, blue);
								iter.index = 0;
							} else {
								for (SortedMovesEntry& iterModif : vec) {
									if (iterModif.index > foundIndex) {
										++iterModif.index;
									}
								}
								jesusFuckingChrist.insert(jesusFuckingChrist.begin() + foundIndex + 1, {
									(CharacterType)charIter, iter.name, red, green, blue
								});
								iter.index = foundIndex + 1;
							}
						}
						SYSTEMTIME time;
						GetSystemTime(&time);
						settings.highlightWhenCancelsIntoMovesAvailable.year = time.wYear;
						settings.highlightWhenCancelsIntoMovesAvailable.month = time.wMonth;
						settings.highlightWhenCancelsIntoMovesAvailable.day = time.wDay;
						settings.highlightWhenCancelsIntoMovesAvailable.hour = time.wHour;
						settings.highlightWhenCancelsIntoMovesAvailable.minute = time.wMinute;
						settings.highlightWhenCancelsIntoMovesAvailable.second = time.wSecond;
						needWriteSettings = true;
						endScene.highlightGreenWhenBecomingIdleChanged();
						endScene.highlightSettingsChanged();
					}
				}
				ImGui::EndTable();
			}
		}
		customEnd();
	}
	popSearchStack();
	searchCollapsibleSection(PinnedWindowEnum_FramebarHelp);
	if (needDraw(PinnedWindowEnum_FramebarHelp) || searching) {
		framebarHelpWindow();
	}
	popSearchStack();
	searchCollapsibleSection(PinnedWindowEnum_HitboxesHelp);
	if (needDraw(PinnedWindowEnum_HitboxesHelp) || searching) {
		hitboxesHelpWindow();
	}
	popSearchStack();
	searchCollapsibleSection(PinnedWindowEnum_FrameAdvantageHelp);
	if (needDraw(PinnedWindowEnum_FrameAdvantageHelp) || searching) {
		ImGui::SetNextWindowSize({ 500.F, 0.F }, ImGuiCond_FirstUseEver);
		customBegin(PinnedWindowEnum_FrameAdvantageHelp);
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
		customEnd();
	}
	popSearchStack();
	searchCollapsibleSection(PinnedWindowEnum_StartupFieldHelp);
	if (needDraw(PinnedWindowEnum_StartupFieldHelp) || searching) {
		ImGui::SetNextWindowSize({ 500.F, 0.F }, ImGuiCond_FirstUseEver);
		customBegin(PinnedWindowEnum_StartupFieldHelp);
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
		customEnd();
	}
	popSearchStack();
	searchCollapsibleSection(PinnedWindowEnum_ActiveFieldHelp);
	if (needDraw(PinnedWindowEnum_ActiveFieldHelp) || searching) {
		ImGui::SetNextWindowSize({ 500.F, 0.F }, ImGuiCond_FirstUseEver);
		customBegin(PinnedWindowEnum_ActiveFieldHelp);
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
		customEnd();
	}
	popSearchStack();
	searchCollapsibleSection(PinnedWindowEnum_TotalFieldHelp);
	if (needDraw(PinnedWindowEnum_TotalFieldHelp) || searching) {
		ImGui::SetNextWindowSize({ 500.F, 0.F }, ImGuiCond_FirstUseEver);
		customBegin(PinnedWindowEnum_TotalFieldHelp);
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
		customEnd();
	}
	popSearchStack();
	searchCollapsibleSection(PinnedWindowEnum_InvulFieldHelp);
	if (needDraw(PinnedWindowEnum_InvulFieldHelp) || searching) {
		ImGui::SetNextWindowSize({ 500.F, 0.F }, ImGuiCond_FirstUseEver);
		customBegin(PinnedWindowEnum_InvulFieldHelp);
		searchFieldTitle("Help Contents");
		ImGui::PushTextWrapPos(0.F);
		ImGui::TextUnformatted(searchTooltip(
			"Strike invul: invulnerable to strike and projectiles.\n"
			"Throw invul: invulnerable to throws.\n"
			"Low profile: low profiles first active frame of Ky f.S.\n"
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
			"The following moves cause guard crush and are not \"unblockables\": Kum Max Charge Falcon Dive, Johnny Bacchus-Powered Mist Finer and Leo bt.214H"));
		customEnd();
	}
	popSearchStack();
	if (searching) {
		ImGui::PopID();
	}
}

// Runs on the graphics thread
void UI::onEndScene(IDirect3DDevice9* device, void* drawData, IDirect3DTexture9* iconTexture, bool needsFramesTextureFramebar, bool needsFramesTextureHelp) {
	if (!imguiInitialized || gifMode.modDisabled || !drawData) {
		return;
	}
	initializeD3D(device);
	
	substituteTextureIDs(device, drawData, iconTexture, needsFramesTextureFramebar, needsFramesTextureHelp);
	ImGui_ImplDX9_RenderDrawData((ImDrawData*)drawData);
}

// Runs on the main thread
void UI::initialize() {
	if (imguiInitialized || windowShowMode == WindowShowMode_None && !needShowFramebarCached || !keyboard.thisProcessWindow || gifMode.modDisabled) return;
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(keyboard.thisProcessWindow);
	
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->GetTexDataAsRGBA32(&fontData, &fontDataWidth, &fontDataHeight);  // imGui complains if we don't call this before preparing its draw data
	
	io.Fonts->SetTexID(TEXID_IMGUIFONT);  // I use fake wishy-washy IDs instead of real textures, because they're created on the
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
		clearSecondaryTextures();
	}
}

// Runs on the main thread while the graphics thread is suspended
void UI::handleResetAfter() {
	if (imguiD3DInitialized) {
		ImGui_ImplDX9_CreateDeviceObjects();
		IDirect3DDevice9** backend = (IDirect3DDevice9**)ImGui::GetIO().BackendRendererUserData;
		onImGuiMessWithFontTexID(backend ? *backend : nullptr);
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
		case WM_MOUSEWHEEL: {
			int wheelDelta = ((long)wParam >> 16);
			keyboard.addWheelDelta(wheelDelta);
		}
		break;
		case WM_APP_WIND_UP_TIMER_FOR_DEFERRED_SETTINGS_WRITE: {
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
			std::vector<wchar_t> selectedPathBuf;
			{
				std::unique_lock<std::mutex> screenshotGuard(settings.screenshotPathMutex);
				if (!settings.screenshotPath.empty()) {
					selectedPathBuf.resize(settings.screenshotPath.size() + 1);
					MultiByteToWideChar(CP_UTF8, 0, settings.screenshotPath.c_str(), -1, selectedPathBuf.data(), selectedPathBuf.size());
					selectedPath = selectedPathBuf.data();
					if (!selectedPath.empty()) {
						lastSelectedScreenshotPath = selectedPath;
					}
				}
			}
			ShowCursor(TRUE);
			if (selectFile(selectedPath, hWnd, L"PNG file (*.png)\0*.PNG\0", lastSelectedScreenshotPath, SELECT_FILE_MODE_WRITE)) {
				{
					std::unique_lock<std::mutex> screenshotGuard(settings.screenshotPathMutex);
					std::vector<char> screenshotPathBuf(selectedPath.size() * 4 + 1);
					WideCharToMultiByte(CP_UTF8, 0, selectedPath.c_str(), -1, screenshotPathBuf.data(), screenshotPathBuf.size(), NULL, NULL);
					logwrap(fprintf(logfile, "From selection dialog set screenshot path to: %s\n", settings.screenshotPath.c_str()));
					settings.screenshotPath = screenshotPathBuf.data();
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
		case WM_APP_UI_REQUEST_FILE_SELECT_WRITE: {
			std::wstring selectedPath;
			ShowCursor(TRUE);
			if (selectFile(selectedPath, hWnd, (const wchar_t*)lParam, *(std::wstring*)wParam, SELECT_FILE_MODE_WRITE)) {
				writeOutFile(selectedPath, *dataToWriteToFile);
				dataToWriteToFile = nullptr;
			}
			ShowCursor(FALSE);
			return TRUE;
		}
		case WM_APP_UI_REQUEST_FILE_SELECT_READ: {
			if (*aswEngine) {
				std::wstring selectedPath;
				ShowCursor(TRUE);
				if (selectFile(selectedPath, hWnd,
						L"JSON or COLLISION file (*.json or *.collision)\0*.json;*.collision\0"
						L"JSON file (*.json)\0*.json\0"
						L"COLLISION file (*.collision)\0*.collision\0",
						lastSelectedCollisionPath,
						SELECT_FILE_MODE_READ)) {
					readCollisionFromFile(selectedPath, wParam);
				}
				ShowCursor(FALSE);
			}
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
	enum DoWhatAction {
		DO_WHAT_ACTION_REMOVE,
		DO_WHAT_ACTION_INVERT
	};
	struct DoWhatWithIndex {
		int index;
		DoWhatAction action;
		DoWhatWithIndex(int index, DoWhatAction action) : index(index), action(action) { }
	};
	std::vector<DoWhatWithIndex> indicesToDoStuffWith;
	indicesToDoStuffWith.reserve(keyCombo.size());
	
	for (int i = 0; i < (int)keyCombo.size(); ++i) {
		if (i > 0) {
			ImGui::TextUnformatted(" + ");
			ImGui::SameLine();
		}
		int currentlySelectedKey = keyCombo[i];
		bool negate = currentlySelectedKey < 0;
		int codeAbs = negate ? -currentlySelectedKey : currentlySelectedKey;
		if (negate) {
			textUnformattedColored(LIGHT_BLUE_COLOR, "NOT HOLDING");
			ImGui::SameLine();
		}
		sprintf_s(strbuf, "##%s%d", info.uiName, i);
		const char* currentKeyStr = settings.getKeyRepresentation(codeAbs);
		ImGui::PushItemWidth(80);
		if (ImGui::BeginCombo(strbuf, searchFieldValue(currentKeyStr, nullptr)))
		{
			imguiContextMenuOpen = true;
			ImGui::PushID(-1);
			if (ImGui::Selectable("Remove this key", false)) {
				indicesToDoStuffWith.emplace_back(i, DO_WHAT_ACTION_REMOVE);
				needWriteSettings = true;
				keyCombosChanged = true;
			}
			if (ImGui::Selectable(negate ? "HOLDING THIS KEY" : "NOT HOLDING THIS KEY", false)) {
				indicesToDoStuffWith.emplace_back(i, DO_WHAT_ACTION_INVERT);
				needWriteSettings = true;
				keyCombosChanged = true;
			}
			ImGui::PopID();
			for (auto it = settings.keys.cbegin(); it != settings.keys.cend(); ++it)
			{
				const Settings::Key& key = it->second;
				ImGui::PushID((void*)&key);
				if (ImGui::Selectable(key.uiName, codeAbs == key.code)) {
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
	sprintf_s(strbuf, "##%sNew", info.uiName);
	ImGui::PushItemWidth(80);
	if (ImGui::BeginCombo(strbuf, ""))
	{
		imguiContextMenuOpen = true;
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
	for (const DoWhatWithIndex& indexToDoStuffWith : indicesToDoStuffWith) {
		switch (indexToDoStuffWith.action) {
			case DO_WHAT_ACTION_REMOVE: {
				keyCombo.erase(keyCombo.begin() + indexToDoStuffWith.index - offset);
				++offset;
				break;
			}
			case DO_WHAT_ACTION_INVERT: {
				int indWithOffset = indexToDoStuffWith.index - offset;
				int code = keyCombo[indWithOffset];
				keyCombo[indWithOffset] = -code;
				break;
			}
		}
	}
	
	ImGui::SameLine();
	HelpMarker(searchTooltip(info.uiDescriptionWithLength));
}

// Runs on the main thread. Called hundreds of times each frame
SHORT WINAPI UI::hook_GetKeyState(int nVirtKey) {
	if (ui.needToDivertCodeInGetKeyState) {
		if (ui.imguiActive) {
			return 0;
		} else if (ui.needTestDelay_dontReleaseKey && nVirtKey == ui.punchCode) {
			ui.testDelayStart = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
			ui.needTestDelay_dontReleaseKey = false;
			ui.needTestDelayStage2 = true;
			ui.needToDivertCodeInGetKeyState = false;
			return (SHORT)0x8000;
		}
	}
	return GetKeyState(nVirtKey);
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

// runs on the main thread
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

// runs on the main thread
void copyDrawList(ImDrawListBackup& destination, const ImDrawList* drawList) {
	destination.CmdBuffer.resize(drawList->CmdBuffer.Size);
	destination.IdxBuffer.resize(drawList->IdxBuffer.Size);
	destination.VtxBuffer.resize(drawList->VtxBuffer.Size);
	memcpy(destination.CmdBuffer.data(), drawList->CmdBuffer.Data, sizeof ImDrawCmd * drawList->CmdBuffer.Size);
	memcpy(destination.IdxBuffer.data(), drawList->IdxBuffer.Data, sizeof ImDrawIdx * drawList->IdxBuffer.Size);
	memcpy(destination.VtxBuffer.data(), drawList->VtxBuffer.Data, sizeof ImDrawVert * drawList->VtxBuffer.Size);
}

// Runs on the graphics thread
void UI::substituteTextureIDs(IDirect3DDevice9* device, void* drawData, IDirect3DTexture9* iconTexture, bool needsFramesTextureFramebar, bool needsFramesTextureHelp) {
	ImGui_ImplDX9_AssignTexID(TEXID_IMGUIFONT, imguiFont);
	ImGui_ImplDX9_AssignTexID(TEXID_IMGUIFONT_OUTLINED, imguiFontAlt ? (IDirect3DTexture9*)imguiFontAlt : imguiFont);
	if (!pinTexture) {
		if (!attemptedCreatingPin) {
			attemptedCreatingPin = true;
			pinTexture.Attach(graphics.createTexture(device, (BYTE*)pinResource->data.data(), pinResource->width, pinResource->height));
		}
	}
	ImGui_ImplDX9_AssignTexID(TEXID_PIN, pinTexture);
	ImGui_ImplDX9_AssignTexID(TEXID_GGICON, iconTexture);
	if (needsFramesTextureHelp) {
		ImGui_ImplDX9_AssignTexID(TEXID_FRAMES_HELP, graphics.getFramesTextureHelp(device, ui.packedTextureHelp));
	}
	if (needsFramesTextureFramebar) {
		ImGui_ImplDX9_AssignTexID(TEXID_FRAMES_FRAMEBAR, graphics.getFramesTextureFramebar(device));
	}
}

// Runs on the graphics thread
void UI::initializeD3D(IDirect3DDevice9* device) {
	if (!imguiD3DInitialized) {
		imguiD3DInitialized = true;
		ImGui_ImplDX9_Init(device);
		ImGui_ImplDX9_NewFrame();
		onImGuiMessWithFontTexID(device);
	}
}

// Runs on the graphics thread
void UI::onImGuiMessWithFontTexID(IDirect3DDevice9* device) {
	imguiFont = ImGui_ImplDX9_GetFontTexture();
	if (imguiFont && !imguiFontAlt && !fontDataAlt.empty() && device) {
		if (!attemptedCreatingAltFont) {
			attemptedCreatingAltFont = true;
			imguiFontAlt.Attach(graphics.createTexture(device, fontDataAlt.data(), fontDataWidth, fontDataHeight));
		}
	}
	if (!imguiFont) {
		clearImGuiFontAlt();
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

// from bbscript: charaName instruction
const char* characterNamesCode[25] {
	"sol",  // 0
	"kyk",  // 1
	"may",  // 2
	"mll",  // 3
	"zat",  // 4
	"pot",  // 5
	"chp",  // 6
	"fau",  // 7
	"axl",  // 8
	"ven",  // 9
	"sly",  // 10
	"ino",  // 11
	"bed",  // 12
	"ram",  // 13
	"sin",  // 14
	"elp",  // 15
	"leo",  // 16
	"jhn",  // 17
	"jko",  // 18
	"jam",  // 19
	"kum",  // 20
	"rvn",  // 21
	"dzy",  // 22
	"bkn",  // 23
	"ans"   // 24
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

// runs on the main thread
GGIcon coordsToGGIcon(int x, int y, int w, int h) {
	GGIcon result;
	result.size = ImVec2{ (float)w, (float)h };
	result.uvStart = ImVec2{ (float)x / 1536.F, (float)y / 1536.F };
	result.uvEnd = ImVec2{ (float)(x + w) / 1536.F, (float)(y + h) / 1536.F };
	return result;
}

// runs on the main thread
void drawGGIcon(const GGIcon& icon) {
	ImGui::Image(TEXID_GGICON, icon.size, icon.uvStart, icon.uvEnd);
}

// runs on the main thread
GGIcon scaleGGIconToHeight(const GGIcon& icon, float height) {
	GGIcon result;
	result.size = ImVec2{ icon.size.x * height / icon.size.y, height };
	result.uvStart = icon.uvStart;
	result.uvEnd = icon.uvEnd;
	return result;
}

// runs on the main thread
CharacterType getPlayerCharacter(int playerSide) {
	if (!*aswEngine || playerSide != 0 && playerSide != 1) return (CharacterType)-1;
	entityList.populate();
	Entity ent = entityList.slots[playerSide];
	if (!ent) return (CharacterType)-1;
	return ent.characterType();
}

// runs on the main thread
void drawPlayerIconWithTooltip(int playerSide) {
	CharacterType charType = getPlayerCharacter(playerSide);
	GGIcon scaledIcon = scaleGGIconToHeight(getCharIcon(charType), 14.F);
	drawGGIcon(scaledIcon);
	if (charType != -1) {
		AddTooltip(characterNamesFull[charType]);
	}
}

// runs on the main thread
void drawFontSizedPlayerIconWithCharacterName(CharacterType charType) {
	GGIcon scaledIcon = scaleGGIconToHeight(getCharIcon(charType), ImGui::GetFontSize());
	drawGGIcon(scaledIcon);
	ImGui::SameLine();
	char buf[11];
	sprintf_s(buf, "%s:", characterNames[charType]);
	textUnformattedColored(LIGHT_BLUE_COLOR, buf);
}

// runs on the main thread
void drawFontSizedPlayerIconWithText(CharacterType charType, const char* text) {
	GGIcon scaledIcon = scaleGGIconToHeight(getCharIcon(charType), ImGui::GetFontSize());
	drawGGIcon(scaledIcon);
	AddTooltip(characterNamesFull[charType]);
	ImGui::SameLine();
	ImGui::TextUnformatted(text);
}

// runs on the main thread
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

// runs on the main thread
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

// runs on the main thread
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

// runs on the main thread
bool UI::selectFile(std::wstring& path, HWND owner, const wchar_t* filterStr, std::wstring& lastSelectedPath, SelectFileMode selectMode) {
	std::wstring szFile;
	szFile = lastSelectedPath;

	std::vector<WCHAR> szFileBuf(MAX_PATH, L'\0');
	while (true) {
		if (!szFile.empty()) {
			memcpy(szFileBuf.data(), szFile.c_str(),
					(
						szFile.size() > MAX_PATH
							? MAX_PATH
							: szFile.size()
					) * sizeof (WCHAR)
				);
			szFileBuf.back() = L'\0';
		}
		
		OPENFILENAMEW selectedFiles{ 0 };
		selectedFiles.lStructSize = sizeof(OPENFILENAMEW);
		selectedFiles.hwndOwner = owner;
		selectedFiles.lpstrFile = szFileBuf.data();
		selectedFiles.nMaxFile = (DWORD)szFileBuf.size();
		selectedFiles.lpstrFilter = filterStr;
		selectedFiles.nFilterIndex = 1;
		selectedFiles.lpstrFileTitle = NULL;
		selectedFiles.nMaxFileTitle = 0;
		selectedFiles.lpstrInitialDir = NULL;
		selectedFiles.Flags =
			selectMode == SELECT_FILE_MODE_WRITE
				? OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR | OFN_NOREADONLYRETURN
				: OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
		
		if (!(
			selectMode == SELECT_FILE_MODE_WRITE
				? GetSaveFileNameW(&selectedFiles)
				: GetOpenFileNameW(&selectedFiles)
		)) {
			DWORD errCode = CommDlgExtendedError();
			if (!errCode) {
				logwrap(fputs("The file selection dialog was closed by the user.\n", logfile));
			}
			else {
				logwrap(fprintf(logfile, "Error selecting file. Error code: %.8x\n", errCode));
			}
			return false;
		}
		szFile = szFileBuf.data();
		
		if (selectMode == SELECT_FILE_MODE_WRITE) {
			
			bool containsOneOfExtensions = false;
			int numberOfExtensions = 0;
			const wchar_t* lastExtension = nullptr;
			
			const wchar_t* extSeek = filterStr;
			while (*extSeek != L'\0') {
				++numberOfExtensions;
				while (*extSeek != L'\0') ++extSeek;
				++extSeek;
				if (*extSeek == L'*') ++extSeek;
				if (*extSeek == L'.') {
					lastExtension = extSeek;
					if (endsWithCaseInsensitive(szFile, extSeek)) {
						containsOneOfExtensions = true;
						break;
					}
				}
				while (*extSeek != L'\0') ++extSeek;
				++extSeek;
			}
			
			if (!containsOneOfExtensions) {
				if (numberOfExtensions == 1) {
					path = szFile + lastExtension;
				} else if (numberOfExtensions > 1) {
					int pos = findCharRevW(szFile.c_str(), L'.');
					int posSlash = findCharRevW(szFile.c_str(), L'\\');
					std::wstring msg;
					if (pos != -1 && posSlash != -1 && pos < posSlash) pos = -1;
					if (pos == -1) {
						msg = L"No file extension specified.";
					} else {
						msg = L"Wrong file extension specified (";
						msg.append(szFile.begin() + pos, szFile.end());
						msg += L").";
					}
					msg += L" Please specify one of: ";
					extSeek = filterStr;
					bool isFirst = true;
					while (*extSeek != L'\0') {
						while (*extSeek != L'\0') ++extSeek;
						++extSeek;
						if (*extSeek == L'*') ++extSeek;
						if (isFirst) {
							isFirst = false;
						} else {
							msg += L", ";
						}
						msg += extSeek;
						while (*extSeek != L'\0') ++extSeek;
						++extSeek;
					}
					int dialogResult = MessageBoxW(keyboard.thisProcessWindow ? keyboard.thisProcessWindow : NULL,
						msg.c_str(),
						pos == -1 ? L"No file extension" : L"Wrong file extension",
						MB_RETRYCANCEL);
					if (dialogResult == IDRETRY) {
						memset(szFileBuf.data(), L'\0', szFileBuf.size() * sizeof (WCHAR));
						continue;
					} else {
						return false;
					}
				}
			}
		}
		break;
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

// runs on the main thread
void AddTooltip(const char* desc) {
	if (ImGui::BeginItemTooltip()) {
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

// runs on the main thread
void AddTooltipNoSharedDelay(const char* desc) {
    if (!ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip | ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay))
        return;
    if (ImGui::BeginTooltip()) {
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

// runs on the main thread
void HelpMarker(const char* desc) {
	ImGui::TextDisabled("(?)");
	AddTooltip(desc);
}

// runs on the main thread
void UI::HelpMarkerWithHotkey(const char* desc, const char* descEnd, std::vector<int>& hotkey) {
	ImGui::TextDisabled("(?)");
	AddTooltipWithHotkey(desc, descEnd, hotkey);
}

// runs on the main thread
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

// runs on the main thread
void UI::AddTooltipWithHotkeyNoSearch(const char* desc, std::vector<int>& hotkey) {
	if (searching || ImGui::BeginItemTooltip()) {
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		int result = sprintf_s(strbuf, "Hotkey: %s", comborepr(hotkey));
		if (result != -1) {
			ImGui::TextUnformatted(strbuf, strbuf + result);
		}
		ImGui::Separator();
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		if (!searching) ImGui::EndTooltip();
	}
}

// runs on the main thread
void RightAlign(float w) {
	const float rightEdge = ImGui::GetCursorPosX() + ImGui::GetColumnWidth();
	const float posX = (rightEdge - w);
	ImGui::SetCursorPosX(posX);
}

// runs on the main thread
void RightAlignedText(const char* txt) {
	RightAlign(ImGui::CalcTextSize(txt).x);
	ImGui::TextUnformatted(txt);
}

// runs on the main thread
void RightAlignedColoredText(const ImVec4& color, const char* txt) {
	RightAlign(ImGui::CalcTextSize(txt).x);
	ImGui::TextColored(color, txt);
}

// runs on the main thread
void CenterAlign(float w) {
	const auto rightEdge = ImGui::GetCursorPosX() + ImGui::GetColumnWidth() / 2;
	const auto posX = (rightEdge - w / 2);
	ImGui::SetCursorPosX(posX);
}

// runs on the main thread
void CenterAlignedText(const char* txt) {
	CenterAlign(ImGui::CalcTextSize(txt).x);
	ImGui::TextUnformatted(txt);
}

// runs on the main thread
const GGIcon& getCharIcon(CharacterType charType) {
	if (charType >= 0 && charType < 25) {
		return characterIconsBorderless[charType];
	}
	return questionMarkIcon;
}

// runs on the main thread
const GGIcon& getPlayerCharIcon(int playerSide) {
	return getCharIcon(getPlayerCharacter(playerSide));
}

// color = 0xRRGGBB
// runs on the main thread
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
// runs on the main thread
ImVec4 ARGBToVec(DWORD color) {
	// they also wrote it as r, g, b, a... just in struct form
	return {
		(float)((color >> 16) & 0xff) * inverse_255,  // red
		(float)((color >> 8) & 0xff) * inverse_255,  // green
		(float)(color & 0xff) * inverse_255,  // blue
		(float)(color >> 24) * inverse_255  // alpha
	};
}

DWORD VecToRGB(const ImVec4& vec) {
	
	DWORD color = 0xFF000000;
	
	// RED
	int val = (int)std::roundf(vec.x * 255.F);
	if (val < 0) val = 0;
	if (val > 255) val = 255;
	color |= val << 16;
	
	// GREEN
	val = (int)std::roundf(vec.y * 255.F);
	if (val < 0) val = 0;
	if (val > 255) val = 255;
	color |= val << 8;
	
	// BLUE
	val = (int)std::roundf(vec.z * 255.F);
	if (val < 0) val = 0;
	if (val > 255) val = 255;
	color |= val;
	
	return color;
}

// runs on the main thread
const char* formatBoolean(bool value) {
	static const char* trueStr = "true";
	static const char* falseStr = "false";
	return value ? trueStr : falseStr;
}

// runs on the main thread
float getItemSpacing() {
	return ImGui::GetStyle().ItemSpacing.x;
}

// runs on the main thread
bool UI::addImage(WORD resourceId, std::unique_ptr<PngResource>& resource) {
	if (!resource) resource = std::make_unique<PngResource>();
	if (!loadPngResource(hInst, resourceId, *resource)) return false;
	return true;
}

// runs on the main thread
bool UI::addDigit(WORD resourceId, WORD resourceIdThickness1, DigitFrame& digit) {
	for (std::unique_ptr<PngResource>& ptr : digit.thickness) {
		if (!ptr) ptr = std::make_unique<PngResource>();
	}
	if (!loadPngResource(hInst, resourceId, *digit.thickness[0])) return false;
	if (!loadPngResource(hInst, resourceIdThickness1, *digit.thickness[1])) return false;
	return true;
}

// runs on the main thread
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

// runs on the main thread
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

// runs on the main thread
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

// runs on the main thread
int numDigits(int num) {
	int answer = 1;
	num /= 10;
	while (num) {
		++answer;
		num /= 10;
	}
	return answer;
}

// runs on the main thread
void UI::addFrameArt(FrameType frameType, WORD resourceIdBothVersions, std::unique_ptr<PngResource>& resourceBothVersions, StringWithLength description) {
	if (!resourceBothVersions) resourceBothVersions = std::make_unique<PngResource>();
	addImage(resourceIdBothVersions, resourceBothVersions);
	frameArtNonColorblind[frameType] = frameArtColorblind[frameType] = {
		frameType,
		resourceBothVersions.get(),
		description
	};
}

// runs on the main thread
void UI::addFrameArt(FrameType frameType, WORD resourceIdColorblind, std::unique_ptr<PngResource>& resourceColorblind,
                 WORD resourceIdNonColorblind, std::unique_ptr<PngResource>& resourceNonColorblind, StringWithLength description) {
	if (!resourceColorblind) resourceColorblind = std::make_unique<PngResource>();
	if (!resourceNonColorblind) resourceNonColorblind = std::make_unique<PngResource>();
	assert(&resourceIdColorblind != &resourceIdNonColorblind);
	assert(&resourceColorblind != &resourceNonColorblind);
	addImage(resourceIdColorblind, resourceColorblind);
	addImage(resourceIdNonColorblind, resourceNonColorblind);
	frameArtColorblind[frameType] = {
		frameType,
		resourceColorblind.get(),
		description
	};
	frameArtNonColorblind[frameType] = {
		frameType,
		resourceNonColorblind.get(),
		description
	};
	
}

// ARGB color
// runs on the main thread
void UI::addFrameMarkerArt(FrameMarkerType markerType,
		WORD resourceIdBothVersions, std::unique_ptr<PngResource>& resourceBothVersions,
		DWORD outlineColorNonColorblind, DWORD outlineColorColorblind,
		bool hasMiddleLineNonColorblind, bool hasMiddleLineColorblind) {
	if (!resourceBothVersions) resourceBothVersions = std::make_unique<PngResource>();
	addImage(resourceIdBothVersions, resourceBothVersions);
	frameMarkerArtNonColorblind[markerType] = frameMarkerArtColorblind[markerType] = {
		markerType,
		resourceBothVersions.get()
	};
	frameMarkerArtNonColorblind[markerType].outlineColor = outlineColorNonColorblind;
	frameMarkerArtColorblind[markerType].outlineColor = outlineColorColorblind;
	frameMarkerArtNonColorblind[markerType].hasMiddleLine = hasMiddleLineNonColorblind;
	frameMarkerArtColorblind[markerType].hasMiddleLine = hasMiddleLineColorblind;
}

// runs on the main thread
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

// runs on the main thread
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

// runs on the main thread
bool UI::needShowFramebar() const {
	if (settings.showFramebar
			&& (!settings.closingModWindowAlsoHidesFramebar || windowShowMode != WindowShowMode_None)
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

// runs on the main thread
template<typename T>
int printCancels(const T& cancels, float maxY) {
	struct Requirement {
		MoveCondition condition;
		const char* description;
	};
	static Requirement requirements[] {
		{ MOVE_CONDITION_REQUIRES_25_TENSION, "requires 25 meter" },
		{ MOVE_CONDITION_REQUIRES_50_TENSION, "requires 50 meter" },
		{ MOVE_CONDITION_REQUIRES_100_TENSION, "requires 100 meter" },
		{ MOVE_CONDITION_IS_TOUCHING_LEFT_SCREEN_EDGE, "must touch left screen edge" },
		{ MOVE_CONDITION_IS_TOUCHING_RIGHT_SCREEN_EDGE, "must touch right screen edge" },
		{ MOVE_CONDITION_IS_TOUCHING_WALL, "must touch arena's wall" }
	};
	int counter = 1;
	bool useSlang = settings.useSlangNames;
	for (size_t i = 0; i < cancels.size(); ++i) {
		const GatlingOrWhiffCancelInfoStored& cancel = cancels[i];
		
		if (i != cancels.size() - 1 && ImGui::GetCursorPosY() >= maxY) {
			ImGui::Text("%d) Skipping %d items...", counter++, cancels.size() - i);
			break;
		}
		
		char* buf = strbuf;
		size_t bufSize = sizeof strbuf;
		int result;
		if (cancel.name) {
			result = sprintf_s(buf, bufSize, "%s", useSlang && cancel.name->slang ? cancel.name->slang : cancel.name->name);
			advanceBuf
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
			if (cancel.move->conditions.getBit(req.condition)) {
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

// runs on the main thread
void UI::hitboxesHelpWindow() {
	ImGui::SetNextWindowSize({ ImGui::GetFontSize() * 35.0f + 16.F, 0.F }, ImGuiCond_FirstUseEver);
	customBegin(PinnedWindowEnum_HitboxesHelp);
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
	ImGui::TextUnformatted("If a move (like Riot Stamp) has a throw box as well as hitbox - both the hitbox and the throw box must connect.");
	
	ImGui::PopTextWrapPos();
	customEnd();
}

// runs on the main thread
void UI::framebarHelpWindow() {
	packTextureHelp();
	
	ImGui::SetNextWindowSize({ ImGui::GetFontSize() * 35.0f + 16.F, 0.F }, ImGuiCond_FirstUseEver);
	customBegin(PinnedWindowEnum_FramebarHelp);
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
		
		ImGui::Image(TEXID_FRAMES_HELP,
			{ frameWidthOriginal, frameHeightOriginal },
			art->help.start,
			art->help.end,
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
	ImGui::Image(TEXID_FRAMES_HELP,
		{ frameWidthOriginal, frameHeightOriginal },
		frameArtArray[FT_STARTUP].help.start,
		frameArtArray[FT_STARTUP].help.end,
		ImVec4(1, 1, 1, 1),
		ImVec4(1, 1, 1, 1));
	ImGui::SetCursorPos({
		cursorPos.x + frameWidthOriginal * 0.5F
			+ 1.F,  // include the white border
		cursorPos.y
			+ 1.F  // include the white border
	});
	ImGui::Image(TEXID_FRAMES_HELP,
		{ frameWidthOriginal * 0.5F, frameHeightOriginal },
		{
			(frameArtArray[FT_ACTIVE].help.start.x + frameArtArray[FT_ACTIVE].help.end.x) * 0.5F,
			frameArtArray[FT_ACTIVE].help.start.y
		},
		frameArtArray[FT_ACTIVE].help.end,
		ImVec4(1, 1, 1, 1));
	
	ImGui::SameLine();
	ImGui::TextUnformatted(searchTooltip("A half-filled active frame means an attack's startup or active frame which first begins during"
		" a superfreeze."));
	
	ImGui::Separator();
	
	const FrameMarkerArt* frameMarkerArtArray = settings.useColorblindHelp ? frameMarkerArtColorblind : frameMarkerArtNonColorblind;
	
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
	
	const float markerOffsetY = std::roundf((ImGui::GetTextLineHeightWithSpacing() - frameMarkerArtArray[MARKER_TYPE_STRIKE_INVUL].help.size.y) * 0.5F);
	for (int i = 0; i < _countof(markerHelps); ++i) {
		MarkerHelpInfo& info = markerHelps[i];
		float cursorY = ImGui::GetCursorPosY();
		ImGui::SetCursorPosY(cursorY + markerOffsetY);
		const FrameMarkerArt& art = frameMarkerArtArray[info.type];
		ImGui::Image(TEXID_FRAMES_HELP,
			art.help.size,
			art.help.start,
			art.help.end
			);
		ImGui::SameLine();
		ImGui::SetCursorPosY(cursorY);
		ImGui::TextUnformatted(searchTooltip(info.description));
	}
	
	float cursorY = ImGui::GetCursorPosY();
	ImGui::SetCursorPosY(cursorY + std::roundf((ImGui::GetTextLineHeightWithSpacing() - powerupFrameArt.help.size.y) * 0.5F));
	ImGui::Image(TEXID_FRAMES_HELP,
		powerupFrameArt.help.size,
		powerupFrameArt.help.start,
		powerupFrameArt.help.end
	);
	ImGui::SameLine();
	ImGui::SetCursorPosY(cursorY);
	ImGui::TextUnformatted(searchTooltip("The move reached some kind of powerup on this frame. For example, for May 6P it means that,"
		" starting from this frame, it deals more stun and has more pushback, while for Venom QV it means the ball has become"
		" bigger, and so on.\n"));
		
	ImGui::Separator();
	
	ImGui::Image(TEXID_FRAMES_HELP,
		firstFrameArt.help.size,
		firstFrameArt.help.start,
		firstFrameArt.help.end);
	ImGui::SameLine();
	ImGui::TextUnformatted(searchTooltip("A first frame, denoting the start of a new animation."
		" If the animation didn't change, may mean transition to some new state in the animation."
		" For blockstun and hitstun may mean leaving hitstop or re-entering hitstun/blockstun/hitstop.\n"
		"Some animation changes are intentionally not shown."));
	
	ImGui::Separator();
	
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	
	ImGui::Image(TEXID_FRAMES_HELP,
		hitConnectedFrameArt.help.size,
		hitConnectedFrameArt.help.start,
		hitConnectedFrameArt.help.end);
	
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
			"\"condenseIntoOneProjectileFramebar\" setting allows you to condense each player's projectiles all into one mini-framebar for that player.\n"
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
			TEXID_FRAMES_HELP,
			firstFrameArt.help.size,
			firstFrameArt.help.start,
			firstFrameArt.help.end
		},
		{
			TEXID_FRAMES_HELP,
			newHitArt->help.size,
			newHitArt->help.start,
			newHitArt->help.end
		}
	};
	ImGui::PopTextWrapPos();
	imGuiDrawWrappedTextWithIcons(searchTooltipStr(generalFramebarHelp),
		generalFramebarHelp.c_str() + generalFramebarHelp.size(),
		wordWrapWidth,
		icons,
		_countof(icons));
	customEnd();
}

// This is from imgui_draw.cpp::ImFont::CalcWordWrapPositionA, modified version. Added icons
// runs on the main thread
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
// runs on the main thread
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
		drawList->AddImage(TEXID_GGICON, \
			{ x, y }, \
			{ x + framebarTooltipInputIconSize.x, y + framebarTooltipInputIconSize.y }, \
			{ icon.uStart, icon.vStart }, \
			{ icon.uEnd, icon.vEnd }, \
			prevRow.name ? darkTint : -1); \
		x += framebarTooltipInputIconSize.x + spacing; \
	}

// runs on the main thread
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

// runs on the main thread
static inline void drawDirectionIcon(ImDrawList* drawList, float& x, float y,
		float spacing, Input row, Input prevRow, const ImVec2& framebarTooltipInputIconSize,
		ImU32 darkTint) {
	const InputsIcon* rowDirection = determineDirectionIcon(row);
	if (rowDirection) {
		drawList->AddImage(TEXID_GGICON,
			{ x, y },
			{ x + framebarTooltipInputIconSize.x, y + framebarTooltipInputIconSize.y },
			{ rowDirection->uStart, rowDirection->vStart },
			{ rowDirection->uEnd, rowDirection->vEnd },
			(((unsigned short)row & 0xf) == ((unsigned short)prevRow & 0xf))
				? darkTint : -1);
		x += framebarTooltipInputIconSize.x + spacing;
	}
}

// runs on the main thread
static inline void printInputsRowP1(ImDrawList* drawList, float x, float y,
			float spacing, int frameCount, Input row, Input prevRow, const ImVec2& framebarTooltipInputIconSize,
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

// runs on the main thread
static inline void printInputsRowP2(ImDrawList* drawList, float x, float y,
		float spacing, int frameCount, Input row, Input prevRow, const ImVec2& framebarTooltipInputIconSize,
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

// runs on the main thread
void UI::drawPlayerFrameInputsInTooltip(const PlayerFrame& frame, int playerIndex) {
	CharacterType charType = endScene.players[playerIndex].charType;
	
	FrameCancelInfoStored* cancelsUse;
	if (frame.cancels) {
		cancelsUse = frame.cancels.get();
	} else {
		static FrameCancelInfoStored emptyCancels;
		static bool emptyCancelsInitialized = false;
		if (!emptyCancelsInitialized) {
			emptyCancelsInitialized = true;
			emptyCancels.gatlings.count = 0;
			emptyCancels.whiffCancels.count = 0;
			emptyCancels.whiffCancelsNote = nullptr;
		}
		cancelsUse = &emptyCancels;
	}
	
	printAllCancels(*cancelsUse,
			frame.enableSpecialCancel,
			frame.clashCancelTimer,
			frame.enableJumpCancel,
			frame.enableSpecials,
			frame.hitAlreadyHappened,
			frame.airborne,
			nullptr,
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
	
	static const float framebarTooltipInputIconSizeFloat = 20.F;
	static const ImVec2 framebarTooltipInputIconSize{ framebarTooltipInputIconSizeFloat, framebarTooltipInputIconSizeFloat };
	static const float spacing = 1.F;
	
	if (frame.multipleInputs ? frame.inputs->empty() : frame.input == Input{0x0000}) return;
	
	ImGui::Separator();
	
	ImDrawList* const drawList = ImGui::GetWindowDrawList();
	const ImVec2 windowPos = ImGui::GetWindowPos();
	const ImVec2 cursorPos = ImGui::GetCursorPos();
	const float x = windowPos.x + cursorPos.x;
	float y = windowPos.y + cursorPos.y;
	const ImU32 darkTint = ImGui::GetColorU32(inputsDark);
	const float oneLineHeight = ImGui::GetTextLineHeightWithSpacing() + 2.F;
	const float textPaddingY = (framebarTooltipInputIconSizeFloat - ImGui::GetFontSize()) * 0.5F;
	
	if (frame.multipleInputs) {
		const std::vector<Input>& inputs = *frame.inputs;
		
		int frameCount = 1;
		const Input* it = inputs.data() + (inputs.size() - 1);
		Input currentInput = *it;
		
		
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
	} else {
		#define piece(funcname) funcname(drawList, x, y, spacing, 1, frame.input, frame.prevInput, framebarTooltipInputIconSize, textPaddingY, darkTint, false, 0.F);
		if (playerIndex == 0) {
			piece(printInputsRowP1)
		} else {
			piece(printInputsRowP2)
		}
		#undef piece
		ImGui::InvisibleButton("##PlayerInputsRender",
			{
				1.F,
				oneLineHeight
			});
	}
	
}

// runs on the main thread
void UI::drawPlayerFrameTooltipInfo(const PlayerFrame& frame, int playerIndex, float wrapWidth) {
	
	static const StringWithLength invulTitle = "Invul: ";
	frame.printInvuls(strbuf, sizeof strbuf - invulTitle.length);
	
	CharacterType charType = endScene.players[playerIndex].charType;
	if (charType == CHARACTER_TYPE_JACKO && frame.u.jackoInfo.hasAegisField) {
		ImGui::TextUnformatted("Aegis Field active.");
	}
	
	if (*strbuf != '\0' || frame.OTGInGeneral || frame.counterhit || frame.crouching) {
		ImGui::Separator();
		if (*strbuf != '\0') {
			yellowText(invulTitle.txt, invulTitle.txt + invulTitle.length);
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
		if (frame.counterhit && frame.crouching) {
			zerohspacing
			textUnformattedColored(CROUCHING_TEXT_COLOR, "Crouching ");
			ImGui::SameLine();
			textUnformattedColored(COUNTERHIT_TEXT_COLOR, "counterhit ");
			ImGui::SameLine();
			ImGui::TextUnformatted("state.");
			_zerohspacing
		} else if (frame.counterhit) {
			zerohspacing
			textUnformattedColored(COUNTERHIT_TEXT_COLOR, "Counterhit ");
			ImGui::SameLine();
			ImGui::TextUnformatted("state.");
			_zerohspacing
		} else if (frame.crouching) {
			zerohspacing
			textUnformattedColored(CROUCHING_TEXT_COLOR, "Crouching ");
			ImGui::SameLine();
			ImGui::TextUnformatted("state.");
			_zerohspacing
		}
	}
	
	if (frame.canYrc || frame.cantRc || frame.canYrcProjectile || frame.createdProjectiles && !frame.createdProjectiles->empty()) {
		ImGui::Separator();
		if (frame.canYrcProjectile) {
			ImGui::TextUnformatted(frame.canYrcProjectile);
		} else if (frame.canYrc) {
			ImGui::TextUnformatted("Can YRC");
		} else if (frame.cantRc) {
			ImGui::TextUnformatted("Can't RC, unless opponent is in hitstun or blockstun or bursting");
		}
		if (frame.createdProjectiles && !frame.createdProjectiles->empty()) {
			for (const CreatedProjectileStruct& element : *frame.createdProjectiles) {
				zerohspacing
				
				const char* createdByNameUse = element.useCreatedByNamePair
					? element.createdByNamePair
						? (settings.useSlangNames && false) && element.createdByNamePair->slang
							? element.createdByNamePair->slang
							: element.createdByNamePair->name
						: nullptr
					: element.createdBy;
					
				if (createdByNameUse) {
					ImGui::TextUnformatted(createdByNameUse);
					ImGui::SameLine();
				}
				
				const char* nameUse = element.useNamePair
					? element.namePair
						? (settings.useSlangNames && false) && element.namePair->slang
							? element.namePair->slang
							: element.namePair->name
						: nullptr
					: element.name;
					
				if (nameUse) {
					char firstLetter = nameUse[0];
					if (element.usePrefix) {
						static bool letters['z' - 'a' + 1];
						static bool lettersStandalone['z' - 'a' + 1];
						static bool lettersInitialized = false;
						if (!lettersInitialized) {
							lettersInitialized = true;
							initializeLetters(letters, lettersStandalone);
						}
						if (createdByNameUse && strcmp(createdByNameUse, nameUse) == 0) {
							ImGui::TextUnformatted(" created another ");
						} else if (firstLetter >= 'a' && firstLetter <= 'z' && letters[firstLetter - 'a']
								|| firstLetter >= 'A' && firstLetter <= 'Z' && letters[firstLetter - 'A']
								|| (firstLetter >= 'a' && firstLetter <= 'z' && lettersStandalone[firstLetter - 'a']
									|| firstLetter >= 'A' && firstLetter <= 'Z' && lettersStandalone[firstLetter - 'A'])
									&& (
										(unsigned char)nameUse[1] <= 32U
										|| nameUse[1] == '.'   // F.D.B
									)) {
							ImGui::TextUnformatted(createdByNameUse ? " created an " : "Created an ");
						} else {
							ImGui::TextUnformatted(createdByNameUse ? " created a " : "Created a ");
						}
						ImGui::SameLine();
						ImGui::TextUnformatted(nameUse);
					} else if (createdByNameUse) {
						strbuf[0] = ' ';
						if (createdByNameUse && firstLetter >= 'A' && firstLetter <= 'Z') {
							strbuf[1] = firstLetter - 'A' + 'a';
							strcpy_s(strbuf + 2, sizeof strbuf - 2, nameUse + 1);
						} else {
							strcpy_s(strbuf + 1, sizeof strbuf - 1, nameUse);
						}
						ImGui::TextUnformatted(strbuf);
					} else {
						ImGui::TextUnformatted(nameUse);
					}
				} else {
					ImGui::TextUnformatted(createdByNameUse ? " created a projectile" : "Created a projectile");
				}
				_zerohspacing
			}
		}
	}
	
	if (charType == CHARACTER_TYPE_SOL) {
		const SolInfo& si = frame.u.solInfo;
		if (si.gunflameDisappearsOnHit
				|| si.gunflameComesOutLater
				|| si.gunflameFirstWaveDisappearsOnHit
				|| si.hasTyrantRavePunch2) {
			ImGui::Separator();
			if (si.gunflameDisappearsOnHit) {
				ImGui::TextUnformatted("The Gunflame will disappear if Sol is hit (non-blocked hit) on this frame.");
			}
			if (si.gunflameComesOutLater) {
				ImGui::TextUnformatted("The Gunflame will come out later if Sol is hit (non-blocked hit) on this frame.");
			}
			if (si.gunflameFirstWaveDisappearsOnHit) {
				ImGui::TextUnformatted("The first wave (but not the consecutive waves) of Gunflame will disappear if Sol is hit (non-blocked hit) on this frame.");
			}
			if (si.hasTyrantRavePunch2) {
				ImGui::TextUnformatted("DI Tyrant Rave Laser will disappear if Sol is hit (non-blocked hit) at any time.");
			}
		}
	} else if (charType == CHARACTER_TYPE_KY) {
		const KyInfo& ki = frame.u.kyInfo;
		if (ki.stunEdgeWillDisappearOnHit
				|| ki.hasChargedStunEdge
				|| ki.hasSPChargedStunEdge
				|| ki.hasjD) {
			ImGui::Separator();
			if (ki.stunEdgeWillDisappearOnHit) {
				ImGui::TextUnformatted("The Stun Edge will disappear if Ky is hit (non-blocked hit) on this frame.");
			}
			if (ki.hasChargedStunEdge) {
				ImGui::TextUnformatted("The Charged Stun Edge will disappear if Ky is hit (non-blocked hit) at any time.");
			}
			if (ki.hasSPChargedStunEdge) {
				ImGui::TextUnformatted("The Fortified Charged Stun Edge will disappear if Ky is hit (non-blocked hit) at any time.");
			}
			if (ki.hasjD) {
				ImGui::TextUnformatted("The j.D will disappear if Ky is hit (non-blocked hit) at any time.");
			}
		}
	} else if (charType == CHARACTER_TYPE_MILLIA) {
		const MilliaInfo& mi = frame.u.milliaInfo;
		if (mi.hasPin
				|| mi.hasSDisc
				|| mi.hasHDisc
				|| mi.hasEmeraldRain
				|| mi.hasHitstunLinkedSecretGarden
				|| mi.hasRose) {
			ImGui::Separator();
			if (mi.hasPin) {
				ImGui::TextUnformatted("Silent Force will stop being active if Millia is hit (non-blocked hit) at any time.");
			}
			if (mi.hasSDisc) {
				ImGui::TextUnformatted("S Tandem Top will disappear if Millia is hit (non-blocked hit) at any time.");
			}
			if (mi.hasHDisc) {
				ImGui::TextUnformatted("H Tandem Top will disappear if Millia is hit or blocks a hit at any time.");
			}
			if (mi.hasEmeraldRain) {
				ImGui::TextUnformatted("Emerald Rain will disappear if Millia is hit or blocks a hit at any time.");
			}
			if (mi.hasHitstunLinkedSecretGarden) {
				ImGui::TextUnformatted("Secret Garden will disappear if Millia is hit (non-blocked hit) on this frame.");
			}
			if (mi.hasRose) {
				ImGui::TextUnformatted("Any Chroming Rose roses will disappear if Millia is hit (non-blocked hit) at any time.");
			}
		}
	} else if (charType == CHARACTER_TYPE_CHIPP) {
		const ChippInfo& ci = frame.u.chippInfo;
		if (ci.hasShuriken
				|| ci.hasKunaiWall
				|| ci.hasRyuuYanagi) {
			ImGui::Separator();
			if (ci.hasShuriken) {
				ImGui::TextUnformatted("Shuriken will disappear if Chipp is hit (non-blocked hit) at any time.");
			}
			if (ci.hasKunaiWall) {
				ImGui::TextUnformatted("Kunai (Wall) will disappear if Chipp is hit (non-blocked hit) at any time.");
			}
			if (ci.hasRyuuYanagi) {
				ImGui::TextUnformatted("Ryuu Yanagi will disappear if Chipp is hit (non-blocked hit) at any time.");
			}
		}
	} else if (charType == CHARACTER_TYPE_ZATO) {
		const ZatoInfo& zi = frame.u.zatoInfo;
		if (zi.hasGreatWhite
				|| zi.hasInviteHell
				|| zi.hasEddie) {
			ImGui::Separator();
			if (zi.hasGreatWhite) {
				ImGui::TextUnformatted("Great White will disappear if Zato is hit (non-blocked hit) at any time.");
			}
			if (zi.hasInviteHell) {
				ImGui::TextUnformatted("Invite Hell will disappear if Zato is hit (non-blocked hit) at any time.");
			}
			if (zi.hasEddie) {
				ImGui::TextUnformatted("Eddie will disappear if Zato is hit or blocks a hit at any time.");
			}
		}
	} else if (charType == CHARACTER_TYPE_FAUST) {
		if (frame.u.faustInfo.hasFlower) {
			ImGui::Separator();
			ImGui::TextUnformatted("Flower will disappear if Faust is hit (non-blocked hit) at any time.");
		}
	} else if (charType == CHARACTER_TYPE_SLAYER) {
		if (frame.u.slayerInfo.hasRetro) {
			ImGui::Separator();
			ImGui::TextUnformatted("Helter-Skelter shockwave will disappear if Slayer is hit (non-blocked hit) at any time.");
		}
	} else if (charType == CHARACTER_TYPE_INO) {
		const InoInfo& ii = frame.u.inoInfo;
		if (ii.hasChemicalLove
				|| ii.hasNote
				|| ii.has5DYRC) {
			ImGui::Separator();
			if (ii.hasChemicalLove) {
				ImGui::TextUnformatted("Chemical Love will disappear if I-No is hit (non-blocked hit) on this frame.");
			}
			if (ii.hasNote) {
				ImGui::TextUnformatted("Antidepressant Scale will disappear if I-No is hit (non-blocked hit) on this frame.");
			}
			if (ii.has5DYRC) {
				ImGui::TextUnformatted("5D will disappear if I-No is hit (non-blocked hit) at any time.");
			}
		}
	} else if (charType == CHARACTER_TYPE_MAY) {
		const MayInfo& mi = frame.u.mayInfo;
		if (mi.hasDolphin || mi.hasBeachBall) {
			ImGui::Separator();
			if (mi.hasDolphin) {
				ImGui::TextUnformatted("The Dolphin will disappear if May is hit (non-blocked hit) at any time.");
			}
			if (mi.hasBeachBall) {
				ImGui::TextUnformatted("The Beach Ball will disappear if May is hit (non-blocked hit) at any time.");
			}
		}
	} else if (charType == CHARACTER_TYPE_POTEMKIN) {
		if (frame.u.potemkinInfo.hasBomb) {
			ImGui::Separator();
			ImGui::TextUnformatted("Trishula explosion will disappear if Potemkin is hit (non-blocked hit) on this frame.");
		}
	} else if (charType == CHARACTER_TYPE_AXL) {
		const AxlInfo& ai = frame.u.axlInfo;
		if (ai.hasSpindleSpinner
				|| ai.hasSickleFlash
				|| ai.hasMelodyChain
				|| ai.hasSickleStorm) {
			ImGui::Separator();
			if (ai.hasSpindleSpinner) {
				ImGui::TextUnformatted("Spindle Spinner will disappear if Axl is hit (non-blocked hit) at any time.");
			}
			if (ai.hasSickleFlash) {
				ImGui::TextUnformatted("Sickle Flash will disappear if Axl is hit (non-blocked hit) at any time.");
			}
			if (ai.hasMelodyChain) {
				ImGui::TextUnformatted("Melody Chain will disappear if Axl is hit (non-blocked hit) at any time.");
			}
			if (ai.hasSickleStorm) {
				ImGui::TextUnformatted("Sickle Storm will disappear if Axl is hit or blocks a hit at any time.");
			}
		}
	} else if (charType == CHARACTER_TYPE_VENOM) {
		const VenomInfo& vi = frame.u.venomInfo;
		if (vi.hasQV
				|| vi.hasQVYRCOnly
				|| vi.hasHCarcassBall
				|| vi.performingQVA
				|| vi.performingQVB
				|| vi.performingQVC
				|| vi.performingQVD
				|| vi.performingQVAHitOnly
				|| vi.performingQVBHitOnly
				|| vi.performingQVCHitOnly
				|| vi.performingQVDHitOnly) {
			ImGui::Separator();
			if (vi.hasQV) {
				ImGui::TextUnformatted("QV shockwave will disappear if Venom RC's or gets hit on this frame.");
			} else if (vi.hasQVYRCOnly) {
				ImGui::TextUnformatted("QV shockwave will disappear if Venom RC's.");
			}
			if (vi.hasHCarcassBall) {
				ImGui::TextUnformatted("H Carcass Raid ball will disappear if Venom is hit (non-blocked hit) at any time.");
			}
			if (vi.performingQVA) {
				ImGui::TextUnformatted("P Ball will disappear if Venom RC's or gets hit on this frame.");
			} else if (vi.performingQVB) {
				ImGui::TextUnformatted("K Ball will disappear if Venom RC's or gets hit on this frame.");
			} else if (vi.performingQVC) {
				ImGui::TextUnformatted("S Ball will disappear if Venom RC's or gets hit on this frame.");
			} else if (vi.performingQVD) {
				ImGui::TextUnformatted("H Ball will disappear if Venom RC's or gets hit on this frame.");
			} else if (vi.performingQVB) {
				ImGui::TextUnformatted("K Ball will disappear if Venom RC's or gets hit on this frame.");
			} else if (vi.performingQVAHitOnly) {
				ImGui::TextUnformatted("P Ball will disappear if Venom gets hit on this frame.");
			} else if (vi.performingQVBHitOnly) {
				ImGui::TextUnformatted("K Ball will disappear if Venom gets hit on this frame.");
			} else if (vi.performingQVCHitOnly) {
				ImGui::TextUnformatted("S Ball will disappear if Venom gets hit on this frame.");
			} else if (vi.performingQVDHitOnly) {
				ImGui::TextUnformatted("H Ball will disappear if Venom gets hit on this frame.");
			}
		}
	} else if (charType == CHARACTER_TYPE_BEDMAN) {
		const BedmanInfo& bi = frame.u.bedmanInfo;
		bool hasSeparator = false;
		#define separator if (!hasSeparator) { hasSeparator = true; ImGui::Separator(); }
		if (bi.hasBoomerangAHead) {
			separator
			ImGui::TextUnformatted("Task A Head will disappear if Bedman is hit or blocks a hit at any time.");
		}
		if (bi.hasBoomerangBHead) {
			separator
			ImGui::TextUnformatted("Task A' Head will disappear if Bedman is hit or blocks a hit at any time.");
		}
		#define dejavuBoomerang(letter, sealName) \
			if (bi.hasDejavuBoomerang##letter) { \
				separator \
				if (bi.hasDejavu##letter##Ghost && bi.seal##letter) { \
					ImGui::TextUnformatted(sealName " Head will disappear if Bedman is hit or blocks at any time." \
						" And the Seal will disappear if Bedman is hit (non-blocked hit) on this frame."); \
				} else { \
					ImGui::TextUnformatted(sealName " Head will disappear if Bedman is hit or blocks at any time."); \
				} \
			} else if (bi.hasDejavu##letter##Ghost) { \
				if (bi.dejavu##letter##GhostAlreadyCreatedBoomerang) { \
					if (bi.seal##letter) { \
						separator \
						ImGui::TextUnformatted(sealName " Seal will disappear if Bedman is hit on this frame."); \
					} \
				} else if (bi.seal##letter) { \
					separator \
					ImGui::TextUnformatted(sealName " Head will disappear if Bedman is hit at any time, but not if he blocks on this frame." \
						" And the Seal will disappear if Bedman is hit (non-blocked hit) on this frame."); \
				} else { \
					separator \
					ImGui::TextUnformatted(sealName " Head will disappear if Bedman is hit at any time, but not if he blocks on this frame."); \
				} \
			}
		dejavuBoomerang(A, "Task A")
		dejavuBoomerang(B, "Task A'")
		#undef dejavuBoomerang
		#define dejavuTask(letter, sealName) \
			if (bi.hasDejavu##letter##Ghost) { \
				if (!bi.dejavu##letter##GhostInRecovery && bi.seal##letter) { \
					separator \
					ImGui::TextUnformatted(sealName " Ghost will disappear if Bedman is hit (non-blocked hit) at any time." \
						" And the Seal will disappear if Bedman is hit (non-blocked hit) on this frame."); \
				} else if (bi.dejavu##letter##GhostInRecovery && bi.seal##letter) { \
					separator \
					ImGui::TextUnformatted("The Seal will disappear if Bedman is hit (non-blocked hit) on this frame."); \
				} else if (!bi.dejavu##letter##GhostInRecovery && !bi.seal##letter) { \
					separator \
					ImGui::TextUnformatted(sealName " Ghost will disappear if Bedman is hit (non-blocked hit) at any time."); \
				} \
			}
		dejavuTask(C, "Task B")
		dejavuTask(D, "Task C")
		#undef dejavuTask
		if (bi.hasShockwaves) {
			separator
			ImGui::TextUnformatted("Any and all Task C and \x44\xC3\xA9\x6A\xC3\xA0 Vu Task C Shockwaves will disappear"
				" if Bedman is hit (non-blocked hit) at any time.");
		}
		if (bi.hasOkkake) {
			separator
			ImGui::TextUnformatted("The Sheep will disappear if Bedman is hit (non-blocked hit) at any time.");
		}
		if (bi.sealAReceivedSignal5) {
			separator
			ImGui::TextUnformatted("On this frame, Task A Seal becomes vulnerable.");
		}
		if (bi.sealBReceivedSignal5) {
			separator
			ImGui::TextUnformatted("On this frame, Task A' Seal becomes vulnerable.");
		}
		if (bi.sealCReceivedSignal5) {
			separator
			ImGui::TextUnformatted("On this frame, Task B Seal becomes vulnerable.");
		}
		if (bi.sealDReceivedSignal5) {
			separator
			ImGui::TextUnformatted("On this frame, Task C Seal becomes vulnerable.");
		}
		#undef separator
	} else if (charType == CHARACTER_TYPE_RAMLETHAL) {
		const RamlethalInfo& ri = frame.u.ramlethalInfo;
		if (ri.sSwordBlockstunLinked
				|| ri.sSwordFallOnHitstun
				|| ri.sSwordRecoilOnHitstun
				|| ri.hSwordBlockstunLinked
				|| ri.hSwordFallOnHitstun
				|| ri.hSwordRecoilOnHitstun
				|| ri.hasLaser
				|| ri.hasLaserSpawnerInStartup
				|| ri.hasSpiral) {
			ImGui::Separator();
		}
		#define swordDisappearMsg(letter, name) \
			if (ri.letter##SwordBlockstunLinked && ri.letter##SwordFallOnHitstun) { \
				ImGui::TextUnformatted(name " will get interrupted if Ramlethal blocks a hit on this frame" \
					" and will fall to the ground if she gets hit (non-blocked hit) on this frame."); \
			} else if (ri.letter##SwordBlockstunLinked && ri.letter##SwordRecoilOnHitstun) { \
				ImGui::TextUnformatted(name " will get interrupted if Ramlethal gets hit or blocks a hit on this frame."); \
			} else if (ri.letter##SwordBlockstunLinked) { \
				ImGui::TextUnformatted(name " will get interrupted if Ramlethal blocks a hit on this frame."); \
			} else if (ri.letter##SwordRecoilOnHitstun) { \
				ImGui::TextUnformatted(name " will get interrupted if Ramlethal gets hit (non-blocked hit) on this frame."); \
			} else if (ri.letter##SwordFallOnHitstun) { \
				ImGui::TextUnformatted(name " will fall to the ground if Ramlethal gets hit (non-blocked hit) on this frame."); \
			}
		swordDisappearMsg(s, "S Sword")
		swordDisappearMsg(h, "H Sword")
		#undef swordDisappearMsg
		
		if (ri.hasLaser) {
			if (ri.hasLaserSpawnerInStartup) {
				ImGui::TextUnformatted("All Lasers will disappear if Ramlethal gets hit (non-blocked hit) at any time,"
					" and all Laser Spawners will disappear if Ramlethal gets hit (non-blocked hit) during the Calvados move.");
			} else if (ri.hasLaserMinionInStartupAndHitstunNotTied) {
				ImGui::TextUnformatted("All Lasers will disappear if Ramlethal gets hit (non-blocked hit) at any time,"
					" but Laser Spawners are not going to disappear.");
			} else {
				ImGui::TextUnformatted("All Lasers will disappear if Ramlethal gets hit (non-blocked hit) at any time.");
			}
		} else if (ri.hasLaserSpawnerInStartup) {
			ImGui::TextUnformatted("All Laser Spawners will disappear if Ramlethal gets hit (non-blocked hit) during the Calvados move.");
		}
		
		if (ri.hasSpiral) {
			ImGui::TextUnformatted("Trance will disappear if Ramlethal gets hit (non-blocked hit) on this frame.");
		}
	} else if (charType == CHARACTER_TYPE_ELPHELT) {
		const ElpheltInfo& ei = frame.u.elpheltInfo;
		if (ei.hasGrenade || ei.hasJD) {
			ImGui::Separator();
			if (ei.hasGrenade) {
				ImGui::TextUnformatted("Berry Pine will explode if Elphelt gets hit (non-blocked hit) at any time.");
			}
			if (ei.hasJD) {
				ImGui::TextUnformatted("j.D will disappear if Elphelt gets hit (non-blocked hit) at any time.");
			}
		}
	} else if (charType == CHARACTER_TYPE_LEO) {
		const LeoInfo& li = frame.u.leoInfo;
		if (li.hasEdgeyowai || li.hasEdgetuyoi) {
			ImGui::Separator();
			if (li.hasEdgeyowai) {
				ImGui::TextUnformatted("S Graviert W\xc3\xbcrde will disappear if Leo gets hit (non-blocked hit) on this frame.");
			}
			if (li.hasEdgetuyoi) {
				ImGui::TextUnformatted("H Graviert W\xc3\xbcrde will disappear if Leo gets hit (non-blocked hit) at any time.");
			}
		}
	} else if (charType == CHARACTER_TYPE_JOHNNY) {
		const JohnnyInfo& ji = frame.u.johnnyInfo;
		if (ji.hasMistKuttsuku || ji.hasMist) {
			ImGui::Separator();
			if (ji.hasMistKuttsuku) {
				ImGui::TextUnformatted("The Bacchus Sigh Debuff on the opponent will disappear if Johnny gets hit at any time.");
			}
			if (ji.hasMist) {
				ImGui::TextUnformatted("The Bacchus Sigh projectile will disappear if Johnny gets hit at any time.");
			}
		}
	} else if (charType == CHARACTER_TYPE_JACKO) {
		const JackoInfo& ji = frame.u.jackoInfo;
		if (ji.hasAegisField
				|| ji.hasServants
				|| ji.hasMagicianProjectile
				|| ji.hasJD
				|| ji.settingPGhost
				|| ji.settingKGhost
				|| ji.settingSGhost
				|| ji.resettingPGhost
				|| ji.resettingKGhost
				|| ji.resettingSGhost
				|| ji.carryingPGhost
				|| ji.carryingKGhost
				|| ji.carryingSGhost
				|| ji.retrievingPGhost
				|| ji.retrievingKGhost
				|| ji.retrievingSGhost) {
			ImGui::Separator();
			if (ji.hasAegisField) {
				ImGui::TextUnformatted("Aegis Field will temporarily disppear if Jack-O is hit at any time.");
			}
			if (ji.hasServants && ji.hasMagicianProjectile) {
				ImGui::TextUnformatted("All Servants and Magician attack projectiles will disappear if Jack-O is hit at any time.");
			} else if (ji.hasServants) {
				ImGui::TextUnformatted("All Servants will disappear if Jack-O is hit at any time.");
			} else if (ji.hasMagicianProjectile) {
				ImGui::TextUnformatted("All Magician attack projectiles will disappear if Jack-O is hit at any time.");
			}
			if (ji.hasJD) {
				ImGui::TextUnformatted("All j.D Fireballs will disappear if Jack-O is hit at any time.");
			}
			if (ji.settingPGhost) {
				ImGui::TextUnformatted("P Ghost will disappear if Jack-O is hit on this frame.");
			} else if (ji.settingKGhost) {
				ImGui::TextUnformatted("K Ghost will disappear if Jack-O is hit on this frame.");
			} else if (ji.settingSGhost) {
				ImGui::TextUnformatted("S Ghost will disappear if Jack-O is hit on this frame.");
			} else if (ji.resettingPGhost) {
				ImGui::TextUnformatted("P Ghost will remain in inventory if Jack-O is hit on this frame.");
			} else if (ji.resettingKGhost) {
				ImGui::TextUnformatted("K Ghost will remain in inventory if Jack-O is hit on this frame.");
			} else if (ji.resettingSGhost) {
				ImGui::TextUnformatted("S Ghost will remain in inventory if Jack-O is hit on this frame.");
			}
			if (ji.retrievingPGhost) {
				ImGui::TextUnformatted("P Ghost will not be returned to the inventory and will drop down if Jack-O is hit on this frame.");
			} else if (ji.retrievingKGhost) {
				ImGui::TextUnformatted("K Ghost will not be returned to the inventory and will drop down if Jack-O is hit on this frame.");
			} else if (ji.retrievingSGhost) {
				ImGui::TextUnformatted("S Ghost will not be returned to the inventory and will drop down if Jack-O is hit on this frame.");
			} else if (ji.carryingPGhost) {
				ImGui::TextUnformatted("P Ghost will drop down if Jack-O is hit while carrying it.");
			} else if (ji.carryingKGhost) {
				ImGui::TextUnformatted("K Ghost will drop down if Jack-O is hit while carrying it.");
			} else if (ji.carryingSGhost) {
				ImGui::TextUnformatted("S Ghost will drop down if Jack-O is hit while carrying it.");
			}
		}
	} else if (charType == CHARACTER_TYPE_HAEHYUN) {
		const HaehyunInfo& hi = frame.u.haehyunInfo;
		if (hi.hasBall || hi.has5D) {
			ImGui::Separator();
			if (hi.hasBall) {
				ImGui::TextUnformatted("Tuning Ball will disappear if Haehyun gets hit or blocks at any time.");
			}
			if (hi.has5D) {
				ImGui::TextUnformatted("5D Projectile will disappear if Haehyun gets hit (non-blocked hit) at any time.");
			}
		}
	} else if (charType == CHARACTER_TYPE_RAVEN) {
		const RavenInfo& ri = frame.u.ravenInfo;
		if (ri.hasNeedle || ri.hasOrb) {
			ImGui::Separator();
			if (ri.hasNeedle) {
				ImGui::TextUnformatted("Needle will disappear if Raven gets hit at any time.");
			}
			if (ri.hasOrb) {
				ImGui::TextUnformatted("Scharf Kugel will disappear if Raven gets hit at any time.");
			}
		}
	} else if (charType == CHARACTER_TYPE_DIZZY) {
		const DizzyInfo& di = frame.u.dizzyInfo;
		if (di.hasIceSpike
				|| di.hasFirePillar
				|| di.hasIceScythe
				|| di.hasFireScythe
				|| di.hasBubble
				|| di.hasFireBubble
				|| di.hasIceSpear
				|| di.hasFireSpearHitstunLink
				|| di.hasFireSpearExplosion
				|| di.hasPFish
				|| di.hasKFish
				|| di.hasSFish
				|| di.hasHFish
				|| di.hasDFish
				|| di.hasLaser
				|| di.hasBakuhatsuCreator
				|| di.hasGammaRay) {
			ImGui::Separator();
			if (di.hasIceSpike) {
				ImGui::TextUnformatted("Ice Spike will disappear if Dizzy is hit (non-blocked hit) at any time.");
			}
			if (di.hasFirePillar) {
				ImGui::TextUnformatted("Fire Pillar will disappear if Dizzy is hit (non-blocked hit) at any time.");
			}
			if (di.hasIceScythe) {
				ImGui::TextUnformatted("Ice Scythe will disappear if Dizzy is hit (non-blocked hit) on this frame.");
			}
			if (di.hasFireScythe) {
				ImGui::TextUnformatted("Fire Scythe will disappear if Dizzy is hit (non-blocked hit) on this frame.");
			}
			if (di.hasBubble) {
				ImGui::TextUnformatted("Bubble will disappear if Dizzy is hit (non-blocked hit) on this frame.");
			}
			if (di.hasFireBubble) {
				ImGui::TextUnformatted("Fire Bubble will disappear if Dizzy is hit (non-blocked hit) on this frame.");
			}
			if (di.hasIceSpear) {
				ImGui::TextUnformatted("Ice Spear will disappear if Dizzy is hit (non-blocked hit) at any time.");
			}
			if (di.hasFireSpearHitstunLink) {
				char* buf = strbuf;
				size_t bufSize = sizeof strbuf;
				int result = sprintf_s(strbuf, "%s", "All Fire Spears will disappear if Dizzy is hit (non-blocked hit) at any time");
				advanceBuf
				
				const char* spearAr[3];
				int spearCount = 0;
				if (di.hasFireSpear1BlockstunLink) {
					spearAr[spearCount++] = "Fire Spear 1";
				}
				if (di.hasFireSpear2BlockstunLink) {
					spearAr[spearCount++] = "Fire Spear 2";
				}
				if (di.hasFireSpear3BlockstunLink) {
					spearAr[spearCount++] = "Fire Spear 3";
				}
				
				for (int i = 0; i < spearCount; ++i) {
					const char* spear = spearAr[i];
					result = sprintf_s(buf, bufSize, "%s%s",
						i == 0
							? ", and "
							: i == spearCount - 1
								? " and "
								: ", ",
						spear);
					advanceBuf
				}
				
				if (spearCount) {
					sprintf_s(buf, bufSize, " will disappear if Dizzy blocks a hit while charging them.");
				} else {
					sprintf_s(buf, bufSize, ".");
				}
				ImGui::TextUnformatted(strbuf);
			}
			if (di.hasFireSpearExplosion) {
				ImGui::TextUnformatted("Fire Spear Explosion will disappear if Dizzy is hit (non-blocked hit) at any time.");
			}
			if (di.hasPFish) {
				ImGui::TextUnformatted("P Blue Fish will disappear if Dizzy is hit (non-blocked hit) at any time.");
			}
			if (di.hasKFish) {
				ImGui::TextUnformatted("K Blue Fish will disappear if Dizzy is hit (non-blocked hit) at any time.");
			}
			if (di.hasSFish) {
				ImGui::TextUnformatted("S Laser Fish will disappear if Dizzy is hit (non-blocked hit) at any time.");
			}
			if (di.hasHFish) {
				ImGui::TextUnformatted("H Laser Fish will disappear if Dizzy is hit (non-blocked hit) at any time.");
			}
			if (di.hasLaser) {
				ImGui::TextUnformatted("Laser will disappear if Dizzy is hit (non-blocked hit) at any time"
					" or if the Laser Fish gets hit.");
			}
			if (di.hasDFish) {
				ImGui::TextUnformatted("Shield Fish will disappear if Dizzy is hit (non-blocked hit) at any time.");
			}
			if (di.hasBakuhatsuCreator) {
				ImGui::TextUnformatted("Imperial Ray Spawner (but not Imperial Ray Pillars that have already been spawned)"
					" will disappear if Dizzy is hit (non-blocked hit) at any time.");
			}
			if (di.hasGammaRay) {
				ImGui::TextUnformatted("Gamma Ray will disappear if Dizzy is hit (non-blocked hit) at any time.");
			}
		}
	} else if (charType == CHARACTER_TYPE_BAIKEN) {
		const BaikenInfo& bi = frame.u.baikenInfo;
		if (bi.has5D
				|| bi.hasJD
				|| bi.hasTeppou
				|| bi.hasTatami) {
			ImGui::Separator();
			if (bi.has5D) {
				ImGui::TextUnformatted("5D Projectile will disappear if Baiken gets hit (non-blocked hit) at any time.");
			}
			if (bi.hasJD) {
				ImGui::TextUnformatted("j.D Projectile will disappear if Baiken gets hit (non-blocked hit) at any time.");
			}
			if (bi.hasTeppou) {
				ImGui::TextUnformatted("Yasha Gatana Projectile will disappear if Baiken gets hit (non-blocked hit) on this frame.");
			}
			if (bi.hasTatami) {
				ImGui::TextUnformatted("Tatami will disappear if Baiken gets hit (non-blocked hit) at any time.");
			}
		}
	} else if (charType == CHARACTER_TYPE_ANSWER) {
		const AnswerInfo& ai = frame.u.answerInfo;
		if (ai.hasCardDestroyOnDamage
				|| ai.hasCardPlayerGotHit
				|| ai.hasClone
				|| ai.hasRSFStart) {
			ImGui::Separator();
			if (ai.hasCardDestroyOnDamage) {
				ImGui::TextUnformatted("Card will disappear if Answer gets hit (non-blocked hit) on this frame.");
			} else if (ai.hasCardPlayerGotHit) {
				ImGui::TextUnformatted("Card will stop being active and will be prevented from becoming active if Answers gets hit (non-blocked hit) at any time.");
			}
			if (ai.hasClone) {
				ImGui::TextUnformatted("Clone will disappear if Answer gets hit (non-blocked hit) at any time.");
			}
			if (ai.hasRSFStart) {
				ImGui::TextUnformatted("Firesale Initial Card will disappear if Answer gets hit (non-blocked hit) at any time.");
			}
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
		
		zerohspacing
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
		_zerohspacing
		
	}
	
	if (frame.powerup) {
		ImGui::Separator();
		const char* newlinePos = nullptr;
		static const char titleOverride[] = "//Title override: ";
		if (strncmp(frame.powerup, titleOverride, sizeof titleOverride - 1) == 0) {
			newlinePos = strchr(frame.powerup, '\n');
		}
		if (newlinePos) {
			if (newlinePos != frame.powerup + sizeof titleOverride - 1) {
				yellowText(frame.powerup + sizeof titleOverride - 1, newlinePos);
			}
			ImGui::TextUnformatted(newlinePos + 1);
		} else {
			yellowText("Reached powerup:");
			ImGui::TextUnformatted(frame.powerup);
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
			if (charType == CHARACTER_TYPE_BEDMAN) {
				ImGui::TextUnformatted("Can't hover due to minimum height requirement.");
			} else {
				ImGui::TextUnformatted("Can't airdash due to minimum height requirement.");
			}
		}
	}
	
	if (charType == CHARACTER_TYPE_SOL) {
		const SolInfo& si = frame.u.solInfo;
		if (si.currentDI) {
			ImGui::Separator();
			zerohspacing
			yellowText("Dragon Install: ");
			ImGui::SameLine();
			if (si.currentDI == USHRT_MAX) {
				ImGui::Text("overdue/%d", si.maxDI);
			} else {
				ImGui::Text("%d/%d", si.currentDI, si.maxDI);
			}
			
			ImGui::PushStyleColor(ImGuiCol_Text, SLIGHTLY_GRAY);
			ImGui::TextUnformatted("This value doesn't decrease in hitstop and superfreeze and decreases"
				" at half the speed when slowed down by opponent's RC.");
			ImGui::PopStyleColor();
			_zerohspacing
		}
	} else if (charType == CHARACTER_TYPE_MILLIA) {
		const MilliaInfo& mi = frame.u.milliaInfo;
		bool insertedSeparator = false;
		if (mi.canProgramSecretGarden || mi.SGInputs) {
			ImGui::Separator();
			insertedSeparator = true;
			ImGui::Text("%sInputs %d/%d",
				mi.canProgramSecretGarden ? "Can program Secret Garden. " : "Secret Garden ",
				mi.SGInputs,
				mi.SGInputsMax);
		}
		if (mi.chromingRose) {
			if (!insertedSeparator) {
				ImGui::Separator();
			}
			zerohspacing
			yellowText("Chroming Rose: ");
			ImGui::SameLine();
			ImGui::Text("%d/%d", mi.chromingRose, mi.chromingRoseMax);
			
			ImGui::PushStyleColor(ImGuiCol_Text, SLIGHTLY_GRAY);
			ImGui::TextUnformatted("This value doesn't decrease in hitstop and superfreeze and decreases"
				" at half the speed when slowed down by opponent's RC.");
			ImGui::PopStyleColor();
			_zerohspacing
		}
		
	} else if (charType == CHARACTER_TYPE_CHIPP) {
		const ChippInfo& ci = frame.u.chippInfo;
		if (ci.invis || ci.wallTime) {
			ImGui::Separator();
			if (ci.invis) {
				printChippInvisibility(ci.invis, endScene.players[playerIndex].maxDI);
			}
			if (ci.wallTime) {
				zerohspacing
				yellowText("Wall time: ");
				ImGui::SameLine();
				if (ci.wallTime == USHRT_MAX) {
					ImGui::TextUnformatted("0/120");
				} else {
					ImGui::Text("%d/120", ci.wallTime);
				}
				ImGui::PushStyleColor(ImGuiCol_Text, SLIGHTLY_GRAY);
				ImGui::PushTextWrapPos(0.F);
				ImGui::TextUnformatted("This value increases slower when opponent slows you down with RC.");
				ImGui::PopTextWrapPos();
				ImGui::PopStyleColor();
				_zerohspacing
			}
		}
	} else if (charType == CHARACTER_TYPE_ZATO) {
		ImGui::Separator();
		zerohspacing
		yellowText("Eddie Gauge: ");
		ImGui::SameLine();
		ImGui::Text("%d/6000", frame.u.zatoInfo.currentEddieGauge);
		_zerohspacing
	} else if (charType == CHARACTER_TYPE_FAUST) {
		if (frame.superArmorActiveInGeneral_IsFull && frame.animName && strcmp(frame.animName->name, "5D") == 0) {
			ImGui::Separator();
			ImGui::TextUnformatted("If Faust gets hit by a reflectable projectile on this frame,"
				" the reflection will be a homerun.");
		}
	} else if (charType == CHARACTER_TYPE_SLAYER) {
		const SlayerInfo& si = frame.u.slayerInfo;
		if (si.currentBloodsuckingUniverseBuff) {
			ImGui::Separator();
			zerohspacing
			yellowText("Bloodsucking Universe Buff: ");
			ImGui::SameLine();
			ImGui::Text("%d/%d", si.currentBloodsuckingUniverseBuff, si.maxBloodsuckingUniverseBuff);
			_zerohspacing
		}
	} else if (charType == CHARACTER_TYPE_INO) {
		const InoInfo& ii = frame.u.inoInfo;
		if (ii.airdashTimer) {
			ImGui::Separator();
			zerohspacing
			yellowText("Airdash active timer: ");
			ImGui::SameLine();
			ImGui::Text("%d", ii.airdashTimer);
			ImGui::PushStyleColor(ImGuiCol_Text, SLIGHTLY_GRAY);
			ImGui::TextUnformatted("While airdash is active, speed Y is continuously set to 0.");
			ImGui::PopStyleColor();
			_zerohspacing
		}
	} else if (charType == CHARACTER_TYPE_BEDMAN) {
		const BedmanInfo& bi = frame.u.bedmanInfo;
		if (bi.sealA || bi.sealB || bi.sealC || bi.sealD) {
			ImGui::Separator();
			printBedmanSeals(bi, true);
		}
	} else if (charType == CHARACTER_TYPE_RAMLETHAL) {
		const RamlethalInfo& ri = frame.u.ramlethalInfo;
		if (ri.sSwordTime || ri.hSwordTime) {
			ImGui::Separator();
			struct BitInfo {
				const char* title;
				int time;
				int timeMax;
				bool isInvulnerable;
			};
			BitInfo bitInfos[2] {
				{
					"S Sword: ",
					ri.sSwordTime,
					ri.sSwordTimeMax,
					ri.sSwordInvulnerable
				},
				{
					"H Sword: ",
					ri.hSwordTime,
					ri.hSwordTimeMax,
					ri.hSwordInvulnerable
				}
			};
			for (int i = 0; i < 2; ++i) {
				BitInfo& bitInfo = bitInfos[i];
				if (bitInfo.time) {
					zerohspacing
					yellowText(bitInfo.title);
					ImGui::SameLine();
					if (bitInfo.timeMax) {
						sprintf_s(strbuf, "%d/%d%s", bitInfo.time, bitInfo.timeMax,
							bitInfo.isInvulnerable ? " (Invulnerable)" : "");
					} else {
						sprintf_s(strbuf, "until landing + %d%s", bitInfo.time,
							bitInfo.isInvulnerable ? " (Invulnerable)" : "");
					}
					ImGui::TextUnformatted(strbuf);
					_zerohspacing
				}
			}
		}
	} else if (charType == CHARACTER_TYPE_ELPHELT) {
		const ElpheltInfo& ei = frame.u.elpheltInfo;
		if (ei.grenadeTimer || ei.grenadeDisabledTimer) {
			ImGui::Separator();
		}
		if (ei.grenadeTimer) {
			zerohspacing
			yellowText("Berry Timer: ");
			ImGui::SameLine();
			sprintf_s(strbuf, "%d/180", ei.grenadeTimer);
			ImGui::TextUnformatted(strbuf);
			_zerohspacing
		}
		if (ei.grenadeDisabledTimer) {
			zerohspacing
			yellowText("Can pull Berry in: ");
			ImGui::SameLine();
			char* buf = strbuf;
			size_t bufSize = sizeof strbuf;
			int result;
			if (ei.grenadeDisabledTimer == 255) {
				result = sprintf_s(buf, bufSize, "%s", "???");
			} else {
				result = sprintf_s(buf, bufSize, "%d", ei.grenadeDisabledTimer);
			}
			advanceBuf
			if (ei.grenadeDisabledTimerMax == 255) {
				sprintf_s(buf, bufSize, "/%s", "???");
			} else {
				sprintf_s(buf, bufSize, "/%d", ei.grenadeDisabledTimerMax);
			}
			ImGui::TextUnformatted(strbuf);
			_zerohspacing
		}
	} else if (charType == CHARACTER_TYPE_JOHNNY) {
		const JohnnyInfo& ji = frame.u.johnnyInfo;
		if (ji.mistTimer || ji.mistTimerMax) {
			ImGui::Separator();
		}
		if (ji.mistTimer) {
			yellowText("Bacchus Sigh Projectile Timer:");
			sprintf_s(strbuf, "%d/%d", ji.mistTimer, ji.mistTimerMax);
			ImGui::TextUnformatted(strbuf);
		}
		
		if (ji.mistKuttsukuTimer) {
			yellowText("Bacchus Sigh Debuff On Opponent:");
			sprintf_s(strbuf, "%d/%d", ji.mistKuttsukuTimer, ji.mistKuttsukuTimerMax);
			ImGui::TextUnformatted(strbuf);
		}
	} else if (charType == CHARACTER_TYPE_JACKO) {
		const JackoInfo& ji = frame.u.jackoInfo;
		if (ji.aegisFieldAvailableIn != JackoInfo::NO_AEGIS_FIELD) {
			zerohspacing
			yellowText("Aegis Field returning in: ");
			ImGui::SameLine();
			if (ji.aegisFieldAvailableIn > 60) {
				ImGui::TextUnformatted("<never>");
			} else {
				sprintf_s(strbuf, "%d/60", 60 - (int)ji.aegisFieldAvailableIn);
				ImGui::TextUnformatted(strbuf);
			}
			_zerohspacing
		}
	} else if (charType == CHARACTER_TYPE_RAVEN) {
		const RavenInfo& ri = frame.u.ravenInfo;
		if (ri.slowTime) {
			ImGui::Separator();
			zerohspacing
			yellowText("Slow Timer On Opponent: ");
			ImGui::SameLine();
			sprintf_s(strbuf, "%d/%d", ri.slowTime, ri.slowTimeMax);
			ImGui::TextUnformatted(strbuf);
			_zerohspacing
			CharacterType opponentCharType = endScene.players[1 - playerIndex].charType;
			if (opponentCharType == CHARACTER_TYPE_ZATO
					|| opponentCharType == CHARACTER_TYPE_LEO
					|| opponentCharType == CHARACTER_TYPE_DIZZY) {
				ImGui::TextUnformatted("If Raven gets hit by his own reflected needle, the slow effect on the opponent will get cancelled.");
			}
		}
	} else if (charType == CHARACTER_TYPE_HAEHYUN) {
		const HaehyunInfo& hi = frame.u.haehyunInfo;
		if (hi.ballTime || hi.superballTime[0].time) {
			ImGui::Separator();
			if (hi.ballTime) {
				zerohspacing
				if (!hi.cantDoBall) {
					yellowText("Ball Time Remaining: ");
				} else {
					yellowText("Can Do Ball In: ");
				}
				ImGui::SameLine();
				sprintf_s(strbuf, "%d/%d", hi.ballTime, hi.ballTimeMax);
				ImGui::TextUnformatted(strbuf);
				_zerohspacing
			}
			for (int i = 0; i < 2; ++i) {
				const HaehyunInfo::TimeAndTimeMax& elem = hi.superballTime[i];
				if (!elem.time) break;
				sprintf_s(strbuf, "Super Ball #%d Time Remaining: ", i + 1);
				zerohspacing
				yellowText(strbuf);
				ImGui::SameLine();
				sprintf_s(strbuf, "%d/%d", elem.time, elem.timeMax);
				ImGui::TextUnformatted(strbuf);
				_zerohspacing
			}
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
			sprintf_s(strbuf, "%d/3", (frame.crossupProtectionIsAbove1 << 1) + frame.crossupProtectionIsOdd);
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
			" get enabled as gatlings from it in addition to any regular gatlings.");
		ImGui::PopStyleColor();
		
	}
}

// runs on the main thread
template<typename FrameT>
inline void drawFrameTooltip(FrameT& frame, int playerIndex, bool useSlang,
			const SkippedFramesInfo& skippedFramesElem, CharacterType owningPlayerCharType,
			const PlayerFrame& correspondingPlayersFrame, int nestingLevel,
			std::vector<const char*>& printedDescriptions,
			bool* skippedFramesShown, bool* startedSayingRepeatedNTimes) {
	
	Frame& projectileFrame = (Frame&)frame;
	const PlayerFrame& playerFrame = (const PlayerFrame&)frame;
	
	bool calledInnerPrint = false;
	int repeatCount = 1;
	
	if (playerIndex == -1 && projectileFrame.next && (nestingLevel > 0 || projectileFrame.next->next)) {
		Frame* firstUnaccountedFor = nullptr;
		
		if (nestingLevel > 0) {
			projectileFrame.accountedFor = true;
			for (Frame* framePtr = projectileFrame.next; framePtr; framePtr = framePtr->next) {
				if (!framePtr->accountedFor) {
					if (*framePtr == projectileFrame) {
						framePtr->accountedFor = true;
						++repeatCount;
					} else if (!firstUnaccountedFor) {
						firstUnaccountedFor = framePtr;
					}
				}
			}
		} else {
			firstUnaccountedFor = projectileFrame.next;
		}
		
		if (firstUnaccountedFor) {
			drawFrameTooltip(*firstUnaccountedFor, playerIndex, useSlang,
				skippedFramesElem, owningPlayerCharType, correspondingPlayersFrame, nestingLevel + 1,
				printedDescriptions, skippedFramesShown, startedSayingRepeatedNTimes);
			if (nestingLevel == 0) return;
			calledInnerPrint = true;
		}
	}
	
	if (repeatCount > 1 || *startedSayingRepeatedNTimes) {
		*startedSayingRepeatedNTimes = true;
		ImGui::Separator();
		ImGui::Separator();
		zerohspacing
		ImGui::TextUnformatted("** The following projectile repeats ");
		ImGui::SameLine();
		sprintf_s(strbuf, "%d", repeatCount);
		yellowText(strbuf);
		ImGui::SameLine();
		ImGui::TextUnformatted(repeatCount == 1 ? " time **" : " times **");
		_zerohspacing
		ImGui::Separator();
		ImGui::Separator();
	} else if (calledInnerPrint) {
		ImGui::Separator();
		static const StringWithLength stars = "**********";
		static const char* starsEnd = stars.txt + stars.length;
		float starsWidth = ImGui::CalcTextSize(stars.txt, starsEnd).x;
		ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - starsWidth) * 0.5F);
		ImGui::TextUnformatted(stars.txt, starsEnd);
		ImGui::Separator();
	}
	
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
	
	FrameType descriptionType = frame.type;
	if (frame.newHit && frameTypeActive(frame.type)) {
		(char&)descriptionType += FT_ACTIVE_NEW_HIT - FT_ACTIVE;
	}
	const StringWithLength& description = drawFramebars_frameArtArray[descriptionType].description;
	if (description.length) {
		bool descAlreadyPrinted = std::find(
				printedDescriptions.begin(),
				printedDescriptions.end(),
				description.txt
			) != printedDescriptions.end();
		
		static const size_t descriptionLimit = 50;
		static char descriptionsElided[FT_LAST][descriptionLimit + 1] { { '\0' } };
		StringWithLength descriptionUseData;
		const StringWithLength* descriptionUse;
		
		if (descAlreadyPrinted && description.length > descriptionLimit) {
			char* descriptionElided = descriptionsElided[descriptionType];
			if (descriptionElided[0] == '\0') {
				memcpy(descriptionElided, description.txt, descriptionLimit - 3);
				memset(descriptionElided + descriptionLimit - 3, '.', 3);
				descriptionElided[descriptionLimit] = '\0';
			}
			descriptionUseData.txt = descriptionElided;
			descriptionUseData.length = descriptionLimit;
			descriptionUse = &descriptionUseData;
		} else {
			descriptionUse = &description;
		}
		
		ImGui::TextUnformatted(descriptionUse->txt, descriptionUse->txt + descriptionUse->length);
		
		if (!descAlreadyPrinted) {
			printedDescriptions.push_back(description.txt);
		}
	}
	
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
	
	int ramlethalTime = 0;
	int ramlethalTimeMax = 0;
	const char* ramlethalSubAnim = nullptr;
	bool ramlethalInvulnerable = false;
	
	if (playerIndex == -1
			&& owningPlayerCharType == CHARACTER_TYPE_RAMLETHAL
			&& (projectileFrame.charSpecific1 || projectileFrame.charSpecific2)) {
		const RamlethalInfo& ri = correspondingPlayersFrame.u.ramlethalInfo;
		if (projectileFrame.charSpecific1) {
			ramlethalTime = ri.sSwordTime;
			ramlethalTimeMax = ri.sSwordTimeMax;
			ramlethalSubAnim = ri.sSwordSubAnim;
			ramlethalInvulnerable = ri.sSwordInvulnerable;
		} else {  // projectileFrame.charSpecific2
			ramlethalTime = ri.hSwordTime;
			ramlethalTimeMax = ri.hSwordTimeMax;
			ramlethalSubAnim = ri.hSwordSubAnim;
			ramlethalInvulnerable = ri.hSwordInvulnerable;
		}
	}
	
	const char* name;
	if (frame.animName) {
		if (useSlang && frame.animName->slang && *frame.animName->slang != '\0') {
			name = frame.animName->slang;
		} else {
			name = frame.animName->name;
		}
	} else {
		name = nullptr;
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
		ui.drawPlayerFrameTooltipInfo(playerFrame, playerIndex, wrapWidth);
	} else if (frame.powerup) {
		ImGui::Separator();
		if (owningPlayerCharType == CHARACTER_TYPE_INO) {
			ImGui::TextUnformatted("The note reached the next level on this frame: it will deal one more hit.");
		} else if (owningPlayerCharType == CHARACTER_TYPE_ELPHELT) {
			ImGui::TextUnformatted("The Berry Pine reached a powerup on this frame: it will deal significantly more hitstun or blockstun,"
				" and will have less pushback on air hit, but more pushback on ground hit or block, and the Berry won't bounce off the floor anymore.");
		} else {
			ImGui::TextUnformatted("The projectile reached some kind of powerup on this frame.");
		}
	}
	if (playerIndex == -1) {
		if (owningPlayerCharType == CHARACTER_TYPE_INO) {
			if (projectileFrame.animName == MOVE_NAME_NOTE) {
				// I am a dirty scumbag
				ImGui::Separator();
				zerohspacing
				yellowText("Note elapsed time: ");
				ImGui::SameLine();
				int time = correspondingPlayersFrame.u.inoInfo.noteTime;
				int timeMax = correspondingPlayersFrame.u.inoInfo.noteTimeMax;
				const char* txt;
				if (correspondingPlayersFrame.u.inoInfo.noteLevel == 5) {
					txt = "68";
				} else {
					txt = ui.printDecimal(correspondingPlayersFrame.u.inoInfo.noteTimeMax, 0, 0, false);
				}
				sprintf_s(strbuf, "%d/%s (%d hits)", time, txt, correspondingPlayersFrame.u.inoInfo.noteLevel);
				ImGui::TextUnformatted(strbuf);
				_zerohspacing
			}
		} else if (ramlethalTime) {
			ImGui::Separator();
			zerohspacing
			yellowText("Time Remaining: ");
			ImGui::SameLine();
			if (ramlethalTimeMax) {
				sprintf_s(strbuf, "%d/%d%s", ramlethalTime, ramlethalTimeMax, ramlethalInvulnerable ? " (Invulnerable)" : "");
			} else {
				sprintf_s(strbuf, "until landing + %d%s", ramlethalTime, ramlethalInvulnerable ? " (Invulnerable)" : "");
			}
			ImGui::TextUnformatted(strbuf);
			_zerohspacing
		} else if (owningPlayerCharType == CHARACTER_TYPE_ELPHELT) {
			if (projectileFrame.animName == PROJECTILE_NAME_BERRY || projectileFrame.animName == PROJECTILE_NAME_BERRY_BUFFED) {
				ImGui::Separator();
				zerohspacing
				yellowText("Berry Timer: ");
				ImGui::SameLine();
				sprintf_s(strbuf, "%d/180", correspondingPlayersFrame.u.elpheltInfo.grenadeTimer);
				ImGui::TextUnformatted(strbuf);
				_zerohspacing
			}
		} else if (owningPlayerCharType == CHARACTER_TYPE_JOHNNY) {
			if (projectileFrame.animName == PROJECTILE_NAME_BACCHUS) {
				ImGui::Separator();
				zerohspacing
				yellowText("Bacchus Sigh Projectile Timer: ");
				ImGui::SameLine();
				sprintf_s(strbuf, "%d/%d", correspondingPlayersFrame.u.johnnyInfo.mistTimer,
					correspondingPlayersFrame.u.johnnyInfo.mistTimerMax);
				ImGui::TextUnformatted(strbuf);
				_zerohspacing
			}
		} else if (owningPlayerCharType == CHARACTER_TYPE_JACKO) {
			if (frame.type == FT_IDLE_NO_DISPOSE
					&& projectileFrame.animName == PROJECTILE_NAME_GHOST) {
				ImGui::Separator();
				ImGui::TextUnformatted("The Ghost is strike invulnerable.");
			}
		} else if (owningPlayerCharType == CHARACTER_TYPE_DIZZY) {
			if (frame.type == FT_IDLE_PROJECTILE_HITTABLE
					&& correspondingPlayersFrame.u.dizzyInfo.shieldFishSuperArmor) {
				ImGui::Separator();
				ImGui::TextUnformatted("Shield Fish super armor active.");
			}
		} else if (owningPlayerCharType == CHARACTER_TYPE_HAEHYUN) {
			if (projectileFrame.animName == PROJECTILE_NAME_TUNING_BALL) {
				ImGui::Separator();
				zerohspacing
				yellowText("Ball Time Remaining: ");
				ImGui::SameLine();
				sprintf_s(strbuf, "%d/%d", correspondingPlayersFrame.u.haehyunInfo.ballTime,
					correspondingPlayersFrame.u.haehyunInfo.ballTimeMax);
				ImGui::TextUnformatted(strbuf);
				_zerohspacing
			} else if (projectileFrame.animName == PROJECTILE_NAME_CELESTIAL_TUNING_BALL) {
				ImGui::Separator();
				zerohspacing
				yellowText("Celestial Ball Time Remaining: ");
				ImGui::SameLine();
				if (projectileFrame.charSpecific1 == projectileFrame.charSpecific2) {
					ImGui::TextUnformatted("??/??");
				} else {
					int index = projectileFrame.charSpecific1 ? 0 : 1;
					sprintf_s(strbuf, "%d/%d", correspondingPlayersFrame.u.haehyunInfo.superballTime[index].time,
						correspondingPlayersFrame.u.haehyunInfo.superballTime[index].timeMax);
					ImGui::TextUnformatted(strbuf);
				}
				_zerohspacing
			}
		}
	}
	if (playerIndex != -1) {
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
	if (skippedFramesElem.count || frame.rcSlowdown || frame.hitConnected || frame.newHit) {
		ImGui::Separator();
		if (skippedFramesElem.count && !*skippedFramesShown) {
			*skippedFramesShown = true;
			skippedFramesElem.print(frameAssumesCanBlockButCantFDAfterSuperfreeze(frame.type));
		}
		if (frame.newHit) {
			ImGui::TextUnformatted("A new (potential) hit starts on this frame.");
		}
		if (playerIndex != -1 && playerFrame.blockedOnThisFrame) {
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
			sprintf_s(strbuf, "%d/%d ", frame.rcSlowdown, frame.rcSlowdownMax);
			ImGui::TextUnformatted(strbuf);
			ImGui::SameLine();
			zerohspacing
			textUnformattedColored(LIGHT_BLUE_COLOR, "(frame ");
			ImGui::SameLine();
			if (frame.rcSlowdown % 2 != 0) {
				textUnformattedColored(FRAME_SKIPPED_COLOR, "skipped");
			} else {
				textUnformattedColored(FRAME_ADVANCED_COLOR, "advanced");
			}
			ImGui::SameLine();
			textUnformattedColored(LIGHT_BLUE_COLOR, ")");
			_zerohspacing
		}
	}
	if (playerIndex != -1) {
		ui.drawPlayerFrameInputsInTooltip(playerFrame, playerIndex);
	}
	ImGui::PopTextWrapPos();
	ImGui::PopStyleVar();
	
}

/// <summary>
/// Draws backgrounds of frames - the base frame graphics. Also registers mouse hovering over a frame and draws the frame tooltip window.
/// runs on the main thread
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
inline void drawFramebar(FramebarT& framebar, UI::FrameDims* preppedDims, ImU32 tintDarker, int playerIndex,
			const std::vector<SkippedFramesInfo>& skippedFrames, const PlayerFramebar& correspondingPlayersFramebar,
			CharacterType owningPlayerCharType, float frameHeight, const FrameAddon& newHitArt,
			const HitConnectedArtSelector& hitConnectedArtSelector) {
	const bool useSlang = settings.useSlangNames;
	const int framesCount = settings.framebarDisplayedFramesCount;
	
	int internalINext = iterateVisualFramesFrom0_getInitialInternalInd();
	int internalI;
	
	for (int visualI = 0; visualI < drawFramebars_framesCount; ++visualI) {
		
		internalI = internalINext;
		iterateVisualFrames_incrementInternalInd(internalINext);
		
		FrameT& frame = framebar[internalI];
		const Frame& projectileFrame = (const Frame&)frame;
		const PlayerFrame& correspondingPlayersFrame = correspondingPlayersFramebar[internalI];
		const SkippedFramesInfo& skippedFramesElem = skippedFrames[internalI];
		const UI::FrameDims& dims = preppedDims[visualI];
		
		ImVec2 frameStartVec { dims.x, drawFramebars_y };
		ImVec2 frameEndVec { dims.x + dims.width, drawFramebars_y + frameHeight };
		const StringWithLength* description = nullptr;
		
		if (frame.type != FT_NONE) {
			
			ImU32 tint = -1;
			if (visualI > drawFramebars_framebarPositionDisplay) {
				tint = tintDarker;
			}
			
			const FrameArt* frameArt = &drawFramebars_frameArtArray[frame.type];
			drawFramebars_drawList->AddImage(TEXID_FRAMES_FRAMEBAR,
				frameStartVec,
				frameEndVec,
				frameArt->framebar.start,
				frameArt->framebar.end,
				tint);
			
			if (frame.newHit && frameTypeActive(frame.type)) {
				
				if (frame.hitConnected) {
					frameStartVec.x += 1.F;
				}
				
				ImVec2 newHitEndVec {
					frameStartVec.x + newHitArt.framebar.size.x,
					frameEndVec.y
				};
				if (newHitEndVec.x > frameEndVec.x) newHitEndVec.x = frameEndVec.x;
				
				drawFramebars_drawList->AddImage(TEXID_FRAMES_FRAMEBAR,
					frameStartVec,
					newHitEndVec,
					newHitArt.framebar.start,
					newHitArt.framebar.end,
					tint);
				
				if (frame.hitConnected) {
					frameStartVec.x -= 1.F;
				}
			}
			
			if (frame.activeDuringSuperfreeze) {
				const FrameArt& superfreezeActiveArt = drawFramebars_frameArtArray[playerIndex == -1 ? FT_ACTIVE_PROJECTILE : FT_ACTIVE];
				drawFramebars_drawList->AddImage(TEXID_FRAMES_FRAMEBAR,
					{
						frameStartVec.x + dims.width * 0.5F,
						frameStartVec.y
					},
					frameEndVec,
					{
						(superfreezeActiveArt.framebar.start.x + superfreezeActiveArt.framebar.end.x) * 0.5F,
						superfreezeActiveArt.framebar.start.y
					},
					superfreezeActiveArt.framebar.end,
					tint);
			}
			
			if (frame.hitConnected) {
				const FrameAddon* art;
				if (frame.type == FT_XSTUN
						|| frame.type == FT_XSTUN_CAN_CANCEL
						|| frame.type == FT_GRAYBEAT_AIR_HITSTUN) {
					art = dims.hitConnectedShouldBeAlt
						? hitConnectedArtSelector.hitConnectedBlackAlt
						: hitConnectedArtSelector.hitConnectedBlack;
				} else {
					art = dims.hitConnectedShouldBeAlt
						? hitConnectedArtSelector.hitConnectedAlt
						: hitConnectedArtSelector.hitConnected;
				}
				
				drawFramebars_drawList->AddImage(TEXID_FRAMES_FRAMEBAR,
					frameStartVec,
					frameEndVec,
					art->framebar.start,
					art->framebar.end,
					tint);
				
			}
		}
		
		if (drawFramebars_hoveredFrameIndex == -1 && ImGui::IsWindowHovered()) {
			ImVec2 frameEndVecForTooltip;
			const ImVec2* frameEndVecForTooltipPtr;
			if (visualI < drawFramebars_framesCount - 1) {
				frameEndVecForTooltip = { preppedDims[visualI + 1].x, frameEndVec.y };
				frameEndVecForTooltipPtr = &frameEndVecForTooltip;
			} else {
				frameEndVecForTooltipPtr = &frameEndVec;
			}
			if (ImGui::IsMouseHoveringRect(frameStartVec, *frameEndVecForTooltipPtr, true)) {
				drawFramebars_hoveredFrameIndex = visualI;
				drawFramebars_hoveredFrameY = drawFramebars_y;
				drawFramebars_hoveredFrameHeight = frameHeight;
				if (frame.type != FT_NONE && ImGui::BeginTooltip()) {
					static std::vector<const char*> printedDescriptions;
					printedDescriptions.clear();
					bool skippedFramesShown = false;
					bool startedSayingRepeatedNTimes = false;
					
					if (playerIndex == -1) {
						for (Frame* framePtr = projectileFrame.next; framePtr; framePtr = framePtr->next) {
							framePtr->accountedFor = false;
						}
					}
					
					drawFrameTooltip(frame, playerIndex, useSlang,
						skippedFramesElem, owningPlayerCharType, correspondingPlayersFrame, 0,
						printedDescriptions, &skippedFramesShown, &startedSayingRepeatedNTimes);
					
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

// runs on the main thread
void drawPlayerFramebar(PlayerFramebar& framebar, UI::FrameDims* preppedDims, ImU32 tintDarker, int playerIndex,
			const std::vector<SkippedFramesInfo>& skippedFrames, CharacterType charType, float frameHeight,
			const FrameAddon& newHitArt, const HitConnectedArtSelector& hitConnectedArtSelector) {
	drawFramebar<PlayerFramebar, PlayerFrame>(framebar, preppedDims, tintDarker, playerIndex, skippedFrames, framebar, charType,
		frameHeight, newHitArt, hitConnectedArtSelector);
}

// runs on the main thread
void drawProjectileFramebar(Framebar& framebar, UI::FrameDims* preppedDims, ImU32 tintDarker,
			const std::vector<SkippedFramesInfo>& skippedFrames, const PlayerFramebar& correspondingPlayersFramebar,
			CharacterType owningPlayerCharType, float frameHeight, const FrameAddon& newHitArt,
			const HitConnectedArtSelector& hitConnectedArtSelector) {
	drawFramebar<Framebar, Frame>(framebar, preppedDims, tintDarker, -1, skippedFrames, correspondingPlayersFramebar, owningPlayerCharType,
		frameHeight, newHitArt, hitConnectedArtSelector);
}

// runs on the main thread
template<typename FramebarT, typename FrameT>
void drawFirstFrames(const FramebarT& framebar, UI::FrameDims* preppedDims, float firstFrameTopY, float firstFrameBottomY, bool* onlyReport) {
	const bool considerSimilarFrameTypesSameForFrameCounts = settings.considerSimilarFrameTypesSameForFrameCounts;
	const bool considerSimilarIdleFramesSameForFrameCounts = settings.considerSimilarIdleFramesSameForFrameCounts;
	const int startFrame = drawFramebars_framebarPosition == _countof(Framebar::frames) - 1
					? 0
					: drawFramebars_framebarPosition + 1;
	int internalIndNext = iterateVisualFramesFrom0_getInitialInternalInd();
	int internalInd;
	for (int visualInd = 0; visualInd < drawFramebars_framesCount; ++visualInd) {
		
		internalInd = internalIndNext;
		iterateVisualFrames_incrementInternalInd(internalIndNext);
		
		const FrameT& frame = framebar[internalInd];
		const UI::FrameDims& dims = preppedDims[visualInd];
		
		bool isFirst = frame.isFirst;
		if (isFirst
				&& considerSimilarFrameTypesSameForFrameCounts  // this whole block is actually all about idle frames.
				// We're just checking considerSimilarFrameTypesSameForFrameCounts for some reason
				
				&& considerSimilarIdleFramesSameForFrameCounts
				&& frameMap(frame.type) == FT_IDLE) {
			if (internalInd == startFrame) {
				isFirst = framebar.preFrameMapped != FT_IDLE;
			} else {
				isFirst = frameMap(framebar[internalInd == 0 ? _countof(Framebar::frames) - 1 : internalInd - 1].type) != FT_IDLE;
			}
		}
		if (isFirst) {
			
			if (onlyReport) {
				*onlyReport = true;
				return;
			}
			
			ImVec2 artStart {
				dims.x - std::floorf(drawFramebars_innerBorderThicknessHalf + firstFrameArt.framebar.size.x * 0.5F),
				firstFrameTopY
			};
			ImVec2 artEnd {
				artStart.x + firstFrameArt.framebar.size.x,
				firstFrameBottomY
			};
			drawFramebars_drawList->AddImage(TEXID_FRAMES_FRAMEBAR,
				artStart,
				artEnd,
				firstFrameArt.framebar.start,
				firstFrameArt.framebar.end,
				-1);
		}
	}
	
	if (onlyReport) {
		*onlyReport = false;
	}
}

// runs on the main thread
void drawDigit(char digit, const UI::FrameDims& dims, float frameNumberYTop, float frameNumberYBottom, ImU32 tint, const DigitUVs* uvs) {
	const DigitUVs& digitImg = uvs[digit];
	
	float digitX = dims.x;
	float digitWidth = digitImg.framebar.size.x;
	
	digitX += std::floorf((dims.width - digitWidth) * 0.5F);
	
	drawFramebars_drawList->AddImage(TEXID_FRAMES_FRAMEBAR,
		{ digitX, frameNumberYTop },
		{ digitX + digitWidth, frameNumberYBottom },
		digitImg.framebar.start,
		digitImg.framebar.end,
		tint);
}

// Draws frame counts of contiguous groups of similarly-typed frames, on top of the frames
// runs on the main thread
template<typename FramebarT, typename FrameT>
void drawDigits(const FramebarT& framebar, UI::FrameDims* preppedDims, float frameNumberYTop, float frameNumberYBottom,
			char* hasDigit, const DigitUVs* uvs) {
	
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
	int positionWithUndoneScroll = drawFramebars_framebarPosition + ui.framebarSettings.scrollXInFrames;
	if (positionWithUndoneScroll > _countof(Framebar::frames)) {
		positionWithUndoneScroll -= (int)_countof(Framebar::frames);
	}
	int internalIndNext = positionWithUndoneScroll == _countof(Framebar::frames) - 1
		? 0
		: positionWithUndoneScroll + 1;
	int internalInd;
	bool prevIndInView = false;
	int prevVisualInd;
	
	const int iEnd = (int)_countof(Framebar::frames) - ui.framebarSettings.scrollXInFrames;
	const int iLast = iEnd - 1;
	for (int i = 0; i < iEnd; ++i) {
		
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
				&& i == iLast
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
				
				drawDigit(remainder, preppedDims[displayPosIter], frameNumberYTop, frameNumberYBottom, -1, uvs);
				
				hasDigit[displayPosIter] = remainder + 1;
				
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

// runs on the main thread
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

// runs on the main thread
void textUnformattedColored(ImVec4 color, const char* str, const char* strEnd) {
	ImGui::PushStyleColor(ImGuiCol_Text, color);
	ImGui::TextUnformatted(str, strEnd);
	ImGui::PopStyleColor();
}

// runs on the main thread
void yellowText(const char* str, const char* strEnd) {
	ImGui::PushStyleColor(ImGuiCol_Text, YELLOW_COLOR);
	ImGui::TextUnformatted(str, strEnd);
	ImGui::PopStyleColor();
}

// runs on the main thread
// without needManualMultilineOutput, the function draws one line and then the rest of all the other lines below in one TextUnformatted call.
// needManualMultilineOutput prevents the remainder of the lines from becoming one giant box.
// Imagine you would want to draw something extra at the end of the text you printed with this function, on the last line of it.
// If the "rest of the lines" was multiple lines of text, now you will never find its actual right extent.
// Since needManualMultilineOutput prints the "rest of the lines" one by one, you will be able to draw on the right of the last line.
// If you don't plan to do that, pass needManualMultilineOutput false.
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
		wrapWidthUse = wrapWidth - ImGui::GetCursorPosX();
		wrapPos = font->CalcWordWrapPositionA(1.F, str, textEnd, wrapWidthUse);
	}
	if (wrapPos == textEnd) {
		ImGui::TextUnformatted(str, textEnd);
		return;
	} else {
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
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetStyle().ItemSpacing.y);
		}
	}
}

// runs on the main thread
static void drawTextButParenthesesInGrayColor(const char* str) {
	
	struct StyleVarPopper {
		~StyleVarPopper() {
			ImGui::PopStyleVar();
		}
	} styleVarPopper;
	
	static const ImVec2 twoZeros { 0.F, 0.F };
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, twoZeros);
	
	const float wrapWidth = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x;
	const char* const strEnd = str + strlen(str);
	const char* newlinePtr = (const char*)memchr(str, '\n', strEnd - str);
	if (!newlinePtr) newlinePtr = strEnd;
	const char* bracePtr = (const char*)memchr(str, '(', strEnd - str);
	if (!bracePtr) bracePtr = strEnd;
	const char* ptr;
	bool lastWasBrace = false;
	
	// the reason we don't use ImGui::TextUnformatted() to render a text that could potentially wrap into multiple lines,
	// is because when doing ImGui::SameLine() from it afterwards, it would go to the right of its whole bounding box,
	// and not to the right from the last character of its last line.
	// We really do have to render text line-by-line
	// When I first started attacking this problem, when writing the drawOneLineOnCurrentLineAndTheRestBelow function,
	// I tried to hack around ImGui and get the position of the last vertex in the draw list. But text may not get rendered
	// due to scrolling, as clipping is performed in ImGui very early on. So hacking around ImGui is not the solution
	
	while (str < strEnd) {
		if (newlinePtr < str) {
			newlinePtr = (const char*)memchr(str, '\n', strEnd - str);
			if (!newlinePtr) newlinePtr = strEnd;
		}
		if (bracePtr < str) {
			bracePtr = (const char*)memchr(str, '(', strEnd - str);
			if (!bracePtr) bracePtr = strEnd;
		}
		if (bracePtr == strEnd) {
			ImGui::PopStyleVar();
			ImGui::PushStyleVarX(ImGuiStyleVar_ItemSpacing, 0.F);
			ImGui::TextUnformatted(str);
			return;
		}
		
		if (newlinePtr < bracePtr) {
			if (lastWasBrace) {
				drawOneLineOnCurrentLineAndTheRestBelow(wrapWidth,
					str, newlinePtr, true, true, false);
			} else {
				ImGui::TextUnformatted(str, newlinePtr);
			}
			ptr = newlinePtr + 1;
			while (ptr < strEnd && *ptr <= 32) ++ptr;
			str = ptr;
			lastWasBrace = false;
			continue;
		}
		
		const char* closeBracePtr;
		if (bracePtr + 1 < strEnd) {
			closeBracePtr = (const char*)memchr(bracePtr + 1, ')', strEnd - (bracePtr + 1));
		} else {
			closeBracePtr = nullptr;
		}
		
		bool insertLineBreak = false;
		const char* strNext;
		bool isLastLine;
		if (!closeBracePtr) {
			ptr = strEnd;
			isLastLine = true;
			strNext = strEnd;
		} else {
			ptr = closeBracePtr + 1;
			strNext = ptr;
			while (ptr < strEnd && *ptr <= 32) {
				if (*ptr == '\n') {
					insertLineBreak = true;
					strNext = ptr + 1;
				}
				++ptr;
			}
			isLastLine = ptr >= strEnd;
		}
		
		const char* textPortionEnd = closeBracePtr ? closeBracePtr + 1 : strEnd;
		if (bracePtr > str) {
			drawOneLineOnCurrentLineAndTheRestBelow(wrapWidth,
				str, bracePtr, lastWasBrace, true, false);
			lastWasBrace = true;
		}
		if (isLastLine) {
			ImGui::PopStyleVar();
			ImGui::PushStyleVarX(ImGuiStyleVar_ItemSpacing, 0.F);
		}
		ImGui::PushStyleColor(ImGuiCol_Text, SLIGHTLY_GRAY);
		drawOneLineOnCurrentLineAndTheRestBelow(wrapWidth,
			bracePtr, textPortionEnd, lastWasBrace, true, isLastLine);
		ImGui::PopStyleColor();
		
		lastWasBrace = !insertLineBreak;
		str = strNext;
	}
}

// runs on the main thread
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

// runs on the main thread
void UI::startupOrTotal(int two, StringWithLength title, PinnedWindowEnum windowIndex) {
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
			headerThatCanBeClickedForTooltip(searchFieldTitle(title), windowIndex, false);
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

// runs on the main thread
bool UI::booleanSettingPresetWithHotkey(bool& settingsRef, std::vector<int>& hotkey) {
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

// runs on the main thread
bool UI::booleanSettingPreset(bool& settingsRef) {
	bool itHappened = false;
	bool boolValue = settingsRef;
	StringWithLength text = settings.getOtherUINameWithLength(&settingsRef);
	if (settingsPresetsUseOutlinedText) {
		pushOutlinedText(true);
	}
	if (ImGui::Checkbox(searchFieldTitle(text), &boolValue)) {
		settingsRef = boolValue;
		needWriteSettings = true;
		itHappened = true;
	}
	if (settingsPresetsUseOutlinedText) {
		popOutlinedText();
	}
	ImGui::SameLine();
	HelpMarker(searchTooltip(settings.getOtherUIDescriptionWithLength(&settingsRef)));
	return itHappened;
}

// runs on the main thread
bool UI::float4SettingPreset(float& settingsPtr, float minValue, float maxValue, float step, float stepFast, float width) {
	bool attentionPossiblyNeeded = false;
	float floatValue = settingsPtr;
	if (width != 0.F) {
		ImGui::SetNextItemWidth(width);
	}
	if (ImGui::InputFloat(searchFieldTitle(settings.getOtherUINameWithLength(&settingsPtr)), &floatValue, step, stepFast, "%.4f")) {
		if (floatValue < minValue) floatValue = minValue;
		if (floatValue > maxValue) floatValue = maxValue;
		settingsPtr = floatValue;
		needWriteSettings = true;
		attentionPossiblyNeeded = true;
	}
	imguiActiveTemp = imguiActiveTemp || ImGui::IsItemActive();
	ImGui::SameLine();
	HelpMarker(searchTooltip(settings.getOtherUIDescriptionWithLength(&settingsPtr)));
	return attentionPossiblyNeeded;
}

// runs on the main thread
bool UI::intSettingPreset(int& settingsPtr, int minValue, int step, int stepFast, float fieldWidth, int maxValue, bool isDisabled) {
	bool isChange = false;
	int oldValue = settingsPtr;
	int intValue = settingsPtr;
	ImGui::SetNextItemWidth(fieldWidth);
	if (isDisabled) {
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5F, 0.5F, 0.5F, 1.F));
	}
	if (ImGui::InputInt(searchFieldTitle(settings.getOtherUINameWithLength(&settingsPtr)), &intValue, step, stepFast,
			isDisabled ? ImGuiInputTextFlags_ReadOnly : 0)) {
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
	if (isDisabled) {
		ImGui::PopStyleColor();
	}
	if (!isChange && oldValue != intValue) {
		needWriteSettings = true;
		isChange = true;
	}
	imguiActiveTemp = imguiActiveTemp || ImGui::IsItemActive();
	ImGui::SameLine();
	HelpMarker(searchTooltip(settings.getOtherUIDescriptionWithLength(&settingsPtr)));
	return isChange;
}

// runs on the main thread
void drawPlayerIconInWindowTitle(int playerIndex) {
	GGIcon scaledIcon = scaleGGIconToHeight(getPlayerCharIcon(playerIndex), 14.F);
	drawPlayerIconInWindowTitle(scaledIcon);
}

// runs on the main thread
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
		windowPos.x + windowWidth - style.FramePadding.x - fontSize - style.ItemInnerSpacing.x
			- (ui.lastCustomBeginHadPinButton ? 19.F + style.ItemInnerSpacing.x : 0.F),
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

// runs on the main thread
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
		windowPos.x + windowWidth - style.FramePadding.x - fontSize - style.ItemInnerSpacing.x
			- (ui.lastCustomBeginHadPinButton ? 19.F + style.ItemInnerSpacing.x : 0.F),
		startPos.y + icon.size.y
	};
	if (clipEnd.x > startPos.x) {
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->PushClipRect(startPos,
			clipEnd,
			false);
		int alpha = ImGui::IsWindowCollapsed() ? 128 : 255;
		drawList->AddImage(TEXID_GGICON,
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

// runs on the main thread
template<typename T>
void UI::printAllCancels(const T& cancels,
		bool enableSpecialCancel,
		bool clashCancelTimer,
		bool enableJumpCancel,
		bool enableSpecials,
		bool hitAlreadyHappened,
		bool airborne,
		const char* canYrc,
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
	searchFieldTitle("YRC");
	searchFieldTitle("Gatlings");
	searchFieldTitle("Whiff Cancels");
	searchFieldTitle("Late Cancels");
	searchFieldTitle("Jump cancel");
	searchFieldTitle("Specials");
	if (canYrc) {
		if (canYrc == (const char*)1) {
			yellowText("YRC");
		} else {
			const float wrapWidth = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x;
			yellowText("YRC: ");
			drawOneLineOnCurrentLineAndTheRestBelow(wrapWidth, canYrc);
		}
	}
	if (!cancels.gatlings.empty() || enableSpecialCancel || clashCancelTimer || enableJumpCancel) {
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
		if (clashCancelTimer && ImGui::GetCursorPosY() < maxY) {
			ImGui::Text("%d) Clash cancels", count);
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
	if (cancels.gatlings.empty() && !enableSpecialCancel && !clashCancelTimer
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

// runs on the main thread
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

// runs on the main thread
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
				settings.useSlangNames,
				true,
				false,
				&lastNameDuration);
		return true;
	}
	return false;
}

// runs on the main thread
void headerThatCanBeClickedForTooltip(const char* title, PinnedWindowEnum windowIndex, bool makeTooltip) {
	CenterAlign(ImGui::CalcTextSize(title).x);
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, { 0.F, 0.F, 0.F, 0.F });
	if (ImGui::Selectable(title)) {
		ui.toggleOpenManually(windowIndex);
	}
	ImGui::PopStyleColor();
	if (makeTooltip) {
		AddTooltip("Click the field for tooltip.");
	}
}

// runs on the main thread
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

// runs on the main thread
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

// runs on the main thread
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

// runs on the main thread
const char* formatBlockType(BlockType blockType) {
	switch (blockType) {
		case BLOCK_TYPE_NORMAL: return "Normal";
		case BLOCK_TYPE_FAULTLESS: return "FD";
		case BLOCK_TYPE_INSTANT: return "IB";
		default: return "Unknown";
	}
}

// runs on the main thread
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

// runs on the main thread
int printChipDamageCalculation(int x, int baseDamage, int attackKezuri, int attackKezuriStandard) {
	ImGui::TableNextColumn();
	ImGui::TextUnformatted("Chip Damage Modif");
	AddTooltip("Chip damage modifier specifies how much of the base damage is applied as chip damage on block."
		" The standard value for supers and specials is 16 and that amounts to 12.5% and goes linearly up or down from there.");
	ImGui::TableNextColumn();
	const char* chipModifStr = ui.printDecimal(attackKezuri * 10000 / 128, 2, 0, true);
	sprintf_s(strbuf, "%d (%s)", attackKezuri, chipModifStr);
	
	ImGui::TextUnformatted(strbuf);
	
	if (attackKezuri != attackKezuriStandard) {
		
		const char* needHelp = nullptr;
		ImVec4* color = &RED_COLOR;
		if (attackKezuri > attackKezuriStandard) {
			needHelp = "higher";
		} else if (attackKezuri < attackKezuriStandard) {
			needHelp = "lower";
			color = &LIGHT_BLUE_COLOR;
		}
		
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

// runs on the main thread
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

// runs on the main thread
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

// runs on the main thread
const char* formatGuardType(GuardType guardType) {
	switch (guardType) {
		case GUARD_TYPE_ANY: return "Any";
		case GUARD_TYPE_HIGH: return "Overhead";
		case GUARD_TYPE_LOW: return "Low";
		case GUARD_TYPE_NONE: return "Unblockable";
		default: return "Unknown";
	}
}

// runs on the main thread
const char* UI::searchCollapsibleSection(const char* collapsibleHeaderName, const char* textEnd) {
	if (!searching) return collapsibleHeaderName;
	searchFieldTitle(collapsibleHeaderName, textEnd);
	pushSearchStack(collapsibleHeaderName);
	return collapsibleHeaderName;
}

// runs on the main thread
void UI::pushSearchStack(const char* name) {
	if (!searching) return;
	searchStack[searchStackCount++] = name;
}

// runs on the main thread
void UI::popSearchStack() {
	if (!searching) return;
	--searchStackCount;
}

// runs on the main thread
static void replaceNewLinesWithSpaces(std::string& str) {
	for (auto it = str.begin(); it != str.end(); ++it) {
		if (*it == '\n') {
			*it = ' ';
		}
	}
}

// runs on the main thread
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

// runs on the main thread
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

// runs on the main thread
const char* UI::rewindToNextUtf8CharStart(const char* ptr, const char* textStart) {
	while (ptr != textStart) {
		--ptr;
		if ((*ptr & 0b11000000) != 0b10000000) {
			return ptr;
		}
	}
	return ptr;
}

// runs on the main thread
const char* UI::skipToNextUtf8CharStart(const char* ptr) {
	while (true) {
		++ptr;
		if (*ptr == '\0') return ptr;
		if ((*ptr & 0b11000000) != 0b10000000) {
			return ptr;
		}
	}
}

// runs on the main thread
const char* UI::skipToNextUtf8CharStart(const char* ptr, const char* textEnd) {
	while (ptr != textEnd) {
		++ptr;
		if ((*ptr & 0b11000000) != 0b10000000) {
			return ptr;
		}
	}
	return ptr;
}

// runs on the main thread
const char* UI::searchFieldTitle(const char* fieldTitle, const char* textEnd) {
	if (!searching) return fieldTitle;
	searchField = fieldTitle;
	searchRawTextMultiResult(fieldTitle, textEnd);
	return fieldTitle;
}

// runs on the main thread
const char* UI::searchTooltip(const char* tooltip, const char* textEnd) {
	if (!searching) return tooltip;
	searchRawTextMultiResult(tooltip, textEnd);
	return tooltip;
}

// runs on the main thread
const char* UI::searchFieldValue(const char* value, const char* textEnd) {
	if (!searching) return value;
	searchRawTextMultiResult(value, textEnd);
	return value;
}

// runs on the main thread
void UI::searchWindow() {
	customBegin(PinnedWindowEnum_Search);
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
	customEnd();
}

// runs on the main thread
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

// runs on the main thread
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

// runs on the main thread
static const char* skippedFramesTypeToString(SkippedFramesType type) {
	switch (type) {
		case SKIPPED_FRAMES_SUPERFREEZE: return "superfreeze";
		case SKIPPED_FRAMES_HITSTOP: return "hitstop";
		case SKIPPED_FRAMES_GRAB: return "grab anim";
		case SKIPPED_FRAMES_SUPER: return "super";
		default: return "something";
	}
}

// runs on the main thread
void SkippedFramesInfo::print(bool canBlockButNotFD_ASSUMPTION) const {
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
	if (elements[count - 1].type == SKIPPED_FRAMES_SUPERFREEZE && canBlockButNotFD_ASSUMPTION) {
		ImGui::PushStyleColor(ImGuiCol_Text, SLIGHTLY_GRAY);
		ImGui::TextUnformatted("Note that cancelling a dash into FD or covering a jump with FD or using FD in general,"
			" including to avoid chip damage, is impossible on this frame, because it immediately follows a superfreeze."
			" Generally doing anything except throw or normal block/IB is impossible in such situations.");
		ImGui::PopStyleColor();
	}
}

// runs on the main thread
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

// runs on the main thread
void printExtraHitstunTooltip(int amount) {
	if (ImGui::BeginItemTooltip()) {
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		printExtraHitstunText(amount);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

// runs on the main thread
void printExtraHitstunText(int amount) {
	sprintf_s(strbuf, "The extra %d hitstun is applied from a floor bounce.", amount);
	ImGui::TextUnformatted(strbuf);
}

// runs on the main thread
void printExtraBlockstunTooltip(int amount) {
	if (ImGui::BeginItemTooltip()) {
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		printExtraBlockstunText(amount);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

// runs on the main thread
void printExtraBlockstunText(int amount) {
	sprintf_s(strbuf, "The extra %d blockstun is applied from landing while in blockstun.", amount);
	ImGui::TextUnformatted(strbuf);
}

// runs on the main thread
const char* comborepr(std::vector<int>& combo) {
	StringWithLength repr = settings.getComboRepresentationUserFriendly(combo);
	if (repr.txt[0] == '\0') return "<not set>";
	return repr.txt;
}

// runs on the main thread
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

// runs on the main thread
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

// runs on the main thread
void UI::resetFrameSelection() {
	selectingFrames = false;
	selectedFrameStart = -1;
	selectedFrameEnd = -1;
}

// runs on the main thread
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

// runs on the main thread
void UI::drawRightAlignedP1TitleWithCharIcon() {
	GGIcon scaledIcon = scaleGGIconToHeight(getPlayerCharIcon(0), 14.F);
	float w = ImGui::CalcTextSize("P1").x + getItemSpacing() + scaledIcon.size.x;
	RightAlign(w);
	drawPlayerIconWithTooltip(0);
	ImGui::SameLine();
	ImGui::TextUnformatted("P1");
}

// runs on the main thread
void UI::onFramebarReset() {
	onFramebarAdvanced();
	framebarAutoScroll = true;
}

// runs on the main thread
void UI::onFramebarAdvanced() {
	if (!selectingFrames && settings.clearFrameSelectionWhenFramebarAdvances) {
		resetFrameSelection();
	}
	framebarScrollX = 0.F;
	framebarAutoScroll = true;
}

// runs on the main thread
void UI::drawFramebars() {
	if (endScene.playerFramebars.size() != 2) return;
	const bool showFirstFrames = settings.showFirstFramesOnFramebar;
	const bool showStrikeInvulOnFramebar = settings.showStrikeInvulOnFramebar;
	const bool showSuperArmorOnFramebar = settings.showSuperArmorOnFramebar;
	const bool showThrowInvulOnFramebar = settings.showThrowInvulOnFramebar;
	const bool showOTGOnFramebar = settings.showOTGOnFramebar;
	ImGuiIO& io = ImGui::GetIO();
	ImVec2 displaySize = io.DisplaySize;
	float settingsPlayerFramebarHeight = std::roundf((float)settings.playerFramebarHeight * displaySize.y / 720.F);
	if (settingsPlayerFramebarHeight < 5.F - 0.001F) {
		settingsPlayerFramebarHeight = 5.F;
	}
	const float scale = settingsPlayerFramebarHeight / 19.F;
	float settingsProjectileFramebarHeight = std::roundf((float)settings.projectileFramebarHeight * displaySize.y / 720.F);
	if (settingsProjectileFramebarHeight < 3.F - 0.001F) {
		settingsProjectileFramebarHeight = 3.F;
	}
	
	static const float outerBorderThicknessUnscaled = 2.F;
	const float outerBorderThicknessScaledBeforeFloor = outerBorderThicknessUnscaled * scale;
	float outerBorderThickness = std::floorf(outerBorderThicknessScaledBeforeFloor + 0.001F);
	if (outerBorderThickness < 1.F - 0.001F) {
		if (outerBorderThicknessScaledBeforeFloor > 0.66F) {
			outerBorderThickness = 2.F;
		} else {
			outerBorderThickness = 1.F;
		}
	} else if (outerBorderThickness < 2.F - 0.001F) outerBorderThickness = 2.F;
	drawFramebars_frameItselfHeight = settingsPlayerFramebarHeight - outerBorderThickness - outerBorderThickness;
	if (drawFramebars_frameItselfHeight < 1.F) {
		drawFramebars_frameItselfHeight = 1.F;
	}
	drawFramebars_frameItselfHeightProjectile = settingsProjectileFramebarHeight - outerBorderThickness - outerBorderThickness;
	if (drawFramebars_frameItselfHeightProjectile < 1.F) {
		drawFramebars_frameItselfHeightProjectile = 1.F;
	}
	static const float frameNumberHeightOriginal = 11.F;
	static const float markerPaddingHeightUnscaled = -1.F;
	const float markerPaddingHeightScaledBeforeFloor = -markerPaddingHeightUnscaled * scale;
	float markerPaddingHeight = -std::floorf(markerPaddingHeightScaledBeforeFloor + 0.001F);
	if (markerPaddingHeight > -1.F && markerPaddingHeightScaledBeforeFloor < -0.3F) {
		markerPaddingHeight = -1.F;
	}
	float paddingBetweenPlayerFramebarsBaseUnscaled = (float)settings.distanceBetweenPlayerFramebars * 0.1F;
	const float paddingBetweenPlayerFramebarsBase = std::roundf(paddingBetweenPlayerFramebarsBaseUnscaled * scale);
	float paddingBetweenProjectileFramebarsBaseUnscaled = (float)settings.distanceBetweenProjectileFramebars * 0.1F;
	const float paddingBetweenProjectileFramebarsBase = std::roundf(paddingBetweenProjectileFramebarsBaseUnscaled * scale);
	static const float paddingBetweenTextAndFramebarUnscaled = 5.F;
	const float paddingBetweenTextAndFramebar = std::roundf(paddingBetweenTextAndFramebarUnscaled * scale);
	static const float textPaddingUnscaled = 2.F;
	const float textPadding = std::roundf(textPaddingUnscaled * scale);
	drawFramebars_frameWidthScaled = std::roundf(frameWidthOriginal * drawFramebars_frameItselfHeight / frameHeightOriginal);
	static const float highlighterWidthUnscaled = 2.F;
	const float highlighterWidth = highlighterWidthUnscaled * scale;
	static const float hoveredFrameHighlightPaddingXUnscaled = 3.F;
	const float hoveredFrameHighlightPaddingX = hoveredFrameHighlightPaddingXUnscaled * scale;
	static const float hoveredFrameHighlightPaddingYUnscaled = 3.F;
	const float hoveredFrameHighlightPaddingY = hoveredFrameHighlightPaddingYUnscaled * scale;
	static const float framebarCurrentPositionHighlighterStickoutDistanceUnscaled = 2.F;
	const float framebarCurrentPositionHighlighterStickoutDistance = std::roundf(framebarCurrentPositionHighlighterStickoutDistanceUnscaled * scale);
	static const float framedataBottomPadding = 0.F;
	drawFramebars_innerBorderThickness = std::floorf(innerBorderThicknessUnscaled * scale + 0.001F);
	if (drawFramebars_innerBorderThickness < 1.F) drawFramebars_innerBorderThickness = 1.F;
	drawFramebars_innerBorderThicknessHalf = drawFramebars_innerBorderThickness * 0.5F;
	
	drawFramebars_framesCount = framebarSettings.framesCount;
	
	const float framesCountFloat = (float)drawFramebars_framesCount;
	
	const float initialWindowWidthForFirstUseEver = 880.F / 1280.F * displaySize.x;
	float widthForFramesWidthCalculation;
	short existingWindowWidth = 0;
	short existingWindowHeight = 0;
	bool windowAlreadyExists = false;
	bool windowAlreadyHasSize = imGuiCorrecter.checkWindowHasSize("Framebar", &existingWindowWidth, &existingWindowHeight, &windowAlreadyExists);
	if (windowAlreadyHasSize) {
		widthForFramesWidthCalculation = (float)existingWindowWidth;
		if (windowAlreadyExists && framebarHadScrollbar) widthForFramesWidthCalculation -= 14.F;
	} else {
		widthForFramesWidthCalculation = initialWindowWidthForFirstUseEver;
	}
	
	const float framesX = drawFramebars_windowPos.x
		+ paddingBetweenTextAndFramebar
		+ outerBorderThickness;
	const float framesXEnd = drawFramebars_windowPos.x
		//+ ImGui::GetContentRegionMax().x  // can't get window size because window does not exist yet
		+ widthForFramesWidthCalculation - 1.F  // -- use alternative approach
		- outerBorderThickness
		+ drawFramebars_innerBorderThickness;
	
	if (framesXEnd > framesX) {
		const float totalVisibleFramesWidth = framesXEnd - framesX;
		const float totalVisibleFramesWidthWithoutInnerBorders = totalVisibleFramesWidth - framesCountFloat * drawFramebars_innerBorderThickness;
		const float singleFrameWidthUnroundedWithoutInnerBorder = totalVisibleFramesWidthWithoutInnerBorders / framesCountFloat;
		const float widthFloor = std::floorf(singleFrameWidthUnroundedWithoutInnerBorder + 0.001F);
		const float widthFraction = singleFrameWidthUnroundedWithoutInnerBorder - widthFloor;
		
		PackTextureSizes newSizes;
		if (widthFraction >= 0.5F) {
			newSizes.frameWidth = (int)widthFloor + 1;
			newSizes.everythingWiderByDefault = true;
		} else {
			newSizes.frameWidth = (int)widthFloor;
			newSizes.everythingWiderByDefault = false;
		}
		newSizes.frameHeight = (int)drawFramebars_frameItselfHeight;
		newSizes.frameHeightProjectile = (int)drawFramebars_frameItselfHeightProjectile;
		int digitThickness = settings.digitThickness;
		if (digitThickness < 1) digitThickness = 1;
		if (digitThickness > 2) digitThickness = 2;
		newSizes.digitThickness = digitThickness;
		newSizes.drawDigits = settings.drawDigits;
		
		packTextureFramebar(&newSizes, settings.useColorblindHelp);
	}
	
	const float powerupHeight = powerupFrameArt.framebar.size.y;
	const float firstFrameHeightScaled = firstFrameArt.framebar.size.y;
	const float firstFrameHeightOffset = std::ceilf(firstFrameHeightScaled * 0.5F - 0.001F);
	static const float frameMarkerSideHeightOriginal = 2.F;  // does not include outline
	const FrameMarkerArt* frameMarkerArtArray = settings.useColorblindHelp ? frameMarkerArtColorblind : frameMarkerArtNonColorblind;
	const FrameMarkerArt& strikeInvulMarker = frameMarkerArtArray[MARKER_TYPE_STRIKE_INVUL];
	const float frameMarkerHeight = strikeInvulMarker.framebar.size.y;
	const float markerWidthUse = strikeInvulMarker.framebar.size.x;
	float frameMarkerSideHeight = std::roundf(
		frameMarkerSideHeightOriginal * (
			frameMarkerHeight
			- 2.F  // exclude outline
		) / frameMarkerHeightOriginal  // does not include outline
	) + 1.F  // add outline, but only on one side
	+ 1.F;  // add 1px padding
	if (frameMarkerSideHeight < 1.F - 0.001F) frameMarkerSideHeight = 1.F;
	
	// Space reserved on top of a framebar for top markers and first frame indicator
	float maxTopPadding;
	// Space reserved on top of a framebar for just the first frame indicator
	float topPaddingFirstFrameOnly;
	if (!showFirstFrames) {
		topPaddingFirstFrameOnly = maxTopPadding = 0.F;
	} else {
		topPaddingFirstFrameOnly = maxTopPadding = std::floorf(firstFrameHeightOffset * 0.5F + 0.001F);
	}
	float topPaddingMarkerOnly;
	if (showStrikeInvulOnFramebar || showSuperArmorOnFramebar) {
		topPaddingMarkerOnly = -outerBorderThickness + markerPaddingHeight + frameMarkerHeight
			- 1.F;  // -1 to ignore the outline and drive framebars a little closer to each other
		if (topPaddingMarkerOnly > maxTopPadding) {
			maxTopPadding = topPaddingMarkerOnly;
		}
	} else {
		topPaddingMarkerOnly = 0.F;
	}
	// Space reserved under a framebar for bottom markers
	float bottomPadding = -outerBorderThickness + markerPaddingHeight + frameMarkerHeight
		 - 1.F;  // subtract 1 to remove the outline from the padding and to hug framebars together a little bit
	if (bottomPadding < 0.F || !showThrowInvulOnFramebar && !showOTGOnFramebar) {
		bottomPadding = 0.F;
	}
	
	// this struct gets copied around. Don't put huge data in it
	struct QueuedFramebar {
		EntityFramebar& framebar = *(EntityFramebar*)nullptr;
		float y = 0.F;  // points to the top of the frame texture
		float padding = 0.F;  // the padding that should be between this framebar and the previous framebar
		Moves::TriBool hasFirstFrames = Moves::TriBool::TRIBOOL_DUNNO;
		char hasDigit[_countof(Framebar::frames)] = { '\0' };  // specifies if a digit was drawn on this frame, and which digit. 0 means no digit was drawn. 1 means 0 was drawn, and so on
		bool condensed = false;
		float heightWithBorder;
		float height;
		bool useMini;
		float frameNumberYTop;
		float frameNumberYBottom;
	};
	
	struct {
		float paddingForPlayers;
		float paddingForProjectilesWithTopMarkerAndFirstFrame;
		float paddingForProjectilesWithTopMarkerOnly;
		float paddingForProjectilesWithFirstFrameOnly;
		float paddingForProjectilesWithoutAnything;
		bool condenseIntoOneProjectileFramebar;
		float onePlayerFramebarHeight;
		float oneProjectileFramebarHeight;
		
		void create(QueuedFramebar& result, EntityFramebar& entityFramebar, int playerIndex) {
			(EntityFramebar*&)result = &entityFramebar;
			if (entityFramebar.belongsToPlayer()) {
				result.useMini = drawFramebars_frameItselfHeight < drawFramebars_frameItselfHeightProjectile;
				result.heightWithBorder = onePlayerFramebarHeight;
				result.height = drawFramebars_frameItselfHeight;
				if (playerIndex == 0) {
					return;
				} else {
					result.padding = paddingForPlayers;
					return;
				}
			}
			
			result.useMini = drawFramebars_frameItselfHeightProjectile < drawFramebars_frameItselfHeight;
			result.heightWithBorder = oneProjectileFramebarHeight;
			result.height = drawFramebars_frameItselfHeightProjectile;
			
			if (playerIndex != 0 && playerIndex != 1) {
				result.padding = paddingForProjectilesWithFirstFrameOnly;
				return;
			}
			
			if (condenseIntoOneProjectileFramebar) {
				result.padding = 0.F;
				result.hasFirstFrames = Moves::TriBool::TRIBOOL_DUNNO;
				return;
			}
			
			const Framebar& projectileFramebar = (const Framebar&)(
				settings.neverIgnoreHitstop
					? entityFramebar.getHitstop()
					: entityFramebar.getMain()
			);
			
			CharacterType correspondingPlayerCharacterType = endScene.players[playerIndex].charType;
			bool hasTopMarker = false;
			bool hasFirstFrame = false;
			
			drawFirstFrames<Framebar, Frame>(projectileFramebar, nullptr, 0.F, 0.F, &hasFirstFrame);
			
			if (settings.showSuperArmorOnFramebar && correspondingPlayerCharacterType == CHARACTER_TYPE_DIZZY
					|| settings.showStrikeInvulOnFramebar && correspondingPlayerCharacterType == CHARACTER_TYPE_JACKO) {
				hasTopMarker = projectileFramebar.lastNFramesHaveMarker(drawFramebars_framebarPosition, drawFramebars_framesCount);
			}
			
			float padding;
			if (hasTopMarker && hasFirstFrame) {
				padding = paddingForProjectilesWithTopMarkerAndFirstFrame;
			} else if (hasTopMarker) {
				padding = paddingForProjectilesWithTopMarkerOnly;
			} else if (hasFirstFrame) {
				padding = paddingForProjectilesWithFirstFrameOnly;
			} else {
				padding = paddingForProjectilesWithoutAnything;
			}
			
			result.padding = padding;
			result.hasFirstFrames = hasFirstFrame ? Moves::TriBool::TRIBOOL_TRUE : Moves::TriBool::TRIBOOL_FALSE;
			
		}
	} queuedFramebarFactory;
	
	queuedFramebarFactory.paddingForPlayers = paddingBetweenPlayerFramebarsBase + (maxTopPadding + bottomPadding);
	queuedFramebarFactory.paddingForProjectilesWithTopMarkerAndFirstFrame = paddingBetweenProjectileFramebarsBase + maxTopPadding;
	queuedFramebarFactory.paddingForProjectilesWithTopMarkerOnly = paddingBetweenProjectileFramebarsBase + topPaddingMarkerOnly;
	queuedFramebarFactory.paddingForProjectilesWithFirstFrameOnly = paddingBetweenProjectileFramebarsBase + topPaddingFirstFrameOnly;
	queuedFramebarFactory.paddingForProjectilesWithoutAnything = paddingBetweenProjectileFramebarsBase;
	const bool condenseIntoOneProjectileFramebar = framebarSettings.condenseIntoOneProjectileFramebar;
	queuedFramebarFactory.condenseIntoOneProjectileFramebar = condenseIntoOneProjectileFramebar;
	
	const float onePlayerFramebarHeight = outerBorderThickness
		+ drawFramebars_frameItselfHeight
		+ outerBorderThickness;
	
	const float oneProjectileFramebarHeight = outerBorderThickness
		+ drawFramebars_frameItselfHeightProjectile
		+ outerBorderThickness;
	
	queuedFramebarFactory.onePlayerFramebarHeight = onePlayerFramebarHeight;
	queuedFramebarFactory.oneProjectileFramebarHeight = oneProjectileFramebarHeight;
	
	const float onePlayerFramebarHeightWithMini = onePlayerFramebarHeight + drawFramebars_frameItselfHeightProjectile + outerBorderThickness;
	
	float framebarFrameDataHeight = 0.F;
	bool needShowFramebarFrameDataP1 = settings.showP1FramedataInFramebar;
	bool needShowFramebarFrameDataP2 = settings.showP2FramedataInFramebar;
	bool framebarFrameDataScaled = false;
	float framebarFrameDataScale;
	if (needShowFramebarFrameDataP1 || needShowFramebarFrameDataP2) {
		framebarFrameDataScale = settings.framedataInFramebarScale;
		if (framebarFrameDataScale < 0.001F) {
			if (displaySize.x > 1920.001F) {
				framebarFrameDataScale = 2.F;
			} else {
				framebarFrameDataScale = 1.F;
			}
		}
		// calling ImGui::SetWindowFontScale here causes the Debug window to appear
		// not even setting it back to 1.F makes it go away
		if (framebarFrameDataScale < 0.999F || framebarFrameDataScale > 1.001F) {
			framebarFrameDataScaled = true;
		}
		framebarFrameDataHeight = std::roundf(ImGui::GetTextLineHeight() * framebarFrameDataScale
			+ 3.F * (framebarFrameDataScale > 1.F ? framebarFrameDataScale : 1.F));  // for whatever reason it's missing ~3px for the tails of letters like y and g
			// also we're going to add a 1px outline all around the text, so that adds 2 more pixels
	}
	
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
	
	// this value is in [0;_countof(Framebar::frames)] coordinate space, its possible range of values is [0;_countof(Framebar::frames)-1]
	// It does not have horizontal scrolling applied to it
	const int framebarPosition = framebarSettings.neverIgnoreHitstop ? endScene.getFramebarPositionHitstop() : endScene.getFramebarPosition();
	
	drawFramebars_framebarPosition = framebarPosition - framebarSettings.scrollXInFrames;
	if (drawFramebars_framebarPosition < 0) {
		drawFramebars_framebarPosition += _countof(Framebar::frames);
	}
	
	int framebarTotalFramesUnlimited_withScroll;
	if (framebarTotalFramesUnlimited < framebarSettings.scrollXInFrames) {
		framebarTotalFramesUnlimited_withScroll = 0;
	} else {
		framebarTotalFramesUnlimited_withScroll = framebarTotalFramesUnlimited - framebarSettings.scrollXInFrames;
	}
	
	drawFramebars_framebarPositionDisplay = framebarTotalFramesUnlimited_withScroll == 0
		? 0
		: (framebarTotalFramesUnlimited_withScroll - 1) % drawFramebars_framesCount;
	
	bool recheckCompletelyEmpty = drawFramebars_framesCount != _countof(Framebar::frames);
	const bool eachProjectileOnSeparateFramebar = framebarSettings.eachProjectileOnSeparateFramebar;
	
	std::vector<bool> framebarsCompletelyEmpty;
	if (recheckCompletelyEmpty) {
		if (eachProjectileOnSeparateFramebar) {
			framebarsCompletelyEmpty.resize(endScene.projectileFramebars.size(), false);
			int index = 0;
			for (const ProjectileFramebar& entityFramebar : endScene.projectileFramebars) {
				const FramebarBase& framebar = framebarSettings.neverIgnoreHitstop ? entityFramebar.getHitstop() : entityFramebar.getMain();
				framebarsCompletelyEmpty[index++] = framebar.lastNFramesCompletelyEmpty(drawFramebars_framebarPosition, drawFramebars_framesCount);
			}
		} else {
			framebarsCompletelyEmpty.resize(endScene.combinedFramebars.size(), false);
			int index = 0;
			for (const CombinedProjectileFramebar& entityFramebar : endScene.combinedFramebars) {
				const FramebarBase& framebar = framebarSettings.neverIgnoreHitstop ? entityFramebar.getHitstop() : entityFramebar.getMain();
				framebarsCompletelyEmpty[index++] = framebar.lastNFramesCompletelyEmpty(drawFramebars_framebarPosition, drawFramebars_framesCount);
			}
		}
	}
	
	std::vector<QueuedFramebar> framebars;
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
		framebars.reserve(2 + nonEmptyCount);
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
		framebars.reserve(2 + nonEmptyCount);
	} else {
		framebars.reserve(2 + endScene.combinedFramebars.size());
	}
	for (PlayerFramebars& entityFramebar : endScene.playerFramebars) {
		framebars.emplace_back();
		queuedFramebarFactory.create(framebars.back(), (EntityFramebar&)entityFramebar, entityFramebar.playerIndex);
	}
	for (int i = 0; i < 2; ++i) {
		if (eachProjectileOnSeparateFramebar) {
			int index = 0;
			for (ProjectileFramebar& entityFramebar : endScene.projectileFramebars) {
				const Framebar& framebar = framebarSettings.neverIgnoreHitstop ? entityFramebar.hitstop : entityFramebar.main;
				if (entityFramebar.playerIndex == i && !(
						framebar.completelyEmpty || recheckCompletelyEmpty && framebarsCompletelyEmpty[index]
				)) {
					framebars.emplace_back();
					queuedFramebarFactory.create(framebars.back(), (EntityFramebar&)entityFramebar, i);
				}
				++index;
			}
		} else {
			int index = 0;
			for (const CombinedProjectileFramebar& entityFramebar : endScene.combinedFramebars) {
				if (entityFramebar.playerIndex == i && !(recheckCompletelyEmpty && framebarsCompletelyEmpty[index])) {
					framebars.emplace_back();
					queuedFramebarFactory.create(framebars.back(), (EntityFramebar&)entityFramebar, i);
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
				framebars.emplace_back();
				queuedFramebarFactory.create(framebars.back(), (EntityFramebar&)entityFramebar, -1);
			}
			++index;
		}
	} else {
		int index = 0;
		for (const CombinedProjectileFramebar& entityFramebar : endScene.combinedFramebars) {
			if (entityFramebar.playerIndex != 0 && entityFramebar.playerIndex != 1
					&& !(recheckCompletelyEmpty && framebarsCompletelyEmpty[index])) {
				framebars.emplace_back();
				queuedFramebarFactory.create(framebars.back(), (EntityFramebar&)entityFramebar, -1);
			}
			++index;
		}
	}
	
	QueuedFramebar* playersCondensedFramebarArray[2] { nullptr, nullptr };
	float framebarsPaddingYTotal = 0.F;
	for (QueuedFramebar& queuedFramebar : framebars) {
		framebarsPaddingYTotal += queuedFramebar.padding;
		queuedFramebar.condensed = condenseIntoOneProjectileFramebar && !queuedFramebar.framebar.belongsToPlayer();
		if (queuedFramebar.condensed && (queuedFramebar.framebar.playerIndex == 0 || queuedFramebar.framebar.playerIndex == 1)) {
			playersCondensedFramebarArray[queuedFramebar.framebar.playerIndex] = &queuedFramebar;
		}
	}
	ImVec2 nextWindowSize { initialWindowWidthForFirstUseEver,
		2.F  // imgui window padding
		+ 1.F
		+ maxTopPadding
		+ (
			condenseIntoOneProjectileFramebar
				? onePlayerFramebarHeightWithMini
				: onePlayerFramebarHeight
		) * 2.F
		+ (float)((int)needShowFramebarFrameDataP1 + (int)needShowFramebarFrameDataP2) * framebarFrameDataHeight
		+ queuedFramebarFactory.paddingForPlayers
		+ bottomPadding
		+ 1.F
		+ 2.F  // imgui window padding
	};
	float initialWindowPosXForFirstUseEver = std::floorf((displaySize.x - nextWindowSize.x) * 0.5F);
	ImGui::SetNextWindowPos({ initialWindowPosXForFirstUseEver, std::floorf(85.F * displaySize.y / 720.F)
		- 1.F  // when testing on my 1920x1080 monitor with the game set to windowed with same resolution, with the default scaling and position,
		// it skips the row of pixels that coincides with the bottom outline of the throw invulnerability marker, which makes it look weird and cut off.
		// Hopefully, -1 offset will resolve this
	}, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(nextWindowSize, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowContentSize({ 0.F,
		// don't add imgui window padding here
		+ 1.F
		+ maxTopPadding
		+ (
			condenseIntoOneProjectileFramebar
				? onePlayerFramebarHeightWithMini * 2.F + queuedFramebarFactory.paddingForPlayers
				: onePlayerFramebarHeight * 2.F + framebarsPaddingYTotal
					+ (
						framebars.size() > 2
							? oneProjectileFramebarHeight * (float)(framebars.size() - 2)
							: 0.F
					)
		)
		+ (float)(
			(int)needShowFramebarFrameDataP1 + (int)needShowFramebarFrameDataP2
		) * framebarFrameDataHeight
		+ (
			!condenseIntoOneProjectileFramebar
					&& needShowFramebarFrameDataP2
					&& framebars.size() > 2
				? framedataBottomPadding
				: 0.F
		)
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
	bool drawFullBorder = (
		ImGui::IsMouseDown(ImGuiMouseButton_Left)
		&& ImGui::IsWindowFocused() || gifMode.editHitboxes && ImGui::IsWindowHovered()
	) && ImGui::IsMouseHoveringRect({ 0.F, 0.F }, { FLT_MAX, FLT_MAX }, true);
	ImGui::PopStyleVar();
	bool scaledText = scale > 1.001F;
	if (scaledText) {
		ImGui::SetWindowFontScale(scale);
	} else {
		ImGui::SetWindowFontScale(1.F);
	}
	drawFramebars_windowPos = ImGui::GetWindowPos();
	const float windowWidth = ImGui::GetWindowWidth();
	const float windowHeight = ImGui::GetWindowHeight();
	
	drawFramebars_drawList = ImGui::GetWindowDrawList();
	
	framebarHadScrollbar = ImGui::GetScrollMaxY() > 0.001F;
	if (framebarHadScrollbar || drawFullBorder) {
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
	
	const float framesXMask = framesX - outerBorderThickness;
	const float framesXEndMask = framesXEnd - drawFramebars_innerBorderThickness + outerBorderThickness;
	
	const float scrollY = std::roundf(ImGui::GetScrollY());
	drawFramebars_y = drawFramebars_windowPos.y 
		+ 2.F  // imgui window padding
		+ 1.F
		+ maxTopPadding
		+ outerBorderThickness
		- scrollY
		+ (needShowFramebarFrameDataP1 ? framebarFrameDataHeight : 0.F)
		+ (condenseIntoOneProjectileFramebar ? outerBorderThickness + drawFramebars_frameItselfHeightProjectile : 0.F);
	
	{
		
		const float frameNumberHeight = settings.drawDigits ? digitUVs[0].framebar.size.y : 0.F;
		const float frameNumberHeightMini = settings.drawDigits ? digitUVsMini[0].framebar.size.y : 0.F;
		const float frameNumberHeightPlayer = drawFramebars_frameItselfHeight >= drawFramebars_frameItselfHeightProjectile
			? frameNumberHeight : frameNumberHeightMini;
		const float frameNumberHeightProjectile = drawFramebars_frameItselfHeightProjectile >= drawFramebars_frameItselfHeight
			? frameNumberHeight : frameNumberHeightMini;
		float frameNumberPaddingYUsePlayer = truncfTowardsZero(
			(drawFramebars_frameItselfHeight - frameNumberHeightPlayer) * 0.5F
		);
		float frameNumberPaddingYUseProjectile = truncfTowardsZero(
			(drawFramebars_frameItselfHeightProjectile - frameNumberHeightProjectile) * 0.5F
		);
		
		float currentY = drawFramebars_y;
		for (QueuedFramebar& queuedFramebar : framebars) {
			
			bool isPlayer = queuedFramebar.framebar.belongsToPlayer();
			int playerIndex = queuedFramebar.framebar.playerIndex;
			
			if (isPlayer || !isPlayer && !condenseIntoOneProjectileFramebar) {
				
				currentY += queuedFramebar.padding;  // first framebar has 0 padding
				
				queuedFramebar.y = currentY;
				
				currentY += queuedFramebar.heightWithBorder;
				if (isPlayer && playerIndex == 1) {
					currentY += bottomPadding;
				}
				
				if (needShowFramebarFrameDataP2
						&& isPlayer
						&& playerIndex == 1) {
					currentY += framebarFrameDataHeight + framedataBottomPadding;
				}
			} else {  // !isPlayer && condenseIntoOneProjectileFramebar
				if (playerIndex == 0) {
					queuedFramebar.y = framebars[0].y - outerBorderThickness - drawFramebars_frameItselfHeightProjectile;
				} else {
					queuedFramebar.y = framebars[1].y - outerBorderThickness + onePlayerFramebarHeight;
				}
			}
			
			if (isPlayer) {
				queuedFramebar.frameNumberYTop = queuedFramebar.y + frameNumberPaddingYUsePlayer;
				queuedFramebar.frameNumberYBottom = queuedFramebar.frameNumberYTop + frameNumberHeightPlayer;
			} else {
				queuedFramebar.frameNumberYTop = queuedFramebar.y + frameNumberPaddingYUseProjectile;
				queuedFramebar.frameNumberYBottom = queuedFramebar.frameNumberYTop + frameNumberHeightProjectile;
			}
			
		}
	}
	
	ImU32 tintDarker = ImGui::GetColorU32(IM_COL32(128, 128, 128, 255));
	ImU32 tintDarkerSemiTransparent = ImGui::GetColorU32(IM_COL32(128, 128, 128, 200));
	
	FrameDims preppedDims[_countof(Framebar::frames)];  // we're only going to use drawFramebars_framesCount of these
	float highlighterXStart[2] { 0.F, 0.F };
	int highlighterCount = 0;
	
	if (framesXEnd > framesX) {
		FrameDims* dims;
		float x = framesX;
		const float totalVisibleFramesWidth = framesXEnd - framesX;
		const float singleFrameWidthUnrounded = totalVisibleFramesWidth / framesCountFloat;
		const float lastPackedSizeWidthFloat = (float)lastPackedSize.frameWidth;
		
		for (int i = 0; i < drawFramebars_framesCount - 1; ++i) {
			
			float thisFrameXEnd = std::roundf(singleFrameWidthUnrounded * (float)(i + 1)) + framesX;
			float thisFrameWidth = thisFrameXEnd - x;
			float thisFrameWidthWithoutOutline = thisFrameWidth - drawFramebars_innerBorderThickness;
			
			if (thisFrameWidth < 1.F) {
				thisFrameWidth = 1.F;
				thisFrameWidthWithoutOutline = 1.F;
			}
			
			dims = preppedDims + i;
			dims->x = x;
			dims->width = thisFrameWidthWithoutOutline;
			dims->hitConnectedShouldBeAlt = fabsf(thisFrameWidthWithoutOutline - lastPackedSizeWidthFloat) > 0.001F;
			
			x = thisFrameXEnd;
		}
		
		dims = preppedDims + (drawFramebars_framesCount - 1);
		dims->x = x;
		x = framesXEnd;
		dims->width = x - drawFramebars_innerBorderThickness - dims->x;
		dims->hitConnectedShouldBeAlt = fabsf(dims->width - lastPackedSizeWidthFloat) > 0.001F;
		
		int highlighterPos = (
			drawFramebars_framebarPositionDisplay == drawFramebars_framesCount - 1
				? 0
				: drawFramebars_framebarPositionDisplay + 1
			);
		highlighterXStart[0] = preppedDims[highlighterPos].x - drawFramebars_innerBorderThickness;
		++highlighterCount;
		
		if (drawFramebars_framebarPositionDisplay == drawFramebars_framesCount - 1) {
			highlighterXStart[1] = framesXEnd - drawFramebars_innerBorderThickness;
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
		P1P2TextSizeWithSpace = std::roundf(ImGui::CalcTextSize("P1 ").x);
		P1P2TextSize = std::roundf(ImGui::CalcTextSize("P1").x);
	}
	
	drawFramebars_frameArtArray = settings.useColorblindHelp ? frameArtColorblind : frameArtNonColorblind;
	drawFramebars_frameArtArrayMini = settings.useColorblindHelp ? frameArtColorblindMini : frameArtNonColorblindMini;
	
	const FrameMarkerArt& throwInvulMarker = frameMarkerArtArray[MARKER_TYPE_THROW_INVUL];
	const FrameMarkerArt& OTGMarker = frameMarkerArtArray[MARKER_TYPE_OTG];
	
	drawFramebars_hoveredFrameIndex = -1;
	const float currentPositionHighlighter_Strip1_StartY = drawFramebars_y - outerBorderThickness;
	
	const bool showPlayerInFramebarTitle = settings.showPlayerInFramebarTitle;
	const int framebarTitleCharsMax = settings.framebarTitleCharsMax;
	const std::vector<SkippedFramesInfo>& skippedFrames = endScene.getSkippedFrames(framebarSettings.neverIgnoreHitstop);
	
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
		const float lineHeight = std::roundf(ImGui::GetTextLineHeightWithSpacing());
		const float textPaddingYPlayer = std::floorf((onePlayerFramebarHeight - lineHeight) * 0.5F);
		const float textPaddingYProjectile = std::floorf((oneProjectileFramebarHeight - lineHeight) * 0.5F);
		const bool dontTruncateFramebarTitles = settings.dontTruncateFramebarTitles;
		const bool allFramebarTitlesDisplayToTheLeft = settings.allFramebarTitlesDisplayToTheLeft;
		const bool useSlang = settings.useSlangNames;
		for (const QueuedFramebar& queuedFramebar : framebars) {
			const EntityFramebar* entityFramebarPtr = &queuedFramebar.framebar;
			const EntityFramebar& entityFramebar = *entityFramebarPtr;
			const Frame* frame = nullptr;  // I initialize this variable because of compiler warning C4703 'potentially uninitialized variable used' which can't be removed even with const bool check in the if
			
			const char* title = nullptr;
			const char* titleFull = nullptr;
			static std::string titleShortStr;
			
			const bool hasTitleInFrame = framebarTitleCharsMax > 0
				&& !entityFramebar.belongsToPlayer()
				&& !queuedFramebar.condensed;
			if (!hasTitleInFrame) {
				title = queuedFramebar.condensed ? "" : nullptr;
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
				if ((frame->charSpecific1 || frame->charSpecific2) && (
						entityFramebar.playerIndex == 0 || entityFramebar.playerIndex == 1
					) && endScene.players[entityFramebar.playerIndex].charType == CHARACTER_TYPE_RAMLETHAL
				) {
					titleFull = selectedTitle = frame->charSpecific1 ? "S Sword" : "H Sword";
				} else if (eachProjectileOnSeparateFramebar && frame->title.uncombined) {
					if (useSlang) {
						selectedTitle = frame->title.uncombined->slang;
					}
					if (!selectedTitle) selectedTitle = frame->title.uncombined->name;
					titleFull = frame->title.uncombined->name;
				}
				if (!selectedTitle && frame->title.text) {
					if (useSlang) {
						selectedTitle = frame->title.text->slang;
					}
					if (!selectedTitle) selectedTitle = frame->title.text->name;
				}
				if (!titleFull && useSlang && frame->title.text) {
					titleFull = frame->title.text->name;
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
				static const char* questionArray[2] = { "?", "??" };
				if (framebarTitleCharsMax <= 0) {
					title = "";
				} else if (framebarTitleCharsMax >= 3) {
					title = "???";
				} else {
					title = questionArray[framebarTitleCharsMax - 1];
				}
			}
			if (hasTitleInFrame
					&& frame->title.full
					&& *frame->title.full != '\0') {
				titleFull = frame->title.full;
			}
			float textX;
			bool isOnTheLeft = true;
			float textSizeX;
			if (entityFramebar.playerIndex == 0 || allFramebarTitlesDisplayToTheLeft) {
				if (!title) {
					textSizeX = 0.F;
				} else {
					textSizeX = std::roundf(ImGui::CalcTextSize(title).x);
				}
				textX = -textPadding;
			} else {
				textX = windowWidth + textPadding;
				isOnTheLeft = false;
			}
			
			const float textY = queuedFramebar.y + scrollY
				- outerBorderThickness
				+ (entityFramebar.belongsToPlayer() ? textPaddingYPlayer : textPaddingYProjectile)
				- drawFramebars_windowPos.y;
			
			bool hoveredExtra = false;
			
			bool drewTitle = false;
			if (showPlayerInFramebarTitle
					&& (entityFramebar.playerIndex == 0 || entityFramebar.playerIndex == 1)
					&& !queuedFramebar.condensed) {
				
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
										? textX - P1P2TextSizeWithSpace - textSizeX
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
						? textX - textSizeX
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
	
	for (QueuedFramebar& queuedFramebar : framebars) {
		EntityFramebar* entityFramebarPtr = &queuedFramebar.framebar;
		EntityFramebar& entityFramebar = *entityFramebarPtr;
		FramebarBase& framebar = framebarSettings.neverIgnoreHitstop ? entityFramebar.getHitstop() : entityFramebar.getMain();
		drawFramebars_y = queuedFramebar.y;
		const bool isPlayer = entityFramebar.belongsToPlayer();
		const float framebarHeight = queuedFramebar.heightWithBorder;
		const float frameHeight = queuedFramebar.height;
		
		if ((
					needShowFramebarFrameDataP1
					&& entityFramebar.playerIndex == 0
					|| needShowFramebarFrameDataP2
					&& entityFramebar.playerIndex == 1
				)
				&& isPlayer) {
			const PlayerFramebar& playerFramebar = (const PlayerFramebar&)framebar;
			const PlayerFrame& playerFrame = playerFramebar[drawFramebars_framebarPosition];
			sprintf_s(strbuf, "Startup: %d, Active: %d, Recovery: %d, Total: %d, Advantage: ",
				playerFrame.startup,
				playerFrame.active,
				playerFrame.recovery,
				playerFrame.total);
			if (framebarFrameDataScaled) {
				ImGui::SetWindowFontScale(framebarFrameDataScale);
			} else if (scaledText) {
				ImGui::SetWindowFontScale(1.F);
			}
			ImVec2 textPos;
			textPos.x = framesXMask;
			if (entityFramebar.playerIndex == 0) {
				if (condenseIntoOneProjectileFramebar) {
					textPos.y = drawFramebars_y
						- outerBorderThickness
						- drawFramebars_frameItselfHeightProjectile
						- outerBorderThickness
						- framebarFrameDataHeight
						- maxTopPadding;
				} else {
					textPos.y = drawFramebars_y
						- outerBorderThickness
						- framebarFrameDataHeight
						- maxTopPadding;
				}
			} else {
				if (condenseIntoOneProjectileFramebar) {
					textPos.y = drawFramebars_y
						//- outerBorderThickness
						+ onePlayerFramebarHeight
						+ drawFramebars_frameItselfHeightProjectile
						//+ outerBorderThickness
						+ bottomPadding;
				} else {
					textPos.y = drawFramebars_y
						- outerBorderThickness
						+ onePlayerFramebarHeight
						+ bottomPadding;
				}
			}
			outlinedTextRaw(drawFramebars_drawList, textPos, strbuf, nullptr, nullptr, true);
			textPos.x += std::roundf(ImGui::CalcTextSize(strbuf).x);
			
			short frameAdvantage;
			short landingFrameAdvantage;
			if (framebarSettings.scrollXInFrames == 0) {
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
					textPos.x += std::roundf(ImGui::CalcTextSize(strbuf).x);
					outlinedTextRaw(drawFramebars_drawList, textPos, "(", nullptr, nullptr, true);
					textPos.x += std::roundf(ImGui::CalcTextSize("(").x);
					frameAdvantageTextFormat(landingFrameAdvantage, strbuf, sizeof strbuf);
					if (landingFrameAdvantage > 0) {
						color = &GREEN_COLOR;
					} else if (landingFrameAdvantage < 0) {
						color = &RED_COLOR;
					} else {
						color = nullptr;
					}
					outlinedTextRaw(drawFramebars_drawList, textPos, strbuf, color, nullptr, true);
					textPos.x += std::roundf(ImGui::CalcTextSize(strbuf).x);
					outlinedTextRaw(drawFramebars_drawList, textPos, ")", nullptr, nullptr, true);
				}
			}
			if (scaledText) {
				ImGui::SetWindowFontScale(scale);
			} else if (framebarFrameDataScaled) {
				ImGui::SetWindowFontScale(1.F);
			}
		}
		
		if (framesXEndMask > framesXMask && framesXEnd > framesX) {
			drawFramebars_drawList->AddRectFilled(
				{ framesXMask, drawFramebars_y - outerBorderThickness },
				{ framesXEndMask, drawFramebars_y - outerBorderThickness + framebarHeight },
				ImGui::GetColorU32(IM_COL32(0, 0, 0, 255)));
		}
		
		if (framesXEnd > framesX) {
			
			const FrameArt* oldArt = nullptr;
			
			if (queuedFramebar.useMini
				&& !isPlayer  // some important info is shown in the player frame graphic, so cutting bottoms of frames off on player framebars is highly undesirable
			) {
				oldArt = drawFramebars_frameArtArray;
				drawFramebars_frameArtArray = drawFramebars_frameArtArrayMini;
			}
			
			HitConnectedArtSelector hitConnectedArtSelector;
			if (queuedFramebar.useMini) {
				hitConnectedArtSelector.hitConnected = &hitConnectedFrameArtMini;
				hitConnectedArtSelector.hitConnectedAlt = &hitConnectedFrameArtAltMini;
				hitConnectedArtSelector.hitConnectedBlack = &hitConnectedFrameBlackArtMini;
				hitConnectedArtSelector.hitConnectedBlackAlt = &hitConnectedFrameBlackArtAltMini;
			} else {
				hitConnectedArtSelector.hitConnected = &hitConnectedFrameArt;
				hitConnectedArtSelector.hitConnectedAlt = &hitConnectedFrameArtAlt;
				hitConnectedArtSelector.hitConnectedBlack = &hitConnectedFrameBlackArt;
				hitConnectedArtSelector.hitConnectedBlackAlt = &hitConnectedFrameBlackArtAlt;
			}
			
			if (isPlayer) {
				drawPlayerFramebar((PlayerFramebar&)framebar,
					preppedDims,
					tintDarker,
					entityFramebar.playerIndex,
					skippedFrames,
					entityFramebar.playerIndex == 0 || entityFramebar.playerIndex == 1
						? endScene.players[entityFramebar.playerIndex].charType
						: (CharacterType)-1,
					frameHeight,
					queuedFramebar.useMini ? newHitFrameArtMini : newHitFrameArt,
					hitConnectedArtSelector);
			} else {
				const PlayerFramebars& correspondingPlayersFramebars = endScene.playerFramebars[entityFramebar.playerIndex];
				
				drawProjectileFramebar((Framebar&)framebar,
					preppedDims,
					tintDarker,
					skippedFrames,
					framebarSettings.neverIgnoreHitstop
						? (const PlayerFramebar&)correspondingPlayersFramebars.getHitstop()
						: (const PlayerFramebar&)correspondingPlayersFramebars.getMain(),
					entityFramebar.playerIndex == 0 || entityFramebar.playerIndex == 1
						? endScene.players[entityFramebar.playerIndex].charType
						: (CharacterType)-1,
					frameHeight,
					queuedFramebar.useMini ? newHitFrameArtMini : newHitFrameArt,
					hitConnectedArtSelector);
			}
			
			if (oldArt) {
				drawFramebars_frameArtArray = oldArt;
			}
			
			if (settings.drawDigits) {
				
				if (isPlayer) {
					drawDigits<PlayerFramebar, PlayerFrame>(
						(const PlayerFramebar&)framebar, preppedDims,
						queuedFramebar.frameNumberYTop, queuedFramebar.frameNumberYBottom,
						queuedFramebar.hasDigit,
						queuedFramebar.useMini ? digitUVsMini : digitUVs);
				} else {
					drawDigits<Framebar, Frame>(
						(const Framebar&)framebar, preppedDims,
						queuedFramebar.frameNumberYTop, queuedFramebar.frameNumberYBottom,
						queuedFramebar.hasDigit,
						queuedFramebar.useMini ? digitUVsMini : digitUVs);
				}
			}
		}
	}
	
	static const ImU32 digitTints[2] = {
		ImGui::GetColorU32(IM_COL32(255, 255, 255, 150)),
		ImGui::GetColorU32(IM_COL32(128, 128, 128, 150))
	};
	
	for (QueuedFramebar& queuedFramebar : framebars) {
		const EntityFramebar* entityFramebarPtr = &queuedFramebar.framebar;
		const EntityFramebar& entityFramebar = *entityFramebarPtr;
		const FramebarBase& framebar = framebarSettings.neverIgnoreHitstop ? entityFramebar.getHitstop() : entityFramebar.getMain();
		drawFramebars_y = queuedFramebar.y;
		const bool isPlayer = entityFramebar.belongsToPlayer();
		const QueuedFramebar* playersCondensedFramebar = isPlayer ? playersCondensedFramebarArray[entityFramebar.playerIndex] : nullptr;
		const float frameHeight = queuedFramebar.height;
		
		const bool isJacko = (entityFramebar.playerIndex == 0 || entityFramebar.playerIndex == 1)
											&& endScene.players[entityFramebar.playerIndex].charType == CHARACTER_TYPE_JACKO;
		const bool isDizzy = (entityFramebar.playerIndex == 0 || entityFramebar.playerIndex == 1)
											&& endScene.players[entityFramebar.playerIndex].charType == CHARACTER_TYPE_DIZZY;
		const bool isFlipped = queuedFramebar.condensed && entityFramebar.playerIndex == 1;
		
		if (framesXEnd > framesX) {
			if (showStrikeInvulOnFramebar || showSuperArmorOnFramebar || showThrowInvulOnFramebar || showOTGOnFramebar) {
				
				const float yTopRow = drawFramebars_y - markerPaddingHeight - frameMarkerHeight;
				const float markerEndY = yTopRow + frameMarkerHeight;
				const float yBottomRow = drawFramebars_y + frameHeight + markerPaddingHeight;
				const float markerBottomEndY = yBottomRow + frameMarkerHeight;
				const float powerupWidthUse = powerupFrameArt.framebar.size.x;
				
				{
					const PlayerFramebar& framebarPlayer = (const PlayerFramebar&)framebar;
					const Framebar& framebarProjectile = (const Framebar&)framebar;
					int internalIndNext = iterateVisualFramesFrom0_getInitialInternalInd();
					int internalInd;
					for (int visualInd = 0; visualInd < drawFramebars_framesCount; ++visualInd) {
						
						internalInd = internalIndNext;
						iterateVisualFrames_incrementInternalInd(internalIndNext);
						
						const PlayerFrame& playerFrame = framebarPlayer[internalInd];
						const Frame& projectileFrame = framebarProjectile[internalInd];
						const FrameDims& dims = preppedDims[visualInd];
						
						ImU32 tint = -1;
						ImU32 tintSemiTransparent = ImGui::GetColorU32(IM_COL32(255, 255, 255, 200));
						if (visualInd > drawFramebars_framebarPositionDisplay) {
							tint = tintDarker;
							tintSemiTransparent = tintDarkerSemiTransparent;
						}
						
						ImVec2 markerStart { dims.x - 1.F, yTopRow };
						ImVec2 markerEnd { markerStart.x + markerWidthUse, markerEndY };
						ImVec2 bottomMarkerStart { markerStart.x, yBottomRow };
						ImVec2 bottomMarkerEnd { markerEnd.x, markerBottomEndY };
						unsigned char drewTopMarker = 0;
						unsigned char drewBottomMarker = 0;
						const bool hasDigit = settings.drawDigits && queuedFramebar.hasDigit[visualInd];
						#define selectTintTop (hasDigit && drewTopMarker >= 1 ? tintSemiTransparent : tint)
						#define selectTintBottom (hasDigit && drewBottomMarker >= 1 ? tintSemiTransparent : tint)
						
						if (
								(
									isPlayer
										? playerFrame.strikeInvulInGeneral
										: projectileFrame.marker
											&& isJacko
								) && showStrikeInvulOnFramebar
						) {
							drawFramebars_drawList->AddImage(TEXID_FRAMES_FRAMEBAR,
								isFlipped ? bottomMarkerEnd : markerStart,
								isFlipped ? bottomMarkerStart : markerEnd,
								strikeInvulMarker.framebar.start,
								strikeInvulMarker.framebar.end,
								isFlipped ? selectTintBottom : selectTintTop);
							
							if (isFlipped) {
								bottomMarkerStart.y -= frameMarkerSideHeight;
								bottomMarkerEnd.y -= frameMarkerSideHeight;
								++drewBottomMarker;
							} else {
								markerStart.y += frameMarkerSideHeight;
								markerEnd.y += frameMarkerSideHeight;
								++drewTopMarker;
							}
						}
						if (isPlayer) {
							if (playerFrame.superArmorActiveInGeneral && showSuperArmorOnFramebar) {
								const FrameMarkerArt& markerArt = frameMarkerArtArray[
										playerFrame.superArmorActiveInGeneral_IsFull
											? MARKER_TYPE_SUPER_ARMOR_FULL
											: MARKER_TYPE_SUPER_ARMOR
								];
								drawFramebars_drawList->AddImage(TEXID_FRAMES_FRAMEBAR,
									markerStart,
									markerEnd,
									markerArt.framebar.start,
									markerArt.framebar.end,
									selectTintTop);
								++drewTopMarker;
							}
							
							if (playerFrame.throwInvulInGeneral && showThrowInvulOnFramebar) {
								
								drawFramebars_drawList->AddImage(TEXID_FRAMES_FRAMEBAR,
									bottomMarkerStart,
									bottomMarkerEnd,
									throwInvulMarker.framebar.start,
									throwInvulMarker.framebar.end,
									selectTintBottom);
								
								bottomMarkerStart.y -= frameMarkerSideHeight;
								bottomMarkerEnd.y -= frameMarkerSideHeight;
								++drewBottomMarker;
							}
							
							if (playerFrame.OTGInGeneral && showOTGOnFramebar) {
								
								drawFramebars_drawList->AddImage(TEXID_FRAMES_FRAMEBAR,
									bottomMarkerStart,
									bottomMarkerEnd,
									OTGMarker.framebar.start,
									OTGMarker.framebar.end,
									selectTintBottom);
								++drewBottomMarker;
							}
						} else if (projectileFrame.marker
								&& isDizzy
								&& showSuperArmorOnFramebar) {
							const FrameMarkerArt& markerArt = frameMarkerArtArray[MARKER_TYPE_SUPER_ARMOR];
							drawFramebars_drawList->AddImage(TEXID_FRAMES_FRAMEBAR,
								isFlipped ? bottomMarkerEnd : markerStart,
								isFlipped ? bottomMarkerStart : markerEnd,
								markerArt.framebar.start,
								markerArt.framebar.end,
								isFlipped ? selectTintBottom : selectTintTop);
							
							if (isFlipped) ++drewBottomMarker;
							else ++drewTopMarker;
						}
						
						if (isPlayer ? playerFrame.powerup && !playerFrame.dontShowPowerupGraphic : projectileFrame.powerup) {
							float powerupWidthOffset = std::floorf((dims.width - powerupWidthUse) * 0.5F + 0.001F);
							drawFramebars_drawList->AddImage(TEXID_FRAMES_FRAMEBAR,
								{
										dims.x + powerupWidthOffset,
										isFlipped
											? drewBottomMarker
												? yBottomRow - frameMarkerSideHeight
												: yBottomRow
											: isPlayer && playerFrame.superArmorActiveInGeneral
												? yTopRow + frameMarkerSideHeight
												: yTopRow
								},
								{
									dims.x + powerupWidthOffset + powerupWidthUse,
									isFlipped
										? drewBottomMarker
											? yBottomRow + powerupHeight - frameMarkerSideHeight
											: yBottomRow + powerupHeight
										: isPlayer && playerFrame.superArmorActiveInGeneral
											? yTopRow + powerupHeight + frameMarkerSideHeight
											: yTopRow + powerupHeight
								},
								powerupFrameArt.framebar.start,
								powerupFrameArt.framebar.end,
								tint);
						}
						
						if (hasDigit && (drewTopMarker >= 2 || drewBottomMarker >= 2)
								&& !(
									(
										isPlayer
											? frameIsRed(playerFrame.type)
											: frameIsRed(projectileFrame.type)
									) && drewTopMarker >= 2
								)) {
							drawDigit(queuedFramebar.hasDigit[visualInd] - 1, dims,
								queuedFramebar.frameNumberYTop, queuedFramebar.frameNumberYBottom,
								digitTints[visualInd > drawFramebars_framebarPositionDisplay],
								queuedFramebar.useMini ? digitUVsMini : digitUVs);
						}
						
						if (settings.drawDigits && playersCondensedFramebar && playersCondensedFramebar->hasDigit[visualInd]
								&& (entityFramebar.playerIndex == 0 && drewTopMarker
									|| entityFramebar.playerIndex == 1 && drewBottomMarker)) {
							drawDigit(playersCondensedFramebar->hasDigit[visualInd] - 1, dims,
								playersCondensedFramebar->frameNumberYTop, playersCondensedFramebar->frameNumberYBottom,
								digitTints[visualInd > drawFramebars_framebarPositionDisplay],
								playersCondensedFramebar->useMini ? digitUVsMini : digitUVs);
						}
						
						#undef selectTintTop
						#undef selectTintBottom
						
					}
				}
			}
		}
	}
	
	
	for (QueuedFramebar& queuedFramebar : framebars) {
		const EntityFramebar* entityFramebarPtr = &queuedFramebar.framebar;
		const EntityFramebar& entityFramebar = *entityFramebarPtr;
		const FramebarBase& framebar = framebarSettings.neverIgnoreHitstop ? entityFramebar.getHitstop() : entityFramebar.getMain();
		drawFramebars_y = queuedFramebar.y;
			
		if (framesXEnd > framesX
				&& showFirstFrames
				&& queuedFramebar.hasFirstFrames != Moves::TriBool::TRIBOOL_FALSE) {
				
			const float firstFrameTopY = drawFramebars_y - firstFrameHeightOffset;
			const float firstFrameBottomY = firstFrameTopY + firstFrameHeightScaled;
			
			if (entityFramebar.belongsToPlayer()) {
				drawFirstFrames<PlayerFramebar, PlayerFrame>((const PlayerFramebar&)framebar, preppedDims, firstFrameTopY, firstFrameBottomY, nullptr);
			} else {
				drawFirstFrames<Framebar, Frame>((const Framebar&)framebar, preppedDims, firstFrameTopY, firstFrameBottomY, nullptr);
			}
		}
	}
	
	struct HighlighterStartEnd {
		float start;
		float end;
	};
	
	bool hasCondensed[2];
	if (condenseIntoOneProjectileFramebar) {
		for (int i = 0; i < 2; ++i) {
			hasCondensed[i] = false;
			for (const QueuedFramebar& queuedFramebar : framebars) {
				if (queuedFramebar.condensed && (
							queuedFramebar.framebar.playerIndex == 0
							|| queuedFramebar.framebar.playerIndex == 1
						)) {
					hasCondensed[queuedFramebar.framebar.playerIndex] = true;
				}
			}
		}
	}
	
	int highlighterStripCount = 1;
	HighlighterStartEnd highlighterYStartEnd[2];
	if (condenseIntoOneProjectileFramebar && hasCondensed[0]) {
		highlighterYStartEnd[0].start = currentPositionHighlighter_Strip1_StartY
			- drawFramebars_frameItselfHeightProjectile
			- outerBorderThickness
			- framebarCurrentPositionHighlighterStickoutDistance;
	} else {
		highlighterYStartEnd[0].start = currentPositionHighlighter_Strip1_StartY
			- framebarCurrentPositionHighlighterStickoutDistance;
	}
	if (condenseIntoOneProjectileFramebar) {
		if (hasCondensed[1]) {
			highlighterYStartEnd[0].end = framebars[1].y
				//- outerBorderThickness
				+ onePlayerFramebarHeight
				+ drawFramebars_frameItselfHeightProjectile
				//+ outerBorderThickness
				+ framebarCurrentPositionHighlighterStickoutDistance;
		} else {
			highlighterYStartEnd[0].end = framebars[1].y
				- outerBorderThickness
				+ onePlayerFramebarHeight
				+ framebarCurrentPositionHighlighterStickoutDistance;
		}
	} else {
		highlighterYStartEnd[0].end = framebars.back().y
			- outerBorderThickness
			+ framebars.back().heightWithBorder
			+ framebarCurrentPositionHighlighterStickoutDistance;
	}
	
	if (needShowFramebarFrameDataP2 && framebars.size() > 2 && !condenseIntoOneProjectileFramebar) {
		highlighterStripCount = 2;
		
		highlighterYStartEnd[0].end = framebars[1].y
			- outerBorderThickness
			+ onePlayerFramebarHeight
			+ framebarCurrentPositionHighlighterStickoutDistance;
		
		highlighterYStartEnd[1] = {
			framebars[2].y
				- outerBorderThickness
				- framebarCurrentPositionHighlighterStickoutDistance,
			framebars.back().y
				- outerBorderThickness
				+ oneProjectileFramebarHeight
				+ framebarCurrentPositionHighlighterStickoutDistance
		};
	}
	
	float windowViewableRegionStartY = drawFramebars_windowPos.y;
	float windowViewableRegionEndY = drawFramebars_windowPos.y + windowHeight;
	
	if (framesXEnd > framesX) {
		
		for (int i = 0; i < highlighterCount; ++i) {
			for (int j = 0; j < highlighterStripCount; ++j) {
				drawFramebars_drawList->AddRectFilled(
					{
						highlighterXStart[i],
						highlighterYStartEnd[j].start
					},
					{
						highlighterXStart[i] + highlighterWidth,
						highlighterYStartEnd[j].end
					},
					ImGui::GetColorU32(IM_COL32(255, 255, 255, 255)));
			}
		}
		
		bool needInitStitchParams = true;
		static const float distanceBetweenStitches = 4.F;
		static const float stitchSize = 3.F;
		static int visiblePreviousStitchesAtTheTopOfAStitch = -1;  // -1 means not calculated yet
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
			iterateVisualFrames_incrementInternalInd(internalIndNext);
			
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
				if (visiblePreviousStitchesAtTheTopOfAStitch == -1) {  // -1 means not calculated yet
					if (distanceBetweenStitches >= stitchSize) {
						visiblePreviousStitchesAtTheTopOfAStitch = 0;
					} else {
						visiblePreviousStitchesAtTheTopOfAStitch = (int)std::floor(stitchSize / distanceBetweenStitches);
					}
				}
				needInitStitchParams = false;
				for (int j = 0; j < highlighterStripCount; ++j) {
					StitchParams& params = stitchParams[j];
					params.startY = highlighterYStartEnd[j].start;
					float stitchEndYWithWindowClipping = highlighterYStartEnd[j].end;
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
				drawFramebars_hoveredFrameY + drawFramebars_hoveredFrameHeight + hoveredFrameHighlightPaddingY - 1.F
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
	
	if (selectedFrameStart != -1 && selectedFrameEnd != -1 && drawFramebars_framesCount > 0) {
		ImVec2 startPos;
		ImVec2 endPos;
		
		float selectionBoxStartY = highlighterYStartEnd[0].start + framebarCurrentPositionHighlighterStickoutDistance;
		float selectionBoxEndY =
			highlighterYStartEnd[highlighterStripCount - 1].end
			- framebarCurrentPositionHighlighterStickoutDistance;
		
		int selFrameStart;
		int selFrameEnd;
		if (selectedFrameStart <= selectedFrameEnd) {
			selFrameStart = selectedFrameStart;
			selFrameEnd = selectedFrameEnd;
		} else {
			selFrameEnd = selectedFrameStart;
			selFrameStart = selectedFrameEnd;
		}
		if (selFrameStart >= drawFramebars_framesCount) {
			selFrameStart = drawFramebars_framesCount - 1;
		}
		if (selFrameEnd >= drawFramebars_framesCount) {
			selFrameEnd = drawFramebars_framesCount - 1;
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
// runs on the main thread
void UI::adjustMousePosition() {
	imGuiCorrecter.adjustMousePosition();
}

// runs on the main thread
void UI::printLineOfResultOfHookingRankIcons(const char* placeName, bool result) {
	yellowText(placeName);
	ImGui::SameLine();
	ImGui::TextUnformatted(" - ");
	ImGui::SameLine();
	if (result) {
		textUnformattedColored(GREEN_COLOR, "SUCCESS;");
	} else {
		textUnformattedColored(RED_COLOR, "FAILED!");
	}
}

// runs on the main thread
void UI::prepareSecondaryFrameArts(UITextureType type) {
	
	FrameArt* arrays[2] = { frameArtColorblind, frameArtNonColorblind };
	FrameMarkerArt* arraysMarkers[2] = { frameMarkerArtColorblind, frameMarkerArtNonColorblind };
	for (int i = 0; i < 2; ++i) {
		FrameArt& art = arrays[i][FT_HITSTOP];
		art = arrays[i][FT_IDLE];
		art.type = FT_HITSTOP;
		art.description = "Hitstop: the time is stopped for this player due to a hit or blocking.";
	}
	for (int i = 0; i < 2; ++i) {
		FrameArt* theArray = arrays[i];
		
		static const StringWithLength projectileIsNotActive = "Projectile is not active.";
		
		FrameArt* ptr;
		ptr = &theArray[FT_IDLE_PROJECTILE];
		*ptr = arrays[i][FT_IDLE];
		ptr->description = projectileIsNotActive;
		
		ptr = &theArray[FT_IDLE_NO_DISPOSE];
		*ptr = arrays[i][FT_IDLE];
		ptr->description = projectileIsNotActive;
		
		ptr = &theArray[FT_ACTIVE_PROJECTILE];
		*ptr = arrays[i][FT_ACTIVE];
		ptr->description = "Projectile is active.";
		
		ptr = &theArray[FT_ACTIVE_NEW_HIT_PROJECTILE];
		*ptr = arrays[i][FT_ACTIVE_NEW_HIT];
		ptr->description = "Projectile is active, new (potential) hit has started:"
			" The black shadow on the left side of the frame denotes the start of a new (potential) hit."
			" The projectile may be capable of doing fewer hits than 1+the number of \"new hits\" displayed,"
			" and the first actual hit may occur on any active frame independent of \"new hit\" frames."
			" \"New hit\" frame merely means that the hit #2, #3 and so on can only happen after a \"new hit\" frame,"
			" and, between the first hit and the next \"new hit\", projectile is inactive (even though it is displayed as active)."
			" When the second hit happens the situation resets and you need another \"new hit\" frame to do an actual hit and so on.";
		
		ptr = &theArray[FT_ACTIVE_HITSTOP_PROJECTILE];
		*ptr = arrays[i][FT_ACTIVE_HITSTOP];
		ptr->description = "Projectile is active, and in hitstop: while the projectile is in hitstop, time doesn't advance for it and"
			" it doesn't hit enemies.";
		
		ptr = &theArray[FT_NON_ACTIVE_PROJECTILE];
		*ptr = arrays[i][FT_NON_ACTIVE];
		ptr->description = "A frame inbetweeen the active frames of a projectile.";
		
		ptr = &theArray[FT_STARTUP_ANYTIME_NOW];
		*ptr = arrays[i][FT_IDLE_ELPHELT_RIFLE];
		ptr->description = "Startup of a holdable move: can release the button any time to either attack or cancel the move."
			" Can't block or FD or perform normal attacks.";
		
		ptr = &theArray[FT_STARTUP_CAN_PROGRAM_SECRET_GARDEN];
		*ptr = arrays[i][FT_IDLE_ELPHELT_RIFLE];
		ptr->description = "Startup: can program Secret Garden."
			" Can't block or FD or perform normal attacks.";
		
		ptr = &theArray[FT_STARTUP_STANCE];
		*ptr = arrays[i][FT_IDLE_ELPHELT_RIFLE];
		ptr->description = "Being in some form of stance: can cancel into one or more specials. Can't block or FD or perform normal attacks.";
		
		ptr = &theArray[FT_IDLE_ELPHELT_RIFLE_READY];
		*ptr = arrays[i][FT_STARTUP_STANCE_CAN_STOP_HOLDING];
		ptr->description = "Idle: Can cancel the stance into specials or fire the rifle. Can't block or FD or perform normal attacks.";
		
		ptr = &theArray[FT_STARTUP_CAN_BLOCK_AND_CANCEL];
		*ptr = arrays[i][FT_STARTUP_STANCE_CAN_STOP_HOLDING];
		ptr->description = "Startup, but can block and cancel into certain moves:"
		" an attack's active frames have not yet started, or this is not an attack."
		" Can block and/or maybe FD, possibly can't switch block, and can cancel into certain moves.";
		
		ptr = &theArray[FT_RECOVERY_CAN_RELOAD];
		*ptr = arrays[i][FT_RECOVERY_HAS_GATLINGS];
		ptr->description = "Recovery, but can reload. Can't attack, block or FD.";
		
		ptr = &theArray[FT_STARTUP_ANYTIME_NOW_CAN_ACT];
		*ptr = arrays[i][FT_IDLE_CANT_BLOCK];
		ptr->description = "Startup of a holdable move: can release the button any time to either attack or cancel the move,"
			" and can also cancel into other moves."
			" Can't block or FD or perform normal attacks.";
		
		ptr = &theArray[FT_IDLE_AIRBORNE_BUT_CAN_GROUND_BLOCK];
		*ptr = arrays[i][FT_IDLE_CANT_FD];
		ptr->description = "Idle while airborne on the pre-landing (last airborne) frame:"
			" can block grounded on this frame and regular (without FD) block ground attacks that require FD to be blocked in the air."
			" Can attack, block and FD.";
		
		ptr = &theArray[FT_EDDIE_STARTUP];
		*ptr = arrays[i][FT_STARTUP];
		ptr->description = "Eddie's attack is in startup.";
		
		ptr = &theArray[FT_EDDIE_ACTIVE];
		*ptr = arrays[i][FT_ACTIVE];
		ptr->description = "Eddie's attack is active.";
		
		ptr = &theArray[FT_EDDIE_ACTIVE_HITSTOP];
		*ptr = arrays[i][FT_ACTIVE_HITSTOP];
		ptr->description = "Eddie's attack is active, but Eddie is in hitstop:"
			" during it, his attack can't hurt anybody and Eddie doesn't move.";
		
		ptr = &theArray[FT_EDDIE_ACTIVE_NEW_HIT];
		*ptr = arrays[i][FT_ACTIVE_NEW_HIT];
		ptr->description = "Eddie's attack is active, and a new (potential) hit starts on this frame."
			" The black shadow on the left side of the frame denotes the start of a new (potential) hit."
			" Eddie may be capable of doing fewer hits than 1+the number of \"new hits\" displayed,"
			" and the first actual hit may occur on any active frame independent of \"new hit\" frames."
			" \"New hit\" frame merely means that the hit #2, #3 and so on can only happen after a \"new hit\" frame,"
			" and, between the first hit and the next \"new hit\", Eddie is inactive (even though he's displayed as active)."
			" When the second hit happens the situation resets and you need another \"new hit\" frame to do an actual hit and so on.";
		
		ptr = &theArray[FT_EDDIE_RECOVERY];
		*ptr = arrays[i][FT_RECOVERY];
		ptr->description = "Eddie's attack is in recovery.";
		
		ptr = &theArray[FT_IDLE_PROJECTILE_HITTABLE];
		*ptr = arrays[i][FT_EDDIE_IDLE];
		ptr->description = "Projectile is not active and can be hit by attacks.";
		
		// sorry, please prepare frameArtColorblind, frameArtNonColorblind UVs yourself, before calling this function
		
	}
	// sorry, please prepare frameMarkerArtColorblind, frameMarkerArtNonColorblind UVs yourself
	// sorry, please prepare digitUVs yourself
	
}

// runs on the main thread
static void assignFromResourceHelper(UVStartEnd& se, const PngResource& res) {
	se.width = res.width;
	se.height = res.height;
	se.size = { (float)se.width, (float)se.height };
	se.start = { res.uStart, res.vStart };
	se.end = { res.uEnd, res.vEnd };
}

// runs on the main thread
void UI::packTexture(PngResource& packedTexture, UITextureType type, const PackTextureSizes* sizes) {
	
	#define selectSE(thing) (type == UITextureType::UITEX_HELP ? thing.help : thing.framebar)
	#define assignFromResource(thing, res) assignFromResourceHelper(selectSE(thing), res)
	
	using PixelA = PngResource::PixelA;
	TexturePacker texturePacker;
	const int originalFrameWidthInt = 9;
	const int originalFrameHeightInt = 15;
	const float originalFrameWidth = (float)originalFrameWidthInt;
	const float originalFrameHeight = (float)originalFrameHeightInt;
	const float sizesFrameWidthFloat = (float)sizes->frameWidth;
	int maxHeight = max(sizes->frameHeight, sizes->frameHeightProjectile);
	int minHeight = min(sizes->frameHeight, sizes->frameHeightProjectile);
	const float sizesFrameHeightMaxFloat = (float)maxHeight;
	const float sizesFrameHeightMinFloat = (float)minHeight;
	const float frameScaleHoriz = sizesFrameWidthFloat / originalFrameWidth;
	const float frameScaleVert = sizesFrameHeightMaxFloat / originalFrameHeight;
	const int thicknessIndex = sizes->digitThickness == 2 ? 0 : 1;
	const int miniCount = (sizes->frameHeight != sizes->frameHeightProjectile && type == UITextureType::UITEX_FRAMEBAR ? 2 : 1);
	
	int colorsCount = 0;
	FrameArt* frameArtArrays[2] { nullptr };
	FrameArt* frameArtArrayMini = nullptr;
	FrameMarkerArt* frameMarkerArtArrays[2] { nullptr };
	
	if (type == UITextureType::UITEX_HELP) {
		colorsCount = 2;
		frameArtArrays[0] = frameArtColorblind;
		frameArtArrays[1] = frameArtNonColorblind;
		frameMarkerArtArrays[0] = frameMarkerArtColorblind;
		frameMarkerArtArrays[1] = frameMarkerArtNonColorblind;
	} else {
		colorsCount = 1;
		
		if (settings.useColorblindHelp) {
			frameArtArrays[0] = frameArtColorblind;
			frameMarkerArtArrays[0] = frameMarkerArtColorblind;
		} else {
			frameArtArrays[0] = frameArtNonColorblind;
			frameMarkerArtArrays[0] = frameMarkerArtNonColorblind;
		}
		if (miniCount == 2) {
			frameArtArrayMini = settings.useColorblindHelp ? frameArtColorblindMini : frameArtNonColorblindMini;
		}
	}
	
	PngResource* uniqueFrameArts[FT_LAST * 2] { nullptr };
	int uniqueFrameArtsCount = 0;
	{
		
		for (int i = 0; i < colorsCount; ++i) {
			for (int j = 1; j < FT_LAST; ++j) {
				if (type == UITextureType::UITEX_FRAMEBAR && isNewHitType((FrameType)j)) continue;
				FrameArt& art = frameArtArrays[i][j];
				bool found = false;
				for (int k = 0; k < uniqueFrameArtsCount; ++k) {
					if (uniqueFrameArts[k] == art.resource) {
						found = true;
						break;
					}
				}
				if (!found) {
					uniqueFrameArts[uniqueFrameArtsCount++] = art.resource;
					texturePacker.addImage(*art.resource);
				}
			}
		}
	}
	
	struct UsedMarker {
		FrameMarkerArt markerArt;
		PngResource resource;
	};
	UsedMarker uniqueOutlinedScaledMarkers[2 * MARKER_TYPE_LAST] { };
	int uniqueOutlinedScaledMarkersCount = 0;
	{
		const int desiredMarkerHeightWhenUnscaledInt = 4;
		const float desiredMarkerHeightWhenUnscaled = (float)desiredMarkerHeightWhenUnscaledInt;
		int markerHeight = (int)std::roundf(desiredMarkerHeightWhenUnscaled * frameScaleVert);
		if (markerHeight < 1) markerHeight = 1;
		int markerHeightAccordingToWidth = (int)std::roundf(desiredMarkerHeightWhenUnscaled * frameScaleHoriz);
		if (markerHeight > markerHeightAccordingToWidth) markerHeight = markerHeightAccordingToWidth;
		int markerWidth = sizes->frameWidth;
		if (markerWidth < 1) markerWidth = 1;
		
		for (int i = 0; i < colorsCount; ++i) {
			for (int j = 0; j < MARKER_TYPE_LAST; ++j) {
				const FrameMarkerArt& art = frameMarkerArtArrays[i][j];
				const PngResource& sourceMarker = *art.resource;
				
				bool found = false;
				for (int k = 0; k < uniqueOutlinedScaledMarkersCount; ++k) {
					UsedMarker& usedMarker = uniqueOutlinedScaledMarkers[k];
					if (usedMarker.markerArt.equal(art)) {
						found = true;
						break;
					}
				}
				if (found) continue;
				
				UsedMarker& newUsedMarker = uniqueOutlinedScaledMarkers[uniqueOutlinedScaledMarkersCount++];
				newUsedMarker.markerArt = art;
				
				PngResource scaledMarker;
				scaledMarker.resize(markerWidth, markerHeight);
				sourceMarker.stretchRect(scaledMarker,
					0, 0,
					sourceMarker.width, sourceMarker.height,
					0, 0,
					scaledMarker.width, scaledMarker.height);
				
				PngResource& outlinedScaledMarker = newUsedMarker.resource;
				outlinedScaledMarker.resize(2 + scaledMarker.width, 2 + scaledMarker.height);
				
				for (int offX = 0; offX < 3; ++offX) {
					for (int offY = 0; offY < 3; ++offY) {
						if (offX == 1 && offY == 1) continue;
						scaledMarker.drawTintWithMaxAlpha(outlinedScaledMarker,
							0, 0,
							offX, offY,
							scaledMarker.width, scaledMarker.height,
							art.outlineColor);
					}
				}
				//outlinedScaledMarker.multiplyAlphaByPercent(0, 0, outlinedScaledMarker.width, outlinedScaledMarker.height, 130);
				scaledMarker.draw(outlinedScaledMarker,
					0, 0,
					1, 1,
					scaledMarker.width, scaledMarker.height);
				
				if (art.hasMiddleLine) {
					int middle = outlinedScaledMarker.width / 2;
					for (size_t row = 0; row < outlinedScaledMarker.height; ++row) {
						DWORD* pixel = (DWORD*)outlinedScaledMarker.getPixel(middle, row);
						*pixel = *pixel & 0xff000000 | art.outlineColor & 0x00FFFFFF;
					}
				}
				
				texturePacker.addImage(outlinedScaledMarker);
			}
		}
	}
	
	
	PngResource firstFrame;
	{
		const float originalFirstFrameWidth = 1.F;
		const int originalFirstFrameHeightInt = 5;
		const float originalFirstFrameHeight = (float)originalFirstFrameHeightInt;
		
		float firstFrameWidthFloat = frameScaleHoriz * originalFirstFrameWidth;
		float fraction = firstFrameWidthFloat - std::truncf(firstFrameWidthFloat);
		int firstFrameWidth = (int)firstFrameWidthFloat;
		if (fraction > 0.66F) {
			++firstFrameWidth;
		}
		if (firstFrameWidth < 1) firstFrameWidth = 1;
		
		int firstFrameHeight = (int)std::roundf(frameScaleVert * originalFirstFrameHeight);
		if (firstFrameHeight < 1) firstFrameHeight = 1;
		if (firstFrameHeight > originalFirstFrameHeightInt) {
			firstFrameHeight -= (int)std::ceilf((frameScaleVert * originalFirstFrameHeight - originalFirstFrameHeight) * 0.5F);
		}
		
		firstFrame.resize(1 + firstFrameWidth + 1, 1 + firstFrameHeight + 1);
		
		PixelA* ptr = firstFrame.getPixel(0, 0);
		for (size_t column = 0; column < firstFrame.width; ++column) {
			*(DWORD*)ptr = 0xFF000000;
			++ptr;
		}
		
		for (int row = 0; row < firstFrameHeight; ++row) {
			*(DWORD*)ptr = 0xFF000000;
			++ptr;
			for (int column = 0; column < firstFrameWidth; ++column) {
				*(DWORD*)ptr = 0xFFFFFFFF;
				++ptr;
			}
			*(DWORD*)ptr = 0xFF000000;
			++ptr;
		}
		
		for (size_t column = 0; column < firstFrame.width; ++column) {
			*(DWORD*)ptr = 0xFF000000;
			++ptr;
		}
		texturePacker.addImage(firstFrame);
	}
	
	PngResource hitConnectedFrameAr[2] { };
	PngResource hitConnectedFrameArMini[2] { };
	PngResource hitConnectedBlackFrameAr[2] { };
	PngResource hitConnectedBlackFrameArMini[2] { };
	{
		for (int isMiniIter = 0; isMiniIter < miniCount; ++isMiniIter) {
			const int hitConnectedWidthsCount = type == UITextureType::UITEX_HELP ? 1 : 2;
			int frameHeightToUse = isMiniIter == 0
				? maxHeight
				: minHeight;
				
			for (int i = 0; i < hitConnectedWidthsCount; ++i) {
				int w = sizes->frameWidth;
				if (i == 1) {
					if (sizes->everythingWiderByDefault) {
						--w;
					} else {
						++w;
					}
				}
				if (w < 1) w = 1;
				
				PngResource& hitConnectedFrame = (isMiniIter == 0 ? hitConnectedFrameAr : hitConnectedFrameArMini)[i];
				hitConnectedFrame.resize(w, frameHeightToUse);
				if (frameHeightToUse) {
					hitConnectedFrame.drawRectOutline(
						0, 0,
						w, frameHeightToUse,
						0xFFFFFFFF);
				}
				texturePacker.addImage(hitConnectedFrame);
				
				PngResource& hitConnectedBlackFrame = (isMiniIter == 0 ? hitConnectedBlackFrameAr : hitConnectedBlackFrameArMini)[i];
				hitConnectedBlackFrame.resize(w, frameHeightToUse);
				if (frameHeightToUse) {
					hitConnectedBlackFrame.drawRectOutline(
						0, 0,
						w, frameHeightToUse,
						0xFF000000);
				}
				if (w > 2 && frameHeightToUse > 2) {
					hitConnectedBlackFrame.drawRectOutline(
						1, 1,
						w - 2, frameHeightToUse - 2,
						0xFFFFFFFF);
				}
				texturePacker.addImage(hitConnectedBlackFrame);
			}
		}
	}
	
	PngResource powerup;
	{
		const DWORD outlineColor = 0xffffd065;
		const DWORD fillColor = 0xffffffcc;
		int i, j, end;
		PixelA* ptr;
		
		float wF = std::roundf(frameScaleHoriz * 7.F);
		float hF = std::roundf(frameScaleVert * 7.F);
		int w = (int)wF;
		int h = (int)hF;
		if (w < 5) w = 5;
		if (h < 5) h = 5;
		int size = min(w, h);
		float sizeF = min(wF, hF);
		int thickness = (size - 2) / 5;
		if (thickness < 1) thickness = 1;
		
		if (thickness % 2 != size % 2) {
			if ((float)size > sizeF) {
				--size;
				if (size < 5) {
					thickness = 1;
					size = 5;
				}
			} else {
				++size;
			}
		}
		
		powerup.resize(size, size);
		memset(powerup.data.data(), 0, size * size * sizeof(PixelA));
		
		const int armLength = (size - 2) / 2;
		// if size is even, [1 + armLength] points to the start of the second half
		// if size if uneven, [1 + armLength] point to the center
		int thicknessHalf = thickness / 2;
		
		// the offset from the left border of the image to the outline of the thin part of the cross
		int horizOffsetToOutline = (1 + armLength) - thicknessHalf - 1;
		
		// the top row
		ptr = powerup.getPixel(horizOffsetToOutline, 0);
		end = thickness + 2;
		for (i = 0; i < end; ++i) {
			*(DWORD*)ptr = outlineColor;
			++ptr;
		}
		
		ptr = ptr - thickness - 2 + powerup.width;
		int heightToReachHorizontalOutline = armLength - thicknessHalf - 1;
		// the part between the top row and horizontal outline start
		end = heightToReachHorizontalOutline;
		for (i = 0; i < end; ++i) {
			PixelA* ptrRewindingPoint = ptr;
			*(DWORD*)ptr = outlineColor;
			++ptr;
			for (j = 0; j < thickness; ++j) {
				*(DWORD*)ptr = fillColor;
				++ptr;
			}
			*(DWORD*)ptr = outlineColor;
			++ptr;
			
			ptr = ptrRewindingPoint;
			powerup.incrementRow(ptr);
		}
		
		ptr -= horizOffsetToOutline;
		// offset from the left border of the image to the center fill
		int offsetToTheStartOfFillColorOfCenter = horizOffsetToOutline + 1;
		// the horizontal outline, until the fill-colored region of the middle
		end = offsetToTheStartOfFillColorOfCenter;
		for (i = 0; i < end; ++i) {
			*(DWORD*)ptr = outlineColor;
			++ptr;
		}
		
		// the fill-colored middle region
		for (i = 0; i < thickness; ++i) {
			*(DWORD*)ptr = fillColor;
			++ptr;
		}
		
		// the horizontal outline that continues on the right
		for (i = end + thickness; i < size; ++i) {
			*(DWORD*)ptr = outlineColor;
			++ptr;
		}
		
		end = size - 2;
		// the part of the horizontal piece that isn't its top and bottom rows
		for (i = 0; i < thickness; ++i) {
			*(DWORD*)ptr = outlineColor;
			++ptr;
			for (j = 0; j < end; ++j) {
				*(DWORD*)ptr = fillColor;
				++ptr;
			}
			*(DWORD*)ptr = outlineColor;
			++ptr;
		}
		
		// the bottom row of the horizontal piece, until the fill-colored middle
		end = offsetToTheStartOfFillColorOfCenter;
		for (i = 0; i < end; ++i) {
			*(DWORD*)ptr = outlineColor;
			++ptr;
		}
		
		// the fill color of the middle
		for (i = 0; i < thickness; ++i) {
			*(DWORD*)ptr = fillColor;
			++ptr;
		}
		
		// the outline of the bottom row of the horizontal piece that is continued on the right
		for (i = end + thickness; i < size; ++i) {
			*(DWORD*)ptr = outlineColor;
			++ptr;
		}
		
		ptr += horizOffsetToOutline;
		// the part from the bottom row of the horizontal piece to the bottom row of the entire image
		end = heightToReachHorizontalOutline;
		for (i = 0; i < end; ++i) {
			PixelA* ptrRewindingPoint = ptr;
			*(DWORD*)ptr = outlineColor;
			++ptr;
			for (j = 0; j < thickness; ++j) {
				*(DWORD*)ptr = fillColor;
				++ptr;
			}
			*(DWORD*)ptr = outlineColor;
			++ptr;
			
			ptr = ptrRewindingPoint;
			powerup.incrementRow(ptr);
		}
		
		// the bottom row
		end = thickness + 2;
		for (i = 0; i < end; ++i) {
			*(DWORD*)ptr = outlineColor;
			++ptr;
		}
		
		texturePacker.addImage(powerup);
	}
	
	PngResource newHit;
	PngResource newHitMini;
	{
		int stripeWidth = 2;
		if (sizes->frameWidth > originalFrameWidthInt) {
			stripeWidth = stripeWidth * sizes->frameWidth / originalFrameWidthInt;
		}
		for (int isMiniIter = 0; isMiniIter < miniCount; ++isMiniIter) {
			PngResource& newHitUse = (isMiniIter == 0 ? newHit : newHitMini);
			
			newHitUse.resize(stripeWidth + 1,
				isMiniIter == 0 ? maxHeight : minHeight);
			
			std::vector<DWORD> preppedGradient(newHitUse.width, 0);
			for (size_t i = 0; i < newHitUse.width; ++i) {
				preppedGradient[i] = (255 * (newHitUse.width - i) / (newHitUse.width + 1)) << 24;
			}
			
			PixelA* ptr = newHitUse.getPixel(0, 0);
			for (size_t i = 0; i < newHitUse.height; ++i) {
				if (i % 2 == 0) {
					for (int j = 0; j < stripeWidth; ++j) {
						*(DWORD*)ptr = 0xFF000000;
						++ptr;
					}
					*(DWORD*)ptr = preppedGradient.back();
					++ptr;
				} else {
					for (size_t j = 0; j < newHitUse.width; ++j) {
						*(DWORD*)ptr = preppedGradient[j];
						++ptr;
					}
				}
			}
			texturePacker.addImage(newHitUse);
		}
	}
	
	PngResource digits[10] { };
	PngResource digitsMini[10] { };
	
	if (type == UITextureType::UITEX_FRAMEBAR && sizes->drawDigits) {
		for (int isMiniIter = 0; isMiniIter < miniCount; ++isMiniIter) {
			const PngResource& firstDigit = *digitFrame[0].thickness[thicknessIndex];
			
			float frameHeightToUse;
			float scaleVertToUse;
			if (isMiniIter == 0) {
				frameHeightToUse = sizesFrameHeightMaxFloat;
				scaleVertToUse = frameScaleVert;
			} else {
				frameHeightToUse = sizesFrameHeightMinFloat;
				scaleVertToUse = frameHeightToUse / originalFrameHeight;
			}
			
			static const float originalDigitHeight = 11.F;
			int digitHeight;
			if (scaleVertToUse > 1.F) {
				digitHeight = (int)std::roundf(originalDigitHeight * scaleVertToUse + 0.25F);
				// commented out to allow text to upscale on 3K resolution: https://github.com/kkots/ggxrd_hitbox_overlay_2211/issues/6
				//if (digitHeight > (int)firstDigit.height) {
				//	digitHeight = (int)firstDigit.height;
				//}
			} else {
				if (frameHeightToUse < originalDigitHeight) {
					digitHeight = (int)frameHeightToUse;
				} else {
					digitHeight = (int)originalDigitHeight;
				}
			}
			if (digitHeight < 3) digitHeight = 3;
			int w = sizes->frameWidth;
			if (frameScaleHoriz >= 1.F) {
				w -= (int)std::ceilf(frameScaleHoriz - 1.F);
			}
			if (w > (int)drawFramebars_frameWidthScaled) w = (int)drawFramebars_frameWidthScaled;
			// commented out to allow text to upscale on 3K resolution: https://github.com/kkots/ggxrd_hitbox_overlay_2211/issues/6
			//if (w > (int)firstDigit.width) w = firstDigit.width;
			if (w < 3) w = 3;
			for (int i = 0; i <= 9; ++i) {
				PngResource& digit = (isMiniIter == 1 ? digitsMini : digits)[i];
				digit.resize(w, digitHeight);
				
				PngResource digitScaled;
				digitScaled.resize(w - 2, digitHeight - 2);
				
				const PngResource& source = *digitFrame[i].thickness[thicknessIndex];
				source.stretchRect(digitScaled,
					0, 0,
					source.width,
					source.height,
					0, 0,
					digitScaled.width,
					digitScaled.height);
				
				for (int offX = 0; offX < 3; ++offX) {
					for (int offY = 0; offY < 3; ++offY) {
						if (offX == 1 && offY == 1) continue;
						digitScaled.drawTintWithMaxAlpha(digit,
							0, 0,
							offX, offY,
							digitScaled.width,
							digitScaled.height,
							0xFF000000);
					}
				}
				//digit.multiplyAlphaByPercent(0, 0, digit.width, digit.height, 130);
				digitScaled.draw(digit,
					0, 0,
					1, 1,
					digitScaled.width, digitScaled.height);
				texturePacker.addImage(digit);
			}
		}
	}
	
	packedTexture = texturePacker.getTexture();
	
	{
		
		for (int i = 0; i < colorsCount; ++i) {
			for (int j = 1; j < FT_LAST; ++j) {
				if (type == UITextureType::UITEX_FRAMEBAR && isNewHitType((FrameType)j)) continue;
				FrameArt& art = frameArtArrays[i][j];
				for (int k = 0; k < uniqueFrameArtsCount; ++k) {
					const PngResource* res = uniqueFrameArts[k];
					if (art.resource == res) {
						assignFromResource(art, *res);
						break;
					}
				}
			}
		}
	}
	
	if (miniCount == 2) {
		const float coeff = sizesFrameHeightMaxFloat == 0.F ? 1.F : sizesFrameHeightMinFloat / sizesFrameHeightMaxFloat;
		memcpy(frameArtArrayMini, frameArtArrays[0], FT_LAST * sizeof FrameArt);
		for (int i = 1; i < FT_LAST; ++i) {
			FrameArt& art = frameArtArrayMini[i];
			art.framebar.end.y = art.framebar.start.y
				+ (art.framebar.end.y - art.framebar.start.y) * coeff;
		}
	}
	
	{
		
		for (int i = 0; i < colorsCount; ++i) {
			for (int j = 0; j < MARKER_TYPE_LAST; ++j) {
				FrameMarkerArt& art = frameMarkerArtArrays[i][j];
				
				for (int k = 0; k < uniqueOutlinedScaledMarkersCount; ++k) {
					UsedMarker& usedMarker = uniqueOutlinedScaledMarkers[k];
					if (usedMarker.markerArt.equal(art)) {
						assignFromResource(art, usedMarker.resource);
						break;
					}
				}
			}
		}
	}
	
	assignFromResource(firstFrameArt, firstFrame);
	
	assignFromResource(hitConnectedFrameArt, hitConnectedFrameAr[0]);
	assignFromResource(hitConnectedFrameBlackArt, hitConnectedBlackFrameAr[0]);
	if (type == UITextureType::UITEX_FRAMEBAR) {
		assignFromResource(hitConnectedFrameArtAlt, hitConnectedFrameAr[1]);
		assignFromResource(hitConnectedFrameBlackArtAlt, hitConnectedBlackFrameAr[1]);
		if (miniCount == 2) {
			assignFromResource(hitConnectedFrameArtMini, hitConnectedFrameArMini[0]);
			assignFromResource(hitConnectedFrameArtAltMini, hitConnectedFrameArMini[1]);
			assignFromResource(hitConnectedFrameBlackArtMini, hitConnectedBlackFrameArMini[0]);
			assignFromResource(hitConnectedFrameBlackArtAltMini, hitConnectedBlackFrameArMini[1]);
		}
	}
	
	assignFromResource(powerupFrameArt, powerup);
	
	assignFromResource(newHitFrameArt, newHit);
	if (miniCount == 2) {
		assignFromResource(newHitFrameArtMini, newHitMini);
	}
	
	if (type == UITextureType::UITEX_FRAMEBAR && sizes->drawDigits) {
		for (int i = 0; i <= 9; ++i) {
			assignFromResource(digitUVs[i], digits[i]);
		}
		if (miniCount == 2) {
			for (int i = 0; i <= 9; ++i) {
				assignFromResource(digitUVsMini[i], digitsMini[i]);
			}
		}
	}
	
	#undef assignFromResource
	#undef selectSE
}

// runs on the main thread
void UI::packTextureHelp() {
	if (packedTextureHelp.width) return;
	PackTextureSizes sizes;
	sizes.frameWidth = 9;
	sizes.frameHeight = 15;
	sizes.frameHeightProjectile = sizes.frameHeight;
	packTexture(packedTextureHelp, UITextureType::UITEX_HELP, &sizes);
	endScene.needsFramesTextureHelp = true;
}

// runs on the main thread
void UI::packTextureFramebar(const PackTextureSizes* sizes, bool isColorblind) {
	if (packedTextureFramebar.width && lastPackedSize == *sizes && textureIsColorblind == isColorblind) return;
	needUpdateGraphicsFramebarTexture = true;
	lastPackedSize = *sizes;
	textureIsColorblind = isColorblind;
	packTexture(packedTextureFramebar, UITextureType::UITEX_FRAMEBAR, sizes);
	endScene.needsFramesTextureFramebar = true;
}

// runs on the main thread
float truncfTowardsZero(float value) {
	return std::truncf(value
		+ (
			value > 0.F
				? 0.001F
				: -0.001F
		)
	);
}

// runs on the main thread
void UI::testDelay() {
	if (!*aswEngine) {
		needTestDelay = false;
		needTestDelayStage2 = false;
		idiotPressedTestDelayButtonOutsideBattle = true;
		return;
	}
	idiotPressedTestDelayButtonOutsideBattle = false;
	needTestDelay = true;
	needTestDelayStage2 = false;
	hasTestDelayResult = false;
	if (buttonSettings) {
		punchCode = buttonSettings[game.getPlayerPadID()].punch;
		if ((int)punchCode > 0 && punchCode < 256) {
			SendMessageA(keyboard.thisProcessWindow, WM_KEYDOWN, punchCode, 0);
			needTestDelay_dontReleaseKey = true;
		}
	}
}

// runs on the main thread
void UI::printBedmanSeals(const BedmanInfo& bi, bool forFrameTooltip) {
	struct SealInfo {
		const char* txt;
		unsigned short timer;
		unsigned short timerMax;
		unsigned short invulnerable;
	};
	SealInfo seals[4] {
		{ forFrameTooltip ? "Task A Seal: " : searchFieldTitle("Task A Seal Timer: "), bi.sealA, bi.sealAMax, bi.sealAInvulnerable },
		{ forFrameTooltip ? "Task A' Seal: " : searchFieldTitle("Task A' Seal Timer: "), bi.sealB, bi.sealBMax, bi.sealBInvulnerable },
		{ forFrameTooltip ? "Task B Seal: " : searchFieldTitle("Task B Seal Timer: "), bi.sealC, bi.sealCMax, bi.sealCInvulnerable },
		{ forFrameTooltip ? "Task C Seal: " : searchFieldTitle("Task C Seal Timer: "), bi.sealD, bi.sealDMax, bi.sealDInvulnerable }
	};
	for (int i = 0; i < 4; ++i) {
		SealInfo& seal = seals[i];
		if (!forFrameTooltip || seal.timer) {
			yellowText(seal.txt);
			ImGui::SameLine();
			sprintf_s(strbuf, "%d/%d%s", seal.timer, seal.timerMax,
				seal.invulnerable ? " (Invulnerable)" : "");
			ImGui::TextUnformatted(strbuf);
		}
	}
}

// runs on the main thread
static void initializeLettersHelper(bool* array, const std::initializer_list<char>& list) {
	for (char c = 'a'; c <= 'z'; ++c) {
		array[c - 'a'] = false;
	}
	for (char c : list) {
		array[c - 'a'] = true;
	}
}

// runs on the main thread
void initializeLetters(bool* letters, bool* lettersStandalone) {
	initializeLettersHelper(letters, { 'a', 'o', 'e', 'i' });
	initializeLettersHelper(lettersStandalone, { 's', 'h', 'x', 'r', 'f', 'l', 'm' });
}

// runs on the main thread
void printReflectableProjectilesList() {
	yellowText("Reflectable Projectiles (Rev2)");
	drawFontSizedPlayerIconWithText(CHARACTER_TYPE_SOL, "Gunflame");
	drawFontSizedPlayerIconWithText(CHARACTER_TYPE_KY, "Stun Edge (not reinforced)");
	drawFontSizedPlayerIconWithText(CHARACTER_TYPE_KY, "CSE (not reinforced)");
	drawFontSizedPlayerIconWithText(CHARACTER_TYPE_MAY, "Beach Ball");
	drawFontSizedPlayerIconWithText(CHARACTER_TYPE_MILLIA, "Tandem Top");
	drawFontSizedPlayerIconWithText(CHARACTER_TYPE_VENOM, "Ball (but not Bishop Runout or Red Hail)");
	drawFontSizedPlayerIconWithText(CHARACTER_TYPE_INO, "HCL");
	drawFontSizedPlayerIconWithText(CHARACTER_TYPE_INO, "HCL (Follow-up)");
	drawFontSizedPlayerIconWithText(CHARACTER_TYPE_INO, "VCL");
	drawFontSizedPlayerIconWithText(CHARACTER_TYPE_INO, "Antidepressant Scale");
	drawFontSizedPlayerIconWithText(CHARACTER_TYPE_RAMLETHAL, "Cassius");
	drawFontSizedPlayerIconWithText(CHARACTER_TYPE_LEO, "Graviert Wurde");
	drawFontSizedPlayerIconWithText(CHARACTER_TYPE_JACKO, "Magician Attack");
	drawFontSizedPlayerIconWithText(CHARACTER_TYPE_JACKO, "j.D");
	drawFontSizedPlayerIconWithText(CHARACTER_TYPE_HAEHYUN, "Tuning Ball");
	drawFontSizedPlayerIconWithText(CHARACTER_TYPE_RAVEN, "Needle");
	drawFontSizedPlayerIconWithText(CHARACTER_TYPE_BAIKEN, "Yasha Gatana");
}

// runs on the main thread
int __cdecl UI::CompareMoveInfo(void const* moveLeft, void const* moveRight) {
	SortedMovesEntry* ptrLeft = (SortedMovesEntry*)moveLeft;
	SortedMovesEntry* ptrRight = (SortedMovesEntry*)moveRight;
	return (BYTE*)ptrLeft->comparisonValue - (BYTE*)ptrRight->comparisonValue;
}

// runs on the main thread
void UI::onAswEngineDestroyed() {
	sortedMovesRedoPendingWhenAswEngingExists = true;
	if (gifMode.editHitboxes && settings.hitboxEditUnfreezeGameWhenLeavingEditMode && freezeGame) {
		stopHitboxEditMode(true);
	}
	gifMode.editHitboxesEntity = nullptr;
	selectedHitboxes.clear();
	resetKeyboardMoveCache();
	resetCoordsMoveCache();
	sortedAnimSeqs.clear();
	for (FPACSecondaryData& data : hitboxEditorFPACSecondaryData) {
		data.clear();
	}
	eraseHistory();
}

// runs on the main thread
void UI::highlightedMovesChanged() {
	sortedMovesRedoPending = true;
}

// runs on the main thread
void UI::prepareOutlinedFont() {
	static bool ranOnce = false;
	if (ranOnce) return;
	ranOnce = true;
	const ImFont* font = ImGui::GetFont();
	if (!font) return;
	
	fontDataAlt.resize(fontDataWidth * fontDataHeight * 4, 0);
	struct Pixel {
		BYTE r;
		BYTE g;
		BYTE b;
		BYTE a;
	};
	const Pixel* fontDataPtr = (const Pixel*)fontData;
	Pixel* fontDataAltPtr = (Pixel*)fontDataAlt.data();
	const Pixel theBlackPixel { 0, 0, 0, 255 };
	const float fontDataWidthFloat = (float)fontDataWidth;
	const float fontDataHeightFloat = (float)fontDataHeight;
	for (int i = 0; i < font->Glyphs.Size; ++i) {
		const ImFontGlyph& glyph = font->Glyphs.Data[i];
		if (glyph.Codepoint
				&& glyph.Codepoint > 32
				&& glyph.Codepoint != 0x7f
				&& glyph.Codepoint != 0x81
				&& glyph.Codepoint != 0x8d
				&& glyph.Codepoint != 0x8f
				&& glyph.Codepoint != 0x90
				&& glyph.Codepoint != 0x9d
				&& glyph.Codepoint != 0xa0
				&& glyph.Codepoint != 0xad) {
			int x = (int)std::roundf(glyph.U0 * fontDataWidthFloat);
			int y = (int)std::roundf(glyph.V0 * fontDataHeightFloat);
			int w = (int)glyph.X1 - (int)glyph.X0;
			int h = (int)glyph.Y1 - (int)glyph.Y0;
			int pxNextRow = fontDataWidth - w;
			const int offsetIndex = y * fontDataWidth + x;
			const Pixel* px = &fontDataPtr[offsetIndex];
			Pixel* destPx = &fontDataAltPtr[offsetIndex];
			for (int yIter = 0; yIter < h; ++yIter) {
				for (int xIter = 0; xIter < w; ++xIter) {
					if (px->a) {
						*(destPx                 - 1) = theBlackPixel;
						*(destPx                 + 1) = theBlackPixel;
						*(destPx - fontDataWidth - 1) = theBlackPixel;
						*(destPx - fontDataWidth    ) = theBlackPixel;
						*(destPx - fontDataWidth + 1) = theBlackPixel;
						*(destPx + fontDataWidth - 1) = theBlackPixel;
						*(destPx + fontDataWidth    ) = theBlackPixel;
						*(destPx + fontDataWidth + 1) = theBlackPixel;
					}
					++px;
					++destPx;
				}
				px += pxNextRow;
				destPx += pxNextRow;
			}
		}
	}
	const Pixel* px = fontDataPtr;
	Pixel* destPx = fontDataAltPtr;
	for (int y = 0; y < fontDataHeight; ++y) {
		for (int x = 0; x < fontDataWidth; ++x) {
			if (px->a) {
				*destPx = *px;
			}
			++px;
			++destPx;
		}
	}
	
}

// runs on the main thread
void setOutlinedText(bool isOutlined) {
	*outlinedTextHead = isOutlined;
    if (isOutlined != ImGui::imGuiTextOutlined) {
		static ImFont* outlinedFont = nullptr;
        ImGui::imGuiTextOutlined = isOutlined;
        if (isOutlined) {
        	if (!outlinedFont) {
        		outlinedFont = ImGui::GetFont();
        	}
        	outlinedFont->ContainerAtlas->TexID = TEXID_IMGUIFONT_OUTLINED;
	    	ImGui::PushFont(outlinedFont);
	    	ImGui::GetWindowDrawList()->PushTextureID(TEXID_IMGUIFONT_OUTLINED);
        } else {
        	if (outlinedFont) {
        		outlinedFont->ContainerAtlas->TexID = TEXID_IMGUIFONT;
        	}
        	ImGui::PopFont();
	    	ImGui::GetWindowDrawList()->PopTextureID();
        }
    }
}

// runs on the main thread
void pushOutlinedText(bool isOutlined) {
	++outlinedTextHead;
	setOutlinedText(isOutlined);
}

// runs on the main thread
void popOutlinedText() {
	--outlinedTextHead;
	setOutlinedText(*outlinedTextHead);
}

void UI::clearImGuiFontAlt() {
	if (imguiFontAlt) {
		imguiFontAlt = nullptr;
		attemptedCreatingAltFont = false;
	}
}

void UI::clearPinTexture() {
	if (pinTexture) {
		pinTexture = nullptr;
		attemptedCreatingPin = false;
	}
}

void UI::clearSecondaryTextures() {
	clearImGuiFontAlt();
	clearPinTexture();
}

void UI::readPinnedUniversal(PinnedWindowEnum index) {
	bool newVal = ImGui::IsWindowPinned();
	PinnedWindowElement& dest = settings.pinnedWindows[index];
	if (dest.isPinned != newVal) {
		dest.isPinned = newVal;
		needWriteSettings = true;
		if (newVal) {
			int order = 0;
			for (int i = 0; i < _countof(settings.pinnedWindows.elements); ++i) {
				if (i != index) {
					PinnedWindowElement& element = settings.pinnedWindows[i];
					if (element.isPinned) {
						element.order = order++;
					}
				}
			}
			dest.order = order;
		}
	}
}

void UI::pinnedWindowsChanged() {
	// welp
}

void UI::WindowStruct::setOpen(bool newOpen, bool isManual) {
	if (newOpen == isOpen) return;
	isOpen = newOpen;
	if (newOpen) openedJustNow = true;
	if (newOpen && isManual && !ui.needDraw(index) && ui.windowShowMode == WindowShowMode_Pinned && !isPinned()) {
		setPinned(true);
	}
	if (!newOpen && isManual) {
		setPinned(false);
	}
}

UI::WindowStruct::WindowStruct(PinnedWindowEnum index, const char* titleFmtString, ...)
		: index(index) {
	int requiredLength = 0;  // does not include null
	{
		va_list args;
	    va_start(args, titleFmtString);
	    requiredLength = vsnprintf(nullptr, 0, titleFmtString, args);
	    // the returned count is in characters, not including null
	    va_end(args);
	}
	if (!requiredLength) {
		char buf[] = "search_0xFFFFFFFF";
		sprintf_s(buf, "search_0x%p", this);
		searchTitle = buf;
		
		sprintf_s(buf, "0x%p", this);
		title = buf;
		return;
	}
	// resizing string and then writing into it has not worked for me in some situations in the past but maybe I am
	// misremembering exactly what happened and the problem was something else.
	// Point is, I'm no longer trusting string to be contiguous until the call to c_str(), until they release and
	// make available a writable pointer to their data.
	// Without this, we might be running around, +'ing strings together which may or may not be string builders in disguise
	// and we will never know the truth
	// In case you're confused, I was talking about why I am using std::vector instead of std::string
	std::vector<char> buf(requiredLength + 1);
	va_list args;
    va_start(args, titleFmtString);
    // - - - - - - - - but this size is in bytes
    vsnprintf(buf.data(), buf.size(), titleFmtString, args);
    va_end(args);  // why I need to clean this up is beyond me. My caller is reponsible for cleaning up stack
    title.assign(buf.data(), buf.data() + requiredLength);
    
    searchTitle = "search_" + title;
}

void UI::WindowStruct::init() {
	element = &settings.pinnedWindows[index];
	setOpen(settings.openPinnedWindowsOnStartup && isPinned(), false);
}

void UI::customBegin(PinnedWindowEnum index) {
	lastCustomBeginPushedAStyleStack[lastCustomBeginPushedStackDepth] = lastCustomBeginPushedAStyle;
	lastCustomBeginPushedOutlinedTextStack[lastCustomBeginPushedStackDepth] = lastCustomBeginPushedOutlinedText;
	lastCustomBeginHadPinButtonStack[lastCustomBeginPushedStackDepth] = lastCustomBeginHadPinButton;
	lastWindowClosedStack[lastCustomBeginPushedStackDepth] = lastWindowClosed;
	++lastCustomBeginPushedStackDepth;
	
	WindowStruct& windowStruct = windows[index];
	ImGuiWindowFlags newFlags;
	ImGuiWindowFlags pinFlag = settings.disablePinButton ? 0 : ImGuiWindowFlags_HasPinButton;
	if (searching) {
		newFlags = ImGuiWindowFlags_NoSavedSettings;
	} else if (index == PinnedWindowEnum_ComboDamage_1
			|| index == PinnedWindowEnum_ComboDamage_2) {
		newFlags = ImGuiWindowFlags_NoBackground | pinFlag;
	} else if (index == PinnedWindowEnum_ComboRecipe_1
			|| index == PinnedWindowEnum_ComboRecipe_2) {
		if (settings.comboRecipe_transparentBackground) {
			newFlags = ImGuiWindowFlags_NoBackground | pinFlag;
		} else {
			newFlags = (windowFlags & ImGuiWindowFlags_NoBackground) | pinFlag;
		}
	} else {
		newFlags = windowFlags;
	}
	if (windowStruct.openedJustNow) {
		windowStruct.openedJustNow = false;
	} else {
		newFlags |= ImGuiWindowFlags_NoFocusOnAppearing;
	}
	if (overrideWindowColor && !(newFlags & ImGuiWindowFlags_NoBackground)) {
		ImGui::PushStyleColor(ImGuiCol_WindowBg, windowColor);
		lastCustomBeginPushedAStyle = true;
	} else {
		lastCustomBeginPushedAStyle = false;
	}
	const PinnedWindowElement& pinnedElement = settings.pinnedWindows[index];
	if (pinnedElement.isPinned && !settings.disablePinButton) {
		ImGui::IsNextWindowPinned = true;
		ImGui::NextWindowPinnedOrder = pinnedElement.order;
	}
	if (searching) {
		ImGui::SetNextWindowPos({ 100000.F, 100000.F }, ImGuiCond_Always);
	}
	bool isOpen = true;
	lastCustomBeginHadPinButton = (newFlags & ImGuiWindowFlags_HasPinButton);
	ImGui::Begin(searching ? windowStruct.searchTitle.c_str() : windowStruct.title.c_str(), &isOpen, newFlags);
	lastWindowClosed = !isOpen;
	if (index != PinnedWindowEnum_ComboDamage_1
			&& index != PinnedWindowEnum_ComboDamage_2
			&& index != PinnedWindowEnum_ComboRecipe_1
			&& index != PinnedWindowEnum_ComboRecipe_2
			&& settingOutlineText) {
		pushOutlinedText(true);
		lastCustomBeginPushedOutlinedText = true;
	} else {
		lastCustomBeginPushedOutlinedText = false;
	}
	lastCustomBeginIndex = index;
	readPinnedUniversal(index);
}

void UI::customEnd() {
	WindowStruct& lastWindow = windows[lastCustomBeginIndex];
	if (!searching && lastWindowClosed) {
		lastWindow.setOpen(false, true);
		if (settings.closingModWindowAlsoHidesFramebar && !isVisible()) {
			windowShowMode = WindowShowMode_None;  // for closingModWindowAlsoHidesFramebar
		}
	}
	if (lastCustomBeginPushedOutlinedText) popOutlinedText();
	if (lastCustomBeginPushedAStyle) ImGui::PopStyleColor();
	
	--lastCustomBeginPushedStackDepth;
	lastCustomBeginPushedAStyle = lastCustomBeginPushedAStyleStack[lastCustomBeginPushedStackDepth];
	lastCustomBeginPushedOutlinedText = lastCustomBeginPushedOutlinedTextStack[lastCustomBeginPushedStackDepth];
	lastCustomBeginHadPinButton = lastCustomBeginHadPinButtonStack[lastCustomBeginPushedStackDepth];
	lastWindowClosed = lastWindowClosedStack[lastCustomBeginPushedStackDepth];
	
	ImGui::End();
}

bool UI::needDraw(PinnedWindowEnum index) const {
	const WindowStruct& windowStruct = windows[index];
	if (!windowStruct.isOpen) return false;
	switch (windowShowMode) {
		case WindowShowMode_Pinned: return windowStruct.isPinned();
		case WindowShowMode_None: return false;
		default: return true;
	}
}

void UI::toggleOpen(PinnedWindowEnum index, bool isManual) {
	WindowStruct& windowStruct = windows[index];
	windowStruct.toggleOpen(isManual);
}

void UI::setOpen(PinnedWindowEnum index, bool isOpen, bool isManual) {
	WindowStruct& windowStruct = windows[index];
	windowStruct.setOpen(isOpen, isManual);
}

bool UI::hasAtLeastOnePinnedOpenWindow() const {
	for (int i = 0; i < PinnedWindowEnum_Last; ++i) {
		const WindowStruct& windowStruct = windows[i];
		if (windowStruct.isOpen && windowStruct.isPinned()) {
			return true;
		}
	}
	return false;
}

bool UI::hasAtLeastOnePinnedWindowThatIsNotTheMainWindow() const {
	for (int i = 0; i < PinnedWindowEnum_Last; ++i) {
		const WindowStruct& windowStruct = windows[i];
		if (windowStruct.isPinned() && i != PinnedWindowEnum_MainWindow) {
			return true;
		}
	}
	return false;
}

bool UI::hasAtLeastOneUnpinnedOpenWindow() const {
	for (int i = 0; i < PinnedWindowEnum_Last; ++i) {
		const WindowStruct& windowStruct = windows[i];
		if (windowStruct.isOpen && !windowStruct.isPinned()) {
			return true;
		}
	}
	return false;
}

bool UI::hasAtLeastOneOpenWindow() const {
	for (int i = 0; i < PinnedWindowEnum_Last; ++i) {
		const WindowStruct& windowStruct = windows[i];
		if (windowStruct.isOpen) {
			return true;
		}
	}
	return false;
}

void UI::onVisibilityToggleKeyboardShortcutPressed() {
	WindowStruct& mainWindow = windows[PinnedWindowEnum_MainWindow];
	if (hasAtLeastOnePinnedOpenWindow() && hasAtLeastOneUnpinnedOpenWindow()) {
		switch (windowShowMode) {
			case WindowShowMode_All: windowShowMode = WindowShowMode_Pinned; break;
			case WindowShowMode_Pinned: windowShowMode = WindowShowMode_None; break;
			case WindowShowMode_None: windowShowMode = WindowShowMode_All; break;
		}
		if (windowShowMode == WindowShowMode_All) {
			mainWindow.setOpen(true, false);
		} else if (windowShowMode == WindowShowMode_Pinned && mainWindow.isPinned()) {
			mainWindow.setOpen(true, false);
		}
	} else {
		if (isVisible()) {
			windowShowMode = WindowShowMode_None;
		} else {
			// no, you are not allowed to have no windows visible at all and still demand that I switch windowShowMode to None, that is an insane demand
			windowShowMode = WindowShowMode_All;
			mainWindow.setOpen(true, false);
		}
	}
}

bool UI::isVisible() const {
	for (int i = 0; i < PinnedWindowEnum_Last; ++i) {
		if (needDraw((PinnedWindowEnum)i)) {
			return true;
		}
	}
	return false;
}

void UI::onDisablePinButtonChanged(bool postedFromOutsideUI) {
	if (!settings.disablePinButton) return;
	
	if (windowShowMode == WindowShowMode_Pinned) {
		windowShowMode = WindowShowMode_All;
	}
	
	if (postedFromOutsideUI) return;
	bool weNeedToTalk = false;
	for (int i = 0; i < PinnedWindowEnum_Last; ++i) {
		bool& value = settings.pinnedWindows[i].isPinned;
		if (value) {
			value = false;
			weNeedToTalk = true;
		}
	}
	if (weNeedToTalk) {
		needWriteSettings = true;
	}
}

void UI::startHitboxEditMode() {
	if (gifMode.editHitboxes) return;
	gifMode.editHitboxes = true;
	entityList.populate();
	if (!gifMode.editHitboxesEntity) {
		gifMode.editHitboxesEntity = entityList.slots[game.currentPlayerControllingSide()];
	}
	if (!freezeGame) {
		stateChanged = true;
		freezeGame = true;
	}
	
	Entity pawn = entityList.slots[game.currentPlayerControllingSide()];
	
	
	static DrawHitboxArrayCallParams boxes[17];
	RECT bounds;
	int posX = pawn.posX();
	int posY = pawn.posY();
	bounds.left = posX;
	bounds.right = posX;
	bounds.top = posY;
	bounds.bottom = posY;
	
	for (int i = 0; i < 17; ++i) {
		int count = pawn.hitboxes()->count[i];
		if (!count) continue;
		DrawHitboxArrayCallParams& box = boxes[i];
		box.data.resize(count);
		memcpy(box.data.data(), pawn.hitboxes()->data[i], sizeof (Hitbox) * count);
		box.params.flip = pawn.isFacingLeft() ? 1 : -1;
		box.params.scaleX = pawn.scaleX();
		box.params.scaleY = pawn.scaleY();
		box.params.angle = pawn.pitch();
		box.params.transformCenterX = pawn.transformCenterX();
		box.params.transformCenterY = pawn.transformCenterY();
		box.params.posX = posX;
		box.params.posY = posY;
		combineBounds(bounds, box.getWorldBounds());
	}
	
	camera.editHitboxesOffsetX = 0.F;
	camera.editHitboxesOffsetY = settings.cameraCenterOffsetY_WhenForcePitch0;
	camera.editHitboxesViewDistance = settings.cameraCenterOffsetZ;
	
	// I want to take a brief moment to ajust the camera view to capture all of the hitbox data without having to manually scroll anywhere
	// I am as lost as you are so please bear with me
	
	// UE3 coordinate space is:
	// Z points up
	// Y points away from the fighting arena, to the right from P1's character's side
	// X points to the right of arena
	
	// we force pitch to be 0, no yaw allowed during edit mode, so it's -PI/2. No roll either
	// right = (1,0,0)     ; must later become y in camera space
	// up = (0,0,1)        ; must later become z in camera space
	// forward = (0,-1,0)  ; must later become x in camera space
	
	// (x,0,y) - the original vector of a hitbox point, x,y are from ArcSys space where x points right and y up
	// we must first untranslate the camera. So, given cam_x, cam_y, cam_z, we get relative vector r:
	// r = (x,0,y) - (cam_x, cam_y, cam_z) =
	//   = (x-cam_x,-cam_y,y-cam_z)
	// if we undo the yaw, the only rotation, we get, in UE3 camera's space (where x points forward, y right, z up):
	// r'=(cam_y,x-cam_x,y-cam_z)
	
	// we have this: float t = 1.F / tanf(fov);  // this is from Altimor's formula
	// we will never auto-adjust fov, so this is just an opaque constant to us that I'll call t
	
	// and this projection matrix:
	//projection = {
	// 1.F / viewportW, 1.F / viewportH,            1.F, 1.F,  // UE3 uses left-hand coordinates, and D3D also uses left-hand
	// t,               0.F,                        0.F, 0.F,
	// 0.F,             t * viewportW / viewportH,  0.F, 0.F,
	// 0.F,             0.F,                        0.F, 0.F
	//};
	
	// we will shorten viewportW to vw, viewportH to vh
	
	// so I take point r' and multiply by matrix to get projected point p:
	// p=...hold on
	// in case you're wondering how am I multiplying an (x,y,z) point by (4x4) matrix, I am legally obligated to add a fourth dimension, w=1, to the point.
	// p=( r'x/vw + r'y*t, r'x/vh + r'z*t*vw/vh, r'x, r'x )
	// D3D then autopilots the p'.x = p.x / p.z, p'.y = p.y / p.z, and p' is the actual screen coordinate, screen 0;0 being in the center, x right, y up, 1;-1 are edges
	// p.z would equal our r'.x (makes sense)
	
	// I got this:
	// p' = ( 1/vw + (x-cam_x)/cam_y*t, 1/vh+(y-cam_z)/cam_y*t*vw/vh )
	// x and y are ArcSys coordinates, cam_x/y/z are UE3 camera coordinates, p' coordinates we just discussed
	
	// if we imagine we've locked cam_y in place, we can actually see how the point on the screen would move linearly when either
	// moving the camera or the ArcSys (source) point. So there's some coefficient we can get that would allow us to convert
	// displacement in the source point into displacement in camera position.
	// But first I am most interested in adjusting cam_y so that our viewing rect is big enough to accomodate all the boxes.
	// I assume same coefficient we can multiply by a size expressed in ArcSys units and get a size in screen units.
	// And before even that, I want to get how big the default camera already is. Maybe we don't need to resize or move anything.
	// p' must equal (1,1), then (-1,-1). Subtract one (resulting ArcSys point) from the other.
	// 1/vw + (x-cam_x)/cam_y*t = DESTx;
	// 1/vh+(y-cam_z)/cam_y*t*vw/vh = DESTy;
	// --------
	// (x-cam_x)/cam_y = (DESTx-1/vw)/t;
	// (y-cam_z)/cam_y = (DESTy-1/vh)/t/vw*vh;
	// --------
	// x-cam_x = cam_y*(DESTx-1/vw)/t;
	// y-cam_z = cam_y*(DESTy-1/vh)/t/vw*vh;
	// --------
	// x = cam_y*(DESTx-1/vw)/t+cam_x;
	// y = cam_y*(DESTy-1/vh)/t/vw*vh+cam_z;
	
	EndScene::HitboxEditorCameraValues& vals = endScene.hitboxEditorCameraValues;
	if (vals.prepare(pawn)) {
		float boundsWASW = (bounds.right - bounds.left) * vals.xCoeff;
		float boundsHASW = (bounds.bottom - bounds.top) * vals.yCoeff;
		
		if (boundsWASW > 1.9F || boundsHASW > 1.9F) {
			// boundsWASW*t/cam_y*m must equal = 1.9F
			// boundsHASW*t*vw/vh/cam_y*m must equal = 1.9F
			// --------
			// boundsWASW*t/cam_y*m = 1.9F
			// boundsHASW*t*vw/vh/cam_y*m = 1.9F
			// --------
			// boundsWASW*t*m = 1.9F*cam_y
			// boundsHASW*t*vw/vh*m = 1.9F*cam_y
			// --------
			// cam_y = boundsWASW*t*m/1.9F
			// cam_y = boundsHASW*t*vw/vh*m/1.9F
			float new_cam_y = boundsWASW * vals.t * vals.m / 1.9F;
			float val = boundsHASW * vals.t * vals.vw / vals.vh * vals.m / 1.9F;
			if (val > new_cam_y) {
				new_cam_y = val;
			}
			camera.editHitboxesViewDistance = new_cam_y;
		}
		
		float boundsXASW = (float)(
			(bounds.right + bounds.left) >> 1
		);
		float boundsYASW = (float)(
			(bounds.bottom + bounds.top) >> 1
		);
		
		camera.editHitboxesOffsetX = (boundsXASW - (float)pawn.posX()) * vals.m;
		camera.editHitboxesOffsetY = (boundsYASW - (float)pawn.posY()) * vals.m;
		
	}
	
	// we're frozen, the camera won't update the normal way
	camera.updateCameraManually();
	
}

void UI::stopHitboxEditMode(bool resetEntity) {
	if (!gifMode.editHitboxes) return;
	resetKeyboardMoveCache();
	resetCoordsMoveCache();
	gifMode.editHitboxes = false;
	sortedAnimSeqs.clear();
	if (resetEntity) {
		if (settings.hitboxEditUnfreezeGameWhenLeavingEditMode) {
			if (freezeGame) {
				stateChanged = true;
				freezeGame = false;
			}
		}
		gifMode.editHitboxesEntity = nullptr;
		selectedHitboxes.clear();
	}
}

void UI::drawHitboxEditor() {
	
	if (gifMode.editHitboxes && gifMode.editHitboxesEntity) {
		#define predetermText(name, text) \
			static const StringWithLength name = text; \
			static bool name##WidthCalculated = false; \
			static float name##Width = 0.F; \
			if (!name##WidthCalculated) { \
				name##WidthCalculated = true; \
				name##Width = ImGui::CalcTextSize(name.txt, name.txt + name.length).x; \
			}
		Entity playerEnt = Entity{gifMode.editHitboxesEntity}.playerEntity();
		ImVec2 windowPos = ImGui::GetWindowPos();
		float windowWidth = ImGui::GetWindowWidth();
		ImGuiStyle& style = ImGui::GetStyle();
		const float fontSize = ImGui::GetFontSize();
		int bbscrIndexInAswEng = Entity{gifMode.editHitboxesEntity}.bbscrIndexInAswEng();
		const std::string& title = windows[PinnedWindowEnum_HitboxEditor].title;
		ImVec2 titleSize = ImGui::CalcTextSize(title.c_str(), title.c_str() + title.size());
		GGIcon icon;
		if (bbscrIndexInAswEng != 2) {
			icon = scaleGGIconToHeight(getCharIcon(playerEnt.characterType()), fontSize);
		}
		ImVec2 startPos {
			windowPos.x + style.FramePadding.x + fontSize + style.ItemSpacing.x + titleSize.x + style.ItemSpacing.x,
			windowPos.y + style.FramePadding.y
		};
		float startPosOrigX = startPos.x;
		ImVec2 clipEnd {
			windowPos.x + windowWidth - style.FramePadding.x - fontSize - style.ItemInnerSpacing.x
				- (ui.lastCustomBeginHadPinButton ? 19.F + style.ItemInnerSpacing.x : 0.F),
			startPos.y + icon.size.y
		};
		if (clipEnd.x > startPos.x) {
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			drawList->PushClipRect({ startPos.x, windowPos.y },
				{ clipEnd.x, windowPos.y + style.FramePadding.y + fontSize + style.FramePadding.y },
				false);
			int alpha = ImGui::IsWindowCollapsed() ? 128 : 255;
			ImU32 white = ImGui::GetColorU32(IM_COL32(255, 255, 255, alpha));
			predetermText(parenthesisOpen, "(")
			drawList->AddText(startPos, white, parenthesisOpen.txt, parenthesisOpen.txt + parenthesisOpen.length);
			startPos.x += parenthesisOpenWidth;
			drawList->AddImage(TEXID_GGICON,
				startPos,
				{
					startPos.x + icon.size.x,
					startPos.y + icon.size.y
				},
				icon.uvStart,
				icon.uvEnd,
				white);
			startPos.x += icon.size.x + style.ItemInnerSpacing.x;
			if (bbscrIndexInAswEng != 2) {
				int strbufLength = sprintf_s(strbuf, "P%d's Collision (%s))",
					bbscrIndexInAswEng + 1,
					characterNames[playerEnt.characterType()]);
				drawList->AddText(startPos, white, strbuf, strbuf + strbufLength);
				startPos.x += ImGui::CalcTextSize(strbuf, strbuf + strbufLength).x;
			} else {
				predetermText(commonCollision, "Common Collision)")
				drawList->AddText(startPos, white, commonCollision.txt, commonCollision.txt + commonCollision.length);
				startPos.x += commonCollisionWidth;
			}
			drawList->PopClipRect();
		}
		#undef predetermText
	}
	
	struct DrawHitboxEditorOnExit {
		~DrawHitboxEditorOnExit() {
			if (currentSpriteElement) {
				ui.processDragNDrop(currentSpriteElement, 0);
			} else {
				ui.endDragNDrop();
				ui.dragNDropInterpolationTimer = 0;
			}
			checkClearSelectedHitboxes(nullptr);
		}
		void checkClearSelectedHitboxes(SortedSprite* currentSpriteElement) {
			if (gotToSelectedLayersPart) return;
			this->currentSpriteElement = currentSpriteElement;
			gotToSelectedLayersPart = true;
			Entity editEntity{gifMode.editHitboxesEntity};
			if (!gifMode.editHitboxes || !editEntity
					|| strcmp(editEntity.spriteName(), ui.selectedHitboxesSprite) != 0) {
				ui.selectedHitboxes.clear();
				ui.resetKeyboardMoveCache();
				ui.resetCoordsMoveCache();
				ui.endDragNDrop();
				ui.dragNDropInterpolationTimer = 0;
			}
			if (gifMode.editHitboxes && editEntity) {
				memcpy(ui.selectedHitboxesSprite, editEntity.spriteName(), 32);
			} else {
				memset(ui.selectedHitboxesSprite, 0, 32);
			}
		}
		SortedSprite* currentSpriteElement = nullptr;
		bool gotToSelectedLayersPart = false;
	} drawHitboxEditorOnExit;
	
	for (
			CogwheelButtonContext cogwheel(
				"##HitboxEditorCogwheel",
				&showHitboxEditorSettings,
				false,
				"Settings for the Hitbox Editor."
			);
			cogwheel.needShowSettings(); ) {
		ImGui::PushTextWrapPos(0.F);
		ImGui::TextUnformatted("The edited data gets lost on character change or after leaving training mode, unless saved into a .collision file.");
		ImGui::PopTextWrapPos();
		keyComboControl(settings.hitboxEditModeToggle);
		keyComboControl(settings.hitboxEditMoveCameraUp);
		keyComboControl(settings.hitboxEditMoveCameraDown);
		floatSettingPreset(settings.hitboxEditMoveCameraVerticalSpeedMultiplier);
		keyComboControl(settings.hitboxEditMoveCameraLeft);
		keyComboControl(settings.hitboxEditMoveCameraRight);
		floatSettingPreset(settings.hitboxEditMoveCameraHorizontalSpeedMultiplier);
		keyComboControl(settings.hitboxEditMoveCameraBack);
		keyComboControl(settings.hitboxEditMoveCameraForward);
		floatSettingPreset(settings.hitboxEditMoveCameraPerpendicularSpeedMultiplier);
		booleanSettingPreset(settings.hitboxEditUnfreezeGameWhenLeavingEditMode);
		booleanSettingPreset(settings.hitboxEditShowHitboxesOfEntitiesOtherThanTheOneBeingEdited);
		booleanSettingPreset(settings.hitboxEditShowFloorline);
		booleanSettingPreset(settings.hitboxEditShowOriginPoints);
		booleanSettingPreset(settings.hitboxEditZoomOntoMouseCursor);
		keyComboControl(settings.hitboxEditMoveHitboxesUp);
		keyComboControl(settings.hitboxEditMoveHitboxesDown);
		keyComboControl(settings.hitboxEditMoveHitboxesLeft);
		keyComboControl(settings.hitboxEditMoveHitboxesRight);
		intSettingPreset(settings.hitboxEditMoveHitboxesNormalAmount, INT_MIN, 1000, 10000);
		keyComboControl(settings.hitboxEditMoveHitboxesALotUp);
		keyComboControl(settings.hitboxEditMoveHitboxesALotDown);
		keyComboControl(settings.hitboxEditMoveHitboxesALotLeft);
		keyComboControl(settings.hitboxEditMoveHitboxesALotRight);
		intSettingPreset(settings.hitboxEditMoveHitboxesLargeAmount, INT_MIN, 1000, 10000);
		booleanSettingPreset(settings.hitboxEditDisplayRawCoordinates);
		keyComboControl(settings.hitboxEditAddSpriteHotkey);
		keyComboControl(settings.hitboxEditDeleteSpriteHotkey);
		keyComboControl(settings.hitboxEditRenameSpriteHotkey);
		keyComboControl(settings.hitboxEditSelectionToolHotkey);
		keyComboControl(settings.hitboxEditAddHitboxHotkey);
		keyComboControl(settings.hitboxEditDeleteSelectedHitboxesHotkey);
		keyComboControl(settings.hitboxEditUndoHotkey);
		keyComboControl(settings.hitboxEditRedoHotkey);
		keyComboControl(settings.hitboxEditArrangeHitboxesToBack);
		keyComboControl(settings.hitboxEditArrangeHitboxesBackwards);
		keyComboControl(settings.hitboxEditArrangeHitboxesUpwards);
		keyComboControl(settings.hitboxEditArrangeHitboxesToFront);
		ImGui::Separator();
	}
	
	comboBoxExtension.beginFrame();
	
	hitboxEditPressedToggleEditMode_isOn = gifMode.editHitboxes;
	if (ImGui::Checkbox(searchFieldTitle("Hitbox Edit Mode On"), &hitboxEditPressedToggleEditMode_isOn)) {
		hitboxEditPressedToggleEditMode = true;
	}
	static std::string hitboxEditModeCheckboxHelp;
	const char* frameNumTitle = "Frame/99:";
	ImGuiStyle& style = ImGui::GetStyle();
	static ImVec2 averageSpriteNameSize;
	static ImVec2 longestAnimSeqNameSize;
	static ImVec2 longestFrameNumSize;
	static ImVec2 frameNumTitleSize;
	static ImVec2 addSpriteButtonSize;
	static ImVec2 slashSize;
	if (hitboxEditModeCheckboxHelp.empty()) {
		hitboxEditModeCheckboxHelp = settings.convertToUiDescription(
			"The hotkey for this checkbox can be configured at \"hitboxEditModeToggle\".\n\n"
			
			"The game will freeze the frame automatically once you enter the hitbox editing mode."
			" Use the controls from the other, main menu, from the Hitboxes section, to"
			" unfreeze/freeze the frame or step one frame forward at a time (those controls"
			" can have hotkeys configured for them).\n\n"
			
			"The camera gets centered on the player. To move the camera up, down, left, right,"
			" zoom it in or out, use the mouse or keyboard or gamepad controls configured in"
			" \"hitboxEditMoveCameraUp\" and settings nearby there."
			
			);
		averageSpriteNameSize = ImGui::CalcTextSize("sol999_999_rx");
		longestAnimSeqNameSize = ImGui::CalcTextSize("WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW");
		longestAnimSeqNameSize.x += 40.F;
		longestFrameNumSize = ImGui::CalcTextSize("999");
		longestFrameNumSize.x += 50.F;
		frameNumTitleSize = ImGui::CalcTextSize(frameNumTitle);
		addSpriteButtonSize.x = PinAtlas::add_size + style.FramePadding.x * 2.F;
		slashSize = ImGui::CalcTextSize("/");
	}
	ImGui::SameLine();
	HelpMarkerWithHotkey(searchTooltip(hitboxEditModeCheckboxHelp), settings.hitboxEditModeToggle);
	
	ImGui::SameLine();
	if (ImGui::Button("Save##wasntthereanotherthingnamedSavesomewhere")) {
		ImGui::OpenPopup("Select Save Format");
	}
	AddTooltip("Upon pressing this button, you will be asked to select which data to save, in what format, and where."
		" The choices will be presented as a table with 3 rows and 3 columns."
		" Each column (P1, P2, Common) represents which data to save. The data currently being edited is displayed in"
		" the title of the 'Hitbox Editor' window."
		" Each row represents a data format and where to save. You can choose to save:\n"
		"1) .collision file. This will ask you to select a file, and binary .collision data will be written into it;\n"
		"2) .json file. This will ask you to select a file, and text .json data will be written into it;\n"
		"3) .json file to clipboard. This will overwrite your clipboard with JSON text. You will be able to paste that text into any text editor.");
	if (ImGui::BeginPopup("Select Save Format")) {
		enum SaveFormat {
			SAVE_FORMAT_OOPS,
			SAVE_FORMAT_COLLISION_FILE,
			SAVE_FORMAT_JSON_FILE,
			SAVE_FORMAT_JSON_TO_CLIPBOARD,
			SAVE_FORMAT_TOTAL
		} saveFormat = SAVE_FORMAT_OOPS;
		int player = -1;
		if (ImGui::BeginTable("##SelectSaveFormat", 3, tableFlags)) {
			P1P2CommonTable table;
			ImGui::TableHeadersRow();
			
			for (int row = 1; row < SAVE_FORMAT_TOTAL; ++row) {
				ImGui::PushID(row);
			
				const char* elementName = "";
				switch (row) {
					case SAVE_FORMAT_COLLISION_FILE: elementName = ".collision file"; break;
					case SAVE_FORMAT_JSON_FILE: elementName = ".json file"; break;
					case SAVE_FORMAT_JSON_TO_CLIPBOARD: elementName = ".json to clipboard"; break;
				}
				
				for (int column = 0; column < 3; ++column) {
					ImGui::PushID(column);
					
					ImGui::TableNextColumn();
					bool youareallnotselected = false;
					if (ImGui::Selectable(elementName, &youareallnotselected)) {
						if (youareallnotselected) {
							saveFormat = (SaveFormat)row;
							player = column;
						}
					}
					
					ImGui::PopID();
				}
				ImGui::PopID();
			}
			
			ImGui::EndTable();
		}
		ImGui::EndPopup();
		
		if (saveFormat > SAVE_FORMAT_OOPS && saveFormat < SAVE_FORMAT_TOTAL && player >= 0 && player < 3) {
			ImGui::CloseCurrentPopup();
			std::vector<BYTE> data;
			switch (saveFormat) {
				case SAVE_FORMAT_COLLISION_FILE:
					makeFileSelectRequest(&UI::serializeCollision, L"COLLISION file (*.collision)\0*.collision\0", player, false);
					break;
				case SAVE_FORMAT_JSON_FILE:
					makeFileSelectRequest(&UI::serializeJson, L"JSON file (*.json)\0*.json\0", player, true);
					break;
				case SAVE_FORMAT_JSON_TO_CLIPBOARD:
					serializeJson(data, player);
					writeOutClipboard(data);
					break;
			}
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Load")) {
		ImGui::OpenPopup("Select Location to Load From");
	}
	AddTooltip("Load collision data from a .collision file or .json file, or get json data from clipboard (similar to paste, but you have to press this button)."
		" Upon pressing this button, you will be asked to select where to get the data from. The choice will be presented as a table with 3 columns and 2 rows."
		" The columns are P1, P2 and Common, and rows are \"From file\" or \"JSON from clipboard\". There's also a \"Fom file (Autodetect P1/P2/Common)\" at"
		" the top. If you press the autodetect button, you will be asked to provide a file, and the program will attempt to determine whose collision data that"
		" is based on the file name. If the file name is, for example, sol.collision or sol.json, it will find the player that is a Sol and load the data into"
		" him. If the file name is weird and it's impossible to determine like that, you will get an error message saying it couldn't autodetect, and you'll have"
		" to click this button again and specify \"From file\" from one of the columns to tell the program explicitly whose data you're going to load.\n\n"
		"If you select \"From file\" in one of the columns, see right above what I just described.\n"
		"The program automatically determines whether the file is a JSON or a .collision file based on its contents.\n\n"
		"If you select \"JSON from clipboard\", your need to have copied a JSON text into your clipboard previously (that means selecting some JSON"
		" text and pressing Ctrl+C, then pressing this button). The text will get read from the clipboard, it will be assumed to be JSON and not a .collision"
		" binary data. Whose data that is (P1's, P2's, or Common) will be decided based on which column of the table you selected \"JSON from clipboard\" from."
		" For example, if you picked it from the first column (P1), the data from the JSON will replace P1's collision data, and so on.");
	if (ImGui::BeginPopup("Select Location to Load From")) {
		
		ImGui::Separator();
		bool selected = false;
		if (ImGui::Selectable("From file (Autodetect P1/P2/Common)", &selected)) {
			if (selected) {
				ImGui::CloseCurrentPopup();
			}
			if (keyboard.thisProcessWindow) {
				PostMessageW(keyboard.thisProcessWindow, WM_APP_UI_REQUEST_FILE_SELECT_READ, (WPARAM)-1, NULL);
			}
		}
		ImGui::Separator();
		
		if (ImGui::BeginTable("##LoadFormat", 3, tableFlags)) {
			P1P2CommonTable table;
			ImGui::TableHeadersRow();
			
			for (int row = 0; row < 2; ++row) {
				ImGui::PushID(row);
				for (int column = 0; column < 3; ++column) {
					ImGui::PushID(column);
					ImGui::TableNextColumn();
					selected = false;
					if (row == 0) {
						if (ImGui::Selectable("From file", &selected)) {
							if (selected) {
								ImGui::CloseCurrentPopup();
								if (keyboard.thisProcessWindow) {
									PostMessageW(keyboard.thisProcessWindow, WM_APP_UI_REQUEST_FILE_SELECT_READ, (WPARAM)column, NULL);
								}
							}
						}
					} else {
						if (ImGui::Selectable("JSON from clipboard", &selected)) {
							if (selected) {
								ImGui::CloseCurrentPopup();
								readJsonFromClipboard(column);
							}
						}
					}
					ImGui::PopID();
				}
				ImGui::PopID();
			}
			
			ImGui::EndTable();
		}
		ImGui::EndPopup();
	}
	
	if (!gifMode.editHitboxes) return;
	
	bool editEntityChanged = false;
	Entity editEntity{gifMode.editHitboxesEntity};
	ImGui::TextUnformatted("Whom To Edit:");
	AddTooltip("The two numbers in parentheses next to each listed Projectile show:\n"
		" 1) The index of the Projectile in the game's internal Projectiles array;\n"
		" 2) The number of hitboxes of that Projectile.\n"
		"\n"
		"For each displayed Player, the one number in parentheses shows:\n"
		" 1) The number of hitboxes of that Player.");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(200.F);
	if (ImGui::BeginCombo("##whomToEdit", editEntity.ent ? getEditEntityRepr(editEntity) : nullptr)) {
		imguiContextMenuOpen = true;
		for (int i = 0; i < entityList.count; ++i) {
			Entity ent = entityList.list[i];
			ImGui::PushID(i);
			if (ImGui::Selectable(getEditEntityRepr(ent), editEntity == ent)) {
				gifMode.editHitboxesEntity = ent;
				selectedHitboxes.clear();
				editEntity = ent;
				editEntityChanged = true;
			}
			ImGui::PopID();
		}
		ImGui::EndCombo();
	}
	
	if (editEntityChanged) {
		restartHitboxEditMode();
	}
	
	if (!editEntity) return;
	FPACSecondaryData& secondaryData = hitboxEditorFPACSecondaryData[editEntity.bbscrIndexInAswEng()];
	FPAC* fpac = editEntity.fpac();
	if (!fpac || fpac->flag2() || !fpac->useHash()) return;
	
	parseAllSprites(editEntity);
	
	ImGui::TextUnformatted("Current Sprite:");
	const char* spriteTooltip = "Displays the current sprite. May be changed. You can use the mouse wheel to scroll the dropdown list up and down."
		" While the dropdown list is open, you can use the up and down arrow keys on the keyboard to select the"
		" previous or next sprite in the list.";
	AddTooltip(spriteTooltip);
	ImGui::SameLine();
	
	sprintf_s(strbuf, "%d", editEntity.spriteFrameCounterMax());
	ImVec2 frameCounterTextSize = ImGui::CalcTextSize(strbuf);
	frameCounterTextSize.x = frameCounterTextSize.x * 2.F + slashSize.x;
	
	float w = ImGui::GetContentRegionAvail().x
		- style.ItemSpacing.x
		// BeginCombo
		- style.ItemSpacing.x
		- frameCounterTextSize.x
		- style.ItemSpacing.x
		- addSpriteButtonSize.x  // add button
		- style.ItemSpacing.x
		- addSpriteButtonSize.x  // delete button
		- style.ItemSpacing.x
		- addSpriteButtonSize.x  // rename button
		;
	
	if (w < averageSpriteNameSize.x) {
		w = averageSpriteNameSize.x;
	}
	ImGui::SetNextItemWidth(w);
	
	ImGui::SameLine();
	SortedSprite* selectedSprite = nullptr;
	REDPawn* pawnWorld = editEntity.pawnWorld();
	
	const FName* currentAnimSeqName = &FName::nullFName;
	int maxFrame = -1;
	int currentFrame = -1;
	
	if (pawnWorld) {
		REDAnimNodeSequence* animSeq = pawnWorld->getFirstAnimSeq();
		if (animSeq) {
			currentAnimSeqName = &animSeq->AnimSeqName;
			maxFrame = animSeq->FrameMax;
			currentFrame = animSeq->CurrentFrame;
		}
	}
	
	SortedSprite* currentSpriteElement = hitboxEditFindCurrentSprite();
	
	char spriteRepr[128];
	strcpy_s(spriteRepr, getSpriteRepr(currentSpriteElement));
	
	if (ImGui::BeginCombo("##spriteName", spriteRepr)) {
		imguiContextMenuOpen = true;
		comboBoxExtension.onComboBoxBegin();
		
		comboBoxExtension.totalCount = secondaryData.sortedSprites.size();
		
		char* currentSpriteNameInLookupTable = (char*)fpac->findLookupEntry(editEntity.spriteName());
		
		int imguiID = 0;
		int sortedSpritesCount = (int)secondaryData.sortedSprites.size();
		SortedSprite* sortedSpritePtr = secondaryData.sortedSprites.data();
		for (int i = 0; i < sortedSpritesCount; ++i) {
			SortedSprite& sortedSprite = *sortedSpritePtr;
			++sortedSpritePtr;
			
			ImGui::PushID(imguiID++);
			bool isSelected = sortedSprite.name == currentSpriteNameInLookupTable;
			if (isSelected) {
				comboBoxExtension.selectedIndex = i;
			}
			if (ImGui::Selectable(sortedSprite.repr(), isSelected)) {
				selectedSprite = &sortedSprite;
			}
			ImGui::PopID();
		};
		
		if (comboBoxExtension.fastScrollWithKeys()) {
			selectedSprite = &secondaryData.sortedSprites[comboBoxExtension.selectedIndex];
		}
		
		ImGui::EndCombo();
	}
	if (ImGui::BeginItemTooltip()) {
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted("Current sprite:");
		ImGui::SameLine();
		ImGui::TextUnformatted(spriteRepr);
		ImGui::TextUnformatted(spriteTooltip);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
	
	ImGui::SameLine();
	sprintf_s(strbuf, "%d/%d", editEntity.spriteFrameCounter(), editEntity.spriteFrameCounterMax());
	ImGui::TextUnformatted(strbuf);
	AddTooltip("Current Sprite's frame-1/Max sprite's frames");
	
	ImGui::SameLine();
	if (drawIconButton("##newSpriteBtn", &addBtn)) {
		hitboxEditNewSpritePressed = true;
	}
	AddTooltipWithHotkeyNoSearch("Add Sprite.\n\nAdds a new sprite by copying the current one.", settings.hitboxEditAddSpriteHotkey);
	
	ImGui::SameLine();
	if (drawIconButton("##deleteSpriteBtn", &deleteBtn)) {
		hitboxEditDeleteSpritePressed = true;
	}
	AddTooltipWithHotkeyNoSearch("Delete/Undelete current sprite.\n\nMarks the current sprite for deletion, if it's not already deleted,"
		" and unmarks it for deletion, if it's already deleted. Deleted sprites will not be saved to disk into a .collision file.",
			settings.hitboxEditDeleteSpriteHotkey);
	
	ImGui::SameLine();
	if (drawIconButton("##renameSpriteBtn", &renameBtn)) {
		hitboxEditRenameSpritePressed = true;
	}
	AddTooltipWithHotkeyNoSearch("Rename current sprite.\n\nExisting bbscript will still refer to the sprite by old name, but into a .collision file"
		" it will be exported with the new name. When importing that .collision file back, old renamed sprites will remain in the game"
		" under their old name, and the new renamed sprites will be considered entirely new. Renaming sprites must go hand-in-hand"
		" with modifying the bbscript. Use Pangaea's ggxrd-mod to load both the bbscript and the .collision file during the loading screen,"
		" that's how you do all these changes properly.",
		settings.hitboxEditRenameSpriteHotkey);
	
	if (selectedSprite && endScene.spriteImpl) {
		endScene.spriteImpl((void*)editEntity.ent, selectedSprite->name, true);
		game.allowTickForActor(pawnWorld);
	}
	
	ImGui::TextUnformatted("Anim Sequence:");
	const char* animSequenceTooltip = "The name of the current animation sequence."
		" Changing this will modify which data is associated with the Current Sprite, until the round is restarted."
		" You can scroll the dropdown list using the mouse wheel, and, when the dropdown is open, you can use the"
		" up and down arrow keys to select the previous and the next item in the list.";
	AddTooltip(animSequenceTooltip);
	ImGui::SameLine();
	
	w = ImGui::GetContentRegionAvail().x
		- style.ItemSpacing.x * 2
		- frameNumTitleSize.x
		- longestFrameNumSize.x;
	ImGui::SetNextItemWidth(max(w, 100.F));
	
	const SortedAnimSeq* selectedAnimSeq = nullptr;
	static const SortedAnimSeq nullAnimSeq { { 0, 0 }, "None" };
	
	if (currentAnimSeqName && editEntity.hitboxes()->nameCount == 0) {
		strbuf[0] = '\0';
	} else {
		currentAnimSeqName->print(strbuf);
	}
	
	if (ImGui::BeginCombo("##animSequence", strbuf)) {
		imguiContextMenuOpen = true;
		comboBoxExtension.onComboBoxBegin();
		
		if (sortedAnimSeqs.empty()) {
			if (pawnWorld) {
				for (int meshInd = 0; meshInd < pawnWorld->MeshControlNum(); ++meshInd) {
					MeshControl& meshCtrl = pawnWorld->MeshControls()[meshInd];
					REDAnimNodeSequence* animSeq = meshCtrl.AnimSeq;
					if (animSeq) {
						USkeletalMeshComponent* skelComp = animSeq->SkelComponent;
						if (skelComp && skelComp->SkelMesh) {
							for (int animSetInd = 0; animSetInd < skelComp->AnimSets.ArrayNum; ++animSetInd) {
								UAnimSet* animSet = skelComp->AnimSets.Data[animSetInd];
								if (animSet) {
									for (int animSeqIndex = 0; animSeqIndex < animSet->Sequences.ArrayNum; ++animSeqIndex) {
										UAnimSequence* animSeq = animSet->Sequences.Data[animSeqIndex];
										if (animSeq) {
											const FName* fname = &animSeq->SequenceName;
											int insertIndex = findInsertionIndexUnique(sortedAnimSeqs, fname, fname->print(strbuf));
											if (insertIndex != -1) {
												SortedAnimSeq newSeq { *fname };
												strcpy(newSeq.buf, strbuf);
												sortedAnimSeqs.insert(sortedAnimSeqs.begin() + insertIndex, newSeq);
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
		comboBoxExtension.totalCount = sortedAnimSeqs.size() + 1  // add null anim seq
		;
		
		if (ImGui::Selectable(nullAnimSeq.buf, currentAnimSeqName->low == 0)) {
			selectedAnimSeq = &nullAnimSeq;
		}
		
		for (const SortedAnimSeq& elem : sortedAnimSeqs) {
			bool isSelected = *currentAnimSeqName == elem.fname;
			if (isSelected) {
				comboBoxExtension.selectedIndex = &elem - sortedAnimSeqs.data() + 1;
			}
			if (ImGui::Selectable(elem.buf, isSelected)) {
				selectedAnimSeq = &elem;
			}
		}
		
		if (comboBoxExtension.fastScrollWithKeys()) {
			selectedAnimSeq = &sortedAnimSeqs[comboBoxExtension.selectedIndex - 1];
		}
		
		ImGui::EndCombo();
	}
	AddTooltip(animSequenceTooltip);
	
	comboBoxExtension.endFrame();
	
	if (selectedAnimSeq
			&& endScene.spriteImpl
			&& !(selectedAnimSeq == &nullAnimSeq && editEntity.hitboxes()->nameCount)
			&& selectedAnimSeq->fname != *currentAnimSeqName) {
		SetAnimOperation newOp;
		newOp.fill(selectedAnimSeq->fname);
		performOp(&newOp);
	}
	
	int frameNum = currentFrame;
	if (currentFrame != -1 && maxFrame != -1) {
		ImGui::SameLine();
		sprintf_s(strbuf, "Frame/%d", maxFrame);
		ImGui::TextUnformatted(strbuf);
		if (ImGui::BeginItemTooltip()) {
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			sprintf_s(strbuf, "Maximum frame: %d", maxFrame);
			ImGui::TextUnformatted(strbuf);
			ImGui::TextUnformatted("Shows the current frame number out of the highest possible frame (inclusive)."
				" Frame numbers start from 0 and end on the Maximum frame, inclusively.");
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(longestFrameNumSize.x);
		if (ImGui::InputInt("##frameNum", &frameNum, 1, 1)) {
			if (frameNum > maxFrame) {
				frameNum = maxFrame;
			}
			if (frameNum < 0) frameNum = 0;
			if (frameNum != currentFrame && currentAnimSeqName && currentAnimSeqName->low && editEntity.hitboxes()->nameCount) {
				SetAnimOperation newOp;
				newOp.fill(*currentAnimSeqName, frameNum);
				performOp(&newOp);
			}
		}
		AddTooltip("Select the current frame of the Animation Sequence associated with this sprite using this field and '-', '+' buttons.\n"
			"The changes will take effect immediately, but will be lost on stage reload, unless saved to a .collision file.");
	}
	
	ImGui::TextUnformatted("Hitbox Type:");
	const char* hitboxTypeTooltip = "Choose color for each type of hitbox and whether to show hitboxes of that type."
		" The value displayed in () is the current number of hitboxes of that type.";
	AddTooltip(hitboxTypeTooltip);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100.F);
	sprintf_s(strbuf, "%s (%d)", hitboxTypeName[hitboxEditorCurrentHitboxType], editEntity.hitboxes()->count[hitboxEditorCurrentHitboxType]);
	static bool popupOpen = false;
	if (ImGui::BeginCombo("##hitboxType", strbuf)) {
		imguiContextMenuOpen = true;
		for (int i = 0; i < 17; ++i) {
			// no need for PushID as all names are different
			sprintf_s(strbuf, "%s (%d)", hitboxTypeName[i], editEntity.hitboxes()->count[i]);
			if (ImGui::Selectable(strbuf, i == hitboxEditorCurrentHitboxType)) {
				hitboxEditorCurrentHitboxType = (HitboxType)i;
			}
		}
		ImGui::EndCombo();
	}
	AddTooltip(hitboxTypeTooltip);
	
	ImGui::SameLine();
	HitboxListElement& currentHitboxType = settings.hitboxList[hitboxEditorCurrentHitboxType];
	ImVec4 colorVec = ARGBToVec(currentHitboxType.color);
	colorVec.w = 1.F;
	if (ImGui::ColorButton("Color", colorVec)) {
		memcpy(hitboxEditorRefCol, &colorVec, sizeof(float) << 2);
		ImGui::OpenPopup("picker");
	}
	ImGui::SameLine();
	bool isShow = currentHitboxType.show;
	if (ImGui::Checkbox("##Show", &isShow)) {
		currentHitboxType.show = isShow;
		needWriteSettings = true;
	}
	AddTooltip("Show this hitbox type.");
	
    if (ImGui::BeginPopup("picker"))
    {
    	float square_sz = ImGui::GetFrameHeight();
        ImGuiColorEditFlags picker_flags = ImGuiColorEditFlags_DisplayMask_ | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaPreviewHalf
        	| ImGuiColorEditFlags_NoAlpha;
        ImGui::SetNextItemWidth(square_sz * 12.0f);
        if (ImGui::ColorPicker4("##picker", (float*)&colorVec, picker_flags, hitboxEditorRefCol)) {
        	currentHitboxType.color = VecToRGB(colorVec);
        	needWriteSettings = true;
        }
        ImGui::EndPopup();
    }
	
	ImGui::SameLine();
	static float widthToBeConsumedByAllButtons;
	static bool widthToBeConsumedByAllButtonsInitialized = false;
	if (!widthToBeConsumedByAllButtonsInitialized) {
		widthToBeConsumedByAllButtonsInitialized = true;
		struct MyBtnSize {
			void add(const PinIcon& icon) {
				if (!isFirst) {
					result += spacing;
				} else {
					isFirst = false;
				}
				float iconWidth = icon.size.x;
				float widthUse;
				if (iconWidth < drawIconButton_minSize.x) {
					widthUse = drawIconButton_minSize.x;
				} else {
					widthUse = iconWidth;
				}
				result += padding + widthUse + padding;
			}
			float padding;
			float spacing;
			bool isFirst = true;
			float result = 0.F;
		} myBtnSize { style.FramePadding.x, style.ItemSpacing.x };
		myBtnSize.add(selectBtn);
		myBtnSize.add(rectBtn);
		myBtnSize.add(rect_deleteBtn);
		myBtnSize.add(undoBtn);
		myBtnSize.add(undoBtn);
		widthToBeConsumedByAllButtons = myBtnSize.result;
	}
	float remainingWidth = ImGui::GetContentRegionAvail().x;
	if (remainingWidth > widthToBeConsumedByAllButtons) {
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (remainingWidth - widthToBeConsumedByAllButtons));
	}
	if (drawIconButton("##selectionToolBtn", &selectBtn, hitboxEditorTool == HITBOXEDITTOOL_SELECTION)) {
		hitboxEditSelectionToolPressed = true;
	}
	AddTooltipWithHotkeyNoSearch("Selection Tool.\n\nUse the selection tool to select hitboxes by clicking and clicking and dragging the left"
			" mouse button. Holding Shift or Ctrl will add or remove boxes to/from the selection."
			" You can also resize hitboxes and move them with the mouse.",
			settings.hitboxEditSelectionToolHotkey);
	
	ImGui::SameLine();
	if (drawIconButton("##rectToolBtn", &rectBtn, hitboxEditorTool == HITBOXEDITTOOL_ADD_BOX)) {
		hitboxEditRectToolPressed = true;
	}
	AddTooltipWithHotkeyNoSearch("New Hitbox Tool.\n\nUse this tool to add a new hitbox by clicking and dragging the left mouse button."
			" Simply clicking will produce a point-sized hitbox, useful for certain types of hitboxes."
			" The type of the created hitbox will be decided by the value currently selected in the 'Hitbox Type' field.",
			settings.hitboxEditAddHitboxHotkey);
	
	ImGui::SameLine();
	if (drawIconButton("##rectDeleteBtn", &rect_deleteBtn)) {
		hitboxEditRectDeletePressed = true;
	}
	AddTooltipWithHotkeyNoSearch("Delete Selected Hitboxes.\n\nThis is not a tool, as in you can't toggle this on and off."
			" As soon as you press this button, all currently selected hitboxes will be deleted.",
			settings.hitboxEditDeleteSelectedHitboxesHotkey);
	
	ImGui::SameLine();
	if (drawIconButton("##undoBtn", &undoBtn, false, false, false, !undoRingBuffer[undoRingBufferIndex])) {
		hitboxEditUndoPressed = true;
	}
	if (ImGui::BeginItemTooltip()) {
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		int result = sprintf_s(strbuf, "Hotkey: %s", comborepr(settings.hitboxEditUndoHotkey));
		if (result != -1) {
			ImGui::TextUnformatted(strbuf, strbuf + result);
		}
		ImGui::Separator();
		if (undoRingBuffer[undoRingBufferIndex]) {
			sprintf_s(strbuf, "Undo '%s' operation.", undoOperationName[undoRingBuffer[undoRingBufferIndex]->type]);
			ImGui::TextUnformatted(strbuf);
		} else {
			ImGui::TextUnformatted("Undo.");
		}
		ImGui::Separator();
		ImGui::TextUnformatted("Pressing this button causes the last performed hitbox or sprite editing operation to be undone."
			" You can undo multiple operations by repeatedly pressing this, up to a certain limit.");
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
	
	ImGui::SameLine();
	if (drawIconButton("##redoBtn", &undoBtn, false, true, false, !redoRingBuffer[redoRingBufferIndex])) {
		hitboxEditRedoPressed = true;
	}
	if (ImGui::BeginItemTooltip()) {
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		int result = sprintf_s(strbuf, "Hotkey: %s", comborepr(settings.hitboxEditDeleteSelectedHitboxesHotkey));
		if (result != -1) {
			ImGui::TextUnformatted(strbuf, strbuf + result);
		}
		ImGui::Separator();
		if (redoRingBuffer[redoRingBufferIndex]) {
			sprintf_s(strbuf, "Redo '%s' operation.", undoOperationName[redoRingBuffer[redoRingBufferIndex]->type]);
			ImGui::TextUnformatted(strbuf);
		} else {
			ImGui::TextUnformatted("Redo.");
		}
		ImGui::Separator();
		ImGui::TextUnformatted("Pressing this button causes the last undone hitbox or sprite editing operation to be redone."
			" You can only perform a redo, if the last thing you did was an undo. If since the last undo you have performed some"
			" new operation, you will lose the ability to redo, until you perform an undo again."
			" You can redo multiple undone operatios, up to a certain extent.");
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
	
	ImVec2 contentRegionAvail = ImGui::GetContentRegionAvail();
	
	bool showPushbox = editEntity.showPushbox();
	
	if (currentSpriteElement) {
		drawHitboxEditorOnExit.checkClearSelectedHitboxes(currentSpriteElement);
		
		int overallHitboxIndex = 0;
		
		if (
			beginDragNDropFrame(
				currentSpriteElement,
				ImGui::BeginChild(
					"##layers",
					{ contentRegionAvail.x * 0.5F, max(contentRegionAvail.y, 50.F) },
					ImGuiChildFlags_Borders | ImGuiChildFlags_FrameStyle
				)
			)
		) {
			
			float oldHoverDelay = style.HoverDelayNormal;
			
			style.HoverDelayNormal *= 4.F;
			
			const char* dragNDropHelp =
				"Select an element to select that hitbox."
				" Shift- or Ctrl-select to add or remove that hitbox to/from the selection."
				" Drag-select to box-select multiple elements."
				" Hold Shift while drag-selecting to add elements to the selection via box-select."
				" Drag an already selected element or elements to move them, changing the order of the boxes."
				" An alternative way to change the order of the boxes is to set up hotkeys through the cogwheel"
				" at the top-right of the 'Hitbox Editor' window for the 'Arrange Hitboxes ...' commands.\n"
				"\n"
				"The eye icon means that hitbox type is visible. To make a hitbox type visible or invisible,"
				" go to 'Hitbox Type' field, select the hitbox type of interest and check or uncheck"
				" the checkbox on the right, denoting if it's visible.";
			
			bool shiftHeld = ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift);
			
			float windowWidth = ImGui::GetWindowWidth();
			float itemHeight = ImGui::GetTextLineHeightWithSpacing();
			
			ImGui::PushStyleVarY(ImGuiStyleVar_ItemSpacing, 0.F);
			
			DragNDropItemInfo itemInfo;
			itemInfo.topLayer = false;
			bool destinationDrawn = false;
			
			if (currentSpriteElement) {
				LayerIterator layerIterator(editEntity, currentSpriteElement);
				int layersCount = layerIterator.count();
				layerIterator.scrollToEnd();
				while (layerIterator.getPrev()) {
					if (!layerIterator.isPushbox || showPushbox) {
						int layerInd = layersCount - layerIterator.index - 1;
						bool selected = hitboxIsSelected(layerIterator.originalIndex);
						ImVec2 cursorPos;
						bool isCarried = dragNDropMouseDragPurpose == MOUSEDRAGPURPOSE_MOVE_ELEMENTS && selected;
						if (!isCarried) {
							if (dragNDropMouseDragPurpose == MOUSEDRAGPURPOSE_MOVE_ELEMENTS
									&& layerInd >= dragNDropDestinationIndex
									&& !destinationDrawn) {
								destinationDrawn = true;
								ImGui::InvisibleButton("##dragDestination", {
									1.F,
									itemHeight * (float)selectedHitboxes.size()
								});
								AddTooltipNoSharedDelay(dragNDropHelp);
							}
							cursorPos = ImGui::GetCursorPos();
							ImGui::PushID(layerInd);
							ImGui::InvisibleButton("##selElement", {
								windowWidth,
								itemHeight
							});
							AddTooltipNoSharedDelay(dragNDropHelp);
							ImGui::PopID();
						} else {
							cursorPos = ImGui::GetCursorPos();
						}
						itemInfo.color = ARGBToABGR(settings.hitboxList[layerIterator.type].color);
						itemInfo.hitboxIndex = layerIterator.subindex;
						itemInfo.hitboxType = layerIterator.type;
						itemInfo.isPushbox = layerIterator.isPushbox;
						itemInfo.layerIndex = layerInd;
						itemInfo.originalIndex = layerIterator.originalIndex;
						if (!isCarried) {
							itemInfo.x = cursorPos.x;
							itemInfo.y = cursorPos.y;
							itemInfo.active = ImGui::IsItemActive();
							itemInfo.hovered = dragNDropMouseDragPurpose == MOUSEDRAGPURPOSE_NONE && ImGui::IsItemHovered()
								|| dragNDropMouseDragPurpose == MOUSEDRAGPURPOSE_BOX_SELECT
								&& shiftHeld
								&& hitboxIsSelectedPreBoxSelect(layerIterator.originalIndex);
							itemInfo.selected = selected;
							dragNDropOnItem(currentSpriteElement, &itemInfo);
						} else {
							itemInfo.x = 0.F;
							itemInfo.y = 0.F;
							itemInfo.active = false;
							itemInfo.hovered = true;
							itemInfo.selected = false;
							dragNDropOnCarriedItem(currentSpriteElement, &itemInfo);
						}
					}
				}
			}
			
			ImVec2 contentRegionAvail = ImGui::GetContentRegionAvail();
			float cursorY = ImGui::GetCursorPosY();
			float remainingSpaceY = ImGui::GetWindowHeight() - ImGui::GetStyle().FramePadding.y - cursorY;
			if (remainingSpaceY >= 1.F) {
				ImGui::InvisibleButton("##layersEnd", {
					contentRegionAvail.x,
					remainingSpaceY
				});
				dragNDropOnItem(currentSpriteElement, nullptr);
				AddTooltipNoSharedDelay(dragNDropHelp);
			}
			
			ImGui::PopStyleVar();
			
			style.HoverDelayNormal = oldHoverDelay;
			
		}
		processDragNDrop(currentSpriteElement, overallHitboxIndex);
		ImGui::EndChild();
	}
	
	ImGui::SameLine();
	if (ImGui::BeginChild("##coordinates", { contentRegionAvail.x * 0.5F, max(contentRegionAvail.y, 50.F) },
					ImGuiChildFlags_Borders | ImGuiChildFlags_FrameStyle)) {
		int selectedCount = (int)selectedHitboxes.size();
		int pushboxIndex = pushboxOriginalIndex();
		if (selectedCount && pushboxIndex != -1 && hitboxIsSelected(pushboxIndex)) {
			--selectedCount;
		}
		if (!settings.hitboxEditDisplayRawCoordinates) {
			lastOverallSelectionBoxReady = false;
			DrawHitboxArrayCallParams params;
			convertedBoxes.clear();
			HitboxHolder* hitboxes = editEntity.hitboxes();
			Hitbox* hitboxesStart = hitboxes->hitboxesStart();
			if (selectedCount && !boxMouseDown) {
				editHitboxesFillParams(params, editEntity);
				editHitboxesConvertBoxes(params, editEntity, currentSpriteElement);
				convertedBoxesPrepared = false;
			}
			
			// ImGui text field has a buffer and holds onto some old value.
			// If you hold down the "+" button for example, and don't update the int value
			// supplied to the field, the field will try to increment from the value you supplied.
			// So you will never be able to increment the value using "+" like that, until you
			// actually update the int value and supply a new value to the field.
			// Meanwhile, all this time the field will be displaying a different value from its buffer,
			// and that value will already be incremented.
			// But that's beside the point. The user needs a way to resize boxes even if boxes did not react
			// to the resizing on the last frame (due to scaling, rounding error, floating point fuzziness, rotation, whatever).
			static int left = 0;
			static int top = 0;
			static int right = 0;
			static int bottom = 0;
			bool ready = lastOverallSelectionBoxReady && selectedCount && !boxMouseDown;
			if (!coordsEntity && ready || !ready) {
				left = lastOverallSelectionBox.bounds.left;
				top = lastOverallSelectionBox.bounds.top;
				right = lastOverallSelectionBox.bounds.right;
				bottom = lastOverallSelectionBox.bounds.bottom;
			}
			bool changed = false;
			static union {
				const char* asArray[4] {
					"Left:",
					"Top:",
					"Right:",
					"Bottom:"
				};
				struct {
					const char* left;
					const char* top;
					const char* right;
					const char* bottom;
				} asStruct;
			} u;
			static bool longestTitleArraySet = false;
			static ImVec2 longestTitleArraySize;
			if (!longestTitleArraySet) {
				longestTitleArraySet = true;
				for (int i = 0; i < _countof(u.asArray); ++i) {
					ImVec2 size = ImGui::CalcTextSize(u.asArray[i]);
					if (i == 0 || size.x > longestTitleArraySize.x) {
						longestTitleArraySize.x = size.x;
					}
				}
			}
			if (!ready) ImGui::PushStyleColor(ImGuiCol_Text, { 0.7F, 0.7F, 0.7F, 1.F });
			ImGui::TextUnformatted(u.asStruct.left);
			ImGui::SameLine();
			ImGui::SetCursorPosX(longestTitleArraySize.x + style.ItemSpacing.x);
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			if (ImGui::InputInt("##left", &left, 1000, 10000, !ready ? ImGuiInputTextFlags_ReadOnly : 0)) {
				changed = true;
			}
			imguiActiveTemp = imguiActiveTemp || ImGui::IsItemActive();
			ImGui::TextUnformatted(u.asStruct.top);
			ImGui::SameLine();
			ImGui::SetCursorPosX(longestTitleArraySize.x + style.ItemSpacing.x);
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			if (ImGui::InputInt("##top", &bottom, 1000, 10000, !ready ? ImGuiInputTextFlags_ReadOnly : 0)) {
				changed = true;
			}
			imguiActiveTemp = imguiActiveTemp || ImGui::IsItemActive();
			ImGui::TextUnformatted(u.asStruct.right);
			ImGui::SameLine();
			ImGui::SetCursorPosX(longestTitleArraySize.x + style.ItemSpacing.x);
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			if (ImGui::InputInt("##right", &right, 1000, 10000, !ready ? ImGuiInputTextFlags_ReadOnly : 0)) {
				changed = true;
			}
			imguiActiveTemp = imguiActiveTemp || ImGui::IsItemActive();
			ImGui::TextUnformatted(u.asStruct.bottom);
			ImGui::SameLine();
			ImGui::SetCursorPosX(longestTitleArraySize.x + style.ItemSpacing.x);
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			if (ImGui::InputInt("##bottom", &top, 1000, 10000, !ready ? ImGuiInputTextFlags_ReadOnly : 0)) {
				changed = true;
			}
			imguiActiveTemp = imguiActiveTemp || ImGui::IsItemActive();
			if (!ready) ImGui::PopStyleColor();
			if (changed && selectedCount && !boxMouseDown && ready) {
				resetKeyboardMoveCache();
				int hitboxCount = hitboxes->hitboxCount();
				if (coordsSprite != currentSpriteElement || coordsEntity != editEntity || boxResizeOldHitboxes.size() != hitboxCount) {
					coordsSprite = currentSpriteElement;
					coordsEntity = editEntity;
					
					boxResizeOldHitboxes.resize(hitboxCount);
					memcpy(boxResizeOldHitboxes.data(), hitboxesStart, boxResizeOldHitboxes.size() * sizeof (Hitbox));
					overallSelectionBoxOldBounds = lastOverallSelectionBox.bounds;
				}
				lastOverallSelectionBox.bounds.left = left;
				lastOverallSelectionBox.bounds.top = top;
				lastOverallSelectionBox.bounds.right = right;
				lastOverallSelectionBox.bounds.bottom = bottom;
				if (overallSelectionBoxOldBounds.left == overallSelectionBoxOldBounds.left) {
					boxSelectArenaXStart = overallSelectionBoxOldBounds.right;
					boxSelectArenaXEnd = lastOverallSelectionBox.bounds.right;
				} else {
					boxSelectArenaXStart = overallSelectionBoxOldBounds.left;
					boxSelectArenaXEnd = lastOverallSelectionBox.bounds.left;
				}
				if (overallSelectionBoxOldBounds.top == lastOverallSelectionBox.bounds.top) {
					boxSelectArenaYStart = overallSelectionBoxOldBounds.bottom;
					boxSelectArenaYEnd = lastOverallSelectionBox.bounds.bottom;
				} else {
					boxSelectArenaYStart = overallSelectionBoxOldBounds.top;
					boxSelectArenaYEnd = lastOverallSelectionBox.bounds.top;
				}
				boxResizePart = BOXPART_NONE;
				
				MoveResizeBoxesUndoHelper helper(hitboxesStart, hitboxCount, 60 * 10  /* 10 seconds */);
				
				boxResizeChangedSomething = false;
				for (BoxSelectBox& box : convertedBoxes) {
					if (hitboxIsSelected(box.originalIndex)) {
						boxResizeProcessBox(params, hitboxesStart, box.ptr);
					}
				}
				if (boxResizeChangedSomething) {
					helper.finish();
				}
			}
		} else if (selectedCount > 1) {
			ImGui::TextUnformatted("More than one box selected.\nPlease select only one box.");
		} else {
			static int x = 0;
			static int y = 0;
			static int width = 0;
			static int height = 0;
			bool changed = false;
			static union {
				const char* asArray[4] {
					"X:",
					"Y:",
					"Width:",
					"Height:"
				};
				struct {
					const char* x;
					const char* y;
					const char* width;
					const char* height;
				} asStruct;
			} u;
			Hitbox* ptr = nullptr;
			static bool longestTitleArraySet = false;
			static ImVec2 longestTitleArraySize;
			if (!longestTitleArraySet) {
				longestTitleArraySet = true;
				for (int i = 0; i < _countof(u.asArray); ++i) {
					ImVec2 size = ImGui::CalcTextSize(u.asArray[i]);
					if (i == 0 || size.x > longestTitleArraySize.x) {
						longestTitleArraySize.x = size.x;
					}
				}
			}
			if (selectedCount == 1) {
				int selectedHitbox;
				if (pushboxIndex == selectedHitboxes[0]) {
					selectedHitbox = selectedHitboxes[1];
				} else {
					selectedHitbox = selectedHitboxes[0];
				}
				LayerIterator layerIterator(editEntity, currentSpriteElement);
				while (layerIterator.getNext()) {
					if (selectedHitbox == layerIterator.originalIndex) {
						x = (int)layerIterator.ptr->offX;
						y = (int)layerIterator.ptr->offY;
						width = (int)layerIterator.ptr->sizeX;
						height = (int)layerIterator.ptr->sizeY;
						ptr = layerIterator.ptr;
						break;
					}
				}
			}
			bool ready = selectedCount == 1 && !boxMouseDown;
			if (!ready) ImGui::PushStyleColor(ImGuiCol_Text, { 0.7F, 0.7F, 0.7F, 1.F });
			ImGui::TextUnformatted(u.asStruct.x);
			ImGui::SameLine();
			ImGui::SetCursorPosX(longestTitleArraySize.x + style.ItemSpacing.x);
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			if (ImGui::InputInt("##x", &x, 1, 10, !ready ? ImGuiInputTextFlags_ReadOnly : 0)) {
				changed = true;
			}
			ImGui::TextUnformatted(u.asStruct.y);
			ImGui::SameLine();
			ImGui::SetCursorPosX(longestTitleArraySize.x + style.ItemSpacing.x);
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			if (ImGui::InputInt("##y", &y, 1, 10, !ready ? ImGuiInputTextFlags_ReadOnly : 0)) {
				changed = true;
			}
			ImGui::TextUnformatted(u.asStruct.width);
			ImGui::SameLine();
			ImGui::SetCursorPosX(longestTitleArraySize.x + style.ItemSpacing.x);
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			if (ImGui::InputInt("##width", &width, 1, 10, !ready ? ImGuiInputTextFlags_ReadOnly : 0)) {
				changed = true;
			}
			ImGui::TextUnformatted(u.asStruct.height);
			ImGui::SameLine();
			ImGui::SetCursorPosX(longestTitleArraySize.x + style.ItemSpacing.x);
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			if (ImGui::InputInt("##height", &height, 1, 10, !ready ? ImGuiInputTextFlags_ReadOnly : 0)) {
				changed = true;
			}
			if (!ready) ImGui::PopStyleColor();
			if (changed && ptr) {
				HitboxHolder* hitboxes = editEntity.hitboxes();
				MoveResizeBoxesUndoHelper helper(hitboxes->hitboxesStart(), hitboxes->hitboxCount(), 60 * 10 /* 10 seconds */ );
				
				ptr->offX = (float)x;
				ptr->offY = (float)y;
				ptr->sizeX = (float)width;
				ptr->sizeY = (float)height;
				convertedBoxesPrepared = false;
				
				helper.finish();
			}
		}
	}
	ImGui::EndChild();
	
}

void UI::onToggleHitboxEditMode() {
	if (gifMode.editHitboxes || !*aswEngine || !game.isTrainingMode()) {
		stopHitboxEditMode();
	} else {
		startHitboxEditMode();
	}
}

void UI::freezeGameAndAllowNextFrameControls() {
	
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
	
}

const char* getEditEntityRepr(Entity ent) {
	static char buf[3 + 32 + 14 + 1];
	if (ent.isPawn()) {
		sprintf_s(buf, "Player %d (%d)", ent.team() + 1, ent.hitboxes()->count[HITBOXTYPE_HITBOX]);
	} else {
		sprintf_s(buf, "P%d %s (%d) (%d)", ent.team() + 1, ent.animationName(), ent.getEffectIndex(), ent.hitboxes()->count[HITBOXTYPE_HITBOX]);
	}
	return buf;
}

void UI::showErrorDlg(const char* msg, bool isManual) {
	if (!msg) {
		errorMsgBuf.clear();
	} else {
		errorMsgBuf = msg;
	}
	if (errorMsgBuf.empty()) {
		errorDialogText = nullptr;
		return;
	}
	errorDialogText = errorMsgBuf.c_str();
	if (windowShowMode == WindowShowMode_Pinned) {
		windows[PinnedWindowEnum_Error].setPinned(true);
	}
	setOpen(PinnedWindowEnum_Error, true, isManual);
	if (isManual && windowShowMode == WindowShowMode_None) {
		windowShowMode = WindowShowMode_All;
		windows[PinnedWindowEnum_MainWindow].setOpen(true, false);
	}
	if (ImGui::WindowIsNotNull()) {
		ImVec2 cursorPos = ImGui::GetCursorPos();
		ImVec2 windowPos = ImGui::GetWindowPos();
		errorDialogPos[0] = cursorPos.x + windowPos.x;
		errorDialogPos[1] = cursorPos.y + windowPos.y;
	} else {
		errorDialogPos[0] = lastClickPosX;
		errorDialogPos[1] = lastClickPosY;
	}
	
	const ImVec2& displaySize = ImGui::GetIO().DisplaySize;
	if (errorDialogPos[0] > displaySize.x - 50.F) {
		errorDialogPos[0] -= 200.F;
	}
	
	if (errorDialogPos[1] > displaySize.y - 50.F) {
		errorDialogPos[1] -= 200.F;
	}
	
}

int UI::findInsertionIndexUnique(const std::vector<SortedAnimSeq>& ar, const FName* value, const char* str) {
	if (ar.empty()) return 0;
	size_t start = 0;
	size_t end = ar.size() - 1;
	const SortedAnimSeq* data = ar.data();
	const size_t size = ar.size();
	while (true) {
		size_t mid = (start + end) >> 1;
		const SortedAnimSeq& midVal = data[mid];
		int cmpResult = strcmp(str, midVal.buf);
		if (cmpResult > 0) {
			start = mid + 1;
			if (start > end) {
				return (int)start;
			}
		} else if (cmpResult < 0) {
			if (mid == start) {
				return (int)start;
			}
			end = mid - 1;
		} else {
			return -1;
		}
	}
}

void UI::ComboBoxPopupExtension::onComboBoxBegin() {
	comboBoxBeganThisFrame = true;
	if (appearing && selectedIndex != -1 && totalCount) {
		float windowHeight = ImGui::GetWindowHeight();
		float scrollMaxY = ImGui::GetScrollMaxY();
		if (scrollMaxY > 0.001F) {
			float oneElementHeight = (windowHeight + scrollMaxY) / (float)totalCount;
			int numElementsFit = (int)std::roundf(windowHeight / oneElementHeight + 0.5F);
			if (selectedIndex < numElementsFit - 1 || totalCount <= (size_t)numElementsFit) {
				ImGui::SetScrollY(0.F);
			} else {
				int iOff = selectedIndex - numElementsFit + (numElementsFit >> 1);
				ImGui::SetScrollY(scrollMaxY * (float)iOff / (float)(totalCount - (size_t)numElementsFit));
			}
		}
	}
	selectedIndex = -1;
	appearing = ImGui::IsWindowAppearing();
}

bool UI::ComboBoxPopupExtension::fastScrollWithKeys() {
	if (selectedIndex != -1) {
		if (ImGui::IsKeyPressed(ImGuiKey_DownArrow, true)) {
			if (selectedIndex + 1 < (int)totalCount) {
				++selectedIndex;
				requestAutoScroll();
				return true;
			}
		} else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow, true)) {
			if (selectedIndex > 0) {
				--selectedIndex;
				requestAutoScroll();
				return true;
			}
		}
	}
	return false;
}

void UI::ComboBoxPopupExtension::endFrame() {
	if (!comboBoxBeganThisFrame) {
		appearing = false;
		selectedIndex = -1;
		totalCount = 0;
	}
}

bool drawIconButton(const char* buttonName, const PinIcon* icon, bool toolActive, bool flipHoriz, bool flipVert, bool disabled) {
	ImVec2 buttonPos = ImGui::GetCursorPos();
	const ImGuiStyle& style = ImGui::GetStyle();
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, style.Colors[ImGuiCol_Button]);
	
	ImVec2 sizeUse = icon->size;
	
	float padX = 0.F;
	float padY = 0.F;
	
	if (sizeUse.x < drawIconButton_minSize.x) {
		padX = std::floorf((drawIconButton_minSize.x - sizeUse.x) * 0.5F);
		sizeUse.x = drawIconButton_minSize.x;
	}
	if (sizeUse.y < drawIconButton_minSize.y) {
		padY = std::floorf((drawIconButton_minSize.y - sizeUse.y) * 0.5F);
		sizeUse.y = drawIconButton_minSize.y;
	}
	
	ImVec2 buttonSize {
		sizeUse.x + style.FramePadding.x * 2.F,
		sizeUse.y + style.FramePadding.y * 2.F
	};
	
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	
	ImVec2 windowPos = ImGui::GetWindowPos();
	float scrollY = ImGui::GetScrollY();
	
	if (toolActive) {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.19f, 0.30f, 0.56f, 0.75f));
		float outlineThickness = 4.F;
		ImVec2 buttonToolOutlineStart {
			std::floorf(windowPos.x + buttonPos.x - outlineThickness * 0.5F),
			std::floorf(windowPos.y - scrollY + buttonPos.y - outlineThickness * 0.5F)
		};
		drawList->AddRectFilled(buttonToolOutlineStart, {
			std::ceilf(buttonToolOutlineStart.x + buttonSize.x + outlineThickness),
			std::ceilf(buttonToolOutlineStart.y + buttonSize.y + outlineThickness)
		}, 0xFFFFFFFF, 0.F);
	}
	
	struct MyDarken {
		static ImVec4 darken(const ImVec4 src) {
			return { src.x * 0.7F, src.y * 0.7F, src.z * 0.7F, src.w };
		}
	};
	
	if (disabled) {
		ImVec4 colorOfTheFuture = MyDarken::darken(style.Colors[ImGuiCol_Button]);
		ImGui::PushStyleColor(ImGuiCol_Button, colorOfTheFuture);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, colorOfTheFuture);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colorOfTheFuture);
	}
	
	bool clicked = ImGui::Button(buttonName, buttonSize) && !disabled;
	bool isHovered = ImGui::IsItemHovered();
	bool mousePressed = isHovered && ImGui::IsMouseDown(ImGuiMouseButton_Left);
	
	if (disabled) {
		ImGui::PopStyleColor();
		ImGui::PopStyleColor();
		ImGui::PopStyleColor();
	}
	
	if (toolActive) {
		ImGui::PopStyleColor();
	}
	
	ImGui::PopStyleColor();
	
	ImVec2 uvStart;
	ImVec2 uvEnd;
	if (!flipHoriz && !flipVert) {
		uvStart = icon->uvStart;
		uvEnd = icon->uvEnd;
	} else if (!flipHoriz && flipVert) {
		uvStart.x = icon->uvStart.x;
		uvStart.y = icon->uvEnd.y;
		uvEnd.x = icon->uvEnd.x;
		uvEnd.y = icon->uvStart.y;
	} else if (flipHoriz && !flipVert) {
		uvStart.x = icon->uvEnd.x;
		uvStart.y = icon->uvStart.y;
		uvEnd.x = icon->uvStart.x;
		uvEnd.y = icon->uvEnd.y;
	} else if (flipHoriz && flipVert) {
		uvStart = icon->uvEnd;
		uvEnd = icon->uvStart;
	}
	ImVec2 imgStart {
		windowPos.x + buttonPos.x + style.FramePadding.x + padX,
		windowPos.y - scrollY + buttonPos.y + style.FramePadding.y + padY + (isHovered && mousePressed && !disabled ? 1.F : 0.F)
	};
	drawList->AddImage(TEXID_PIN,
		imgStart, {
			imgStart.x + icon->size.x,
			imgStart.y + icon->size.y
		}, uvStart, uvEnd, disabled ? 0xFFAAAAAA : 0xFFFFFFFF);
	if ((isHovered || mousePressed) && !disabled) {
		ImU32 tint = isHovered ? 0x50FFFFFF : 0x50000000;
		drawList->AddRectFilled({
			windowPos.x + buttonPos.x,
			windowPos.y - scrollY + buttonPos.y
		}, {
			windowPos.x + buttonPos.x + buttonSize.x,
			windowPos.y - scrollY + buttonPos.y + buttonSize.y
		}, tint);
	}
	
	return clicked;
}

void UI::detachFPAC() {
	for (FPACSecondaryData& data : hitboxEditorFPACSecondaryData) {
		data.detach();
	}
}

LayerIterator UI::getEntityLayers(Entity ent) {
	if (ent != gifMode.editHitboxesEntity || !gifMode.editHitboxes) {
		return { ent, nullptr };
	} else {
		return { ent, hitboxEditFindCurrentSprite(ent) };
	}
}

// called right before drawing all the ImGui::Selectables and the ImGui::InvisibleButton
bool UI::beginDragNDropFrame(SortedSprite* currentSpriteElement, bool childBegan) {
	dragNDropBeganFrame = true;
	dragNDropChildBegan = childBegan;
	if (!dragNDropChildBegan) {
		ImVec2 windowPos = ImGui::GetCursorPos();
		dragNDropChildNotBegan_mouseAboveWholeWindow = ImGui::GetIO().MousePos.y < ImGui::GetCursorPosY();
		dragNDropChildNotBegan_x = windowPos.x;
		dragNDropChildNotBegan_y = windowPos.y;
	}
	dragNDropMouseIndex = -1;
	dragNDropItems.clear();
	dragNDropItemsCarried.clear();
	dragNDropItemHeight = ImGui::GetTextLineHeightWithSpacing();
	dragNDropMouseDragPurposePending = MOUSEDRAGPURPOSE_NONE;
	selectedHitboxesBoxSelect.clear();
	return childBegan;
}

// called after drawing ImGui::InvisibleButton
// The index is -1 for the final ImGui::InvisibleButton that corresponds to the empty space
void UI::dragNDropOnCarriedItem(SortedSprite* currentSpriteElement, DragNDropItemInfo* itemInfo) {
	dragNDropItemsCarried.push_back(*itemInfo);
}
	
void UI::dragNDropOnItem(SortedSprite* currentSpriteElement, DragNDropItemInfo* itemInfo) {
	
	if (dragNDropMouseDragPurpose == MOUSEDRAGPURPOSE_MOVE_ELEMENTS && itemInfo) {
		ImVec2 mousePos = ImGui::GetIO().MousePos;
		ImVec2 windowPos = ImGui::GetWindowPos();
		float scrollY = ImGui::GetScrollY();
		if (mousePos.y >= itemInfo->y + (windowPos.y - scrollY) + dragNDropItemHeight * 0.5F) {
			dragNDropMouseIndex = itemInfo->layerIndex + 1;
		}
	}
	
	if (itemInfo) {
		dragNDropItems.push_back(*itemInfo);
	}
	if (!dragNDropActive && (itemInfo ? itemInfo->active : ImGui::IsItemActive())) {
		dragNDropActivePending = true;
		dragNDropActivePendingInfoIsNull = itemInfo == nullptr;
		if (itemInfo) {
			dragNDropActivePendingInfo = *itemInfo;
		}
	}
	
	if (itemInfo && dragNDropMouseDragPurpose == MOUSEDRAGPURPOSE_BOX_SELECT && boxSelectYSet) {
		if (itemInfo->y < boxSelectYEnd
				&& itemInfo->y + dragNDropItemHeight >= boxSelectYStart) {
			selectedHitboxesBoxSelect.push_back(itemInfo->originalIndex);
			dragNDropItems.back().hovered = true;
		}
	}
	
}

// called after all ImGui::InvisibleButton have been drawn
void UI::processDragNDrop(SortedSprite* currentSpriteElement, int overallHitboxCount) {
	if (!dragNDropBeganFrame) return;
	dragNDropBeganFrame = false;
	const ImGuiIO& io = ImGui::GetIO();
	bool mouseDown = io.MouseDown[0];
	if (dragNDropActive && dragNDropMouseDragPurpose == MOUSEDRAGPURPOSE_NONE && !mouseDown) {
		bool wasSelected = false;
		auto itEnd = selectedHitboxes.end();
		std::vector<int>::iterator it;
		if (dragNDropActiveIndex != -1) {
			for (it = selectedHitboxes.begin(); it != itEnd; ++it) {
				if (*it == dragNDropActiveIndex) {
					wasSelected = true;
					break;
				}
			}
		}
		if (dragNDropWasShiftHeld || dragNDropWasCtrlHeld) {
			resetKeyboardMoveCache();
			resetCoordsMoveCache();
			if (wasSelected) {
				selectedHitboxes.erase(it);
			} else {
				selectedHitboxes.push_back(dragNDropActiveIndex);
			}
		} else if (!(
			selectedHitboxes.size() == 1 && selectedHitboxes[0] == dragNDropActiveIndex
		)) {
			resetKeyboardMoveCache();
			resetCoordsMoveCache();
			selectedHitboxes.clear();
			if (dragNDropActiveIndex != -1) {
				selectedHitboxes.push_back(dragNDropActiveIndex);
			}
		}
	}
	
	
	if (dragNDropActive && dragNDropMouseDragPurpose == MOUSEDRAGPURPOSE_MOVE_ELEMENTS && mouseDown) {
		if (!dragNDropChildBegan) {
			if (dragNDropChildNotBegan_mouseAboveWholeWindow) {
				dragNDropMouseIndex = 0;
			} else {
				dragNDropMouseIndex = overallHitboxCount;
			}
		}
		if (dragNDropMouseIndex < 0) dragNDropMouseIndex = 0;
		if (dragNDropDestinationIndex != dragNDropMouseIndex) {
			dragNDropInterpolationTimer = dragNDropInterpolationTimerMax;
			dragNDropDestinationIndex = dragNDropMouseIndex;
		}
	}
	
	ImVec2 windowPos = ImGui::GetWindowPos();
	float windowWidth = ImGui::GetWindowWidth();
	float windowHeight = ImGui::GetWindowHeight();
	float scrollY = ImGui::GetScrollY();
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	ImVec4 clipRect = drawList->_CmdHeader.ClipRect;
	
	float itemHeight = ImGui::GetTextLineHeightWithSpacing();
	
	ImGuiStyle& style = ImGui::GetStyle();
	
	float fontSize = ImGui::GetFontSize();
	
	ImDrawList* foregroundDrawList = ImGui::GetForegroundDrawList();
	
	if (dragNDropChildBegan) {
		
		for (DragNDropItemInfo& info : dragNDropItems) {
			ImVec2 infoStart {
				info.x,
				info.y
			};
			bool needTopLayer = false;
			if (dragNDropInterpolationTimer != 0) {
				bool prevPosFound = false;
				ImVec2 prevPos;
				for (int prevIndex = 0; prevIndex < (int)prevDragNDropItems.size(); ++prevIndex) {
					PrevDragNDropItemInfo& prevInfo = prevDragNDropItems[prevIndex];
					if (prevInfo.originalIndex == info.originalIndex) {
						prevPos.x = prevInfo.x;
						prevPos.y = prevInfo.y;
						prevPosFound = true;
						needTopLayer = prevInfo.topLayer;
						break;
					}
				}
				if (prevPosFound) {
					ImVec2 dist {
						infoStart.x - prevPos.x,
						infoStart.y - prevPos.y
					};
					if (!dragNDropInterpolationAnimReady) {
						dragNDropInterpolationAnimReady = true;
						for (int animInd = dragNDropInterpolationTimerMax - 1; animInd >= 0; --animInd) {
							int sum = 0;
							for (int animIndOther = 0; animIndOther <= animInd; ++animIndOther) {
								sum += dragNDropInterpolationAnimRaw[animIndOther];
							}
							dragNDropInterpolationAnim[animInd] = (float)dragNDropInterpolationAnimRaw[animInd] / (float)sum;
						}
					}
					float distCoeff = dragNDropInterpolationAnim[dragNDropInterpolationTimer - 1];
					ImVec2 speed {
						(infoStart.x - prevPos.x) * distCoeff,
						(infoStart.y - prevPos.y) * distCoeff
					};
					if (dist.x < 0.F ? speed.x < dist.x : speed.x > dist.x) speed.x = dist.x;
					if (dist.y < 0.F ? speed.y < dist.y : speed.y > dist.y) speed.y = dist.y;
					infoStart.x = prevPos.x + speed.x;
					infoStart.y = prevPos.y + speed.y;
				}
			}
			info.topLayer = needTopLayer;
			info.drawX = infoStart.x;
			info.drawY = infoStart.y;
			infoStart.x = std::roundf(infoStart.x + windowPos.x - style.FramePadding.x);
			infoStart.y = std::roundf(infoStart.y + windowPos.y - scrollY - style.FramePadding.y);
			ImVec2 infoEnd {
				infoStart.x + windowWidth,
				infoStart.y + itemHeight
			};
			if (infoStart.x < clipRect.z && infoEnd.x >= clipRect.x
					&& infoStart.y < clipRect.w && infoEnd.y >= clipRect.y) {
				dragNDropDrawItem(&info,
					infoStart.x,
					infoStart.y,
					windowWidth,
					settings.hitboxList[info.hitboxType].show ? Moves::TriBool::TRIBOOL_TRUE : Moves::TriBool::TRIBOOL_FALSE,
					needTopLayer ? (BYTE*)foregroundDrawList : (BYTE*)drawList);
			}
		}
	}
	if (dragNDropInterpolationTimer > 0) --dragNDropInterpolationTimer;
	prevDragNDropItems.clear();
	for (const DragNDropItemInfo& item : dragNDropItems) {
		prevDragNDropItems.emplace_back();
		PrevDragNDropItemInfo& newElem = prevDragNDropItems.back();
		newElem.x = item.drawX;
		newElem.y = item.drawY;
		newElem.originalIndex = item.originalIndex;
		newElem.topLayer = item.topLayer;
	}
	
	if (dragNDropMouseDragPurpose == MOUSEDRAGPURPOSE_MOVE_ELEMENTS) {
		float largestTextWidth = 0.F;
		for (DragNDropItemInfo& elem : dragNDropItemsCarried) {
			ImVec2 textSize = ImGui::CalcTextSize(elem.printLabel());
			if (textSize.x > largestTextWidth) {
				largestTextWidth = textSize.x;
			}
		}
		ImVec2 allItemsSize {
			style.FramePadding.x + fontSize + style.ItemSpacing.x + largestTextWidth + style.FramePadding.x,
			style.FramePadding.y + (fontSize + style.ItemSpacing.y) * (float)dragNDropItemsCarried.size() + style.FramePadding.y
		};
		ImVec2 topLeftPos {
			std::roundf(io.MousePos.x - allItemsSize.x * 0.5F),
			std::roundf(io.MousePos.y - allItemsSize.y * 0.5F)
		};
		for (DragNDropItemInfo& elem : dragNDropItemsCarried) {
			dragNDropDrawItem(&elem,
				topLeftPos.x,
				topLeftPos.y,
				allItemsSize.x,
				Moves::TRIBOOL_DUNNO,
				(BYTE*)foregroundDrawList);
			
			prevDragNDropItems.emplace_back();
			PrevDragNDropItemInfo& newElem = prevDragNDropItems.back();
			newElem.x = topLeftPos.x - windowPos.x + style.FramePadding.x;
			newElem.y = topLeftPos.y - (windowPos.y - scrollY) + style.FramePadding.y;
			newElem.originalIndex = elem.originalIndex;
			newElem.topLayer = true;
			
			topLeftPos.y += itemHeight;
		}
	}
	
	if (!mouseDown) {
		Entity editEntity = gifMode.editHitboxesEntity;
		if (dragNDropMouseDragPurpose == MOUSEDRAGPURPOSE_MOVE_ELEMENTS && currentSpriteElement && editEntity
				&& !dragNDropOperationWontDoAnything(currentSpriteElement)) {
			ReorderLayersOperation newOp;
			newOp.fill(dragNDropDestinationIndex);
			performOp(&newOp);
		}
		endDragNDrop();
	}
	
	if (dragNDropMouseDragPurpose == MOUSEDRAGPURPOSE_BOX_SELECT && dragNDropChildBegan) {
		ImGuiIO& io = ImGui::GetIO();
		ImVec2 windowPos = ImGui::GetWindowPos();
		float scrollY = ImGui::GetScrollY();
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 boxSelectGraphicStartPos {
			windowPos.x + dragNDropMouseClickedX,
			windowPos.y - scrollY + dragNDropMouseClickedY
		};
		drawList->AddRectFilled(boxSelectGraphicStartPos, io.MousePos, 0x66FF6688, 0.F, 0);
		drawList->AddRect(boxSelectGraphicStartPos, io.MousePos, 0xFFFF99AA, 0.F, 0);
		boxSelectYSet = true;
		boxSelectYStart = dragNDropMouseClickedY;
		boxSelectYEnd = io.MousePos.y - (windowPos.y - scrollY);
		if (boxSelectYStart > boxSelectYEnd) {
			std::swap(boxSelectYStart, boxSelectYEnd);
		}
		resetKeyboardMoveCache();
		resetCoordsMoveCache();
		selectedHitboxes.clear();
		if (ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift)) {
			for (int i : selectedHitboxesPreBoxSelect) {
				selectedHitboxes.push_back(i);
			}
		}
		for (int i : selectedHitboxesBoxSelect) {
			bool found = false;
			for (int j : selectedHitboxes) {
				if (i == j) {
					found = true;
					break;
				}
			}
			if (!found) {
				selectedHitboxes.push_back(i);
			}
		}
	}
	
	if (dragNDropActivePending) {
		dragNDropActivePending = false;
		resetKeyboardMoveCache();
		resetCoordsMoveCache();
		ImGuiIO& io = ImGui::GetIO();
		ImVec2 windowPos = ImGui::GetWindowPos();
		float scrollY = ImGui::GetScrollY();
		dragNDropMouseClickedX = io.MouseClickedPos[0].x - windowPos.x;
		dragNDropMouseClickedY = io.MouseClickedPos[0].y - (windowPos.y - scrollY);
		dragNDropActiveIndex = dragNDropActivePendingInfoIsNull ? -1 : dragNDropActivePendingInfo.originalIndex;
		dragNDropActive = true;
		dragNDropActiveIndexWasSelected = dragNDropActivePendingInfoIsNull ? false : dragNDropActivePendingInfo.selected;
		dragNDropWasShiftHeld = ImGui::IsKeyDown(ImGuiKey_LeftShift)
			|| ImGui::IsKeyDown(ImGuiKey_RightShift);
		dragNDropWasCtrlHeld = ImGui::IsKeyDown(ImGuiKey_LeftCtrl)
			|| ImGui::IsKeyDown(ImGuiKey_RightCtrl);
	}
	
	if (dragNDropMouseDragPurpose == MOUSEDRAGPURPOSE_NONE && dragNDropActive && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 3.F)) {
		if (dragNDropActiveIndexWasSelected) {
			dragNDropMouseDragPurposePending = MOUSEDRAGPURPOSE_MOVE_ELEMENTS;
			for (PrevDragNDropItemInfo& info : prevDragNDropItems){
				info.topLayer = false;
			}
		} else {
			dragNDropMouseDragPurposePending = MOUSEDRAGPURPOSE_BOX_SELECT;
			boxSelectYSet = false;
			selectedHitboxesPreBoxSelect = selectedHitboxes;
		}
	}
	
	if (dragNDropMouseDragPurposePending != MOUSEDRAGPURPOSE_NONE) {
		dragNDropMouseDragPurpose = dragNDropMouseDragPurposePending;
		if (dragNDropMouseDragPurpose == MOUSEDRAGPURPOSE_MOVE_ELEMENTS) {
			dragNDropInterpolationTimer = dragNDropInterpolationTimerMax;
		}
	}
}

// called when mod's UI is not visible,
// or the Hitbox Editor window is not visible,
// or hitbox editing mode is not on,
// or there is no entity specified, for which hitboxes are being edited,
// or the sprite of the entity changed from the last one,
// or the drawHitboxEditor function didn't get to the drawing layers part
void UI::endDragNDrop() {
	MouseDragPurpose prevMode = dragNDropMouseDragPurpose;
	dragNDropMouseDragPurpose = MOUSEDRAGPURPOSE_NONE;
	dragNDropMouseDragPurposePending = MOUSEDRAGPURPOSE_NONE;
	if (!dragNDropActive) return;
	dragNDropActive = false;
	dragNDropItemsCarried.clear();
	dragNDropActivePending = false;
	if (prevMode == MOUSEDRAGPURPOSE_MOVE_ELEMENTS) {
		dragNDropInterpolationTimer = dragNDropInterpolationTimerMax;
	} else if (prevMode == MOUSEDRAGPURPOSE_BOX_SELECT) {
		resetKeyboardMoveCache();
		resetCoordsMoveCache();
		selectedHitboxes.clear();
		if (ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift)) {
			for (int i : selectedHitboxesPreBoxSelect) {
				selectedHitboxes.push_back(i);
			}
		}
		for (int i : selectedHitboxesBoxSelect) {
			bool found = false;
			for (int j : selectedHitboxes) {
				if (i == j) {
					found = true;
					break;
				}
			}
			if (!found) {
				selectedHitboxes.push_back(i);
			}
		}
	}
}

bool UI::hitboxIsSelected(int index) const {
	for (int selectedIndex : selectedHitboxes) {
		if (index == selectedIndex) {
			return true;
		}
	}
	return false;
}

DWORD multiplyColor(DWORD colorLeft, DWORD colorRight) {
	DWORD leftOne = colorLeft & 0xFF;
	DWORD leftTwo = (colorLeft >> 8) & 0xFF;
	DWORD leftThree = (colorLeft >> 16) & 0xFF;
	DWORD leftFour = (colorLeft >> 24) & 0xFF;
	DWORD rightOne = colorRight & 0xFF;
	DWORD rightTwo = (colorRight >> 8) & 0xFF;
	DWORD rightThree = (colorRight >> 16) & 0xFF;
	DWORD rightFour = (colorRight >> 24) & 0xFF;
	DWORD result = 0;
	DWORD resultByte = leftOne * rightOne / 255;
	result |= resultByte;
	resultByte = leftTwo * rightTwo / 255;
	result |= resultByte << 8;
	resultByte = leftThree * rightThree / 255;
	result |= resultByte << 16;
	resultByte = leftFour * rightFour / 255;
	result |= resultByte << 24;
	return result;
}

const char* UI::DragNDropItemInfo::printLabel() const {
	if (isPushbox) {
		return "  PUSHBOX";
	} else {
		sprintf_s(strbuf, "  %s %d", hitboxTypeName[hitboxType], hitboxIndex + 1);
		return strbuf;
	}
}

void UI::dragNDropDrawItem(const DragNDropItemInfo* item, float x, float y, float width, Moves::TriBool eye, BYTE* drawListPtr) {
	ImVec4 color4;
	if (item->hovered) {
		if (ImGui::GetIO().MouseDown[0] && dragNDropMouseDragPurpose != MOUSEDRAGPURPOSE_BOX_SELECT) {
			color4 = ImVec4(0.f, 0.0f, 0.6f, 0.31f);
		} else {
			color4 = ImVec4(0.26f, 0.59f, 0.98f, 0.98f);
		}
	} else if (item->selected) {
		color4 = ImVec4(0.26f, 0.59f, 0.98f, 0.6f);
	} else {
		color4 = ImVec4(0.06f, 0.06f, 0.06f, 0.0f);
	}
	ImVec2 infoEnd {
		x + width,
		y + ImGui::GetTextLineHeightWithSpacing()
	};
	ImDrawList* drawList = (ImDrawList*)drawListPtr;
	if (color4.w > 0.001F) {
		drawList->AddRectFilled({ x, y }, infoEnd, ImGui::GetColorU32(color4), 0.F, 0);
	}
	
	const ImGuiStyle& style = ImGui::GetStyle();
	const float eyeSize = dragNDropItemHeight;
	float spaceForEye = 0.F;
	if (eye != Moves::TRIBOOL_DUNNO) {
		spaceForEye = eyeSize + 2.F;
		const PinIcon& eyeIcon = eye == Moves::TRIBOOL_TRUE ? eye_openBtn : eye_closedBtn;
		drawList->AddImage(TEXID_PIN,
			{ x + style.FramePadding.x, y + 1.F },
			{ x + style.FramePadding.x + eyeSize - 2.F, y + eyeSize - 2.F },
			eyeIcon.uvStart, eyeIcon.uvEnd);
	}
	
	drawList->AddText({
		x + style.FramePadding.x + spaceForEye,
		y + style.ItemSpacing.y * 0.5F
	}, 0xFFFFFFFF, item->printLabel(), 0);
	
	ImVec2 colorRectStart {
		x + style.FramePadding.x + spaceForEye,
		y + style.FramePadding.y
	};
	float fontSize = ImGui::GetFontSize();
	ImVec2 colorRectEnd {
		colorRectStart.x + fontSize,
		colorRectStart.y + fontSize
	};
	drawList->AddRectFilled(colorRectStart, colorRectEnd, item->color, 0.F, 0);
	drawList->AddRect(colorRectStart, colorRectEnd, multiplyColor(item->color, 0xFF7F7F7F) | 0xFF000000, 0.F, 0, 1.F);
}

bool UI::dragNDropOperationWontDoAnything(SortedSprite* currentSpriteElement) {
	int firstSelectedLayer = -1;
	if (!gifMode.editHitboxesEntity) return false;
	Entity editEntity{gifMode.editHitboxesEntity};
		
	LayerIterator layerIterator(editEntity, currentSpriteElement);
	layerIterator.scrollToEnd();
	bool prevSelected = false;
	int iReverse = 0;
	while (layerIterator.getPrev()) {
		bool selected = hitboxIsSelected(layerIterator.originalIndex);
		if (!prevSelected && selected && firstSelectedLayer != -1) {
			return false;
		}
		if (selected && firstSelectedLayer == -1) {
			firstSelectedLayer = iReverse;
		}
		prevSelected = selected;
		++iReverse;
	}
	
	return firstSelectedLayer == dragNDropDestinationIndex;
}

void UI::hitboxEditorBoxSelect() {
	
	if (!gifMode.editHitboxes || !gifMode.editHitboxesEntity) {
		boxMouseDown = false;
		boxSelectingBoxesDragging = false;
		return;
	}
	
	Entity editEntity{gifMode.editHitboxesEntity};
	SortedSprite* sortedSprite = hitboxEditFindCurrentSprite();
	
	DrawHitboxArrayCallParams params;
	editHitboxesFillParams(params, editEntity);
	
	if (!convertedBoxesPrepared && sortedSprite) {
		convertedBoxes.clear();
		editHitboxesConvertBoxes(params, editEntity, sortedSprite);
	}
	
	lastOverallSelectionBoxHoverPart = BOXPART_NONE;
	
	const ImGuiIO& io = ImGui::GetIO();
	bool isInSomeOtherWindow = ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow
				| ImGuiHoveredFlags_AllowWhenBlockedByActiveItem
				| ImGuiHoveredFlags_AllowWhenBlockedByPopup);
	
	if (isInSomeOtherWindow && !boxMouseDown) {
		boxHoverOriginalIndex = -1;
		boxHoverPart = BOXPART_NONE;
		return;
	}
	
	FPACSecondaryData& secondaryData = hitboxEditorFPACSecondaryData[editEntity.bbscrIndexInAswEng()];
	FPAC* fpac = secondaryData.Collision->TopData;
	
	if (dragNDropMouseDragPurpose == MOUSEDRAGPURPOSE_NONE
			&& dragNDropMouseDragPurposePending == MOUSEDRAGPURPOSE_NONE
			&& !isInSomeOtherWindow) {
		if (io.MouseClicked[0]) {
			resetKeyboardMoveCache();
			resetCoordsMoveCache();
			boxMouseDown = true;
			boxResizePart = BOXPART_NONE;
			boxSelectingBoxesDragging = false;
			selectedHitboxesPreBoxSelect = selectedHitboxes;
			selectedHitboxesBoxSelect.clear();
			boxSelectingOriginalIndex = -1;
			boxResizeHappened = false;
		}
	}
	
	if (boxMouseDown && boxResizePart != BOXPART_NONE && boxResizeSortedSprite != sortedSprite
			|| io.MousePos.x == -FLT_MAX || io.MousePos.y == -FLT_MAX) {
		boxMouseDown = false;
	}
	
	ImDrawList* drawList = ImGui::GetForegroundDrawList();
	EndScene::HitboxEditorCameraValues& vals = endScene.hitboxEditorCameraValues;
	if (vals.prepare(editEntity)) {
		
		boxSelectArenaXEnd = (int)(
			vals.cam_xASW + (io.MousePos.x - vals.vw * 0.5F) / vals.vw * vals.screenWidthASW
		);
		boxSelectArenaYEnd = (int)(
			vals.cam_zASW - (io.MousePos.y - vals.vh * 0.5F) / vals.vh * vals.screenHeightASW
		);
		
		if (boxMouseDown) {
			
			if (io.MouseClicked[0]) {
				boxSelectArenaXStartOrig = (int)(
					vals.cam_xASW + (io.MouseClickedPos[0].x - vals.vw * 0.5F) / vals.vw * vals.screenWidthASW
				);
				boxSelectArenaYStartOrig = (int)(
					vals.cam_zASW - (io.MouseClickedPos[0].y - vals.vh * 0.5F) / vals.vh * vals.screenHeightASW
				);
			}
			
			boxSelectArenaXStart = boxSelectArenaXStartOrig;
			boxSelectArenaYStart = boxSelectArenaYStartOrig;
			
			int xStartASW;
			int xEndASW;
			int yStartASW;
			int yEndASW;
			
			if (boxSelectArenaXStart < boxSelectArenaXEnd) {
				xStartASW = boxSelectArenaXStart;
				xEndASW = boxSelectArenaXEnd;
			} else {
				xEndASW = boxSelectArenaXStart;
				xStartASW = boxSelectArenaXEnd;
			}
			
			if (boxSelectArenaYStart < boxSelectArenaYEnd) {
				yStartASW = boxSelectArenaYStart;
				yEndASW = boxSelectArenaYEnd;
			} else {
				yEndASW = boxSelectArenaYStart;
				yStartASW = boxSelectArenaYEnd;
			}
			
			ImVec2 mouseStartImgui {
				((float)xStartASW - vals.cam_xASW) * vals.xCoeff * vals.vw * 0.5F + vals.vw * 0.5F,
				-((float)yStartASW - vals.cam_zASW) * vals.yCoeff * vals.vh * 0.5F + vals.vh * 0.5F
			};
			
			ImVec2 mouseEndImgui {
				((float)xEndASW - vals.cam_xASW) * vals.xCoeff * vals.vw * 0.5F + vals.vw * 0.5F,
				-((float)yEndASW - vals.cam_zASW) * vals.yCoeff * vals.vh * 0.5F + vals.vh * 0.5F
			};
			
			if (boxSelectingBoxesDragging && hitboxEditorTool == HITBOXEDITTOOL_SELECTION) {
				drawList->AddRectFilled(mouseStartImgui, mouseEndImgui, 0x66FF6688, 0.F, 0);
				drawList->AddRect(mouseStartImgui, mouseEndImgui, 0xFFFF99AA, 0.F, 0);
			}
			
		} else {
			
			boxSelectArenaXStart = boxSelectArenaXEnd;
			boxSelectArenaYStart = boxSelectArenaYEnd;
			
		}
		
		aswOneScreenPixelWidth = (int)std::ceilf(1.F / vals.vw * 2.F / vals.xCoeff);
		aswOneScreenPixelHeight = (int)std::ceilf(1.F / vals.vh * 2.F / vals.yCoeff);
		
	} else if (boxMouseDown) {
		if (io.MouseClicked[0]) {
			boxSelectArenaXStartOrig = 0;
			boxSelectArenaYStartOrig = 0;
		}
		boxSelectArenaXStart = 0;
		boxSelectArenaYStart = 0;
		boxSelectArenaXEnd = 0;
		boxSelectArenaYEnd = 0;
		
		if (boxSelectingBoxesDragging && hitboxEditorTool == HITBOXEDITTOOL_SELECTION) {
			drawList->AddRectFilled(io.MouseClickedPos[0], io.MousePos, 0x66FF6688, 0.F, 0);
			drawList->AddRect(io.MouseClickedPos[0], io.MousePos, 0xFFFF99AA, 0.F, 0);
		}
		aswOneScreenPixelWidth = 1250;
		aswOneScreenPixelHeight = 1250;
		
		if (hitboxEditorTool == HITBOXEDITTOOL_ADD_BOX) {
			// won't be able to add a box like this
			boxMouseDown = false;
		}
		
	}
	
	aswOneScreenPixelDiameter = max(aswOneScreenPixelWidth, aswOneScreenPixelHeight);
	boxHoverRequestedCursor = -1;
	boxHoverPart = BOXPART_NONE;
	boxHoverOriginalIndex = -1;
	
	if (boxSelectArenaXStart < boxSelectArenaXEnd) {
		boxSelectArenaXMin = boxSelectArenaXStart;
		boxSelectArenaXMax = boxSelectArenaXEnd;
	} else {
		boxSelectArenaXMax = boxSelectArenaXStart;
		boxSelectArenaXMin = boxSelectArenaXEnd;
	}
	
	if (boxSelectArenaYStart < boxSelectArenaYEnd) {
		boxSelectArenaYMin = boxSelectArenaYStart;
		boxSelectArenaYMax = boxSelectArenaYEnd;
	} else {
		boxSelectArenaYMax = boxSelectArenaYStart;
		boxSelectArenaYMin = boxSelectArenaYEnd;
	}
	
	
	if (boxMouseDown) {
		selectedHitboxesBoxSelect.clear();
	}
	
	bool shiftHeld = ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift)
		|| !boxSelectingBoxesDragging
		&& (
			ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl)
		);
	
	HitboxHolder* hitboxes = editEntity.hitboxes();
	Hitbox* hitboxesStart = hitboxes->hitboxesStart();
	bool showPushbox = editEntity.showPushbox();
	
	if (hitboxEditorTool == HITBOXEDITTOOL_SELECTION) {
		
		int selectedHitboxesCountWithoutPushbox = (int)selectedHitboxes.size();
		int _pushboxOriginalIndex = pushboxOriginalIndex();
		if (_pushboxOriginalIndex != -1 && hitboxIsSelected(_pushboxOriginalIndex)) {
			--selectedHitboxesCountWithoutPushbox;
		}
		if (selectedHitboxesCountWithoutPushbox > 1 && !(boxMouseDown && boxSelectingBoxesDragging)) {
			boxSelectProcessHitbox(lastOverallSelectionBox);
			if (boxHoverPart == BOXPART_MIDDLE) {
				boxHoverRequestedCursor = -1;
				boxHoverPart = BOXPART_NONE;
				boxHoverOriginalIndex = -1;
			} else {
				lastOverallSelectionBoxHoverPart = boxHoverPart;
			}
			selectedHitboxesBoxSelect.clear();
		}
		
		// the boxSelectProcessHitbox calls in this if's body fill in the following variables:
		//  BoxPart boxHoverPart. The hovered part of the hovered box. BOXPART_NONE if none.
		//  int boxHoverOriginalIndex. The identifier of the hovered box. -1 if none.
		//  ImGuiMouseCursor boxHoverRequestedCursor. The cursor to ask ImGui to set. -1 if none.
		//  std::vector<int> selectedHitboxesBoxSelect. The boxes that are getting selected.
		// We only call the boxSelectProcessHitbox's when mouse click first happens, or if box select is happening.
		if (params.params.scaleX && params.params.scaleY && sortedSprite
				&& (
					!boxMouseDown
					|| boxMouseDown
					&& (
						boxSelectingBoxesDragging
						|| io.MouseClicked[0]
					)
				)
				&& boxHoverPart == BOXPART_NONE
		) {
			for (BoxSelectBox& box : convertedBoxes) {
				boxSelectProcessHitbox(box);
			}
		}
		
		if (boxHoverOriginalIndex != -2 && selectedHitboxesCountWithoutPushbox > 1 && boxHoverPart != BOXPART_NONE) {
			boxHoverPart = BOXPART_MIDDLE;
			boxHoverRequestedCursor = ImGuiMouseCursor_ResizeAll;
		}
		
		// this is for when we did not call the boxSelectProcessHitbox's - the box you originally clicked must remain selected,
		// until you release the mouse. Unless you're doing box select
		if (boxMouseDown && !boxSelectingBoxesDragging && !io.MouseClicked[0]
				&& boxSelectingOriginalIndex != -1
				&& boxSelectingOriginalIndex != -2) {
			selectedHitboxesBoxSelect.push_back(boxSelectingOriginalIndex);
		}
		
		// clicked on a box
		if (io.MouseClicked[0] && boxMouseDown && boxHoverOriginalIndex != -1 && sortedSprite) {
			boxResizePart = boxHoverPart;
			boxResizeSortedSprite = sortedSprite;
			boxSelectingBoxesDragging = false;
			boxSelectingOriginalIndex = boxHoverOriginalIndex;
			// back up in case you're resizing or moving the boxes
			boxResizeOldHitboxes.resize(hitboxes->hitboxCount());
			memcpy(boxResizeOldHitboxes.data(), hitboxesStart, boxResizeOldHitboxes.size() * sizeof (Hitbox));
			// click on an unselected box? Select only it. The box should already be in selectedHitboxesBoxSelect, so no need to add it there
			if (boxHoverOriginalIndex != -2 && !hitboxIsSelected(boxHoverOriginalIndex) && !shiftHeld) {
				resetKeyboardMoveCache();
				resetCoordsMoveCache();
				selectedHitboxes.clear();
				selectedHitboxes.push_back(boxHoverOriginalIndex);
				selectedHitboxesPreBoxSelect.clear();
				selectedHitboxesPreBoxSelect.push_back(boxHoverOriginalIndex);
				convertedBoxes.clear();
				editHitboxesConvertBoxes(params, editEntity, sortedSprite);
			}
			overallSelectionBoxOldBounds = lastOverallSelectionBox.bounds;
			
		} else if (!(boxMouseDown && boxSelectingBoxesDragging) && boxHoverRequestedCursor != -1 && (!boxMouseDown || boxResizePart == BOXPART_NONE)) {
			ImGui::SetMouseCursor(boxHoverRequestedCursor);
		}
		
		if (boxMouseDown && boxResizePart != BOXPART_NONE) {
			ImGuiMouseCursor imguiCursor = (ImGuiMouseCursor)-1;
			switch (boxResizePart) {
				case BOXPART_MIDDLE: imguiCursor = ImGuiMouseCursor_ResizeAll; break;
				case BOXPART_TOPLEFT: imguiCursor = ImGuiMouseCursor_ResizeNESW; break;
				case BOXPART_TOPRIGHT: imguiCursor = ImGuiMouseCursor_ResizeNWSE; break;
				case BOXPART_BOTTOMLEFT: imguiCursor = ImGuiMouseCursor_ResizeNWSE; break;
				case BOXPART_BOTTOMRIGHT: imguiCursor = ImGuiMouseCursor_ResizeNESW; break;
				case BOXPART_TOP: imguiCursor = ImGuiMouseCursor_ResizeNS; break;
				case BOXPART_BOTTOM: imguiCursor = ImGuiMouseCursor_ResizeNS; break;
				case BOXPART_LEFT: imguiCursor = ImGuiMouseCursor_ResizeEW; break;
				case BOXPART_RIGHT: imguiCursor = ImGuiMouseCursor_ResizeEW; break;
			}
			if (imguiCursor != (ImGuiMouseCursor)-1) {
				ImGui::SetMouseCursor(imguiCursor);
			}
		}
		
		if (boxMouseDown) {
			resetKeyboardMoveCache();
			resetCoordsMoveCache();
			selectedHitboxes.clear();
			// holding shift or clicking on one of already selected boxes keeps all boxes selected (clicking on a not yet selected box is handled earlier)
			if (shiftHeld || boxResizePart != BOXPART_NONE) {
				for (int i : selectedHitboxesPreBoxSelect) {
					selectedHitboxes.push_back(i);
				}
			}
			for (int i : selectedHitboxesBoxSelect) {
				bool found = false;
				for (int j : selectedHitboxes) {
					if (i == j) {
						found = true;
						break;
					}
				}
				if (!found) {
					selectedHitboxes.push_back(i);
				}
			}
		}
		
	}
	
	// can only initiate box select if have not originated the click by clicking on a box
	if (boxMouseDown && boxResizePart == BOXPART_NONE && !boxSelectingBoxesDragging
				&& ImGui::IsMouseDragging(ImGuiMouseButton_Left, 3.F)) {
		boxSelectingBoxesDragging = true;
	}
	
	if (boxMouseDown && boxResizePart != BOXPART_NONE
			&& (
				boxSelectArenaXStart != boxSelectArenaXEnd
				|| boxSelectArenaYStart != boxSelectArenaYEnd
			) && sortedSprite) {
		int hitboxCount = hitboxes->hitboxCount();
		if ((int)boxResizeOldHitboxes.size() == hitboxCount) {
			
			boxResizeHappened = true;
			
			RECT& b = lastOverallSelectionBox.bounds;
			b = overallSelectionBoxOldBounds;
			
			switch (boxResizePart) {
				case BOXPART_MIDDLE:
					b.left += boxSelectArenaXEnd - boxSelectArenaXStart;
					b.top += boxSelectArenaYEnd - boxSelectArenaYStart;
					b.right += boxSelectArenaXEnd - boxSelectArenaXStart;
					b.bottom += boxSelectArenaYEnd - boxSelectArenaYStart;
					break;
				case BOXPART_TOPLEFT:
					b.left += boxSelectArenaXEnd - boxSelectArenaXStart;
					b.top += boxSelectArenaYEnd - boxSelectArenaYStart;
					break;
				case BOXPART_TOPRIGHT:
					b.right += boxSelectArenaXEnd - boxSelectArenaXStart;
					b.top += boxSelectArenaYEnd - boxSelectArenaYStart;
					break;
				case BOXPART_BOTTOMLEFT:
					b.left += boxSelectArenaXEnd - boxSelectArenaXStart;
					b.bottom += boxSelectArenaYEnd - boxSelectArenaYStart;
					break;
				case BOXPART_BOTTOMRIGHT:
					b.right += boxSelectArenaXEnd - boxSelectArenaXStart;
					b.bottom += boxSelectArenaYEnd - boxSelectArenaYStart;
					break;
				case BOXPART_LEFT:
					b.left += boxSelectArenaXEnd - boxSelectArenaXStart;
					break;
				case BOXPART_RIGHT:
					b.right += boxSelectArenaXEnd - boxSelectArenaXStart;
					break;
				case BOXPART_TOP:
					b.top += boxSelectArenaYEnd - boxSelectArenaYStart;
					break;
				case BOXPART_BOTTOM:
					b.bottom += boxSelectArenaYEnd - boxSelectArenaYStart;
					break;
			}
			
			
			MoveResizeBoxesUndoHelper helper(hitboxesStart, hitboxCount, 60 * 10  /* 10 seconds */);
			
			boxResizeChangedSomething = false;
			for (BoxSelectBox& box : convertedBoxes) {
				if (hitboxIsSelected(box.originalIndex)) {
					boxResizeProcessBox(params, hitboxesStart, box.ptr);
				}
			}
			if (boxResizeChangedSomething) {
				helper.finish();
			}
		}
	}
	
	bool escPressed = ImGui::IsKeyPressed(ImGuiKey_Escape);
	if (boxMouseDown && (!io.MouseDown[0] || escPressed)) {
		if (!escPressed) {
			if (hitboxEditorTool == HITBOXEDITTOOL_ADD_BOX) {
				AddBoxOperation newOp;
				newOp.fill(hitboxEditorCurrentHitboxType, params.params,
					boxSelectArenaXStart,
					boxSelectArenaYStart,
					boxSelectArenaXEnd,
					boxSelectArenaYEnd);
				performOp(&newOp);
			}
			if (!boxSelectingBoxesDragging && !boxResizeHappened && hitboxEditorTool == HITBOXEDITTOOL_SELECTION && shiftHeld) {
				for (auto i = selectedHitboxes.begin(); i != selectedHitboxes.end(); ) {
					bool foundPre = false;
					bool foundAfter = false;
					int index = *i;
					for (int j : selectedHitboxesPreBoxSelect) {
						if (index == j) {
							foundPre = true;
							break;
						}
					}
					for (int j : selectedHitboxesBoxSelect) {
						if (index == j) {
							foundAfter = true;
							break;
						}
					}
					if (foundPre && foundAfter) {
						i = selectedHitboxes.erase(i);
					} else {
						++i;
					}
				}
			}
			if (!boxSelectingBoxesDragging && hitboxEditorTool == HITBOXEDITTOOL_SELECTION && !shiftHeld
					&& boxResizePart != BOXPART_NONE && !boxResizeHappened) {
				selectedHitboxes = selectedHitboxesBoxSelect;
			}
		}
		boxMouseDown = false;
		boxSelectingBoxesDragging = false;
		boxResizePart = BOXPART_NONE;
	}
	
	if (boxMouseDown && boxSelectingBoxesDragging && hitboxEditorTool == HITBOXEDITTOOL_ADD_BOX) {
		DrawHitboxArrayCallParams::arenaToHitbox(params.params, boxSelectArenaXStart, boxSelectArenaYStart,
			boxSelectArenaXEnd, boxSelectArenaYEnd, params.data.data());
		endScene.forceFeedHitboxes(params);
	}
	
}

void UI::boxSelectProcessHitbox(BoxSelectBox& box) {
	RECT* rect = &box.bounds;
	
	bool isPoint = rect->right - rect->left < 1500
		|| rect->bottom - rect->top < 1500;
	
	unsigned int imprecision = (unsigned int)aswOneScreenPixelDiameter * 4;
	
	bool hit = false;
	
	if (!(boxMouseDown && boxSelectingBoxesDragging) && !isPoint && !box.isPushbox) {
		
		if (
			dist(
				boxSelectArenaXEnd,
				boxSelectArenaYEnd,
				rect->left,
				rect->top
			) <= imprecision
		) {
			boxHoverRequestedCursor = ImGuiMouseCursor_ResizeNESW;
			boxHoverPart = BOXPART_TOPLEFT;
			boxHoverOriginalIndex = box.originalIndex;
			hit = true;
		} else if (
			dist(
				boxSelectArenaXEnd,
				boxSelectArenaYEnd,
				rect->right,
				rect->top
			) <= imprecision
		) {
			boxHoverRequestedCursor = ImGuiMouseCursor_ResizeNWSE;
			boxHoverPart = BOXPART_TOPRIGHT;
			boxHoverOriginalIndex = box.originalIndex;
			hit = true;
		} else if (
			dist(
				boxSelectArenaXEnd,
				boxSelectArenaYEnd,
				rect->left,
				rect->bottom
			) <= imprecision
		) {
			boxHoverRequestedCursor = ImGuiMouseCursor_ResizeNWSE;
			boxHoverPart = BOXPART_BOTTOMLEFT;
			boxHoverOriginalIndex = box.originalIndex;
			hit = true;
		} else if (
			dist(
				boxSelectArenaXEnd,
				boxSelectArenaYEnd,
				rect->right,
				rect->bottom
			) <= imprecision
		) {
			boxHoverRequestedCursor = ImGuiMouseCursor_ResizeNESW;
			boxHoverPart = BOXPART_BOTTOMRIGHT;
			boxHoverOriginalIndex = box.originalIndex;
			hit = true;
		} else if (
			(unsigned int)abs(boxSelectArenaYEnd - rect->top) <= imprecision
			&& boxSelectArenaXEnd >= rect->left - (int)imprecision
			&& boxSelectArenaXEnd < rect->right + (int)imprecision
		) {
			boxHoverRequestedCursor = ImGuiMouseCursor_ResizeNS;
			boxHoverPart = BOXPART_TOP;
			boxHoverOriginalIndex = box.originalIndex;
			hit = true;
		} else if (
			(unsigned int)abs(boxSelectArenaYEnd - rect->bottom) <= imprecision
			&& boxSelectArenaXEnd >= rect->left - (int)imprecision
			&& boxSelectArenaXEnd < rect->right + (int)imprecision
		) {
			boxHoverRequestedCursor = ImGuiMouseCursor_ResizeNS;
			boxHoverPart = BOXPART_BOTTOM;
			boxHoverOriginalIndex = box.originalIndex;
			hit = true;
		} else if (
			(unsigned int)abs(boxSelectArenaXEnd - rect->left) <= imprecision
			&& boxSelectArenaYEnd >= rect->top - (int)imprecision
			&& boxSelectArenaYEnd < rect->bottom + (int)imprecision
		) {
			boxHoverRequestedCursor = ImGuiMouseCursor_ResizeEW;
			boxHoverPart = BOXPART_LEFT;
			boxHoverOriginalIndex = box.originalIndex;
			hit = true;
		} else if (
			(unsigned int)abs(boxSelectArenaXEnd - rect->right) <= imprecision
			&& boxSelectArenaYEnd >= rect->top - (int)imprecision
			&& boxSelectArenaYEnd < rect->bottom + (int)imprecision
		) {
			boxHoverRequestedCursor = ImGuiMouseCursor_ResizeEW;
			boxHoverPart = BOXPART_RIGHT;
			boxHoverOriginalIndex = box.originalIndex;
			hit = true;
		}
		
	}
	
	if (!hit
		&& (
			rect->left < boxSelectArenaXMax && rect->right >= boxSelectArenaXMin
			&& rect->top < boxSelectArenaYMax && rect->bottom >= boxSelectArenaYMin
			|| (
				isPoint
				&& dist(
					boxSelectArenaXEnd,
					boxSelectArenaYEnd,
					(rect->right + rect->left) >> 1,
					(rect->bottom + rect->top) >> 1
				) <= imprecision
			)
		)
	) {
		hit = true;
		if (!box.isPushbox) {
			boxHoverOriginalIndex = box.originalIndex;
			if (!(boxMouseDown && boxSelectingBoxesDragging)) {
				boxHoverRequestedCursor = ImGuiMouseCursor_ResizeAll;
				boxHoverPart = BOXPART_MIDDLE;
			}
		}
	}
	
	if (hit) {
		if (boxMouseDown) {
			if (!boxSelectingBoxesDragging && !box.isPushbox) selectedHitboxesBoxSelect.clear();
			selectedHitboxesBoxSelect.push_back(box.originalIndex);
		}
	}
}

bool UI::hitboxIsSelectedForEndScene(int originalIndex) const {
	return hitboxIsSelected(originalIndex);
}

// Algorithm taken from the Guilty Gear executable via reverse engineering! See BBScript instruction called 'calcDistance'
// COPYRIGHT OWNED BY ARCSYS (we have no permission to copy this algorithm!)
unsigned int UI::dist(int x1, int y1, int x2, int y2) {
	int distX = (x2 - x1) / 100;
	int distY = (y2 - y1) / 100;
	unsigned int distPow2 = distX * distX + distY * distY;
	if (distPow2 != 0) {
		unsigned int i = 1;
		unsigned int p = distPow2;
		if (distPow2 > 1) {
			do {
				i <<= 1;
				p >>= 1;
			} while (i < p);
		}
		do {
			p = i;
			// wait, if we're dividing by p IN A LOOP, how is this faster?
			i = (p + distPow2 / p) >> 1;
		} while (i < p);
		return p * 100;
	}
	return 0;
}

bool UI::hitboxIsSelectedPreBoxSelect(int originalIndex) const {
	for (int i : selectedHitboxesPreBoxSelect) {
		if (i == originalIndex) return true;
	}
	return false;
}

void UI::getMousePos(float* pos) const {
	const ImVec2& imguiPos = ImGui::GetIO().MousePos;
	pos[0] = imguiPos.x;
	pos[1] = imguiPos.y;
}

void UI::hitboxEditProcessBackground() {
	hitboxEditProcessKeyboardShortcuts();
	hitboxEditProcessPressedCommands();
}

void UI::hitboxEditProcessKeyboardShortcuts() {
	if (keyboard.gotPressed(settings.hitboxEditModeToggle)) {
		hitboxEditPressedToggleEditMode = true;
		hitboxEditPressedToggleEditMode_isOn = !gifMode.editHitboxes;
	}
	if (gifMode.editHitboxes && gifMode.editHitboxesEntity) {
		if (keyboard.gotPressed(settings.hitboxEditAddSpriteHotkey)) {
			hitboxEditNewSpritePressed = true;
		}
		if (keyboard.gotPressed(settings.hitboxEditDeleteSpriteHotkey)) {
			hitboxEditDeleteSpritePressed = true;
		}
		if (keyboard.gotPressed(settings.hitboxEditRenameSpriteHotkey)) {
			hitboxEditRenameSpritePressed = true;
		}
		if (keyboard.gotPressed(settings.hitboxEditSelectionToolHotkey)) {
			hitboxEditSelectionToolPressed = true;
		}
		if (keyboard.gotPressed(settings.hitboxEditAddHitboxHotkey)) {
			hitboxEditRectToolPressed = true;
		}
		if (keyboard.gotPressed(settings.hitboxEditDeleteSelectedHitboxesHotkey)) {
			hitboxEditRectDeletePressed = true;
		}
		if (keyboard.gotPressed(settings.hitboxEditUndoHotkey)) {
			hitboxEditUndoPressed = true;
		}
		if (keyboard.gotPressed(settings.hitboxEditRedoHotkey)) {
			hitboxEditRedoPressed = true;
		}
		if (keyboard.gotPressed(settings.hitboxEditArrangeHitboxesToBack)) {
			hitboxEditSendToBackPressed = true;
		}
		if (keyboard.gotPressed(settings.hitboxEditArrangeHitboxesBackwards)) {
			hitboxEditSendBackwardsPressed = true;
		}
		if (keyboard.gotPressed(settings.hitboxEditArrangeHitboxesUpwards)) {
			hitboxEditSendForwardsPressed = true;
		}
		if (keyboard.gotPressed(settings.hitboxEditArrangeHitboxesToFront)) {
			hitboxEditSendToFrontPressed = true;
		}
	}
}

void UI::hitboxEditProcessPressedCommands() {
	if (hitboxEditPressedToggleEditMode) {
		if (hitboxEditPressedToggleEditMode_isOn) {
			startHitboxEditMode();
		} else {
			stopHitboxEditMode();
		}
	}
	
	if (gifMode.editHitboxes && gifMode.editHitboxesEntity) {
		
		Entity editEntity = Entity{gifMode.editHitboxesEntity};
		
		REDPawn* pawnWorld = editEntity.pawnWorld();
		FPACSecondaryData& secondaryData = hitboxEditorFPACSecondaryData[editEntity.bbscrIndexInAswEng()];
		SortedSprite* currentSpriteElement = hitboxEditFindCurrentSprite();
		
		if (hitboxEditNewSpritePressed) {
			AddSpriteOperation newOp;
			newOp.fill();
			performOp(&newOp);
		}
		
		if (hitboxEditDeleteSpritePressed) {
			DeleteSpriteOperation newOp;
			newOp.fill();
			performOp(&newOp);
		}
		
		if (hitboxEditRenameSpritePressed) {
			if (!currentSpriteElement) {
				showErrorDlgS("Can't rename the 'null' sprite.");
			} else {
				ImGui::OpenPopup("Rename Sprite");
				const char* oldName = currentSpriteElement->newName[0] ? currentSpriteElement->newName : currentSpriteElement->name;
				memcpy(renameSpriteBuf, oldName, 32);
			}
		}
		
		if (hitboxEditSelectionToolPressed) {
			hitboxEditorTool = HITBOXEDITTOOL_SELECTION;
			boxMouseDown = false;
		}
		
		if (hitboxEditRectToolPressed) {
			hitboxEditorTool = HITBOXEDITTOOL_ADD_BOX;
			boxMouseDown = false;
		}
		
		if (hitboxEditRectDeletePressed && !selectedHitboxes.empty() && currentSpriteElement) {
			DeleteLayersOperation newOp;
			newOp.fill();
			performOp(&newOp);
		}
		
		if (hitboxEditUndoPressed) {
			ThreadUnsafeSharedPtr<UndoOperationBase> undoOp = undoRingBuffer[undoRingBufferIndex];
			if (undoOp) {
				undoRingBuffer[undoRingBufferIndex] = nullptr;
				--undoRingBufferIndex;
				if (undoRingBufferIndex < 0) {
					undoRingBufferIndex = (int)undoRingBuffer.size() - 1;
				}
				ThreadUnsafeSharedPtr<UndoOperationBase>& redoOp = allocateRedo();
				undoOp->restoreView();
				if (!undoOp->perform(&redoOp)) {
					--redoRingBufferIndex;
					if (redoRingBufferIndex < 0) {
						redoRingBufferIndex = (int)redoRingBuffer.size() - 1;
					}
				}
			}
		}
		
		if (hitboxEditRedoPressed) {
			ThreadUnsafeSharedPtr<UndoOperationBase> redoOp = redoRingBuffer[redoRingBufferIndex];
			if (redoOp) {
				redoRingBuffer[redoRingBufferIndex] = nullptr;
				--redoRingBufferIndex;
				if (redoRingBufferIndex < 0) {
					redoRingBufferIndex = (int)redoRingBuffer.size() - 1;
				}
				ThreadUnsafeSharedPtr<UndoOperationBase>& undoOp = allocateUndo();
				redoOp->restoreView();
				if (!redoOp->perform(&undoOp)) {
					--undoRingBufferIndex;
					if (undoRingBufferIndex < 0) {
						undoRingBufferIndex = (int)undoRingBuffer.size() - 1;
					}
				}
			}
		}
		
		if (hitboxEditSendToBackPressed && !selectedHitboxes.empty() && !selectedHitboxesAlreadyAtTheBottom(currentSpriteElement)) {
			currentSpriteElement->convertToLayers(&secondaryData);
			std::vector<EditedHitbox> newLayers(currentSpriteElement->layersSize);
			EditedHitbox* dstLayer = newLayers.data();
			for (int iteration = 0; iteration < 2; ++iteration) {
				EditedHitbox* srcLayer = currentSpriteElement->layers;
				for (int layerIndex = 0; layerIndex < (int)currentSpriteElement->layersSize; ++layerIndex) {
					if (hitboxIsSelected(srcLayer->originalIndex) == (iteration == 0)) {
						*dstLayer = *srcLayer;
						++dstLayer;
					}
					++srcLayer;
				}
			}
			UndoReorderLayersOperation newOp;
			newOp.fill(std::move(newLayers));
			performOp(&newOp);
		}
		if (hitboxEditSendBackwardsPressed && !selectedHitboxes.empty() && !selectedHitboxesAlreadyAtTheBottom(currentSpriteElement)) {
			currentSpriteElement->convertToLayers(&secondaryData);
			std::vector<EditedHitbox> newLayers(currentSpriteElement->layersSize);
			memcpy(newLayers.data(), currentSpriteElement->layers, currentSpriteElement->layersSize * sizeof (EditedHitbox));
			bool prevLayerSelected;
			if (!newLayers.empty()) {
				prevLayerSelected = hitboxIsSelected(newLayers.front().originalIndex);
			} else {
				prevLayerSelected = false;
			}
			EditedHitbox* layer = newLayers.data() + 1;
			for (int layerIndex = 1; layerIndex < (int)currentSpriteElement->layersSize; ++layerIndex) {
				bool thisLayerSelected = hitboxIsSelected(layer->originalIndex);
				if (thisLayerSelected && !prevLayerSelected) {
					std::swap(*layer, *(layer - 1));
					prevLayerSelected = false;
				} else {
					prevLayerSelected = thisLayerSelected;
				}
				++layer;
			}
			
			UndoReorderLayersOperation newOp;
			newOp.fill(std::move(newLayers));
			performOp(&newOp);
		}
		if (hitboxEditSendForwardsPressed && !selectedHitboxes.empty() && !selectedHitboxesAlreadyAtTheTop(currentSpriteElement)) {
			currentSpriteElement->convertToLayers(&secondaryData);
			std::vector<EditedHitbox> newLayers(currentSpriteElement->layersSize);
			memcpy(newLayers.data(), currentSpriteElement->layers, currentSpriteElement->layersSize * sizeof (EditedHitbox));
			bool prevLayerSelected;
			if (!newLayers.empty()) {
				prevLayerSelected = hitboxIsSelected(newLayers.back().originalIndex);
			} else {
				prevLayerSelected = false;
			}
			EditedHitbox* layer = newLayers.data() + (newLayers.size() - 2);
			for (int layerIndex = 1; layerIndex < (int)currentSpriteElement->layersSize; ++layerIndex) {
				bool thisLayerSelected = hitboxIsSelected(layer->originalIndex);
				if (thisLayerSelected && !prevLayerSelected) {
					std::swap(*layer, *(layer + 1));
					prevLayerSelected = false;
				} else {
					prevLayerSelected = thisLayerSelected;
				}
				--layer;
			}
			
			UndoReorderLayersOperation newOp;
			newOp.fill(std::move(newLayers));
			performOp(&newOp);
		}
		if (hitboxEditSendToFrontPressed && !selectedHitboxes.empty() && !selectedHitboxesAlreadyAtTheTop(currentSpriteElement)) {
			currentSpriteElement->convertToLayers(&secondaryData);
			std::vector<EditedHitbox> newLayers(currentSpriteElement->layersSize);
			EditedHitbox* dstLayer = newLayers.data() + (newLayers.size() - 1);
			for (int iteration = 0; iteration < 2; ++iteration) {
				EditedHitbox* srcLayer = currentSpriteElement->layers + (currentSpriteElement->layersSize - 1);
				for (int layerIndex = 0; layerIndex < (int)currentSpriteElement->layersSize; ++layerIndex) {
					if (hitboxIsSelected(srcLayer->originalIndex) == (iteration == 0)) {
						*dstLayer = *srcLayer;
						--dstLayer;
					}
					--srcLayer;
				}
			}
			UndoReorderLayersOperation newOp;
			newOp.fill(std::move(newLayers));
			performOp(&newOp);
		}
		
		if (ImGui::BeginPopup("Name Clash")) {
			popupsOpen = true;
			ImGui::PushTextWrapPos(0.F);
			sprintf_s(strbuf, "The entered name, '%s', has a hash that clashes with another name's hash. Please pick a different name.", renameSpriteBuf);
			ImGui::TextUnformatted(strbuf);
			ImGui::PopTextWrapPos();
			ImGui::EndPopup();
		}
		
		bool nameClash = false;
		if (ImGui::BeginPopup("Rename Sprite")) {
			popupsOpen = true;
			ImGui::TextUnformatted("New Name:");
			ImGui::SameLine();
			
			struct MyCallback {
				static int callback(ImGuiInputTextCallbackData* data) {
					if (data->EventFlag == ImGuiInputTextFlags_CallbackCharFilter) {
						if (data->EventChar <= 32 || data->EventChar > 126) {
							return 1;
						}
					}
					return 0;
				}
			};
			
			ImGui::InputText("##NewName", renameSpriteBuf, 32, ImGuiInputTextFlags_CallbackCharFilter, MyCallback::callback);
			imguiActiveTemp = imguiActiveTemp || ImGui::IsItemActive();
			if (ImGui::IsWindowAppearing()) {
				ImGui::SetKeyboardFocusHere(-1);
			}
			
			if (ImGui::Button("Save") || ImGui::IsKeyDown(ImGuiKey_Enter)) {
				if (currentSpriteElement) {
					RenameSpriteOperation newOp;
					int len = strlen(renameSpriteBuf);
					if (len < 32) memset(renameSpriteBuf + len, 0, 32 - len);
					newOp.fill(renameSpriteBuf);
					if (!performOp(&newOp)) nameClash = true;
				}
				ImGui::CloseCurrentPopup();
			}
			
			ImGui::EndPopup();
		}
		if (nameClash) {
			ImGui::OpenPopup("Name Clash");
		}
	}
	
	hitboxEditPressedToggleEditMode = false;
	hitboxEditNewSpritePressed = false;
	hitboxEditDeleteSpritePressed = false;
	hitboxEditRenameSpritePressed = false;
	hitboxEditSelectionToolPressed = false;
	hitboxEditRectToolPressed = false;
	hitboxEditRectDeletePressed = false;
	hitboxEditUndoPressed = false;
	hitboxEditRedoPressed = false;
	hitboxEditSendToBackPressed = false;
	hitboxEditSendBackwardsPressed = false;
	hitboxEditSendForwardsPressed = false;
	hitboxEditSendToFrontPressed = false;
}

SortedSprite* UI::hitboxEditFindCurrentSprite() {
	
	if (!gifMode.editHitboxes || !gifMode.editHitboxesEntity) return nullptr;
	
	Entity editEntity{gifMode.editHitboxesEntity};
	return hitboxEditFindCurrentSprite(editEntity);
}

SortedSprite* UI::hitboxEditFindCurrentSprite(Entity ent) {
	FPACSecondaryData& secondaryData = hitboxEditorFPACSecondaryData[ent.bbscrIndexInAswEng()];
	
	DWORD currentSpriteHash = Entity::hashStringLowercase(ent.spriteName());
	SortedSprite* currentSpriteElement;
	auto currentSpriteIt = secondaryData.oldNameToSortedSpriteIndex.find(currentSpriteHash);
	if (currentSpriteIt == secondaryData.oldNameToSortedSpriteIndex.end()) {
		currentSpriteElement = nullptr;
	} else {
		currentSpriteElement = &secondaryData.sortedSprites[currentSpriteIt->second];
	}
	
	return currentSpriteElement;
}

const char* UI::getSpriteRepr(SortedSprite* sortedSprite) {
	if (!sortedSprite) {
		static const char nullSpriteName[32] = "null";
		return nullSpriteName;
	}
	return sortedSprite->repr();
}

BoxPart UI::hitboxHoveredPart(int hitboxIndex) const {
	if (boxMouseDown && boxResizePart != BOXPART_NONE) {
		if (hitboxIndex == boxSelectingOriginalIndex) {
			return boxResizePart;
		}
		return BOXPART_NONE;
	}
	if (hitboxIndex == boxHoverOriginalIndex) {
		return boxHoverPart;
	}
	return BOXPART_NONE;
}

void UI::boxResizeProcessBox(DrawHitboxArrayCallParams& params, Hitbox* hitboxesStart, Hitbox* ptr) {
	Hitbox newData;
	int index = ptr - hitboxesStart;
	RECT oldBounds;
	if (index >= 0 && index < (int)boxResizeOldHitboxes.size()) {
		oldBounds = params.getWorldBounds(
			boxResizeOldHitboxes[ptr - hitboxesStart]
		);
	} else {
		return;
	}
	
	if (overallSelectionBoxOldBounds.right != overallSelectionBoxOldBounds.left) {
		oldBounds.left = (int)std::roundf(
			(float)lastOverallSelectionBox.bounds.left / 100.F
			+ (float)(oldBounds.left - overallSelectionBoxOldBounds.left) / 100.F
			* (float)(lastOverallSelectionBox.bounds.right - lastOverallSelectionBox.bounds.left)
			/ (float)(overallSelectionBoxOldBounds.right - overallSelectionBoxOldBounds.left)
		) * 100;
		
		oldBounds.right = (int)std::roundf(
			(float)lastOverallSelectionBox.bounds.left / 100.F
			+ (float)(oldBounds.right - overallSelectionBoxOldBounds.left) / 100.F
			* (float)(lastOverallSelectionBox.bounds.right - lastOverallSelectionBox.bounds.left)
			/ (float)(overallSelectionBoxOldBounds.right - overallSelectionBoxOldBounds.left)
		) * 100;
	} else {
		oldBounds.left += boxSelectArenaXEnd - boxSelectArenaXStart;
		oldBounds.right += boxSelectArenaXEnd - boxSelectArenaXStart;
	}
	
	if (overallSelectionBoxOldBounds.bottom != overallSelectionBoxOldBounds.top) {
		oldBounds.top = (int)std::roundf(
			(float)lastOverallSelectionBox.bounds.top / 100.F
			+ (float)(oldBounds.top - overallSelectionBoxOldBounds.top) / 100.F
			* (float)(lastOverallSelectionBox.bounds.bottom - lastOverallSelectionBox.bounds.top)
			/ (float)(overallSelectionBoxOldBounds.bottom - overallSelectionBoxOldBounds.top)
		) * 100;
		
		oldBounds.bottom = (int)std::roundf(
			(float)lastOverallSelectionBox.bounds.top / 100.F
			+ (float)(oldBounds.bottom - overallSelectionBoxOldBounds.top) / 100.F
			* (float)(lastOverallSelectionBox.bounds.bottom - lastOverallSelectionBox.bounds.top)
			/ (float)(overallSelectionBoxOldBounds.bottom - overallSelectionBoxOldBounds.top)
		) * 100;
	} else {
		oldBounds.top += boxSelectArenaYEnd - boxSelectArenaYStart;
		oldBounds.bottom += boxSelectArenaYEnd - boxSelectArenaYStart;
	}
	
	DrawHitboxArrayCallParams::arenaToHitbox(params.params,
		oldBounds.left,
		oldBounds.top,
		oldBounds.right,
		oldBounds.bottom,
		&newData);
	if (newData.offX != ptr->offX
			|| newData.offY != ptr->offY) {
		boxResizeChangedSomething = true;
	}
	ptr->offX = newData.offX;
	ptr->offY = newData.offY;
	if (boxResizePart != BOXPART_MIDDLE) {
		if (newData.sizeX != ptr->sizeX
				|| newData.sizeY != ptr->sizeY) {
			boxResizeChangedSomething = true;
		}
		ptr->sizeX = newData.sizeX;
		ptr->sizeY = newData.sizeY;
	}
}

bool UI::hasOverallSelectionBox(RECT* bounds, BoxPart* hoverPart) {
	
	int selectedHitboxesCountWithoutPushbox = (int)selectedHitboxes.size();
	int _pushboxOriginalIndex = pushboxOriginalIndex();
	if (_pushboxOriginalIndex != -1 && hitboxIsSelected(_pushboxOriginalIndex)) {
		--selectedHitboxesCountWithoutPushbox;
	}
	
	if (selectedHitboxesCountWithoutPushbox > 1 && lastOverallSelectionBoxReady) {
		*bounds = lastOverallSelectionBox.bounds;
		*hoverPart = lastOverallSelectionBoxHoverPart;
		return true;
	}
	return false;
}

void UI::editHitboxesProcessControls() {
	editHitboxesProcessCamera();
	editHitboxesProcessHitboxMoving();
}

void UI::editHitboxesProcessCamera() {
	float cameraVertical = 0.F;
	float cameraHorizontal = 0.F;
	float cameraPerpendicular = 0.F;
	if (keyboard.isHeld(settings.hitboxEditMoveCameraUp)) {
		cameraVertical += keyboard.moveAmount(settings.hitboxEditMoveCameraUp, MULTIPLICATION_GOAL_CAMERA_MOVE);
	}
	if (keyboard.isHeld(settings.hitboxEditMoveCameraDown)) {
		cameraVertical -= keyboard.moveAmount(settings.hitboxEditMoveCameraDown, MULTIPLICATION_GOAL_CAMERA_MOVE);
	}
	if (keyboard.isHeld(settings.hitboxEditMoveCameraLeft)) {
		cameraHorizontal -= keyboard.moveAmount(settings.hitboxEditMoveCameraLeft, MULTIPLICATION_GOAL_CAMERA_MOVE);
	}
	if (keyboard.isHeld(settings.hitboxEditMoveCameraRight)) {
		cameraHorizontal += keyboard.moveAmount(settings.hitboxEditMoveCameraRight, MULTIPLICATION_GOAL_CAMERA_MOVE);
	}
	if (keyboard.isHeld(settings.hitboxEditMoveCameraForward)) {
		cameraPerpendicular -= keyboard.moveAmount(settings.hitboxEditMoveCameraForward, MULTIPLICATION_GOAL_CAMERA_MOVE);
	}
	if (keyboard.isHeld(settings.hitboxEditMoveCameraBack)) {
		cameraPerpendicular += keyboard.moveAmount(settings.hitboxEditMoveCameraBack, MULTIPLICATION_GOAL_CAMERA_MOVE);
	}
	if (camera.editHitboxesViewDistance < 300.F) {
		float coeff = camera.editHitboxesViewDistance / 300.F;
		cameraHorizontal *= coeff * settings.hitboxEditMoveCameraHorizontalSpeedMultiplier;
		cameraVertical *= coeff * settings.hitboxEditMoveCameraVerticalSpeedMultiplier;
		cameraPerpendicular *= coeff * settings.hitboxEditMoveCameraPerpendicularSpeedMultiplier;
	}
	if (cameraPerpendicular && settings.hitboxEditZoomOntoMouseCursor) {
		EndScene::HitboxEditorCameraValues& vals = endScene.hitboxEditorCameraValues;
		Entity editEntity{gifMode.editHitboxesEntity};
		if (vals.prepare(editEntity)) {
			float screenWidthUE3_beforeMove = vals.screenWidthASW / vals.m;
			float screenHeightUE3_beforeMove = vals.screenHeightASW / vals.m;
			float cam_xUE3_beforeMove = vals.cam_xASW / vals.m;
			float cam_zUE3_beforeMove = vals.cam_zASW / vals.m;
			float cursor[2];
			ui.getMousePos(cursor);
			if (cursor[0] >= 0.F && cursor[0] <= vals.vw
					&& cursor[1] >= 0.F && cursor[1] <= vals.vh) {
				float cam_yafterMove = vals.cam_y + cameraPerpendicular;
				// cursor mapped to from -1 to 1
				float cursor1x = ((cursor[0] - vals.vw * 0.5F) / vals.vw * 2.F);
				float cursor1y = -((cursor[1] - vals.vh * 0.5F) / vals.vh * 2.F);
				float xCoeffUE3_beforeMove = 1.F / vals.t * vals.cam_y;
				float yCoeffUE3_beforeMove = 1.F / vals.t  / vals.vw * vals.vh * vals.cam_y;
				float xCoeffUE3_afterMove = 1.F / vals.t * cam_yafterMove;
				float yCoeffUE3_afterMove = 1.F / vals.t  / vals.vw * vals.vh * cam_yafterMove;
				float cursor_xUE3_beforeMove = cursor1x * xCoeffUE3_beforeMove;
				float cursor_yUE3_beforeMove = cursor1y * yCoeffUE3_beforeMove;
				float cursor_xUE3_afterMove = cursor1x * xCoeffUE3_afterMove;
				float cursor_yUE3_afterMove = cursor1y * yCoeffUE3_afterMove;
				float cursorMoveDistX = cursor_xUE3_afterMove - cursor_xUE3_beforeMove;
				float cursorMoveDistY = cursor_yUE3_afterMove - cursor_yUE3_beforeMove;
				cameraHorizontal -= cursorMoveDistX;
				cameraVertical -= cursorMoveDistY;
			}
		}
	}
	camera.editHitboxesOffsetX += cameraHorizontal;
	camera.editHitboxesOffsetY += cameraVertical;
	camera.editHitboxesViewDistance += cameraPerpendicular;
	if (camera.editHitboxesViewDistance < 1.F) {
		camera.editHitboxesViewDistance = 1.F;
	}
}

static bool keyHeldWithRepeatCount(int* repeatCount, const std::vector<int>& keyCombo) {
	if (keyboard.gotPressed(keyCombo)) {
		*repeatCount = 1;
		return true;
	}
	if (*repeatCount == 0) return false;
	bool isHeld = keyboard.isHeld(keyCombo);
	if (!isHeld) {
		*repeatCount = 0;
		return false;
	}
	if (*repeatCount >= 20) {
		return true;
	}
	++*repeatCount;
	return false;
}

static bool anythingDiffersExceptBounds(const std::vector<UI::BoxSelectBox>& left, const std::vector<UI::BoxSelectBox>& right) {
	int count = (int)left.size();
	if (count != (int)right.size()) return true;
	
	const UI::BoxSelectBox* boxLeft = left.data();
	const UI::BoxSelectBox* boxRight = right.data();
	for (int i = 0; i < count; ++i) {
		if (memcmp(boxLeft, boxRight, sizeof EditedHitbox) != 0) return true;
		++boxLeft;
		++boxRight;
	}
	return false;
}

void UI::editHitboxesProcessHitboxMoving() {
	
	static int repeatCounts[4*4] { 0 };
	
	struct OnExit {
		~OnExit() {
			if (!reachedUsefulPart) {
				memset(repeatCounts, 0, sizeof repeatCounts);
				ui.keyboardMoveEntity = nullptr;
				ui.keyboardMoveSprite = nullptr;
			}
		}
		bool reachedUsefulPart = false;
	} onExit;
	
	if (!gifMode.editHitboxes || !gifMode.editHitboxesEntity
		|| selectedHitboxes.empty()) return;
	
	int selectedHitboxesCountWithoutPushbox = (int)selectedHitboxes.size();
	int _pushboxOriginalIndex = pushboxOriginalIndex();
	if (_pushboxOriginalIndex != -1 && hitboxIsSelected(_pushboxOriginalIndex)) {
		--selectedHitboxesCountWithoutPushbox;
	}
	
	if (selectedHitboxesCountWithoutPushbox == 0) return;
	
	Entity editEntity{gifMode.editHitboxesEntity};
	SortedSprite* sortedSprite = hitboxEditFindCurrentSprite();
	if (!sortedSprite) return;
	
	if (editEntity.scaleX() == 0 || editEntity.scaleY() == 0) return;
	
	DrawHitboxArrayCallParams params;
	editHitboxesFillParams(params, editEntity);
	
	onExit.reachedUsefulPart = true;
	
	std::vector<BoxSelectBox> currentConvertedBoxes;
	
	LayerIterator layerIterator(editEntity, sortedSprite);
	currentConvertedBoxes.reserve(layerIterator.count());
	while (layerIterator.getNext()) {
		if (settings.hitboxList[layerIterator.type].show && !layerIterator.isPushbox && hitboxIsSelected(layerIterator.originalIndex)) {
			currentConvertedBoxes.emplace_back();
			BoxSelectBox& newBox = currentConvertedBoxes.back();
			layerIterator.copyTo(&newBox);
			newBox.bounds = params.getWorldBounds(*layerIterator.ptr);
		}
	}
	
	int moveX = 0;
	int moveY = 0;
	if (keyHeldWithRepeatCount(repeatCounts + 0, settings.hitboxEditMoveHitboxesUp)) {
		moveY -= settings.hitboxEditMoveHitboxesNormalAmount;
	}
	if (keyHeldWithRepeatCount(repeatCounts + 1, settings.hitboxEditMoveHitboxesDown)) {
		moveY += settings.hitboxEditMoveHitboxesNormalAmount;
	}
	if (keyHeldWithRepeatCount(repeatCounts + 2, settings.hitboxEditMoveHitboxesLeft)) {
		moveX -= settings.hitboxEditMoveHitboxesNormalAmount;
	}
	if (keyHeldWithRepeatCount(repeatCounts + 3, settings.hitboxEditMoveHitboxesRight)) {
		moveX += settings.hitboxEditMoveHitboxesNormalAmount;
	}
	if (keyHeldWithRepeatCount(repeatCounts + 4, settings.hitboxEditMoveHitboxesALotUp)) {
		moveY -= settings.hitboxEditMoveHitboxesLargeAmount;
	}
	if (keyHeldWithRepeatCount(repeatCounts + 5, settings.hitboxEditMoveHitboxesALotDown)) {
		moveY += settings.hitboxEditMoveHitboxesLargeAmount;
	}
	if (keyHeldWithRepeatCount(repeatCounts + 6, settings.hitboxEditMoveHitboxesALotLeft)) {
		moveX -= settings.hitboxEditMoveHitboxesLargeAmount;
	}
	if (keyHeldWithRepeatCount(repeatCounts + 7, settings.hitboxEditMoveHitboxesALotRight)) {
		moveX += settings.hitboxEditMoveHitboxesLargeAmount;
	}
	
	if (DrawHitboxArrayCallParams::willFlipWidthAndHeight(editEntity.pitch())) {
		moveX *= editEntity.scaleY() / 1000;
		moveY *= editEntity.scaleX() / 1000;
	} else {
		moveX *= editEntity.scaleX() / 1000;
		moveY *= editEntity.scaleY() / 1000;
	}
	
	if (moveX == 0 && moveY == 0) return;
	
	if (editEntity != keyboardMoveEntity || sortedSprite != keyboardMoveSprite
			|| anythingDiffersExceptBounds(currentConvertedBoxes, keyboardMoveBackupBoxes)) {
		for (int i = 0; i < _countof(repeatCounts); ++i) {
			int repeatCount = repeatCounts[i];
			if (repeatCount != 1) repeatCounts[i] = 0;
		}
		resetKeyboardMoveCache();
		resetCoordsMoveCache();
		keyboardMoveEntity = editEntity;
		keyboardMoveSprite = sortedSprite;
		keyboardMoveBackupBoxes.clear();
		keyboardMoveBackupBoxes.reserve(currentConvertedBoxes.size());
		for (BoxSelectBox& box : currentConvertedBoxes) {
			if (hitboxIsSelected(box.originalIndex)) {
				keyboardMoveBackupBoxes.push_back(box);
			}
		}
	}
	
	keyboardMoveX += moveX;
	keyboardMoveY += moveY;
	
	HitboxHolder* hitboxes = editEntity.hitboxes();
	Hitbox* hitboxesStart = hitboxes->hitboxesStart();
	
	int hitboxCount = hitboxes->hitboxCount();
	
	MoveResizeBoxesUndoHelper helper(hitboxesStart, hitboxCount, 60 * 10  /* 10 seconds */);
	bool actuallyMovedSomething = false;
	
	for (BoxSelectBox& box : keyboardMoveBackupBoxes) {
		if (hitboxIsSelected(box.originalIndex)) {
			Hitbox newData;
			DrawHitboxArrayCallParams::arenaToHitbox(params.params,
				box.bounds.left + keyboardMoveX,
				box.bounds.top - keyboardMoveY,
				box.bounds.right + keyboardMoveX,
				box.bounds.bottom - keyboardMoveY,
				&newData);
			actuallyMovedSomething = actuallyMovedSomething || box.ptr->offX != newData.offX
				|| box.ptr->offY != newData.offY;
			box.ptr->offX = newData.offX;
			box.ptr->offY = newData.offY;
		}
	}
	
	if (actuallyMovedSomething) {
		helper.finish();
	}
	
}

void UI::editHitboxesConvertBoxes(DrawHitboxArrayCallParams& params, Entity editEntity, SortedSprite* sortedSprite) {
	lastOverallSelectionBox.isPushbox = false;
	lastOverallSelectionBox.originalIndex = -2;
	lastOverallSelectionBoxReady = false;
	
	LayerIterator layerIterator(editEntity, sortedSprite);
	convertedBoxes.reserve(layerIterator.count());
	while (layerIterator.getNext()) {
		if (settings.hitboxList[layerIterator.type].show && !layerIterator.isPushbox) {
			convertedBoxes.emplace_back();
			BoxSelectBox& newBox = convertedBoxes.back();
			layerIterator.copyTo(&newBox);
			newBox.bounds = params.getWorldBounds(*layerIterator.ptr);
			if (hitboxIsSelected(layerIterator.originalIndex)) {
				if (!lastOverallSelectionBoxReady) {
					lastOverallSelectionBoxReady = true;
					lastOverallSelectionBox.bounds = newBox.bounds;
				} else {
					combineBounds(lastOverallSelectionBox.bounds, newBox.bounds);
				}
			}
		}
	}
	
	convertedBoxesPrepared = true;
	
}

void UI::editHitboxesFillParams(DrawHitboxArrayCallParams& params, Entity editEntity) {
	params.params.posX = editEntity.posX();
	params.params.posY = editEntity.posY();
	params.params.angle = editEntity.pitch();
	params.params.flip = editEntity.isFacingLeft() ? 1 : -1;
	params.params.scaleX = editEntity.scaleX();
	params.params.scaleY = editEntity.scaleY();
	params.params.transformCenterX = editEntity.transformCenterX();
	params.params.transformCenterY = editEntity.transformCenterY();
	params.data.resize(1);
	params.fillColor = replaceAlpha(64, settings.hitboxList[hitboxEditorCurrentHitboxType].color);
	params.outlineColor = replaceAlpha(255, params.fillColor);
	params.thickness = 1;
}

int UI::pushboxOriginalIndex() {
	if (!gifMode.editHitboxes || !gifMode.editHitboxesEntity) return -1;
	Entity editEntity{gifMode.editHitboxesEntity};
	if (!editEntity.showPushbox()) return -1;
	const SortedSprite* sortedSprite = hitboxEditFindCurrentSprite();
	if (!sortedSprite) return -1;
	if (sortedSprite->layers) {
		EditedHitbox* layer = sortedSprite->layers;
		for (int i = 0; i < (int)sortedSprite->layersSize; ++i) {
			if (layer->isPushbox) return layer->originalIndex;
			++layer;
		}
		return -1;
	} else {
		HitboxHolder* hitboxes = editEntity.hitboxes();
		int overallHitboxIndex = 0;
		for (int hitboxType = 0; hitboxType < HITBOXTYPE_PUSHBOX; ++hitboxType) {
			overallHitboxIndex += hitboxes->count[hitboxType];
		}
		return overallHitboxIndex + hitboxes->count[HITBOXTYPE_PUSHBOX];
	}
}

void UI::resetKeyboardMoveCache() {
	keyboardMoveX = 0;
	keyboardMoveY = 0;
	keyboardMoveEntity = nullptr;
	keyboardMoveSprite = nullptr;
}

void UI::editHitboxesChangeView(Entity newEditEntity) {
	if (newEditEntity == gifMode.editHitboxesEntity) return;
	gifMode.editHitboxesEntity = newEditEntity;
	selectedHitboxes.clear();
	restartHitboxEditMode();
}

UI::SortedAnimSeq::SortedAnimSeq(FName fname, const char* str) : fname(fname) {
	char* ptr = buf;
	size_t bufSize = sizeof buf - 1;  // reserve one char for null
	while (bufSize) {
		*ptr = *str;
		--bufSize;
		++ptr;
		if (*str == '\0') {
			break;
		}
		++str;
	}
	++bufSize;
	memset(ptr, 0, bufSize);
}

void UI::castrateUndos() {
	while (undoRingBuffer[undoRingBufferIndex]) {
		undoRingBuffer[undoRingBufferIndex] = nullptr;
		--undoRingBufferIndex;
		if (undoRingBufferIndex < 0) {
			undoRingBufferIndex = (int)undoRingBuffer.size() - 1;
		}
	}
}

void UI::castrateRedos() {
	while (redoRingBuffer[redoRingBufferIndex]) {
		redoRingBuffer[redoRingBufferIndex] = nullptr;
		--redoRingBufferIndex;
		if (redoRingBufferIndex < 0) {
			redoRingBufferIndex = (int)redoRingBuffer.size() - 1;
		}
	}
}

ThreadUnsafeSharedPtr<UndoOperationBase>& UI::allocateUndo() {
	// this will spawn the very first ever element at index 1
	++undoRingBufferIndex;
	if (undoRingBufferIndex >= (int)undoRingBuffer.size()) {
		undoRingBufferIndex = 0;
	}
	return undoRingBuffer[undoRingBufferIndex];
}

ThreadUnsafeSharedPtr<UndoOperationBase>& UI::allocateRedo() {
	// this will spawn the very first ever element at index 1
	++redoRingBufferIndex;
	if (redoRingBufferIndex >= (int)redoRingBuffer.size()) {
		redoRingBufferIndex = 0;
	}
	return redoRingBuffer[redoRingBufferIndex];
}

bool UI::performOp(UndoOperationBase* op) {
	ThreadUnsafeSharedPtr<UndoOperationBase>& oldOp = undoRingBuffer[undoRingBufferIndex];
	if (oldOp && oldOp->combine(*op)) {
		castrateRedos();
		return true;
	}
	ThreadUnsafeSharedPtr<UndoOperationBase>& undoOp = allocateUndo();
	if (!op->perform(&undoOp)) {
		--undoRingBufferIndex;
		if (undoRingBufferIndex < 0) {
			undoRingBufferIndex = (int)undoRingBuffer.size() - 1;
		}
		return false;
	}
	castrateRedos();
	return true;
}

void UI::onAddBoxWithoutLayers(BYTE* jonbin) {
	if (gifMode.editHitboxes && gifMode.editHitboxesEntity && Entity{gifMode.editHitboxesEntity}.hitboxes()->jonbinPtr == jonbin) {
		selectedHitboxesPreBoxSelect.clear();
		selectedHitboxesBoxSelect.clear();
		prevDragNDropItems.clear();
		dragNDropItems.clear();
		dragNDropItemsCarried.clear();
		dragNDropActive = false;
		dragNDropMouseDragPurpose = MOUSEDRAGPURPOSE_NONE;
		dragNDropInterpolationTimer = 0;
	}
}

UI::MoveResizeBoxesUndoHelper::MoveResizeBoxesUndoHelper(Hitbox* hitboxesStart, int hitboxCount, int timerRange)
	: prevUndo(
		(ThreadUnsafeSharedPtr<MoveResizeBoxesOperation>&)ui.undoRingBuffer[ui.undoRingBufferIndex]
	),
	hitboxesStart(hitboxesStart),
	hitboxCount(hitboxCount)
{
	
	extern unsigned int getUE3EngineTick();
	engineTick = getUE3EngineTick();
	prevUndoOk = prevUndo && prevUndo->combineOk(
		gifMode.editHitboxesEntity, engineTick, timerRange, UNDO_OPERATION_TYPE_MOVE_RESIZE_BOXES,
		hitboxCount);
	
	if (!prevUndoOk) {
		oldData.resize(hitboxCount);
		memcpy(oldData.data(), hitboxesStart, hitboxCount * sizeof (Hitbox));
	}
	
}

void UI::MoveResizeBoxesUndoHelper::finish() {
	
	Entity editEntity{gifMode.editHitboxesEntity};
	
	ui.castrateRedos();
	
	if (prevUndoOk) {
		prevUndo->update(hitboxesStart, hitboxCount);
		return;
	}
	
	ThreadUnsafeSharedPtr<MoveResizeBoxesOperation>& newOp = (ThreadUnsafeSharedPtr<MoveResizeBoxesOperation>&)ui.allocateUndo();
	newOp = new ThreadUnsafeSharedResource<MoveResizeBoxesOperation>();
	newOp->fill(std::move(oldData), hitboxesStart, hitboxCount);
	
}

static int __cdecl NewSortSortedSprite(const void* Ptr1, const void* Ptr2) {
	SortedSprite** sprite1Ptr = (SortedSprite**)Ptr1;
	SortedSprite** sprite2Ptr = (SortedSprite**)Ptr2;
	SortedSprite* sprite1 = *sprite1Ptr;
	SortedSprite* sprite2 = *sprite2Ptr;
	DWORD hash1 = Entity::hashStringLowercase(sprite1->newName[0] ? sprite1->newName : sprite1->name);
	DWORD hash2 = Entity::hashStringLowercase(sprite2->newName[0] ? sprite2->newName : sprite2->name);
	if (hash1 == hash2) {
		return 0;
	} else if (hash1 < hash2) {
		return -1;
	} else {
		return 1;
	}
}

static int __cdecl NewSortSortedSpriteAlphabetically(const void* Ptr1, const void* Ptr2) {
	SortedSprite** sprite1Ptr = (SortedSprite**)Ptr1;
	SortedSprite** sprite2Ptr = (SortedSprite**)Ptr2;
	SortedSprite* sprite1 = *sprite1Ptr;
	SortedSprite* sprite2 = *sprite2Ptr;
	return strcmp(sprite1->newName[0] ? sprite1->newName : sprite1->name,
		sprite2->newName[0] ? sprite2->newName : sprite2->name);
}

static void sortNewSorted(FPACSecondaryData& secondaryData, std::vector<SortedSprite*>& newSorted, int (__cdecl * CompareFunc)(const void*, const void*)) {
	
	newSorted.reserve(secondaryData.sortedSprites.size());
	for (SortedSprite& sortedSprite : secondaryData.sortedSprites) {
		if (!sortedSprite.deleted) {
			newSorted.push_back(&sortedSprite);
		}
	}
	qsort(newSorted.data(), newSorted.size(), sizeof (SortedSprite*), CompareFunc);
	
}

template<typename T>
static void serializeCollision_writeJonbin(FPACSecondaryData& secondaryData, FPAC* oldFpac, FPAC* newFpac) {
	DWORD offset = 0;
	T* newLookupEntry = (T*)(
		(BYTE*)newFpac + 0x20
	);
	DWORD newLookupEntryIndex = 0;
	
	std::vector<SortedSprite*> newSorted;
	sortNewSorted(secondaryData, newSorted, NewSortSortedSprite);
	
	for (SortedSprite* sortedSprite : newSorted) {
		T* oldLookupEntry = (T*)sortedSprite->name;
		
		memcpy(newLookupEntry, oldLookupEntry, sizeof (T));
		if (sortedSprite->newName[0]) {
			memcpy(newLookupEntry->spriteName, sortedSprite->newName, 32);
			newLookupEntry->hash = Entity::hashStringLowercase(sortedSprite->newName);
		}
		newLookupEntry->index = newLookupEntryIndex++;
		newLookupEntry->offset = offset;
		
		DWORD size = oldLookupEntry->size;
		BYTE* newJonbin = (BYTE*)newFpac + newFpac->headerSize + offset;
		memcpy(newJonbin, (BYTE*)oldFpac + oldFpac->headerSize + oldLookupEntry->offset, size);
		
		if (size & 7) {
			DWORD slack = 8 - (size & 7);
			memset(newJonbin + size, 0, slack);
			size += slack;
		}
		
		offset += size;
		++newLookupEntry;
	}
}

void UI::serializeCollision(std::vector<BYTE>& data, int player) {
	DWORD totalSize = 0;
	FPACSecondaryData& secondaryData = hitboxEditorFPACSecondaryData[player];
	secondaryData.parseAllSprites();
	FPAC* fpac = secondaryData.Collision->TopData;
	DWORD count = 0;  // lookup entries count
	if (fpac->size0x50()) {
		for (const SortedSprite& sortedSprite : secondaryData.sortedSprites) {
			if (!sortedSprite.deleted) {
				++count;
				DWORD size = ((FPACLookupElement0x50*)sortedSprite.name)->size;
				size = (size + 7) & (~7);
				totalSize += size;
			}
		}
	} else {
		for (const SortedSprite& sortedSprite : secondaryData.sortedSprites) {
			if (!sortedSprite.deleted) {
				++count;
				DWORD size = ((FPACLookupElement0x30*)sortedSprite.name)->size;
				size = (size + 7) & (~7);
				totalSize += size;
			}
		}
	}
	DWORD lookupEntrySize = fpac->elementSize();
	totalSize += 0x20  // FPAC base data
		+ lookupEntrySize * count;  // lookup entries
	data.resize(totalSize);
	FPAC* newFpac = (FPAC*)data.data();
	memcpy(newFpac, fpac, 0x20);
	newFpac->count = count;
	newFpac->rawSize = data.size();
	newFpac->headerSize = 0x20 + count * lookupEntrySize;
	if (fpac->size0x50()) {
		serializeCollision_writeJonbin<FPACLookupElement0x50>(secondaryData, fpac, newFpac);
	} else {
		serializeCollision_writeJonbin<FPACLookupElement0x30>(secondaryData, fpac, newFpac);
	}
}

class MyOwnStringBuilder {
private:
	static const DWORD blockSize = 1024;
public:

	// error C3074: an array cannot be initialized with a parenthesized initializer
	// me: *wraps array into a struct*
	// me: and now it's ok somehow
	struct IHateAllOfYou {
		char array[blockSize];
	};
	
	void dump(std::vector<BYTE>& out) const {
		if (memory.empty()) {
			out.clear();
			return;
		}
		DWORD sizeLeft = blockSize * (memory.size() - 1) + blockSize - remaining;
		out.resize(sizeLeft + 1);
		BYTE* ptr = out.data();
		for (const IHateAllOfYou& block : memory) {
			DWORD sizeToCopy = min(sizeLeft, blockSize);
			memcpy(ptr, block.array, sizeToCopy);
			sizeLeft -= blockSize;
			ptr += sizeToCopy;
		}
		out.back() = '\0';
	}
	MyOwnStringBuilder& operator<<(const char* str) {
		
		DWORD len = strlen(str);
		
		while (len) {
			
			DWORD sizeToCopy = min(len, remaining);
			if (sizeToCopy) {
				memcpy(buf, str, sizeToCopy);
				len -= sizeToCopy;
				str += sizeToCopy;
				remaining -= sizeToCopy;
				buf += sizeToCopy;
			}
			
			if (len) newBlock();
			
		}
		return *this;
	}
	MyOwnStringBuilder& operator<<(char c) {
		if (remaining == 0) newBlock();
		*buf = c;
		++buf;
		--remaining;
		return *this;
	}
	MyOwnStringBuilder& operator<<(int num) {
		char localBuf[12];
		sprintf_s(localBuf, "%d", num);
		return *this << localBuf;
	}
	MyOwnStringBuilder& operator<<(unsigned long num) {
		char localBuf[11];
		sprintf_s(localBuf, "%u", num);
		return *this << localBuf;
	}
	// print floats yourself. I have no idea how long they can be and someone needs to set precision
private:
	std::list<IHateAllOfYou> memory;
	char* buf;
	DWORD remaining = 0;
	void newBlock() {
		memory.push_back({});
		buf = memory.back().array;
		remaining = blockSize;
	}
};

static void printJsonString(const char* text, MyOwnStringBuilder& ss) {
	while (*text != '\0') {
		char c = *text;
		// https://www.json.org/json-en.html
		if (c == '"') {
			ss << "\\\"";
		} else if (c == '\\') {
			ss << "\\\\";
		} else if (c == '\b') {
			ss << "\\b";
		} else if (c == '\f') {
			ss << "\\f";
		} else if (c == '\n') {
			ss << "\\n";
		} else if (c == '\r') {
			ss << "\\r";
		} else if (c == '\t') {
			ss << "\\t";
		} else if (c > 126 || c < 32) {
			// I'm not gonna read your UTF-8
			// after seeing the walls of hardcode in UE3 and imgui I don't think wikipedia is telling the whole story on it
			ss << '?';
		} else {
			ss << c;
		}
		++text;
	}
}

static void printAnimSeq(BYTE* jonbin, MyOwnStringBuilder& ss) {
	jonbin += 4;  // skip JONB
	short nameCount = *(short*)jonbin;
	if (!nameCount) {
		ss << "null";
		return;
	}
	ss << '"';
	jonbin += 2;
	printJsonString((const char*)jonbin, ss);
	ss << '"';
}

template<typename T>
void serializeJson_impl(FPACSecondaryData& secondaryData, MyOwnStringBuilder& ss) {
	
	FPAC* fpac = secondaryData.Collision->TopData;
	
	std::vector<SortedSprite*> newSorted;
	sortNewSorted(secondaryData, newSorted, NewSortSortedSpriteAlphabetically);
	
	bool isFirst = true;
	
	for (SortedSprite* sortedSprite : newSorted) {
		T* lookupEntry = (T*)sortedSprite->name;
		BYTE* jonbin = (BYTE*)fpac + fpac->headerSize + lookupEntry->offset;
		if (isFirst) {
			isFirst = false;
		} else {
			ss << ",\n";
		}
		ss << "\t{\n"
			"\t\t\"sprite\": \""; printJsonString(sortedSprite->name, ss); ss << "\",\n"
			"\t\t\"anim\": "; printAnimSeq(jonbin, ss); ss << ",\n"
			"\t\t\"boxes\": [";
		
		LayerIterator layerIterator(fpac, sortedSprite);
		int hitboxCount = layerIterator.count();  // includes pushbox
		if (hitboxCount <= 1) {
			ss << "]\n"
			"\t}";
			continue;
		}
		
		// this checks if the name count is > 0
		bool showPushbox = *(short*)(
			jonbin + 4  // skip JONB
		) > 0;
		
		int printedCount = 0;
		while (layerIterator.getNext()) {
			if (!layerIterator.isPushbox || showPushbox) {
				
				if (layerIterator.isPushbox) {
					continue;
				}
				
				if (printedCount) {
					ss << ",\n"
					"\t\t\t{\n"
					"\t\t\t\t\"type\": \"";
				} else {
					ss << "\n"
					"\t\t\t{\n"
					"\t\t\t\t\"type\": \"";
				}
				++printedCount;
				ss << hitboxTypeName[layerIterator.type] << "\",\n"
					"\t\t\t\t\"x\": " << (int)(layerIterator.ptr->offX) << ",\n"
					"\t\t\t\t\"y\": " << (int)(layerIterator.ptr->offY) << ",\n"
					"\t\t\t\t\"w\": " << (int)(layerIterator.ptr->sizeX) << ",\n"
					"\t\t\t\t\"h\": " << (int)(layerIterator.ptr->sizeY) << "\n"
					"\t\t\t}";
			}
		}
		
		if (!printedCount) {
			ss << "]\n"
			"\t}";
		} else {
			ss << "\n"
			"\t\t]\n"
			"\t}";
		}
		
	}
	
}

void UI::serializeJson(std::vector<BYTE>& data, int player) {
	FPACSecondaryData& secondaryData = hitboxEditorFPACSecondaryData[player];
	secondaryData.parseAllSprites();
	FPAC* fpac = secondaryData.Collision->TopData;
	MyOwnStringBuilder ss;
	ss << "[\n";
	
	if (fpac->size0x50()) {
		serializeJson_impl<FPACLookupElement0x50>(secondaryData, ss);
	} else {
		serializeJson_impl<FPACLookupElement0x30>(secondaryData, ss);
	}
	ss << "\n"
		"]";
	ss.dump(data);
}

void UI::writeOutClipboard(const std::vector<BYTE>& data) {
	char errorbuf[1024];
	#define exitWinErr(msg) { \
		WinError winErr; \
		sprintf_s(errorbuf, msg ": %ls", winErr.getMessage()); \
		showErrorDlgS(errorbuf); \
		return; \
	}
		
	if (!OpenClipboard(0)) {
		exitWinErr("OpenClipboard failed")
	}
	class ClipboardCloser {
	public:
		~ClipboardCloser() {
			if (!closedAlready) {
				CloseClipboard();
			}
		}
		bool closedAlready = false;
	} clipboardCloser;
	if (!EmptyClipboard()) {
		exitWinErr("EmptyClipboard failed")
	}
	HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, data.size());
	if (!hg) {
		exitWinErr("GlobalAlloc failed")
	}
	LPVOID hgLock = GlobalLock(hg);
	if (!hgLock) {
		exitWinErr("GlobalLock failed")
	}
	memcpy(hgLock, data.data(), data.size());
	GlobalUnlock(hg);
	if (!SetClipboardData(CF_TEXT, hg)) {
		exitWinErr("SetClipboardData failed")
	}
	clipboardCloser.closedAlready = true;
	CloseClipboard();
	GlobalFree(hg);
	#undef exitWinErr
}

void UI::requestFileSelect(const wchar_t* fileSpec, std::wstring& lastSelectedPath) {
	if (keyboard.thisProcessWindow) {
		PostMessageW(keyboard.thisProcessWindow, WM_APP_UI_REQUEST_FILE_SELECT_WRITE, (WPARAM)&lastSelectedPath, (LPARAM)fileSpec);
	}
}

void UI::writeOutFile(const std::wstring& path, const std::vector<BYTE>& data) {
	
	struct FileCloser {
		~FileCloser() {
			if (file) CloseHandle(file);
		}
		HANDLE file;
	} fileCloser{NULL};
	
	char errorbuf[1024];
	#define exitWinErr(msg) { \
		WinError winErr; \
		sprintf_s(errorbuf, msg ": %ls", winErr.getMessage()); \
		showErrorDlgS(errorbuf); \
		return; \
	}
	
	fileCloser.file = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (fileCloser.file == NULL || fileCloser.file == INVALID_HANDLE_VALUE) {
		fileCloser.file = NULL;
		WinError winErr;
		sprintf_s(errorbuf, "Failed to open file '%ls': %ls", path.c_str(), winErr.getMessage());
		showErrorDlgS(errorbuf);
		return;
	}
	const BYTE* ptr = data.data();
	size_t remaining = data.size();
	DWORD bytesWritten;
	while (remaining) {
		size_t bytesToWrite = min(remaining, 1024);
		if (!WriteFile(fileCloser.file, ptr, bytesToWrite, &bytesWritten, NULL)) {
			exitWinErr("Failed to write to file")
		}
		if (bytesWritten != bytesToWrite) {
			sprintf_s(errorbuf, "Number of bytes written doesn't match number of bytes we were supposed to write: wrote %u, wanted to write %u",
				bytesWritten, bytesToWrite);
			showErrorDlgS(errorbuf);
			return;
		}
		ptr += bytesToWrite;
		remaining -= bytesToWrite;
	}
	#undef exitWinErr
	// fileCloser will close the file
}

void UI::makeFileSelectRequest(void(UI::*serializer)(std::vector<BYTE>& data, int player),
				const wchar_t* filterStr, int player, bool removeNullTerminator) {
	if (!dataToWriteToFile) {
		dataToWriteToFile = new ThreadUnsafeSharedResource<std::vector<BYTE>>();
	}
	(ui.*serializer)(*dataToWriteToFile, player);
	if (removeNullTerminator && !dataToWriteToFile->empty() && dataToWriteToFile->back() == '\0') {
		dataToWriteToFile->resize(dataToWriteToFile->size() - 1);
	}
	int requiredStrLength;
	if (player == 2) {
		requiredStrLength = 3;
	} else {
		Entity ent = entityList.slots[player];
		requiredStrLength = strlen(ent.charCodename());
	}
	const wchar_t* extSeek = filterStr;
	const wchar_t* lastDot = nullptr;
	while (*extSeek != L'\0') ++extSeek;
	++extSeek;
	if (*extSeek == L'*') ++extSeek;
	if (*extSeek == L'.') lastDot = extSeek;
	while (*extSeek != L'\0') ++extSeek;
	if (lastDot) {
		requiredStrLength += extSeek - lastDot;
	} else {
		lastDot = extSeek;
	}
	
	lastSelectedCollisionPath.clear();
	lastSelectedCollisionPath.reserve(requiredStrLength);
	
	if (player == 2) {
		lastSelectedCollisionPath = L"cmn";
	} else {
		Entity ent = entityList.slots[player];
		char* codename = ent.charCodename();
		while (*codename != '\0') {
			lastSelectedCollisionPath += (wchar_t)*codename;
			++codename;
		}
	}
	
	while (*lastDot != L'\0') {
		lastSelectedCollisionPath += towlower(*lastDot);
		++lastDot;
	}
	
	requestFileSelect(filterStr, lastSelectedCollisionPath);
}

void UI::readCollisionFromFile(const std::wstring& path, int player) {
	
	if (player == -1) {
		int slashPos = findCharRevW(path.c_str(), L'\\');
		int dotPos = findCharRevW(path.c_str(), L'.');
		if (dotPos != -1 && slashPos != -1 && dotPos < slashPos) dotPos = -1;
		if (dotPos != -1) {
			++slashPos;
			std::wstring baseName(path.begin() + slashPos, path.begin() + dotPos);
			std::vector<char> baseNameAscii;
			baseNameAscii.reserve(baseName.size() + 1);
			bool error = false;
			for (wchar_t wc : baseName) {
				if ((wc & 0xFF00) != 0) {
					error = true;
					break;
				}
				char c = (char)wc;
				if (c > 126 || c < 32) {
					error = true;
					break;
				}
				baseNameAscii.push_back(c);
			}
			if (!error) {
				baseNameAscii.push_back('\0');
				for (int i = 0; i < 2; ++i) {
					entityList.populate();
					Entity ent = entityList.slots[i];
					if (_stricmp(ent.charCodename(), baseNameAscii.data()) == 0) {
						player = i;
						break;
					}
				}
				if (player == -1 && _stricmp(baseNameAscii.data(), "cmn") == 0) {
					player = 2;
				}
			}
		}
		if (player == -1) {
			showErrorDlgS("Couldn't determine whose collision data is being loaded based on the file name alone.\n"
				"Please, press the \"Load\" button again and, from the selection that shows up,"
				" select \"From file\" from one of the columns. This will specify whose data to overwrite with this file.");
			return;
		}
	}
	
	struct FileCloser {
		~FileCloser() {
			if (file) CloseHandle(file);
		}
		HANDLE file;
	} fileCloser{NULL};
	
	char buf[1024];
	#define exitWinErr(msg) { \
		WinError winErr; \
		sprintf_s(buf, msg ": %ls", winErr.getMessage()); \
		showErrorDlgS(buf); \
		return; \
	}
	
	fileCloser.file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (fileCloser.file == NULL || fileCloser.file == INVALID_HANDLE_VALUE) {
		fileCloser.file = NULL;
		WinError winErr;
		sprintf_s(buf, "Failed to open file '%ls': %ls", path.c_str(), winErr.getMessage());
		showErrorDlgS(buf);
		return;
	}
	
	HANDLE file = fileCloser.file;
	DWORD bytesRead = 0;
	
	if (!ReadFile(file, buf, 4, &bytesRead, NULL)) {
		exitWinErr("Failed to read file")
	}
	
	SetFilePointer(file, 0, NULL, FILE_BEGIN);
	
	std::vector<BYTE> data;
	if (!readWholeFile(data, file, false, buf)) return;
	
	if (bytesRead == 4 && memcmp(buf, "FPAC", 4) == 0) {
		readFpacFromBinaryData(data, player);
	} else {
		readJsonFromText(data.data(), data.size(), player);
	}
	
	// fileCloser will close the file
	
	#undef exitWinErr
}

void UI::readJsonFromClipboard(int player) {
	char errorbuf[1024];
	#define exitWinErr(msg) { \
		WinError winErr; \
		sprintf_s(errorbuf, msg ": %ls", winErr.getMessage()); \
		showErrorDlgS(errorbuf); \
		return; \
	}
		
	if (!OpenClipboard(0)) {
		exitWinErr("OpenClipboard failed")
	}
	class ClipboardCloser {
	public:
		~ClipboardCloser() {
			if (!closedAlready) {
				CloseClipboard();
			}
		}
		bool closedAlready = false;
	} clipboardCloser;
	
	UINT format = 0;
	while (true) {
		format = EnumClipboardFormats(format);
		if (!format) {
			if (GetLastError() != ERROR_SUCCESS) {
				exitWinErr("Failed to enumerate clipboard formats")
			} else {
				break;
			}
		}
		if (format == CF_TEXT) {
			HANDLE cData = GetClipboardData(format);
			if (cData == NULL) {
				exitWinErr("Failed to get CF_TEXT format clipboard data")
			}
			UINT flags = GlobalFlags(cData);
			if ((flags & 0xff00) == GMEM_INVALID_HANDLE) {
				showErrorDlgS("Got invalid memory handle from clipboard data.");
				return;
			}
			SIZE_T size = GlobalSize(cData);
			if (!size) {
				exitWinErr("Failed on GlobalSize when trying to get clipboard data")
			}
			LPVOID lpPtr = GlobalLock(cData);
			if (!lpPtr) {
				exitWinErr("Failed on GlobalLock when trying to get clipboard data")
			}
			readJsonFromText((const BYTE*)lpPtr, size, player);
			GlobalUnlock(cData);
			break;
		}
	}
	#undef exitWinErr
}

bool UI::readWholeFile(std::vector<BYTE>& data, HANDLE file, bool addNullTerminator, char (&errorbuf)[1024]) {
	DWORD fileSizeHigh;
	DWORD fileSizeLow;
	fileSizeLow = GetFileSize(file, &fileSizeHigh);
	if (fileSizeLow == INVALID_FILE_SIZE && GetLastError() != NO_ERROR) {
		WinError winErr;
		sprintf_s(errorbuf, "Failed to get file size: %ls", winErr.getMessage());
		showErrorDlgS(errorbuf);
		return false;
	}
	if (fileSizeHigh || fileSizeLow >= 0x7FFFFFFE) {
		sprintf_s(errorbuf, "The file size is too big: %llu bytes", ((unsigned long long)fileSizeHigh << 32) | (unsigned long long)fileSizeLow);
		showErrorDlgS(errorbuf);
		return false;
	}
	
	try {
		data.resize(fileSizeLow + addNullTerminator);
	} catch (std::length_error& err) {
		(err);
		sprintf_s(errorbuf, "The file size is too big: %llu bytes", ((unsigned long long)fileSizeHigh << 32) | (unsigned long long)fileSizeLow);
		return false;
	} catch (std::bad_alloc& err) {
		(err);
		sprintf_s(errorbuf, "The file does not fit into RAM: %llu bytes", ((unsigned long long)fileSizeHigh << 32) | (unsigned long long)fileSizeLow);
		return false;
	}
	if (data.empty()) return true;
	BYTE* ptr = data.data();
	size_t remaining = fileSizeLow;
	DWORD bytesRead = 0;
	while (remaining) {
		DWORD bytesToRead = min(remaining, 1024);
		if (!ReadFile(file, ptr, bytesToRead, &bytesRead, NULL)) {
			WinError winErr;
			sprintf_s(errorbuf, "Failed to read file: %ls", winErr.getMessage());
			showErrorDlgS(errorbuf);
			return false;
		}
		if (bytesToRead != bytesRead) {
			// um... ok
			remaining -= bytesRead;
			if ((int)remaining > 0) {
				data.resize(data.size() - remaining);
			}
			if (addNullTerminator) {
				data.back() = '\0';
			}
			return true;
		}
		remaining -= bytesRead;
		ptr += bytesRead;
	}
	return true;
}

template<typename T>
static bool validateFpacImpl(const FPAC* fpac) {
	T* lookupEntry = (T*)((BYTE*)fpac + 0x20);
	for (int lookupEntryIndex = 0; lookupEntryIndex < (int)fpac->count; ++lookupEntryIndex) {
		if (lookupEntry->spriteName[31] != '\0') {
			sprintf_s(strbuf, "Lookup entry #%d has a sprite name that is not null-terminated.", lookupEntryIndex);
			showErrorDlgS(strbuf);
			return false;
		}
		if (lookupEntry->index != lookupEntryIndex) {
			sprintf_s(strbuf, "Lookup entry #%d's index does not match its declared index (%d).",
				lookupEntryIndex,
				lookupEntry->index);
			showErrorDlgS(strbuf);
			return false;
		}
		BYTE* jonbin = (BYTE*)fpac + fpac->headerSize + lookupEntry->offset;
		if (jonbin < (BYTE*)fpac || jonbin + lookupEntry->size > (BYTE*)fpac + fpac->rawSize) {
			sprintf_s(strbuf, "Lookup entry #%d offset and size point to a jonbin that goes out of bounds.", lookupEntryIndex);
			showErrorDlgS(strbuf);
			return false;
		}
		DWORD hash = Entity::hashStringLowercase(lookupEntry->spriteName);
		if (hash != lookupEntry->hash) {
			sprintf_s(strbuf, "Lookup entry #%d's hash does not match the actual hash of its sprite name.", lookupEntryIndex);
			showErrorDlgS(strbuf);
			return false;
		}
		for (const char* c = lookupEntry->spriteName; *c != '\0'; ++c) {
			char cVal = *c;
			if (cVal <= 32 || cVal > 126) {
				sprintf_s(strbuf, "Lookup entry #%d's sprite name contains characters that are not allowed (even a space isn't allowed).", lookupEntryIndex);
				showErrorDlgS(strbuf);
				return false;
			}
		}
		DWORD size = lookupEntry->size;
		if (lookupEntry->size < 4  // JONB
				+ 2  // name count
				+ 1  // number of types
		) {
			sprintf_s(strbuf, "Lookup entry #%d's declared JONBIN size is too small.", lookupEntryIndex);
			showErrorDlgS(strbuf);
			return false;
		}
		if (memcmp(jonbin, "JONB", 4) != 0) {
			sprintf_s(strbuf, "Lookup entry #%d's JONBIN does not start with \"JONB\".", lookupEntryIndex);
			showErrorDlgS(strbuf);
			return false;
		}
		jonbin += 4;
		size -= 4;
		short nameCount = *(short*)jonbin;
		if (nameCount > 7) {
			sprintf_s(strbuf, "Lookup entry #%d's JONBIN has more than 7 anim names.", lookupEntryIndex);
			showErrorDlgS(strbuf);
			return false;
		}
		jonbin += 2;
		size -= 2;
		for (short nameIndex = 0; nameIndex < nameCount; ++nameIndex) {
			if (size < 32) {
				sprintf_s(strbuf, "Lookup entry #%d's JONBIN size is not big enough to contain name #%d.", lookupEntryIndex, nameIndex);
				showErrorDlgS(strbuf);
				return false;
			}
			if (*(jonbin + 31) != '\0') {
				sprintf_s(strbuf, "Lookup entry #%d's JONBIN name #%d isn't null-terminated.", lookupEntryIndex, nameIndex);
				showErrorDlgS(strbuf);
				return false;
			}
			for (const char* c = (const char*)jonbin; *c != '\0'; ++c) {
				if (*c <= 32 || *c > 126) {
					sprintf_s(strbuf, "Lookup entry #%d's JONBIN name %d contains characters that are not allowed (even a space isn't allowed).",
						lookupEntryIndex, nameIndex);
					showErrorDlgS(strbuf);
					return false;
				}
			}
			jonbin += 32;
			size -= 32;
		}
		BYTE numTypes = *jonbin;
		if (numTypes < 3) {
			sprintf_s(strbuf, "Lookup entry #%d's JONBIN's number of types is too low.", lookupEntryIndex);
			showErrorDlgS(strbuf);
			return false;
		}
		++jonbin;
		--size;
		if (size < (DWORD)numTypes * 2) {
			sprintf_s(strbuf, "Lookup entry #%d's JONBIN's size is not big enough to hold all the hitbox counts.", lookupEntryIndex);
			showErrorDlgS(strbuf);
			return false;
		}
		short short1 = *(short*)jonbin;
		jonbin += 2;
		size -= 2;
		short short2 = *(short*)jonbin;
		jonbin += 2;
		size -= 2;
		short short3 = *(short*)jonbin;
		jonbin += 2;
		size -= 2;
		int hitboxCount = 0;
		numTypes -= 3;
		for (BYTE boxType = 0; boxType < numTypes; ++boxType) {
			hitboxCount += *(short*)jonbin;
			jonbin += 2;
			size -= 2;
		}
		int weirdCrap = short1 * FPAC::size1 + short2 * FPAC::size2 + short3 * FPAC::size3;
		if (size < weirdCrap + hitboxCount * sizeof (Hitbox)) {
			sprintf_s(strbuf, "Lookup entry #%d's JONBIN's size is not big enough to hold all the weird data it declared it has.", lookupEntryIndex);
			showErrorDlgS(strbuf);
			return false;
		}
		// we're done with size
		// nothing to validate in hitboxes
		++lookupEntry;
	}
	return true;
}

static bool validateFpac(const std::vector<BYTE>& data) {
	if (data.size() < 0x20) {
		showErrorDlgS("FPAC is too small.");
		return false;
	}
	const BYTE* ptr = data.data();
	if (memcmp(ptr, "FPAC", 4) != 0) {
		showErrorDlgS("FPAC magic number is wrong.");
		return false;
	}
	const FPAC* fpac = (const FPAC*)ptr;
	if (fpac->headerSize != fpac->count * fpac->elementSize() + 0x20) {
		showErrorDlgS("FPAC header size is wrong.");
		return false;
	}
	if (fpac->headerSize > data.size()) {
		showErrorDlgS("FPAC header size is too big.");
		return false;
	}
	if (fpac->rawSize > data.size()) {
		showErrorDlgS("FPAC 'rawSize' is too big.");
		return false;
	}
	if (fpac->flag2() || !fpac->useHash()) {
		showErrorDlgS("Don't know how to read this type of FPAC.");
		return false;
	}
	if (fpac->size0x50()) {
		return validateFpacImpl<FPACLookupElement0x50>(fpac);
	} else {
		return validateFpacImpl<FPACLookupElement0x30>(fpac);
	}
}

template<typename T>
struct BinaryFpacProvider : public SourceFpacProvier {
	const FPAC* fpac;
	T* current;
	int i;
	int count;
	virtual void rewind() {
		i = -1;
		current = (T*)((BYTE*)fpac + 0x20);
		--current;
	}
	virtual bool getNext() {
		if (i >= count - 1) {
			i = count;
			current = (T*)fpac->lookupEnd();
			return false;
		}
		++i;
		++current;
		return true;
	}
	// of current entry
	virtual DWORD getHash() const {
		return current->hash;
	}
	virtual const char (&getName() const)[32] {
		return current->spriteName;
	}
	virtual int getSize() const {
		return current->size;
	}
	virtual BYTE* getJonbin() const {
		return (BYTE*)fpac + fpac->headerSize + current->offset;
	}
	virtual void forgetJonbin()  // , it's mine now
	{
		throw std::logic_error("the whole FPAC is gonna be GONE YOU FOOL!!");
	}
	BinaryFpacProvider(const FPAC* fpac) : fpac(fpac) {
		current = (T*)((BYTE*)fpac + 0x20);
		--current;
		count = (int)fpac->count;
		i = -1;
	}
};

void UI::readFpacFromBinaryData(const std::vector<BYTE>& data, int player) {
	if (!validateFpac(data)) return;
	
	const FPAC* sourceFpac = (const FPAC*)data.data();
	FPACSecondaryData& secondaryData = hitboxEditorFPACSecondaryData[player];
	secondaryData.parseAllSprites();
	
	BYTE shutup[sizeof (BinaryFpacProvider<FPACLookupElement0x50>)];
	
	if (sourceFpac->size0x50()) {
		new (shutup) BinaryFpacProvider<FPACLookupElement0x50>(sourceFpac);
	} else {
		new (shutup) BinaryFpacProvider<FPACLookupElement0x30>(sourceFpac);
	}
	
	if (secondaryData.Collision->TopData->size0x50()) {
		secondaryData.replaceFpac<FPACLookupElement0x50, false>((SourceFpacProvier*)shutup);
	} else {
		secondaryData.replaceFpac<FPACLookupElement0x30, false>((SourceFpacProvier*)shutup);
	}
}

// the data might or might not be null-terminated
// dataSize may point past the null character, if any
void UI::readJsonFromText(const BYTE* data, size_t dataSize, int player) {
	
	std::vector<JSONParsedSprite> parsedSprites;
	
	// I feel like this is 100 times better than writing a move constructor and a destructor for JSONParsedSprite. std::vector, new[] suck and their model is not very suited for us
	struct OnExitFreeAllMemory {
		std::vector<JSONParsedSprite>& vec;
		OnExitFreeAllMemory(std::vector<JSONParsedSprite>& vec) : vec(vec) { }
		~OnExitFreeAllMemory() {
			for (JSONParsedSprite& sprite : vec) {
				if (sprite.jonbin) free(sprite.jonbin);
			}
		}
	} onExitFreeAllMemory{parsedSprites};
	
	if (!parseJson(data, &dataSize, parsedSprites)) return;
	struct MyProvider : public SourceFpacProvier {
		std::vector<JSONParsedSprite>& parsedSprites;
		JSONParsedSprite* current;
		int i;
		int count;
		virtual void rewind() {
			i = -1;
			current = parsedSprites.data() - 1;
		}
		virtual bool getNext() {
			if (i >= count - 1) {
				i = count;
				current = parsedSprites.data() + count;
				return false;
			}
			++i;
			++current;
			return true;
		}
		// of current entry
		virtual DWORD getHash() const {
			return Entity::hashStringLowercase(current->name);
		}
		virtual const char (&getName() const)[32] {
			return current->name;
		}
		virtual int getSize() const {
			return current->size;
		}
		virtual BYTE* getJonbin() const {
			return current->jonbin;
		}
		virtual void forgetJonbin()  // , it's mine now
		{
			current->jonbin = nullptr;
		}
		MyProvider(std::vector<JSONParsedSprite>& parsedSprites) : parsedSprites(parsedSprites) {
			current = parsedSprites.data() - 1;
			count = (int)parsedSprites.size();
			i = -1;
		}
	} provider { parsedSprites };
	
	FPACSecondaryData& secondaryData = hitboxEditorFPACSecondaryData[player];
	secondaryData.parseAllSprites();
	
	if (secondaryData.Collision->TopData->size0x50()) {
		secondaryData.replaceFpac<FPACLookupElement0x50, true>(&provider);
	} else {
		secondaryData.replaceFpac<FPACLookupElement0x30, true>(&provider);
	}
}

void UI::hitboxEditorCheckEntityStillAlive() {
	if (gifMode.editHitboxes && gifMode.editHitboxesEntity && !Entity{gifMode.editHitboxesEntity}.isActive()) {
		gifMode.editHitboxesEntity = nullptr;
	}
}

P1P2CommonTable::P1P2CommonTable() {
	for (const char* tableHeaderText : tableHeaderTexts) {
		ImGui::TableSetupColumn(tableHeaderText, ImGuiTableColumnFlags_WidthStretch, 1.F);
	}
	
	if (!tableHeaderTextWidthsKnown) {
		tableHeaderTextWidthsKnown = true;
		float* ptr = tableHeaderTextWidths;
		for (const char* tableHeaderText : tableHeaderTexts) {
			*ptr = ImGui::CalcTextSize(tableHeaderText).x;
			++ptr;
		}
	}
	
	tableCursorPos = ImGui::GetCursorPos();
}

P1P2CommonTable::~P1P2CommonTable() {
	ImVec2 windowPos = ImGui::GetWindowPos();
	float windowWidth = ImGui::GetWindowWidth();
	const ImGuiStyle& style = ImGui::GetStyle();
	const float fontSize = ImGui::GetFontSize();
	
	for (int column = 0; column < 2; ++column) {
		GGIcon icon = scaleGGIconToHeight(getCharIcon(entityList.slots[column].characterType()), fontSize);
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 imgStart {
			windowPos.x + style.WindowPadding.x + (windowWidth - style.WindowPadding.x * 2.F) / 3.F * (float)column
				+ tableHeaderTextWidths[column] + style.ItemInnerSpacing.x,
			windowPos.y + tableCursorPos.y + 2.F
		};
		drawList->PushClipRect({0.F,0.F},{10000.F,10000.F},false);
		drawList->AddImage(TEXID_GGICON, imgStart, {
			imgStart.x + icon.size.x,
			imgStart.y + icon.size.y
		}, icon.uvStart, icon.uvEnd, 0xFFFFFFFF);
		drawList->PopClipRect();
	}
}

bool UI::selectedHitboxesAlreadyAtTheBottom(SortedSprite* sortedSprite) {
	bool encounteredSelected = false;
	bool encounteredNonSelected = false;
	LayerIterator layerIterator(Entity{gifMode.editHitboxesEntity}, sortedSprite);
	while (layerIterator.getNext()) {
		if (!hitboxIsSelected(layerIterator.originalIndex)) {
			if (!encounteredSelected) return false;
			encounteredNonSelected = true;
		} else {
			if (encounteredNonSelected) {
				return false;
			}
			encounteredSelected = true;
		}
	}
	
	return true;
}

bool UI::selectedHitboxesAlreadyAtTheTop(SortedSprite* sortedSprite) {
	bool encounteredSelected = false;
	bool encounteredNonSelected = false;
	LayerIterator layerIterator(Entity{gifMode.editHitboxesEntity}, sortedSprite);
	layerIterator.scrollToEnd();
	while (layerIterator.getPrev()) {
		if (!hitboxIsSelected(layerIterator.originalIndex)) {
			if (!encounteredSelected) return false;
			encounteredNonSelected = true;
		} else {
			if (encounteredNonSelected) {
				return false;
			}
			encounteredSelected = true;
		}
	}
	
	return true;
}
