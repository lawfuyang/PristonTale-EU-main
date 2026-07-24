#include "stdafx.h"
#include "netserver.h"
#include "UserServer.h"
#include "userdataserver.h"
#include "Logger.h"

int												iListServers = 0;

#define RECVNET_BUFFER_LEN						512


extern void		CheckAndAutoSyncNewSkillsDataforGMsOnly ( User * pcGMUser );
extern void		CheckAndAutoSyncNewSkillsInfoforGMsOnly ( User * pcGMUser );


NetServer::NetServer()
{
}


NetServer::~NetServer()
{
}

void NetServer::Reconnect( class SocketData * pcSocketData )
{

}

void NetServer::Connect( NetConnection * nc )
{
	nc->pcSocketData = SOCKETSERVER->Connect( nc->szIP, nc->iPort );

	if ( nc->pcSocketData )
	{
		User * u = nc->pcSocketData->u;
		if ( u )
		{
			u->bNetServer						= TRUE;
			nc->pcSocketData->bKeepAlive		= TRUE;
			nc->pcSocketData->GetSocketDataContext()->InterServerAuthed = true;
		}
		PacketNetIdentifier sPacket;
		sPacket.iLength		= sizeof( PacketNetIdentifier );
		sPacket.iHeader		= PKTHDR_NetIdentifier;
		sPacket.iServerID	= SERVER_CODE;
		sPacket.iPort		= SERVER_PORT;
		sPacket.uPasswordNUM = 0x45821547;
		SendPacket( nc->pcSocketData, &sPacket );
	}
}

void NetServer::AddConnection( char * pszIP, int iPort, int iServerType )
{
	NetConnection * psClient	= new NetConnection;
	STRINGCOPY( psClient->szIP, pszIP );
	psClient->iPort				= iPort;
	psClient->iServerType		= iServerType;
	psClient->pcSocketData		= NULL;
	vNetConnections.push_back( psClient );
}

NetConnection * NetServer::AddNetConnection( class SocketData * pcSocketData )
{
	NetConnection * psClient	= new NetConnection;
	STRINGCOPY( psClient->szIP, pcSocketData->szIP );
	psClient->iPort				= 0;
	psClient->iServerType		= GAME_SERVER == TRUE ? 0 : -1;
	psClient->pcSocketData		= pcSocketData;
	vNetConnections.push_back( psClient );
	return psClient;
}

BOOL NetServer::Disconnected( class SocketData * sd )
{
	for( std::vector<NetConnection*>::iterator it = vNetConnections.begin(); it != vNetConnections.end(); it++ )
	{
		NetConnection * nc = *it;

		if( nc->pcSocketData == sd )
		{
			nc->pcSocketData = NULL;
			Connect( nc );

			return TRUE;
		}
	}
	return FALSE;
}


void NetServer::ConnectAll()
{
	for ( std::vector<NetConnection*>::iterator it = vNetConnections.begin(); it != vNetConnections.end(); it++ )
	{
		NetConnection * nc = *it;
		Connect( nc );
	}
}

void NetServer::DisconnectAll()
{
	for ( std::vector<NetConnection*>::iterator it = vNetConnections.begin(); it != vNetConnections.end(); it++ )
	{
		NetConnection * nc = *it;
		CALL_WITH_ARG1( 0x0055C390, (DWORD)nc->pcSocketData );
	}
}

NetConnection * NetServer::GetLoginServerConnection()
{
	for ( std::vector<NetConnection*>::iterator it = vNetConnections.begin(); it != vNetConnections.end(); it++ )
	{
		NetConnection * nc = *it;
		if ( nc->iServerType == 0 )
			return nc;
	}
	return NULL;
}

void NetServer::SendCommandGameServers( enum PacketsHeader ePacketHeaderId, int iValue1, int iValue2, int iValue3, int iValue4 )
{
	if (LOGIN_SERVER)
	{
		PacketCommand sPacket{};
		sPacket.iLength	= sizeof( PacketCommand );
		sPacket.iHeader = ePacketHeaderId;
		sPacket.p1 = iValue1;
		sPacket.p2 = iValue2;
		sPacket.p3 = iValue3;
		sPacket.p4 = iValue4;

		for ( std::vector<NetConnection*>::iterator it = vNetConnections.begin(); it != vNetConnections.end(); it++ )
		{
			NetConnection * nc = *it;
			if (nc->iServerType != 0 && nc->iServerType != -1)
				SendPacket( nc->pcSocketData, &sPacket );
		}
	}
	else
	{
		WARN( "SendPacketGameServers called on GAME_SERVER for packet id = %d", ePacketHeaderId );
	}
}

void NetServer::SendPacketGameServers( void * packet )
{
	if (LOGIN_SERVER)
	{
		for ( std::vector<NetConnection*>::iterator it = vNetConnections.begin(); it != vNetConnections.end(); it++ )
		{
			NetConnection * nc = *it;
			if (nc->iServerType != 0 && nc->iServerType != -1)
				SendPacket( nc->pcSocketData, packet );
		}
	}
	else
	{
		int iPacketID = ((Packet*)packet)->iHeader;
		WARN( "SendPacketGameServers called on GAME_SERVER for packet id = %d", iPacketID );
	}
}

/// <summary>
/// Also update Login server
/// See below, i.e. search for PKTHDR_NetChristmasEvent
/// </summary>
void NetServer::SyncEventStatusToLoginServer ( enum PacketsHeader netHeader, BOOL enabled )
{
	PacketNetEvent sPacket;
	sPacket.iLength = sizeof ( PacketNetEvent );
	sPacket.iHeader = netHeader;
	sPacket.Enabled = enabled;

	NetConnection * nc = GetLoginServerConnection ();
	if ( nc )
	{
		SendPacket ( nc->pcSocketData, &sPacket );
	}
}


/**
Also enable / disable aging event
on game servers for aging UI behavior
*/
void NetServer::SetAgingEvent(BOOL bFreeAging, BOOL bNoBreak, BOOL bHalfPrice)
{
	PacketNetAgingEvent sPacket;
	sPacket.iLength = sizeof(PacketNetAgingEvent);
	sPacket.iHeader = PKTHDR_NetAgingEvent;
	sPacket.FreeEnabled = bFreeAging;
	sPacket.NoBreakEnabled = bNoBreak;
	sPacket.HalfPrice = bHalfPrice;

	NETSERVER->SendPacketToLoginServer(&sPacket);
}


void NetServer::SendBotAdd( void * pPacket )
{
	if ( GAME_SERVER )
	{
		NetConnection * nc = GetLoginServerConnection();
		if ( nc )
			SendPacket( nc->pcSocketData, pPacket );
	}
}

void NetServer::SendExp( UserData *pcUserData, INT64 iExp )
{
	if ( pcUserData && GAME_SERVER )
	{
		PacketNetExp sPacket;
		sPacket.iLength = sizeof( PacketNetExp );
		sPacket.iHeader = PKTHDR_NetExp;
		sPacket.uObjectID = pcUserData->iID;
		sPacket.iExp = iExp;
		NetConnection * nc = GetLoginServerConnection();
		if ( nc )
			SendPacket( nc->pcSocketData, &sPacket );
	}
}

void NetServer::GiveEXP( UserData *pcUserData, INT64 iExp )
{
	if ( pcUserData && GAME_SERVER )
	{
		PacketNetExp sPacket;
		sPacket.iLength = sizeof( PacketNetExp );
		sPacket.iHeader = PKTHDR_NetGiveExp;
		sPacket.uObjectID = pcUserData->iID;
		sPacket.iExp = iExp;
		NetConnection * nc = GetLoginServerConnection();
		if ( nc )
			SendPacket( nc->pcSocketData, &sPacket );
	}
}

void NetServer::SendExpLogin( UserData *pcUserData, INT64 iExp )
{
	if ( pcUserData )
	{
		PacketNetExp sPacket;
		sPacket.iLength = sizeof( PacketNetExp );
		sPacket.iHeader = PKTHDR_NetExp;
		sPacket.uObjectID = pcUserData->iID;
		sPacket.iExp = iExp;
		SendPacketGameServers( &sPacket );
	}
}

