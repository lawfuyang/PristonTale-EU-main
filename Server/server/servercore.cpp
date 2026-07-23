#include "stdafx.h"
#include "servercore.h"

#include "CServer.h"
#include "CConfigFileReader.h"
#include "Logger.h"

DWORD * ServerCore::pdwTickCount = (DWORD *)0x07AC9D30;
DWORD * ServerCore::pdwUnitsOnline = (DWORD *)0x07AC9B94;
DWORD * ServerCore::pdwUsersOnline = (DWORD *)0x07AC9BC8;


#define	srRECORD_DEFAULT_SIZE	16384

extern void InitSkills();



static std::string			sCurrCharName;
static std::string			sCurrAccountName;


void ServerCore::LoadDirty()
{
	SetTickCounts( GetTickCount() );

	if ( GAME_SERVER )
		WRITEDWORD( 0x07AC9D3C, 1 ); //BOOL isGameServer

	string sName = "";
	DWORD dwIPKeyLogin = 0;
	BOOL bSiegeWar = FALSE;
	int iSiegeDay = 0;
	int iSiegeHour = 0;

	CConfigFileReader * pcConfigFileReader = new CConfigFileReader( "server.ini" );
	if ( pcConfigFileReader->Open() )
	{
		sName = pcConfigFileReader->ReadString( "LoginServer", "Name" );

		dwIPKeyLogin = (DWORD)inet_addr( pcConfigFileReader->ReadString( "LoginServer", "IP" ).c_str() );

		pcConfigFileReader->Close();
	}
	SAFE_DELETE( pcConfigFileReader );


	//This partial with stage decompressing can and should be removed from server so these valeus can be removed safely anyway ;)
	ZeroMemory( (void *)0x008487B8, 7572 );
	ZeroMemory( (void *)0x00848320, 172 );
	ZeroMemory( (void *)0x0084A550, 908 );
	ZeroMemory( (void *)0x0084585C, 10948 );
	WRITEDWORD( 0x0088BCE4, 0 );
	// Open Server
	SERVER_WINDOW = -1;

	// Enable Record Save Character
	RECORD_SAVE_CHARACTER = TRUE;
	WRITEDWORD( 0x008487BC, 0x0320 );
	WRITEDWORD( 0x008487C0, 0x0258 );
	WRITEDWORD( 0x008487C4, 0x10 );
	WRITEDWORD( 0x0084A26C, 0 );
	WRITEDWORD( 0x008487C8, 0 );
	WRITEDWORD( 0x008487D0, 2 );
	WRITEDWORD( 0x008487CC, 1 );
	WRITEDWORD( 0x0084A400, 0xA0 );
	WRITEDWORD( 0x0084A404, 0x012C );
	WRITEDWORD( 0x0084A408, 0x0100 );
	WRITEDWORD( 0x0084A40C, -0x0100 );
	WRITEDWORD( 0x0084A410, 0x80 );


	/* Core Server Defined Settings */
																//Server Code

	StringCchCopyA( SERVER_NAME, 32, sName.c_str() );											//Server Name

	WRITEDWORD( 0x0084609C, 1 );
	WRITEDWORD( 0x00845874, 1 );

	INI::CReader cReader( "server.ini" );


	INFO ("INIT SERVER");

	if ( GAME_SERVER )
	{
		//Exp
		RATE_EXP = cReader.ReadInt( "Event", "RateExp" );
		EVENT_EXP = cReader.ReadInt( "Event", "EventExp" );

		//Bless Castle
		if ( cReader.ReadOnOff( "Event", "SiegeWar" ) )
		{
			EVENT_BLESSCASTLE = 1;
			EVENT_BLESSCASTLE_DAY = cReader.ReadInt( "Event", "SiegeWarDay" );
			EVENT_BLESSCASTLE_HOUR = cReader.ReadInt( "Event", "SiegeWarHour" );
		}

		CHARACTERSERVER->SetSiegeWarDay( cReader.ReadInt( "Event", "SiegeWarDay" ) );
		CHARACTERSERVER->SetSiegeWarHour( cReader.ReadInt( "Event", "SiegeWarHour" ) );

		//Bellatra
		if ( cReader.ReadOnOff( "Event", "Bellatra" ) )
		{
			EVENT_BELLATRA = 2;
			EVENT_BELLATRA_MONSTER_EXP_TO_SCORE_SCALE = cReader.ReadInt( "Event", "BellatraMonsterExpToScoreScale" );
			EVENT_BELLATRA_FIRST_CLAN_TAX = cReader.ReadInt( "Event", "BellatraFirstClanTax" );
			EVENT_BELLATRA_SECOND_CLAN_TAX = cReader.ReadInt("Event", "BellatraSecondClanTax");
			EVENT_BELLATRA_THIRD_CLAN_TAX = cReader.ReadInt("Event", "BellatraThirdClanTax");
			STRINGCOPYLEN( EVENT_BELLATRA_TEXT, 64, cReader.ReadString( "Event", "BellatraText" ).c_str() );


			EVENT_BELLATRA_PERFORMANCE_GOLD_SCALE = cReader.ReadInt("Event", "BellatraPerformanceGoldScale");
			EVENT_BELLATRA_ROOM_SCORE_TO_GOLD_SCALE = cReader.ReadInt("Event", "BellatraSharedGoldScale");
			EVENT_BELLATRA_MAINTENANCE_TAX = cReader.ReadInt("Event", "BellatraMaintenanceTax");

			INFO ("BellatraLeadingClanTax = %d", EVENT_BELLATRA_FIRST_CLAN_TAX);
			INFO ("BellatraPerformanceGoldScale = %d", EVENT_BELLATRA_PERFORMANCE_GOLD_SCALE);
			INFO ("BellatraSharedGoldScale = %d", EVENT_BELLATRA_ROOM_SCORE_TO_GOLD_SCALE);
			INFO ("BellatraMaintenanceTax = %d", EVENT_BELLATRA_MAINTENANCE_TAX);
		}


		//Wanted Moriph
		if ( cReader.ReadOnOff( "Event", "WantedMoriph" ) )
		{
			EVENT_WANTEDMORIPH_EVENT = TRUE;

			EVENT_HUNT_MAX_SPAWN = cReader.ReadInt( "Event", "WantedMoriphMaxSpawnCount" );
			EVENT_HUNT_SPAWN_DELAY_MIN = cReader.ReadInt( "Event", "WantedMoriphSpawnDelayMin" );
		}

		//Wanted Wolf
		if (cReader.ReadOnOff("Event", "WantedWolf"))
		{
			EVENT_WANTEDWOLF_EVENT = TRUE;

			EVENT_HUNT_MAX_SPAWN = cReader.ReadInt("Event", "WantedWolfMaxSpawnCount");
			EVENT_HUNT_SPAWN_DELAY_MIN = cReader.ReadInt("Event", "WantedWolfSpawnDelayMin");
		}

		//Reduce Monster Damage
		if ( cReader.ReadOnOff( "Event", "MonsterDamageReduce" ) )
		{
			EVENT_REDUCE_DAMAGEMON = TRUE;
			EVENT_REDUCE_DAMAGEMON_LIVE = TRUE;
		}
	}



	//Wanted Moriph
	if ( cReader.ReadOnOff( "Event", "Mimic" ) )
	{
		EVENT_MIMIC = TRUE;
		INFO( "Event> Mimic enabled" );
	}

	//Halloween
	if ( cReader.ReadOnOff( "Event", "Halloween" ) )
	{
		EVENT_HALLOWEEN = TRUE;
		RATE_HALLOWEEN = cReader.ReadInt("Event", "HalloweenPercent");

		INFO( "Event> Halloween enabled" );
	}

	//Christmas
	if ( cReader.ReadOnOff ( "Event", "Christmas" ) )
	{
		EVENT_CHRISTMAS = TRUE;

		INFO( "Event> Chrismas enabled" );
	}

	//Easter
	if (cReader.ReadOnOff("Event", "Easter"))
	{
		EVENT_EASTER = TRUE;

		//Set Christmas event to false if Easter event is true
		//otherwise logic will fail in CManufactureHandler.cpp
		EVENT_CHRISTMAS = FALSE;

		INFO( "Event> Easter enabled" );
	}

	//Star wars
	if (cReader.ReadOnOff("Event", "StarWars"))
	{
		EVENT_STARWARS = TRUE;
		EVENT_EASTER = FALSE;

		INFO( "Event> StarWars enabled" );
	}

	//Star wars
	if (cReader.ReadOnOff("Event", "Bee"))
	{
		EVENT_BEE = TRUE;
		EVENT_EASTER = FALSE;
		EVENT_STARWARS = FALSE;

		INFO( "Event> Bee enabled" );
	}

	//Christmas
	if ( cReader.ReadOnOff ( "Event", "EventGirlFree" ) )
	{
		EVENT_FREEEVENTGIRL = TRUE;

		INFO( "Event> Free Event Girl enabled" );
	}

	//Free aging
	if (cReader.ReadOnOff("Event", "AgingFree"))
	{
		EVENT_AGINGFREE = TRUE;

		INFO( "Event> Free Aging enabled" );
	}

	//Aging no break  event
	if (cReader.ReadOnOff("Event", "AgingNoBreak"))
	{
		EVENT_AGING_NOBREAK = TRUE;

		INFO( "Event> No Break Aging enabled" );
	}

	//Skill MP cost percent (100 = normal, 50 = half cost, 0 = free)
	SKILL_MP_COST_PERCENT = cReader.ReadInt("Event", "SkillMPCostPercent");
	if (SKILL_MP_COST_PERCENT <= 0) SKILL_MP_COST_PERCENT = 100;
	INFO("Event> Skill MP Cost: %d%%", SKILL_MP_COST_PERCENT);

	//Skill SP (Stamina) cost percent
	SKILL_SP_COST_PERCENT = cReader.ReadInt("Event", "SkillSPCostPercent");
	if (SKILL_SP_COST_PERCENT <= 0) SKILL_SP_COST_PERCENT = 100;
	INFO("Event> Skill SP Cost: %d%%", SKILL_SP_COST_PERCENT);

	//Always aging success (never fail, never break)
	ALWAYS_AGING_SUCCESS = cReader.ReadOnOff("Event", "AlwaysAgingSuccess");
	if (ALWAYS_AGING_SUCCESS)
		INFO("Event> Always Aging Success: ON (never fail, never break)");


	//Loot mode: 0=normal, 1=cheat (no gold, always perfect, class match, always spec)
	LOOT_MODE = cReader.ReadInt("Event", "LootMode");
	if (LOOT_MODE) INFO("Event> Loot Mode: CHEAT (no gold, perfect, class match, spec)");

	//Aging no break  event
	if (cReader.ReadOnOff("Event", "AgingHalfPrice"))
	{
		EVENT_AGING_HALFPRICE = TRUE;

		INFO( "Event> Half Price Aging enabled" );
	}

	EVENT_BLESSCASTLE_MODE = cReader.ReadInt( "Event", "SiegeWarMode" );
	WRITEDWORD( 0x08B86750, EVENT_BLESSCASTLE_MODE );

	//The Grand Fury
	//Note - The Fury reward parameters will be read  in via  CFuryArenaHandler::ReadConfigIniValues()
	FURYARENA_ISENABLED	 = cReader.ReadOnOff("Event", "FuryArena");

	WRITEDWORD( 0x0061A26C, 0x1400 );

	// Level Server
	*(int*)(0x08B80725) = 140;
	*(int*)(0x08B80739) = 140;
	*(int*)(0x08B80752) = 140;

	// Set Alloc Size
	{
		// Monsters
		*(UINT*)0x08B81328 = sizeof( CharacterData ) * (MAX_UNITS + 1);
		*(UINT*)0x08B8132C = sizeof( UnitInfo ) * (MAX_UNITS + 1);

		*(UINT*)0x08B81380 = sizeof( UnitData ) * (MAX_UNITS)+8;	// Alloc Size
		*(UINT*)0x08B81384 = (*(UINT*)0x08B81380) - 8;							    // Alloc Size
		*(UINT*)0x08B81388 = MAX_UNITS;								// Max pcUnitData online
		*(UINT*)0x08B8138C = ((*(UINT*)0x08B81380) - 8) / 4;						// Clear Memory size

																					// Items
		*(UINT*)0x08B81330 = sizeof( DefinitionItem ) * (ITEM_SERVER_MAX + 1);

		// Generate Item Form
		(*(WORD*)0x08B64170) = 27242;
	}

	// Item Requeriments
	{
		DWORD old;
		VirtualProtect( (void*)0x0043C22A, 4, PAGE_EXECUTE_READWRITE, &old );
		VirtualProtect( (void*)0x0043C3F1, 4, PAGE_EXECUTE_READWRITE, &old );
		*(UINT*)0x0043C22A = (UINT)&saItemRequeriments + 4;
		*(UINT*)0x0043C3F1 = (UINT)&saItemRequeriments[11];
		VirtualProtect( (void*)0x00439D9C, 4, PAGE_EXECUTE_READWRITE, &old );
		VirtualProtect( (void*)0x00439F3A, 4, PAGE_EXECUTE_READWRITE, &old );
		*(UINT*)0x00439D9C = (UINT)&saItemRequeriments + 4;
		*(UINT*)0x00439F3A = (UINT)&saItemRequeriments[11];
	}

	//Threading of these functions can be considered to be removed imo
	ZeroMemory( (void *)0x07A9EA60, 0xB0 );

	if ( LOGIN_SERVER )
	{
		// LOGS
		(*(BOOL*)0x07A9EA68) = 1;
		(*(DWORD*)(0x07A9EAA4)) = (DWORD)&AccountServer::SQLCharacterLog;
		(*(DWORD*)(0x07A9EB04)) = (DWORD)&AccountServer::SQLUserOnline;
	}

	CALL( 0x00467DC0 );

	InitSkills();

	MIXHANDLER->SQLLoadMixingList();
	//QUESTSERVER->UpdateQuestActiveMonsterKill();

	TEST( "Class Map", sizeof( BaseMap ), 0x0EA8 );

	BOSS_TIME = Dice::RandomI ( 15, 45 );

	if ( GAME_SERVER )
	{
		GSERVER->SqlUpdateOrInsertMetadata ( "boss.time.second", BOSS_TIME );
	}

	(*(WORD*)0x07AC9F54) = GetServerTime()->wDay;
}

