#include "stdafx.h"
#include "userserver.h"
#include "HNSSkill.h"

extern void __cdecl AddFinalDamage( UserData * pcUserData, void * pPacket );

User * pcaUser;

UserServer::UserServer()
{
	pcaUserInGame = new User*[PLAYERS_MAX + 256];

	pcaUser = new User[PLAYERS_MAX + 256];

	pcaUserData = new UserData[PLAYERS_MAX + 256];

	USERSDATA = pcaUserData;

	TEST( "UserData", sizeof( UserData ), 0xB510 );

	for ( int i = 0; i < PLAYERS_MAX; i++ )
	{
		User * u = pcaUser + i;
		ZeroMemory( u, sizeof( User ) );
		u->uIndex = i;
		u->pcUserData = &USERSDATA[i];
	}
}

UserServer::~UserServer()
{
}

void UserServer::CreateData()
{

}

User * UserServer::GetFreeUser()
{
	for ( int i = 0; i < PLAYERS_MAX; i++ )
	{
		User * u = pcaUser + i;

		if ( !u->bInUse )
		{
			u->Init();
			return u;
		}
	}

	//Error
	return NULL;
}

User * UserServer::UserdatatoUser( UserData * pcUserData )
{
	if ( pcUserData == NULL )
		return NULL;

	return (pcaUser + (pcUserData - pcaUserData));
}

User * UserServer::GetUserByIndex( UINT uIndex )
{
	return pcaUser + uIndex;
}

/// <summary>
/// Leaked: srFindUserFromSerial
/// ASM: 0x0054CC10
/// </summary>
/// <param name="iPlayerId">The unique object ID of the player</param>
UserData * UserServer::GetUserdata( int iPlayerId )
{
	typedef UserData *( __cdecl *t_GetPlayerPointer ) ( UINT iID );
	t_GetPlayerPointer GetPlayerPointer = ( t_GetPlayerPointer )0x0054CC10;
	return GetPlayerPointer( iPlayerId );
}

UserData * UserServer::GetUserdata( const char * pszCharacterName )
{
	for ( int i = 0; i < PLAYERS_MAX; i++ )
	{
		if ( USERSDATA[i].pcSocketData )
		{
			if ( GAME_SERVER )
			{
				if ( STRINGCOMPAREI( USERSDATA[i].sCharacterData.szName, pszCharacterName ) )
					return &USERSDATA[i];
			}
			else
			{
				if ( STRINGCOMPAREI( USERSDATA[i].szCharacterName, pszCharacterName ) )
					return &USERSDATA[i];
			}
		}
	}
	return NULL;
}

UserData * UserServer::GetUserdataByAccount( const char * pszAccountName )
{
	for ( int i = 0; i < PLAYERS_MAX; i++ )
	{
		User * pcUser = USERDATATOUSER( &USERSDATA[i] );

		if ( pcUser && pcUser->IsConnected() && pcUser->pcUserData && pcUser->pcUserData->szAccountName[0] )
		{
			if ( STRINGCOMPAREI( pcUser->pcUserData->szAccountName, pszAccountName ) )
				return pcUser->pcUserData;
		}
	}

	return nullptr;
}

UserData * UserServer::GetUserdataSocket( SocketData * pcSocketData )
{
	for ( int i = 0; i < PLAYERS_MAX; i++ )
	{
		if ( USERSDATA[i].pcSocketData && USERSDATA[i].pcSocketData == pcSocketData )
		{
			return &USERSDATA[i];
		}
	}
	return NULL;
}

int UserServer::SQLGetAccountID( const char * pszAccountName )
{
	int iRet = -1;

	SQLConnection * pcDB = SQLCONNECTION( DATABASEID_UserDB_PrimaryServer );
	if ( pcDB->Open() )
	{
		if ( pcDB->Prepare( "SELECT ID FROM UserInfo WHERE AccountName=?" ) )
		{
			pcDB->BindParameterInput( 1, PARAMTYPE_String, (char*)pszAccountName, STRLEN( pszAccountName ) );
			if ( pcDB->Execute() && pcDB->Fetch() )
			{
				pcDB->GetData( 1, PARAMTYPE_Integer, &iRet );
			}
		}
		pcDB->Close();
	}

	return iRet;
}

void UserServer::CancelForceOrb( User * pcUser )
{
	//DEBUG( "CancelForceOrb" );

	ItemPremium * psPremium = ITEMSERVER->GetPremiumUse( pcUser, ITEMTIMERTYPE_ForceOrb );
	if ( psPremium == NULL )
	{
		ITEMSERVER->RemoveForceOrb( pcUser );

		WARN( "Force orb was cancelled but user doesn't have force orb?" ); //maybe logic / bug on server side
		return;
	}

	SENDPACKETCOMMAND( pcUser, PKTHDR_ForceOrbCancel, psPremium->dwCode );
	ITEMSERVER->OnPremiumItemCancelledOrExpired( pcUser, psPremium, FALSE );
}

int UserServer::GetTicket()
{
	srand( (DWORD)time( NULL ) );
	int iRand = Dice::RandomI(1, 10000);
	return iRand;
}


BOOL UserServer::SetAccountNameFromSQL( UserData * pcUserData )
{
	if ( pcUserData && !pcUserData->szAccountName[ 0 ] )
	{
		char szUserID[ 32 ] = { 0 };
		ZeroMemory( szUserID, 32 );

		char * pszCharName = CHARACTERSERVER->GetCharacterName(pcUserData);

		SQLConnection * pcDB = SQLCONNECTION( DATABASEID_UserDB_LocalServer_CharInfo );
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

		if ( IsNull(szUserID) )
			return FALSE;

		STRINGCOPY( pcUserData->szAccountName, szUserID );
		return TRUE;
	}
	return FALSE;
}

void UserServer::ReconnectUser( UserData * pcUserData )
{
#ifdef RECONNECT_SYSTEM
	CHATSERVER->SendChatEx( USERDATATOUSER( pcUserData ), CHATCOLOR_Error, "> Reconnected to %s Server!", GAME_SERVER ? "Game" : "Login" );
	PacketSimple sPacket;
	sPacket.iLength = sizeof( PacketSimple );
	sPacket.iHeader = PKTHDR_ReconnectedServer;
	sPacket.iUnk = 0;
#endif
}

/// <summary>
/// This function gets called at 64FPS
/// </summary>
void UserServer::UpdateUnitStatus( User * pcUser )
{
	UserData * pcUserData = pcUser->pcUserData;

	//User Unit Status
	pcUser->sUnitStatusContainer.iHeader = PKTHDR_UnitStatusContainer;
	pcUser->sUnitStatusContainer.iCounter = pcUserData->iUnitStatusCounter & 0x00FFFFFF;
	//Monster Unit Status
	pcUser->sUnitStatusContainerM.iHeader = PKTHDR_UnitStatusContainer;
	pcUser->sUnitStatusContainerM.iCounter = (pcUserData->iUnitStatusCounter & 0x00FFFFFF) - 0x80000000;


	//Increment Status Counter
	pcUserData->iUnitStatusCounter++;

	//If user is no longer moving,
	//increment the not moving counter, and
	//increase the priority counter
	if ( pcUserData->bNotMovingCounter != 0 )
	{
		pcUser->uPriorityCounter++;
		pcUserData->bNotMovingCounter++;

		const int duration1Second = 64;
		const int duration4Second = 64 * 4;
		const int duration8Second = 64 * 8;
		const int duration16Second = 64 * 8;

		if ( pcUser->pcUserData->bAFK == FALSE )
		{
			//not moving for 16 seconds
			if ( pcUser->uPriorityCounter > duration16Second )
			{
				pcUser->uMovementStatus = EPlayerMovementStatus::AFK;
				pcUser->pcUserData->bAFK = TRUE;
			}

			//not moving for 8 seconds
			else if ( pcUser->uPriorityCounter > duration8Second )
			{
				pcUser->uMovementStatus = EPlayerMovementStatus::Standing_8_Sec;
			}

			//not moving for four seconds
			else if ( pcUser->uPriorityCounter > duration4Second )
			{
				pcUser->uMovementStatus = EPlayerMovementStatus::Standing_4_Sec;
			}

			//not moving for one second
			else if ( pcUser->uPriorityCounter > duration1Second )
			{
				pcUser->uMovementStatus = EPlayerMovementStatus::Standing_1_Sec;
			}
			else
			{
				pcUser->uMovementStatus = EPlayerMovementStatus::Moving;
			}
		}
	}
	else
	{
		//user is active.
		pcUser->uPriorityCounter = 0;
		pcUser->uMovementStatus = EPlayerMovementStatus::Moving;

		pcUser->pcUserData->bAFK = FALSE;
	}
}

/// <summary>
/// Used to send misc data about this player to other players
/// Such as playing a certain animation or skill
/// See UnitGame::HandlePacketUnitStatus( UnitData * pcUnitData, Packet * psPacket ) on client side
/// </summary>
/// <param name="pcUser">This player</param>
/// <param name="eCommand">The command to send</param>
/// <param name="arg1">Optional command #1</param>
/// <param name="arg2">Optional command #2</param>
void UserServer::SendUserMiscCommandToOtherNearbyPlayers ( User * pcUser, EUnitDataMiscCommand eCommand, int iArg1, int iArg2 )
{
	if ( pcUser && pcUser->pcUserData )
	{
		EMapID eThisUserMapId = pcUser->GetMapID ();
		Point3D thisUserPosition = pcUser->pcUserData->sPosition;

		PacketCommand sExtraPacket;
		sExtraPacket.iLength = sizeof ( PacketCommand );
		sExtraPacket.iHeader = PKTHDR_MiscUnitCommand;
		sExtraPacket.p1 = (int)eCommand;
		sExtraPacket.p2 = (int)pcUser->pcUserData->iID;
		sExtraPacket.p3 = iArg1;
		sExtraPacket.p4 = iArg2;

		for ( UINT i = 0; i < USERSERVER->uUsersInGame; i++ )
		{
			User * pcOtherUser = USERSERVER->pcaUserInGame[i];
			if ( pcOtherUser && pcOtherUser->pcUserData && pcOtherUser->IsConnected () &&	//user has socket?
				 pcOtherUser->GetMapID () == eThisUserMapId &&								//same map?
				 pcOtherUser != pcUser )													//is not me?
			{
				int iDistXZ = thisUserPosition.GetPTDistanceXZ ( &pcOtherUser->pcUserData->sPosition );

				//only send to nearby users (within 20 meters from user)
				if ( iDistXZ < DISTANCE_XY_20_meters )
				{
					//std::cout << "sent unit " << pcOtherUser->pcUserData->iID << " stats to user" << std::endl;
					SENDPACKET( pcOtherUser, &sExtraPacket );
				}
			}
		}
	}
}

void UserServer::OnHandlerFunctions( DWORD dwCode, DWORD * pdwOptional )
{
	switch ( dwCode )
	{
		// Debug Unit
		case 1:
		{
			//No longer used

			WARN ( "OnHandlerFunctions 1 used somehow???? ");

			break;
		}

		// User Load
		case 2:
			USERSERVER->OnLoadUser( USERSERVER->UserdatatoUser( (UserData*)pdwOptional[0] ) );
			break;

		// User UnLoad
		case 3:
			USERSERVER->OnUnLoadUser( USERSERVER->UserdatatoUser( (UserData*)pdwOptional[0] ) );
			break;

		// Final Damage
		case 4:
			AddFinalDamage( (UserData*)pdwOptional[0], (void*)pdwOptional[1] );
			break;


		case 5:
			NETSERVER->SendSyncChar( (UserData*)pdwOptional[0] );
			CHARACTERSERVER->OnCharacterSyncData( (UserData*)pdwOptional[0] );
			break;

		case 6:
			SOCKETSERVER->SocketClose( (SocketData*)pdwOptional[0] );
			break;

		case 7:

			break;

		case 8:
			BLESSCASTLEHANDLER->UpdateSiegeWar();
			break;

		case 9:
			BLESSCASTLESERVER->NetSendBlessCastleOwnerClanID( pdwOptional[0] );
			break;

		case 0x0A:
			BLESSCASTLESERVER->SQLReadBlessCastleStatus();
			break;

		case 0x0B:
			BLESSCASTLESERVER->SQLWriteBlessCastleStatus();
			break;

		case 0x0C:
			PRIESTESSSKILL->OnHolyMindHandler( USERDATATOUSER((UserData*)pdwOptional[0]), (int*)pdwOptional[1] );
			break;

		case 0x0D:
			ITEMSERVER->OnUseCrystal( (UserData*)pdwOptional[0], (UnitData*)pdwOptional[1], (DWORD)pdwOptional[2] );
			break;

		case 0x0E:
			ITEMSERVER->OnUseCrystalDie( (UserData*)pdwOptional[0], (UnitData*)pdwOptional[1] );
			break;

		case 0x0F:
			//Not called here?
			//HALLOWEENHANDLER->OnSpawnEventMonster( (CharacterData*)pdwOptional[0] );
			//CHRISTMASHANDLER->OnSpawnMonsterEvent( (CharacterData*)pdwOptional[0], (Map*)pdwOptional[1] );
			//GENERALEVENTHANDLER->OnSpawnMonsterEvent( (CharacterData*)pdwOptional[0], (Map*)pdwOptional[1] );
			//EVENTSERVER->OnSpawnEventMonster( (CharacterData*)pdwOptional[0], (Map*)pdwOptional[1] );
			break;

		case 0x11:
			break;

		case 0x12:
			break;

		case 0x13:
		{
			User * u;
			u = USERDATATOUSER( (UserData *)pdwOptional[1] );
			MAPSERVER->SendStageItem( (Map *)pdwOptional[0], u );
			break;
		}
		case 0x14:
			if ( (UserData*)pdwOptional[1] )
				MAPSERVER->SendStageSpawn( (Map*)pdwOptional[0], (UserData*)pdwOptional[1] );
			break;

		case 0x15:
			if ( (UnitData*)pdwOptional[0] )
				CHRISTMASHANDLER->OnSayTime( (UnitData*)pdwOptional[0] );
			break;

		case 0x16:
			if ( (UnitData*)pdwOptional[0] )
				UNITSERVER->IsMultiplyHPMonter( (UnitData*)pdwOptional[0] ); //called at creation of unit (0x559F30)
			break;

		case 0x17:
			MAPSERVER->SpawnMonsterBoss( (Map*)pdwOptional[0], (Map::SpawnSetting::Boss*)pdwOptional[1] );
			break;

		case 0x18:
			MAGICIANSKILL->UseDistortionUser( USERDATATOUSER((UserData*)pdwOptional[0]), pdwOptional[1] );
			break;

		case 0x19:
			BLESSCASTLESERVER->Start();
			break;

		default:
			break;
	}
}

