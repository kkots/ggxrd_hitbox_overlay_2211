#pragma once
#include "Entity.h"
#include "Moves.h"
#include <string>

enum FrameType : char {
	FT_NONE,
	FT_IDLE,
	FT_HITSTOP,
	FT_ACTIVE,
	FT_STARTUP,
	FT_RECOVERY,
	FT_NON_ACTIVE,
	FT_PROJECTILE,
	FT_LANDING_RECOVERY,
	FT_XSTUN
};

// This struct is initialized by doing memset to 0. Make sure every child struct is ok to memset to 0.
// This means that types like std::vector are not allowed.
struct Frame {
	DWORD aswEngineTick;
	FrameType type;
	bool strikeInful:1;
	bool throwInvul:1;
	bool superArmorActive:1;
	bool isFirst:1;
	bool enableNormals:1;
	bool canBlock:1;
};

// This struct is initialized by doing memset to 0. Make sure every child struct is ok to memset to 0.
// This means that types like std::vector are not allowed.
struct Framebar {
	FrameType preFrame = FT_NONE;
	DWORD preFrameLength = 0;
	bool requestFirstFrame = false;
	Frame frames[80] { 0 };
	bool completelyEmpty = false;
	inline Frame& operator[](int index) { return frames[index]; }
	inline const Frame& operator[](int index) const { return frames[index]; }
	void clear();
	int findTickNoGreaterThan(int startingPos, DWORD tick) const;
	void soakUpIntoPreFrame(const Frame& srcFrame);
	void combineFramebar(const Framebar& source);
};

struct EntityFramebar {
	int playerIndex = -1;
	int id = -1;
	const MoveInfo* move = nullptr;
	static const int titleShortCharsCountMax = 12;
	char titleShort[titleShortCharsCountMax * 4 + 1];  // UTF8, 12 characters, 4 bytes maximum each, plus a terminating null
	std::string titleFull;
	Framebar main { FT_NONE };  // the one framebar that is displayed
	Framebar idle { FT_NONE };  // because we don't update the framebar when players are idle and reset it when an action beghins after
	                            // EndScene::framebarIdleForLimit f of idle, if an action begins before EndScene::framebarIdleForLimit f
	                            // of idle with some non-zero f idle time, we need to display what happened during that
	                            // idle time so we will take that information from here. This framebar shall be updated even during idle time
	Framebar hitstop { FT_NONE };  // we omit hitstop in the main framebar, but there's a setting to show it anyway, and we want the
	                               // framebar to update upon changing this setting without having to re-record the action.
	                               // Hence this framebar works in parallel with the main one, with the one difference that it always records hitstop
	Framebar idleHitstop { FT_NONE };  // information from this gets copied to the hitstop framebar when the omitted idle frames are needed as
	                                   // described in the 'idle' framebar
	bool foundOnThisFrame = false;
	void setTitle(const char* text, const char* textFull = nullptr);
	void copyTitle(const EntityFramebar& source);
	void changePreviousFrames(FrameType prevType,
		FrameType newType,
		int positionHitstopIdle,
		int positionHitstop,
		int positionIdle,
		int position,
		int maxCount,
		bool stopAtFirstFrame = false);
	static int confinePos(int pos);
	inline static void decrementPos(int& pos) { if (pos == 0) pos = _countof(main.frames) - 1; else --pos; }
	inline static void incrementPos(int& pos) { if (pos == _countof(main.frames) - 1) pos = 0; else ++pos; }
	inline bool belongsToProjectile() const { return id != -1; }
	inline bool belongsToPlayer() const { return id == -1; }
	// maxCodepointCount - pass in a pointer to the maximum number of whole characters (codepoints). This
	//  number of characters will be included in the byte-length that will be returned via the same
	//  (maxCodepointCount) pointer.
	