void ServerCore::UnLoadDirty()
{
	CALL( 0x0054F6F0 ); //Unload .inx files
	CALL( 0x005492D0 ); //Unload Item Handling Thread Memory

	WRITEDWORD( 0x00726B38, 1 );  //Used by threads to check if server is shutting down, to ExitThread()
}

__declspec(naked) long ServerCore::Hash( char * s )
{
	JUMP( pfnHash );
}

/**
Dumb item metadata (not all) from warehouse or character inventory
into database
*/
void ServerCore::DumpItemDataToSQL(Item* psItem, SQLConnection* psSql, BOOL bIsInventory, const char* pszAccountName, const char* pszCharName)
{
	int iItemCode = psItem->sItemID.ToItemID();
	int iItemTypeCode = psItem->sItemID.ToItemType();
	int iItemBaseCode = psItem->sItemID.ToItemBase();

	std::string query = "INSERT INTO [dbo].[ItemDump]("
		"[AccountName],[CharName],[ItemName],[ItemLevel],[ItemId],"
		"[ItemTypeId],[ItemBaseId],[ItemHeader],[ItemChecksum],[ItemBackupHeader],"
		"[ItemBackupChecksum],[MixEffect],[MixID],[AgeLevel],[AttackRange],"
		"[AttackSpeed],[AttackRating],[ATKPowMin],[ATKPowMax],[Critical],"
		"[Absorb],[Defense],[Block],[Organic],[Fire],"
		"[Frost],[Lighting],[Poison],[StrengthReq],[SpiritReq],"
		"[TalentReq],[AgilityReq],[HealthReq],[MPRegen],[HPRegen],"
		"[SPRegen],[AddHP],[AddMP],[AddSP],[ItemSpec],[SpecAbsorb],"
		"[SpecDef],[SpecBlockRating],[SpecAttackSpeed],[SpecCritical],[SpecAttackRatingDiv],"
		"[SpecAttackPowerDiv],[SpecMPRegen],[SpecHPRegen],[SpecSPRegen],"
		"[ItemUniqueID],[SalePrice],[CreatedDate])"
		" VALUES (";

	if (pszAccountName)
		query += "?,";
	else
		query += "NULL,";

	if (pszCharName)
		query += "?,";
	else
		query += "NULL,";

	query += "?,?,?,"
		"?,?,?,?,?,"
		"?,?,?,?,?,"
		"?,?,?,?,?,"
		"?,?,?,?,?,"
		"?,?,?,?,?,"
		"?,?,?,?,?,"
		"?,?,?,?,?,?,"
		"?,?,?,?,?,"
		"?,?,?,?,"
		"?,?,?)";

	if (psSql->Prepare(query.c_str()))
	{
		int i = 1;

		//"[IsInventory],[FileName],[ItemName],[ItemLevel],[ItemCode],"

		if (pszAccountName)
			psSql->BindParameterInput(i++, PARAMTYPE_String,	(void*)pszAccountName, 32);

		if (pszCharName)
			psSql->BindParameterInput(i++, PARAMTYPE_String,	(void*)pszCharName, 32);

		psSql->BindParameterInput(i++, PARAMTYPE_String,	psItem->szItemName, 50);
		psSql->BindParameterInput(i++, PARAMTYPE_Integer,	&psItem->iLevel);
		psSql->BindParameterInput(i++, PARAMTYPE_Integer,	&iItemCode);


		//"[ItemBaseCode],[ItemCode],[ItemHeader],[ItemChecksum],[ItemBackupHeader],"
		psSql->BindParameterInput(i++, PARAMTYPE_Integer,	&iItemTypeCode);
		psSql->BindParameterInput(i++, PARAMTYPE_Integer,	&iItemBaseCode);
		psSql->BindParameterInput(i++, PARAMTYPE_Integer,	&psItem->iChk1);
		psSql->BindParameterInput(i++, PARAMTYPE_Integer,	&psItem->iChk2);
		psSql->BindParameterInput(i++, PARAMTYPE_Integer,	&psItem->iBackupKey);

		//"[ItemBackupChecksum],[MixEffect],[MixID],[AgeLevel],[AttackRange],"
		psSql->BindParameterInput(i++, PARAMTYPE_Integer,	&psItem->iBackupChk);
		psSql->BindParameterInput(i++, PARAMTYPE_Integer,	&psItem->eMixEffect);
		psSql->BindParameterInput(i++, PARAMTYPE_Integer,	&psItem->sMixUniqueID1);
		psSql->BindParameterInput(i++, PARAMTYPE_Integer,	&psItem->sAgeLevel);
		psSql->BindParameterInput(i++, PARAMTYPE_Integer,	&psItem->iAttackRange);

		//"[AttackSpeed],[AttackRating],[ATKPowMin],[ATKPowMax],[Critical],"
		psSql->BindParameterInput(i++, PARAMTYPE_Integer,	&psItem->iAttackSpeed);
		psSql->BindParameterInput(i++, PARAMTYPE_Integer,	&psItem->iAttackRating);
		psSql->BindParameterInput(i++, PARAMTYPE_Short,		&psItem->sDamage.sMin);
		psSql->BindParameterInput(i++, PARAMTYPE_Short,		&psItem->sDamage.sMax);
		psSql->BindParameterInput(i++, PARAMTYPE_Integer,	&psItem->iCritical);

		//"[Absorb],[Defense],[Block],[Organic],[Fire],"
		psSql->BindParameterInput(i++, PARAMTYPE_Float,		&psItem->fAbsorbRating, 0);
		psSql->BindParameterInput(i++, PARAMTYPE_Integer,	&psItem->iDefenseRating);
		psSql->BindParameterInput(i++, PARAMTYPE_Float,		&psItem->fBlockRating, 0);
		psSql->BindParameterInput(i++, PARAMTYPE_Short,		&psItem->saElementalDef[0]);
		psSql->BindParameterInput(i++, PARAMTYPE_Short,		&psItem->saElementalDef[2]);

		//"[Frost],[Lightning],[Poison],[StrengthReq],[SpiritReq],"
		psSql->BindParameterInput(i++, PARAMTYPE_Short,		&psItem->saElementalDef[3]);
		psSql->BindParameterInput(i++, PARAMTYPE_Short,		&psItem->saElementalDef[4]);
		psSql->BindParameterInput(i++, PARAMTYPE_Short,		&psItem->saElementalDef[5]);
		psSql->BindParameterInput(i++, PARAMTYPE_Integer,	&psItem->iStrength);
		psSql->BindParameterInput(i++, PARAMTYPE_Integer,	&psItem->iSpirit);

		//"[TalentReq],[AgilityReq],[HealthReq],[MPRegen],[HPRegen],"
		psSql->BindParameterInput(i++, PARAMTYPE_Integer,	&psItem->iTalent);
		psSql->BindParameterInput(i++, PARAMTYPE_Integer,	&psItem->iAgility);
		psSql->BindParameterInput(i++, PARAMTYPE_Integer,	&psItem->iHealth);
		psSql->BindParameterInput(i++, PARAMTYPE_Float,		&psItem->fMPRegen, 0);
		psSql->BindParameterInput(i++, PARAMTYPE_Float,		&psItem->fHPRegen, 0);

		//"[SPRegen],[AddHP],[AddMP],[AddSP],[ItemSpec],[SpecAbsorb],"
		psSql->BindParameterInput(i++, PARAMTYPE_Float,		&psItem->fSPRegen, 0);
		psSql->BindParameterInput(i++, PARAMTYPE_Float,		&psItem->fAddHP, 0);
		psSql->BindParameterInput(i++, PARAMTYPE_Float,		&psItem->fAddMP, 0);
		psSql->BindParameterInput(i++, PARAMTYPE_Float,		&psItem->fAddSP, 0);
		psSql->BindParameterInput(i++, PARAMTYPE_Integer,	&psItem->eSpecialization);
		psSql->BindParameterInput(i++, PARAMTYPE_Float,		&psItem->sSpecData.fSpecAbsorbRating, 0);

		//"[SpecDef],[SpecBlockRating],[SpecAttackSpeed],[SpecCritical],[SpecAttackRatingDiv],"
		psSql->BindParameterInput(i++, PARAMTYPE_Integer,	&psItem->sSpecData.iSpecDefenseRating);
		psSql->BindParameterInput(i++, PARAMTYPE_Float,		&psItem->sSpecData.fSpecBlockRating, 0);
		psSql->BindParameterInput(i++, PARAMTYPE_Integer,	&psItem->sSpecData.iSpecAttackSpeed);
		psSql->BindParameterInput(i++, PARAMTYPE_Integer,	&psItem->sSpecData.iSpecCritical);
		psSql->BindParameterInput(i++, PARAMTYPE_Short,		&psItem->sSpecData.iSpecAttackRatingDiv);

		//"[SpecAttackPowerDiv],[SpecMPRegen],[SpecHPRegen],[SpecSPRegen],"
		psSql->BindParameterInput(i++, PARAMTYPE_Short,		&psItem->sSpecData.iSpecAttackPowerDiv);
		psSql->BindParameterInput(i++, PARAMTYPE_Float,		&psItem->sSpecData.fSpecMPRegen, 0);
		psSql->BindParameterInput(i++, PARAMTYPE_Float,		&psItem->sSpecData.fSpecHPRegen, 0);
		psSql->BindParameterInput(i++, PARAMTYPE_Float,		&psItem->sSpecData.fSpecSPRegen, 0);

		//"[ItemUniqueID],[SalePrice],[CreatedDate])"
		psSql->BindParameterInput(i++, PARAMTYPE_Integer,	&psItem->iItemUniqueID);
		psSql->BindParameterInput(i++, PARAMTYPE_Integer,	&psItem->iSalePrice);
		psSql->BindParameterInput(i++, PARAMTYPE_Integer,	&psItem->dwTime);

		psSql->Execute();

		psSql->Clear();
	}
}

const char * ServerCore::GetCharOrAccountName()
{
	if ( !sCurrCharName.empty() )
	{
		if ( !sCurrAccountName.empty() )
		{
			return FormatString( "[%s][ %s ] ", sCurrAccountName.c_str(), sCurrCharName.c_str() );
		}
		else
		{
			return FormatString( "[ %s ] ", sCurrCharName.c_str() );
		}
	}
	else if ( !sCurrAccountName.empty() )
	{
		return FormatString( "[%s] ", sCurrAccountName.c_str() );
	}
	else
	{
		return "";
	}
}

