#include "pch.h"
#include "Settings.h"
#include "logging.h"
#include "Keyboard.h"
#include "GifMode.h"
#include "Detouring.h"
#include "WinError.h"
#include <list>
#include "UI.h"
#include "CustomWindowMessages.h"
#include <unordered_map>
#include <functional>

Settings settings;

bool Settings::onDllMain() {
	addKey("Backspace", "Backspace", VK_BACK);
	addKey("Tab", "Tab", VK_TAB);
	addKey("Enter", "Enter", VK_RETURN);
	addKey("PauseBreak", "PauseBreak", VK_PAUSE);
	addKey("CapsLock", "CapsLock", VK_CAPITAL);
	addKey("Escape", "Escape", VK_ESCAPE);
	addKey("Space", "Space", VK_SPACE);
	addKey("PageUp", "PageUp", VK_PRIOR);
	addKey("PageDown", "PadeDown", VK_NEXT);
	addKey("End", "End", VK_END);
	addKey("Home", "Home", VK_HOME);
	addKey("Left", "Arrow Left", VK_LEFT);
	addKey("Up", "Arrow Up", VK_UP);
	addKey("Right", "Arrow Right", VK_RIGHT);
	addKey("Down", "Arrow Down", VK_DOWN);
	addKey("PrintScreen", "PrintScreen", VK_SNAPSHOT);
	addKey("Insert", "Insert", VK_INSERT);
	addKey("Delete", "Delete", VK_DELETE);
	addKey("Num0", "Num0", VK_NUMPAD0);
	addKey("Num1", "Num1", VK_NUMPAD1);
	addKey("Num2", "Num2", VK_NUMPAD2);
	addKey("Num3", "Num3", VK_NUMPAD3);
	addKey("Num4", "Num4", VK_NUMPAD4);
	addKey("Num5", "Num5", VK_NUMPAD5);
	addKey("Num6", "Num6", VK_NUMPAD6);
	addKey("Num7", "Num7", VK_NUMPAD7);
	addKey("Num8", "Num8", VK_NUMPAD8);
	addKey("Num9", "Num9", VK_NUMPAD9);
	addKey("NumMultiply", "Num*", VK_MULTIPLY);
	addKey("NumAdd", "Num+", VK_ADD);
	addKey("NumSubtract", "Num-", VK_SUBTRACT);
	addKey("NumDecimal", "Num.", VK_DECIMAL);
	addKey("NumDivide", "Num/", VK_DIVIDE);
	addKey("F1", "F1", VK_F1);
	addKey("F2", "F2", VK_F2);
	addKey("F3", "F3", VK_F3);
	addKey("F4", "F4", VK_F4);
	addKey("F5", "F5", VK_F5);
	addKey("F6", "F6", VK_F6);
	addKey("F7", "F7", VK_F7);
	addKey("F8", "F8", VK_F8);
	addKey("F9", "F9", VK_F9);
	addKey("F10", "F10", VK_F10);
	addKey("F11", "F11", VK_F11);
	addKey("F12", "F12", VK_F12);
	addKey("NumLock", "NumLock", VK_NUMLOCK);
	addKey("ScrollLock", "ScrollLock", VK_SCROLL);
	addKey("Colon", ":", VK_OEM_1);
	addKey("Plus", "+", VK_OEM_PLUS);
	addKey("Minus", "-", VK_OEM_MINUS);
	addKey("Comma", ",", VK_OEM_COMMA);
	addKey("Period", ".", VK_OEM_PERIOD);
	addKey("Slash", "/", VK_OEM_2);
	addKey("Tilde", "~", VK_OEM_3);
	addKey("OpenSquareBracket", "[", VK_OEM_4);
	addKey("Backslash", "\\", VK_OEM_5);
	addKey("CloseSquareBracket", "]", VK_OEM_6);
	addKey("Quote", "\"", VK_OEM_7);
	addKey("Backslash2", "\\ (2)", VK_OEM_102);

	addKeyRange('0', '9');
	addKeyRange('A', 'Z');

	addKey("Shift", "Shift", VK_SHIFT);
	addKey("Ctrl", "Ctrl", VK_CONTROL);
	addKey("Alt", "Alt", VK_MENU);
	
	for (auto it = keys.begin(); it != keys.end(); ++it) {
		reverseKeys.insert({it->second.code, &it->second});
	}
	
	insertKeyComboToParse("gifModeToggle", "GIF Mode Toggle", &gifModeToggle, "F1",
		"; A keyboard shortcut to toggle GIF mode.\n"
		"; GIF mode is:\n"
		"; 1) Background becomes black\n"
		"; 2) Camera is centered on you\n"
		"; 3) Opponent is invisible and invulnerable\n"
		"; 4) Hide HUD");
	insertKeyComboToParse("noGravityToggle", "No Gravity Toggle", &noGravityToggle, "F2",
		"; A keyboard shortcut to toggle No gravity mode\n"
		"; No gravity mode is you can't fall basically");
	insertKeyComboToParse("freezeGameToggle", "Freeze Game Toggle", &freezeGameToggle, "F3",
		"; A keyboard shortcut to freeze the game");
	insertKeyComboToParse("slowmoGameToggle", "Slow-mo Game Toggle", &slowmoGameToggle, "F4",
		"; A keyboard shortcut to play the game in slow motion.\n"
		"; Please specify by how many times to slow the game down in \"slowmoTimes\"");
	insertKeyComboToParse("allowNextFrameKeyCombo", "Allow Next Frame", &allowNextFrameKeyCombo, "F5",
		"; A keyboard shortcut. Only works while the game is frozen using \"freezeGameToggle\".\n"
		"; Advances the game forward one frame");
	insertKeyComboToParse("disableModToggle", "Disable Mod Toggle", &disableModKeyCombo, "F6",
		"; A keyboard shortcut to enable/disable the mod without having to load/unload it");
	insertKeyComboToParse("disableHitboxDisplayToggle", "Disable Hitbox Display Toggle", &disableHitboxDisplayToggle, "F7",
		"; A keyboard shortcut to enable/disable only the mod hitbox drawing feature:\n"
		"; the GIF mode and no gravity, etc will keep working");
	insertKeyComboToParse("screenshotBtn", "Take Screenshot", &screenshotBtn, "F8", "; A keyboard shortcut.\n"
		"; Takes a screenshot and saves it at \"screenshotPath\" path\n"
		"; To take screenshots over a transparent background you need to go to the game's\n"
		"; Display Settings and turn off Post-Effect (or use \"togglePostEffectOnOff\" and\n"
		"; \"turnOffPostEffectWhenMakingBackgroundBlack\" settings for this), then use GIF mode (make background dark).\n"
		"; Then screenshots will film character over transparent background.\n"
		"; If the \"dontUseScreenshotTransparency\" setting is true, screenshot will be without\n"
		"; transparency anyway");
	insertKeyComboToParse("continuousScreenshotToggle", "Continuous Screenshot Toggle", &continuousScreenshotToggle, "",
		"; A keyboard shortcut.\n"
		"; This toggle can be used same way as \"screenshotBtn\" (when it's combined with\n"
		"; \"allowContinuousScreenshotting\" = true), except it's a separate key combination and when you\n"
		"; press it, it toggles the continuous screenshot taking every game logical frame. This\n"
		"; toggle does not require \"allowContinuousScreenshotting\" to be set to true,\n"
		"; or screenshotBtn to be set to anything.");
	insertKeyComboToParse("gifModeToggleBackgroundOnly", "GIF Mode Toggle (Background Only)", &gifModeToggleBackgroundOnly, "",
		"; A keyboard shortcut to only toggle the \"background becomes black\" part of the \"gifModeToggle\".\n"
		"; Empty by default, which means no hotkey is assigned. Assign your desired hotkey manually here.\n"
		"; This option can be combined with the other \"only\" options, by sharing the same key binding for example");
	insertKeyComboToParse("togglePostEffectOnOff", "Toggle Post-Effect On/Off", &togglePostEffectOnOff, "",
		"; A keyboard shortcut to toggle the game's Settings - Display Settings - Post-Effect. Changing it this way does not\n"
		"; require the current match to be restarted.\n"
		"; Alternatively, you could set the \"turnOffPostEffectWhenMakingBackgroundBlack\" setting in this INI file to true\n"
		"; so that whenever you enter either the GIF mode or the GIF mode (black background only), the Post-Effect is\n"
		"; turned off automatically, and when you leave those modes, it gets turned back on.\n"
		"; This hotkey is empty by default, which means no hotkey is assigned. Assign your desired hotkey manually here.\n"
		"; This option can be combined with the other hotkey options, by sharing the same key binding for example.");
	insertKeyComboToParse("gifModeToggleCameraCenterOnly", "GIF Mode Toggle (Camera Only)", &gifModeToggleCameraCenterOnly, "",
		"; A keyboard shortcut to only toggle the \"Camera is centered on you\" part of the \"gifModeToggle\".\n"
		"; Empty by default, which means no hotkey is assigned. Assign your desired hotkey manually here.\n"
		"; This option can be combined with the other \"only\" options, by sharing the same key binding for example");
	insertKeyComboToParse("toggleCameraCenterOpponent", "Center camera on the opponent", &toggleCameraCenterOpponent, "",
		"; A keyboard shortcut to toggle the camera to be centered on the opponent.\n"
		"; Empty by default, which means no hotkey is assigned. Assign your desired hotkey manually here.\n"
		"; This option can be combined with GIF Mode options, by sharing the same key binding for example");
	insertKeyComboToParse("gifModeToggleHideOpponentOnly", "GIF Mode Toggle (Hide Opponent Only)", &gifModeToggleHideOpponentOnly, "",
		"; A keyboard shortcut to only toggle the \"Opponent is invisible and invulnerable\" part of the \"gifModeToggle\".\n"
		"; Empty by default, which means no hotkey is assigned. Assign your desired hotkey manually here.\n"
		"; This option can be combined with the other \"only\" options, by sharing the same key binding for example");
	insertKeyComboToParse("toggleHidePlayer", "Hide Player", &toggleHidePlayer, "",
		"; A keyboard shortcut to toggle hiding the player.\n"
		"; Empty by default, which means no hotkey is assigned. Assign your desired hotkey manually here.\n"
		"; This option can be combined with GIF Mode options, by sharing the same key binding for example");
	insertKeyComboToParse("gifModeToggleHudOnly", "GIF Mode Toggle (HUD Only)", &gifModeToggleHudOnly, "",
		"; A keyboard shortcut to only toggle the \"hide hud\" part of the \"gifModeToggle\".\n"
		"; Empty by default, which means no hotkey is assigned. Assign your desired hotkey manually here.\n"
		"; This option can be combined with the other \"only\" options, by sharing the same key binding for example");
	insertKeyComboToParse("modWindowVisibilityToggle", "Hide UI Toggle", &modWindowVisibilityToggle, "Escape",
		"; A keyboard shortcut.\n"
		"; Pressing this shortcut will show/hide the mod's UI window.");
	insertKeyComboToParse("framebarVisibilityToggle", "Hide Framebar Toggle", &framebarVisibilityToggle, "",
		"; A keyboard shortcut.\n"
		"; Pressing this shortcut will show/hide the framebar window by changing the \"showFramebar\" setting.\n"
		"; If \"closingModWindowAlsoHidesFramebar\" is true, then setting \"showFramebar\" to true is not enough, as the main\n"
		"; mod's UI window must also be open in order to show the framebar. If the window is not open, and \"showFramebar\" is true,\n"
		"; then the only way to open it and to show the framebar is with the \"modWindowVisibilityToggle\" hotkey.");
	insertKeyComboToParse("toggleDisableGrayHurtboxes", "Disable Gray Hurtboxes", &toggleDisableGrayHurtboxes, "",
		"; A keyboard shortcut.\n"
		"; Pressing this shortcut will disable/enable the display of residual hurtboxes that appear on hit/block and show\n"
		"; the defender's hurtbox at the moment of impact. These hurtboxes display for only a brief time on impacts but\n"
		"; they can get in the way when trying to do certain stuff such as take screenshots of hurtboxes.");
	
	static const char* hitboxesStr = "Hitboxes";
	static const char* settingsHitboxSettingsStr = "Settings - Hitbox Settings";
	static const char* settingsGeneralSettingsStr = "Settings - General Settings";
	static const char* settingsFramebarSettingsStr = "Settings - Framebar Settings";
	#define settingAndItsName(name) &name, #name
	registerOtherDescription(settingAndItsName(slowmoTimes), "Slow-Mo Factor", hitboxesStr, "; A number.\n"
			"; This works in conjunction with \"slowmoGameToggle\". Only round numbers greater than 1 allowed.\n"
			"; Specifies by how many times to slow the game down");
	registerOtherDescription(settingAndItsName(framebarHeight), "Framebar Height", settingsFramebarSettingsStr, "; A number.\n"
			"; Specifies the height of a single framebar of one player, in pixels, including the black outlines on the outside.\n"
			"; The standard height is 19.");
	registerOtherDescription(settingAndItsName(allowContinuousScreenshotting), "Allow Continuous Screenshotting When Button Is Held", settingsHitboxSettingsStr,
			"; Specify true or false.\n"
			"; When this is true that means screenshots are being taken every game loop logical frame as\n"
			"; long as the \"screenshotBtn\" is being held. Game loop logical frame means that if the game is\n"
			"; paused or the actual animations are not playing for whatever reason, screenshot won't be taken.\n"
			"; A new screenshot is only taken when animation frames change on the player characters.\n"
			"; Be cautions not to run out of disk space if you're low. This option doesn't\n"
			"; work if \"screenshotPath\" is empty, it's not allowed to work outside of training mode or when\n"
			"; a match (training session) isn't currently running (for example on character selection screen).");
	registerOtherDescription(settingAndItsName(displayUIOnTopOfPauseMenu), "Display Mod's UI on top of Pause Menu", settingsGeneralSettingsStr,
			"; Specify true or false.\n"
			"; Display mod's UI on top of the game's Pause Menu.");
	registerOtherDescription(settingAndItsName(lowProfileCutoffPoint), "Low Profile Cut-Off Height", settingsGeneralSettingsStr,
			"; Specify a whole number.\n"
			"; In the UI's 'Invul' field and in the framebar on-mouse-hover info,"
			"; hurtboxes that are below this Y will trigger the display of 'low profile' invulnerability type.\n"
			"; After changing this value, redo the move to see the updated value.");
	registerOtherDescription(settingAndItsName(neverIgnoreHitstop), "Never Ignore Hitstop", settingsFramebarSettingsStr,
			"; Specify true or false.\n"
			"; Normally we don't display hitstop in the framebar if both players are in hitstop on that frame,\n"
			"; unless a projectile or a blocking Baiken is present.\n"
			"; If this is set to true, then we always show hitstop in the framebar.");
	registerOtherDescription(settingAndItsName(considerRunAndWalkNonIdle), "Consider Running And Walking Non-Idle", settingsFramebarSettingsStr,
			"; Specify true or false.\n"
			"; Normally we consider running and walking as being idle, which does not advance the framebar forward.\n"
			"; The framebar only advances when one of the players is \"busy\".\n"
			"; If this is set to true, then one player running or walking will be treated same way as \"busy\" and will advance the framebar.");
	registerOtherDescription(settingAndItsName(considerCrouchNonIdle), "Consider Crouching Non-Idle", settingsFramebarSettingsStr,
			"; Specify true or false.\n"
			"; Normally we consider crouching as being idle, which does not advance the framebar forward.\n"
			"; The framebar only advances when one of the players is \"busy\".\n"
			"; If this is set to true, then one player crouching or walking will be treated same way as \"busy\" and will advance the framebar.\n"
			"; A dummy who is crouching automatically due to training settings will still not be considered \"busy\" no matter what.");
	registerOtherDescription(settingAndItsName(useSimplePixelBlender), "Use Simple CPU Pixel Blender", settingsHitboxSettingsStr,
			"; Specify true or false.\n"
			"; Setting this to true may increase the performance of transparent screenshotting which may be useful if you're screenshotting every frame.\n"
			"; The produced screenshots won't have such improvements as improving visibility of semi-transparent effects or changing hitbox outlines to\n"
			"; black when drawn over the same color.");
	registerOtherDescription(settingAndItsName(dontShowBoxes), "Don't Show Boxes", settingsHitboxSettingsStr,
			"; Specify true or false.\n"
			"; Setting this to true will hide all hurtboxes, hitboxes, pushboxes and other boxes and points.");
	registerOtherDescription(settingAndItsName(neverDisplayGrayHurtboxes), "Disable Gray Hurtboxes", hitboxesStr,
			"; Specify true or false.\n"
			"; This disables the display of gray hurtboxes (for a toggle see \"toggleDisableGrayHurtboxes\").\n"
			"; Gray hurtboxes are residual hurtboxes that appear on hit/block and show the defender's hurtbox at the moment of impact.\n"
			"; These hurtboxes display for only a brief time on impacts but they can get in the way when trying to do certain stuff such\n"
			"; as take screenshots of hurtboxes on hit/block.");
	registerOtherDescription(settingAndItsName(showFramebar), "Show Framebar", settingsFramebarSettingsStr,
			"; Specify true or false.\n"
			"; This setting can be changed using the \"framebarVisibilityToggle\" hotkey.\n"
			"; If \"closingModWindowAlsoHidesFramebar\" is true, then setting \"showFramebar\" to true is not enough, as the main\n"
			"; mod's UI window must also be open in order to show the framebar. If the window is not open, and \"showFramebar\" is true,\n"
			"; then the only way to open it and to show the framebar is with the \"modWindowVisibilityToggle\" hotkey.");
	registerOtherDescription(settingAndItsName(showFramebarInTrainingMode), "Show Framebar In Training Mode", settingsFramebarSettingsStr,
			"; Specify true or false.\n"
			"; If false, the framebar will not be shown when in training mode even when \"showFramebar\" is true and the UI is open.");
	registerOtherDescription(settingAndItsName(showFramebarInReplayMode), "Show Framebar In Replay Mode", settingsFramebarSettingsStr,
			"; Specify true or false.\n"
			"; If false, the framebar will not be shown when in replay mode even when \"showFramebar\" is true and the UI is open.");
	registerOtherDescription(settingAndItsName(showFramebarInOtherModes), "Show Framebar In Other Modes", settingsFramebarSettingsStr,
			"; Specify true or false.\n"
			"; If false, the framebar will not be shown when in other modes even when \"showFramebar\" is true and the UI is open.");
	registerOtherDescription(settingAndItsName(showStrikeInvulOnFramebar), "Show Strike Invul", settingsFramebarSettingsStr,
			"; Specify true or false.\n"
			"; Strike invul will be displayed using a green ^ on top of a frame.");
	registerOtherDescription(settingAndItsName(showSuperArmorOnFramebar), "Show Super Armor", settingsFramebarSettingsStr,
			"; Specify true or false.\n"
			"; Super armor will be displayed using a purple ^ on top of a frame. It includes reflect, parry and projectile-only invulnerability\n"
			"; (excluding Aegis Field, that isn't displayed on the framebar at all). If both strike invul and super armor are present, super armor\n"
			"; will be below the strike invul.");
	registerOtherDescription(settingAndItsName(showThrowInvulOnFramebar), "Show Throw Invul", settingsFramebarSettingsStr,
			"; Specify true or false.\n"
			"; Throw invul will be displayed using a yellow v underneath a frame.");
	registerOtherDescription(settingAndItsName(showFirstFramesOnFramebar), "Show First Frames", settingsFramebarSettingsStr,
			"; Specify true or false.\n"
			"; When a player's animation changes from one to another, except in certain cases, the first frame of the new animation is denoted with\n"
			"; a ' mark before that frame. For some animations a first frame is denoted even when\n"
			"; the animation didn't change, but instead reached some important point. This includes entering and leaving hitstop.");
	registerOtherDescription(settingAndItsName(useColorblindHelp), "Use Colorblindness Help", settingsFramebarSettingsStr,
			"; Specify true or false.\n"
			"; If true, certain types of frames in the framebar will be displayed with distinct hatches on them.\n"
			"; Make sure the framebar is scaled wide enough and the screen resolution is large enough that you can see the hatches properly.\n"
			"; To scale the framebar you can drag its right edge.");
	registerOtherDescription(settingAndItsName(considerKnockdownWakeupAndAirtechIdle), "Consider Knockdown, Wakeup and Airtech Idle", settingsFramebarSettingsStr,
			"; Specify true or false\n"
			"; This controls whether a character being knocked down, waking up or air recovering causes the framebar to advance forward (if you're also idle).\n"
			"; Framebar only advances forward when one or both players are not idle.\n"
			"; Framebar advancing forward means it continuously overwrites its oldest contents with new data to display.\n"
			"; This could be bad if you wanted to study why a combo dropped, as some knockdowns can be very long and erase all the info you wanted to see.\n"
			"; Setting this to true may prevent that.\n"
			"; The first frame when the opponent is in OTG state and onwards - those frames do not get included in the framebar.\n"
			"; If you recover from your move later than the opponent enters OTG state, the frames are included for your whole recovery for both you and the opponent,\n"
			"; which means OTG state may partially or fully get included into the framebar.\n"
			"; In such cases, look for an animation start delimiter on the opponent's framebar, shown as a white ' between frames.");
	registerOtherDescription(settingAndItsName(startDisabled), "startDisabled", "INI file",
			"; Specify true or false.\n"
			"; When true, starts the mod in a disabled state: it doesn't draw boxes or affect anything");
	registerOtherDescription(settingAndItsName(screenshotPath), "Screenshots Path", settingsHitboxSettingsStr,
			"; A path to a file or a directory.\n"
			"; It specifies where screenshots will be saved.\n"
			"; If you provided a file path, it must be with .png extension, and if such name already exists, a\n"
			"; number will be appended to it, increasing from 1 to infinity consecutively so that it's unique,\n"
			"; so that new screenshots will never overwrite old ones.\n"
			"; If you provided a directory path, it must already exist, and \"screen.png\" will be appended to\n"
			"; it with an increasing number at the end in case the filename is not unique.\n"
			"; The provided path must be without quotes.\n"
			"; If you want the path to be multilingual you need to save this file in UTF-8.\n"
			"; On Ubuntu/Linux running Guilty Gear Xrd under Steam Proton you need to specify paths with\n"
			"; the Z:\\ drive, path separator is backslash (\\), not forward slash (/). Example: Z:\\home\\yourUserName\\ggscreen.png\n"
			"; If the path is not specified or is empty, the screenshot will be saved into your clipboard so\n"
			"; it can be pasted into any image editing program. For example, GIMP will recognize the PNG\n"
			"; format and paste that, with transparency. This would work even on Ubuntu/Linux.\n"
			"; Only PNG format is supported.");
	registerOtherDescription(settingAndItsName(dontUseScreenshotTransparency), "Take Screenshots Without Transparency", settingsHitboxSettingsStr,
			"; Specify true or false.\n"
			"; Setting this to true will produce screenshots without transparency");
	registerOtherDescription(settingAndItsName(turnOffPostEffectWhenMakingBackgroundBlack), "Turn Off Post-Effect When Making Background Black", settingsHitboxSettingsStr,
			"; Specify true or false.\n"
			"; When true, whenever you enter either the GIF mode or the GIF mode (black background only),\n"
			"; the Post-Effect is turned off automatically, and when you leave those modes, it gets turned back on.");
	registerOtherDescription(settingAndItsName(drawPushboxCheckSeparately), "Draw Pushbox Check Separately", settingsHitboxSettingsStr,
			"; Specify true or false.\n"
			"; Setting this to true will make throw boxes show in an opponent-character-independent way:\n"
			"; The part of the throw box that checks for pushboxes proximity will be shown in blue,\n"
			"; while the part of the throw box that checks x or y of the origin point will be shown in purple\n"
			"; Setting this to false will combine both the checks of the throw so that you only see the final box\n"
			"; in blue which only checks the opponent's origin point. Be warned, such a throw box\n"
			"; is affected by the width of the opponent's pushbox. Say, on Potemkin, for example,\n"
			"; all ground throw ranges should be higher.");
	registerOtherDescription(settingAndItsName(modWindowVisibleOnStart), "Mod Window Visible On Start", settingsGeneralSettingsStr,
			"; Specify true or false.\n"
			"; If this is false, when this mod starts, the mod's UI window will be invisible.");
	registerOtherDescription(settingAndItsName(closingModWindowAlsoHidesFramebar), "Closing Mod's Window Also Hides Framebar", settingsFramebarSettingsStr,
			"; Specify true or false.\n"
			"; If this is true, then closing the main mod's window, either using a hotkey or the cross mark,\n"
			"; will also hide the framebar, while opening the main mod's window will show the framebar.");
	#undef settingAndItsName
	
	pointerIntoSettingsIntoDescription.resize(offsetof(Settings, settingsMembersEnd) - offsetof(Settings, settingsMembersStart));
	for (OtherDescription& desc : otherDescriptions) {
		pointerIntoSettingsIntoDescription[(BYTE*)desc.ptr - (BYTE*)this - offsetof(Settings, settingsMembersStart)] = &desc;
		iniNameToUiNameMap.insert({ desc.iniNameAllCaps.c_str(), desc.uiFullPath.c_str() });
	}
	for (auto it = keyCombosToParse.begin(); it != keyCombosToParse.end(); ++it) {
		std::string& strRef = it->second.uiFullName;
		strRef.reserve(33 + strlen(it->second.uiName) + 1);
		strRef = "'Settings - Keyboard Shortcuts - ";
		strRef += it->second.uiName;
		strRef += '\'';
		iniNameToUiNameMap.insert({ it->first.c_str(), strRef.c_str() });
	}
	for (auto it = keyCombosToParse.begin(); it != keyCombosToParse.end(); ++it) {
		it->second.uiDescription = convertToUiDescription(it->second.iniDescription);
	}
	for (OtherDescription& desc : otherDescriptions) {
		desc.uiDescription = convertToUiDescription(desc.iniDescription);
	}
	
	std::wstring currentDir = getCurrentDirectory();
	settingsPath = currentDir + L"\\ggxrd_hitbox_overlay.ini";
	logwrap(fprintf(logfile, "INI file path: %ls\n", settingsPath.c_str()));
	
	registerListenerForChanges();

	readSettings(false);

	return true;
}

