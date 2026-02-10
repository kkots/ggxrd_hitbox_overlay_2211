// this file gets parsed by regenerate_ini_and_update_readme.ps1. Do not change its structure
#pragma warning(push)
settingsKeyCombo(gifModeToggle, "GIF Mode Toggle", "",
	"; A keyboard shortcut to toggle GIF mode.\n"
	"; GIF mode is:\n"
	"; 1) Background becomes black\n"
	"; 2) Camera is centered on you\n"
	"; 3) Opponent is invisible and invulnerable\n"
	"; 4) Hide HUD (interface)\n"
	"; GIF Mode can also be toggled using 'UI - Hitboxes - GIF Mode'.")
	
settingsKeyCombo(gifModeToggleBackgroundOnly, "GIF Mode Toggle (Background Only)", "",
	"; A keyboard shortcut to only toggle the \"background becomes black\" part of the \"gifModeToggle\".\n"
	"; Empty by default, which means no hotkey is assigned. Assign your desired hotkey manually here.\n"
	"; This option can be combined with the other \"only\" options, by sharing the same key binding for example.\n"
	"; Black background can also be toggled using 'UI - Hitboxes - Black Background'.")
	
settingsKeyCombo(togglePostEffectOnOff, "Toggle Post-Effect On/Off", "",
	"; A keyboard shortcut to toggle the game's Settings - Display Settings - Post-Effect. Changing it this way does not\n"
	"; require the current match to be restarted.\n"
	"; Alternatively, you could set the \"turnOffPostEffectWhenMakingBackgroundBlack\" setting in this INI file to true\n"
	"; so that whenever you enter either the GIF mode or the GIF mode (black background only), the Post-Effect is\n"
	"; turned off automatically, and when you leave those modes, it gets turned back on.\n"
	"; In the UI, Post-Effect can be toggled using 'UI - Hitboxes - Post-Effect On' checkbox.\n"
	"; This hotkey is empty by default, which means no hotkey is assigned. Assign your desired hotkey manually here.\n"
	"; This option can be combined with the other hotkey options, by sharing the same key binding for example.")
	
#pragma warning(disable:4003)
settingsField(bool, turnOffPostEffectWhenMakingBackgroundBlack, true,
	"Turn Off Post-Effect When Making Background Black", SETTINGS_HITBOX_SETTINGS,
	"; Specify true or false.\n"
	"; When true, whenever you enter either the GIF mode or the GIF mode (black background only),\n"
	"; the Post-Effect is turned off automatically, and when you leave those modes, it gets turned back on.\n"
	"; An alternative way to turn off Post-Effect without reload a match is using the 'UI - Hitboxes - Post-Effect On' checkbox,\n"
	"; or \"togglePostEffectOnOff\" toggle.")
	
settingsField(bool, forceZeroPitchDuringCameraCentering, true,
	"Force Zero Pitch During Camera Centering", SETTINGS_HITBOX_SETTINGS,
	"; Specify true or false.\n"
	"; When entering a camera-center mode using \"gifModeToggle\", \"gifModeToggleCameraCenterOnly\" or \"toggleCameraCenterOpponent\",\n"
	"; which center the camera on either you or the opponent, the camera is angled downwards slightly, and this camera angle is called \"pitch\".\n"
	"; It may cause lines of hitboxes to not be displayed entirely parallel to the sides of the screen.\n"
	"; By setting this to true you can force the camera to look straight forward.\n"
	"; All original hitbox screenshots on dustloop were taken with a slightly angled pitch, so setting this to true\n"
	"; would produce screenshots different from those of dustloop's.")
	
settingsField(float, cameraCenterOffsetX, 0.F,
	"Camera Centering - X Offset", SETTINGS_HITBOX_SETTINGS,
	"; Specify a floating point value where '.' is the delimiter, like so: 0.0\n"
	"; When entering a camera-center mode using \"gifModeToggle\", \"gifModeToggleCameraCenterOnly\" or \"toggleCameraCenterOpponent\",\n"
	"; which center the camera on either you or the opponent, this value will control how offset the camera is horizontally, relative\n"
	"; to the player. A positive value offsets left, and a negative offsets right. Default value is 0.0.")
	
settingsField(float, cameraCenterOffsetY, 106.4231F,
	"Camera Centering - Y Offset (When Pitch Is Not Forced To 0)", SETTINGS_HITBOX_SETTINGS,
	"; Specify a floating point value where '.' is the delimiter, like so: 106.4231\n"
	"; When entering a camera-center mode using \"gifModeToggle\", \"gifModeToggleCameraCenterOnly\" or \"toggleCameraCenterOpponent\",\n"
	"; which center the camera on either you or the opponent, this value will control how offset the camera is vertically, relative\n"
	"; to the player. A positive value offsets down, and a negative offsets up. Default value is 106.4231.")
	
settingsField(float, cameraCenterOffsetY_WhenForcePitch0, 130.4231F,
	"Camera Centering - Y Offset (When Pitch Is Forced To 0)", SETTINGS_HITBOX_SETTINGS,
	"; Specify a floating point value where '.' is the delimiter, like so: 130.4231\n"
	"; When entering a camera-center mode using \"gifModeToggle\", \"gifModeToggleCameraCenterOnly\" or \"toggleCameraCenterOpponent\",\n"
	"; which center the camera on either you or the opponent, AND \"forceZeroPitchDuringCameraCentering\" is set to true,\n"
	"; this value will control how offset the camera is vertically, relative\n"
	"; to the player. A positive value offsets down, and a negative offsets up. Default value is 130.4231.")
	
settingsField(float, cameraCenterOffsetZ, 540.F,
	"Camera Centering - Z Offset", SETTINGS_HITBOX_SETTINGS,
	"; Specify a floating point value where '.' is the delimiter, like so: 540.0\n"
	"; When entering a camera-center mode using \"gifModeToggle\", \"gifModeToggleCameraCenterOnly\" or \"toggleCameraCenterOpponent\",\n"
	"; which center the camera on either you or the opponent,\n"
	"; this value will control how far the camera is. The bigger the value, the further the camera will be. Default value is 540.0.")
	
settingsKeyCombo(gifModeToggleCameraCenterOnly, "GIF Mode Toggle (Camera Only)", "",
	"; A keyboard shortcut to only toggle the \"Camera is centered on you\" part of the \"gifModeToggle\".\n"
	"; In the UI, this mode can also be toggled using 'UI - Hitboxes - Camera Center on Player' checkbox.\n"
	"; Empty by default, which means no hotkey is assigned. Assign your desired hotkey manually here.\n"
	"; This option can be combined with the other \"only\" options, by sharing the same key binding for example")
	
settingsKeyCombo(toggleCameraCenterOpponent, "Center Camera On The Opponent", "",
	"; A keyboard shortcut to toggle the camera to be centered on the opponent.\n"
	"; In the UI, this mode can also be toggled using 'UI - Hitboxes - Camera Center on Opponent' checkbox.\n"
	"; Empty by default, which means no hotkey is assigned. Assign your desired hotkey manually here.\n"
	"; This option can be combined with GIF Mode options, by sharing the same key binding for example")
	
settingsKeyCombo(gifModeToggleHideOpponentOnly, "GIF Mode Toggle (Hide Opponent Only)", "",
	"; A keyboard shortcut to only toggle the \"Opponent is invisible and invulnerable\" part of the \"gifModeToggle\".\n"
	"; In the UI, this mode can also be toggled using 'UI - Hitboxes - Hide Opponent' checkbox.\n"
	"; Empty by default, which means no hotkey is assigned. Assign your desired hotkey manually here.\n"
	"; This option can be combined with the other \"only\" options, by sharing the same key binding for example")
	
settingsKeyCombo(toggleHidePlayer, "Hide Player", "",
	"; A keyboard shortcut to toggle hiding the player.\n"
	"; In the UI, this mode can also be toggled using 'UI - Hitboxes - Hide Player' checkbox.\n"
	"; Empty by default, which means no hotkey is assigned. Assign your desired hotkey manually here.\n"
	"; This option can be combined with GIF Mode options, by sharing the same key binding for example")
	
settingsKeyCombo(gifModeToggleHudOnly, "GIF Mode Toggle (HUD Only)", "",
	"; A keyboard shortcut to only toggle the \"hide HUD (interface)\" part of the \"gifModeToggle\".\n"
	"; In the UI, this mode can also be toggled using 'UI - Hitboxes - Hide HUD' checkbox.\n"
	"; Empty by default, which means no hotkey is assigned. Assign your desired hotkey manually here.\n"
	"; This option can be combined with the other \"only\" options, by sharing the same key binding for example")
	
settingsKeyCombo(noGravityToggle, "No Gravity Toggle", "",
	"; A keyboard shortcut to toggle No gravity mode\n"
	"; In the UI, this mode can also be toggled using 'UI - Hitboxes - No Gravity' checkbox.\n"
	"; No gravity mode is you can't fall basically")
	
settingsKeyCombo(freezeGameToggle, "Freeze Game Toggle", "",
	"; A keyboard shortcut to freeze the game\n"
	"; In the UI, this mode can also be toggled using 'UI - Hitboxes - Freeze Game' checkbox.")
	
settingsKeyCombo(slowmoGameToggle, "Slow-mo Game Toggle", "",
	"; A keyboard shortcut to play the game in slow motion.\n"
	"; Please specify the FPS of the Slow-mo mode in \"slowmoFps\".\n"
	"; In the UI, this mode can also be toggled using 'UI - Hitboxes - Slow-Mo Mode' checkbox.\n"
	"; Slow-mo mode only works in Training Mode.\n")
	
settingsKeyCombo(allowNextFrameKeyCombo, "Allow Next Frame", "",
	"; A keyboard shortcut. Only works while the game is frozen using \"freezeGameToggle\".\n"
	"; Advances the game forward one frame.\n"
	"; In the UI, this mode can also be toggled using 'UI - Hitboxes - Next Frame' button.")
	
settingsField(float, slowmoFps, 30.F,
	"Slow-Mo FPS", SETTINGS_HITBOX,
	"; A floating point number. Default value is 30.0 (half the normal game's FPS, which is 60).\n"
	"; This works in conjunction with \"slowmoGameToggle\". Only numbers greater than or equal to 1 and lower than or equal to 999 allowed.\n"
	"; Specifies the FPS that the game will run in when Slow-mo mode is active.\n"
	"; Slow-mo mode only works in Training Mode.")
	
settingsKeyCombo(disableModToggle, "Disable Mod Toggle", "F6",
	"; A keyboard shortcut to enable/disable the mod without having to load/unload it")
	
settingsField(bool, startDisabled, false,
	"startDisabled", "INI file",
	"; Specify true or false.\n"
	"; When true, starts the mod in a disabled state: it doesn't draw boxes or affect anything")
	
settingsKeyCombo(disableHitboxDisplayToggle, "Disable Hitbox Display Toggle", "F7",
	"; A keyboard shortcut to enable/disable only the mod hitbox drawing feature:\n"
	"; the GIF mode and no gravity, etc will keep working.\n"
	"; In the UI, this mode can also be toggled using \"dontShowBoxes\" checkbox.")
	
settingsKeyCombo(screenshotBtn, "Take Screenshot", "",
	"; A keyboard shortcut.\n"
	"; Takes a screenshot and saves it at \"screenshotPath\" path\n"
	"; To take screenshots over a transparent background you need to go to the game's\n"
	"; Display Settings and turn off Post-Effect (or use \"togglePostEffectOnOff\" and\n"
	"; \"turnOffPostEffectWhenMakingBackgroundBlack\" settings for this), then use GIF mode (make background dark).\n"
	"; Then screenshots will film character over transparent background.\n"
	"; If the \"dontUseScreenshotTransparency\" setting is true, screenshot will be without\n"
	"; transparency anyway.\n"
	"; In the UI, taking screenshots in possible using 'UI - Hitboxes - Take Screenshot' button.")
	
