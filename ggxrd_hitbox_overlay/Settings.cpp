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
	addKey("JoystickBtn1", "JoystickBtn1", JOY_BTN_0);
	addKey("JoystickBtn2", "JoystickBtn2", JOY_BTN_1);
	addKey("JoystickBtn3", "JoystickBtn3", JOY_BTN_2);
	addKey("JoystickBtn4", "JoystickBtn4", JOY_BTN_3);
	addKey("JoystickLeftTrigger", "JoystickLeftTrigger", JOY_BTN_4);
	addKey("JoystickRightTrigger", "JoystickRightTrigger", JOY_BTN_5);
	addKey("JoystickLeftTrigger2", "JoystickLeftTrigger2", JOY_BTN_6);
	addKey("JoystickRightTrigger2", "JoystickRightTrigger2", JOY_BTN_7);
	addKey("JoystickBtn9", "JoystickBtn9", JOY_BTN_8);
	addKey("JoystickBtn10", "JoystickBtn10", JOY_BTN_9);
	addKey("JoystickBtn11", "JoystickBtn11", JOY_BTN_10);
	addKey("JoystickBtn12", "JoystickBtn12", JOY_BTN_11);
	addKey("JoystickBtn13", "JoystickBtn13", JOY_BTN_12);
	addKey("JoystickBtn14", "JoystickBtn14", JOY_BTN_13);
	addKey("JoystickBtn15", "JoystickBtn15", JOY_BTN_14);
	addKey("JoystickBtn16", "JoystickBtn16", JOY_BTN_15);
	addKey("LeftStickLeft", "LeftStickLeft", JOY_LEFT_STICK_LEFT);
	addKey("LeftStickUp", "LeftStickUp", JOY_LEFT_STICK_UP);
	addKey("LeftStickRight", "LeftStickRight", JOY_LEFT_STICK_RIGHT);
	addKey("LeftStickDown", "LeftStickDown", JOY_LEFT_STICK_DOWN);
	addKey("DPadLeft", "DPadLeft", JOY_DPAD_LEFT);
	addKey("DPadUp", "DPadUp", JOY_DPAD_UP);
	addKey("DPadRight", "DPadRight", JOY_DPAD_RIGHT);
	addKey("DPadDown", "DPadDown", JOY_DPAD_DOWN);
	addKey("PS4DualshockRightStickLeft", "PS4DualshockRightStickLeft", JOY_PS4_DUALSHOCK_RIGHT_STICK_LEFT);
	addKey("PS4DualshockRightStickUp", "PS4DualshockRightStickUp", JOY_PS4_DUALSHOCK_RIGHT_STICK_UP);
	addKey("PS4DualshockRightStickRight", "PS4DualshockRightStickRight", JOY_PS4_DUALSHOCK_RIGHT_STICK_RIGHT);
	addKey("PS4DualshockRightStickDown", "PS4DualshockRightStickDown", JOY_PS4_DUALSHOCK_RIGHT_STICK_DOWN);
	addKey("XboxTypeSRightStickLeft", "XboxTypeSRightStickLeft", JOY_XBOX_TYPE_S_RIGHT_STICK_LEFT);
	addKey("XboxTypeSRightStickUp", "XboxTypeSRightStickUp", JOY_XBOX_TYPE_S_RIGHT_STICK_UP);
	addKey("XboxTypeSRightStickRight", "XboxTypeSRightStickRight", JOY_XBOX_TYPE_S_RIGHT_STICK_RIGHT);
	addKey("XboxTypeSRightStickDown", "XboxTypeSRightStickDown", JOY_XBOX_TYPE_S_RIGHT_STICK_DOWN);

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
		"; 4) Hide HUD (interface)\n"
		"; GIF Mode can also be toggled using 'UI - Hitboxes - GIF Mode'.");
	insertKeyComboToParse("noGravityToggle", "No Gravity Toggle", &noGravityToggle, "F2",
		"; A keyboard shortcut to toggle No gravity mode\n"
		"; In the UI, this mode can also be toggled using 'UI - Hitboxes - No Gravity' checkbox.\n"
		"; No gravity mode is you can't fall basically");
	insertKeyComboToParse("freezeGameToggle", "Freeze Game Toggle", &freezeGameToggle, "F3",
		"; A keyboard shortcut to freeze the game\n"
		"; In the UI, this mode can also be toggled using 'UI - Hitboxes - Freeze Game' checkbox.\n");
	insertKeyComboToParse("slowmoGameToggle", "Slow-mo Game Toggle", &slowmoGameToggle, "F4",
		"; A keyboard shortcut to play the game in slow motion.\n"
		"; Please specify by how many times to slow the game down in \"slowmoTimes\".\n"
		"; In the UI, this mode can also be toggled using 'UI - Hitboxes - Slow-Mo Mode' checkbox.\n");
	insertKeyComboToParse("allowNextFrameKeyCombo", "Allow Next Frame", &allowNextFrameKeyCombo, "F5",
		"; A keyboard shortcut. Only works while the game is frozen using \"freezeGameToggle\".\n"
		"; Advances the game forward one frame.\n"
		"; In the UI, this mode can also be toggled using 'UI - Hitboxes - Next Frame' button.\n");
	insertKeyComboToParse("disableModToggle", "Disable Mod Toggle", &disableModKeyCombo, "F6",
		"; A keyboard shortcut to enable/disable the mod without having to load/unload it");
	insertKeyComboToParse("disableHitboxDisplayToggle", "Disable Hitbox Display Toggle", &disableHitboxDisplayToggle, "F7",
		"; A keyboard shortcut to enable/disable only the mod hitbox drawing feature:\n"
		"; the GIF mode and no gravity, etc will keep working.\n"
		"; In the UI, this mode can also be toggled using \"dontShowBoxes\" checkbox.\n");
	insertKeyComboToParse("screenshotBtn", "Take Screenshot", &screenshotBtn, "F8", "; A keyboard shortcut.\n"
		"; Takes a screenshot and saves it at \"screenshotPath\" path\n"
		"; To take screenshots over a transparent background you need to go to the game's\n"
		"; Display Settings and turn off Post-Effect (or use \"togglePostEffectOnOff\" and\n"
		"; \"turnOffPostEffectWhenMakingBackgroundBlack\" settings for this), then use GIF mode (make background dark).\n"
		"; Then screenshots will film character over transparent background.\n"
		"; If the \"dontUseScreenshotTransparency\" setting is true, screenshot will be without\n"
		"; transparency anyway.\n"
		"; In the UI, taking screenshots in possible using 'UI - Hitboxes - Take Screenshot' button.");
	insertKeyComboToParse("continuousScreenshotToggle", "Continuous Screenshot Toggle", &continuousScreenshotToggle, "",
		"; A keyboard shortcut.\n"
		"; This toggle can be used same way as \"screenshotBtn\" (when it's combined with\n"
		"; \"allowContinuousScreenshotting\" = true), except it's a separate key combination and when you\n"
		"; press it, it toggles the continuous screenshot taking every game logical frame. This\n"
		"; toggle does not require \"allowContinuousScreenshotting\" to be set to true,\n"
		"; or \"screenshotBtn\" to be set to anything\n."
		"; In the UI, you can toggle this mode using 'UI - Hitboxes - Continuous Screenshotting Mode' checkbox.");
	insertKeyComboToParse("gifModeToggleBackgroundOnly", "GIF Mode Toggle (Background Only)", &gifModeToggleBackgroundOnly, "",
		"; A keyboard shortcut to only toggle the \"background becomes black\" part of the \"gifModeToggle\".\n"
		"; Empty by default, which means no hotkey is assigned. Assign your desired hotkey manually here.\n"
		"; This option can be combined with the other \"only\" options, by sharing the same key binding for example.\n"
		"; Black background can also be toggled using 'UI - Hitboxes - Black Background'.");
	insertKeyComboToParse("togglePostEffectOnOff", "Toggle Post-Effect On/Off", &togglePostEffectOnOff, "",
		"; A keyboard shortcut to toggle the game's Settings - Display Settings - Post-Effect. Changing it this way does not\n"
		"; require the current match to be restarted.\n"
		"; Alternatively, you could set the \"turnOffPostEffectWhenMakingBackgroundBlack\" setting in this INI file to true\n"
		"; so that whenever you enter either the GIF mode or the GIF mode (black background only), the Post-Effect is\n"
		"; turned off automatically, and when you leave those modes, it gets turned back on.\n"
		"; In the UI, Post-Effect can be toggled using 'UI - Hitboxes - Post-Effect On' checkbox.\n"
		"; This hotkey is empty by default, which means no hotkey is assigned. Assign your desired hotkey manually here.\n"
		"; This option can be combined with the other hotkey options, by sharing the same key binding for example.");
	insertKeyComboToParse("gifModeToggleCameraCenterOnly", "GIF Mode Toggle (Camera Only)", &gifModeToggleCameraCenterOnly, "",
		"; A keyboard shortcut to only toggle the \"Camera is centered on you\" part of the \"gifModeToggle\".\n"
		"; In the UI, this mode can also be toggled using 'UI - Hitboxes - Camera Center on Player' checkbox.\n"
		"; Empty by default, which means no hotkey is assigned. Assign your desired hotkey manually here.\n"
		"; This option can be combined with the other \"only\" options, by sharing the same key binding for example");
	insertKeyComboToParse("toggleCameraCenterOpponent", "Center Camera On The Opponent", &toggleCameraCenterOpponent, "",
		"; A keyboard shortcut to toggle the camera to be centered on the opponent.\n"
		"; In the UI, this mode can also be toggled using 'UI - Hitboxes - Camera Center on Opponent' checkbox.\n"
		"; Empty by default, which means no hotkey is assigned. Assign your desired hotkey manually here.\n"
		"; This option can be combined with GIF Mode options, by sharing the same key binding for example");
	insertKeyComboToParse("gifModeToggleHideOpponentOnly", "GIF Mode Toggle (Hide Opponent Only)", &gifModeToggleHideOpponentOnly, "",
		"; A keyboard shortcut to only toggle the \"Opponent is invisible and invulnerable\" part of the \"gifModeToggle\".\n"
		"; In the UI, this mode can also be toggled using 'UI - Hitboxes - Hide Opponent' checkbox.\n"
		"; Empty by default, which means no hotkey is assigned. Assign your desired hotkey manually here.\n"
		"; This option can be combined with the other \"only\" options, by sharing the same key binding for example");
	insertKeyComboToParse("toggleHidePlayer", "Hide Player", &toggleHidePlayer, "",
		"; A keyboard shortcut to toggle hiding the player.\n"
		"; In the UI, this mode can also be toggled using 'UI - Hitboxes - Hide Player' checkbox.\n"
		"; Empty by default, which means no hotkey is assigned. Assign your desired hotkey manually here.\n"
		"; This option can be combined with GIF Mode options, by sharing the same key binding for example");
	insertKeyComboToParse("gifModeToggleHudOnly", "GIF Mode Toggle (HUD Only)", &gifModeToggleHudOnly, "",
		"; A keyboard shortcut to only toggle the \"hide HUD (interface)\" part of the \"gifModeToggle\".\n"
		"; In the UI, this mode can also be toggled using 'UI - Hitboxes - Hide HUD' checkbox.\n"
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
		"; they can get in the way when trying to do certain stuff such as take screenshots of hurtboxes.\n"
		"; In the UI, you can toggle the display of gray hurtboxes using 'UI - Hitboxes - Disable Gray Hurtboxes' checkbox.");
	insertKeyComboToParse("toggleNeverIgnoreHitstop", "Never Ignore Hitstop", &toggleNeverIgnoreHitstop, "",
		"; A keyboard shortcut.\n"
		"; Pressing this shortcut will disable/enable the \"neverIgnoreHitstop\" setting which controls whether\n"
		"; the framebar advances during hitstop.");
	insertKeyComboToParse("toggleShowInputHistory", "Disable Show Input History", &toggleShowInputHistory, "",
		"; A keyboard shortcut.\n"
		"; Pressing this shortcut will disable the input history shown via \"displayInputHistoryWhenObserving\"\n"
		"; and \"displayInputHistoryInSomeOfflineModes\". It only works when one of those settings is enabled\n"
		"; and is showing history in corresponding game mode.");
	
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
	registerOtherDescription(settingAndItsName(framebarTitleCharsMax), "Framebar Title Max Characters", settingsFramebarSettingsStr, "; A number.\n"
			"; Specifies the maximum number of characters that can be displayed in a framebar title.\n"
			"; This does not include the \"P1 \" and \"P2 \" additional text that is displayed when \"showPlayerInFramebarTitle\" is true.\n"
			"; You can use \"useSlangNames\" to help reduce the lengths of text displayed in framebar titles.\n"
			"; The standard value is 12.");
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
			"; Display mod's UI on top of the game's Pause Menu. This setting has no effect when \"dodgeObsRecording\" is true and\n"
			"; an OBS is connected to the game.");
	registerOtherDescription(settingAndItsName(dodgeObsRecording), "Dodge OBS Recording", settingsGeneralSettingsStr,
			"; Specify true or false.\n"
			"; To have this mod avoid OBS capture set this setting to true and also make sure\n"
			"; that in OBS, in Sources you selected your Source, clicked Cogwheel and unchecked the\n"
			"; 'Capture third-party overlays (such as steam)'.\n"
			"; I am very sorry, but the mod's UI, the framebar and the boxes cannot be drawn under the game's\n"
			"; Pause Menu and game's own UI while using 'Dodge OBS Recording'.");
	registerOtherDescription(settingAndItsName(neverIgnoreHitstop), "Never Ignore Hitstop", settingsFramebarSettingsStr,
			"; Specify true or false.\n"
			"; Normally we don't display hitstop in the framebar if both players are in hitstop on that frame,\n"
			"; unless a projectile or a blocking Baiken (see \"ignoreHitstopForBlockingBaiken\") is present.\n"
			"; If this is set to true, then we always show hitstop in the framebar.\n"
			"; After changing this setting, you don't need to repeat the moves or actions to see updated result in the framebar.\n"
			"; This setting can be changed with the \"toggleNeverIgnoreHitstop\" hotkey.");
	registerOtherDescription(settingAndItsName(ignoreHitstopForBlockingBaiken), "Ignore Hitstop For Blocking Baiken", settingsFramebarSettingsStr,
			"; Specify true or false.\n"
			"; Normally we don't display hitstop in the framebar if both players are in hitstop on that frame,\n"
			"; unless a projectile or a blocking Baiken is present.\n"
			"; If this is set to true, then we ignore the blocking Baiken part and display hitstop in the framebar only\n"
			"; when a projectile is present.\n"
			"; There's another setting that controls the display of hitstop in the framebar: \"neverIgnoreHitstop\".\n"
			"; If that setting is set to true, hitstop is always shown, no matter what \"ignoreHitstopForBlockingBaiken\" setting is.\n"
			"; After changing this setting, you need to repeat the moves or actions to see updated result in the framebar.");
	registerOtherDescription(settingAndItsName(considerRunAndWalkNonIdle), "Consider Running And Walking Non-Idle", settingsFramebarSettingsStr,
			"; Specify true or false.\n"
			"; Normally we consider running and walking as being idle, which does not advance the framebar forward.\n"
			"; The framebar only advances when one of the players is \"busy\".\n"
			"; If this is set to true, then one player running or walking will be treated same way as \"busy\" and will advance the framebar.\n"
			"; In the UI's 'Frame Advantage' display, idle running (except Leo's) and walking will still always be considered idle.");
	registerOtherDescription(settingAndItsName(considerCrouchNonIdle), "Consider Crouching Non-Idle", settingsFramebarSettingsStr,
			"; Specify true or false.\n"
			"; Normally we consider crouching as being idle, which does not advance the framebar forward.\n"
			"; The framebar only advances when one of the players is \"busy\".\n"
			"; If this is set to true, then one player crouching or walking will be treated same way as \"busy\" and will advance the framebar.\n"
			"; A dummy who is crouching automatically due to training settings will still not be considered \"busy\" no matter what.\n"
			"; In the UI's 'Frame Advantage' display, idle crouching will still always be considered idle.");
	registerOtherDescription(settingAndItsName(considerDummyPlaybackNonIdle), "Consider The Entirety Of Dummy Recording Playback Non-Idle", settingsFramebarSettingsStr,
			"; Specify true or false.\n"
			"; Normally we consider standing as being idle, which does not advance the framebar forward.\n"
			"; The framebar only advances when one of the players is \"busy\".\n"
			"; If this is set to true, then as soon as you tell the dummy to play a recording slot, the entirety of the playback will be considered\n"
			"; as \"busy\" and will advance the framebar, even if in parts of the recording the dummy is not doing anything.");
	registerOtherDescription(settingAndItsName(considerKnockdownWakeupAndAirtechIdle), "Consider Knockdown, Wakeup and Airtech Idle", settingsFramebarSettingsStr,
			"; Specify true or false\n"
			"; This controls whether a character being knocked down, waking up or air recovering causes the framebar to advance forward (if you're also idle).\n"
			"; Framebar only advances forward when one or both players are not idle.\n"
			"; Framebar advancing forward means it continuously overwrites its oldest contents with new data to display.\n"
			"; This could be bad if you wanted to study why a combo dropped, as some knockdowns can be very long and erase all the info you wanted to see.\n"
			"; Setting this to true may prevent that.\n"
			"; The first frame when the opponent is in OTG state (meaning if they get hit now, it will be OTG) and onwards -\n"
			"; those frames do not get included in the framebar when this setting is true.\n"
			"; But if you recover from your move later than the opponent enters OTG state, the frames are included anyway for your whole recovery\n"
			"; for both you and the opponent, which means OTG state may partially or fully get included into the framebar even while this setting is true.\n"
			"; In such cases, look for an animation start delimiter on the opponent's framebar, shown as a white ' between frames.");
	registerOtherDescription(settingAndItsName(considerIdleInvulIdle), "Consider Invul While Being Idle As Idle", settingsFramebarSettingsStr,
			"; Specify true or false\n"
			"; After waking up, or leaving blockstun or hitstun, or airteching, usually there's some strike and/or throw invul while\n"
			"; you're idle and are able to fully act. Framebar only advances forward when one or both players are not idle, however,\n"
			"; if this invul is present, you can set this setting to false to advance the framebar anyway.");
	registerOtherDescription(settingAndItsName(useSimplePixelBlender), "Use Simple CPU Pixel Blender", settingsHitboxSettingsStr,
			"; Specify true or false.\n"
			"; Setting this to true may increase the performance of transparent screenshotting which may be useful if you're screenshotting every frame.\n"
			"; The produced screenshots won't have such improvements as improving visibility of semi-transparent effects or changing hitbox outlines to\n"
			"; black when drawn over the same color.");
	registerOtherDescription(settingAndItsName(dontShowBoxes), "Don't Show Boxes", hitboxesStr,
			"; Specify true or false.\n"
			"; Setting this to true will hide all hurtboxes, hitboxes, pushboxes and other boxes and points.\n"
			"; \"disableHitboxDisplayToggle\" can be used to toggle this setting using a keyboard shortcut.");
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
			"; If false, the framebar will not be shown when in other modes even when \"showFramebar\" is true and the UI is open.\n"
			"; Note that the framebar will never be displayed in online mode when playing as a non-observer, even if this setting is true.\n"
			"; The reason for that is that it malfunctions and shows incorrect framedata when rollback frames happen.\n"
			"; It is possible to put more work into it to fix that, but it will take as many times more computing resources as\n"
			"; there are rollback frames and may start working slower on slower PCs.");
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
	registerOtherDescription(settingAndItsName(showOTGOnFramebar), "Show OTG", settingsFramebarSettingsStr,
			"; Specify true or false.\n"
			"; OTG state will be displayed using a gray v underneath a frame.");
	registerOtherDescription(settingAndItsName(showFirstFramesOnFramebar), "Show First Frames", settingsFramebarSettingsStr,
			"; Specify true or false.\n"
			"; When a player's animation changes from one to another, except in certain cases, the first frame of the new animation is denoted with\n"
			"; a ' mark before that frame. For some animations a first frame is denoted even when\n"
			"; the animation didn't change, but instead reached some important point. This includes entering hitstop.");
	registerOtherDescription(settingAndItsName(considerSimilarFrameTypesSameForFrameCounts), "Consider Similar Frame Types Same For Frame Count", settingsFramebarSettingsStr,
			"; Specify true or false.\n"
			"; This affects the frame counts that are displayed on the framebar. If true, the frame counter will not reset\n"
			"; between different frame graphics that all mean recovery or all mean startup and so on. Idle frames are not\n"
			"; affected by this and for them you should use the \"considerSimilarIdleFramesSameForFrameCounts\" setting.");
	registerOtherDescription(settingAndItsName(considerSimilarIdleFramesSameForFrameCounts), "Consider Similar Idle Frames Same For Frame Count", settingsFramebarSettingsStr,
			"; Specify true or false.\n"
			"; This affects the frame counts that are displayed on the framebar. If true, the frame counter will not reset\n"
			"; between different frame graphics that all mean idle.");
	registerOtherDescription(settingAndItsName(combineProjectileFramebarsWhenPossible), "Combine Projectile Framebars When Possible", settingsFramebarSettingsStr,
			"; Specify true or false.\n"
			"; When true, two or more projectile framebars will be combined into one if their active/non-active frames don't intersect.");
	registerOtherDescription(settingAndItsName(eachProjectileOnSeparateFramebar), "Each Projectile On A Separate Framebar", settingsFramebarSettingsStr,
			"; Specify true or false.\n"
			"; When true, projectiles will never be combined even if there are very many of them and they're all same.");
	registerOtherDescription(settingAndItsName(dontClearFramebarOnStageReset), "Don't Clear Framebar On Stage Reset", settingsFramebarSettingsStr,
			"; Specify true or false.\n"
			"; When true, the framebar won't be cleared when resetting the stage with Record+Playback or Backspace or Pause Menu - Reset Position in Training Mode,\n"
			"; or when a player dies or when a new rounds starts.");
	registerOtherDescription(settingAndItsName(dontTruncateFramebarTitles), "Don't Truncate Framebar Titles", settingsFramebarSettingsStr,
			"; Specify true or false.\n"
			"; Each framebar displays a label or title either to the left or to the right from it. The titles are truncated to 12 character by default.\n"
			"; By setting this setting to true, you can stop them from truncating and always display full titles.");
	registerOtherDescription(settingAndItsName(useSlangNames), "Use Slang In Move & Projectile Names", settingsFramebarSettingsStr,
			"; Specify true or false.\n"
			"; Each framebar displays a label or title either to the left or to the right from it.\n"
			"; When the title is too long, depending on the setting, it gets cut off. Setting this to true changes some\n"
			"; projectile titles to slang names to make framebar titles short so that they fit.\n"
			"; This also changes names of moves that are displayed in main UI window and other windows.");
	registerOtherDescription(settingAndItsName(allFramebarTitlesDisplayToTheLeft), "All Framebar Titles Display On The Left", settingsFramebarSettingsStr,
			"; Specify true or false.\n"
			"; Each framebar displays a label or title either to the left or to the right from it, depending on which player it belongs to.\n"
			"; By setting this setting to true, you can make all framebar titles always display on the left.\n"
			"; To help tell which player it is you can use the \"showPlayerInFramebarTitle\" setting.");
	registerOtherDescription(settingAndItsName(showPlayerInFramebarTitle), "Show Player In Framebar Title", settingsFramebarSettingsStr,
			"; Specify true or false.\n"
			"; Each framebar displays a label or title either to the left or to the right from it, depending on which player it belongs to.\n"
			"; The \"allFramebarTitlesDisplayToTheLeft\" setting can be used to make all framebar titles always display on the left.\n"
			"; How to tell which player it is then? This is where this setting comes in.\n"
			"; Setting this to true adds \"P1\" or \"P2\" color-coded text to each framebar's title.");
	registerOtherDescription(settingAndItsName(useColorblindHelp), "Use Colorblindness Help", settingsFramebarSettingsStr,
			"; Specify true or false.\n"
			"; If true, certain types of frames in the framebar will be displayed with distinct hatches on them.\n"
			"; Make sure the framebar is scaled wide enough and the screen resolution is large enough that you can see the hatches properly.\n"
			"; To scale the framebar you can drag its right edge.");
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
			"; the Post-Effect is turned off automatically, and when you leave those modes, it gets turned back on.\n"
			"; An alternative way to turn off Post-Effect without reload a match is using the 'UI - Hitboxes - Post-Effect On' checkbox,\n"
			"; or \"togglePostEffectOnOff\" toggle.");
	registerOtherDescription(settingAndItsName(drawPushboxCheckSeparately), "Draw Pushbox Check Separately", settingsHitboxSettingsStr,
			"; Specify true or false.\n"
			"; Setting this to true will make throw boxes show in an opponent-character-independent way:\n"
			"; The part of the throw box that checks for pushboxes proximity will be shown in blue,\n"
			"; while the part of the throw box that checks x or y of the origin point will be shown in purple\n"
			"; Setting this to false will combine both the checks of the throw so that you only see the final box\n"
			"; in blue which only checks the opponent's origin point. Be warned, such a throw box\n"
			"; is affected by the width of the opponent's pushbox. Say, on Potemkin, for example,\n"
			"; all ground throw ranges should be higher.");
	registerOtherDescription(settingAndItsName(frameAdvantage_dontUsePreBlockstunTime), "Frame Advantage: Don't Use Pre-Blockstun Time", settingsGeneralSettingsStr,
			"; Specify true or false.\n"
			"; This setting will affect how the 'Frame Advantage' field in the UI is calculated.\n"
			"; Normally, attacks put your opponent in blockstun right away, however, there are some moves that\n"
			"; may put your opponent in blockstun after the move is over. Such moves are usually projectiles, such\n"
			"; Ky j.D or I-No Antidepressant Scale.\n"
			"; When this setting is false, then, whenever the opponent enters blockstun, the time that you spent idle\n"
			"; before that gets included in the frame advantage with a +. For example:\n"
			"; You shoot a projectile and then recover. Then you and the opponent are just standing there for 1000000 frames.\n"
			"; Then, the projectile that you shot puts the opponent into blockstun for 1 frame. Your frame advantage will be 1000001.\n"
			"; Setting this to false will cause the idle time that you spent before the opponent entered blockstun to not be included\n"
			"; in your frame advantage, and your frame advantage in the example above will be just +1.\n"
			"; After changing this setting you don't need to repeat the last move, as the 'Frame Adv.' field will get updated automatically.");
	registerOtherDescription(settingAndItsName(skipGrabsInFramebar), "Skip Grab/Super Animations In Framebar", settingsFramebarSettingsStr,
			"; Specify true or false.\n"
			"; Setting this to true (default) will skip grab animations such as ground throw or some supers that connected in the framebar.");
	registerOtherDescription(settingAndItsName(showFramebarHatchedLineWhenSkippingGrab), "Show Hatched/Dashed Slicing Line When Skipping Grab/Super In Framebar", settingsFramebarSettingsStr,
			"; Specify true or false.\n"
			"; When this setting is true (default), if a grab or super is skipped because of the \"skipGrabsInFramebar\" setting,\n"
			"; a white line made out of small diagonal hatches will be displayed on the framebar in places where the grab or super was skipped.");
	registerOtherDescription(settingAndItsName(showFramebarHatchedLineWhenSkippingHitstop), "Show Hatched/Dashed Slicing Line When Skipping Hitstop In Framebar", settingsFramebarSettingsStr,
			"; Specify true or false.\n"
			"; When this setting is true (which is not the default), if hitstop is skipped because of the \"neverIgnoreHitstop\"\n"
			"; setting being false,\n"
			"; a white line made out of small diagonal hatches will be displayed on the framebar in places where hitstop was skipped.");
	registerOtherDescription(settingAndItsName(showFramebarHatchedLineWhenSkippingSuperfreeze), "Show Hatched/Dashed Slicing Line When Skipping Superfreeze In Framebar", settingsFramebarSettingsStr,
			"; Specify true or false.\n"
			"; When this setting is true (default), if superfreeze (also called superflash) is skipped, which is always the case,\n"
			"; a white line made out of small diagonal hatches will be displayed on the framebar in places where superfreeze was skipped.");
	registerOtherDescription(settingAndItsName(showComboProrationInRiscGauge), "Show Combo Proration In RISC Gauge", settingsGeneralSettingsStr,
			"; Specify true or false.\n"
			"; Setting this to true will modify the RISC gauge so that the middle represents 0 RISC and no combo proration,\n"
			"; the right half represents RISC, and the left half represents combo proration.");
	registerOtherDescription(settingAndItsName(displayInputHistoryWhenObserving), "Display Input History When Observing", settingsGeneralSettingsStr,
			"; Specify true or false.\n"
			"; Setting this to true will display both players' input history when observing online matches.\n"
			"; The associated hotkey setting for this setting is \"toggleShowInputHistory\".");
	registerOtherDescription(settingAndItsName(displayInputHistoryInSomeOfflineModes), "Display Input History In Some Offline Modes Vs CPU", settingsGeneralSettingsStr,
			"; Specify true or false.\n"
			"; Setting this to true will display both players' input history when playing against CPU in Episode/Story, offline Versus,\n"
			"; Tutorial, offline MOM and Mission.\n"
			"; The associated hotkey setting for this setting is \"toggleShowInputHistory\".");
	registerOtherDescription(settingAndItsName(showDurationsInInputHistory), "Display Durations In Input History", settingsGeneralSettingsStr,
			"; Specify true or false.\n"
			"; Setting this to true will display the duration of each input, in frames, in the input history.");
	registerOtherDescription(settingAndItsName(useAlternativeStaggerMashProgressDisplay), "Use Alternative Stagger Mash Progress Display", "Main UI Window - Stun/Stagger Mash",
			"; Specify true or false.\n"
			"; Setting this to true will display Progress differently in Stun/Stagger Mash window.\n"
			"; Instead of displaying it as Mashed + Animation Duration / Stagger Duration - 4, it will\n"
			"; display as Animation Duration / Stagger Duration - 4 - Mashed");
	registerOtherDescription(settingAndItsName(dontShowMayInteractionChecks), "Don't Show May Interaction Checks", settingsHitboxSettingsStr,
			"; Specify true or false.\n"
			"; When a May P or K Ball is on the screen, a circle is drawn around it, an extra point is displayed on the Ball,\n"
			"; and, when May is airborne, an extra point is displayed in the center of body of May, and a line connecting that\n"
			"; point to the extra point on the Ball is displayed.\n"
			"; For Dolphin, this displays an extra point on May, a line connecting that point to the origin point of the Dolphin,\n"
			"; and a circle around the Dolphin denoting the range in which May's extra point must be in order for May to hop\n"
			"; on the Dolphin.\n"
			"; When this setting is true, none of this is displayed.");
	registerOtherDescription(settingAndItsName(showMilliaBadMoonBuffHeight), "Show Millia Bad Moon Buff Height (Rev2 only)", "UI - Character specific",
			"; Specify true or false.\n"
			"; When this setting is on, and one of the character is Millia, a horizontal line is displayed high above the arena,\n"
			"; showing the height on which Millia's Bad Moon obtains some kind of attack powerup.\n"
			"; Millia's origin point must be above the line in order to gain the powerup.\n"
			"; Note that Bad Moon's maximum number of hits increases as it gets higher, up to 10 hits maximum.");
	registerOtherDescription(settingAndItsName(showFaustOwnFlickRanges), "Show Faust Thrown Item Flick Ranges", "UI - Character specific",
			"; Specify true or false.\n"
			"; When this setting is on, when Faust does a 5D, two ranges are shown around his flickpoint,\n"
			"; denoting ranges in which his thrown items' origin points must be to get either hit or homerun hit.");
	registerOtherDescription(settingAndItsName(showBedmanTaskCHeightBuffY), "Show Bedman Task C Height Buff Y", "UI - Character specific",
			"; Specify true or false.\n"
			"; When this setting is on, a horizontal line is constantly shown on the screen at the height above which\n"
			"; Bedman's Task C gains a buff.\n"
			"; This line is so high you can't see it unless you jump.");
	registerOtherDescription(settingAndItsName(showJackoGhostPickupRange), "Show Jack-O' Ghost Pickup Range", "UI - Character specific",
			"; Specify true or false.\n"
			"; When this setting is on, an infinite vertical box around each Ghost (house) denotes the range in which\n"
			"; Jack-O's origin point must be in order to pick up the Ghost or gain Tension from it.");
	registerOtherDescription(settingAndItsName(showJackoSummonsPushboxes), "Show Jack-O' Summons' Pushboxes", "UI - Character specific",
			"; Specify true or false.\n"
			"; When this setting is on, yellow pushboxes are drawn around the Servants and the Ghosts, similar to how they're drawn\n"
			"; around the players.");
	registerOtherDescription(settingAndItsName(showJackoAegisFieldRange), "Show Jack-O' Aegis Field Range", "UI - Character specific",
			"; Specify true or false.\n"
			"; When this setting is on, the white circle around Jack-O' shows the range where the Servants' and the Ghosts'\n"
			"; origin point must be in order to receive protection from Aegis Field.");
	registerOtherDescription(settingAndItsName(showJackoServantAttackRange), "Show Jack-O' Servant Attack Range", "UI - Character specific",
			"; Specify true or false.\n"
			"; When this setting is on, the white box around each Servant shows the area where the opponent's player's\n"
			"; origin point must be in order for the Servant to initiate an attack.");
	registerOtherDescription(settingAndItsName(ignoreScreenshotPathAndSaveToClipboard), "Ignore Screenshot Path And Save To Clipboard", settingsHitboxSettingsStr,
			"; Specify true or false.\n"
			"; When this setting is on, screenshots get saved to clipboard only, even if a screenshot path is specified.");
	registerOtherDescription(settingAndItsName(forceZeroPitchDuringCameraCentering), "Force Zero Pitch During Camera Centering", settingsHitboxSettingsStr,
			"; Specify true or false.\n"
			"; When entering a camera-center mode using \"gifModeToggle\", \"gifModeToggleCameraCenterOnly\" or \"toggleCameraCenterOpponent\",\n"
			"; which center the camera on either you or the opponent, the camera is angled downwards slightly, and this camera angle is called \"pitch\".\n"
			"; It may cause lines of hitboxes to not be displayed entirely parallel to the sides of the screen.\n"
			"; By setting this to true you can force the camera to look straight forward.");
	registerOtherDescription(settingAndItsName(cameraCenterOffsetX), "Camera Centering - X Offset", settingsHitboxSettingsStr,
			"; Specify a floating point value where '.' is the delimiter, like so: 0.0\n"
			"; When entering a camera-center mode using \"gifModeToggle\", \"gifModeToggleCameraCenterOnly\" or \"toggleCameraCenterOpponent\",\n"
			"; which center the camera on either you or the opponent, this value will control how offset the camera is horizontally, relative\n"
			"; to the player. A positive value offsets left, and a negative offsets right. Default value is 0.0.");
	registerOtherDescription(settingAndItsName(cameraCenterOffsetY), "Camera Centering - Y Offset (When Pitch Is Not Forced To 0)", settingsHitboxSettingsStr,
			"; Specify a floating point value where '.' is the delimiter, like so: 106.4231\n"
			"; When entering a camera-center mode using \"gifModeToggle\", \"gifModeToggleCameraCenterOnly\" or \"toggleCameraCenterOpponent\",\n"
			"; which center the camera on either you or the opponent, this value will control how offset the camera is vertically, relative\n"
			"; to the player. A positive value offsets down, and a negative offsets up. Default value is 106.4231.");
	registerOtherDescription(settingAndItsName(cameraCenterOffsetY_WhenForcePitch0), "Camera Centering - Y Offset (When Pitch Is Forced To 0)", settingsHitboxSettingsStr,
			"; Specify a floating point value where '.' is the delimiter, like so: 130.4231\n"
			"; When entering a camera-center mode using \"gifModeToggle\", \"gifModeToggleCameraCenterOnly\" or \"toggleCameraCenterOpponent\",\n"
			"; which center the camera on either you or the opponent, AND \"forceZeroPitchDuringCameraCentering\" is set to true,\n"
			"; this value will control how offset the camera is vertically, relative\n"
			"; to the player. A positive value offsets down, and a negative offsets up. Default value is 130.4231.");
	registerOtherDescription(settingAndItsName(cameraCenterOffsetZ), "Camera Centering - Z Offset", settingsHitboxSettingsStr,
			"; Specify a floating point value where '.' is the delimiter, like so: 540.0\n"
			"; When entering a camera-center mode using \"gifModeToggle\", \"gifModeToggleCameraCenterOnly\" or \"toggleCameraCenterOpponent\",\n"
			"; which center the camera on either you or the opponent,\n"
			"; this value will control how far the camera is. The bigger the value, the further the camera will be. Default value is 540.0.");
	registerOtherDescription(settingAndItsName(modWindowVisibleOnStart), "Mod Window Visible On Start", settingsGeneralSettingsStr,
			"; Specify true or false.\n"
			"; If this is false, when this mod starts, the mod's UI window will be invisible.");
	registerOtherDescription(settingAndItsName(closingModWindowAlsoHidesFramebar), "Closing Mod's Window Also Hides Framebar", settingsFramebarSettingsStr,
			"; Specify true or false.\n"
			"; If this is true, then closing the main mod's window, either using a hotkey or the cross mark,\n"
			"; will also hide the framebar, while opening the main mod's window will show the framebar.\n"
			"; Alternatively you could set up a separate hotkey to control visibility of the framebar, using \"framebarVisibilityToggle\".\n"
			"; Note that even when the UI is open, \"showFramebar\" must be set to true for the framebar to be visible.");
	registerOtherDescription(settingAndItsName(dontShowMoveName), "Don't Show Move's Name", settingsGeneralSettingsStr,
			"; Specify true or false.\n"
			"; In the main UI window, there's a field called 'Move' which displays the last performed move or several moves\n"
			"; that the mod decided to combine together. That text can get pretty long. If you set this setting to true,\n"
			"; then that field will be hidden, and you will only be able to see moves' names either in 'Cancels (P1/P2)' window,\n"
			"; or by hovering your mouse over the 'Startup' or 'Total' fields in the main UI window and reading their tooltip.");
	#undef settingAndItsName
	
	pointerIntoSettingsIntoDescription.resize(offsetof(Settings, settingsMembersEnd) - offsetof(Settings, settingsMembersStart));
	for (OtherDescription& desc : otherDescriptions) {
		pointerIntoSettingsIntoDescription[(BYTE*)desc.ptr - (BYTE*)this - offsetof(Settings, settingsMembersStart)] = &desc;
		settingNameToOffset[desc.iniNameAllCaps] = (BYTE*)desc.ptr - (BYTE*)this;
		iniNameToUiNameMap.insert(
			{
				desc.iniNameAllCaps.c_str(),
				{ desc.uiFullPath.c_str(), desc.uiName }
			});
	}
	for (auto it = keyCombosToParse.begin(); it != keyCombosToParse.end(); ++it) {
		std::string& strRef = it->second.uiFullName;
		strRef.reserve(33 + strlen(it->second.uiName) + 1);
		strRef = "'Settings - Keyboard Shortcuts - ";
		strRef += it->second.uiName;
		strRef += '\'';
		iniNameToUiNameMap.insert(
			{
				it->first.c_str(),
				{ strRef.c_str(), it->second.uiName }
			});
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


	bool slowmoTimesParsed = false;
	
	bool framebarHeightParsed = false;

	bool framebarTitleCharsMaxParsed = false;

	bool cameraCenterOffsetXParsed = false;
	bool cameraCenterOffsetYParsed = false;
	bool cameraCenterOffsetY_WhenForcePitch0Parsed = false;
	bool cameraCenterOffsetZParsed = false;
	
	bool startDisabledParsed = false;

	screenshotPath.clear();  // we can clear this in advance because it is protected by screenshotPathMutex
	// keyboard shortcuts are protected by keyCombosMutex
	// the other settings are exposed bare
	bool screenshotPathParsed = false;

	bool allowContinuousScreenshottingParsed = false;
	
	bool oldTurnOffPostEffectWhenMakingBackgroundBlack = turnOffPostEffectWhenMakingBackgroundBlack;
	bool turnOffPostEffectWhenMakingBackgroundBlackParsed = false;
	
	bool displayUIOnTopOfPauseMenuParsed = false;
	
	bool dodgeObsRecordingParsed = false;
	
	bool neverIgnoreHitstopParsed = false;
	
	bool ignoreHitstopForBlockingBaikenParsed = false;
	
	bool considerRunAndWalkNonIdleParsed = false;
	
	bool considerCrouchNonIdleParsed = false;
	
	bool considerDummyPlaybackNonIdleParsed = false;
	
	bool considerKnockdownWakeupAndAirtechIdleParsed = false;
	
	bool considerIdleInvulIdleParsed = false;
	
	bool useSimplePixelBlenderParsed = false;
	
	bool dontShowBoxesParsed = false;
	
	bool neverDisplayGrayHurtboxesParsed = false;
	
	bool showFramebarParsed = false;
	
	bool showFramebarInTrainingModeParsed = false;
	
	bool showFramebarInReplayModeParsed = false;
	
	bool showFramebarInOtherModesParsed = false;
	
	bool showStrikeInvulOnFramebarParsed = false;
	
	bool showSuperArmorOnFramebarParsed = false;
	
	bool showThrowInvulOnFramebarParsed = false;
	
	bool showOTGOnFramebarParsed = false;
	
	bool showFirstFramesOnFramebarParsed = false;
	
	bool considerSimilarFrameTypesSameForFrameCountsParsed = false;
	
	bool considerSimilarIdleFramesSameForFrameCountsParsed = false;
	
	bool combineProjectileFramebarsWhenPossibleParsed = false;
	
	bool eachProjectileOnSeparateFramebarParsed = false;
	
	bool dontClearFramebarOnStageResetParsed = false;
	
	bool dontTruncateFramebarTitlesParsed = false;
	
	bool useSlangNamesParsed = false;
	
	bool allFramebarTitlesDisplayToTheLeftParsed = false;
	
	bool showPlayerInFramebarTitleParsed = false;
	
	bool useColorblindHelpParsed = false;
	
	bool dontUseScreenshotTransparencyParsed = false;
	
	bool drawPushboxCheckSeparatelyParsed = false;
	
	bool frameAdvantage_dontUsePreBlockstunTimeParsed = false;
	
	bool skipGrabsInFramebarParsed = false;
	
	bool showFramebarHatchedLineWhenSkippingGrabParsed = false;
	
	bool showFramebarHatchedLineWhenSkippingHitstopParsed = false;
	
	bool showFramebarHatchedLineWhenSkippingSuperfreezeParsed = false;
	
	bool showComboProrationInRiscGaugeParsed = false;
	
	bool displayInputHistoryWhenObservingParsed = false;
	
	bool displayInputHistoryInSomeOfflineModesParsed = false;
	
	bool showDurationsInInputHistoryParsed = false;
	
	bool useAlternativeStaggerMashProgressDisplayParsed = false;
	
	bool dontShowMayInteractionChecksParsed = false;
	
	bool showMilliaBadMoonBuffHeightParsed = false;
	
	bool showFaustOwnFlickRangesParsed = false;
	
	bool showBedmanTaskCHeightBuffYParsed = false;
	
	bool showJackoGhostPickupRangeParsed = false;
	
	bool showJackoSummonsPushboxesParsed = false;
	
	bool showJackoAegisFieldRangeParsed = false;
	
	bool showJackoServantAttackRangeParsed = false;
	
	bool ignoreScreenshotPathAndSaveToClipboardParsed = false;
	
	bool forceZeroPitchDuringCameraCenteringParsed = false;
	
	bool modWindowVisibleOnStartParsed = false;
	
	bool closingModWindowAlsoHidesFramebarParsed = false;

	bool dontShowMoveNameParsed = false;

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
			} else {
				auto foundOffsetIt = settingNameToOffset.find(keyNameUpper);
				DWORD foundOffset = 0xffffffff;
				if (foundOffsetIt != settingNameToOffset.end()) {
					foundOffset = foundOffsetIt->second;
					switch (foundOffset) {
						#define booleanPreset(name) \
							case offsetof(Settings, name): \
								if (!name##Parsed) { \
									name##Parsed = parseBoolean(#name, keyValue, name); \
								} \
								break;
							
						case offsetof(Settings, slowmoTimes):
							if (!slowmoTimesParsed) {
								slowmoTimesParsed = parseInteger("slowmoTimes", keyValue, slowmoTimes);
							}
							break;
						case offsetof(Settings, framebarHeight):
							if (!framebarHeightParsed) {
								framebarHeightParsed = parseInteger("framebarHeight", keyValue, framebarHeight);
							}
							break;
						case offsetof(Settings, framebarTitleCharsMax):
							if (!framebarTitleCharsMaxParsed) {
								framebarTitleCharsMaxParsed = parseInteger("framebarTitleCharsMax", keyValue, framebarTitleCharsMax);
							}
							break;
						case offsetof(Settings, cameraCenterOffsetX):
							if (!cameraCenterOffsetXParsed) {
								cameraCenterOffsetXParsed = parseFloat("cameraCenterOffsetX", keyValue, cameraCenterOffsetX);
							}
							break;
						case offsetof(Settings, cameraCenterOffsetY):
							if (!cameraCenterOffsetYParsed) {
								cameraCenterOffsetYParsed = parseFloat("cameraCenterOffsetY", keyValue, cameraCenterOffsetY);
							}
							break;
						case offsetof(Settings, cameraCenterOffsetY_WhenForcePitch0):
							if (!cameraCenterOffsetY_WhenForcePitch0Parsed) {
								cameraCenterOffsetY_WhenForcePitch0Parsed = parseFloat("cameraCenterOffsetY_WhenForcePitch0", keyValue, cameraCenterOffsetY_WhenForcePitch0);
							}
							break;
						case offsetof(Settings, cameraCenterOffsetZ):
							if (!cameraCenterOffsetZParsed) {
								cameraCenterOffsetZParsed = parseFloat("cameraCenterOffsetZ", keyValue, cameraCenterOffsetZ);
							}
							break;
						booleanPreset(displayUIOnTopOfPauseMenu)
						booleanPreset(dodgeObsRecording)
						booleanPreset(neverIgnoreHitstop)
						booleanPreset(ignoreHitstopForBlockingBaiken)
						booleanPreset(considerRunAndWalkNonIdle)
						booleanPreset(considerCrouchNonIdle)
						booleanPreset(considerDummyPlaybackNonIdle)
						booleanPreset(considerKnockdownWakeupAndAirtechIdle)
						booleanPreset(considerIdleInvulIdle)
						booleanPreset(useSimplePixelBlender)
						booleanPreset(dontShowBoxes)
						booleanPreset(neverDisplayGrayHurtboxes)
						booleanPreset(showFramebar)
						booleanPreset(showFramebarInTrainingMode)
						booleanPreset(showFramebarInReplayMode)
						booleanPreset(showFramebarInOtherModes)
						booleanPreset(showStrikeInvulOnFramebar)
						booleanPreset(showSuperArmorOnFramebar)
						booleanPreset(showThrowInvulOnFramebar)
						booleanPreset(showOTGOnFramebar)
						booleanPreset(showFirstFramesOnFramebar)
						booleanPreset(considerSimilarFrameTypesSameForFrameCounts)
						booleanPreset(considerSimilarIdleFramesSameForFrameCounts)
						booleanPreset(combineProjectileFramebarsWhenPossible)
						booleanPreset(eachProjectileOnSeparateFramebar)
						booleanPreset(dontClearFramebarOnStageReset)
						booleanPreset(dontTruncateFramebarTitles)
						booleanPreset(useSlangNames)
						booleanPreset(allFramebarTitlesDisplayToTheLeft)
						booleanPreset(showPlayerInFramebarTitle)
						booleanPreset(useColorblindHelp)
						booleanPreset(allowContinuousScreenshotting)
						booleanPreset(turnOffPostEffectWhenMakingBackgroundBlack)
						booleanPreset(startDisabled)
						case offsetof(Settings, screenshotPath):
							if (!screenshotPathParsed) {
								screenshotPathParsed = true;
								screenshotPath = keyValue;  // in UTF-8
								logwrap(fprintf(logfile, "Parsed screenshotPath (UTF8): %s\n", keyValue.c_str()));
							}
							break;
						booleanPreset(dontUseScreenshotTransparency)
						booleanPreset(drawPushboxCheckSeparately)
						booleanPreset(frameAdvantage_dontUsePreBlockstunTime)
						booleanPreset(skipGrabsInFramebar)
						booleanPreset(showFramebarHatchedLineWhenSkippingGrab)
						booleanPreset(showFramebarHatchedLineWhenSkippingHitstop)
						booleanPreset(showFramebarHatchedLineWhenSkippingSuperfreeze)
						booleanPreset(showComboProrationInRiscGauge)
						booleanPreset(displayInputHistoryWhenObserving)
						booleanPreset(displayInputHistoryInSomeOfflineModes)
						booleanPreset(showDurationsInInputHistory)
						booleanPreset(useAlternativeStaggerMashProgressDisplay)
						booleanPreset(dontShowMayInteractionChecks)
						booleanPreset(showMilliaBadMoonBuffHeight)
						booleanPreset(showFaustOwnFlickRanges)
						booleanPreset(showBedmanTaskCHeightBuffY)
						booleanPreset(showJackoGhostPickupRange)
						booleanPreset(showJackoSummonsPushboxes)
						booleanPreset(showJackoAegisFieldRange)
						booleanPreset(showJackoServantAttackRange)
						booleanPreset(ignoreScreenshotPathAndSaveToClipboard)
						booleanPreset(forceZeroPitchDuringCameraCentering)
						booleanPreset(modWindowVisibleOnStart)
						booleanPreset(closingModWindowAlsoHidesFramebar)
						booleanPreset(dontShowMoveName)
						#undef booleanPreset
					}
				}
			}
			if (feof(file)) break;
		}
		fclose(file);
	}
	
	if (!slowmoTimesParsed) {
		slowmoTimes = 3;
	}
	
	if (!framebarHeightParsed) {
		framebarHeight = 19;
	}
	
	if (!framebarTitleCharsMaxParsed) {
		framebarTitleCharsMax = 12;
	}
	
	if (!startDisabledParsed) {
		startDisabled = false;
	}
	
	if (!allowContinuousScreenshottingParsed) {
		allowContinuousScreenshotting = false;
	}
	
	if (!cameraCenterOffsetXParsed) {
		cameraCenterOffsetX = cameraCenterOffsetX_defaultValue;
	}
	
	if (!cameraCenterOffsetYParsed) {
		cameraCenterOffsetY = cameraCenterOffsetY_defaultValue;
	}
	
	if (!cameraCenterOffsetY_WhenForcePitch0Parsed) {
		cameraCenterOffsetY_WhenForcePitch0 = cameraCenterOffsetY_WhenForcePitch0_defaultValue;
	}
	
	if (!cameraCenterOffsetZParsed) {
		cameraCenterOffsetZ = cameraCenterOffsetZ_defaultValue;
	}
	
	if (!turnOffPostEffectWhenMakingBackgroundBlackParsed) {
		turnOffPostEffectWhenMakingBackgroundBlack = true;
	}
	
	if (!displayUIOnTopOfPauseMenuParsed) {
		displayUIOnTopOfPauseMenu = true;
	}
	
	if (!dodgeObsRecordingParsed) {
		dodgeObsRecording = true;
	}
	
	if (!neverIgnoreHitstopParsed) {
		neverIgnoreHitstop = false;
	}
	
	if (!ignoreHitstopForBlockingBaikenParsed) {
		ignoreHitstopForBlockingBaiken = false;
	}
	
	if (!considerRunAndWalkNonIdleParsed) {
		considerRunAndWalkNonIdle = false;
	}
	
	if (!considerCrouchNonIdleParsed) {
		considerCrouchNonIdle = false;
	}
	
	if (!considerDummyPlaybackNonIdleParsed) {
		considerDummyPlaybackNonIdle = false;
	}
	
	if (!useSimplePixelBlenderParsed) {
		useSimplePixelBlender = false;
	}
	
	if (!dontShowBoxesParsed) {
		dontShowBoxes = false;
	}
	
	if (!neverDisplayGrayHurtboxesParsed) {
		neverDisplayGrayHurtboxes = false;
	}
	
	if (!showFramebarParsed) {
		showFramebar = true;
	}
	
	if (!showFramebarInTrainingModeParsed) {
		showFramebarInTrainingMode = true;
	}
	
	if (!showFramebarInReplayModeParsed) {
		showFramebarInReplayMode = true;
	}
	
	if (!showFramebarInOtherModesParsed) {
		showFramebarInOtherModes = false;
	}
	
	if (!showStrikeInvulOnFramebarParsed) {
		showStrikeInvulOnFramebar = true;
	}
	
	if (!showSuperArmorOnFramebarParsed) {
		showSuperArmorOnFramebar = true;
	}
	
	if (!showThrowInvulOnFramebarParsed) {
		showThrowInvulOnFramebar = true;
	}
	
	if (!showOTGOnFramebarParsed) {
		showOTGOnFramebar = true;
	}
	
	if (!showFirstFramesOnFramebarParsed) {
		showFirstFramesOnFramebar = true;
	}
	
	if (!considerSimilarFrameTypesSameForFrameCountsParsed) {
		considerSimilarFrameTypesSameForFrameCounts = true;
	}
	
	if (!considerSimilarIdleFramesSameForFrameCountsParsed) {
		considerSimilarIdleFramesSameForFrameCounts = false;
	}
	
	if (!combineProjectileFramebarsWhenPossibleParsed) {
		combineProjectileFramebarsWhenPossible = true;
	}
	
	if (!eachProjectileOnSeparateFramebarParsed) {
		eachProjectileOnSeparateFramebar = false;
	}
	
	if (!dontClearFramebarOnStageResetParsed) {
		dontClearFramebarOnStageReset = false;
	}
	
	if (!dontTruncateFramebarTitlesParsed) {
		dontTruncateFramebarTitles = false;
	}
	
	if (!useSlangNamesParsed) {
		useSlangNames = false;
	}
	
	if (!allFramebarTitlesDisplayToTheLeftParsed) {
		allFramebarTitlesDisplayToTheLeft = true;
	}
	
	if (!showPlayerInFramebarTitleParsed) {
		showPlayerInFramebarTitle = true;
	}
	
	if (!useColorblindHelpParsed) {
		useColorblindHelp = false;
	}
	
	if (!considerKnockdownWakeupAndAirtechIdleParsed) {
		considerKnockdownWakeupAndAirtechIdle = false;
	}
	
	if (!considerIdleInvulIdleParsed) {
		considerIdleInvulIdle = false;
	}
	
	if (!dontUseScreenshotTransparencyParsed) {
		dontUseScreenshotTransparency = false;
	}
	
	if (!drawPushboxCheckSeparatelyParsed) {
		drawPushboxCheckSeparately = true;
	}
	
	if (!frameAdvantage_dontUsePreBlockstunTimeParsed) {
		frameAdvantage_dontUsePreBlockstunTime = false;
	}
	
	if (!skipGrabsInFramebarParsed) {
		skipGrabsInFramebar = true;
	}
	
	if (!showFramebarHatchedLineWhenSkippingGrabParsed) {
		showFramebarHatchedLineWhenSkippingGrab = true;
	}
	
	if (!showFramebarHatchedLineWhenSkippingHitstopParsed) {
		showFramebarHatchedLineWhenSkippingHitstop = false;
	}
	
	if (!showFramebarHatchedLineWhenSkippingSuperfreezeParsed) {
		showFramebarHatchedLineWhenSkippingSuperfreeze = true;
	}
	
	if (!showComboProrationInRiscGaugeParsed) {
		showComboProrationInRiscGauge = false;
	}
	
	if (!displayInputHistoryWhenObservingParsed) {
		displayInputHistoryWhenObserving = true;
	}
	
	if (!displayInputHistoryInSomeOfflineModesParsed) {
		displayInputHistoryInSomeOfflineModes = false;
	}
	
	if (!showDurationsInInputHistoryParsed) {
		showDurationsInInputHistory = false;
	}
	
	if (!useAlternativeStaggerMashProgressDisplayParsed) {
		useAlternativeStaggerMashProgressDisplay = false;
	}
	
	if (!dontShowMayInteractionChecksParsed) {
		dontShowMayInteractionChecks = true;
	}
	
	if (!showMilliaBadMoonBuffHeightParsed) {
		showMilliaBadMoonBuffHeight = false;
	}
	
	if (!showFaustOwnFlickRangesParsed) {
		showFaustOwnFlickRanges = true;
	}
	
	if (!showBedmanTaskCHeightBuffYParsed) {
		showBedmanTaskCHeightBuffY = false;
	}
	
	if (!showJackoGhostPickupRangeParsed) {
		showJackoGhostPickupRange = false;
	}
	
	if (!showJackoSummonsPushboxesParsed) {
		showJackoSummonsPushboxes = false;
	}
	
	if (!showJackoAegisFieldRangeParsed) {
		showJackoAegisFieldRange = false;
	}
	
	if (!showJackoServantAttackRangeParsed) {
		showJackoServantAttackRange = false;
	}
	
	if (!ignoreScreenshotPathAndSaveToClipboardParsed) {
		ignoreScreenshotPathAndSaveToClipboard = false;
	}
	
	if (!forceZeroPitchDuringCameraCenteringParsed) {
		forceZeroPitchDuringCameraCentering = true;
	}
	
	if (!modWindowVisibleOnStartParsed) {
		modWindowVisibleOnStart = true;
	}
	
	if (!closingModWindowAlsoHidesFramebarParsed) {
		closingModWindowAlsoHidesFramebar = true;
	}
	
	if (!dontShowMoveNameParsed) {
		dontShowMoveName = false;
	}
	
	if (!dontShowMoveNameParsed) {
		dontShowMoveName = false;
	}
	
	if (firstSettingsParse) {
		ui.visible = modWindowVisibleOnStart;
		if (startDisabled) {
			gifMode.modDisabled = true;
		}
	} else if (turnOffPostEffectWhenMakingBackgroundBlack != oldTurnOffPostEffectWhenMakingBackgroundBlack && keyboard.thisProcessWindow) {
		PostMessageW(keyboard.thisProcessWindow, WM_APP_TURN_OFF_POST_EFFECT_SETTING_CHANGED, FALSE, TRUE);
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

bool Settings::parseFloat(const char* keyName, const std::string& keyValue, float& floatValue) {
	bool isError;
	floatValue = parseFloat(keyValue.c_str(), &isError);
	if (isError) {
		return false;
	}
	logwrap(fprintf(logfile, "Parsed float for %s: %d\n", keyName, floatValue));
	return true;
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
		bool needReform = false;  // we gettin political ove here
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
	
	if (lines.empty()) {
		lines.emplace_back();
		lines.back().line = "; Place this file into the game folder containing 'GuiltyGearXrd.exe' so that it gets seen by the mod."
			" Allowed key names: Backspace, Tab, Enter, PauseBreak, CapsLock, Escape, Space, PageUp, PageDown, End, Home, Left, Up,"
			" Right, Down, PrintScreen, Insert, Delete, Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9, NumMultiply,"
			" NumAdd, NumSubtract, NumDecimal, NumDivide, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, NumLock, ScrollLock,"
			" Colon, Plus, Minus, Comma, Period, Slash, Tilde, OpenSquareBracket, Backslash, CloseSquareBracket, Quote, Backslash2,"
			" 0123456789, ABCDEFGHIJKLMNOPQRSTUVWXYZ, Shift, Ctrl, Alt.";
		lines.emplace_back();
		lines.emplace_back();
		lines.back().line = "; Key combinations can be specified by separating key names with '+' sign.";
		lines.emplace_back();
		lines.back().line = "; You can assign same key to multiple features - it will toggle/set in motion all of them simultaneously.";
		lines.emplace_back();
		lines.back().line = "; You don't need to reload the mod when you change this file - it re-reads this settings file automatically when it changes.";
		lines.emplace_back();
		lines.emplace_back();
		lines.back().line = "; All of these settings can be changed using the mod's UI which can be seen in the game if you press ESC (by default, see modWindowVisibilityToggle).";
	}
	
	#define booleanPreset(name) replaceOrAddSetting(#name, formatBoolean(name), getOtherINIDescription(&name));
	replaceOrAddSetting("slowmoTimes", formatInteger(slowmoTimes).c_str(), getOtherINIDescription(&slowmoTimes));
	booleanPreset(allowContinuousScreenshotting)
	booleanPreset(startDisabled)
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
	booleanPreset(ignoreScreenshotPathAndSaveToClipboard)
	
	booleanPreset(dontUseScreenshotTransparency)
	booleanPreset(turnOffPostEffectWhenMakingBackgroundBlack)
	booleanPreset(drawPushboxCheckSeparately)
	booleanPreset(frameAdvantage_dontUsePreBlockstunTime)
	booleanPreset(skipGrabsInFramebar)
	booleanPreset(showFramebarHatchedLineWhenSkippingGrab)
	booleanPreset(showFramebarHatchedLineWhenSkippingHitstop)
	booleanPreset(showFramebarHatchedLineWhenSkippingSuperfreeze)
	booleanPreset(showComboProrationInRiscGauge)
	booleanPreset(displayInputHistoryWhenObserving)
	booleanPreset(displayInputHistoryInSomeOfflineModes)
	booleanPreset(showDurationsInInputHistory)
	booleanPreset(useAlternativeStaggerMashProgressDisplay)
	booleanPreset(dontShowMayInteractionChecks)
	booleanPreset(showMilliaBadMoonBuffHeight)
	booleanPreset(showFaustOwnFlickRanges)
	booleanPreset(showBedmanTaskCHeightBuffY)
	booleanPreset(showJackoGhostPickupRange)
	booleanPreset(showJackoSummonsPushboxes)
	booleanPreset(showJackoAegisFieldRange)
	booleanPreset(showJackoServantAttackRange)
	booleanPreset(forceZeroPitchDuringCameraCentering)
	booleanPreset(useSimplePixelBlender)
	booleanPreset(modWindowVisibleOnStart)
	booleanPreset(closingModWindowAlsoHidesFramebar)
	booleanPreset(dontShowMoveName)
	booleanPreset(neverDisplayGrayHurtboxes)
	booleanPreset(dontShowBoxes)
	booleanPreset(displayUIOnTopOfPauseMenu)
	booleanPreset(dodgeObsRecording)
	booleanPreset(showFramebar)
	booleanPreset(showFramebarInTrainingMode)
	booleanPreset(showFramebarInReplayMode)
	booleanPreset(showFramebarInOtherModes)
	booleanPreset(showStrikeInvulOnFramebar)
	booleanPreset(showSuperArmorOnFramebar)
	booleanPreset(showThrowInvulOnFramebar)
	booleanPreset(showOTGOnFramebar)
	booleanPreset(showFirstFramesOnFramebar)
	booleanPreset(considerSimilarFrameTypesSameForFrameCounts)
	booleanPreset(considerSimilarIdleFramesSameForFrameCounts)
	booleanPreset(combineProjectileFramebarsWhenPossible)
	booleanPreset(eachProjectileOnSeparateFramebar)
	booleanPreset(dontClearFramebarOnStageReset)
	booleanPreset(dontTruncateFramebarTitles)
	booleanPreset(useSlangNames)
	booleanPreset(allFramebarTitlesDisplayToTheLeft)
	booleanPreset(showPlayerInFramebarTitle)
	replaceOrAddSetting("framebarHeight", formatInteger(framebarHeight).c_str(), getOtherINIDescription(&framebarHeight));
	replaceOrAddSetting("framebarTitleCharsMax", formatInteger(framebarTitleCharsMax).c_str(), getOtherINIDescription(&framebarTitleCharsMax));
	booleanPreset(neverIgnoreHitstop)
	booleanPreset(ignoreHitstopForBlockingBaiken)
	booleanPreset(considerRunAndWalkNonIdle)
	booleanPreset(considerCrouchNonIdle)
	booleanPreset(considerDummyPlaybackNonIdle)
	booleanPreset(considerKnockdownWakeupAndAirtechIdle)
	booleanPreset(considerIdleInvulIdle)
	booleanPreset(useColorblindHelp)
	replaceOrAddSetting("cameraCenterOffsetX", formatFloat(cameraCenterOffsetX).c_str(), getOtherINIDescription(&cameraCenterOffsetX));
	replaceOrAddSetting("cameraCenterOffsetY", formatFloat(cameraCenterOffsetY).c_str(), getOtherINIDescription(&cameraCenterOffsetY));
	replaceOrAddSetting("cameraCenterOffsetY_WhenForcePitch0", formatFloat(cameraCenterOffsetY_WhenForcePitch0).c_str(), getOtherINIDescription(&cameraCenterOffsetY_WhenForcePitch0));
	replaceOrAddSetting("cameraCenterOffsetZ", formatFloat(cameraCenterOffsetZ).c_str(), getOtherINIDescription(&cameraCenterOffsetZ));
	#undef booleanPreset
	
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
			info->uiNameWithLength = { info->uiName, strlen(info->uiName) };
			info->uiDescription = combo.uiDescription.c_str();
			info->uiDescriptionWithLength = { info->uiDescription, combo.uiDescription.size() };
			return;
		}
	}
	info->uiName = info->uiDescription = "";
}

