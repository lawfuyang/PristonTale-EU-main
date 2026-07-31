#include "stdafx.h"
#include "unitserver.h"
#include "unitdataserver.h"
#include "Logger.h"

int iUpdateMaxPacket = 80;
int iUpdateMaxUnitMask = 2048;
int iUpdateFrameCounter = 160;



UnitServer::UnitServer()
{
	pcaUnitInGame	= new Unit*[MAX_UNITS];
	pcaUnit			= new Unit[MAX_UNITS];
	pcaUnitData		= new UnitData[MAX_UNITS];

	WRITEDWORD( 0x07AC3E78, pcaUnitData );

	for ( UINT i = 0; i < MAX_UNITS; i++ )
	{
		pcaUnit[i].uIndex = i;
		pcaUnit[i].pcUnitData = pcaUnitData + i;
	}
}

/// <summary>
/// smCHAR *srFindCharFromSerial(DWORD dwObjectSerial)
/// </summary>
UnitData * UnitServer::GetUnit( UINT uEnemyID )
{
	typedef UnitData*( __cdecl *t_FindUnitEnemybyID )( UINT uID );
	t_FindUnitEnemybyID FindUnitEnemybyID = ( t_FindUnitEnemybyID )0x0054CC60;
	return FindUnitEnemybyID( uEnemyID );
}

/// <summary>
/// smCHAR *srFindCharFromSerial(DWORD dwObjectSerial)
/// </summary>
UnitData * UnitServer::GetNPCUnit( UINT uNPCID, int iMapID )
{
	for ( UINT i = 0; i < MAX_UNITS; i++ )
	{
		Unit * pcUnit = pcaUnit + i;
		UnitData * pcUnitData = pcUnit->pcUnitData;

		if ( pcUnitData->bActive )
		{
			if ( pcUnitData->sCharacterData.iType == ECharacterType::CHARACTERTYPE_NPC &&
				 (iMapID == -1 || pcUnitData->sCharacterData.sMapID == iMapID) &&
				 pcUnitData->sCharacterData.iNPCId == uNPCID)
			{
				return pcUnitData;
			}
		}
	}

	return NULL;
}


NAKED UnitData * UnitServer::UnitDataByIDMap( int iID, int iMapID )
{
	JMP( pfnUnitDataByIDMap );
}

Unit * UnitServer::UnitDataToUnit( UnitData * pcUnitData )
{
	if ( pcUnitData == NULL )
		return NULL;

	if ( pcUnitData->pcUnit && pcUnitData->pcUnit->iID == pcUnitData->iID )
		return pcUnitData->pcUnit;

	Unit * pcUnit = pcaUnit + (pcUnitData - pcaUnitData);
	if ( pcUnit->iID != pcUnitData->iID )
		pcUnit->Load();

	return pcUnit;
}

CharacterData * UnitServer::GetCharacterDataByEffect( EMonsterEffectID dwEffect, EMonsterClass iClass, int iMonsterLevel )
{
	CharacterData * psCharInfo = *(CharacterData**)0x07AB3050;
	for ( UINT i = 0; i < UNITINFODATA->GetTotalUnitEnemy (); i++ )
	{
		if ( psCharInfo[i].iMonsterEffectID == dwEffect && ( iMonsterLevel == 0 ||  psCharInfo[i].iLevel == iMonsterLevel) )
		{
			if ( iClass != EMonsterClass::Normal )
			{
				if ( psCharInfo[i].sMonsterClass == iClass )
				{
					return &psCharInfo[i];
				}
			}
			else
				return &psCharInfo[i];
		}
	}
	return NULL;
}

CharacterData * UnitServer::GetCharacterDataByName( const char * pszName )
{
	CharacterData * psCharInfo = *(CharacterData**)0x07AB3050;
	for ( UINT i = 0; i < UNITINFODATA->GetTotalUnitEnemy (); i++ )
	{
		if ( STRINGCOMPARE( psCharInfo[i].szName, pszName ) )
		{
			return &psCharInfo[i];
		}
	}
	return NULL;
}

UnitInfo * UnitServer::GetUnitInfoByName( const char * pszName )
{
	UnitInfo * psaUnitInfo = (*(UnitInfo**)0x07AB22A4);

	for ( UINT i = 0; i < UNITINFODATA->GetTotalUnitEnemy (); i++ )
	{
		if ( STRINGCOMPARE( psaUnitInfo[i].szName, pszName ) )
			return &psaUnitInfo[i];
	}

	return NULL;
}

int UnitServer::GetMonsterHealthPointDefinition( int iMonsterId )
{
	auto it = mapMonsterToHP.find( iMonsterId );

	if ( it == mapMonsterToHP.end() )
	{
		LOGERROR( "Monster HP not found for %d", iMonsterId );
		return 30000;
	}

	return it->second;
}

void UnitServer::AddOrUpdateMonsterHealthPointDefinition( int iMonsterId, int iHealthValue )
{
	auto it = mapMonsterToHP.find( iMonsterId );
	if ( it == mapMonsterToHP.end() )
	{
		mapMonsterToHP.insert( { iMonsterId, iHealthValue } );
	}
	else
	{
		it->second = iHealthValue;
	}
}


int UnitServer::GetArrayPosition( UnitData * pcUnitData )
{
	UINT uaUnits = *(DWORD*)0x7AC3E78;
	return ((UINT)pcUnitData - uaUnits) / 0x4B3C;
}

INT64 UnitServer::GetExpLevelDiference( int iLevel1, int iLevel2, INT64 iExp )
{
	int iLevel = abs( iLevel1 - iLevel2 );

	if ( iLevel < 10 )
		iExp = iExp;
	else if ( iLevel < 15 )
		iExp = ( iExp * 75 ) / 100;
	else if ( iLevel < 20 )
		iExp = ( iExp * 50 ) / 100;
	else
		iExp = ( iExp * 25 ) / 100;

	return iExp;
}

INT64 UnitServer::GetExpLevelDiferenceVsPlayer( int iLevel1, int iLevel2, INT64 iExp )
{
	int iLevel = abs( iLevel1 - iLevel2 );

	if ( iLevel < 15 )
		iExp = iExp;
	else if ( iLevel < 20 )
		iExp = ( iExp * 75 ) / 100;
	else if ( iLevel < 25 )
		iExp = ( iExp * 50 ) / 100;
	else
		iExp = ( iExp * 25 ) / 100;

	return iExp;
}

INT64 UnitServer::GetExp( UnitData * pcUnitData )
{
	if ( pcUnitData )
		return ( ( INT64 )pcUnitData->sCharacterData.iCurrentExpHigh << 32 ) | ( ( INT64 )pcUnitData->sCharacterData.iCurrentExpLow & 0xFFFFFFFF );
	return 0;
}

BOOL UnitServer::SetExp( UnitData * pcUnitData, INT64 iNewExp )
{
	//-ve is not acceptable mate!
	if ( iNewExp < 0 )
		return FALSE;

	if ( pcUnitData == NULL )
		return FALSE;

	if ( RATE_EXP > 0 )
		iNewExp = iNewExp * RATE_EXP;

	pcUnitData->sUnitInfo.iExp = (int)iNewExp;

	UnitInfo * pcUnitInfo = UNITINFODATA->GetUnitInfoByMonsterID( pcUnitData->sCharacterData.iUniqueMonsterID );
	if ( pcUnitInfo )
	{
		pcUnitInfo->iExp = (int)iNewExp;
	}

	typedef void(__cdecl* t_SetExpUnitFunc)(CharacterData* pMonster, INT64 iExp);
	t_SetExpUnitFunc SetExpUnitFunc = (t_SetExpUnitFunc)0x08B815CC;
	SetExpUnitFunc(&pcUnitData->sCharacterData, iNewExp);

	CharacterData * pcCharacterData = UNITINFODATA->GetCharacterDataByMonsterID( pcUnitData->sCharacterData.iUniqueMonsterID );
	if ( pcCharacterData )
	{
		typedef void(__cdecl* t_SetExpUnitFunc)(CharacterData* pMonster, INT64 iExp);
		t_SetExpUnitFunc SetExpUnitFunc = (t_SetExpUnitFunc)0x08B815CC;
		SetExpUnitFunc(pcCharacterData, iNewExp);
	}

	return TRUE;
}

BOOL UnitServer::SetMotionFromCode( UnitData * pcUnitData, int iMotionCode )
{
	typedef int( __thiscall *t_SetMotionFromCodePT )( UnitData * pUnit, int iCode );
	t_SetMotionFromCodePT SetMotionFromCodePT = ( t_SetMotionFromCodePT )0x00416E30;

	if ( pcUnitData )
	{
		//INFO("UnitServer::SetMotionFromCode for %s = %d", pcUnitData->sCharacterData.szName, iMotionCode);

		if (iMotionCode == ANIMATIONTYPE_Die)
		{
			pcUnitData->SetCurrentHealth( 0 );
			//INFO("Setting sHP.SCurr to 0");
		}


		return SetMotionFromCodePT( pcUnitData, iMotionCode );
	}
	return FALSE;
}

void UnitServer::SendBossMessage( BaseMap * pcBaseMap, UINT pUnit )
{
	CHATSERVER->SendChatAllUsersInStage( pcBaseMap->GetID(), CHATCOLOR_Global, "Boss> %s Spawned!", ( char * )*( UINT* )( pUnit ) );
}

void UnitServer::HandleNPCClick( User * pcUser, PacketNPCClick * pPacket )
{
	UnitData * l_UnitTarget = UNITSERVER->GetUnit ( pPacket->dwTarObjectSerial );
	if ( l_UnitTarget == NULL )
	{
		WARN( "HandleNPCClick failed. NPC (%d) doesn't exist for id: %d", pPacket->iNPCId, pPacket->dwTarObjectSerial );
		return;
	}

	if ( l_UnitTarget->sCharacterData.iNPCId != pPacket->iNPCId )
	{
		WARN( "HandleNPCClick failed. NPC ID mismatch" );
		return;
	}

	OnUnitNpcClick( pcUser, l_UnitTarget );
}

/// <summary>
/// Triggered on npc clicked by player
/// Note - OnUnitNpcClick only get called via GAME_SERVER
/// </summary>
BOOL UnitServer::OnUnitNpcClick( User * pcUser, UnitData * pcUnitData )
{
	//Only for game server
	if ( LOGIN_SERVER )
		return FALSE;

	if ( pcUser == NULL || pcUser->IsValidAndInGame() == FALSE )
	{
		WARN( "OnUnitNpcClick pcUser is null" );
		return FALSE;
	}

	if ( pcUser->dwIgnoreNpcClickTime != 0 &&
		TICKCOUNT < pcUser->dwIgnoreNpcClickTime )
	{
		WARN( "Ignoring NPC click spam" );
		return FALSE;
	}
	pcUser->dwIgnoreNpcClickTime = TICKCOUNT + 1000;


	if (pcUnitData->szMessage[0] && ( !pcUnitData->sUnitInfo.EventNPC || pcUnitData->sUnitInfo.EventNPC == 5 ) ) //5 = SOD Accountant
	{
		PacketChatMessage	TransChatMessage;
		ZeroMemory( &TransChatMessage, sizeof( PacketChatMessage ) );

		//Send reply message
		if (pcUnitData->sUnitInfo.iNumMessage > 1 )
		{
			//Multiple messages to choose from
			TransChatMessage.iHeader = PKTHDR_ChatMessage;
			TransChatMessage.dwIP = 0;
			TransChatMessage.dwObjectSerial = pcUnitData->iID;
			lstrcpy ( TransChatMessage.szMessage,
				pcUnitData->sUnitInfo.pszaMessage[Dice::RandomI(0, pcUnitData->sUnitInfo.iNumMessage - 1)] ); //pick a random one

			TransChatMessage.iLength = 0x8 + 0x4 + 0x4 + lstrlen ( TransChatMessage.szMessage );

			SENDPACKET ( pcUser, (char *)&TransChatMessage );
		}
		else
		{
			//Single message
			TransChatMessage.iHeader = PKTHDR_ChatMessage;
			TransChatMessage.iLength = 0x8 + 0x4 + 0x4 + lstrlen (pcUnitData->szMessage );
			TransChatMessage.dwIP = 0;
			TransChatMessage.dwObjectSerial = pcUnitData->iID;
			lstrcpy ( TransChatMessage.szMessage, pcUnitData->szMessage );

			SENDPACKET ( pcUser, (char *)&TransChatMessage );
		}
	}

	BOOL bNPCHandled = TRUE;

	// Is only Game Master?
	if ( pcUnitData->sUnitInfo.bGMOnly &&
		pcUser->GetGameLevel() != GAMELEVEL_Four)
	{
		CHATSERVER->SendDebugChat( pcUser, CHATCOLOR_Global, "> Only for Admins!" );
		return FALSE;
	}

	// Is Warehouse Pet?
	if ( pcUnitData->sUnitInfo.WareHouseMasterNPC && pcUnitData->lExclusiveTargetID &&
			pcUser->GetGameLevel() <= GAMELEVEL_One &&
			pcUser->GetID() != pcUnitData->lExclusiveTargetID )
	{
		DEBUG ( "Warehouse pet" );

		return FALSE;
	}

	//Event girl
	else if ( pcUnitData->sUnitInfo.iEventGirl )
	{
		DEBUG ( "pcUnitData->sUnitInfo.iEventGirl" );

		PacketEventGirl s;
		s.iLength = sizeof ( PacketEventGirl );
		s.iHeader = PKTHDR_OpenEventGirl;
		s.bFreeGold = EVENT_FREEEVENTGIRL;
		SENDPACKET ( pcUser, &s );
	}

	//Item aging NPC
	else if ( pcUnitData->sUnitInfo.ItemAgingNPC )
	{
		DEBUG ( "pcUnitData->sUnitInfo.ItemAgingNPC" );

		PacketAgingEvent s;
		s.iLength = sizeof ( PacketAgingEvent );
		s.iHeader = PKTHDR_OpenAgingMaster;
		s.bFreeAging = EVENT_AGINGFREE;
		s.bNoBreak = EVENT_AGING_NOBREAK;
		s.bHalfPrice = EVENT_AGING_HALFPRICE;
		SENDPACKET ( pcUser, &s );
	}

	//Shop NPC
	else if ( pcUnitData->sUnitInfo.SellAttackItemCount ||
				pcUnitData->sUnitInfo.SellDefenceItemCount ||
				pcUnitData->sUnitInfo.SellEtcItemCount )
	{
		DEBUG ( "pcUnitData->sUnitInfo.Sell" );

		//Send store item list to client
		ITEMSERVER->SendItemShopList(pcUser, pcUnitData);
	}

	//Skill Master NPC
	else if ( pcUnitData->sUnitInfo.SkillMasterNPC )
	{
		DEBUG ( "pcUnitData->sUnitInfo.SkillMasterNPC" );

		CALL_WITH_ARG3 ( (DWORD)ENPCFunction::SEND_SKILL_MENU, (DWORD)pcUser->pcUserData->pcSocketData, (DWORD)pcUnitData, pcUnitData->sUnitInfo.SkillChangeJobNPC );
	}

	//Item storage NPC
	else if ( pcUnitData->sUnitInfo.WareHouseMasterNPC )
	{
		DEBUG ( "pcUnitData->sUnitInfo.WareHouseMasterNPC" );
		NETSERVER->SendCommandToLoginServer(PacketsHeader::PKTHDR_NetOpenWarehouse, pcUser->GetID());
	}

	//Item combination NPC
	else if ( pcUnitData->sUnitInfo.ItemMixNPC )
	{
		DEBUG ( "pcUnitData->sUnitInfo.ItemMixNPC (%d)", pcUnitData->sUnitInfo.ItemMixNPC );
		SENDPACKETCOMMAND(pcUser, PKTHDR_OpenMixItem, NULL, pcUnitData->sUnitInfo.ItemMixNPC);
	}

	//Item distributor
	else if ( pcUnitData->sUnitInfo.iItemBox )
	{
		DEBUG ( "pcUnitData->sUnitInfo.iItemBox" );

		NETSERVER->SendCommandToLoginServer( PacketsHeader::PKTHDR_NetOpenItemDistributor, pcUser->GetID());
	}

	//Wings quest NPC
	else if ( pcUnitData->sUnitInfo.WingQuestNpc)
	{
		DEBUG ( "pcUnitData->sUnitInfo.WingQuestNpc" );

		SENDPACKETCOMMAND ( pcUser, PKTHDR_TeleportEvent, 0, pcUnitData->sUnitInfo.WingQuestNpc );
	}

	else if (pcUnitData->sCharacterData.iNPCId == NPCID_SAILOR_SPARROW)
	{
		DEBUG("Clicked on neverSink NPC");

		SENDPACKETBLANK(pcUser, PKTHDR_NeverSinkOpenUI, TRUE);
	}

	//Teleport NPC
	else if ( pcUnitData->sUnitInfo.TeleportID )
	{
		DEBUG ( "pcUnitData->sUnitInfo.TeleportID (%d)", pcUnitData->sUnitInfo.TeleportID );

		SENDPACKETCOMMAND ( pcUser, PKTHDR_TeleportEvent, pcUnitData->sUnitInfo.TeleportID );
	}

	//Give money NPC (what is this?) could be worth setting up /test_this_npc command
	else if ( pcUnitData->sUnitInfo.GiveMoneyNpc )
	{
		DEBUG ( "pcUnitData->sUnitInfo.GiveMoneyNpc" );

		SENDPACKETBLANK ( pcUser, PKTHDR_OpenGiveMoney, TRUE );
	}

	//No idea what this is?? could be worth setting up /test_this_npc command
	else if ( pcUnitData->sUnitInfo.StarPointNPC && pcUser->GetCharacterLevel() >= pcUnitData->sUnitInfo.StarPointNPC )
	{
		DEBUG ( "pcUnitData->sUnitInfo.StarPointNPC" );

		SENDPACKETBLANK ( pcUser, PKTHDR_OpenStarPoint, TRUE );
	}

	else if ( pcUnitData->sUnitInfo.EventNPC )
	{
		DEBUG ( "pcUnitData->sUnitInfo.EventNPC %d", pcUnitData->sUnitInfo.EventNPC );

		//SOD Accountant
		if ( pcUnitData->sUnitInfo.EventNPC == 5 )
		{
			SENDPACKETCOMMAND ( pcUser, PKTHDR_OpenCollect, 0, 5 );
		}

		//SOD Admission Assistant
		else
		{
			CALL_WITH_ARG3 ( (DWORD)ENPCFunction::SEND_OPEN_EVENT, (DWORD)pcUnitData, (DWORD)pcUser->pcUserData, (DWORD)pcUnitData->sUnitInfo.EventNPC );
		}

		//if idle, walking, running, sprint or falling
		if ( pcUnitData->psModelAnimation->iType < EAnimationType::ANIMATIONTYPE_Attack )
		{
			pcUnitData->Animate ( EAnimationType::ANIMATIONTYPE_Attack );
		}
	}

	//Clan NPC
	else if ( pcUnitData->sUnitInfo.iClanController )
	{
		DEBUG ( "pcUnitData->sUnitInfo.iClanController" );

		SENDPACKETBLANK ( pcUser, PKTHDR_OpenClanMenu );
	}

	//Blessed Castle Info NPC
	else if ( pcUnitData->sUnitInfo.BlessCastleNPC )
	{
		DEBUG ( "pcUnitData->sUnitInfo.BlessCastleNPC" );

		CALL_WITH_ARG2 ( (DWORD)ENPCFunction::SEND_OPEN_BLESSCASTLE_INFO, (DWORD)pcUser->pcUserData, (DWORD)pcUnitData->sUnitInfo.BlessCastleNPC );
	}

	//Polling NPC
	else if ( pcUnitData->sUnitInfo.PollingNpc )
	{
		DEBUG ( "pcUnitData->sUnitInfo.PollingNpc" );

		SENDPACKETCOMMAND ( pcUser, PKTHDR_PublicPolling, 0, pcUnitData->sUnitInfo.PollingNpc );
	}

	// Item distributor
	else if (pcUnitData->sUnitInfo.iItemBox)
	{
		if (pcUser->GetGameLevel() >= GAMELEVEL_One)
		{
			CHATSERVER->SendChat( pcUser, EChatColor::CHATCOLOR_White, " > Event mode is active! Premium items taken out can be dropped. To disable, use /deactivategm" );
		}
	}

	else if ( pcUnitData->sUnitInfo.SellAttackItemCount ||
				pcUnitData->sUnitInfo.SellDefenceItemCount ||
				pcUnitData->sUnitInfo.SellEtcItemCount )
	{
		ITEMSERVER->SendItemShopList( pcUser, pcUnitData );
	}

	else if ( pcUnitData->sUnitInfo.NPCEventCode > 0 )
	{
		DEBUG ( "pcUnitData->sUnitInfo.NPCEventCode %d", pcUnitData->sUnitInfo.NPCEventCode );

		switch ( pcUnitData->sUnitInfo.NPCEventCode )
		{
			case NPCEVENTCODE_Manufacture:				SENDPACKETBLANK ( pcUser, PKTHDR_OpenManufacture, TRUE );			break;
			case NPCEVENTCODE_Smelting:					SENDPACKETBLANK ( pcUser, PKTHDR_OpenSmelting, TRUE );				break;
			case NPCEVENTCODE_IhinEvent:				SENDPACKETBLANK ( pcUser, PKTHDR_NPCIhinRewardRequest, TRUE );		break;
			case NPCEVENTCODE_EasterNPC:				SENDPACKETBLANK ( pcUser, PKTHDR_NPCEasterEgg, TRUE );				break;
			case NPCEVENTCODE_ResetItem:				SENDPACKETBLANK ( pcUser, PKTHDR_OpenResetItem, TRUE );				break;
			case NPCEVENTCODE_CoinShop:					SENDPACKETBLANK ( pcUser, PKTHDR_OpenCoinShop, TRUE );				break;
			case NPCEVENTCODE_LarryQuestsNPC:			SENDPACKETBLANK ( pcUser, PKTHDR_NPCLarryOpen, TRUE );				break;
			case NPCEVENTCODE_MarinaQuestsNPC:			SENDPACKETBLANK ( pcUser, PKTHDR_NPCMarinaOpen, TRUE );				break;
			case NPCEVENTCODE_RudolphChristmasNPC:		SENDPACKETBLANK ( pcUser, PKTHDR_NPCRudolphChristmas, TRUE );		break;
			case NPCEVENTCODE_MixGuild:					MIXHANDLER->SendPacketMixList ( pcUser );							break;
			case NPCEVENTCODE_RankList:					RANKINGLISTHANDLER->SendRankingData ( pcUser, 0 );				break;
			case NPCEVENTCODE_SoloRankList:				RANKINGLISTHANDLER->SendBellatraSoloRankingData ( pcUser, 0 );	break;
			case NPCEVENTCODE_Warehouse:				NETSERVER->SendCommandToLoginServer ( PacketsHeader::PKTHDR_NetOpenWarehouse,	      pcUser->GetID() );	break;
			case NPCEVENTCODE_WarehouseSeasonal:		NETSERVER->SendCommandToLoginServer ( PacketsHeader::PKTHDR_NetOpenWarehouseSeasonal, pcUser->GetID() );	break;
			case NPCEVENTCODE_FuryArenaNPCTeleportCT2:	FURYARENAHANDLER->OnFuryArenaTeleporterNPCClicked ( pcUser );		break;
			case NPCEVENTCODE_FuryArenaNPCEnter:		FURYARENAHANDLER->OnFuryArenaNPCClicked ( pcUser );					break;
			case NPCEVENTCODE_SocketSystemNPCDrill:
			case NPCEVENTCODE_SocketSystemNPCStone:		MIXHANDLER->OnSocketSystemNPCClicked ( pcUser, pcUnitData );		break;
			default:
				WARN ( "Unknown NPCEventCode = %d", pcUnitData->sUnitInfo.NPCEventCode );
				break;
		}
	}
	else
	{
		bNPCHandled = FALSE;
	}


	if ( QUESTSERVER->OnClickNPC( pcUser, pcUnitData ) )
	{
		bNPCHandled = TRUE;
	}

	//NPCs that gives a quest window (repeatables)
	else if ( QUESTSERVER->OnClickDynamicNPC ( pcUser, pcUnitData ) )
	{
		bNPCHandled = TRUE;
	}



	//if (bNPCHandled == FALSE )
	//	WARN ( "NPC not handled = %s", pcUnitData->GetName() );

	return TRUE;
}

