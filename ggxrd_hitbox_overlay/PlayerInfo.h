#pragma once
#include "Entity.h"
#include "Moves.h"
#include <string>
#include <vector>
#include "Input.h"
#include "InputRingBuffer.h"
#include <memory>

#define INVUL_TYPES_TABLE \
	INVUL_TYPES_EXEC(STRIKE_INVUL, "strike", strikeInvul) \
	INVUL_TYPES_EXEC(THROW_INVUL, "throw", throwInvul) \
	INVUL_TYPES_EXEC(SUPER_LOW_PROFILE, "super low profile", superLowProfile) \
	INVUL_TYPES_EXEC(LOW_PROFILE, "low profile", lowProfile) \
	INVUL_TYPES_EXEC(SOMEWHAT_LOW_PROFILE, "somewhat low profile", somewhatLowProfile) \
	INVUL_TYPES_EXEC(UPPER_BODY_INVUL, "upper body", upperBodyInvul) \
	INVUL_TYPES_EXEC(ABOVE_WAIST_INVUL, "above waist", aboveWaistInvul) \
	INVUL_TYPES_EXEC(TOE_INVUL, "toe", toeInvul) \
	INVUL_TYPES_EXEC(FOOT_INVUL, "foot", footInvul) \
	INVUL_TYPES_EXEC(LEG_INVUL, "leg", legInvul) \
	INVUL_TYPES_EXEC(AIRBORNE_INVUL, "airborne", airborneInvul) \
	INVUL_TYPES_EXEC(AIRBORNE_BUT_WONT_GO_OVER_LOWS, "airborne, but won't go over lows", airborneButWontGoOverLows) \
	INVUL_TYPES_EXEC(CONSIDERED_AIRBORNE, "considered airborne", consideredAirborne) \
	INVUL_TYPES_EXEC(FRONT_LEG_INVUL, "front leg invul", frontLegInvul) \
	INVUL_TYPES_EXEC(PROJECTILE_ONLY_INVUL, "projectile only", projectileOnlyInvul) \
	INVUL_TYPES_EXEC(SUPER_ARMOR, "super armor", superArmor) \
	INVUL_TYPES_EXEC(SUPER_ARMOR_THROW, "throws", superArmorThrow) \
	INVUL_TYPES_EXEC(SUPER_ARMOR_BURST, "burst", superArmorBurst) \
	INVUL_TYPES_EXEC(SUPER_ARMOR_MID, "mids", superArmorMid) \
	INVUL_TYPES_EXEC(SUPER_ARMOR_OVERHEAD, "overheads", superArmorOverhead) \
	INVUL_TYPES_EXEC(SUPER_ARMOR_LOW, "lows", superArmorLow) \
	INVUL_TYPES_EXEC(SUPER_ARMOR_GUARD_IMPOSSIBLE, "unblockables", superArmorGuardImpossible) \
	INVUL_TYPES_EXEC(SUPER_ARMOR_OBJECT_ATTACCK, "projectiles only", superArmorObjectAttacck) \
	INVUL_TYPES_EXEC(SUPER_ARMOR_HONTAI_ATTACCK, "non-projectiles only", superArmorHontaiAttacck) \
	/* this flag only matters when it is absent, and it is present by default. Level 0 are unflickable projectiles */ \
	INVUL_TYPES_EXEC(SUPER_ARMOR_PROJECTILE_LEVEL_0, "error ERROR", superArmorProjectileLevel0) \
	INVUL_TYPES_EXEC(SUPER_ARMOR_OVERDRIVE, "overdrives", superArmorOverdrive) \
	INVUL_TYPES_EXEC(SUPER_ARMOR_BLITZ_BREAK, "max charge blitz or overdrives", superArmorBlitzBreak) \
	INVUL_TYPES_EXEC(REFLECT, "reflect", reflect)

struct MilliaInfo {
	DWORD canProgramSecretGarden:1;
	DWORD SGInputs:3;
	DWORD SGInputsMax:3;
	DWORD chromingRose:9;
	DWORD chromingRoseMax:9;
	DWORD hasPin:1;
	DWORD hasSDisc:1;
	DWORD hasHDisc:1;
	DWORD hasEmeraldRain:1;
	DWORD hasHitstunLinkedSecretGarden:1;
	DWORD hasRose:1;
};

struct ChippInfo {
	unsigned short invis;
	unsigned short wallTime;
	bool hasShuriken;
	bool hasKunaiWall;
	bool hasRyuuYanagi;
};

struct SolInfo {
	unsigned short currentDI;
	unsigned short maxDI:15;
	unsigned short gunflameDisappearsOnHit:1;
	unsigned short gunflameComesOutLater:1;
	unsigned short gunflameFirstWaveDisappearsOnHit:1;
	unsigned short hasTyrantRavePunch2:1;
};

// used by multiple chars
struct CurrentTotalInfo {
	unsigned short current;
	unsigned short max;
};

struct InoInfo {
	unsigned short airdashTimer:13;
	unsigned short noteLevel:3;
	unsigned char noteTime;
	unsigned char noteTimeMax;
	bool hasChemicalLove:1;
	bool hasNote:1;
	bool has5DYRC:1;
};

struct BedmanInfo {
	unsigned short sealA:14;
	unsigned short hasTaskA:1;
	unsigned short hasTaskAApostrophe:1;
	unsigned short sealAMax:14;
	unsigned short hasDejavuAGhost:1;
	unsigned short hasDejavuABoomerang:1;
	unsigned short sealB;
	unsigned short sealBMax;
	unsigned short sealC;
	unsigned short sealCMax;
	unsigned short sealD;
	unsigned short sealDMax;
};

struct RamlethalInfo {
	unsigned char sSwordTime;
	unsigned char sSwordTimeMax;
	unsigned char hSwordTime;
	unsigned char hSwordTimeMax;
	const char* sSwordSubAnim;
	const char* hSwordSubAnim;
};

struct ElpheltInfo {
	unsigned short grenadeTimer;
	unsigned char grenadeDisabledTimer;
	unsigned char grenadeDisabledTimerMax;
};

struct JohnnyInfo {
	unsigned short mistTimer:10;
	unsigned short mistTimerMax:10;
	unsigned short mistKuttsukuTimer:10;
	unsigned short mistKuttsukuTimerMax:10;
};

struct RavenInfo {
	unsigned short slowTime;
	unsigned short slowTimeMax;
};

struct DizzyInfo {
	bool shieldFishSuperArmor;
};

struct KyInfo {
	bool stunEdgeWillDisappearOnHit;
	bool hasChargedStunEdge;
	bool hasSPChargedStunEdge;
	bool hasjD;
};

struct MayInfo {
	bool hasDolphin;
	bool hasBeachBall;
};

struct ZatoInfo {
	unsigned short currentEddieGauge;
	unsigned short maxEddieGauge;
	bool hasGreatWhite;
	bool hasInviteHell;
	bool hasEddie;
};

struct PotemkinInfo {
	bool hasBomb;  // Trishula
};

struct FaustInfo {
	bool hasFlower;  // the ground one
};

struct AxlInfo {
	bool hasSpindleSpinner;
	bool hasSickleFlash;
	bool hasMelodyChain;
	bool hasSickleStorm;
};

struct VenomInfo {
	bool hasQV;
};

struct SlayerInfo {
	unsigned int currentBloodsuckingUniverseBuff;
	unsigned int maxBloodsuckingUniverseBuff;
	bool hasRetro;
};

struct GatlingOrWhiffCancelInfo {
	const char* name;
	const char* slangName;
	const char* replacementInputs;
	const AddedMoveData* move;
	int iterationIndex;
	int bufferTime;
	int framesBeenAvailableFor = 0;  // starts from 1 on the frame 
	int framesBeenAvailableForNotIncludingHitstopFreeze = 0;  // but includes RC slowdown
	bool nameIncludesInputs:1;
	bool wasAddedDuringHitstopFreeze:1;  // if framesBeenAvailableFor/framesBeenAvailableForNotIncludingHitstopFreeze says 1, should we increment it next time we leave hitstop and freeze?
	bool foundOnThisFrame:1;
	bool countersIncremented:1;
	GatlingOrWhiffCancelInfo();
	inline bool operator==(const GatlingOrWhiffCancelInfo& other) const { return move == other.move && bufferTime == other.bufferTime; }
	inline bool operator!=(const GatlingOrWhiffCancelInfo& other) const { return !(*this == other); }
};

