#pragma once
#include "pch.h"
#include <vector>
#include "PlayerInfo.h"
#include "InputRingBuffer.h"
#include "InputRingBufferStored.h"
#include "DrawHitboxArrayCallParams.h"
#include "ThreadUnsafeSharedPtr.h"

struct ThrowInfo {
	Entity owner{ nullptr };
	bool isPawn = false;
	bool isMettagiri = false;
	bool isThrow = false;
	AttackType attackType = ATTACK_TYPE_NONE;

	bool hasPushboxCheck = false;
	int throwRange = 0;
	int pushboxCheckMinX = 0;
	int pushboxCheckMaxX = 0;

	bool hasXCheck = false;
	int minX = 0;
	int maxX = 0;

	bool hasYCheck = false;
	int minY = 0;
	int maxY = 0;

	bool leftUnlimited = true;
	int left = 0;
	bool rightUnlimited = true;
	int right = 0;
	bool topUnlimited = true;
	int top = 0;
	bool bottomUnlimited = true;
	int bottom = 0;
	bool active = true;
	int framesLeft = 0;
	bool firstFrame = true;
	
	bool hatched = false;
	int originX = 0;
	int originY = 0;
};
struct DetectedHitboxes {
	Entity entity{nullptr};
	bool isPawn = false;
	int team = 0;
	DrawHitboxArrayCallParams hitboxes;
	int activeTime = 0;  // this is needed for Chipp's Gamma Blade, it stops being active on the frame after it hits
	int counter = 0;
	unsigned int previousTime = 0;
	int hitboxesCount = 0;
	bool timeHasChanged(bool globalTimeHasChanged);
	bool entityInTheList = false;
	bool entityInTheListAndActive = false;
};
struct Rejection {
	Entity owner{nullptr};
	int left = 0;
	int top = 0;
	int right = 0;
	int bottom = 0;
	int counter = 0;
	int skipFrame = 0;
	int activeFrame = 0;
	bool hatched = false;
	int originX = 0;
	int originY = 0;
	bool firstFrame = false;
};

enum SkippedFramesType : char {
	SKIPPED_FRAMES_SUPERFREEZE,
	SKIPPED_FRAMES_HITSTOP,
	SKIPPED_FRAMES_GRAB,
	SKIPPED_FRAMES_SUPER
};

struct SkippedFramesElement {
	SkippedFramesType type;
	unsigned short count;
};

struct SkippedFramesInfo {
	SkippedFramesElement elements[4];
	char count:7;
	char overflow:1;
	void addFrame(SkippedFramesType type);
	void clear();
	void transitionToOverflow();
	void print(bool canBlockButNotFD_ASSUMPTION) const;
};

struct EndSceneStoredState {
	std::vector<PlayerInfo> players{2};
	std::vector<ProjectileInfo> projectiles;
	
	bool measuringFrameAdvantage = false;
	int measuringLandingFrameAdvantage = -1;  // index of the player who is in the air and needs to land
	
	int tensionRecordedHit[2] { 0 };
	int burstRecordedHit[2] { 0 };
	int tensionGainOnLastHit[2] { 0 };
	bool tensionGainOnLastHitUpdated[2] { 0 };
	int burstGainOnLastHit[2] { 0 };
	bool burstGainOnLastHitUpdated[2] { 0 };
	
	DWORD prevAswEngineTickCount = 0;  // it's the tick that was at the time of simulating the step. It's called prev, because when the next tick comes, it will be the previous one
	DWORD prevAswEngineTickCountForInputs = 0;
	struct RegisteredHit {
		ProjectileInfo projectile;
		HitResult hitResult;
		Entity attacker;
		Entity defender;
		bool hasHitbox;
		bool isPawn;
	};
	std::vector<RegisteredHit> registeredHits;
	struct LeoParry {
		int x;
		int y;
		int timer;
		DWORD aswEngTick;
	};
	std::vector<LeoParry> leoParries;
	struct OccuredEvent {
		enum OccuredEventType {
			SET_ANIM,
			SIGNAL
		} type;
		union OccuredEventUnion {
			struct OccuredEventSetAnim {
				Entity pawn;
				char fromAnim[32];
			} setAnim;
			struct OccuredEventSignal {
				Entity from;
				Entity to;
				char fromAnim[32];  // this is needed from Bedman 236H's bomb1 being created by Flying_bomb1.
									// Flying_bomb1 disappears on that very frame and is never actually visible,
									// and we get a different animation string ("423wind") when reading from its pointer
				CreatedProjectileStruct creatorName;
			} signal;
			inline OccuredEventUnion() { }
		} u;
	};
	std::vector<OccuredEvent> events;
	int framebarPosition = 0;
	int framebarTotalFramesUnlimited = 0;
	int framebarPositionHitstop = 0;
	int framebarTotalFramesHitstopUnlimited = 0;
	int framebarIdleFor = 0;
	int framebarIdleHitstopFor = 0;
	int superfreezeHasBeenGoingFor = 0;
	int superflashCounterAllied = 0;
	int superflashCounterAlliedMax = 0;
	int superflashCounterOpponent = 0;
	int superflashCounterOpponentMax = 0;
	Entity lastNonZeroSuperflashInstigator;
	InputRingBuffer prevInputRingBuffers[2] { InputRingBuffer{}, InputRingBuffer{} };
	Input lastFewInputs[2][50];
	int lastFewInputsSize = 0;
	DWORD lastFramebarClearTime = 0;
	SkippedFramesInfo nextSkippedFrames;
	SkippedFramesInfo nextSkippedFramesIdle;
	SkippedFramesInfo nextSkippedFramesHitstop;
	SkippedFramesInfo nextSkippedFramesIdleHitstop;
	bool isFirstTickOfAMatch = false;
	bool startedNewRound = false;
	int reachedMaxStun[2] { -1, -1 };  // on this frame
	struct AttackHitbox {
		DrawHitboxArrayCallParams hitbox;
		bool notClash = false;
		bool clash = false;
		int count = 0;
		Entity ent { nullptr };
		bool found = false;
		int team = 2;
	};
	// this is needed to display hitboxes of projectiles that disappear when their player is hit, on the very frame their player gets hit.
	// Without this, those projectiles would get deleted by the end of the tick and we would not see their hitboxes.
	// While in reality, projectiles run hit collision before players do, and there may be some order between different projectiles,
	// so such projectiles could potentially deal hits on that very frame.
	std::vector<AttackHitbox> attackHitboxes;
	bool lastRoundendContainedADeath = false;
	struct PunishMessageTimer {
		int animFrame = -1;
		int currentAnimFrame = -1;
		int animFrameRepeatCount = 0;
		float yOff = 0.F;
		float xOff = 0.F;
	};
	std::array<std::vector<PunishMessageTimer>, 2> punishMessageTimers;
	bool attackerInRecoveryAfterBlock[2] { false, false };
	bool attackerInRecoveryAfterCreatingProjectile[2][75] { false };
	
	struct {
		std::vector<DetectedHitboxes> hitboxesThatHit;
		std::vector<DetectedHitboxes> hurtboxesThatGotHit;
		std::vector<Rejection> rejections;
	
		DWORD previousTime = 0;
		bool timeHasChanged = false;
	} hitDetector;
	
	struct {
		std::vector<ThrowInfo> infos;
		DWORD previousTime = 0;
	} throws;
};

