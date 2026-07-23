#include "stdafx.h"


#include "server.h"
#include "ini.h"
#include "CServer.h"
#include "HNSSkill.h"

BOOL g_bLoginServer = FALSE;
BOOL g_bGameServer = FALSE;
BOOL g_bServerIsSeasonal = FALSE;

Server * Server::pcInstance = NULL;

CLogger* g_pcLogger = NULL;


extern void UnInitSkills ();


void Server::CreateSQLConnection( EDatabaseID eID, std::string configPrefix )
{
	SQLConnection * pcSQL = new SQLConnection;
	pcSQL->Init( eID, configPrefix );
}

Server::Server()
{
	GetLocalTime( &sLocalTime );

	srand( static_cast<unsigned int>( time( 0 ) ) );

	iServers = 0;
	ZeroMemory( &sPacketServerList, sizeof( sPacketServerList ) );

	bMaintenanceCountdown = false;
	iMaintenanceCountdown = 0;

	InitializeCriticalSection( (CRITICAL_SECTION *)0x07AC3E7C );
	pcMutex = new CMutex( "Server", (CRITICAL_SECTION *)0x07AC3E7C );

	SocketServer::CreateInstance();
	pcSocketServer			= SocketServer::GetInstance();
	pcPacketServer			= new PacketServer();
	pcAccountServer			= new AccountServer();
	pcSQLServerConn			= new SQLConnection();
	pcCharacterServer		= new CharacterServer();
	pcChatServer			= new ChatServer();
	pcServerCommand			= new ServerCommand();
	pcUnitServer			= new UnitServer();
	pcUserServer			= new UserServer();
	pcSQLSkill				= new CSQLSkill();
	pcNetServer				= new NetServer();
	pcItemServer			= new ItemServer();
	pcLootServer			= new LootServer();
	pcLogServer				= new LogServer();
	pcCheatServer			= new CheatServer();
	pcMapServer				= new MapServer();
	pcQuestServer			= new QuestServer();
	pcUnitInfoServer		= new UnitInfoServer();
	pcBellatraServer		= new BellatraServer();
	pcBlessCastleServer		= new BlessCastleServer();
	pcPvPServer				= new PvPServer();
}

Server::~Server()
{
	SAFE_DELETE( pcPacketServer );
	SAFE_DELETE( pcAccountServer );
	SAFE_DELETE( pcSQLServerConn );
	SAFE_DELETE( pcCharacterServer );
	SAFE_DELETE( pcChatServer );
	SAFE_DELETE( pcServerCommand );
	SAFE_DELETE( pcUnitServer );
	SAFE_DELETE( pcUserServer );
	SAFE_DELETE( pcSQLSkill );
	SAFE_DELETE( pcNetServer );
	SAFE_DELETE( pcItemServer );
	SAFE_DELETE( pcLootServer );
	SAFE_DELETE( pcLogServer );
	SAFE_DELETE( pcCheatServer );
	SAFE_DELETE( pcMapServer );
	SAFE_DELETE( pcQuestServer );
	SAFE_DELETE( pcUnitInfoServer );
	SAFE_DELETE( pcBellatraServer );
	SAFE_DELETE( pcBlessCastleServer );
	SAFE_DELETE( pcPvPServer );
	SAFE_DELETE( pcSocketServer );

	SAFE_DELETE( pcMutex );

	UnInitSkills ();
}

