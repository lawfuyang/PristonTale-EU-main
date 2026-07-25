#include "stdafx.h"
#include "accountserver.h"
#include "picosha2.h"


//#define _LOGIN_TEST

PacketCharacterRecordData sPacketCharacterDataLogin;
PacketUserInfo sPacketUserInfoLogin;

bool AccountServer::bActive;
int AccountServer::iWaitingAccountLogins;
CMutex * AccountServer::pcMutex;
AccountLogin AccountServer::saAccountLogin[MAX_ACCOUNTLOGINQUEUE];

AccountServer::AccountServer()
{
	pcMutex = new CMutex( "Account Server" );

	ZeroMemory( &saAccountLogin, sizeof( saAccountLogin ) );
	iWaitingAccountLogins = 0;
	bActive = true;
}

AccountServer::~AccountServer()
{
	bActive = false;
	iWaitingAccountLogins = 0;
	ZeroMemory( &saAccountLogin, sizeof( saAccountLogin ) );

	SAFE_DELETE( pcMutex );
}

void AccountServer::LogAccountLogin( UserData * pcUserData, AccountLogin & al, ELogAccount eLogAccount )
{

	char szLogDescription[ 512 ] = { 0 };
	char * pszIP = NULL;
	char szDateTime[ 20 ] = { 0 };
	GetDateTime( szDateTime );

	char * pszSystemOS = "Unknown";

	switch ( (ESystemOS)al.iSystemOS )
	{
		case SYSTEMOS_Windows2000:
			pszSystemOS = "Windows 2000";
			break;
		case SYSTEMOS_WindowsXP:
			pszSystemOS = "Windows XP";
			break;
		case SYSTEMOS_Windows2003:
			pszSystemOS = "Windows Server 2003";
			break;
		case SYSTEMOS_Windows2003R2:
			pszSystemOS = "Windows Server 2003 R2";
			break;
		case SYSTEMOS_WindowsVista:
			pszSystemOS = "Windows Vista";
			break;
		case SYSTEMOS_Windows2008:
			pszSystemOS = "Windows Server 2008";
			break;
		case SYSTEMOS_Windows2008R2:
			pszSystemOS = "Windows Server 2008 R2";
			break;
		case SYSTEMOS_Windows7:
			pszSystemOS = "Windows 7";
			break;
		case SYSTEMOS_Windows2012:
			pszSystemOS = "Windows Server 2012";
			break;
		case SYSTEMOS_Windows8:
			pszSystemOS = "Windows 8";
			break;
		case SYSTEMOS_Windows2012R2:
			pszSystemOS = "Windows Server 2012 R2";
			break;
		case SYSTEMOS_Windows8_1:
			pszSystemOS = "Windows 8.1";
			break;
		case SYSTEMOS_Windows2016:
			pszSystemOS = "Windows Server 2016";
			break;
		case SYSTEMOS_Windows10:
			pszSystemOS = "Windows 10";
			break;
	}

	if ( pcUserData )
	{
		if ( pcUserData->pcSocketData )
			pszIP = pcUserData->pcSocketData->szIP;
	}

	switch ( eLogAccount )
	{
		case ACCLOGID_LoginSuccess:
			STRINGFORMAT( szLogDescription, "[%s] (Login Success) -> Password[%s] MacAddr[%s] PCName[%s] SerialHD[%d] HardwareID[%s] VideoName[%s] Width[%d] Height[%d] SystemOS[%s]",
				al.szAccountName, al.szPassword, al.szMacAddress, al.szPCName, al.uSerialHD, al.szHardwareID, al.szVideoName, al.uWidthScreen, al.uHeightScreen, pszSystemOS );
			break;

		case ACCLOGID_IncorrectAccount:
			STRINGFORMAT( szLogDescription, INCORRECT_ACCOUNT_LOGACCTEXT,
				al.szAccountName, al.szPassword, al.szMacAddress, al.szPCName, al.uSerialHD, al.szVideoName );
			break;

		case ACCLOGID_IncorrectPassword:
			STRINGFORMAT( szLogDescription, INCORRECT_PASSWORD_LOGACCTEXT, al.szAccountName, al.szPassword );
			break;

		case ACCLOGID_BlockedAccount:
			STRINGFORMAT( szLogDescription, TRYLOGIN_ACCBLOCKED_LOGACCTEXT, al.szAccountName, al.szPassword );
			break;

		case ACCLOGID_UnblockedAccount:
			STRINGFORMAT( szLogDescription, UNBANNED_ACCBLOCKED_LOGACCTEXT, al.szAccountName, al.szPassword );
			break;

		case ACCLOGID_NotActivatedAccount:
			STRINGFORMAT( szLogDescription, TRYLOGIN_ACCNOTACTIVATED_LOGACCTEXT, al.szAccountName, al.szPassword );
			break;

		case ACCLOGID_CharacterSelectSend:
			STRINGFORMAT( szLogDescription, CHARACTERSELECTSEND_LOGACCTEXT, al.szAccountName );
			break;

		case ACCLOGID_TempBanAccount:
			STRINGFORMAT( szLogDescription, "[%s] (Temporarily Banned) -> Password[%s] MacAddr[%s] PCName[%s] SerialHD[%d] VideoName[%s]",
				al.szAccountName, al.szPassword, al.szMacAddress, al.szPCName, al.uSerialHD, al.szVideoName );
			break;

		default:
			break;
	}

	SQLConnection * pcDB = SQLCONNECTION( DATABASEID_LogDB, 1 );

	if ( pcDB->Open() )
	{
		if ( pcDB->Prepare( "INSERT INTO AccountLog VALUES (?,?,?,?,?,?)" ) )
		{
			pcDB->BindParameterInput( 1, PARAMTYPE_String, pszIP );
			pcDB->BindParameterInput( 2, PARAMTYPE_String, (void*)al.szAccountName );
			pcDB->BindParameterInput( 3, PARAMTYPE_Integer, &eLogAccount );
			pcDB->BindParameterInput( 4, PARAMTYPE_String, szLogDescription, 512 );
			pcDB->BindParameterInput( 5, PARAMTYPE_String, szDateTime );
			pcDB->BindParameterInput( 6, PARAMTYPE_Integer, &GAME_SERVER );
			pcDB->Execute();
		}
		pcDB->Close();
	}
}

BOOL AccountServer::IsOnlineAccount( UserData * pcUserData, const char * pszUserID )
{
	for ( int i = 0; i < PLAYERS_MAX; i++ )
	{
		if ( &USERSDATA[i] != pcUserData && USERSDATA[i].pcSocketData )
		{
			if ( lstrcmpiA( USERSDATA[i].szAccountName, pszUserID ) == 0 )
			{
				if ( USERSDATA[i].dwHashAccountName == pcUserData->dwHashAccountName )
				{
					if ( USERSDATA[i].pcSocketData->bConnected == FALSE )
					{
						LOGSERVER->LogSocketClose( USERSDATA[i].pcSocketData, "AccountServer::IsOnlineAccount", _ReturnAddress() );
						WNDPROC_SOCKETCLOSE( SOCKETSERVER->GetHWND(), USERSDATA[i].pcSocketData );

#ifdef RECONNECT_SYSTEM
						USERSERVER->DeleteReconnectUser( pszUserID );
#endif
						return FALSE;
					}

					return TRUE;
				}
			}
		}
	}

#ifdef RECONNECT_SYSTEM
	USERSERVER->DeleteReconnectUser( pszUserID );
#endif

	return FALSE;
}