BOOL ServerCore::CheckAndAdjustFloat ( const char * szItemName, ItemID pItemID, const char * label, float * pFloat, float minValue, float maxValue, float minValueOld, float maxValueOld )
{
	if ( minValue > maxValue )
	{
		LOGERROR ( "%s%s: %s has min value %0.1f greater than max value %0.1f", GetCharOrAccountName(), szItemName, label, minValue, maxValue );
		return FALSE;
	}

	float fDiffMin = *pFloat - minValue;
	float fDiffMax = *pFloat - maxValue;

	float fDiffMinOld = *pFloat - minValueOld;
	float fDiffMaxOld = *pFloat - maxValueOld;

	//Is new value different than the old values?
	BOOL bValuesIsDifferent = minValue != minValueOld || maxValue != maxValueOld;

	//Is the current value within range of the old values?
	BOOL bWithinRangeOfOldValues = fDiffMinOld > -0.1 && fDiffMaxOld < 0.1;

	//Is the current value within range of the new values?
	BOOL bWithinRangeOfNewValues = fDiffMin > -0.1 && fDiffMaxOld < 0.1;


	//current value is zero, or old values are zero, but now we have a new value
	BOOL bHasNewValue = (*pFloat == 0.0f || minValueOld == 0.0f || maxValueOld == 0.0f) && ( minValue > 0.1f || maxValue > 0.1f );

	//current stats is lower than the original min stats?? force reroll
	BOOL bForceRerollFull = fDiffMinOld <= -0.1 || bHasNewValue;

	//current stats is higher than the original max stats?? force reroll using upper 80th percentile
	BOOL bForceRerollUpperTier = fDiffMaxOld >= 0.1;

	float newValue = -1.0f;

	//full reroll
	if ( bForceRerollFull )
	{
		newValue = RoundOffToNearestDP ( Dice::RandomF ( minValue, maxValue ), 1 );

		if ( bHasNewValue )
		{
			INFO ( "%s%s: %s has a new value: Forcing a full reroll to %0.1f from 0.0 (reroll from range: %0.1f - %0.1f)", GetCharOrAccountName(), szItemName, label, newValue, minValue, maxValue );
		}
		else if ( !bValuesIsDifferent )
		{
			INFO( "%s%s: %s %0.1f is outside of min value: %0.1f, correcting to %0.1f (reroll from range: %0.1f - %0.1f)", GetCharOrAccountName(), szItemName, label, *pFloat, minValueOld, newValue, minValue, maxValue);
		}
		else
		{
			INFO ( "%s%s: %s %0.1f is outside of old min value: %0.1f, correcting to %0.1f (reroll from range: %0.1f - %0.1f)", GetCharOrAccountName(), szItemName, label, *pFloat, minValueOld, newValue, minValue, maxValue );
		}
	}

	//reroll using upper 80th percentile
	else if ( bForceRerollUpperTier && maxValue > minValue )
	{
		float range = ( maxValue - minValue );
		float percentile80 = range * 0.8f;
		newValue = RoundOffToNearestDP ( Dice::RandomF ( minValue + percentile80, maxValue ), 1 );

		INFO ( "%s%s: %s %0.1f is outside of old max value: %0.1f, correcting to %0.1f (random upper 80th from range: %0.1f - %0.1f)", GetCharOrAccountName(), szItemName, label, *pFloat, maxValueOld, newValue, minValue, maxValue );
	}

	//value is different, but we'd want to perfect stats vs perfect
	else if ( bValuesIsDifferent )
	{
		newValue = maxValue + fDiffMaxOld;

		if ( newValue > maxValue )
		{
			newValue = maxValue;
			INFO ( "%s%s: %s %0.1f (old max %0.1f) updated to %0.1f (new max %0.1f) due to 'P' %0.1f stats falling outside of new range: %0.1f - %0.1f", GetCharOrAccountName(), szItemName, label, *pFloat, maxValueOld, newValue, maxValue, fDiffMaxOld, minValue, maxValue );
		}
		else if ( newValue < minValue )
		{
			newValue = minValue;
			INFO ( "%s%s: %s %0.1f (old max %0.1f) updated to %0.1f (new min %0.1f) due to 'P' %0.1f stats falling outside of new range: %0.1f - %0.1f", GetCharOrAccountName(), szItemName, label, *pFloat, maxValueOld, newValue, minValue, fDiffMaxOld, minValue, maxValue );
		}
		else if ( newValue != *pFloat )
		{
			INFO ( "%s%s: %s %0.1f (old max %0.1f) updated to %0.1f (new max %0.1f) ('P' stats of %0.1f perserved)", GetCharOrAccountName(), szItemName, label, *pFloat, maxValueOld, newValue, maxValue, fDiffMaxOld );
		}
		else
		{
			newValue = -1.0f; //nothing was changed
		}
	}
	else
	{
		//Nothing is changed
		return FALSE;
	}

	if ( newValue < 0.0f )
	{
		//Nothing is changed
		return FALSE;
	}

	*pFloat = newValue;
	return TRUE;
}

BOOL ServerCore::CheckAndAdjustFloat ( const char * szItemName, ItemID pItemID, const char * label, float * pFloat, float value )
{
	float fDiff = *pFloat - value;

	if ( fDiff <= -0.1 || fDiff >= 0.1 )
	{
		INFO ( "%s%s: %s %0.1f is outside of base value: %0.1f, correcting to %0.1f", GetCharOrAccountName(), szItemName, label, *pFloat, value, value );

		*pFloat = value;
		return TRUE;
	}

	return FALSE;
}


BOOL ServerCore::CheckAndAdjustInt ( const char * szItemName, ItemID pItemID, const char * label, int * pInt, int minValue, int maxValue, int minValueOld, int maxValueOld )
{
	if ( minValue > maxValue )
	{
		LOGERROR ( "%s%s: %s has min value %d greater than max value %d", GetCharOrAccountName(), szItemName, label, minValue, maxValue );
		return FALSE;
	}

	//Get the difference vs old max
	int iDiffMaxOld = *pInt - maxValueOld;

	//Is new value different than the old values?
	BOOL bValuesIsDifferent = minValue != minValueOld || maxValue != maxValueOld;

	//Is the current value within range of the old values?
	BOOL bWithinRangeOfOldValues = *pInt > minValueOld && *pInt < maxValueOld;

	//Is the current value within range of the old values?
	BOOL bWithinRangeOfNewValues = *pInt > minValue && *pInt < maxValue;

	//current value is zero, or old values are zero, but now we have a new value
	BOOL bHasNewValue = ( *pInt == 0 || minValueOld == 0 || maxValueOld == 0 ) && ( minValue > 0 || maxValue > 0 );

	//current stats is lower than the original min stats?? force reroll
	BOOL bForceRerollFull = *pInt < minValueOld || bHasNewValue;

	//current stats is higher than the original max stats?? force reroll using upper 80th percentile
	BOOL bForceRerollUpperTier = *pInt > maxValueOld;

	int newValue = -1;

	//full reroll
	if ( bForceRerollFull )
	{
		newValue = Dice::RandomI ( minValue, maxValue );

		if ( bHasNewValue )
		{
			INFO ( "%s%s: %s has a new value: Forcing a full reroll to %d from 0 (reroll from new range: %d - %d)", GetCharOrAccountName(), szItemName, label, newValue, minValue, maxValue );
		}
		else
		{
			INFO ( "%s%s: %s %d is outside of old min value: %d, correcting to %d (reroll from new range: %d - %d)", GetCharOrAccountName(), szItemName, label, *pInt, minValueOld, newValue, minValue, maxValue );
		}
	}

	//reroll using upper 80th percentile
	else if ( bForceRerollUpperTier )
	{
		int range = ( maxValue - minValue );
		float percentile80 = static_cast<float>( range ) * 0.8f;

		newValue = static_cast<int>( round( Dice::RandomF( minValue + percentile80, static_cast<float>( maxValue ) ) ) );

		if ( pItemID.ToInt() != 16843008 ) //Stone axe (For some reason it always had to be corrected..)
		{
			INFO( "%s%s: %s %d is outside of old max value: %d, correcting to %d (random upper 80th from range: %d - %d)", GetCharOrAccountName(), szItemName, label, *pInt, maxValueOld, newValue, minValue, maxValue );
		}
	}

	//value is different, but we'd want to perfect stats vs perfect
	else if ( bValuesIsDifferent )
	{
		newValue = maxValue + iDiffMaxOld;

		if ( newValue < minValue )
		{
			newValue = minValue;
			INFO ( "%s%s: %s %d (old max %d) updated to %d (new min %d) due to 'P' %d stats falling outside of new range: %d - %d", GetCharOrAccountName(), szItemName, label, *pInt, maxValueOld, newValue, minValue, iDiffMaxOld, minValue, maxValue );
		}
		else if ( newValue != *pInt )
		{
			INFO ( "%s%s: %s %d (old max %d) updated to %d (new max %d) (P stats of %d perserved)", GetCharOrAccountName(), szItemName, label, *pInt, maxValueOld, newValue, maxValue, iDiffMaxOld );
		}
		else
		{
			newValue = -1; //nothing was changed
		}
	}

	if ( newValue == -1 )
	{
		//Nothing is changed
		return FALSE;
	}

	*pInt = newValue;
	return TRUE;
}


BOOL ServerCore::CheckAndAdjustInt ( const char * szItemName, ItemID pItemID, const char * label, int * pInt, int value )
{
	if ( *pInt != value )
	{
		INFO ( "%s%s: %s %d is outside of base value: %d, correcting to %d", GetCharOrAccountName(), szItemName, label, *pInt, value, value );
		*pInt = value;
		return TRUE;
	}

	return FALSE;
}

BOOL ServerCore::CheckAndAdjustShort ( const char * szItemName, ItemID pItemID, const char * label, short * pShort, short minValue, short maxValue, short minValueOld, short maxValueOld )
{
	int tempInt = *pShort;

	if ( CheckAndAdjustInt ( szItemName, pItemID, label, &tempInt, (int)minValue, (int)maxValue, (int)minValueOld, (int)maxValueOld ) )
	{
		*pShort = static_cast<short>( tempInt );
		return TRUE;
	}

	return FALSE;
}

BOOL ServerCore::CheckAndAdjustShort ( const char * szItemName, ItemID pItemID, const char * label, short * pShort, short value )
{
	if ( *pShort != value )
	{
		INFO ( "%s%s: %s %d is outside of base value: %d, correcting to %d", GetCharOrAccountName(), szItemName, label, *pShort, value, value );
		*pShort = value;
		return TRUE;
	}

	return FALSE;
}

