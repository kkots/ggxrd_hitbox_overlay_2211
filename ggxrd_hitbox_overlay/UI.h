#pragma once
#include "pch.h"
#include <d3d9.h>
#include <vector>
#include "PngResource.h"
#include "TexturePacker.h"
#include "PlayerInfo.h"
#include <memory>
#include "StringWithLength.h"
#include "PackTextureSizes.h"
#include "Moves.h"
#include "characterTypes.h"
#include <array>
#include <atlbase.h>
#include "PinnedWindowList.h"
#include "EditedHitbox.h"
#include <map>
#include <unordered_set>
#include "DrawHitboxArrayCallParams.h"
#include "DrawBoxCallParams.h"
#include "SortedSprite.h"
#include "LayerIterator.h"
#include "UndoOperations.h"
#include "ThreadUnsafeSharedPtr.h"

enum FrameMarkerType {
	MARKER_TYPE_STRIKE_INVUL,
	MARKER_TYPE_SUPER_ARMOR,
	MARKER_TYPE_SUPER_ARMOR_FULL,
	MARKER_TYPE_THROW_INVUL,
	MARKER_TYPE_OTG,
	MARKER_TYPE_LAST
};

struct DigitFrame {
	std::unique_ptr<PngResource> thickness[2] { std::unique_ptr<PngResource>{nullptr}, std::unique_ptr<PngResource>{nullptr} };
};

struct ButtonSettings {
	DWORD up;
	DWORD down;
	DWORD left;
	DWORD right;
	DWORD punch;
	DWORD kick;
	DWORD slash;
	DWORD heavySlash;
	DWORD dust;
	DWORD taunt;
	DWORD special;
	DWORD pkMacro;
	DWORD pksMacro;
	DWORD pkshMacro;
	DWORD shMacro;
	DWORD hdMacro;
	DWORD play;
	DWORD record;
	DWORD menu;
	DWORD unknown;
};

class UI
{
public:
	bool onDllMain();
	void onDllDetachStage1_killTimer();
	void onDllDetachGraphics();
	void onDllDetachNonGraphics();
	void prepareDrawData();
	void onEndScene(IDirect3DDevice9* device, void* drawData, IDirect3DTexture9* iconTexture, bool needsFramesTextureFramebar, bool needsFramesTextureHelp);
	LRESULT WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void handleResetBefore();
	void handleResetAfter();
	char* printDecimal(int num, int numAfterPoint, int padding, bool percentage = false);
	inline bool getFramebarAutoScroll() const { return framebarAutoScroll; }
	inline float getFramebarScrollX() const { return framebarScrollX; }
	inline float getFramebarMaxScrollX() const { return framebarMaxScrollX; }
	
	struct FrameDims {
		float x;
		float width;
		bool hitConnectedShouldBeAlt;
	};
	bool stateChanged = false;
	bool gifModeOn = false;
	bool gifModeToggleBackgroundOnly = false;
	bool gifModeToggleCameraCenterOnly = false;
	bool toggleCameraCenterOpponent = false;
	bool gifModeToggleHideOpponentOnly = false;
	bool toggleHidePlayer = false;
	bool gifModeToggleHudOnly = false;
	bool noGravityOn = false;
	bool freezeGame = false;
	bool allowNextFrame = false;
	int allowNextFrameTimer = 0;
	bool takeScreenshot = false;
	bool takeScreenshotPress = false;
	int takeScreenshotTimer = 0;
	bool clearTensionGainMaxCombo[2] { false };
	int clearTensionGainMaxComboTimer[2] { 0 };
	bool clearBurstGainMaxCombo[2] { false };
	int clearBurstGainMaxComboTimer[2] { 0 };
	bool continuousScreenshotToggle = false;
	bool imguiActive = false;
	void* drawData = nullptr;
	bool timerDisabled = false;
	void copyDrawDataTo(std::vector<BYTE>& destinationBuffer);
	void substituteTextureIDs(IDirect3DDevice9* device, void* drawData, IDirect3DTexture9* iconTexture, bool needsFramesTextureFramebar, bool needsFramesTextureHelp);
	void drawPlayerFrameTooltipInfo(const PlayerFrame& frame, int playerIndex, float wrapWidth);
	void drawPlayerFrameInputsInTooltip(const PlayerFrame& frame, int playerIndex);
	bool pauseMenuOpen = false;
	bool isDisplayingOnTop = false;
	bool drewFramebar = false;
	bool drewFrameTooltip = false;
	bool drawingPostponed = false;
	bool needSplitFramebar = false;
	void getFramebarDrawData(std::vector<BYTE>& dData);
	std::vector<BYTE> framebarTooltipDrawDataCopy;
	bool needShowFramebar() const;
	bool needShowFramebarCached = false;
	// At the time of calling this function FramebarSettings framebarSettings must already be updated.
	void onFramebarReset();
	// At the time of calling this function FramebarSettings framebarSettings must already be updated.
	void onFramebarAdvanced();
	// Since UI may change some of the settings right before it draws the framebar,
	// while combined projectile framebars may have been prepared or not prepared
	// according to settings before the change, we need to store a snapshot
	// of those settings that are needed to draw the framebar and which
	// are used in both EndScene::prepareDrawData() and UI::drawFramebars().
	// These values are filled in by EndScene::prepareDrawData().
	struct FramebarSettings {
		bool neverIgnoreHitstop = false;
		bool eachProjectileOnSeparateFramebar = false;
		bool condenseIntoOneProjectileFramebar = false;
		int framesCount = -1;
		int storedFramesCount = -1;
		int scrollXInFrames = 0;
		bool framebarAutoScroll = true;
	} framebarSettings;
	bool comboRecipeUpdatedOnThisFrame[2] { false, false };
	
