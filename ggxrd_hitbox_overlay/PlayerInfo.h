#pragma once
#include "Entity.h"
struct ActiveData {
	short actives = 0;
	short nonActives = 0;
};

struct ActiveDataArray {
	int count = 0;  // the number of internal storage elements in the array
	int prevHitNum = -1;
	ActiveData data[60] { 0 };  // the array of internal storage elements
	bool hitNumConflict:1;
	bool merging:1;
	ActiveDataArray() : hitNumConflict(false), merging(false) { }
	// Adds a specified amount of non-active frames to the end of the array.
	inline void addNonActive(int n = 1) {
		if (count == 0) {
			return;
		}
		data[count - 1].nonActives += n;
		if (!merging) {
			prevHitNum = -1;
		}
	}
	// Adds a specified amount of active frames to the end of the array, with the given hit number.
	// A new span of active frames is created if the hit number doesn't match the previously stored
	// hit number or there is a span of non-active frames currently at the end of the array.
	void addActive(int hitNum = -1, int n = 1, bool forceNewHit = false);
	// For use in superfreeze. Allows to replace the last frame, whether it was active or inactive, with an active frame.
	void addSuperfreezeActive(int hitNum = -1);
	inline void clear() { count = 0; hitNumConflict = false; }
	// Print active frames, not including the final span of non-active frames.
	// Separate distinct hits using a comma (",") character.
	// Non-active frames are printed in parentheses ("(123)") inbetween the active frames.
	void print(char* buf, size_t bufSize);
	// Print active frames, not including the final span of non-active frames.
	// Fuse distinct hits together into unified spans of active frames. Don't use comma (",") character.
	// Non-active frames are printed in parentheses ("(123)") inbetween the active frames.
	void printNoSeparateHits(char* buf, size_t bufSize);
	void printNoSeparateHitsGapsBiggerThan3(char* buf, size_t bufSize);
	void removeSeparateHits(int* outIndex);
	// Wrap multiple calls to addActive(...) in beginMergeFrame() and endMergeFrame(),
	// if you want multiple entities to contribute to the active frames on the same frame.
	inline void beginMergeFrame() { merging = true; addNonActive(1); }
	inline void endMergeFrame() { merging = false; }
	// Don't wrap this in beginMergeFrame(), endMergeFrame().
	// It combines the -other- with this.
	// If both mark a frame as active, it remains active.
	// If one marks a frame as non-active, the other as active, then the frame is active.
	// If both mark a frame as non-active, the frame is non-active.
	// If information about multi-hit active frames is non-conflicting, it is preserved.
	// Otherwise, the multi-hit information is disposed of, leaving only spans of active frames and non-active frames.
	void mergeTimeline(int startup, const ActiveDataArray& other);
	bool checkHitNumConflict(int startup, const ActiveDataArray& other);
	void findFrame(int frame, int* outIndex, int* outFrame) const;
	int total() const;
};

struct ProjectileInfo {
	Entity ptr = 0;  // may be 0 if the entity tied to this projectile no longer exists - such projectiles are removed at the start of the next logic tick. Otherwise points to that entity
	int team = 0;  // updated every frame
	int lifeTimeCounter = 0;  // updated every frame
	int animFrame = 0;  // updated every frame
	int hitstop = 0;  // updated every frame
	int startup = 0;  // if active frames have not started yet, is equal to total. Otherwise, means time since the owning player has started their last move until active frames, inclusive
	int total = 0;  // time since the owning player started their last move
	int hitNumber = 0;  // updated every frame
	ActiveDataArray actives;
	char animName[32] { 0 };
	bool markActive:1;  // cleared at the start of prepareDrawData. True means hitboxes were found on this frame, or on this logic tick this projectile registered a hit.
	bool startedUp:1;  // cleared upon disabling. True means active frames have started.
	bool landedHit:1;  // cleared at the start of each logic tick. Set to true from a hit registering function hook.
	bool disabled:1;  // set to true for all projectiles belonging to a player when the player starts a new move.
	ProjectileInfo() :
		markActive(false),
		startedUp(false),
		landedHit(false),
		disabled(false)
	{
	}
	void fill(Entity ent);
};

// This struct is cleared by setting all its memory to zero. If you add a new member, make sure it's ok to initialize it with 0.
// This means you cannot add std::vector or std::string here.
struct PlayerInfo {
	Entity pawn{ nullptr };
	int hp = 0;
	int maxHp = 0;
	int defenseModifier = 0;  // dmg = dmg * ((256 + defenseModifier) / 256)
	int gutsRating = 0;
	int gutsPercentage = 0;
	int risc = 0;  // max 12800
	
	int tension = 0;  // max 10000
	int tensionPulse = 0;  // -25000 to 25000. You can read about this in AddTooltip in UI.cpp
	int negativePenaltyTimer = 0;  // time remaining until negative penalty wears off, in frames
	int negativePenalty = 0;  // 0 to 10000
	int tensionPulsePenalty = 0;  // 0 to 1800
	int cornerPenalty = 0;  // 0 to 960
	
	int tensionPulsePenaltyGainModifier_distanceModifier = 0;
	int tensionPulsePenaltyGainModifier_tensionPulseModifier = 0;
	int tensionGainModifier_distanceModifier = 0;
	int tensionGainModifier_negativePenaltyModifier = 0;
	int tensionGainModifier_tensionPulseModifier = 0;
	int extraTensionGainModifier = 0;
	int receivedComboCountTensionGainModifier = 0;
	int dealtComboCountTensionGainModifier = 0;
	
	int tensionGainOnLastHit = 0;
	int burstGainOnLastHit = 0;
	int tensionGainLastCombo = 0;
	int burstGainLastCombo = 0;
	int tensionGainMaxCombo = 0;
	int burstGainMaxCombo = 0;
	