void Settings::onDllDetach() {
	if (directoryChangeHandle) {
		HANDLE temp = directoryChangeHandle;
		directoryChangeHandle = NULL;
		FindCloseChangeNotification(temp);
	}
	if (changesListener) {
		if (!changesListenerStarted) {
			// When the DLL returns FALSE in DllMain upon DLL_PROCESS_ATTACH, and
			// the changesListener thread has already been created,
			// it will not have started yet by the time this code runs.
			// We need to terminate it because there's absolutely no way to interact with it,
			// since its waiting on a lock that DllMain holds.
			TerminateThread(changesListener, 0);
		} else {
			changesListenerWakeType = WAKE_TYPE_EXIT;
			// We're calling WaitForSingleObject on the changeListenerExitedEvent, instead of on the
			// changesListener, because this call happens in DllMain, which happens under an internal
			// system lock, called the "loader" lock.
			// "The loader lock is taken by any function that needs to access the list of DLLs loaded into the process." - Raymond Chen.
			// When a thread exits, even if you used DisableThreadLibraryCalls(hModule), it calls DllMain with the loader lock, and this
			// happens before its object is signaled.
			// As such, you cannot wait on a thread inside DllMain.
			
			bool eventIsSet = false;
			while (true) {
				DWORD exitCode;
				if (GetExitCodeThread(changesListener, &exitCode)) {
					// On normal exit the thread is already killed by something
					if (exitCode != STILL_ACTIVE) {
						break;
					}
				}
				if (!eventIsSet) {
					eventIsSet = true;
					SetEvent(changesListenerWakeEvent);
				}
				DWORD waitResult = WaitForSingleObject(changeListenerExitedEvent, 100);
				if (waitResult == WAIT_OBJECT_0) break;
			}
		}
		CloseHandle(changeListenerExitedEvent);
		CloseHandle(changesListener);
		CloseHandle(changesListenerWakeEvent);
		changesListener = NULL;
	}
}

