#pragma once
#include "characterTypes.h"

using getPos_t = int(__thiscall*)(const void*);
using getPushbox_t = int(__thiscall*)(const void*);
using getExtraTensionModifier_t = int(__thiscall*)(const void*, int param1);

struct EntityState {
	bool strikeInvuln;
	bool throwInvuln;
	bool isASummon;
	CharacterType charType;
	CharacterType ownerCharType;
	char team;
	bool counterhit;
	bool doingAThrow;
	bool isGettingThrown;
	unsigned int flagsField;
	bool inHitstunBlockstun;
	int posY;
};

struct WakeupTimings {
	int faceUp = 0;
	int faceDown = 0;
};

enum CmnActIndex {
	CmnActStand = 0x0,  // standing
	CmnActStandTurn = 0x1,  // turning around
	CmnActStand2Crouch = 0x2,  // switching from stand to crouching animation
	CmnActCrouch = 0x3,  // crouching
	CmnActCrouchTurn = 0x4,  // turning around while crouching
	CmnActCrouch2Stand = 0x5,  // switching from crouching to standing animation"
	CmnActJumpPre = 0x6,  // prejump
	CmnActJump = 0x7,  // jumping
	CmnActJumpLanding = 0x8,  // landing from a jump
	CmnActLandingStiff = 0x9,  // landing recovery after doing an air move, like Ky j.D
	CmnActFWalk = 0xa,  // walking forward
	CmnActBWalk = 0xb,  // walking backwards
	CmnActFDash = 0xc,  // runing
	CmnActFDashStop = 0xd,  // stoping running
	CmnActBDash = 0xe,  // backdash
	CmnActAirFDash = 0xf,  // airdashing forward
	CmnActAirBDash = 0x10,  // airdashing backwards
	CmnActNokezoriHighLv1 = 0x11,  // these are all hit animations
	CmnActNokezoriHighLv2 = 0x12,
	CmnActNokezoriHighLv3 = 0x13,
	CmnActNokezoriHighLv4 = 0x14,
	CmnActNokezoriHighLv5 = 0x15,
	CmnActNokezoriLowLv1 = 0x16,
	CmnActNokezoriLowLv2 = 0x17,
	CmnActNokezoriLowLv3 = 0x18,
	CmnActNokezoriLowLv4 = 0x19,
	CmnActNokezoriLowLv5 = 0x1a,
	CmnActNokezoriCrouchLv1 = 0x1b,
	CmnActNokezoriCrouchLv2 = 0x1c,
	CmnActNokezoriCrouchLv3 = 0x1d,
	CmnActNokezoriCrouchLv4 = 0x1e,
	CmnActNokezoriCrouchLv5 = 0x1f,
	CmnActBDownUpper = 0x20,  // knocked down backwards, moving upwards
	CmnActBDownUpperEnd = 0x21,  // seems to be unused
	CmnActBDownDown = 0x22,  // knocked down backwards, moving downwards
	CmnActBDownBound = 0x23,  // landing on the back
	CmnActBDownLoop = 0x24,  // lying on the back
	CmnActBDown2Stand = 0x25,  // standing up from having been lying on their back
	CmnActFDownUpper = 0x26,  // knocked down forward, moving upwards
	CmnActFDownUpperEnd = 0x27,  // seems to be unused
	CmnActFDownDown = 0x28,  // knocked down forward, moving downwards
	CmnActFDownBound = 0x29,  // landing on their face
	CmnActFDownLoop = 0x2a,  // lying on their face
	CmnActFDown2Stand = 0x2b,  // standing up from their face
	CmnActVDownUpper = 0x2c,  // launched into air, going up
	CmnActVDownUpperEnd = 0x2d,  // seems to be unused
	CmnActVDownDown = 0x2e,  // launched into air, is at the top of the trajectory or going down
	CmnActVDownBound = 0x2f,  // landing from air launch
	CmnActVDownLoop = 0x30,  // laying down (on the face) from having been launched into the air
	CmnActBlowoff = 0x31,  // don't know
	CmnActKirimomiUpper = 0x32,  // got hit by 5D
	CmnActWallBound = 0x33,  // bouncing from a wall
	CmnActWallBoundDown = 0x34,  // falling down after bouncing from a wall
	CmnActWallHaritsuki = 0x35,  // wallsplat from 5D6/other moves, and wallslump animation
	CmnActWallHaritsukiLand = 0x36,  // landing on the ground from wallslump, also laying/sitting there
	CmnActWallHaritsukiGetUp = 0x37,  // getting up after wallslump, followed by crouch2stand
	CmnActJitabataLoop = 0x38,  // stagger (as in from Ky 5H CH) animation
	CmnActKizetsu = 0x39,  // dizziness animation
	CmnActHizakuzure = 0x3a,  // crumple from getting hit by max charge blitz. Is followed by CmnActFDownLoop
	CmnActKorogari = 0x3b,  // tumbling animation
	CmnActZSpin = 0x3c,  // happens when Haehyun airthrows you, you spin in the air around the axis perpendicular to the screen
	CmnActUkemi = 0x3d,  // airtech (air recovery)
	CmnActMidGuardPre = 0x3e,  // seems to be unused
	CmnActMidGuardLoop = 0x3f,  // mid proximity blocking or already blocking, in block histop/blockstun
	CmnActMidGuardEnd = 0x40,  // just left blockstun or leaving proximity block animation
	CmnActHighGuardPre = 0x41,  // seems to be unused
	CmnActHighGuardLoop = 0x42,  // overhead proximity blocking or already blocking, in block histop/blockstun
	CmnActHighGuardEnd = 0x43,  // just left blockstun or leaving proximity block animation
	CmnActCrouchGuardPre = 0x44,  // seems to be unused
	CmnActCrouchGuardLoop = 0x45,  // crouch proximity blocking or already blocking, in block histop/blockstun
	CmnActCrouchGuardEnd = 0x46,  // just left crouch blockstun or leaving proximity block animation
	CmnActAirGuardPre = 0x47,  // seems to be unused
	CmnActAirGuardLoop = 0x48,  // air blocking, in block histop/blockstun
	CmnActAirGuardEnd = 0x49,  // just left air blockstun
	CmnActHajikareStand = 0x4a,  // rejected by blitz, also throw break
	CmnActHajikareCrouch = 0x4b,  // rejected by blitz
	CmnActHajikareAir = 0x4c,  // rejected by blitz, also air throw break
	CmnActAirTurn = 0x4d,  // turning in the air
	CmnActLockWait = 0x4e,  // about to get ground/air grabbed, grab already registered, and also being grabbed/participating in grab animation as the victim
	CmnActLockReject = 0x4f,  // don't know
	CmnActAirLockWait = 0x50,  // don't know
	CmnActAirLockReject = 0x51,  // don't know
	CmnActItemUse = 0x52,  // don't know, MOM probably
	CmnActBurst = 0x53,  // burst
	CmnActRomanCancel = 0x54,  // YRC, PRC, RRC
	CmnActEntry = 0x55,  // don't know
	CmnActRoundWin = 0x56,  // round win
	CmnActMatchWin = 0x57,  // match victory animation
	CmnActLose = 0x58,  // don't know
	CmnActResultWin = 0x59,  // static pose that happens on victory quote screen
	CmnActResultLose = 0x5a,  // static pose that happens on victory quote screen
	CmnActEntryWait = 0x5b,  // you're invisible
	CmnActExDamage = 0x5c,  // don't know
	CmnActExDamageLand = 0x5d,  // don't know
	NotACmnAct = 0xffffffff
};