void Server::Init( HWND hWnd )
{
	this->hWnd = hWnd;

	ShowWindow( hWnd, SW_HIDE );
	UpdateWindow( hWnd );


	std::string logFileName = "Log.txt";
	std::string oldLogName = "Log_old.txt";
	//move log file to Log_old.txt
	//and start fresh
	remove ( oldLogName.c_str () );
	rename ( logFileName.c_str (), oldLogName.c_str () );
	remove ( logFileName.c_str () );

	LOGGER = new CLogger ( logFileName );

	CreateSQLConnection( DATABASEID_GameDB );
	CreateSQLConnection( DATABASEID_GameDB_Event );
	CreateSQLConnection( DATABASEID_GameDB_Aging );
	CreateSQLConnection( DATABASEID_GameDB_Npc );
	CreateSQLConnection( DATABASEID_GameDB_Monsters );
	CreateSQLConnection( DATABASEID_GameDB_Mixing );

	CreateSQLConnection( DATABASEID_GameDB_QuestList );
	CreateSQLConnection( DATABASEID_GameDB_QuestList_Top );
	CreateSQLConnection( DATABASEID_GameDB_QuestEventList );
	CreateSQLConnection(DATABASEID_GameDB_PetSystem);
	CreateSQLConnection(DATABASEID_GameDB_PetSystem_Skin);

	CreateSQLConnection( DATABASEID_ServerDB );
	CreateSQLConnection( DATABASEID_LogDB );
	CreateSQLConnection( DATABASEID_LogDB_ServerLog );
	CreateSQLConnection( DATABASEID_LogDB_Cleaner );
	CreateSQLConnection( DATABASEID_LogDB_PacketLog );
	CreateSQLConnection( DATABASEID_LogDB_DatabaseLog );
	CreateSQLConnection( DATABASEID_LogDB_EventKill );
	CreateSQLConnection( DATABASEID_SkillDB );
	CreateSQLConnection( DATABASEID_EventDB );
	CreateSQLConnection( DATABASEID_ItemDB);

	CreateSQLConnection( DATABASEID_ClanDB );
	CreateSQLConnection( DATABASEID_ClanDB_2 );
	CreateSQLConnection( DATABASEID_ClanDB_3 );
	CreateSQLConnection( DATABASEID_ClanDB_4 );

	CreateSQLConnection( DATABASEID_ChatDB );
	CreateSQLConnection( DATABASEID_ChatDB_Cleaner );

	CreateSQLConnection ( DATABASEID_UserDB_LocalServer_Misc );
	CreateSQLConnection ( DATABASEID_UserDB_LocalServer_CharInfo );
	CreateSQLConnection ( DATABASEID_UserDB_LocalServer_UserInfo );
	CreateSQLConnection ( DATABASEID_UserDB_LocalServer_Bless );
	CreateSQLConnection ( DATABASEID_UserDB_LocalServer_Quest );
	CreateSQLConnection ( DATABASEID_UserDB_LocalServer_Quest_Sub );
	CreateSQLConnection ( DATABASEID_UserDB_LocalServer_Timers );
	CreateSQLConnection ( DATABASEID_UserDB_LocalServer_TitleAura );
	CreateSQLConnection ( DATABASEID_UserDB_LocalServer_KillTracker );
	CreateSQLConnection ( DATABASEID_UserDB_LocalServer_ItemBox );

	INI::CReader cReader ( "server.ini" );
	SERVER_IS_SEASONAL = cReader.ReadBool ( "Server", "Seasonal");


	//These connections will always point to the primary sql server if it's a seasonal server
	//where the connections above will point to the 'local' sql server
	CreateSQLConnection ( DATABASEID_UserDB_PrimaryServer,		"Primary" );
	CreateSQLConnection ( DATABASEID_UserDB_PrimaryServer_2,	"Primary" );

	//For syncing data from local to primary
	CreateSQLConnection ( DATABASEID_UserDB_PrimaryServerSyncer, "Primary" );


	CreateSQLConnection( DATABASEID_SkillDBNew, "SkillDBNew" );

}




//int rsRecordCheckFile(rsPLAYINFO *lpPlayInfo)
//int rsRecordMemoryBuffToFile( rsPLAYINFO *lpPlayInfo , char *szName ,  char *lpRecordMemBuff )
//int rsRecordMemoryBuff_CheckInvenItem( rsPLAYINFO *lpPlayInfo , int Mode )
//int rsRECORD_DBASE::MakeRecordData( smCHAR_INFO *lpCharInfo , sITEM *lpItems , sITEM *lpItems2 , sITEM *lpMouseItem )
//int rsRECORD_DBASE::MakeRecordItemList( rsPLAYINFO *lpPlayInfo ) <<<<<<<<<<<<<<<<< is where
/*void Server::Test()
{
	std::string path = "C:/Users/futur/Documents/GitHub/pristontaleeu-src-server/Login/Data/Character/Fishy.chr";



	//std::cout << entry.path() << std::endl;


	//300 is max number of inventory items

	//also need to update checksum of InvenItemInfo
	// see record.cpp line 3115


	PacketCharacterRecordData sCharacterRecord;
	BOOL bRead = FALSE;

	FILE* fp = NULL;
	int iSizeFile = 0;

	const char* filePath = path.c_str();

	fopen_s(&fp, filePath, "rb");

	// Character Data open?
	if (fp)
	{
		// Read .chr File

		fseek(fp, 0, SEEK_END);
		iSizeFile = ftell(fp);

		fseek(fp, 0, SEEK_SET);
		fread(&sCharacterRecord, iSizeFile, 1, fp);
		fclose(fp);

		bRead = TRUE;
	}

	if (bRead)
	{
		std::cout << sCharacterRecord.sCharacterData.iClanID;

		BYTE* pbaData = sCharacterRecord.baData;
		std::vector<RecordItem*> vRecordItem;

		for (int i = 0; i < sCharacterRecord.iItemCount; i++)
		{
			//Inventory Limit...
			if (i >= 200)
				break;

			RecordItem* ps = new RecordItem;
			ITEMSERVER->DecodeItemsData(pbaData, ps, 0x7FFFF);

			if (ps->sItem.sItemID.ToItemBase() == EItemBase::ITEMBASE_Weapon)
			{
				if (ps->sItem.sAgeLevel == 0)
				{
					ps->sItem.sDamage.sMax = 120;
					//ps->sItem.sDamage = MinMax(28, 49);

					ITEMSERVER->ValidateItem(&ps->sItem);

					//ps->sItem.iBackupKey = ps->sItem.iChk1;
					//ps->sItem.iBackupChk = ps->sItem.iChk2;

					BOOL valid = ITEMSERVER->CheckValidItem(&ps->sItem);

					std::cout << " - " << ps->sItem.szItemName << " : " << valid << std::endl;
				}

				//ITEMSERVER->SaveItemData(&ps->sItem);
			}

			vRecordItem.push_back(ps);

			//Next Item
			pbaData += ((int*)pbaData)[0];
		}


		pbaData = sCharacterRecord.baData;
		sCharacterRecord.iItemCount = 0;
		sCharacterRecord.iDataSize = 0;

		//Write Items
		for (RecordItem* ps : vRecordItem)
		{
			//Encode
			int iSize = ITEMSERVER->EncodeItemsData(ps, pbaData, sizeof(RecordItem));

			pbaData += iSize;
			sCharacterRecord.iDataSize += iSize;
			sCharacterRecord.iItemCount++;
		}

		vRecordItem.clear();
	}

	if (bRead)
	{
		FILE* fp = NULL;
		fopen_s(&fp, filePath, "wb");
		if (fp)
		{
			fseek(fp, 0, SEEK_SET);
			fwrite(&sCharacterRecord, iSizeFile, 1, fp);
			fclose(fp);
		}
	}
}*/
void Server::UnInit()
{
}