std::string EWhereActionToStr(EWhereAction p_Where)
{
    switch (p_Where)
    {
        case WHEREID_None             : return "None";
        case WHEREID_Trade			  : return "Trade";
        case WHEREID_Distributor	  : return "Distributor";
        case WHEREID_Aging			  : return "Aging";
        case WHEREID_MixItem		  : return "MixItem";
        case WHEREID_ForceOrb		  : return "ForceOrb";
        case WHEREID_LearnSkill		  : return "LearnSkill";
        case WHEREID_WarpGate		  : return "WarpGate";
        case WHEREID_BuyItem		  : return "BuyItem";
        case WHEREID_BuyPotion		  : return "BuyPotion";
        case WHEREID_SellItem		  : return "SellItem";
        case WHEREID_LoadPlayer		  : return "LoadPlayer";
        case WHEREID_DropItem		  : return "DropItem";
        case WHEREID_GetItem		  : return "GetItem";
        case WHEREID_StarPoint		  : return "StarPoint";
        case WHEREID_ClanMoney		  : return "ClanMoney";
        case WHEREID_GiveMoney		  : return "GiveMoney";
        case WHEREID_PersonalShop	  : return "PersonalShop";
        case WHEREID_OpenWarehouse    : return "OpenWarehouse";
        case WHEREID_QuestGold		  : return "QuestGold";
        case WHEREID_FuryArena		  : return "FuryArena";
        case WHEREID_RepairItem       : return "RepairItem";
        case WHEREID_DropItemRestoreOk: return "DropItemRestoreOk";
        case WHEREID_Wings            : return "Wings";
    }

    return "Unknown";
}

BOOL __fastcall CustomAddGoldServer(UserData * p_UserData, void  *p_PlaceHolder, int p_Gold, EWhereAction p_Where)
{
    if (p_Where == EWhereAction::WHEREID_GetItem)
    {
        User * l_User = p_UserData->pcSocketData->u;

        uint32_t l_TickCount = TICKCOUNT;

        if ((l_TickCount - l_User->GetItemGoldLastTime) > MAX_GET_ITEM_GOLD_TIME)
            l_User->GetItemGoldTotal = 0;

        if ((l_TickCount - l_User->GetItemGoldLastTime) > (MAX_GET_ITEM_GOLD_TIME / 2))
            l_User->GetItemGoldTotal /= 2;

        l_User->GetItemGoldTotal    += p_Gold;
        l_User->GetItemGoldLastTime  = l_TickCount;

		//skip gold check for wanted wolf event
        if (EVENT_WANTEDWOLF_EVENT == FALSE && l_User->GetItemGoldTotal > MAX_GET_ITEM_GOLD_PER_TIME(p_UserData->GetLevel()))
        {
            PacketLogCheat sPacket;
            sPacket.iCheatID    = CHEATLOGID_GenerateGoldHack;
            sPacket.LParam      = p_Gold;
            sPacket.SParam      = p_Gold;
            sPacket.WxParam	    = 23;
            LOGSERVER->OnLogCheat(p_UserData, &sPacket);

			if ( l_User->GetGameLevel() != GAMELEVEL_Four )
			{
				NETSERVER->DisconnectUser( p_UserData );

				SENDPACKETBLANK( USERDATATOUSER( p_UserData ), PKTHDR_Disconnect );
				WNDPROC_SOCKETCLOSE( SOCKETSERVER->GetHWND(), p_UserData->pcSocketData );
			}
        }
    }

    std::string l_LogAction = EWhereActionToStr(p_Where);
    std::string l_ActionStr = "Add " + std::to_string(p_Gold) + " gold, reason : " + l_LogAction + " server balance before " + std::to_string(p_UserData->GetGold());

    SQLConnection * l_Connection = SQLCONNECTION(DATABASEID_LogDB, 33);
    if (l_Connection->Open())
    {
        if (l_Connection->Prepare("INSERT INTO GoldLog VALUES (?,?,?,?,?,?,?)"))
        {
            char szDateTime[20];
            GetDateTime(szDateTime);
            int l_Action = p_Gold;
            l_Connection->BindParameterInput(1, PARAMTYPE_String, p_UserData->szAccountName);
            l_Connection->BindParameterInput(2, PARAMTYPE_Integer, &p_Where);
            l_Connection->BindParameterInput(3, PARAMTYPE_Integer, &l_Action);
            l_Connection->BindParameterInput(4, PARAMTYPE_Integer, &p_UserData->iInventoryGold);
            l_Connection->BindParameterInput(5, PARAMTYPE_String, (char*)l_ActionStr.c_str());
            l_Connection->BindParameterInput(6, PARAMTYPE_String, szDateTime);
            l_Connection->BindParameterInput(7, PARAMTYPE_Integer, &GAME_SERVER);
            l_Connection->Execute();
        }
        l_Connection->Close();
    }

    typedef BOOL(__thiscall * tfnAddGoldServer)(UserData * pcUserData, int iGold, EWhereAction eWhere);
    tfnAddGoldServer fnAddGoldServer = (tfnAddGoldServer)0x00575A30;

    return fnAddGoldServer(p_UserData, p_Gold, p_Where);
}
BOOL __fastcall CustomSubGoldServer(UserData * p_UserData, void * p_PlaceHolder, int p_Gold, EWhereAction p_Where)
{
    std::string l_LogAction = EWhereActionToStr(p_Where);
    std::string l_ActionStr = "Remove " + std::to_string(p_Gold) + " gold, reason : " + l_LogAction + " server balance before " + std::to_string(p_UserData->GetGold());

    SQLConnection * l_Connection = SQLCONNECTION(DATABASEID_LogDB, 34);
    if (l_Connection->Open())
    {
        if (l_Connection->Prepare("INSERT INTO GoldLog VALUES (?,?,?,?,?,?,?)"))
        {
            char szDateTime[20];
            GetDateTime(szDateTime);
            int l_Action = -p_Gold;
            l_Connection->BindParameterInput(1, PARAMTYPE_String, p_UserData->szAccountName);
            l_Connection->BindParameterInput(2, PARAMTYPE_Integer, &p_Where);
            l_Connection->BindParameterInput(3, PARAMTYPE_Integer, &l_Action);
            l_Connection->BindParameterInput(4, PARAMTYPE_Integer, &p_UserData->iInventoryGold);
            l_Connection->BindParameterInput(5, PARAMTYPE_String, (char *)l_ActionStr.c_str());
            l_Connection->BindParameterInput(6, PARAMTYPE_String, szDateTime);
            l_Connection->BindParameterInput(7, PARAMTYPE_Integer, &GAME_SERVER);
            l_Connection->Execute();
        }
        l_Connection->Close();
    }

	//int	rsPLAYINFO::SubServerMoney( int Money , int WhereParam )
    typedef BOOL(__thiscall * tfnSubGoldServer)(UserData * pcUserData, int iGold, EWhereAction eWhere);
    tfnSubGoldServer fnSubGoldServer = (tfnSubGoldServer)0x00575AF0;

    return fnSubGoldServer(p_UserData, p_Gold, p_Where);
}


BOOL UserServer::AddServerUserGold( UserData * pcUserData, int iGold, EWhereAction eWhere )
{
    return CustomAddGoldServer(pcUserData, nullptr, iGold, eWhere);
}

BOOL UserServer::SubServerUserGold( UserData * pcUserData, int iGold, EWhereAction eWhere )
{
    return CustomSubGoldServer(pcUserData, nullptr, iGold, eWhere);
}


DWORD UserServer::GetQuestLogBit( UserData * pcUserData )
{
	//Check
	if ( pcUserData->psCharacterFile == NULL )
		return FALSE;

	PacketCharacterRecordData * psCharacter = (PacketCharacterRecordData*)pcUserData->psCharacterFile;

	//DWORD pp = (DWORD)&psCharacter->sCharacterSaveData;
	//DWORD dwQuestLog = *(DWORD *)( pp + 0xA0 );

	return psCharacter->sCharacterSaveData.iQuestLog;
}

/// <summary>
/// Get Character Tier Rank
/// 0 = Tier 1
/// 1 = Tier 2
/// 2 = Tier 3
/// 3 = Tier 4
/// </summary>
DWORD UserServer::GetCharacterTier( UserData * pcUserData )
{
	//Check
	if ( pcUserData->psCharacterFile == NULL )
		return 0;

	PacketCharacterRecordData * psCharacter = (PacketCharacterRecordData*)pcUserData->psCharacterFile;

	//DWORD pp = (DWORD)&psCharacter->sCharacterSaveData;
	//DWORD dwQuestLog = *(DWORD *)( pp + 0xA0 );

	return psCharacter->sCharacterData.iRank;
}