/// <summary>
/// Existing Item Updater Function
///
/// Please follow them exactly otherwise it will break the process!!:
///
/// 1) Backup ItemListOld (if it exists) and ItemList
///
/// 2) Before you make changes to ItemsList table, create a clone of the original
///    ItemList as ItemListOld by running this query:
///
///    DROP TABLE ItemListOld; Select * INTO ItemListOld from ItemList;
///
///    Now you have ItemListOld which contains the ORIGINAL values before the modifications.
///    This is needed because we need to figure out how 'perfect' the original items are
///    as we will be removing the mix and age from each item before adjusting the changes
///    THEN apply back in the original mix and age.
///
/// 3) Now you can modify ItemList table. I suggest to put all the changes you need into one .sql file, so that
///    if you screw up, you can restore the original ItemList table and then run the .sql again.
///
/// 4) Backup Character, CharacterBackup, Warehouse, WarehouseBackup, WarehouseSeasonal by zipping it all.
///
/// 4) Enable UpdateItemsAndDumpItemsToItemsDB = True on login's server.ini
///
/// 5) Run login server and wait for the upgrade process to be done. This will update all items in Character and Warehouse folders
///
///    If you see WARN or ERRORs, it could indicate problem. Check with a dev! You might need to roll back the character and warehouse folders
///    BEFORE running it again otherwise some items gets updated twice!
///
/// 6) After this is completed, and everything is all good run this query to have ItemListOld and ItemList match.
///
///    DROP TABLE ItemListOld; Select * INTO ItemListOld from ItemList;
///
///    If you don't do this, someone might set `UpdateItemsAndDumpItemsToItemsDB = True` and then all items gets double upgraded.///
///
/// This process allows you to modify any items in the ItemList table and then it will automatically adjust all existing items
/// regardless of item type.And it will maintain the 'P' stats of existing stats.For new stats, it will do a full reroll.
///
/// It is recommended to use this sheet as a template to update existing items: https://docs.google.com/spreadsheets/d/1WT5tQo2NylxwNvjMau56DmPtvHS1xpDp-Srq2S_wieM/edit#gid=847858883
///
/// As always, if in doubt or have questions, please ask Iron!
///
/// </summary>
BOOL ServerCore::CheckAndUpdateStats ( Item * psItem )
{
    Item lsItem;
    CopyMemory ( &lsItem, psItem, sizeof ( Item ) );

	BOOL isNonMixableNonAgable =
		lsItem.sItemID.ToItemType () == ITEMTYPE_Costume ||
		lsItem.sItemID.ToItemType () == ITEMTYPE_Costume2 ||
		lsItem.sItemID.ToItemType () == ITEMTYPE_Costume3 ||
		lsItem.sItemID.ToItemType () == ITEMTYPE_Costume4 ||
		lsItem.sItemID.ToItemType () == ITEMTYPE_Event;

	BOOL bAnyChanged = FALSE;

    if ( lsItem.eCraftType == EItemCraftType::ITEMCRAFTTYPE_Mixing )
    {
        //legacy mixes. Cannot really determine what the orignal stats are
        if ( lsItem.eMixTypeName == EMixTypeName::MIXTYPENAME_LegacyDoNotUse )
        {
            return FALSE;
        }
		else if ( isNonMixableNonAgable )
		{
			WARN ( "Item %s is mixed. Removing mix", psItem->szItemName );

			psItem->eCraftType = EItemCraftType::ITEMCRAFTTYPE_None;
			psItem->sAgeLevel = 0;
			psItem->sMatureBar.sCur = 0;
			psItem->sMatureBar.sMax = 0;
			psItem->bMaturing = false;

			lsItem.eCraftType = EItemCraftType::ITEMCRAFTTYPE_None;
			lsItem.sAgeLevel = 0;
			lsItem.sMatureBar.sCur = 0;
			lsItem.sMatureBar.sMax = 0;
			lsItem.bMaturing = false;

			bAnyChanged = TRUE;
		}
        else
        {
			if ( !MIXHANDLER->SetClearMixStatus( &lsItem, lsItem.sMixUniqueID1 ) )
			{
				return FALSE;
			}
        }

    }
    else if ( lsItem.eCraftType == EItemCraftType::ITEMCRAFTTYPE_Aging )
    {
        while ( lsItem.sAgeLevel > 0 )
        {
            AGEHANDLER->OnDownAge ( &lsItem );
        }

        if ( isNonMixableNonAgable
			|| lsItem.sItemID.ToItemType() == ITEMTYPE_Boots
			|| lsItem.sItemID.ToItemType() == ITEMTYPE_Gauntlets
			|| lsItem.sItemID.ToItemType() == ITEMTYPE_Bracelets )
        {
			if ( isNonMixableNonAgable )
			{
				WARN ( "Item %s is aged. Removing aging", psItem->szItemName );

				psItem->eCraftType = EItemCraftType::ITEMCRAFTTYPE_None;
				psItem->sAgeLevel = 0;
				psItem->sMatureBar.sCur = 0;
				psItem->sMatureBar.sMax = 0;
				psItem->bMaturing = false;

				bAnyChanged = TRUE;
			}

            lsItem.eCraftType = EItemCraftType::ITEMCRAFTTYPE_None;
            lsItem.sAgeLevel = 0;
            lsItem.sMatureBar.sCur = 0;
            lsItem.sMatureBar.sMax = 0;
            lsItem.bMaturing = false;
        }
    }
	else if ( lsItem.eCraftType == EItemCraftType::ITEMCRAFTTYPE_QuestWeapon )
	{
		return FALSE;
	}
    else if ( lsItem.eCraftType != EItemCraftType::ITEMCRAFTTYPE_None )
    {
        //ignore everthing else
        return FALSE;
    }

	FixItemBasedOnRarity( &lsItem, lsItem.eRarity, TRUE ); //remove rarity boost

	lsItem.eRarity = EItemRarity::NONE;
	int iOrigRarity = (int)psItem->eRarity;

    DefinitionItem * defItemOld = ITEMSERVER->FindOldItemDefByCode ( lsItem.sItemID.ToItemID () );
    DefinitionItem * defItem = ITEMSERVER->FindItemDefByCode ( lsItem.sItemID.ToItemID () );

    if ( defItem )
    {
        if ( CheckAndAdjustInt ( lsItem.szItemName, lsItem.sItemID, "Weight",         &lsItem.iWeight                      ,defItem->sItem.iWeight ) )	      bAnyChanged = TRUE;
        if ( CheckAndAdjustInt ( lsItem.szItemName, lsItem.sItemID, "ATKRange",       &lsItem.iAttackRange                 ,defItem->sItem.iAttackRange ) )	  bAnyChanged = TRUE;
        if ( CheckAndAdjustInt ( lsItem.szItemName, lsItem.sItemID, "ATKSpeed",       &lsItem.iAttackSpeed                 ,defItem->sItem.iAttackSpeed ) )   bAnyChanged = TRUE;
        if ( CheckAndAdjustInt ( lsItem.szItemName, lsItem.sItemID, "PotionStorage",  &lsItem.iPotionStorage               ,defItem->sItem.iPotionStorage ) ) bAnyChanged = TRUE;

		if (ITEMSERVER->CheckAndAdjustItemRequirements ( &lsItem, defItem )) bAnyChanged = TRUE;

		switch ( psItem->sItemID.ToItemID() )
		{
			case EItemID::ITEMID_RebirthScroll:
			case EItemID::ITEMID_TeleportCore:
			case EItemID::ITEMID_HairPotionA:
			case EItemID::ITEMID_HairPotionB:
			case EItemID::ITEMID_HairPotionC:
			{
				if ( psItem->iSalePrice != 0 )
				{
					DEBUG("Set sale price to 0 from %d for: %s", psItem->iSalePrice, psItem->szItemName);

					psItem->iSalePrice = 0;
					ITEMSERVER->ReformItem( psItem );
				}
				break;
			}
			default:
			{
				//Check item buy price
				int itemBuyPrice = ITEMSERVER->GetItemBuyPrice ( lsItem.sItemID.ToItemID (), lsItem.eSpecialization > 0, (int)lsItem.eRarity );

				if ( itemBuyPrice != lsItem.iSalePrice )
				{
					if ( abs( itemBuyPrice - lsItem.iSalePrice ) > 1 )
					{
						INFO( "Sale price corrected for: %s (%d -> %d)", lsItem.szItemName, lsItem.iSalePrice, itemBuyPrice );
					}

					lsItem.iSalePrice = itemBuyPrice;
					bAnyChanged = TRUE;
				}
			}
		}

		if ( CheckAndAdjustInt   ( lsItem.szItemName, lsItem.sItemID, "ATKRating",    &lsItem.iAttackRating                ,defItem->AttackRatingMin        ,defItem->AttackRatingMax     ,defItemOld->AttackRatingMin      ,defItemOld->AttackRatingMax   ) ) bAnyChanged = TRUE;
		if ( CheckAndAdjustShort ( lsItem.szItemName, lsItem.sItemID, "ATKPowMin",    &lsItem.sDamage.sMin                 ,defItem->AttackPower1Min        ,defItem->AttackPower2Min     ,defItemOld->AttackPower1Min      ,defItemOld->AttackPower2Min   ) ) bAnyChanged = TRUE;
        if ( CheckAndAdjustShort ( lsItem.szItemName, lsItem.sItemID, "ATKPowMax",    &lsItem.sDamage.sMax                 ,defItem->AttackPower2Min        ,defItem->AttackPower2Max     ,defItemOld->AttackPower2Min      ,defItemOld->AttackPower2Max   ) ) bAnyChanged = TRUE;
        if ( CheckAndAdjustInt   ( lsItem.szItemName, lsItem.sItemID, "Defense",      &lsItem.iDefenseRating               ,defItem->DefenseMin             ,defItem->DefenseMax          ,defItemOld->DefenseMin           ,defItemOld->DefenseMax        ) ) bAnyChanged = TRUE;
        if ( CheckAndAdjustShort ( lsItem.szItemName, lsItem.sItemID, "Organic",      &lsItem.saElementalDef[0]            ,defItem->OrganicMin             ,defItem->OrganicMax          ,defItemOld->OrganicMin           ,defItemOld->OrganicMax        ) ) bAnyChanged = TRUE;
        if ( CheckAndAdjustShort ( lsItem.szItemName, lsItem.sItemID, "Fire",         &lsItem.saElementalDef[2]            ,defItem->FireMin                ,defItem->FireMax             ,defItemOld->FireMin              ,defItemOld->FireMax           ) ) bAnyChanged = TRUE;
        if ( CheckAndAdjustShort ( lsItem.szItemName, lsItem.sItemID, "Ice",          &lsItem.saElementalDef[3]            ,defItem->FrostMin               ,defItem->FrostMax            ,defItemOld->FrostMin             ,defItemOld->FrostMax          ) ) bAnyChanged = TRUE;
        if ( CheckAndAdjustShort ( lsItem.szItemName, lsItem.sItemID, "Lightning",    &lsItem.saElementalDef[4]            ,defItem->LightningMin           ,defItem->LightningMax        ,defItemOld->LightningMin         ,defItemOld->LightningMax      ) ) bAnyChanged = TRUE;
        if ( CheckAndAdjustShort ( lsItem.szItemName, lsItem.sItemID, "Poison",       &lsItem.saElementalDef[5]            ,defItem->PoisonMin              ,defItem->PoisonMax           ,defItemOld->PoisonMin            ,defItemOld->PoisonMax         ) ) bAnyChanged = TRUE;


		if ( CheckAndAdjustFloat ( lsItem.szItemName, lsItem.sItemID, "Absorb",       &lsItem.fAbsorbRating                ,static_cast<float>(defItem->AbsorbMin)              ,static_cast<float>(defItem->AbsorbMax)           ,static_cast<float>(defItemOld->AbsorbMin)            ,static_cast<float>(defItemOld->AbsorbMax)         ) ) bAnyChanged = TRUE;
		if ( CheckAndAdjustFloat ( lsItem.szItemName, lsItem.sItemID, "Block",        &lsItem.fBlockRating                 ,static_cast<float>(defItem->BlockRatingMin)         ,static_cast<float>(defItem->BlockRatingMax)      ,static_cast<float>(defItemOld->BlockRatingMin)       ,static_cast<float>(defItemOld->BlockRatingMax)    ) ) bAnyChanged = TRUE;
		if ( CheckAndAdjustFloat ( lsItem.szItemName, lsItem.sItemID, "Speed",        &lsItem.fMovementSpeed               ,static_cast<float>(defItem->RunSpeedMin)            ,static_cast<float>(defItem->RunSpeedMax)         ,static_cast<float>(defItemOld->RunSpeedMin)          ,static_cast<float>(defItemOld->RunSpeedMax)       ) ) bAnyChanged = TRUE;
		if ( CheckAndAdjustFloat ( lsItem.szItemName, lsItem.sItemID, "HPRegen",      &lsItem.fHPRegen                     ,static_cast<float>(defItem->HPRegenMin)             ,static_cast<float>(defItem->HPRegenMax)          ,static_cast<float>(defItemOld->HPRegenMin )          ,static_cast<float>(defItemOld->HPRegenMax)        ) ) bAnyChanged = TRUE;
		if ( CheckAndAdjustFloat ( lsItem.szItemName, lsItem.sItemID, "MPRegen",      &lsItem.fMPRegen                     ,static_cast<float>(defItem->MPRegenMin)             ,static_cast<float>(defItem->MPRegenMax)          ,static_cast<float>(defItemOld->MPRegenMin)           ,static_cast<float>(defItemOld->MPRegenMax)        ) ) bAnyChanged = TRUE;
		if ( CheckAndAdjustFloat ( lsItem.szItemName, lsItem.sItemID, "SPRegen",      &lsItem.fSPRegen                     ,static_cast<float>(defItem->STMRegenMin)            ,static_cast<float>(defItem->STMRegenMax)         ,static_cast<float>(defItemOld->STMRegenMin)          ,static_cast<float>(defItemOld->STMRegenMax)       ) ) bAnyChanged = TRUE;
		if ( CheckAndAdjustFloat ( lsItem.szItemName, lsItem.sItemID, "AddHP",        &lsItem.fAddHP                       ,static_cast<float>(defItem->AddHPMin)               ,static_cast<float>(defItem->AddHPMax)            ,static_cast<float>(defItemOld->AddHPMin)             ,static_cast<float>(defItemOld->AddHPMax)          ) ) bAnyChanged = TRUE;
		if ( CheckAndAdjustFloat ( lsItem.szItemName, lsItem.sItemID, "AddMP",        &lsItem.fAddMP                       ,static_cast<float>(defItem->AddMPMin)               ,static_cast<float>(defItem->AddMPMax)            ,static_cast<float>(defItemOld->AddMPMin)             ,static_cast<float>(defItemOld->AddMPMax)          ) ) bAnyChanged = TRUE;
		if ( CheckAndAdjustFloat ( lsItem.szItemName, lsItem.sItemID, "AddSTM",       &lsItem.fAddSP                       ,static_cast<float>(defItem->AddSTMMin)              ,static_cast<float>(defItem->AddSTMMax)           ,static_cast<float>(defItemOld->AddSTMMin)            ,static_cast<float>(defItemOld->AddSTMMax)         ) ) bAnyChanged = TRUE;


        if ( lsItem.eSpecialization > 0)
        {
            if ( CheckAndAdjustFloat ( lsItem.szItemName, lsItem.sItemID, "SpecSpeed",      &lsItem.sSpecData.fSpecMovementSpeed   ,defItem->AddSpecRunSpeedMin                  ,defItem->AddSpecRunSpeedMax    ,defItemOld->AddSpecRunSpeedMin                  ,defItemOld->AddSpecRunSpeedMax ) ) bAnyChanged = TRUE;
            if ( CheckAndAdjustFloat ( lsItem.szItemName, lsItem.sItemID, "SpecAbsorb",     &lsItem.sSpecData.fSpecAbsorbRating    ,defItem->AddSpecAbsorbMin                    ,defItem->AddSpecAbsorbMax      ,defItemOld->AddSpecAbsorbMin                    ,defItemOld->AddSpecAbsorbMax   ) ) bAnyChanged = TRUE;
            if ( CheckAndAdjustInt   ( lsItem.szItemName, lsItem.sItemID, "SpecDefense",    &lsItem.sSpecData.iSpecDefenseRating   ,defItem->AddSpecDefenseMin                   ,defItem->AddSpecDefenseMax     ,defItemOld->AddSpecDefenseMin                   ,defItemOld->AddSpecDefenseMax  ) ) bAnyChanged = TRUE;
            if ( CheckAndAdjustFloat ( lsItem.szItemName, lsItem.sItemID, "SpecHPRegen",    &lsItem.sSpecData.fSpecHPRegen         ,defItem->sItem.sSpecData.fSpecHPRegen                                                                                                                         ) ) bAnyChanged = TRUE;
            if ( CheckAndAdjustFloat ( lsItem.szItemName, lsItem.sItemID, "SpecSPRegen",    &lsItem.sSpecData.fSpecSPRegen         ,defItem->sItem.sSpecData.fSpecSPRegen                                                                                                                         ) ) bAnyChanged = TRUE;
            if ( CheckAndAdjustFloat ( lsItem.szItemName, lsItem.sItemID, "SpecMPRegen",    &lsItem.sSpecData.fSpecMPRegen         ,defItem->AddSpecMPRegenMin                    ,defItem->AddSpecMPRegenMax    ,defItemOld->AddSpecMPRegenMin                   ,defItemOld->AddSpecMPRegenMax  ) ) bAnyChanged = TRUE;
            if ( CheckAndAdjustFloat ( lsItem.szItemName, lsItem.sItemID, "SpecBlock",      &lsItem.sSpecData.fSpecBlockRating     ,defItem->sItem.sSpecData.fSpecBlockRating                                                                                                                     ) ) bAnyChanged = TRUE;
            if ( CheckAndAdjustInt   ( lsItem.szItemName, lsItem.sItemID, "SpecAtkSpd",     &lsItem.sSpecData.iSpecAttackSpeed     ,defItem->sItem.sSpecData.iSpecAttackSpeed                                                                                                                     ) ) bAnyChanged = TRUE;
            if ( CheckAndAdjustInt   ( lsItem.szItemName, lsItem.sItemID, "SpecCrit",       &lsItem.sSpecData.iSpecCritical        ,defItem->sItem.sSpecData.iSpecCritical                                                                                                                        ) ) bAnyChanged = TRUE;
        }

        if ( bAnyChanged )
        {
			FixItemBasedOnRarity( &lsItem, (EItemRarity)iOrigRarity, FALSE ); //re-add back in rarity boost

			lsItem.eRarity = (EItemRarity)iOrigRarity; //restore old rarity

			ITEMSERVER->UpgradeNewItem ( psItem, &lsItem );
			ITEMSERVER->ReformItem ( &lsItem );

			CopyMemory ( psItem, &lsItem, sizeof ( Item ) );

			return TRUE;
        }
    }


    return FALSE;
}