void Server::Load()
{
	auto t_start = std::chrono::steady_clock::now();

	iGameVersion = SERVERCONFIG->GetVersionGame();
	iServerVersion = SERVERCONFIG->GetVersionServer();

	iServers = 1;

	int iID = 0;

	CConfigFileReader * pcConfigFileReader = new CConfigFileReader( "server.ini" );
	if ( pcConfigFileReader->Open() )
	{
		iServers = pcConfigFileReader->ReadInt( "Server", "GameServers" ) + 1;

		iID = pcConfigFileReader->ReadInt( pcConfigFileReader->ReadString( "Server", "Type" ), "ID" );
		SERVER_CODE = iID;

		if ( iID >= 0 )
		{
			LoadServerInfo( pcConfigFileReader, saServerInfo + 0, SERVERTYPE_Login, "LoginServer" );

			for ( int i = 1; i < iServers; i++ )
			{
				char szGameServer[32];
				StringCbPrintfA( szGameServer, _countof( szGameServer ), "GameServer%d", i );

				LoadServerInfo( pcConfigFileReader, saServerInfo + i, SERVERTYPE_Game, szGameServer );
			}
		}
		else
		{
			iID = 0;
			SERVER_CODE = 0;

			LoadServerInfo( pcConfigFileReader, saServerInfo + 0, SERVERTYPE_Multi, "LoginServer" );

			for ( int i = 1; i < iServers; i++ )
			{
				char szGameServer[32];
				StringCbPrintfA( szGameServer, _countof( szGameServer ), "GameServer%d", i );

				LoadServerInfo( pcConfigFileReader, saServerInfo + i, SERVERTYPE_Game, szGameServer );
			}
		}

		for ( int i = 0; i < iServers; i++ )
		{
			ServerInfo * p = saServerInfo + i;

			if ( i != iID )
				NETSERVER->AddConnection( p->szNetIP, p->iPort, i );
		}

		pcConfigFileReader->Close();
	}
	SAFE_DELETE( pcConfigFileReader );


	psServerInfo = saServerInfo + iID;
	iServerType = psServerInfo->iType;

	switch ( iServerType )
	{
		case SERVERTYPE_Login:
			g_bLoginServer = TRUE;
			g_bGameServer = FALSE;
			break;
		case SERVERTYPE_Game:
			g_bLoginServer = FALSE;
			g_bGameServer = TRUE;
			break;
		case SERVERTYPE_Multi:
			g_bLoginServer = TRUE;
			g_bGameServer = TRUE;
			break;
	}


	if ( LOGIN_SERVER && GAME_SERVER )
		SetConsoleTitle( TEXT( "Game and Login Server" ) );
	else if ( LOGIN_SERVER )
		SetConsoleTitle( TEXT( "Login Server" ) );
	else
		SetConsoleTitle( TEXT( "Game Server" ) );


#ifdef DEV_MODE

	BOOL bShowErrorOnProd = FALSE;

	if ( STRINGCOMPAREI( psServerInfo->szIP, "54.36.127.45" ) )
	{
		bShowErrorOnProd = TRUE;
	}

#ifdef SEASONAL_IS_ACTIVE
	if ( STRINGCOMPAREI( psServerInfo->szIP, "51.195.60.124" ) )
	{
		bShowErrorOnProd = TRUE;
	}
#endif

	if ( bShowErrorOnProd )
	{
		LOGERROR( "Production server has DEV_MODE enabled!!" );
		ERRORBOX( "Production server has DEV_MODE enabled!!" );
	}
#endif

	LoadDirty();

	(*(BOOL*)0x0084585C) = GAME_SERVER;
	(*(BOOL*)0x07AC9D3C) = GAME_SERVER;

	SERVER_VERSION_MIN = iGameVersion;
	SERVER_VERSION_MAX = iGameVersion;
	SERVER_VERSION = iServerVersion;
	MAX_USERS = psServerInfo->iMaxUsers;


	CALL( 0x0056B960 ); //Load All
	CALL( 0x0056BA90 );

	SOCKETSERVER->Init( hWnd );

	if (GAME_SERVER)
	{
		NETSERVER->Init();
		DAMAGEHANDLER->LoadNewPVPScalingSettings();
	}

	BLESSCASTLESERVER->SQLReadBlessCastleStatus();


	ITEMSERVER->UpdateItemRarity ( );
	ITEMSERVER->GetItemSpecMod();
	ITEMSERVER->GetBaselineMod();

	BELLATRASERVER->Init();

#ifdef ITEMFIXES
	ITEMSERVER->UpdateItemFixes ( SQLCONNECTION ( DATABASEID_GameDB ) );
#endif


	CHARACTERSERVER->UpdatePlayerCharacterOverallRanking ();

	QUESTSERVER->LoadNPCQuests();

	MAPSERVER->SQLBuildMapIndicators();

	AccountServer::SQLUserOnline( "", "", "", 0, USERSONLINE_DeleteAll, 0, 0 );


	if ( GAME_SERVER )
	{
		UpdateAndCacheSkillInfoAndData();
		PETSYSTEMHANDLER->LoadSkinData();
	}

	if (LOGIN_SERVER)
	{
		//Temp - to fix exp values in CharacterInfo table
		//ServerCore::UpdateCharacterLevelAll ();

		//for testing purposes
		//ServerCore::WarehouseTest();

		INI::CReader cReader("server.ini");

		if ( cReader.ReadBool("Database", "UpdateItemsAndDumpItemsToItemsDB") )
		{
			SQLConnection* cDB;

			cDB = SQLCONNECTION(DATABASEID_ItemDB);

			INFO ( "Clearing ItemDump Table in ItemsDB" );
			if (cDB->Open())
			{
				if (cDB->Prepare("DELETE FROM [dbo].[ItemDump]"))
				{
					cDB->Execute();
				}

				cDB->Close();
			}

			INFO ( "Reading in all items from player's inventory and warehouse into ItemDump table" );
			INFO ( "And upgrading items" );
			INFO ( "This will take up to 5 minutes. Please wait.." );
			ServerCore::UpdateCharacterItemsLevelAll(TRUE);

			cReader.WriteBool("Database", "UpdateItemsAndDumpItemsToItemsDB", FALSE);
		}
		else if ( cReader.ReadBool("Database", "DumpItemsToItemsDB") )
		{
			DumpItemsIntoDatabase();
		}
	}

	if ( LOGIN_SERVER )
	{
		SqlUpdateOrInsertMetadata ( "server.version", GAME_VERSION );
		SqlUpdateOrInsertMetadata ( "server.seasonal.num", SERVER_IS_SEASONAL ? CURRENT_SEASONAL_NUM : 0 );

		STATUS ( "Login Server Started! (V%d)", GAME_VERSION );

		LOGSERVER->CleanUpOldLogsAsync ();
	}
	else
	{
		STATUS ( "Game Server Started! (V%d)", GAME_VERSION );
	}

	if ( SERVER_IS_SEASONAL )
	{
		STATUS ( "This server is running as a Seasonal server" );
	}

	if ( LOGGER->GetNumOfErrors () > 0 )
		LOGERROR ( "Number of errors: %d", LOGGER->GetNumOfErrors () );

	if ( LOGGER->GetNumOfWarnings () > 0 )
		WARN ( "Number of warnings: %d", LOGGER->GetNumOfWarnings () );

	LOGGER->SetAllowInsertIntoServerLogTable ( TRUE );

	//LOGERROR ( "TEst error %s", 1 ); //how to catch this? bad format

    auto t_end = std::chrono::steady_clock::now();
    auto t_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count();
    INFO("Server::Load() completed in %lld ms", t_ms);
}