// *cosplaying std::vector*
template<size_t size>
struct FixedArrayOfGatlingOrWhiffCancelInfos {
	GatlingOrWhiffCancelInfo elems[size] { };  // the maximum number of moves in the game
	int count;
	inline bool empty() const { return count == 0; }
	// (reads online how to implement a custom iterator) "holy fucking shit this can be way more complicated than it needs to"
	inline GatlingOrWhiffCancelInfo* begin() { return elems; }
	inline GatlingOrWhiffCancelInfo* end() { return elems + count; }
	inline const GatlingOrWhiffCancelInfo* begin() const { return elems; }
	inline const GatlingOrWhiffCancelInfo* end() const { return elems + count; }
	inline size_t size() const { return (size_t)count; }
	inline GatlingOrWhiffCancelInfo* erase(GatlingOrWhiffCancelInfo* ptr) {
		int index = ptr - elems;
		if (count - index > 1) {
			const size_t elemSize = sizeof (GatlingOrWhiffCancelInfo);
			memmove(ptr, ptr + 1, elemSize * (count - index - 1));
		}
		--count;
		return elems + index;
	}
	inline void clear() { count = 0; }
	inline void emplace_back() { if (count != _countof(elems)) { ++count; } }
	inline void emplace(GatlingOrWhiffCancelInfo* ptr) {
		if (count == _countof(elems)) return;
		int index = ptr - elems;
		if (index == count) {
			++count;
			return;
		}
		const size_t elemSize = sizeof (GatlingOrWhiffCancelInfo);
		memmove(ptr + 1, ptr, elemSize * (count - index));
		++count;
	}
	inline GatlingOrWhiffCancelInfo& front() { return elems[0]; }
	inline const GatlingOrWhiffCancelInfo& front() const { return elems[0]; }
	inline GatlingOrWhiffCancelInfo& back() { return elems[count - 1]; }
	inline const GatlingOrWhiffCancelInfo& back() const { return elems[count - 1]; }
	inline GatlingOrWhiffCancelInfo& operator[](int index) { return elems[index]; }
	inline const GatlingOrWhiffCancelInfo& operator[](int index) const { return elems[index]; }
	inline GatlingOrWhiffCancelInfo* data() { return elems; }
	inline const GatlingOrWhiffCancelInfo* data() const { return elems; }
	inline bool hasCancel(const char* skillName, const GatlingOrWhiffCancelInfo** infoPtr = nullptr) const {
		for (const GatlingOrWhiffCancelInfo& info : *this) {
			if (strcmp(info.move->name, skillName) == 0) {
				if (infoPtr) *infoPtr = &info;
				return true;
			}
		}
		return false;
	}
};

template<size_t size>
struct FrameCancelInfo {
	FixedArrayOfGatlingOrWhiffCancelInfos<size> gatlings;
	FixedArrayOfGatlingOrWhiffCancelInfos<size> whiffCancels;
	const char* whiffCancelsNote = nullptr;
	FrameCancelInfo() = default;
	inline void clear() {
		gatlings.clear();
		whiffCancels.clear();
		whiffCancelsNote = nullptr;
	}
	inline bool hasCancel(const char* skillName, const GatlingOrWhiffCancelInfo** infoPtr = nullptr) const {
		for (const GatlingOrWhiffCancelInfo& info : gatlings) {
			if (strcmp(info.move->name, skillName) == 0) {
				if (infoPtr) *infoPtr = &info;
				return true;
			}
		}
		for (const GatlingOrWhiffCancelInfo& info : whiffCancels) {
			if (strcmp(info.move->name, skillName) == 0) {
				if (infoPtr) *infoPtr = &info;
				return true;
			}
		}
		return false;
	}
	inline void unsetWasFoundOnThisFrame(bool unsetCountersIncremented) {
		for (GatlingOrWhiffCancelInfo& info : gatlings) {
			info.foundOnThisFrame = false;
			if (unsetCountersIncremented) {
				info.countersIncremented = false;
			}
		}
		for (GatlingOrWhiffCancelInfo& info : whiffCancels) {
			info.foundOnThisFrame = false;
			if (unsetCountersIncremented) {
				info.countersIncremented = false;
			}
		}
	}
	inline void deleteThatWhichWasNotFound() {
		size_t counter;
		size_t i;
		if (!gatlings.empty()) {
			for (counter = gatlings.size(); counter != 0; --counter) {
				i = counter - 1;
				if (!gatlings[i].foundOnThisFrame) {
					gatlings.erase(gatlings.begin() + i);
				}
			}
		}
		if (!whiffCancels.empty()) {
			for (counter = whiffCancels.size(); counter != 0; --counter) {
				i = counter - 1;
				if (!whiffCancels[i].foundOnThisFrame) {
					whiffCancels.erase(whiffCancels.begin() + i);
				}
			}
		}
	}
	template<size_t srcSize>
	inline void copyFromAnotherSizedArray(FrameCancelInfo<srcSize>& src) {
		copyCancelsFromAnotherSizePart<srcSize>(gatlings, src.gatlings);
		copyCancelsFromAnotherSizePart<srcSize>(whiffCancels, src.whiffCancels);
		whiffCancelsNote = src.whiffCancelsNote;
	}
	template<size_t srcSize>
	inline void copyCancelsFromAnotherSizePart(FixedArrayOfGatlingOrWhiffCancelInfos<size>& dest,
			FixedArrayOfGatlingOrWhiffCancelInfos<srcSize>& src) {
		size_t elemsToCopy = src.size();
		if (elemsToCopy > size) {
			elemsToCopy = size;
		}
		dest.count = elemsToCopy;
		memcpy(dest.elems, src.elems, sizeof (GatlingOrWhiffCancelInfo) * elemsToCopy);
	}
	template<size_t srcSize>
	inline bool equalTruncated(FrameCancelInfo<srcSize>& src) {
		return equalTruncatedPart<srcSize>(gatlings, src.gatlings)
			&& equalTruncatedPart<srcSize>(whiffCancels, src.whiffCancels)
			&& whiffCancelsNote == src.whiffCancelsNote;
	}
	template<size_t srcSize>
	inline bool equalTruncatedPart(FixedArrayOfGatlingOrWhiffCancelInfos<size>& dest,
			FixedArrayOfGatlingOrWhiffCancelInfos<srcSize>& src) {
		size_t elemsToCompare = src.size();
		if (elemsToCompare > size) {
			elemsToCompare = size;
		}
		return dest.count == elemsToCompare
			&& memcmp(dest.elems, src.elems, sizeof (GatlingOrWhiffCancelInfo) * elemsToCompare) == 0;
	}
};

struct PlayerCancelInfo {
	int start = 0;
	int end = 0; // inclusive
	FrameCancelInfo<30> cancels;
	bool enableJumpCancel:1;
	bool enableSpecialCancel:1;
	bool enableSpecials:1;
	bool airborne:1;
	bool hitAlreadyHappened:1;
	
	bool cancelsEqual(const PlayerCancelInfo& other) const;
	void clear();
	bool isCompletelyEmpty() const;
	inline bool hasCancel(const char* skillName) const { return cancels.hasCancel(skillName); }
};

