#pragma once
#include "Entity.h"
struct ActiveData {
	int actives = 0;
	int nonActives = 0;
};

struct ActiveDataArray {
	int count = 0;
	ActiveData data[32] { 0 };
	inline void addNonActive(int n = 1) {
		if (count == 0) {
			return;
		}
		data[count - 1].nonActives += n;
	}
	void addActive(int n = 1);
	inline void clear() { count = 0; }
	void print(char* buf, size_t bufSize);
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
	int timePassed = 0;
	int timePassedLanding = 0;
	int hitboxesCount = 0;
	int activeProjectilesCount = 0;
	Entity activeProjectiles[32] { nullptr };
	bool hasNewActiveProjectiles = false;
	int superfreezeStartup = 0;
	int projectileStartup = 0;
	ActiveDataArray projectileActives { 0 };
	int timeSinceLastBusyStart = 0;
	int timeSinceLastActiveProjectile = 0;
	int startup = 0;
	ActiveDataArray actives { 0 };
	int recovery = 0;
	int total = 0;
	int landingOrPreJumpFrames = 0;
	int remainingDoubleJumps = 0;
	int remainingAirDashes = 0;
	char tensionPulsePenaltySeverity = 0;
	char cornerPenaltySeverity = 0;
	bool frameAdvantageValid = false;
	bool landingFrameAdvantageValid = false;
	bool idle:1;
	bool idleNext:1;
	bool idlePlus:1;
	bool idleLanding:1;
	bool startedUp:1;
	bool projectileStartedUp:1;
	bool onTheDefensive:1;
	bool landingOrPreJump:1;
	bool landed:1;
	bool isLanding:1;
	bool isLandingOrPreJump:1;
	bool needLand:1;
	bool airborne:1;
	bool inPain:1;
	bool wasCombod:1;
	bool gettingUp:1;
	CharacterType charType = CHARACTER_TYPE_SOL;
	char anim[32] { 0 };
	void removeActiveProjectile(int index);
	int findActiveProjectile(Entity ent);
	void addActiveProjectile(Entity ent);
	inline void clearGaps() { gapsCount = 0; }
	void addGap(int length = 1);
	void printGaps(char* buf, size_t bufSize);
};
