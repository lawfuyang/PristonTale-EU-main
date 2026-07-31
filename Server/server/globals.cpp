#include "stdafx.h"
#include "globals.h"

int								iEventExpGlobal;
int								iServerCodeGlobal;
int								iRateExp;
BOOL							bFreeTradeChat;
BOOL							bHalloweenEvent;
int								iRateHalloweenMonster;
BOOL							bChristmasEvent;
BOOL							bMimicEvent;
int								iSantaId;
int								iSantaMightyId;
int								iSantaSuperId;
int								iHuntEventMaxCount;
int								iHuntEventSpawnDelay;
int								iHuntEventSpawnDistance = DISTANCE_XY_41_meters;
int								iRateWolfMonster;
BOOL							bEasterEvent;
int								iExtraDropsEvent = 0;
BOOL							bStarWarsEvent;
BOOL							bBeeEvent;
BOOL							bEventGirlFree;
int								iBlessCastleMode;

BOOL							bAgingFreeEvent;
BOOL							bAgingNoBreakEvent;
BOOL							bAgingHalfPriceEvent;
int								SKILL_MP_COST_PERCENT	= 100;
int								SKILL_SP_COST_PERCENT	= 100;
BOOL							ALWAYS_AGING_SUCCESS	= FALSE;

int								LOOT_MODE				= 0;

//Unit (monster/NPC) status replication tuning. See server.ini [Performance].
//UNIT_STATUS_UPDATE_DIVISOR: how often LoopUnits() runs, as a divisor of the
//  64 FPS user wheel. 8 = 8Hz, 4 = 16Hz, 2 = 32Hz, 1 = 64Hz (every frame).
//UNIT_DIRTY_WINDOW_INTERVAL: how often UnitServer::Update() opens its
//  status-dirty window (in frames). Lower = more frequent baseline updates.
int								UNIT_STATUS_UPDATE_DIVISOR	= 8;
int								UNIT_DIRTY_WINDOW_INTERVAL	= 64;

//User (player) status replication tuning. See server.ini [Performance].
//USER_STATUS_UPDATE_DIVISOR: divisor of the 64 FPS user wheel that drives the
//  b8/b16/b32 tiers. 8 = original (b8=8Hz, b16=4Hz, b32=2Hz), 1 = every frame.
//  Note b64 always stays at a true 1Hz because UserTick1s() drives regen/DoT.
int								USER_STATUS_UPDATE_DIVISOR	= 8;

//LoopUsers() contains idle-resend timers counted in invocations, not in time
//(thresholds 2/4/8/16 assume LoopUsers() runs at 2Hz). When LoopUsers() is
//sped up, those thresholds must be multiplied by the same factor or idle
//players would be retransmitted N times more often. 8/divisor gives that
//factor: divisor 8 -> 1 (original), divisor 1 -> 8.
int								USER_IDLE_THRESHOLD_SCALE	= 1;

//Sleep interval of the clock thread that drives the server frame, in ms.
//The frame accumulator wants 15.625ms (64 FPS); a 15ms sleep with default
//timer resolution actually yields ~15.6ms+ and beats against it. Lower values
//(with timeBeginPeriod) let the accumulator hit its target reliably.
int								SERVER_UPDATE_INTERVAL_MS	= 15;

//FURY ARENA
BOOL							bFuryArenaEnabled;

//See https://docs.google.com/spreadsheets/d/1GNWePBhgcB1YX1n9pny5WlQk2y5lnsa2jJ8JCBAWMMA/edit#gid=1488786183

int								iFuryArenaKingFuryExpInitial	   = 200'000'000;
int								iFuryArenaKingFuryGoldInitial      = 0;
int								iFuryArenaLordBabelExpInitial	   = 60'000'000;
int								iFuryArenaLordBabelGoldInitial     = 0;

int								iFuryArenaKingFuryExpMinimum		= 100'000'000;
int								iFuryArenaLordBabelExpMinimum		= 20'000'000;

int								iFuryArenaKingFuryExpLossPerTick   = 10'000'000;
int								iFuryArenaKingFuryGoldLossPerTick  = 0;
int								iFuryArenaLordBabelExpLossPerTick  = 4'000'000;
int								iFuryArenaLordBabelGoldLossPerTick = 0;

int								iFuryArenaKingFuryGoldPerMinion    = 50'000;
int								iFuryArenaLordBabelGoldPerMinion   = 20'000;

int								iFuryArenaKingFuryMeteorDamageMin  = 250;
int								iFuryArenaKingFuryMeteorDamageMax  = 400;
int								iFuryArenaLordBabelMeteorDamageMin = 150;
int								iFuryArenaLordBabelMeteorDamageMax = 250;

BOOL							bWantedMoriphHuntEvent;
BOOL							bWantedWolfHuntEvent;

int								iBellatraRoomScoreGoldScale;
int								iBellatraPerformanceGoldScale;
int								iBellatraMaintenanceTax = 0;
int								iBellatraSecondClanTax = 0;
int								iBellatraThirdClanTax = 0;


BOOL							bValantinesDayEvent = FALSE;


SYSTEMTIME						sLocalTimeServer;
