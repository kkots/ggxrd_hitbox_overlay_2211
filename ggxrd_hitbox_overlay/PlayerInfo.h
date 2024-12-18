#pragma once
#include "Entity.h"
#include "Moves.h"
#include <string>
#include <vector>

struct CanProgramSecretGardenInfo {
	DWORD can:1;
	DWORD inputs:3;
	DWORD inputsMax:3;
};

struct ChippInfo {
	unsigned short invis;
	unsigned short wallTime;
};

struct DIInfo {
	unsigned short current;
	unsigned short max;
};

struct GatlingOrWhiffCancelInfo {
	const char* name;
	const char* replacementInputs;
	const AddedMoveData* move;
	int iterationIndex;
	bool nameIncludesInputs;
	int bufferTime;
};

struct FrameCancelInfo {
	std::vector<GatlingOrWhiffCancelInfo> gatlings;
	std::vector<GatlingOrWhiffCancelInfo> whiffCancels;
	const char* whiffCancelsNote = nullptr;
	FrameCancelInfo() = default;
	void clear();
};

enum FrameType : char {
	FT_NONE,
	FT_IDLE,
	FT_IDLE_PROJECTILE,
	FT_IDLE_CANT_BLOCK,
	FT_IDLE_CANT_FD,
	FT_IDLE_AIRBORNE_BUT_CAN_GROUND_BLOCK,
	FT_IDLE_ELPHELT_RIFLE,
	FT_IDLE_ELPHELT_RIFLE_READY,
	FT_HITSTOP,
	FT_ACTIVE,
	FT_ACTIVE_PROJECTILE,
	FT_ACTIVE_HITSTOP,
	FT_ACTIVE_HITSTOP_PROJECTILE,
	FT_ACTIVE_NEW_HIT,
	FT_ACTIVE_NEW_HIT_PROJECTILE,
	FT_STARTUP,
	FT_STARTUP_ANYTIME_NOW,
	FT_STARTUP_ANYTIME_NOW_CAN_ACT,
	FT_STARTUP_STANCE,
	FT_STARTUP_STANCE_CAN_STOP_HOLDING,
	FT_STARTUP_CAN_BLOCK,
	FT_STARTUP_CAN_BLOCK_AND_CANCEL,
	FT_STARTUP_CAN_PROGRAM_SECRET_GARDEN,
	FT_ZATO_BREAK_THE_LAW_STAGE2,
	FT_ZATO_BREAK_THE_LAW_STAGE3,
	FT_ZATO_BREAK_THE_LAW_STAGE2_RELEASED,
	FT_ZATO_BREAK_THE_LAW_STAGE3_RELEASED,
	FT_RECOVERY,
	FT_RECOVERY_HAS_GATLINGS,
	FT_RECOVERY_CAN_ACT,
	FT_RECOVERY_CAN_RELOAD,
	FT_NON_ACTIVE,
	FT_NON_ACTIVE_PROJECTILE,
	FT_PROJECTILE,
	FT_LANDING_RECOVERY,
	FT_LANDING_RECOVERY_CAN_CANCEL,
	FT_XSTUN,
	FT_XSTUN_CAN_CANCEL,
	FT_XSTUN_HITSTOP,
	FT_GRAYBEAT_AIR_HITSTUN,
	FT_LAST  // must always be last
};
inline bool frameTypeDiscardable(FrameType type) {
	return type == FT_NONE || type == FT_IDLE || type == FT_IDLE_PROJECTILE;
}
extern bool cantAttackTypes[FT_LAST];
void initializeCantAttackTypes();
inline bool frameTypeAssumesCantAttack(FrameType type) {
	return cantAttackTypes[type];
}
inline bool frameTypeActive(FrameType type) {
	return type == FT_ACTIVE || type == FT_ACTIVE_PROJECTILE;
}
static FrameType landingRecoveryTypes[] {
	FT_LANDING_RECOVERY,
	FT_LANDING_RECOVERY_CAN_CANCEL
};
static FrameType recoveryFrameTypes[] {
	FT_RECOVERY,
	FT_RECOVERY_HAS_GATLINGS,
	FT_RECOVERY_CAN_ACT,
	FT_RECOVERY_CAN_RELOAD
};
static FrameType projectileFrameTypes[] {
	FT_IDLE_PROJECTILE,
	FT_ACTIVE_PROJECTILE,
	FT_ACTIVE_NEW_HIT_PROJECTILE,
	FT_ACTIVE_HITSTOP_PROJECTILE,
	FT_NON_ACTIVE_PROJECTILE
};