void Server::DumpItemsIntoDatabase()
{
	SQLConnection* cDB;

	cDB = SQLCONNECTION(DATABASEID_ItemDB);

	//Clear all old data first.
	INFO ( "Clearing ItemDump Table in ItemDB" );
	if (cDB->Open())
	{
		if (cDB->Prepare("DELETE FROM [dbo].[ItemDump]"))
		{
			cDB->Execute();
		}

		cDB->Clear();

		if (cDB->Prepare("DELETE FROM [dbo].[GoldDump]"))
		{
			cDB->Execute();
		}

		cDB->Close();
	}

	INFO ( "Reading in all items from player's inventory and warehouse into ItemDump table" );
	INFO ( "This will take up to 5 minutes. Please wait.." );


	//Character
	{
		INFO ( "Reading from Character.." );

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
					ServerCore::UpdateCharacterItemsLevelData(szPath, (char*)s.szAccountName, (char*)s.szCharacterName, FALSE, TRUE ); //dump to sql only
				}
				else
				{
					WARN( "User and char not found: %s", fileNameWithoutExt.c_str() );
				}
			}
			while ( FindNextFileA( hFile, &sFindData ) );

			FindClose( hFile );
		}
	}
	//CharacterBackup

	//Warehouse
	{
		INFO ( "Reading from Warehouse.." );

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
				ServerCore::UpdateCharacterItemsLevelWarehouse( szPath, fileNameWithoutExt.c_str(), FALSE, TRUE); //dump to sql only
			}
			while ( FindNextFileA( hFile, &sFindData ) );

			FindClose( hFile );
		}
	}

}