/// <summary>
/// Unmute player's account based on character name, and optionally reset strikes
/// </summary>
/// <param name="pszCharacterName">Player name</param>
/// <param name="bReset">If true, clear all strikes</param>
BOOL AccountServer::UnmuteAccountByCharacterName(User * pcGMUser, const char * pszCharacterName, BOOL bReset)
{
	std::string szAccountName = CHARACTERSERVER->SQLGetAccountName(pszCharacterName);

	//account not found. character doesn't exist
	if (szAccountName.empty())
	{
		CHATSERVER->SendChatEx(pcGMUser, CHATCOLOR_Error, "> /!unmute failed! Character '%s' does not exist!", pszCharacterName);
		return FALSE;
	}

	//get info about account, including mute expiry, mute status and mute count
	SQLUser psSqlUser;
	ZeroMemory(&psSqlUser, sizeof(SQLUser));
	if (!ACCOUNTSERVER->GetSQLUserInfoData(szAccountName.c_str(), &psSqlUser))
	{
		CHATSERVER->SendChatEx(pcGMUser, CHATCOLOR_Error, "> /!unmute failed! Account '%s' does not exist!", szAccountName.c_str()); //shouldn't happen.. but let's check anyway
		return FALSE;
	}

	User* pcMuteUser = USERDATATOUSER(USERSERVER->GetUserdata(pszCharacterName));

	if (psSqlUser.bIsMuted == FALSE && bReset == FALSE)
	{
		CHATSERVER->SendChatEx(pcGMUser, CHATCOLOR_Error, "> Character '%s' is already unmuted!", pszCharacterName);

		if (pcMuteUser)
		{
			pcMuteUser->bMuted = FALSE;
			pcMuteUser->dwUnMuteExpiryTime = 0;

			USERSERVER->SyncUserMuteStatus( pcMuteUser );
		}

		return TRUE;
	}


	//Character is online
	if (pcMuteUser)
	{
		pcMuteUser->bMuted = FALSE;
		pcMuteUser->dwUnMuteExpiryTime = 0;

		USERSERVER->SyncUserMuteStatus( pcMuteUser );

		CHATSERVER->SendChat(pcMuteUser, CHATCOLOR_Error, "> You have been unmuted by a GM. You can now speak freely!");
	}

	auto pcDB = SQLCONNECTION(DATABASEID_UserDB_PrimaryServer);

	if (pcDB->Open())
	{
		if (bReset)
		{
			// Update UserInfo with mute status and mute counter
			if (pcDB->Prepare( "UPDATE UserInfo SET IsMuted=0, MuteCount=(MuteCount-1), UnmuteDate=NULL, MuteCount=0 WHERE ID=?" ))
			{
				pcDB->BindParameterInput( 1, PARAMTYPE_Integer, &psSqlUser.iID );
				pcDB->ExecuteUpdate();
			}
		}
		else
		{
			// Update UserInfo with mute status and mute counter
			if (pcDB->Prepare( "UPDATE UserInfo SET IsMuted=0, MuteCount=(MuteCount-1), UnmuteDate=NULL WHERE ID=?" ))
			{
				pcDB->BindParameterInput( 1, PARAMTYPE_Integer, &psSqlUser.iID );
				pcDB->ExecuteUpdate();
			}
		}

		pcDB->Close();
	}

	if ( bReset )
	{
		// On success send message to chat
		CHATSERVER->SendChatAllGM( "GM> Player '%s' is unmuted and ALL strikes cleared", pszCharacterName );
	}
	else
	{
		// On success send message to chat
		CHATSERVER->SendChatAllGM( "GM> Player '%s' is unmuted and one strike removed", pszCharacterName );

		if (pcGMUser->GetGameLevel() == EGameLevel::GAMELEVEL_One)
		{
			CHATSERVER->SendChatEx( pcGMUser, EChatColor::CHATCOLOR_White, "Player '%s' is unmuted and one strike removed", pszCharacterName);
		}
	}

	return TRUE;
}

BOOL  AccountServer::GetSQLUserInfoData ( const char * pszAccountName, SQLUser * psSqlUserOut )
{
	ZeroMemory ( psSqlUserOut, sizeof ( SQLUser ) );

	SQLConnection * cDB = SQLCONNECTION ( DATABASEID_UserDB_PrimaryServer );

	BOOL bResult = FALSE;

	if ( cDB->Open () )
	{
		if ( cDB->Prepare ( "SELECT TOP(1) [ID],[AccountName],[Password],[Flag],[Active],[Coins],[GameMasterType],[GameMasterLevel],[GameMasterMacAddress],[BanStatus],[UnbanDate],[IsMuted],[MuteCount],[UnmuteDate] FROM UserInfo WHERE AccountName=?" ) )
		{
			cDB->BindParameterInput ( 1, PARAMTYPE_String, (char*)pszAccountName, 32 );

			if ( cDB->Execute () && cDB->Fetch ())
			{
				int iGameMasterLevel = 0;
				int iBanStatus = 0;

				cDB->GetData ( 1, PARAMTYPE_Integer, &psSqlUserOut->iID );
				cDB->GetData ( 2, PARAMTYPE_String, psSqlUserOut->szAccountName, 32 );
				cDB->GetData ( 3, PARAMTYPE_String, psSqlUserOut->szPassword, 65 ); //need to be 65. See remark about BufferLength in https://docs.microsoft.com/en-us/sql/odbc/reference/syntax/sqlgetdata-function?view=sql-server-ver15
				cDB->GetData ( 4, PARAMTYPE_Integer, &psSqlUserOut->iFlag);
				cDB->GetData ( 5, PARAMTYPE_Integer, &psSqlUserOut->iActive );
				cDB->GetData ( 6, PARAMTYPE_Integer, &psSqlUserOut->iCoins );
				cDB->GetData ( 7, PARAMTYPE_Integer, &psSqlUserOut->bGameMasterType );
				cDB->GetData ( 8, PARAMTYPE_Integer, &iGameMasterLevel );
				cDB->GetData ( 9, PARAMTYPE_String, psSqlUserOut->szGameMasterMacAddress, 20 );
				cDB->GetData ( 10, PARAMTYPE_Integer, &iBanStatus);
				cDB->GetData ( 11, PARAMTYPE_Time, &psSqlUserOut->sUnbanDate );
				cDB->GetData ( 12, PARAMTYPE_Integer, &psSqlUserOut->bIsMuted );   //player is muted?
				cDB->GetData ( 13, PARAMTYPE_Integer, &psSqlUserOut->iMuteCount ); //number of times player was muted
				cDB->GetData ( 14, PARAMTYPE_Time, &psSqlUserOut->sUnmuteDate );   //mute expiry date

				psSqlUserOut->iGameMasterLevel = (EGameLevel) iGameMasterLevel;
				psSqlUserOut->iBanStatus = (EBanStatus)iBanStatus;

				bResult = TRUE;
			}
			else
			{
				WARN("SELECT TOP(1) FROM UserInfo query failed for account '%s'", pszAccountName);
			}
		}

		cDB->Close ();
	}

	return bResult;
}

BOOL AccountServer::IsAdminMacAddress( const char * pszAccountName, const char * pszMacAddress, UINT uSerialHD )
{
	/**
	* GM Level 4 direct on the server
	* Unico that can log all accounts with any password and has GM enabled automatically
	 */

	//IronFishNZ's main PC
	//if( STRINGCOMPAREI( pszMacAddress, "B4-2E-99-CD-C0-BF" ) || uSerialHD == 510130974)
		//return TRUE;

	return FALSE;
}

BOOL AccountServer::IsIPAccept( const char * pszIP )
{
	BOOL bRet = FALSE;

	SQLConnection * pcDB = SQLCONNECTION( DATABASEID_ServerDB, 1 );
	if ( pcDB->Open() )
	{
		if ( pcDB->Prepare( "SELECT Corno FROM Maintenance WHERE (Mode=1) AND (IP=?)" ) )
		{
			pcDB->BindParameterInput( 1, PARAMTYPE_String, (void*)pszIP );

			if ( pcDB->Execute() && pcDB->Fetch() )
			{
				bRet = TRUE;
			}
		}
		pcDB->Close();
	}

	return bRet;
}

BOOL AccountServer::IsAccountAccept( const char * pszAccountName )
{
	BOOL bRet = FALSE;

	SQLConnection * pcDB = SQLCONNECTION( DATABASEID_ServerDB, 2 );
	if ( pcDB->Open() )
	{
		if ( pcDB->Prepare( "SELECT Corno FROM Maintenance WHERE (Mode=7) AND (IP=?)" ) )
		{
			pcDB->BindParameterInput( 1, PARAMTYPE_String, (void*)pszAccountName );

			if ( pcDB->Execute() && pcDB->Fetch() )
			{
				bRet = TRUE;
			}
		}
		pcDB->Close();
	}

	return bRet;
}

