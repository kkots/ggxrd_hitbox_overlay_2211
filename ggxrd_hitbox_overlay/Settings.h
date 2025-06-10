#pragma once
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include "StringWithLength.h"
#include "HandleWrapper.h"

const int JOY_BTN_0 = 0x107;
const int JOY_BTN_1 = 0x108;
const int JOY_BTN_2 = 0x109;
const int JOY_BTN_3 = 0x10a;
const int JOY_BTN_4 = 0x10b;
const int JOY_BTN_5 = 0x10c;
const int JOY_BTN_6 = 0x10d;
const int JOY_BTN_7 = 0x10e;
const int JOY_BTN_8 = 0x10f;
const int JOY_BTN_9 = 0x110;
const int JOY_BTN_10 = 0x111;
const int JOY_BTN_11 = 0x112;
const int JOY_BTN_12 = 0x113;
const int JOY_BTN_13 = 0x114;
const int JOY_BTN_14 = 0x115;
const int JOY_BTN_15 = 0x116;
const int JOY_LEFT_STICK_LEFT = 0x117;  // lX, lY
const int JOY_LEFT_STICK_UP = 0x118;
const int JOY_LEFT_STICK_RIGHT = 0x119;
const int JOY_LEFT_STICK_DOWN = 0x11a;
const int JOY_DPAD_LEFT = 0x11b;
const int JOY_DPAD_UP = 0x11c;
const int JOY_DPAD_RIGHT = 0x11d;
const int JOY_DPAD_DOWN = 0x11e;
const int JOY_PS4_DUALSHOCK_RIGHT_STICK_LEFT = 0x11f;  // lZ, lRz
const int JOY_PS4_DUALSHOCK_RIGHT_STICK_UP = 0x120;
const int JOY_PS4_DUALSHOCK_RIGHT_STICK_RIGHT = 0x121;
const int JOY_PS4_DUALSHOCK_RIGHT_STICK_DOWN = 0x122;
const int JOY_XBOX_TYPE_S_RIGHT_STICK_LEFT = 0x123;  // lRx, lRy
const int JOY_XBOX_TYPE_S_RIGHT_STICK_UP = 0x124;
const int JOY_XBOX_TYPE_S_RIGHT_STICK_RIGHT = 0x125;
const int JOY_XBOX_TYPE_S_RIGHT_STICK_DOWN = 0x126;