settingsField(ScreenshotPath, screenshotPath, "",
	"Screenshots Path", SETTINGS_HITBOX_SETTINGS,
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
	"; Only PNG format is supported.",
	";C:\\Users\\yourUser\\Desktop\\test screenshot name.png   don't forget to uncomment (; is a comment)")
	
settingsField(bool, ignoreScreenshotPathAndSaveToClipboard, false,
	"Ignore Screenshot Path And Save To Clipboard", SETTINGS_HITBOX_SETTINGS,
	"; Specify true or false.\n"
	"; When this setting is on, screenshots get saved to clipboard only, even if a screenshot path is specified.")
	
settingsField(bool, allowContinuousScreenshotting, false,
	"Allow Continuous Screenshotting When Button Is Held", SETTINGS_HITBOX_SETTINGS,
	"; Specify true or false.\n"
	"; When this is true that means screenshots are being taken every game loop logical frame as\n"
	"; long as the \"screenshotBtn\" is being held. Game loop logical frame means that if the game is\n"
	"; paused or the actual animations are not playing for whatever reason, screenshot won't be taken.\n"
	"; A new screenshot is only taken when animation frames change on the player characters.\n"
	"; Be cautions not to run out of disk space if you're low. This option doesn't\n"
	"; work if \"screenshotPath\" is empty, it's not allowed to work outside of training mode or when\n"
	"; a match (training session) isn't currently running (for example on character selection screen).")
	
settingsKeyCombo(continuousScreenshotToggle, "Continuous Screenshot Toggle", "",
	"; A keyboard shortcut.\n"
	"; This toggle can be used same way as \"screenshotBtn\" (when it's combined with\n"
	"; \"allowContinuousScreenshotting\" = true), except it's a separate key combination and when you\n"
	"; press it, it toggles the continuous screenshot taking every game logical frame. This\n"
	"; toggle does not require \"allowContinuousScreenshotting\" to be set to true,\n"
	"; or \"screenshotBtn\" to be set to anything.\n"
	"; In the UI, you can toggle this mode using 'UI - Hitboxes - Continuous Screenshotting Mode' checkbox.")
	
settingsField(bool, dontUseScreenshotTransparency, false,
	"Take Screenshots Without Transparency", SETTINGS_HITBOX_SETTINGS,
	"; Specify true or false.\n"
	"; Setting this to true will produce screenshots without transparency")
	
settingsField(bool, drawPushboxCheckSeparately, true,
	"Draw Pushbox Check Separately", SETTINGS_HITBOX_SETTINGS,
	"; Specify true or false.\n"
	"; Setting this to true will make throw boxes show in an opponent-character-independent way:\n"
	"; The part of the throw box that checks for pushboxes proximity will be shown in blue,\n"
	"; while the part of the throw box that checks x or y of the origin point will be shown in purple\n"
	"; Setting this to false will combine both the checks of the throw so that you only see the final box\n"
	"; in blue which only checks the opponent's origin point. Be warned, such a throw box\n"
	"; is affected by the width of the opponent's pushbox. Say, on Potemkin, for example,\n"
	"; all ground throw ranges should be higher.")
	
settingsField(bool, useSimplePixelBlender, false,
	"Use Simple CPU Pixel Blender", SETTINGS_HITBOX_SETTINGS,
	"; Specify true or false.\n"
	"; Setting this to true may increase the performance of transparent screenshotting which may be useful if you're screenshotting every frame.\n"
	"; The produced screenshots won't have such improvements as improving visibility of semi-transparent effects or changing hitbox outlines to\n"
	"; black when drawn over the same color.")
	
settingsField(bool, usePixelShader, true,
	"Use Pixel Shader", SETTINGS_HITBOX_SETTINGS,
	"; Specify true or false.\n"
	"; The pixel shader allows hitbox outlines to be shown on top of background of same color by changing the color of\n"
	"; the outline to black only on those pixels.\n"
	"; This is helpful when drawing red outlines on top of Ramlethal's mirror color or Raven's standard (default)\n"
	"; color orb, which are both red.")
	
settingsField(bool, showIndividualHitboxOutlines, false,
	"Show Individual Hitbox Outlines", SETTINGS_HITBOX_SETTINGS,
	"; Specify true or false.\n"
	"; Hitboxes and hurtboxes are complex shapes made up of individual boxes.\n"
	"; Setting this to true allows you to see the outlines of each box\n"
	"; within the combined overall outline.")
	
settingsKeyCombo(modWindowVisibilityToggle, "Hide Mod's UI Toggle", "Escape",
	"; A keyboard shortcut.\n"
	"; Pressing this shortcut will show/hide the mod's UI windows.")
	
settingsField(bool, modWindowVisibleOnStart, true,
	"Open Mod's Main UI Window On Startup", SETTINGS_GENERAL,
	"; Specify true or false.\n"
	"; If this is false, when this mod starts, the mod's UI window will be invisible.")
	
settingsKeyCombo(toggleDisableGrayHurtboxes, "Disable Gray Hurtboxes", "",
	"; A keyboard shortcut.\n"
	"; Pressing this shortcut will disable/enable the display of residual hurtboxes that appear on hit/block and show\n"
	"; the defender's hurtbox at the moment of impact. These hurtboxes display for only a brief time on impacts but\n"
	"; they can get in the way when trying to do certain stuff such as take screenshots of hurtboxes.\n"
	"; In the UI, you can toggle the display of gray hurtboxes using 'UI - Hitboxes - Disable Gray Hurtboxes' checkbox.")
	
settingsKeyCombo(toggleNeverIgnoreHitstop, "Never Ignore Hitstop", "",
	"; A keyboard shortcut.\n"
	"; Pressing this shortcut will disable/enable the \"neverIgnoreHitstop\" setting which controls whether\n"
	"; the framebar advances during hitstop.")
	
settingsKeyCombo(toggleShowInputHistory, "Disable Show Input History", "",
	"; A keyboard shortcut.\n"
	"; Pressing this shortcut will disable the input history shown via \"displayInputHistoryWhenObserving\"\n"
	"; and \"displayInputHistoryInSomeOfflineModes\". It only works when one of those settings is enabled\n"
	"; and is showing history in corresponding game mode.")
	
settingsKeyCombo(toggleAllowCreateParticles, "Allow Creation Of Particles", "",
	"; A keyboard shortcut.\n"
	"; Pressing this shortcut will disable/enable creation of particle effects such as superfreeze flash.\n"
	"; It will not remove particles that are already created or make particles that have already\n"
	"; not been created appear.")
	
settingsKeyCombo(clearInputHistory, "Clear Input History", "",
	"; A keyboard shortcut.\n"
	"; Pressing this shortcut will clear the input history, for example in training mode,\n"
	"; when input history display is enabled.\n"
	"; Alternatively, you can use the \"clearInputHistoryOnStageReset\" boolean setting to\n"
	"; make the game clear input history when you reset positions in training mode or when\n"
	"; round restarts in any game mode.\n"
	"; Alternatively, you can use the \"clearInputHistoryOnStageResetInTrainingMode\" boolean setting to\n"
	"; make the game clear input history when you reset positions in training mode only.")
	
settingsField(bool, clearInputHistoryOnStageReset, false,
	"Clear Input History On Stage Reset", SETTINGS_GENERAL,
	"; Specify true or false.\n"
	"; Specify true if you want the input history to be cleared when stage is reset in any game mode,\n"
	"; including when positions are reset in training mode.\n"
	"; You can also use a hotkey to clear history manually, specified in \"clearInputHistory\".")
	
settingsField(bool, clearInputHistoryOnStageResetInTrainingMode, false,
	"Clear Input History On Stage Reset In Training Mode", SETTINGS_GENERAL,
	"; Specify true or false.\n"
	"; Specify true if you want the input history to be cleared when positions are reset in\n"
	"; training mode only.\n"
	"; You can also use a hotkey to clear history manually, specified in \"clearInputHistory\".")
	
settingsField(bool, neverDisplayGrayHurtboxes, false,
	"Disable Gray Hurtboxes", SETTINGS_HITBOX,
	"; Specify true or false.\n"
	"; This disables the display of gray hurtboxes (for a toggle see \"toggleDisableGrayHurtboxes\").\n"
	"; Gray hurtboxes are residual hurtboxes that appear on hit/block and show the defender's hurtbox at the moment of impact.\n"
	"; These hurtboxes display for only a brief time on impacts but they can get in the way when trying to do certain stuff such\n"
	"; as take screenshots of hurtboxes on hit/block.")
	
settingsField(bool, dontShowBoxes, false,
	"Don't Show Boxes", SETTINGS_HITBOX,
	"; Specify true or false.\n"
	"; Setting this to true will hide all hurtboxes, hitboxes, pushboxes and other boxes and points.\n"
	"; \"disableHitboxDisplayToggle\" can be used to toggle this setting using a keyboard shortcut.")
	
settingsField(bool, displayUIOnTopOfPauseMenu, true,
	"Display Mod's UI on top of Pause Menu", SETTINGS_GENERAL,
	"; Specify true or false.\n"
	"; Display mod's UI on top of the game's Pause Menu. This setting has no effect when \"dodgeObsRecording\" is true and\n"
	"; an OBS is connected to the game.")
	
settingsField(bool, dodgeObsRecording, false,
	"Dodge OBS Recording", SETTINGS_GENERAL,
	"; Specify true or false.\n"
	"; To have this mod avoid OBS capture set this setting to true and also make sure\n"
	"; that in OBS, in Sources you selected your Source, clicked Cogwheel and unchecked the\n"
	"; 'Capture third-party overlays (such as steam)'.\n"
	"; I am very sorry, but the mod's UI, the framebar and the boxes cannot be drawn under the game's\n"
	"; Pause Menu and game's own UI while using 'Dodge OBS Recording'.")
	
settingsField(bool, showFramebar, true,
	"Show Framebar", SETTINGS_FRAMEBAR,
	"; Specify true or false.\n"
	"; This setting can be changed using the \"framebarVisibilityToggle\" hotkey.\n"
	"; If \"closingModWindowAlsoHidesFramebar\" is true, then setting \"showFramebar\" to true is not enough, as the main\n"
	"; mod's UI window must also be open in order to show the framebar. If the window is not open, and \"showFramebar\" is true,\n"
	"; then the only way to open it and to show the framebar is with the \"modWindowVisibilityToggle\" hotkey.")
	
settingsField(bool, showFramebarInTrainingMode, true,
	"Show Framebar In Training Mode", SETTINGS_FRAMEBAR,
	"; Specify true or false.\n"
	"; If false, the framebar will not be shown when in training mode even when \"showFramebar\" is true and the UI is open.")
	
settingsField(bool, showFramebarInReplayMode, true,
	"Show Framebar In Replay Mode", SETTINGS_FRAMEBAR,
	"; Specify true or false.\n"
	"; If false, the framebar will not be shown when in replay mode even when \"showFramebar\" is true and the UI is open.")
	
settingsField(bool, showFramebarInOtherModes, false,
	"Show Framebar In Other Modes", SETTINGS_FRAMEBAR,
	"; Specify true or false.\n"
	"; If false, the framebar will not be shown when in other modes even when \"showFramebar\" is true and the UI is open.\n"
	"; Note that the framebar will never be displayed in online mode when playing as a non-observer, even if this setting is true.\n"
	"; The reason for that is that it malfunctions and shows incorrect framedata when rollback frames happen.\n"
	"; It is possible to put more work into it to fix that, but it will take as many times more computing resources as\n"
	"; there are rollback frames and may start working slower on slower PCs.")
	