BOOL AccountServer::IsMacBanned( UserData * pcUserData, const char * pszMacAddress )
{
	BOOL bRet = FALSE;

	SQLConnection * pcDB = SQLCONNECTION( DATABASEID_ServerDB, 3 );

	if ( pcDB->Open() )
	{
		if ( pcDB->Prepare( "SELECT Corno FROM Maintenance WHERE (Mode=4) AND (IP=?)" ) )
		{
			pcDB->BindParameterInput( 1, PARAMTYPE_String, (void*)pszMacAddress );

			if ( pcDB->Execute() && pcDB->Fetch() )
			{
				bRet = TRUE;
			}
		}
		pcDB->Close();
	}

	if ( bRet == FALSE )
	{
		/////Mac Address of VM Ware
		///if ( std::string( pszMacAddress ).find( "00-50-56-", 0, 9 ) != std::string::npos )
		///	bRet = TRUE;
        ///
		/////Mac Address of VM Ware 2
		///if ( std::string( pszMacAddress ).find( "00-0C-29-", 0, 9 ) != std::string::npos )
		///	bRet = TRUE;
        ///
		/////Mac Address of VM Ware 3
		///if ( std::string( pszMacAddress ).find( "00-05-69-", 0, 9 ) != std::string::npos )
		///	bRet = TRUE;
        ///
		/////Mac Address of Virtual Box
		///if ( std::string( pszMacAddress ).find( "08-00-27-", 0, 9 ) != std::string::npos )
		///	bRet = TRUE;
        ///
		/////Mac Address of Virtual PC
		///if ( std::string( pszMacAddress ).find( "00-03-FF-", 0, 9 ) != std::string::npos )
		///	bRet = TRUE;
	}

	return bRet;
}

BOOL AccountServer::IsSerialHDBanned( UserData * pcUserData, const char * pszSerialHD )
{
	BOOL bRet = FALSE;

	SQLConnection * pcDB = SQLCONNECTION( DATABASEID_ServerDB, 4 );

	if ( pcDB->Open() )
	{
		if ( pcDB->Prepare( "SELECT Corno FROM Maintenance WHERE (Mode=5) AND (IP=?)" ) )
		{
			pcDB->BindParameterInput( 1, PARAMTYPE_String, (void*)pszSerialHD );

			if ( pcDB->Execute() && pcDB->Fetch() )
			{
				bRet = TRUE;
			}
		}
		pcDB->Close();
	}
	return bRet;
}

BOOL AccountServer::IsComputerNameBanned( UserData * pcUserData, const char * pszPCName )
{
	BOOL bRet = FALSE;

	SQLConnection * pcDB = SQLCONNECTION( DATABASEID_ServerDB, 5 );

	if ( pcDB->Open() )
	{
		if ( pcDB->Prepare( "SELECT Corno FROM Maintenance WHERE (Mode=6) AND (IP=?)" ) )
		{
			pcDB->BindParameterInput( 1, PARAMTYPE_String, (void*)pszPCName );

			if ( pcDB->Execute() && pcDB->Fetch() )
			{
				bRet = TRUE;
			}
		}
		pcDB->Close();
	}

	if ( bRet == FALSE )
	{
		///if ( std::string( (char*)0x008B867F0 ).find( "VMWare" ) != std::string::npos )
		///	bRet = TRUE;
		///if ( std::string( (char*)0x008B867F0 ).find( "Virtual" ) != std::string::npos )
		///	bRet = TRUE;
	}

	return bRet;
}

BOOL AccountServer::IsHardwareIDBanned( UserData * pcUserData, const char * pszHardwareID )
{
	BOOL bRet = FALSE;

	SQLConnection * pcDB = SQLCONNECTION( DATABASEID_ServerDB, 6 );

	if ( pcDB->Open() )
	{
		if ( pcDB->Prepare( "SELECT Corno FROM Maintenance WHERE (Mode=8) AND (IP=?)" ) )
		{
			pcDB->BindParameterInput( 1, PARAMTYPE_String, (void *)pszHardwareID );

			if ( pcDB->Execute() && pcDB->Fetch() )
			{
				bRet = TRUE;
			}
		}
		pcDB->Close();
	}
	return bRet;
}

void AccountServer::SQLCharacterLog( char * pszAccountName, int iCharacterClass, char * pszCharacterName, char * pszCode, char * pszIP, char * pszServerName, int iCharacterLevel, __int64 iCharacterExperience, int iCharacterGold, int iAction )
{
	switch ( iAction )
	{
		case 3:
		{
			CHARACTERSERVER->SQLUpdateCharacter ( pszCharacterName, iCharacterClass, iCharacterLevel, iCharacterExperience, iCharacterGold );
		}
		break;

		default:
			break;
	}
}

void AccountServer::SQLUserOnline( char * pszAccountName, char * pszCharacterName, char * pszIP, int iTicket, int iAction, int iCharacterClass, int iCharacterLevel )
{
	SQLConnection * pcDB = SQLCONNECTION( DATABASEID_ServerDB, 7 );

	if( pcDB->Open() != FALSE )
	{
		switch( iAction )
		{
		case 0:
			if( pcDB->Prepare( "INSERT INTO UsersOnline ([AccountName],[CharacterName],[IP],[CharacterClass],[CharacterLevel],[Ticket],[LoginTime]) VALUES (?,?,?,?,?,?,GETDATE())" ) )
			{
				pcDB->BindParameterInput( 1, PARAMTYPE_String, pszAccountName );
				pcDB->BindParameterInput( 2, PARAMTYPE_String, pszCharacterName );
				pcDB->BindParameterInput( 3, PARAMTYPE_String, pszIP );
				pcDB->BindParameterInput( 4, PARAMTYPE_Integer, &iCharacterClass );
				pcDB->BindParameterInput( 5, PARAMTYPE_Integer, &iCharacterLevel );
				pcDB->BindParameterInput( 6, PARAMTYPE_Integer, &iTicket );

				pcDB->Execute();

			}
			break;
		case 1:

			if( pcDB->Prepare( "DELETE FROM UsersOnline WHERE AccountName=?" ) )
			{
				pcDB->BindParameterInput( 1, PARAMTYPE_String, pszAccountName );

				pcDB->Execute();
			}
			break;

		case 2:

			if( pcDB->Prepare( "DELETE FROM UsersOnline" ) )
			{
				pcDB->Execute();
			}
			break;

			default:
				break;
		}

		pcDB->Close();
	}
}

/// <summary>
/// Check if the warehouse quest is completed for account. If so, unlock for all chars
/// </summary>
/// <param name="pcUser"></param>
/// <param name="iPageNum">1-based page number</param>
BOOL AccountServer::CanAccessWarehousePage ( User * pcUser, int iPageNum )
{
	SQLConnection * pcDB = SQLCONNECTION( DATABASEID_UserDB_LocalServer_CharInfo );

	//First page has level 10 requirement (account-wide)
	if ( iPageNum == 1 )
	{
		//this user is >= level 10. all good.
		if ( pcUser->pcUserData->GetLevel () >= 10 )
			return TRUE;

		//does this user have any chars that is level >= 10?
		if ( pcDB->Open () )
		{
			int iCount = 0;

			if ( pcDB->Prepare ( "SELECT COUNT(*) FROM CharacterInfo WHERE Level >= 10 AND AccountName=?" ) )
			{
				pcDB->BindParameterInput( 1, PARAMTYPE_String, pcUser->pcUserData->szAccountName, 20 );

				if ( pcDB->Execute () && pcDB->Fetch () )
				{
					pcDB->GetData ( 1, PARAMTYPE_Integer, &iCount );
				}
			}

			pcDB->Close ();

			if ( iCount >= 1 )
			{
				return TRUE;
			}
		}
	}
	else if ( iPageNum == 2 )
	{
		return QUESTSERVER->AccountHasFinishedQuest ( pcUser->pcUserData->szAccountName, EQuestID::QUESTID_Warehouse2Page );
	}

	return FALSE;
}

void AccountServer::SQLCharacterSave( UserData * pcUserData )
{
	char szLogDescription[512] = { 0 };
	char * pszIP = NULL;
	char szDateTime[20] = { 0 };
	GetDateTime( szDateTime );

	if ( pcUserData )
	{
		if ( pcUserData->pcSocketData )
			pszIP = pcUserData->pcSocketData->szIP;
	}


	int iLogID = ACCLOGID_CharacterSave;

	SQLConnection * pcDB = SQLCONNECTION( DATABASEID_LogDB, 2 );

	std::string str = FormatNumber( (INT64)pcUserData->GetGold());

	STRINGFORMAT( szLogDescription, "Character[%s] Save EXP[%s] Gold[%s]", CHARACTERSERVER->GetCharacterName( pcUserData ),
		FormatNumber( pcUserData->liEXP.QuadPart ), str.c_str() );

	if ( pcDB->Open() )
	{
		if ( pcDB->Prepare( "INSERT INTO CharacterLog VALUES (?,?,?,?,?,?)" ) )
		{
			pcDB->BindParameterInput( 1, PARAMTYPE_String, pszIP );
			pcDB->BindParameterInput( 2, PARAMTYPE_String, pcUserData->szAccountName );
			pcDB->BindParameterInput( 3, PARAMTYPE_Integer, &iLogID );
			pcDB->BindParameterInput( 4, PARAMTYPE_String, szLogDescription, 512 );
			pcDB->BindParameterInput( 5, PARAMTYPE_String, szDateTime );
			pcDB->BindParameterInput( 6, PARAMTYPE_Integer, &GAME_SERVER );
			pcDB->Execute();
		}
		pcDB->Close();
	}
}


