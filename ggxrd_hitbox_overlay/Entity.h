#pragma once
#include "characterTypes.h"
#include "Hitbox.h"

using getPos_t = int(__thiscall*)(const void*);
using getPushbox_t = int(__thiscall*)(const void*);
using getExtraTensionModifier_t = int(__thiscall*)(const void*, int param1);

enum HitboxType : char {
	HITBOXTYPE_HURTBOX,
	HITBOXTYPE_HITBOX,
	HITBOXTYPE_EX_POINT,  // used in some animations like Millia Bad Moon
	HITBOXTYPE_EX_POINT_EXTENDED,
	HITBOXTYPE_TYPE4,
	HITBOXTYPE_TYPE5,
	HITBOXTYPE_TYPE6,
	HITBOXTYPE_NECK,
	HITBOXTYPE_ABDOMEN,
	HITBOXTYPE_R_LEG,
	HITBOXTYPE_L_LEG,
	HITBOXTYPE_PRIVATE0,
	HITBOXTYPE_PRIVATE1,
	HITBOXTYPE_PRIVATE2,
	HITBOXTYPE_PRIVATE3,
	HITBOXTYPE_TYPE17
};

struct EntityState {
	bool strikeInvuln;
	bool throwInvuln;
	bool superArmorActive;
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
	CmnActStand2Crouch = 0x2,  // switching from standing to crouching animation
	CmnActCrouch = 0x3,  // crouching
	CmnActCrouchTurn = 0x4,  // turning around while crouching
	CmnActCrouch2Stand = 0x5,  // switching from crouching to standing animation
	CmnActJumpPre = 0x6,  // prejump
	CmnActJump = 0x7,  // jumping
	CmnActJumpLanding = 0x8,  // landing from a jump
	CmnActLandingStiff = 0x9,  // landing recovery after doing an air move, like Ky j.D
	CmnActFWalk = 0xa,  // walking forward
	CmnActBWalk = 0xb,  // walking backwards
	CmnActFDash = 0xc,  // running
	CmnActFDashStop = 0xd,  // stopping running
	CmnActBDash = 0xe,  // backdash
	CmnActAirFDash = 0xf,  // airdashing forward
	CmnActAirBDash = 0x10,  // airdashing backward
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
	CmnActVDownLoop = 0x30,  // laying down (on the face) from having been launched into the air. Followed by CmnActFDown2Stand
	CmnActBlowoff = 0x31,  // getting hit by 6P, pilebunker
	CmnActKirimomiUpper = 0x32,  // got hit by 5D
	CmnActWallBound = 0x33,  // bouncing from a wall
	CmnActWallBoundDown = 0x34,  // falling down after bouncing from a wall
	CmnActWallHaritsuki = 0x35,  // wallsplat from 5D6/other moves, and wallslump animation
	CmnActWallHaritsukiLand = 0x36,  // landing on the ground from wallslump, also laying/sitting there
	CmnActWallHaritsukiGetUp = 0x37,  // getting up after wallslump, followed by crouch2stand (not part of haritsuki getup)
	CmnActJitabataLoop = 0x38,  // stagger (as in from Ky 5H CH) animation
	CmnActKizetsu = 0x39,  // dizziness animation. You may get a jitabata first, which then transitions into kizetsu
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
	CmnActExDamage = 0x5c,  // getting hit by Elphelt 236236K
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

enum AttackType {
	ATTACK_TYPE_NONE,
	ATTACK_TYPE_NORMAL,
	ATTACK_TYPE_EX,
	ATTACK_TYPE_OVERDRIVE,
	ATTACK_TYPE_IK
};

enum SuperArmorType {
	SUPER_ARMOR_DODGE,
	SUPER_ARMOR_ARMOR
};

class Entity
{
public:
	inline Entity() {}
	inline Entity(void* ent) { this->ent = (char*)ent; }
	char* ent = nullptr;
	