struct FrameStopInfo {
	unsigned char value:7;  // hitstun, blockstun or hitstop
	bool isHitstun:1;
	unsigned char valueMax:7;  // hitstunMax, blockstunMax or hitstopMax
	bool isBlockstun:1;
};

void printFameStop(char* buf, size_t bufSize, const FrameStopInfo* stopInfo, int hitstop, int hitstopMax);

struct FrameBase {
	DWORD aswEngineTick;
	const char* animName;
};

// This struct is initialized by doing memset to 0. Make sure every child struct is ok to memset to 0.
// This means that types like std::vector are not allowed.
struct Frame : public FrameBase {
	short skippedSuperfreeze;  // this value is same for all framebars and happens rarely. However if we start allocating vectors for each nuance like this how much will we actually save?
	FrameType type;
	unsigned char hitstop;  // because of danger time can go up to 99
	unsigned char hitstopMax;
	unsigned char skippedHitstop;
	unsigned short rcSlowdown:6;
	unsigned short isFirst:1;
	unsigned short hitConnected:1;  // true only for the frame on which a hit connected
	unsigned short rcSlowdownMax:6;
	unsigned short hitstopConflict:1;
	unsigned short newHit:1;
};

// This struct is initialized by doing memset to 0. Make sure every child struct is ok to memset to 0.
// This means that types like std::vector are not allowed.
struct PlayerFrame : public FrameBase {
	FrameCancelInfo cancels;
	union {
		CanProgramSecretGardenInfo canProgramSecretGarden;
		ChippInfo chippInfo;
		DIInfo diInfo;
	} u;
	short skippedSuperfreeze;
	short poisonDuration;
	FrameStopInfo stop;
	FrameType type;
	unsigned char hitstop;  // because of danger time can go up to 99
	unsigned char hitstopMax;
	unsigned char skippedHitstop;
	unsigned char rcSlowdown;
	unsigned char rcSlowdownMax;
	
	
	bool isFirst:1;
	bool hitConnected:1;  // true only for the frame on which a hit connected
	bool hitstopConflict:1;
	bool newHit:1;
	
	bool enableNormals:1;
	bool canBlock:1;
	
	bool strikeInvulInGeneral:1;
	bool throwInvulInGeneral:1;
	bool superArmorActiveInGeneral:1;
	bool superArmorActiveInGeneral_IsFull:1;
	
	bool strikeInvul:1;
	bool throwInvul:1;
	bool lowProfile:1;
	bool frontLegInvul:1;
	bool projectileOnlyInvul:1;
	bool superArmor:1;
	bool superArmorThrow:1;
	bool superArmorBurst:1;
	bool superArmorMid:1;
	bool superArmorOverhead:1;
	bool superArmorLow:1;
	bool superArmorGuardImpossible:1;
	bool superArmorObjectAttacck:1;
	bool superArmorHontaiAttacck:1;
	bool superArmorProjectileLevel0:1;
	bool superArmorOverdrive:1;
	bool superArmorBlitzBreak:1;
	bool reflect:1;
	
	bool hitOccured:1;  // stays true for the remainder of the move
	bool enableSpecialCancel:1;
	bool enableSpecials:1;
	bool airborne:1;
	
	bool crossupProtectionIsOdd:1;
	bool crossupProtectionIsAbove1:1;
	
	void printInvuls(char* buf, size_t bufSize) const;
	void clear();
};

// This struct is initialized by doing memset to 0. Make sure every child struct is ok to memset to 0.
// This means that types like std::vector are not allowed.
struct FramebarBase {
	virtual void copyFrame(FrameBase& destFrame, const FrameBase& srcFrame) const = 0;
	virtual void copyFrame(FrameBase& destFrame, FrameBase&& srcFrame) const = 0;
	virtual void clear() = 0;
	virtual int findTickNoGreaterThan(int startingPos, DWORD tick) const = 0;
	virtual void soakUpIntoPreFrame(const FrameBase& srcFrame) = 0;
	virtual void processRequests(FrameBase& destinationFrame) = 0;
	virtual void processRequests(int destinationPosition) = 0;
	virtual void copyRequests(FramebarBase& source) = 0;
	virtual void clearRequests() = 0;
	virtual void catchUpToIdle(FramebarBase& source, int destinationStartingPosition, int framesToCatchUpFor) = 0;
	virtual FrameBase& getFrame(int index) = 0;
	FrameType preFrame = FT_NONE;
	DWORD preFrameLength = 0;
	int skippedHitstop = 0;
	bool requestFirstFrame = false;
	bool requestNextHit = false;
	bool completelyEmpty = false;
};