	/// <summary>
	/// Determines the length of the UTF8 encoded string in both bytes, not including the terminating null character,
	/// and codepoints (whole UTF32 characters), as well as returns the number of bytes, not including the terminating
	/// null character, that the left portion of the string of given maximum codepoint length is occupying.
	/// </summary>
	/// <param name="txt">The UTF8 encoded string, must be null terminated.</param>
	/// <param name="byteLen">Returns the length of the string, in bytes, not including the terminating null character.</param>
	/// <param name="cpCountTotal">Returns the number of codepoints in the string, not including the terminating null character.</param>
	/// <param name="maxCodepointCount">Provide the maximum number of codepoints to use to calculate byteLenBelowMax.</param>
	/// <param name="byteLenBelowMax">Uses the first maxCodepointCount codepoints of the string to calculate their byte length
	/// and return it. Does not include terminating null characters in either the calculation or the return value.
	/// If the provided maxCodepointCount is more than the total number of codepoints in the string, returns the same
	/// value as byteLen.</param>
	static void utf8len(const char* txt, int* byteLen, int* cpCountTotal, int maxCodepointCount, int* byteLenBelowMax);
};

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
	void print(char* buf, size_t bufSize) const;
	// Print active frames, not including the final span of non-active frames.
	// Fuse distinct hits together into unified spans of active frames. Don't use comma (",") character.
	// Non-active frames are printed in parentheses ("(123)") inbetween the active frames.
	void printNoSeparateHits(char* buf, size_t bufSize) const;
	void printNoSeparateHitsGapsBiggerThan3(char* buf, size_t bufSize) const;
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

struct PrevStartupsInfo {
	short startups[10] { 0 };
	char count = 0;
	inline short& operator[](int index) {
		return startups[index];
	}
	inline void clear() { count = 0; }
	void add(short n);
	void print(char*& buf, size_t& bufSize) const;
	int total() const;
	inline bool empty() const { return count == 0; }
};

struct SpriteFrameInfo {
	char name[32] { 0 };
	int frame = 0;
	int frameMax = 0;
	void print(char* buf, size_t bufSize) const;
	void fill(Entity ent);
};

struct EddieInfo {
	Entity ptr = nullptr;
	Entity landminePtr = nullptr;
	
	int startup = 0;
	bool startedUp = false;
	ActiveDataArray actives;
	int recovery = 0;
	int total = 0;
	
	int hitstop = 0;
	int hitstopMax = 0;
	
	int timePassed = 0;
	int frameAdvantage = 0;
	int landingFrameAdvantage = 0;
	int frameAdvantageCanBlock = 0;
	int landingFrameAdvantageCanBlock = 0;
	
	char anim[32] { '\0' };
	int prevResource = 0;
	int consumedResource = 0;
	
	DWORD moveStartTime_aswEngineTick = 0;
	
	bool idle:1;
	bool prevEnemyIdle:1;
	bool prevEnemyIdleLanding:1;
	bool prevEnemyIdleCanBlock:1;
	bool prevEnemyIdleLandingCanBlock:1;
	bool frameAdvantageValid:1;
	bool landingFrameAdvantageValid:1;
	bool frameAdvantageIncludesIdlenessInNewSection:1;
	bool landingFrameAdvantageIncludesIdlenessInNewSection:1;
};