enum HitResult {
	HIT_RESULT_NONE,
	HIT_RESULT_NORMAL,
	HIT_RESULT_BLOCKED,
	HIT_RESULT_IGNORED,
	HIT_RESULT_ARMORED,  // or rejected
	HIT_RESULT_5
};

class Entity
{
public:
	inline Entity() {}
	inline Entity(const char* ent) { this->ent = const_cast<char*>(ent); }
	char* ent = nullptr;
	
	// Active means attack frames are coming
	inline bool isActive() const {
		return (*(unsigned int*)(ent + 0x23C) & 0x100) != 0  // This signals that attack's hitboxes should not be ignored. Can happen before hitboxes come out
			&& (*(unsigned int*)(ent + 0x234) & 0x40000000) == 0;  // This signals that attack's hitboxes should be ignored.
		                                                           // Can be simultaneous with 0x100 flag in 0x23C - recovery takes priority
		                                                           // Some moves don't have this flag during their recovery
	}
	
	// 0 for P1, 1 for P2
	inline char team() const { return *(char*)(ent + 0x40); }
	
	inline bool isPawn() const { return *(bool*)(ent + 0x10); }
	inline CharacterType characterType() const { return (CharacterType)*(char*)(ent + 0x44); }
	
	inline int x() const { return *(int*)(ent + 0x24c); }
	inline int y() const { return *(int*)(ent + 0x250); }
	// This function is for hitbox calculation only. To obtain raw x position, use x()
	int posX() const;
	// This function is for hitbox calculation only. To obtain raw y position, use y()
	int posY() const;
	
	inline bool isFacingLeft() const { return *(int*)(ent + 0x248) == 1; }

	bool isGettingThrown() const;

	int pushboxWidth() const;
	int pushboxTop() const;
	// Everyone always seems to have this value set to 0 no matter what
	inline int pushboxFrontWidthOffset() const { return *(int*)(ent + 0x32C); }
	int pushboxBottom() const;

	void pushboxLeftRight(int* left, int* right) const;
	