//PacketLoginSyncData
void NetServer::SendLoginUserMiscDataToGameServers( User * pcUserData )
{
	if ( pcUserData )
	{
		/*PacketNetLogin sPacket;
		sPacket.iLength = sizeof( PacketNetExp );
		sPacket.iHeader = PKTHDR_NetExp;
		sPacket.uObjectID = pcUserData->iID;
		sPacket.iExp = iExp;
		SendPacketGameServers( &sPacket );*/
	}
}

/// <summary>
/// Obsolete
/// </summary>
/// <param name="pcUserData"></param>
/// <param name="bCity"></param>
void NetServer::SendForceOrbToLoginServer( UserData * pcUserData, BOOL bCity, int iForceOrbTime )
{
	if ( pcUserData && GAME_SERVER )
	{
		PacketNetForceOrbTime sPacket;
		sPacket.iLength		= sizeof( PacketNetForceOrbTime );
		sPacket.iHeader		= PKTHDR_NetForceOrb;
		sPacket.dwObjectID	= pcUserData->iID;
		sPacket.bUpdate		= bCity;
		sPacket.dwTime		= iForceOrbTime;
		NetConnection * nc = GetLoginServerConnection();
		if ( nc )
			SendPacket( nc->pcSocketData, &sPacket );
	}
}

void NetServer::SendEXPPotion( UserData * pcUserData, int iEXPPotion )
{
	if ( pcUserData && GAME_SERVER )
	{
		PacketNetEXPPotion sPacket;
		sPacket.iLength		= sizeof( PacketNetEXPPotion );
		sPacket.iHeader		= PKTHDR_NetPremiumEXP;
		sPacket.dwObjectID	= pcUserData->iID;
		sPacket.iEXPPotion		= iEXPPotion;
		NetConnection * nc = GetLoginServerConnection();
		if ( nc )
			SendPacket( nc->pcSocketData, &sPacket );
	}
}


void NetServer::SendClan( UserData *pcUserData )
{
	if ( pcUserData && GAME_SERVER )
	{
		PacketNetClan sPacket;
		sPacket.iLength = sizeof( PacketNetClan );
		sPacket.iHeader = PKTHDR_NetClan;
		sPacket.uObjectID = pcUserData->iID;
		sPacket.dwClan = pcUserData->iClanID;
		NetConnection * nc = GetLoginServerConnection();
		if ( nc )
			SendPacket( nc->pcSocketData, &sPacket );
	}
}
void NetServer::SendSyncChar( UserData *pcUserData )
{
	if ( pcUserData )
	{
		PacketNetCharacterSync sPacket;
		sPacket.iLength = sizeof( PacketNetCharacterSync );
		sPacket.iHeader = PKTHDR_NetCharacterSync;
		sPacket.dwObjectID = pcUserData->iID;
		STRINGCOPY( sPacket.szCharName, pcUserData->sCharacterData.szName );

		NETSERVER->SendPacketToLoginServer( &sPacket );

		DEBUG( "SendSyncChar ID = %d", pcUserData->iID );
	}
	else
	{
		WARN( "SendSyncChar failed. pcUserData is NULL" );
	}
}

void NetServer::DisconnectUser( int iUserID )
{
	PacketNetDisconnectUser sPacket;
	sPacket.iLength = sizeof( PacketNetDisconnectUser );
	sPacket.iHeader = PKTHDR_NetDisconnectUser;
	sPacket.dwObjectID = iUserID;

	if ( GAME_SERVER )
	{
		NETSERVER->SendPacketToLoginServer( &sPacket );
	}
	else
	{
		NETSERVER->SendPacketGameServers( &sPacket );
	}
}

void NetServer::DisconnectUser( UserData *pcUserData )
{
	if ( pcUserData )
	{
		PacketNetDisconnectUser sPacket;
		sPacket.iLength = sizeof( PacketNetDisconnectUser );
		sPacket.iHeader = PKTHDR_NetDisconnectUser;
		sPacket.dwObjectID = pcUserData->iID;
		if ( GAME_SERVER )
		{
			NETSERVER->SendPacketToLoginServer( &sPacket );
		}
		else
		{
			NETSERVER->SendPacketGameServers( &sPacket );
		}
	}
	else
	{
		WARN( "DisconnectUser failed. pcUserData is NULL" );
	}
}

void NetServer::OnUnloadUser( UserData * pcUserData )
{
	if ( pcUserData)
	{
		PacketNetUnloadUser sPacket;
		sPacket.iLength = sizeof( PacketNetUnloadUser );
		sPacket.iHeader = PKTHDR_NetUnloadUser;
		sPacket.dwObjectID = pcUserData->iID;
		SendPacketGameServers( &sPacket );
	}
	else
	{
		WARN( "OnUnloadUser failed. pcUserData is NULL" );
	}
}

void NetServer::SyncGameMaster( UserData * pcUserData )
{
	if ( pcUserData )
	{
		PacketNetGameMaster sPacket;
		sPacket.iLength = sizeof( PacketNetGameMaster );
		sPacket.iHeader = PKTHDR_NetGameMaster;
		sPacket.uObjectID = pcUserData->iID;
		sPacket.iGameMasterLevel = pcUserData->iGameLevel;

		if (GAME_SERVER)
		{
			NETSERVER->SendPacketToLoginServer( &sPacket );
		}
		else
		{
			NETSERVER->SendPacketGameServers( &sPacket );
		}
	}
}

void NetServer::LoadQuestData( User * pcUser )
{
	NETSERVER->SendCommandToLoginServer( PKTHDR_NetLoadQuestData, pcUser->GetID() );
}

void NetServer::LoadPVPData( User * pcUser )
{
	NETSERVER->SendCommandToLoginServer( PKTHDR_NetLoadPVPData, pcUser->GetID() );
}

void NetServer::OnGetGameMaster( UserData * pcUserData )
{
	if ( pcUserData )
	{
		PacketNetGetGameMaster sPacket;
		sPacket.iLength = sizeof( PacketNetGetGameMaster );
		sPacket.iHeader = PKTHDR_NetGetGameMaster;
		sPacket.uObjectID = pcUserData->iID;
		sPacket.iGameMasterLevel = pcUserData->iGameLevel;
		STRINGCOPY( sPacket.szName, pcUserData->sCharacterData.szName );
		NETSERVER->SendPacketToLoginServer( &sPacket );
	}
}

void NetServer::GetExp( UserData * pcUserData )
{
	PacketNetGetExp sPacket;
	sPacket.iLength = sizeof( PacketNetGetExp );
	sPacket.iHeader = PKTHDR_NetGetExp;
	sPacket.uObjectID = pcUserData->iID;
	SendPacketGameServers( &sPacket );
}

void NetServer::SendTradeCoin(int iID, int iReceiverID)
{
	PacketNetTradeCoin s;
	s.iLength = sizeof(PacketNetTradeCoin);
	s.iHeader = PKTHDR_NetTradeCoin;
	s.iID = iID;
	s.iReceiverID = iReceiverID;
	SendPacketGameServers(&s);
}

void NetServer::SendQuestData( User * pcUser )
{
	PacketNetT5QuestData s;
	s.iLength			= sizeof( PacketNetT5QuestData );
	s.iHeader			= PKTHDR_NetQuestT5Data;
	s.iID				= pcUser->GetID();
	s.uQuestID			= pcUser->uQuestT5ID;
	s.sQuestValue[0]	= pcUser->sQuestT5ProgressValue[0];
	s.sQuestValue[1]	= pcUser->sQuestT5ProgressValue[1];
	SendPacketGameServers( &s );
}

void NetServer::SendQuestCancelOrFinish( User * pcUser, int iQuestID, BOOL bForceFinish )
{
	if ( iQuestID == 0 )
	{
		WARN( "SendQuestCancelOrFinish - QuestID is 0?" );
		return;
	}


	if ( GAME_SERVER )
		return;

	PacketCommand s{};
	s.iLength		= sizeof( PacketCommand );
	s.iHeader		= PKTHDR_NetQuestCancelOrFinish;
	s.p1			= pcUser->GetID();
	s.p2			= iQuestID;
	s.p3			= bForceFinish;

	SendPacketGameServers( &s );
}