settingsField(bool, closingModWindowAlsoHidesFramebar, true,
	"Closing All Of Mod's Windows Also Hides Framebar", SETTINGS_FRAMEBAR,
	"; Specify true or false.\n"
	"; If this is true, then closing every last window of the mod, either using a hotkey or the cross marks,\n"
	"; will also hide the framebar, while any new windows will show the framebar.\n"
	"; Alternatively you could set up a separate hotkey to control visibility of the framebar, using \"framebarVisibilityToggle\".\n"
	"; Note that even when the UI is open, \"showFramebar\" must be set to true for the framebar to be visible.")
	
settingsField(bool, dontShowMoveName, false,
	"Don't Show Move's Name", SETTINGS_GENERAL,
	"; Specify true or false.\n"
	"; In the main UI window, there's a field called 'Move' which displays the last performed move or several moves\n"
	"; that the mod decided to combine together. That text can get pretty long. If you set this setting to true,\n"
	"; then that field will be hidden, and you will only be able to see moves' names either in 'Cancels (P1/P2)' window,\n"
	"; or by hovering your mouse over the 'Startup' or 'Total' fields in the main UI window and reading their tooltip.")
	
settingsField(bool, showComboProrationInRiscGauge, false,
	"Show Combo Proration In RISC Gauge", SETTINGS_GENERAL,
	"; Specify true or false.\n"
	"; Setting this to true will modify the RISC gauge so that the middle represents 0 RISC and no combo proration,\n"
	"; the right half represents RISC, and the left half represents combo proration.")
	
settingsField(bool, displayInputHistoryWhenObserving, true,
	"Display Input History When Observing", SETTINGS_GENERAL,
	"; Specify true or false.\n"
	"; Setting this to true will display both players' input history when observing online matches.\n"
	"; The associated hotkey setting for this setting is \"toggleShowInputHistory\".")
	
settingsField(bool, displayInputHistoryInSomeOfflineModes, false,
	"Display Input History In Some Offline Modes Vs CPU", SETTINGS_GENERAL,
	"; Specify true or false.\n"
	"; Setting this to true will display both players' input history when playing against CPU in Episode/Story, offline Versus,\n"
	"; Tutorial, offline MOM and Mission.\n"
	"; The associated hotkey setting for this setting is \"toggleShowInputHistory\".")
	
settingsField(bool, displayInputHistoryInOnline, false,
	"Display Input History In Online Modes", SETTINGS_GENERAL,
	"; Specify true or false.\n"
	"; Setting this to true will display only your input history when playing online.\n"
	"; In online training mode, both players' input history is shown."
	"; The associated hotkey setting for this setting is \"toggleShowInputHistory\".")
	
settingsField(bool, showDurationsInInputHistory, false,
	"Display Durations In Input History", SETTINGS_GENERAL,
	"; Specify true or false.\n"
	"; Setting this to true will display the duration of each input, in frames, in the input history.")
	
settingsField(bool, usePositionResetMod, false,
	"Use Position Reset Mod", SETTINGS_GENERAL,
	"; Specify true or false.\n"
	"; Setting this to true will override the game's default behavior of position reset (stage reset) in offline Training Mode.\n"
	"; The numbers mentioned below are directions, in numpad notation.\n"
	"; 2+Reset: non-swapped roundstart position;\n"
	"; 8+Reset: swapped roundstart position;\n"
	"; 4+Reset: left corner. Human Player outside the corner, CPU inside;\n"
	"; 1+Reset: left corner. Human Player inside the corner, CPU outside;\n"
	"; 6+Reset: right corner. Human Player outside the corner, CPU inside;\n"
	"; 3+Reset: right corner. Human Player inside the corner, CPU outside;\n"
	"; 5+Reset: reset to last used position;\n"
	"; 9+Reset: set current arbitrary position of both players as 'last used position'.\n"
	"; You can use the \"positionResetDistBetweenPlayers\" and \"positionResetDistFromCorner\" settings to tweak\n"
	"; the default positions in the corner.")
	
settingsField(int, positionResetDistBetweenPlayers, 105000,
	"Position Reset Corner - Distance Between Players", SETTINGS_GENERAL,
	"; A number.\n"
	"; Specifies the distance between the two players, not divided by 100, when resetting position into the corner.\n"
	"; This setting is only used when \"usePositionResetMod\" setting is enabled.\n"
	"; The default value of this field is 105000.")
	
settingsField(int, positionResetDistFromCorner, 0,
	"Position Reset Corner - Distance From Corner", SETTINGS_GENERAL,
	"; A number.\n"
	"; Specifies the distance of the player closest to the corner, from said corner, not divided by 100,\n"
	"; when resetting position into the corner.\n"
	"; This setting is only used when \"usePositionResetMod\" setting is enabled.\n"
	"; The default value of this field is 0.")
	
settingsField(bool, useAlternativeStaggerMashProgressDisplay, false,
	"Use Alternative Stagger Mash Progress Display", "Main UI Window - Stun/Stagger Mash",
	"; Specify true or false.\n"
	"; Setting this to true will display Progress differently in Stun/Stagger Mash window.\n"
	"; Instead of displaying it as Mashed + Animation Duration / Stagger Duration - 4, it will\n"
	"; display as Animation Duration / Stagger Duration - 4 - Mashed")
	
settingsField(bool, dontShowMayInteractionChecks, false,
	"Don't Show May Interaction Checks", SETTINGS_HITBOX_SETTINGS,
	"; Specify true or false.\n"
	"; When a May P or K Ball is on the screen, a circle is drawn around it, an extra point is displayed on the Ball,\n"
	"; and, when May is airborne, an extra point is displayed in the center of body of May, and a line connecting that\n"
	"; point to the extra point on the Ball is displayed.\n"
	"; For Dolphin, this displays an extra point on May, a line connecting that point to the origin point of the Dolphin,\n"
	"; and a circle around the Dolphin denoting the range in which May's extra point must be in order for May to hop\n"
	"; on the Dolphin.\n"
	"; When this setting is true, none of this is displayed.")
	
settingsField(bool, showMilliaBadMoonBuffHeight, false,
	"Show Millia Bad Moon Buff Height (Rev2 only)", SETTINGS_CHARACTER_SPECIFIC,
	"; Specify true or false.\n"
	"; When this setting is on, and one of the character is Millia, a horizontal line is displayed high above the arena,\n"
	"; showing the height on which Millia's Bad Moon obtains some kind of attack powerup.\n"
	"; Millia's origin point must be above the line in order to gain the powerup.\n"
	"; Note that Bad Moon's maximum number of hits increases as it gets higher, up to 10 hits maximum.\n"
	"; The line is the lowest height needed to get any powerup at all.")
	
settingsField(bool, showFaustOwnFlickRanges, true,
	"Show Faust Thrown Item Flick Ranges", SETTINGS_CHARACTER_SPECIFIC,
	"; Specify true or false.\n"
	"; When this setting is on, when Faust does a 5D, two ranges are shown around his flickpoint,\n"
	"; denoting ranges in which his thrown items' origin points must be to get either hit or homerun hit.")
	
settingsField(bool, showBedmanTaskCHeightBuffY, false,
	"Show Bedman Task C Height Buff Y", SETTINGS_CHARACTER_SPECIFIC,
	"; Specify true or false.\n"
	"; When this setting is on, a horizontal line is constantly shown on the screen at the height above which\n"
	"; Bedman's Task C gains a buff.\n"
	"; This line is so high you can't see it unless you jump.")
	
settingsField(bool, showJackoGhostPickupRange, false,
	"Show Jack-O' Ghost Pickup Range", SETTINGS_CHARACTER_SPECIFIC,
	"; Specify true or false.\n"
	"; When this setting is on, an infinite vertical box around each Ghost (house) denotes the range in which\n"
	"; Jack-O's origin point must be in order to pick up the Ghost or gain Tension from it.")
	
settingsField(bool, showJackoSummonsPushboxes, false,
	"Show Jack-O' Summons' Pushboxes", SETTINGS_CHARACTER_SPECIFIC,
	"; Specify true or false.\n"
	"; When this setting is on, yellow pushboxes are drawn around the Servants and the Ghosts, similar to how they're drawn\n"
	"; around the players.")
	
settingsField(bool, showJackoAegisFieldRange, false,
	"Show Jack-O' Aegis Field Range", SETTINGS_CHARACTER_SPECIFIC,
	"; Specify true or false.\n"
	"; When this setting is on, the white circle around Jack-O' shows the range where the Servants' and the Ghosts'\n"
	"; origin point must be in order to receive protection from Aegis Field.")
	
settingsField(bool, showJackoServantAttackRange, false,
	"Show Jack-O' Servant Attack Range", SETTINGS_CHARACTER_SPECIFIC,
	"; Specify true or false.\n"
	"; When this setting is on, the white box around each Servant shows the area where the opponent's player's\n"
	"; origin point must be in order for the Servant to initiate an attack.")
	
settingsKeyCombo(framebarVisibilityToggle, "Hide Framebar Toggle", "",
	"; A keyboard shortcut.\n"
	"; Pressing this shortcut will show/hide the framebar window by changing the \"showFramebar\" setting.\n"
	"; If \"closingModWindowAlsoHidesFramebar\" is true, then setting \"showFramebar\" to true is not enough, as the main\n"
	"; mod's UI window must also be open in order to show the framebar. If the window is not open, and \"showFramebar\" is true,\n"
	"; then the only way to open it and to show the framebar is with the \"modWindowVisibilityToggle\" hotkey.")
	
settingsField(bool, showStrikeInvulOnFramebar, true,
	"Show Strike Invul", SETTINGS_FRAMEBAR,
	"; Specify true or false.\n"
	"; Strike invul will be displayed using a green ^ on top of a frame.\n"
	"; Note: when \"condenseIntoOneProjectileFramebar\" is used, for P2 these will be displayed below a frame.")
	
settingsField(bool, showSuperArmorOnFramebar, true,
	"Show Super Armor", SETTINGS_FRAMEBAR,
	"; Specify true or false.\n"
	"; Super armor will be displayed using a purple ^ on top of a frame.\n"
	"; It includes reflect, parry and projectile-only invulnerability\n"
	"; (excluding Aegis Field, that isn't displayed on the framebar at all). If both strike invul and super armor are present, super armor\n"
	"; will be below the strike invul (for P2 framebar: on top).\n"
	"; Note: when \"condenseIntoOneProjectileFramebar\" is used, for P2 these will be displayed below a frame, instead of on top.")
	
settingsField(bool, showThrowInvulOnFramebar, true,
	"Show Throw Invul", SETTINGS_FRAMEBAR,
	"; Specify true or false.\n"
	"; Throw invul will be displayed using a yellow v underneath a frame.")
	
settingsField(bool, showOTGOnFramebar, true,
	"Show OTG", SETTINGS_FRAMEBAR,
	"; Specify true or false.\n"
	"; OTG state will be displayed using a gray v underneath a frame.")
	
settingsField(bool, showFirstFramesOnFramebar, true,
	"Show First Frames", SETTINGS_FRAMEBAR,
	"; Specify true or false.\n"
	"; When a player's animation changes from one to another, except in certain cases, the first frame of the new animation is denoted with\n"
	"; a ' mark before that frame. For some animations a first frame is denoted even when\n"
	"; the animation didn't change, but instead reached some important point. This includes entering hitstop.")
	