	inline unsigned int currentAnimDuration() const { return *(const unsigned int*)(ent + 0x130); }
	inline const char* animationName() const { return (const char*)(ent + 0x2444); }
	inline CmnActIndex cmnActIndex() const { return *(CmnActIndex*)(ent + 0xa01c); }
	inline int hitstop() const { return *(int*)(ent + 0x1ac); }
	inline int hitstun() const { return *(int*)(ent + 0x9808); }
	inline int blockstun() const { return *(int*)(ent + 0x4d54); }
	inline int tension() const { return *(int*)(ent + 0x2d134); }  // meter
	inline int tensionPulse() const { return *(int*)(ent + 0x2d128); }  // affects how fast you gain tension
	inline int negativePenaltyTimer() const { return *(int*)(ent + 0x2d12c); }  // starts after you reach negative penalty
	inline int tensionPulsePenalty() const { return *(int*)(ent + 0x2d140); }  // reduces tension pulse and increases negative penalty
	inline int negativePenalty() const { return *(int*)(ent + 0x2d144); }  // progress towards negative penalty
	inline int cornerPenalty() const { return *(int*)(ent + 0x2d14c); }  // penalty for touching the wall
	inline int risc() const { return *(DWORD*)(ent + 0x24e30); }
	inline int hp() const { return *(int*)(ent + 0x9cc); }
	inline int maxHp() const { return *(int*)(ent + 0x9d0); }
	inline int defenseModifier() const { return *(int*)(ent + 0x9874); }  // dmg = dmg * (defenseModifier + 0x100) / 0x100
	inline int gutsRating() const { return *(int*)(ent + 0x9878); }
	inline int stun() const { return *(int*)(ent + 0x9fc4); }
	inline int stunThreshold() const { return *(int*)(ent + 0x9fc8); }
	inline int weight() const { return *(int*)(ent + 0x9880); }
	inline bool enableWhiffCancels() const { return (*(DWORD*)(ent + 0x4d48) & 0x2) != 0; }
	inline bool enableNormals() const { return (*(DWORD*)(ent + 0x4d3c) & 0x1000) != 0; }
	inline bool enableGatlings() const { return (*(DWORD*)(ent + 0x4d48) & 0x1) != 0; }
	inline DWORD forceDisableFlags() const { return *(DWORD*)(ent + 0x24e3c); }
	inline int lifeTimeCounter() const { return *(int*)(ent + 0x18); }
	inline bool inPain() const { return (*(DWORD*)(ent + 0x23c) & 0x6) != 0; }
	inline int comboCount() const { return *(int*)(ent + 0x9F28); }
	inline bool gettingUp() const { return (*(DWORD*)(ent + 0x4d28) & 0x4000) != 0; }
	inline Entity previousEntity() const { return Entity{*(char**)(ent + 0x208)}; }
	inline int& scaleX() { return *(int*)(ent + 0x264); }
	inline int& scaleY() { return *(int*)(ent + 0x268); }
	inline int& scaleZ() { return *(int*)(ent + 0x26c); }
	inline int& scaleDefault() { return *(int*)(ent + 0x2594); }
	inline int& scaleDefault2() { return *(int*)(ent + 0x2664); }
	inline int& physicsYImpulse() { return *(int*)(ent + 0x300); }

	void getState(EntityState*) const;
	
	bool isIdle() const;
	bool isIdleHaritsukiKeep() const;
	bool isIdleSouten() const;
	bool isIdleSouten8() const;
	bool isIdleDaiRensen() const;
	bool isIdleRifle() const;
	bool isIdleSemuke() const;
	bool isIdleNeoHochihu() const;
	bool isIdleAmi_Hold() const;
	bool isIdleAmi_Move() const;
	
	static void getWakeupTimings(CharacterType charType, WakeupTimings* output);
	void getWakeupTimings(WakeupTimings* output);
	int calculateGuts();
	
	inline char* operator+(int offset) const { return (char*)(ent + offset); }

	inline bool operator==(const Entity& other) const { return ent == other.ent; }
	inline bool operator!=(const Entity& other) const { return ent != other.ent; }
	inline bool operator==(void* other) const { return (void*)ent == (void*)other; }
	inline bool operator!=(void* other) const { return (void*)ent != (void*)other; }

	inline operator bool() const { return ent != nullptr; }
	inline operator void*() const { return (void*)ent; }
	inline operator char*() const { return ent; }

};

class EntityManager {
public:
	bool onDllMain();
	int calculateExtraTensionGainModifier(void* pawn);
	char calculateTensionPulsePenaltySeverity(int tensionPulsePenalty);
	char calculateCornerPenaltySeverity(int cornerPenalty);
	void calculateTensionGainModifier(
		int distance,
		int negativePenaltyTimer,
		int tensionPulse,
		int* distanceModifier,
		int* tensionPenaltyModifier,
		int* tensionPulseModifier);
	void calculateTensionPulsePenaltyGainModifier(
		int distance,
		int tensionPulse,
		int* distanceModifier,
		int* tensionPulseModifier);
	int calculateReceivedComboCountTensionGainModifier(bool inPain, int comboCount);
	int calculateDealtComboCountTensionGainModifier(bool inPain, int comboCount);
private:
	friend class Entity;
	getPos_t getPosX;
	getPos_t getPosY;
	getPushbox_t getPushboxWidth;
	getPushbox_t getPushboxTop;
	getPushbox_t getPushboxBottom;
	getExtraTensionModifier_t getExtraTensionModifier = nullptr;
};

extern EntityManager entityManager;