// This struct is initialized by doing memset to 0. Make sure every child struct is ok to memset to 0.
// This means that types like std::vector are not allowed.
struct Framebar : public FramebarBase {
	Frame frames[80] { Frame{} };
	inline Frame& operator[](int index) { return frames[index]; }
	inline const Frame& operator[](int index) const { return frames[index]; }
	virtual void copyFrame(FrameBase& destFrame, const FrameBase& srcFrame) const override;
	virtual void copyFrame(FrameBase& destFrame, FrameBase&& srcFrame) const override;
	virtual void clear() override;
	virtual int findTickNoGreaterThan(int startingPos, DWORD tick) const override;
	virtual void soakUpIntoPreFrame(const FrameBase& srcFrame) override;
	virtual void processRequests(FrameBase& destinationFrame) override;
	virtual void processRequests(int destinationPosition) override;
	virtual void copyRequests(FramebarBase& source) override;
	virtual void clearRequests() override;
	virtual void catchUpToIdle(FramebarBase& source, int destinationStartingPosition, int framesToCatchUpFor) override;
	virtual FrameBase& getFrame(int index) override;
};

// This struct is initialized by doing memset to 0. Make sure every child struct is ok to memset to 0.
// This means that types like std::vector are not allowed.
struct PlayerFramebar : public FramebarBase {
	PlayerFrame frames[80] { PlayerFrame{} };
	inline PlayerFrame& operator[](int index) { return frames[index]; }
	inline const PlayerFrame& operator[](int index) const { return frames[index]; }
	virtual void copyFrame(FrameBase& destFrame, const FrameBase& srcFrame) const override;
	virtual void copyFrame(FrameBase& destFrame, FrameBase&& srcFrame) const override;
	virtual void clear() override;
	virtual int findTickNoGreaterThan(int startingPos, DWORD tick) const override;
	virtual void soakUpIntoPreFrame(const FrameBase& srcFrame) override;
	virtual void processRequests(FrameBase& destinationFrame) override;
	virtual void processRequests(int destinationPosition) override;
	virtual void copyRequests(FramebarBase& source) override;
	virtual void clearRequests() override;
	virtual void catchUpToIdle(FramebarBase& source, int destinationStartingPosition, int framesToCatchUpFor) override;
	virtual FrameBase& getFrame(int index) override;
	void clearCancels();
	void clearCancels(int index);
};

struct EntityFramebar {
	EntityFramebar() = default;
	EntityFramebar(int playerIndex, int id) : playerIndex(playerIndex), id(id) { }
	int playerIndex = -1;
	int id = -1;
	const MoveInfo* move = nullptr;
	const char* titleShort = nullptr;
	const char* titleFull = nullptr;
	bool foundOnThisFrame = false;
	virtual void copyFrame(FrameBase& destFrame, const FrameBase& srcFrame) const = 0;
	virtual void copyFrame(FrameBase& destFrame, FrameBase&& srcFrame) const = 0;
	void setTitle(const char* text, const char* textFull = nullptr);
	void copyTitle(const EntityFramebar& source);
	inline void changePreviousFramesOneType(FrameType prevType,
			FrameType newType,
			int positionHitstopIdle,
			int positionHitstop,
			int positionIdle,
			int position,
			int maxCount,
			bool stopAtFirstFrame = false) {
		changePreviousFrames(&prevType,
			1,
			newType,
			positionHitstopIdle,
			positionHitstop,
			positionIdle,
			position,
			maxCount,
			stopAtFirstFrame);
	}
	virtual void changePreviousFrames(FrameType* prevTypes,
		int prevTypesCount,
		FrameType newType,
		int positionHitstopIdle,
		int positionHitstop,
		int positionIdle,
		int position,
		int maxCount,
		bool stopAtFirstFrame = false) = 0;
	static int confinePos(int pos);
	inline static void decrementPos(int& pos) { if (pos == 0) pos = _countof(PlayerFramebar::frames) - 1; else --pos; }
	inline static void incrementPos(int& pos) { if (pos == _countof(PlayerFramebar::frames) - 1) pos = 0; else ++pos; }
	inline bool belongsToProjectile() const { return id != -1; }
	inline bool belongsToPlayer() const { return id == -1; }
	virtual FramebarBase& getMain() = 0;
	virtual FramebarBase& getHitstop() = 0;
	virtual FramebarBase& getIdle() = 0;
	virtual FramebarBase& getIdleHitstop() = 0;
	virtual const FramebarBase& getMain() const = 0;
	virtual const FramebarBase& getHitstop() const = 0;
	virtual const FramebarBase& getIdle() const = 0;
	virtual const FramebarBase& getIdleHitstop() const = 0;
	// maxCodepointCount - pass in a pointer to the maximum number of whole characters (codepoints). This
	//  number of characters will be included in the byte-length that will be returned via the same
	//  (maxCodepointCount) pointer.
	