void NetServer::SendQuestUpdateDataPart( User * pcUser, PacketQuestUpdateData * data, BOOL bUpdateNPCQuestIcon )
{
	if ( GAME_SERVER )
		return;

	PacketNetQuestUpdateDataPart s{};
	s.iLength			  = sizeof( PacketNetQuestUpdateDataPart );
	s.iHeader			  = PKTHDR_NetQuestUpdateDataPart;
	s.iUserID			  = pcUser->GetID();
	s.bUpdateNPCQuestIcon = bUpdateNPCQuestIcon;
	CopyMemory( &s.sData, data, sizeof( PacketQuestUpdateData ) );

	SendPacketGameServers( &s );
}

void NetServer::SendQuestUpdateData( User * pcUser )
{
	if ( GAME_SERVER )
		return;

	PacketNetQuestUpdateData s;
	s.iLength			= sizeof( PacketNetQuestUpdateData );
	s.iHeader			= PKTHDR_NetQuestUpdateData;
	s.iID				= pcUser->GetID();
	CopyMemory( s.saPacket, pcUser->saPacketQuestData, sizeof( PacketQuestUpdateData ) * MAX_QUESTPACKETDATA );
	SendPacketGameServers( &s );
}

/// <summary>
/// Send packet between servers
/// </summary>
void NetServer::SendPacket( class SocketData * pcSocketData, void * packet )
{
	if (!pcSocketData)
		return;

#if defined (DEV_MODE) && defined (PACKET_LOG)
	/// ok go ahead and compile
	const std::string & l_OpcodeName = SOCKETSERVER->GetOpcodeName(((Packet*)packet)->iHeader);


	if (!PACKETSERVER->IsPacketIgnored((Packet*)packet))
	{
		const std::string extraInfo = PACKETSERVER->GetExtraInfoForPacket((Packet*)packet);
		const std::string str = GAME_SERVER ? "SERVER --> LOGIN" : "SERVER --> GAME";

		if ( !l_OpcodeName.empty() )
		{
			if ( !extraInfo.empty() )
			{
				DEBUGPACKET( "[ %s ] %s -- %s", str.c_str(), l_OpcodeName.c_str(), extraInfo.c_str() );
			}
			else
			{
				DEBUGPACKET( "[ %s ] %s", str.c_str(), l_OpcodeName.c_str() );
			}
		}
		else
		{
			char hexString[16];
			_itoa_s( ( (Packet *)packet )->iHeader, hexString, 16 );
			WARN( "[ %s ] Unknown Opcode. Add packet 0x%s via ADD_OPCODE_NAME", str.c_str(), hexString );
		}
	}


#endif

	pcSocketData->SendPacket( (Packet*)packet, FALSE );
}

/// <summary>
/// Send blank packet to login server from game server
/// As good practice, ensure packet id starts with PKTHDR_Net...
/// See also: NETSERVER->OnReceiveFromGameServer (below)
/// </summary>
/// <param name="packet">Use PKTHDR_Net...</param>
void NetServer::SendCommandToLoginServer ( enum PacketsHeader ePacketHeaderId, int iValue1, int iValue2, int iValue3, int iValue4 )
{
	if ( GAME_SERVER )
	{
		NetConnection * l_LoginServer = NETSERVER->GetLoginServerConnection ();
		if ( l_LoginServer )
		{
			PacketCommand sPacket{};
			sPacket.iLength	= sizeof( PacketCommand );
			sPacket.iHeader = ePacketHeaderId;
			sPacket.p1 = iValue1;
			sPacket.p2 = iValue2;
			sPacket.p3 = iValue3;
			sPacket.p4 = iValue4;

			SendPacket ( l_LoginServer->pcSocketData, &sPacket );
		}
	}
	else
	{
		WARN( "SendCommandToLoginServer called on LOGIN_SERVER for packet id = %d", ePacketHeaderId );
	}
}

/// <summary>
/// Send chat packet to login server from game server
/// This is a short cut to allow the command coming from game
/// server to be handled on login server instead. Particuarlly for inventory related data
/// </summary>
void NetServer::SendGMCommandToLoginServer ( User * pcUser, std::string command )
{
	if ( GAME_SERVER )
	{
		NetConnection * l_LoginServer = NETSERVER->GetLoginServerConnection ();
		if ( l_LoginServer )
		{
			PacketChatMessage sPacket;
			sPacket.iHeader = PKTHDR_NetGameMasterCommand;
			sPacket.dwObjectSerial = pcUser->GetID();
			sPacket.iLength = static_cast<WORD>( sizeof( Packet ) + sizeof( DWORD ) + sizeof( DWORD ) + command.length() + 1 );
			CopyMemory( sPacket.szMessage, command.c_str(), 255 ); //-1 for end of line char

			SendPacket ( l_LoginServer->pcSocketData, &sPacket );
		}
	}
	else
	{
		WARN( "SendGMCommandToLoginServer called on LOGIN_SERVER for packet id = %d", PKTHDR_ChatGame );
	}
}

/// <summary>
/// Send packet to login server from game server
/// As good practice, ensure packet id starts with PKTHDR_Net///
/// See also: NETSERVER->OnReceiveFromGameServer (below)
/// </summary>
/// <param name="packet"></param>
void NetServer::SendPacketToLoginServer ( void * packet )
{
	if ( GAME_SERVER )
	{
		NetConnection * l_LoginServer = NETSERVER->GetLoginServerConnection ();
		if ( l_LoginServer )
		{
			SendPacket ( l_LoginServer->pcSocketData, packet );
		}
	}
	else
	{
		int iPacketID = ((Packet*)packet)->iHeader;
		WARN( "SendPacketToLoginServer called on LOGIN_SERVER for packet id = %d", iPacketID );
	}
}