settingsField(bool, considerSimilarFrameTypesSameForFrameCounts, true,
	"Consider Similar Frame Types Same For Frame Count", SETTINGS_FRAMEBAR,
	"; Specify true or false.\n"
	"; This affects the frame counts that are displayed on the framebar. If true, the frame counter will not reset\n"
	"; between different frame graphics that all mean recovery or all mean startup and so on. Idle frames are not\n"
	"; affected by this and for them you should use the \"considerSimilarIdleFramesSameForFrameCounts\" setting.")
	
settingsField(bool, considerSimilarIdleFramesSameForFrameCounts, false,
	"Consider Similar Idle Frames Same For Frame Count", SETTINGS_FRAMEBAR,
	"; Specify true or false.\n"
	"; This affects the frame counts that are displayed on the framebar. If true, the frame counter will not reset\n"
	"; between different frame graphics that all mean idle.")
	
settingsField(bool, combineProjectileFramebarsWhenPossible, true,
	"Combine Projectile Framebars When Possible", SETTINGS_FRAMEBAR,
	"; Specify true or false.\n"
	"; When true, two or more projectile framebars will be combined into one if their active/non-active frames don't intersect.")
	
settingsField(bool, condenseIntoOneProjectileFramebar, true,
	"Condense Into One Projectile Framebar Per Player", SETTINGS_FRAMEBAR,
	"; Specify true or false.\n"
	"; When true, all projectiles belonging to a player will use the same one framebar, located on top\n"
	"; of Player 1's main framebar and below Player 2's main framebar.\n"
	"; You can use the \"projectileFramebarHeight\" setting to make the projectile framebar thin.")
	
settingsField(bool, eachProjectileOnSeparateFramebar, false,
	"Each Projectile On A Separate Framebar", SETTINGS_FRAMEBAR,
	"; Specify true or false.\n"
	"; When true, projectiles will never be combined even if there are very many of them and they're all same.")
	
settingsField(bool, dontClearFramebarOnStageReset, false,
	"Don't Clear Framebar On Stage Reset", SETTINGS_FRAMEBAR,
	"; Specify true or false.\n"
	"; When true, the framebar won't be cleared when resetting the stage with Record+Playback or Backspace or Pause Menu - Reset Position in Training Mode,\n"
	"; or when a player dies or when a new rounds starts.")
	
settingsField(bool, dontTruncateFramebarTitles, false,
	"Don't Truncate Framebar Titles", SETTINGS_FRAMEBAR,
	"; Specify true or false.\n"
	"; Each framebar displays a label or title either to the left or to the right from it. The titles are truncated to 12 character by default.\n"
	"; By setting this setting to true, you can stop them from truncating and always display full titles.")
	
settingsField(bool, useSlangNames, false,
	"Use Slang In Move & Projectile Names", SETTINGS_FRAMEBAR,
	"; Specify true or false.\n"
	"; Each framebar displays a label or title either to the left or to the right from it.\n"
	"; When the title is too long, depending on the setting, it gets cut off. Setting this to true changes some\n"
	"; projectile titles to slang names to make framebar titles short so that they fit.\n"
	"; This also changes names of moves that are displayed in main UI window and other windows.")
	
settingsField(bool, showPlayerInFramebarTitle, true,
	"Show Player In Framebar Title", SETTINGS_FRAMEBAR,
	"; Specify true or false.\n"
	"; Each framebar displays a label or title either to the left or to the right from it, depending on which player it belongs to.\n"
	"; The \"allFramebarTitlesDisplayToTheLeft\" setting can be used to make all framebar titles always display on the left.\n"
	"; How to tell which player it is then? This is where this setting comes in.\n"
	"; Setting this to true adds \"P1\" or \"P2\" color-coded text to each framebar's title.")
	
settingsField(bool, allFramebarTitlesDisplayToTheLeft, true,
	"All Framebar Titles Display On The Left", SETTINGS_FRAMEBAR,
	"; Specify true or false.\n"
	"; Each framebar displays a label or title either to the left or to the right from it, depending on which player it belongs to.\n"
	"; By setting this setting to true, you can make all framebar titles always display on the left.\n"
	"; To help tell which player it is you can use the \"showPlayerInFramebarTitle\" setting.")
	
settingsField(int, playerFramebarHeight, 19,
	"Player Framebar Height", SETTINGS_FRAMEBAR,
	"; A number.\n"
	"; Specifies the height of a single framebar of one player, including the black outlines on the outside.\n"
	"; The standard height is 19.\n"
	"; This value is specified in pixels only on 1280x720 resolution. On higher resolutions this gets multiplied by\n"
	"; Screen Height / 720.")
	
settingsField(int, projectileFramebarHeight, 11,
	"Projectile Framebar Height", SETTINGS_FRAMEBAR,
	"; A number.\n"
	"; Specifies the height of a single framebar of one projectile, including the black outlines on the outside.\n"
	"; The standard height of projectile framebar is 11. Of player framebar, 19.\n"
	"; This value is specified in pixels only on 1280x720 resolution. On higher resolutions this gets multiplied by\n"
	"; Screen Height / 720.")

settingsField(int, digitThickness, 1,
	"Framebar Digit Thickness", SETTINGS_FRAMEBAR,
	"; A number. Can only equal 1 or 2.\n"
	"; Specifies the thickness of text for the numbers displayed in the framebar over the frames, denoting the number\n"
	"; of consecutive same frames.")

settingsField(bool, drawDigits, true,
	"Draw Digits", SETTINGS_FRAMEBAR,
	"; Specify true or false.\n"
	"; Disables the drawing of numbers displayed in the framebar over the frames, denoting the number\n"
	"; of consecutive same frames.")

settingsField(int, distanceBetweenPlayerFramebars, 30,
	"Distance Between Player Framebars", SETTINGS_FRAMEBAR,
	"; A number. Can be negative.\n"
	"; Specifies the padding between the two main player framebars, but keep in mind,\n"
	"; the actual padding may be greater to accomodate for throw invul, strike invul and other triangular markers\n"
	"; on top and below the frames.\n"
	"; This distance gets divided by 10 and multiplied by 19 / \"playerFramebarHeight\" * Screen Height / 720\n"
	"; to get the actual distance in pixels (this does not include the extra padding for invulnerability markers).")

settingsField(int, distanceBetweenProjectileFramebars, 30,
	"Distance Between Projectile Framebars", SETTINGS_FRAMEBAR,
	"; A number. Can be negative.\n"
	"; Specifies the padding between the the Player 2 framebar and the next projectile framebar,\n"
	"; and between the projectile framebars, but keep in mind, the actual padding may be greater\n"
	"; to accomodate for throw invul, strike invul and other triangular markers on top and below the frames.\n"
	"; This distance gets divided by 10 and multiplied by 19 / \"playerFramebarHeight\" * Screen Height / 720\n"
	"; to get the actual distance in pixels (this does not include the extra padding for invulnerability markers).")

settingsField(int, framebarTitleCharsMax, 12,
	"Framebar Title Max Characters", SETTINGS_FRAMEBAR,
	"; A number.\n"
	"; Specifies the maximum number of characters that can be displayed in a framebar title.\n"
	"; This does not include the \"P1 \" and \"P2 \" additional text that is displayed when \"showPlayerInFramebarTitle\" is true.\n"
	"; You can use \"useSlangNames\" to help reduce the lengths of text displayed in framebar titles.\n"
	"; The standard value is 12.")
	
settingsField(int, framebarDisplayedFramesCount, 80,
	"Number Of Displayed Frames", SETTINGS_FRAMEBAR,
	"; A number.\n"
	"; Specifies the maximum number of frames that will be displayed on the screen at a time.\n"
	"; If there're more frames stored (see \"framebarStoredFramesCount\") in the framebar than this number,\n"
	"; a horizontal scrollbar will be displayed.\n"
	"; This value can't be more than 200.\n"
	"; The standard value is 80.")
	
settingsField(int, framebarStoredFramesCount, 200,
	"Number Of Stored Frames", SETTINGS_FRAMEBAR,
	"; A number.\n"
	"; Specifies the maximum number of past frames that can be viewed by scrolling the framebar horizontally,\n"
	"; including frames that are visible without having to scroll.\n"
	"; This value can't be more than 200.\n"
	"; The standard value is 200.")
	
settingsField(bool, frameAdvantage_dontUsePreBlockstunTime, true,
	"Frame Advantage: Don't Use Pre-Blockstun Time", SETTINGS_GENERAL,
	"; Specify true or false.\n"
	"; This setting will affect how the 'Frame Advantage' field in the UI is calculated.\n"
	"; Normally, attacks put your opponent in blockstun right away, however, there are some moves that\n"
	"; may put your opponent in blockstun after the move is over. Such moves are usually projectiles, such\n"
	"; as Ky j.D or I-No Antidepressant Scale.\n"
	"; When this setting is false, then, whenever the opponent enters blockstun, the time that you spent idle\n"
	"; before that gets included in the frame advantage with a +. For example:\n"
	"; You shoot a projectile and then recover. Then you and the opponent are just standing there for 1000000 frames.\n"
	"; Then, the projectile, that you shot, puts the opponent into blockstun for 1 frame. Your frame advantage will be 1000001.\n"
	"; Setting this to true will cause the idle time that you spent before the opponent entered blockstun to not be included\n"
	"; in your frame advantage, and your frame advantage in the example above will be just +1.\n"
	"; After changing this setting you don't need to repeat the last move, as the 'Frame Adv.' field will get updated automatically.")
	
settingsField(bool, skipGrabsInFramebar, true,
	"Skip Grab/Super Animations In Framebar", SETTINGS_FRAMEBAR,
	"; Specify true or false.\n"
	"; Setting this to true (default) will skip grab animations such as ground throw or some supers that connected in the framebar.")
	
settingsField(bool, showFramebarHatchedLineWhenSkippingGrab, true,
	"Show Hatched/Dashed Slicing Line When Skipping Grab/Super In Framebar", SETTINGS_FRAMEBAR,
	"; Specify true or false.\n"
	"; When this setting is true (default), if a grab or super is skipped because of the \"skipGrabsInFramebar\" setting,\n"
	"; a white line made out of small diagonal hatches will be displayed on the framebar in places where the grab or super was skipped.")
	
settingsField(bool, showFramebarHatchedLineWhenSkippingHitstop, false,
	"Show Hatched/Dashed Slicing Line When Skipping Hitstop In Framebar", SETTINGS_FRAMEBAR,
	"; Specify true or false.\n"
	"; When this setting is true (which is not the default), if hitstop is skipped because of the \"neverIgnoreHitstop\"\n"
	"; setting being false,\n"
	"; a white line made out of small diagonal hatches will be displayed on the framebar in places where hitstop was skipped.")
	
settingsField(bool, showFramebarHatchedLineWhenSkippingSuperfreeze, true,
	"Show Hatched/Dashed Slicing Line When Skipping Superfreeze In Framebar", SETTINGS_FRAMEBAR,
	"; Specify true or false.\n"
	"; When this setting is true (default), if superfreeze (also called superflash) is skipped, which is always the case,\n"
	"; a white line made out of small diagonal hatches will be displayed on the framebar in places where superfreeze was skipped.")
	
settingsField(bool, showP1FramedataInFramebar, true,
	"Show P1 Framedata In Framebar", SETTINGS_FRAMEBAR,
	"; Specify true or false.\n"
	"; When this setting is true (default), Startup/Active/Recovery/Frame Advantage will be displayed on top of P1's framebar.")
	
settingsField(bool, showP2FramedataInFramebar, true,
	"Show P2 Framedata In Framebar", SETTINGS_FRAMEBAR,
	"; Specify true or false.\n"
	"; When this setting is true (default), Startup/Active/Recovery/Frame Advantage will be displayed underneath P2's framebar.")
	
