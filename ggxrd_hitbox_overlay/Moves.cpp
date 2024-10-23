#include "pch.h"
#include "Moves.h"
#include "PlayerInfo.h"
#include "memoryFunctions.h"

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
static bool sectionSeparator_FDB(Entity ent);
static bool sectionSeparator_soutenBC(Entity ent);
static bool sectionSeparator_QV(Entity ent);
static bool sectionSeparator_sultryPerformance(Entity ent);
static bool sectionSeparator_beakDriver(Entity ent);
static bool sectionSeparator_rifle(Entity ent);
static bool sectionSeparator_leoGuardStance(Entity ent);
static bool sectionSeparator_treasureHunt(Entity ent);
static bool sectionSeparator_falconDive(Entity ent);
static bool sectionSeparator_fourTigersSwordRev(Entity ent);
static bool sectionSeparator_blackHoleAttack(Entity ent);
static bool sectionSeparator_armorDance(Entity ent);
static bool sectionSeparator_kinomiNecro(Entity ent);
static bool sectionSeparator_azami(Entity ent);

static bool isIdle_enableWhiffCancels(const PlayerInfo& player);
static bool isIdle_Souten8(const PlayerInfo& player);
static bool isIdle_Rifle(const PlayerInfo& player);
static bool alwaysTrue(const PlayerInfo& player);
static bool alwaysFalse(const PlayerInfo& player);
static bool canBlock_neoHochihu(const PlayerInfo& player);
static bool isIdle_Ami_Move(const PlayerInfo& player);
static bool isIdle_IrukasanRidingAttack(const PlayerInfo& player);

