#pragma once
#include "pch.h"
#include <d3d9.h>
#include <vector>
#include <mutex>
#include "PngResource.h"
#include "TexturePacker.h"
#include "PlayerInfo.h"

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
	MARKER_TYPE_THROW_INVUL
};
const FrameMarkerType MARKER_TYPE_LAST = MARKER_TYPE_THROW_INVUL;

class UI
{
public:
	bool onDllMain(HMODULE hModule);
	void onDllDetachStage1();
	void onDllDetachGraphics();
	void onDllDetachNonGraphics();
	void prepareDrawData();
	void onEndScene(IDirect3DDevice9* device, void* drawData, IDirect3DTexture9* iconTexture);
	LRESULT WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void handleResetBefore();
	void handleResetAfter();
	
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
	char* printDecimal(int num, int numAfterPoint, int padding, bool percentage = false);
	bool showTensionData = false;
	bool showSpeedsData = false;
	bool showFrameAdvTooltip = false;
	void* hook_GetKeyStatePtr = nullptr;
	IDirect3DTexture9* imguiFont = nullptr;
	void onImGuiMessWithFontTexID();
	bool showCharSpecific[2] = { false, false };
	PngResource packedTexture;
	PngResource activeFrame;
	PngResource activeFrameNonColorblind;
	PngResource startupFrame;
	PngResource startupFrameNonColorblind;
	PngResource startupFrameCanBlock;
	PngResource startupFrameCanBlockNonColorblind;
	PngResource recoveryFrame;
	PngResource recoveryFrameNonColorblind;
	PngResource recoveryFrameHasGatlings;
	PngResource recoveryFrameHasGatlingsNonColorblind;
	PngResource recoveryFrameCanAct;
	PngResource recoveryFrameCanActNonColorblind;
	PngResource nonActiveFrame;
	PngResource nonActiveFrameNonColorblind;
	PngResource projectileFrame;
	PngResource projectileFrameNonColorblind;
	PngResource landingRecoveryFrame;
	PngResource landingRecoveryFrameNonColorblind;
	PngResource firstFrame;
	PngResource idleFrame;
	PngResource idleFrameCantBlockNonColorblind;
	PngResource idleFrameCantBlock;
	PngResource idleFrameCantFDNonColorblind;
	PngResource idleFrameCantFD;
	PngResource idleFrameElpheltRifleNonColorblind;
	PngResource idleFrameElpheltRifle;
	PngResource strikeInvulFrame;
	PngResource throwInvulFrame;
	PngResource superArmorFrame;
	PngResource superArmorFrameNonColorblind;
	PngResource superArmorFrameFull;
	PngResource superArmorFrameFullNonColorblind;
	PngResource digitFrame[10];
	PngResource xstunFrame;
	PngResource xstunFrameNonColorblind;
	TexturePacker texturePacker;
	void addFrameArt(HINSTANCE hModule, FrameType frameType, WORD resourceIdColorblind, PngResource& resourceColorblind,
                 WORD resourceIdNonColorblind, PngResource& resourceNonColorblind, const char* description);
	void addFrameArt(HINSTANCE hModule, FrameType frameType, WORD resourceIdBothVersions, PngResource& resourceBothVersions, const char* description);
	void addFrameMarkerArt(HINSTANCE hModule, FrameMarkerType markerType, WORD resourceIdColorblind, PngResource& resourceColorblind,
                 WORD resourceIdNonColorblind, PngResource& resourceNonColorblind);
	void addFrameMarkerArt(HINSTANCE hModule, FrameMarkerType markerType, WORD resourceIdBothVersions, PngResource& resourceBothVersions);
	bool addImage(HMODULE hModule, WORD resourceId, PngResource& resource);
	void drawFramebars();
	bool showShaderCompilationError = true;
	const std::string* shaderCompilationError = nullptr;
	bool needShowFramebar() const;
};

extern UI ui;
