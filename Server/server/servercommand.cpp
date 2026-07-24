#include "stdafx.h"
#include "servercommand.h"
#include "Logger.h"
#include "Priestess.h"
#include "HNSSkill.h"
#include "Utilities.h"

#define COMMAND StrCmpPT

typedef UINT( __cdecl *t_GetItemBySpeckPerf )( UserData * pcUserData, UserData * pcUserDataRecv, char * ItemCodeOrName );
t_GetItemBySpeckPerf GetItemBySpeckPerf = ( t_GetItemBySpeckPerf )0x08B70388;

typedef BOOL( __cdecl *tfnGetParamOld )(char * pszBuffRecv, char * pszBuffer);
tfnGetParamOld fnGetParamOld = (tfnGetParamOld)0x00552320;

typedef UserData*(__cdecl *tfnGetUsedataLoginName)(char * pszName);
tfnGetUsedataLoginName fnGetUsedataLoginName = (tfnGetUsedataLoginName)0x00552400;

extern int iUpdateMaxPacket;
extern int iUpdateMaxUnitMask;
extern int iUpdateFrameCounter;

int StrCmpPT( const char *Comando1, const char *Comando2 )
{
	int Resultado = 0;
	__asm{
		pushad
		push ecx
		push Comando2
		mov eax, Comando1
		mov ecx, 0x005522D0
		call ecx
		add esp, 4
		mov Resultado, eax
		pop ecx
		popad
	}
	return Resultado;
}

void WriteSQLSkillInfo( char * pszColumnName, char * pszSkillTier, int iClass )
{
	int iCount = 0;

	struct SQLSkillDataInfo
	{
		char szColumnName[64];
		char szValues[10][32];
	};

	SQLSkillDataInfo * ps = new SQLSkillDataInfo[10];

	SQLConnection * pcDB = SQLCONNECTION( DATABASEID_SkillDB );

	if ( pcDB->Open() )
	{
		if ( pcDB->Prepare( FormatString( "SELECT * FROM %s", pszColumnName ) ) )
		{
			if ( pcDB->Execute() )
			{
				if ( pcDB->GetColumnCount( iCount ) )
				{
					for ( int i = 0; i < iCount; i++ )
					{
						pcDB->GetColumnName( i + 1, ps[i].szColumnName, _countof( ps[i].szColumnName ) );
					}
				}

				int j = 0;
				while ( pcDB->Fetch() )
				{
					for ( int i = 0; i < iCount; i++ )
					{
						pcDB->GetData( i + 1, PARAMTYPE_String, ps[i].szValues[j], _countof( ps[i].szValues[j] ) );
					}

					j++;
				}
			}
		}

		pcDB->Close();
	}

	char * pszClass = "Fighter";

	if ( iClass == CHARACTERCLASS_Mechanician )
		pszClass = "Mech";
	if ( iClass == CHARACTERCLASS_Archer )
		pszClass = "Archer";
	if ( iClass == CHARACTERCLASS_Pikeman )
		pszClass = "Pike";
	if ( iClass == CHARACTERCLASS_Atalanta )
		pszClass = "Ata";
	if ( iClass == CHARACTERCLASS_Knight )
		pszClass = "Knight";
	if ( iClass == CHARACTERCLASS_Magician )
		pszClass = "Mage";
	if ( iClass == CHARACTERCLASS_Priestess )
		pszClass = "Prs";
	if ( iClass == CHARACTERCLASS_Assassin )
		pszClass = "Asn";
	if ( iClass == CHARACTERCLASS_Shaman )
		pszClass = "Sha";

	LOGERROR( "[%s] BEGIN", pszColumnName );
	for ( int i = 0; i < iCount; i++ )
	{
		LOGERROR( "const int ia%s%s%s[10] = { %s, %s, %s, %s, %s, %s, %s, %s, %s, %s };",
			pszClass,
			pszSkillTier,
			ps[i].szColumnName,
			ps[i].szValues[0],
			ps[i].szValues[1],
			ps[i].szValues[2],
			ps[i].szValues[3],
			ps[i].szValues[4],
			ps[i].szValues[5],
			ps[i].szValues[6],
			ps[i].szValues[7],
			ps[i].szValues[8],
			ps[i].szValues[9] );
	}

	DEBUG( "[%s] END", pszColumnName );
	DEBUG( "" );

	SAFE_DELETE_ARRAY ( ps );
}

BOOL WriteMonsterStatus( UnitData * pcUnitData, const char * pszTableName, const char * pszNameAttribute, char * pszValueAttribute)
{
	char szQuery[512] = { 0 };

	BOOL bRet = FALSE;

	int iMonsterID = CHARACTERSERVER->SQLGetMonsterID( pcUnitData->GetName() );

	float fValue = 0.0f;
	int iValue = 0;
	INT64 iValue64 = 0;

	EDatabaseDataType iParamType = PARAMTYPE_Null;

	//Get Parameter Type
	SQLConnection * pcDB = SQLCONNECTION( DATABASEID_GameDB_Monsters, 5 );
	if ( pcDB->Open() )
	{
		iParamType = pcDB->GetColumnType( pszTableName, pszNameAttribute );

		pcDB->Close();
	}


	if ( iParamType == PARAMTYPE_Integer )
		iValue = atoi( pszValueAttribute );
	else if ( iParamType == PARAMTYPE_Float )
		fValue = (float)atof( pszValueAttribute );
	else if ( iParamType == PARAMTYPE_Int64 )
		iValue64 = _atoi64( pszValueAttribute );
	else
		iParamType = PARAMTYPE_String;

	STRINGFORMAT( szQuery, "UPDATE %s SET %s=? WHERE ID=%d", pszTableName, pszNameAttribute, iMonsterID );

	if ( iMonsterID != -1 )
	{
		pcDB = SQLCONNECTION( DATABASEID_GameDB_Monsters, 6 );

		if ( pcDB->Open() )
		{
			if ( pcDB->Prepare( szQuery ) )
			{

				if ( iParamType == PARAMTYPE_Integer )
					pcDB->BindParameterInput( 1, iParamType, &iValue );
				else if ( iParamType == PARAMTYPE_Float )
					pcDB->BindParameterInput( 1, iParamType, &fValue );
				else if ( iParamType == PARAMTYPE_Int64 )
					pcDB->BindParameterInput( 1, iParamType, &iValue64 );
				else
					pcDB->BindParameterInput( 1, PARAMTYPE_String, pszValueAttribute, STRLEN( pszValueAttribute ) );

				if ( pcDB->ExecuteUpdate() )
					bRet = TRUE;
			}
			pcDB->Close();
		}
	}

	return bRet;
}

ServerCommand::ServerCommand()
{
}


ServerCommand::~ServerCommand()
{
}

/// <summary>
/// Check for any command to be executed every ten seconds
/// </summary>
void ServerCommand::SQLReadAndExecuteGMCommand ()
{
	if ( LOGIN_SERVER )
		return;

	SQLConnection * pcDB = SQLCONNECTION ( DATABASEID_ServerDB, 18 );

	if ( pcDB->Open () )
	{
		std::vector<int> excutedDBIds;
		std::vector<int> excutionResult;

		if ( pcDB->Prepare ( "SELECT [ID], [Command], [UnixTimeToExecute] FROM GMCommand WHERE IsActive = 1" ) )
		{
			char szCommand[100];
			int iDBId;
			DWORD unixTimeToExecute;
			DWORD unixTimeNow = GetUnixTime ();

			if ( pcDB->Execute () )
			{
				while ( pcDB->Fetch () )
				{
					szCommand[0] = { 0 };

					pcDB->GetData ( 1, EDatabaseDataType::PARAMTYPE_Integer, &iDBId );
					pcDB->GetData ( 2, EDatabaseDataType::PARAMTYPE_String, szCommand, 100 );
					pcDB->GetData ( 3, EDatabaseDataType::PARAMTYPE_Integer, &unixTimeToExecute );

					if ( unixTimeToExecute >= unixTimeNow )
					{
						excutedDBIds.push_back ( iDBId );

						if ( OnGameMasterAdminCommand ( NULL, szCommand ) ||
							OnGameMasterLevel3Command ( NULL, szCommand ) ||
							OnGameMasterLevel2Command ( NULL, szCommand ) ||
							OnGameMasterLevel1Command ( NULL, szCommand ) )
						{
							LOGGER->LogStatus ( "GMCommand command successful! '%s'", szCommand );
							excutionResult.push_back ( 1 );
						}
						else
						{
							LOGERROR ( "GMCommand command failed! '%s'", szCommand );
							excutionResult.push_back ( 0 );
						}
					}
				}

				for ( UINT i = 0; i < excutedDBIds.size (); i++ )
				{
					pcDB->Clear ();
					if ( pcDB->Prepare ( "UPDATE GMCommand SET IsActive=0, Result=? WHERE ID=?" ) )
					{
						pcDB->BindParameterInput ( 1, EDatabaseDataType::PARAMTYPE_Integer, &excutionResult[i] );
						pcDB->BindParameterInput ( 2, EDatabaseDataType::PARAMTYPE_Integer, &excutedDBIds[i] );

						pcDB->ExecuteUpdate ();
					}
				}
			}
		}

		pcDB->Close ();
	}
}

void ServerCommand::SQLRead()
{
	if ( GAME_SERVER )
		return;

	int iProcessed[MAX_COMMANDSPERTIME];
	for ( int i = 0; i < MAX_COMMANDSPERTIME; i++ )
		iProcessed[i] = -1;

	SQLConnection * pcDB = SQLCONNECTION( DATABASEID_ServerDB, 19 );

	int iType;

	if ( pcDB->Open() )
	{
		if ( pcDB->Prepare( "SELECT ID, CommandTypeID, Parameter1, Parameter2, Parameter3 FROM Command WHERE DateProcessed IS NULL" ) )
		{
			pcDB->Execute();

			for ( int i = 0; i < MAX_COMMANDSPERTIME; i++ )
			{
				if ( !pcDB->Fetch() )
					break;

				SQLServerCommand s;
				pcDB->GetData( 1, PARAMTYPE_Integer, &iProcessed[i] );
				pcDB->GetData( 2, PARAMTYPE_Integer, &iType );
				pcDB->GetData( 3, PARAMTYPE_String, s.szParameter1, _countof( s.szParameter1 ) );
				pcDB->GetData( 4, PARAMTYPE_String, s.szParameter2, _countof( s.szParameter2 ) );
				pcDB->GetData( 5, PARAMTYPE_String, s.szParameter3, _countof( s.szParameter3 ) );

				s.iType = static_cast<ESQLServerCommand>( iType );

				if ( !SERVERCOMMAND->Process( &s ) )
					iProcessed[i] = -1;
			}
		}

		for ( int i = 0; i < MAX_COMMANDSPERTIME; i++ )
		{
			if ( iProcessed[i] != -1 )
			{
				pcDB->Clear();

				if ( pcDB->Prepare( "UPDATE Command SET DateProcessed=GETDATE() WHERE ID=?" ) )
				{
					pcDB->BindParameterInput( 1, PARAMTYPE_Integer, &iProcessed[i] );

					pcDB->ExecuteUpdate();
				}
			}
		}

		pcDB->Close();
	}
}

BOOL ServerCommand::SQLReadFromCharacter( const char * pszCharacterName, ESQLServerCommand iCommand, SQLServerCommand & s )
{
	BOOL bRet = FALSE;

	SQLConnection * pcDB = SQLCONNECTION( DATABASEID_ServerDB, 20 );

	int iType = -1;

	if ( pcDB->Open() != FALSE )
	{
		if ( pcDB->Prepare( "SELECT CommandTypeID, Parameter1, Parameter2, Parameter3 FROM Command WHERE (Parameter3=?) AND (DateProcessed IS NULL)" ) )
		{
			pcDB->BindParameterInput( 1, PARAMTYPE_String, (char*)pszCharacterName, STRLEN( pszCharacterName ) );
			if ( pcDB->Execute() && pcDB->Fetch() )
			{
				pcDB->GetData( 1, PARAMTYPE_Integer, &iType );
				pcDB->GetData( 2, PARAMTYPE_String, s.szParameter1, _countof( s.szParameter1 ) );
				pcDB->GetData( 3, PARAMTYPE_String, s.szParameter2, _countof( s.szParameter2 ) );
				pcDB->GetData( 4, PARAMTYPE_String, s.szParameter3, _countof( s.szParameter3 ) );

				s.iType = (ESQLServerCommand)iType;

				bRet = TRUE;
			}
		}
		pcDB->Close();
	}

	return bRet;
}

bool ServerCommand::Process( SQLServerCommand * ps )
{
	bool bRet = false;

	switch ( ps->iType )
	{
		case SQLSERVERCOMMAND_ChangeCharacterName:
		if ( (ps->szParameter1[0] != 0) && (ps->szParameter2[0] != 0) )
		{
			int iResult = CHARACTERSERVER->ChangeCharacterName( ps->szParameter1, ps->szParameter2 );

			if ( iResult >= 0 )
				bRet = true;
		}
		else
			bRet = true;
		break;

		case SQLSERVERCOMMAND_ChangeCharacterLevel:
		if ( (ps->szParameter1[0] != 0) && (ps->szParameter2[0] != 0) )
		{
			char * pszCharacterName = ps->szParameter1;
			int iCharacterLevel = atoi( ps->szParameter2 );

			int iResult = CHARACTERSERVER->ChangeCharacterLevel( pszCharacterName, iCharacterLevel );

			if ( iResult >= 0 )
				bRet = true;
		}
		else
			bRet = true;
		break;

		case SQLSERVERCOMMAND_ChangeCharacterClass:
		if ( (ps->szParameter1[0] != 0) && (ps->szParameter2[0] != 0) )
		{
			char * pszCharacterName = ps->szParameter1;
			int iCharacterClass = atoi( ps->szParameter2 );

			int iResult = CHARACTERSERVER->ChangeCharacterClass( pszCharacterName, iCharacterClass );

			if ( iResult >= 0 )
				bRet = true;
		}
		else
			bRet = true;
		break;

		default:
		break;
	}

	return bRet;
}

void ServerCommand::SetVersion( int iVersion )
{
	SERVER_VERSION		= iVersion;
	SERVER_VERSION_MIN	= SERVER_VERSION;
	SERVER_VERSION_MAX	= SERVER_VERSION;

	char szVersion[32] = { 0 };
	char szDir[MAX_PATH] = { 0 };
	char szBuf[256] = { 0 };
	GetCurrentDirectoryA( MAX_PATH, szDir );
	STRINGFORMAT( szBuf, "%s\\%s", szDir, "server.ini" );
	STRINGFORMAT( szVersion, "%d", iVersion );
	WritePrivateProfileStringA( "Server", "Version", szVersion, szBuf );
}

/**
iSpawnCount = max number of Moriphs to spawn globally. If not defined, read in value from .ini
iSpawnDelayMin = spawn delay in minutes after kill. If not defined, read in value from .ini
*/
void ServerCommand::SetMoriphEvent( BOOL bEnable, int iSpawnCount, int iSpawnDelayMin )
{
	INI::CReader cReader( "server.ini" );

	cReader.WriteOnOff( "Event", "WantedMoriph", bEnable );

	if (bEnable)
	{
		EVENT_WANTEDMORIPH_EVENT				= TRUE;

		if (iSpawnCount > 0)
		{
			cReader.WriteInt("Event", "WantedMoriphMaxSpawnCount", iSpawnCount);
			EVENT_HUNT_MAX_SPAWN = iSpawnCount;
		}
		else
		{
			EVENT_HUNT_MAX_SPAWN = cReader.ReadInt("Event", "WantedMoriphMaxSpawnCount");
		}

		if (iSpawnDelayMin > 0)
		{
			cReader.WriteInt("Event", "WantedMoriphSpawnDelayMin", iSpawnDelayMin);
			EVENT_HUNT_SPAWN_DELAY_MIN = iSpawnDelayMin;
		}
		else
		{
			EVENT_HUNT_SPAWN_DELAY_MIN = cReader.ReadInt("Event", "WantedMoriphSpawnDelayMin");
		}
	}
	else
	{
		EVENT_WANTEDMORIPH_EVENT				= FALSE;
		EVENT_HUNT_MAX_SPAWN			= 0;
		EVENT_HUNT_SPAWN_DELAY_MIN		= 0;
	}
}

/**
iSpawnCount = max number of Moriphs to spawn globally. If not defined, read in value from .ini
iSpawnDelayMin = spawn delay in minutes after kill. If not defined, read in value from .ini
*/
void ServerCommand::SetWolfEvent(BOOL bEnable, int iSpawnCount, int iSpawnDelayMin)
{
	INI::CReader cReader("server.ini");

	cReader.WriteOnOff("Event", "WantedWolf", bEnable);

	if (bEnable)
	{
		EVENT_WANTEDWOLF_EVENT = TRUE;

		if (iSpawnCount > 0)
		{
			cReader.WriteInt("Event", "WantedWolfMaxSpawnCount", iSpawnCount);
			EVENT_HUNT_MAX_SPAWN = iSpawnCount;
		}
		else
		{
			EVENT_HUNT_MAX_SPAWN = cReader.ReadInt("Event", "WantedWolfMaxSpawnCount");
		}

		if (iSpawnDelayMin > 0)
		{
			cReader.WriteInt("Event", "WantedWolfSpawnDelayMin", iSpawnDelayMin);
			EVENT_HUNT_SPAWN_DELAY_MIN = iSpawnDelayMin;
		}
		else
		{
			EVENT_HUNT_SPAWN_DELAY_MIN = cReader.ReadInt("Event", "WantedWolfSpawnDelayMin");
		}
	}
	else
	{
		EVENT_WANTEDWOLF_EVENT = FALSE;
		EVENT_HUNT_MAX_SPAWN = 0;
		EVENT_HUNT_SPAWN_DELAY_MIN = 0;
	}
}

void ServerCommand::SetHalloweenEvent( BOOL b )
{
	INI::CReader cReader( "server.ini" );
	cReader.WriteOnOff( "Event", "Halloween", b );
	INFO ( "EventHalloween = %d", b );
	EVENT_HALLOWEEN = b;
	EVENT_BEE = FALSE;
	EVENT_STARWARS = FALSE;

	if ( GAME_SERVER )
	{
		NETSERVER->SyncEventStatusToLoginServer( PacketsHeader::PKTHDR_NetHallowenEvent, EVENT_HALLOWEEN );
	}
}

void ServerCommand::SetMimicEvent ( BOOL b )
{
	INI::CReader cReader ( "server.ini" );
	cReader.WriteOnOff ( "Event", "Mimic", b );

	EVENT_MIMIC = b;
	INFO ( "EventMimic = %d", b );

	if ( GAME_SERVER )
	{
		NETSERVER->SyncEventStatusToLoginServer( PacketsHeader::PKTHDR_NetMimicEvent, EVENT_MIMIC );
	}
}

void ServerCommand::SetChristmasEvent ( BOOL b )
{
	INI::CReader cReader ( "server.ini" );
	cReader.WriteOnOff ( "Event", "Christmas", b );

	EVENT_CHRISTMAS = b;
	EVENT_BEE = FALSE;
	EVENT_STARWARS = FALSE;
	EVENT_HALLOWEEN = FALSE;

	INFO ( "EventChristmas = %d", b );

	if ( GAME_SERVER )
	{
		NETSERVER->SyncEventStatusToLoginServer( PacketsHeader::PKTHDR_NetChristmasEvent, EVENT_CHRISTMAS );
	}
}

void ServerCommand::SetEventGirlFree ( BOOL b )
{
	INI::CReader cReader ( "server.ini" );
	cReader.WriteOnOff ( "Event", "EventGirlFree", b );

	INFO ( "EventGirlFree = %d", b );
	EVENT_FREEEVENTGIRL = b;

	if ( GAME_SERVER )
	{
		NETSERVER->SyncEventStatusToLoginServer( PacketsHeader::PKTHDR_NetFreeEventGirl, EVENT_FREEEVENTGIRL );
	}
}

void ServerCommand::SetEasterEvent(BOOL b)
{
	INI::CReader cReader("server.ini");
	cReader.WriteOnOff("Event", "Easter", b);

	EVENT_EASTER = b;
	EVENT_STARWARS = FALSE;
	EVENT_CHRISTMAS = FALSE;
	EVENT_BEE = FALSE;
	EVENT_HALLOWEEN = FALSE;
	INFO( "EasterEvent = %d", b );

	if ( GAME_SERVER )
	{
		NETSERVER->SyncEventStatusToLoginServer( PacketsHeader::PKTHDR_NetEasterEvent, EVENT_EASTER );
	}
}



void ServerCommand::SetStarWarsEvent(BOOL b)
{
	INI::CReader cReader("server.ini");
	cReader.WriteOnOff("Event", "StarWars", b);

	EVENT_STARWARS = b;
	EVENT_EASTER = FALSE;
	EVENT_CHRISTMAS = FALSE;
	EVENT_BEE = FALSE;
	EVENT_HALLOWEEN = FALSE;
	INFO ( "StarWarsEvent = %d", b );

	if ( GAME_SERVER )
	{
		NETSERVER->SyncEventStatusToLoginServer( PacketsHeader::PKTHDR_NetStarWarsEvent, EVENT_STARWARS );
	}
}


void ServerCommand::SetBeeEvent(BOOL b)
{
	INI::CReader cReader("server.ini");
	cReader.WriteOnOff("Event", "Bee", b);

	EVENT_BEE = b;
	EVENT_STARWARS = FALSE;
	EVENT_EASTER = FALSE;
	EVENT_CHRISTMAS = FALSE;
	EVENT_HALLOWEEN = FALSE;
	INFO ( "BeeEvent = %d", b );

	if ( GAME_SERVER )
	{
		NETSERVER->SyncEventStatusToLoginServer( PacketsHeader::PKTHDR_NetBeeEvent, EVENT_BEE );
	}
}


void ServerCommand::SetValentineDayEvent(BOOL b)
{
	INI::CReader cReader("server.ini");

	cReader.WriteOnOff("Event", "ValentineDay", b);

	int eventType = CEventServer::ServerEventID::Valentine;
	int timerType = ITEMTIMERTYPE_BigHeadValentine;
	int itemID = ITEMID_BigHeadValentine;

	if (!EVENT_VALENTINESDAY && b == TRUE)
	{
		EVENT_VALENTINESDAY = true;

		/*SQLConnection* pcDB = SQLCONNECTION(DATABASEID_LogDB);

		if (pcDB->Open())
		{
		if (pcDB->Prepare("DELETE FROM EventLog WHERE EventType=?"))
		{
		pcDB->BindParameterInput(1, PARAMTYPE_Integer, &eventType);
		pcDB->Execute();
		}
		pcDB->Close();
		}*/

		//Add Valentine's head
		for (int i = 0; i < PLAYERS_MAX; i++)
		{
			if (USERSDATA[i].pcSocketData)
			{
				auto pcUser = USERDATATOUSER(&USERSDATA[i]);

				BOOL bAnyHeadFound = FALSE;

				for (int i = 0; i < PREMIUM_ITEM_TOTAL; i++)
				{
					ItemPremium* psItem = &pcUser->sPremiumData.saItemList[i];

					if (ITEMSERVER->IsHead(psItem->eType))
					{
						bAnyHeadFound = true;
						break;
					}
				}

				//can't add another head on top of head
				//you will lose the 'old head' info
				//requires more work, not enought time
				if (!bAnyHeadFound)
				{
					ITEMSERVER->SQLDeleteItemTimer(pcUser, itemID);

					//Add To User
					if (ITEMSERVER->AddItemTimer(pcUser, ITEMTIMERTYPE_BigHeadValentine, EItemID::ITEMID_BigHeadValentine, ITEMTIMERTIME_12H))
					{
						ITEMSERVER->SendItemTimer(pcUser, EItemID::ITEMID_BigHeadValentine, ITEMTIMERTYPE_BigHeadValentine, ITEMTIMERTIME_12H, ITEMTIMERTIME_12H);
					}

					SQLConnection* pcDB = SQLCONNECTION(DATABASEID_LogDB, 31);

					if (pcDB->Prepare("INSERT INTO EventLog ([CharacterID],[CharacterName],[EventType],[EventValue],[Date]) VALUES (?,?,8,0,GETDATE())"))
					{
						pcDB->BindParameterInput(1, PARAMTYPE_Integer, &pcUser->iCharacterID);
						pcDB->BindParameterInput(2, PARAMTYPE_String, CHARACTERSERVER->GetCharacterName(pcUser->pcUserData));

						pcDB->Execute();
					}

					pcDB->Close();

				}
			}
		}
	}
	else if (EVENT_VALENTINESDAY && b == FALSE)
	{
		EVENT_VALENTINESDAY = FALSE;
	}
}

void ServerCommand::SetAgingEvent( BOOL bFree, BOOL bNoBreak, BOOL bHalfPrice )
{
	INI::CReader cReader( "server.ini" );

	cReader.WriteOnOff( "Event", "AgingFree", bFree );
	cReader.WriteOnOff( "Event", "AgingNoBreak", bNoBreak );
	cReader.WriteOnOff( "Event", "AgingHalfPrice", bHalfPrice );

	EVENT_AGINGFREE = bFree;
	EVENT_AGING_NOBREAK = bNoBreak;
	EVENT_AGING_HALFPRICE = bHalfPrice;
}

void ServerCommand::OnActivateGameMaster( User * pcUser )
{
	UserData * pcUserData = pcUser->pcUserData;

	int iGameMasterType = 0;
	int iGameMasterLevel = 0;

	char szUserID[ 32 ] = { 0 };

	SQLConnection * pcDB = SQLCONNECTION( DATABASEID_UserDB_LocalServer_CharInfo );


	char * pszCharName = CHARACTERSERVER->GetCharacterName( pcUser );

	if ( pcDB->Open() )
	{
		if ( pcDB->Prepare( "SELECT AccountName FROM CharacterInfo WHERE Name=?" ) )
		{
			pcDB->BindParameterInput( 1, PARAMTYPE_String, pszCharName, 32 );
			if ( pcDB->Execute() && pcDB->Fetch() )
			{
				pcDB->GetData( 1, PARAMTYPE_String, szUserID, 32 );
			}
		}
		pcDB->Close();
	}

	pcDB = SQLCONNECTION ( DATABASEID_UserDB_PrimaryServer );

	if ( pcDB->Open() )
	{
		if ( pcDB->Prepare( "SELECT GameMasterType, GameMasterLevel FROM UserInfo WHERE AccountName=?" ) )
		{
			pcDB->BindParameterInput( 1, PARAMTYPE_String, szUserID );
			if ( pcDB->Execute() && pcDB->Fetch() )
			{
				pcDB->GetData( 1, PARAMTYPE_Integer, &iGameMasterType );
				pcDB->GetData( 2, PARAMTYPE_Integer, &iGameMasterLevel );
			}
		}
		pcDB->Close();
	}

	if (iGameMasterType && (iGameMasterLevel >= GAMELEVEL_One))
	{
		pcUser->pcUserData->iGameLevel = (EGameLevel)iGameMasterLevel;
		CHATSERVER->SendChatEx( pcUser, EChatColor::CHATCOLOR_White, "> GM level %d activated!", iGameMasterLevel );
	}
	else
	{
		pcUser->pcUserData->iGameLevel = EGameLevel::GAMELEVEL_None;
		iGameMasterLevel = EGameLevel::GAMELEVEL_None;
	}

	NETSERVER->SyncGameMaster( pcUserData );
}

BOOL ServerCommand::OnGameMasterAdminCommand( User * pcUser, const char * pszBuff )
{
	UserData * pcUserData = pcUser->pcUserData;

	int iLen = 0;

	char szCommandParam1[256] = { 0 };
	char szCommandParam2[256] = { 0 };
	char szCommandParam3[256] = { 0 };
	char szCommandParam4[256] = { 0 };
	char szCommandParam5[256] = { 0 };
	char szCommandParam6[256] = { 0 };

	//Usage: /!unmute_and_reset <char name>
	//WARNING - This also clears out all existing strikes that the account has
	if ( iLen = COMMAND( "/!unmute_and_reset", pszBuff ) )
	{
		if ( GetParameterString( pszBuff, 1, szCommandParam1 ) )
		{
			ACCOUNTSERVER->UnmuteAccountByCharacterName( pcUser, szCommandParam1, TRUE );
			return TRUE;
		}
		else
		{
			CHATSERVER->SendChat( pcUser, CHATCOLOR_Error, "> Usage: /!unmute_and_reset <char name>" );
		}

		return TRUE;
	}

	else if ( COMMAND( "/rarity_update", pszBuff ) || COMMAND( "/Rarity_Update", pszBuff ) )
	{
		ITEMSERVER->UpdateItemRarity();
		CHATSERVER->SendChatAllGM( "GM > Pulled rarity values from DB" );
	}

	else if (COMMAND("/specmod_update", pszBuff) || COMMAND("/SpecMod_Update", pszBuff))
	{
		ITEMSERVER->GetItemSpecMod();
		CHATSERVER->SendChatAllGM("GM > Pulled spec mod values from DB");
	}

	else if (COMMAND("/baseline_update", pszBuff) || COMMAND("/Baseline_Update", pszBuff))
	{
		ITEMSERVER->GetBaselineMod();
		CHATSERVER->SendChatAllGM("GM > Pulled baseline values from DB");
	}


	else if ( COMMAND( "/rarity_mod_update", pszBuff ) || COMMAND( "/Rarity_Mod_Update", pszBuff ) )
	{
		ITEMSERVER->UpdateItemRarityMods();
		CHATSERVER->SendChatAllGM( "GM > Pulled rarity mods from DB" );
	}
	else if ( COMMAND( "/rarity_get", pszBuff ) || COMMAND( "/Rarity_Get", pszBuff ) )
	{
		if ( GetParameterString( pszBuff, 1, szCommandParam1 ) &&
			GetParameterString( pszBuff, 2, szCommandParam2 ) )
		{
			int RarityGroup = atoi( szCommandParam1 );
			int Rarity = atoi( szCommandParam2 );

			int ReturnChance = ITEMSERVER->GetRarityChance( RarityGroup, Rarity );

			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_White, "> Chance: %d in 10000 ", ReturnChance );
		}
	}

	else if ( COMMAND( "/rarity_set", pszBuff ) || COMMAND( "/Rarity_Set", pszBuff ) )
	{
		if ( GetParameterString( pszBuff, 1, szCommandParam1 ) &&
			GetParameterString( pszBuff, 2, szCommandParam2 ) &&
			GetParameterString( pszBuff, 3, szCommandParam3 ) )
		{
			int RarityGroup = atoi( szCommandParam1 );
			int Rarity = atoi( szCommandParam2 );
			int Chance = atoi( szCommandParam3 );

			ITEMSERVER->SetRarityDB( RarityGroup, Rarity, Chance );

			CHATSERVER->SendChatAllGM( "GM> Updated Rarity Chance in DB" );

			ITEMSERVER->UpdateItemRarity();
		}
	}

