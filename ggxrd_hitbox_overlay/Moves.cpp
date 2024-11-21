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
static bool sectionSeparator_organ(Entity ent);

static bool isIdle_enableWhiffCancels(const PlayerInfo& player);
static bool isIdle_Souten8(const PlayerInfo& player);
static bool isIdle_Rifle(const PlayerInfo& player);
static bool alwaysTrue(const PlayerInfo& player);
static bool alwaysFalse(const PlayerInfo& player);
static bool canBlock_neoHochihu(const PlayerInfo& player);
static bool isIdle_Ami_Move(const PlayerInfo& player);

static bool isDangerous_alwaysTrue(Entity ent);
static bool isDangerous_gunflame(Entity ent);
static bool isDangerous_notNull(Entity ent);
static bool isDangerous_notInRecovery(Entity ent);
static bool isDangerous_playerInRCOrHasActiveFlag_AndNotInRecovery(Entity ent);
static bool isDangerous_not_NullWhileActive(Entity ent);
static bool isDangerous_aboveGround(Entity ent);
static bool isDangerous_not_hasHitNumButInactive(Entity ent);
static bool isDangerous_not_hasHitNumButNoHitboxes(Entity ent);
static bool isDangerous_amorphous(Entity ent);
static bool isDangerous_active(Entity ent);
static bool isDangerous_hasNotCreatedAnythingYet(Entity ent);
static bool isDangerous_djavu(Entity ent);
static bool isDangerous_Djavu_D_Ghost(Entity ent);
static bool isDangerous_launchGreatsword(Entity ent);
static bool isDangerous_ramSwordMove(Entity ent);
static bool isDangerous_hasHitboxes(Entity ent);
static bool isDangerous_bubble(Entity ent);
static bool isDangerous_laserFish(Entity ent);
static bool isDangerous_kFish(Entity ent);
static bool isDangerous_card(Entity ent);
static bool isDangerous_kum5D(Entity ent);
static bool isDangerous_rsfMeishi(Entity ent);
static bool isDangerous_displayModel(Entity ent);

void Moves::addMove(const AddedMove& move) {
	map.insert( {
		{ move.charType, move.name, move.isEffect },
		{
			move.combineWithPreviousMove,
			move.usePlusSignInCombination,
			move.displayName,
			move.sectionSeparator,
			move.considerIdleInSeparatedSectionAfterThisManyFrames,
			move.preservesNewSection,
			move.isIdle,
			move.canBlock,
			move.isDangerous,
			move.framebarId,
			move.framebarName,
			move.framebarNameFull
		}
	} );
}