BOOL ServerCore::UpdateItemsMisc( Item * psItem )
{
	const EItemID itemID = psItem->sItemID.ToItemID();
	const EItemBase itemBase = psItem->sItemID.ToItemBase();
	const EItemType itemType = psItem->sItemID.ToItemType();
	//
	//
	//
	//if (psItem->eCraftType == EItemCraftType::ITEMCRAFTTYPE_Mixing)
	//{
	//	//legacy mixes. Cannot really determine what stats
	//	if (psItem->eMixTypeName == EMixTypeName::MIXTYPENAME_LegacyDoNotUse)
	//	{
	//		return FALSE;
	//	}
	//}
	//else if (psItem->eCraftType == EItemCraftType::ITEMCRAFTTYPE_QuestWeapon)
	//{
	//	return FALSE;
	//}
	//else if (psItem->eCraftType != EItemCraftType::ITEMCRAFTTYPE_None && psItem->eCraftType != EItemCraftType::ITEMCRAFTTYPE_Aging)
	//{
	//	//ignore everthing else
	//	return FALSE;
	//}
	//
	////if ( ( itemBase == EItemBase::ITEMBASE_Weapon || itemType == EItemType::ITEMTYPE_Armor || itemType == EItemType::ITEMTYPE_Robe
	////	|| itemType == EItemType::ITEMTYPE_Amulet || itemType == EItemType::ITEMTYPE_Shield || itemType == EItemType::ITEMTYPE_Boots
	////	|| itemType == EItemType::ITEMTYPE_Gauntlets || itemType == EItemType::ITEMTYPE_Bracelets
	////	|| itemType == EItemType::ITEMTYPE_Ring || itemType == EItemType::ITEMTYPE_Ring2 || itemType == EItemType::ITEMTYPE_Orb ))
	////{
	////	//npc shop items had rarity of 0
	////	if ( psItem->eRarity == 0 )
	////	{
	////		psItem->eRarity = EItemRarity::COMMON;
	////		FixItemBasedOnRarity( psItem, psItem->eRarity );
	////		ITEMSERVER->ReformItem( psItem );
	////
	////		INFO( "%sFixed rarity of NPC shop item: %s", ServerCore::GetCharOrAccountName(), psItem->szItemName );
	////	}
	////}
	//
	//
	//if ( psItem->sItemID.ToItemType() == ITEMTYPE_Costume
	//	|| psItem->sItemID.ToItemType() == ITEMTYPE_Costume2
	//	|| psItem->sItemID.ToItemType() == ITEMTYPE_Costume3
	//	|| psItem->sItemID.ToItemType() == ITEMTYPE_Costume4 )
	//{
	//	if ( psItem->fMovementSpeed != 1.0f )
	//	{
	//		INFO( "Set movement speed to 1.0 from %.1f for costume: %s", psItem->fMovementSpeed, psItem->szItemName );
	//
	//		psItem->fMovementSpeed = 1.0f;
	//		ITEMSERVER->ReformItem( psItem );
	//	}
	//
	//
	//	if ( psItem->eSpecialization != 0 &&
	//		 psItem->sSpecData.fSpecMovementSpeed != 3.0f )
	//	{
	//		INFO( "Set movement speed to 3.0 from %.1f for costume: %s", psItem->fMovementSpeed, psItem->szItemName );
	//
	//		psItem->sSpecData.fSpecMovementSpeed = 3.0f;
	//		ITEMSERVER->ReformItem( psItem );
	//	}
	//}

	//if ( ITEMSERVER->IsBossOrSpecialItem( psItem ) &&
	//	psItem->eRarity != EItemRarity::LEGENDARY )
	//{
	//	const int statsBoostMultiplier = psItem->eRarity - 1;
	//	const int itemLevel = psItem->iLevel;
	//
	//	int attackPowerBoost = 0;
	//	int rangeBoost = 0;
	//	int attackRatingBoost = 0;
	//	int criticalBoost = 0;
	//
	//	if (itemLevel >= 0  && itemLevel < 103)
	//	{
	//		attackPowerBoost = 3;
	//		rangeBoost = 3;
	//		attackRatingBoost = 10;
	//		criticalBoost = 1;
	//	}
	//	else if (itemLevel >= 103 && itemLevel < 113)
	//	{
	//		attackPowerBoost = 4;
	//		rangeBoost = 4;
	//		attackRatingBoost = 13;
	//		criticalBoost = 1;
	//	}
	//	else if (itemLevel >= 113)
	//	{
	//		attackPowerBoost = 5;
	//		rangeBoost = 5;
	//		attackRatingBoost = 20;
	//		criticalBoost = 1;
	//	}
	//
	//	STATUS( "%sSetting boss item to LEGENDARY status: %s", ServerCore::GetCharOrAccountName(), psItem->szItemName );
	//	switch ( psItem->sItemID.ToItemID() )
	//	{
	//		case EItemID::ITEMID_DraxosBoots:
	//		{
	//			psItem->iAgility -= (-3 * statsBoostMultiplier);
	//
	//			if (itemLevel >= 0 && itemLevel < 103)
	//			{
	//				psItem->fAbsorbRating -= static_cast<float>(0.7 * statsBoostMultiplier);
	//				psItem->iDefenseRating -= (7 * statsBoostMultiplier);
	//			}
	//			else if (itemLevel >= 103)
	//			{
	//				psItem->fAbsorbRating -= static_cast<float>(1.6 * statsBoostMultiplier);
	//				psItem->iDefenseRating -= (20 * statsBoostMultiplier);
	//			}
	//
	//			psItem->eRarity = EItemRarity::LEGENDARY;
	//
	//			ITEMSERVER->ReformItem( psItem );
	//		}
	//		break;
	//
	//		case EItemID::ITEMID_AbyssRing:
	//		{
	//			psItem->iSpirit -= (-3 * statsBoostMultiplier);
	//
	//			if (itemLevel >= 0 && itemLevel < 103)
	//			{
	//				psItem->fAddHP -= static_cast<float>(1 * statsBoostMultiplier);
	//
	//				psItem->fAddSP -= static_cast<float>(2 * statsBoostMultiplier);
	//			}
	//			else if (itemLevel >= 103)
	//			{
	//				psItem->fAddHP -= static_cast<float>(3 * statsBoostMultiplier);
	//
	//				psItem->fAddSP -= static_cast<float>(4 * statsBoostMultiplier);
	//			}
	//
	//			psItem->eRarity = EItemRarity::LEGENDARY;
	//
	//			ITEMSERVER->ReformItem( psItem );
	//		}
	//		break;
	//
	//		default:
	//		{
	//			LOGERROR( "UpdateItemsMisc - Not handled!" );
	//		}
	//	}
	//}
	return TRUE;
}