void UnitServer::OnSendExp( UserData * pcUserData, UnitData * pcUnitData, int iKillerUserDataId )
{
	User * pcUser = USERDATATOUSER( pcUserData );

	if ( pcUserData && pcUnitData && pcUser )
	{
		//if user is invisible in mini map,
		//disable any exp gains so that
		//players using bots cannot hide themselves
		if (pcUser->bInvisibleInMiniMap)
			return;

		// First all, notify Quest Handler
		if ( FURYARENAHANDLER->OnUnitKilled( pcUserData, pcUnitData, iKillerUserDataId ) )
			return;
		if ( HALLOWEENHANDLER->OnUnitKilled( pcUnitData ) )
			return;
		if ( QUESTSERVER->OnUnitKilled( pcUser, UNITDATATOUNIT( pcUnitData ) ) )
			return;

		INT64 iExp = GetExp( pcUnitData );

		if ( pcUserData->iMapID != MAPID_Bellatra )
			iExp += (iExp * (*(int*)0x0084601C)) / 100;

		//EXP Party
		if( (pcUser->bParty && pcUser->psParty && pcUser->psParty->pcLeader) && pcUser->psParty->iMembersCount > 1 )
		{
			PartyInfo * psParty = pcUser->psParty;

			int iPercent = 0;

			//Percentage to EXP Mode, Normal Mode and Hunt Mode
			/*if( psParty->iMode == PARTYMODE_NORMAL )
				iPercent = 180 + (80 * (psParty->iMembersCount - 2));*/
			if( psParty->iMode == PARTYMODE_Normal )
				iPercent = 180 + (80 * (psParty->iMembersCount - 2));
			else if( psParty->iMode == PARTYMODE_Hunt )
				iPercent = 80 + (20 * (psParty->iMembersCount - 2));

			iExp = ((iExp * iPercent) / 100) / psParty->iMembersCount;


			//Get Level Average (weighted)
			int iAvgPartyLevelWeighed = PARTYHANDLER->GetWeighedAveragePartyLevel( pcUser );

			//check and reduce exp if party weighed mean is higher than the monster's level
			//if ( iAvgPartyLevelWeighed > pcUnitData->sCharacterData.iLevel )
			//	iExp = GetExpLevelDiference( iAvgPartyLevelWeighed, pcUnitData->sCharacterData.iLevel, iExp );

			//Send EXP to Users
			for( size_t i = 0; i < MAX_PARTY_MEMBERS; i++ )
			{
				User * p = i == 0 ? psParty->pcLeader : psParty->pcaMembers[i-1];
				if(p && p->pcUserData && p->pcUserData->pcSocketData)
				{
					int iX = (p->pcUserData->sPosition.iX - pcUserData->sPosition.iX) >> 8;
					int iZ = (p->pcUserData->sPosition.iZ - pcUserData->sPosition.iZ) >> 8;
					int iDist = iX*iX + iZ*iZ;

					INT64 iGain = 0;

					//get adjusted experience based on this party member vs party weighted average level
					INT64 iExpMember = GetExpLevelDiferenceVsPlayer( iAvgPartyLevelWeighed, p->pcUserData->sCharacterData.iLevel, iExp );

					//EXP Pot
					if( p->iEXPPotion > 0 )
						iGain = (iExpMember * (INT64)p->iEXPPotion) / 100;

					//Is near distance?
					if( iDist < DISTANCE_MAX_PARTY && p->GetMapID() == pcUserData->iMapID )
					{
						CHARACTERSERVER->GiveEXP( p, iExpMember + iGain );

						if( p->iEXPPotion > 0 )
							CHATSERVER->SendDebugChat( p, CHATCOLOR_Error, "> ExpUp +%d%%", p->iEXPPotion );

						CHATSERVER->SendChatEx( p, CHATCOLOR_Notice, "> Gained %s experience [%d%% / %d] in the party!", FormatNumber(iExp + iGain), iPercent, psParty->iMembersCount );
					}
				}
			}

			//Boss kill for quests are handled elsewhere
			if (pcUnitData->sCharacterData.sMonsterClass != EMonsterClass::Boss)
			{
				//Raid & Party Kill
				if (psParty->iRaidCount > 0)
				{
					for (int i = 0; i < MAX_RAID; i++)
					{
						PartyInfo* psP = i == 0 ? psParty->pcRaidLeader : psParty->pcaRaid[i - 1];
						if ( psP )
						{
							for ( size_t j = 0; j < MAX_PARTY_MEMBERS; j++ )
							{
								User * p = j == 0 ? psP->pcLeader : psP->pcaMembers[j - 1];
								if ( p && p->pcUserData && p->pcUserData->pcSocketData )
								{
									int iX = ( p->pcUserData->sPosition.iX - pcUserData->sPosition.iX ) >> 8;
									int iZ = ( p->pcUserData->sPosition.iZ - pcUserData->sPosition.iZ ) >> 8;
									int iDist = iX * iX + iZ * iZ;

									//Is near distance?
									if ( iDist < DISTANCE_MAX_PARTY && p->GetMapID () == pcUserData->iMapID )
									{
										ProcessPacketKillUnitID ( p, pcUnitData->iID, iKillerUserDataId );
									}
								}
							}
						}
					}
				}
				else
				{
					for (size_t i = 0; i < MAX_PARTY_MEMBERS; i++)
					{
						User* p = i == 0 ? psParty->pcLeader : psParty->pcaMembers[i - 1];
						if (p && p->pcUserData && p->pcUserData->pcSocketData)
						{
							int iX = (p->pcUserData->sPosition.iX - pcUserData->sPosition.iX) >> 8;
							int iZ = (p->pcUserData->sPosition.iZ - pcUserData->sPosition.iZ) >> 8;
							int iDist = iX * iX + iZ * iZ;

							//Is near distance?
							if (iDist < DISTANCE_MAX_PARTY && p->GetMapID() == pcUserData->iMapID)
							{
								ProcessPacketKillUnitID(p, pcUnitData->iID, iKillerUserDataId);
							}
						}
					}
				}
			}
		}
		else
		{
			//adjust exp for solo player based on level diff vs monster, and only
			//if the player level is higher than the monster's level
			//if (pcUser->pcUserData->GetLevel() > pcUnitData->sCharacterData.iLevel )
			//	iExp = GetExpLevelDiference( pcUser->pcUserData->GetLevel(), pcUnitData->sCharacterData.iLevel, iExp );

			// Premium Exp pot?
			if ( pcUser->iEXPPotion > 0 )
			{
				INT64 iGain = (iExp * (INT64)pcUser->iEXPPotion) / 100;
				iExp += iGain;
			}

			//Quest Increase EXP
			iExp += QUESTSERVER->GetQuestEXPOnKill( pcUser, pcUnitData, iExp );

			CHARACTERSERVER->GiveEXP( USERDATATOUSER( pcUserData ), iExp );

			if ( pcUser->iEXPPotion > 0 )
				CHATSERVER->SendDebugChat( pcUser, CHATCOLOR_Error, "> ExpUp +%d%%", pcUser->iEXPPotion );

			//Boss kill for quests are handled elsewhere
			if (pcUnitData->sCharacterData.sMonsterClass != EMonsterClass::Boss)
			{
				ProcessPacketKillUnitID(pcUser, pcUnitData->iID, iKillerUserDataId );
			}

			CHATSERVER->SendChatEx( USERDATATOUSER( pcUserData ), CHATCOLOR_Notice, "> Gained %s experience!", FormatNumber( iExp ) );
		}
	}
}

void UnitServer::OnSetDrop( UserData * pcUserData, UnitData * pcUnitData )
{
	User * pcUser = USERDATATOUSER( pcUserData );

	if( pcUserData && pcUnitData && pcUser )
	{
		//Premium Third Eye?
		ItemPremium * psPremium = ITEMSERVER->GetPremiumUse( pcUser, ITEMTIMERTYPE_ThirdEye );
		if( psPremium && psPremium->dwTimeLeft )
		{
			int iDropCount = pcUnitData->sUnitInfo.iNumDrops;
			pcUnitData->sUnitInfo.iNumDrops++;
			CHATSERVER->SendDebugChat( pcUser, CHATCOLOR_Error, "> ThirdEye +1 Drop [ %d -> %d ]", iDropCount, pcUnitData->sUnitInfo.iNumDrops );
		}

		//Quest Reward Item Sixth Sense
		ItemPremium* pssPremium = ITEMSERVER->GetPremiumUse(pcUser, ITEMTIMERTYPE_SixthSense);
		if (pssPremium && pssPremium->dwTimeLeft)
		{
			if (Dice::RandomI(0, 99) < 25) //25% chance
			{
				int iDropCount = pcUnitData->sUnitInfo.iNumDrops;
				pcUnitData->sUnitInfo.iNumDrops++;

				CHATSERVER->SendDebugChat(pcUser, CHATCOLOR_Error, "> SixthSense +1 Drop [ %d -> %d ]", iDropCount, pcUnitData->sUnitInfo.iNumDrops);
			}
		}

		//Server Wide Drop Buff
		ItemPremium* pssdbPremium = ITEMSERVER->GetPremiumUse(pcUser, ITEMTIMERTYPE_SDropBuff);
		if (pssdbPremium && pssdbPremium->dwTimeLeft)
		{
			if (Dice::RandomI(0, 99) < 15) //15% chance
			{
				int iDropCount = pcUnitData->sUnitInfo.iNumDrops;
				pcUnitData->sUnitInfo.iNumDrops++;

				CHATSERVER->SendDebugChat(pcUser, CHATCOLOR_Error, "> Server Wide Drop Buff +1 Drop [ %d -> %d ]", iDropCount, pcUnitData->sUnitInfo.iNumDrops);
			}
		}

		//Is in Party and is in hunt mode?
		if( (pcUser->bParty && pcUser->psParty) &&
			(pcUser->psParty->iMembersCount > 1) &&
			(pcUser->psParty->iMode == PARTYMODE_Hunt) )
		{
			int iDropCount = pcUnitData->sUnitInfo.iNumDrops;
			pcUnitData->sUnitInfo.iNumDrops += 1;

			CHATSERVER->SendDebugChat( pcUser, CHATCOLOR_Error, "> Party Drop [ %d -> %d ]", iDropCount, pcUnitData->sUnitInfo.iNumDrops );
		}

		//Quest Reduce Drop
		if ( pcUnitData->sUnitInfo.iNumDrops > 1 )
		{
			pcUnitData->sUnitInfo.iNumDrops -= QUESTSERVER->GetReduceQuestDrop ( pcUser, pcUnitData );
		}

		//Extra drops from event
		if ( EVENT_EXTRADROPS > 0 )
		{
			int iDropCount = pcUnitData->sUnitInfo.iNumDrops;
			pcUnitData->sUnitInfo.iNumDrops += EVENT_EXTRADROPS;
			CHATSERVER->SendDebugChat( pcUser, CHATCOLOR_Error, "> Extra Drop [ %d -> %d ]", iDropCount, pcUnitData->sUnitInfo.iNumDrops );
		}
	}
}

void UnitServer::OnSetTargetPosition( UserData * pcUserData, UnitData * pcUnitData )
{
	if ( pcUserData && pcUnitData )
	{
		//DEBUG("UnitServer::OnSetTargetPosition");

		if ( FURYARENAHANDLER->InsideArena( &pcUnitData->sPosition ) )
		{
			FURYARENAHANDLER->IsBoss( pcUnitData );
			return;
		}

		Unit * pcUnit = UNITDATATOUNIT( pcUnitData );

		if ( pcUnitData->iID != pcUnit->iID )
		{
			pcUnit->iID						= pcUnitData->iID;
			pcUnit->dwTimePositionUpdate	= TICKCOUNT;
		}

		int iX = (pcUnit->sLastRunningPosition.iX - pcUnitData->sPosition.iX) >> 8;
		int iZ = (pcUnit->sLastRunningPosition.iZ - pcUnitData->sPosition.iZ) >> 8;
		int iDistance = (iX*iX + iZ*iZ);

		int iMotionCode = pcUnitData->psModelAnimation ? (*(UINT*)pcUnitData->psModelAnimation) : 0;

		if ( (abs( iX ) > 4 || abs( iZ ) > 4) ||
		   ( iMotionCode != EAnimationType::ANIMATIONTYPE_Idle &&
			 iMotionCode != EAnimationType::ANIMATIONTYPE_Walking &&
			 iMotionCode != EAnimationType::ANIMATIONTYPE_Running) )
		{
			pcUnit->dwTimePositionUpdate = TICKCOUNT;
		}

		pcUnit->sLastRunningPosition.iX = pcUnitData->sPosition.iX;
		pcUnit->sLastRunningPosition.iZ = pcUnitData->sPosition.iZ;

		pcUnitData->NextTarget(pcUserData->sPosition.iX, pcUserData->sPosition.iY, pcUserData->sPosition.iZ);

		if ( pcUnitData->DontMoveFlag == FALSE && pcUnitData->iDamageLastAttacker && (pcUnitData->dwDamageLastTime + 2000) > TICKCOUNT )
		{
			if ( pcUnitData->GetCurrentHealth() > 0 &&
				 pcUnitData->sCharacterData.iMonsterType != MONSTERTYPE_Summon &&
				 (pcUnit->dwTimePositionUpdate + 10000) < TICKCOUNT &&
				 (iMotionCode == ANIMATIONTYPE_Walking || iMotionCode == ANIMATIONTYPE_Running || iMotionCode == ANIMATIONTYPE_Idle) )
			{

				int iY = abs( ((int)CALLT_WITH_ARG2( 0x004694D0, (DWORD)pcUnitData->pcStage, pcUserData->sPosition.iX, pcUserData->sPosition.iZ ) >> 8) - (pcUserData->sPosition.iY >> 8));

				if ( iY <= 120 )
				{
					pcUnitData->sPosition.iX = pcUserData->sPosition.iX;
					pcUnitData->sPosition.iY = pcUserData->sPosition.iY;
					pcUnitData->sPosition.iZ = pcUserData->sPosition.iZ;

					pcUnitData->NextTarget( 0, 0, 0 );
					pcUnitData->SetAngleByPosition( pcUserData->sPosition.iX >> 8, pcUserData->sPosition.iZ >> 8 );
					pcUnitData->Animate( ANIMATIONTYPE_Idle );

					//DEBUG("iY <= 120, ANIMATIONTYPE_Idle");
				}
				else
				{
					pcUnitData->sPosition.iX = pcUnitData->sSpawnPosition.iX;
					pcUnitData->sPosition.iZ = pcUnitData->sSpawnPosition.iZ;
					pcUnitData->sPosition.iY = CALLT_WITH_ARG2( 0x004694D0, (DWORD)pcUnitData->pcStage, pcUnitData->sPosition.iX, pcUnitData->sPosition.iZ );

					pcUnitData->NextTarget( 0, 0, 0 );
					pcUnitData->SetAngleByPosition( pcUserData->sPosition.iX >> 8, pcUserData->sPosition.iZ >> 8 );
					pcUnitData->Animate( ANIMATIONTYPE_Idle );

					//DEBUG("iY > 120, ANIMATIONTYPE_Idle");
				}

				pcUnit->dwTimePositionUpdate = TICKCOUNT;
			}
		}
	}
}