enum FrameType : char {
	FT_NONE,
	FT_IDLE,
	// this frame type is only intended to prevent projectiles' framebars that existed only during a superfreeze from being erased due to their framebar being completely empty
	FT_IDLE_NO_DISPOSE,
	FT_IDLE_PROJECTILE,
	FT_IDLE_PROJECTILE_HITTABLE,
	FT_IDLE_CANT_BLOCK,
	FT_IDLE_CANT_FD,
	FT_IDLE_AIRBORNE_BUT_CAN_GROUND_BLOCK,
	FT_IDLE_ELPHELT_RIFLE,
	FT_IDLE_ELPHELT_RIFLE_READY,
	FT_EDDIE_IDLE,
	FT_BACCHUS_SIGH,
	FT_HITSTOP,
	FT_ACTIVE,
	FT_ACTIVE_PROJECTILE,
	FT_EDDIE_ACTIVE,
	FT_ACTIVE_HITSTOP,
	FT_ACTIVE_HITSTOP_PROJECTILE,
	FT_EDDIE_ACTIVE_HITSTOP,
	FT_ACTIVE_NEW_HIT,
	FT_ACTIVE_NEW_HIT_PROJECTILE,
	FT_EDDIE_ACTIVE_NEW_HIT,
	FT_STARTUP,
	FT_STARTUP_ANYTIME_NOW,
	FT_STARTUP_ANYTIME_NOW_CAN_ACT,
	FT_STARTUP_STANCE,
	FT_STARTUP_STANCE_CAN_STOP_HOLDING,
	FT_STARTUP_CAN_BLOCK,
	FT_STARTUP_CAN_BLOCK_AND_CANCEL,
	FT_STARTUP_CAN_PROGRAM_SECRET_GARDEN,
	FT_EDDIE_STARTUP,
	FT_ZATO_BREAK_THE_LAW_STAGE2,
	FT_ZATO_BREAK_THE_LAW_STAGE3,
	FT_ZATO_BREAK_THE_LAW_STAGE2_RELEASED,
	FT_ZATO_BREAK_THE_LAW_STAGE3_RELEASED,
	FT_RECOVERY,
	FT_RECOVERY_HAS_GATLINGS,
	FT_RECOVERY_CAN_ACT,
	FT_RECOVERY_CAN_RELOAD,
	FT_EDDIE_RECOVERY,
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
inline bool frameTypeDiscardable(FrameType type) {  // disposable types, framebars wholly consisting of only these frame types can be deleted
	return type == FT_NONE || type == FT_IDLE || type == FT_IDLE_PROJECTILE;
}
inline bool frameTypeAssumesCantAttack(FrameType type) {
	return type == FT_ACTIVE
		|| type == FT_ACTIVE_PROJECTILE
		|| type == FT_STARTUP
		|| type == FT_STARTUP_ANYTIME_NOW
		|| type == FT_STARTUP_ANYTIME_NOW_CAN_ACT
		|| type == FT_STARTUP_STANCE
		|| type == FT_STARTUP_STANCE_CAN_STOP_HOLDING
		|| type == FT_STARTUP_CAN_BLOCK
		|| type == FT_STARTUP_CAN_BLOCK_AND_CANCEL
		|| type == FT_STARTUP_CAN_PROGRAM_SECRET_GARDEN
		|| type == FT_ZATO_BREAK_THE_LAW_STAGE2
		|| type == FT_ZATO_BREAK_THE_LAW_STAGE3
		|| type == FT_ZATO_BREAK_THE_LAW_STAGE2_RELEASED
		|| type == FT_ZATO_BREAK_THE_LAW_STAGE3_RELEASED
		|| type == FT_RECOVERY
		|| type == FT_RECOVERY_HAS_GATLINGS
		|| type == FT_RECOVERY_CAN_ACT
		|| type == FT_NON_ACTIVE
		|| type == FT_NON_ACTIVE_PROJECTILE
		|| type == FT_PROJECTILE
		|| type == FT_LANDING_RECOVERY
		|| type == FT_LANDING_RECOVERY_CAN_CANCEL
		|| type == FT_XSTUN
		|| type == FT_XSTUN_CAN_CANCEL
		|| type == FT_XSTUN_HITSTOP
		|| type == FT_GRAYBEAT_AIR_HITSTUN;
}
inline bool frameTypeActive(FrameType type) {
	return type == FT_ACTIVE || type == FT_ACTIVE_PROJECTILE || type == FT_EDDIE_ACTIVE;
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
	FT_IDLE_NO_DISPOSE,
	FT_IDLE_PROJECTILE,
	FT_IDLE_PROJECTILE_HITTABLE,
	FT_ACTIVE_PROJECTILE,
	FT_ACTIVE_NEW_HIT_PROJECTILE,
	FT_ACTIVE_HITSTOP_PROJECTILE,
	FT_NON_ACTIVE_PROJECTILE,
	FT_BACCHUS_SIGH,
	FT_EDDIE_IDLE,
	FT_EDDIE_STARTUP,
	FT_EDDIE_ACTIVE,
	FT_EDDIE_ACTIVE_NEW_HIT,
	FT_EDDIE_ACTIVE_HITSTOP,
	FT_EDDIE_RECOVERY
};
inline bool isEddieFrame(FrameType type) {
	return type == FT_EDDIE_IDLE
			|| type == FT_EDDIE_STARTUP
			|| type == FT_EDDIE_ACTIVE
			|| type == FT_EDDIE_ACTIVE_NEW_HIT
			|| type == FT_EDDIE_ACTIVE_HITSTOP
			|| type == FT_EDDIE_RECOVERY;
};
inline bool isNewHitType(FrameType type) {
	return type == FT_ACTIVE_NEW_HIT
		|| type == FT_ACTIVE_NEW_HIT_PROJECTILE
		|| type == FT_EDDIE_ACTIVE_NEW_HIT;
}
inline bool frameAssumesCanBlockButCantFDAfterSuperfreeze(FrameType type) {
	return type == FT_IDLE_AIRBORNE_BUT_CAN_GROUND_BLOCK
		|| type == FT_STARTUP_CAN_BLOCK
		|| type == FT_STARTUP_CAN_BLOCK_AND_CANCEL;
}
inline FrameType frameMap(FrameType type) {
	switch (type) {
		case FT_NONE:                               return FT_NONE;
		case FT_IDLE:                               return FT_IDLE;
		case FT_IDLE_NO_DISPOSE:                    return FT_IDLE_PROJECTILE;
		case FT_IDLE_PROJECTILE:                    return FT_IDLE_PROJECTILE;
		case FT_IDLE_PROJECTILE_HITTABLE:           return FT_IDLE_PROJECTILE_HITTABLE;
		case FT_IDLE_CANT_BLOCK:                    return FT_IDLE;
		case FT_IDLE_CANT_FD:                       return FT_IDLE;
		case FT_IDLE_AIRBORNE_BUT_CAN_GROUND_BLOCK: return FT_IDLE;
		case FT_IDLE_ELPHELT_RIFLE:                 return FT_STARTUP;
		case FT_IDLE_ELPHELT_RIFLE_READY:           return FT_IDLE;
		case FT_EDDIE_IDLE:                         return FT_EDDIE_IDLE;
		case FT_BACCHUS_SIGH:                         return FT_BACCHUS_SIGH;
		case FT_HITSTOP:                            return FT_HITSTOP;
		case FT_ACTIVE:                             return FT_ACTIVE;
		case FT_ACTIVE_PROJECTILE:                  return FT_ACTIVE_PROJECTILE;
		case FT_EDDIE_ACTIVE:                       return FT_EDDIE_ACTIVE;
		case FT_ACTIVE_HITSTOP:                     return FT_ACTIVE_HITSTOP;
		case FT_ACTIVE_HITSTOP_PROJECTILE:          return FT_ACTIVE_HITSTOP_PROJECTILE;
		case FT_EDDIE_ACTIVE_HITSTOP:               return FT_EDDIE_ACTIVE_HITSTOP;
		case FT_ACTIVE_NEW_HIT:                     return FT_ACTIVE_NEW_HIT;
		case FT_ACTIVE_NEW_HIT_PROJECTILE:          return FT_ACTIVE_NEW_HIT_PROJECTILE;
		case FT_EDDIE_ACTIVE_NEW_HIT:               return FT_EDDIE_ACTIVE_NEW_HIT;
		case FT_STARTUP:                            return FT_STARTUP;
		case FT_STARTUP_ANYTIME_NOW:                return FT_STARTUP;
		case FT_STARTUP_ANYTIME_NOW_CAN_ACT:        return FT_STARTUP;
		case FT_STARTUP_STANCE:                     return FT_STARTUP;
		case FT_STARTUP_STANCE_CAN_STOP_HOLDING:    return FT_STARTUP;
		case FT_STARTUP_CAN_BLOCK:                  return FT_STARTUP;
		case FT_STARTUP_CAN_BLOCK_AND_CANCEL:       return FT_STARTUP;
		case FT_STARTUP_CAN_PROGRAM_SECRET_GARDEN:  return FT_STARTUP;
		case FT_EDDIE_STARTUP:                      return FT_EDDIE_STARTUP;
		case FT_ZATO_BREAK_THE_LAW_STAGE2:          return FT_STARTUP;
		case FT_ZATO_BREAK_THE_LAW_STAGE3:          return FT_STARTUP;
		case FT_ZATO_BREAK_THE_LAW_STAGE2_RELEASED: return FT_STARTUP;
		case FT_ZATO_BREAK_THE_LAW_STAGE3_RELEASED: return FT_STARTUP;
		case FT_RECOVERY:                           return FT_RECOVERY;
		case FT_RECOVERY_HAS_GATLINGS:              return FT_RECOVERY;
		case FT_RECOVERY_CAN_ACT:                   return FT_RECOVERY;
		case FT_RECOVERY_CAN_RELOAD:                return FT_RECOVERY;
		case FT_EDDIE_RECOVERY:                     return FT_EDDIE_RECOVERY;
		case FT_NON_ACTIVE:                         return FT_NON_ACTIVE;
		case FT_NON_ACTIVE_PROJECTILE:              return FT_NON_ACTIVE_PROJECTILE;
		case FT_PROJECTILE:                         return FT_PROJECTILE;
		case FT_LANDING_RECOVERY:                   return FT_LANDING_RECOVERY;
		case FT_LANDING_RECOVERY_CAN_CANCEL:        return FT_LANDING_RECOVERY;
		case FT_XSTUN:                              return FT_XSTUN;
		case FT_XSTUN_CAN_CANCEL:                   return FT_XSTUN;
		case FT_XSTUN_HITSTOP:                      return FT_XSTUN_HITSTOP;
		case FT_GRAYBEAT_AIR_HITSTUN:               return FT_GRAYBEAT_AIR_HITSTUN;
		default:                                    return FT_NONE;
	}
}
inline FrameType frameMapNoIdle(FrameType type) {
	switch (type) {
		case FT_NONE:                               return FT_NONE;
		case FT_IDLE:                               return FT_IDLE;
		case FT_IDLE_NO_DISPOSE:                    return FT_IDLE_PROJECTILE;
		case FT_IDLE_PROJECTILE:                    return FT_IDLE_PROJECTILE;
		case FT_IDLE_PROJECTILE_HITTABLE:           return FT_IDLE_PROJECTILE_HITTABLE;
		case FT_IDLE_CANT_BLOCK:                    return FT_IDLE_CANT_BLOCK;
		case FT_IDLE_CANT_FD:                       return FT_IDLE_CANT_FD;
		case FT_IDLE_AIRBORNE_BUT_CAN_GROUND_BLOCK: return FT_IDLE_AIRBORNE_BUT_CAN_GROUND_BLOCK;
		case FT_IDLE_ELPHELT_RIFLE:                 return FT_IDLE_ELPHELT_RIFLE;
		case FT_IDLE_ELPHELT_RIFLE_READY:           return FT_IDLE_ELPHELT_RIFLE_READY;
		case FT_EDDIE_IDLE:                         return FT_EDDIE_IDLE;
		case FT_BACCHUS_SIGH:                       return FT_BACCHUS_SIGH;
		case FT_HITSTOP:                            return FT_HITSTOP;
		case FT_ACTIVE:                             return FT_ACTIVE;
		case FT_ACTIVE_PROJECTILE:                  return FT_ACTIVE_PROJECTILE;
		case FT_EDDIE_ACTIVE:                       return FT_EDDIE_ACTIVE;
		case FT_ACTIVE_HITSTOP:                     return FT_ACTIVE_HITSTOP;
		case FT_ACTIVE_HITSTOP_PROJECTILE:          return FT_ACTIVE_HITSTOP_PROJECTILE;
		case FT_EDDIE_ACTIVE_HITSTOP:               return FT_EDDIE_ACTIVE_HITSTOP;
		case FT_ACTIVE_NEW_HIT:                     return FT_ACTIVE_NEW_HIT;
		case FT_ACTIVE_NEW_HIT_PROJECTILE:          return FT_ACTIVE_NEW_HIT_PROJECTILE;
		case FT_EDDIE_ACTIVE_NEW_HIT:               return FT_EDDIE_ACTIVE_NEW_HIT;
		case FT_STARTUP:                            return FT_STARTUP;
		case FT_STARTUP_ANYTIME_NOW:                return FT_STARTUP;
		case FT_STARTUP_ANYTIME_NOW_CAN_ACT:        return FT_STARTUP;
		case FT_STARTUP_STANCE:                     return FT_STARTUP;
		case FT_STARTUP_STANCE_CAN_STOP_HOLDING:    return FT_STARTUP;
		case FT_STARTUP_CAN_BLOCK:                  return FT_STARTUP;
		case FT_STARTUP_CAN_BLOCK_AND_CANCEL:       return FT_STARTUP;
		case FT_STARTUP_CAN_PROGRAM_SECRET_GARDEN:  return FT_STARTUP;
		case FT_EDDIE_STARTUP:                      return FT_EDDIE_STARTUP;
		case FT_ZATO_BREAK_THE_LAW_STAGE2:          return FT_STARTUP;
		case FT_ZATO_BREAK_THE_LAW_STAGE3:          return FT_STARTUP;
		case FT_ZATO_BREAK_THE_LAW_STAGE2_RELEASED: return FT_STARTUP;
		case FT_ZATO_BREAK_THE_LAW_STAGE3_RELEASED: return FT_STARTUP;
		case FT_RECOVERY:                           return FT_RECOVERY;
		case FT_RECOVERY_HAS_GATLINGS:              return FT_RECOVERY;
		case FT_RECOVERY_CAN_ACT:                   return FT_RECOVERY;
		case FT_RECOVERY_CAN_RELOAD:                return FT_RECOVERY;
		case FT_EDDIE_RECOVERY:                     return FT_EDDIE_RECOVERY;
		case FT_NON_ACTIVE:                         return FT_NON_ACTIVE;
		case FT_NON_ACTIVE_PROJECTILE:              return FT_NON_ACTIVE_PROJECTILE;
		case FT_PROJECTILE:                         return FT_PROJECTILE;
		case FT_LANDING_RECOVERY:                   return FT_LANDING_RECOVERY;
		case FT_LANDING_RECOVERY_CAN_CANCEL:        return FT_LANDING_RECOVERY;
		case FT_XSTUN:                              return FT_XSTUN;
		case FT_XSTUN_CAN_CANCEL:                   return FT_XSTUN;
		case FT_XSTUN_HITSTOP:                      return FT_XSTUN_HITSTOP;
		case FT_GRAYBEAT_AIR_HITSTUN:               return FT_GRAYBEAT_AIR_HITSTUN;
		default:                                    return FT_NONE;
	}
}

struct FrameStopInfo {
	unsigned short value:12;  // hitstun, blockstun or hitstop
	unsigned short isHitstun:1;
	unsigned short isStagger:1;
	unsigned short isWakeup:1;
	unsigned short isRejection:1;
	