bool AccountServer::SQLAccountHasFlag( char * pszAccountName, EAccountFlag iFlag )
{

	return true;
}


void AccountServer::PHAccountLogin( User * pcUser, PacketLoginUser * psPacket )
{
	if ( !LOGIN_SERVER )
		return;

	AccountLogin al;
	al.pcUser = pcUser;
	al.pcUserData = pcUser->pcUserData;
	al.pcSocketData = pcUser->pcUserData->pcSocketData;
	al.uSerialHD = psPacket->dwSerialHD;
	al.uWidthScreen = psPacket->uWidthScreen;
	al.uHeightScreen = psPacket->uHeightScreen;
	al.iSystemOS = psPacket->iSystemOS;
	STRINGCOPY( al.szAccountName, psPacket->szUserID );
	STRINGCOPY( al.szPassword, psPacket->szPassword );
	STRINGCOPY( al.szIP, pcUser->pcUserData->pcSocketData->szIP );
	STRINGCOPY( al.szMacAddress, psPacket->szMacAddr );
	STRINGCOPY( al.szPCName, psPacket->szPCName );
	STRINGCOPY( al.szVideoName, psPacket->szVideoName );
	STRINGCOPY( al.szHardwareID, psPacket->szHardwareID );

	EAccountLogin iCode = ACCOUNTLOGIN_Success;

	if ( !AddAccountLogin( pcUser, al ) )
	{
		//Connection Failed
		iCode = ACCOUNTLOGIN_LoginPending;
	}

	if ( (iCode != ACCOUNTLOGIN_Success) && (iCode != ACCOUNTLOGIN_WrongVersion) )
	{
		PacketAccountLoginCode s;
		s.iLength = sizeof( s );
		s.iHeader = PKTHDR_AccountLoginCode;
		s.dwReserved = 0;
		s.iCode = iCode;
		s.iFailCode = 1;

		WARN ( "PKTHDR_AccountLoginCode1 %d for %s", (int)iCode, al.szAccountName );

		SENDPACKET( pcUser, &s, TRUE );
	}
}


bool AccountServer::AddAccountLogin( User * pcUser, AccountLogin & al )
{
	bool bRet = false;

	pcMutex->Lock( 4000 );

	if ( iWaitingAccountLogins < MAX_ACCOUNTLOGINQUEUE )
	{
		CopyMemory( &saAccountLogin[iWaitingAccountLogins], &al, sizeof( AccountLogin ) );

		iWaitingAccountLogins++;

		bRet = true;
	}

	pcMutex->Unlock();

	return bRet;
}

/// <summary>
/// Get account mute status and get mute count
/// from the database
/// </summary>
/// <param name="szAccountName">Account name</param>
/// <param name="iMuteCount">Pointer to an int to hold the mute count</param>
/// <returns>TRUE if muted, otherwise FALSE</returns>
BOOL AccountServer::SqlIsAccountMuted ( std::string szAccountName, int * iMuteCount )
{
	// Temp vars for SQL
	BOOL IsMuted = FALSE;
	int MuteCounter = 0;

	// Connect to UserDatabase to get MuteCounter and check if player is muted
	SQLConnection* pcDB = SQLCONNECTION(DATABASEID_UserDB_LocalServer_UserInfo);

	if (pcDB->Open())
	{
		if (pcDB->Prepare("SELECT IsMuted, MuteCount FROM UserInfo WHERE AccountName=?"))
		{
			pcDB->BindParameterInput(1, PARAMTYPE_String, (char*)szAccountName.c_str(), 32);

			if (pcDB->Execute() && pcDB->Fetch())
			{
				pcDB->GetData(1, PARAMTYPE_Integer, &IsMuted);
				pcDB->GetData(2, PARAMTYPE_Integer, &MuteCounter);

				*iMuteCount = MuteCounter;
			}
		}
		pcDB->Close();
	}

	return IsMuted;
}

BOOL AccountServer::SQLAccountIsAdmin( char* pszAccountName )
{
	SQLConnection* cDB;
	int iGameMasterType, iGameMasterLevel;
	BOOL bIsAdmin = FALSE;

	cDB = SQLCONNECTION( DATABASEID_UserDB_PrimaryServer );

	if (cDB->Open())
	{
		if (cDB->Prepare("SELECT GameMasterType, GameMasterLevel FROM UserInfo WHERE AccountName=?"))
		{
			cDB->BindParameterInput(1, PARAMTYPE_String, pszAccountName);
			if (cDB->Execute() && cDB->Fetch())
			{
				cDB->GetData(1, PARAMTYPE_Integer, &iGameMasterType);
				cDB->GetData(2, PARAMTYPE_Integer, &iGameMasterLevel);

				if (iGameMasterType == TRUE &&
					iGameMasterLevel == EGameLevel::GAMELEVEL_Four)
				{
					bIsAdmin = TRUE;
				}
			}

		}
		cDB->Close();
	}

	return bIsAdmin;

}

EAccountFlag AccountServer::SQLAccountGetFlag( char * pszAccountName )
{
	EAccountFlag iRet = ACCOUNTFLAG_NotExist;

	SQLConnection * cDB;

	cDB = SQLCONNECTION( DATABASEID_UserDB_PrimaryServer );

	if ( cDB->Open() != FALSE )
	{
		if ( cDB->Prepare( "SELECT Flag FROM UserInfo WHERE AccountName=?" ) )
		{
			cDB->BindParameterInput( 1, PARAMTYPE_String, pszAccountName, 32 );

			//Note - if this is failing and you're getting 'Incorrect account'
			// on client even though the user name exists in the database
			//It can be because your local database server authentication
			// isn't set up for 'SQL Server and Windows Authentication mode'. Using this mode
			// will allow 'sa' to connect in. To troubleshoot, try connecting
			// do the database via SQL Server Management using sa account.
			if (cDB->Execute() && cDB->Fetch())
			{
				int iFlag;
				cDB->GetData(1, PARAMTYPE_Integer, &iFlag);
				iRet = (EAccountFlag)iFlag;
			}
		}

		cDB->Close();
	}

	return iRet;
}

bool AccountServer::SQLUnbanAccountId ( int iAccountID )
{
	bool bRet = false;

	SQLConnection * cDB;

	cDB = SQLCONNECTION ( DATABASEID_UserDB_PrimaryServer );

	if ( cDB->Open () != FALSE )
	{
		if ( cDB->Prepare ( "UPDATE UserInfo SET BanStatus=0, UnbanDate=NULL WHERE ID=?" ) )
		{
			cDB->BindParameterInput ( 1, PARAMTYPE_Integer, &iAccountID );

			if ( cDB->ExecuteUpdate () )
			{
				bRet = true;
			}
		}

		cDB->Close ();
	}

	return bRet;

}



void AccountServer::SQLSyncBanStatusToCharacterInfo ( SQLConnection * cDB, char * pszAccountName, EBanStatus eBanStatus )
{
	if ( cDB->Open () )
	{
		if ( cDB->Prepare ( "UPDATE CharacterInfo SET Banned=? WHERE AccountName=?" ) )
		{
			cDB->BindParameterInput ( 1, PARAMTYPE_Integer, &eBanStatus );
			cDB->BindParameterInput ( 2, PARAMTYPE_String, pszAccountName, 32 );

			if ( cDB->ExecuteUpdate () == FALSE )
			{
				WARN ( "Account name %s does not exist in CharacterInfo", pszAccountName );
			}
		}

		cDB->Close ();
	}
}

///
bool AccountServer::SQLUnbanAccount( char * pszAccountName )
{
	bool bRet = false;

	SQLConnection * cDB;

	cDB = SQLCONNECTION( DATABASEID_UserDB_PrimaryServer );

	if ( cDB->Open() != FALSE )
	{
		if ( cDB->Prepare( "UPDATE UserInfo SET BanStatus=0,UnbanDate=NULL WHERE AccountName=?" ) )
		{
			cDB->BindParameterInput( 1, PARAMTYPE_String, pszAccountName, 32 );

			if ( cDB->ExecuteUpdate() )
				bRet = true;
		}

		cDB->Close();
	}

	return bRet;
}