settingsField(float, framedataInFramebarScale, 0.F,
	"Framedata In Framebar Scale", SETTINGS_FRAMEBAR,
	"; Specify a floating point number. <=0 means to scale to a factor of 2 if the screen width is > 1920.\n"
	"; This controls the size of the 'Startup, Active, Recovery, Total, Advantage' text above P1's framebar and below P2's framebar.")
	
settingsField(bool, neverIgnoreHitstop, false,
	"Never Ignore Hitstop", SETTINGS_FRAMEBAR,
	"; Specify true or false.\n"
	"; Normally we don't display hitstop in the framebar if both players are in hitstop on that frame,\n"
	"; unless a projectile or a blocking Baiken (see \"ignoreHitstopForBlockingBaiken\") is present.\n"
	"; If this is set to true, then we always show hitstop in the framebar.\n"
	"; After changing this setting, you don't need to repeat the moves or actions to see updated result in the framebar.\n"
	"; This setting can be changed with the \"toggleNeverIgnoreHitstop\" hotkey.")
	
settingsField(bool, ignoreHitstopForBlockingBaiken, false,
	"Ignore Hitstop For Blocking Baiken", SETTINGS_FRAMEBAR,
	"; Specify true or false.\n"
	"; Normally we don't display hitstop in the framebar if both players are in hitstop on that frame,\n"
	"; unless a projectile or a blocking Baiken is present.\n"
	"; If this is set to true, then we ignore the blocking Baiken part and display hitstop in the framebar only\n"
	"; when a projectile is present.\n"
	"; There's another setting that controls the display of hitstop in the framebar: \"neverIgnoreHitstop\".\n"
	"; If that setting is set to true, hitstop is always shown, no matter what \"ignoreHitstopForBlockingBaiken\" setting is.\n"
	"; After changing this setting, you need to repeat the moves or actions to see updated result in the framebar.")
	
settingsField(bool, considerRunAndWalkNonIdle, false,
	"Consider Running And Walking Non-Idle", SETTINGS_FRAMEBAR,
	"; Specify true or false.\n"
	"; Normally we consider running and walking as being idle, which does not advance the framebar forward.\n"
	"; The framebar only advances when one of the players is \"busy\".\n"
	"; If this is set to true, then one player running or walking will be treated same way as \"busy\" and will advance the framebar.\n"
	"; In the UI's 'Frame Advantage' display, idle running (except Leo's) and walking will still always be considered idle.")
	
settingsField(bool, considerCrouchNonIdle, false,
	"Consider Crouching Non-Idle", SETTINGS_FRAMEBAR,
	"; Specify true or false.\n"
	"; Normally we consider crouching as being idle, which does not advance the framebar forward.\n"
	"; The framebar only advances when one of the players is \"busy\".\n"
	"; If this is set to true, then one player crouching or walking will be treated same way as \"busy\" and will advance the framebar.\n"
	"; A dummy who is crouching automatically due to training settings will still not be considered \"busy\" no matter what.\n"
	"; In the UI's 'Frame Advantage' display, idle crouching will still always be considered idle.")
	
settingsField(bool, considerKnockdownWakeupAndAirtechIdle, false,
	"Consider Knockdown, Wakeup and Airtech Idle", SETTINGS_FRAMEBAR,
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
	"; In such cases, look for an animation start delimiter on the opponent's framebar, shown as a white ' between frames.")
	
settingsField(bool, considerIdleInvulIdle, false,
	"Consider Invul While Being Idle As Idle", SETTINGS_FRAMEBAR,
	"; Specify true or false\n"
	"; After waking up, or leaving blockstun or hitstun, or airteching, usually there's some strike and/or throw invul while\n"
	"; you're idle and are able to fully act. Framebar only advances forward when one or both players are not idle, however,\n"
	"; if this invul is present, you can set this setting to false to advance the framebar anyway.")
	
settingsField(bool, considerDummyPlaybackNonIdle, false,
	"Consider The Entirety Of Dummy Recording Playback Non-Idle", SETTINGS_FRAMEBAR,
	"; Specify true or false.\n"
	"; Normally we consider standing as being idle, which does not advance the framebar forward.\n"
	"; The framebar only advances when one of the players is \"busy\".\n"
	"; If this is set to true, then as soon as you tell the dummy to play a recording slot, the entirety of the playback will be considered\n"
	"; as \"busy\" and will advance the framebar, even if in parts of the recording the dummy is not doing anything.")
	
settingsField(bool, useColorblindHelp, false,
	"Use Colorblindness Help", SETTINGS_FRAMEBAR,
	"; Specify true or false.\n"
	"; If true, certain types of frames in the framebar will be displayed with distinct hatches on them.\n"
	"; Make sure the framebar is scaled wide enough and the screen resolution is large enough that you can see the hatches properly.\n"
	"; To scale the framebar you can drag its right edge.")
	
settingsField(bool, clearFrameSelectionWhenFramebarAdvances, true,
	"Clear Frame Selection When Framebar Advances", SETTINGS_FRAMEBAR,
	"; Specify true or false.\n"
	"; If true, when framebar moves forward, it will automatically reset your frame selection on the framebar.\n"
	"; You can select a range of frames on the framebar using your mouse and that is what gets reset.\n"
	"; The selection won't get reset no matter what, if you're still holding down the mouse button when the framebar advances.")
	
settingsField(bool, comboRecipe_showDelaysBetweenCancels, true,
	"Show Delays Between Cancels", SETTINGS_COMBO_RECIPE,
	"; Specify true or false.\n"
	"; Setting this to true (default) will show delays on dedicated separate lines in gray text in the following format: '(Delay #f)',\n"
	"; where # is a number.")
	
settingsField(bool, comboRecipe_showIdleTimeBetweenMoves, true,
	"Show Idle Time Between Moves", SETTINGS_COMBO_RECIPE,
	"; Specify true or false.\n"
	"; Setting this to true (default) will show idle time on dedicated separate lines in gray text in the following format: '(Idle #f)',\n"
	"; where # is a number.")
	
settingsField(bool, comboRecipe_showDashes, true,
	"Show Microdashes/Dashes", SETTINGS_COMBO_RECIPE,
	"; Specify true or false.\n"
	"; Setting this to true (default) will show microdashes and dashes on dedicated separate lines in the following format:\n"
	"; '#f Microdash/Dash', where # is a number.")
	
settingsField(bool, comboRecipe_showWalks, true,
	"Show Microwalks/Walks", SETTINGS_COMBO_RECIPE,
	"; Specify true or false.\n"
	"; Setting this to true (default) will show microwalks and walks on dedicated separate lines in the following format:\n"
	"; '#f Microwalk/Walk/Microwalk Back/Walk Back', where # is a number.")
	
settingsField(bool, comboRecipe_showSuperJumpInstalls, true,
	"Show Super Jump Installs", SETTINGS_COMBO_RECIPE,
	"; Specify true or false.\n"
	"; Setting this to true (default) will show super jump installs on dedicated separate lines in the following format: 'Super Jump Install'.\n"
	"; This setting does not affect the display of (regular) jump installs, which are always displayed.")
	
settingsField(bool, comboRecipe_showNumberOfHits, true,
	"Show Number Of Hits", SETTINGS_COMBO_RECIPE,
	"; Specify true or false.\n"
	"; Setting this to true will make the Combo Recipe panel display the number of hits in parentheses, like so: 2H(2);\n"
	"; This would mean that 2H hit twice.")
	
settingsField(bool, comboRecipe_showCharge, true,
	"Show Charge", SETTINGS_COMBO_RECIPE,
	"; Specify true or false.\n"
	"; Setting this to true will make the Combo Recipe panel display the charge out of max charge of chargeable moves in\n"
	"; parentheses, like so: 6P (Held: 2/99f);\n"
	"; This would mean that you have advanced 2 frames out of the required 99 to reach the next level of charge.")
	
settingsField(bool, comboRecipe_clearOnPositionReset, true,
	"Clear On Position Reset", SETTINGS_COMBO_RECIPE,
	"; Specify true or false.\n"
	"; Setting this to true will make the Combo Recipe panel clear when positions are reset only.\n"
	"; This won't include when the round ends due to one of the players dying, but will include round\n"
	"; resets that happened due to timeout.")
	
settingsField(bool, comboRecipe_transparentBackground, false,
	"Transparent Background", SETTINGS_COMBO_RECIPE,
	"; Specify true or false.\n"
	"; Setting this to true will make the Combo Recipe panel display without a background, just text and grid cell outlines.")
	
settingsField(int, startingTensionPulseP1, 0,
	"Starting Tension Pulse For Player 1", SETTINGS_GENERAL,
	"; A number.\n"
	"; Works only in Training Mode. Upon a stage reset, the Tension Pulse of Player 1 will be set to this value.\n"
	"; Must be in the range [-25000; +25000].")
	
settingsField(int, startingTensionPulseP2, 0,
	"Starting Tension Pulse For Player 2", SETTINGS_GENERAL,
	"; A number.\n"
	"; Works only in Training Mode. Upon a stage reset, the Tension Pulse of Player 2 will be set to this value.\n"
	"; Must be in the range [-25000; +25000].")
	
settingsField(int, startingBurstGaugeP1, 15000,
	"Starting Burst Gauge For Player 1", SETTINGS_GENERAL,
	"; A number.\n"
	"; Works only in Training Mode and if Pause Menu - Psych Burst is set to Infinite.\n"
	"; Instead of to maximum, the Burst Gauge of Player 1 will be set to this value.\n"
	"; Must be in the range [0; 15000]. Default value is 15000.")
	
settingsField(int, startingBurstGaugeP2, 15000,
	"Starting Burst Gauge For Player 2", SETTINGS_GENERAL,
	"; A number.\n"
	"; Works only in Training Mode and if Pause Menu - Psych Burst is set to Infinite.\n"
	"; Instead of to maximum, the Burst Gauge of Player 2 will be set to this value.\n"
	"; Must be in the range [0; 15000]. Default value is 15000.")
	
settingsField(bool, hideWins, false,
	"Hide Wins - On/Off", SETTINGS_GENERAL,
	"; Specify true or false.\n"
	"; Prevents wins from being displayed on the online rematch screen.\n"
	"; Works both when you're playing a match or observing.")
	
settingsField(bool, hideWinsDirectParticipantOnly, false,
	"Hide Wins - Only When Playing - On/Off", SETTINGS_GENERAL,
	"; Specify true or false.\n"
	"; Prevents wins from being displayed on the online rematch screen.\n"
	"; Only works when you're playing a match, not when observing.")
	
settingsField(int, hideWinsExceptOnWins, 0,
	"Hide Wins - Except When N Wins Reached", SETTINGS_GENERAL,
	"; A number.\n"
	"; Prevents wins from being hidden by the \"hideWins\" and \"hideWinsDirectParticipantOnly\" settings,\n"
	"; when one of the players reaches the specified number of wins. Set to 0 or a negative number to disable.")
	
settingsField(bool, hideRankIcons, false,
	"Hide Rank Icons", SETTINGS_GENERAL,
	"; Specify true or false.\n"
	"; Prevents rank icons (circle, arrow up, arrow down, equal sign) from showing up next to players.")
	
settingsField(bool, showDebugFields, false,
	"Show Debug Fields", SETTINGS_GENERAL,
	"; Specify true or false.\n"
	"; Setting this to true will add a number of extra fields to the UI that display debug or miscellaneous information.")
	
settingsField(bool, ignoreNumpadEnterKey, false,
	"Ignore Numpad Enter Key", SETTINGS_GENERAL,
	"; Specify true or false.\n"
	"; Setting this to true will hide the numpad Enter key presses from the game.")
	
settingsField(bool, ignoreRegularEnterKey, false,
	"Ignore Regular Enter Key", SETTINGS_GENERAL,
	"; Specify true or false.\n"
	"; Setting this to true will hide the regular, non-numpad Enter key presses from the game.")
	