	unsigned short valueMax:11;  // hitstunMax, blockstunMax or hitstopMax
	unsigned short valueMaxExtra:4;  // hitstunMaxFloorbounceExtra, blockstunMaxLandExtra
	unsigned short isBlockstun:1;
	
	unsigned short tumble;
	
	unsigned short tumbleMax:14;
	unsigned short tumbleIsWallstick:1;
	unsigned short tumbleIsKnockdown:1;
};

void printFameStop(char* buf, size_t bufSize, const FrameStopInfo* stopInfo, int hitstop, int hitstopMax, bool lastBlockWasIB, bool lastBlockWasFD);

struct FramebarTitle {
	const char* text = nullptr;  // non-slang short title
	const char* slang = nullptr;  // slang short title
	const char* uncombined = nullptr;  // short title for 'display each projectile on a separate framebar'
	const char* slangUncombined = nullptr;  // short slang title for 'display each projectile on a separate framebar'
	const char* full = nullptr;  // tooltip text (full title) for when mouse is hovered over the short title
};

struct FrameBase {
	DWORD aswEngineTick;
};

// This struct is initialized by doing memset to 0. Make sure every child struct is ok to memset to 0.
// This means that types like std::vector require special handling in the clear() method.
struct Frame : public FrameBase {
	const char* animName;
	const char* animSlangName;
	Frame* next;
	FramebarTitle title;  // title is stored in a frame, instead of (whole) framebar, so that titles could change as we horizontally scroll the framebar through its history
	unsigned char hitstop;  // because of danger time can go up to 99
	unsigned char hitstopMax;
	unsigned short rcSlowdown:6;
	unsigned short isFirst:1;
	unsigned short hitConnected:1;  // true only for the frame on which a hit connected
	unsigned short rcSlowdownMax:6;
	unsigned short hitstopConflict:1;
	unsigned short newHit:1;
	FrameType type;
	bool activeDuringSuperfreeze:1;
	bool powerup:1;
	bool marker:1;  // either strike invulnerability marker for Jack-O houses, or super armor marker for Dizzy D-Fish
	bool accountedFor:1;  // used by tooltip drawing
	bool operator==(const Frame& other) const;
	inline bool operator!=(const Frame& other) const { return !(*this == other); }
};

// This struct is initialized by doing memset to 0. Make sure every child struct is ok to memset to 0.
// This means that types like std::vector require special handling in the clear() method.
struct PlayerFrame : public FrameBase {
	const char* animName;
	const char* animSlangName;
	std::shared_ptr<FrameCancelInfo<30>> cancels;
	std::vector<Input> inputs;
	const char* powerupExplanation;
	Input prevInput;
	union {
		MilliaInfo milliaInfo;
		ChippInfo chippInfo;
		SolInfo solInfo;
		KyInfo kyInfo;
		CurrentTotalInfo currentTotalInfo;  // Used by multiple chars
		InoInfo inoInfo;
		BedmanInfo bedmanInfo;
		RamlethalInfo ramlethalInfo;
		ElpheltInfo elpheltInfo;
		JohnnyInfo johnnyInfo;
		RavenInfo ravenInfo;
		DizzyInfo dizzyInfo;
		MayInfo mayInfo;
		ZatoInfo zatoInfo;
		PotemkinInfo potemkinInfo;
		FaustInfo faustInfo;
		AxlInfo axlInfo;
		VenomInfo venomInfo;
		SlayerInfo slayerInfo;
	} u;
	short poisonDuration;
	short poisonMax:14;
	short poisonIsBacchusSigh:1;
	short poisonIsRavenSlow:1;
	short startup;
	short active;
	short recovery;
	short frameAdvantage;
	short landingFrameAdvantage;
	short frameAdvantageNoPreBlockstun;
	short landingFrameAdvantageNoPreBlockstun;
	short total;
	FrameStopInfo stop;
	FrameType type;
	unsigned char hitstop;  // because of danger time can go up to 99
	unsigned char hitstopMax;
	unsigned char rcSlowdown;
	unsigned char rcSlowdownMax;
	unsigned char chargeLeft;
	unsigned char chargeRight;
	unsigned char chargeDown;
	unsigned char chargeLeftLast;
	unsigned char chargeRightLast;
	unsigned char chargeDownLast;
	unsigned char doubleJumps:4;
	unsigned char airDashes:3;
	unsigned char needShowAirOptions:1;
	unsigned char dustGatlingTimer;
	unsigned char dustGatlingTimerMax;
	
	bool isFirst:1;
	bool hitConnected:1;  // true only for the frame on which a hit connected
	bool hitstopConflict:1;
	bool newHit:1;
	bool activeDuringSuperfreeze:1;
	
	bool enableNormals:1;
	bool canBlock:1;
	
	bool strikeInvulInGeneral:1;
	bool throwInvulInGeneral:1;
	bool OTGInGeneral:1;
	bool superArmorActiveInGeneral:1;
	bool superArmorActiveInGeneral_IsFull:1;
	
	#define INVUL_TYPES_EXEC(enumName, stringDesc, fieldName) bool fieldName:1;
	INVUL_TYPES_TABLE
	#undef INVUL_TYPES_EXEC
	
	bool hitAlreadyHappened:1;  // stays true for the remainder of the move
	bool enableSpecialCancel:1;
	bool enableSpecials:1;
	bool enableJumpCancel:1;
	bool airborne:1;
	
	bool crossupProtectionIsOdd:1;
	bool crossupProtectionIsAbove1:1;
	bool crossedUp:1;
	bool inputsOverflow:1;
	bool canYrc:1;
	bool canYrcProjectile:1;
	bool createdDangerousProjectile:1;
	bool IBdOnThisFrame:1;
	bool FDdOnThisFrame:1;
	bool blockedOnThisFrame:1;
	bool lastBlockWasFD:1;
	bool lastBlockWasIB:1;
	bool powerup:1;
	bool airthrowDisabled:1;
	bool running:1;
	bool cantBackdash:1;
	bool suddenlyTeleported:1;
	bool dontShowPowerupGraphic:1;
	bool cantAirdash:1;
	bool counterhit:1;
	