UnitData * UnitServer::CreateUnitEnemy( char * pszName, int iX, int iY, int iZ, UserData * pcUserData )
{
	typedef UnitData *(__cdecl *t_CreateUnitEnemy)( char * pszName, int iX, int iY, int iZ, UserData * pcUserData );
	t_CreateUnitEnemy CreateUnitEnemyPT = (t_CreateUnitEnemy)0x0055EBC0;
	return CreateUnitEnemyPT( pszName, iX, iY, iZ, pcUserData );
}

/// <summary>
/// Called whenever a monster is added to a map
/// Call via
/// smCHAR *STG_AREA::OpenMonster(STG_CHAR_INFO *lpStgCharInfo, int gGroup)
/// smCHAR *rsOpenSubMonster(smCHAR *lpMainChar, DWORD dwCharSoundCode, int x, int y, int z, int MaxFlag)
/// And for night / day transitions
/// </summary>
/// <param name="pcMap"></param>
/// <param name="pcUnitData"></param>
void UnitServer::OnMonsterAddedToMap( Map * pcMap, UnitData * pcUnitData )
{
	pcUnitData->bTestMapUnit = FALSE;

	auto it = mapMonsterToHP.find( pcUnitData->sCharacterData.iUniqueMonsterID );
	if ( it == mapMonsterToHP.end() )
	{
		WARN( "OnMonsterOpened - Monster id to HP mapping not found for id: %d, name: %s", pcUnitData->sCharacterData.iUniqueMonsterID, pcUnitData->GetName() );
		return;
	}

	pcUnitData->InitHealthPoint(it->second);



	//DEBUG( "OnMonsterAddedToMap - pcUnitData %s HP = %d, block = %d-%d, hp = %d-%d", pcUnitData->GetName(), pcUnitData->GetMaxHealth(),
	//	pcUnitData->sHPBlocks.sCur, pcUnitData->sHPBlocks.sMax,
	//	pcUnitData->sCharacterData.sHP.sCur, pcUnitData->sCharacterData.sHP.sMax );
}

/// <summary>
/// This code gets called each time on creation of unit
/// and this code is used to set the HP multiplier
/// </summary>
BOOL UnitServer::IsMultiplyHPMonter( UnitData * pcUnitData )
{
	return FALSE;



	//DEBUG( "IsMultiplyHPMonter: %s", pcUnitData->GetName() );

	//BOOL bMultiply = FALSE;

	//// Monsters from effect ID
	//switch ( pcUnitData->sCharacterData.iMonsterEffectID )
	//{
	//	case MONSTEREFFECTID_DEATHKNIGHT:
	//	case MONSTEREFFECTID_KELVEZU:
	//	case MONSTEREFFECTID_MOKOVA:
	//	case MONSTEREFFECTID_TULLA:
	//	case MONSTEREFFECTID_DevilShy:
	//	case MONSTEREFFECTID_Yagditha:
	//	case MONSTEREFFECTID_Draxos:
	//	case MONSTEREFFECTID_CASTLE_TOWER_B:
	//	case MONSTEREFFECTID_CASTLE_DOOR:
	//	case MONSTEREFFECTID_CASTLE_CRYSTAL_R:
	//	case MONSTEREFFECTID_CASTLE_CRYSTAL_G:
	//	case MONSTEREFFECTID_CASTLE_CRYSTAL_B:
	//	case MONSTEREFFECTID_CASTLE_CRYSTAL_N:
	//	case MONSTEREFFECTID_Greedy:
	//	case MONSTEREFFECTID_BEEVIL:
	//	case MONSTEREFFECTID_S_F_ELEMENTAL:
	//
	//		bMultiply = TRUE;
	//		break;
	//	default:
	//		break;
	//}



	//For Glow = 1, use Potion for HP block multipler
	//if (pcUnitData->sCharacterData.sMonsterClass == EMonsterClass::Boss)
	//	bMultiply = TRUE;
	//
	//
	//if (pcUnitData->pcMap->pcBaseMap->iLevel >= 108)
	//	bMultiply = TRUE;
	//
	//
	//// Multiply HP?
	//if ( bMultiply )
	//{
	//	pcUnitData->iHPBlocks					= pcUnitData->sUnitInfo.iMaxPotions;
	//	pcUnitData->sHPBlocks.sCur				= (short)pcUnitData->iHPBlocks;
	//	pcUnitData->sHPBlocks.sMax				= (short)pcUnitData->iHPBlocks;
	//	pcUnitData->sVirtualHP.sCur				= (short)pcUnitData->iHPBlocks;
	//	pcUnitData->sVirtualHP.sMax				= (short)pcUnitData->iHPBlocks;
	//}
	//else
	//{
	//	pcUnitData->iHPBlocks					= 0;
	//	pcUnitData->sHPBlocks.sCur				= 0;
	//	pcUnitData->sHPBlocks.sMax				= 0;
	//	pcUnitData->sVirtualHP.sCur				= 0;
	//	pcUnitData->sVirtualHP.sMax				= 0;
	//
	//	// Set potion percent, maybe pot anim
	//	if ( pcUnitData->sUnitInfo.iMaxPotions )
	//	{
	//		if (Dice::RandomI(1, 100) < pcUnitData->sUnitInfo.iPerPotions )
	//		{
	//			pcUnitData->sUnitInfo.iMaxPotions = Dice::RandomI(1, pcUnitData->sUnitInfo.iMaxPotions);
	//		}
	//		else
	//		{
	//			pcUnitData->sUnitInfo.iMaxPotions = 0;
	//		}
	//	}
	//}
	//return pcUnitData->iHPBlocks > 0;
}

NAKED BOOL UnitServer::GetTop10Damage( UnitData * pcUnitData, AttackDamageData * psaDamageData )
{
	JMP( pfnGetTop10DamageUnitData );
}

BOOL UnitServer::HandleKill( UnitData * pcUnitData, UserData * pcUserData, int iKillerUserDataId )
{
	DEBUG( "Handle unit kill: %s (%d), killer: %s", pcUnitData->GetName(), pcUnitData->iID, CHARACTERSERVER->GetCharacterName( pcUserData ) );

	User * pcUser = USERDATATOUSER( pcUserData );

	if (pcUser == NULL || pcUser->IsValidAndInGame() == FALSE)
		return FALSE;

	//Quest
	if ( pcUnitData->sUnitInfo.iQuestItemID )
	{
		if ( (BOOL)CALL_WITH_ARG3 ( 0x005500D0, pcUnitData->sUnitInfo.iQuestItemID, (DWORD)pcUserData, (DWORD)pcUnitData ) == TRUE )
		{
			ITEMSERVER->CreateItem1 ( pcUser, pcUnitData->sUnitInfo.iQuestItemID, EItemSource::QuestKill, 1 );
		}
	}

	BOOL bIsBoss = pcUnitData->sCharacterData.sMonsterClass == EMonsterClass::Boss;


	//Extra Drops
	for( int i = 0; i < pcUnitData->sUnitInfo.iDropPlusCount; i++ )
	{
		if(Dice::RandomI( 0, 9999 ) < pcUnitData->sUnitInfo.saDropDataCustom[i].iPercent )
		{
			LootItem sLootItem;
			EItemID eItemID = (EItemID)pcUnitData->sUnitInfo.saDropDataCustom[i].iItemID;
			DefinitionItem * pItem = ITEMSERVER->FindItemDefByCode( eItemID );

			if( pItem && (pItem->sItem.iItemUniqueID == FALSE))
			{
				ITEMSERVER->CreateItem( &sLootItem.sItem, pItem, bIsBoss ? EItemSource::BossKill : EItemSource::MonsterKill );

				if ( (sLootItem.sItem.sItemID.ToItemType() == ITEMTYPE_Quest) || (sLootItem.sItem.sItemID.ToItemType() == ITEMTYPE_MorionT2Quest) )
				{
					ITEMSERVER->SendItemData( pcUserData, &sLootItem.sItem );
					break;
				}

				if( pcUnitData->PartyFlag != 0x100 || (pItem->sItem.sItemID.ToItemType() != ITEMTYPE_GoldAndExp && pItem->sItem.sItemID.ToItemBase() != ITEMBASE_Potion) )
				{
					Map * pcMap = (Map*)pcUnitData->pcMap;
					if( pcMap )
					{
						Loot * psItemD = MAPSERVER->AddItemV2(pcMap, &sLootItem, pcUnitData->sPosition.iX >> 8, pcUnitData->sPosition.iY >> 8, pcUnitData->sPosition.iZ >> 8, ELootType::Initial);
						if( psItemD )
						{
							if ( pcUnitData->sUnitInfo.bPublicDrop )
								MAPSERVER->SendItemStageNearUsers( pcMap, psItemD );
							else
								MAPSERVER->SendItemStageUser( pcMap, pcUser, psItemD );

							psItemD->dwDropTime += 5000; //for other players
							break;
						}
					}
				}
			}
		}
	}

	//Set the quantity of drops
	OnSetDrop( pcUserData, pcUnitData );

	//Normal Drops
	int iDropIndex = 1;

	Map * pcMap = (Map *)pcUnitData->pcMap;
	if ( pcMap == NULL )
		return FALSE;

	BOOL bQuestItemSent = FALSE;

	if ( pcMap->pcBaseMap->iMapID != MAPID_Bellatra)
	{
		for (int i = 0; i < pcUnitData->sUnitInfo.iNumDrops; i++)
		{
			LootItem sItemLoot;
			ZeroMemory( &sItemLoot, sizeof(LootItem) );

			if (LOOTSERVER->GetRandomItemForMonster(pcUnitData, pcUser, &sItemLoot.sItem))
			{
				if ( ( sItemLoot.sItem.sItemID.ToItemType() == ITEMTYPE_Quest ) || ( sItemLoot.sItem.sItemID.ToItemType() == ITEMTYPE_MorionT2Quest ) )
				{
					DEBUGSTATUS( "Sending quest item: %s", sItemLoot.sItem.szItemName );
					ITEMSERVER->SendItemData( pcUserData, &sItemLoot.sItem, TRUE );
					bQuestItemSent = TRUE;
					continue;
				}

				sItemLoot.bUse = TRUE;

				//Log drops from Wanted Moriph / Wolf event
				if (EVENT_WANTEDMORIPH_EVENT || EVENT_WANTEDWOLF_EVENT)
				{
					if (pcUnitData->sCharacterData.iMonsterEffectID == MONSTEREFFECTID_NPC_MORIF ||
						pcUnitData->sCharacterData.iMonsterEffectID == MONSTEREFFECTID_NPC_MOLLYWOLF)
					{
						EVENTSERVER->LogEventMobDropItem(pcUnitData, pcUserData, &sItemLoot.sItem);
					}
				}

				Loot* psItemD = MAPSERVER->AddItemV2(pcMap, &sItemLoot, pcUnitData->sPosition.iX >> 8, pcUnitData->sPosition.iY >> 8, pcUnitData->sPosition.iZ >> 8, ELootType::Initial);
				if (psItemD)
				{
					psItemD->dwDropTime += 5000;

					if (pcUnitData->sUnitInfo.bPublicDrop)
					{
						if (ROLLDICEHANDLER->OnExchangeItemDrop(pcUnitData, pcMap, psItemD, iDropIndex++) == FALSE)
						{
							MAPSERVER->SendItemStageNearUsers(pcMap, psItemD);
						}
					}
					else
					{
						MAPSERVER->SendItemStageUser(pcMap, pcUser, psItemD);
					}
				}
			}
		}


		if ( !bQuestItemSent )
		{
			LOOTSERVER->SendQuestDropItemToUser( pcUnitData, pcUser );
		}
	}

	//Send EXP from Monster
	OnSendExp( pcUserData, pcUnitData, iKillerUserDataId );

	if (pcUnitData->sCharacterData.sMonsterClass == EMonsterClass::Boss &&
		pcMap->pcBaseMap->iMapID != MAPID_Bellatra)
	{
		HandleBossQuestKill(pcUnitData, iKillerUserDataId);
	}

	//Update Monsters killed by User
	pcUserData->uUnitKillCounter++;

	return TRUE;
}

//For handling boss kill for quest
void UnitServer::HandleBossQuestKill(UnitData* pcUnitData, int iKillerUserDataId)
{
	//std::cout << "HandleBossQuestKill" << std::endl;

	Unit * pcBossUnit = UNITDATATOUNIT(pcUnitData);

	if (pcBossUnit)
	{
		DEBUG ("");
		DEBUG ("------------------------------------------------------------------------------------------------");
		DEBUG ("BOSS: %s, Hitters: %d", pcBossUnit->GetName(), pcBossUnit->mapLastDamageReceived.size());

		if (pcUnitData->psaDamageUsersData)
		{
#ifdef  DEV_MODE
			for (int i = 0; i < ATTACK_DAMAGE_MAX; i++)
			{
				AttackDamageData* p = pcUnitData->psaDamageUsersData + i;

				if (p && p->iID)
				{
					DEBUG("BOSS: %s: Damage user id = %d, total damage = %d", pcBossUnit->GetName(), p->iID, p->iDamage);
				}
			}
#endif
		}
		else
		{
			WARN("BOSS: %s: psaDamageUsersData is NULL!");
		}

		std::map<DWORD, BOOL> processedUserIds;
		DWORD dwThisUserId = 0;
		DWORD dwThisUserLastHitTickCount = 0;

		for (std::pair<DWORD, DWORD> kvp : pcBossUnit->mapLastDamageReceived)
		{
			dwThisUserId = kvp.first;
			dwThisUserLastHitTickCount = kvp.second;

			//party or raid hitter already processed
			if (dwThisUserLastHitTickCount == 0)
				continue;

			//check if user is already processed (kill handed out)
			if (processedUserIds.find(dwThisUserId) != processedUserIds.end())
				continue;

			UserData* pcUserData = USERDATABYID(dwThisUserId);
			User* pcUser = USERDATATOUSER(pcUserData);

			if (pcUser && pcUserData && pcUserData->pcSocketData)
			{
				long timeDiff = TICKCOUNT - kvp.second;

				DEBUG ("------------------");
				DEBUG ("Processing user: %s, id: %d, time diff = %d", CHARACTERSERVER->GetCharacterName(pcUserData), pcUserData->iID, timeDiff);

				//If time diff is greater than 10 seconds then
				//the damage doesn't count for this player
				//there is a delay of 1.7s due to death animation
				if (timeDiff < 0 || timeDiff > (12 * 1000))
				{
					DEBUG (" - Skipping user. Time is greater than 12000s. (%d)", timeDiff);
					continue;
				}

				//std::cout << "Time diff : " << timeDiff << std::endl;


				//If not in same map as boss, skip
				if (pcUserData->iMapID != pcUnitData->pcMap->pcBaseMap->iMapID)
				{
					DEBUG (" - Skipping user. Not in same map as boss. (%d vs %d)", pcUserData->iMapID, pcUnitData->pcMap->pcBaseMap->iMapID);

					kvp.second = 0;
					continue;
				}

				int iX = (pcUnitData->sPosition.iX - pcUserData->sPosition.iX) >> 8;
				int iZ = (pcUnitData->sPosition.iZ - pcUserData->sPosition.iZ) >> 8;
				int iDist = iX * iX + iZ * iZ;

				//Is this user near the boss? If not, skip
				//but he might get the boss kill via another party memeber
				if (iDist > DISTANCE_MAX_BOSS)
				{
					DEBUG (" - Skipping user. Unit too far away from boss (%d vs %d)", iDist, DISTANCE_MAX_BOSS);

					//std::cout << "User is too far away from boss: " << CHARACTERSERVER->GetCharacterName(pcUserData) << std::endl;
					continue;
				}
				else
				{
					//std::cout << "User is near boss: " << CHARACTERSERVER->GetCharacterName(pcUserData) << std::endl;
				}

				DEBUG (" - User is processed. Is within distance and is in same map.");

				//std::cout << "User is solo: " << CHARACTERSERVER->GetCharacterName(pcUserData) << std::endl;



				//set to 0 so that it doesn't get processed elsewhere
				kvp.second = 0;
				processedUserIds.insert({ pcUserData->iID, TRUE });
				ProcessPacketKillUnitID(pcUser, pcUnitData->iID, iKillerUserDataId );

				//User is in a party or raid?
				if ((pcUser->bParty && pcUser->psParty && pcUser->psParty->pcLeader) && pcUser->psParty->iMembersCount > 1)
				{
					//std::cout << "User is in party or raid: " << CHARACTERSERVER->GetCharacterName(pcUserData) << std::endl;

					PartyInfo* psParty = pcUser->psParty;

					//Raid and party kill
					if (psParty->iRaidCount > 0)
					{
						DEBUG (" - User is in a raid");

						for (int i = 0; i < MAX_RAID; i++)
						{
							PartyInfo* psP = i == 0 ? psParty->pcRaidLeader : psParty->pcaRaid[i - 1];
							if ( psP )
							{
								for ( size_t j = 0; j < MAX_PARTY_MEMBERS; j++ )
								{
									User * p = j == 0 ? psP->pcLeader : psP->pcaMembers[j - 1];

									if ( p && p->pcUserData && p->pcUserData->pcSocketData )
									{
										//skip me
										if ( p->pcUserData->iID == pcUserData->iID )
											continue;

										//check if it hasn't been processed yet
										if ( processedUserIds.find ( p->pcUserData->iID ) == processedUserIds.end () )
										{
											int iX = ( p->pcUserData->sPosition.iX - pcUserData->sPosition.iX ) >> 8;
											int iZ = ( p->pcUserData->sPosition.iZ - pcUserData->sPosition.iZ ) >> 8;
											int iDist = iX * iX + iZ * iZ;

											//Is near distance?
											if ( iDist < DISTANCE_MAX_PARTY && p->GetMapID () == pcUserData->iMapID )
											{
												DEBUG ( "   - Raid user (%s) processed. Is within distance and is in same map.",
													CHARACTERSERVER->GetCharacterName ( p->pcUserData ) );

												//add user to processed ids so it doesn't get processed again
												processedUserIds.insert ( { p->pcUserData->iID, TRUE } );

												ProcessPacketKillUnitID ( p, pcUnitData->iID, iKillerUserDataId);

											}
											else
											{
												DEBUG ( "   - Raid user (%s) is not within distance (%d vs %d) or not same map (%d vs %d)",
													CHARACTERSERVER->GetCharacterName ( p->pcUserData ), iDist, DISTANCE_MAX_PARTY, p->GetMapID (), pcUserData->iMapID );
											}
										}
									}
								}
							}
						}
					}

					//Party only
					else
					{
						DEBUG (" - User is in a party");

						for (size_t i = 0; i < MAX_PARTY_MEMBERS; i++)
						{
							User* p = i == 0 ? psParty->pcLeader : psParty->pcaMembers[i - 1];
							if (p && p->pcUserData && p->pcUserData->pcSocketData)
							{
								//skip me
								if (p->pcUserData->iID == pcUserData->iID)
									continue;

								//check if it hasn't been processed yet
								if (processedUserIds.find(p->pcUserData->iID) == processedUserIds.end())
								{
									int iX = (p->pcUserData->sPosition.iX - pcUserData->sPosition.iX) >> 8;
									int iZ = (p->pcUserData->sPosition.iZ - pcUserData->sPosition.iZ) >> 8;
									int iDist = iX * iX + iZ * iZ;

									//Is near distance?
									if (iDist < DISTANCE_MAX_PARTY && p->GetMapID() == pcUserData->iMapID)
									{
										DEBUG ("   - Party user (%s) processed. Is within distance and is in same map.",
											CHARACTERSERVER->GetCharacterName(p->pcUserData));

										//add user to processed ids so it doesn't get processed again
										processedUserIds.insert({ p->pcUserData->iID, TRUE });

										ProcessPacketKillUnitID(p, pcUnitData->iID, iKillerUserDataId);

									}
									else
									{
										DEBUG ("   - Party user (%s) is not within distance (%d vs %d) or not same map (%d vs %d)",
											CHARACTERSERVER->GetCharacterName(p->pcUserData), iDist, DISTANCE_MAX_PARTY, p->GetMapID(), pcUserData->iMapID);
									}
								}
							}
						}
					}
				}

			}
		}

		processedUserIds.clear();
	}
	else
	{
		WARN("Boss unit is null!");
	}

	DEBUG ("------------------------------------------------------------------------------------------------");
	DEBUG ("");
}

