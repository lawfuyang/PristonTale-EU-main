#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#define WM_UPDATE			WM_USER +   1
#define WM_WINDOWREADY		WM_USER +  51
#define WM_SOCKETACCEPT		WM_USER + 101
#define WM_SOCKETPACKET		WM_USER + 102
#define WM_SOCKETCLOSE		WM_USER + 103

#include "CApplication.h"

//Global Functions
#include "def.h"
#include "common.h"
#include "strings.h"
#include "point.h"
#include <iostream>
#include <ctime>

//File Library
#include "CReader.h"
#include "CReaderFile.h"
#include "CMutex.h"

#include "packets.h"
#include "socket.h"


#include "CSystem.h"

//Definitions
#include "sound.h"
#include "map.h"
#include "quest.h"
#include "party.h"
#include "chat.h"
#include "warehouse.h"
#include "Coin.h"
#include "CoinShop.h"
#include "debug.h"
#include "user.h"
#include "account.h"

#include "skill.h"

#include "point.h"
#include "party.h"
#include "SQLConnection.h"
#include "server.h"


#define IDI_ICON			107

#define WINDOW_WIDTH		300
#define WINDOW_HEIGHT		120




extern SYSTEMTIME								sLocalTimeServer;


static void UpdateServerTime()
{
	//GetLocalTime( &sLocalTimeServer );
}

static DWORD GetUnixTime()
{
	std::time_t result = std::time(nullptr);
	return static_cast<DWORD>(result);
}

static SYSTEMTIME * GetServerTime()
{
	GetLocalTime( &sLocalTimeServer );
	return &sLocalTimeServer;
}

extern int								iEventExpGlobal;
extern int								iServerCodeGlobal;
extern int								iRateExp;
extern BOOL								bFreeTradeChat;
extern BOOL								bHalloweenEvent;
extern int								iRateHalloweenMonster;
extern BOOL								bChristmasEvent;
extern BOOL								bMimicEvent;
extern int								iSantaId;
extern int								iSantaMightyId;
extern int								iSantaSuperId;
extern BOOL								bEasterEvent;
extern BOOL								bStarWarsEvent;
extern BOOL								bBeeEvent;
extern int								iExtraDropsEvent;
extern int								iEasterEventRate;
extern int								iEasterEventRateMapEXP;
extern BOOL								bEventGirlFree;
extern int								iBlessCastleMode;

extern BOOL								bAgingFreeEvent;
extern BOOL								bAgingNoBreakEvent;
extern BOOL								bAgingHalfPriceEvent;
extern int								SKILL_MP_COST_PERCENT;
extern int								SKILL_SP_COST_PERCENT;
extern BOOL								ALWAYS_AGING_SUCCESS;

extern int								LOOT_MODE;
//FURY ARENA
extern BOOL								bFuryArenaEnabled;
extern int								iFuryArenaKingFuryExpInitial;
extern int								iFuryArenaKingFuryGoldInitial;
extern int								iFuryArenaLordBabelExpInitial;
extern int								iFuryArenaLordBabelGoldInitial;

extern int								iFuryArenaKingFuryExpMinimum;
extern int								iFuryArenaLordBabelExpMinimum;

extern int								iFuryArenaKingFuryExpLossPerTick;
extern int								iFuryArenaKingFuryGoldLossPerTick;
extern int								iFuryArenaLordBabelExpLossPerTick;
extern int								iFuryArenaLordBabelGoldLossPerTick;

extern int								iFuryArenaKingFuryGoldPerMinion;
extern int								iFuryArenaLordBabelGoldPerMinion;

extern int								iFuryArenaKingFuryMeteorDamageMin;
extern int								iFuryArenaKingFuryMeteorDamageMax;
extern int								iFuryArenaLordBabelMeteorDamageMin;
extern int								iFuryArenaLordBabelMeteorDamageMax;


extern BOOL								bWantedMoriphHuntEvent;
extern BOOL								bWantedWolfHuntEvent;
extern int								iHuntEventMaxCount;
extern int								iHuntEventSpawnDelay;
extern int								iHuntEventSpawnDistance;

extern BOOL								bValantinesDayEvent;