struct ProjectileInfo {
	Entity ptr = nullptr;  // may be 0 if the entity tied to this projectile no longer exists - such projectiles are removed at the start of the next logic tick. Otherwise points to that entity
	int team = 0;  // updated every frame
	int lifeTimeCounter = 0;  // updated every frame
	int animFrame = 0;  // updated every frame
	int hitstop = 0;  // updated every frame
	int startup = 0;  // if active frames have not started yet, is equal to total. Otherwise, means time since the owning player has started their last move until active frames, inclusive
	int total = 0;  // time since the owning player started their last move
	int hitNumber = 0;  // updated every frame
	DWORD creationTime_aswEngineTick = 0;
	ActiveDataArray actives;
	const MoveInfo* move = nullptr;
	PrevStartupsInfo prevStartups { 0 };
	SpriteFrameInfo sprite;
	int framebarId = -1;
	char creatorName[32] { 0 };
	char animName[32] { 0 };
	bool markActive:1;  // cleared at the start of prepareDrawData. True means hitboxes were found on this frame, or on this logic tick this projectile registered a hit.
	bool startedUp:1;  // cleared upon disabling. True means active frames have started.
	bool landedHit:1;  // cleared at the start of each logic tick. Set to true from a hit registering function hook.
	bool disabled:1;  // set to true for all projectiles belonging to a player when the player starts a new move.
	bool inNewSection:1;
	bool isDangerous:1;
	bool superArmorActive:1;
	ProjectileInfo() :
		markActive(false),
		startedUp(false),
		landedHit(false),
		disabled(false)
	{
	}
	void fill(Entity ent);
	void printStartup(char* buf, size_t bufSize);
	void printTotal(char* buf, size_t bufSize);
};

// This struct is cleared by setting all its memory to zero. If you add a new member, make sure it's ok to initialize it with 0.
// This means you cannot add std::vector or std::string here.
struct PlayerInfo {
	Entity pawn{ nullptr };
	int hp = 0;
	int maxHp = 0;
	int defenseModifier = 0;  // dmg = dmg * (256 + defenseModifier) / 256
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
	int hitstunPushbackModifier = 100;
	int comboTimerPushbackModifier = 100;
	int ibPushbackModifier = 100;
	
	int receivedSpeedY = 0;
	int receivedSpeedYWeight = 100;
	int receivedSpeedYComboProration = 100;
	
	int hitstunProration = 100;
	
	int lastIgnoredHitNum = -1;  // this is to stop Dizzy ground throw from showing a hitbox on the leech bite
	
	int stun = 0;
	int stunThreshold = 0;
	int hitstunMax = 0;
	int blockstunMax = 0;
	int hitstopMax = 0;
	int hitstopMaxSuperArmor = 0;  // for super armors showing correct hitstop max
	int blockstun = 0;
	int hitstun = 0;
	int hitstop = 0;
	int clashHitstop = 0;
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
	
	// time passed since a change in idlePlus. If it's false, this measures the time you've been busy for.
	// If it's true, this measures the time you've been idle for
	int timePassed = 0;
	// time passed since a change in idleLanding. If it's false, this measures the time since the start of an air move.
	// If it's false, this measures the time since you've last landed or been idle on the ground
	int timePassedLanding = 0;
	                            
	int hitboxesCount = 0;
	int superfreezeStartup = 0;
	
	int startup = 0;  // startup of the last move done directly by the character
	ActiveDataArray actives;  // active frames of the last move done directly by the character
	int recovery = 0;  // recovery of the last move done directly by the character. Includes only frames where you can't attack
	int total = 0;  // total frames of the last move done directly by the character. Includes only frames where you can't attack
	
	int totalCanBlock = 0;  // total frames of the last move done directly by the character. Includes only frames where you can't block
	
	int totalFD = 0;  // number of frames for which you were holding FD
	
	PrevStartupsInfo prevStartups { 0 };  // startups of moves that you whiff canceled from
	
	int startupDisp = 0;  // startup to display in the UI. Either current or of the last move
	ActiveDataArray activesDisp;  // active frames to display in the UI. Either current or of the last move
	int recoveryDisp = 0;  // recovery to display in the UI. Either current or of the last move. Includes only frames where you can't attack
	int totalDisp = 0;  // total frames to display in the UI. Either current or of the last move. Includes only frames where you can't attack
	
	int startupProj = 0;  // startup of all projectiles. Either current or of the last move
	ActiveDataArray activesProj;  // active frames of all projectiles. Either current or of the last move
	