void Settings::addKey(const char* name, const char* uiName, int code) {
	keys.insert({toUppercase(name), {name, uiName, code}});
}

void Settings::addKeyRange(char start, char end) {
	static std::list<std::string> mem;
	for (char c = start; c <= end; ++c) {
		mem.emplace_back(2, '\0');
		std::string& newMem = mem.back();
		newMem[0] = c;
		addKey(newMem.c_str(), newMem.c_str(), c);
	}
}

void Settings::insertKeyComboToParse(const char* name, const char* uiName, std::vector<int>* keyCombo, const char* defaultValue, const char* iniDescription) {
	keyCombosToParse.insert({ toUppercase(name), KeyComboToParse(name, uiName, keyCombo, defaultValue, iniDescription) });
}

// INI file must be placed next the the game's executable at SteamLibrary\steamapps\common\GUILTY GEAR Xrd -REVELATOR-\Binaries\Win32\ggxrd_hitbox_overlay.ini
// Example INI file content:
// gifModeToggle = Ctrl+F3
void Settings::readSettings(bool dontReadIfDoesntExist) {
	logwrap(fputs("Reading settings\n", logfile));
	
	char errorString[500];
	FILE* file = NULL;
	if (_wfopen_s(&file, settingsPath.c_str(), L"rt") || !file) {
		strerror_s(errorString, errno);
		logwrap(fprintf(logfile, "Could not open INI file: %s\n", errorString));
		file = NULL;
	}
	if (dontReadIfDoesntExist && !file) {
		return;
	}
	std::unique_lock<std::mutex> keyboardGuard(keyboard.mutex);
	Keyboard::MutexLockedFromOutsideGuard keyboardOutsideGuard;
	std::unique_lock<std::mutex> guard(keyCombosMutex);
	std::unique_lock<std::mutex> screenshotGuard(screenshotPathMutex);
	
	for (auto it = keyCombosToParse.begin(); it != keyCombosToParse.end(); ++it) {
		it->second.keyCombo->clear();
	}
	keyboard.removeAllKeyCodes();


	slowmoTimes = 3;
	bool slowmoTimesParsed = false;
	
	framebarHeight = 19;
	bool framebarHeightParsed = false;

	lowProfileCutoffPoint = 175000;
	bool lowProfileCutoffPointParsed = false;

	startDisabled = false;
	bool startDisabledParsed = false;

	screenshotPath.clear();
	bool screenshotPathParsed = false;

	allowContinuousScreenshotting = false;
	bool allowContinuousScreenshottingParsed = false;
	
	turnOffPostEffectWhenMakingBackgroundBlack = true;
	bool turnOffPostEffectWhenMakingBackgroundBlackParsed = false;
	
	displayUIOnTopOfPauseMenu = false;
	bool displayUIOnTopOfPauseMenuParsed = false;
	
	neverIgnoreHitstop = false;
	bool neverIgnoreHitstopParsed = false;
	
	considerRunAndWalkNonIdle = true;
	bool considerRunAndWalkNonIdleParsed = false;
	
	considerCrouchNonIdle = true;
	bool considerCrouchNonIdleParsed = false;
	
	useSimplePixelBlender = false;
	bool useSimplePixelBlenderParsed = false;
	
	dontShowBoxes = false;
	bool dontShowBoxesParsed = false;
	
	neverDisplayGrayHurtboxes = false;
	bool neverDisplayGrayHurtboxesParsed = false;
	
	showFramebar = true;
	bool showFramebarParsed = false;
	
	showFramebarInTrainingMode = true;
	bool showFramebarInTrainingModeParsed = false;
	
	showFramebarInReplayMode = true;
	bool showFramebarInReplayModeParsed = false;
	
	showFramebarInOtherModes = false;
	bool showFramebarInOtherModesParsed = false;
	
	showStrikeInvulOnFramebar = true;
	bool showStrikeInvulOnFramebarParsed = false;
	
	showSuperArmorOnFramebar = true;
	bool showSuperArmorOnFramebarParsed = false;
	
	showThrowInvulOnFramebar = true;
	bool showThrowInvulOnFramebarParsed = false;
	
	showFirstFramesOnFramebar = true;
	bool showFirstFramesOnFramebarParsed = false;
	
	useColorblindHelp = false;
	bool useColorblindHelpParsed = false;
	
	considerKnockdownWakeupAndAirtechIdle = false;
	bool considerKnockdownWakeupAndAirtechIdleParsed = false;

	dontUseScreenshotTransparency = false;
	bool dontUseScreenshotTransparencyParsed = false;

	drawPushboxCheckSeparately = true;
	bool drawPushboxCheckSeparatelyParsed = false;
	
	modWindowVisibleOnStart = true;
	bool modWindowVisibleOnStartParsed = false;
	
	closingModWindowAlsoHidesFramebar = true;
	bool closingModWindowAlsoHidesFramebarParsed = false;

	std::string accum;
	char buf[128];
	if (file) {
		while (true) {
			accum.clear();
			bool exitOuterLoop = false;
			bool exitOuterLoopIfEmpty = false;
			while (true) {
				if (!fgets(buf, 127, file)) {
					if (ferror(file)) {
						exitOuterLoop = true;
						strerror_s(errorString, errno);
						logwrap(fprintf(logfile, "Error reading INI file: %s\n", errorString));
					}
					exitOuterLoopIfEmpty = true;
					break;
				}
				buf[127] = '\0';
				accum += buf;
				if (buf[strlen(buf) - 1] == '\n') break;
			}
			if (exitOuterLoop) {
				break;
			}
			if (exitOuterLoopIfEmpty && accum.empty()) {
				break;
			}
			std::string keyName = parseKeyName(accum.c_str());
			std::string keyNameUpper = toUppercase(keyName);
			std::string keyValue = getKeyValue(accum.c_str());
			auto found = keyCombosToParse.find(keyNameUpper);
			if (found != keyCombosToParse.end()) {
				found->second.isParsed = parseKeys(found->second.name, keyValue, *found->second.keyCombo);
			}
			if (!slowmoTimesParsed && _stricmp(keyName.c_str(), "slowmoTimes") == 0) {
				slowmoTimesParsed = parseInteger("slowmoTimes", keyValue, slowmoTimes);
			}
			if (!framebarHeightParsed && _stricmp(keyName.c_str(), "framebarHeight") == 0) {
				framebarHeightParsed = parseInteger("framebarHeight", keyValue, framebarHeight);
			}
			if (!lowProfileCutoffPointParsed && _stricmp(keyName.c_str(), "lowProfileCutoffPoint") == 0) {
				lowProfileCutoffPointParsed = parseInteger("lowProfileCutoffPoint", keyValue, lowProfileCutoffPoint);
			}
			if (!displayUIOnTopOfPauseMenuParsed && _stricmp(keyName.c_str(), "displayUIOnTopOfPauseMenu") == 0) {
				displayUIOnTopOfPauseMenuParsed = parseBoolean("displayUIOnTopOfPauseMenu", keyValue, displayUIOnTopOfPauseMenu);
			}
			if (!neverIgnoreHitstopParsed && _stricmp(keyName.c_str(), "neverIgnoreHitstop") == 0) {
				neverIgnoreHitstopParsed = parseBoolean("neverIgnoreHitstop", keyValue, neverIgnoreHitstop);
			}
			if (!considerRunAndWalkNonIdleParsed && _stricmp(keyName.c_str(), "considerRunAndWalkNonIdle") == 0) {
				considerRunAndWalkNonIdleParsed = parseBoolean("considerRunAndWalkNonIdle", keyValue, considerRunAndWalkNonIdle);
			}
			if (!considerCrouchNonIdleParsed && _stricmp(keyName.c_str(), "considerCrouchNonIdle") == 0) {
				considerCrouchNonIdleParsed = parseBoolean("considerCrouchNonIdle", keyValue, considerCrouchNonIdle);
			}
			if (!useSimplePixelBlenderParsed && _stricmp(keyName.c_str(), "useSimplePixelBlender") == 0) {
				useSimplePixelBlenderParsed = parseBoolean("useSimplePixelBlender", keyValue, useSimplePixelBlender);
			}
			if (!dontShowBoxesParsed && _stricmp(keyName.c_str(), "dontShowBoxes") == 0) {
				dontShowBoxesParsed = parseBoolean("dontShowBoxes", keyValue, dontShowBoxes);
			}
			if (!neverDisplayGrayHurtboxesParsed && _stricmp(keyName.c_str(), "neverDisplayGrayHurtboxes") == 0) {
				neverDisplayGrayHurtboxesParsed = parseBoolean("neverDisplayGrayHurtboxes", keyValue, neverDisplayGrayHurtboxes);
			}
			if (!showFramebarParsed && _stricmp(keyName.c_str(), "showFramebar") == 0) {
				showFramebarParsed = parseBoolean("showFramebar", keyValue, showFramebar);
			}
			if (!showFramebarInTrainingModeParsed && _stricmp(keyName.c_str(), "showFramebarInTrainingMode") == 0) {
				showFramebarInTrainingModeParsed = parseBoolean("showFramebarInTrainingMode", keyValue, showFramebarInTrainingMode);
			}
			if (!showFramebarInReplayModeParsed && _stricmp(keyName.c_str(), "showFramebarInReplayMode") == 0) {
				showFramebarInReplayModeParsed = parseBoolean("showFramebarInReplayMode", keyValue, showFramebarInReplayMode);
			}
			if (!showFramebarInOtherModesParsed && _stricmp(keyName.c_str(), "showFramebarInOtherModes") == 0) {
				showFramebarInOtherModesParsed = parseBoolean("showFramebarInOtherModes", keyValue, showFramebarInOtherModes);
			}
			if (!showStrikeInvulOnFramebarParsed && _stricmp(keyName.c_str(), "showStrikeInvulOnFramebar") == 0) {
				showStrikeInvulOnFramebarParsed = parseBoolean("showStrikeInvulOnFramebar", keyValue, showStrikeInvulOnFramebar);
			}
			if (!showSuperArmorOnFramebarParsed && _stricmp(keyName.c_str(), "showSuperArmorOnFramebar") == 0) {
				showSuperArmorOnFramebarParsed = parseBoolean("showSuperArmorOnFramebar", keyValue, showSuperArmorOnFramebar);
			}
			if (!showThrowInvulOnFramebarParsed && _stricmp(keyName.c_str(), "showThrowInvulOnFramebar") == 0) {
				showThrowInvulOnFramebarParsed = parseBoolean("showThrowInvulOnFramebar", keyValue, showThrowInvulOnFramebar);
			}
			if (!showFirstFramesOnFramebarParsed && _stricmp(keyName.c_str(), "showFirstFramesOnFramebar") == 0) {
				showFirstFramesOnFramebarParsed = parseBoolean("showFirstFramesOnFramebar", keyValue, showFirstFramesOnFramebar);
			}
			if (!useColorblindHelpParsed && _stricmp(keyName.c_str(), "useColorblindHelp") == 0) {
				useColorblindHelpParsed = parseBoolean("useColorblindHelp", keyValue, useColorblindHelp);
			}
			if (!considerKnockdownWakeupAndAirtechIdleParsed && _stricmp(keyName.c_str(), "considerKnockdownWakeupAndAirtechIdle") == 0) {
				considerKnockdownWakeupAndAirtechIdleParsed = parseBoolean("considerKnockdownWakeupAndAirtechIdle", keyValue, considerKnockdownWakeupAndAirtechIdle);
			}
			if (!allowContinuousScreenshottingParsed && _stricmp(keyName.c_str(), "allowContinuousScreenshotting") == 0) {
				allowContinuousScreenshottingParsed = parseBoolean("allowContinuousScreenshotting", keyValue, allowContinuousScreenshotting);
			}
			if (!turnOffPostEffectWhenMakingBackgroundBlackParsed && _stricmp(keyName.c_str(), "turnOffPostEffectWhenMakingBackgroundBlack") == 0) {
				turnOffPostEffectWhenMakingBackgroundBlackParsed = parseBoolean("turnOffPostEffectWhenMakingBackgroundBlack", keyValue, turnOffPostEffectWhenMakingBackgroundBlack);
			}
			if (!startDisabledParsed && _stricmp(keyName.c_str(), "startDisabled") == 0) {
				startDisabledParsed = parseBoolean("startDisabled", keyValue, startDisabled);
			}
			if (!screenshotPathParsed && _stricmp(keyName.c_str(), "screenshotPath") == 0) {
				screenshotPathParsed = true;
				screenshotPath = keyValue;  // in UTF-8
				logwrap(fprintf(logfile, "Parsed screenshotPath (UTF8): %s\n", keyValue.c_str()));
			}
			if (!dontUseScreenshotTransparencyParsed && _stricmp(keyName.c_str(), "dontUseScreenshotTransparency") == 0) {
				dontUseScreenshotTransparencyParsed = parseBoolean("dontUseScreenshotTransparency", keyValue, dontUseScreenshotTransparency);
			}
			if (!drawPushboxCheckSeparatelyParsed && _stricmp(keyName.c_str(), "drawPushboxCheckSeparately") == 0) {
				drawPushboxCheckSeparatelyParsed = parseBoolean("drawPushboxCheckSeparately", keyValue, drawPushboxCheckSeparately);
			}
			if (!modWindowVisibleOnStartParsed && _stricmp(keyName.c_str(), "modWindowVisibleOnStart") == 0) {
				modWindowVisibleOnStartParsed = parseBoolean("modWindowVisibleOnStart", keyValue, modWindowVisibleOnStart);
			}
			if (!closingModWindowAlsoHidesFramebarParsed && _stricmp(keyName.c_str(), "closingModWindowAlsoHidesFramebar") == 0) {
				closingModWindowAlsoHidesFramebarParsed = parseBoolean("closingModWindowAlsoHidesFramebar", keyValue, closingModWindowAlsoHidesFramebar);
			}
			if (feof(file)) break;
		}
		fclose(file);
	}
	
	if (firstSettingsParse) {
		ui.visible = modWindowVisibleOnStart;
		if (startDisabled) {
			gifMode.modDisabled = true;
		}
	} else if (turnOffPostEffectWhenMakingBackgroundBlack) {
		if (keyboard.thisProcessWindow) {
			PostMessageW(keyboard.thisProcessWindow, WM_APP_TURN_OFF_POST_EFFECT_SETTING_CHANGED, FALSE, TRUE);
		}
	}
	
	for (auto it = keyCombosToParse.begin(); it != keyCombosToParse.end(); ++it) {
		if (!it->second.isParsed) {
			parseKeys(it->first.c_str(), it->second.defaultValue, *it->second.keyCombo);
		}
		keyboard.addNewKeyCodes(*it->second.keyCombo);
	}

	firstSettingsParse = false;
}

