#pragma once
#include "pch.h"
#include <d3d9.h>
#include <vector>
#include <mutex>
#include "PngResource.h"
#include "TexturePacker.h"
#include "PlayerInfo.h"
#include <memory>
#include "StringWithLength.h"
#include "PackTextureSizes.h"

enum UITexture {
	TEXID_NONE,
	TEXID_IMGUIFONT,
	TEXID_GGICON,
	TEXID_FRAMES_HELP,
	TEXID_FRAMES_FRAMEBAR
};

enum FrameMarkerType {
	MARKER_TYPE_STRIKE_INVUL,
	MARKER_TYPE_SUPER_ARMOR,
	MARKER_TYPE_SUPER_ARMOR_FULL,
	MARKER_TYPE_THROW_INVUL,
	MARKER_TYPE_OTG,
	MARKER_TYPE_LAST
};

class UI
{
public:
	bool onDllMain(HMODULE hModule);
	void onDllDetachStage1_killTimer();
	void onDllDetachGraphics();
	void onDllDetachNonGraphics();
	void prepareDrawData();
	void onEndScene(IDirect3DDevice9* device, void* drawData, IDirect3DTexture9* iconTexture);
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
	bool visible = true;
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
	bool slowmoGame = false;
	bool continuousScreenshotToggle = false;
	std::mutex lock;
	bool imguiActive = false;
	void* drawData = nullptr;
	bool timerDisabled = false;
	void copyDrawDataTo(std::vector<BYTE>& destinationBuffer);
	void substituteTextureIDs(IDirect3DDevice9* device, void* drawData, IDirect3DTexture9* iconTexture);
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
		int framesCount = -1;
		int storedFramesCount = -1;
		int scrollXInFrames = 0;
		bool framebarAutoScroll = true;
	} framebarSettings;
	bool comboRecipeUpdatedOnThisFrame[2] { false, false };
	
	bool screenSizeKnown = false;
	float screenWidth = 0.F;
	float screenHeight = 0.F;
	bool usePresentRect = false;
	int presentRectW = 0;
	int presentRectH = 0;
	
	bool needUpdateGraphicsFramebarTexture = false;
	inline void getFramebarTexture(const PngResource** texture, const PackTextureSizes** sizes, bool* isColorblind) const {
		*texture = &packedTextureFramebar;
		*sizes = &lastPackedSize;
		*isColorblind = textureIsColorblind;
	}