/// <summary>
/// Unmute an account by account name
/// </summary>
BOOL AccountServer::SQLUnmuteAccount( char * pszAccountName )
{
	BOOL bRet = false;

	SQLConnection * cDB;

	cDB = SQLCONNECTION( DATABASEID_UserDB_PrimaryServer );
	if ( cDB->Open() )
	{
		if ( cDB->Prepare( "UPDATE UserInfo SET IsMuted=0, UnmuteDate=NULL WHERE AccountName=?" ) )
		{
			cDB->BindParameterInput( 1, PARAMTYPE_String, pszAccountName, 32 );
			bRet = cDB->ExecuteUpdate();
		}

		cDB->Close();
	}

	return bRet;
}

/// <summary>
/// Unmute account by account id
/// </summary>
BOOL AccountServer::SQLUnmuteAccountId ( int iAccountID )
{
	BOOL bRet = FALSE;

	SQLConnection* cDB;

	cDB = SQLCONNECTION(DATABASEID_UserDB_PrimaryServer);

	if (cDB->Open())
	{
		if (cDB->Prepare("UPDATE UserInfo SET IsMuted=0, UnmuteDate=NULL WHERE ID=?"))
		{
			cDB->BindParameterInput(1, PARAMTYPE_Integer, &iAccountID);
			if (cDB->ExecuteUpdate())
			{
				bRet = TRUE;
			}
		}

		cDB->Close();
	}

	return bRet;
}





void AccountServer::ProcessAccountLogin( AccountLogin & al )
{
	SQLUser * psSqlUser = new SQLUser ();
	ZeroMemory(psSqlUser, sizeof(SQLUser));

	User * pcUser = al.pcUser;
	UserData * pcUserData = al.pcUserData;
	SocketData * pcSocketData = al.pcSocketData;
	char * pszAccountName = al.szAccountName;
	char * pszPassword = al.szPassword;
	char * pszIP = al.szIP;

	bool bContinue = false;

	if ( pcUser && pcUserData && pcSocketData && pszAccountName && pszPassword && pszIP )
	{
		if ( pcUser->pcUserData == pcUserData )
		{
			if ( pcUser->pcUserData->pcSocketData == pcSocketData )
			{
				if ( lstrcmpiA( pcSocketData->szIP, pszIP ) == 0 )
				{
					bContinue = true;
				}
			}
		}
	}

	if ( bContinue )
	{
		pcUser->SetGameLevel( GAMELEVEL_None );

		EAccountLogin iCode = ACCOUNTLOGIN_LoginPending;

		if ( pszAccountName[0] == 0 )
		{
			//User did not fill in Account Name
			iCode = ACCOUNTLOGIN_IncorrectName;
		}
		else if ( pszPassword[0] == 0 )
		{
			//User did not fill in Password
			iCode = ACCOUNTLOGIN_IncorrectPassword;
		}

		//Check if this account is already logged in
		if ( USERDATABYACCOUNTNAME ( pszAccountName ) != NULL )
		{
			iCode = ACCOUNTLOGIN_LoggedIn;
		}

		if ( iCode == ACCOUNTLOGIN_LoginPending )
		{
			if ( ACCOUNTSERVER->GetSQLUserInfoData ( pszAccountName, psSqlUser ) )
			{
				int iAccountFlag = psSqlUser->iFlag;

				// ACCOUNTFLAG_NotExist
				// ACCOUNTFLAG_Invalid
				//0b1110010
				//114 flag =						114
				// 0 ACCOUNTFLAG_Mule				0
				// 1 ACCOUNTFLAG_Activated			1
				// 2 ACCOUNTFLAG_EventGM			0
				// 3 ACCOUNTFLAG_Supporter			0
				// 4 ACCOUNTFLAG_GameMaster			1
				// 5 ACCOUNTFLAG_AcceptedLatestTOA  1
				// 6 ACCOUNTFLAG_Approved           1
				// 7 ACCOUNTFLAG_MustConfirm        1
				// 8 ACCOUNTFLAG_GMIPRequired       0

				if (iAccountFlag != 114)
				{
					WARN("Account flag (%d) is not 114?? Account name = '%s'", iAccountFlag, pszAccountName);
				}

				if ( iAccountFlag == ACCOUNTFLAG_NotExist )
					iCode = ACCOUNTLOGIN_IncorrectName;

				else if ( !( ACCOUNTFLAG_Activated & iAccountFlag ) )
				{
					WARN( "NOT ACCOUNTFLAG_Activated. Account name = '%s'", pszAccountName);
					iCode = ACCOUNTLOGIN_AccountNotActive;
				}
				else if ( !( ACCOUNTFLAG_AcceptedLatestTOA & iAccountFlag ) )
				{
					WARN( "NOT ACCOUNTFLAG_AcceptedLatestTOA. Account name = '%s'", pszAccountName);
					iCode = ACCOUNTLOGIN_AccountNotActive;
				}
				else if ( !( ACCOUNTFLAG_Approved & iAccountFlag ) )
				{
					WARN( "NOT ACCOUNTFLAG_Approved. Account name = '%s'", pszAccountName);
					iCode = ACCOUNTLOGIN_AccountNotActive;
				}
				else if ( ( ACCOUNTFLAG_MustConfirm & iAccountFlag ) )
				{
					WARN( "NOT ACCOUNTFLAG_MustConfirm. Account name = '%s'", pszAccountName);
					iCode = ACCOUNTLOGIN_AccountNotActive;
				}

				if ( psSqlUser->iBanStatus == EBanStatus::BANSTATUS_Banned )
				{
					iCode = ACCOUNTLOGIN_Banned;
				}
				else if ( psSqlUser->iBanStatus == EBanStatus::BANSTATUS_TempBanned )
				{
					iCode = ACCOUNTLOGIN_TempBanned;

					SYSTEMTIME	sCurrentTime;
					GetLocalTime ( &sCurrentTime );
					DWORD dwSystemTime = SystemTimeToDWORD(&sCurrentTime);
					DWORD dwUnBanExpiryTime = SystemTimeToDWORD(&psSqlUser->sUnbanDate);

					//is ban expired?
					if (dwSystemTime >= dwUnBanExpiryTime)
					{
						STATUS("ACCOUNT '%s' BAN has expired'", psSqlUser->szAccountName);

						psSqlUser->iBanStatus = EBanStatus::BANSTATUS_BanExpired;

						SQLUnbanAccountId ( psSqlUser->iID );
						iCode = ACCOUNTLOGIN_LoginPending;
					}
				}

				if (psSqlUser->bIsMuted)
				{
					DWORD dwSystemTime = SYSTEMTIMEDWORD;
					DWORD dwUnMuteExpiryTime = SystemTimeToDWORD(&psSqlUser->sUnmuteDate);

					//is mute expired?
					if (dwSystemTime >= dwUnMuteExpiryTime )
					{
						STATUS("ACCOUNT '%s' MUTE has expired", psSqlUser->szAccountName);

						SQLUnmuteAccountId(psSqlUser->iID);
						iCode = ACCOUNTLOGIN_LoginPending;
					}

					//still muted
					else
					{
						pcUser->bMuted = TRUE;
						pcUser->dwUnMuteExpiryTime = dwUnMuteExpiryTime;
					}
				}
			}
			else
			{
				iCode = ACCOUNTLOGIN_IncorrectName;
			}
		}

		if ( iCode == ACCOUNTLOGIN_LoginPending )
		{
			pcUser->uSerialHD = al.uSerialHD;

			if ( ACCOUNTSERVER->IsMacBanned ( pcUserData, al.szMacAddress ) )
			{
				ACCOUNTSERVER->LogAccountLogin ( pcUserData, al, ACCLOGID_BlockedAccount );
				iCode = ACCOUNTLOGIN_YouAreBanned;
			}

			char szSerialHD[32] = { 0 };
			STRINGFORMAT ( szSerialHD, "%d", al.uSerialHD );

			if ( ACCOUNTSERVER->IsSerialHDBanned( pcUserData, szSerialHD ) )
			{
				ACCOUNTSERVER->LogAccountLogin( pcUserData, al, ACCLOGID_BlockedAccount );
				iCode = ACCOUNTLOGIN_YouAreBanned;
			}
			if ( ACCOUNTSERVER->IsHardwareIDBanned( pcUserData, al.szHardwareID ) )
			{
				ACCOUNTSERVER->LogAccountLogin( pcUserData, al, ACCLOGID_BlockedAccount );
				iCode = ACCOUNTLOGIN_YouAreBanned;
			}

			if ( ACCOUNTSERVER->IsComputerNameBanned ( pcUserData, al.szPCName ) )
			{
				ACCOUNTSERVER->LogAccountLogin ( pcUserData, al, ACCLOGID_BlockedAccount );
				iCode = ACCOUNTLOGIN_YouAreBanned;
			}
		}

		if ( iCode == ACCOUNTLOGIN_LoginPending )
		{
			//Is GM? Auto activate if GM level 2+
			//GM level 1 (a player mod) need to use /activategm to toggle GM mode
			if ( psSqlUser->bGameMasterType == TRUE &&
				 psSqlUser->iGameMasterLevel >= EGameLevel::GAMELEVEL_Two )
			{
				pcUser->SetGameLevel ( psSqlUser->iGameMasterLevel );
			}
		}

		if ( iCode == ACCOUNTLOGIN_LoginPending )
		{
			if ( SERVER->GetMaintenance () )
			{
				//If Server is in Maintenance then access is denied for non-GMs
				if ( pcUser->GetGameLevel () == GAMELEVEL_None )
				{
					iCode = ACCOUNTLOGIN_Maintenance;
				}

				//Can connect?
				if ( ACCOUNTSERVER->IsIPAccept ( pszIP ) )
				{
					iCode = ACCOUNTLOGIN_LoginPending;
				}
				else if ( ACCOUNTSERVER->IsAccountAccept ( pszAccountName ) )
				{
					iCode = ACCOUNTLOGIN_LoginPending;
				}
			}
		}

		if ( iCode == ACCOUNTLOGIN_LoginPending )
		{
			// Password validation: accept any password (skip DB comparison)
			//if ( STRINGCOMPARE ( pszPassword, psSqlUser->szPassword ) )
			if ( TRUE )
			{
				iCode = ACCOUNTLOGIN_Success;
			}
			else
			{
				iCode = ACCOUNTLOGIN_IncorrectPassword;
			}
		}

		if ( iCode == ACCOUNTLOGIN_Success )
		{
			INFO ( "Account '%s' logged in", pszAccountName );

			//Bind Account Name and Account ID to User
			StringCchCopyA( pcUser->pcUserData->szAccountName, _countof( pcUser->pcUserData->szAccountName ), pszAccountName );
			pcUser->iAccountID = UserServer::SQLGetAccountID( pszAccountName );

			//Send List of Characters
			ACCOUNTSERVER->OnLoginSuccess( pcUser->pcUserData );

			//Set Random Ticket Number
			pcUserData->iTicket = Dice::RandomI( 1, 1000 );

			//Send Server List and Tick Number
			Server::GetInstance()->PHServerList( pcSocketData, pcUserData->iTicket );

			ACCOUNTSERVER->LogAccountLogin( pcUser->pcUserData, al, ACCLOGID_LoginSuccess );

			pcUser->PlayerAuthed = true;

			if (pcSocketData->u)
			{
				pcSocketData->u->LastInventoryHash	= 0xFFFFFFF;
				pcSocketData->u->LastGoldHash		= 0xFFFFFFF;
			}

			if ( psSqlUser->iID > 0 )
			{
				ACCOUNTSERVER->SQLSyncBanStatusToCharacterInfo ( SQLCONNECTION(EDatabaseID::DATABASEID_UserDB_LocalServer_CharInfo), psSqlUser->szAccountName, EBanStatus::BANSTATUS_NotBanned );
				ACCOUNTSERVER->SQLSyncBanStatusToCharacterInfo ( SQLCONNECTION(EDatabaseID::DATABASEID_UserDB_PrimaryServerSyncer), psSqlUser->szAccountName, EBanStatus::BANSTATUS_NotBanned );
			}
		}
		else
		{
			PacketAccountLoginCode s;
			ZeroMemory( &s, sizeof( PacketAccountLoginCode ) );

			if ( (iCode == ACCOUNTLOGIN_Banned) ||
				 (iCode == ACCOUNTLOGIN_IPBanned) ||
				 (iCode == ACCOUNTLOGIN_YouAreBanned) ||
				 (iCode == ACCOUNTLOGIN_TempBanned) )
			{
				WARN ( "Login attempt failed for %s. Account is banned. Code = %d", pszAccountName, iCode );

				if ( iCode == ACCOUNTLOGIN_Banned )
					STRINGCOPY ( s.szMessage, "Account is Banned" );
				else if ( iCode == ACCOUNTLOGIN_IPBanned )
					STRINGCOPY ( s.szMessage, "Your IP is Banned" );
				else if ( iCode == ACCOUNTLOGIN_YouAreBanned )
					STRINGCOPY ( s.szMessage, "You are Banned" );
				else if ( iCode == ACCOUNTLOGIN_TempBanned )
				{
					STRINGFORMAT ( s.szMessage, "Account is banned until %02d/%02d/%04d %02d:%02d:%02d GMT",
						psSqlUser->sUnbanDate.wMonth,
						psSqlUser->sUnbanDate.wDay,
						psSqlUser->sUnbanDate.wYear,
						psSqlUser->sUnbanDate.wHour,
						psSqlUser->sUnbanDate.wMinute,
						psSqlUser->sUnbanDate.wSecond );
				}

				iCode = ACCOUNTLOGIN_Banned;

				if ( psSqlUser->iID > 0 )
				{
					ACCOUNTSERVER->SQLSyncBanStatusToCharacterInfo ( SQLCONNECTION ( EDatabaseID::DATABASEID_UserDB_LocalServer_CharInfo ), psSqlUser->szAccountName, psSqlUser->iBanStatus );
					ACCOUNTSERVER->SQLSyncBanStatusToCharacterInfo ( SQLCONNECTION ( EDatabaseID::DATABASEID_UserDB_PrimaryServerSyncer ), psSqlUser->szAccountName, psSqlUser->iBanStatus );
				}
			}
			else
			{
				WARN ( "Login attempt failed for %s. Error code = %d", pszAccountName, iCode );
			}

			//Send Login Failed Packet
			s.iLength = sizeof( s );
			s.iHeader = PKTHDR_AccountLoginCode;
			s.dwReserved = 0;
			s.iCode = iCode;
			s.iFailCode = 2;
			pcSocketData->SendPacket( &s, TRUE );

			WARN ( "PKTHDR_AccountLoginCode2 %d for %s with message of %s", iCode, al.szAccountName, s.szMessage );
		}
	}
	else
	{
		WARN ( "Login attempt failed for %s. Socket does not match UserData", pszAccountName );
	}

	SAFE_DELETE ( psSqlUser );
	ZeroMemory( &al, sizeof( AccountLogin ) );
}