	static void shoveMoreInputs(Input& prevInput, std::vector<Input>& destination, const Input& sourcePrevInput, const std::vector<Input>& source, bool* overflow);
	static void shoveMoreInputsAtTheStart(Input& prevInput, std::vector<Input>& destination, const Input& sourcePrevInput, const std::vector<Input>& source, bool* overflow);
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
	virtual void collectRequests(FramebarBase& source, bool framebarAdvancedIdleHitstop, const FrameBase& sourceFrame) = 0;
	virtual void cloneRequests(FramebarBase& source) = 0;
	virtual void clearRequests() = 0;
	virtual void catchUpToIdle(FramebarBase& source, int destinationStartingPosition, int framesToCatchUpFor) = 0;
	virtual FrameBase& getFrame(int index) = 0;
	virtual const FrameBase& getFrame(int index) const = 0;
	virtual bool lastNFramesCompletelyEmpty(int framebarPosition, int n) const = 0;
	FrameType preFrame = FT_NONE;
	FrameType preFrameMapped = FT_NONE;
	FrameType preFrameMappedNoIdle = FT_NONE;
	DWORD preFrameLength = 0;
	DWORD preFrameMappedLength = 0;
	DWORD preFrameMappedNoIdleLength = 0;
	bool requestFirstFrame = false;
	bool requestNextHit = false;
	bool completelyEmpty = false;
};

// This struct is initialized by doing memset to 0. Make sure every child struct is ok to memset to 0.
// This means that types like std::vector are not allowed.
struct Framebar : public FramebarBase {
	Frame frames[200] { Frame{} };
	inline Frame& operator[](int index) { return frames[index]; }
	inline const Frame& operator[](int index) const { return frames[index]; }
	virtual void copyFrame(FrameBase& destFrame, const FrameBase& srcFrame) const override;
	virtual void copyFrame(FrameBase& destFrame, FrameBase&& srcFrame) const override;
	virtual void clear() override;
	virtual int findTickNoGreaterThan(int startingPos, DWORD tick) const override;
	virtual void soakUpIntoPreFrame(const FrameBase& srcFrame) override;
	virtual void processRequests(FrameBase& destinationFrame) override;
	virtual void processRequests(int destinationPosition) override;
	virtual void collectRequests(FramebarBase& source, bool framebarAdvancedIdleHitstop, const FrameBase& sourceFrame) override;
	virtual void cloneRequests(FramebarBase& source) override;
	virtual void clearRequests() override;
	virtual void catchUpToIdle(FramebarBase& source, int destinationStartingPosition, int framesToCatchUpFor) override;
	virtual FrameBase& getFrame(int index) override;
	virtual const FrameBase& getFrame(int index) const override;
	virtual bool lastNFramesCompletelyEmpty(int framebarPosition, int n) const override;
	bool lastNFramesHaveMarker(int framebarPosition, int n) const;  // defined in UI.cpp
	void modifyFrame(int pos, DWORD aswEngineTick, FrameType newType);
};

// This struct is initialized by doing memset to 0. Make sure every child struct is ok to memset to 0.
// This means that types like std::vector are not allowed.
struct PlayerFramebar : public FramebarBase {
	PlayerFrame frames[200] { PlayerFrame{} };
	inline PlayerFrame& operator[](int index) { return frames[index]; }
	inline const PlayerFrame& operator[](int index) const { return frames[index]; }
	virtual void copyFrame(FrameBase& destFrame, const FrameBase& srcFrame) const override;
	virtual void copyFrame(FrameBase& destFrame, FrameBase&& srcFrame) const override;
	virtual void clear() override;
	virtual int findTickNoGreaterThan(int startingPos, DWORD tick) const override;
	virtual void soakUpIntoPreFrame(const FrameBase& srcFrame) override;
	virtual void processRequests(FrameBase& destinationFrame) override;
	virtual void processRequests(int destinationPosition) override;
	virtual void collectRequests(FramebarBase& source, bool framebarAdvancedIdleHitstop, const FrameBase& sourceFrame) override;
	virtual void cloneRequests(FramebarBase& source) override;
	virtual void clearRequests() override;
	virtual void catchUpToIdle(FramebarBase& source, int destinationStartingPosition, int framesToCatchUpFor) override;
	virtual FrameBase& getFrame(int index) override;
	virtual const FrameBase& getFrame(int index) const override;
	virtual bool lastNFramesCompletelyEmpty(int framebarPosition, int n) const override;
	void clearCancels();
	void clearCancels(int index);
	std::vector<Input> inputs;
	Input prevInput{0x0000};
	bool inputsOverflow = false;
	bool prevInputCopied = false;
};

struct EntityFramebar {
	EntityFramebar() = default;
	EntityFramebar(int playerIndex, int id) : playerIndex(playerIndex), id(id) { }
	int playerIndex = -1;
	int id = -1;
	int moveFramebarId = -1;
	bool foundOnThisFrame = false;
	virtual void copyFrame(FrameBase& destFrame, const FrameBase& srcFrame) const = 0;
	virtual void copyFrame(FrameBase& destFrame, FrameBase&& srcFrame) const = 0;
	virtual void copyActiveDuringSuperfreeze(FrameBase& destFrame, const FrameBase& srcFrame) const = 0;
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
	static int confinePos(int pos, int size);
	inline static int posMinusOne(int pos) { if (pos == 0) return _countof(PlayerFramebar::frames) - 1; else return pos - 1; }
	inline static int posPlusOne(int pos) { if (pos == _countof(PlayerFramebar::frames) - 1) return 0; else return pos + 1; }
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
	bool isEddie = false;
	virtual void copyFrame(FrameBase& destFrame, const FrameBase& srcFrame) const override;
	virtual void copyFrame(FrameBase& destFrame, FrameBase&& srcFrame) const override;
	virtual void copyActiveDuringSuperfreeze(FrameBase& destFrame, const FrameBase& srcFrame) const override;
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
	inline int idForCombinedFramebar() const { if (moveFramebarId == -1) return INT_MAX - 1 + playerIndex; else return moveFramebarId; }
};
	
struct CombinedProjectileFramebar : public EntityFramebar {
	CombinedProjectileFramebar() = default;
	CombinedProjectileFramebar(int playerIndex, int id) : EntityFramebar(playerIndex, id) {}
	Framebar main { };  // the one framebar that is displayed
	bool isEddie = false;
	const ProjectileFramebar* sources[_countof(Framebar::frames)] { nullptr };
	int combinedIds[_countof(Framebar::frames)] { 0 };
	virtual void copyFrame(FrameBase& destFrame, const FrameBase& srcFrame) const override;
	virtual void copyFrame(FrameBase& destFrame, FrameBase&& srcFrame) const override;
	virtual void copyActiveDuringSuperfreeze(FrameBase& destFrame, const FrameBase& srcFrame) const override;
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
	bool canBeCombined(const Framebar& source, int sourceId) const;
	void combineFramebar(int framebarPosition, Framebar& source, const ProjectileFramebar* dad);
	void determineName(int framebarPosition, bool isHitstop);
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
	virtual void copyActiveDuringSuperfreeze(FrameBase& destFrame, const FrameBase& srcFrame) const override;
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

// iterates the frames from framebarPosition back into the past for frameCount frames
#define iterateFramesBegin(framebarPosition, frameCount) \
	int iterateFrames_loop1Start, iterateFrames_loop1End, iterateFrames_loop2Start; \
	if (framebarPosition >= frameCount - 1) { \
		iterateFrames_loop1Start = framebarPosition - (frameCount - 1); \
		iterateFrames_loop1End = framebarPosition + 1; \
		iterateFrames_loop2Start = (int)_countof(Framebar::frames); \
	} else { \
		iterateFrames_loop1Start = 0; \
		iterateFrames_loop1End = framebarPosition + 1; \
		iterateFrames_loop2Start = (int)_countof(Framebar::frames) - ( \
			frameCount - (framebarPosition + 1) \
		); \
	} \
	for (int iterateFrames_loopCount = 1; iterateFrames_loopCount <= 2; ++iterateFrames_loopCount) { \
		int iterateFrames_loopStart, iterateFrames_loopEnd; \
		if (iterateFrames_loopCount == 1) { \
			iterateFrames_loopStart = iterateFrames_loop1Start; \
			iterateFrames_loopEnd = iterateFrames_loop1End; \
		} else { \
			iterateFrames_loopStart = iterateFrames_loop2Start; \
			iterateFrames_loopEnd = (int)_countof(Framebar::frames); \
		} \
		for (int iterateFrames_pos = iterateFrames_loopEnd - 1; iterateFrames_pos >= iterateFrames_loopStart; --iterateFrames_pos) {
			// code here
#define iterateFramesEnd \
		} \
	}

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
	inline ActiveData& operator[](int index) { return data[index]; }
};

struct PrevStartupsInfoElem {
	const char* moveName = nullptr;
	const char* moveSlangName = nullptr;
	bool partOfStance = false;
	inline const char* selectName(bool slang) const { return slang && moveSlangName ? moveSlangName : moveName; }
	short startup = 0;
};

// a name and a duration
struct NameDuration {
	const char* name;
	int duration;
};

struct PrevStartupsInfo {
	PrevStartupsInfoElem startups[10] { 0 };
	char count = 0;
	int initialSkip = 0;
	inline PrevStartupsInfoElem& operator[](int index) {
		return startups[index];
	}
	inline void clear() { count = 0; initialSkip = 0; }
	void add(short n, bool partOfStance, const char* name, const char* slangName);
	void print(char*& buf, size_t& bufSize, std::vector<NameDuration>* elems = nullptr) const;
	void printNames(char*& buf, size_t& bufSize, const char** lastNames, int lastNamesCount,
					bool slang, bool useMultiplicationSign = true,
					bool printFrames = false,
					int* lastNamesDurations = nullptr) const;
	int countOfNonEmptyUniqueNames(const char** lastNames, int lastNamesCount, bool slang) const;
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
	MaxHitInfo maxHit;
	int hitOnFrame = 0;
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
	int prevAnimFrame = 0;
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
	int hitstopElapsed = 0;
	int hitstopMax = 0;
	int hitstopWithSlow = 0;
	int hitstopMaxWithSlow = 0;
	int clashHitstop = 0;
	int startup = 0;  // if active frames have not started yet, is equal to total. Otherwise, means time since the owning player has started their last move until active frames, inclusive
	int hitOnFrame = 0;
	int total = 0;  // time since the owning player started their last move
	int hitNumber = 0;  // updated every frame
	int numberOfHits = 0;
	union {
		int bedmanSealElapsedTime = 0;
		int ramlethalSwordElapsedTime;
	};
	int elapsedTime = 0;
	
