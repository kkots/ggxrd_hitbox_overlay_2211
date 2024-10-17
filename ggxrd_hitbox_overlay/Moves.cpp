#include "pch.h"
#include "Moves.h"
#include "PlayerInfo.h"

Moves moves;

static const CharacterType GENERAL = (CharacterType)-1;

static bool sectionSeparator_enableWhiffCancels(Entity ent);
static bool sectionSeparator_mistFinerAirDash(Entity ent);
static bool sectionSeparator_mistFinerDash(Entity ent);
static bool sectionSeparator_alwaysTrue(Entity ent);
static bool sectionSeparator_blitzShield(Entity ent);
static bool sectionSeparator_dolphin(Entity ent);
static bool sectionSeparator_may6P(Entity ent);
static bool sectionSeparator_may6H(Entity ent);
static bool sectionSeparator_breakTheLaw(Entity ent);

static bool isIdle_enableWhiffCancels(const PlayerInfo& player);
static bool isIdle_Souten8(const PlayerInfo& player);
static bool isIdle_Rifle(const PlayerInfo& player);
static bool isIdle_alwaysTrue(const PlayerInfo& player);
static bool isIdle_alwaysFalse(const PlayerInfo& player);
static bool canBlock_neoHochihu(const PlayerInfo& player);
static bool isIdle_Ami_Move(const PlayerInfo& player);
static bool isIdle_IrukasanRidingAttack(const PlayerInfo& player);