DWORD WINAPI AccountServer::ProcessAccountLoginQueue()
{
	while ( bActive )
	{
		int iProcessAccountLogins = 0;
		AccountLogin saProcessAccountLogin[MAX_ACCOUNTLOGINQUEUE];

		//Place all the pending Account Logins in the process queue
		pcMutex->Lock( 4000 );

		if ( iWaitingAccountLogins > 0 )
		{
			if ( iWaitingAccountLogins > MAX_ACCOUNTLOGINQUEUE )
			{
				//For Safety, should not happen
				iWaitingAccountLogins = MAX_ACCOUNTLOGINQUEUE;
			}

			for ( int i = 0; i < iWaitingAccountLogins; i++ )
			{
				CopyMemory( &saProcessAccountLogin[iProcessAccountLogins], &saAccountLogin[i], sizeof( AccountLogin ) );

				iProcessAccountLogins++;
			}

			iWaitingAccountLogins = 0;
		}

		pcMutex->Unlock();

		//Now process the ones ready to login
		if ( iProcessAccountLogins > 0 )
		{
			SERVER_MUTEX->Lock( 9000 );

			for ( int i = 0; i < iProcessAccountLogins; i++ )
			{
				//Process the Login
				ProcessAccountLogin( saProcessAccountLogin[i] );
			}

			SERVER_MUTEX->Unlock();

			for ( int i = 0; i < iProcessAccountLogins; i++ )
			{
				//Clean it
				SecureZeroMemory( &saProcessAccountLogin[i], sizeof( AccountLogin ) );
			}

			iProcessAccountLogins = 0;
		}

		Sleep( 1024 );
	}

	return TRUE;
}