	int x = 0;
	int y = 0;
	
	int hitboxTopY = 0;
	int hitboxBottomY = 0;
	MaxHitInfo maxHit;
	
	DWORD creationTime_aswEngineTick = 0;
	ActiveDataArray actives;
	MoveInfo move {};
	PrevStartupsInfo prevStartups { 0 };
	const char* lastName = nullptr;
	const char* lastSlangName = nullptr;
	SpriteFrameInfo sprite;
	int framebarId = -1;
	char creatorName[32] { 0 };
	Entity creator { nullptr };
	FramebarTitle framebarTitle { nullptr };
	char rcSlowedDownCounter = 0;
	char rcSlowedDownMax = 0;
	char animName[32] { 0 };
	char trialName[32] { 0 };
	bool markActive:1;  // cleared at the start of prepareDrawData. True means hitboxes were found on this frame, or on this logic tick this projectile registered a hit.
	bool startedUp:1;  // cleared upon disabling. True means active frames have started.
	bool landedHit:1;  // cleared at the start of each logic tick. Set to true from a hit registering function hook.
	bool gotHitOnThisFrame:1;
	bool disabled:1;  // set to true for all projectiles belonging to a player when the player starts a new move.
	bool inNewSection:1;
	bool isDangerous:1;
	bool superArmorActive:1;
	bool clashedOnThisFrame:1;
	bool rcSlowedDown:1;
	bool moveNonEmpty:1;
	bool hitboxTopBottomValid:1;
	bool isRamlethalSword:1;
	bool strikeInvul:1;
	bool dontReplaceFramebarTitle:1;
	bool titleIsFromAFrameThatHitSomething:1;
	bool alreadyIncludedInComboRecipe:1;
	ProjectileInfo() :
		markActive(false),
		startedUp(false),
		landedHit(false),
		gotHitOnThisFrame(false),
		disabled(false)
	{
	}
	void fill(Entity ent, Entity superflashInstigator, bool isCreated, bool fillName = true);
	void printStartup(char* buf, size_t bufSize);
	void printTotal(char* buf, size_t bufSize);
	void determineMoveNameAndSlangName(const char** name, const char** slangName) const;
	static void determineMoveNameAndSlangName(const MoveInfo* move, Entity ptr, const char** name, const char** slangName);
	static void determineMoveNameAndSlangName(Entity ptr, const char** name, const char** slangName, const char** framebarNameFull = nullptr);
	void fillInMove();
	bool hitConnectedForFramebar() const;
};

struct InvulData {
	bool active = false;
	int start = 0;
	ActiveDataArray frames;
	void clear();
	void addInvulFrame(int prevTotal);
	void removeFirstNFrames(int n);
};

enum BlockType {
	BLOCK_TYPE_NORMAL,
	BLOCK_TYPE_INSTANT,
	BLOCK_TYPE_FAULTLESS
};

struct DmgCalc {
	DWORD aswEngineCounter = 0;
	HitResult hitResult = HIT_RESULT_NONE;
	BlockType blockType = BLOCK_TYPE_NORMAL;
	const char* attackName = nullptr;
	const char* attackSlangName = nullptr;
	const char* nameFull = nullptr;
	bool isProjectile = false;
	GuardType guardType = GUARD_TYPE_ANY;
	bool airUnblockable = false;
	bool guardCrush = false;
	bool isThrow = false;
	
	int attackLevel;
	int attackOriginalAttackLevel;
	int attackLevelForGuard;
	int dealtOriginalDamage;
	int standardDamage;
	bool scaleDamageWithHp;
	int oldHp;
	int maxHp;
	bool isOtg;
	bool ignoreOtg;
	bool adds5Dmg;
	AttackType attackType;
	
	union DmgCalcU {
		struct DmgCalcBlock {
			int defenderRisc;
			int riscPlusBase;
			int attackLevel;
			int riscPlusBaseStandard;
			int guardBalanceDefence;
			bool groundedAndOverheadOrLow;
			bool wasInBlockstun;
			
			int baseDamage;
			int attackKezuri;  // chip damage in units of 1/128. By default 16 for specials and supers, 0 for normals, which stands for 12,5%
			int attackKezuriStandard;
		} block;
		struct DmgCalcArmor {
			int baseDamage;
			int damageScale;
			bool isProjectile;
			int projectileDamageScale;
			int superArmorDamagePercent;
			bool superArmorHeadAttribute;
			
			int defenseModifier;
			int gutsRating;
			int gutsLevel;
			int guts;
			
			int attackKezuri;  // chip damage in units of 1/128. By default 16 for specials and supers, 0 for normals, which stands for 12,5%
			int attackKezuriStandard;
		} armor;
		struct DmgCalcHit {
			int baseDamage;
			bool increaseDmgBy50Percent;
			int extraInverseProration;
			bool isStylish;
			int stylishDamageInverseModifier;
			int handicap;
			
			int damageScale;
			bool isProjectile;
			int projectileDamageScale;
			
			int dustProration1;
			int dustProration2;
			
			bool attackerHellfireState;
			bool attackerHpLessThan10Percent;
			bool attackHasHellfireEnabled;
			
			CounterHitType attackCounterHitType;
			bool trainingSettingIsForceCounterHit;
			bool dangerTime;
			bool wasHitDuringRc;
			bool rcDmgProration;
			int proration;
			int risc;
			bool needReduceRisc;
			bool guardBreakInitialProrationApplied;
			bool isFirstHit;
			int initialProration;
			int forcedProration;
			int riscMinusStarter;
			bool riscMinusOnceUsed;
			int riscMinusOnce;
			int riscMinus;
			
			int comboProration;
			bool noDamageScaling;
			
			int minimumDamagePercent;
			
			int defenseModifier;
			int gutsRating;
			int gutsLevel;
			int guts;
			
			bool kill;
			
			int baseStun;
			int comboCount;
			CounterHitEntityValue counterHit;
			TensionMode tensionMode;
			int oldStun;
			int stunMax;
			
			bool throwLockExecute;
			int originalAttackStun;
			
		} hit;
	} u;
};

struct FrameAdvantageForFramebarResult {
	short frameAdvantage;
	short landingFrameAdvantage;
	short frameAdvantageNoPreBlockstun;
	short landingFrameAdvantageNoPreBlockstun;
};

// instances of this class are being moved between each other using memmove and sorted using c runtime library's qsort function which does it in a similar way
struct ComboRecipeElement {
	char stateName[32] { '\0' };  // for projectiles
	char trialName[32] { '\0' };  // for projectiles
	const char* name = nullptr;
	const char* slangName = nullptr;
	int dashDuration = 0;  // also used for walk forward and walk backward
	DWORD timestamp = 0;  // might mean either earliest hit or the time the move was initiated
	int framebarId = 0;  // for projectiles
	int cancelDelayedBy = 0;
	bool whiffed:1;
	bool counterhit:1;
	bool otg:1;
	bool isMeleeAttack:1;
	bool isProjectile:1;
	bool artificial:1;
	bool isWalkForward:1;
	bool isWalkBackward:1;
	bool doneAfterIdle:1;
	bool isJump:1;
	bool isSuperJumpInstall:1;
	ComboRecipeElement();
};

// This struct is cleared using memset to 0. If you add complex elements, add them into the clear() method as well
struct PlayerInfo {
	Entity pawn{ nullptr };
	int prevHp = 0;
	int hp = 0;
	int maxHp = 0;
	int defenseModifier = 0;  // dmg = dmg * (256 + defenseModifier) / 256
	int gutsRating = 0;
	int gutsPercentage = 0;
	int risc = 0;  // max 12800, min -12800
	
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
	int burstGainModifier = 0;
	int stylishBurstGainModifier = 0;
	
	int tensionGainOnLastHit = 0;
	int burstGainOnLastHit = 0;
	int tensionGainLastCombo = 0;
	int burstGainLastCombo = 0;
	int tensionGainMaxCombo = 0;
	int burstGainMaxCombo = 0;
	int stunCombo = 0;
	
	int x = 0;
	int y = 0;
	int speedX = 0;
	int speedY = 0;
	int speedYBeforeSpeedLost = 0;
	int gravity = 0;
	
	int hurtboxTopY = 0;
	int hurtboxBottomY = 0;
	bool hurtboxTopBottomValid = false;
	int hitboxTopY = 0;
	int hitboxBottomY = 0;
	bool hitboxTopBottomValid = false;
	
	int throwRange = 0;
	int throwMinX = 0;
	int throwMaxX = 0;
	int throwMinY = 0;
	int throwMaxY = 0;
	int pushboxWidth = 0;
	int pushboxHeight = 0;
	