	/// <summary>
	/// Determines the length of the UTF8 encoded string in both bytes, not including the terminating null character,
	/// and codepoints (whole UTF32 characters), as well as returns the number of bytes, not including the terminating
	/// null character, that the left portion of the string of given maximum codepoint length is occupying.
	/// </summary>
	/// <param name="txt">The UTF8 encoded string, must be null terminated.</param>
	/// <param name="byteLen">Returns the total length of the string, in bytes, not including the terminating null character.</param>
	/// <param name="cpCountTotal">Returns the total number of codepoints in the string, not including the terminating null character.</param>
	/// <param name="maxCodepointCount">Provide the maximum number of codepoints to use to calculate byteLenBelowMax.</param>
	/// <param name="byteLenBelowMax">Uses the first maxCodepointCount codepoints of the string to calculate their byte length
	/// and return it. Does not include terminating null characters in either the calculation or the return value.
	/// If the provided maxCodepointCount is more than the total number of codepoints in the string, returns the same
	/// value as byteLen.</param>
	static void utf8len(const char* txt, int* byteLen, int* cpCountTotal, int maxCodepointCount, int* byteLenBelowMax);
};

struct ProjectileFramebar : public EntityFramebar {
	ProjectileFramebar() = default;
	ProjectileFramebar(int playerIndex, int id) : EntityFramebar(playerIndex, id) {}
	Framebar main { };  // the one framebar that is displayed
	Framebar idle { };  // because we don't update the framebar when players are idle and reset it when an action beghins after
	                            // EndScene::framebarIdleForLimit f of idle, if an action begins before EndScene::framebarIdleForLimit f
	                            // of idle with some non-zero f idle time, we need to display what happened during that
	                            // idle time so we will take that information from here. This framebar shall be updated even during idle time
	Framebar hitstop { };  // we omit hitstop in the main framebar, but there's a setting to show it anyway, and we want the
	                               // framebar to update upon changing this setting without having to re-record the action.
	                               // Hence this framebar works in parallel with the main one, with the one difference that it always records hitstop
	Framebar idleHitstop { };  // information from this gets copied to the hitstop framebar when the omitted idle frames are needed as
	                                   // described in the 'idle' framebar
	virtual void copyFrame(FrameBase& destFrame, const FrameBase& srcFrame) const override;
	virtual void copyFrame(FrameBase& destFrame, FrameBase&& srcFrame) const override;
	virtual void changePreviousFrames(FrameType* prevTypes,
		int prevTypesCount,
		FrameType newType,
		int positionHitstopIdle,
		int positionHitstop,
		int positionIdle,
		int position,
		int maxCount,
		bool stopAtFirstFrame = false) override;
	virtual FramebarBase& getMain() override;
	virtual FramebarBase& getHitstop() override;
	virtual FramebarBase& getIdle() override;
	virtual FramebarBase& getIdleHitstop() override;
	virtual const FramebarBase& getMain() const override;
	virtual const FramebarBase& getHitstop() const override;
	virtual const FramebarBase& getIdle() const override;
	virtual const FramebarBase& getIdleHitstop() const override;
};
	