bool Moves::onDllMain() {
	defaultMove.isIdle = isIdle_default;
	
	{
		AddedMove move(GENERAL, "ThrowExe");
		move.combineWithPreviousMove = true;
		move.displayName = "Ground Throw";
		addMove(move);}
	{
		AddedMove move(GENERAL, "CounterGuardStand");
		move.displayName = "Blitz Shield";
		move.sectionSeparator = sectionSeparator_blitzShield;
		addMove(move);}
	{
		AddedMove move(GENERAL, "CounterGuardCrouch");
		move.displayName = "Blitz Shield";
		move.sectionSeparator = sectionSeparator_blitzShield;
		addMove(move);}
	{
		AddedMove move(GENERAL, "CounterGuardAir");
		move.displayName = "Blitz Shield";
		move.sectionSeparator = sectionSeparator_blitzShield;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_SOL, "BanditBringer");
		move.displayName = "Bandit Bringer";
		move.combineWithPreviousMove = true;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_SOL, "BukkiraExe");
		move.displayName = "Wild Throw";
		move.combineWithPreviousMove = true;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_SOL, "BukkirabouNiNageru");
		move.displayName = "Wild Throw";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_SOL, "GunFlameHibashira", true);
		move.isDangerous = isDangerous_gunflame;
		move.framebarId = 1;
		move.framebarName = "Gunflame";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_SOL, "GunFlameHibashira_DI", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 1;
		move.framebarName = "Gunflame";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_SOL, "TyrantRavePunch2", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 2;
		move.framebarName = "Tyrant Rave";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_SOL, "TyrantRavePunch2_DI", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 2;
		move.framebarName = "Tyrant Rave";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_SOL, "KudakeroEF", true);
		move.isDangerous = isDangerous_not_NullWhileActive;
		move.framebarId = 3;
		move.framebarName = "Break";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_SOL, "KudakeroEF_DI", true);
		move.isDangerous = isDangerous_not_NullWhileActive;
		move.framebarId = 3;
		move.framebarName = "Break";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_SOL, "RiotStamp_DI_Bomb", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 4;
		move.framebarName = "Riot Stamp";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_SOL, "GroundViperDash_DI", true);
		move.isDangerous = isDangerous_notNull;
		move.framebarId = 5;
		move.framebarName = "GV Fire Pillars";
		move.framebarNameFull = "Ground Viper Fire Pillars";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_KY, "StunEdgeObj", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 6;
		move.framebarName = "Stun Edge";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_KY, "SPStunEdgeObj", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 6;
		move.framebarName = "DCSE";
		move.framebarNameFull = "Fortified Stun Edge (Durandal Call Stun Edge)";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_KY, "ChargedStunEdgeObj", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 7;
		move.framebarName = "CSE";
		move.framebarNameFull = "Charged Stun Edge";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_KY, "SPChargedStunEdgeObj", true);
		move.isDangerous = isDangerous_not_NullWhileActive;
		move.framebarId = 7;
		move.framebarName = "DCCSE";
		move.framebarNameFull = "Fortified Charged Stun Edge (Drandal Call Charged Stun Edge)";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_KY, "AirDustAttackObj", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 8;
		move.framebarName = "j.D";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_KY, "DustEffectShot", true);
		move.isDangerous = isDangerous_playerInRCOrHasActiveFlag_AndNotInRecovery;
		move.framebarId = 9;
		move.framebarName = "5D";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_KY, "SacredEdgeObj", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 10;
		move.framebarName = "Sacred Edge";
		addMove(move);}
	{
		// the initial move of grounded Mist Finer, is 1f long
		AddedMove move(CHARACTER_TYPE_JOHNNY, "MistFinerA");
		move.displayName = "P Mist Finer";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JOHNNY, "MistFinerB");
		move.displayName = "K Mist Finer";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JOHNNY, "MistFinerC");
		move.displayName = "S Mist Finer";
		addMove(move);}
	{
		// is entered into from MistFinerA/B/C, has variable duration depending on Mist Finer level
		AddedMove move(CHARACTER_TYPE_JOHNNY, "MistFinerDehajime");
		move.combineWithPreviousMove = true;
		addMove(move);}
	{
		// entered into from MistFinerDehajime, enables whiff cancels on f2
		AddedMove move(CHARACTER_TYPE_JOHNNY, "MistFinerLoop");
		move.combineWithPreviousMove = true;
		move.sectionSeparator = sectionSeparator_enableWhiffCancels;
		move.considerIdleInSeparatedSectionAfterThisManyFrames = 5;
		move.preservesNewSection = true;
		addMove(move);}
	{
		// performed when releasing the Mist Finer attack
		AddedMove move(CHARACTER_TYPE_JOHNNY, "MistFinerALv0");
		move.combineWithPreviousMove = true;
		move.usePlusSignInCombination = true;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JOHNNY, "MistFinerALv1");
		move.combineWithPreviousMove = true;
		move.usePlusSignInCombination = true;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JOHNNY, "MistFinerALv2");
		move.combineWithPreviousMove = true;
		move.usePlusSignInCombination = true;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JOHNNY, "MistFinerBLv0");
		move.combineWithPreviousMove = true;
		move.usePlusSignInCombination = true;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JOHNNY, "MistFinerBLv1");
		move.combineWithPreviousMove = true;
		move.usePlusSignInCombination = true;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JOHNNY, "MistFinerBLv2");
		move.combineWithPreviousMove = true;
		move.usePlusSignInCombination = true;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JOHNNY, "MistFinerCLv0");
		move.combineWithPreviousMove = true;
		move.usePlusSignInCombination = true;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JOHNNY, "MistFinerCLv1");
		move.combineWithPreviousMove = true;
		move.usePlusSignInCombination = true;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JOHNNY, "MistFinerCLv2");
		move.combineWithPreviousMove = true;
		move.usePlusSignInCombination = true;
		addMove(move);}
	{
		// backdash during grounded Mist Finer
		AddedMove move(CHARACTER_TYPE_JOHNNY, "MistFinerBDash");
		move.combineWithPreviousMove = true;
		move.usePlusSignInCombination = true;
		move.sectionSeparator = sectionSeparator_mistFinerDash;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JOHNNY, "MistFinerFDash");
		move.combineWithPreviousMove = true;
		move.usePlusSignInCombination = true;
		move.sectionSeparator = sectionSeparator_mistFinerDash;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JOHNNY, "MistFinerBWalk");
		move.combineWithPreviousMove = true;
		move.usePlusSignInCombination = true;
		move.sectionSeparator = sectionSeparator_alwaysTrue;
		move.considerIdleInSeparatedSectionAfterThisManyFrames = 5;
		move.preservesNewSection = true;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JOHNNY, "MistFinerFWalk");
		move.combineWithPreviousMove = true;
		move.usePlusSignInCombination = true;
		move.sectionSeparator = sectionSeparator_alwaysTrue;
		move.considerIdleInSeparatedSectionAfterThisManyFrames = 5;
		move.preservesNewSection = true;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JOHNNY, "MistFinerCancel");
		move.combineWithPreviousMove = true;
		move.usePlusSignInCombination = true;
		addMove(move);}
	{
		// the initial move of air Mist Finer, is 1f long
		AddedMove move(CHARACTER_TYPE_JOHNNY, "AirMistFinerA");
		move.displayName = "Air P Mist Finer";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JOHNNY, "AirMistFinerB");
		move.displayName = "Air K Mist Finer";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JOHNNY, "AirMistFinerC");
		move.displayName = "Air S Mist Finer";
		addMove(move);}
	{
		// is entered into from MistFinerA/B/C, has variable duration depending on Mist Finer level
		AddedMove move(CHARACTER_TYPE_JOHNNY, "AirMistFinerDehajime");
		move.combineWithPreviousMove = true;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JOHNNY, "AirMistFinerLoop");
		move.combineWithPreviousMove = true;
		move.sectionSeparator = sectionSeparator_enableWhiffCancels;
		move.considerIdleInSeparatedSectionAfterThisManyFrames = 5;
		move.preservesNewSection = true;
		addMove(move);}
	{
		// forward dash during air Mist Finer
		AddedMove move(CHARACTER_TYPE_JOHNNY, "MistFinerFDashAir");
		move.combineWithPreviousMove = true;
		move.usePlusSignInCombination = true;
		move.sectionSeparator = sectionSeparator_mistFinerAirDash;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JOHNNY, "MistFinerBDashAir");
		move.combineWithPreviousMove = true;
		move.usePlusSignInCombination = true;
		move.sectionSeparator = sectionSeparator_mistFinerAirDash;
		addMove(move);}
	{
		// performed when releasing the Mist Finer attack
		AddedMove move(CHARACTER_TYPE_JOHNNY, "AirMistFinerALv0");
		move.combineWithPreviousMove = true;
		move.usePlusSignInCombination = true;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JOHNNY, "AirMistFinerALv1");
		move.combineWithPreviousMove = true;
		move.usePlusSignInCombination = true;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JOHNNY, "AirMistFinerALv2");
		move.combineWithPreviousMove = true;
		move.usePlusSignInCombination = true;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JOHNNY, "AirMistFinerBLv0");
		move.combineWithPreviousMove = true;
		move.usePlusSignInCombination = true;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JOHNNY, "AirMistFinerBLv1");
		move.combineWithPreviousMove = true;
		move.usePlusSignInCombination = true;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JOHNNY, "AirMistFinerBLv2");
		move.combineWithPreviousMove = true;
		move.usePlusSignInCombination = true;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JOHNNY, "AirMistFinerCLv0");
		move.combineWithPreviousMove = true;
		move.usePlusSignInCombination = true;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JOHNNY, "AirMistFinerCLv1");
		move.combineWithPreviousMove = true;
		move.usePlusSignInCombination = true;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JOHNNY, "AirMistFinerCLv2");
		move.combineWithPreviousMove = true;
		move.usePlusSignInCombination = true;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JOHNNY, "AirMistFinerCancel");
		move.combineWithPreviousMove = true;
		move.usePlusSignInCombination = true;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JOHNNY, "TreasureHunt");
		move.displayName = "Treasure Hunt";
		move.sectionSeparator = sectionSeparator_treasureHunt;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JOHNNY, "Coin", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 80;
		move.framebarName = "Glitter Is Gold";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JOHNNY, "Sinwaza_Shot2", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 81;
		move.framebarName = "Zwei Hander";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JOHNNY, "Sinwaza_Shot2_Air", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 81;
		move.framebarName = "Zwei Hander";
		addMove(move);}
	{
		// this dolphin is created on 41236P/K/S/H. When ridden it disappears
		AddedMove move(CHARACTER_TYPE_MAY, "IrukasanRidingObject", true);
		move.sectionSeparator = sectionSeparator_dolphin;
		move.isDangerous = isDangerous_aboveGround;
		move.framebarId = 11;
		move.framebarName = "Dolphin";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_MAY, "MayBallA", true);
		move.sectionSeparator = sectionSeparator_dolphin;
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 12;
		move.framebarName = "Beach Ball";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_MAY, "MayBallB", true);
		move.sectionSeparator = sectionSeparator_dolphin;
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 12;
		move.framebarName = "Beach Ball";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_MAY, "NmlAtk6A");
		move.displayName = "6P";
		move.sectionSeparator = sectionSeparator_may6P;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_MAY, "NmlAtk6D");
		move.displayName = "6H";
		move.sectionSeparator = sectionSeparator_may6H;
		addMove(move);}
	{
		// May riding horizontal Dolphin
		AddedMove move(CHARACTER_TYPE_MAY, "IrukasanRidingAttackYokoA");
		move.displayName = "Hop on Dolphin";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_MAY, "IrukasanRidingAttackYokoB");
		move.displayName = "Hop on Dolphin";
		addMove(move);}
	{
		// May riding vertical Dolphin
		AddedMove move(CHARACTER_TYPE_MAY, "IrukasanRidingAttackTateA");
		move.displayName = "Hop on Dolphin";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_MAY, "IrukasanRidingAttackTateB");
		move.displayName = "Hop on Dolphin";
		addMove(move);}
	{
		// big whale attack
		AddedMove move(CHARACTER_TYPE_MAY, "Yamada", true);
		move.framebarName = "Yamada";
		move.framebarId = 12;
		move.isDangerous = isDangerous_not_hasHitNumButInactive;
		addMove(move);}
	{
		// May spins and may do a suicide whale in the end. This is the suicide whale
		AddedMove move(CHARACTER_TYPE_MAY, "SK_Goshogawara", true);
		move.framebarName = "Goshogawara";
		move.framebarId = 13;
		move.isDangerous = isDangerous_not_hasHitNumButNoHitboxes;
		addMove(move);}
	{
		// Chipp wall cling idle/moving up/down
		AddedMove move(CHARACTER_TYPE_CHIPP, "HaritsukiKeep");
		move.displayName = "Wall Climb";
		move.combineWithPreviousMove = true;
		move.isIdle = isIdle_enableWhiffCancels;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_CHIPP, "GammaBladeObj", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 26;
		move.framebarName = "Gamma Blade";
		move.combineWithPreviousMove = true;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_CHIPP, "ShurikenObj", true);
		move.isDangerous = isDangerous_notNull;
		move.framebarId = 27;
		move.framebarName = "Shuriken Slow";
		move.combineWithPreviousMove = true;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_CHIPP, "ShurikenObj1", true);
		move.isDangerous = isDangerous_notNull;
		move.framebarId = 28;
		move.framebarName = "Shuriken Fast";
		move.combineWithPreviousMove = true;
		addMove(move);}
	{
		// throwing daggers from wall cling
		AddedMove move(CHARACTER_TYPE_CHIPP, "Kunai_Wall", true);
		move.isDangerous = isDangerous_notNull;
		move.framebarId = 29;
		move.framebarName = "Kunai";
		move.combineWithPreviousMove = true;
		addMove(move);}
	{
		// 214214K air super
		AddedMove move(CHARACTER_TYPE_CHIPP, "Kunai", true);
		move.isDangerous = isDangerous_notNull;
		move.framebarId = 30;
		move.framebarName = "Ryuu Yanagi";
		move.combineWithPreviousMove = true;
		addMove(move);}
	// Faust Pogo
	{
		// Pogo entry
		AddedMove move(CHARACTER_TYPE_FAUST, "Souten");
		move.displayName = "Spear Point Centripetal Dance";
		move.isIdle = isIdle_enableWhiffCancels;
		addMove(move);}
	{
		// Pogo P
		AddedMove move(CHARACTER_TYPE_FAUST, "SoutenA");
		move.displayName = "Just A Taste!";
		move.isIdle = isIdle_enableWhiffCancels;
		addMove(move);}
	{
		// Pogo hop
		AddedMove move(CHARACTER_TYPE_FAUST, "Souten9");
		move.displayName = "Short Hop";
		move.isIdle = isIdle_enableWhiffCancels;
		addMove(move);}
	{
		// Pogo hop
		AddedMove move(CHARACTER_TYPE_FAUST, "Souten44");
		move.displayName = "Backward Movement";
		move.isIdle = isIdle_enableWhiffCancels;
		addMove(move);}
	{
		// Pogo 66
		AddedMove move(CHARACTER_TYPE_FAUST, "Souten66");
		move.displayName = "Forward Movement";
		move.isIdle = isIdle_enableWhiffCancels;
		addMove(move);}
	{
		// Pogo K (head flower)
		AddedMove move(CHARACTER_TYPE_FAUST, "SoutenB");
		move.displayName = "Growing Flower";
		move.sectionSeparator = sectionSeparator_soutenBC;
		move.isIdle = isIdle_enableWhiffCancels;
		addMove(move);}
	{
		// Pogo S (ground flower)
		AddedMove move(CHARACTER_TYPE_FAUST, "SoutenC");
		move.displayName = "See? I'm a Flower!";
		move.sectionSeparator = sectionSeparator_soutenBC;
		move.isIdle = isIdle_enableWhiffCancels;
		addMove(move);}
	{
		// Pogo Going My Wa
		AddedMove move(CHARACTER_TYPE_FAUST, "SoutenE");
		move.displayName = "Going My Way";
		move.isIdle = isIdle_enableWhiffCancels;
		addMove(move);}
	{
		// Faust Pogo Helicopter
		AddedMove move(CHARACTER_TYPE_FAUST, "Souten8");
		move.displayName = "Doctor-Copter";
		move.isIdle = isIdle_Souten8;
		addMove(move);}
	{
		// Faust 41236K (long ass fishing pole poke that drags you) succeeeding
		AddedMove move(CHARACTER_TYPE_FAUST, "Hikimodoshi");
		move.combineWithPreviousMove = true;
		addMove(move);}
	{
		// ground flower. The head flower cannot be RC'd
		AddedMove move(CHARACTER_TYPE_FAUST, "OreHana_Shot", true);
		move.isDangerous = isDangerous_playerInRCOrHasActiveFlag_AndNotInRecovery;
		move.framebarId = 31;
		move.framebarName = "Flower";
		addMove(move);}
	{
		// ground flower maximum
		AddedMove move(CHARACTER_TYPE_FAUST, "OreHanaBig_Shot", true);
		move.isDangerous = isDangerous_playerInRCOrHasActiveFlag_AndNotInRecovery;
		move.framebarId = 31;
		move.framebarName = "Flower";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_FAUST, "Item_Bomb", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 32;
		move.framebarName = "Bomb";
		addMove(move);}
	{
		// fire created when setting oil on fire
		AddedMove move(CHARACTER_TYPE_FAUST, "OilFire", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 33;
		move.framebarName = "Oil Fire";
		addMove(move);}
	{
		// normal meteor. Does not have active frames. Creates several MeteoInseki which have active frames
		AddedMove move(CHARACTER_TYPE_FAUST, "Item_Meteo", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 34;
		move.framebarName = "Meteor";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_FAUST, "MeteoInseki", true);
		move.isDangerous = isDangerous_notNull;
		move.framebarId = 34;
		move.framebarName = "Meteor";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_FAUST, "Item_Hammer", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 35;
		move.framebarName = "Hammer";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_FAUST, "Item_ChibiFaust", true);
		move.isDangerous = isDangerous_not_hasHitNumButNoHitboxes;
		move.framebarId = 36;
		move.framebarName = "Small Faust";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_FAUST, "Item_Frasco", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 37;
		move.framebarName = "Poison";
		addMove(move);}
	{
		// the poison cloud created when poison flask lands
		AddedMove move(CHARACTER_TYPE_FAUST, "SubItem_Poison", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 37;
		move.framebarName = "Poison";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_FAUST, "Item_JumpStand", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 38;
		move.framebarName = "Platform";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_FAUST, "Item_100t", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 39;
		move.framebarName = "100-ton Weight";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_FAUST, "Item_FireWorks", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 40;
		move.framebarName = "Fireworks";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_FAUST, "Item_Armageddon", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 41;
		move.framebarName = "Massive Meteor";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_FAUST, "ArmageddonInseki", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 41;
		move.framebarName = "Massive Meteor";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_FAUST, "Item_GoldenHammer", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 42;
		move.framebarName = "Golden Hammer";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_FAUST, "Item_BigFaust", true);
		move.isDangerous = isDangerous_not_hasHitNumButNoHitboxes;
		move.framebarId = 43;
		move.framebarName = "Huge Faust";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_FAUST, "Item_Golden100t", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 44;
		move.framebarName = "10,000 Ton Weight";
		addMove(move);}
	{
		// the initial projectile Faust drops
		AddedMove move(CHARACTER_TYPE_FAUST, "Ai_Bomb", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 45;
		move.framebarName = "Love";
		addMove(move);}
	{
		// the explosion created when Love touches the ground
		AddedMove move(CHARACTER_TYPE_FAUST, "Ai_Bomb2", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 45;
		move.framebarName = "Love";
		addMove(move);}
	{
		// Axl Haitaka stance
		AddedMove move(CHARACTER_TYPE_AXL, "DaiRensen");
		move.displayName = "Sparrowhawk Stance";
		move.isIdle = isIdle_enableWhiffCancels;
		addMove(move);}
	{
		// Axl Rensen + 2 followup
		AddedMove move(CHARACTER_TYPE_AXL, "Sensageki");
		move.displayName = "Spinning Chain Strike";
		move.combineWithPreviousMove = true;
		addMove(move);}
	{
		// Axl Rensen + 8 followup
		AddedMove move(CHARACTER_TYPE_AXL, "Kyokusageki");
		move.displayName = "Melody Chain";
		move.combineWithPreviousMove = true;
		addMove(move);}
	{
		// the command grab
		AddedMove move(CHARACTER_TYPE_AXL, "RashosenObj", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 46;
		move.framebarName = "Spindle Spinner";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_AXL, "RensengekiObj", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 47;
		move.framebarName = "Sickle Flash";
		addMove(move);}
	{
		// the 8 followup
		AddedMove move(CHARACTER_TYPE_AXL, "KyokusagekiObj", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 47;
		move.framebarName = "Melody Chain";
		addMove(move);}
	{
		// the 2363214H super second hit
		AddedMove move(CHARACTER_TYPE_AXL, "ByakueObj", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 48;
		move.framebarName = "Sickle Storm";
		addMove(move);}
	{
		// Elphelt Ms. Confille (rifle)
		AddedMove move(CHARACTER_TYPE_ELPHELT, "Rifle_Start");
		move.displayName = "Aim Ms. Confille";
		move.sectionSeparator = sectionSeparator_rifle;
		move.isIdle = isIdle_Rifle;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_ELPHELT, "Rifle_Reload");
		move.displayName = "Reload";
		move.sectionSeparator = sectionSeparator_rifle;
		move.isIdle = isIdle_Rifle;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_ELPHELT, "Rifle_Reload_Perfect");
		move.displayName = "Perfect Reload";
		move.sectionSeparator = sectionSeparator_rifle;
		move.isIdle = isIdle_Rifle;
		addMove(move);}
	{
		// entered into from CmnActRomanCancel if its performed during rifle stance after reloading.
		// On f1 whiff cancels are not enabled yet, on f2 enabled
		AddedMove move(CHARACTER_TYPE_ELPHELT, "Rifle_Reload_Roman");
		move.displayName = "Rifle RC";
		move.sectionSeparator = sectionSeparator_rifle;
		move.isIdle = isIdle_Rifle;
		addMove(move);}
	{
		// similar to Rifle_Reload_Roman but performed either after entering the stance of after firing, not after reloading.
		AddedMove move(CHARACTER_TYPE_ELPHELT, "Rifle_Roman");
		move.displayName = "Rifle RC";
		move.sectionSeparator = sectionSeparator_rifle;
		move.isIdle = isIdle_Rifle;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_ELPHELT, "Shotgun_Reload");
		move.displayName = "Shotgun Reload";
		move.combineWithPreviousMove = true;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_ELPHELT, "GrenadeBomb", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 73;
		move.framebarName = "Berry Pine";
		addMove(move);}
	{
		// This explosion results from the timer running out normally
		AddedMove move(CHARACTER_TYPE_ELPHELT, "GrenadeBomb_Explode", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 73;
		move.framebarName = "Berry Pine";
		addMove(move);}
	{
		// This explosion results from clashing with the opponent's projectiles
		AddedMove move(CHARACTER_TYPE_ELPHELT, "GrenadeBomb_Explode2", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 73;
		move.framebarName = "Berry Pine";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_ELPHELT, "HandGun_air_shot", true);
		move.isDangerous = isDangerous_playerInRCOrHasActiveFlag_AndNotInRecovery;
		move.framebarId = 74;
		move.framebarName = "j.D";
		addMove(move);}
	{
		// Max charge shotgun shot spawns two projectiles: Shotgun_max_1, Shotgun_max_2
		AddedMove move(CHARACTER_TYPE_ELPHELT, "Shotgun_max_1", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 75;
		move.framebarName = "Shotgun";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_ELPHELT, "Shotgun_max_2", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 75;
		move.framebarName = "Shotgun";
		addMove(move);}
	{
		// Shotgun shot spawns two projectiles: Shotgun_min_1, Shotgun_min_2
		AddedMove move(CHARACTER_TYPE_ELPHELT, "Shotgun_min_1", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 75;
		move.framebarName = "Shotgun";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_ELPHELT, "Shotgun_min_2", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 75;
		move.framebarName = "Shotgun";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_ELPHELT, "Bazooka_Fire", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 76;
		move.framebarName = "Genoverse";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_ELPHELT, "Bazooka_Explosive", true);
		move.isDangerous = isDangerous_not_NullWhileActive;
		move.framebarId = 76;
		move.framebarName = "Genoverse";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_ELPHELT, "Rifle_Fire_MAX", true);
		move.isDangerous = isDangerous_not_NullWhileActive;
		move.framebarId = 77;
		move.framebarName = "Ms. Confille";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_ELPHELT, "Rifle_Fire_MIN", true);
		move.isDangerous = isDangerous_not_NullWhileActive;
		move.framebarId = 77;
		move.framebarName = "Ms. Confille";
		addMove(move);}
	{
		// Leo backturn idle and also exiting backturn via 22
		AddedMove move(CHARACTER_TYPE_LEO, "Semuke");
		move.displayName = "Brynhildr Stance";
		move.isIdle = isIdle_enableWhiffCancels;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_LEO, "NmlAtk5CFar_Guard");
		move.displayName = "f.S~P";
		move.sectionSeparator = sectionSeparator_leoGuardStance;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_LEO, "NmlAtk5D_Guard");
		move.displayName = "5H~P";
		move.sectionSeparator = sectionSeparator_leoGuardStance;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_LEO, "Edgeyowai", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 78;
		move.framebarName = "Graviert W\xc3\xbcrde";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_LEO, "Edgetuyoi", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 78;
		move.framebarName = "Graviert W\xc3\xbcrde";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_LEO, "SemukeKakusei_Obj", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 79;
		move.framebarName = "Stahl Wirbel";
		addMove(move);}
	{
		// Jam parry
		AddedMove move(CHARACTER_TYPE_JAM, "NeoHochihu");
		move.displayName = "Hochifu";
		move.isIdle = canBlock_neoHochihu;
		addMove(move);}
	{
		// Jam 236S~H
		AddedMove move(CHARACTER_TYPE_JAM, "SenriShinshou");
		move.displayName = "Senri Shinshou";
		move.combineWithPreviousMove = true;
		move.usePlusSignInCombination = true;
		addMove(move);}
	{
		// Jam 236S~S
		AddedMove move(CHARACTER_TYPE_JAM, "HyappoShinshou");
		move.displayName = "Hyappo Shinshou";
		move.combineWithPreviousMove = true;
		move.usePlusSignInCombination = true;
		addMove(move);}
	{
		// Jam 236S~K
		AddedMove move(CHARACTER_TYPE_JAM, "Ashibarai");
		move.displayName = "Hamonkyaku";
		move.combineWithPreviousMove = true;
		move.usePlusSignInCombination = true;
		addMove(move);}
	{
		// Jam 236S~P
		AddedMove move(CHARACTER_TYPE_JAM, "Mawarikomi");
		move.displayName = "Mawarikomi";
		move.combineWithPreviousMove = true;
		move.usePlusSignInCombination = true;
		addMove(move);}
	{
		// Jam 46P
		AddedMove move(CHARACTER_TYPE_JAM, "TuikaA");
		move.displayName = "Zekkei";
		move.combineWithPreviousMove = true;
		move.usePlusSignInCombination = true;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JAM, "RenhoukyakuObj", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 88;
		move.framebarName = "Renhoukyaku";
		addMove(move);}
	{
		// Answer scroll cling idle
		AddedMove move(CHARACTER_TYPE_ANSWER, "Ami_Hold");
		move.displayName = "Savvy Ninpo: Seal of Approval";
		move.isIdle = isIdle_enableWhiffCancels;
		addMove(move);}
	{
		// Answer sD
		AddedMove move(CHARACTER_TYPE_ANSWER, "Ami_Move");
		move.displayName = "Savvy Ninpo: Safety Net";
		move.isIdle = alwaysFalse;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_ANSWER, "Meishi", true);
		move.isDangerous = isDangerous_card;
		move.framebarId = 103;
		move.framebarName = "BN: Caltrops";
		move.framebarNameFull = "Business Ninpo: Caltrops";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_ANSWER, "Nin_Jitsu", true);
		move.isDangerous = isDangerous_not_NullWhileActive;
		move.framebarId = 104;
		move.framebarName = "BN: Under the Bus";
		move.framebarNameFull = "Business Ninpo: Under the Bus";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_ANSWER, "RSF_Start", true);
		move.isDangerous = isDangerous_card;
		move.framebarId = 106;
		move.framebarName = "Dead Stock Ninpo: Firesale";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_ANSWER, "RSF_Meishi", true);
		move.isDangerous = isDangerous_rsfMeishi;
		move.framebarId = 106;
		move.framebarName = "Dead Stock Ninpo: Firesale";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_ANSWER, "RSF_Finish", true);
		move.isDangerous = isDangerous_rsfMeishi;
		move.framebarId = 106;
		move.framebarName = "Dead Stock Ninpo: Firesale";
		addMove(move);}
	{
		// Millia Roll Roll
		AddedMove move(CHARACTER_TYPE_MILLIA, "SaiZenten");
		move.displayName = "Forward Roll Again";
		move.combineWithPreviousMove = true;
		move.usePlusSignInCombination = true;
		addMove(move);}
	{
		// Millia Roll > S
		AddedMove move(CHARACTER_TYPE_MILLIA, "ZentenShaker");
		move.displayName = "Lust Shaker (Follow-up)";
		move.combineWithPreviousMove = true;
		move.usePlusSignInCombination = true;
		addMove(move);}
	{
		// Millia Roll > H
		AddedMove move(CHARACTER_TYPE_MILLIA, "Digitalis");
		move.displayName = "Digitalis";
		move.combineWithPreviousMove = true;
		move.usePlusSignInCombination = true;
		addMove(move);}
	{
		// represents both S and H pins
		AddedMove move(CHARACTER_TYPE_MILLIA, "SilentForceKnife", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 14;
		move.framebarName = "Silent Force";
		addMove(move);}
	{
		// s-disc
		AddedMove move(CHARACTER_TYPE_MILLIA, "TandemTopCRing", true);
		move.isDangerous = isDangerous_notNull;
		move.framebarId = 15;
		move.framebarName = "Tandem Top";
		addMove(move);}
	{
		// h-disc
		AddedMove move(CHARACTER_TYPE_MILLIA, "TandemTopDRing", true);
		move.isDangerous = isDangerous_notNull;
		move.framebarId = 16;
		move.framebarName = "Tandem Top";
		addMove(move);}
	{
		// each ring of the 236236S super is separately named
		AddedMove move(CHARACTER_TYPE_MILLIA, "EmeraldRainRing1", true);
		move.isDangerous = isDangerous_notNull;
		move.framebarId = 17;
		move.framebarName = "Emerald Rain";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_MILLIA, "EmeraldRainRing2", true);
		move.isDangerous = isDangerous_notNull;
		move.framebarId = 17;
		move.framebarName = "Emerald Rain";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_MILLIA, "EmeraldRainRing3", true);
		move.isDangerous = isDangerous_notNull;
		move.framebarId = 17;
		move.framebarName = "Emerald Rain";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_MILLIA, "SecretGardenBall", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 18;
		move.framebarName = "Secret Garden";
		addMove(move);}
	{
		// a rose created during Rose Install. Many of these can be on the screen at the same time
		AddedMove move(CHARACTER_TYPE_MILLIA, "RoseObj", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 19;
		move.framebarName = "Rose";
		addMove(move);}
	{
		// Zato 214K
		AddedMove move(CHARACTER_TYPE_ZATO, "BreakTheLaw");
		move.displayName = "Break the Law";
		move.sectionSeparator = sectionSeparator_breakTheLaw;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_ZATO, "DrillC", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 20;
		move.framebarName = "Drill";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_ZATO, "DrillD", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 20;
		move.framebarName = "Drill";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_ZATO, "AmorphousObj", true);
		move.isDangerous = isDangerous_amorphous;
		move.framebarId = 21;
		move.framebarName = "Amorphous";
		addMove(move);}
	{
		// this can only be created on the boss version of Zato
		AddedMove move(CHARACTER_TYPE_ZATO, "AmorphousObj2", true);
		move.isDangerous = isDangerous_not_hasHitNumButNoHitboxes;
		move.framebarId = 21;
		move.framebarName = "Amorphous";
		addMove(move);}
	{
		// Potemkin Flick
		AddedMove move(CHARACTER_TYPE_POTEMKIN, "FDB");
		move.displayName = "F.D.B.";
		move.sectionSeparator = sectionSeparator_FDB;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_POTEMKIN, "HammerFallBrake");
		move.displayName = "Hammer Fall Break";
		move.combineWithPreviousMove = true;
		move.usePlusSignInCombination = true;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_POTEMKIN, "SlideHead_Obj", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 22;
		move.framebarName = "Slide Head";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_POTEMKIN, "FDB_Obj", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 23;
		move.framebarName = "FDB";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_POTEMKIN, "GiganObj", true);
		move.isDangerous = isDangerous_notNull;
		move.framebarId = 24;
		move.framebarName = "Giganter";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_POTEMKIN, "Bomb", true);
		move.isDangerous = isDangerous_not_hasHitNumButNoHitboxes;
		move.framebarId = 25;
		move.framebarName = "Trishula";
		addMove(move);}
	{
		// Venom QV
		AddedMove move(CHARACTER_TYPE_VENOM, "DubiousCurveA");
		move.displayName = "P QV";
		move.sectionSeparator = sectionSeparator_QV;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_VENOM, "DubiousCurveB");
		move.displayName = "K QV";
		move.sectionSeparator = sectionSeparator_QV;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_VENOM, "DubiousCurveC");
		move.displayName = "S QV";
		move.sectionSeparator = sectionSeparator_QV;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_VENOM, "DubiousCurveD");
		move.displayName = "H QV";
		move.sectionSeparator = sectionSeparator_QV;
		addMove(move);}
	{
		// this is Stinger and Carcass Raid balls, ball set, including when such balls are launched.
		// Charged balls and even Bishop Runout and Red Hail are also this
		AddedMove move(CHARACTER_TYPE_VENOM, "Ball", true);
		move.isDangerous = isDangerous_active;
		move.framebarId = 49;
		move.framebarName = "Balls";
		addMove(move);}
	{
		// every QV when released creates this shockwave and it persists on RC
		AddedMove move(CHARACTER_TYPE_VENOM, "Debious_AttackBall", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 50;
		move.framebarName = "QV";
		addMove(move);}
	{
		// created before Dark Angel comes out
		AddedMove move(CHARACTER_TYPE_VENOM, "DarkAngelBallStart", true);
		move.isDangerous = isDangerous_hasNotCreatedAnythingYet;
		move.framebarId = 51;
		move.framebarName = "Dark Angel";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_VENOM, "DarkAngelBall", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 51;
		move.framebarName = "Dark Angel";
		addMove(move);}
	{
		// Slayer dandy step follow-ups
		AddedMove move(CHARACTER_TYPE_SLAYER, "CrossWise");
		move.displayName = "Crosswise Heel";
		move.combineWithPreviousMove = true;
		move.usePlusSignInCombination =  true;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_SLAYER, "UnderPressure");
		move.displayName = "Under Pressure";
		move.combineWithPreviousMove = true;
		move.usePlusSignInCombination =  true;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_SLAYER, "RetroFire");
		move.displayName = "Helter Skelter";
		move.combineWithPreviousMove = true;
		move.usePlusSignInCombination =  true;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_SLAYER, "PileBunker");
		move.displayName = "Pilebunker";
		move.combineWithPreviousMove = true;
		move.usePlusSignInCombination =  true;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_SLAYER, "Retro", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 52;
		move.framebarName = "Helter Skelter";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_SLAYER, "KetsuFire", true);
		move.isDangerous = isDangerous_not_hasHitNumButNoHitboxes;
		move.framebarId = 53;
		move.framebarName = "Straight-Down Dandy";
		addMove(move);}
	{
		// I-No Sultry Performance
		AddedMove move(CHARACTER_TYPE_INO, "KyougenA");
		move.displayName = "P Sultry Performance";
		move.sectionSeparator = sectionSeparator_sultryPerformance;
		move.isIdle = isIdle_enableWhiffCancels;
		move.canBlock = canBlock_default;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_INO, "KyougenB");
		move.displayName = "K Sultry Performance";
		move.sectionSeparator = sectionSeparator_sultryPerformance;
		move.isIdle = isIdle_enableWhiffCancels;
		move.canBlock = canBlock_default;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_INO, "KyougenC");
		move.displayName = "S Sultry Performance";
		move.sectionSeparator = sectionSeparator_sultryPerformance;
		move.isIdle = isIdle_enableWhiffCancels;
		move.canBlock = canBlock_default;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_INO, "KyougenD");
		move.displayName = "H Sultry Performance";
		move.sectionSeparator = sectionSeparator_sultryPerformance;
		move.isIdle = isIdle_enableWhiffCancels;
		move.canBlock = canBlock_default;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_INO, "ChemicalAdd");
		move.displayName = "Chemical Love (Follow-up)";
		move.combineWithPreviousMove = true;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_INO, "BChemiLaser", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 54;
		move.framebarName = "Chemical Love";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_INO, "AddChemiLaser", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 54;
		move.framebarName = "Chemical Love (Follow-up)";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_INO, "CChemiLaser", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 54;
		move.framebarName = "Chemical Love (Vertical)";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_INO, "Onpu", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 55;
		move.framebarName = "Antidepressant Scale";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_INO, "DustObjShot", true);
		move.isDangerous = isDangerous_playerInRCOrHasActiveFlag_AndNotInRecovery;
		move.framebarId = 56;
		move.framebarName = "5D";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_INO, "GenkaiObj", true);
		move.isDangerous = isDangerous_not_hasHitNumButNoHitboxes;
		move.framebarId = 57;
		move.framebarName = "Ultimate Fortissimo";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_INO, "MadogiwaObj", true);
		move.isDangerous = isDangerous_not_hasHitNumButNoHitboxes;
		move.framebarId = 58;
		move.framebarName = "Longing Desperation";
		addMove(move);}
	{
		// Bedman Teleporting from the boomerang head hitting
		AddedMove move(CHARACTER_TYPE_BEDMAN, "BWarp");
		move.displayName = "Task B Teleport";
		move.combineWithPreviousMove = true;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_BEDMAN, "Aralm_Obj", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 59;
		move.framebarName = "Sinusoidal Helios";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_BEDMAN, "Okkake", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 60;
		move.framebarName = "Hemi Jack";
		addMove(move);}
	{
		// the flying head
		AddedMove move(CHARACTER_TYPE_BEDMAN, "Boomerang_A_Head", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 61;
		move.framebarName = "Task A";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_BEDMAN, "Boomerang_A_Head_Air", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 61;
		move.framebarName = "Task A";
		addMove(move);}
	{
		// created when doing Deja Vu (Task A). Creates either Boomerang_A_Djavu or Boomerang_A_Djavu_Air
		AddedMove move(CHARACTER_TYPE_BEDMAN, "Djavu_A_Ghost", true);
		move.isDangerous = isDangerous_djavu;
		move.framebarId = 62;
		move.framebarName = "Deja Vu (Task A)";
		addMove(move);}
	{
		// the flying head
		AddedMove move(CHARACTER_TYPE_BEDMAN, "Boomerang_A_Djavu", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 62;
		move.framebarName = "Deja Vu (Task A)";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_BEDMAN, "Boomerang_A_Djavu_Air", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 62;
		move.framebarName = "Deja Vu (Task A)";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_BEDMAN, "Boomerang_B_Head", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 63;
		move.framebarName = "Task A'";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_BEDMAN, "Boomerang_B_Head_Air", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 63;
		move.framebarName = "Task A'";
		addMove(move);}
	{
		// created when doing Deja Vu (Task A'). Creates either Boomerang_B_Djavu or Boomerang_B_Djavu_Air
		AddedMove move(CHARACTER_TYPE_BEDMAN, "Djavu_B_Ghost", true);
		move.isDangerous = isDangerous_djavu;
		move.framebarId = 64;
		move.framebarName = "Deja Vu (Task A')";
		addMove(move);}
	{
		// the flying head
		AddedMove move(CHARACTER_TYPE_BEDMAN, "Boomerang_B_Djavu", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 64;
		move.framebarName = "Deja Vu (Task A')";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_BEDMAN, "Boomerang_B_Djavu_Air", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 64;
		move.framebarName = "Deja Vu (Task A')";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_BEDMAN, "Djavu_C_Ghost", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 65;
		move.framebarName = "Deja Vu (Task B)";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_BEDMAN, "bomb1", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 66;
		move.framebarName = "Task C Shockwave";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_BEDMAN, "bomb2", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 66;
		move.framebarName = "Task C Shockwave";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_BEDMAN, "Djavu_D_Ghost", true);
		move.isDangerous = isDangerous_Djavu_D_Ghost;
		move.framebarId = 67;
		move.framebarName = "Deja Vu (Task C)";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_RAMLETHAL, "BitN6C", true);
		move.isDangerous = isDangerous_launchGreatsword;
		move.framebarId = 68;
		move.framebarName = "S Sword";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_RAMLETHAL, "BitF6D", true);
		move.isDangerous = isDangerous_launchGreatsword;
		move.framebarId = 69;
		move.framebarName = "H Sword";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_RAMLETHAL, "BitN6CBunriShot", true);
		move.isDangerous = isDangerous_ramSwordMove;
		move.framebarId = 68;
		move.framebarName = "S Sword";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_RAMLETHAL, "BitN6CBunriShot_Boss", true);
		move.isDangerous = isDangerous_ramSwordMove;
		move.framebarId = 68;
		move.framebarName = "S Sword";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_RAMLETHAL, "BitF6DBunriShot", true);
		move.isDangerous = isDangerous_ramSwordMove;
		move.framebarId = 69;
		move.framebarName = "H Sword";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_RAMLETHAL, "BitF6DBunriShot_Boss", true);
		move.isDangerous = isDangerous_ramSwordMove;
		move.framebarId = 69;
		move.framebarName = "H Sword";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_RAMLETHAL, "BitSpiral_NSpiral", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 68;
		move.framebarName = "S Sword";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_RAMLETHAL, "BitSpiral_FSpiral", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 69;
		move.framebarName = "H Sword";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_RAMLETHAL, "BitLaser_Minion", true);
		move.isDangerous = isDangerous_hasNotCreatedAnythingYet;
		move.framebarId = 70;
		move.framebarName = "Laser";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_RAMLETHAL, "BitLaser", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 70;
		move.framebarName = "Laser";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_RAMLETHAL, "middleShot", true);
		move.isDangerous = isDangerous_not_hasHitNumButNoHitboxes;
		move.framebarId = 71;
		move.framebarName = "Cassius";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_SIN, "BeakDriver");
		move.displayName = "Beak Driver";
		move.sectionSeparator = sectionSeparator_beakDriver;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_SIN, "SuperShotStart", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 72;
		move.framebarName = "Voltec Dein";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_SIN, "Shot_Land", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 72;
		move.framebarName = "Voltec Dein";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_SIN, "SuperShotAirStart", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 72;
		move.framebarName = "Voltec Dein";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_SIN, "Shot_Air", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 72;
		move.framebarName = "Voltec Dein";
		addMove(move);}
	{
		// Haehyun 21[4K]
		AddedMove move(CHARACTER_TYPE_HAEHYUN, "LandBlow4Hasei");
		move.displayName = "Falcon Dive (Reverse Ver.)";
		move.sectionSeparator = sectionSeparator_falconDive;
		move.combineWithPreviousMove = true;
		addMove(move);}
	{
		// Haehyun 214[K]
		AddedMove move(CHARACTER_TYPE_HAEHYUN, "LandBlow6Hasei");
		move.displayName = "Falcon Dive";
		move.sectionSeparator = sectionSeparator_falconDive;
		move.combineWithPreviousMove = true;
		addMove(move);}
	{
		// Haehyun 623[K]
		AddedMove move(CHARACTER_TYPE_HAEHYUN, "AntiAir6Hasei");
		move.displayName = "Four Tigers Sword (Hold)";
		move.combineWithPreviousMove = true;
		addMove(move);}
	{
		// Haehyun 623[4K]
		AddedMove move(CHARACTER_TYPE_HAEHYUN, "AntiAir4Hasei");
		move.displayName = "Four Tigers Sword (Reverse Ver.)";
		move.combineWithPreviousMove = true;
		move.sectionSeparator = sectionSeparator_fourTigersSwordRev;
		addMove(move);}
	{
		// Haehyun 236236H
		AddedMove move(CHARACTER_TYPE_HAEHYUN, "BlackHoleAttack");
		move.displayName = "Enlightened 3000 Palm Strike";
		move.sectionSeparator = sectionSeparator_blackHoleAttack;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_HAEHYUN, "EnergyBall", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 89;
		move.framebarName = "Tuning Ball";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_HAEHYUN, "SuperEnergyBall", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 90;
		move.framebarName = "Celestial Tuning Ball";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_HAEHYUN, "kum_205shot", true);
		move.isDangerous = isDangerous_kum5D;
		move.framebarId = 105;
		move.framebarName = "5D";
		addMove(move);}
	{
		// Raven stance
		AddedMove move(CHARACTER_TYPE_RAVEN, "ArmorDance");
		move.displayName = "Give it to me HERE";
		move.sectionSeparator = sectionSeparator_armorDance;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_RAVEN, "SlowNeeldeObjLand", true);
		move.isDangerous = isDangerous_not_NullWhileActive;
		move.framebarId = 90;
		move.framebarName = "Schmerz Berg";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_RAVEN, "SlowNeeldeObjAir", true);
		move.isDangerous = isDangerous_not_NullWhileActive;
		move.framebarId = 90;
		move.framebarName = "Grebechlich Licht";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_RAVEN, "AirSettingTypeNeedleObj", true);
		move.isDangerous = isDangerous_not_NullWhileActive;
		move.framebarId = 91;
		move.framebarName = "Scharf Kugel";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_RAVEN, "LandSettingTypeNeedleObj", true);
		move.isDangerous = isDangerous_not_NullWhileActive;
		move.framebarId = 91;
		move.framebarName = "Scharf Kugel";
		addMove(move);}
	{
		// Dizzy 421H
		AddedMove move(CHARACTER_TYPE_DIZZY, "KinomiNecro");
		move.displayName = "For roasting chestnuts...";
		move.sectionSeparator = sectionSeparator_kinomiNecro;
		addMove(move);}
	{
		// Baiken Azami
		AddedMove move(CHARACTER_TYPE_BAIKEN, "BlockingStand");
		move.displayName = "Azami (Standing)";
		move.sectionSeparator = sectionSeparator_azami;
		addMove(move);}
	{
		// Baiken Azami
		AddedMove move(CHARACTER_TYPE_BAIKEN, "BlockingCrouch");
		move.displayName = "Azami (Crouching)";
		move.sectionSeparator = sectionSeparator_azami;
		addMove(move);}
	{
		// Baiken Azami
		AddedMove move(CHARACTER_TYPE_BAIKEN, "BlockingAir");
		move.displayName = "Azami (Aerial)";
		move.sectionSeparator = sectionSeparator_azami;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_BAIKEN, "NmlAtk5EShotObj", true);
		move.isDangerous = isDangerous_playerInRCOrHasActiveFlag_AndNotInRecovery;
		move.framebarId = 99;
		move.framebarName = "5D";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_BAIKEN, "NmlAtkAir5EShotObj", true);
		move.isDangerous = isDangerous_playerInRCOrHasActiveFlag_AndNotInRecovery;
		move.framebarId = 100;
		move.framebarName = "j.D";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_BAIKEN, "TeppouObj", true);
		move.isDangerous = isDangerous_not_hasHitNumButNoHitboxes;
		move.framebarId = 101;
		move.framebarName = "Yasha Gatana";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_BAIKEN, "TatamiLandObj", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 102;
		move.framebarName = "Tatami Gaeshi";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_BAIKEN, "TatamiAirObj", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 102;
		move.framebarName = "Tatami Gaeshi";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JACKO, "OrganOpen");
		move.displayName = "Organ Deployment";
		move.sectionSeparator = sectionSeparator_organ;
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JACKO, "ServantA", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 82;
		move.framebarName = "Servants (Ground)";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JACKO, "ServantB", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 82;
		move.framebarName = "Servants (Ground)";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JACKO, "magicAtkLv1", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 83;
		move.framebarName = "Servants (Air)";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JACKO, "magicAtkLv2", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 83;
		move.framebarName = "Servants (Air)";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JACKO, "magicAtkLv3", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 83;
		move.framebarName = "Servants (Air)";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JACKO, "Fireball", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 84;
		move.framebarName = "Remove the Chain of Chiron";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JACKO, "CalvadosObj", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 85;
		move.framebarName = "Calvados";
		addMove(move);}
	{
		// Only the boss version spawns this
		AddedMove move(CHARACTER_TYPE_JACKO, "CalvadosObj2", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 85;
		move.framebarName = "Calvados";
		addMove(move);}
	{
		// Only the boss version spawns this
		AddedMove move(CHARACTER_TYPE_JACKO, "CalvadosObj3", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 85;
		move.framebarName = "Calvados";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JACKO, "Bomb", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 85;
		move.framebarName = "Calvados";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JACKO, "GhostA", true);
		move.isDangerous = isDangerous_displayModel;
		move.framebarId = 86;
		move.framebarName = "Ghost";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JACKO, "GhostB", true);
		move.isDangerous = isDangerous_displayModel;
		move.framebarId = 86;
		move.framebarName = "Ghost";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JACKO, "GhostC", true);
		move.isDangerous = isDangerous_displayModel;
		move.framebarId = 86;
		move.framebarName = "Ghost";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JACKO, "Suicidal_explosion", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 87;
		move.framebarName = "Explosion";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JACKO, "Suicidal_explosion2", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 87;
		move.framebarName = "Explosion";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_JACKO, "Suicidal_explosion3", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 87;
		move.framebarName = "Explosion";
		addMove(move);}
	{
		//  ifOperation: (IS_EQUAL), Mem(CREATE_ARG_HIKITSUKI_VAL_1), Val(1)
		//    gotoLabelRequests: s32'Necro'
		//  endIf: 
		AddedMove move(CHARACTER_TYPE_DIZZY, "SakanaObj", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 92;
		move.framebarName = "Ice Spike/Fire Pillar";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_DIZZY, "AkariObj", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 93;
		move.framebarName = "Ice/Fire Scythe";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_DIZZY, "AwaPObj", true);
		move.isDangerous = isDangerous_bubble;
		move.framebarId = 94;
		move.framebarName = "Please, leave me alone";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_DIZZY, "AwaKObj", true);
		move.isDangerous = isDangerous_bubble;
		move.framebarId = 94;
		move.framebarName = "What happens when I'm TOO alone";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_DIZZY, "KinomiObj", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 95;
		move.framebarName = "I use this to pick fruit";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_DIZZY, "KinomiObjNecro", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 95;
		move.framebarName = "For roasting chestnuts...";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_DIZZY, "KinomiObjNecro2", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 95;
		move.framebarName = "For roasting chestnuts...";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_DIZZY, "KinomiObjNecro3", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 95;
		move.framebarName = "For roasting chestnuts...";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_DIZZY, "KinomiObjNecrobomb", true);
		move.isDangerous = isDangerous_not_hasHitNumButInactive;
		move.framebarId = 95;
		move.framebarName = "For roasting chestnuts...";
		addMove(move);}
	{
		// P fish
		AddedMove move(CHARACTER_TYPE_DIZZY, "HanashiObjA", true);
		move.isDangerous = isDangerous_not_hasHitNumButInactive;
		move.framebarId = 96;
		move.framebarName = "We talked a lot together";
		addMove(move);}
	{
		// K fish
		AddedMove move(CHARACTER_TYPE_DIZZY, "HanashiObjB", true);
		move.isDangerous = isDangerous_kFish;
		move.framebarId = 96;
		move.framebarName = "We talked a lot together";
		addMove(move);}
	{
		// S laser fish
		AddedMove move(CHARACTER_TYPE_DIZZY, "HanashiObjD", true);
		move.isDangerous = isDangerous_laserFish;
		move.framebarId = 96;
		move.framebarName = "We fought a lot together";
		addMove(move);}
	{
		// H laser fish
		AddedMove move(CHARACTER_TYPE_DIZZY, "HanashiObjC", true);
		move.isDangerous = isDangerous_laserFish;
		move.framebarId = 96;
		move.framebarName = "We fought a lot together";
		addMove(move);}
	{
		// H/S laser fish's laser
		AddedMove move(CHARACTER_TYPE_DIZZY, "Laser", true);
		move.isDangerous = isDangerous_notNull;
		move.framebarId = 96;
		move.framebarName = "We fought a lot together";
		addMove(move);}
	{
		// Shield fish
		AddedMove move(CHARACTER_TYPE_DIZZY, "HanashiObjE", true);
		move.isDangerous = isDangerous_notInRecovery;
		move.framebarId = 96;
		move.framebarName = "We fought a lot together";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_DIZZY, "ImperialRayCreater", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 97;
		move.framebarName = "Imperial Ray";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_DIZZY, "ImperialRayBakuhatsu", true);
		move.isDangerous = isDangerous_alwaysTrue;
		move.framebarId = 97;
		move.framebarName = "Imperial Ray";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_DIZZY, "GammaRayLaser", true);
		move.isDangerous = isDangerous_notNull;
		move.framebarId = 98;
		move.framebarName = "Gamma Ray";
		addMove(move);}
	{
		AddedMove move(CHARACTER_TYPE_DIZZY, "GammaRayLaserMax", true);
		move.isDangerous = isDangerous_not_NullWhileActive;
		move.framebarId = 98;
		move.framebarName = "Gamma Ray";
		addMove(move);}
		
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
	markerPos = moves.gotoNextInstruction(markerPos);
	if (moves.instructionType(markerPos) != Moves::instr_sprite) return false;
	return ent.bbscrCurrentInstr() > markerPos;
}
bool sectionSeparator_soutenBC(Entity ent) {
	return strcmp(ent.gotoLabelRequest(), "open") == 0
		|| !ent.hasUpon(3);
}
bool sectionSeparator_QV(Entity ent) {
	return strcmp(ent.gotoLabelRequest(), "End") == 0
		|| !ent.mem45() && ent.currentAnimDuration() > 12;
}
bool sectionSeparator_sultryPerformance(Entity ent) {
	return strcmp(ent.gotoLabelRequest(), "Attack") == 0
		|| !ent.hasUpon(3) && ent.currentAnimDuration() > 9;
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
bool sectionSeparator_organ(Entity ent) {
	return strcmp(ent.gotoLabelRequest(), "A") == 0
		|| strcmp(ent.gotoLabelRequest(), "B") == 0
		|| strcmp(ent.gotoLabelRequest(), "C") == 0
		|| strcmp(ent.gotoLabelRequest(), "D") == 0
		|| strcmp(ent.gotoLabelRequest(), "E") == 0
		|| strcmp(ent.gotoLabelRequest(), "tame") == 0;
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

MoveInfo::MoveInfo(bool combineWithPreviousMove,
		bool usePlusSignInCombination,
		const char* displayName,
		sectionSeparator_t sectionSeparator,
		int considerIdleInSeparatedSectionAfterThisManyFrames,
		bool preservesNewSection,
		isIdle_t isIdle,
		isIdle_t canBlock,
		isDangerous_t isDangerous,
		int framebarId,
		const char* framebarName,
		const char* framebarNameFull)
		:
		combineWithPreviousMove(combineWithPreviousMove),
		usePlusSignInCombination(usePlusSignInCombination),
		displayName(displayName),
		sectionSeparator(sectionSeparator),
		considerIdleInSeparatedSectionAfterThisManyFrames(considerIdleInSeparatedSectionAfterThisManyFrames),
		preservesNewSection(preservesNewSection),
		isIdle(isIdle ? isIdle : isIdle_default),
		isDangerous(isDangerous),
		framebarId(framebarId),
		framebarName(framebarName),
		framebarNameFull(framebarNameFull) {
	
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

BYTE* Moves::findCreateObj(BYTE* in, const char* name) const {
	while (true) {
		InstructionType type = instructionType(in);
		if (type == instr_endState) {
			return nullptr;
		}
		if (type == instr_createObject
				&& strcmp((const char*)(in + 4), name) == 0) {
			return in;
		}
		in += bbscrInstructionSizes[type];
	}
}

BYTE* Moves::findSpriteNull(BYTE* in) const {
	while (true) {
		InstructionType type = instructionType(in);
		if (type == instr_endState) {
			return nullptr;
		}
		if (type == instr_sprite
				&& strcmp((const char*)(in + 4), "null") == 0) {
			return in;
		}
		in += bbscrInstructionSizes[type];
	}
}

BYTE* Moves::findSpriteNonNull(BYTE* in) const {
	while (true) {
		InstructionType type = instructionType(in);
		if (type == instr_endState) {
			return nullptr;
		}
		if (type == instr_sprite
				&& strcmp((const char*)(in + 4), "null") != 0) {
			return in;
		}
		in += bbscrInstructionSizes[type];
	}
}

int Moves::hashString(const char* str, int startingHash) {
	for (const char* c = str; *c != '\0'; ++c) {
		startingHash = startingHash * 0x89 + *c;
	}
	return startingHash;
}

bool isDangerous_default(Entity ent) {
	return false;
}

bool isDangerous_alwaysTrue(Entity ent) {
	return true;
}

bool isDangerous_gunflame(Entity ent) {
	return !ent.isRecoveryState() && !ent.mem46();
}

bool isDangerous_notInRecovery(Entity ent) {
	return !ent.isRecoveryState();
}

bool isDangerous_playerInRCOrHasActiveFlag_AndNotInRecovery(Entity ent) {
	return !ent.isRecoveryState() && (ent.playerEntity().cmnActIndex() == CmnActRomanCancel || ent.hasActiveFlag());
}

bool isDangerous_notNull(Entity ent) {
	return strcmp(ent.spriteName(), "null") != 0;
}

bool isDangerous_not_NullWhileActive(Entity ent) {
	return !(strcmp(ent.spriteName(), "null") == 0 && ent.hasActiveFlag());
}

bool isDangerous_aboveGround(Entity ent) {
	return ent.y() >= 0;
}

bool isDangerous_not_hasHitNumButInactive(Entity ent) {
	return !(ent.currentHitNum() != 0 && !ent.hasActiveFlag());
}

bool isDangerous_not_hasHitNumButNoHitboxes(Entity ent) {
	return !(ent.currentHitNum() != 0 && ent.hitboxCount(HITBOXTYPE_HITBOX) == 0);
}

bool isDangerous_amorphous(Entity ent) {
	return !(ent.currentHitNum() != 0 && ent.hitboxCount(HITBOXTYPE_HITBOX) == 0)
		|| ent.untechableTime() == 60; // boss version
}

bool isDangerous_active(Entity ent) {
	return ent.isActiveFrames();
}

bool isDangerous_hasNotCreatedAnythingYet(Entity ent) {
	return ent.previousEntity() == nullptr;
}

bool isDangerous_djavu(Entity ent) {
	return ent.stackEntity(0) == nullptr;
}

bool isDangerous_Djavu_D_Ghost(Entity ent) {
	return !(ent.atkAngle() == -90 && ent.hitboxCount(HITBOXTYPE_HITBOX) == 0);
}

bool isDangerous_launchGreatsword(Entity ent) {
	return !(ent.currentHitNum() != 0 && ent.hitboxCount(HITBOXTYPE_HITBOX) == 0
		|| !ent.hasUpon(38));
}

bool isDangerous_ramSwordMove(Entity ent) {
	return !(ent.currentHitNum() == 3 && ent.hitboxCount(HITBOXTYPE_HITBOX) == 0
		|| !ent.hasUpon(38));
}

bool isDangerous_hasHitboxes(Entity ent) {
	return ent.hitboxCount(HITBOXTYPE_HITBOX) > 0;
}

bool isDangerous_bubble(Entity ent) {
	BYTE* markerPos = moves.findSetMarker(ent.bbscrCurrentFunc(), "bomb");
	if (!markerPos) {
		return false;
	}
	return ent.bbscrCurrentInstr() > markerPos
		&& !(ent.currentHitNum() != 0 && ent.hitboxCount(HITBOXTYPE_HITBOX) == 0)
		|| strcmp(ent.gotoLabelRequest(), "bomb") == 0;
}

bool isDangerous_kFish(Entity ent) {
	return !(ent.currentHitNum() == 2 && ent.hitboxCount(HITBOXTYPE_HITBOX) == 0);
}

bool isDangerous_laserFish(Entity ent) {
	BYTE* markerPos = moves.findCreateObj(ent.bbscrCurrentFunc(), "Laser");
	if (!markerPos) {
		return false;
	}
	return ent.bbscrCurrentInstr() < markerPos;
}

bool isDangerous_card(Entity ent) {
	return ent.mem50() && !ent.hasActiveFlag()
		? false
		: !ent.isRecoveryState();
}

bool isDangerous_kum5D(Entity ent) {
	BYTE* markerPos1 = moves.findSpriteNonNull(ent.bbscrCurrentFunc());
	if (!markerPos1) return false;
	BYTE* markerPos2 = moves.findSpriteNull(markerPos1);
	if (!markerPos2) return false;
	return ent.hasActiveFlag() && ent.bbscrCurrentInstr() <= markerPos2;
}

bool isDangerous_rsfMeishi(Entity ent) {
	BYTE* markerPos = moves.findSetMarker(ent.bbscrCurrentFunc(), "End");
	if (!markerPos) return false;
	markerPos = moves.findSpriteNull(markerPos);
	if (!markerPos) return false;
	return ent.bbscrCurrentInstr() <= markerPos;
}

bool isDangerous_displayModel(Entity ent) {
	return ent.displayModel();
}