	PrevStartupsInfo prevStartupsDisp { 0 };  // things to add over a + sign in the displayed startup field
	PrevStartupsInfo prevStartupsTotalDisp { 0 };  // things to add over a + sign in the displayed 'Total' field
	//  this relies on there being only one active projectile for a move.
	//  it's a copy of previous startups of that projectile
	PrevStartupsInfo prevStartupsProj { 0 };
	
	int landingRecovery = 0;  // number of landing recovery frames. Either current or of the last performed move
	int animFrame = 0;
	enum XstunDisplay {
		XSTUN_DISPLAY_NONE,
		XSTUN_DISPLAY_HIT,  // hitstun
		XSTUN_DISPLAY_BLOCK  // blockstun
	} xStunDisplay = XSTUN_DISPLAY_NONE;  // the last thing that was displayed in UI in 'Hitstop+X-stun' field.
	CmnActIndex cmnActIndex = CmnActStand;
	int timeInNewSection = 0;
	DWORD wasForceDisableFlags = 0;
	SpriteFrameInfo sprite;
	const MoveInfo* move = nullptr;
	
	int playerval0 = 0;
	int playerval1 = 0;
	int maxDI = 0;
	int remainingDoubleJumps = 0;
	EddieInfo eddie { 0 };
	
	DWORD moveStartTime_aswEngineTick = 0;
	
	char attackLockAction[32] { '\0' };
	char prevAttackLockAction[32] { '\0' };
	char tensionPulsePenaltySeverity = 0;  // the higher, the worse
	char cornerPenaltySeverity = 0;  // the higher, the worse
	bool frameAdvantageValid:1;
	bool landingFrameAdvantageValid:1;
	bool idle:1;  // is able to perform a non-cancel move
	bool idlePlus:1;  // is able to perform a non-cancel move. Jump startup and landing are considered 'idle'
	bool idleLanding:1;  // is able to perform a non-cancel move. Time spent in the air after recovering from an air move is considered 'not idle'
	bool idleForFramebar:1;
	bool startedUp:1;  // if true, recovery frames or gaps in active frames are measured instead of startup
	bool onTheDefensive:1;  // true when blocking or being combo'd/hit, or when teching or waking up
	bool landingOrPreJump:1;  // becomes true when transitioning from idle to prejump/landing. Becomes false when exiting prejump/landing
	bool isLanding:1;  // on this frame, is it landing animation or the first frame of a customized landing animation
	bool isLandingOrPreJump:1;  // on this frame, is it either landing or prejump animation or the first frame of a customized landing animation
	// you recovered in the air. Upon next landing, don't treat it as "busy"
	// for the purposes of the air frame advantage calculator
	bool dontRestartTheNonLandingFrameAdvantageCountdownUponNextLanding:1;
	// needed for May dolphin riding and Air Blitz Shield (whiff): custom landing animation.
	// This makes it so that when the player touches the ground, the remainder of their busy,
	// non-idle state is considered to be landing recovery.
	// If the animation changes to another one, this has to be reset.
	bool theAnimationIsNotOverYetLolConsiderBusyNonAirborneFramesAsLandingAnimation:1;
	bool airborne:1;  // is y > 0 or speed y != 0. Note that tumbling state and pre-landing frame may be y == 0, and getting hit by Greed Sever puts you airborne at y == 0, so also check speedY == 0
	bool inHitstun:1;  // being combo'd. I guess this should be called inHitstun
	bool gettingUp:1;  // playing a wakeup animation
	bool wasIdle:1;  // briefly became idle during the frame while transitioning through some animations
	bool startedDefending:1;  // triggers restart of frame advantage measurement
	bool moveOriginatedInTheAir:1;  // for measuring landing recovery of moves that started in the air only
	bool setHitstopMax:1;  // during this logic tick, from some hook, hitstopMax field was updated
	bool setHitstopMaxSuperArmor:1;  // during this logic tick, from some hook, hitstopMaxSuperArmor field was updated
	bool setHitstunMax:1;  // during this logic tick, from some hook, hitstunMax field was updated
	bool setBlockstunMax:1;  // during this logic tick, from some hook, blockstunMax field was updated
	bool displayHitstop:1;  // should hitstop be displayed in UI
	bool oppoWasTouchingWallOnFD:1;  // used to calculate FD pushback modifier on the display
	bool receivedSpeedYValid:1;  // should display received speed Y, instead of "???"
	bool hitstunProrationValid:1;  // should display hitstun proration, instead of "--"
	bool hitSomething:1;  // during this logic tick, hit someone with own (non-projectile) active frames
	bool changedAnimOnThisFrame:1;
	bool changedAnimFiltered:1; // changedAnimOnThisFrame but with extra checks
	bool inNewMoveSection:1;  // see Moves.h:MoveInfo::sectionSeparator
	bool idleInNewSection:1;  // see Moves.h:MoveInfo::considerIdleInSeparatedSectionAfterThisManyFrames
	bool frameAdvantageIncludesIdlenessInNewSection:1;  // since frame advantage gets corrected after becoming idle in new section, we need to track if we changed it already
	bool landingFrameAdvantageIncludesIdlenessInNewSection:1;  // since frame advantage gets corrected after becoming idle in new section, we need to track if we changed it already
	bool airteched:1;
	