private:
	void initialize();
	void initializeD3D(IDirect3DDevice9* device);
	bool imguiInitialized = false;
	bool imguiD3DInitialized = false;
	void keyComboControl(std::vector<int>& keyCombo);
	bool needWriteSettings = false;
	bool keyCombosChanged = false;
	char screenshotsPathBuf[MAX_PATH] { 0 };
	UINT_PTR timerId = 0;
	bool selectFile(std::wstring& path, HWND owner);
	std::wstring lastSelectedPath;
	static void __stdcall Timerproc(HWND unnamedParam1, UINT unnamedParam2, UINT_PTR unnamedParam3, DWORD unnamedParam4);
	static SHORT WINAPI hook_GetKeyState(int nVirtKey);
	void decrementFlagTimer(int& timer, bool& flag);
	void frameAdvantageControl(int frameAdvantage, int landingFrameAdvantage, bool frameAdvantageValid, bool landingFrameAdvantageValid, bool rightAlign);
	int frameAdvantageTextFormat(int frameAdv, char* buf, size_t bufSize);
	void frameAdvantageText(int frameAdv);
	bool showTensionData = false;
	bool showBurstGain = false;
	bool showSpeedsData = false;
	bool showProjectiles = false;
	bool showBoxExtents = false;
	bool showFrameAdvTooltip = false;
	bool showStartupTooltip = false;
	bool showActiveTooltip = false;
	bool showTotalTooltip = false;
	bool showInvulTooltip = false;
	void* hook_GetKeyStatePtr = nullptr;
	IDirect3DTexture9* imguiFont = nullptr;
	void onImGuiMessWithFontTexID();
	bool showCharSpecific[2] = { false, false };
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
	std::unique_ptr<PngResource> digitFrame[10];
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
	enum UITextureType {
		UITEX_HELP,
		UITEX_FRAMEBAR
	};
	void prepareSecondaryFrameArts(UITextureType type);
	void addFrameArt(HINSTANCE hModule, FrameType frameType, WORD resourceIdColorblind, std::unique_ptr<PngResource>& resourceColorblind,
                 WORD resourceIdNonColorblind, std::unique_ptr<PngResource>& resourceNonColorblind, StringWithLength description);
	void addFrameArt(HINSTANCE hModule, FrameType frameType, WORD resourceIdBothVersions, std::unique_ptr<PngResource>& resourceBothVersions, StringWithLength description);
	void addFrameMarkerArt(HINSTANCE hModule, FrameMarkerType markerType,
			WORD resourceIdBothVersions, std::unique_ptr<PngResource>& resourceBothVersions,
			DWORD outlineColorNonColorblind, DWORD outlineColorColorblind,
			bool hasMiddleLineNonColorblind, bool hasMiddleLineColorblind);
	bool addImage(HMODULE hModule, WORD resourceId, std::unique_ptr<PngResource>& resource);
	PngResource packedTextureHelp;  // do not change this once it is created
	void packTexture(PngResource& packedTexture, UITextureType type, const PackTextureSizes* sizes);
	void packTextureHelp();
	PngResource packedTextureFramebar;
	PackTextureSizes lastPackedSize;
	bool textureIsColorblind;
	void packTextureFramebar(const PackTextureSizes* sizes, bool isColorblind);
	void drawFramebars();
	bool showShaderCompilationError = true;
	const std::string* shaderCompilationError = nullptr;
	int two = 2;
	bool imguiActiveTemp = false;
	bool takeScreenshotTemp = false;
	bool showErrorDialog = false;
	const char* errorDialogText = nullptr;
	void* errorDialogPos = nullptr;
	bool showFramebarHelp = false;
	bool showBoxesHelp = false;
	void framebarHelpWindow();
	void hitboxesHelpWindow();
	bool booleanSettingPreset(std::atomic_bool& settingsRef);
	bool booleanSettingPresetWithHotkey(std::atomic_bool& settingsRef, std::vector<int>& hotkey);
	bool float4SettingPreset(float& settingsPtr);
	bool intSettingPreset(std::atomic_int& settingsPtr, int minValue, int step = 1, int stepFast = 1, float fieldWidth = 80.F, int maxValue = INT_MAX);
	bool showCancels[2] { false, false };
	bool showDamageCalculation[2] { false, false };
	bool showStunmash[2] { false, false };
	void drawSearchableWindows();
	bool searching = false;
	bool showSearch = false;
	char searchString[101] { '\0' };
	char searchStringOriginal[101] { '\0' };
	size_t searchStringLen = 0;
	size_t searchStep[256] { 0 };
	bool searchStringOk = false;
	bool showTooFewCharactersError = false;
	std::string lastFoundTextLeft;
	std::string lastFoundTextMid;
	std::string lastFoundTextRight;
	std::string searchStack[5] { };
	int searchStackCount = 0;
	std::string searchField{};
	struct SearchResult {
		std::string searchStack[5] { };
		int searchStackCount = 0;
		std::string field;
		std::string foundLeft;
		std::string foundMid;
		std::string foundRight;
	};
	std::list<SearchResult> searchResults;
	void pushSearchStack(const char* name);
	void popSearchStack();
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
	void HelpMarkerWithHotkey(const char* desc, const char* descEnd, std::vector<int>& hotkey);
	inline void HelpMarkerWithHotkey(const char* desc, std::vector<int>& hotkey) { HelpMarkerWithHotkey(desc, nullptr, hotkey); }
	template<size_t size> inline void HelpMarkerWithHotkey(const char(&desc)[size], std::vector<int>& hotkey) { HelpMarkerWithHotkey(desc, desc + size - 1, hotkey); }
	inline void HelpMarkerWithHotkey(const StringWithLength& desc, std::vector<int>& hotkey) { HelpMarkerWithHotkey(desc.txt, desc.txt + desc.length, hotkey); }
	inline void HelpMarkerWithHotkey(const std::string& desc, std::vector<int>& hotkey) { HelpMarkerWithHotkey(desc.c_str(), desc.c_str() + desc.size(), hotkey); }
	void printAllCancels(const FrameCancelInfo<30>& cancels,
		bool enableSpecialCancel,
		bool enableJumpCancel,
		bool enableSpecials,
		bool hitOccured,
		bool airborne,
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
	void startupOrTotal(int two, StringWithLength title, bool* showTooltipFlag);
	bool showComboDamage[2] { false };
	bool showComboRecipe[2] { false };
	bool showComboRecipeSettings[2] { false };
	bool settingsPresetsUseOutlinedText = false;
	std::string pixelShaderFailReason;
	bool pixelShaderFailReasonObtained = false;
	bool showingFailedHideRankSigscanMessage = false;
	void printLineOfResultOfHookingRankIcons(const char* placeName, bool result);
	bool framebarHadScrollbar = false;
};

extern UI ui;