	int pushback = 0;
	int pushbackMax = 0;
	int basePushback = 0;
	int baseFdPushback = 0;
	int fdPushback = 0;
	int fdPushbackMax = 0;
	int comboTimer = 0;
	int lastHitComboTimer = 0;
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
	int hitstunMaxFloorbounceExtra = 0;
	int hitstunMaxWithSlow = 0;
	int tumbleMax = 0;
	int tumbleMaxWithSlow = 0;
	int wallstickMax = 0;
	int wallstickMaxWithSlow = 0;
	int knockdownMax = 0;
	int knockdownMaxWithSlow = 0;
	int lastHitstopBeforeWipe = 0;
	int blockstunMax = 0;
	int blockstunMaxLandExtra = 0;
	int blockstunMaxWithSlow = 0;
	int hitstopMax = 0;
	int hitstopMaxSuperArmor = 0;  // for super armors showing correct hitstop max
	int hitstopMaxWithSlow = 0;
	int blockstun = 0;
	int blockstunElapsed = 0;
	int blockstunWithSlow = 0;
	int hitstun = 0;
	int hitstunElapsed = 0;
	int hitstunWithSlow = 0;
	int tumble = 0;
	int tumbleElapsed = 0;
	int tumbleWithSlow = 0;
	int wallstick = 0;
	int wallstickElapsed = 0;
	int wallstickWithSlow = 0;
	int knockdown = 0;
	int knockdownElapsed = 0;
	int knockdownWithSlow = 0;
	int stagger = 0;
	int staggerElapsed = 0;
	int staggerWithSlow = 0;
	int staggerMax = 0;
	int staggerMaxWithSlow = 0;
	int hitstop = 0;
	int hitstopElapsed = 0;
	int hitstopWithSlow = 0;
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
	int wakeupTimingElapsed = 0;
	int wakeupTimingWithSlow = 0;
	int wakeupTimingMaxWithSlow = 0;
	WakeupTimings wakeupTimings;
	int rejection = 0;
	int rejectionMax = 0;
	int rejectionElapsed = 0;
	int rejectionWithSlow = 0;
	int rejectionMaxWithSlow = 0;
	
	// time passed since a change in idlePlus. If it's false, this measures the time you've been busy for.
	// If it's true, this measures the time you've been idle for
	int timePassed = 0;
	// time passed since a change in idleLanding. If it's false, this measures the time since the start of an air move.
	// If it's false, this measures the time since you've last landed or been idle on the ground
	int timePassedLanding = 0;
	                            
	int hitboxesCount = 0;
	int superfreezeStartup = 0;
	
	int startup = 0;  // startup of the last move done directly by the character
	int hitOnFrame = 0;
	ActiveDataArray actives;  // active frames of the last move done directly by the character
	int recovery = 0;  // recovery of the last move done directly by the character. Includes only frames where you can't attack
	int total = 0;  // total frames of the last move done directly by the character. Includes only frames where you can't attack
	
	int totalCanBlock = 0;  // total frames of the last move done directly by the character. Includes only frames where you can't block
	int totalCanFD = 0;  // total frames of the last move done directly by the character. Includes only frames where you can't FD
	
	int totalFD = 0;  // number of frames for which you were holding FD
	
	PrevStartupsInfo prevStartups { 0 };  // startups of moves that you whiff cancelled from
	const char* lastPerformedMoveName = nullptr;
	const char* lastPerformedMoveSlangName = nullptr;
	
	int startupDisp = 0;  // startup to display in the UI. Either current or of the last move
	ActiveDataArray activesDisp;  // active frames to display in the UI. Either current or of the last move
	int hitOnFrameDisp = 0;
	MaxHitInfo maxHitDisp;
	int recoveryDisp = 0;  // recovery to display in the UI. Either current or of the last move. Includes only frames where you can't attack
	int recoveryDispCanBlock = -1;  // recovery until becoming able to block to display in the UI. Either current or of the last move. Includes only frames where you can't block. -1 means need to determine automatically
	int totalDisp = 0;  // total frames to display in the UI. Either current or of the last move. Includes only frames where you can't attack
	MaxHitInfo maxHit;
	MaxHitInfo maxHitUse;
	int hitNumber = 0;
	
	int startupProj = 0;  // startup of all projectiles. Either current or of the last move
	int hitOnFrameProj = 0;
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
	
	#define INVUL_TYPES_EXEC(enumName, stringDesc, fieldName) InvulData fieldName { 0 };
	INVUL_TYPES_TABLE
	#undef INVUL_TYPES_EXEC
	
	int landingRecovery = 0;  // number of landing recovery frames. Either current or of the last performed move
	int animFrame = 0;
	enum XstunDisplay {
		XSTUN_DISPLAY_NONE,
		XSTUN_DISPLAY_HIT,  // hitstun
		XSTUN_DISPLAY_HIT_WITH_SLOW,  // hitstun
		XSTUN_DISPLAY_BLOCK,  // blockstun
		XSTUN_DISPLAY_BLOCK_WITH_SLOW,  // blockstun
		XSTUN_DISPLAY_STAGGER,  // stagger
		XSTUN_DISPLAY_STAGGER_WITH_SLOW,  // stagger
		XSTUN_DISPLAY_REJECTION,  // rejection
		XSTUN_DISPLAY_REJECTION_WITH_SLOW  // rejection
	} xStunDisplay = XSTUN_DISPLAY_NONE;  // the last thing that was displayed in UI in 'Hitstop+X-stun' field.
	CmnActIndex cmnActIndex = CmnActStand;
	int timeInNewSection = 0;
	DWORD wasPrevFrameForceDisableFlags = 0;
	DWORD wasForceDisableFlags = 0;
	SpriteFrameInfo sprite;
	MoveInfo move {};
	
	int prevBbscrvar = 0;
	int prevBbscrvar5 = 0;
	int playerval0 = 0;
	int playerval1 = 0;
	int maxDI = 0;  // Also used by some other chars
	struct {
		int totalSpriteLengthUntilCreation = 0;
		int totalSpriteLength = 0;
	} gunflameParams;
	char remainingDoubleJumps = 0;
	char remainingAirDashes = 0;
	char wasProhibitFDTimer = 0;
	char wasAirdashHorizontallingTimer = 0;
	char rcSlowdownCounter = 0;
	char rcSlowdownMaxLastSet = 0;
	char rcSlowdownMax = 0;
	char rcSlowedDownCounter = 0;
	char rcSlowedDownMax = 0;
	unsigned short poisonDuration = 0;
	unsigned short poisonDurationMax = 0;
	EddieInfo eddie { 0 };
	
	HitResult lastHitResult = HIT_RESULT_NONE;
	BlockType blockType = BLOCK_TYPE_NORMAL;
	
	
	DWORD moveStartTime_aswEngineTick = 0;
	AddedMoveData* standingFDMove = nullptr;
	AddedMoveData* crouchingFDMove = nullptr;
	PlayerCancelInfo cancels[10] { };
	int cancelsTimer = 0;
	FrameCancelInfo<180> prevFrameCancels;
	FrameCancelInfo<180> wasCancels;
	int cancelsCount = 0;
	std::vector<DmgCalc> dmgCalcs;
	std::vector<Input> inputs;
	std::string lastMoveNameBeforeSuperfreeze;
	std::string lastMoveNameAfterSuperfreeze;
	std::vector<ComboRecipeElement> comboRecipe;
	Input prevInput;
	int dmgCalcsSkippedHits = 0;
	int proration = 0;
	int dustProration1 = 0;
	int dustProration2 = 0;
	int comboProrationNormal = 0;
	int comboProrationOverdrive = 0;
	int prevFrameStunValue = 0;
	int prevFrameMem45 = 0;
	int prevFrameMem46 = 0;
	int prevFrameGroundHitEffect = 0;
	int prevFrameGroundBounceCount = 0;
	int prevFrameTumbleDuration = 0;
	int prevFrameMaxHit = 0;
	int prevFramePlayerval0 = 0;
	int prevFramePlayerval1 = 0;
	int elpheltPrevFrameWasPlayerval1 = 0;
	int elpheltWasPlayerval1 = 0;
	int prevFrameElpheltRifle_AimMem46 = 0;
	int prevFrameResource[4] { 0 };
	RomanCancelAvailability prevFrameRomanCancelAvailability = ROMAN_CANCEL_DISALLOWED;
	int playervalSetterOffset = 0;
	int wasCantBackdashTimer = 0;
	int lastNoteTime = 0;
	int lastNoteTimeMax = 0;
	int lastNoteTimeWithSlow = 0;
	int lastNoteTimeWithSlowMax = 0;
	int noteTime = 0;
	int noteTimeMax = 0;
	int noteTimeWithSlow = 0;
	int noteTimeWithSlowMax = 0;
	int noteLevel = 0;
	int prevPosX = 0;
	int prevPosY = 0;
	int ramlethalBitNStartPos = 0;
	int ramlethalBitFStartPos = 0;
	int wasPlayerval[4] { 0 };
	int wasPlayerval1Idling = 0;
	int wasResource = 0;
	int milliaChromingRoseTimeLeft = 0;
	BedmanInfo bedmanInfo;
	const char* ramlethalSSwordAnim = nullptr;
	const char* ramlethalHSwordAnim = nullptr;
	const char* ramlethalSSwordSubanim = nullptr;
	const char* ramlethalHSwordSubanim = nullptr;
	int ramlethalSSwordTime = 0;
	int ramlethalSSwordTimeMax = 0;
	int ramlethalHSwordTime = 0;
	int ramlethalHSwordTimeMax = 0;
	int sinHawkBakerStartX;
	int elpheltGrenadeElapsed = 0;
	int elpheltGrenadeRemainingWithSlow = 0;
	int elpheltGrenadeMaxWithSlow = 0;
	int elpheltShotgunX = 0;
	int elpheltRifle_AimMem46 = 0;
	int johnnyMistElapsed = 0;
	int johnnyMistTimerWithSlow = 0;
	int johnnyMistTimerMaxWithSlow = 0;
	int johnnyMistKuttsukuElapsed = 0;
	int johnnyMistKuttsukuTimerWithSlow = 0;
	int johnnyMistKuttsukuTimerMaxWithSlow = 0;
	int jackoAegisElapsed = 0;
	int jackoAegisTimeWithSlow = 0;
	int jackoAegisTimeMaxWithSlow = 0;
	int haehyunBallElapsed = 0;
	int haehyunBallTimeWithSlow = 0;
	int haehyunBallTimeMaxWithSlow = 0;
	int haehyunBallRemainingElapsed = 0;
	int haehyunBallRemainingTimeWithSlow = 0;
	int haehyunBallRemainingTimeMaxWithSlow = 0;
	int haehyunSuperBallRemainingElapsed[10] { 0 };
	int haehyunSuperBallRemainingTimeWithSlow[10] { 0 };
	int haehyunSuperBallRemainingTimeMaxWithSlow[10] { 0 };
	int ravenNeedleElapsed = 0;
	int ravenNeedleTime = 0;
	int ravenNeedleTimeMax = 0;
	RavenInfo ravenInfo { 0 };
	int slowTimeElapsed = 0;
	int dizzyFireSpearElapsed = 0;
	int dizzyFireSpearTime = 0;
	int dizzyFireSpearTimeMax = 0;
	int dizzySpearX = 0;
	int dizzySpearY = 0;
	int dizzySpearSpeedX = 0;
	int dizzySpearSpeedY = 0;
	int dizzyScytheElapsed = 0;
	int dizzyScytheTime = 0;
	int dizzyScytheTimeMax = 0;
	int dizzyFishElapsed = 0;
	int dizzyFishTime = 0;
	int dizzyFishTimeMax = 0;
	int dizzyBubbleElapsed = 0;
	int dizzyBubbleTime = 0;
	int dizzyBubbleTimeMax = 0;
	int answerCantCardElapsed = 0;
	int answerCantCardTime = 0;
	int answerCantCardTimeMax = 0;
	char grabAnimation[32] { '\0' };
	DWORD timePassedInNonFrozenFramesSinceStartOfAnim = 0;  // for combo recipe - needed to not show 1-2f of a move that gets kara cancelled into something else
	int delayLastMoveWasCancelledIntoWith = 0;
	int timeSinceWasEnableSpecialCancel = 0;
	int timeSinceWasEnableSpecials = 0;
	int timeSinceWasEnableJumpCancel = 0;
	int timePassedPureIdle = 0;  // time passed since last change in 'idle' property from false to true, in frames no including superfreeze and hitstop. For combo recipe. The reason we take pure 'idle' and not 'idlePlus' is because we want to start measuring idle time after landing separately
	int dustGatlingTimer = 0;
	int dustGatlingTimerMax = 0;
	int ikMoveIndex = -1;
	int counterGuardAirMoveIndex = -1;
	int counterGuardStandMoveIndex = -1;
	int counterGuardCrouchMoveIndex = -1;
	unsigned char chargeLeftLast;
	unsigned char chargeRightLast;
	unsigned char chargeDownLast;
	