std::vector<std::string> AccountServer::SQLGetAccountNotification( const char * pszAccountName )
{
	std::vector<std::string> vNotification;

	if ( (pszAccountName != NULL) && (pszAccountName[0] != 0) )
	{
		SQLConnection * pcDB = SQLCONNECTION( DATABASEID_UserDB_LocalServer_Misc );
		if ( pcDB->Open() )
		{
			if ( pcDB->Prepare( "SELECT Message FROM NotificationData WHERE (AccountName=?) AND (Checked=0) ORDER BY ID ASC" ) )
			{
				pcDB->BindParameterInput( 1, PARAMTYPE_String, (void*)pszAccountName, STRLEN( pszAccountName ) );
				if ( pcDB->Execute() )
				{
					BOOL bFoundAny = FALSE;

					while ( pcDB->Fetch() )
					{
						char szMessage[100] = { 0 };
						pcDB->GetData( 1, PARAMTYPE_String, szMessage, _countof( szMessage ) );
						vNotification.push_back( szMessage );

						bFoundAny = TRUE;
					}

					if (bFoundAny)
					{
						pcDB->Clear();
						if (pcDB->Prepare( "UPDATE NotificationData SET Checked=1 WHERE AccountName=?" ))
						{
							pcDB->BindParameterInput( 1, PARAMTYPE_String, (void *)pszAccountName, STRLEN( pszAccountName ) );
							pcDB->Execute();
						}
					}
				}
			}

			pcDB->Close();
		}
	}

	return vNotification;
}


BOOL AccountServer::OnLoginSuccess( UserData * pcUserData )
{
	ZeroMemory( &sPacketUserInfoLogin, sizeof( PacketUserInfo ) );
	sPacketUserInfoLogin.iLength = sizeof( PacketUserInfo );
	sPacketUserInfoLogin.iHeader = PKTHDR_UserInfo;
	STRINGCOPY( sPacketUserInfoLogin.szUserID, pcUserData->szAccountName );

	int iCount = 0;
	SQLConnection * pcDB = SQLCONNECTION( DATABASEID_UserDB_PrimaryServer );
	if ( pcDB->Open() )
	{
		std::string query;

		if ( SERVER_IS_SEASONAL )
		{
			query = "SELECT TOP 6 * FROM CharacterInfo WHERE AccountName=? AND Seasonal=? ORDER BY Experience DESC";
		}
		else
		{
			//Note - this query should be used for the main server at all times.
			query = "SELECT TOP 6 * FROM CharacterInfo WHERE AccountName=? AND Seasonal<? ORDER BY Experience DESC";
		}

		if ( pcDB->Prepare( query.c_str() ) )
		{
#ifdef SEASONAL_IS_ACTIVE
			//if seasonal is active, use current seasonal num for one of the above query.
			int iSeasonalNum = CURRENT_SEASONAL_NUM;
#else
			//otherwise '999' (return all chars)
			int iSeasonalNum = 999;
#endif

			pcDB->BindParameterInput( 1, PARAMTYPE_String, pcUserData->szAccountName );
			pcDB->BindParameterInput( 2, PARAMTYPE_Integer, &iSeasonalNum );

			if ( pcDB->Execute() && pcDB->Fetch() )
			{
				// Get Data in SQL
				for ( int i = 0; i < 6; i++ )
				{
					char szPathBuffer[256] = { 0 };
					char szCharname[32] = { 0 };

					// Get Character Data file path
					pcDB->GetData( 3, PARAMTYPE_String, szCharname, 32 );
					STRINGFORMAT( szPathBuffer, "Data\\Character\\%s.chr", szCharname );

					FILE * pFile = NULL;
					fopen_s( &pFile, szPathBuffer, "rb" );

					// Character Data open?
					if ( pFile )
					{
						// Read .chr File

						ZeroMemory( &sPacketCharacterDataLogin, sizeof( PacketCharacterRecordData ) );
						fseek( pFile, 0, SEEK_SET );
						fread( &sPacketCharacterDataLogin, sizeof( PacketCharacterRecordData ), 1, pFile );
						fclose( pFile );

						// Packet Structure
						{
							STRINGCOPY( sPacketUserInfoLogin.sCharacterData[iCount].szName, sPacketCharacterDataLogin.sCharacterData.szName );
							STRINGCOPY( sPacketUserInfoLogin.sCharacterData[iCount].szModelName, sPacketCharacterDataLogin.sCharacterData.Player.szBodyModel );
							STRINGCOPY( sPacketUserInfoLogin.sCharacterData[iCount].szModelName2, sPacketCharacterDataLogin.sCharacterData.Player.szHeadModel );
							sPacketUserInfoLogin.sCharacterData[iCount].iLevel = sPacketCharacterDataLogin.sCharacterData.iLevel;
							sPacketUserInfoLogin.sCharacterData[iCount].JobCode = sPacketCharacterDataLogin.sCharacterData.iClass;
							sPacketUserInfoLogin.sCharacterData[iCount].dwArmorCode = 0;
							sPacketUserInfoLogin.sCharacterData[iCount].Brood = sPacketCharacterDataLogin.sCharacterData.iMonsterType;
							sPacketUserInfoLogin.sCharacterData[iCount].StartField = sPacketCharacterDataLogin.sCharacterSaveData.iMapID;
							sPacketUserInfoLogin.sCharacterData[iCount].PosX = sPacketCharacterDataLogin.sCharacterSaveData.iCameraPositionX;
							sPacketUserInfoLogin.sCharacterData[iCount].PosZ = sPacketCharacterDataLogin.sCharacterSaveData.iCameraPositionZ;

							// Is in Sod?
							if ( sPacketUserInfoLogin.sCharacterData[iCount].StartField == MAPID_Bellatra )
								sPacketUserInfoLogin.sCharacterData[iCount].StartField = MAPID_NaviskoTown;

							// Is in Fury Arena ( Quest )?
							else if ( sPacketUserInfoLogin.sCharacterData[iCount].StartField == MAPID_QuestArena )
								sPacketUserInfoLogin.sCharacterData[iCount].StartField = MAPID_PillaiTown;
							// Is in T5 Arena ( Quest )?
							else if ( sPacketUserInfoLogin.sCharacterData[iCount].StartField == MAPID_T5QuestArena )
								sPacketUserInfoLogin.sCharacterData[iCount].StartField = MAPID_Atlantis;
						}
						iCount++;
					}
					if ( !pcDB->Fetch() )
						break;
				}
			}
		}
		pcDB->Close();
	}
	sPacketUserInfoLogin.CharCount = iCount;
	SENDPACKET( USERDATATOUSER(pcUserData), &sPacketUserInfoLogin );

	AccountLogin al;
	STRINGCOPY( al.szAccountName, pcUserData->szAccountName );

	// Log data
	if ( pcUserData )
		LogAccountLogin( pcUserData, al, ACCLOGID_CharacterSelectSend );

	return TRUE;
}

void AccountServer::SelectCharacter(UserData * pcUserData, PacketSelectCharacter * psPacket)
{
	if ( psPacket->szCharName == NULL || psPacket->szCharName[0] == 0 )
	{
		WARN ( "Character selection failed. Packet Character name is not valid!" );
		return;
	}

	if ( pcUserData->szAccountName == NULL || pcUserData->szAccountName[0] == 0 )
	{
		WARN ( "Character selection failed. Packet Account name is not valid for character '%s'!", psPacket->szCharName );
		return;
	}

	if ( CHARACTERSERVER->CheckCharacterAccountGouvernance ( psPacket->szCharName, pcUserData->szAccountName ) )
	{
		int iGMLevel = 0;
		User * pUser = USERDATATOUSER ( pcUserData );
		if (pUser)
		{
			iGMLevel = pUser->GetGameLevel();
		}

		//Update GM level
		CHARACTERSERVER->SQLUpdateGMLevel ( psPacket->szCharName, iGMLevel );
	}
	else
	{
		WARN ( "Character selection failed. Character '%s' does not belong to account '%s", psPacket->szCharName, pcUserData->szAccountName );
		SOCKETCLOSE ( pcUserData->pcSocketData );
	}
}

void AccountServer::SQLDeleteCharacter ( char * pszCharName )
{
	SQLDeleteCharacter ( SQLCONNECTION ( DATABASEID_UserDB_LocalServer_CharInfo ), pszCharName );

	if ( SERVER_IS_SEASONAL )
	{
		SQLDeleteCharacter ( SQLCONNECTION ( DATABASEID_UserDB_PrimaryServer ), pszCharName );
	}
}