BOOL UserServer::InventoryServerCheck( UserData * pcUserData, int iUnused )
{
	User * pcUser = USERDATATOUSER( pcUserData );

	//Check
	if ( pcUserData->psCharacterFile == NULL )
		return FALSE;

	if ( pcUserData->iGameLevel >= GAMELEVEL_Three )
		return FALSE;

	PacketCharacterRecordData * psCharacter = (PacketCharacterRecordData*)pcUserData->psCharacterFile;

	RecordItem sRecordItem;

	DropItemData sDropData[64];

	ZeroMemory( sDropData, sizeof( DropItemData ) * 64 );

	BYTE * pbaItems = psCharacter->baData;

	int iBufferWheel = 0;

	int iCount = 0;

	int iDropItemCount = 0;

	for ( int i = 0; i < psCharacter->iItemCount; i++ )
	{
		//Inventory Limit...
		if ( i >= 200 )
			break;

		ITEMSERVER->DecodeItemsData( pbaItems, &sRecordItem, 44 );

		if ( sRecordItem.sItem.sItemID.ToInt() && sRecordItem.sItem.iChk1 && sRecordItem.sItem.iChk2 )
		{
			iCount = 0;

			if ( sRecordItem.sItem.sItemID.ToItemBase() == ITEMBASE_Potion )
			{
				if ( pcUserData->iTradePotion > 0 )
				{
					if ( ITEMSERVER->GetDropPotion( pcUserData, sRecordItem.sItem.sItemID.ToInt() ) )
						iCount = 1;
				}
				else
					iCount = (-1);
			}
			else
			{
				//Inventory 1 and 2, backup
				for ( int j = 0; j < 300; j++ ) //why not 316..? for later.
				{
					DropItemData * ps = pcUserData->sIntentoryItems + j;

					if ( ps->iItemID && (ps->iChk1 == sRecordItem.sItem.iChk1) && (ps->iChk2 == sRecordItem.sItem.iChk2) )
					{
						//DEBUG ( "InventoryServerCheck - Setting item '%d' as INVALID", ps->iItemID );

						ps->iItemID = ITEMID_None;

						iCount++;
					}
				}

				//Warehouse
				for ( int j = 0; j < 420; j++ )
				{
					DropItemData * ps = pcUser->sWarehouseItems + j;

					if ( ps->iItemID && (ps->iChk1 == sRecordItem.sItem.iChk1) && (ps->iChk2 == sRecordItem.sItem.iChk2) )
					{
						PacketLogCheat s;
						s.iCheatID = CHEATLOGID_ItemErrorInventoryRecord;
						s.WxParam = 1;
						s.LxParam = (int)"*RECORD ITEM WAREHOUSE";
						s.LParam = sRecordItem.sItem.sItemID.ToInt();
						s.SParam = sRecordItem.sItem.iChk1;
						s.EParam = sRecordItem.sItem.iChk2;

						LOGSERVER->OnLogCheat( pcUserData, &s );
						iCount++;
					}
				}
			}

			//Not Found...
			if ( iCount == 0 )
			{

				int iLoop = 0;

				for ( iLoop = 0; iLoop < psCharacter->iDropItemCount; iLoop++ )
				{
					DropItemData * ps = psCharacter->sDropItemData + iLoop;

					if ( (ps->iItemID == sRecordItem.sItem.sItemID.ToInt()) && (ps->iChk1 == sRecordItem.sItem.iChk1) && (ps->iChk2 == sRecordItem.sItem.iChk2) )
						break;
				}


				if ( iDropItemCount < 64 )
				{
					int k = 0;

					for ( k = 0; k < iDropItemCount; k++ )
					{
						DropItemData * ps = sDropData + k;

						if ( (ps->iItemID == sRecordItem.sItem.sItemID.ToInt()) && (ps->iChk1 == sRecordItem.sItem.iChk1) && (ps->iChk2 == sRecordItem.sItem.iChk2) )
							break;
					}

					if ( k >= iDropItemCount )
					{
						sDropData[iDropItemCount].iItemID = sRecordItem.sItem.sItemID.ToInt();
						sDropData[iDropItemCount].iChk1 = sRecordItem.sItem.iChk1;
						sDropData[iDropItemCount].iChk2 = sRecordItem.sItem.iChk2;
						iDropItemCount++;
					}
				}

				if ( iLoop >= psCharacter->iDropItemCount )
				{
					PacketLogCheat s;
					s.iCheatID = CHEATLOGID_ItemErrorInventoryRecord;
					s.WxParam = 1;
					s.LxParam = (int)"*RECORD ITEM";
					s.LParam = sRecordItem.sItem.sItemID.ToInt();
					s.SParam = sRecordItem.sItem.iChk1;
					s.EParam = sRecordItem.sItem.iChk2;

					LOGSERVER->OnLogCheat( pcUserData, &s );
				}
			}
			else
			{
				if ( iCount > 0 )
				{
					for ( int j = 0; j < psCharacter->iDropItemCount; j++ )
					{
						DropItemData * ps = psCharacter->sDropItemData + j;

						if ( (ps->iItemID == sRecordItem.sItem.sItemID.ToInt()) && (ps->iChk1 == sRecordItem.sItem.iChk1) && (ps->iChk2 == sRecordItem.sItem.iChk2) )
						{
							if ( iDropItemCount < 64 )
							{
								sDropData[iDropItemCount].iItemID = sRecordItem.sItem.sItemID.ToInt();
								sDropData[iDropItemCount].iChk1 = sRecordItem.sItem.iChk1;
								sDropData[iDropItemCount].iChk2 = sRecordItem.sItem.iChk2;
								iDropItemCount++;
							}
							break;
						}
					}
				}
			}

			if ( iCount > 1 )
			{
				PacketLogCheat s;
				s.iCheatID = CHEATLOGID_ItemErrorInventoryRecord;
				s.WxParam = iCount;
				s.LxParam = (int)"*RECORD COPIED ITEM";
				s.LParam = sRecordItem.sItem.sItemID.ToInt();
				s.SParam = sRecordItem.sItem.iChk1;
				s.EParam = sRecordItem.sItem.iChk2;

				LOGSERVER->OnLogCheat( pcUserData, &s );
			}
		}

		int iSize = ((int*)pbaItems)[0];

		iBufferWheel += iSize;

		pbaItems += iSize;

		if ( iBufferWheel >= (sizeof( RecordItem ) * 200) )
			break;
	}

	if ( iDropItemCount > 0 )
	{
		CopyMemory( psCharacter->sDropItemData, sDropData, sizeof( DropItemData ) * iDropItemCount );
		psCharacter->iDropItemCount = iDropItemCount;
	}

	if ( psCharacter->sCharacterData.iGold > pcUserData->GetGold())
	{
		PacketLogCheat s;
		s.iCheatID = CHEATLOGID_ServerMoneyErrorInventory;
		s.LParam = 1;
		s.SParam = pcUserData->GetGold();
		s.EParam = psCharacter->sCharacterData.iGold;
		LOGSERVER->OnLogCheat( pcUserData, &s );

		psCharacter->sCharacterData.iGold = pcUserData->GetGold();

		pcUserData->iSaveGold = 0;

		if ( psCharacter->sCharacterData.iGold < 0 )
			psCharacter->sCharacterData.iGold = 0;
	}

	int iGold = psCharacter->sCharacterSaveData.iLastGold;

	if ( iGold && (iGold - 1) > pcUserData->GetGold())
	{
		PacketLogCheat s;
		s.iCheatID = CHEATLOGID_ServerMoneyErrorInventory;
		s.LParam = 3;
		s.SParam = pcUserData->GetGold();
		s.EParam = psCharacter->sCharacterSaveData.iLastGold - 1;
		LOGSERVER->OnLogCheat( pcUserData, &s );

		psCharacter->sCharacterSaveData.iLastGold = pcUserData->GetGold() + 1;

		if ( psCharacter->sCharacterSaveData.iLastGold < 0 )
			psCharacter->sCharacterSaveData.iLastGold = 0;
	}

	return TRUE;
}

BOOL UserServer::AddUserOnline( UserData * pcUserData, BOOL bOffline )
{

	return TRUE;
}

BOOL UserServer::OnTradeRequest( UserData * pcUserData, PacketSimple * psPacket)
{
	UserData * pcUserDataTarget = GetUserdata( psPacket->iUnk2 );

	User * pcUser = USERDATATOUSER( pcUserDataTarget );


	if ( pcUserDataTarget && pcUserDataTarget->pcSocketData )
	{
		if ( pcUserData->bBlockAction || pcUserDataTarget->bBlockAction )
			return FALSE;
		if ( pcUserData->szOwner[0] || *(BOOL*)(0x00848314) )
			return FALSE;

		//Does the other player have trade disabled? - Send message to request sender.
		//I cant figure this out for some reason
		/*if (pcUserDataTarget->sSettingsData.bNoTradeRequest)
		{
			CHATSERVER->SendChatEx(USERDATATOUSER(pcUserData), CHATCOLOR_Error, "> %s has trade disabled.", CHARACTERSERVER->GetCharacterName(pcUserDataTarget));
			return FALSE;
		}*/
		//Target has trade disabled, send message to receiver.
		if ( pcUser->sSettingsData.bNoTradeRequest )
		{
			CHATSERVER->SendChatEx( USERDATATOUSER( pcUserData ), CHATCOLOR_Error, "> %s traded you, but you have trade disabled!", CHARACTERSERVER->GetCharacterName(pcUserDataTarget));
			return FALSE;
		}
		SENDPACKET( USERDATATOUSER( pcUserDataTarget ), psPacket, TRUE );
	}
	return TRUE;
}

BOOL UserServer::OnLoadUser( User * pcUser )
{
	//std::cout << "UserServer::OnLoadUser" << std::endl;

	//DEBUGSTATUS( "OnLoadUser" );

	if (LOGIN_SERVER)
	{
		if ( pcUser && pcUser->pcUserData )
		{
			//INFO ( "User loaded: %s (%d)", CHARACTERSERVER->GetCharacterName ( pcUser->pcUserData ), pcUser->GetID() );
			LOGSERVER->OnLogEx( pcUser, ACCLOGID_CharacterLoad, "[%s][%s] OnLoadUser (ID = %d)", (GAME_SERVER ? "GS" : "LS"), CHARACTERSERVER->GetCharacterName ( pcUser->pcUserData ), pcUser->GetID() );
		}
		else
		{
			return FALSE;
		}

		USERSERVER->UpdateCharacterIDFromSQL(pcUser);
		//USERSERVER->SyncUserMuteStatus( pcUser );

		pcUser->uUserLoadedTime = TIME;

		//I think the 'User' or 'UserData' on Game server isn't ready yet while
		// in this code. Use CharacterServer::OnCharacterSyncData instead which occurs on player character load

		CALL_WITH_ARG1(0x00557F10, (DWORD)pcUser->pcUserData);
		CALL_WITH_ARG1(0x00551EE0, (DWORD)pcUser->pcUserData);
	}

	return TRUE;
}

BOOL UserServer::OnUnLoadUser( User * pcUser )
{
	char * charName = CHARACTERSERVER->GetCharacterName( pcUser->pcUserData );

	//no char loaded
	if ( STRLEN( charName ) == 0 )
	{
		DEBUG( "No char was loaded for account id: %d", pcUser->iAccountID );
		return FALSE;
	}

	// Load Quests
	QUESTSERVER->OnUnLoadUser( pcUser );
	PVPSERVER->OnUnLoadUser( pcUser );
	ITEMSERVER->OnUnLoadUser( pcUser );

	//Only for login servers
	if ( LOGIN_SERVER )
	{
		CHARACTERSERVER->SQLSaveCharacterDataEx( pcUser );

		NETSERVER->OnUnloadUser( pcUser->pcUserData );
	}
	else
		PETSYSTEMHANDLER->RemovePet(pcUser->pcUserData);

	return TRUE;
}

void UserServer::ResetSkills( User * pcUser )
{
	pcUser->pcUserData->dwAutomationEndTime		= 0;
	pcUser->pcUserData->iAutomationDamageBoost	= 0;

	pcUser->pcUserData->dwMaximizeEndTime		= 0;
	pcUser->pcUserData->iMaximizeMaxDamageBoost = 0;

	pcUser->pcUserData->dwPhysicalAbsorbEndTime = 0;
	pcUser->pcUserData->iPhysicalAbsorbBoost	= 0;

	pcUser->pcUserData->dwExtremeShieldEndTime	= 0;
	pcUser->pcUserData->iExtremeShield			= 0;

	pcUser->pcUserData->dwHolyBodyEndTime		= 0;
	pcUser->pcUserData->iHolyBodyAbsorb			= 0;

	pcUser->pcUserData->dwHolyValorEndTime_Obsolete		= 0;
	pcUser->pcUserData->iHolyValorParam_Obsolete			= 0;

	pcUser->pcUserData->dwHolyReflectionEndTime = 0;
	pcUser->pcUserData->iHolyReflectionDamage	= 0;

	pcUser->pcUserData->dwEnchantWeaponEndTime_DontUse	= 0;
	pcUser->pcUserData->iEnchantWeaponAttr_DontUse		= 0;

	pcUser->pcUserData->dwMetalArmorEndTime		= 0;
	pcUser->pcUserData->iMetalArmorParam		= 0;

	pcUser->pcUserData->dwSparkShieldEndTime	= 0;
	pcUser->pcUserData->iSparkShieldDamage		= 0;

	pcUser->pcUserData->dwFalconEndTime			= 0;
	pcUser->pcUserData->saFalconDamage[0]		= 0;
	pcUser->pcUserData->saFalconDamage[1]		= 0;

	pcUser->pcUserData->dwDivineShieldEndTime		= 0;
	pcUser->pcUserData->sDivineShieldAbsorb			= 0;
	pcUser->pcUserData->sDivineShieldAttackRating	= 0;

	pcUser->pcUserData->dwTriumphOfValhallaEndTime_Obsolete	= 0;
	pcUser->pcUserData->iTriumphOfValhallaDamage_Obsolete	= 0;

	pcUser->pcUserData->dwVirtualLifeEndTime	= 0;
	pcUser->pcUserData->iVirtualLifeData		= 0;

	pcUser->pcUserData->dwEnergyShieldEndTime	= 0;
	pcUser->pcUserData->iEnergyShieldDamageReduce		= 0;

	pcUser->pcUserData->dwSpiritElementalEndTime	= 0;
	pcUser->pcUserData->iSpiritElementalBoost		= 0;

	pcUser->pcUserData->dwDancingSwordEndTime_NotUsed	= 0; //obsolete
	pcUser->pcUserData->iDancingSwordParam_NotUsed		= 0; //obsolete
	pcUser->pcUserData->dwDancingSwordDelayTime = 0;

	pcUser->pcUserData->dwVanishEndTime			= 0;
	pcUser->pcUserData->saVanishParam[0]		= 0;
	pcUser->pcUserData->saVanishParam[1]		= 0;

	pcUser->pcUserData->dwHookedTargetEndTime	= 0;
	pcUser->pcUserData->iCompulsionParam		= 0;

	pcUser->pcUserData->dwMagneticSphereEndTime = 0;
	pcUser->pcUserData->iMagneticSphereParam	= 0;

	pcUser->pcUserData->dwBerserkerEndTime_Obsolete		= 0;
	pcUser->pcUserData->iBerserkerParam_Obsolete			= 0;

	pcUser->pcUserData->dwAssassinEyeEndTime	= 0;
	pcUser->pcUserData->AssassinEyeParam		= 0;

	pcUser->pcUserData->dwVagueEndTime			= 0;
	pcUser->pcUserData->iVagueParam				= 0;

	pcUser->pcUserData->dwForceOfNatureEndTime	= 0;
	pcUser->pcUserData->saForceOfNatureParam[0] = 0;
	pcUser->pcUserData->saForceOfNatureParam[1] = 0;

	pcUser->pcUserData->dwGodlyShiedEndTime		= 0;
	pcUser->pcUserData->iGodlyShiedParam		= 0;

	pcUser->pcUserData->GodsBlessingTime		= 0;
	pcUser->pcUserData->GodsBlessingParam		= 0;

	pcUser->pcUserData->HallOfValhallaTimeObsolete		= 0;
	pcUser->pcUserData->HallOfValhallaParamObsolete[0]	= 0;
	pcUser->pcUserData->HallOfValhallaParamObsolete[1]	= 0;

	pcUser->pcUserData->FrostTime_DoNotUse		= 0;
	pcUser->pcUserData->FrostParam_DoNotUse		= 0;

	pcUser->pcUserData->RegenerationFieldTime	= 0;

	pcUser->pcUserData->SummonMuspellTime		= 0;
	pcUser->pcUserData->SummonMuspellParam		= 0;

	pcUser->uFierceWindLevel					= 0;
	pcUser->dwFierceWindTimeOut					= 0;

	pcUser->uHolyConvictionLevel				= 0;
	pcUser->dwHolyConvictionTimeOut				= 0;
}