	inline bool isActive() const { return *(DWORD*)(ent + 0xc) == 1; }
	// Active means attack frames are coming
	inline bool isActiveFrames() const {
		return (*(DWORD*)(ent + 0x23C) & 0x100) != 0  // This signals that attack's hitboxes should not be ignored. Can happen before hitboxes come out
			&& (*(DWORD*)(ent + 0x234) & 0x40000000) == 0;  // This signals that attack's hitboxes should be ignored.
		                                                           // Can be simultaneous with 0x100 flag in 0x23C - recovery takes priority
		                                                           // Some moves don't have this flag during their recovery
	}
	inline bool hasActiveFlag() const { return (*(DWORD*)(ent + 0x23C) & 0x100) != 0; }
	
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
	inline int clashHitstop() const { return *(int*)(ent + 0x1b0); }
	inline bool needSetHitstop() const { return *(DWORD*)(ent + 0x1b8) != 0; }
	inline int startingHitstop() const { return *(int*)(ent + 0x1b4); }
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
	// some of these are really hard to describe without comments that super continent's (Pangaea's) bbscript database doesn't have.
	// This is set when hitting someone and reset on recovery
	inline bool attackCollidedSoCanCancelNow() const { return (*(DWORD*)(ent + 0x23c) & 0x20) != 0; }
	inline bool enableWhiffCancels() const { return (*(DWORD*)(ent + 0x4d48) & 0x2) != 0; }
	inline bool enableNormals() const { return (*(DWORD*)(ent + 0x4d3c) & 0x1000) != 0; }
	inline bool enableGatlings() const { return (*(DWORD*)(ent + 0x4d48) & 0x1) != 0; }
	inline DWORD forceDisableFlags() const { return *(DWORD*)(ent + 0x24e3c); }
	inline bool enableBlock() const { return (*(DWORD*)(ent + 0x4d3c) & 0x10000) != 0; }
	inline bool enableCrouch() const { return (*(DWORD*)(ent + 0x4d3c) & 0x2) != 0; }
	inline bool enableWalkBack() const { return (*(DWORD*)(ent + 0x4d3c) & 0x20) != 0; }
	inline int lifeTimeCounter() const { return *(int*)(ent + 0x18); }
	inline bool inHitstun() const { return (*(DWORD*)(ent + 0x23c) & 0x6) != 0; }
	inline bool inHitstunThisFrame() const { return (*(DWORD*)(ent + 0x23c) & 0x4) != 0; }
	inline bool inHitstunNextFrame() const { return (*(DWORD*)(ent + 0x23c) & 0x2) != 0; }
	inline int comboCount() const { return *(int*)(ent + 0x9F28); }  // this is set on the one getting combo'd
	inline bool gettingUp() const { return (*(DWORD*)(ent + 0x4d28) & 0x4000) != 0; }
	inline Entity previousEntity() const { return Entity{*(char**)(ent + 0x208)}; }  // the last created entity by this entity
	inline Entity stackEntity(int index) const { return Entity{*(char**)(ent + 0x210 + index * 4)}; }  // STACK_0..7 from bbscript
	inline int& scaleX() { return *(int*)(ent + 0x264); }
	inline int& scaleY() { return *(int*)(ent + 0x268); }
	inline int& scaleZ() { return *(int*)(ent + 0x26c); }
	inline int& scaleDefault() { return *(int*)(ent + 0x2594); }
	inline int& scaleDefault2() { return *(int*)(ent + 0x2664); }
	inline int& scaleForParticles() { return *(int*)(ent + 0x2618); }
	inline int speedX() { return *(int*)(ent + 0x2fc); }
	inline int& speedY() { return *(int*)(ent + 0x300); }
	inline int gravity() { return *(int*)(ent + 0x304); }
	inline int comboTimer() { return *(int*)(ent + 0x9f50); }
	inline bool dontUseComboTimerForPushback() { return (*(DWORD*)(ent + 0x710 + 0x10) & 0x20) != 0; }
	inline bool dontUseComboTimerForSpeedY() { return (*(DWORD*)(ent + 0x710 + 0x10) & 0x400) != 0; }
	inline bool noHitstunScaling() { return (*(DWORD*)(ent + 0x710 + 0x10) & 0x10) != 0; }
	inline bool ignoreWeight() { return (*(DWORD*)(ent + 0x710 + 0x10) & 0x1) != 0; }
	inline int fdPushback() { return *(int*)(ent + 0x31c); }  // Ky 5H on May FD ground block, Ky gets -900 on first frame of hitstop. facing right
	inline int pushback() { return *(int*)(ent + 0x2cc); }  // Ky punch May with 5P, May gets pushback 20020 on the frame after hitstop ends. = pendingPushback * 175 / 10
	inline int pendingPushback() { return *(int*)(ent + 0x318); }  // Ky punch May with 5P, May gets pushback 1144 on the frame after hitstop ends.
	inline int pushbackModifier() { return *(int*)(ent + 0x710 + 0x154); }  // Ky 6K has this set to 65 and bbscript calls it hitPushbackX
	inline int airPushbackModifier() { return *(int*)(ent + 0x710 + 0x228); }  // Has not been spotted yet to not be 0
	inline bool ascending() { return (*(DWORD*)(ent + 0x234) & 0x1) != 0; }  // this does not mean prejump. It is set on the initial 7 frames of May jump, 10 Ky jump.
	                                                                         // Those are the frames when your sprite isn't changing, it changes as soon as flag gets unset.
	inline int pushbackModifierDuringHitstun() { return *(int*)(ent + 0x710 + 0x158); }
	inline bool displayModel() { return *(bool*)(ent + 0x2814); }
	inline bool isHidden() { return (*(DWORD*)(ent + 0x11c) & 0x40000000) != 0; }
	inline bool isRecoveryState() { return (*(DWORD*)(ent + 0x234) & 0x40000000) != 0; }
	inline int playerVal(int n) const { return *(int*)(ent + 0x24c50 + 4 * n); }
	inline int currentHitNum() const { return *(int*)(ent + 0x26d8); }
	inline AttackType attackType() { return *(AttackType*)(ent + 0x44c); }
	inline int throwRange() { return *(int*)(ent + 0x494); }
	inline int throwMinX() { return *(int*)(ent + 0x48c); }
	inline int throwMaxX() { return *(int*)(ent + 0x484); }
	inline int throwMaxY() { return *(int*)(ent + 0x488); }
	inline int throwMinY() { return *(int*)(ent + 0x490); }
	// Starts at 2560 on Sol getting dizzied.
	// On next frame decreases by 10 if you didn't press a button.
	// On next frame doesn't decrease because you're in 27f hitstop.
	// Starting on the frame hitstop reaches 0, including that frame, decreases by 10 every frame.
	// When it reaches 0, you're free from dizziness.
	// Is 0 on stagger hits (Ky CH 5H for ex.) and is not used for stagger mashing
	inline int dizzyMashAmountLeft() { return *(int*)(ent + 0x9fcc); }
	inline int dizzyMashAmountMax() { return *(int*)(ent + 0x9fd0); }
	inline bool landed() { return (*(int*)(ent + 0x234) & 0x4) != 0; }  // is true for only one frame - the frame on which you touched the ground
	inline bool hitSomethingOnThisFrame() { return (*(int*)(ent + 0x12c) & 0x20000) != 0; }  // is true for only one frame - the frame on which you hit something
	inline bool inHitstunNextFrame() { return (*(int*)(ent + 0x23c) & 0x2) != 0; }  // is true for only one frame - the frame on which you get hit
	inline bool inBlockstunNextFrame() { return (*(int*)(ent + 0x23c) & 0x1000000) != 0; }  // is true for only one frame - the frame on which you block a hit
	inline bool enableGuardBreak() { return (*(int*)(ent + 0x710 + 0xc) & 0x40000) != 0; }  // i don't know what this is, bbscript calls it enableGuardBreak
	inline Entity currentRunOnObject() { return *(Entity*)(ent + 0x2464); }
	inline bool successfulIB() { return (*(DWORD*)(ent + 0x23c) & 0x800000) != 0; }  // can be set even on FD IB. Remains set even after blockstun is over.
	inline HitResult lastHitResult() { return *(HitResult*)(ent + 0x984); }
	inline int receivedAttackLevel() { return *(int*)(ent + 0x710 + 0x4); }
	inline bool isTouchingLeftWall() { return (*(DWORD*)(ent + 0x118) & 0x400000) != 0; }
	inline bool isTouchingRightWall() { return (*(DWORD*)(ent + 0x118) & 0x800000) != 0; }
	inline bool isSuperFrozen() { return (*(DWORD*)(ent + 0x118) & 0x4000000) != 0; }
	inline bool isRCFrozen() { return (*(DWORD*)(ent + 0x11c) & 0x200000) != 0; }
	inline Entity attacker() { return *(Entity*)(ent + 0x708); }
	inline bool holdingFD() { return (*(DWORD*)(ent + 0x23c) & 0x20000000) != 0; }
	inline int receivedSpeedY() { return *(int*)(ent + 0x944); }
	inline bool clashOnly() { return (*(DWORD*)(ent + 0x44c + 0x14) & 0x1000000) != 0; }  // RTL RideAura has this, Jack O' Aigisfield
	inline bool superArmorEnabled() const { return (*(DWORD*)(ent + 0x9a4) & 0x2) != 0; }
	inline SuperArmorType superArmorType() const { return *(SuperArmorType*)(ent + 0x9a8); }
	inline bool superArmorForReflect() const { return (*(DWORD*)(ent + 0x9a4) & 0x100000) != 0; }
	inline bool superArmorHontaiAttacck() const { return (*(DWORD*)(ent + 0x9a4) & 0x40000) != 0; }  // this is not a typo, it's from bbscript
	inline bool invulnForAegisField() const { return (*(DWORD*)(ent + 0x238) & 0x400) != 0; }
	bool hasUpon(int index) const;
	inline int mem45() const { return *(int*)(ent + 0x14c); }
	inline int mem46() const { return *(int*)(ent + 0x150); }
	inline int mem50() const { return *(int*)(ent + 0x160); }
	inline int mem51() const { return *(int*)(ent + 0x164); }
	inline int mem57() const { return *(int*)(ent + 0x17c); }
	inline int mem58() const { return *(int*)(ent + 0x180); }
	inline int mem59() const { return *(int*)(ent + 0x184); }
	inline int mem60() const { return *(int*)(ent + 0x188); }
	inline int storage(int n) const { return *(int*)(ent + 0x18c + 4 * n); }
	inline int exGaugeValue(int n) const { return *(int*)(ent + 0x24cbc + 36 * n + 0x10); }
	inline int exGaugeMaxValue(int n) const { return *(int*)(ent + 0x24cbc + 36 * n + 0xc); }
	inline const char* gotoLabelRequest() const { return (const char*)(ent + 0x2474 + 0x24); }  // on the next frame, go to marker named this, within the same state
	inline const char* spriteName() const { return (const char*)(ent + 0xa58); }
	inline const char* attackLockAction() const { return (const char*)(ent + 0x44c + 0x54); }
	inline int spriteFrameCounter() const { return *(int*)(ent + 0xa78); }
	inline int spriteFrameCounterMax() const { return *(int*)(ent + 0xa80); }
	// If playing a sprite, points to the next sprite command that would go after it.
	// When on the last sprite, points to the endState instruction.
	inline BYTE* bbscrCurrentInstr() const { return *(BYTE**)(ent + 0xa50); }
	inline BYTE* bbscrCurrentFunc() const { return *(BYTE**)(ent + 0xa54); }  // points to a beginState instruction
	inline int remainingDoubleJumps() const { return *(int*)(ent + 0x4d58); }
	inline Entity playerEntity() const { return *(Entity*)(ent + 0x1d0); }
	inline Entity effectLinkedCollision() const { return *(Entity*)(ent + 0x204); }
	inline bool collisionForceExpand() const { return (*(DWORD*)(ent + 0x44c + 0x14) & 0x4) != 0; }  // having this flag means you ignore the hitboxes hit detection check
	inline int pitch() const { return *(int*)(ent + 0x258); }
	inline int hitboxOffsetX() const { return *(int*)(ent + 0x27c); }
	inline int hitboxOffsetY() const { return *(int*)(ent + 0x280); }
	inline int hitboxCount(HitboxType type) const { return *(int*)(ent + 0xa0 + (int)type * 4); }
	inline const Hitbox* hitboxData(HitboxType type) const { return *(const Hitbox**)(ent + 0x58 + (int)type * 4); }
	inline int untechableTime() const { return *(int*)(ent + 0x694); }
	inline int floorBouncesRemaining() const { return *(int*)(ent + 0x960); }
	inline bool isOtg() const { return (*(DWORD*)(ent + 0x4d24) & 0x800000) != 0; }
	inline int atkAngle() const { return *(int*)(ent + 0x44c + 0x164); }
	inline int strikeInvulnFrames() const { return *(int*)(ent + 0x9a0); }
	inline int throwInvulnFrames() const { return *(int*)(ent + 0x99c); }
	inline bool strikeInvul() const { return (*(DWORD*)(ent + 0x238) & 0x10) != 0; }
	inline bool throwInvul() const { return (*(DWORD*)(ent + 0x238) & 0x20) != 0; }
	inline bool fullInvul() const { return (*(DWORD*)(ent + 0x238) & 0x40) != 0; }
	