struct CombinedProjectileFramebar : public ProjectileFramebar {
	CombinedProjectileFramebar() = default;
	CombinedProjectileFramebar(int playerIndex, int id) : ProjectileFramebar(playerIndex, id) {}
	Framebar main { };  // the one framebar that is displayed
	const ProjectileFramebar* sources[_countof(Framebar::frames)] { nullptr };
	virtual void changePreviousFrames(FrameType* prevTypes,
		int prevTypesCount,
		FrameType newType,
		int positionHitstopIdle,
		int positionHitstop,
		int positionIdle,
		int position,
		int maxCount,
		bool stopAtFirstFrame = false) override;
	virtual FramebarBase& getMain() override;
	virtual FramebarBase& getHitstop() override;
	virtual FramebarBase& getIdle() override;
	virtual FramebarBase& getIdleHitstop() override;
	virtual const FramebarBase& getMain() const override;
	virtual const FramebarBase& getHitstop() const override;
	virtual const FramebarBase& getIdle() const override;
	virtual const FramebarBase& getIdleHitstop() const override;
	bool canBeCombined(const Framebar& source) const;
	void combineFramebar(const Framebar& source, const ProjectileFramebar* dad);
	void determineName();
};

struct PlayerFramebars : public EntityFramebar {
	PlayerFramebar main { };  // the one framebar that is displayed
	PlayerFramebar idle { };  // because we don't update the framebar when players are idle and reset it when an action beghins after
	                            // EndScene::framebarIdleForLimit f of idle, if an action begins before EndScene::framebarIdleForLimit f
	                            // of idle with some non-zero f idle time, we need to display what happened during that
	                            // idle time so we will take that information from here. This framebar shall be updated even during idle time
	PlayerFramebar hitstop { };  // we omit hitstop in the main framebar, but there's a setting to show it anyway, and we want the
	                               // framebar to update upon changing this setting without having to re-record the action.
	                               // Hence this framebar works in parallel with the main one, with the one difference that it always records hitstop
	PlayerFramebar idleHitstop { };  // information from this gets copied to the hitstop framebar when the omitted idle frames are needed as
	                                   // described in the 'idle' framebar
	virtual void copyFrame(FrameBase& destFrame, const FrameBase& srcFrame) const override;
	virtual void copyFrame(FrameBase& destFrame, FrameBase&& srcFrame) const override;
	virtual void changePreviousFrames(FrameType* prevTypes,
		int prevTypesCount,
		FrameType newType,
		int positionHitstopIdle,
		int positionHitstop,
		int positionIdle,
		int position,
		int maxCount,
		bool stopAtFirstFrame = false) override;
	virtual FramebarBase& getMain() override;
	virtual FramebarBase& getHitstop() override;
	virtual FramebarBase& getIdle() override;
	virtual FramebarBase& getIdleHitstop() override;
	virtual const FramebarBase& getMain() const override;
	virtual const FramebarBase& getHitstop() const override;
	virtual const FramebarBase& getIdle() const override;
	virtual const FramebarBase& getIdleHitstop() const override;
};

struct ActiveData {
	short actives = 0;
	short nonActives = 0;
	inline bool operator==(const ActiveData& other) const { return actives == other.actives && nonActives == other.nonActives; }
	inline bool operator!=(const ActiveData& other) const { return !(*this == other); }
	inline short total() { return actives + nonActives; }
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
	int print(char* buf, size_t bufSize) const;
	// Print active frames, not including the final span of non-active frames.
	// Fuse distinct hits together into unified spans of active frames. Don't use comma (",") character.
	// Non-active frames are printed in parentheses ("(123)") inbetween the active frames.
	int printNoSeparateHits(char* buf, size_t bufSize) const;
	int printNoSeparateHitsGapsBiggerThan3(char* buf, size_t bufSize) const;
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
	inline ActiveData& last() { return data[count - 1]; }
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

struct MaxHitInfo {
	int current = 0;
	int max = 0;
	int currentUse = 0;
	int maxUse = 0;
	void fill(Entity player, int currentHitNum);
	inline void clear() { current = -1; max = -1; currentUse = -1; maxUse = -1; }
	inline bool empty() const { return currentUse == -1 && maxUse == -1; }
	inline bool operator==(const MaxHitInfo& other) { return currentUse == other.currentUse && maxUse == other.maxUse; }
	inline bool operator!=(const MaxHitInfo& other) { return !(*this == other); }
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
	int hitstopMax = 0;
	int clashHitstop = 0;
	int startup = 0;  // if active frames have not started yet, is equal to total. Otherwise, means time since the owning player has started their last move until active frames, inclusive
	int total = 0;  // time since the owning player started their last move
	int hitNumber = 0;  // updated every frame
	int numberOfHits = 0;
	