void UserServer::HandlePacket( User * pcUser, PacketPlayData * psPacket )
{
	//Entered a new map
	if ( (int)psPacket->sMapID != pcUser->pcUserData->iMapID )
	{
		USERSERVER->OnLeaveMap(pcUser,pcUser->pcUserData->iMapID  );

		if ( CHARACTERSERVER->CharacterInTown(  (EMapID)psPacket->sMapID ) )
		{
			ResetSkills( pcUser );
			DAMAGEHANDLER->ClearExpiredSkillBuffs( pcUser, TRUE );
		}

		//Boss Time
		if ( (psPacket->sMapID >= 0) && (psPacket->sMapID <= NUM_MAPS) )
			MAPSERVER->UpdateUserBossTime( pcUser, psPacket->sMapID );

		MAPSERVER->SendSpawnFlagMarkers( pcUser, psPacket->sMapID );

		MAPSERVER->SendMapIndicators( pcUser, psPacket->sMapID );
		MAPSERVER->SendMapIndicators( pcUser, psPacket->sSecondMapID );

	}

	pcUser->eMapID = (EMapID)psPacket->sMapID;
	pcUser->pcUserData->iMapID = (EMapID)psPacket->sMapID;
	pcUser->eSecondMapID = (EMapID)psPacket->sSecondMapID;
	pcUser->pcUserData->sCharacterData.sMP = psPacket->sMP;
	pcUser->pcUserData->sCharacterData.sSP = psPacket->sSP;
	pcUser->pcUserData->sCharacterData.iAttackSpeed		= psPacket->saSpeed[0];
	pcUser->pcUserData->sCharacterData.iMovementSpeed	= psPacket->saSpeed[1];
	pcUser->pcUserData->sAngle.iY = psPacket->sAngleY;
	pcUser->pcUserData->sCharacterData.sSize = psPacket->sSize;
	pcUser->bInvisibleInMiniMap = psPacket->bInvisibleInMiniMap;

	BOOL bLootFilterChanged = FALSE;

	if (pcUser->bLootFilterEnabled != psPacket->bLootFilterEnabled ||
		pcUser->iLootFilterFlags != psPacket->iLootFilterFlag ||
		pcUser->sEquipmentMinLevel != psPacket->sEquipmentMinLevel)
	{
		bLootFilterChanged = TRUE;
	}

	pcUser->bLootFilterEnabled = psPacket->bLootFilterEnabled;
	pcUser->iLootFilterFlags = psPacket->iLootFilterFlag;
	pcUser->sEquipmentMinLevel = psPacket->sEquipmentMinLevel;


	psPacket->iLevel = pcUser->pcUserData->sCharacterData.iLevel;

	//Buffer
	if ( psPacket->iPlayBuffCount < 32 )
	{
		int iIndex = 0;

		//idling
		if ( psPacket->iPlayBuffCount <= 1 )
		{
			if ( pcUser->pcUserData->bNotMovingCounter == 0 )
				pcUser->pcUserData->bNotMovingCounter = 1;

			iIndex = 4; //4 only
		}

		//is moving
		else
		{
			iIndex = pcUser->pcUserData->uUnitStatusCounter % 4; //0-3 only
			pcUser->pcUserData->bNotMovingCounter = 0;
			pcUser->pcUserData->uUnitStatusCounter++;
		}

		CopyMemory( pcUser->baUnitStatusNew[iIndex], psPacket, psPacket->iLength );
		pcUser->pcUserData->dwaUnitStatusSize[iIndex] = psPacket->iLength;
	}

	//Force update items around user
	if (bLootFilterChanged)
	{
		Map * pcMap = MAPSDATA + pcUser->GetMapID();
		if( !pcMap )
			return;

		MAPSERVER->SendStageItem( pcMap, pcUser );
	}
}

void UserServer::OnKilled( User * pcUser, PacketDeadCharacter * psPacket )
{
	ResetSkills( pcUser );
	DAMAGEHANDLER->ClearExpiredSkillBuffs( pcUser, TRUE );

	User * pcKiller = USERDATATOUSER( USERDATABYID( psPacket->dwObjectID ) );
	if( pcKiller )
	{
		if( LOGIN_SERVER )
			PVPSERVER->OnKilled( pcKiller, pcUser );
		else if( GAME_SERVER )
			ITEMSERVER->ProcessIntegrity( pcUser, INTEGRITYUPDATE_KilledUser );

		return;
	}
	else if( GAME_SERVER )
		ITEMSERVER->ProcessIntegrity( pcUser, INTEGRITYUPDATE_KilledMonster );

	if( LOGIN_SERVER )
		psPacket->iHeader = 0;
}

void UserServer::SettingsDataSet( User * pcUser, PacketSettingsData * psPacket )
{
	if( !GAME_SERVER )
	{
		if( pcUser->sSettingsData.bNoWhisper != psPacket->bNoWhisper )
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> Whisper %s!", pcUser->sSettingsData.bNoWhisper ? "enabled" : "disabled" );

		if( pcUser->sSettingsData.bNoTradeChat != psPacket->bNoTradeChat )
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> Trade Chat %s!", pcUser->sSettingsData.bNoTradeChat ? "enabled" : "disabled" );

		if( pcUser->sSettingsData.bNoPartyRequest != psPacket->bNoPartyRequest )
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> Party Requests %s!", pcUser->sSettingsData.bNoPartyRequest ? "enabled" : "disabled" );

		if( pcUser->sSettingsData.bNoTradeRequest != psPacket->bNoTradeRequest )
			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> Trade Requests %s!", pcUser->sSettingsData.bNoTradeRequest ? "enabled" : "disabled" );

		//not initial load
		if ( psPacket->bInital == FALSE )
		{
#ifdef ENCHANTWEAPON_SETTING
			if ( pcUser->sSettingsData.bWeaponEnchantPerference != psPacket->bWeaponEnchantPerference )
				CHATSERVER->SendChat( pcUser, CHATCOLOR_Error, "> Enchant Weapon perference updated!" );
#endif
		}

		//initial load
		else
		{

		}
	}

	CopyMemory( &pcUser->sSettingsData, psPacket, sizeof( PacketSettingsData ) );
	pcUser->pcUserData->bDisabledWhisper = *(bool*)&pcUser->sSettingsData.bNoWhisper;
}

void UserServer::UpdatePetData( User * pcUser, PacketUpdatePetData * psPacket )
{
	switch( psPacket->dwSkillID )
	{
		case SKILLID_RecallHestian:
		case SKILLID_MetalGolem:
		case SKILLID_RecallWolverine:
		case SKILLID_FireElemental:
		case SKILLID_IceElemental:
		case SKILLID_SummonTiger:
		{
			if ( pcUser->pcUserData->pcPetData && pcUser->pcUserData->pcPetData->iID == pcUser->pcUserData->dwObjectPetID && pcUser->pcUserData->pcPetData->iPetID )
			{
				//std::cout << "pcUser->pcUserData->pcPetData->bAggressive: " << psPacket->bAutoAttack << std::endl;

				pcUser->pcUserData->pcPetData->bAggressive = psPacket->bAutoAttack;
				pcUser->pcUserData->pcPetData->pcFocusTarget = NULL;
				pcUser->pcUserData->pcPetData->pcAttackTarget = NULL;
				pcUser->pcUserData->pcPetData->pcFocusingTarget = NULL;

				if ( pcUser->pcUserData->pcPetData->iFocusingLastID != ( -1 ) )
				{
					PacketUpdatePetDataEx sPacket;
					sPacket.iLength = sizeof( PacketUpdatePetDataEx );
					sPacket.iHeader = PKTHDR_UpdatePetDataEx;
					sPacket.iTargetID = -1;
					sPacket.iLastTargetID = pcUser->pcUserData->pcPetData->iFocusingLastID;
					SENDPACKET( pcUser, &sPacket );
				}

				pcUser->pcUserData->pcPetData->iFocusingLastID = -1;
			}
			break;
		}

		default:
			break;
	}
}

void UserServer::UpdatePetDataEx( User * pcUser, PacketUpdatePetDataEx * psPacket )
{
	if( pcUser->pcUserData->pcPetData && pcUser->pcUserData->pcPetData->iID == pcUser->pcUserData->dwObjectPetID && pcUser->pcUserData->pcPetData->iPetID )
	{
		UnitData * pcUnitData = UNITSERVER->GetUnit( psPacket->iTargetID );

		if( pcUnitData )
		{
			psPacket->iLastTargetID = pcUser->pcUserData->pcPetData->iFocusingLastID;

			pcUnitData->Animate( ANIMATIONTYPE_Idle );

			pcUser->pcUserData->pcLastAttack						= NULL;
			pcUser->pcUserData->pcPetData->pcFocusTarget			= NULL;
			pcUser->pcUserData->pcPetData->pcAttackTarget			= NULL;
			pcUser->pcUserData->pcPetData->pcFocusingTarget			= pcUnitData;
			pcUser->pcUserData->pcPetData->iFocusingLastID			= pcUnitData->iID;

			CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> Pet is focusing on %s", pcUnitData->sCharacterData.szName );
			SENDPACKET( pcUser, psPacket );
		}
	}
}

void UserServer::DisconnectOldUser( User * pcUser )
{
	for ( int i = 0; i < PLAYERS_MAX; i++ )
	{
		UserData * ud = &USERSDATA[i];
		if ( ud != pcUser->pcUserData &&
			( ud->iID == pcUser->GetID() ||
			  STRINGCOMPAREI( CHARACTERSERVER->GetCharacterName( ud ), CHARACTERSERVER->GetCharacterName( pcUser->pcUserData ) )) )
		{
			std::string charName = CHARACTERSERVER->GetCharacterName( pcUser->pcUserData );

			if ( charName.empty() )
				charName = pcUser->pcUserData->pcSocketData->szIP;

			LOGSERVER->OnLogEx( pcUser, ACCLOGID_CharacterLoad, "[%s][%s] DisconnectOldUser (old: %d, new: %d)", ( GAME_SERVER ? "GS" : "LS" ), charName.c_str(), ud->iID, pcUser->GetID() );

			if ( ud->pcSocketData )
			{
				LOGSERVER->LogSocketClose(ud->pcSocketData, FormatString("UserServer::DisconnectOldUser (%d)", ud->iID), _ReturnAddress());
				WNDPROC_SOCKETCLOSE(SOCKETSERVER->GetHWND(), ud->pcSocketData);
			}
			else
			{
				ZeroMemory(ud, 0xB510);
			}

			break;
		}
	}
}

void UserServer::ReconnectUser(UserData* pcUserData, struct PacketReconnectUser * psPacket )
{
#ifdef RECONNECT_SYSTEM
	SocketData* pcSocket = pcUserData->pcSocketData;
	User* pcUser = USERDATATOUSER(pcUserData);

	for (int i = PLAYERS_MAX; i < PLAYERS_MAX + 256; i++)
	{
		UserData* currentUserData = (USERSDATA + i);
		User* currentUser = USERDATATOUSER(currentUserData);

		if (currentUserData && currentUser->bNetServer == FALSE && currentUserData->szAccountName[0] && (psPacket->dwObjectID == currentUserData->iID) && STRINGCOMPAREI(currentUserData->szAccountName, psPacket->szUserID) && currentUserData != pcUserData && pcUserData->pcSocketData)
		{
			CharacterFile* pCharaerFile = pcUserData->psCharacterFile;

			int iIndex = pcUser->uIndex;

			CopyMemory(pcUserData, currentUserData, sizeof(UserData));

			pcUser->pcUserData->psCharacterFile = pCharaerFile;

			if (pcUser->pcUserData->psCharacterFile && currentUserData->psCharacterFile)
			{
				CopyMemory(pcUserData->psCharacterFile, currentUserData->psCharacterFile, sizeof(CharacterFile));
				pcUserData->psCharacterFile->iLength = 0;
			}

			//Clearold
			ZeroMemory(pcUserData, sizeof(UserData));

			pcUser->uIndex = iIndex;
			pcUserData->iID = psPacket->dwObjectID;
			pcUserData->pcSocketData = pcSocket;

			USERSERVER->UpdateCharacterIDFromSQL(pcUser);

			if ((pcUserData->szAccountName[0] != 0) && (CHARACTERSERVER->GetCharacterName(pcUserData)[0] != 0))
			{
				ACCOUNTSERVER->SQLUserOnline(pcUserData->szAccountName, "", "", 0, 1, 0, 0);
				ACCOUNTSERVER->SQLUserOnline(pcUserData->szAccountName,
					CHARACTERSERVER->GetCharacterName(pcUser),
					pcSocket->szIP,
					pcUserData->iTicket,
					0,
					pcUserData->sCharacterData.iClass,
					pcUserData->sCharacterData.iLevel);
			}

			pcUser->bParty = FALSE;
			pcUser->psParty = NULL;

#ifdef RECONNECT_SYSTEM
			SOCKETSERVER->AddReconnectUser(pcUserData);
#endif

			if (!GAME_SERVER)
				SENDPACKETBLANK(pcUser, PKTHDR_Save);

			break;
		}
	}
#endif
}