settingsField(bool, overrideOnlineInputDelay, false,
	"Override Online Input Delay", SETTINGS_GENERAL,
	"; Specify true or false.\n"
	"; Setting this to true will allow you to use \"onlineInputDelayFullscreen\" and \"onlineInputDelayWindowed\"\n"
	"; settings to change the online input delay.\n"
	"; Setting this to false does not undo those changes.\n"
	"; If Pangaea's mod's online input delay setting is used, this mod will have priority over Pangaea's mod.")
	
settingsField(int, onlineInputDelayFullscreen, 1,
	"Online Input Delay - Fullscreen", SETTINGS_GENERAL,
	"; A number from 0 to 4, in frames. Default value is 1 frame.\n"
	"; This setting only works if \"overrideOnlineInputDelay\" is set to true.\n"
	"; Is meant for fullscreen non-windowed mode.\n"
	"; For windowed or fullscreen windowed mode use the \"onlineInputDelayWindowed\" setting.\n"
	"; The correct setting is chosen based on whether the game is fullscreen or not at the time of starting the battle.\n"
	"; It is possible to change these settings while the game is running but it has not been tested whether they take effect\n"
	"; immediately, or a battle restart is required.\n"
	"; If Pangaea's mod's online input delay setting is used, this mod will have priority over Pangaea's mod.")
	
settingsField(int, onlineInputDelayWindowed, 1,
	"Online Input Delay - Windowed/Fullscreen-Windowed", SETTINGS_GENERAL,
	"; A number from 0 to 4, in frames. Default value is 1 frame.\n"
	"; This setting only works if \"overrideOnlineInputDelay\" is set to true.\n"
	"; Is meant for windowed and fullscreen windowed modes.\n"
	"; For fullscreen mode use the \"onlineInputDelayFullscreen\" setting.\n"
	"; The correct setting is chosen based on whether the game is fullscreen or not at the time of starting the battle.\n"
	"; It is possible to change these settings while the game is running but it has not been tested whether they take effect\n"
	"; immediately, or a battle restart is required.\n"
	"; If Pangaea's mod's online input delay setting is used, this mod will have priority over Pangaea's mod.")
	
settingsField(bool, player1IsBoss, false,
	"Player 1 Is Boss (Offline Only)", SETTINGS_GENERAL,
	"; Specify true or false.\n"
	"; Setting this to true will make Player 1 be considered the Arcade Boss.\n"
	"; Works only in Training and Versus Modes.")
	
settingsField(bool, player2IsBoss, false,
	"Player 2 Is Boss (Offline Only)", SETTINGS_GENERAL,
	"; Specify true or false.\n"
	"; Setting this to true will make Player 2 be considered the Arcade Boss.\n"
	"; Works only in Training and Versus Modes.")
	
settingsField(bool, p1RamlethalDisableMarteliForpeli, false,
	"Disable Marteli & Forpeli (P1)", SETTINGS_GENERAL,
	"; Specify true or false.\n"
	"; Setting this to true will disable P1 Ramlethal's Marteli and Forpeli special moves when the Sword hasn't been deployed yet.\n"
	"; This will allow you, if you also use the \"player1IsBoss\" setting, to use the boss exclusive 214S and 214H moves.\n"
	"; Works only in Training and Versus Modes.")
	
settingsField(bool, p2RamlethalDisableMarteliForpeli, false,
	"Disable Marteli & Forpeli (P2)", SETTINGS_GENERAL,
	"; Specify true or false.\n"
	"; Setting this to true will disable P2 Ramlethal's Marteli and Forpeli special moves when the Sword hasn't been deployed yet.\n"
	"; This will allow you, if you also use the \"player2IsBoss\" setting, to use the boss exclusive 214S and 214H moves.\n"
	"; Works only in Training and Versus Modes.")
	
settingsField(bool, p1RamlethalUseBoss6SHSwordDeploy, false,
	"Use Boss Ver. Of 6S/6H Sword Deploy (P1)", SETTINGS_GENERAL,
	"; Specify true or false.\n"
	"; Setting this to true will allow you, if you also use the \"player1IsBoss\" setting,\n"
	"; to use the boss exclusive 6S and 6H moves.\n"
	"; Works only in Training and Versus Modes.")
	
settingsField(bool, p2RamlethalUseBoss6SHSwordDeploy, false,
	"Use Boss Ver. Of 6S/6H Sword Deploy (P2)", SETTINGS_GENERAL,
	"; Specify true or false.\n"
	"; Setting this to true will allow you, if you also use the \"player2IsBoss\" setting,\n"
	"; to use the boss exclusive 6S and 6H moves.\n"
	"; Works only in Training and Versus Modes.")
	
settingsField(bool, useSigscanCaching, true,
	"Use Sigscan Caching", SETTINGS_GENERAL,
	"; Specify true or false.\n"
	"; Setting this to true will allow the mod to load faster, at the expense of potential\n"
	"; crashes in case the game's GuiltyGearXrd.exe executable's code is altered in such a way\n"
	"; that affects this mod.\n"
	"; A sigscan is a search for a sequence of bytes in the executable code.\n"
	"; Mods, recording software, cheats and hacks modify it, and in this case sigscans must be\n"
	"; repeated. Caching them may lead to incorrect results and then this mod may crash the game.\n"
	"; Reasonable safeguards have been put in place to prevent this, but they had to not slow down\n"
	"; the load time of the mod and therefore may not guarantee that there will be no crash.\n"
	"; You sould enable this setting if you are not altering the recording software that you use,\n"
	"; or are changing which mods are already loaded by the time this mod gets loaded, or\n"
	"; all those things change game executable code only in a way that does not matter for this mod.")
	
settingsField(bool, overrideYourConnectionTierForFilter, false,
	"Override Your Connection Tier For Filter", SETTINGS_GENERAL,
	"; Specify true or false.\n"
	"; If you specify true, when observing the lobby list and trying to enter a lobby,\n"
	"; your connection tier will be substituted with the tier specified in\n"
	"; \"connectionTierToPretendAs\".")
	
settingsField(int, connectionTierToPretendAs, 0,
	"Connection Tier To Pretend As", SETTINGS_GENERAL,
	"; This value must be between 0 and 4.\n"
	"; This setting only works if \"overrideYourConnectionTierForFilter\" is set to true.\n"
	"; Specify the connection tier here to use when entering a room or viewing the lobby list.\n"
	"; 0 means T0, 1 means T1, and so on.")
	
settingsField(bool, highlightRedWhenBecomingIdle, false,
	"Highlight Red When Becoming Idle", SETTINGS_GENERAL,
	"; Specify true or false.\n"
	"; In Training Mode, when your character becomes idle, it will flash red.")
	
settingsField(bool, highlightGreenWhenBecomingIdle, false,
	"Highlight Green When Becoming Idle", SETTINGS_GENERAL,
	"; Specify true or false.\n"
	"; In Training Mode, when your character becomes idle, it will flash green.")
	
settingsField(bool, highlightBlueWhenBecomingIdle, false,
	"Highlight Blue When Becoming Idle", SETTINGS_GENERAL,
	"; Specify true or false.\n"
	"; In Training Mode, when your character becomes idle, it will flash blue.")
	
settingsField(MoveList, highlightWhenCancelsIntoMovesAvailable, {},
	"Highlight When Cancels Into Moves Available", SETTINGS_GENERAL,
	"; Specify a list of moves and red, green and/or blue highlight for each.\n"
	"; When a cancel into any of the specified moves becomes available, your character will flash that color.")
	
settingsField(int, globalWindowTransparency, 100,
	"Global Window Transparency", SETTINGS_GENERAL,
	"; Specify a number from 0 to 100.\n"
	"; Controls the transparency of the background of all UI windows, Combo Damage & Combo Stun.\n"
	"; Combo Recipe has its own setting for transparency, and if it is set to transparent,\n"
	"; that will override this setting, but Combo Recipe's setting is not set, then this setting will take\n"
	"; priority.\n"
	"; Combo Damage is just always transparent and that can't be configured by any setting.")
	
settingsField(bool, outlineAllWindowText, false,
	"Outline All Window Text", SETTINGS_GENERAL,
	"; Specify true or false.\n"
	"; If true, all text in UI windows, except Combo Recipe and Combo Damage & Combo Stun, will be outlined with black color.\n"
	"; Combo Recipe has its own setting for this, and Combo Damage is just always outlined.")
	
settingsField(PinnedWindowList, pinnedWindows, { { false } },
	"Pinned Windows", SETTINGS_GENERAL,
	"; Specify a list of windows that are pinned.")
	
settingsField(bool, openPinnedWindowsOnStartup, true,
	"Open Pinned Windows On Startup", SETTINGS_GENERAL,
	"; Specify true or false.\n"
	"; When true, all pinned windows will be opened when the mod starts.")
	
settingsField(bool, disablePinButton, false,
	"Disable Pin Buttons On Windows", SETTINGS_GENERAL,
	"; Specify true or false.\n"
	"; When true, windows no longer will show pin buttons next to their X button and the pin functionality will be disabled entirely.")
	
settingsField(bool, showYrcWindowsInCancelsPanel, true,
	"Show YRC Windows In Cancels Panel", SETTINGS_GENERAL,
	"; Specify true or false.\n"
	"; When false, the Cancels Panel will no longer show YRC windows.\n"
	"; After changing this setting, you need to redo the move to update the display.")
	
settingsField(bool, dontResetBurstAndTensionGaugesWhenInStunOrFaint, false,
	"Don't Reset Burst And Tension Gauges When In Faint", SETTINGS_GENERAL,
	"; Specify true or false.\n"
	"; Only for Training Mode.\n"
	"; By default, when this is false (the default), the game will reset the Burst Gauge\n"
	"; and Tension Gauge of you or the dummy when none is performing an attack,\n"
	"; or in blockstun or hitstun, or has throw protection, or is airborne or dead.\n"
	"; Unfortunately, the default rules consider 'idle' things like Faint animation,\n"
	"; because that animation is not hitstun.\n"
	"; The Burst Gauge resets to maximum if Training Mode Pause Menu - Psych Burst is set to Infinite.\n"
	"; The Tension Gauge resets to the value set in Training Mode Pause Menu - Tension Gauge.\n"
	"; Setting this to true prevents the Burst Gauge from being reset during anything but actually being idle,\n"
	"; as in ready to perform a 5P on the ground and having no throw protection on top.")
	
settingsField(bool, dontResetRISCWhenInBurstOrFaint, false,
	"Don't Reset RISC When In Burst Or Faint", SETTINGS_GENERAL,
	"; Specify true or false.\n"
	"; Only for Training Mode.\n"
	"; By default, when this is false (the default), the game will reset the RISC Gauge\n"
	"; of a player when that player is not in hitstun, not in blockstun and does not have\n"
	"; throw protection.\n"
	"; Unfortunately, the default rules reset RISC during Burst and Faint animations,\n"
	"; because they are not hitstun.\n"
	"; The RISC Gauge resets to the value specified in Training Mode Pause Menu - R.I.S.C. Level.\n"
	"; Setting this to true prevents the RISC Gauge from being reset during anything but actually being idle,\n"
	"; as in ready to perform a 5P or j.P and having no throw protection on top.")
	