void Moves::onDllMain() {
	defaultMove.isIdle = isIdle_default;
	map.insert({
		{
			{ GENERAL, "ThrowExe" },
			{ true, false, "Ground Throw" }
		},
		{
			{ GENERAL, "AirThrowExe" },
			{ true, false, "Airthrow" }
		},
		{
			{ GENERAL, "CounterGuardStand" },
			{ false, false, "Blitz Shield", sectionSeparator_blitzShield }
		},
		{
			{ GENERAL, "CounterGuardCrouch" },
			{ false, false, "Blitz Shield", sectionSeparator_blitzShield }
		},
		{
			{ GENERAL, "CounterGuardAir" },
			{ false, false, "Air Blitz Shield" }
		},
		{
			{ CHARACTER_TYPE_SOL, "BanditBringer" },
			{ true, false, "Bandit Bringer" }
		},
		{
			{ CHARACTER_TYPE_SOL, "BukkirabouNiNageru" },
			{ false, false, "Wild Throw" }
		},
		{
			{ CHARACTER_TYPE_SOL, "BukkiraExe" },
			{ true, false, "Wild Throw" }
		},
		{
			{ CHARACTER_TYPE_JOHNNY, "MistFinerA" },
			{ false, false, "P Mist Finer" }
		},
		{
			{ CHARACTER_TYPE_JOHNNY, "MistFinerB" },
			{ false, false, "K Mist Finer" }
		},
		{
			{ CHARACTER_TYPE_JOHNNY, "MistFinerC" },
			{ false, false, "S Mist Finer" }
		},
		{
			{ CHARACTER_TYPE_JOHNNY, "MistFinerDehajime" },
			{ true, false, nullptr }
		},
		{
			{ CHARACTER_TYPE_JOHNNY, "MistFinerLoop" },
			{ true, false, nullptr, sectionSeparator_enableWhiffCancels, 5 }
		},
		{
			{ CHARACTER_TYPE_JOHNNY, "MistFinerALv0" },
			{ true, true, nullptr }
		},
		{
			{ CHARACTER_TYPE_JOHNNY, "MistFinerALv1" },
			{ true, true, nullptr }
		},
		{
			{ CHARACTER_TYPE_JOHNNY, "MistFinerALv2" },
			{ true, true, nullptr }
		},
		{
			{ CHARACTER_TYPE_JOHNNY, "MistFinerBLv0" },
			{ true, true, nullptr }
		},
		{
			{ CHARACTER_TYPE_JOHNNY, "MistFinerBLv1" },
			{ true, true, nullptr }
		},
		{
			{ CHARACTER_TYPE_JOHNNY, "MistFinerBLv2" },
			{ true, true, nullptr }
		},
		{
			{ CHARACTER_TYPE_JOHNNY, "MistFinerCLv0" },
			{ true, true, nullptr }
		},
		{
			{ CHARACTER_TYPE_JOHNNY, "MistFinerCLv1" },
			{ true, true, nullptr }
		},
		{
			{ CHARACTER_TYPE_JOHNNY, "MistFinerCLv2" },
			{ true, true, nullptr }
		},
		{
			{ CHARACTER_TYPE_JOHNNY, "MistFinerBDash" },
			{ true, true, nullptr, sectionSeparator_mistFinerDash }
		},
		{
			{ CHARACTER_TYPE_JOHNNY, "MistFinerFDash" },
			{ true, true, nullptr, sectionSeparator_mistFinerDash }
		},
		{
			{ CHARACTER_TYPE_JOHNNY, "MistFinerBWalk" },
			{ true, true, nullptr, sectionSeparator_alwaysTrue, 5, true }
		},
		{
			{ CHARACTER_TYPE_JOHNNY, "MistFinerFWalk" },
			{ true, true, nullptr, sectionSeparator_alwaysTrue, 5, true }
		},
		{
			{ CHARACTER_TYPE_JOHNNY, "MistFinerCancel" },
			{ true, true, nullptr }
		},
		{
			{ CHARACTER_TYPE_JOHNNY, "AirMistFinerA" },
			{ false, false, "Air P Mist Finer" }
		},
		{
			{ CHARACTER_TYPE_JOHNNY, "AirMistFinerB" },
			{ false, false, "Air K Mist Finer" }
		},
		{
			{ CHARACTER_TYPE_JOHNNY, "AirMistFinerC" },
			{ false, false, "Air S Mist Finer" }
		},
		{
			{ CHARACTER_TYPE_JOHNNY, "AirMistFinerDehajime" },
			{ true, false, nullptr }
		},
		{
			{ CHARACTER_TYPE_JOHNNY, "AirMistFinerDehajime" },
			{ true, false, nullptr }
		},
		{
			{ CHARACTER_TYPE_JOHNNY, "AirMistFinerLoop" },
			{ true, false, nullptr, sectionSeparator_enableWhiffCancels, 5 }
		},
		{
			{ CHARACTER_TYPE_JOHNNY, "MistFinerFDashAir" },
			{ true, true, nullptr, sectionSeparator_mistFinerAirDash }
		},
		{
			{ CHARACTER_TYPE_JOHNNY, "MistFinerBDashAir" },
			{ true, true, nullptr, sectionSeparator_mistFinerAirDash }
		},
		{
			{ CHARACTER_TYPE_JOHNNY, "AirMistFinerALv0" },
			{ true, true, nullptr }
		},
		{
			{ CHARACTER_TYPE_JOHNNY, "AirMistFinerALv1" },
			{ true, true, nullptr }
		},
		{
			{ CHARACTER_TYPE_JOHNNY, "AirMistFinerALv2" },
			{ true, true, nullptr }
		},
		{
			{ CHARACTER_TYPE_JOHNNY, "AirMistFinerBLv0" },
			{ true, true, nullptr }
		},
		{
			{ CHARACTER_TYPE_JOHNNY, "AirMistFinerBLv1" },
			{ true, true, nullptr }
		},
		{
			{ CHARACTER_TYPE_JOHNNY, "AirMistFinerBLv2" },
			{ true, true, nullptr }
		},
		{
			{ CHARACTER_TYPE_JOHNNY, "AirMistFinerCLv0" },
			{ true, true, nullptr }
		},
		{
			{ CHARACTER_TYPE_JOHNNY, "AirMistFinerCLv1" },
			{ true, true, nullptr }
		},
		{
			{ CHARACTER_TYPE_JOHNNY, "AirMistFinerCLv2" },
			{ true, true, nullptr }
		},
		{
			{ CHARACTER_TYPE_JOHNNY, "AirMistFinerCancel" },
			{ true, true, nullptr }
		},
		{
			{ CHARACTER_TYPE_MAY, "IrukasanRidingObject", true },
			{ false, false, nullptr, sectionSeparator_dolphin }
		},
		{
			{ CHARACTER_TYPE_MAY, "NmlAtk6A" },
			{ false, false, "6P", sectionSeparator_may6P }
		},
		{
			{ CHARACTER_TYPE_MAY, "NmlAtk6D" },
			{ false, false, "6H", sectionSeparator_may6H }
		},
		// May riding horizontal Dolphin
		{
			{ CHARACTER_TYPE_MAY, "IrukasanRidingAttackYokoA" },
			{ false, false, "Hop on Dolphin", nullptr, 0, false, isIdle_IrukasanRidingAttack }
		},
		{
			{ CHARACTER_TYPE_MAY, "IrukasanRidingAttackYokoB" },
			{ false, false, "Hop on Dolphin", nullptr, 0, false, isIdle_IrukasanRidingAttack }
		},
		// May riding vertical Dolphin
		{
			{ CHARACTER_TYPE_MAY, "IrukasanRidingAttackTateA" },
			{ false, false, "Hop on Dolphin", nullptr, 0, false, isIdle_IrukasanRidingAttack }
		},
		// May riding vertical Dolphin
		{
			{ CHARACTER_TYPE_MAY, "IrukasanRidingAttackTateB" },
			{ false, false, "Hop on Dolphin", nullptr, 0, false, isIdle_IrukasanRidingAttack }
		},
		// Chipp wall cling idle/moving up/down
		{
			{ CHARACTER_TYPE_CHIPP, "HaritsukiKeep" },
			{ false, false, "Wall Climb", nullptr, 0, false, isIdle_enableWhiffCancels }
		},
		// Faust Pogo
		// Pogo entry
		{
			{ CHARACTER_TYPE_FAUST, "Souten" },
			{ false, false, "Spear Point Centripetal Dance", nullptr, 0, false, isIdle_enableWhiffCancels }
		},
		// Pogo P
		{
			{ CHARACTER_TYPE_FAUST, "SoutenA" },
			{ false, false, "Just A Taste!", nullptr, 0, false, isIdle_enableWhiffCancels }
		},
		// Pogo hop
		{
			{ CHARACTER_TYPE_FAUST, "Souten9" },
			{ false, false, "Short Hop", nullptr, 0, false, isIdle_enableWhiffCancels }
		},
		// Pogo 44
		{
			{ CHARACTER_TYPE_FAUST, "Souten44" },
			{ false, false, "Backward Movement", nullptr, 0, false, isIdle_enableWhiffCancels }
		},
		// Pogo 66
		{
			{ CHARACTER_TYPE_FAUST, "Souten66" },
			{ false, false, "Forward Movement", nullptr, 0, false, isIdle_enableWhiffCancels }
		},
		// Pogo K (head flower)
		{
			{ CHARACTER_TYPE_FAUST, "SoutenB" },
			{ false, false, "Growing Flower", nullptr, 0, false, isIdle_enableWhiffCancels }
		},
		// Pogo S (ground flower)
		{
			{ CHARACTER_TYPE_FAUST, "SoutenC" },
			{ false, false, "See? I'm a Flower!", nullptr, 0, false, isIdle_enableWhiffCancels }
		},
		// Pogo Going My Way
		{
			{ CHARACTER_TYPE_FAUST, "SoutenE" },
			{ false, false, "Going My Way", nullptr, 0, false, isIdle_enableWhiffCancels }
		},
		// Faust Pogo Helicopter
		{
			{ CHARACTER_TYPE_FAUST, "Souten8" },
			{ false, false, "Doctor-Copter", nullptr, 0, false, isIdle_Souten8 }
		},
		// Axl Haitaka stance
		{
			{ CHARACTER_TYPE_AXL, "DaiRensen" },
			{ false, false, "Sparrowhawk Stance", nullptr, 0, false, isIdle_enableWhiffCancels }
		},
		// Elphelt Ms. Confille (rifle)
		{
			{ CHARACTER_TYPE_ELPHELT, "Rifle_Start" },
			{ false, false, "Aim Ms. Confille", nullptr, 0, false, isIdle_Rifle }
		},
		{
			{ CHARACTER_TYPE_ELPHELT, "Rifle_Reload" },
			{ false, false, "Reload", nullptr, 0, false, isIdle_Rifle }
		},
		{
			{ CHARACTER_TYPE_ELPHELT, "Rifle_Reload_Perfect" },
			{ false, false, "Perfect Reload", nullptr, 0, false, isIdle_Rifle }
		},
		{
			{ CHARACTER_TYPE_ELPHELT, "Rifle_Reload_Roman" },
			{ false, false, "Reload RC", nullptr, 0, false, isIdle_Rifle }
		},
		// Leo backturn idle and also exiting backturn via 22
		{
			{ CHARACTER_TYPE_LEO, "Semuke" },
			{ false, false, "Brynhildr Stance", nullptr, 0, false, isIdle_enableWhiffCancels }
		},
		// Jam parry
		{
			{ CHARACTER_TYPE_JAM, "NeoHochihu" },
			{ false, false, "Hochifu", nullptr, 0, false, isIdle_alwaysFalse, canBlock_neoHochihu }
		},
		// Answer scroll cling idle
		{
			{ CHARACTER_TYPE_ANSWER, "Ami_Hold" },
			{ false, false, "Savvy Ninpo: Seal of Approval", nullptr, 0, false, isIdle_enableWhiffCancels }
		},
		// Answer s.D
		{
			{ CHARACTER_TYPE_ANSWER, "Ami_Move" },
			{ false, false, "Savvy Ninpo: Safety Net", nullptr, 0, false, isIdle_Ami_Move }
		},
		// Millia Roll Roll
		{
			{ CHARACTER_TYPE_MILLIA, "SaiZenten" },
			{ true, true, "Forward Roll Again" }
		},
		// Millia Roll > S
		{
			{ CHARACTER_TYPE_MILLIA, "ZentenShaker" },
			{ true, true, "Lust Shaker (Follow-up)" }
		},
		// Millia Roll > H
		{
			{ CHARACTER_TYPE_MILLIA, "Digitalis" },
			{ true, true, "Digitalis" }
		},
		// Zato 214K
		{
			{ CHARACTER_TYPE_ZATO, "BreakTheLaw" },
			{ false, false, "Break the Law", sectionSeparator_breakTheLaw }
		}
	});
}