void UnitServer::MainUnitData( UnitData * pcUnitData )
{
	CALLT( 0x00429930, (DWORD)pcUnitData );
}

/// <summary>
/// Leak: int rsSendTransSkillAttack(smCHAR *lpChar, rsPLAYINFO *lpPlayInfo)
/// ASM: 0x0054FAB0
/// </summary>
NAKED void UnitServer::SkillUnitDamage( UnitData * pcUnitData, UserData * pcUserData )
{
	JMP( pfnUnitDamageSkill );
}

NAKED void UnitServer::UnitSwapper( UnitData * pcUnitData, CharacterData * psCharacterData, UnitInfo * psUnitInfo )
{
	JMP( pfnUnitSwapper );
}

BOOL UnitServer::OnUnitChanger( UnitData * pcUnitData )
{
	CharacterData * psaCharacterData	= *(CharacterData**)0x07AB3050;
	UnitInfo * psaUnitInfo			= *(UnitInfo**)0x07AB22A4;

	int iCounterCharacterData			= UNITINFODATA->GetTotalUnitEnemy ();

	BOOL bRet							= FALSE;

	//Special Summon Effect?
	if ( pcUnitData->psModelAnimation->iType == ANIMATIONTYPE_Special && pcUnitData->iFrameCounter > 60 )
	{
		EMonsterEffectID eEffect			= (EMonsterEffectID)pcUnitData->sCharacterData.iMonsterEffectID;

		//Mini Hulk?
		if ( eEffect == MONSTEREFFECTID_HUNGKY )
		{
			//Find Hulk
			for ( int i = 0; i < iCounterCharacterData; i++ )
			{
				CharacterData * ps	= psaCharacterData + i;
				UnitInfo * psU = psaUnitInfo + i;

				//Hulk and not Crystal or Bellatra Monster?
				if ( ps->iMonsterEffectID == MONSTEREFFECTID_HULK && ps->szName[0] != ' ' )
				{
					if ( pcUnitData->iHits <= 80 )
					{
						if ( ps->iLevel < 80 )
						{
							//Change
							UnitSwapper( pcUnitData, ps, psU );
							bRet = TRUE;
							break;
						}
					}
					else if ( ps->iLevel >= 80 )
					{
						//Change
						UnitSwapper( pcUnitData, ps, psU );
						bRet = TRUE;
						break;
					}
				}
			}
		}

		//Mini Dead Hopy?
		if ( eEffect == MONSTEREFFECTID_DEADHOPT )
		{
			//Find Dead King Hopy
			for ( int i = 0; i < iCounterCharacterData; i++ )
			{
				CharacterData * ps	= psaCharacterData + i;
				UnitInfo * psU = psaUnitInfo + i;

				//Dead King Hopy and not Crystal or Bellatra Monster?
				if ( ps->iMonsterEffectID == MONSTEREFFECTID_DEADKINGHOPY && ps->szName[0] != ' ' )
				{
					if ( ps->iLevel >= 108 )
					{
						//Change
						UnitSwapper( pcUnitData, ps, psU );
						bRet = TRUE;
						break;
					}
				}
			}
		}
	}

	return bRet;
}