	bool needUpdateGraphicsFramebarTexture = false;
	inline void getFramebarTexture(const PngResource** texture, const PackTextureSizes** sizes, bool* isColorblind) const {
		*texture = &packedTextureFramebar;
		*sizes = &lastPackedSize;
		*isColorblind = textureIsColorblind;
	}
	bool needTestDelay = false;
	bool needTestDelayStage2 = false;
	bool hasTestDelayResult = false;
	DWORD testDelayResult = 0;
	bool needTestDelay_dontReleaseKey = false;
	unsigned long long testDelayStart = 0;
	DWORD punchCode = 0;
	void onAswEngineDestroyed();
	void highlightedMovesChanged();
	void pinnedWindowsChanged();
	void setOpen(PinnedWindowEnum index, bool isOpen, bool isManual);
	void toggleOpen(PinnedWindowEnum index, bool isManual);
	inline void toggleOpenManually(PinnedWindowEnum index) { toggleOpen(index, true); }
	void onVisibilityToggleKeyboardShortcutPressed();
	bool isVisible() const;
	void onDisablePinButtonChanged(bool postedFromOutsideUI);
	bool lastCustomBeginHadPinButton = false;
	void startHitboxEditMode();
	void stopHitboxEditMode(bool resetEntity = true);
	inline void restartHitboxEditMode() { stopHitboxEditMode(false); startHitboxEditMode(); }
	void onToggleHitboxEditMode();
	std::vector<EditedHitbox> hitboxEditorLayers;
	LayerIterator getEntityLayers(Entity ent);
	bool hitboxIsSelectedForEndScene(int hitboxIndex) const;
	bool hitboxIsSelected(int hitboxIndex) const;
	BoxPart hitboxHoveredPart(int hitboxIndex) const;
	void getMousePos(float* pos) const;
	bool hasOverallSelectionBox(RECT* bounds, BoxPart* hoverPart);
	void editHitboxesProcessControls();
	struct BoxSelectBox : public EditedHitbox {
		RECT bounds;
	};
	void editHitboxesChangeView(Entity newEditEntity);
	inline void clearSelectedHitboxes() { selectedHitboxes.clear(); }
	inline void setSelectedHitboxes(const std::vector<int>& source) { selectedHitboxes = source; }
	inline void setSelectedHitboxes(std::vector<int>&& source) { selectedHitboxes = source; }
	inline void onNewSprite() {
		resetKeyboardMoveCache();
		resetCoordsMoveCache();
	}
	void showErrorDlg(const char* msg, bool isManual);
	void onAddBoxWithoutLayers(BYTE* jonbin);
	void activateQuickCharSelect();
	inline bool quickCharSelectVisible() { return windows[PinnedWindowEnum_QuickCharacterSelect].isOpen; }
	bool isVisibleAnything();
private:
	friend class UndoOperationBase;
	friend class DeleteLayersOperation;
	friend struct LayerIterator;
	void initialize();
	void initializeD3D(IDirect3DDevice9* device);
	bool imguiInitialized = false;
	bool imguiD3DInitialized = false;
	void keyComboControl(std::vector<int>& keyCombo);
	bool needWriteSettings = false;
	bool keyCombosChanged = false;
	char screenshotsPathBuf[MAX_PATH] { 0 };
	UINT_PTR timerId = 0;
	enum SelectFileMode {
		SELECT_FILE_MODE_READ,
		SELECT_FILE_MODE_WRITE
	};
	bool selectFile(std::wstring& path, HWND owner, const wchar_t* filterStr, std::wstring& lastSelectedPath, SelectFileMode selectMode);
	std::wstring lastSelectedScreenshotPath;
	static void __stdcall Timerproc(HWND unnamedParam1, UINT unnamedParam2, UINT_PTR unnamedParam3, DWORD unnamedParam4);
	static SHORT WINAPI hook_GetKeyState(int nVirtKey);
	void decrementFlagTimer(int& timer, bool& flag);
	void frameAdvantageControl(int frameAdvantage, int landingFrameAdvantage, bool frameAdvantageValid, bool landingFrameAdvantageValid, bool rightAlign);
	int frameAdvantageTextFormat(int frameAdv, char* buf, size_t bufSize);
	void frameAdvantageText(int frameAdv);
	void* hook_GetKeyStatePtr = nullptr;
	IDirect3DTexture9* imguiFont = nullptr;
	CComPtr<IDirect3DTexture9> imguiFontAlt;
	bool attemptedCreatingAltFont = false;
	void clearImGuiFontAlt();
	CComPtr<IDirect3DTexture9> pinTexture;
	bool attemptedCreatingPin = false;
	void clearPinTexture();
	void clearSecondaryTextures();
	void onImGuiMessWithFontTexID(IDirect3DDevice9* device);
	std::unique_ptr<PngResource> activeFrame;
	std::unique_ptr<PngResource> activeFrameNonColorblind;
	std::unique_ptr<PngResource> activeFrameHitstop;
	std::unique_ptr<PngResource> activeFrameHitstopNonColorblind;
	std::unique_ptr<PngResource> activeFrameNewHit;
	std::unique_ptr<PngResource> activeFrameNewHitNonColorblind;
	std::unique_ptr<PngResource> startupFrame;
	std::unique_ptr<PngResource> startupFrameNonColorblind;
	std::unique_ptr<PngResource> startupFrameCanBlock;
	std::unique_ptr<PngResource> startupFrameCanBlockNonColorblind;
	std::unique_ptr<PngResource> recoveryFrame;
	std::unique_ptr<PngResource> recoveryFrameNonColorblind;
	std::unique_ptr<PngResource> recoveryFrameHasGatlings;
	std::unique_ptr<PngResource> recoveryFrameHasGatlingsNonColorblind;
	std::unique_ptr<PngResource> recoveryFrameCanAct;
	std::unique_ptr<PngResource> recoveryFrameCanActNonColorblind;
	std::unique_ptr<PngResource> nonActiveFrame;
	std::unique_ptr<PngResource> nonActiveFrameNonColorblind;
	std::unique_ptr<PngResource> projectileFrame;
	std::unique_ptr<PngResource> projectileFrameNonColorblind;
	std::unique_ptr<PngResource> landingRecoveryFrame;
	std::unique_ptr<PngResource> landingRecoveryFrameNonColorblind;
	std::unique_ptr<PngResource> landingRecoveryFrameCanCancel;
	std::unique_ptr<PngResource> landingRecoveryFrameCanCancelNonColorblind;
	std::unique_ptr<PngResource> idleFrame;
	std::unique_ptr<PngResource> idleFrameCantBlockNonColorblind;
	std::unique_ptr<PngResource> idleFrameCantBlock;
	std::unique_ptr<PngResource> idleFrameCantFDNonColorblind;
	std::unique_ptr<PngResource> idleFrameCantFD;
	std::unique_ptr<PngResource> idleFrameElpheltRifleNonColorblind;
	std::unique_ptr<PngResource> idleFrameElpheltRifle;
	std::unique_ptr<PngResource> idleFrameElpheltRifleCanStopHoldingNonColorblind;
	std::unique_ptr<PngResource> idleFrameElpheltRifleCanStopHolding;
	std::unique_ptr<PngResource> strikeInvulFrame;
	std::unique_ptr<PngResource> throwInvulFrame;
	std::unique_ptr<PngResource> OTGFrame;
	std::unique_ptr<PngResource> superArmorFrame;
	std::unique_ptr<PngResource> superArmorFrameNonColorblind;
	std::unique_ptr<PngResource> superArmorFrameFull;
	std::unique_ptr<PngResource> superArmorFrameFullNonColorblind;
	DigitFrame digitFrame[10];
	std::unique_ptr<PngResource> xstunFrame;
	std::unique_ptr<PngResource> xstunFrameNonColorblind;
	std::unique_ptr<PngResource> xstunFrameCanCancel;
	std::unique_ptr<PngResource> xstunFrameCanCancelNonColorblind;
	std::unique_ptr<PngResource> xstunFrameHitstop;
	std::unique_ptr<PngResource> xstunFrameHitstopNonColorblind;
	std::unique_ptr<PngResource> graybeatAirHitstunFrame;
	std::unique_ptr<PngResource> graybeatAirHitstunFrameNonColorblind;
	std::unique_ptr<PngResource> zatoBreakTheLawStage2Frame;
	std::unique_ptr<PngResource> zatoBreakTheLawStage2FrameNonColorblind;
	std::unique_ptr<PngResource> zatoBreakTheLawStage3Frame;
	std::unique_ptr<PngResource> zatoBreakTheLawStage3FrameNonColorblind;
	std::unique_ptr<PngResource> zatoBreakTheLawStage2ReleasedFrame;
	std::unique_ptr<PngResource> zatoBreakTheLawStage2ReleasedFrameNonColorblind;
	std::unique_ptr<PngResource> zatoBreakTheLawStage3ReleasedFrame;
	std::unique_ptr<PngResource> zatoBreakTheLawStage3ReleasedFrameNonColorblind;
	std::unique_ptr<PngResource> eddieIdleFrame;
	std::unique_ptr<PngResource> eddieIdleFrameNonColorblind;
	std::unique_ptr<PngResource> bacchusSighFrame;
	std::unique_ptr<PngResource> bacchusSighFrameNonColorblind;
	std::unique_ptr<PngResource> backdashRecoveryFrame;
	std::unique_ptr<PngResource> normalLandingRecoveryFrame;
	std::unique_ptr<PngResource> pinResource;
	enum UITextureType {
		UITEX_HELP,
		UITEX_FRAMEBAR
	};
	void prepareSecondaryFrameArts(UITextureType type);
	void addFrameArt(FrameType frameType, WORD resourceIdColorblind, std::unique_ptr<PngResource>& resourceColorblind,
                 WORD resourceIdNonColorblind, std::unique_ptr<PngResource>& resourceNonColorblind, StringWithLength description);
	void addFrameArt(FrameType frameType, WORD resourceIdBothVersions, std::unique_ptr<PngResource>& resourceBothVersions, StringWithLength description);
	void addFrameMarkerArt(FrameMarkerType markerType,
			WORD resourceIdBothVersions, std::unique_ptr<PngResource>& resourceBothVersions,
			DWORD outlineColorNonColorblind, DWORD outlineColorColorblind,
			bool hasMiddleLineNonColorblind, bool hasMiddleLineColorblind);
	bool addImage(WORD resourceId, std::unique_ptr<PngResource>& resource);
	bool addDigit(WORD resourceId, WORD resourceIdThickness1, DigitFrame& digit);
	PngResource packedTextureHelp;  // do not change this once it is created
	void packTexture(PngResource& packedTexture, UITextureType type, const PackTextureSizes* sizes);
	void packTextureHelp();
	PngResource packedTextureFramebar;
	PackTextureSizes lastPackedSize;
	bool textureIsColorblind;
	void packTextureFramebar(const PackTextureSizes* sizes, bool isColorblind);
	void drawFramebars();
	const std::string* shaderCompilationError = nullptr;
	int two = 2;
	bool imguiActiveTemp = false;
	bool takeScreenshotTemp = false;
	const char* errorDialogText = nullptr;
	float errorDialogPos[2];  // should be enough to hold an ImVec2
	void framebarHelpWindow();
	void hitboxesHelpWindow();
	bool booleanSettingPreset(bool& settingsRef);
	bool booleanSettingPresetWithHotkey(bool& settingsRef, std::vector<int>& hotkey);
	inline bool floatSettingPreset(float& settingsPtr, float minValue = FLT_MIN, float maxValue = FLT_MAX, float step = 1.F, float stepFast = 10.F, float width = 120.F) {
		return float4SettingPreset(settingsPtr, minValue, maxValue, step, stepFast, width);
	}
	bool float4SettingPreset(float& settingsPtr, float minValue = FLT_MIN, float maxValue = FLT_MAX, float step = 1.F, float stepFast = 10.F, float width = 120.F);
	bool intSettingPreset(int& settingsPtr, int minValue, int step = 1, int stepFast = 1, float fieldWidth = 80.F, int maxValue = INT_MAX, bool isDisabled = false);
	bool colorSettingPreset(DWORD& settingsRef, bool withAlpha);
	void drawSearchableWindows();
	bool searching = false;
	char searchString[101] { '\0' };
	char searchStringOriginal[101] { '\0' };
	size_t searchStringLen = 0;
	size_t searchStep[256] { 0 };
	bool searchStringOk = false;
	bool showTooFewCharactersError = false;
	std::string lastFoundTextLeft;
	std::string lastFoundTextMid;
	std::string lastFoundTextRight;
	std::string searchStack[5] { std::string{}, std::string{}, std::string{}, std::string{}, std::string{} };
	int searchStackCount = 0;
	std::string searchField{};
	struct SearchResult {
		std::string searchStack[5] { std::string{}, std::string{}, std::string{}, std::string{}, std::string{} };
		int searchStackCount = 0;
		std::string field;
		std::string foundLeft;
		std::string foundMid;
		std::string foundRight;
	};
	std::list<SearchResult> searchResults;
	void pushSearchStack(const char* name);
	void popSearchStack();
	inline const char* searchCollapsibleSection(PinnedWindowEnum index) {
		const std::string& title = windows[index].title;
		return searchCollapsibleSection(title.c_str(), title.c_str() + title.size());
	}
	template<size_t size> inline const char* searchCollapsibleSection(const char(&txt)[size]) { return searchCollapsibleSection(txt, txt + size - 1); }
	template<size_t size> inline const char* searchFieldTitle(const char(&txt)[size]) { return searchFieldTitle(txt, txt + size - 1); }
	template<size_t size> inline const char* searchTooltip(const char(&txt)[size]) { return searchTooltip(txt, txt + size - 1); }
	template<size_t size> inline const char* searchFieldValue(const char(&txt)[size]) { return searchFieldValue(txt, txt + size - 1); }
	const char* searchCollapsibleSection(const char* collapsibleHeaderName, const char* textEnd);
	const char* searchFieldTitle(const char* fieldTitle, const char* textEnd);
	const char* searchTooltip(const char* tooltip, const char* textEnd);
	const char* searchFieldValue(const char* value, const char* textEnd);
	inline const char* searchCollapsibleSection(const StringWithLength& txt) { return searchCollapsibleSection(txt.txt, txt.txt + txt.length); }
	inline const char* searchFieldTitle(const StringWithLength& txt) { return searchFieldTitle(txt.txt, txt.txt + txt.length); }
	inline const char* searchTooltip(const StringWithLength& txt) { return searchTooltip(txt.txt, txt.txt + txt.length); }
	inline const char* searchFieldValue(const StringWithLength& txt) { return searchFieldValue(txt.txt, txt.txt + txt.length); }
	inline const char* searchCollapsibleSectionStr(const std::string& txt) { return searchCollapsibleSection(txt.c_str(), txt.c_str() + txt.size()); }
	inline const char* searchFieldTitleStr(const std::string& txt) { return searchFieldTitle(txt.c_str(), txt.c_str() + txt.size()); }
	inline const char* searchTooltipStr(const std::string& txt) { return searchTooltip(txt.c_str(), txt.c_str() + txt.size()); }
	inline const char* searchFieldValueStr(const std::string& txt) { return searchFieldValue(txt.c_str(), txt.c_str() + txt.size()); }
	const char* searchRawText(const char* txt, const char* txtStart, const char** txtEnd);
	void searchRawTextMultiResult(const char* txt, const char* txtEnd = nullptr);
	const char* rewindToNextUtf8CharStart(const char* ptr, const char* textStart);
	const char* skipToNextUtf8CharStart(const char* ptr, const char* textEnd);
	const char* skipToNextUtf8CharStart(const char* ptr);
	void searchWindow();
	void AddTooltipWithHotkey(const char* desc, const char* descEnd, std::vector<int>& hotkey);
	void AddTooltipWithHotkeyNoSearch(const char* desc, std::vector<int>& hotkey);
	void HelpMarkerWithHotkey(const char* desc, const char* descEnd, std::vector<int>& hotkey);
	inline void HelpMarkerWithHotkey(const char* desc, std::vector<int>& hotkey) { HelpMarkerWithHotkey(desc, nullptr, hotkey); }
	template<size_t size> inline void HelpMarkerWithHotkey(const char(&desc)[size], std::vector<int>& hotkey) { HelpMarkerWithHotkey(desc, desc + size - 1, hotkey); }
	inline void HelpMarkerWithHotkey(const StringWithLength& desc, std::vector<int>& hotkey) { HelpMarkerWithHotkey(desc.txt, desc.txt + desc.length, hotkey); }
	inline void HelpMarkerWithHotkey(const std::string& desc, std::vector<int>& hotkey) { HelpMarkerWithHotkey(desc.c_str(), desc.c_str() + desc.size(), hotkey); }
	template<typename T>
	void printAllCancels(const T& cancels,
		bool enableSpecialCancel,
		bool clashCancelTimer,
		bool enableJumpCancel,
		bool enableSpecials,
		bool hitOccurred,
		bool airborne,
		const char* canYrc,
		bool insertSeparators,
		bool useMaxY);
	int printBaseDamageCalc(const DmgCalc& dmgCalc, int* dmgWithHpScale);
	void printAttackLevel(const DmgCalc& dmgCalc);
	std::vector<BYTE> framebarWindowDrawDataCopy;
	std::vector<BYTE> framebarHorizontalScrollbarDrawDataCopy;
	bool showHowFlickingWorks[2] { false };
	const char* faust5DHelp = "The inner circle shows the range in which the thrown item's origin point must be in order to achieve a homerun hit."
			" The outer circles shows the range in which the thrown item's origin point must be in order to achieve a non-homerun"
			" hit. This display needs to be enabled with \"showFaustOwnFlickRanges\" setting.\n"
			"The circle is displayed for more than one frame, but it is filled in on only one."
			" The frame when it is filled in is when it is active, and the other frames are just for visual clarity,"
			" so you could have enough time to actually see the circle.";
	std::string faust5D;
	void printChargeInFrameTooltip(const char* title, unsigned char value, unsigned char valueMax, unsigned char valueLast);
	void printChargeInCharSpecific(int playerIndex, bool showHoriz, bool showVert, int maxCharge);
	bool printSlayerBuffedMoves[2] { false };
	bool printElpheltShotgunPowerup[2] { false };
	BYTE* jamPantyPtr = nullptr;
	bool printRavenBuffedMoves[2] { false };
	bool printHowAzamiWorks[2] { false };
	bool selectingFrames = false;
	int selectedFrameStart = -1;
	// Inclusive.
	int selectedFrameEnd = -1;
	int findHoveredFrame(float x, FrameDims* dims);
	void drawRightAlignedP1TitleWithCharIcon();
	float framebarScrollX = 0.F;  // this scroll is obsolete 2 frames. We store this because we keep framebar's horizontal scrollbar in a separate window
	float framebarMaxScrollX = 0.F;  // this scroll max is obsolete 2 frames. We store this because framebar window might get resized and our scroll is from 2 frames ago
	bool framebarAutoScroll = false;
	void resetFrameSelection();
	bool dontUsePreBlockstunTime = false;
	void adjustMousePosition();
	void startupOrTotal(int two, StringWithLength title, PinnedWindowEnum windowIndex);
	bool showComboRecipeSettings[2] { false };
	bool settingsPresetsUseOutlinedText = false;
	std::string pixelShaderFailReason;
	bool pixelShaderFailReasonObtained = false;
	void printLineOfResultOfHookingRankIcons(const char* placeName, bool result);
	bool framebarHadScrollbar = false;
	bool sigscannedButtonSettings = false;
	ButtonSettings* buttonSettings = nullptr;
	bool needToDivertCodeInGetKeyState = false;
	bool needTestDelayOnNextUiFrame = false;  // sigscanning takes ~7ms
	void testDelay();
	bool idiotPressedTestDelayButtonOutsideBattle = false;
	void printBedmanSeals(const BedmanInfo& bedmanInfo, bool forFrameTooltip);
	bool showPotCanFlick[2] { false, false };
	bool showPotCantFlick[2] { false, false };
	bool showPotCantFlickIncludeSupers[2] { false, false };
	bool showFaustCanFlick[2] { false, false };
	bool showFaustCantFlick[2] { false, false };
	bool showFaustCantFlickIncludeSupers[2] { false, false };
	bool showZatoReflectableProjectiles[2] { false, false };
	bool showLeoReflectableProjectiles[2] { false, false };
	bool showDizzyReflectableProjectiles[2] { false, false };
	struct SortedMovesEntry {
		const char* name = nullptr;
		const NamePair* displayName = nullptr;
		int index = -1;
		void const* comparisonValue = nullptr;
		inline SortedMovesEntry(const char* name, const NamePair* displayName, int index, void const* comparisonValue)
			: name(name), displayName(displayName), index(index), comparisonValue(comparisonValue) { }
	};
	std::array<std::vector<SortedMovesEntry>, CHARACTER_TYPE_ANSWER + 1> sortedMoves;
	static int __cdecl CompareMoveInfo(void const* moveLeft, void const* moveRight);
	bool sortedMovesRedoPending = true;
	bool sortedMovesRedoPendingWhenAswEngingExists = true;
	BYTE* fontData = nullptr;
	// do not change this once fontDataAlt is ready
	// do not submit an FRenderCommand for rendering ImGui until this is ready
	int fontDataWidth = 0;
	// do not change this once fontDataAlt is ready
	// do not submit an FRenderCommand for rendering ImGui until this is ready
	int fontDataHeight = 0;
	// do not change this once it is filled in
	// do not submit an FRenderCommand for rendering ImGui until this is ready
	std::vector<BYTE> fontDataAlt;
	struct WindowStruct {
		PinnedWindowEnum index;
		bool isOpen = false;
		bool openedJustNow = false;
		std::string title;
		std::string searchTitle;
		PinnedWindowElement* element = nullptr;
		bool isPinned() const { return element->isPinned; }
		void setPinned(bool isPinned) { element->isPinned = isPinned; }
		void setOpen(bool newOpen, bool isManual);
		inline void toggleOpen(bool isManual) { setOpen(!isOpen, isManual); }
		WindowStruct(PinnedWindowEnum index, const char* titleFmtString, ...);
		void init();
	};
	WindowStruct windows[PinnedWindowEnum_Last] {
		#define pinnableWindowsFunc(name, title) {PinnedWindowEnum_##name, title},
		#define pinnableWindowsPairFunc(name, titleFmtString) \
			{PinnedWindowEnum_##name##_1, titleFmtString, 1}, \
			{PinnedWindowEnum_##name##_2, titleFmtString, 2},
		pinnableWindowsEnum
		#undef pinnableWindowsFunc
		#undef pinnableWindowsPairFunc
	};
	void prepareOutlinedFont();
	void readPinnedUniversal(PinnedWindowEnum index);
	void customBegin(PinnedWindowEnum index);
	void customEnd();
	bool needDraw(PinnedWindowEnum index) const;
	enum WindowShowMode {
		WindowShowMode_All,
		WindowShowMode_Pinned,
		WindowShowMode_None
	} windowShowMode = WindowShowMode_All;
	bool lastCustomBeginPushedAStyle = false;
	bool lastCustomBeginPushedOutlinedText = false;
	bool lastCustomBeginPushedAStyleStack[10] { false };
	bool lastCustomBeginPushedOutlinedTextStack[10] { false };
	bool lastCustomBeginHadPinButtonStack[10] { false };
	bool lastWindowClosedStack[10] { false };
	int lastCustomBeginPushedStackDepth = 0;
	PinnedWindowEnum lastCustomBeginIndex = (PinnedWindowEnum)-1;
	bool lastWindowClosed = false;
	bool hasAtLeastOnePinnedOpenWindow() const;
	bool hasAtLeastOnePinnedWindowThatIsNotTheMainWindow() const;
	bool hasAtLeastOneUnpinnedOpenWindow() const;
	void drawHitboxEditor();
	void freezeGameAndAllowNextFrameControls();
	bool showHitboxEditorSettings = false;
	HitboxType hitboxEditorCurrentHitboxType = HITBOXTYPE_HURTBOX;
	std::string errorMsgBuf;
	float hitboxEditorRefCol[4];
	struct SortedAnimSeq {
		FName fname;
		char buf[32  // 32 chars max
			+ 11  // longest possible number part
			+ 1];  // null character
		inline SortedAnimSeq(FName fname) : fname(fname) { }
		SortedAnimSeq(FName fname, const char* str);
	};
	std::vector<SortedAnimSeq> sortedAnimSeqs;
	static int findInsertionIndexUnique(const std::vector<SortedAnimSeq>& ar, const FName* value, const char* str);
	struct ComboBoxPopupExtension {
		bool comboBoxBeganThisFrame = false;
		int selectedIndex = -1;
		bool appearing = false;
		size_t totalCount = 0;
		inline void requestAutoScroll() { appearing = true; }
		void onComboBoxBegin();
		bool fastScrollWithKeys();
		void endFrame();
		inline void beginFrame() { comboBoxBeganThisFrame = false; }
	} comboBoxExtension, comboBoxExtensionQuickCharSelect;
	// 3 elements, because one for P1, one for P2 and one for cmn_ef
	std::array<FPACSecondaryData, 3> hitboxEditorFPACSecondaryData;
	FPACSecondaryData& getSecondaryData(int bbscrIndexInAswEng);
	
	// when updating FPAC data you should also update:
	// Each Entity's::hitboxes() {
	//   jonbinPtr,
	//   ptrLookup,
	//   ptrRawAfterShort1,
	//   data,
	//   count
	//   short2
	//   names,
	//   nameCount
	// },
	// Each Entity's::fpac()
	// FPAC lookup table entry's offset
	// FPAC lookup table entry's index
	// FPAC headerSize
	// FPAC rawSize
	// FPAC lookup table entry's name
	// FPAC lookup table entry's name hash
	// FPAC lookup table entry's declared size of JONBIN data
	// REDAssetCollision's TopData. It gets set into Entity::fpac() on round restart and freed on aswEngine destruction.
	//     TopData is owned by GMalloc, so
	//     use EndScene.cpp's appFree to free and Game.cpp's appRealloc to relocate
	// If both Players are the same character, they share the same REDAssetCollision
	// Our things to update:
	// - sortedSprites - the char*'s in this array point to lookup entries
	// - jonbinToLookupUE3 - the pair.first points to a jonbin, pair.second points to lookup entry. The jonbins are located in UE3-owned memory. Lookup is always in the UE3-owned memory.
	// - oldNameToSortedSpriteIndex - maps an old, unmodified name's hash to an index in sortedSprites
	// - newHashMap - must contain only all hashes after all the sprite deletions and renames
	// - SortedSprite::layers - the 'ptr' field points to a Hitbox
	
	inline void parseAllSprites(Entity editEntity) {
		getSecondaryData(editEntity.bbscrIndexInAswEng()).parseAllSprites();
	}
	void detachFPAC();
	char renameSpriteBuf[32];
	char selectedHitboxesSprite[32] { 0 };
	std::vector<int> selectedHitboxes;
	std::vector<int> selectedHitboxesPreBoxSelect;
	std::vector<int> selectedHitboxesBoxSelect;  // filled by boxSelectProcessHitbox. The boxes that are getting selected by the click or box select.
	float boxSelectYStart = 0.F;
	float boxSelectYEnd = 0.F;
	bool boxSelectYSet = false;
	bool beginDragNDropFrame(SortedSprite* currentSpriteElement, bool childBegan);
	struct DragNDropItemInfo {
		float x;
		float y;
		float drawX;
		float drawY;
		int layerIndex;
		int originalIndex;
		HitboxType hitboxType;
		int hitboxIndex;
		DWORD color;
		bool isPushbox;
		bool active;
		bool selected;
		bool hovered;
		const char* printLabel() const;
		bool topLayer;
	};
	struct PrevDragNDropItemInfo {
		float x;
		float y;
		int originalIndex;
		bool topLayer;
	};
	std::vector<PrevDragNDropItemInfo> prevDragNDropItems;
	std::vector<DragNDropItemInfo> dragNDropItems;
	std::vector<DragNDropItemInfo> dragNDropItemsCarried;
	void dragNDropOnItem(SortedSprite* currentSpriteElement, DragNDropItemInfo* itemInfo);
	void dragNDropOnCarriedItem(SortedSprite* currentSpriteElement, DragNDropItemInfo* itemInfo);
	void processDragNDrop(SortedSprite* currentSpriteElement, int overallHitboxCount);
	void endDragNDrop();
	bool dragNDropBeganFrame = false;
	bool dragNDropActive = false;
	int dragNDropActiveIndex = -1;
	bool dragNDropActiveIndexWasSelected = false;
	enum MouseDragPurpose {
		MOUSEDRAGPURPOSE_NONE,
		MOUSEDRAGPURPOSE_BOX_SELECT,
		MOUSEDRAGPURPOSE_MOVE_ELEMENTS
	} dragNDropMouseDragPurpose;
	MouseDragPurpose dragNDropMouseDragPurposePending;
	bool dragNDropWasShiftHeld = false;
	bool dragNDropWasCtrlHeld = false;
	bool dragNDropChildBegan = false;
	int dragNDropDestinationIndex = -1;
	float dragNDropMouseClickedX = 0.F;
	float dragNDropMouseClickedY = 0.F;
	DragNDropItemInfo dragNDropActivePendingInfo;
	bool dragNDropActivePending = false;
	bool dragNDropActivePendingInfoIsNull = false;
	bool dragNDropChildNotBegan_mouseAboveWholeWindow = false;
	int dragNDropMouseIndex = -1;
	float dragNDropChildNotBegan_x = 0.F;
	float dragNDropChildNotBegan_y = 0.F;
	float dragNDropItemHeight = 0.F;
	int dragNDropInterpolationTimer = 0;
	static const int dragNDropInterpolationTimerMax = 20;
	static float dragNDropInterpolationAnim[dragNDropInterpolationTimerMax];
	static const int dragNDropInterpolationAnimRaw[dragNDropInterpolationTimerMax];
	// things to keep track of:
	// don't draw selectables that are clipped or if dragNDropChildBegan is false
	// reset all the things in endDragNDrop and when releasing the mouse
	void dragNDropDrawItem(const DragNDropItemInfo* item, float x, float y, float width, Moves::TriBool eye, BYTE* drawListPtr);
	bool dragNDropOperationWontDoAnything(SortedSprite* currentSpriteElement);
	bool boxMouseDown = false;
	void hitboxEditorBoxSelect();
	int boxSelectArenaXStartOrig = 0;
	int boxSelectArenaYStartOrig = 0;
	int boxSelectArenaXStart = 0;
	int boxSelectArenaYStart = 0;
	int boxSelectArenaXEnd = 0;
	int boxSelectArenaYEnd = 0;
	int boxSelectArenaXMin = 0;
	int boxSelectArenaYMin = 0;
	int boxSelectArenaXMax = 0;
	int boxSelectArenaYMax = 0;
	bool boxSelectShiftHeld = false;
	bool boxSelectingBoxesDragging = false;
	int boxSelectingOriginalIndex = -1;
	static unsigned int dist(int x1, int y1, int x2, int y2);
	int boxHoverRequestedCursor = -1;  // filled by boxSelectProcessHitbox. The cursor to ask ImGui to set. -1 if none
	int aswOneScreenPixelWidth = 0;
	int aswOneScreenPixelHeight = 0;
	int aswOneScreenPixelDiameter = 0;
	BoxPart boxHoverPart = BOXPART_NONE;  // filled by boxSelectProcessHitbox. The hovered part of the hovered box. BOXPART_NONE if none
	int boxHoverOriginalIndex = -1;  // filled by boxSelectProcessHitbox. The identifier of the hovered box. -1 if none
	enum HitboxEditorToolEnum {
		HITBOXEDITTOOL_SELECTION,
		HITBOXEDITTOOL_ADD_BOX
	} hitboxEditorTool = HITBOXEDITTOOL_SELECTION;
	bool hitboxIsSelectedPreBoxSelect(int originalIndex) const;
	bool hitboxEditPressedToggleEditMode = false;
	bool hitboxEditPressedToggleEditMode_isOn = false;
	bool hitboxEditNewSpritePressed = false;
	bool hitboxEditDeleteSpritePressed = false;
	bool hitboxEditRenameSpritePressed = false;
	bool hitboxEditSelectionToolPressed = false;
	bool hitboxEditRectToolPressed = false;
	bool hitboxEditRectDeletePressed = false;
	bool hitboxEditUndoPressed = false;
	bool hitboxEditRedoPressed = false;
	bool hitboxEditSendToBackPressed = false;
	bool hitboxEditSendBackwardsPressed = false;
	bool hitboxEditSendForwardsPressed = false;
	bool hitboxEditSendToFrontPressed = false;
	void hitboxEditProcessBackground();
	void hitboxEditProcessKeyboardShortcuts();
	void hitboxEditProcessPressedCommands();
	SortedSprite* hitboxEditFindCurrentSprite();
	SortedSprite* hitboxEditFindCurrentSprite(Entity ent);
	static const char* getSpriteRepr(SortedSprite* sortedSprite);
	bool popupsOpen = false;
	BoxPart boxResizePart = BOXPART_NONE;
	SortedSprite* boxResizeSortedSprite = nullptr;
	std::vector<Hitbox> boxResizeOldHitboxes;
	// this flag is needed, because if you click one box, then shift click another, then click just the other, we want to select just the other box.
	// But if you click one box, then shift click another, then resize both boxes, we want to keep them selected.
	bool boxResizeHappened = false;
	void boxResizeProcessBox(DrawHitboxArrayCallParams& params, Hitbox* hitboxesStart, Hitbox* ptr);
	void boxSelectProcessHitbox(BoxSelectBox& box);
	BoxSelectBox lastOverallSelectionBox;
	bool lastOverallSelectionBoxReady = false;
	BoxPart lastOverallSelectionBoxHoverPart = BOXPART_NONE;
	RECT overallSelectionBoxOldBounds;
	bool imguiContextMenuOpen = false;
	void editHitboxesProcessCamera();
	void editHitboxesProcessHitboxMoving();
	void editHitboxesConvertBoxes(DrawHitboxArrayCallParams& params, Entity editEntity, SortedSprite* sortedSprite);
	void editHitboxesFillParams(DrawHitboxArrayCallParams& params, Entity editEntity);
	std::vector<BoxSelectBox> convertedBoxes;
	bool convertedBoxesPrepared = false;
	int pushboxOriginalIndex();
	Entity keyboardMoveEntity = nullptr;
	SortedSprite* keyboardMoveSprite = nullptr;
	std::vector<BoxSelectBox> keyboardMoveBackupBoxes;
	int keyboardMoveX = 0;
	int keyboardMoveY = 0;
	void resetKeyboardMoveCache();
	SortedSprite* coordsSprite = nullptr;
	Entity coordsEntity = nullptr;
	inline void resetCoordsMoveCache() { coordsSprite = nullptr; coordsEntity = nullptr; }
	std::array<ThreadUnsafeSharedPtr<UndoOperationBase>, 100> undoRingBuffer;
	int undoRingBufferIndex = 0;
	ThreadUnsafeSharedPtr<UndoOperationBase>& allocateUndo();
	std::array<ThreadUnsafeSharedPtr<UndoOperationBase>, 100> redoRingBuffer;
	int redoRingBufferIndex = 0;
	ThreadUnsafeSharedPtr<UndoOperationBase>& allocateRedo();
	void castrateUndos();
	void castrateRedos();
	inline void eraseHistory() { castrateUndos(); castrateRedos(); }
	bool performOp(UndoOperationBase* op);
	struct MoveResizeBoxesUndoHelper {
		MoveResizeBoxesUndoHelper(Hitbox* hitboxesStart, int hitboxCount, int timerRange = 20);
		ThreadUnsafeSharedPtr<MoveResizeBoxesOperation>& prevUndo;
		Hitbox* hitboxesStart;
		int hitboxCount;
		std::vector<Hitbox> oldData;
		DWORD engineTick;
		bool prevUndoOk;
		void finish();
	};
	bool boxResizeChangedSomething = false;
	void requestFileSelect(const wchar_t* fileSpec, std::wstring& lastSelectedPath);
	void writeOutClipboard(const std::vector<BYTE>& data);
	void serializeCollision(std::vector<BYTE>& data, int player);
	void serializeJson(std::vector<BYTE>& data, int player);
	ThreadUnsafeSharedPtr<std::vector<BYTE>> dataToWriteToFile;  // once the user selects it
	void writeOutFile(const std::wstring& path, const std::vector<BYTE>& data);
	std::wstring lastSelectedCollisionPath;
	void makeFileSelectRequest(void(UI::*serializer)(std::vector<BYTE>& data, int player),
		const wchar_t* filterStr, int player, bool removeNullTerminator);
	void readCollisionFromFile(const std::wstring& path, int player);
	void readJsonFromClipboard(int player);
	inline bool readWholeFile(std::vector<BYTE>& data, HANDLE file, bool addNullTerminator, char (&errorbuf)[1024]);
	void readFpacFromBinaryData(const std::vector<BYTE>& data, int player);
	// the data might or might not be null-terminated
	// dataSize may point past the null character, if any
	void readJsonFromText(const BYTE* data, size_t dataSize, int player);
	float lastClickPosX = 50.F;
	float lastClickPosY = 50.F;
	void hitboxEditorCheckEntityStillAlive();
	bool selectedHitboxesAlreadyAtTheTop(SortedSprite* sortedSprite);
	bool selectedHitboxesAlreadyAtTheBottom(SortedSprite* sortedSprite);
	void hitboxEditorButton();
	bool drawQuickCharSelect(bool isWindow);
	bool quickCharSelectFocusRequested = false;
	bool drawQuickCharSelectControllerFriendly();
	uintptr_t getSelectedCharaLocation();
	void quickCharSelect_save(CharacterType newCharType);
	const char* getMostModDisabledMsg();
	// calls ImGui::TextUnformatted(...) if most of the mod is disabled
	bool isMostModDisabledPlusMsg();
};

extern UI ui;