BOOL ServerCore::UpdateItemsToUncommon(Item* psItem)
{

	return FALSE;

	if ( psItem->eRarity != EItemRarity::NONE )
	{
		//WARN( "Item %s already has rarity set: %d", psItem->szItemName, psItem->eRarity );
		return FALSE;
	}

	const EItemID itemID = psItem->sItemID.ToItemID();
	const EItemBase itemBase = psItem->sItemID.ToItemBase();
	const EItemType itemType = psItem->sItemID.ToItemType();

	if (itemBase != EItemBase::ITEMBASE_Weapon
		&& itemType != EItemType::ITEMTYPE_Armor
		&& itemType != EItemType::ITEMTYPE_Robe
		&& itemType != EItemType::ITEMTYPE_Amulet
		&& itemType != EItemType::ITEMTYPE_Shield
		&& itemType != EItemType::ITEMTYPE_Boots
		&& itemType != EItemType::ITEMTYPE_Gauntlets
		&& itemType != EItemType::ITEMTYPE_Bracelets
		&& itemType != EItemType::ITEMTYPE_Ring
		&& itemType != EItemType::ITEMTYPE_Ring2
		&& itemType != EItemType::ITEMTYPE_Orb
		&& !ITEMSERVER->IsBossOrSpecialItem(itemID))
	{
		return FALSE;
	}

	Item lsItem;
	CopyMemory(&lsItem, psItem, sizeof(Item));

	if (lsItem.eCraftType == EItemCraftType::ITEMCRAFTTYPE_Mixing)
	{
		//legacy mixes. Cannot really determine what stats
		if (lsItem.eMixTypeName == EMixTypeName::MIXTYPENAME_LegacyDoNotUse)
		{
			return FALSE;
		}
		else
		{
			if ( !MIXHANDLER->SetClearMixStatus( &lsItem, lsItem.sMixUniqueID1 ) )
			{
				return FALSE;
			}
		}

	}
	else if (lsItem.eCraftType == EItemCraftType::ITEMCRAFTTYPE_Aging)
	{
		while (lsItem.sAgeLevel > 0)
		{
			AGEHANDLER->OnDownAge(&lsItem);
		}

		if (lsItem.sItemID.ToItemType() == ITEMTYPE_Boots || lsItem.sItemID.ToItemType() == ITEMTYPE_Gauntlets || lsItem.sItemID.ToItemType() == ITEMTYPE_Bracelets)
		{
			lsItem.eCraftType = EItemCraftType::ITEMCRAFTTYPE_None;
			lsItem.sAgeLevel = 0;
			lsItem.sMatureBar.sCur = 0;
			lsItem.sMatureBar.sMax = 0;
			lsItem.bMaturing = false;
		}
	}
	else if (lsItem.eCraftType == EItemCraftType::ITEMCRAFTTYPE_QuestWeapon)
	{
		return FALSE;
	}
	else if (lsItem.eCraftType != EItemCraftType::ITEMCRAFTTYPE_None)
	{
		//ignore everthing else
		return FALSE;
	}

	lsItem.eRarity = EItemRarity::UNCOMMON;
	FixItemBasedOnRarity(&lsItem, lsItem.eRarity);

	ITEMSERVER->UpgradeNewItem(psItem, &lsItem);
	ITEMSERVER->ReformItem(&lsItem);

	CopyMemory(psItem, &lsItem, sizeof(Item));

	return TRUE;
}

void ServerCore::UpdateCharacterItemsLevelData( const char * pszFilePath, const char * pszAccountName, const char* pszCharName, BOOL bUpgradeItems, BOOL bDumpToSQL )
{
	if ( pszAccountName )
		sCurrAccountName = pszAccountName;
	else
		sCurrAccountName = "";

	if ( pszCharName[0] )
		sCurrCharName = pszCharName;
	else
		sCurrCharName = "";

	static PacketCharacterRecordData sCharacterRecord;
	BOOL bRead = FALSE;

	FILE * fp = NULL;
	int iSizeFile = 0;
	fopen_s( &fp, pszFilePath, "rb" );
	if ( fp )
	{
		fseek( fp, 0, SEEK_END );
		iSizeFile = ftell( fp );

		fseek( fp, 0, SEEK_SET );
		fread( &sCharacterRecord, iSizeFile, 1, fp );
		fclose( fp );

		bRead = TRUE;
	}

	int iDataSizeDifference = 0;

	if ( bRead )
	{
		if (sCharacterRecord.sCharacterData.iLevel >= 1 &&
			sCharacterRecord.sCharacterData.iLevel <= 150 &&
			sCharacterRecord.sCharacterData.iType == ECharacterType::CHARACTERTYPE_Player)
		{
			//CharacterSave * baseAddress1 = *(WORD*)(( &sCharacterRecord1.sCharacterSaveData.wForceOrbUsing ) + 16);

			DWORD pp = (DWORD)&sCharacterRecord.sCharacterSaveData;

			BYTE* pbaData = sCharacterRecord.baData;
			std::vector<RecordItem*> vRecordItem;

			SQLConnection* cDB;

			//DEBUG( "Character: %s, Account: %s, Path: %s", pszCharName, pszAccountName, pszFilePath );

			cDB = SQLCONNECTION(DATABASEID_ItemDB);

			//INFO ( "Item count before: %d", sCharacterRecord.iItemCount );

			if (cDB->Open())
			{
				if ( bDumpToSQL )
				{
					if ( cDB->Prepare( "INSERT INTO [dbo].[GoldDump]([AccountName],[CharName],[WarehouseGold],[CharacterGold]) VALUES (?,?,NULL,?)"))
					{
						cDB->BindParameterInput( 1, EDatabaseDataType::PARAMTYPE_String, (char*)pszAccountName, 32 );
						cDB->BindParameterInput( 2, EDatabaseDataType::PARAMTYPE_String, (char*)pszCharName, 32 );
						cDB->BindParameterInput( 3, EDatabaseDataType::PARAMTYPE_Integer, &sCharacterRecord.sCharacterData.iGold );

						cDB->Execute();
					}
					cDB->Clear();
				}

				BOOL bAnyChanged = FALSE;

				for (int i = 0; i < sCharacterRecord.iItemCount; i++)
				{
					//Inventory Limit...
					if ( i >= 200 )
					{
						WARN ( "Inventory limit reached!" );
						break;
					}

					RecordItem* ps = new RecordItem;
					ITEMSERVER->DecodeItemsData(pbaData, ps, 0x7FFFF);

					//skip upgrading of union cores, as they tend to get corrupted somehow
					if ( ps->sItem.sItemID.ToItemID() != EItemID::ITEMID_UnionCore )
					{
						if ( ps->sItem.iChk1 != 0 && ps->sItem.iChk2 != 0 )
						{
							if ( ITEMSERVER->CheckItemForm( &ps->sItem ) )
							{
								//DEBUG ( "IS VALID = %s", ps->sItem.szItemName );

								if ( bUpgradeItems )
								{
									UpdateItemsMisc( &ps->sItem );
									CheckAndUpdateStats( &ps->sItem );
								}

								if ( bDumpToSQL )
								{
									DumpItemDataToSQL( &ps->sItem, cDB, TRUE, pszAccountName, pszCharName );

									if ( bUpgradeItems )
									{
										ITEMSERVER->OnSaveItemData( &ps->sItem );
									}
								}
							}
							else
							{
								WARN( "Item form is incorrect for item: %s", ps->sItem.szItemName );
								ps->sItem.iChk1 = 0;
								ps->sItem.iChk2 = 0;
							}
						}
						else if ( ps->sItem.iChk1 != 0 && ps->sItem.iChk2 == 0 )
						{
							WARN( "Item has header but zero checksum: %s", ps->sItem.szItemName );
							ps->sItem.iChk1 = 0;
							ps->sItem.iChk2 = 0;
						}
						else if ( ps->sItem.iChk1 == 0 && ps->sItem.iChk2 != 0 )
						{
							WARN( "Item has zero header but non-zero checksum: %s", ps->sItem.szItemName );
							ps->sItem.iChk1 = 0;
							ps->sItem.iChk2 = 0;
						}
						else if ( ps->sItem.iChk1 == 0 && ps->sItem.iChk2 == 0 )
						{
							//WARN ( "Item has has zero checksum and header: ", ps->sItem.szItemName );
						}
					}

					vRecordItem.push_back ( ps );

					//Next Item
					pbaData += ((int*)pbaData)[0];
				}

				cDB->Close();
			}

			//INFO ( "sCharacterRecord.iDataSize before: %d", sCharacterRecord.iDataSize );

			if ( bUpgradeItems )
			{
				int iDataSizeBefore = sCharacterRecord.iDataSize;

				pbaData = sCharacterRecord.baData;
				sCharacterRecord.iItemCount = 0;
				sCharacterRecord.iDataSize = 0;

				//Write Items
				for ( std::vector<RecordItem *>::iterator it = vRecordItem.begin(); it != vRecordItem.end(); it++ )
				{
					RecordItem * ps = ( *it );

					//Encode
					int iSize = ITEMSERVER->EncodeItemsData( ps, pbaData, sizeof( RecordItem ) );

					pbaData += iSize;
					sCharacterRecord.iDataSize += iSize;
					sCharacterRecord.iItemCount++;

					delete ps;
				}

				int iDataSizeAfter = sCharacterRecord.iDataSize;
				iDataSizeDifference = iDataSizeAfter - iDataSizeBefore;

				//INFO ( "sCharacterRecord.iDataSize after: %d", sCharacterRecord.iDataSize );
				//INFO ( "Item count after: %d", sCharacterRecord.iItemCount );
			}

			vRecordItem.clear();
		}
		else
		{
			if ( pszFilePath )
			{
				WARN ( "Char file is corrupted. Deleting..: %s", pszFilePath );
				DeleteFileA ( pszFilePath );
			}

			bRead = FALSE;
		}
	}

	if ( bRead && bUpgradeItems )
	{
		FILE * fp = NULL;
		fopen_s( &fp, pszFilePath, "wb" );
		if ( fp )
		{
			sCharacterRecord.iLength += iDataSizeDifference;

			int fileSize = sCharacterRecord.iLength;
			if ( fileSize < srRECORD_DEFAULT_SIZE )
				fileSize = srRECORD_DEFAULT_SIZE;

			fseek( fp, 0, SEEK_SET );
			fwrite( &sCharacterRecord, fileSize, 1, fp );
			fclose( fp );
		}
	}
}

void ServerCore::UpdateCharacterItemsLevelWarehouse( const char * pszFilePath, const char* pszAccountName, BOOL bUpgradeItems, BOOL bDumpToSQL)
{
	sCurrAccountName = pszAccountName;
	sCurrCharName = "";

	BOOL bForceUpdate = FALSE;

	CWarehouseBase * pcWarehouse = WAREHOUSEHANDLER->Open( pszFilePath );
	if ( WAREHOUSEHANDLER->HaveFile( pcWarehouse ) )
	{
		if ( WAREHOUSEHANDLER->Decrypt( pcWarehouse ) )
		{
			//For testing purposes
			//WAREHOUSEHANDLER->CheckForInvalidItems( NULL, pcWarehouse );
			//WAREHOUSEHANDLER->CheckForDupedWarehouseItems( NULL, pcWarehouse );

			SQLConnection* cDB;
			cDB = SQLCONNECTION(DATABASEID_ItemDB);

			std::string sAccountName = pszAccountName;

			//use this code to force update gold in a player's warehouse
			//if ( STRINGCOMPAREI( pszAccountName, "example_account_name" )
			//{
			//	pcWarehouse->psWarehouse->iGold = 10'000'000 + 2023; //2023 is needed here
			//	pcWarehouse->iGold = 10'000'000;
			//	bForceUpdate = TRUE;
			//}


			if ( bDumpToSQL )
			{
				if ( cDB->Open() )
				{
					if ( cDB->Prepare( "INSERT INTO [dbo].[GoldDump]([AccountName],[CharName],[WarehouseGold],[CharacterGold]) VALUES (?,NULL,?,NULL)" ) )
					{
						cDB->BindParameterInput( 1, EDatabaseDataType::PARAMTYPE_String, (char *)pszAccountName, 32 );
						cDB->BindParameterInput( 2, EDatabaseDataType::PARAMTYPE_Integer, &pcWarehouse->iGold );

						cDB->Execute();
					}
					cDB->Close();
				}
			}

			if (cDB->Open())
			{
				for (int i = 0; i < 405; i++)
				{
					ItemData & pc = pcWarehouse->psWarehouse->saItem[i];

					//skip union cores as they tend to get corrupted during the upgrade
					//for some reason..
					if (pc.bValid &&
						pc.sItem.sItemID.ToItemID() != EItemID::ITEMID_UnionCore )
					{
						if (ITEMSERVER->CheckItemForm(&pc.sItem) == FALSE)
						{
							if ( pszAccountName )
							{
								WARN ( "WH Item not valid. Setting bValid to 0: %s : %s", pszAccountName, pc.sItem.szItemName );
								pc.sItem.iChk1 = 0;
								pc.bValid = 0;
							}
						}
						else
						{
							if ( bUpgradeItems )
							{
								UpdateItemsMisc( &pc.sItem );
								CheckAndUpdateStats ( &pc.sItem );
							}

							if (bDumpToSQL)
							{
								DumpItemDataToSQL(&pc.sItem, cDB, TRUE, pszAccountName, NULL);

								if ( bUpgradeItems )
								{
									ITEMSERVER->OnSaveItemData(&pc.sItem);
								}
							}
						}
					}
				}

				cDB->Close();
			}

			if ( (bUpgradeItems ||  bForceUpdate) )
			{
				WAREHOUSEHANDLER->Encrypt( pcWarehouse );
				WAREHOUSEHANDLER->Save( pcWarehouse );
			}
		}
	}

	WAREHOUSEHANDLER->Close( pcWarehouse );
}