//int srAutoCharMain(smCHAR *lpChar)
//OnServer.cpp, line 9219
//this function gets called ~ 16 times per second
//or at 62.5 FPS
BOOL UnitServer::UpdateUnitData( UnitData * pcUnitData )
{
	//Cannot move? no Update...
	if ( pcUnitData->bNoMove )
		return TRUE;


	//the hp value that ASM will use for pet damage on monsters
	int iHpValueASM = 0;

	//Pet attack
	if ( pcUnitData->pcOwner && pcUnitData->pcAttackTarget )
	{
		//note - do not use GetCurrentHealth() here! because ASM acts on sCharacterData.sHP.sCur
		iHpValueASM = pcUnitData->pcAttackTarget->sCharacterData.sHP.sCur;
	}
	else if (pcUnitData->pcAttackTarget && pcUnitData->pcAttackTarget->IsSummon() )
	{
		iHpValueASM = pcUnitData->pcAttackTarget->sCharacterData.sHP.sCur;
	}

	pcUnitData->Main();

	//Pet attack after
	if ( pcUnitData->pcOwner && pcUnitData->pcAttackTarget )				//this unit has an owner and has a target (typically this is for pets only)
	{
		if ( pcUnitData->pcAttackTarget->GetCurrentHealth() > 0 &&								//monster is still alive
			 pcUnitData->pcAttackTarget->GetAnimation() != EAnimationType::ANIMATIONTYPE_Die ) //monster is not dead
		{
			//calculate hp difference and use this
			//to determine if the pet caused any damage on the mosnter
			//note - do not use GetCurrentHealth() here!
			const int iPetDamageDealt = iHpValueASM - pcUnitData->pcAttackTarget->sCharacterData.sHP.sCur;

			//if iPetDamageDealt is greater than zero
			//then we can assume that the pet DID cause some damage
			if ( iPetDamageDealt > 0 )
			{
				//take actual hp off the monster here.
				//We're using a different HP value to store HP value greater than 32,000
				pcUnitData->pcAttackTarget->TakeHealth( iPetDamageDealt );
				pcUnitData->pcAttackTarget->CheckHealthStatusAfterDamageReceived( pcUnitData->pcOwner );

				//set the ASM HP value back to MAX to ensure that
				//ASM doesn't kill the monster
				pcUnitData->pcAttackTarget->sCharacterData.sHP.sCur = MONSTER_HP_OVERRIDE;

				MAPSERVER->SendUnitDamageInfoTextToNearbyPlayers( pcUnitData->pcOwner, pcUnitData->pcAttackTarget, iPetDamageDealt, FALSE, 0, EDamageTextType::PetAttack );
			}
		}
	}
	//a monster is attacking a summon
	else if (pcUnitData->pcAttackTarget && pcUnitData->pcAttackTarget->IsSummon() )
	{
		if ( pcUnitData->pcAttackTarget->GetCurrentHealth() > 0 &&								//monster is still alive
			 pcUnitData->pcAttackTarget->GetAnimation() != EAnimationType::ANIMATIONTYPE_Die ) //monster is not dead
		{
			//calculate hp difference and use this
			//to determine if the pet caused any damage on the mosnter
			//note - do not use GetCurrentHealth() here!
			const int iDamageDealtToSummon = iHpValueASM - pcUnitData->pcAttackTarget->sCharacterData.sHP.sCur;

			if ( iDamageDealtToSummon > 0 )
			{
				MAPSERVER->SendUnitDamageInfoTextToNearbyPlayers( NULL, pcUnitData->pcAttackTarget, iDamageDealtToSummon, FALSE, 0, EDamageTextType::Damage );
			}
		}
	}

	if ( pcUnitData->IsNonPlayerMonster() )
	{
		//only check if hp isn't 0 and is still 'alive'
		if ( pcUnitData->sCharacterData.sHP.sCur != 0 &&
			 pcUnitData->GetAnimation() != EAnimationType::ANIMATIONTYPE_Die )
		{
			//set HP to max here
			//to make sure that the HP doesn't get down to 0 in ASM
			//otherwise ASM will kill off the monster
			if ( pcUnitData->sCharacterData.sHP.sCur < pcUnitData->sCharacterData.sHP.sMax )
			{
				pcUnitData->sCharacterData.sHP.sCur = pcUnitData->sCharacterData.sHP.sMax;
			}
		}
	}


	if ( pcUnitData->bTestMapUnit )
	{
		if ( TESTMAPHANDLER->UnitCanTargetPlayer ( pcUnitData ) == FALSE &&
			 pcUnitData->GetAnimation () != ANIMATIONTYPE_Die &&
			 pcUnitData->eNextMotionCode != ANIMATIONTYPE_Die )
		{
			//pcUnitData->bNoMove = TRUE;
			pcUnitData->pcFocusTarget = NULL;
			pcUnitData->pcAttackTarget = NULL;
			pcUnitData->pcFocusingTarget = NULL;
			pcUnitData->pcHookedTarget = NULL;
			pcUnitData->pcTarget = NULL;

			pcUnitData->bRunMode = FALSE;
			pcUnitData->bWalking = FALSE;
			pcUnitData->iTargetX = 0;
			pcUnitData->iTargetZ = 0;

			pcUnitData->sAngle.iX = 0;
			pcUnitData->sAngle.iZ = 0;
			pcUnitData->sAngle.iY = 0;

			pcUnitData->iLureDistance = 1;
			pcUnitData->dwIgnoreOthersTime = TICKCOUNT + 3000;

			pcUnitData->sAttackPosition.iX = 0;
			pcUnitData->sAttackPosition.iY = 0;
			pcUnitData->sAttackPosition.iZ = 0;

			pcUnitData->sMoveToPosition.iX = 0;
			pcUnitData->sMoveToPosition.iY = 0;
			pcUnitData->sMoveToPosition.iZ = 0;

		    pcUnitData->iMoveCount = 0;
			pcUnitData->iMovementSpeed = 0;
			pcUnitData->iStopMotionCounter = 20;
			pcUnitData->iTargetMoveCount = 0;
			pcUnitData->iAutoMoveStep = 0;

			pcUnitData->sMoveBeginPosition.iX = 0;
			pcUnitData->sMoveBeginPosition.iY = 0;
			pcUnitData->sMoveBeginPosition.iZ = 0;

			if ( pcUnitData->eNextMotionCode != ANIMATIONTYPE_Idle &&
				 pcUnitData->eNextMotionCode != ANIMATIONTYPE_Flinch )
			{
				pcUnitData->eNextMotionCode = ANIMATIONTYPE_Idle;
			}

			if ( pcUnitData->GetAnimation () != ANIMATIONTYPE_Idle &&
				 pcUnitData->GetAnimation () != ANIMATIONTYPE_Flinch )
			{
				pcUnitData->Animate ( ANIMATIONTYPE_Idle );
			}
		}
	}

	//Animated?
	if ( pcUnitData->psModelAnimation )
	{
		//Die?
		if ( pcUnitData->psModelAnimation->iType == ANIMATIONTYPE_Die )
		{
			//Handle Servers Update
			CHRISTMASHANDLER->OnSayTime( pcUnitData );
			EVENTSERVER->OnSayTime( pcUnitData );
			QUESTSERVER->OnSayTime( pcUnitData );

			Unit * pcUnit = UNITDATATOUNIT(pcUnitData);

			//Killed by an User and isn't a pet?
			if ( pcUnitData->iFrameCounter > 128 && !pcUnitData->pcOwner )
			{
				//for events, need to tidy up even though there is no killer
				EVENTSERVER->OnUnitKilled( pcUnitData, USERDATATOUSER(pcUnitData->pcKillerData));

				if ( pcUnitData->pcKillerData )
				{
					UserData * ud				= pcUnitData->pcKillerData;

					QUESTARENAHANDLER->OnUnitKilled( pcUnitData->pcKillerData, pcUnitData, TRUE );

					int iKillerUserDataId = pcUnitData->pcKillerData->iID;

					pcUnitData->pcKillerData	= NULL;

					//Have Last User Attack and not the same User?
					if ( pcUnitData->pcPriorityAttacker && pcUnitData->pcPriorityAttacker != ud )
					{
						//Unit received Damage more than 40% from Last User?
						if ( pcUnitData->iDamageLastAttacker >= ((pcUnitData->GetMaxHealth() * 40) / 100) )
						{
							//Update User and discard the Killer User
							ud = pcUnitData->pcPriorityAttacker;
						}
					}

					//Quest Unit?
					if ( pcUnitData->sUnitInfo.iQuestItemID )
					{
						if ( pcUnitData->sUnitInfo.eSpecialType )
							MapServer::HandleUnitQuestMap( pcUnitData->pcMap, pcUnitData );
					}

					//User Alive?
					if ( ud->pcSocketData && ud->iID )
					{
						//Handle Kill
						HandleKill( pcUnitData, ud, iKillerUserDataId );

						//Increment Counter Unit Kills
						ud->uUnitKillCounter++;
					}

					//Update Top10 Damage on Bosses
					if ( pcUnitData->psaDamageUsersData )
					{
						if ( pcUnitData->pcKillerData )
						{
							QUESTARENAHANDLER->OnUnitKilled( pcUnitData->pcKillerData, pcUnitData, TRUE );
						}

						//Not Valhalla?
						if ( pcUnitData->sCharacterData.iMonsterEffectID != MONSTEREFFECTID_CASTLE_TOWER_B )
						{
							//Get Top10
							UnitServer::GetTop10Damage( pcUnitData, pcUnitData->psaDamageUsersData );
						}
					}

					//Bellatra User and Unit?
					if ( ud->dwExclusiveNum == 0x100 && pcUnitData->PartyFlag == 0x100 )
					{
						//Don't Have Killer of Bellatra?
						if ( !pcUnitData->pcBellatraKiller )
						{
							//Score
							LARGE_INTEGER lsScore;
							lsScore.QuadPart = UNITSERVER->GetExp(pcUnitData) / EVENT_BELLATRA_MONSTER_EXP_TO_SCORE_SCALE;

							//Add Score
							if (BELLATRASERVER->IsSodActive())
							{
								//Boss?
								if ( pcUnitData->sCharacterData.sMonsterClass == EMonsterClass::Boss )
								{
									//Notify Kill
									CHATSERVER->SendChatEx ( USERDATATOUSER ( ud ), EChatColor::CHATCOLOR_Global, "Bellatra> %s killed!", pcUnitData->sCharacterData.szName );
									CHATSERVER->SendChatEx ( USERDATATOUSER ( ud ), EChatColor::CHATCOLOR_Global, "Bellatra> %s points", FormatNumber ( lsScore.LowPart ) );
								}

								ud->iBellatraScore += lsScore.LowPart;
								ud->iBellatraKills++;
							}

							//Room Kill
							BELLATRASERVER->UnitKilled ( pcUnitData );

							if (BELLATRASERVER->IsSodActive())
							{
								//Handle Score Team if is in Room
								if (ud->sBellatraRoomID >= 0 && ud->sBellatraRoomID < 4)
									BELLATRASERVER->AddRoomScore((int)ud->sBellatraRoomID, lsScore.LowPart);

								//Update User Score
								BellatraServer::UpdateBellatraUserScore ( ud );

								//User Personal Kills
								{
									PacketSimple s;
									s.iLength = sizeof ( PacketSimple );
									s.iHeader = PKTHDR_KillsInSod;
									s.iUnk = ud->iBellatraKills;
									SENDPACKET ( USERDATATOUSER ( ud ), &s, TRUE );
								}
							}

							//Handle Item Room
							BellatraServer::HandleItemRoom ( pcUnitData, ud );
						}
					}
				}
			}

			//Unit disappearing?
			if ( pcUnitData->iFrameCounter > 200 )
			{
				if (pcUnitData->sCharacterData.sMonsterClass == EMonsterClass::Boss)
				{
					DEBUG("BOSS: %s AttackDamageData cleared (B)", pcUnitData->GetName());
				}

				//Clear
				SAFE_DELETE( pcUnitData->psaDamageUsersData );

				QUESTARENAHANDLER->OnUnitKilled( pcUnitData );

				//Is a Pet and User Owner Pet is Online?
				if ( pcUnitData->pcOwner && pcUnitData->pcOwner->pcSocketData )
				{
					//Crystal Die
					ITEMSERVER->OnUseCrystalDie( pcUnitData->pcOwner, pcUnitData );

					if ( pcUnitData->iPetID )
					{
						//Send Cancel Pet Skill
						PacketSimple s;

						ZeroMemory( &s, sizeof( PacketSimple ) );

						//Convert PetID
						int iPetID = 0;

						//Metal Golem
						if ( pcUnitData->iPetID == 15 )
							iPetID = 77;
						//Fire Elemental
						else if ( pcUnitData->iPetID == 77 )
							iPetID = 74;
						//Wolverine
						else if ( pcUnitData->iPetID == 141 )
							iPetID = 74;
						//Hestian
						else if ( pcUnitData->iPetID == 16 )
							iPetID = 74;

						s.iLength = sizeof( PacketSimple );
						s.iHeader = PKTHDR_CancelSkill;
						s.iUnk = pcUnitData->iID;
						s.iUnk2 = iPetID;
						SENDPACKET( USERDATATOUSER( pcUnitData->pcOwner ), &s, TRUE );
					}
				}

				Map * pcMap = (Map *)pcUnitData->pcMap;

				if (pcUnitData->sCharacterData.sMonsterClass == EMonsterClass::Boss)
				{
					DEBUG("BOSS: %s AttackDamageData cleared (A)", pcUnitData->GetName());
				}

				//Clear
				SAFE_DELETE( pcUnitData->psaDamageUsersData );

				//Close Unit
				pcUnitData->Free();

				//Decrease Monster Counter on Map and delete from Spawn Flag
				pcMap->iNumAliveUnitData--;
				MAPSERVER->DelMonsterSpawn( pcMap, pcUnitData );

				//Done
				return TRUE;
			}
		}
		else
		{
			if (pcUnitData->sCharacterData.sMonsterClass == EMonsterClass::Boss &&
				pcUnitData->pcMap->pcBaseMap->iMapID != MAPID_Bellatra)
			{
				if (pcUnitData->psaDamageUsersData == nullptr)
				{
					//DEBUG ("BOSS: %s AttackDamageData initialized (unitserver)", pcUnitData->GetName());

					//huh? We have a boss unit, but we don't have pcUnitData->psaDamageUsersData set?

					pcUnitData->psaDamageUsersData = new AttackDamageData[ATTACK_DAMAGE_MAX];
					if (pcUnitData->psaDamageUsersData)
					{
						ZeroMemory(pcUnitData->psaDamageUsersData, sizeof(AttackDamageData) * ATTACK_DAMAGE_MAX);
					}
				}
			}
		}

		//Unit Swapper
		OnUnitChanger( pcUnitData );
	}

	//Unit Hitted?
	if ( pcUnitData->iAttackUserFlag )
	{
		//User attack on this count
		pcUnitData->iAttackUserFlag = FALSE;

		//Reset
		if ( pcUnitData->pcFocusTarget )
		{
			pcUnitData->pcFocusTarget->iUnitHitCounter = 0;	//Monster attack counter reset
		}
	}

	//If Negative or zero, set zero
	if (pcUnitData->GetCurrentHealth() <= 0 &&
		pcUnitData->GetAnimation() != EAnimationType::ANIMATIONTYPE_Die )
	{
		pcUnitData->SetCurrentHealth( 0 );
		pcUnitData->Animate( EAnimationType::ANIMATIONTYPE_Die );
	}

	//Reload Timer to Appear
	pcUnitData->iDeathCounter = 256; //REOPEN_COUNT_NUM

	//check if we have any burning left
	if ( pcUnitData->pcUnit->bBurningCount > 0 &&
		 TICKCOUNT > pcUnitData->pcUnit->dwBurningNextTime &&
		 pcUnitData->GetCurrentHealth() > 0 &&
	     pcUnitData->IsDead() == FALSE)
	{

		if ( pcUnitData->pcUnit->idBurningCastedBy != 0 )
		{
			User * pcCaster = USERDATATOUSER( USERSERVER->GetUserdata( pcUnitData->pcUnit->idBurningCastedBy ) );

			//Caster no longer exists. set to 0 so that it will get set by someone else
			if ( pcCaster == NULL || pcCaster->IsValidAndInGame() == FALSE )
			{
				WARN( "Burn DoT caster no longer exists" );
				pcUnitData->pcUnit->idBurningCastedBy = 0;
			}
			else
			{
				//if user is no longer in range (further than 30 meters), remove the id too so that
				//it will get set by someone else
				if ( !pcUnitData->sPosition.WithinXZDistance( &pcCaster->pcUserData->sPosition, DISTANCE_XY_30_meters, DISTANCE_MISC_Y_EX ) )
				{
					WARN( "%s (%d): Burn DoT caster is no lnoger in range", pcUnitData->GetName(), pcUnitData->iID );
					pcUnitData->pcUnit->idBurningCastedBy = 0;
				}
			}
		}

		pcUnitData->pcUnit->dwBurningNextTime = TICKCOUNT + BURNING_TICKRATE_MS;
		pcUnitData->pcUnit->bBurningCount -= 1;

		//only allow kill only if the caster is still in-game and is within range
		//that's why the pcUnitData->pcUnit->idBurningCastedBy != 0 check exists.
		const int iDamageDone = pcUnitData->ApplyDamageOverTime( pcUnitData->pcUnit->sBurningDamagePerTick,
			pcUnitData->pcUnit->idBurningCastedBy != 0 );

		if ( iDamageDone > 0 )
		{
			//remove stun
			pcUnitData->iStunTimeLeft = 0;

			UserData * pcCasterUserData = USERSERVER->GetUserdata( pcUnitData->pcUnit->idBurningCastedBy );

			MAPSERVER->SendUnitDamageInfoTextToNearbyPlayers ( pcCasterUserData, pcUnitData, iDamageDone, FALSE, 0, EDamageTextType::Burning );

			pcUnitData->CheckHealthStatusAfterDamageOverTimeReceived( pcCasterUserData );
		}
	}

	//check if we have any burning left
	if ( pcUnitData->pcUnit->bPoisonCount > 0 &&
		 TICKCOUNT > pcUnitData->pcUnit->dwPoisonNextTime &&
		 pcUnitData->GetCurrentHealth() > 0 &&
		 pcUnitData->IsDead() == FALSE)
	{

		if ( pcUnitData->pcUnit->idPoisonCastedBy != 0 )
		{
			User * pcCaster = USERDATATOUSER( USERSERVER->GetUserdata( pcUnitData->pcUnit->idPoisonCastedBy ) );

			//Caster no longer exists. set to 0 so that it will get set by someone else
			if ( pcCaster == NULL || pcCaster->IsValidAndInGame() == FALSE )
			{
				WARN( "Poison DoT caster no longer exists" );
				pcUnitData->pcUnit->idPoisonCastedBy = 0;
			}
			else
			{
				//if user is no longer in range (further than 30 meters), remove the id too so that
				//it will get set by someone else
				if ( !pcUnitData->sPosition.WithinXZDistance( &pcCaster->pcUserData->sPosition, DISTANCE_XY_30_meters , DISTANCE_MISC_Y_EX) )
				{
					WARN( "%s (%d): Poison DoT caster (%s) no longer in range", pcUnitData->GetName(), pcUnitData->iID, pcCaster->GetName() );
					pcUnitData->pcUnit->idPoisonCastedBy = 0;
				}
			}
		}

		pcUnitData->pcUnit->dwPoisonNextTime = TICKCOUNT + POISON_TICKRATE_MS;
		pcUnitData->pcUnit->bPoisonCount -= 1;

		//only allow kill only if the caster is still in-game and is within range
		//that's why the pcUnitData->pcUnit->idPoisonCastedBy != 0 check exists.
		const int iDamageDone = pcUnitData->ApplyDamageOverTime( pcUnitData->pcUnit->sPoisonDamagePerStack * pcUnitData->pcUnit->bPoisonStackCount,
			pcUnitData->pcUnit->idPoisonCastedBy != 0 );

		if ( iDamageDone > 0 )
		{
			//remove stun
			pcUnitData->iStunTimeLeft = 0;

			UserData * pcCasterUserData = USERSERVER->GetUserdata( pcUnitData->pcUnit->idPoisonCastedBy );

			MAPSERVER->SendUnitDamageInfoTextToNearbyPlayers ( pcCasterUserData, pcUnitData, iDamageDone, FALSE, pcUnitData->pcUnit->bPoisonStackCount, EDamageTextType::Poison );

			pcUnitData->CheckHealthStatusAfterDamageOverTimeReceived( pcCasterUserData );
		}
	}


	//Update Unit Status and Scout Enemies?
	//OnServer.cpp line 9459
	//this section gets excuted at roughty 1s interval
	if ( READBYTE( 0x07AC9D68 ) != 0 )
	{
		//Update Animation Data
		Unit * pcUnit = UNITDATATOUNIT( pcUnitData );

		//has poison duration still and is still alive
		//and still has >= 1 hp
		if ( pcUnitData->sCharacterData.iType == CHARACTERTYPE_Monster )
		{
			DAMAGEHANDLER->ClearExpiredDebuffs( pcUnit );
		}

		//Unit * pcUnit = UNITDATATOUNIT( pcUnitData );

		//pcUnitData->MakeUnitBufferData((char*)pcUnit->baUnitBufferNew, 0x10, 4);
		//OnHandleUnitDataBufferNew(pcUnit, (PacketPlayData*)pcUnit->baUnitBufferNew);

		//int iX								= pcUnitData->sPosition.iX;
		//int iY								= pcUnitData->sPosition.iY;
		//int iZ								= pcUnitData->sPosition.iZ;

		//int iDistanceMin					= 0x00320000; //DIST_TRANSLEVEL_LOW
		int iDistanceMin					= DISTANCE_XY_54_meters;

		Map * psStageData	= (Map *)pcUnitData->pcMap;

		//On Stage and have Owner User?




		if( psStageData && pcUnitData->pcOwner )
		{
#pragma region  PET_UNIT_DATA_CODE


			//std::cout << "pcUnitData->pcOwner " << pcUnitData->pcOwner->szCharacterName << ", name " << pcUnitData->GetName() <<  std::endl;

			UnitData * pcTarget = NULL;

			//Update Pet position when player change the map
			//if( psStageData->pcBaseMap->iMapID != pcUnitData->pcOwner->iMapID )
			//{
			//	pcUnitData->pcMap = MAPSDATA + pcUnitData->pcOwner->iMapID;
			//	pcUnitData->pcStage = pcUnitData->pcMap->pcStage;
			//	pcUnitData->SetPosition( pcUnitData->pcOwner->sPosition.iX, pcUnitData->pcOwner->sPosition.iY, pcUnitData->pcOwner->sPosition.iZ );
			//	pcUnitData->sMoveToPosition = pcUnitData->pcOwner->sPosition;
			//}
			if (!pcUnitData->pcOwner->dwObjectPetSystemID)//if it is a petsystem if it is a petsystem it does not receive pctarget
			{
				if (pcUnitData->pcOwner->pcLastAttack)
				{
					if (pcUnitData->pcOwner->pcLastAttack->GetCurrentHealth() <= 0 ||
						pcUnitData->pcOwner->pcLastAttack->GetAnimation() == EAnimationType::ANIMATIONTYPE_Die)
					{
						pcUnitData->pcOwner->pcLastAttack = NULL;
					}
				}

				//Check and clear current attack target
				if (pcUnitData->pcAttackTarget)
				{
					if (pcUnitData->pcAttackTarget->GetCurrentHealth() <= 0 ||
						pcUnitData->pcAttackTarget->GetAnimation() == EAnimationType::ANIMATIONTYPE_Die ||
						pcUnitData->pcOwner->sPosition.WithinXZYDistance(&pcUnitData->pcAttackTarget->sPosition, DISTANCE_300, 200) == FALSE)
					{
						pcUnitData->pcAttackTarget = NULL;
					}
				}

				//Check and clear current focusing target
				if (pcUnitData->pcFocusingTarget)
				{
					if (pcUnitData->pcFocusingTarget->GetCurrentHealth() <= 0 ||
						pcUnitData->pcFocusingTarget->GetAnimation() == EAnimationType::ANIMATIONTYPE_Die ||
						pcUnitData->pcOwner->sPosition.WithinXZYDistance(&pcUnitData->pcFocusingTarget->sPosition, DISTANCE_300, 200) == FALSE)
					{
						pcUnitData->pcFocusingTarget = NULL;
						pcUnitData->iFocusingLastID = -1;
					}
				}

				//Check and clear current focusing target
				if (pcUnitData->pcFocusingTarget)
				{
					if (pcUnitData->pcFocusingTarget->GetCurrentHealth() <= 0 ||
						pcUnitData->pcFocusingTarget->GetAnimation() == EAnimationType::ANIMATIONTYPE_Die ||
						pcUnitData->pcOwner->sPosition.WithinXZYDistance(&pcUnitData->pcFocusingTarget->sPosition, DISTANCE_300, 200) == FALSE)
					{
						pcUnitData->pcFocusingTarget = NULL;
						pcUnitData->iFocusingLastID = -1;
					}
				}

				//Set Target for Pet
				for (int i = 0; i < psStageData->iNumUsingUnitData; i++)
				{
					UnitData* ps = (UnitData*)psStageData->pcaUnitData[i];

					//Alive and not the same Unit?
					if (ps && ps != pcUnitData && ps->GetCurrentHealth() > 0 && ps->GetAnimation() != EAnimationType::ANIMATIONTYPE_Die)
					{
						//Both is Pet?
						if (pcUnitData->pcOwner && ps->pcOwner)
							continue;

						//Unit is the same Clan Pet? then cannot Attack
						if (pcUnitData->sCharacterData.iClanID && pcUnitData->sCharacterData.iClanID == ps->sCharacterData.iClanID)
							continue;

						//Check Unit Clan SW Master Type
						if (pcUnitData->iPetID && pcUnitData->bBlessCastleCrown && ps->bBlessCastleCrown)
						{
							//Skill pet? then cannot Attack
							if (ps->iPetID)
								continue;

							//Unit Pet is not of User? then cannot Attack
							if (ps->lExclusiveTargetID && ps->lExclusiveTargetID != pcUnitData->pcOwner->iID)
								continue;
						}

						//SW Pet? then cannot Attack
						if (pcUnitData->bBlessCastleCrown && ps->bBlessCastleCrown)
							continue;

						//Unit Exclusive? then cannot Attack
						if (pcUnitData == ps->pcExclusiveUnitTarget || pcUnitData->pcExclusiveUnitTarget == ps)
							continue;



						//Boss check
						if (ps->sCharacterData.sMonsterClass == EMonsterClass::Boss &&
							pcUnitData->psModelAnimation && ps->psModelAnimation &&
							pcUnitData->psModelAnimation->iType != ANIMATIONTYPE_Die &&
							pcUnitData->PartyFlag != 0x200 &&
							ps->psModelAnimation->iType != ANIMATIONTYPE_Die)
						{
							int iDistXZY, iDistXZ, iDistX, iDistZ, iDistY;
							ps->sPosition.GetPTDistanceXZY(&pcUnitData->sPosition, iDistXZY, iDistXZ, iDistX, iDistZ, iDistY);

							//Is pet near boss? If so kill pet
							if (iDistXZ < DISTANCE_XY_54_meters && iDistX < DISTANCE_MISC && iDistZ < DISTANCE_MISC)
							{
								int iEffectID = ps->sCharacterData.iMonsterEffectID;

								if (iEffectID != MONSTEREFFECTID_CASTLE_TOWER_B &&
									iEffectID != MONSTEREFFECTID_CASTLE_DOOR &&
									iEffectID != MONSTEREFFECTID_CASTLE_CRYSTAL_R &&
									iEffectID != MONSTEREFFECTID_CASTLE_CRYSTAL_G &&
									iEffectID != MONSTEREFFECTID_CASTLE_CRYSTAL_B &&
									iEffectID != MONSTEREFFECTID_CASTLE_CRYSTAL_N &&
									iEffectID != MONSTEREFFECTID_CASTLE_SOLDER_A &&
									iEffectID != MONSTEREFFECTID_CASTLE_SOLDER_B &&
									iEffectID != MONSTEREFFECTID_CASTLE_SOLDER_C)
								{
									//Kill
									pcUnitData->SetCurrentHealth(0);
									pcUnitData->Animate(ANIMATIONTYPE_Die);
									return TRUE;
								}
							}
						}

						//skill summoned pet
						if (pcUnitData->iPetID)
						{
							//currently has no focusing target
							//let's set focus target to the owner's last attacked target
							if (pcUnitData->pcFocusingTarget == NULL)
							{
								//found matching target? set focus to it!
								if (pcUnitData->pcOwner->pcLastAttack &&
									pcUnitData->pcOwner->pcLastAttack == ps &&
									pcUnitData->pcOwner->sPosition.WithinXZYDistance(&pcUnitData->pcOwner->pcLastAttack->sPosition, DISTANCE_300, 200))
								{
									pcUnitData->pcFocusingTarget = pcUnitData->pcOwner->pcLastAttack;

									if (pcUnitData->iFocusingLastID == -1 || ps->iID == pcUnitData->iFocusingLastID)
									{
										pcTarget = pcUnitData->pcFocusingTarget;
										break;
									}
									else
									{
										pcUnitData->iFocusingLastID = -1;
										continue;
									}
								}
								else
								{
									//if the pet is not set to not aggressive, then have it ignore everyone else
									if (pcUnitData->bAggressive == FALSE)
										continue;
								}
							}
						}

						//pets returns to owner if they are ~11m (300) away from the owner.
						//so we check here too
						if (!pcUnitData->pcOwner->sPosition.WithinXZYDistance(&ps->sPosition, DISTANCE_300, 200))
							continue;

						int iDistXZY, iDistXZ, iDistX, iDistZ, iDistY;
						pcUnitData->sPosition.GetPTDistanceXZY(&ps->sPosition, iDistXZY, iDistXZ, iDistX, iDistZ, iDistY);

						//Unit Near from Pet?
						if (iDistXZ < DISTANCE_MAX_PET_RANGE && iDistX < DISTANCE_MISC && iDistZ < DISTANCE_MISC && iDistY < 200)
						{
							//Update Minimal Distance
							iDistanceMin = iDistXZ;

							//Set Pet Target
							pcTarget = ps;

							//let's continue attacking this current focusing attack or focus target
							//since this attack target is already in range
							if (pcTarget == pcUnitData->pcFocusingTarget)
								break;
						}
					}
				}
				//target have dwObjectPetSystemID pc unitdata not receive target
				if (pcTarget)
				{
					if (pcTarget->pcOwner != nullptr)
					{
						if (pcTarget->pcOwner->dwObjectPetSystemID)
						{
							pcTarget = NULL;
						}
					}
				}
				//Have Unit Target? 
				if (pcTarget )
				{
					
					//Update Target
					pcUnitData->pcAttackTarget = pcTarget;
					pcUnitData->pcFocusingTarget = pcTarget;

					//std::cout << " | pcUnitData->pcAttackTarget to " << pcTarget->GetName();

					pcUnitData->sMoveToPosition = pcTarget->sPosition;

					//Update Position to Target Position
					pcUnitData->NextTarget(pcTarget->sPosition.iX, pcTarget->sPosition.iY, pcTarget->sPosition.iZ);

					if (pcUnitData->GetAnimation() == EAnimationType::ANIMATIONTYPE_Idle)
						pcUnitData->Animate(EAnimationType::ANIMATIONTYPE_Walking);
				}

				
				//Time Alive is Over?
				if (pcUnitData->dwUpdateCharInfoTime && pcUnitData->dwUpdateCharInfoTime < TICKCOUNT && pcUnitData->GetCurrentHealth() > 0)
				{
					//Kill Unit
					pcUnitData->SetCurrentHealth(0);
					pcUnitData->Animate(ANIMATIONTYPE_Die);
				}
			}
			//std::cout << std::endl;

#pragma endregion
		}

		//Unit doesn't have an owner, therefore is a monster
		else
		{
			if ( pcUnitData->dwUpdateCharInfoTime && pcUnitData->dwUpdateCharInfoTime < TICKCOUNT
				 && pcUnitData->GetCurrentHealth() > 0
				 && pcUnitData->iAliveCounter < 10) //if alive counter is greater than 10 then someone encountered it recently, don't kill off yet.
			{
				//Kill Unit
				pcUnitData->SetCurrentHealth( 0 );
				pcUnitData->Animate( ANIMATIONTYPE_Die );
			}

			BOOL bNextTarget = FALSE;	//TargetSearchFlag

			//Quest Arena Check
			if ( QUESTARENAHANDLER->IsUpdatedUnit( pcUnitData ) )
			{
				UnitData * pcTarget = pcUnitData->pcAttackTarget;

				//Update Position to Target Position
				pcUnitData->NextTarget( pcTarget->sPosition.iX, pcTarget->sPosition.iY, pcTarget->sPosition.iZ );

				bNextTarget = TRUE;
			}

			//Normal monster unit received Compulsion Skill?
			if ( pcUnitData->dwHookedTargetEndTime && (pcUnitData->sCharacterData.sMonsterClass == EMonsterClass::Normal) )
			{
				UserData * ud = pcUnitData->pcHookedTarget;

				//Compulsion Alive?
				if ( pcUnitData->dwHookedTargetEndTime > TICKCOUNT && ud && ud->pcSocketData )
				{
					//Set Focus on User
					pcUnitData->pcFocusTarget = ud;

					//Update Position to Target Position
					pcUnitData->NextTarget( ud->sPosition.iX, ud->sPosition.iY, ud->sPosition.iZ );

					//Find Next Target
					bNextTarget = TRUE;
				}
				else
				{
					//Clear Compulsion Skill
					pcUnitData->dwHookedTargetEndTime	= 0;
					pcUnitData->pcHookedTarget			= NULL;
				}
			}

			//TargetSerachFlag = FALSE
			if ( !bNextTarget )
			{
				//Unit does not have any char target
				//lpChar->lpTargetChar = FALSE (OnServer.cpp)
				//Here the unit 'search' for a target (i think)
				if ( !pcUnitData->pcAttackTarget )
				{
					//DEBUG("!bNextTarget && !pcUnitData->pcAttackTarget");

					//std::cout << TICKCOUNT << " | " << "lpChar->lpTargetChar = FALSE" << std::endl;

					//Find Users Near
					UserData * pcTarget = NULL;

					for ( int i = 0; i < PLAYERS_MAX; i++ )
					{
						UserData * ud = USERSDATA + i;
						User * u = USERDATATOUSER( ud );

						if ( ud->pcSocketData && ud->iID &&
							 ud->GetCurrentHealth() > 0 &&
							 ud->sCharacterData.iType != ECharacterType::CHARACTERTYPE_Monster &&
							 ud->bHideMode == FALSE)
						{
							if ( TESTMAPHANDLER->UnitCanTargetPlayer ( pcUnitData ) == FALSE )
								continue;

							//Not Exclusive User? Next Target...
							if ( pcUnitData->lExclusiveTargetID && pcUnitData->lExclusiveTargetID != ud->iID )
								continue;

							//Unit is belongs to the same clan of user? Next Target...
							if ( pcUnitData->iClanID && pcUnitData->iClanID == ud->iClanID )
								continue;


							int iDistXZY, iDistXZ, iDistX, iDistZ, iDistY;
							pcUnitData->sPosition.GetPTDistanceXZY(&ud->sPosition, iDistXZY, iDistXZ, iDistX, iDistZ, iDistY);

							//User Near from Unit? (41 meters)
							if ( iDistXZ < DISTANCE_XY_41_meters && iDistX < DISTANCE_MISC && iDistZ < DISTANCE_MISC )
							{
								//Area Skill, and is within max skill range?
								if ( pcUnitData->iAreaDamageArea && iDistXZ < DISTANCE_MAX_SKILL_RANGE && iDistY < DISTANCE_MISC_Y )
								{
									//Inside Area Skill?
									if ( pcUnitData->sUnitInfo.iSkillPierceRange && pcUnitData->sUnitInfo.iSkillBoxRight && pcUnitData->sUnitInfo.iSkillBoxLeft )
									{
										//Inside X? (left / right bird view)
										if ( ud->sPosition.iX > pcUnitData->sUnitInfo.iSkillBoxLeft && ud->sPosition.iX < pcUnitData->sUnitInfo.iSkillBoxRight )
										{
											//Inside Z? (up / down bird view)
											if ( ud->sPosition.iZ > pcUnitData->sUnitInfo.iSkillBoxTop && ud->sPosition.iZ < pcUnitData->sUnitInfo.iSkillBoxBottom )
											{
												//std::cout << TICKCOUNT << " | " << "SkillUnitDamage - iSkillPierceRange" << std::endl;

												//Send Damage to User
												SkillUnitDamage( pcUnitData, ud );
											}
										}
									}

									//Simple radius
									else if ( iDistXZ < pcUnitData->iAreaDamageArea )
									{
										//std::cout << TICKCOUNT << " | " << "SkillUnitDamage - iAreaDamageArea" << std::endl;

										//Send Damage to User
										SkillUnitDamage( pcUnitData, ud );
									}
								}

								//Is this unit closer than the previous unit, and isn't too high or too low ?
								if ( iDistXZ < iDistanceMin && iDistY < 50 )
								{
									//New Distance relative to View Sight of Unit
									int iDistanceN = pcUnitData->sCharacterData.iSight;

									int iVanishParam = ud->saVanishParam[1];

									//Vanish, and the unit is not boss?
									if ( iVanishParam &&
										 pcUnitData->sCharacterData.sMonsterClass != EMonsterClass::Boss)
									{
										//Vanish Distance can affect Unit
										iDistanceN = (pcUnitData->sUnitInfo.iSight * (int)ud->saVanishParam[1]) / 100;
										iDistanceN += pcUnitData->sUnitInfo.iSight;

										//Compute Sight Distance
										if ( iDistanceN > 0 )
											iDistanceN *= iDistanceN;
										else
											iDistanceN = 0;
									}

									//Not Hidden and on distance?
									if ( iDistXZ < iDistanceN && !ud->bHideMode )
									{
										//Both in Bellatra?
										if ( pcUnitData->PartyFlag == 0x100 )
										{
											if ( ud->dwExclusiveNum == 0x100 )
											{
												//Target User
												pcTarget = ud;
											}
										}
										else
										{
											pcTarget = ud;
										}

										//Update Minimal Distance
										iDistanceMin = iDistXZ;
									}
								}
							}
						}
					}

					//Has stun? No Target
					if ( pcUnitData->iStunTimeLeft )
						pcTarget = NULL;

					//Range Skill? Clear it...
					if ( pcUnitData->iAreaDamageArea )
						pcUnitData->iAreaDamageArea = 0;

					//std::cout << TICKCOUNT << " | " << "pcUnitData->pcFocusTarget set!" << std::endl;

					//Set New Target
					pcUnitData->pcFocusTarget = pcTarget;

					if (pcUnitData->bActive &&
						pcUnitData->sCharacterData.sMonsterClass == EMonsterClass::Normal &&
						pcUnitData->pcMap->pcBaseMap->iLevel >= 108 &&
						(pcUnitData->PkMode_CharState == CHARACTERTYPE_Monster || pcUnitData->sCharacterData.iType == CHARACTERTYPE_Monster) &&
						pcUnitData->pcOwner == NULL) //pets
					{

						UpdateUnitData_AntiStuck( pcUnit, pcUnitData );
					}


					//If unit is an exclusive bosses - prevent them getting stuck
					//by implement some measures.
					else if (IsStuckPreventionBossUnit(pcUnitData) &&
						     pcUnitData->pcMap->pcBaseMap->iMapID != MAPID_Bellatra &&
						     pcUnitData->pcMap->pcBaseMap->iMapID != MAPID_QuestArena) //don't implement logic for bellatra
					{

						UpdateUnitData_AntiStuck( pcUnit, pcUnitData );

					} //end of IsStuckPreventionBossUnit
					else
					{
						if (pcUnitData->pcFocusTarget)
						{
							pcUnitData->NextTarget(pcUnitData->pcFocusTarget->sPosition.iX, pcUnitData->pcFocusTarget->sPosition.iY, pcUnitData->pcFocusTarget->sPosition.iZ);
						}
					}
				}
				else
				{
					//DEBUG("!bNextTarget && pcUnitData->pcAttackTarget");

					UnitData * pcTarget = pcUnitData->pcAttackTarget;

					//Update Position to Target Position
					pcUnitData->NextTarget( pcTarget->sPosition.iX, pcTarget->sPosition.iY, pcTarget->sPosition.iZ );
				}
			}
			else
			{
				//DEBUG("bNextTarget");
			}


		}

		//Not Alive?
		if ( pcUnitData->eActionPattern == ACTIONMODE_ServerMonster && pcUnitData->dwLastActiveTime )
		{
			DWORD dwTime = pcUnitData->dwLastActiveTime + (5 * 60 * 1000); //5 minutes active time

			if ( !pcUnitData->dwUpdateCharInfoTime && dwTime < TICKCOUNT )
			{
				//Close Unit
				pcUnitData->Free();

				//Decrease Monster Counter on Map and delete from Spawn Flag
				psStageData->iNumAliveUnitData--;
				MAPSERVER->DelMonsterSpawn( psStageData, pcUnitData );
			}
		}

		//Pet?
		if ( pcUnitData->pcOwner )
		{
			UserData * ud = pcUnitData->pcOwner;
			Map * pcMap = pcUnitData->pcMap;

			//Online?
			if ( pcMap && pcMap->pcBaseMap && ud->pcSocketData )
			{
				if ( ud->iMapID != pcMap->pcBaseMap->iMapID )
				{
					//Clear
					SAFE_DELETE( pcUnitData->psaDamageUsersData );

					//Close Unit
					pcUnitData->Free();

					//Decrease Monster Counter on Map and delete from Spawn Flag
					psStageData->iNumAliveUnitData--;
					MAPSERVER->DelMonsterSpawn( psStageData, pcUnitData );
					if (ud->dwObjectPetSystemID)
					{
						pcUnitData->SetCurrentHealth(0);
						pcUnitData->Animate(ANIMATIONTYPE_Die);
						ud->dwObjectPetSystemID = 0;
						ud->pcPetSystem = nullptr;
						ud ->bPetSystemSkinID = 0;
						PETSYSTEMHANDLER->SpawnPet(ud);
					}
				}
			}
		}

		pcUnitData->MakeUnitBufferData((char*)pcUnit->baUnitBufferNew, 0x10, 4);
		OnHandleUnitDataBufferNew(pcUnit, (PacketPlayData*)pcUnit->baUnitBufferNew);
	}

	return TRUE;
}