settingsField(bool, onlyApplyCounterhitSettingWhenDefenderNotInBurstOrFaintOrHitstun, false,
	"Only Apply 'Counter Hit' Training Setting When Dummy Is Not In Burst Or Faint Or Hitstun", SETTINGS_GENERAL,
	"; Specify true or false.\n"
	"; Only for Training Mode.\n"
	"; By default, when this is false (the default), the game will apply the 'Counter Hit'\n"
	"; Training Mode setting only when the player or dummy were hit when not already being in hitstun.\n"
	"; Unfortunately, the default rules apply Counter Hit during Burst and Faint animations,\n"
	"; because they are not hitstun.\n"
	"; Setting this to true prevents the 'Counter Hit' setting from working explicitly just for Burst\n"
	"; and Faint animations, including all hitstun.")
	
settingsKeyCombo(hitboxEditModeToggle, "Hitbox Edit Mode Toggle", "",
	"; A keyboard shortcut.\n"
	"; Pressing this shortcut will enable/disable the hitbox editing mode.\n"
	"; Note that the 'Hitbox Editor' window is accessible through the 'Hitbox Editor' button in the 'Hitboxes' section,\n"
	"; and the main functionality is in Hitbox Editor.")
	
settingsKeyCombo(hitboxEditMoveCameraUp, "Move Camera Up", "-Shift+-Ctrl+MouseWheelUp",
	"; A keyboard shortcut.\n"
	"; Pressing this shortcut will move the camera up during the hitbox editing mode.\n"
	"; You can adjust the speed with which the camera moves vertically using \"hitboxEditMoveCameraVerticalSpeedMultiplier\".")
	
settingsKeyCombo(hitboxEditMoveCameraDown, "Move Camera Down", "-Shift+-Ctrl+MouseWheelDown",
	"; A keyboard shortcut.\n"
	"; Pressing this shortcut will move the camera down during the hitbox editing mode.\n"
	"; You can adjust the speed with which the camera moves vertically using \"hitboxEditMoveCameraVerticalSpeedMultiplier\".")
	
settingsKeyCombo(hitboxEditMoveCameraLeft, "Move Camera Left", "Shift+MouseWheelUp",
	"; A keyboard shortcut.\n"
	"; Pressing this shortcut will move the camera left during the hitbox editing mode.\n"
	"; You can adjust the speed with which the camera moves horizontally using \"hitboxEditMoveCameraHorizontalSpeedMultiplier\".")
	
settingsKeyCombo(hitboxEditMoveCameraRight, "Move Camera Right", "Shift+MouseWheelDown",
	"; A keyboard shortcut.\n"
	"; Pressing this shortcut will move the camera right during the hitbox editing mode.\n"
	"; You can adjust the speed with which the camera moves horizontally using \"hitboxEditMoveCameraHorizontalSpeedMultiplier\".")
	
settingsKeyCombo(hitboxEditMoveCameraForward, "Move Camera Forward", "Ctrl+MouseWheelUp",
	"; A keyboard shortcut.\n"
	"; Pressing this shortcut will move the camera forward during the hitbox editing mode.\n"
	"; You can adjust the speed with which the camera moves forward and back using \"hitboxEditMoveCameraPerpendicularSpeedMultiplier\".")
	
settingsKeyCombo(hitboxEditMoveCameraBack, "Move Camera Back", "Ctrl+MouseWheelDown",
	"; A keyboard shortcut.\n"
	"; Pressing this shortcut will move the camera back during the hitbox editing mode.\n"
	"; You can adjust the speed with which the camera moves forward and back using \"hitboxEditMoveCameraPerpendicularSpeedMultiplier\".")
	
settingsField(float, hitboxEditMoveCameraVerticalSpeedMultiplier, 1.F,
	"Move Camera Vertical Speed Multiplier", SETTINGS_HITBOX_EDITOR,
	"; A floating point value. Default value is 1.0.\n"
	"; Allows you to adjust how fast the camera moves vertically during the hitbox edit mode.\n"
	"; The camera can be moved up/down using \"hitboxEditMoveCameraUp\" and \"hitboxEditMoveCameraDown\".")
	
settingsField(float, hitboxEditMoveCameraHorizontalSpeedMultiplier, 1.F,
	"Move Camera Horizontal Speed Multiplier", SETTINGS_HITBOX_EDITOR,
	"; A floating point value. Default value is 1.0.\n"
	"; Allows you to adjust how fast the camera moves horizontally during the hitbox edit mode.\n"
	"; The camera can be moved left/right using \"hitboxEditMoveCameraLeft\" and \"hitboxEditMoveCameraRight\".")
	
settingsField(float, hitboxEditMoveCameraPerpendicularSpeedMultiplier, 2.F,
	"Move Camera Perpendicular Speed Multiplier", SETTINGS_HITBOX_EDITOR,
	"; A floating point value. Default value is 1.0.\n"
	"; Allows you to adjust how fast the camera moves farther away from or closer to the arena during the hitbox edit mode.\n"
	"; The camera can be moved left/right using \"hitboxEditMoveCameraForward\" and \"hitboxEditMoveCameraBack\".")
	
settingsField(bool, hitboxEditUnfreezeGameWhenLeavingEditMode, true,
	"Unfreeze Game When Leaving Edit Mode", SETTINGS_HITBOX_EDITOR,
	"; Specify true or false.\n"
	"; Specifying true means that, when you stop editing hitboxes using the 'Hitbox Editor' window,\n"
	"; or exit the edit mode via a shortcut, you will unfreeze the game, if it's currently frozen.\n"
	"; You can manually freeze and unfreeze the game using the controls in Main Mod UI - Hitboxes or\n"
	"; \"freezeGameToggle\".")
	
settingsField(bool, hitboxEditShowHitboxesOfEntitiesOtherThanTheOneBeingEdited, true,
	"Show Hitboxes Of Entities Other Than The One Being Edited", SETTINGS_HITBOX_EDITOR,
	"; Specify true or false.\n"
	"; Specifying true means that, when you're in hitbox editing mode,\n"
	"; you will see hitboxes of all entities on the arena.\n"
	"; When this is false, you will only see hitboxes of the entity you're currently editing.")
	
settingsField(HitboxList, hitboxList,
		// 0xAARRGGBB Show (true/false). Can't use commas because preprocessor doesn't understand that they're inside curly braces
		/* HURTBOX */           "0xFF00FF00 1 "
		/* HITBOX */            "0xFFFF0000 1 "
		/* EX_POINT */          "0xFFFFFFFF 0 "
		/* EX_POINT_EXTENDED */ "0xFFFFFFFF 0 "
		/* TYPE4 */             "0xFFFFFFFF 0 "
		/* PUSHBOX */           "0xFFFFFF00 1 "
		/* TYPE6 */             "0xFFFFFFFF 0 "
		/* NECK */              "0xFFFFFFFF 0 "
		/* ABDOMEN */           "0xFFFFFFFF 0 "
		/* R_LEG */             "0xFFFFFFFF 0 "
		/* L_LEG */             "0xFFFFFFFF 0 "
		/* PRIVATE0 */          "0xFFFFFFFF 0 "
		/* PRIVATE1 */          "0xFFFFFFFF 0 "
		/* PRIVATE2 */          "0xFFFFFFFF 0 "
		/* PRIVATE3 */          "0xFFFFFFFF 0 "
		/* TYPE15 */            "0xFFFFFFFF 0 "
		/* TYPE16 */            "0xFFFFFFFF 0 ",
	"Hitbox List", SETTINGS_HITBOX_EDITOR,
	"; Specify a list of hitboxes, sorted by type. You're not allowed to skip types.\n"
	"; There are 17 types total. For each hitbox specify color in 0xAARRGGBB and whether it should\n"
	"; be displayed (0/1). Separate everything with spaces.")
	
settingsField(bool, hitboxEditShowFloorline, true,
	"Show Floorline", SETTINGS_HITBOX_EDITOR,
	"; Specify true or false.\n"
	"; Specifying true will draw a white line denoting the floor level during hitbox editing mode.")
	
settingsField(bool, hitboxEditShowOriginPoints, true,
	"Show Origin Points", SETTINGS_HITBOX_EDITOR,
	"; Specify true or false.\n"
	"; Specifying true will draw origin points of both player during hitbox editing mode.")
	
settingsField(bool, showPunishMessageOnBlock, false,
	"Show 'Punish' Message When Punishing After A Block/Hit/Armor", SETTINGS_GENERAL,
	"; Specify true or false.\n"
	"; Shows a message when you blocked/got hit by/armored an attack and then at that moment opponent was doing a move,\n"
	"; and then you hit them while they were still in that move or in landing recovery after the move.")
	
settingsField(bool, showPunishMessageOnWhiff, false,
	"Show 'Punish' Message When Punishing Presumed Recovery", SETTINGS_GENERAL,
	"; Specify true or false.\n"
	"; Shows a message when a player got hit during recovery of an attack move, or during\n"
	"; landing animation of any move, or after 15 frames of starting a move.")
	
settingsKeyCombo(hitboxEditAddSpriteHotkey, "Add Sprite", "",
	"; A keyboard shortcut.\n"
	"; Pressing this shortcut will add a new sprite when inside the hitbox editor and hitbox editing mode is active.")
	
settingsKeyCombo(hitboxEditDeleteSpriteHotkey, "Delete Sprite", "",
	"; A keyboard shortcut.\n"
	"; Pressing this shortcut will delete the current sprite when inside the hitbox editor and hitbox editing mode is active.")
	
settingsKeyCombo(hitboxEditRenameSpriteHotkey, "Rename Sprite", "",
	"; A keyboard shortcut.\n"
	"; Pressing this shortcut will prompt to rename the current sprite when inside the hitbox editor and hitbox editing mode is active.")
	
settingsKeyCombo(hitboxEditAddHitboxHotkey, "Add Hitbox", "",
	"; A keyboard shortcut.\n"
	"; Pressing this shortcut will turn on the hitbox addition mode, and when you drag your mouse across the stage,\n"
	"; it will create a new hitbox of the type specified in the 'Hitbox Type' field.\n"
	"; If you don't drag and just click instead, you will create a point-sized hitbox.\n"
	"; This all only works inside the hitbox editor when hitbox editing mode is active (hitbox editor window need not be visible).")
	
settingsKeyCombo(hitboxEditDeleteSelectedHitboxesHotkey, "Delete Selected Hitboxes", "",
	"; A keyboard shortcut.\n"
	"; Pressing this shortcut will delete the currently selected hitboxes when inside the hitbox editor and hitbox editing mode is active\n"
	"; (hitbox editor window need not be visible).")
	
settingsKeyCombo(hitboxEditSelectionToolHotkey, "Selection Tool", "",
	"; A keyboard shortcut.\n"
	"; Pressing this shortcut will turn on the selection tool mode, and when you hover your mouse over hitboxes,\n"
	"; the cursor will change, and you'll be able to select, box-select and move and resize hitboxes.\n"
	"; This all only works inside the hitbox editor when hitbox editing mode is active (hitbox editor window need not be visible).")
	
settingsKeyCombo(hitboxEditUndoHotkey, "Undo", "Ctrl+Z",
	"; A keyboard shortcut.\n"
	"; Pressing this shortcut will undo the last hitbox or sprite editing operation.\n"
	"; This all only works inside the hitbox editor when hitbox editing mode is active (hitbox editor window need not be visible).")
	
settingsKeyCombo(hitboxEditRedoHotkey, "Redo", "Ctrl+Y",
	"; A keyboard shortcut.\n"
	"; Pressing this shortcut will redo the last undone hitbox or sprite editing operation.\n"
	"; If you have not yet undone any operation, or you did a new operation after last undoing one, redo is impossible.\n"
	"; This all only works inside the hitbox editor when hitbox editing mode is active (hitbox editor window need not be visible).")
	
settingsField(bool, hitboxEditZoomOntoMouseCursor, true,
	"Zoom Onto/Away From Mouse Cursor", SETTINGS_HITBOX_EDITOR,
	"; Specify true or false.\n"
	"; When using some hotkey to zoom the camera in/out during hitbox editing mode,\n"
	"; the camera will be scaled around the current position of the mouse cursor.")
	