int Settings::findChar(const char* buf, char c, int startingPos) {
	const char* ptr = buf + startingPos;
	while (*ptr != '\0') {
		if (*ptr == c) return ptr - buf;
		++ptr;
	}
	return -1;
}

std::pair<int, int> Settings::trim(std::string& str) {
	if (str.empty()) return {0,0};
	const char* strStart = &str.front();
	const char* c = strStart;
	while (*c <= 32 && *c != '\0') {
		++c;
	}
	if (*c == '\0') {
		str.clear();
		return {c - strStart, 0};
	}

	const char* cEnd = strStart + str.size() - 1;
	while (cEnd >= c && *cEnd <= 32) {
		--cEnd;
	}
	if (cEnd < c) {
		str.clear();
		return {c - strStart, strStart + str.size() - 1 - cEnd};
	}

	str = std::string(c, cEnd - c + 1);
	return {c - strStart, strStart + str.size() - 1 - cEnd};
}

std::string Settings::toUppercase(const std::string& str) {
	std::string result;
	result.reserve(str.size());
	for (char c : str) {
		result.push_back(toupper(c));
	}
	return result;
}

std::vector<std::string> Settings::split(const std::string& str, char c) {
	std::vector<std::string> result;
	const char* strStart = &str.front();
	const char* strEnd = strStart + str.size();
	const char* prevPtr = strStart;
	const char* ptr = strStart;
	while (*ptr != '\0') {
		if (*ptr == c) {
			if (ptr > prevPtr) {
				result.emplace_back(prevPtr, ptr - prevPtr);
			} else if (ptr == prevPtr) {
				result.emplace_back();
			}
			prevPtr = ptr + 1;
		}
		++ptr;
	}
	if (prevPtr < strEnd) {
		result.emplace_back(prevPtr, strEnd - prevPtr);
	}
	return result;
}

