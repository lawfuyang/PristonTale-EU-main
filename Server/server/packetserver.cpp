#include "stdafx.h"
#include "packetserver.h"
#include "HNSSkill.h"
#include <iostream>
#include <CStable.h>


CRITICAL_SECTION CriticalSectionPacketSend;
BOOL bPacketCritical = FALSE;



#define TRANS_RECORD_LEN (8192 - 32)
extern BOOL __cdecl BuffReceive( User * pcUser, PacketSkillPartyData * pPacket );

extern uint32_t hash_data(char * p_Data, std::size_t p_Size);

PacketServer::PacketServer()
{
}

PacketServer::~PacketServer()
{
}

BOOL PacketServer::AnalyzePacket( User * pcUser, PacketReceiving * p )
{
	Packet * psPacket = (Packet*)p->baPacket;
	UserData * pcUserData = pcUser->pcUserData;

	auto l_FNFowardPacketToBinary = [](Packet * psPacket, UserData* p_pcUserData) {
		/* TEMPORARILY */
		char packettemp[0x202C];
		char * pointertotmp = packettemp;
		int len = psPacket->iLength;

		if (len > MAX_PKTSIZ)
			return TRUE;

		memcpy(packettemp + 0x2C, psPacket, len);

		WRITEDWORD(packettemp, (DWORD)p_pcUserData->pcSocketData);
		WRITEDWORD((DWORD)(packettemp + 4), (DWORD)p_pcUserData);

		CALL_WITH_ARG1(0x0056F300, (DWORD)packettemp);
	};

	/// Filter NetMessage here (inter server comms)
	if ((psPacket->iHeader & 0xFF000000) == 0x7F000000)
	{
		NETSERVER->OnReceivePacket(pcUser, psPacket);
	}

#if defined (DEV_MODE) && defined (PACKET_LOG)

	if (!PACKETSERVER->IsPacketIgnored((Packet*)psPacket))
	{
		const std::string extraInfo = PACKETSERVER->GetExtraInfoForPacket((Packet*)psPacket);
		const std::string & l_OpcodeName = SOCKETSERVER->GetOpcodeName(psPacket->iHeader);

		std::string charName = CHARACTERSERVER->GetCharacterName( pcUserData );

		if ( charName.empty() )
			charName = pcUserData->pcSocketData->szIP;

		//ignore inter server comms, it is already logged elsewhere (in NETSERVER->OnReceivePacket)
		if ( !pcUser->bNetServer )
		{
			if ( !l_OpcodeName.empty() )
			{
				if ( !extraInfo.empty() )
				{
					DEBUGPACKET( "[ SERVER <-- '%s' ] %s -- %s", charName.c_str(), l_OpcodeName.c_str(), extraInfo.c_str() );
				}
				else
				{
					DEBUGPACKET( "[ SERVER <-- '%s' ] %s", charName.c_str(), l_OpcodeName.c_str() );
				}
			}
			else
			{
				char hexString[16];
				_itoa_s( static_cast<DWORD>( psPacket->iHeader ), hexString, 16 );
				WARN( "[ SERVER <-- '%s' ] Unknown Opcode. Please add packet 0x%s to ADD_OPCODE_NAME(..)", charName.c_str(), hexString);
			}
		}
	}

#endif

	bool l_IsBeforeAuthPacket = false;

	switch (psPacket->iHeader)
	{
		case PKTHDR_LoginUser:
		case PKTHDR_Version:
		case PKTHDR_KeySet:
		case PKTHDR_Ping:
		case PKTHDR_PlayDataEx:
			l_IsBeforeAuthPacket = true;
			break;
	}

	/// Inter server exception
	if (pcUserData->pcSocketData->GetSocketDataContext()->InterServerAuthed)
		l_IsBeforeAuthPacket = true;

	//if (!pcUser->PlayerAuthed && !l_IsBeforeAuthPacket)
	//{
	//	std::string charName = CHARACTERSERVER->GetCharacterName( pcUserData );
	//
	//	if ( charName.empty() )
	//		charName = pcUserData->pcSocketData->szIP;
	//
	//	//INFO ( "PKTHDR_WorldLoginAuth OK: %s", charName.c_str() );
	//	LOGSERVER->OnLogEx( USERDATATOUSER(pcUserData), ACCLOGID_CharacterLoad, "[%s][%s] Packet spoofing", (GAME_SERVER ? "GS" : "LS"), charName.c_str() );
	//
	//	LOGERROR( "[PLAYER] Packet spoofing %s", pcUserData->pcSocketData->szIP );
	//
	//	if ( pcUserData->iGameLevel != EGameLevel::GAMELEVEL_Four )
	//	{
	//		NETSERVER->DisconnectUser( pcUserData );
	//
	//		SENDPACKETBLANK( USERDATATOUSER( pcUserData ), PKTHDR_Disconnect );
	//		WNDPROC_SOCKETCLOSE( SOCKETSERVER->GetHWND(), pcUserData->pcSocketData );
	//	}
	//}

	/// Disabled packets
	switch (psPacket->iHeader)
	{
		case PKTHDR_OpenManufacture:
		case PKTHDR_ManufactureItem:
			break;

		default:
			break;
	}

    int l_OldGold = pcUserData ? pcUserData->GetGold() : 0;

	switch ( psPacket->iHeader )
	{
		case PKTHDR_Ping:
			if ( ((PacketPing*)psPacket)->iLength != sizeof( PacketPing ) )
				break;

			((PacketPing*)psPacket)->dwTick = GetTickCount();
			SENDPACKET( pcUser, psPacket, TRUE );

			if ( pcUserData->dwNoticeCounter < READDWORD( 0x07AC9BF0 ) )
				CALL_WITH_ARG1( 0x00550930, (DWORD)pcUserData );
			//printf("Cheat engine %u %s\n", ((PacketPing*)psPacket)->CheatEngineDetected, pcUserData->szCharacterName);
			// ALL CHEAT DETECTION DISABLED for local offline play
			#if 0
			if (LOGIN_SERVER && ((PacketPing *)psPacket)->CheatEngineDetected && strlen(pcUserData->szCharacterName) > 0)
			{
				//printf("Try ban\n");
				UserData * pcUserBanned = USERSERVER->GetUserdata(pcUserData->szCharacterName);
				if (pcUserBanned)
				{
					//printf("Ban\n");
					PacketLogCheatEx sPacket;
					sPacket.iCheatID = CHEATLOGID_GMReason;
					STRINGCOPY(sPacket.szParam, "AUTOMATIC - CheatEngine detected");
					LOGSERVER->OnLogCheat(pcUserBanned, &sPacket);
				}
			}
			#endif

			break;

		case PKTHDR_Client_Error:
		{
			if ( GAME_SERVER )
				return TRUE;

			PacketTransCommand * lpCmd = reinterpret_cast<PacketTransCommand *>( psPacket );

#ifdef DEV_MODE

			if (lpCmd->LParam != 0 || lpCmd->SParam != 0 )
				WARN( "Client energy check failed. Life = %d, Mana = %d, Stamina %d", lpCmd->LParam, lpCmd->SParam, lpCmd->EParam );
#endif

			//Unsure what this is. Seen this in testing too

			break;
		}
		case PKTHDR_Crash:
			CHEATSERVER->HandlePacket( pcUser, (PacketCrash*)psPacket );
			break;

		case PKTHDR_CrashData:
			CHEATSERVER->HandlePacket( pcUser, (PacketCrashData*)psPacket );
			break;

		case PKTHDR_CharacterDataEx:
			CHARACTERSERVER->HandlePacket( pcUser, (PacketCharacterDataEx*)psPacket );
			break;

		case PKTHDR_CreateCharacter:
			CHARACTERSERVER->CharacterCreate( pcUser, (PacketCreateCharacter*)psPacket );
			break;

		case PKTHDR_LoginUser:
			if (GAME_SERVER)
				return TRUE;

			pcUserData->iVerifiedLogin++;

			PacketLoginUser * p;
			p = (PacketLoginUser*)psPacket;

			if (p->iLength != sizeof(PacketLoginUser) || p->iVersion != GAME_VERSION)
			{
				PacketAccountLoginCode s;
				s.iLength		= sizeof(s);
				s.iHeader		= PKTHDR_AccountLoginCode;
				s.dwReserved	= 0;
				s.iCode			= ACCOUNTLOGIN_WrongVersion;
				s.iFailCode		= 1;
				strcpy(s.szMessage, "Game version is incorrect please update\0");

				WARN ( "PKTHDR_AccountLoginCode3 %d for %s (actual: %d vs expected: %d)", (int)ACCOUNTLOGIN_WrongVersion, p->szUserID, p->iVersion, GAME_VERSION );

				SENDPACKET(pcUser, &s, TRUE);
			}
			else
			{
				ACCOUNTSERVER->PHAccountLogin( pcUser, p );
				WRITEDWORD( 0x07AC9D80, READDWORD( 0x07AC9D80 ) + 1 );
				CHEATSERVER->OnLoginUserPacket( pcUserData );
			}

			break;
		case PKTHDR_SaveData:
        {

			if (GAME_SERVER)
				return TRUE;

            struct	TRANS_RECORD_DATAS : Packet
            {
                int	Count, Total;
                int	TransSize;
                char	szData[8192 - 32];
            };

            struct TempCharTransPacket : Packet
            {
                char	szHeader[8];
                struct  CharacterData CharData;

            };

			ACCOUNTSERVER->SQLCharacterSave( pcUserData );



            TRANS_RECORD_DATAS * l_Trans = reinterpret_cast<TRANS_RECORD_DATAS *>(psPacket);

            /// Character chunk
            if (l_Trans->Count == 0)
            {
				BLESSCASTLESERVER->SendTopClansSync( pcUser );

                TempCharTransPacket * l_TempPacket = reinterpret_cast<TempCharTransPacket *>(l_Trans->szData);


                static const int s_LevelMax = sizeof(s_TotalExpPerLevel) / sizeof(s_TotalExpPerLevel[0]);

                /// Force packet value for the future handling to avoid packet modifications
                l_TempPacket->CharData.iGold            = pcUserData->GetGold();
                l_TempPacket->CharData.iCurrentExpHigh  = pcUserData->liEXP.HighPart;
                l_TempPacket->CharData.iCurrentExpLow   = pcUserData->liEXP.LowPart;

                int l_Level = 1;
                for (int l_I = 0; l_I < s_LevelMax; ++l_I)
                {
                    if (static_cast<uint64_t>(pcUserData->liEXP.QuadPart) >= s_TotalExpPerLevel[l_I])
                        l_Level = l_I + 1;
                }

                if (l_Level > s_LevelMax)
                    l_Level = s_LevelMax;

                l_TempPacket->CharData.iLevel = l_Level;

				pcUser->pcUserData->sCharacterData.iLevel = l_Level;
				if ( pcUser && pcUser->IsValidAndInGame () )
				{
					CHARACTERSERVER->SQLUpdateCharacterExpAndLevel ( pcUser );
				}

                /// Update checksum
                reinterpret_cast<int(*)(char *)>(0x446050)((char*)&l_TempPacket->CharData);

                const int s_MaxRunSpeed         = 60;
                const int s_MaxAbsorb           = 600;
                const int s_MaxDefense          = 10000;
                const int s_MaxAtkRating        = 10000;
                const int s_MaxAtkPowerMin      = 4500;
                const int s_MaxAtkPowerMax      = 5500;
                const int s_MaxCriticalRating   = 90;
                const int s_MaxBlockRating      = 90;
				const int s_MaxHPRegen			= 500;
				const int s_MaxMPRegen			= 500;
				const int s_MaxSPRegen			= 500;

                /// Check stats hard limit
                if (l_TempPacket->CharData.iMovementSpeed       > s_MaxRunSpeed
                    || l_TempPacket->CharData.iAbsorbRating     > s_MaxAbsorb
                    || l_TempPacket->CharData.iDefenseRating    > s_MaxDefense
                    || l_TempPacket->CharData.iAttackRating     > s_MaxAtkRating
                    || l_TempPacket->CharData.iMinDamage        > s_MaxAtkPowerMin
                    || l_TempPacket->CharData.iMaxDamage        > s_MaxAtkPowerMax
                    || l_TempPacket->CharData.iCritical         > s_MaxCriticalRating
                    || l_TempPacket->CharData.iBlockRating      > s_MaxBlockRating
					|| l_TempPacket->CharData.fHPRegen			> s_MaxHPRegen
					|| l_TempPacket->CharData.fMPRegen			> s_MaxMPRegen
					|| l_TempPacket->CharData.fSPRegen			> s_MaxSPRegen )
                {
                    PacketLogCheat sPacket;
                    sPacket.iCheatID    = CHEATLOGID_StatsHacking;
                    LOGSERVER->OnLogCheat(pcUser->pcUserData, &sPacket);

					WARN( "Stats hacking detected!" );

					if ( pcUser->GetGameLevel() != GAMELEVEL_Four )
					{
						NETSERVER->DisconnectUser( pcUser->pcUserData );

						SENDPACKETBLANK( USERDATATOUSER( pcUser->pcUserData ), PKTHDR_Disconnect );
						WNDPROC_SOCKETCLOSE( SOCKETSERVER->GetHWND(), pcUser->pcUserData->pcSocketData );
					}

					return TRUE;
                }

                /// @TODO Check items
                if (l_Trans->TransSize < TRANS_RECORD_LEN)
                {
					/*PacketCharacterRecordData l_TempPacket2;

					char *lpData;
					lpData = (char *)&l_TempPacket2;

					memcpy( &lpData[l_Trans->Count*TRANS_RECORD_LEN], l_Trans->szData, l_Trans->TransSize );

					if ( l_Trans->Count >= l_Trans->Total - 1 && l_TempPacket2.iHeader == PKTHDR_SaveData )
					{

					}

					DWORD l_LastHash = pcUser->LastInventoryHash;
					DWORD l_CurrentHash = hash_data((char*)&pcUserData->sIntentoryItems, sizeof(DropItemData) * 316);


					l_TempPacket->CharData.
					l_Trans->szData*/
                }
            }

			break;
        }

		case PKTHDR_Version:
		{

			std::string charName = CHARACTERSERVER->GetCharacterName( pcUserData );

			if ( charName.empty() )
				charName = pcUserData->pcSocketData->szIP;

			//INFO( "PKTHDR_Version: %s. Server full: %d, Version: %d", charName.c_str(), ( (PacketVersion *)psPacket )->bServerFull, ( (PacketVersion *)psPacket )->iVersion );
			LOGSERVER->OnLogEx( USERDATATOUSER( pcUserData ), ACCLOGID_CharacterLoad, "[%s][%s] PKTHDR_Version. Server full: %d, Version: %d", (GAME_SERVER ? "GS" : "LS"), charName.c_str(), ( (PacketVersion *)psPacket )->bServerFull, ( (PacketVersion *)psPacket )->iVersion );

		}
		break;

		case PKTHDR_LogCheat:
			LOGSERVER->OnLogCheat( pcUserData, (PacketLogCheat*)psPacket );
			break;

		case PKTHDR_Reconnect:

#ifdef RECONNECT_SYSTEM
			USERSERVER->ReconnectUser( pcUserData, (PacketReconnectUser*)psPacket );
#endif
			return TRUE;
			break;

		case PKTHDR_ChatGame:
			if ( SERVERCOMMAND->OnReceiveChat( pcUser, (PacketChat*)psPacket ) )
				return TRUE;
			break;

		case PKTHDR_ChatItemLink:
			CHATSERVER->HandlePacket( pcUser, (PacketItemLinkChat*)psPacket );
			break;

		case PKTHDR_PlayDataEx:
			CHARACTERSERVER->OnCharacterSyncDataEx( pcUserData, (PacketPlayDataEx*)psPacket );
            return TRUE;

		case PKTHDR_LoseExp:
			CHARACTERSERVER->OnLoseExp( pcUserData, (PacketLoseExperience*)psPacket );
			break;

		case PKTHDR_SaveThrowMoney:
			if (GAME_SERVER)
				return TRUE;

			//std::cout << " player TradeWindowOpen " << pcUser->TradeWindowOpen << std::endl;
			//std::cout << " player character GetGold() " << pcUserData->GetGold() << std::endl;
			//std::cout << " WParam " << ((PacketTransCommand*)psPacket)->WParam << std::endl;
			//std::cout << " PreventMoneyLoss " << pcUser->PreventMoneyLoss << std::endl;

			if (pcUserData->iWarehouseStatus || pcUser->TradeWindowOpen)
				return TRUE;

			if (!pcUserData)
				break;

			if (((PacketTransCommand*)psPacket)->WParam != pcUserData->GetGold())
			{
                PacketSetCharacterGold sPacket;
				sPacket.iHeader = PKTHDR_SetGold;
				sPacket.iLength = sizeof(PacketSetCharacterGold);
				sPacket.dwGold	= pcUserData->GetGold();

				PACKETSERVER->Send(pcUserData, &sPacket);

                pcUserData->iSaveGold                       = pcUserData->GetGold();
                ((PacketTransCommand *)psPacket)->WParam    = pcUserData->GetGold();
            }

			break;

        case PKTHDR_SaveThrowItem2:
        {
			//Login server only
			if ( GAME_SERVER )
				return FALSE;

			if ( pcUser && pcUser->IsValidAndInGame () )
			{
				PacketTransCommandEx * lpTransCommandEx = reinterpret_cast<PacketTransCommandEx *>( psPacket );

				int l_GoldDrop = lpTransCommandEx->ExParam;

				if ( l_GoldDrop < 0 || l_GoldDrop > pcUserData->GetGold () || l_GoldDrop > MAX_DROP_GOLD ( pcUserData->sCharacterData.iLevel ) )
					return TRUE;    ///< Skip the packet

				l_FNFowardPacketToBinary ( psPacket, pcUserData );

				/// Avoid double gold loss
				if ( l_GoldDrop != 0 )
				{
					USERSERVER->AddServerUserGold ( pcUserData, l_GoldDrop, WHEREID_DropItemRestoreOk );
					pcUserData->iSaveGold = pcUserData->GetGold ();

					PacketSetCharacterGold sPacket;
					sPacket.iHeader = PKTHDR_SetGold;
					sPacket.iLength = sizeof ( PacketSetCharacterGold );
					sPacket.dwGold = pcUserData->GetGold ();

					PACKETSERVER->Send ( pcUserData, &sPacket );
				}
			}


            return TRUE;
        }

        case PKTHDR_WingItem:
        {
            PacketWingItem * l_WingItemPacket = reinterpret_cast<PacketWingItem *>(psPacket);

            //std::cout << "Money : " << l_WingItemPacket->Money << std::endl;

            if (pcUserData && (l_WingItemPacket->Money < 0 || l_WingItemPacket->Money > pcUserData->GetGold()))
                return TRUE;
            else
            {
                USERSERVER->SubServerUserGold(pcUserData, l_WingItemPacket->Money, WHEREID_Wings);
                l_WingItemPacket->Money = 0;
            }

            break;
        }

		case PKTHDR_UserOnline:
			SERVERCOMMAND->CheckUserOnline( pcUser, (PacketUserOnline*)psPacket );
			break;

		case PKTHDR_FailRecordData:
		{
			WARN( "Failed to load save data?? (PKTHDR_FailRecordData)" );
			LOGSERVER->OnLogEx( pcUser, ACCLOGID_CharacterLoad, "[%s] PKTHDR_FailRecordData", (GAME_SERVER ? "GS" : "LS"));

			break;
		}

		case PKTHDR_SelectCharacter:
		{
			ACCOUNTSERVER->SelectCharacter( pcUserData, (PacketSelectCharacter *)psPacket );

			//INFO( "PKTHDR_SelectCharacter: %s", ( (PacketSelectCharacter *)psPacket )->szCharName );
			LOGSERVER->OnLogEx( pcUser, ACCLOGID_CharacterLoad, "[%s][%s] PKTHDR_SelectCharacter", (GAME_SERVER ? "GS" : "LS"), ( (PacketSelectCharacter *)psPacket )->szCharName);

			//psPacket->iHeader = 0;
		}
		break;

		case PKTHDR_DeleteCharacter:
			ACCOUNTSERVER->DeleteCharacter( pcUserData, (PacketDeleteCharacter*)psPacket );
			break;

		case PKTHDR_QuestMonsterNameReq:
			QUESTSERVER->HandleQuestMonsterNameRequest( pcUser, ((PacketCommand *)psPacket)->p1 );
			break;

			//Item
		case PKTHDR_BuyItemNPCShop:

			//Login only
			if (GAME_SERVER)
				return TRUE;

			//return here. do not process this packet in ASM (via l_FNFowardPacketToBinary)
			return ITEMSERVER->RecvBuyShopItem ( pcUser, (PacketBuyNPCShop *)psPacket );

			break;

		case PKTHDR_SellItemNPCShop:
			if (GAME_SERVER)
				return TRUE;

			if (!ITEMSERVER->HandlePacket(pcUser, (PacketSellNPCShop*)psPacket))
				return TRUE;

			break;

        case PKTHDR_RepairItemNPCShop:
            if (GAME_SERVER)
                return TRUE;

            if (ITEMSERVER->HandlePacket(pcUser, (PacketRepairNPCShop*)psPacket))
                return TRUE;

            break;

		case PKTHDR_PerfectItemData:
			ITEMSERVER->PHGetPerfectItem( pcUser, (PacketGetItemPerfectView*)psPacket );
			break;

		case PKTHDR_MixItem:
			if ( GAME_SERVER )
			{
				WARN( "MIXING should be done on login server only" ); //because only the login server holds info about player inventory
				return TRUE;
			}
			MIXHANDLER->OnMixingItem( pcUser, (PacketMixServer*)psPacket );
			return TRUE;

		case PKTHDR_SocketItemDataToServer:
			ITEMSERVER->OnSocketItem( pcUser, (PacketSocketSystem*)psPacket );
			break;

		case PKTHDR_ResetItem:
			ITEMSERVER->OnResetItem( pcUser, (PacketResetItem*)psPacket );
			break;

		case PKTHDR_AgingItem:
			if ( GAME_SERVER )
			{
				WARN( "AGING should be done on login server only" ); //because only the login server holds info about player inventory
				return TRUE;
			}

			ITEMSERVER->OnAgingItem( pcUser, (PacketAgingItem*)psPacket );
			break;

		case PKTHDR_ManufactureItem:
			MANUFACTUREHANDLER->HandlePacket( pcUser, (PacketManufactureItem*)psPacket );
			break;

		case PKTHDR_QuestItemSwap:
			QUESTSERVER->HandlePacket( pcUser, (PacketQuestItemSwap*)psPacket );
			break;

		case PKTHDR_UpdateLevelClan:
			CHARACTERSERVER->OnCharacterUpdateLevelClan( pcUser, (PacketUpdateClanLevel*)psPacket );
			break;

		case PKTHDR_UpgradeAgingItem:
			ITEMSERVER->OnUpgradeAgingItem( pcUserData, (PacketItem*)psPacket );
			break;

		case PKTHDR_UpgradeQuestWeapon: //login
			QUESTSERVER->HandleQuestWeaponUpgrade( pcUser, (PacketQuestWeaponMature*)psPacket );
			break;

		case PKTHDR_RecvWarehouse:
			WAREHOUSEHANDLER->OnWarehouseOpened( pcUser, pcUser->pcUserData->iWarehouseStatus == 2 ); //doesn't get called anymore
			break;

		case PKTHDR_SendWarehouse:
			WAREHOUSEHANDLER->HandlePacket( pcUser, (PacketWarehouse*)psPacket);
			break;

		case PKTHDR_NPCItemBox:
			ITEMSERVER->OpenItemBox( pcUser->pcUserData, ((PacketTransCommand*)psPacket)->WParam );
			break;

			//Quest Handlers
		case PKTHDR_QuestNPCAccept:
			QUESTSERVER->HandlePacket( pcUser, (PacketQuestNPCAccept*)psPacket );
			break;
		case PKTHDR_QuestCancel:
			QUESTSERVER->HandlePacket( pcUser, (PacketQuestHandleCancel*)psPacket );
			break;

		case PKTHDR_QuestUpdateData:
			QUESTSERVER->HandlePacket( pcUser, (PacketQuestUpdateData*)psPacket );
			break;

		case PKTHDR_QuestNPCFinish:
			QUESTSERVER->HandlePacket( pcUser, (PacketQuestNPCFinish*)psPacket );
			break;
			//Bellatra
		case PKTHDR_GoldBellatraClan:
			BELLATRASERVER->PHGoldClanRecv( pcUser, (PacketClanGoldReceive*)psPacket );
			break;

        case PKTHDR_OpenPlayerShop:
            /// @LEVEL RESTRICTION
            if (pcUserData->GetLevel() < 10)
            {
                CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> You need to be level 10 to be able to open a player shop.");
                return TRUE;
            }
            break;

		case PKTHDR_QuestCommand:
		{
			QUESTSERVER->HandleQuestCommand( pcUser, (PacketTransCommand *)psPacket );
			psPacket->iHeader = 0;
			return TRUE;
		}
		break;

		case PKTHDR_PlayerShopConfig:
		{
			if (GAME_SERVER)
			{
				NetConnection * l_LoginServerConnection = NETSERVER->GetLoginServerConnection();

				if (l_LoginServerConnection)
					NETSERVER->SendPacket(l_LoginServerConnection->pcSocketData, psPacket);
				else
					return TRUE;
			}
			else if (LOGIN_SERVER)
			{
				int l_CompressedSize = psPacket->iLength - 48;

				struct	sMYSHOP_ITEM {
					Item		sItem;
					int			Price;
					short       Posi[2];
					DWORD		dwTemp[3];
				};
				struct sMYSHOP {
					DWORD			CHAR_CODE;
					DWORD			dwTemp[6];
					sMYSHOP_ITEM	sMyShopItem[30];
				};

#define TRANS_TRADE_BUFF_SIZE		4000
				struct TRANS_TRADEITEMS : Packet {
					DWORD	dwSender;
					DWORD	dwRecver;
					DWORD	dwSum;
					DWORD	Temp[4];

					BYTE	TradeBuff[TRANS_TRADE_BUFF_SIZE];
				};

				TRANS_TRADEITEMS * l_TradeItemPacket = reinterpret_cast<TRANS_TRADEITEMS*>(psPacket);

                UserData * l_ShopOwner = USERSERVER->GetUserdata(l_TradeItemPacket->dwSender);
                UserData * l_Receiver  = USERSERVER->GetUserdata(l_TradeItemPacket->dwRecver);

				sMYSHOP l_MyShop;
				memset(&l_MyShop, 0, sizeof(sMYSHOP));

				if (l_ShopOwner && ITEMSERVER->DecodeItemsData(l_TradeItemPacket->TradeBuff, &l_MyShop, 0x7FFFF))
				{
					for (int l_ItemI = 0; l_ItemI < 30; ++l_ItemI)
					{
						sMYSHOP_ITEM & l_CurrentItem = l_MyShop.sMyShopItem[l_ItemI];
						if (l_CurrentItem.sItem.sItemID.ToItemID())
						{
							//std::cout << "iteratre for " << l_CurrentItem.sItem.sItemID.ToItemID() << " " << l_CurrentItem.Price << std::endl;

							l_ShopOwner->sItemShopList[l_ItemI].eItemID		= l_CurrentItem.sItem.sItemID.ToItemID();
							l_ShopOwner->sItemShopList[l_ItemI].dwHead		= l_CurrentItem.sItem.iChk1;
							l_ShopOwner->sItemShopList[l_ItemI].dwChecksum	= l_CurrentItem.sItem.iChk2;
							l_ShopOwner->sItemShopList[l_ItemI].iPrice		= l_CurrentItem.Price;

//                             if (!ITEMSERVER->IsPotion(l_CurrentItem.sItem.sItemID.ToItemID()))
//                             {
//                                 bool l_ItemFound = ITEMSERVER->HasItemInInventory(l_ShopOwner, l_CurrentItem.sItem.sItemID.ToItemID(), l_CurrentItem.sItem.iChk1, l_CurrentItem.sItem.iChk2);
//
//                                 if (!l_ItemFound)
//                                 {
//                                     SENDPACKETBLANK(USERDATATOUSER(l_ShopOwner), PKTHDR_Disconnect);
//                                     WNDPROC_SOCKETCLOSE(SOCKETSERVER->GetHWND(), l_ShopOwner->pcSocketData);
//
//                                     NETSERVER->DisconnectUser(l_ShopOwner);
//
//                                     return TRUE;
//                                 }
//                             }
						}
					}
				}

				return TRUE;
			}

			break;
		}

		case PKTHDR_PlayerShopTrade:
		{
			PacketTransCommand * l_ShopPacket = reinterpret_cast<PacketTransCommand*>(psPacket);
			UserData * l_OtherL = USERSERVER->GetUserdata(l_ShopPacket->LParam);

			bool l_AllOk = false;
			UserData * l_DCTarget = pcUserData;

			if (l_OtherL)
			{
				struct sMYSHOP_ITEM_SERVER {
					DWORD CODE;
					DWORD CkSum;
					DWORD Head;
					DWORD sTime;
					DWORD Price;
					DWORD SendFlag;
					DWORD Buyer;
					DWORD Temp[4];
				};

				sMYSHOP_ITEM_SERVER * l_Query = reinterpret_cast<sMYSHOP_ITEM_SERVER*>(reinterpret_cast<char*>(l_ShopPacket) + sizeof(PacketTransCommand));

				if (l_Query->SendFlag == 1)
				{
					l_AllOk = true;
				}
				else if (l_Query->SendFlag == 2)
				{
					l_DCTarget = l_OtherL;

					bool l_ItemFound = false;
					int l_ItemI = 0;
					///std::cout << "Looking for " << l_Query->CODE << " " << l_Query->Head << " " << l_Query->CkSum << std::endl;
					for (; l_ItemI < 32; ++l_ItemI)
					{
						SellItemShopData & l_CurrentEntry = pcUserData->sItemShopList[l_ItemI];
						//std::cout << "iteratre for " << l_CurrentEntry.eItemID << " " << l_CurrentEntry.iPrice << " " << l_CurrentEntry.dwHead << " " << l_CurrentEntry.dwChecksum << std::endl;
						if (l_CurrentEntry.eItemID == l_Query->CODE && l_CurrentEntry.dwHead == l_Query->Head && l_CurrentEntry.dwChecksum == l_Query->CkSum)
						{
							l_ItemFound = true;
							break;
						}
					}

					if ( l_ItemFound )
					{
						SellItemShopData & l_Item = pcUserData->sItemShopList[l_ItemI];

						///std::cout << "Buy price : " << l_Item.iPrice << std::endl;
						///std::cout << "count : " << l_Query->sTime << std::endl;
						///std::cout << "packet price : " << l_Query->Price << std::endl;

						int		l_QueryPrice = l_Query->Price;
						int		l_QueryBuyCount = l_Query->sTime;
						int		l_ItemUnitPrice = l_Item.iPrice;
						bool	l_IsIntegerDivider = ( l_QueryPrice % l_ItemUnitPrice ) == 0;
						int		l_BuyQuantity = l_QueryPrice / l_ItemUnitPrice;
						int		l_ExceptedBuyPrice = l_ItemUnitPrice * l_BuyQuantity;

						///std::cout << "!l_IsIntegerDivider : " << !l_IsIntegerDivider << std::endl;
						///std::cout << "l_QueryPrice != l_ExceptedBuyPrice : " << (l_QueryPrice != l_ExceptedBuyPrice) << std::endl;
						///std::cout << "(l_BuyQuantity != 1 && l_BuyQuantity != l_QueryBuyCount) : " << (l_BuyQuantity != 1 && l_BuyQuantity != l_QueryBuyCount) << std::endl;
						///std::cout << "l_Item.iPrice > l_OtherL->GetGold() : " << (l_Item.iPrice > l_OtherL->GetGold()) << std::endl;

						if ( !l_IsIntegerDivider													///< Is unit price multiple of total price
							|| l_QueryPrice != l_ExceptedBuyPrice								///< Is computed price matching packet price
							|| ( l_BuyQuantity != 1 && l_BuyQuantity != l_QueryBuyCount )			///< Is buy quantity ok
							|| l_Item.iPrice > l_OtherL->GetGold () )				        		///< Has enought money
						{
							PacketLogCheat sPacket;
							sPacket.iCheatID = CHEATLOGID_GenerateGoldHack;
							sPacket.LParam = l_Item.iPrice;
							sPacket.SParam = l_OtherL->GetGold ();
							sPacket.WxParam = 7;
							LOGSERVER->OnLogCheat ( l_DCTarget, &sPacket );

							WARN ( "Player (%s) shop SCAM ATTEMP try to buy (%d or %d) x item(%d) for price of %d instead of %d from player shop of Player (%s)",
								l_OtherL->szCharacterName, l_QueryBuyCount, l_BuyQuantity, l_Item.eItemID, l_QueryPrice, l_ExceptedBuyPrice, pcUserData->szCharacterName );
						}
						else
						{
							l_FNFowardPacketToBinary ( psPacket, pcUserData );

							///std::cout << "Buy price : " << l_Item.iPrice << std::endl;
							///std::cout << "pcUserData : " << pcUserData->GetGold() << std::endl;
							///std::cout << "l_OtherL : " << l_OtherL->GetGold() << std::endl;

							{
								PacketSetCharacterGold sPacket;
								sPacket.iHeader = PKTHDR_SetGold;
								sPacket.iLength = sizeof ( PacketSetCharacterGold );
								sPacket.dwGold = pcUserData->GetGold ();

								PACKETSERVER->Send ( pcUserData, &sPacket );
							}
							{
								PacketSetCharacterGold sPacket;
								sPacket.iHeader = PKTHDR_SetGold;
								sPacket.iLength = sizeof ( PacketSetCharacterGold );
								sPacket.dwGold = l_OtherL->GetGold ();

								PACKETSERVER->Send ( l_OtherL, &sPacket );
							}

							l_AllOk = true;

							return TRUE;
						}
					}
					else
					{
						WARN ( "Player shop item not found" );
					}
				}
			}
			else
			{
				WARN ( "Player shop owner not found" );
			}

			if (!l_AllOk)
			{
				LOGERROR ( "Error with shop - D/C user" );

				if ( pcUser->GetGameLevel() != GAMELEVEL_Four )
				{
					NETSERVER->DisconnectUser( l_DCTarget );

					SENDPACKETBLANK( USERDATATOUSER( l_DCTarget ), PKTHDR_Disconnect );
					WNDPROC_SOCKETCLOSE( SOCKETSERVER->GetHWND(), l_DCTarget->pcSocketData );
				}

				return TRUE;
			}

			break;
		}

		case PKTHDR_SendStablePet:
			PETSYSTEMHANDLER->HandlePacket(pcUserData);
			break;
			
		case PKTHDR_SendStablePet_ActivePet:
			PETSYSTEMHANDLER->HandlePacket(pcUserData,(PacketStablePetActivePet*)psPacket);
			break;

			//Coin Shop
		case PKTHDR_CoinShopAmount:
			COINSHOPHANDLER->HandlePacket( pcUser, (PacketCoinAmount*)psPacket );
			break;

		case PKTHDR_CoinShopNPC:
			COINSHOPHANDLER->HandlePacket( pcUser, (PacketCoinShopNPC*)psPacket );
			break;

		case PKTHDR_CoinShopOpenTab:
			COINSHOPHANDLER->HandlePacket( pcUser, (PacketCoinOpenTab*)psPacket );
			break;

		case PKTHDR_CoinShopOpenItemData:
			COINSHOPHANDLER->HandlePacket( pcUser, (PacketCoinOpenItemData*)psPacket );
			break;

		case PKTHDR_CoinShopBuy:
			COINSHOPHANDLER->HandlePacket( pcUser, (PacketCoinBuy*)psPacket );
			break;

		case PKTHDR_CoinServiceBuy:
			COINSHOPHANDLER->HandlePacket( pcUser, (PacketCoinServiceBuy*)psPacket );
			break;

		case PKTHDR_KillCharacter:
			USERSERVER->OnKilled( pcUser, (PacketDeadCharacter*)psPacket );
			break;

		case PKTHDR_SettingsData:
			USERSERVER->SettingsDataSet( pcUser, (PacketSettingsData*)psPacket );
			break;

		case PKTHDR_FuryArena:
			FURYARENAHANDLER->PacketHandlerData( pcUser, (PacketFuryArenaRequest*)psPacket );
			break;

		case PTKHDR_EventGirlReset:
			EVENTGIRLAHANDLER->OnEventGirlResetRequest(pcUser, (PacketEventGirlReset*)psPacket);
			break;

		case PKTHDR_RankingListRequest:
			RANKINGLISTHANDLER->SendRankingData(pcUser, ((PacketRankingListRequest*)psPacket)->iRankingType);
			break;

		case PKTHDR_SoloRankingListRequest:
			RANKINGLISTHANDLER->SendBellatraSoloRankingData(pcUser, ((PacketRankingListRequest*)psPacket)->iRankingType);
			break;

			//Event
		case PKTHDR_NPCEasterEgg:
			EVENTSERVER->HandlePacket( pcUserData, (PacketNPCEasterEgg*)psPacket );
			break;

		case PKTHDR_NPCRudolphChristmas:
			EVENTSERVER->HandlePacket( pcUserData, (PacketNPCRudolphChristmas*)psPacket );
			break;

		case PKTHDR_ItemSlotHandle:
			ITEMSERVER->HandlePacket( pcUser, (PacketSlotItemHandle*)psPacket );
			break;

		//damage info sent from player to other players.
		case PKTHDR_DamageDebugInfo:
			MAPSERVER->SendUserReceivedDamageInfoTextToNearbyPlayers( pcUserData, (PacketDebugDamageInfo*)psPacket );
			break;

		case PKTHDR_SingleTargetSkillData:
			DAMAGEHANDLER->HandlePacket( pcUser, (PacketSingleTargetSkillData*) psPacket);
			psPacket->iHeader = 0;
			break;

		case PKTHDR_NpcUnitClick:
			UNITSERVER->HandleNPCClick( pcUser, (PacketNPCClick*) psPacket);
			break;

		case PKTHDR_MultiTargetSkillData:
			DAMAGEHANDLER->HandlePacket( pcUser, (PacketMultiTargetSkillData*)psPacket );
			psPacket->iHeader = 0;
			break;

		case PKTHDR_AttackData:
			break;

		case PKTHDR_DisableGM:
			pcUserData->iGameLevel = GAMELEVEL_None;
			NETSERVER->SyncGameMaster( pcUserData );
			CHATSERVER->SendChat( USERDATATOUSER( pcUserData ), CHATCOLOR_Error, "> GM Disabled!" );
			break;

		case PKTHDR_DeleteFlagPoint:
			MAPSERVER->DeleteFlagPoint( pcUserData, (PacketDeleteFlagPoint*)psPacket );
			break;

		case PKTHDR_GetItemData:
			if ( pcUserData->iGameLevel == GAMELEVEL_Four )
			{
				// Set Temp Item
				CopyMemory( &pcUser->sTempItem, ((PacketItemData*)psPacket), sizeof( PacketItemData ) );
				CHATSERVER->SendDebugChat( pcUser, CHATCOLOR_Error, "> Item Temp Getted!" );
			}
			break;

		case PKTHDR_DynamicQuestOpenNPC:
			QUESTSERVER->HandlePacket( pcUser, (PacketDynamicQuestNPCOpen*)psPacket );
			break;

		case PKTHDR_PlayDataChar:
			USERSERVER->HandlePacket( pcUser, (PacketPlayData*)psPacket );
			break;

#pragma region(TradeSystem)
		case PKTHDR_TradeRequest:
			if (GAME_SERVER)
			{
                /// @LEVEL RESTRICTION for trade init
                if (((PacketTradeRequest *)psPacket)->iType == 0 && pcUserData->GetLevel() < 10)
                {
                    CHATSERVER->SendChat(pcUser, CHATCOLOR_Error, "> You need to be level 10 to trade other players.");
                    return TRUE;
                }

				NetConnection * l_LoginServerConnection = NETSERVER->GetLoginServerConnection();

				if (l_LoginServerConnection)
					NETSERVER->SendPacket(l_LoginServerConnection->pcSocketData, psPacket);
				else
					return TRUE;
			}
			else if (LOGIN_SERVER)
			{
				int		l_ReceiverID	= ((PacketTradeRequest*)psPacket)->iReceiverID;
				int		l_SenderID		= ((PacketTradeRequest*)psPacket)->iSenderID;
				User *	l_PacketOwner	= USERDATATOUSER(USERDATABYID(l_SenderID));
				User *	l_ReceiverOwner	= USERDATATOUSER(USERDATABYID(l_ReceiverID));

                ///std::cout << "Trade type " << ((PacketTradeRequest *)psPacket)->iType << "ta " << l_PacketOwner << " tb " << l_ReceiverOwner  << std::endl;

                if (l_PacketOwner)
				{
					USERSERVER->OnTradeRequest(l_PacketOwner->pcUserData,	(PacketSimple*)psPacket);
					TRADEHANDLER->HandlePacket(l_PacketOwner,				(PacketTradeRequest*)psPacket);

					l_FNFowardPacketToBinary(psPacket, l_PacketOwner->pcUserData);
					return TRUE;
				}
				else if (((PacketTradeRequest*)psPacket)->iType == 3 && l_ReceiverOwner)
				{
					l_ReceiverOwner->iTradeGold			= 0;
					l_ReceiverOwner->TradeWindowOpen	= false;

                    /// Strange ?
                    if (!l_ReceiverOwner->WasInTwoSideTrade)
                    {
						LOGERROR( "Unknown PacketTradeRequest status" );

						if ( pcUser->GetGameLevel() != GAMELEVEL_Four )
						{
							/// Kick to avoid dup item to be saved
							SENDPACKETBLANK( l_ReceiverOwner, PKTHDR_Disconnect );
							WNDPROC_SOCKETCLOSE( SOCKETSERVER->GetHWND(), l_ReceiverOwner->pcUserData->pcSocketData );

							NETSERVER->DisconnectUser( l_ReceiverOwner->pcUserData );
						}

                        return TRUE;
                    }

                    l_ReceiverOwner->WasInTwoSideTrade = false;
				}
				else
					return TRUE;
			}

			break;

		case PKTHDR_ForceOrbData:
			{
				return TRUE;
			//std::cout << "PKTHDR_ForceOrbData" << std::endl;
			}
			break;

		case PKTHDR_TradeData:
			if (GAME_SERVER)
			{
				NetConnection * l_LoginServerConnection = NETSERVER->GetLoginServerConnection();

				if (l_LoginServerConnection)
					NETSERVER->SendPacket(l_LoginServerConnection->pcSocketData, psPacket);
				else
					return TRUE;
			}
			else if (LOGIN_SERVER)
			{
				int		l_SenderID		= ((PacketTradeData*)psPacket)->iSenderID;
				User *	l_PacketOwner	 = USERDATATOUSER(USERDATABYID(l_SenderID));

				if (l_PacketOwner)
				{
					TRADEHANDLER->HandlePacket(l_PacketOwner, (PacketTradeData*)psPacket);

					l_FNFowardPacketToBinary(psPacket, l_PacketOwner->pcUserData);
					return TRUE;
				}
				else
					return TRUE;
			}
			break;

		case PKTHDR_TradeReady:
			if (GAME_SERVER)
			{
				NetConnection * l_LoginServerConnection = NETSERVER->GetLoginServerConnection();

				if (l_LoginServerConnection)
					NETSERVER->SendPacket(l_LoginServerConnection->pcSocketData, psPacket);
				else
					return TRUE;
			}
			else if (LOGIN_SERVER)
			{
				int		l_SenderID = ((PacketTradeSuccess*)psPacket)->iSenderID;
				User *	l_PacketOwner = USERDATATOUSER(USERDATABYID(l_SenderID));

				if (l_PacketOwner)
				{
					/// @NOT HANLDED ATM

					l_FNFowardPacketToBinary(psPacket, l_PacketOwner->pcUserData);
					return TRUE;
				}
				else
					return TRUE;
			}
			break;

		case PKTHDR_TradeItems:
			if (GAME_SERVER)
			{
				NetConnection * l_LoginServerConnection = NETSERVER->GetLoginServerConnection();

				if (l_LoginServerConnection)
					NETSERVER->SendPacket(l_LoginServerConnection->pcSocketData, psPacket);
				else
					return TRUE;
			}
			else if (LOGIN_SERVER)
			{
				int		l_SenderID		= ((PacketTradeSuccess*)psPacket)->iSenderID;
				User *	l_PacketOwner	= USERDATATOUSER(USERDATABYID(l_SenderID));

				if (l_PacketOwner)
				{
					TRADEHANDLER->HandlePacket(l_PacketOwner, (PacketTradeSuccess*)psPacket);

					l_FNFowardPacketToBinary(psPacket, l_PacketOwner->pcUserData);
					return TRUE;
				}
				else
					return TRUE;
			}
			break;
#pragma endregion

		case PKTHDR_CoinTradeSet:
			COINSHOPHANDLER->HandlePacket( pcUser, (PacketTradeCoinSet*)psPacket );
			break;

		case PKTHDR_BuffSkillsParty:
			BuffReceive( pcUser, (PacketSkillPartyData*)psPacket );
			break;

		case PKTHDR_UpdatePetData:
			USERSERVER->UpdatePetData( pcUser, (PacketUpdatePetData*)psPacket );
			break;

		case PKTHDR_UpdatePetDataEx:
			USERSERVER->UpdatePetDataEx( pcUser, (PacketUpdatePetDataEx*)psPacket );
			break;

		case PKTHDR_CharacterCombatData:
			CHARACTERSERVER->HandlePacket( pcUser, (PacketCharacterCombatData*)psPacket );
			break;

		case PKTHDR_HolyMind:
			PRIESTESSSKILL->OnUseHolyMind( pcUser, (PacketHolyMind*)psPacket );
			break;

		case PKTHDR_Wisp:
			ASSASSINSKILL->OnUseWisp( pcUser, (PacketWisp*)psPacket );
			break;

		case PKTHDR_GetPlayerInfoData:
			BOTSERVER->OnGetUnitInfo( pcUser, (PacketSimple*)psPacket );
			USERSERVER->OnGetPlayerInfoData(pcUser, (PacketSimple*)psPacket );
			QUESTSERVER->HandleGetUnitInfoData( pcUser, ((PacketSimple *)psPacket)->iUnk2 );
			break;

		case PKTHDR_CancelForceOrb:
			USERSERVER->CancelForceOrb( pcUser );
			break;

		case PKTHDR_QuestNamesRequest: //login server
			QUESTSERVER->HandlePacket(pcUser, (PacketQuestNamesRequest*)psPacket);
			break;

		case PKTHDR_QuestDataRequest:
			QUESTSERVER->HandlePacket(pcUser, (PacketQuestDataRequest*)psPacket);
			break;

		case PKTHDR_CommandUser:
		{
			break;
		}

		case PKTHDR_ThrowItem:
			if (LOGIN_SERVER)
				return TRUE;

			if (MAPSERVER->OnThrowItem(pcUser, (PacketThrowItem*)psPacket))
				return TRUE;
			/*else
			{
				PacketNetPlayerThrow l_NetThrowPacket;
				l_NetThrowPacket.iLength		= sizeof(PacketNetPlayerThrow);
				l_NetThrowPacket.iHeader		= PKTHDR_NetPlayerThrow;
				l_NetThrowPacket.ObjectID		= pcUserData->iID;
				l_NetThrowPacket.Item.iItemID	= ((PacketThrowItem*)psPacket)->sItem.sItemID.ToItemID();
				l_NetThrowPacket.Item.iChk1		= ((PacketThrowItem*)psPacket)->sItem.iChk1;
				l_NetThrowPacket.Item.iChk2		= ((PacketThrowItem*)psPacket)->sItem.iChk2;
				l_NetThrowPacket.Gold		    = ((PacketThrowItem*)psPacket)->sItem.iGold;

				NetConnection * l_LoginServerConnection = NETSERVER->GetLoginServerConnection();
				if (l_LoginServerConnection)
					NETSERVER->SendPacket(l_LoginServerConnection->pcSocketData, &l_NetThrowPacket);
				else
					return TRUE;
			}*/

			break;

		case PKTHDR_CheckItemUseSlot:
			ITEMSERVER->PHCheckItemUsing( pcUserData, (PacketListUsingItem*)psPacket );
			break;

		case PKTHDR_CheckItemUseSlotEx:
			ITEMSERVER->PHCheckItemUsingEx( pcUserData, (PacketListUsingItem*)psPacket );
			break;

		case PKTHDR_UseUnionCore:
			MAPSERVER->UseUnionCore( pcUserData, (PacketUseUnionCore*)psPacket );
			break;

		case PKTHDR_CancelDrasticSpirit:
			KNIGHTSKILL->OnCancelDrasticSpirit( pcUser );
			break;

		case PKTHDR_NPCIhinRewardRequest:
			BELLATRASERVER->HandlePacket( pcUser, (PacketReceiveItemIhin*)psPacket );
			break;

			//Item
		case PKTHDR_SetItemTimer:
			ITEMSERVER->HandlePacket( pcUser, (PacketNewItemTimer*)psPacket );
			break;
		case PKTHDR_CancelItemTimer:
			ITEMSERVER->HandlePacket( pcUser, (PacketCancelItemTimer*)psPacket );
			break;
		case PKTHDR_CharacterHeadOldUse:
			ITEMSERVER->HandlePacket( pcUser, (PacketOldHeadUse*)psPacket );
			break;
		case PKTHDR_T5QuestArena:
			QUESTARENAHANDLER->HandlePacket( pcUser, (PacketQuestArenaTier5*)psPacket );
			break;
		case PKTHDR_NewItem:
			ITEMSERVER->HandlePacket( pcUser, (PacketGetItem*)psPacket );
			return TRUE;

        case PKTHDR_GetItem:
        {
			if ( pcUser && pcUser->IsValidAndInGame () )
			{
				PacketTransCommandEx * l_Packet = reinterpret_cast<PacketTransCommandEx *>( psPacket );

				int32_t     l_Gold = l_Packet->EParam ^ PKTHDR_GetItem ^ l_Packet->SParam;
				uint32_t    l_PacketHash = hash_data ( (char *)psPacket, psPacket->iLength );

				if ( l_Gold < 0 || l_Gold > MAX_DROP_GOLD ( pcUser->pcUserData->sCharacterData.iLevel ) )
				{
					PacketLogCheat sPacket;
					sPacket.iCheatID = CHEATLOGID_GenerateGoldHack;
					sPacket.LParam = l_Gold;
					sPacket.SParam = l_Gold;
					sPacket.WxParam = 22;
					LOGSERVER->OnLogCheat ( pcUser->pcUserData, &sPacket );

					if ( pcUser->GetGameLevel() != GAMELEVEL_Four )
					{
						NETSERVER->DisconnectUser( pcUser->pcUserData );

						SENDPACKETBLANK( USERDATATOUSER( pcUser->pcUserData ), PKTHDR_Disconnect );
						WNDPROC_SOCKETCLOSE( SOCKETSERVER->GetHWND(), pcUser->pcUserData->pcSocketData );
					}

					return TRUE;
				}

				//auto l_It = std::find(pcUser->PlayerItemPutHashs.begin(), pcUser->PlayerItemPutHashs.end(), l_PacketHash);
				//if (l_It != pcUser->PlayerItemPutHashs.end())
				{
					// pcUser->PlayerItemPutHashs.erase(l_It);

					l_FNFowardPacketToBinary ( psPacket, pcUserData );
				}
			}

            /// Skip the packet
            return TRUE;
        }

        case PKTHDR_PaymentMoney:
        {
            PacketTransCommand * l_Packet = reinterpret_cast<PacketTransCommand*>(psPacket);
            int l_Money = l_Packet->LParam;

            if (pcUserData && (l_Money < 0 || l_Money > pcUserData->GetGold()))
            {
                PacketLogCheat sPacket;
		        sPacket.iCheatID	= CHEATLOGID_GenerateGoldHack;
		        sPacket.LParam		= 0;
		        sPacket.SParam		= 0;
		        sPacket.WxParam		= 15;
		        LOGSERVER->OnLogCheat(pcUser->pcUserData, &sPacket);

                return TRUE;
            }

            break;
        }

        case PKTHDR_RollDiceDropUserResult:
			ROLLDICEHANDLER->HandlePacket( pcUser, (PacketRollDiceDrop*)psPacket );
			break;

			//Party
		case PKTHDR_RequestParty:
			PARTYHANDLER->HandlePacket( pcUser, (PacketRequestParty*)psPacket );
			break;
		case PKTHDR_JoinParty:
			PARTYHANDLER->HandlePacket( pcUser, (PacketJoinParty*)psPacket );
			break;
		case PKTHDR_ActionParty:
			PARTYHANDLER->HandlePacket( pcUser, (PacketActionParty*)psPacket );
			break;
		case PKTHDR_RequestRaid:
			PARTYHANDLER->HandlePacket( pcUser, (PacketRequestRaid*)psPacket );
			break;
		case PKTHDR_JoinRaid:
			PARTYHANDLER->HandlePacket( pcUser, (PacketJoinRaid*)psPacket );
			break;

		case PKTHDR_BossTimeUpdate:
			MAPSERVER->HandlePacket( pcUser, (PacketBossTimeUpdate*)psPacket );
			break;

		case PKTHDR_QuickHealthUpdate:
			if (reinterpret_cast<PacketQuickHealthUpdate*>(psPacket)->dwID == pcUserData->iID)
                SENDPACKETRANGE(psPacket, &pcUserData->sPosition, pcUserData->iMapID, DISTANCE_MAX_SKILL_VISUAL, true, pcUserData);
			return true;

        case PKTHDR_EnterBellatraCastle:
        {
            if (LOGIN_SERVER)
                return TRUE;

            int l_CharLevel = pcUserData->GetLevel();

            if (l_CharLevel < 40)
                return TRUE;

			//from level 40 to 80
            float fPrice = 300.0f * l_CharLevel;

			if (l_CharLevel >= 80)
			{
				//https://docs.google.com/spreadsheets/d/1i_L5-JUzPuEar8zmhDiUoJ_9DqX1mkA2zkztBK43aco/edit?folder=1SuFzuqBkywRit6ppZylWYEn7VIhzhq2q#gid=243745468
				//Level 80 = 40k
				//Level 90 = 85k
				//Level 100 = 130k
				//fPrice = round((500.f * l_CharLevel + (4000 * (l_CharLevel - 80)) ) / 1000.0f) * 1000;
				//new system requires change in Game.dll

				fPrice = 700.0f * l_CharLevel;
			}

			int iPrice = static_cast<int>(fPrice);

			//protection against level-based cheats if there is one...
			if (iPrice < 0)
				iPrice = 0;

            int l_Result = reinterpret_cast<signed int(__cdecl*)(UserData *, int)>(0x560310)(pcUserData, 0);
			const int currentPlayerGold = pcUser->pcUserData->GetGold();

			if (iPrice <= currentPlayerGold)
			{
				if (l_Result == 1)
				{
					CHARACTERSERVER->GiveGOLD(pcUser, -iPrice, WHEREID_BellatraSOD);
					CHATSERVER->SendChatEx(pcUser, CHATCOLOR_White, "Bellatra> %s gp paid for entrance fee", FormatNumber(iPrice));
					BELLATRASERVER->AddEntranceFee(iPrice);
				}
			}
			else
			{
				CHATSERVER->SendTitleBox(pcUser, "Not enough gold!");
			}

            return TRUE;
        }

		case PKTHDR_NeverSinkTeleportRequest:
		{
			if (LOGIN_SERVER)
			{
				return TRUE;
			}

			const int charLevel = pcUserData->GetLevel();
			int price = 0;

			PacketNeverSinkTeleportRequest* teleportRequest = (PacketNeverSinkTeleportRequest*)psPacket;

			PacketWarpGateField teleportResponse;

			teleportResponse.iLength = sizeof(PacketWarpGateField);
			teleportResponse.iHeader = PKTHDR_WarpGateField;
			teleportResponse.iStage = teleportRequest->fieldID;

			switch (teleportRequest->fieldID)
			{
				case MAPID_RicartenTown:
					teleportResponse.iX = 3665 << 8;
					teleportResponse.iZ = -17470 << 8;
					price = 2500;

					if (charLevel < 1)
					{
						CHATSERVER->SendTitleBox(pcUser, "Your level does not meet the level requierment: %d\n", 1);
						return TRUE;
					}
					break;
				case MAPID_CastleOfTheLost:
					teleportResponse.iX = -1798 << 8;
					teleportResponse.iZ = 12499 << 8;
					price = 2500;

					if (charLevel < 10)
					{
						CHATSERVER->SendTitleBox(pcUser, "Your level does not meet the level requierment: %d\n", 10);
						return TRUE;
					}
					break;
				case MAPID_BlessCastle:
					teleportResponse.iX = 36731 << 8;
					teleportResponse.iZ = -22491 << 8;
					price = 30000;

					if (charLevel < 39)
					{
						CHATSERVER->SendTitleBox(pcUser, "Your level does not meet the level requierment: %d\n", 39);
						return TRUE;
					}
					break;
				case MAPID_LandOfChaos:
					teleportResponse.iX = -10041 << 8;
					teleportResponse.iZ = 3399 << 8;
					price = 25000;

					if (charLevel < 95)
					{
						CHATSERVER->SendTitleBox(pcUser, "Your level does not meet the level requierment: %d\n", 96);
						return TRUE;
					}
			}

			const int currentPlayerGold = pcUser->pcUserData->GetGold();

			if (price <= currentPlayerGold)
			{
				CHARACTERSERVER->GiveGOLD(pcUser, -price, WHEREID_NeverSinkTeleport);
				SENDPACKET(pcUser, &teleportResponse, TRUE);
			}
			else
			{
				CHATSERVER->SendTitleBox(pcUser, "Not enough gold!");
			}
		}
		break;
	}

	l_FNFowardPacketToBinary(psPacket, pcUserData);

    int l_NewGold = pcUserData ? pcUserData->GetGold() : 0;

    if (pcUserData && l_OldGold != l_NewGold)
    {
        if (GAME_SERVER)
        {
            int l_Diff = l_NewGold - l_OldGold;

            PacketNetPlayerGoldDiff l_DiffPacket;
            l_DiffPacket.iHeader    = PKTHDR_NetPlayerGoldDiff;
            l_DiffPacket.iLength    = sizeof(PacketNetPlayerGoldDiff);
            l_DiffPacket.ObjectID   = pcUserData->iID;
            l_DiffPacket.Diff       = l_Diff;

            NetConnection * l_LoginServer = NETSERVER->GetLoginServerConnection();
            if (l_LoginServer)
                NETSERVER->SendPacket(l_LoginServer->pcSocketData, &l_DiffPacket);
        }
        else if (LOGIN_SERVER)
        {
            PacketSetCharacterGold sPacket;
            sPacket.iHeader = PKTHDR_SetGold;
            sPacket.iLength = sizeof(PacketSetCharacterGold);
            sPacket.dwGold  = pcUserData->GetGold();
        }
    }

    if (LOGIN_SERVER && pcUserData)
    {
        int l_GoldLimit = 0;

		if (pcUserData->GetLevel() <= 10)
			l_GoldLimit = 260'000;

		else if (pcUserData->GetLevel() < 20)
			l_GoldLimit = (pcUserData->GetLevel() * 200'000);

        else
            l_GoldLimit = 500'000'000;

        if (pcUserData->GetGold() > l_GoldLimit)
        {
            PacketLogCheat sPacket;
		    sPacket.iCheatID	= CHEATLOGID_GoldLimitReached;
		    sPacket.LParam		= pcUserData->GetGold();
		    sPacket.SParam		= l_GoldLimit;
		    LOGSERVER->OnLogCheat(pcUser->pcUserData, &sPacket);
        }
    }

	return TRUE;
}


void PacketServer::Send( User * pcUser, void * pPacket, BOOL bEncrypted )
{
	if (!pcUser || !pcUser->pcUserData)
		return;

	Send(pcUser->pcUserData, pPacket, bEncrypted);
}

//WIP - for later
void PacketServer::ShopCodeTemp()
{

	/*case PKTHDR_Shop_ItemList:
	{
	Item * lpItemInfo;
	BYTE * lpBuff;
	BYTE * lpBuffCheck;

	int cnt;

	char szBuff[0x2000];
	smTRANS_SELL_ITEM * lpTransSellItemNew;
	char * lpBuffNew;
	int TransLenNew;
	int	ItemCountNew;
	int len;

	lpTransSellItemNew = (smTRANS_SELL_ITEM *)szBuff;
	lpTransSellItemNew->iHeader = PKTHDR_Shop_ItemList;

	ItemCountNew = 0;
	lpBuffNew = ( (char *)lpTransSellItemNew ) + sizeof ( smTRANS_SELL_ITEM );
	TransLenNew = sizeof ( smTRANS_SELL_ITEM );

	smTRANS_SELL_ITEM * lpTransSellItem = reinterpret_cast<smTRANS_SELL_ITEM *>( pPacket );


	//BYTE * lpBuffNew = new BYTE[sizeof ( smTRANS_SELL_ITEM ) * lpTransSellItem->ItemCounter];

	lpItemInfo = new Item[lpTransSellItem->ItemCounter];
	lpBuff = (BYTE *)pPacket + sizeof ( smTRANS_SELL_ITEM );
	for ( cnt = 0; cnt < lpTransSellItem->ItemCounter; cnt++ )
	{
	lpBuff += ITEMSERVER->DecodeCompress ( lpBuff, (BYTE *)&lpItemInfo[cnt] );
	//lpItemInfo[cnt].iSalePrice = ItemServer::GetItemBuyPrice ( lpItemInfo[cnt].sItemID.ToItemID (), lpItemInfo[cnt].eSpecialization != 0 );

	DefinitionItem * lpDefItem = ITEMSERVER->FindItemDefByCode ( lpItemInfo[cnt].sItemID.ToItemID () );


	//lpDefItem->sItem.iSalePrice = ItemServer::GetItemBuyPrice ( lpItemInfo[cnt].sItemID.ToItemID (), lpItemInfo[cnt].eSpecialization != 0 );

	if ( lpDefItem && ( TransLenNew + lpDefItem->DefCompressDataLen ) < 0x2000 )
	{
	int len = ITEMSERVER->EncodeCompress ( (BYTE *)&lpDefItem->sItem, (BYTE *)lpBuffNew, sizeof ( Item ) );

	//memcpy ( lpBuffNew, lpDefItem->DefCompressData, lpDefItem->DefCompressDataLen );
	TransLenNew += len;
	lpBuffNew += len;
	ItemCountNew++;
	}


	/*len = ITEMSERVER->EncodeCompress ( (BYTE *)&lpItemInfo[cnt], (BYTE*)lpBuffNew, sizeof ( smTRANS_SELL_ITEM ) );

	TransLenNew += len;
	lpBuffNew += len;
	ItemCountNew++;*

	}

	lpTransSellItemNew->iHeader = lpTransSellItem->iHeader;
	lpTransSellItemNew->iLength = TransLenNew;
	lpTransSellItemNew->ItemCounter = ItemCountNew;
	lpTransSellItemNew->ItemKind = lpTransSellItem->ItemKind;

	CopyMemory ( &lpTransSellItemNew->Ext, &lpTransSellItem->Ext, sizeof ( int ) * 4 );

	//The first smTRANS_SELL_ITEM holds information about the packet (such as number of items, and header)
	//but we want to replace the remaining buffer with the updated one
	//CopyMemory ( newPacket + sizeof ( smTRANS_SELL_ITEM ), lpBuffNew, sizeof ( smTRANS_SELL_ITEM ) * lpTransSellItem->ItemCounter + sizeof ( smTRANS_SELL_ITEM ) );

	/*lpBuffCheck = (BYTE *)lpTransSellItemNew + sizeof ( smTRANS_SELL_ITEM );

	for ( cnt = 0; cnt < lpTransSellItemNew->ItemCounter; cnt++ )
	{
	lpBuffCheck += ITEMSERVER->DecodeCompress ( lpBuffCheck, (BYTE *)&lpItemInfo[cnt] );
	lpItemInfo[cnt].iSalePrice = ItemServer::GetItemBuyPrice ( lpItemInfo[cnt].sItemID.ToItemID (), lpItemInfo[cnt].eSpecialization != 0 );
	}

	std::cout << "lpTransSellItem: " << lpTransSellItem->iHeader << std::endl;
	*
	//this calls this code again

	pcUserData->pcSocketData->SendPacket ( (Packet *)lpTransSellItemNew, bEncrypted );

	//return so that we don't send the other packet

	ZeroMemory ( lpTransSellItemNew, sizeof ( smTRANS_SELL_ITEM ) );
	return;
	}*/

}


const std::string PacketServer::GetExtraInfoForPacket( Packet * packet  )
{
	std::string extraInfo = "";

	switch ( packet->iHeader )
	{
		case PKTHDR_CharacterData:
		{
			auto p = (PacketUnitInfo *)packet;
			extraInfo = FormatString( "ID: %d (%s)", p->iID, p->sCharacterData.szName );
			break;
		}
		case PKTHDR_GetPlayerInfoData:
		{
			auto p = (PacketSimple *)packet;
			extraInfo = FormatString( "ID: %d", p->iUnk2 );
			break;
		}
		case PKTHDR_HACKTRAP:
		{
			auto p = (PacketTransCommand *)packet;
			extraInfo = FormatString( "Trap code: %d (%d)", p->LParam );
			break;
		}

		case PKTHDR_SET_BLACKLIST:
		case PKTHDR_SET_BLACKLIST2:
		case PKTHDR_SET_BLACKLIST3:
		{
			auto p = (PacketTransCommand *)packet;
			extraInfo = FormatString( "Cheat ID (WParm): %d", p->WParam );
			break;
		}
	}

	return extraInfo;
}

BOOL PacketServer::IsPacketIgnored( Packet * packet )
{
	switch ( packet->iHeader )
	{
		//case PKTHDR_SkillBuffStatus:
		case PKTHDR_NetIdentifier:			//for inter server to identify itself
		case PKTHDR_NetUsersOnline:			//game server sends this to login server every minute
		case PKTHDR_GameTimeSync:			//game server sends this to each player every ~ 1 second
		case PKTHDR_NetClan:				//game server sends this to login server every ~ 5 seconds
		case PKTHDR_ServerInfo:				//game server sends this to player every ~ 5 seconds
		case PKTHDR_PlayData:				//game server receives this from each player at 1s interval
		case PKTHDR_PlayDataEx:				//game server and login server receives this from each player at about 5s interval
		case PKTHDR_UnitStatusContainer:	//info about other units in a group
		case PKTHDR_Ping:					//received by both server from players at about ~3 second interval
		case PKTHDR_NetPlayDataEx:			//game server sends this to the login server after receiving PlayDataEx from client. ~ 5 sec interval
		case PKTHDR_PremiumDataSync:		//game server sends this to each player every ~ 10 seconds
		case PKTHDR_PlayItem:				//for showing items on the floor
		case PKTHDR_DamageDebugInfo:		//for sending damage text info to players (and to nearby players)
		case PKTHDR_AttackData:				//When monsters attack players
		case PKTHDR_CharacterCombatData:	//Player sends combat info whenever def / abs / attack rating changes; or every 5s in BC or every 10s in normal maps
		case PKTHDR_QuestUpdateData:		//Received from client every 10s
		case PKTHDR_NetQuestUpdateDataPart:	//For sync'ing game server with quest data from login server
		case PKTHDR_NetGiveExp:				//game sends to login
		case PKTHDR_AddExp:					//For giving exp
			return TRUE;
	}

	return FALSE;

}

void PacketServer::Send(UserData * pcUserData, void * pPacket, BOOL bEncrypted)
{
	if (!pcUserData || !pcUserData->pcSocketData)
		return;

#if defined (DEV_MODE) && defined (PACKET_LOG)

#ifdef DEV_MODE
	if ( ((Packet*)pPacket)->iHeader == 0 )
	{
		WARN( "Sending packet header is 0!" );
	}

	if ( ((Packet*)pPacket)->iLength == 0 )
	{
		WARN( "Sending packet length is 0!" );
	}
#endif

	if (!IsPacketIgnored((Packet*)pPacket) )
	{
		const std::string extraInfo = GetExtraInfoForPacket((Packet*)pPacket);
		const std::string& l_OpcodeName = SOCKETSERVER->GetOpcodeName(((Packet*)pPacket)->iHeader);
		std::string charName = CHARACTERSERVER->GetCharacterName( pcUserData );

		if ( charName.empty() )
			charName = pcUserData->pcSocketData->szIP;

		if ( !l_OpcodeName.empty() )
		{
			if ( !extraInfo.empty() )
			{
				DEBUGPACKET( "[ SERVER --> '%s' ] %s -- %s", charName.c_str(), l_OpcodeName.c_str(), extraInfo.c_str() );
			}
			else
			{
				DEBUGPACKET( "[ SERVER --> '%s' ] %s", charName.c_str(), l_OpcodeName.c_str() );
			}
		}
		else
		{
			char hexString[16];
			_itoa_s( ( (Packet *)pPacket )->iHeader, hexString, 16 );
			WARN( "[ SERVER --> %s ] Unknown Opcode. Please add packet 0x%s tp ADD_OPCODE_NAME(..)", charName.c_str(), hexString);
		}
    }
#endif

	if ( ( (Packet *)pPacket )->iHeader == PKTHDR_ForceOrbData )
	{
		//std::cout << "Skipping PKTHDR_ForceOrbData" << std::endl;
		//Don't send
		return;
	}

	if ( ( (Packet *)pPacket )->iHeader == PKTHDR_SendGameServer )
	{
		std::string charName = CHARACTERSERVER->GetCharacterName( pcUserData );

		if ( charName.empty() )
			charName = pcUserData->pcSocketData->szIP;

		//INFO ( "PKTHDR_SendGameServer: %s", charName.c_str() );
		LOGSERVER->OnLogEx( USERDATATOUSER(pcUserData), ACCLOGID_CharacterLoad, "[%s][%s] PKTHDR_SendGameServer", (GAME_SERVER ? "GS" : "LS"), charName.c_str() );

	}

	if ( ( (Packet *)pPacket )->iHeader == PKTHDR_SaveData )
	{
		std::string charName = CHARACTERSERVER->GetCharacterName( pcUserData );

		if ( charName.empty() )
			charName = pcUserData->pcSocketData->szIP;

		//INFO ( "PKTHDR_SaveData: %s", charName.c_str() );
		LOGSERVER->OnLogEx( USERDATATOUSER(pcUserData), ACCLOGID_CharacterLoad, "[%s][%s] PKTHDR_SaveData", (GAME_SERVER ? "GS" : "LS"), charName.c_str() );

	}

	if ( ( (Packet *)pPacket )->iHeader ==  PKTHDR_DamageEncodeMem )
	{
		std::string charName = CHARACTERSERVER->GetCharacterName( pcUserData );

		if ( charName.empty() )
			charName = pcUserData->pcSocketData->szIP;

		//INFO ( "PKTHDR_DamageEncodeMem: %s", charName.c_str() );
		LOGSERVER->OnLogEx( USERDATATOUSER(pcUserData), ACCLOGID_CharacterLoad, "[%s][%s] PKTHDR_DamageEncodeMem", (GAME_SERVER ? "GS" : "LS"), charName.c_str() );

	}

    if (((Packet *)pPacket)->iHeader == PKTHDR_NewItem)
    {
        PacketItem * l_OriginalPacket = reinterpret_cast<PacketItem*>(pPacket);

        PacketTransCommandEx TransCommandEx;
        TransCommandEx.iHeader = PKTHDR_GetItem;
        TransCommandEx.iLength = sizeof(PacketTransCommandEx);
        TransCommandEx.WParam = l_OriginalPacket->sItem.sItemID.ToInt();
        TransCommandEx.LParam = l_OriginalPacket->sItem.iChk1;
        TransCommandEx.SParam = l_OriginalPacket->sItem.iChk2;
        TransCommandEx.EParam = l_OriginalPacket->sItem.iGold ^ PKTHDR_GetItem ^ l_OriginalPacket->sItem.iChk2;
        TransCommandEx.WxParam = l_OriginalPacket->dwSecCode[0];
        TransCommandEx.LxParam = l_OriginalPacket->dwSecCode[1];
        TransCommandEx.SxParam = l_OriginalPacket->dwSecCode[2];
        TransCommandEx.ExParam = l_OriginalPacket->dwSecCode[3];

        uint32_t l_ExceptedHash = hash_data((char*)&TransCommandEx, sizeof(PacketTransCommandEx));

        if (GAME_SERVER)
        {
            PacketNetPlayerItemPut l_NetPacket;
            l_NetPacket.iHeader         = PKTHDR_NetPlayerItemPut;
            l_NetPacket.iLength         = sizeof(PacketNetPlayerItemPut);
            l_NetPacket.ObjectID        = pcUserData->iID;
            l_NetPacket.ExceptedHash    = l_ExceptedHash;

            NetConnection * l_LoginServer = NETSERVER->GetLoginServerConnection();
            if (l_LoginServer)
                NETSERVER->SendPacket(l_LoginServer->pcSocketData, &l_NetPacket);
        }
        else if (LOGIN_SERVER)
            pcUserData->pcSocketData->u->PlayerItemPutHashs.push_back(l_ExceptedHash);
    }

	pcUserData->pcSocketData->SendPacket((Packet*)pPacket, bEncrypted);
}

void PacketServer::SendPacketBlank( User * pcUser, int iPacket, BOOL bEncrypted )
{
	PacketTransCommand sPacket;
	sPacket.iLength	= sizeof( PacketTransCommand );
	sPacket.iHeader = iPacket;

	SENDPACKET( pcUser, &sPacket, bEncrypted );
}

void PacketServer::SendPacketCommand( User * pcUser, int iPacket, int LParam, int WParam, int SParam, int EParam )
{
	PacketTransCommand sPacket;
	sPacket.iLength	= sizeof( PacketTransCommand );
	sPacket.iHeader = iPacket;
	sPacket.LParam = LParam;
	sPacket.WParam = WParam;
	sPacket.SParam = SParam;
	sPacket.EParam = EParam;

	SENDPACKET( pcUser, &sPacket, FALSE );
}



void PacketServer::SendSkillCastCommandRange( User * pcCaster, int LParam, int WParam, int SParam, int EParam )
{
	PacketTransCommand sPacket;
	sPacket.iLength	= sizeof( PacketTransCommand );
	sPacket.iHeader = PacketsHeader::PKTHDR_SkillCastData;
	sPacket.LParam = LParam;
	sPacket.WParam = WParam;
	sPacket.SParam = SParam;
	sPacket.EParam = EParam;

	for ( int i = 0; i < PLAYERS_MAX; i++ )
	{
		User * u = USERDATATOUSER( &USERSDATA[i] );
		if ( u && u->IsValidAndInGame() && u->pcUserData->iMapID == pcCaster->GetMapID())
		{
			if ( pcCaster->pcUserData->sPosition.WithinXZDistance(&u->pcUserData->sPosition, DISTANCE_MAX_SKILL_VISUAL, DISTANCE_MISC_Y_EX ) )
			{
				SENDPACKET(u, &sPacket, FALSE);
			}
		}
	}
}

void PacketServer::SendPacketStage( void * pPacket, int iStageID, BOOL bEncrypt )
{
	for ( int i = 0; i < PLAYERS_MAX; i++ )
	{
		User * u = USERDATATOUSER( &USERSDATA[i] );
		if ( u && u->pcUserData && u->IsConnected() && u->IsInGame() && u->pcUserData->iMapID == iStageID )
		{
			SENDPACKET( u, pPacket, bEncrypt );
		}
	}
}

void PacketServer::SendPacketRange( void * pPacket, Point3D * psPos, int iStageID, int iDistance, BOOL bEncrypt, UserData * p_Exclude )
{
	for ( int i = 0; i < PLAYERS_MAX; i++ )
	{
		UserData* ud = &USERSDATA[i];

		if (ud == p_Exclude)
			continue;

		User * u = USERDATATOUSER( ud );
		if ( u && u->IsValidAndInGame())
		{
			if ( u->GetMapID() == iStageID )
			{
				UserData * ud = u->pcUserData;

				if ( psPos->WithinXZDistance( &ud->sPosition, iDistance, DISTANCE_MISC_Y_EX) )
				{
					SENDPACKET( u, pPacket, bEncrypt );
				}
			}
		}
	}
}