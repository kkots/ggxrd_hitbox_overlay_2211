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

enum UITexture {
	TEXID_NONE,
	TEXID_IMGUIFONT,
	TEXID_GGICON,
	TEXID_FRAMES
};

enum FrameMarkerType {
	MARKER_TYPE_STRIKE_INVUL,
	MARKER_TYPE_SUPER_ARMOR,
	MARKER_TYPE_SUPER_ARMOR_FULL,
	MARKER_TYPE_THROW_INVUL,
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
	bool slowmoGame = false;
	bool continuousScreenshotToggle = false;
	std::mutex lock;
	bool imguiActive = false;
	void* drawData = nullptr;
	bool timerDisabled = false;
	void copyDrawDataTo(std::vector<BYTE>& destinationBuffer);
	void substituteTextureIDs(IDirect3DDevice9* device, void* drawData, IDirect3DTexture9* iconTexture);
	const PngResource& getPackedFramesTexture() const;
	enum InputNameType {
		BUTTON,
		MULTIWORD_BUTTON,
		MOTION,
		MULTIWORD_MOTION
	};
	struct InputName {
		const char* name;
		InputNameType type;
	};
	std::vector<InputName> inputNames{ 0x127 };
	std::unique_ptr<PngResource> firstFrame;
	void drawPlayerFrameTooltipInfo(const PlayerFrame& frame, int playerIndex, float wrapWidth);
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
	void frameAdvantageTextFormat(int frameAdv, char* buf, size_t bufSize);
	void frameAdvantageText(int frameAdv);
	bool showTensionData = false;
	bool showBurstGain = false;
	bool showSpeedsData = false;
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
	std::unique_ptr<PngResource> packedTexture;
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
	std::vector<PngResource> allResources;
	TexturePacker texturePacker;
	void addFrameArt(HINSTANCE hModule, FrameType frameType, WORD resourceIdColorblind, std::unique_ptr<PngResource>& resourceColorblind,
                 WORD resourceIdNonColorblind, std::unique_ptr<PngResource>& resourceNonColorblind, StringWithLength description);
	void addFrameArt(HINSTANCE hModule, FrameType frameType, WORD resourceIdBothVersions, std::unique_ptr<PngResource>& resourceBothVersions, StringWithLength description);
	void addFrameMarkerArt(HINSTANCE hModule, FrameMarkerType markerType, WORD resourceIdColorblind, std::unique_ptr<PngResource>& resourceColorblind,
                 WORD resourceIdNonColorblind, std::unique_ptr<PngResource>& resourceNonColorblind);
	void addFrameMarkerArt(HINSTANCE hModule, FrameMarkerType markerType, WORD resourceIdBothVersions, std::unique_ptr<PngResource>& resourceBothVersions);
	bool addImage(HMODULE hModule, WORD resourceId, std::unique_ptr<PngResource>& resource);
	void drawFramebars();
	bool showShaderCompilationError = true;
	const std::string* shaderCompilationError = nullptr;
	bool needShowFramebar() const;
	int two = 2;
	bool imguiActiveTemp = false;
	bool takeScreenshotTemp = false;
	bool showErrorDialog = false;
	const char* errorDialogText = nullptr;
	void* errorDialogPos = nullptr;
	bool showFramebarHelp = false;
	void framebarHelpWindow();
	bool booleanSettingPreset(std::atomic_bool& settingsRef);
	bool booleanSettingPresetWithHotkey(std::atomic_bool& settingsRef, std::vector<int>& hotkey);
	bool float4SettingPreset(float& settingsPtr);
	bool intSettingPreset(std::atomic_int& settingsPtr, int minValue);
	bool showCancels[2] { false, false };
	bool showDamageCalculation[2] { false, false };
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
	void HelpMarkerWithHotkey(const char* desc, const char* descEnd, std::vector<int>& hotkey);
	inline void HelpMarkerWithHotkey(const char* desc, std::vector<int>& hotkey) { HelpMarkerWithHotkey(desc, nullptr, hotkey); }
	template<size_t size> inline void HelpMarkerWithHotkey(const char(&desc)[size], std::vector<int>& hotkey) { HelpMarkerWithHotkey(desc, desc + size - 1, hotkey); }
	inline void HelpMarkerWithHotkey(const StringWithLength& desc, std::vector<int>& hotkey) { HelpMarkerWithHotkey(desc.txt, desc.txt + desc.length, hotkey); }
	inline void HelpMarkerWithHotkey(const std::string& desc, std::vector<int>& hotkey) { HelpMarkerWithHotkey(desc.c_str(), desc.c_str() + desc.size(), hotkey); }
	void printAllCancels(const FrameCancelInfo& cancels,
		bool enableSpecialCancel,
		bool enableJumpCancel,
		bool enableSpecials,
		bool hitOccured,
		bool airborne,
		bool insertSeparators);
};

extern UI ui;
