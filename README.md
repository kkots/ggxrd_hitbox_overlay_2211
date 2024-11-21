# ggxrd_hitbox_overlay_2211

![Screenshot can't be viewed](screen.jpg)

## Description

Adds hitboxes overlaid on top of characters/projectiles for Guilty Gear Xrd Rev2 version 2211 (as of 3'rd January 2024).  
Also can freeze the game and play it frame-by-frame (with box display turned off for example).  
Also can screenshot the game with transparency enabled/disabled (made with help from WorseThanYou (visit his website! <https://worsety.github.io/>)).

## Credits

Original version created by Altimor: <http://www.dustloop.com/forums/index.php?/forums/topic/12495-xrd-pc-hitbox-overlay-mod/>  
Created in 2016.  
This version is adapted for Guilty Gear Xrd Rev2 version 2211 with the full featureset of the original.

Many thanks to WorseThanYou (@worsety), without whose help this wouldn't have been possible.  
Thanks to @PCvolt for advice on framebar.

## System requirements

Intel processor architecture x86 (32-bit) or x64 (64-bit) (AMD will work). Windows operating system.  
For Ubuntu/Linux you need to be running the Windows version of Guilty Gear Xrd (I'm not aware of a Linux version existing) under Steam Proton.

## Quickstart for Windows

1. Launch the game. Or you can do the next step at any point while the game is running.

2. Go to downloaded folder and launch `ggxrd_hitbox_injector.exe`.

3. Start a match. Hitboxes should display.

Read the `Features` section to understand what the colors mean and what the hotkeys are.  
You can also play the game frame-by-frame (described in `Features`).

To turn off the mod you can launch `ggxrd_hitbox_injector.exe` again.  
If trying to use the mod with a game version that it doesn't fit, the game may crash. The mod should be possible to combine with other mods, but there might be some mods that can't be combined with this one (if they hook or sigscan the same functions).

The mod may show up as a virus. I swear this is not a virus, check the source code, compile this yourself if still doubting. Check commit history of this repo. Add this to whatever antivirus exceptions necessary and run as administrator if necessary.

## Quickstart for Ubuntu/Linux

On Ubuntu/Linux the injector won't work on its own. You need to cd into the mod's directory and type into the console:

```bash
.\launch_ggxrd_hitbox_injector_linux.sh
```

This will launch the injector (the .exe one) on the same Wine server that Guilty Gear runs on. Assuming you're running the game using Steam Proton, and its version is supported, everything should work.  
If it doesn't work, you can use the patcher:

## Patching the game to always launch with the mod

If you patch the game it will always load the DLL automatically on startup. The `ggxrd_hitbox_patcher_linux` and `ggxrd_hitbox_patcher` do exactly that (on Linux and Windows respectively) and must be launched directly, without Wine. The patcher on Ubuntu/Linux, when it asks, must be provided the full path to the game executable (`GuiltyGearXrd.exe`) without quotes. The Windows patcher will show a file selection dialog instead.  
The patched game will now try to load the `ggxrd_hitbox_overlay.dll` on startup. In order for the game to find the DLL it must be placed in the same directory as the game executable, which should be in Steam's directory, for example: `~/.steam/debian-installation/steamapps/common/GUILTY GEAR Xrd -REVELATOR-/Binaries/Win32`, where `~` is your home directory.  
If the DLL is not found when the game launches, it will just run normally, without the mod.  
Normally you can run the injector to unload the mod, but if for whatever reason you can't run on Linux, then there's no way to unload the mod. To solve this you can add the `.ini` file mentioned in `Hotkey configuration` section into the folder with the game executable and change the line `startDisabled = false` to `startDisabled = true` in it and use the `F6` (the default) hotkey to enable the mod when you need.

If Steam Proton is taking forever to launch the game, patched or not, then rename the mod's dll and put the original, unpatched version of the game executable back, then try switching the Proton version in Steam's settings and agree to restart Steam. After restarting Steam will start downloading two things related to Guilty Gear Xrd and will launch GGXrd automatically (even though you didn't tell it to). GGXrd should work. Quit GGXrd. Place the patched version of the game executable and the mod's dll back and restart GGXrd. GGXrd will launch with the mod. Idk what causes this.

## Features

### Red - Hitboxes

Strike and projectile non-throw hitboxes are shown in red. Clash-only hitboxes are shown in more transparent red with thinner outline. Hitboxes' fills may never be absent.

### Green - Hurtboxes

Normally hurtboxes display in green. The rules in general are such, that when a hitbox (red) makes contact with hurtbox, a hit occurs.  
If a hurtbox is displayed fully transparent (i.e. shows outline only), that means strike invulnerability.

### Light blue - Would-be counterhit hurtboxes

If your hurtbox is displaying light blue, that means, should you get hit, you would enter counterhit state. It means that moves that have light blue hurtbox on recovery are more punishable.

### Gray - Pre-hit hurtboxes

When you get hit a gray outline appears on top of your current hurtbox. This outline represents the previous state of your hurtbox, before you got hit. Its purpose is to make it easier to see how or why you got hit.

### Yellow - Pushboxes

Each player has a pushbox. When two pushboxes collide, the players get pushed apart until their pushboxes no longer collide. Pushbox widths also affect throw range - more on that in next section(s).  
If a pushbox is displayed fully transparent (i.e. shows outline only), that means throw invulnerability.  
Pushbox outline is always thin.

### Point/Cross - Origin points

Each player and entity has an origin point which is shown as a black-white cross on the ground between their feet. When players jump, the origin point tracks their location. Origin points play a key role in throw hit detection.

### Blue - Rejection boxes

When a Blitz Shield meets a projectile it displays a square blue box around the rejecting player, and if the opponent's origin point is within that box the opponent enters rejected state. The box does not show when rejecting normal, melee attacks because those cause rejection no matter the distance. Pushboxes and their sizes do not affect the distance check in any way, i.e. only the X and Y distances between the players' origin points are checked.

### Blue - Throw box pushbox check (when drawPushboxCheckSeparately = true - the default)

When a player does a throw he displays a throw box which may either be represented as one blue box, one purple box or one blue and one purple box. Throw boxes are usually only active for one frame (that's when they display semi-transparent). This period is so brief throw boxes have to show for a few extra frames, but during those frames they're no longer active and so they display fully transparent (outline only).

The blue part of the box shows the pushbox-checking throw box. It is never limited vertically, because it only checks the horizontal distance between pushboxes. The rule is if this blue throw box touches the yellow pushbox of the opponent, the pushbox-checking part of the throw is satisfied.  
But there may be another part of the throw that checks the X and/or Y of the opponent's origin point (shown in purple). If such a part of the throw box is present, it must also be satisfied, or else the throw won't connect.

This mode of showing throw boxes shows them like this:

![Screenshot can't be viewed](throw_box_drawPushboxCheckSeparately_true.jpg)

### Purple - Throw box origin point check (when drawPushboxCheckSeparately = true - the default)

The purple part of the throw box displays the region where the opponent's origin point must be in order for this part of the throw check to connect.  
If there's a pushbox-proximity-checking part of the throw (blue), then satisfying the origin point check (purple) is not enough - the pushbox check must be passed as well. The pushbox-checking part of the throw displays in blue and is described in the section above.

### Blue - Throw boxes (when drawPushboxCheckSeparately = false)

There's an alternative mode in the mod to show the part of the throw box that checks pushbox proximity (blue) and the part of the throw box that checks the X and Y of the opponent's origin point (purple) as a single blue throw box which only checks the opponent's origin point. To turn this mode on place the `.ini` file described in `Hotkey configuration` into the folder with the game executable and change `drawPushboxCheckSeparately = true` to be `drawPushboxCheckSeparately = false`. (Reloading the mod is unnecessary.)

In this mode, when a player does a throw he displays a throw box in blue color. Throw boxes are usually only active for one frame (that's when they display semi-transparent). This period is so brief throw boxes have to show for a few extra frames, but during those frames they're no longer active and so they display fully transparent (outline only).

In this mode, throw boxes must only include the opponent's origin point in order to connect (but that's not all, read on). That means that the sizes of your and your opponent's pushboxes affect the width of your throw box. Try crouching and see how your pushbox becomes wider a bit. This means crouching opponents are slightly easier to throw. Or Potemkin - he has a wider pushbox than average.

It's in the rules of the game that ground throws may only connect with non-airborne opponents and air throws may only connect with airborne opponents. Opponents who are in prejump state cannot be ground thrown.  
Most normal ground throws and ground command throws for this reason do not limit their throw boxes vertically: the throw box shows as a pillar reaching vertically over the entire screen's height. This doesn't mean they capture anything above or below the thrower, though, and the rules mentioned above are still being obeyed.

Some throws check for vertical position of the opponent's origin point and so display their throw box limited in size vertically.  
When visually checking to see if a throw box would connect with an opponent you should, in this mod, ignore the pushboxes altogether and focus only on the throw box catching the origin point.  
Note that normal ground throw actually simply checks if distance between the pushboxes is below the attacker's throw range (values listed on Dustloop), however some throws like command throws or air throws also check if the origin point specifically is within x or y range. Hence this is why this mode exists to just always show the check on the origin point only.

This mode of showing throw boxes shows them like this:

![Screenshot can't be viewed](throw_box_drawPushboxCheckSeparately_false.jpg)

### Outlines lie within their boxes/on the edge

If a box's outline is thick, it lies within that box's bounds, meaning that two boxes intersect if either their fills or outlines touch or both. This is relevant for throwboxes too.  
If a box's outline is thin, like that of a pushbox or a clash-only hitbox for example, then that outline lies on the edge of the box.

### Projectile-only invul

Some non-parry/reflect moves in the game have invulnerability that only protects the user from projectiles. There are only 2 such moves: Ky's Ride the Lightning and Jack O's Aegis Field. The projectile invulnerability lasts all the way until the red clash-only hitbox disappears, so there's no separate display for it.

### General notes about throw boxes

For some command throws such as Sol's Wild Throw a purple box is displayed, which supposedly checks the Y of the origin point of the opponent. This doesn't really make sense since Wild Throw only connects on grounded opponents. If anyone has information on this, I would gladly listen.

If a command throw has a throw box as well as hitbox, such as Raven's command throw, - for such moves I haven't fully studied the conditions under which they connect - but it's likely that the hitbox doesn't matter and only the throw box has to connect, since it was already proven that Potemkin Buster's hitbox does not matter - only the throw box (proof: <https://youtu.be/59uc9h6KKIE>).

### Frame-by-frame animation playback

You can force the game to play one frame at a time (in training mode only). Read on in `F3 - Freeze game` section and sections after that.

### F1 - GIF mode

In training mode (only) you can press F1 to enter "GIF mode", which makes the background black, centers the camera on you, hides all of the HUD and makes opponent invisible and unhittable.  
Press the key again to turn off the mode.  
GIF mode can be broken down into separate toggles for each of its functionalities.  
Section "Hotkey configuration" describes how to configure hotkeys.

### F2 - No gravity mode

In training mode (only) you can press F2 to enter "No gravity mode" which makes your vertical speed always 0, i.e. you become unable to fall. This may be useful for screenshotting some air moves.  
Press the key again to turn off the mode.  
Section "Hotkey configuration" describes how to configure hotkeys.

### F3 - Freeze game

Freezes the game and stops animations and game logic from advancing.  
Section "Hotkey configuration" describes how to configure hotkeys.

### F4 - Slow-motion mode

Plays the game at 3 times (the default) slower rate. You configure the rate in settings (read on in `Hotkey configuration`), but the `slowmoTimes` must be a whole, round number greater than 1. I.e. the game can only be slowed down 2, 3, 4, etc times.  
Section "Hotkey configuration" describes how to configure hotkeys.

### F5 - Advance to next frame

While the game is frozen using `Freeze game` feature, advances the game forward by 1 frame. Does nothing if the game is not currently frozen.  
Section "Hotkey configuration" describes how to configure hotkeys.

### F6 - Disable mod

This toggle enables/disables the mod if you don't want to (or can't) load/unload it every time.  
There's also a setting named `startDisabled = false/true` which is a boolean which tells the mod to either be disabled or enabled right when it starts, so that you could start the game with the mod disabled initially.

### F7 - Toggle hitboxes display

Since the `Disable mod` toggle disables the whole mod, this toggle only disables the hitbox drawing, so that GIF mode, No gravity mode, freeze game mode, etc may still work, only the boxes don't display.

### F8 - Take transparent/non-transparent screenshot

This is a big section that is described in `Taking transparent/non-transparent screenshots` section. Basically this is a button to take a screenshot of the game with transparency enabled/disabled. Transparency only works under conditions described in that section.

### Hotkey configuration

If you wish to configure hotkeys for Gif mode and No gravity mode and other modes, you can use the mod's UI that you can see in the game's window when you load the mod, or you can create a text file named `ggxrd_hitbox_overlay.ini` and place it in the directory where the game executable is. For example, for me my Steam version of the game is located at `...\SteamLibrary\steamapps\common\GUILTY GEAR Xrd -REVELATOR-\Binaries\Win32`.  
Here's an example of the `.ini` file:

```ini
; Place this file into the game folder containing 'GuiltyGearXrd.exe' so that it gets seen by the mod. Allowed key names: Backspace, Tab, Enter, PauseBreak, CapsLock, Escape, Space, PageUp, PageDown, End, Home, Left, Up, Right, Down, PrintScreen, Insert, Delete, Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9, NumMultiply, NumAdd, NumSubtract, NumDecimal, NumDivide, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, NumLock, ScrollLock, Colon, Plus, Minus, Comma, Period, Slash, Tilde, OpenSquareBracket, Backslash, CloseSquareBracket, Quote, Backslash2, 0123456789, ABCDEFGHIJKLMNOPQRSTUVWXYZ, Shift, Ctrl, Alt.

; Key combinations can be specified by separating key names with '+' sign.
; You can assign same key to multiple features - it will toggle/set in motion all of them simultaneously.
; You don't need to reload the mod when you change this file - it re-reads this settings file automatically when it changes.

; All of these settings can be changed using the mod's UI which can be seen in the game if you press ESC (by default, see modWindowVisibilityToggle).

; A keyboard shortcut to toggle GIF mode.
; GIF mode is:
; 1) Background becomes black
; 2) Camera is centered on you
; 3) Opponent is invisible and invulnerable
; 4) Hide HUD
gifModeToggle =

; A keyboard shortcut to only toggle the "background becomes black" part of the gifModeToggle.
; Empty by default, which means no hotkey is assigned. Assign your desired hotkey manually here.
; This option can be combined with the other "only" options, by sharing the same key binding for example
gifModeToggleBackgroundOnly =

; A keyboard shortcut to only toggle the "Camera is centered on you" part of the gifModeToggle.
; Empty by default, which means no hotkey is assigned. Assign your desired hotkey manually here.
; This option can be combined with the other "only" options, by sharing the same key binding for example
gifModeToggleCameraCenterOnly =

; A keyboard shortcut to toggle the camera to be centered on the opponent.
; Empty by default, which means no hotkey is assigned. Assign your desired hotkey manually here.
; This option can be combined with GIF Mode options, by sharing the same key binding for example
toggleCameraCenterOpponent =

; A keyboard shortcut to only toggle the "Opponent is invisible and invulnerable" part of the gifModeToggle.
; Empty by default, which means no hotkey is assigned. Assign your desired hotkey manually here.
; This option can be combined with the other "only" options, by sharing the same key binding for example
gifModeToggleHideOpponentOnly =

; A keyboard shortcut to toggle hiding the player.
; Empty by default, which means no hotkey is assigned. Assign your desired hotkey manually here.
; This option can be combined with GIF Mode options, by sharing the same key binding for example
toggleHidePlayer =

; A keyboard shortcut to only toggle the "hide hud" part of the gifModeToggle.
; Empty by default, which means no hotkey is assigned. Assign your desired hotkey manually here.
; This option can be combined with the other "only" options, by sharing the same key binding for example
gifModeToggleHudOnly =

; A keyboard shortcut to toggle No gravity mode
; No gravity mode is you can't fall basically
noGravityToggle = F2

; A keyboard shortcut to freeze the game
freezeGameToggle = F3

; A keyboard shortcut to play the game in slow motion.
; Please specify by how many times to slow the game down in "slowmoTimes"
slowmoGameToggle = F4

; A keyboard shortcut. Only works while the game is frozen using freezeGameToggle.
; Advances the game forward one frame
allowNextFrameKeyCombo = F5

; A number.
; This works in conjunction with slowmoGameToggle. Only round numbers greater than 1 allowed.
; Specifies by how many times to slow the game down
slowmoTimes = 3

; A keyboard shortcut to enable/disable the mod without having to load/unload it
disableModToggle = F6

; Specify true or false.
; When true, starts the mod in a disabled state: it doesn't draw boxes or affect anything
startDisabled = false

; A keyboard shortcut to enable/disable only the mod hitbox drawing feature:
; the GIF mode and no gravity, etc will keep working
disableHitboxDisplayToggle = F7

; A keyboard shortcut.
; Takes a screenshot and saves it at screenshotPath path
; To take screenshots over a transparent background you need to go to the game's
; Display Settings and turn off Post-Effects, then use GIF mode (make background dark).
; Then screenshots will film character over transparent background.
; If the dontUseScreenshotTransparency setting is true, screenshot will be without
; transparency anyway
screenshotBtn = F8

; A path to a file or a directory.
; It specifies where screenshots will be saved.
; If you provided a file path, it must be with .png extension, and if such name already exists, a
; number will be appended to it, increasing from 1 to infinity consecutively so that it's unique,
; so that new screenshots will never overwrite old ones.
; If you provided a directory path, it must already exist, and "screen.png" will be appended to
; it with an increasing number at the end in case the filename is not unique.
; The provided path must be without quotes.
; If you want the path to be multilingual you need to save this file in UTF-8.
; On Ubuntu/Linux running Guilty Gear Xrd under Steam Proton you need to specify paths with
; the Z:\ drive, path separator is backslash (\), not forward slash (/). Example: Z:\home\yourUserName\ggscreen.png
; If the path is not specified or is empty, the screenshot will be saved into your clipboard so
; it can be pasted into any image editing program. For example, GIMP will recognize the PNG
; format and paste that, with transparency. This would work even on Ubuntu/Linux.
; Only PNG format is supported.
screenshotPath = ;C:\Users\yourUser\Desktop\test screenshot name.png   don't forget to uncomment (; is a comment)

; Specify true or false.
; When this is true that means screenshots are being taken every game loop logical frame as
; long as the screenshotBtn is being held. Game loop logical frame means that if the game is
; paused or the actual animations are not playing for whatever reason, screenshot won't be taken.
; A new screenshot is only taken when animation frames change on the player characters.
; Be cautions not to run out of disk space if you're low. This option doesn't
; work if screenshotPath is empty, it's not allowed to work outside of training mode or when
; a match (training session) isn't currently running (for example on character selection screen).
allowContinuousScreenshotting = false

; A keyboard shortcut.
; This toggle can be used same way as screenshotBtn (when it's combined with
; allowContinuousScreenshotting = true), except it's a separate key combination and when you
; press it, it toggles the continuous screenshot taking every game logical frame. This
; toggle does not require allowContinuousScreenshotting to be set to true,
; or screenshotBtn to be set to anything.
continuousScreenshotToggle =

; Specify true or false.
; Setting this to true will produce screenshots without transparency
dontUseScreenshotTransparency = false

; Specify true or false.
; Setting this to true will make throw boxes show in an opponent-character-independent way:
; The part of the throw box that checks for pushboxes proximity will be shown in blue,
; while the part of the throw box that checks x or y of the origin point will be shown in purple
; Setting this to false will combine both the checks of the throw so that you only see the final box
; in blue which only checks the opponent's origin point. Be warned, such a throw box
; is affected by the width of the opponent's pushbox. Say, on Potemkin, for example,
; all ground throw ranges should be higher.
drawPushboxCheckSeparately = true

; Specify true or false.
; Setting this to true may increase the performance of transparent screenshotting which may be useful if you're screenshotting every frame.
; The produced screenshots won't have such improvements as improving visibility of semi-transparent effects or changing hitbox outlines to
; black when drawn over the same color.
useSimplePixelBlender = false

; A keyboard shortcut.
; Pressing this shortcut will show/hide the mod's UI window.
modWindowVisibilityToggle = Escape

; Specify true or false.
; If this is false, when this mod starts, the mod's UI window will be invisible.
modWindowVisibleOnStart = true

; A keyboard shortcut.
; Pressing this shortcut will disable/enable the display of residual hurtboxes that appear on hit/block and show
; the defender's hurtbox at the moment of impact. These hurtboxes display for only a brief time on impacts but
; they can get in the way when trying to do certain stuff such as take screenshots of hurtboxes.
toggleDisableGrayHurtboxes = 

; Specify true or false.
; This disables the display of gray hurtboxes (for a toggle see toggleDisableGrayHurtboxes).
; Gray hurtboxes are residual hurtboxes that appear on hit/block and show the defender's hurtbox at the moment of impact.
; These hurtboxes display for only a brief time on impacts but they can get in the way when trying to do certain stuff such
; as take screenshots of hurtboxes on hit/block.
neverDisplayGrayHurtboxes = false

; Specify true or false.
; Setting this to true will hide all hurtboxes, hitboxes, pushboxes and other boxes and points.
dontShowBoxes = false

; Specify true or false.
; Display mod's UI on top of the game's Pause Menu.
displayUIOnTopOfPauseMenu = false

; Specify true or false.
; The framebar is only shown when the UI is shown. If the UI is not shown the only way to show it is via the
; modWindowVisibilityToggle hotkey. By default it's ESC keyboard key, can be configured either using the INI file or the UI.
showFramebar = true

; A number.
; Specifies the height of a single framebar of one player, in pixels, including the black outlines on the outside.
; The standard height is 19.
framebarHeight = 19

; Specify true or false.
; Normally we don't display hitstop in the framebar if both players are in hitstop on that frame,
; unless a projectile or a blocking Baiken is present.
; If this is set to true, then we always show hitstop in the framebar.
neverIgnoreHitstop = false

; Specify true or false.
; Normally we consider running and walking as being idle, which does not advance the framebar forward.
; The framebar only advances when one of the players is "busy".
; If this is set to true, then one player running or walking will be treated same way as "busy" and will advance the framebar.
considerRunAndWalkNonIdle = true

; Specify true or false
; This controls whether a character being knocked down, waking up or air recovering causes the framebar to advance forward (if you're also idle).
; Framebar only advances forward when one or both players are not idle.
; Framebar advancing forward means it continuously overwrites its oldest contents with new data to display.
; This could be bad if you wanted to study why a combo dropped, as some knockdowns can be very long and erase all the info you wanted to see.
; Setting this to true may prevent that.
; The first frame when the opponent is in OTG state and onwards - those frames do not get included in the framebar.
; If you recover from your move later than the opponent enters OTG state, the frames are included for your whole recovery for both you and the opponent,
; which means OTG state may partially or fully get included into the framebar.
; In such cases, look for an animation start delimiter on the opponent's framebar, shown as a white ' between frames.
considerKnockdownWakeupAndAirtechIdle = false

; Specify true or false.
; If true, certain types of frames in the framebar will be displayed with distinct hatches on them.
; Make sure the framebar is scaled wide enough and the screen resolution is large enough that you can see the hatches properly.
; To scale the framebar you can drag its right edge.
useColorblindHelp = false

```

You can specify a combination of keys, separated by `+` sign. You can assign same key to multiple features - it will toggle/set in motion all of them simultaneously.  
Only the following key names are allowed: Backspace, Tab, Enter, PauseBreak, CapsLock, Escape, Space, PageUp, PageDown, End, Home, Left, Up, Right, Down, PrintScreen, Insert, Delete, Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9, NumMultiply, NumAdd, NumSubtract, NumDecimal, NumDivide, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, NumLock, ScrollLock, Colon, Plus, Minus, Comma, Period, Slash, Tilde, OpenSquareBracket, Backslash, CloseSquareBracket, Quote, Backslash2, 0123456789, ABCDEFGHIJKLMNOPQRSTUVWXYZ, Shift, Ctrl, Alt.

If the mod is already running you don't need to do anything in order to apply the new hotkeys and settings. The mod can reread the settings on the fly, without reloading the mod or restarting the game (this was tested to work even on Ubuntu/Linux running GGXrd under Steam Proton).

`slowmoTimes` is not a key combination, it must be a round integer number.

## Taking transparent/non-transparent screenshots

The mod allows you to take screenshots of the game with the transparency in the background, with characters overlaid on top without transparency. To achieve that, you need to go into the game's `Display settings` and set `Post-Effect` to `OFF`.

![Screenshot can't be viewed](posteffect_off.jpg)

Post-Effect set to Off seems to turn off anti-aliasing, but without it the trick won't work. Then you can load the mod and enter "GIF mode" (F1 is the default hotkey) or "gifModeToggleBackgroundOnly" (no hotkey by default) to make the background black and that would actually make the background transparent - but you can't see that with a naked eye. You need to press "screenshotBtn" (F8, copies to clipboard by default) to take a screenshot and paste it into a graphical editor supporting transparency, like GIMP for example, in order to see transparency.  
Transparency in the game is actually inverted, meaning the background is fully opaque while the characters are fully transparent. The screenshotter inverts the alpha channel to make it correct.  
Only GIMP has been tested to support the PNG screenshot format that the mod produces, and this works on Windows and on Ubuntu/Linux, where Guilty Gear Xrd runs under Steam Proton.  

### Screenshot saving location

By default the mod saves the screenshot into clipboard buffer, meaning you can paste it afterwards into a graphics program which supports transparency. In order to save screenshots to a file or directory you can either specify it in the mod's UI window in its Settings, or you can add the `ggxrd_hitbox_overlay.ini` file into the same folder as the game executable and write the path into the `screenshotPath` setting in it, without quotes. Now when you save multiple screenshots in a row, each consecutive one will get a number in its name, increasing from 1 to infinity. Screenshots are never cleaned up, so if you never clean them up yourself, you might fill up your hard drive.  
The only supported format by the mod is PNG and it uses `libpng` to encode that. You don't need to do anything to install `libpng` since it should come working inside the DLL already.  

### Continuous screenshotting

You can use the `.ini` setting `allowContinuousScreenshotting` to make it so that as you hold down the `screenshotBtn` the screenshots get taken every game frame. This only includes non-frozen and non-paused game frames, i.e. only animation frames that are actually new. This feature only works under following conditions:

- The `screenshotPath` in the `.ini` is not empty;
- It only works in Training mode;
- A match (training session) must currently be running;
- The mod is not currently disabled using `disableModToggle` or `startDisabled`.

There's also a toggle you can use instead of holding down a button, and that toggle is a checkbox in the mod's UI called 'Continuous Screenshotting Mode' or you can also edit the `continuousScreenshotToggle` setting in the `.ini`. It doesn't require `allowContinuousScreenshotting` to be set to `true` in order to work, can be any hotkey and toggles continuous screenshotting on and off, but it still works only under the aforementioned conditions.

### Non-transparent screenshotting

To take regular, non-transparency-enabled screenshots you can set the `dontUseScreenshotTransparency` setting to `true` either through the mod's UI window or in the `.ini` file (which must be placed into the game's folder).

### Converting PNGs into WEBP animation with transparency with ffmpeg

You can use ffmpeg (<https://www.ffmpeg.org/>) to convert multiple PNGs into one animation. Now, GIF could be used but it doesn't support semi-transparency: each pixel in GIF can only be either fully transparent or not transparent at all, which would ruin most animations containing special effects.  
WEBP supports full transparency and is an animation format. The command to transform PNGs into a WEBP is as follows:

```cmd
ffmpeg -framerate 4 -i imageFrameName%d.png -filter:v "crop=out_w:out_h:x:y" -quality 100 -loop 65535 webpFileName.webp
```

Make sure to substitute:  

- `ffmpeg` with the real location to the ffmpeg.exe on your computer;
- `-framerate 4` - make sure to substitute with the desired framerate, the higher the faster;
- `imageFrameName%d.png` with the path to the PNGs that you want to convert. The `%d` is the number part of the filename. So for example, if your PNGs are named like `screen1.png`, `screen2.png`, `screen3.png`, etc you would write here `screen%d.png`;
- `out_w`:`out_h`:`x`:`y` with the width and height of the cropping rectangle, x and y with the x and y of its top-left corner. You can get the bounding rectangle location by doing a selection in GIMP and maybe even in Paint;
- `-quality 100` - can be lowered of course to conserve file size;
- `webpFileName.webp` is the output webp file name. If you replace the `.webp` with `.gif` here and remove the `-quality` option, it might work for a GIF as well;

If any paths contain spaces (to ffmpeg, to the input PNGs, to the output WEBP) you must enclose them in quotation marks.

You can add a frame counter to the bottom of the animation using following ffmpeg command (taken from <https://stackoverflow.com/questions/15364861/frame-number-overlay-with-ffmpeg>):

```cmd
ffmpeg -framerate 2 -i imageFrameName%d.png -loop 65535 -vf "drawtext=fontfile=Arial.ttf: text='%{frame_num}': start_number=1: x=(w-tw)/2: y=h-(2*lh): fontcolor=black: fontsize=20: box=1: boxcolor=white: boxborderw=5" -c:a copy webpFileName.webp
```

Here I think `w` in `x=...` means width of the image, and `tw` is the width of the text. `h` is height of the image. Perhaps you can regulate the text position by setting the x and y to literal numbers. I'm sorry, I can't be of much help here.

### Converting PNGs into GIF animation with transparency with ffmpeg

This command takes a set of PNG screenshots labeled "screen1.png", "screen2.png", "screen3.png", etc (it's important that the numbers start from 0 or 1 or somewhere close to that) with transparency and converts them into a GIF with transparency with cropping with given framerate:

First generate a palette, so that we get the best quality of colors:

```cmd
ffmpeg -i screenspath\screen%d.png -vf palettegen=reserve_transparent=1 palettepath\palette.png
```

Not much we can customize in this.  
Then use the source images and the palette to produce a GIF:

```cmd
ffmpeg -framerate 20 -i screenspath\screen%d.png -i palettepath\palette.png -lavfi "crop=out_w:out_h:x:y,paletteuse=alpha_threshold=128:dither=floyd_steinberg" -gifflags -offsetting output\out.gif
```

Here the options you must replace are:

- `ffmpeg` - this is not an option, it's the command to be run, provide the full path to your ffmpeg.exe here;
- `-framerate 20` - replace 20 with desired framerate;
- `screenspath\screen%d.png` - the path to your screenshot PNG files. The %d is the number part of the filename. So for example, if your PNGs are named like screen1.png, screen2.png, screen3.png, etc you would write here screen%d.png;
- `crop=out_w:out_h:x:y` - this is the crop filter and its arguments. What a crop filter does is cut out only a part of the image. You must substitute `out_w` (width), `out_h` (height), `x` (top-left corner x) and `y` (top-left corner y) with the size and position of the cropping rectangle. You can get these positions by selecting a region of the image in GIMP or MSPaint;
- `,` - the `crop` filter and its arguments are then followed by `,`, which separates filters in a filter chain. The crop takes the first `-i` input as its input (crop has only one input) and produces one output. The first output of the `crop` filter and the second `-i` input (which is the palette) then go as inputs to the next, `paletteuse` filter;
- `paletteuse=` - this is the paletteuse filter and what follows are its arguments, separated by `:`;
- `alpha_threshold=128` - this is the first argument of the `paletteuse` filter. It specifies the cut-off alpha threshold after which the pixel is considered fully transparent. Since there's no partial transparency in GIF - only either full transparency or no transparency - this value is very important if there's partial transparency or some kind of special effects in your animation;
- `dither=floyd_steinberg` - this is the second argument of the `paletteuse` filter. It specifies dithering. Dithering is the process by which all the other colors that are not in the palette are achieved in GIF. Personally I find that the `floyd_steinberg` value produces the best results, but the other values that you could use for `dither` are: `bayer` (fixed grid (static) dithering - the classic look and feel of the GIF format), `none` (no dithering, I guess for when you got the colors in the palette exactly right or for 8-bit images).
- `gifflags`, `offsetting` - I don't know what these mean, got them from <https://stackoverflow.com/questions/53566442/ffmpeg-gif-with-transparency-from-png-image-sequence>;
- `output\out.gif` - the path to the GIF output file.

Again, if any paths contain spaces, you must enclose them in quotes. You can read about ffmpeg filter syntax on: <https://ffmpeg.org/ffmpeg-filters.html#toc-Filtering-Introduction>  
I will add that `-lavfi`, `-filter_complex`, `-vf`, `-af`, `-filter` mean exactly the same thing, which is a filtergraph. `:v` usually means the video part of an input, `:a` means the audio part of an input.

## Renumbering frames and changing framerate of a subrange of frames in a GIF

The tools provided in <https://github.com/kkots/GIFTools> allow renumbering PNG files in a PNG sequence and altering individual GIF frame lengths through a console (non-graphical) interface.

## Developing

### How to clone this repo

This git repo has a submodule for `imgui`. To clone this repo you can either follow this guide: <https://git-scm.com/book/en/v2/Git-Tools-Submodules>  
Or use command:

```bash
git clone --recurse-submodules https://github.com/kkots/ggxrd_hitbox_overlay_2211.git
```

This will create a subfolder called `ggxrd_hitbox_overlay_2211` in your current directory, download the mod's files into it, in that folder there will be another folder called `imgui` and that will contain the files for the version of imgui that we use.

### Project structure

There are multiple separate projects in the repository.

The `ggxrd_hitbox_injector` project builds an application that will inject a dll into the process and exit. The main action will then take place in the dll, in the target process' address space.

The `ggxrd_hitbox_overlay` project builds the dll that's responsible for drawing the hitboxes.

The `ggxrd_hitbox_patcher` project is cross-platform for Windows and Ubuntu/Linux and patches the GuiltyGearXrd.exe executable so that it launches the mod's overlay DLL on startup. This is needed in case injector doesn't work on Ubuntu/Linux (there is a launcher script for it though, try that) or to make the game always start with the mod on Windows.

Each project should have its own separate README.md.

The `libpng` series of projects is an outside repository that we depend on that I had to modify, so it's not included as a git submodule, but copied directly into the mod's sources. The reason I had to modify some of it was because I needed to retarget its Visual Studio projects since I didn't have the older build tools.

`imgui` is an outside project that we depend on, it's included in this mod as a git submodule. Please do not modify anything there and try not to update it using git fetch or git pull, unless you're ready to make corresponding changes in our mod.

`detours` is Microsoft Detours library, also included as a git submodule.

`zlib` is a compression library needed for libpng. Unlike libpng, it *is* included as a git submodule here. Please also refrain from modifying it.

The dependency projects are better described in *Development dependencies*.

## Development dependencies

Dependencies are better described in each project's README.md. Short version is, the project depends on:

- Microsoft Detours library: <https://github.com/microsoft/Detours> Used for hooking functions. We need the 32-bit (x86) statically linked library of it. It is included in the mod as a git submodule: <https://github.com/microsoft/Detours.git>.
  If you run into the problem that Detours rebuilds itself and all its samples each time you try to build the ggxrd_hitbox_overlay project, try replacing the Makefile located in SOLUTION_ROOT/Detours/Makefile with an empty file. Do not commit this change to the Detours repository, either locally or remotely.

- `dxsdk` - repository version of Microsoft's Direct3D 9 SDK. Needed for the `d3dx9.h` header file. The repo is included in this mod as a git submodule: <https://github.com/apitrace/dxsdk.git>

- `libpng` - a PNG encoder library. This is needed for the transparent screenshotting functionality. You should statically link its 32-bit verion into this mod, it's sources are included in the mod directly (not as a submodule). libpng homepage: <http://www.libpng.org/pub/png/libpng.html>. libpng github repository: <https://github.com/pnggroup/libpng>

- `zlib` - a compression library needed for libpng. You should statically link its 32-bit verion into this mod, it's included in the mod as a git submodule: <https://github.com/madler/zlib.git>

- `imgui` - a graphical user interface library for C++. This is used to draw the mod's UI using Direct3D 9 API in the overlay, inside the game. The sources of imgui are included in this mod as a git submodule: <https://github.com/ocornut/imgui.git>.

- `D3DCompiler_47.dll` - required to compile a pixel shader at run-time. This DLL should be present on the end user's Windows machine, in the C:\\Windows\\SysWOW64 folder. Distributing it on your own may constitute a violation of copyright. If problems arise with it you'll need to remove it from dependencies. See ggxrd_hitbox_overlay's README.

## Changelog

- 2023 October 13: Now hitboxes belonging to the same group may be displayed as a single shape with one combined outline;
- 2023 October 16: Added Unicode support to the injector, meaning you should be able to include any non-english characters in the path to the directory in which the injector and the .dll reside;  
                   Tweaked hitbox drawing so that outlines always draw on top of all hitboxes, hitboxes always draw on top of all hurtboxes.  
                   Restricted the hitbox drawing to only non-online matches until I figure out a way to tell if Chipp is doing the invisibility thing in an online match, in which case his boxes should not display.
- 2023 October 21: Now boxes won't be drawn for Chipp & his projectiles in online mode if he's invisible. The mod in all other cases will display boxes in online mode.  
                   Thanks to WorseThanYou's help, added counterhit state to hurtboxes. Your hurtbox will be blue if, should you get hit, you will be in a counter hit state.  
                   Boxes now don't show if a menu is open or an Instant Kill cutscene is currently playing.  
                   Now hitboxes show only during active frames, i.e. there's no longer a problem of them showing during recovery. Hitboxes never show as transparent, only filled in.  
                   Hitboxes keep showing even after an attack connects, as long as the attack's active frames are going on.  
                   Millia's Tandem Top and Sol's Gunflame now show hitbox for a brief period after hitting a target so you could actually see the hitbox on hit.  
                   Added invulnerability check related to startup of supers, such as May's temper tantrum, and possibly many other moves.  
                   Added invulnerability check related to throws but it's incomplete yet, Slayer still shows up as vulnerable for the remainder of his ground throw.  
                   You can now unload the dll after it has been loaded, reverting the game back to normal without having to restart it.  
                   Added proper hitbox display for attached entities like Ky's, I-No's and Millia's Dust (5D) attacks.
- 2023 October 27: Major refactoring.  
                   Fixed hitboxes display for rotated projectiles like Ky's Stun Edge. Previously it was showing as rotated boxes which is not how the game actually checks if it hits. Now it's showing as a box ladder which is correct.  
                   Now while doing a throw you display as strike invulnerable throughout the entire duration of the throw, not just part of it.  
                   Made counterhit state not display if you're strike invulnerable.  
                   Made counterhit state not display on summons or anyone but the main player entities.  
                   Added pushboxes.  
                   Fixed an issue when all boxes were always drawn twice per frame, making them more opaque. Now they should be more transparent.  
                   Moved the binaries into Github's Releases section of this project.
- 2023 October 28: Added gray boxes which are the previous arrangement of the hurtbox that was before the moment it got hit by an attack.  
                   Added missing throw invulnerability checks.  
                   Added throw boxes.  
                   Made Ky's grinders display as strike invulnerable.  
                   Made Jack-O's and Bedman's summons display extra transparent.  
                   Made counterhit display for longer on the one who got hit.  
                   Fixed an error when after some computer restarts signatures wouldn't be found anymore.  
- 2023 October 31: Added rejection boxes that only show up when rejecting projectiles, as normal hits get rejected no matter the distance.  
                   Removed counterhit prolonged display, meaning it no longer still displays you as being in counterhit state after you get hit. This is because counterhit state display is for when you haven't got hit yet, but would enter counterhit state should you get hit.  
                   Removed hitbox showing from episode mode interludes, intro cinematics, before the match starts and on victory/defeat screen. Hitboxes still show during roundend screenfreeze.  
                   Fixed Chipp invisibility in online mode so that only the opponent's invisible Chipp doesn't display boxes. Previously you couldn't see even your own Chipp's boxes if you went invisible. Thanks to WorseThanYou for finding the value to tell which side you're on in online mode.  
                   Fixed an issue when gray boxes (from before the hit) were showing even if you were strike invulnerable at the moment of the hit, meaning you didn't actually get hit. Also fixed gray boxes so that their outlines now show behind the real hurtbox's outlines.  
                   Fixed prolonged hitbox display for Chipp's Gamma Blade, so now the hitbox doesn't immediately disappear as soon as Gamma Blade hits.  
                   Fixed gray boxes still showing after IK cutscene.  
                   Fixed pause menu falsely being reported as open after some computer and game restarts.
- 2023 November 1: Fixed possible crash when unloading the DLL.
- 2023 November 6: Remade DLL unloading and added "GIF mode" and "no gravity" mode.
- 2023 November 13: Fixed a possible freeze when unloading the DLL.
- 2023 November 14: Added ability to play the game frame-by-frame or in "slow motion" mode (in training mode only).
- 2023 November 15: Removed Potemkin Buster hitbox - it's fake and doesn't actually affect anything.
- 2023 November 15: Added patcher.
- 2023 November 16: Made fixes so it works under Steam Proton on Ubuntu.
- 2023 November 16: Added toggle to only disable box drawing.
- 2023 November 17: With help from WorseThanYou added a transparent screenshotter to the mod.
- 2023 November 17: Fixed origin point showing the dummy even when it's hidden using "GIF mode" (or similar feature).
- 2023 November 17: Fixed pasting of transparent screenshots into MSPaint and added non-transparent screenshotting.
- 2023 November 19: Added ability to reload the settings file on the fly, without reloading the mod.
- 2023 November 20: Made GIF mode hide all entities that belong to the opponent's side (still can't hide the tension lightning though). Made continuous screenshot toggle not take a screenshot erroneously twice when toggled on during the game frozen state.
- 2023 November 20: Added the `drawPushboxCheckSeparately` mode which breaks throw boxes into two parts: pushbox-proximity-checking and origin-point-checking.
- 2023 November 21: Added reverse chroma key to the transparent screenshotting, so that pixels that have non-black color but 0 alpha still get some alpha.
- 2023 November 26: Made real-time continuous screenshotting absolutely frame-precise, meaning hitboxes always match positions of the sprites displayed on the screen.
- 2023 November 29: Fixed a crash when quitting a match/training session while the mod is running.
- 2023 December 12: Made some fixes in code related to thread safety.
- 2023 December 13: Tried to fix an issue with hotkeys not working during frame freeze.
- 2023 December 20: Fixed May's dolphin hurtbox rotation.
- 2024 January 1: Fixed an issue when boxes weren't frame perfect on very low FPS (caused by excessive logging in debug version).
- 2024 January 3: Fixed the issue of boxes being 1 frame behind during frame freeze mode + gif mode.
- 2024 January 3: Fixed the issue of continuous screenshot mode taking screens during frame freeze 60 times per second even if the game is frozen. Fixed the issue of screenshot button not working during frame freeze mode.
- 2024 January 3: Fixed the issue of boxes being placed wrong during GIF mode + frame freeze on moves that move the character. Fixed boxes displaying during frame freeze even when menu is open.
- 2024 January 3: Removed Raven's ground command grab hitbox - it does nothing.
- 2024 January 12: Fixed hitboxes' position not responding to camera movement after using the mod for a while.
- 2024 January 13: Fixed hitboxes' position not responding to camera movement in online matches due to rollback. Fixed a crash caused by using hitbox overlay in rollback-affected matches.
- 2024 January 19: Fixed hitbox display disabling/enabling during frame freeze mode.
- 2024 January 26: Added a toggle to hide the hud only/separated hud out of the "GIF mode". Fixed an issue when rapidly freezing/unfreezing the game made the hitboxes desync from the visuals.
- 2024 September 16: Added imGui.
- 2024 September 24: Fixed some animations still playing in frame freeze mode and playing too fast is slow-mo mode. Fixed danger time countdown playing during frame freeze mode. Fixed Johnny coins and stunmash indicator not displaying in frame freeze mode and displaying glitchily in slow-mo mode. Fixed play, record and backspace buttons not working in frame freeze and slow-mo modes. Draw hitboxes and imgui under steam overlay instead of on top. Not interfere with OBS recording. Can now hide ky sword lightning and >50 tension sparkles in gif mode. Fixed johnny coins, jacko organ cooldown indicators, jam powerup icons not being hidden in gif mode.
- 2024 October 3: drawing hitboxes underneath not just steam overlay, but the in-game pause menu and training HUD, while the origin point is still drawn on top of training HUD, but underneath the pause menu. Added in-game icons to imgui window. Fixed Potemkin IK throwbox, which also fixed throwboxes for Sol Riot Stamp, maybe Potemkin ICPM if it wasn't showing before (now it does), maybe other moves that grab you on hit. Fixed 6P+H,5 (blitz input by pressing 6P+H and immediately releasing 6) displaying a throw which is active for 2 frames which is incorrect - throw is actually active for 1 frame. Changed how backside and upside Ky's Ride the Lightning hitboxes display - they're "clash only". Similarly for Jack O Aegis Field hitbox. Hiding boxes on Slayer's forward and back dashes in online mode for the sake of making it a guess which side he's teleporting to. Fixed Dizzy Reflect hitbox - it was not visible.
- 2024 October 20: added missing check for full invul that happens during clash hitstop.
- 2024 October 23: fixed missing hitboxes for Dizzy's bubble pop
- 2024 October 30: added toggleCameraCenterOpponent setting into INI and the UI to center camera on the opponent. Added a toggle and setting to hide the player. Added a toggle to disable gray hurtboxes. In other words, you can now screenshot hurtboxes of the dummy during hitstun and blockstun.
- 2024 November 12: made hitboxes' outline change to black when drawing over similarly colored background. For example, previously, when the character is red, the hitbox outline would not be visible on top of them, but now the outline will be black in this particular case.