/// <summary>
/// Login Server received from GameServer
/// As good practice, ensure packet id starts with PKTHDR_Net
/// See also: NETSERVER->SendPacketToLoginServer (above)
/// </summary>
void NetServer::OnReceiveFromGameServer( UserData * pcUserDataServer, void * pPacket )
{
	//Login server only
	if ( GAME_SERVER )
		return;

	UserData * pcUserData = NULL;
	User * pcUser = NULL;

	User * us = USERDATATOUSER( pcUserDataServer );

	int iPacketID = ((Packet*)pPacket)->iHeader;

	NetConnection * nc = NULL;

	if( pcUserDataServer )
	{
		nc = GetNetConnectionBySocketData( pcUserDataServer->pcSocketData );

		if( nc == NULL )
			nc = AddNetConnection( pcUserDataServer->pcSocketData );

		if( nc == NULL )
			return;
	}

	if ( iPacketID == PKTHDR_NetIdentifier )
	{
		for ( std::vector<NetConnection*>::iterator it = vNetConnections.begin(); it != vNetConnections.end(); it++ )
		{
			NetConnection * ncit = *it;
			if ( (ncit->iServerType == ((PacketNetIdentifier*)pPacket)->iServerID) && (((PacketNetIdentifier*)pPacket)->uPasswordNUM == 0x45821547) )
			{
				nc->iPort			= ((PacketNetIdentifier*)pPacket)->iPort;
				ncit->pcSocketData	= nc->pcSocketData;
				us->bNetServer		= TRUE;
				us->pcUserData->pcSocketData->bKeepAlive		= TRUE;
				us->pcUserData->pcSocketData->GetSocketDataContext()->InterServerAuthed = true;
				vNetConnections.pop_back();
				return;
			}
		}
		nc->iServerType	= ((PacketNetIdentifier*)pPacket)->iServerID;
		nc->iPort		= ((PacketNetIdentifier*)pPacket)->iPort;
		us->bNetServer = TRUE;
		us->pcUserData->bKeepAlive = TRUE;
		us->pcUserData->pcSocketData->bKeepAlive = TRUE;
		us->pcUserData->pcSocketData->GetSocketDataContext()->InterServerAuthed = true;
		return;
	}

	switch ( iPacketID )
	{
		case PKTHDR_NetOpenItemDistributor:
		{
			int id = ( (PacketCommand *)pPacket )->p1;
			pcUserData = USERSERVER->GetUserdata ( id );
			if ( pcUserData )
			{
				ITEMSERVER->OpenItemBox ( pcUserData, FALSE );
			}
			else
			{
				WARN( "PKTHDR_NetOpenItemDistributor - User not found for id: %d", id );
			}
			break;
		}

		case PKTHDR_NetOpenWarehouse:
		{
			int id = ( (PacketCommand *)pPacket )->p1;
			pcUser = USERDATATOUSER ( USERSERVER->GetUserdata ( id ) );
			if ( pcUser )
			{
				WAREHOUSEHANDLER->OnWarehouseOpened ( pcUser, FALSE );
			}
			else
			{
				WARN( "PKTHDR_NetOpenWarehouse - User not found for id: %d", id );
			}
		}
		break;

		case PKTHDR_NetGetMuteSync:
		{
			int id = ( (PacketCommand *)pPacket )->p1;
			pcUser = USERDATATOUSER(USERSERVER->GetUserdata( id ));

			if (pcUser)
			{
				if (pcUser->bMuted)
				{

					DWORD dwSystemTime = SYSTEMTIMEDWORD;

					long lDeltaSec = pcUser->dwUnMuteExpiryTime - dwSystemTime;

					float fDeltaHour = static_cast<float>( lDeltaSec / 3600.0f );

					if (fDeltaHour > 8760.0f)
					{
						CHATSERVER->SendChat( pcUser, CHATCOLOR_Error, "> You have been permanently muted" );
						CHATSERVER->SendTitleBox( pcUser->pcUserData, "You are permanently muted!" );
					}
					else
					{
						CHATSERVER->SendChatEx( pcUser, CHATCOLOR_Error, "> You have been muted by a GM. Your mute will expiry in %.1f hours", fDeltaHour );
						CHATSERVER->SendTitleBox( pcUser->pcUserData, "You are muted!" );
					}
				}

				USERSERVER->SyncUserMuteStatus( pcUser ); //sync to game server
			}
			else
			{
				WARN( "PKTHDR_NetGetMuteSync - User not found for id: %d", id );
			}

			break;
		}

		case PKTHDR_NetMuteSync:
		{
			int id = ( (PacketNetMuteSync *)pPacket )->dwObjectID;
			pcUserData = USERSERVER->GetUserdata( id );

			if ( pcUserData )
			{
				pcUser = USERDATATOUSER ( pcUserData );

				if (pcUser)
				{
					DEBUG( "NetMuteSync - User id: %d", id );

					pcUser->bMuted = ( (PacketNetMuteSync *)pPacket )->bMuted;
					pcUser->dwUnMuteExpiryTime = ( (PacketNetMuteSync *)pPacket )->dwUnMuteExpiryTime;
				}
				else
				{
					WARN( "NetMuteSync - User not found for id: %d", id );
				}
			}
			else
			{
				WARN( "NetMuteSync - UserData not found for id: %d", id );
			}

			break;
		}

		case PKTHDR_NetReloadCoinShop:
		{
			COINSHOPHANDLER->LoadCoinShop ();
			CHATSERVER->SendChat ( pcUser, CHATCOLOR_Error, "> Coin Shop Reloaded!" );
		}
		break;

		case PKTHDR_NetVersion:
		{
			SERVERCOMMAND->SetVersion ( ( (PacketCommand *)pPacket )->p1 );
			CHATSERVER->SendChatEx ( pcUser, CHATCOLOR_Error, "> New Version %d!", SERVER_VERSION );
		}
		break;

		case PKTHDR_NetKickAll:
		{
			CHATSERVER->SendChat ( pcUser, CHATCOLOR_Error, "> Kicking...!" );
			for ( int i = 0; i < PLAYERS_MAX; i++ )
			{
				User * pcU = USERDATATOUSER ( &USERSDATA[i] );
				if ( USERSDATA[i].pcSocketData && USERSDATA[i].iGameLevel == GAMELEVEL_None && pcU && !pcU->bNetServer )
				{
					SENDPACKETBLANK ( USERDATATOUSER ( &USERSDATA[i] ), PKTHDR_Disconnect, TRUE );
					WNDPROC_SOCKETCLOSE ( SOCKETSERVER->GetHWND (), USERSDATA[i].pcSocketData );
				}
			}
			CHATSERVER->SendChat ( pcUser, CHATCOLOR_Error, "> All Kicked!" );
		}
		break;

		case PKTHDR_NetClearTickChristmas:
		{
			EVENTSERVER->ClearTickOnlineRewardChristmas ();
			CHATSERVER->SendChat ( pcUser, CHATCOLOR_Error, "> Reward Online Christmas: Cleaned!" );
		}
		break;

		case PKTHDR_NetClearTickRO:
		{
			EVENTSERVER->ClearTickOnlineReward ();
			CHATSERVER->SendChat ( pcUser, CHATCOLOR_Error, "> Reward Online: Cleaned!" );
		}
		break;

		case PKTHDR_NetEXPEvent:
		{
			*(int *)( 0x0084601C ) = ( (PacketCommand *)pPacket )->p1;
		}
		break;

		case PKTHDR_NetOpenWarehouseSeasonal:
		{
			int id = ( (PacketCommand *)pPacket )->p1;
			pcUser = USERDATATOUSER ( USERSERVER->GetUserdata ( id ) );
			if ( pcUser )
			{
				WAREHOUSEHANDLER->OnWarehouseOpened ( pcUser, TRUE );
			}
		}
		break;

		//Bless Castle
		case PKTHDR_NetBCStatusReq:
			BLESSCASTLESERVER->NetSendBlessCastleStatus( nc );
			break;
		case PKTHDR_NetBCStatusSnd:
			BLESSCASTLESERVER->NetRecvBlessCastleStatus( (PacketNetBCStatus*)pPacket );
			break;
		case PKTHDR_NetBCNewOwnerClanID:
			BLESSCASTLESERVER->SetBlessCastleOwnerClanID( ((PacketNetBCNewOwnerClanID *)pPacket)->iClanID1, ((PacketNetBCNewOwnerClanID *)pPacket)->iClanID2, ((PacketNetBCNewOwnerClanID *)pPacket)->iClanID3 );
			break;

		case PKTHDR_NetRemoveFinishedQuestData:
			QUESTSERVER->RemoveQuestFinishData( USERDATATOUSER( USERSERVER->GetUserdata( ( (PacketCommand *)pPacket )->p1 ) ), ((PacketCommand *)pPacket )->p2 );
			break;

		case PKTHDR_NetAgingEvent:
		{
			auto packetAging = ( (PacketNetAgingEvent *)pPacket );
			SERVERCOMMAND->SetAgingEvent ( packetAging->FreeEnabled, packetAging->NoBreakEnabled, packetAging->HalfPrice );
		}
		break;

		//Experience
		case PKTHDR_NetExp:
			pcUserData = USERSERVER->GetUserdata( ((PacketNetExp*)pPacket)->uObjectID );
			pcUser = USERDATATOUSER( pcUserData );
			if ( pcUserData && USERDATATOUSER(pcUserData) )
			{
				CHARACTERSERVER->SetExp( pcUserData, ((PacketNetExp*)pPacket)->iExp );
				pcUser->bReceivedExp = TRUE;
			}
			break;
		case PKTHDR_NetGiveExp:
			pcUserData = USERSERVER->GetUserdata( ((PacketNetExp*)pPacket)->uObjectID );
			pcUser = USERDATATOUSER( pcUserData );
			if ( pcUserData && pcUser )
			{
				CHARACTERSERVER->GiveEXP( pcUser, ((PacketNetExp*)pPacket)->iExp );
			}
			break;


		case PKTHDR_NetLoadQuestData:
			pcUserData = USERSERVER->GetUserdata( ((PacketCommand*)pPacket)->p1 );
			pcUser = USERDATATOUSER( pcUserData );
			if ( pcUserData && pcUser )
			{
				QUESTSERVER->OnLoadUser(pcUser);
			}
			break;

		case PKTHDR_NetLoadPVPData:
			pcUserData = USERSERVER->GetUserdata( ((PacketCommand*)pPacket)->p1 );
			pcUser = USERDATATOUSER( pcUserData );
			if ( pcUserData && pcUser )
			{
				PVPSERVER->OnLoadUser(pcUser);
			}
			break;

		case PKTHDR_NetGameMaster:
			pcUserData = USERSERVER->GetUserdata( ((PacketNetGameMaster*)pPacket)->uObjectID );
			if (pcUserData)
			{
				pcUserData->iGameLevel = (EGameLevel)( (PacketNetGameMaster *)pPacket )->iGameMasterLevel;
			}
			else
			{
				WARN( "PKTHDR_NetGameMaster failed - UserData not found for id = %d", ((PacketNetGameMaster*)pPacket)->uObjectID );
			}
			break;

		case PKTHDR_NetGetGameMaster:
			pcUserData = USERSERVER->GetUserdata( ((PacketNetGetGameMaster*)pPacket)->szName );
			if ( pcUserData )
			{
				((PacketNetGetGameMaster*)pPacket)->iGameMasterLevel = pcUserData->iGameLevel;
				SendPacket( nc->pcSocketData, pPacket );

				NETSERVER->SendQuestData( USERDATATOUSER( pcUserData ) );
			}
			else
			{
				WARN( "PKTHDR_NetGetGameMaster failed - UserData not found for name = %s", ((PacketNetGetGameMaster*)pPacket)->szName );
			}
		break;

		case PKTHDR_NetDisconnectUser:
			pcUserData = USERSERVER->GetUserdata( ((PacketNetGameMaster*)pPacket)->uObjectID );
			pcUser = USERDATATOUSER( pcUserData );
			if ( pcUserData && pcUserData->pcSocketData && pcUser )
			{
#ifdef RECONNECT_SYSTEM
				if ( pcUserData->iID )
					SOCKETSERVER->AddReconnectUser( pcUserData );
#endif

				ACCOUNTSERVER->OnDisconnectUser( pcUserData );
				LOGSERVER->LogSocketClose( pcUserData->pcSocketData, "NetServer::OnReceiveFromGameServer(PKTHDR_NetDisconnectUser)", _ReturnAddress() );
				WNDPROC_SOCKETCLOSE( SOCKETSERVER->GetHWND(), pcUserData->pcSocketData );
			}
			break;

		case PKTHDR_NetForceOrb:
			pcUserData = USERSERVER->GetUserdata( ((PacketNetForceOrbTime*)pPacket)->dwObjectID );
			if ( pcUserData && pcUserData->pcSocketData )
			{
				pcUserData->dwForceOrb_SaveTime_DoNotUse		= ((PacketNetForceOrbTime*)pPacket)->dwTime;
			}
			break;

		case PKTHDR_NetPremiumEXP:
			pcUserData = USERSERVER->GetUserdata( ((PacketNetEXPPotion*)pPacket)->dwObjectID );
			pcUser = USERDATATOUSER( pcUserData );
			if ( pcUserData && pcUserData->pcSocketData && pcUser )
			{
				pcUser->iEXPPotion = ((PacketNetEXPPotion*)pPacket)->iEXPPotion;
			}
			break;

		case PKTHDR_NetCharacterSync:
		{
			int id = ( (PacketNetCharacterSync *)pPacket )->dwObjectID;
			pcUserData = USERSERVER->GetUserdata( id );
			if (pcUserData && pcUserData->pcSocketData)
			{
				pcUserData->pcSocketData->u->LastInventoryHash = 0xFFFFFFFF;
				pcUserData->pcSocketData->u->LastGoldHash = 0xFFFFFFFF;

				STRINGCOPY( pcUserData->szCharacterName, ( (PacketNetCharacterSync *)pPacket )->szCharName );
			}
			else
			{
				WARN( "PKTHDR_NetCharacterSync failed - UserData not found for id = %d", id );
			}
			break;
		}

		case PKTHDR_NetBotAdd:
			BOTSERVER->AddGameBot( (PacketBotLogin*)pPacket );
			break;

		case PKTHDR_NetClan:
			pcUserData = USERSERVER->GetUserdata( ((PacketNetGameMaster*)pPacket)->uObjectID );
			pcUser = USERDATATOUSER( pcUserData );
			if ( pcUserData && pcUser && pcUser->dwTimeSync )
			{
				DWORD dwOldClan;
				dwOldClan = pcUserData->iClanID;

				if ( pcUserData )
					pcUserData->iClanID = ((PacketNetClan*)pPacket)->dwClan;

				if ( !GAME_SERVER && pcUserData->iClanID && pcUserData->iClanID != dwOldClan )
				{
					int iServerID = pcUser->pData.dwServerID;
					if ( iServerID < 0 || iServerID > 3 )
						iServerID = 0;

					CHATSERVER->SendChatAllUsersInClan( pcUserData, CHATCOLOR_Clan, "> %s is now online at %s", pcUserData->sCharacterData.szName, GSERVER->GetServerName( iServerID ) );
				}
			}
			break;

		case PKTHDR_NetUsersOnline:
			LOGSERVER->HandlePacket( (PacketNetUsersOnline*)pPacket );
			break;

		case PKTHDR_NetQuestT5Data:
			QUESTSERVER->HandlePacket( (PacketNetT5QuestData*)pPacket );
			break;

		case PKTHDR_NetStartMaintenance:
		{
			GSERVER->StartMaintenanceCountdown ( reinterpret_cast<PacketCommand*>( pPacket )->p1 );
			break;
		}
		case PKTHDR_NetStopMaintenance:
		{
			GSERVER->StopMaintenanceCountdown ( );
			break;
		}

        case PKTHDR_NetPlayerGoldDiff:
        {
            PacketNetPlayerGoldDiff * l_GoldPacket  = reinterpret_cast<PacketNetPlayerGoldDiff*>(pPacket);
            UserData                * l_UserData    = USERDATABYID(l_GoldPacket->ObjectID);

            if (l_UserData)
            {
                l_UserData->iInventoryGold         += l_GoldPacket->Diff;
                l_UserData->iSaveGold               = l_UserData->GetGold();
                l_UserData->sCharacterData.iGold    = l_UserData->GetGold();

                PacketSetCharacterGold sPacket;
                sPacket.iHeader = PKTHDR_SetGold;
                sPacket.iLength = sizeof(PacketSetCharacterGold);
                sPacket.dwGold  = l_UserData->GetGold();

                PACKETSERVER->Send(l_UserData->pcSocketData->u, &sPacket);
            }
			else
			{
				WARN ( "PKTHDR_NetPlayerGoldDiff USER NOT FOUND" );
			}

            break;
        }

		case PKTHDR_NetEasterEvent:
		{
			PacketNetEvent* l_Packet = reinterpret_cast<PacketNetEvent*>(pPacket);
			SERVERCOMMAND->SetEasterEvent( l_Packet->Enabled );
			break;
		}

		case PKTHDR_NetStarWarsEvent:
		{
			PacketNetEvent* l_Packet = reinterpret_cast<PacketNetEvent*>(pPacket);
			SERVERCOMMAND->SetStarWarsEvent( l_Packet->Enabled );
			break;
		}

		case PKTHDR_NetBeeEvent:
		{
			PacketNetEvent* l_Packet = reinterpret_cast<PacketNetEvent*>(pPacket);
			SERVERCOMMAND->SetBeeEvent( l_Packet->Enabled );
			break;
		}

		case PKTHDR_NetGameMasterCommand:
		{
			PacketChatMessage* l_Packet = reinterpret_cast<PacketChatMessage*>(pPacket);
			SERVERCOMMAND->OnGameMasterCommand( USERDATATOUSER( USERDATABYID( l_Packet->dwObjectSerial ) ), l_Packet->szMessage );
			break;
		}

		case PKTHDR_NetPlayDataEx:
		{
			PacketPlayDataEx * l_Packet = reinterpret_cast<PacketPlayDataEx*>(pPacket);
			UserData         * l_UserData    = USERDATABYID(l_Packet->userID);

			if ( l_UserData )
			{
				CHARACTERSERVER->OnCharacterSyncDataEx( l_UserData, l_Packet );
			}
			else
			{
				WARN( "PKTHDR_NetPlayDataEx - user data not found for %d", l_Packet->userID );
				//NETSERVER->DisconnectUser( l_Packet->userID );
			}
			break;
		}

		case PKTHDR_NetSendItemData:
		{
			PacketNetSendItemData * l_Packet = reinterpret_cast<PacketNetSendItemData*>(pPacket);
			UserData         * l_UserData    = USERDATABYID(l_Packet->dwUserID);

			if ( l_UserData )
			{
				ITEMSERVER->SendItemData( USERDATATOUSER(l_UserData), l_Packet->iItemID, l_Packet->eItemSource );
			}
			else
			{
				WARN( "PKTHDR_NetSendItemData - user data not found for %d", l_Packet->dwUserID );
				//NETSERVER->DisconnectUser( l_Packet->userID );
			}
			break;
		}

		case PKTHDR_NetHallowenEvent:
		{
			PacketNetEvent * l_Packet = reinterpret_cast<PacketNetEvent*>(pPacket);
			SERVERCOMMAND->SetHalloweenEvent( l_Packet->Enabled );
			break;
		}
		case PKTHDR_NetMimicEvent:
		{
			PacketNetEvent * l_Packet = reinterpret_cast<PacketNetEvent*>(pPacket);
			SERVERCOMMAND->SetMimicEvent( l_Packet->Enabled );
			break;
		}
		case PKTHDR_NetChristmasEvent:
		{
			PacketNetEvent * l_Packet = reinterpret_cast<PacketNetEvent *>( pPacket );
			SERVERCOMMAND->SetChristmasEvent( l_Packet->Enabled );
			break;
		}
		case PKTHDR_NetFreeEventGirl:
		{
			PacketNetEvent * l_Packet = reinterpret_cast<PacketNetEvent *>( pPacket );
			SERVERCOMMAND->SetEventGirlFree( l_Packet->Enabled );

			break;
		}

        case PKTHDR_NetPlayerItemPut:
        {
            PacketNetPlayerItemPut  * l_ItemPacket  = reinterpret_cast<PacketNetPlayerItemPut *>(pPacket);
            UserData                * l_UserData    = USERDATABYID(l_ItemPacket->ObjectID);

			if ( l_UserData )
				l_UserData->pcSocketData->u->PlayerItemPutHashs.push_back ( l_ItemPacket->ExceptedHash );
			else
				WARN ( "PKTHDR_NetPlayerItemPut USER NOT FOUND" );

            break;
        }

		//received from game server via
		//BellatraServer::OnLoadUser
		case PKTHDR_NetBellatraSoloRanking:
		{
			auto pCommand = reinterpret_cast<PacketCommand*>(pPacket);
			pcUserData = USERSERVER->GetUserdata(pCommand->p1); //dwObjectID
			pcUser = USERDATATOUSER(pcUserData);
			if (pcUserData && pcUser)
			{
				pcUser->sBellatraSoloCrown = pCommand->p2; //solo crown ranking
				INFO ( "pcUser->iBellatraSoloCrownRanking = %d ", pcUser->sBellatraSoloCrown );
			}
			else
			{
				WARN ( "pcUser->iBellatraSoloCrownRanking = pcuserData or pcUser is null!" );
			}
		}
		break;

		/*case PKTHDR_NetPlayerThrow:
		{
			PacketNetPlayerThrow * l_Packet   = reinterpret_cast<PacketNetPlayerThrow*>(pPacket);
			UserData             * l_UserData = USERDATABYID(l_Packet->ObjectID);

			if (l_UserData)
			{
				bool l_Failed = false;

				if (!ITEMSERVER->DeleteItemInventory(l_UserData, l_Packet->Item.iItemID, l_Packet->Item.iChk1, l_Packet->Item.iChk2))
					l_Failed = true;

				if (!USERSERVER->SubServerUserGold(l_UserData, l_Packet->Gold, (EWhereAction)120))
					l_Failed = true;

				l_UserData->iSaveGold = l_UserData->GetGold() - l_Packet->Gold;

				if (!l_UserData->iSaveGold)
					l_Failed = true;

				if (l_Failed)
				{
					SOCKETSERVER->AddBanIP(pcUserData->pcSocketData->lIP, "THROW NOT OWNED ITEM / GOLD");

					DisconnectUser(pcUserData);

					SENDPACKETBLANK(USERDATATOUSER(pcUserData), PKTHDR_Disconnect);
					WNDPROC_SOCKETCLOSE(SOCKETSERVER->GetHWND(), pcUserData->pcSocketData);
				}
			}
			else
				std::cout << "PKTHDR_NetPlayerGold USER NOT FOUND" << std::endl;

			break;
		}*/

		default:
			break;
	}
}

