#include "pch.h"
#include "Moves.h"
#include "PlayerInfo.h"
#include "memoryFunctions.h"
#include "EntityList.h"
#include <vector>
#include "EndScene.h"  // that's right, I'm including what I probably shouldn't and bypassing having to provide dependencies in function arguments
// this whole, entire file is hardcode
#include "SpecificFramebarIds.h"

Moves moves;

static const CharacterType GENERAL = (CharacterType)-1;
static std::vector<MoveInfoProperty> allProperties;
bool charDoesNotCareAboutSuperJumpInstalls[25] { false };

static bool sectionSeparator_enableWhiffCancels(PlayerInfo& ent);
static bool sectionSeparator_mistFinerAirDash(PlayerInfo& ent);
static bool sectionSeparator_mistFinerDash(PlayerInfo& ent);
static bool sectionSeparator_alwaysTrue(PlayerInfo& ent);
static bool sectionSeparator_blitzShield(PlayerInfo& ent);
static bool sectionSeparator_may6P(PlayerInfo& ent);
static bool sectionSeparator_may6H(PlayerInfo& ent);
static bool sectionSeparator_breakTheLaw(PlayerInfo& ent);
static bool sectionSeparator_FDB(PlayerInfo& ent);
static bool sectionSeparator_soutenBC(PlayerInfo& ent);
static bool sectionSeparator_QV(PlayerInfo& ent);
static bool sectionSeparator_stingerS(PlayerInfo& ent);
static bool sectionSeparator_stingerH(PlayerInfo& ent);
static bool sectionSeparator_sultryPerformance(PlayerInfo& ent);
static bool sectionSeparator_beakDriver(PlayerInfo& ent);
static bool sectionSeparator_rifle(PlayerInfo& ent);
static bool sectionSeparator_leoGuardStance(PlayerInfo& ent);
static bool sectionSeparator_treasureHunt(PlayerInfo& ent);
static bool sectionSeparator_falconDive(PlayerInfo& ent);
static bool sectionSeparator_fourTigersSwordRev(PlayerInfo& ent);
static bool sectionSeparator_blackHoleAttack(PlayerInfo& ent);
static bool sectionSeparator_armorDance(PlayerInfo& ent);
static bool sectionSeparator_kinomiNecro(PlayerInfo& ent);
static bool sectionSeparator_azami(PlayerInfo& ent);
static bool sectionSeparator_organ(PlayerInfo& ent);
static bool sectionSeparator_dizzy6H(PlayerInfo& ent);
static bool sectionSeparator_saishingeki(PlayerInfo& ent);

static bool sectionSeparatorProjectile_dolphin(Entity ent);

static bool isIdle_enableWhiffCancels(PlayerInfo& player);
static bool isIdle_sparrowhawkStance(PlayerInfo& player);
static bool isIdle_Souten8(PlayerInfo& player);
static bool isIdle_Rifle(PlayerInfo& player);
static bool alwaysTrue(PlayerInfo& player);
static bool alwaysFalse(PlayerInfo& player);
static bool canBlock_neoHochihu(PlayerInfo& player);
static bool canBlock_azami(PlayerInfo& player);

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
static bool isDangerous_grenade(Entity ent);
static bool isDangerous_djavu(Entity ent);
static bool isDangerous_Djavu_D_Ghost(Entity ent);
static bool isDangerous_launchGreatsword(Entity ent);
static bool isDangerous_ramSwordMove(Entity ent);
static bool isDangerous_hasHitboxes(Entity ent);
static bool isDangerous_bubble(Entity ent);
static bool isDangerous_laserFish(Entity ent);
static bool isDangerous_pFish(Entity ent);
static bool isDangerous_kFish(Entity ent);
static bool isDangerous_dFish(Entity ent);
static bool isDangerous_card(Entity ent);
static bool isDangerous_kum5D(Entity ent);
static bool isDangerous_rsfMeishi(Entity ent);
static bool isDangerous_displayModel(Entity ent);
static bool isDangerous_vacuumAtk(Entity ent);
static bool isDangerous_mistKuttsuku(Entity ent);

static const char* nameSelector_iceSpike(Entity ent);
static const char* slangNameSelector_iceSpike(Entity ent);
static const char* nameSelector_iceScythe(Entity ent);
static const char* slangNameSelector_iceScythe(Entity ent);
static const char* framebarNameSelector_djvuD(Entity ent);
static const char* framebarSlangNameSelector_djvuD(Entity ent);
static const char* framebarNameSelector_closeShot(Entity ent);
static const char* framebarNameSelector_gunflameProjectile(Entity ent);
static const char* framebarSlangNameSelector_gunflameProjectile(Entity ent);
static const char* framebarNameSelector_venomBall(Entity ent);
static const char* framebarSlangNameSelector_venomBall(Entity ent);

static bool isInVariableStartupSection_treasureHunt(PlayerInfo& ent);
static bool isInVariableStartupSection_zweiLand(PlayerInfo& ent);
static bool isInVariableStartupSection_blitzShield(PlayerInfo& ent);
static bool isInVariableStartupSection_may6Por6H(PlayerInfo& ent);
static bool isInVariableStartupSection_soutenBC(PlayerInfo& ent);
static bool isInVariableStartupSection_amiMove(PlayerInfo& ent);
static bool isInVariableStartupSection_beakDriver(PlayerInfo& ent);
static bool isInVariableStartupSection_organOpen(PlayerInfo& ent);
static bool isInVariableStartupSection_breakTheLaw(PlayerInfo& ent);
static bool isInVariableStartupSection_fdb(PlayerInfo& ent);
static bool isInVariableStartupSection_qv(PlayerInfo& ent);
static bool isInVariableStartupSection_stinger(PlayerInfo& ent);
static bool isInVariableStartupSection_inoDivekick(PlayerInfo& ent);
static bool isInVariableStartupSection_sinRTL(PlayerInfo& ent);
static bool isInVariableStartupSection_falconDive(PlayerInfo& ent);
static bool isInVariableStartupSection_blackHoleAttack(PlayerInfo& ent);
static bool isInVariableStartupSection_dizzy6H(PlayerInfo& ent);
static bool isInVariableStartupSection_kinomiNecro(PlayerInfo& ent);
static bool isInVariableStartupSection_saishingeki(PlayerInfo& ent);

static bool isRecoveryHasGatlings_mayRideTheDolphin(PlayerInfo& ent);
static bool isRecoveryCanAct_mayRideTheDolphin(PlayerInfo& ent);
static bool isRecoveryHasGatlings_enableWhiffCancels(PlayerInfo& ent);
static bool hasWhiffCancels(PlayerInfo& ent);
static bool hasWhiffCancelsAndCantBlock(PlayerInfo& ent);
static bool isRecoveryHasGatlings_beakDriver(PlayerInfo& ent);
static bool isRecoveryCanAct_beakDriver(PlayerInfo& ent);

static bool aSectionBeforeVariableStartup_leoParry(PlayerInfo& ent);

static bool canStopHolding_armorDance(PlayerInfo& ent);

static bool frontLegInvul_potemkinBuster(PlayerInfo& ent);

static bool isRecoveryCanReload_rifle(PlayerInfo& ent);

static DWORD zatoHoldLevel_breakTheLaw(PlayerInfo& ent);

static bool conditionForAddingWhiffCancels_blockingBaiken(PlayerInfo& ent);

static bool secondaryStartup_saishingeki(PlayerInfo& ent);

static bool isRecovery_daisenpu(PlayerInfo& ent);
static bool isRecovery_RTL(PlayerInfo& ent);
static bool isRecovery_sinRTL(PlayerInfo& ent);
static bool isRecovery_recovery(PlayerInfo& ent);
static bool isRecovery_zanseiRouga(PlayerInfo& ent);
static bool isRecovery_land(PlayerInfo& ent);
static bool isRecovery_byakueRenshou(PlayerInfo& ent);

static bool forceSuperHitAnyway_zanseiRouga(PlayerInfo& ent);
static bool forceSuperHitAnyway_hououshou(PlayerInfo& ent);

static const char* displayNameSelector_pogoEntry(PlayerInfo& ent);
static const char* displaySlangNameSelector_pogoEntry(PlayerInfo& ent);
static const char* displayNameSelector_pogoA(PlayerInfo& ent);
static const char* displaySlangNameSelector_pogoA(PlayerInfo& ent);
static const char* displayNameSelector_pogo9(PlayerInfo& ent);
static const char* displaySlangNameSelector_pogo9(PlayerInfo& ent);
static const char* displayNameSelector_pogo44(PlayerInfo& ent);
static const char* displaySlangNameSelector_pogo44(PlayerInfo& ent);
static const char* displayNameSelector_pogo66(PlayerInfo& ent);
static const char* displaySlangNameSelector_pogo66(PlayerInfo& ent);
static const char* displayNameSelector_pogoB(PlayerInfo& ent);
static const char* displaySlangNameSelector_pogoB(PlayerInfo& ent);
static const char* displayNameSelector_pogoC(PlayerInfo& ent);
static const char* displaySlangNameSelector_pogoC(PlayerInfo& ent);
static const char* displayNameSelector_pogoD(PlayerInfo& ent);
static const char* displaySlangNameSelector_pogoD(PlayerInfo& ent);
static const char* displayNameSelector_pogoE(PlayerInfo& ent);
static const char* displaySlangNameSelector_pogoE(PlayerInfo& ent);
static const char* displayNameSelector_pogo8(PlayerInfo& ent);
static const char* displaySlangNameSelector_pogo8(PlayerInfo& ent);
static const char* displayNameSelector_RC(PlayerInfo& ent);
static const char* displaySlangNameSelector_RC(PlayerInfo& ent);
static const char* displayNameSelector_may6P(PlayerInfo& ent);
static const char* displayNameSelector_may6H(PlayerInfo& ent);
static const char* displayNameSelector_badMoon(PlayerInfo& ent);
static const char* displaySlangNameSelector_badMoon(PlayerInfo& ent);
static const char* displayNameSelector_carcassRaidS(PlayerInfo& ent);
static const char* displaySlangNameSelector_carcassRaidS(PlayerInfo& ent);
static const char* displayNameSelector_carcassRaidH(PlayerInfo& ent);
static const char* displaySlangNameSelector_carcassRaidH(PlayerInfo& ent);
static const char* displayNameSelector_stingerS(PlayerInfo& ent);
static const char* displaySlangNameSelector_stingerS(PlayerInfo& ent);
static const char* displayNameSelector_stingerH(PlayerInfo& ent);
static const char* displaySlangNameSelector_stingerH(PlayerInfo& ent);
static const char* displayNameSelector_taskCAir(PlayerInfo& ent);
static const char* displaySlangNameSelector_taskCAir(PlayerInfo& ent);
static const char* framebarNameSelector_blueBurst(Entity ent);
static const char* displayNameSelector_blueBurst(PlayerInfo& ent);
static const char* displayNameSelector_rifleStart(PlayerInfo& ent);
static const char* displaySlangNameSelector_rifleStart(PlayerInfo& ent);
static const char* displayNameSelector_rifleReload(PlayerInfo& ent);
static const char* displaySlangNameSelector_rifleReload(PlayerInfo& ent);
static const char* displayNameSelector_riflePerfectReload(PlayerInfo& ent);
static const char* displaySlangNameSelector_riflePerfectReload(PlayerInfo& ent);
static const char* displayNameSelector_rifleRC(PlayerInfo& ent);
static const char* displaySlangNameSelector_rifleRC(PlayerInfo& ent);
static const char* displayNameSelector_mistEntry(PlayerInfo& ent);
static const char* displaySlangNameSelector_mistEntry(PlayerInfo& ent);
static const char* displayNameSelector_mistLoop(PlayerInfo& ent);
static const char* displaySlangNameSelector_mistLoop(PlayerInfo& ent);
static const char* displayNameSelector_mistWalkForward(PlayerInfo& ent);
static const char* displaySlangNameSelector_mistWalkForward(PlayerInfo& ent);
static const char* displayNameSelector_mistWalkBackward(PlayerInfo& ent);
static const char* displaySlangNameSelector_mistWalkBackward(PlayerInfo& ent);
static const char* displayNameSelector_mistDash(PlayerInfo& ent);
static const char* displaySlangNameSelector_mistDash(PlayerInfo& ent);
static const char* displayNameSelector_mistBackdash(PlayerInfo& ent);
static const char* displaySlangNameSelector_mistBackdash(PlayerInfo& ent);
static const char* displayNameSelector_airMistEntry(PlayerInfo& ent);
static const char* displaySlangNameSelector_airMistEntry(PlayerInfo& ent);
static const char* displayNameSelector_airMistLoop(PlayerInfo& ent);
static const char* displaySlangNameSelector_airMistLoop(PlayerInfo& ent);
static const char* displayNameSelector_airMistDash(PlayerInfo& ent);
static const char* displaySlangNameSelector_airMistDash(PlayerInfo& ent);
static const char* displayNameSelector_airMistBackdash(PlayerInfo& ent);
static const char* displaySlangNameSelector_airMistBackdash(PlayerInfo& ent);
static const char* displayNameSelector_gekirinLv2or3(PlayerInfo& ent);
static const char* displayNameSelector_airGekirinLv2or3(PlayerInfo& ent);
static const char* displayNameSelector_ryujinLv2or3(PlayerInfo& ent);
static const char* displayNameSelector_airRyujinLv2or3(PlayerInfo& ent);
static const char* displayNameSelector_kenroukakuLv2or3(PlayerInfo& ent);
static const char* displayNameSelector_airKenroukakuLv2or3(PlayerInfo& ent);
static const char* displayNameSelector_standingAzami(PlayerInfo& ent);
static const char* displayNameSelector_crouchingAzami(PlayerInfo& ent);
static const char* displayNameSelector_airAzami(PlayerInfo& ent);
static const char* displayNameSelector_gunflame(PlayerInfo& ent);
static const char* displaySlangNameSelector_gunflame(PlayerInfo& ent);
static const char* displayNameSelector_gunflameDI(PlayerInfo& ent);
static const char* displaySlangNameSelector_gunflameDI(PlayerInfo& ent);
static const char* displayNameSelector_standingBlitzShield(PlayerInfo& ent);
static const char* displaySlangNameSelector_standingBlitzShield(PlayerInfo& ent);
static const char* displayNameSelector_crouchingBlitzShield(PlayerInfo& ent);
static const char* displaySlangNameSelector_crouchingBlitzShield(PlayerInfo& ent);
static const char* displayNameSelector_pilebunker(PlayerInfo& ent);
static const char* displaySlangNameSelector_pilebunker(PlayerInfo& ent);
static const char* displayNameSelector_crosswise(PlayerInfo& ent);
static const char* displaySlangNameSelector_crosswise(PlayerInfo& ent);
static const char* displayNameSelector_underPressure(PlayerInfo& ent);
static const char* displaySlangNameSelector_underPressure(PlayerInfo& ent);
static const char* displayNameSelector_jacko4D(PlayerInfo& ent);
static const char* displayNameSelector_jackoj4D(PlayerInfo& ent);

static bool canYrcProjectile_default(PlayerInfo& ent);
static bool canYrcProjectile_prevNoLinkDestroyOnStateChange(PlayerInfo& ent);
static bool createdProjectile_ky5D(PlayerInfo& ent);
static bool canYrcProjectile_ky5D(PlayerInfo& ent);
static bool createdProjectile_splitCiel(PlayerInfo& ent);
static bool canYrcProjectile_splitCiel(PlayerInfo& ent);
static bool canYrcProjectile_flower(PlayerInfo& ent);
static bool canYrcProjectile_qv(PlayerInfo& ent);
static bool createdProjectile_bishop(PlayerInfo& ent);
static bool canYrcProjectile_bishop(PlayerInfo& ent);
static bool createdProjectile_ino5D(PlayerInfo& ent);
static bool canYrcProjectile_ino5D(PlayerInfo& ent);
static bool createdProjectile_onf5(PlayerInfo& ent);
static bool canYrcProjectile_onf5(PlayerInfo& ent);
static bool createdProjectile_onf7(PlayerInfo& ent);
static bool canYrcProjectile_onf7(PlayerInfo& ent);
static bool canYrcProjectile_onf9(PlayerInfo& ent);
static bool createdProjectile_elpheltjD(PlayerInfo& ent);
static bool canYrcProjectile_elpheltjD(PlayerInfo& ent);
static bool createdProjectile_PGhost(PlayerInfo& ent);
static bool canYrcProjectile_PGhost(PlayerInfo& ent);
static bool createdProjectile_KGhost(PlayerInfo& ent);
static bool canYrcProjectile_KGhost(PlayerInfo& ent);
static bool createdProjectile_SGhost(PlayerInfo& ent);
static bool canYrcProjectile_SGhost(PlayerInfo& ent);
static bool createdProjectile_ThrowGhost(PlayerInfo& ent);
static bool canYrcProjectile_ThrowGhost(PlayerInfo& ent);
static bool createdProjectile_AirThrowGhost(PlayerInfo& ent);
static bool canYrcProjectile_AirThrowGhost(PlayerInfo& ent);
static bool createdProjectile_kum5D(PlayerInfo& ent);
static bool canYrcProjectile_kum5D(PlayerInfo& ent);
static bool createdProjectile_baiken5D(PlayerInfo& ent);
static bool canYrcProjectile_baiken5D(PlayerInfo& ent);
static bool canYrcProjectile_scroll(PlayerInfo& ent);
static bool createdProjectile_firesale(PlayerInfo& ent);
static bool canYrcProjectile_firesale(PlayerInfo& ent);

static bool powerup_may6P(PlayerInfo& ent);
static bool powerup_may6H(PlayerInfo& ent);
static const char* powerupExplanation_may6P(PlayerInfo& ent);
static const char* powerupExplanation_may6H(PlayerInfo& ent);
static bool powerup_qv(PlayerInfo& ent);
static const char* powerupExplanation_qvA(PlayerInfo& ent);
static const char* powerupExplanation_qvB(PlayerInfo& ent);
static const char* powerupExplanation_qvC(PlayerInfo& ent);
static const char* powerupExplanation_qvD(PlayerInfo& ent);
static bool powerup_kyougenA(PlayerInfo& ent);
static bool powerup_kyougenB(PlayerInfo& ent);
static bool powerup_kyougenC(PlayerInfo& ent);
static bool powerup_kyougenD(PlayerInfo& ent);
static const char* powerupExplanation_kyougenA(PlayerInfo& ent);
static const char* powerupExplanation_kyougenB(PlayerInfo& ent);
static const char* powerupExplanation_kyougenC(PlayerInfo& ent);
static const char* powerupExplanation_kyougenD(PlayerInfo& ent);
static bool powerup_onpu(ProjectileInfo& projectile);
static bool powerup_djavu(PlayerInfo& ent);
static const char* powerupExplanation_djavu(PlayerInfo& ent);
static bool powerup_stingerS(PlayerInfo& ent);
static bool powerup_stingerH(PlayerInfo& ent);
static const char* powerupExplanation_stinger(PlayerInfo& ent);
static bool powerup_closeShot(ProjectileInfo& ent);
static bool powerup_rifle(PlayerInfo& ent);
static const char* powerupExplanation_rifle(PlayerInfo& ent);
static bool powerup_beakDriver(PlayerInfo& ent);
static const char* powerupExplanation_beakDriver(PlayerInfo& ent);
static bool dontShowPowerupGraphic_beakDriver(PlayerInfo& ent);
static bool powerup_mistFiner(PlayerInfo& ent);
static const char* powerupExplanation_mistFiner(PlayerInfo& ent);
static bool powerup_eatMeat(PlayerInfo& ent);
static const char* powerupExplanation_eatMeat(PlayerInfo& ent);
static bool powerup_cardK(PlayerInfo& ent);
static bool powerup_cardS(PlayerInfo& ent);
static bool powerup_cardH(PlayerInfo& ent);
static const char* powerupExplanation_card(PlayerInfo& ent);
static bool powerup_hayabusaRev(PlayerInfo& ent);
static const char* powerupExplanation_hayabusaRev(PlayerInfo& ent);
static bool powerup_hayabusaHeld(PlayerInfo& ent);
static const char* powerupExplanation_hayabusaHeld(PlayerInfo& ent);
static bool powerup_grampaMax(PlayerInfo& ent);
static const char* powerupExplanation_grampaMax(PlayerInfo& ent);
static bool powerup_armorDance(PlayerInfo& ent);
static const char* powerupExplanation_armorDance(PlayerInfo& ent);
static bool powerup_fireSpear(PlayerInfo& ent);
static const char* powerupExplanation_fireSpear(PlayerInfo& ent);
static bool powerup_zweit(PlayerInfo& ent);
static const char* powerupExplanation_zweit(PlayerInfo& ent);
static bool powerup_kuuhuku(PlayerInfo& ent);
static const char* powerupExplanation_kuuhuku(PlayerInfo& ent);
static bool powerup_secretGarden(PlayerInfo& ent);
static const char* powerupExplanation_secretGarden(PlayerInfo& ent);

static void fillMay6HOffsets(BYTE* func);

static MoveInfoProperty& newProperty(MoveInfoStored* move, DWORD property) {
	if (!move->count) move->startInd = allProperties.size();
	++move->count;
	allProperties.emplace_back();
	MoveInfoProperty& prop = allProperties.back();
	prop.type = property;
	return prop;
}


void Moves::addMove(const MoveInfo& move) {
	#ifdef _DEBUG
	MyKey newKey{ move.charType, move.name, move.isEffect };
	auto found = map.find(newKey);
	if (found != map.end()) {
		MyCompareFunction func;
		bool alreadyContains = false;
		for (MyKey& key : repeatingMoves) {
			if (func(key, newKey)) {
				alreadyContains = true;
				break;
			}
		}
		if (!alreadyContains) {
			repeatingMoves.push_back(newKey);
		}
		return;
	}
	#endif
	
	MoveInfoStored newMove;
	#define MOVE_INFO_EXEC(type, prop, name, defaultValue) if (move.name != defaultValue) newProperty(&newMove, offsetof(MoveInfo, name)).u.prop = move.name;
	MOVE_INFO_PROPERTY_TABLE
	#undef MOVE_INFO_EXEC
	
	if (!move.canBlock && move.isIdle) newProperty(&newMove, offsetof(MoveInfo, canBlock)).u.isIdleValue = move.isIdle;
	
	map.insert( {
		#ifdef _DEBUG
		newKey
		#else
		{ move.charType, move.name, move.isEffect }
		#endif
		,
		newMove
	} );
}

MoveInfo::MoveInfo(const MoveInfoStored& info) {
	const MoveInfoProperty* prop = info.startPtr;
	for (int i = 0; i < info.count; ++i) {
		switch (prop->type) {
			#define MOVE_INFO_EXEC(type, propName, name, defaultValue) case offsetof(MoveInfo, name): name = prop->u.propName; break;
			MOVE_INFO_PROPERTY_TABLE
			#undef MOVE_INFO_EXEC
		}
		++prop;
	}
	if (!canBlock && isIdle) canBlock = isIdle;
	if (!isIdle) isIdle = isIdle_default;
	if (!canBlock) canBlock = canBlock_default;
}

void MoveInfo::addForceAddWhiffCancel(const char* name) {
	if (!forceAddWhiffCancelsCount) {
		forceAddWhiffCancelsStart = moves.forceAddWhiffCancels.size();
		moves.forceAddWhiffCancels.emplace_back();
		moves.forceAddWhiffCancels.back().name = name;
		forceAddWhiffCancelsCount = 1;
	} else {
		moves.forceAddWhiffCancels.emplace_back();
		moves.forceAddWhiffCancels.back().name = name;
		++forceAddWhiffCancelsCount;
	}
}

ForceAddedWhiffCancel* MoveInfo::getForceAddWhiffCancel(int index) const {
	return &moves.forceAddWhiffCancels[forceAddWhiffCancelsStart + index];
}

bool Moves::onDllMain() {
	defaultMove.isIdle = isIdle_default;
	MoveInfo move;
	
	move = MoveInfo(GENERAL, "CmnStandForce");
	move.displayName = "Stand";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnNeutral");
	move.displayName = "Neutral";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "RomanCancelHit");
	move.displayName = "Roman Cancel";
	move.displayNameSelector = displayNameSelector_RC;
	move.slangName = "RC";
	move.displaySlangNameSelector = displaySlangNameSelector_RC;
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "cmn_BurstObjGoldHontai", true);
	move.framebarName = "Gold Burst";
	move.framebarId = 111;
	addMove(move);
	
	move = MoveInfo(GENERAL, "cmn_BurstObjBlueHontai", true);
	move.framebarName = "Blue Burst";
	move.framebarNameSelector = framebarNameSelector_blueBurst;
	move.framebarId = 111;
	addMove(move);
	
	// This was spotted when throwing Blue Burst on the very frame it comes out
	move = MoveInfo(GENERAL, "cmn_BurstObjBlueObject", true);
	move.framebarName = "Blue Burst";
	move.framebarNameSelector = framebarNameSelector_blueBurst;
	move.framebarId = 111;
	addMove(move);
	
	// This was spotted when throwing Gold Burst on the very frame it comes out
	move = MoveInfo(GENERAL, "cmn_BurstObjGoldObject", true);
	move.framebarName = "Gold Burst";
	move.framebarId = 111;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnDamageBurst");
	move.displayName = "Blue Burst";
	move.displayNameSelector = displayNameSelector_blueBurst;
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnMaximumBurst");
	move.displayName = "Gold Burst";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "FaultlessDefenceCrouch");
	move.displayName = "Crouching Faultless Defense";
	move.slangName = "Crouching FD";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "FaultlessDefenceAir");
	move.displayName = "Air Faultless Defense";
	move.slangName = "Air FD";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "FaultlessDefenceStand");
	move.displayName = "Standing Faultless Defense";
	move.slangName = "Standing FD";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "DeadAngleAttack");
	move.displayName = "Dead Angle Attack";
	move.slangName = "DAA";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "IchigekiJunbi");
	move.displayName = "Instant Kill Activation";
	move.slangName = "IK Activation";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "Ichigeki");
	move.displayName = "Instant Kill";
	move.slangName = "IK";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActStand");
	move.displayName = "Stand";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActStandTurn");
	move.displayName = "Stand Turn";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActStand2Crouch");
	move.displayName = "Stand to Crouch";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActCrouch");
	move.displayName = "Crouch";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActCrouchTurn");
	move.displayName = "Crouch Turn";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActCrouch2Stand");
	move.displayName = "Crouch to Stand";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActJumpPre");
	move.displayName = "Prejump";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActJump");
	move.displayName = "Jump";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActJumpLanding");
	move.displayName = "Landing";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActLandingStiff");
	move.displayName = "Landing Recovery";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActFWalk");
	move.displayName = "Walk Forward";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnFWalk");
	move.displayName = "Walk Forward";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActBWalk");
	move.displayName = "Walk Back";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnBWalk");
	move.displayName = "Walk Back";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActFDash");
	move.displayName = "Forward Dash";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);

	move = MoveInfo(GENERAL, "CmnFDash");
	move.displayName = "Forward Dash";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActFDashStop");
	move.displayName = "Forward Dash Stop";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActBDash");
	move.displayName = "Backdash";
	move.slangName = "BD";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnBDash");
	move.displayName = "Backdash";
	move.slangName = "BD";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActAirFDash");
	move.displayName = "Airdash Forward";
	move.slangName = "ADF";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnFAirDash");
	move.displayName = "Airdash Forward";
	move.slangName = "ADF";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActAirBDash");
	move.displayName = "Airdash Back";
	move.slangName = "ADB";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnBAirDash");
	move.displayName = "Airdash Back";
	move.slangName = "ADB";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "HomingDash2");
	move.displayName = "Homing Dash";
	move.ignoreJumpInstalls = true;
	addMove(move);

	move = MoveInfo(GENERAL, "HomingJump");
	move.displayName = "Homing Jump";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActNokezoriHighLv1");
	move.displayName = "Hitstun High Lv1";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActNokezoriHighLv2");
	move.displayName = "Hitstun High Lv2";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActNokezoriHighLv3");
	move.displayName = "Hitstun High Lv3";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActNokezoriHighLv4");
	move.displayName = "Hitstun High Lv4";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActNokezoriHighLv5");
	move.displayName = "Hitstun High Lv5";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActNokezoriLowLv1");
	move.displayName = "Hitstun Low Lv1";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActNokezoriLowLv2");
	move.displayName = "Hitstun Low Lv2";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActNokezoriLowLv3");
	move.displayName = "Hitstun Low Lv3";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActNokezoriLowLv4");
	move.displayName = "Hitstun Low Lv4";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActNokezoriLowLv5");
	move.displayName = "Hitstun Low Lv5";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActNokezoriCrouchLv1");
	move.displayName = "Hitstun Crouch Lv1";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActNokezoriCrouchLv2");
	move.displayName = "Hitstun Crouch Lv2";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActNokezoriCrouchLv3");
	move.displayName = "Hitstun Crouch Lv3";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActNokezoriCrouchLv4");
	move.displayName = "Hitstun Crouch Lv4";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActNokezoriCrouchLv5");
	move.displayName = "Hitstun Crouch Lv5";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActBDownUpper");
	move.displayName = "Launched Into Air Back";
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActBDownUpperEnd");
	move.displayName = "Starting To Fall Back";
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActBDownDown");
	move.displayName = "Falling Down Back";
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActBDownBound");
	move.displayName = "Fell Down On The Back";
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActBDownLoop");
	move.displayName = "Lying On The Back";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActBDown2Stand");
	move.displayName = "Waking Up On The Back";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActFDownUpper");
	move.displayName = "Launched Into Air Forward";
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActFDownUpperEnd");
	move.displayName = "Starting To Fall Forward";
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActFDownDown");
	move.displayName = "Falling Down Forward";
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActFDownBound");
	move.displayName = "Fell Down Forward";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActFDownLoop");
	move.displayName = "Lying On The Face";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActFDown2Stand");
	move.displayName = "Waking Up Face Down";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActVDownUpper");
	move.displayName = "Launched Into Air Vertically";
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActVDownUpperEnd");
	move.displayName = "Starting To Fall Vertically";
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActVDownDown");
	move.displayName = "Falling Down Vertical";
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActVDownBound");
	move.displayName = "Fell Down Vertical";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActVDownLoop");
	move.displayName = "Lying On The Face (From Vertical Fall)";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActBlowoff");
	move.displayName = "Blown Off";
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActKirimomiUpper");
	move.displayName = "Launched";
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActWallBound");
	move.displayName = "Wallbounce";
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActWallBoundDown");
	move.displayName = "Falling Down From Wallbounce";
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActWallHaritsuki");
	move.displayName = "Wallsplat/Wallslump";
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActWallHaritsukiLand");
	move.displayName = "Landed From Wallslump";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActWallHaritsukiGetUp");
	move.displayName = "Waking Up From Wallslump";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActJitabataLoop");
	move.displayName = "Stagger";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActKizetsu");
	move.displayName = "Faint";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActHizakuzure");
	move.displayName = "Crumple";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActKorogari");
	move.displayName = "Tumble";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActZSpin");
	move.displayName = "Spinning";
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActUkemi");
	move.displayName = "Air Recovery";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActMidGuardPre");
	move.displayName = "Pre Mid Block";
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActMidGuardLoop");
	move.displayName = "Mid Block";
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActMidGuardEnd");
	move.displayName = "Mid Block End";
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActHighGuardPre");
	move.displayName = "Pre High Block";
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActHighGuardLoop");
	move.displayName = "High Block";
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActHighGuardEnd");
	move.displayName = "High Block End";
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActCrouchGuardPre");
	move.displayName = "Pre Crouching Block";
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActCrouchGuardLoop");
	move.displayName = "Crouching Block";
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActCrouchGuardEnd");
	move.displayName = "Crouching Block End";
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActAirGuardPre");
	move.displayName = "Pre Air Block";
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActAirGuardLoop");
	move.displayName = "Air Block";
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActAirGuardEnd");
	move.displayName = "Air Block End";
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActHajikareStand");
	move.displayName = "Stand Rejected";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActHajikareCrouch");
	move.displayName = "Crouch Rejected";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActHajikareAir");
	move.displayName = "Air Rejected";
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActAirTurn");
	move.displayName = "Air Turn";
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActLockWait");
	move.displayName = "Grabbed";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActLockReject");
	move.displayName = "CmnActLockReject";
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActAirLockWait");
	move.displayName = "CmnActAirLockWait";
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActAirLockReject");
	move.displayName = "CmnActAirLockReject";
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActItemUse");
	move.displayName = "Item";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActBurst");
	move.displayName = "Burst";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActRomanCancel");
	move.displayName = "Roman Cancel";
	move.slangName = "RC";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActEntry");
	move.displayName = "CmnActEntry";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActRoundWin");
	move.displayName = "Round Win";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActMatchWin");
	move.displayName = "Match Win";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActLose");
	move.displayName = "CmnActLose";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActResultWin");
	move.displayName = "Victory Screen Win";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActResultLose");
	move.displayName = "Victory Screen Lose";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActEntryWait");
	move.displayName = "Invisible";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActExDamage");
	move.displayName = "Ex Damage";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActExDamageLand");
	move.displayName = "CmnActExDamageLand";
	move.ignoreJumpInstalls = true;
	addMove(move);

	move = MoveInfo(GENERAL, "NmlAtk5A");
	move.displayName = "5P";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk5B");
	move.displayName = "5K";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk5C");
	move.displayName = "5S";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk5CNear");
	move.displayName = "c.S";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk5CFar");
	move.displayName = "f.S";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk5D");
	move.displayName = "5H";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk5E");
	move.displayName = "5D";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk5F");
	move.displayName = "Taunt";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk2A");
	move.displayName = "2P";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk2B");
	move.displayName = "2K";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk2C");
	move.displayName = "2S";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk2D");
	move.displayName = "2H";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk2E");
	move.displayName = "2D";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk1A");
	move.displayName = "1P";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk1B");
	move.displayName = "1K";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk1C");
	move.displayName = "1S";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk1D");
	move.displayName = "1H";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk1E");
	move.displayName = "1D";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk3A");
	move.displayName = "3P";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk3B");
	move.displayName = "3K";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk3C");
	move.displayName = "3S";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk3D");
	move.displayName = "3H";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk3E");
	move.displayName = "3D";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk4A");
	move.displayName = "4P";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk4B");
	move.displayName = "4K";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk4C");
	move.displayName = "4S";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk4D");
	move.displayName = "4H";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk4E");
	move.displayName = "4D";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk6A");
	move.displayName = "6P";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk6B");
	move.displayName = "6K";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk6C");
	move.displayName = "6S";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk6D");
	move.displayName = "6H";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk6E");
	move.displayName = "6D";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk6F");
	move.displayName = "Respect";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtkAir5A");
	move.displayName = "j.P";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtkAir5B");
	move.displayName = "j.K";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtkAir5C");
	move.displayName = "j.S";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtkAir5D");
	move.displayName = "j.H";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtkAir5E");
	move.displayName = "j.D";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtkAir2A");
	move.displayName = "j.2P";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtkAir2B");
	move.displayName = "j.2K";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtkAir2C");
	move.displayName = "j.2S";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtkAir2D");
	move.displayName = "j.2H";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtkAir2E");
	move.displayName = "j.2D";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtkAir4A");
	move.displayName = "j.4P";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtkAir4B");
	move.displayName = "j.4K";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtkAir4C");
	move.displayName = "j.4S";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtkAir4D");
	move.displayName = "j.4H";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtkAir4E");
	move.displayName = "j.4D";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtkAir6A");
	move.displayName = "j.6P";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtkAir6B");
	move.displayName = "j.6K";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtkAir6C");
	move.displayName = "j.6S";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtkAir6D");
	move.displayName = "j.6H";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtkAir6E");
	move.displayName = "j.6D";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtkAir8E");
	move.displayName = "j.8D";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnVJump");
	move.displayName = "Jump Neutral";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnFJump");
	move.displayName = "Jump Forward";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnBJump");
	move.displayName = "Jump Back";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnVHighJump");
	move.displayName = "Superjump Neutral";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnFHighJump");
	move.displayName = "Superjump Forward";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnBHighJump");
	move.displayName = "Superjump Back";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnVAirJump");
	move.displayName = "Double Jump Neutral";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnFAirJump");
	move.displayName = "Double Jump Forward";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnBAirJump");
	move.displayName = "Double Jump Back";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "ThrowExe");
	move.displayName = "Ground Throw";
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "AirThrowExe");
	move.displayName = "Airthrow";
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CounterGuardStand");
	move.displayName = "Standing Blitz Shield";
	move.displayNameSelector = displayNameSelector_standingBlitzShield;
	move.slangName = "Standing Blitz";
	move.displaySlangNameSelector = displaySlangNameSelector_standingBlitzShield;
	move.sectionSeparator = sectionSeparator_blitzShield;
	move.isInVariableStartupSection = isInVariableStartupSection_blitzShield;
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CounterGuardCrouch");
	move.displayName = "Crouching Blitz Shield";
	move.displayNameSelector = displayNameSelector_crouchingBlitzShield;
	move.slangName = "Crouching Blitz";
	move.displaySlangNameSelector = displaySlangNameSelector_crouchingBlitzShield;
	move.sectionSeparator = sectionSeparator_blitzShield;
	move.isInVariableStartupSection = isInVariableStartupSection_blitzShield;
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CounterGuardAir");
	move.displayName = "Air Blitz Shield";
	move.slangName = "Air Blitz";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "BanditRevolverLand");
	move.displayName = "Bandit Revolver";
	move.slangName = "BR";
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "BanditRevolverLand_DI");
	move.displayName = "DI Bandit Revolver";
	move.slangName = "DI BR";
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "GunFlame");
	move.displayName = "Gunflame";
	move.displayNameSelector = displayNameSelector_gunflame;
	move.slangName = "GF";
	move.displaySlangNameSelector = displaySlangNameSelector_gunflame;
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "GunFlame_DI");
	move.displayName = "DI Gunflame";
	move.displayNameSelector = displayNameSelector_gunflameDI;
	move.slangName = "DI GF";
	move.displaySlangNameSelector = displaySlangNameSelector_gunflameDI;
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "BanditRevolverAir");
	move.displayName = "Air Bandit Revolver";
	move.slangName = "Air BR";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "BanditRevolverAir_DI");
	move.displayName = "DI Air Bandit Revolver";
	move.slangName = "DI Air BR";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "GroundViper");
	move.displayName = "Ground Viper";
	move.slangName = "GV";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "GroundViper_DI");
	move.displayName = "DI Ground Viper";
	move.slangName = "DI GV";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "RiotStamp");
	move.displayName = "Riot Stamp";
	move.slangName = "RS";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "RiotStamp_DI");
	move.displayName = "DI Riot Stamp";
	move.slangName = "DI RS";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "GunFlameFeint");
	move.displayName = "Gunflame Feint";
	move.slangName = "GF Feint";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "Kudakero");
	move.displayName = "Break";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "Kudakero_DI");
	move.displayName = "DI Break";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "VolcanicViperLandHS");
	move.displayName = "H Volcanic Viper";
	move.slangName = "hVV";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "VolcanicViperLandHS_DI");
	move.displayName = "DI H Volcanic Viper";
	move.slangName = "DI hVV";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "VolcanicViperLandS");
	move.displayName = "S Volcanic Viper";
	move.slangName = "sVV";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "VolcanicViperLandS_DI");
	move.displayName = "DI S Volcanic Viper";
	move.slangName = "DI sVV";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "AirCommandThrow");
	move.displayName = "P.B.B.";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "AirCommandThrowExe");
	move.displayName = "P.B.B.";
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "VolcanicViperAirHS");
	move.displayName = "Air H Volcanic Viper";
	move.slangName = "Air HVV";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "VolcanicViperAirHS_DI");
	move.displayName = "DI Air H Volcanic Viper";
	move.slangName = "DI Air HVV";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "VolcanicViperAirS");
	move.displayName = "Air S Volcanic Viper";
	move.slangName = "Air SVV";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "VolcanicViperAirS_DI");
	move.displayName = "DI Air S Volcanic Viper";
	move.slangName = "DI Air SVV";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "Fefnir");
	move.displayName = "Fafnir";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "Fefnir_DI");
	move.displayName = "DI Fafnir";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "BanditBringer");
	move.displayName = "Bandit Bringer";
	move.slangName = "BB";
	move.combineWithPreviousMove = true;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "BanditBringer_DI");
	move.displayName = "DI Bandit Bringer";
	move.slangName = "DI BB";
	move.combineWithPreviousMove = true;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "VolcanicViperKick");
	move.displayName = "Volcanic Viper Knockdown";
	move.slangName = "VV KD";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "VolcanicViperKick_DI");
	move.displayName = "DI Volcanic Viper Knockdown";
	move.slangName = "DI VV KD";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "TyrantRave");
	move.displayName = "Tyrant Rave";
	move.slangName = "TR";
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "TyrantRave_DI");
	move.displayName = "DI Tyrant Rave";
	move.slangName = "DI TR";
	move.iKnowExactlyWhenTheRecoveryOfThisMoveIs = isRecovery_recovery;
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "TyrantRaveBurst");
	move.displayName = "Burst Tyrant Rave";
	move.slangName = "Burst TR";
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "DragonInstall");
	move.displayName = "Dragon Install";
	move.slangName = "DI";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "ExKizetsu");
	move.displayName = "DI Recovery";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "BukkiraExe");
	move.displayName = "Wild Throw";
	move.slangName = "WT";
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "BukkirabouNiNageru");
	move.displayName = "Wild Throw";
	move.slangName = "WT";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "GunFlameHibashira", true);
	move.isDangerous = isDangerous_gunflame;
	move.framebarId = 1;
	move.framebarName = "Gunflame";
	move.framebarNameSelector = framebarNameSelector_gunflameProjectile;
	move.framebarSlangName = "GF";
	move.framebarSlangNameSelector = framebarSlangNameSelector_gunflameProjectile;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "GunFlameHibashira_DI", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 1;
	move.framebarName = "Gunflame";
	move.framebarNameSelector = framebarNameSelector_gunflameProjectile;
	move.framebarSlangName = "GF";
	move.framebarSlangNameSelector = framebarSlangNameSelector_gunflameProjectile;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "TyrantRavePunch2", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 2;
	move.framebarName = "Tyrant Rave";
	move.framebarSlangName = "TR";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "TyrantRavePunch2_DI", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 2;
	move.framebarName = "Tyrant Rave";
	move.framebarSlangName = "TR";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "KudakeroEF", true);
	move.isDangerous = isDangerous_not_NullWhileActive;
	move.framebarId = 3;
	move.framebarName = "Break Explosion";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "KudakeroEF_DI", true);
	move.isDangerous = isDangerous_not_NullWhileActive;
	move.framebarId = 3;
	move.framebarName = "Break Explosion";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "RiotStamp_DI_Bomb", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 4;
	move.framebarName = "Riot Stamp";
	move.framebarSlangName = "RS";
	move.combineHitsFromDifferentProjectiles = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "GroundViperDash_DI", true);
	move.isDangerous = isDangerous_notNull;
	move.framebarId = 5;
	move.framebarName = "GV Fire Pillars";
	move.framebarNameFull = "Ground Viper Fire Pillars";
	move.combineHitsFromDifferentProjectiles = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "StunEdge2");
	move.displayName = "Charged Stun Edge";
	move.slangName = "CSE";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "StunEdge1");
	move.displayName = "Stun Edge";
	move.slangName = "SE";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "StunDipper");
	move.displayName = "Stun Dipper";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "HolyBrand");
	move.displayName = "Split Ciel";
	move.createdProjectile = createdProjectile_splitCiel;
	move.canYrcProjectile = canYrcProjectile_splitCiel;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "GreedSaber");
	move.displayName = "Greed Sever";
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "AirStunEdge2");
	move.displayName = "Air H Stun Edge";
	move.slangName = "Air H SE";
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "AirStunEdge1");
	move.displayName = "Air S Stun Edge";
	move.slangName = "Air S SE";
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "VaporThrustD");
	move.displayName = "H Vapor Thrust";
	move.slangName = "HVT";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "VaporThrustC");
	move.displayName = "S Vapor Thrust";
	move.slangName = "SVT";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "AirVaporThrustD");
	move.displayName = "Air H Vapor Thrust";
	move.slangName = "Air HVT";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "AirVaporThrust");
	move.displayName = "Air S Vapor Thrust";
	move.slangName = "Air SVT";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "SacredEdge");
	move.displayName = "Sacred Edge";
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "RideTheLightning");
	move.displayName = "Ride The Lightning";
	move.slangName = "RTL";
	move.iKnowExactlyWhenTheRecoveryOfThisMoveIs = isRecovery_RTL;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "RideTheLightningBurst");
	move.displayName = "Burst Ride The Lightning";
	move.slangName = "Burst RTL";
	move.iKnowExactlyWhenTheRecoveryOfThisMoveIs = isRecovery_RTL;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "AirRideTheLightning");
	move.displayName = "Air Ride The Lightning";
	move.slangName = "Air RTL";
	move.iKnowExactlyWhenTheRecoveryOfThisMoveIs = isRecovery_RTL;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "AirRideTheLightningBurst");
	move.displayName = "Air Burst Ride The Lightning";
	move.slangName = "Air Burst RTL";
	move.iKnowExactlyWhenTheRecoveryOfThisMoveIs = isRecovery_RTL;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "StunEdgeObj", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 6;
	move.framebarName = "Stun Edge";
	move.framebarSlangName = "SE";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "Mahojin", true);
	move.framebarName = "Durandal Call Grinder";  // can get displayed in the framebar due to clashing with an opponent's projectile
	move.framebarSlangName = "DC Grinder";
	move.framebarId = 114;
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "SPStunEdgeObj", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 6;
	move.framebarName = "DCSE";
	move.framebarNameFull = "Fortified Stun Edge (Durandal Call Stun Edge)";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "ChargedStunEdgeObj", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 7;
	move.framebarName = "CSE";
	move.framebarNameFull = "Charged Stun Edge";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "SPChargedStunEdgeObj", true);
	move.isDangerous = isDangerous_not_NullWhileActive;
	move.framebarId = 7;
	move.framebarName = "DCCSE";
	move.framebarNameFull = "Fortified Charged Stun Edge (Drandal Call Charged Stun Edge)";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "AirDustAttackObj", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 8;
	move.framebarName = "j.D";
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "NmlAtk5E");
	move.displayName = "5D";
	move.nameIncludesInputs = true;
	move.createdProjectile = createdProjectile_ky5D;
	move.canYrcProjectile = canYrcProjectile_ky5D;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "NmlAtkAir5E");
	move.displayName = "j.D";
	move.nameIncludesInputs = true;
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	// can't YRC in Rev1. In fact this doesn't even exist in Rev1
	move = MoveInfo(CHARACTER_TYPE_KY, "DustEffectShot", true);
	move.isDangerous = isDangerous_playerInRCOrHasActiveFlag_AndNotInRecovery;
	move.framebarId = 9;
	move.framebarName = "5D";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "SacredEdgeObj", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 10;
	move.framebarName = "Sacred Edge";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "SPSacredEdgeObj", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 10;
	move.framebarName = "Fortified Sacred Edge";
	move.framebarSlangName = "DC Sacred Edge";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "NmlAtk6B");
	move.displayName = "6K";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "GlitterIsGold");
	move.displayName = "Glitter Is Gold";
	move.slangName = "Coin";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "BucchusSigh");
	move.displayName = "Bacchus Sigh";
	move.slangName = "Bacchus";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "Mist", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarName = "Bacchus Sigh";
	move.framebarSlangName = PROJECTILE_NAME_BACCHUS;
	move.framebarId = 112;
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistKuttsuku", true);
	move.isDangerous = isDangerous_mistKuttsuku;
	move.framebarName = "Bacchus Sigh Debuff";
	move.framebarSlangName = "Bacchus";
	move.framebarId = 112;
	addMove(move);
	
	// the initial move of grounded Mist Finer, is 1f long
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistFinerA");
	move.displayName = "P Mist Finer Stance Entry";
	move.displayNameSelector = displayNameSelector_mistEntry;
	move.slangName = "PMF Entry";
	move.displaySlangNameSelector = displaySlangNameSelector_mistEntry;
	move.partOfStance = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistFinerB");
	move.displayName = "K Mist Finer Stance Entry";
	move.displayNameSelector = displayNameSelector_mistEntry;
	move.slangName = "KMF Entry";
	move.displaySlangNameSelector = displaySlangNameSelector_mistEntry;
	move.partOfStance = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistFinerC");
	move.displayName = "S Mist Finer Stance Entry";
	move.displayNameSelector = displayNameSelector_mistEntry;
	move.slangName = "SMF Entry";
	move.displaySlangNameSelector = displaySlangNameSelector_mistEntry;
	move.partOfStance = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	// is entered into from MistFinerA/B/C, has variable duration depending on Mist Finer level
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistFinerDehajime");
	move.displayName = "Mist Finer Entry";
	move.displayNameSelector = displayNameSelector_mistEntry;
	move.slangName = "MF Entry";
	move.displaySlangNameSelector = displaySlangNameSelector_mistEntry;
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	// entered into from MistFinerDehajime, enables whiff cancels on f2.
	// In Rev2 is exited out of instantly into another Mist Finer from things like MistFinerFWalk.
	// In Rev1 takes one frame to transition.
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistFinerLoop");
	move.displayName = "Mist Finer Stance";
	move.displayNameSelector = displayNameSelector_mistLoop;
	move.slangName = "MF Stance";
	move.displaySlangNameSelector = displaySlangNameSelector_mistLoop;
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.sectionSeparator = sectionSeparator_enableWhiffCancels;
	move.considerIdleInSeparatedSectionAfterThisManyFrames = 5;
	move.preservesNewSection = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	// performed when releasing the Mist Finer attack
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistFinerALv0");
	move.displayName = "Lv1 P Mist Finer";
	move.slangName = "Lv1 PMF";
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.powerup = powerup_mistFiner;
	move.powerupExplanation = powerupExplanation_mistFiner;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistFinerALv1");
	move.displayName = "Lv2 P Mist Finer";
	move.slangName = "Lv2 PMF";
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.powerup = powerup_mistFiner;
	move.powerupExplanation = powerupExplanation_mistFiner;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistFinerALv2");
	move.displayName = "Lv3 P Mist Finer";
	move.slangName = "Lv3 PMF";
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.powerup = powerup_mistFiner;
	move.powerupExplanation = powerupExplanation_mistFiner;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistFinerBLv0");
	move.displayName = "Lv1 K Mist Finer";
	move.slangName = "Lv1 KMF";
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.powerup = powerup_mistFiner;
	move.powerupExplanation = powerupExplanation_mistFiner;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistFinerBLv1");
	move.displayName = "Lv2 K Mist Finer";
	move.slangName = "Lv2 KMF";
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.powerup = powerup_mistFiner;
	move.powerupExplanation = powerupExplanation_mistFiner;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistFinerBLv2");
	move.displayName = "Lv3 K Mist Finer";
	move.slangName = "Lv3 KMF";
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.powerup = powerup_mistFiner;
	move.powerupExplanation = powerupExplanation_mistFiner;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistFinerCLv0");
	move.displayName = "Lv1 S Mist Finer";
	move.slangName = "Lv1 SMF";
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.powerup = powerup_mistFiner;
	move.powerupExplanation = powerupExplanation_mistFiner;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistFinerCLv1");
	move.displayName = "Lv2 S Mist Finer";
	move.slangName = "Lv2 SMF";
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.powerup = powerup_mistFiner;
	move.powerupExplanation = powerupExplanation_mistFiner;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistFinerCLv2");
	move.displayName = "Lv3 S Mist Finer";
	move.slangName = "Lv3 SMF";
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.powerup = powerup_mistFiner;
	move.powerupExplanation = powerupExplanation_mistFiner;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	// backdash during grounded Mist Finer
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistFinerBDash");
	move.displayName = "Mist Finer Backdash";
	move.slangName = "MF BD";
	move.partOfStance = true;
	move.nameIncludesInputs = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.sectionSeparator = sectionSeparator_mistFinerDash;
	move.considerNewSectionAsBeingInVariableStartup = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistFinerFDash");
	move.displayName = "Mist Finer Forward Dash";
	move.displayNameSelector = displayNameSelector_mistDash;
	move.slangName = "MF Dash";
	move.displaySlangNameSelector = displaySlangNameSelector_mistDash;
	move.partOfStance = true;
	move.nameIncludesInputs = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.sectionSeparator = sectionSeparator_mistFinerDash;
	move.considerNewSectionAsBeingInVariableStartup = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistFinerBWalk");
	move.displayName = "Mist Finer Walk Back";
	move.displayNameSelector = displayNameSelector_mistWalkBackward;
	move.slangName = "MF Walk Back";
	move.displaySlangNameSelector = displaySlangNameSelector_mistWalkBackward;
	move.partOfStance = true;
	move.nameIncludesInputs = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.sectionSeparator = sectionSeparator_alwaysTrue;
	move.considerIdleInSeparatedSectionAfterThisManyFrames = 5;
	move.preservesNewSection = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistFinerFWalk");
	move.displayName = "Mist Finer Walk Forward";
	move.displayNameSelector = displayNameSelector_mistWalkForward;
	move.slangName = "MF Walk Forward";
	move.displaySlangNameSelector = displaySlangNameSelector_mistWalkForward;
	move.partOfStance = true;
	move.nameIncludesInputs = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.sectionSeparator = sectionSeparator_alwaysTrue;
	move.considerIdleInSeparatedSectionAfterThisManyFrames = 5;
	move.preservesNewSection = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistFinerCancel");
	move.displayName = "Mist Finer Cancel";
	move.slangName = "MFC";
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	// the initial move of air Mist Finer, is 1f long
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "AirMistFinerA");
	move.displayName = "Air P Mist Finer Stance Entry";
	move.displayNameSelector = displayNameSelector_airMistEntry;
	move.slangName = "j.PMF Entry";
	move.displaySlangNameSelector = displaySlangNameSelector_airMistEntry;
	move.partOfStance = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "AirMistFinerB");
	move.displayName = "Air K Mist Finer Stance Entry";
	move.displayNameSelector = displayNameSelector_airMistEntry;
	move.slangName = "j.KMF Entry";
	move.displaySlangNameSelector = displaySlangNameSelector_airMistEntry;
	move.partOfStance = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "AirMistFinerC");
	move.displayName = "Air S Mist Finer Stance Entry";
	move.displayNameSelector = displayNameSelector_airMistEntry;
	move.slangName = "j.SMF Entry";
	move.displaySlangNameSelector = displaySlangNameSelector_airMistEntry;
	move.partOfStance = true;
	addMove(move);
	
	// is entered into from MistFinerA/B/C, has variable duration depending on Mist Finer level
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "AirMistFinerDehajime");
	move.displayName = "Air Mist Finer Entry";
	move.displayNameSelector = displayNameSelector_airMistEntry;
	move.slangName = "j.MF Entry";
	move.displaySlangNameSelector = displaySlangNameSelector_airMistEntry;
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	addMove(move);
	
	// entered into from AirMistFinerDehajime, enables whiff cancels on f2.
	// In Rev2 is exited out of instantly into another Mist Finer from things like MistFinerFWalk.
	// In Rev1 takes one frame to transition.
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "AirMistFinerLoop");
	move.displayName = "Air Mist Finer Stance";
	move.displayNameSelector = displayNameSelector_airMistLoop;
	move.slangName = "j.MF Stance";
	move.displaySlangNameSelector = displaySlangNameSelector_airMistLoop;
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.sectionSeparator = sectionSeparator_enableWhiffCancels;
	move.considerIdleInSeparatedSectionAfterThisManyFrames = 5;
	move.preservesNewSection = true;
	addMove(move);
	
	// forward dash during air Mist Finer
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistFinerFDashAir");
	move.displayName = "Air Mist Finer Forward Dash";
	move.displayNameSelector = displayNameSelector_airMistDash;
	move.slangName = "j.MF Forward Dash";
	move.displaySlangNameSelector = displaySlangNameSelector_airMistDash;
	move.partOfStance = true;
	move.nameIncludesInputs = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.sectionSeparator = sectionSeparator_mistFinerAirDash;
	move.considerNewSectionAsBeingInVariableStartup = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistFinerBDashAir");
	move.displayName = "Air Mist Finer Backdash";
	move.displayNameSelector = displayNameSelector_airMistBackdash;
	move.slangName = "j.MF BD";
	move.displaySlangNameSelector = displaySlangNameSelector_airMistBackdash;
	move.partOfStance = true;
	move.nameIncludesInputs = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.sectionSeparator = sectionSeparator_mistFinerAirDash;
	move.considerNewSectionAsBeingInVariableStartup = true;
	addMove(move);
	
	// performed when releasing the Mist Finer attack
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "AirMistFinerALv0");
	move.displayName = "Lv1 Air P Mist Finer";
	move.slangName = "Lv1 j.PMF";
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.powerup = powerup_mistFiner;
	move.powerupExplanation = powerupExplanation_mistFiner;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "AirMistFinerALv1");
	move.displayName = "Lv2 Air P Mist Finer";
	move.slangName = "Lv2 j.PMF";
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.powerup = powerup_mistFiner;
	move.powerupExplanation = powerupExplanation_mistFiner;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "AirMistFinerALv2");
	move.displayName = "Lv3 Air P Mist Finer";
	move.slangName = "Lv3 j.PMF";
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.powerup = powerup_mistFiner;
	move.powerupExplanation = powerupExplanation_mistFiner;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "AirMistFinerBLv0");
	move.displayName = "Lv1 Air K Mist Finer";
	move.slangName = "Lv1 j.KMF";
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.powerup = powerup_mistFiner;
	move.powerupExplanation = powerupExplanation_mistFiner;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "AirMistFinerBLv1");
	move.displayName = "Lv2 Air K Mist Finer";
	move.slangName = "Lv2 j.KMF";
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.powerup = powerup_mistFiner;
	move.powerupExplanation = powerupExplanation_mistFiner;
	move.usePlusSignInCombination = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "AirMistFinerBLv2");
	move.displayName = "Lv3 Air K Mist Finer";
	move.slangName = "Lv3 j.KMF";
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.powerup = powerup_mistFiner;
	move.powerupExplanation = powerupExplanation_mistFiner;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "AirMistFinerCLv0");
	move.displayName = "Lv1 Air S Mist Finer";
	move.slangName = "Lv1 j.SMF";
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.powerup = powerup_mistFiner;
	move.powerupExplanation = powerupExplanation_mistFiner;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "AirMistFinerCLv1");
	move.displayName = "Lv2 Air S Mist Finer";
	move.slangName = "Lv2 j.SMF";
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.powerup = powerup_mistFiner;
	move.powerupExplanation = powerupExplanation_mistFiner;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "AirMistFinerCLv2");
	move.displayName = "Lv3 Air S Mist Finer";
	move.slangName = "Lv3 j.SMF";
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.powerup = powerup_mistFiner;
	move.powerupExplanation = powerupExplanation_mistFiner;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "AirMistFinerCancel");
	move.displayName = "Air Mist Finer Cancel";
	move.slangName = "j.MFC";
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "TreasureHunt");
	move.displayName = "Treasure Hunt";
	move.slangName = "TH";
	move.sectionSeparator = sectionSeparator_treasureHunt;
	move.isInVariableStartupSection = isInVariableStartupSection_treasureHunt;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "StepTreasureHunt");
	move.displayName = "Stance Dash Treasure Hunt";
	move.slangName = "SDTH";
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.sectionSeparator = sectionSeparator_treasureHunt;
	move.isInVariableStartupSection = isInVariableStartupSection_treasureHunt;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "Coin", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 80;
	move.framebarName = "Glitter Is Gold";
	move.framebarSlangName = "Coin";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "Sinwaza");
	move.displayName = "Zwei Hander";
	move.slangName = "Zwei";
	move.isInVariableStartupSection = isInVariableStartupSection_zweiLand;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "Sinwaza_Shot");
	move.displayName = "Zwei Hander Attack";
	move.slangName = "Zwei K";
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "Sinwaza_Air");
	move.displayName = "Air Zwei Hander";
	move.slangName = "j.Z";
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "Sinwaza_Shot2", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 81;
	move.framebarName = "Zwei Hander";
	move.framebarSlangName = "Zwei";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "Sinwaza_Shot2_Air", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 81;
	move.framebarName = "Zwei Hander";
	move.framebarSlangName = "Zwei";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "Orenona");
	move.displayName = "That's My Name";
	move.slangName = "TMN";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "OrenonaBurst");
	move.displayName = "Burst That's My Name";
	move.slangName = "Burst TMN";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "OrenonaExe");
	move.displayName = "That's My Name";
	move.slangName = "TMN";
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "NmlAtk3B");
	move.displayName = "3K";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "IrukasanTsubureru_tama", true);
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	// this dolphin is created on 41236P/K/S/H. When ridden it disappears
	move = MoveInfo(CHARACTER_TYPE_MAY, "IrukasanRidingObject", true);
	move.sectionSeparatorProjectile = sectionSeparatorProjectile_dolphin;
	move.isDangerous = isDangerous_aboveGround;
	move.framebarId = 11;
	move.framebarName = "Dolphin";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "MayBallA", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 12;
	move.framebarName = "Beach Ball";
	move.framebarSlangName = "Ball";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "MayBallB", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 12;
	move.framebarName = "Beach Ball";
	move.framebarSlangName = "Ball";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "NmlAtk6A");
	move.displayName = "6P";
	move.displayNameSelector = displayNameSelector_may6P;
	move.nameIncludesInputs = true;
	move.sectionSeparator = sectionSeparator_may6P;
	move.isInVariableStartupSection = isInVariableStartupSection_may6Por6H;
	move.powerup = powerup_may6P;
	move.powerupExplanation = powerupExplanation_may6P;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "NmlAtk6D");
	move.displayName = "6H";
	move.displayNameSelector = displayNameSelector_may6H;
	move.nameIncludesInputs = true;
	move.sectionSeparator = sectionSeparator_may6H;
	move.isInVariableStartupSection = isInVariableStartupSection_may6Por6H;
	move.powerup = powerup_may6H;
	move.powerupExplanation = powerupExplanation_may6H;
	addMove(move);
	
	// May riding horizontal Dolphin
	move = MoveInfo(CHARACTER_TYPE_MAY, "IrukasanRidingAttackYokoA");
	move.displayName = "Hop on Dolphin";
	move.slangName = "HoD";
	move.isRecoveryHasGatlings = isRecoveryHasGatlings_mayRideTheDolphin;
	move.isRecoveryCanAct = isRecoveryCanAct_mayRideTheDolphin;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "IrukasanRidingAttackYokoB");
	move.displayName = "Hop on Dolphin";
	move.slangName = "HoD";
	move.isRecoveryHasGatlings = isRecoveryHasGatlings_mayRideTheDolphin;
	move.isRecoveryCanAct = isRecoveryCanAct_mayRideTheDolphin;
	addMove(move);
	
	// May riding vertical Dolphin
	move = MoveInfo(CHARACTER_TYPE_MAY, "IrukasanRidingAttackTateA");
	move.displayName = "Hop on Dolphin";
	move.slangName = "HoD";
	move.isRecoveryHasGatlings = isRecoveryHasGatlings_mayRideTheDolphin;
	move.isRecoveryCanAct = isRecoveryCanAct_mayRideTheDolphin;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "IrukasanRidingAttackTateB");
	move.displayName = "Hop on Dolphin";
	move.slangName = "HoD";
	move.isRecoveryHasGatlings = isRecoveryHasGatlings_mayRideTheDolphin;
	move.isRecoveryCanAct = isRecoveryCanAct_mayRideTheDolphin;
	addMove(move);
	
	// big whale attack
	move = MoveInfo(CHARACTER_TYPE_MAY, "Yamada", true);
	move.framebarName = "Yamada";
	move.framebarId = 12;
	move.isDangerous = isDangerous_not_hasHitNumButInactive;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "Goshogawara");
	move.displayName = "Deluxe Goshogawara Bomber";
	move.framebarSlangName = "USW~P";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	// May spins and may do a suicide whale in the end. This is the suicide whale
	move = MoveInfo(CHARACTER_TYPE_MAY, "SK_Goshogawara", true);
	move.framebarName = "Goshogawara";
	move.framebarId = 13;
	move.isDangerous = isDangerous_not_hasHitNumButNoHitboxes;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "RakkoJump");
	move.displayName = "Ball Jump";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "RakkoJump_F");
	move.displayName = "Ball Jump Forward";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "RakkoJump_B");
	move.displayName = "Ball Jump Back";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "IrukasanYokoD");
	move.displayName = "H Mr. Dolphin Horizontal";
	move.slangName = "H Dolphin";
	move.forceLandingRecovery = true;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "IrukasanYokoC");
	move.displayName = "S Mr. Dolphin Horizontal";
	move.slangName = "S Dolphin";
	move.forceLandingRecovery = true;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "IrukasanTateD");
	move.displayName = "H Mr. Dolphin Vertical";
	move.slangName = "H Updolphin";
	move.forceLandingRecovery = true;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "IrukasanTateC");
	move.displayName = "S Mr. Dolphin Vertical";
	move.slangName = "S Updolphin";
	move.forceLandingRecovery = true;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "RakkoBallB");
	move.displayName = "K Don't Miss It";
	move.slangName = "K Ball";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "RakkoBallA");
	move.displayName = "P Don't Miss It";
	move.slangName = "P Ball";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "OverHeadKiss");
	move.displayName = "Overhead Kiss";
	move.slangName = "OHK";
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "OverHeadKissExe");
	move.displayName = "Overhead Kiss";
	move.slangName = "OHK";
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "IrukasanTateBShoukan");
	move.displayName = "H Applause for the Victim";
	move.slangName = "H-Hoop";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "IrukasanYokoBShoukan");
	move.displayName = "S Applause for the Victim";
	move.slangName = "S-Hoop";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "IrukasanTateAShoukan");
	move.displayName = "K Applause for the Victim";
	move.slangName = "K-Hoop";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "IrukasanYokoAShoukan");
	move.displayName = "P Applause for the Victim";
	move.slangName = "P-Hoop";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "DivingAttack");
	move.displayName = "Ensenga?";
	move.slangName = "Ensenga";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "Dadakko");
	move.displayName = "Ultimate Whiner";
	move.slangName = "Whiner";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "Daisenpu");
	move.displayName = "Ultimate Spinning Whirlwind";
	move.slangName = "USW";
	move.iKnowExactlyWhenTheRecoveryOfThisMoveIs = isRecovery_daisenpu;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "Yamada");
	move.displayName = "Great Yamada Attack";
	move.slangName = "Yamada";
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "YamadaBurst");
	move.displayName = "Burst Great Yamada Attack";
	move.slangName = "Burst Yamada";
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "HaritsukiLA_Hold");
	move.displayName = "Left Wall Climb";
	move.caresAboutWall = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "HaritsukiLC_Hold");
	move.displayName = "Left Wall Climb";
	move.caresAboutWall = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "HaritsukiLD_Hold");
	move.displayName = "Left Wall Climb";
	move.caresAboutWall = true;
	addMove(move);
	
	// Chipp wall cling attach
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "HaritsukiL");
	move.displayName = "Left Wall Climb";
	move.caresAboutWall = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "HaritsukiRA_Hold");
	move.displayName = "Right Wall Climb";
	move.caresAboutWall = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "HaritsukiRC_Hold");
	move.displayName = "Right Wall Climb";
	move.caresAboutWall = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "HaritsukiRD_Hold");
	move.displayName = "Right Wall Climb";
	move.caresAboutWall = true;
	addMove(move);
	
	// Chipp wall cling attach
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "HaritsukiR");
	move.displayName = "Right Wall Climb";
	move.caresAboutWall = true;
	addMove(move);
	
	// Chipp wall cling idle/moving up/down
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "HaritsukiKeep");
	move.displayName = "Wall Climb";
	move.combineWithPreviousMove = true;
	move.isIdle = isIdle_enableWhiffCancels;
	move.canBlock = canBlock_default;
	move.canBeUnableToBlockIndefinitelyOrForVeryLongTime = true;
	move.caresAboutWall = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "SankakuTobiUpper");
	move.displayName = "w.9";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "SankakuTobiDown");
	move.displayName = "w.3/6";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "Kaijo");
	move.displayName = "w.4";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "HaritsukiD");
	move.displayName = "w.H";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "GenyouzanExe");
	move.displayName = "w.H";
	move.combineWithPreviousMove = true;
	move.conditionForAddingWhiffCancels = hasWhiffCancels;
	move.addForceAddWhiffCancel("HaritsukiLD_Hold");
	move.addForceAddWhiffCancel("HaritsukiRD_Hold");
	move.isGrab = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "HaritsukiC");
	move.displayName = "w.S";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "HaritsukiB");
	move.displayName = "w.K";
	move.caresAboutWall = true;
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "HaritsukiA");
	move.displayName = "w.A";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "AlphaPlus");
	move.displayName = "Alpha Plus";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "WarpA");
	move.displayName = "P Tsuyoshi-shiki Ten'i";
	move.slangName = "22P";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "WarpB");
	move.displayName = "K Tsuyoshi-shiki Ten'i";
	move.slangName = "22K";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "WarpC");
	move.displayName = "S Tsuyoshi-shiki Ten'i";
	move.slangName = "22S";
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = hasWhiffCancels;
	move.conditionForAddingWhiffCancels = hasWhiffCancels;
	move.addForceAddWhiffCancel("HaritsukiLC_Hold");
	move.addForceAddWhiffCancel("HaritsukiRC_Hold");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "WarpD");
	move.displayName = "H Tsuyoshi-shiki Ten'i";
	move.slangName = "22H";
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = hasWhiffCancels;
	move.conditionForAddingWhiffCancels = hasWhiffCancels;
	move.addForceAddWhiffCancel("HaritsukiLD_Hold");
	move.addForceAddWhiffCancel("HaritsukiRD_Hold");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "AlphaBlade");
	move.displayName = "Alpha Blade";
	move.slangName = "Alpha";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "AirAlphaBlade");
	move.displayName = "Air Alpha Blade";
	move.slangName = "Air Alpha";
	move.conditionForAddingWhiffCancels = hasWhiffCancels;
	move.addForceAddWhiffCancel("HaritsukiLA_Hold");
	move.addForceAddWhiffCancel("HaritsukiRA_Hold");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "Sushi");
	move.displayName = "Resshou";
	move.ignoreSuperJumpInstalls = true;  // this move only cancels into Rokusai, Senshuu and Shinkirou, and of those only two can make you airborne, and they both give a free airdash
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "Sukiyaki");
	move.displayName = "Rokusai";
	move.ignoreSuperJumpInstalls = true;  // only cancels into Senshuu and Shinkirou, both of which give a free airdash
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "Banzai");
	move.displayName = "Senshuu";
	move.ignoreSuperJumpInstalls = true;  // gives an airdash
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "GammaBladeObj", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 26;
	move.framebarName = "Gamma Blade";
	move.framebarSlangName = "Gamma";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "Shuriken");
	move.displayName = "Shuriken";
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "Meisai");
	move.displayName = "Tsuyoshi-shiki Meisai";
	move.slangName = "Meisai";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "Tobiagari");
	move.displayName = "Shinkirou";
	move.conditionForAddingWhiffCancels = hasWhiffCancels;
	move.addForceAddWhiffCancel("HaritsukiLD_Hold");
	move.addForceAddWhiffCancel("HaritsukiRD_Hold");
	move.ignoreSuperJumpInstalls = true;  // gets an airdash
	// after active frames end, this move recovers double jumps as well, but you do care about jump installs if you want to obtain them sooner
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "BetaBlade");
	move.displayName = "Beta Blade";
	move.slangName = "Beta";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "AirBetaBlade");
	move.displayName = "Air Beta Blade";
	move.slangName = "Air Beta";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "Genrouzan");
	move.displayName = "Genrou Zan";
	move.conditionForAddingWhiffCancels = hasWhiffCancels;
	move.addForceAddWhiffCancel("HaritsukiLC_Hold");
	move.addForceAddWhiffCancel("HaritsukiRC_Hold");
	move.ignoreSuperJumpInstalls = true;  // gives an airdash
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "GenrouzanExe");
	move.displayName = "Genrou Zan";
	move.combineWithPreviousMove = true;
	move.conditionForAddingWhiffCancels = hasWhiffCancels;
	move.addForceAddWhiffCancel("HaritsukiLC_Hold");
	move.addForceAddWhiffCancel("HaritsukiRC_Hold");
	move.isGrab = true;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "GammaBlade");
	move.displayName = "Gamma Blade";
	move.slangName = "Gamma";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "6wayKunai");
	move.displayName = "Ryuu Yanagi";
	move.slangName = "Ryuu";
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "BankiMessai");
	move.displayName = "Banki Messai";
	move.slangName = "Banki";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "ZanseiRouga");
	move.displayName = "Zansei Rouga";
	move.slangName = "Zansei";
	move.forceLandingRecovery = true;
	move.forceSuperHitAnyway = forceSuperHitAnyway_zanseiRouga;
	move.iKnowExactlyWhenTheRecoveryOfThisMoveIs = isRecovery_zanseiRouga;
	move.ignoreSuperJumpInstalls = true;  // gets an airdash
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "ZanseiRougaBurst");
	move.displayName = "Burst Zansei Rouga";
	move.slangName = "Burst Zansei";
	move.forceLandingRecovery = true;
	move.ignoreSuperJumpInstalls = true;  // gets an airdash
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "ShurikenObj", true);
	move.isDangerous = isDangerous_notNull;
	move.framebarId = 27;
	move.framebarName = "Shuriken Slow";
	move.framebarSlangName = "Shuriken-";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "ShurikenObj1", true);
	move.isDangerous = isDangerous_notNull;
	move.framebarId = 28;
	move.framebarName = "Shuriken Fast";
	move.framebarSlangName = "Shuriken+";
	addMove(move);
	
	// throwing daggers from wall cling
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "Kunai_Wall", true);
	move.isDangerous = isDangerous_notNull;
	move.framebarId = 29;
	move.framebarName = "Kunai";
	addMove(move);
	
	// 214214K air super
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "Kunai", true);
	move.isDangerous = isDangerous_notNull;
	move.framebarId = 30;
	move.framebarName = "Ryuu Yanagi";
	move.framebarSlangName = "Ryuu";
	move.combineHitsFromDifferentProjectiles = true;
	addMove(move);
	
	// All specials that can put Faust into the air from the ground already give him an airdash by default, without having
	// to super jump install. As such, Faust never ever cares about super jump installs.
	charDoesNotCareAboutSuperJumpInstalls[CHARACTER_TYPE_FAUST] = true;
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "NmlAtk5E");
	move.displayName = "5D";
	move.nameIncludesInputs = true;
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "CrouchFWalk");
	move.displayName = "Crouchwalk Forward";
	move.slangName = "3";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "CrouchBWalk");
	move.displayName = "Crouchwalk Back";
	move.slangName = "1";
	move.nameIncludesInputs = true;
	addMove(move);
	
	// Faust Pogo
	// Pogo entry
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Souten");
	move.displayName = "Spear Point Centripetal Dance";
	move.displayNameSelector = displayNameSelector_pogoEntry;
	move.slangName = "Pogo";
	move.displaySlangNameSelector = displaySlangNameSelector_pogoEntry;
	move.isIdle = isIdle_enableWhiffCancels;
	move.canBlock = canBlock_default;
	move.canBeUnableToBlockIndefinitelyOrForVeryLongTime = true;
	move.faustPogo = true;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "SoutenCancel");
	move.displayName = "Spear Point Cenripetal Dance Cancel";
	move.slangName = "Pogo Cancel";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "MettaGiri");
	move.displayName = "Hack 'n Slash";
	move.slangName = "214H";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "NanigaDerukana");
	move.displayName = "What Could This Be?";
	move.slangName = "Toss";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Oissu");
	move.displayName = "Hello!";
	move.slangName = "236P";
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "KoegaChiisai");
	move.displayName = "Can't Hear You!";
	move.slangName = "236P";
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = hasWhiffCancels;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "MouicchoOissu");
	move.displayName = "Hello Again!";
	move.slangName = "236P";
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "UekaraIkimasuyo");
	move.displayName = "From Above";
	move.slangName = "S-Door";
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "UshirokaraIkimasuyo");
	move.displayName = "From Behind";
	move.slangName = "K-Door";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "MaekaraIkimasuyo");
	move.displayName = "From the Front";
	move.slangName = "P-Door";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "AirGoingMyWay");
	move.displayName = "Air Going My Way";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Ai");
	move.displayName = "Love";
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "RerereNoTsuki");
	move.displayName = "Re-re-re Thrust";
	move.slangName = "Scalpel";
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "NaNaNaNanigaDerukana");
	move.displayName = "W-W-What Could This Be?";
	move.slangName = "Super Toss";
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "SugoiNaNaNaNanigaDerukana");
	move.displayName = "W-W-What Could This Be? 100% Ver.";
	move.slangName = "Max Super Toss";
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Shigeki");
	move.displayName = "Stimulating Fists of Annihilation";
	move.slangName = "Fists";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "ShigekiBurst");
	move.displayName = "Burst Stimulating Fists of Annihilation";
	move.slangName = "Burst Fists";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	// Pogo P
	move = MoveInfo(CHARACTER_TYPE_FAUST, "SoutenA");
	move.displayName = "Just A Taste!";
	move.displayNameSelector = displayNameSelector_pogoA;
	move.slangName = "Pogo-P";
	move.displaySlangNameSelector = displaySlangNameSelector_pogoA;
	move.isIdle = isIdle_enableWhiffCancels;
	move.canBlock = canBlock_default;
	move.canBeUnableToBlockIndefinitelyOrForVeryLongTime = true;
	move.faustPogo = true;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	// Pogo hop
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Souten9");
	move.displayName = "Short Hop";
	move.displayNameSelector = displayNameSelector_pogo9;
	move.slangName = "Pogo-9";
	move.displaySlangNameSelector = displaySlangNameSelector_pogo9;
	move.isIdle = isIdle_enableWhiffCancels;
	move.canBeUnableToBlockIndefinitelyOrForVeryLongTime = true;
	move.faustPogo = true;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	// Pogo 44
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Souten44");
	move.displayName = "Backward Movement";
	move.displayNameSelector = displayNameSelector_pogo44;
	move.slangName = "Pogo-44";
	move.displaySlangNameSelector = displaySlangNameSelector_pogo44;
	move.isIdle = isIdle_enableWhiffCancels;
	move.canBlock = canBlock_default;
	move.canBeUnableToBlockIndefinitelyOrForVeryLongTime = true;
	move.faustPogo = true;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	// Pogo 66
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Souten66");
	move.displayName = "Forward Movement";
	move.displayNameSelector = displayNameSelector_pogo66;
	move.slangName = "Pogo-66";
	move.displaySlangNameSelector = displaySlangNameSelector_pogo66;
	move.isIdle = isIdle_enableWhiffCancels;
	move.canBlock = canBlock_default;
	move.canBeUnableToBlockIndefinitelyOrForVeryLongTime = true;
	move.faustPogo = true;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	// Pogo K (head flower)
	move = MoveInfo(CHARACTER_TYPE_FAUST, "SoutenB");
	move.displayName = "Growing Flower";
	move.displayNameSelector = displayNameSelector_pogoB;
	move.slangName = "Pogo-K";
	move.displaySlangNameSelector = displaySlangNameSelector_pogoB;
	move.sectionSeparator = sectionSeparator_soutenBC;
	move.isIdle = isIdle_enableWhiffCancels;
	move.canBlock = canBlock_default;
	move.canBeUnableToBlockIndefinitelyOrForVeryLongTime = true;
	move.isInVariableStartupSection = isInVariableStartupSection_soutenBC;
	move.faustPogo = true;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	// Pogo S (ground flower)
	move = MoveInfo(CHARACTER_TYPE_FAUST, "SoutenC");
	move.displayName = "See? I'm a Flower!";
	move.displayNameSelector = displayNameSelector_pogoC;
	move.slangName = "Pogo-S";
	move.displaySlangNameSelector = displaySlangNameSelector_pogoC;
	move.sectionSeparator = sectionSeparator_soutenBC;
	move.isIdle = isIdle_enableWhiffCancels;
	move.canBlock = canBlock_default;
	move.canBeUnableToBlockIndefinitelyOrForVeryLongTime = true;
	move.isInVariableStartupSection = isInVariableStartupSection_soutenBC;
	move.faustPogo = true;
	move.canYrcProjectile = canYrcProjectile_flower;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	// Pogo Going My Way
	move = MoveInfo(CHARACTER_TYPE_FAUST, "SoutenD");
	move.displayName = "Spear Point Centripetal Dance Going My Way";
	move.displayNameSelector = displayNameSelector_pogoD;
	move.slangName = "Pogo-H";
	move.displaySlangNameSelector = displaySlangNameSelector_pogoD;
	move.isIdle = isIdle_enableWhiffCancels;
	move.canBlock = canBlock_default;
	move.canBeUnableToBlockIndefinitelyOrForVeryLongTime = true;
	move.faustPogo = true;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "SoutenE");
	move.displayName = "Spear Point Centripetal Dance What Could This Be?";
	move.displayNameSelector = displayNameSelector_pogoE;
	move.slangName = "Pogo-D";
	move.displaySlangNameSelector = displaySlangNameSelector_pogoE;
	move.isIdle = isIdle_enableWhiffCancels;
	move.canBlock = canBlock_default;
	move.canBeUnableToBlockIndefinitelyOrForVeryLongTime = true;
	move.faustPogo = true;
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	// Faust Pogo Helicopter
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Souten8");
	move.displayName = "Doctor-Copter";
	move.displayNameSelector = displayNameSelector_pogo8;
	move.slangName = "Pogo-8";
	move.displaySlangNameSelector = displaySlangNameSelector_pogo8;
	move.isIdle = hasWhiffCancels;
	move.canBlock = canBlock_default;
	move.canBeUnableToBlockIndefinitelyOrForVeryLongTime = true;
	move.faustPogo = true;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	// Faust 41236K (long ass fishing pole poke that drags you) succeeeding
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Hikimodoshi");
	move.displayName = "Pull Back";
	move.combineWithPreviousMove = true;
	move.butForFramebarDontCombineWithPreviousMove = true;
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	// ground flower. The head flower cannot be RC'd. This is not the head flower. This flower can be RC'd, but not in Rev1.
	move = MoveInfo(CHARACTER_TYPE_FAUST, "OreHana_Shot", true);
	move.isDangerous = isDangerous_playerInRCOrHasActiveFlag_AndNotInRecovery;
	move.framebarId = 31;
	move.framebarName = "Flower";
	addMove(move);
	
	// ground flower maximum. Not present in Rev1
	move = MoveInfo(CHARACTER_TYPE_FAUST, "OreHanaBig_Shot", true);
	move.isDangerous = isDangerous_playerInRCOrHasActiveFlag_AndNotInRecovery;
	move.framebarId = 31;
	move.framebarName = "Flower";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Item_Oilcan", true);
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Item_Bomb", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 32;
	move.framebarName = "Bomb";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Item_BlackHole", true);
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	// fire created when setting oil on fire
	move = MoveInfo(CHARACTER_TYPE_FAUST, "OilFire", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 33;
	move.framebarName = "Oil Fire";
	addMove(move);
	
	// normal meteor. Does not have active frames. Creates several MeteoInseki which have active frames
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Item_Meteo", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 34;
	move.framebarName = "Meteor";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "MeteoInseki", true);
	move.isDangerous = isDangerous_notNull;
	move.framebarId = 34;
	move.framebarName = "Meteor";
	move.combineHitsFromDifferentProjectiles = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Item_Helium", true);
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Item_Hammer", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 35;
	move.framebarName = "Hammer";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Item_ChibiFaust", true);
	move.isDangerous = isDangerous_not_hasHitNumButNoHitboxes;
	move.framebarId = 36;
	move.framebarName = "Small Faust";
	move.framebarSlangName = "Mini Faust";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Item_Frasco", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 37;
	move.framebarName = "Poison";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Item_Chocolate", true);
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Item_BestChocolate", true);
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Item_Donut", true);
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Item_ManyDonut", true);
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	// the poison cloud created when poison flask lands
	move = MoveInfo(CHARACTER_TYPE_FAUST, "SubItem_Poison", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 37;
	move.framebarName = "Poison";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Item_JumpStand", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 38;
	move.framebarName = "Platform";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Item_100t", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 39;
	move.framebarName = "100-ton Weight";
	move.framebarSlangName = "Weight";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Item_FireWorks", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 40;
	move.framebarName = "Fireworks";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Item_Armageddon", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 41;
	move.framebarName = "Massive Meteor";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "ArmageddonInseki", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 41;
	move.framebarName = "Massive Meteor";
	move.framebarSlangName = "Big Meteor";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Item_GoldenHammer", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 42;
	move.framebarName = "Golden Hammer";
	move.framebarSlangName = "Gold Hammer";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Item_BigFaust", true);
	move.isDangerous = isDangerous_not_hasHitNumButNoHitboxes;
	move.framebarId = 43;
	move.framebarName = "Huge Faust";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Item_Golden100t", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 44;
	move.framebarName = "10,000 Ton Weight";
	move.framebarSlangName = "Gold Weight";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	// the initial projectile Faust drops
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Ai_Bomb", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 45;
	move.framebarName = "Love";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	// the explosion created when Love touches the ground
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Ai_Bomb2", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 45;
	move.framebarName = "Love";
	move.framebarNameUncombined = "Love Explosion";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "ShigekiJibakuObj", true);
	move.framebarId = 108;
	move.framebarName = "Stimulating Fists of Annihilation Self-Destruct";
	move.framebarSlangName = "Self-Destuct";
	addMove(move);
	
	// Axl has only one move that can put him airborne from the ground, and that is Raiei, but it already gives an airdash by default.
	// Therefore, Axl has no use for super jump installs.
	charDoesNotCareAboutSuperJumpInstalls[CHARACTER_TYPE_AXL] = true;
	
	move = MoveInfo(CHARACTER_TYPE_AXL, "NmlAtk5CNearHasei");
	move.displayName = "c.S";
	move.nameIncludesInputs = true;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	// Axl Haitaka stance
	move = MoveInfo(CHARACTER_TYPE_AXL, "DaiRensen");
	move.displayName = "Sparrowhawk Stance";
	move.slangName = "Haitaka";
	move.isIdle = isIdle_sparrowhawkStance;
	move.canBlock = canBlock_default;
	move.canBeUnableToBlockIndefinitelyOrForVeryLongTime = true;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = hasWhiffCancels;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_AXL, "DaiRensenA");
	move.displayName = "Sparrowhawk Stance P";
	move.slangName = "Haitaka-P";
	move.ignoreJumpInstalls = true;
	addMove(move);

	move = MoveInfo(CHARACTER_TYPE_AXL, "DaiRensenB");
	move.displayName = "Sparrowhawk Stance K";
	move.slangName = "Haitaka-K";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_AXL, "DaiRensenC");
	move.displayName = "Sparrowhawk Stance S";
	move.slangName = "Haitaka-S";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_AXL, "DaiRensenD");
	move.displayName = "Sparrowhawk Stance H";
	move.slangName = "Haitaka-H";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_AXL, "AxelBomber");
	move.displayName = "Axl Bomber";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_AXL, "TenhousekiJou");
	move.displayName = "P Heaven Can Wait";
	move.slangName = "P-Parry";
	move.ignoreJumpInstalls = true;  // can only RC on the ground
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_AXL, "TenhousekiGe");
	move.displayName = "K Heaven Can Wait";
	move.slangName = "K-Parry";
	move.ignoreJumpInstalls = true;  // can only RC on the ground
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_AXL, "BentenGari");
	move.displayName = "Artemis Hunter";
	move.slangName = "Benten";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_AXL, "Raieisageki");
	move.displayName = "Thunder Shadow Chain";
	move.slangName = "Raiei";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_AXL, "KairagiYakou");
	move.displayName = "Shark Strike";
	move.slangName = "Kairagi";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_AXL, "ByakueRenshou");
	move.displayName = "Sickle Storm";
	move.iKnowExactlyWhenTheRecoveryOfThisMoveIs = isRecovery_byakueRenshou;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_AXL, "ByakueRenshouBurst");
	move.displayName = "Burst Sickle Storm";
	move.iKnowExactlyWhenTheRecoveryOfThisMoveIs = isRecovery_byakueRenshou;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	// Axl Rensen
	move = MoveInfo(CHARACTER_TYPE_AXL, "Rensengeki");
	move.displayName = "Sickle Flash";
	move.slangName = "Rensen";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	// Axl Rensen + 2 followup
	move = MoveInfo(CHARACTER_TYPE_AXL, "Sensageki");
	move.displayName = "Spinning Chain Strike";
	move.slangName = "Rensen-2";
	move.combineWithPreviousMove = true;
	move.butForFramebarDontCombineWithPreviousMove = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	// Axl Rensen + 8 followup
	move = MoveInfo(CHARACTER_TYPE_AXL, "Kyokusageki");
	move.displayName = "Melody Chain";
	move.slangName = "Rensen-8";
	move.combineWithPreviousMove = true;
	move.butForFramebarDontCombineWithPreviousMove = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	// the command grab
	move = MoveInfo(CHARACTER_TYPE_AXL, "Rashosen");
	move.displayName = "Spindle Spinner";
	move.slangName = "Rashousen";
	move.isGrab = true;
	move.ignoreJumpInstalls = true;  // because you can only RC it on the ground
	addMove(move);
	
	// the command grab
	move = MoveInfo(CHARACTER_TYPE_AXL, "RashosenObj", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 46;
	move.framebarName = "Spindle Spinner";
	move.framebarSlangName = "Rashousen";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_AXL, "RensengekiObj", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 47;
	move.framebarName = "Sickle Flash";
	move.framebarSlangName = "Rensen";
	addMove(move);
	
	// the 8 followup
	move = MoveInfo(CHARACTER_TYPE_AXL, "KyokusagekiObj", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 47;
	move.framebarName = "Melody Chain";
	move.framebarSlangName = "Rensen-8";
	addMove(move);
	
	// the 2363214H super second hit
	move = MoveInfo(CHARACTER_TYPE_AXL, "ByakueObj", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 48;
	move.framebarName = "Sickle Storm";
	addMove(move);
	
	// the only move that Elphelt has that can transfer her from the ground into the air is Judge Better Half,
	// and you get a free airdash on it anyway.
	// And for moves that are jump cancellable, all she can do is jump from them, and get one airdash and one double jump by default,
	// without requiring any installs.
	// So a super jump install is, mathematically proven, useless to her
	charDoesNotCareAboutSuperJumpInstalls[CHARACTER_TYPE_ELPHELT] = true;
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "NmlAtkAir5E");
	move.displayName = "j.D";
	move.nameIncludesInputs = true;
	move.createdProjectile = createdProjectile_elpheltjD;
	move.canYrcProjectile = canYrcProjectile_elpheltjD;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "CmnActStand");
	move.displayName = "Stand";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "CmnActCrouch2Stand");
	move.displayName = "Crouch to Stand";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	// Elphelt Ms. Confille (rifle)
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Rifle_Start");
	move.displayName = "Aim Ms. Confille";
	move.displayNameSelector = displayNameSelector_rifleStart;
	move.slangName = "Rifle";
	move.displaySlangNameSelector = displaySlangNameSelector_rifleStart;
	move.sectionSeparator = sectionSeparator_rifle;
	move.isIdle = isIdle_Rifle;
	move.canBlock = canBlock_default;
	move.considerNewSectionAsBeingInElpheltRifleStateBeforeBeingAbleToShoot = true;
	move.canBeUnableToBlockIndefinitelyOrForVeryLongTime = true;
	move.powerup = powerup_rifle;
	move.powerupExplanation = powerupExplanation_rifle;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Rifle_Reload");
	move.displayName = "Ms. Confille Reload";
	move.displayNameSelector = displayNameSelector_rifleReload;
	move.slangName = "Reload";
	move.displaySlangNameSelector = displaySlangNameSelector_rifleReload;
	move.sectionSeparator = sectionSeparator_rifle;
	move.isIdle = isIdle_Rifle;
	move.canBlock = canBlock_default;
	move.considerNewSectionAsBeingInElpheltRifleStateBeforeBeingAbleToShoot = true;
	move.canBeUnableToBlockIndefinitelyOrForVeryLongTime = true;
	move.powerup = powerup_rifle;
	move.powerupExplanation = powerupExplanation_rifle;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Rifle_Reload_Perfect");
	move.displayName = "Ms. Confille Perfect Reload";
	move.displayNameSelector = displayNameSelector_riflePerfectReload;
	move.slangName = "Perfect Reload";
	move.displaySlangNameSelector = displaySlangNameSelector_riflePerfectReload;
	move.sectionSeparator = sectionSeparator_rifle;
	move.isIdle = isIdle_Rifle;
	move.canBlock = canBlock_default;
	move.considerNewSectionAsBeingInElpheltRifleStateBeforeBeingAbleToShoot = true;
	move.canBeUnableToBlockIndefinitelyOrForVeryLongTime = true;
	move.replacementInputs = "46S. S must be either on the same frame as 6 or on the frame after";
	move.replacementBufferTime = 1;
	move.powerup = powerup_rifle;
	move.powerupExplanation = powerupExplanation_rifle;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	// Entered into from CmnActRomanCancel if its performed during rifle stance either after entering the stance or after firing or after reloading.
	// On f1 whiff cancels are not enabled yet, on f2 enabled
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Rifle_Roman");
	move.displayName = "Ms. Confille Roman Cancel";
	move.displayNameSelector = displayNameSelector_rifleRC;
	move.slangName = "Rifle RC";
	move.displaySlangNameSelector = displaySlangNameSelector_rifleRC;
	move.sectionSeparator = sectionSeparator_rifle;
	move.isIdle = isIdle_Rifle;
	move.canBlock = canBlock_default;
	move.considerNewSectionAsBeingInElpheltRifleStateBeforeBeingAbleToShoot = true;
	move.canBeUnableToBlockIndefinitelyOrForVeryLongTime = true;
	move.powerup = powerup_rifle;
	move.powerupExplanation = powerupExplanation_rifle;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Rifle_Fire");
	move.displayName = "Ms. Confille Fire";
	move.slangName = "Fire";
	move.isRecoveryCanReload = isRecoveryCanReload_rifle;
	move.canYrcProjectile = canYrcProjectile_default;
	move.powerup = powerup_rifle;
	move.powerupExplanation = powerupExplanation_rifle;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Shotgun_Fire_MIN");
	move.displayName = "sg.H";
	move.nameIncludesInputs = true;
	move.isRecoveryCanReload = isRecoveryHasGatlings_enableWhiffCancels;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Shotgun_Fire_MAX");
	move.displayName = "Max Charge sg.H";
	move.nameIncludesInputs = true;
	move.isRecoveryCanReload = isRecoveryHasGatlings_enableWhiffCancels;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Shotgun_Reload");
	move.displayName = "Ms. Travailler Reload";
	move.slangName = "Shotgun Reload";
	move.combineWithPreviousMove = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Shotgun_Upper");
	move.displayName = "sg.S";
	move.nameIncludesInputs = true;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Shotgun_Rolling");
	move.displayName = "sg.K";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Shotgun_Koduki");
	move.displayName = "sg.P";
	move.nameIncludesInputs = true;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Grenade_Land_Throw_Upper");
	move.displayName = "High Toss";
	move.slangName = "4Toss";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Shotgun_Grenade_Throw_Upper");
	move.displayName = "Ms. Travailler Stance High Toss";
	move.slangName = "4Toss";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Grenade_Air_Throw");
	move.displayName = "Air High Toss";
	move.slangName = "Air Toss";
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Grenade_Land_Throw_Down");
	move.displayName = "Low Toss";
	move.slangName = "2Toss";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Shotgun_Grenade_Throw_Down");
	move.displayName = "Ms. Travailler Stance Low Toss";
	move.slangName = "2Toss";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Shotgun_CQC");
	move.displayName = "CQC";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Shotgun_CQCExe");
	move.displayName = "CQC";
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Rifle_End");
	move.displayName = "Ms. Confille Stance Exit";
	move.slangName = "Rifle Exit";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Shotgun_Ready");
	move.displayName = "Aim Ms. Travailler";
	move.slangName = "Pull Shotgun";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Shotgun_End");
	move.displayName = "Ms. Travailler Cancel";
	move.slangName = "Shotgun Exit";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Shotgun_Rolling2");
	move.displayName = "Roll, and Aim with Miss Travailler";
	move.slangName = "Roll";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Grenade_Land_Standby");
	move.displayName = "Berry Pine";
	move.slangName = "Pull";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Grenade_Air_Standby");
	move.displayName = "Air Berry Pine";
	move.slangName = "Air Pull";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Shotgun_Grenade_Standby");
	move.displayName = "Ms. Travailler Stance Berry Pine";
	move.slangName = "Pull";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "BridalExpress_Land");
	move.displayName = "Bridal Express";
	move.slangName = "Bridal";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "BridalExpress_Air");
	move.displayName = "Air Bridal Express";
	move.slangName = "Air Bridal";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Bazooka");
	move.displayName = "Genoverse";
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Judge_BetterHalf");
	move.displayName = "Judge Better Half";
	move.slangName = "JBH";
	move.iKnowExactlyWhenTheRecoveryOfThisMoveIs = isRecovery_land;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Judge_BetterHalfBurst");
	move.displayName = "Burst Judge Better Half";
	move.slangName = "Burst JBH";
	move.iKnowExactlyWhenTheRecoveryOfThisMoveIs = isRecovery_land;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "GrenadeBomb", true);
	move.isDangerous = isDangerous_grenade;
	move.framebarId = 73;
	move.framebarName = "Berry Pine";
	move.framebarSlangName = PROJECTILE_NAME_BERRY;
	move.drawProjectileOriginPoint = true;
	move.showMultipleHitsFromOneAttack = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "GrenadeBomb_Ready", true);
	move.isDangerous = isDangerous_grenade;
	move.framebarId = 73;
	move.framebarName = "Self-Detonate";
	move.framebarSlangName = "Self-Detonate";
	addMove(move);
	
	// This explosion results from the timer running out normally
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "GrenadeBomb_Explode", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 73;
	move.framebarName = "Berry Explosion";
	move.framebarSlangName = "Explosion";
	addMove(move);
	
	// This explosion results from clashing with the opponent's projectiles
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "GrenadeBomb_Explode2", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 73;
	move.framebarName = "Berry Pine";
	move.framebarSlangName = "Berry Explode";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "HandGun_air_shot", true);
	move.isDangerous = isDangerous_playerInRCOrHasActiveFlag_AndNotInRecovery;
	move.framebarId = 74;
	move.framebarName = "j.D";
	// in Rev1 you can't YRC this
	addMove(move);
	
	// Max charge shotgun shot spawns two projectiles: Shotgun_max_1, Shotgun_max_2
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Shotgun_max_1", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 75;
	move.framebarNameSelector = framebarNameSelector_closeShot;
	move.projectilePowerup = powerup_closeShot;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Shotgun_max_2", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 75;
	move.framebarName = "Max Far Shot";
	addMove(move);
	
	// Shotgun shot spawns two projectiles: Shotgun_min_1, Shotgun_min_2
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Shotgun_min_1", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 75;
	move.framebarName = "Close Shot";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Shotgun_min_2", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 75;
	move.framebarName = "Far Shot";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Bazooka_Fire", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 76;
	move.framebarName = "Genoverse";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Bazooka_Explosive", true);
	move.isDangerous = isDangerous_not_NullWhileActive;
	move.framebarId = 76;
	move.framebarName = "Geno Explode";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Rifle_Fire_MAX", true);
	move.isDangerous = isDangerous_not_NullWhileActive;
	move.framebarId = 77;
	move.framebarName = "Ms. Confille Shot";
	move.framebarSlangName = "Max Rifleshot";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Rifle_Fire_MIN", true);
	move.isDangerous = isDangerous_not_NullWhileActive;
	move.framebarId = 77;
	move.framebarName = "Ms. Confille Shot";
	move.framebarSlangName = "Rifleshot";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "NmlAtk2E");
	move.displayName = "2D";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	// Leo backturn idle and also exiting backturn via 22
	move = MoveInfo(CHARACTER_TYPE_LEO, "Semuke");
	move.displayName = "Brynhildr Stance";
	move.slangName = "Backturn";
	move.isIdle = isIdle_enableWhiffCancels;
	move.canBlock = canBlock_default;
	move.canBeUnableToBlockIndefinitelyOrForVeryLongTime = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "Semuke5A");
	move.displayName = "bt.P";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "Semuke5B");
	move.displayName = "bt.K";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "Semuke5C");
	move.displayName = "bt.S";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "Semuke5D");
	move.displayName = "bt.H";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "Semuke5E");
	move.displayName = "bt.D";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "Tobidogu2");
	move.displayName = "H Graviert W\xc3\xbcrde";
	move.slangName = "H Fireball";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "Tobidogu1");
	move.displayName = "S Graviert W\xc3\xbcrde";
	move.slangName = "S Fireball";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "AntiAirAttack2");
	move.displayName = "H Eisen Sturm";
	move.slangName = "H Eisen";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "AntiAirAttack1");
	move.displayName = "S Eisen Sturm";
	move.slangName = "S Eisen";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "Tossin2");
	move.displayName = "Kaltes Gest\xc3\xb6\x62\x65r Zweit";
	move.slangName = "Zweit";
	move.powerup = powerup_zweit;
	move.powerupExplanation = powerupExplanation_zweit;
	move.dontShowPowerupGraphic = alwaysTrue;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "Tossin1");
	move.displayName = "Kaltes Gest\xc3\xb6\x62\x65r Erst";
	move.slangName = "Rekka";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "SemukeCantGuard");
	move.displayName = "Blitzschlag";
	move.slangName = "bt.214H";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "SemukeDageki");
	move.displayName = "Kaltes Gest\xc3\xb6\x62\x65r Dritt";
	move.slangName = "Dritt";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "AirSpecial");
	move.displayName = "Siegesparade";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "AirSpecialExe");
	move.displayName = "Siegesparade";
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "Tossin2_Hasei");
	move.displayName = "Kaltes Gest\xc3\xb6\x62\x65r Zweit (Follow-up)";
	move.slangName = "> Zweit";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "SemukeDageki_Hasei");
	move.displayName = "Kaltes Gest\xc3\xb6\x62\x65r Dritt (Follow-up)";
	move.slangName = "> Dritt";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "Gorengeki");
	move.displayName = "Leidenschaft Dirigent";
	move.slangName = "Leidenschaft";
	move.iKnowExactlyWhenTheRecoveryOfThisMoveIs = isRecovery_land;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "SemukeKakusei");
	move.displayName = "Stahl Wirbel";
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "SemukeKakuseiBurst");
	move.displayName = "Burst Stahl Wirbel";
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "SemukeFDashStep");
	move.displayName = "Brynhildr Stance Forward Dash";
	move.slangName = "bt.66";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "SemukeBDashStep");
	move.displayName = "Brynhildr Stance Backdash";
	move.slangName = "bt.44";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "NmlAtk5CFar");
	move.displayName = "f.S";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "NmlAtk5CFar_Guard");
	move.displayName = "f.S~P";
	move.nameIncludesInputs = true;
	move.sectionSeparator = sectionSeparator_leoGuardStance;
	move.canStopHolding = aSectionBeforeVariableStartup_leoParry;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "NmlAtk5D");
	move.displayName = "5H";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "NmlAtk5D_Guard");
	move.displayName = "5H~P";
	move.nameIncludesInputs = true;
	move.sectionSeparator = sectionSeparator_leoGuardStance;
	move.canStopHolding = aSectionBeforeVariableStartup_leoParry;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "Edgeyowai", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 78;
	move.framebarName = "Graviert W\xc3\xbcrde";
	move.framebarSlangName = "S Fireball";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "Edgetuyoi", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 78;
	move.framebarName = "Graviert W\xc3\xbcrde";
	move.framebarSlangName = "H Fireball";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "SemukeKakusei_Obj", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 79;
	move.framebarName = "Stahl Wirbel";
	addMove(move);
	
	// Jam parry
	move = MoveInfo(CHARACTER_TYPE_JAM, "NeoHochihu");
	move.displayName = "Hochifu";
	move.isIdle = canBlock_neoHochihu;
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.canFaultlessDefend = alwaysTrue;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	// Jam 236S
	move = MoveInfo(CHARACTER_TYPE_JAM, "Bakushuu");
	move.displayName = "Bakushuu";
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.considerVariableStartupAsStanceForFramebar = true;
	addMove(move);
	
	// Jam 236S~H
	move = MoveInfo(CHARACTER_TYPE_JAM, "SenriShinshou");
	move.displayName = "Senri Shinshou";
	move.slangName = "H Puffball";
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "HaseiSenriShinshou");
	move.displayName = "Senri Shinshou (Follow-up)";
	move.slangName = "H Puffball";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	// Jam 236S~S
	move = MoveInfo(CHARACTER_TYPE_JAM, "HyappoShinshou");
	move.displayName = "Hyappo Shinshou";
	move.slangName = "Puffball";
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	// Jam 236S~K
	move = MoveInfo(CHARACTER_TYPE_JAM, "Ashibarai");
	move.displayName = "Hamonkyaku";
	move.slangName = "Splitkick";
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	addMove(move);
	
	// Jam 236S~P
	move = MoveInfo(CHARACTER_TYPE_JAM, "Mawarikomi");
	move.displayName = "Mawarikomi";
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	// Jam 46P
	move = MoveInfo(CHARACTER_TYPE_JAM, "TuikaA");
	move.displayName = "Zekkei";
	move.slangName = "46P";
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "TuikaAA");
	move.displayName = "Goushao";
	move.slangName = "46PP";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "TuikaAB");
	move.displayName = "Dowanga";
	move.slangName = "46PK";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "Youeikyaku");
	move.displayName = "j.2K";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "AsanagiB");
	move.displayName = "K Asanagi no Kokyuu";
	move.slangName = "K-Card";
	move.powerup = powerup_cardK;
	move.powerupExplanation = powerupExplanation_card;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "AsanagiC");
	move.displayName = "S Asanagi no Kokyuu";
	move.slangName = "S-Card";
	move.powerup = powerup_cardS;
	move.powerupExplanation = powerupExplanation_card;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "AsanagiD");
	move.displayName = "H Asanagi no Kokyuu";
	move.slangName = "H-Card";
	move.powerup = powerup_cardH;
	move.powerupExplanation = powerupExplanation_card;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "RyujinLand");
	move.displayName = "Ryuujin";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "RyujinAir");
	move.displayName = "Air Ryuujin";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "RyujinHasei");
	move.displayName = "Ryuujin";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "RyujinKyoukaLand");
	move.displayName = "Carded Ryuujin";
	move.displayNameSelector = displayNameSelector_ryujinLv2or3;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "RyujinKyoukaAir");
	move.displayName = "Carded Air Ryuujin";
	move.displayNameSelector = displayNameSelector_airRyujinLv2or3;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "RyujinKyoukaHasei");
	move.displayName = "Carded Ryuujin";
	move.displayNameSelector = displayNameSelector_ryujinLv2or3;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "GekirinLand");
	move.displayName = "Gekirin";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "GekirinAir");
	move.displayName = "Air Gekirin";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "GekirinHasei");
	move.displayName = "Gekirin";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "GekirinKyoukaLand");
	move.displayName = "Carded Gekirin";
	move.displayNameSelector = displayNameSelector_gekirinLv2or3;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "GekirinKyoukaAir");
	move.displayName = "Carded Air Gekirin";
	move.displayNameSelector = displayNameSelector_airGekirinLv2or3;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "GekirinKyoukaHasei");
	move.displayName = "Carded Gekirin";
	move.displayNameSelector = displayNameSelector_gekirinLv2or3;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "KenroukakuLand");
	move.displayName = "Kenroukaku";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "KenroukakuAir");
	move.displayName = "Air Kenroukaku";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "KenroukakuHasei");
	move.displayName = "Kenroukaku";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "KenroukakuKyoukaLand");
	move.displayName = "Carded Kenroukaku";
	move.displayNameSelector = displayNameSelector_kenroukakuLv2or3;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "KenroukakuKyoukaAir");
	move.displayName = "Carded Air Kenroukaku";
	move.displayNameSelector = displayNameSelector_airKenroukakuLv2or3;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "KenroukakuKyoukaHasei");
	move.displayName = "Carded Kenroukaku";
	move.displayNameSelector = displayNameSelector_kenroukakuLv2or3;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "Renhoukyaku");
	move.displayName = "Renhoukyaku";
	move.slangName = "Super Puffball";
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "RenhoukyakuObj", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 88;
	move.framebarName = "Renhoukyaku";
	move.framebarSlangName = "Super Puffball";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "Hououshou");
	move.displayName = "Choukyaku Hou'oushou";
	move.slangName = "Choukyaku";
	move.forceSuperHitAnyway = forceSuperHitAnyway_hououshou;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "HououshouBurst");
	move.displayName = "Burst Choukyaku Hou'oushou";
	move.slangName = "Burst Choukyaku";
	move.forceSuperHitAnyway = forceSuperHitAnyway_hououshou;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "Saishingeki");
	move.displayName = "Bao Saishinshou";
	move.slangName = "Bao";
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = isInVariableStartupSection_saishingeki;
	move.secondaryStartup = secondaryStartup_saishingeki;
	move.dontSkipSuper = true;
	move.showMultipleHitsFromOneAttack = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_Hold_End");
	move.displayName = "Savvy Ninpo: Seal of Approval Cancel";
	move.slangName = "Uncling";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_Atemi");
	move.displayName = "s.P";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_Stamp");
	move.displayName = "s.K";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_Gedan");
	move.displayName = "s.S";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_Chudan");
	move.displayName = "s.H";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_Atemi2");
	move.displayName = "Savvy Ninpo: Data Logging";
	move.slangName = "22P";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Meishi_WarpA");
	move.displayName = "S Business Ninpo: Under the Rug";
	move.slangName = "S Teleport";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Meishi_WarpB");
	move.displayName = "H Business Ninpo: Under the Rug";
	move.slangName = "H Teleport";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Zaneiken");
	move.displayName = "Resshou";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Meishi_ThrowA");
	move.displayName = "S Business Ninpo: Caltrops";
	move.slangName = "S Card";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Meishi_ThrowB");
	move.displayName = "H Business Ninpo: Caltrops";
	move.slangName = "H Card";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_set_LandA");
	move.displayName = "P Savvy Ninpo: Request for Approval";
	move.slangName = "P Scroll";
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.canYrcProjectile = canYrcProjectile_scroll;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_set_LandB");
	move.displayName = "K Savvy Ninpo: Request for Approval";
	move.slangName = "K Scroll";
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.canYrcProjectile = canYrcProjectile_scroll;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_set_LandC");
	move.displayName = "S Savvy Ninpo: Request for Approval";
	move.slangName = "S Scroll";
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.canYrcProjectile = canYrcProjectile_scroll;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_set_LandD");
	move.displayName = "H Savvy Ninpo: Request for Approval";
	move.slangName = "H Scroll";
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.canYrcProjectile = canYrcProjectile_scroll;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_set_AirA");
	move.displayName = "P Air Savvy Ninpo: Request for Approval";
	move.slangName = "Air P Scroll";
	move.canYrcProjectile = canYrcProjectile_scroll;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_set_AirB");
	move.displayName = "K Air Savvy Ninpo: Request for Approval";
	move.slangName = "Air K Scroll";
	move.canYrcProjectile = canYrcProjectile_scroll;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_set_AirC");
	move.displayName = "S Air Savvy Ninpo: Request for Approval";
	move.slangName = "Air S Scroll";
	move.canYrcProjectile = canYrcProjectile_scroll;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_set_AirD");
	move.displayName = "H Air Savvy Ninpo: Request for Approval";
	move.slangName = "Air H Scroll";
	move.canYrcProjectile = canYrcProjectile_scroll;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Mozu_Land");
	move.displayName = "Savvy Ninpo: Tax Write-off";
	move.slangName = "Izuna Drop";
	move.ignoreJumpInstalls = true;  // when you RC this move, you can only be on the ground
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Mozu_Land_Exe");
	move.displayName = "Savvy Ninpo: Tax Write-off";
	move.slangName = "Izuna Drop";
	move.combineWithPreviousMove = true;
	move.ignoreJumpInstalls = true;  // when you RC this move, you can only be on the ground
	move.isGrab = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Mozu_Air");
	move.displayName = "Air Savvy Ninpo: Tax Write-off";
	move.slangName = "Air Izuna Drop";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Mozu_Air_Exe");
	move.displayName = "Air Savvy Ninpo: Tax Write-off";
	move.slangName = "Air Izuna Drop";
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Meishi_Nin_JitsuA");
	move.displayName = "S Business Ninpo: Under the Bus";
	move.slangName = "S Clone";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Meishi_Nin_JitsuB");
	move.displayName = "H Business Ninpo: Under the Bus";
	move.slangName = "H Clone";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Human_Suriken");
	move.displayName = "Business Ultimate Ninpo: All Hands";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Human_SurikenExe");
	move.displayName = "Business Ultimate Ninpo: All Hands";
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Meishi_Meteor");
	move.displayName = "Air Dead Stock Ninpo: Firesale";
	move.slangName = "Air Firesale";
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Meteor", true);
	move.framebarName = "Air Dead Stock Ninpo: Firesale";
	move.framebarSlangName = "Air Firesale";
	move.framebarNameUncombined = "Air Dead Stock Ninpo: Firesale Card";
	move.framebarSlangNameUncombined = "Firesale Card";
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 113;
	move.combineHitsFromDifferentProjectiles = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Suriken", true);
	move.framebarName = "Air Dead Stock Ninpo: Firesale";
	move.framebarSlangName = "Air Firesale";
	move.framebarNameUncombined = "Air Dead Stock Ninpo: Firesale Shuriken";
	move.framebarSlangNameUncombined = "Shuriken";
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 113;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Royal_Straight_Flush");
	move.displayName = "Dead Stock Ninpo: Firesale";
	move.slangName = "Firesale";
	move.dontSkipSuper = true;
	move.createdProjectile = createdProjectile_firesale;
	move.canYrcProjectile = canYrcProjectile_firesale;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Royal_Straight_Flush_Burst");
	move.displayName = "Burst Dead Stock Ninpo: Firesale";
	move.slangName = "Burst Firesale";
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_default;
	move.createdProjectile = createdProjectile_firesale;
	move.canYrcProjectile = canYrcProjectile_firesale;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	// Answer scroll cling idle. Happens from an s.D if not holding D
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_Move_Hold");
	move.displayName = "Savvy Ninpo: Seal of Approval";
	move.slangName = "Scroll Cling";
	move.isIdle = isIdle_enableWhiffCancels;
	move.canBlock = canBlock_default;
	move.canBeUnableToBlockIndefinitelyOrForVeryLongTime = true;
	addMove(move);
	
	// Answer scroll cling idle. Happens from an s.D if not holding Special
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_Move_Hold_S");
	move.displayName = "Savvy Ninpo: Seal of Approval";
	move.slangName = "Scroll Cling";
	move.isIdle = isIdle_enableWhiffCancels;
	move.canBlock = canBlock_default;
	move.canBeUnableToBlockIndefinitelyOrForVeryLongTime = true;
	addMove(move);
	
	// Answer scroll cling idle. Happens from jumping at it or 22P'ing at it
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_Hold");
	move.displayName = "Savvy Ninpo: Seal of Approval";
	move.slangName = "Scroll Cling";
	move.isIdle = isIdle_enableWhiffCancels;
	move.canBlock = canBlock_default;
	move.canBeUnableToBlockIndefinitelyOrForVeryLongTime = true;
	addMove(move);
	
	// Answer 1sD
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_Move1");
	move.displayName = "1s.D";
	move.nameIncludesInputs = true;
	move.isIdle = alwaysFalse;
	move.isInVariableStartupSection = isInVariableStartupSection_amiMove;
	move.considerVariableStartupAsStanceForFramebar = true;
	addMove(move);
	
	// Answer 2sD
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_Move2");
	move.displayName = "2s.D";
	move.nameIncludesInputs = true;
	move.isIdle = alwaysFalse;
	move.isInVariableStartupSection = isInVariableStartupSection_amiMove;
	move.considerVariableStartupAsStanceForFramebar = true;
	addMove(move);
	
	// Answer 3sD
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_Move3");
	move.displayName = "3s.D";
	move.nameIncludesInputs = true;
	move.isIdle = alwaysFalse;
	move.isInVariableStartupSection = isInVariableStartupSection_amiMove;
	move.considerVariableStartupAsStanceForFramebar = true;
	addMove(move);
	
	// Answer 4sD
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_Move4");
	move.displayName = "4s.D";
	move.nameIncludesInputs = true;
	move.isIdle = alwaysFalse;
	move.isInVariableStartupSection = isInVariableStartupSection_amiMove;
	move.considerVariableStartupAsStanceForFramebar = true;
	addMove(move);
	
	// Answer sD
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_MoveD");
	move.displayName = "s.D";
	move.nameIncludesInputs = true;
	move.isIdle = alwaysFalse;
	move.isInVariableStartupSection = isInVariableStartupSection_amiMove;
	move.considerVariableStartupAsStanceForFramebar = true;
	addMove(move);
	
	// Answer 6sD
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_Move6");
	move.displayName = "6s.D";
	move.nameIncludesInputs = true;
	move.isIdle = alwaysFalse;
	move.isInVariableStartupSection = isInVariableStartupSection_amiMove;
	move.considerVariableStartupAsStanceForFramebar = true;
	addMove(move);
	
	// Answer 7sD
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_Move7");
	move.displayName = "7s.D";
	move.nameIncludesInputs = true;
	move.isIdle = alwaysFalse;
	move.isInVariableStartupSection = isInVariableStartupSection_amiMove;
	move.considerVariableStartupAsStanceForFramebar = true;
	addMove(move);
	
	// Answer 8sD
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_Move8");
	move.displayName = "8s.D";
	move.nameIncludesInputs = true;
	move.isIdle = alwaysFalse;
	move.isInVariableStartupSection = isInVariableStartupSection_amiMove;
	move.considerVariableStartupAsStanceForFramebar = true;
	addMove(move);
	
	// Answer 9sD
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_Move9");
	move.displayName = "9s.D";
	move.nameIncludesInputs = true;
	move.isIdle = alwaysFalse;
	move.isInVariableStartupSection = isInVariableStartupSection_amiMove;
	move.considerVariableStartupAsStanceForFramebar = true;
	addMove(move);
	
	// Answer sD
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_Move");
	move.displayName = "s.D";
	move.nameIncludesInputs = true;
	move.isIdle = alwaysFalse;
	move.isInVariableStartupSection = isInVariableStartupSection_amiMove;
	move.considerVariableStartupAsStanceForFramebar = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami", true);
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Meishi", true);
	move.isDangerous = isDangerous_card;
	move.framebarId = 103;
	move.framebarName = "BN: Caltrops";
	move.framebarSlangName = "Card";
	move.framebarNameFull = "Business Ninpo: Caltrops";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Nin_Jitsu", true);
	move.isDangerous = isDangerous_not_NullWhileActive;
	move.framebarId = 104;
	move.framebarName = "BN: Under the Bus";
	move.framebarSlangName = "Clone";
	move.framebarNameFull = "Business Ninpo: Under the Bus";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "RSF_Start", true);
	move.isDangerous = isDangerous_card;
	move.framebarId = ANSWER_RSF_FRAMEBAR_ID;
	move.framebarName = "Dead Stock Ninpo: Firesale";
	move.framebarSlangName = "Firesale";
	move.framebarSlangNameUncombined = "Firesale Start";
	move.framebarNameUncombined = "Dead Stock Ninpo: Firesale Start";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "RSF_Meishi", true);
	move.isDangerous = isDangerous_rsfMeishi;
	move.framebarId = ANSWER_RSF_FRAMEBAR_ID;
	move.framebarName = "Dead Stock Ninpo: Firesale";
	move.framebarSlangName = "Firesale";
	move.framebarSlangNameUncombined = "Firesale Card";
	move.framebarNameUncombined = "Dead Stock Ninpo: Firesale Card";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "RSF_Finish", true);
	move.isDangerous = isDangerous_rsfMeishi;
	move.framebarId = ANSWER_RSF_FRAMEBAR_ID;
	move.framebarName = "Dead Stock Ninpo: Firesale";
	move.framebarSlangName = "Firesale";
	move.framebarSlangNameUncombined = "Shuriken";
	move.framebarNameUncombined = "Dead Stock Ninpo: Firesale Shuriken";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "LustShakerRenda");
	move.displayName = "Mash Lust Shaker";
	move.slangName = "Mash Shaker";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	// Millia Roll Roll
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "SaiZenten");
	move.displayName = "Forward Roll Again";
	move.slangName = "Doubleroll";
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	// Millia Roll > S
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "ZentenShaker");
	move.displayName = "Lust Shaker (Follow-up)";
	move.slangName = "> Shaker";
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	// Millia Roll > H
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "Digitalis");
	move.displayName = "Digitalis";
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	// represents both S and H pins
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "SilentForceKnife", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 14;
	move.framebarName = "Silent Force";
	move.framebarSlangName = "Pin";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	// s-disc
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "TandemTopC");
	move.displayName = "S Tandem Top";
	move.slangName = "S-Disc";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	// s-disc
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "TandemTopCRing", true);
	move.isDangerous = isDangerous_notNull;
	move.framebarId = 15;
	move.framebarName = "S Tandem Top";
	move.framebarSlangName = "S-Disc";
	addMove(move);
	
	// h-disc
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "TandemTopD");
	move.displayName = "H Tandem Top";
	move.framebarSlangName = "H-Disc";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	// h-disc
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "TandemTopDRing", true);
	move.isDangerous = isDangerous_notNull;
	move.framebarId = 16;
	move.framebarName = "H Tandem Top";
	move.framebarSlangName = "H-Disc";
	addMove(move);
	
	// Bad Moon does not get a height buff in Rev1
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "BadMoon");
	move.displayName = "Bad Moon";
	move.displayNameSelector = displayNameSelector_badMoon;
	move.slangName = "BM";
	move.displaySlangNameSelector = displaySlangNameSelector_badMoon;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "SecretGarden");
	move.displayName = "Secret Garden";
	move.slangName = "Garden";
	move.ignoreJumpInstalls = true;
	move.powerup = powerup_secretGarden;
	move.powerupExplanation = powerupExplanation_secretGarden;
	move.dontShowPowerupGraphic = alwaysTrue;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "LustShaker");
	move.displayName = "Lust Shaker";
	move.slangName = "Shaker";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "Zenten");
	move.displayName = "Forward Roll";
	move.slangName = "Roll";
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = hasWhiffCancels;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "IronSavior");
	move.displayName = "Iron Savior";
	move.slangName = "Haircar";
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "SilentForce2");
	move.displayName = "H Silent Force";
	move.slangName = "H-Pin";
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "SilentForce");
	move.displayName = "S Silent Force";
	move.slangName = "S-Pin";
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "KousokuRakka");
	move.displayName = "Turbo Fall";
	move.slangName = "TF";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "EmeraldRain");
	move.displayName = "Emerald Rain";
	move.slangName = "ER";
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "ChromingRose");
	move.displayName = "Chroming Rose";
	move.slangName = "Rose Install";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "Winger");
	move.displayName = "Winger";
	move.iKnowExactlyWhenTheRecoveryOfThisMoveIs = isRecovery_land;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "WingerBurst");
	move.displayName = "Burst Winger";
	move.iKnowExactlyWhenTheRecoveryOfThisMoveIs = isRecovery_land;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	// each ring of the 236236S super is separately named
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "EmeraldRainRing1", true);
	move.isDangerous = isDangerous_notNull;
	move.framebarId = 17;
	move.framebarName = "Emerald Rain";
	move.framebarSlangName = "ER";
	move.framebarSlangNameUncombined = "ER Ring1";
	move.framebarNameUncombined = "Emeral Rain Ring 1";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "EmeraldRainRing2", true);
	move.isDangerous = isDangerous_notNull;
	move.framebarId = 17;
	move.framebarName = "Emerald Rain";
	move.framebarSlangName = "ER";
	move.framebarSlangNameUncombined = "ER Ring2";
	move.framebarNameUncombined = "Emeral Rain Ring 2";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "EmeraldRainRing3", true);
	move.isDangerous = isDangerous_notNull;
	move.framebarId = 17;
	move.framebarName = "Emerald Rain";
	move.framebarSlangName = "ER";
	move.framebarSlangNameUncombined = "ER Ring3";
	move.framebarNameUncombined = "Emeral Rain Ring 3";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "SecretGardenBall", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 18;
	move.framebarName = "Secret Garden";
	move.framebarSlangName = "Garden";
	move.combineHitsFromDifferentProjectiles = true;
	addMove(move);
	
	// a rose created during Rose Install. Many of these can be on the screen at the same time
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "RoseObj", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 19;
	move.framebarName = "Rose";
	addMove(move);
	
	// Zato does not have a move that makes him airborne from the ground, so he never cares about super jump installs.
	// And if some move only cancels into specials or only leads to other moves that only cancel into specials, then
	// he doesn't care about jump installs either (Zato has no such moves)
	charDoesNotCareAboutSuperJumpInstalls[CHARACTER_TYPE_ZATO] = true;
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "NmlAtk6B");
	move.displayName = "6K";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;  // a dead end move
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "NmlAtk6D");
	move.displayName = "6H";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;  // a dead end move
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "EddieMegalithHead", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarName = "Great White";
	move.framebarId = 109;
	move.isEddie = true;
	move.showMultipleHitsFromOneAttack = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "ChouDoriru", true);
	move.isDangerous = isDangerous_notNull;
	move.framebarName = "Giga Drill";
	move.framebarId = 109;
	move.isEddie = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "KageDamari", true);
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "EddieA", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarName = "Eddie P";
	move.framebarId = 109;
	move.drawProjectileOriginPoint = true;
	move.isEddie = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "EddieB", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarName = "Eddie K";
	move.framebarId = 109;
	move.drawProjectileOriginPoint = true;
	move.isEddie = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "EddieC", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarName = "Eddie S";
	move.framebarId = 109;
	move.drawProjectileOriginPoint = true;
	move.isEddie = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "EddieD", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarName = "Eddie H";
	move.framebarId = 109;
	move.drawProjectileOriginPoint = true;
	move.isEddie = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "EddieE", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarName = "Eddie D";
	move.framebarId = 109;
	move.drawProjectileOriginPoint = true;
	move.isEddie = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "Eddie4", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarName = "Eddie 4";
	move.framebarId = 109;
	move.drawProjectileOriginPoint = true;
	move.isEddie = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "Eddie", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarName = "Eddie";
	move.framebarId = 109;
	move.drawProjectileOriginPoint = true;
	move.isEddie = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "Eddie6", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarName = "Eddie 6";
	move.framebarId = 109;
	move.drawProjectileOriginPoint = true;
	move.isEddie = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "Fly");
	move.displayName = "Flight";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "InviteHellC");
	move.displayName = "S Invite Hell";
	move.slangName = "S-Drill";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "InviteHellD");
	move.displayName = "H Invite Hell";
	move.slangName = "H-Drill";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "EddieSummonD");
	move.displayName = "Summon Eddie Shadow Dive";
	move.slangName = "Summon";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "EddieSummonC");
	move.displayName = "Summon Eddie Anti-air Attack";
	move.slangName = "Nobiru";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "EddieSummonB");
	move.displayName = "Summon Eddie Traversing Attack";
	move.slangName = "Mawaru";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "EddieSummonA");
	move.displayName = "Summon Eddie Small Attack";
	move.slangName = "P Summon";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "EddieReturn");
	move.displayName = "Recall Eddie";
	move.slangName = "Recall";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "EddieSummonD2");
	move.displayName = "Shadow Puddle Eddie Summon";
	move.slangName = "Puddle Summon";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "DrunkerdShade");
	move.displayName = "Drunkard Shade";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "DamnedFang");
	move.displayName = "Damned Fang";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "DamnedFangExe");
	move.displayName = "Damned Fang";
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "ShadowGallary");
	move.displayName = "Shadow Gallery";
	move.slangName = "SG";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "Executer");
	move.displayName = "Executor";
	move.iKnowExactlyWhenTheRecoveryOfThisMoveIs = isRecovery_land;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "Amorphous");
	move.displayName = "Amorphous";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "AmorphousBurst");
	move.displayName = "Burst Amorphous";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "MegalithHead");
	move.displayName = "Great White";
	move.dontSkipSuper = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "MegalithHead2");
	move.displayName = "Great White";
	move.dontSkipSuper = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	// Zato 214K
	move = MoveInfo(CHARACTER_TYPE_ZATO, "BreakTheLaw");
	move.displayName = "Break the Law";
	move.sectionSeparator = sectionSeparator_breakTheLaw;
	move.zatoHoldLevel = zatoHoldLevel_breakTheLaw;
	move.isInVariableStartupSection = isInVariableStartupSection_breakTheLaw;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "DrillC", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 20;
	move.framebarName = "Invite Hell";
	move.framebarSlangName = "Drill";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "DrillD", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 20;
	move.framebarName = "Invite Hell";
	move.framebarSlangName = "Drill";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "AmorphousObj", true);
	move.isDangerous = isDangerous_amorphous;
	move.framebarId = 21;
	move.framebarName = "Amorphous";
	move.framebarSlangNameUncombined = "Amorphous";
	move.framebarNameUncombined = "Amorphous";
	addMove(move);
	
	// this can only be created on the boss version of Zato
	move = MoveInfo(CHARACTER_TYPE_ZATO, "AmorphousObj2", true);
	move.isDangerous = isDangerous_not_hasHitNumButNoHitboxes;
	move.framebarId = 21;
	move.framebarName = "Amorphous";
	move.framebarSlangNameUncombined = "Amorphous2";
	move.framebarNameUncombined = "Amorphous Hit 2";
	addMove(move);
	
	// Potemkin does not care about super jump installs at all because he will never make use of the airdash that he gets,
	// but that is already handled in our code that registers the airdash install, because it won't trigger a super jump install,
	// if the airdash count is already equal to the maximum, which is 0, so we don't even need a = true here.
	charDoesNotCareAboutSuperJumpInstalls[CHARACTER_TYPE_POTEMKIN] = true;
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "NmlAtk2E");
	move.displayName = "2D";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "SlideHead");
	move.displayName = "Slide Head";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "MegaFistFront");
	move.displayName = "Forward Megafist";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "MegaFistBack");
	move.displayName = "Back Megafist";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "HeatKnucle");
	move.displayName = "Heat Knuckle";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "HammerFall");
	move.displayName = "Hammer Fall";
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = hasWhiffCancels;
	move.partOfStance = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "HeatExtend");
	move.displayName = "Heat Extend";
	move.isGrab = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "PotemkinBuster");
	move.displayName = "Potemkin Buster";
	move.slangName = "Potbuster";
	move.frontLegInvul = frontLegInvul_potemkinBuster;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "PotemkinBusterExe");
	move.displayName = "Potemkin Buster";
	move.slangName = "Potbuster";
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "Ichigeki");
	move.displayName = "Instant Kill";
	move.slangName = "IK";
	move.frontLegInvul = frontLegInvul_potemkinBuster;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	// Potemkin Flick
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "FDB");
	move.displayName = "F.D.B.";
	move.sectionSeparator = sectionSeparator_FDB;
	move.isInVariableStartupSection = isInVariableStartupSection_fdb;
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "FDB_obj", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarName = "F.D.B.";
	move.framebarId = 110;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "Anti_AirExplode");
	move.displayName = "Trishula";
	move.canYrcProjectile = canYrcProjectile_prevNoLinkDestroyOnStateChange;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "RocketDive");
	move.displayName = "I.C.P.M.";
	move.dontSkipGrab = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "RocketDiveExe");
	move.displayName = "I.C.P.M.";
	move.combineWithPreviousMove = true;
	move.dontSkipGrab = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "HeavenlyPBuster");
	move.displayName = "Heavenly Potemkin Buster";
	move.slangName = "HPB";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "HeavenlyPBusterBurst");
	move.displayName = "Burst Heavenly Potemkin Buster";
	move.slangName = "Burst HPB";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "HeavenlyPBusterExe");
	move.displayName = "Heavenly Potemkin Buster";
	move.slangName = "HPB";
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "HammerFallBrake");
		move.displayName = "Hammer Fall Break";
	move.slangName = "HFB";
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.partOfStance = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "SlideHead_Obj", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 22;
	move.framebarName = "Slide Head";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "FDB_Obj", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 23;
	move.framebarName = "FDB";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "Giganter");
	move.displayName = "Giganter Kai";
	move.slangName = "Giganter";
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "GiganObj", true);
	move.isDangerous = isDangerous_notNull;
	move.framebarId = 24;
	move.framebarName = "Giganter";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "GiganticBullet");
	move.displayName = "Gigantic Bullet Kai";
	move.slangName = "Bullet";
	move.dontSkipSuper = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "Bomb", true);
	move.isDangerous = isDangerous_not_hasHitNumButNoHitboxes;
	move.framebarId = 25;
	move.framebarName = "Trishula";
	addMove(move);
	
	// Venom has only one special move that can make him airborne: Teleport.
	// Which means he will never care about super jump installs.
	charDoesNotCareAboutSuperJumpInstalls[CHARACTER_TYPE_VENOM] = true;
	
	// not ignoring jump installs here, because it can whiff cancel into teleport and normally you can't
	// double jump from it, but if you jump install, you can.
	// You do get a guaranteed airdash from teleport, so we will only ignore super jump installs
	move = MoveInfo(CHARACTER_TYPE_VENOM, "BallSeiseiA");
	move.displayName = "P Ball Set";
	move.slangName = "P Ball";
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.addForceAddWhiffCancel("WarpB");
	move.addForceAddWhiffCancel("BallSeiseiB_Hold");
	move.addForceAddWhiffCancel("BallSeiseiC_Hold");
	move.addForceAddWhiffCancel("BallSeiseiD_Hold");
	move.conditionForAddingWhiffCancels = hasWhiffCancels;
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "BallSeiseiA_Hold");
	move.displayName = "P Ball Set";
	move.slangName = "P Ball";
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.addForceAddWhiffCancel("WarpB");
	move.addForceAddWhiffCancel("BallSeiseiB_Hold");
	move.addForceAddWhiffCancel("BallSeiseiC_Hold");
	move.addForceAddWhiffCancel("BallSeiseiD_Hold");
	move.conditionForAddingWhiffCancels = hasWhiffCancels;
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "BallSeiseiB");
	move.displayName = "K Ball Set";
	move.slangName = "K Ball";
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.addForceAddWhiffCancel("WarpB");
	move.addForceAddWhiffCancel("BallSeiseiA_Hold");
	move.addForceAddWhiffCancel("BallSeiseiC_Hold");
	move.addForceAddWhiffCancel("BallSeiseiD_Hold");
	move.conditionForAddingWhiffCancels = hasWhiffCancels;
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "BallSeiseiB_Hold");
	move.displayName = "K Ball Set";
	move.slangName = "K Ball";
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.addForceAddWhiffCancel("WarpB");
	move.addForceAddWhiffCancel("BallSeiseiA_Hold");
	move.addForceAddWhiffCancel("BallSeiseiC_Hold");
	move.addForceAddWhiffCancel("BallSeiseiD_Hold");
	move.conditionForAddingWhiffCancels = hasWhiffCancels;
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "BallSeiseiC");
	move.displayName = "S Ball Set";
	move.slangName = "S Ball";
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.addForceAddWhiffCancel("WarpB");
	move.addForceAddWhiffCancel("BallSeiseiA_Hold");
	move.addForceAddWhiffCancel("BallSeiseiB_Hold");
	move.addForceAddWhiffCancel("BallSeiseiD_Hold");
	move.conditionForAddingWhiffCancels = hasWhiffCancels;
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "BallSeiseiC_Hold");
	move.displayName = "S Ball Set";
	move.slangName = "S Ball";
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.addForceAddWhiffCancel("WarpB");
	move.addForceAddWhiffCancel("BallSeiseiA_Hold");
	move.addForceAddWhiffCancel("BallSeiseiB_Hold");
	move.addForceAddWhiffCancel("BallSeiseiD_Hold");
	move.conditionForAddingWhiffCancels = hasWhiffCancels;
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "BallSeiseiD");
	move.displayName = "H Ball Set";
	move.slangName = "H Ball";
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.addForceAddWhiffCancel("WarpB");
	move.addForceAddWhiffCancel("BallSeiseiA_Hold");
	move.addForceAddWhiffCancel("BallSeiseiB_Hold");
	move.addForceAddWhiffCancel("BallSeiseiC_Hold");
	move.conditionForAddingWhiffCancels = hasWhiffCancels;
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);

	move = MoveInfo(CHARACTER_TYPE_VENOM, "BallSeiseiD_Hold");
	move.displayName = "H Ball Set";
	move.slangName = "H Ball";
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.addForceAddWhiffCancel("WarpB");
	move.addForceAddWhiffCancel("BallSeiseiA_Hold");
	move.addForceAddWhiffCancel("BallSeiseiB_Hold");
	move.addForceAddWhiffCancel("BallSeiseiC_Hold");
	move.conditionForAddingWhiffCancels = hasWhiffCancels;
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "AirBallSeiseiA");
	move.displayName = "Air P Ball Set";
	move.slangName = "Air P Ball";
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "AirBallSeiseiB");
	move.displayName = "Air K Ball Set";
	move.slangName = "Air K Ball";
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "AirBallSeiseiC");
	move.displayName = "Air S Ball Set";
	move.slangName = "Air S Ball";
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "AirBallSeiseiD");
	move.displayName = "Air H Ball Set";
	move.slangName = "Air H Ball";
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "MadStrugguleD");
	move.displayName = "H Mad Struggle";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "MadStrugguleC");
	move.displayName = "S Mad Struggle";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "Warp");
	move.displayName = "Teleport";
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "WarpB");
	move.displayName = "Teleport";
	move.replacementInputs = "Hold the button you set the ball with";
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "DoubleHeadMorbidD");
	move.displayName = "H Double Head Morbid";
	move.slangName = "HDHM";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "DoubleHeadMorbidC");
	move.displayName = "S Double Head Morbid";
	move.slangName = "SDHM";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "StingerAimD");
	move.displayName = "H Stinger Aim";
	move.displayNameSelector = displayNameSelector_stingerH;
	move.sectionSeparator = sectionSeparator_stingerH;
	move.isInVariableStartupSection = isInVariableStartupSection_stinger;
	move.slangName = "H Stinger";
	move.displaySlangNameSelector = displaySlangNameSelector_stingerH;
	move.canYrcProjectile = canYrcProjectile_default;
	move.powerup = powerup_stingerH;
	move.powerupExplanation = powerupExplanation_stinger;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "StingerAimC");
	move.displayName = "S Stinger Aim";
	move.displayNameSelector = displayNameSelector_stingerS;
	move.sectionSeparator = sectionSeparator_stingerS;
	move.isInVariableStartupSection = isInVariableStartupSection_stinger;
	move.slangName = "S Stinger";
	move.displaySlangNameSelector = displaySlangNameSelector_stingerS;
	move.canYrcProjectile = canYrcProjectile_default;
	move.powerup = powerup_stingerS;
	move.powerupExplanation = powerupExplanation_stinger;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "CarcassRaidD");
	move.displayName = "H Carcass Raid";
	move.displayNameSelector = displayNameSelector_carcassRaidH;
	move.slangName = "H Carcass";
	move.displaySlangNameSelector = displaySlangNameSelector_carcassRaidH;
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "CarcassRaidC");
	move.displayName = "S Carcass Raid";
	move.displayNameSelector = displayNameSelector_carcassRaidS;
	move.slangName = "S Carcass";
	move.displaySlangNameSelector = displaySlangNameSelector_carcassRaidS;
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	// Venom QV
	move = MoveInfo(CHARACTER_TYPE_VENOM, "DubiousCurveA");
	move.displayName = "P QV";
	move.sectionSeparator = sectionSeparator_QV;
	move.isInVariableStartupSection = isInVariableStartupSection_qv;
	move.canYrcProjectile = canYrcProjectile_qv;
	move.powerup = powerup_qv;
	move.powerupExplanation = powerupExplanation_qvA;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "DubiousCurveB");
	move.displayName = "K QV";
	move.sectionSeparator = sectionSeparator_QV;
	move.isInVariableStartupSection = isInVariableStartupSection_qv;
	move.canYrcProjectile = canYrcProjectile_qv;
	move.powerup = powerup_qv;
	move.powerupExplanation = powerupExplanation_qvB;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "DubiousCurveC");
	move.displayName = "S QV";
	move.sectionSeparator = sectionSeparator_QV;
	move.isInVariableStartupSection = isInVariableStartupSection_qv;
	move.canYrcProjectile = canYrcProjectile_qv;
	move.powerup = powerup_qv;
	move.powerupExplanation = powerupExplanation_qvC;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "DubiousCurveD");
	move.displayName = "H QV";
	move.sectionSeparator = sectionSeparator_QV;
	move.isInVariableStartupSection = isInVariableStartupSection_qv;
	move.canYrcProjectile = canYrcProjectile_qv;
	move.powerup = powerup_qv;
	move.powerupExplanation = powerupExplanation_qvD;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "RedHail");
	move.displayName = "Red Hail";
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	// this is Stinger and Carcass Raid balls, ball set, including when such balls are launched.
	// Charged balls and even Bishop Runout and Red Hail are also this
	move = MoveInfo(CHARACTER_TYPE_VENOM, "Ball", true);
	move.isDangerous = isDangerous_active;
	move.framebarId = 49;
	move.framebarName = "Balls";
	move.framebarNameSelector = framebarNameSelector_venomBall;
	move.framebarSlangName = "Balls";
	move.framebarSlangNameSelector = framebarSlangNameSelector_venomBall;
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	// every QV when released creates this shockwave and it persists on RC
	move = MoveInfo(CHARACTER_TYPE_VENOM, "Debious_AttackBall", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 50;
	move.framebarName = "QV";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "DarkAngel");
	move.displayName = "Dark Angel";
	move.slangName = "DA";
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "DarkAngelBurst");
	move.displayName = "Burst Dark Angel";
	move.slangName = "Burst DA";
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "SummonGoldBall");
	move.displayName = "Bishop Runout";
	move.slangName = "Bishop";
	move.dontSkipSuper = true;
	move.createdProjectile = createdProjectile_bishop;
	move.canYrcProjectile = canYrcProjectile_bishop;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	// created before Dark Angel comes out
	move = MoveInfo(CHARACTER_TYPE_VENOM, "DarkAngelBallStart", true);
	move.isDangerous = isDangerous_hasNotCreatedAnythingYet;
	move.framebarId = 51;
	move.framebarName = "Dark Angel";
	move.framebarSlangName = "DA";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "DarkAngelBall", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 51;
	move.framebarName = "Dark Angel";
	move.framebarSlangName = "DA";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "NmlAtk6B");
	move.displayName = "6K";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "NmlAtk2D");
	move.displayName = "2H";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "NmlAtk6A");
	move.displayName = "6P";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "NmlAtk2E");
	move.displayName = "2D";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	// The only moves that you get from Dandy Steps that make you airborne are Helter-Skelter and Crosswise Heel,
	// and those give you an airdash anyway, so there is no need to care about super jump installs.
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "DandyStepA");
	move.displayName = "P Dandy Step";
	move.slangName = "P-Dandy";
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = hasWhiffCancels;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "DandyStepB");
	move.displayName = "K Dandy Step";
	move.slangName = "K-Dandy";
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = hasWhiffCancels;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	// Slayer dandy step follow-ups
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "CrossWise");
	move.displayName = "Crosswise Heel";
	move.displayNameSelector = displayNameSelector_crosswise;
	move.slangName = "CW";
	move.displaySlangNameSelector = displaySlangNameSelector_crosswise;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination =  true;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "UnderPressure");
	move.displayName = "Under Pressure";
	move.displayNameSelector = displayNameSelector_underPressure;
	move.slangName = "UP";
	move.displaySlangNameSelector = displaySlangNameSelector_underPressure;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination =  true;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "ItsLate");
	move.displayName = "It's Late";
	move.slangName = "IL";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "RetroFire");
	move.displayName = "Helter Skelter";
	move.slangName = "Helter";
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination =  true;
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "Retro", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 52;
	move.framebarName = "Helter Skelter";
	move.framebarSlangName = "Helter";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "PileBunker");
	move.displayName = "Pilebunker";
	move.displayNameSelector = displayNameSelector_pilebunker;
	move.slangName = "Pile";
	move.displaySlangNameSelector = displaySlangNameSelector_pilebunker;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination =  true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "6BFeint");
	move.displayName = "6K Feint";
	move.combineWithPreviousMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "MappaHunchA");
	move.displayName = "P Mappa Hunch";
	move.slangName = "P Mappa";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "MappaHunchB");
	move.displayName = "K Mappa Hunch";
	move.slangName = "K Mappa";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "FootLoose");
	move.displayName = "Footloose Journey";
	move.slangName = "Footloose";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "ChiwosuuUchuu");
	move.displayName = "Bloodsucking Universe";
	move.slangName = "Bite";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "ChiwosuuUchuuExe");
	move.displayName = "Bloodsucking Universe";
	move.slangName = "Bite";
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "UnderTow");
	move.displayName = "Undertow";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "ChokkagataDandy");
	move.displayName = "Straight-Down Dandy";
	move.slangName = "SDD";
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "EienNoTsubasa");
	move.displayName = "Eternal Wings";
	move.slangName = "EW";
	move.iKnowExactlyWhenTheRecoveryOfThisMoveIs = isRecovery_land;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "DeadOnTime");
	move.displayName = "Dead on Time";
	move.slangName = "DoT";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "DeadOnTimeBurst");
	move.displayName = "Burst Dead on Time";
	move.slangName = "BDoT";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "KetsuFire", true);
	move.isDangerous = isDangerous_not_hasHitNumButNoHitboxes;
	move.framebarId = 53;
	move.framebarName = "Straight-Down Dandy";
	move.framebarSlangName = "SDD";
	addMove(move);
	
	// I-No has only the following moves that can make her airborne:
	// 1) HCL
	// 2) VCL
	// 3) Sterilization Method
	// 4) Dash cancels
	// Dash cancels restore all air options.
	// HCL and VCL gift you a free airdash.
	// Sterilization Method on hit gives you all air options.
	// Sterilization Method does not give neither airdashes, nor double jumps on whiff.
	// Unfortunately this means we must show super jump installs on I-No.
	
	move = MoveInfo(CHARACTER_TYPE_INO, "NmlAtk5E");
	move.displayName = "5D";
	move.nameIncludesInputs = true;
	move.createdProjectile = createdProjectile_ino5D;
	move.canYrcProjectile = canYrcProjectile_ino5D;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "AirFDash_Under");
	move.displayName = "Downwards Dash";
	move.slangName = "Hoverdown";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "KouutsuOnkai");
	move.displayName = "Antidepressant Scale";
	move.slangName = MOVE_NAME_NOTE;
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "KouutsuOnkaiAir");
	move.displayName = "Air Antidepressant Scale";
	move.slangName = "Air Note";
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	// I-No Sultry Performance
	move = MoveInfo(CHARACTER_TYPE_INO, "KyougenA");
	move.displayName = "P Sultry Performance";
	move.slangName = "P-Dive";
	move.sectionSeparator = sectionSeparator_sultryPerformance;
	move.isIdle = isIdle_enableWhiffCancels;
	move.canBlock = canBlock_default;
	move.isInVariableStartupSection = isInVariableStartupSection_inoDivekick;
	move.powerup = powerup_kyougenA;
	move.powerupExplanation = powerupExplanation_kyougenA;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "KyougenB");
	move.displayName = "K Sultry Performance";
	move.slangName = "K-Dive";
	move.sectionSeparator = sectionSeparator_sultryPerformance;
	move.isIdle = isIdle_enableWhiffCancels;
	move.canBlock = canBlock_default;
	move.isInVariableStartupSection = isInVariableStartupSection_inoDivekick;
	move.powerup = powerup_kyougenB;
	move.powerupExplanation = powerupExplanation_kyougenB;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "KyougenC");
	move.displayName = "S Sultry Performance";
	move.slangName = "S-Dive";
	move.sectionSeparator = sectionSeparator_sultryPerformance;
	move.isIdle = isIdle_enableWhiffCancels;
	move.canBlock = canBlock_default;
	move.isInVariableStartupSection = isInVariableStartupSection_inoDivekick;
	move.powerup = powerup_kyougenC;
	move.powerupExplanation = powerupExplanation_kyougenC;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "KyougenD");
	move.displayName = "H Sultry Performance";
	move.slangName = "H-Dive";
	move.sectionSeparator = sectionSeparator_sultryPerformance;
	move.isIdle = isIdle_enableWhiffCancels;
	move.canBlock = canBlock_default;
	move.isInVariableStartupSection = isInVariableStartupSection_inoDivekick;
	move.powerup = powerup_kyougenD;
	move.powerupExplanation = powerupExplanation_kyougenD;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "CommandThrow");
	move.displayName = "Sterilization Method";
	move.slangName = "SM";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "CommandThrowExe");
	move.displayName = "Sterilization Method";
	move.slangName = "SM";
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "AirCommandThrow");
	move.displayName = "Air Sterilization Method";
	move.slangName = "Air SM";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "AirCommandThrowExe");
	move.displayName = "Air Sterilization Method";
	move.slangName = "Air SM";
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "TaibokuC");
	move.displayName = "S Strike the Big Tree";
	move.slangName = "S-STBT";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "TaibokuD");
	move.displayName = "H Strike the Big Tree";
	move.slangName = "H-STBT";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "ChemicalB");
	move.displayName = "Chemical Love";
	move.slangName = "HCL";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "ChemicalAirB");
	move.displayName = "Air Chemical Love";
	move.slangName = "Air HCL";
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "ChemicalC");
	move.displayName = "Vertical Chemical Love";
	move.slangName = "VCL";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "ChemicalAirC");
	move.displayName = "Air Vertical Chemical Love";
	move.slangName = "Air VCL";
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "ChemicalAdd");
	move.displayName = "Chemical Love (Follow-up)";
	move.slangName = "214K~214S";
	move.combineWithPreviousMove = true;
	move.butForFramebarDontCombineWithPreviousMove = true;
	move.canYrcProjectile = canYrcProjectile_default;  // typically opponent will be in blockstun, but I did a hacktest and ye you can YRC this
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "Madogiwa");
	move.displayName = "Longing Desperation";
	move.slangName = "Desperation";
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);

	move = MoveInfo(CHARACTER_TYPE_INO, "MadogiwaBurst");
	move.displayName = "Burst Longing Desperation";
	move.slangName = "Burst Desperation";
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "Genkai");
	move.displayName = "Ultimate Fortissimo";
	move.slangName = "Fortissimo";
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "BChemiLaser", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 54;
	move.framebarName = "Chemical Love";
	move.framebarSlangName = "HCL";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "AddChemiLaser", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 54;
	move.framebarName = "Chemical Love (Follow-up)";
	move.framebarSlangName = "214K~214S";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "CChemiLaser", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 54;
	move.framebarName = "Vertical Chemical Love";
	move.framebarSlangName = "VCL";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "Onpu", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 55;
	move.framebarName = "Antidepressant Scale";
	move.framebarSlangName = "Note";
	move.projectilePowerup = powerup_onpu;
	addMove(move);
	
	// Boss version only
	move = MoveInfo(CHARACTER_TYPE_INO, "Onpu2", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 55;
	move.framebarName = "Antidepressant Scale";
	move.framebarSlangName = "Note";
	move.projectilePowerup = powerup_onpu;
	addMove(move);
	
	// Boss version only
	move = MoveInfo(CHARACTER_TYPE_INO, "Onpu3", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 55;
	move.framebarName = "Antidepressant Scale";
	move.framebarSlangName = "Note";
	move.projectilePowerup = powerup_onpu;
	addMove(move);
	
	// cannot be YRC'd in Rev1
	move = MoveInfo(CHARACTER_TYPE_INO, "DustObjShot", true);
	move.isDangerous = isDangerous_playerInRCOrHasActiveFlag_AndNotInRecovery;
	move.framebarId = 56;
	move.framebarName = "5D";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "GenkaiObj", true);
	move.isDangerous = isDangerous_not_hasHitNumButNoHitboxes;
	move.framebarId = 57;
	move.framebarName = "Ultimate Fortissimo";
	move.framebarSlangName = "Fortissimo";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "MadogiwaObj", true);
	move.isDangerous = isDangerous_not_hasHitNumButNoHitboxes;
	move.framebarId = 58;
	move.framebarName = "Longing Desperation";
	move.framebarSlangName = "Desperation";
	addMove(move);
	
	// The only move that can put Bedman into the air is Task C, and he gets a free airdash from it.
	// So, Bedman does not care about super jump installs.
	// Bedman does not care about regular jump installs either, as he has no double jump - it's a
	// hover, which works even when he has no double jumps.
	charDoesNotCareAboutSuperJumpInstalls[CHARACTER_TYPE_BEDMAN] = true;
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "CmnActFDash");
	move.displayName = "Forward Dash";
	move.nameIncludesInputs = true;
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.considerVariableStartupAsStanceForFramebar = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "NmlAtk6D_2");
	move.displayName = "6H (Follow-up)";
	move.slangName = "6H~6H";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "CrouchFWalk");
	move.displayName = "Crouchwalk Forward";
	move.slangName = "3";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "CrouchBWalk");
	move.displayName = "Crouchwalk Back";
	move.slangName = "1";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Boomerang_A");
	move.displayName = "Task A";
	move.slangName = "Boomerang";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Boomerang_A_Air");
	move.displayName = "Air Task A";
	move.slangName = "Air Boomerang";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Dejavu_A");
	move.displayName = "P \x44\xC3\xA9\x6A\xC3\xA0 Vu";
	move.slangName = "DVA";
	move.powerup = powerup_djavu;
	move.powerupExplanation = powerupExplanation_djavu;
	move.dontShowPowerupGraphic = alwaysTrue;
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Dejavu_A_Air");
	move.displayName = "P Air \x44\xC3\xA9\x6A\xC3\xA0 Vu";
	move.slangName = "j.DVA";
	move.powerup = powerup_djavu;
	move.powerupExplanation = powerupExplanation_djavu;
	move.dontShowPowerupGraphic = alwaysTrue;
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Boomerang_B");
	move.displayName = "Task A'";
	move.slangName = "Teleport Boomerang";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Boomerang_B_Air");
	move.displayName = "Air Task A'";
	move.slangName = "Air Teleport Boomerang";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Dejavu_B");
	move.displayName = "K \x44\xC3\xA9\x6A\xC3\xA0 Vu";
	move.slangName = "DVA'";
	move.powerup = powerup_djavu;
	move.powerupExplanation = powerupExplanation_djavu;
	move.dontShowPowerupGraphic = alwaysTrue;
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Dejavu_B_Air");
	move.displayName = "K Air \x44\xC3\xA9\x6A\xC3\xA0 Vu";
	move.slangName = "j.DVA'";
	move.powerup = powerup_djavu;
	move.powerupExplanation = powerupExplanation_djavu;
	move.dontShowPowerupGraphic = alwaysTrue;
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "SpiralBed");
	move.displayName = "Task B";
	move.slangName = "Task B";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "SpiralBed_Air");
	move.displayName = "Air Task B";
	move.slangName = "Air Task B";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Dejavu_C");
	move.displayName = "S \x44\xC3\xA9\x6A\xC3\xA0 Vu";
	move.slangName = "DVB";
	move.powerup = powerup_djavu;
	move.powerupExplanation = powerupExplanation_djavu;
	move.dontShowPowerupGraphic = alwaysTrue;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Dejavu_C_Air");
	move.displayName = "S Air \x44\xC3\xA9\x6A\xC3\xA0 Vu";
	move.slangName = "j.DVB";
	move.powerup = powerup_djavu;
	move.powerupExplanation = powerupExplanation_djavu;
	move.dontShowPowerupGraphic = alwaysTrue;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "FlyingBed");
	move.displayName = "Task C";
	move.slangName = "Task C";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "FlyingBed_Air");
	move.displayName = "Air Task C";
	move.displayNameSelector = displayNameSelector_taskCAir;
	move.slangName = "Air Task C";
	move.displaySlangNameSelector = displaySlangNameSelector_taskCAir;
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Dejavu_D");
	move.displayName = "H \x44\xC3\xA9\x6A\xC3\xA0 Vu";
	move.slangName = "DVC";
	move.powerup = powerup_djavu;
	move.powerupExplanation = powerupExplanation_djavu;
	move.dontShowPowerupGraphic = alwaysTrue;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Dejavu_D_Air");
	move.displayName = "H Air \x44\xC3\xA9\x6A\xC3\xA0 Vu";
	move.slangName = "j.DVC";
	move.powerup = powerup_djavu;
	move.powerupExplanation = powerupExplanation_djavu;
	move.dontShowPowerupGraphic = alwaysTrue;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Alarm");
	move.displayName = "Sinusoidal Helios";
	move.slangName = "Clock Super";
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "AlarmBurst");
	move.displayName = "Burst Sinusoidal Helios";
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Merry");
	move.displayName = "Hemi Jack";
	move.slangName = "Sheep Super";
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	// Bedman Teleporting from the boomerang head hitting
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "BWarp");
	move.displayName = "Task A' Teleport";
	move.combineWithPreviousMove = true;
	move.dontSkipSuper = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Aralm_Obj", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 59;
	move.framebarName = "Sinusoidal Helios";
	move.framebarSlangName = "Helios";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Okkake", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 60;
	move.framebarName = "Hemi Jack";
	move.framebarSlangName = "Sheep Super";
	addMove(move);
	
	// the flying head
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Boomerang_A_Head", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 61;
	move.framebarName = "Task A";
	move.framebarSlangName = "Boomerang";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Boomerang_A_Head_Air", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 61;
	move.framebarName = "Task A";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	// created when doing Deja Vu (Task A). Creates either Boomerang_A_Djavu or Boomerang_A_Djavu_Air
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Djavu_A_Ghost", true);
	move.isDangerous = isDangerous_djavu;
	move.framebarId = 62;
	move.framebarName = "Deja Vu (Task A)";
	move.framebarSlangName = "DVA";
	addMove(move);
	
	// the flying head
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Boomerang_A_Djavu", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 62;
	move.framebarName = "Deja Vu (Task A)";
	move.framebarSlangName = "DVA";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Boomerang_A_Djavu_Air", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 62;
	move.framebarName = "Deja Vu (Air Task A)";
	move.framebarSlangName = "DVA";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Boomerang_B_Head", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 63;
	move.framebarName = "Task A'";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Boomerang_B_Head_Air", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 63;
	move.framebarName = "Task A'";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	// created when doing Deja Vu (Task A'). Creates either Boomerang_B_Djavu or Boomerang_B_Djavu_Air
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Djavu_B_Ghost", true);
	move.isDangerous = isDangerous_djavu;
	move.framebarId = 64;
	move.framebarName = "Deja Vu (Task A')";
	move.framebarSlangName = "DVA'";
	addMove(move);
	
	// the flying head
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Boomerang_B_Djavu", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 64;
	move.framebarName = "Deja Vu (Task A')";
	move.framebarSlangName = "DVA'";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Boomerang_B_Djavu_Air", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 64;
	move.framebarName = "Deja Vu (Air Task A')";
	move.framebarSlangName = "DVA'";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Djavu_C_Ghost", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 65;
	move.framebarName = "Deja Vu (Task B)";
	move.framebarSlangName = "DVB";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "bomb1", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 66;
	move.framebarName = "Task C Shockwave";
	move.framebarSlangName = "Shockwave";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "bomb2", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 66;
	move.framebarName = "Task C Shockwave";
	move.framebarSlangName = "Shockwave";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Djavu_D_Ghost", true);
	move.isDangerous = isDangerous_Djavu_D_Ghost;
	move.framebarId = 67;
	move.framebarName = "Deja Vu (Task C)";
	move.framebarSlangName = "DVC";
	move.framebarNameSelector = framebarNameSelector_djvuD;
	move.framebarSlangNameSelector = framebarSlangNameSelector_djvuD;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "AirStop");
	move.displayName = "Hover";
	move.addForceAddWhiffCancel("7Move");
	move.addForceAddWhiffCancel("8Move");
	move.addForceAddWhiffCancel("9Move");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "9Move");
	move.displayName = "Hover-9";
	move.replacementInputs = "Hold 9 for 12f";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "8Move");
	move.displayName = "Hover-8";
	move.replacementInputs = "Hold 8 for 12f";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "7Move");
	move.displayName = "Hover-7";
	move.replacementInputs = "Hold 7 for 12f";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "6Move");
	move.displayName = "Hover-6";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "4Move");
	move.displayName = "Hover-4";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "3Move");
	move.displayName = "Hover-3";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "2Move");
	move.displayName = "Hover-2";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "1Move");
	move.displayName = "Hover-1";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "MerryWarp");
	move.displayName = "Teleport to Sheep";
	addMove(move);
	
	// Ramlethal has two moves that can make her airborne: Sildo Detruo and Explode.
	// Sildo Detruo gives an airdash, and Explode gives nothing without jump installs.
	// Because of Explode, we have to not ignore super jump installs.
	// So anything that can't route into Explode, we have to ignore super jump installs there.
	// Turns out, there is not one such move, at least not where we'd want regular jump installs, but not super jump installs.
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "NmlAtk6B");
	move.displayName = "6K";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "NmlAtk3B");
	move.displayName = "3K";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitN", true);
	move.displayName = "Stand";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitNBack", true);
	move.displayName = "Back";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitNForward", true);
	move.displayName = "Forward";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitNUp", true);
	move.displayName = "Jump Up";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitNUpEnd", true);
	move.displayName = "Jump End";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitNDown", true);
	move.displayName = "Jump Down";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitNCrouch", true);
	move.displayName = "Crouch";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitNTurn", true);
	move.displayName = "Turn";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitNDamage", true);
	move.displayName = "Damage";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitN5C", true);
	move.displayName = "f.S";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitNAir5C", true);
	move.displayName = "j.S";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitNAir5C_Boss", true);
	move.displayName = "j.S (Boss)";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitN2C", true);
	move.displayName = "2S";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitN2C_Boss", true);
	move.displayName = "2S (Boss)";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitNBunri", true);
	move.displayName = "Deployed";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitFBunri", true);
	move.displayName = "Deployed";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "Bit4C", true);
	move.displayName = "Recall";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitNLaser", true);
	move.displayName = "Calvados";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitBlowC_Sword", true);
	move.displayName = "Marteli";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitBlowC_Sword_Bunri", true);
	move.displayName = "Marteli (Slow)";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitBlowC_SwordAir", true);
	move.displayName = "Air Marteli";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitBlowC_SwordAir_Bunri", true);
	move.displayName = "Air Marteli (Slow)";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitActionNeutral", true);
	move.displayName = "Neutral";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitNWinTypeA", true);
	move.displayName = "Victory";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitNWinTypeABunri", true);
	move.displayName = "Victory Deployed";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitNWinTypeB", true);
	move.displayName = "Victory";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitNWinTypeBBunri", true);
	move.displayName = "Victory Deployed";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitN5C_Boss", true);
	move.displayName = "f.S (Boss)";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitF", true);
	move.displayName = "Stand";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitFBack", true);
	move.displayName = "Back";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitFForward", true);
	move.displayName = "Forward";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitFUp", true);
	move.displayName = "Jump Up";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitFUpEnd", true);
	move.displayName = "Jump End";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitFDown", true);
	move.displayName = "Jump Down";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitFCrouch", true);
	move.displayName = "Crouch";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitFTurn", true);
	move.displayName = "Turn";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitFDamage", true);
	move.displayName = "Damage";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitFAir5D", true);
	move.displayName = "j.H";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitFAir5D_Boss", true);
	move.displayName = "j.H (Boss)";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitF2D", true);
	move.displayName = "2H";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitF2D_Boss", true);
	move.displayName = "2H (Boss)";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitF5D", true);
	move.displayName = "5H";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "Bit4D", true);
	move.displayName = "Recall";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitFLaser", true);
	move.displayName = "Calvados";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitBlowD_Sword", true);
	move.displayName = "Forpeli";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitBlowD_Sword_Bunri", true);
	move.displayName = "Forpeli (Slow)";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitBlowD_SwordAir", true);
	move.displayName = "Air Forpeli";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitBlowD_SwordAir_Bunri", true);
	move.displayName = "Air Forpeli (Slow)";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitFWinTypeA", true);
	move.displayName = "Victory";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitFWinTypeABunri", true);
	move.displayName = "Victory Deployed";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitFWinTypeB", true);
	move.displayName = "Victory";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitFWinTypeBBunri", true);
	move.displayName = "Victory Deployed";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "NmlAtk4B");
	move.displayName = "5K";  // Ramlethal's NmlAtk4B is just an alias for 5K
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "6D_Soubi_Land");
	move.displayName = "H Launch Greatsword";
	move.slangName = "6H";
	move.createdProjectile = createdProjectile_onf5;
	move.canYrcProjectile = canYrcProjectile_onf5;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "NmlAtk5DBunri");
	move.displayName = "Unarmed 5H";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "6D_Bunri_Land");
	move.displayName = "H Launch Greatsword (Already Deployed)";
	move.slangName = "6H";
	move.createdProjectile = createdProjectile_onf5;
	move.canYrcProjectile = canYrcProjectile_onf5;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "4D_Bunri_Land");
	move.displayName = "H Recover Greatsword";
	move.slangName = "4H";
	move.createdProjectile = createdProjectile_onf5;
	move.canYrcProjectile = canYrcProjectile_onf5;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "2D_Bunri_Land");
	move.displayName = "2H Launch Greatsword";
	move.slangName = "2H Summon";
	move.nameIncludesInputs = true;
	move.createdProjectile = createdProjectile_onf7;
	move.canYrcProjectile = canYrcProjectile_onf7;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "NmlAtk5C");
	move.displayName = "f.S";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "6C_Soubi_Land");
	move.displayName = "S Launch Greatsword";
	move.slangName = "6S";
	move.createdProjectile = createdProjectile_onf5;
	move.canYrcProjectile = canYrcProjectile_onf5;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "NmlAtk5CBunri");
	move.displayName = "Unarmed f.S";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "6C_Bunri_Land");
	move.displayName = "S Launch Greatsword (Already Deployed)";
	move.slangName = "6S";
	move.createdProjectile = createdProjectile_onf5;
	move.canYrcProjectile = canYrcProjectile_onf5;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "4C_Bunri_Land");
	move.displayName = "S Recover Greatsword";
	move.slangName = "4S";
	move.createdProjectile = createdProjectile_onf5;
	move.canYrcProjectile = canYrcProjectile_onf5;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "2C_Bunri_Land");
	move.displayName = "2S Launch Greatsword";
	move.slangName = "2S Summon";
	move.nameIncludesInputs = true;
	move.createdProjectile = createdProjectile_onf7;
	move.canYrcProjectile = canYrcProjectile_onf7;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "6D_Soubi_Air");
	move.displayName = "Air H Launch Greatsword";
	move.slangName = "Air 6H";
	move.createdProjectile = createdProjectile_onf5;
	move.canYrcProjectile = canYrcProjectile_onf5;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "NmlAtkAir5DBunri");
	move.displayName = "Unarmed j.H";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "6D_Bunri_Air");
	move.displayName = "Air H Launch Greatsword (Already Deployed)";
	move.slangName = "Air 6H";
	move.createdProjectile = createdProjectile_onf5;
	move.canYrcProjectile = canYrcProjectile_onf5;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "2D_Bunri_Air");
	move.displayName = "Air 2H Launch Greatsword";
	move.slangName = "Air 2H Summon";
	move.nameIncludesInputs = true;
	move.createdProjectile = createdProjectile_onf5;
	move.canYrcProjectile = canYrcProjectile_onf5;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "4D_Bunri_Air");
	move.displayName = "Air H Recover Greatsword";
	move.slangName = "Air 4H";
	move.createdProjectile = createdProjectile_onf5;
	move.canYrcProjectile = canYrcProjectile_onf5;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "6C_Soubi_Air");
	move.displayName = "Air S Launch Greatsword";
	move.slangName = "Air 6S";
	move.createdProjectile = createdProjectile_onf5;
	move.canYrcProjectile = canYrcProjectile_onf5;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "NmlAtkAir5CBunri");
	move.displayName = "Unarmed j.S";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "6C_Bunri_Air");
	move.displayName = "Air S Launch Greatsword (Already Deployed)";
	move.slangName = "Air 6S";
	move.createdProjectile = createdProjectile_onf5;
	move.canYrcProjectile = canYrcProjectile_onf5;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "2C_Bunri_Air");
	move.displayName = "Air 2S Launch Greatsword";
	move.slangName = "Air 2S Summon";
	move.nameIncludesInputs = true;
	move.createdProjectile = createdProjectile_onf5;
	move.canYrcProjectile = canYrcProjectile_onf5;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "4C_Bunri_Air");
	move.displayName = "Air S Recover Greatsword";
	move.slangName = "Air 4S";
	move.createdProjectile = createdProjectile_onf5;
	move.canYrcProjectile = canYrcProjectile_onf5;
	addMove(move);
	
	// Ramlethal's combination attacks only at best combo into sword summon, sword recall, 2S/2H sword summon or more combination attacks.
	// And none of them can make Ramlethal airborne.
	// As such, they do not care about jump installs.
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "CombinationBA");
	move.displayName = "Combination KP";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "CombinationBBB");
	move.displayName = "Combination KKK";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "CombinationBB");
	move.displayName = "Combination KK";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "CombinationAB");
	move.displayName = "Combination PK";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "CombinationAA");
	move.displayName = "Combination PP";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "CombinationBAB");
	move.displayName = "Combination KPK";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "CombinationBAA");
	move.displayName = "Combination KPP";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "CombinationAAB");
	move.displayName = "Combination PPK";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "CombinationAAA");
	move.displayName = "Combination PPP";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "Combination2BB");
	move.displayName = "Combination 2KK";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "Combination2BA");
	move.displayName = "Combination 2KP";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "Combination2BAB");
	move.displayName = "Combination 2KPK";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "Combination2BAA");
	move.displayName = "Combination 2KPP";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "Combination2AAB");
	move.displayName = "Combination 2PPK";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "Combination2AB");
	move.displayName = "Combination 2PK";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "Combination2AA");
	move.displayName = "Combination 2PP";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "Combination4B");
	move.displayName = "Combination 4K";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "6CBunriShot");
	move.displayName = "S Launch Greatsword (Boss Ver.)";
	move.slangName = "6S (Boss Ver.)";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "6DBunriShot");
	move.displayName = "H Launch Greatsword (Boss Ver.)";
	move.slangName = "6H (Boss Ver.)";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "MiddleShot");
	move.displayName = "Cassius";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BanditRevolverLand");
	move.displayName = "Sildo Detruo";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BanditRevolverAir");
	move.displayName = "Air Sildo Detruo";
	addMove(move);
	
	// Marteli and Forpeli are dead end moves where only Martli can cancel into Forpeli.
	// Which makes them not care about jump installs.
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitBlowD");
	move.displayName = "Forpeli";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitBlowC");
	move.displayName = "Marteli";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitBlowDAir");
	move.displayName = "Air Forpeli";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitBlowCAir");
	move.displayName = "Air Marteli";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitBlowD_Bunri");
	move.displayName = "Forpeli With Sword Recover";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitBlowC_Bunri");
	move.displayName = "Marteli With Sword Recover";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitBlowDAir_Bunri");
	move.displayName = "Air Forpeli With Sword Recover";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitBlowCAir_Bunri");
	move.displayName = "Air Marteli With Sword Recover";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "CommandThrow");
	move.displayName = "Flama Cargo";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "CommandThrowExe");
	move.displayName = "Flama Cargo";
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "FujinStep");
	move.displayName = "Fujin Step";
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "EasyFujinken");
	move.displayName = "Dauro";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "Fujinken");
	move.displayName = "Dauro";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "FastFujinken");
	move.displayName = "Green Dauro";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "TosshinRanbu");
	move.displayName = "Explode";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "TosshinRanbuExe");
	move.displayName = "Explode";
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitLaser");
	move.displayName = "Calvados";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitLaserBurst");
	move.displayName = "Burst Calvados";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitLaserBoss");
	move.displayName = "Calvados (Boss Ver.)";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitLaserBossBurst");
	move.displayName = "Burst Calvados (Boss Ver.)";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitSpiral");
	move.displayName = "Trance";
	move.canYrcProjectile = canYrcProjectile_onf9;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitSpiralBoss");
	move.displayName = "Trance (Boss Ver.)";
	move.canYrcProjectile = canYrcProjectile_onf9;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitN6C", true);
	move.displayName = "6S";
	move.isDangerous = isDangerous_launchGreatsword;
	move.framebarId = 68;
	move.framebarName = "S Sword";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitF6D", true);
	move.displayName = "6H";
	move.isDangerous = isDangerous_launchGreatsword;
	move.framebarId = 69;
	move.framebarName = "H Sword";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitN6CBunriShot", true);
	move.displayName = "Sword Spinny Attack";
	move.isDangerous = isDangerous_ramSwordMove;
	move.framebarId = 68;
	move.framebarName = "S Sword";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitN2C_Bunri", true);
	move.displayName = "2S Deployed";
	move.isDangerous = isDangerous_ramSwordMove;
	move.framebarId = 68;
	move.framebarName = "S Sword";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitN6CBunriShot_Boss", true);
	move.displayName = "Sword Spinny Attack (Boss)";
	move.isDangerous = isDangerous_ramSwordMove;
	move.framebarId = 68;
	move.framebarName = "S Sword";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitF6DBunriShot", true);
	move.displayName = "Sword Spinny Attack";
	move.isDangerous = isDangerous_ramSwordMove;
	move.framebarId = 69;
	move.framebarName = "H Sword";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitF2D_Bunri", true);
	move.displayName = "2H Deployed";
	move.isDangerous = isDangerous_ramSwordMove;
	move.framebarId = 69;
	move.framebarName = "H Sword";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitF6DBunriShot_Boss", true);
	move.displayName = "Sword Spinny Attack (Boss)";
	move.isDangerous = isDangerous_ramSwordMove;
	move.framebarId = 69;
	move.framebarName = "H Sword";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitSpiral_NSpiral", true);
	move.displayName = "Trance";
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 68;
	move.framebarName = "S Sword";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitSpiral_FSpiral", true);
	move.displayName = "Trance";
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 69;
	move.framebarName = "H Sword";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitLaser_Minion", true);
	move.isDangerous = isDangerous_hasNotCreatedAnythingYet;
	move.framebarId = 70;
	move.framebarName = "Laser";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitLaser", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 70;
	move.framebarName = "Laser";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "middleShot", true);
	move.isDangerous = isDangerous_not_hasHitNumButNoHitboxes;
	move.framebarId = 71;
	move.framebarName = "Cassius";
	addMove(move);
	
	// Sin's only moves that make him airborne are R.T.L. and Leaps.
	// All of them give him airdash.
	// This means Sin does not care about super jump install.
	charDoesNotCareAboutSuperJumpInstalls[CHARACTER_TYPE_SIN] = true;
	
	move = MoveInfo(CHARACTER_TYPE_SIN, "Tatakitsuke");
	move.displayName = "Bull Bash";
	move.powerup = powerup_kuuhuku;
	move.powerupExplanation = powerupExplanation_kuuhuku;
	move.dontShowPowerupGraphic = alwaysTrue;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SIN, "TobiagariA");
	move.displayName = "P Leap";
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.considerVariableStartupAsStanceForFramebar = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SIN, "TobiagariB");
	move.displayName = "K Leap";
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.considerVariableStartupAsStanceForFramebar = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SIN, "EatMeat");
	move.displayName = "Still Growing";
	move.slangName = "Eat";
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.powerup = powerup_eatMeat;
	move.powerupExplanation = powerupExplanation_eatMeat;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SIN, "EatMeat_Okawari");
	move.displayName = "Mash Still Growing";
	move.slangName = "Mash Eat";
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.powerup = powerup_eatMeat;
	move.powerupExplanation = powerupExplanation_eatMeat;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SIN, "Tatakiage");
	move.displayName = "Vulture Seize";
	move.slangName = "Vulture";
	move.powerup = powerup_kuuhuku;
	move.powerupExplanation = powerupExplanation_kuuhuku;
	move.dontShowPowerupGraphic = alwaysTrue;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SIN, "BeakDriver");
	move.displayName = "Beak Driver";
	move.slangName = "Beak";
	move.isInVariableStartupSection = isInVariableStartupSection_beakDriver;
	move.sectionSeparator = sectionSeparator_beakDriver;
	move.powerup = powerup_beakDriver;
	move.powerupExplanation = powerupExplanation_beakDriver;
	move.dontShowPowerupGraphic = dontShowPowerupGraphic_beakDriver;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SIN, "BeakDriver_Air");
	move.displayName = "Aerial Beak Driver";
	move.slangName = "Air Beak";
	move.powerup = powerup_kuuhuku;
	move.powerupExplanation = powerupExplanation_kuuhuku;
	move.dontShowPowerupGraphic = alwaysTrue;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SIN, "RideTheLightning");
	move.displayName = "R.T.L";
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = isInVariableStartupSection_sinRTL;
	move.iKnowExactlyWhenTheRecoveryOfThisMoveIs = isRecovery_sinRTL;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SIN, "RideTheLightningBurst");
	move.displayName = "Burst R.T.L";
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = isInVariableStartupSection_sinRTL;
	move.iKnowExactlyWhenTheRecoveryOfThisMoveIs = isRecovery_sinRTL;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SIN, "AirRideTheLightning");
	move.displayName = "Air R.T.L";
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = isInVariableStartupSection_sinRTL;
	move.iKnowExactlyWhenTheRecoveryOfThisMoveIs = isRecovery_sinRTL;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SIN, "AirRideTheLightningBurst");
	move.displayName = "Air Burst R.T.L";
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = isInVariableStartupSection_sinRTL;
	move.iKnowExactlyWhenTheRecoveryOfThisMoveIs = isRecovery_sinRTL;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SIN, "Ashibarai");
	move.displayName = "Elk Hunt";
	move.slangName = "Elk";
	move.powerup = powerup_kuuhuku;
	move.powerupExplanation = powerupExplanation_kuuhuku;
	move.dontShowPowerupGraphic = alwaysTrue;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SIN, "UkaseWaza");
	move.displayName = "Hawk Baker";
	move.powerup = powerup_kuuhuku;
	move.powerupExplanation = powerupExplanation_kuuhuku;
	move.dontShowPowerupGraphic = alwaysTrue;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SIN, "BeakDriver_Renda");
	move.displayName = "I'm Sure I'll Hit Something";
	move.slangName = "Beak Mash";
	move.powerup = powerup_kuuhuku;
	move.powerupExplanation = powerupExplanation_kuuhuku;
	move.dontShowPowerupGraphic = alwaysTrue;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SIN, "PhantomBarrel_Land");
	move.displayName = "Voltec Dein";
	move.slangName = "Voltec";
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SIN, "PhantomBarrel_Air");
	move.displayName = "Air Voltec Dein";
	move.slangName = "Air Voltec";
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SIN, "SuperShotStart", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 72;
	move.framebarName = "Voltec Dein";
	move.framebarSlangName = "Voltec";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SIN, "Shot_Land", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 72;
	move.framebarName = "Voltec Dein";
	move.framebarSlangName = "Voltec";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SIN, "SuperShotAirStart", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 72;
	move.framebarName = "Voltec Dein";
	move.framebarSlangName = "Voltec";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SIN, "Shot_Air", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 72;
	move.framebarName = "Voltec Dein";
	move.framebarSlangName = "Voltec";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_HAEHYUN, "NmlAtk6D");
	move.displayName = "6H";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_HAEHYUN, "NmlAtk5E");
	move.displayName = "5D";
	move.nameIncludesInputs = true;
	move.createdProjectile = createdProjectile_kum5D;
	move.canYrcProjectile = canYrcProjectile_kum5D;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_HAEHYUN, "CrouchFDash");
	move.displayName = "Crouchwalk";
	move.slangName = "3";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_HAEHYUN, "HomingEnergyC");
	move.displayName = "S Tuning Ball";
	move.slangName = "S Fireball";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_HAEHYUN, "HomingEnergyD");
	move.displayName = "H Tuning Ball";
	move.slangName = "H Fireball";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_HAEHYUN, "LandBlowAttack");
	move.displayName = "Falcon Dive";
	move.slangName = "Hayabusa";
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_HAEHYUN, "AirBlowAttack");
	move.displayName = "Aerial Falcon Dive";
	move.slangName = "Air Hayabusa";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_HAEHYUN, "AntiAirAttack");
	move.displayName = "Four Tigers Sword";
	move.slangName = "Shinken";
	move.isInVariableStartupSection = hasWhiffCancels;
	move.considerVariableStartupAsStanceForFramebar = true;
	addMove(move);
	
	// Haehyun 21[4K]
	move = MoveInfo(CHARACTER_TYPE_HAEHYUN, "LandBlow4Hasei");
	move.displayName = "Falcon Dive (Reverse Ver.)";
	move.slangName = "Hayabusa (Reverse)";
	move.sectionSeparator = sectionSeparator_falconDive;
	move.combineWithPreviousMove = true;
	move.isInVariableStartupSection = isInVariableStartupSection_falconDive;
	move.powerup = powerup_hayabusaRev;
	move.powerupExplanation = powerupExplanation_hayabusaRev;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	// Haehyun 214[K]
	move = MoveInfo(CHARACTER_TYPE_HAEHYUN, "LandBlow6Hasei");
	move.displayName = "Falcon Dive (Held)";
	move.slangName = "Hayabusa (Held)";
	move.sectionSeparator = sectionSeparator_falconDive;
	move.combineWithPreviousMove = true;
	move.isInVariableStartupSection = isInVariableStartupSection_falconDive;
	move.powerup = powerup_hayabusaHeld;
	move.powerupExplanation = powerupExplanation_hayabusaHeld;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	// Haehyun 623[K]
	move = MoveInfo(CHARACTER_TYPE_HAEHYUN, "AntiAir6Hasei");
	move.displayName = "Four Tigers Sword (Hold)";
	move.slangName = "Grampa Viper";
	move.combineWithPreviousMove = true;
	move.isInVariableStartupSection = isInVariableStartupSection_falconDive;
	move.powerup = powerup_grampaMax;
	move.powerupExplanation = powerupExplanation_grampaMax;
	addMove(move);
	
	// Haehyun 623[4K]
	move = MoveInfo(CHARACTER_TYPE_HAEHYUN, "AntiAir4Hasei");
	move.displayName = "Four Tigers Sword (Reverse Ver.)";
	move.slangName = "Shinken";
	move.combineWithPreviousMove = true;
	move.sectionSeparator = sectionSeparator_fourTigersSwordRev;
	move.isInVariableStartupSection = isInVariableStartupSection_falconDive;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	// Haehyun 236236H
	move = MoveInfo(CHARACTER_TYPE_HAEHYUN, "BlackHoleAttack");
	move.displayName = "Enlightened 3000 Palm Strike";
	move.slangName = "Clap Super";
	move.sectionSeparator = sectionSeparator_blackHoleAttack;
	move.isInVariableStartupSection = isInVariableStartupSection_blackHoleAttack;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_HAEHYUN, "BlackHoleAttackBurst");
	move.displayName = "Burst Enlightened 3000 Palm Strike";
	move.slangName = "Burst Clap";
	move.sectionSeparator = sectionSeparator_blackHoleAttack;
	move.isInVariableStartupSection = isInVariableStartupSection_blackHoleAttack;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_HAEHYUN, "SuperHomingEnergy");
	move.displayName = "Celestial Tuning Ball";
	move.slangName = "Super Ball";
	move.dontSkipSuper = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_HAEHYUN, "VacuumAtk", true);
	move.isDangerous = isDangerous_vacuumAtk;
	move.framebarId = 107;
	move.framebarName = "Enlightened 3000 Palm Strike Vacuum";
	move.framebarSlangName = "Vacuum";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_HAEHYUN, "EnergyBall", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 89;
	move.framebarName = "Tuning Ball";
	move.framebarSlangName = "Fireball";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_HAEHYUN, "SuperEnergyBall", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 90;
	move.framebarName = "Celestial Tuning Ball";
	move.framebarSlangName = "Super Ball";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_HAEHYUN, "kum_205shot", true);
	move.isDangerous = isDangerous_kum5D;
	move.framebarId = 105;
	move.framebarName = "5D";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "NmlAtk4AHasei");
	move.displayName = "4P";
	move.nameIncludesInputs = true;
	addMove(move);
	
	// The only moves Raven has that can make him airborne are 6H and Needle.
	// Needle gives airdash. And 6H is weird, but super jump installing it does not make you have 2 airdashes, no matter what.
	// Raven has no use for super jump installs.
	charDoesNotCareAboutSuperJumpInstalls[CHARACTER_TYPE_RAVEN] = true;
	
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "Kakkuu");
	move.displayName = "Glide";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "LandSettingTypeNeedle");
	move.displayName = "Scharf Kugel";
	move.slangName = "Orb";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "LandBlowAttack");
	move.displayName = "Grausam Impuls";
	move.slangName = "Scratch";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "LandSlowNeedle");
	move.displayName = "Schmerz Berg";
	move.slangName = "Needle";
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "AirSettingTypeNeedle");
	move.displayName = "Air Scharf Kugel";
	move.slangName = "Air Orb";
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "AirBlowAttack");
	move.displayName = "Air Grausam Impuls";
	move.slangName = "j.Scratch";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "AirSlowNeedleB");
	move.displayName = "K Grebechlich Licht";
	move.slangName = "Air K Needle";
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "AirSlowNeedleA");
	move.displayName = "P Grebechlich Licht";
	move.slangName = "Air P Needle";
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "CommandThrow");
	move.displayName = "H Wachen Zweig";
	move.slangName = "Command Grab";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "CommandThrowEx");
	move.displayName = "H Wachen Zweig";
	move.slangName = "Command Grab";
	move.combineWithPreviousMove = true;
	move.ignoreJumpInstalls = true;
	move.isGrab = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "AntiAirCommandThrow");
	move.displayName = "S Wachen Zweig";
	move.slangName = "S Grab";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "AntiAirCommandThrowEx");
	move.displayName = "S Wachen Zweig";
	move.slangName = "S Grab";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "LandDashAttack");
	move.displayName = "Verzweifelt";
	move.slangName = "Dash Super";
	move.dontSkipSuper = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "AirDashAttack");
	move.displayName = "Air Verzweifelt";
	move.slangName = "Air Dash Super";
	move.dontSkipSuper = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "RevengeAttack");
	move.displayName = "Getreuer";
	move.slangName = "Stab Super";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "RevengeAttackBurst");
	move.displayName = "Burst Getreuer";
	move.slangName = "Burst Stab";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "RevengeAttackEx");
	move.displayName = "Getreuer";
	move.slangName = "Stab Super";
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	// Raven stance when first entering it
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "ArmorDance");
	move.displayName = "Give it to me HERE";
	move.slangName = "Stance";
	move.sectionSeparator = sectionSeparator_armorDance;
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.canStopHolding = canStopHolding_armorDance;
	move.powerup = powerup_armorDance;
	move.powerupExplanation = powerupExplanation_armorDance;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	// Raven stance after armoring a hit in ArmorDance
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "ArmorDance2");
	move.displayName = "Give it to me HERE";
	move.slangName = "Stance";
	move.sectionSeparator = sectionSeparator_armorDance;
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.canStopHolding = canStopHolding_armorDance;
	move.powerup = powerup_armorDance;
	move.powerupExplanation = powerupExplanation_armorDance;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "SlowNeeldeObjLand", true);
	move.isDangerous = isDangerous_not_NullWhileActive;
	move.framebarId = 90;
	move.framebarName = "Schmerz Berg";
	move.framebarSlangName = "Needle";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "SlowNeeldeObjAir", true);
	move.isDangerous = isDangerous_not_NullWhileActive;
	move.framebarId = 90;
	move.framebarName = "Grebechlich Licht";
	move.framebarSlangName = "Needle";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "AirSettingTypeNeedleObj", true);
	move.isDangerous = isDangerous_not_NullWhileActive;
	move.framebarId = 91;
	move.framebarName = "Scharf Kugel";
	move.framebarSlangName = "Orb";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "LandSettingTypeNeedleObj", true);
	move.isDangerous = isDangerous_not_NullWhileActive;
	move.framebarId = 91;
	move.framebarName = "Scharf Kugel";
	move.framebarSlangName = "Orb";
	addMove(move);
	
	// Dizzy does not have a single special or super that can put her into the air,
	// so she never cares about super jump installs.
	// And if some move only leads to a special cancel, then regular jump installs don't matter on that move either
	charDoesNotCareAboutSuperJumpInstalls[CHARACTER_TYPE_DIZZY] = true;
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "NmlAtk2E");
	move.displayName = "2D";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;  // only leads to specials
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "NmlAtk6C");
	move.displayName = "4S";
	move.nameIncludesInputs = true;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "NmlAtk6D");
	move.displayName = "6H";
	move.nameIncludesInputs = true;
	move.sectionSeparator = sectionSeparator_dizzy6H;
	move.isInVariableStartupSection = isInVariableStartupSection_dizzy6H;
	move.ignoreJumpInstalls = true;  // only leads to specials
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "GammaRay");
	move.displayName = "Gamma Ray";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "Sakana");
	move.displayName = "I used this to catch fish";
	move.slangName = "Ice Spike";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "SakanaNecro");
	move.displayName = "For searing cod...";
	move.slangName = "Fire Pillar";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "Akari");
	move.displayName = "The light was so small in the beginning";
	move.slangName = "Fire Scythe";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "AkariUndine");
	move.displayName = "For putting out the light...";
	move.slangName = "Ice Scythe";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "HanashiD");
	move.displayName = "H We fought a lot together";
	move.slangName = "H Fish";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "HanashiD_Air");
	move.displayName = "H Air We fought a lot together...";
	move.slangName = "Air H Fish";
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "HanashiC");
	move.displayName = "S We fought a lot together";
	move.slangName = "S Fish";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "HanashiC_Air");
	move.displayName = "S Air We fought a lot together";
	move.slangName = "Air S Fish";
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "HanashiB");
	move.displayName = "K We talked a lot together";
	move.slangName = "K Fish";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "HanashiB_Air");
	move.displayName = "K Air We talked a lot together";
	move.slangName = "Air K Fish";
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "HanashiA");
	move.displayName = "P We talked a lot together";
	move.slangName = "P Fish";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "HanashiA_Air");
	move.displayName = "P Air We talked a lot together";
	move.slangName = "Air P Fish";
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "HanashiE");
	move.displayName = "D We fought a lot together";
	move.slangName = "Shield Fish";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);

	move = MoveInfo(CHARACTER_TYPE_DIZZY, "HanashiE_Air");
	move.displayName = "D Air We fought a lot together";
	move.slangName = "Air Shield Fish";
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "AwaP");
	move.displayName = "Please, leave me alone";
	move.slangName = "Bubble";
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "AwaK");
	move.displayName = "What happens when I'm TOO alone";
	move.slangName = "Fire Bubble";
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	// Dizzy 421H
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "KinomiNecro");
	move.displayName = "For roasting chestnuts...";
	move.slangName = "Fire Spears";
	move.sectionSeparator = sectionSeparator_kinomiNecro;
	move.isInVariableStartupSection = isInVariableStartupSection_kinomiNecro;
	move.powerup = powerup_fireSpear;
	move.powerupExplanation = powerupExplanation_fireSpear;
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "Kinomi");
	move.displayName = "I use this to pick fruit";
	move.slangName = "Ice Spear";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "ImperialRay");
	move.displayName = "Imperial Ray";
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "ImperialRayBurst");
	move.displayName = "Burst Imperial Ray";
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "KirikaeshiKakusei");
	move.displayName = "Don't be overprotective";
	move.slangName = "Mirror";
	move.dontSkipSuper = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "Tossin");
	move.displayName = "Rokkon Sogi";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "Tetsuzansen");
	move.displayName = "Tetsuzan Sen";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "CommandThrow");
	move.displayName = "Himawari";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "CommandThrowExe");
	move.displayName = "Himawari";
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "AirGCAntiGroundC");
	move.displayName = "S Kikyo";
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "AirGCAntiGroundD");
	move.displayName = "H Kikyo";
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "AirGCAntiGroundCGuard");
	move.displayName = "S Kikyo";
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "AirGCAntiGroundDGuard");
	move.displayName = "H Kikyo";
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "AirGCAntiAirA");
	move.displayName = "P Tsubaki";
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "AirGCAntiAirAGuard");
	move.displayName = "P Tsubaki";
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "AirGCAntiAirB");
	move.displayName = "K Tsubaki";
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	addMove(move);

	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "AirGCAntiAirBGuard");
	move.displayName = "K Tsubaki";
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "TatamiLand");
	move.displayName = "Tatami Gaeshi";
	move.slangName = "Tatami";
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "TatamiAir");
	move.displayName = "Air Tatami Gaeshi";
	move.slangName = "Air Tatami";
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "YouZanSen");
	move.displayName = "Yozan Sen";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "Kabari");
	move.displayName = "H Kabari";
	move.ignoreJumpInstalls = true;  // Kabaris only lead to moves that end up on the ground
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "KabariAntiAir");
	move.displayName = "S Kabari";
	move.ignoreJumpInstalls = true;  // Kabaris only lead to moves that end up on the ground
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "BlockingKakusei");
	move.displayName = "Metsudo Kushodo";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "BlockingKakuseiExe");
	move.displayName = "Metsudo Kushodo";
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	move.forceLandingRecovery = true;
	move.iKnowExactlyWhenTheRecoveryOfThisMoveIs = isRecovery_land;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "TsuraneSanzuWatashi");
	move.displayName = "Tsurane Sanzu-watashi";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "TsuraneSanzuWatashiBurst");
	move.displayName = "Burst Tsurane Sanzu-watashi";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "TsuraneSanzuWatashiExe");
	move.displayName = "Tsurane Sanzu-watashi";
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	// Baiken Suzuran
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "Suzuran");
	move.ignoresHitstop = true;
	move.whiffCancelsNote = "You must not be holding back for whiff cancels to be available.";
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = hasWhiffCancels;
	addMove(move);
	
	// Baiken Azami
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "BlockingStand");
	move.displayName = "Standing Azami";
	move.displayNameSelector = displayNameSelector_standingAzami;
	move.sectionSeparator = sectionSeparator_azami;
	move.canBlock = canBlock_default;
	move.ignoresHitstop = true;
	move.isInVariableStartupSection = hasWhiffCancelsAndCantBlock;
	move.considerVariableStartupAsStanceForFramebar = true;
	addMove(move);
	
	// Baiken Azami
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "BlockingCrouch");
	move.displayName = "Crouching Azami";
	move.displayNameSelector = displayNameSelector_crouchingAzami;
	move.sectionSeparator = sectionSeparator_azami;
	move.canBlock = canBlock_default;
	move.ignoresHitstop = true;
	move.isInVariableStartupSection = hasWhiffCancelsAndCantBlock;
	move.considerVariableStartupAsStanceForFramebar = true;
	addMove(move);
	
	// Baiken Azami
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "BlockingAir");
	move.displayName = "Aerial Azami";
	move.displayNameSelector = displayNameSelector_airAzami;
	move.sectionSeparator = sectionSeparator_azami;
	move.canBlock = canBlock_default;
	move.ignoresHitstop = true;
	move.isInVariableStartupSection = hasWhiffCancelsAndCantBlock;
	move.considerVariableStartupAsStanceForFramebar = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "Youshijin");   // P followup
	move.displayName = "Kuchinashi";
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "Mawarikomi");   // K followup
	move.displayName = "Mawari-komi";
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "Sakura");  // S followup
	move.displayName = "Sakura";
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "Issen");  // H followup
	move.displayName = "Rokkon Sogi";
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "Teppou");  // D followup
	move.displayName = "Yasha Gatana";
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "NmlAtk5E");
	move.displayName = "5D";
	move.nameIncludesInputs = true;
	move.createdProjectile = createdProjectile_baiken5D;
	move.canYrcProjectile = canYrcProjectile_baiken5D;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "NmlAtk5EShotObj", true);
	move.isDangerous = isDangerous_playerInRCOrHasActiveFlag_AndNotInRecovery;
	move.framebarId = 99;
	move.framebarName = "5D";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "NmlAtkAir5EShotObj", true);
	move.isDangerous = isDangerous_playerInRCOrHasActiveFlag_AndNotInRecovery;
	move.framebarId = 100;
	move.framebarName = "j.D";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "TeppouObj", true);
	move.isDangerous = isDangerous_not_hasHitNumButNoHitboxes;
	move.framebarId = 101;
	move.framebarName = "Yasha Gatana";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "TatamiLandObj", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 102;
	move.framebarName = "Tatami Gaeshi";
	move.framebarSlangName = "Tatami";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "TatamiAirObj", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 102;
	move.framebarName = "Tatami Gaeshi";
	move.framebarSlangName = "Tatami";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	// The only move that puts Jack-O in the air and does not give her an airdash is Zest.
	// If not for that, she would not care about super jump installs at all.
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "NmlAtk5D");
	move.displayName = "3H";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "NmlAtk6B");
	move.displayName = "6K";
	move.nameIncludesInputs = true;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "DustAtk");
	move.displayName = "5D";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "IronballGenocide");
	move.displayName = "4D";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "IronballGenocideEx");
	move.displayName = "4D";
	move.displayNameSelector = displayNameSelector_jacko4D;
	move.nameIncludesInputs = true;
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "IronballGenocideEx_Weak");
	move.displayName = "4D (Weak)";
	move.nameIncludesInputs = true;
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "CommandThorw");
	move.displayName = "6D";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "Zest");
	move.displayName = "2D";
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "UntieKiron'sChain");
	move.displayName = "j.D";
	move.nameIncludesInputs = true;
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "AirIronballGenocide");
	move.displayName = "j.4D";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "AirIronballGenocideEx");
	move.displayName = "j.4D";
	move.displayNameSelector = displayNameSelector_jackoj4D;
	move.nameIncludesInputs = true;
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "AirIronballGenocideEx_Weak");
	move.displayName = "j.4D (Weak)";
	move.nameIncludesInputs = true;
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "AirCommandThorw");
	move.displayName = "j.6D";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "CommandThorwEx");
	move.displayName = "6D/j.6D";
	move.nameIncludesInputs = true;
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "ThorwGhost");
	move.displayName = "Throw Ghost";
	move.createdProjectile = createdProjectile_ThrowGhost;
	move.canYrcProjectile = canYrcProjectile_ThrowGhost;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "AirThorwGhost");
	move.displayName = "Air Throw Ghost";
	move.createdProjectile = createdProjectile_AirThrowGhost;
	move.canYrcProjectile = canYrcProjectile_AirThrowGhost;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "PickUpGhost");
	move.displayName = "Pick Up Ghost";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "PutGhost");
	move.displayName = "Put Ghost Back Down";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "ReturnGhost");
	move.displayName = "Put Away Ghost";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "AirZest");
	move.displayName = "j.2D";
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "AirReturnGhost");
	move.displayName = "Air Put Away Ghost";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "SummonGhostA");
	move.displayName = "Set P Ghost";
	move.createdProjectile = createdProjectile_PGhost;
	move.canYrcProjectile = canYrcProjectile_PGhost;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "SummonGhostB");
	move.displayName = "Set K Ghost";
	move.createdProjectile = createdProjectile_KGhost;
	move.canYrcProjectile = canYrcProjectile_KGhost;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "SummonGhostC");
	move.displayName = "Set S Ghost";
	move.createdProjectile = createdProjectile_SGhost;
	move.canYrcProjectile = canYrcProjectile_SGhost;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "OrganOpen");
	move.displayName = "Organ Deployment";
	move.slangName = "Organ";
	move.sectionSeparator = sectionSeparator_organ;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = isInVariableStartupSection_organOpen;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "Calvados");
	move.displayName = "Calvados";
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "CalvadosBurst");
	move.displayName = "Burst Calvados";
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "ScrewPileDriver");
	move.displayName = "Forever Elysion Driver";
	move.slangName = "Supergrab";
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "AirScrewPileDriver");
	move.displayName = "Air Forever Elysion Driver";
	move.slangName = "Air Supergrab";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "ScrewPileDriverEx");
	move.displayName = "Air/Ground Forever Elysion Driver";
	move.slangName = "Air/Ground Supergrab";
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "AirCalvados");
	move.displayName = "Air Calvados";
	move.dontSkipSuper = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "AirCalvadosBurst");
	move.displayName = "Air Burst Calvados";
	move.dontSkipSuper = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "ServantA", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 82;
	move.framebarName = "Sword/Spear men";
	move.framebarNameUncombined = "Knight";
	move.combineHitsFromDifferentProjectiles = true;  // we need this because we don't want two knights attacking simultaneously displayed as two hits
	move.showMultipleHitsFromOneAttack = true;  // we need this because it's the same guy attacking over and over
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "ServantB", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 82;
	move.framebarName = "Sword/Spear men";
	move.framebarNameUncombined = "Lancer";
	move.combineHitsFromDifferentProjectiles = true;
	move.showMultipleHitsFromOneAttack = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "ServantC", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = MAGICIAN_FRAMEBAR_ID;
	move.framebarName = "Magicians";
	move.framebarNameUncombined = "Magician";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "magicAtkLv1", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = MAGICIAN_FRAMEBAR_ID;
	move.framebarName = "Magicians";
	move.framebarNameUncombined = "Magician Lv1";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "magicAtkLv2", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = MAGICIAN_FRAMEBAR_ID;
	move.framebarName = "Magicians";
	move.framebarNameUncombined = "Magician Lv2";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "magicAtkLv3", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = MAGICIAN_FRAMEBAR_ID;
	move.framebarName = "Magicians";
	move.framebarNameUncombined = "Magician Lv3";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "Fireball", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 84;
	move.framebarName = "j.D";
	move.combineHitsFromDifferentProjectiles = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "CalvadosObj", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 85;
	move.framebarName = "Calvados";
	move.framebarNameUncombined = "Calvados";
	addMove(move);
	
	// Only the boss version spawns this
	move = MoveInfo(CHARACTER_TYPE_JACKO, "CalvadosObj2", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 85;
	move.framebarName = "Calvados";
	move.framebarNameUncombined = "Calvados Hit 2";
	addMove(move);
	
	// Only the boss version spawns this
	move = MoveInfo(CHARACTER_TYPE_JACKO, "CalvadosObj3", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 85;
	move.framebarName = "Calvados";
	move.framebarNameUncombined = "Calvados Hit 3";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "Bomb", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 85;
	move.framebarName = "Calvados";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "GhostA", true);
	move.isDangerous = isDangerous_displayModel;
	move.framebarId = 86;
	move.framebarName = PROJECTILE_NAME_GHOST;
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "GhostB", true);
	move.isDangerous = isDangerous_displayModel;
	move.framebarId = 86;
	move.framebarName = PROJECTILE_NAME_GHOST;
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "GhostC", true);
	move.isDangerous = isDangerous_displayModel;
	move.framebarId = 86;
	move.framebarName = PROJECTILE_NAME_GHOST;
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "Suicidal_explosion", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 87;
	move.framebarName = "Explosion";
	move.framebarNameUncombined = "Explosion1";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "Suicidal_explosion2", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 87;
	move.framebarName = "Explosion";
	move.framebarNameUncombined = "Explosion2";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "Suicidal_explosion3", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 87;
	move.framebarName = "Explosion";
	move.framebarNameUncombined = "Explosion3";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "SakanaObj", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 92;
	move.framebarNameSelector = nameSelector_iceSpike;
	move.framebarSlangNameSelector = slangNameSelector_iceSpike;
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "AkariObj", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = 93;
	move.framebarNameSelector = nameSelector_iceScythe;
	move.framebarSlangNameSelector = slangNameSelector_iceScythe;
	move.canYrcProjectile = canYrcProjectile_default;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "AwaPObj", true);
	move.isDangerous = isDangerous_bubble;
	move.framebarId = 94;
	move.framebarName = "Please, leave me alone";
	move.framebarSlangName = "Bubble";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "AwaKObj", true);
	move.isDangerous = isDangerous_bubble;
	move.framebarId = 94;
	move.framebarName = "What happens when I'm TOO alone";
	move.framebarSlangName = "Fire Bubble";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "KinomiObj", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 95;
	move.framebarName = "I use this to pick fruit";
	move.framebarSlangName = "Ice Spear";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "KinomiObjNecro", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = KINOMI_OBJ_NECRO_FRAMEBAR_ID;
	move.framebarName = "For roasting chestnuts...";
	move.framebarSlangName = "Fire Spears";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "KinomiObjNecro2", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = KINOMI_OBJ_NECRO_FRAMEBAR_ID;
	move.framebarName = "For roasting chestnuts...";
	move.framebarSlangName = "Fire Spears";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "KinomiObjNecro3", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = KINOMI_OBJ_NECRO_FRAMEBAR_ID;
	move.framebarName = "For roasting chestnuts...";
	move.framebarSlangName = "Fire Spears";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "KinomiObjNecrobomb", true);
	move.isDangerous = isDangerous_not_hasHitNumButInactive;
	move.framebarId = KINOMI_OBJ_NECRO_FRAMEBAR_ID;
	move.framebarName = "For roasting chestnuts...";
	move.framebarSlangName = "Fire Spears";
	addMove(move);
	
	// P fish
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "HanashiObjA", true);
	move.isDangerous = isDangerous_pFish;
	move.framebarId = 96;
	move.framebarName = "We talked a lot together";
	move.framebarSlangName = "P Blue Fish";
	move.showMultipleHitsFromOneAttack = true;
	addMove(move);
	
	// K fish
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "HanashiObjB", true);
	move.isDangerous = isDangerous_kFish;
	move.framebarId = 96;
	move.framebarName = "We talked a lot together";
	move.framebarSlangName = "K Blue Fish";
	move.showMultipleHitsFromOneAttack = true;
	addMove(move);
	
	// S laser fish
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "HanashiObjD", true);
	move.isDangerous = isDangerous_laserFish;
	move.framebarId = 96;
	move.framebarName = "We fought a lot together";
	move.framebarSlangName = "Laser";
	addMove(move);
	
	// H laser fish
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "HanashiObjC", true);
	move.isDangerous = isDangerous_laserFish;
	move.framebarId = 96;
	move.framebarName = "We fought a lot together";
	move.framebarSlangName = "Laser";
	addMove(move);
	
	// H/S laser fish's laser
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "Laser", true);
	move.isDangerous = isDangerous_notNull;
	move.framebarId = 96;
	move.framebarName = "We fought a lot together";
	move.framebarSlangName = "Laser";
	addMove(move);
	
	// Shield fish
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "HanashiObjE", true);
	move.isDangerous = isDangerous_dFish;
	move.framebarId = 96;
	move.framebarName = "We fought a lot together";
	move.framebarSlangName = "Shield Fish";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "ImperialRayCreater", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 97;
	move.framebarName = "Imperial Ray";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "ImperialRayBakuhatsu", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = 97;
	move.framebarName = "Imperial Ray";
	move.combineHitsFromDifferentProjectiles = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "GammaRayLaser", true);
	move.isDangerous = isDangerous_notNull;
	move.framebarId = GAMMA_RAY_LASER_FRAMEBAR_ID;
	move.framebarName = "Gamma Ray";
	move.framebarNameUncombined = "Gamma Ray Laser";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "GammaRayLaserMax", true);
	move.isDangerous = isDangerous_not_NullWhileActive;
	move.framebarId = GAMMA_RAY_LASER_FRAMEBAR_ID;
	move.framebarName = "Gamma Ray";
	move.framebarNameUncombined = "Gamma Ray Max Laser";
	addMove(move);
	
	for (auto& it : map) {
		it.second.startPtr = allProperties.data() + it.second.startInd;
	}
	
	bool error = false;
	bbscrInstructionSizes = (unsigned short*)sigscanOffset(
		"GuiltyGearXrd.exe:.rdata",
		"24 00 04 00 28 00 04 00 0C 00 04 00 18 00 0C 00",
		&error, "bbscrInstructionSizes");
	
	#ifdef _DEBUG
	if (!repeatingMoves.empty()) {
		int totalLen = 0;
		int counter = 0;
		for (MyKey& key : repeatingMoves) {
			if (counter > 0) {
				++totalLen;  // "\n"
			}
			if (counter > 20) {
				totalLen += 3;  // "..."
				break;
			}
			if (key.charType == GENERAL) {
				totalLen += 7;  // "GENERAL"
			} else {
				totalLen += strlen(characterNames[key.charType]);
			}
			totalLen += 1;  // " "
			totalLen += strlen(key.name);
			if (key.isEffect) {
				totalLen += 9;  // " (Effect)"
			}
			++counter;
		}
		
		std::string massiveGigaString;
		massiveGigaString.reserve(17  // "Repeating moves:\n"
			+ totalLen);
		massiveGigaString += "Repeating moves:\n";
		
		counter = 0;
		for (MyKey& key : repeatingMoves) {
			if (counter > 0) {
				massiveGigaString += "\r\n";
			}
			if (counter > 20) {
				massiveGigaString += "...";
				break;
			}
			if (key.charType == GENERAL) {
				massiveGigaString += "GENERAL";
			} else {
				massiveGigaString += characterNames[key.charType];
			}
			massiveGigaString += ' ';
			massiveGigaString += key.name;
			if (key.isEffect) {
				massiveGigaString += " (Effect)";
			}
			++counter;
		}
		MessageBoxA(NULL, massiveGigaString.c_str(), "Error", MB_OK);
		return false;
	}
	#endif
	
	return !error;
}

void Moves::onAswEngineDestroyed() {
	armorDanceEndOffset = 0;
	armorDance2EndOffset = 0;
	saishingeki_SaishintuikaOffset = 0;
	saishingeki_SaishintuikaEndOffset = 0;
	zanseiRougaRecoveryOffset = 0;
	for (int i = 0; i < 2; ++i) {
		sinRtl_end_air_offset[i] = 0;
	}
	hououshouHitOffset = 0;
	stunEdgeMahojinDistX = 0;
	stunEdgeMahojinDistY = 0;
	chargedStunEdgeMahojinDistX = 0;
	chargedStunEdgeMahojinDistY = 0;
	sacredEdgeMahojinDistX = 0;
	sacredEdgeMahojinDistY = 0;
	spChargedStunEdgeKowareSpriteDuration = 0;
	stunEdgeDeleteSpriteSum = 0;
	laserFishCreateLaserOffset = 0;
	ky5DDustEffectShot_firstSpriteAfter_Offset = 0;
	mayPBallJumpConnectOffset = 0;
	mayKBallJumpConnectOffset = 0;
	mayPBallJumpConnectRange = 0;
	mayKBallJumpConnectRange = 0;
	mayIrukasanRidingObjectYokoA.clear();
	mayIrukasanRidingObjectYokoB.clear();
	mayIrukasanRidingObjectTateA.clear();
	mayIrukasanRidingObjectTateB.clear();
	may6H_6DHoldOffset = 0;
	may6H_6DHoldAttackOffset = 0;
	milliaIsRev2 = TRIBOOL_DUNNO;
	faust5DExPointX = -1;
	faust5DExPointY = -1;
	venomQvClearUponAfterExitOffset = 0;
	venomBishopCreateOffset = 0;
	ino5DCreateDustObjShotOffset = 0;
	bedmanSealA.clear();
	bedmanSealB.clear();
	bedmanSealC.clear();
	bedmanSealD.clear();
	venomStingerSPowerups.clear();
	venomStingerHPowerups.clear();
	kyMahojin.clear();
	ramlethalBitN6C.clear();
	ramlethalBitF6D.clear();
	ramlethalBitN2C.clear();
	ramlethalBitF2D.clear();
	elpheltRifleStartEndMarkerOffset = 0;
	elpheltRifleReloadEndMarkerOffset = 0;
	elpheltRifleReloadPerfectEndMarkerOffset = 0;
	elpheltRifleRomanEndMarkerOffset = 0;
	jackoAegisMax = 0;
	ghostAStateOffsets.clear();
	ghostBStateOffsets.clear();
	ghostCStateOffsets.clear();
	jackoThrowGhostOffset = 0;
	jackoAirThrowGhostOffset = 0;
	for (int i = 0; i < 2; ++i) {
		jackoGhostAExp[i] = 0;
		jackoGhostBExp[i] = 0;
		jackoGhostCExp[i] = 0;
	}
	for (int i = 0; i < 3; ++i) {
		jackoGhostACreationTimer[i] = 0;
		jackoGhostBCreationTimer[i] = 0;
		jackoGhostCCreationTimer[i] = 0;
	}
	for (int i = 0; i < 6; ++i) {
		jackoGhostAHealingTimer[i] = 0;
		jackoGhostBHealingTimer[i] = 0;
		jackoGhostCHealingTimer[i] = 0;
	}
	jackoGhostBuffTimer = 0;
	jackoGhostExplodeTimer = 0;
	servantAStateOffsets.clear();
	servantBStateOffsets.clear();
	servantCStateOffsets.clear();
	ghostADummyTotalFrames = 0;
	ghostBDummyTotalFrames = 0;
	ghostCDummyTotalFrames = 0;
	for (int i = 0; i < 2; ++i) {
		servantCooldownA[i] = 0;
		servantCooldownB[i] = 0;
		servantCooldownC[i] = 0;
	}
	servantExplosionTimer = 0;
	servantClockUpTimer = 0;
	servantTimeoutTimer = 0;
	for (int i = 0; i < 6; ++i) {
		servantAAtk[i].clear();
		servantBAtk[i].clear();
		servantCAtk[i].clear();
	}
	ghostPickupRange = 0;
	jackoAegisFieldRange = 0;
	jackoServantAAggroX = 0;
	jackoServantAAggroY = 0;
	jackoServantBAggroX = 0;
	jackoServantBAggroY = 0;
	jackoServantCAggroX = 0;
	jackoServantCAggroY = 0;
	jamSaishingekiY = 0;
	kum5Dcreation = 0;
	for (int i = 0; i < 3; ++i) {
		dizzyKinomiNecroBombMarker[i] = 0;
		dizzyKinomiNecroCreateBomb[i] = 0;
	}
	dizzyKinomiNecrobomb.clear();
	dizzyAkari.clear();
	dizzyPFishEnd.clear();
	dizzyKFishEnd.clear();
	dizzyDFishEnd.clear();
	dizzySFishNormal = 0;
	dizzySFishAlt = 0;
	dizzyHFishNormal = 0;
	dizzyHFishAlt = 0;
	dizzyAwaPKoware = 0;
	dizzyAwaPBomb.clear();
	dizzyAwaKKoware = 0;
	dizzyAwaKBomb.clear();
	baiken5Dcreation = 0;
	for (ForceAddedWhiffCancel& cancel : forceAddWhiffCancels) {
		cancel.clearCachedValues();
	}
	milliaSecretGardenUnlink = 0;
	milliaSecretGardenUnlinkFailedToFind = false;
	elpheltRifleFireStartup = 0;
	elpheltRifleFirePowerupStartup = 0;
}

void ForceAddedWhiffCancel::clearCachedValues() {
	moveIndexPerPlayer[0] = 0;
	moveIndexPerPlayer[1] = 0;
}

int ForceAddedWhiffCancel::getMoveIndex(Entity ent) {
	int playerIndex = ent.team();
	int* moveIndex = moveIndexPerPlayer + playerIndex;
	if (*moveIndex != 0) {
		if (*moveIndex == -1) return 0;
		return *moveIndex;
	}
	const AddedMoveData* base = ent.movesBase();
	int* indices = ent.moveIndices();
	for (int i = ent.moveIndicesCount() - 1; i >= 0; --i) {
		const AddedMoveData* move = base + indices[i];
		if (strcmp(move->name, name) == 0) {
			*moveIndex = i;
			return i;
		}
	}
	*moveIndex = -1;  // some moves only present in Rev2, won't be found in Rev1
	return 0;
}

bool Moves::getInfo(MoveInfo& returnValue, CharacterType charType, const char* moveName, const char* stateName, bool isEffect) {
	if (moveName && *moveName != '\0') {
		if (getInfo(returnValue, charType, moveName, isEffect)) {
			return true;
		}
	}
	if (stateName && *stateName != '\0') {
		if (getInfo(returnValue, charType, stateName, isEffect)) {
			return true;
		}
	}
	returnValue = defaultMove;
	return false;
}

bool Moves::getInfo(MoveInfo& returnValue, CharacterType charType, const char* name, bool isEffect) {
	auto it = map.find({charType, name, isEffect});
	if (it != map.end()) {
		returnValue = MoveInfo(it->second);
		return true;
	}
	if (charType != GENERAL) {
		it = map.find({GENERAL, name, isEffect});
		if (it != map.end()) {
			returnValue = MoveInfo(it->second);
			return true;
		}
	}
	return false;
}

bool sectionSeparator_enableWhiffCancels(PlayerInfo& ent) {
	return ent.wasEnableWhiffCancels;
}
bool sectionSeparator_mistFinerAirDash(PlayerInfo& ent) {
	return ent.pawn.currentAnimDuration() == 9;
}
bool sectionSeparator_mistFinerDash(PlayerInfo& ent) {
	return ent.pawn.currentAnimDuration() == 10;
}
bool sectionSeparator_alwaysTrue(PlayerInfo& ent) {
	return true;
}
bool sectionSeparator_blitzShield(PlayerInfo& ent) {
	return ent.pawn.currentAnimDuration() >= 16 && ent.pawn.mem45()
		|| strcmp(ent.pawn.gotoLabelRequest(), "attack") == 0;
}
bool sectionSeparator_may6P(PlayerInfo& ent) {
	return strcmp(ent.pawn.gotoLabelRequest(), "6AHoldAttack") == 0 || ent.pawn.mem45() == 0 && ent.pawn.currentAnimDuration() > 9;
}
bool sectionSeparator_may6H(PlayerInfo& ent) {
	return strcmp(ent.pawn.gotoLabelRequest(), "6DHoldAttack") == 0 || ent.pawn.mem45() == 0 && ent.pawn.currentAnimDuration() > 9;
}
bool sectionSeparator_breakTheLaw(PlayerInfo& ent) {
	return strcmp(ent.pawn.gotoLabelRequest(), "up") == 0;
}
bool sectionSeparator_FDB(PlayerInfo& ent) {
	if (strcmp(ent.pawn.gotoLabelRequest(), "Attack") == 0) {
		return true;
	}
	BYTE* markerPos = moves.findSetMarker(ent.pawn.bbscrCurrentFunc(), "Attack");
	if (!markerPos) return false;
	markerPos = moves.skipInstruction(markerPos);
	if (moves.instructionType(markerPos) != instr_sprite) return false;
	return ent.pawn.bbscrCurrentInstr() > markerPos;
}
bool sectionSeparator_soutenBC(PlayerInfo& ent) {
	return strcmp(ent.pawn.gotoLabelRequest(), "open") == 0
		|| !ent.pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED);
}
bool sectionSeparator_QV(PlayerInfo& ent) {
	return strcmp(ent.pawn.gotoLabelRequest(), "End") == 0
		|| !ent.pawn.mem45() && ent.pawn.currentAnimDuration() > 12;
}
bool sectionSeparator_stingerS(PlayerInfo& ent) {
	return strcmp(ent.pawn.gotoLabelRequest(), "Shot") == 0
		|| !ent.pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED) && ent.pawn.currentAnimDuration() > 9;
}
bool sectionSeparator_stingerH(PlayerInfo& ent) {
	return strcmp(ent.pawn.gotoLabelRequest(), "Shot") == 0
		|| !ent.pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED) && ent.pawn.currentAnimDuration() > 3;
}
bool sectionSeparator_sultryPerformance(PlayerInfo& ent) {
	return strcmp(ent.pawn.gotoLabelRequest(), "Attack") == 0
		|| !ent.pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED) && ent.pawn.currentAnimDuration() > 9;
}
bool sectionSeparator_beakDriver(PlayerInfo& ent) {
	return strcmp(ent.pawn.gotoLabelRequest(), "Attack") == 0
		|| strcmp(ent.pawn.gotoLabelRequest(), "HeavyAttack") == 0;
}
bool sectionSeparator_rifle(PlayerInfo& ent) {
	return ent.wasEnableWhiffCancels;
}
bool sectionSeparator_leoGuardStance(PlayerInfo& ent) {
	if (strcmp(ent.pawn.gotoLabelRequest(), "End") == 0) {
		return true;
	}
	BYTE* markerPos = moves.findSetMarker(ent.pawn.bbscrCurrentFunc(), "End");
	if (!markerPos) {
		return false;
	}
	BYTE* nextInstr = moves.skipInstruction(markerPos);
	if (moves.instructionType(nextInstr) != instr_sprite) {
		return false;
	}
	return ent.pawn.bbscrCurrentInstr() > nextInstr;
}
bool sectionSeparator_treasureHunt(PlayerInfo& ent) {
	return strcmp(ent.pawn.gotoLabelRequest(), "Run") == 0;
}
bool sectionSeparator_falconDive(PlayerInfo& ent) {
	return strcmp(ent.pawn.gotoLabelRequest(), "Attack") == 0
		|| strcmp(ent.pawn.gotoLabelRequest(), "Attack2") == 0;
}
bool sectionSeparator_fourTigersSwordRev(PlayerInfo& ent) {
	return strcmp(ent.pawn.gotoLabelRequest(), "Attack") == 0;
}
bool sectionSeparator_blackHoleAttack(PlayerInfo& ent) {
	return strcmp(ent.pawn.gotoLabelRequest(), "Attack") == 0;
}
bool sectionSeparator_armorDance(PlayerInfo& ent) {
	return strcmp(ent.pawn.gotoLabelRequest(), "End") == 0
		|| strcmp(ent.pawn.gotoLabelRequest(), "End2") == 0
		|| strcmp(ent.pawn.gotoLabelRequest(), "End3") == 0;
}
bool sectionSeparator_kinomiNecro(PlayerInfo& ent) {
	return strcmp(ent.pawn.gotoLabelRequest(), "end") == 0;
}
bool sectionSeparator_azami(PlayerInfo& ent) {
	return strcmp(ent.pawn.gotoLabelRequest(), "End") == 0;
}
bool sectionSeparator_organ(PlayerInfo& ent) {
	return strcmp(ent.pawn.gotoLabelRequest(), "A") == 0
		|| strcmp(ent.pawn.gotoLabelRequest(), "B") == 0
		|| strcmp(ent.pawn.gotoLabelRequest(), "C") == 0
		|| strcmp(ent.pawn.gotoLabelRequest(), "D") == 0
		|| strcmp(ent.pawn.gotoLabelRequest(), "E") == 0
		|| strcmp(ent.pawn.gotoLabelRequest(), "tame") == 0;
}
bool sectionSeparator_dizzy6H(PlayerInfo& ent) {
	return strcmp(ent.pawn.gotoLabelRequest(), "Attack") == 0;
}
bool sectionSeparator_saishingeki(PlayerInfo& ent) {
	return strcmp(ent.pawn.gotoLabelRequest(), "Saishintuika") == 0;
}

bool sectionSeparatorProjectile_dolphin(Entity ent) {
	return strcmp(ent.gotoLabelRequest(), "move") == 0 || ent.mem51() == 0;
}

bool isIdle_default(PlayerInfo& player) {
	return player.wasEnableNormals;
}
bool canBlock_default(PlayerInfo& player) {
	return player.pawn.enableBlock();
}
bool canBlock_azami(PlayerInfo& player) {
	return !player.inNewMoveSection || strcmp(player.pawn.gotoLabelRequest(), "end") == 0;
}
bool isIdle_enableWhiffCancels(PlayerInfo& player) {
	return player.wasEnableWhiffCancels;
}
bool isIdle_sparrowhawkStance(PlayerInfo& player) {
	return player.pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED) && player.wasEnableWhiffCancels;
}
bool isIdle_Souten8(PlayerInfo& player) {
	return !player.wasEnableGatlings;
}
bool isIdle_Rifle(PlayerInfo& player) {
	return player.wasEnableWhiffCancels
		&& (player.wasForceDisableFlags & 0x2) == 0;  // 0x2 is the force disable flag for Rifle_Fire
}
bool alwaysTrue(PlayerInfo& player) {
	return true;
}
bool alwaysFalse(PlayerInfo& player) {
	return false;
}
bool canBlock_neoHochihu(PlayerInfo& player) {
	return player.pawn.enableWalkBack();
}

BYTE* Moves::skipInstruction(BYTE* in) const {
	return in + bbscrInstructionSizes[*(unsigned int*)in];
}

inline InstructionType Moves::instructionType(BYTE* in) const {
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

BYTE* Moves::findNextMarker(BYTE* in, const char** name) const {
	if (name) *name = nullptr;
	while (true) {
		InstructionType type = instructionType(in);
		if (type == instr_endState) {
			return nullptr;
		}
		if (type == instr_setMarker) {
			if (name) *name = (const char*)(in + 4);
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

BYTE* Moves::findSprite(BYTE* in, const char* name) const {
	while (true) {
		InstructionType type = instructionType(in);
		if (type == instr_endState) {
			return nullptr;
		}
		if (type == instr_sprite
				&& strcmp((const char*)(in + 4), name) == 0) {
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

bool isDangerous_grenade(Entity ent) {
	return ent.hasUpon(BBSCREVENT_PLAYER_GOT_HIT);
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
	return !(ent.currentHitNum() != 0 && !(
		ent.hitboxCount(HITBOXTYPE_HITBOX) != 0
		&& ent.isActiveFrames()
	));
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
	return !(ent.dealtAttack()->angle == -90 && ent.hitboxCount(HITBOXTYPE_HITBOX) == 0);
}

bool isDangerous_launchGreatsword(Entity ent) {
	return !(ent.currentHitNum() != 0 && ent.hitboxCount(HITBOXTYPE_HITBOX) == 0
		|| !ent.hasUpon(BBSCREVENT_PLAYER_BLOCKED));
}
bool isDangerous_ramSwordMove(Entity ent) {
	return !(ent.currentHitNum() == 3 && ent.hitboxCount(HITBOXTYPE_HITBOX) == 0
		|| !ent.hasUpon(BBSCREVENT_PLAYER_BLOCKED));
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
	return !(ent.currentHitNum() == 2 && ent.hitboxCount(HITBOXTYPE_HITBOX) == 0)
		|| !ent.fullInvul();
}
bool isDangerous_pFish(Entity ent) {
	return !(ent.currentHitNum() != 0 && !ent.hasActiveFlag())
		|| !ent.fullInvul();
}
bool isDangerous_laserFish(Entity ent) {
	BYTE* func = ent.bbscrCurrentFunc();
	if (moves.laserFishCreateLaserOffset == 0) {
		BYTE* markerPos = moves.findCreateObj(func, "Laser");
		if (!markerPos) return false;
		moves.laserFishCreateLaserOffset = markerPos - func;
	}
	if (!moves.laserFishCreateLaserOffset) return false;
	return ent.bbscrCurrentInstr() < func + moves.laserFishCreateLaserOffset
		|| !ent.fullInvul();
}
bool isDangerous_dFish(Entity ent) {
	return !ent.isRecoveryState()
		|| !ent.fullInvul();
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
bool isDangerous_vacuumAtk(Entity ent) {
	return ent.currentHitNum() > 0 && ent.hitAlreadyHappened() < ent.theValueHitAlreadyHappenedIsComparedAgainst()
		|| ent.currentHitNum() == 0 && ent.currentAnimDuration() <= 2 && ent.enemyEntity().inHitstun();
}
bool isDangerous_mistKuttsuku(Entity ent) {
	return ent.lifeTimeCounter() == 0;
}

const char* nameSelector_iceSpike(Entity ent) {
	if (ent.createArgHikitsukiVal1() == 1) {
		return "For searing cod...";
	} else {
		return "I used this to catch fish";
	}
}
const char* slangNameSelector_iceSpike(Entity ent) {
	if (ent.createArgHikitsukiVal1() == 1) {
		return "Fire Pillar";
	} else {
		return "Ice Spike";
	}
}
const char* nameSelector_iceScythe(Entity ent) {
	if (ent.createArgHikitsukiVal1() == 1) {
		return "For putting out the light...";
	} else {
		return "The light was so small in the beginning";
	}
}
const char* slangNameSelector_iceScythe(Entity ent) {
	if (ent.createArgHikitsukiVal1() == 1) {
		return "Ice Scythe";
	} else {
		return "Fire Scythe";
	}
}
const char* framebarNameSelector_djvuD(Entity ent) {
	bool hasHeightBuff = ent.mem45();
	if (ent.currentAnimDuration() <= 7) {
		hasHeightBuff = ent.y() >= 700000;
	}
	if (hasHeightBuff) {
		return "Deja Vu (Task C) Buffed";
	} else {
		return "Deja Vu (Task C)";
	}
}
const char* framebarSlangNameSelector_djvuD(Entity ent) {
	bool hasHeightBuff = ent.mem45();
	if (ent.currentAnimDuration() <= 7) {
		hasHeightBuff = ent.y() >= 700000;
	}
	if (hasHeightBuff) {
		return "DVC Buffed";
	} else {
		return "DVC";
	}
}
const char* framebarNameSelector_closeShot(Entity ent) {
	entityList.populate();
	int dist = ent.enemyEntity().posX() - ent.posX();
	if (dist < 0) dist = -dist;
	if (dist >= 300000) return "Max Close Shot";
	return "Max Close Shot Buffed";
}
const char* framebarNameSelector_gunflameProjectile(Entity ent) {
	Entity player = ent.playerEntity();
	if (player) {
		if (strcmp(player.animationName(), "GunFlame") == 0
				&& strcmp(player.previousAnimName(), "CmnActFDash") == 0) {
			return "Runflame";
		}
	}
	return "Gunflame";
}
const char* framebarSlangNameSelector_gunflameProjectile(Entity ent) {
	Entity player = ent.playerEntity();
	if (player) {
		if (strcmp(player.animationName(), "GunFlame") == 0
				&& strcmp(player.previousAnimName(), "CmnActFDash") == 0) {
			return "Runflame";
		}
	}
	return "GF";
}
const char* framebarNameSelector_venomBall(Entity ent) {
	if (strcmp(ent.dealtAttack()->trialName, "Ball_RedHail") == 0) {
		return "Red Hail";
	} else if (strcmp(ent.dealtAttack()->trialName, "Ball_Gold") == 0) {
		return "Bishop Runout";
	} else {
		return "Balls";
	}
}
const char* framebarSlangNameSelector_venomBall(Entity ent) {
	if (strcmp(ent.dealtAttack()->trialName, "Ball_RedHail") == 0) {
		return "Red Hail";
	} else if (strcmp(ent.dealtAttack()->trialName, "Ball_Gold") == 0) {
		return "Bishop";
	} else {
		return "Balls";
	}
}

const char* MoveInfo::getFramebarName(Entity ent) const {
	if (framebarNameSelector && ent) return framebarNameSelector(ent);
	return framebarName;
}

const char* MoveInfo::getFramebarSlangName(Entity ent) const {
	if (framebarSlangNameSelector && ent) return framebarSlangNameSelector(ent);
	return framebarSlangName;
}

bool isInVariableStartupSection_treasureHunt(PlayerInfo& ent) {
	BYTE* markerPos = moves.findSetMarker(ent.pawn.bbscrCurrentFunc(), "Run");
	if (!markerPos) return false;
	return ent.pawn.bbscrCurrentInstr() <= markerPos && ent.pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED);
}
bool isInVariableStartupSection_zweiLand(PlayerInfo& ent) {
	return ent.wasEnableWhiffCancels && ent.y > 0;
}
bool isInVariableStartupSection_blitzShield(PlayerInfo& ent) {
	if (ent.pawn.mem45() == 1) return false;
	BYTE* markerPos = moves.findSetMarker(ent.pawn.bbscrCurrentFunc(), "attack");
	if (!markerPos) return false;
	return ent.pawn.currentAnimDuration() >= 16 && ent.pawn.bbscrCurrentInstr() <= markerPos && *ent.pawn.gotoLabelRequest() == '\0';
}
bool isInVariableStartupSection_may6Por6H(PlayerInfo& ent) {
	return ent.pawn.mem45() && *ent.pawn.gotoLabelRequest() == '\0';
}
bool isInVariableStartupSection_soutenBC(PlayerInfo& ent) {
	return ent.pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED) && *ent.pawn.gotoLabelRequest() == '\0' && !ent.pawn.isRecoveryState();
}
bool isInVariableStartupSection_amiMove(PlayerInfo& ent) {
	return ent.wasEnableWhiffCancels && !ent.pawn.hitstop();
}
bool isInVariableStartupSection_beakDriver(PlayerInfo& ent) {
	return ent.pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED);
}
bool isInVariableStartupSection_organOpen(PlayerInfo& ent) {
	return ent.pawn.mem45() && ent.pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED) && ent.pawn.gotoLabelRequest()[0] == '\0';
}
bool isInVariableStartupSection_breakTheLaw(PlayerInfo& ent) {
	return ent.pawn.mem45() && ent.pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED) && ent.pawn.gotoLabelRequest()[0] == '\0';
}
bool isInVariableStartupSection_fdb(PlayerInfo& ent) {
	return ent.pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED);
}
bool isInVariableStartupSection_qv(PlayerInfo& ent) {
	return ent.pawn.mem45() && ent.pawn.gotoLabelRequest()[0] == '\0';
}
bool isInVariableStartupSection_stinger(PlayerInfo& ent) {
	return ent.pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED) && ent.pawn.gotoLabelRequest()[0] == '\0';
}
bool isInVariableStartupSection_inoDivekick(PlayerInfo& ent) {
	return ent.pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED) && ent.pawn.gotoLabelRequest()[0] == '\0';
}
bool isInVariableStartupSection_sinRTL(PlayerInfo& ent) {
	return ent.pawn.mem49() && ent.pawn.mem45() && ent.pawn.mem46() <= 1 && ent.pawn.gotoLabelRequest()[0] == '\0';
}
bool isInVariableStartupSection_falconDive(PlayerInfo& ent) {
	return ent.pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED);
}
bool isInVariableStartupSection_blackHoleAttack(PlayerInfo& ent) {
	return ent.pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED) && ent.pawn.mem45();
}
bool isInVariableStartupSection_dizzy6H(PlayerInfo& ent) {
	return ent.pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED);
}
bool isInVariableStartupSection_kinomiNecro(PlayerInfo& ent) {
	return ent.pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED);
}
bool isInVariableStartupSection_saishingeki(PlayerInfo& ent) {
	return ent.pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED) && ent.pawn.mem45();
}

bool isRecoveryHasGatlings_mayRideTheDolphin(PlayerInfo& ent) {
	return ent.pawn.attackCollidedSoCanCancelNow() && ent.pawn.enableGatlings();
}
bool isRecoveryCanAct_mayRideTheDolphin(PlayerInfo& ent) {
	return ent.wasEnableWhiffCancels && ent.airborne_afterTick();
}
bool isRecoveryHasGatlings_enableWhiffCancels(PlayerInfo& ent) {
	return ent.wasEnableWhiffCancels;
}
bool hasWhiffCancels(PlayerInfo& ent) {
	return ent.wasEnableWhiffCancels && ent.wasHadWhiffCancels();
}
bool hasWhiffCancelsAndCantBlock(PlayerInfo& ent) {
	return ent.wasEnableWhiffCancels && ent.wasHadWhiffCancels() && !ent.pawn.enableBlock();
}
bool isRecoveryHasGatlings_beakDriver(PlayerInfo& ent) {
	return ent.wasEnableGatlings && ent.wasAttackCollidedSoCanCancelNow;
}
bool isRecoveryCanAct_beakDriver(PlayerInfo& ent) {
	return ent.wasEnableGatlings && ent.wasAttackCollidedSoCanCancelNow && ent.wasHadGatling("BeakDriver_Renda");
}

bool aSectionBeforeVariableStartup_leoParry(PlayerInfo& ent) {
	return *ent.pawn.gotoLabelRequest() == '\0' && ent.pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED);
}

bool canStopHolding_armorDance(PlayerInfo& ent) {
	if (!ent.pawn.mem45() || *ent.pawn.gotoLabelRequest() != '\0') return false;
	BYTE* funcStart = ent.pawn.bbscrCurrentFunc();
	
	int* armorDanceEndOffset;
	// length of "ArmorDance" == 10
	if (*(ent.pawn.animationName() + 10) == '2') {  // is ArmorDance2
		armorDanceEndOffset = &moves.armorDance2EndOffset;
	} else {
		armorDanceEndOffset = &moves.armorDanceEndOffset;
	}
	
	if (*armorDanceEndOffset == 0) {
		BYTE* markerPos = moves.findSetMarker(ent.pawn.bbscrCurrentFunc(), "End");
		if (!markerPos) return false;
		*armorDanceEndOffset = markerPos - funcStart;
	}
	BYTE* markerPos = funcStart + *armorDanceEndOffset;
	return ent.pawn.bbscrCurrentInstr() <= markerPos;
}

bool canStopHolding_armorDance2(PlayerInfo& ent) {
	if (!ent.pawn.mem45() || *ent.pawn.gotoLabelRequest() != '\0') return false;
	BYTE* funcStart = ent.pawn.bbscrCurrentFunc();
	if (moves.armorDance2EndOffset == 0) {
		BYTE* markerPos = moves.findSetMarker(ent.pawn.bbscrCurrentFunc(), "End");
		if (!markerPos) return false;
		moves.armorDance2EndOffset = markerPos - funcStart;
	}
	BYTE* markerPos = funcStart + moves.armorDance2EndOffset;
	return ent.pawn.bbscrCurrentInstr() <= markerPos;
}

bool frontLegInvul_potemkinBuster(PlayerInfo& ent) {
	return strcmp(ent.sprite.name, "pot409_00") == 0;
}

bool isRecoveryCanReload_rifle(PlayerInfo& ent) {
	return ent.wasEnableWhiffCancels && ent.wasHadWhiffCancel("Rifle_Reload");
}

DWORD zatoHoldLevel_breakTheLaw(PlayerInfo& ent) {
	DWORD level = ent.pawn.mem47() ? 3 : ent.pawn.mem46() ? 2 : 0;
	DWORD released = ent.inNewMoveSection;
	return level | (released << 2);
}

bool conditionForAddingWhiffCancels_blockingBaiken(PlayerInfo& ent) {
	return ent.pawn.playerVal(0);
}

bool secondaryStartup_saishingeki(PlayerInfo& ent) {
	if (strcmp(ent.pawn.gotoLabelRequest(), "Saishintuika") == 0) return true;
	if (ent.pawn.currentHitNum() == 2 && !ent.pawn.isActiveFrames()) return false;
	BYTE* funcStart = ent.pawn.bbscrCurrentFunc();
	if (moves.saishingeki_SaishintuikaOffset == 0 && moves.saishingeki_SaishintuikaEndOffset == 0) {
		BYTE* markerPos = moves.findSetMarker(ent.pawn.bbscrCurrentFunc(), "Saishintuika");
		if (!markerPos) return false;
		moves.saishingeki_SaishintuikaOffset = markerPos - funcStart;
		BYTE* nextSearchStart = moves.skipInstruction(markerPos);
		BYTE* nextMarkerPos = moves.findNextMarker(nextSearchStart, nullptr);
		if (nextMarkerPos) {
			moves.saishingeki_SaishintuikaEndOffset = nextMarkerPos - funcStart;
		}
	}
	if (moves.saishingeki_SaishintuikaOffset == 0 && moves.saishingeki_SaishintuikaEndOffset == 0) return false;
	if (moves.saishingeki_SaishintuikaEndOffset == 0) {
		return ent.pawn.bbscrCurrentInstr() >= funcStart + moves.saishingeki_SaishintuikaEndOffset;
	} else {
		BYTE* currentInstr = ent.pawn.bbscrCurrentInstr();
		return currentInstr >= funcStart + moves.saishingeki_SaishintuikaOffset
			&& currentInstr < funcStart + moves.saishingeki_SaishintuikaEndOffset;
	}
}

bool isRecovery_daisenpu(PlayerInfo& ent) {
	return ent.pawn.currentHitNum() >= 10;
}
bool isRecovery_RTL(PlayerInfo& ent) {
	return ent.pawn.currentHitNum();
}
bool isRecovery_sinRTL(PlayerInfo& ent) {
	BYTE* funcStart = ent.pawn.bbscrCurrentFunc();
	bool isAir = strncmp(ent.pawn.animationName(), "Air", 3) == 0;
	int* offset = moves.sinRtl_end_air_offset + isAir;
	if (*offset == 0) {
		BYTE* markerPos = moves.findSetMarker(ent.pawn.bbscrCurrentFunc(), "end_air");
		if (!markerPos) return false;
		*offset = markerPos - funcStart;
	}
	if (*offset == 0) return false;
	BYTE* currentInstr = ent.pawn.bbscrCurrentInstr();
	return currentInstr >= funcStart + *offset;
}
bool isRecovery_recovery(PlayerInfo& ent) {
	return ent.pawn.isRecoveryState();
}
bool isRecovery_zanseiRouga(PlayerInfo& ent) {
	BYTE* funcStart = ent.pawn.bbscrCurrentFunc();
	if (moves.zanseiRougaRecoveryOffset == 0) {
		BYTE* spritePos = moves.findSprite(funcStart, "chp431_17");
		if (!spritePos) return false;
		moves.zanseiRougaRecoveryOffset = spritePos - funcStart;
	}
	if (!moves.zanseiRougaRecoveryOffset) return false;
	BYTE* currentInstr = ent.pawn.bbscrCurrentInstr();
	return currentInstr > funcStart + moves.zanseiRougaRecoveryOffset;
}
bool isRecovery_land(PlayerInfo& ent) {
	return ent.pawn.y() == 0;
}
bool isRecovery_byakueRenshou(PlayerInfo& ent) {
	return ent.wasEnableWhiffCancels;
}

bool forceSuperHitAnyway_zanseiRouga(PlayerInfo& ent) {
	return ent.pawn.hideUI();
}
bool forceSuperHitAnyway_hououshou(PlayerInfo& ent) {
	BYTE* funcStart = ent.pawn.bbscrCurrentFunc();
	if (moves.hououshouHitOffset == 0) {
		BYTE* markerPos = moves.findSetMarker(funcStart, "hit");
		if (!markerPos) return false;
		moves.hououshouHitOffset = markerPos - funcStart;
	}
	if (!moves.hououshouHitOffset) return false;
	BYTE* currentInstr = ent.pawn.bbscrCurrentInstr();
	return currentInstr >= funcStart + moves.hououshouHitOffset;
}

const char* displayNameSelector_pogoEntry(PlayerInfo& ent) {
	return ent.idle ? "Spear Point Centripetal Dance Idle" : "Spear Point Centripetal Dance Entry";
}
const char* displaySlangNameSelector_pogoEntry(PlayerInfo& ent) {
	return ent.idle ? "Pogo Idle" : "Pogo Entry";
}
const char* displayNameSelector_pogoA(PlayerInfo& ent) {
	return ent.idle ? "Spear Point Centripetal Dance Idle" : "Just A Taste!";
}
const char* displaySlangNameSelector_pogoA(PlayerInfo& ent) {
	return ent.idle ? "Pogo Stance" : "Pogo-P";
}
const char* displayNameSelector_pogo9(PlayerInfo& ent) {
	return ent.idle ? "Spear Point Centripetal Dance Idle" : "Short Hop";
}
const char* displaySlangNameSelector_pogo9(PlayerInfo& ent) {
	return ent.idle ? "Pogo Stance" : "Pogo-9";
}
const char* displayNameSelector_pogo44(PlayerInfo& ent) {
	return ent.idle ? "Spear Point Centripetal Dance Idle" : "Backward Movement";
}
const char* displaySlangNameSelector_pogo44(PlayerInfo& ent) {
	return ent.idle ? "Pogo Stance" : "Pogo-44";
}
const char* displayNameSelector_pogo66(PlayerInfo& ent) {
	return ent.idle ? "Spear Point Centripetal Dance Idle" : "Forward Movement";
}
const char* displaySlangNameSelector_pogo66(PlayerInfo& ent) {
	return ent.idle ? "Pogo Stance" : "Pogo-66";
}
const char* displayNameSelector_pogoB(PlayerInfo& ent) {
	return ent.idle ? "Spear Point Centripetal Dance Idle" : "Growing Flower";
}
const char* displaySlangNameSelector_pogoB(PlayerInfo& ent) {
	return ent.idle ? "Pogo Stance" : "Pogo-K";
}
const char* displayNameSelector_pogoC(PlayerInfo& ent) {
	return ent.idle ? "Spear Point Centripetal Dance Idle" : "See? I'm a Flower!";
}
const char* displaySlangNameSelector_pogoC(PlayerInfo& ent) {
	return ent.idle ? "Pogo Stance" : "Pogo-S";
}
const char* displayNameSelector_pogoD(PlayerInfo& ent) {
	return ent.idle ? "Spear Point Centripetal Dance Idle" : "Spear Point Centripetal Dance Going My Way";
}
const char* displaySlangNameSelector_pogoD(PlayerInfo& ent) {
	return ent.idle ? "Pogo Stance" : "Pogo-H";
}
const char* displayNameSelector_pogoE(PlayerInfo& ent) {
	return ent.idle ? "Spear Point Centripetal Dance Idle" : "Spear Point Centripetal Dance What Could This Be?";
}
const char* displaySlangNameSelector_pogoE(PlayerInfo& ent) {
	return ent.idle ? "Pogo Stance" : "Pogo-D";
}
const char* displayNameSelector_pogo8(PlayerInfo& ent) {
	return ent.idle ? "Spear Point Centripetal Dance Idle" : "Doctor-Copter";
}
const char* displaySlangNameSelector_pogo8(PlayerInfo& ent) {
	return ent.idle ? "Pogo Stance" : "Pogo-8";
}
const char* displayNameSelector_RC(PlayerInfo& ent) {
	return ent.pawn.yellowRomanCancel()
		? "Yellow Roman Cancel"
		: ent.pawn.purpleRomanCancel()
			? "Purple Roman Cancel"
			: "Red Roman Cancel";
}
const char* displaySlangNameSelector_RC(PlayerInfo& ent) {
	return ent.pawn.yellowRomanCancel()
		? "YRC"
		: ent.pawn.purpleRomanCancel()
			? "PRC"
			: "RRC";
}
const char* displayNameSelector_may6P(PlayerInfo& ent) {
	struct May6PElement {
		const char* name;
		int stun;
	};
	static const May6PElement ar[] {
		{ "6P", 88 },
		{ "6P (Lvl1)", 110 },
		{ "6P (Lvl2)", 121 },
		{ "6P (Lvl3)", 132 },
		{ "6P Max", 143 }
	};
	for (int i = 0; i < _countof(ar); ++i) {
		if (ent.pawn.dealtAttack()->stun == ar[i].stun) {
			return ar[i].name;
		}
	}
	return ar[0].name;
}
const char* displayNameSelector_may6H(PlayerInfo& ent) {
	if (strcmp(ent.pawn.gotoLabelRequest(), "6DHoldAttack") == 0) {
		return "6H Slightly Held";
	}
	BYTE* func = ent.pawn.bbscrCurrentFunc();
	fillMay6HOffsets(func);
	int offset = ent.pawn.bbscrCurrentInstr() - func;
	if (offset > moves.may6H_6DHoldOffset && offset < moves.may6H_6DHoldAttackOffset) {
		return ent.pawn.mem45() == 0 ? "6H Max" : "6H Hold";
	} else if (offset > moves.may6H_6DHoldAttackOffset) {
		return "6H Slightly Held";
	} else {
		return "6H";
	}
}
const char* displayNameSelector_badMoon(PlayerInfo& ent) {
	int maxHit = ent.maxHit.max;
	if (maxHit == 10) {
		return "Bad Moon (Height Buff, 10 hits)";
	} else if (maxHit == 9) {
		return "Bad Moon (Height Buff, 9 hits)";
	} else if (maxHit == 8) {
		return "Bad Moon (Height Buff, 8 hits)";
	} else if (maxHit == 7) {
		return "Bad Moon (Height Buff, 7 hits)";
	} else if (maxHit == 6) {
		return "Bad Moon (Height Buff, 6 hits)";
	} else if (maxHit == 5) {
		return "Bad Moon (Height Buff, 5 hits)";
	} else if (ent.pawn.mem46()) {
		return "Bad Moon (Height Buff)";
	} else {
		return "Bad Moon";
	}
}
const char* displaySlangNameSelector_badMoon(PlayerInfo& ent) {
	int maxHit = ent.maxHit.max;
	if (maxHit == 10) {
		return "BM (Height Buff, 10 hits)";
	} else if (maxHit == 9) {
		return "BM (Height Buff, 9 hits)";
	} else if (maxHit == 8) {
		return "BM (Height Buff, 8 hits)";
	} else if (maxHit == 7) {
		return "BM (Height Buff, 7 hits)";
	} else if (maxHit == 6) {
		return "BM (Height Buff, 6 hits)";
	} else if (maxHit == 5) {
		return "BM (Height Buff, 5 hits)";
	} else if (ent.pawn.mem46()) {
		return "BM (Height Buff, 4 hits)";
	} else {
		return "BM";
	}
}
const char* displayNameSelector_carcassRaidS(PlayerInfo& ent) {
	if (ent.pawn.createArgHikitsukiVal2_outgoing() == 1  // rev1
			&& ent.pawn.venomBallArg3() == 164) {
		return "S Carcass Raid With Spin";
	} else {
		return "S Carcass Raid";
	}
}
const char* displaySlangNameSelector_carcassRaidS(PlayerInfo& ent) {
	if (ent.pawn.createArgHikitsukiVal2_outgoing() == 1  // rev1
			&& ent.pawn.venomBallArg3() == 164) {
		return "S Carcass With Spin";
	} else {
		return "S Carcass";
	}
}
const char* displayNameSelector_carcassRaidH(PlayerInfo& ent) {
	if (ent.pawn.venomBallArg3() == 2724) {
		return "H Carcass Raid With Spin";
	} else {
		return "H Carcass Raid";
	}
}
const char* displaySlangNameSelector_carcassRaidH(PlayerInfo& ent) {
	if (ent.pawn.venomBallArg3() == 2724) {
		return "H Carcass With Spin";
	} else {
		return "H Carcass";
	}
}
const char* displayNameSelector_stingerS(PlayerInfo& ent) {
	if (ent.pawn.createArgHikitsukiVal2_outgoing() == 1  // rev1
			&& ent.pawn.venomBallArg3() == 164) {
		return "S Stinger Aim With Spin";
	} else {
		return "S Stinger Aim";
	}
}
const char* displaySlangNameSelector_stingerS(PlayerInfo& ent) {
	if (ent.pawn.createArgHikitsukiVal2_outgoing() == 1  // rev1
			&& ent.pawn.venomBallArg3() == 164) {
		return "S Stinger With Spin";
	} else {
		return "S Stinger";
	}
}
const char* displayNameSelector_stingerH(PlayerInfo& ent) {
	if (ent.pawn.venomBallArg3() == 164) {
		return "H Stinger Aim With Spin";
	} else {
		return "H Stinger Aim";
	}
}
const char* displaySlangNameSelector_stingerH(PlayerInfo& ent) {
	if (ent.pawn.venomBallArg3() == 164) {
		return "H Stinger With Spin";
	} else {
		return "H Stinger";
	}
}
const char* displayNameSelector_taskCAir(PlayerInfo& ent) {
	if (ent.pawn.mem45()) {
		return "Air Task C Buffed";
	} else {
		return "Air Task C";
	}
}
const char* displaySlangNameSelector_taskCAir(PlayerInfo& ent) {
	if (ent.pawn.mem45()) {
		return "Air Task C Buffed";
	} else {
		return "Air Task C";
	}
}
const char* framebarNameSelector_blueBurst(Entity ent) {
	int team = ent.team();
	if (!(team == 0 || team == 1)) return "Blue Burst";
	return endScene.players[team].wasOtg ? "OTG Burst" : "Blue Burst";
}
const char* displayNameSelector_blueBurst(PlayerInfo& ent) {
	return ent.wasOtg ? "OTG Burst" : "Blue Burst";
}
const char* displayNameSelector_rifleStart(PlayerInfo& ent) {
	if (ent.idle) return "Ms. Confille";
	const char* response = moves.rifleAutoExit(ent, &moves.elpheltRifleStartEndMarkerOffset, "Ms. Confille");
	if (response) return response;
	if (moves.forCancels) return "Aim Ms. Confille";
	return !ent.inNewMoveSection ? "Aim Ms. Confille Until Able to Cancel" : "Aim Ms. Confille Until Able to Fire";
}
const char* displaySlangNameSelector_rifleStart(PlayerInfo& ent) {
	if (ent.idle) return "Rifle";
	const char* response = moves.rifleAutoExit(ent, &moves.elpheltRifleStartEndMarkerOffset, "Rifle Autoexit");
	if (response) return response;
	if (moves.forCancels) return "Rifle";
	return !ent.inNewMoveSection ? "Rifle Until Able to Cancel" : "Rifle Until Able to Fire";
}
const char* displayNameSelector_rifleReload(PlayerInfo& ent) {
	if (ent.idle) return "Ms. Confille";
	const char* response = moves.rifleAutoExit(ent, &moves.elpheltRifleReloadEndMarkerOffset, "Ms. Confille Autoexit");
	if (response) return response;
	if (moves.forCancels) return "Ms. Confille Reload";
	return !ent.inNewMoveSection ? "Ms. Confille Reload Until Able to Cancel" : "Ms. Confille Reload Until Able to Fire";
}
const char* displaySlangNameSelector_rifleReload(PlayerInfo& ent) {
	if (ent.idle) return "Rifle";
	const char* response = moves.rifleAutoExit(ent, &moves.elpheltRifleReloadEndMarkerOffset, "Rifle Autoexit");
	if (response) return response;
	if (moves.forCancels) return "Reload";
	return !ent.inNewMoveSection ? "Reload Until Able to Cancel" : "Reload Until Able to Fire";
}
const char* displayNameSelector_riflePerfectReload(PlayerInfo& ent) {
	if (ent.idle) return "Ms. Confille";
	const char* response = moves.rifleAutoExit(ent, &moves.elpheltRifleReloadPerfectEndMarkerOffset, "Ms. Confille Autoexit");
	if (response) return response;
	if (moves.forCancels) return "Ms. Confille Perfect Reload";
	return !ent.inNewMoveSection ? "Ms. Confille Perfect Reload Until Able to Cancel" : "Ms. Confille Perfect Reload Until Able to Fire";
}
const char* displaySlangNameSelector_riflePerfectReload(PlayerInfo& ent) {
	if (ent.idle) return "Rifle";
	const char* response = moves.rifleAutoExit(ent, &moves.elpheltRifleReloadPerfectEndMarkerOffset, "Rifle Autoexit");
	if (response) return response;
	if (moves.forCancels) return "Perfect Reload";
	return !ent.inNewMoveSection ? "Perfect Reload Until Able to Cancel" : "Perfect Reload Until Able to Fire";
}
const char* displayNameSelector_rifleRC(PlayerInfo& ent) {
	if (ent.idle) return "Ms. Confille";
	const char* response = moves.rifleAutoExit(ent, &moves.elpheltRifleRomanEndMarkerOffset, "Ms. Confille Autoexit");
	if (response) return response;
	if (moves.forCancels) return "Ms. Confille Roman Cancel";
	return !ent.inNewMoveSection ? "Ms. Confille Roman Cancel Until Able to Cancel" : "Ms. Confille Roman Cancel Until Able to Fire";
}
const char* displaySlangNameSelector_rifleRC(PlayerInfo& ent) {
	if (ent.idle) return "Rifle";
	const char* response = moves.rifleAutoExit(ent, &moves.elpheltRifleRomanEndMarkerOffset, "Rifle Autoexit");
	if (response) return response;
	if (moves.forCancels) return "Rifle RC";
	return !ent.inNewMoveSection ? "Rifle RC Until Able to Cancel" : "Rifle RC Until Able to Fire";
}
const char* displayNameSelector_mistEntry(PlayerInfo& ent) {
	int type = ent.pawn.mem53();
	int lvl = ent.pawn.playerVal(1);
	if (type == 0) {
		if (lvl == 0) return "Lv1 P Mist Finer Entry";
		if (lvl == 1) return "Lv2 P Mist Finer Entry";
		if (lvl == 2) return "Lv3 P Mist Finer Entry";
	}
	if (type == 1) {
		if (lvl == 0) return "Lv1 K Mist Finer Entry";
		if (lvl == 1) return "Lv2 K Mist Finer Entry";
		if (lvl == 2) return "Lv3 K Mist Finer Entry";
	}
	if (type == 2) {
		if (lvl == 0) return "Lv1 S Mist Finer Entry";
		if (lvl == 1) return "Lv2 S Mist Finer Entry";
		if (lvl == 2) return "Lv3 S Mist Finer Entry";
	}
	return "Mist Finer Entry";
}
const char* displaySlangNameSelector_mistEntry(PlayerInfo& ent) {
	int type = ent.pawn.mem53();
	int lvl = ent.pawn.playerVal(1);
	if (type == 0) {
		if (lvl == 0) return "Lv1 PMF Entry";
		if (lvl == 1) return "Lv2 PMF Entry";
		if (lvl == 2) return "Lv3 PMF Entry";
	}
	if (type == 1) {
		if (lvl == 0) return "Lv1 KMF Entry";
		if (lvl == 1) return "Lv2 KMF Entry";
		if (lvl == 2) return "Lv3 KMF Entry";
	}
	if (type == 2) {
		if (lvl == 0) return "Lv1 SMF Entry";
		if (lvl == 1) return "Lv2 SMF Entry";
		if (lvl == 2) return "Lv3 SMF Entry";
	}
	return "MF Entry";
}
const char* displayNameSelector_mistLoop(PlayerInfo& ent) {
	int type = ent.pawn.mem53();
	int lvl = ent.pawn.playerVal(1);
	if (ent.pawn.mem54()) {
		// it is possible to end up here only in Rev1
		if (type == 0) {
			if (lvl == 0) return "Lv1 P Mist Finer";
			if (lvl == 1) return "Lv2 P Mist Finer";
			if (lvl == 2) return "Lv3 P Mist Finer";
		}
		if (type == 1) {
			if (lvl == 0) return "Lv1 K Mist Finer";
			if (lvl == 1) return "Lv2 K Mist Finer";
			if (lvl == 2) return "Lv3 K Mist Finer";
		}
		if (type == 2) {
			if (lvl == 0) return "Lv1 S Mist Finer";
			if (lvl == 1) return "Lv2 S Mist Finer";
			if (lvl == 2) return "Lv3 S Mist Finer";
		}
		return "Mist Finer";
	}
	if (!ent.inNewMoveSection) {
		if (type == 0) {
			if (lvl == 0) return "Lv1 P Mist Finer Entry";
			if (lvl == 1) return "Lv2 P Mist Finer Entry";
			if (lvl == 2) return "Lv3 P Mist Finer Entry";
		}
		if (type == 1) {
			if (lvl == 0) return "Lv1 K Mist Finer Entry";
			if (lvl == 1) return "Lv2 K Mist Finer Entry";
			if (lvl == 2) return "Lv3 K Mist Finer Entry";
		}
		if (type == 2) {
			if (lvl == 0) return "Lv1 S Mist Finer Entry";
			if (lvl == 1) return "Lv2 S Mist Finer Entry";
			if (lvl == 2) return "Lv3 S Mist Finer Entry";
		}
		return "Mist Finer Entry";
	}
	if (type == 0) {
		if (lvl == 0) return "Lv1 P Mist Finer Stance";
		if (lvl == 1) return "Lv2 P Mist Finer Stance";
		if (lvl == 2) return "Lv3 P Mist Finer Stance";
	}
	if (type == 1) {
		if (lvl == 0) return "Lv1 K Mist Finer Stance";
		if (lvl == 1) return "Lv2 K Mist Finer Stance";
		if (lvl == 2) return "Lv3 K Mist Finer Stance";
	}
	if (type == 2) {
		if (lvl == 0) return "Lv1 S Mist Finer Stance";
		if (lvl == 1) return "Lv2 S Mist Finer Stance";
		if (lvl == 2) return "Lv3 S Mist Finer Stance";
	}
	return "Mist Finer Stance";
}
const char* displaySlangNameSelector_mistLoop(PlayerInfo& ent) {
	int type = ent.pawn.mem53();
	int lvl = ent.pawn.playerVal(1);
	if (ent.pawn.mem54()) {
		// it is possible to end up here only in Rev1
		if (type == 0) {
			if (lvl == 0) return "Lv1 PMF";
			if (lvl == 1) return "Lv2 PMF";
			if (lvl == 2) return "Lv3 PMF";
		}
		if (type == 1) {
			if (lvl == 0) return "Lv1 KMF";
			if (lvl == 1) return "Lv2 KMF";
			if (lvl == 2) return "Lv3 KMF";
		}
		if (type == 2) {
			if (lvl == 0) return "Lv1 SMF";
			if (lvl == 1) return "Lv2 SMF";
			if (lvl == 2) return "Lv3 SMF";
		}
		return "MF";
	}
	if (!ent.inNewMoveSection) {
		if (type == 0) {
			if (lvl == 0) return "Lv1 PMF Entry";
			if (lvl == 1) return "Lv2 PMF Entry";
			if (lvl == 2) return "Lv3 PMF Entry";
		}
		if (type == 1) {
			if (lvl == 0) return "Lv1 KMF Entry";
			if (lvl == 1) return "Lv2 KMF Entry";
			if (lvl == 2) return "Lv3 KMF Entry";
		}
		if (type == 2) {
			if (lvl == 0) return "Lv1 SMF Entry";
			if (lvl == 1) return "Lv2 SMF Entry";
			if (lvl == 2) return "Lv3 SMF Entry";
		}
		return "MF Entry";
	}
	if (type == 0) {
		if (lvl == 0) return "Lv1 PMF Stance";
		if (lvl == 1) return "Lv2 PMF Stance";
		if (lvl == 2) return "Lv3 PMF Stance";
	}
	if (type == 1) {
		if (lvl == 0) return "Lv1 KMF Stance";
		if (lvl == 1) return "Lv2 KMF Stance";
		if (lvl == 2) return "Lv3 KMF Stance";
	}
	if (type == 2) {
		if (lvl == 0) return "Lv1 SMF Stance";
		if (lvl == 1) return "Lv2 SMF Stance";
		if (lvl == 2) return "Lv3 SMF Stance";
	}
	return "MF Stance";
}
const char* displayNameSelector_mistWalkForward(PlayerInfo& ent) {
	int type = ent.pawn.mem53();
	int lvl = ent.pawn.playerVal(1);
	if (type == 0) {
		if (lvl == 0) return "Lv1 P Mist Finer Walk Forward";
		if (lvl == 1) return "Lv2 P Mist Finer Walk Forward";
		if (lvl == 2) return "Lv3 P Mist Finer Walk Forward";
	}
	if (type == 1) {
		if (lvl == 0) return "Lv1 K Mist Finer Walk Forward";
		if (lvl == 1) return "Lv2 K Mist Finer Walk Forward";
		if (lvl == 2) return "Lv3 K Mist Finer Walk Forward";
	}
	if (type == 2) {
		if (lvl == 0) return "Lv1 S Mist Finer Walk Forward";
		if (lvl == 1) return "Lv2 S Mist Finer Walk Forward";
		if (lvl == 2) return "Lv3 S Mist Finer Walk Forward";
	}
	return "Mist Finer Walk Forward";
}
const char* displaySlangNameSelector_mistWalkForward(PlayerInfo& ent) {
	int type = ent.pawn.mem53();
	int lvl = ent.pawn.playerVal(1);
	if (type == 0) {
		if (lvl == 0) return "Lv1 PMF Walk Forward";
		if (lvl == 1) return "Lv2 PMF Walk Forward";
		if (lvl == 2) return "Lv3 PMF Walk Forward";
	}
	if (type == 1) {
		if (lvl == 0) return "Lv1 KMF Walk Forward";
		if (lvl == 1) return "Lv2 KMF Walk Forward";
		if (lvl == 2) return "Lv3 KMF Walk Forward";
	}
	if (type == 2) {
		if (lvl == 0) return "Lv1 SMF Walk Forward";
		if (lvl == 1) return "Lv2 SMF Walk Forward";
		if (lvl == 2) return "Lv3 SMF Walk Forward";
	}
	return "MF Walk Forward";
}
const char* displayNameSelector_mistWalkBackward(PlayerInfo& ent) {
	int type = ent.pawn.mem53();
	int lvl = ent.pawn.playerVal(1);
	if (type == 0) {
		if (lvl == 0) return "Lv1 P Mist Finer Walk Backward";
		if (lvl == 1) return "Lv2 P Mist Finer Walk Backward";
		if (lvl == 2) return "Lv3 P Mist Finer Walk Backward";
	}
	if (type == 1) {
		if (lvl == 0) return "Lv1 K Mist Finer Walk Backward";
		if (lvl == 1) return "Lv2 K Mist Finer Walk Backward";
		if (lvl == 2) return "Lv3 K Mist Finer Walk Backward";
	}
	if (type == 2) {
		if (lvl == 0) return "Lv1 S Mist Finer Walk Backward";
		if (lvl == 1) return "Lv2 S Mist Finer Walk Backward";
		if (lvl == 2) return "Lv3 S Mist Finer Walk Backward";
	}
	return "Mist Finer Walk Backward";
}
const char* displaySlangNameSelector_mistWalkBackward(PlayerInfo& ent) {
	int type = ent.pawn.mem53();
	int lvl = ent.pawn.playerVal(1);
	if (type == 0) {
		if (lvl == 0) return "Lv1 PMF Walk Backward";
		if (lvl == 1) return "Lv2 PMF Walk Backward";
		if (lvl == 2) return "Lv3 PMF Walk Backward";
	}
	if (type == 1) {
		if (lvl == 0) return "Lv1 KMF Walk Backward";
		if (lvl == 1) return "Lv2 KMF Walk Backward";
		if (lvl == 2) return "Lv3 KMF Walk Backward";
	}
	if (type == 2) {
		if (lvl == 0) return "Lv1 SMF Walk Backward";
		if (lvl == 1) return "Lv2 SMF Walk Backward";
		if (lvl == 2) return "Lv3 SMF Walk Backward";
	}
	return "MF Walk Backward";
}
const char* displayNameSelector_mistDash(PlayerInfo& ent) {
	int type = ent.pawn.mem53();
	int lvl = ent.pawn.playerVal(1);
	if (type == 0) {
		if (lvl == 0) return "Lv1 P Mist Finer Dash";
		if (lvl == 1) return "Lv2 P Mist Finer Dash";
		if (lvl == 2) return "Lv3 P Mist Finer Dash";
	}
	if (type == 1) {
		if (lvl == 0) return "Lv1 K Mist Finer Dash";
		if (lvl == 1) return "Lv2 K Mist Finer Dash";
		if (lvl == 2) return "Lv3 K Mist Finer Dash";
	}
	if (type == 2) {
		if (lvl == 0) return "Lv1 S Mist Finer Dash";
		if (lvl == 1) return "Lv2 S Mist Finer Dash";
		if (lvl == 2) return "Lv3 S Mist Finer Dash";
	}
	return "Mist Finer Dash";
}
const char* displaySlangNameSelector_mistDash(PlayerInfo& ent) {
	int type = ent.pawn.mem53();
	int lvl = ent.pawn.playerVal(1);
	if (type == 0) {
		if (lvl == 0) return "Lv1 PMF Dash";
		if (lvl == 1) return "Lv2 PMF Dash";
		if (lvl == 2) return "Lv3 PMF Dash";
	}
	if (type == 1) {
		if (lvl == 0) return "Lv1 KMF Dash";
		if (lvl == 1) return "Lv2 KMF Dash";
		if (lvl == 2) return "Lv3 KMF Dash";
	}
	if (type == 2) {
		if (lvl == 0) return "Lv1 SMF Dash";
		if (lvl == 1) return "Lv2 SMF Dash";
		if (lvl == 2) return "Lv3 SMF Dash";
	}
	return "MF Dash";
}
const char* displayNameSelector_mistBackdash(PlayerInfo& ent) {
	int type = ent.pawn.mem53();
	int lvl = ent.pawn.playerVal(1);
	if (type == 0) {
		if (lvl == 0) return "Lv1 P Mist Finer Backdash";
		if (lvl == 1) return "Lv2 P Mist Finer Backdash";
		if (lvl == 2) return "Lv3 P Mist Finer Backdash";
	}
	if (type == 1) {
		if (lvl == 0) return "Lv1 K Mist Finer Backdash";
		if (lvl == 1) return "Lv2 K Mist Finer Backdash";
		if (lvl == 2) return "Lv3 K Mist Finer Backdash";
	}
	if (type == 2) {
		if (lvl == 0) return "Lv1 S Mist Finer Backdash";
		if (lvl == 1) return "Lv2 S Mist Finer Backdash";
		if (lvl == 2) return "Lv3 S Mist Finer Backdash";
	}
	return "Mist Finer Backdash";
}
const char* displaySlangNameSelector_mistBackdash(PlayerInfo& ent) {
	int type = ent.pawn.mem53();
	int lvl = ent.pawn.playerVal(1);
	if (type == 0) {
		if (lvl == 0) return "Lv1 PMF Backdash";
		if (lvl == 1) return "Lv2 PMF Backdash";
		if (lvl == 2) return "Lv3 PMF Backdash";
	}
	if (type == 1) {
		if (lvl == 0) return "Lv1 KMF Backdash";
		if (lvl == 1) return "Lv2 KMF Backdash";
		if (lvl == 2) return "Lv3 KMF Backdash";
	}
	if (type == 2) {
		if (lvl == 0) return "Lv1 SMF Backdash";
		if (lvl == 1) return "Lv2 SMF Backdash";
		if (lvl == 2) return "Lv3 SMF Backdash";
	}
	return "MF Backdash";
}
const char* displayNameSelector_airMistEntry(PlayerInfo& ent) {
	int type = ent.pawn.mem53();
	int lvl = ent.pawn.playerVal(1);
	if (type == 0) {
		if (lvl == 0) return "Lv1 Air P Mist Finer Entry";
		if (lvl == 1) return "Lv2 Air P Mist Finer Entry";
		if (lvl == 2) return "Lv3 Air P Mist Finer Entry";
	}
	if (type == 1) {
		if (lvl == 0) return "Lv1 Air K Mist Finer Entry";
		if (lvl == 1) return "Lv2 Air K Mist Finer Entry";
		if (lvl == 2) return "Lv3 Air K Mist Finer Entry";
	}
	if (type == 2) {
		if (lvl == 0) return "Lv1 Air S Mist Finer Entry";
		if (lvl == 1) return "Lv2 Air S Mist Finer Entry";
		if (lvl == 2) return "Lv3 Air S Mist Finer Entry";
	}
	return "Air Mist Finer Entry";
}
const char* displaySlangNameSelector_airMistEntry(PlayerInfo& ent) {
	int type = ent.pawn.mem53();
	int lvl = ent.pawn.playerVal(1);
	if (type == 0) {
		if (lvl == 0) return "Lv1 j.PMF Entry";
		if (lvl == 1) return "Lv2 j.PMF Entry";
		if (lvl == 2) return "Lv3 j.PMF Entry";
	}
	if (type == 1) {
		if (lvl == 0) return "Lv1 j.KMF Entry";
		if (lvl == 1) return "Lv2 j.KMF Entry";
		if (lvl == 2) return "Lv3 j.KMF Entry";
	}
	if (type == 2) {
		if (lvl == 0) return "Lv1 j.SMF Entry";
		if (lvl == 1) return "Lv2 j.SMF Entry";
		if (lvl == 2) return "Lv3 j.SMF Entry";
	}
	return "j.MF Entry";
}
const char* displayNameSelector_airMistLoop(PlayerInfo& ent) {
	int type = ent.pawn.mem53();
	int lvl = ent.pawn.playerVal(1);
	if (ent.pawn.mem54()) {
		// it is possible to end up here only in Rev1
		if (type == 0) {
			if (lvl == 0) return "Lv1 Air P Mist Finer";
			if (lvl == 1) return "Lv2 Air P Mist Finer";
			if (lvl == 2) return "Lv3 Air P Mist Finer";
		}
		if (type == 1) {
			if (lvl == 0) return "Lv1 Air K Mist Finer";
			if (lvl == 1) return "Lv2 Air K Mist Finer";
			if (lvl == 2) return "Lv3 Air K Mist Finer";
		}
		if (type == 2) {
			if (lvl == 0) return "Lv1 Air S Mist Finer";
			if (lvl == 1) return "Lv2 Air S Mist Finer";
			if (lvl == 2) return "Lv3 Air S Mist Finer";
		}
		return "Air Mist Finer";
	}
	if (!ent.inNewMoveSection) {
		if (type == 0) {
			if (lvl == 0) return "Lv1 Air P Mist Finer Entry";
			if (lvl == 1) return "Lv2 Air P Mist Finer Entry";
			if (lvl == 2) return "Lv3 Air P Mist Finer Entry";
		}
		if (type == 1) {
			if (lvl == 0) return "Lv1 Air K Mist Finer Entry";
			if (lvl == 1) return "Lv2 Air K Mist Finer Entry";
			if (lvl == 2) return "Lv3 Air K Mist Finer Entry";
		}
		if (type == 2) {
			if (lvl == 0) return "Lv1 Air S Mist Finer Entry";
			if (lvl == 1) return "Lv2 Air S Mist Finer Entry";
			if (lvl == 2) return "Lv3 Air S Mist Finer Entry";
		}
		return "Air Mist Finer Entry";
	}
	if (type == 0) {
		if (lvl == 0) return "Lv1 Air P Mist Finer Stance";
		if (lvl == 1) return "Lv2 Air P Mist Finer Stance";
		if (lvl == 2) return "Lv3 Air P Mist Finer Stance";
	}
	if (type == 1) {
		if (lvl == 0) return "Lv1 Air K Mist Finer Stance";
		if (lvl == 1) return "Lv2 Air K Mist Finer Stance";
		if (lvl == 2) return "Lv3 Air K Mist Finer Stance";
	}
	if (type == 2) {
		if (lvl == 0) return "Lv1 Air S Mist Finer Stance";
		if (lvl == 1) return "Lv2 Air S Mist Finer Stance";
		if (lvl == 2) return "Lv3 Air S Mist Finer Stance";
	}
	return "Air Mist Finer Stance";
}
const char* displaySlangNameSelector_airMistLoop(PlayerInfo& ent) {
	int type = ent.pawn.mem53();
	int lvl = ent.pawn.playerVal(1);
	if (ent.pawn.mem54()) {
		// it is possible to end up here only in Rev1
		if (type == 0) {
			if (lvl == 0) return "Lv1 j.PMF";
			if (lvl == 1) return "Lv2 j.PMF";
			if (lvl == 2) return "Lv3 j.PMF";
		}
		if (type == 1) {
			if (lvl == 0) return "Lv1 j.KMF";
			if (lvl == 1) return "Lv2 j.KMF";
			if (lvl == 2) return "Lv3 j.KMF";
		}
		if (type == 2) {
			if (lvl == 0) return "Lv1 j.SMF";
			if (lvl == 1) return "Lv2 j.SMF";
			if (lvl == 2) return "Lv3 j.SMF";
		}
		return "j.MF";
	}
	if (!ent.inNewMoveSection) {
		if (type == 0) {
			if (lvl == 0) return "Lv1 j.PMF Entry";
			if (lvl == 1) return "Lv2 j.PMF Entry";
			if (lvl == 2) return "Lv3 j.PMF Entry";
		}
		if (type == 1) {
			if (lvl == 0) return "Lv1 j.KMF Entry";
			if (lvl == 1) return "Lv2 j.KMF Entry";
			if (lvl == 2) return "Lv3 j.KMF Entry";
		}
		if (type == 2) {
			if (lvl == 0) return "Lv1 j.SMF Entry";
			if (lvl == 1) return "Lv2 j.SMF Entry";
			if (lvl == 2) return "Lv3 j.SMF Entry";
		}
		return "j.MF Entry";
	}
	if (type == 0) {
		if (lvl == 0) return "Lv1 j.PMF Stance";
		if (lvl == 1) return "Lv2 j.PMF Stance";
		if (lvl == 2) return "Lv3 j.PMF Stance";
	}
	if (type == 1) {
		if (lvl == 0) return "Lv1 j.KMF Stance";
		if (lvl == 1) return "Lv2 j.KMF Stance";
		if (lvl == 2) return "Lv3 j.KMF Stance";
	}
	if (type == 2) {
		if (lvl == 0) return "Lv1 j.SMF Stance";
		if (lvl == 1) return "Lv2 j.SMF Stance";
		if (lvl == 2) return "Lv3 j.SMF Stance";
	}
	return "j.MF Stance";
}
const char* displayNameSelector_airMistDash(PlayerInfo& ent) {
	int type = ent.pawn.mem53();
	int lvl = ent.pawn.playerVal(1);
	if (type == 0) {
		if (lvl == 0) return "Lv1 Air P Mist Finer Airdash";
		if (lvl == 1) return "Lv2 Air P Mist Finer Airdash";
		if (lvl == 2) return "Lv3 Air P Mist Finer Airdash";
	}
	if (type == 1) {
		if (lvl == 0) return "Lv1 Air K Mist Finer Airdash";
		if (lvl == 1) return "Lv2 Air K Mist Finer Airdash";
		if (lvl == 2) return "Lv3 Air K Mist Finer Airdash";
	}
	if (type == 2) {
		if (lvl == 0) return "Lv1 Air S Mist Finer Airdash";
		if (lvl == 1) return "Lv2 Air S Mist Finer Airdash";
		if (lvl == 2) return "Lv3 Air S Mist Finer Airdash";
	}
	return "Air Mist Finer Airdash";
}
const char* displaySlangNameSelector_airMistDash(PlayerInfo& ent) {
	int type = ent.pawn.mem53();
	int lvl = ent.pawn.playerVal(1);
	if (type == 0) {
		if (lvl == 0) return "Lv1 j.PMF Airdash";
		if (lvl == 1) return "Lv2 j.PMF Airdash";
		if (lvl == 2) return "Lv3 j.PMF Airdash";
	}
	if (type == 1) {
		if (lvl == 0) return "Lv1 j.KMF Airdash";
		if (lvl == 1) return "Lv2 j.KMF Airdash";
		if (lvl == 2) return "Lv3 j.KMF Airdash";
	}
	if (type == 2) {
		if (lvl == 0) return "Lv1 j.SMF Airdash";
		if (lvl == 1) return "Lv2 j.SMF Airdash";
		if (lvl == 2) return "Lv3 j.SMF Airdash";
	}
	return "j.MF Airdash";
}
const char* displayNameSelector_airMistBackdash(PlayerInfo& ent) {
	int type = ent.pawn.mem53();
	int lvl = ent.pawn.playerVal(1);
	if (type == 0) {
		if (lvl == 0) return "Lv1 Air P Mist Finer Airdash Back";
		if (lvl == 1) return "Lv2 Air P Mist Finer Airdash Back";
		if (lvl == 2) return "Lv3 Air P Mist Finer Airdash Back";
	}
	if (type == 1) {
		if (lvl == 0) return "Lv1 Air K Mist Finer Airdash Back";
		if (lvl == 1) return "Lv2 Air K Mist Finer Airdash Back";
		if (lvl == 2) return "Lv3 Air K Mist Finer Airdash Back";
	}
	if (type == 2) {
		if (lvl == 0) return "Lv1 Air S Mist Finer Airdash Back";
		if (lvl == 1) return "Lv2 Air S Mist Finer Airdash Back";
		if (lvl == 2) return "Lv3 Air S Mist Finer Airdash Back";
	}
	return "Air Mist Finer Airdash Back";
}
const char* displaySlangNameSelector_airMistBackdash(PlayerInfo& ent) {
	int type = ent.pawn.mem53();
	int lvl = ent.pawn.playerVal(1);
	if (type == 0) {
		if (lvl == 0) return "Lv1 j.PMF Airdash Back";
		if (lvl == 1) return "Lv2 j.PMF Airdash Back";
		if (lvl == 2) return "Lv3 j.PMF Airdash Back";
	}
	if (type == 1) {
		if (lvl == 0) return "Lv1 j.KMF Airdash Back";
		if (lvl == 1) return "Lv2 j.KMF Airdash Back";
		if (lvl == 2) return "Lv3 j.KMF Airdash Back";
	}
	if (type == 2) {
		if (lvl == 0) return "Lv1 j.SMF Airdash Back";
		if (lvl == 1) return "Lv2 j.SMF Airdash Back";
		if (lvl == 2) return "Lv3 j.SMF Airdash Back";
	}
	return "j.MF Airdash Back";
}
const char* displayNameSelector_gekirinLv2or3(PlayerInfo& ent) {
	if (strcmp(ent.labelAtTheStartOfTheMove, "Lv3Gekirin") == 0) {
		return "Lv3 Gekirin";
	}
	return "Lv2 Gekirin";
}
const char* displayNameSelector_airGekirinLv2or3(PlayerInfo& ent) {
	if (strcmp(ent.labelAtTheStartOfTheMove, "Lv3Gekirin") == 0) {
		return "Lv3 Air Gekirin";
	}
	return "Lv2 Air Gekirin";
}
const char* displayNameSelector_ryujinLv2or3(PlayerInfo& ent) {
	if (strcmp(ent.labelAtTheStartOfTheMove, "Lv3Ryujin") == 0) {
		return "Lv3 Ryuujin";
	}
	return "Lv2 Ryuujin";
}
const char* displayNameSelector_airRyujinLv2or3(PlayerInfo& ent) {
	if (strcmp(ent.labelAtTheStartOfTheMove, "Lv3Ryujin") == 0) {
		return "Lv3 Air Ryuujin";
	}
	return "Lv2 Air Ryuujin";
}
const char* displayNameSelector_kenroukakuLv2or3(PlayerInfo& ent) {
	if (strcmp(ent.labelAtTheStartOfTheMove, "Lv3Kenroukaku") == 0) {
		return "Lv3 Kenroukaku";
	}
	return "Lv2 Kenroukaku";
}
const char* displayNameSelector_airKenroukakuLv2or3(PlayerInfo& ent) {
	if (strcmp(ent.labelAtTheStartOfTheMove, "Lv3Kenroukaku") == 0) {
		return "Lv3 Air Kenroukaku";
	}
	return "Lv2 Air Kenroukaku";
}
const char* displayNameSelector_standingAzami(PlayerInfo& ent) {
	return ent.pawn.mem46() ? "Standing Red Azami" : "Standing Azami";
}
const char* displayNameSelector_crouchingAzami(PlayerInfo& ent) {
	return ent.pawn.mem46() ? "Crouching Red Azami" : "Crouching Azami";
}
const char* displayNameSelector_airAzami(PlayerInfo& ent) {
	return ent.pawn.mem46() ? "Aerial Red Azami" : "Aerial Azami";
}
const char* displayNameSelector_gunflame(PlayerInfo& ent) {
	if (strcmp(ent.pawn.previousAnimName(), "CmnActFDash") == 0) {
		return "Runflame";
	}
	return "Gunflame";
}
const char* displaySlangNameSelector_gunflame(PlayerInfo& ent) {
	if (strcmp(ent.pawn.previousAnimName(), "CmnActFDash") == 0) {
		return "Runflame";
	}
	return "GF";
}
const char* displayNameSelector_gunflameDI(PlayerInfo& ent) {
	if (strcmp(ent.pawn.previousAnimName(), "CmnActFDash") == 0) {
		return "DI Runflame";
	}
	return "DI Gunflame";
}
const char* displaySlangNameSelector_gunflameDI(PlayerInfo& ent) {
	if (strcmp(ent.pawn.previousAnimName(), "CmnActFDash") == 0) {
		return "DI Runflame";
	}
	return "DI GF";
}
const char* displayNameSelector_standingBlitzShield(PlayerInfo& ent) {
	GroundBlitzType type = Moves::getBlitzType(ent);
	switch (type) {
		case BLITZTYPE_MAXCHARGE: return "Max Charge Standing Blitz Shield";
		case BLITZTYPE_CHARGE: return "Charge Standing Blitz Shield";
		default: return "Tap Standing Blitz Shield";
	}
}
const char* displaySlangNameSelector_standingBlitzShield(PlayerInfo& ent) {
	GroundBlitzType type = Moves::getBlitzType(ent);
	switch (type) {
		case BLITZTYPE_MAXCHARGE: return "Max Standing Blitz";
		case BLITZTYPE_CHARGE: return "Charge Standing Blitz";
		default: return "Tap Standing Blitz";
	}
}
const char* displayNameSelector_crouchingBlitzShield(PlayerInfo& ent) {
	GroundBlitzType type = Moves::getBlitzType(ent);
	switch (type) {
		case BLITZTYPE_MAXCHARGE: return "Max Charge Crouching Blitz Shield";
		case BLITZTYPE_CHARGE: return "Charge Crouching Blitz Shield";
		default: return "Tap Crouching Blitz Shield";
	}
}
const char* displaySlangNameSelector_crouchingBlitzShield(PlayerInfo& ent) {
	GroundBlitzType type = Moves::getBlitzType(ent);
	switch (type) {
		case BLITZTYPE_MAXCHARGE: return "Max Crouching Blitz";
		case BLITZTYPE_CHARGE: return "Charge Crouching Blitz";
		default: return "Tap Crouching Blitz";
	}
}
const char* displayNameSelector_pilebunker(PlayerInfo& ent) {
	if (strcmp(ent.pawn.previousAnimName(), "DandyStepA") == 0) {
		return "P Pilebunker";
	}
	return "K Pilebunker";
}
const char* displaySlangNameSelector_pilebunker(PlayerInfo& ent) {
	if (strcmp(ent.pawn.previousAnimName(), "DandyStepA") == 0) {
		return "P Pile";
	}
	return "K Pile";
}
const char* displayNameSelector_crosswise(PlayerInfo& ent) {
	if (strcmp(ent.pawn.previousAnimName(), "DandyStepA") == 0) {
		return "P Crosswise Heel";
	}
	return "K Crosswise Heel";
}
const char* displaySlangNameSelector_crosswise(PlayerInfo& ent) {
	if (strcmp(ent.pawn.previousAnimName(), "DandyStepA") == 0) {
		return "P CW";
	}
	return "K CW";
}
const char* displayNameSelector_underPressure(PlayerInfo& ent) {
	if (strcmp(ent.pawn.previousAnimName(), "DandyStepA") == 0) {
		return "P Under Pressure";
	}
	return "K Under Pressure";
}
const char* displaySlangNameSelector_underPressure(PlayerInfo& ent) {
	if (strcmp(ent.pawn.previousAnimName(), "DandyStepA") == 0) {
		return "P UP";
	}
	return "K UP";
}
const char* displayNameSelector_jacko4D(PlayerInfo& ent) {
	int x = ent.pawn.hitAirPushbackX();
	int y = ent.pawn.hitAirPushbackY();
	if (x == 0 && y == -30000 && ent.pawn.groundBounceCount() == INT_MAX) return "4D";
	y -= 20000;
	
	enum Dir {
		DIRX_PLUS = 0,
		DIRX_NEUTRAL = 1,
		DIRX_MINUS = 2,
		DIRY_PLUS = (0 << 4),
		DIRY_NEUTRAL = (1 << 4),
		DIRY_MINUS = (2 << 4)
	};
	
	Dir dirX;
	Dir dirY;
	
	if (x > 0) dirX = DIRX_PLUS;
	else if (x == 0) dirX = DIRX_NEUTRAL;
	else dirX = DIRX_MINUS;
	
	if (y > 0) dirY = DIRY_PLUS;
	else if (y == 0) dirY = DIRY_NEUTRAL;
	else dirY = DIRY_MINUS;
	
	switch (
		dirX | dirY
	) {
		case DIRX_MINUS | DIRY_MINUS: return "4D1";
		case DIRX_NEUTRAL | DIRY_MINUS: return "4D2";
		case DIRX_PLUS | DIRY_MINUS: return "4D3";
		case DIRX_MINUS | DIRY_NEUTRAL: return "4D4";
		case DIRX_PLUS | DIRY_NEUTRAL: return "4D6";
		case DIRX_MINUS | DIRY_PLUS: return "4D7";
		case DIRX_NEUTRAL | DIRY_PLUS: return "4D8";
		case DIRX_PLUS | DIRY_PLUS: return "4D9";
		default: return "4D";
	}
}
const char* displayNameSelector_jackoj4D(PlayerInfo& ent) {
	int x = ent.pawn.hitAirPushbackX();
	int y = ent.pawn.hitAirPushbackY();
	if (x == 0 && y == -30000 && ent.pawn.groundBounceCount() == INT_MAX) return "j.4D";
	y -= 25000;
	
	enum Dir {
		DIRX_PLUS = 0,
		DIRX_NEUTRAL = 1,
		DIRX_MINUS = 2,
		DIRY_PLUS = (0 << 4),
		DIRY_NEUTRAL = (1 << 4),
		DIRY_MINUS = (2 << 4)
	};
	
	Dir dirX;
	Dir dirY;
	
	if (x > 0) dirX = DIRX_PLUS;
	else if (x == 0) dirX = DIRX_NEUTRAL;
	else dirX = DIRX_MINUS;
	
	if (y > 0) dirY = DIRY_PLUS;
	else if (y == 0) dirY = DIRY_NEUTRAL;
	else dirY = DIRY_MINUS;
	
	switch (
		dirX | dirY
	) {
		case DIRX_MINUS | DIRY_MINUS: return "j.4D1";
		case DIRX_NEUTRAL | DIRY_MINUS: return "j.4D2";
		case DIRX_PLUS | DIRY_MINUS: return "j.4D3";
		case DIRX_MINUS | DIRY_NEUTRAL: return "j.4D4";
		case DIRX_PLUS | DIRY_NEUTRAL: return "j.4D6";
		case DIRX_MINUS | DIRY_PLUS: return "j.4D7";
		case DIRX_NEUTRAL | DIRY_PLUS: return "j.4D8";
		case DIRX_PLUS | DIRY_PLUS: return "j.4D9";
		default: return "j.4D";
	}
}

bool canYrcProjectile_default(PlayerInfo& player) {
	return player.prevFrameHadDangerousNonDisabledProjectiles
		&& player.hasDangerousNonDisabledProjectiles;
}
bool canYrcProjectile_prevNoLinkDestroyOnStateChange(PlayerInfo& player) {
	return player.prevFrameHadDangerousNonDisabledProjectiles
		&& player.hasDangerousNonDisabledProjectiles
		&& (
			player.pawn.previousEntity() == nullptr
			|| player.pawn.previousEntity().linkObjectDestroyOnStateChange() != player.pawn
		)
		&& !player.prevFramePreviousEntityLinkObjectDestroyOnStateChangeWasEqualToPlayer;
}
bool createdProjectile_ky5D(PlayerInfo& player) {
	return player.pawn.previousEntity()
		&& player.pawn.previousEntity().lifeTimeCounter() == 0
		&& !player.pawn.isRCFrozen()
		&& (
			strcmp(player.pawn.previousEntity().animationName(), "Mahojin") == 0
			|| strcmp(player.pawn.previousEntity().animationName(), "DustEffectShot") == 0
		);
}
bool canYrcProjectile_ky5D(PlayerInfo& player) {
	// STACK_1 seems to hold the ground mahojin
	if (moves.ky5DDustEffectShot_firstSpriteAfter_Offset == -1) return false; // rev1
	BYTE* func = player.pawn.bbscrCurrentFunc();
	if (!moves.ky5DDustEffectShot_firstSpriteAfter_Offset) {
		BYTE* pos = moves.findCreateObj(func, "DustEffectShot");
		if (!pos) {
			moves.ky5DDustEffectShot_firstSpriteAfter_Offset = -1;
			return canYrcProjectile_default(player); // rev1
		}
		pos = moves.findSpriteNonNull(pos);
		if (pos) moves.ky5DDustEffectShot_firstSpriteAfter_Offset = pos - func;
	}
	if (!moves.ky5DDustEffectShot_firstSpriteAfter_Offset) return false;
	return player.pawn.bbscrCurrentInstr() > func + moves.ky5DDustEffectShot_firstSpriteAfter_Offset
		|| player.pawn.bbscrCurrentInstr() == func + moves.ky5DDustEffectShot_firstSpriteAfter_Offset
		&& player.pawn.spriteFrameCounter() > 0;
}
bool createdProjectile_splitCiel(PlayerInfo& player) {
	return player.pawn.previousEntity()
		&& strcmp(player.pawn.previousEntity().animationName(), "Mahojin") == 0
		&& player.pawn.previousEntity().lifeTimeCounter() == 0
		&& !player.pawn.isRCFrozen();
}
bool canYrcProjectile_splitCiel(PlayerInfo& player) {
	return player.pawn.previousEntity()
		&& strcmp(player.pawn.previousEntity().animationName(), "Mahojin") == 0
		&& player.pawn.previousEntity().lifeTimeCounter() > 0;
}
bool canYrcProjectile_flower(PlayerInfo& player) {
	for (int i = 2; i < entityList.count; ++i) {
		Entity p = entityList.list[i];
		if (p.isActive() && p.team() == player.index && !p.isPawn()
				&& (
					strcmp(p.animationName(), "OreHanaBig_Shot") == 0
					|| strcmp(p.animationName(), "OreHana_Shot") == 0
				)
				&& p.lifeTimeCounter() > 0
				&& !p.isActiveFrames()) {  // the last check against potential get on pogo -> flower -> YRC -> get on pogo -> flower again
			return true;
		}
	}
	return false;
}
bool canYrcProjectile_qv(PlayerInfo& player) {
	if (moves.venomQvClearUponAfterExitOffset == 0) {
		BYTE* func = player.pawn.bbscrCurrentFunc();
		for (BYTE* instr = moves.skipInstruction(func);
				moves.instructionType(instr) != instr_endState;
				instr = moves.skipInstruction(instr)) {
			if (moves.instructionType(instr) == instr_clearUpon && asInstr(instr, clearUpon)->event == BBSCREVENT_PLAYER_CHANGED_STATE) {
				moves.venomQvClearUponAfterExitOffset = instr - func;
				break;
			}
		}
	}
	if (!moves.venomQvClearUponAfterExitOffset) return false;
	BYTE* currentInstr = player.pawn.bbscrCurrentInstr();
	BYTE* minInstr = moves.skipInstruction(player.pawn.bbscrCurrentFunc() + moves.venomQvClearUponAfterExitOffset);
	return currentInstr == minInstr
		&& player.pawn.spriteFrameCounter() != 0
		|| currentInstr > minInstr;
}
bool createdProjectile_bishop(PlayerInfo& player) {
	return player.pawn.previousEntity() && player.pawn.previousEntity().lifeTimeCounter() == 0;
}
bool canYrcProjectile_bishop(PlayerInfo& player) {
	if (moves.venomBishopCreateOffset == 0) {
		BYTE* func = player.pawn.bbscrCurrentFunc();
		bool found = false;
		for (BYTE* instr = moves.skipInstruction(func);
				moves.instructionType(instr) != instr_endState;
				instr = moves.skipInstruction(instr)) {
			if (moves.instructionType(instr) == instr_createObjectWithArg && strcmp(asInstr(instr, createObjectWithArg)->state, "Ball") == 0) {
				found = true;
			} else if (found && moves.instructionType(instr) == instr_sprite) {
				moves.venomBishopCreateOffset = instr - func;
				break;
			}
		}
	}
	if (!moves.venomBishopCreateOffset) return false;
	return player.pawn.bbscrCurrentInstr() - player.pawn.bbscrCurrentFunc() > moves.venomBishopCreateOffset;
}
bool createdProjectile_ino5D(PlayerInfo& player) {
	return player.pawn.previousEntity()
		&& player.pawn.previousEntity().lifeTimeCounter() == 0
		&& !player.pawn.isRCFrozen()
		&& strcmp(player.pawn.previousEntity().animationName(), "DustObjShot") == 0;
}
bool canYrcProjectile_ino5D(PlayerInfo& player) {
	if (moves.ino5DCreateDustObjShotOffset == -1) return false; // rev1
	BYTE* func = player.pawn.bbscrCurrentFunc();
	if (!moves.ino5DCreateDustObjShotOffset) {
		BYTE* pos = moves.findCreateObj(func, "DustObjShot");
		if (!pos) {
			moves.ino5DCreateDustObjShotOffset = -1;
			return canYrcProjectile_default(player); // rev1
		}
		pos = moves.findSpriteNonNull(pos);
		if (pos) moves.ino5DCreateDustObjShotOffset = pos - func;
	}
	if (!moves.ino5DCreateDustObjShotOffset) return false;
	return player.pawn.bbscrCurrentInstr() > func + moves.ino5DCreateDustObjShotOffset
		|| player.pawn.bbscrCurrentInstr() == func + moves.ino5DCreateDustObjShotOffset
		&& player.pawn.spriteFrameCounter() > 0;
}
bool createdProjectile_onf5(PlayerInfo& ent) {
	return ent.pawn.currentAnimDuration() == 5 && !ent.pawn.isRCFrozen();
}
bool canYrcProjectile_onf5(PlayerInfo& ent) {
	return ent.pawn.currentAnimDuration() == 5 && ent.pawn.isRCFrozen()
		|| ent.pawn.currentAnimDuration() > 5;
}
bool createdProjectile_onf7(PlayerInfo& ent) {
	return ent.pawn.currentAnimDuration() == 7 && !ent.pawn.isRCFrozen();
}
bool canYrcProjectile_onf7(PlayerInfo& ent) {
	return ent.pawn.currentAnimDuration() == 7 && ent.pawn.isRCFrozen()
		|| ent.pawn.currentAnimDuration() > 7;
}
bool canYrcProjectile_onf9(PlayerInfo& ent) {
	return ent.pawn.currentAnimDuration() > 9;
}
bool createdProjectile_elpheltjD(PlayerInfo& player) {
	if (player.pawn.previousEntity()
			&& player.pawn.previousEntity().lifeTimeCounter() == 0
			&& !player.pawn.isRCFrozen()) {
		for (int i = 2; i < entityList.count; ++i) {
			Entity p = entityList.list[i];
			if (p.isActive() && p.team() == player.index && !p.isPawn()
					&& strcmp(p.animationName(), "HandGun_air_shot") == 0
					&& p.lifeTimeCounter() == 0) {
				return true;
			}
		}
	}
	return false;
}
bool canYrcProjectile_elpheltjD(PlayerInfo& player) {
	if (player.pawn.effectLinkedCollision() != nullptr
			&& player.pawn.previousEntity()
			&& player.pawn.previousEntity().lifeTimeCounter() > 0) {
		for (int i = 2; i < entityList.count; ++i) {
			Entity p = entityList.list[i];
			if (p.isActive() && p.team() == player.index && !p.isPawn()
					&& strcmp(p.animationName(), "HandGun_air_shot") == 0) {
				return true;
			}
		}
	}
	return false;
}
bool createdProjectile_Ghost(PlayerInfo& player, int index) {
	Entity p = player.pawn.stackEntity(index);
	return p && p.isActive() && p.displayModel() && p.posY() == 0 && p.mem45() != 1 && !player.pawn.isRCFrozen();
}
bool canYrcProjectile_Ghost(PlayerInfo& player, int index) {
	Entity p = player.pawn.stackEntity(index);
	return p && p.isActive() && p.displayModel() && p.mem45() == 1;
}
bool createdProjectile_PGhost(PlayerInfo& player) {
	return createdProjectile_Ghost(player, 0);
}
bool canYrcProjectile_PGhost(PlayerInfo& player) {
	return canYrcProjectile_Ghost(player, 0);
}
bool createdProjectile_KGhost(PlayerInfo& player) {
	return createdProjectile_Ghost(player, 1);
}
bool canYrcProjectile_KGhost(PlayerInfo& player) {
	return canYrcProjectile_Ghost(player, 1);
}
bool createdProjectile_SGhost(PlayerInfo& player) {
	return createdProjectile_Ghost(player, 2);
}
bool canYrcProjectile_SGhost(PlayerInfo& player) {
	return canYrcProjectile_Ghost(player, 2);
}
bool createdProjectile_XThrowGhost(PlayerInfo& player, int* offset) {
	BYTE* func = player.pawn.bbscrCurrentFunc();
	moves.fillJackoThrowGhostOffset(func, offset);
	return player.pawn.bbscrCurrentInstr() - func == *offset && !player.pawn.isRCFrozen() && player.pawn.spriteFrameCounter() == 0;
}
bool canYrcProjectile_XThrowGhost(PlayerInfo& player, int* offset) {
	int mem59 = player.pawn.mem59();
	if (mem59 != 1 && mem59 != 2 && mem59 != 3) return false;
	Entity p = player.pawn.stackEntity(mem59 - 1);
	if (!p || !p.isActive()) return false;
	return !createdProjectile_XThrowGhost(player, offset)
		&& p.isActiveFrames();
}
bool createdProjectile_ThrowGhost(PlayerInfo& player) {
	return createdProjectile_XThrowGhost(player, &moves.jackoThrowGhostOffset);
}
bool canYrcProjectile_ThrowGhost(PlayerInfo& player) {
	return canYrcProjectile_XThrowGhost(player, &moves.jackoThrowGhostOffset);
}
bool createdProjectile_AirThrowGhost(PlayerInfo& player) {
	return createdProjectile_XThrowGhost(player, &moves.jackoAirThrowGhostOffset);
}
bool canYrcProjectile_AirThrowGhost(PlayerInfo& player) {
	return canYrcProjectile_XThrowGhost(player, &moves.jackoAirThrowGhostOffset);
}
bool createdProjectile_kum5D(PlayerInfo& player) {
	return player.pawn.previousEntity()
		&& player.pawn.previousEntity().lifeTimeCounter() == 0
		&& !player.pawn.isRCFrozen()
		&& strcmp(player.pawn.previousEntity().animationName(), "kum_205shot") == 0;
}
bool canYrcProjectile_kum5D(PlayerInfo& player) {
	if (moves.kum5Dcreation == -1) return false; // rev1
	BYTE* func = player.pawn.bbscrCurrentFunc();
	if (!moves.kum5Dcreation) {
		BYTE* pos = moves.findCreateObj(func, "kum_205shot");
		if (!pos) {
			moves.kum5Dcreation = -1;
			return canYrcProjectile_default(player); // rev1
		}
		pos = moves.findSpriteNonNull(pos);
		if (pos) moves.kum5Dcreation = pos - func;
	}
	if (!moves.kum5Dcreation) return false;
	return player.pawn.bbscrCurrentInstr() > func + moves.kum5Dcreation
		|| player.pawn.bbscrCurrentInstr() == func + moves.kum5Dcreation
		&& player.pawn.spriteFrameCounter() > 0;
}
bool createdProjectile_baiken5D(PlayerInfo& player) {
	for (int i = 2; i < entityList.count; ++i) {
		Entity p = entityList.list[i];
		if (p.isActive() && p.team() == player.index && !p.isPawn() && strcmp(p.animationName(), "NmlAtk5EShotObj") == 0) {
			return p.lifeTimeCounter() == 0;
		}
	}
	return false;
}
bool canYrcProjectile_baiken5D(PlayerInfo& player) {
	if (moves.baiken5Dcreation == -1) return false; // error
	BYTE* func = player.pawn.bbscrCurrentFunc();
	if (!moves.baiken5Dcreation) {
		BYTE* pos = moves.findCreateObj(func, "NmlAtk5EShotObj");
		if (!pos) {
			moves.baiken5Dcreation = -1;
			return canYrcProjectile_default(player); // error
		}
		pos = moves.findSpriteNonNull(pos);
		if (pos) moves.baiken5Dcreation = pos - func;
	}
	if (!moves.baiken5Dcreation) return false;
	return player.pawn.bbscrCurrentInstr() > func + moves.baiken5Dcreation
		|| player.pawn.bbscrCurrentInstr() == func + moves.baiken5Dcreation
		&& player.pawn.spriteFrameCounter() > 0;
}
bool canYrcProjectile_scroll(PlayerInfo& player) {
	return player.pawn.currentAnimDuration() > 7
		|| player.pawn.currentAnimDuration() == 7
		&& !player.pawn.isSuperFrozen()
		&& player.pawn.isRCFrozen();
}
bool createdProjectile_firesale(PlayerInfo& player) {
	return player.pawn.currentAnimDuration() < 100
		? player.answerCreatedRSFStart
		: player.createdDangerousProjectile
						|| player.createdProjectileThatSometimesCanBeDangerous;
}
bool canYrcProjectile_firesale(PlayerInfo& player) {
	if (player.pawn.currentAnimDuration() < 100) {
		for (int i = 2; i < entityList.count; ++i) {
			Entity p = entityList.list[i];
			if (p.isActive() && p.team() == player.index && !p.isPawn()
					&& strcmp(p.animationName(), "RSF_Start") == 0) {
				return p.linkObjectDestroyOnStateChange() == nullptr;
			}
		}
		return false;
	} else {
		return canYrcProjectile_default(player);
	}
}

bool powerup_may6P(PlayerInfo& player) {
	return player.pawn.dealtAttack()->stun > player.prevFrameStunValue;
}
const char* powerupExplanation_may6P(PlayerInfo& player) {
	int stun = player.pawn.dealtAttack()->stun;
	if (stun == 110) {
		return "Base Stun Value increased from 88 to 110. Blockstun increased from 16 to 23. Pushback modifier increased from 100% to 125%.";
	} else if (stun == 121) {
		return "Base Stun Value increased from 110 to 121. Blockstun increased from 23 to 26. Pushback modifier increased from 125% to 150%.";
	} else if (stun == 132) {
		return "Base Stun Value increased from 121 to 132. Blockstun increased from 26 to 30. Pushback modifier increased from 150% to 175%.";
	} else if (stun == 143) {
		return "Base Stun Value increased from 132 to 143. Blockstun increased from 30 to 34. Pushback modifier increased from 175% to 200%."
			" Gives wallstick in the corner.";
	}
	return "";
}
bool powerup_may6H(PlayerInfo& player) {
	BYTE* func = player.pawn.bbscrCurrentFunc();
	fillMay6HOffsets(func);
	int offset = player.pawn.bbscrCurrentInstr() - func;
	if (offset > moves.may6H_6DHoldOffset && offset < moves.may6H_6DHoldAttackOffset) {
		return player.prevFrameMem45 == 1 && player.pawn.mem45() == 0;
	}
	return false;
}
const char* powerupExplanation_may6H(PlayerInfo& player) {
	return "Became an overhead.";
}
bool powerup_qv(PlayerInfo& player) {
	return player.prevFrameMem46 != player.pawn.mem46();
}
const char* powerupExplanation_qv(int level) {
	if (level == 1) return "Ball reached level 1.";
	if (level == 2) return "Ball reached level 2.";
	if (level == 3) return "Ball reached level 3.";
	if (level == 4) return "Ball reached level 4.";
	if (level == 5) return "Ball reached level 5.";
	return "Ball reached level ???.";
}
const char* powerupExplanation_qvA(PlayerInfo& player) {
	int level = 0;
	Entity p = player.pawn.stackEntity(0);
	if (p) {
		level = p.storage(1);
	}
	return powerupExplanation_qv(level);
}
const char* powerupExplanation_qvB(PlayerInfo& player) {
	int level = 0;
	Entity p = player.pawn.stackEntity(1);
	if (p) {
		level = p.storage(1);
	}
	return powerupExplanation_qv(level);
}
const char* powerupExplanation_qvC(PlayerInfo& player) {
	int level = 0;
	Entity p = player.pawn.stackEntity(2);
	if (p) {
		level = p.storage(1);
	}
	return powerupExplanation_qv(level);
}
const char* powerupExplanation_qvD(PlayerInfo& player) {
	int level = 0;
	Entity p = player.pawn.stackEntity(3);
	if (p) {
		level = p.storage(1);
	}
	return powerupExplanation_qv(level);
}
void Moves::fillInVenomStingerPowerup(BYTE* func, std::vector<int>& powerups) {
	if (!powerups.empty()) return;
	bool foundSendSignal = false;
	BYTE* instr;
	for (
			instr = skipInstruction(func);
			instructionType(instr) != instr_endState;
			instr = skipInstruction(instr)
	) {
		InstructionType type = instructionType(instr);
		if (type == instr_sendSignal) {
			foundSendSignal = true;
		} else if (foundSendSignal && type == instr_sprite) {
			powerups.push_back(instr - func);
			foundSendSignal = false;
		}
	}
}
const char* powerupExplanation_stinger(PlayerInfo& player) {
	int level = 0;
	Entity p = player.pawn.stackEntity(4);
	if (p) level = p.storage(1);
	if (level == 5) return "Ball reached level 5.";
	if (level == 4) return "Ball reached level 4.";
	if (level == 3) return "Ball reached level 3.";
	if (level == 2) return "Ball reached level 2.";
	if (level == 1) return "Ball reached level 1.";
	return "Ball reached level ???.";
}
bool powerup_stinger(PlayerInfo& player, std::vector<int>& powerups) {
	BYTE* func = player.pawn.bbscrCurrentFunc();
	moves.fillInVenomStingerPowerup(func, powerups);
	int offset = player.pawn.bbscrCurrentInstr() - func;
	for (int i = 0; i < (int)powerups.size(); ++i) {
		if (offset == powerups[i]) {
			return !player.pawn.isRCFrozen() && player.pawn.spriteFrameCounter() == 0;
		}
	}
	return false;
}
bool powerup_stingerS(PlayerInfo& player) {
	return powerup_stinger(player, moves.venomStingerSPowerups);
}
bool powerup_stingerH(PlayerInfo& player) {
	return powerup_stinger(player, moves.venomStingerHPowerups);
}
bool powerup_kyougenA(PlayerInfo& ent) {
	return ent.prevFrameGroundHitEffect != 8 && ent.pawn.groundHitEffect() == 8;
}
const char* powerupExplanation_kyougenA(PlayerInfo& ent) {
	return "Ground bounces on hit.";
}
bool powerup_kyougenB(PlayerInfo& ent) {
	return ent.prevFrameGroundBounceCount != 1 && ent.pawn.groundBounceCount() == 1;
}
const char* powerupExplanation_kyougenB(PlayerInfo& ent) {
	return "Without the powerup, this move doesn't ground bounce, but KDs on normal hit."
	" It ground bounces on CH with -350.00 starting speed Y and 227.50 starting speed X."
	" You can combo from this if it was an airhit, and if it was a ground hit, you might need RRC.\n"
	"With the powerup, it ground bounces even on normal hit,"
	" with -250.00 starting speed Y and 175.00 starting speed X, and is very easy to combo from"
	" on both air and ground hits.\n"
	"With the powerup and the counterhit, it launches with -300.00 starting speed Y"
	" and 50.00 starting speed X.";
}
bool powerup_kyougenC(PlayerInfo& ent) {
	return ent.prevFrameTumbleDuration == INT_MAX && ent.pawn.tumbleDuration() != INT_MAX;
}
const char* powerupExplanation_kyougenC(PlayerInfo& ent) {
	return "Without the powerup, this move doesn't ground bounce, but KDs on normal ground hit"
	" and wallbounces on normal air hit, with no KD, and you can't combo from that without RRC.\n"
	"On air CH, it ground bounces and then wall bounces, and can be easily combo'd from both midscreen and in the corner.\n"
	"On ground CH, it ground bounces, and can be combo'd from with a fast move both midscreen and in the corner.\n"
	"With the powerup, on normal air or ground hit, it tumbles at 300.00 starting speed X and gives 51-52 tumble frames.\n"
	"With the powerup, on CH air or ground hit, it tumbles at 245.00 starting speed X and gives 69-70 tumble frames.";
}
const char* powerupExplanation_kyougenD(PlayerInfo& ent) {
	BYTE* func = ent.pawn.bbscrCurrentFunc();
	BYTE* instr = moves.skipInstruction(func);
	instr = moves.skipInstruction(instr);
	instr = moves.skipInstruction(instr);
	instr = moves.skipInstruction(instr);
	instr = moves.skipInstruction(instr);
	bool isRev2 = moves.instructionType(instr) == instr_hitAirPushbackX;
	if (isRev2) {
		return "Increases maximum number of hits from 3 to 5 and removes landing recovery.";
	} else {
		return "Increases maximum number of hits from 3 to 5 and increases speed X and Y"
			" that is given to the opponent on hit from:\n"
			"70.00 speed X, 175.00 speed Y without the powerup to\n"
			"140.00 speed X, 180.00 speed Y with the powerup.";
	}
}
bool powerup_kyougenD(PlayerInfo& ent) {
	return ent.prevFrameMaxHit != 5 && ent.pawn.maxHit() == 5;
}
bool powerup_onpu(ProjectileInfo& projectile) {
	return !(projectile.ptr && projectile.ptr.isRCFrozen())
		&& (
			projectile.animFrame == 32
			|| projectile.animFrame == 44
			|| projectile.animFrame == 56
			|| projectile.animFrame == 68
		)
		&& !(
			projectile.ptr
			&& projectile.ptr.mem45()
			&& strcmp(projectile.ptr.gotoLabelRequest(), "hit") != 0
		);
}
bool powerup_djavu(PlayerInfo& ent) {
	return ent.animFrame == 6 && !ent.pawn.isRCFrozen();
}
const char* powerupExplanation_djavu(PlayerInfo& ent) {
	return "//Title override: \n"
		"On this frame \x44\xC3\xA9\x6A\xC3\xA0 Vu checks for the existence of the seal.";
}
bool powerup_closeShot(ProjectileInfo& projectile) {
	entityList.populate();
	if (!projectile.ptr) {
		if (projectile.landedHit) {
			int team = projectile.team;
			if (team == 0 || team == 1) {
				int opponentX = entityList.slots[1 - team].posX();
				int thisX = projectile.x;
				int dist = opponentX - thisX;
				if (dist < 0) dist = -dist;
				return dist < 300000;
			}
		}
		return false;
	}
	if (projectile.ptr.currentAnimDuration() == 1 && !projectile.ptr.isRCFrozen()) {
		int dist = projectile.ptr.enemyEntity().posX() - projectile.ptr.posX();
		if (dist < 0) dist = -dist;
		return dist < 300000;
	}
	return false;
}
bool powerup_rifle(PlayerInfo& ent) {
	return !ent.prevFrameElpheltRifle_AimMem46 && ent.elpheltRifle_AimMem46;
}
const char* powerupExplanation_rifle(PlayerInfo& ent) {
	return "Ms. Confille reached maximum charge.";
}
bool powerup_beakDriver(PlayerInfo& ent) {
	return !ent.prevFrameMem45 && ent.pawn.mem45()
		|| ent.pawn.romanCancelAvailability() == ROMAN_CANCEL_DISALLOWED_ON_WHIFF_WITH_X_MARK;
}
const char* powerupExplanation_beakDriver(PlayerInfo& ent) {
	if (ent.pawn.romanCancelAvailability() == ROMAN_CANCEL_DISALLOWED_ON_WHIFF_WITH_X_MARK) {
		if (ent.prevFrameRomanCancelAvailability != ROMAN_CANCEL_DISALLOWED_ON_WHIFF_WITH_X_MARK) {
			return "//Title override: \n"
				"Starting on this frame, can't RC, unless opponent is in hitstun or blockstun.";
		} else {
			return "//Title override: \n"
				"Can't RC, unless opponent is in hitstun or blockstun.";
		}
	} else {
		return "Will perform the maximum power attack upon release.";
	}
}
bool dontShowPowerupGraphic_beakDriver(PlayerInfo& ent) {
	return ent.pawn.romanCancelAvailability() == ROMAN_CANCEL_DISALLOWED_ON_WHIFF_WITH_X_MARK;
}
bool powerup_mistFiner(PlayerInfo& ent) {
	return ent.johnnyMistFinerBuffedOnThisFrame;
}
const char* powerupExplanation_mistFiner(PlayerInfo& ent) {
	if (ent.pawn.dealtAttack()->guardType == GUARD_TYPE_NONE) {
		return "Mist Finer became unblockable and may change to Guard Break instead, if the opponent lands.";
	} else {
		return "Mist Finer acquired Guard Break property and may change to an unblockable, if the opponent jumps.";
	}
}
bool powerup_eatMeat(PlayerInfo& ent) {
	return ent.pawn.exGaugeValue(0) > ent.prevFrameResource[0];
}
const char* powerupExplanation_eatMeat(PlayerInfo& ent) {
	return "Restored Calorie Gauge.";
}
bool powerup_cardK(PlayerInfo& ent) {
	return ent.pawn.exGaugeValue(0) > ent.prevFrameResource[0];
}
bool powerup_cardS(PlayerInfo& ent) {
	return ent.pawn.exGaugeValue(1) > ent.prevFrameResource[1];
}
bool powerup_cardH(PlayerInfo& ent) {
	return ent.pawn.exGaugeValue(2) > ent.prevFrameResource[2];
}
const char* powerupExplanation_card(PlayerInfo& ent) {
	return "Obtained Card.";
}
bool powerup_hayabusaRev(PlayerInfo& ent) {
	return ent.prevFrameMem45 == 0 && ent.pawn.mem45() != 0;
}
const char* powerupExplanation_hayabusaRev(PlayerInfo& ent) {
	return "Acquired Guard Break property.";
}
bool powerup_hayabusaHeld(PlayerInfo& ent) {
	return ent.prevFrameMem45 == 0 && ent.pawn.mem45() != 0;
}
const char* powerupExplanation_hayabusaHeld(PlayerInfo& ent) {
	return "Reached maximum charge.";
}
bool powerup_grampaMax(PlayerInfo& ent) {
	return ent.prevFrameMem45 == 0 && ent.pawn.mem45() != 0;
}
const char* powerupExplanation_grampaMax(PlayerInfo& ent) {
	return "Reached maximum charge.";
}
bool powerup_armorDance(PlayerInfo& ent) {
	return ent.pawn.exGaugeValue(0) > ent.prevFrameResource[0];
}
const char* powerupExplanation_armorDance(PlayerInfo& ent) {
	return "Gained Excitement.";
}
bool powerup_fireSpear(PlayerInfo& ent) {
	return ent.pawn.previousEntity()
		&& ent.pawn.previousEntity().lifeTimeCounter() == 0
		&& strcmp(ent.pawn.previousEntity().animationName(), "KinomiObjNecro") != 0;  // ignore the first spear
}
const char* powerupExplanation_fireSpear(PlayerInfo& ent) {
	return "Created the next fire spear.";
}
bool powerup_zweit(PlayerInfo& ent) {
	if (strcmp(ent.pawn.gotoLabelRequest(), "FrontEnd") == 0) return true;
	if (!ent.pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED)) return false;
	BYTE* instr = ent.pawn.uponStruct(BBSCREVENT_ANIMATION_FRAME_ADVANCED)->uponInstrPtr;
	instr = moves.skipInstruction(instr);
	if (moves.instructionType(instr) == instr_ifOperation
			&& asInstr(instr, ifOperation)->op == BBSCROP_IS_GREATER_OR_EQUAL) {
		return true;
	}
	return false;
}
const char* powerupExplanation_zweit(PlayerInfo& ent) {
	return "//Title override: \n"
		"On this frame checks if Leo's origin point is still not behind opponent's origin point."
		" If on any of these frames Leo failed to cross his opponent up, he will transition to non-backturn ender.";
}
bool powerup_kuuhuku(PlayerInfo& ent) {
	return ent.pawn.romanCancelAvailability() == ROMAN_CANCEL_DISALLOWED_ON_WHIFF_WITH_X_MARK;
}
const char* powerupExplanation_kuuhuku(PlayerInfo& ent) {
	if (ent.prevFrameRomanCancelAvailability != ROMAN_CANCEL_DISALLOWED_ON_WHIFF_WITH_X_MARK) {
		return "//Title override: \n"
			"Starting on this frame, can't RC, unless opponent is in hitstun or blockstun.";
	} else {
		return "//Title override: \n"
			"Can't RC, unless opponent is in hitstun or blockstun.";
	}
}
bool powerup_secretGarden(PlayerInfo& ent) {
	entityList.populate();
	for (int i = 2; i < entityList.count; ++i) {
		Entity proj = entityList.list[i];
		if (proj.team() == ent.index && !proj.isPawn() && strcmp(proj.animationName(), "SecretGardenBall") == 0) {
			BYTE* funcStart = proj.bbscrCurrentFunc();
			moves.fillMilliaSecretGardenUnlink(funcStart);
			if (moves.milliaSecretGardenUnlink && proj.bbscrCurrentInstr() - funcStart == moves.milliaSecretGardenUnlink
					&& proj.spriteFrameCounter() == 0 && !proj.isRCFrozen() && !proj.isSuperFrozen()) {
				return true;
			}
		}
	}
	return false;
}
const char* powerupExplanation_secretGarden(PlayerInfo& ent) {
	return "//Title override: \n"
		"On this frame Secret Garden detached from the player, and, starting on the next frame, can be RC'd, and the Secret Garden will stay.";
}

void fillMay6HOffsets(BYTE* func) {
	if (moves.may6H_6DHoldOffset == 0) {
		moves.may6H_6DHoldOffset = moves.findSetMarker(func, "6DHold") - func;
		moves.may6H_6DHoldAttackOffset = moves.findSetMarker(func, "6DHoldAttack") - func;
	}
}

int Moves::getBedmanSealRemainingFrames(ProjectileInfo& projectile, MayIrukasanRidingObjectInfo& info, BBScrEvent signal, bool* isFrameAfter) {
	BYTE* func = projectile.ptr.bbscrCurrentFunc();
	if (info.totalFrames == 0) {
		BYTE* instr;
		bool metSprite = false;
		bool metSpriteEnd = false;
		bool metSendSignal = false;
		int lastSpriteLength = 0;
		bool isInsideUpon = false;
		for (
				instr = moves.skipInstruction(func);
				moves.instructionType(instr) != instr_endState;
				instr = moves.skipInstruction(instr)
		) {
			InstructionType type = moves.instructionType(instr);
			if (metSpriteEnd) {
				if (metSprite) {
					info.frames.back().offset = instr - func;
				}
				metSprite = false;
				metSpriteEnd = false;
			}
			if (type == instr_sprite) {
				if (metSprite) {
					info.frames.back().offset = instr - func;
				}
				info.frames.emplace_back();
				MayIrukasanRidingObjectFrames& newThing = info.frames.back();
				newThing.frames = info.totalFrames;
				if (!metSendSignal) {
					lastSpriteLength = asInstr(instr, sprite)->duration;
					info.totalFrames += lastSpriteLength;
				}
				metSprite = true;
			} else if (type == instr_spriteEnd) {
				metSpriteEnd = true;
			} else if (type == instr_sendSignal
					&& asInstr(instr, sendSignal)->entity == ENT_PLAYER
					&& asInstr(instr, sendSignal)->event == signal
					&& !metSendSignal
					&& !isInsideUpon) {
				metSendSignal = true;
				info.totalFrames -= lastSpriteLength;
			} else if (type == instr_upon && asInstr(instr, upon)->event == BBSCREVENT_BEFORE_EXIT) {
				isInsideUpon = true;
			} else if (type == instr_endUpon) {
				isInsideUpon = false;
			}
		}
		if (metSprite) {
			info.frames.back().offset = instr - func;
		}
	}
	
	BYTE* currentInstr = projectile.ptr.bbscrCurrentInstr();
	int currentOffset = currentInstr - func;
	int timer = 0;
	for (int i = 0; i < (int)info.frames.size(); ++i) {
		const MayIrukasanRidingObjectFrames& elem = info.frames[i];
		if (currentOffset == elem.offset) {
			timer = elem.frames;
			*isFrameAfter = i == 3 && projectile.ptr.spriteFrameCounter() == 0 && !projectile.ptr.isRCFrozen();
			break;
		}
	}
	timer += projectile.ptr.spriteFrameCounter();
	if (timer >= info.totalFrames) return 0;
	return info.totalFrames - timer;
}

void Moves::fillInKyMahojin(BYTE* func) {
	if (kyMahojin.totalFrames != 0) return;
	bool metSprite = false;  //
	bool metSpriteEnd = false;
	int lastSpriteLength = 0;
	BYTE* instr;
	for (instr = skipInstruction(func);
			instructionType(instr) != instr_endState;
			instr = skipInstruction(instr)) {
		InstructionType type = instructionType(instr);
		if (metSpriteEnd) {
			kyMahojin.frames.emplace_back();
			MayIrukasanRidingObjectFrames& newFrames = kyMahojin.frames.back();
			newFrames.frames = kyMahojin.totalFrames;
			newFrames.offset = instr - func;
			kyMahojin.totalFrames += lastSpriteLength;
			metSpriteEnd = false;
			metSprite = false;
		}
		if (type == instr_sprite) {
			if (metSprite) {
				kyMahojin.frames.emplace_back();
				MayIrukasanRidingObjectFrames& newFrames = kyMahojin.frames.back();
				newFrames.frames = kyMahojin.totalFrames;
				newFrames.offset = instr - func;
				kyMahojin.totalFrames += lastSpriteLength;
				metSprite = false;
			}
			lastSpriteLength = asInstr(instr, sprite)->duration;
			metSprite = true;
		} else if (type == instr_spriteEnd) {
			metSpriteEnd = true;
		}
	}
}

int Moves::MayIrukasanRidingObjectInfo::remainingTime(int offset, int spriteFrame) const {
	for (int i = 0; i < (int)frames.size(); ++i) {
		if (offset == frames[i].offset) {
			int total = frames[i].frames + spriteFrame;
			if (total >= totalFrames) return 0;
			return totalFrames - total;
		}
	}
	return 0;
}

void Moves::fillInRamlethalBitN6C_F6D(BYTE* func, std::vector<RamlethalSwordInfo>& ramlethalBit) {
	if (!ramlethalBit.empty()) return;
	BYTE* instr;
	ramlethalBit.emplace_back();
	RamlethalSwordInfo* currentElem = &ramlethalBit.back();
	currentElem->state = ram_teleport;
	int lastSpriteLengthSoubi = 0;
	int lastSpriteLengthBunri = 0;
	bool metSprite = false;
	bool metSpriteEnd = false;
	bool metSetMarker = false;
	bool metJumpToState = false;
	for (
			instr = skipInstruction(func);
			instructionType(instr) != instr_endState;
			instr = skipInstruction(instr)
	) {
		InstructionType type = instructionType(instr);
		if (metSpriteEnd) {
			if (metSprite) {
				currentElem->addFrames(instr - func, lastSpriteLengthSoubi, lastSpriteLengthBunri);
				metSprite = false;
			}
			metSpriteEnd = false;
		}
		if (type == instr_sprite) {
			if (metSprite) {
				currentElem->addFrames(instr - func, lastSpriteLengthSoubi, lastSpriteLengthBunri);
			}
			if (metSetMarker) {
				RamlethalStateName prevState = currentElem->state;
				ramlethalBit.emplace_back();
				currentElem = &ramlethalBit.back();
				currentElem->state = (RamlethalStateName)((int)prevState + 1);
				metSetMarker = false;
			}
			lastSpriteLengthSoubi = asInstr(instr, sprite)->duration;
			lastSpriteLengthBunri = lastSpriteLengthSoubi;
			metSprite = true;
		} else if (type == instr_overrideSpriteLengthIf) {
			lastSpriteLengthBunri = asInstr(instr, overrideSpriteLengthIf)->duration;
		} else if (type == instr_setMarker) {
			if (metJumpToState) {
				metJumpToState = false;
				if (metSetMarker) {
					RamlethalStateName prevState = currentElem->state;
					ramlethalBit.emplace_back();
					currentElem = &ramlethalBit.back();
					currentElem->state = (RamlethalStateName)((int)prevState + 1);
				}
			}
			metSetMarker = true;
		} else if (type == instr_jumpToState
				|| type == instr_callSubroutine
				&& strcmp(asInstr(instr, jumpToState)->name, "BitActionNeutral") == 0) {
			metSprite = false;
			metJumpToState = true;
		} else if (type == instr_spriteEnd) {
			metSpriteEnd = true;
		}
	}
}

void Moves::RamlethalSwordInfo::addFrames(int offset, int lengthSoubi, int lengthBunri) {
	framesSoubi.frames.emplace_back();
	MayIrukasanRidingObjectFrames* newFrames = &framesSoubi.frames.back();
	newFrames->offset = offset;
	newFrames->frames = framesSoubi.totalFrames;
	framesSoubi.totalFrames += lengthSoubi;
	
	framesBunri.frames.emplace_back();
	newFrames = &framesBunri.frames.back();
	newFrames->offset = offset;
	newFrames->frames = framesBunri.totalFrames;
	framesBunri.totalFrames += lengthBunri;
}

void Moves::fillInFindMarker(BYTE* func, int* result, const char* markerName) {
	if (*result != 0) return;
	BYTE* pos = findSetMarker(func, markerName);
	if (pos) {
		*result = pos - func;
	}
}

const char* Moves::rifleAutoExit(PlayerInfo& player, int* offsetStorage, const char* moveName) {
	BYTE* func = player.pawn.bbscrCurrentFunc();
	moves.fillInFindMarker(func, offsetStorage, "End");
	if (*offsetStorage && player.pawn.bbscrCurrentInstr() - func > *offsetStorage) {
		return moveName;
	}
	return nullptr;
}

void Moves::fillGhostStateOffsets(BYTE* func, std::vector<int>& offsets) {
	if (!offsets.empty()) return;
	BYTE* instr;
	bool metSprite = false;
	bool metSpriteEnd = false;
	bool metSetMarker = false;
	int lastOffset = 0;
	for (
			instr = skipInstruction(func);
			instructionType(instr) != instr_endState;
			instr = skipInstruction(instr)
	) {
		InstructionType type = instructionType(instr);
		if (metSpriteEnd) {
			metSprite = false;
			metSpriteEnd = false;
			lastOffset = instr - func;
		}
		if (type == instr_sprite) {
			if (metSprite) {
				lastOffset = instr - func;
			}
			if (metSetMarker) {
				metSetMarker = false;
				offsets.push_back(lastOffset);
			}
			metSprite = true;
		} else if (type == instr_spriteEnd) {
			metSpriteEnd = true;
		} else if (type == instr_setMarker) {
			metSetMarker = true;
		}
	}
	if (metSetMarker) {
		offsets.push_back(instr - func);
	}
}

int Moves::findGhostState(int offset, const std::vector<int>& offsets) {
	if (offsets.empty()) return 0;
	if (offset <= offsets[0]) return 0;
	for (int i = (int)offsets.size() - 1; i >= 0; --i) {
		if (offset > offsets[i]) {
			return i + 1;
		}
	}
	return 0;
}

void Moves::fillJackoThrowGhostOffset(BYTE* func, int* offset) {
	if (*offset != 0) return;
	BYTE* instr;
	bool found = false;
	for (
			instr = skipInstruction(func);
			instructionType(instr) != instr_endState;
			instr = skipInstruction(instr)
	) {
		InstructionType type = instructionType(instr);
		if (type == instr_sendSignalToAction) {
			found = true;
		} else if (type == instr_sprite && found) {
			*offset = instr - func;
			return;
		}
	}
}

void Moves::fillJackoGhostExp(BYTE* func, int* jackoGhostExp) {
	if (jackoGhostExp[0] != 0) return;
	BYTE* instr;
	int counter = 2;
	for (
			instr = skipInstruction(func);
			instructionType(instr) != instr_endState;
			instr = skipInstruction(instr)
	) {
		InstructionType type = instructionType(instr);
		if (type == instr_ifOperation
				&& asInstr(instr, ifOperation)->op == BBSCROP_IS_EQUAL
				&& asInstr(instr, ifOperation)->left == MEM(46)
				&& asInstr(instr, ifOperation)->right == BBSCRTAG_VALUE) {
			*jackoGhostExp = asInstr(instr, ifOperation)->right.value;
			++jackoGhostExp;
			--counter;
			if (counter == 0) return;
		}
	}
}

void Moves::fillJackoGhostCreationTimer(BYTE* func, int* jackoGhostCreationTimer) {
	if (jackoGhostCreationTimer[0] != 0) return;
	BYTE* instr;
	int counter = 3;
	for (
			instr = skipInstruction(func);
			instructionType(instr) != instr_endState;
			instr = skipInstruction(instr)
	) {
		InstructionType type = instructionType(instr);
		if (type == instr_ifOperation
				&& asInstr(instr, ifOperation)->op == BBSCROP_IS_GREATER_OR_EQUAL
				&& asInstr(instr, ifOperation)->left == MEM(57)
				&& asInstr(instr, ifOperation)->right == BBSCRTAG_VALUE) {
			*jackoGhostCreationTimer = asInstr(instr, ifOperation)->right.value;
			++jackoGhostCreationTimer;
			--counter;
			if (counter == 0) return;
		}
	}
}

void Moves::fillJackoGhostHealingTimer(BYTE* func, int* jackoGhostHealingTimer) {
	if (jackoGhostHealingTimer[0] != 0) return;
	int* jackoGhostHealingTimerOrig = jackoGhostHealingTimer;
	BYTE* instr;
	int counter = 6;
	for (
			instr = skipInstruction(func);
			instructionType(instr) != instr_endState;
			instr = skipInstruction(instr)
	) {
		InstructionType type = instructionType(instr);
		if (type == instr_ifOperation
				&& asInstr(instr, ifOperation)->op == BBSCROP_IS_EQUAL
				&& asInstr(instr, ifOperation)->left == MEM(48)
				&& asInstr(instr, ifOperation)->right == BBSCRTAG_VALUE) {
			*jackoGhostHealingTimer = asInstr(instr, ifOperation)->right.value;
			++jackoGhostHealingTimer;
			--counter;
			if (counter == 0) return;
		}
	}
	jackoGhostHealingTimerOrig[0] = -1;  // Rev1
}

void Moves::fillJackoGhostBuffTimer(BYTE* func) {
	if (jackoGhostBuffTimer != 0) return;
	BYTE* instr;
	for (
			instr = skipInstruction(func);
			instructionType(instr) != instr_endState;
			instr = skipInstruction(instr)
	) {
		InstructionType type = instructionType(instr);
		if (type == instr_ifOperation
				&& asInstr(instr, ifOperation)->op == BBSCROP_IS_EQUAL
				&& asInstr(instr, ifOperation)->left == MEM(52)
				&& asInstr(instr, ifOperation)->right == BBSCRTAG_VALUE
				&& asInstr(instr, ifOperation)->right.value != 1) {
			jackoGhostBuffTimer = asInstr(instr, ifOperation)->right.value;
			return;
		}
	}
}

void Moves::fillJackoGhostExplodeTimer(BYTE* func) {
	if (jackoGhostExplodeTimer != 0) return;
	BYTE* instr;
	int counter = 4;
	for (
			instr = skipInstruction(func);
			instructionType(instr) != instr_endState;
			instr = skipInstruction(instr)
	) {
		InstructionType type = instructionType(instr);
		if (type == instr_ifOperation
				&& asInstr(instr, ifOperation)->op == BBSCROP_IS_EQUAL
				&& asInstr(instr, ifOperation)->left == MEM(49)
				&& asInstr(instr, ifOperation)->right == BBSCRTAG_VALUE) {
			--counter;
			if (counter == 0) {
				jackoGhostExplodeTimer = asInstr(instr, ifOperation)->right;
				return;
			}
		}
	}
}

void Moves::fillServantCooldown(BYTE* func, int* servantCooldown) {
	if (servantCooldown[0] != 0) return;
	BYTE* instr;
	int counter = 2;
	for (
			instr = skipInstruction(func);
			instructionType(instr) != instr_endState;
			instr = skipInstruction(instr)
	) {
		InstructionType type = instructionType(instr);
		if (type == instr_sprite) {
			*servantCooldown = asInstr(instr, sprite)->duration;
			++servantCooldown;
			--counter;
			if(counter == 0) return;
		}
	}
}

void Moves::fillServantExplosionTimer(BYTE* func) {
	if (servantExplosionTimer != 0) return;
	BYTE* instr;
	int counter = 4;
	for (
			instr = skipInstruction(func);
			instructionType(instr) != instr_endState;
			instr = skipInstruction(instr)
	) {
		InstructionType type = instructionType(instr);
		if (type == instr_ifOperation
				&& asInstr(instr, ifOperation)->op == BBSCROP_IS_EQUAL
				&& asInstr(instr, ifOperation)->left == MEM(54)
				&& asInstr(instr, ifOperation)->right == BBSCRTAG_VALUE) {
			--counter;
			if(counter == 0) {
				servantExplosionTimer = asInstr(instr, ifOperation)->right.value;
				return;
			}
		}
	}
}

void Moves::fillServantClockUpTimer(BYTE* func) {
	if (servantClockUpTimer != 0) return;
	BYTE* instr;
	int counter = 2;
	for (
			instr = skipInstruction(func);
			instructionType(instr) != instr_endState;
			instr = skipInstruction(instr)
	) {
		InstructionType type = instructionType(instr);
		if (type == instr_ifOperation
				&& asInstr(instr, ifOperation)->op == BBSCROP_IS_EQUAL
				&& asInstr(instr, ifOperation)->left == MEM(49)
				&& asInstr(instr, ifOperation)->right == BBSCRTAG_VALUE) {
			--counter;
			if(counter == 0) {
				servantClockUpTimer = asInstr(instr, ifOperation)->right.value;
				return;
			}
		}
	}
}

void Moves::fillServantTimeoutTimer(BYTE* func) {
	if (servantTimeoutTimer != 0) return;
	BYTE* instr;
	for (
			instr = skipInstruction(func);
			instructionType(instr) != instr_endState;
			instr = skipInstruction(instr)
	) {
		InstructionType type = instructionType(instr);
		if (type == instr_ifOperation
				&& asInstr(instr, ifOperation)->op == BBSCROP_IS_GREATER_OR_EQUAL
				&& asInstr(instr, ifOperation)->left == BBSCRVAR_FRAMES_SINCE_REGISTERING_FOR_THE_ANIMATION_FRAME_ADVANCED_SIGNAL
				&& asInstr(instr, ifOperation)->right == BBSCRTAG_VALUE) {
			servantTimeoutTimer = asInstr(instr, ifOperation)->right.value;
			return;
		}
	}
}

void Moves::fillServantAtk(BYTE* func, MayIrukasanRidingObjectInfo* servantAtk) {
	if (servantAtk[0].totalFrames != 0) return;
	BYTE* instr;
	bool start = false;
	bool metSprite = false;
	bool metSpriteEnd = false;
	bool metSetMarker = false;
	int lastSpriteLength = 0;
	int counter = 6;
	for (
			instr = skipInstruction(func);
			instructionType(instr) != instr_endState;
			instr = skipInstruction(instr)
	) {
		InstructionType type = instructionType(instr);
		if (type == instr_setMarker && strcmp(asInstr(instr, setMarker)->name, "AtkLv1") == 0) {
			start = true;
			instr = skipInstruction(instr);
			break;
		}
	}
	if (!start) return;
	for (
			;
			instructionType(instr) != instr_endState;
			instr = skipInstruction(instr)
	) {
		InstructionType type = instructionType(instr);
		if (metSpriteEnd) {
			servantAtk->frames.emplace_back();
			MayIrukasanRidingObjectFrames& newFrames = servantAtk->frames.back();
			newFrames.offset = instr - func;
			newFrames.frames = servantAtk->totalFrames;
			servantAtk->totalFrames += lastSpriteLength;
			
			metSpriteEnd = false;
			metSprite = false;
		}
		if (type == instr_sprite) {
			if (metSprite) {
				servantAtk->frames.emplace_back();
				MayIrukasanRidingObjectFrames& newFrames = servantAtk->frames.back();
				newFrames.offset = instr - func;
				newFrames.frames = servantAtk->totalFrames;
				servantAtk->totalFrames += lastSpriteLength;
			}
			lastSpriteLength = asInstr(instr, sprite)->duration;
			metSprite = true;
		} else if (type == instr_spriteEnd) {
			metSpriteEnd = true;
		} else if (type == instr_goToMarker) {
			++servantAtk;
			--counter;
			if (counter == 0) return;
		}
	}
}

void Moves::fillInJamSaishingekiY(BYTE* func) {
	if (jamSaishingekiY != 0) return;
	int counter = 2;
	bool inUponHitTheEnemyPlayer = false;
	BYTE* instr;
	for (
			instr = skipInstruction(func);
			instructionType(instr) != instr_endState;
			instr = skipInstruction(instr)
	) {
		InstructionType type = instructionType(instr);
		if (type == instr_upon && asInstr(instr, upon)->event == BBSCREVENT_HIT_THE_ENEMY_PLAYER) {
			--counter;
			if (counter == 0) {
				inUponHitTheEnemyPlayer = true;
				break;
			}
		}
	}
	if (!inUponHitTheEnemyPlayer) return;
	for (
			;
			instructionType(instr) != instr_endState;
			instr = skipInstruction(instr)
	) {
		InstructionType type = instructionType(instr);
		if (type == instr_ifOperation
				&& asInstr(instr, ifOperation)->op == BBSCROP_IS_LESSER
				&& asInstr(instr, ifOperation)->left == BBSCRVAR_OPPONENT_Y_OFFSET
				&& asInstr(instr, ifOperation)->right == BBSCRTAG_VALUE) {
			jamSaishingekiY = asInstr(instr, ifOperation)->right.value;
			return;
		}
	}
}

void Moves::fillDizzyKinomiNecro(BYTE* func, int* bombMarker, int* createBomb) {
	if (*bombMarker != 0) return;
	BYTE* pos = findSetMarker(func, "bomb");
	if (!pos) return;
	pos = findSpriteNonNull(pos);
	if (!pos) return;
	*bombMarker = pos - func;
	pos = findCreateObj(pos, "KinomiObjNecrobomb");
	if (!pos) return;
	*createBomb = pos - func;
}

void Moves::fillDizzyKinomiNecrobomb(BYTE* func) {
	if (dizzyKinomiNecrobomb.totalFrames != 0) return;
	BYTE* instr;
	bool metSprite = false;
	int lastSpriteLength = 0;
	for (
			instr = skipInstruction(func);
			instructionType(instr) != instr_endState;
			instr = skipInstruction(instr)
	) {
		if (instructionType(instr) == instr_sprite) {
			if (metSprite) {
				dizzyKinomiNecrobomb.frames.emplace_back();
				MayIrukasanRidingObjectFrames& newFrames = dizzyKinomiNecrobomb.frames.back();
				newFrames.frames = dizzyKinomiNecrobomb.totalFrames;
				dizzyKinomiNecrobomb.totalFrames += lastSpriteLength;
				newFrames.offset = instr - func;
			}
			metSprite = true;
			lastSpriteLength = asInstr(instr, sprite)->duration;
		}
	}
	if (metSprite) {
		dizzyKinomiNecrobomb.frames.emplace_back();
		MayIrukasanRidingObjectFrames& newFrames = dizzyKinomiNecrobomb.frames.back();
		newFrames.frames = dizzyKinomiNecrobomb.totalFrames;
		dizzyKinomiNecrobomb.totalFrames += lastSpriteLength;
		newFrames.offset = instr - func;
	}
}

void Moves::fillDizzyAkari(BYTE* func) {
	if (!dizzyAkari.empty()) return;
	BYTE* instr;
	int lastSpriteLength = 0;
	bool metSprite = false;
	bool metSpriteEnd = false;
	bool metSetMarker = false;
	dizzyAkari.emplace_back();
	MayIrukasanRidingObjectInfo* elem = &dizzyAkari.back();
	for (
			instr = skipInstruction(func);
			instructionType(instr) != instr_endState;
			instr = skipInstruction(instr)
	) {
		InstructionType type = instructionType(instr);
		if (metSpriteEnd) {
			elem->frames.emplace_back();
			MayIrukasanRidingObjectFrames& newFrames = elem->frames.back();
			newFrames.frames = elem->totalFrames;
			newFrames.offset = instr - func;
			elem->totalFrames += lastSpriteLength;
			metSprite = false;
			metSpriteEnd = false;
		}
		if (type == instr_sprite) {
			if (metSprite) {
				elem->frames.emplace_back();
				MayIrukasanRidingObjectFrames& newFrames = elem->frames.back();
				newFrames.frames = elem->totalFrames;
				newFrames.offset = instr - func;
				elem->totalFrames += lastSpriteLength;
			}
			if (metSetMarker) {
				dizzyAkari.emplace_back();
				elem = &dizzyAkari.back();
				metSetMarker = false;
			}
			metSprite = true;
			lastSpriteLength = asInstr(instr, sprite)->duration;
		} else if (type == instr_spriteEnd) {
			metSpriteEnd = true;
		} else if (type == instr_setMarker) {
			metSetMarker = true;
		}
	}
	if (metSprite) {
		elem->frames.emplace_back();
		MayIrukasanRidingObjectFrames& newFrames = elem->frames.back();
		newFrames.frames = elem->totalFrames;
		newFrames.offset = instr - func;
		elem->totalFrames += lastSpriteLength;
	}
}

void Moves::fillDizzyFish(BYTE* func, MayIrukasanRidingObjectInfo& fish) {
	if (fish.totalFrames != 0) return;
	BYTE* instr = findSetMarker(func, "end");
	if (!instr) return;
	bool metSprite = false;
	int lastSpriteLength = 0;
	for (
			instr = skipInstruction(instr);
			instructionType(instr) != instr_endState;
			instr = skipInstruction(instr)
	) {
		InstructionType type = instructionType(instr);
		if (type == instr_sprite) {
			if (metSprite) {
				fish.frames.emplace_back();
				MayIrukasanRidingObjectFrames& newFrames = fish.frames.back();
				newFrames.frames = fish.totalFrames;
				newFrames.offset = instr - func;
				fish.totalFrames += lastSpriteLength;
			}
			lastSpriteLength = asInstr(instr, sprite)->duration;
			metSprite = true;
		}
	}
	if (metSprite) {
		fish.frames.emplace_back();
		MayIrukasanRidingObjectFrames& newFrames = fish.frames.back();
		newFrames.frames = fish.totalFrames;
		newFrames.offset = instr - func;
		fish.totalFrames += lastSpriteLength;
	}
}

void Moves::fillDizzyLaserFish(BYTE* func, int* normal, int* alt) {
	if (*normal != 0) return;
	BYTE* instr;
	int lastSpriteLength = 0;
	for (
			instr = skipInstruction(func);
			instructionType(instr) != instr_endState;
			instr = skipInstruction(instr)
	) {
		InstructionType type = instructionType(instr);
		if (type == instr_sprite) {
			lastSpriteLength = asInstr(instr, sprite)->duration;
			*normal += lastSpriteLength;
			*alt += lastSpriteLength;
		} else if (type == instr_overrideSpriteLengthIf) {
			*alt = *alt - lastSpriteLength + asInstr(instr, overrideSpriteLengthIf)->duration;
		}
	}
}

void Moves::fillDizzyAwaKoware(BYTE* func, int* koware) {
	if (*koware != 0) return;
	BYTE* instr = findSetMarker(func, "koware");
	if (!instr) return;
	for (
			instr = skipInstruction(instr);
			true;
			instr = skipInstruction(instr)
	) {
		InstructionType type = instructionType(instr);
		if (type == instr_exitState || type == instr_endState) return;
		if (type == instr_sprite) {
			*koware += asInstr(instr, sprite)->duration;
		}
	}
}

void Moves::fillDizzyAwaBomb(BYTE* func, MayIrukasanRidingObjectInfo& info) {
	if (info.totalFrames != 0) return;
	BYTE* instr = findSetMarker(func, "bomb");
	if (!instr) return;
	bool metSprite = false;
	int lastSpriteLength = 0;
	for (
			instr = skipInstruction(instr);
			instructionType(instr) != instr_endState;
			instr = skipInstruction(instr)
	) {
		InstructionType type = instructionType(instr);
		if (type == instr_sprite) {
			if (metSprite) {
				info.frames.emplace_back();
				MayIrukasanRidingObjectFrames& newFrames = info.frames.back();
				newFrames.frames = info.totalFrames;
				newFrames.offset = instr - func;
				info.totalFrames += lastSpriteLength;
			}
			lastSpriteLength = asInstr(instr, sprite)->duration;
			metSprite = true;
		}
	}
	if (metSprite) {
		info.frames.emplace_back();
		MayIrukasanRidingObjectFrames& newFrames = info.frames.back();
		newFrames.frames = info.totalFrames;
		newFrames.offset = instr - func;
		info.totalFrames += lastSpriteLength;
	}
}

void Moves::fillMilliaSecretGardenUnlink(BYTE* funcStart) {
	if (milliaSecretGardenUnlink || milliaSecretGardenUnlinkFailedToFind) return;
	bool metUnlink = false;
	for (
		BYTE* instr = skipInstruction(funcStart);
		instructionType(instr) != instr_endState;
		instr = skipInstruction(instr)
	) {
		InstructionType type = instructionType(instr);
		if (type == instr_sprite) {
			if (metUnlink) {
				milliaSecretGardenUnlink = instr - funcStart;
				return;
			}
		} else if (type == instr_setLinkObjectDestroyOnStateChange && asInstr(instr, setLinkObjectDestroyOnStateChange)->entity == 0) {
			metUnlink = true;
		}
	}
	milliaSecretGardenUnlinkFailedToFind = true;
}

void Moves::fillElpheltRifleFireStartup(Entity ent) {
	if (elpheltRifleFireStartup) return;
	BYTE* func = ent.findStateStart("Rifle_Fire");
	if (!func) return;
	int prevDuration = 0;
	int startup = 0;
	for (BYTE* instr = skipInstruction(func);
			instructionType(instr) != instr_endState;
			instr = skipInstruction(instr)) {
		InstructionType type = instructionType(instr);
		if (type == instr_sprite) {
			startup += prevDuration;
			prevDuration = asInstr(instr, sprite)->duration;
		} else if (type == instr_sendSignalToAction
				&& strcmp(asInstr(instr, sendSignalToAction)->name, "Rifle_Aim") == 0
				&& asInstr(instr, sendSignalToAction)->signal == BBSCREVENT_CUSTOM_SIGNAL_0) {
			break;
		}
	}
	
	++startup;
	elpheltRifleFireStartup = startup;
}

void Moves::fillElpheltRifleFirePowerupStartup(BYTE* funcStart) {
	if (elpheltRifleFirePowerupStartup) return;
	for (BYTE* instr = skipInstruction(funcStart);
			instructionType(instr) != instr_endState;
			instr = skipInstruction(instr)) {
		InstructionType type = instructionType(instr);
		if (type == instr_ifOperation
				&& asInstr(instr, ifOperation)->op == BBSCROP_IS_GREATER_OR_EQUAL
				&& asInstr(instr, ifOperation)->left == MEM(45)
				&& asInstr(instr, ifOperation)->right == BBSCRTAG_VALUE) {
			elpheltRifleFirePowerupStartup = asInstr(instr, ifOperation)->right.value;
			return;
		}
	}
}

GroundBlitzType Moves::getBlitzType(PlayerInfo& ent) {
	if (ent.inNewMoveSection && ent.prevStartups.count == 1 && ent.prevStartups[0].startup == 50) {
		return BLITZTYPE_MAXCHARGE;
	}
	if (ent.pawn.mem48()) {
		if (ent.pawn.hitboxCount(HITBOXTYPE_HITBOX) > 0 && ent.pawn.currentAnimDuration() > 62) {
			return BLITZTYPE_MAXCHARGE;
		}
		return BLITZTYPE_CHARGE;
	}
	if (ent.pawn.mem51() <= 12) {
		return BLITZTYPE_TAP;
	} else {
		return BLITZTYPE_CHARGE;
	}
}