extern int								iBellatraRoomScoreGoldScale;
extern int								iBellatraPerformanceGoldScale;
extern int								iBellatraMaintenanceTax;
extern int								iBellatraSecondClanTax;
extern int								iBellatraThirdClanTax;

#define USERDATATOUSER(ud)				(USERSERVER->UserdatatoUser(ud))
#define USERDATABYID(id)				(UserServer::GetUserdata((int)id))
#define USERDATABYNAME(n)				(UserServer::GetUserdata((char*)n))
#define USERDATABYACCOUNTNAME(n)		(UserServer::GetUserdataByAccount((char*)n))
#define UNITDATABYIDMAP(id,m)			(UnitServer::UnitDataByIDMap(id,m))
#define UNITDATATOUNIT(ud)				(UNITSERVER->UnitDataToUnit(ud))

// Definition Server
#define HWND_SERVER						( *( HWND* )0x07258F4 )
#define SERVER_CODE						( *( int* )0x00845858 ) //rsServerConfig.ServerCode
#define TOTAL_SERVERS					( *( int* )0x0084A578 )
#define MAX_USERS						( *( int* )0x00845868 )
#define USERS_ONLINE					( *( int* )0x07AC9BC8 )
#define SERVER_VERSION_MIN				( *( int* )0x00845988 ) // 0x00845988 // 0x003F903F9 < - original value
#define SERVER_VERSION_MAX				( *( int* )0x0084598C ) // 0x0084598C // 0x003F903FB < - original value
#define SERVER_VERSION					( *( int* )0x006E4F5C ) // 0x006E4F5C // 0x003F903F9 < - original value
#define SERVER_WINDOW					( *( int* )0x008487B8 )
#define RECORD_SAVE_CHARACTER			( *( int* )0x00845870 )
#define SERVER_NAME						( ( char* )0x00845A40 )
#define BOSS_TIME						( *( int* )0x006E46A8 )
#define CHANNEL_PACKET_SERVER			( 0x0084A550 )
#define USERSDATA						( *( UserData** )0x007AAC888 )
#define UNITSDATA						( *( UnitData** )0x007AC3E78 )
#define MAPSDATA						( *(Map**)0x07AC9CF4)
#define MAX_UNITS						2048
#define PLAYERS_MAX						1024
#define AGING_MAX						20
#define NET_SERVER_PORT					"30010"
#define NET_SERVER_MAX_CONNECTION		3
#define WM_NET_SERVER_SOCKET			0x6A60
#define ALLOCMEM						fnAllocMemoryPT
#define DEALLOCMEM						fnDeAllocMemoryPT
#define ServerTime						GetServerTime
#define FREE_TRADECHAT					bFreeTradeChat
#define TIMEHOUR						(*(UINT*)0x07AAC88C)

#define SQLCONNECTION					(SQLConnection::GetConnection)




//						Events
// Bless Castle
#define EVENT_BLESSCASTLE				( *( int* )0x00846024 )
#define EVENT_BLESSCASTLE_DAY			( *( short* )0x00846028 )
#define EVENT_BLESSCASTLE_HOUR			( *( short* )0x0084602A )
#define EVENT_BLESSCASTLE_MODE			iBlessCastleMode

// Bellatra
#define EVENT_BELLATRA					( *( int* )0x00845FEC )
#define EVENT_BELLATRA_MONSTER_EXP_TO_SCORE_SCALE			( *( int* )0x006E46A4 )
#define EVENT_BELLATRA_FIRST_CLAN_TAX	( *( int* )0x006E46A0 )
#define EVENT_BELLATRA_TEXT				( ( char* )0x07AC9E40 )
#define EVENT_BELLATRA_ROOM_SCORE_TO_GOLD_SCALE	iBellatraRoomScoreGoldScale
#define EVENT_BELLATRA_PERFORMANCE_GOLD_SCALE	iBellatraPerformanceGoldScale
#define EVENT_BELLATRA_MAINTENANCE_TAX			iBellatraMaintenanceTax
#define EVENT_BELLATRA_SECOND_CLAN_TAX			iBellatraSecondClanTax
#define EVENT_BELLATRA_THIRD_CLAN_TAX			iBellatraThirdClanTax

#define EVENT_VALENTINESDAY			    bValantinesDayEvent

