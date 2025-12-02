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

unsigned short* Moves::bbscrInstructionSizes;

const NamePair emptyNamePair { "", nullptr };
static const CharacterType GENERAL = (CharacterType)-1;
static std::vector<MoveInfoProperty> allProperties;
bool charDoesNotCareAboutSuperJumpInstalls[25] { false };
static char strbuf[1024];
#define advanceBuf if (result != -1) { buf += result; bufSize -= result; }
static int blockstuns[] { 9, 11, 13, 16, 18, 20 };

const GhostState ghostStateNames[ghostStateNamesCount] {
	"Appear",
	"Land",
	"Reappear",
	"Idle",
	"Create",
	"Create",
	{ "Pick Up", true },
	"Hold",
	"Put",
	"Throw",
	"Throw",
	"Drop",
	"Damage"
};
static int findGhostStateNamePickUp() {
	for (int i = 0; i < _countof(ghostStateNames); ++i) {
		if (ghostStateNames[i].isPickUp) {
			return i;
		}
	}
	return -1;
}
int ghostStateName_PickUp = findGhostStateNamePickUp();

const ServantState servantStateNames[] {
	"Spawn",
	"Move",
	"Move",
	"Turn",
	"Attack",
	"Attack",
	"Attack",
	"Damage",
	{ "Death", true },
	{ "Death", true },
	"Wave Goodbye",
	"Waiting",
	"Lose",
	"Win"
};
const ServantState servantStateNamesSpearman[] {
	"Spawn",
	"Move",
	"Move",
	"Turn",
	"Attack",
	"Attack",
	"Attack",
	"Damage",
	"Damage",
	"Damage",
	{ "Death", true },
	{ "Death", true },
	"Wave Goodbye",
	"Waiting",
	"Lose",
	"Win"
};

static void findSpriteAfterIf(BYTE* func, int* result);

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
static bool isDangerous_servant(Entity ent);

static const NamePair* nameSelector_iceSpike(Entity ent);
static const NamePair* nameSelectorUncombined_iceSpike(Entity ent);
static const NamePair* nameSelector_iceScythe(Entity ent);
static const NamePair* nameSelectorUncombined_iceScythe(Entity ent);
static const NamePair* framebarNameSelector_djvuD(Entity ent);
static const NamePair* framebarNameSelector_closeShot(Entity ent);
static const NamePair* framebarNameSelector_gunflameProjectile(Entity ent);
static const NamePair* framebarNameSelector_venomBall(Entity ent);
static const NamePair* framebarNameUncombinedSelector_venomBall(Entity ent);
static const NamePair* framebarNameSelector_grenadeBomb(Entity ent);
static const NamePair* framebarNameSelector_grenadeBombReady(Entity ent);
static const NamePair* framebarNameSelector_landSettingTypeNeedleObj(Entity ent);

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
static bool conditionForAddingWhiffCancels_airStop(PlayerInfo& ent);

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

static const NamePair* displayNameSelector_pogoEntry(PlayerInfo& ent);
static const NamePair* displayNameSelector_pogoA(PlayerInfo& ent);
static const NamePair* displayNameSelector_pogo9(PlayerInfo& ent);
static const NamePair* displayNameSelector_pogo44(PlayerInfo& ent);
static const NamePair* displayNameSelector_pogo66(PlayerInfo& ent);
static const NamePair* displayNameSelector_pogoB(PlayerInfo& ent);
static const NamePair* displayNameSelector_pogoC(PlayerInfo& ent);
static const NamePair* displayNameSelector_pogoD(PlayerInfo& ent);
static const NamePair* displayNameSelector_pogoE(PlayerInfo& ent);
static const NamePair* displayNameSelector_pogo8(PlayerInfo& ent);
static const NamePair* displayNameSelector_itemToss(PlayerInfo& ent);
static const NamePair* displayNameSelector_RC(PlayerInfo& ent);
static const NamePair* displayNameSelector_may6P(PlayerInfo& ent);
static const NamePair* displayNameSelector_may6H(PlayerInfo& ent);
static const NamePair* displayNameSelector_badMoon(PlayerInfo& ent);
static const NamePair* displayNameSelector_carcassRaidS(PlayerInfo& ent);
static const NamePair* displayNameSelector_carcassRaidH(PlayerInfo& ent);
static const NamePair* displayNameSelector_stingerS(PlayerInfo& ent);
static const NamePair* displayNameSelector_stingerH(PlayerInfo& ent);
static const NamePair* displayNameSelector_taskCAir(PlayerInfo& ent);
static const NamePair* framebarNameSelector_blueBurst(Entity ent);
static const NamePair* displayNameSelector_blueBurst(PlayerInfo& ent);
static const NamePair* displayNameSelector_rifleStart(PlayerInfo& ent);
static const NamePair* displayNameSelector_rifleReload(PlayerInfo& ent);
static const NamePair* displayNameSelector_riflePerfectReload(PlayerInfo& ent);
static const NamePair* displayNameSelector_rifleRC(PlayerInfo& ent);
static const NamePair* displayNameSelector_mistEntry(PlayerInfo& ent);
static const NamePair* displayNameSelector_mistLoop(PlayerInfo& ent);
static const NamePair* displayNameSelector_mistWalkForward(PlayerInfo& ent);
static const NamePair* displayNameSelector_mistWalkBackward(PlayerInfo& ent);
static const NamePair* displayNameSelector_mistBDash(PlayerInfo& ent);
static const NamePair* displayNameSelector_mistDash(PlayerInfo& ent);
static const NamePair* displayNameSelector_mistBackdash(PlayerInfo& ent);
static const NamePair* displayNameSelector_airMistEntry(PlayerInfo& ent);
static const NamePair* displayNameSelector_airMistLoop(PlayerInfo& ent);
static const NamePair* displayNameSelector_airMistDash(PlayerInfo& ent);
static const NamePair* displayNameSelector_airMistBackdash(PlayerInfo& ent);
static const NamePair* displayNameSelector_gekirinLv2or3(PlayerInfo& ent);
static const NamePair* displayNameSelector_airGekirinLv2or3(PlayerInfo& ent);
static const NamePair* displayNameSelector_ryujinLv2or3(PlayerInfo& ent);
static const NamePair* displayNameSelector_airRyujinLv2or3(PlayerInfo& ent);
static const NamePair* displayNameSelector_kenroukakuLv2or3(PlayerInfo& ent);
static const NamePair* displayNameSelector_airKenroukakuLv2or3(PlayerInfo& ent);
static const NamePair* displayNameSelector_standingAzami(PlayerInfo& ent);
static const NamePair* displayNameSelector_crouchingAzami(PlayerInfo& ent);
static const NamePair* displayNameSelector_airAzami(PlayerInfo& ent);
static const NamePair* displayNameSelector_gunflame(PlayerInfo& ent);
static const NamePair* displayNameSelector_gunflameDI(PlayerInfo& ent);
static const NamePair* displayNameSelector_standingBlitzShield(PlayerInfo& ent);
static const NamePair* displayNameSelector_crouchingBlitzShield(PlayerInfo& ent);
static const NamePair* displayNameSelector_pilebunker(PlayerInfo& ent);
static const NamePair* displayNameSelector_crosswise(PlayerInfo& ent);
static const NamePair* displayNameSelector_underPressure(PlayerInfo& ent);
static const NamePair* displayNameSelector_jacko4D(PlayerInfo& ent);
static const NamePair* displayNameSelector_jackoj4D(PlayerInfo& ent);
static const NamePair* displayNameSelector_beakDriver(PlayerInfo& ent);
static const NamePair* displayNameSelector_elkHunt(PlayerInfo& ent);
static const NamePair* displayNameSelector_hawkBaker(PlayerInfo& ent);
static const NamePair* displayNameSelector_vultureSeize(PlayerInfo& ent);
static const NamePair* displayNameSelector_beakDriverAir(PlayerInfo& ent);
static const NamePair* displayNameSelector_bullBash(PlayerInfo& ent);
static const NamePair* displayNameSelector_beakDriverMash(PlayerInfo& ent);
static const NamePair* displayNameSelector_answer6K(PlayerInfo& ent);
static const NamePair* displayNameSelector_backturn(PlayerInfo& ent);
static const NamePair* displayNameSelector_tossin2(PlayerInfo& ent);
static const NamePair* displayNameSelector_tossin2Hasei(PlayerInfo& ent);
static const NamePair* displayNameSelector_leo5H(PlayerInfo& ent);
static const NamePair* displayNameSelector_leo6H(PlayerInfo& ent);
static const NamePair* displayNameSelector_gorengeki(PlayerInfo& ent);
static const NamePair* displayNameSelector_treasureHunt(PlayerInfo& ent);
static const NamePair* displayNameSelector_stepTreasureHunt(PlayerInfo& ent);
static const NamePair* displayNameSelector_asanagiB(PlayerInfo& ent);
static const NamePair* displayNameSelector_asanagiC(PlayerInfo& ent);
static const NamePair* displayNameSelector_asanagiD(PlayerInfo& ent);
static const NamePair* displayNameSelector_antiAir4Hasei(PlayerInfo& ent);
static const NamePair* displayNameSelector_antiAir6Hasei(PlayerInfo& ent);
static const NamePair* displayNameSelector_landBlow4Hasei(PlayerInfo& ent);
static const NamePair* displayNameSelector_landBlow6Hasei(PlayerInfo& ent);
static const NamePair* displayNameSelector_blackHoleAttack(PlayerInfo& ent);
static const NamePair* displayNameSelector_blackHoleAttackBurst(PlayerInfo& ent);
static const NamePair* displayNameSelector_landBlowAttack(PlayerInfo& ent);
static const NamePair* displayNameSelector_airBlowAttack(PlayerInfo& ent);
static const NamePair* displayNameSelector_commandThrow(PlayerInfo& ent);
static const NamePair* displayNameSelector_commandThrowEx(PlayerInfo& ent);
static const NamePair* displayNameSelector_antiAirCommandThrow(PlayerInfo& ent);
static const NamePair* displayNameSelector_antiAirCommandThrowEx(PlayerInfo& ent);
static const NamePair* displayNameSelector_dizzy6H(PlayerInfo& ent);
static const NamePair* displayNameSelector_kinomiNecro(PlayerInfo& ent);

static const char* canYrcProjectile_default(PlayerInfo& ent);
static const char* canYrcProjectile_gunflame(PlayerInfo& ent);
static const char* canYrcProjectile_tyrantRave(PlayerInfo& ent);
static const char* canYrcProjectile_cse(PlayerInfo& ent);
static const char* canYrcProjectile_se(PlayerInfo& ent);
static const char* canYrcProjectile_sacredEdge(PlayerInfo& ent);
static const char* canYrcProjectile_prevNoLinkDestroyOnStateChange(PlayerInfo& ent);
static const CreatedProjectileStruct* createdProjectile_ky5D(PlayerInfo& ent);
static const char* canYrcProjectile_ky5D(PlayerInfo& ent);
static const char* canYrcProjectile_kyJD(PlayerInfo& ent);
static const CreatedProjectileStruct* createdProjectile_splitCiel(PlayerInfo& ent);
static const char* canYrcProjectile_splitCiel(PlayerInfo& ent);
static const char* canYrcProjectile_coin(PlayerInfo& ent);
static const char* canYrcProjectile_bacchusSigh(PlayerInfo& ent);
static const char* canYrcProjectile_sinwazaShot(PlayerInfo& ent);
static const char* canYrcProjectile_beachBall(PlayerInfo& ent);
static const char* canYrcProjectile_dolphin(PlayerInfo& ent);
static const char* canYrcProjectile_yamada(PlayerInfo& ent);
static const char* canYrcProjectile_wallclingKunai(PlayerInfo& ent);
static const char* canYrcProjectile_shuriken(PlayerInfo& ent);
static const char* canYrcProjectile_gammaBlade(PlayerInfo& ent);
static const char* canYrcProjectile_ryuuYanagi(PlayerInfo& ent);
static const char* canYrcProjectile_faust5D(PlayerInfo& ent);
static const char* canYrcProjectile_itemToss(PlayerInfo& ent);
static const char* canYrcProjectile_pogoItemToss(PlayerInfo& ent);
static const char* canYrcProjectile_love(PlayerInfo& ent);
static const char* canYrcProjectile_superToss(PlayerInfo& ent);
static const char* canYrcProjectile_flower(PlayerInfo& ent);
static const char* canYrcProjectile_sickleFlash(PlayerInfo& ent);
static const char* canYrcProjectile_qvA(PlayerInfo& ent);
static const char* canYrcProjectile_qvB(PlayerInfo& ent);
static const char* canYrcProjectile_qvC(PlayerInfo& ent);
static const char* canYrcProjectile_qvD(PlayerInfo& ent);
static const char* canYrcProjectile_redHail(PlayerInfo& ent);
static const char* canYrcProjectile_darkAngel(PlayerInfo& ent);
static const CreatedProjectileStruct* createdProjectile_bishop(PlayerInfo& ent);
static const char* canYrcProjectile_bishop(PlayerInfo& ent);
static const char* canYrcProjectile_helterSkelter(PlayerInfo& ent);
static const char* canYrcProjectile_sdd(PlayerInfo& ent);
static const CreatedProjectileStruct* createdProjectile_ino5D(PlayerInfo& ent);
static const char* canYrcProjectile_ino5D(PlayerInfo& ent);
static const char* canYrcProjectile_kouutsuOnkai(PlayerInfo& ent);
static const char* canYrcProjectile_chemicalLove(PlayerInfo& ent);
static const char* canYrcProjectile_madogiwa(PlayerInfo& ent);
static const char* canYrcProjectile_genkai(PlayerInfo& ent);
static const char* canYrcProjectile_boomerang(PlayerInfo& ent);
static const char* canYrcProjectile_dejavuAB(PlayerInfo& ent);
static const char* canYrcProjectile_dejavuC(PlayerInfo& ent);
static const char* canYrcProjectile_dejavuD(PlayerInfo& ent);
static const char* canYrcProjectile_alarm(PlayerInfo& ent);
static const char* canYrcProjectile_merry(PlayerInfo& ent);
static const CreatedProjectileStruct* createdProjectile_onf5_s(PlayerInfo& ent);
static const CreatedProjectileStruct* createdProjectile_onf5_s_recall(PlayerInfo& ent);
static const CreatedProjectileStruct* createdProjectile_onf5_h(PlayerInfo& ent);
static const CreatedProjectileStruct* createdProjectile_onf5_h_recall(PlayerInfo& ent);
static const char* canYrcProjectile_onf5_sLaunch(PlayerInfo& ent);
static const char* canYrcProjectile_onf5_sRelaunch(PlayerInfo& ent);
static const char* canYrcProjectile_onf5_sRecover(PlayerInfo& ent);
static const char* canYrcProjectile_onf5_hLaunch(PlayerInfo& ent);
static const char* canYrcProjectile_onf5_hRelaunch(PlayerInfo& ent);
static const char* canYrcProjectile_onf5_hRecover(PlayerInfo& ent);
static const CreatedProjectileStruct* createdProjectile_onf7_s(PlayerInfo& ent);
static const CreatedProjectileStruct* createdProjectile_onf7_h(PlayerInfo& ent);
static const char* canYrcProjectile_onf7_sLaunch(PlayerInfo& ent);
static const char* canYrcProjectile_onf7_sRelaunch(PlayerInfo& ent);
static const char* canYrcProjectile_onf7_sRecover(PlayerInfo& ent);
static const char* canYrcProjectile_onf7_hLaunch(PlayerInfo& ent);
static const char* canYrcProjectile_onf7_hRelaunch(PlayerInfo& ent);
static const char* canYrcProjectile_onf7_hRecover(PlayerInfo& ent);
static const char* canYrcProjectile_bitSpiral(PlayerInfo& ent);
static const CreatedProjectileStruct* createdProjectile_elpheltjD(PlayerInfo& ent);
static const char* canYrcProjectile_elpheltjD(PlayerInfo& ent);
static const char* canYrcProjectile_rifleFire(PlayerInfo& ent);
static const char* canYrcProjectile_grenadeToss(PlayerInfo& ent);
static const CreatedProjectileStruct* createdProjectile_PGhost(PlayerInfo& ent);
static const char* canYrcProjectile_PGhost(PlayerInfo& ent);
static const CreatedProjectileStruct* createdProjectile_KGhost(PlayerInfo& ent);
static const char* canYrcProjectile_KGhost(PlayerInfo& ent);
static const CreatedProjectileStruct* createdProjectile_SGhost(PlayerInfo& ent);
static const char* canYrcProjectile_SGhost(PlayerInfo& ent);
static const CreatedProjectileStruct* createdProjectile_ThrowGhost(PlayerInfo& ent);
static const char* canYrcProjectile_ThrowGhost(PlayerInfo& ent);
static const CreatedProjectileStruct* createdProjectile_AirThrowGhost(PlayerInfo& ent);
static const char* canYrcProjectile_AirThrowGhost(PlayerInfo& ent);
static const char* canYrcProjectile_pickUpGhost(PlayerInfo& ent);
static const char* canYrcProjectile_putGhost(PlayerInfo& ent);
static const char* canYrcProjectile_returnGhost(PlayerInfo& ent);
static const char* canYrcProjectile_organ(PlayerInfo& ent);
static const char* canYrcProjectile_jackoCalvados(PlayerInfo& ent);
static const char* canYrcProjectile_iceSpike(PlayerInfo& ent);
static const char* canYrcProjectile_firePillar(PlayerInfo& ent);
static const char* canYrcProjectile_iceScythe(PlayerInfo& ent);
static const char* canYrcProjectile_fireScythe(PlayerInfo& ent);
static const CreatedProjectileStruct* createdProjectile_kum5D(PlayerInfo& ent);
static const char* canYrcProjectile_kum5D(PlayerInfo& ent);
static const char* canYrcProjectile_tuningBall(PlayerInfo& ent);
static const char* canYrcProjectile_ravenOrb(PlayerInfo& ent);
static const char* canYrcProjectile_ravenNeedle(PlayerInfo& ent);
static const char* canYrcProjectile_fish(PlayerInfo& ent);
static const char* canYrcProjectile_bubble(PlayerInfo& ent);
static const char* canYrcProjectile_fireBubble(PlayerInfo& ent);
static const char* canYrcProjectile_fireSpears(PlayerInfo& ent);
static const char* canYrcProjectile_iceSpear(PlayerInfo& ent);
static const char* canYrcProjectile_imperialRay(PlayerInfo& ent);
static const char* canYrcProjectile_tatami(PlayerInfo& ent);
static const char* canYrcProjectile_teppou(PlayerInfo& ent);
static const CreatedProjectileStruct* createdProjectile_baiken5D(PlayerInfo& ent);
static const char* canYrcProjectile_baiken5D(PlayerInfo& ent);
static const char* canYrcProjectile_baikenJD(PlayerInfo& ent);
static const char* canYrcProjectile_jackoJD(PlayerInfo& ent);
static const char* canYrcProjectile_scroll(PlayerInfo& ent);
static const char* canYrcProjectile_clone(PlayerInfo& ent);
static const char* canYrcProjectile_meishiMeteor(PlayerInfo& ent);
static const CreatedProjectileStruct* createdProjectile_firesale(PlayerInfo& ent);
static const char* canYrcProjectile_firesale(PlayerInfo& ent);
static const char* canYrcProjectile_disc(PlayerInfo& ent);
static const char* canYrcProjectile_silentForce(PlayerInfo& ent);
static const char* canYrcProjectile_emeraldRain(PlayerInfo& ent);
static const char* canYrcProjectile_eddie(PlayerInfo& ent);
static const char* canYrcProjectile_amorphous(PlayerInfo& ent);
static const char* canYrcProjectile_slideHead(PlayerInfo& ent);
static const char* canYrcProjectile_fdb(PlayerInfo& ent);
static const char* canYrcProjectile_trishula(PlayerInfo& ent);
static const char* canYrcProjectile_giganter(PlayerInfo& ent);
static const char* canYrcProjectile_berryPull(PlayerInfo& ent);
static const char* canYrcProjectile_bazooka(PlayerInfo& ent);
static const char* canYrcProjectile_graviertWurde(PlayerInfo& ent);
static const char* canYrcProjectile_stahlWirbel(PlayerInfo& ent);
static const char* canYrcProjectile_card(PlayerInfo& ent);
static const char* canYrcProjectile_renhoukyaku(PlayerInfo& ent);
static const char* canYrcProjectile_caltrops(PlayerInfo& ent);
static const char* canYrcProjectile_ballSeiseiA(PlayerInfo& ent);
static const char* canYrcProjectile_ballSeiseiB(PlayerInfo& ent);
static const char* canYrcProjectile_ballSeiseiC(PlayerInfo& ent);
static const char* canYrcProjectile_ballSeiseiD(PlayerInfo& ent);
static const char* canYrcProjectile_airBallSeiseiA(PlayerInfo& ent);
static const char* canYrcProjectile_airBallSeiseiB(PlayerInfo& ent);
static const char* canYrcProjectile_airBallSeiseiC(PlayerInfo& ent);
static const char* canYrcProjectile_airBallSeiseiD(PlayerInfo& ent);
static const char* canYrcProjectile_stinger(PlayerInfo& ent);
static const char* canYrcProjectile_carcass(PlayerInfo& ent);
static const CreatedProjectileStruct* createdProjectile_ballSet(PlayerInfo& ent);
static const CreatedProjectileStruct* createdProjectile_qv(PlayerInfo& ent);
static const char* canYrcProjectile_eatMeat(PlayerInfo& ent);
static const char* canYrcProjectile_eatMeatOkawari(PlayerInfo& ent);
static const char* canYrcProjectile_voltecDein(PlayerInfo& ent);

static const char* powerup_may6P(PlayerInfo& ent);
static const char* powerup_may6H(PlayerInfo& ent);
static const char* powerup_qvA(PlayerInfo& ent);
static const char* powerup_qvB(PlayerInfo& ent);
static const char* powerup_qvC(PlayerInfo& ent);
static const char* powerup_qvD(PlayerInfo& ent);
static const char* powerup_kyougenA(PlayerInfo& ent);
static const char* powerup_kyougenB(PlayerInfo& ent);
static const char* powerup_kyougenC(PlayerInfo& ent);
static const char* powerup_kyougenD(PlayerInfo& ent);
static bool projectilePowerup_onpu(ProjectileInfo& projectile);
static bool projectilePowerup_grenadeBomb(ProjectileInfo& projectile);
static const char* powerup_djavu(PlayerInfo& ent);
static const char* powerup_boomerangA(PlayerInfo& ent);
static bool dontShowPowerupGraphic_boomerangA(PlayerInfo& ent);
static const char* powerup_boomerangAAir(PlayerInfo& ent);
static bool dontShowPowerupGraphic_boomerangAAir(PlayerInfo& ent);
static const char* powerup_boomerangB(PlayerInfo& ent);
static bool dontShowPowerupGraphic_boomerangB(PlayerInfo& ent);
static const char* powerup_boomerangBAir(PlayerInfo& ent);
static bool dontShowPowerupGraphic_boomerangBAir(PlayerInfo& ent);
static const char* powerup_taskB(PlayerInfo& ent);
static bool dontShowPowerupGraphic_taskB(PlayerInfo& ent);
static const char* powerup_taskBAir(PlayerInfo& ent);
static bool dontShowPowerupGraphic_taskBAir(PlayerInfo& ent);
static const char* powerup_taskC(PlayerInfo& ent);
static bool dontShowPowerupGraphic_taskC(PlayerInfo& ent);
static const char* powerup_taskCAir(PlayerInfo& ent);
static bool dontShowPowerupGraphic_taskCAir(PlayerInfo& ent);
static const char* powerup_stingerS(PlayerInfo& ent);
static const char* powerup_stingerH(PlayerInfo& ent);
static bool projectilePowerup_closeShot(ProjectileInfo& ent);
static const char* powerup_rifle(PlayerInfo& ent);
static const char* powerup_beakDriver(PlayerInfo& ent);
static bool dontShowPowerupGraphic_beakDriver(PlayerInfo& ent);
static const char* powerup_mistFiner(PlayerInfo& ent);
static const char* powerup_eatMeat(PlayerInfo& ent);
static const char* powerup_cardK(PlayerInfo& ent);
static const char* powerup_cardS(PlayerInfo& ent);
static const char* powerup_cardH(PlayerInfo& ent);
static const char* powerup_hayabusaRev(PlayerInfo& ent);
static const char* powerup_hayabusaHeld(PlayerInfo& ent);
static const char* powerup_grampaMax(PlayerInfo& ent);
static const char* powerup_antiAir4Hasei(PlayerInfo& ent);
static const char* powerup_blackHoleAttack(PlayerInfo& ent);
static const char* powerup_armorDance(PlayerInfo& ent);
static const char* powerup_fireSpear(PlayerInfo& ent);
static const char* powerup_zweit(PlayerInfo& ent);
static const char* powerup_secretGarden(PlayerInfo& ent);
static const char* powerup_pickUpGhost(PlayerInfo& ent);
static const char* powerup_putGhost(PlayerInfo& ent);
static const char* powerup_returnGhost(PlayerInfo& ent);
static const char* powerup_dizzy6H(PlayerInfo& ent);

static void fillMay6HOffsets(BYTE* func);

static void charge_may6P(PlayerInfo& ent, ChargeData* result);
static void charge_may6H(PlayerInfo& ent, ChargeData* result);
static void charge_standingBlitzShield(PlayerInfo& ent, ChargeData* result);
static void charge_crouchingBlitzShield(PlayerInfo& ent, ChargeData* result);
static void charge_fdb(PlayerInfo& ent, ChargeData* result);
static void charge_soutenBC(PlayerInfo& ent, ChargeData* result);
static void charge_dubiousCurve(PlayerInfo& ent, ChargeData* result);
static void charge_stingerAim(PlayerInfo& ent, ChargeData* result);
static void charge_beakDriver(PlayerInfo& ent, ChargeData* result);
static void charge_elpheltStand(PlayerInfo& ent, ChargeData* result);
static void charge_elpheltCrouch2Stand(PlayerInfo& ent, ChargeData* result);
static void charge_elpheltLanding(PlayerInfo& ent, ChargeData* result);
static void charge_elpheltDashStop(PlayerInfo& ent, ChargeData* result);
static void charge_shotgunCharge(PlayerInfo& ent, ChargeData* result);
static void charge_rifleCharge(PlayerInfo& ent, ChargeData* result);
static void charge_treasureHunt(PlayerInfo& ent, ChargeData* result);
static void charge_stepTreasureHunt(PlayerInfo& ent, ChargeData* result);
static void charge_antiAir6Hasei(PlayerInfo& ent, ChargeData* result);
static void charge_landBlow4Hasei(PlayerInfo& ent, ChargeData* result);
static void charge_landBlow6Hasei(PlayerInfo& ent, ChargeData* result);
static void charge_antiAir4Hasei(PlayerInfo& ent, ChargeData* result);
static void charge_blackHoleAttack(PlayerInfo& ent, ChargeData* result);
static void charge_dizzy6H(PlayerInfo& ent, ChargeData* result);
static void charge_kinomiNecro(PlayerInfo& ent, ChargeData* result);

static MoveInfoProperty& newProperty(MoveInfoStored* move, DWORD property) {
	if (moves.justCountingMoves) {
		++moves.propertiesCount;
		static MoveInfoProperty placeholderProp;
		return placeholderProp;
	}
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
	
	if (justCountingMoves) {
		++movesCount;
		return;
	}
	
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

MoveInfo::MoveInfo() : isIdle(isIdle_default), canBlock(canBlock_default) { }

MoveInfo::MoveInfo(const MoveInfoStored& info) {
	const MoveInfoProperty* prop = info.startPtr;
	for (int i = 0; i < info.count; ++i) {
		*(DWORD*)((BYTE*)this + prop->type) = (DWORD&)prop->u.strValue;
		++prop;
	}
	if (!canBlock && isIdle) canBlock = isIdle;
	if (!isIdle) isIdle = isIdle_default;
	if (!canBlock) canBlock = canBlock_default;
}

void MoveInfo::addForceAddWhiffCancel(const char* name) {
	if (moves.justCountingMoves) {
		++moves.forceAddWhiffCancelsTotalCount;
		return;
	}
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
	
	justCountingMoves = true;
	addMoves();
	justCountingMoves = false;
	
	map.reserve(movesCount);
	forceAddWhiffCancels.reserve(forceAddWhiffCancelsTotalCount);
	allProperties.reserve(propertiesCount);
	
	addMoves();
	
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

#define rememberFramebarId(name) const int name = __COUNTER__

#define generateFramebarId() __COUNTER__

void Moves::addMoves() {
	MoveInfo move;
	
	move = MoveInfo(GENERAL, "CmnStandForce");
	move.displayName = assignName("Stand");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnNeutral");
	move.displayName = assignName("Neutral");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "RomanCancelHit");
	move.displayName = assignName("Roman Cancel", "RC");
	move.displayNameSelector = displayNameSelector_RC;
	move.nameIncludesInputs = true;
	addMove(move);
	
	rememberFramebarId(Burst_framebarId);

	move = MoveInfo(GENERAL, "cmn_BurstObjGoldHontai", true);
	move.framebarName = assignName("Gold Burst");
	move.framebarId = Burst_framebarId;
	addMove(move);
	
	move = MoveInfo(GENERAL, "cmn_BurstObjBlueHontai", true);
	move.framebarName = assignName("Blue Burst");
	move.framebarNameSelector = framebarNameSelector_blueBurst;
	move.framebarId = Burst_framebarId;
	addMove(move);
	
	// This was spotted when throwing Blue Burst on the very frame it comes out
	move = MoveInfo(GENERAL, "cmn_BurstObjBlueObject", true);
	move.framebarName = assignName("Blue Burst");
	move.framebarNameSelector = framebarNameSelector_blueBurst;
	move.framebarId = Burst_framebarId;
	addMove(move);
	
	// This was spotted when throwing Gold Burst on the very frame it comes out
	move = MoveInfo(GENERAL, "cmn_BurstObjGoldObject", true);
	move.framebarName = assignName("Gold Burst");
	move.framebarId = Burst_framebarId;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnDamageBurst");
	move.displayName = assignName("Blue Burst");
	move.displayNameSelector = displayNameSelector_blueBurst;
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnMaximumBurst");
	move.displayName = assignName("Gold Burst");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "FaultlessDefenceCrouch");
	move.displayName = assignName("Crouching Faultless Defense", "Crouching FD");
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "FaultlessDefenceAir");
	move.displayName = assignName("Air Faultless Defense", "Air FD");
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "FaultlessDefenceStand");
	move.displayName = assignName("Standing Faultless Defense", "Standing FD");
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "DeadAngleAttack");
	move.displayName = assignName("Dead Angle Attack", "DAA");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "IchigekiJunbi");
	move.displayName = assignName("Instant Kill Activation", "IK Activation");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "Ichigeki");
	move.displayName = assignName("Instant Kill", "IK");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActStand");
	move.displayName = assignName("Stand");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActStandTurn");
	move.displayName = assignName("Stand Turn");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActStand2Crouch");
	move.displayName = assignName("Stand to Crouch");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActCrouch");
	move.displayName = assignName("Crouch");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActCrouchTurn");
	move.displayName = assignName("Crouch Turn");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActCrouch2Stand");
	move.displayName = assignName("Crouch to Stand");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActJumpPre");
	move.displayName = assignName("Prejump");
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActJump");
	move.displayName = assignName("Jump");
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActJumpLanding");
	move.displayName = assignName("Landing");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActLandingStiff");
	move.displayName = assignName("Landing Recovery");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActFWalk");
	move.displayName = assignName("Walk Forward");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnFWalk");
	move.displayName = assignName("Walk Forward");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActBWalk");
	move.displayName = assignName("Walk Back");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnBWalk");
	move.displayName = assignName("Walk Back");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActFDash");
	move.displayName = assignName("Forward Dash");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);

	move = MoveInfo(GENERAL, "CmnFDash");
	move.displayName = assignName("Forward Dash");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActFDashStop");
	move.displayName = assignName("Forward Dash Stop");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActBDash");
	move.displayName = assignName("Backdash", "BD");
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnBDash");
	move.displayName = assignName("Backdash", "BD");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActAirFDash");
	move.displayName = assignName("Airdash Forward", "ADF");
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnFAirDash");
	move.displayName = assignName("Airdash Forward", "ADF");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActAirBDash");
	move.displayName = assignName("Airdash Back", "ADB");
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnBAirDash");
	move.displayName = assignName("Airdash Back", "ADB");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "HomingDash2");
	move.displayName = assignName("Homing Dash");
	move.ignoreJumpInstalls = true;
	addMove(move);

	move = MoveInfo(GENERAL, "HomingJump");
	move.displayName = assignName("Homing Jump");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActNokezoriHighLv1");
	move.displayName = assignName("Hitstun High Lv1");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActNokezoriHighLv2");
	move.displayName = assignName("Hitstun High Lv2");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActNokezoriHighLv3");
	move.displayName = assignName("Hitstun High Lv3");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActNokezoriHighLv4");
	move.displayName = assignName("Hitstun High Lv4");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActNokezoriHighLv5");
	move.displayName = assignName("Hitstun High Lv5");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActNokezoriLowLv1");
	move.displayName = assignName("Hitstun Low Lv1");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActNokezoriLowLv2");
	move.displayName = assignName("Hitstun Low Lv2");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActNokezoriLowLv3");
	move.displayName = assignName("Hitstun Low Lv3");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActNokezoriLowLv4");
	move.displayName = assignName("Hitstun Low Lv4");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActNokezoriLowLv5");
	move.displayName = assignName("Hitstun Low Lv5");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActNokezoriCrouchLv1");
	move.displayName = assignName("Hitstun Crouch Lv1");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActNokezoriCrouchLv2");
	move.displayName = assignName("Hitstun Crouch Lv2");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActNokezoriCrouchLv3");
	move.displayName = assignName("Hitstun Crouch Lv3");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActNokezoriCrouchLv4");
	move.displayName = assignName("Hitstun Crouch Lv4");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActNokezoriCrouchLv5");
	move.displayName = assignName("Hitstun Crouch Lv5");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActBDownUpper");
	move.displayName = assignName("Launched Into Air Back");
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActBDownUpperEnd");
	move.displayName = assignName("Starting To Fall Back");
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActBDownDown");
	move.displayName = assignName("Falling Down Back");
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActBDownBound");
	move.displayName = assignName("Fell Down On The Back");
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActBDownLoop");
	move.displayName = assignName("Lying On The Back");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActBDown2Stand");
	move.displayName = assignName("Waking Up On The Back");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActFDownUpper");
	move.displayName = assignName("Launched Into Air Forward");
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActFDownUpperEnd");
	move.displayName = assignName("Starting To Fall Forward");
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActFDownDown");
	move.displayName = assignName("Falling Down Forward");
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActFDownBound");
	move.displayName = assignName("Fell Down Forward");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActFDownLoop");
	move.displayName = assignName("Lying On The Face");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActFDown2Stand");
	move.displayName = assignName("Waking Up Face Down");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActVDownUpper");
	move.displayName = assignName("Launched Into Air Vertically");
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActVDownUpperEnd");
	move.displayName = assignName("Starting To Fall Vertically");
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActVDownDown");
	move.displayName = assignName("Falling Down Vertical");
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActVDownBound");
	move.displayName = assignName("Fell Down Vertical");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActVDownLoop");
	move.displayName = assignName("Lying On The Face (From Vertical Fall)");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActBlowoff");
	move.displayName = assignName("Blown Off");
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActKirimomiUpper");
	move.displayName = assignName("Launched");
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActWallBound");
	move.displayName = assignName("Wallbounce");
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActWallBoundDown");
	move.displayName = assignName("Falling Down From Wallbounce");
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActWallHaritsuki");
	move.displayName = assignName("Wallsplat/Wallslump");
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActWallHaritsukiLand");
	move.displayName = assignName("Landed From Wallslump");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActWallHaritsukiGetUp");
	move.displayName = assignName("Waking Up From Wallslump");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActJitabataLoop");
	move.displayName = assignName("Stagger");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActKizetsu");
	move.displayName = assignName("Faint");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActHizakuzure");
	move.displayName = assignName("Crumple");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActKorogari");
	move.displayName = assignName("Tumble");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActZSpin");
	move.displayName = assignName("Spinning");
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActUkemi");
	move.displayName = assignName("Air Recovery");
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActMidGuardPre");
	move.displayName = assignName("Pre Mid Block");
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActMidGuardLoop");
	move.displayName = assignName("Mid Block");
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActMidGuardEnd");
	move.displayName = assignName("Mid Block End");
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActHighGuardPre");
	move.displayName = assignName("Pre High Block");
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActHighGuardLoop");
	move.displayName = assignName("High Block");
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActHighGuardEnd");
	move.displayName = assignName("High Block End");
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActCrouchGuardPre");
	move.displayName = assignName("Pre Crouching Block");
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActCrouchGuardLoop");
	move.displayName = assignName("Crouching Block");
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActCrouchGuardEnd");
	move.displayName = assignName("Crouching Block End");
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActAirGuardPre");
	move.displayName = assignName("Pre Air Block");
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActAirGuardLoop");
	move.displayName = assignName("Air Block");
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActAirGuardEnd");
	move.displayName = assignName("Air Block End");
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActHajikareStand");
	move.displayName = assignName("Stand Rejected");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActHajikareCrouch");
	move.displayName = assignName("Crouch Rejected");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActHajikareAir");
	move.displayName = assignName("Air Rejected");
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActAirTurn");
	move.displayName = assignName("Air Turn");
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActLockWait");
	move.displayName = assignName("Grabbed");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActLockReject");
	move.displayName = assignName("CmnActLockReject");
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActAirLockWait");
	move.displayName = assignName("CmnActAirLockWait");
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActAirLockReject");
	move.displayName = assignName("CmnActAirLockReject");
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActItemUse");
	move.displayName = assignName("Item");
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActBurst");
	move.displayName = assignName("Burst");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActRomanCancel");
	move.displayName = assignName("Roman Cancel", "RC");
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActEntry");
	move.displayName = assignName("CmnActEntry");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActRoundWin");
	move.displayName = assignName("Round Win");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActMatchWin");
	move.displayName = assignName("Match Win");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActLose");
	move.displayName = assignName("CmnActLose");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActResultWin");
	move.displayName = assignName("Victory Screen Win");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActResultLose");
	move.displayName = assignName("Victory Screen Lose");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActEntryWait");
	move.displayName = assignName("Invisible");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActExDamage");
	move.displayName = assignName("Ex Damage");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnActExDamageLand");
	move.displayName = assignName("Ground Ex Damage");
	move.ignoreJumpInstalls = true;
	addMove(move);

	move = MoveInfo(GENERAL, "NmlAtk5A");
	move.displayName = assignName("5P");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk5B");
	move.displayName = assignName("5K");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk5C");
	move.displayName = assignName("5S");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk5CNear");
	move.displayName = assignName("c.S");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk5CFar");
	move.displayName = assignName("f.S");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk5D");
	move.displayName = assignName("5H");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk5E");
	move.displayName = assignName("5D");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk5F");
	move.displayName = assignName("Taunt");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk2A");
	move.displayName = assignName("2P");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk2B");
	move.displayName = assignName("2K");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk2C");
	move.displayName = assignName("2S");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk2D");
	move.displayName = assignName("2H");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk2E");
	move.displayName = assignName("2D");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk1A");
	move.displayName = assignName("1P");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk1B");
	move.displayName = assignName("1K");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk1C");
	move.displayName = assignName("1S");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk1D");
	move.displayName = assignName("1H");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk1E");
	move.displayName = assignName("1D");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk3A");
	move.displayName = assignName("3P");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk3B");
	move.displayName = assignName("3K");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk3C");
	move.displayName = assignName("3S");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk3D");
	move.displayName = assignName("3H");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk3E");
	move.displayName = assignName("3D");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk4A");
	move.displayName = assignName("4P");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk4B");
	move.displayName = assignName("4K");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk4C");
	move.displayName = assignName("4S");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk4D");
	move.displayName = assignName("4H");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk4E");
	move.displayName = assignName("4D");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk6A");
	move.displayName = assignName("6P");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk6B");
	move.displayName = assignName("6K");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk6C");
	move.displayName = assignName("6S");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk6D");
	move.displayName = assignName("6H");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk6E");
	move.displayName = assignName("6D");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtk6F");
	move.displayName = assignName("Respect");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtkAir5A");
	move.displayName = assignName("j.P");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtkAir5B");
	move.displayName = assignName("j.K");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtkAir5C");
	move.displayName = assignName("j.S");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtkAir5D");
	move.displayName = assignName("j.H");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtkAir5E");
	move.displayName = assignName("j.D");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtkAir2A");
	move.displayName = assignName("j.2P");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtkAir2B");
	move.displayName = assignName("j.2K");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtkAir2C");
	move.displayName = assignName("j.2S");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtkAir2D");
	move.displayName = assignName("j.2H");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtkAir2E");
	move.displayName = assignName("j.2D");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtkAir4A");
	move.displayName = assignName("j.4P");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtkAir4B");
	move.displayName = assignName("j.4K");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtkAir4C");
	move.displayName = assignName("j.4S");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtkAir4D");
	move.displayName = assignName("j.4H");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtkAir4E");
	move.displayName = assignName("j.4D");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtkAir6A");
	move.displayName = assignName("j.6P");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtkAir6B");
	move.displayName = assignName("j.6K");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtkAir6C");
	move.displayName = assignName("j.6S");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtkAir6D");
	move.displayName = assignName("j.6H");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtkAir6E");
	move.displayName = assignName("j.6D");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "NmlAtkAir8E");
	move.displayName = assignName("j.8D");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnVJump");
	move.displayName = assignName("Jump Neutral");
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnFJump");
	move.displayName = assignName("Jump Forward");
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnBJump");
	move.displayName = assignName("Jump Back");
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnVHighJump");
	move.displayName = assignName("Superjump Neutral");
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnFHighJump");
	move.displayName = assignName("Superjump Forward");
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnBHighJump");
	move.displayName = assignName("Superjump Back");
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnVAirJump");
	move.displayName = assignName("Double Jump Neutral");
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnFAirJump");
	move.displayName = assignName("Double Jump Forward");
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CmnBAirJump");
	move.displayName = assignName("Double Jump Back");
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "ThrowExe");
	move.displayName = assignName("Ground Throw");
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "AirThrowExe");
	move.displayName = assignName("Airthrow");
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CounterGuardStand");
	move.displayName = assignName("Standing Blitz Shield", "Standing Blitz");
	move.displayNameSelector = displayNameSelector_standingBlitzShield;
	move.sectionSeparator = sectionSeparator_blitzShield;
	move.isInVariableStartupSection = isInVariableStartupSection_blitzShield;
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	move.charge = charge_standingBlitzShield;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CounterGuardCrouch");
	move.displayName = assignName("Crouching Blitz Shield", "Crouching Blitz");
	move.displayNameSelector = displayNameSelector_crouchingBlitzShield;
	move.sectionSeparator = sectionSeparator_blitzShield;
	move.isInVariableStartupSection = isInVariableStartupSection_blitzShield;
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	move.charge = charge_crouchingBlitzShield;
	addMove(move);
	
	move = MoveInfo(GENERAL, "CounterGuardAir");
	move.displayName = assignName("Air Blitz Shield", "Air Blitz");
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "BanditRevolverLand");
	move.displayName = assignName("Bandit Revolver", "BR");
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "BanditRevolverLand_DI");
	move.displayName = assignName("DI Bandit Revolver", "DI BR");
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "GunFlame");
	move.displayName = assignName("Gunflame", "GF");
	move.displayNameSelector = displayNameSelector_gunflame;
	move.canYrcProjectile = canYrcProjectile_gunflame;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "GunFlame_DI");
	move.displayName = assignName("DI Gunflame", "DI GF");
	move.displayNameSelector = displayNameSelector_gunflameDI;
	move.canYrcProjectile = canYrcProjectile_gunflame;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "BanditRevolverAir");
	move.displayName = assignName("Air Bandit Revolver", "Air BR");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "BanditRevolverAir_DI");
	move.displayName = assignName("DI Air Bandit Revolver", "DI Air BR");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "GroundViper");
	move.displayName = assignName("Ground Viper", "GV");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "GroundViper_DI");
	move.displayName = assignName("DI Ground Viper", "DI GV");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "RiotStamp");
	move.displayName = assignName("Riot Stamp", "RS");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "RiotStamp_DI");
	move.displayName = assignName("DI Riot Stamp", "DI RS");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "GunFlameFeint");
	move.displayName = assignName("Gunflame Feint", "GF Feint");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "Kudakero");
	move.displayName = assignName("Break");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "Kudakero_DI");
	move.displayName = assignName("DI Break");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "VolcanicViperLandHS");
	move.displayName = assignName("H Volcanic Viper", "hVV");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "VolcanicViperLandHS_DI");
	move.displayName = assignName("DI H Volcanic Viper", "DI hVV");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "VolcanicViperLandS");
	move.displayName = assignName("S Volcanic Viper", "sVV");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "VolcanicViperLandS_DI");
	move.displayName = assignName("DI S Volcanic Viper", "DI sVV");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "AirCommandThrow");
	move.displayName = assignName("P.B.B.");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "AirCommandThrowExe");
	move.displayName = assignName("P.B.B.");
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "VolcanicViperAirHS");
	move.displayName = assignName("Air H Volcanic Viper", "Air HVV");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "VolcanicViperAirHS_DI");
	move.displayName = assignName("DI Air H Volcanic Viper", "DI Air HVV");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "VolcanicViperAirS");
	move.displayName = assignName("Air S Volcanic Viper", "Air SVV");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "VolcanicViperAirS_DI");
	move.displayName = assignName("DI Air S Volcanic Viper", "DI Air SVV");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "Fefnir");
	move.displayName = assignName("Fafnir");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "Fefnir_DI");
	move.displayName = assignName("DI Fafnir");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "BanditBringer");
	move.displayName = assignName("Bandit Bringer", "BB");
	move.combineWithPreviousMove = true;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "BanditBringer_DI");
	move.displayName = assignName("DI Bandit Bringer", "DI BB");
	move.combineWithPreviousMove = true;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "VolcanicViperKick");
	move.displayName = assignName("Volcanic Viper Knockdown", "VV KD");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "VolcanicViperKick_DI");
	move.displayName = assignName("DI Volcanic Viper Knockdown", "DI VV KD");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "TyrantRave");
	move.displayName = assignName("Tyrant Rave", "TR");
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_tyrantRave;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "TyrantRave_DI");
	move.displayName = assignName("DI Tyrant Rave", "DI TR");
	move.iKnowExactlyWhenTheRecoveryOfThisMoveIs = isRecovery_recovery;
	move.canYrcProjectile = canYrcProjectile_tyrantRave;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "TyrantRaveBurst");
	move.displayName = assignName("Burst Tyrant Rave", "Burst TR");
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_tyrantRave;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "DragonInstall");
	move.displayName = assignName("Dragon Install", "DI");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "ExKizetsu");
	move.displayName = assignName("DI Recovery");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "BukkiraExe");
	move.displayName = assignName("Wild Throw", "WT");
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "BukkirabouNiNageru");
	move.displayName = assignName("Wild Throw", "WT");
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	rememberFramebarId(GunFlame_framebarId);

	move = MoveInfo(CHARACTER_TYPE_SOL, "GunFlameStart", true);
	move.framebarId = GunFlame_framebarId;
	move.framebarName = assignName("Gunflame", "GF");
	move.framebarNameSelector = framebarNameSelector_gunflameProjectile;
	move.framebarNameUncombined = assignName("Gunflame Spawner", "GF Spawner");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "GunFlameHibashira", true);
	move.isDangerous = isDangerous_gunflame;
	move.framebarId = GunFlame_framebarId;
	move.framebarName = assignName("Gunflame", "GF");
	move.framebarNameSelector = framebarNameSelector_gunflameProjectile;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "GunFlameHibashira_DI", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = GunFlame_framebarId;
	move.framebarName = assignName("Gunflame", "GF");
	move.framebarNameSelector = framebarNameSelector_gunflameProjectile;
	addMove(move);
	
	rememberFramebarId(TyrantRavePunch2_framebarId);

	move = MoveInfo(CHARACTER_TYPE_SOL, "TyrantRavePunch2", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = TyrantRavePunch2_framebarId;
	move.framebarName = assignName("Tyrant Rave", "TR");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "TyrantRavePunch2_DI", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = TyrantRavePunch2_framebarId;
	move.framebarName = assignName("Tyrant Rave", "TR");
	addMove(move);
	
	rememberFramebarId(KudakeroEF_framebarId);

	move = MoveInfo(CHARACTER_TYPE_SOL, "KudakeroEF", true);
	move.isDangerous = isDangerous_not_NullWhileActive;
	move.framebarId = KudakeroEF_framebarId;
	move.framebarName = assignName("Break Explosion");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "KudakeroEF_DI", true);
	move.isDangerous = isDangerous_not_NullWhileActive;
	move.framebarId = KudakeroEF_framebarId;
	move.framebarName = assignName("Break Explosion");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "RiotStamp_DI_Bomb", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("Riot Stamp", "RS");
	move.combineHitsFromDifferentProjectiles = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SOL, "GroundViperDash_DI", true);
	move.isDangerous = isDangerous_notNull;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("GV Fire Pillars");
	move.framebarNameFull = "Ground Viper Fire Pillars";
	move.framebarNameUncombined = assignName("GV Fire Pillar");
	move.combineHitsFromDifferentProjectiles = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "StunEdge2");
	move.displayName = assignName("Charged Stun Edge", "CSE");
	move.canYrcProjectile = canYrcProjectile_cse;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "StunEdge1");
	move.displayName = assignName("Stun Edge", "SE");
	move.canYrcProjectile = canYrcProjectile_se;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "StunDipper");
	move.displayName = assignName("Stun Dipper");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "HolyBrand");
	move.displayName = assignName("Split Ciel");
	move.createdProjectile = createdProjectile_splitCiel;
	move.canYrcProjectile = canYrcProjectile_splitCiel;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "GreedSaber");
	move.displayName = assignName("Greed Sever");
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "AirStunEdge2");
	move.displayName = assignName("Air H Stun Edge", "Air H SE");
	move.canYrcProjectile = canYrcProjectile_se;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "AirStunEdge1");
	move.displayName = assignName("Air S Stun Edge", "Air S SE");
	move.canYrcProjectile = canYrcProjectile_se;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "VaporThrustD");
	move.displayName = assignName("H Vapor Thrust", "HVT");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "VaporThrustC");
	move.displayName = assignName("S Vapor Thrust", "SVT");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "AirVaporThrustD");
	move.displayName = assignName("Air H Vapor Thrust", "Air HVT");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "AirVaporThrust");
	move.displayName = assignName("Air S Vapor Thrust", "Air SVT");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "SacredEdge");
	move.displayName = assignName("Sacred Edge");
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_sacredEdge;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "RideTheLightning");
	move.displayName = assignName("Ride The Lightning", "RTL");
	move.iKnowExactlyWhenTheRecoveryOfThisMoveIs = isRecovery_RTL;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "RideTheLightningBurst");
	move.displayName = assignName("Burst Ride The Lightning", "Burst RTL");
	move.iKnowExactlyWhenTheRecoveryOfThisMoveIs = isRecovery_RTL;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "AirRideTheLightning");
	move.displayName = assignName("Air Ride The Lightning", "Air RTL");
	move.iKnowExactlyWhenTheRecoveryOfThisMoveIs = isRecovery_RTL;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "AirRideTheLightningBurst");
	move.displayName = assignName("Air Burst Ride The Lightning", "Air Burst RTL");
	move.iKnowExactlyWhenTheRecoveryOfThisMoveIs = isRecovery_RTL;
	move.isMove = true;
	addMove(move);
	
	rememberFramebarId(StunEdgeObj_framebarId);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "StunEdgeObj", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = StunEdgeObj_framebarId;
	move.framebarName = assignName("Stun Edge", "SE");
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "Mahojin", true);
	move.framebarName = assignName("Durandal Call Grinder", "DC Grinder");  // can get displayed in the framebar due to clashing with an opponent's projectile
	move.framebarId = generateFramebarId();
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "SPStunEdgeObj", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = StunEdgeObj_framebarId;
	move.framebarName = assignName("DCSE");
	move.framebarNameFull = "Fortified Stun Edge (Durandal Call Stun Edge)";
	addMove(move);
	
	rememberFramebarId(ChargedStunEdgeObj_framebarId);

	move = MoveInfo(CHARACTER_TYPE_KY, "ChargedStunEdgeObj", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = ChargedStunEdgeObj_framebarId;
	move.framebarName = assignName("CSE");
	move.framebarNameFull = "Charged Stun Edge";
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "SPChargedStunEdgeObj", true);
	move.isDangerous = isDangerous_not_NullWhileActive;
	move.framebarId = ChargedStunEdgeObj_framebarId;
	move.framebarName = assignName("DCCSE");
	move.framebarNameFull = "Fortified Charged Stun Edge (Drandal Call Charged Stun Edge)";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "AirDustAttackObj", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("j.D");
	move.framebarNameUncombined = assignName("j.D Grinder");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "NmlAtk5E");
	move.displayName = assignName("5D");
	move.nameIncludesInputs = true;
	move.createdProjectile = createdProjectile_ky5D;
	move.canYrcProjectile = canYrcProjectile_ky5D;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "NmlAtkAir5E");
	move.displayName = assignName("j.D");
	move.nameIncludesInputs = true;
	move.canYrcProjectile = canYrcProjectile_kyJD;
	move.isMove = true;
	addMove(move);
	
	// can't YRC in Rev1. In fact this doesn't even exist in Rev1
	move = MoveInfo(CHARACTER_TYPE_KY, "DustEffectShot", true);
	move.isDangerous = isDangerous_playerInRCOrHasActiveFlag_AndNotInRecovery;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("5D");
	addMove(move);
	
	rememberFramebarId(SacredEdgeObj_framebarId);

	move = MoveInfo(CHARACTER_TYPE_KY, "SacredEdgeObj", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = SacredEdgeObj_framebarId;
	move.framebarName = assignName("Sacred Edge");
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_KY, "SPSacredEdgeObj", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = SacredEdgeObj_framebarId;
	move.framebarName = assignName("Fortified Sacred Edge", "DC Sacred Edge");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "NmlAtk6B");
	move.displayName = assignName("6K");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "GlitterIsGold");
	move.displayName = assignName("Glitter Is Gold", "Coin");
	move.canYrcProjectile = canYrcProjectile_coin;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "BucchusSigh");
	move.displayName = assignName("Bacchus Sigh", "Bacchus");
	move.canYrcProjectile = canYrcProjectile_bacchusSigh;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	rememberFramebarId(Mist_framebarId);

	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "Mist", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarName = PROJECTILE_NAME_BACCHUS;
	move.framebarId = Mist_framebarId;
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistKuttsuku", true);
	move.isDangerous = isDangerous_mistKuttsuku;
	move.framebarName = assignName("Bacchus Sigh Debuff", "Bacchus");
	move.framebarId = Mist_framebarId;
	addMove(move);
	
	// the initial move of grounded Mist Finer, is 1f long
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistFinerA");
	move.displayName = assignName("P Mist Finer Stance Entry", "PMF Entry");
	move.displayNameSelector = displayNameSelector_mistEntry;
	move.partOfStance = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistFinerB");
	move.displayName = assignName("K Mist Finer Stance Entry", "KMF Entry");
	move.displayNameSelector = displayNameSelector_mistEntry;
	move.partOfStance = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistFinerC");
	move.displayName = assignName("S Mist Finer Stance Entry", "SMF Entry");
	move.displayNameSelector = displayNameSelector_mistEntry;
	move.partOfStance = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	// is entered into from MistFinerA/B/C, has variable duration depending on Mist Finer level
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistFinerDehajime");
	move.displayName = assignName("Mist Finer Entry", "MF Entry");
	move.displayNameSelector = displayNameSelector_mistEntry;
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	// entered into from MistFinerDehajime, enables whiff cancels on f2.
	// In Rev2 is exited out of instantly into another Mist Finer from things like MistFinerFWalk.
	// In Rev1 takes one frame to transition.
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistFinerLoop");
	move.displayName = assignName("Mist Finer Stance", "MF Stance");
	move.displayNameSelector = displayNameSelector_mistLoop;
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.sectionSeparator = sectionSeparator_enableWhiffCancels;
	move.considerIdleInSeparatedSectionAfterThisManyFrames = 5;
	move.preservesNewSection = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	// performed when releasing the Mist Finer attack
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistFinerALv0");
	move.displayName = assignName("Lv1 P Mist Finer", "Lv1 PMF");
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.powerup = powerup_mistFiner;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistFinerALv1");
	move.displayName = assignName("Lv2 P Mist Finer", "Lv2 PMF");
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.powerup = powerup_mistFiner;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistFinerALv2");
	move.displayName = assignName("Lv3 P Mist Finer", "Lv3 PMF");
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.powerup = powerup_mistFiner;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistFinerBLv0");
	move.displayName = assignName("Lv1 K Mist Finer", "Lv1 KMF");
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.powerup = powerup_mistFiner;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistFinerBLv1");
	move.displayName = assignName("Lv2 K Mist Finer", "Lv2 KMF");
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.powerup = powerup_mistFiner;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistFinerBLv2");
	move.displayName = assignName("Lv3 K Mist Finer", "Lv3 KMF");
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.powerup = powerup_mistFiner;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistFinerCLv0");
	move.displayName = assignName("Lv1 S Mist Finer", "Lv1 SMF");
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.powerup = powerup_mistFiner;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistFinerCLv1");
	move.displayName = assignName("Lv2 S Mist Finer", "Lv2 SMF");
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.powerup = powerup_mistFiner;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistFinerCLv2");
	move.displayName = assignName("Lv3 S Mist Finer", "Lv3 SMF");
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.powerup = powerup_mistFiner;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	// backdash during grounded Mist Finer
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistFinerBDash");
	move.displayName = assignName("Mist Finer Backdash", "MF BD");
	move.displayNameSelector = displayNameSelector_mistBDash;
	move.partOfStance = true;
	move.nameIncludesInputs = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.sectionSeparator = sectionSeparator_mistFinerDash;
	move.considerNewSectionAsBeingInVariableStartup = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistFinerFDash");
	move.displayName = assignName("Mist Finer Forward Dash", "MF Dash");
	move.displayNameSelector = displayNameSelector_mistDash;
	move.partOfStance = true;
	move.nameIncludesInputs = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.sectionSeparator = sectionSeparator_mistFinerDash;
	move.considerNewSectionAsBeingInVariableStartup = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistFinerBWalk");
	move.displayName = assignName("Mist Finer Walk Back", "MF Walk Back");
	move.displayNameSelector = displayNameSelector_mistWalkBackward;
	move.partOfStance = true;
	move.nameIncludesInputs = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.sectionSeparator = sectionSeparator_alwaysTrue;
	move.considerIdleInSeparatedSectionAfterThisManyFrames = 5;
	move.preservesNewSection = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistFinerFWalk");
	move.displayName = assignName("Mist Finer Walk Forward", "MF Walk Forward");
	move.displayNameSelector = displayNameSelector_mistWalkForward;
	move.partOfStance = true;
	move.nameIncludesInputs = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.sectionSeparator = sectionSeparator_alwaysTrue;
	move.considerIdleInSeparatedSectionAfterThisManyFrames = 5;
	move.preservesNewSection = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistFinerCancel");
	move.displayName = assignName("Mist Finer Cancel", "MFC");
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	// the initial move of air Mist Finer, is 1f long
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "AirMistFinerA");
	move.displayName = assignName("Air P Mist Finer Stance Entry", "j.PMF Entry");
	move.displayNameSelector = displayNameSelector_airMistEntry;
	move.partOfStance = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "AirMistFinerB");
	move.displayName = assignName("Air K Mist Finer Stance Entry", "j.KMF Entry");
	move.displayNameSelector = displayNameSelector_airMistEntry;
	move.partOfStance = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "AirMistFinerC");
	move.displayName = assignName("Air S Mist Finer Stance Entry", "j.SMF Entry");
	move.displayNameSelector = displayNameSelector_airMistEntry;
	move.partOfStance = true;
	move.isMove = true;
	addMove(move);
	
	// is entered into from MistFinerA/B/C, has variable duration depending on Mist Finer level
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "AirMistFinerDehajime");
	move.displayName = assignName("Air Mist Finer Entry", "j.MF Entry");
	move.displayNameSelector = displayNameSelector_airMistEntry;
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	addMove(move);
	
	// entered into from AirMistFinerDehajime, enables whiff cancels on f2.
	// In Rev2 is exited out of instantly into another Mist Finer from things like MistFinerFWalk.
	// In Rev1 takes one frame to transition.
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "AirMistFinerLoop");
	move.displayName = assignName("Air Mist Finer Stance", "j.MF Stance");
	move.displayNameSelector = displayNameSelector_airMistLoop;
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.sectionSeparator = sectionSeparator_enableWhiffCancels;
	move.considerIdleInSeparatedSectionAfterThisManyFrames = 5;
	move.preservesNewSection = true;
	addMove(move);
	
	// forward dash during air Mist Finer
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistFinerFDashAir");
	move.displayName = assignName("Air Mist Finer Forward Dash", "j.MF Forward Dash");
	move.displayNameSelector = displayNameSelector_airMistDash;
	move.partOfStance = true;
	move.nameIncludesInputs = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.sectionSeparator = sectionSeparator_mistFinerAirDash;
	move.considerNewSectionAsBeingInVariableStartup = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "MistFinerBDashAir");
	move.displayName = assignName("Air Mist Finer Backdash", "j.MF BD");
	move.displayNameSelector = displayNameSelector_airMistBackdash;
	move.partOfStance = true;
	move.nameIncludesInputs = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.sectionSeparator = sectionSeparator_mistFinerAirDash;
	move.considerNewSectionAsBeingInVariableStartup = true;
	addMove(move);
	
	// performed when releasing the Mist Finer attack
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "AirMistFinerALv0");
	move.displayName = assignName("Lv1 Air P Mist Finer", "Lv1 j.PMF");
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.powerup = powerup_mistFiner;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "AirMistFinerALv1");
	move.displayName = assignName("Lv2 Air P Mist Finer", "Lv2 j.PMF");
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.powerup = powerup_mistFiner;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "AirMistFinerALv2");
	move.displayName = assignName("Lv3 Air P Mist Finer", "Lv3 j.PMF");
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.powerup = powerup_mistFiner;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "AirMistFinerBLv0");
	move.displayName = assignName("Lv1 Air K Mist Finer", "Lv1 j.KMF");
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.powerup = powerup_mistFiner;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "AirMistFinerBLv1");
	move.displayName = assignName("Lv2 Air K Mist Finer", "Lv2 j.KMF");
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.powerup = powerup_mistFiner;
	move.usePlusSignInCombination = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "AirMistFinerBLv2");
	move.displayName = assignName("Lv3 Air K Mist Finer", "Lv3 j.KMF");
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.powerup = powerup_mistFiner;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "AirMistFinerCLv0");
	move.displayName = assignName("Lv1 Air S Mist Finer", "Lv1 j.SMF");
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.powerup = powerup_mistFiner;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "AirMistFinerCLv1");
	move.displayName = assignName("Lv2 Air S Mist Finer", "Lv2 j.SMF");
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.powerup = powerup_mistFiner;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "AirMistFinerCLv2");
	move.displayName = assignName("Lv3 Air S Mist Finer", "Lv3 j.SMF");
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.powerup = powerup_mistFiner;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "AirMistFinerCancel");
	move.displayName = assignName("Air Mist Finer Cancel", "j.MFC");
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "TreasureHunt");
	move.displayName = assignName("Treasure Hunt", "TH");
	move.displayNameSelector = displayNameSelector_treasureHunt;
	move.sectionSeparator = sectionSeparator_treasureHunt;
	move.isInVariableStartupSection = isInVariableStartupSection_treasureHunt;
	move.ignoreJumpInstalls = true;
	move.charge = charge_treasureHunt;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "StepTreasureHunt");
	move.displayName = assignName("Stance Dash Treasure Hunt", "SDTH");
	move.displayNameSelector = displayNameSelector_stepTreasureHunt;
	move.partOfStance = true;
	move.combineWithPreviousMove = true;
	move.usePlusSignInCombination = true;
	move.sectionSeparator = sectionSeparator_treasureHunt;
	move.isInVariableStartupSection = isInVariableStartupSection_treasureHunt;
	move.ignoreJumpInstalls = true;
	move.charge = charge_stepTreasureHunt;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "Coin", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("Glitter Is Gold", "Coin");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "Sinwaza");
	move.displayName = assignName("Zwei Hander", "Zwei");
	move.isInVariableStartupSection = isInVariableStartupSection_zweiLand;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "Sinwaza_Shot");
	move.displayName = assignName("Zwei Hander Attack", "Zwei K");
	move.canYrcProjectile = canYrcProjectile_sinwazaShot;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "Sinwaza_Air");
	move.displayName = assignName("Air Zwei Hander", "j.Z");
	move.canYrcProjectile = canYrcProjectile_sinwazaShot;
	move.isMove = true;
	addMove(move);
	
	rememberFramebarId(Sinwaza_Shot2_framebarId);

	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "Sinwaza_Shot2", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = Sinwaza_Shot2_framebarId;
	move.framebarName = assignName("Zwei Hander", "Zwei");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "Sinwaza_Shot2_Air", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = Sinwaza_Shot2_framebarId;
	move.framebarName = assignName("Zwei Hander", "Zwei");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "Orenona");
	move.displayName = assignName("That's My Name", "TMN");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "OrenonaBurst");
	move.displayName = assignName("Burst That's My Name", "Burst TMN");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JOHNNY, "OrenonaExe");
	move.displayName = assignName("That's My Name", "TMN");
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "NmlAtk3B");
	move.displayName = assignName("3K");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "IrukasanTsubureru_tama", true);
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	// this dolphin is created on 41236P/K/S/H. When ridden it disappears
	move = MoveInfo(CHARACTER_TYPE_MAY, "IrukasanRidingObject", true);
	move.sectionSeparatorProjectile = sectionSeparatorProjectile_dolphin;
	move.isDangerous = isDangerous_aboveGround;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("Dolphin");
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	rememberFramebarId(MayBall_framebarId);

	move = MoveInfo(CHARACTER_TYPE_MAY, "MayBallA", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = MayBall_framebarId;
	move.framebarName = assignName("Beach Ball", "Ball");
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "MayBallB", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = MayBall_framebarId;
	move.framebarName = assignName("Beach Ball", "Ball");
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "NmlAtk6A");
	move.displayName = assignName("6P");
	move.displayNameSelector = displayNameSelector_may6P;
	move.nameIncludesInputs = true;
	move.sectionSeparator = sectionSeparator_may6P;
	move.isInVariableStartupSection = isInVariableStartupSection_may6Por6H;
	move.powerup = powerup_may6P;
	move.charge = charge_may6P;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "NmlAtk6D");
	move.displayName = assignName("6H");
	move.displayNameSelector = displayNameSelector_may6H;
	move.nameIncludesInputs = true;
	move.sectionSeparator = sectionSeparator_may6H;
	move.isInVariableStartupSection = isInVariableStartupSection_may6Por6H;
	move.powerup = powerup_may6H;
	move.charge = charge_may6H;
	move.isMove = true;
	addMove(move);
	
	// May riding horizontal Dolphin
	move = MoveInfo(CHARACTER_TYPE_MAY, "IrukasanRidingAttackYokoA");
	move.displayName = assignName("Hop on Dolphin", "HoD");
	move.isRecoveryHasGatlings = isRecoveryHasGatlings_mayRideTheDolphin;
	move.isRecoveryCanAct = isRecoveryCanAct_mayRideTheDolphin;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "IrukasanRidingAttackYokoB");
	move.displayName = assignName("Hop on Dolphin", "HoD");
	move.isRecoveryHasGatlings = isRecoveryHasGatlings_mayRideTheDolphin;
	move.isRecoveryCanAct = isRecoveryCanAct_mayRideTheDolphin;
	move.isMove = true;
	addMove(move);
	
	// May riding vertical Dolphin
	move = MoveInfo(CHARACTER_TYPE_MAY, "IrukasanRidingAttackTateA");
	move.displayName = assignName("Hop on Dolphin", "HoD");
	move.isRecoveryHasGatlings = isRecoveryHasGatlings_mayRideTheDolphin;
	move.isRecoveryCanAct = isRecoveryCanAct_mayRideTheDolphin;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "IrukasanRidingAttackTateB");
	move.displayName = assignName("Hop on Dolphin", "HoD");
	move.isRecoveryHasGatlings = isRecoveryHasGatlings_mayRideTheDolphin;
	move.isRecoveryCanAct = isRecoveryCanAct_mayRideTheDolphin;
	move.isMove = true;
	addMove(move);
	
	// big whale attack
	move = MoveInfo(CHARACTER_TYPE_MAY, "Yamada", true);
	move.framebarName = assignName("Yamada");
	move.framebarId = generateFramebarId();
	move.isDangerous = isDangerous_not_hasHitNumButInactive;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "Goshogawara");
	move.displayName = assignName("Deluxe Goshogawara Bomber");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	// May spins and may do a suicide whale in the end. This is the suicide whale
	move = MoveInfo(CHARACTER_TYPE_MAY, "SK_Goshogawara", true);
	move.framebarName = assignName("Goshogawara");
	move.framebarId = generateFramebarId();
	move.isDangerous = isDangerous_not_hasHitNumButNoHitboxes;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "RakkoJump");
	move.displayName = assignName("Ball Jump");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "RakkoJump_F");
	move.displayName = assignName("Ball Jump Forward");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "RakkoJump_B");
	move.displayName = assignName("Ball Jump Back");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "IrukasanYokoD");
	move.displayName = assignName("H Mr. Dolphin Horizontal", "H Dolphin");
	move.forceLandingRecovery = true;
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "IrukasanYokoC");
	move.displayName = assignName("S Mr. Dolphin Horizontal", "S Dolphin");
	move.forceLandingRecovery = true;
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "IrukasanTateD");
	move.displayName = assignName("H Mr. Dolphin Vertical", "H Updolphin");
	move.forceLandingRecovery = true;
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "IrukasanTateC");
	move.displayName = assignName("S Mr. Dolphin Vertical", "S Updolphin");
	move.forceLandingRecovery = true;
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "RakkoBallB");
	move.displayName = assignName("K Don't Miss It", "K Ball");
	move.canYrcProjectile = canYrcProjectile_beachBall;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "RakkoBallA");
	move.displayName = assignName("P Don't Miss It", "P Ball");
	move.canYrcProjectile = canYrcProjectile_beachBall;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "OverHeadKiss");
	move.displayName = assignName("Overhead Kiss", "OHK");
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "OverHeadKissExe");
	move.displayName = assignName("Overhead Kiss", "OHK");
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "IrukasanTateBShoukan");
	move.displayName = assignName("H Applause for the Victim", "H-Hoop");
	move.canYrcProjectile = canYrcProjectile_dolphin;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "IrukasanYokoBShoukan");
	move.displayName = assignName("S Applause for the Victim", "S-Hoop");
	move.canYrcProjectile = canYrcProjectile_dolphin;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "IrukasanTateAShoukan");
	move.displayName = assignName("K Applause for the Victim", "K-Hoop");
	move.canYrcProjectile = canYrcProjectile_dolphin;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "IrukasanYokoAShoukan");
	move.displayName = assignName("P Applause for the Victim", "P-Hoop");
	move.canYrcProjectile = canYrcProjectile_dolphin;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "DivingAttack");
	move.displayName = assignName("Ensenga?", "Ensenga");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "Dadakko");
	move.displayName = assignName("Ultimate Whiner", "Whiner");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "Daisenpu");
	move.displayName = assignName("Ultimate Spinning Whirlwind", "USW");
	move.iKnowExactlyWhenTheRecoveryOfThisMoveIs = isRecovery_daisenpu;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "Yamada");
	move.displayName = assignName("Great Yamada Attack", "Yamada");
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_yamada;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MAY, "YamadaBurst");
	move.displayName = assignName("Burst Great Yamada Attack", "Burst Yamada");
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_yamada;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "HaritsukiLA_Hold");
	move.displayName = assignName("Left Wall Climb (Hold P)");
	move.caresAboutWall = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "HaritsukiLC_Hold");
	move.displayName = assignName("Left Wall Climb (Hold S)");
	move.caresAboutWall = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "HaritsukiLD_Hold");
	move.displayName = assignName("Left Wall Climb (Hold H)");
	move.caresAboutWall = true;
	move.isMove = true;
	addMove(move);
	
	// Chipp wall cling attach
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "HaritsukiL");
	move.displayName = assignName("Left Wall Climb");
	move.caresAboutWall = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "HaritsukiRA_Hold");
	move.displayName = assignName("Right Wall Climb (Hold P)");
	move.caresAboutWall = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "HaritsukiRC_Hold");
	move.displayName = assignName("Right Wall Climb (Hold S)");
	move.caresAboutWall = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "HaritsukiRD_Hold");
	move.displayName = assignName("Right Wall Climb (Hold H)");
	move.caresAboutWall = true;
	move.isMove = true;
	addMove(move);
	
	// Chipp wall cling attach
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "HaritsukiR");
	move.displayName = assignName("Right Wall Climb");
	move.caresAboutWall = true;
	move.isMove = true;
	addMove(move);
	
	// Chipp wall cling idle/moving up/down
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "HaritsukiKeep");
	move.displayName = assignName("Wall Climb");
	move.combineWithPreviousMove = true;
	move.isIdle = isIdle_enableWhiffCancels;
	move.canBlock = canBlock_default;
	move.canBeUnableToBlockIndefinitelyOrForVeryLongTime = true;
	move.caresAboutWall = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "SankakuTobiUpper");
	move.displayName = assignName("w.9");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "SankakuTobiDown");
	move.displayName = assignName("w.3/6");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "Kaijo");
	move.displayName = assignName("w.4");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "HaritsukiD");
	move.displayName = assignName("w.H");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "GenyouzanExe");
	move.displayName = assignName("w.H");
	move.combineWithPreviousMove = true;
	move.conditionForAddingWhiffCancels = hasWhiffCancels;
	move.addForceAddWhiffCancel("HaritsukiLD_Hold");
	move.addForceAddWhiffCancel("HaritsukiRD_Hold");
	move.isGrab = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "HaritsukiC");
	move.displayName = assignName("w.S");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "HaritsukiB");
	move.displayName = assignName("w.K");
	move.caresAboutWall = true;
	move.canYrcProjectile = canYrcProjectile_wallclingKunai;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "HaritsukiA");
	move.displayName = assignName("w.A");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "AlphaPlus");
	move.displayName = assignName("Alpha Plus");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "WarpA");
	move.displayName = assignName("P Tsuyoshi-shiki Ten'i", "22P");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "WarpB");
	move.displayName = assignName("K Tsuyoshi-shiki Ten'i", "22K");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "WarpC");
	move.displayName = assignName("S Tsuyoshi-shiki Ten'i", "22S");
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = hasWhiffCancels;
	move.conditionForAddingWhiffCancels = hasWhiffCancels;
	move.addForceAddWhiffCancel("HaritsukiLC_Hold");
	move.addForceAddWhiffCancel("HaritsukiRC_Hold");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "WarpD");
	move.displayName = assignName("H Tsuyoshi-shiki Ten'i", "22H");
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = hasWhiffCancels;
	move.conditionForAddingWhiffCancels = hasWhiffCancels;
	move.addForceAddWhiffCancel("HaritsukiLD_Hold");
	move.addForceAddWhiffCancel("HaritsukiRD_Hold");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "AlphaBlade");
	move.displayName = assignName("Alpha Blade", "Alpha");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "AirAlphaBlade");
	move.displayName = assignName("Air Alpha Blade", "Air Alpha");
	move.conditionForAddingWhiffCancels = hasWhiffCancels;
	move.addForceAddWhiffCancel("HaritsukiLA_Hold");
	move.addForceAddWhiffCancel("HaritsukiRA_Hold");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "Sushi");
	move.displayName = assignName("Resshou");
	move.ignoreSuperJumpInstalls = true;  // this move only cancels into Rokusai, Senshuu and Shinkirou, and of those only two can make you airborne, and they both give a free airdash
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "Sukiyaki");
	move.displayName = assignName("Rokusai");
	move.ignoreSuperJumpInstalls = true;  // only cancels into Senshuu and Shinkirou, both of which give a free airdash
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "Banzai");
	move.displayName = assignName("Senshuu");
	move.ignoreSuperJumpInstalls = true;  // gives an airdash
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "GammaBladeObj", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("Gamma Blade", "Gamma");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "Shuriken");
	move.displayName = assignName("Shuriken");
	move.canYrcProjectile = canYrcProjectile_shuriken;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "Meisai");
	move.displayName = assignName("Tsuyoshi-shiki Meisai", "Meisai");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "Tobiagari");
	move.displayName = assignName("Shinkirou");
	move.conditionForAddingWhiffCancels = hasWhiffCancels;
	move.addForceAddWhiffCancel("HaritsukiLD_Hold");
	move.addForceAddWhiffCancel("HaritsukiRD_Hold");
	move.ignoreSuperJumpInstalls = true;  // gets an airdash
	// after active frames end, this move recovers double jumps as well, but you do care about jump installs if you want to obtain them sooner
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "BetaBlade");
	move.displayName = assignName("Beta Blade", "Beta");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "AirBetaBlade");
	move.displayName = assignName("Air Beta Blade", "Air Beta");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "Genrouzan");
	move.displayName = assignName("Genrou Zan");
	move.conditionForAddingWhiffCancels = hasWhiffCancels;
	move.addForceAddWhiffCancel("HaritsukiLC_Hold");
	move.addForceAddWhiffCancel("HaritsukiRC_Hold");
	move.ignoreSuperJumpInstalls = true;  // gives an airdash
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "GenrouzanExe");
	move.displayName = assignName("Genrou Zan");
	move.combineWithPreviousMove = true;
	move.conditionForAddingWhiffCancels = hasWhiffCancels;
	move.addForceAddWhiffCancel("HaritsukiLC_Hold");
	move.addForceAddWhiffCancel("HaritsukiRC_Hold");
	move.isGrab = true;
	move.ignoreSuperJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "GammaBlade");
	move.displayName = assignName("Gamma Blade", "Gamma");
	move.canYrcProjectile = canYrcProjectile_gammaBlade;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "6wayKunai");
	move.displayName = assignName("Ryuu Yanagi", "Ryuu");
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_ryuuYanagi;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "BankiMessai");
	move.displayName = assignName("Banki Messai", "Banki");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "ZanseiRouga");
	move.displayName = assignName("Zansei Rouga", "Zansei");
	move.forceLandingRecovery = true;
	move.forceSuperHitAnyway = forceSuperHitAnyway_zanseiRouga;
	move.iKnowExactlyWhenTheRecoveryOfThisMoveIs = isRecovery_zanseiRouga;
	move.ignoreSuperJumpInstalls = true;  // gets an airdash
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "ZanseiRougaBurst");
	move.displayName = assignName("Burst Zansei Rouga", "Burst Zansei");
	move.forceLandingRecovery = true;
	move.ignoreSuperJumpInstalls = true;  // gets an airdash
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "ShurikenObj", true);
	move.isDangerous = isDangerous_notNull;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("Shuriken Slow", "Shuriken-");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "ShurikenObj1", true);
	move.isDangerous = isDangerous_notNull;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("Shuriken Fast", "Shuriken+");
	addMove(move);
	
	// throwing daggers from wall cling
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "Kunai_Wall", true);
	move.isDangerous = isDangerous_notNull;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("Kunai");
	addMove(move);
	
	// 214214K air super
	move = MoveInfo(CHARACTER_TYPE_CHIPP, "Kunai", true);
	move.isDangerous = isDangerous_notNull;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("Ryuu Yanagi", "Ryuu");
	move.combineHitsFromDifferentProjectiles = true;
	addMove(move);
	
	// All specials that can put Faust into the air from the ground already give him an airdash by default, without having
	// to super jump install. As such, Faust never ever cares about super jump installs.
	charDoesNotCareAboutSuperJumpInstalls[CHARACTER_TYPE_FAUST] = true;
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "NmlAtk5E");
	move.displayName = assignName("5D");
	move.nameIncludesInputs = true;
	move.canYrcProjectile = canYrcProjectile_faust5D;
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "CrouchFWalk");
	move.displayName = assignName("Crouchwalk Forward", "3");
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "CrouchBWalk");
	move.displayName = assignName("Crouchwalk Back", "1");
	move.nameIncludesInputs = true;
	addMove(move);
	
	// Faust Pogo
	// Pogo entry
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Souten");
	move.displayName = assignName("Spear Point Centripetal Dance", "Pogo");
	move.displayNameSelector = displayNameSelector_pogoEntry;
	move.isIdle = isIdle_enableWhiffCancels;
	move.canBlock = canBlock_default;
	move.canBeUnableToBlockIndefinitelyOrForVeryLongTime = true;
	move.faustPogo = true;
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "SoutenCancel");
	move.displayName = assignName("Spear Point Cenripetal Dance Cancel", "Pogo Cancel");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "MettaGiri");
	move.displayName = assignName("Hack 'n Slash", "214H");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "NanigaDerukana");
	move.displayName = assignName("What Could This Be?", "Toss");
	move.displayNameSelector = displayNameSelector_itemToss;
	move.canYrcProjectile = canYrcProjectile_itemToss;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Oissu");
	move.displayName = assignName("Hello!", "236P");
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "KoegaChiisai");
	move.displayName = assignName("Can't Hear You!", "236P");
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = hasWhiffCancels;
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "MouicchoOissu");
	move.displayName = assignName("Hello Again!", "236P");
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "UekaraIkimasuyo");
	move.displayName = assignName("From Above", "S-Door");
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "UshirokaraIkimasuyo");
	move.displayName = assignName("From Behind", "K-Door");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "MaekaraIkimasuyo");
	move.displayName = assignName("From the Front", "P-Door");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "AirGoingMyWay");
	move.displayName = assignName("Air Going My Way");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Ai");
	move.displayName = assignName("Love");
	move.canYrcProjectile = canYrcProjectile_love;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "RerereNoTsuki");
	move.displayName = assignName("Re-re-re Thrust", "Scalpel");
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "NaNaNaNanigaDerukana");
	move.displayName = assignName("W-W-What Could This Be?", "Super Toss");
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_superToss;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "SugoiNaNaNaNanigaDerukana");
	move.displayName = assignName("W-W-What Could This Be? 100% Ver.", "Max Super Toss");
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_superToss;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Shigeki");
	move.displayName = assignName("Stimulating Fists of Annihilation", "Fists");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "ShigekiBurst");
	move.displayName = assignName("Burst Stimulating Fists of Annihilation", "Burst Fists");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	// Pogo P
	move = MoveInfo(CHARACTER_TYPE_FAUST, "SoutenA");
	move.displayName = assignName("Just A Taste!", "Pogo-P");
	move.displayNameSelector = displayNameSelector_pogoA;
	move.isIdle = isIdle_enableWhiffCancels;
	move.canBlock = canBlock_default;
	move.canBeUnableToBlockIndefinitelyOrForVeryLongTime = true;
	move.faustPogo = true;
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	// Pogo hop
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Souten9");
	move.displayName = assignName("Short Hop", "Pogo-9");
	move.displayNameSelector = displayNameSelector_pogo9;
	move.isIdle = isIdle_enableWhiffCancels;
	move.canBeUnableToBlockIndefinitelyOrForVeryLongTime = true;
	move.faustPogo = true;
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	// Pogo 44
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Souten44");
	move.displayName = assignName("Backward Movement", "Pogo-44");
	move.displayNameSelector = displayNameSelector_pogo44;
	move.isIdle = isIdle_enableWhiffCancels;
	move.canBlock = canBlock_default;
	move.canBeUnableToBlockIndefinitelyOrForVeryLongTime = true;
	move.faustPogo = true;
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	// Pogo 66
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Souten66");
	move.displayName = assignName("Forward Movement", "Pogo-66");
	move.displayNameSelector = displayNameSelector_pogo66;
	move.isIdle = isIdle_enableWhiffCancels;
	move.canBlock = canBlock_default;
	move.canBeUnableToBlockIndefinitelyOrForVeryLongTime = true;
	move.faustPogo = true;
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	// Pogo K (head flower)
	move = MoveInfo(CHARACTER_TYPE_FAUST, "SoutenB");
	move.displayName = assignName("Growing Flower", "Pogo-K");
	move.displayNameSelector = displayNameSelector_pogoB;
	move.sectionSeparator = sectionSeparator_soutenBC;
	move.isIdle = isIdle_enableWhiffCancels;
	move.canBlock = canBlock_default;
	move.canBeUnableToBlockIndefinitelyOrForVeryLongTime = true;
	move.isInVariableStartupSection = isInVariableStartupSection_soutenBC;
	move.faustPogo = true;
	move.ignoreSuperJumpInstalls = true;
	move.charge = charge_soutenBC;
	move.isMove = true;
	addMove(move);
	
	// Pogo S (ground flower)
	move = MoveInfo(CHARACTER_TYPE_FAUST, "SoutenC");
	move.displayName = assignName("See? I'm a Flower!", "Pogo-S");
	move.displayNameSelector = displayNameSelector_pogoC;
	move.sectionSeparator = sectionSeparator_soutenBC;
	move.isIdle = isIdle_enableWhiffCancels;
	move.canBlock = canBlock_default;
	move.canBeUnableToBlockIndefinitelyOrForVeryLongTime = true;
	move.isInVariableStartupSection = isInVariableStartupSection_soutenBC;
	move.faustPogo = true;
	move.canYrcProjectile = canYrcProjectile_flower;
	move.ignoreSuperJumpInstalls = true;
	move.charge = charge_soutenBC;
	move.isMove = true;
	addMove(move);
	
	// Pogo Going My Way
	move = MoveInfo(CHARACTER_TYPE_FAUST, "SoutenD");
	move.displayName = assignName("Spear Point Centripetal Dance Going My Way", "Pogo-H");
	move.displayNameSelector = displayNameSelector_pogoD;
	move.isIdle = isIdle_enableWhiffCancels;
	move.canBlock = canBlock_default;
	move.canBeUnableToBlockIndefinitelyOrForVeryLongTime = true;
	move.faustPogo = true;
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "SoutenE");
	move.displayName = assignName("Spear Point Centripetal Dance What Could This Be?", "Pogo-D");
	move.displayNameSelector = displayNameSelector_pogoE;
	move.isIdle = isIdle_enableWhiffCancels;
	move.canBlock = canBlock_default;
	move.canBeUnableToBlockIndefinitelyOrForVeryLongTime = true;
	move.faustPogo = true;
	move.canYrcProjectile = canYrcProjectile_pogoItemToss;
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	// Faust Pogo Helicopter
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Souten8");
	move.displayName = assignName("Doctor-Copter", "Pogo-8");
	move.displayNameSelector = displayNameSelector_pogo8;
	move.isIdle = hasWhiffCancels;
	move.canBlock = canBlock_default;
	move.canBeUnableToBlockIndefinitelyOrForVeryLongTime = true;
	move.faustPogo = true;
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	// Faust 41236K (long ass fishing pole poke that drags you) succeeeding
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Hikimodoshi");
	move.displayName = assignName("Pull Back");
	move.combineWithPreviousMove = true;
	move.splitForComboRecipe = true;
	move.butForFramebarDontCombineWithPreviousMove = true;
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	rememberFramebarId(OreHana_Shot_framebarId);

	// ground flower. The head flower cannot be RC'd. This is not the head flower. This flower can be RC'd, but not in Rev1.
	move = MoveInfo(CHARACTER_TYPE_FAUST, "OreHana_Shot", true);
	move.isDangerous = isDangerous_playerInRCOrHasActiveFlag_AndNotInRecovery;
	move.framebarId = OreHana_Shot_framebarId;
	move.framebarName = assignName("Flower");
	addMove(move);
	
	// ground flower maximum. Not present in Rev1
	move = MoveInfo(CHARACTER_TYPE_FAUST, "OreHanaBig_Shot", true);
	move.isDangerous = isDangerous_playerInRCOrHasActiveFlag_AndNotInRecovery;
	move.framebarId = OreHana_Shot_framebarId;
	move.framebarName = assignName("Flower");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Item_Oilcan", true);
	move.drawProjectileOriginPoint = true;
	move.framebarNameUncombined = assignName("Oilcan Item");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Item_Bomb", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("Bomb");
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Item_BlackHole", true);
	move.drawProjectileOriginPoint = true;
	move.framebarNameUncombined = assignName("Black Hole Item");
	addMove(move);
	
	rememberFramebarId(oil_framebarId);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "OilWater", true);
	move.framebarId = oil_framebarId;
	move.framebarName = assignName("Oil Patch");
	addMove(move);
	
	// fire created when setting oil on fire
	move = MoveInfo(CHARACTER_TYPE_FAUST, "OilFire", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = oil_framebarId;
	move.framebarName = assignName("Oil Fire");
	addMove(move);
	
	rememberFramebarId(Meteo_framebarId);

	// normal meteor. Does not have active frames. Creates several MeteoInseki which have active frames
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Item_Meteo", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = Meteo_framebarId;
	move.framebarName = assignName("Meteor Item");
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "MeteoInseki", true);
	move.isDangerous = isDangerous_notNull;
	move.framebarId = Meteo_framebarId;
	move.framebarName = assignName("Meteor");
	move.combineHitsFromDifferentProjectiles = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Item_Helium", true);
	move.drawProjectileOriginPoint = true;
	move.framebarNameUncombined = assignName("Helium Item");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Item_Hammer", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("Hammer");
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Item_ChibiFaust", true);
	move.isDangerous = isDangerous_not_hasHitNumButNoHitboxes;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("Small Faust", "Mini Faust");
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	rememberFramebarId(Poison_framebarId);

	move = MoveInfo(CHARACTER_TYPE_FAUST, "Item_Frasco", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = Poison_framebarId;
	move.framebarName = assignName("Poison");
	move.framebarNameUncombined = assignName("Poison Item");
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Item_Chocolate", true);
	move.drawProjectileOriginPoint = true;
	move.framebarNameUncombined = assignName("Chocolate");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Item_BestChocolate", true);
	move.drawProjectileOriginPoint = true;
	move.framebarNameUncombined = assignName("Valentine's Chocolate");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Item_Donut", true);
	move.drawProjectileOriginPoint = true;
	move.framebarNameUncombined = assignName("Donut");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Item_ManyDonut", true);
	move.drawProjectileOriginPoint = true;
	move.framebarNameUncombined = assignName("Box of Donuts");
	addMove(move);
	
	// the poison cloud created when poison flask lands
	move = MoveInfo(CHARACTER_TYPE_FAUST, "SubItem_Poison", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = Poison_framebarId;
	move.framebarName = assignName("Poison");
	move.framebarNameUncombined = assignName("Poison Cloud");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Item_JumpStand", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("Platform");
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Item_100t", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("100-ton Weight", "Weight");
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Item_FireWorks", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("Fireworks");
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	rememberFramebarId(Armageddon_framebarId);

	move = MoveInfo(CHARACTER_TYPE_FAUST, "Item_Armageddon", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = Armageddon_framebarId;
	move.framebarName = assignName("Massive Meteor");
	move.framebarNameUncombined = assignName("Massive Meteor Item");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "ArmageddonInseki", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = Armageddon_framebarId;
	move.framebarName = assignName("Massive Meteor", "Big Meteor");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Item_GoldenHammer", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("Golden Hammer", "Gold Hammer");
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Item_BigFaust", true);
	move.isDangerous = isDangerous_not_hasHitNumButNoHitboxes;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("Huge Faust");
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Item_Golden100t", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("10,000 Ton Weight", "Gold Weight");
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	rememberFramebarId(Ai_Bomb_framebarId);

	// the initial projectile Faust drops
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Ai_Bomb", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = Ai_Bomb_framebarId;
	move.framebarName = assignName("Love");
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	// the explosion created when Love touches the ground
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Ai_Bomb2", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = Ai_Bomb_framebarId;
	move.framebarName = assignName("Love");
	move.framebarNameUncombined = assignName("Love Explosion");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "ShigekiJibakuObj", true);
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("Stimulating Fists of Annihilation Self-Destruct", "Self-Destuct");
	addMove(move);
	
	rememberFramebarId(baseball_framebarId);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Shot_Hit", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.drawProjectileOriginPoint = true;
	move.framebarId = baseball_framebarId;
	move.framebarName = assignName("Baseball");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_FAUST, "Shot_HomeRun", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.drawProjectileOriginPoint = true;
	move.framebarId = baseball_framebarId;
	move.framebarName = assignName("Homerun Baseball");
	addMove(move);
	
	// Axl has only one move that can put him airborne from the ground, and that is Raiei, but it already gives an airdash by default.
	// Therefore, Axl has no use for super jump installs.
	charDoesNotCareAboutSuperJumpInstalls[CHARACTER_TYPE_AXL] = true;
	
	move = MoveInfo(CHARACTER_TYPE_AXL, "NmlAtk5CNearHasei");
	move.displayName = assignName("c.S");
	move.nameIncludesInputs = true;
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	// Axl Haitaka stance
	move = MoveInfo(CHARACTER_TYPE_AXL, "DaiRensen");
	move.displayName = assignName("Sparrowhawk Stance", "Haitaka");
	move.isIdle = isIdle_sparrowhawkStance;
	move.canBlock = canBlock_default;
	move.canBeUnableToBlockIndefinitelyOrForVeryLongTime = true;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = hasWhiffCancels;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_AXL, "DaiRensenA");
	move.displayName = assignName("Sparrowhawk Stance P", "Haitaka-P");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);

	move = MoveInfo(CHARACTER_TYPE_AXL, "DaiRensenB");
	move.displayName = assignName("Sparrowhawk Stance K", "Haitaka-K");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_AXL, "DaiRensenC");
	move.displayName = assignName("Sparrowhawk Stance S", "Haitaka-S");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_AXL, "DaiRensenD");
	move.displayName = assignName("Sparrowhawk Stance H", "Haitaka-H");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_AXL, "AxelBomber");
	move.displayName = assignName("Axl Bomber");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_AXL, "TenhousekiJou");
	move.displayName = assignName("P Heaven Can Wait", "P-Parry");
	move.ignoreJumpInstalls = true;  // can only RC on the ground
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_AXL, "TenhousekiGe");
	move.displayName = assignName("K Heaven Can Wait", "K-Parry");
	move.ignoreJumpInstalls = true;  // can only RC on the ground
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_AXL, "BentenGari");
	move.displayName = assignName("Artemis Hunter", "Benten");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_AXL, "Raieisageki");
	move.displayName = assignName("Thunder Shadow Chain", "Raiei");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_AXL, "KairagiYakou");
	move.displayName = assignName("Shark Strike", "Kairagi");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_AXL, "ByakueRenshou");
	move.displayName = assignName("Sickle Storm");
	move.iKnowExactlyWhenTheRecoveryOfThisMoveIs = isRecovery_byakueRenshou;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_AXL, "ByakueRenshouBurst");
	move.displayName = assignName("Burst Sickle Storm");
	move.iKnowExactlyWhenTheRecoveryOfThisMoveIs = isRecovery_byakueRenshou;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	// Axl Rensen
	move = MoveInfo(CHARACTER_TYPE_AXL, "Rensengeki");
	move.displayName = assignName("Sickle Flash", "Rensen");
	move.canYrcProjectile = canYrcProjectile_sickleFlash;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	// Axl Rensen + 2 followup
	move = MoveInfo(CHARACTER_TYPE_AXL, "Sensageki");
	move.displayName = assignName("Spinning Chain Strike", "Rensen-2");
	move.combineWithPreviousMove = true;
	move.splitForComboRecipe = true;
	move.butForFramebarDontCombineWithPreviousMove = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	// Axl Rensen + 8 followup
	move = MoveInfo(CHARACTER_TYPE_AXL, "Kyokusageki");
	move.displayName = assignName("Melody Chain", "Rensen-8");
	move.combineWithPreviousMove = true;
	move.splitForComboRecipe = true;
	move.butForFramebarDontCombineWithPreviousMove = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	// the command grab
	move = MoveInfo(CHARACTER_TYPE_AXL, "Rashosen");
	move.displayName = assignName("Spindle Spinner", "Rashousen");
	move.isGrab = true;
	move.ignoreJumpInstalls = true;  // because you can only RC it on the ground
	move.isMove = true;
	addMove(move);
	
	// the command grab
	move = MoveInfo(CHARACTER_TYPE_AXL, "RashosenObj", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("Spindle Spinner", "Rashousen");
	move.isMove = true;
	addMove(move);
	
	rememberFramebarId(RensengekiObj_framebarId);

	move = MoveInfo(CHARACTER_TYPE_AXL, "RensengekiObj", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = RensengekiObj_framebarId;
	move.framebarName = assignName("Sickle Flash", "Rensen");
	addMove(move);
	
	// the 8 followup
	move = MoveInfo(CHARACTER_TYPE_AXL, "KyokusagekiObj", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = RensengekiObj_framebarId;
	move.framebarName = assignName("Melody Chain", "Rensen-8");
	addMove(move);
	
	// the 2363214H super second hit
	move = MoveInfo(CHARACTER_TYPE_AXL, "ByakueObj", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("Sickle Storm");
	addMove(move);
	
	// the only move that Elphelt has that can transfer her from the ground into the air is Judge Better Half,
	// and you get a free airdash on it anyway.
	// And for moves that are jump cancellable, all she can do is jump from them, and get one airdash and one double jump by default,
	// without requiring any installs.
	// So a super jump install is, mathematically proven, useless to her
	charDoesNotCareAboutSuperJumpInstalls[CHARACTER_TYPE_ELPHELT] = true;

	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "NmlAtkAir5E");
	move.displayName = assignName("j.D");
	move.nameIncludesInputs = true;
	move.createdProjectile = createdProjectile_elpheltjD;
	move.canYrcProjectile = canYrcProjectile_elpheltjD;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "CmnActStand");
	move.displayName = assignName("Stand");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	move.charge = charge_elpheltStand;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "CmnActCrouch2Stand");
	move.displayName = assignName("Crouch to Stand");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	move.charge = charge_elpheltCrouch2Stand;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "CmnActJumpLanding");
	move.displayName = assignName("Landing");
	move.ignoreJumpInstalls = true;
	move.charge = charge_elpheltLanding;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "CmnActFDashStop");
	move.displayName = assignName("Forward Dash Stop");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	move.charge = charge_elpheltDashStop;
	addMove(move);
	
	// Elphelt Ms. Confille (rifle)
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Rifle_Start");
	move.displayName = assignName("Aim Ms. Confille", "Rifle");
	move.displayNameSelector = displayNameSelector_rifleStart;
	move.sectionSeparator = sectionSeparator_rifle;
	move.isIdle = isIdle_Rifle;
	move.canBlock = canBlock_default;
	move.considerNewSectionAsBeingInElpheltRifleStateBeforeBeingAbleToShoot = true;
	move.canBeUnableToBlockIndefinitelyOrForVeryLongTime = true;
	move.powerup = powerup_rifle;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Rifle_Reload");
	move.displayName = assignName("Ms. Confille Reload", "Reload");
	move.displayNameSelector = displayNameSelector_rifleReload;
	move.sectionSeparator = sectionSeparator_rifle;
	move.isIdle = isIdle_Rifle;
	move.canBlock = canBlock_default;
	move.considerNewSectionAsBeingInElpheltRifleStateBeforeBeingAbleToShoot = true;
	move.canBeUnableToBlockIndefinitelyOrForVeryLongTime = true;
	move.powerup = powerup_rifle;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Rifle_Reload_Perfect");
	move.displayName = assignName("Ms. Confille Perfect Reload", "Perfect Reload");
	move.displayNameSelector = displayNameSelector_riflePerfectReload;
	move.sectionSeparator = sectionSeparator_rifle;
	move.isIdle = isIdle_Rifle;
	move.canBlock = canBlock_default;
	move.considerNewSectionAsBeingInElpheltRifleStateBeforeBeingAbleToShoot = true;
	move.canBeUnableToBlockIndefinitelyOrForVeryLongTime = true;
	move.replacementInputs = "46S. S must be either on the same frame as 6 or on the frame after";
	move.replacementBufferTime = 1;
	move.powerup = powerup_rifle;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	// Entered into from CmnActRomanCancel if its performed during rifle stance either after entering the stance or after firing or after reloading.
	// On f1 whiff cancels are not enabled yet, on f2 enabled
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Rifle_Roman");
	move.displayName = assignName("Ms. Confille Roman Cancel", "Rifle RC");
	move.displayNameSelector = displayNameSelector_rifleRC;
	move.sectionSeparator = sectionSeparator_rifle;
	move.isIdle = isIdle_Rifle;
	move.canBlock = canBlock_default;
	move.considerNewSectionAsBeingInElpheltRifleStateBeforeBeingAbleToShoot = true;
	move.canBeUnableToBlockIndefinitelyOrForVeryLongTime = true;
	move.powerup = powerup_rifle;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Rifle_Fire");
	move.displayName = assignName("Ms. Confille Fire", "Fire");
	move.isRecoveryCanReload = isRecoveryCanReload_rifle;
	move.canYrcProjectile = canYrcProjectile_rifleFire;
	move.powerup = powerup_rifle;
	move.ignoreJumpInstalls = true;
	move.charge = charge_rifleCharge;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Shotgun_Fire_MIN");
	move.displayName = assignName("sg.H");
	move.nameIncludesInputs = true;
	move.isRecoveryCanReload = isRecoveryHasGatlings_enableWhiffCancels;
	move.ignoreJumpInstalls = true;
	move.charge = charge_shotgunCharge;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Shotgun_Fire_MAX");
	move.displayName = assignName("Max Charge sg.H");
	move.nameIncludesInputs = true;
	move.isRecoveryCanReload = isRecoveryHasGatlings_enableWhiffCancels;
	move.ignoreJumpInstalls = true;
	move.charge = charge_shotgunCharge;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Shotgun_Reload");
	move.displayName = assignName("Ms. Travailler Reload", "Shotgun Reload");
	move.combineWithPreviousMove = true;
	move.splitForComboRecipe = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Shotgun_Upper");
	move.displayName = assignName("sg.S");
	move.nameIncludesInputs = true;
	move.ignoreSuperJumpInstalls = true;
	move.charge = charge_shotgunCharge;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Shotgun_Rolling");
	move.displayName = assignName("sg.K");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	move.charge = charge_shotgunCharge;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Shotgun_Koduki");
	move.displayName = assignName("sg.P");
	move.nameIncludesInputs = true;
	move.ignoreSuperJumpInstalls = true;
	move.charge = charge_shotgunCharge;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Grenade_Land_Throw_Upper");
	move.displayName = assignName("High Toss", "4Toss");
	move.canYrcProjectile = canYrcProjectile_grenadeToss;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Shotgun_Grenade_Throw_Upper");
	move.displayName = assignName("Ms. Travailler Stance High Toss", "4Toss");
	move.canYrcProjectile = canYrcProjectile_grenadeToss;
	move.ignoreJumpInstalls = true;
	move.charge = charge_shotgunCharge;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Grenade_Air_Throw");
	move.displayName = assignName("Air High Toss", "Air Toss");
	move.canYrcProjectile = canYrcProjectile_grenadeToss;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Grenade_Land_Throw_Down");
	move.displayName = assignName("Low Toss", "2Toss");
	move.canYrcProjectile = canYrcProjectile_grenadeToss;
	move.ignoreJumpInstalls = true;
	// shotgun charge doesn't get preserved
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Shotgun_Grenade_Throw_Down");
	move.displayName = assignName("Ms. Travailler Stance Low Toss", "2Toss");
	move.canYrcProjectile = canYrcProjectile_grenadeToss;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Shotgun_CQC");
	move.displayName = assignName("CQC");
	move.ignoreJumpInstalls = true;
	move.charge = charge_shotgunCharge;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Shotgun_CQCExe");
	move.displayName = assignName("CQC");
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Rifle_End");
	move.displayName = assignName("Ms. Confille Stance Exit", "Rifle Exit");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Shotgun_Ready");
	move.displayName = assignName("Aim Ms. Travailler", "Pull Shotgun");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Shotgun_End");
	move.displayName = assignName("Ms. Travailler Cancel", "Shotgun Exit");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Shotgun_Rolling2");
	move.displayName = assignName("Roll, and Aim with Miss Travailler", "Roll");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Grenade_Land_Standby");
	move.displayName = assignName("Berry Pine", "Pull");
	move.ignoreJumpInstalls = true;
	move.canYrcProjectile = canYrcProjectile_berryPull;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Grenade_Air_Standby");
	move.displayName = assignName("Air Berry Pine", "Air Pull");
	move.canYrcProjectile = canYrcProjectile_berryPull;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Shotgun_Grenade_Standby");
	move.displayName = assignName("Ms. Travailler Stance Berry Pine", "Pull");
	move.ignoreJumpInstalls = true;
	move.canYrcProjectile = canYrcProjectile_berryPull;
	move.charge = charge_shotgunCharge;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "BridalExpress_Land");
	move.displayName = assignName("Bridal Express", "Bridal");
	move.ignoreJumpInstalls = true;
	move.charge = charge_shotgunCharge;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "BridalExpress_Air");
	move.displayName = assignName("Air Bridal Express", "Air Bridal");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "CmnActRomanCancel");
	move.displayName = assignName("Roman Cancel", "RC");
	move.nameIncludesInputs = true;
	move.charge = charge_shotgunCharge;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Bazooka");
	move.displayName = assignName("Genoverse");
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_bazooka;
	move.ignoreJumpInstalls = true;
	move.charge = charge_shotgunCharge;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Judge_BetterHalf");
	move.displayName = assignName("Judge Better Half", "JBH");
	move.iKnowExactlyWhenTheRecoveryOfThisMoveIs = isRecovery_land;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Judge_BetterHalfBurst");
	move.displayName = assignName("Burst Judge Better Half", "Burst JBH");
	move.iKnowExactlyWhenTheRecoveryOfThisMoveIs = isRecovery_land;
	move.isMove = true;
	addMove(move);
	
	rememberFramebarId(GrenadeBomb_framebarId);
	
	// thrown grenade
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "GrenadeBomb", true);
	move.isDangerous = isDangerous_grenade;
	move.framebarId = GrenadeBomb_framebarId;
	move.framebarName = PROJECTILE_NAME_BERRY;
	move.framebarNameSelector = framebarNameSelector_grenadeBomb;
	move.drawProjectileOriginPoint = true;
	move.showMultipleHitsFromOneAttack = true;
	move.projectilePowerup = projectilePowerup_grenadeBomb;
	addMove(move);
	
	// held grenade
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "GrenadeBomb_Ready", true);
	move.isDangerous = isDangerous_grenade;
	move.framebarId = GrenadeBomb_framebarId;
	move.framebarName = assignName("Berry Pine", "Berry");
	move.framebarNameSelector = framebarNameSelector_grenadeBombReady;
	addMove(move);
	
	// This explosion results from the timer running out normally
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "GrenadeBomb_Explode", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = GrenadeBomb_framebarId;
	move.framebarName = assignName("Berry Explosion", "Explosion");
	addMove(move);
	
	// This explosion results from clashing with the opponent's projectiles
	// Or shooting it with 5H, j.D, Bazooka, Shotgun, Rifle
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "GrenadeBomb_Explode2", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = GrenadeBomb_framebarId;
	move.framebarName = assignName("Berry Pine", "Berry Explode");
	addMove(move);
	
	rememberFramebarId(elpheltJD_framebarId);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "HandGun_air_shot", true);
	move.isDangerous = isDangerous_playerInRCOrHasActiveFlag_AndNotInRecovery;
	move.framebarId = elpheltJD_framebarId;
	move.framebarName = assignName("j.D");
	move.framebarNameUncombined = assignName("j.D Projectile");
	// in Rev1 you can't YRC this
	addMove(move);
	
	rememberFramebarId(Shotgun_framebarId);

	// Max charge shotgun shot spawns two projectiles: Shotgun_max_1, Shotgun_max_2
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Shotgun_max_1", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = Shotgun_framebarId;
	move.framebarNameSelector = framebarNameSelector_closeShot;
	move.projectilePowerup = projectilePowerup_closeShot;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Shotgun_max_2", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = Shotgun_framebarId;
	move.framebarName = assignName("Max Far Shot");
	addMove(move);
	
	// Shotgun shot spawns two projectiles: Shotgun_min_1, Shotgun_min_2
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Shotgun_min_1", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = Shotgun_framebarId;
	move.framebarName = assignName("Close Shot");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Shotgun_min_2", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = Shotgun_framebarId;
	move.framebarName = assignName("Far Shot");
	addMove(move);
	
	rememberFramebarId(Bazooka_framebarId);

	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Bazooka_Fire", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = Bazooka_framebarId;
	move.framebarName = assignName("Genoverse");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Bazooka_Explosive", true);
	move.isDangerous = isDangerous_not_NullWhileActive;
	move.framebarId = Bazooka_framebarId;
	move.framebarName = assignName("Geno Explode");
	addMove(move);
	
	rememberFramebarId(Rifle_Fire_framebarId);

	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Rifle_Fire_MAX", true);
	move.isDangerous = isDangerous_not_NullWhileActive;
	move.framebarId = Rifle_Fire_framebarId;
	move.framebarName = assignName("Max Ms. Confille Shot", "Max Rifleshot");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ELPHELT, "Rifle_Fire_MIN", true);
	move.isDangerous = isDangerous_not_NullWhileActive;
	move.framebarId = Rifle_Fire_framebarId;
	move.framebarName = assignName("Ms. Confille Shot", "Rifleshot");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "NmlAtk2E");
	move.displayName = assignName("2D");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "NmlAtk6D");
	move.displayName = assignName("6H");
	move.displayNameSelector = displayNameSelector_leo6H;
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	// Leo backturn idle and also exiting backturn via 22
	move = MoveInfo(CHARACTER_TYPE_LEO, "Semuke");
	move.displayName = assignName("Brynhildr Stance", "Backturn");
	move.displayNameSelector = displayNameSelector_backturn;
	move.isIdle = isIdle_enableWhiffCancels;
	move.canBlock = canBlock_default;
	move.canBeUnableToBlockIndefinitelyOrForVeryLongTime = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "Semuke5A");
	move.displayName = assignName("bt.P");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "Semuke5B");
	move.displayName = assignName("bt.K");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "Semuke5C");
	move.displayName = assignName("bt.S");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "Semuke5D");
	move.displayName = assignName("bt.H");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "Semuke5E");
	move.displayName = assignName("bt.D");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "Tobidogu2");
	move.displayName = assignName("H Graviert W\xc3\xbcrde", "H Fireball");
	move.canYrcProjectile = canYrcProjectile_graviertWurde;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "Tobidogu1");
	move.displayName = assignName("S Graviert W\xc3\xbcrde", "S Fireball");
	move.canYrcProjectile = canYrcProjectile_graviertWurde;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "AntiAirAttack2");
	move.displayName = assignName("H Eisen Sturm", "H Eisen");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "AntiAirAttack1");
	move.displayName = assignName("S Eisen Sturm", "S Eisen");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "Tossin2");
	move.displayName = assignName("Kaltes Gest\xc3\xb6\x62\x65r Zweit", "Zweit");
	move.displayNameSelector = displayNameSelector_tossin2;
	move.powerup = powerup_zweit;
	move.dontShowPowerupGraphic = alwaysTrue;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "Tossin1");
	move.displayName = assignName("Kaltes Gest\xc3\xb6\x62\x65r Erst", "Rekka");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "SemukeCantGuard");
	move.displayName = assignName("Blitzschlag", "bt.214H");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "SemukeDageki");
	move.displayName = assignName("Kaltes Gest\xc3\xb6\x62\x65r Dritt", "Dritt");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "AirSpecial");
	move.displayName = assignName("Siegesparade");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "AirSpecialExe");
	move.displayName = assignName("Siegesparade");
	move.combineWithPreviousMove = true;
	move.isMove = true;
	move.isGrab = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "Tossin2_Hasei");
	move.displayName = assignName("Kaltes Gest\xc3\xb6\x62\x65r Zweit (Follow-up)", "> Zweit");
	move.displayNameSelector = displayNameSelector_tossin2Hasei;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "SemukeDageki_Hasei");
	move.displayName = assignName("Kaltes Gest\xc3\xb6\x62\x65r Dritt (Follow-up)", "> Dritt");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "Gorengeki");
	move.displayName = assignName("Leidenschaft Dirigent", "Leidenschaft");
	move.displayNameSelector = displayNameSelector_gorengeki;
	move.iKnowExactlyWhenTheRecoveryOfThisMoveIs = isRecovery_land;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "SemukeKakusei");
	move.displayName = assignName("Stahl Wirbel");
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_stahlWirbel;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "SemukeKakuseiBurst");
	move.displayName = assignName("Burst Stahl Wirbel");
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_stahlWirbel;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "SemukeFDashStep");
	move.displayName = assignName("Brynhildr Stance Forward Dash", "bt.66");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "SemukeBDashStep");
	move.displayName = assignName("Brynhildr Stance Backdash", "bt.44");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "NmlAtk5CFar");
	move.displayName = assignName("f.S");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "NmlAtk5CFar_Guard");
	move.displayName = assignName("f.S~P");
	move.nameIncludesInputs = true;
	move.sectionSeparator = sectionSeparator_leoGuardStance;
	move.canStopHolding = aSectionBeforeVariableStartup_leoParry;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "NmlAtk5D");
	move.displayName = assignName("5H");
	move.displayNameSelector = displayNameSelector_leo5H;
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "NmlAtk5D_Guard");
	move.displayName = assignName("5H~P");
	move.nameIncludesInputs = true;
	move.sectionSeparator = sectionSeparator_leoGuardStance;
	move.canStopHolding = aSectionBeforeVariableStartup_leoParry;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.isMove = true;
	addMove(move);
	
	rememberFramebarId(GraviertWurde_framebarId);

	move = MoveInfo(CHARACTER_TYPE_LEO, "Edgeyowai", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = GraviertWurde_framebarId;
	move.framebarName = assignName("Graviert W\xc3\xbcrde", "S Fireball");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "Edgetuyoi", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = GraviertWurde_framebarId;
	move.framebarName = assignName("Graviert W\xc3\xbcrde", "H Fireball");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_LEO, "SemukeKakusei_Obj", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("Stahl Wirbel");
	addMove(move);
	
	// Jam parry
	move = MoveInfo(CHARACTER_TYPE_JAM, "NeoHochihu");
	move.displayName = assignName("Hochifu");
	move.isIdle = canBlock_neoHochihu;
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.canFaultlessDefend = alwaysTrue;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	// Jam 236S
	move = MoveInfo(CHARACTER_TYPE_JAM, "Bakushuu");
	move.displayName = assignName("Bakushuu");
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isMove = true;
	addMove(move);
	
	// Jam 236S~H
	move = MoveInfo(CHARACTER_TYPE_JAM, "SenriShinshou");
	move.displayName = assignName("Senri Shinshou", "H Puffball");
	move.combineWithPreviousMove = true;
	move.splitForComboRecipe = true;
	move.usePlusSignInCombination = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "HaseiSenriShinshou");
	move.displayName = assignName("Senri Shinshou (Follow-up)", "H Puffball");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	// Jam 236S~S
	move = MoveInfo(CHARACTER_TYPE_JAM, "HyappoShinshou");
	move.displayName = assignName("Hyappo Shinshou", "Puffball");
	move.combineWithPreviousMove = true;
	move.splitForComboRecipe = true;
	move.usePlusSignInCombination = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	// Jam 236S~K
	move = MoveInfo(CHARACTER_TYPE_JAM, "Ashibarai");
	move.displayName = assignName("Hamonkyaku", "Splitkick");
	move.combineWithPreviousMove = true;
	move.splitForComboRecipe = true;
	move.usePlusSignInCombination = true;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.isMove = true;
	addMove(move);
	
	// Jam 236S~P
	move = MoveInfo(CHARACTER_TYPE_JAM, "Mawarikomi");
	move.displayName = assignName("Mawarikomi");
	move.combineWithPreviousMove = true;
	move.splitForComboRecipe = true;
	move.usePlusSignInCombination = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	// Jam 46P
	move = MoveInfo(CHARACTER_TYPE_JAM, "TuikaA");
	move.displayName = assignName("Zekkei", "46P");
	move.combineWithPreviousMove = true;
	move.splitForComboRecipe = true;
	move.usePlusSignInCombination = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "TuikaAA");
	move.displayName = assignName("Goushao", "46PP");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "TuikaAB");
	move.displayName = assignName("Dowanga", "46PK");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "Youeikyaku");
	move.displayName = assignName("j.2K");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "AsanagiB");
	move.displayName = assignName("K Asanagi no Kokyuu", "K-Card");
	move.displayNameSelector = displayNameSelector_asanagiB;
	move.powerup = powerup_cardK;
	move.canYrcProjectile = canYrcProjectile_card;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "AsanagiC");
	move.displayName = assignName("S Asanagi no Kokyuu", "S-Card");
	move.displayNameSelector = displayNameSelector_asanagiC;
	move.powerup = powerup_cardS;
	move.canYrcProjectile = canYrcProjectile_card;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "AsanagiD");
	move.displayName = assignName("H Asanagi no Kokyuu", "H-Card");
	move.displayNameSelector = displayNameSelector_asanagiD;
	move.powerup = powerup_cardH;
	move.canYrcProjectile = canYrcProjectile_card;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "RyujinLand");
	move.displayName = assignName("Ryuujin");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "RyujinAir");
	move.displayName = assignName("Air Ryuujin");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "RyujinHasei");
	move.displayName = assignName("Ryuujin");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "RyujinKyoukaLand");
	move.displayName = assignName("Carded Ryuujin");
	move.displayNameSelector = displayNameSelector_ryujinLv2or3;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "RyujinKyoukaAir");
	move.displayName = assignName("Carded Air Ryuujin");
	move.displayNameSelector = displayNameSelector_airRyujinLv2or3;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "RyujinKyoukaHasei");
	move.displayName = assignName("Carded Ryuujin");
	move.displayNameSelector = displayNameSelector_ryujinLv2or3;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "GekirinLand");
	move.displayName = assignName("Gekirin");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "GekirinAir");
	move.displayName = assignName("Air Gekirin");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "GekirinHasei");
	move.displayName = assignName("Gekirin");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "GekirinKyoukaLand");
	move.displayName = assignName("Carded Gekirin");
	move.displayNameSelector = displayNameSelector_gekirinLv2or3;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "GekirinKyoukaAir");
	move.displayName = assignName("Carded Air Gekirin");
	move.displayNameSelector = displayNameSelector_airGekirinLv2or3;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "GekirinKyoukaHasei");
	move.displayName = assignName("Carded Gekirin");
	move.displayNameSelector = displayNameSelector_gekirinLv2or3;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "KenroukakuLand");
	move.displayName = assignName("Kenroukaku");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "KenroukakuAir");
	move.displayName = assignName("Air Kenroukaku");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "KenroukakuHasei");
	move.displayName = assignName("Kenroukaku");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "KenroukakuKyoukaLand");
	move.displayName = assignName("Carded Kenroukaku");
	move.displayNameSelector = displayNameSelector_kenroukakuLv2or3;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "KenroukakuKyoukaAir");
	move.displayName = assignName("Carded Air Kenroukaku");
	move.displayNameSelector = displayNameSelector_airKenroukakuLv2or3;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "KenroukakuKyoukaHasei");
	move.displayName = assignName("Carded Kenroukaku");
	move.displayNameSelector = displayNameSelector_kenroukakuLv2or3;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "Renhoukyaku");
	move.displayName = assignName("Renhoukyaku", "Super Puffball");
	move.canYrcProjectile = canYrcProjectile_renhoukyaku;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "RenhoukyakuObj", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("Renhoukyaku", "Super Puffball");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "Hououshou");
	move.displayName = assignName("Choukyaku Hou'oushou", "Choukyaku");
	move.forceSuperHitAnyway = forceSuperHitAnyway_hououshou;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "HououshouBurst");
	move.displayName = assignName("Burst Choukyaku Hou'oushou", "Burst Choukyaku");
	move.forceSuperHitAnyway = forceSuperHitAnyway_hououshou;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JAM, "Saishingeki");
	move.displayName = assignName("Bao Saishinshou", "Bao");
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = isInVariableStartupSection_saishingeki;
	move.secondaryStartup = secondaryStartup_saishingeki;
	move.dontSkipSuper = true;
	move.showMultipleHitsFromOneAttack = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "NmlAtk6B");
	move.displayName = assignName("6K");
	move.displayNameSelector = displayNameSelector_answer6K;
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_Hold_End");
	move.displayName = assignName("Savvy Ninpo: Seal of Approval Cancel", "Uncling");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_Atemi");
	move.displayName = assignName("s.P");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_Stamp");
	move.displayName = assignName("s.K");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_Gedan");
	move.displayName = assignName("s.S");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_Chudan");
	move.displayName = assignName("s.H");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_Atemi2");
	move.displayName = assignName("Savvy Ninpo: Data Logging", "22P");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Meishi_WarpA");
	move.displayName = assignName("S Business Ninpo: Under the Rug", "S Teleport");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Meishi_WarpB");
	move.displayName = assignName("H Business Ninpo: Under the Rug", "H Teleport");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Zaneiken");
	move.displayName = assignName("Resshou");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Meishi_ThrowA");
	move.displayName = assignName("S Business Ninpo: Caltrops", "S Card");
	move.canYrcProjectile = canYrcProjectile_caltrops;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Meishi_ThrowB");
	move.displayName = assignName("H Business Ninpo: Caltrops", "H Card");
	move.canYrcProjectile = canYrcProjectile_caltrops;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_set_LandA");
	move.displayName = assignName("P Savvy Ninpo: Request for Approval", "P Scroll");
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.canYrcProjectile = canYrcProjectile_scroll;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_set_LandB");
	move.displayName = assignName("K Savvy Ninpo: Request for Approval", "K Scroll");
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.canYrcProjectile = canYrcProjectile_scroll;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_set_LandC");
	move.displayName = assignName("S Savvy Ninpo: Request for Approval", "S Scroll");
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.canYrcProjectile = canYrcProjectile_scroll;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_set_LandD");
	move.displayName = assignName("H Savvy Ninpo: Request for Approval", "H Scroll");
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.canYrcProjectile = canYrcProjectile_scroll;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_set_AirA");
	move.displayName = assignName("P Air Savvy Ninpo: Request for Approval", "Air P Scroll");
	move.canYrcProjectile = canYrcProjectile_scroll;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_set_AirB");
	move.displayName = assignName("K Air Savvy Ninpo: Request for Approval", "Air K Scroll");
	move.canYrcProjectile = canYrcProjectile_scroll;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_set_AirC");
	move.displayName = assignName("S Air Savvy Ninpo: Request for Approval", "Air S Scroll");
	move.canYrcProjectile = canYrcProjectile_scroll;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_set_AirD");
	move.displayName = assignName("H Air Savvy Ninpo: Request for Approval", "Air H Scroll");
	move.canYrcProjectile = canYrcProjectile_scroll;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Mozu_Land");
	move.displayName = assignName("Savvy Ninpo: Tax Write-off", "Izuna Drop");
	move.ignoreJumpInstalls = true;  // when you RC this move, you can only be on the ground
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Mozu_Land_Exe");
	move.displayName = assignName("Savvy Ninpo: Tax Write-off", "Izuna Drop");
	move.combineWithPreviousMove = true;
	move.ignoreJumpInstalls = true;  // when you RC this move, you can only be on the ground
	move.isGrab = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Mozu_Air");
	move.displayName = assignName("Air Savvy Ninpo: Tax Write-off", "Air Izuna Drop");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Mozu_Air_Exe");
	move.displayName = assignName("Air Savvy Ninpo: Tax Write-off", "Air Izuna Drop");
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Meishi_Nin_JitsuA");
	move.displayName = assignName("S Business Ninpo: Under the Bus", "S Clone");
	move.canYrcProjectile = canYrcProjectile_clone;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Meishi_Nin_JitsuB");
	move.displayName = assignName("H Business Ninpo: Under the Bus", "H Clone");
	move.canYrcProjectile = canYrcProjectile_clone;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Human_Suriken");
	move.displayName = assignName("Business Ultimate Ninpo: All Hands");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Human_SurikenExe");
	move.displayName = assignName("Business Ultimate Ninpo: All Hands");
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Meishi_Meteor");
	move.displayName = assignName("Air Dead Stock Ninpo: Firesale", "Air Firesale");
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_meishiMeteor;
	move.isMove = true;
	addMove(move);
	
	rememberFramebarId(Firesale_framebarId);

	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Meteor", true);
	move.framebarName = assignName("Air Dead Stock Ninpo: Firesale", "Air Firesale");
	move.framebarNameUncombined = assignName("Air Dead Stock Ninpo: Firesale Card", "Firesale Card");
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = Firesale_framebarId;
	move.combineHitsFromDifferentProjectiles = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Suriken", true);
	move.framebarName = assignName("Air Dead Stock Ninpo: Firesale", "Air Firesale");
	move.framebarNameUncombined = assignName("Air Dead Stock Ninpo: Firesale Shuriken", "Shuriken");
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = Firesale_framebarId;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Royal_Straight_Flush");
	move.displayName = assignName("Dead Stock Ninpo: Firesale", "Firesale");
	move.dontSkipSuper = true;
	move.createdProjectile = createdProjectile_firesale;
	move.canYrcProjectile = canYrcProjectile_firesale;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Royal_Straight_Flush_Burst");
	move.displayName = assignName("Burst Dead Stock Ninpo: Firesale", "Burst Firesale");
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_default;
	move.createdProjectile = createdProjectile_firesale;
	move.canYrcProjectile = canYrcProjectile_firesale;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	// Answer scroll cling idle. Happens from an s.D if not holding D
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_Move_Hold");
	move.displayName = assignName("Savvy Ninpo: Seal of Approval", "Scroll Cling");
	move.isIdle = isIdle_enableWhiffCancels;
	move.canBlock = canBlock_default;
	move.canBeUnableToBlockIndefinitelyOrForVeryLongTime = true;
	move.isMove = true;
	addMove(move);
	
	// Answer scroll cling idle. Happens from an s.D if not holding Special
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_Move_Hold_S");
	move.displayName = assignName("Savvy Ninpo: Seal of Approval", "Scroll Cling");
	move.isIdle = isIdle_enableWhiffCancels;
	move.canBlock = canBlock_default;
	move.canBeUnableToBlockIndefinitelyOrForVeryLongTime = true;
	move.isMove = true;
	addMove(move);
	
	// Answer scroll cling idle. Happens from jumping at it or 22P'ing at it
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_Hold");
	move.displayName = assignName("Savvy Ninpo: Seal of Approval", "Scroll Cling");
	move.isIdle = isIdle_enableWhiffCancels;
	move.canBlock = canBlock_default;
	move.canBeUnableToBlockIndefinitelyOrForVeryLongTime = true;
	move.isMove = true;
	addMove(move);
	
	// Answer 1sD
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_Move1");
	move.displayName = assignName("1s.D");
	move.nameIncludesInputs = true;
	move.isIdle = alwaysFalse;
	move.isInVariableStartupSection = isInVariableStartupSection_amiMove;
	move.considerVariableStartupAsStanceForFramebar = true;
	addMove(move);
	
	// Answer 2sD
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_Move2");
	move.displayName = assignName("2s.D");
	move.nameIncludesInputs = true;
	move.isIdle = alwaysFalse;
	move.isInVariableStartupSection = isInVariableStartupSection_amiMove;
	move.considerVariableStartupAsStanceForFramebar = true;
	addMove(move);
	
	// Answer 3sD
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_Move3");
	move.displayName = assignName("3s.D");
	move.nameIncludesInputs = true;
	move.isIdle = alwaysFalse;
	move.isInVariableStartupSection = isInVariableStartupSection_amiMove;
	move.considerVariableStartupAsStanceForFramebar = true;
	addMove(move);
	
	// Answer 4sD
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_Move4");
	move.displayName = assignName("4s.D");
	move.nameIncludesInputs = true;
	move.isIdle = alwaysFalse;
	move.isInVariableStartupSection = isInVariableStartupSection_amiMove;
	move.considerVariableStartupAsStanceForFramebar = true;
	addMove(move);
	
	// Answer sD
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_MoveD");
	move.displayName = assignName("s.D");
	move.nameIncludesInputs = true;
	move.isIdle = alwaysFalse;
	move.isInVariableStartupSection = isInVariableStartupSection_amiMove;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isMove = true;
	addMove(move);
	
	// Answer 6sD
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_Move6");
	move.displayName = assignName("6s.D");
	move.nameIncludesInputs = true;
	move.isIdle = alwaysFalse;
	move.isInVariableStartupSection = isInVariableStartupSection_amiMove;
	move.considerVariableStartupAsStanceForFramebar = true;
	addMove(move);
	
	// Answer 7sD
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_Move7");
	move.displayName = assignName("7s.D");
	move.nameIncludesInputs = true;
	move.isIdle = alwaysFalse;
	move.isInVariableStartupSection = isInVariableStartupSection_amiMove;
	move.considerVariableStartupAsStanceForFramebar = true;
	addMove(move);
	
	// Answer 8sD
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_Move8");
	move.displayName = assignName("8s.D");
	move.nameIncludesInputs = true;
	move.isIdle = alwaysFalse;
	move.isInVariableStartupSection = isInVariableStartupSection_amiMove;
	move.considerVariableStartupAsStanceForFramebar = true;
	addMove(move);
	
	// Answer 9sD
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_Move9");
	move.displayName = assignName("9s.D");
	move.nameIncludesInputs = true;
	move.isIdle = alwaysFalse;
	move.isInVariableStartupSection = isInVariableStartupSection_amiMove;
	move.considerVariableStartupAsStanceForFramebar = true;
	addMove(move);
	
	// Answer sD
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami_Move");
	move.displayName = assignName("s.D");
	move.nameIncludesInputs = true;
	move.isIdle = alwaysFalse;
	move.isInVariableStartupSection = isInVariableStartupSection_amiMove;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Ami", true);
	move.drawProjectileOriginPoint = true;
	move.framebarName = assignName("Scroll");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Meishi", true);
	move.isDangerous = isDangerous_card;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("BN: Caltrops", "Card");
	move.framebarNameFull = "Business Ninpo: Caltrops";
	move.framebarNameUncombined = assignName("Card");
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "Nin_Jitsu", true);
	move.isDangerous = isDangerous_not_NullWhileActive;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("BN: Under the Bus", "Clone");
	move.framebarNameUncombined = assignName("Clone");
	move.framebarNameFull = "Business Ninpo: Under the Bus";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "RSF_Start", true);
	move.isDangerous = isDangerous_card;
	move.framebarId = ANSWER_RSF_FRAMEBAR_ID;
	move.framebarName = assignName("Dead Stock Ninpo: Firesale", "Firesale");
	move.framebarNameUncombined = assignName("Dead Stock Ninpo: Firesale Start", "Firesale Start");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "RSF_Meishi", true);
	move.isDangerous = isDangerous_rsfMeishi;
	move.framebarId = ANSWER_RSF_FRAMEBAR_ID;
	move.framebarName = assignName("Dead Stock Ninpo: Firesale", "Firesale");
	move.framebarNameUncombined = assignName("Dead Stock Ninpo: Firesale Card", "Firesale Card");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ANSWER, "RSF_Finish", true);
	move.isDangerous = isDangerous_rsfMeishi;
	move.framebarId = ANSWER_RSF_FRAMEBAR_ID;
	move.framebarName = assignName("Dead Stock Ninpo: Firesale", "Firesale");
	move.framebarNameUncombined = assignName("Dead Stock Ninpo: Firesale Shuriken", "Shuriken");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "LustShakerRenda");
	move.displayName = assignName("Mash Lust Shaker", "Mash Shaker");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	// Millia Roll Roll
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "SaiZenten");
	move.displayName = assignName("Forward Roll Again", "Doubleroll");
	move.combineWithPreviousMove = true;
	move.splitForComboRecipe = true;
	move.usePlusSignInCombination = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	// Millia Roll > S
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "ZentenShaker");
	move.displayName = assignName("Lust Shaker (Follow-up)", "> Shaker");
	move.combineWithPreviousMove = true;
	move.splitForComboRecipe = true;
	move.usePlusSignInCombination = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	// Millia Roll > H
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "Digitalis");
	move.displayName = assignName("Digitalis");
	move.combineWithPreviousMove = true;
	move.splitForComboRecipe = true;
	move.usePlusSignInCombination = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	// represents both S and H pins
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "SilentForceKnife", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("Silent Force", "Pin");
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	// s-disc
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "TandemTopC");
	move.displayName = assignName("S Tandem Top", "S-Disc");
	move.canYrcProjectile = canYrcProjectile_disc;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	// s-disc
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "TandemTopCRing", true);
	move.isDangerous = isDangerous_notNull;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("S Tandem Top", "S-Disc");
	addMove(move);
	
	// h-disc
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "TandemTopD");
	move.displayName = assignName("H Tandem Top", "H-Disc");
	move.canYrcProjectile = canYrcProjectile_disc;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	// h-disc
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "TandemTopDRing", true);
	move.isDangerous = isDangerous_notNull;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("H Tandem Top", "H-Disc");
	addMove(move);
	
	// Bad Moon does not get a height buff in Rev1
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "BadMoon");
	move.displayName = assignName("Bad Moon", "BM");
	move.displayNameSelector = displayNameSelector_badMoon;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "SecretGarden");
	move.displayName = assignName("Secret Garden", "Garden");
	move.ignoreJumpInstalls = true;
	move.powerup = powerup_secretGarden;
	move.dontShowPowerupGraphic = alwaysTrue;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "LustShaker");
	move.displayName = assignName("Lust Shaker", "Shaker");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "Zenten");
	move.displayName = assignName("Forward Roll", "Roll");
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = hasWhiffCancels;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "IronSavior");
	move.displayName = assignName("Iron Savior", "Haircar");
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "SilentForce2");
	move.displayName = assignName("H Silent Force", "H-Pin");
	move.canYrcProjectile = canYrcProjectile_silentForce;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "SilentForce");
	move.displayName = assignName("S Silent Force", "S-Pin");
	move.canYrcProjectile = canYrcProjectile_silentForce;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "KousokuRakka");
	move.displayName = assignName("Turbo Fall", "TF");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "EmeraldRain");
	move.displayName = assignName("Emerald Rain", "ER");
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_emeraldRain;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "ChromingRose");
	move.displayName = assignName("Chroming Rose", "Rose Install");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "Winger");
	move.displayName = assignName("Winger");
	move.iKnowExactlyWhenTheRecoveryOfThisMoveIs = isRecovery_land;
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "WingerBurst");
	move.displayName = assignName("Burst Winger");
	move.iKnowExactlyWhenTheRecoveryOfThisMoveIs = isRecovery_land;
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	rememberFramebarId(EmeraldRain_framebarId);

	// each ring of the 236236S super is separately named
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "EmeraldRainRing1", true);
	move.isDangerous = isDangerous_notNull;
	move.framebarId = EmeraldRain_framebarId;
	move.framebarName = assignName("Emerald Rain", "ER");
	move.framebarNameUncombined = assignName("Emeral Rain Ring 1", "ER Ring1");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "EmeraldRainRing2", true);
	move.isDangerous = isDangerous_notNull;
	move.framebarId = EmeraldRain_framebarId;
	move.framebarName = assignName("Emerald Rain", "ER");
	move.framebarNameUncombined = assignName("Emeral Rain Ring 2", "ER Ring2");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "EmeraldRainRing3", true);
	move.isDangerous = isDangerous_notNull;
	move.framebarId = EmeraldRain_framebarId;
	move.framebarName = assignName("Emerald Rain", "ER");
	move.framebarNameUncombined = assignName("Emeral Rain Ring 3", "ER Ring3");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "SecretGardenBall", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("Secret Garden", "Garden");
	move.combineHitsFromDifferentProjectiles = true;
	addMove(move);
	
	// a rose created during Rose Install. Many of these can be on the screen at the same time
	move = MoveInfo(CHARACTER_TYPE_MILLIA, "RoseObj", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("Rose");
	addMove(move);
	
	// Zato does not have a move that makes him airborne from the ground, so he never cares about super jump installs.
	// And if some move only cancels into specials or only leads to other moves that only cancel into specials, then
	// he doesn't care about jump installs either (Zato has no such moves)
	charDoesNotCareAboutSuperJumpInstalls[CHARACTER_TYPE_ZATO] = true;
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "NmlAtk6B");
	move.displayName = assignName("6K");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;  // a dead end move
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "NmlAtk6D");
	move.displayName = assignName("6H");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;  // a dead end move
	move.isMove = true;
	addMove(move);
	
	rememberFramebarId(Eddie_framebarId);

	move = MoveInfo(CHARACTER_TYPE_ZATO, "EddieMegalithHead", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarName = assignName("Great White");
	move.framebarId = Eddie_framebarId;
	move.isEddie = true;
	move.showMultipleHitsFromOneAttack = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "ChouDoriru", true);
	move.isDangerous = isDangerous_notNull;
	move.framebarName = assignName("Giga Drill");
	move.framebarId = Eddie_framebarId;
	move.isEddie = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "KageDamari", true);
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "EddieA", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarName = assignName("Eddie P");
	move.framebarId = Eddie_framebarId;
	move.drawProjectileOriginPoint = true;
	move.isEddie = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "EddieB", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarName = assignName("Eddie K");
	move.framebarId = Eddie_framebarId;
	move.drawProjectileOriginPoint = true;
	move.isEddie = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "EddieC", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarName = assignName("Eddie S");
	move.framebarId = Eddie_framebarId;
	move.drawProjectileOriginPoint = true;
	move.isEddie = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "EddieD", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarName = assignName("Eddie H");
	move.framebarId = Eddie_framebarId;
	move.drawProjectileOriginPoint = true;
	move.isEddie = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "EddieE", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarName = assignName("Eddie D");
	move.framebarId = Eddie_framebarId;
	move.drawProjectileOriginPoint = true;
	move.isEddie = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "Eddie4", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarName = assignName("Eddie 4");
	move.framebarId = Eddie_framebarId;
	move.drawProjectileOriginPoint = true;
	move.isEddie = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "Eddie", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarName = assignName("Eddie");
	move.framebarId = Eddie_framebarId;
	move.drawProjectileOriginPoint = true;
	move.isEddie = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "Eddie6", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarName = assignName("Eddie 6");
	move.framebarId = Eddie_framebarId;
	move.drawProjectileOriginPoint = true;
	move.isEddie = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "Fly");
	move.displayName = assignName("Flight");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "InviteHellC");
	move.displayName = assignName("S Invite Hell", "S-Drill");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "InviteHellD");
	move.displayName = assignName("H Invite Hell", "H-Drill");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "EddieSummonD");
	move.displayName = assignName("Summon Eddie Shadow Dive", "Summon");
	move.canYrcProjectile = canYrcProjectile_eddie;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "EddieSummonC");
	move.displayName = assignName("Summon Eddie Anti-air Attack", "Nobiru");
	move.canYrcProjectile = canYrcProjectile_eddie;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "EddieSummonB");
	move.displayName = assignName("Summon Eddie Traversing Attack", "Mawaru");
	move.canYrcProjectile = canYrcProjectile_eddie;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "EddieSummonA");
	move.displayName = assignName("Summon Eddie Small Attack", "P Summon");
	move.canYrcProjectile = canYrcProjectile_eddie;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "EddieReturn");
	move.displayName = assignName("Recall Eddie", "Recall");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "EddieSummonD2");
	move.displayName = assignName("Shadow Puddle Eddie Summon", "Puddle Summon");
	move.canYrcProjectile = canYrcProjectile_eddie;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "DrunkerdShade");
	move.displayName = assignName("Drunkard Shade");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "DamnedFang");
	move.displayName = assignName("Damned Fang");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "DamnedFangExe");
	move.displayName = assignName("Damned Fang");
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "ShadowGallary");
	move.displayName = assignName("Shadow Gallery", "SG");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "Executer");
	move.displayName = assignName("Executor");
	move.iKnowExactlyWhenTheRecoveryOfThisMoveIs = isRecovery_land;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "Amorphous");
	move.displayName = assignName("Amorphous");
	move.canYrcProjectile = canYrcProjectile_amorphous;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "AmorphousBurst");
	move.displayName = assignName("Burst Amorphous");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "MegalithHead");
	move.displayName = assignName("Great White");
	move.dontSkipSuper = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "MegalithHead2");
	move.displayName = assignName("Great White");
	move.dontSkipSuper = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	// Zato 214K
	move = MoveInfo(CHARACTER_TYPE_ZATO, "BreakTheLaw");
	move.displayName = assignName("Break the Law");
	move.sectionSeparator = sectionSeparator_breakTheLaw;
	move.zatoHoldLevel = zatoHoldLevel_breakTheLaw;
	move.isInVariableStartupSection = isInVariableStartupSection_breakTheLaw;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	rememberFramebarId(Drill_framebarId);

	move = MoveInfo(CHARACTER_TYPE_ZATO, "DrillC", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = Drill_framebarId;
	move.framebarName = assignName("Invite Hell", "Drill");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_ZATO, "DrillD", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = Drill_framebarId;
	move.framebarName = assignName("Invite Hell", "Drill");
	addMove(move);
	
	rememberFramebarId(AmorphousObj_framebarId);

	move = MoveInfo(CHARACTER_TYPE_ZATO, "AmorphousObj", true);
	move.isDangerous = isDangerous_amorphous;
	move.framebarId = AmorphousObj_framebarId;
	move.framebarName = assignName("Amorphous");
	move.framebarNameUncombined = assignName("Amorphous", "Amorphous");
	addMove(move);
	
	// this can only be created on the boss version of Zato
	move = MoveInfo(CHARACTER_TYPE_ZATO, "AmorphousObj2", true);
	move.isDangerous = isDangerous_not_hasHitNumButNoHitboxes;
	move.framebarId = AmorphousObj_framebarId;
	move.framebarName = assignName("Amorphous");
	move.framebarNameUncombined = assignName("Amorphous Hit 2", "Amorphous2");
	addMove(move);
	
	// Potemkin does not care about super jump installs at all because he will never make use of the airdash that he gets,
	// but that is already handled in our code that registers the airdash install, because it won't trigger a super jump install,
	// if the airdash count is already equal to the maximum, which is 0, so we don't even need a = true here.
	charDoesNotCareAboutSuperJumpInstalls[CHARACTER_TYPE_POTEMKIN] = true;
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "NmlAtk2E");
	move.displayName = assignName("2D");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "SlideHead");
	move.displayName = assignName("Slide Head");
	move.canYrcProjectile = canYrcProjectile_slideHead;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "MegaFistFront");
	move.displayName = assignName("Forward Megafist");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "MegaFistBack");
	move.displayName = assignName("Back Megafist");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "HeatKnucle");
	move.displayName = assignName("Heat Knuckle");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "HammerFall");
	move.displayName = assignName("Hammer Fall");
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = hasWhiffCancels;
	move.partOfStance = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "HeatExtend");
	move.displayName = assignName("Heat Extend");
	move.isGrab = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "PotemkinBuster");
	move.displayName = assignName("Potemkin Buster", "Potbuster");
	move.frontLegInvul = frontLegInvul_potemkinBuster;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "PotemkinBusterExe");
	move.displayName = assignName("Potemkin Buster", "Potbuster");
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "Ichigeki");
	move.displayName = assignName("Instant Kill", "IK");
	move.frontLegInvul = frontLegInvul_potemkinBuster;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	// Potemkin Flick
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "FDB");
	move.displayName = assignName("F.D.B.");
	move.sectionSeparator = sectionSeparator_FDB;
	move.isInVariableStartupSection = isInVariableStartupSection_fdb;
	move.canYrcProjectile = canYrcProjectile_fdb;
	move.ignoreJumpInstalls = true;
	move.charge = charge_fdb;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "FDB_obj", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarName = assignName("F.D.B.");
	move.framebarId = generateFramebarId();
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "Anti_AirExplode");
	move.displayName = assignName("Trishula");
	move.canYrcProjectile = canYrcProjectile_trishula;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "RocketDive");
	move.displayName = assignName("I.C.P.M.");
	move.dontSkipGrab = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "RocketDiveExe");
	move.displayName = assignName("I.C.P.M.");
	move.combineWithPreviousMove = true;
	move.dontSkipGrab = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "HeavenlyPBuster");
	move.displayName = assignName("Heavenly Potemkin Buster", "HPB");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "HeavenlyPBusterBurst");
	move.displayName = assignName("Burst Heavenly Potemkin Buster", "Burst HPB");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "HeavenlyPBusterExe");
	move.displayName = assignName("Heavenly Potemkin Buster", "HPB");
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "HammerFallBrake");
	move.displayName = assignName("Hammer Fall Break");
	move.combineWithPreviousMove = true;
	move.splitForComboRecipe = true;
	move.usePlusSignInCombination = true;
	move.partOfStance = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "SlideHead_Obj", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("Slide Head");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "FDB_Obj", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("FDB");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "Giganter");
	move.displayName = assignName("Giganter Kai", "Giganter");
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_giganter;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "GiganObj", true);
	move.isDangerous = isDangerous_notNull;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("Giganter");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "GiganticBullet");
	move.displayName = assignName("Gigantic Bullet Kai", "Bullet");
	move.dontSkipSuper = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_POTEMKIN, "Bomb", true);
	move.isDangerous = isDangerous_not_hasHitNumButNoHitboxes;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("Trishula");
	addMove(move);
	
	// Venom has only one special move that can make him airborne: Teleport.
	// Which means he will never care about super jump installs.
	charDoesNotCareAboutSuperJumpInstalls[CHARACTER_TYPE_VENOM] = true;
	
	// not ignoring jump installs here, because it can whiff cancel into teleport and normally you can't
	// double jump from it, but if you jump install, you can.
	// You do get a guaranteed airdash from teleport, so we will only ignore super jump installs
	move = MoveInfo(CHARACTER_TYPE_VENOM, "BallSeiseiA");
	move.displayName = assignName("P Ball Set", "P Ball");
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.addForceAddWhiffCancel("WarpB");
	move.addForceAddWhiffCancel("BallSeiseiB_Hold");
	move.addForceAddWhiffCancel("BallSeiseiC_Hold");
	move.addForceAddWhiffCancel("BallSeiseiD_Hold");
	move.conditionForAddingWhiffCancels = hasWhiffCancels;
	move.createdProjectile = createdProjectile_ballSet;
	move.canYrcProjectile = canYrcProjectile_ballSeiseiA;
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "BallSeiseiA_Hold");
	move.displayName = assignName("P Ball Set", "P Ball");
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.addForceAddWhiffCancel("WarpB");
	move.addForceAddWhiffCancel("BallSeiseiB_Hold");
	move.addForceAddWhiffCancel("BallSeiseiC_Hold");
	move.addForceAddWhiffCancel("BallSeiseiD_Hold");
	move.conditionForAddingWhiffCancels = hasWhiffCancels;
	move.createdProjectile = createdProjectile_ballSet;
	move.canYrcProjectile = canYrcProjectile_ballSeiseiA;
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "BallSeiseiB");
	move.displayName = assignName("K Ball Set", "K Ball");
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.addForceAddWhiffCancel("WarpB");
	move.addForceAddWhiffCancel("BallSeiseiA_Hold");
	move.addForceAddWhiffCancel("BallSeiseiC_Hold");
	move.addForceAddWhiffCancel("BallSeiseiD_Hold");
	move.conditionForAddingWhiffCancels = hasWhiffCancels;
	move.createdProjectile = createdProjectile_ballSet;
	move.canYrcProjectile = canYrcProjectile_ballSeiseiB;
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "BallSeiseiB_Hold");
	move.displayName = assignName("K Ball Set", "K Ball");
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.addForceAddWhiffCancel("WarpB");
	move.addForceAddWhiffCancel("BallSeiseiA_Hold");
	move.addForceAddWhiffCancel("BallSeiseiC_Hold");
	move.addForceAddWhiffCancel("BallSeiseiD_Hold");
	move.conditionForAddingWhiffCancels = hasWhiffCancels;
	move.createdProjectile = createdProjectile_ballSet;
	move.canYrcProjectile = canYrcProjectile_ballSeiseiB;
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "BallSeiseiC");
	move.displayName = assignName("S Ball Set", "S Ball");
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.addForceAddWhiffCancel("WarpB");
	move.addForceAddWhiffCancel("BallSeiseiA_Hold");
	move.addForceAddWhiffCancel("BallSeiseiB_Hold");
	move.addForceAddWhiffCancel("BallSeiseiD_Hold");
	move.conditionForAddingWhiffCancels = hasWhiffCancels;
	move.createdProjectile = createdProjectile_ballSet;
	move.canYrcProjectile = canYrcProjectile_ballSeiseiC;
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "BallSeiseiC_Hold");
	move.displayName = assignName("S Ball Set", "S Ball");
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.addForceAddWhiffCancel("WarpB");
	move.addForceAddWhiffCancel("BallSeiseiA_Hold");
	move.addForceAddWhiffCancel("BallSeiseiB_Hold");
	move.addForceAddWhiffCancel("BallSeiseiD_Hold");
	move.conditionForAddingWhiffCancels = hasWhiffCancels;
	move.createdProjectile = createdProjectile_ballSet;
	move.canYrcProjectile = canYrcProjectile_ballSeiseiC;
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "BallSeiseiD");
	move.displayName = assignName("H Ball Set", "H Ball");
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.addForceAddWhiffCancel("WarpB");
	move.addForceAddWhiffCancel("BallSeiseiA_Hold");
	move.addForceAddWhiffCancel("BallSeiseiB_Hold");
	move.addForceAddWhiffCancel("BallSeiseiC_Hold");
	move.conditionForAddingWhiffCancels = hasWhiffCancels;
	move.createdProjectile = createdProjectile_ballSet;
	move.canYrcProjectile = canYrcProjectile_ballSeiseiD;
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);

	move = MoveInfo(CHARACTER_TYPE_VENOM, "BallSeiseiD_Hold");
	move.displayName = assignName("H Ball Set", "H Ball");
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.addForceAddWhiffCancel("WarpB");
	move.addForceAddWhiffCancel("BallSeiseiA_Hold");
	move.addForceAddWhiffCancel("BallSeiseiB_Hold");
	move.addForceAddWhiffCancel("BallSeiseiC_Hold");
	move.conditionForAddingWhiffCancels = hasWhiffCancels;
	move.createdProjectile = createdProjectile_ballSet;
	move.canYrcProjectile = canYrcProjectile_ballSeiseiD;
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "AirBallSeiseiA");
	move.displayName = assignName("Air P Ball Set", "Air P Ball");
	move.createdProjectile = createdProjectile_ballSet;
	move.canYrcProjectile = canYrcProjectile_airBallSeiseiA;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "AirBallSeiseiB");
	move.displayName = assignName("Air K Ball Set", "Air K Ball");
	move.createdProjectile = createdProjectile_ballSet;
	move.canYrcProjectile = canYrcProjectile_airBallSeiseiB;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "AirBallSeiseiC");
	move.displayName = assignName("Air S Ball Set", "Air S Ball");
	move.createdProjectile = createdProjectile_ballSet;
	move.canYrcProjectile = canYrcProjectile_airBallSeiseiC;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "AirBallSeiseiD");
	move.displayName = assignName("Air H Ball Set", "Air H Ball");
	move.createdProjectile = createdProjectile_ballSet;
	move.canYrcProjectile = canYrcProjectile_airBallSeiseiD;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "MadStrugguleD");
	move.displayName = assignName("H Mad Struggle");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "MadStrugguleC");
	move.displayName = assignName("S Mad Struggle");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "Warp");
	move.displayName = assignName("Teleport");
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "WarpB");
	move.displayName = assignName("Teleport");
	move.replacementInputs = "Hold the button you set the ball with";
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "DoubleHeadMorbidD");
	move.displayName = assignName("H Double Head Morbid", "HDHM");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "DoubleHeadMorbidC");
	move.displayName = assignName("S Double Head Morbid", "SDHM");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "StingerAimD");
	move.displayName = assignName("H Stinger Aim", "H Stinger");
	move.displayNameSelector = displayNameSelector_stingerH;
	move.sectionSeparator = sectionSeparator_stingerH;
	move.isInVariableStartupSection = isInVariableStartupSection_stinger;
	move.canYrcProjectile = canYrcProjectile_stinger;
	move.powerup = powerup_stingerH;
	move.ignoreJumpInstalls = true;
	move.charge = charge_stingerAim;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "StingerAimC");
	move.displayName = assignName("S Stinger Aim", "S Stinger");
	move.displayNameSelector = displayNameSelector_stingerS;
	move.sectionSeparator = sectionSeparator_stingerS;
	move.isInVariableStartupSection = isInVariableStartupSection_stinger;
	move.canYrcProjectile = canYrcProjectile_stinger;
	move.powerup = powerup_stingerS;
	move.ignoreJumpInstalls = true;
	move.charge = charge_stingerAim;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "CarcassRaidD");
	move.displayName = assignName("H Carcass Raid", "H Carcass");
	move.displayNameSelector = displayNameSelector_carcassRaidH;
	move.canYrcProjectile = canYrcProjectile_carcass;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "CarcassRaidC");
	move.displayName = assignName("S Carcass Raid", "S Carcass");
	move.displayNameSelector = displayNameSelector_carcassRaidS;
	move.canYrcProjectile = canYrcProjectile_carcass;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	// Venom QV
	move = MoveInfo(CHARACTER_TYPE_VENOM, "DubiousCurveA");
	move.displayName = assignName("P QV");
	move.sectionSeparator = sectionSeparator_QV;
	move.isInVariableStartupSection = isInVariableStartupSection_qv;
	move.createdProjectile = createdProjectile_qv;
	move.canYrcProjectile = canYrcProjectile_qvA;
	move.powerup = powerup_qvA;
	move.ignoreJumpInstalls = true;
	move.charge = charge_dubiousCurve;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "DubiousCurveB");
	move.displayName = assignName("K QV");
	move.sectionSeparator = sectionSeparator_QV;
	move.isInVariableStartupSection = isInVariableStartupSection_qv;
	move.createdProjectile = createdProjectile_qv;
	move.canYrcProjectile = canYrcProjectile_qvB;
	move.powerup = powerup_qvB;
	move.ignoreJumpInstalls = true;
	move.charge = charge_dubiousCurve;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "DubiousCurveC");
	move.displayName = assignName("S QV");
	move.sectionSeparator = sectionSeparator_QV;
	move.isInVariableStartupSection = isInVariableStartupSection_qv;
	move.createdProjectile = createdProjectile_qv;
	move.canYrcProjectile = canYrcProjectile_qvC;
	move.powerup = powerup_qvC;
	move.ignoreJumpInstalls = true;
	move.charge = charge_dubiousCurve;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "DubiousCurveD");
	move.displayName = assignName("H QV");
	move.sectionSeparator = sectionSeparator_QV;
	move.isInVariableStartupSection = isInVariableStartupSection_qv;
	move.createdProjectile = createdProjectile_qv;
	move.canYrcProjectile = canYrcProjectile_qvD;
	move.powerup = powerup_qvD;
	move.ignoreJumpInstalls = true;
	move.charge = charge_dubiousCurve;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "RedHail");
	move.displayName = assignName("Red Hail");
	move.canYrcProjectile = canYrcProjectile_redHail;
	move.isMove = true;
	addMove(move);
	
	// this is Stinger and Carcass Raid balls, ball set, including when such balls are launched.
	// Charged balls and even Bishop Runout and Red Hail are also this
	move = MoveInfo(CHARACTER_TYPE_VENOM, "Ball", true);
	move.isDangerous = isDangerous_active;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("Balls", "Balls");
	move.framebarNameSelector = framebarNameSelector_venomBall;
	move.framebarNameUncombined = assignName("Ball");
	move.framebarNameUncombinedSelector = framebarNameUncombinedSelector_venomBall;
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	// every QV when released creates this shockwave and it persists on RC
	move = MoveInfo(CHARACTER_TYPE_VENOM, "Debious_AttackBall", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("QV");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "DarkAngel");
	move.displayName = assignName("Dark Angel", "DA");
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_darkAngel;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "DarkAngelBurst");
	move.displayName = assignName("Burst Dark Angel", "Burst DA");
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_darkAngel;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "SummonGoldBall");
	move.displayName = assignName("Bishop Runout", "Bishop");
	move.dontSkipSuper = true;
	move.createdProjectile = createdProjectile_bishop;
	move.canYrcProjectile = canYrcProjectile_bishop;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	rememberFramebarId(DarkAngel_framebarId);

	// created before Dark Angel comes out
	move = MoveInfo(CHARACTER_TYPE_VENOM, "DarkAngelBallStart", true);
	move.isDangerous = isDangerous_hasNotCreatedAnythingYet;
	move.framebarId = DarkAngel_framebarId;
	move.framebarName = assignName("Dark Angel", "DA");
	move.framebarNameUncombined = assignName("Dark Angel Spawner", "DA Spawner");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_VENOM, "DarkAngelBall", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = DarkAngel_framebarId;
	move.framebarName = assignName("Dark Angel", "DA");
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "NmlAtk6B");
	move.displayName = assignName("6K");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "NmlAtk2D");
	move.displayName = assignName("2H");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "NmlAtk6A");
	move.displayName = assignName("6P");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "NmlAtk2E");
	move.displayName = assignName("2D");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	// The only moves that you get from Dandy Steps that make you airborne are Helter-Skelter and Crosswise Heel,
	// and those give you an airdash anyway, so there is no need to care about super jump installs.
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "DandyStepA");
	move.displayName = assignName("P Dandy Step", "P-Dandy");
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = hasWhiffCancels;
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "DandyStepB");
	move.displayName = assignName("K Dandy Step", "K-Dandy");
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = hasWhiffCancels;
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	// Slayer dandy step follow-ups
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "CrossWise");
	move.displayName = assignName("Crosswise Heel", "CW");
	move.displayNameSelector = displayNameSelector_crosswise;
	move.combineWithPreviousMove = true;
	move.splitForComboRecipe = true;
	move.usePlusSignInCombination =  true;
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "UnderPressure");
	move.displayName = assignName("Under Pressure", "UP");
	move.displayNameSelector = displayNameSelector_underPressure;
	move.combineWithPreviousMove = true;
	move.splitForComboRecipe = true;
	move.usePlusSignInCombination =  true;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "ItsLate");
	move.displayName = assignName("It's Late", "IL");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "RetroFire");
	move.displayName = assignName("Helter Skelter", "Helter");
	move.combineWithPreviousMove = true;
	move.splitForComboRecipe = true;
	move.usePlusSignInCombination =  true;
	move.canYrcProjectile = canYrcProjectile_helterSkelter;
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "Retro", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("Helter Skelter", "Helter");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "PileBunker");
	move.displayName = assignName("Pilebunker", "Pile");
	move.displayNameSelector = displayNameSelector_pilebunker;
	move.combineWithPreviousMove = true;
	move.splitForComboRecipe = true;
	move.usePlusSignInCombination =  true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "6BFeint");
	move.displayName = assignName("6K Feint");
	move.combineWithPreviousMove = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "MappaHunchA");
	move.displayName = assignName("P Mappa Hunch", "P Mappa");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "MappaHunchB");
	move.displayName = assignName("K Mappa Hunch", "K Mappa");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "FootLoose");
	move.displayName = assignName("Footloose Journey", "Footloose");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "ChiwosuuUchuu");
	move.displayName = assignName("Bloodsucking Universe", "Bite");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "ChiwosuuUchuuExe");
	move.displayName = assignName("Bloodsucking Universe", "Bite");
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "UnderTow");
	move.displayName = assignName("Undertow");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "ChokkagataDandy");
	move.displayName = assignName("Straight-Down Dandy", "SDD");
	move.canYrcProjectile = canYrcProjectile_sdd;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "EienNoTsubasa");
	move.displayName = assignName("Eternal Wings", "EW");
	move.iKnowExactlyWhenTheRecoveryOfThisMoveIs = isRecovery_land;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "DeadOnTime");
	move.displayName = assignName("Dead on Time", "DoT");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "DeadOnTimeBurst");
	move.displayName = assignName("Burst Dead on Time", "BDoT");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SLAYER, "KetsuFire", true);
	move.isDangerous = isDangerous_not_hasHitNumButNoHitboxes;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("Straight-Down Dandy", "SDD");
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
	move.displayName = assignName("5D");
	move.nameIncludesInputs = true;
	move.createdProjectile = createdProjectile_ino5D;
	move.canYrcProjectile = canYrcProjectile_ino5D;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "AirFDash_Under");
	move.displayName = assignName("Downwards Dash", "Hoverdown");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "KouutsuOnkai");
	move.displayName = MOVE_NAME_NOTE;
	move.canYrcProjectile = canYrcProjectile_kouutsuOnkai;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "KouutsuOnkaiAir");
	move.displayName = assignName("Air Antidepressant Scale", "Air Note");
	move.canYrcProjectile = canYrcProjectile_kouutsuOnkai;
	move.isMove = true;
	addMove(move);
	
	// I-No Sultry Performance
	move = MoveInfo(CHARACTER_TYPE_INO, "KyougenA");
	move.displayName = assignName("P Sultry Performance", "P-Dive");
	move.sectionSeparator = sectionSeparator_sultryPerformance;
	move.isIdle = isIdle_enableWhiffCancels;
	move.canBlock = canBlock_default;
	move.isInVariableStartupSection = isInVariableStartupSection_inoDivekick;
	move.powerup = powerup_kyougenA;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "KyougenB");
	move.displayName = assignName("K Sultry Performance", "K-Dive");
	move.sectionSeparator = sectionSeparator_sultryPerformance;
	move.isIdle = isIdle_enableWhiffCancels;
	move.canBlock = canBlock_default;
	move.isInVariableStartupSection = isInVariableStartupSection_inoDivekick;
	move.powerup = powerup_kyougenB;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "KyougenC");
	move.displayName = assignName("S Sultry Performance", "S-Dive");
	move.sectionSeparator = sectionSeparator_sultryPerformance;
	move.isIdle = isIdle_enableWhiffCancels;
	move.canBlock = canBlock_default;
	move.isInVariableStartupSection = isInVariableStartupSection_inoDivekick;
	move.powerup = powerup_kyougenC;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "KyougenD");
	move.displayName = assignName("H Sultry Performance", "H-Dive");
	move.sectionSeparator = sectionSeparator_sultryPerformance;
	move.isIdle = isIdle_enableWhiffCancels;
	move.canBlock = canBlock_default;
	move.isInVariableStartupSection = isInVariableStartupSection_inoDivekick;
	move.powerup = powerup_kyougenD;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "CommandThrow");
	move.displayName = assignName("Sterilization Method", "SM");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "CommandThrowExe");
	move.displayName = assignName("Sterilization Method", "SM");
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "AirCommandThrow");
	move.displayName = assignName("Air Sterilization Method", "Air SM");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "AirCommandThrowExe");
	move.displayName = assignName("Air Sterilization Method", "Air SM");
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "TaibokuC");
	move.displayName = assignName("S Strike the Big Tree", "S-STBT");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "TaibokuD");
	move.displayName = assignName("H Strike the Big Tree", "H-STBT");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "ChemicalB");
	move.displayName = assignName("Chemical Love", "HCL");
	move.canYrcProjectile = canYrcProjectile_chemicalLove;
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "ChemicalAirB");
	move.displayName = assignName("Air Chemical Love", "Air HCL");
	move.canYrcProjectile = canYrcProjectile_chemicalLove;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "ChemicalC");
	move.displayName = assignName("Vertical Chemical Love", "VCL");
	move.canYrcProjectile = canYrcProjectile_chemicalLove;
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "ChemicalAirC");
	move.displayName = assignName("Air Vertical Chemical Love", "Air VCL");
	move.canYrcProjectile = canYrcProjectile_chemicalLove;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "ChemicalAdd");
	move.displayName = assignName("Chemical Love (Follow-up)", "214K~214S");
	move.combineWithPreviousMove = true;
	move.splitForComboRecipe = true;
	move.butForFramebarDontCombineWithPreviousMove = true;
	move.canYrcProjectile = canYrcProjectile_chemicalLove;  // typically opponent will be in blockstun, but I did a hacktest and ye you can YRC this
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "Madogiwa");
	move.displayName = assignName("Longing Desperation", "Desperation");
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_madogiwa;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);

	move = MoveInfo(CHARACTER_TYPE_INO, "MadogiwaBurst");
	move.displayName = assignName("Burst Longing Desperation", "Burst Desperation");
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_madogiwa;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "Genkai");
	move.displayName = assignName("Ultimate Fortissimo", "Fortissimo");
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_genkai;
	move.isMove = true;
	addMove(move);
	
	rememberFramebarId(ChemicalLove_framebarId);

	move = MoveInfo(CHARACTER_TYPE_INO, "BChemiLaser", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = ChemicalLove_framebarId;
	move.framebarName = assignName("Chemical Love", "HCL");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "AddChemiLaser", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = ChemicalLove_framebarId;
	move.framebarName = assignName("Chemical Love (Follow-up)", "214K~214S");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "CChemiLaser", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = ChemicalLove_framebarId;
	move.framebarName = assignName("Vertical Chemical Love", "VCL");
	addMove(move);
	
	rememberFramebarId(Onpu_framebarId);

	move = MoveInfo(CHARACTER_TYPE_INO, "Onpu", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = Onpu_framebarId;
	move.framebarName = assignName("Antidepressant Scale", "Note");
	move.projectilePowerup = projectilePowerup_onpu;
	addMove(move);
	
	// Boss version only
	move = MoveInfo(CHARACTER_TYPE_INO, "Onpu2", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = Onpu_framebarId;
	move.framebarName = assignName("Antidepressant Scale", "Note");
	move.framebarNameUncombined = assignName("Antidepressant Scale 2", "Note 2");
	move.projectilePowerup = projectilePowerup_onpu;
	addMove(move);
	
	// Boss version only
	move = MoveInfo(CHARACTER_TYPE_INO, "Onpu3", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = Onpu_framebarId;
	move.framebarName = assignName("Antidepressant Scale", "Note");
	move.framebarNameUncombined = assignName("Antidepressant Scale 3", "Note 3");
	move.projectilePowerup = projectilePowerup_onpu;
	addMove(move);
	
	// cannot be YRC'd in Rev1
	move = MoveInfo(CHARACTER_TYPE_INO, "DustObjShot", true);
	move.isDangerous = isDangerous_playerInRCOrHasActiveFlag_AndNotInRecovery;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("5D");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "GenkaiObj", true);
	move.isDangerous = isDangerous_not_hasHitNumButNoHitboxes;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("Ultimate Fortissimo", "Fortissimo");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_INO, "MadogiwaObj", true);
	move.isDangerous = isDangerous_not_hasHitNumButNoHitboxes;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("Longing Desperation", "Desperation");
	addMove(move);
	
	// The only move that can put Bedman into the air is Task C, and he gets a free airdash from it.
	// So, Bedman does not care about super jump installs.
	// Bedman does not care about regular jump installs either, as he has no double jump - it's a
	// hover, which works even when he has no double jumps.
	charDoesNotCareAboutSuperJumpInstalls[CHARACTER_TYPE_BEDMAN] = true;
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "CmnActFDash");
	move.displayName = assignName("Forward Dash");
	move.nameIncludesInputs = true;
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.considerVariableStartupAsStanceForFramebar = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "NmlAtk6D_2");
	move.displayName = assignName("6H (Follow-up)", "6H~6H");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "CrouchFWalk");
	move.displayName = assignName("Crouchwalk Forward", "3");
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "CrouchBWalk");
	move.displayName = assignName("Crouchwalk Back", "1");
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Boomerang_A");
	move.displayName = assignName("Task A", "TA");
	move.powerup = powerup_boomerangA;
	move.dontShowPowerupGraphic = dontShowPowerupGraphic_boomerangA;
	move.canYrcProjectile = canYrcProjectile_boomerang;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Boomerang_A_Air");
	move.displayName = assignName("Air Task A", "j.TA");
	move.powerup = powerup_boomerangAAir;
	move.dontShowPowerupGraphic = dontShowPowerupGraphic_boomerangAAir;
	move.canYrcProjectile = canYrcProjectile_boomerang;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Dejavu_A");
	move.displayName = assignName("\x44\xC3\xA9\x6A\xC3\xA0 Vu A", "DVA");
	move.powerup = powerup_djavu;
	move.dontShowPowerupGraphic = alwaysTrue;
	move.canYrcProjectile = canYrcProjectile_dejavuAB;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Dejavu_A_Air");
	move.displayName = assignName("Air \x44\xC3\xA9\x6A\xC3\xA0 Vu A", "j.DVA");
	move.powerup = powerup_djavu;
	move.dontShowPowerupGraphic = alwaysTrue;
	move.canYrcProjectile = canYrcProjectile_dejavuAB;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Boomerang_B");
	move.displayName = assignName("Task A'", "TA'");
	move.powerup = powerup_boomerangB;
	move.dontShowPowerupGraphic = dontShowPowerupGraphic_boomerangB;
	move.canYrcProjectile = canYrcProjectile_boomerang;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Boomerang_B_Air");
	move.displayName = assignName("Air Task A'", "j.TA'");
	move.powerup = powerup_boomerangBAir;
	move.dontShowPowerupGraphic = dontShowPowerupGraphic_boomerangBAir;
	move.canYrcProjectile = canYrcProjectile_boomerang;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Dejavu_B");
	move.displayName = assignName("\x44\xC3\xA9\x6A\xC3\xA0 Vu A'", "DVA'");
	move.powerup = powerup_djavu;
	move.dontShowPowerupGraphic = alwaysTrue;
	move.canYrcProjectile = canYrcProjectile_dejavuAB;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Dejavu_B_Air");
	move.displayName = assignName("Air \x44\xC3\xA9\x6A\xC3\xA0 Vu A'", "j.DVA'");
	move.powerup = powerup_djavu;
	move.dontShowPowerupGraphic = alwaysTrue;
	move.canYrcProjectile = canYrcProjectile_dejavuAB;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "SpiralBed");
	move.displayName = assignName("Task B", "TB");
	move.powerup = powerup_taskB;
	move.dontShowPowerupGraphic = dontShowPowerupGraphic_taskB;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "SpiralBed_Air");
	move.displayName = assignName("Air Task B", "j.TB");
	move.powerup = powerup_taskBAir;
	move.dontShowPowerupGraphic = dontShowPowerupGraphic_taskBAir;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Dejavu_C");
	move.displayName = assignName("\x44\xC3\xA9\x6A\xC3\xA0 Vu B", "DVB");
	move.powerup = powerup_djavu;
	move.dontShowPowerupGraphic = alwaysTrue;
	move.canYrcProjectile = canYrcProjectile_dejavuC;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Dejavu_C_Air");
	move.displayName = assignName("Air \x44\xC3\xA9\x6A\xC3\xA0 Vu B", "j.DVB");
	move.powerup = powerup_djavu;
	move.dontShowPowerupGraphic = alwaysTrue;
	move.canYrcProjectile = canYrcProjectile_dejavuC;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "FlyingBed");
	move.displayName = assignName("Task C", "TC");
	move.powerup = powerup_taskC;
	move.dontShowPowerupGraphic = dontShowPowerupGraphic_taskC;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "FlyingBed_Air");
	move.displayName = assignName("Air Task C", "j.TC");
	move.displayNameSelector = displayNameSelector_taskCAir;
	move.canYrcProjectile = canYrcProjectile_default;
	move.powerup = powerup_taskCAir;
	move.dontShowPowerupGraphic = dontShowPowerupGraphic_taskCAir;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Dejavu_D");
	move.displayName = assignName("\x44\xC3\xA9\x6A\xC3\xA0 Vu C", "DVC");
	move.powerup = powerup_djavu;
	move.dontShowPowerupGraphic = alwaysTrue;
	move.canYrcProjectile = canYrcProjectile_dejavuD;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Dejavu_D_Air");
	move.displayName = assignName("Air \x44\xC3\xA9\x6A\xC3\xA0 Vu C", "j.DVC");
	move.powerup = powerup_djavu;
	move.dontShowPowerupGraphic = alwaysTrue;
	move.canYrcProjectile = canYrcProjectile_dejavuD;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Alarm");
	move.displayName = assignName("Sinusoidal Helios", "Clock Super");
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_alarm;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "AlarmBurst");
	move.displayName = assignName("Burst Sinusoidal Helios");
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_alarm;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Merry");
	move.displayName = assignName("Hemi Jack", "Sheep Super");
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_merry;
	move.isMove = true;
	addMove(move);
	
	// Bedman Teleporting from the boomerang head hitting
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "BWarp");
	move.displayName = assignName("Task A' Teleport");
	move.combineWithPreviousMove = true;
	move.splitForComboRecipe = true;
	move.dontSkipSuper = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Aralm_Obj", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("Sinusoidal Helios", "Helios");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Okkake", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("Hemi Jack", "Sheep Super");
	addMove(move);
	
	rememberFramebarId(Boomerang_A_Head_framebarId);
	
	// the flying head
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Boomerang_A_Head", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = Boomerang_A_Head_framebarId;
	move.framebarName = assignName("Task A", "Boomerang");
	move.framebarNameUncombined = assignName("Task A Boomerang Head", "Boomerang");
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Boomerang_A_Head_Air", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = Boomerang_A_Head_framebarId;
	move.framebarName = assignName("Task A", "Boomerang Head");
	move.framebarNameUncombined = assignName("Task A Boomerang Head", "Boomerang Head");
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	rememberFramebarId(Djavu_A_framebarId);

	// created when doing Deja Vu (Task A). Creates either Boomerang_A_Djavu or Boomerang_A_Djavu_Air
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Djavu_A_Ghost", true);
	move.isDangerous = isDangerous_djavu;
	move.framebarId = Djavu_A_framebarId;
	move.framebarName = assignName("\x44\xC3\xA9\x6A\xC3\xA0 Vu (Task A)", "DVA");
	move.framebarNameUncombined = assignName("\x44\xC3\xA9\x6A\xC3\xA0 Vu Ghost (Task A)", "DVA Ghost");
	addMove(move);
	
	// the flying head
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Boomerang_A_Djavu", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = Djavu_A_framebarId;
	move.framebarName = assignName("\x44\xC3\xA9\x6A\xC3\xA0 Vu (Task A)", "DVA");
	move.framebarNameUncombined = assignName("\x44\xC3\xA9\x6A\xC3\xA0 Vu Ghost (Task A)", "DVA Boomerang Head");
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Boomerang_A_Djavu_Air", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = Djavu_A_framebarId;
	move.framebarName = assignName("\x44\xC3\xA9\x6A\xC3\xA0 Vu (Air Task A)", "j.DVA");
	move.framebarNameUncombined = assignName("\x44\xC3\xA9\x6A\xC3\xA0 Vu Boomerang Head (Air Task A)", "j.DVA Boomerang Head");
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	rememberFramebarId(Boomerang_B_Head_framebarId);

	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Boomerang_B_Head", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = Boomerang_B_Head_framebarId;
	move.framebarName = assignName("Task A'", "Teleport Boomerang Head");
	move.framebarNameUncombined = assignName("Task A' Boomerang Head", "Teleport Boomerang Head");
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Boomerang_B_Head_Air", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = Boomerang_B_Head_framebarId;
	move.framebarName = assignName("Task A'");
	move.framebarNameUncombined = assignName("Task A' Boomerang Head", "Teleport Boomerang Head");
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	rememberFramebarId(Djavu_B_framebarId);

	// created when doing Deja Vu (Task A'). Creates either Boomerang_B_Djavu or Boomerang_B_Djavu_Air
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Djavu_B_Ghost", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = Djavu_B_framebarId;
	move.framebarName = assignName("\x44\xC3\xA9\x6A\xC3\xA0 Vu (Task A')", "DVA'");
	move.framebarNameUncombined = assignName("\x44\xC3\xA9\x6A\xC3\xA0 Vu Ghost (Task A')", "DVA' Ghost");
	addMove(move);
	
	// the flying head
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Boomerang_B_Djavu", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = Djavu_B_framebarId;
	move.framebarName = assignName("\x44\xC3\xA9\x6A\xC3\xA0 Vu (Task A')", "DVA'");
	move.framebarNameUncombined = assignName("\x44\xC3\xA9\x6A\xC3\xA0 Vu Boomerang Head (Task A')", "DVA' Boomerang Head");
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Boomerang_B_Djavu_Air", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = Djavu_B_framebarId;
	move.framebarName = assignName("\x44\xC3\xA9\x6A\xC3\xA0 Vu (Air Task A')", "j.DVA'");
	move.framebarNameUncombined = assignName("\x44\xC3\xA9\x6A\xC3\xA0 Vu Boomerang Head (Air Task A')", "j.DVA' Boomerang Head");
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Djavu_C_Ghost", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("\x44\xC3\xA9\x6A\xC3\xA0 Vu (Task B)", "DVB");
	move.framebarNameUncombined = assignName("\x44\xC3\xA9\x6A\xC3\xA0 Vu Ghost (Task B)", "DVB Ghost");
	addMove(move);
	
	rememberFramebarId(BedmanTaskCShockwave_framebarId);

	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "bomb1", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = BedmanTaskCShockwave_framebarId;
	move.framebarName = assignName("Task C Shockwave", "Shockwave");
	move.framebarNameUncombined = assignName("Task C Shockwave", "Shockwave");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "bomb2", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = BedmanTaskCShockwave_framebarId;
	move.framebarName = assignName("Task C Shockwave", "Shockwave");
	move.framebarNameUncombined = assignName("Task C Big Shockwave", "Big Shockwave");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "Djavu_D_Ghost", true);
	move.isDangerous = isDangerous_Djavu_D_Ghost;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("\x44\xC3\xA9\x6A\xC3\xA0 Vu (Task C)", "DVC");
	move.framebarNameSelector = framebarNameSelector_djvuD;
	move.framebarNameUncombined = assignName("\x44\xC3\xA9\x6A\xC3\xA0 Vu Ghost (Task C)", "DVC Ghost");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "AirStop");
	move.displayName = assignName("Hover");
	move.addForceAddWhiffCancel("7Move");
	move.addForceAddWhiffCancel("8Move");
	move.addForceAddWhiffCancel("9Move");
	move.conditionForAddingWhiffCancels = conditionForAddingWhiffCancels_airStop;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "AirStopCancelOnly");
	move.displayName = assignName("Hover Cancel");
	// displaying cancels on this is misleading, because you can't cancel into multidirectional hovers until 10 frames have passed, and it always picks a direction
	// there is no way to view frames after 10 of this move
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "9Move");
	move.displayName = assignName("Hover-9");
	move.replacementInputs = "Hold 9 for 12f";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "8Move");
	move.displayName = assignName("Hover-8");
	move.replacementInputs = "Hold 8 for 12f";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "7Move");
	move.displayName = assignName("Hover-7");
	move.replacementInputs = "Hold 7 for 12f";
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "6Move");
	move.displayName = assignName("Hover-6");
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "4Move");
	move.displayName = assignName("Hover-4");
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "3Move");
	move.displayName = assignName("Hover-3");
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "2Move");
	move.displayName = assignName("Hover-2");
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "1Move");
	move.displayName = assignName("Hover-1");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BEDMAN, "MerryWarp");
	move.displayName = assignName("Teleport to Sheep");
	addMove(move);
	
	// Ramlethal has two moves that can make her airborne: Sildo Detruo and Explode.
	// Sildo Detruo gives an airdash, and Explode gives nothing without jump installs.
	// Because of Explode, we have to not ignore super jump installs.
	// So anything that can't route into Explode, we have to ignore super jump installs there.
	// Turns out, there is not one such move, at least not where we'd want regular jump installs, but not super jump installs.
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "NmlAtk6B");
	move.displayName = assignName("6K");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "NmlAtk3B");
	move.displayName = assignName("3K");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitN", true);
	move.displayName = assignName("Stand");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitNBack", true);
	move.displayName = assignName("Back");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitNForward", true);
	move.displayName = assignName("Forward");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitNUp", true);
	move.displayName = assignName("Jump Up");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitNUpEnd", true);
	move.displayName = assignName("Jump End");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitNDown", true);
	move.displayName = assignName("Jump Down");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitNCrouch", true);
	move.displayName = assignName("Crouch");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitNTurn", true);
	move.displayName = assignName("Turn");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitNDamage", true);
	move.displayName = assignName("Damage");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitN5C", true);
	move.displayName = assignName("f.S");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitNAir5C", true);
	move.displayName = assignName("j.S");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitNAir5C_Boss", true);
	move.displayName = assignName("j.S (Boss)");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitN2C", true);
	move.displayName = assignName("2S");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitN2C_Boss", true);
	move.displayName = assignName("2S (Boss)");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitNBunri", true);
	move.displayName = assignName("Deployed");
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitFBunri", true);
	move.displayName = assignName("Deployed");
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "Bit4C", true);
	move.displayName = assignName("Recall");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitNLaser", true);
	move.displayName = assignName("Calvados");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitBlowC_Sword", true);
	move.displayName = assignName("Marteli");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitBlowC_Sword_Bunri", true);
	move.displayName = assignName("Marteli (Slow)");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitBlowC_SwordAir", true);
	move.displayName = assignName("Air Marteli");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitBlowC_SwordAir_Bunri", true);
	move.displayName = assignName("Air Marteli (Slow)");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitActionNeutral", true);
	move.displayName = assignName("Neutral");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitNWinTypeA", true);
	move.displayName = assignName("Victory");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitNWinTypeABunri", true);
	move.displayName = assignName("Victory Deployed");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitNWinTypeB", true);
	move.displayName = assignName("Victory");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitNWinTypeBBunri", true);
	move.displayName = assignName("Victory Deployed");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitN5C_Boss", true);
	move.displayName = assignName("f.S (Boss)");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitF", true);
	move.displayName = assignName("Stand");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitFBack", true);
	move.displayName = assignName("Back");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitFForward", true);
	move.displayName = assignName("Forward");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitFUp", true);
	move.displayName = assignName("Jump Up");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitFUpEnd", true);
	move.displayName = assignName("Jump End");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitFDown", true);
	move.displayName = assignName("Jump Down");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitFCrouch", true);
	move.displayName = assignName("Crouch");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitFTurn", true);
	move.displayName = assignName("Turn");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitFDamage", true);
	move.displayName = assignName("Damage");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitFAir5D", true);
	move.displayName = assignName("j.H");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitFAir5D_Boss", true);
	move.displayName = assignName("j.H (Boss)");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitF2D", true);
	move.displayName = assignName("2H");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitF2D_Boss", true);
	move.displayName = assignName("2H (Boss)");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitF5D", true);
	move.displayName = assignName("5H");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "Bit4D", true);
	move.displayName = assignName("Recall");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitFLaser", true);
	move.displayName = assignName("Calvados");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitBlowD_Sword", true);
	move.displayName = assignName("Forpeli");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitBlowD_Sword_Bunri", true);
	move.displayName = assignName("Forpeli (Slow)");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitBlowD_SwordAir", true);
	move.displayName = assignName("Air Forpeli");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitBlowD_SwordAir_Bunri", true);
	move.displayName = assignName("Air Forpeli (Slow)");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitFWinTypeA", true);
	move.displayName = assignName("Victory");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitFWinTypeABunri", true);
	move.displayName = assignName("Victory Deployed");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitFWinTypeB", true);
	move.displayName = assignName("Victory");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitFWinTypeBBunri", true);
	move.displayName = assignName("Victory Deployed");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "NmlAtk4B");
	move.displayName = assignName("5K");  // Ramlethal's NmlAtk4B is just an alias for 5K
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "6D_Soubi_Land");
	move.displayName = assignName("H Launch Greatsword", "6H");
	move.createdProjectile = createdProjectile_onf5_h;
	move.canYrcProjectile = canYrcProjectile_onf5_hLaunch;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "NmlAtk5DBunri");
	move.displayName = assignName("Unarmed 5H");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "6D_Bunri_Land");
	move.displayName = assignName("H Launch Greatsword (Already Deployed)", "6H");
	move.createdProjectile = createdProjectile_onf5_h;
	move.canYrcProjectile = canYrcProjectile_onf5_hRelaunch;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "4D_Bunri_Land");
	move.displayName = assignName("H Recover Greatsword", "4H");
	move.createdProjectile = createdProjectile_onf5_h_recall;
	move.canYrcProjectile = canYrcProjectile_onf5_hRecover;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "2D_Bunri_Land");
	move.displayName = assignName("2H Launch Greatsword", "2H Summon");
	move.nameIncludesInputs = true;
	move.createdProjectile = createdProjectile_onf7_h;
	move.canYrcProjectile = canYrcProjectile_onf7_hRelaunch;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "NmlAtk5C");
	move.displayName = assignName("f.S");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "6C_Soubi_Land");
	move.displayName = assignName("S Launch Greatsword", "6S");
	move.createdProjectile = createdProjectile_onf5_s;
	move.canYrcProjectile = canYrcProjectile_onf5_sLaunch;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "NmlAtk5CBunri");
	move.displayName = assignName("Unarmed f.S");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "6C_Bunri_Land");
	move.displayName = assignName("S Launch Greatsword (Already Deployed)", "6S");
	move.createdProjectile = createdProjectile_onf5_s;
	move.canYrcProjectile = canYrcProjectile_onf5_sRelaunch;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "4C_Bunri_Land");
	move.displayName = assignName("S Recover Greatsword", "4S");
	move.createdProjectile = createdProjectile_onf5_s_recall;
	move.canYrcProjectile = canYrcProjectile_onf5_sRecover;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "2C_Bunri_Land");
	move.displayName = assignName("2S Launch Greatsword", "2S Summon");
	move.nameIncludesInputs = true;
	move.createdProjectile = createdProjectile_onf7_s;
	move.canYrcProjectile = canYrcProjectile_onf7_sRelaunch;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "6D_Soubi_Air");
	move.displayName = assignName("Air H Launch Greatsword", "Air 6H");
	move.createdProjectile = createdProjectile_onf5_h;
	move.canYrcProjectile = canYrcProjectile_onf5_hLaunch;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "NmlAtkAir5DBunri");
	move.displayName = assignName("Unarmed j.H");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "6D_Bunri_Air");
	move.displayName = assignName("Air H Launch Greatsword (Already Deployed)", "Air 6H");
	move.createdProjectile = createdProjectile_onf5_h;
	move.canYrcProjectile = canYrcProjectile_onf5_hRelaunch;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "2D_Bunri_Air");
	move.displayName = assignName("Air 2H Launch Greatsword", "Air 2H Summon");
	move.nameIncludesInputs = true;
	move.createdProjectile = createdProjectile_onf5_h;
	move.canYrcProjectile = canYrcProjectile_onf5_hRelaunch;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "4D_Bunri_Air");
	move.displayName = assignName("Air H Recover Greatsword", "Air 4H");
	move.createdProjectile = createdProjectile_onf5_h_recall;
	move.canYrcProjectile = canYrcProjectile_onf5_hRecover;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "6C_Soubi_Air");
	move.displayName = assignName("Air S Launch Greatsword", "Air 6S");
	move.createdProjectile = createdProjectile_onf5_s;
	move.canYrcProjectile = canYrcProjectile_onf5_sLaunch;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "NmlAtkAir5CBunri");
	move.displayName = assignName("Unarmed j.S");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "6C_Bunri_Air");
	move.displayName = assignName("Air S Launch Greatsword (Already Deployed)", "Air 6S");
	move.createdProjectile = createdProjectile_onf5_s;
	move.canYrcProjectile = canYrcProjectile_onf5_sRelaunch;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "2C_Bunri_Air");
	move.displayName = assignName("Air 2S Launch Greatsword", "Air 2S Summon");
	move.nameIncludesInputs = true;
	move.createdProjectile = createdProjectile_onf5_s;
	move.canYrcProjectile = canYrcProjectile_onf5_sRelaunch;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "4C_Bunri_Air");
	move.displayName = assignName("Air S Recover Greatsword", "Air 4S");
	move.createdProjectile = createdProjectile_onf5_s_recall;
	move.canYrcProjectile = canYrcProjectile_onf5_sRecover;
	move.isMove = true;
	addMove(move);
	
	// Ramlethal's combination attacks only at best combo into sword summon, sword recall, 2S/2H sword summon or more combination attacks.
	// And none of them can make Ramlethal airborne.
	// As such, they do not care about jump installs.
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "CombinationBA");
	move.displayName = assignName("Combination KP");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "CombinationBBB");
	move.displayName = assignName("Combination KKK");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "CombinationBB");
	move.displayName = assignName("Combination KK");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "CombinationAB");
	move.displayName = assignName("Combination PK");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "CombinationAA");
	move.displayName = assignName("Combination PP");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "CombinationBAB");
	move.displayName = assignName("Combination KPK");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "CombinationBAA");
	move.displayName = assignName("Combination KPP");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "CombinationAAB");
	move.displayName = assignName("Combination PPK");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "CombinationAAA");
	move.displayName = assignName("Combination PPP");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "Combination2BB");
	move.displayName = assignName("Combination 2KK");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "Combination2BA");
	move.displayName = assignName("Combination 2KP");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "Combination2BAB");
	move.displayName = assignName("Combination 2KPK");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "Combination2BAA");
	move.displayName = assignName("Combination 2KPP");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "Combination2AAB");
	move.displayName = assignName("Combination 2PPK");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "Combination2AB");
	move.displayName = assignName("Combination 2PK");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "Combination2AA");
	move.displayName = assignName("Combination 2PP");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "Combination4B");
	move.displayName = assignName("Combination 4K");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "6CBunriShot");
	move.displayName = assignName("S Launch Greatsword (Boss Ver.)", "S Launch (Boss Ver.)");
	move.ignoreJumpInstalls = true;
	move.createdProjectile = createdProjectile_onf5_s;
	move.canYrcProjectile = canYrcProjectile_onf5_sRelaunch;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "6DBunriShot");
	move.displayName = assignName("H Launch Greatsword (Boss Ver.)", "H Launch (Boss Ver.)");
	move.ignoreJumpInstalls = true;
	move.createdProjectile = createdProjectile_onf5_h;
	move.canYrcProjectile = canYrcProjectile_onf5_hRelaunch;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "MiddleShot");
	move.displayName = assignName("Cassius");
	move.canYrcProjectile = canYrcProjectile_default;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BanditRevolverLand");
	move.displayName = assignName("Sildo Detruo");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BanditRevolverAir");
	move.displayName = assignName("Air Sildo Detruo");
	move.isMove = true;
	addMove(move);
	
	// Marteli and Forpeli are dead end moves where only Martli can cancel into Forpeli.
	// Which makes them not care about jump installs.
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitBlowD");
	move.displayName = assignName("Forpeli");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitBlowC");
	move.displayName = assignName("Marteli");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitBlowDAir");
	move.displayName = assignName("Air Forpeli");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitBlowCAir");
	move.displayName = assignName("Air Marteli");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitBlowD_Bunri");
	move.displayName = assignName("Forpeli With Sword Recover");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitBlowC_Bunri");
	move.displayName = assignName("Marteli With Sword Recover");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitBlowDAir_Bunri");
	move.displayName = assignName("Air Forpeli With Sword Recover");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitBlowCAir_Bunri");
	move.displayName = assignName("Air Marteli With Sword Recover");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "CommandThrow");
	move.displayName = assignName("Flama Cargo");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "CommandThrowExe");
	move.displayName = assignName("Flama Cargo");
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "FujinStep");
	move.displayName = assignName("Fujin Step");
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "EasyFujinken");
	move.displayName = assignName("Dauro");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "Fujinken");
	move.displayName = assignName("Dauro");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "FastFujinken");
	move.displayName = assignName("Green Dauro");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "TosshinRanbu");
	move.displayName = assignName("Explode");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "TosshinRanbuExe");
	move.displayName = assignName("Explode");
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitLaser");
	move.displayName = assignName("Calvados");
	//move.canYrcProjectile = canYrcProjectile_bitLaser;  // gets handled in EndScene.cpp using hardcoded rules, because we need to also fill in fancy "disappers on hit" flags
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitLaserBurst");
	move.displayName = assignName("Burst Calvados");
	//move.canYrcProjectile = canYrcProjectile_bitLaser;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitLaserBoss");
	move.displayName = assignName("Calvados (Boss Ver.)");
	//move.canYrcProjectile = canYrcProjectile_bitLaser;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitLaserBossBurst");
	move.displayName = assignName("Burst Calvados (Boss Ver.)");
	//move.canYrcProjectile = canYrcProjectile_bitLaser;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitSpiral");
	move.displayName = assignName("Trance");
	move.canYrcProjectile = canYrcProjectile_bitSpiral;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitSpiralBoss");
	move.displayName = assignName("Trance (Boss Ver.)");
	move.canYrcProjectile = canYrcProjectile_bitSpiral;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	rememberFramebarId(RamlethalSSword_framebarId);

	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitN6C", true);
	move.displayName = assignName("6S");
	move.isDangerous = isDangerous_launchGreatsword;
	move.framebarId = RamlethalSSword_framebarId;
	move.framebarName = assignName("S Sword");
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	rememberFramebarId(RamlethalHSword_framebarId);

	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitF6D", true);
	move.displayName = assignName("6H");
	move.isDangerous = isDangerous_launchGreatsword;
	move.framebarId = RamlethalHSword_framebarId;
	move.framebarName = assignName("H Sword");
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitN6CBunriShot", true);
	move.displayName = assignName("Sword Spinny Attack");
	move.isDangerous = isDangerous_ramSwordMove;
	move.framebarId = RamlethalSSword_framebarId;
	move.framebarName = assignName("S Sword");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitN2C_Bunri", true);
	move.displayName = assignName("2S Deployed");
	move.isDangerous = isDangerous_ramSwordMove;
	move.framebarId = RamlethalSSword_framebarId;
	move.framebarName = assignName("S Sword");
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitN6CBunriShot_Boss", true);
	move.displayName = assignName("Sword Spinny Attack (Boss)");
	move.isDangerous = isDangerous_ramSwordMove;
	move.framebarId = RamlethalSSword_framebarId;
	move.framebarName = assignName("S Sword");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitF6DBunriShot", true);
	move.displayName = assignName("Sword Spinny Attack");
	move.isDangerous = isDangerous_ramSwordMove;
	move.framebarId = RamlethalHSword_framebarId;
	move.framebarName = assignName("H Sword");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitF2D_Bunri", true);
	move.displayName = assignName("2H Deployed");
	move.isDangerous = isDangerous_ramSwordMove;
	move.framebarId = RamlethalHSword_framebarId;
	move.framebarName = assignName("H Sword");
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitF6DBunriShot_Boss", true);
	move.displayName = assignName("Sword Spinny Attack (Boss)");
	move.isDangerous = isDangerous_ramSwordMove;
	move.framebarId = RamlethalHSword_framebarId;
	move.framebarName = assignName("H Sword");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitSpiral_NSpiral", true);
	move.displayName = assignName("Trance");
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = RamlethalSSword_framebarId;
	move.framebarName = assignName("S Sword");
	move.combineHitsFromDifferentProjectiles = true;
	addMove(move);
	
	// does not have 'attackType:' set, so doesn't actually have any active frames
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitSpiral_FSpiral", true);
	move.displayName = assignName("Trance");
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = RamlethalHSword_framebarId;
	move.framebarName = assignName("H Sword");
	addMove(move);
	
	rememberFramebarId(ramlethal_bitLaser);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitLaser_Minion", true);
	move.isDangerous = isDangerous_hasNotCreatedAnythingYet;
	move.framebarId = ramlethal_bitLaser;
	move.framebarName = assignName("Laser");
	move.framebarNameUncombined = assignName("Laser Spawner");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "BitLaser", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = ramlethal_bitLaser;
	move.framebarName = assignName("Laser");
	move.combineHitsFromDifferentProjectiles = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAMLETHAL, "middleShot", true);
	move.isDangerous = isDangerous_not_hasHitNumButNoHitboxes;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("Cassius");
	addMove(move);
	
	// Sin's only moves that make him airborne are R.T.L. and Leaps.
	// All of them give him airdash.
	// This means Sin does not care about super jump install.
	charDoesNotCareAboutSuperJumpInstalls[CHARACTER_TYPE_SIN] = true;
	
	move = MoveInfo(CHARACTER_TYPE_SIN, "Tatakitsuke");
	move.displayName = assignName("Bull Bash");
	move.displayNameSelector = displayNameSelector_bullBash;
	move.dontShowPowerupGraphic = alwaysTrue;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SIN, "TobiagariA");
	move.displayName = assignName("P Leap");
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SIN, "TobiagariB");
	move.displayName = assignName("K Leap");
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SIN, "EatMeat");
	move.displayName = assignName("Still Growing", "Eat");
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.powerup = powerup_eatMeat;
	move.ignoreJumpInstalls = true;
	move.canYrcProjectile = canYrcProjectile_eatMeat;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SIN, "EatMeat_Okawari");
	move.displayName = assignName("Mash Still Growing", "Mash Eat");
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.powerup = powerup_eatMeat;
	move.ignoreJumpInstalls = true;
	move.canYrcProjectile = canYrcProjectile_eatMeatOkawari;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SIN, "Tatakiage");
	move.displayName = assignName("Vulture Seize", "Vulture");
	move.displayNameSelector = displayNameSelector_vultureSeize;
	move.dontShowPowerupGraphic = alwaysTrue;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SIN, "BeakDriver");
	move.displayName = assignName("Beak Driver", "Beak");
	move.displayNameSelector = displayNameSelector_beakDriver;
	move.isInVariableStartupSection = isInVariableStartupSection_beakDriver;
	move.sectionSeparator = sectionSeparator_beakDriver;
	move.powerup = powerup_beakDriver;
	move.dontShowPowerupGraphic = dontShowPowerupGraphic_beakDriver;
	move.charge = charge_beakDriver;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SIN, "BeakDriver_Air");
	move.displayName = assignName("Aerial Beak Driver", "Air Beak");
	move.displayNameSelector = displayNameSelector_beakDriverAir;
	move.dontShowPowerupGraphic = alwaysTrue;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SIN, "RideTheLightning");
	move.displayName = assignName("R.T.L");
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = isInVariableStartupSection_sinRTL;
	move.iKnowExactlyWhenTheRecoveryOfThisMoveIs = isRecovery_sinRTL;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SIN, "RideTheLightningBurst");
	move.displayName = assignName("Burst R.T.L");
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = isInVariableStartupSection_sinRTL;
	move.iKnowExactlyWhenTheRecoveryOfThisMoveIs = isRecovery_sinRTL;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SIN, "AirRideTheLightning");
	move.displayName = assignName("Air R.T.L");
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = isInVariableStartupSection_sinRTL;
	move.iKnowExactlyWhenTheRecoveryOfThisMoveIs = isRecovery_sinRTL;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SIN, "AirRideTheLightningBurst");
	move.displayName = assignName("Air Burst R.T.L");
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = isInVariableStartupSection_sinRTL;
	move.iKnowExactlyWhenTheRecoveryOfThisMoveIs = isRecovery_sinRTL;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SIN, "Ashibarai");
	move.displayName = assignName("Elk Hunt", "Elk");
	move.displayNameSelector = displayNameSelector_elkHunt;
	move.dontShowPowerupGraphic = alwaysTrue;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SIN, "UkaseWaza");
	move.displayName = assignName("Hawk Baker");
	move.displayNameSelector = displayNameSelector_hawkBaker;
	move.dontShowPowerupGraphic = alwaysTrue;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SIN, "BeakDriver_Renda");
	move.displayName = assignName("I'm Sure I'll Hit Something", "Beak Mash");
	move.displayNameSelector = displayNameSelector_beakDriverMash;
	move.dontShowPowerupGraphic = alwaysTrue;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SIN, "PhantomBarrel_Land");
	move.displayName = assignName("Voltec Dein", "Voltec");
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_voltecDein;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SIN, "PhantomBarrel_Air");
	move.displayName = assignName("Air Voltec Dein", "Air Voltec");
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_voltecDein;
	move.isMove = true;
	addMove(move);
	
	rememberFramebarId(VoltecDein_framebarId);

	move = MoveInfo(CHARACTER_TYPE_SIN, "SuperShotStart", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = VoltecDein_framebarId;
	move.framebarName = assignName("Voltec Dein", "Voltec");
	move.framebarNameUncombined = assignName("Voltec Dein Spawner", "Voltec Spawner");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SIN, "Shot_Land", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = VoltecDein_framebarId;
	move.framebarName = assignName("Voltec Dein", "Voltec");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SIN, "SuperShotAirStart", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = VoltecDein_framebarId;
	move.framebarName = assignName("Voltec Dein", "Voltec");
	move.framebarNameUncombined = assignName("Voltec Dein Spawner", "Voltec Spawner");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_SIN, "Shot_Air", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = VoltecDein_framebarId;
	move.framebarName = assignName("Voltec Dein", "Voltec");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_HAEHYUN, "NmlAtk6D");
	move.displayName = assignName("6H");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_HAEHYUN, "NmlAtk5E");
	move.displayName = assignName("5D");
	move.nameIncludesInputs = true;
	move.createdProjectile = createdProjectile_kum5D;
	move.canYrcProjectile = canYrcProjectile_kum5D;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_HAEHYUN, "CrouchFDash");
	move.displayName = assignName("Crouchwalk", "3");
	move.nameIncludesInputs = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_HAEHYUN, "HomingEnergyC");
	move.displayName = assignName("S Tuning Ball", "S Fireball");
	move.canYrcProjectile = canYrcProjectile_tuningBall;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_HAEHYUN, "HomingEnergyD");
	move.displayName = assignName("H Tuning Ball", "H Fireball");
	move.canYrcProjectile = canYrcProjectile_tuningBall;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_HAEHYUN, "LandBlowAttack");
	move.displayName = assignName("Falcon Dive", "Hayabusa");
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_HAEHYUN, "AirBlowAttack");
	move.displayName = assignName("Aerial Falcon Dive", "Air Hayabusa");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_HAEHYUN, "AntiAirAttack");
	move.displayName = assignName("Four Tigers Sword", "Uncharge Shinken");
	move.isInVariableStartupSection = hasWhiffCancels;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isMove = true;
	addMove(move);
	
	// Haehyun 21[4K]
	move = MoveInfo(CHARACTER_TYPE_HAEHYUN, "LandBlow4Hasei");
	move.displayName = assignName("Falcon Dive (Reverse Ver.)", "Hayabusa (Reverse)");
	move.displayNameSelector = displayNameSelector_landBlow4Hasei;
	move.sectionSeparator = sectionSeparator_falconDive;
	move.combineWithPreviousMove = true;
	move.isInVariableStartupSection = isInVariableStartupSection_falconDive;
	move.powerup = powerup_hayabusaRev;
	move.ignoreJumpInstalls = true;
	move.charge = charge_landBlow4Hasei;
	move.isMove = true;
	addMove(move);
	
	// Haehyun 214[K]
	move = MoveInfo(CHARACTER_TYPE_HAEHYUN, "LandBlow6Hasei");
	move.displayName = assignName("Falcon Dive (Held)", "Hayabusa (Held)");
	move.displayNameSelector = displayNameSelector_landBlow6Hasei;
	move.sectionSeparator = sectionSeparator_falconDive;
	move.combineWithPreviousMove = true;
	move.isInVariableStartupSection = isInVariableStartupSection_falconDive;
	move.powerup = powerup_hayabusaHeld;
	move.ignoreJumpInstalls = true;
	move.charge = charge_landBlow6Hasei;
	move.isMove = true;
	addMove(move);
	
	// Haehyun 623[K]
	move = MoveInfo(CHARACTER_TYPE_HAEHYUN, "AntiAir6Hasei");
	move.displayName = assignName("Four Tigers Sword (Hold)", "Grampa Viper");
	move.displayNameSelector = displayNameSelector_antiAir6Hasei;
	move.combineWithPreviousMove = true;
	move.isInVariableStartupSection = isInVariableStartupSection_falconDive;
	move.powerup = powerup_grampaMax;
	move.charge = charge_antiAir6Hasei;
	move.isMove = true;
	addMove(move);
	
	// Haehyun 623[4K]
	move = MoveInfo(CHARACTER_TYPE_HAEHYUN, "AntiAir4Hasei");
	move.displayName = assignName("Four Tigers Sword (Reverse Ver.)", "Shinken");
	move.displayNameSelector = displayNameSelector_antiAir4Hasei;
	move.combineWithPreviousMove = true;
	move.sectionSeparator = sectionSeparator_fourTigersSwordRev;
	move.isInVariableStartupSection = isInVariableStartupSection_falconDive;
	move.ignoreJumpInstalls = true;
	move.powerup = powerup_antiAir4Hasei;
	move.charge = charge_antiAir4Hasei;
	move.isMove = true;
	addMove(move);
	
	// Haehyun 236236H
	move = MoveInfo(CHARACTER_TYPE_HAEHYUN, "BlackHoleAttack");
	move.displayName = assignName("Enlightened 3000 Palm Strike", "Clap Super");
	move.displayNameSelector = displayNameSelector_blackHoleAttack;
	move.sectionSeparator = sectionSeparator_blackHoleAttack;
	move.isInVariableStartupSection = isInVariableStartupSection_blackHoleAttack;
	move.ignoreJumpInstalls = true;
	move.powerup = powerup_blackHoleAttack;
	move.charge = charge_blackHoleAttack;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_HAEHYUN, "BlackHoleAttackBurst");
	move.displayName = assignName("Burst Enlightened 3000 Palm Strike", "Burst Clap");
	move.displayNameSelector = displayNameSelector_blackHoleAttackBurst;
	move.sectionSeparator = sectionSeparator_blackHoleAttack;
	move.isInVariableStartupSection = isInVariableStartupSection_blackHoleAttack;
	move.ignoreJumpInstalls = true;
	move.powerup = powerup_blackHoleAttack;
	move.charge = charge_blackHoleAttack;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_HAEHYUN, "SuperHomingEnergy");
	move.displayName = assignName("Celestial Tuning Ball", "Super Ball");
	move.dontSkipSuper = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_HAEHYUN, "VacuumAtk", true);
	move.isDangerous = isDangerous_vacuumAtk;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("Enlightened 3000 Palm Strike Vacuum", "Vacuum");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_HAEHYUN, "EnergyBall", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = generateFramebarId();
	move.framebarName = PROJECTILE_NAME_TUNING_BALL;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_HAEHYUN, "SuperEnergyBall", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = generateFramebarId();
	move.framebarName = PROJECTILE_NAME_CELESTIAL_TUNING_BALL;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_HAEHYUN, "kum_205shot", true);
	move.isDangerous = isDangerous_kum5D;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("5D");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "NmlAtk4AHasei");
	move.displayName = assignName("4P");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	// The only moves Raven has that can make him airborne are 6H and Needle.
	// Needle gives airdash. And 6H is weird, but super jump installing it does not make you have 2 airdashes, no matter what.
	// Raven has no use for super jump installs.
	charDoesNotCareAboutSuperJumpInstalls[CHARACTER_TYPE_RAVEN] = true;
	
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "Kakkuu");
	move.displayName = assignName("Glide");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "LandSettingTypeNeedle");
	move.displayName = assignName("Scharf Kugel", "Orb");
	move.canYrcProjectile = canYrcProjectile_ravenOrb;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "LandBlowAttack");
	move.displayName = assignName("Grausam Impuls", "Scratch");
	move.displayNameSelector = displayNameSelector_landBlowAttack;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "LandSlowNeedle");
	move.displayName = assignName("Schmerz Berg", "Needle");
	move.canYrcProjectile = canYrcProjectile_ravenNeedle;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "AirSettingTypeNeedle");
	move.displayName = assignName("Air Scharf Kugel", "Air Orb");
	move.canYrcProjectile = canYrcProjectile_ravenOrb;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "AirBlowAttack");
	move.displayName = assignName("Air Grausam Impuls", "j.Scratch");
	move.displayNameSelector = displayNameSelector_airBlowAttack;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "AirSlowNeedleB");
	move.displayName = assignName("K Grebechlich Licht", "Air K Needle");
	move.canYrcProjectile = canYrcProjectile_ravenNeedle;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "AirSlowNeedleA");
	move.displayName = assignName("P Grebechlich Licht", "Air P Needle");
	move.canYrcProjectile = canYrcProjectile_ravenNeedle;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "CommandThrow");
	move.displayName = assignName("H Wachen Zweig", "Command Grab");
	move.displayNameSelector = displayNameSelector_commandThrow;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "CommandThrowEx");
	move.displayName = assignName("H Wachen Zweig", "Command Grab");
	move.displayNameSelector = displayNameSelector_commandThrowEx;
	move.combineWithPreviousMove = true;
	move.ignoreJumpInstalls = true;
	move.isGrab = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "AntiAirCommandThrow");
	move.displayName = assignName("S Wachen Zweig", "S Grab");
	move.displayNameSelector = displayNameSelector_antiAirCommandThrow;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "AntiAirCommandThrowEx");
	move.displayName = assignName("S Wachen Zweig", "S Grab");
	move.displayNameSelector = displayNameSelector_antiAirCommandThrowEx;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "LandDashAttack");
	move.displayName = assignName("Verzweifelt", "Dash Super");
	move.dontSkipSuper = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "AirDashAttack");
	move.displayName = assignName("Air Verzweifelt", "Air Dash Super");
	move.dontSkipSuper = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "RevengeAttack");
	move.displayName = assignName("Getreuer", "Stab Super");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "RevengeAttackBurst");
	move.displayName = assignName("Burst Getreuer", "Burst Stab");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "RevengeAttackEx");
	move.displayName = assignName("Getreuer", "Stab Super");
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	// Raven stance when first entering it
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "ArmorDance");
	move.displayName = assignName("Give it to me HERE", "Stance");
	move.sectionSeparator = sectionSeparator_armorDance;
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.canStopHolding = canStopHolding_armorDance;
	move.powerup = powerup_armorDance;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	// Raven stance after armoring a hit in ArmorDance
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "ArmorDance2");
	move.displayName = assignName("Give it to me HERE", "Stance");
	move.sectionSeparator = sectionSeparator_armorDance;
	move.isInVariableStartupSection = isRecoveryHasGatlings_enableWhiffCancels;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.canStopHolding = canStopHolding_armorDance;
	move.powerup = powerup_armorDance;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	rememberFramebarId(SlowNeeldeObjLand_framebarId);

	move = MoveInfo(CHARACTER_TYPE_RAVEN, "SlowNeeldeObjLand", true);
	move.isDangerous = isDangerous_not_NullWhileActive;
	move.framebarId = SlowNeeldeObjLand_framebarId;
	move.framebarName = assignName("Schmerz Berg", "Needle");
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "SlowNeeldeObjAir", true);
	move.isDangerous = isDangerous_not_NullWhileActive;
	move.framebarId = SlowNeeldeObjLand_framebarId;
	move.framebarName = assignName("Grebechlich Licht", "Needle");
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	rememberFramebarId(AirSettingTypeNeedleObj_framebarId);

	move = MoveInfo(CHARACTER_TYPE_RAVEN, "AirSettingTypeNeedleObj", true);
	move.isDangerous = isDangerous_not_NullWhileActive;
	move.framebarId = AirSettingTypeNeedleObj_framebarId;
	move.framebarName = assignName("Scharf Kugel", "Orb");
	move.framebarNameSelector = framebarNameSelector_landSettingTypeNeedleObj;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_RAVEN, "LandSettingTypeNeedleObj", true);
	move.isDangerous = isDangerous_not_NullWhileActive;
	move.framebarId = AirSettingTypeNeedleObj_framebarId;
	move.framebarName = assignName("Scharf Kugel", "Orb");
	move.framebarNameSelector = framebarNameSelector_landSettingTypeNeedleObj;
	addMove(move);
	
	// Dizzy does not have a single special or super that can put her into the air,
	// so she never cares about super jump installs.
	// And if some move only leads to a special cancel, then regular jump installs don't matter on that move either
	charDoesNotCareAboutSuperJumpInstalls[CHARACTER_TYPE_DIZZY] = true;
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "NmlAtk2E");
	move.displayName = assignName("2D");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;  // only leads to specials
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "NmlAtk6C");
	move.displayName = assignName("4S");
	move.nameIncludesInputs = true;
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "NmlAtk6D");
	move.displayName = assignName("6H");
	move.displayNameSelector = displayNameSelector_dizzy6H;
	move.nameIncludesInputs = true;
	move.sectionSeparator = sectionSeparator_dizzy6H;
	move.isInVariableStartupSection = isInVariableStartupSection_dizzy6H;
	move.ignoreJumpInstalls = true;  // only leads to specials
	move.powerup = powerup_dizzy6H;
	move.charge = charge_dizzy6H;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "GammaRay");
	move.displayName = assignName("Gamma Ray");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "Sakana");
	move.displayName = assignName("I used this to catch fish", "Ice Spike");
	move.ignoreJumpInstalls = true;
	move.canYrcProjectile = canYrcProjectile_iceSpike;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "SakanaNecro");
	move.displayName = assignName("For searing cod...", "Fire Pillar");
	move.ignoreJumpInstalls = true;
	move.canYrcProjectile = canYrcProjectile_firePillar;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "Akari");
	move.displayName = assignName("The light was so small in the beginning", "Fire Scythe");
	move.ignoreJumpInstalls = true;
	move.canYrcProjectile = canYrcProjectile_fireScythe;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "AkariUndine");
	move.displayName = assignName("For putting out the light...", "Ice Scythe");
	move.ignoreJumpInstalls = true;
	move.canYrcProjectile = canYrcProjectile_iceScythe;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "HanashiD");
	move.displayName = assignName("H We fought a lot together", "H Fish");
	move.canYrcProjectile = canYrcProjectile_fish;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "HanashiD_Air");
	move.displayName = assignName("H Air We fought a lot together...", "Air H Fish");
	move.canYrcProjectile = canYrcProjectile_fish;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "HanashiC");
	move.displayName = assignName("S We fought a lot together", "S Fish");
	move.canYrcProjectile = canYrcProjectile_fish;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "HanashiC_Air");
	move.displayName = assignName("S Air We fought a lot together", "Air S Fish");
	move.canYrcProjectile = canYrcProjectile_fish;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "HanashiB");
	move.displayName = assignName("K We talked a lot together", "K Fish");
	move.canYrcProjectile = canYrcProjectile_fish;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "HanashiB_Air");
	move.displayName = assignName("K Air We talked a lot together", "Air K Fish");
	move.canYrcProjectile = canYrcProjectile_fish;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "HanashiA");
	move.displayName = assignName("P We talked a lot together", "P Fish");
	move.canYrcProjectile = canYrcProjectile_fish;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "HanashiA_Air");
	move.displayName = assignName("P Air We talked a lot together", "Air P Fish");
	move.canYrcProjectile = canYrcProjectile_fish;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "HanashiE");
	move.displayName = assignName("D We fought a lot together", "Shield Fish");
	move.canYrcProjectile = canYrcProjectile_fish;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);

	move = MoveInfo(CHARACTER_TYPE_DIZZY, "HanashiE_Air");
	move.displayName = assignName("D Air We fought a lot together", "Air Shield Fish");
	move.canYrcProjectile = canYrcProjectile_fish;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "AwaP");
	move.displayName = assignName("Please, leave me alone", "Bubble");
	move.canYrcProjectile = canYrcProjectile_bubble;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "AwaK");
	move.displayName = assignName("What happens when I'm TOO alone", "Fire Bubble");
	move.canYrcProjectile = canYrcProjectile_fireBubble;
	move.isMove = true;
	addMove(move);
	
	// Dizzy 421H
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "KinomiNecro");
	move.displayName = assignName("For roasting chestnuts...", "Fire Spears");
	move.displayNameSelector = displayNameSelector_kinomiNecro;
	move.sectionSeparator = sectionSeparator_kinomiNecro;
	move.isInVariableStartupSection = isInVariableStartupSection_kinomiNecro;
	move.powerup = powerup_fireSpear;
	move.canYrcProjectile = canYrcProjectile_fireSpears;
	move.ignoreJumpInstalls = true;
	move.charge = charge_kinomiNecro;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "Kinomi");
	move.displayName = assignName("I use this to pick fruit", "Ice Spear");
	move.canYrcProjectile = canYrcProjectile_iceSpear;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "ImperialRay");
	move.displayName = assignName("Imperial Ray");
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_imperialRay;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "ImperialRayBurst");
	move.displayName = assignName("Burst Imperial Ray");
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_imperialRay;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "KirikaeshiKakusei");
	move.displayName = assignName("Don't be overprotective", "Mirror");
	move.dontSkipSuper = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "SakanaObj", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = generateFramebarId();
	move.framebarNameSelector = nameSelector_iceSpike;
	move.framebarNameUncombinedSelector = nameSelectorUncombined_iceSpike;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "AkariObj", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = generateFramebarId();
	move.framebarNameSelector = nameSelector_iceScythe;
	move.framebarNameUncombinedSelector = nameSelectorUncombined_iceScythe;
	addMove(move);
	
	rememberFramebarId(DizzyBubble_framebarId);

	move = MoveInfo(CHARACTER_TYPE_DIZZY, "AwaPObj", true);
	move.isDangerous = isDangerous_bubble;
	move.framebarId = DizzyBubble_framebarId;
	move.framebarName = assignName("Please, leave me alone", "Bubble");
	move.framebarNameUncombined = assignName("Bubble");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "AwaKObj", true);
	move.isDangerous = isDangerous_bubble;
	move.framebarId = DizzyBubble_framebarId;
	move.framebarName = assignName("What happens when I'm TOO alone", "Fire Bubble");
	move.framebarNameUncombined = assignName("Fire Bubble");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "KinomiObj", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("I use this to pick fruit", "Ice Spear");
	move.framebarNameUncombined = assignName("Ice Spear");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "KinomiObjNecro", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = KINOMI_OBJ_NECRO_FRAMEBAR_ID;
	move.framebarName = assignName("For roasting chestnuts...", "Fire Spears");
	move.framebarNameUncombined = assignName("Fire Spear 1");
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "KinomiObjNecro2", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = KINOMI_OBJ_NECRO_FRAMEBAR_ID;
	move.framebarName = assignName("For roasting chestnuts...", "Fire Spears");
	move.framebarNameUncombined = assignName("Fire Spear 2");
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "KinomiObjNecro3", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = KINOMI_OBJ_NECRO_FRAMEBAR_ID;
	move.framebarName = assignName("For roasting chestnuts...", "Fire Spears");
	move.framebarNameUncombined = assignName("Fire Spear 3");
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "KinomiObjNecrobomb", true);
	move.isDangerous = isDangerous_not_hasHitNumButInactive;
	move.framebarId = KINOMI_OBJ_NECRO_FRAMEBAR_ID;
	move.framebarName = assignName("For roasting chestnuts...", "Fire Spears");
	move.framebarNameUncombined = assignName("Fire Spear Explosion");
	addMove(move);
	
	rememberFramebarId(DizzyFish_framebarId);

	// P fish
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "HanashiObjA", true);
	move.isDangerous = isDangerous_pFish;
	move.framebarId = DizzyFish_framebarId;
	move.framebarName = assignName("P Blue Fish");
	move.framebarNameUncombined = assignName("P Blue Fish");
	move.showMultipleHitsFromOneAttack = true;
	addMove(move);
	
	// K fish
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "HanashiObjB", true);
	move.isDangerous = isDangerous_kFish;
	move.framebarId = DizzyFish_framebarId;
	move.framebarName = assignName("K Blue Fish");
	move.framebarNameUncombined = assignName("K Blue Fish");
	move.showMultipleHitsFromOneAttack = true;
	addMove(move);
	
	// S laser fish
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "HanashiObjD", true);
	move.isDangerous = isDangerous_laserFish;
	move.framebarId = DizzyFish_framebarId;
	move.framebarName = assignName("We fought a lot together Fish", "Laser Fish");
	move.framebarNameUncombined = assignName("H Laser Fish");
	addMove(move);
	
	// H laser fish
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "HanashiObjC", true);
	move.isDangerous = isDangerous_laserFish;
	move.framebarId = DizzyFish_framebarId;
	move.framebarName = assignName("We fought a lot together Fish", "Laser Fish");
	move.framebarNameUncombined = assignName("S Laser Fish");
	addMove(move);
	
	// H/S laser fish's laser
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "Laser", true);
	move.isDangerous = isDangerous_notNull;
	move.framebarId = DizzyFish_framebarId;
	move.framebarName = assignName("We fought a lot together Fish", "Laser Fish");
	move.framebarNameUncombined = assignName("Laser");
	addMove(move);
	
	// Shield fish
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "HanashiObjE", true);
	move.isDangerous = isDangerous_dFish;
	move.framebarId = DizzyFish_framebarId;
	move.framebarName = assignName("We fought a lot together Shield Fish", "Shield Fish");
	move.framebarNameUncombined = assignName("Shield Fish");
	addMove(move);
	
	rememberFramebarId(ImperialRay_framebarId);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "ImperialRayCreater", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = ImperialRay_framebarId;
	move.framebarName = assignName("Imperial Ray");
	move.framebarNameUncombined = assignName("Imperial Ray Spawner");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "ImperialRayBakuhatsu", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = ImperialRay_framebarId;
	move.framebarName = assignName("Imperial Ray");
	move.combineHitsFromDifferentProjectiles = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "GammaRayLaser", true);
	move.isDangerous = isDangerous_notNull;
	move.framebarId = GAMMA_RAY_LASER_FRAMEBAR_ID;
	move.framebarName = assignName("Gamma Ray");
	move.framebarNameUncombined = assignName("Gamma Ray Laser");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_DIZZY, "GammaRayLaserMax", true);
	move.isDangerous = isDangerous_not_NullWhileActive;
	move.framebarId = GAMMA_RAY_LASER_FRAMEBAR_ID;
	move.framebarName = assignName("Gamma Ray");
	move.framebarNameUncombined = assignName("Gamma Ray Max Laser");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "Tossin");
	move.displayName = assignName("Rokkon Sogi");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "Tetsuzansen");
	move.displayName = assignName("Tetsuzan Sen");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "CommandThrow");
	move.displayName = assignName("Himawari");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "CommandThrowExe");
	move.displayName = assignName("Himawari");
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "AirGCAntiGroundC");
	move.displayName = assignName("S Kikyo");
	move.combineWithPreviousMove = true;
	move.splitForComboRecipe = true;
	move.usePlusSignInCombination = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "AirGCAntiGroundD");
	move.displayName = assignName("H Kikyo");
	move.combineWithPreviousMove = true;
	move.splitForComboRecipe = true;
	move.usePlusSignInCombination = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "AirGCAntiGroundCGuard");
	move.displayName = assignName("S Kikyo");
	move.combineWithPreviousMove = true;
	move.splitForComboRecipe = true;
	move.usePlusSignInCombination = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "AirGCAntiGroundDGuard");
	move.displayName = assignName("H Kikyo");
	move.combineWithPreviousMove = true;
	move.splitForComboRecipe = true;
	move.usePlusSignInCombination = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "AirGCAntiAirA");
	move.displayName = assignName("P Tsubaki");
	move.combineWithPreviousMove = true;
	move.splitForComboRecipe = true;
	move.usePlusSignInCombination = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "AirGCAntiAirAGuard");
	move.displayName = assignName("P Tsubaki");
	move.combineWithPreviousMove = true;
	move.splitForComboRecipe = true;
	move.usePlusSignInCombination = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "AirGCAntiAirB");
	move.displayName = assignName("K Tsubaki");
	move.combineWithPreviousMove = true;
	move.splitForComboRecipe = true;
	move.usePlusSignInCombination = true;
	move.isMove = true;
	addMove(move);

	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "AirGCAntiAirBGuard");
	move.displayName = assignName("K Tsubaki");
	move.combineWithPreviousMove = true;
	move.splitForComboRecipe = true;
	move.usePlusSignInCombination = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "TatamiLand");
	move.displayName = assignName("Tatami Gaeshi", "Tatami");
	move.canYrcProjectile = canYrcProjectile_tatami;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "TatamiAir");
	move.displayName = assignName("Air Tatami Gaeshi", "Air Tatami");
	move.canYrcProjectile = canYrcProjectile_tatami;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "YouZanSen");
	move.displayName = assignName("Yozan Sen");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "Kabari");
	move.displayName = assignName("H Kabari");
	move.ignoreJumpInstalls = true;  // Kabaris only lead to moves that end up on the ground
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "KabariAntiAir");
	move.displayName = assignName("S Kabari");
	move.ignoreJumpInstalls = true;  // Kabaris only lead to moves that end up on the ground
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "BlockingKakusei");
	move.displayName = assignName("Metsudo Kushodo");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "BlockingKakuseiExe");
	move.displayName = assignName("Metsudo Kushodo Connect");
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	move.forceLandingRecovery = true;
	move.iKnowExactlyWhenTheRecoveryOfThisMoveIs = isRecovery_land;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "TsuraneSanzuWatashi");
	move.displayName = assignName("Tsurane Sanzu-watashi");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "TsuraneSanzuWatashiBurst");
	move.displayName = assignName("Burst Tsurane Sanzu-watashi");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "TsuraneSanzuWatashiExe");
	move.displayName = assignName("Tsurane Sanzu-watashi");
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
	move.isMove = true;
	addMove(move);
	
	// Baiken Azami
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "BlockingStand");
	move.displayName = assignName("Standing Azami");
	move.displayNameSelector = displayNameSelector_standingAzami;
	move.sectionSeparator = sectionSeparator_azami;
	move.canBlock = canBlock_default;
	move.ignoresHitstop = true;
	move.isInVariableStartupSection = hasWhiffCancelsAndCantBlock;
	move.considerVariableStartupAsStanceForFramebar = true;
	addMove(move);
	
	// Baiken Azami
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "BlockingCrouch");
	move.displayName = assignName("Crouching Azami");
	move.displayNameSelector = displayNameSelector_crouchingAzami;
	move.sectionSeparator = sectionSeparator_azami;
	move.canBlock = canBlock_default;
	move.ignoresHitstop = true;
	move.isInVariableStartupSection = hasWhiffCancelsAndCantBlock;
	move.considerVariableStartupAsStanceForFramebar = true;
	addMove(move);
	
	// Baiken Azami
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "BlockingAir");
	move.displayName = assignName("Aerial Azami");
	move.displayNameSelector = displayNameSelector_airAzami;
	move.sectionSeparator = sectionSeparator_azami;
	move.canBlock = canBlock_default;
	move.ignoresHitstop = true;
	move.isInVariableStartupSection = hasWhiffCancelsAndCantBlock;
	move.considerVariableStartupAsStanceForFramebar = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "Youshijin");   // P followup
	move.displayName = assignName("Kuchinashi");
	move.combineWithPreviousMove = true;
	move.splitForComboRecipe = true;
	move.usePlusSignInCombination = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "Mawarikomi");   // K followup
	move.displayName = assignName("Mawari-komi");
	move.combineWithPreviousMove = true;
	move.splitForComboRecipe = true;
	move.usePlusSignInCombination = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "Sakura");  // S followup
	move.displayName = assignName("Sakura");
	move.combineWithPreviousMove = true;
	move.splitForComboRecipe = true;
	move.usePlusSignInCombination = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "Issen");  // H followup
	move.displayName = assignName("Rokkon Sogi");
	move.combineWithPreviousMove = true;
	move.splitForComboRecipe = true;
	move.usePlusSignInCombination = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "Teppou");  // D followup
	move.displayName = assignName("Yasha Gatana");
	move.combineWithPreviousMove = true;
	move.splitForComboRecipe = true;
	move.usePlusSignInCombination = true;
	move.canYrcProjectile = canYrcProjectile_teppou;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "NmlAtk5E");
	move.displayName = assignName("5D");
	move.nameIncludesInputs = true;
	move.createdProjectile = createdProjectile_baiken5D;
	move.canYrcProjectile = canYrcProjectile_baiken5D;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "NmlAtk5EShotObj", true);
	move.isDangerous = isDangerous_playerInRCOrHasActiveFlag_AndNotInRecovery;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("5D");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "NmlAtkAir5E");
	move.displayName = assignName("j.D");
	move.nameIncludesInputs = true;
	move.canYrcProjectile = canYrcProjectile_baikenJD;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "NmlAtkAir5EShotObj", true);
	move.isDangerous = isDangerous_playerInRCOrHasActiveFlag_AndNotInRecovery;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("j.D");
	move.framebarNameUncombined = assignName("j.D Projectile");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "TeppouObj", true);
	move.isDangerous = isDangerous_not_hasHitNumButNoHitboxes;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("Yasha Gatana");
	move.framebarNameUncombined = assignName("Yasha Gatana Projectile");
	addMove(move);
	
	rememberFramebarId(Tatami_framebarId);

	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "TatamiLandObj", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = Tatami_framebarId;
	move.framebarName = assignName("Tatami");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_BAIKEN, "TatamiAirObj", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = Tatami_framebarId;
	move.framebarName = assignName("Tatami");
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	// The only move that puts Jack-O in the air and does not give her an airdash is Zest.
	// If not for that, she would not care about super jump installs at all.
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "NmlAtk5D");
	move.displayName = assignName("3H");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "NmlAtk6B");
	move.displayName = assignName("6K");
	move.nameIncludesInputs = true;
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "DustAtk");
	move.displayName = assignName("5D");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "IronballGenocide");
	move.displayName = assignName("4D");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "IronballGenocideEx");
	move.displayName = assignName("4D");
	move.displayNameSelector = displayNameSelector_jacko4D;
	move.nameIncludesInputs = true;
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "IronballGenocideEx_Weak");
	move.displayName = assignName("4D (Weak)");
	move.nameIncludesInputs = true;
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "CommandThorw");
	move.displayName = assignName("6D");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "Zest");
	move.displayName = assignName("2D");
	move.nameIncludesInputs = true;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "UntieKiron'sChain");
	move.displayName = assignName("j.D");
	move.nameIncludesInputs = true;
	move.canYrcProjectile = canYrcProjectile_jackoJD;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "AirIronballGenocide");
	move.displayName = assignName("j.4D");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "AirIronballGenocideEx");
	move.displayName = assignName("j.4D");
	move.displayNameSelector = displayNameSelector_jackoj4D;
	move.nameIncludesInputs = true;
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "AirIronballGenocideEx_Weak");
	move.displayName = assignName("j.4D (Weak)");
	move.nameIncludesInputs = true;
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "AirCommandThorw");
	move.displayName = assignName("j.6D");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "CommandThorwEx");
	move.displayName = assignName("6D/j.6D");
	move.nameIncludesInputs = true;
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "ThorwGhost");
	move.displayName = assignName("Throw Ghost");
	move.createdProjectile = createdProjectile_ThrowGhost;
	move.canYrcProjectile = canYrcProjectile_ThrowGhost;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "AirThorwGhost");
	move.displayName = assignName("Air Throw Ghost");
	move.createdProjectile = createdProjectile_AirThrowGhost;
	move.canYrcProjectile = canYrcProjectile_AirThrowGhost;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "PickUpGhost");
	move.displayName = assignName("Pick Up Ghost");
	move.ignoreJumpInstalls = true;
	move.powerup = powerup_pickUpGhost;
	move.dontShowPowerupGraphic = alwaysTrue;
	move.canYrcProjectile = canYrcProjectile_pickUpGhost;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "PutGhost");
	move.displayName = assignName("Put Ghost Back Down");
	move.ignoreJumpInstalls = true;
	move.powerup = powerup_putGhost;
	move.dontShowPowerupGraphic = alwaysTrue;
	move.canYrcProjectile = canYrcProjectile_putGhost;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "ReturnGhost");
	move.displayName = assignName("Put Away Ghost");
	move.ignoreJumpInstalls = true;
	move.powerup = powerup_returnGhost;
	move.dontShowPowerupGraphic = alwaysTrue;
	move.canYrcProjectile = canYrcProjectile_returnGhost;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "AirZest");
	move.displayName = assignName("j.2D");
	move.nameIncludesInputs = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "AirReturnGhost");
	move.displayName = assignName("Air Put Away Ghost");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "SummonGhostA");
	move.displayName = assignName("Set P Ghost");
	move.createdProjectile = createdProjectile_PGhost;
	move.canYrcProjectile = canYrcProjectile_PGhost;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "SummonGhostB");
	move.displayName = assignName("Set K Ghost");
	move.createdProjectile = createdProjectile_KGhost;
	move.canYrcProjectile = canYrcProjectile_KGhost;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "SummonGhostC");
	move.displayName = assignName("Set S Ghost");
	move.createdProjectile = createdProjectile_SGhost;
	move.canYrcProjectile = canYrcProjectile_SGhost;
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "OrganOpen");
	move.displayName = assignName("Organ Deployment", "Organ");
	move.sectionSeparator = sectionSeparator_organ;
	move.considerVariableStartupAsStanceForFramebar = true;
	move.isInVariableStartupSection = isInVariableStartupSection_organOpen;
	move.ignoreJumpInstalls = true;
	move.canYrcProjectile = canYrcProjectile_organ;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "Calvados");
	move.displayName = assignName("Calvados");
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_jackoCalvados;
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "CalvadosBurst");
	move.displayName = assignName("Burst Calvados");
	move.dontSkipSuper = true;
	move.canYrcProjectile = canYrcProjectile_jackoCalvados;
	move.ignoreSuperJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "ScrewPileDriver");
	move.displayName = assignName("Forever Elysion Driver", "Supergrab");
	move.ignoreJumpInstalls = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "AirScrewPileDriver");
	move.displayName = assignName("Air Forever Elysion Driver", "Air Supergrab");
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "ScrewPileDriverEx");
	move.displayName = assignName("Air/Ground Forever Elysion Driver", "Air/Ground Supergrab");
	move.combineWithPreviousMove = true;
	move.isGrab = true;
	move.ignoreJumpInstalls = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "AirCalvados");
	move.displayName = assignName("Air Calvados");
	move.dontSkipSuper = true;
	move.isMove = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "AirCalvadosBurst");
	move.displayName = assignName("Air Burst Calvados");
	move.dontSkipSuper = true;
	move.isMove = true;
	addMove(move);
	
	rememberFramebarId(ServantAAndB_framebarId);

	move = MoveInfo(CHARACTER_TYPE_JACKO, "ServantA", true);
	move.isDangerous = isDangerous_servant;
	move.framebarId = ServantAAndB_framebarId;
	move.framebarName = assignName("Sword/Spear men");
	move.framebarNameUncombined = assignName("Knight");
	move.combineHitsFromDifferentProjectiles = true;  // we need this because we don't want two knights attacking simultaneously displayed as two hits
	move.showMultipleHitsFromOneAttack = true;  // we need this because it's the same guy attacking over and over
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "ServantB", true);
	move.isDangerous = isDangerous_servant;
	move.framebarId = ServantAAndB_framebarId;
	move.framebarName = assignName("Sword/Spear men");
	move.framebarNameUncombined = assignName("Lancer");
	move.combineHitsFromDifferentProjectiles = true;
	move.showMultipleHitsFromOneAttack = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "ServantC", true);
	move.isDangerous = isDangerous_servant;
	move.framebarId = MAGICIAN_FRAMEBAR_ID;
	move.framebarName = assignName("Magicians");
	move.framebarNameUncombined = assignName("Magician");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "magicAtkLv1", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = MAGICIAN_FRAMEBAR_ID;
	move.framebarName = assignName("Magicians");
	move.framebarNameUncombined = assignName("Magician Lv1");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "magicAtkLv2", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = MAGICIAN_FRAMEBAR_ID;
	move.framebarName = assignName("Magicians");
	move.framebarNameUncombined = assignName("Magician Lv2");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "magicAtkLv3", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = MAGICIAN_FRAMEBAR_ID;
	move.framebarName = assignName("Magicians");
	move.framebarNameUncombined = assignName("Magician Lv3");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "Fireball", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = generateFramebarId();
	move.framebarName = assignName("j.D");
	move.framebarNameUncombined = assignName("j.D Fireball");
	move.combineHitsFromDifferentProjectiles = true;
	addMove(move);
	
	rememberFramebarId(CalvadosObj_framebarId);

	move = MoveInfo(CHARACTER_TYPE_JACKO, "CalvadosObj", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = CalvadosObj_framebarId;
	move.framebarName = assignName("Calvados");
	move.framebarNameUncombined = assignName("Calvados Ball");
	addMove(move);
	
	// Only the boss version spawns this
	move = MoveInfo(CHARACTER_TYPE_JACKO, "CalvadosObj2", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = CalvadosObj_framebarId;
	move.framebarName = assignName("Calvados");
	move.framebarNameUncombined = assignName("Calvados Ball 2");
	addMove(move);
	
	// Only the boss version spawns this
	move = MoveInfo(CHARACTER_TYPE_JACKO, "CalvadosObj3", true);
	move.isDangerous = isDangerous_alwaysTrue;
	move.framebarId = CalvadosObj_framebarId;
	move.framebarName = assignName("Calvados");
	move.framebarNameUncombined = assignName("Calvados Ball 3");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "Bomb", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = CalvadosObj_framebarId;
	move.framebarName = assignName("Calvados");
	move.framebarNameUncombined = assignName("Calvados Explosion Pillar");
	addMove(move);
	
	rememberFramebarId(Ghosts_framebarId);

	move = MoveInfo(CHARACTER_TYPE_JACKO, "GhostA", true);
	move.isDangerous = isDangerous_displayModel;
	move.framebarId = Ghosts_framebarId;
	move.framebarName = PROJECTILE_NAME_GHOST;
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "GhostB", true);
	move.isDangerous = isDangerous_displayModel;
	move.framebarId = Ghosts_framebarId;
	move.framebarName = PROJECTILE_NAME_GHOST;
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "GhostC", true);
	move.isDangerous = isDangerous_displayModel;
	move.framebarId = Ghosts_framebarId;
	move.framebarName = PROJECTILE_NAME_GHOST;
	move.drawProjectileOriginPoint = true;
	addMove(move);
	
	rememberFramebarId(Suicidal_explosion_framebarId);

	move = MoveInfo(CHARACTER_TYPE_JACKO, "Suicidal_explosion", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = Suicidal_explosion_framebarId;
	move.framebarName = assignName("Explosion");
	move.framebarNameUncombined = assignName("Explosion1");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "Suicidal_explosion2", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = Suicidal_explosion_framebarId;
	move.framebarName = assignName("Explosion");
	move.framebarNameUncombined = assignName("Explosion2");
	addMove(move);
	
	move = MoveInfo(CHARACTER_TYPE_JACKO, "Suicidal_explosion3", true);
	move.isDangerous = isDangerous_notInRecovery;
	move.framebarId = Suicidal_explosion_framebarId;
	move.framebarName = assignName("Explosion");
	move.framebarNameUncombined = assignName("Explosion3");
	addMove(move);
	
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
	venomQvAClearUponAfterExitOffset = 0;
	venomQvBClearUponAfterExitOffset = 0;
	venomQvCClearUponAfterExitOffset = 0;
	venomQvDClearUponAfterExitOffset = 0;
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
	ghostABecomePickedUp = 0;
	ghostBBecomePickedUp = 0;
	ghostCBecomePickedUp = 0;
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
	baikenJDcreation = 0;
	for (ForceAddedWhiffCancel& cancel : forceAddWhiffCancels) {
		cancel.clearCachedValues();
	}
	milliaSecretGardenUnlink = 0;
	milliaSecretGardenUnlinkFailedToFind = false;
	elpheltRifleFireStartup = 0;
	elpheltRifleFirePowerupStartup = 0;
	bedmanBoomerangASeal.clear();
	bedmanBoomerangAAirSeal.clear();
	bedmanBoomerangBSeal.clear();
	bedmanBoomerangBAirSeal.clear();
	bedmanTaskBSeal.clear();
	bedmanAirTaskBSeal.clear();
	bedmanGroundTaskCSealOffset.clear();
	bedmanAirTaskCSeal.clear();
	bedmanDejavuAStartup = 0;
	bedmanDejavuBStartup = 0;
	bedmanDejavuCStartup = 0;
	bedmanDejavuDStartup = 0;
	venomBallSeiseiABallCreation = 0;
	venomBallSeiseiBBallCreation = 0;
	venomBallSeiseiCBallCreation = 0;
	venomBallSeiseiDBallCreation = 0;
	venomAirBallSeiseiABallCreation = 0;
	venomAirBallSeiseiBBallCreation = 0;
	venomAirBallSeiseiCBallCreation = 0;
	venomAirBallSeiseiDBallCreation = 0;
	ramlethalCreateBitLaserMinion = 0;
	ramlethalBitLaserMinionBossStartMarker = 0;
	ramlethalBitLaserMinionNonBossCreateLaser = 0;
	ramlethalBitLaserMinionBossCreateLaser = 0;
	sinEatMeatPowerup = 0;
	sinEatMeatOkawariPowerup = 0;
	jackoPutGhost = 0;
	jackoReturnGhost = 0;
	rsfStartStateLinkBreak = 0;
	jackoOrganP = JackoOrgan { 0, 0 };
	jackoOrganK = JackoOrgan { 0, 0 };
	jackoOrganS = JackoOrgan { 0, 0 };
	jackoOrganH = JackoOrgan { 0, 0 };
	jamCardPowerup[0] = 0;
	jamCardPowerup[1] = 0;
	jamCardPowerup[2] = 0;
	dizzyAwaP = 0;
	dizzyAwaK = 0;
	faustItemToss = 0;
	faustPogoItemToss = 0;
	may6PElements.clear();
	faustFastToss = 0;
	faustFastTossGoto = 0;
	for (VenomQvChargeElement& elem : venomQvCharges) {
		elem.clear();
	}
	for (std::vector<int>& elem : venomStingerChargeLevels) {
		elem.clear();
	}
	sinBeakDriverMinCharge = 0;
	sinBeakDriverMaxCharge = 0;
	elpheltCrouch2StandChargeDuration = 0;
	leoSemukeFrontWalkStart = 0;
	leoSemukeFrontWalkEnd = 0;
	leoSemukeBackWalkStart = 0;
	leoSemukeBackWalkEnd = 0;
	leoSemukeEnd = 0;
	leoTossin2FrontEnd = 0;
	leoTossin2HaseiFrontEnd = 0;
	leo5HKamae = 0;
	leo6HKamae = 0;
	leoGorengekiKamae = 0;
	johnnyTreasureHuntMaxCharge = 0;
	johnnyTreasureHuntMinCharge = 0;
	johnnyStepTreasureHuntMinCharge = 0;
	johnnyStepTreasureHuntMaxCharge = 0;
	jamKCardLongHold = 0;
	jamSCardLongHold = 0;
	jamHCardLongHold = 0;
	haehyunAntiAir6HaseiMaxChargeLower = 0;
	haehyunAntiAir6HaseiMaxChargeUpper = 0;
	haehyunLandBlow6HaseiMaxChargeLower = 0;
	haehyunLandBlow6HaseiMaxChargeUpper = 0;
	haehyunLandBlow6HaseiAttack2 = 0;
	haehyunAntiAir6HaseiLanding = 0;
	haehyunAntiAir6HaseiHeavyAttack = 0;
	haehyunLandBlow4HaseiAttack2 = 0;
	haehyunLandBlow4HaseiMaxChargeLower = 0;
	haehyunLandBlow4HaseiMaxChargeUpper = 0;
	haehyunBlackHoleAttackMaxCharge = 0;
	reavenLandSettingTypeNeedleObjIsRev2 = TRIBOOL_DUNNO;
	dizzy6HMinCharge = 0;
	dizzy6HMaxCharge = 0;
	dizzyKinomiNecroMinCharge = 0;
	dizzyKinomiNecroSpear2 = 0;
	dizzyKinomiNecroSpear3 = 0;
	dizzyKinomiNecroEndOffset = 0;
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

bool Moves::getInfoWithName(MoveInfo& returnValue, CharacterType charType, const char* moveName, const char* stateName, bool isEffect, const char** outNamePtr, void const** sortValue) {
	if (moveName && *moveName != '\0') {
		if (getInfoWithName(returnValue, charType, moveName, isEffect, outNamePtr, sortValue)) {
			return true;
		}
	}
	if (stateName && *stateName != '\0') {
		if (getInfoWithName(returnValue, charType, stateName, isEffect, outNamePtr, sortValue)) {
			return true;
		}
	}
	returnValue = defaultMove;
	if (outNamePtr) *outNamePtr = nullptr;
	if (sortValue) *sortValue = nullptr;
	return false;
}
bool Moves::getInfo(MoveInfo& returnValue, CharacterType charType, const char* name, bool isEffect) {
	auto it = map.find({charType, name, isEffect});
	if (it != map.end()) {
		new (&returnValue) MoveInfo(it->second);
		return true;
	}
	if (charType != GENERAL) {
		it = map.find({GENERAL, name, isEffect});
		if (it != map.end()) {
			new (&returnValue) MoveInfo(it->second);
			return true;
		}
	}
	return false;
}

bool Moves::getInfoWithName(MoveInfo& returnValue, CharacterType charType, const char* name, bool isEffect, const char** outNamePtr, void const** sortValue) {
	auto it = map.find({charType, name, isEffect});
	if (it != map.end()) {
		if (outNamePtr) *outNamePtr = it->first.name;
		if (sortValue) *sortValue = it->second.startPtr;
		new (&returnValue) MoveInfo(it->second);
		return true;
	}
	if (charType != GENERAL) {
		it = map.find({GENERAL, name, isEffect});
		if (it != map.end()) {
			if (outNamePtr) *outNamePtr = it->first.name;
			if (sortValue) *sortValue = it->second.startPtr;
			new (&returnValue) MoveInfo(it->second);
			return true;
		}
	}
	if (outNamePtr) *outNamePtr = nullptr;
	if (sortValue) *sortValue = nullptr;
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
		|| strcmp(ent.pawn.gotoLabelRequests(), "attack") == 0;
}
bool sectionSeparator_may6P(PlayerInfo& ent) {
	return strcmp(ent.pawn.gotoLabelRequests(), "6AHoldAttack") == 0 || ent.pawn.mem45() == 0 && ent.pawn.currentAnimDuration() > 9;
}
bool sectionSeparator_may6H(PlayerInfo& ent) {
	return strcmp(ent.pawn.gotoLabelRequests(), "6DHoldAttack") == 0 || ent.pawn.mem45() == 0 && ent.pawn.currentAnimDuration() > 9;
}
bool sectionSeparator_breakTheLaw(PlayerInfo& ent) {
	return strcmp(ent.pawn.gotoLabelRequests(), "up") == 0;
}
bool sectionSeparator_FDB(PlayerInfo& ent) {
	if (strcmp(ent.pawn.gotoLabelRequests(), "Attack") == 0) {
		return true;
	}
	if (!ent.pawn.bbscrCurrentFunc()) return false;
	BYTE* markerPos = moves.findSetMarker(ent.pawn.bbscrCurrentFunc(), "Attack");
	if (!markerPos) return false;
	markerPos = moves.skipInstr(markerPos);
	if (moves.instrType(markerPos) != instr_sprite) return false;
	return ent.pawn.bbscrCurrentInstr() > markerPos;
}
bool sectionSeparator_soutenBC(PlayerInfo& ent) {
	return strcmp(ent.pawn.gotoLabelRequests(), "open") == 0
		|| !ent.pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED);
}
bool sectionSeparator_QV(PlayerInfo& ent) {
	return strcmp(ent.pawn.gotoLabelRequests(), "End") == 0
		|| !ent.pawn.mem45() && ent.pawn.currentAnimDuration() > 12;
}
bool sectionSeparator_stingerS(PlayerInfo& ent) {
	return strcmp(ent.pawn.gotoLabelRequests(), "Shot") == 0
		|| !ent.pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED) && ent.pawn.currentAnimDuration() > 9;
}
bool sectionSeparator_stingerH(PlayerInfo& ent) {
	return strcmp(ent.pawn.gotoLabelRequests(), "Shot") == 0
		|| !ent.pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED) && ent.pawn.currentAnimDuration() > 3;
}
bool sectionSeparator_sultryPerformance(PlayerInfo& ent) {
	return strcmp(ent.pawn.gotoLabelRequests(), "Attack") == 0
		|| !ent.pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED) && ent.pawn.currentAnimDuration() > 9;
}
bool sectionSeparator_beakDriver(PlayerInfo& ent) {
	return strcmp(ent.pawn.gotoLabelRequests(), "Attack") == 0
		|| strcmp(ent.pawn.gotoLabelRequests(), "HeavyAttack") == 0;
}
bool sectionSeparator_rifle(PlayerInfo& ent) {
	return ent.wasEnableWhiffCancels;
}
bool sectionSeparator_leoGuardStance(PlayerInfo& ent) {
	if (strcmp(ent.pawn.gotoLabelRequests(), "End") == 0) {
		return true;
	}
	if (!ent.pawn.bbscrCurrentFunc()) return false;
	BYTE* markerPos = moves.findSetMarker(ent.pawn.bbscrCurrentFunc(), "End");
	if (!markerPos) {
		return false;
	}
	BYTE* nextInstr = moves.skipInstr(markerPos);
	if (moves.instrType(nextInstr) != instr_sprite) {
		return false;
	}
	return ent.pawn.bbscrCurrentInstr() > nextInstr;
}
bool sectionSeparator_treasureHunt(PlayerInfo& ent) {
	return strcmp(ent.pawn.gotoLabelRequests(), "Run") == 0;
}
bool sectionSeparator_falconDive(PlayerInfo& ent) {
	return strcmp(ent.pawn.gotoLabelRequests(), "Attack") == 0
		|| strcmp(ent.pawn.gotoLabelRequests(), "Attack2") == 0;
}
bool sectionSeparator_fourTigersSwordRev(PlayerInfo& ent) {
	return strcmp(ent.pawn.gotoLabelRequests(), "Attack") == 0;
}
bool sectionSeparator_blackHoleAttack(PlayerInfo& ent) {
	return strcmp(ent.pawn.gotoLabelRequests(), "Attack") == 0;
}
bool sectionSeparator_armorDance(PlayerInfo& ent) {
	return strcmp(ent.pawn.gotoLabelRequests(), "End") == 0
		|| strcmp(ent.pawn.gotoLabelRequests(), "End2") == 0
		|| strcmp(ent.pawn.gotoLabelRequests(), "End3") == 0;
}
bool sectionSeparator_kinomiNecro(PlayerInfo& ent) {
	return strcmp(ent.pawn.gotoLabelRequests(), "end") == 0;
}
bool sectionSeparator_azami(PlayerInfo& ent) {
	return strcmp(ent.pawn.gotoLabelRequests(), "End") == 0;
}
bool sectionSeparator_organ(PlayerInfo& ent) {
	return strcmp(ent.pawn.gotoLabelRequests(), "A") == 0
		|| strcmp(ent.pawn.gotoLabelRequests(), "B") == 0
		|| strcmp(ent.pawn.gotoLabelRequests(), "C") == 0
		|| strcmp(ent.pawn.gotoLabelRequests(), "D") == 0
		|| strcmp(ent.pawn.gotoLabelRequests(), "E") == 0
		|| strcmp(ent.pawn.gotoLabelRequests(), "tame") == 0;
}
bool sectionSeparator_dizzy6H(PlayerInfo& ent) {
	return strcmp(ent.pawn.gotoLabelRequests(), "Attack") == 0;
}
bool sectionSeparator_saishingeki(PlayerInfo& ent) {
	return strcmp(ent.pawn.gotoLabelRequests(), "Saishintuika") == 0;
}

bool sectionSeparatorProjectile_dolphin(Entity ent) {
	return strcmp(ent.gotoLabelRequests(), "move") == 0 || ent.mem51() == 0;
}

bool isIdle_default(PlayerInfo& player) {
	return player.wasEnableNormals;
}
bool canBlock_default(PlayerInfo& player) {
	return player.pawn.enableBlock();
}
bool canBlock_azami(PlayerInfo& player) {
	return !player.inNewMoveSection || strcmp(player.pawn.gotoLabelRequests(), "end") == 0;
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

BYTE* Moves::findSetMarker(BYTE* in, const char* name) {
	while (true) {
		InstrType type = instrType(in);
		if (type == instr_endState) {
			return nullptr;
		}
		if (type == instr_setMarker
				&& strcmp(asInstr(in, setMarker)->name, name) == 0) {
			return in;
		}
		in += bbscrInstructionSizes[type];
	}
}

BYTE* Moves::findNextMarker(BYTE* in, const char** name) {
	if (name) *name = nullptr;
	while (true) {
		InstrType type = instrType(in);
		if (type == instr_endState) {
			return nullptr;
		}
		if (type == instr_setMarker) {
			if (name) *name = asInstr(in, setMarker)->name;
			return in;
		}
		in += bbscrInstructionSizes[type];
	}
}

BYTE* Moves::findCreateObj(BYTE* in, const char* name) {
	while (true) {
		InstrType type = instrType(in);
		if (type == instr_endState) {
			return nullptr;
		}
		if (type == instr_createObject
				&& strcmp(asInstr(in, createObject)->name, name) == 0) {
			return in;
		}
		in += bbscrInstructionSizes[type];
	}
}

BYTE* Moves::findSpriteNonNull(BYTE* in) {
	while (true) {
		InstrType type = instrType(in);
		if (type == instr_endState) {
			return nullptr;
		}
		if (type == instr_sprite
				&& strcmp(asInstr(in, sprite)->name, "null") != 0) {
			return in;
		}
		in += bbscrInstructionSizes[type];
	}
}

BYTE* Moves::findSprite(BYTE* in, const char* name) {
	while (true) {
		InstrType type = instrType(in);
		if (type == instr_endState) {
			return nullptr;
		}
		if (type == instr_sprite
				&& strcmp(asInstr(in, sprite)->name, name) == 0) {
			return in;
		}
		in += bbscrInstructionSizes[type];
	}
}

BYTE* Moves::findAnySprite(BYTE* in) {
	while (true) {
		InstrType type = instrType(in);
		if (type == instr_endState) {
			return nullptr;
		}
		if (type == instr_sprite) {
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
		ent.hitboxes()->count[HITBOXTYPE_HITBOX] != 0
		&& ent.isActiveFrames()
	));
}

bool isDangerous_amorphous(Entity ent) {
	return !(ent.currentHitNum() != 0 && ent.hitboxes()->count[HITBOXTYPE_HITBOX] == 0)
		|| ent.inflicted()->untechableTime == 60; // boss version
}

bool isDangerous_active(Entity ent) {
	return ent.isActiveFrames();
}

bool isDangerous_hasNotCreatedAnythingYet(Entity ent) {
	return ent.previousEntity() == nullptr;
}

bool isDangerous_djavu(Entity ent) {
	return ent.stackEntity(0) == nullptr && !ent.destructionRequested();
}

bool isDangerous_Djavu_D_Ghost(Entity ent) {
	return !(ent.dealtAttack()->angle == -90 && ent.hitboxes()->count[HITBOXTYPE_HITBOX] == 0);
}

bool isDangerous_launchGreatsword(Entity ent) {
	return !(ent.currentHitNum() != 0 && ent.hitboxes()->count[HITBOXTYPE_HITBOX] == 0
		|| !ent.hasUpon(BBSCREVENT_PLAYER_BLOCKED));
}
bool isDangerous_ramSwordMove(Entity ent) {
	return !(ent.currentHitNum() == 3 && ent.hitboxes()->count[HITBOXTYPE_HITBOX] == 0
		|| !ent.hasUpon(BBSCREVENT_PLAYER_BLOCKED));
}
bool isDangerous_hasHitboxes(Entity ent) {
	return ent.hitboxes()->count[HITBOXTYPE_HITBOX] > 0;
}
bool isDangerous_bubble(Entity ent) {
	if (!ent.bbscrCurrentFunc()) return false;
	BYTE* markerPos = moves.findSetMarker(ent.bbscrCurrentFunc(), "bomb");
	if (!markerPos) {
		return false;
	}
	return ent.bbscrCurrentInstr() > markerPos
		&& !(ent.currentHitNum() != 0 && ent.hitboxes()->count[HITBOXTYPE_HITBOX] == 0)
		|| strcmp(ent.gotoLabelRequests(), "bomb") == 0;
}
bool isDangerous_kFish(Entity ent) {
	return !(ent.currentHitNum() == 2 && ent.hitboxes()->count[HITBOXTYPE_HITBOX] == 0)
		|| !ent.fullInvul();
}
bool isDangerous_pFish(Entity ent) {
	return !(ent.currentHitNum() != 0 && !ent.hasActiveFlag())
		|| !ent.fullInvul();
}
bool isDangerous_laserFish(Entity ent) {
	BYTE* func = ent.bbscrCurrentFunc();
	if (moves.laserFishCreateLaserOffset == 0 && func) {
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
	if (!ent.bbscrCurrentFunc()) return false;
	BYTE* markerPos1 = moves.findSpriteNonNull(ent.bbscrCurrentFunc());
	if (!markerPos1) return false;
	BYTE* markerPos2 = moves.findSpriteNull(markerPos1);
	if (!markerPos2) return false;
	return ent.hasActiveFlag() && ent.bbscrCurrentInstr() <= markerPos2;
}

bool isDangerous_rsfMeishi(Entity ent) {
	if (!ent.bbscrCurrentFunc()) return false;
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
bool isDangerous_servant(Entity ent) {
	std::vector<int>* offsets = nullptr;
	const ServantState* stateNames = nullptr;
	int stateNamesCount = 0;
	switch (ent.animationName()[7]) {
		case 'A':
			offsets = &moves.servantAStateOffsets;
			stateNames = servantStateNames;
			stateNamesCount = _countof(servantStateNames);
			break;
		case 'B':
			offsets = &moves.servantBStateOffsets;
			stateNames = servantStateNamesSpearman;
			stateNamesCount = _countof(servantStateNamesSpearman);
			break;
		case 'C':
			offsets = &moves.servantCStateOffsets;
			stateNames = servantStateNames;
			stateNamesCount = _countof(servantStateNames);
			break;
	}
	if (!offsets) return true;
	
	BYTE* func = ent.bbscrCurrentFunc();
	moves.fillGhostStateOffsets(func, *offsets);
	int state = moves.findGhostState(ent.bbscrCurrentInstr() - func, *offsets);
	return state < 0 || state >= stateNamesCount || !stateNames[state].isDeath;
}

const NamePair* nameSelector_iceSpike(Entity ent) {
	if (ent.createArgHikitsukiVal1() == 1) {
		return assignName("For searing cod...", "Fire Pillar");
	} else {
		return assignName("I used this to catch fish", "Ice Spike");
	}
}
const NamePair* nameSelectorUncombined_iceSpike(Entity ent) {
	if (ent.createArgHikitsukiVal1() == 1) {
		return assignName("Fire Pillar");
	} else {
		return assignName("Ice Spike");
	}
}
const NamePair* nameSelector_iceScythe(Entity ent) {
	if (ent.createArgHikitsukiVal1() == 1) {
		return assignName("For putting out the light...", "Ice Scythe");
	} else {
		return assignName("The light was so small in the beginning", "Fire Scythe");
	}
}
const NamePair* nameSelectorUncombined_iceScythe(Entity ent) {
	if (ent.createArgHikitsukiVal1() == 1) {
		return assignName("Ice Scythe");
	} else {
		return assignName("Fire Scythe");
	}
}
const NamePair* framebarNameSelector_djvuD(Entity ent) {
	bool hasHeightBuff = ent.mem45();
	if (ent.currentAnimDuration() <= 7) {
		hasHeightBuff = ent.y() >= 700000;
	}
	if (hasHeightBuff) {
		return assignName("\x44\xC3\xA9\x6A\xC3\xA0 Vu (Task C) Buffed", "DVC Buffed");
	} else {
		return assignName("\x44\xC3\xA9\x6A\xC3\xA0 Vu (Task C)", "DVC");
	}
}
const NamePair* framebarNameSelector_closeShot(Entity ent) {
	entityList.populate();
	int dist = ent.enemyEntity().posX() - ent.posX();
	if (dist < 0) dist = -dist;
	if (dist >= 300000) return assignName("Max Close Shot");
	return assignName("Max Close Shot Buffed");
}
const NamePair* framebarNameSelector_gunflameProjectile(Entity ent) {
	Entity player = ent.playerEntity();
	if (player) {
		if (strcmp(player.animationName(), "GunFlame") == 0
				&& strcmp(player.previousAnimName(), "CmnActFDash") == 0) {
			return assignName("Runflame");
		}
	}
	return assignName("Gunflame", "GF");
}
const NamePair* framebarNameSelector_venomBall(Entity ent) {
	if (strcmp(ent.dealtAttack()->trialName, "Ball_RedHail") == 0) {
		return assignName("Red Hail");
	} else if (strcmp(ent.dealtAttack()->trialName, "Ball_Gold") == 0) {
		return assignName("Bishop Runout", "Bishop");
	} else {
		return assignName("Balls");
	}
}
const NamePair* framebarNameUncombinedSelector_venomBall(Entity ent) {
	if (strcmp(ent.dealtAttack()->trialName, "Ball_RedHail") == 0) {
		return assignName("Red Hail");
	} else if (strcmp(ent.dealtAttack()->trialName, "Ball_Gold") == 0) {
		return assignName("Bishop Runout", "Bishop");
	} else {
		Entity player = ent.playerEntity();
		if (player) {
			#define thing(i, name) if (player.stackEntity(i) == ent) return assignName(name);
			thing(0, "P Ball")
			thing(1, "K Ball")
			thing(2, "S Ball")
			thing(3, "H Ball")
			#undef thing
		}
		return assignName("Ball");
	}
}
const NamePair* framebarNameSelector_grenadeBomb(Entity ent) {
	if (ent.dealtAttack()->level == 3) {
		return PROJECTILE_NAME_BERRY_BUFFED;
	}
	return PROJECTILE_NAME_BERRY;
}
const NamePair* framebarNameSelector_grenadeBombReady(Entity ent) {
	if (ent.currentHitNum()) {
		return assignName("Self-Detonate");
	}
	return assignName("Berry Pine", "Berry");
}
const NamePair* framebarNameSelector_landSettingTypeNeedleObj(Entity ent) {
	if (moves.reavenLandSettingTypeNeedleObjIsRev2 == Moves::TRIBOOL_DUNNO) {
		int count = 0;
		for (loopInstr(ent.bbscrCurrentFunc())) {
			if (moves.instrType(instr) == instr_numberOfHits) {
				++count;
			}
		}
		if (count > 1) {
			moves.reavenLandSettingTypeNeedleObjIsRev2 = Moves::TRIBOOL_TRUE;
		} else {
			moves.reavenLandSettingTypeNeedleObjIsRev2 = Moves::TRIBOOL_FALSE;
		}
	}
	if (moves.reavenLandSettingTypeNeedleObjIsRev2 == Moves::TRIBOOL_TRUE) {
		switch (ent.numberOfHits()) {
			case 3: return assignName("Scharf Kugel Lv2", "Orb Lv2");
			case 4: return assignName("Scharf Kugel Lv3", "Orb Lv3");
			default: return assignName("Scharf Kugel Lv1", "Orb Lv1");
		}
	} else {
		switch (ent.dealtAttack()->level){
			case 2: return assignName("Scharf Kugel", "Orb");
			default: return assignName("Scharf Kugel Buffed", "Orb Buffed");
		}
	}
}

const NamePair* MoveInfo::getFramebarName(Entity ent) const {
	if (framebarNameSelector && ent) return framebarNameSelector(ent);
	return framebarName;
}

bool isInVariableStartupSection_treasureHunt(PlayerInfo& ent) {
	if (!ent.pawn.bbscrCurrentFunc()) return false;
	BYTE* markerPos = moves.findSetMarker(ent.pawn.bbscrCurrentFunc(), "Run");
	if (!markerPos) return false;
	return ent.pawn.bbscrCurrentInstr() <= markerPos && ent.pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED);
}
bool isInVariableStartupSection_zweiLand(PlayerInfo& ent) {
	return ent.wasEnableWhiffCancels && ent.y > 0;
}
bool isInVariableStartupSection_blitzShield(PlayerInfo& ent) {
	if (ent.pawn.mem45() == 1) return false;
	if (!ent.pawn.bbscrCurrentFunc()) return false;
	BYTE* markerPos = moves.findSetMarker(ent.pawn.bbscrCurrentFunc(), "attack");
	if (!markerPos) return false;
	return ent.pawn.currentAnimDuration() >= 16 && ent.pawn.bbscrCurrentInstr() <= markerPos && *ent.pawn.gotoLabelRequests() == '\0';
}
bool isInVariableStartupSection_may6Por6H(PlayerInfo& ent) {
	return ent.pawn.mem45() && *ent.pawn.gotoLabelRequests() == '\0';
}
bool isInVariableStartupSection_soutenBC(PlayerInfo& ent) {
	return ent.pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED) && *ent.pawn.gotoLabelRequests() == '\0' && !ent.pawn.isRecoveryState();
}
bool isInVariableStartupSection_amiMove(PlayerInfo& ent) {
	return ent.wasEnableWhiffCancels && !ent.pawn.hitstop();
}
bool isInVariableStartupSection_beakDriver(PlayerInfo& ent) {
	return ent.pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED);
}
bool isInVariableStartupSection_organOpen(PlayerInfo& ent) {
	return ent.pawn.mem45() && ent.pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED) && ent.pawn.gotoLabelRequests()[0] == '\0';
}
bool isInVariableStartupSection_breakTheLaw(PlayerInfo& ent) {
	return ent.pawn.mem45() && ent.pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED) && ent.pawn.gotoLabelRequests()[0] == '\0';
}
bool isInVariableStartupSection_fdb(PlayerInfo& ent) {
	return ent.pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED);
}
bool isInVariableStartupSection_qv(PlayerInfo& ent) {
	return ent.pawn.mem45() && ent.pawn.gotoLabelRequests()[0] == '\0';
}
bool isInVariableStartupSection_stinger(PlayerInfo& ent) {
	return ent.pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED) && ent.pawn.gotoLabelRequests()[0] == '\0';
}
bool isInVariableStartupSection_inoDivekick(PlayerInfo& ent) {
	return ent.pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED) && ent.pawn.gotoLabelRequests()[0] == '\0';
}
bool isInVariableStartupSection_sinRTL(PlayerInfo& ent) {
	return ent.pawn.mem49() && ent.pawn.mem45() && ent.pawn.mem46() <= 1 && ent.pawn.gotoLabelRequests()[0] == '\0';
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
	return *ent.pawn.gotoLabelRequests() == '\0' && ent.pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED);
}

bool canStopHolding_armorDance(PlayerInfo& ent) {
	if (!ent.pawn.mem45() || *ent.pawn.gotoLabelRequests() != '\0') return false;
	BYTE* funcStart = ent.pawn.bbscrCurrentFunc();
	
	int* armorDanceEndOffset;
	// length of "ArmorDance" == 10
	if (*(ent.pawn.animationName() + 10) == '2') {  // is ArmorDance2
		armorDanceEndOffset = &moves.armorDance2EndOffset;
	} else {
		armorDanceEndOffset = &moves.armorDanceEndOffset;
	}
	
	if (*armorDanceEndOffset == 0 && funcStart) {
		BYTE* markerPos = moves.findSetMarker(funcStart, "End");
		if (!markerPos) return false;
		*armorDanceEndOffset = markerPos - funcStart;
	}
	BYTE* markerPos = funcStart + *armorDanceEndOffset;
	return ent.pawn.bbscrCurrentInstr() <= markerPos;
}

bool canStopHolding_armorDance2(PlayerInfo& ent) {
	if (!ent.pawn.mem45() || *ent.pawn.gotoLabelRequests() != '\0') return false;
	BYTE* funcStart = ent.pawn.bbscrCurrentFunc();
	if (moves.armorDance2EndOffset == 0 && funcStart) {
		BYTE* markerPos = moves.findSetMarker(funcStart, "End");
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
bool conditionForAddingWhiffCancels_airStop(PlayerInfo& ent) {
	return !ent.wasCancels.whiffCancels.empty();
}

bool secondaryStartup_saishingeki(PlayerInfo& ent) {
	if (strcmp(ent.pawn.gotoLabelRequests(), "Saishintuika") == 0) return true;
	if (ent.pawn.currentHitNum() == 2 && !ent.pawn.isActiveFrames()) return false;
	BYTE* funcStart = ent.pawn.bbscrCurrentFunc();
	if (moves.saishingeki_SaishintuikaOffset == 0 && moves.saishingeki_SaishintuikaEndOffset == 0 && funcStart) {
		BYTE* markerPos = moves.findSetMarker(funcStart, "Saishintuika");
		if (!markerPos) return false;
		moves.saishingeki_SaishintuikaOffset = markerPos - funcStart;
		BYTE* nextSearchStart = moves.skipInstr(markerPos);
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
	if (*offset == 0 && funcStart) {
		BYTE* markerPos = moves.findSetMarker(funcStart, "end_air");
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
	if (moves.zanseiRougaRecoveryOffset == 0 && funcStart) {
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
	if (moves.hououshouHitOffset == 0 && funcStart) {
		BYTE* markerPos = moves.findSetMarker(funcStart, "hit");
		if (!markerPos) return false;
		moves.hououshouHitOffset = markerPos - funcStart;
	}
	if (!moves.hououshouHitOffset) return false;
	BYTE* currentInstr = ent.pawn.bbscrCurrentInstr();
	return currentInstr >= funcStart + moves.hououshouHitOffset;
}

const NamePair* displayNameSelector_pogoEntry(PlayerInfo& ent) {
	return ent.idle ? assignName("Spear Point Centripetal Dance Idle", "Pogo Idle")
		: assignName("Spear Point Centripetal Dance Entry", "Pogo Entry");
}
const NamePair* displayNameSelector_pogoA(PlayerInfo& ent) {
	return ent.idle ? assignName("Spear Point Centripetal Dance Idle", "Pogo Stance")
		: assignName("Just A Taste!", "Pogo-P");
}
const NamePair* displayNameSelector_pogo9(PlayerInfo& ent) {
	return ent.idle ? assignName("Spear Point Centripetal Dance Idle", "Pogo Stance")
		: assignName("Short Hop", "Pogo-9");
}
const NamePair* displayNameSelector_pogo44(PlayerInfo& ent) {
	return ent.idle ? assignName("Spear Point Centripetal Dance Idle", "Pogo Stance")
		: assignName("Backward Movement", "Pogo-44");
}
const NamePair* displayNameSelector_pogo66(PlayerInfo& ent) {
	return ent.idle ? assignName("Spear Point Centripetal Dance Idle", "Pogo Stance")
		: assignName("Forward Movement", "Pogo-66");
}
const NamePair* displayNameSelector_pogoB(PlayerInfo& ent) {
	return ent.idle ? assignName("Spear Point Centripetal Dance Idle", "Pogo Stance")
		: assignName("Growing Flower", "Pogo-K");
}
const NamePair* displayNameSelector_pogoC(PlayerInfo& ent) {
	return ent.idle ? assignName("Spear Point Centripetal Dance Idle", "Pogo Stance")
		: assignName("See? I'm a Flower!", "Pogo-S");
}
const NamePair* displayNameSelector_pogoD(PlayerInfo& ent) {
	return ent.idle ? assignName("Spear Point Centripetal Dance Idle", "Pogo Stance")
		: assignName("Spear Point Centripetal Dance Going My Way", "Pogo-H");
}
const NamePair* displayNameSelector_pogoE(PlayerInfo& ent) {
	return ent.idle ? assignName("Spear Point Centripetal Dance Idle", "Pogo Stance")
		: assignName("Spear Point Centripetal Dance What Could This Be?", "Pogo-D");
}
const NamePair* displayNameSelector_pogo8(PlayerInfo& ent) {
	return ent.idle ? assignName("Spear Point Centripetal Dance Idle", "Pogo Stance")
		: assignName("Doctor-Copter", "Pogo-8");
}
const NamePair* displayNameSelector_itemToss(PlayerInfo& ent) {
	Entity pawn = ent.pawn;
	BYTE* func = pawn.bbscrCurrentFunc();
	if (moves.faustFastToss == 0) {
		bool encounteredGoto = false;
		for (loopInstr(func)) {
			InstrType type = moves.instrType(instr);
			if (type == instr_setMarker) {
				if (strcmp(asInstr(instr, setMarker)->name, "fastthrow") == 0) {
					moves.faustFastToss = instr - func;
					break;
				}
			} else if (type == instr_gotoLabelRequests) {
				if (strcmp(asInstr(instr, gotoLabelRequests)->name, "fastthrow") == 0) {
					encounteredGoto = true;
				}
			} else if (type == instr_sprite) {
				if (encounteredGoto) {
					encounteredGoto = false;
					moves.faustFastTossGoto = instr - func;
				}
			}
		}
	}
	int offset = pawn.bbscrCurrentInstr() - func;
	if (offset > moves.faustFastToss || offset <= moves.faustFastTossGoto) {
		return assignName("What Could This Be?", "Toss");
	} else {
		return assignName("What Could This Be? (Held)", "Toss (Held)");
	}
}
const NamePair* displayNameSelector_RC(PlayerInfo& ent) {
	return ent.pawn.yellowRomanCancel()
		? assignName("Yellow Roman Cancel", "YRC")
		: ent.pawn.purpleRomanCancel()
			? assignName("Purple Roman Cancel", "PRC")
			: assignName("Red Roman Cancel", "RRC");
}
const NamePair* displayNameSelector_may6P(PlayerInfo& ent) {
	Entity pawn = ent.pawn;
	const AttackData* dealtAttack = pawn.dealtAttack();
	
	BYTE* func = pawn.bbscrCurrentFunc();
	moves.fillMay6PElements(func);
	
	Moves::May6PAttackData attackData;
	attackData.stun = dealtAttack->stun;
	attackData.blockstun = dealtAttack->blockstun == INT_MAX ? blockstuns[dealtAttack->level] : dealtAttack->blockstun;
	attackData.pushback = dealtAttack->pushbackModifier;
	attackData.wallstick = pawn.inflicted()->wallstickDuration;
	
	if (!moves.may6PElements.empty()) {
		int offset = pawn.bbscrCurrentInstr() - func;
		if (offset >= moves.may6PElements.front().offset) {
			for (const Moves::May6PElement& elem : moves.may6PElements) {
				if (elem.attackData.stun == attackData.stun
						&& elem.attackData.blockstun == attackData.blockstun
						&& elem.attackData.pushback == attackData.pushback
						&& elem.attackData.wallstick == attackData.wallstick) {
					return &elem.name;
				}
			}
		}
	}
	
	return assignName("6P");
}
const NamePair* displayNameSelector_may6H(PlayerInfo& ent) {
	if (strcmp(ent.pawn.gotoLabelRequests(), "6DHoldAttack") == 0) {
		return assignName("6H Slightly Held");
	}
	BYTE* func = ent.pawn.bbscrCurrentFunc();
	if (func) {
		fillMay6HOffsets(func);
		int offset = ent.pawn.bbscrCurrentInstr() - func;
		if (offset > moves.may6H_6DHoldOffset && offset < moves.may6H_6DHoldAttackOffset) {
			return ent.pawn.mem45() == 0 ? assignName("6H Max") : assignName("6H Hold");
		} else if (offset > moves.may6H_6DHoldAttackOffset) {
			return assignName("6H Slightly Held");
		}
	}
	return assignName("6H");
}
const NamePair* displayNameSelector_badMoon(PlayerInfo& ent) {
	int maxHit = ent.maxHit.max;
	if (maxHit == 10) {
		return assignName("Bad Moon (Height Buff, 10 hits)", "BM (Height Buff, 10 hits)");
	} else if (maxHit == 9) {
		return assignName("Bad Moon (Height Buff, 9 hits)", "BM (Height Buff, 9 hits)");
	} else if (maxHit == 8) {
		return assignName("Bad Moon (Height Buff, 8 hits)", "BM (Height Buff, 8 hits)");
	} else if (maxHit == 7) {
		return assignName("Bad Moon (Height Buff, 7 hits)", "BM (Height Buff, 7 hits)");
	} else if (maxHit == 6) {
		return assignName("Bad Moon (Height Buff, 6 hits)", "BM (Height Buff, 6 hits)");
	} else if (maxHit == 5) {
		return assignName("Bad Moon (Height Buff, 5 hits)", "BM (Height Buff, 5 hits)");
	} else if (ent.pawn.mem46()) {
		return assignName("Bad Moon (Height Buff)", "BM (Height Buff)");
	} else {
		return assignName("Bad Moon", "BM");
	}
}
const NamePair* displayNameSelector_carcassRaidS(PlayerInfo& ent) {
	if (ent.pawn.createArgHikitsukiVal2_outgoing() == 1  // rev1
			&& ent.pawn.venomBallArg3() == 164) {
		return assignName("S Carcass Raid With Spin", "S Carcass With Spin");
	} else {
		return assignName("S Carcass Raid", "S Carcass");
	}
}
const NamePair* displayNameSelector_carcassRaidH(PlayerInfo& ent) {
	if (ent.pawn.venomBallArg3() == 2724) {
		return assignName("H Carcass Raid With Spin", "H Carcass With Spin");
	} else {
		return assignName("H Carcass Raid", "H Carcass");
	}
}
const NamePair* displayNameSelector_stingerS(PlayerInfo& ent) {
	if (ent.pawn.createArgHikitsukiVal2_outgoing() == 1  // rev1
			&& ent.pawn.venomBallArg3() == 164) {
		return assignName("S Stinger Aim With Spin", "S Stinger With Spin");
	} else {
		return assignName("S Stinger Aim", "S Stinger");
	}
}
const NamePair* displayNameSelector_stingerH(PlayerInfo& ent) {
	if (ent.pawn.venomBallArg3() == 164) {
		return assignName("H Stinger Aim With Spin", "H Stinger With Spin");
	} else {
		return assignName("H Stinger Aim", "H Stinger");
	}
}
const NamePair* displayNameSelector_taskCAir(PlayerInfo& ent) {
	if (ent.pawn.mem45()) {
		return assignName("Air Task C Buffed", "j.TC Buffed");
	} else {
		return assignName("Air Task C", "j.TC");
	}
}
const NamePair* framebarNameSelector_blueBurst(Entity ent) {
	int team = ent.team();
	if (!(team == 0 || team == 1)) return assignName("Blue Burst");
	return endScene.currentState->players[team].wasOtg ? assignName("OTG Burst") : assignName("Blue Burst");
}
const NamePair* displayNameSelector_blueBurst(PlayerInfo& ent) {
	return ent.wasOtg ? assignName("OTG Burst") : assignName("Blue Burst");
}
const NamePair* displayNameSelector_rifleStart(PlayerInfo& ent) {
	if (ent.idle) return assignName("Ms. Confille", "Rifle");
	const NamePair* response = moves.rifleAutoExit(ent, &moves.elpheltRifleStartEndMarkerOffset, assignName("Ms. Confille", "Rifle Autoexit"));
	if (response) return response;
	if (moves.forCancels) return assignName("Aim Ms. Confille", "Rifle");
	return !ent.inNewMoveSection ? assignName("Aim Ms. Confille Until Able to Cancel", "Rifle Until Able to Cancel")
		: assignName("Aim Ms. Confille Until Able to Fire", "Rifle Until Able to Fire");
}
const NamePair* displayNameSelector_rifleReload(PlayerInfo& ent) {
	if (ent.idle) return assignName("Ms. Confille", "Rifle");
	const NamePair* response = moves.rifleAutoExit(ent, &moves.elpheltRifleReloadEndMarkerOffset, assignName("Ms. Confille Autoexit", "Rifle Autoexit"));
	if (response) return response;
	if (moves.forCancels) return assignName("Ms. Confille Reload", "Reload");
	return !ent.inNewMoveSection ? assignName("Ms. Confille Reload Until Able to Cancel", "Reload Until Able to Cancel")
		: assignName("Ms. Confille Reload Until Able to Fire", "Reload Until Able to Fire");
}
const NamePair* displayNameSelector_riflePerfectReload(PlayerInfo& ent) {
	if (ent.idle) return assignName("Ms. Confille", "Rifle");
	const NamePair* response = moves.rifleAutoExit(ent, &moves.elpheltRifleReloadPerfectEndMarkerOffset, assignName("Ms. Confille Autoexit", "Rifle Autoexit"));
	if (response) return response;
	if (moves.forCancels) return assignName("Ms. Confille Perfect Reload", "Perfect Reload");
	return !ent.inNewMoveSection ? assignName("Ms. Confille Perfect Reload Until Able to Cancel", "Perfect Reload Until Able to Cancel")
		: assignName("Ms. Confille Perfect Reload Until Able to Fire", "Perfect Reload Until Able to Fire");
}
const NamePair* displayNameSelector_rifleRC(PlayerInfo& ent) {
	if (ent.idle) return assignName("Ms. Confille", "Rifle");
	const NamePair* response = moves.rifleAutoExit(ent, &moves.elpheltRifleRomanEndMarkerOffset, assignName("Ms. Confille Autoexit", "Rifle Autoexit"));
	if (response) return response;
	if (moves.forCancels) return assignName("Ms. Confille Roman Cancel", "Rifle RC");
	return !ent.inNewMoveSection ? assignName("Ms. Confille Roman Cancel Until Able to Cancel", "Rifle RC Until Able to Cancel")
		: assignName("Ms. Confille Roman Cancel Until Able to Fire", "Rifle RC Until Able to Fire");
}
#define johnnyMFNameSelect(type, lvl, name) \
	if (type == 0) { \
		if (lvl == 0) return assignName("Lv1 P Mist Finer" name, "Lv1 PMF" name); \
		if (lvl == 1) return assignName("Lv2 P Mist Finer" name, "Lv1 PMF" name); \
		if (lvl == 2) return assignName("Lv3 P Mist Finer" name, "Lv1 PMF" name); \
	} \
	if (type == 1) { \
		if (lvl == 0) return assignName("Lv1 K Mist Finer" name, "Lv1 KMF" name); \
		if (lvl == 1) return assignName("Lv2 K Mist Finer" name, "Lv2 KMF" name); \
		if (lvl == 2) return assignName("Lv3 K Mist Finer" name, "Lv3 KMF" name); \
	} \
	if (type == 2) { \
		if (lvl == 0) return assignName("Lv1 S Mist Finer" name, "Lv1 SMF" name); \
		if (lvl == 1) return assignName("Lv2 S Mist Finer" name, "Lv2 SMF" name); \
		if (lvl == 2) return assignName("Lv3 S Mist Finer" name, "Lv3 SMF" name); \
	}
#define johnnyMFNameSelectWithSlang(type, lvl, name, slang) \
	if (type == 0) { \
		if (lvl == 0) return assignName("Lv1 P Mist Finer" name, "Lv1 PMF" slang); \
		if (lvl == 1) return assignName("Lv2 P Mist Finer" name, "Lv1 PMF" slang); \
		if (lvl == 2) return assignName("Lv3 P Mist Finer" name, "Lv1 PMF" slang); \
	} \
	if (type == 1) { \
		if (lvl == 0) return assignName("Lv1 K Mist Finer" name, "Lv1 KMF" slang); \
		if (lvl == 1) return assignName("Lv2 K Mist Finer" name, "Lv2 KMF" slang); \
		if (lvl == 2) return assignName("Lv3 K Mist Finer" name, "Lv3 KMF" slang); \
	} \
	if (type == 2) { \
		if (lvl == 0) return assignName("Lv1 S Mist Finer" name, "Lv1 SMF" slang); \
		if (lvl == 1) return assignName("Lv2 S Mist Finer" name, "Lv2 SMF" slang); \
		if (lvl == 2) return assignName("Lv3 S Mist Finer" name, "Lv3 SMF" slang); \
	}
#define johnnyAirborneMFNameSelect(type, lvl, name) \
	if (type == 0) { \
		if (lvl == 0) return assignName("Lv1 Air P Mist Finer" name, "Lv1 j.PMF" name); \
		if (lvl == 1) return assignName("Lv2 Air P Mist Finer" name, "Lv1 j.PMF" name); \
		if (lvl == 2) return assignName("Lv3 Air P Mist Finer" name, "Lv1 j.PMF" name); \
	} \
	if (type == 1) { \
		if (lvl == 0) return assignName("Lv1 Air K Mist Finer" name, "Lv1 j.KMF" name); \
		if (lvl == 1) return assignName("Lv2 Air K Mist Finer" name, "Lv2 j.KMF" name); \
		if (lvl == 2) return assignName("Lv3 Air K Mist Finer" name, "Lv3 j.KMF" name); \
	} \
	if (type == 2) { \
		if (lvl == 0) return assignName("Lv1 Air S Mist Finer" name, "Lv1 j.SMF" name); \
		if (lvl == 1) return assignName("Lv2 Air S Mist Finer" name, "Lv2 j.SMF" name); \
		if (lvl == 2) return assignName("Lv3 Air S Mist Finer" name, "Lv3 j.SMF" name); \
	}
const NamePair* displayNameSelector_mistEntry(PlayerInfo& ent) {
	int type = ent.pawn.mem53();
	int lvl = ent.pawn.playerVal(1);
	johnnyMFNameSelect(type, lvl, " Entry")
	return assignName("Mist Finer Entry", "MF Entry");
}
const NamePair* displayNameSelector_mistLoop(PlayerInfo& ent) {
	int type = ent.pawn.mem53();
	int lvl = ent.pawn.playerVal(1);
	if (ent.pawn.mem54()) {
		// it is possible to end up here only in Rev1
		johnnyMFNameSelect(type, lvl, "")
		return assignName("Mist Finer", "MF");
	}
	if (!ent.inNewMoveSection) {
		johnnyMFNameSelect(type, lvl, " Entry")
		return assignName("Mist Finer Entry", "MF Entry");
	}
	johnnyMFNameSelect(type, lvl, " Stance")
	return assignName("Mist Finer Stance", "MF Stance");
}
const NamePair* displayNameSelector_mistWalkForward(PlayerInfo& ent) {
	int type = ent.pawn.mem53();
	int lvl = ent.pawn.playerVal(1);
	johnnyMFNameSelect(type, lvl, " Walk Forward")
	return assignName("Mist Finer Walk Forward", "MF Walk Forward");
}
const NamePair* displayNameSelector_mistWalkBackward(PlayerInfo& ent) {
	int type = ent.pawn.mem53();
	int lvl = ent.pawn.playerVal(1);
	johnnyMFNameSelect(type, lvl, " Walk Backward")
	return assignName("Mist Finer Walk Backward", "MF Walk Backward");
}
const NamePair* displayNameSelector_mistDash(PlayerInfo& ent) {
	int type = ent.pawn.mem53();
	int lvl = ent.pawn.playerVal(1);
	johnnyMFNameSelect(type, lvl, " Dash")
	return assignName("Mist Finer Dash", "MF Dash");
}
const NamePair* displayNameSelector_mistBDash(PlayerInfo& ent) {
	int type = ent.pawn.mem53();
	int lvl = ent.pawn.playerVal(1);
	johnnyMFNameSelectWithSlang(type, lvl, " Backdash", " BD")
	return assignName("Mist Finer Backdash", "MF BD");
}
const NamePair* displayNameSelector_mistBackdash(PlayerInfo& ent) {
	int type = ent.pawn.mem53();
	int lvl = ent.pawn.playerVal(1);
	johnnyMFNameSelect(type, lvl, " Backdash")
	return assignName("Mist Finer Backdash", "MF Backdash");
}
const NamePair* displayNameSelector_airMistEntry(PlayerInfo& ent) {
	int type = ent.pawn.mem53();
	int lvl = ent.pawn.playerVal(1);
	johnnyAirborneMFNameSelect(type, lvl, " Entry")
	return assignName("Air Mist Finer Entry", "j.MF Entry");
}
const NamePair* displayNameSelector_airMistLoop(PlayerInfo& ent) {
	int type = ent.pawn.mem53();
	int lvl = ent.pawn.playerVal(1);
	if (ent.pawn.mem54()) {
		// it is possible to end up here only in Rev1
		johnnyAirborneMFNameSelect(type, lvl, "")
		return assignName("Air Mist Finer", "j.MF");
	}
	if (!ent.inNewMoveSection) {
		johnnyAirborneMFNameSelect(type, lvl, " Entry")
		return assignName("Air Mist Finer Entry", "j.MF Entry");
	}
	johnnyAirborneMFNameSelect(type, lvl, " Stance")
	return assignName("Air Mist Finer Stance", "j.MF Stance");
}
const NamePair* displayNameSelector_airMistDash(PlayerInfo& ent) {
	int type = ent.pawn.mem53();
	int lvl = ent.pawn.playerVal(1);
	johnnyAirborneMFNameSelect(type, lvl, " Airdash")
	return assignName("Air Mist Finer Airdash", "j.MF Airdash");
}
const NamePair* displayNameSelector_airMistBackdash(PlayerInfo& ent) {
	int type = ent.pawn.mem53();
	int lvl = ent.pawn.playerVal(1);
	johnnyAirborneMFNameSelect(type, lvl, " Airdash Back")
	return assignName("Air Mist Finer Airdash Back", "j.MF Airdash Back");
}
const NamePair* displayNameSelector_gekirinLv2or3(PlayerInfo& ent) {
	if (strcmp(ent.labelAtTheStartOfTheMove, "Lv3Gekirin") == 0) {
		return assignName("Lv3 Gekirin");
	}
	return assignName("Lv2 Gekirin");
}
const NamePair* displayNameSelector_airGekirinLv2or3(PlayerInfo& ent) {
	if (strcmp(ent.labelAtTheStartOfTheMove, "Lv3Gekirin") == 0) {
		return assignName("Lv3 Air Gekirin");
	}
	return assignName("Lv2 Air Gekirin");
}
const NamePair* displayNameSelector_ryujinLv2or3(PlayerInfo& ent) {
	if (strcmp(ent.labelAtTheStartOfTheMove, "Lv3Ryujin") == 0) {
		return assignName("Lv3 Ryuujin");
	}
	return assignName("Lv2 Ryuujin");
}
const NamePair* displayNameSelector_airRyujinLv2or3(PlayerInfo& ent) {
	if (strcmp(ent.labelAtTheStartOfTheMove, "Lv3Ryujin") == 0) {
		return assignName("Lv3 Air Ryuujin");
	}
	return assignName("Lv2 Air Ryuujin");
}
const NamePair* displayNameSelector_kenroukakuLv2or3(PlayerInfo& ent) {
	if (strcmp(ent.labelAtTheStartOfTheMove, "Lv3Kenroukaku") == 0) {
		return assignName("Lv3 Kenroukaku");
	}
	return assignName("Lv2 Kenroukaku");
}
const NamePair* displayNameSelector_airKenroukakuLv2or3(PlayerInfo& ent) {
	if (strcmp(ent.labelAtTheStartOfTheMove, "Lv3Kenroukaku") == 0) {
		return assignName("Lv3 Air Kenroukaku");
	}
	return assignName("Lv2 Air Kenroukaku");
}
const NamePair* displayNameSelector_standingAzami(PlayerInfo& ent) {
	return ent.pawn.mem46() ? assignName("Standing Red Azami") : assignName("Standing Azami");
}
const NamePair* displayNameSelector_crouchingAzami(PlayerInfo& ent) {
	return ent.pawn.mem46() ? assignName("Crouching Red Azami") : assignName("Crouching Azami");
}
const NamePair* displayNameSelector_airAzami(PlayerInfo& ent) {
	return ent.pawn.mem46() ? assignName("Aerial Red Azami") : assignName("Aerial Azami");
}
const NamePair* displayNameSelector_gunflame(PlayerInfo& ent) {
	if (strcmp(ent.pawn.previousAnimName(), "CmnActFDash") == 0) {
		return assignName("Runflame");
	}
	return assignName("Gunflame", "GF");
}
const NamePair* displayNameSelector_gunflameDI(PlayerInfo& ent) {
	if (strcmp(ent.pawn.previousAnimName(), "CmnActFDash") == 0) {
		return assignName("DI Runflame");
	}
	return assignName("DI Gunflame", "DI GF");
}
const NamePair* displayNameSelector_standingBlitzShield(PlayerInfo& ent) {
	GroundBlitzType type = Moves::getBlitzType(ent);
	switch (type) {
		case BLITZTYPE_MAXCHARGE: return assignName("Max Charge Standing Blitz Shield", "Max Standing Blitz");
		case BLITZTYPE_CHARGE: return assignName("Charge Standing Blitz Shield", "Charge Standing Blitz");
		default: return assignName("Tap Standing Blitz Shield", "Tap Standing Blitz");
	}
}
const NamePair* displayNameSelector_crouchingBlitzShield(PlayerInfo& ent) {
	GroundBlitzType type = Moves::getBlitzType(ent);
	switch (type) {
		case BLITZTYPE_MAXCHARGE: return assignName("Max Charge Crouching Blitz Shield", "Max Crouching Blitz");
		case BLITZTYPE_CHARGE: return assignName("Charge Crouching Blitz Shield", "Charge Crouching Blitz");
		default: return assignName("Tap Crouching Blitz Shield", "Tap Crouching Blitz");
	}
}
const NamePair* displayNameSelector_pilebunker(PlayerInfo& ent) {
	if (strcmp(ent.pawn.previousAnimName(), "DandyStepA") == 0) {
		return assignName("P Pilebunker", "P Pile");
	}
	return assignName("K Pilebunker", "K Pile");
}
const NamePair* displayNameSelector_crosswise(PlayerInfo& ent) {
	if (strcmp(ent.pawn.previousAnimName(), "DandyStepA") == 0) {
		return assignName("P Crosswise Heel", "P CW");
	}
	return assignName("K Crosswise Heel", "K CW");
}
const NamePair* displayNameSelector_underPressure(PlayerInfo& ent) {
	if (strcmp(ent.pawn.previousAnimName(), "DandyStepA") == 0) {
		return assignName("P Under Pressure", "P UP");
	}
	return assignName("K Under Pressure", "K UP");
}
const NamePair* displayNameSelector_jacko4DImpl(PlayerInfo& ent, int height, const NamePair* nameBase,
		const NamePair* name1,
		const NamePair* name2,
		const NamePair* name3,
		const NamePair* name4,
		const NamePair* name6,
		const NamePair* name7,
		const NamePair* name8,
		const NamePair* name9) {
	int x = ent.pawn.inflicted()->impulseX;
	int y = ent.pawn.inflicted()->impulseY;
	if (x == 0 && y == -30000 && ent.pawn.inflicted()->groundBounceCount == INT_MAX) return nameBase;
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
		case DIRX_MINUS | DIRY_MINUS: return name1;
		case DIRX_NEUTRAL | DIRY_MINUS: return name2;
		case DIRX_PLUS | DIRY_MINUS: return name3;
		case DIRX_MINUS | DIRY_NEUTRAL: return name4;
		case DIRX_PLUS | DIRY_NEUTRAL: return name6;
		case DIRX_MINUS | DIRY_PLUS: return name7;
		case DIRX_NEUTRAL | DIRY_PLUS: return name8;
		case DIRX_PLUS | DIRY_PLUS: return name9;
		default: return nameBase;
	}
}
const NamePair* displayNameSelector_jacko4D(PlayerInfo& ent) {
	return displayNameSelector_jacko4DImpl(ent, 20000, assignName("4D"),
		assignName("4D1"),
		assignName("4D2"),
		assignName("4D3"),
		assignName("4D4"),
		assignName("4D6"),
		assignName("4D7"),
		assignName("4D8"),
		assignName("4D9"));
}
const NamePair* displayNameSelector_jackoj4D(PlayerInfo& ent) {
	return displayNameSelector_jacko4DImpl(ent, 25000, assignName("4D"),
		assignName("j.4D1"),
		assignName("j.4D2"),
		assignName("j.4D3"),
		assignName("j.4D4"),
		assignName("j.4D6"),
		assignName("j.4D7"),
		assignName("j.4D8"),
		assignName("j.4D9"));
}
const NamePair* displayNameSelector_beakDriver(PlayerInfo& ent) {
	if (ent.sinHunger) {
		return assignName("Hunger");
	} else {
		return assignName("Beak Driver", "Beak");
	}
}
const NamePair* displayNameSelector_elkHunt(PlayerInfo& ent) {
	if (ent.sinHunger) {
		return assignName("Hunger");
	} else {
		return assignName("Elk Hunt", "Elk");
	}
}
const NamePair* displayNameSelector_hawkBaker(PlayerInfo& ent) {
	if (ent.sinHunger) {
		return assignName("Hunger");
	} else {
		return assignName("Hawk Baker");
	}
}
const NamePair* displayNameSelector_vultureSeize(PlayerInfo& ent) {
	if (ent.sinHunger) {
		return assignName("Hunger");
	} else {
		return assignName("Vulture Seize", "Vulture");
	}
}
const NamePair* displayNameSelector_beakDriverAir(PlayerInfo& ent) {
	if (ent.sinHunger) {
		return assignName("Hunger");
	} else {
		return assignName("Aerial Beak Driver", "Air Beak");
	}
}
const NamePair* displayNameSelector_bullBash(PlayerInfo& ent) {
	if (ent.sinHunger) {
		return assignName("Hunger");
	} else {
		return assignName("Bull Bash");
	}
}
const NamePair* displayNameSelector_beakDriverMash(PlayerInfo& ent) {
	if (ent.sinHunger) {
		return assignName("Hunger");
	} else {
		return assignName("I'm Sure I'll Hit Something", "Beak Mash");
	}
}
const NamePair* displayNameSelector_answer6K(PlayerInfo& ent) {
	Entity pawn = ent.pawn;
	BYTE* func = pawn.bbscrCurrentFunc();
	if (moves.answerFaint == 0) {
		moves.answerFaint = moves.findSetMarker(func, "Faint") - func;  // sic
	}
	int offset = pawn.bbscrCurrentInstr() - func;
	if (offset >= moves.answerFaint) {
		return assignName("6K Feint");
	} else {
		return assignName("6K");
	}
}
const NamePair* displayNameSelector_backturn(PlayerInfo& ent) {
	Entity pawn = ent.pawn;
	Moves::SemukeSubanim subanim = moves.parseSemukeSubanim(ent.pawn, Moves::SEMUKEPARSE_ANIM);
	switch (subanim) {
		case Moves::SEMUKESUBANIM_WALK_BACK: return assignName("Brynhildr Walk Back", "Backturn Walk Back");
		case Moves::SEMUKESUBANIM_WALK_FORWARD: return assignName("Brynhildr Walk Forward", "Backturn Walk Forward");
		case Moves::SEMUKESUBANIM_EXIT: return assignName("Brynhildr Exit", "Backturn Exit");
		default: return assignName("Brynhildr Stance", "Backturn");
	}
}
const NamePair* displayNameSelector_tossin2(PlayerInfo& ent) {
	BYTE* func = ent.pawn.bbscrCurrentFunc();
	if (!moves.leoTossin2FrontEnd) {
		moves.leoTossin2FrontEnd = moves.findSetMarker(func, "FrontEnd") - func;
	}
	int offset = ent.pawn.bbscrCurrentInstr() - func;
	if (ent.pawn.currentHitNum() > 0 && offset < moves.leoTossin2FrontEnd) {
		return assignName("Kaltes Gest\xc3\xb6\x62\x65r Zweit into Brynhildr", "Zweit into Backturn");
	} else {
		return assignName("Kaltes Gest\xc3\xb6\x62\x65r Zweit", "Zweit");
	}
}
const NamePair* displayNameSelector_tossin2Hasei(PlayerInfo& ent) {
	BYTE* func = ent.pawn.bbscrCurrentFunc();
	if (!moves.leoTossin2HaseiFrontEnd) {
		moves.leoTossin2HaseiFrontEnd = moves.findSetMarker(func, "FrontEnd") - func;
	}
	int offset = ent.pawn.bbscrCurrentInstr() - func;
	if (ent.pawn.currentHitNum() > 0 && offset < moves.leoTossin2HaseiFrontEnd) {
		return assignName("Kaltes Gest\xc3\xb6\x62\x65r Zweit (Follow-up) into Brynhildr", "> Zweit into Backturn");
	} else {
		return assignName("Kaltes Gest\xc3\xb6\x62\x65r Zweit (Follow-up)", "> Zweit");
	}
}
const NamePair* displayNameSelector_leo5H(PlayerInfo& ent) {
	BYTE* func = ent.pawn.bbscrCurrentFunc();
	if (!moves.leo5HKamae) {
		moves.leo5HKamae = moves.findSetMarker(func, "Kamae") - func;
	}
	int offset = ent.pawn.bbscrCurrentInstr() - func;
	if (offset > moves.leo5HKamae) {
		return assignName("5H into Brynhildr", "5H into Backturn");
	} else {
		return assignName("5H");
	}
}
const NamePair* displayNameSelector_leo6H(PlayerInfo& ent) {
	BYTE* func = ent.pawn.bbscrCurrentFunc();
	if (!moves.leo6HKamae) {
		moves.leo6HKamae = moves.findSetMarker(func, "Kamae") - func;
	}
	int offset = ent.pawn.bbscrCurrentInstr() - func;
	if (offset > moves.leo6HKamae) {
		return assignName("6H into Brynhildr", "6H into Backturn");
	} else {
		return assignName("6H");
	}
}
const NamePair* displayNameSelector_gorengeki(PlayerInfo& ent) {
	BYTE* func = ent.pawn.bbscrCurrentFunc();
	if (!moves.leoGorengekiKamae) {
		moves.leoGorengekiKamae = moves.findSetMarker(func, "SemukeKamae") - func;
	}
	int offset = ent.pawn.bbscrCurrentInstr() - func;
	if (offset > moves.leoGorengekiKamae) {
		return assignName("Leidenschaft Dirigent into Brynhildr", "Leidenschaft into Backturn");
	} else {
		return assignName("Leidenschaft Dirigent", "Leidenschaft");
	}
}
const NamePair* displayNameSelector_treasureHunt(PlayerInfo& ent) {
	if (ent.pawn.dealtAttack()->guardType == GUARD_TYPE_NONE) {
		return assignName("Max Treasure Hunt", "Max TH");
	} else {
		return assignName("Treasure Hunt", "TH");
	}
}
const NamePair* displayNameSelector_stepTreasureHunt(PlayerInfo& ent) {
	if (ent.pawn.dealtAttack()->guardType == GUARD_TYPE_NONE) {
		return assignName("Max Stance Dash Treasure Hunt", "Max SDTH");
	} else {
		return assignName("Stance Dash Treasure Hunt", "SDTH");
	}
}
static const NamePair* displayNameSelector_asanagi(PlayerInfo& ent, int* storage, const NamePair* namePair, const NamePair* namePairMax) {
	Entity pawn = ent.pawn;
	BYTE* func = pawn.bbscrCurrentFunc();
	if (!*storage) {
		*storage = moves.findSetMarker(func, "LongHold") - func;
	}
	int offset = pawn.bbscrCurrentInstr() - func;
	if (offset >= *storage) {
		return namePairMax;
	} else {
		return namePair;
	}
}
const NamePair* displayNameSelector_asanagiB(PlayerInfo& ent) {
	return displayNameSelector_asanagi(ent, &moves.jamKCardLongHold, assignName("K Asanagi no Kokyuu", "K-Card"), assignName("Max K Asanagi no Kokyuu", "Max K-Card"));
}
const NamePair* displayNameSelector_asanagiC(PlayerInfo& ent) {
	return displayNameSelector_asanagi(ent, &moves.jamSCardLongHold, assignName("S Asanagi no Kokyuu", "S-Card"), assignName("Max S Asanagi no Kokyuu", "Max S-Card"));
}
const NamePair* displayNameSelector_asanagiD(PlayerInfo& ent) {
	return displayNameSelector_asanagi(ent, &moves.jamHCardLongHold, assignName("H Asanagi no Kokyuu", "H-Card"), assignName("Max H Asanagi no Kokyuu", "Max H-Card"));
}
const NamePair* displayNameSelector_antiAir4Hasei(PlayerInfo& ent) {
	if (ent.pawn.mem45()) {
		return assignName("Four Tigers Sword (Reverse Ver. Charged)", "Charge Shinken");
	} else {
		return assignName("Four Tigers Sword (Reverse Ver.)", "Shinken");
	}
}
const NamePair* displayNameSelector_antiAir6Hasei(PlayerInfo& ent) {
	Entity pawn = ent.pawn;
	BYTE* func = pawn.bbscrCurrentFunc();
	if (!moves.haehyunAntiAir6HaseiLanding) {
		moves.haehyunAntiAir6HaseiLanding = moves.findSetMarker(func, "landing") - func;
		moves.haehyunAntiAir6HaseiHeavyAttack = moves.findSetMarker(func, "HeavyAttack") - func;
	}
	int offset = pawn.bbscrCurrentInstr() - func;
	if (offset > moves.haehyunAntiAir6HaseiLanding && pawn.dealtAttack()->hitstop == 22
			|| offset > moves.haehyunAntiAir6HaseiHeavyAttack
			|| strcmp(pawn.gotoLabelRequests(), "HeavyAttack") == 0
			|| pawn.mem45()
			&& pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED)) {
		return assignName("Four Tigers Sword (Charged)", "Charged Grampa Viper");
	} else {
		return assignName("Four Tigers Sword (Hold)", "Grampa Viper");
	}
}
const NamePair* displayNameSelector_landBlow4Hasei(PlayerInfo& ent) {
	Entity pawn = ent.pawn;
	BYTE* func = pawn.bbscrCurrentFunc();
	if (!moves.haehyunLandBlow4HaseiAttack2) {
		moves.haehyunLandBlow4HaseiAttack2 = moves.findSetMarker(func, "Attack2") - func;
	}
	int offset = pawn.bbscrCurrentInstr() - func;
	if (offset > moves.haehyunLandBlow4HaseiAttack2
			|| strcmp(pawn.gotoLabelRequests(), "Attack2") == 0
			|| pawn.mem45()
			&& pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED)) {
		return assignName("Falcon Dive (Reverse Ver. Charged)", "Hayabusa (Reverse Charged)");
	} else {
		return assignName("Falcon Dive (Reverse Ver.)", "Hayabusa (Reverse)");
	}
}
const NamePair* displayNameSelector_landBlow6Hasei(PlayerInfo& ent) {
	Entity pawn = ent.pawn;
	BYTE* func = pawn.bbscrCurrentFunc();
	if (!moves.haehyunLandBlow6HaseiAttack2) {
		moves.haehyunLandBlow6HaseiAttack2 = moves.findSetMarker(func, "Attack2") - func;
	}
	int offset = pawn.bbscrCurrentInstr() - func;
	if (offset > moves.haehyunLandBlow6HaseiAttack2
			|| strcmp(pawn.gotoLabelRequests(), "Attack2") == 0
			|| pawn.mem45()
			&& pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED)) {
		return assignName("Falcon Dive (Charged)", "Hayabusa (Charged)");
	} else {
		return assignName("Falcon Dive (Held)", "Hayabusa (Held)");
	}
}
const NamePair* displayNameSelector_blackHoleAttack(PlayerInfo& ent) {
	if (ent.pawn.dealtAttack()->guardType == GUARD_TYPE_NONE) {
		return assignName("Max Enlightened 3000 Palm Strike", "Max Clap Super");
	} else {
		return assignName("Enlightened 3000 Palm Strike", "Clap Super");
	}
}
const NamePair* displayNameSelector_blackHoleAttackBurst(PlayerInfo& ent) {
	if (ent.pawn.dealtAttack()->guardType == GUARD_TYPE_NONE) {
		return assignName("Max Burst Enlightened 3000 Palm Strike", "Max Burst Clap Super");
	} else {
		return assignName("Burst Enlightened 3000 Palm Strike", "Burst Clap Super");
	}
}
const NamePair* displayNameSelector_landBlowAttack(PlayerInfo& ent) {
	if (ent.pawn.inflictedCH()->tumbleDuration != INT_MAX) {
		return assignName("Grausam Impuls Lv3", "Scratch Lv3");
	} else if (ent.pawn.inflictedCH()->wallbounceCount != INT_MAX) {
		return assignName("Grausam Impuls Lv2", "Scratch Lv2");
	} else {
		return assignName("Grausam Impuls Lv1", "Scratch Lv1");
	}
}
const NamePair* displayNameSelector_airBlowAttack(PlayerInfo& ent) {
	if (ent.pawn.inflicted()->impulseY == 36000) {
		return assignName("Air Grausam Impuls Lv3", "j.Scratch Lv3");
	} else if (ent.pawn.inflicted()->impulseY == 34000) {
		return assignName("Air Grausam Impuls Lv2", "j.Scratch Lv2");
	} else {
		return assignName("Air Grausam Impuls Lv1", "j.Scratch Lv1");
	}
}
#define displayNameSelector_commandThrowImpl(baseName, baseNameSlang) \
	int resource = ent.pawn.exGaugeValue(0); \
	if (resource >= 6) { \
		return assignName(baseName " Lv3", baseNameSlang " Lv3"); \
	} else if (resource >= 3) { \
		return assignName(baseName " Lv2", baseNameSlang " Lv2"); \
	} else { \
		return assignName(baseName " Lv1", baseNameSlang " Lv1"); \
	}
#define displayNameSelector_commandThrowExImpl(baseName, baseNameSlang) \
	int value = ent.pawn.inflicted()->impulseY; \
	if (value == 0) { \
		int resource = ent.pawn.exGaugeValue(0); \
		if (resource >= 6) { \
			return assignName(baseName " Lv3", baseNameSlang " Lv3"); \
		} else if (resource >= 3) { \
			return assignName(baseName " Lv2", baseNameSlang " Lv2"); \
		} else { \
			return assignName(baseName " Lv1", baseNameSlang " Lv1"); \
		} \
	} else if (value == 35000) { \
		return assignName(baseName " Lv3", baseNameSlang " Lv3"); \
	} else if (value == 30000) { \
		return assignName(baseName " Lv2", baseNameSlang " Lv2"); \
	} else { \
		return assignName(baseName " Lv1", baseNameSlang " Lv1"); \
	}
const NamePair* displayNameSelector_commandThrow(PlayerInfo& ent) {
	displayNameSelector_commandThrowImpl("H Wachen Zweig", "Command Grab")
}
const NamePair* displayNameSelector_commandThrowEx(PlayerInfo& ent) {
	displayNameSelector_commandThrowExImpl("H Wachen Zweig", "Command Grab")
}
const NamePair* displayNameSelector_antiAirCommandThrow(PlayerInfo& ent) {
	displayNameSelector_commandThrowImpl("S Wachen Zweig", "S Grab")
}
const NamePair* displayNameSelector_antiAirCommandThrowEx(PlayerInfo& ent) {
	displayNameSelector_commandThrowExImpl("S Wachen Zweig", "S Grab")
}
const NamePair* displayNameSelector_dizzy6H(PlayerInfo& ent) {
	// 6H becomes an overhead only in Rev2
	if (strcmp(ent.pawn.dealtAttack()->trialName, "NmlAtk6DHold") == 0) {
		return assignName("Max 6H");
	} else {
		return assignName("6H");
	}
}
const NamePair* displayNameSelector_kinomiNecro(PlayerInfo& ent) {
	Entity pawn = ent.pawn;
	BYTE* func = pawn.bbscrCurrentFunc();
	moves.fillInKinomiNecroChargePrereq(func);
	BYTE* currentInstr = pawn.bbscrCurrentInstr();
	int offset = currentInstr - func;
	int frame = pawn.currentAnimDuration();
	if (offset > moves.dizzyKinomiNecroEndOffset) {
		int beforeYou = 0;
		int prevDur = 0;
		for (loopInstr(func + moves.dizzyKinomiNecroEndOffset)) {
			if (instr < currentInstr) {
				beforeYou += prevDur;
				prevDur = asInstr(instr, sprite)->duration;
			} else {
				break;
			}
		}
		frame -= beforeYou + pawn.spriteFrameCounter() + 1;
	}
	if (frame >= moves.dizzyKinomiNecroSpear3) {
		return assignName("For roasting chestnuts... Lv3", "Fire Spears Lv3");
	} else if (frame >= moves.dizzyKinomiNecroSpear2) {
		return assignName("For roasting chestnuts... Lv2", "Fire Spears Lv2");
	} else {
		return assignName("For roasting chestnuts... Lv1", "Fire Spears Lv1");
	}
}

const char* canYrcProjectile_default(PlayerInfo& player) {
	if (player.prevFrameHadDangerousNonDisabledProjectiles
			&& player.hasDangerousNonDisabledProjectiles) {
		return "Can YRC, and projectile/powerup will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_gunflame(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Gunflame will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_tyrantRave(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Tyrant Rave second punch will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_cse(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Charged Stun Edge will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_se(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Stun Edge will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_sacredEdge(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Sacred Edge will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_prevNoLinkDestroyOnStateChange(PlayerInfo& player) {
	if (player.prevFrameHadDangerousNonDisabledProjectiles
			&& player.hasDangerousNonDisabledProjectiles
			&& (
				player.pawn.previousEntity() == nullptr
				|| player.pawn.previousEntity().linkObjectDestroyOnStateChange() != player.pawn
			)
			&& !player.prevFramePreviousEntityLinkObjectDestroyOnStateChangeWasEqualToPlayer) {
		return "Can YRC, and projectile/powerup will stay";
	}
	return nullptr;
}
const CreatedProjectileStruct* createdProjectile_ky5D(PlayerInfo& player) {
	if (player.pawn.previousEntity()
		&& player.pawn.previousEntity().lifeTimeCounter() == 0
		&& !player.pawn.isRCFrozen()) {
		if (strcmp(player.pawn.previousEntity().animationName(), "Mahojin") == 0) {
			return assignCreatedProjectile("Created Grinder");
		} else if (strcmp(player.pawn.previousEntity().animationName(), "DustEffectShot") == 0) {
			return assignCreatedProjectile("Created 5D Projectile");
		}
	}
	return nullptr;
}
const char* canYrcProjectile_ky5D(PlayerInfo& player) {
	// STACK_1 seems to hold the ground mahojin
	if (moves.ky5DDustEffectShot_firstSpriteAfter_Offset == -1) return nullptr; // rev1
	BYTE* func = player.pawn.bbscrCurrentFunc();
	if (!moves.ky5DDustEffectShot_firstSpriteAfter_Offset && func) {
		BYTE* pos = moves.findCreateObj(func, "DustEffectShot");
		if (!pos) {
			moves.ky5DDustEffectShot_firstSpriteAfter_Offset = -1;
			if (canYrcProjectile_default(player)) {  // rev1
				return "Can YRC, and Grinder will stay";
			}
			return nullptr;
		}
		pos = moves.findSpriteNonNull(pos);
		if (pos) moves.ky5DDustEffectShot_firstSpriteAfter_Offset = pos - func;
	}
	if (!moves.ky5DDustEffectShot_firstSpriteAfter_Offset) return nullptr;
	if (player.pawn.bbscrCurrentInstr() > func + moves.ky5DDustEffectShot_firstSpriteAfter_Offset
			|| player.pawn.bbscrCurrentInstr() == func + moves.ky5DDustEffectShot_firstSpriteAfter_Offset
			&& player.pawn.spriteFrameCounter() > 0) {
		return "Can YRC, and 5D Projectile will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_kyJD(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Grinder will stay";
	}
	return nullptr;
}
const CreatedProjectileStruct* createdProjectile_splitCiel(PlayerInfo& player) {
	if (player.pawn.previousEntity()
		&& strcmp(player.pawn.previousEntity().animationName(), "Mahojin") == 0
		&& player.pawn.previousEntity().lifeTimeCounter() == 0
		&& !player.pawn.isRCFrozen()) {
		return assignCreatedProjectile("Created Grinder");
	}
	return nullptr;
}
const char* canYrcProjectile_splitCiel(PlayerInfo& player) {
	if (player.pawn.previousEntity()
			&& strcmp(player.pawn.previousEntity().animationName(), "Mahojin") == 0
			&& player.pawn.previousEntity().lifeTimeCounter() > 0) {
		return "Can YRC, and Grinder will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_coin(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Coin will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_bacchusSigh(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Bacchus Sigh will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_sinwazaShot(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Zweihander Pillar will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_beachBall(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Beach Ball will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_dolphin(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Dolphin will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_yamada(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Yamada will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_wallclingKunai(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Kunai will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_shuriken(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Shuriken will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_gammaBlade(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Gamma Blade will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_ryuuYanagi(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Ryuu Yanagi will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_faust5D(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and the reflected projectile will stay";
	}
	return nullptr;
}
static const char* canYrcProjectile_itemTossImpl(PlayerInfo& player, int* storage) {
	BYTE* func = player.pawn.bbscrCurrentFunc();
	if (!*storage) {
		bool foundCreate = false;
		for (loopInstr(func)) {
			InstrType type = moves.instrType(instr);
			if (type == instr_createObjectWithArg
					&& strcmp(asInstr(instr, createObjectWithArg)->name, "Item_Pre") == 0) {
				foundCreate = true;
			} else if (foundCreate && type == instr_sprite) {
				*storage = instr - func;
				break;
			}
		}
	}
	if (!*storage) return nullptr;
	int offset = player.pawn.bbscrCurrentInstr() - func;
	if (offset > *storage
			|| offset == *storage && !player.pawn.justReachedSprite()) {
		return "Can YRC, and the tossed item will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_itemToss(PlayerInfo& player) {
	return canYrcProjectile_itemTossImpl(player, &moves.faustItemToss);
}
const char* canYrcProjectile_pogoItemToss(PlayerInfo& player) {
	return canYrcProjectile_itemTossImpl(player, &moves.faustPogoItemToss);
}
const char* canYrcProjectile_love(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Love will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_superToss(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and the items that have been tossed so far will stay (remaining items would not get tossed)";
	}
	return nullptr;
}
const char* canYrcProjectile_flower(PlayerInfo& player) {
	for (int i = 2; i < entityList.count; ++i) {
		Entity p = entityList.list[i];
		if (p.isActive() && p.team() == player.index && !p.isPawn()
				&& (
					strcmp(p.animationName(), "OreHanaBig_Shot") == 0
					|| strcmp(p.animationName(), "OreHana_Shot") == 0
				)
				&& p.lifeTimeCounter() > 0
				&& !p.isActiveFrames()) {  // the last check against potential get on pogo -> flower -> YRC -> get on pogo -> flower again
			return "Can YRC, and Flower will stay";
		}
	}
	return nullptr;
}
const char* canYrcProjectile_sickleFlash(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Sickle Flash will stay";
	}
	return nullptr;
}
static const char* canYrcProjectile_qv(PlayerInfo& player, int* storage, const char* result) {
	BYTE* func = player.pawn.bbscrCurrentFunc();
	if (!func) return nullptr;
	if (*storage == 0) {
		bool foundTheThing = false;
		for (loopInstr(func)) {
			InstrType type = moves.instrType(instr);
			if (type == instr_clearUpon) {
				if (asInstr(instr, clearUpon)->event == BBSCREVENT_PLAYER_CHANGED_STATE) {
					foundTheThing = true;
				}
			} else if (foundTheThing && type == instr_sprite) {
				*storage = instr - func;
				break;
			}
		}
	}
	if (!*storage) return nullptr;
	BYTE* currentInstr = player.pawn.bbscrCurrentInstr();
	BYTE* minInstr = func + *storage;
	if (currentInstr == minInstr
			&& player.pawn.spriteFrameCounter() != 0
			|| currentInstr > minInstr) {
		return result;
	}
	return nullptr;
}
const char* canYrcProjectile_qvA(PlayerInfo& player) {
	return canYrcProjectile_qv(player, &moves.venomQvAClearUponAfterExitOffset, "Can YRC, and P Ball and QV Shockwave will stay");
}
const char* canYrcProjectile_qvB(PlayerInfo& player) {
	return canYrcProjectile_qv(player, &moves.venomQvBClearUponAfterExitOffset, "Can YRC, and K Ball and QV Shockwave will stay");
}
const char* canYrcProjectile_qvC(PlayerInfo& player) {
	return canYrcProjectile_qv(player, &moves.venomQvCClearUponAfterExitOffset, "Can YRC, and S Ball and QV Shockwave will stay");
}
const char* canYrcProjectile_qvD(PlayerInfo& player) {
	return canYrcProjectile_qv(player, &moves.venomQvDClearUponAfterExitOffset, "Can YRC, and H Ball and QV Shockwave will stay");
}
const char* canYrcProjectile_redHail(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Red Hail balls that have been created so far will stay, while no new balls will be created";
	}
	return nullptr;
}
const char* canYrcProjectile_darkAngel(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Dark Angel will stay";
	}
	return nullptr;
}
const CreatedProjectileStruct* createdProjectile_bishop(PlayerInfo& player) {
	if (player.pawn.previousEntity() && player.pawn.previousEntity().lifeTimeCounter() == 0) {
		return assignCreatedProjectile(assignName("Created Bishop Runout", "Created Bishop"));
	}
	return nullptr;
}
const char* canYrcProjectile_bishop(PlayerInfo& player) {
	BYTE* func = player.pawn.bbscrCurrentFunc();
	if (moves.venomBishopCreateOffset == 0 && func) {
		bool found = false;
		for (loopInstr(func)) {
			if (moves.instrType(instr) == instr_createObjectWithArg && strcmp(asInstr(instr, createObjectWithArg)->name, "Ball") == 0) {
				found = true;
			} else if (found && moves.instrType(instr) == instr_sprite) {
				moves.venomBishopCreateOffset = instr - func;
				break;
			}
		}
	}
	if (!moves.venomBishopCreateOffset) return nullptr;
	if (player.pawn.bbscrCurrentInstr() - func > moves.venomBishopCreateOffset) {
		return "Can YRC, and Bishop will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_helterSkelter(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Helter Skelter shockwave will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_sdd(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Straight Down Dandy back jet projectile will stay";
	}
	return nullptr;
}
const CreatedProjectileStruct* createdProjectile_ino5D(PlayerInfo& player) {
	if (player.pawn.previousEntity()
		&& player.pawn.previousEntity().lifeTimeCounter() == 0
		&& !player.pawn.isRCFrozen()
		&& strcmp(player.pawn.previousEntity().animationName(), "DustObjShot") == 0) {
		return assignCreatedProjectile("Created 5D Projectile");
	}
	return nullptr;
}
const char* canYrcProjectile_ino5D(PlayerInfo& player) {
	if (moves.ino5DCreateDustObjShotOffset == -1) return nullptr; // rev1
	BYTE* func = player.pawn.bbscrCurrentFunc();
	if (!moves.ino5DCreateDustObjShotOffset && func) {
		BYTE* pos = moves.findCreateObj(func, "DustObjShot");
		if (!pos) {
			moves.ino5DCreateDustObjShotOffset = -1;
			return nullptr; // rev1
		}
		pos = moves.findSpriteNonNull(pos);
		if (pos) moves.ino5DCreateDustObjShotOffset = pos - func;
	}
	if (!moves.ino5DCreateDustObjShotOffset) return nullptr;
	if (player.pawn.bbscrCurrentInstr() > func + moves.ino5DCreateDustObjShotOffset
			|| player.pawn.bbscrCurrentInstr() == func + moves.ino5DCreateDustObjShotOffset
			&& player.pawn.spriteFrameCounter() > 0) {
		return "Can YRC, and 5D Projectile will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_kouutsuOnkai(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Antidepressant Scale will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_chemicalLove(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Chemical Love projectile will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_madogiwa(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and the Longing Desperation blast will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_genkai(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Genkai will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_boomerang(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Boomerang Head will still be thrown";
	}
	return nullptr;
}
const char* canYrcProjectile_dejavuAB(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and the \x44\xC3\xA9\x6A\xC3\xA0 Vu Ghost will stay, which could later spawn the Ghost Boomerang Head";
	}
	return nullptr;
}
const char* canYrcProjectile_dejavuC(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and the \x44\xC3\xA9\x6A\xC3\xA0 Vu B Ghost will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_dejavuD(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and the \x44\xC3\xA9\x6A\xC3\xA0 Vu C Ghost will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_alarm(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Sinusoidal Helios will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_merry(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Hami Jack will stay";
	}
	return nullptr;
}
const CreatedProjectileStruct* createdProjectile_onf5_s(PlayerInfo& ent) {
	if (ent.pawn.currentAnimDuration() == 5 && !ent.pawn.isRCFrozen()) {
		return assignCreatedProjectile("Summoned S Sword");
	}
	return nullptr;
}
const CreatedProjectileStruct* createdProjectile_onf5_s_recall(PlayerInfo& ent) {
	if (ent.pawn.currentAnimDuration() == 5 && !ent.pawn.isRCFrozen()) {
		return assignCreatedProjectile("Recalled S Sword");
	}
	return nullptr;
}
const CreatedProjectileStruct* createdProjectile_onf5_h(PlayerInfo& ent) {
	if (ent.pawn.currentAnimDuration() == 5 && !ent.pawn.isRCFrozen()) {
		return assignCreatedProjectile("Summoned H Sword");
	}
	return nullptr;
}
const CreatedProjectileStruct* createdProjectile_onf5_h_recall(PlayerInfo& ent) {
	if (ent.pawn.currentAnimDuration() == 5 && !ent.pawn.isRCFrozen()) {
		return assignCreatedProjectile("Recalled H Sword");
	}
	return nullptr;
}
const char* canYrcProjectile_onf5_sLaunch(PlayerInfo& ent) {
	if (ent.pawn.currentAnimDuration() == 5 && ent.pawn.isRCFrozen()
			|| ent.pawn.currentAnimDuration() > 5) {
		return "Can YRC, and S Sword will still be deployed";
	}
	return nullptr;
}
const char* canYrcProjectile_onf5_sRelaunch(PlayerInfo& ent) {
	if (ent.pawn.currentAnimDuration() == 5 && ent.pawn.isRCFrozen()
			|| ent.pawn.currentAnimDuration() > 5) {
		return "Can YRC, and S Sword will still be redeployed";
	}
	return nullptr;
}
const char* canYrcProjectile_onf5_sRecover(PlayerInfo& ent) {
	if (ent.pawn.currentAnimDuration() == 5 && ent.pawn.isRCFrozen()
			|| ent.pawn.currentAnimDuration() > 5) {
		return "Can YRC, and S Sword will still be recovered";
	}
	return nullptr;
}
const char* canYrcProjectile_onf5_hLaunch(PlayerInfo& ent) {
	if (ent.pawn.currentAnimDuration() == 5 && ent.pawn.isRCFrozen()
			|| ent.pawn.currentAnimDuration() > 5) {
		return "Can YRC, and H Sword will still be deployed";
	}
	return nullptr;
}
const char* canYrcProjectile_onf5_hRelaunch(PlayerInfo& ent) {
	if (ent.pawn.currentAnimDuration() == 5 && ent.pawn.isRCFrozen()
			|| ent.pawn.currentAnimDuration() > 5) {
		return "Can YRC, and H Sword will still be redeployed";
	}
	return nullptr;
}
const char* canYrcProjectile_onf5_hRecover(PlayerInfo& ent) {
	if (ent.pawn.currentAnimDuration() == 5 && ent.pawn.isRCFrozen()
			|| ent.pawn.currentAnimDuration() > 5) {
		return "Can YRC, and H Sword will still be recovered";
	}
	return nullptr;
}
const CreatedProjectileStruct* createdProjectile_onf7_s(PlayerInfo& ent) {
	if (ent.pawn.currentAnimDuration() == 7 && !ent.pawn.isRCFrozen()) {
		return assignCreatedProjectile("Summoned S Sword");
	}
	return nullptr;
}
const CreatedProjectileStruct* createdProjectile_onf7_h(PlayerInfo& ent) {
	if (ent.pawn.currentAnimDuration() == 7 && !ent.pawn.isRCFrozen()) {
		return assignCreatedProjectile("Summoned H Sword");
	}
	return nullptr;
}
const char* canYrcProjectile_onf7_sLaunch(PlayerInfo& ent) {
	if (ent.pawn.currentAnimDuration() == 7 && ent.pawn.isRCFrozen()
			|| ent.pawn.currentAnimDuration() > 7) {
		return "Can YRC, and S Sword will still be deployed";
	}
	return nullptr;
}
const char* canYrcProjectile_onf7_sRelaunch(PlayerInfo& ent) {
	if (ent.pawn.currentAnimDuration() == 7 && ent.pawn.isRCFrozen()
			|| ent.pawn.currentAnimDuration() > 7) {
		return "Can YRC, and S Sword will still be redeployed";
	}
	return nullptr;
}
const char* canYrcProjectile_onf7_sRecover(PlayerInfo& ent) {
	if (ent.pawn.currentAnimDuration() == 7 && ent.pawn.isRCFrozen()
			|| ent.pawn.currentAnimDuration() > 7) {
		return "Can YRC, and S Sword will still be recovered";
	}
	return nullptr;
}
const char* canYrcProjectile_onf7_hLaunch(PlayerInfo& ent) {
	if (ent.pawn.currentAnimDuration() == 7 && ent.pawn.isRCFrozen()
			|| ent.pawn.currentAnimDuration() > 7) {
		return "Can YRC, and H Sword will still be deployed";
	}
	return nullptr;
}
const char* canYrcProjectile_onf7_hRelaunch(PlayerInfo& ent) {
	if (ent.pawn.currentAnimDuration() == 7 && ent.pawn.isRCFrozen()
			|| ent.pawn.currentAnimDuration() > 7) {
		return "Can YRC, and H Sword will still be redeployed";
	}
	return nullptr;
}
const char* canYrcProjectile_onf7_hRecover(PlayerInfo& ent) {
	if (ent.pawn.currentAnimDuration() == 7 && ent.pawn.isRCFrozen()
			|| ent.pawn.currentAnimDuration() > 7) {
		return "Can YRC, and H Sword will still be recovered";
	}
	return nullptr;
}
const char* canYrcProjectile_bitSpiral(PlayerInfo& ent) {
	if (ent.pawn.currentAnimDuration() > 9) {
		return "Can YRC, and Trance will still be deployed";
	}
	return nullptr;
}
const CreatedProjectileStruct* createdProjectile_elpheltjD(PlayerInfo& player) {
	if (player.pawn.previousEntity()
			&& player.pawn.previousEntity().lifeTimeCounter() == 0
			&& !player.pawn.isRCFrozen()) {
		for (int i = 2; i < entityList.count; ++i) {
			Entity p = entityList.list[i];
			if (p.isActive() && p.team() == player.index && !p.isPawn()
					&& strcmp(p.animationName(), "HandGun_air_shot") == 0
					&& p.lifeTimeCounter() == 0) {
				return assignCreatedProjectile("Created j.D Projectile");
			}
		}
	}
	return nullptr;
}
const char* canYrcProjectile_elpheltjD(PlayerInfo& player) {
	if (player.pawn.effectLinkedCollision() != nullptr
			&& player.pawn.previousEntity()
			&& player.pawn.previousEntity().lifeTimeCounter() > 0) {
		for (int i = 2; i < entityList.count; ++i) {
			Entity p = entityList.list[i];
			if (p.isActive() && p.team() == player.index && !p.isPawn()
					&& strcmp(p.animationName(), "HandGun_air_shot") == 0) {
				return "Can YRC, and j.D Projectile will stay";
			}
		}
	}
	return nullptr;
}
const char* canYrcProjectile_rifleFire(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, without interrupting the Rifle Shot";
	}
	return nullptr;
}
const char* canYrcProjectile_grenadeToss(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Berry Pine will still be thrown";
	}
	return nullptr;
}
static inline const CreatedProjectileStruct* createdProjectile_Ghost(PlayerInfo& player, int index, const CreatedProjectileStruct* msg) {
	Entity p = player.pawn.stackEntity(index);
	if (p && p.isActive() && p.displayModel() && p.posY() == 0 && p.mem45() != 1 && !player.pawn.isRCFrozen()) {
		return msg;
	}
	return nullptr;
}
static const char* canYrcProjectile_Ghost(PlayerInfo& player, int index, const char* result) {
	Entity p = player.pawn.stackEntity(index);
	if (p && p.isActive() && p.displayModel() && p.mem45() == 1) {
		return result;
	}
	return nullptr;
}
const CreatedProjectileStruct* createdProjectile_PGhost(PlayerInfo& player) {
	return createdProjectile_Ghost(player, 0, assignCreatedProjectile("Created P Ghost"));
}
const char* canYrcProjectile_PGhost(PlayerInfo& player) {
	return canYrcProjectile_Ghost(player, 0, "Can YRC, and P Ghost will remain summoned");
}
const CreatedProjectileStruct* createdProjectile_KGhost(PlayerInfo& player) {
	return createdProjectile_Ghost(player, 1, assignCreatedProjectile("Created K Ghost"));
}
const char* canYrcProjectile_KGhost(PlayerInfo& player) {
	return canYrcProjectile_Ghost(player, 1, "Can YRC, and K Ghost will remain summoned");
}
const CreatedProjectileStruct* createdProjectile_SGhost(PlayerInfo& player) {
	return createdProjectile_Ghost(player, 2, assignCreatedProjectile("Created S Ghost"));
}
const char* canYrcProjectile_SGhost(PlayerInfo& player) {
	return canYrcProjectile_Ghost(player, 2, "Can YRC, and S Ghost will remain summoned");
}
static const CreatedProjectileStruct* createdProjectile_XThrowGhost(PlayerInfo& player, int* offset) {
	BYTE* func = player.pawn.bbscrCurrentFunc();
	if (!func) return nullptr;
	moves.fillJackoThrowGhostOffset(func, offset);
	if (player.pawn.bbscrCurrentInstr() - func == *offset && player.pawn.justReachedSprite()) {
		return assignCreatedProjectile("Threw Ghost");
	}
	return nullptr;
}
const char* canYrcProjectile_XThrowGhost(PlayerInfo& player, int* offset) {
	int mem59 = player.pawn.mem59();
	if (mem59 != 1 && mem59 != 2 && mem59 != 3) return nullptr;
	Entity p = player.pawn.stackEntity(mem59 - 1);
	if (!p || !p.isActive()) return nullptr;
	if (!createdProjectile_XThrowGhost(player, offset)
			&& p.isActiveFrames()) {
		return "Can YRC, and Ghost will remain thrown";
	}
	return nullptr;
}
const CreatedProjectileStruct* createdProjectile_ThrowGhost(PlayerInfo& player) {
	return createdProjectile_XThrowGhost(player, &moves.jackoThrowGhostOffset);
}
const char* canYrcProjectile_ThrowGhost(PlayerInfo& player) {
	return canYrcProjectile_XThrowGhost(player, &moves.jackoThrowGhostOffset);
}
const CreatedProjectileStruct* createdProjectile_AirThrowGhost(PlayerInfo& player) {
	return createdProjectile_XThrowGhost(player, &moves.jackoAirThrowGhostOffset);
}
const char* canYrcProjectile_returnGhost(PlayerInfo& player) {
	BYTE* func = player.pawn.bbscrCurrentFunc();
	findSpriteAfterIf(func, &moves.jackoReturnGhost);
	int offset = player.pawn.bbscrCurrentInstr() - func;
	if (offset >= moves.jackoReturnGhost
			&& !(offset == moves.jackoReturnGhost && player.pawn.justReachedSprite())) {
		return "Can YRC, and the Ghost will remain in the inventory";
	}
	return nullptr;
}
const char* canYrcProjectile_AirThrowGhost(PlayerInfo& player) {
	return canYrcProjectile_XThrowGhost(player, &moves.jackoAirThrowGhostOffset);
}
struct PickUpGhostAnalysisResult {
	bool onThisFramePickedUpGhost;
	const char* yrcMessage;
};
void analyzePickUpGhost(PlayerInfo& player, PickUpGhostAnalysisResult* result) {
	result->onThisFramePickedUpGhost = false;
	result->yrcMessage = nullptr;
	
	int mem59 = player.pawn.mem59();
	if (ghostStateName_PickUp < 1 || mem59 < 1 || mem59 > 3) {
		return;
	}
	static struct {
		std::vector<int>& offsets;
		int& pickedUp;
		const char* ghostState;
	} allThings[3] {
		{
			moves.ghostAStateOffsets,
			moves.ghostABecomePickedUp,
			"GhostA"
		},
		{
			moves.ghostBStateOffsets,
			moves.ghostBBecomePickedUp,
			"GhostB"
		},
		{
			moves.ghostCStateOffsets,
			moves.ghostCBecomePickedUp,
			"GhostC"
		}
	};
	auto& thing = allThings[mem59 - 1];
	
	Entity ghost;
	entityList.populate();
	for (int i = 2; i < entityList.count; ++i) {
		Entity ent = entityList.list[i];
		if (ent.isActive() && ent.team() == player.index && strcmp(ent.animationName(), thing.ghostState) == 0) {
			ghost = ent;
			break;
		}
	}
	if (!ghost) return;
	
	BYTE* ghostFunc = ghost.bbscrCurrentFunc();
	moves.fillGhostStateOffsets(ghostFunc, thing.offsets);
	
	if (ghostStateName_PickUp - 1 >= (int)thing.offsets.size()) return;
	int firstOffset = thing.offsets[ghostStateName_PickUp - 1];
	
	if (!thing.pickedUp) {
		BYTE* firstInstr = ghostFunc + firstOffset;
		bool encounteredStoreValue = false;
		for (loopInstr(firstInstr)) {
			InstrType type = moves.instrType(instr);
			if (type == instr_setMarker) {
				if (strcmp(asInstr(instr, setMarker)->name, "pickup") != 0) {
					break;
				}
			} else if (type == instr_storeValue) {
				if (asInstr(instr, storeValue)->dest == MEM(45)
						&& asInstr(instr, storeValue)->src == AccessedValue(BBSCRTAG_VALUE, 6)) {
					encounteredStoreValue = true;
				}
			} else if (encounteredStoreValue) {
				if (type == instr_sprite) {
					thing.pickedUp = instr - ghostFunc;
					break;
				} else if (type == instr_spriteEnd) {
					instr = moves.skipInstr(instr);
					thing.pickedUp = instr - ghostFunc;
					break;
				}
			}
		}
	}
	if (!thing.pickedUp) return;
	
	int ghostOffset = ghost.bbscrCurrentInstr() - ghostFunc;
	
	int mem45 = ghost.mem45();
	
	bool onThisFramePickedUpGhost = mem45 == 6
		&& ghostOffset == thing.pickedUp
		&& ghost.justReachedSprite();
	
	result->onThisFramePickedUpGhost = onThisFramePickedUpGhost;
	
	if (mem45 == 2
			|| mem45 == 6
			&& (
				ghostOffset < thing.pickedUp
				|| onThisFramePickedUpGhost
			)) {
		result->yrcMessage = "Can YRC, and Ghost will drop down";
	} else if (mem45 == 6) {
		result->yrcMessage = "Can YRC, and Ghost will remain picked up";
	}
}
const char* canYrcProjectile_pickUpGhost(PlayerInfo& player) {
	PickUpGhostAnalysisResult result;
	analyzePickUpGhost(player, &result);
	return result.yrcMessage;
}
const char* canYrcProjectile_putGhost(PlayerInfo& player) {
	BYTE* func = player.pawn.bbscrCurrentFunc();
	findSpriteAfterIf(func, &moves.jackoPutGhost);
	int offset = player.pawn.bbscrCurrentInstr() - func;
	if (offset >= moves.jackoPutGhost
			&& !(offset == moves.jackoPutGhost && player.pawn.justReachedSprite())) {
		return "Can YRC, and Ghost will remain on the ground";
	}
	return nullptr;
}
static const char* canYrcProjectile_organImpl(Entity ent, int offset, const Moves::JackoOrgan* elem, const char* result) {
	if (offset == elem->start ? ent.spriteFrameCounter() > 0 : offset > elem->start && offset <= elem->end) {
		return result;
	}
	return nullptr;
}
const char* canYrcProjectile_organ(PlayerInfo& player) {
	BYTE* func = player.pawn.bbscrCurrentFunc();
	if (!moves.jackoOrganP.start) {
		Moves::JackoOrgan* elem = nullptr;
		bool encounteredSprite = false;
		for (loopInstr(func)) {
			InstrType type = moves.instrType(instr);
			if (type == instr_setMarker) {
				switch (asInstr(instr, setMarker)->name[0]) {
					case 'A': elem = &moves.jackoOrganP; break;
					case 'B': elem = &moves.jackoOrganK; break;
					case 'C': elem = &moves.jackoOrganS; break;
					case 'D': elem = &moves.jackoOrganH; break;
				}
				encounteredSprite = false;
			} else if (elem) {
				if (type == instr_sprite) {
					if (encounteredSprite) {
						if (!elem->start) {
							elem->start = instr - func;
						}
					}
					encounteredSprite = true;
				} else if (type == instr_exitState) {
					elem->end = instr - func;
					elem = nullptr;
					encounteredSprite = false;
				}
			}
		}
	}
	int offset = player.pawn.bbscrCurrentInstr() - func;
	const char* result;
	result = canYrcProjectile_organImpl(player.pawn, offset, &moves.jackoOrganP, "Can YRC, and all Ghosts will still be recovered");
	if (result) return result;
	result = canYrcProjectile_organImpl(player.pawn, offset, &moves.jackoOrganK, "Can YRC, and all Ghosts and Servants will remain sped up");
	if (result) return result;
	result = canYrcProjectile_organImpl(player.pawn, offset, &moves.jackoOrganS, "Can YRC, and all Ghosts will still be told to explode");
	if (result) return result;
	result = canYrcProjectile_organImpl(player.pawn, offset, &moves.jackoOrganH, "Can YRC, and Aegis Field will remain");
	if (result) return result;
	return nullptr;
}
const char* canYrcProjectile_jackoCalvados(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Calvados will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_iceSpike(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Ice Spike will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_firePillar(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Fire Pillar will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_iceScythe(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Ice Scythe will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_fireScythe(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Fire Scythe will stay";
	}
	return nullptr;
}
const CreatedProjectileStruct* createdProjectile_kum5D(PlayerInfo& player) {
	if (player.pawn.previousEntity()
			&& player.pawn.previousEntity().lifeTimeCounter() == 0
			&& !player.pawn.isRCFrozen()
			&& strcmp(player.pawn.previousEntity().animationName(), "kum_205shot") == 0) {
		return assignCreatedProjectile("Created 5D Projectile");
	}
	return nullptr;
}
const char* canYrcProjectile_kum5D(PlayerInfo& player) {
	if (moves.kum5Dcreation == -1) return nullptr; // rev1
	BYTE* func = player.pawn.bbscrCurrentFunc();
	if (!moves.kum5Dcreation && func) {
		BYTE* pos = moves.findCreateObj(func, "kum_205shot");
		if (!pos) {
			moves.kum5Dcreation = -1;
			return nullptr; // rev1
		}
		pos = moves.findSpriteNonNull(pos);
		if (pos) moves.kum5Dcreation = pos - func;
	}
	if (!moves.kum5Dcreation) return nullptr;
	if (player.pawn.bbscrCurrentInstr() > func + moves.kum5Dcreation
			|| player.pawn.bbscrCurrentInstr() == func + moves.kum5Dcreation
			&& player.pawn.spriteFrameCounter() > 0) {
		return "Can YRC, and 5D Projectile will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_tuningBall(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Tuning Ball will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_ravenOrb(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Scharf Kugel will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_ravenNeedle(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Needle will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_fish(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and summoned Fish will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_bubble(PlayerInfo& player) {
	BYTE* func = player.pawn.bbscrCurrentFunc();
	if (!moves.dizzyAwaP) {
		bool foundCreate = false;
		for (loopInstr(func)) {
			InstrType type = moves.instrType(instr);
			if (type == instr_createObjectWithArg && strcmp(asInstr(instr, createObjectWithArg)->name, "AwaPObj") == 0){
				foundCreate = true;
			} else if (type == instr_sprite && foundCreate) {
				moves.dizzyAwaP = instr - func;
				break;
			}
		}
	}
	if (!moves.dizzyAwaP) return nullptr;
	int offset = player.pawn.bbscrCurrentInstr() - func;
	if (offset >= moves.dizzyAwaP && !(
		offset == moves.dizzyAwaP && player.pawn.justReachedSprite()
	)) {
		return "Can YRC, and summoned Bubble will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_fireBubble(PlayerInfo& player) {
	BYTE* func = player.pawn.bbscrCurrentFunc();
	if (!moves.dizzyAwaK) {
		bool foundCreate = false;
		for (loopInstr(func)) {
			InstrType type = moves.instrType(instr);
			if (type == instr_createObjectWithArg && strcmp(asInstr(instr, createObjectWithArg)->name, "AwaKObj") == 0){
				foundCreate = true;
			} else if (type == instr_sprite && foundCreate) {
				moves.dizzyAwaK = instr - func;
				break;
			}
		}
	}
	if (!moves.dizzyAwaK) return nullptr;
	int offset = player.pawn.bbscrCurrentInstr() - func;
	if (offset >= moves.dizzyAwaK && !(
		offset == moves.dizzyAwaK && player.pawn.justReachedSprite()
	)) {
		return "Can YRC, and summoned Fire Bubble will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_fireSpears(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and all summoned Fire Spears so far will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_iceSpear(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Ice Spear will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_imperialRay(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Imperial Ray Spawner and Imperial Ray Pillars will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_tatami(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Tatami will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_teppou(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Yasha Gatana projectile will stay";
	}
	return nullptr;
}
const CreatedProjectileStruct* createdProjectile_baiken5D(PlayerInfo& player) {
	for (int i = 2; i < entityList.count; ++i) {
		Entity p = entityList.list[i];
		if (p.isActive() && p.team() == player.index && !p.isPawn() && strcmp(p.animationName(), "NmlAtk5EShotObj") == 0) {
			if (p.lifeTimeCounter() == 0) {
				return assignCreatedProjectile("Created 5D Projectile");
			}
			return nullptr;
		}
	}
	return nullptr;
}
const char* canYrcProjectile_baiken5D(PlayerInfo& player) {
	if (moves.baiken5Dcreation == -1) return nullptr; // error
	BYTE* func = player.pawn.bbscrCurrentFunc();
	if (!moves.baiken5Dcreation && func) {
		BYTE* pos = moves.findCreateObj(func, "NmlAtk5EShotObj");
		if (!pos) {
			moves.baiken5Dcreation = -1;
			return nullptr; // error
		}
		pos = moves.findSpriteNonNull(pos);
		if (pos) moves.baiken5Dcreation = pos - func;
	}
	if (!moves.baiken5Dcreation) return nullptr;
	if (player.pawn.bbscrCurrentInstr() > func + moves.baiken5Dcreation
			|| player.pawn.bbscrCurrentInstr() == func + moves.baiken5Dcreation
			&& player.pawn.spriteFrameCounter() > 0) {
		return "Can YRC, and 5D Projectile will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_baikenJD(PlayerInfo& player) {
	if (moves.baikenJDcreation == -1) return nullptr; // error
	BYTE* func = player.pawn.bbscrCurrentFunc();
	if (!moves.baikenJDcreation && func) {
		BYTE* pos = moves.findCreateObj(func, "NmlAtkAir5EShotObj");
		if (!pos) {
			moves.baikenJDcreation = -1;
			return nullptr; // error
		}
		pos = moves.findSpriteNonNull(pos);
		if (pos) moves.baikenJDcreation = pos - func;
	}
	if (!moves.baikenJDcreation) return nullptr;
	if (player.pawn.bbscrCurrentInstr() > func + moves.baikenJDcreation
			|| player.pawn.bbscrCurrentInstr() == func + moves.baikenJDcreation
			&& player.pawn.spriteFrameCounter() > 0) {
		return "Can YRC, and j.D Projectile will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_jackoJD(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and whatever Fireballs have been created so far will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_scroll(PlayerInfo& player) {
	if (player.pawn.currentAnimDuration() > 7
			|| player.pawn.currentAnimDuration() == 7
			&& !player.pawn.isSuperFrozen()
			&& player.pawn.isRCFrozen()) {
		return "Can YRC, and Scroll will still be set";
	}
	return nullptr;
}
const char* canYrcProjectile_clone(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Clone will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_meishiMeteor(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Firesale will still be unleashed";
	}
	return nullptr;
}
const CreatedProjectileStruct* createdProjectile_firesale(PlayerInfo& player) {
	if (player.pawn.currentAnimDuration() < 100) {
		if (player.answerCreatedRSFStart) {
			return assignCreatedProjectile("Created Firesale Starting Projectile");
		}
		return nullptr;
	} else if (!player.createdProjectiles.empty()) {
		return &player.createdProjectiles.front();
	}
	return nullptr;
}
const char* canYrcProjectile_firesale(PlayerInfo& player) {
	if (player.pawn.currentAnimDuration() < 100) {
		for (int i = 2; i < entityList.count; ++i) {
			Entity p = entityList.list[i];
			if (p.isActive() && p.team() == player.index && strcmp(p.animationName(), "RSF_Start") == 0) {
				BYTE* func = p.bbscrCurrentFunc();
				if (!moves.rsfStartStateLinkBreak) {
					int uponLevel = 0;
					bool encounteredLinkBreak = false;
					for (loopInstr(func)) {
						InstrType type = moves.instrType(instr);
						if (type == instr_upon) {
							++uponLevel;
						} else if (type == instr_endUpon) {
							--uponLevel;
						} else if (uponLevel == 0) {
							if (type == instr_setLinkObjectDestroyOnStateChange) {
								encounteredLinkBreak = true;
							} else if (type == instr_sprite && encounteredLinkBreak) {
								moves.rsfStartStateLinkBreak = instr - func;
								break;
							}
						}
					}
				}
				if (p.linkObjectDestroyOnStateChange() == nullptr && !(
						p.bbscrCurrentInstr() - func == moves.rsfStartStateLinkBreak
						&& p.spriteFrameCounter() == 0
					)) {
					return "Can YRC, and the initial (vertical) Card will be thrown, but not the rest of the cards";
				}
				return nullptr;
			}
		}
		return nullptr;
	} else {
		if (canYrcProjectile_default(player)) {
			return "Can YRC, and Firesale will still be unleashed";
		}
		return nullptr;
	}
}
const char* canYrcProjectile_disc(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Tandem Top will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_silentForce(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Silent Force will still be thrown";
	}
	return nullptr;
}
const char* canYrcProjectile_emeraldRain(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Emerald Rain will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_eddie(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Eddie will remain summoned";
	}
	return nullptr;
}
const char* canYrcProjectile_amorphous(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Amorphous will remain summoned";
	}
	return nullptr;
}
const char* canYrcProjectile_slideHead(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Slide Head Shockwave will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_fdb(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and F.D.B reflected projectile will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_trishula(PlayerInfo& player) {
	if (canYrcProjectile_prevNoLinkDestroyOnStateChange(player)) {
		return "Can YRC, and Trishula will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_giganter(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Giganter will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_berryPull(PlayerInfo& player) {
	if (player.wasResource && player.wasResource != player.pawn.exGaugeMaxValue(0)) {
		return "Can YRC, and Berry Pine will remain primed";
	}
	return nullptr;
}
const char* canYrcProjectile_bazooka(PlayerInfo& player) {
	if (player.wasResource && player.wasResource != player.pawn.exGaugeMaxValue(0)) {
		return "Can YRC, and Genoverse rocket will remain created";
	}
	return nullptr;
}
const char* canYrcProjectile_graviertWurde(PlayerInfo& player) {
	if (player.wasResource && player.wasResource != player.pawn.exGaugeMaxValue(0)) {
		return "Can YRC, and Graviert W\xc3\xbcrde will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_stahlWirbel(PlayerInfo& player) {
	if (player.wasResource && player.wasResource != player.pawn.exGaugeMaxValue(0)) {
		return "Can YRC, and Stahl Wirbel will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_card(PlayerInfo& player) {
	const char letter = player.pawn.animationName()[7];
	if (letter < 'B' || letter > 'D') return nullptr;
	int& powerup = moves.jamCardPowerup[letter - 'B'];
	BYTE* func = player.pawn.bbscrCurrentFunc();
	if (!powerup) {
		bool encounteredModifyVar = false;
		for (loopInstr(func)) {
			InstrType type = moves.instrType(instr);
			if (type == instr_modifyVar) {
				encounteredModifyVar = true;
			} else if (type == instr_sprite && encounteredModifyVar) {
				powerup = instr - func;
				break;
			}
		}
	}
	if (!powerup) return nullptr;
	int offset = player.pawn.bbscrCurrentInstr() - func;
	if (offset > powerup || offset == powerup && !player.pawn.justReachedSprite()) {
		return "Can YRC, and Card powerup will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_renhoukyaku(PlayerInfo& player) {
	if (player.wasResource && player.wasResource != player.pawn.exGaugeMaxValue(0)) {
		return "Can YRC, and the puffballs will stay";
	}
	return nullptr;
}
const char* canYrcProjectile_caltrops(PlayerInfo& player) {
	if (player.wasResource && player.wasResource != player.pawn.exGaugeMaxValue(0)) {
		return "Can YRC, and the Card will still be thrown";
	}
	return nullptr;
}
static const char* canYrcProjectile_ballSet(PlayerInfo& player, int* storage, const char* result) {
	BYTE* funcStart = player.pawn.bbscrCurrentFunc();
	moves.fillVenomBallCreation(funcStart, storage);
	int offset = player.pawn.bbscrCurrentInstr() - funcStart;
	if (offset > *storage
			|| offset == *storage
			&& player.pawn.spriteFrameCounter() > 0) {
		return result;
	}
	return nullptr;
}
const char* canYrcProjectile_ballSeiseiA(PlayerInfo& player) {
	return canYrcProjectile_ballSet(player, &moves.venomBallSeiseiABallCreation, "Can YRC, and a new P Ball will still be set");
}
const char* canYrcProjectile_ballSeiseiB(PlayerInfo& player) {
	return canYrcProjectile_ballSet(player, &moves.venomBallSeiseiBBallCreation, "Can YRC, and a new K Ball will still be set");
}
const char* canYrcProjectile_ballSeiseiC(PlayerInfo& player) {
	return canYrcProjectile_ballSet(player, &moves.venomBallSeiseiCBallCreation, "Can YRC, and a new S Ball will still be set");
}
const char* canYrcProjectile_ballSeiseiD(PlayerInfo& player) {
	return canYrcProjectile_ballSet(player, &moves.venomBallSeiseiDBallCreation, "Can YRC, and a new H Ball will still be set");
}
const char* canYrcProjectile_airBallSeiseiA(PlayerInfo& player) {
	return canYrcProjectile_ballSet(player, &moves.venomAirBallSeiseiABallCreation, "Can YRC, and a new P Ball will still be set");
}
const char* canYrcProjectile_airBallSeiseiB(PlayerInfo& player) {
	return canYrcProjectile_ballSet(player, &moves.venomAirBallSeiseiBBallCreation, "Can YRC, and a new K Ball will still be set");
}
const char* canYrcProjectile_airBallSeiseiC(PlayerInfo& player) {
	return canYrcProjectile_ballSet(player, &moves.venomAirBallSeiseiCBallCreation, "Can YRC, and a new S Ball will still be set");
}
const char* canYrcProjectile_airBallSeiseiD(PlayerInfo& player) {
	return canYrcProjectile_ballSet(player, &moves.venomAirBallSeiseiDBallCreation, "Can YRC, and a new H Ball will still be set");
}
const char* canYrcProjectile_stinger(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Stinger Aim will still shoot out a ball";
	}
	return nullptr;
}
const char* canYrcProjectile_carcass(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Carcass Raid will still shoot out a ball";
	}
	return nullptr;
}
static const char* canYrcProjectile_eatMeatImpl(PlayerInfo& player, int* storage) {
	BYTE* funcStart = player.pawn.bbscrCurrentFunc();
	moves.fillSinEatMeatPowerup(funcStart, storage);
	int offset = player.pawn.bbscrCurrentInstr() - funcStart;
	if (offset > *storage
			|| offset == *storage
			&& player.pawn.spriteFrameCounter() > 0) {
		return "Can YRC, and will retain regained energy";
	}
	return nullptr;
}
const char* canYrcProjectile_eatMeat(PlayerInfo& player) {
	return canYrcProjectile_eatMeatImpl(player, &moves.sinEatMeatPowerup);
}
const char* canYrcProjectile_eatMeatOkawari(PlayerInfo& player) {
	return canYrcProjectile_eatMeatImpl(player, &moves.sinEatMeatOkawariPowerup);
}
const char* canYrcProjectile_voltecDein(PlayerInfo& player) {
	if (canYrcProjectile_default(player)) {
		return "Can YRC, and Voltec Dein will stay";
	}
	return nullptr;
}
static inline char getLastLetter(const char* animName) {
	int len = strlen(animName);
	if (!len) return '\0';
	char lastLetter = animName[len - 1];
	if (lastLetter < 'A' || lastLetter > 'D') return '\0';
	return lastLetter;
}
static const CreatedProjectileStruct* createdProjectile_ballSetReusable(PlayerInfo& ent, const char lastLetter) {
	Entity ball = ent.pawn.stackEntity(lastLetter - 'A');
	if (!ball || ball.lifeTimeCounter() != 0) return nullptr;
	switch (lastLetter) {
		case 'A': return assignCreatedProjectile("Created P Ball");
		case 'B': return assignCreatedProjectile("Created K Ball");
		case 'C': return assignCreatedProjectile("Created S Ball");
		case 'D': return assignCreatedProjectile("Created H Ball");
	}
	return assignCreatedProjectile("Created a Ball");
}
const CreatedProjectileStruct* createdProjectile_ballSet(PlayerInfo& ent) {
	const char lastLetter = getLastLetter(ent.pawn.animationName());
	if (!lastLetter) return nullptr;
	return createdProjectile_ballSetReusable(ent, lastLetter);
}
const CreatedProjectileStruct* createdProjectile_qv(PlayerInfo& ent) {
	const char lastLetter = getLastLetter(ent.pawn.animationName());
	if (!lastLetter) return nullptr;
	const CreatedProjectileStruct* otherResult = createdProjectile_ballSetReusable(ent, lastLetter);
	if (otherResult) return otherResult;
	Entity lastCreated = ent.pawn.previousEntity();
	if (lastCreated && lastCreated.lifeTimeCounter() == 0
			&& strcmp(lastCreated.animationName(), "Debious_AttackBall") == 0) {
		return assignCreatedProjectile("Created QV Shockwave");
	}
	return nullptr;
}

const char* powerup_may6P(PlayerInfo& player) {
	Entity pawn = player.pawn;
	BYTE* func = pawn.bbscrCurrentFunc();
	moves.fillMay6PElements(func);
	
	int offset = pawn.bbscrCurrentInstr() - func;
	
	for (const Moves::May6PElement& elem : moves.may6PElements) {
		if (offset == elem.offset) {
			if (pawn.justReachedSprite() && !elem.powerupExplanation.empty()) {
				return elem.powerupExplanation.data();
			}
			break;
		}
	}
	return nullptr;
}
const char* powerup_may6H(PlayerInfo& player) {
	BYTE* func = player.pawn.bbscrCurrentFunc();
	if (!func) return nullptr;
	fillMay6HOffsets(func);
	int offset = player.pawn.bbscrCurrentInstr() - func;
	if (offset > moves.may6H_6DHoldOffset && offset < moves.may6H_6DHoldAttackOffset) {
		if (player.prevFrameMem45 == 1 && player.pawn.mem45() == 0) {
			return "Became an overhead.";
		}
	}
	return nullptr;
}
static const char* powerup_qv(PlayerInfo& player, int stackIndex) {
	if (player.prevFrameMem46 != player.pawn.mem46()) {
		int level = 0;
		Entity p = player.pawn.stackEntity(stackIndex);
		if (p) {
			level = p.storage(1);
			if (level == 1) return "Ball reached level 1.";
			if (level == 2) return "Ball reached level 2.";
			if (level == 3) return "Ball reached level 3.";
			if (level == 4) return "Ball reached level 4.";
			if (level == 5) return "Ball reached level 5.";
		}
		return "Ball reached level ???.";
	}
	return nullptr;
}
const char* powerup_qvA(PlayerInfo& player) {
	return powerup_qv(player, 0);
}
const char* powerup_qvB(PlayerInfo& player) {
	return powerup_qv(player, 1);
}
const char* powerup_qvC(PlayerInfo& player) {
	return powerup_qv(player, 2);
}
const char* powerup_qvD(PlayerInfo& player) {
	return powerup_qv(player, 3);
}
void Moves::fillInVenomStingerPowerup(BYTE* func, std::vector<int>& powerups) {
	if (!powerups.empty()) return;
	bool foundSendSignal = false;
	for (loopInstr(func)) {
		InstrType type = instrType(instr);
		if (type == instr_sendSignal) {
			foundSendSignal = true;
		} else if (foundSendSignal && type == instr_sprite) {
			powerups.push_back(instr - func);
			foundSendSignal = false;
		}
	}
}
const char* powerup_stinger(PlayerInfo& player, std::vector<int>& powerups) {
	BYTE* func = player.pawn.bbscrCurrentFunc();
	if (!func) return nullptr;
	moves.fillInVenomStingerPowerup(func, powerups);
	int offset = player.pawn.bbscrCurrentInstr() - func;
	for (int i = 0; i < (int)powerups.size(); ++i) {
		if (offset == powerups[i]) {
			if (player.pawn.justReachedSprite()) {
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
			return nullptr;
		}
	}
	return nullptr;
}
const char* powerup_stingerS(PlayerInfo& player) {
	return powerup_stinger(player, moves.venomStingerSPowerups);
}
const char* powerup_stingerH(PlayerInfo& player) {
	return powerup_stinger(player, moves.venomStingerHPowerups);
}
const char* powerup_kyougenA(PlayerInfo& ent) {
	if (ent.prevFrameGroundHitEffect != HIT_EFFECT_AIR_FACE_UP && ent.pawn.inflicted()->groundHitEffect == HIT_EFFECT_AIR_FACE_UP) {
		return "Ground bounces on hit.";
	}
	return nullptr;
}
const char* powerup_kyougenB(PlayerInfo& ent) {
	if (ent.prevFrameGroundBounceCount != 1 && ent.pawn.inflicted()->groundBounceCount == 1) {
		return "Without the powerup, this move doesn't ground bounce, but KDs on normal hit."
		" It ground bounces on CH with -350.00 starting speed Y and 227.50 starting speed X."
		" You can combo from this if it was an airhit, and if it was a ground hit, you might need RRC.\n"
		"With the powerup, it ground bounces even on normal hit,"
		" with -250.00 starting speed Y and 175.00 starting speed X, and is very easy to combo from"
		" on both air and ground hits.\n"
		"With the powerup and the counterhit, it launches with -300.00 starting speed Y"
		" and 50.00 starting speed X.";
	}
	return nullptr;
}
const char* powerup_kyougenC(PlayerInfo& ent) {
	if (ent.prevFrameTumbleDuration == INT_MAX && ent.pawn.inflicted()->tumbleDuration != INT_MAX) {
		return "Without the powerup, this move doesn't ground bounce, but KDs on normal ground hit"
		" and wallbounces on normal air hit, with no KD, and you can't combo from that without RRC.\n"
		"On air CH, it ground bounces and then wall bounces, and can be easily combo'd from both midscreen and in the corner.\n"
		"On ground CH, it ground bounces, and can be combo'd from with a fast move both midscreen and in the corner.\n"
		"With the powerup, on normal air or ground hit, it tumbles at 300.00 starting speed X and gives 51-52 tumble frames.\n"
		"With the powerup, on CH air or ground hit, it tumbles at 245.00 starting speed X and gives 69-70 tumble frames.";
	}
	return nullptr;
}
const char* powerup_kyougenD(PlayerInfo& ent) {
	if (ent.prevFrameMaxHit != 5 && ent.pawn.maxHit() == 5) {
		BYTE* func = ent.pawn.bbscrCurrentFunc();
		if (!func) return nullptr;
		BYTE* instr = moves.skipInstr(func);
		instr = moves.skipInstr(instr);
		instr = moves.skipInstr(instr);
		instr = moves.skipInstr(instr);
		instr = moves.skipInstr(instr);
		bool isRev2 = moves.instrType(instr) == instr_hitAirPushbackX;
		if (isRev2) {
			return "Increases maximum number of hits from 3 to 5 and removes landing recovery.";
		} else {
			return "Increases maximum number of hits from 3 to 5 and increases speed X and Y"
				" that is given to the opponent on hit from:\n"
				"70.00 speed X, 175.00 speed Y without the powerup to\n"
				"140.00 speed X, 180.00 speed Y with the powerup.";
		}
	}
	return nullptr;
}
bool projectilePowerup_onpu(ProjectileInfo& projectile) {
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
			&& strcmp(projectile.ptr.gotoLabelRequests(), "hit") != 0
		);
}
bool projectilePowerup_grenadeBomb(ProjectileInfo& projectile) {
	bool result = false;
	if (projectile.ptr) {
		Entity owner = projectile.ptr.playerEntity();
		PlayerInfo& player = endScene.findPlayer(owner);
		if (
			(
				projectile.ptr.lifeTimeCounter() == 0
				|| player.pawn
				&& player.elpheltLastFrameGrenadeAttackLevel != 3
			)
			&& projectile.ptr.dealtAttack()->level == 3
			&& (
				projectile.ptr.hitSomethingOnThisFrame()
				|| projectile.ptr.hitAlreadyHappened() < projectile.ptr.theValueHitAlreadyHappenedIsComparedAgainst()
			)
		) {
			result = true;
		}
		player.elpheltLastFrameGrenadeAttackLevel = projectile.ptr.dealtAttack()->level;
	}
	return result;
}
const char* powerup_djavu(PlayerInfo& ent) {
	if (ent.animFrame == 6 && !ent.pawn.isRCFrozen()) {
		return "//Title override: \n"
			"On this frame \x44\xC3\xA9\x6A\xC3\xA0 Vu checks for the existence of the Seal"
			" and makes the Seal invulnerable.";
	}
	return nullptr;
}
static const char* bedmanSealPowerup(PlayerInfo& player, Moves::BedmanActivateReactivate* storage, const char* deleteString, const char* createString) {
	if (!player.pawn.bbscrCurrentFunc()) return nullptr;
	moves.fillBedmanSealFrames(player.pawn.bbscrCurrentFunc(), storage);
	if (player.pawn.isRCFrozen()) return nullptr;
	int frame = player.pawn.currentAnimDuration();
	if (frame == storage->deactivate || frame == storage->reactivate) {
		int frame = player.pawn.currentAnimDuration();
		if (frame == storage->deactivate) {
			return deleteString;
		} else if (frame == storage->reactivate) {
			return createString;
		}
	}
	return nullptr;
}
#define bedmanSealStrings(sealName) \
	"//Title override: \n" \
	"On this frame the old " sealName " Seal gets deleted.", \
	"On this frame the " sealName " Seal gets created."
	
#define bedmanSealDontShowPowerupGraphic(storage) \
	return ent.pawn.currentAnimDuration() == storage.deactivate;
	
const char* powerup_boomerangA(PlayerInfo& ent) {
	return bedmanSealPowerup(ent, &moves.bedmanBoomerangASeal, bedmanSealStrings("Task A"));
}
bool dontShowPowerupGraphic_boomerangA(PlayerInfo& ent) {
	bedmanSealDontShowPowerupGraphic(moves.bedmanBoomerangASeal)
}
const char* powerup_boomerangAAir(PlayerInfo& ent) {
	return bedmanSealPowerup(ent, &moves.bedmanBoomerangAAirSeal, bedmanSealStrings("Task A"));
}
bool dontShowPowerupGraphic_boomerangAAir(PlayerInfo& ent) {
	bedmanSealDontShowPowerupGraphic(moves.bedmanBoomerangAAirSeal)
}
const char* powerup_boomerangB(PlayerInfo& ent) {
	return bedmanSealPowerup(ent, &moves.bedmanBoomerangBSeal, bedmanSealStrings("Task A'"));
}
bool dontShowPowerupGraphic_boomerangB(PlayerInfo& ent) {
	bedmanSealDontShowPowerupGraphic(moves.bedmanBoomerangBSeal)
}
const char* powerup_boomerangBAir(PlayerInfo& ent) {
	return bedmanSealPowerup(ent, &moves.bedmanBoomerangBAirSeal, bedmanSealStrings("Task A'"));
}
bool dontShowPowerupGraphic_boomerangBAir(PlayerInfo& ent) {
	bedmanSealDontShowPowerupGraphic(moves.bedmanBoomerangBAirSeal)
}
const char* powerup_taskB(PlayerInfo& ent) {
	return bedmanSealPowerup(ent, &moves.bedmanTaskBSeal, bedmanSealStrings("Task B"));
}
bool dontShowPowerupGraphic_taskB(PlayerInfo& ent) {
	bedmanSealDontShowPowerupGraphic(moves.bedmanTaskBSeal)
}
const char* powerup_taskBAir(PlayerInfo& ent) {
	return bedmanSealPowerup(ent, &moves.bedmanAirTaskBSeal, bedmanSealStrings("Task B"));
}
bool dontShowPowerupGraphic_taskBAir(PlayerInfo& ent) {
	bedmanSealDontShowPowerupGraphic(moves.bedmanAirTaskBSeal)
}
const char* powerup_taskC(PlayerInfo& ent) {
	BYTE* funcStart = ent.pawn.bbscrCurrentFunc();
	if (!funcStart) return nullptr;
	moves.fillBedmanGroundTaskCSealOffsets(funcStart);
	if (!moves.bedmanGroundTaskCSealOffset.deactivate || !moves.bedmanGroundTaskCSealOffset.reactivate
		|| !ent.pawn.justReachedSprite()) return nullptr;
	int offset = ent.pawn.bbscrCurrentInstr() - funcStart;
	if (offset == moves.bedmanGroundTaskCSealOffset.deactivate) {
		return "//Title override: \n"
			"On this frame the old Task C Seal gets deleted.";
	} else if (offset == moves.bedmanGroundTaskCSealOffset.reactivate) {
		return "On this frame the Task C Seal gets created.";
	}
	return nullptr;
		
}
bool dontShowPowerupGraphic_taskC(PlayerInfo& ent) {
	int offset = ent.pawn.bbscrCurrentInstr() - ent.pawn.bbscrCurrentFunc();
	return offset == moves.bedmanGroundTaskCSealOffset.deactivate;
}
const char* powerup_taskCAir(PlayerInfo& ent) {
	return bedmanSealPowerup(ent, &moves.bedmanAirTaskCSeal, bedmanSealStrings("Task C"));
}
bool dontShowPowerupGraphic_taskCAir(PlayerInfo& ent) {
	bedmanSealDontShowPowerupGraphic(moves.bedmanAirTaskCSeal)
}
bool projectilePowerup_closeShot(ProjectileInfo& projectile) {
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
const char* powerup_rifle(PlayerInfo& ent) {
	if (!ent.prevFrameElpheltRifle_AimMem46 && ent.elpheltRifle_AimMem46) {
		return "Ms. Confille reached maximum charge.";
	}
	return nullptr;
}
const char* powerup_beakDriver(PlayerInfo& ent) {
	if (!ent.prevFrameMem45 && ent.pawn.mem45()) {
		return "Will perform the maximum power attack upon release.";
	}
	return nullptr;
}
bool dontShowPowerupGraphic_beakDriver(PlayerInfo& ent) {
	return ent.pawn.romanCancelAvailability() == ROMAN_CANCEL_DISALLOWED_ON_WHIFF_WITH_X_MARK;
}
const char* powerup_mistFiner(PlayerInfo& ent) {
	if (ent.johnnyMistFinerBuffedOnThisFrame) {
		if (ent.pawn.dealtAttack()->guardType == GUARD_TYPE_NONE) {
			return "Mist Finer became unblockable and may change to Guard Break instead, if the opponent lands.";
		} else {
			return "Mist Finer acquired Guard Break property and may change to an unblockable, if the opponent jumps.";
		}
	}
	return nullptr;
}
const char* powerup_eatMeat(PlayerInfo& ent) {
	if (ent.pawn.exGaugeValue(0) > ent.prevFrameResource[0]) {
		return "Restored Calorie Gauge.";
	}
	return nullptr;
}
const char* powerup_cardK(PlayerInfo& ent) {
	int lvl = ent.pawn.exGaugeValue(0);
	if (lvl > ent.prevFrameResource[0]) {
		if (lvl == 1) return "Obtained K Card Lvl 1.";
		if (lvl == 2) return "Obtained K Card Lvl 2.";
		if (lvl == 3) return "Obtained K Card Lvl 3.";
	}
	return nullptr;
}
const char* powerup_cardS(PlayerInfo& ent) {
	int lvl = ent.pawn.exGaugeValue(1);
	if (lvl > ent.prevFrameResource[1]) {
		if (lvl == 1) return "Obtained S Card Lvl 1.";
		if (lvl == 2) return "Obtained S Card Lvl 2.";
		if (lvl == 3) return "Obtained S Card Lvl 3.";
	}
	return nullptr;
}
const char* powerup_cardH(PlayerInfo& ent) {
	int lvl = ent.pawn.exGaugeValue(2);
	if (lvl > ent.prevFrameResource[2]) {
		if (lvl == 1) return "Obtained H Card Lvl 1.";
		if (lvl == 2) return "Obtained H Card Lvl 2.";
		if (lvl == 3) return "Obtained H Card Lvl 3.";
	}
	return nullptr;
}
const char* powerup_hayabusaRev(PlayerInfo& ent) {
	if (ent.prevFrameMem45 == 0 && ent.pawn.mem45() != 0 && ent.pawn.gotoLabelRequests()[0] == '\0') {
		return "Acquired Guard Break property, also now does more damage.";
	}
	return nullptr;
}
const char* powerup_hayabusaHeld(PlayerInfo& ent) {
	if (ent.prevFrameMem45 == 0 && ent.pawn.mem45() != 0 && ent.pawn.gotoLabelRequests()[0] == '\0') {
		return "Reached maximum charge.";
	}
	return nullptr;
}
const char* powerup_grampaMax(PlayerInfo& ent) {
	if (ent.prevFrameMem45 == 0 && ent.pawn.mem45() != 0
			&& ent.pawn.gotoLabelRequests()[0] == '\0') {
		return "Reached maximum charge.";
	}
	return nullptr;
}
const char* powerup_antiAir4Hasei(PlayerInfo& ent) {
	if (ent.prevFrameMem45 == 0 && ent.pawn.mem45() != 0) {
		return "Reached maximum charge.";
	}
	return nullptr;
}
const char* powerup_blackHoleAttack(PlayerInfo& ent) {
	if (strcmp(ent.pawn.gotoLabelRequests(), "Attack") == 0 && ent.pawn.dealtAttack()->guardType == GUARD_TYPE_NONE
			&& !ent.pawn.isRCFrozen()) {
		return "Reached maximum charge.";
	}
	return nullptr;
}
const char* powerup_armorDance(PlayerInfo& ent) {
	if (ent.pawn.exGaugeValue(0) > ent.prevFrameResource[0]) {
		return "Gained Excitement.";
	}
	return nullptr;
}
const char* powerup_fireSpear(PlayerInfo& ent) {
	if (ent.pawn.previousEntity()
			&& ent.pawn.previousEntity().lifeTimeCounter() == 0
			&& strcmp(ent.pawn.previousEntity().animationName(), "KinomiObjNecro") != 0) {  // ignore the first spear
		return "Created the next fire spear.";
	}
	return nullptr;
}
const char* powerup_zweit(PlayerInfo& ent) {
	bool powerup = false;
	if (strcmp(ent.pawn.gotoLabelRequests(), "FrontEnd") == 0) {
		powerup = true;
	} else {
		if (!ent.pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED)) return nullptr;
		BYTE* instr = ent.pawn.uponStruct(BBSCREVENT_ANIMATION_FRAME_ADVANCED)->uponInstrPtr;
		instr = moves.skipInstr(instr);
		powerup = moves.instrType(instr) == instr_ifOperation
				&& asInstr(instr, ifOperation)->op == BBSCROP_IS_GREATER_OR_EQUAL;
	}
	if (powerup) {
		return "//Title override: \n"
			"On this frame checks if Leo's origin point is still not behind opponent's origin point."
			" If on any of these frames Leo failed to cross his opponent up, he will transition to non-backturn ender.";
	}
	return nullptr;
}
const char* powerup_secretGarden(PlayerInfo& ent) {
	entityList.populate();
	for (int i = 2; i < entityList.count; ++i) {
		Entity proj = entityList.list[i];
		if (proj.team() == ent.index && strcmp(proj.animationName(), "SecretGardenBall") == 0) {
			BYTE* funcStart = proj.bbscrCurrentFunc();
			if (!funcStart) continue;
			moves.fillMilliaSecretGardenUnlink(funcStart);
			if (moves.milliaSecretGardenUnlink && proj.bbscrCurrentInstr() - funcStart == moves.milliaSecretGardenUnlink
					&& proj.justReachedSprite()) {
				return "//Title override: \n"
					"On this frame Secret Garden detached from the player, and, starting on the next frame, can be RC'd, and the Secret Garden will stay.";
			}
		}
	}
	return nullptr;
}
void findSpriteAfterIf(BYTE* func, int* result) {
	if (!*result) {
		bool encounteredIf = false;
		for (loopInstr(func)) {
			InstrType type = moves.instrType(instr);
			if (type == instr_ifOperation) {
				encounteredIf = true;
			} else if (type == instr_sprite && encounteredIf) {
				*result = instr - func;
				break;
			}
		}
	}
}
const char* powerup_pickUpGhost(PlayerInfo& ent) {
	PickUpGhostAnalysisResult result;
	analyzePickUpGhost(ent, &result);
	if (result.onThisFramePickedUpGhost) {
		return "//Title override: \n"
			"Ghost is now considered to be fully picked up.";
	}
	return nullptr;
}
const char* powerup_putGhost(PlayerInfo& ent) {
	BYTE* func = ent.pawn.bbscrCurrentFunc();
	findSpriteAfterIf(func, &moves.jackoPutGhost);
	int offset = ent.pawn.bbscrCurrentInstr() - func;
	if (offset == moves.jackoPutGhost && ent.pawn.justReachedSprite()) {
		return "//Title override: \n"
			"No longer considered carrying the Ghost.";
	}
	return nullptr;
}
const char* powerup_returnGhost(PlayerInfo& ent) {
	BYTE* func = ent.pawn.bbscrCurrentFunc();
	findSpriteAfterIf(func, &moves.jackoReturnGhost);
	int offset = ent.pawn.bbscrCurrentInstr() - func;
	if (offset == moves.jackoReturnGhost && ent.pawn.justReachedSprite()) {
		return "//Title override: \n"
			"Retrieved Ghost to inventory.";
	}
	return nullptr;
}
const char* powerup_dizzy6H(PlayerInfo& ent) {
	if (strcmp(ent.pawn.gotoLabelRequests(), "Attack") == 0
			&& !ent.pawn.isRCFrozen()
			&& strcmp(ent.pawn.dealtAttack()->trialName, "NmlAtk6DHold") == 0) {
		return "Reached maximum charge.";
	}
	return nullptr;
}

void fillMay6HOffsets(BYTE* func) {
	if (moves.may6H_6DHoldOffset == 0) {
		moves.may6H_6DHoldOffset = moves.findSetMarker(func, "6DHold") - func;
		moves.may6H_6DHoldAttackOffset = moves.findSetMarker(func, "6DHoldAttack") - func;
	}
}

int Moves::getBedmanSealRemainingFrames(ProjectileInfo& projectile, MayIrukasanRidingObjectInfo& info, BBScrEvent signal, bool* isFrameAfter) {
	BYTE* func = projectile.ptr.bbscrCurrentFunc();
	if (!func) return 0;
	if (info.totalFrames == 0) {
		BYTE* instr;
		bool metSprite = false;
		bool metSpriteEnd = false;
		bool metSendSignal = false;
		int lastSpriteLength = 0;
		bool isInsideUpon = false;
		for (loopInstrNoRedefine(func)) {
			InstrType type = moves.instrType(instr);
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
			*isFrameAfter = i == 3 && projectile.ptr.justReachedSprite();
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
	for (loopInstr(func)) {
		InstrType type = instrType(instr);
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
	ramlethalBit.emplace_back();
	RamlethalSwordInfo* currentElem = &ramlethalBit.back();
	currentElem->state = ram_teleport;
	int lastSpriteLengthSoubi = 0;
	int lastSpriteLengthBunri = 0;
	bool metSprite = false;
	bool metSpriteEnd = false;
	bool metSetMarker = false;
	bool metJumpToState = false;
	for (loopInstr(func)) {
		InstrType type = instrType(instr);
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

const NamePair* Moves::rifleAutoExit(PlayerInfo& player, int* offsetStorage, const NamePair* moveName) {
	BYTE* func = player.pawn.bbscrCurrentFunc();
	if (!func) return nullptr;
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
	for (loopInstrNoRedefine(func)) {
		InstrType type = instrType(instr);
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
	bool found = false;
	for (loopInstr(func)) {
		InstrType type = instrType(instr);
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
	int counter = 2;
	for (loopInstr(func)) {
		InstrType type = instrType(instr);
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
	int counter = 3;
	for (loopInstr(func)) {
		InstrType type = instrType(instr);
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
	int counter = 6;
	for (loopInstr(func)) {
		InstrType type = instrType(instr);
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
	for (loopInstr(func)) {
		InstrType type = instrType(instr);
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
	int counter = 4;
	for (loopInstr(func)) {
		InstrType type = instrType(instr);
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
	int counter = 2;
	for (loopInstr(func)) {
		InstrType type = instrType(instr);
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
	int counter = 4;
	for (loopInstr(func)) {
		InstrType type = instrType(instr);
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
	int counter = 2;
	for (loopInstr(func)) {
		InstrType type = instrType(instr);
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
	for (loopInstr(func)) {
		InstrType type = instrType(instr);
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
	for (loopInstrNoRedefine(func)) {
		InstrType type = instrType(instr);
		if (type == instr_setMarker && strcmp(asInstr(instr, setMarker)->name, "AtkLv1") == 0) {
			start = true;
			break;
		}
	}
	if (!start) return;
	for (loopInstrNoRedefine(instr)) {
		InstrType type = instrType(instr);
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
	for (loopInstrNoRedefine(func)) {
		InstrType type = instrType(instr);
		if (type == instr_upon && asInstr(instr, upon)->event == BBSCREVENT_HIT_A_PLAYER) {
			--counter;
			if (counter == 0) {
				inUponHitTheEnemyPlayer = true;
				break;
			}
		}
	}
	if (!inUponHitTheEnemyPlayer) return;
	for (loopInstrNoRedefine(instr)) {
		InstrType type = instrType(instr);
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
			instr = skipInstr(func);
			true;
			instr = skipInstr(instr)
	) {
		InstrType type = instrType(instr);
		if (type == instr_sprite || type == instr_endState) {
			if (metSprite) {
				dizzyKinomiNecrobomb.frames.emplace_back();
				MayIrukasanRidingObjectFrames& newFrames = dizzyKinomiNecrobomb.frames.back();
				newFrames.frames = dizzyKinomiNecrobomb.totalFrames;
				dizzyKinomiNecrobomb.totalFrames += lastSpriteLength;
				newFrames.offset = instr - func;
			}
			if (type == instr_sprite) {
				metSprite = true;
				lastSpriteLength = asInstr(instr, sprite)->duration;
			} else {  // type == instr_endState
				break;
			}
		}
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
			instr = skipInstr(func);
			true;
			instr = skipInstr(instr)
	) {
		InstrType type = instrType(instr);
		if (metSpriteEnd) {
			elem->frames.emplace_back();
			MayIrukasanRidingObjectFrames& newFrames = elem->frames.back();
			newFrames.frames = elem->totalFrames;
			newFrames.offset = instr - func;
			elem->totalFrames += lastSpriteLength;
			metSprite = false;
			metSpriteEnd = false;
		}
		if (type == instr_sprite || type == instr_endState) {
			if (metSprite) {
				elem->frames.emplace_back();
				MayIrukasanRidingObjectFrames& newFrames = elem->frames.back();
				newFrames.frames = elem->totalFrames;
				newFrames.offset = instr - func;
				elem->totalFrames += lastSpriteLength;
			}
			if (type == instr_sprite) {
				if (metSetMarker) {
					dizzyAkari.emplace_back();
					elem = &dizzyAkari.back();
					metSetMarker = false;
				}
				metSprite = true;
				lastSpriteLength = asInstr(instr, sprite)->duration;
			} else {  // type == instr_endState
				break;
			}
		} else if (type == instr_spriteEnd) {
			metSpriteEnd = true;
		} else if (type == instr_setMarker) {
			metSetMarker = true;
		}
	}
}

void Moves::fillDizzyFish(BYTE* func, MayIrukasanRidingObjectInfo& fish) {
	if (fish.totalFrames != 0) return;
	BYTE* instr = findSetMarker(func, "end");
	if (!instr) return;
	bool metSprite = false;
	int lastSpriteLength = 0;
	for (
			instr = skipInstr(instr);
			true;
			instr = skipInstr(instr)
	) {
		InstrType type = instrType(instr);
		if (type == instr_sprite || type == instr_endState) {
			if (metSprite) {
				fish.frames.emplace_back();
				MayIrukasanRidingObjectFrames& newFrames = fish.frames.back();
				newFrames.frames = fish.totalFrames;
				newFrames.offset = instr - func;
				fish.totalFrames += lastSpriteLength;
			}
			if (type == instr_sprite) {
				lastSpriteLength = asInstr(instr, sprite)->duration;
				metSprite = true;
			} else {  // type == instr_endState
				break;
			}
		}
	}
}

void Moves::fillDizzyLaserFish(BYTE* func, int* normal, int* alt) {
	if (*normal != 0) return;
	int lastSpriteLength = 0;
	for (loopInstr(func)) {
		InstrType type = instrType(instr);
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
			instr = skipInstr(instr);
			true;
			instr = skipInstr(instr)
	) {
		InstrType type = instrType(instr);
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
			instr = skipInstr(instr);
			true;
			instr = skipInstr(instr)
	) {
		InstrType type = instrType(instr);
		if (type == instr_sprite || type == instr_endState) {
			if (metSprite) {
				info.frames.emplace_back();
				MayIrukasanRidingObjectFrames& newFrames = info.frames.back();
				newFrames.frames = info.totalFrames;
				newFrames.offset = instr - func;
				info.totalFrames += lastSpriteLength;
			}
			if (type == instr_sprite) {
				lastSpriteLength = asInstr(instr, sprite)->duration;
				metSprite = true;
			} else {  // type == instr_endState
				break;
			}
		}
	}
}

void Moves::fillMilliaSecretGardenUnlink(BYTE* funcStart) {
	if (milliaSecretGardenUnlink || milliaSecretGardenUnlinkFailedToFind) return;
	bool metUnlink = false;
	for (loopInstr(funcStart)) {
		InstrType type = instrType(instr);
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
	fillElpheltRifleFireStartup(func);
}
void Moves::fillElpheltRifleFireStartup(BYTE* func) {
	if (elpheltRifleFireStartup) return;
	int prevDuration = 0;
	int startup = 0;
	for (loopInstr(func)) {
		InstrType type = instrType(instr);
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
	for (loopInstr(funcStart)) {
		InstrType type = instrType(instr);
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
		if (ent.pawn.hitboxes()->count[HITBOXTYPE_HITBOX] > 0 && ent.pawn.currentAnimDuration() > 62) {
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

void Moves::fillBedmanSealFrames(BYTE* funcStart, BedmanActivateReactivate* storage) {
	if (storage->deactivate || storage->reactivate) return;  // if both are zero despite being found, then they're somewhere near the start of the state and wasting time to re-find them is OK
	int totalFrames = 0;
	int spriteFrames = 0;
	for (loopInstr(funcStart)) {
		InstrType type = instrType(instr);
		if (type == instr_sprite) {
			totalFrames += spriteFrames;
			spriteFrames = asInstr(instr, sprite)->duration;
		} else if (type == instr_deactivateObjectByName && strncmp(asInstr(instr, deactivateObjectByName)->name, "DejavIcon", 9) == 0) {
			storage->deactivate = totalFrames + 1;
		} else if (type == instr_createObject && strncmp(asInstr(instr, createObject)->name, "DejavIcon", 9) == 0) {
			storage->reactivate = totalFrames + 1;
			return;
		}
	}
}

void Moves::fillBedmanGroundTaskCSealOffsets(BYTE* funcStart) {
	if (bedmanGroundTaskCSealOffset.deactivate) return;
	bool encounteredDeactivation = false;
	bool encounteredReactivation = false;
	for (loopInstr(funcStart)) {
		InstrType type = instrType(instr);
		if (type == instr_deactivateObjectByName && strcmp(asInstr(instr, deactivateObjectByName)->name, "DejavIconFlyingBed") == 0) {
			encounteredDeactivation = true;
		} else if (type == instr_createObject && strcmp(asInstr(instr, createObject)->name, "DejavIconFlyingBed") == 0) {
			encounteredReactivation = true;
		} else if (type == instr_sprite) {
			if (encounteredDeactivation) {
				encounteredDeactivation = false;
				bedmanGroundTaskCSealOffset.deactivate = instr - funcStart;
			}
			if (encounteredReactivation) {
				encounteredReactivation = false;
				bedmanGroundTaskCSealOffset.reactivate = instr - funcStart;
				return;
			}
		}
	}
}

void Moves::fillBedmanDejavuStartup(BYTE* funcStart, int* startup) {
	if (*startup) return;
	int totalFrames = 0;
	int spriteFrames = 0;
	for (loopInstr(funcStart)) {
		InstrType type = instrType(instr);
		if (type == instr_sprite) {
			totalFrames += spriteFrames;
			spriteFrames = asInstr(instr, sprite)->duration;
		} else if (type == instr_createObject && strncmp(asInstr(instr, createObject)->name, "Djavu_", 6) == 0) {
			*startup = totalFrames + 1;
			return;
		}
	}
}

void Moves::fillVenomBallCreation(BYTE* funcStart, int* result) {
	if (*result) return;
	bool foundTheThing = false;
	for (loopInstr(funcStart)) {
		InstrType type = instrType(instr);
		if (type == instr_createObject) {
			if (strcmp(asInstr(instr, createObject)->name, "Ball") == 0
					&& asInstr(instr, createObject)->pos == 0) {
				foundTheThing = true;
			}
		} else if (foundTheThing && type == instr_sprite) {
			if (foundTheThing) {
				*result = instr - funcStart;
				return;
			}
		}
	}
}

void Moves::fillRamlethalCreateBitLaserMinion(BYTE* funcStart) {
	if (ramlethalCreateBitLaserMinion) return;
	for (loopInstr(funcStart)) {
		InstrType type = instrType(instr);
		if (type == instr_createObject && strcmp(asInstr(instr, createObject)->name, "BitLaser_Minion") == 0) {
			ramlethalCreateBitLaserMinion = instr - funcStart;
			return;
		}
	}
}

void Moves::fillRamlethalBitLaserMinionStuff(BYTE* funcStart) {
	if (ramlethalBitLaserMinionNonBossCreateLaser) return;
	bool encounteredMarker = false;
	for (loopInstr(funcStart)) {
		InstrType type = instrType(instr);
		if (type == instr_setMarker) {
			if (strcmp(asInstr(instr, setMarker)->name, "BossStart") == 0) {
				encounteredMarker = true;
				ramlethalBitLaserMinionBossStartMarker = instr - funcStart;
			}
		} else if (type == instr_createObjectWithArg) {
			if (strcmp(asInstr(instr, createObjectWithArg)->name, "BitLaser") == 0) {
				if (!ramlethalBitLaserMinionBossStartMarker) {
					ramlethalBitLaserMinionNonBossCreateLaser = instr - funcStart;
				} else {
					ramlethalBitLaserMinionBossCreateLaser = instr - funcStart;
					return;
				}
			}
		}
	}
			
}

void Moves::fillSinEatMeatPowerup(BYTE* funcStart, int* storage) {
	if (*storage) return;
	bool encounteredPowerup = false;
	for (loopInstr(funcStart)) {
		InstrType type = instrType(instr);
		if (type == instr_modifyVar) {
			if (asInstr(instr, modifyVar)->left == BBSCRVAR_RESOURCE_AMOUNT) {
				encounteredPowerup = true;
			}
		} else if (type == instr_sprite && encounteredPowerup) {
			*storage = instr - funcStart;
			return;
		}
	}
}

void charge_may6P(PlayerInfo& ent, ChargeData* result) {
	Entity pawn = ent.pawn;
	BYTE* func = pawn.bbscrCurrentFunc();
	moves.fillMay6PElements(func);
	if (moves.may6PElements.empty()) {
		result->current = 0;
		result->max = 0;
		return;
	}
	
	int offset = pawn.bbscrCurrentInstr() - func;
	int frame = pawn.spriteFrameCounter() + 1;
	const Moves::May6PElement& lastElem = moves.may6PElements.back();
	bool hasRequest = strcmp(pawn.gotoLabelRequests(), "6AHoldAttack") == 0;
	if (offset == lastElem.offset) {
		int currentCharge = lastElem.charge + frame;
		result->current = currentCharge - (
			hasRequest
			|| currentCharge == lastElem.maxCharge + pawn.spriteFrameCounterMax()
		);
		result->max = lastElem.maxCharge;
		return;
	}
	
	for (const Moves::May6PElement* ptr = moves.may6PElements.data(); ptr != &lastElem; ++ptr) {
		if (offset == ptr->offset) {
			int currentCharge = ptr->charge + frame - hasRequest;
			result->current = currentCharge;
			if (currentCharge > ptr->charge) {
				result->max = ptr->maxCharge;
			} else {
				result->max = ptr->charge;
			}
			return;
		}
	}
	
	result->current = 0;
	result->max = 0;
}

void charge_may6H(PlayerInfo& ent, ChargeData* result) {
	Entity pawn = ent.pawn;
	BYTE* func = pawn.bbscrCurrentFunc();
	fillMay6HOffsets(func);
	BYTE* currentInstr = pawn.bbscrCurrentInstr();
	int offset =  currentInstr - func;
	if (offset > moves.may6H_6DHoldOffset && offset < moves.may6H_6DHoldAttackOffset) {
		int accumulatedChargeResult = 0;
		int charge = 0;
		BYTE* instr;
		
		{
			int prevDuration = 0;
			int accumulatedChargeNext = 0;
			int accumulatedCharge = 0;
			int nextCharge = 0;
			for (loopInstrNoRedefine(func + moves.may6H_6DHoldOffset)) {
				InstrType type = moves.instrType(instr);
				if (type == instr_sprite) {
					accumulatedCharge = accumulatedChargeNext;
					accumulatedChargeNext += prevDuration;
					prevDuration = asInstr(instr, sprite)->duration;
					charge = nextCharge;
					nextCharge += prevDuration;
				} else if (type == instr_storeValue) {
					if (asInstr(instr, storeValue)->dest == MEM(45)
							&& asInstr(instr, storeValue)->src == AccessedValue(BBSCRTAG_VALUE, 0)) {
						break;
					}
				}
				if (instr == currentInstr) {
					accumulatedChargeResult = accumulatedCharge;
				}
			}
		}
		
		int currentCharge;
		if (currentInstr > instr) {
			currentCharge = charge;
		} else {
			currentCharge = accumulatedChargeResult + pawn.spriteFrameCounter() + 1;
		}
		result->current = currentCharge - (
			strcmp(pawn.gotoLabelRequests(), "6DHoldAttack") == 0
		);
		result->max = charge;
		return;
	}
	
	result->current = 0;
	result->max = 0;
}
static void charge_blitzShield(PlayerInfo& ent, ChargeData* result, BlitzShieldPrereqData* data) {
	Entity pawn = ent.pawn;
	
	int animFrame = pawn.currentAnimDuration();
	int frameSteps = pawn.animFrameStepCounter();
	int skippedFrames = frameSteps - animFrame;
	int mem45 = pawn.mem45();  // H button released
	int mem51 = pawn.mem51();  // how long H button was held for, in frame steps
	int mx = 11 + min(2, skippedFrames);
	
	BYTE* func = pawn.bbscrCurrentFunc();
	moves.fillBlitzShieldChargePrereq(func, data);
	int offset = pawn.bbscrCurrentInstr() - func;
	
	if (offset >= data->end) {
		result->current = 0;
		result->max = 0;
		return;
	}
	
	if (animFrame < 15) {
		result->current = min(mx, mem51);
		result->max = mx;
		return;
	}
	
	BYTE* current = pawn.bbscrCurrentInstr();
	
	int howMuchSinceStartOfAttack;
	int animFrameOnWhichGotoAttackHappened;
	int activeFramesAddon = 0;
	if (offset >= data->attackStart) {
		int framesUntilHit = 0;
		int framesPlayedSinceHit = 0;
		BYTE* hitInstr = func + data->hitStart;
		int timeUntilCharge;
		if (offset >= data->hitStart) {
			for (BYTE* instr = func + data->hitStart; instr != current; instr = moves.skipInstr(instr)) {
				InstrType type = moves.instrType(instr);
				if (type == instr_sprite) {
					framesPlayedSinceHit += asInstr(instr, sprite)->duration;
				}
			}
			
			framesPlayedSinceHit = framesPlayedSinceHit + pawn.spriteFrameCounter() + 1;
			if (framesPlayedSinceHit > 3
					|| pawn.hitAlreadyHappened() == pawn.theValueHitAlreadyHappenedIsComparedAgainst()
					&& !pawn.hitSomethingOnThisFrame()) {
				result->current = 0;
				result->max = 0;
				return;
			}
			if (framesPlayedSinceHit > 1) activeFramesAddon = framesPlayedSinceHit - 1;
			timeUntilCharge = 63 - animFrame;
			howMuchSinceStartOfAttack = data->attackStartup + framesPlayedSinceHit;
		} else {
			howMuchSinceStartOfAttack = 0;
			int lastDur = 0;
			for (loopInstr(func + data->attackStart)) {
				InstrType type = moves.instrType(instr);
				if (instr == current) break;
				if (type == instr_sprite) {
					lastDur = asInstr(instr, sprite)->duration;
					howMuchSinceStartOfAttack += lastDur;
				}
			}
			howMuchSinceStartOfAttack = howMuchSinceStartOfAttack
				- lastDur
				+ pawn.spriteFrameCounter()
				+ 1;
			
			lastDur = 0;
			for (loopInstr(current)) {
				InstrType type = moves.instrType(instr);
				if (type == instr_hit) break;
				if (type == instr_sprite) {
					lastDur = asInstr(instr, sprite)->duration;
					framesUntilHit += lastDur;
				}
			}
			framesUntilHit = framesUntilHit - lastDur
				+ (
					pawn.spriteFrameCounterMax()
					- pawn.spriteFrameCounter()
					- 1
				);
			timeUntilCharge = 63 - (
				animFrame
				+ framesUntilHit
				+ 1
			);
		}
		
		if (timeUntilCharge > activeFramesAddon) {
			result->current = 0;
			result->max = 0;
			return;
		}
		
		animFrameOnWhichGotoAttackHappened = animFrame - howMuchSinceStartOfAttack;
	} else if (mem45) {
		if (animFrame > mem51 + skippedFrames + 1) {
			animFrameOnWhichGotoAttackHappened = animFrame;
		} else {
			animFrameOnWhichGotoAttackHappened = animFrame + 1;
		}
	} else {
		animFrameOnWhichGotoAttackHappened = animFrame + 2;
	}
	
	int chargeInAnim = animFrameOnWhichGotoAttackHappened - 16 + activeFramesAddon;
	int currentCharge = mx + chargeInAnim;
	if (currentCharge <= mx) {
		result->current = mx;
		result->max = mx;
	} else {
		mx = mx + 51 - 16;
		result->current = min(mx, currentCharge);
		result->max = mx;
	}
	
}
void charge_standingBlitzShield(PlayerInfo& ent, ChargeData* result) {
	charge_blitzShield(ent, result, &ent.standingBlitzShieldPrereqData);
}
void charge_crouchingBlitzShield(PlayerInfo& ent, ChargeData* result) {
	charge_blitzShield(ent, result, &ent.crouchingBlitzShieldPrereqData);
}
void charge_fdb(PlayerInfo& ent, ChargeData* result) {
	Entity pawn = ent.pawn;
	BYTE* currentInstr = pawn.bbscrCurrentInstr();
	
	if (pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED)) {
		if (moves.instrType(currentInstr) == instr_groundHitEffect) {
			result->current = 1 + pawn.spriteFrameCounter() + 1;
			result->max = 1 + pawn.spriteFrameCounterMax();
		} else {
			// pawn.spriteFrameCounterMax() must equal 1
			result->current = 1;
			result->max = 1 + asInstr(currentInstr, sprite)->duration;
		}
		return;
	}
	
	result->current = 0;
	result->max = 0;
}
void charge_soutenBC(PlayerInfo& ent, ChargeData* result) {
	Entity pawn = ent.pawn;
	if (pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED)) {
		BYTE* firstInstr = moves.skipInstr(pawn.uponStruct(BBSCREVENT_ANIMATION_FRAME_ADVANCED)->uponInstrPtr);
		if (moves.instrType(firstInstr) == instr_checkInput) {
			BYTE* func = pawn.bbscrCurrentFunc();
			BYTE* currentInstr = pawn.bbscrCurrentInstr();
			int prevDur = 0;
			int charge = 0;
			int chargeMax = 0;
			for (loopInstr(func)) {
				InstrType type = moves.instrType(instr);
				if (type == instr_sprite) {
					if (instr < currentInstr) {
						charge += prevDur;
					}
					chargeMax += prevDur;
					prevDur = asInstr(instr, sprite)->duration;
				} else if (type == instr_clearUpon) {
					break;
				}
			}
			result->current = charge + pawn.spriteFrameCounter() + 1 - (
				strcmp(pawn.gotoLabelRequests(), "open") == 0
			);
			result->max = chargeMax;
			return;
		}
	}
	
	result->current = 0;
	result->max = 0;
}
void charge_dubiousCurve(PlayerInfo& ent, ChargeData* result) {
	Entity pawn = ent.pawn;
	if (pawn.mem45()) {
		BYTE* func = pawn.bbscrCurrentFunc();
		static const char dubiousCurve[] = "DubiousCurve";
		const char letter = pawn.animationName()[sizeof dubiousCurve - 1];
		if (letter >= 'A' && letter <= 'D') {
			Moves::VenomQvChargeElement& elem = moves.venomQvCharges[letter - 'A'];
			moves.fillVenomQvCharges(func, elem);
			const Moves::VenomQvChargeSubelement& subelem = elem.getElem(pawn.bbscrCurrentInstr() - func);
			int currentCharge = subelem.charge + pawn.spriteFrameCounter() + 1
				- (
					strcmp(pawn.gotoLabelRequests(), "End") == 0
					|| &subelem - elem.elements.data() == elem.elements.size() - 1
					&& pawn.spriteFrameCounter() == pawn.spriteFrameCounterMax() - 1
				);
			result->current = currentCharge;
			if (currentCharge <= subelem.charge && subelem.isKeyElement) {
				result->max = subelem.charge;
			} else {
				result->max = subelem.maxCharge;
			}
			return;
		}
	}
	
	result->current = 0;
	result->max = 0;
}
void charge_stingerAim(PlayerInfo& ent, ChargeData* result) {
	Entity pawn = ent.pawn;
	if (pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED)) {
		BYTE* func = pawn.bbscrCurrentFunc();
		int frameStartOfCharge = 0;
		int prevDur = 0;
		for (loopInstr(func)) {
			InstrType type = moves.instrType(instr);
			if (type == instr_sprite) {
				frameStartOfCharge += prevDur;
				prevDur = asInstr(instr, sprite)->duration;
			} else if (type == instr_upon) {
				if (asInstr(instr, upon)->event == BBSCREVENT_ANIMATION_FRAME_ADVANCED) {
					break;
				}
			}
		}
		
		static const char stingerAim[] = "StingerAim";
		const char letter = pawn.animationName()[sizeof stingerAim - 1];
		if (letter == 'C' || letter == 'D') {
			std::vector<int>& elems = moves.venomStingerChargeLevels[letter - 'C'];
			if (elems.empty()) {
				prevDur = 0;
				int currentLength = 0;
				for (loopInstr(func)) {
					InstrType type = moves.instrType(instr);
					if (type == instr_sprite) {
						currentLength += prevDur;
						prevDur = asInstr(instr, sprite)->duration;
					} else if (type == instr_sendSignal) {
						elems.push_back(currentLength);
					}
				}
			}
			
			int animFrame = pawn.currentAnimDuration() - (
				strcmp(pawn.gotoLabelRequests(), "Shot") == 0
			);
			int lastLength = 0;
			for (int length : elems) {
				lastLength = length;
				if (length >= animFrame) {
					break;
				}
			}
			
			result->current = animFrame - frameStartOfCharge;
			result->max = lastLength - frameStartOfCharge;
			return;
		}
	}
	
	result->current = 0;
	result->max = 0;
}
void charge_beakDriver(PlayerInfo& ent, ChargeData* result) {
	Entity pawn = ent.pawn;
	if (pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED)) {
		BYTE* func = pawn.bbscrCurrentFunc();
		if (!moves.sinBeakDriverMinCharge) {
			int totalFrames = 0;
			int prevDur = 0;
			for (loopInstr(func)) {
				InstrType type = moves.instrType(instr);
				if (type == instr_sprite) {
					totalFrames += prevDur;
					prevDur = asInstr(instr, sprite)->duration;
				} else if (type == instr_upon) {
					if (asInstr(instr, upon)->event == BBSCREVENT_ANIMATION_FRAME_ADVANCED) {
						moves.sinBeakDriverMinCharge = totalFrames;
					}
				} else if (type == instr_storeValue) {
					if (asInstr(instr, storeValue)->dest == MEM(45)
							&& asInstr(instr, storeValue)->src == AccessedValue(BBSCRTAG_VALUE, 1)) {
						moves.sinBeakDriverMaxCharge = totalFrames;
						break;
					}
				}
			}
		}
		
		int animFrame = pawn.currentAnimDuration();
		result->current = animFrame - moves.sinBeakDriverMinCharge;
		result->max = moves.sinBeakDriverMaxCharge - moves.sinBeakDriverMinCharge;
		return;
	}
	
	result->current = 0;
	result->max = 0;
}
void charge_elpheltStand(PlayerInfo& ent, ChargeData* result) {
	Entity pawn = ent.pawn;
	BOOL isInShotgun = (BOOL)pawn.playerVal(0);
	ChargeData& sc = ent.elpheltShotgunCharge;
	const char* previousAnim = pawn.previousAnimName();
	if (!isInShotgun) {
		sc.current = 0;
		sc.max = 0;
		sc.elpheltShotgunChargeSkippedFrames = 0;
	} else if (strcmp(previousAnim, "CounterGuardStand") == 0
			|| strcmp(previousAnim, "CounterGuardCrouch") == 0) {
		sc.current = ent.timePassedPureIdle + 1;
		// max is set in EndScene.cpp hardcode
		sc.elpheltShotgunChargeSkippedFrames = ent.elpheltSkippedTimePassed;
	} else {
		BOOL hasFullCharge = (BOOL)pawn.playerVal(1);
		int animFrame = pawn.currentAnimDuration();
		if (animFrame == 1) {
			if (hasFullCharge) {
				if (strcmp(previousAnim, "CmnActCrouch2Stand") != 0) {
					sc.max = 0;
				}
			} else if (sc.max == 0) {
				sc.max = 13;
			}
		}
		if (sc.max) {
			sc.current = ent.timePassedPureIdle + 1;
			sc.elpheltShotgunChargeSkippedFrames = ent.elpheltSkippedTimePassed;
		} else {
			sc.current = 0;
			sc.elpheltShotgunChargeSkippedFrames = 0;
		}
	}
	
	if (sc.max) {
		ent.elpheltShotgunChargeConsumed = false;
	}
	result->current = 0;
	result->max = 0;
}
void charge_elpheltCrouch2Stand(PlayerInfo& ent, ChargeData* result) {
	
	ChargeData& sc = ent.elpheltShotgunCharge;
	Entity pawn = ent.pawn;
	if (!moves.elpheltCrouch2StandChargeDuration) {
		BYTE* func = pawn.bbscrCurrentFunc();
		int total = 0;
		int prevDur = 0;
		for (loopInstr(func)) {
			InstrType type = moves.instrType(instr);
			if (type == instr_sprite) {
				total += prevDur;
				prevDur = asInstr(instr, sprite)->duration;
			} else if (type == instr_storeValue) {
				if (asInstr(instr, storeValue)->dest == BBSCRVAR_PLAYERVAL_1
						&& asInstr(instr, storeValue)->src == AccessedValue(BBSCRTAG_VALUE, 1)) {
					moves.elpheltCrouch2StandChargeDuration = total + 1;
					break;
				}
			}
		}
	}
	
	if (pawn.playerVal(0)) {
		sc.current = pawn.currentAnimDuration();
		sc.max = moves.elpheltCrouch2StandChargeDuration;
		sc.elpheltShotgunChargeSkippedFrames = ent.elpheltSkippedTimePassed;
		ent.elpheltShotgunChargeConsumed = false;
	} else {
		sc.current = 0;
		sc.max = 0;
		sc.elpheltShotgunChargeSkippedFrames = 0;
	}
	result->current = 0;
	result->max = 0;
}
void charge_elpheltLanding(PlayerInfo& ent, ChargeData* result) {
	
	Entity pawn = ent.pawn;
	ChargeData& sc = ent.elpheltShotgunCharge;
	if (pawn.playerVal(0) && !pawn.playerVal(1)) {
		// you can only end up here from a Gold Burst, and it does not preserve existing charge, so maybe pawn.playerVal(1) check is not needed
		sc.current = pawn.currentAnimDuration();
		sc.max = 6  // duration of landing animation
			+ 13;  // for how long to charge in CmnActStand
		sc.elpheltShotgunChargeSkippedFrames = ent.elpheltSkippedTimePassed;
	} else {
		sc.current = 0;
		sc.max = 0;
		sc.elpheltShotgunChargeSkippedFrames = 0;
	}
	result->current = 0;
	result->max = 0;
	
}
void charge_elpheltDashStop(PlayerInfo& ent, ChargeData* result) {
	
	Entity pawn = ent.pawn;
	ChargeData& sc = ent.elpheltShotgunCharge;
	if (pawn.playerVal(0)) {
		// can't have charge in this animation at all, since you can only end up here from CmnActFDash, which deprives you of it, so no need for pawn.playerVal(1) check
		sc.current = pawn.currentAnimDuration();
		sc.max = 13  // duration of CmnActFDashStop
			+ 13;  // for how long to charge in CmnActStand
		sc.elpheltShotgunChargeSkippedFrames = ent.elpheltSkippedTimePassed;
	} else {
		sc.current = 0;
		sc.max = 0;
		sc.elpheltShotgunChargeSkippedFrames = 0;
	}
	result->current = 0;
	result->max = 0;
	
}
void charge_shotgunCharge(PlayerInfo& ent, ChargeData* result) {
	ent.elpheltShotgunChargeConsumed = true;
	*result = ent.elpheltShotgunCharge;
}
void charge_rifleCharge(PlayerInfo& ent, ChargeData* result) {
	Entity pawn = ent.pawn;
	int animFrame = pawn.currentAnimDuration();
	if (animFrame < moves.elpheltRifleFireStartup) {
		for (int i = 2; i < entityList.count; ++i) {
			Entity aim = entityList.list[i];
			if (aim.isActive() && aim.team() == ent.index && strcmp(aim.animationName(), "Rifle_Aim") == 0) {
				result->current = aim.mem45();
				result->max = moves.elpheltRifleFirePowerupStartup;
				return;
			}
		}
	}
	
	result->current = 0;
	result->max = 0;
}
static void charge_treasureHuntImpl(PlayerInfo& ent, ChargeData* result, int* minCharge, int* maxCharge) {
	Entity pawn = ent.pawn;
	if (!*minCharge) {
		int total = 0;
		int prevDur = 0;
		for (loopInstr(pawn.bbscrCurrentFunc())) {
			InstrType type = moves.instrType(instr);
			if (type == instr_sprite) {
				total += prevDur;
				prevDur = asInstr(instr, sprite)->duration;
			} else if (type == instr_upon) {
				if (asInstr(instr, upon)->event == BBSCREVENT_ANIMATION_FRAME_ADVANCED) {
					if (!*minCharge) {
						*minCharge = total;
					}
				}
			} else if (type == instr_ifOperation) {
				if (asInstr(instr, ifOperation)->op == BBSCROP_IS_GREATER
						&& asInstr(instr, ifOperation)->left == BBSCRVAR_FRAMES_PLAYED_IN_STATE
						&& asInstr(instr, ifOperation)->right.tag == BBSCRTAG_VALUE) {
					*maxCharge = asInstr(instr, ifOperation)->right.value;
					break;
				}
			}
		}
	}
	
	BYTE* currentInstr = pawn.bbscrCurrentInstr();
	const char* gotoLabel = pawn.gotoLabelRequests();
	int currentCharge = pawn.currentAnimDuration() - *minCharge;
	int maximumCharge = *maxCharge - *minCharge;
	if (currentCharge > maximumCharge) currentCharge = maximumCharge;
	if (gotoLabel[0] != '\0'
			&& strcmp(gotoLabel, "Run") == 0
			&& pawn.dealtAttack()->guardType == GUARD_TYPE_NONE
			|| pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED)
			&& moves.instrType(
				moves.skipInstr(
					pawn.uponStruct(BBSCREVENT_ANIMATION_FRAME_ADVANCED)->uponInstrPtr
				)
			) == instr_checkInput
	) {
		result->current = currentCharge;
		result->max = maximumCharge;
		return;
	}
	
	result->current = 0;
	result->max = 0;
}
void charge_treasureHunt(PlayerInfo& ent, ChargeData* result) {
	charge_treasureHuntImpl(ent, result, &moves.johnnyTreasureHuntMinCharge, &moves.johnnyTreasureHuntMaxCharge);
}
void charge_stepTreasureHunt(PlayerInfo& ent, ChargeData* result) {
	charge_treasureHuntImpl(ent, result, &moves.johnnyStepTreasureHuntMinCharge, &moves.johnnyStepTreasureHuntMaxCharge);
}
static void charge_6HaseiImpl(PlayerInfo& ent, ChargeData* result, int* storageMin, int* storageMax, const char* heavyAttackName, int frameMin, int frameMax, int diff) {
	Entity pawn = ent.pawn;
	if (pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED)) {
		if (!*storageMin) {
			InstrType prevType = (InstrType)0;
			int lastValue = 0;
			for (loopInstr(pawn.bbscrCurrentFunc())) {
				InstrType type = moves.instrType(instr);
				if (type == instr_ifOperation) {
					if (asInstr(instr, ifOperation)->op == BBSCROP_IS_EQUAL
							&& (
								asInstr(instr, ifOperation)->left == BBSCRVAR_FRAMES_PLAYED_IN_STATE
								|| asInstr(instr, ifOperation)->left == BBSCRVAR_FRAMES_SINCE_REGISTERING_FOR_THE_ANIMATION_FRAME_ADVANCED_SIGNAL
							)
							&& asInstr(instr, ifOperation)->right.tag == BBSCRTAG_VALUE) {
						lastValue = asInstr(instr, ifOperation)->right.value;
					}
				} else if (type == instr_storeValue) {
					if (asInstr(instr, storeValue)->dest == MEM(45)
							&& asInstr(instr, storeValue)->src == AccessedValue(BBSCRTAG_VALUE, 1)) {
						*storageMin = lastValue;
					}
				} else if (type == instr_gotoLabelRequests) {
					if (strcmp(asInstr(instr, gotoLabelRequests)->name, heavyAttackName) == 0
							&& prevType == instr_ifOperation) {
						*storageMax = lastValue - 1;
						break;
					}
				}
				prevType = type;
			}
		}
		int currentCharge = frameMin - diff;
		if (frameMax - 1 > *storageMax) {
			currentCharge = *storageMax + diff;
		}
		// increment both by 1 because if you have this event handler at all, that means you charged for at least one frame
		result->current = currentCharge + diff;
		result->max = *storageMin + diff;
		return;
	}
	result->current = 0;
	result->max = 0;
}
void charge_antiAir6Hasei(PlayerInfo& ent, ChargeData* result) {
	charge_6HaseiImpl(ent,
		result,
		&moves.haehyunAntiAir6HaseiMaxChargeLower,
		&moves.haehyunAntiAir6HaseiMaxChargeUpper,
		"HeavyAttack",
		ent.pawn.framesSinceRegisteringForTheIdlingSignal(),
		ent.pawn.framesSinceRegisteringForTheIdlingSignal(),
		1);
}
void charge_landBlow6Hasei(PlayerInfo& ent, ChargeData* result) {
	charge_6HaseiImpl(ent,
		result,
		&moves.haehyunLandBlow6HaseiMaxChargeLower,
		&moves.haehyunLandBlow6HaseiMaxChargeUpper,
		"Attack2",
		ent.pawn.currentAnimDuration(),
		ent.pawn.framesSinceRegisteringForTheIdlingSignal(),
		0);
}
void charge_landBlow4Hasei(PlayerInfo& ent, ChargeData* result) {
	charge_6HaseiImpl(ent,
		result,
		&moves.haehyunLandBlow4HaseiMaxChargeLower,
		&moves.haehyunLandBlow4HaseiMaxChargeUpper,
		"Attack2",
		ent.pawn.currentAnimDuration(),
		ent.pawn.framesSinceRegisteringForTheIdlingSignal(),
		0);
}
void charge_antiAir4Hasei(PlayerInfo& ent, ChargeData* result) {
	Entity pawn = ent.pawn;
	if (pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED)) {
		int maxChargeParsed = 0;
		int lastValue = 0;
		for (loopInstr(pawn.uponStruct(BBSCREVENT_ANIMATION_FRAME_ADVANCED)->uponInstrPtr)) {
			InstrType type = moves.instrType(instr);
			if (type == instr_ifOperation) {
				if (asInstr(instr, ifOperation)->op == BBSCROP_IS_EQUAL
						&& asInstr(instr, ifOperation)->left == BBSCRVAR_FRAMES_SINCE_REGISTERING_FOR_THE_ANIMATION_FRAME_ADVANCED_SIGNAL
						&& asInstr(instr, ifOperation)->right.tag == BBSCRTAG_VALUE) {
					lastValue = asInstr(instr, ifOperation)->right.value;
				}
			} else if (type == instr_storeValue
					&& asInstr(instr, storeValue)->dest == MEM(45)
					&& asInstr(instr, storeValue)->src == AccessedValue(BBSCRTAG_VALUE, 1)) {
				maxChargeParsed = lastValue - 1;
				break;
			}
		}
		
		int animFrame = pawn.framesSinceRegisteringForTheIdlingSignal() - 1;
		result->current = animFrame;
		result->max = maxChargeParsed;
		return;
	}
	
	result->current = 0;
	result->max = 0;
}
void charge_blackHoleAttack(PlayerInfo& ent, ChargeData* result) {
	Entity pawn = ent.pawn;
	if (pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED) && pawn.mem45()) {
		if (!moves.haehyunBlackHoleAttackMaxCharge) {
			int theValue = 0;
			for (loopInstr(pawn.uponStruct(BBSCREVENT_ANIMATION_FRAME_ADVANCED)->uponInstrPtr)) {
				InstrType type = moves.instrType(instr);
				if (type == instr_ifOperation) {
					if (asInstr(instr, ifOperation)->op == BBSCROP_IS_EQUAL
							&& asInstr(instr, ifOperation)->left == BBSCRVAR_FRAMES_SINCE_REGISTERING_FOR_THE_ANIMATION_FRAME_ADVANCED_SIGNAL
							&& asInstr(instr, ifOperation)->right.tag == BBSCRTAG_VALUE) {
						theValue = asInstr(instr, ifOperation)->right.value;
					}
				} else if (type == instr_endUpon) {
					break;
				}
			}
			moves.haehyunBlackHoleAttackMaxCharge = theValue;
		}
		
		result->current = pawn.framesSinceRegisteringForTheIdlingSignal() - 1;
		result->max = moves.haehyunBlackHoleAttackMaxCharge - 1;
		return;
	}
	result->current = 0;
	result->max = 0;
}
void charge_dizzy6H(PlayerInfo& ent, ChargeData* result) {
	if (!moves.dizzy6HMinCharge) {
		int total = 0;
		int prevDur = 0;
		bool encounteredUpon = false;
		for (loopInstr(ent.pawn.bbscrCurrentFunc())) {
			InstrType type = moves.instrType(instr);
			if (type == instr_sprite) {
				if (!encounteredUpon) {
					total += prevDur;
					prevDur = asInstr(instr, sprite)->duration;
				}
			} else if (type == instr_ifOperation) {
				if (asInstr(instr, ifOperation)->op == BBSCROP_IS_GREATER_OR_EQUAL
						&& asInstr(instr, ifOperation)->left == BBSCRVAR_FRAMES_PLAYED_IN_STATE
						&& asInstr(instr, ifOperation)->right.tag == BBSCRTAG_VALUE) {
					moves.dizzy6HMaxCharge = asInstr(instr, ifOperation)->right.value;
					break;
				}
			} else if (type == instr_upon) {
				if (asInstr(instr, upon)->event == BBSCREVENT_ANIMATION_FRAME_ADVANCED) {
					encounteredUpon = true;
				}
			}
		}
		moves.dizzy6HMinCharge = total;
	}
	int frame = ent.pawn.currentAnimDuration();
	if (frame > moves.dizzy6HMinCharge && (
			ent.pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED)
			|| strcmp(ent.pawn.gotoLabelRequests(), "loop") == 0
		)) {
		result->current = frame - moves.dizzy6HMinCharge;
		result->max = moves.dizzy6HMaxCharge - moves.dizzy6HMinCharge - 1;
		return;
	}
	result->current = 0;
	result->max = 0;
}
void charge_kinomiNecro(PlayerInfo& ent, ChargeData* result) {
	Entity pawn = ent.pawn;
	moves.fillInKinomiNecroChargePrereq(pawn.bbscrCurrentFunc());
	if (pawn.hasUpon(BBSCREVENT_ANIMATION_FRAME_ADVANCED)) {
		int maxCharge = moves.dizzyKinomiNecroSpear2 - 1;
		int currentCharge = pawn.currentAnimDuration();
		if (currentCharge > maxCharge) {
			maxCharge = moves.dizzyKinomiNecroSpear3 - 1;
		}
		result->current = currentCharge - moves.dizzyKinomiNecroMinCharge;
		result->max = maxCharge - moves.dizzyKinomiNecroMinCharge;
		return;
	}
	result->current = 0;
	result->max = 0;
}

void Moves::fillMay6PElements(BYTE* func) {
	if (!may6PElements.empty()) return;
	May6PElement preparedElement;
	preparedElement.attackData.blockstun = 0;
	preparedElement.attackData.pushback = 0;
	preparedElement.attackData.wallstick = INT_MAX;
	bool startedHolding = false;
	bool startedAttack = false;
	bool preparedElementReady = false;
	int charge = 0;
	int nextCharge = 0;
	int keyElementCount = 0;
	int lastKeyElementIndex = -1;
	for (loopInstr(func)) {
		InstrType type = instrType(instr);
		if (type == instr_setMarker) {
			if (strcmp(asInstr(instr, setMarker)->name, "6AHold") == 0) {
				if (may6PElements.empty()) return;
				startedHolding = true;
			} else if (strcmp(asInstr(instr, setMarker)->name, "6AHoldAttack") == 0) {
				startedAttack = true;
			}
		} else if (type == instr_stunValue) {
			preparedElement.attackData.stun = asInstr(instr, stunValue)->amount;
			preparedElementReady = true;
		} else if (type == instr_blockstunAmount) {
			preparedElement.attackData.blockstun = asInstr(instr, blockstunAmount)->amount;
			preparedElementReady = true;
		} else if (type == instr_hitPushbackX) {
			preparedElement.attackData.pushback = asInstr(instr, hitPushbackX)->amount;
			preparedElementReady = true;
		} else if (type == instr_wallstickDuration) {
			preparedElement.attackData.wallstick = asInstr(instr, wallstickDuration)->amount;
			preparedElementReady = true;
		} else if (type == instr_attackLevel) {
			if (!preparedElement.attackData.blockstun) {
				preparedElement.attackData.blockstun = blockstuns[asInstr(instr, attackLevel)->amount];
			}
			if (!preparedElement.attackData.pushback) {
				preparedElement.attackData.pushback = 100;
			}
			preparedElementReady = true;
		} else if (type == instr_sprite) {
			if (preparedElementReady) {
				may6PElements.push_back(preparedElement);
				May6PElement& newElement = may6PElements.back();
				newElement.offset = instr - func;
				newElement.charge = charge;
				newElement.keyElement = true;
				++keyElementCount;
				
				sprintf_s(newElement.nameData, "6P (Lv%d)", keyElementCount);
				if (lastKeyElementIndex != -1) {
					const May6PElement& prevElement = may6PElements[lastKeyElementIndex];
					char* buf = strbuf;
					size_t bufSize = sizeof strbuf;
					bool isFirst = true;
					int result;
					int totalSize = 0;
					#define appendString(fmt, ...) \
						if (!isFirst && bufSize > 1) { \
							buf[0] = ' '; \
							buf[1] = '\0'; \
							++buf; \
							--bufSize; \
							++totalSize; \
						} \
						isFirst = false; \
						result = sprintf_s(buf, bufSize, fmt, __VA_ARGS__); \
						if (result != -1) totalSize += result; \
						advanceBuf
					if (prevElement.attackData.stun != newElement.attackData.stun) {
						appendString("Base Stun increased from %d to %d.", prevElement.attackData.stun, newElement.attackData.stun)
					}
					if (prevElement.attackData.blockstun != newElement.attackData.blockstun) {
						appendString("Blockstun increased from %d to %d.", prevElement.attackData.blockstun, newElement.attackData.blockstun)
					}
					if (prevElement.attackData.pushback != newElement.attackData.pushback) {
						appendString("Pushback modifier increased from %d%c to %d%c.", prevElement.attackData.pushback, '%',
							newElement.attackData.pushback, '%')
					}
					if (!prevElement.attackData.wallstick && newElement.attackData.wallstick) {
						appendString("Gives wallstick in the corner.")
					}
					newElement.powerupExplanation.resize(totalSize + 1);
					memcpy(newElement.powerupExplanation.data(), strbuf, totalSize + 1);
					
					int newCharge = charge;
					May6PElement* ptr = may6PElements.data() + (may6PElements.size() - 2);
					int counter = (int)may6PElements.size() - 2;
					do {
						ptr->maxCharge = newCharge;
						
						if (ptr->keyElement) break;
						--ptr;
						--counter;
					} while (counter >= 0);
				}
				
				lastKeyElementIndex = (int)may6PElements.size() - 1;
				
			} else if (startedHolding) {
				may6PElements.emplace_back();
				May6PElement& newElement = may6PElements.back();
				newElement.offset = instr - func;
				newElement.charge = charge;
				newElement.keyElement = false;
				newElement.attackData = preparedElement.attackData;
				strcpy(newElement.nameData, "6P (Lv1)");
			}
			if (startedHolding) {
				charge = nextCharge;
				nextCharge += asInstr(instr, sprite)->duration;
			}
			preparedElementReady = false;
			if (startedAttack) {
				if (may6PElements.size() >= 2) {
					may6PElements.erase(may6PElements.begin(), may6PElements.begin() + 2);
				}
				if (!may6PElements.empty()) {
					if (may6PElements.size() > 2) {
						May6PElement& lastElem = may6PElements.back();
						strcpy(lastElem.nameData, "6P Max");
						lastElem.maxCharge = may6PElements[may6PElements.size() - 2].maxCharge;
					}
					const char* lastNonEmptyName = nullptr;
					for (May6PElement& elem : may6PElements) {
						bool isEmpty = elem.nameData[0] == '\0';
						if (!isEmpty) {
							lastNonEmptyName = elem.nameData;
						}
						elem.name.name = lastNonEmptyName;
						elem.name.slang = nullptr;
					}
				}
				return;
			}
		}
	}
}

void Moves::fillBlitzShieldChargePrereq(BYTE* func, BlitzShieldPrereqData* data) {
	if (data->attackStartup) return;
	bool inAttack = false;
	bool foundHit = false;
	bool hitConsumed = false;
	int startup = 0;
	int prevDuration = 0;
	for (loopInstr(func)) {
		InstrType type = instrType(instr);
		if (type == instr_setMarker) {
			if (strcmp(asInstr(instr, setMarker)->name, "attack") == 0) {
				inAttack = true;
				data->attackStart = instr - func;
			} else if (strcmp(asInstr(instr, setMarker)->name, "end") == 0) {
				data->end = instr - func;
				data->attackStartup = startup;
				return;
			}
		} else if (type == instr_hit) {
			if (inAttack) foundHit = true;
		} else if (type == instr_sprite) {
			if (foundHit) {
				if (!hitConsumed) {
					hitConsumed = true;
					data->hitStart = instr - func;
				}
			} else if (inAttack) {
				startup += prevDuration;
				prevDuration = asInstr(instr, sprite)->duration;
			}
		}
	}
}

void Moves::fillVenomQvCharges(BYTE* func, VenomQvChargeElement& data) {
	if (data.firstOffset) return;
	bool foundStoreMem45 = false;
	BYTE* firstInstr = nullptr;
	BYTE* lastInstr = nullptr;
	int prevDur = 0;
	for (loopInstr(func)) {
		InstrType type = instrType(instr);
		if (type == instr_storeValue) {
			if (asInstr(instr, storeValue)->dest == MEM(45)) {
				if (asInstr(instr, storeValue)->src == AccessedValue(BBSCRTAG_VALUE, 1)) {
					foundStoreMem45 = true;
				} else {
					break;
				}
			}
		} else if (type == instr_sprite) {
			if (foundStoreMem45) {
				firstInstr = instr;
				foundStoreMem45 = false;
			}
			if (!firstInstr) {
				prevDur = asInstr(instr, sprite)->duration;
			}
			lastInstr = instr;
		}
	}
	
	if (!firstInstr || !lastInstr) return;
	data.firstOffset = firstInstr - func;
	
	data.elements.resize((lastInstr - firstInstr) / sizeof BBScrInstr_sprite + 1);
	VenomQvChargeSubelement* firstPtr = data.elements.data();
	VenomQvChargeSubelement* lastPtr = firstPtr - 1;
	
	bool isKeyElement = false;
	int charge = 0;
	for (BYTE* instr = firstInstr; instrType(instr) != instr_endState; instr = skipInstr(instr)) {
		InstrType type = instrType(instr);
		if (type == instr_sprite) {
			VenomQvChargeSubelement& newElem = data.getElem(instr - func);
			lastPtr = &newElem;
			newElem.charge = charge;
			newElem.maxCharge = 0;
			newElem.isKeyElement = isKeyElement;
			isKeyElement = false;
			charge += prevDur;
			prevDur = asInstr(instr, sprite)->duration;
		} else if (type == instr_storeValue
				&& asInstr(instr, storeValue)->dest == MEM(46)
				|| type == instr_clearUpon) {
			
			if (type == instr_clearUpon) {
				charge -= prevDur;
			} else {
				isKeyElement = true;
			}
			for (VenomQvChargeSubelement* ptr = lastPtr; ptr >= firstPtr && !ptr->maxCharge; --ptr) {
				ptr->maxCharge = charge;
			}
			if (type == instr_clearUpon) {
				break;
			}
		}
	}
	
}

void Moves::fillLeoSemuke(BYTE* func) {
	if (leoSemukeFrontWalkStart) return;
	bool inFront = false;
	bool inBack = false;
	bool metSprite = false;
	bool metSpriteEnd = false;
	bool metThingWasFront = false;
	for (loopInstr(func)) {
		InstrType type = instrType(instr);
		
		if (metSpriteEnd) {
			metSpriteEnd = false;
			if (metThingWasFront) leoSemukeFrontWalkEnd = instr - func;
			else leoSemukeBackWalkEnd = instr - func;
		}
		
		if (type == instr_setMarker) {
			if (strcmp(asInstr(instr, setMarker)->name, "SemukeFrontWalk") == 0) {
				inBack = false;
				inFront = true;
			} else if (strcmp(asInstr(instr, setMarker)->name, "SemukeBackWalk") == 0) {
				inBack = true;
				inFront = false;
			} else if (strcmp(asInstr(instr, setMarker)->name, "SemukeEnd") == 0) {
				leoSemukeEnd = instr - func;
				return;
			}
		} else if (type == instr_sprite) {
			if (inFront || inBack) {
				if (inFront) {
					if (!leoSemukeFrontWalkStart) {
						leoSemukeFrontWalkStart = instr - func;
					}
				} else {
					if (!leoSemukeBackWalkStart) {
						leoSemukeBackWalkStart = instr - func;
					}
				}
				if (metSprite) {
					if (metThingWasFront) leoSemukeFrontWalkEnd = instr - func;
					else leoSemukeBackWalkEnd = instr - func;
				} else {
					metSprite = true;
				}
				metThingWasFront = inFront;
			}
		} else if (type == instr_spriteEnd) {
			if (inFront || inBack) {
				metSpriteEnd = true;
				metSprite = false;
				metThingWasFront = inFront;
			}
		}
	}
}

Moves::SemukeSubanim Moves::parseSemukeSubanim(BYTE* func, BYTE* instr, const char* gotoLabelRequests, SemukeParseMode mode) {
	int offset = instr - func;
	moves.fillLeoSemuke(func);
	if (mode == SEMUKEPARSE_INPUT) {
		if (offset >= moves.leoSemukeBackWalkStart
				&& offset <= moves.leoSemukeBackWalkEnd
				&& gotoLabelRequests[0] == '\0'
				|| strcmp(gotoLabelRequests, "SemukeBackWalk") == 0) {
			return SEMUKESUBANIM_WALK_BACK;
		} else if (offset >= moves.leoSemukeFrontWalkStart
				&& offset <= moves.leoSemukeFrontWalkEnd
				&& gotoLabelRequests[0] == '\0'
				|| strcmp(gotoLabelRequests, "SemukeFrontWalk") == 0) {
			return SEMUKESUBANIM_WALK_FORWARD;
		} else if (offset >= moves.leoSemukeEnd
				|| strcmp(gotoLabelRequests, "SemukeEnd") == 0) {
			return SEMUKESUBANIM_EXIT;
		}
	} else {  // mode == SEMUKEPARSE_ANIM
		if (offset >= moves.leoSemukeBackWalkStart
				&& offset <= moves.leoSemukeBackWalkEnd) {
			return SEMUKESUBANIM_WALK_BACK;
		} else if (offset >= moves.leoSemukeFrontWalkStart
				&& offset <= moves.leoSemukeFrontWalkEnd) {
			return SEMUKESUBANIM_WALK_FORWARD;
		} else if (offset >= moves.leoSemukeEnd) {
			return SEMUKESUBANIM_EXIT;
		}
	}
	return SEMUKESUBANIM_NONE;
}

Moves::SemukeSubanim Moves::parseSemukeSubanimWithCheck(Entity pawn, SemukeParseMode mode) {
	if (strcmp(pawn.animationName(), "Semuke") == 0) {
		return parseSemukeSubanim(pawn.bbscrCurrentFunc(), pawn.bbscrCurrentInstr(), pawn.gotoLabelRequests(), mode);
	}
	return SEMUKESUBANIM_NONE;
}

Moves::SemukeSubanim Moves::parseSemukeSubanim(Entity pawn, SemukeParseMode mode) {
	return parseSemukeSubanim(pawn.bbscrCurrentFunc(), pawn.bbscrCurrentInstr(), pawn.gotoLabelRequests(), mode);
}

void Moves::fillInKinomiNecroChargePrereq(BYTE* func) {
	if (!dizzyKinomiNecroMinCharge) {
		int total = 0;
		int prevDur = 0;
		bool encounteredUpon = false;
		for (loopInstr(func)) {
			InstrType type = instrType(instr);
			if (type == instr_sprite) {
				if (!encounteredUpon) {
					total += prevDur;
					prevDur = asInstr(instr, sprite)->duration;
				}
			} else if (type == instr_upon) {
				if (asInstr(instr, upon)->event == BBSCREVENT_ANIMATION_FRAME_ADVANCED) {
					encounteredUpon = true;
				}
			} else if (type == instr_ifOperation) {
				if (asInstr(instr, ifOperation)->op == BBSCROP_IS_EQUAL
						&& asInstr(instr, ifOperation)->left == BBSCRVAR_FRAMES_PLAYED_IN_STATE
						&& asInstr(instr, ifOperation)->right.tag == BBSCRTAG_VALUE) {
					const int rightValue = asInstr(instr, ifOperation)->right.value;
					if (!dizzyKinomiNecroSpear2) {
						dizzyKinomiNecroSpear2 = rightValue;
					} else {
						dizzyKinomiNecroSpear3 = rightValue;
					}
				}
			} else if (type == instr_setMarker) {
				if (strcmp(asInstr(instr, setMarker)->name, "end") == 0) {
					dizzyKinomiNecroEndOffset = instr - func;
					break;
				}
			}
		}
		dizzyKinomiNecroMinCharge = total;
	}
}