bool Moves::onDllMain() {
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
			{ true, false, nullptr, sectionSeparator_enableWhiffCancels, 5, true }
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
			{ true, false, nullptr, sectionSeparator_enableWhiffCancels, 5, true }
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
			{ CHARACTER_TYPE_JOHNNY, "TreasureHunt" },
			{ false, false, "Treasure Hunt", sectionSeparator_treasureHunt }
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
			{ false, false, "Hop on Dolphin", nullptr, 0, false, isIdle_IrukasanRidingAttack, alwaysFalse }
		},
		{
			{ CHARACTER_TYPE_MAY, "IrukasanRidingAttackYokoB" },
			{ false, false, "Hop on Dolphin", nullptr, 0, false, isIdle_IrukasanRidingAttack, alwaysFalse }
		},
		// May riding vertical Dolphin
		{
			{ CHARACTER_TYPE_MAY, "IrukasanRidingAttackTateA" },
			{ false, false, "Hop on Dolphin", nullptr, 0, false, isIdle_IrukasanRidingAttack, alwaysFalse }
		},
		// May riding vertical Dolphin
		{
			{ CHARACTER_TYPE_MAY, "IrukasanRidingAttackTateB" },
			{ false, false, "Hop on Dolphin", nullptr, 0, false, isIdle_IrukasanRidingAttack, alwaysFalse }
		},
		// Chipp wall cling idle/moving up/down
		{
			{ CHARACTER_TYPE_CHIPP, "HaritsukiKeep" },
			{ true, false, "Wall Climb", nullptr, 0, false, isIdle_enableWhiffCancels }
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
			{ false, false, "Growing Flower", sectionSeparator_soutenBC, 0, false, isIdle_enableWhiffCancels }
		},
		// Pogo S (ground flower)
		{
			{ CHARACTER_TYPE_FAUST, "SoutenC" },
			{ false, false, "See? I'm a Flower!", sectionSeparator_soutenBC, 0, false, isIdle_enableWhiffCancels }
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
		// Faust 41236K (long ass fishing pole poke that drags you) succeeeding
		{
			{ CHARACTER_TYPE_FAUST, "Hikimodoshi" },
			{ true, false, nullptr }
		},
		// Axl Haitaka stance
		{
			{ CHARACTER_TYPE_AXL, "DaiRensen" },
			{ false, false, "Sparrowhawk Stance", nullptr, 0, false, isIdle_enableWhiffCancels }
		},
		// Axl Rensen + 2 followup
		{
			{ CHARACTER_TYPE_AXL, "Sensageki" },
			{ true, false, "Spinning Chain Strike" }
		},
		// Axl Rensen + 8 followup
		{
			{ CHARACTER_TYPE_AXL, "Kyokusageki" },
			{ true, false, "Melody Chain" }
		},
		// Elphelt Ms. Confille (rifle)
		{
			{ CHARACTER_TYPE_ELPHELT, "Rifle_Start" },
			{ false, false, "Aim Ms. Confille", sectionSeparator_rifle, 0, false, isIdle_Rifle }
		},
		{
			{ CHARACTER_TYPE_ELPHELT, "Rifle_Reload" },
			{ false, false, "Reload", sectionSeparator_rifle, 0, false, isIdle_Rifle }
		},
		{
			{ CHARACTER_TYPE_ELPHELT, "Rifle_Reload_Perfect" },
			{ false, false, "Perfect Reload", sectionSeparator_rifle, 0, false, isIdle_Rifle }
		},
		{
			{ CHARACTER_TYPE_ELPHELT, "Rifle_Reload_Roman" },
			{ false, false, "Rifle RC", sectionSeparator_rifle, 0, false, isIdle_Rifle }
		},
		{
			{ CHARACTER_TYPE_ELPHELT, "Rifle_Roman" },
			{ false, false, "Rifle RC", sectionSeparator_rifle, 0, false, isIdle_Rifle }
		},
		{
			{ CHARACTER_TYPE_ELPHELT, "Shotgun_Reload" },
			{ true, false, "Shotgun Reload" }
		},
		// Leo backturn idle and also exiting backturn via 22
		{
			{ CHARACTER_TYPE_LEO, "Semuke" },
			{ false, false, "Brynhildr Stance", nullptr, 0, false, isIdle_enableWhiffCancels }
		},
		{
			{ CHARACTER_TYPE_LEO, "NmlAtk5CFar_Guard" },
			{ false, false, "f.S~P", sectionSeparator_leoGuardStance }
		},
		{
			{ CHARACTER_TYPE_LEO, "NmlAtk5D_Guard" },
			{ false, false, "5H~P", sectionSeparator_leoGuardStance }
		},
		// Jam parry
		{
			{ CHARACTER_TYPE_JAM, "NeoHochihu" },
			{ false, false, "Hochifu", nullptr, 0, false, canBlock_neoHochihu, canBlock_neoHochihu }
		},
		// Jam 236S~H
		{
			{ CHARACTER_TYPE_JAM, "SenriShinshou" },
			{ true, true, "Senri Shinshou" }
		},
		// Jam 236S~S
		{
			{ CHARACTER_TYPE_JAM, "HyappoShinshou" },
			{ true, true, "Hyappo Shinshou" }
		},
		// Jam 236S~K
		{
			{ CHARACTER_TYPE_JAM, "Ashibarai" },
			{ true, true, "Hamonkyaku" }
		},
		// Jam 236S~P
		{
			{ CHARACTER_TYPE_JAM, "Mawarikomi" },
			{ true, true, "Mawarikomi" }
		},
		// Jam 46P
		{
			{ CHARACTER_TYPE_JAM, "TuikaA" },
			{ true, true, "Zekkei" }
		},
		// Answer scroll cling idle
		{
			{ CHARACTER_TYPE_ANSWER, "Ami_Hold" },
			{ false, false, "Savvy Ninpo: Seal of Approval", nullptr, 0, false, isIdle_enableWhiffCancels }
		},
		// Answer s.D
		{
			{ CHARACTER_TYPE_ANSWER, "Ami_Move" },
			{ false, false, "Savvy Ninpo: Safety Net", nullptr, 0, false, alwaysFalse }
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
		},
		// Potemkin Flick
		{
			{ CHARACTER_TYPE_POTEMKIN, "FDB" },
			{ false, false, "F.D.B.", sectionSeparator_FDB }
		},
		{
			{ CHARACTER_TYPE_POTEMKIN, "HammerFallBrake" },
			{ true, true, "Hammer Fall Break" }
		},
		// Venom QV
		{
			{ CHARACTER_TYPE_VENOM, "DubiousCurveA" },
			{ false, false, "P QV", sectionSeparator_QV }
		},
		{
			{ CHARACTER_TYPE_VENOM, "DubiousCurveB" },
			{ false, false, "K QV", sectionSeparator_QV }
		},
		{
			{ CHARACTER_TYPE_VENOM, "DubiousCurveC" },
			{ false, false, "S QV", sectionSeparator_QV }
		},
		{
			{ CHARACTER_TYPE_VENOM, "DubiousCurveD" },
			{ false, false, "H QV", sectionSeparator_QV }
		},
		// Slayer dandy step follow-ups
		{
			{ CHARACTER_TYPE_SLAYER, "CrossWise" },
			{ true, true, "Crosswise Heel" }
		},
		{
			{ CHARACTER_TYPE_SLAYER, "UnderPressure" },
			{ true, true, "Under Pressure" }
		},
		// Helter Skelter
		{
			{ CHARACTER_TYPE_SLAYER, "RetroFire" },
			{ true, true, "Helter Skelter" }
		},
		{
			{ CHARACTER_TYPE_SLAYER, "PileBunker" },
			{ true, true, "Pilebunker" }
		},
		// I-No Sultry Performance
		{
			{ CHARACTER_TYPE_INO, "KyougenA" },
			{ false, false, "P Sultry Performance", sectionSeparator_sultryPerformance, 0, false, isIdle_enableWhiffCancels, canBlock_default }
		},
		{
			{ CHARACTER_TYPE_INO, "KyougenB" },
			{ false, false, "K Sultry Performance", sectionSeparator_sultryPerformance, 0, false, isIdle_enableWhiffCancels, canBlock_default }
		},
		{
			{ CHARACTER_TYPE_INO, "KyougenC" },
			{ false, false, "S Sultry Performance", sectionSeparator_sultryPerformance, 0, false, isIdle_enableWhiffCancels, canBlock_default }
		},
		{
			{ CHARACTER_TYPE_INO, "KyougenD" },
			{ false, false, "H Sultry Performance", sectionSeparator_sultryPerformance, 0, false, isIdle_enableWhiffCancels, canBlock_default }
		},
		// Chemical Love (Follow-up)
		{
			{ CHARACTER_TYPE_INO, "ChemicalAdd" },
			{ true, false, "Chemical Love (Follow-up)" }
		},
		// Bedman Teleporting from the boomerang head hitting
		{
			{ CHARACTER_TYPE_BEDMAN, "BWarp" },
			{ true, false, "Task B Teleport" }
		},
		{
			{ CHARACTER_TYPE_SIN, "BeakDriver" },
			{ false, false, "Beak Driver", sectionSeparator_beakDriver }
		},
		// Haehyun 21[4K]
		{
			{ CHARACTER_TYPE_HAEHYUN, "LandBlow4Hasei" },
			{ true, false, "Falcon Dive (Reverse Ver.)", sectionSeparator_falconDive }
		},
		// Haehyun 214[K]
		{
			{ CHARACTER_TYPE_HAEHYUN, "LandBlow6Hasei" },
			{ true, false, "Falcon Dive", sectionSeparator_falconDive }
		},
		// Haehyun 623[K]
		{
			{ CHARACTER_TYPE_HAEHYUN, "AntiAir6Hasei" },
			{ true, false, "Four Tigers Sword (Hold)" }
		},
		// Haehyun 623[4K]
		{
			{ CHARACTER_TYPE_HAEHYUN, "AntiAir4Hasei" },
			{ true, false, "Four Tigers Sword (Reverse Ver.)", sectionSeparator_fourTigersSwordRev }
		},
		// Haehyun 236236H
		{
			{ CHARACTER_TYPE_HAEHYUN, "BlackHoleAttack" },
			{ false, false, "Enlightened 3000 Palm Strike", sectionSeparator_blackHoleAttack }
		},
		// Raven stance
		{
			{ CHARACTER_TYPE_RAVEN, "ArmorDance" },
			{ false, false, "Give it to me HERE", sectionSeparator_armorDance }
		},
		// Dizzy 421H
		{
			{ CHARACTER_TYPE_DIZZY, "KinomiNecro" },
			{ false, false, "For roasting chestnuts...", sectionSeparator_kinomiNecro }
		},
		// Baiken Azami
		{
			{ CHARACTER_TYPE_BAIKEN, "BlockingStand" },
			{ false, false, "Azami (Standing)", sectionSeparator_azami }
		},
		{
			{ CHARACTER_TYPE_BAIKEN, "BlockingCrouch" },
			{ false, false, "Azami (Crouching)", sectionSeparator_azami }
		},
		{
			{ CHARACTER_TYPE_BAIKEN, "BlockingAir" },
			{ false, false, "Azami (Aerial)", sectionSeparator_azami }
		}
	});
	
	bool error = false;
	bbscrInstructionSizes = (unsigned short*)sigscanOffset(
		"GuiltyGearXrd.exe:.rdata",
		"24 00 04 00 28 00 04 00 0C 00 04 00 18 00 0C 00",
		&error, "bbscrInstructionSizes");
	
	return !error;
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
	return ent.currentAnimDuration() == 9;
}
bool sectionSeparator_mistFinerDash(Entity ent) {
	return ent.currentAnimDuration() == 10;
}
bool sectionSeparator_alwaysTrue(Entity ent) {
	return true;
}
bool sectionSeparator_blitzShield(Entity ent) {
	return ent.currentAnimDuration() >= 16 && ent.mem45()
		|| strcmp(ent.gotoLabelRequest(), "attack") == 0;
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
bool sectionSeparator_FDB(Entity ent) {
	if (strcmp(ent.gotoLabelRequest(), "Attack") == 0) {
		return true;
	}
	BYTE* markerPos = moves.findSetMarker(ent.bbscrCurrentFunc(), "Attack");
	if (!markerPos) return false;
	return ent.bbscrCurrentInstr() > markerPos;
}
bool sectionSeparator_soutenBC(Entity ent) {
	return strcmp(ent.gotoLabelRequest(), "open") == 0
		|| !ent.hasUponIdling();
}
bool sectionSeparator_QV(Entity ent) {
	return strcmp(ent.gotoLabelRequest(), "End") == 0
		|| !ent.mem45() && ent.currentAnimDuration() > 12;
}
bool sectionSeparator_sultryPerformance(Entity ent) {
	return strcmp(ent.gotoLabelRequest(), "Attack") == 0
		|| !ent.hasUponIdling() && ent.currentAnimDuration() > 9;
}
bool sectionSeparator_beakDriver(Entity ent) {
	return strcmp(ent.gotoLabelRequest(), "Attack") == 0
		|| strcmp(ent.gotoLabelRequest(), "HeavyAttack") == 0;
}
bool sectionSeparator_rifle(Entity ent) {
	return ent.enableWhiffCancels();
}
bool sectionSeparator_leoGuardStance(Entity ent) {
	if (strcmp(ent.gotoLabelRequest(), "End") == 0) {
		return true;
	}
	BYTE* markerPos = moves.findSetMarker(ent.bbscrCurrentFunc(), "End");
	if (!markerPos) {
		return false;
	}
	BYTE* nextInstr = moves.gotoNextInstruction(markerPos);
	if (moves.instructionType(nextInstr) != Moves::instr_sprite) {
		return false;
	}
	return ent.bbscrCurrentInstr() > nextInstr;
}
bool sectionSeparator_treasureHunt(Entity ent) {
	return strcmp(ent.gotoLabelRequest(), "Run") == 0;
}
bool sectionSeparator_falconDive(Entity ent) {
	return strcmp(ent.gotoLabelRequest(), "Attack") == 0
		|| strcmp(ent.gotoLabelRequest(), "Attack2") == 0;
}
bool sectionSeparator_fourTigersSwordRev(Entity ent) {
	return strcmp(ent.gotoLabelRequest(), "Attack") == 0;
}
bool sectionSeparator_blackHoleAttack(Entity ent) {
	return strcmp(ent.gotoLabelRequest(), "Attack") == 0;
}
bool sectionSeparator_armorDance(Entity ent) {
	return strcmp(ent.gotoLabelRequest(), "End") == 0
		|| strcmp(ent.gotoLabelRequest(), "End2") == 0
		|| strcmp(ent.gotoLabelRequest(), "End3") == 0;
}
bool sectionSeparator_kinomiNecro(Entity ent) {
	return strcmp(ent.gotoLabelRequest(), "end") == 0;
}
bool sectionSeparator_azami(Entity ent) {
	return strcmp(ent.gotoLabelRequest(), "End") == 0;
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
bool isIdle_Souten8(const PlayerInfo& player) {
	return !player.wasEnableGatlings;
}
bool isIdle_Rifle(const PlayerInfo& player) {
	return player.wasEnableWhiffCancels
		&& (player.wasForceDisableFlags & 0x2) == 0;  // 0x2 is the force disable flag for Rifle_Fire
}
bool alwaysTrue(const PlayerInfo& player) {
	return true;
}
bool alwaysFalse(const PlayerInfo& player) {
	return false;
}
bool canBlock_neoHochihu(const PlayerInfo& player) {
	return player.pawn.enableWalkBack();
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

BYTE* Moves::gotoNextInstruction(BYTE* in) const {
	return in + bbscrInstructionSizes[*(unsigned int*)in];
}

inline Moves::InstructionType Moves::instructionType(BYTE* in) const {
	return *(InstructionType*)in;
}

BYTE* Moves::findSetMarker(BYTE* in, const char* name) const {
	while (true) {
		InstructionType type = instructionType(in);
		if (type == instr_endState) {
			return nullptr;
		}
		if (type == instr_setMarker
				&& strcmp((const char*)(in + 4), name) == 0) {
			return in;
		}
		in += bbscrInstructionSizes[type];
	}
}