void NetServer::OnReceiveClient( UserData * pcUserDataServer, void * pPacket )
{
	UserData * pcUserData = NULL;

	User * us = USERDATATOUSER( pcUserDataServer );

	NetConnection * nc = NULL;

	User * pcUser = NULL;

	int iPacketID = ((Packet*)pPacket)->iHeader;

	if( pcUserDataServer )
	{
		nc = GetNetConnectionBySocketData( pcUserDataServer->pcSocketData );

		if( nc == NULL )
			nc = AddNetConnection( pcUserDataServer->pcSocketData );

		if( nc == NULL )
			return;
	}

	if ( iPacketID == PKTHDR_NetIdentifier )
	{
		for ( std::vector<NetConnection*>::iterator it = vNetConnections.begin(); it != vNetConnections.end(); it++ )
		{
			NetConnection * ncit = *it;
			if ( (ncit->iServerType == ((PacketNetIdentifier*)pPacket)->iServerID) && (((PacketNetIdentifier*)pPacket)->uPasswordNUM == 0x45821547) )
			{
				nc->iPort			= ((PacketNetIdentifier*)pPacket)->iPort;
				ncit->pcSocketData	= nc->pcSocketData;
				us->bNetServer		= TRUE;
				us->pcUserData->pcSocketData->bKeepAlive		= TRUE;
				us->pcUserData->pcSocketData->GetSocketDataContext()->InterServerAuthed = TRUE;
				vNetConnections.pop_back();
				return;
			}
		}
		nc->iServerType	= ((PacketNetIdentifier*)pPacket)->iServerID;
		nc->iPort		= ((PacketNetIdentifier*)pPacket)->iPort;
		us->bNetServer = TRUE;
		us->pcUserData->pcSocketData->bKeepAlive = TRUE;
		us->pcUserData->pcSocketData->GetSocketDataContext()->InterServerAuthed = TRUE;
		return;
	}

	switch ( iPacketID )
	{
		case PKTHDR_NetSyncFinishedQuestData:
		{
			pcUserData = USERSERVER->GetUserdata( ((PacketSyncFinishedQuestData *)pPacket)->dwUserID  );
			pcUser = USERDATATOUSER( pcUserData );

			if ( pcUser )
			{
				QUESTSERVER->HandlePacket( pcUser,  (PacketSyncFinishedQuestData *)pPacket );
			}
			else
			{
				WARN( "[PKTHDR_NetSyncFinishedQuestData] User %d does not exist", ( (PacketSyncFinishedQuestData *)pPacket )->dwUserID );
			}
		}
		break;

		// BC
		case PKTHDR_NetBCStatusReq:
			BLESSCASTLESERVER->NetSendBlessCastleStatus( nc );
			break;
		case PKTHDR_NetBCStatusSnd:
			BLESSCASTLESERVER->NetRecvBlessCastleStatus( (PacketNetBCStatus*)pPacket );
			break;
		case PKTHDR_NetBCNewOwnerClanID:
			BLESSCASTLESERVER->SetBlessCastleOwnerClanID( ((PacketNetBCNewOwnerClanID *)pPacket)->iClanID1, ((PacketNetBCNewOwnerClanID *)pPacket)->iClanID2, ((PacketNetBCNewOwnerClanID *)pPacket)->iClanID3 );
			break;

		case PKTHDR_NetMuteSync:
		{
			int id = ( (PacketNetMuteSync *)pPacket )->dwObjectID;
			pcUserData = USERSERVER->GetUserdata( id );

			if ( pcUserData )
			{
				pcUser = USERDATATOUSER ( pcUserData );

				if (pcUser)
				{
					INFO( "NetMuteSync - User id: %d", id );

					pcUser->bMuted = ( (PacketNetMuteSync *)pPacket )->bMuted;
					pcUser->dwUnMuteExpiryTime = ( (PacketNetMuteSync *)pPacket )->dwUnMuteExpiryTime;
				}
				else
				{
					WARN( "NetMuteSync - User not found for id: %d", id );
				}
			}
			else
			{
				WARN( "NetMuteSync - UserData not found for id: %d", id );
			}

			break;
		}

		case PKTHDR_NetDisconnectUser:
			pcUserData = USERSERVER->GetUserdata( ((PacketNetGameMaster*)pPacket)->uObjectID );
			if ( pcUserData && pcUserData->pcSocketData )
			{
				pcUser = USERDATATOUSER( pcUserData );
				if ( pcUser )
					pcUser->bDisconnectNet = TRUE;

#ifdef RECONNECT_SYSTEM
				if ( pcUser && pcUserData->iID )
					SOCKETSERVER->AddReconnectUser( pcUserData );
#endif

				ACCOUNTSERVER->OnDisconnectUser( pcUserData );
				LOGSERVER->LogSocketClose( pcUserData->pcSocketData, "NetServer::OnReceiveFromGameServer(PKTHDR_NetDisconnectUser)", _ReturnAddress() );
				WNDPROC_SOCKETCLOSE( SOCKETSERVER->GetHWND(), pcUserData->pcSocketData );
			}
			break;

		case PKTHDR_NetTradeCoin:
			TRADEHANDLER->HandleTradeCoin(USERDATATOUSER(USERDATABYID(((PacketNetTradeCoin*)pPacket)->iID)), USERDATATOUSER(USERDATABYID(((PacketNetTradeCoin*)pPacket)->iReceiverID)));
			break;

		case PKTHDR_NetUnloadUser:
			pcUserData = USERSERVER->GetUserdata( ((PacketNetUnloadUser*)pPacket)->dwObjectID );
			pcUser = USERDATATOUSER( pcUserData );
			if ( pcUserData && pcUser )
			{
				USERSERVER->OnUnLoadUser( pcUser );
			}
			break;

		case PKTHDR_NetGetExp:
			pcUserData = USERSERVER->GetUserdata( ((PacketNetGetExp*)pPacket)->uObjectID );
			if ( pcUserData )
			{
				NETSERVER->SendExp( pcUserData, CHARACTERSERVER->GetExp( pcUserData ) );
				NETSERVER->SendClan( pcUserData );
			}
			break;


		case PKTHDR_NetExp:
			pcUserData = USERSERVER->GetUserdata( ( ( PacketNetExp* )pPacket )->uObjectID );
			if ( pcUserData )
			{
				CHARACTERSERVER->SetExp( pcUserData, ((PacketNetExp*)pPacket)->iExp );
			}
			break;

		case PKTHDR_NetGetGameMaster:
			pcUserData = USERSERVER->GetUserdata( ((PacketNetGetGameMaster*)pPacket)->szName );
			if ( pcUserData )
			{
				pcUserData->iGameLevel = (EGameLevel)((PacketNetGetGameMaster*)pPacket)->iGameMasterLevel;

				if (pcUserData->iGameLevel >= GAMELEVEL_One)
				{
					CHATSERVER->SendChatEx( USERDATATOUSER(pcUserData), EChatColor::CHATCOLOR_White, "> GM level %d activated!", pcUserData->iGameLevel );

					//only for GM only
					//It's so that they will get the latest skill edits in-progress
					CheckAndAutoSyncNewSkillsInfoforGMsOnly( USERDATATOUSER( pcUserData ) );
					CheckAndAutoSyncNewSkillsDataforGMsOnly( USERDATATOUSER( pcUserData ) );
				}
			}
		break;

		case PKTHDR_NetQuestT5Data:
			QUESTSERVER->HandlePacket( (PacketNetT5QuestData*)pPacket );
			break;

		case PKTHDR_NetRemoveFinishedQuestData:
			QUESTSERVER->RemoveQuestFinishData( USERDATATOUSER( USERSERVER->GetUserdata( ( (PacketCommand *)pPacket )->p1 ) ), ((PacketCommand *)pPacket )->p2 );
			break;

		case PKTHDR_NetQuestUpdateData:
			QUESTSERVER->HandlePacket( USERDATATOUSER( USERDATABYID( ((PacketNetQuestUpdateData*)pPacket)->iID ) ), (PacketNetQuestUpdateData*)pPacket );
			break;

		case PKTHDR_NetQuestUpdateDataPart:
			QUESTSERVER->HandlePacket( USERDATATOUSER( USERDATABYID( ((PacketNetQuestUpdateDataPart*)pPacket)->iUserID ) ), (PacketNetQuestUpdateDataPart*)pPacket );
			break;

		case PKTHDR_NetQuestCancelOrFinish:
			QUESTSERVER->HandleQuestCancelOrFinish( USERDATATOUSER( USERDATABYID( ((PacketCommand*)pPacket)->p1 ) ), ((PacketCommand*)pPacket)->p2, ((PacketCommand*)pPacket)->p3 );
			break;

		case PKTHDR_NetPlayerWorldToken:
		{
			std::string l_Token = ((PacketNetPlayerWorldToken*)pPacket)->Token;
			std::string l_TokenPass = ((PacketNetPlayerWorldToken*)pPacket)->TokenPass;

			m_PlayerWorldLoginTokenMutex.lock();
			m_PlayerWorldLoginToken[l_Token] = l_TokenPass;
			m_PlayerWorldLoginTokenMutex.unlock();
			break;
		}
		case PKTHDR_NetPlayerInventory:
		{
			PacketNetPlayerInventory* l_InvPacket = reinterpret_cast<PacketNetPlayerInventory*>(pPacket);
			UserData* l_UserData = USERDATABYID(l_InvPacket->ObjectID);
			User* pcUser = USERDATATOUSER(l_UserData);
			if (l_UserData && pcUser)
			{
				memcpy(pcUser->sLastInventoryItems, l_UserData->sIntentoryItems, sizeof(DropItemData) * 316); //store previous inventory state for PKTHDR_ThrowItem
				memcpy(l_UserData->sIntentoryItems, l_InvPacket->sIntentoryItems, sizeof(DropItemData) * 316);
				l_UserData->pcSocketData->u->LastDroppedItemsThrowMapHack.clear();

				// Track equipped items for LootFilter ilvl comparison
				// sIntentoryItems is a flat array (NOT slot-ordered) — scan by item type
				EItemID eWeaponScan = (EItemID)0;
				EItemID eShieldScan = (EItemID)0;
				pcUser->eArmorEquipped = (EItemID)0;
				pcUser->eBootsEquipped = (EItemID)0;
				pcUser->eGauntletsEquipped = (EItemID)0;
				pcUser->eBraceletEquipped = (EItemID)0;
				pcUser->eRingRightEquipped = (EItemID)0;
				pcUser->eRingLeftEquipped = (EItemID)0;
				pcUser->eOrbEquipped = (EItemID)0;
				pcUser->eRobeEquipped = (EItemID)0;
				pcUser->eAmuletEquipped = (EItemID)0;

				for (int i = 0; i < 316; i++)
				{
					DropItemData& item = l_InvPacket->sIntentoryItems[i];
					if (!item.iItemID)
						continue;

					auto pDef = ITEMSERVER->FindItemDefByCode(item.iItemID);
					if (!pDef)
						continue;

					DWORD eItemType = pDef->sItem.sItemID.ToItemType();
					DWORD eItemBase = eItemType & 0xFF000000;

					if (eItemBase == ITEMBASE_Weapon && !eWeaponScan)
						eWeaponScan = (EItemID)item.iItemID;
					else if (eItemType == ITEMTYPE_Shield && !eShieldScan)
						eShieldScan = (EItemID)item.iItemID;

					switch (eItemType)
					{
					case ITEMTYPE_Armor:		pcUser->eArmorEquipped = (EItemID)item.iItemID; break;
					case ITEMTYPE_Boots:		pcUser->eBootsEquipped = (EItemID)item.iItemID; break;
					case ITEMTYPE_Gauntlets:	pcUser->eGauntletsEquipped = (EItemID)item.iItemID; break;
					case ITEMTYPE_Bracelets:	pcUser->eBraceletEquipped = (EItemID)item.iItemID; break;
					case ITEMTYPE_Orb:			pcUser->eOrbEquipped = (EItemID)item.iItemID; break;
					case ITEMTYPE_Robe:			pcUser->eRobeEquipped = (EItemID)item.iItemID; break;
					case ITEMTYPE_Amulet:		pcUser->eAmuletEquipped = (EItemID)item.iItemID; break;
					case ITEMTYPE_Ring:
					case ITEMTYPE_Ring2:
						// Equipped rings come first in the array; Right then Left
						if (!pcUser->eRingRightEquipped)
							pcUser->eRingRightEquipped = (EItemID)item.iItemID;
						else if (!pcUser->eRingLeftEquipped)
							pcUser->eRingLeftEquipped = (EItemID)item.iItemID;
						break;
					default: break;
					}
				}

				// // Log equipped items for confirmation
				// auto LogEquip = [](const char* slot, EItemID id) {
				// 	if (id)
				// 	{
				// 		auto pDef = ITEMSERVER->FindItemDefByCode(id);
				// 		if (pDef)
				// 			INFO("  [%s] %s (ilvl %d)", slot, pDef->sItem.szItemName, pDef->sItem.iLevel);
				// 	}
				// };
				// INFO("Equipped items for %s:", l_UserData->szCharacterName);
				// LogEquip("Weapon", eWeaponScan);
				// LogEquip("Shield", eShieldScan);
				// LogEquip("Armor",  pcUser->eArmorEquipped);
				// LogEquip("Boots",  pcUser->eBootsEquipped);
				// LogEquip("Gaunt",  pcUser->eGauntletsEquipped);
				// LogEquip("Brace",  pcUser->eBraceletEquipped);
				// LogEquip("Ring R", pcUser->eRingRightEquipped);
				// LogEquip("Ring L", pcUser->eRingLeftEquipped);
				// LogEquip("Orb",    pcUser->eOrbEquipped);
				// LogEquip("Robe",   pcUser->eRobeEquipped);
				// LogEquip("Amulet", pcUser->eAmuletEquipped);
			}
			else
			{
				if (LOGIN_SERVER)
					WARN("PKTHDR_NetPlayerInventory USER NOT FOUND");
			}

			break;
		}
		case PKTHDR_NetPlayerGold:
		{
			PacketNetPlayerGold * l_GoldPacket = reinterpret_cast<PacketNetPlayerGold*>(pPacket);
			UserData            * l_UserData   = USERDATABYID(l_GoldPacket->ObjectID);

			if (l_UserData)
			{
				 l_UserData->iGoldIn				= l_GoldPacket->iGoldIn;
				 l_UserData->iGoldOut				= l_GoldPacket->iGoldOut;
				 l_UserData->iInventoryGold			= l_GoldPacket->iInventoryGold;
				 l_UserData->iSaveGold				= l_GoldPacket->iSaveGold;
				 l_UserData->sCharacterData.iGold	= l_GoldPacket->sCharacterData_iGold;
			}
			else
			{
				if ( LOGIN_SERVER )
					WARN ( "PKTHDR_NetPlayerGold USER NOT FOUND" );
			}

			break;
		}

		//received from login server via
		//BellatraServer::RecordPoint
		case PKTHDR_NetBellatraSoloRanking:
		{
			auto pCommand = reinterpret_cast<PacketCommand*>(pPacket);
			pcUserData = USERSERVER->GetUserdata(pCommand->p1); //dwObjectID
			pcUser = USERDATATOUSER(pcUserData);
			if (pcUserData && pcUser)
			{
				BELLATRASERVER->UpdateAndSyncUserSoloRanking(pcUser, pCommand->p2, FALSE); //important to set FALSE here to prevent a loop!

				pcUser->sBellatraSoloCrown = pCommand->p2; //solo crown ranking
				INFO ( "pcUser->iBellatraSoloCrownRanking = %d", pcUser->sBellatraSoloCrown );
			}
			else
			{
				WARN ( "pcUser->iBellatraSoloCrownRanking = pcuserData or pcUser is null!" );
			}
		}
		break;

		default:
			break;
	}
}