void Server::RunBackgroundTask ()
{
	LOGSERVER->CleanUpOldLogsAsync ();
	Sleep ( 5000 );
}

void Server::UnLoad()
{
	AccountServer::SQLUserOnline( "", "", "", 0, USERSONLINE_DeleteAll, 0, 0 );
/*
	ServerNet::GetInstance()->DisconnectAll( true );

	SocketServer::GetInstance()->UnLoad();
	BlessCastleServer::GetInstance()->UnLoad();
	MapServer::GetInstance()->UnLoad();
	UserServer::GetInstance()->UnLoad();
	SQLManager::GetInstance()->UnLoad();
*/
	UnLoadDirty();
}

void Server::Begin()
{
	ShowWindow( hWnd, SW_HIDE );
	UpdateWindow( hWnd );

	InvalidateRect( hWnd, NULL, TRUE );
	UpdateWindow( hWnd );

	if ( LOGIN_SERVER )
	{
//		CreateThread( NULL, KB256, (LPTHREAD_START_ROUTINE)BellatraServer::ProcessBellatraScores, NULL, STACK_SIZE_PARAM_IS_A_RESERVATION, NULL );
//		CreateThread( NULL, KB256, (LPTHREAD_START_ROUTINE)ChatServer::ProcessGlobalTradeChats, NULL, STACK_SIZE_PARAM_IS_A_RESERVATION, NULL );
		CreateThread( NULL, KB256, (LPTHREAD_START_ROUTINE)AccountServer::ProcessAccountLoginQueue, NULL, STACK_SIZE_PARAM_IS_A_RESERVATION, NULL );
	}
}

void Server::End()
{
	InvalidateRect( hWnd, NULL, TRUE );
	UpdateWindow( hWnd );

	ShowWindow( hWnd, SW_HIDE );
	UpdateWindow( hWnd );
}

void Server::Tick( ETickType iTickType )
{
	switch ( iTickType )
	{
		case TICKTYPE_500MilliSecond:
		{
			CALL ( 0x00560520 );
			_time32 ( (__time32_t *)0x07AC9D6C );
			break;
		}
		case TICKTYPE_1Second:
		{
			AnnounceMaintenanceCountdown ();

			GetLocalTime ( &sLocalTime );

			for ( int i = 0; i < PLAYERS_MAX; i++ )
			{
				if ( USERSDATA[i].pcSocketData )
				{
					CHARACTERSERVER->OnPremiumTimersAndForceOrbTick1s ( &USERSDATA[i] );
				}
			}

			PVPSERVER->Update ();
			FURYARENAHANDLER->Tick1s ();
			TRADEHANDLER->Tick1s ();
			TESTMAPHANDLER->Tick1s ();
			LOGSERVER->OnLogUsersOnline ();

			if ( GAME_SERVER )
			{
				ROLLDICEHANDLER->Tick ();
			}
		}
		break;
		case TICKTYPE_10Second:
		{
			Tick10s ();
		}
		break;
		case TICKTYPE_1Minute:
		{
			CHARACTERSERVER->Tick1Min ();

			if ( LOGIN_SERVER )
				ServerCommand::SQLRead ();
		}
		break;
	}
}