BOOL UserServer::DeleteReconnectUser( const char * pszAccountName )
{
	BOOL bRet = FALSE;

	return bRet;
}

DWORD UserServer::AllowPacket( User * pcUser )
{
	DWORD dwTickCount = GetTickCount();

	UserData * ud = pcUser->pcUserData;

	if( pcUser->bNetServer == FALSE )
	{
		//Flood protection
		if ( ud->iPacketCounter > MAX_PACKETFLOW )
		{
			PacketLogCheat s;
			s.LParam = ud->iPacketCounter;
			s.SParam = MAX_PACKETFLOW;
			s.EParam = TIM_PACKETFLOW;
			s.iCheatID = CHEATLOGID_TooManyPackets;
			LOGSERVER->OnLogCheat( ud, &s );
			return 0;
		}

		if( (dwTickCount - ud->dwTickPacketCounter) > TIM_PACKETFLOW )
		{
			ud->iPacketCounter		= 0;
			ud->dwTickPacketCounter	= dwTickCount;
		}
		else
			ud->iPacketCounter++;
	}

	ud->dwTimeLastPacket = dwTickCount;

	return dwTickCount;
}

uint32_t hash_data(char * p_Data, std::size_t p_Size)
{
	uint32_t hash = 0;

	for (std::size_t l_I = 0; l_I < p_Size; ++l_I)
	{
		hash += p_Data[l_I];
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}

	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);

	return hash;
}
void UserServer::Update()
{
	//LoginServers and GameServers
	static UINT iWheel = 0;

	USERSERVER->uUsersInGame = 0;

	USERS_ONLINE = 0;

	for( UINT i = 0; i < PLAYERS_MAX; i++ )
	{
		User * pcUser			= pcaUser + i;
		UserData * pcUserData	= pcUser->pcUserData;

		if( pcUser && pcUserData && pcUser->IsConnected() )
		{
			if( pcUser->IsInGame() )
			{
				USERS_ONLINE++;

				USERSERVER->pcaUserInGame[USERSERVER->uUsersInGame] = pcUser;
				USERSERVER->uUsersInGame++;

				if( GAME_SERVER )
				{
					UpdateUnitStatus( pcUser );		//64 FPS
					//UpdateSkillStatus ( pcUser );	//64 FPS

					//Unit (monster/NPC) status replication tick.
					//UNIT_STATUS_UPDATE_DIVISOR: 8 = 8Hz (legacy-ish), 4 = 16Hz,
					//2 = 32Hz, 1 = 64Hz (every frame, no load spreading).
					if ( UNIT_STATUS_UPDATE_DIVISOR <= 1 )
					{
						pcUser->bUnitStatus = TRUE;
					}
					else
					{
						pcUser->bUnitStatus =
							( ( i % UNIT_STATUS_UPDATE_DIVISOR ) == ( iWheel % UNIT_STATUS_UPDATE_DIVISOR ) );
					}

					//8 times per second
					if ( ( i % 8 ) == ( iWheel % 8 ) )
					{
						pcUser->b8 = TRUE;
					}
					else
					{
						pcUser->b8 = FALSE;
					}

					//4 times per second
					if ( pcUser->b8 && (i % 16) == (iWheel % 16))
					{
						pcUser->b16 = TRUE;
					}
					else
					{
						pcUser->b16 = FALSE;
					}

					//2 times per second
					if ( pcUser->b16 && (i % 32) == (iWheel % 32))
					{
						pcUser->b32 = TRUE;
					}
					else
					{
						pcUser->b32 = FALSE;
					}

					//1 time per second
					if ( pcUser->b32 && ( i % 64 ) == ( iWheel % 64 ) )
					{
						//Deep Status Update
						pcUser->b64 = TRUE;

						pcUser->iSecondTick++; //increase by 1 every second

						if ( pcUser->iSecondTick % 10 == 0 )
						{
							pcUser->bTenSeconds = TRUE;   //flag to allow process
						}

						if ( pcUser->iSecondTick % 60 == 0 )
						{
							pcUser->bOneMin = TRUE;   //flag to allow process
						}
					}
					else
					{
						//No Deep Status Update
						pcUser->b64 = FALSE;
					}
				}
				else
				{
					DWORD l_LastHash = pcUser->LastInventoryHash;
					DWORD l_CurrentHash = hash_data((char*)&pcUserData->sIntentoryItems, sizeof(DropItemData) * 316);

					if (l_LastHash != l_CurrentHash)
					{
#if defined (DEV_MODE) && defined (DEBUG_INVENTORY)

						if ( pcUser->bLastDropItemsValidTemp )
						{
							for ( int i_this = 0; i_this < 316; i_this++ )
							{
								DropItemData * lsThisDrop = pcUserData->sIntentoryItems + i_this;

								if ( lsThisDrop->iItemID == 0 ||
									lsThisDrop->iChk1 == 0 ||
									lsThisDrop->iChk2 == 0 )
								{
									continue;
								}

								bool found = false;

								for ( int i_Prev = 0; i_Prev < 316; i_Prev++ )
								{
									DropItemData * lsPrevDrop = pcUser->sLastDropItemsTemp + i_Prev;

									if ( lsPrevDrop->iItemID == lsThisDrop->iItemID &&
										lsPrevDrop->iChk1 == lsThisDrop->iChk1 &&
										lsPrevDrop->iChk2 == lsThisDrop->iChk2 )
									{
										found = true;

										//found. Nothing chnaged
										break;
									}
								}

								if ( found == false )
								{
									auto lsItemDef = ITEMSERVER->FindItemDefByCode( lsThisDrop->iItemID );
									INFO( "Inventory - Item added: %s (%d, %d)", lsItemDef->sItem.szItemName, lsThisDrop->iChk1, lsThisDrop->iChk2 );
								}
							}


							bool found = false;

							for ( int i_Prev = 0; i_Prev < 316; i_Prev++ )
							{
								DropItemData * lsPrevDrop = pcUser->sLastDropItemsTemp + i_Prev;

								if ( lsPrevDrop->iItemID == 0 ||
									lsPrevDrop->iChk1 == 0 ||
									lsPrevDrop->iChk2 == 0 )
								{
									continue;
								}

								bool found = false;

								for ( int i_this = 0; i_this < 316; i_this++ )
								{
									DropItemData * lsThisDrop = pcUserData->sIntentoryItems + i_this;

									if ( lsPrevDrop->iItemID == lsThisDrop->iItemID &&
										lsPrevDrop->iChk1 == lsThisDrop->iChk1 &&
										lsPrevDrop->iChk2 == lsThisDrop->iChk2 )
									{
										found = true;

										//found. Nothing changed
										break;
									}
								}

								if ( found == false )
								{
									auto lsItemDef = ITEMSERVER->FindItemDefByCode( lsPrevDrop->iItemID );
									INFO( "Inventory - Item removed: %s (%d, %d)", lsItemDef->sItem.szItemName, lsPrevDrop->iChk1, lsPrevDrop->iChk2 );
								}
							}
						}
#endif


						DEBUG( "Inventory hash is different. Last = %I64d vs Current = %I64d. Syncing game server(s)", l_LastHash, l_CurrentHash );

						PacketNetPlayerInventory l_InvSyncPacket;
						l_InvSyncPacket.iLength = sizeof(PacketNetPlayerInventory);
						l_InvSyncPacket.iHeader = PKTHDR_NetPlayerInventory;
						l_InvSyncPacket.ObjectID = pcUserData->iID;
						memcpy(l_InvSyncPacket.sIntentoryItems, pcUserData->sIntentoryItems, sizeof(DropItemData) * 316);

						NETSERVER->SendPacketGameServers(&l_InvSyncPacket);

						pcUser->LastInventoryHash = l_CurrentHash;

#if defined (DEV_MODE) && defined (DEBUG_INVENTORY)
						CopyMemory( &pcUser->sLastDropItemsTemp, &pcUserData->sIntentoryItems, sizeof( DropItemData ) * 316 );
						pcUser->bLastDropItemsValidTemp = TRUE;
#endif
					}

					PacketNetPlayerGold l_NetPlayerGoldPacket;
					l_NetPlayerGoldPacket.iLength				= sizeof(PacketNetPlayerGold);
					l_NetPlayerGoldPacket.iHeader				= PKTHDR_NetPlayerGold;
					l_NetPlayerGoldPacket.ObjectID				= pcUserData->iID;
					l_NetPlayerGoldPacket.iGoldIn				= pcUserData->iGoldIn;
					l_NetPlayerGoldPacket.iGoldOut				= pcUserData->iGoldOut;
					l_NetPlayerGoldPacket.iInventoryGold		= pcUserData->GetGold();
					l_NetPlayerGoldPacket.iSaveGold				= pcUserData->iSaveGold;
					l_NetPlayerGoldPacket.sCharacterData_iGold	= pcUserData->sCharacterData.iGold;

					l_LastHash		= pcUser->LastGoldHash;
					l_CurrentHash	= hash_data((char*)&l_NetPlayerGoldPacket, sizeof(PacketNetPlayerGold));

					if (l_LastHash != l_CurrentHash)
					{
						DEBUG( "Gold hash is different. Syncing game server(s)" );

						NETSERVER->SendPacketGameServers(&l_NetPlayerGoldPacket);
						pcUser->LastGoldHash = l_CurrentHash;
					}
				}

			}

			if( (i % 512) == (iWheel % 512) )
			{
				//Every 512 (8 seconds)
				DWORD dwDisconnectTime;

				//Old Disconnect Time
				dwDisconnectTime = pcUserData->dwDisconnectTime;

				if( dwDisconnectTime != 0 )
				{
					if ( TICKCOUNT > dwDisconnectTime && pcUserData->pcSocketData )
					{
						SocketData * sd = pcUserData->pcSocketData;
						if ( (sd->bKeepAlive == FALSE) && (pcUser->bNetServer == FALSE) )
						{
							pcUserData->dwDisconnectTime = 0;
							LOGSERVER->LogSocketClose( sd, "UserServer::Update()::OldDisconnectTime", _ReturnAddress() );
							WNDPROC_SOCKETCLOSE( SOCKETSERVER->GetHWND(), sd );
						}
					}
				}

				//Lag Grace
				DWORD dwTimeLastPacketReceived = 0;

				if ( pcUserData && pcUserData->pcSocketData )
					dwTimeLastPacketReceived = pcUserData->pcSocketData->dwTimeLastPacketReceived;

				if( dwTimeLastPacketReceived != 0 )
				{
					SocketData * sd = pcUserData->pcSocketData;

					if( sd && (sd->bKeepAlive == FALSE) && (pcUser->bNetServer == FALSE) )
					{
						DWORD dwLag = TICKCOUNT - dwTimeLastPacketReceived;

						if (dwLag > (5 * 60 * 1000))
						{
							LOGSERVER->LogSocketClose( sd, "UserServer::Update()::LagGrace", _ReturnAddress() );
							WNDPROC_SOCKETCLOSE( SOCKETSERVER->GetHWND(), sd );
						}
					}
				}
			}
		}
	}

	iWheel++;
}

//This never gets called. werid.
DWORD WINAPI UserServer::UsersUpdate()
{
	return TRUE;
}