const char* Settings::getOtherUIName(void* ptr) {
	return pointerIntoSettingsIntoDescription[(BYTE*)ptr - (BYTE*)this - offsetof(Settings, settingsMembersStart)]->uiName;
}

StringWithLength Settings::getOtherUINameWithLength(void* ptr) {
	return pointerIntoSettingsIntoDescription[(BYTE*)ptr - (BYTE*)this - offsetof(Settings, settingsMembersStart)]->uiNameWithLength;
}

const char* Settings::getOtherUIFullName(void* ptr) {
	return pointerIntoSettingsIntoDescription[(BYTE*)ptr - (BYTE*)this - offsetof(Settings, settingsMembersStart)]->uiFullPath.c_str();
}

const char* Settings::getOtherUIDescription(void* ptr) {
	return pointerIntoSettingsIntoDescription[(BYTE*)ptr - (BYTE*)this - offsetof(Settings, settingsMembersStart)]->uiDescription.c_str();
}

StringWithLength Settings::getOtherUIDescriptionWithLength(void* ptr) {
	const std::string& str = pointerIntoSettingsIntoDescription[(BYTE*)ptr - (BYTE*)this - offsetof(Settings, settingsMembersStart)]->uiDescription;
	return { str.c_str(), str.size() };
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
	desc.uiNameWithLength = { uiName, strlen(uiName) };
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

static const char* convertToUiDescription_lookaheadPart(const char* mainPtr) {
	const char* ptr = mainPtr + 1;
	while (*ptr == '\n' && *ptr != '\0') {
		++ptr;
	}
	if (*ptr != ';') return nullptr;
	if (*++ptr != ' ') return nullptr;
	char c = *++ptr;
	if (!(c >= 'a' && c <= 'z')) return nullptr;
	return ptr - 1;
}

std::string Settings::convertToUiDescription(const char* iniDescription) {
	std::string result;
	if (*iniDescription != '\0') {
		
		const char* const iniDescriptionInitialValue = iniDescription;
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
				}
				parsingMode = PARSING_WHITESPACE;
			} else if (cVal == '\n') {
				if (parsingMode == PARSING_QUOTATION) {
					result.append(quotationStart, iniDescription - quotationStart);
				}
				parsingMode = PARSING_NEWLINE;
				if (iniDescription != iniDescriptionInitialValue) {
					char p = *(iniDescription - 1);
					if (p >= 'a' && p <= 'z'
							|| p == ','
							|| p >= '0' && p <= '9') {
						const char* reply = convertToUiDescription_lookaheadPart(iniDescription);
						if (reply) {
							iniDescription = reply;
							cVal = *reply;
							parsingMode = PARSING_WHITESPACE;
						}
					}
				}
			} else if (cVal <= 32) {
				if (parsingMode == PARSING_SEMICOLON) {
					parsingMode = PARSING_WHITESPACE;
					continue;
				} else if (parsingMode == PARSING_QUOTATION) {
					result.append(quotationStart, iniDescription - quotationStart);
				}
				parsingMode = PARSING_WHITESPACE;
			} else if (cVal == '"') {
				if (parsingMode == PARSING_WHITESPACE || parsingMode == PARSING_NEWLINE) {
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
						if (std::find(mentionedFullPaths.begin(), mentionedFullPaths.end(), found->first) != mentionedFullPaths.end()) {
							result += '\'';
							result += found->second.name;
							result += '\'';
						} else {
							result += found->second.fullName;
							mentionedFullPaths.push_back(found->first);
						}
					}
					parsingMode = PARSING_WHITESPACE;
					continue;
				}
				parsingMode = PARSING_WHITESPACE;
			} else if (parsingMode == PARSING_QUOTATION) {
				continue;
			} else {
				parsingMode = PARSING_WHITESPACE;
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

float Settings::parseFloat(const char* inputString, bool* error) {
	if (error) *error = false;
	float result;
	int returnValue = sscanf(inputString, "%f", &result);
	if (returnValue == EOF || returnValue == 0 || returnValue == -1) {
		if (error) *error = true;
		return 0.F;
	}
	return result;
}

std::string Settings::formatInteger(int d) {
	static const char* formatString = "%d";
	
	std::string result(
		snprintf(nullptr, 0, formatString, d),
		'\0');
	
	if (result.empty()) {
		return result;
	}
	
	snprintf(&result.front(), result.size() + 1, formatString, d);
	return result;
}

std::string Settings::formatFloat(float f) {
	static const char* formatString = "%.4f";
	
	std::string result(
		snprintf(nullptr, 0, formatString, f),
		'\0');
	
	if (result.empty()) {
		return result;
	}
	
	int resultSize = (int)result.size();
	auto resultBegin = result.begin();
	auto firstDotPos = result.end();
	auto firstNonZeroPos = result.end();
	bool foundFirstNonZero = false;
	auto lastCharPos = resultBegin + (resultSize - 1);
	
	snprintf(&result.front(), resultSize + 1, formatString, f);
	
	for (auto it = lastCharPos; ; ) {
		char c = *it;
		
		if (c == '.') {
			firstDotPos = it;
			break;
		}
		if (c != '0' && !foundFirstNonZero) {
			if (it == lastCharPos) break;
			foundFirstNonZero = true;
			firstNonZeroPos = it;
		}
		if (it == resultBegin) break;
		--it;
	}
	if (firstDotPos == result.end()) {
		return result;
	}
	std::string::iterator firstZeroPos;
	if (!foundFirstNonZero) {
		firstZeroPos = firstDotPos + 1;
	} else {
		firstZeroPos = firstNonZeroPos + 1;
	}
	if (firstZeroPos == firstDotPos + 1) {
		if (firstZeroPos == lastCharPos) {
			return result;
		}
		++firstZeroPos;
	}
	result.erase(firstZeroPos, result.end());
	return result;
}