class Settings
{
public:
	bool onDllMain();
	void onDllDetach();
	struct Key {
		const char* name = nullptr;
		const char* uiName = nullptr;
		int code = 0;
	};
	std::mutex keyCombosMutex;
	std::map<std::string, Key> keys;  // string to int code
	std::map<int, Key*> reverseKeys; // int code to string
	std::vector<int> gifModeToggle;
	std::vector<int> noGravityToggle;
	std::vector<int> freezeGameToggle;
	std::vector<int> slowmoGameToggle;
	std::vector<int> allowNextFrameKeyCombo;
	std::vector<int> disableModKeyCombo;
	std::vector<int> disableHitboxDisplayToggle;
	std::vector<int> continuousScreenshotToggle;
	std::vector<int> gifModeToggleBackgroundOnly;
	std::vector<int> togglePostEffectOnOff;
	std::vector<int> gifModeToggleCameraCenterOnly;
	std::vector<int> toggleCameraCenterOpponent;
	std::vector<int> gifModeToggleHideOpponentOnly;
	std::vector<int> toggleHidePlayer;
	std::vector<int> gifModeToggleHudOnly;
	std::vector<int> screenshotBtn;
	std::vector<int> modWindowVisibilityToggle;
	std::vector<int> framebarVisibilityToggle;
	std::vector<int> toggleDisableGrayHurtboxes;
	std::vector<int> toggleNeverIgnoreHitstop;
	std::vector<int> toggleShowInputHistory;
	std::vector<int> toggleAllowCreateParticles;
	std::vector<int> clearInputHistory;
	std:: mutex screenshotPathMutex;
	bool settingsMembersStart = false;  // make sure all settings are contained between this and settingsMembersEnd
	std::string screenshotPath;
	std::atomic_bool displayUIOnTopOfPauseMenu = true;
	std::atomic_bool dodgeObsRecording = true;
	std::atomic_bool allowContinuousScreenshotting = false;
	std::atomic_bool dontUseScreenshotTransparency = false;
	std::atomic_bool turnOffPostEffectWhenMakingBackgroundBlack = true;
	std::atomic_int slowmoTimes = 3;
	std::atomic_int framebarHeight = 19;
	std::atomic_int framebarTitleCharsMax = 12;
	std::atomic_int framebarDisplayedFramesCount = 80;
	std::atomic_int framebarStoredFramesCount = 200;
	std::atomic_int positionResetDistBetweenPlayers = 105000;
	std::atomic_int positionResetDistFromCorner = 0;
	std::atomic_int startingTensionPulse = 0;
	std::atomic_int hideWinsExceptOnWins = 0;
	const float cameraCenterOffsetX_defaultValue = 0.F;
	const float cameraCenterOffsetY_defaultValue = 106.4231F;
	const float cameraCenterOffsetY_WhenForcePitch0_defaultValue = 130.4231F;
	const float cameraCenterOffsetZ_defaultValue = 540.F;
	float cameraCenterOffsetX = cameraCenterOffsetX_defaultValue;
	float cameraCenterOffsetY = cameraCenterOffsetY_defaultValue;
	float cameraCenterOffsetY_WhenForcePitch0 = cameraCenterOffsetY_WhenForcePitch0_defaultValue;
	float cameraCenterOffsetZ = cameraCenterOffsetZ_defaultValue;
	std::atomic_bool startDisabled = false;
	std::atomic_bool drawPushboxCheckSeparately = true;
	std::atomic_bool forceZeroPitchDuringCameraCentering = true;
	std::atomic_bool modWindowVisibleOnStart = true;
	std::atomic_bool closingModWindowAlsoHidesFramebar = true;
	std::atomic_bool dontShowMoveName = true;
	std::atomic_bool neverIgnoreHitstop = false;
	std::atomic_bool ignoreHitstopForBlockingBaiken = false;
	std::atomic_bool considerRunAndWalkNonIdle = false;
	std::atomic_bool considerCrouchNonIdle = false;
	std::atomic_bool considerDummyPlaybackNonIdle = false;
	std::atomic_bool useSimplePixelBlender = false;
	std::atomic_bool usePixelShader = true;
	std::atomic_bool dontShowBoxes = false;
	std::atomic_bool neverDisplayGrayHurtboxes = false;
	std::atomic_bool showFramebar = true;
	std::atomic_bool showFramebarInTrainingMode = true;
	std::atomic_bool showFramebarInReplayMode = true;
	std::atomic_bool showFramebarInOtherModes = true;
	std::atomic_bool showStrikeInvulOnFramebar = true;
	std::atomic_bool showSuperArmorOnFramebar = true;
	std::atomic_bool showThrowInvulOnFramebar = true;
	std::atomic_bool showOTGOnFramebar = true;
	std::atomic_bool showFirstFramesOnFramebar = true;
	std::atomic_bool considerSimilarFrameTypesSameForFrameCounts = true;
	std::atomic_bool considerSimilarIdleFramesSameForFrameCounts = false;
	std::atomic_bool combineProjectileFramebarsWhenPossible = true;
	std::atomic_bool eachProjectileOnSeparateFramebar = false;
	std::atomic_bool dontClearFramebarOnStageReset = false;
	std::atomic_bool useColorblindHelp = false;
	std::atomic_bool dontTruncateFramebarTitles = false;
	std::atomic_bool useSlangNames = false;
	std::atomic_bool allFramebarTitlesDisplayToTheLeft = true;
	std::atomic_bool showPlayerInFramebarTitle = true;
	std::atomic_bool considerKnockdownWakeupAndAirtechIdle = false;
	std::atomic_bool considerIdleInvulIdle = false;
	std::atomic_bool frameAdvantage_dontUsePreBlockstunTime = true;
	std::atomic_bool skipGrabsInFramebar = true;
	std::atomic_bool showFramebarHatchedLineWhenSkippingGrab = true;
	std::atomic_bool showFramebarHatchedLineWhenSkippingHitstop = false;
	std::atomic_bool showFramebarHatchedLineWhenSkippingSuperfreeze = true;
	std::atomic_bool showP1FramedataInFramebar = true;
	std::atomic_bool showP2FramedataInFramebar = true;
	std::atomic_bool showComboProrationInRiscGauge = false;
	std::atomic_bool displayInputHistoryWhenObserving = true;
	std::atomic_bool displayInputHistoryInSomeOfflineModes = false;
	std::atomic_bool showDurationsInInputHistory = false;
	std::atomic_bool useAlternativeStaggerMashProgressDisplay = false;
	std::atomic_bool dontShowMayInteractionChecks = false;
	std::atomic_bool showMilliaBadMoonBuffHeight = false;
	std::atomic_bool showFaustOwnFlickRanges = true;
	std::atomic_bool ignoreScreenshotPathAndSaveToClipboard = false;
	std::atomic_bool showBedmanTaskCHeightBuffY = false;
	std::atomic_bool showJackoGhostPickupRange = false;
	std::atomic_bool showJackoSummonsPushboxes = false;
	std::atomic_bool showJackoAegisFieldRange = false;
	std::atomic_bool showJackoServantAttackRange = false;
	std::atomic_bool usePositionResetMod = false;
	std::atomic_bool showDebugFields = false;
	std::atomic_bool ignoreNumpadEnterKey = false;
	std::atomic_bool ignoreRegularEnterKey = false;
	std::atomic_bool comboRecipe_showDelaysBetweenCancels = true;
	std::atomic_bool comboRecipe_showIdleTimeBetweenMoves = true;
	std::atomic_bool comboRecipe_showDashes = true;
	std::atomic_bool comboRecipe_showWalks = true;
	std::atomic_bool comboRecipe_showSuperJumpInstalls = true;
	std::atomic_bool comboRecipe_transparentBackground = false;
	std::atomic_bool clearInputHistoryOnStageReset = false;
	std::atomic_bool clearInputHistoryOnStageResetInTrainingMode = false;
	std::atomic_bool hideWins = false;
	std::atomic_bool hideWinsDirectParticipantOnly = false;
	bool settingsMembersEnd = false;
	const char* getKeyRepresentation(int code);
	void readSettings(bool dontReadIfDoesntExist);
	void writeSettings();
	struct ComboInfo {
		const char* uiName = nullptr;
		StringWithLength uiNameWithLength;
		const char* uiDescription = nullptr;
		StringWithLength uiDescriptionWithLength;
	};
	void onKeyCombosUpdated();
	void getComboInfo(std::vector<int>& keyCombo, ComboInfo* info);
	const char* getOtherUIName(void* ptr);
	StringWithLength getOtherUINameWithLength(void* ptr);
	const char* getOtherUIFullName(void* ptr);
	const char* getOtherUIDescription(void* ptr);
	StringWithLength getOtherUIDescriptionWithLength(void* ptr);
	const char* getOtherINIDescription(void* ptr);
	std::string convertToUiDescription(const char* iniDescription);
	const char* getComboRepresentation(std::vector<int>& toggle);
private:
	struct KeyComboToParse {
		const char* name = nullptr;
		const char* uiName = nullptr;
		std::string uiFullName;
		std::vector<int>* keyCombo = nullptr;
		const char* defaultValue = nullptr;
		const char* iniDescription = nullptr;
		std::string uiDescription;
		bool isParsed = false;
		bool representationGenerated = false;
		std::string representation;
		void generateRepresentation();
		KeyComboToParse(const char* name, const char* uiName, std::vector<int>* keyCombo, const char* defaultValue, const char* iniDescription);
	};
	std::map<std::string, KeyComboToParse> keyCombosToParse;
	struct MyKey {
		const char* str;
	};
	static int hashString(const char* str, int startingHash = 0);
	struct MyHashFunction {
		inline std::size_t operator()(const char* key) const {
			return hashString(key);
		}
	};
	struct IniNameToUiNameMapElement {
		const char* fullName;
		const char* name;
	};
	struct MyCompareFunction {
		inline bool operator()(const char* key, const char* other) const {
			return key == other || strcmp(key, other) == 0;
		}
	};
	std::unordered_map<const char*, IniNameToUiNameMapElement, MyHashFunction, MyCompareFunction> iniNameToUiNameMap;
	void insertKeyComboToParse(const char* name, const char* uiName, std::vector<int>* keyCombo, const char* defaultValue, const char* iniDescription);
	void addKey(const char* name, const char* uiName, int code);
	static int findMinCommentPos(const char* buf);
	static std::string parseKeyName(const char* buf);
	static std::string getKeyValue(const char* buf);
	void addKeyRange(char start, char end);
	static int findChar(const char* buf, char c, int startingPos = 0);
	static int findCharRev(const char* buf, char c);
	static std::pair<int, int> trim(std::string& str); // Trims left and right in-place. Returns how many chars were cut off from left (.first) and from right (.second).
	static std::string toUppercase(const std::string& str);
	static std::vector<std::string> split(const std::string& str, char c);
	bool parseKeys(const char* keyName, const std::string& keyValue, std::vector<int>& keyCodes);
	static bool parseInteger(const char* keyName, const std::string& keyValue, std::atomic_int& integer);
	static bool parseBoolean(const char* keyName, const std::string& keyValue, std::atomic_bool& aBooleanValue);
	static const char* formatBoolean(bool value);
	static bool parseFloat(const char* keyName, const std::string& keyValue, float& floatValue);
	static float parseFloat(const char* inputString, bool* error = nullptr);
	static std::string formatFloat(float f);
	static std::string formatInteger(int f);
	static std::wstring getCurrentDirectory();
	void registerListenerForChanges();
	std::wstring settingsPath;
	bool firstSettingsParse = true;
	HANDLE directoryChangeHandle = NULL;
	bool lastCallFailedToGetTime = false;
	HandleWrapper changesListener = NULL;
	bool changesListenerStarted = false;
	HandleWrapper changesListenerWakeEvent = NULL;
	HandleWrapper changeListenerExitedEvent = NULL;
	enum ChangesListenerWakeType {
		WAKE_TYPE_EXIT,
		WAKE_TYPE_WRITING_FILE
	} changesListenerWakeType;
	static DWORD WINAPI changesListenerLoop(LPVOID lpThreadParameter);
	void writeSettingsMain();
	static bool isWhitespace(const char* str);
	static int compareKeyCombos(const std::vector<int>& left, const std::vector<int>& right);
	const char* getKeyTxtName(int code);
	void trashComboRepresentation(std::vector<int>& toggle);
	struct OtherDescription {
		void* ptr = nullptr;
		const char* iniName = nullptr;
		std::string iniNameAllCaps;
		const char* uiName = nullptr;
		StringWithLength uiNameWithLength;
		std::string uiFullPath;
		const char* iniDescription = nullptr;
		std::string uiDescription;
	};
	std::vector<OtherDescription> otherDescriptions;
	void registerOtherDescription(void* ptr, const char* iniName, const char* uiName, const char* uiPath, const char* iniDescription);
	std::vector<OtherDescription*> pointerIntoSettingsIntoDescription;
	std::unordered_map<std::string, DWORD> settingNameToOffset;  // case-insensitive
};

extern Settings settings;