void ServerCore::UpdateCharacterItemsLevel( const char * pszBaseFolder, const char * pszName )
{
	char szPath[MAX_PATH] = { 0 };

	SQLCharacter s;
	if (CHARACTERSERVER->SQLGetCharacter( SQLCONNECTION ( DATABASEID_UserDB_LocalServer_CharInfo ), (char*)pszName, &s))
	{
		STRINGFORMAT(szPath, "%s\\Character\\%s.chr", pszBaseFolder, pszName);
		ServerCore::UpdateCharacterItemsLevelData(szPath, (char*)s.szAccountName, (char*)s.szCharacterName, TRUE, TRUE);

		STRINGFORMAT(szPath, "%s\\CharacterBackup\\%s.chr", pszBaseFolder, pszName);
		ServerCore::UpdateCharacterItemsLevelData(szPath, NULL, NULL, TRUE, FALSE);

		STRINGFORMAT(szPath, "%s\\CharacterDelete\\%s.chr", pszBaseFolder, pszName);
		ServerCore::UpdateCharacterItemsLevelData(szPath, NULL, NULL, TRUE, FALSE);

		STRINGFORMAT( szPath, "%s\\Warehouse\\%s.dat", pszBaseFolder, s.szAccountName );
		ServerCore::UpdateCharacterItemsLevelWarehouse( szPath, (char*)s.szAccountName, TRUE, TRUE);

		STRINGFORMAT( szPath, "%s\\WarehouseSeasonal\\%s.dat", pszBaseFolder, s.szAccountName );
		ServerCore::UpdateCharacterItemsLevelWarehouse( szPath, (char*)s.szAccountName, TRUE, TRUE);

		STRINGFORMAT( szPath, "%s\\WarehouseBackup\\%s.dat", pszBaseFolder, s.szAccountName );
		ServerCore::UpdateCharacterItemsLevelWarehouse( szPath, NULL, TRUE, FALSE);

		STRINGFORMAT( szPath, "%s\\WarehouseSeasonalBackup\\%s.dat", pszBaseFolder, s.szAccountName );
		ServerCore::UpdateCharacterItemsLevelWarehouse( szPath, NULL, TRUE, FALSE);
	}
}

void ServerCore::WarehouseTest()
{
	//Warehouse
	{
		char* pszFolder = "Data\\Warehouse";

		WIN32_FIND_DATAA sFindData;
		char szFileName[MAX_PATH] = { 0 };
		HANDLE hFile = NULL;
		StringCchPrintfA(szFileName, MAX_PATH, "%s\\*.*", pszFolder, szFileName);

		char szPath[512] = { 0 };

		if (hFile = FindFirstFileA(szFileName, &sFindData))
		{
			do
			{
				if (sFindData.cFileName == "." || sFindData.cFileName == "..")
					continue;

				std::string fn = sFindData.cFileName;
				std::string fileNameWithoutExt = fn.substr(0, fn.find_last_of("."));

				StringCchPrintfA(szPath, 512, "%s\\%s", pszFolder, sFindData.cFileName);
				ServerCore::UpdateCharacterItemsLevelWarehouse(szPath, fileNameWithoutExt.c_str(), TRUE, TRUE);
			}
			while (FindNextFileA(hFile, &sFindData));

			FindClose(hFile);
		}
	}
}

void ServerCore::UpdateCharacterLevelAll (  )
{
	INFO ( "Syncing Character level and experience to CharacterInfo.." );

	char * pszFolder = "Data\\Character";

	WIN32_FIND_DATAA sFindData;
	char szFileName[MAX_PATH] = { 0 };
	HANDLE hFile = NULL;
	StringCchPrintfA ( szFileName, MAX_PATH, "%s\\*.*", pszFolder );

	char szPath[512] = { 0 };

	static PacketCharacterRecordData sCharacterRecord;

	if ( hFile = FindFirstFileA ( szFileName, &sFindData ) )
	{
		do
		{
			std::string fn = sFindData.cFileName;

			if ( fn == "." || fn == ".." )
				continue;

			std::string fileNameWithoutExt = fn.substr ( 0, fn.find_last_of ( "." ) );

			SQLCharacter s;
			if ( CHARACTERSERVER->SQLGetCharacter ( SQLCONNECTION ( DATABASEID_UserDB_LocalServer_CharInfo ), (char *)fileNameWithoutExt.c_str (), &s ) )
			{
				StringCchPrintfA ( szPath, 512, "%s\\%s", pszFolder, sFindData.cFileName );

				BOOL bRead = FALSE;

				FILE * fp = NULL;
				int iSizeFile = 0;
				fopen_s ( &fp, szPath, "rb" );
				if ( fp )
				{
					fseek ( fp, 0, SEEK_END );
					iSizeFile = ftell ( fp );

					fseek ( fp, 0, SEEK_SET );
					fread ( &sCharacterRecord, iSizeFile, 1, fp );
					fclose ( fp );

					bRead = TRUE;
				}

				if ( bRead )
				{
					if ( sCharacterRecord.sCharacterData.iLevel >= 1 &&
						 sCharacterRecord.sCharacterData.iLevel <= 150 &&
						 sCharacterRecord.sCharacterData.iType == ECharacterType::CHARACTERTYPE_Player )
					{
						UINT iLow = sCharacterRecord.sCharacterData.iCurrentExpLow;
						UINT iHigh = sCharacterRecord.sCharacterData.iCurrentExpHigh;

						INT64 iExp = ( (INT64)iHigh << 32 ) | ( (INT64)iLow & 0xFFFFFFFF );

						SQLConnection * pcDB = SQLCONNECTION ( DATABASEID_UserDB_LocalServer_CharInfo );
						if ( pcDB->Open () )
						{
							if ( pcDB->Prepare ( "UPDATE CharacterInfo SET Experience=? WHERE ID=?" ) )
							{
								pcDB->BindParameterInput ( 1, EDatabaseDataType::PARAMTYPE_Int64, &iExp );
								pcDB->BindParameterInput ( 2, EDatabaseDataType::PARAMTYPE_Integer, &s.iID );

								pcDB->ExecuteUpdate ();
							}

							pcDB->Close ();
						}

						if ( SERVER_IS_SEASONAL )
						{
							SQLConnection * pcDB = SQLCONNECTION ( DATABASEID_UserDB_PrimaryServerSyncer );
							if ( pcDB->Open () )
							{
								if ( pcDB->Prepare ( "UPDATE CharacterInfo SET Experience=? WHERE ID=?" ) )
								{
									pcDB->BindParameterInput ( 1, EDatabaseDataType::PARAMTYPE_Int64, &iExp );
									pcDB->BindParameterInput ( 2, EDatabaseDataType::PARAMTYPE_Integer, &s.iID );

									pcDB->ExecuteUpdate ();
								}

								pcDB->Close ();
							}
						}
					}
				}
			}
			else
			{
				WARN ( "Character not found in CharacterInfo table: %s", fileNameWithoutExt.c_str () );
			}
		}
		while ( FindNextFileA ( hFile, &sFindData ) );

		FindClose ( hFile );
	}
}

void ServerCore::UpdateCharacterItemsLevelAll( BOOL bSkipBackups )
{
	//Character
	{
		INFO ( "Upgrading Character.." );

		char * pszFolder = "Data\\Character";

		WIN32_FIND_DATAA sFindData;
		char szFileName[MAX_PATH] = { 0 };
		HANDLE hFile = NULL;
		StringCchPrintfA( szFileName, MAX_PATH, "%s\\*.*", pszFolder );

		char szPath[512] = { 0 };

		if ( hFile = FindFirstFileA( szFileName, &sFindData ) )
		{
			do
			{
				std::string fn = sFindData.cFileName;

				if (fn == "." || fn == "..")
					continue;

				std::string fileNameWithoutExt = fn.substr(0, fn.find_last_of("."));

				SQLCharacter s;
				if ( CHARACTERSERVER->SQLGetCharacter( SQLCONNECTION ( DATABASEID_UserDB_LocalServer_CharInfo ), (char*)fileNameWithoutExt.c_str(), &s))
				{
					StringCchPrintfA(szPath, 512, "%s\\%s", pszFolder, sFindData.cFileName);
					ServerCore::UpdateCharacterItemsLevelData(szPath, (char*)s.szAccountName, (char*)s.szCharacterName, TRUE, FALSE);
				}
				else
				{
					StringCchPrintfA(szPath, 512, "%s\\%s", pszFolder, sFindData.cFileName);
					ServerCore::UpdateCharacterItemsLevelData(szPath, NULL, (char*)fileNameWithoutExt.c_str(), TRUE, FALSE);
				}
			}
			while ( FindNextFileA( hFile, &sFindData ) );

			FindClose( hFile );
		}
	}
	//CharacterBackup

	if (bSkipBackups == FALSE)
	{
		INFO ( "Upgrading CharacterBackup.." );

		char * pszFolder = "Data\\CharacterBackup";

		WIN32_FIND_DATAA sFindData;
		char szFileName[MAX_PATH] = { 0 };
		HANDLE hFile = NULL;
		StringCchPrintfA( szFileName, MAX_PATH, "%s\\*.*", pszFolder );

		char szPath[512] = { 0 };

		if ( hFile = FindFirstFileA( szFileName, &sFindData ) )
		{
			do
			{
				if (sFindData.cFileName == "." || sFindData.cFileName == "..")
					continue;

				std::string fn = sFindData.cFileName;

				std::string fileNameWithoutExt = fn.substr(0, fn.find_last_of("."));
				StringCchPrintfA( szPath, 512, "%s\\%s", pszFolder, sFindData.cFileName );
				//ServerCore::UpdateCharacterItemsLevelData( szPath, NULL, NULL, FALSE);
			}
			while ( FindNextFileA( hFile, &sFindData ) );

			FindClose( hFile );
		}
	}



	//CharacterDelete
	if (bSkipBackups == FALSE)
	{
		INFO ( "Upgrading CharacterDelete.." );

		char * pszFolder = "Data\\CharacterDelete";

		WIN32_FIND_DATAA sFindData;
		char szFileName[MAX_PATH] = { 0 };
		HANDLE hFile = NULL;
		StringCchPrintfA( szFileName, MAX_PATH, "%s\\*.*", pszFolder );

		char szPath[512] = { 0 };

		if ( hFile = FindFirstFileA( szFileName, &sFindData ) )
		{
			do
			{
				if (sFindData.cFileName == "." || sFindData.cFileName == "..")
					continue;

				StringCchPrintfA( szPath, 512, "%s\\%s", pszFolder, sFindData.cFileName );
				//ServerCore::UpdateCharacterItemsLevelData( szPath, NULL, NULL, FALSE);
			}
			while ( FindNextFileA( hFile, &sFindData ) );

			FindClose( hFile );
		}
	}

	//Warehouse
	{
		INFO ( "Upgrading Warehouse.." );

		char * pszFolder = "Data\\Warehouse";

		WIN32_FIND_DATAA sFindData;
		char szFileName[MAX_PATH] = { 0 };
		HANDLE hFile = NULL;
		StringCchPrintfA( szFileName, MAX_PATH, "%s\\*.*", pszFolder, szFileName);

		char szPath[512] = { 0 };

		if ( hFile = FindFirstFileA( szFileName, &sFindData ) )
		{
			do
			{
				if (sFindData.cFileName == "." || sFindData.cFileName == "..")
					continue;

				std::string fn = sFindData.cFileName;
				std::string fileNameWithoutExt = fn.substr(0, fn.find_last_of("."));

				StringCchPrintfA( szPath, 512, "%s\\%s", pszFolder, sFindData.cFileName );
				ServerCore::UpdateCharacterItemsLevelWarehouse( szPath, fileNameWithoutExt.c_str(), TRUE, FALSE);
			}
			while ( FindNextFileA( hFile, &sFindData ) );

			FindClose( hFile );
		}
	}
	//WarehouseSeasonal
	{
		INFO ( "Upgrading Warehouse Seasonal.." );

		char * pszFolder = "Data\\WarehouseSeasonal";

		WIN32_FIND_DATAA sFindData;
		char szFileName[MAX_PATH] = { 0 };
		HANDLE hFile = NULL;
		StringCchPrintfA( szFileName, MAX_PATH, "%s\\*.*", pszFolder, szFileName);

		char szPath[512] = { 0 };

		if ( hFile = FindFirstFileA( szFileName, &sFindData ) )
		{
			do
			{
				if (sFindData.cFileName == "." || sFindData.cFileName == "..")
					continue;

				std::string fn = sFindData.cFileName;
				std::string fileNameWithoutExt = fn.substr(0, fn.find_last_of("."));

				StringCchPrintfA( szPath, 512, "%s\\%s", pszFolder, sFindData.cFileName );
				ServerCore::UpdateCharacterItemsLevelWarehouse( szPath, fileNameWithoutExt.c_str(), TRUE, FALSE);
			}
			while ( FindNextFileA( hFile, &sFindData ) );

			FindClose( hFile );
		}
	}

	//WarehouseBackup
	if (bSkipBackups == FALSE)
	{
		INFO ( "Upgrading WarehouseBackup.." );

		char * pszFolder = "Data\\WarehouseBackup";

		WIN32_FIND_DATAA sFindData;
		char szFileName[MAX_PATH] = { 0 };
		HANDLE hFile = NULL;
		StringCchPrintfA( szFileName, MAX_PATH, "%s\\*.*", pszFolder );

		char szPath[512] = { 0 };

		if ( hFile = FindFirstFileA( szFileName, &sFindData ) )
		{
			do
			{
				if (sFindData.cFileName == "." || sFindData.cFileName == "..")
					continue;

				StringCchPrintfA( szPath, 512, "%s\\%s", pszFolder, sFindData.cFileName );
				//ServerCore::UpdateCharacterItemsLevelWarehouse( szPath, NULL, FALSE);
			}
			while ( FindNextFileA( hFile, &sFindData ) );

			FindClose( hFile );
		}
	}
}