bool Settings::parseKeys(const char* keyName, const std::string& keyValue, std::vector<int>& keyCodes) {
	if (!keyValue.empty()) {
		std::string keyValueUppercase = toUppercase(keyValue);
		std::vector<std::string> keyNames = split(keyValueUppercase, '+');
		for (std::string& str : keyNames) {
			trim(str);
			auto found = keys.find(str);
			if (found != keys.end()) {
				keyCodes.push_back(found->second.code);
			} else {
				logwrap(fprintf(logfile, "Key combo parsing error: key not found %s\n", str.c_str()));
				return false;
			}
		}
		logwrap(fprintf(logfile, "Parsed key codes for %s: %s\n", keyName, keyValue.c_str()));
	} else {
		logwrap(fprintf(logfile, "Parsed that key codes are empty for %s\n", keyName));
	}
	return true;
}

bool Settings::parseInteger(const char* keyName, const std::string& keyValue, std::atomic_int& integer) {
	for (auto it = keyValue.begin(); it != keyValue.end(); ++it) {
		if (!(*it >= '0' && *it <= '9')) return false;  // apparently atoi doesn't do this check
	}
	int result = std::atoi(keyValue.c_str());
	if (result == 0 && keyValue != "0") return false;
	integer = result;
	logwrap(fprintf(logfile, "Parsed integer for %s: %d\n", keyName, integer.load()));
	return true;
}

bool Settings::parseBoolean(const char* keyName, const std::string& keyValue, std::atomic_bool& aBooleanValue) {
	if (_stricmp(keyValue.c_str(), "true") == 0) {
		logwrap(fprintf(logfile, "Parsed boolean for %s: %d\n", keyName, 1));
		aBooleanValue = true;
		return true;
	}
	if (_stricmp(keyValue.c_str(), "false") == 0) {
		logwrap(fprintf(logfile, "Parsed boolean for %s: %d\n", keyName, 0));
		aBooleanValue = false;
		return true;
	}
	return false;
}