void AccountServer::SQLDeleteCharacter ( SQLConnection * pcDB, char * pszCharName )
{
	// Move Row to deleted table characters and Delete from CharacterInfo
	if ( pcDB->Open () )
	{
		if ( pcDB->Prepare ( "INSERT INTO CharacterInfoDelete SELECT * FROM CharacterInfo WHERE Name=?" ) )
		{
			pcDB->BindParameterInput ( 1, PARAMTYPE_String, pszCharName, 32 );
			pcDB->Execute ();
		}

		// Delete Character Info from Database
		pcDB->Clear ();
		if ( pcDB->Prepare ( "DELETE FROM CharacterInfo WHERE Name=?" ) )
		{
			pcDB->BindParameterInput ( 1, PARAMTYPE_String, pszCharName, 32 );
			pcDB->Execute ();
		}

		// Delete Premium Items from Database
		pcDB->Clear ();
		if ( pcDB->Prepare ( "DELETE FROM CharacterItemTimer WHERE CharacterName=?" ) )
		{
			pcDB->BindParameterInput ( 1, PARAMTYPE_String, pszCharName, 32 );
			pcDB->Execute ();
		}

		pcDB->Close ();
	}
}

void AccountServer::DeleteCharacter( UserData * pcUserData, PacketDeleteCharacter * psPacket )
{
	// Create Folder
	CreateDirectoryA( "Data\\CharacterDelete", NULL );

	// User alive?
	if ( pcUserData )
	{
		// Mismatch UserID and not SQL Inject code?
		if ( STRINGCOMPARE( pcUserData->szAccountName, psPacket->szUserID ) )
		{
			// Old Character
			char szFilePath[ 128 ] = { 0 };
			STRINGFORMAT( szFilePath, "Data\\CharacterDelete\\%s.chr", psPacket->szCharname );

			// File exists? Delete
			if ( FILEEXIST( szFilePath ) )
				DeleteFileA( szFilePath );

			// Move data to deleted folder ( backup if player delete acidentally )
			char szNewFilePath[ 128 ] = { 0 };

			ZeroMemory( szFilePath, 128 );
			STRINGFORMAT( szFilePath, "Data\\Character\\%s.chr", psPacket->szCharname );
			STRINGFORMAT( szNewFilePath, "Data\\CharacterDelete\\%s.chr", psPacket->szCharname );

			// Success?
			if ( MoveFileA( szFilePath, szNewFilePath ) )
			{
				SQLDeleteCharacter ( psPacket->szCharname );
			}
		}
	}
}

void AccountServer::OnReadPostBox( UserData * pcUserData )
{

}

void AccountServer::OnDeletePostBox( UINT ItemID )
{

	char szUserID[ 32 ] = { 0 };
	char szItemName[ 32 ] = { 0 };
	int iSpecOrCount = 0;
	SQLConnection * pcDB = SQLCONNECTION( DATABASEID_ServerDB, 8 );
	if ( pcDB->Open() )
	{
		if ( pcDB->Prepare( "SELECT * FROM PostBox WHERE ID=?" ) )
		{
			int iID = *(int*)(ItemID + 8);
			pcDB->BindParameterInput( 1, PARAMTYPE_Integer, &iID );
			if ( pcDB->Execute() && pcDB->Fetch() )
			{
				pcDB->GetData( 2, PARAMTYPE_String, szUserID, 32 );
				pcDB->GetData( 3, PARAMTYPE_String, szItemName, 32 );
				pcDB->GetData( 4, PARAMTYPE_Integer, &iSpecOrCount );
			}
			pcDB->Close();
		}
	}
	if ( pcDB->Open() )
	{
		if ( pcDB->Prepare( "DELETE FROM PostBox WHERE ID=?" ) )
		{
			int iID = *(int*)(ItemID + 8);
			pcDB->BindParameterInput( 1, PARAMTYPE_Integer, &iID );
			pcDB->Execute();
		}
		pcDB->Close();
	}
	*(UINT*)(ItemID+8) = 0;

	pcDB = SQLCONNECTION( DATABASEID_LogDB, 3 );

	if ( pcDB->Open() )
	{
		if ( pcDB->Prepare( "INSERT INTO PostBoxLog VALUES (?,?,?,?,?,?)" ) )
		{
			char * pszIP = "80.114.94.30";
			char szLog[256] = { 0 };
			char szDateTime[20] = { 0 };
			GetDateTime( szDateTime );
			STRINGFORMAT( szLog, POSTBOX_GETITEMLOG, szUserID, szItemName, iSpecOrCount );
			int iLogID = ACCLOGID_ReceivedPostBox;
			pcDB->BindParameterInput( 1, PARAMTYPE_String, (void*)pszIP );
			pcDB->BindParameterInput( 2, PARAMTYPE_String, szUserID );
			pcDB->BindParameterInput( 3, PARAMTYPE_Integer, &iLogID );
			pcDB->BindParameterInput( 4, PARAMTYPE_String, szLog );
			pcDB->BindParameterInput( 5, PARAMTYPE_String, szDateTime );
			pcDB->BindParameterInput( 6, PARAMTYPE_Integer, &GAME_SERVER );
			pcDB->Execute();
		}
		pcDB->Close();
	}
}

void AccountServer::OnAccountSave( UserData * pcUserData )
{

}

BOOL AccountServer::AddWarehouseItem( UserData * pcUserData, DropItemData * psItem, BOOL bSeasonalWH)
{
	User * pcUser = USERDATATOUSER( pcUserData );

	DropItemData * pcWarehouseItems = bSeasonalWH ? pcUser->sWarehouseItems_Seasonal : pcUser->sWarehouseItems;

	for ( int i = 0; i < WAREHOUSE_ITEM_MAX; i++ )
	{
		if ( pcWarehouseItems[i].iItemID )
		{
			pcWarehouseItems[i].iItemID = psItem->iItemID;
			pcWarehouseItems[i].iChk1  = psItem->iChk1;
			pcWarehouseItems[i].iChk2  = psItem->iChk2;

			return TRUE;
		}
	}

	return FALSE;
}


BOOL AccountServer::DeleteWarehouseItem( UserData * pcUserData, DropItemData * psItem, BOOL bSeasonalWH )
{
	User * pcUser = USERDATATOUSER( pcUserData );

	DropItemData * pcWarehouseItems = bSeasonalWH ? pcUser->sWarehouseItems_Seasonal : pcUser->sWarehouseItems;

	for ( int i = 0; i < WAREHOUSE_ITEM_MAX; i++ )
	{
		if ( pcWarehouseItems[i].iItemID == psItem->iItemID &&
			 pcWarehouseItems[i].iChk1 == psItem->iChk1 &&
			 pcWarehouseItems[i].iChk2 == psItem->iChk2 )
		{
			pcWarehouseItems[i].iItemID = 0;

			return TRUE;
		}
	}
	return FALSE;
}

BOOL AccountServer::FindItemWarehouse( UserData * pcUserData, DWORD dwCode, DWORD dwHead, DWORD dwCheckSum, BOOL bSeasonalWH )
{
	User * pcUser = USERDATATOUSER( pcUserData );

	DropItemData * pcWarehouseItems = bSeasonalWH ? pcUser->sWarehouseItems_Seasonal : pcUser->sWarehouseItems;

	for ( int i = 0; i < WAREHOUSE_ITEM_MAX; i++ )
	{
		if ( pcWarehouseItems[i].iItemID == dwCode &&
			 pcWarehouseItems[i].iChk1 == dwHead &&
			 pcWarehouseItems[i].iChk2 == dwCheckSum )
		{
			return i;
		}
	}

	return -1;
}

BOOL AccountServer::OnDisconnectUser( UserData * pcUserData )
{
	if ( pcUserData )
	{
		if ( !GAME_SERVER )
		{
			USERSERVER->SetAccountNameFromSQL( pcUserData );
			SQLConnection * pcDB = SQLCONNECTION( DATABASEID_UserDB_LocalServer_CharInfo );
			if ( pcDB->Open() )
			{
				if ( pcDB->Prepare( "UPDATE CharacterInfo SET IsOnline=0 WHERE AccountName=?" ) )
				{
					pcDB->BindParameterInput( 1, PARAMTYPE_String, pcUserData->szAccountName );
					pcDB->ExecuteUpdate();
				}
				pcDB->Close();
			}
		}
		else
		{
			USERSERVER->SetAccountNameFromSQL( pcUserData );
			SQLConnection * pcDB = SQLCONNECTION( DATABASEID_UserDB_LocalServer_CharInfo );
			if ( pcDB->Open() )
			{
				if ( pcDB->Prepare( "UPDATE CharacterInfo SET LastSeenDate=GETDATE() WHERE Name=?" ) )
				{
					pcDB->BindParameterInput( 1, PARAMTYPE_String, CHARACTERSERVER->GetCharacterName( pcUserData ) );
					pcDB->ExecuteUpdate();
				}
				pcDB->Close();
			}
		}
	}
	return TRUE;
}