NetConnection * NetServer::GetNetConnectionBySocketData( class SocketData * sd )
{
	for( std::vector<NetConnection*>::iterator it = vNetConnections.begin(); it != vNetConnections.end(); it++ )
	{
		NetConnection * nc = *it;

		if( nc->pcSocketData == sd )
			return nc;
	}

	return NULL;
}

void NetServer::OnReceivePacket( User * pcUser, void * pPacket )
{
	UserData * pcUserData = pcUser->pcUserData;

	int iPacketID = ((Packet*)pPacket)->iHeader;

	/// Filter all player packets here
	if ((iPacketID & 0xFF000000) != 0x7F000000)
		return;

#if defined (DEV_MODE) && defined (PACKET_LOG)

	const std::string & l_OpcodeName = SOCKETSERVER->GetOpcodeName(iPacketID);

	if (!PACKETSERVER->IsPacketIgnored((Packet*)pPacket))
	{
		const std::string extraInfo =PACKETSERVER->GetExtraInfoForPacket((Packet*)pPacket);

		std::string str = "";

		if ( pcUser->bNetServer )
		{
			str = GAME_SERVER ? "SERVER <-- LOGIN" : "SERVER <-- GAME";
		}
		else
		{
			std::string charName = CHARACTERSERVER->GetCharacterName( pcUserData );

			if ( charName.empty() )
				charName = pcUserData->pcSocketData->szIP;

			str = FormatString( "SERVER <-- '%s'", charName.c_str() );
		}

		if ( !l_OpcodeName.empty() )
		{
			DEBUGPACKET( "[ %s ] %s", str.c_str(), l_OpcodeName.c_str() );
		}
		else
		{
			char hexString[16];
			_itoa_s( static_cast<DWORD>( iPacketID ), hexString, 16 );
			WARN( "[ %s ] Unknown Opcode, Please add packet 0x%s to ADD_OPCODE_NAME(..)", str.c_str(), hexString);
		}
	}


#endif

	if (!pcUserData->pcSocketData->GetSocketDataContext()->InterServerAuthed && iPacketID != PKTHDR_NetIdentifier)
	{
		LOGERROR("INTER SERVER SPOOFING KICKING");
		SENDPACKETBLANK(USERDATATOUSER(pcUserData), PKTHDR_Disconnect);
		WNDPROC_SOCKETCLOSE(SOCKETSERVER->GetHWND(), pcUserData->pcSocketData);

		return;
	}

	UserData * pcUserD = pcUserData;

	if ( LOGIN_SERVER )
		OnReceiveFromGameServer( pcUserD, pPacket );
	else
		OnReceiveClient( pcUserD, pPacket );
}