const char* Settings::formatBoolean(bool value) {
	static const char* trueStr = "true";
	static const char* falseStr = "false";
	return value ? trueStr : falseStr;
}

int Settings::findMinCommentPos(const char* buf) {
	int colonPos = findChar(buf, ';');
	int hashtagPos = findChar(buf, '#');
	int minCommentPos = -1;
	if (colonPos != -1) minCommentPos = colonPos;
	if (minCommentPos == -1 || hashtagPos != -1 && minCommentPos != -1 && hashtagPos < minCommentPos) minCommentPos = hashtagPos;
	return minCommentPos;
}

std::string Settings::parseKeyName(const char* buf) {

	int minCommentPos = findMinCommentPos(buf);

	int equalSignPos = findChar(buf, '=');

	if (equalSignPos == -1 || minCommentPos != -1 && equalSignPos != -1 && equalSignPos > minCommentPos) return std::string{};

	std::string keyNameStr(buf, equalSignPos);
	trim(keyNameStr);

	return keyNameStr;
}

std::string Settings::getKeyValue(const char* buf) {
	int minCommentPos = findMinCommentPos(buf);
	int equalSignPos = findChar(buf, '=');

	if (equalSignPos == -1 || minCommentPos != -1 && equalSignPos != -1 && equalSignPos > minCommentPos) return std::string{};

	const char* bufPos = buf + equalSignPos + 1;
	size_t bufLength = strlen(buf);
	if (minCommentPos != -1) bufLength -= (bufLength - minCommentPos);
	int lengthFromBufPos = buf + bufLength - bufPos;
	if (lengthFromBufPos == 0) return std::string{};
	std::string keyValue(bufPos, lengthFromBufPos);
	trim(keyValue);

	return keyValue;
}

std::wstring Settings::getCurrentDirectory() {
	DWORD requiredSize = GetCurrentDirectoryW(0, NULL);
	if (!requiredSize) {
		WinError winErr;
		logwrap(fprintf(logfile, "GetCurrentDirectoryW failed: %ls\n", winErr.getMessage()));
		return std::wstring{};
	}
	std::wstring currentDir;
	currentDir.resize(requiredSize - 1);
	if (!GetCurrentDirectoryW(currentDir.size() + 1, &currentDir.front())) {
		WinError winErr;
		logwrap(fprintf(logfile, "GetCurrentDirectoryW (second call) failed: %ls\n", winErr.getMessage()));
		return std::wstring{};
	}
	return currentDir;
}

const char* Settings::getComboRepresentation(std::vector<int>& toggle) {
	for (auto it = keyCombosToParse.begin(); it != keyCombosToParse.end(); ++it) {
		KeyComboToParse& combo = it->second;
		if (combo.keyCombo == &toggle) {
			if (!combo.representationGenerated) combo.generateRepresentation();
			if (combo.representation.empty()) return "";
			return combo.representation.c_str();
		}
	}
	return "";
}

void Settings::trashComboRepresentation(std::vector<int>& toggle) {
	for (auto it = keyCombosToParse.begin(); it != keyCombosToParse.end(); ++it) {
		KeyComboToParse& combo = it->second;
		if (combo.keyCombo == &toggle) {
			combo.representationGenerated = false;
			return;
		}
	}
}

void Settings::KeyComboToParse::generateRepresentation() {
	representation.clear();
	representationGenerated = true;
	std::vector<int>& kCombo = *keyCombo;
	bool isFirst = true;
	for (int code : kCombo) {
		const char* keyStr = settings.getKeyTxtName(code);
		if (!isFirst) {
			representation += "+";
		}
		isFirst = false;
		representation += keyStr;
	}
}

const char* Settings::getKeyTxtName(int code) {
	auto found = reverseKeys.find(code);
	if (found != reverseKeys.end()) {
		return found->second->name;
	}
	return "";
}

const char* Settings::getKeyRepresentation(int code) {
	auto found = reverseKeys.find(code);
	if (found != reverseKeys.end()) {
		return found->second->uiName;
	}
	return "";
}

void Settings::writeSettings() {
	HANDLE temp = directoryChangeHandle;
	directoryChangeHandle = NULL;
	changesListenerWakeType = WAKE_TYPE_WRITING_FILE;
	SetEvent(changesListenerWakeEvent);
	FindCloseChangeNotification(temp);
	
	writeSettingsMain();
	
	registerListenerForChanges();
}