	int hitboxTopY = 0;
	int hitboxBottomY = 0;
	MaxHitInfo maxHit;
	bool hitboxTopBottomValid = false;
	
	DWORD creationTime_aswEngineTick = 0;
	ActiveDataArray actives;
	const MoveInfo* move = nullptr;
	PrevStartupsInfo prevStartups { 0 };
	SpriteFrameInfo sprite;
	int framebarId = -1;
	char creatorName[32] { 0 };
	Entity creator { nullptr };
	char animName[32] { 0 };
	bool markActive:1;  // cleared at the start of prepareDrawData. True means hitboxes were found on this frame, or on this logic tick this projectile registered a hit.
	bool startedUp:1;  // cleared upon disabling. True means active frames have started.
	bool landedHit:1;  // cleared at the start of each logic tick. Set to true from a hit registering function hook.
	bool disabled:1;  // set to true for all projectiles belonging to a player when the player starts a new move.
	bool inNewSection:1;
	bool isDangerous:1;
	bool superArmorActive:1;
	bool clashedOnThisFrame:1;
	bool immuneToRCSlowdown:1;
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

struct InvulData {
	bool active = false;
	int start = 0;
	ActiveDataArray frames;
	void clear();
	void addInvulFrame(int prevTotal);
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
	
	int hurtboxTopY = 0;
	int hurtboxBottomY = 0;
	bool hurtboxTopBottomValid = false;
	int hitboxTopY = 0;
	int hitboxBottomY = 0;
	bool hitboxTopBottomValid = false;
	
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
	int lastHitstopBeforeWipe = 0;
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
	int frameAdvantageNoPreBlockstun = 0;
	int landingFrameAdvantageNoPreBlockstun = 0;
	
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
	int totalCanFD = 0;  // total frames of the last move done directly by the character. Includes only frames where you can't FD
	
	int totalFD = 0;  // number of frames for which you were holding FD
	
	PrevStartupsInfo prevStartups { 0 };  // startups of moves that you whiff canceled from
	
	int startupDisp = 0;  // startup to display in the UI. Either current or of the last move
	ActiveDataArray activesDisp;  // active frames to display in the UI. Either current or of the last move
	MaxHitInfo maxHitDisp;
	int recoveryDisp = 0;  // recovery to display in the UI. Either current or of the last move. Includes only frames where you can't attack
	int recoveryDispCanBlock = -1;  // recovery until becoming able to block to display in the UI. Either current or of the last move. Includes only frames where you can't block. -1 means need to determine automatically
	int totalDisp = 0;  // total frames to display in the UI. Either current or of the last move. Includes only frames where you can't attack
	MaxHitInfo maxHit;
	MaxHitInfo maxHitUse;
	int hitNumber = 0;
	
	int startupProj = 0;  // startup of all projectiles. Either current or of the last move
	unsigned short startupProjIgnoredForFramebar = 0;
	ActiveDataArray activesProj;  // active frames of all projectiles. Either current or of the last move
	MaxHitInfo maxHitProj;
	bool maxHitProjConflict = false;
	Entity maxHitProjLastPtr = nullptr;
	
	PrevStartupsInfo prevStartupsDisp { 0 };  // things to add over a + sign in the displayed startup field
	PrevStartupsInfo prevStartupsTotalDisp { 0 };  // things to add over a + sign in the displayed 'Total' field
	//  this relies on there being only one active projectile for a move.
	//  it's a copy of previous startups of that projectile
	PrevStartupsInfo prevStartupsProj { 0 };
	