BOOL UnitServer::UpdateUnitData_AntiStuck(Unit * pcUnit, UnitData * pcUnitData)
{
	BOOL bIsABoss = pcUnitData->sCharacterData.sMonsterClass == EMonsterClass::Boss;

	if (pcUnitData->GetAnimation() == EAnimationType::ANIMATIONTYPE_Die)
	{
		pcUnit->bHealItself = FALSE;
		pcUnit->bReturningToSpawn = FALSE;
		pcUnit->bHasTargetFocus = FALSE;
		pcUnit->dwReturningToSpawnTimeout = 0;
		pcUnit->dwLastFocusOrAttackTime = 0;
		pcUnit->iIdleWithFocusCounter = 0;
		pcUnit->dwLastNonIdleTime = 0;
		pcUnit->iStuckCounter = 0;

		pcUnitData->SetCurrentHealth( 0 );

		pcUnitData->pcFocusTarget = NULL;
		pcUnitData->pcFocusingTarget = NULL;
		pcUnitData->pcTarget = NULL;

		return TRUE;
	}

	std::stringstream ss;


	ss << pcUnit->GetName() << " | Lure = " << pcUnitData->iLureDistance << " | " << pcUnitData->bRunMode << " | " << pcUnitData->bWalking;



	//heal itself until it retains target focus
	if (pcUnit->bHealItself)
	{
		//restore 1/30 of its health per tick
		//so for a monster with 30,000 health that's 1000 per tick
		//or ~ 30 seconds to heal
		pcUnitData->GiveHealth( pcUnitData->GetMaxHealth() / 30 );
	}


	if (pcUnit->bReturningToSpawn)
	{
		BOOL bIsWithinSpawn = pcUnitData->sPosition.WithinXZDistance(&pcUnitData->sSpawnPosition, DISTANCE_XY_3_meters);

		int iLastRunOrWalkPosDistXZ = pcUnitData->sPosition.GetPTDistanceXZ(&pcUnit->dwLastWalkOrRunPos);

		if (bIsWithinSpawn == FALSE)
		{
			if (iLastRunOrWalkPosDistXZ < DISTANCE_XY_025)
			{
				pcUnit->iStuckCounter++;

				if (pcUnit->iStuckCounter >= 3)
				{
					pcUnitData->dwIgnoreOthersTime = TICKCOUNT + 5000;
				}
			}
			else
			{
				pcUnit->dwReturningToSpawnTimeout = TICKCOUNT + 5000;
				pcUnit->iStuckCounter = 0;
				pcUnit->dwLastWalkOrRunPos.iX = pcUnitData->sPosition.iX;
				pcUnit->dwLastWalkOrRunPos.iZ = pcUnitData->sPosition.iZ;
			}
		}

		BOOL bTimedOutReturningToSpawn = pcUnit->dwReturningToSpawnTimeout != 0 && TICKCOUNT > pcUnit->dwReturningToSpawnTimeout;

		ss << " | Moving back to spawn | Stuck counter = " << pcUnit->iStuckCounter << " | " << iLastRunOrWalkPosDistXZ;

		//is no longer ignoring targets while returning to spawn, and has a nearby target to focus
		if (pcUnitData->pcFocusTarget != NULL && pcUnitData->dwIgnoreOthersTime < TICKCOUNT)
		{
			ss << " | Has target!";

			pcUnitData->iTargetX = 0;
			pcUnitData->iTargetZ = 0;
		//	pcUnitData->bRunMode = TRUE;
			pcUnit->iStuckCounter = 0;
			pcUnit->bReturningToSpawn = FALSE;
			pcUnit->dwReturningToSpawnTimeout = 0;
		}
		else if (bIsWithinSpawn || bTimedOutReturningToSpawn || pcUnit->iStuckCounter >= 5)
		{
			if (bTimedOutReturningToSpawn || pcUnit->iStuckCounter >= 5)
			{
				ss << " | Teleport!";

				pcUnitData->Animate(EAnimationType::ANIMATIONTYPE_Special);

				pcUnitData->sPosition.iX = pcUnit->sReturningToSpawnPosition.iX;
				pcUnitData->sPosition.iZ = pcUnit->sReturningToSpawnPosition.iZ;
				pcUnitData->sPosition.iY = CALLT_WITH_ARG2(0x004694D0, (DWORD)pcUnitData->pcStage, pcUnitData->sPosition.iX, pcUnitData->sPosition.iZ);
			}
			else
			{
				ss << " | Within";
			}

			pcUnitData->iTargetX = 0;
			pcUnitData->iTargetZ = 0;
			//pcUnitData->bRunMode = FALSE;
			pcUnit->iStuckCounter = 0;
			pcUnit->bReturningToSpawn = FALSE;
			pcUnit->dwReturningToSpawnTimeout = 0;
			pcUnitData->Animate(EAnimationType::ANIMATIONTYPE_Idle);
		}
		else
		{
			pcUnitData->SetAngleByPosition(pcUnitData->sSpawnPosition.iX >> 8, pcUnitData->sSpawnPosition.iZ >> 8);

			if (pcUnitData->GetAnimation() == EAnimationType::ANIMATIONTYPE_Idle)
			{
				ss << " | Set to Run";

				if (!pcUnitData->Animate(ANIMATIONTYPE_Running))
					pcUnitData->Animate(ANIMATIONTYPE_Walking);

				pcUnitData->iAutoMoveStep = pcUnitData->sUnitInfo.iIntelligence - 5;
			}

			ss << " | NOT within";
		}


	}

	//Unit is NOT returning to spawn.

	if (pcUnit->bReturningToSpawn == FALSE)
	{
		int iSpawnDistXZ = pcUnitData->sPosition.GetPTDistanceXZ(&pcUnitData->sSpawnPosition);

		ss << " | iSpawnDistXZ = " << iSpawnDistXZ;

		//unit vendered beyond allowable lure distance
		if (iSpawnDistXZ > pcUnitData->iLureDistance)
		{
			ss << " | iLureDistance reached";
			pcUnitData->dwIgnoreOthersTime = TICKCOUNT + 5000;

			pcUnitData->pcAttackTarget = NULL;
			pcUnitData->pcFocusTarget = NULL;
			pcUnitData->pcFocusingTarget = NULL;
			pcUnitData->iFocusingLastID = -1;
			//pcUnitData->bRunMode = TRUE;
			pcUnitData->iTargetMoveCount = 6;

			pcUnitData->SetAngleByPosition(pcUnitData->sSpawnPosition.iX >> 8, pcUnitData->sSpawnPosition.iZ >> 8);

			pcUnitData->sMoveToPosition = Point3D(pcUnitData->sSpawnPosition.iX, pcUnitData->sSpawnPosition.iY, pcUnitData->sSpawnPosition.iZ);

			pcUnit->bHealItself = TRUE;
			pcUnit->bReturningToSpawn = TRUE;
			pcUnit->iStuckCounter = 0;
			pcUnit->dwReturningToSpawnTimeout = TICKCOUNT + 10000; //10s to return
			pcUnit->sReturningToSpawnPosition = Point3D(pcUnitData->sSpawnPosition.iX, pcUnitData->sSpawnPosition.iY, pcUnitData->sSpawnPosition.iZ);
			pcUnit->dwLastWalkOrRunPos.iX = pcUnitData->sPosition.iX;
			pcUnit->dwLastWalkOrRunPos.iZ = pcUnitData->sPosition.iZ;

			pcUnitData->NextTarget(pcUnitData->sSpawnPosition.iX, pcUnitData->sSpawnPosition.iY, pcUnitData->sSpawnPosition.iZ);

			if (!pcUnitData->Animate(ANIMATIONTYPE_Running))
				pcUnitData->Animate(ANIMATIONTYPE_Walking);

			return TRUE;
		}

		//Is unit not idling?
		if (pcUnitData->GetAnimation() != EAnimationType::ANIMATIONTYPE_Idle)
		{
			ss << " | Non-idling";

			//Record down the tick when the unit last attacked someone
			pcUnit->dwLastNonIdleTime = TICKCOUNT;
		}
		else
		{
			ss << " | Idling";
		}

		//unit does not have focus or attack target
		if ((pcUnitData->pcFocusingTarget == NULL && pcUnitData->pcFocusTarget == NULL && pcUnitData->pcAttackTarget == NULL))
		{
			pcUnit->bHasTargetFocus = FALSE;
			ss << " | No focus or attack target";

			//No focus or attack target for at least 6 seconds?
			if ((pcUnit->dwLastFocusOrAttackTime == 0 && pcUnit->bHealItself == FALSE) ||
				pcUnit->dwLastFocusOrAttackTime != 0 && (TICKCOUNT - pcUnit->dwLastFocusOrAttackTime) > 6000)
			{
				pcUnit->bHealItself = TRUE;

				//Is unit at least 7 meters away from spawn?
				//if so, move back to spawn
				//otherwise don't move.
				if (pcUnitData->sPosition.WithinXZDistance(&pcUnitData->sSpawnPosition, DISTANCE_XY_7_meters) == FALSE)
				{
					pcUnitData->pcAttackTarget = NULL;
					pcUnitData->pcFocusTarget = NULL;
					pcUnitData->pcFocusingTarget = NULL;
					pcUnitData->iFocusingLastID = -1;
					//pcUnitData->bRunMode = TRUE;
					pcUnitData->iTargetMoveCount = 6;

					pcUnitData->SetAngleByPosition(pcUnitData->sSpawnPosition.iX >> 8, pcUnitData->sSpawnPosition.iZ >> 8);

					pcUnitData->sMoveToPosition = Point3D(pcUnitData->sSpawnPosition.iX, pcUnitData->sSpawnPosition.iY, pcUnitData->sSpawnPosition.iZ);

					pcUnit->bHealItself = TRUE;
					pcUnit->bReturningToSpawn = TRUE;
					pcUnit->iStuckCounter = 0;
					pcUnit->dwReturningToSpawnTimeout = TICKCOUNT + 10000; //10s to return
					pcUnit->sReturningToSpawnPosition = Point3D(pcUnitData->sSpawnPosition.iX, pcUnitData->sSpawnPosition.iY, pcUnitData->sSpawnPosition.iZ);

					pcUnit->dwLastWalkOrRunPos.iX = pcUnitData->sPosition.iX;
					pcUnit->dwLastWalkOrRunPos.iZ = pcUnitData->sPosition.iZ;

					pcUnitData->NextTarget(pcUnitData->sSpawnPosition.iX, pcUnitData->sSpawnPosition.iY, pcUnitData->sSpawnPosition.iZ);

					if (!pcUnitData->Animate(ANIMATIONTYPE_Running))
						pcUnitData->Animate(ANIMATIONTYPE_Walking);
				}
			}
		}
		else
		{
			if (pcUnitData->pcAttackTarget != NULL)
			{
				ss << " | Has attack target";
			}

			if (pcUnitData->pcFocusingTarget != NULL)
			{
				ss << " | Has focusing target";
			}

			//Record tick when this unit last had focus or attack target
			//if no focus or attack target for 5 seconds, let's move back to spawn.
			pcUnit->dwLastFocusOrAttackTime = TICKCOUNT;
		}

		//Has focus target is isn't ignoring
		if (pcUnitData->pcFocusTarget != NULL && pcUnitData->dwIgnoreOthersTime < TICKCOUNT)
		{
			ss << " | Has focus target";

			//stop healing
			pcUnit->bHealItself = FALSE;
			pcUnit->bReturningToSpawn = FALSE;

			//pcUnitData->bRunMode = TRUE;

			if (pcUnit->bHasTargetFocus == FALSE && iSpawnDistXZ <= pcUnitData->iLureDistance)
			{
				ss << " | First focus target";

				pcUnitData->iTargetX = pcUnitData->pcFocusTarget->sPosition.iX;
				pcUnitData->iTargetZ = pcUnitData->pcFocusTarget->sPosition.iZ;

				pcUnitData->SetAngleByPosition(pcUnitData->pcFocusTarget->sPosition.iX >> 8,
					pcUnitData->pcFocusTarget->sPosition.iZ >> 8);

				pcUnitData->NextTarget(pcUnitData->pcFocusTarget->sPosition.iX,
					pcUnitData->pcFocusTarget->sPosition.iY,
					pcUnitData->pcFocusTarget->sPosition.iZ);


				pcUnit->bHasTargetFocus = TRUE;
			}

			//has focus but idle or walking? Move its lazy butt
			if (pcUnitData->GetAnimation() == EAnimationType::ANIMATIONTYPE_Idle ||
				pcUnitData->GetAnimation() == EAnimationType::ANIMATIONTYPE_Walking)
			{
				int iDistXZY, iDistXZ, iDistX, iDistZ, iDistY;
				pcUnitData->sPosition.GetPTDistanceXZY(&pcUnitData->pcFocusTarget->sPosition, iDistXZY, iDistXZ, iDistX, iDistZ, iDistY);

				if (iDistXZY >= DISTANCE_XY_3_meters && iSpawnDistXZ <= pcUnitData->iLureDistance)
				{
					ss << " | Set to Running | " << pcUnitData->iAutoMoveStep << " | " << pcUnitData->sUnitInfo.iIntelligence;

					if (!pcUnitData->Animate(ANIMATIONTYPE_Running))
						pcUnitData->Animate(ANIMATIONTYPE_Walking);

					pcUnitData->iAutoMoveStep = pcUnitData->sUnitInfo.iIntelligence - 5;
					pcUnitData->SetAngleByPosition(pcUnitData->pcFocusTarget->sPosition.iX >> 8, pcUnitData->pcFocusTarget->sPosition.iZ >> 8);
					pcUnitData->NextTarget(pcUnitData->pcFocusTarget->sPosition.iX, pcUnitData->pcFocusTarget->sPosition.iY, pcUnitData->pcFocusTarget->sPosition.iZ);

				}
			}

			//Unit is not attacking and isn't dead
			if (pcUnitData->GetAnimation() != EAnimationType::ANIMATIONTYPE_Attack &&
				pcUnitData->GetAnimation() != EAnimationType::ANIMATIONTYPE_Skill &&
				pcUnitData->GetAnimation() != EAnimationType::ANIMATIONTYPE_Die)
			{
				int iDistXZ = pcUnitData->sPosition.GetPTDistanceXZ(&pcUnit->dwLastWalkOrRunPos);

				if (iDistXZ < DISTANCE_XY_025)
				{
					pcUnit->iStuckCounter++;
					ss << " | Dist " << iDistXZ << ", Stuck " << pcUnit->iStuckCounter;

					//been stuck for 7 seconds (at similar position)
					if (pcUnit->iStuckCounter >= 6)
					{
						pcUnitData->pcAttackTarget = NULL;
						pcUnitData->pcFocusTarget = NULL;
						pcUnitData->pcFocusingTarget = NULL;
						pcUnitData->iFocusingLastID = -1;
						pcUnitData->dwIgnoreOthersTime = TICKCOUNT + 5000;
						pcUnitData->iTargetMoveCount = 6;
						pcUnitData->iTargetTraceMode = FALSE;

						pcUnitData->SetAngleByPosition(pcUnitData->sSpawnPosition.iX >> 8, pcUnitData->sSpawnPosition.iZ >> 8);

						pcUnitData->sMoveToPosition = Point3D(pcUnitData->sSpawnPosition.iX, pcUnitData->sSpawnPosition.iY, pcUnitData->sSpawnPosition.iZ);

						pcUnit->bHealItself = TRUE;
						pcUnit->bReturningToSpawn = TRUE;
						pcUnit->iStuckCounter = 0;
						pcUnit->dwReturningToSpawnTimeout = TICKCOUNT + 10000; //10 seconds to return.
						pcUnit->sReturningToSpawnPosition = Point3D(pcUnitData->sSpawnPosition.iX, pcUnitData->sSpawnPosition.iY, pcUnitData->sSpawnPosition.iZ);

						pcUnit->dwLastWalkOrRunPos.iX = pcUnitData->sPosition.iX;
						pcUnit->dwLastWalkOrRunPos.iZ = pcUnitData->sPosition.iZ;

						pcUnitData->NextTarget(pcUnitData->sSpawnPosition.iX, pcUnitData->sSpawnPosition.iY, pcUnitData->sSpawnPosition.iZ);

						if (!pcUnitData->Animate(ANIMATIONTYPE_Running))
							pcUnitData->Animate(ANIMATIONTYPE_Walking);
					}
				}
				else
				{
					pcUnit->dwLastWalkOrRunPos.iX = pcUnitData->sPosition.iX;
					pcUnit->dwLastWalkOrRunPos.iZ = pcUnitData->sPosition.iZ;
					pcUnit->iStuckCounter = 0;
				}
			}
			else
			{
				pcUnit->dwLastWalkOrRunPos.iX = pcUnitData->sPosition.iX;
				pcUnit->dwLastWalkOrRunPos.iZ = pcUnitData->sPosition.iZ;
				pcUnit->iStuckCounter = 0;
			}
		}


		if (pcUnitData->pcFocusTarget && iSpawnDistXZ <= pcUnitData->iLureDistance && pcUnitData->dwIgnoreOthersTime < TICKCOUNT)
		{
			ss << " | Setting next target";

			pcUnitData->SetAngleByPosition(pcUnitData->pcFocusTarget->sPosition.iX >> 8, pcUnitData->pcFocusTarget->sPosition.iZ >> 8);
			pcUnitData->NextTarget(pcUnitData->pcFocusTarget->sPosition.iX, pcUnitData->pcFocusTarget->sPosition.iY, pcUnitData->pcFocusTarget->sPosition.iZ);
		}
	}

	return TRUE;
}