void NetServer::AddWorldConnectAllowance(const std::string & p_TokenHash, const std::string & p_TokenPassHash)
{
	PacketNetPlayerWorldToken l_Packet;
	l_Packet.iLength				= sizeof(PacketNetPlayerWorldToken);
	l_Packet.iHeader				= PKTHDR_NetPlayerWorldToken;
	STRINGCOPY(l_Packet.Token,		p_TokenHash.c_str());
	STRINGCOPY(l_Packet.TokenPass,	p_TokenPassHash.c_str());

	SendPacketGameServers( &l_Packet);
}

bool NetServer::UsePlayerWorldLoginToken(const std::string & p_TokenHash, const std::string & p_TokenPassHash)
{
	std::lock_guard<std::mutex> l_Lock(m_PlayerWorldLoginTokenMutex);

	auto l_It = m_PlayerWorldLoginToken.find(p_TokenHash);

	if (l_It == m_PlayerWorldLoginToken.end())
		return false;

	if (l_It->second == p_TokenPassHash)
	{
		m_PlayerWorldLoginToken.erase(l_It);
		return true;
	}

	return false;
}

void NetServer::Init()
{
	if ( !bInit )
	{
		ConnectAll();
		bInit = TRUE;

		bReceivedBack = TRUE;
	}
}

void NetServer::Tick()
{
	if ( GAME_SERVER )
	{
		SYSTEMTIME * ps = GetServerTime();

		if ( wTimeMinute != ps->wMinute )
		{
			PacketNetUsersOnline s;
			s.iLength = sizeof( PacketNetUsersOnline );
			s.iHeader = PKTHDR_NetUsersOnline;
			s.iID = SERVER_CODE - 1;
			s.iUsersOnline = (*(DWORD*)0x07AC9BC8);

			NetConnection * nc = GetLoginServerConnection();
			if ( nc )
				SendPacket( nc->pcSocketData, &s );

			wTimeMinute = ps->wMinute;
		}
	}
}