void UserServer::Loop()
{
	//JLM - look inside - very interesting!
	//HALLOWEENHANDLER->Tick();
	//UpdateServerTime(); - doesn't do anything?


	BOTSERVER->Tick();
	NETSERVER->Tick();

	if ( LOGIN_SERVER )
		return;

	for ( UINT i = 0, j = uUsersInGame; i < j; i++ )
	{
		User * pcUser = pcaUserInGame[i];
		UserData * pcUserData = pcUser->pcUserData;

		//Unit (monster/NPC) status - rate set by UNIT_STATUS_UPDATE_DIVISOR.
		//Hoisted out of the b64 (1Hz) "deep status" tier so monster HP bars
		//track damage closely instead of lagging up to a second behind.
		if ( pcUser->bUnitStatus )
		{
			if ( ( TICKCOUNT - pcUserData->dwTimeLastPacket ) < USER_STATUS_UPDATE_GRACE )
			{
				//Send Unit Status of other Monsters to this User
				LoopUnits( pcUser );
			}
		}

		//8 times per second
		if ( pcUser->b8 )
		{
			DWORD dwTimeDifference = TICKCOUNT - pcUserData->dwTimeLastPacket;

			if ( dwTimeDifference < USER_STATUS_UPDATE_GRACE )
			{
				SendDamageInfoAndClearBuffer ( pcUser );

				//0.5s interval
				if ( pcUser->b32 )
				{
					//1 second interval
					if ( pcUser->b64 )
					{
						DAMAGEHANDLER->UserTick1s( pcUser );

						if ( GAME_SERVER )
						{
							DWORD l_GameTime = GetTickCount();

							PacketTransCommand l_Packet;
							l_Packet.iHeader = PKTHDR_GameTimeSync;
							l_Packet.iLength = sizeof( PacketTransCommand );
							l_Packet.LParam = l_GameTime;

							PACKETSERVER->Send( pcUserData, &l_Packet );
						}
					}

					//Send Unit Status of other Users to this User
					LoopUsers ( pcUser );
				}
			}
		}


		if ( pcUser->bTenSeconds )
		{
			pcUser->bTenSeconds = FALSE;

			if ( GAME_SERVER )
			{
				ITEMSERVER->SyncPremiumTimersToClient( pcUser ); //sync with client every 10 seconds

				if ( CHARACTERSERVER->CharacterInTown( pcUser->pcUserData ) == FALSE &&
					 pcUser->uSkillBuffHash != 0)
				{
					USERSERVER->SendSkillBuffStatusToAllUsersinRange( pcUser );
				}
			}
		}


		//every 1 minute - but not neccesarily at time for all users
		if (pcUser->bOneMin)
		{
			pcUser->bOneMin = FALSE;


			if ( LOGIN_SERVER )
			{
				//user is muted, check expiry
				if ( pcUser->bMuted )
				{
					SYSTEMTIME	sCurrentTime;
					GetLocalTime( &sCurrentTime );
					DWORD dwSystemTime = SystemTimeToDWORD( &sCurrentTime );

					//is mute expired?
					if ( dwSystemTime >= pcUser->dwUnMuteExpiryTime )
					{
						pcUser->bMuted = FALSE;
						pcUser->dwUnMuteExpiryTime = 0;

						ACCOUNTSERVER->SQLUnmuteAccountId( pcUser->iAccountID );

						CHATSERVER->SendChat( pcUser, CHATCOLOR_Error, "> Your mute has expired. You can now speak freely!" );

						USERSERVER->SyncUserMuteStatus( pcUser );
					}
				}
			}
			else
			{
				if (CHARACTERSERVER->CharacterInTownEx( pcUser->pcUserData ) == FALSE )
					ITEMSERVER->SyncPremiumTimersToDatabase( pcUser ); //sync with database every 10s

				QUESTSERVER->RemoveExpiredQuestFinishData( pcUser );

				//update user effect every minute
				//note this needs to be done on game server
				CHARACTERSERVER->SyncUserEffectToUser( pcUser );
			}
		}
	}
}

void UserServer::LoopUnits( User * pcUser )
{ //0x0055A590
	UserData * pcUserData = pcUser->pcUserData;

	//Alive Unit array
	UINT uUnitsInGame = UNITSERVER->uUnitsInGame;
	Unit ** pcaUnitInGame = UNITSERVER->pcaUnitInGame;
	ECharacterType charType = ECharacterType::CHARACTERTYPE_None;


	//Receive Unit Status
	for ( UINT i = 0, j = uUnitsInGame; i < j; i++ )
	{
		Unit * pcUnit = pcaUnitInGame[i];
		UnitData * pcUnitData = pcUnit->pcUnitData;

		charType = pcUnitData->sCharacterData.iType;

		if ( pcUnitData->uUnitInfoSize != 0 )
		{
			if ( pcUser->CanSee( pcUnit ) )
			{
				int iDistanceXZ, iDistanceXZY, iDistanceX, iDistanceZ, iDistanceY;
				pcUserData->sPosition.GetPTDistanceXZY( &pcUnitData->sPosition, iDistanceXZY, iDistanceXZ, iDistanceX, iDistanceZ, iDistanceY);

				BOOL bWithinBasicDistance = iDistanceXZ < DISTANCE_MAX_PLAYER_BASIC_VIEW && (iDistanceX < DISTANCE_MISC) && (iDistanceZ < DISTANCE_MISC) && (iDistanceY < DISTANCE_MISC_Y);
				BOOL bWithinDetailedDistance = bWithinBasicDistance && iDistanceXZ < DISTANCE_MAX_PLAYER_DETAILED_VIEW;

				BOOL bForceUnitStatus = FALSE;

				//force char status
				if (charType == CHARACTERTYPE_Player &&
					pcUserData->iMapID != MAPID_BlessCastle &&
					pcUserData->iMapID == pcUnitData->pcMap->pcBaseMap->iMapID)
				{
					bForceUnitStatus = TRUE;
				}


				//force npc status in current and second map
				if ( bForceUnitStatus == FALSE &&
					 charType == CHARACTERTYPE_NPC &&
					 (pcUser->eMapID == pcUnitData->pcMap->pcBaseMap->iMapID ||
					  pcUser->eSecondMapID == pcUnitData->pcMap->pcBaseMap->iMapID) )
				{
					bForceUnitStatus = TRUE;
				}

				//force boss status in current and second map
				if (bForceUnitStatus == FALSE &&
					pcUnitData->sCharacterData.sMonsterClass == EMonsterClass::Boss &&
					(pcUser->eMapID == pcUnitData->pcMap->pcBaseMap->iMapID ||
					 pcUser->eSecondMapID == pcUnitData->pcMap->pcBaseMap->iMapID))
				{
					bForceUnitStatus = TRUE;
				}

				//Force show Wolverins in current and second map
				if (bForceUnitStatus == FALSE &&
					pcUnitData->sCharacterData.iMonsterEffectID == MONSTEREFFECTID_WOLVERLIN &&
					MAPSERVER->IsNight() &&
					(pcUser->eMapID == pcUnitData->pcMap->pcBaseMap->iMapID ||
					 pcUser->eSecondMapID == pcUnitData->pcMap->pcBaseMap->iMapID) )
				{
					bForceUnitStatus = TRUE;
				}

				if (bForceUnitStatus == FALSE &&
					pcUnitData->DontMoveFlag != 0)
				{
					switch (pcUnitData->sCharacterData.iMonsterEffectID)
					{
					case MONSTEREFFECTID_CASTLE_DOOR:
					case MONSTEREFFECTID_CASTLE_TOWER_B:
						bForceUnitStatus = TRUE;
						break;
					}
				}

				if ( bWithinDetailedDistance )
				{
					if ( pcUserData->uaUpdateCounter5[pcUnit->uIndex] != pcUnit->uLastUpdate )
					{
						SendUnitStatusM( pcUser, (Packet *)(pcUnit->baUnitBufferNew) );

						if (pcUnitData->GetAnimation() != EAnimationType::ANIMATIONTYPE_Die )
							pcUnitData->iAliveCounter = 256;

						pcUserData->uaUpdateCounter5[pcUnit->uIndex] = pcUnit->uLastUpdate;
					}
				}
				else if (bForceUnitStatus || bWithinBasicDistance)
				{
					BOOL bSendBasicStatsUpdate = FALSE;

					if ( bWithinBasicDistance )
					{
						bSendBasicStatsUpdate = pcUserData->uaUpdateCounter5[pcUnit->uIndex] != pcUnit->uLastUpdate && ( pcUnit->uLastUpdate % 2 == 0 );
					}
					else if (charType == ECharacterType::CHARACTERTYPE_NPC)
					{
						bSendBasicStatsUpdate = pcUserData->uaUpdateCounter5[pcUnit->uIndex] != pcUnit->uLastUpdate && ( pcUnit->uLastUpdate % 8 == 0 );
					}
					else
					{
						bSendBasicStatsUpdate = pcUserData->uaUpdateCounter5[pcUnit->uIndex] != pcUnit->uLastUpdate && ( pcUnit->uLastUpdate % 4 == 0 );
					}

					if (bSendBasicStatsUpdate || pcUserData->uaUpdateCounter5[pcUnit->uIndex] == 0)
					{
						if (pcUnitData->GetAnimation() != ANIMATIONTYPE_Die)
						{
							PacketUnitStatusMove sMovePacket{};
							sMovePacket.iLength = sizeof(sMovePacket);
							sMovePacket.iHeader = PKTHDR_UnitStatusMove;
							sMovePacket.lID = pcUnitData->iID;
							sMovePacket.sPosition = pcUnitData->sPosition;
							sMovePacket.sAngleY = pcUnitData->sAngle.iY;
							sMovePacket.sMapID = pcUnit->GetMapID();
							sMovePacket.sAnimationType = pcUnitData->GetAnimation();
							sMovePacket.iFrame = pcUnitData->iFrame;
							sMovePacket.dwHashCharacterName = pcUnitData->sUnitInfo.dwHashCharacterName;

							SendUnitStatusM(pcUser, &sMovePacket);

							pcUnitData->iAliveCounter = 256;
						}

						pcUserData->uaUpdateCounter5[pcUnit->uIndex] = pcUnit->uLastUpdate;
					}
				}
			}
			else
			{
				pcUserData->uaUpdateCounter5[pcUnit->uIndex] = 0;
			}
		}
	}

	//Game Status Babel Storm Map
	if ( (pcUser->GetMapID() == (*MapServer::piBabelStormMapID)) && ((*MapServer::pdwBabelStormEndTime) > TICKCOUNT) )
	{
		PacketGameStatusBabelStormMap sPacket;
		sPacket.iLength = sizeof( sPacket );
		sPacket.iHeader = PKTHDR_GameStatus;
		sPacket.iStatusID = 0x200;
		sPacket.lID = pcUser->GetID();
		sPacket.eBabelStormMapID = (*MapServer::piBabelStormMapID);
		SendUnitStatusM( pcUser, &sPacket, FALSE );
	}

	//Game Status Bless Castle
	if ( pcUser->GetMapID() == MAPID_BlessCastle )
	{
		PacketGameStatusBlessCastle sPacket;
		sPacket.iLength = sizeof( sPacket );
		sPacket.iHeader = PKTHDR_GameStatus;
		sPacket.iStatusID = 0x210;
		sPacket.lID = pcUser->GetID();
		sPacket.dwNum1 = ((*(DWORD*)0x07AB1EF0) & 0xFFFF) | (*((DWORD*)0x07AB1EE8) << 16);
		sPacket.iClanID1 = *(DWORD *)0x07AB1EEC;
		sPacket.iClanID2 = BLESSCASTLESERVER->GetClanTop( 2 );
		sPacket.iClanID3 = BLESSCASTLESERVER->GetClanTop( 3 );
		sPacket.dwNum3 = *(DWORD*)0x07AB2054;
		sPacket.dwNum4 = *(DWORD*)0x07AB1F1C;
		sPacket.sConRate = pcUserData->sBlessCastle_Damage;
		sPacket.sBattlePoints = pcUserData->sBlessCastle_Kills;
		sPacket.dwNum6 = 0;
		SendUnitStatusM( pcUser, &sPacket, FALSE );

		pcUserData->sBlessCastle_Damage = 0;
		pcUserData->sBlessCastle_Kills = 0;
	}

	//Game Status PvP Map
	EMapID ePvPMapID = (EMapID)(READDWORD( 0x0084831C ));
	if ( (ePvPMapID != MAPID_Invalid) && (pcUser->GetMapID() == ePvPMapID) )
	{
		PacketGameStatusPvPMap sPacket;
		sPacket.iLength = sizeof( sPacket );
		sPacket.iHeader = PKTHDR_GameStatus;
		sPacket.iStatusID = 0x220;
		sPacket.lID = pcUser->GetID();
		sPacket.ePvPMapID = ePvPMapID;
		SendUnitStatusM( pcUser, &sPacket, FALSE );
	}

	SendAndClearUnitStatusM( pcUser );
}