void UnitServer::UpdateCharacterData(User* pcUser, UnitData* pcUnitData)
{
	if (pcUnitData && pcUser)
	{
		PacketUnitInfo sPacket{};
		sPacket.iHeader = PKTHDR_CharacterData;
		sPacket.iLength = sizeof(PacketUnitInfo);
		sPacket.iID = pcUnitData->iID;
		sPacket.sPosition = pcUnitData->sPosition;
		CopyMemory(&sPacket.sCharacterData, &pcUnitData->sCharacterData, sizeof(CharacterData));
		SENDPACKET(pcUser, &sPacket, TRUE);
	}
}


BOOL UnitServer::IsStuckPreventionBossUnit( UnitData * pcUnitData )
{
	if (pcUnitData && pcUnitData->sCharacterData.sMonsterClass == EMonsterClass::Boss)
	{
		switch (pcUnitData->sCharacterData.iMonsterEffectID)
		{
			case MONSTEREFFECTID_CHAOSCARA:   //Chaos Cara Queen
			case MONSTEREFFECTID_DEATHKNIGHT: //Valento
			case MONSTEREFFECTID_KELVEZU:
			case MONSTEREFFECTID_MOKOVA:
			//case MONSTEREFFECTID_GOBLINSHAMAN:
			//case MONSTEREFFECTID_GORGON:
			case MONSTEREFFECTID_TULLA:
			case MONSTEREFFECTID_DevilShy:
			case MONSTEREFFECTID_Yagditha:
			case MONSTEREFFECTID_Draxos:
			case MONSTEREFFECTID_Greedy:
			case MONSTEREFFECTID_FLAMEMAIDEN:
			case MONSTEREFFECTID_S_F_ELEMENTAL:
			case MONSTEREFFECTID_TEMPLEGUARD:
			case MONSTEREFFECTID_BABEL:
			case MONSTEREFFECTID_Hagios:
			case MONSTEREFFECTID_Tyrcus:

				return TRUE;

			default:
			{
				//otherwise all bosses from map level 90 onwards will be considered for 'stuck' prevention
				if (pcUnitData->pcMap->pcBaseMap->iLevel >= 84)
				{
					return TRUE;
				}
			}
		}


	}

	return FALSE;
}

AttackDamageData * UnitServer::GetUserInDamageList( UnitData * pcUnitData, User * pcUser )
{
	AttackDamageData * psRet = NULL;

	if ( pcUnitData->psaDamageUsersData )
	{
		for ( int i = 0; i < ATTACK_DAMAGE_MAX; i++ )
		{
			AttackDamageData * p = pcUnitData->psaDamageUsersData + i;

			if ( p && p->iID == pcUser->GetID() )
			{
				psRet = p;
				break;
			}
		}
	}

	return psRet;
}

//This packet is only used for kills towards quest
void UnitServer::ProcessPacketKillUnitID( User * pcUser, int iMonsterID, int iKillerID )
{
	MonsterKill sPacketOld;
	sPacketOld.iLength = sizeof( MonsterKill );
	sPacketOld.iHeader = PKTHDR_MonsterKill;
	sPacketOld.iMonsterID = iMonsterID;
	sPacketOld.iKillerID = iKillerID;
	sPacketOld.SParam = 0;
	sPacketOld.EParam = 0;
	SENDPACKET( pcUser, &sPacketOld );
}

#define TRANS_PLAYDATA_EVENT_EXT 3

void UnitServer::PlayBuffDataAddDebuff( Unit * pcUnit, PacketPlayData * psPacket )
{
	PacketBuffData * psPacketBuff = (PacketBuffData *)&( (BYTE *)psPacket )[psPacket->iLength];
	ZeroMemory( psPacketBuff, sizeof( PacketBuffData ) );

	psPacketBuff->iLength = sizeof( PacketBuffData );
	psPacketBuff->iHeader = PTKHDR_Effect_Debuff;

	if ( pcUnit->bPoisonCount )
	{
		psPacketBuff->Debuff.bPoison = pcUnit->bPoisonStackCount;
	}

	if ( pcUnit->bBurningCount )
	{
		psPacketBuff->Debuff.bBurning = TRUE;
	}

	if ( pcUnit->pcUnitData->bDistortion )
	{
		psPacketBuff->Debuff.bDistortion = TRUE;
		psPacketBuff->Debuff.iDamageReduction = pcUnit->pcUnitData->bDamageReduction;
	}

	if ( pcUnit->pcUnitData->iIceOverlay )
	{
		psPacketBuff->Debuff.bIceOverlay = TRUE;
	}

	if ( pcUnit->pcUnitData->iSlowTimeLeft )
	{
		psPacketBuff->Debuff.iStunCount = pcUnit->pcUnitData->iSlowTimeLeft;
	}

	if ( pcUnit->bCursed )
	{
		psPacketBuff->Debuff.bCurse = pcUnit->bCursed; //note - this can be 1 or 2
		psPacketBuff->Debuff.iDamageReduction = pcUnit->pcUnitData->bDamageReduction;
	}

	psPacket->bEventInfo[TRANS_PLAYDATA_EVENT_EXT]++;

	psPacket->iLength += sizeof( PacketBuffData );
	psPacketBuff++;
}


void UnitServer::OnHandleUnitDataBufferNew( Unit * pcUnit, PacketPlayData * psPacket )
{
	psPacket->sHPLong.iMin = pcUnit->pcUnitData->GetCurrentHealth();
	psPacket->sHPLong.iMax = pcUnit->pcUnitData->GetMaxHealth();
	psPacket->sMapID = pcUnit->GetMapID();
	pcUnit->sHPLong = psPacket->sHPLong;
	pcUnit->pcUnitData->sCharacterData.sMapID = pcUnit->GetMapID();

	PlayBuffDataAddDebuff( pcUnit, psPacket );
}

void UnitServer::UpdateUnit( Unit * pcUnit )
{
	UnitData * pcUnitData = pcUnit->pcUnitData;

	WRITEDWORD( 0x07AC5658, pcUnitData );

	BOOL unitIsBoss = pcUnitData->sCharacterData.sMonsterClass == EMonsterClass::Boss;
	BOOL unitIsBossMinion = pcUnitData->PartyFlag == 5;
	BOOL unitIsWolverine = pcUnitData->sCharacterData.iMonsterEffectID == MONSTEREFFECTID_WOLVERLIN && pcUnitData->pcOwner == NULL;
	BOOL unitIsTest = pcUnitData->bTestMapUnit;
	BOOL unitIsHuntEvent = EVENTSERVER->IsEventHuntMonster ( pcUnitData );

	//BOSS_TIME is a random integer value between 0 and 45
	//and we will allow 10 minutes of sending unit status
	// before we stop. It's to keep it alive.
	// See also userserver.cpp
	int minSinceBoss = (GetServerTime()->wMinute - BOSS_TIME);
	int iTimeout = ((unitIsBoss || unitIsBossMinion) && pcUnit->GetMapLevel() >= 98) ? 20 : 5; //LT onwards

	//Keep boss alive for 5 minutes in each map
	// and keep wolverines alive for duration of night
	BOOL unitForceAlive = ((unitIsBoss || unitIsBossMinion ) && minSinceBoss >= 0 && minSinceBoss <= iTimeout) ||
						  (unitIsWolverine && MAPSERVER->IsNight()) ||
						  (unitIsTest) ||
						  (unitIsHuntEvent && EVENTSERVER->IsHuntEventActive());

	//force keep alive
	if (pcUnitData->iAliveCounter == 0 && unitForceAlive)
	{
		pcUnitData->iAliveCounter = 3;
	}

	if ( (pcUnitData->iAliveCounter != 0 || unitForceAlive) ||
		(pcUnitData->GetAnimation() == ANIMATIONTYPE_Die) ||
		(pcUnitData->sUnitInfo.eSpecialType != SPECIALUNITTYPE_None) )
	{
		UpdateUnitData( pcUnit->pcUnitData );

		pcUnitData->iAliveCounter--;
	}
	else
	{
		if ( pcUnitData->sCharacterData.iType == CHARACTERTYPE_Monster )
		{
			pcUnitData->iDeathCounter--;

			if ( pcUnitData->iDeathCounter <= 0 )
			{
				pcUnitData->Free();

				pcUnitData->pcMap->iNumAliveUnitData--;
				MAPSERVER->DelMonsterSpawn( pcUnitData->pcMap, pcUnitData );
			}
		}
	}
}