const MoveInfo& Moves::getInfo(CharacterType charType, const char* name, bool isEffect) {
	auto it = map.find({charType, name, isEffect});
	if (it != map.end()) {
		return it->second;
	}
	if (charType != GENERAL) {
		it = map.find({GENERAL, name, isEffect});
		if (it != map.end()) {
			return it->second;
		}
	}
	return defaultMove;
}

bool sectionSeparator_enableWhiffCancels(Entity ent) {
	return ent.enableWhiffCancels();
}

bool sectionSeparator_mistFinerAirDash(Entity ent) {
	return ent.currentAnimDuration() == 9
		|| ent.currentAnimDuration() > 9
		&& ent.hasUponIdling();
}

bool sectionSeparator_mistFinerDash(Entity ent) {
	return ent.currentAnimDuration() >= 10;
}

bool sectionSeparator_alwaysTrue(Entity ent) {
	return true;
}

bool sectionSeparator_blitzShield(Entity ent) {
	return ent.currentAnimDuration() >= 16 && ent.mem45();
}

bool sectionSeparator_dolphin(Entity ent) {
	return strcmp(ent.gotoLabelRequest(), "move") == 0 || ent.mem51() == 0;
}

bool sectionSeparator_may6P(Entity ent) {
	return strcmp(ent.gotoLabelRequest(), "6AHoldAttack") == 0 || ent.mem45() == 0 && ent.currentAnimDuration() > 9;
}