	int x = 0;
	int y = 0;
	int prevSpeedX = 0;
	int speedX = 0;
	int speedY = 0;
	int gravity = 0;
	
	int pushback = 0;
	int pushbackMax = 0;
	int basePushback = 0;
	int baseFdPushback = 0;
	int fdPushback = 0;
	int fdPushbackMax = 0;
	int comboTimer = 0;
	int attackPushbackModifier = 100;
	int painPushbackModifier = 100;
	int comboTimerPushbackModifier = 100;
	int ibPushbackModifier = 100;
	
	int receivedSpeedY = 0;
	int receivedSpeedYWeight = 100;
	int receivedSpeedYComboProration = 100;
	
	int hitstunProration = 100;
	
	int stun = 0;
	int stunThreshold = 0;
	int hitstunMax = 0;
	int blockstunMax = 0;
	int hitstopMax = 0;
	int blockstun = 0;
	int hitstun = 0;
	int hitstop = 0;
	int burst = 0;  // max 15000
	int comboCountBurstGainModifier = 0;
	int frameAdvantage = 0;
	int landingFrameAdvantage = 0;
	int gaps[10] { 0 };
	int gapsCount = 0;
	int timeSinceLastGap = 0;
	int weight = 0;
	int wakeupTiming = 0;
	WakeupTimings wakeupTimings;
	int timePassed = 0;  // time passed since a change in idlePlus. If it's false, this measures the time you've been busy for.
	                     // If it's true, this measures the time you've been idle for
	int timePassedLanding = 0;  // time passed since a change in idleLanding. If it's false, this measures the time since the start of an air move.
	                            // If it's false, this measures the time since you've last landed or been idle on the ground
	int hitboxesCount = 0;
	int superfreezeStartup = 0;
	
	int startup = 0;  // startup of attacks done directly by the character. Either current or of the last move
	ActiveDataArray actives;  // active frames of attacks done directly by the character. Either current or of the last move
	int recovery = 0;  // recovery of attacks done directly by the character. Either current or of the last move
	int total = 0;  // total frames of attacks done directly by the character. Either current or of the last move
	
	int startupDisp = 0;  // startup to display in the UI. Either current or of the last move
	ActiveDataArray activesDisp;  // active frames to display in the UI. Either current or of the last move
	int recoveryDisp = 0;  // recovery to display in the UI. Either current or of the last move
	int totalDisp = 0;  // total frames to display in the UI. Either current or of the last move
	
	int startupProj = 0;  // startup of all projectiles. Either current or of the last move
	ActiveDataArray activesProj;  // active frames of all projectiles. Either current or of the last move
	
	int landingOrPreJumpFrames = 0;  // number of frames currently spent in landing or prejump
	int landingRecovery = 0;  // number of landing recovery frames. Either current or of the last performed move
	int animFrame = 0;
	enum XstunDisplay {
		XSTUN_DISPLAY_NONE,
		XSTUN_DISPLAY_HIT,  // hitstun
		XSTUN_DISPLAY_BLOCK  // blockstun
	} xStunDisplay = XSTUN_DISPLAY_NONE;  // the last thing that was displayed in UI in 'Hitstop+X-stun' field.
	CmnActIndex cmnActIndex = CmnActStand;
	char tensionPulsePenaltySeverity = 0;  // the higher, the worse
	char cornerPenaltySeverity = 0;  // the higher, the worse
	bool frameAdvantageValid:1;
	bool landingFrameAdvantageValid:1;
	bool idle:1;  // is able to perform a non-cancel move
	bool idleNext:1;  // the precomputed new value for 'idle'. We need this because a change of idle is checked twice in separate loops
	bool idlePlus:1;  // is able to perform a non-cancel move. Jump startup and landing are considered 'idle' if not immediately following or preceding a move
	bool idleLanding:1;  // is able to perform a non-cancel move. Time spent in the air after recovering from an air move is considered 'not idle'
	bool startedUp:1;  // if true, recovery frames or gaps in active frames are measured instead of startup
	bool onTheDefensive:1;  // true when blocking or being combo'd/hit, or when teching or waking up
	bool landingOrPreJump:1;  // becomes true when transitioning from idle to prejump/landing. Becomes false when exiting prejump/landing
	bool isLanding:1;  // on this frame, is it landing animation
	bool isLandingOrPreJump:1;  // on this frame, is it either landing or prejump animation
	bool needLand:1;  // recovery from an air move happened and now landing is needed to measure frame advantage on landing
	bool airborne:1;  // is y > 0
	bool inPain:1;  // being combo'd
	bool gettingUp:1;  // playing a wakeup animation
	bool wasIdle:1;  // briefly became idle during the frame while transitioning through some animations
	bool startedDefending:1;  // triggers restart of frame advantage measurement
	bool moveOriginatedInTheAir:1;  // for measuring landing recovery of moves that started in the air only
	bool setHitstopMax:1;  // during this logic tick, from some hook, hitstopMax field was updated
	bool setHitstunMax:1;  // during this logic tick, from some hook, hitstunMax field was updated
	bool setBlockstunMax:1;  // during this logic tick, from some hook, blockstunMax field was updated
	bool displayHitstop:1;  // should hitstop be displayed in UI
	bool oppoWasTouchingWallOnFD:1;
	bool receivedSpeedYValid:1;
	bool hitstunProrationValid:1;
	CharacterType charType = CHARACTER_TYPE_SOL;
	char anim[32] { 0 };
	char index = 0;  // the index of this PlayerInfo in endScene's 'players' array
	inline void clearGaps() { gapsCount = 0; }
	void addGap(int length = 1);
	void printGaps(char* buf, size_t bufSize);
	void clear();
	void copyTo(PlayerInfo& dest);
};