void UnitServer::Update()
{
	//Only GameServers
	if ( !GAME_SERVER )
		return;

	static UINT iUnitWheel = 0;
	static UINT iActiWheel = 0;

	//Check if action wheel must be set
	//UNIT_DIRTY_WINDOW_INTERVAL (server.ini [Performance]) controls how often
	//the status-dirty window opens. Default 64 frames (~1s) matches the
	//original behaviour; lower values refresh idle units more often.
	//Note this is only the *baseline* refresh - HP changes now flag units
	//immediately via UnitData::MarkStatusDirty().
	if ( iActiWheel == 0 )
	{
		if ( (iUnitWheel % (UINT)UNIT_DIRTY_WINDOW_INTERVAL) == 0 )
			iActiWheel = 4;
	}
	else
		iActiWheel--;

	//Write global unit vars
	WRITEDWORD( 0x008B8D18, GetTickCount() );
	WRITEDWORD( 0x07AC9D68, iActiWheel );

	uUnitsInGame = 0;
	for ( UINT i = 0; i < MAX_UNITS; i++ )
	{
		Unit * pcUnit = pcaUnit + i;
		UnitData * pcUnitData = pcUnit->pcUnitData;

		if ( pcUnit->iID != pcUnitData->iID )
			pcUnit->Load();

		if ( pcUnitData->bActive )
		{
			pcaUnitInGame[uUnitsInGame] = pcUnit;
			uUnitsInGame++;

			//Frame Check
			if ( (i % 4) == (iUnitWheel % 4) )
			{
				//Update Unit
				UpdateUnit( pcUnit );

				//Status Update?
				if ( iActiWheel != 0 )
					pcUnit->uLastUpdate++;
			}
		}
	}

	iUnitWheel++;
}

int UnitServer::IsFreezedUnitFollow( UnitData * pcUnitData )
{
	//Return 0 = jump all, 1 = can die, 2 = follow

	if ( pcUnitData->pcOwner == NULL )
		return 0;

	if ( pcUnitData->iPetID == 0 )
	{
		if ( pcUnitData->lExclusiveTargetID )
		{
			Map * pcMap = MAPSDATA + MAPID_T5QuestArena;

			if ( pcMap )
			{
				if ( (pcUnitData->pcMap == pcMap) && (pcUnitData->lExclusiveTargetID == pcUnitData->pcOwner->iID) )
					return 0;
			}
		}

		//Follow
		if ( pcUnitData->sCharacterData.iMonsterType == MONSTERTYPE_Summon )
			return 2;

		//Follow
		if ( pcUnitData->sCharacterData.iType == CHARACTERTYPE_NPC )
			return 2;

		return 0;
	}

	if ( pcUnitData->pcOwner->pcSocketData == NULL )
		return 0;

	if ( pcUnitData->GetCurrentHealth() <= 0 )
		return 0;

	if ( pcUnitData->pcOwner->pcPetData != pcUnitData )
	{
		//Die
		if ( pcUnitData->iPetID != 1 )
			return 1;
	}

	//Follow
	return 2;
}

BOOL UnitServer::MainServerUnitData( UnitData * pcUnitData )
{
	//Used to Skip Frames

	switch ( pcUnitData->eActionPattern )
	{
		case ACTIONMODE_ServerMonster:

			break;
	}

	return 0;
}

//Line 5012 in leaked source code
//character.cpp - int smCHAR::EventAttack(int Flag) case 5:	//monster
//ActionPattern = 5 = UPDATEMODE_ServerMonster
//This code doesn't work at the moment. The monster doesn't
//attack players.
BOOL UnitServer::MainUnitMonsterData( UnitData * pcUnitData )
{
	//Frames
	int iMovementSpeed	= 0;
	int iFrameSpeed		= 0;

	if ( pcUnitData->iSlowTimeLeft )
	{
		if ( pcUnitData->iPlaySlowSpeed )
		{
			iMovementSpeed	= (pcUnitData->iPlaySlowSpeed * SERVER_CHAR_SKIP) >> 8;   //64 for each iteration of movement speed reduction. 64 = 1 (slowest), 128 = 2 etc.  256 = 4
			iFrameSpeed		= (pcUnitData->iPlaySlowSpeed * SERVER_CHAR_FRAME_STEP) >> 8; //256 slow speed * 320 >> 8 = 320 (The normal speed)
		}

		//half
		else
		{
			iMovementSpeed	= SERVER_CHAR_SKIP >> 1; //2
			iFrameSpeed		= SERVER_CHAR_FRAME_STEP >> 1; //160
		}
	}
	else
	{
		iMovementSpeed	= 4; //4
		iFrameSpeed		= SERVER_CHAR_FRAME_STEP; //320
	}

	//Special (No Movement?)
	if ( pcUnitData->GetAnimation() == ANIMATIONTYPE_Special )
	{
		pcUnitData->iFrame += iFrameSpeed;

		if ( pcUnitData->psModelAnimation )
		{
			if ( pcUnitData->iFrame > (pcUnitData->psModelAnimation->iEndFrame * 160) )
			{
				pcUnitData->Free();
				return TRUE;
			}
		}

		//Save Frames
		CALLT( 0x004172C0, (DWORD)pcUnitData );
		pcUnitData->iFrameCounter += 3;
	}

	//No Move? No speed...
	if ( pcUnitData->bNoMove )
		iMovementSpeed = 0;

	//Babel? Set Delay to spawn...
	//Maintaining the appearance of Babel
	if ( pcUnitData->sCharacterData.iMonsterEffectID == MONSTEREFFECTID_BABEL )
		WRITEDWORD( 0x07AC9D54, TICKCOUNT + 5000 );

	//Have Animation and speed?
	if ( iMovementSpeed && pcUnitData->GetAnimation() )
	{
		//Running or Walking?
		if ( (pcUnitData->GetAnimation() == ANIMATIONTYPE_Walking) || (pcUnitData->GetAnimation() == ANIMATIONTYPE_Running) )
		{
			//Save Old Position...
			Point3D sOldPosition = pcUnitData->sPosition;

			//Running
			BOOL bRunning = (pcUnitData->GetAnimation() == ANIMATIONTYPE_Running) ? TRUE : FALSE;

			//Movement Based on IQ
			if ( pcUnitData->saMoveStepCount[0] )
			{
				int iSpeed = iMovementSpeed;

				//Manual setting of moving step
				if ( pcUnitData->saMoveStepCount[1] )
					iSpeed *= pcUnitData->saMoveStepCount[1];

				//Running? Double Speed...
				if ( bRunning )
					iSpeed *= 2;

				//Move...
				for ( int i = 0; i < pcUnitData->saMoveStepCount[0]; i++ )
					pcUnitData->MoveAngle ( iSpeed );
			}
			else
			{
				//Follow Target
				BOOL bFollowTarget = FALSE;

				//Set Monster Speed
				switch ( pcUnitData->sCharacterData.iMonsterEffectID )
				{
					case MONSTEREFFECTID_DEATHKNIGHT:
						if ( pcUnitData->sCharacterData.sMonsterClass == EMonsterClass::Boss )
							iMovementSpeed *= 2;
						break;

					case MONSTEREFFECTID_GREVEN:
						if ( bRunning )
						{
							if ( pcUnitData->pcFocusTarget )
								bFollowTarget = TRUE;

							iMovementSpeed *= 5;
						}
						break;

					default:
						break;
				}

				//Follow?
				if ( pcUnitData->pcFocusTarget && bFollowTarget )
					pcUnitData->SetAngleByPosition( pcUnitData->pcFocusTarget->sPosition.iX >> 8, pcUnitData->pcFocusTarget->sPosition.iZ >> 8 );

				//Move
				pcUnitData->MoveAngle( iMovementSpeed );
			}
		}
		else
		{
			if ( pcUnitData->GetAnimation() != ANIMATIONTYPE_Idle )
				pcUnitData->Animate( ANIMATIONTYPE_Idle );
		}
	}

	//Update Frame
	pcUnitData->iFrame += iFrameSpeed;

	BOOL bSpawnBegin = FALSE;

	if ( pcUnitData->iFrame > (pcUnitData->psModelAnimation->iEndFrame * 160) )
	{
		//Die?
		if ( pcUnitData->GetAnimation() == ANIMATIONTYPE_Die )
		{
			//End Frame
			pcUnitData->iFrame = pcUnitData->psModelAnimation->iEndFrame * 160;
		}
		else
		{
			//Use Potion?
			if ( (pcUnitData->GetAnimation() == ANIMATIONTYPE_Potion) && pcUnitData->iPotionStatus == POTIONSTATUS_HP )
			{
				//Use virtual potion, restore 1/4 of hp
				pcUnitData->GiveHealth( pcUnitData->GetMaxHealth() / 4 );
			}

			//???
			if ( pcUnitData->sUnitInfo.iSkillType && (pcUnitData->sCharacterData.iMonsterType != MONSTERTYPE_Summon) )
			{
				if ( pcUnitData->GetAnimation() == ANIMATIONTYPE_Attack )
					pcUnitData->pcFocusTarget = NULL;

				//Have Target Monster and use skill? poison it
				if ( pcUnitData->pcTarget && (pcUnitData->GetAnimation() == ANIMATIONTYPE_Skill) )
				{
					pcUnitData->pcTarget->sSuccubus[0] = Dice::RandomI( 10, 20 );
					pcUnitData->pcTarget->sSuccubus[1] = 16 * 20;
					pcUnitData->pcTarget = NULL;
				}
			}

			if ( (pcUnitData->pcFocusTarget || pcUnitData->iTargetMoveCount) && (pcUnitData->GetAnimation() < ANIMATIONTYPE_Attack) )
			{
				DEBUG("pcUnitData->pcFocusTarget = %s", CHARACTERSERVER->GetCharacterName(pcUnitData->pcFocusTarget));

				pcUnitData->iMoveBeginCounter++;
			}

			//Repeat Animation?
			//Running / Walking / Idle
			if ( pcUnitData->psModelAnimation->bLoop )
			{
				//Repeat
				pcUnitData->iFrame = (pcUnitData->psModelAnimation->iBeginFrame * 160) + (pcUnitData->iFrame - (pcUnitData->psModelAnimation->iEndFrame * 160));
			}
			else
			{
				if ( (pcUnitData->GetAnimation() == ANIMATIONTYPE_Flinch) && !pcUnitData->iStunTimeLeft && !pcUnitData->iStopMotionCounter &&
					pcUnitData->Animate( ANIMATIONTYPE_Walking ) )
				{
					pcUnitData->iTargetMoveCount = 2;
				}
				else
					pcUnitData->Animate( ANIMATIONTYPE_Idle );

				pcUnitData->iAutoMoveStep = 1;
			}

			//When Spawn
			if ( pcUnitData->iMoveBeginCounter > 4 )
			{
				int iX = (pcUnitData->sPosition.iX - pcUnitData->sMoveBeginPosition.iX) >> 8;
				int iZ = (pcUnitData->sPosition.iZ - pcUnitData->sMoveBeginPosition.iZ) >> 8;

				int iDistance = (iX * iX) + (iZ * iZ);

				pcUnitData->sMoveBeginPosition.iX = pcUnitData->sPosition.iX;
				pcUnitData->sMoveBeginPosition.iZ = pcUnitData->sPosition.iZ;
				pcUnitData->iMoveBeginCounter = 0;


				//5 x 5 (was 32 vs 32)
				if ( iDistance < (5 * 5) )
				{
					//std::cout << "iDistance < (5 * 5) == " << iDistance << " < " << (5 * 5) << std::endl;

					pcUnitData->SetTargetPosi( pcUnitData->sSpawnPosition.iX >> 8, pcUnitData->sSpawnPosition.iZ >> 8 );
					pcUnitData->iTargetTraceMode = 0;

					int iAngle = (rand() & PTANGLE_90) - PTANGLE_45;
					pcUnitData->sAngle.iY = (pcUnitData->sAngle.iY + iAngle) & PTANGLE_Mask;

					pcUnitData->iTargetMoveCount = 10;

					pcUnitData->bCheckCollision = FALSE;

					pcUnitData->sMoveToPosition = Point3D( 0, 0, 0 );

					if ( !pcUnitData->Animate( ANIMATIONTYPE_Idle ) )
						pcUnitData->Animate( ANIMATIONTYPE_Walking );

					bSpawnBegin = TRUE;
				}
			}

			//Not Spawn Time?
			if ( bSpawnBegin == FALSE )
			{
				DEBUG("bSpawnBegin == FALSE");

				if ( pcUnitData->iTargetMoveCount > 0 )
				{
					pcUnitData->iTargetMoveCount--;
					if ( pcUnitData->iTargetMoveCount <= 0 )
						pcUnitData->Animate( ANIMATIONTYPE_Idle );
				}

				if ( pcUnitData->iLureDistance && ((pcUnitData->GetAnimation() == ANIMATIONTYPE_Walking) || (pcUnitData->GetAnimation() == ANIMATIONTYPE_Running)) )
				{

					int iX = (pcUnitData->sPosition.iX - pcUnitData->sSpawnPosition.iX) >> 8;
					int iZ = (pcUnitData->sPosition.iZ - pcUnitData->sSpawnPosition.iZ) >> 8;
					int iDistance = (iX * iX) + (iZ * iZ);



					if ( iDistance < pcUnitData->iLureDistance )
					{
						DEBUG("IS WITHIN - iLureDistance = %d, iDistance = %d", pcUnitData->iLureDistance, iDistance);

						pcUnitData->SetTargetPosi( pcUnitData->sSpawnPosition.iX >> 8, pcUnitData->sSpawnPosition.iZ >> 8 );
						pcUnitData->sMoveToPosition.iX = 0;
						pcUnitData->sMoveToPosition.iZ = 0;
						pcUnitData->iTargetMoveCount = 6;
						pcUnitData->iTargetTraceMode = 0;
					}
					else
					{
						DEBUG("IS OUTSIDE - iLureDistance = %d, iDistance = %d", pcUnitData->iLureDistance, iDistance);
					}
				}

				if ( !pcUnitData->iTargetMoveCount && pcUnitData->sMoveToPosition.iX  && pcUnitData->sMoveToPosition.iZ )
				{

					DEBUG("pcUnitData->iTargetMoveCount = %d", pcUnitData->iTargetMoveCount);

					pcUnitData->SetTargetPosi( pcUnitData->sMoveToPosition.iX >> 8, pcUnitData->sMoveToPosition.iZ >> 8 );
					pcUnitData->iTargetTraceMode = TRUE;

					//Movement based on IQ
					if ( pcUnitData->iAutoMoveStep <= 0 )
					{
						if ( pcUnitData->GetAnimation() == ANIMATIONTYPE_Idle )
						{
							pcUnitData->iAutoMoveStep = pcUnitData->sUnitInfo.iIntelligence - 5;
							if ( pcUnitData->iAutoMoveStep < 0 )
								pcUnitData->iAutoMoveStep = 0;

							int iX = (pcUnitData->sPosition.iX - pcUnitData->sMoveToPosition.iX) >> 8;
							int iZ = (pcUnitData->sPosition.iZ - pcUnitData->sMoveToPosition.iZ) >> 8;
							int iDistance = (iX * iX) + (iZ * iZ);

							if ( (pcUnitData->sUnitInfo.iIntelligence >= 6) && (iDistance < (64 * 8 * 64 * 8)) )
							{
								if ( pcUnitData->Animate( ANIMATIONTYPE_Running ) )
									pcUnitData->iAutoMoveStep <<= 2;
								else
									pcUnitData->Animate( ANIMATIONTYPE_Walking );
							}
							else
								pcUnitData->Animate( ANIMATIONTYPE_Walking );
						}
						else
							pcUnitData->Animate( ANIMATIONTYPE_Idle );
					}
					else
					{
						pcUnitData->SetAngleByPosition( pcUnitData->sMoveToPosition.iX >> 8, pcUnitData->sMoveToPosition.iZ >> 8 );
						pcUnitData->iAutoMoveStep--;
					}

					pcUnitData->sMoveToPosition = Point3D( 0, 0, 0 );
					DEBUG("Move to position 0, 0, 0");
				}

				if ( pcUnitData->sUnitInfo.iMaxPotions && pcUnitData->GetCurrentHealth() > 0 )
				{
					//current hp is 1/4
					if ( pcUnitData->GetHealthRemainingPercentage() < 25 )
					{
						//iPerPotions determines chance to succeed
						if ( pcUnitData->sUnitInfo.iPerPotions < Dice::RandomI( 0, 99 ) )
						{
							if ( pcUnitData->sCharacterData.iMonsterEffectID == MONSTEREFFECTID_NPC_MAGICMASTER )
							{
								PacketSimple sPacket;
								sPacket.iLength = sizeof( PacketSimple );
								sPacket.iHeader = PKTHDR_SkillCastData;
								sPacket.iUnk = 0x71;
								sPacket.iUnk2 = pcUnitData->iID;

								if ( pcUnitData->pcExclusiveTarget )
									SENDPACKET( USERDATATOUSER( pcUnitData->pcExclusiveTarget ), &sPacket );

								//Use virtual potion, restore 1/4 of hp
								pcUnitData->GiveHealth( pcUnitData->GetMaxHealth() / 4 );
							}
							else
							{
								pcUnitData->Animate( ANIMATIONTYPE_Potion );
								pcUnitData->iPotionStatus = POTIONSTATUS_HP;
							}
						}

						pcUnitData->sUnitInfo.iMaxPotions--;
					}
				}

				if ( pcUnitData->eNextMotionCode )
				{
					if ( pcUnitData->Animate( pcUnitData->eNextMotionCode ) == FALSE )
						pcUnitData->Animate( ANIMATIONTYPE_Idle );

					pcUnitData->eNextMotionCode = ANIMATIONTYPE_None;
				}

				if ( pcUnitData->sCharacterData.iMonsterEffectID == MONSTEREFFECTID_DEADHOPT )
				{
					if ( pcUnitData->GetCurrentHealth() > 0 )
					{
						if ( (pcUnitData->GetHealthRemainingPercentage() < 30) || (pcUnitData->iHits > 80) )
						{
							pcUnitData->Animate( ANIMATIONTYPE_Special );
							pcUnitData->pcFocusTarget = NULL;
						}
					}
				}
			}
		}
	}

	if ( pcUnitData->DontMoveFlag == 2 )
		pcUnitData->sAngle = Point3D( 0, 0, 0 );

	//Save Frames
	CALLT( 0x004172C0, (DWORD)pcUnitData );

	return 0;
}