bool sectionSeparator_may6H(Entity ent) {
	return strcmp(ent.gotoLabelRequest(), "6DHoldAttack") == 0 || ent.mem45() == 0 && ent.currentAnimDuration() > 9;
}

bool sectionSeparator_breakTheLaw(Entity ent) {
	return strcmp(ent.gotoLabelRequest(), "up") == 0;
}

bool isIdle_default(const PlayerInfo& player) {
	return player.wasEnableNormals;
}

bool canBlock_default(const PlayerInfo& player) {
	return player.enableBlock;
}

bool isIdle_enableWhiffCancels(const PlayerInfo& player) {
	return player.wasEnableWhiffCancels;
}
// Faust pogo helicopter
bool isIdle_Souten8(const PlayerInfo& player) {
	return !player.wasEnableGatlings;
}
bool isIdle_Rifle(const PlayerInfo& player) {
	return player.wasEnableWhiffCancels
		&& (player.wasForceDisableFlags & 0x2) == 0;  // 0x2 is the force disable flag for Rifle_Fire
}
bool isIdle_alwaysTrue(const PlayerInfo& player) {
	return true;
}
bool isIdle_alwaysFalse(const PlayerInfo& player) {
	return false;
}
bool canBlock_neoHochihu(const PlayerInfo& player) {
	return player.pawn.currentAnimDuration() > 3
		&& player.enableBlock;
}
bool isIdle_Ami_Move(const PlayerInfo& player) {
	return player.wasEnableWhiffCancels
		&& player.pawn.hitstop() == 0;
}
bool isIdle_IrukasanRidingAttack(const PlayerInfo& player) {
	return player.wasEnableWhiffCancels
		&& player.pawn.attackCollidedSoCanCancelNow();
}

MoveInfo::MoveInfo(bool combineWithPreviousMove,
		bool usePlusSignInCombination,
		const char* displayName,
		sectionSeparator_t sectionSeparator,
		int considerIdleInSeparatedSectionAfterThisManyFrames,
		bool preservesNewSection,
		isIdle_t isIdle,
		isIdle_t canBlock)
		:
		combineWithPreviousMove(combineWithPreviousMove),
		usePlusSignInCombination(usePlusSignInCombination),
		displayName(displayName),
		sectionSeparator(sectionSeparator),
		considerIdleInSeparatedSectionAfterThisManyFrames(considerIdleInSeparatedSectionAfterThisManyFrames),
		preservesNewSection(preservesNewSection),
		isIdle(isIdle ? isIdle : isIdle_default) {
	
	if (!isIdle && !canBlock) {
		this->canBlock = canBlock_default;
	} else if (canBlock) {
		this->canBlock = canBlock;
	} else if (isIdle) {
		this->canBlock = isIdle;
	}
}