void Settings::writeSettingsMain() {
	logwrap(fputs("Writing settings\n", logfile));
	HANDLE file = CreateFileW(settingsPath.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		WinError winErr;
		logwrap(fprintf(logfile, "Could not open INI file: %ls\n", winErr.getMessage()));
		return;
	}
	struct LineInfo {
		int lineNumber = -1;
		int keyPos = -1;
		int equalSignPos = -1;
		int valuePos = -1;
		int commentPos = -1;
		std::string line;
		std::string key;
		std::string keyUpper;
		std::string value;
		std::string comment;
		bool needReform = false;
	};
	std::unordered_map<std::string, LineInfo*> keyToLine;
	int lineNumber = 0;
	std::list<LineInfo> lines;
	std::string accum;
	char buf[128];
	buf[0] = '\0';
	int bufContentSize = 0;
	bool reachedEnd = false;
	bool rCharDetected = false;
	while (true) {
		accum.clear();
		++lineNumber;
		int pos = findChar(buf, '\n');
		bool needToReadMore = true;
		if (pos != -1) {
			needToReadMore = false;
			*(buf + pos) = '\0';
			if (pos > 0 && *(buf + pos - 1) == '\r') {
				rCharDetected = true;
				*(buf + pos - 1) = '\0';
			}
			if (pos == 0 && !accum.empty() && accum.back() == '\r') {
				rCharDetected = true;
				accum.resize(accum.size() - 1);
			}
			accum.append(buf);
			if (pos + 1 >= bufContentSize) {
				bufContentSize = 0;
				buf[0] = '\0';
			} else {
				bufContentSize = bufContentSize - (pos + 1);
				memmove(buf, buf + pos + 1, bufContentSize + 1);
			}
		}
		while (needToReadMore) {
			if (bufContentSize) {
				accum += buf;
				// can have \r character at the end of buf here, will deal with it in the branch above
				bufContentSize = 0;
				buf[0] = '\0';
			}
			if (reachedEnd) {
				needToReadMore = false;
				break;
			}
			needToReadMore = true;
			DWORD bytesRead;
			if (!ReadFile(file, buf, sizeof buf - 1, &bytesRead, NULL)) {
				WinError winErr;
				logwrap(fprintf(logfile, "Error reading INI file: %ls\n", winErr.getMessage()));
				CloseHandle(file);
				return;
			}
			bufContentSize = bytesRead;
			buf[bytesRead] = '\0';
			char* bufPtr = buf;
			pos = findChar(buf, '\n');
			if (pos != -1) {
				*(buf + pos) = '\0';
				if (pos > 0 && *(buf + pos - 1) == '\r') {
					rCharDetected = true;
					*(buf + pos - 1) = '\0';
				}
				needToReadMore = false;
				if (pos == 0 && !accum.empty() && accum.back() == '\r') {
					rCharDetected = true;
					accum.resize(accum.size() - 1);
				}
				accum.append(buf);
				if (pos + 1 >= bufContentSize) {
					bufContentSize = 0;
					buf[0] = '\0';
				} else {
					bufContentSize = bufContentSize - (pos + 1);
					memmove(buf, buf + pos + 1, bufContentSize + 1);
				}
			}
			if (bytesRead < sizeof buf - 1) {
				if (needToReadMore && bytesRead > 0) {
					accum += buf;
					// can't have \r character here
					bufContentSize = 0;
					buf[0] = '\0';
				}
				needToReadMore = false;
				reachedEnd = true;
				break;
			}
		}
		if (reachedEnd && accum.empty()) {
			break;
		}
		
		int commentPos = findMinCommentPos(accum.c_str());
	
		int equalSignPos = findChar(accum.c_str(), '=');
		
		lines.emplace_back();
		LineInfo& li = lines.back();
		li.lineNumber = lineNumber;
		li.line = accum;
		
		if (equalSignPos == -1 || commentPos != -1 && equalSignPos != -1 && equalSignPos > commentPos) {
			li.comment = accum;
			li.commentPos = 0;
			continue;
		}
		
		std::string keyStr(accum.begin(), accum.begin() + equalSignPos);
		std::pair<int, int> trimResult = trim(keyStr);
		
		li.keyPos = trimResult.first;
		li.key = keyStr;
		li.keyUpper = toUppercase(keyStr);
		li.commentPos = commentPos;
		if (commentPos != -1) {
			li.comment.assign(accum.begin() + commentPos, accum.end());
		}
		li.equalSignPos = equalSignPos;
		
		const char* bufPos = &accum.front() + equalSignPos + 1;
		auto accumEnd = commentPos == -1 ? accum.end() : accum.begin() + commentPos;
		li.value.assign(accum.begin() + equalSignPos + 1, accumEnd);
		trimResult = trim(li.value);
		li.valuePos = equalSignPos + 1 + trimResult.first;
		
		auto found = keyCombosToParse.find(li.keyUpper);
		if (found != keyCombosToParse.end()) {
			std::vector<int> parsedCombo;
			if (!parseKeys(found->second.name, li.value, parsedCombo)) {
				li.needReform = true;
			} else if (compareKeyCombos(*found->second.keyCombo, parsedCombo) != 0) {
				li.needReform = true;
			}
			if (li.needReform) {
				li.value = getComboRepresentation(*found->second.keyCombo);
				logwrap(fprintf(logfile, "Combo representation for %s: %s\n", li.key.c_str(), li.value.c_str()));
				logwrap(fputs("Combo contains codes: ", logfile));
				for (int code : *found->second.keyCombo) {
					logwrap(fprintf(logfile, "%d ", code));
				}
				logwrap(fprintf(logfile, "\n"));
			}
		} else {
			li.needReform = true;
		}
		auto ktl = keyToLine.find(li.keyUpper);
		if (ktl != keyToLine.end()) {
			ktl->second = &li;
		} else {
			keyToLine.insert({li.keyUpper, &li});
		}
	}
	
	std::function<LineInfo&(const char*, const char*, const char*)> appendLine = [&](
		const char* name, const char* value, const char* iniDescription
	) -> LineInfo& {
		if (!lines.empty() && !isWhitespace(lines.back().line.c_str())) {
			lines.emplace_back();
			LineInfo& li = lines.back();
			li.commentPos = 0;
			li.lineNumber = lines.size();
		}
		for (const std::string& piece : split(iniDescription, '\n')) {
			lines.emplace_back();
			LineInfo& li = lines.back();
			li.lineNumber = lines.size();
			li.commentPos = 0;
			li.comment = piece;
			li.needReform = true;
		}
		lines.emplace_back();
		LineInfo& li = lines.back();
		li.lineNumber = lines.size();
		li.keyPos = 0;
		li.key = name;
		li.keyUpper = toUppercase(li.key);
		li.equalSignPos = li.key.size() + 1;
		li.valuePos = li.equalSignPos + 2;
		li.value = value;
		li.needReform = true;
		return li;
	};
	
	for (auto it = keyCombosToParse.begin(); it != keyCombosToParse.end(); ++it) {
		KeyComboToParse& k = it->second;
		auto found = keyToLine.find(it->first);
		if (found == keyToLine.end()) {
			appendLine(k.name, getComboRepresentation(*k.keyCombo), k.iniDescription);
		}
	}
	
	std::function<LineInfo&(const char*, const char*, const char*)> replaceOrAddSetting = [&](
		const char* name, const char* value, const char* description
	) -> LineInfo& {
		auto found = keyToLine.find(toUppercase(name));
		if (found == keyToLine.end()) {
			return appendLine(name, value, description);
		} else {
			found->second->needReform = true;
			found->second->value = value;
			return *found->second;
		}
	};
	
	replaceOrAddSetting("slowmoTimes", std::to_string(slowmoTimes).c_str(), getOtherINIDescription(&slowmoTimes));
	replaceOrAddSetting("allowContinuousScreenshotting", formatBoolean(allowContinuousScreenshotting), getOtherINIDescription(&allowContinuousScreenshotting));
	replaceOrAddSetting("startDisabled", formatBoolean(startDisabled), getOtherINIDescription(&startDisabled));
	std::string scrPathCpy;
	{
		std::unique_lock<std::mutex> screenshotGuard(screenshotPathMutex);
		scrPathCpy = screenshotPath;
	}
	logwrap(fprintf(logfile, "Writing screenshot path: %s\n", screenshotPath.c_str()));
	LineInfo& li = replaceOrAddSetting("screenshotPath", scrPathCpy.c_str(), getOtherINIDescription(&screenshotPath));
	if (li.value.empty()) {
		li.commentPos = li.equalSignPos + 2;
		li.comment = ";C:\\Users\\yourUser\\Desktop\\test screenshot name.png   don't forget to uncomment (; is a comment)";
	}
	
	replaceOrAddSetting("dontUseScreenshotTransparency", formatBoolean(dontUseScreenshotTransparency), getOtherINIDescription(&dontUseScreenshotTransparency));
	replaceOrAddSetting("turnOffPostEffectWhenMakingBackgroundBlack", formatBoolean(turnOffPostEffectWhenMakingBackgroundBlack), getOtherINIDescription(&turnOffPostEffectWhenMakingBackgroundBlack));
	replaceOrAddSetting("drawPushboxCheckSeparately", formatBoolean(drawPushboxCheckSeparately), getOtherINIDescription(&drawPushboxCheckSeparately));
	replaceOrAddSetting("useSimplePixelBlender", formatBoolean(useSimplePixelBlender), getOtherINIDescription(&useSimplePixelBlender));
	replaceOrAddSetting("modWindowVisibleOnStart", formatBoolean(modWindowVisibleOnStart), getOtherINIDescription(&modWindowVisibleOnStart));
	replaceOrAddSetting("closingModWindowAlsoHidesFramebar", formatBoolean(closingModWindowAlsoHidesFramebar), getOtherINIDescription(&closingModWindowAlsoHidesFramebar));
	replaceOrAddSetting("neverDisplayGrayHurtboxes", formatBoolean(neverDisplayGrayHurtboxes), getOtherINIDescription(&neverDisplayGrayHurtboxes));
	replaceOrAddSetting("dontShowBoxes", formatBoolean(dontShowBoxes), getOtherINIDescription(&dontShowBoxes));
	replaceOrAddSetting("displayUIOnTopOfPauseMenu", formatBoolean(displayUIOnTopOfPauseMenu), getOtherINIDescription(&displayUIOnTopOfPauseMenu));
	replaceOrAddSetting("lowProfileCutoffPoint", std::to_string(lowProfileCutoffPoint).c_str(), getOtherINIDescription(&lowProfileCutoffPoint));
	replaceOrAddSetting("showFramebar", formatBoolean(showFramebar), getOtherINIDescription(&showFramebar));
	replaceOrAddSetting("showFramebarInTrainingMode", formatBoolean(showFramebarInTrainingMode), getOtherINIDescription(&showFramebarInTrainingMode));
	replaceOrAddSetting("showFramebarInReplayMode", formatBoolean(showFramebarInReplayMode), getOtherINIDescription(&showFramebarInReplayMode));
	replaceOrAddSetting("showFramebarInOtherModes", formatBoolean(showFramebarInOtherModes), getOtherINIDescription(&showFramebarInOtherModes));
	replaceOrAddSetting("showStrikeInvulOnFramebar", formatBoolean(showStrikeInvulOnFramebar), getOtherINIDescription(&showStrikeInvulOnFramebar));
	replaceOrAddSetting("showSuperArmorOnFramebar", formatBoolean(showSuperArmorOnFramebar), getOtherINIDescription(&showSuperArmorOnFramebar));
	replaceOrAddSetting("showThrowInvulOnFramebar", formatBoolean(showThrowInvulOnFramebar), getOtherINIDescription(&showThrowInvulOnFramebar));
	replaceOrAddSetting("showFirstFramesOnFramebar", formatBoolean(showFirstFramesOnFramebar), getOtherINIDescription(&showFirstFramesOnFramebar));
	replaceOrAddSetting("framebarHeight", std::to_string(framebarHeight).c_str(), getOtherINIDescription(&framebarHeight));
	replaceOrAddSetting("neverIgnoreHitstop", formatBoolean(neverIgnoreHitstop), getOtherINIDescription(&neverIgnoreHitstop));
	replaceOrAddSetting("considerRunAndWalkNonIdle", formatBoolean(considerRunAndWalkNonIdle), getOtherINIDescription(&considerRunAndWalkNonIdle));
	replaceOrAddSetting("considerCrouchNonIdle", formatBoolean(considerCrouchNonIdle), getOtherINIDescription(&considerCrouchNonIdle));
	replaceOrAddSetting("considerKnockdownWakeupAndAirtechIdle", formatBoolean(considerKnockdownWakeupAndAirtechIdle), getOtherINIDescription(&considerKnockdownWakeupAndAirtechIdle));
	replaceOrAddSetting("useColorblindHelp", formatBoolean(useColorblindHelp), getOtherINIDescription(&useColorblindHelp));
	
	SetFilePointer(file, 0, NULL, FILE_BEGIN);
	
	std::string lineStr;
	for (LineInfo& li : lines) {
		lineStr.clear();
		DWORD bytesWritten;
		if (!li.needReform) {
			WriteFile(file, li.line.c_str(), li.line.size(), &bytesWritten, NULL);
		} else {
			if (li.keyPos != -1) {
				lineStr.append(li.keyPos, ' ');
				lineStr += li.key;
			}
			if (li.equalSignPos != -1) {
				if (li.equalSignPos > (int)lineStr.size()) {
					lineStr.append(li.equalSignPos - lineStr.size(), ' ');
				}
				lineStr += '=';
			}
			if (li.valuePos != -1) {
				if (li.valuePos > (int)lineStr.size()) {
					lineStr.append(li.valuePos - lineStr.size(), ' ');
				}
				lineStr += li.value;
			}
			if (li.commentPos != -1) {
				if (li.commentPos > (int)lineStr.size()) {
					lineStr.append(li.commentPos - lineStr.size(), ' ');
				}
				lineStr += li.comment;
			}
			WriteFile(file, lineStr.c_str(), lineStr.size(), &bytesWritten, NULL);
		}
		if (rCharDetected) {
			WriteFile(file, "\r\n", 2, &bytesWritten, NULL);
		} else {
			WriteFile(file, "\n", 1, &bytesWritten, NULL);
		}
	}
	
	SetEndOfFile(file);
	
	CloseHandle(file);
}

