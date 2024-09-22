#pragma once
#include "Entity.h"
struct ActiveData {
	short actives = 0;
	short nonActives = 0;
};

struct ActiveDataArray {
	int count = 0;
	int prevHitNum = -1;
	ActiveData data[60] { 0 };
	inline void addNonActive(int n = 1) {
		if (count == 0) {
			return;
		}
		data[count - 1].nonActives += n;
	}
	void addActive(int hitNum = -1, int n = 1, bool forceNewHit = false);
	inline void clear() { count = 0; }
	void print(char* buf, size_t bufSize);
};

struct ProjectileInfo {
	Entity ptr = 0;
	int team = 0;
	int lifeTimeCounter = 0;
	int animFrame = 0;
	int hitstop = 0;
	int nextHitstop = 0;
	int startup = 0;
	ActiveDataArray actives { 0 };
	char animName[32] { 0 };
	bool ignoreHitstop:1;
	bool markActive:1;
	bool startedUp:1;
	bool cameFromHitDetect:1;
	ProjectileInfo() :
		ignoreHitstop(false),
		markActive(false),
		startedUp(false),
		cameFromHitDetect(false)
	{
	}
	void fill(Entity ent) {
		ptr = ent;
		team = ent.team();
		animFrame = ent.currentAnimDuration();
		nextHitstop = ent.hitstop();
		ignoreHitstop = !hitstop && nextHitstop;
		lifeTimeCounter = ent.lifeTimeCounter();
		memcpy(animName, ent.animationName(), 32);
	}
};

struct PlayerInfo {
	int hp = 0;
	int maxHp = 0;
	int defenseModifier = 0;  // dmg = dmg * ((256 + defenseModifier) / 256)
	int gutsRating = 0;
	int gutsPercentage = 0;
	int risc = 0;  // max 12800
	int tension = 0;  // max 10000
	int tensionPulse = 0;  // -25000 to 25000
	int negativePenaltyTimer = 0;  // time remaining until negative penalty wears off
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
	int stun = 0;
	int stunThreshold = 0;
	int blockstun = 0;
	int hitstun = 0;
	int hitstop = 0;
	int nextHitstop = 0;
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
	int startup = 0;
	ActiveDataArray actives { 0 };
	int recovery = 0;
	int total = 0;
	int landingOrPreJumpFrames = 0;
	int animFrame = 0;
	char tensionPulsePenaltySeverity = 0;
	char cornerPenaltySeverity = 0;
	bool frameAdvantageValid = false;
	bool landingFrameAdvantageValid = false;
	bool idle:1;  // is able to perform a non-cancel move
	bool idleNext:1;  // the precomputed new value for 'idle'. We need this because a change of idle is checked twice in separate loops
	bool idlePlus:1;  // is able to perform a non-cancel move. Jump startup and landing are considered 'idle' if not immediately done before/after a move
	bool idleLanding:1;  // is able to perform a non-cancel move. Time spent in the air after recovering from an air move is considered 'not idle'
	bool startedUp:1;  // if true, recovery frames or gaps in active frames are measured instead of startup
	bool onTheDefensive:1;  // true when blocking or being combo'd/hit, or when teching or waking up
	bool landingOrPreJump:1;
	bool isLanding:1;
	bool isLandingOrPreJump:1;
	bool needLand:1;  // recovery from an air move happened and now landing is needed to measure frame advantage on landing
	bool airborne:1;
	bool inPain:1;
	bool gettingUp:1;
	bool ignoreHitstop:1;
	CharacterType charType = CHARACTER_TYPE_SOL;
	char anim[32] { 0 };
	inline void clearGaps() { gapsCount = 0; }
	void addGap(int length = 1);
	void printGaps(char* buf, size_t bufSize);
	void clear();
	void copyTo(PlayerInfo& dest);
};