/// <summary>
/// This function is called every 500ms
/// </summary>
void UserServer::LoopUsers( User * pcUser )
{ //0x005501C0

	if ( pcUser == NULL || pcUser->IsValidAndInGame() == FALSE )
		return;

	UserData * pcUserData = pcUser->pcUserData;

	//Update Party
	if( pcUser->bParty && pcUser->psParty )
		PARTYHANDLER->UpdatePartyData( pcUser, pcUser->psParty );

	DAMAGEHANDLER->UpdateDamagePvPDamageBuffs(pcUser);

	//Do Chat Update?
	//BOOL bChat = pcUserData->szChat[0] != 0;

	//Do Status Update?
	BOOL bThisUserStatusIsValid = (pcUserData->dwTimeLastStatusUpdate + USER_USER_STATUS_UPDATE_GRACE) >= TICKCOUNT;

	PacketExtraUserInfo * sExtraUserInfo = new PacketExtraUserInfo ();
	PacketUserEffect * sUserEffect = new PacketUserEffect ();



	if ( bThisUserStatusIsValid )
	{
		//Fill Chat Packet
		/*PacketChatBoxMessage sPacketChat;

		if ( bChat )
		{
			sPacketChat.iHeader = PKTHDR_ChatMessage;
			sPacketChat.iChatColor = CHATCOLOR_Normal;
			sPacketChat.lID = pcUserData->iID;
			STRINGCOPY( sPacketChat.szChatBoxMessage, pcUserData->szChat );

			sPacketChat.iLength = sizeof( Packet ) + sizeof( sPacketChat.iChatColor ) + sizeof( sPacketChat.lID ) + STRLEN( sPacketChat.szChatBoxMessage ) + 1;

			pcUserData->szChat[0] = 0;
		}*/


		const int iBasicViewRange =     pcUserData->iMapID == MAPID_RicartenTown ? DISTANCE_MAX_PLAYER_BASIC_VIEW_RICT : DISTANCE_MAX_PLAYER_BASIC_VIEW;
		const int iDetailedViewRange =  pcUserData->iMapID == MAPID_RicartenTown ? DISTANCE_MAX_PLAYER_DETAILED_VIEW_RICT : DISTANCE_MAX_PLAYER_DETAILED_VIEW;

		//Send Chat Message and Receive Unit Status
		for ( UINT i = 0, j = uUsersInGame; i < j; i++ )
		{
			User * pcOtherUser = pcaUserInGame[i];

			//skip self
			if ( pcOtherUser == pcUser )
				continue;

			UserData * pcOtherUserData = pcOtherUser->pcUserData;

			//server haven't gotten any unit stats from this user yet
			if ( pcOtherUserData->uUnitStatusCounter == 0 )
				continue;

			//check if this user can see the other user
			if ( pcUser->CanSee( pcOtherUser ) == FALSE )
				continue;


			UINT uOtherIndex = pcOtherUser->uIndex;

			UINT uMyUnitStatusNumOfOther = pcUserData->uaUpdateCounter1_statusNum[uOtherIndex];
			UINT uUnitStatusNumOfOther  = pcOtherUserData->uUnitStatusCounter;

			int iDistanceXZ, iDistanceXZY, iDistanceX, iDistanceZ, iDistanceY;
			pcUserData->sPosition.GetPTDistanceXZY(&pcOtherUserData->sPosition, iDistanceXZY, iDistanceXZ, iDistanceX, iDistanceZ, iDistanceY);

			BOOL bForcePlayerMiniMapStatus = FALSE;
			if ((pcUserData->iMapID != MAPID_RicartenTown &&
					pcUserData->iMapID != MAPID_PillaiTown &&
					pcUserData->iMapID == pcOtherUserData->iMapID) ||
				(pcUserData->iMapID == MAPID_NaviskoTown && pcOtherUserData->iMapID == MAPID_Bellatra)) //show sod players in bellatra if user is in nav
			{
				//force show user stats
				// for mini map
				bForcePlayerMiniMapStatus = TRUE;
			}


			BOOL bWithinBasicDistance = iDistanceXZ < iBasicViewRange && (iDistanceX < DISTANCE_MISC) && (iDistanceZ < DISTANCE_MISC) && (iDistanceY < DISTANCE_MISC_Y);
			BOOL bWithinDetailedDistance = bWithinBasicDistance && iDistanceXZ < iDetailedViewRange;

			if ( bWithinBasicDistance || bForcePlayerMiniMapStatus )
			{
				ID lOtherID = pcOtherUserData->iID;
				int iOtherNotMovingCounter = pcOtherUserData->bNotMovingCounter;

				UINT uUnitStatusIndex = 0;
				BOOL bForceDetailedStatus = FALSE;
				BOOL bForceUpdateOtherUserInfo = FALSE;

				if ( pcUserData->uaUpdateCounter2_userID[uOtherIndex] == lOtherID )
				{
					if ( ( iOtherNotMovingCounter == 0 ) || ( uMyUnitStatusNumOfOther < uUnitStatusNumOfOther ) )
					{
						if ( ( uMyUnitStatusNumOfOther == 0 ) || ( uMyUnitStatusNumOfOther >= uUnitStatusNumOfOther ) )
						{
							pcUserData->uaUpdateCounter1_statusNum[uOtherIndex] = uUnitStatusNumOfOther;
							continue;
						}
						else
						{
							UINT uTemp = uUnitStatusNumOfOther - uMyUnitStatusNumOfOther;

							//difference is larger than 3
							if ( uTemp > 3 )
							{
								uMyUnitStatusNumOfOther = uUnitStatusNumOfOther - 3;
							}

							uUnitStatusIndex = uMyUnitStatusNumOfOther % 4; //note, this is for 0-3 only

							uMyUnitStatusNumOfOther++;
							pcUserData->uaUpdateCounter1_statusNum[uOtherIndex] = uMyUnitStatusNumOfOther;
						}
					}
					else
					{
						//this section is called every 500s when uaUpdateCounter4_CountDown is 0 for other user
						if ( pcUserData->uaUpdateCounter4_DelaySend[uOtherIndex] == 0 )
						{
							//is player standing?
							if ( pcOtherUser->uMovementStatus >= EPlayerMovementStatus::Standing_1_Sec )
							{
								//increment the idle counter for the other user
								//for this user.
								pcUserData->uaUpdateCounter3_IdleCounter[uOtherIndex]++;

								const UINT uIdleCounterOfOtherUser = pcUserData->uaUpdateCounter3_IdleCounter[uOtherIndex];

								BOOL bSkipUpdateOfOtherUser = TRUE;

								switch ( pcOtherUser->uMovementStatus )
								{
									case EPlayerMovementStatus::Standing_1_Sec:
									{
										//send every 1s
										if ( uIdleCounterOfOtherUser >= 2 )
										{
											bSkipUpdateOfOtherUser = FALSE;
											break;
										}
										break;
									}

									case EPlayerMovementStatus::Standing_4_Sec:
									{
										//send every 2s
										if ( uIdleCounterOfOtherUser >= 4 )
										{
											bSkipUpdateOfOtherUser = FALSE;
											break;
										}
									}
									break;


									case EPlayerMovementStatus::Standing_8_Sec:
									{
										//send every 4s
										if ( uIdleCounterOfOtherUser >= 8 )
										{
											bSkipUpdateOfOtherUser = FALSE;
											break;
										}
									}
									break;

									//AFK
									case EPlayerMovementStatus::AFK:
									default:
									{
										//send every 8s seconds
										if ( uIdleCounterOfOtherUser >= 16 )
										{
											bSkipUpdateOfOtherUser = FALSE;
											break;
										}
										break;
									}
								}

								//skip sending
								if ( bSkipUpdateOfOtherUser )
									continue;
							}
						}

						uUnitStatusIndex = 4; //use the movement data (index = 4)
					}

					pcUserData->uaUpdateCounter3_IdleCounter[uOtherIndex] = 0;
				}

				//different user or not set
				//force update
				else
				{
					//INFO( "%d: Different user for index: %d", TICKCOUNT, uOtherIndex );

					pcUserData->uaUpdateCounter1_statusNum[uOtherIndex] = uUnitStatusNumOfOther;
					pcUserData->uaUpdateCounter2_userID[uOtherIndex] = lOtherID;
					pcUserData->uaUpdateCounter3_IdleCounter[uOtherIndex] = 0;
					pcUserData->uaUpdateCounter4_DelaySend[uOtherIndex] = 4;		//delay send by ~2 second (not sure why)
					pcUser->uaUnitSkillHash[uOtherIndex] = 0;
					uUnitStatusIndex = 4;
					bForceDetailedStatus = TRUE;
				}

				if (pcUserData->uaUpdateCounter4_DelaySend[uOtherIndex] > 0)
					pcUserData->uaUpdateCounter4_DelaySend[uOtherIndex]--;

				//no data yet
				if ( pcOtherUserData->dwaUnitStatusSize[uUnitStatusIndex] == 0 )
					continue;

				if ( bWithinDetailedDistance || bForceDetailedStatus )
				{
					//CHATSERVER->SendDebugChat( pcUser, CHATCOLOR_Error, "> Send Unit Stats [%s] [%d]", CHARACTERSERVER->GetCharacterName( pcOtherUserData ), uUnitStatusIndex );

					SendUnitStatus( pcUser, (Packet *)( pcOtherUser->baUnitStatusNew[uUnitStatusIndex] ) );
				}

				//outside of detailed status
				else if ( bWithinBasicDistance || bForcePlayerMiniMapStatus )
				{
					PacketUnitStatusMove sMovePacket{};
					sMovePacket.iLength = sizeof( sMovePacket );
					sMovePacket.iHeader = PKTHDR_UnitStatusMove;
					sMovePacket.lID = pcOtherUserData->iID;
					sMovePacket.sPosition = pcOtherUserData->sPosition;
					sMovePacket.sAngleY = pcOtherUserData->sAngle.iY;
					sMovePacket.sMapID = pcOtherUserData->iMapID;
					sMovePacket.sAnimationType = ANIMATIONTYPE_None;
					sMovePacket.iFrame = 0;
					sMovePacket.dwHashCharacterName = 0;

					SendUnitStatus( pcUser, &sMovePacket );

					//if ( bWithinBasicDistance )
					//	CHATSERVER->SendDebugChat( pcUser, CHATCOLOR_Error, "> Basic User Status [%s] [%d]", CHARACTERSERVER->GetCharacterName( pcOtherUserData ), TICKCOUNT );
					//else
					//	CHATSERVER->SendDebugChat( pcUser, CHATCOLOR_Error, "> Minimap User Status [%s] [%d]", CHARACTERSERVER->GetCharacterName( pcOtherUserData ), TICKCOUNT );

				}

				//For other info
				//uUnitStatusIndex = 0 - while moving (every 2 seconds), and
				//uUnitStatusIndex = 4 - while idle (every 1 - 16 seconds)
				if ( bForceDetailedStatus ||
					(bWithinDetailedDistance  && (uUnitStatusIndex == 0 || uUnitStatusIndex == 4)))
				{
					ZeroMemory( sExtraUserInfo, sizeof( PacketExtraUserInfo ) );
					sExtraUserInfo->iLength = sizeof( PacketExtraUserInfo );
					sExtraUserInfo->iHeader = PKTHDR_ExtraUserData;
					sExtraUserInfo->iUniqueUserID = pcOtherUser->GetID();
					sExtraUserInfo->sBellatraSoloCrown = pcOtherUser->sBellatraSoloCrown;
					sExtraUserInfo->iForceOrbId = ( pcOtherUser->eMapID == MAPID_RicartenTown || pcOtherUser->eMapID == MAPID_PillaiTown ) ?
						EItemID::ITEMID_None : pcOtherUser->eForceOrbItemID;	//don't render the force orb in a city (causes some drop in fps)

					if ( sExtraUserInfo->sBellatraSoloCrown > 0 && BELLATRASERVER->IsSoloCrownHumorModeOn() )
					{
						sExtraUserInfo->sBellatraSoloCrown += 3; //Add 3 for humor
					}

					ZeroMemory( sUserEffect, sizeof( sUserEffect ) );
					sUserEffect->iLength = sizeof( PacketUserEffect );
					sUserEffect->iHeader = PKTHDR_Effect_User;
					sUserEffect->iUniqueUserID = pcOtherUser->GetID();
					sUserEffect->sTitleRarity = pcOtherUser->sTitleRarity;
					sUserEffect->sDialogSkinId = pcOtherUser->sDialogSkinId;
					sUserEffect->sPlayerAuraId = pcOtherUser->sClassRanking; //aura to be based on class ranking (top 3 only)
					STRINGCOPY( sUserEffect->szTitle, pcOtherUser->szTitle );

					//don't show aura in BC
					if ( pcOtherUser->GetMapID() == MAPID_BlessCastle )
					{
						sUserEffect->sPlayerAuraId = 0;
					}

					//top ranking gets '4' as a aura
					else if ( CHARACTERSERVER->GetTopRankingCharacterID() == pcOtherUser->iCharacterID )
					{
						sUserEffect->sPlayerAuraId = 4;
					}

					SendUnitStatus( pcUser, sExtraUserInfo );
					SendUnitStatus( pcUser, sUserEffect );


					//CHATSERVER->SendDebugChat( pcUser, CHATCOLOR_Error, "> Detailed User Status [%s] [%d]", pcOtherUserData->sCharacterData.szName, uUnitStatusIndex );

				}

				if ( pcUser->uaUnitSkillHash[uOtherIndex] != pcOtherUser->uSkillBuffHash )
				{
					USERSERVER->SendSkillBuffStatus( pcUser, pcOtherUser );
					pcUser->uaUnitSkillHash[uOtherIndex] = pcOtherUser->uSkillBuffHash;
				}
			}
			else
			{
				//Reset so it will refresh again next time even if same User is still logged in
				pcUserData->uaUpdateCounter2_userID[uOtherIndex] = 0;
			}
		}

		ClearUnitStatus( pcUser );
	}

	SAFE_DELETE ( sExtraUserInfo );
	SAFE_DELETE ( sUserEffect )
}



void UserServer::SendUnitStatus( User * pcUser, Packet * psPacket, BOOL bIncrementAmount )
{
	if ( psPacket->iLength > 0x1FF0 )
		return;

	psPacket->iEncKeyIndex = 0;
	psPacket->iEncrypted = 0;

	UINT uFreeSpace = 0x1FF0 - pcUser->uBufferPosition;

	if ( psPacket->iLength > uFreeSpace )
		ClearUnitStatus( pcUser );

	BYTE * buf = ((BYTE*)pcUser->sUnitStatusContainer.baBuffer) + pcUser->uBufferPosition;

	CopyMemory( buf, psPacket, psPacket->iLength );

	pcUser->uBufferPosition += psPacket->iLength;

	if ( bIncrementAmount )
		pcUser->sUnitStatusContainer.iAmount++;
}

void UserServer::ClearUnitStatus( User * pcUser )
{
	if ( (pcUser->sUnitStatusContainer.iAmount > 0) || (pcUser->uBufferPosition > 0) )
	{
		pcUser->sUnitStatusContainer.iLength = sizeof( Packet ) + sizeof( int ) + sizeof( int ) + pcUser->uBufferPosition;

		SENDPACKET( pcUser, &pcUser->sUnitStatusContainer );

		pcUser->uBufferPosition = 0;
		pcUser->sUnitStatusContainer.iAmount = 0;
	}
}