//Some Events
//Haloween
#define EVENT_HALLOWEEN					bHalloweenEvent
#define RATE_HALLOWEEN					iRateHalloweenMonster

//Aging Free
#define EVENT_AGINGFREE					bAgingFreeEvent
#define EVENT_AGING_NOBREAK				bAgingNoBreakEvent
#define EVENT_AGING_HALFPRICE			bAgingHalfPriceEvent

//Christmas
#define EVENT_CHRISTMAS					bChristmasEvent

//Event girl
#define EVENT_FREEEVENTGIRL				bEventGirlFree

//Event mimic
#define EVENT_MIMIC						bMimicEvent

//FURY ARENA
//==============================================================================================
#define FURYARENA_ISENABLED						bFuryArenaEnabled

//Initial values. Also used to define max value if killed in less than 3 minutes
#define FURYARENA_KINGFURY_EXP_INITIAL			iFuryArenaKingFuryExpInitial
#define FURYARENA_KINGFURY_GOLD_INITIAL			iFuryArenaKingFuryGoldInitial
#define FURYARENA_LORDBABEL_EXP_INITIAL			iFuryArenaLordBabelExpInitial
#define FURYARENA_LORDBABEL_GOLD_INITIAL		iFuryArenaLordBabelGoldInitial

#define FURYARENA_KINGFURY_EXP_MINIMUM			iFuryArenaKingFuryExpMinimum
#define FURYARENA_LORDBABEL_EXP_MINIMUM			iFuryArenaLordBabelExpMinimum

//PER TICK = Per 10 seconds starting at 3 minutes, up to 5 minutes
#define FURYARENA_KINGFURY_EXP_LOSSPERTICK		iFuryArenaKingFuryExpLossPerTick
#define FURYARENA_KINGFURY_GOLD_LOSSPERTICK		iFuryArenaKingFuryGoldLossPerTick
#define FURYARENA_LORDBABEL_EXP_LOSSPERTICK		iFuryArenaLordBabelExpLossPerTick
#define FURYARENA_LORDBABEL_GOLD_LOSSPERTICK	iFuryArenaLordBabelGoldLossPerTick

#define FURYARENA_KINGFURY_GOLD_PERMINION		iFuryArenaKingFuryGoldPerMinion
#define FURYARENA_LORDBABEL_GOLD_PERMINION   	iFuryArenaKingFuryGoldPerMinion

#define FURYARENA_KINGFURY_METEOR_DAMAGE_MIN 	iFuryArenaKingFuryMeteorDamageMin
#define FURYARENA_KINGFURY_METEOR_DAMAGE_MAX 	iFuryArenaKingFuryMeteorDamageMax
#define FURYARENA_LORDBABEL_METEOR_DAMAGE_MIN	iFuryArenaLordBabelMeteorDamageMin
#define FURYARENA_LORDBABEL_METEOR_DAMAGE_MAX	iFuryArenaLordBabelMeteorDamageMax

//==============================================================================================


//Wanted Moriph / Wanted Wolf
#define EVENT_WANTEDMORIPH_EVENT		bWantedMoriphHuntEvent
#define EVENT_WANTEDWOLF_EVENT			bWantedWolfHuntEvent

//For Wanted Moriph or Wanted Wolf
#define EVENT_HUNT_MAX_SPAWN			iHuntEventMaxCount
#define EVENT_HUNT_MIN_SPAWN_MIN_DIST	iHuntEventSpawnDistance
#define EVENT_HUNT_SPAWN_DELAY_MIN		iHuntEventSpawnDelay

//#define EVENT_VALENTINEDAY				( *( BYTE* )0x00846004 )
#define RATE_EXP						iRateExp
#define EVENT_EXP						iEventExpGlobal

#define EVENT_EXTRADROPS				iExtraDropsEvent
#define EVENT_EASTER					bEasterEvent
#define EVENT_STARWARS					bStarWarsEvent
#define EVENT_BEE						bBeeEvent
#define EVENT_REDUCE_DAMAGEMON_LIVE		(*(BOOL*)0x07AC9D4C)
#define EVENT_REDUCE_DAMAGEMON			(*(BOOL*)0x00845FF0)

#endif