	InvulData strikeInvul { 0 };
	InvulData throwInvul { 0 };
	InvulData lowProfile { 0 };
	InvulData frontLegInvul { 0 };
	InvulData projectileOnlyInvul { 0 };
	InvulData superArmor { 0 };
	InvulData superArmorThrow { 0 };
	InvulData superArmorBurst { 0 };
	InvulData superArmorMid { 0 };
	InvulData superArmorOverhead { 0 };
	InvulData superArmorLow { 0 };
	InvulData superArmorGuardImpossible { 0 };
	InvulData superArmorObjectAttacck { 0 };
	InvulData superArmorHontaiAttacck { 0 };
	InvulData superArmorProjectileLevel0 { 0 };  // this flag only matters when it is absent, and it is present by default. Level 0 are unflickable projectiles
	InvulData superArmorOverdrive { 0 };
	InvulData superArmorBlitzBreak { 0 };
	InvulData reflect { 0 };
	
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
	char remainingDoubleJumps = 0;
	char wasProhibitFDTimer = 0;
	char rcSlowdownCounter = 0;
	char rcSlowdownMax = 0;
	unsigned short poisonDuration = 0;
	unsigned short poisonDurationMax = 0;
	EddieInfo eddie { 0 };
	
	DWORD moveStartTime_aswEngineTick = 0;
	AddedMoveData* standingFDMove = nullptr;
	AddedMoveData* crouchingFDMove = nullptr;
	
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
	bool inHitstun:1;  // being combo'd
	bool inHitstunNowOrNextFrame:1;  // being combo'd
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
	bool forceBusy:1;  // force startup/total to keep counting frames and the framebar to advance
	bool frameAdvantageIncludesIdlenessInNewSection:1;  // since frame advantage gets corrected after becoming idle in new section, we need to track if we changed it already
	bool landingFrameAdvantageIncludesIdlenessInNewSection:1;  // since frame advantage gets corrected after becoming idle in new section, we need to track if we changed it already
	bool airteched:1;
	bool wokeUp:1;
	
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
	bool wasEnableSpecials:1;
	bool wasEnableSpecialCancel:1;
	bool wasEnableAirtech:1;
	bool wasAttackCollidedSoCanCancelNow:1;
	bool obtainedForceDisableFlags:1;
	
	bool enableBlock:1;  // this holds the raw value of ent.enableBlock() flag
	bool canFaultlessDefense:1;  // this contains the result of a decision that determines whether you can FD based on information gathered during the logic tick
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
	bool hasDangerousProjectiles:1;
	
	bool counterhit:1;
	
	// Blitz Shield rejection changes super armor enabled and full invul flags at the end of a logic tick
	bool wasSuperArmorEnabled:1;
	bool wasFullInvul:1;
	bool immuneToRCSlowdown:1;
	bool wasHitOnPreviousFrame:1;
	bool wasHitOnThisFrame:1;
	
	CharacterType charType = CHARACTER_TYPE_SOL;
	char anim[32] { '\0' };
	char moveName[32] = { '\0' };
	char animIntraFrame[32] { '\0' };
	char moveNameIntraFrame[32] = { '\0' };
	char index = 0;  // the index of this PlayerInfo in endScene's 'players' array
	inline void clearGaps() { gapsCount = 0; }
	void addGap(int length = 1);
	void printGaps(char* buf, size_t bufSize);
	void clear();
	void copyTo(PlayerInfo& dest);
	void printStartup(char* buf, size_t bufSize);
	void printRecovery(char* buf, size_t bufSize);
	void printTotal(char* buf, size_t bufSize);
	void printInvuls(char* buf, size_t bufSize) const;
	bool isIdleInNewSection();
	bool isInVariableStartupSection();
	bool canPrintTotal() const;
	void setMoveName(char* destination, Entity ent);
	void addActiveFrame(Entity ent, PlayerFramebar& framebar);
	AddedMoveData* findMoveByName(const char* name) const;
	// use this function to determine whether the player is airborne when
	// the game's logic tick is still ongoing, as inside various game's function hooks for example
	inline bool airborne_insideTick() const { return pawn.ascending() || pawn.posY() > 0; }
	// use THIS function to determine whether the player is airborne after
	// the game's logic tick is over, in EndScene's prepareDrawData
	inline bool airborne_afterTick() const { return !(y == 0 && speedY == 0); }
	bool wasHadGatling(const char* name) const;
	bool wasHadWhiffCancel(const char* name) const;
	bool wasHadGatlings() const;
	bool wasHadWhiffCancels() const;
	CanProgramSecretGardenInfo canProgramSecretGarden() const;
};