	// These fields are needed because when tap Blitz Shield rejects an attack,
	// it only enables normals after hitstop at the end of the logic tick, when
	// it doesn't matter anymore for that tick, so what we end up seeing is
	// enableNormals == true, but for the most duration of the tick it was actually
	// enableNormals == false. These fields hold the values at the moment of
	// dicision making of which move you're doing.
	// Also this is needed for superfreezes, because on the frame after superfreeze
	// you can't initiate a 5P, but enableNormals would say yes. So technically
	// enableNormals is not true on that frame.
	bool wasEnableNormals:1;
	bool wasEnableGatlings:1;
	bool wasEnableWhiffCancels:1;
	bool obtainedForceDisableFlags:1;
	
	bool enableBlock:1;  // this holds the raw value of ent.enableBlock() flag
	bool canBlock:1;  // this may either contain the value from enableBlock field or the result of the decision override by the current move
	
	bool isInFDWithoutBlockstun:1;
	
	bool armoredHitOnThisFrame:1;  // for super armors showing correct hitstop max
	bool gotHitOnThisFrame:1;  // for super armors showing correct hitstop max
	bool baikenReturningToBlockstunAfterAzami:1;  // for Baiken azamiing a hit and not doing a followup. She puts herself in blockstun, but this blockstun takes effect immediately,
	                                              // because it has no hitstop, so there's no need to decrement it by 1
	bool ignoreNextInabilityToBlockOrAttack:1;  // When next you become unable to block, do not include that as part of the move, into totalCanBlock
	bool inBlockstunNextFrame:1;  // This flag is needed so that when you transfer blockstun from air to ground the blockstunMax doesn't get reset,
	                              // because normally it would, because technically you changed animation and we don't treat all blockstun animations
	                              // as the same animation yet. If we allow such reset we will wrongfully decrement blockstunMax by 1 in our next prepareDrawData call
	bool leftHitstop:1;
	bool hasDangerousProjectiles:1;
	
	bool projectileOnlyInvul:1;
	bool strikeInvul:1;
	bool throwInvul:1;
	bool superArmorActive:1;
	bool counterhit:1;
	
	CharacterType charType = CHARACTER_TYPE_SOL;
	char anim[32] { 0 };
	char animIntraFrame[32] { '\0' };
	char index = 0;  // the index of this PlayerInfo in endScene's 'players' array
	inline void clearGaps() { gapsCount = 0; }
	void addGap(int length = 1);
	void printGaps(char* buf, size_t bufSize);
	void clear();
	void copyTo(PlayerInfo& dest);
	void printStartup(char* buf, size_t bufSize);
	void printRecovery(char* buf, size_t bufSize);
	void printTotal(char* buf, size_t bufSize);
	bool isIdleInNewSection();
};