	char attackLockAction[32] { '\0' };
	char prevAttackLockAction[32] { '\0' };
	char labelAtTheStartOfTheMove[32] { '\0' };
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
	bool regainedAirOptions:1;
	
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
	bool wasPrevFrameEnableNormals:1;
	bool wasPrevFrameEnableWhiffCancels:1;
	bool wasEnableGatlings:1;
	bool wasEnableWhiffCancels:1;
	bool wasEnableSpecials:1;
	bool wasPrevFrameEnableSpecials:1;
	bool wasEnableSpecialCancel:1;
	bool wasEnableJumpCancel:1;
	bool wasEnableAirtech:1;
	bool wasCanYrc:1;
	bool wasEnableThrow:1;
	bool wasAttackCollidedSoCanCancelNow:1;
	bool wasOtg:1;
	bool wasCantAirdash:1;
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
	bool hasDangerousNonDisabledProjectiles:1;
	bool prevFrameHadDangerousNonDisabledProjectiles:1;
	bool prevFramePreviousEntityLinkObjectDestroyOnStateChangeWasEqualToPlayer:1;
	
	bool counterhit:1;
	
	// Blitz Shield rejection changes super armor enabled and full invul flags at the end of a logic tick
	bool wasSuperArmorEnabled:1;
	bool wasFullInvul:1;
	bool rcSlowedDown:1;
	bool wasHitOnPreviousFrame:1;
	bool wasHitOnThisFrame:1;
	bool grab:1;  // this doesn't work on regular ground and air throws. This flag means the player is in an attackLockAction
	bool lastMoveIsPartOfStance:1;
	bool moveNonEmpty:1;
	bool increaseDmgBy50Percent:1;
	bool leftBlockstunHitstun:1;
	bool cancelsOverflow:1;
	bool receivedNewDmgCalcOnThisFrame:1;
	bool blockedAHitOnThisFrame:1;
	bool rcProration:1;
	bool burstGainOnly20Percent:1;
	bool performingASuper:1;
	bool startedSuperWhenComboing:1;
	bool prevGettingHitBySuper:1;
	bool gettingHitBySuper:1;
	bool prejumped:1;  // this is to fix normal - jump cancel - FD displaying as frames of the normal up to the jump cancel + FD (jump startup is skipped)
	bool performingBDC:1;
	bool staggerMaxFixed:1;
	bool hitstunContaminatedByRCSlowdown:1;
	bool tumbleContaminatedByRCSlowdown:1;
	bool wallstickContaminatedByRCSlowdown:1;
	bool knockdownContaminatedByRCSlowdown:1;
	bool blockstunContaminatedByRCSlowdown:1;
	bool inputsOverflow:1;
	bool createdDangerousProjectile:1;
	bool createdProjectileThatSometimesCanBeDangerous:1;
	bool lastBlockWasIB:1;
	bool lastBlockWasFD:1;
	bool displayTumble:1;
	bool displayWallstick:1;
	bool displayKnockdown:1;
	bool tumbleStartedInHitstop:1;
	bool ramlethalSSwordTimerActive:1;
	bool ramlethalHSwordTimerActive:1;
	bool ramlethalSSwordKowareSonoba:1;
	bool ramlethalHSwordKowareSonoba:1;
	bool johnnyMistFinerBuffed:1;
	bool johnnyMistFinerBuffedOnThisFrame:1;
	bool dizzySpearIsIce:1;
	bool dizzyShieldFishSuperArmor:1;
	bool answerPrevFrameRSFStart:1;
	bool answerCreatedRSFStart:1;
	bool lastPerformedMoveNameIsInComboRecipe:1;
	bool jumpNonCancel:1;  // for combo recipe
	bool superJumpNonCancel:1;  // for combo recipe
	bool jumpCancelled:1;  // for combo recipe
	bool superJumpCancelled:1;  // for combo recipe
	bool doubleJumped:1;  // for combo recipe
	bool startedRunning:1;
	bool isRunning:1;
	bool jumpInstalled:1;
	bool superJumpInstalled:1;
	bool jumpInstalledStage2:1;
	bool superJumpInstalledStage2:1;
	bool startedWalkingForward:1;
	bool isWalkingForward:1;
	bool startedWalkingBackward:1;
	bool isWalkingBackward:1;
	bool isFirstCheckFirePerFrameUponsWrapperOfTheFrame:1;
	bool wasInHitstopFreezeDuringSkillCheck:1;
	bool delayInTheLastMoveIsAfterIdle:1;
	bool lastMoveWasJumpInstalled:1;
	bool lastMoveWasSuperJumpInstalled:1;
	bool throwRangeValid:1;
	bool throwXValid:1;
	bool throwYValid:1;
	bool prevFrameSilentForceKnifeExisted:1;
	bool pickedUpSilentForceKnifeOnThisFrame:1;
	bool lostSpeedYOnThisFrame:1;
	bool wasAirborneOnAnimChange:1;
	bool elpheltFirstWasPlayerval1Measurement:1;
	
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
	int startupType() const;
	void updateLastMoveNameBeforeAfterSuperfreeze(bool disableSlang);
	const char* getLastPerformedMoveName(bool disableSlang) const;
	void printStartup(char* buf, size_t bufSize, std::vector<NameDuration>* elems = nullptr);
	int printStartupForFramebar();
	void printRecovery(char* buf, size_t bufSize);
	int printRecoveryForFramebar();
	void printTotal(char* buf, size_t bufSize, std::vector<NameDuration>* elems = nullptr);
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
	MilliaInfo canProgramSecretGarden() const;
	void appendPlayerCancelInfo(const PlayerCancelInfo& playerCancel);
	void appendPlayerCancelInfo(PlayerCancelInfo&& playerCancel);
	void determineMoveNameAndSlangName(const char** name, const char** slangName);
	static void determineMoveNameAndSlangName(const MoveInfo* move, bool idle, PlayerInfo& pawn, const char** name, const char** slangName);
	static void determineMoveNameAndSlangName(Entity pawn, const char** name, const char** slangName);
	static bool determineMove(Entity pawn, MoveInfo* destination);
	void onAnimReset();
	void removeNonStancePrevStartups();
	void fillInMove();
	static void calculateSlow(int valueElapsed, int valueRemaining, int slowRemaining, int* result, int* resultMax, int *newSlowRemaining);
	void getInputs(const InputRingBuffer* ringBuffer, bool isTheFirstFrameInTheMatch);
	void fillInPlayervalSetter(int playervalNum);
	int getElpheltRifle_AimMem46() const;
	void calcFrameAdvantageForFramebar(FrameAdvantageForFramebarResult* result) const;
	void bringComboElementToEnd(ComboRecipeElement* modifiedElement);
	ComboRecipeElement* findLastNonProjectileComboElement();
	ComboRecipeElement* findLastDash();
	bool lastComboHitEqualsProjectile(Entity ptr, int framebarId) const;
};