//60FPS
void Server::Loop()
{
	//Update Global TickCount
	//IMPORTANT NOTE: GetTickCount returns milliseconds since
	//system start, and it rolls over every  49 days back to
	//0 seconds. So this can cause all kind of issues when doing computations
	// TICKCOUNT > NextTimeSync can result in 0 > 123354455 on roll over
	// Time difference = TICKCOUNT - LastTickCount = can cause negative on roll over
	TickCount ( GetTickCount () );

	//Update Users
	USERSERVER->Update();

	//Update Units
	UNITSERVER->Update();

	//Update Maps
	MAPSERVER->Update();

	//Update Alive Users
	USERSERVER->Loop();
}

void Server::Time( double fTime, CServer * pServer)
{
	static double fCount_500MS = 0.0f;  static double f500MS = 500.0f;
	static double fCount_1SECO = 0.0f;  static double f1SECO = 1000.0f;
	static double fCount_10SECO = 0.0f; static double f10SECO = 10000.0f;
	static double fCount_1MINU = 0.0f;  static double f1MINU = 60000.0f;
	static double fCount_1HOUR = 0.0f;  static double f1HOUR = 3600000.0f;

	fCount_500MS += fTime;
	fCount_1SECO += fTime;
	fCount_10SECO += fTime;
	fCount_1MINU += fTime;
	fCount_1HOUR += fTime;

	while ( fCount_500MS >= f500MS )
	{
		Server::GetInstance()->Tick( TICKTYPE_500MilliSecond );

		fCount_500MS -= f500MS;
	}

	while ( fCount_1SECO >= f1SECO )
	{
		Server::GetInstance()->Tick( TICKTYPE_1Second );

		pServer->Tick1s();

		fCount_1SECO -= f1SECO;
	}

	while ( fCount_10SECO >= f10SECO )
	{
		Server::GetInstance ()->Tick ( TICKTYPE_10Second );

		fCount_10SECO -= f10SECO;

		//Flush the log every 10 sec
		LOGGER->Flush();
	}

	while ( fCount_1MINU >= f1MINU )
	{
		Server::GetInstance()->Tick( TICKTYPE_1Minute );

		fCount_1MINU -= f1MINU;
	}

	while ( fCount_1HOUR >= f1HOUR )
	{
		Server::GetInstance ()->Tick ( TICKTYPE_1Hour );

		fCount_1HOUR -= f1HOUR;
	}
}

void Server::LoadServerInfo( CConfigFileReader * pcConfigFileReader, ServerInfo * si, EServerType eServerType, string strSection )
{
	//Server Type
	si->iType = eServerType;

	//Name
	StringCchCopyA( si->szName, _countof( si->szName ), pcConfigFileReader->ReadString( strSection, "Name" ).c_str() );

	//IP 1
	si->lIP.s_addr = inet_addr( pcConfigFileReader->ReadString( strSection, "IP" ).c_str() );
	StringCchCopyA( si->szIP, _countof( si->szIP ), inet_ntoa( si->lIP ) );

	//IP 2
	si->lIP2.s_addr = inet_addr( pcConfigFileReader->ReadString( strSection, "IP" ).c_str() );
	StringCchCopyA( si->szIP2, _countof( si->szIP2 ), inet_ntoa( si->lIP2 ) );

	//IP 3
	si->lIP3.s_addr = inet_addr( pcConfigFileReader->ReadString( strSection, "IP" ).c_str() );
	StringCchCopyA( si->szIP3, _countof( si->szIP3 ), inet_ntoa( si->lIP3 ) );

	//IP Net
	si->lNetIP.s_addr = inet_addr( pcConfigFileReader->ReadString( strSection, "NetIP" ).c_str() );
	StringCchCopyA( si->szNetIP, _countof( si->szNetIP ), inet_ntoa( si->lNetIP ) );

	//Port
	si->iPort = pcConfigFileReader->ReadInt( strSection, "Port" );

	si->iMaxUsers = pcConfigFileReader->ReadInt( strSection, "MaxUsers" );
}

char * Server::GetServerName( int iIndex )
{
	if ( (iIndex >= 0) && (iIndex < MAX_SERVERINFO) )
		return saServerInfo[iIndex].szName;

	return "";
}

int Server::GetServerIndex( char * pszIP, int iPort )
{
	for ( int i = 0; i < MAX_SERVERINFO; i++ )
	{
		ServerInfo * si = saServerInfo + i;

		if ( si->iPort == iPort )
		{
			if ( STRINGCOMPAREI( si->szIP, pszIP ) )
				return i;

			if ( STRINGCOMPAREI( si->szIP2, pszIP ) )
				return i;

			if ( STRINGCOMPAREI( si->szIP3, pszIP ) )
				return i;
		}
	}

	return -1;
}