void Settings::registerListenerForChanges() {
	std::wstring currentDir = settingsPath;
	if (!currentDir.empty()) {
		for (auto it = currentDir.begin() + (currentDir.size() - 1); ; --it) {
			if (*it == L'\\') {
				currentDir.resize(it - currentDir.begin());
				break;
			}
			if (it == currentDir.begin()) break;
		}
	}
	logwrap(fprintf(logfile, "registerListenerForChanges currentDir: %ls\n", currentDir.c_str()));
	
	if (!currentDir.empty()) {
		directoryChangeHandle = FindFirstChangeNotificationW(
			currentDir.c_str(), // directory to watch 
			FALSE,              // do not watch subtree 
			FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE); // watch file name changes and last write date changes
		if (directoryChangeHandle == INVALID_HANDLE_VALUE || !directoryChangeHandle) {
			WinError winErr;
			logwrap(fprintf(logfile, "FindFirstChangeNotificationW failed: %ls\n", winErr.getMessage()));
			directoryChangeHandle = NULL;
		} else if (!changesListener) {
			changesListenerWakeEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
			changeListenerExitedEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
			DWORD changesListenerId = 0;
			changesListener = CreateThread(NULL, 0, changesListenerLoop, NULL, 0, &changesListenerId);
			logwrap(fprintf(logfile, "changesListenerId: %d\n", changesListenerId));
		} else {
			changesListenerWakeType = WAKE_TYPE_WRITING_FILE;
			SetEvent(changesListenerWakeEvent);
		}
	} else {
		logwrap(fputs("registerListenerForChanges: the current directory is empty\n", logfile));
	}
}

DWORD WINAPI Settings::changesListenerLoop(LPVOID lpThreadParameter) {
	struct SetEventOnExit {
	public:
		~SetEventOnExit() {
			SetEvent(settings.changeListenerExitedEvent);
		}
	} setEventOnExit;
	settings.changesListenerStarted = true;
	while (true) {
		HANDLE handles[2] { 0 };
		int handlesCount = 0;
		if (settings.directoryChangeHandle) {
			handles[handlesCount++] = settings.directoryChangeHandle;
		}
		handles[handlesCount++] = settings.changesListenerWakeEvent;
		DWORD result = WaitForMultipleObjects(handlesCount, handles, FALSE, INFINITE);
		if (result == WAIT_OBJECT_0 && handlesCount == 2) {
			if (settings.directoryChangeHandle) {
				if (!settings.firstSettingsParse && keyboard.thisProcessWindow) {
					PostMessageW(keyboard.thisProcessWindow, WM_APP_SETTINGS_FILE_UPDATED, 0, 0);
				}
				if (!FindNextChangeNotification(settings.directoryChangeHandle)) {
					WinError winErr;
					logwrap(fprintf(logfile, "FindNextChangeNotification failed: %ls\n", winErr.getMessage()));
					FindCloseChangeNotification(settings.directoryChangeHandle);
					settings.directoryChangeHandle = NULL;
					return 0;
				}
			}
		} else if (result == WAIT_OBJECT_0 + (handlesCount == 2 ? 1 : 0)) {
			if (settings.changesListenerWakeType == WAKE_TYPE_EXIT) {
				return 0;
			}
		}
	}
}

bool Settings::isWhitespace(const char* str) {
	for (const char* c = str; ; ++c) {
		char cVal = *c;
		if (cVal == '\0') return true;
		if (cVal > 32) return false;
	}
}

int Settings::compareKeyCombos(const std::vector<int>& left, const std::vector<int>& right) {
	auto itLeft = left.cbegin();
	auto itLeftEnd = left.cend();
	auto itRight = right.cbegin();
	auto itRightEnd = right.cend();
	while (true) {
		if (itLeft == itLeftEnd) {
			if (itRight == itRightEnd) return 0;
			return -1;
		} else if (itRight == itRightEnd) return 1;
		if (*itLeft != *itRight) {
			if (*itLeft < *itRight) return -1;
			return 1;
		}
		++itLeft;
		++itRight;
	}
}

void Settings::onKeyCombosUpdated() {
	keyboard.removeAllKeyCodes();
	for (auto it = keyCombosToParse.begin(); it != keyCombosToParse.end(); ++it) {
		KeyComboToParse& k = it->second;
		k.representationGenerated = false;
		keyboard.addNewKeyCodes(*k.keyCombo);
	}
}

void Settings::getComboInfo(std::vector<int>& keyCombo, ComboInfo* info) {
	for (auto it = keyCombosToParse.begin(); it != keyCombosToParse.end(); ++it) {
		KeyComboToParse& combo = it->second;
		if (combo.keyCombo == &keyCombo) {
			info->uiName = combo.uiName;
			info->uiDescription = combo.uiDescription.c_str();
			return;
		}
	}
	info->uiName = info->uiDescription = "";
}

const char* Settings::getOtherUIName(void* ptr) {
	return pointerIntoSettingsIntoDescription[(BYTE*)ptr - (BYTE*)this - offsetof(Settings, settingsMembersStart)]->uiName;
}

const char* Settings::getOtherUIDescription(void* ptr) {
	return pointerIntoSettingsIntoDescription[(BYTE*)ptr - (BYTE*)this - offsetof(Settings, settingsMembersStart)]->uiDescription.c_str();
}

const char* Settings::getOtherINIDescription(void* ptr) {
	return pointerIntoSettingsIntoDescription[(BYTE*)ptr - (BYTE*)this - offsetof(Settings, settingsMembersStart)]->iniDescription;
}

void Settings::registerOtherDescription(void* ptr, const char* iniName, const char* uiName, const char* uiPath, const char* iniDescription) {
	otherDescriptions.emplace_back();
	OtherDescription& desc = otherDescriptions.back();
	desc.ptr = ptr;
	desc.iniNameAllCaps = toUppercase(iniName);
	desc.iniName = iniName;
	desc.uiName = uiName;
	desc.uiFullPath.reserve(1 + strlen(uiPath) + 3 + strlen(uiName) + 1);
	desc.uiFullPath = '\'';
	desc.uiFullPath += uiPath;
	desc.uiFullPath += " - ";
	desc.uiFullPath += uiName;
	desc.uiFullPath += '\'';
	desc.iniDescription = iniDescription;
}

static void makeUppercase(std::string& destination) {
	for (char& c : destination) {
		c = toupper(c);
	}
}

std::string Settings::convertToUiDescription(const char* iniDescription) {
	std::string result;
	if (*iniDescription != '\0') {
		
		std::string iniNameLookup;
		std::vector<const char*> mentionedFullPaths;
		result.reserve(strlen(iniDescription));
		
		enum ParseMode {
			PARSING_NEWLINE,
			PARSING_SEMICOLON,
			PARSING_WHITESPACE,
			PARSING_QUOTATION
		} parsingMode = PARSING_NEWLINE;
		
		const char* quotationStart = nullptr;
		
		for (; ; ++iniDescription) {
			char cVal = *iniDescription;
			if (cVal == '\0') break;
			if (cVal == ';') {
				if (parsingMode == PARSING_NEWLINE) {
					parsingMode = PARSING_SEMICOLON;
					continue;
				} else if (parsingMode == PARSING_QUOTATION) {
					result.append(quotationStart, iniDescription - quotationStart);
					parsingMode = PARSING_WHITESPACE;
				}
			} else if (cVal == '\n') {
				if (parsingMode == PARSING_QUOTATION) {
					result.append(quotationStart, iniDescription - quotationStart);
				}
				parsingMode = PARSING_NEWLINE;
			} else if (cVal <= 32) {
				if (parsingMode == PARSING_SEMICOLON) {
					parsingMode = PARSING_WHITESPACE;
					continue;
				} else if (parsingMode == PARSING_QUOTATION) {
					result.append(quotationStart, iniDescription - quotationStart);
					parsingMode = PARSING_WHITESPACE;
				}
			} else if (cVal == '"') {
				if (parsingMode == PARSING_WHITESPACE) {
					parsingMode = PARSING_QUOTATION;
					quotationStart = iniDescription;
					continue;
				} else if (parsingMode == PARSING_QUOTATION) {
					iniNameLookup.assign(quotationStart + 1, iniDescription - quotationStart - 1);
					makeUppercase(iniNameLookup);
					auto found = iniNameToUiNameMap.find(iniNameLookup.c_str());
					if (found == iniNameToUiNameMap.end()) {
						result.append(quotationStart, iniDescription - quotationStart + 1);
					} else {
						if (std::find(mentionedFullPaths.begin(), mentionedFullPaths.end(), found->second) != mentionedFullPaths.end()) {
							
						} else {
							result += found->second;
							mentionedFullPaths.push_back(found->second);
						}
					}
					parsingMode = PARSING_WHITESPACE;
					continue;
				}
			} else if (parsingMode == PARSING_QUOTATION) {
				continue;
			}
			result += cVal;
		}
		if (parsingMode == PARSING_QUOTATION) {
			result += quotationStart;
		}
	}
	return result;
}

int Settings::hashString(const char* str, int startingHash) {
	for (const char* c = str; *c != '\0'; ++c) {
		startingHash = startingHash * 0x89 + *c;
	}
	return startingHash;
}

Settings::KeyComboToParse::KeyComboToParse(const char* name, const char* uiName, std::vector<int>* keyCombo, const char* defaultValue, const char* iniDescription)
	: name(name), uiName(uiName), keyCombo(keyCombo), defaultValue(defaultValue), iniDescription(iniDescription) { }

int Settings::findCharRev(const char* buf, char c) {
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