#ifdef ITEMFIXES
	else if ( COMMAND( "/affix_update", pszBuff ) || COMMAND( "/Affix_Update", pszBuff ) )
	{
		ITEMSERVER->UpdateItemFixes( SQLCONNECTION( DATABASEID_GameDB ) );
		CHATSERVER->SendChat( pcUser, CHATCOLOR_White, "> Pulled affixes values from DB" );
	}
#endif

	else if ( iLen = COMMAND( "/quest_npc_reload", pszBuff ) )
	{
		QUESTSERVER->LoadNPCQuests();

		//also reload login server quest data
		if ( GAME_SERVER )
		{
			CHATSERVER->SendChatAllGM( "GM> NPC Quests reloaded" );
			NETSERVER->SendGMCommandToLoginServer( pcUser, pszBuff );
		}
	}
	else if ( iLen = COMMAND( "/force_night_mode", pszBuff ) ) //dev experiemental only
	{
		if ( GetParameterString( pszBuff, 1, szCommandParam1 ) )
		{
			MAPSERVER->ForceNightMode( atoi( szCommandParam1 ) >= 1 );
		}
	}
	else if ( iLen = COMMAND( "/force_day_mode", pszBuff ) ) //dev  experiemental only
	{
		if ( GetParameterString( pszBuff, 1, szCommandParam1 ) )
		{
			MAPSERVER->ForceDayMode( atoi( szCommandParam1 ) >= 1 );
		}
	}
	else if ( iLen = COMMAND( "/testmap_hp", pszBuff ) )
	{
		if ( GetParameterString( pszBuff, 1, szCommandParam1 ) )
		{
			TESTMAPHANDLER->SetMonsterHP( atoi( szCommandParam1 ) );
			TESTMAPHANDLER->Reset();
			TESTMAPHANDLER->SpawnTestMonstersDps( pcUser );
		}
	}
	else if ( iLen = COMMAND( "/testmap_mon", pszBuff ) )
	{
		if ( GetParameterString( pszBuff, 1, szCommandParam1 ) )
		{
			CharacterData * psCharacterData = UNITSERVER->GetCharacterDataByName( szCommandParam1 );
			UnitInfo * psUnitInfo = psCharacterData != NULL ? psCharacterData->psUnitInfo : NULL;

			if ( psCharacterData && psUnitInfo )
			{
				TESTMAPHANDLER->SetTestMonsterName( szCommandParam1 );
				TESTMAPHANDLER->Reset();
				//TESTMAPHANDLER->SpawnTestMonstersAoE( pcUser );
				CHATSERVER->SendChatAllGM( "TestMap> Mon set to: '%s'", szCommandParam1 );
			}
			else
			{
				CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "TestMap> Monster does not exist: '%s'", szCommandParam1 );
			}
		}
		else
		{
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "TestMap> Current monster: '%s'", TESTMAPHANDLER->GetTestMonsterName().c_str() );
		}

		return TRUE;
	}
	else if ( COMMAND( "/testmap_dps", pszBuff ) )
	{
		TESTMAPHANDLER->ClearUnitsFromMap();
		TESTMAPHANDLER->SpawnTestMonstersDps( pcUser );
		CHATSERVER->SendChatAllGM( "TestMap> DPS Mode Active" );

		return TRUE;
	}
	else if ( iLen = COMMAND( "/testmap_reset", pszBuff ) )
	{
		TESTMAPHANDLER->Reset();
	}

	if ( iLen = COMMAND( "/skills_update_everyone", pszBuff ) )
	{
		if ( GAME_SERVER )
		{
			CheckAndReSyncSkillsInfoForEveryone();
			CheckAndReSyncSkillsDataforEveryone();
			CHATSERVER->SendChatAllGM( "GM> Skills updated For everyone!" );
		}

		return TRUE;
	}

	if ( iLen = COMMAND( "/giveitem", pszBuff ) )
	{
		if ( GetParameterString( pszBuff, 1, szCommandParam1 ) &&
			GetParameterString( pszBuff, 2, szCommandParam2 ) )
		{
			Item sItem;
			auto psDef = ITEMSERVER->FindItemDefByCode( szCommandParam2 );

			if ( psDef )
			{
				int iAccountId = USERSERVER->SQLGetAccountID( szCommandParam1 );

				//Account exists?
				if ( iAccountId != -1 )
				{
					ITEMSERVER->AddItemOpenBox( szCommandParam1, szCommandParam2, 0, 1 );
					CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> Item Distributor: Added '%s' for '%s'", szCommandParam2, szCommandParam1 );

					UserData * userData = USERDATABYACCOUNTNAME( szCommandParam1 );

					//user is online ?
					if ( ( userData != NULL ) )
					{
						auto u = USERDATATOUSER( userData );
						CHATSERVER->SendChat( u, CHATCOLOR_Global, "GM> You received an item. Go to Item Distributon in town to collect your item." );
					}
					else
					{
						LOGSERVER->OnLogItemEx( pcUser,
							ITEMLOGID_ItemGivenToUserByGM,
							"Item '%s' added to Item Distributor from GM '%s'",
							szCommandParam2,
							CHARACTERSERVER->GetCharacterName( pcUser ) );
					}
				}
				else
				{
					CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> Account does not exist:  '%s'", szCommandParam1 );
				}
			}
			else
			{
				CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> Invalid item code:  '%s'", szCommandParam2 );
			}
		}
		else
		{
			CHATSERVER->SendChat( pcUser, CHATCOLOR_Error, "> Syntax: /giveitem <account id> <item code>" );
		}

		return TRUE;
	}

	else if ( COMMAND( "/getquestitem", pszBuff ) )
	{
		if ( GAME_SERVER )
		{
			NETSERVER->SendGMCommandToLoginServer(pcUser, pszBuff);
			return TRUE;
		}

		//1st parm is fs or as or ms etc
		//2nd parm is starting age level
		if ( GetParameterString( pszBuff, 1, szCommandParam1 ) && GetParameterString( pszBuff, 2, szCommandParam2 ))
		{
			int iRankNum = atoi( szCommandParam2 );
			if ( iRankNum > 5 )
				iRankNum = 5;

			auto charClass = ClassShortStrToCharacterClass( szCommandParam1 );
			const std::string itemCode = GetCharacterTier3QuestItem( charClass );

			Item sItem;
			if ( QUESTSERVER->GetQuestWeapon( pcUser, &sItem, itemCode, iRankNum ) )
			{
				ITEMSERVER->SendItemData( pcUser->pcUserData, &sItem, TRUE );
				ITEMSERVER->AddItemInventory( pcUser->pcUserData, &sItem );

				CHATSERVER->SendChatAllGM( "GM> Get Quest Item: %s", sItem.szItemName );
			}
		}
	}

	else if ( COMMAND( "/finishquest", pszBuff ) )
	{
		if ( GetParameterString( pszBuff, 1, szCommandParam1 ) )
		{
			int iQuestID = atoi( szCommandParam1 );
			if ( iQuestID > 0 )
			{
				if ( QUESTSERVER->SQLFinishQuest( pcUser, iQuestID ) )
					CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> Quest %d completed!", iQuestID );
				else
					CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> Failed to complete quest %d", iQuestID );
			}
		}
		return TRUE;
	}

	else if ( COMMAND( "/activequests", pszBuff ) )
	{
		std::vector<int> vQuests = QUESTSERVER->SQLGetActiveQuestsID( pcUser );
		if ( vQuests.empty() )
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> No active quests." );
		else
			for ( int id : vQuests )
				CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> Active Quest ID: %d", id );
		return TRUE;
	}

	//override the EXE one
	else if (COMMAND ( "/getitem", pszBuff) ||
			 COMMAND ( "/getitemold", pszBuff ) )
	{
		if ( GAME_SERVER )
		{
			NETSERVER->SendGMCommandToLoginServer(pcUser, pszBuff);
		}
		else
		{
			if ( GetParameterString( pszBuff, 1, szCommandParam1 ) )
			{
				BOOL bIsOld = COMMAND( "/getitemold", pszBuff );

				ECharacterClass charClass = ECharacterClass::CHARACTERCLASS_None; //Default = 0 = random spec, including non-spec
				int iSpecAtkRating = 0;
				int iItemAge = 0;
				int iItemRarity = 0;
				BOOL bPerfect = FALSE;

				if ( GetParameterString( pszBuff, 2, szCommandParam2 ) )
				{
					charClass = ClassShortStrToCharacterClass( szCommandParam2 );
				}

				if ( GetParameterString( pszBuff, 3, szCommandParam3 ) )
				{
					iSpecAtkRating = atoi( szCommandParam3 );

					//validate
					if ( iSpecAtkRating < 1 || iSpecAtkRating > 3 )
						iSpecAtkRating = 0;
				}

				if ( GetParameterString( pszBuff, 4, szCommandParam4 ) )
				{
					iItemAge = atoi( szCommandParam4 );

					//validate
					if ( iItemAge < 1 || iItemAge > 20 )
						iItemAge = 0;
				}

				if ( GetParameterString( pszBuff, 5, szCommandParam5 ) )
				{
					iItemRarity = atoi( szCommandParam5 );

					//validate
					if ( iItemRarity < 0 || iItemRarity > 5 )
						iItemRarity = 0;
				}

				if ( GetParameterString( pszBuff, 6, szCommandParam6 ) )
				{
					int iValue = atoi( szCommandParam6 );

					if ( iValue == 1 )
						bPerfect = TRUE;
				}

				Item sItem;
				auto psDef = bIsOld ? ITEMSERVER->FindOldItemDefByCode( szCommandParam1 ) : ITEMSERVER->FindItemDefByCode( szCommandParam1 );
				if ( psDef )
				{
					ZeroMemory( &sItem, sizeof( Item ) );

					EItemRarity CurrentRarity = static_cast<EItemRarity>( iItemRarity );

					if ( bPerfect )
					{
						*(UINT *)0x8B70264 = 1;
						*(UINT *)0x8B70268 = charClass;
						ITEMSERVER->CreateItem( &sItem, psDef, EItemSource::GameMaster, charClass, iSpecAtkRating, iItemAge );
						*(UINT *)0x8B70264 = 0;
						*(UINT *)0x8B70268 = 0;

						ITEMSERVER->OnSetItemPerfect( &sItem );

						sItem.iItemSpecialType = 1;
					}
					else
					{
						ITEMSERVER->CreateItem( &sItem, psDef, EItemSource::GameMaster, charClass, iSpecAtkRating, iItemAge, CurrentRarity );
					}

					ITEMSERVER->ReformItem( &sItem );
					ITEMSERVER->SendItemData( pcUser->pcUserData, &sItem, TRUE );
					ITEMSERVER->AddItemInventory( pcUser->pcUserData, &sItem );
					CHATSERVER->SendChatAllGM( "GM> Get Item: %s", sItem.szItemName );
				}
				else
				{
					CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> Get Item Failed for: '%s'", szCommandParam1 );
				}
			}
		}

		return TRUE;
	}

	else if (iLen = COMMAND("/getmpg", pszBuff))
	{
		if (GetParameterString(pszBuff, 1, szCommandParam1))
		{
			int iCount = atoi(szCommandParam1);

			if (iCount < 1)   iCount = 1;
			if (iCount > 300) iCount = 300;

			ITEMSERVER->SendPotions( pcUser, ITEMID_ManaGrand, iCount, EItemSource::GameMaster, TRUE );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> Grand mana potions x%d received!", iCount );
			return TRUE;
		}
	}

	else if (iLen = COMMAND("/gethpg", pszBuff))
	{
		if (GetParameterString(pszBuff, 1, szCommandParam1))
		{
			int iCount = atoi(szCommandParam1);

			if (iCount < 1)   iCount = 1;
			if (iCount > 300) iCount = 300;

			ITEMSERVER->SendPotions( pcUser, ITEMID_HealthGrand, iCount, EItemSource::GameMaster, TRUE );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> Grand health potions x%d received!", iCount );

			return TRUE;
		}
	}

	else if (iLen = COMMAND("/getspg", pszBuff))
	{
		if (GetParameterString(pszBuff, 1, szCommandParam1))
		{
			int iCount = atoi(szCommandParam1);

			if (iCount < 1)   iCount = 1;
			if (iCount > 300) iCount = 300;

			ITEMSERVER->SendPotions( pcUser, ITEMID_StaminaGrand, iCount, EItemSource::GameMaster, TRUE );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> Grand stamina potions x%d received!", iCount );

			return TRUE;
		}
	}

	else if (iLen = COMMAND("/getmp", pszBuff))
	{
		if (GetParameterString(pszBuff, 1, szCommandParam1))
		{
			int iCount = atoi(szCommandParam1);

			if (iCount < 1)   iCount = 1;
			if (iCount > 300) iCount = 300;

			ITEMSERVER->SendPotions( pcUser, ITEMID_ManaMystic, iCount, EItemSource::GameMaster, TRUE );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> Mystic mana potions x%d received!", iCount );
			return TRUE;
		}
	}

	else if (iLen = COMMAND("/gethp", pszBuff))
	{
		if (GetParameterString(pszBuff, 1, szCommandParam1))
		{
			int iCount = atoi(szCommandParam1);

			if (iCount < 1)   iCount = 1;
			if (iCount > 300) iCount = 300;

			ITEMSERVER->SendPotions( pcUser, ITEMID_HealthMystic, iCount, EItemSource::GameMaster, TRUE );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> Mystic health potions x%d received!", iCount );
			return TRUE;
		}
	}

	else if (iLen = COMMAND("/getsp", pszBuff))
	{
		if (GetParameterString(pszBuff, 1, szCommandParam1))
		{
			int iCount = atoi(szCommandParam1);

			if (iCount < 1)   iCount = 1;
			if (iCount > 300) iCount = 300;

			ITEMSERVER->SendPotions( pcUser, ITEMID_StaminaMystic, iCount, EItemSource::GameMaster, TRUE );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> Mystic stamina potions x%d received!", iCount );
			return TRUE;
		}
	}
	else if ( (iLen = COMMAND( "/add_map_indicator_script", pszBuff )) ) //see "/add_map_arrow" etc on client side
	{
		if (GetParameterString(pszBuff, 1, szCommandParam1) &&
			GetParameterString(pszBuff, 2, szCommandParam2) &&
			GetParameterString(pszBuff, 3, szCommandParam3) &&
			GetParameterString(pszBuff, 4, szCommandParam4) &&
			GetParameterString(pszBuff, 5, szCommandParam5) &&
			GetParameterString(pszBuff, 6, szCommandParam6))
		{
			int iMapID		   = atoi(szCommandParam1);
			auto iType		   = (MapIndicator::Type)atoi(szCommandParam2);
			int iValue		   = atoi(szCommandParam3);
			int iCompactPosX   = atoi(szCommandParam4);
			int iCompactPosZ   = atoi(szCommandParam5);
			int iAngleY		   = atoi(szCommandParam6);

			if ( MAPSERVER->AddMapIndicator( pcUser, iMapID, iType, iValue, iCompactPosX, iCompactPosZ, iAngleY ) )
			{
				if ( iType == MapIndicator::Type::NextMapArrow )
				{
					CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> Map indicator added at current pos for next map = %s (%d)", pszaMapsName[iValue], iValue );
				}
				else
				{
					CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> Map indicator added at current pos for type %d", iType );
				}
			}
		}
	}
	else if ( (iLen = COMMAND( "/remove_map_indicator", pszBuff )) || (iLen = COMMAND( "/remove_map_icon", pszBuff )) )
	{
		if ( MAPSERVER->RemoveMapIndicatorNearMe( pcUser ) )
		{
			CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Removed nearby indicator!");
		}
		else
		{
			CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Remove nearby indicator failed! None existed within 20 meters.");
		}
	}
	else if (iLen = COMMAND("/GetGold", pszBuff))
	{
		if (GetParameterString(pszBuff, 1, szCommandParam1))
		{
			int l_Gold = atoi(szCommandParam1);
			CHARACTERSERVER->GiveGOLD(pcUser, l_Gold, WHEREID_GiveMoney);
			CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Done!");
		}
		return TRUE;
	}
	if (iLen = COMMAND("/GetCoins", pszBuff))
	{
		if (GetParameterString(pszBuff, 1, szCommandParam1))
		{
			int iCoins = atoi(szCommandParam1);
			SQLConnection * pcDB = SQLCONNECTION(DATABASEID_UserDB_PrimaryServer);
			if (pcDB->Open())
			{
				if (pcDB->Prepare("UPDATE UserInfo SET Coins=Coins+? WHERE AccountName=?"))
				{
					pcDB->BindParameterInput(1, PARAMTYPE_Integer, &iCoins);
					pcDB->BindParameterInput(2, PARAMTYPE_String, pcUserData->szAccountName, 32);
					pcDB->ExecuteUpdate();
				}
				pcDB->Close();
			}
			CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> Added %d Coins to your account!", iCoins);
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/UpdateQuestActiveEvent", pszBuff ) )
	{
		//QUESTSERVER->UpdateQuestActiveMonsterKill();
		//CHATSERVER->SendChat( pcUser, CHATCOLOR_Error, "> Done!" );
		return TRUE;
	}
	if ( iLen = COMMAND( "/viewsocket", pszBuff ) )
	{
		if ( GetParameterString( pszBuff, 1, szCommandParam1 ) )
		{
			//ServerCore::UpdateCharacterItemsLevel( "C:\\Server\\Login\\Data", szCommandParam1 );

			CHATSERVER->SendChat( pcUser, CHATCOLOR_Error, "> Done!" );
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/SetToggleSocketFull", pszBuff ) )
	{

		SOCKETSERVER->SetFull( !SOCKETSERVER->IsFull() );
		CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> [%s]", SOCKETSERVER->IsFull() ? "TRUE" : "FALSE" );
		return TRUE;
	}

	if ( iLen = COMMAND( "/!SetToggleSocketFull", pszBuff ) )
	{

		SOCKETSERVER->SetFull( !SOCKETSERVER->IsFull() );
		CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> [%s]", SOCKETSERVER->IsFull() ? "TRUE" : "FALSE" );
		return TRUE;
	}

	if ( iLen = COMMAND( "/SetUsersOnlineMax", pszBuff ) )
	{
		if ( GetParameterString( pszBuff, 1, szCommandParam1 ) )
		{
			MAX_USERS = atoi( szCommandParam1 );
			if ( MAX_USERS < 1 )
				MAX_USERS = 1;

			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> [%d]", MAX_USERS );
		}
		return TRUE;
	}

	if ( iLen = COMMAND( "/!viewsocket", pszBuff ) )
	{
		//ServerCore::UpdateCharacterItemsLevelAll();
		CHATSERVER->SendChat( pcUser, CHATCOLOR_Error, "> Done!" );
		return TRUE;
	}
	if ( iLen = COMMAND( "/LeakMonsterTest", pszBuff ) )
	{
		// Is User Alive?
		GetParameterString( pszBuff, 1, szCommandParam1 ); // ID
		UnitData * pcUnitData = UNITDATABYIDMAP( atoi( szCommandParam1 ), pcUser->pcUserData->iMapID );
		if ( pcUnitData )
		{
			pcUnitData->psaDamageUsersData = new AttackDamageData[ATTACK_DAMAGE_MAX];
			if ( pcUnitData->psaDamageUsersData )
				ZeroMemory( pcUnitData->psaDamageUsersData, sizeof( AttackDamageData ) * ATTACK_DAMAGE_MAX );

			pcUnitData->dwLastActiveTime = TICKCOUNT + (6 * 60 * 1000);
			pcUnitData->dwUpdateCharInfoTime = 0;
		}
		return TRUE;
	}
	if ( iLen = COMMAND ( "/GetTickCount", pszBuff ) )
	{
		CHATSERVER->SendChatEx ( pcUser, CHATCOLOR_White, "> Current TICKCOUNT Is %d!", TICKCOUNT );
		return TRUE;
	}



	if( iLen = COMMAND( "/event_agingfree", pszBuff ) )
	{
		if( GetParameterString( pszBuff, 1, szCommandParam1 ) )
		{
			if( STRINGCOMPAREI( szCommandParam1, "true" ) )
			{
				SetAgingEvent( TRUE, EVENT_AGING_NOBREAK, EVENT_AGING_HALFPRICE );
				NETSERVER->SetAgingEvent( TRUE, EVENT_AGING_NOBREAK, EVENT_AGING_HALFPRICE);
				CHATSERVER->SendChat( pcUser, CHATCOLOR_Error, "> Aging Free Event ON!" );
			}
			else if( STRINGCOMPAREI( szCommandParam1, "false" ) )
			{
				SetAgingEvent( FALSE, EVENT_AGING_NOBREAK, EVENT_AGING_HALFPRICE);
				NETSERVER->SetAgingEvent( FALSE, EVENT_AGING_NOBREAK, EVENT_AGING_HALFPRICE);
				CHATSERVER->SendChat( pcUser, CHATCOLOR_Error, "> Aging Free Event OFF!" );
			}
			else
				CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> Use: /event_agingfree <true|false> [%s]", EVENT_AGINGFREE ? "On" : "Off" );
		}
		else
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> Use: /event_agingfree <true|false> [%s]", EVENT_AGINGFREE ? "On" : "Off" );

		return TRUE;
	}
	if (iLen = COMMAND("/event_agingnobreak", pszBuff))
	{

		if (GetParameterString(pszBuff, 1, szCommandParam1))
		{
			if (STRINGCOMPAREI(szCommandParam1, "true"))
			{
				SetAgingEvent(EVENT_AGINGFREE, TRUE, EVENT_AGING_HALFPRICE);
				NETSERVER->SetAgingEvent(EVENT_AGINGFREE, TRUE, EVENT_AGING_HALFPRICE);
				CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Aging No Break Event ON!");
			}
			else if (STRINGCOMPAREI(szCommandParam1, "false"))
			{
				SetAgingEvent(EVENT_AGINGFREE, FALSE, EVENT_AGING_HALFPRICE);
				NETSERVER->SetAgingEvent(EVENT_AGINGFREE, FALSE, EVENT_AGING_HALFPRICE);
				CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Aging No Break Event OFF!");
			}
			else
				CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> Use: /event_agingnobreak <true|false> [%s]", EVENT_AGING_NOBREAK ? "On" : "Off");
		}
		else
			CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> Use: /event_agingnobreak <true|false> [%s]", EVENT_AGING_NOBREAK ? "On" : "Off");

		return TRUE;
	}

	if (iLen = COMMAND("/event_aginghalfprice", pszBuff))
	{
		if (LOGIN_SERVER)
		{
			if (GetParameterString(pszBuff, 1, szCommandParam1))
			{
				if (STRINGCOMPAREI(szCommandParam1, "true"))
				{
					SetAgingEvent(EVENT_AGINGFREE, EVENT_AGING_NOBREAK, TRUE);
					NETSERVER->SetAgingEvent(EVENT_AGINGFREE, EVENT_AGING_NOBREAK, TRUE);
					CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Aging Half Price Event ON!");
				}
				else if (STRINGCOMPAREI(szCommandParam1, "false"))
				{
					SetAgingEvent(EVENT_AGINGFREE, EVENT_AGING_NOBREAK, FALSE);
					NETSERVER->SetAgingEvent(EVENT_AGINGFREE, EVENT_AGING_NOBREAK, FALSE);
					CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Aging Half Price Event OFF!");
				}
				else
					CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> Use: /event_aginghalfprice <true|false> [%s]", EVENT_AGING_HALFPRICE ? "On" : "Off");
			}
			else
				CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> Use: /event_aginghalfprice <true|false> [%s]", EVENT_AGING_HALFPRICE ? "On" : "Off");

			return TRUE;
		}
	}


	//Event Girl
	if ( COMMAND ( "/event_girl", pszBuff ) )
	{
		if ( GetParameterString ( pszBuff, 1, szCommandParam1 ) )
		{
			if ( STRINGCOMPAREI ( "true", szCommandParam1 ) )
			{
				CHATSERVER->SendChat ( pcUser, EChatColor::CHATCOLOR_Error, "> Event Girl Free: ON!" );
				SetEventGirlFree ( TRUE );
			}
			else if ( STRINGCOMPAREI ( "false", szCommandParam1 ) )
			{
				CHATSERVER->SendChat ( pcUser, EChatColor::CHATCOLOR_Error, "> Event Girl Free: OFF!" );
				SetEventGirlFree ( FALSE );
			}
			else
				CHATSERVER->SendChatEx ( pcUser, EChatColor::CHATCOLOR_Error, "> Usage: /event_girl <true|false> [%s]", EVENT_FREEEVENTGIRL == TRUE ? "ON" : "OFF" );
		}
		else
			CHATSERVER->SendChatEx ( pcUser, EChatColor::CHATCOLOR_Error, "> Usage: /event_girl <true|false> [%s]", EVENT_FREEEVENTGIRL == TRUE ? "ON" : "OFF" );

		return TRUE;
	}

	if ( iLen = COMMAND( "/event_Halloween", pszBuff ) )
	{
		if ( GetParameterString( pszBuff, 1, szCommandParam1 ) )
		{
			if ( STRINGCOMPAREI( szCommandParam1, "true" ) )
			{
				SetHalloweenEvent( TRUE );
				HALLOWEENHANDLER->SendEventStatusToAllUsers ();
				CHATSERVER->SendChat( pcUser, CHATCOLOR_Error, "> Halloween Event ON!" );
			}
			else if ( STRINGCOMPAREI( szCommandParam1, "false" ) )
			{
				SetHalloweenEvent( FALSE );
				HALLOWEENHANDLER->SendEventStatusToAllUsers ();
				CHATSERVER->SendChat( pcUser, CHATCOLOR_Error, "> Halloween Event OFF!" );
			}
			else
				CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> Use: /event_Halloween <true|false> [%s]", EVENT_HALLOWEEN ? "On" : "Off" );
		}
		else
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> Use: /event_Halloween <true|false> [%s]", EVENT_HALLOWEEN ? "On" : "Off" );

		return TRUE;
	}

	if ( iLen = COMMAND( "/event_MimicReload", pszBuff ) )
	{
		if ( !EVENTSERVER->SQLLoadMimicSpawnData() )
		{
			CHATSERVER->SendChatEx ( pcUser, EChatColor::CHATCOLOR_Error, "Mimic Event has load errors. Please check console" );
		}

		CHATSERVER->SendChatAllGM ( "GM> Mimic Event Reloaded" );

		return TRUE;
	}

	if ( iLen = COMMAND ( "/event_Mimic", pszBuff ) )
	{
		if ( GetParameterString ( pszBuff, 1, szCommandParam1 ) )
		{
			if ( STRINGCOMPAREI ( szCommandParam1, "true" ) )
			{
				SetMimicEvent ( TRUE );
				CHATSERVER->SendChatAllGM ( "GM> Mimic Event ON!" );
			}
			else if ( STRINGCOMPAREI ( szCommandParam1, "false" ) )
			{
				SetMimicEvent ( FALSE );
				CHATSERVER->SendChatAllGM ( "GM> Mimic Event OFF!" );
			}
			else
			{
				CHATSERVER->SendChatEx ( pcUser, CHATCOLOR_Error, "> Use: /event_Mimic <true|false> [Current: %s]", EVENT_MIMIC ? "On" : "Off" );
			}
		}
		else
		{
			CHATSERVER->SendChatEx ( pcUser, CHATCOLOR_Error, "> Use: /event_Mimic <true|false> [Current: %s]", EVENT_MIMIC ? "On" : "Off" );
		}

		return TRUE;
	}


	if ( iLen = COMMAND ( "/event_Christmas", pszBuff ) )
	{
		if ( GetParameterString ( pszBuff, 1, szCommandParam1 ) )
		{
			if ( STRINGCOMPAREI ( szCommandParam1, "true" ) )
			{
				SetChristmasEvent ( TRUE );
				CHATSERVER->SendChatAllGM ( "GM> Christmas Event ON!" );
			}
			else if ( STRINGCOMPAREI ( szCommandParam1, "false" ) )
			{
				SetChristmasEvent ( FALSE );
				CHATSERVER->SendChatAllGM ( "GM> Christmas Event OFF!" );
			}
			else
			{
				CHATSERVER->SendChatEx ( pcUser, CHATCOLOR_Error, "> Use: /event_Christmas <true|false> [Current: %s]", EVENT_CHRISTMAS ? "On" : "Off" );
			}
		}
		else
		{
			CHATSERVER->SendChatEx ( pcUser, CHATCOLOR_Error, "> Use: /event_Christmas <true|false> [Current: %s]", EVENT_CHRISTMAS ? "On" : "Off" );
		}

		return TRUE;
	}

	if (iLen = COMMAND("/event_Easter", pszBuff))
	{
		if (GetParameterString(pszBuff, 1, szCommandParam1))
		{
			if (STRINGCOMPAREI(szCommandParam1, "true"))
			{
				SetEasterEvent(TRUE);
				CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Easter Event ON!");
			}
			else if (STRINGCOMPAREI(szCommandParam1, "false"))
			{
				SetEasterEvent(FALSE);
				CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Easter Event OFF!");
			}
			else
			{
				CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> Usage: /event_Easter <true|false>");
			}
		}
		else
		{
			CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> Usage: /event_Easter <true|false>");
		}

		return TRUE;
	}


	if (iLen = COMMAND("/event_StarWars", pszBuff))
	{
		if (GetParameterString(pszBuff, 1, szCommandParam1))
		{
			if (STRINGCOMPAREI(szCommandParam1, "true"))
			{
				SetStarWarsEvent(TRUE);
				CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Star Wars Event ON!");
			}
			else if (STRINGCOMPAREI(szCommandParam1, "false"))
			{
				SetStarWarsEvent(FALSE);
				CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Star Wars Event OFF!");
			}
			else
			{
				CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> Usage: /event_StarWars <true|false>");
			}
		}
		else
		{
			CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> Usage: /event_StarWars <true|false>");
		}

		return TRUE;
	}

	if (iLen = COMMAND("/event_Bee", pszBuff))
	{
		if (GetParameterString(pszBuff, 1, szCommandParam1))
		{
			if (STRINGCOMPAREI(szCommandParam1, "true"))
			{
				SetBeeEvent(TRUE);
				CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Bee Event ON!");
			}
			else if (STRINGCOMPAREI(szCommandParam1, "false"))
			{
				SetBeeEvent(FALSE);
				CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Bee Event OFF!");
			}
			else
			{
				CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> Usage: /event_Bee <true|false>");
			}
		}
		else
		{
			CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> Usage: /event_Bee <true|false>");
		}

		return TRUE;
	}

	if (iLen = COMMAND("/event_Valentine", pszBuff))
	{
		if (GetParameterString(pszBuff, 1, szCommandParam1))
		{
			if (STRINGCOMPAREI(szCommandParam1, "true"))
			{
				SetValentineDayEvent(TRUE);

				CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Valentine Event ON!");
			}
			else if (STRINGCOMPAREI(szCommandParam1, "false"))
			{
				SetValentineDayEvent(FALSE);
				CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Valentine Event OFF!");
			}
			else
				CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> Use: /event_Valentine <true|false> [%s]", EVENT_VALENTINESDAY ? "On" : "Off");
		}
		else
		{
			CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> Use: /event_Valentine <true|false> [%s]", EVENT_VALENTINESDAY ? "On" : "Off");
		}

		return TRUE;
	}

	//For testing only!
	if (iLen = COMMAND("/event_HuntMinSpawnDist", pszBuff))
	{
		if (GAME_SERVER)
		{
			if (GetParameterString(pszBuff, 1, szCommandParam1))
			{
				//default is DISTANCE_UNIT_FAR (1228800)
				EVENT_HUNT_MIN_SPAWN_MIN_DIST = atoi(szCommandParam1);
				CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> Hunt Event Min Spawn Distance = %d", EVENT_HUNT_MIN_SPAWN_MIN_DIST);
			};
		}
	}

	if ( iLen = COMMAND( "/event_WantedMoriph", pszBuff ) )
	{
		if (GAME_SERVER)
		{
			if (GetParameterString(pszBuff, 1, szCommandParam1))
			{
				if (STRINGCOMPAREI(szCommandParam1, "reset"))
				{
					EVENTSERVER->ResetWantedMoriphEvent();
					CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Wanted Moriph Event Data cleared!");
				}

				//Only if needed. Titles are given out on event's end. See below
				else if ( STRINGCOMPAREI ( szCommandParam1, "titles" ) )
				{
					if ( EVENTSERVER->GiveWantedMoriphTitles () )
					{
						CHATSERVER->SendChatAllGM ( "GM> Wanted Moriph Kill-based titles given out!" );
					}
					else
					{
						CHATSERVER->SendChatAllGM ( "GM> Wanted Moriph Titles FAILED (Error)" );
					}
				}

				else if (STRINGCOMPAREI(szCommandParam1, "true"))
				{
					int iSpawnMax = -1;
					int iSpawnDelay = -1;

					//optional param
					if (GetParameterString(pszBuff, 2, szCommandParam2))
					{
						iSpawnMax = atoi(szCommandParam2);
					}

					//optional param
					if (GetParameterString(pszBuff, 3, szCommandParam3))
					{
						iSpawnDelay = atoi(szCommandParam3);
					}

					if (EVENT_WANTEDMORIPH_EVENT == FALSE)
						CHATSERVER->SendChatAllGM("GM> Wanted Moriph Event ON!");

					SetMoriphEvent(TRUE, iSpawnMax, iSpawnDelay);


					CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> Max Spawn = (%d), Spawn Delay Min = (%d)",
						EVENT_HUNT_MAX_SPAWN,
						EVENT_HUNT_SPAWN_DELAY_MIN);

					CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Use '/event_WantedMoriph true <spawn count> <spawn delay>' to adjust spawn parameters.");
					CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Use '/event_WantedMoriph reset' to reset kill stats");
					CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Use '/event_WantedMoriph titles' to give kill based titles");
				}
				else if (STRINGCOMPAREI(szCommandParam1, "false"))
				{
					if ( EVENT_WANTEDMORIPH_EVENT == TRUE )
					{
						CHATSERVER->SendChatAllGM ( "GM> Wanted Moriph Event OFF!" );
						CHATSERVER->SendChat ( pcUser, CHATCOLOR_Error, "Use '/event_WantedMoriph titles' to give kill based titles" );
					}

					SetMoriphEvent(FALSE);
					EVENTSERVER->StopHuntEvent();
				}
				else
				{
					CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> Usage: /event_WantedMoriph <true|false>. Status = [%s]", EVENT_WANTEDMORIPH_EVENT ? "On" : "Off");
				}
			}
			else
			{
				CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> Usage: /event_WantedMoriph <true|false>. Status =  [%s]", EVENT_WANTEDMORIPH_EVENT ? "On" : "Off");
			}

			return TRUE;
		}
	}



	if (iLen = COMMAND("/event_WantedWolf", pszBuff))
	{
		if (GAME_SERVER)
		{
			if (GetParameterString(pszBuff, 1, szCommandParam1))
			{
				if (STRINGCOMPAREI(szCommandParam1, "reset"))
				{
					EVENTSERVER->ResetWantedWolfEvent();
					CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Wolf Event Data cleared!");
				}

				//Only if needed. Titles are given out on event's end. See below
				else if ( STRINGCOMPAREI ( szCommandParam1, "titles" ) )
				{
					if ( EVENTSERVER->GiveWantedWolfTitles () )
					{
						CHATSERVER->SendChatAllGM ( "GM> Wanted Wolf Kill-based titles given out!" );
					}
					else
					{
						CHATSERVER->SendChatAllGM ( "GM> Wanted Wolf Titles FAILED (Error)" );
					}
				}

				else if (STRINGCOMPAREI(szCommandParam1, "true"))
				{
					int iSpawnMax = -1;
					int iSpawnDelay = -1;

					//optional param
					if (GetParameterString(pszBuff, 2, szCommandParam2))
					{
						iSpawnMax = atoi(szCommandParam2);
					}

					//optional param
					if (GetParameterString(pszBuff, 3, szCommandParam3))
					{
						iSpawnDelay = atoi(szCommandParam3);
					}

					if (EVENT_WANTEDWOLF_EVENT == FALSE)
						CHATSERVER->SendChatAllGM("GM> Wanted Wolf Event ON!");

					SetWolfEvent(TRUE, iSpawnMax, iSpawnDelay);


					CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> Max Spawn = (%d), Spawn Delay Min = (%d)",
						EVENT_HUNT_MAX_SPAWN,
						EVENT_HUNT_SPAWN_DELAY_MIN);

					CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Use '/event_WantedWolf true <spawn count> <spawn delay>' to adjust spawn parameters.");
					CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Use '/event_WantedWolf reset' to reset kill stats");
					CHATSERVER->SendChat ( pcUser, CHATCOLOR_Error, "Use '/event_WantedWolf titles' to give kill based titles" );
				}
				else if (STRINGCOMPAREI(szCommandParam1, "false"))
				{
					if ( EVENT_WANTEDWOLF_EVENT == TRUE )
					{
						CHATSERVER->SendChatAllGM ( "GM> Wanted Wolf Event OFF!" );
						CHATSERVER->SendChat ( pcUser, CHATCOLOR_Error, "Use '/event_WantedWolf titles' to give kill based titles" );
					}

					SetWolfEvent(FALSE);
					EVENTSERVER->StopHuntEvent();
				}
				else
				{
					CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> Usage: /event_WantedWolf <true|false>. Status = [%s]", EVENT_WANTEDWOLF_EVENT ? "On" : "Off");
				}
			}
			else
			{
				CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> Usage: /event_WantedWolf <true|false>. Status =  [%s]", EVENT_WANTEDWOLF_EVENT ? "On" : "Off");
			}

			return TRUE;
		}
	}

	if ( iLen = COMMAND( "/!DanceAll", pszBuff ) )
	{
		for ( int i = 0; i < PLAYERS_MAX; i++ )
		{
			if ( USERSDATA[i].pcSocketData && USERSDATA[i].iGameLevel == GAMELEVEL_None )
			{
				SENDPACKETBLANK( USERDATATOUSER( &USERSDATA[i] ), PKTHDR_PacketFun );
			}
		}
		return TRUE;
	}

	if ( iLen = COMMAND( "/BotCreate", pszBuff ) )
	{
		if ( pcUserData )
		{
			if ( GetParameterString ( pszBuff, 1, szCommandParam1 ) )
			{
				if ( BOTSERVER->CreateBot ( szCommandParam1, pcUserData->sPosition.iX, pcUserData->sPosition.iY, pcUserData->sPosition.iZ ) )
				{
					CHATSERVER->SendChatEx ( pcUser, CHATCOLOR_Error, "> Created Bot %s", szCommandParam1 );
					CHATSERVER->SendChatEx ( pcUser, CHATCOLOR_Error, "> Coordinates [X: %d Y: %d Z: %d]", pcUserData->sPosition.iX, pcUserData->sPosition.iY, pcUserData->sPosition.iZ );
				}
			}
		}
		return TRUE;
	}

	if ( iLen = COMMAND( "/BotDelete", pszBuff ) )
	{
		if ( GetParameterString( pszBuff, 1, szCommandParam1 ) )
		{
			if ( BOTSERVER->DeleteBot( szCommandParam1 ) )
			{
				CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> Deleted Bot %s", szCommandParam1 );
			}
			else
				CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> Bot %s is offline!", szCommandParam1 );
		}
		return TRUE;
	}

	if ( iLen = COMMAND( "/BotLHand", pszBuff ) )
	{
		if ( GetParameterString( pszBuff, 1, szCommandParam1 ) && GetParameterString( pszBuff, 2, szCommandParam2 ) )
		{
			if ( BOTSERVER->SetItemHand( 1, szCommandParam1, szCommandParam2 ) )
			{
				CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> %s LH Item: %s", szCommandParam1, szCommandParam2 );
			}
		}
		return TRUE;
	}

	if ( iLen = COMMAND( "/BotRHand", pszBuff ) )
	{
		if ( GetParameterString( pszBuff, 1, szCommandParam1 ) && GetParameterString( pszBuff, 2, szCommandParam2 ) )
		{
			if ( BOTSERVER->SetItemHand( 2, szCommandParam1, szCommandParam2 ) )
			{
				CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> %s RH Item: %s", szCommandParam1, szCommandParam2 );
			}
		}
		return TRUE;
	}

	if ( COMMAND( "/ServerCrash", pszBuff ) || COMMAND( "/!ServerCrash", pszBuff ) )
	{
		CHATSERVER->SendChatEx( pcUser, EChatColor::CHATCOLOR_Error, "> Crashing %s in 1 second...", GAME_SERVER ? "GameServer" : "LoginServer" );
		Sleep( 1000 );
		CRASH;
		return TRUE;
	}

	if ( iLen = COMMAND( "/BotBHand", pszBuff ) )
	{
		if ( GetParameterString( pszBuff, 1, szCommandParam1 ) && GetParameterString( pszBuff, 2, szCommandParam2 ) )
		{
			if ( BOTSERVER->SetItemHand( 3, szCommandParam1, szCommandParam2 ) )
			{
				CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> %s BH Item: %s", szCommandParam1, szCommandParam2 );
			}
		}
		return TRUE;
	}

	if ( iLen = COMMAND( "/BotSay", pszBuff ) )
	{
		if ( GetParameterString( pszBuff, 1, szCommandParam1 ) && GetParameterString( pszBuff, 2, szCommandParam2 ) )
		{
			if ( BOTSERVER->SetBotSay( szCommandParam1, szCommandParam2 ) )
			{
				CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> %s Bot says!", szCommandParam1 );
			}
		}
		return TRUE;
	}

	if ( iLen = COMMAND( "/TestItem", pszBuff ) )
	{

	}

	if ( iLen = COMMAND( "/!EditItemTest", pszBuff ) )
	{
		if ( GetParameterString( pszBuff, 1, szCommandParam1 ) )
		{
			DefinitionItem * ps = ITEMSERVER->FindItemDefByCode( szCommandParam1 );

			if ( ps )
			{
				DefinitionItem sd;
				CopyMemory( &sd, ps, sizeof( DefinitionItem ) );

				/*				STRINGCOPY( sd.sItem.szItemName, "Deadly Axe" );

				sd.sItem.sItemID.SetItemID( (EItemID)16849920 );

				sd.sItem.sDamage.sMin = 133;
				sd.sItem.sDamage.sMax = 148;
				sd.sItem.iAttackSpeed = 8;
				sd.sItem.iCritical = 21;
				sd.sItem.iAttackRating = 356;
				sd.sItem.fBlockRating = 13;
				sd.sItem.sIntegrity.sCur = 260;
				sd.sItem.sIntegrity.sMax = 260;
				sd.sItem.iLevel = 130;
				sd.sItem.iStrength = 255;
				sd.sItem.iTalent = 90;
				sd.sItem.iWeight = 110;
				sd.sItem.eSpecialization = CLASSFLAG_Fighter;
				sd.sItem.sSpecData.iSpecCritical = 5;
				sd.sItem.sSpecData.iSpecAttackSpeed = 0;
				sd.sItem.sSpecData.iSpecAttackRatingDiv = 1;
				sd.sItem.sSpecData.iSpecAttackPowerDiv = 1;
				sd.sItem.sSpecData.iSpecAttackPowerDivMin = 1;
				sd.sItem.fBlockRating = 13;

				*/
				sd.sItem.iSalePrice = 1;

				ITEMSERVER->ValidateItemTime( &sd.sItem );

				sd.sItem.iBackupChk = 0;
				sd.sItem.iBackupKey = 0;
				sd.sItem.tTime = 0;

				PacketBuyNPCShop s;
				s.iLength = sizeof( PacketBuyNPCShop );
				s.iHeader = PKTHDR_BuyItemNPCShop;
				s.iCount = 1;

				ItemData * p = ITEMSERVER->FindItemPointerTable( szCommandParam1 );
				if ( p )
				{
					CopyMemory( &s.sItemData, p, sizeof( ItemData ) );
					CopyMemory( &s.sItemData.sItem, &sd.sItem, sizeof( Item ) );

					s.sItemData.bValid = TRUE;

					FILE * fp = NULL;
					fopen_s( &fp, "Item.dat", "wb" );
					fwrite( &s, s.iLength, 1, fp );
					fclose( fp );

					CALL_WITH_ARG2( 0x0056A260, (DWORD)pcUserData, (DWORD)&s );

					CHATSERVER->SendChat( pcUser, CHATCOLOR_Error, "> Edited!!" );
				}
			}
		}
		return TRUE;
	}


	// BC
	if ( iLen = COMMAND( "/SetOwnerBC", pszBuff ) )
	{
		if ( GetParameterString( pszBuff, 1, szCommandParam1 ) )
		{
			int iClanID = atoi( szCommandParam1 );
			if ( iClanID != 0 )
			{
				BLESSCASTLESERVER->NetSendBlessCastleOwnerClanID( iClanID );
				CHATSERVER->SendChatAllGM( "GM> New BC Owner: %d" );
			}
			else
				CHATSERVER->SendChat( pcUser, CHATCOLOR_Error, "> Invalid Clan ID" );
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/ClearOwnerBC", pszBuff ) )
	{
		BLESSCASTLESERVER->NetSendBlessCastleOwnerClanID( 0 );
		CHATSERVER->SendChatAllGM( "GM> BC Owner cleared ");
		return TRUE;
	}
	if ( iLen = COMMAND( "/MonAnim", pszBuff ) )
	{
		if ( GetParameterString( pszBuff, 1, szCommandParam1 ) && GetParameterString( pszBuff, 2, szCommandParam2 ) )
		{
			UnitData * pcUnitData = UNITSERVER->CreateUnitEnemy( szCommandParam1, pcUserData->sPosition.iX, pcUserData->sPosition.iY, pcUserData->sPosition.iZ );
			if ( pcUnitData )
			{
				UNITSERVER->SetMotionFromCode( pcUnitData, atoi( szCommandParam2 ) );
				CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> Anim 0x%X Send! ", atoi( szCommandParam2 ) );
			}
			else
				CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> Error: Monster '%s' not exist!", szCommandParam1 );
		}
		CHATSERVER->SendChat( pcUser, CHATCOLOR_Error, "> Done! " );
		return TRUE;
	}

	// Move
	if ( COMMAND( "/WarpAll", pszBuff ) )
	{
		for ( int i = 0; i < PLAYERS_MAX; i++ )
		{
			if ( USERSDATA[i].pcSocketData && USERSDATA[i].iID )
			{
				PacketWarpGateField sPacket;
				sPacket.iLength = sizeof( PacketWarpGateField );
				sPacket.iHeader = PKTHDR_WarpGateField;
				sPacket.iX = (-51993);
				sPacket.iZ = (-4493732);
				sPacket.iStage = MAPID_RicartenTown;
				SENDPACKET( USERDATATOUSER(&USERSDATA[i]), &sPacket );
			}
		}
		CHATSERVER->SendChat( pcUser, CHATCOLOR_Error, "> All Users Warped to Ricarten Town!" );
		return TRUE;
	}

	if (COMMAND("/WarpGarden", pszBuff))
	{
		for (int i = 0; i < PLAYERS_MAX; i++)
		{
			if (USERSDATA[i].pcSocketData && USERSDATA[i].iID)
			{
				PacketWarpGateField sPacket;
				sPacket.iLength = sizeof(PacketWarpGateField);
				sPacket.iHeader = PKTHDR_WarpGateField;
				sPacket.iX = (1225087);
				sPacket.iZ = (-2793065);
				sPacket.iStage = MAPID_GardenOfFreedom;
				SENDPACKET(USERDATATOUSER(&USERSDATA[i]), &sPacket);
			}
		}
		CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> All Users Warped to Gardens of Freedom!");
		return TRUE;
	}

	if (COMMAND("/WarpEvent", pszBuff))
	{
		for (int i = 0; i < PLAYERS_MAX; i++)
		{
			if (USERSDATA[i].pcSocketData && USERSDATA[i].iID)
			{
				PacketWarpGateField sPacket;
				sPacket.iLength = sizeof(PacketWarpGateField);
				sPacket.iHeader = PKTHDR_WarpGateField;
				sPacket.iX = (-9014826);
				sPacket.iZ = (-2783979);
				sPacket.iStage = MAPID_MysteryForest1;
				SENDPACKET(USERDATATOUSER(&USERSDATA[i]), &sPacket);
			}
		}
		CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> All Users Warped to The Mystery Event!");
		return TRUE;
	}

	if ( COMMAND( "/QuestArenaT5", pszBuff ) )
	{
		QUESTARENAHANDLER->EnterArenaT5Quest( USERDATATOUSER( pcUserData ) );
		CHATSERVER->SendChat( pcUser, CHATCOLOR_Error, "> T5 Quest Arena" );
		return TRUE;
	}

	if ( COMMAND( "/QuestT5Cry", pszBuff ) )
	{
		User * pcUser = USERDATATOUSER( pcUserData );
		pcUser->uQuestT5ID = QUESTID_CryMeARiver;
		pcUser->sQuestT5ProgressValue[0] = 0;
		CHATSERVER->SendChat( pcUser, CHATCOLOR_Error, "> T5 Quest Cry me a River!" );
		return TRUE;
	}

	if ( COMMAND( "/QuestT5TestID", pszBuff ) )
	{
		if ( pcUser  )
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> T5 Quest [%d][%d][%d]", pcUser->uQuestT5ID, pcUser->sQuestT5ProgressValue[0], pcUser->sQuestT5ProgressValue[1] );

		return TRUE;
	}
	if ( COMMAND( "/!QuestT5TestID", pszBuff ) )
	{
		if ( pcUser )
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> T5 Quest [%d][%d][%d]", pcUser->uQuestT5ID, pcUser->sQuestT5ProgressValue[0], pcUser->sQuestT5ProgressValue[1] );

		return TRUE;
	}


	if ( iLen = COMMAND( "/!tradechat_free", pszBuff ) )
	{
		if ( GetParameterString( pszBuff, 1, szCommandParam1 ) )
		{
			if ( STRINGCOMPAREI( "true", szCommandParam1 ) )
			{
				FREE_TRADECHAT = TRUE;
				CHATSERVER->SendChat( pcUser, CHATCOLOR_Error, "> Free Trade Chat On!" );
			}
			else if ( STRINGCOMPAREI( "false", szCommandParam1 ) )
			{
				FREE_TRADECHAT = FALSE;
				CHATSERVER->SendChat( pcUser, CHATCOLOR_Error, "> Free Trade Chat Off!" );
			}
			else
				CHATSERVER->SendChat( pcUser, CHATCOLOR_Error, "> Use: /!tradechat_free <true|false>" );
		}
		else
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> Use: /!tradechat_free <true|false> [%s]", FREE_TRADECHAT ? "On" : "Off" );

		return TRUE;
	}

	if ( iLen = COMMAND( "/!wholoves", pszBuff ) )
	{
		CHATSERVER->SendChatAll( CHATCOLOR_Global, "Server> Who Love me? *_*" );
		for ( int i = 0; i < PLAYERS_MAX; i++ )
		{
			User * pcU = USERDATATOUSER( &USERSDATA[i] );
			if ( USERSDATA[i].pcSocketData && USERSDATA[i].iGameLevel == GAMELEVEL_None && pcU && pcU->bNetServer == FALSE )
			{
				CHATSERVER->SendChatAllEx( CHATCOLOR_Trade, "[%c]%s: P> I love you, %s <3!", GSERVER->GetServerName( USERSDATA[i].bServerIndexID )[0], CHARACTERSERVER->GetCharacterName( &USERSDATA[i] ), GAME_NAME );
			}
		}
		return TRUE;
	}

	/*if ( COMMAND( "/NetServerDC", pszBuff ) || COMMAND( "/!NetServerDC", pszBuff ) )
	{
	NETSERVER->DisconnectAll();
	CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> [%s] Net Servers Disconnected!", GAME_SERVER ? "Game Server" : "Login Server" );
	return TRUE;
	}*/
	if ( iLen = COMMAND( "/testmsgbox", pszBuff ) )
	{
		CHATSERVER->SendTitleBox( pcUserData, "> VSF" );
		return TRUE;
	}

	if ( iLen = COMMAND( "/test_roll_bosskill_item", pszBuff ) )
	{
		if ( GetParameterString( pszBuff, 1, szCommandParam1 ) )
		{
			EItemRarity eItemRarity = EItemRarity::NONE;
			EItemSource eItemSource = EItemSource::BossKill;

			if ( GetParameterString( pszBuff, 2, szCommandParam2 ) )
			{
				auto eRarityOverride = (EItemRarity)atoi( szCommandParam2 );
				if ( eRarityOverride >= EItemRarity::NONE || eRarityOverride <= EItemRarity::LEGENDARY )
					eItemRarity = eRarityOverride;
			}

			if ( ROLLDICEHANDLER->TestItemDiceRoll( pcUser, szCommandParam1, eItemSource, eItemRarity ) )
			{
				CHATSERVER->SendChat( pcUser, EChatColor::CHATCOLOR_Normal, "Item added to roll window for testing purposes" );
			}
			else
			{
				CHATSERVER->SendChatEx( pcUser, EChatColor::CHATCOLOR_Error, "Error - invalid item: %s", szCommandParam1 );
			}

		}
		else
		{
			CHATSERVER->SendChat( pcUser, EChatColor::CHATCOLOR_Error, "Expecting 1 parameter" );
		}

		return TRUE;
	}


	if ( iLen = COMMAND( "/spymember", pszBuff ) )
	{
		if ( GetParameterString( pszBuff, 1, szCommandParam1 ) )
		{
			UserData * pcUserSpyoner = USERSERVER->GetUserdata( szCommandParam1 );
			User * pcU = USERDATATOUSER( pcUserSpyoner );
			if ( pcUserSpyoner )
			{
				CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> Spying %s ", pcUserSpyoner->sCharacterData.szName );
				pcU->pcUserDataSpyoner = pcUserData;
			}
			else
				CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> %s can not found!", szCommandParam1 );
		}
		return TRUE;
	}

	if ( iLen = COMMAND( "/notspymember", pszBuff ) )
	{
		if ( GetParameterString( pszBuff, 1, szCommandParam1 ) )
		{
			UserData * pcUserSpyoner = USERSERVER->GetUserdata( szCommandParam1 );
			User * pcU = USERDATATOUSER( pcUserSpyoner );
			if ( pcUserSpyoner )
			{
				CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> Not Spying %s ", pcUserSpyoner->sCharacterData.szName );
				pcU->pcUserDataSpyoner = NULL;
			}
			else
				CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> %s can not found!", szCommandParam1 );
		}
		return TRUE;
	}

	if ( iLen = COMMAND("/!giveexp", pszBuff))
	{
		if (GetParameterString(pszBuff, 1, szCommandParam1))
		{
			SQLConnection* pcDB = SQLCONNECTION(DATABASEID_UserDB_LocalServer_CharInfo);

			INT64 iExp = atoll(szCommandParam1);

			CHARACTERSERVER->SQLGiveEXP (pcUser, iExp);
			CHARACTERSERVER->GiveEXP( pcUser, iExp);
		}
	}

	if ( iLen = COMMAND( "/!levelup", pszBuff ) )
	{
		if ( GetParameterString( pszBuff, 1, szCommandParam1 ) )
		{
			if ( atoi( szCommandParam1 ) > pcUserData->sCharacterData.iLevel )
			{
				INT64 iExp		= CHARACTERSERVER->GetExpFromLevel( atoi( szCommandParam1 ) );
				INT64 iExpOld	= CHARACTERSERVER->GetExp( pcUserData );

				CHARACTERSERVER->SQLSetEXP ( USERDATATOUSER ( pcUserData ), iExp );
				CHARACTERSERVER->GiveEXP( USERDATATOUSER( pcUserData ), iExp - iExpOld );
				CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> New Level Exp (%I64d)[%d]", iExp, atoi( szCommandParam1 ) );
			}
			else
				CHATSERVER->SendChat( pcUser, CHATCOLOR_Error, "> Level must be major than your level!" );
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/shutdowncancel", pszBuff ) )
	{
		if ( *(UINT*)0x7AC9F98 )
		{
			*(UINT*)0x7AC9D20 = 0;
			*(UINT*)0x7AC9D28 = 0;
			*(UINT*)0x7AC9D2C = 0;
			TerminateThread( (HANDLE)*(UINT*)0x7AC9F98, 0 );
			CHATSERVER->SendChatAll( CHATCOLOR_Global, "Server Shutdown> Shutdown Cancelled!" );
		}
	}
	if ( iLen = COMMAND( "/set_spawn", pszBuff ) )
	{
		if ( GetParameterString( pszBuff, 1, szCommandParam1 ) )
		{
			MAPSERVER->SetFlagPoint( pcUserData, szCommandParam1 );
		}
		else
		{
			MAPSERVER->SetFlagPoint( pcUserData, Util::CurrentDate().c_str() );
		}

		return TRUE;
	}
	if ( iLen = COMMAND( "/setversion", pszBuff ) )
	{
		if ( GetParameterString( pszBuff, 1, szCommandParam1 ) )
		{
			SetVersion( atoi( szCommandParam1 ) );
			NETSERVER->SendCommandToLoginServer ( PKTHDR_NetSetVersion, atoi ( szCommandParam1 ) );
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/GetBossCrystal", pszBuff ) )
	{
		struct CrystalOldData
		{
			char * p;
			int i;
			int iRandom;
		};

		CrystalOldData * pda = (CrystalOldData*)0x8B76510;

		int i = 0;
		while ( true )
		{
			CrystalOldData * pd = pda + i;
			if ( pd->p == NULL || pd->p[0] == 0 )
				break;

			LOGERROR( "{ ITEMID_HopyCrystal, %d, \"%s\" },", pd->iRandom, pd->p );

			i++;
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/ReloadCoinShop", pszBuff ) )
	{
		NETSERVER->SendCommandToLoginServer ( PKTHDR_NetReloadCoinShop );
		return TRUE;
	}
	if ( iLen = COMMAND( "/tempitemget", pszBuff ) )
	{
		PacketSimple sPacket;
		sPacket.iLength = sizeof( PacketSimple );
		sPacket.iHeader = PKTHDR_GetItemData;
		sPacket.iUnk = 0;
		SENDPACKET( USERDATATOUSER(pcUserData), &sPacket );
		return TRUE;
	}
	if ( iLen = COMMAND( "/tempitemset", pszBuff ) )
	{
		ITEMSERVER->ReformItem( &pcUser->sTempItem.sItem );
		ITEMSERVER->SendItemData( pcUserData, &pcUser->sTempItem.sItem.sItem );
		ZeroMemory( &pcUser->sTempItem, sizeof( PacketItemData ) );
		CHATSERVER->SendChat( pcUser, CHATCOLOR_Error, "> Item Sent!" );
		return TRUE;
	}
	if ( iLen = COMMAND( "/tempitem_dmg", pszBuff ) )
	{
		if ( pcUser->sTempItem.iHeader )
		{
			GetParameterString( pszBuff, 1, szCommandParam1 );
			GetParameterString( pszBuff, 2, szCommandParam2 );
			pcUser->sTempItem.sItem.sItem.sDamage.sMin = atoi( szCommandParam1 );
			pcUser->sTempItem.sItem.sItem.sDamage.sMax = atoi( szCommandParam2 );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> New Damage ( %d-%d )", pcUser->sTempItem.sItem.sItem.sDamage.sMin, pcUser->sTempItem.sItem.sItem.sDamage.sMax );
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/tempitem_age", pszBuff ) )
	{
		if ( pcUser->sTempItem.iHeader )
		{
			if ( GetParameterString( pszBuff, 1, szCommandParam1 ) )
			{
				int iAge = atoi( szCommandParam1 );
				for ( int i = 0; i < iAge; i++ )
				{
					ITEMSERVER->SetAgingItem( &pcUser->sTempItem.sItem );
				}
				ITEMSERVER->SaveItemDataToDatabase( &pcUser->sTempItem.sItem.sItem );
				CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> New Age ( %d )", pcUser->sTempItem.sItem.sItem.sAgeLevel+1 );
			}
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/tempitem_name", pszBuff ) )
	{
		if ( pcUser->sTempItem.iHeader )
		{
			GetParameterString( pszBuff, 1, szCommandParam1 );
			STRINGCOPY( pcUser->sTempItem.sItem.sItem.szItemName, szCommandParam1 );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> New Name ( %s )", pcUser->sTempItem.sItem.sItem.szItemName );
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/tempitem_atkspeed", pszBuff ) )
	{
		if ( pcUser->sTempItem.iHeader )
		{
			GetParameterString( pszBuff, 1, szCommandParam1 );
			pcUser->sTempItem.sItem.sItem.iAttackSpeed = atoi( szCommandParam1 );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> New Attack Speed ( %d )", pcUser->sTempItem.sItem.sItem.iAttackSpeed );
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/tempitem_socket", pszBuff ) )
	{
		if ( pcUser->sTempItem.iHeader )
		{
			GetParameterString( pszBuff, 1, szCommandParam1 );
			GetParameterString( pszBuff, 2, szCommandParam2 );
			GetParameterString( pszBuff, 3, szCommandParam3 );
			GetParameterString( pszBuff, 4, szCommandParam4 );

			pcUser->sTempItem.sItem.sItem.sSocketData[atoi(szCommandParam1)].eSocketType = (ESocketType)atoi( szCommandParam2 );
			pcUser->sTempItem.sItem.sItem.sSocketData[atoi(szCommandParam1)].eGemType = (EGemType)atoi( szCommandParam3 );
			pcUser->sTempItem.sItem.sItem.sSocketData[atoi(szCommandParam1)].eGemItem = (EGemItem)atoi( szCommandParam4 );

			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> New Socket Data [%d]( %d %d %d )", atoi(szCommandParam1), pcUser->sTempItem.sItem.sItem.sSocketData[atoi(szCommandParam1)].eSocketType, pcUser->sTempItem.sItem.sItem.sSocketData[atoi(szCommandParam1)].eGemType, pcUser->sTempItem.sItem.sItem.sSocketData[atoi(szCommandParam1)].eGemItem );
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/tempitem_crit", pszBuff ) )
	{
		if ( pcUser->sTempItem.iHeader )
		{
			GetParameterString( pszBuff, 1, szCommandParam1 );
			pcUser->sTempItem.sItem.sItem.iCritical = atoi( szCommandParam1 );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> New Critical ( %d )", pcUser->sTempItem.sItem.sItem.iCritical );
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/tempitem_mixflag", pszBuff ) )
	{
		if ( pcUser->sTempItem.iHeader )
		{
			GetParameterString( pszBuff, 1, szCommandParam1 );
			pcUser->sTempItem.sItem.sItem.eMixTypeName = (EMixTypeName)atoi( szCommandParam1 );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> New Mix Flag ( %d )", pcUser->sTempItem.sItem.sItem.eMixTypeName );
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/tempitem_atkrtg", pszBuff ) )
	{
		if ( pcUser->sTempItem.iHeader )
		{
			GetParameterString( pszBuff, 1, szCommandParam1 );
			pcUser->sTempItem.sItem.sItem.iAttackRating = atoi( szCommandParam1 );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> New Attack Rating ( %d )", pcUser->sTempItem.sItem.sItem.iAttackRating );
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/tempitem_def", pszBuff ) )
	{
		if ( pcUser->sTempItem.iHeader )
		{
			GetParameterString( pszBuff, 1, szCommandParam1 );
			pcUser->sTempItem.sItem.sItem.iDefenseRating = atoi( szCommandParam1 );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> New Defense ( %d )", pcUser->sTempItem.sItem.sItem.iDefenseRating );
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/tempitem_block", pszBuff ) )
	{
		if ( pcUser->sTempItem.iHeader )
		{
			GetParameterString( pszBuff, 1, szCommandParam1 );
			pcUser->sTempItem.sItem.sItem.fBlockRating = (float)atof( szCommandParam1 );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> New Block ( %.01f )", pcUser->sTempItem.sItem.sItem.fBlockRating );
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/tempitem_abs", pszBuff ) )
	{
		if ( pcUser->sTempItem.iHeader )
		{
			GetParameterString( pszBuff, 1, szCommandParam1 );
			pcUser->sTempItem.sItem.sItem.fAbsorbRating = (float)atof( szCommandParam1 );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> New Absorption ( %.01f )", pcUser->sTempItem.sItem.sItem.fAbsorbRating );
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/tempitem_changespec", pszBuff ) )
	{
		if ( pcUser->sTempItem.iHeader )
		{
			GetParameterString( pszBuff, 1, szCommandParam1 );
			pcUser->sTempItem.sItem.sItem.eSpecialization = CharacterClassToClassFlag( (ECharacterClass)atoi( szCommandParam1 ) );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> New Spec ( %s )", GetCharacterClassStringByFlag( (EClassFlag)pcUser->sTempItem.sItem.sItem.eSpecialization ) );
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/tempitem_strength", pszBuff ) )
	{
		if ( pcUser->sTempItem.iHeader )
		{
			GetParameterString( pszBuff, 1, szCommandParam1 );
			pcUser->sTempItem.sItem.sItem.iStrength = atoi( szCommandParam1 );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> New Strength ( %d )", pcUser->sTempItem.sItem.sItem.iStrength );
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/tempitem_level", pszBuff ) )
	{
		if ( pcUser->sTempItem.iHeader )
		{
			GetParameterString( pszBuff, 1, szCommandParam1 );
			pcUser->sTempItem.sItem.sItem.iLevel = atoi( szCommandParam1 );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> New Level ( %d )", pcUser->sTempItem.sItem.sItem.iLevel );
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/tempitem_spirit", pszBuff ) )
	{
		if ( pcUser->sTempItem.iHeader )
		{
			GetParameterString( pszBuff, 1, szCommandParam1 );
			pcUser->sTempItem.sItem.sItem.iSpirit = atoi( szCommandParam1 );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> New Spirit ( %d )", pcUser->sTempItem.sItem.sItem.iSpirit );
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/tempitem_talent", pszBuff ) )
	{
		if ( pcUser->sTempItem.iHeader )
		{
			GetParameterString( pszBuff, 1, szCommandParam1 );
			pcUser->sTempItem.sItem.sItem.iTalent = atoi( szCommandParam1 );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> New Talent ( %d )", pcUser->sTempItem.sItem.sItem.iTalent );
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/tempitem_agility", pszBuff ) )
	{
		if ( pcUser->sTempItem.iHeader )
		{
			GetParameterString( pszBuff, 1, szCommandParam1 );
			pcUser->sTempItem.sItem.sItem.iAgility = atoi( szCommandParam1 );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> New Agility ( %d )", pcUser->sTempItem.sItem.sItem.iAgility );
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/tempitem_health", pszBuff ) )
	{
		if ( pcUser->sTempItem.iHeader )
		{
			GetParameterString( pszBuff, 1, szCommandParam1 );
			pcUser->sTempItem.sItem.sItem.iHealth = atoi( szCommandParam1 );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> New Health ( %d )", pcUser->sTempItem.sItem.sItem.iHealth );
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/tempitem_specdef", pszBuff ) )
	{
		if ( pcUser->sTempItem.iHeader )
		{
			GetParameterString( pszBuff, 1, szCommandParam1 );
			pcUser->sTempItem.sItem.sItem.sSpecData.iSpecDefenseRating = atoi( szCommandParam1 );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> New Defense Spec ( %d )", pcUser->sTempItem.sItem.sItem.sSpecData.iSpecDefenseRating );
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/tempitem_specblock", pszBuff ) )
	{
		if ( pcUser->sTempItem.iHeader )
		{
			GetParameterString( pszBuff, 1, szCommandParam1 );
			pcUser->sTempItem.sItem.sItem.sSpecData.fSpecBlockRating = (float)atof( szCommandParam1 );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> New Block Spec ( %.01f )", pcUser->sTempItem.sItem.sItem.sSpecData.fSpecBlockRating );
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/tempitem_specabs", pszBuff ) )
	{
		if ( pcUser->sTempItem.iHeader )
		{
			GetParameterString( pszBuff, 1, szCommandParam1 );
			pcUser->sTempItem.sItem.sItem.sSpecData.fSpecAbsorbRating = (float)atof( szCommandParam1 );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> New Absorb Spec ( %.01f )", pcUser->sTempItem.sItem.sItem.sSpecData.fSpecAbsorbRating );
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/tempitem_spechpregen", pszBuff ) )
	{
		if ( pcUser->sTempItem.iHeader )
		{
			GetParameterString( pszBuff, 1, szCommandParam1 );
			pcUser->sTempItem.sItem.sItem.sSpecData.fSpecHPRegen = (float)atof( szCommandParam1 );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> New HP Regen Spec ( %.01f )", pcUser->sTempItem.sItem.sItem.sSpecData.fSpecHPRegen );
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/tempitem_specmovspeed", pszBuff ) )
	{
		if ( pcUser->sTempItem.iHeader )
		{
			GetParameterString( pszBuff, 1, szCommandParam1 );
			pcUser->sTempItem.sItem.sItem.sSpecData.fSpecMovementSpeed = (float)atof( szCommandParam1 );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> New Movement Speed Spec ( %.01f )", pcUser->sTempItem.sItem.sItem.sSpecData.fSpecMovementSpeed );
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/tempitem_specmpregen", pszBuff ) )
	{
		if ( pcUser->sTempItem.iHeader )
		{
			GetParameterString( pszBuff, 1, szCommandParam1 );
			pcUser->sTempItem.sItem.sItem.sSpecData.fSpecMPRegen = (float)atof( szCommandParam1 );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> New MP Regen Spec ( %.01f )", pcUser->sTempItem.sItem.sItem.sSpecData.fSpecMPRegen );
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/tempitem_specspregen", pszBuff ) )
	{
		if ( pcUser->sTempItem.iHeader )
		{
			GetParameterString( pszBuff, 1, szCommandParam1 );
			pcUser->sTempItem.sItem.sItem.sSpecData.fSpecSPRegen = (float)atof( szCommandParam1 );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> New SP Regen Spec ( %.01f )", pcUser->sTempItem.sItem.sItem.sSpecData.fSpecSPRegen );
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/tempitem_speccritical", pszBuff ) )
	{
		if ( pcUser->sTempItem.iHeader )
		{
			GetParameterString( pszBuff, 1, szCommandParam1 );
			pcUser->sTempItem.sItem.sItem.sSpecData.iSpecCritical = atoi( szCommandParam1 );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> New Critical Spec ( %d )", pcUser->sTempItem.sItem.sItem.sSpecData.iSpecCritical );
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/tempitem_specdivatkpow", pszBuff ) )
	{
		if ( pcUser->sTempItem.iHeader )
		{
			GetParameterString( pszBuff, 1, szCommandParam1 );
			pcUser->sTempItem.sItem.sItem.sSpecData.iSpecAttackPowerDiv = atoi( szCommandParam1 );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> New Attack Power Div Spec ( %d )", pcUser->sTempItem.sItem.sItem.sSpecData.iSpecAttackPowerDiv );
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/tempitem_specdivatkpowmin", pszBuff ) )
	{
		if ( pcUser->sTempItem.iHeader )
		{
			GetParameterString( pszBuff, 1, szCommandParam1 );
			pcUser->sTempItem.sItem.sItem.sSpecData.iSpecAttackPowerDivMin = atoi( szCommandParam1 );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> New Attack Power Div Min Spec ( %d )", pcUser->sTempItem.sItem.sItem.sSpecData.iSpecAttackPowerDivMin );
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/tempitem_specdivatkrtg", pszBuff ) )
	{
		if ( pcUser->sTempItem.iHeader )
		{
			GetParameterString( pszBuff, 1, szCommandParam1 );
			pcUser->sTempItem.sItem.sItem.sSpecData.iSpecAttackRatingDiv = atoi( szCommandParam1 );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> New Attack Rating Div Spec ( %d )", pcUser->sTempItem.sItem.sItem.sSpecData.iSpecAttackRatingDiv );
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/tempitem_specatkspeed", pszBuff ) )
	{
		if ( pcUser->sTempItem.iHeader )
		{
			GetParameterString( pszBuff, 1, szCommandParam1 );
			pcUser->sTempItem.sItem.sItem.sSpecData.iSpecAttackSpeed = atoi( szCommandParam1 );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> New Attack Speed Spec ( %d )", pcUser->sTempItem.sItem.sItem.sSpecData.iSpecAttackSpeed );
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/tempitem_specatkspeed", pszBuff ) )
	{
		if ( pcUser->sTempItem.iHeader )
		{
			GetParameterString( pszBuff, 1, szCommandParam1 );
			pcUser->sTempItem.sItem.sItem.sSpecData.iSpecAttackSpeed = atoi( szCommandParam1 );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> New Attack Speed Spec ( %d )", pcUser->sTempItem.sItem.sItem.sSpecData.iSpecAttackSpeed );
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/tempitem_specatkrange", pszBuff ) )
	{
		if ( pcUser->sTempItem.iHeader )
		{
			GetParameterString( pszBuff, 1, szCommandParam1 );
			pcUser->sTempItem.sItem.sItem.sSpecData.iSpecAttackRange = atoi( szCommandParam1 );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> New Attack Range Spec ( %d )", pcUser->sTempItem.sItem.sItem.sSpecData.iSpecAttackRange );
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/tempitem_specaddhpdiv", pszBuff ) )
	{
		if ( pcUser->sTempItem.iHeader )
		{
			GetParameterString( pszBuff, 1, szCommandParam1 );
			pcUser->sTempItem.sItem.sItem.sSpecData.iSpecAddHPDiv = atoi( szCommandParam1 );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> New Add HP Div Spec ( %d )", pcUser->sTempItem.sItem.sItem.sSpecData.iSpecAddHPDiv );
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/tempitem_specaddmpdiv", pszBuff ) )
	{
		if ( pcUser->sTempItem.iHeader )
		{
			GetParameterString( pszBuff, 1, szCommandParam1 );
			pcUser->sTempItem.sItem.sItem.sSpecData.iSpecAddMPDiv = atoi( szCommandParam1 );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> New Add MP Div Spec ( %d )", pcUser->sTempItem.sItem.sItem.sSpecData.iSpecAddMPDiv );
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/tempitem_addhp", pszBuff ) )
	{
		if ( pcUser->sTempItem.iHeader )
		{
			GetParameterString( pszBuff, 1, szCommandParam1 );
			pcUser->sTempItem.sItem.sItem.fAddHP = (float)atof( szCommandParam1 );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> New Add HP ( %.01f )", pcUser->sTempItem.sItem.sItem.fAddHP );
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/tempitem_addmp", pszBuff ) )
	{
		if ( pcUser->sTempItem.iHeader )
		{
			GetParameterString( pszBuff, 1, szCommandParam1 );
			pcUser->sTempItem.sItem.sItem.fAddMP = (float)atof( szCommandParam1 );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> New Add MP ( %.01f )", pcUser->sTempItem.sItem.sItem.fAddMP );
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/tempitem_addsp", pszBuff ) )
	{
		if ( pcUser->sTempItem.iHeader )
		{
			GetParameterString( pszBuff, 1, szCommandParam1 );
			pcUser->sTempItem.sItem.sItem.fAddSP = (float)atof( szCommandParam1 );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> New Add SP ( %.01f )", pcUser->sTempItem.sItem.sItem.fAddSP );
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/tempitem_hpregen", pszBuff ) )
	{
		if ( pcUser->sTempItem.iHeader )
		{
			GetParameterString( pszBuff, 1, szCommandParam1 );
			pcUser->sTempItem.sItem.sItem.fHPRegen = (float)atof( szCommandParam1 );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> New HP Regen ( %.01f )", pcUser->sTempItem.sItem.sItem.fHPRegen );
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/tempitem_mpregen", pszBuff ) )
	{
		if ( pcUser->sTempItem.iHeader )
		{
			GetParameterString( pszBuff, 1, szCommandParam1 );
			pcUser->sTempItem.sItem.sItem.fMPRegen = (float)atof( szCommandParam1 );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> New MP Regen ( %.01f )", pcUser->sTempItem.sItem.sItem.fMPRegen );
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/tempitem_spregen", pszBuff ) )
	{
		if ( pcUser->sTempItem.iHeader )
		{
			GetParameterString( pszBuff, 1, szCommandParam1 );
			pcUser->sTempItem.sItem.sItem.fSPRegen = (float)atof( szCommandParam1 );
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> New SP Regen ( %.01f )", pcUser->sTempItem.sItem.sItem.fSPRegen );
		}
		return TRUE;
	}

	if ( iLen = COMMAND( "/getitemspec", pszBuff ) )
	{
		GetParameterString( pszBuff, 1, szCommandParam1 );
		GetParameterString( pszBuff, 2, szCommandParam2 );
		GetParameterString( pszBuff, 3, szCommandParam3 );

		// Create Item
		{
			ItemData * psItemData = ITEMSERVER->FindItemPointerTable( szCommandParam1 );
			if ( psItemData )
			{
				DefinitionItem * psDefItem = ITEMSERVER->FindItemDefByCode( psItemData->sBaseItemID.ToItemID() );
				if ( psDefItem )
				{
					ItemData sItemData;
					ITEMSERVER->CreateItem( &sItemData.sItem, psDefItem, EItemSource::GameMaster, atoi( szCommandParam2 ) );
					if ( atoi( szCommandParam3 ) > 0 )
					{
						if ( sItemData.sItem.sSpecData.iSpecAttackRatingDiv )
							sItemData.sItem.sSpecData.iSpecAttackRatingDiv = atoi( szCommandParam3 );

						ITEMSERVER->ReformItem( &sItemData );
					}
					ITEMSERVER->SendItemData( pcUserData, &sItemData.sItem );
					CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> '%s' Created at Spec %d LVATK_R/%d", szCommandParam1, atoi( szCommandParam2 ), sItemData.sItem.sSpecData.iSpecAttackRatingDiv );
				}
			}
		}
		return TRUE;
	}

	if ( iLen = COMMAND ( "/test_command_polling", pszBuff ) )
	{
		if ( GetParameterString ( pszBuff, 1, szCommandParam1 ) ||
			GetParameterString ( pszBuff, 2, szCommandParam2 ) )
		{

			/*PacketTransCommand smTransCommand;
			smTransCommand.iLength = sizeof ( PacketTransCommand );
			smTransCommand.iHeader = PKTHDR_PublicPolling; //smTRANSCODE_PUBLIC_POLLING
			smTransCommand.WParam = atoi ( szCommandParam1 );
			smTransCommand.LParam = atoi ( szCommandParam2 );
			smTransCommand.SParam = 0;
			smTransCommand.EParam = 0;

			SENDPACKET ( pcUserData, &smTransCommand, FALSE );*/
		}
	}


	if ( iLen = COMMAND ( "/test_user_ranking", pszBuff ) )
	{
		if ( GetParameterString ( pszBuff, 1, szCommandParam1 ) )
		{
			CHARACTERSERVER->CheckSyncUserRanking ( pcUser, atoi ( szCommandParam1 ) );
		}
	}

	if ( iLen = COMMAND( "/ItemSemiPerf", pszBuff ) )
	{
		GetParameterString( pszBuff, 1, szCommandParam1 );
		GetParameterString( pszBuff, 2, szCommandParam2 );
		GetParameterString( pszBuff, 3, szCommandParam3 );

		// Create Item
		{
			ItemData * psItemData = ITEMSERVER->FindItemPointerTable( szCommandParam1 );
			if ( psItemData )
			{
				DefinitionItem * psDefItem = ITEMSERVER->FindItemDefByCode( psItemData->sBaseItemID.ToItemID() );
				if ( psDefItem )
				{
					*( UINT* )0x8B70264 = 1;
					*( UINT* )0x8B70268 = atoi( szCommandParam2 );
					ItemData sItemData;
					ITEMSERVER->CreateItem( &sItemData.sItem, psDefItem, EItemSource::GameMaster, atoi( szCommandParam2 ) );
					*( UINT* )0x8B70264 = 0;
					*( UINT* )0x8B70268 = 0;
					if ( atoi( szCommandParam3 ) > 0 )
					{
						short sParam = atoi( szCommandParam3 );
						if ( sItemData.sItem.sDamage.sMin )
						{
							sItemData.sItem.sDamage.sMin -= sParam;
							sItemData.sItem.sDamage.sMax -= sParam;
						}
						if ( sItemData.sItem.iDefenseRating )
						{
							sItemData.sItem.iDefenseRating -= (int)(sParam*2);
						}
						if ( sItemData.sItem.iAttackRating )
						{
							sItemData.sItem.iAttackRating -= (int)(sParam*2)-1;
						}
						ITEMSERVER->ReformItem( &sItemData );
					}
					ITEMSERVER->SendItemData( pcUserData, &sItemData.sItem );
					CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> '%s' Created at Spec %d (-%d -%d)", szCommandParam1, atoi( szCommandParam2 ), sItemData.sItem.sDamage.sMin, sItemData.sItem.sDamage.sMax );
				}
			}
		}
		return TRUE;
	}
	if ( iLen = COMMAND( "/getitemperf", pszBuff ) )
	{
		GetParameterString( pszBuff, 1, szCommandParam1 );
		GetParameterString( pszBuff, 2, szCommandParam2 );

		// Create Item
		{
			ItemData * psItemData = ITEMSERVER->FindItemPointerTable( szCommandParam1 );
			if ( psItemData )
			{
				DefinitionItem * psDefItem = ITEMSERVER->FindItemDefByCode( psItemData->sBaseItemID.ToItemID() );
				if ( psDefItem )
				{
					Item sItem;
					ITEMSERVER->CreatePerfectItem( &sItem, psDefItem, EItemSource::GameMaster, atoi( szCommandParam2 ) );
					ITEMSERVER->SendItemData( pcUserData, &sItem );
				}
			}
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> '%s' Created Perfect at Speck %d", szCommandParam1, atoi( szCommandParam2 ) );
		}
		return TRUE;
	}

	if ( COMMAND( "/extradrop", pszBuff ) )
	{
		if ( GetParameterString( pszBuff, 1, szCommandParam1 ) )
		{
			int iDrop = atoi( szCommandParam1 );
			CHATSERVER->SendChatAllGM( "GM> Extra drop: %d -> %d", EVENT_EXTRADROPS, iDrop );
			EVENT_EXTRADROPS = iDrop;
		}
		else
		{
			CHATSERVER->SendChatEx ( pcUser, CHATCOLOR_Error, "> Current extra drop count: %d", EVENT_EXTRADROPS );
		}

		return TRUE;
	}

	if ( COMMAND( "/event_reducemondmg", pszBuff ) )
	{
		if ( GetParameterString( pszBuff, 1, szCommandParam1 ) )
		{
			if ( STRINGCOMPAREI( "true", szCommandParam1 ) )
			{
				EVENT_REDUCE_DAMAGEMON		= TRUE;
				EVENT_REDUCE_DAMAGEMON_LIVE = TRUE;
				CHATSERVER->SendChat( pcUser, CHATCOLOR_Error, "> Reduce Damage Monster Event is ON!" );
			}
			else if ( STRINGCOMPAREI( "false", szCommandParam1 ) )
			{
				EVENT_REDUCE_DAMAGEMON		= FALSE;
				EVENT_REDUCE_DAMAGEMON_LIVE = FALSE;
				CHATSERVER->SendChat( pcUser, CHATCOLOR_Error, "> Reduce Damage Monster Event is OFF!" );
			}
			else
				CHATSERVER->SendChat( pcUser, CHATCOLOR_Error, "> Use: /event_reducemondmg <true|false>" );
		}
		else
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> Use: /event_reducemondmg <true|false> [%s]", EVENT_REDUCE_DAMAGEMON ? "ON" : "OFF" );

		return TRUE;
	}
	if ( COMMAND( "/serverfps", pszBuff ) )
	{
		if ( GetParameterString( pszBuff, 1, szCommandParam1 ) )
		{
			int iFps = atoi( szCommandParam1 );
			if ( iFps > 1000 )
				iFps = 1000;
			else if ( iFps < 15 )
				iFps = 15;

			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> FPS Server: %d -> %d", *(int*)0x006E46F4, iFps );
			*(int*)(0x006E46F4) = iFps;
			*(DWORD*)(0x07AC9F74) = 0;
		}
		else
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> FPS Server: %d", *(int*)(0x006E46F4) );

		return TRUE;
	}

	if ( COMMAND( "/SQLSkill", pszBuff ) )
	{
		if ( GetParameterString( pszBuff, 1, szCommandParam1 ) && GetParameterString( pszBuff, 2, szCommandParam2 ), GetParameterString( pszBuff, 3, szCommandParam3 ) )
		{
			WriteSQLSkillInfo( szCommandParam1, szCommandParam2, atoi( szCommandParam3 ) );
		}

		return TRUE;
	}
	if ( COMMAND( "/SQLCALCMON", pszBuff ) )
	{
		if ( GetParameterString( pszBuff, 1, szCommandParam1 ) && GetParameterString( pszBuff, 2, szCommandParam2 ) && GetParameterString( pszBuff, 3, szCommandParam3 )
			&& GetParameterString( pszBuff, 4, szCommandParam4 ) )
		{
			int iCount = 0;

			int iTotal = ((atoi( szCommandParam1 ) * 24) * atoi( szCommandParam4 )) * atoi( szCommandParam2 );

			for ( int i = 0; i < iTotal; i++ )
			{
				int iPercentMax = 105019060;

				int iChanceDrop = iPercentMax - atoi( szCommandParam3 );

				int v = CALL( 0x00402050 ) % iPercentMax;

				if ( v < iChanceDrop )
					iCount++;
			}

			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> [%d]", iTotal - iCount );
		}

		return TRUE;
	}

	if ( COMMAND( "/expevent", pszBuff ) )
	{
		if ( GetParameterString( pszBuff, 1, szCommandParam1 ) )
		{
			int iExp = atoi( szCommandParam1 );
			if ( iExp > 1000 )
				iExp = 1000;
			else if ( iExp < 0 )
				iExp = 0;

			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> EXP Event: %d%% -> %d%%", *(int*)0x0084601C, iExp );
			*(int*)(0x0084601C)		= iExp;

			NETSERVER->SendCommandToLoginServer ( PKTHDR_NetEXPEvent, iExp );
		}
		else
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> EXP Event: %d%%", *(int*)(0x0084601C) );

		return TRUE;
	}
	if ( COMMAND( "/PVPMap", pszBuff ) )
	{
		if ( GetParameterString( pszBuff, 1, szCommandParam1 ) )
		{
			int iMap = atoi( szCommandParam1 );

			if ( iMap != -1 )
				CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> PVP Map: %d", iMap );
			else
				CHATSERVER->SendChat( pcUser, CHATCOLOR_Error, "> PVP Map is disabled!" );
			*(int*)(0x0084831C) = iMap;
		}
		else
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> EXP Event: %d%%", *(int*)(0x0084601C) );

		return TRUE;
	}


	if ((iLen = COMMAND("/pvp ", pszBuff)) ||
		(iLen = COMMAND("/PVP ", pszBuff)) ||
		(iLen = COMMAND("/PvP ", pszBuff)))
	{
		if (GetParameterString(pszBuff, 1, szCommandParam1))
		{
			if (STRINGCOMPAREI("dmg_scale_lvl", szCommandParam1))
			{
				if (GetParameterString(pszBuff, 2, szCommandParam2))
				{
					float value = static_cast<float>( atof( szCommandParam2 ) );
					float oldValue = DAMAGEHANDLER->GetNewPVPLevelBasedDamageScale();

					DAMAGEHANDLER->SetNewPVPLevelBasedDamageScale(value);
					CHATSERVER->SendChatAllGM("GM> Level-based Damage Scale: %.2f -> %.2f", oldValue, value);
					DAMAGEHANDLER->UpdatePVPScalingIni("Level Based Scaling", "dLevelBasedPvpDamageScale", value);
				}
				else
				{
					CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Expecting a param for dmg_scale_lvl");
				}

				return TRUE;
			}
			else if (STRINGCOMPAREI("abs_scale_lvl", szCommandParam1))
			{
				if (GetParameterString(pszBuff, 2, szCommandParam2))
				{
					float value = static_cast<float>( atof( szCommandParam2 ) );
					float oldValue = DAMAGEHANDLER->GetNewPVPLevelBasedAbsScale();

					DAMAGEHANDLER->SetNewPVPLevelBasedAbsScale(value);
					CHATSERVER->SendChatAllGM("GM> Level-based Abs Scale: %.2f -> %.2f", oldValue, value);
					DAMAGEHANDLER->UpdatePVPScalingIni("Level Based Scaling", "dLevelBasedPvpAbsScale", value);
				}
				else
				{
					CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Expecting a param for abs_scale");
				}

				return TRUE;
			}
			else if (STRINGCOMPAREI("global_dmg_reduction", szCommandParam1))
			{
				if (GetParameterString(pszBuff, 2, szCommandParam2))
				{
					float oldValue = DAMAGEHANDLER->GetNewPVPGlobalDamageReduction();
					float value = static_cast<float>( atof( szCommandParam2 ) );

					DAMAGEHANDLER->SetNewPVPGlobalDamageReduction(value);
					CHATSERVER->SendChatAllGM("GM> Global Damage reduction: %.2f -> %.2f", oldValue, value);
					DAMAGEHANDLER->UpdatePVPScalingIni("General", "dGlobalDamageReduction", value);
				}
				else
				{
					CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Expecting a param for global_dmg_reduction");
				}
			}
			else if (STRINGCOMPAREI("global_abs_reduction", szCommandParam1))
			{
				if (GetParameterString(pszBuff, 2, szCommandParam2))
				{
					float oldValue = DAMAGEHANDLER->GetNewPVPGlobalAbsReduction();
					float value = static_cast<float>( atof( szCommandParam2 ) );

					DAMAGEHANDLER->SetNewPVPGlobalAbsReduction(value);
					CHATSERVER->SendChatAllGM("GM> Global Abs reduction: %.2f -> %.2f", oldValue, value);
					DAMAGEHANDLER->UpdatePVPScalingIni("General", "dGlobalAbsReduction", value);
				}
				else
				{
					CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Expecting a param for global_abs_reduction");
				}
			}
			else
			{
				CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Invalid PvP Adjust type");
			}

		}
		return TRUE;
	}


	if ( COMMAND( "/EventBC", pszBuff ) )
	{
		if ( GetParameterString( pszBuff, 1, szCommandParam1 ) )
		{
			int i = atoi( szCommandParam1 );

			if ( i != -1 )
				CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> Siege War Mode: %d", i );

			BLESSCASTLESERVER->SetSiegeWarMode( (ESiegeWarMode)i );
		}
		else
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> EXP Event: %d%%", *(int*)(0x0084601C) );

		return TRUE;
	}
	if ( MAPSDATA[MAPID_BlessCastle].iState )
	{
		if ( COMMAND( "/StartSiegeWar", pszBuff ) )
		{
			BLESSCASTLESERVER->Start();
			CHATSERVER->SendChat( pcUser, CHATCOLOR_Error, "> Started!" );

			return TRUE;
		}
		if ( COMMAND( "/EndSiegeWar", pszBuff ) )
		{
			BLESSCASTLESERVER->ResetUserScore();
			BLESSCASTLESERVER->End( FALSE );
			CHATSERVER->SendChat( pcUser, CHATCOLOR_Error, "> Ended!" );

			return TRUE;
		}
		if ( COMMAND( "/EndWinSiegeWar", pszBuff ) )
		{
			BLESSCASTLESERVER->psBlessCastleStatusData->dwBattleOverTime = TICKCOUNT;
			CHATSERVER->SendChat( pcUser, CHATCOLOR_Error, "> Ended!" );

			return TRUE;
		}
		if ( COMMAND( "/EasySiegeWar", pszBuff ) )
		{
			BLESSCASTLESERVER->EasySiegeWar();
			CHATSERVER->SendChat( pcUser, CHATCOLOR_Error, "> Easy Siege War!" );

			return TRUE;
		}
	}

	if ( COMMAND( "/ClearTickRO", pszBuff ) )
	{
		NETSERVER->SendCommandToLoginServer ( PKTHDR_NetClearTickRO );
		return TRUE;
	}

	if ( COMMAND( "/ClearTickChristmas", pszBuff ) )
	{
		NETSERVER->SendCommandToLoginServer ( PKTHDR_NetClearTickChristmas );
		return TRUE;
	}


	if ( COMMAND( "/recoveritem", pszBuff ) )
	{
		if ( GAME_SERVER )
		{
			NETSERVER->SendGMCommandToLoginServer(pcUser, pszBuff);
		}
		else
		{
			if ( GetParameterString( pszBuff, 1, szCommandParam1 ) )
			{
				int itemCreateLogDBId = atoi( szCommandParam1 );
				ITEMSERVER->RecoverItemFromItemCreateLog( pcUser, itemCreateLogDBId );
			}
		}

		return TRUE;
	}

	if (iLen = COMMAND("/ReloadMonsterDropTable", pszBuff))
	{
		LOOTSERVER->SQLUpdateDropTableFromDatabase();
		CHATSERVER->SendChatAllGM("GM> Monster Drop Table Reloaded");

		return TRUE;
	}

	if ( iLen = COMMAND ( "/ReloadItemDef", pszBuff ) )
	{
		//doesn't work atm
		ITEMSERVER->CreateItemMemoryTable ();

		return TRUE;
	}



	if (iLen = COMMAND("/TestMonsterDropTable", pszBuff))
	{
		if (GetParameterString(pszBuff, 1, szCommandParam1))
		{
			LOOTSERVER->GenerateDropStats( szCommandParam1, "ServerCommand");
			CHATSERVER->SendChatEx ( pcUser, CHATCOLOR_Error, "GM> Drop Table Tested for ID: %s", szCommandParam1 );
		}
	}


	if ( COMMAND( "/setbosstime", pszBuff ) )
	{
		if ( GetParameterString( pszBuff, 1, szCommandParam1 ) )
		{
			BOSS_TIME = atoi( szCommandParam1 );
			*(UINT*)0x07AC9F60 = 0;
			*(DWORD*)(0x7AC9F64) = 0;

			MAPSERVER->UpdateUsersBossTime();
			GSERVER->SqlUpdateOrInsertMetadata("boss.time.second", BOSS_TIME);

			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> New Boss Time [xx:%02d]", BOSS_TIME );
		}

		return TRUE;
	}

	if (COMMAND("/spawnbosses", pszBuff))
	{
		BOSS_TIME = GetServerTime()->wMinute - 1;
		if ( BOSS_TIME < 0 )
			BOSS_TIME = 0;

		MAPSERVER->UpdateUsersBossTime();
		GSERVER->SqlUpdateOrInsertMetadata("boss.time.second", BOSS_TIME);

		for (int i = 0; i < NUM_MAPS; i++)
		{
			Map* pcMap = MAPSDATA + i;

			if (pcMap->sSpawnSetting.iTotalBosses > 0)
			{
				MAPSERVER->SpawnMonsterBoss(pcMap, pcMap->sSpawnSetting.saBoss);
			}
		}

		CHATSERVER->SendChatAllGM( "GM> BOSS_TIME updated to now" );
		return TRUE;
	}

	if (iLen = COMMAND("/shiftgametime", pszBuff))
	{
		if (GetParameterString(pszBuff, 1, szCommandParam1))
		{
			int shift = atoi(szCommandParam1);

			if (shift >= 0 && shift <= 24)
			{
				MAPSERVER->iGameTimeShift = shift;
				MAPSERVER->UpdateTime();
				CHATSERVER->SendChatAllGM("GM> Game time is now %d", MAPSERVER->GetTime());
			}
			else
			{
				CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Error: /shiftgametime time needs to be between 0 and 24");
			}
		}
		else
		{
			CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Error: /shiftgametime requires a value between 0 and 24");
		}

		return TRUE;
	}



	if (iLen = COMMAND("/sod_room_score_scale", pszBuff))
	{
		if (GetParameterString(pszBuff, 1, szCommandParam1))
		{
			int iNewScale = atoi(szCommandParam1);
			int iOldScale = EVENT_BELLATRA_ROOM_SCORE_TO_GOLD_SCALE;
			EVENT_BELLATRA_ROOM_SCORE_TO_GOLD_SCALE = iNewScale;

			INI::CReader cReader("server.ini");
			cReader.WriteInt("Event", "BellatraSharedGoldScale", iNewScale);

			CHATSERVER->SendChatAllGM("> SOD room score to gold scale %d -> %d", iOldScale, iNewScale);
		}
		else
		{
			CHATSERVER->SendChatEx(pcUser, CHATCOLOR_White, "> Current room score to gold scale: %d", EVENT_BELLATRA_ROOM_SCORE_TO_GOLD_SCALE);
		}

		return TRUE;
	}

	if (iLen = COMMAND("/sod_performance_gold_scale", pszBuff))
	{
		if (GetParameterString(pszBuff, 1, szCommandParam1))
		{
			int iNewScale = atoi(szCommandParam1);
			int iOldScale = EVENT_BELLATRA_PERFORMANCE_GOLD_SCALE;
			EVENT_BELLATRA_PERFORMANCE_GOLD_SCALE = iNewScale;

			INI::CReader cReader("server.ini");
			cReader.WriteInt("Event", "BellatraPerformanceGoldScale", iNewScale);

			CHATSERVER->SendChatAllGM("> SOD performance to gold scale %d -> %d", iOldScale, iNewScale);
		}
		else
		{
			CHATSERVER->SendChatEx(pcUser, CHATCOLOR_White, "> Current performance to gold scale: %d", EVENT_BELLATRA_PERFORMANCE_GOLD_SCALE);
		}

		return TRUE;
	}

	if (iLen = COMMAND("/sod_update_all_solo_ranking", pszBuff))
	{
		BELLATRASERVER->UpdateAllSoloCrownRankingForAllClasses();
		CHATSERVER->SendChatAllGM("GM> Sod Solo ranking updated for all classes");
	}

	if (iLen = COMMAND("/sod_update_class_solo_ranking", pszBuff))
	{
		if (GetParameterString(pszBuff, 1, szCommandParam1))
		{
			int iClass = atoi(szCommandParam1);
			if (iClass >= 1 && iClass <= 10)
			{
				BELLATRASERVER->UpdateAllSoloCrownRankingForClass((ECharacterClass)iClass);
				CHATSERVER->SendChatAllGM("GM> Sod Solo ranking updated for class: %d", iClass);
			}
		}
	}

	if (iLen = COMMAND("/sod_crown_humor", pszBuff))
	{
		if (GetParameterString(pszBuff, 1, szCommandParam1))
		{
			if (STRINGCOMPAREI(szCommandParam1, "on"))
			{
				BELLATRASERVER->SetSoloCrownHumorMode(TRUE);
				CHATSERVER->SendChatAllGM("GM> SOD Solo Crown Humor Mode = On!");
				return TRUE;
			}
			else
			{
				BELLATRASERVER->SetSoloCrownHumorMode(FALSE);
				CHATSERVER->SendChatAllGM("GM> SOD Solo Crown Humor Mode = Off!");
				return TRUE;
			}
		}

		CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> /sod_crown_humor [ON | Off]");

		return TRUE;
	}






	if (iLen = COMMAND("/sod_skipround", pszBuff))
	{
		BELLATRASERVER->bSkipCurrentRound = TRUE;
		CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> SoD round %d skipped!", BELLATRASERVER->GetRound());

		return TRUE;
	}

	if (iLen = COMMAND("/sod_force_start", pszBuff))
	{
		DEBUG("sod_force_start");
		BELLATRASERVER->ForceStartSod();
		CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> SoD started!", BELLATRASERVER->GetRound());

		return TRUE;
	}

	if (iLen = COMMAND("/sod_force_end", pszBuff))
	{
		DEBUG("sod_force_end");
		BELLATRASERVER->ForceEndSod();
		CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> SoD finished!", BELLATRASERVER->GetRound());

		return TRUE;
	}

	if (iLen = COMMAND("/sod_force_reset_scores", pszBuff))
	{
		BELLATRASERVER->ResetBellatra();
		CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> SoD score cleared!");

		return TRUE;
	}

	if (iLen = COMMAND("/sod_scores_reset_disallowed", pszBuff))
	{
		BELLATRASERVER->AllowBellatraReset(FALSE);
		CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> SoD scores reset disallowed!");

		return TRUE;
	}

	if (iLen = COMMAND("/sod_scores_reset_allowed", pszBuff))
	{
		BELLATRASERVER->AllowBellatraReset(TRUE);
		CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, ">  SoD scores reset allowed!");

		return TRUE;
	}

	if (iLen = COMMAND("/sod_test_drops", pszBuff))
	{
		if (GetParameterString(pszBuff, 1, szCommandParam1))
		{
			int iNumOfDrops = atoi(szCommandParam1);

			BELLATRASERVER->DropBellatraRewards(iNumOfDrops);
			CHATSERVER->SendChatAllGM("GM> %d Bellatra rewards dropped!", iNumOfDrops);
		}

		return TRUE;
	}

	if (iLen = COMMAND("/sod_reload_rewards_from_sql", pszBuff))
	{
		BELLATRASERVER->SQLReadInRewardsTable();
		CHATSERVER->SendChatAllGM("GM> Bellatra rewards reloaded!");
		return TRUE;
	}

	if (iLen = COMMAND("/sod_status", pszBuff))
	{
		//eEventTimeOver
		int eventTimeOver = READDWORD(0x07AC9E30);

		//EventMode
		int eventMode = READDWORD(0x07AC9E2C);

		CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> SoD status: Time over: %d, Event mode = %d", eventTimeOver, eventMode);
		CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> Sod current round is: %d", BELLATRASERVER->GetRound());
		CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> Game time is %d", MAPSERVER->GetTime());
		CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> Sum Entrance fees: %d", BELLATRASERVER->GetSumEntranceFees());



		return TRUE;
	}

	if ( iLen = COMMAND( "/sod_enable", pszBuff ) )
	{
		if ( GetParameterString( pszBuff, 1, szCommandParam1 ) )
		{
			if ( STRINGCOMPAREI( "true", szCommandParam1 ) )
			{
				EVENT_BELLATRA = 2;
				CHATSERVER->SendChat( pcUser, CHATCOLOR_Error, "> Bellatra Event is enabled!" );
			}
			else if ( STRINGCOMPAREI( "false", szCommandParam1 ) )
			{
				EVENT_BELLATRA = 0;
				CHATSERVER->SendChat( pcUser, CHATCOLOR_Error, "> Bellatra Event is disabled!" );
			}
			else
				CHATSERVER->SendChat( pcUser, CHATCOLOR_Error, "> Use: /sod_enable <true|false>" );
		}
		else
			CHATSERVER->SendChat( pcUser, CHATCOLOR_Error, "> Use: /sod_enable <true|false>" );



		return TRUE;
	}

	//boss crystal
	if ( iLen = COMMAND( "/event_crystal", pszBuff ) )
	{
		if ( GetParameterString( pszBuff, 1, szCommandParam1 ) )
		{
			if ( STRINGCOMPAREI( "true", szCommandParam1 ) )
			{
				(*(int*)0x00845FF8) = 2;
				(*(int*)0x07AC9D50) = 1;
				CHATSERVER->SendChat( pcUser, CHATCOLOR_Error, "> Crystal Event is enabled!" );
			}
			else if ( STRINGCOMPAREI( "false", szCommandParam1 ) )
			{
				(*(int*)0x00845FF8) = 0;
				(*(int*)0x07AC9D50) = 0;
				CHATSERVER->SendChat( pcUser, CHATCOLOR_Error, "> Crystal Event is disabled!" );
			}
			else
				CHATSERVER->SendChat( pcUser, CHATCOLOR_Error, "> Use: /event_crystal <true|false>" );
		}
		else
			CHATSERVER->SendChat( pcUser, CHATCOLOR_Error, "> Use: /event_crystal <true|false>" );


		return TRUE;
	}

	if ((iLen = COMMAND("/sql_HP",					pszBuff)) || //Health points
		(iLen = COMMAND("/sql_Size",				pszBuff)) || //Monster Size
		(iLen = COMMAND("/sql_Type",				pszBuff)) || //Monster type
		(iLen = COMMAND("/sql_EXP",					pszBuff)) || //Experience
		(iLen = COMMAND("/sql_Absorb",				pszBuff)) || //Absorb rating
		(iLen = COMMAND("/sql_Block",				pszBuff)) || //Block rating
		(iLen = COMMAND("/sql_StunChance",			pszBuff)) || //Stun chance (doesn't work?)
		(iLen = COMMAND("/sql_Defense",				pszBuff)) || //Defense
		(iLen = COMMAND("/sql_Potion",				pszBuff)) || //Potion
		(iLen = COMMAND("/sql_Organic",				pszBuff)) || //Organic resistence
		(iLen = COMMAND("/sql_Lightning",			pszBuff)) || //Lightning resistence
		(iLen = COMMAND("/sql_Ice",					pszBuff)) || //Ice resistence
		(iLen = COMMAND("/sql_Fire",				pszBuff)) || //Fire resistance
		(iLen = COMMAND("/sql_Poison",				pszBuff)) || //Poison resistence
		(iLen = COMMAND("/sql_Magic",				pszBuff)) || //Magic resistence
		(iLen = COMMAND("/sql_NumDrops",			pszBuff)) || //Number of drops
		(iLen = COMMAND("/sql_PublicDrop",			pszBuff)) || //Drop is public?
		(iLen = COMMAND("/sql_SpawnMin",			pszBuff)) || //Spawn min
		(iLen = COMMAND("/sql_SpawnMax",			pszBuff)) || //Spawn max
		(iLen = COMMAND("/sql_MoveSpeed",			pszBuff)) || //Move speed (doesn't work?)
		(iLen = COMMAND("/sql_ViewSight",			pszBuff)) || //View sight
		(iLen = COMMAND("/sql_AttackMinMax",		pszBuff)) || //Attack min max
		(iLen = COMMAND("/sql_AttackSpeed",			pszBuff)) || //Attack speed (doesn't work?)
		(iLen = COMMAND("/sql_AttackRange",			pszBuff)) || //Attack range
		(iLen = COMMAND("/sql_AttackRating",		pszBuff)) || //Attack rating
		(iLen = COMMAND("/sql_PerfectAttackRate",	pszBuff)) || //Perfect attack rating (doesn't work?)
		(iLen = COMMAND("/sql_SkillMinMax",			pszBuff)) || //Skill min max
		(iLen = COMMAND("/sql_SkillType",			pszBuff)) || //Skill type
		(iLen = COMMAND("/sql_SkillChance",			pszBuff)) || //Skill chance
		(iLen = COMMAND("/sql_SkillHitRange",		pszBuff)) || //iSkillPierceRange // SkillDistance (uses a rect box to determine hit area?)
		(iLen = COMMAND("/sql_SkillHitBoxLeft",		pszBuff)) || //Skill hit box rect (left)
		(iLen = COMMAND("/sql_SkillHitBoxRight",	pszBuff)) || //Skill hit box rect (right)
		(iLen = COMMAND("/sql_SkillHitBoxTop",		pszBuff)) || //Skill hit box rect (top)
		(iLen = COMMAND("/sql_SkillHitBoxBottom",	pszBuff)) || //Skill hit box rect (bottom)
		(iLen = COMMAND("/sql_Glow",				pszBuff)) || //Glow Boss Flag
		(iLen = COMMAND("/sql_SkillArea",			pszBuff)) || //iSkillArea // SkillRange
		(iLen = COMMAND("/sql_Level",				pszBuff)))	 //Monster Level
	{
		char szCommand[256] = { 0 };

		//get the command
		GetCommandString(pszBuff, szCommand);

		//has unit id
		if (GetParameterString(pszBuff, 1, szCommandParam1))
		{
			int unitId = atoi(szCommandParam1);
			UnitData* pcUnitData = UNITSERVER->GetUnit(atoi(szCommandParam1));

			if (pcUnitData)
			{
				CharacterData* psCharacterData = UNITSERVER->GetCharacterDataByName(pcUnitData->GetName());
				UnitInfo* psUnitInfo		   = psCharacterData != NULL ? psCharacterData->psUnitInfo : NULL;

				if (psCharacterData && psUnitInfo)
				{
					int existingValue1 = -1;
					float fExistingValue1 = -1.0;
					std::string szExistingValue = "";
					int existingValue2 = -1;
					INT64 lExistingValue3 = -1;

					std::string command = rtrim(std::string(szCommand));
					std::string commandId = command.substr(command.find("_") + 1);
					std::string columnName1 = "";
					std::string columnName2 = "";
					std::string tableName = "MonsterList";

					if (     STRINGCOMPAREI("/sql_HP",					szCommand)) { columnName1="HealthPoint";		existingValue1 = UNITSERVER->GetMonsterHealthPointDefinition(psCharacterData->iUniqueMonsterID);	}
					else if (STRINGCOMPAREI("/sql_Size",				szCommand)) { columnName1="Size";				fExistingValue1 = psCharacterData->sSize / 256.0f;							}
					else if (STRINGCOMPAREI("/sql_Type",				szCommand)) { columnName1="PropertyMon";		szExistingValue = UNITINFODATA->GetMonsterTypeStr(psCharacterData->iMonsterType); }
					else if (STRINGCOMPAREI("/sql_EXP",					szCommand)) { columnName1="EXP";				lExistingValue3 = UNITSERVER->GetExp(pcUnitData);											}
					else if (STRINGCOMPAREI("/sql_Absorb",				szCommand)) { columnName1="Absorb";				existingValue1 = psCharacterData->iAbsorbRating;							}
					else if (STRINGCOMPAREI("/sql_Block",				szCommand)) { columnName1="Block";				existingValue1 = psCharacterData->iBlockRating;								}
					else if (STRINGCOMPAREI("/sql_StunChance",			szCommand)) { columnName1="Stunchance";			existingValue1 = psUnitInfo->iFlinchChance;										}
					else if (STRINGCOMPAREI("/sql_Defense",				szCommand)) { columnName1="Defense";			existingValue1 = psCharacterData->iDefenseRating;							}
					else if (STRINGCOMPAREI("/sql_PotionPercent",		szCommand)) { columnName1="PotionPercent";		existingValue1 = psUnitInfo->iPerPotions;									}
					else if (STRINGCOMPAREI("/sql_Potion",				szCommand)) { columnName1="Potions";			existingValue1 = psUnitInfo->iMaxPotions;									}
					else if (STRINGCOMPAREI("/sql_Organic",				szCommand)) { columnName1="Organic";			existingValue1 = psCharacterData->sElementalDef[EElementID::Organic];		}
					else if (STRINGCOMPAREI("/sql_Lightning",			szCommand)) { columnName1="Lightning";			existingValue1 = psCharacterData->sElementalDef[EElementID::Lightning];		}
					else if (STRINGCOMPAREI("/sql_Ice",					szCommand)) { columnName1="Ice";				existingValue1 = psCharacterData->sElementalDef[EElementID::Ice];			}
					else if (STRINGCOMPAREI("/sql_Fire",				szCommand)) { columnName1="Fire";				existingValue1 = psCharacterData->sElementalDef[EElementID::Fire];			}
					else if (STRINGCOMPAREI("/sql_Poison",				szCommand)) { columnName1="Poison";				existingValue1 = psCharacterData->sElementalDef[EElementID::Poison];		}
					else if (STRINGCOMPAREI("/sql_Magic",				szCommand)) { columnName1="Magic";				existingValue1 = psCharacterData->sElementalDef[EElementID::Wind];			}
					else if (STRINGCOMPAREI("/sql_SpawnMin",			szCommand)) { columnName1="SpawnMin";			existingValue1 = psUnitInfo->iGroupLimitMin;								}
					else if (STRINGCOMPAREI("/sql_SpawnMax",			szCommand)) { columnName1="SpawnMax";			existingValue1 = psUnitInfo->iGroupLimitMax;								}
					else if (STRINGCOMPAREI("/sql_MoveSpeed",			szCommand)) { columnName1="MoveSpeed";			existingValue1 = ((psCharacterData->iMovementSpeed - 256) / 16) + 9;	    }
					else if (STRINGCOMPAREI("/sql_ViewSight",			szCommand)) { columnName1="ViewSight";			existingValue1 = psUnitInfo->iSight;										}
					else if (STRINGCOMPAREI("/sql_NumDrops",			szCommand)) { columnName1="DropQuantity";		existingValue1 = psUnitInfo->iNumDrops;				                        }
					else if (STRINGCOMPAREI("/sql_PublicDrop",			szCommand)) { columnName1="DropIsPublic";		existingValue1 = psUnitInfo->bPublicDrop;				                    }
					else if (STRINGCOMPAREI("/sql_Glow",				szCommand)) { columnName1="Glow";				existingValue1 = (int)psCharacterData->sMonsterClass;						}
					else if (STRINGCOMPAREI("/sql_Level",				szCommand)) { columnName1="Level";				existingValue1 = psCharacterData->iLevel;									}

					//Basic attack related
					else if (STRINGCOMPAREI("/sql_AttackMinMax",		szCommand)) { columnName1="ATKPowMin";			existingValue1 = psCharacterData->iMinDamage;
					                                                                  columnName2="ATKPowMax";			existingValue2 = psCharacterData->iMaxDamage;								}
					else if (STRINGCOMPAREI("/sql_AttackSpeed",			szCommand)) { columnName1="AttackSpeed";		existingValue1 = psCharacterData->iAttackSpeed >> 8;						}
					else if (STRINGCOMPAREI("/sql_AttackRange",			szCommand)) { columnName1="AttackRange";		existingValue1 = psCharacterData->iAttackRange >> 8;						}
					else if (STRINGCOMPAREI("/sql_AttackRating",		szCommand)) { columnName1="AttackRating";		existingValue1 = psCharacterData->iAttackRating;							}
					else if (STRINGCOMPAREI("/sql_PerfectAttackRate",	szCommand)) { columnName1="PerfectAttackRate";	existingValue1 = psUnitInfo->iLureDistance;									}

					//Skill related
					else if (STRINGCOMPAREI("/sql_SkillType",			szCommand)) { columnName1="SpecialSkillType";	existingValue1 = psUnitInfo->iSkillType;									}
					else if (STRINGCOMPAREI("/sql_SkillChance",			szCommand)) { columnName1="SpecialHitRate";		existingValue1 = psUnitInfo->iSkillChance;									}
					else if (STRINGCOMPAREI("/sql_SkillHitRange",		szCommand)) { columnName1="SpecialSkillHit";	existingValue1 = psUnitInfo->iSkillPierceRange;								}
					else if (STRINGCOMPAREI("/sql_SkillArea",			szCommand)) { columnName1="SpecialHitScope";	existingValue1 = psUnitInfo->iSkillArea;									}
					else if (STRINGCOMPAREI("/sql_SkillHitBoxLeft",		szCommand)) { columnName1="";					existingValue1 = psUnitInfo->iSkillBoxLeft;									}
					else if (STRINGCOMPAREI("/sql_SkillHitBoxRight",	szCommand)) { columnName1="";					existingValue1 = psUnitInfo->iSkillBoxRight;								}
					else if (STRINGCOMPAREI("/sql_SkillHitBoxTop",		szCommand)) { columnName1="";					existingValue1 = psUnitInfo->iSkillBoxTop;									}
					else if (STRINGCOMPAREI("/sql_SkillHitBoxBottom",	szCommand)) { columnName1="";					existingValue1 = psUnitInfo->iSkillBoxBottom;								}
					else if (STRINGCOMPAREI("/sql_SkillMinMax",			szCommand)) { columnName1="SpecialHitPowerMin";	existingValue1 = psUnitInfo->sSkillPower.sMin;
					                                                                  columnName2="SpecialHitPowerMax";	existingValue2 = psUnitInfo->sSkillPower.sMax;								}


					//has value #1
					if (GetParameterString(pszBuff, 2, szCommandParam2))
					{
						int newValue1 = atoi(szCommandParam2);
						float fNewValue1 = static_cast<float>( atof( szCommandParam2 ) );
						std::string szNewValue = szCommandParam2;
						UINT64 lNewValue1 = atoll(szCommandParam2);
						int newValue2 = -1;

						//has value #2 (optional, only for some stats)
						if (GetParameterString(pszBuff, 3, szCommandParam3))
						{
							newValue2 = atoi(szCommandParam3);
						}

						if (     STRINGCOMPAREI("/sql_HP",					szCommand)) { UNITSERVER->AddOrUpdateMonsterHealthPointDefinition( psCharacterData->iUniqueMonsterID, newValue1); }
						else if (STRINGCOMPAREI("/sql_Absorb",				szCommand)) { psCharacterData->iAbsorbRating					   = newValue1; }
						else if (STRINGCOMPAREI("/sql_Type",				szCommand)) { psCharacterData->iMonsterType					       = UNITINFODATA->GetMonsterType(szNewValue.c_str()); }
						else if (STRINGCOMPAREI("/sql_Size",				szCommand)) { psCharacterData->sSize							   = static_cast<WORD>((fNewValue1 > 0 ? (fNewValue1 * 256.0f) : 256.0f)); }
						else if (STRINGCOMPAREI("/sql_EXP",					szCommand)) { UNITSERVER->SetExp(pcUnitData, lNewValue1);			}
						else if (STRINGCOMPAREI("/sql_Block",				szCommand)) { psCharacterData->iBlockRating						   = newValue1; }
						else if (STRINGCOMPAREI("/sql_StunChance",			szCommand)) { psUnitInfo->iFlinchChance							   = newValue1; }
						else if (STRINGCOMPAREI("/sql_Defense",				szCommand)) { psCharacterData->iDefenseRating					   = newValue1; }
						else if (STRINGCOMPAREI("/sql_PotionPercent",		szCommand)) { psUnitInfo->iPerPotions							   = newValue1; }
						else if (STRINGCOMPAREI("/sql_Potion",				szCommand)) { psUnitInfo->iMaxPotions							   = newValue1; }
						else if (STRINGCOMPAREI("/sql_Organic",				szCommand)) { psCharacterData->sElementalDef[EElementID::Organic]  = newValue1; }
						else if (STRINGCOMPAREI("/sql_Lightning",			szCommand)) { psCharacterData->sElementalDef[EElementID::Lightning] = newValue1; }
						else if (STRINGCOMPAREI("/sql_Ice",					szCommand)) { psCharacterData->sElementalDef[EElementID::Ice]	   = newValue1; }
						else if (STRINGCOMPAREI("/sql_Fire",				szCommand)) { psCharacterData->sElementalDef[EElementID::Fire]	   = newValue1; }
						else if (STRINGCOMPAREI("/sql_Poison",				szCommand)) { psCharacterData->sElementalDef[EElementID::Poison]   = newValue1; }
						else if (STRINGCOMPAREI("/sql_Magic",				szCommand)) { psCharacterData->sElementalDef[EElementID::Wind]	   = newValue1; }
						else if (STRINGCOMPAREI("/sql_SpawnMin",			szCommand)) { psUnitInfo->iGroupLimitMin						   = newValue1; }
						else if (STRINGCOMPAREI("/sql_SpawnMax",			szCommand)) { psUnitInfo->iGroupLimitMax						   = newValue1; }
						else if (STRINGCOMPAREI("/sql_MoveSpeed",			szCommand)) { psCharacterData->iMovementSpeed					   = (((newValue1 - 9) * 16) + 256); }
						else if (STRINGCOMPAREI("/sql_ViewSight",			szCommand)) { psCharacterData->iSight							   = newValue1 * newValue1;
						                                                                  psUnitInfo->iSight								   = newValue1; }

						else if (STRINGCOMPAREI("/sql_Glow",				szCommand)) { psCharacterData->sMonsterClass					   = (EMonsterClass)newValue1; }
						else if (STRINGCOMPAREI("/sql_Level",				szCommand)) { psCharacterData->iLevel							   = newValue1; }
						else if (STRINGCOMPAREI("/sql_NumDrops",			szCommand)) { psUnitInfo->iNumDrops								   = newValue1; }
						else if (STRINGCOMPAREI("/sql_PublicDrop",			szCommand)) { psUnitInfo->bPublicDrop							   = newValue1 == 1 ? 1 : 0; }

						//Basic attack related
						else if (STRINGCOMPAREI("/sql_AttackMinMax",		szCommand)) { psCharacterData->iMinDamage	                       = newValue1;
																						  psCharacterData->iMaxDamage						   = newValue2; }
						else if (STRINGCOMPAREI("/sql_AttackSpeed",			szCommand)) { psCharacterData->iAttackSpeed						   = newValue1 << 8; }
						else if (STRINGCOMPAREI("/sql_AttackRange",			szCommand)) { psCharacterData->iAttackRange						   = newValue1 << 8; }
						else if (STRINGCOMPAREI("/sql_AttackRating",		szCommand)) { psCharacterData->iAttackRating				       = newValue1; }
						else if (STRINGCOMPAREI("/sql_PerfectAttackRate",	szCommand)) { psUnitInfo->iLureDistance							   = newValue1; }

						//Skill related
						else if (STRINGCOMPAREI("/sql_SkillType",			szCommand)) { psUnitInfo->iSkillType							   = newValue1; }
						else if (STRINGCOMPAREI("/sql_SkillChance",			szCommand)) { psUnitInfo->iSkillChance							   = newValue1; }
						else if (STRINGCOMPAREI("/sql_SkillHitRange",		szCommand)) { psUnitInfo->iSkillPierceRange						   = newValue1; }
						else if (STRINGCOMPAREI("/sql_SkillArea",			szCommand)) { psUnitInfo->iSkillArea							   = newValue1; }
						else if (STRINGCOMPAREI("/sql_SkillHitBoxLeft",		szCommand)) { psUnitInfo->iSkillBoxLeft							   = newValue1; }
						else if (STRINGCOMPAREI("/sql_SkillHitBoxRight",	szCommand)) { psUnitInfo->iSkillBoxRight						   = newValue1; }
						else if (STRINGCOMPAREI("/sql_SkillHitBoxTop",		szCommand)) { psUnitInfo->iSkillBoxTop							   = newValue1; }
						else if (STRINGCOMPAREI("/sql_SkillHitBoxBottom",	szCommand)) { psUnitInfo->iSkillBoxBottom						   = newValue1; }
						else if (STRINGCOMPAREI("/sql_SkillMinMax",			szCommand)) { psUnitInfo->sSkillPower.sMin						   = newValue1;
						                                                                  psUnitInfo->sSkillPower.sMax						   = newValue2; }

						if (columnName1.size() > 0)
							WriteMonsterStatus(pcUnitData, tableName.c_str(), columnName1.c_str(), szCommandParam2);

						if (columnName2.size() > 0)
							WriteMonsterStatus(pcUnitData, tableName.c_str(), columnName2.c_str(), szCommandParam3);


						int respawnPosX = pcUnitData->sPosition.iX;
						int respawnPosY = pcUnitData->sPosition.iY;
						int respawnPosZ = pcUnitData->sPosition.iZ;

						// Kill the Unit
						pcUnitData->SetCurrentHealth( 0 );
						pcUnitData->Animate( ANIMATIONTYPE_Die );


						//Respawn monster by id
						Map* pcMap = MAPSDATA + pcUserData->iMapID;
						if (pcMap)
						{
							MonsterMapSpawnHandle sMonster;
							sMonster.sCharacterData.szName[0] = 0;
							CopyMemory(&sMonster.sCharacterData, psCharacterData, sizeof(CharacterData));

							static Point3D sPosition;
							sPosition.iX = respawnPosX;
							sPosition.iY = respawnPosY;
							sPosition.iZ = respawnPosZ;

							(*(Point3D**)0x07AC9E24) = &sPosition;

							pcMap->bForceCreateMonster = TRUE;
							UnitData* pcUnitData = MAPSERVER->CreateUnit(pcMap, &sMonster, 0);
							pcUnitData->sSpawnPosition = pcUnitData->sPosition;

							(*(Point3D**)0x07AC9E24) = NULL;
						}


						if (existingValue2 != -1)
						{
							CHATSERVER->SendChatAllGM("GM> '%s' %s: %d-%d -> %d-%d (new)",
								pcUnitData->GetName(),
								commandId.c_str(),
								existingValue1,
								existingValue2,
								newValue1,
								newValue2);
						}
						else if (fExistingValue1 != -1.0)
						{
							CHATSERVER->SendChatAllGM("GM> '%s' %s: %.1f -> %.1f (new)",
								pcUnitData->GetName(),
								commandId.c_str(),
								fExistingValue1,
								fNewValue1);
						}
						else if (lExistingValue3 != -1)
						{
							CHATSERVER->SendChatAllGM( "GM> '%s' %s: %s -> %s (new)",
								pcUnitData->GetName(),
								commandId.c_str(),
								std::string( FormatNumber( lExistingValue3 ) ).c_str(),
								std::string( FormatNumber( lNewValue1 ) ).c_str() );
						}
						else if (!szExistingValue.empty())
						{
							CHATSERVER->SendChatAllGM( "GM> '%s' %s: %s -> %s (new)",
								pcUnitData->GetName(),
								commandId.c_str(),
								szExistingValue.c_str(),
								szNewValue.c_str() );
						}
						else
						{
							CHATSERVER->SendChatAllGM("GM> '%s' %s: %d -> %d (new)",
								pcUnitData->GetName(),
								commandId.c_str(),
								existingValue1,
								newValue1);
						}
					}

					//Does not have value. Return current value
					else
					{
						if (existingValue2 != -1)
						{
							CHATSERVER->SendChatEx(pcUser, CHATCOLOR_White, "> '%s' %s: %d-%d (current)",
								pcUnitData->GetName(),
								commandId.c_str(),
								existingValue1,
								existingValue2);
						}
						else if (fExistingValue1 != -1)
						{
							CHATSERVER->SendChatEx(pcUser, CHATCOLOR_White, "> '%s' %s: %.1f (current)",
								pcUnitData->GetName(),
								commandId.c_str(),
								existingValue1);
						}
						else if (lExistingValue3 != -1)
						{
							CHATSERVER->SendChatEx(pcUser, CHATCOLOR_White, "> '%s' %s: %lld (current)",
								pcUnitData->GetName(),
								commandId.c_str(),
								lExistingValue3);
						}
						else if (!szExistingValue.empty())
						{
							CHATSERVER->SendChatEx(pcUser, CHATCOLOR_White, "> '%s' %s: %s (current)",
								pcUnitData->GetName(),
								commandId.c_str(),
								szExistingValue.c_str());
						}
						else
						{
							CHATSERVER->SendChatEx(pcUser, CHATCOLOR_White, "> '%s' %s: %d (current)",
								pcUnitData->GetName(),
								commandId.c_str(),
								existingValue1);
						}
					}
				}
			}
		}
	}

	if ( iLen = COMMAND( "/remove_npc", pszBuff ) )
	{
		if ( GetParameterString( pszBuff, 1, szCommandParam1 ) ) //npc id (via mouse id)
		{
			int currentStageId = pcUserData->iMapID;
			int npcID = atoi( szCommandParam1 );
			int iX, iZ;
			BOOL bFound = FALSE;

			Map * psMap = MAPSDATA + currentStageId;

			if ( psMap == NULL )
				return TRUE;

			for ( int i = 0; i < MAX_NPCINMAP; i++ )
			{
				PacketUnitInfo* psNPC = psMap->saNPC + i;

				if ( psNPC->iHeader == 0 )
					continue;

				if ( psNPC->iID == npcID )
				{
					iX = psNPC->sPosition.iX >> 8;
					iZ = psNPC->sPosition.iZ >> 8;

					ZeroMemory( psNPC, sizeof( PacketUnitInfo ) );

					bFound = TRUE;
					break;
				}
			}

			if ( bFound == FALSE )
			{
				CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> NPC remove error for NPC ID: %d. Not a NPC?", npcID);
				return TRUE;
			}
			else
			{
				MAPSERVER->RemoveNPC(psMap, npcID);

				SQLConnection * pcDB = SQLCONNECTION( DATABASEID_GameDB_Npc, 4 );

				if ( pcDB->Open() )
				{
					if ( pcDB->Prepare( "DELETE FROM MapNPC WHERE Stage=? AND X=? AND Z=?" ) )
					{
						pcDB->BindParameterInput( 1, PARAMTYPE_Integer, &currentStageId );
						pcDB->BindParameterInput( 2, PARAMTYPE_Integer, &iX );
						pcDB->BindParameterInput( 3, PARAMTYPE_Integer, &iZ );

						if ( !pcDB->Execute() )
						{
							WARN( "Remove NPC failed. Stage: %d, X: %d, Z: %d", currentStageId, iX, iZ );
						}
					}

					pcDB->Close();
				}

				CHATSERVER->SendChatAllGM("GM> NPC removed from Stage ID(%d) AT X(%d) Z(%d)", currentStageId, iX, iZ );
			}
		}
		else
		{
			CHATSERVER->SendChat( pcUser, CHATCOLOR_Error, "> Expecting an id for first param" );
		}

		return TRUE;
	}


	//Add NPC
	if (iLen = COMMAND("/add_npc", pszBuff))
	{
		if (GetParameterString(pszBuff, 1, szCommandParam1) &&
			GetParameterString(pszBuff, 2, szCommandParam2) &&
			GetParameterString(pszBuff, 3, szCommandParam3) &&
			GetParameterString(pszBuff, 4, szCommandParam4) &&
			GetParameterString(pszBuff, 5, szCommandParam5))
		{
			int currentStageId = pcUserData->iMapID;
			int dbNpcId = atoi(szCommandParam1);
			int iX = atoi(szCommandParam2);
			int iY = atoi(szCommandParam3);
			int iZ = atoi(szCommandParam4);
			int iAngle = atoi(szCommandParam5);
			int enabled = 1;
			int onlyGM = 0;

			SQLConnection* pcDB = SQLCONNECTION(DATABASEID_GameDB_Npc, 5);

			int npcCount = 0;
			if (pcDB->Open())
			{
				if (pcDB->Prepare("SELECT COUNT(ID) FROM NPCList WHERE ID=?"))
				{
					pcDB->BindParameterInput(1, PARAMTYPE_Integer, &dbNpcId);
					if (pcDB->Execute() && pcDB->Fetch())
					{
						pcDB->GetData(1, PARAMTYPE_Integer, &npcCount);
					}
				}

				if (npcCount == 1)
				{
					pcDB->Clear();

					if (pcDB->Prepare("INSERT INTO MapNPC VALUES (?,?,?,?,?,?,?,?)"))
					{
						pcDB->BindParameterInput(1, PARAMTYPE_Integer, &pcUserData->iMapID); //Stage
						pcDB->BindParameterInput(2, PARAMTYPE_Integer, &iX);
						pcDB->BindParameterInput(3, PARAMTYPE_Integer, &iY);
						pcDB->BindParameterInput(4, PARAMTYPE_Integer, &iZ);
						pcDB->BindParameterInput(5, PARAMTYPE_Integer, &iAngle);
						pcDB->BindParameterInput(6, PARAMTYPE_Integer, &dbNpcId); //IDNPC
						pcDB->BindParameterInput(7, PARAMTYPE_Integer, &enabled); //Enabled
						pcDB->BindParameterInput(8, PARAMTYPE_Integer, &onlyGM);  //OnlyGM
						pcDB->Execute();
					}

					pcDB->Close();

					bool found = false;

					for (UINT i = 0; i < MAX_NPCINMAP; i++)
					{
						PacketUnitInfo* psNPC = MAPSDATA->saNPC + i;

						//found next empty one
						if ((psNPC->iHeader == 0))
						{
							psNPC[i].sCharacterData.iClanID = onlyGM;

							std::string s = std::to_string(dbNpcId);
							strcpy_s(psNPC[i].sCharacterData.szName, 32, s.c_str());

							// Packet structure
							psNPC[i].iLength = sizeof(PacketUnitInfo);
							psNPC[i].iHeader = PKTHDR_NpcData;
							psNPC[i].sCharacterData.saUnused = 200; // ???
							psNPC[i].sCharacterData.sHP.sCur = 100; // HP default
							psNPC[i].sCharacterData.sHP.sMax = 100; // HP default
							psNPC[i].iID = 8100; // Default ID
							psNPC[i].sPosition.iX = iX * 256; // X << 8
							psNPC[i].sPosition.iY = iY * 256; // Y << 8
							psNPC[i].sPosition.iZ = iZ * 256; // Z << 8
							psNPC[i].sAngle.iX = 0;
							psNPC[i].sAngle.iY = iAngle;
							psNPC[i].sAngle.iZ = 0;

							MAPSERVER->SpawnNPC(MAPSDATA + currentStageId, &psNPC[i]);

							CHATSERVER->SendChatAllGM("GM> NPC ID(%d) added to stage ID(%d) AT X(%d) Z(%d)", dbNpcId, pcUserData->iMapID, iX, iZ);

							found = true;
							break;
						}
					}

					if (!found)
					{
						CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> NPC ID(%d) wasn't added to stage ID(%d). MAX reached!", dbNpcId, pcUserData->iMapID);
					}
				}
				else
				{
					pcDB->Close();
					CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> NPC ID(%d) does not exist!", dbNpcId);
				}
			}
		}
	}

	//Update Position
	if (iLen = COMMAND("/set_pos", pszBuff))
	{
		// Is User Alive?
		if (pcUserData)
		{
			if (GetParameterString(pszBuff, 1, szCommandParam1) &&
				GetParameterString(pszBuff, 2, szCommandParam2) &&
				GetParameterString(pszBuff, 3, szCommandParam3) &&
				GetParameterString(pszBuff, 4, szCommandParam4))
			{
				int iID = atoi(szCommandParam1);
				int iX = atoi(szCommandParam2);
				int iY = atoi(szCommandParam3);
				int iZ = atoi(szCommandParam4);

				UnitData* pcUnitData = UNITSERVER->GetUnit(iID);

				if (pcUnitData && MAPSERVER->UpdateUnitPosition(pcUnitData, pcUserData->iMapID, iX, iY, iZ))
				{
					CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> %s ID(%d) X(%d) Y(%d) Z(%d)", pcUnitData->sCharacterData.szName, iID, iX, iY, iZ);
				}
			}
		}
		return TRUE;
	}

	//Update Angle
	if (iLen = COMMAND("/set_ang", pszBuff))
	{
		// Is User Alive?
		if (pcUserData)
		{
			if (GetParameterString(pszBuff, 1, szCommandParam1) &&
				GetParameterString(pszBuff, 2, szCommandParam2))
			{
				int iID = atoi(szCommandParam1);
				int iAngle = atoi(szCommandParam2);

				UnitData* pcUnitData = UNITSERVER->GetUnit(iID);

				if (pcUnitData && MAPSERVER->UpdateUnitAngle(pcUnitData, pcUserData->iMapID, iAngle))
				{
					CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> %s ID(%d) Angle(%d)", pcUnitData->sCharacterData.szName, iID, iAngle);
				}
			}
		}
		return TRUE;
	}



	if (iLen = COMMAND("/test_gold_drop", pszBuff))
	{
		if (GetParameterString(pszBuff, 1, szCommandParam1))
		{
			int iTotalNumDrops = atoi(szCommandParam1);

			if (iTotalNumDrops < 0)
				iTotalNumDrops = 1;
			else if (iTotalNumDrops > 300)
				iTotalNumDrops = 300;

			MAPSERVER->AddMassGoldDrops(MAPSDATA + pcUser->GetMapID(),
				pcUser->pcUserData->sPosition.iX >> 8,
				pcUser->pcUserData->sPosition.iY >> 8,
				pcUser->pcUserData->sPosition.iZ >> 8,
				pcUser->pcUserData->sCharacterData.iLevel * 10,
				pcUser->pcUserData->sCharacterData.iLevel * 15,
				iTotalNumDrops,
				pcUser);
		}
	}

	if (iLen = COMMAND("/test_MovetoMe", pszBuff))
	{
		// Is User Alive?
		if (pcUserData)
		{
			GetParameterString(pszBuff, 1, szCommandParam1);
			GetParameterString(pszBuff, 2, szCommandParam2);
			UnitData* pcUnitData = UNITSERVER->GetUnit(atoi(szCommandParam1));

			int command = atoi(szCommandParam2);

			if (pcUnitData)
			{
				if (command == 1)
				{
					//int iX = (pcUnitData->sPosition.iX - pcUnitData->sMoveBeginPosition.iX) >> 8;
					//int iZ = (pcUnitData->sPosition.iZ - pcUnitData->sMoveBeginPosition.iZ) >> 8;

					//int iDistance = (iX * iX) + (iZ * iZ);

					pcUnitData->pcAttackTarget = NULL;
					pcUnitData->pcFocusTarget = NULL;
					pcUnitData->pcFocusingTarget = NULL;
					pcUnitData->iFocusingLastID = -1;
					pcUnitData->dwIgnoreOthersTime = TICKCOUNT + 3000;
					//pcUnitData->bRunMode = TRUE;
					pcUnitData->iTargetMoveCount = 6;
					pcUnitData->iTargetTraceMode = FALSE;
					pcUnitData->bCheckCollision = FALSE;
					pcUnitData->sMoveToPosition = Point3D(pcUserData->sPosition.iX, pcUserData->sPosition.iY, pcUserData->sPosition.iZ);

					UNITDATATOUNIT(pcUnitData)->bReturningToSpawn = TRUE;
					UNITDATATOUNIT(pcUnitData)->dwReturningToSpawnTimeout = TICKCOUNT + 20000;
					UNITDATATOUNIT(pcUnitData)->sReturningToSpawnPosition = Point3D(pcUserData->sPosition.iX, pcUserData->sPosition.iY, pcUserData->sPosition.iZ);

					pcUnitData->NextTarget(0, 0, 0);
					pcUnitData->SetAngleByPosition(pcUserData->sPosition.iX >> 8, pcUserData->sPosition.iZ >> 8);
					pcUnitData->MoveAngle(4);

					pcUnitData->Animate(ANIMATIONTYPE_Running);
				}
			}
		}
	}

	// Kill Enemy
	if (iLen = COMMAND("/killmon", pszBuff))
	{
		// Is User Alive?
		if (pcUserData)
		{
			GetParameterString(pszBuff, 1, szCommandParam1);
			UnitData* pcUnitData = UNITSERVER->GetUnit(atoi(szCommandParam1));

			// Is Unit Alive?
			if (pcUnitData)
			{
				// Kill Unit
				pcUnitData->SetCurrentHealth( 0 );

				if (pcUnitData->Animate( ANIMATIONTYPE_Die ))
					CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> '%s' Killed!", pcUnitData->sCharacterData.szName);
			}
		}
		return TRUE;
	}
	// Set HP Enemy
	if (iLen = COMMAND("/set_hp", pszBuff))
	{
		// Is User Alive?
		if (pcUserData)
		{
			GetParameterString(pszBuff, 1, szCommandParam1); // ID
			GetParameterString(pszBuff, 2, szCommandParam2); // HP

			UnitData* pcUnitData = UNITSERVER->GetUnit(atoi(szCommandParam1));
			short sEnemyHP = (short)atoi(szCommandParam2);

			// Is more than 30000 or minor than 0?
			if (sEnemyHP < 1)
			{
				CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> HP value is less than 1!");
				return TRUE;
			}

			// Is Unit Alive?
			if (pcUnitData)
			{
				int iOldEnemyHP = pcUnitData->GetCurrentHealth();

				// Change HP Unit
				pcUnitData->SetCurrentHealth( sEnemyHP );

				CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> '%s' HP ( %d -> %d )!", pcUnitData->sCharacterData.szName, iOldEnemyHP, sEnemyHP);
			}
		}
		return TRUE;
	}

	// Set HP Enemy
	if (iLen = COMMAND("/set_abs", pszBuff))
	{
		// Is User Alive?
		if (pcUserData)
		{
			GetParameterString(pszBuff, 1, szCommandParam1);
			GetParameterString(pszBuff, 2, szCommandParam2);

			UnitData* pcUnitData = UNITSERVER->GetUnit(atoi(szCommandParam1));
			short sEnemyAbs = (short)atoi(szCommandParam2);

			// Is more than 30000 or minor than 0?
			if (sEnemyAbs < 0)
			{
				CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> No more than 30k Abs or minor than 0!");
				return TRUE;
			}

			// Is Unit Alive?
			if (pcUnitData)
			{
				// Change HP Unit
				short sOldEnemyAbs = pcUnitData->sCharacterData.iAbsorbRating;
				pcUnitData->sCharacterData.iAbsorbRating = sEnemyAbs;

				CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> '%s' Absorption ( %d -> %d )!", pcUnitData->sCharacterData.szName, sOldEnemyAbs, sEnemyAbs);
			}
		}
		return TRUE;
	}

	// Set Damage Enemy
	if (iLen = COMMAND("/set_dmg", pszBuff))
	{
		// Is User Alive?
		if (pcUserData)
		{

			GetParameterString(pszBuff, 1, szCommandParam1);
			GetParameterString(pszBuff, 2, szCommandParam2);
			GetParameterString(pszBuff, 3, szCommandParam3);

			UnitData* pcUnitData = UNITSERVER->GetUnit(atoi(szCommandParam1));
			short sDamageMin = (short)atoi(szCommandParam2);
			short sDamageMax = (short)atoi(szCommandParam3);

			// Is more than 30000 or minor than 0?
			if (sDamageMin < 0 || sDamageMax < 0)
			{
				CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> No more than 30k HP or minor than 0!");
				return TRUE;
			}

			// Is Unit Alive?
			if (pcUnitData)
			{
				// Change HP Unit
				short sOldDamageMin = pcUnitData->sCharacterData.iMinDamage;
				short sOldDamageMax = pcUnitData->sCharacterData.iMaxDamage;
				pcUnitData->sCharacterData.iMinDamage = sDamageMin;
				pcUnitData->sCharacterData.iMaxDamage = sDamageMax;

				CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> '%s' Damage ( %d-%d -> %d-%d )!", pcUnitData->sCharacterData.szName, sOldDamageMin, sOldDamageMax, sDamageMin, sDamageMax);
			}
		}
		return TRUE;
	}

	// Kill Character
	if (COMMAND("/killch", pszBuff) || COMMAND("/!killch", pszBuff))
	{
		// Is User Alive?
		if (pcUserData)
		{
			if (GetParameterString(pszBuff, 1, szCommandParam1))
			{
				UserData* pcUserKilled = USERSERVER->GetUserdata(szCommandParam1);
				if (pcUserKilled)
				{
					PacketSimple sPacket;
					sPacket.iLength = sizeof(PacketSimple);
					sPacket.iHeader = PKTHDR_TheDeath;
					sPacket.iUnk = 0;
					SENDPACKET(USERDATATOUSER(pcUserKilled), &sPacket);

					CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> %s Killed!", GAME_SERVER ? pcUserKilled->sCharacterData.szName : pcUserKilled->szCharacterName);
				}
				else
				{
					if (GAME_SERVER)
						CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Try use /!killch");
					else
						CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Try use /killch");
				}
			}
			else
				CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Usage: /(!)killch <chname>");
		}
		return TRUE;
	}

	if (iLen = COMMAND("/!ResetPVPRank", pszBuff))
	{
		PVPSERVER->ResetTick();
		CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> PvP Rank resetted!");
	}

	// Set Defense Enemy
	if (iLen = COMMAND("/set_def", pszBuff))
	{
		// Is User Alive?
		if (pcUserData)
		{
			GetParameterString(pszBuff, 1, szCommandParam1);
			GetParameterString(pszBuff, 2, szCommandParam2);

			UnitData* pcUnitData = UNITSERVER->GetUnit(atoi(szCommandParam1));
			int iDefense = atoi(szCommandParam2);

			// Is minor than 0?
			if (iDefense < 0)
			{
				CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> No minor than 0!");
				return TRUE;
			}

			// Is Unit Alive?
			if (pcUnitData)
			{
				// Change HP Unit
				int iOldDefense = pcUnitData->sCharacterData.iDefenseRating;
				pcUnitData->sCharacterData.iDefenseRating = iDefense;

				CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> '%s' Defense ( %d -> %d )!", pcUnitData->sCharacterData.szName, iOldDefense, iDefense);
			}
		}
		return TRUE;
	}

	// Set Level Enemy
	if (iLen = COMMAND("/set_lvl", pszBuff))
	{
		// Is User Alive?
		if (pcUserData)
		{
			GetParameterString(pszBuff, 1, szCommandParam1);
			GetParameterString(pszBuff, 2, szCommandParam2);

			UnitData* pcUnitData = UNITSERVER->GetUnit(atoi(szCommandParam1));
			int iLevel = atoi(szCommandParam2);

			// Is minor than 0?
			if (iLevel < 0)
			{
				CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> No minor than 0!");
				return TRUE;
			}

			// Is Unit Alive?
			if (pcUnitData)
			{
				// Change HP Unit
				int iOldLevel = pcUnitData->sCharacterData.iLevel;
				pcUnitData->sCharacterData.iLevel = iLevel;

				CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> '%s' Level ( %d -> %d )!", pcUnitData->sCharacterData.szName, iOldLevel, iLevel);
			}
		}
		return TRUE;
	}

	// Create a pet with warehouse function
	if (iLen = COMMAND("/petwh", pszBuff))
	{
		GetParameterString(pszBuff, 1, szCommandParam1);

		// No have name?
		if (szCommandParam1[0] == 0)
			return TRUE;

		// Create Unit?
		UnitData* pcUnitData = UNITSERVER->CreateUnitEnemy(szCommandParam1, pcUserData->sPosition.iX, pcUserData->sPosition.iY, pcUserData->sPosition.iZ);
		if (pcUnitData)
		{
			char szNewNamePet[32] = { 0 };
			STRINGFORMAT(szNewNamePet, "%s's WH Pet", pcUserData->sCharacterData.szName);
			STRINGCOPY(pcUnitData->sCharacterData.szName, szNewNamePet);
			pcUnitData->sCharacterData.iType = CHARACTERTYPE_NPC;
			pcUnitData->sUnitInfo.WareHouseMasterNPC = TRUE;
			pcUnitData->pcExclusiveTarget = pcUserData;
			pcUnitData->lExclusiveTargetID = pcUserData->iID;
			pcUnitData->Animate(ANIMATIONTYPE_Idle);
			pcUnitData->sCharacterData.iAbsorbRating = 2000000000;
			pcUnitData->pcMapSpawn = NULL;
			pcUnitData->dwUpdateCharInfoTime = 0;
		}
	}
	if (iLen = COMMAND("/Summon_pet", pszBuff))
	{
		PETSYSTEMHANDLER->CreatePet(pcUserData, 1);
	}
	// Create a pet with warehouse function
	if (iLen = COMMAND("/TestCrash", pszBuff))
	{
		GetParameterString(pszBuff, 1, szCommandParam1);

		// No have name?
		if (szCommandParam1[0] == 0)
			return TRUE;

		// Create Unit?
		UnitData* pcUnitData = UNITSERVER->CreateUnitEnemy("Skeleton Warrior", pcUserData->sPosition.iX, pcUserData->sPosition.iY, pcUserData->sPosition.iZ);
		if (pcUnitData)
		{
			Point3D sp(0xFFCBDC22, 0x646E, 0xFF99628A);
			Point3D sa(0, 0x0546, 0);
			Point3D sn(0, -1, 0x0042B69A);

			CALLT_WITH_ARG7(0x0046A650, (DWORD)pcUnitData->pcMapSpawn, (DWORD)&sp, (DWORD)&sa, (DWORD)&sn, 0x200, 0x34A0, 0x3EA5, 0);
		}
	}

	// Move
	if (COMMAND("/!getfield", pszBuff))
	{
		// Is User Alive?
		if (pcUserData)
		{
			CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> Field [%d]", pcUserData->iMapID);
			return TRUE;
		}
	}

	if (COMMAND("/debug", pszBuff))
	{
		// Is User Alive?
		if (pcUserData)
		{
			User* pcUser = USERDATATOUSER(pcUserData);
			if (pcUser)
			{
				pcUser->bDebugChat = pcUser->bDebugChat ? FALSE : TRUE;
				CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> Debug [%s]", pcUser->bDebugChat ? "ON" : "OFF");
				return TRUE;
			}
		}
	}

	if (COMMAND("/lootdebug", pszBuff))
	{
		if (pcUserData)
		{
			User* pcUser = USERDATATOUSER(pcUserData);
			if (pcUser)
			{
				LOOTSERVER->bLootDebug = !LOOTSERVER->bLootDebug;
				CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> Loot Debug [%s]", LOOTSERVER->bLootDebug ? "ON" : "OFF");
				return TRUE;
			}
		}
	}

	if ( COMMAND("/disable_errors_relay", pszBuff) )
	{
		if ( pcUserData )
		{
			User* pcUser = USERDATATOUSER(pcUserData);
			if ( pcUser )
			{
				pcUser->bDisableWarnAndErrorRelaysToClient = pcUser->bDisableWarnAndErrorRelaysToClient ? FALSE : TRUE;
				CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> Errors relay [%s]", pcUser->bDisableWarnAndErrorRelaysToClient ? "ON" : "OFF");
				return TRUE;
			}
		}
	}

	if (COMMAND("/getclan", pszBuff))
	{
		// Is User Alive?
		if (pcUserData)
		{
			if (GetParameterString(pszBuff, 1, szCommandParam1))
			{
				UserData* pcUserClan = USERSERVER->GetUserdata(szCommandParam1);
				if (pcUserClan)
				{
					CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> Clan [%d]", pcUserClan->iClanID);
				}
			}
		}
		return TRUE;
	}

	if (iLen = COMMAND("/sod_update_crowns", pszBuff))
	{
		BELLATRASERVER->SyncOldClanTablesWithNew();
		BELLATRASERVER->CheckAndUpdateClanTopScore();
		BELLATRASERVER->UpdateCrownClanIDs();
		BELLATRASERVER->SendFullCrownDataToAllUsers();
	}


	if (iLen = COMMAND("/Meteor", pszBuff))
	{
		// Is User Alive?
		if (pcUserData)
		{
			PacketFireMeteorite sPacket;
			sPacket.iLength = sizeof(PacketFireMeteorite);
			sPacket.iHeader = PKTHDR_FireMeteorite;
			sPacket.bSmallMeteor = FALSE;
			sPacket.iDelay = Dice::RandomI(50, 251);
			sPacket.sBegin.iX = pcUserData->sPosition.iX - 8092;
			sPacket.sBegin.iY = pcUserData->sPosition.iY + 150000;
			sPacket.sBegin.iZ = pcUserData->sPosition.iZ - 8092;
			sPacket.sEnd.iX = pcUserData->sPosition.iX + 8092;
			sPacket.sEnd.iY = pcUserData->sPosition.iY;
			sPacket.sEnd.iZ = pcUserData->sPosition.iZ + 8092;
			sPacket.iCount = Dice::RandomI(1, 3);
			SENDPACKETRANGE(&sPacket, (Point3D*)&pcUserData->sPosition, pcUserData->iMapID, DISTANCE_XY_33_meters);

			if (GetParameterString(pszBuff, 1, szCommandParam1) && GetParameterString(pszBuff, 2, szCommandParam2))
			{
				// Structure Packet to Quick Damage
				PacketDamageQuick sPDM;
				sPDM.iLength = sizeof(PacketDamageQuick);
				sPDM.iHeader = PKTHDR_DamageQuick;
				sPDM.bPercentile = FALSE;
				sPDM.bBaseCurrentHP = FALSE;
				sPDM.iDamage = atoi(szCommandParam1);
				SENDPACKETRANGE(&sPDM, (Point3D*)&pcUserData->sPosition, pcUserData->iMapID, (atoi(szCommandParam2) * 27));
			}
		}
	}

	if (iLen = COMMAND("/print_clan_id", pszBuff))
	{
		INFO("CLANID = %d", pcUserData->iClanID);
	}

	//Enable Fury Event
	if (iLen = COMMAND("/FuryArena_EnableEvent", pszBuff))
	{
		INFO("FuryArena_EnableEvent");

		if (GetParameterString(pszBuff, 1, szCommandParam1))
		{
			if (STRINGCOMPAREI("true", szCommandParam1))
			{
				FURYARENAHANDLER->SetEnable(TRUE);
				CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> The Fury Arena Event is enabled!");
			}
			else if (STRINGCOMPAREI("false", szCommandParam1))
			{
				FURYARENAHANDLER->SetEnable(FALSE);
				CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> The Fury Arena Event is disabled!");
			}
			else
				CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Use: /FuryArena_EnableEvent <true|false>");
		}
		else
			CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Use: /FuryArena_EnableEvent <true|false>");
	}

	//Clear tracking data used to track how many times player have received rewards from Fury Arena
	if (iLen = COMMAND("/FuryArena_ClearRewardTracker", pszBuff))
	{
		FURYARENAHANDLER->SQLClearFuryArenaRewardTracker();
		CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Reward Tracker is cleared!");
	}

	//Force start Fury in test mode, but with low HP (1 hit kills)
	if (iLen = COMMAND("/FuryArena_ForceStartTestMode", pszBuff))
	{
		if (FURYARENAHANDLER->IsStarted() == FALSE)
		{
			if (GetParameterString(pszBuff, 1, szCommandParam1))
			{
				int iEventID = atoi(szCommandParam1);

				if (iEventID == 0 ||
					iEventID == 1)
				{
					FURYARENAHANDLER->ForceStartEventTestMode(iEventID);
				}
				else
				{
					CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Expecting 0 or 1 for EventId");
				}
			}
			else
			{
				CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Use: /FuryArena_ForceStartTestMode <EventId>");
			}
		}
		else
		{
			CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Fury Arena is already started!");
		}
	}

	//Force start Fury, but with normal HP etc
	else if (iLen = COMMAND("/FuryArena_ForceStart", pszBuff))
	{
		if (FURYARENAHANDLER->IsStarted() == FALSE)
		{
			if (GetParameterString(pszBuff, 1, szCommandParam1))
			{
				int iEventID = atoi(szCommandParam1);

				if (iEventID == 0 ||
					iEventID == 1)
				{
					FURYARENAHANDLER->ForceStartEvent(iEventID);
				}
				else
				{
					CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Expecting 0 or 1 for EventId");
				}
			}
			else
			{
				CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Use: /FuryArena_ForceStart <EventId>");
			}
		}
		else
		{
			CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Fury Arena is already started!");
		}
	}

	else if (iLen = COMMAND("/FuryArena_End", pszBuff))
	{
		if (FURYARENAHANDLER->IsStarted())
		{
			FURYARENAHANDLER->EndEvent();
			CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Fury Arena is ended!");
		}
		else
			CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Fury Arena is already ended!");
	}


	if (iLen = COMMAND("/gf_call", pszBuff))
	{
		if (GetParameterString(pszBuff, 1, szCommandParam1))
		{
			User* pcUser = USERDATATOUSER(USERSERVER->GetUserdata(szCommandParam1));
			// Is User Alive?
			if (pcUser)
			{
				// Enter to arena
				FURYARENAHANDLER->EnterFromArena(pcUser);
			}
		}
		else
			CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Use: /gf_call <name>");
	}

	if (iLen = COMMAND("/gf_time", pszBuff))
	{
		FURYARENAHANDLER->UpdateTime();
		CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "TGF> Time Updated!");
	}

	if (iLen = COMMAND("/gf_gettick", pszBuff))
	{
		CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "TGF> Tick: %d", FURYARENAHANDLER->GetTick());
	}

	if (iLen = COMMAND("/gf_getstone", pszBuff))
	{
		if (GetParameterString(pszBuff, 1, szCommandParam1) && GetParameterString(pszBuff, 2, szCommandParam2))
		{
			DefinitionItem* psDef = ITEMSERVER->FindItemDefByCode(ITEMID_KingStone);
			if (psDef)
			{
				Item sItem;
				ITEMSERVER->CreateItem(&sItem, psDef, EItemSource::GameMaster);
				sItem.eMixTypeName = (EMixTypeName)atoi(szCommandParam1);
				sItem.tTime = atoi(szCommandParam2);
				ITEMSERVER->ValidateItemTime(&sItem);
				ITEMSERVER->SendItemData(pcUserData, &sItem);
			}
		}
	}


	if (iLen = COMMAND("/gf_timeminion", pszBuff))
	{
		if (GetParameterString(pszBuff, 1, szCommandParam1))
		{
			if (FURYARENAHANDLER->SetTimeMinion(atoi(szCommandParam1)))
			{
				CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> The Grand Fury Time Minion: %d", atoi(szCommandParam1));
			}
		}
		else
			CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Use: /gf_timeminion <minutes>");
	}

	if (iLen = COMMAND("/gf_boss", pszBuff))
	{
		if (GetParameterString(pszBuff, 1, szCommandParam1))
		{
			if (FURYARENAHANDLER->SetBoss(szCommandParam1))
			{
				CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> The Grand Fury Boss: %s", szCommandParam1);
			}
		}
		else
			CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Use: /gf_boss <name>");
	}

	if (iLen = COMMAND("/gf_minion", pszBuff))
	{
		if (GetParameterString(pszBuff, 1, szCommandParam1))
		{
			if (FURYARENAHANDLER->SetMinion(szCommandParam1))
			{
				CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> The Grand Fury Minion: %s", szCommandParam1);
			}
		}
		else
			CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Use: /gf_minion <name>");
	}

	if (iLen = COMMAND("/gf_monevent", pszBuff))
	{
		if (GetParameterString(pszBuff, 1, szCommandParam1))
		{
			if (FURYARENAHANDLER->SetMonsterEvent(szCommandParam1))
			{
				CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> The Grand Fury Monster Event: %s", szCommandParam1);
			}
		}
		else
			CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Use: /gf_monevent <name>");
	}

	if (iLen = COMMAND("/gf_kick", pszBuff))
	{
		if (GetParameterString(pszBuff, 1, szCommandParam1))
		{
			User* pcUser = USERDATATOUSER(USERSERVER->GetUserdata(szCommandParam1));
			// Is User Alive?
			if (pcUser)
			{
				// Enter to arena
				FURYARENAHANDLER->LeaveFromArena(pcUser);
			}
		}
		else
			CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Use: /gf_kick <name>");
	}


	if (iLen = COMMAND("/SetPacketUnit", pszBuff))
	{
		if (GetParameterString(pszBuff, 1, szCommandParam1))
		{
			CHATSERVER->SendChatEx(pcUser, EChatColor::CHATCOLOR_Error, "> Packet Max: %d -> %d", iUpdateMaxPacket, atoi(szCommandParam1));
			iUpdateMaxPacket = atoi(szCommandParam1);
		}
	}

	if (iLen = COMMAND("/SetMaskUnit", pszBuff))
	{
		if (GetParameterString(pszBuff, 1, szCommandParam1))
		{
			CHATSERVER->SendChatEx(pcUser, EChatColor::CHATCOLOR_Error, "> Mask Max: %d -> %d", iUpdateMaxUnitMask, atoi(szCommandParam1));
			iUpdateMaxUnitMask = atoi(szCommandParam1);
		}
	}

	if (iLen = COMMAND("/SetFrameCounterUnit", pszBuff))
	{
		if (GetParameterString(pszBuff, 1, szCommandParam1))
		{
			CHATSERVER->SendChatEx(pcUser, EChatColor::CHATCOLOR_Error, "> Frame Counter Max: %d -> %d", iUpdateFrameCounter, atoi(szCommandParam1));
			iUpdateFrameCounter = atoi(szCommandParam1);
		}
	}



	if (iLen = COMMAND("/!HWCombination", pszBuff))
	{
		if (GetParameterString(pszBuff, 1, szCommandParam1))
		{
			HALLOWEENHANDLER->SetCombinationID(atoi(szCommandParam1));
			CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> CombinationID ( %d )", HALLOWEENHANDLER->GetCombinationID());
		}
		else
			CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> CombinationID ( %d )", HALLOWEENHANDLER->GetCombinationID());
	}

	if (iLen = COMMAND("/kickall", pszBuff))
	{
		NETSERVER->SendCommandToLoginServer(PKTHDR_NetKickAll);
		return TRUE;
	}


	if (COMMAND("/StartMaintenance", pszBuff))
	{
		GetParameterString(pszBuff, 1, szCommandParam1);

		int iSeconds = 300;
		if (szCommandParam1[0] != 0)
			iSeconds = atoi(szCommandParam1);

		if (iSeconds > 0)
		{
			//CHATSERVER->SendChatAll( CHATCOLOR_Global, "Notice> Server Announcement!" );
			CHATSERVER->SendChatAll(CHATCOLOR_Global, "Notice> The server is restarting to update!");
			CHATSERVER->SendChatAll(CHATCOLOR_Global, "Notice> Check our Discord for more information");
			CHATSERVER->SendChatAll(CHATCOLOR_Global, "Notice> We'll be back shortly!");
			CHATSERVER->SendChatAll(CHATCOLOR_Global, "Notice> Server restart countdown started."/*, GAME_NAME**/);

			NETSERVER->SendCommandToLoginServer(PKTHDR_NetStartMaintenance, iSeconds);

			GSERVER->StartMaintenanceCountdown(iSeconds);

			CHATSERVER->SendChatAllGM("GM> Maintenance Countdown Started!");
		}
	}

	if (COMMAND("/StopMaintenance", pszBuff))
	{
		GSERVER->StopMaintenanceCountdown();

		NETSERVER->SendCommandToLoginServer(PKTHDR_NetStopMaintenance);

		CHATSERVER->SendChatAll(CHATCOLOR_Global, "Notice> Maintenance Countdown Cancelled");
		CHATSERVER->SendChatAllGM("GM> Maintenance Countdown stopped!");
	}

	if ( iLen = COMMAND( "/KillUnitsMap", pszBuff ) )
	{
		if ( pcUserData )
		{
			// Is User Alive?
			if ( pcUserData )
			{
				Map * pcMap = MAPSDATA + pcUserData->iMapID;

				if ( pcMap )
				{
					int iCount = 0;

					for ( int i = 0; i < MAX_ALIVEMONSTERS; i++ )
					{
						UnitData * pcUnitData = pcMap->pcaUnitData[i];

						// Is Unit Alive?
						if ( pcUnitData && pcUnitData->GetCurrentHealth() > 0 )
						{
							// Kill Unit
							pcUnitData->SetCurrentHealth( 0 );

							if ( pcUnitData->Animate( ANIMATIONTYPE_Die ) )
								iCount++;
						}
					}

					CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> Units Killed[%d] MapID[%d]", iCount, pcUserData->iMapID );
				}
			}
		}

		return TRUE;
	}

	return FALSE;
}

BOOL ServerCommand::OnGameMasterLevel1Command(User* pcUser, const char* pszBuff)
{
	UserData * pcUserData = pcUser->pcUserData;

	int iLen = 0;

	char szCommandParam1[256] = { 0 };
	char szCommandParam2[256] = { 0 };
	char szCommandParam3[256] = { 0 };
	char szCommandParam4[256] = { 0 };
	char szCommandParam5[256] = { 0 };

	if ( iLen = COMMAND( "/event_treasurehunting", pszBuff ) )
	{
		if ( GetParameterString( pszBuff, 1, szCommandParam1 ) )
		{
			if ( STRINGCOMPAREI( szCommandParam1, "true" ) || STRINGCOMPAREI( szCommandParam1, "on" ) )
			{
				pcUser->bTeasureHuntingMode = TRUE;
				CHATSERVER->SendChat(pcUser, CHATCOLOR_White, "> Treasure Hunting Mode on!");
				CHATSERVER->SendChat( pcUser, CHATCOLOR_Error, "> Any items that you drop will appear as a 'gift' and will last for 48 hours on ground, and can only be found via mouse cursor." );
			}
			else if ( STRINGCOMPAREI( szCommandParam1, "false" ) || STRINGCOMPAREI( szCommandParam1, "off" ) )
			{
				pcUser->bTeasureHuntingMode = FALSE;
				CHATSERVER->SendChat(pcUser, CHATCOLOR_White, "> Treasure Hunting Mode off!");
			}
			else
			{
				CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Usage: /event_treasurehunting <true|false>");
			}
		}
		else
		{
			CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Usage: /event_treasurehunting <true|false>");
		}

		return TRUE;
	}

	if (iLen = COMMAND( "/mute", pszBuff ))
	{
		CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Use /!mute command");  //it's so that the command is received on login server not game server
		return TRUE;
	}

	if (iLen = COMMAND( "/unmute", pszBuff ))
	{
		CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Use /!unmute command"); //it's so that the command is received on login server not game server
		return TRUE;
	}

	// Syntax: /!mute <char name> <reason>
	// note - /! is so that this command is received at login server
	else if (iLen = COMMAND("/!mute", pszBuff))
	{
		if (GAME_SERVER)
			return TRUE;

		// Expect all 2 Parameters to be given, else return error
		if (GetParameterString(pszBuff, 1, szCommandParam1) &&
			GetParameterString(pszBuff, 2, szCommandParam2))
		{
			if (GetParameterString( pszBuff, 3, szCommandParam3 ))
			{
				CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> /!mute failed! Expecting two parameters. Tip - surround the multi-worded \"<reason>\" with double quotes", szCommandParam1);
				return TRUE;
			}

			std::string szAccountName = CHARACTERSERVER->SQLGetAccountName(szCommandParam1);

			//account not found. character doesn't exist
			if (szAccountName.empty())
			{
				CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> /!mute failed! Character '%s' does not exist!", szCommandParam1);
				return TRUE;
			}

			//get info about account, including mute expiry, mute status and mute count
			SQLUser psSqlUser;
			ZeroMemory(&psSqlUser, sizeof(SQLUser));
			if (!ACCOUNTSERVER->GetSQLUserInfoData(szAccountName.c_str(), &psSqlUser))
			{
				CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> /!mute failed! Account '%s' does not exist!", szAccountName.c_str()); //shouldn't happen.. but let's check anyway
				return TRUE;
			}

			User* pcMuteUser = USERDATATOUSER(USERSERVER->GetUserdata(szCommandParam1));

			//Character is online
			if (pcMuteUser)
			{
				pcMuteUser->bMuted = TRUE;
				pcMuteUser->dwUnMuteExpiryTime = SystemTimeToDWORD(&psSqlUser.sUnmuteDate);
			}

			//player is already muted
			if (psSqlUser.bIsMuted)
			{
				CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> Account '%s' is already muted!", szAccountName.c_str()); //shouldn't happen.. but let's check anyway

				USERSERVER->SyncUserMuteStatus( pcMuteUser );

				return TRUE;
			}

			if (psSqlUser.iMuteCount < 0)
				psSqlUser.iMuteCount = 0;

			int iNewMuteCount = psSqlUser.iMuteCount + 1;
			int iMuteDurationHrs = 0;


			switch (iNewMuteCount)
			{
				case 1:   iMuteDurationHrs = 1;		 break;
				case 2:   iMuteDurationHrs = 3;		 break;
				case 3:   iMuteDurationHrs = 6;		 break;
				case 4:   iMuteDurationHrs = 12;	 break;
				case 5:   iMuteDurationHrs = 24;	 break;
				case 6:   iMuteDurationHrs = 24 * 7; break; //1 week
				case 7:   iMuteDurationHrs = 730;    break; //1 month
				case 8:   iMuteDurationHrs = 8760;   break; //1 year
				default:  iMuteDurationHrs = 87600;  break; //10 year
			}

			INFO( "Player '%s' muted (Account: '%s'). New Mute count: %d. Mute duration: %d hrs", szCommandParam1, szAccountName.c_str(), iNewMuteCount, iMuteDurationHrs );


			std::string reason = szCommandParam2;
			std::string reasonTrim = reason.substr(0, 50); //50 max length (otherwise insert query fails)

			auto pcDB = SQLCONNECTION(DATABASEID_UserDB_PrimaryServer);

			if (pcDB->Open())
			{
				// Update UserInfo with mute status and mute counter
				if (pcDB->Prepare( "UPDATE UserInfo SET IsMuted=1, MuteCount=?, UnmuteDate=DATEADD(hour, ?, GETDATE()) WHERE ID=?" ))
				{
					pcDB->BindParameterInput( 1, PARAMTYPE_Integer, &iNewMuteCount );
					pcDB->BindParameterInput( 2, PARAMTYPE_Integer, &iMuteDurationHrs );
					pcDB->BindParameterInput( 3, PARAMTYPE_Integer, &psSqlUser.iID );

					pcDB->ExecuteUpdate();
				}

				pcDB->Close();
			}

			// Switch selected Database...
			pcDB = SQLCONNECTION(DATABASEID_LogDB, 32);

			if (pcDB->Open())
			{
				// Write info to MutePlayerLog in the Chat Database - could be relocated if needed
				if (pcDB->Prepare("INSERT INTO MutePlayerLog ([AccountName],[CharacterName],[UnixMuteStart],[UnixMuteEnd],[MuteCount],[MuteReason],[MutedBy]) VALUES (?,?,?,?,?,?,?)"))
				{
					DWORD uMuteStart = GetUnixTime();
					DWORD uMuteEnd = GetUnixTime() + (iMuteDurationHrs * 60 * 60);

					pcDB->BindParameterInput(1, PARAMTYPE_String, (char*)szAccountName.c_str(), 32);
					pcDB->BindParameterInput(2, PARAMTYPE_String, szCommandParam1, 32);
					pcDB->BindParameterInput(3, PARAMTYPE_Integer, &uMuteStart);
					pcDB->BindParameterInput(4, PARAMTYPE_Integer, &uMuteEnd);
					pcDB->BindParameterInput(5, PARAMTYPE_Integer, &iNewMuteCount);
					pcDB->BindParameterInput(6, PARAMTYPE_String, (char*)reasonTrim.c_str(), 50);
					pcDB->BindParameterInput(7, PARAMTYPE_String, (char*)pcUser->pcUserData->szAccountName, 32);

					pcDB->Execute();
				}

				pcDB->Close();
			}

			if (pcMuteUser)
			{
				DWORD dwSystemTime = SYSTEMTIMEDWORD;

				pcMuteUser->bMuted = TRUE;
				pcMuteUser->dwUnMuteExpiryTime = dwSystemTime + (iMuteDurationHrs * 60 * 60);

				USERSERVER->SyncUserMuteStatus( pcMuteUser );

				if (iMuteDurationHrs > 8760)
				{
					// On success send message to chat
					CHATSERVER->SendChatEx( pcMuteUser, CHATCOLOR_Error, "> You have been permaently muted by a GM (strike # %d). Reason: %s", iMuteDurationHrs, iNewMuteCount, reasonTrim.c_str() );
					CHATSERVER->SendTitleBox( pcUser->pcUserData, "You are permanently muted!" );
				}
				else
				{
					// On success send message to chat
					CHATSERVER->SendChatEx( pcMuteUser, CHATCOLOR_Error, "> You have been muted by a GM for %d hours (strike # %d). Reason: %s", iMuteDurationHrs, iNewMuteCount, reasonTrim.c_str() );
					CHATSERVER->SendTitleBox( pcUser->pcUserData, "You are muted!" );
				}
			}


			CHATSERVER->SendChatAllGM( "GM> Player %s was muted for %d hours! (Strike # %d). Reason: \"%s\"", szCommandParam1, iMuteDurationHrs, iNewMuteCount, reasonTrim.c_str());
		}

		else
		{
			// If commands is incomplete, suggest the right format
			CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Usage: /!mute <char name> \"<Reason>\"");
		}

		return TRUE;
	}

	// Syntax: /!unmute <char name>
	// note - /! is so that this command is received at login server
	// note - calling this to unmute a player removes a strike
	else if ( iLen = COMMAND("/!unmute", pszBuff))
	{
		if (GAME_SERVER)
			return TRUE;

		if (GetParameterString( pszBuff, 1, szCommandParam1 ))
		{
			ACCOUNTSERVER->UnmuteAccountByCharacterName( pcUser, szCommandParam1, FALSE );
			return TRUE;
		}
		else
		{
			CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Usage: /!unmute <char name>");
		}

		return TRUE;
	}


	return FALSE;
}

BOOL ServerCommand::OnGameMasterLevel2Command( User * pcUser, const char * pszBuff )
{

	UserData * pcUserData = pcUser->pcUserData;

	int iLen = 0;

	char szCommandParam1[256] = { 0 };
	char szCommandParam2[256] = { 0 };
	char szCommandParam3[256] = { 0 };
	char szCommandParam4[256] = { 0 };
	char szCommandParam5[256] = { 0 };

	if ( pszBuff[0] == '/' && pszBuff[1] == '!' )
	{
		if ( pszBuff[2] == '>' )
		{
			if ( pszBuff[3] == '>' )
			{
				CHATSERVER->SendChatAllEx( CHATCOLOR_Global, "%s", (char*)&pszBuff[4] );
			}
			else
			{
				char * p = CHARACTERSERVER->GetCharacterName( pcUser );

				if ( p[0] != 0 )
					CHATSERVER->SendChatAllEx( CHATCOLOR_Global, "GM> %s", (char*)&pszBuff[3] );
			}
		}
		else if ( pszBuff[2] == '+' )
		{
			char * p = CHARACTERSERVER->GetCharacterName( pcUser );

			if ( p[0] != 0 )
				CHATSERVER->SendChatAllEx( CHATCOLOR_Global, "%s> %s", p, (char*)&pszBuff[3] );
		}
	}



	if ( COMMAND("/exprate", pszBuff) )
	{
		if ( GetParameterString ( pszBuff, 1, szCommandParam1 ) && GetParameterString(pszBuff, 2, szCommandParam2) )
		{
			TESTMAPHANDLER->GetExperienceRatePerFlag ( pcUser, szCommandParam1, szCommandParam2 );
		}
	}

	if (COMMAND("/BONUSALL", pszBuff))
	{
		for (int i=0; i < PLAYERS_MAX; i++)
		{
			if (USERSDATA[i].pcSocketData && USERSDATA[i].iMapID >= 0 && USERSDATA[i].iMapID < NUM_MAPS)
			{
				User* pcUser = USERDATATOUSER(&USERSDATA[i]);
				ITEMSERVER->CreatePremium(pcUser, ITEMID_SHPRegen, ITEMTIMERTYPE_SHPRegen, 600, true);
				ITEMSERVER->CreatePremium(pcUser, ITEMID_SSPRegen, ITEMTIMERTYPE_SSPRegen, 600, true);
				ITEMSERVER->CreatePremium(pcUser, ITEMID_SMPRegen, ITEMTIMERTYPE_SMPRegen, 600, true);
				ITEMSERVER->CreatePremium(pcUser, ITEMID_SMPDown, ITEMTIMERTYPE_SMPDown, 600, true);
				ITEMSERVER->CreatePremium(pcUser, ITEMID_SSPDown, ITEMTIMERTYPE_SSPDown, 600, true);
				ITEMSERVER->CreatePremium(pcUser, ITEMID_SBonusDamage, ITEMTIMERTYPE_SBonusDamage, 600, true);
				ITEMSERVER->CreatePremium(pcUser, ITEMID_SBonusAbsorb, ITEMTIMERTYPE_SBonusAbsorb, 600, true);
				ITEMSERVER->CreatePremium(pcUser, ITEMID_SMoveSpeed, ITEMTIMERTYPE_SMoveSpeed, 600, true);
				ITEMSERVER->CreatePremium(pcUser, ITEMID_SExpBuff, ITEMTIMERTYPE_SExpBuff, 600, true);
				ITEMSERVER->CreatePremium(pcUser, ITEMID_SDropBuff, ITEMTIMERTYPE_SDropBuff, 600, true);

				CHATSERVER->SendTitleBox(pcUserData, "> ALL Bonuses ACTIVE!");
			}
		}
		return TRUE;
	}

	if (COMMAND("/fieldlist1", pszBuff))
	{
		for ( int i = 0; i <= 30; i++ )
		{
			Map * pcMap = MAPSDATA + i;
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> Field %d: %s (Lv. %d)", i, pszaMapsName[i], pcMap->pcBaseMap->iLevel );
		}

		return TRUE;
	}

	if (COMMAND("/fieldlist2", pszBuff))
	{
		for ( int i = 31; i < NUM_MAPS; i++ )
		{
			Map * pcMap = MAPSDATA + i;
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> Field %d: %s (Lv. %d)", i, pszaMapsName[i], pcMap->pcBaseMap->iLevel );
		}

		return TRUE;
	}

	/*if (COMMAND("/field", pszBuff))
	{
	if (GetParameterString(pszBuff, 1, szCommandParam1))
	{
	int iMapId = atoi(szCommandParam1);
	if (iMapId >= 0 || iMapId <= 61)
	{
	CALL_WITH_ARG1((DWORD)0x004300C0, (DWORD)iMapId); //int WarpField2( int Num ) leaked code
	}
	else
	{
	CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Map id out of range (0 - 61)");
	return TRUE;
	}
	}

	CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Usage: /field <map id>");
	return TRUE;
	}*/
	else if (COMMAND("/wrap", pszBuff))
	{
		if (GetParameterString(pszBuff, 1, szCommandParam1) &&
			GetParameterString(pszBuff, 2, szCommandParam2) &&
			GetParameterString(pszBuff, 3, szCommandParam3))
		{
			int iMapId = atoi(szCommandParam1);

			if (iMapId >= 0 || iMapId < NUM_MAPS)
			{
				int x = atoi(szCommandParam2);
				int z = atoi(szCommandParam3);

				PacketWarpGateField sPacket;
				sPacket.iLength = sizeof(PacketWarpGateField);
				sPacket.iHeader = PKTHDR_WarpGateField;
				sPacket.iX = x;
				sPacket.iZ = z;
				sPacket.iStage = iMapId;
				SENDPACKET(pcUser->pcUserData, &sPacket);
			}
			else
			{
				CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Usage: /wrap <map id> <x> <z>");
			}
		}
		else
		{
			CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Usage: /wrap <map id> <x> <z>");
		}

		return TRUE;
	}

	//for Ashe's live streaming
	else if (COMMAND("/get_party_view", pszBuff))
	{
		// Is User Alive?

		std::vector<SQLSpawnCamera*> psList;

		SQLConnection* pcDB = SQLCONNECTION(DATABASEID_ServerDB, 21);
		if (pcDB->Open())
		{
			//usually better to define the column names here
			if (pcDB->Prepare("SELECT [MapNumber],[SpawnX],[SpawnY],[SpawnZ],[CamXCoord],[CamZCoord],[CamTurn],[CamAngle],[CamZoom],[MinPlayerCount],[SearchRadius],[SpawnType] FROM SpawnCamera"))
			{
				if (pcDB->Execute())
				{
					while (pcDB->Fetch())
					{

						SQLSpawnCamera* ps = new SQLSpawnCamera();
						ZeroMemory(ps, sizeof(SQLSpawnCamera));

						pcDB->GetData(1, PARAMTYPE_Integer, &ps->szMapNumber);
						pcDB->GetData(2, PARAMTYPE_Integer, &ps->szSpawn.iX);
						pcDB->GetData(3, PARAMTYPE_Integer, &ps->szSpawn.iY);
						pcDB->GetData(4, PARAMTYPE_Integer, &ps->szSpawn.iZ);
						pcDB->GetData(5, PARAMTYPE_Integer, &ps->szCamXCoord);
						pcDB->GetData(6, PARAMTYPE_Integer, &ps->szCamZCoord);
						pcDB->GetData(7, PARAMTYPE_Integer, &ps->szCamTurn);
						pcDB->GetData(8, PARAMTYPE_Integer, &ps->szCamAngle);
						pcDB->GetData(9, PARAMTYPE_Integer, &ps->szCamZoom);
						pcDB->GetData(10, PARAMTYPE_Integer, &ps->szMinPlayerCount);
						pcDB->GetData(11, PARAMTYPE_Integer, &ps->szSearchRadius);
						pcDB->GetData(12, PARAMTYPE_Integer, &ps->szSpawnType);


						psList.push_back(ps);
					}
				}
			}
			pcDB->Close();
		}

		std::vector<SQLSpawnCamera*> searchList;


		//Find number of players at each hell spawn
		for (int i = 0; i < PLAYERS_MAX; i++)
		{
			int iCount = 0;

			UserData* lcUserData = &USERSDATA[i];

			if (lcUserData && lcUserData->pcSocketData && lcUserData->iMapID >= 0 && lcUserData->iMapID < NUM_MAPS && lcUserData->iID != pcUserData->iID)
			{
				for (SQLSpawnCamera* ps : psList)
				{
					if (ps->szSpawn.WithinXZDistance(&lcUserData->sPosition, ps->szSearchRadius, DISTANCE_MISC_Y_EX))
					{
						ps->iPlayersPerSpawn += 1;
					}
				}
			}
		}

		for (SQLSpawnCamera* ps : psList)
		{
			if (ps->iPlayersPerSpawn > 0)
			{
				if (ps->iPlayersPerSpawn >= ps->szMinPlayerCount)
				{
					DEBUG("> Players in map %d HS : %d", ps->szMapNumber, ps->iPlayersPerSpawn);

					searchList.push_back(ps);
				}
			}
		}

		if (searchList.size() > 0)
		{

			int iRandomView = Dice::RandomI(0, searchList.size() - 1);

			//Send Packet to chagne view
			PacketWrapCamera sPacket;
			sPacket.iLength = sizeof(PacketWrapCamera);
			sPacket.iHeader = PKTHDR_WrapCamera;
			sPacket.iMap = searchList[iRandomView]->szMapNumber;
			sPacket.iX = searchList[iRandomView]->szCamXCoord;
			sPacket.iZ = searchList[iRandomView]->szCamZCoord;
			sPacket.iTurn = searchList[iRandomView]->szCamTurn;
			sPacket.iAngle = searchList[iRandomView]->szCamAngle;
			sPacket.iZoom = searchList[iRandomView]->szCamZoom;
			SENDPACKET(pcUser, &sPacket);


			if (sPacket.iMap >= 0 || sPacket.iMap <= 60)
			{
				PacketWarpGateField sPacketWrap;
				sPacketWrap.iLength = sizeof(PacketWarpGateField);
				sPacketWrap.iHeader = PKTHDR_WarpGateField;
				sPacketWrap.iX = sPacket.iX;
				sPacketWrap.iZ = sPacket.iZ;
				sPacketWrap.iStage = sPacket.iMap;
				SENDPACKET(pcUser->pcUserData, &sPacketWrap);
			}
		}

		for (SQLSpawnCamera* ps : psList)
		{
			SAFE_DELETE(ps); //delete pointer otherwise a leak will happen
		}

		psList.clear();
		searchList.clear();

		return TRUE;
	}



	// Teleport to near player
	else if (COMMAND("/near", pszBuff))
	{
		if (GetParameterString(pszBuff, 1, szCommandParam1))
		{
			UserData* pcUserWarp = USERDATABYNAME(szCommandParam1);
			if (pcUserWarp)
			{
				PacketWarpGateField sPacket;
				sPacket.iLength = sizeof(PacketWarpGateField);
				sPacket.iHeader = PKTHDR_WarpGateField;
				sPacket.iX = pcUserWarp->sPosition.iX;
				sPacket.iZ = pcUserWarp->sPosition.iZ;
				sPacket.iStage = pcUserWarp->iMapID;
				SENDPACKET(pcUser, &sPacket);
			}
			else
			{
				CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> /near failed! Character '%s' not found", szCommandParam1);
			}
		}
		else
		{
			CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Usage: /near <char name>");
		}

		return TRUE;
	}

	// Get user count in each map
	if ( COMMAND( "/mapusers", pszBuff ) )
	{
		// Is User Alive?
		if ( pcUser )
		{
			std::map<int, int> data;

			int iCount = 0;

			for (int i = 0; i < NUM_MAPS; i++)
			{
				data.insert({i, 0});
			}

			// Search player
			for ( int i = 0; i < PLAYERS_MAX; i++ )
			{
				// Found?
				if ( USERSDATA[i].pcSocketData && USERSDATA[i].iMapID >= 0 && USERSDATA[i].iMapID < NUM_MAPS )
				{
					data[USERSDATA[i].iMapID]++;
				}
			}

			for (auto kvp : data)
			{
				if (kvp.second > 0)
				{
					CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> Stage %d user count : %d", kvp.first, kvp.second);
				}
			}

			data.clear();
		}

		return TRUE;
	}

	// Send list of user names in map
	if ( COMMAND( "/mapchars", pszBuff ) )
	{
		// Is User Alive?
		if ( pcUser )
		{
			EMapID eThisMapId = pcUser->GetMapID();

			int counter = 0;

			// Search player
			for ( int i = 0; i < PLAYERS_MAX; i++ )
			{
				// Found?
				if ( USERSDATA[i].pcSocketData && USERSDATA[i].iMapID == eThisMapId )
				{
					counter++;
					CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> Map char #%d: '%s'", counter, CHARACTERSERVER->GetCharacterName( &USERSDATA[i] ) );
				}
			}
		}

		return TRUE;
	}


	// Kick Character
	if ( COMMAND( "/kickch", pszBuff ) || COMMAND( "/!kickch", pszBuff ) )
	{
		if ( GetParameterString( pszBuff, 1, szCommandParam1 ) )
		{
			UserData * pcUserKilled = USERSERVER->GetUserdata( szCommandParam1 );
			if ( pcUserKilled )
			{
				CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> %s Kicked!", GAME_SERVER ? pcUserKilled->sCharacterData.szName : pcUserKilled->szCharacterName );
				ACCOUNTSERVER->OnDisconnectUser( pcUserKilled );
				SENDPACKETBLANK( USERDATATOUSER( pcUserKilled ), PKTHDR_Disconnect, TRUE );
				pcUserKilled->dwDisconnectTime = TICKCOUNT + 1500;
				NETSERVER->DisconnectUser( pcUserKilled );
			}
			else
			{
				if ( GAME_SERVER )
					CHATSERVER->SendChat( pcUser, CHATCOLOR_Error, "> Try use /!kickch" );
				else
					CHATSERVER->SendChat( pcUser, CHATCOLOR_Error, "> Try use /kickch" );
			}
		}
		else
		{
			CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Usage: /(!)kickch <chname>");

		}

		return TRUE;
	}

	if (COMMAND("/grant_title", pszBuff))
	{
		if (!GetParameterString(pszBuff, 1, szCommandParam1))
		{
			CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Usage: /(!)grant_title <CharacterName> <Title> <Color Rank>");
			return TRUE;
		}
		if (!GetParameterString(pszBuff, 2, szCommandParam2))
		{
			CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Usage: /(!)grant_title <CharacterName> <Title> <Color Rank>");
			return TRUE;
		}
		if (!GetParameterString(pszBuff, 3, szCommandParam3))
		{
			CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Usage: /(!)grant_title <CharacterName> <Title> <Color Rank>");
			return TRUE;
		}

		const std::string charName(szCommandParam1);
		const std::string title(szCommandParam2);
		const int titleRarity = atoi(szCommandParam3);

		this->HandleGrantTitleCommand(charName, title, titleRarity);

		return TRUE;
	}


	return FALSE;
}

//TODO - tidy up GM commands for Lvl1, Lvl2, Lvl3, Lvl4
// so we can recruit level 1 GMs etc
BOOL ServerCommand::OnGameMasterLevel3Command(User* pcUser, const char* pszBuff)
{

	UserData * pcUserData = pcUser->pcUserData;

	int iLen = 0;

	char szCommandParam1[256] = { 0 };
	char szCommandParam2[256] = { 0 };
	char szCommandParam3[256] = { 0 };
	char szCommandParam4[256] = { 0 };
	char szCommandParam5[256] = { 0 };

	// Call specific character to port to you
	if (COMMAND("/call", pszBuff))
	{
		if (GetParameterString(pszBuff, 1, szCommandParam1))
		{
			UserData* pcUserWarp = USERDATABYNAME(szCommandParam1);
			if (pcUserWarp)
			{
				PacketWarpGateField sPacket;
				sPacket.iLength = sizeof(PacketWarpGateField);
				sPacket.iHeader = PKTHDR_WarpGateField;
				sPacket.iX = pcUser->pcUserData->sPosition.iX;
				sPacket.iZ = pcUser->pcUserData->sPosition.iZ;
				sPacket.iStage = pcUser->pcUserData->iMapID;
				SENDPACKET(USERDATATOUSER(pcUserWarp), &sPacket);
			}
			else
			{
				CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> /call failed! Character '%s' not found", szCommandParam1);
			}
		}
		else
		{
			CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> Usage: /call <char name>");
		}

		return TRUE;
	}

	if ( COMMAND( "/hide", pszBuff ) )
	{
		pcUserData->bHideMode = TRUE;
		CHATSERVER->SendChat( pcUser, EChatColor::CHATCOLOR_White, "> Hidden!" );
	}

	if ( COMMAND( "/show", pszBuff ) )
	{
		pcUserData->bHideMode = FALSE;
		CHATSERVER->SendChat( pcUser, EChatColor::CHATCOLOR_White, "> Shown!" );
	}

	// Ban Character
	else if (COMMAND("/banacc", pszBuff))
	{
		// Is User Alive?
		if (GetParameterString(pszBuff, 1, szCommandParam1) &&
			GetParameterString(pszBuff, 2, szCommandParam2))
		{
			UserData* pcUserBanned = USERSERVER->GetUserdata(szCommandParam1);
			if (pcUserBanned)
			{
				PacketLogCheatEx sPacket;
				sPacket.iCheatID = CHEATLOGID_GMReason;
				STRINGCOPY(sPacket.szParam, szCommandParam2);
				LOGSERVER->OnLogCheat(pcUserBanned, &sPacket);
				CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> %s was banned!", pcUserBanned->sCharacterData.szName);
			}
			else
				CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> %s not found!", szCommandParam1);
		}

		return TRUE;
	}

	// UnBan account
	if (COMMAND("/unbanacc", pszBuff))
	{
		if (GetParameterString(pszBuff, 1, szCommandParam1))
		{
			CHEATSERVER->UnbanAccount(szCommandParam1);
			CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> %s Unbanned!", szCommandParam1);
		}

		return TRUE;
	}

	return FALSE;
}


BOOL ServerCommand::OnPlayerCommand( User * pcUser, const char * pszBuff )
{
	int iLen = 0, iLen2 = 0;

	char szCommandParam1[256] = { 0 };
	char szCommandParam2[256] = { 0 };
	char szCommandParam3[256] = { 0 };
	char szCommandParam4[256] = { 0 };
	char szCommandParam5[256] = { 0 };

	if( GAME_SERVER )
	{
		//Emote Commands
		if( pszBuff[0] == 'e' && isdigit(pszBuff[1]) && lstrlenA(pszBuff) < 4 )
		{
			CHATSERVER->SendUserBoxChatRange( pcUser->pcUserData->sPosition.iX, pcUser->pcUserData->sPosition.iZ, 0x4B000, pcUser->GetID(), pszBuff );
			return TRUE;
		}

		//Party Chat
		if( pszBuff[0] == '@' )
		{
			if( pcUser )
			{
				if( (pcUser->bParty && pcUser->psParty) && pcUser->psParty->pcLeader )
				{
					if ( pcUser->bMuted )
					{
						INFO( "Player '%s' is muted", CHARACTERSERVER->GetCharacterName( pcUser->pcUserData ) );

						CHATSERVER->SendChat ( pcUser, CHATCOLOR_Error, "> You have been muted by a GM. You may not talk freely until your mute has expired." );
						CHATSERVER->SendTitleBox( pcUser->pcUserData, "You are muted!" );
						return TRUE;
					}

					std::string str( pszBuff );
					str.erase( 0, 1 );

					PARTYHANDLER->SendChatParty( pcUser, FormatString("%s: %s", pcUser->pcUserData->sCharacterData.szName, str.c_str()), CHATCOLOR_Party );
					CHATSERVER->RecordPartyChatToDatabase(pcUser->pcUserData, str.c_str());
				}
			}
			return TRUE;
		}
		else if( pszBuff[0] == '#' )
		{
			if( pcUser )
			{
				if( (pcUser->bParty && pcUser->psParty) && pcUser->psParty->pcLeader && pcUser->psParty->iRaidCount > 0 )
				{
					if ( pcUser->bMuted )
					{
						INFO( "Player '%s' is muted", CHARACTERSERVER->GetCharacterName( pcUser->pcUserData ) );

						CHATSERVER->SendChat ( pcUser, CHATCOLOR_Error, "> You have been muted by a GM. You may not talk freely until your mute has expired." );
						CHATSERVER->SendTitleBox( pcUser->pcUserData, "You are muted!" );
						return TRUE;
					}

					std::string str( pszBuff );
					str.erase( 0, 1 );

					PARTYHANDLER->SendChatRaid( pcUser, FormatString( "%s: %s", pcUser->pcUserData->sCharacterData.szName, str.c_str() ), CHATCOLOR_Raid );
				}
			}
			return TRUE;
		}

		if( COMMAND( "/request_party", pszBuff ) || COMMAND( "/party", pszBuff ) || COMMAND( "//party", pszBuff ) || COMMAND( "//PARTY", pszBuff ) )
		{
			if( GetParameterString( pszBuff, 1, szCommandParam1 ) )
			{
				UserData * pcUserDataParty = USERSERVER->GetUserdata( szCommandParam1 );

				if( pcUserDataParty )
					PARTYHANDLER->HandlePacket( pcUser, &PacketRequestParty( pcUserDataParty->iID ) );
			}
		}
		if( COMMAND( "/request_raid", pszBuff ) || COMMAND( "/raid", pszBuff ) || COMMAND( "//raid", pszBuff ) || COMMAND( "//RAID", pszBuff ) )
		{
			if( GetParameterString( pszBuff, 1, szCommandParam1 ) )
			{
				UserData * pcUserDataParty = USERSERVER->GetUserdata( szCommandParam1 );

				if( pcUserDataParty )
					PARTYHANDLER->HandlePacket( pcUser, &PacketRequestRaid( pcUserDataParty->iID) );
			}
		}
		else if ( COMMAND("/titles", pszBuff) )
		{
			CHARACTERSERVER->SendFullTitleListIfAny ( pcUser );
		}
		else if ( COMMAND ( "/title_clear", pszBuff ) )
		{
			if ( CHARACTERSERVER->CharacterInCityStage ( pcUser->pcUserData ) )
			{
				CHARACTERSERVER->ClearTitle (pcUser);
			}
			else
			{
				//todo - later on, use a town NPC to set title
				CHATSERVER->SendChat ( pcUser, EChatColor::CHATCOLOR_Error, "Titles can only be modified in a town!" );
			}
		}
		else if ( COMMAND ( "/title_set", pszBuff ) )
		{
			if ( CHARACTERSERVER->CharacterInCityStage ( pcUser->pcUserData ) )
			{
				if ( GetParameterString ( pszBuff, 1, szCommandParam1 ) )
				{
					int iTitleID = atoi ( szCommandParam1 );
					if ( iTitleID >= 1 && iTitleID <= 2000 )
					{
						if ( TIME > pcUser->uTitleSetTimeOut )
						{
							CHARACTERSERVER->SetTitle ( pcUser, iTitleID );

#ifdef DEV_MODE
							pcUser->uTitleSetTimeOut = TIME + 5; //+5s
#else
							pcUser->uTitleSetTimeOut = TIME + 60; //+60s
#endif
						}
						else
						{
							CHATSERVER->SendChat ( pcUser, EChatColor::CHATCOLOR_Error, "Please wait 1 minute between changing title" );
						}
					}
					else
					{
						CHATSERVER->SendChat ( pcUser, EChatColor::CHATCOLOR_Error, "Title set failed. Invalid id" );
					}
				}
				else
				{
					CHATSERVER->SendChatEx ( pcUser, EChatColor::CHATCOLOR_Error, "Expecting one parameter for /title_set" );
				}
			}
			else
			{
				//todo - later on, use a town NPC to set title
				CHATSERVER->SendChatEx ( pcUser, EChatColor::CHATCOLOR_Error, "Titles can only be modified in a town!" );
			}

			return TRUE;
		}

		else if ( COMMAND("/solo", pszBuff) )
		{
			if (pcUser->pcUserData->iMapID == MAPID_Bellatra && pcUser->pcUserData->dwExclusiveNum == 0x100)
			{
				BELLATRASERVER->SetRoomSoloMode(pcUser->pcUserData->sBellatraRoomID, TRUE);
			}
		}
		else if( COMMAND( "/leave_party", pszBuff ) )
		{
			if ( pcUser->bParty && pcUser->psParty )
			{
				PARTYHANDLER->LeaveParty( pcUser );
			}
		}
		else if( COMMAND( "/lot", pszBuff ) || COMMAND( "/lottery", pszBuff ) )
		{
			if( pcUser->bParty && pcUser->psParty )
			{
				int iRandom = Dice::RandomI( 0, 1000 );
				PARTYHANDLER->SendChatParty( pcUser, FormatString( "> %s played on lottery and take %d points.", pcUser->pcUserData->sCharacterData.szName, iRandom ), CHATCOLOR_Notice, true );
			}
		}
		else if( COMMAND( "/kick_party", pszBuff ) )
		{
			if( GetParameterString( pszBuff, 1, szCommandParam1 ) )
			{
				UserData * pcUserDataParty = USERSERVER->GetUserdata( szCommandParam1 );

				if( pcUserDataParty )
				{
					User * uParty = USERDATATOUSER( pcUserDataParty );

					if ( pcUser && uParty &&
						 pcUser->bParty && pcUser->psParty && uParty->bParty && uParty->psParty &&
						 pcUser->psParty == uParty->psParty &&
						 pcUser->psParty->pcLeader == pcUser)
					{
						PARTYHANDLER->LeaveParty( USERDATATOUSER( pcUserDataParty ), true );
					}
				}
			}
		}
	}

	//on game server
	if ( iLen = COMMAND( "/CLAN>", pszBuff ) )
	{
		//JLM - disable for now. seems to cause clan overlaps somehow..!
		/*if ( pcUser->uNextClanIdCheckTime == 0 ||
		pcUser->uNextClanIdCheckTime > TIME )
		{
		pcUser->uNextClanIdCheckTime = TIME + 120; //Check every 2 minutes

		//update clan id if needed
		pcUserData->sCharacterData.iClanID = CHARACTERSERVER->SQLGetCharacterLongClanID ( CHARACTERSERVER->GetCharacterName ( pcUserData ) );
		pcUserData->iClanID = CHARACTERSERVER->GetClanCode ( pcUserData->sCharacterData.iClanID );
		}*/


		if ( pcUser->pcUserData->iClanID != 0 )
		{
			BOOL bFound = FALSE;

			for ( int i = iLen; i < 100; i++ )
			{
				if ( pszBuff[i] == 0 )
					break;

				if ( pszBuff[i] != ' ' )
				{
					iLen = i;
					bFound = TRUE;
					break;
				}


			}
			if ( bFound )
			{
				if ( pcUser->bMuted )
				{
					INFO( "Player '%s' is muted", CHARACTERSERVER->GetCharacterName( pcUser->pcUserData ) );

					CHATSERVER->SendChat ( pcUser, CHATCOLOR_Error, "> You have been muted by a GM. You may not talk freely until your mute has expired." );
					CHATSERVER->SendTitleBox( pcUser->pcUserData, "You are muted!" );
					return TRUE;
				}

				EChatColor eColor = CHATCOLOR_Clan;

				if ( CHARACTERSERVER->IsCharacterLeader( pcUser ) )
					eColor = CHATCOLOR_ClanLeader;

				CHATSERVER->SendChatAllUsersInClan( pcUser->pcUserData, eColor, "[%c]%s: %s", CHARACTERSERVER->GetServerName( pcUser )[0], CHARACTERSERVER->GetCharacterName( pcUser ), pszBuff + iLen );
				CHATSERVER->SendChatEx( pcUser, eColor, "[%c]%s: %s", CHARACTERSERVER->GetServerName( pcUser )[0], CHARACTERSERVER->GetCharacterName( pcUser ), pszBuff + iLen );
				CHATSERVER->RecordClanChatToDatabase(pcUser->pcUserData, pszBuff + iLen);
			}

		}
		return TRUE;
	}

	if( LOGIN_SERVER )
	{
		if( COMMAND("/TRADE>", pszBuff) )
		{
			CHATSERVER->SendChatTrade( pcUser->pcUserData, pszBuff );
		}
		else if( pszBuff[0] == '/' && ( pszBuff[1] == ';' || pszBuff[1] == ':' ) )
		{
			std::string raw = trim(std::string(&pszBuff[2]));
			std::string msg = "";

			size_t index = raw.find(' ');
			if ( index == std::string::npos )
				index = raw.size();
			else
				msg = trim(raw.substr(index + 1, std::string::npos));

			const std::string name = raw.substr(0, index);

			// Bot
			if( BOTSERVER->IsOnlineBot(name.c_str()) )
			{
				if ( !msg.empty() )
					CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Blue, "To> %s: %s", name.c_str(), msg.c_str());
				else
					CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "%s is online", name.c_str() );

				return TRUE;
			}

			User* lcTargetUser = USERDATATOUSER(USERSERVER->GetUserdata(name.c_str()));
			if (lcTargetUser && lcTargetUser->IsValidAndInGame() )
			{
				if ( pcUser->bMuted )
				{
					INFO( "Player '%s' is muted", CHARACTERSERVER->GetCharacterName( pcUser->pcUserData ) );

					CHATSERVER->SendChat ( pcUser, CHATCOLOR_Error, "> You have been muted by a GM. You may not talk freely until your mute has expired." );
					CHATSERVER->SendTitleBox( pcUser->pcUserData, "You are muted!" );
					return TRUE;
				}

				CHATSERVER->SendWhisper(pcUser, lcTargetUser, msg);
				CHATSERVER->RecordPrivateChatToDatabase(pcUser->pcUserData, lcTargetUser->pcUserData, msg);
			}
			else
			{
				CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Error, "> %s is offline or not found!", name.c_str());
			}

			return TRUE;
		}
	}

	return FALSE;
}

BOOL ServerCommand::OnGameMasterCommand( User * pcUser, const char * pszBuff )
{
	if ( pcUser == NULL )
		return FALSE;

	int iLen = 0;

	// Game master activate
	if ( iLen = COMMAND( "/activategm", pszBuff ) )
	{
		OnActivateGameMaster( pcUser );
		return TRUE;
	}

	// Game master deactivate
	else if ( iLen = COMMAND( "/deactivategm", pszBuff ) )
	{
		// Send server information to user debug
		PacketSimple sPacket;
		sPacket.iLength	= sizeof( PacketSimple );
		sPacket.iHeader = PKTHDR_DisableGM;
		SENDPACKET( pcUser, &sPacket );

		CHATSERVER->SendChat( pcUser, EChatColor::CHATCOLOR_White, "> GM mode deactivated!" );

		pcUser->pcUserData->iGameLevel = EGameLevel::GAMELEVEL_None;
		NETSERVER->SyncGameMaster( pcUser->pcUserData );

		return TRUE;
	}

	// Game Master Commands
	if ( pcUser->GetGameLevel() > GAMELEVEL_None)
	{
		if (pcUser->GetGameLevel() >= GAMELEVEL_One &&
			OnGameMasterLevel1Command(pcUser, pszBuff))
		{
			CHATSERVER->RecordGameMasterCommandToDatabase ( pcUser, pszBuff );
			INFO ( "GM_1 command: %s", pszBuff );
			return TRUE;
		}

		if ( pcUser->GetGameLevel() >= GAMELEVEL_Two &&
			OnGameMasterLevel2Command(pcUser, pszBuff))
		{
			CHATSERVER->RecordGameMasterCommandToDatabase ( pcUser, pszBuff );
			INFO ( "GM_2 command: %s", pszBuff );
			return TRUE;
		}

		if (pcUser->GetGameLevel() >= GAMELEVEL_Three &&
			OnGameMasterLevel3Command ( pcUser, pszBuff ) )
		{
			CHATSERVER->RecordGameMasterCommandToDatabase ( pcUser, pszBuff );
			INFO ( "GM_3 command: %s", pszBuff );
			return TRUE;
		}

		// Level 4 (Admin)
		if ( pcUser->GetGameLevel() >= GAMELEVEL_Four &&
			OnGameMasterAdminCommand(pcUser, pszBuff))
		{
			CHATSERVER->RecordGameMasterCommandToDatabase ( pcUser, pszBuff );
			INFO ( "GM_4 command: %s", pszBuff );
			return TRUE;
		}
	}

	return FALSE;
}

void ServerCommand::HandleGrantTitleCommand(const std::string& charName, const std::string& title, const int titleRarity)
{
	SQLConnection* pcDB = SQLCONNECTION(DATABASEID_UserDB_PrimaryServer);

	int charID = 0;
	if (pcDB->Open())
	{
		const std::string query = "Select [ID] FROM CharacterInfo WHERE[AccountName]='" + charName + "'";

		if (pcDB->Prepare(query.c_str(), true))
		{
			if (pcDB->Execute())
			{
				if (pcDB->Fetch())
				{
					pcDB->GetData(1, PARAMTYPE_Integer, &charID);
				}
			}
		}
		pcDB->Close();
	}

	int titleID = 0;
	if (pcDB->Open())
	{
		const std::string query = "Select [TitleID] FROM TitleDef WHERE [Title]='" + title + "' AND [TitleRarity]=" + std::to_string(titleRarity);
		if (pcDB->Prepare(query.c_str(), true))
		{
			if (pcDB->Execute())
			{
				if (pcDB->Fetch())
				{
					pcDB->GetData(1, PARAMTYPE_Integer, &titleID);
				}
			}
		}

		pcDB->Close();
	}

	if (charID != 0 && titleID != 0 && pcDB->Open())
	{
		const std::string query = "INSERT INTO CharacterTitleList(CharacterID, TitleID, DateAcquired) VALUES(" + std::to_string(charID) + ", " + std::to_string(titleID) + ", CURRENT_TIMESTAMP)";
		if (pcDB->Prepare(query.c_str()))
		{
			pcDB->Execute();
		}

		pcDB->Close();
	}
}

template<typename T, typename... N>
auto my_make_array(N&&... args)->std::array<T, sizeof...(args)>
{
	return { std::forward<N>(args)... };
}
BOOL ServerCommand::OnReceiveChat ( User * pcUser, PacketChat * psPacket )
{
	if ( pcUser == NULL || pcUser->IsValidAndInGame() == FALSE )
		return TRUE; //to prevent it from being processed in assembly

	UserData * pcUserData = pcUser->pcUserData;

	if ( OnPlayerCommand ( pcUser, psPacket->szChat ) )
		return TRUE;

	if ( strlen ( psPacket->szChat ) >= 2 && psPacket->szChat[0] == '/' && psPacket->szChat[1] != ':' )
	{
		if ( OnGameMasterCommand ( pcUser, psPacket->szChat ) )
		{
			return TRUE;
		}

		std::string l_Command = psPacket->szChat;
		std::size_t l_EndPos = l_Command.find_first_of ( ' ' );

		if ( l_EndPos == std::string::npos )
			l_EndPos = l_Command.length ();

		l_Command = l_Command.substr ( 0, l_EndPos );

		/// HARDCPP : OLD COMMAND FILTERING
		/// TODO: Rewrite / Reverse these
		static const std::string s_AllowedOldCommands[] = {
			"/mon",
			"/field"
		};

		if (pcUser->pcUserData->iGameLevel <= EGameLevel::GAMELEVEL_One)
			return TRUE; //prevent processing in assembly

		for ( int l_I = 0; l_I < ( sizeof ( s_AllowedOldCommands ) / sizeof ( s_AllowedOldCommands[0] ) ); ++l_I )
		{
			//found matching command. return false so it gets processed otherwhere
			if ( strcmp ( s_AllowedOldCommands[l_I].c_str (), l_Command.c_str () ) == 0 )
			{
				CHATSERVER->RecordGameMasterCommandToDatabase ( pcUser, psPacket->szChat );
				return FALSE; //allow process in assembly
			}
		}
		return TRUE;
	}

	if ( pcUser->bMuted )
	{
		INFO( "Player '%s' is muted", CHARACTERSERVER->GetCharacterName( pcUser->pcUserData ) );

		CHATSERVER->SendChat ( pcUser, CHATCOLOR_Error, "> You have been muted by a GM. You may not talk freely until your mute has expired." );
		CHATSERVER->SendTitleBox( pcUser->pcUserData, "You are muted!" );
	}
	else
	{
		CHATSERVER->RecordPublicChatToDatabase( pcUserData, psPacket->szChat );
		CHATSERVER->SendChatAllUsersInRangeButMe( pcUserData, DISTANCE_MAX_CHATRANGE, CHATCOLOR_Normal, std::string( psPacket->szChat ) );
	}

	return TRUE;
}

void ServerCommand::CheckUserOnline( User * pcUser, struct PacketUserOnline * psPacket )
{
	UserData * pcUserOn = fnGetUsedataLoginName( psPacket->szName );
	if ( pcUserOn )
		psPacket->bOnline = TRUE;
	else
		psPacket->bOnline = FALSE;

	SENDPACKET( pcUser, psPacket, TRUE );

	CHATSERVER->SendDebugChat( pcUser, CHATCOLOR_Error, "> CheckUserOnline( %s ) [%s]", psPacket->szName, psPacket->bOnline ? "TRUE" : "FALSE" );
}

void ServerCommand::AddCommand( int iCommandID, char * pszParameter1, char * pszParameter2, char * pszParameter3 )
{
	SQLConnection * pcDB = SQLCONNECTION( DATABASEID_ServerDB, 22 );

	if ( pcDB->Open() )
	{
		if ( pcDB->Prepare( "INSERT INTO Command([CommandTypeID],[Parameter1],[Parameter2],[Parameter3],[DateProcessed]) VALUES(?,?,?,?,NULL)" ) )
		{
			pcDB->BindParameterInput( 1, PARAMTYPE_Integer, &iCommandID );
			pcDB->BindParameterInput( 2, PARAMTYPE_String, pszParameter1, STRLEN( pszParameter1 ) );
			pcDB->BindParameterInput( 3, PARAMTYPE_String, pszParameter2, STRLEN( pszParameter2 ) );
			pcDB->BindParameterInput( 4, PARAMTYPE_String, pszParameter3, STRLEN( pszParameter3 ) );

			pcDB->Execute();
		}

		pcDB->Close();
	}
}