/// <summary>
/// Called every 10s
/// </summary>
void Server::Tick10s ()
{
	DWORD unixTime = GetUnixTime ();

	if ( LOGIN_SERVER )
	{
		SqlUpdateOrInsertMetadata ( "server.login.unix.time", unixTime );
		SqlUpdateOrInsertMetadata ( "users.online", USERS_ONLINE );

		//we will consider the server NOT under maintenance is there is more than 10 users online.
		if ( USERS_ONLINE > 10 && SERVER->GetMaintenance () == FALSE )
		{
			SqlUpdateOrInsertMetadata ( "under.maintenance", FALSE );
		}
	}
	else
	{
		SqlUpdateOrInsertMetadata ( "server.game.unix.time", unixTime );
		SERVERCOMMAND->SQLReadAndExecuteGMCommand ();
	}
}


void Server::SqlUpdateOrInsertMetadata ( const char * key, int value )
{
	SQLConnection * pcDB = SQLCONNECTION ( DATABASEID_ServerDB, 16 );
	if ( pcDB->Open () )
	{
		std::string query = " IF EXISTS (SELECT 0 FROM Metadata WHERE [Key]=?)"
			" BEGIN"
			" UPDATE Metadata SET [Value]=? WHERE [Key]=?"
			" END ELSE BEGIN"
			" INSERT INTO Metadata ([Key],[Value]) VALUES (?, ?)"
			" END";

		if ( pcDB->Prepare ( query.c_str () ) )
		{
			pcDB->BindParameterInput ( 1, PARAMTYPE_String, (char *)key, 20 );
			pcDB->BindParameterInput ( 2, PARAMTYPE_Integer, &value );
			pcDB->BindParameterInput ( 3, PARAMTYPE_String, (char *)key, 20 );
			pcDB->BindParameterInput ( 4, PARAMTYPE_String, (char *)key, 20 );
			pcDB->BindParameterInput ( 5, PARAMTYPE_Integer, &value );

			if ( pcDB->Execute () )
			{
				//success
			}
		}
		pcDB->Close ();
	}
}

void Server::StartMaintenanceCountdown( int iSeconds )
{
	bMaintenanceCountdown = true;
	iMaintenanceCountdown = iSeconds;
	dwLastAnnounce = 0;

	AnnounceMaintenanceCountdown();

	if ( LOGIN_SERVER )
	{
		SqlUpdateOrInsertMetadata ( "under.maintenance", TRUE );
	}

	SERVER->SetMaintenance( TRUE );
}

void Server::StopMaintenanceCountdown()
{
	bMaintenanceCountdown = false;

	if ( LOGIN_SERVER )
	{
		SqlUpdateOrInsertMetadata ( "under.maintenance", FALSE );
	}

	SERVER->SetMaintenance( FALSE );
}

void Server::AnnounceMaintenanceCountdown()
{
	if ( bMaintenanceCountdown )
	{
		iMaintenanceCountdown--;

		if ( iMaintenanceCountdown > 0 )
		{
			int iMinutes = iMaintenanceCountdown / 60;
			int iSeconds = iMaintenanceCountdown - (iMinutes * 60);

			if ( iMaintenanceCountdown == 10 )
			{
				Packet s;
				s.iLength = sizeof( s );
				s.iHeader = PKTHDR_Save;
				for ( int i = 0; i < PLAYERS_MAX; i++ )
				{
					UserData * pcCur = USERSDATA + i;

					if ( (pcCur->pcSocketData != NULL) && pcCur->iID )
						WNDPROC_SOCKETCLOSE( SOCKETSERVER->GetHWND(), pcCur->pcSocketData );
				}
			}

			bool bAnnounce = false;
			if ( iMaintenanceCountdown <= 5 )
			{
				bAnnounce = true;
			}
			else if ( iMaintenanceCountdown <= 15 )
			{
				if ( (iMaintenanceCountdown % 2) == 0 )
				{
					bAnnounce = true;
				}
			}
			else if ( iMaintenanceCountdown <= 60 )
			{
				if ( (iMaintenanceCountdown % 10) == 0 )
				{
					bAnnounce = true;
				}
			}
			else if ( iMaintenanceCountdown <= 120 )
			{
				if ( (iMaintenanceCountdown % 20) == 0 )
				{
					bAnnounce = true;
				}
			}
			else
			{
				if ( (iMaintenanceCountdown % 30) == 0 )
				{
					bAnnounce = true;
				}
			}

			if ( LOGIN_SERVER )
			{
				if ( bAnnounce )
					CHATSERVER->SendChatAllEx( CHATCOLOR_Global, "Maintenance Countdown> %dmin %dsec", iMinutes, iSeconds );
			}
		}
		else
		{
			for ( int i = 0; i < PLAYERS_MAX; i++ )
			{
				UserData * pcCur = USERSDATA + i;

				if ( (pcCur->pcSocketData != NULL) && (pcCur->iGameLevel < GAMELEVEL_Four) )
					WNDPROC_SOCKETCLOSE( SOCKETSERVER->GetHWND(), pcCur->pcSocketData );
			}

			bMaintenanceCountdown = false;
			int ret_val = 0x0000BEEF;
			exit(ret_val);
		}
	}
}