settingsKeyCombo(hitboxEditMoveHitboxesUp, "Move Hitboxes Up", "-Ctrl+-Shift+Up",
	"; A keyboard shortcut.\n"
	"; Pressing this shortcut will move the currently selected hitboxes up by a normal amount\n"
	"; (specified in \"hitboxEditMoveHitboxesNormalAmount\").\n"
	"; This only works inside the hitbox editor when hitbox editing mode is active (hitbox editor window need not be visible).")
	
settingsKeyCombo(hitboxEditMoveHitboxesDown, "Move Hitboxes Down", "-Ctrl+-Shift+Down",
	"; A keyboard shortcut.\n"
	"; Pressing this shortcut will move the currently selected hitboxes down by a normal amount\n"
	"; (specified in \"hitboxEditMoveHitboxesNormalAmount\").\n"
	"; This only works inside the hitbox editor when hitbox editing mode is active (hitbox editor window need not be visible).")
	
settingsKeyCombo(hitboxEditMoveHitboxesLeft, "Move Hitboxes Left", "-Ctrl+-Shift+Left",
	"; A keyboard shortcut.\n"
	"; Pressing this shortcut will move the currently selected hitboxes left by a normal amount\n"
	"; (specified in \"hitboxEditMoveHitboxesNormalAmount\").\n"
	"; This only works inside the hitbox editor when hitbox editing mode is active (hitbox editor window need not be visible).")
	
settingsKeyCombo(hitboxEditMoveHitboxesRight, "Move Hitboxes Right", "-Ctrl+-Shift+Right",
	"; A keyboard shortcut.\n"
	"; Pressing this shortcut will move the currently selected hitboxes right by a normal amount\n"
	"; (specified in \"hitboxEditMoveHitboxesNormalAmount\").\n"
	"; This only works inside the hitbox editor when hitbox editing mode is active (hitbox editor window need not be visible).")
	
settingsField(int, hitboxEditMoveHitboxesNormalAmount, 1000,
	"Move Hitboxes 'Normal Amount'", SETTINGS_HITBOX_EDITOR,
	"; Specify a number. Default value is 1000.\n"
	"; This controls how much the hitboxes are moved by the \"hitboxEditMoveHitboxesUp\",\n"
	"; \"hitboxEditMoveHitboxesDown\", \"hitboxEditMoveHitboxesLeft\", \"hitboxEditMoveHitboxesRight\" controls.")
	
settingsKeyCombo(hitboxEditMoveHitboxesALotUp, "Move Hitboxes A Lot Up", "-Ctrl+Shift+Up",
	"; A keyboard shortcut.\n"
	"; Pressing this shortcut will move the currently selected hitboxes up by a large amount\n"
	"; (specified in \"hitboxEditMoveHitboxesLargeAmount\").\n"
	"; This only works inside the hitbox editor when hitbox editing mode is active (hitbox editor window need not be visible).")
	
settingsKeyCombo(hitboxEditMoveHitboxesALotDown, "Move Hitboxes A Lot Down", "-Ctrl+Shift+Down",
	"; A keyboard shortcut.\n"
	"; Pressing this shortcut will move the currently selected hitboxes down by a large amount\n"
	"; (specified in \"hitboxEditMoveHitboxesLargeAmount\").\n"
	"; This only works inside the hitbox editor when hitbox editing mode is active (hitbox editor window need not be visible).")
	
settingsKeyCombo(hitboxEditMoveHitboxesALotLeft, "Move Hitboxes A Lot Left", "-Ctrl+Shift+Left",
	"; A keyboard shortcut.\n"
	"; Pressing this shortcut will move the currently selected hitboxes left by a large amount\n"
	"; (specified in \"hitboxEditMoveHitboxesLargeAmount\").\n"
	"; This only works inside the hitbox editor when hitbox editing mode is active (hitbox editor window need not be visible).")
	
settingsKeyCombo(hitboxEditMoveHitboxesALotRight, "Move Hitboxes A Lot Right", "-Ctrl+Shift+Right",
	"; A keyboard shortcut.\n"
	"; Pressing this shortcut will move the currently selected hitboxes right by a large amount\n"
	"; (specified in \"hitboxEditMoveHitboxesLargeAmount\").\n"
	"; This only works inside the hitbox editor when hitbox editing mode is active (hitbox editor window need not be visible).")
	
settingsField(int, hitboxEditMoveHitboxesLargeAmount, 10000,
	"Move Hitboxes 'Large Amount'", SETTINGS_HITBOX_EDITOR,
	"; Specify a number. Default value is 10000.\n"
	"; This controls how much the hitboxes are moved by the \"hitboxEditMoveHitboxesALotUp\",\n"
	"; \"hitboxEditMoveHitboxesALotDown\", \"hitboxEditMoveHitboxesALotLeft\", \"hitboxEditMoveHitboxesALotRight\" controls.")
	
settingsField(bool, hitboxEditDisplayRawCoordinates, false,
	"Display Raw Coordinates", SETTINGS_HITBOX_EDITOR,
	"; Specify true or false.\n"
	"; This controls the four fields that are located on the bottom right of the Hitbox Editor:\n"
	"; the 'Left', 'Top', 'Right' and 'Bottom' fields. When this setting is true, those fields show\n"
	"; 'X', 'Y', 'Width', 'Height' instead, and the coordinates are in the internal coordinate system\n"
	"; of the hitbox data.\n"
	"; When this setting is false, those fields show 'Left', 'Top', 'Right', 'Bottom' and show arena\n"
	"; coodinates, which are usually multipled by 1000, and if the projectile or player is rotated\n"
	"; or scaled, the coordinates will reflect that.")
	
settingsKeyCombo(hitboxEditArrangeHitboxesToBack, "Arrange Hitboxes To Back", "",
	"; A keyboard shortcut.\n"
	"; Pressing this shortcut will move the currently selected hitboxes as far back as possible in Z-order.\n"
	"; This makes them display behind all the other boxes.\n"
	"; This only works inside the hitbox editor when hitbox editing mode is active (hitbox editor window need not be visible).")
	
settingsKeyCombo(hitboxEditArrangeHitboxesBackwards, "Arrange Hitboxes Backward", "",
	"; A keyboard shortcut.\n"
	"; Pressing this shortcut will move the currently selected hitboxes back in Z-order.\n"
	"; This makes them display behind other boxes.\n"
	"; This only works inside the hitbox editor when hitbox editing mode is active (hitbox editor window need not be visible).")
	
settingsKeyCombo(hitboxEditArrangeHitboxesUpwards, "Arrange Hitboxes Forward", "",
	"; A keyboard shortcut.\n"
	"; Pressing this shortcut will move the currently selected hitboxes forward in Z-order.\n"
	"; This makes them display in front of other boxes.\n"
	"; This only works inside the hitbox editor when hitbox editing mode is active (hitbox editor window need not be visible).")
	
settingsKeyCombo(hitboxEditArrangeHitboxesToFront, "Arrange Hitboxes To Front", "",
	"; A keyboard shortcut.\n"
	"; Pressing this shortcut will move the currently selected hitboxes as far forward as possible in Z-order.\n"
	"; This makes them display in front of all the other boxes.\n"
	"; This only works inside the hitbox editor when hitbox editing mode is active (hitbox editor window need not be visible).")
	
settingsField(bool, enableScriptMods, false,
	"Enable Script Mods", SETTINGS_MODDING,
	"; Specify true or false.\n"
	"; This controls whether custom hitboxes (.collision files) and bbscript (.bbscript files)\n"
	"; will be loaded on battle initialization from the game's Mods folder\n"
	"; (GUILTY GEAR Xrd -REVELATOR-\\Mods).")
	
settingsKeyCombo(fastForwardReplay, "Fast Forward Replay", "",
	"; A keyboard shortcut.\n"
	"; Holding down this key during a replay will speed up the playback by a factor specified in\n"
	"; \"fastForwardReplayFactor\".\n"
	"; This feature causes danger time entry countdown to go out of sync.")
	
settingsField(int, fastForwardReplayFactor, 2,
	"Fast Forward Replay Factor", SETTINGS_GENERAL,
	"; Specify a positive whole number. Default value is 2.\n"
	"; When holding the \"fastForwardReplay\" hotkey during a replay,\n"
	"; this is by how many times to speed up the replay.")
	
settingsKeyCombo(openQuickCharSelect, "Open Quick Character Select", "",
	"; A keyboard shortcut to open the Quick Character Select window.")
	
settingsField(bool, useControllerFriendlyQuickCharSelect, true,
	"Use Controller-Friendly Quick Char Select", SETTINGS_GENERAL,
	"; Specify true or false. Default value is true.\n"
	"; Setting this to true will change the appearance of the character select window,\n"
	"; and make it automatically close when the hotkey that was used to open it\n"
	"; is released, confirming the current selection.")
	
settingsField(bool, enableMouseInControllerFriendlyQuickCharSelect, true,
	"Enable Mouse In Controller-Friendly Quick Char Select", SETTINGS_GENERAL,
	"; Specify true or false. Default value is true.\n"
	"; Setting this to true will allow you to use the compuer mouse to move the cursor\n"
	"; in the 'Controller-Friendly' version of the 'Quick Character Select' (that you\n"
	"; can turn on using the \"useControllerFriendlyQuickCharSelect\" setting).")
	
settingsKeyCombo(quickCharSelect_moveLeft, "Quick Character Select - Move Left", "LeftStickLeft",
	"; A keyboard shortcut to move the cursor left in the Quick Character Select window.\n"
	"; More settings can be found in Main Mod UI - Quick Character Select.")
	
settingsKeyCombo(quickCharSelect_moveUp, "Quick Character Select - Move Up", "LeftStickUp",
	"; A keyboard shortcut to move the cursor up in the Quick Character Select window.\n"
	"; More settings can be found in Main Mod UI - Quick Character Select.")
	
settingsKeyCombo(quickCharSelect_moveRight, "Quick Character Select - Move Right", "LeftStickRight",
	"; A keyboard shortcut to move the cursor right in the Quick Character Select window.\n"
	"; More settings can be found in Main Mod UI - Quick Character Select.")
	
settingsKeyCombo(quickCharSelect_moveDown, "Quick Character Select - Move Down", "LeftStickDown",
	"; A keyboard shortcut to move the cursor down in the Quick Character Select window.\n"
	"; More settings can be found in Main Mod UI - Quick Character Select.")
	
settingsKeyCombo(quickCharSelect_moveLeft_2, "Quick Character Select - Move Left (2)", "DPadLeft",
	"; An alternative keyboard shortcut to move the cursor left in the Quick Character Select window.\n"
	"; More settings can be found in Main Mod UI - Quick Character Select.")
	
settingsKeyCombo(quickCharSelect_moveUp_2, "Quick Character Select - Move Up (2)", "DPadUp",
	"; An alternative keyboard shortcut to move the cursor up in the Quick Character Select window.\n"
	"; More settings can be found in Main Mod UI - Quick Character Select.")
	
settingsKeyCombo(quickCharSelect_moveRight_2, "Quick Character Select - Move Right (2)", "DPadRight",
	"; An alternative keyboard shortcut to move the cursor right in the Quick Character Select window.\n"
	"; More settings can be found in Main Mod UI - Quick Character Select.")
	
settingsKeyCombo(quickCharSelect_moveDown_2, "Quick Character Select - Move Down (2)", "DPadDown",
	"; An alternative keyboard shortcut to move the cursor down in the Quick Character Select window.\n"
	"; More settings can be found in Main Mod UI - Quick Character Select.")
	
#pragma warning(pop)