/// <summary>
///
/// </summary>
/// <param name="item"></param>
/// <param name="itemRarity"></param>
/// <param name="bReverse">If TRUE, reverse direction of the 'upgrade'</param>
void ServerCore::FixItemBasedOnRarity(Item* item, const EItemRarity& itemRarity, BOOL bReverse )
{
	if (itemRarity == EItemRarity::NONE)
	{
		return;
	}

	//boss and special items does not have any stats change, but
	//displays legendary
	if ( ITEMSERVER->IsBossOrSpecialItem( item ) )
	{
		//boss and special items always have legendary status
		item->eRarity = EItemRarity::LEGENDARY;
		return;
	}

	const EItemBase itemBase = item->sItemID.ToItemBase();
	const EItemType itemType = item->sItemID.ToItemType();
	int statsBoostMultiplier = itemRarity - 1;
	const int itemLevel = item->iLevel;

	int attackPowerBoost = 0;
	int rangeBoost = 0;
	int attackRatingBoost = 0;
	int criticalBoost = 0;

	if (itemLevel >= 0  && itemLevel < 103)
	{
		attackPowerBoost = 3;
		rangeBoost = 3;
		attackRatingBoost = 10;
		criticalBoost = 1;
	}
	else if (itemLevel >= 103 && itemLevel < 113)
	{
		attackPowerBoost = 4;
		rangeBoost = 4;
		attackRatingBoost = 13;
		criticalBoost = 1;
	}
	else if (itemLevel >= 113)
	{
		attackPowerBoost = 5;
		rangeBoost = 5;
		attackRatingBoost = 20;
		criticalBoost = 1;
	}

	if ( bReverse )
	{
		item->iSalePrice = ( item->iSalePrice * 2 ) / ( statsBoostMultiplier + 1 );
		statsBoostMultiplier *= -1; //go the opposite direction for stats
	}
	else
	{
		item->iSalePrice = ( item->iSalePrice / 2 ) * ( statsBoostMultiplier + 1 );
	}


	if (itemBase == EItemBase::ITEMBASE_Weapon)
	{
		item->sDamage.sMin += attackPowerBoost * statsBoostMultiplier;
		item->sDamage.sMax += attackPowerBoost * statsBoostMultiplier;

		item->iAttackRange += rangeBoost * statsBoostMultiplier;

		item->iAttackRating += attackRatingBoost * statsBoostMultiplier;
		item->iCritical += criticalBoost * statsBoostMultiplier;
	}

	switch (itemType)
	{
		case EItemType::ITEMTYPE_Armor:
		SetRarityBoostArmors(item, statsBoostMultiplier, itemLevel);
		break;
		case EItemType::ITEMTYPE_Boots:
		SetRarityBoostBoots(item, statsBoostMultiplier, itemLevel);
		break;
		case EItemType::ITEMTYPE_Robe:
		SetRarityBoostRobes(item, statsBoostMultiplier, itemLevel);
		break;
		case EItemType::ITEMTYPE_Gauntlets:
		SetRarityBoostGauntlets(item, statsBoostMultiplier, itemLevel);
		break;
		case EItemType::ITEMTYPE_Bracelets:
		SetRarityBoostBracelets(item, statsBoostMultiplier, itemLevel);
		break;
		case EItemType::ITEMTYPE_Amulet:
		SetRarityBoostAmulets(item, statsBoostMultiplier, itemLevel);
		break;
		case EItemType::ITEMTYPE_Ring:
		SetRarityBoostRings(item, statsBoostMultiplier, itemLevel);
		break;
		case EItemType::ITEMTYPE_Shield:
		SetRarityBoostShields(item, statsBoostMultiplier, itemLevel);
		break;
		case EItemType::ITEMTYPE_Orb:
		SetRarityBoostOrbs(item, statsBoostMultiplier, itemLevel);
		break;
	}
}

void ServerCore::SetRarityBoostArmors(Item* item, const int statsBoostMultiplier, const int itemLevel)
{
	item->iStrength += (-3 * statsBoostMultiplier);

	if (itemLevel >= 0 && itemLevel < 103)
	{
		item->fAbsorbRating += static_cast<float>(1.4 * statsBoostMultiplier);

		item->iDefenseRating += (15 * statsBoostMultiplier);
	}
	else if (itemLevel >= 103)
	{
		item->fAbsorbRating += static_cast<float>(4 * statsBoostMultiplier);

		item->iDefenseRating += (25 * statsBoostMultiplier);
	}
}

void ServerCore::SetRarityBoostBoots(Item* item, const int statsBoostMultiplier, const int itemLevel)
{
	item->iAgility += (-3 * statsBoostMultiplier);

	if (itemLevel >= 0 && itemLevel < 103)
	{
		item->fAbsorbRating += static_cast<float>(0.7 * statsBoostMultiplier);

		item->iDefenseRating += (7 * statsBoostMultiplier);
	}
	else if (itemLevel >= 103)
	{
		item->fAbsorbRating += static_cast<float>(1.6 * statsBoostMultiplier);

		item->iDefenseRating += (20 * statsBoostMultiplier);
	}
}

void ServerCore::SetRarityBoostRobes(Item* item, const int statsBoostMultiplier, const int itemLevel)
{
	if (itemLevel >= 0 && itemLevel < 103)
	{
		item->fAbsorbRating += static_cast<float>(1.4 * statsBoostMultiplier);

		item->iDefenseRating += (15 * statsBoostMultiplier);
	}
	else if (itemLevel >= 103)
	{
		item->fAbsorbRating += static_cast<float>(4 * statsBoostMultiplier);

		item->iDefenseRating += (25 * statsBoostMultiplier);
	}
}

void ServerCore::SetRarityBoostGauntlets(Item* item, const int statsBoostMultiplier, const int itemLevel)
{
	if (itemLevel >= 0 && itemLevel  < 103)
	{
		item->fAbsorbRating += static_cast<float>(0.7 * statsBoostMultiplier);

		item->iDefenseRating += (7 * statsBoostMultiplier);
	}
	else if (itemLevel >= 103)
	{
		item->fAbsorbRating += static_cast<float>(1.6 * statsBoostMultiplier);

		item->iDefenseRating += (20 * statsBoostMultiplier);
	}
}

void ServerCore::SetRarityBoostBracelets(Item* item, const int statsBoostMultiplier, const int itemLevel)
{
	if (itemLevel >= 0 && itemLevel  < 103)
	{
		item->iAttackRating += (5 * statsBoostMultiplier);

		item->iDefenseRating += (6 * statsBoostMultiplier);

		item->iPotionStorage += (3 * statsBoostMultiplier);
	}
	else if (itemLevel >= 103)
	{
		item->iAttackRating += (12 * statsBoostMultiplier);

		item->iDefenseRating += (14 * statsBoostMultiplier);

		item->iPotionStorage += (4 * statsBoostMultiplier);
	}
}

void ServerCore::SetRarityBoostAmulets(Item* item, const int statsBoostMultiplier, const int itemLevel)
{
	item->iSpirit += (-3 * statsBoostMultiplier);

	if (itemLevel >= 0 && itemLevel < 103)
	{
		item->fAddHP += static_cast<float>(2 * statsBoostMultiplier);

		item->fAddMP += static_cast<float>(2 * statsBoostMultiplier);
	}
	else if (itemLevel >= 103)
	{
		item->fAddHP += static_cast<float>(3 * statsBoostMultiplier);

		item->fAddMP += static_cast<float>(4 * statsBoostMultiplier);
	}
}

void ServerCore::SetRarityBoostRings(Item* item, const int statsBoostMultiplier, const int itemLevel)
{
	item->iSpirit += (-3 * statsBoostMultiplier);

	if (itemLevel >= 0 && itemLevel < 103)
	{
		item->fAddHP += static_cast<float>(1 * statsBoostMultiplier);

		item->fAddSP += static_cast<float>(2 * statsBoostMultiplier);
	}
	else if (itemLevel >= 103)
	{
		item->fAddHP += static_cast<float>(3 * statsBoostMultiplier);

		item->fAddSP += static_cast<float>(4 * statsBoostMultiplier);
	}
}

void ServerCore::SetRarityBoostShields(Item* item, const int statsBoostMultiplier, const int itemLevel)
{
	item->iStrength += (-3 * statsBoostMultiplier);

	if (itemLevel >= 0 && itemLevel < 103)
	{
		item->fAbsorbRating += static_cast<float>(0.5 * statsBoostMultiplier);

		item->iDefenseRating += (7 * statsBoostMultiplier);

		item->fBlockRating += static_cast<float>(.3 * statsBoostMultiplier);
	}
	else if (itemLevel >= 103 && itemLevel <= 110)
	{
		item->fAbsorbRating += static_cast<float>(.7 * statsBoostMultiplier);

		item->iDefenseRating += (13 * statsBoostMultiplier);

		item->fBlockRating += static_cast<float>(.3 * statsBoostMultiplier);
	}
	else if (itemLevel > 110)
	{
		item->fAbsorbRating += static_cast<float>(1 * statsBoostMultiplier);

		item->iDefenseRating += (20 * statsBoostMultiplier);

		item->fBlockRating += static_cast<float>(0.5 * statsBoostMultiplier);
	}
}

void ServerCore::SetRarityBoostOrbs(Item* item, const int statsBoostMultiplier, const int itemLevel)
{
	if (itemLevel >= 0 && itemLevel < 103)
	{
		item->fBlockRating += static_cast<float>(0.3 * statsBoostMultiplier);

		item->fAbsorbRating += static_cast<float>(0.5 * statsBoostMultiplier);

		item->iDefenseRating += (7 * statsBoostMultiplier);
	}
	else if (itemLevel >= 103 && itemLevel <= 110)
	{
		item->fBlockRating += static_cast<float>(0.3 * statsBoostMultiplier);

		item->fAbsorbRating += static_cast<float>(0.7 * statsBoostMultiplier);

		item->iDefenseRating += (13 * statsBoostMultiplier);
	}
	else if (itemLevel > 110)
	{
		item->fBlockRating += static_cast<float>(0.5 * statsBoostMultiplier);

		item->fAbsorbRating += static_cast<float>(1 * statsBoostMultiplier);

		item->iDefenseRating += (20 * statsBoostMultiplier);

	}
}