	void getState(EntityState*) const;
	
	static void getWakeupTimings(CharacterType charType, WakeupTimings* output);
	void getWakeupTimings(WakeupTimings* output);
	int calculateGuts();
	
	inline char* operator+(int offset) const { return (char*)(ent + offset); }
	inline char* operator+(DWORD offset) const { return (char*)(ent + offset); }

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
	int calculateReceivedComboCountTensionGainModifier(bool inHitstun, int comboCount);
	int calculateDealtComboCountTensionGainModifier(bool inHitstun, int comboCount);
	void calculatePushback(
		int attackLevel,
		int comboTimer,
		bool dontUseComboTimer,
		bool ascending,
		int y,
		int pushbackModifier,
		int airPushbackModifier,
		bool inHitstunOrInHitstunNextFrame,
		int pushbackModifierOnHitstun,
		int* basePushback,
		int* attackPushbackModifier,
		int* hitstunPushbackModifier,
		int* comboTimerPushbackModifier);
	void calculateSpeedYProration(
		int comboCount,
		int weight,
		bool ignoreWeight,
		bool disableComboProtation,
		int* weightModifier,
		int* comboCountModifier);
	void calculateHitstunProration(
		bool noHitstunScaling,
		bool isInAir,
		int comboTimer,
		int* hitstunProration);
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