void UserServer::SendUnitStatusM( User * pcUser, Packet * psPacket, BOOL bIncrementAmount )
{
	if ( psPacket->iLength > 0x1FF0 )
		return;

	psPacket->iEncKeyIndex = 0;
	psPacket->iEncrypted = 0;

	UINT uFreeSpace = 0x1FF0 - pcUser->uBufferPositionM;

	if ( psPacket->iLength > uFreeSpace )
		SendAndClearUnitStatusM( pcUser );

	BYTE * buf = ((BYTE*)pcUser->sUnitStatusContainerM.baBuffer) + pcUser->uBufferPositionM;

	CopyMemory( buf, psPacket, psPacket->iLength );

	pcUser->uBufferPositionM += psPacket->iLength;
	if ( bIncrementAmount )
		pcUser->sUnitStatusContainerM.iAmount++;

	pcUser->uUnitStatusTime = TIME;
}


void UserServer::SendDamageInfoAndClearBuffer ( User * pcUser )
{
	if ( pcUser->sDamageInfoContainer.sAmount > 0 )
	{
		pcUser->sDamageInfoContainer.iHeader = PKTHDR_DamageInfoContainer;
		pcUser->sDamageInfoContainer.iLength = sizeof ( Packet ) + sizeof ( short ) + ( pcUser->sDamageInfoContainer.sAmount * sizeof ( PacketDebugDamageInfo ) ); //max = 340 per second. should be enough

		SENDPACKET ( pcUser, &pcUser->sDamageInfoContainer, FALSE );

		ZeroMemory ( &pcUser->sDamageInfoContainer, sizeof ( PacketDebugDamageInfoContainer ) );
		pcUser->sDamageInfoContainer.sAmount = 0;
	}
}

void UserServer::OnLeaveMap( User * pcUser, int iOldMapId )
{
	if ( pcUser == NULL || pcUser->IsValidAndInGame() == FALSE )
		return;

	PacketCommand sPacket{};
	sPacket.iLength	= sizeof( PacketCommand );
	sPacket.iHeader = PKTHDR_ClearUnitDataFromMap;
	sPacket.p1 = pcUser->GetID();

	SENDPACKETRANGE( &sPacket, &pcUser->pcUserData->sPosition, iOldMapId, DISTANCE_XY_30_meters, FALSE, pcUser->pcUserData  );
}

void UserServer::OnGetPlayerInfoData( User * pcUser, PacketSimple* psPacket)
{

}

void UserServer::SendSkillBuffStatus( User * pcUser, User * pcOtherUser )
{
	if ( pcOtherUser->sSkillBuffStatus.iHeader == 0 )
	{
		pcOtherUser->sSkillBuffStatus.iHeader = PKTHDR_SkillBuffStatus;
		pcOtherUser->sSkillBuffStatus.iLength = sizeof( PacketSkillBuffStatus );
		pcOtherUser->sSkillBuffStatus.iTargetID = pcOtherUser->GetID();
	}

	SENDPACKET( pcUser, &pcOtherUser->sSkillBuffStatus );
}

void UserServer::SendSkillBuffStatusToAllUsersinRange ( User * pcUser )
{
	if ( pcUser == FALSE || pcUser->IsValidAndInGame() == FALSE )
		return;

	pcUser->sSkillBuffStatus.iHeader = PKTHDR_SkillBuffStatus;
	pcUser->sSkillBuffStatus.iLength = sizeof( PacketSkillBuffStatus );
	pcUser->sSkillBuffStatus.iTargetID = pcUser->GetID();

	SENDPACKETRANGE( &pcUser->sSkillBuffStatus, &pcUser->pcUserData->sPosition, pcUser->GetMapID(), DISTANCE_MAX_SKILL_VISUAL, FALSE);
}


BOOL UserServer::UpdateSkillBuffHash( User * pcUser )
{
	const int maxSize = 15;

	int iaData[maxSize]{};

	iaData[0] = pcUser->sSkillBuffStatus.bConcealLevel1Based;
	iaData[1] = pcUser->sSkillBuffStatus.bDemonBaneLevel1Based;
	iaData[2] = pcUser->sSkillBuffStatus.bAlasLevel1Based;
	iaData[3] = pcUser->sSkillBuffStatus.bHolyValorLevel1Based;
	iaData[4] = pcUser->sSkillBuffStatus.bHolyBodyLevel1Based;
	iaData[5] = pcUser->sSkillBuffStatus.bDrasticSpiritLevel1Based;
	iaData[6] = pcUser->sSkillBuffStatus.bForceOfNatureLevel1Based;
	iaData[7] = pcUser->sSkillBuffStatus.bInpesLevel1Based;
	iaData[8] = pcUser->sSkillBuffStatus.bAdventMigalLevel1Based;
	iaData[9] = pcUser->sSkillBuffStatus.bHallOfValhallaLevel1Based;
	iaData[10] = pcUser->sSkillBuffStatus.bTriumphOfValhallaLevel1Based;
	iaData[11] = pcUser->sSkillBuffStatus.bVirtualLifeLevel1Based;
	iaData[12] = pcUser->sSkillBuffStatus.bVirtualRegenLevel1Based;
	iaData[13] = pcUser->sSkillBuffStatus.bVenomLevel1Based;
	iaData[14] = pcUser->sSkillBuffStatus.bElectricalOverloadLevel1Based;

	UINT skillBuffHash = static_cast<UINT>(hash_data ( (char*)iaData, maxSize * sizeof(int) ));

	if ( pcUser->uSkillBuffHash != skillBuffHash || pcUser->bForceSkillBuffUpdate )
	{
		//INFO( "Skill buff hash changed: %d --> %d", pcUser->uSkillBuffHash, skillBuffHash );
		pcUser->uSkillBuffHash = skillBuffHash;
		pcUser->bForceSkillBuffUpdate = FALSE;

		return TRUE;
	}

	return FALSE;
}

BOOL UserServer::CheckAndUpdateSkillStatus( User * pcUser, ESkillID eSkillID, time_t & uEndTime, BYTE & bLevel1Based, USHORT & sSyncTimeLeft, BYTE & bSyncLevel1Based, BOOL bForceExpiry )
{
	BOOL bActive = TRUE;

	//solo mode for bellatra
	if ( pcUser->GetMapID() == MAPID_Bellatra &&
		 pcUser->iBellatraRoomInitialPlayerCount == 1 &&
		 pcUser->pcUserData->dwExclusiveNum == 0x100 ) //0x100 = Bellatra sod event id
	{
		switch ( eSkillID )
		{
			case SKILLID_VirtualLife:
			case SKILLID_VirtualRegen:		bForceExpiry = pcUser->pcUserData->sCharacterData.iClass != ECharacterClass::CHARACTERCLASS_Priestess;
				break;

			case SKILLID_Alas:
			case SKILLID_Inpes:				bForceExpiry = pcUser->pcUserData->sCharacterData.iClass != ECharacterClass::CHARACTERCLASS_Assassin;
				break;

			case SKILLID_HolyValor:			bForceExpiry = pcUser->pcUserData->sCharacterData.iClass != ECharacterClass::CHARACTERCLASS_Knight;
				break;

			case SKILLID_AdventMigal:		bForceExpiry = pcUser->pcUserData->sCharacterData.iClass != ECharacterClass::CHARACTERCLASS_Shaman;
				break;

			case SKILLID_DemonsBane:		bForceExpiry = pcUser->pcUserData->sCharacterData.iClass != ECharacterClass::CHARACTERCLASS_Fighter;
				break;

			case SKILLID_TriumphofValhalla:
			case SKILLID_HallofValhalla:	bForceExpiry = pcUser->pcUserData->sCharacterData.iClass != ECharacterClass::CHARACTERCLASS_Atalanta;
				break;

			case SKILLID_EnchantWeapon:		bForceExpiry = pcUser->pcUserData->sCharacterData.iClass != ECharacterClass::CHARACTERCLASS_Magician;
				break;

			case SKILLID_ForceofNature:		bForceExpiry = pcUser->pcUserData->sCharacterData.iClass != ECharacterClass::CHARACTERCLASS_Archer;
				break;
		}
	}

	if ( uEndTime )
	{
		if ( TIME > uEndTime || bForceExpiry )
		{
			sSyncTimeLeft = 0;
			uEndTime = 0;
			bSyncLevel1Based = bLevel1Based = 0;

			bActive = FALSE;
		}
		else
		{
			int iSyncNewTimeLeft = static_cast<int>(uEndTime - TIME);

			if ( iSyncNewTimeLeft > sSyncTimeLeft )
			{
				//INFO( "Forcing skill buff update" );
				pcUser->bForceSkillBuffUpdate = TRUE;
			}

			sSyncTimeLeft = static_cast<USHORT>(uEndTime - TIME);
			bSyncLevel1Based = bLevel1Based;
		}
	}
	else
	{
		uEndTime = 0;
		sSyncTimeLeft = 0;
		bSyncLevel1Based = bLevel1Based = 0;
	}

	return bActive;
}

void UserServer::UpdateSkillBuffStatus( User * pcUser, ESkillID eSkillID, time_t uEndTime, BYTE bLevel1Based, int iParameter )
{
	//auto entry = pcUser->sSkillBuffStatusMap.find( eSkillID );
	//
	////not found, add new
	//if ( entry == pcUser->sSkillBuffStatusMap.end() )
	//{
	//	if ( uEndTime == 0 || TIME > uEndTime )
	//		return;
	//
	//	SkillBuffStatus status;
	//	status.eSkillID = eSkillID;
	//	status.sTimeLeftSec = uEndTime;
	//	status.bSkillLevel1Based = bLevel1Based;
	//	status.iParameter = iParameter;
	//
	//	pcUser->sSkillBuffStatusMap.insert( { eSkillID, status } );
	//}
	//else
	//{
	//	//skill expired or invalid? remove
	//	if ( uEndTime == 0 || TIME > uEndTime )
	//	{
	//		DEBUG( "Skill %d expired ", eSkillID );
	//
	//		pcUser->sSkillBuffStatusMap.erase( eSkillID );
	//		return;
	//	}
	//
	//	//update parameters
	//	entry->second.eSkillID = eSkillID;
	//	entry->second.sTimeLeftSec = TIME - uEndTime;
	//	entry->second.bSkillLevel1Based = bLevel1Based;
	//	entry->second.iParameter = iParameter;
	//}
}


void UserServer::SendAndClearUnitStatusM( User * pcUser )
{
	if ( (pcUser->sUnitStatusContainerM.iAmount > 0) || (pcUser->uBufferPositionM > 0) )
	{
		pcUser->sUnitStatusContainerM.iLength = sizeof( Packet ) + sizeof( int ) + sizeof( int ) + pcUser->uBufferPositionM;

		//INFO( "Send %d status data, packet len: %d", pcUser->sUnitStatusContainerM.iAmount, pcUser->sUnitStatusContainerM.iLength );

		SENDPACKET( pcUser, &pcUser->sUnitStatusContainerM );

		pcUser->uUnitStatusTime = 0;
		pcUser->uBufferPositionM = 0;
		pcUser->sUnitStatusContainerM.iAmount = 0;
	}
}

/// <summary>
/// Sync mute status and expiry between game server and login server
/// </summary>
void UserServer::SyncUserMuteStatus( User * pcUser )
{
	if ( pcUser == NULL || pcUser->pcUserData == NULL )
		return;

	PacketNetMuteSync packet = {};
	packet.iLength = sizeof( PacketNetMuteSync );
	packet.iHeader = PKTHDR_NetMuteSync;
	packet.dwObjectID = pcUser->pcUserData->iID;
	packet.bMuted = pcUser->bMuted;
	packet.dwUnMuteExpiryTime = pcUser->dwUnMuteExpiryTime;

	if ( LOGIN_SERVER )
	{
		NETSERVER->SendPacketGameServers( &packet );
	}
	else
	{
		NETSERVER->SendPacketToLoginServer( &packet );
	}
}

/// <summary>
/// Send server message from MessageLogin table
/// </summary>
/// <param name="pcUser"></param>
void UserServer::SendServerMessages ( User * pcUser )
{
	char szMessageLogin[128] = { 0 };

	SQLConnection * pcDB = SQLCONNECTION ( DATABASEID_ServerDB, 23 );

	if ( pcDB->Open () )
	{
		if ( pcDB->Prepare ( "SELECT Message FROM MessageLogin" ) )
		{
			if ( pcDB->Execute () )
			{
				while ( pcDB->Fetch () )
				{
					pcDB->GetData ( 1, PARAMTYPE_String, szMessageLogin, 128 );
					CHATSERVER->SendChat ( pcUser, CHATCOLOR_Global, szMessageLogin );
				}
			}
		}
		pcDB->Close ();
	}
}


void UserServer::UpdateCharacterIDFromSQL( User * pcUser )
{
	//DEBUGSTATUS( "UpdateCharacterIDFromSQL" );

	if ( pcUser && pcUser->pcUserData )
	{
		pcUser->iCharacterID = CHARACTERSERVER->SQLGetCharacterID(
			SQLCONNECTION ( DATABASEID_UserDB_LocalServer_CharInfo ),
			CHARACTERSERVER->GetCharacterName( pcUser ) );
	}
}

int UserServer::GetNewUserID()
{
	return CALL(0x0054CEE0);
}