BOOL Server::SQLGetMaintenace()
{
	BOOL bRet = FALSE;

	SQLConnection * pcDB = SQLCONNECTION( DATABASEID_ServerDB, 17 );
	if ( pcDB->Open() )
	{
		if ( pcDB->Prepare( "SELECT IP FROM Maintenance WHERE (Mode=3)" ) )
		{
			if ( pcDB->Execute() && pcDB->Fetch() )
			{
				pcDB->GetData( 1, PARAMTYPE_Integer, &bRet );
			}
		}
		pcDB->Close();
	}

	return bRet;
}

ID Server::GetNextID()
{
	static ID lBeginID = 0x00100000;

	ID lNextID = (lBeginID++);

	if ( GAME_SERVER )
		lNextID += 0x3FF00000;

	return lNextID;
}

void Server::PHServerList( SocketData * pcSocketData, int iTicket )
{
	if ( sPacketServerList.iHeader != PKTHDR_ServerList )
	{
		sPacketServerList.iLength = sizeof( Packet ) + sizeof( PacketServerList::Header );
		sPacketServerList.iHeader = PKTHDR_ServerList;

		StringCchCopyA( sPacketServerList.sHeader.szServerName, _countof( sPacketServerList.sHeader.szServerName ), psServerInfo->szName );

		int j = 0;
		for ( int i = 0; (i < iServers) && (j < (_countof( sPacketServerList.sServers ) - 1)); i++ )
		{
			ServerInfo * p = saServerInfo + i;

			if ( p->iType == SERVERTYPE_Game )
			{
				StringCchCopyA( sPacketServerList.sServers[j].szName, _countof( sPacketServerList.sServers[j].szName ), p->szName );

				StringCchCopyA( sPacketServerList.sServers[j].szaIP[0], _countof( sPacketServerList.sServers[j].szaIP[0] ), p->szIP );
				StringCchCopyA( sPacketServerList.sServers[j].szaIP[1], _countof( sPacketServerList.sServers[j].szaIP[1] ), p->szIP2 );
				StringCchCopyA( sPacketServerList.sServers[j].szaIP[2], _countof( sPacketServerList.sServers[j].szaIP[2] ), p->szIP3 );

				sPacketServerList.sServers[j].iaPort[0] = p->iPort;
				sPacketServerList.sServers[j].iaPort[1] = p->iPort;
				sPacketServerList.sServers[j].iaPort[2] = p->iPort;
				sPacketServerList.sServers[j].iaPort[3] = 0;

				j++;
			}
		}

		sPacketServerList.sHeader.iGameServers = j;
		sPacketServerList.sHeader.iClanServerIndex = j;

		StringCchCopyA( sPacketServerList.sServers[j].szName, _countof( sPacketServerList.sServers[j].szName ), "clan" );

		StringCchCopyA( sPacketServerList.sServers[j].szaIP[0], _countof( sPacketServerList.sServers[j].szaIP[0] ), saServerInfo[0].szIP );
		StringCchCopyA( sPacketServerList.sServers[j].szaIP[1], _countof( sPacketServerList.sServers[j].szaIP[1] ), saServerInfo[0].szIP2 );
		StringCchCopyA( sPacketServerList.sServers[j].szaIP[2], _countof( sPacketServerList.sServers[j].szaIP[2] ), saServerInfo[0].szIP3 );

		sPacketServerList.sServers[j].iaPort[0] = 80;
		sPacketServerList.sServers[j].iaPort[1] = saServerInfo[0].iPort;
		sPacketServerList.sServers[j].iaPort[2] = saServerInfo[0].iPort;
		sPacketServerList.sServers[j].iaPort[3] = 0;

		sPacketServerList.iLength += ((sizeof( PacketServerList::Server ) * sPacketServerList.sHeader.iGameServers) + (sizeof( PacketServerList::Server ) * (sPacketServerList.sHeader.iClanServerIndex != 0 ? 1 : 0)));
	}

	sPacketServerList.sHeader.iTicket = iTicket;

	sPacketServerList.sHeader.dwTime = READDWORD( 0x07AC9D6C );

	pcSocketData->SendPacket( &sPacketServerList );
}
