#include "stdafx.h"
#include "HNSSkill.h"



typedef void( __cdecl *t_ForeOrbAdd ) ( UserData * pcUserData, int v, UINT SkillArray, UINT packet34, UINT packet28, UINT packet2A );
t_ForeOrbAdd  ForceOrbAdd = ( t_ForeOrbAdd )0x00577F10;

typedef void( __cdecl *t_SendSkillEffect ) ( UserData * pcUserData, UserData * pcUserDataTarget, UINT skCode, UINT skLevel );
t_SendSkillEffect SendPacketSkillEffect = ( t_SendSkillEffect )0x0045142C;

typedef UINT( __cdecl *t_FindByUniqueID ) ( UINT id );
t_FindByUniqueID FindPlayerByUniqueID = ( t_FindByUniqueID )0x0054CC10; //GetUserdata
t_FindByUniqueID FindMonsterByUniqueID = ( t_FindByUniqueID )0x0054CC60;


CFighter										* pcFighter = NULL;
CMechanician									* pcMechanician = NULL;
CArcher											* pcArcher = NULL;
CPikeman										* pcPikeman = NULL;
CAtalanta										* pcAtalanta = NULL;
CKnight											* pcKnight = NULL;
CMagician										* pcMagician = NULL;
CPriestess										* pcPriestess = NULL;
CAssassin										* pcAssassin = NULL;
CShaman											* pcShaman = NULL;



/// <summary>
/// Skill info for all players
/// </summary>
std::map<ESkillID, SkillInfo>					mapSkillInfo;

//Common skills data that is needed by all classes, such as buffs
// as this buff data gets displayed at top corner
PacketSkillDataContainer sCommonSkillDataContainerAllClasses;

//Common skills info that is needed by all classes, such as buffs
// as this buff data gets displayed at top corner
PacketCommonSkillInfoContainer sCommonSkillInfoContainerAllClasses;


/// <summary>
/// Skill data for normal players
/// Key being Skill pointer id
/// </summary>
std::map<DWORD, SkillArrayData>					mapSkillData;


/// <summary>
/// Skill chain scale factor values
/// Key being Skill pointer id
/// if the mapping doesn't exist, then use default 1.0 scale factor
/// we only need to send non-1.0 data to client
/// mapping as following: skill array pointer >> (class id, scale factor)
/// </summary>
std::map<DWORD, std::tuple<int, float>>			mapSkillChainScale;

/// <summary>
/// Skill data for GM only. This is so that we can test changes beforehand (For balancing)
/// it only contains override values for mapSkillData
/// </summary>
std::map<DWORD, SkillArrayData>					mapSkillDataGM;

/// <summary>
/// Filtered list for skill info or data that we want to update only.
/// It is based on syncFlag value;
/// This is used to help filter in skills id that we want to update
/// </summary>
std::vector<int>							vSkillIdFilterList;
enum ECharacterClass						eSkillIdFilterCharacterClass;

time_t										uNextAutoSyncTime;

void InitSkills()
{
	if ( !pcFighter )
	{
		pcFighter = new CFighter;
		pcMechanician = new CMechanician;
		pcArcher = new CArcher;
		pcPikeman = new CPikeman;
		pcAtalanta = new CAtalanta;
		pcKnight = new CKnight;
		pcMagician = new CMagician;
		pcPriestess = new CPriestess;
		pcAssassin = new CAssassin;
		pcShaman = new CShaman;
	}
}

void UnInitSkills ()
{
	SAFE_DELETE ( pcFighter );
	SAFE_DELETE ( pcMechanician );
	SAFE_DELETE ( pcArcher );
	SAFE_DELETE ( pcPikeman );
	SAFE_DELETE ( pcAtalanta );
	SAFE_DELETE ( pcKnight );
	SAFE_DELETE ( pcMagician );
	SAFE_DELETE ( pcPriestess );
	SAFE_DELETE ( pcAssassin );
	SAFE_DELETE ( pcShaman );
}

/// <summary>
/// Tick at 1s interval
/// </summary>
void HNSSkill_Tick1s ()
{
	if ( LOGIN_SERVER )
		return;

	if ( TIME > uNextAutoSyncTime )
	{
		CheckAndAutoSyncNewSkillsDataforGMsOnly ( NULL );
		CheckAndAutoSyncNewSkillsInfoforGMsOnly ( NULL );

		uNextAutoSyncTime = TIME + 5; //5s later
	}
}

void CheckAndReSyncSkillsInfoForEveryone ()
{
	uNextAutoSyncTime = TIME + 10; //10s later

	eSkillIdFilterCharacterClass = ECharacterClass::CHARACTERCLASS_None;
	vSkillIdFilterList.clear ();

	int iSkillID;
	std::vector<int> ids;

	SQLConnection * pcDB = SQLCONNECTION ( DATABASEID_SkillDBNew, 1 );
	if ( pcDB->Open () )
	{
		if ( pcDB->Prepare ( "SELECT SkillId FROM SkillInfo WHERE SyncFlagInfo >= 1" ) )
		{
			if ( pcDB->Execute () )
			{
				while ( pcDB->Fetch () )
				{
					pcDB->GetData ( 1, PARAMTYPE_Integer, &iSkillID );

					ids.push_back ( iSkillID );
				}
			}
		}

		pcDB->Close ();
	}

	if ( ids.size () > 0 )
	{
		ZeroMemory ( &sCommonSkillInfoContainerAllClasses, sizeof ( PacketCommonSkillInfoContainer ) );
		sCommonSkillInfoContainerAllClasses.iHeader = PKTHDR_ServerCommonSkillInfo;
		sCommonSkillInfoContainerAllClasses.iCount = 0;

		UpdateServerSkillInfoData();

		//Get skill info from database and cache it locally
		//note this needs to be done before vSkillIdFilterList contains values.
		FIGHTERSKILL->UpdateAndCacheServerSkillInfo ();
		MECHANICIANSKILL->UpdateAndCacheServerSkillInfo ();
		ARCHERSKILL->UpdateAndCacheServerSkillInfo ();
		PIKEMANSKILL->UpdateAndCacheServerSkillInfo ();
		ASSASSINSKILL->UpdateAndCacheServerSkillInfo ();
		ATALANTASKILL->UpdateAndCacheServerSkillInfo ();
		KNIGHTSKILL->UpdateAndCacheServerSkillInfo ();
		MAGICIANSKILL->UpdateAndCacheServerSkillInfo ();
		PRIESTESSSKILL->UpdateAndCacheServerSkillInfo ();
		SHAMANSKILL->UpdateAndCacheServerSkillInfo ();

		//now start the filter process. only sync new ones to existing clients
		vSkillIdFilterList = ids;

		for ( int i = 0; i < PLAYERS_MAX; i++ )
		{
			if ( USERSDATA[i].pcSocketData == NULL )
				continue;

			//sync new skills data to user, including GMs
			SyncFilteredSkillsInfoforUser ( USERDATATOUSER ( &USERSDATA[i] ) );
		}

		if ( pcDB->Open () )
		{
			pcDB->Clear ();
			if ( pcDB->Prepare ( "UPDATE SkillInfo SET SyncFlagInfo=NULL" ) )
			{
				pcDB->Execute ();
			}
			pcDB->Close ();
		}

		eSkillIdFilterCharacterClass = ECharacterClass::CHARACTERCLASS_None;
		vSkillIdFilterList.clear ();
	}
}

const SkillInfo * GetSkillInfo(ESkillID p_SkillID)
{
	auto it = mapSkillInfo.find(p_SkillID);

	if (it != mapSkillInfo.end())
		return &it->second;
	else
		return nullptr;
}

/// <summary>
/// Get the scale factor the chained skill value
/// </summary>
float GetChainedSkillScale( enum ESkillArrayPointer arrayPointer )
{
	auto it = mapSkillChainScale.find(arrayPointer);
	if ( it != mapSkillChainScale.end() )
		return std::get<1>(it->second); //2nd part of the tuple is the scale value
	else
		return 1.0;
}

void ClearAllSyncFlags()
{
	SQLConnection * pcDB = SQLCONNECTION ( DATABASEID_SkillDBNew, 2 );
	if ( pcDB->Open () )
	{
		pcDB->Clear ();
		if ( pcDB->Prepare ( "UPDATE SkillInfo SET SyncFlagData=NULL, SyncFlagInfo=NULL" ) )
		{
			pcDB->Execute ();
		}
		pcDB->Close ();
	}
}

void CheckAndReSyncSkillsDataforEveryone()
{
	uNextAutoSyncTime = TIME + 10; //10s later

	eSkillIdFilterCharacterClass = ECharacterClass::CHARACTERCLASS_None;
	vSkillIdFilterList.clear ();

	int iSkillID;
	std::vector<int> ids;

	SQLConnection * pcDB = SQLCONNECTION ( DATABASEID_SkillDBNew, 3 );
	if ( pcDB->Open () )
	{
		if ( pcDB->Prepare ( "SELECT SkillId FROM SkillInfo WHERE SyncFlagData >= 1" ) )
		{
			if ( pcDB->Execute () )
			{
				while ( pcDB->Fetch () )
				{
					pcDB->GetData ( 1, PARAMTYPE_Integer, &iSkillID );

					ids.push_back ( iSkillID );
				}
			}
		}

		pcDB->Close ();
	}

	if ( ids.size () > 0 )
	{
		mapSkillData.clear ();
		mapSkillChainScale.clear();

		ZeroMemory ( &sCommonSkillInfoContainerAllClasses, sizeof ( PacketCommonSkillInfoContainer ) );
		sCommonSkillInfoContainerAllClasses.iHeader = PKTHDR_ServerCommonSkillInfo;
		sCommonSkillInfoContainerAllClasses.iCount = 0;

		//Get skill data from database and cache it locally
		FIGHTERSKILL->UpdateAndCacheServerSkillData ();
		MECHANICIANSKILL->UpdateAndCacheServerSkillData ();
		ARCHERSKILL->UpdateAndCacheServerSkillData ();
		PIKEMANSKILL->UpdateAndCacheServerSkillData ();
		ASSASSINSKILL->UpdateAndCacheServerSkillData ();
		ATALANTASKILL->UpdateAndCacheServerSkillData ();
		KNIGHTSKILL->UpdateAndCacheServerSkillData ();
		MAGICIANSKILL->UpdateAndCacheServerSkillData ();
		PRIESTESSSKILL->UpdateAndCacheServerSkillData ();
		SHAMANSKILL->UpdateAndCacheServerSkillData ();

		//Common skills data for all classes
		UpdateAndCacheServerCommonSkillDataForAllClasses ();

		//now start the filter process. only sync new ones to existing clients
		vSkillIdFilterList = ids;

		for ( int i = 0; i < PLAYERS_MAX; i++ )
		{
			if ( USERSDATA[i].pcSocketData == NULL )
				continue;

			//sync new skills data to user, including GMs
			SyncFilteredSkillsDataforUser ( USERDATATOUSER ( &USERSDATA[i] ) );
		}

		if ( pcDB->Open () )
		{
			pcDB->Clear ();
			if ( pcDB->Prepare ( "UPDATE SkillInfo SET SyncFlagData=NULL" ) )
			{
				pcDB->Execute ();
			}
			pcDB->Close ();
		}


		eSkillIdFilterCharacterClass = ECharacterClass::CHARACTERCLASS_None;
		vSkillIdFilterList.clear ();
	}

	//clear all existing GM mapping as everyons is now up to date.
	mapSkillDataGM.clear ();
}

/// <summary>
/// Checks SkillsInfo database and auto update GM clients
/// with the new skill data. Google spreadsheet sets the flag to '2'
/// which means 'auto update GM'. This flag will then get set to '1'
/// which means 'pending update for everyone'.
/// <param name="pcUser">If this is set, this sync will only be for that GM, otherwise for all GMs</param>
void CheckAndAutoSyncNewSkillsDataforGMsOnly ( User * pcGMUser)
{

	int iSyncFlag = pcGMUser == NULL ? 2 : 1; //2 is for all GMs (auto-update in real time), and 1 is for this GM (on login)
	int iSkillID;
	vSkillIdFilterList.clear ();

	SQLConnection * pcDB = SQLCONNECTION ( DATABASEID_SkillDBNew, 4 );
	if ( pcDB->Open () )
	{
		if ( pcDB->Prepare ( FormatString("SELECT SkillId FROM SkillInfo WHERE SyncFlagData=%d", iSyncFlag) ) )
		{
			if ( pcDB->Execute () )
			{
				while ( pcDB->Fetch () )
				{
					pcDB->GetData ( 1, PARAMTYPE_Integer, &iSkillID );

					if ( iSyncFlag == 2 )
						std::cout << "AUTO SYNCING GM SKILL DATA For Skill ID: " << iSkillID << std::endl;

					vSkillIdFilterList.push_back ( iSkillID );
				}
			}
		}

		pcDB->Close ();
	}

	if ( vSkillIdFilterList.size () > 0 )
	{
		if ( pcGMUser )
		{
			SyncFilteredSkillsDataforUser ( pcGMUser );
		}
		else
		{
			BOOL bCache = TRUE;
			for ( int i = 0; i < PLAYERS_MAX; i++ )
			{
				if ( USERSDATA[i].iGameLevel >= GAMELEVEL_One &&
					 USERSDATA[i].pcSocketData )
				{
					SyncFilteredSkillsDataforUser ( USERDATATOUSER ( &USERSDATA[i] ), bCache );

					bCache = FALSE; //only need to cache once.
				}
			}
		}

		if ( iSyncFlag == 2 )
		{
			if ( pcDB->Open () )
			{
				pcDB->Clear ();
				if ( pcDB->Prepare ( "UPDATE SkillInfo SET SyncFlagData=1 WHERE SyncFlagData=2" ) ) //1 is for everyone else.
				{
					pcDB->Execute ();
				}
				pcDB->Close ();
			}
		}

		vSkillIdFilterList.clear ();

	}
}

/// <summary>
/// Sync filtered skills data for user, and optionally cache it for GMs
/// </summary>
/// <param name="pcUser">The user object</param>
/// <param name="bCacheDataLocallyForGM">Should only be called if neccesary</param>
void SyncFilteredSkillsDataforUser ( User * pcUser, BOOL bCacheDataLocallyForGM )
{
	if ( pcUser == NULL || pcUser->IsValid () == FALSE )
		return;

	PacketSkillDataContainer lpDataContainer;
	ZeroMemory ( &lpDataContainer, sizeof ( PacketSkillDataContainer ) );

	eSkillIdFilterCharacterClass = pcUser->pcUserData->sCharacterData.iClass;

	//This class skill
	FIGHTERSKILL->BuildPacketSkillDataContainer		( &lpDataContainer,	 eSkillIdFilterCharacterClass == ECharacterClass::CHARACTERCLASS_Fighter );
	MECHANICIANSKILL->BuildPacketSkillDataContainer ( &lpDataContainer,	 eSkillIdFilterCharacterClass == ECharacterClass::CHARACTERCLASS_Mechanician );
	ARCHERSKILL->BuildPacketSkillDataContainer		( &lpDataContainer,	 eSkillIdFilterCharacterClass == ECharacterClass::CHARACTERCLASS_Archer );
	PIKEMANSKILL->BuildPacketSkillDataContainer		( &lpDataContainer,	 eSkillIdFilterCharacterClass == ECharacterClass::CHARACTERCLASS_Pikeman );
	ASSASSINSKILL->BuildPacketSkillDataContainer	( &lpDataContainer,	 eSkillIdFilterCharacterClass == ECharacterClass::CHARACTERCLASS_Assassin );
	ATALANTASKILL->BuildPacketSkillDataContainer	( &lpDataContainer,	 eSkillIdFilterCharacterClass == ECharacterClass::CHARACTERCLASS_Atalanta );
	KNIGHTSKILL->BuildPacketSkillDataContainer		( &lpDataContainer,	 eSkillIdFilterCharacterClass == ECharacterClass::CHARACTERCLASS_Knight );
	MAGICIANSKILL->BuildPacketSkillDataContainer	( &lpDataContainer,	 eSkillIdFilterCharacterClass == ECharacterClass::CHARACTERCLASS_Magician );
	PRIESTESSSKILL->BuildPacketSkillDataContainer	( &lpDataContainer,	 eSkillIdFilterCharacterClass == ECharacterClass::CHARACTERCLASS_Priestess );
	SHAMANSKILL->BuildPacketSkillDataContainer		( &lpDataContainer,	 eSkillIdFilterCharacterClass == ECharacterClass::CHARACTERCLASS_Shaman );

	if ( lpDataContainer.iCount > 0 )
	{
		lpDataContainer.iHeader = PKTHDR_ServerSkillData;
		lpDataContainer.iCharacterClass = pcUser->pcUserData->sCharacterData.iClass;
		lpDataContainer.iLength = sizeof ( Packet ) + 4 + 4 + ( lpDataContainer.iCount * sizeof ( SkillArrayData ) );

		SENDPACKET ( pcUser, &lpDataContainer );

		if ( bCacheDataLocallyForGM )
			CacheSkillArrayDataFromContainerForGMOnly ( &lpDataContainer );
	}

	eSkillIdFilterCharacterClass = ECharacterClass::CHARACTERCLASS_None;

	ZeroMemory ( &lpDataContainer, sizeof ( PacketSkillDataContainer ) );

	//Common class skills (buffs etc)
	FIGHTERSKILL->BuildPacketSkillDataContainer ( &lpDataContainer, FALSE );
	MECHANICIANSKILL->BuildPacketSkillDataContainer ( &lpDataContainer, FALSE );
	ARCHERSKILL->BuildPacketSkillDataContainer ( &lpDataContainer, FALSE );
	PIKEMANSKILL->BuildPacketSkillDataContainer ( &lpDataContainer, FALSE );
	ASSASSINSKILL->BuildPacketSkillDataContainer ( &lpDataContainer, FALSE );
	ATALANTASKILL->BuildPacketSkillDataContainer ( &lpDataContainer, FALSE );
	KNIGHTSKILL->BuildPacketSkillDataContainer ( &lpDataContainer, FALSE );
	MAGICIANSKILL->BuildPacketSkillDataContainer ( &lpDataContainer, FALSE );
	PRIESTESSSKILL->BuildPacketSkillDataContainer ( &lpDataContainer, FALSE );
	SHAMANSKILL->BuildPacketSkillDataContainer ( &lpDataContainer, FALSE );

	if ( lpDataContainer.iCount > 0 )
	{
		lpDataContainer.iHeader = PKTHDR_ServerSkillData;
		lpDataContainer.iCharacterClass = CHARACTERCLASS_None;
		lpDataContainer.iLength = sizeof ( Packet ) + 4 + 4 + ( lpDataContainer.iCount * sizeof ( SkillArrayData ) );

		SENDPACKET ( pcUser, &lpDataContainer );

		if ( bCacheDataLocallyForGM )
			CacheSkillArrayDataFromContainerForGMOnly ( &lpDataContainer );
	}


	SendSkillChainScaleDataToClient( pcUser );

	eSkillIdFilterCharacterClass = ECharacterClass::CHARACTERCLASS_None;
}

/// <summary>
/// Checks SkillsInfo database and auto update GM clients
/// with the new skill data. Google spreadsheet sets the flag to '2'
/// which means 'auto update GM'. This flag will then get set to '1'
/// which means 'pending update for everyone'.
/// </summary>
/// <param name="pcUser">If this is set, this sync will only be for that GM, otherwise for all GMs</param>
void CheckAndAutoSyncNewSkillsInfoforGMsOnly ( User * pcGMUser )
{
	int iSyncFlag = pcGMUser == NULL ? 2 : 1; //2 is for all GMs (auto-update in real time), and 1 is for this GM (on login)
	int iSkillID;
	vSkillIdFilterList.clear ();

	SQLConnection * pcDB = SQLCONNECTION ( DATABASEID_SkillDBNew, 5 );
	if ( pcDB->Open () )
	{
		if ( pcDB->Prepare ( FormatString("SELECT SkillId FROM SkillInfo WHERE SyncFlagInfo=%d", iSyncFlag )))
		{
			if ( pcDB->Execute () )
			{
				while ( pcDB->Fetch () )
				{
					pcDB->GetData ( 1, PARAMTYPE_Integer, &iSkillID );

					if ( iSyncFlag == 2 )
						std::cout << "AUTO SYNCING GM SKILL INFO For Skill ID: " << iSkillID << std::endl;

					vSkillIdFilterList.push_back ( iSkillID );
				}
			}
		}

		pcDB->Close ();
	}

	if ( vSkillIdFilterList.size () > 0 )
	{
		UpdateServerSkillInfoData();

		if ( pcGMUser )
		{
			SyncFilteredSkillsInfoforUser ( pcGMUser );
		}
		else
		{
			for ( int i = 0; i < PLAYERS_MAX; i++ )
			{
				if ( USERSDATA[i].iGameLevel >= GAMELEVEL_One &&
					 USERSDATA[i].pcSocketData )
				{
					SyncFilteredSkillsInfoforUser ( USERDATATOUSER ( &USERSDATA[i] ) );
				}
			}
		}

		if ( iSyncFlag == 2 )
		{
			if ( pcDB->Open () )
			{
				pcDB->Clear ();
				if ( pcDB->Prepare ( "UPDATE SkillInfo SET SyncFlagInfo=1 WHERE SyncFlagInfo=2" ) ) //1 is for everyone else.
				{
					pcDB->Execute ();
				}
				pcDB->Close ();
			}
		}

		vSkillIdFilterList.clear ();
		eSkillIdFilterCharacterClass = ECharacterClass::CHARACTERCLASS_None;
	}
}

/// <summary>
/// Sync filtered skills info for user.
/// Only new skill infos will get synced to user. See vSkillIdFilterList
/// </summary>
void SyncFilteredSkillsInfoforUser ( User * pcUser )
{
	if ( pcUser == NULL || pcUser->IsValid () == FALSE )
		return;

	PacketSkillInfoContainer lpInfoContainer;
	ZeroMemory ( &lpInfoContainer, sizeof ( PacketSkillInfoContainer ) );

	eSkillIdFilterCharacterClass = pcUser->pcUserData->sCharacterData.iClass;

	BuildPacketSkillInfoContainer ( &lpInfoContainer, ECharacterClass::CHARACTERCLASS_Fighter );
	BuildPacketSkillInfoContainer ( &lpInfoContainer, ECharacterClass::CHARACTERCLASS_Mechanician );
	BuildPacketSkillInfoContainer ( &lpInfoContainer, ECharacterClass::CHARACTERCLASS_Archer );
	BuildPacketSkillInfoContainer ( &lpInfoContainer, ECharacterClass::CHARACTERCLASS_Pikeman );
	BuildPacketSkillInfoContainer ( &lpInfoContainer, ECharacterClass::CHARACTERCLASS_Atalanta );
	BuildPacketSkillInfoContainer ( &lpInfoContainer, ECharacterClass::CHARACTERCLASS_Knight );
	BuildPacketSkillInfoContainer ( &lpInfoContainer, ECharacterClass::CHARACTERCLASS_Magician );
	BuildPacketSkillInfoContainer ( &lpInfoContainer, ECharacterClass::CHARACTERCLASS_Priestess );
	BuildPacketSkillInfoContainer ( &lpInfoContainer, ECharacterClass::CHARACTERCLASS_Assassin );
	BuildPacketSkillInfoContainer ( &lpInfoContainer, ECharacterClass::CHARACTERCLASS_Shaman );

	if ( lpInfoContainer.iCount > 0 )
	{
		lpInfoContainer.iHeader = PKTHDR_ServerSkillInfo;
		lpInfoContainer.iCharacterClass = pcUser->pcUserData->sCharacterData.iClass;
		lpInfoContainer.iLength = sizeof ( Packet ) + 4 + 4 + ( lpInfoContainer.iCount * sizeof ( SkillInfo ) );

		if ( lpInfoContainer.iLength > MAX_PKTSIZ)
		{
			LOGERROR( "SkillInfoContainer extended length of 0x2000 for class: %d", pcUser->pcUserData->sCharacterData.iClass );
		}

		SENDPACKET ( pcUser, &lpInfoContainer );
	}
}

void SendServerCommonSkillInfoToClient ( User * pcUser )
{
	SENDPACKET ( pcUser, &sCommonSkillInfoContainerAllClasses );
}

void SendServerCommonSkillDataToClient ( User * pcUser )
{
	SENDPACKET ( pcUser, &sCommonSkillDataContainerAllClasses );
}

void SendSkillChainScaleDataToClient( User * pcUser )
{
	if ( pcUser == NULL || pcUser->pcUserData == NULL )
		return;

	PacketGenericContainer packet;
	ZeroMemory( &packet, sizeof( PacketGenericContainer ) );
	packet.iHeader = PKTHDR_ServerSkillChainData;

	int classNum = pcUser->pcUserData->sCharacterData.iClass;

	map<DWORD, tuple<int, float>>::iterator it;
	for (it = mapSkillChainScale.begin(); it != mapSkillChainScale.end(); it++)
	{
		// check class
		if ( std::get<0>( it->second ) != classNum )
			continue;

		SkillChainScaleData sData;
		sData.dwSkillArrayPointer = it->first;
		sData.fScale = std::get<1>(it->second);

		int buffPos = packet.iCount * sizeof( SkillChainScaleData );
		CopyMemory( packet.baBuffer + buffPos, &sData, sizeof( SkillChainScaleData ) );
		packet.iCount += 1;
		packet.iLength = sizeof( Packet ) + 4 + 4 + packet.iCount * sizeof( SkillChainScaleData );
	}

	if ( packet.iCount > 0 )
	{
		SENDPACKET( pcUser, &packet );
	}
}

void UpdateAndCacheSkillInfoAndData()
{
	//clear all sync flags
	ClearAllSyncFlags();

	mapSkillDataGM.clear();
	mapSkillData.clear();
	mapSkillInfo.clear();
	mapSkillChainScale.clear();

	ZeroMemory ( &sCommonSkillInfoContainerAllClasses, sizeof ( PacketCommonSkillInfoContainer ) );
	sCommonSkillInfoContainerAllClasses.iHeader = PKTHDR_ServerCommonSkillInfo;
	sCommonSkillInfoContainerAllClasses.iCount = 0;

	//Get skill data from database and cache it locally
	FIGHTERSKILL->UpdateAndCacheServerSkillData ();
	MECHANICIANSKILL->UpdateAndCacheServerSkillData ();
	ARCHERSKILL->UpdateAndCacheServerSkillData ();
	PIKEMANSKILL->UpdateAndCacheServerSkillData ();
	ASSASSINSKILL->UpdateAndCacheServerSkillData ();
	ATALANTASKILL->UpdateAndCacheServerSkillData ();
	KNIGHTSKILL->UpdateAndCacheServerSkillData ();
	MAGICIANSKILL->UpdateAndCacheServerSkillData ();
	PRIESTESSSKILL->UpdateAndCacheServerSkillData ();
	SHAMANSKILL->UpdateAndCacheServerSkillData ();

	//Common skills data for all classes
	UpdateAndCacheServerCommonSkillDataForAllClasses ();

	//Get skill info from database and cache it locally
	FIGHTERSKILL->UpdateAndCacheServerSkillInfo ();
	MECHANICIANSKILL->UpdateAndCacheServerSkillInfo ();
	ARCHERSKILL->UpdateAndCacheServerSkillInfo ();
	PIKEMANSKILL->UpdateAndCacheServerSkillInfo ();
	ASSASSINSKILL->UpdateAndCacheServerSkillInfo ();
	ATALANTASKILL->UpdateAndCacheServerSkillInfo ();
	KNIGHTSKILL->UpdateAndCacheServerSkillInfo ();
	MAGICIANSKILL->UpdateAndCacheServerSkillInfo ();
	PRIESTESSSKILL->UpdateAndCacheServerSkillInfo ();
	SHAMANSKILL->UpdateAndCacheServerSkillInfo ();
}


void UpdateAndCacheServerCommonSkillDataForAllClasses ()
{
	ZeroMemory ( &sCommonSkillDataContainerAllClasses, sizeof ( PacketSkillDataContainer ) );
	sCommonSkillDataContainerAllClasses.iHeader = PKTHDR_ServerSkillData;
	sCommonSkillDataContainerAllClasses.iCharacterClass = CHARACTERCLASS_None;
	sCommonSkillDataContainerAllClasses.iCount = 0;

	INFO( "Updating and caching common skill data for all classes.." );

	FIGHTERSKILL->BuildPacketSkillDataContainer ( &sCommonSkillDataContainerAllClasses, FALSE );
	MECHANICIANSKILL->BuildPacketSkillDataContainer ( &sCommonSkillDataContainerAllClasses, FALSE );
	ARCHERSKILL->BuildPacketSkillDataContainer ( &sCommonSkillDataContainerAllClasses, FALSE );
	PIKEMANSKILL->BuildPacketSkillDataContainer ( &sCommonSkillDataContainerAllClasses, FALSE );
	ASSASSINSKILL->BuildPacketSkillDataContainer ( &sCommonSkillDataContainerAllClasses, FALSE );
	ATALANTASKILL->BuildPacketSkillDataContainer ( &sCommonSkillDataContainerAllClasses, FALSE );
	KNIGHTSKILL->BuildPacketSkillDataContainer ( &sCommonSkillDataContainerAllClasses, FALSE );
	MAGICIANSKILL->BuildPacketSkillDataContainer ( &sCommonSkillDataContainerAllClasses, FALSE );
	PRIESTESSSKILL->BuildPacketSkillDataContainer ( &sCommonSkillDataContainerAllClasses, FALSE );
	SHAMANSKILL->BuildPacketSkillDataContainer ( &sCommonSkillDataContainerAllClasses, FALSE );

	sCommonSkillDataContainerAllClasses.iLength = sizeof ( Packet ) + sizeof ( int ) + sizeof ( int ) + ( sCommonSkillDataContainerAllClasses.iCount * sizeof ( SkillArrayData ) );

}

void __cdecl AddFinalDamage( UserData * pcUserData, void * pPacket )
{
	User * pcUser = USERDATATOUSER ( pcUserData );

	if ( pcUser == NULL || pcUser->IsValid () == FALSE )
		return;

	PacketAttackData * psPacket = (PacketAttackData *)pPacket;

	ASSASSINSKILL->OnAddFinalDamage ( pcUser, psPacket );
	ARCHERSKILL->OnAddFinalDamage ( pcUser, psPacket );

	if ( pcUserData->saBCBuff[0].dwItemEndTime > TICKCOUNT )
	{
		if ( pcUserData->saBCBuff[0].iItemID == ITEMID_FatalEdge )
		{
			if ( pcUserData->iMapID == MAPID_Bellatra )
			{
				int iAdd = ( psPacket->iDamage * pcUserData->saBCBuff[0].iValueData ) / 100;

				CHATSERVER->SendDebugChat( pcUser, CHATCOLOR_Error, "> Fatal Edge[%d + %d[%d%%]] = %d", psPacket->iDamage, iAdd, pcUserData->saBCBuff[0].iValueData, psPacket->iDamage + iAdd );

				psPacket->iDamage += iAdd;
			}
		}
	}
}


/// <summary>
/// THis gets called each time the target (dwObjectID)
/// hits the player. Returning TRUE will mean
/// that the evasive was successful.
/// </summary>
BOOL __cdecl OnEvade ( UserData * pcUserData, DWORD dwObjectID )
{
	return FALSE;

	//User * pcUser = USERDATATOUSER ( pcUserData );
	//
	//if ( pcUser == NULL || pcUser->IsValid () == FALSE )
	//	return FALSE;
	//
	//UserData * pcUserEnemyData = NULL;
	//UnitData * pcUnitEnemyData = NULL;
	//
	//pcUserEnemyData = USERDATABYID( dwObjectID );
	//
	//short sTotalChance = 0;
	//char * pszName = NULL;
	//
	//if ( !pcUserEnemyData )
	//{
	//	pcUnitEnemyData = UNITSERVER->GetUnit( dwObjectID );
	//	pszName = pcUnitEnemyData->sCharacterData.szName;
	//}
	//else
	//{
	//	pszName = CHARACTERSERVER->GetCharacterName( pcUserEnemyData );
	//}

	//if ( pcUnitEnemyData || pcUserEnemyData )
	//{
	//
	//	sTotalChance += pcUser->sAlasEvasion;
	//	sTotalChance += pcUser->pcUserData->sCharacterData.eva
	//
	//
	//	if ( Dice::RandomS(0, 99) < pcUser->sAlasEvasion )
	//	{
	//		CHATSERVER->SendDebugChat( pcUser, CHATCOLOR_Error, "> %s Damage Evaded( Alas [%d] )", pszName, pcUser->sAlasEvasion );
	//
	//		PacketEvasion sPacket;
	//		sPacket.iLength		= sizeof( PacketEvasion );
	//		sPacket.iHeader		= PKTHDR_EvasionSuccess;
	//		sPacket.iUnk		= 0;
	//		sPacket.iSkillID	= AssassinSkill::Alas;
	//		sPacket.dwObjectID	= dwObjectID;
	//		SENDPACKET( pcUser, &sPacket );
	//		return TRUE;
	//	}
	//
	//}


	return FALSE;
}

int __cdecl UseDistortionMonster ( UserData * pcUserData, PacketSkillDamageRange * psPacket, int iLevelSkill )
{
	User * pcUser = USERDATATOUSER ( pcUserData );

	if ( pcUser == NULL || pcUser->IsValid () == FALSE )
		return FALSE;

	return MAGICIANSKILL->UseDistortionMonster ( pcUser, psPacket, iLevelSkill );
}

/// <summary>
/// Build skill info container using data from database
/// </summary>
void BuildPacketSkillInfoContainer ( PacketSkillInfoContainer * container, enum ECharacterClass eCharClass )
{
	if ( eSkillIdFilterCharacterClass != ECharacterClass::CHARACTERCLASS_None &&
		 eSkillIdFilterCharacterClass != eCharClass )
	{
		return;
	}

	PacketSkillInfoContainer temp;
	ZeroMemory ( &temp, sizeof ( PacketSkillInfoContainer ) );
	temp.iCount = 0;

	AddToSkillInfoContainer ( &temp, eCharClass );

	if ( temp.iCount > 0 )
	{
		CopyMemory ( &container->saSkillInfo[container->iCount], &temp.saSkillInfo, temp.iCount * sizeof ( SkillInfo ) );

		container->iHeader = PKTHDR_ServerSkillInfo;
		container->iCharacterClass = eCharClass;
		container->iCount += temp.iCount;

		container->iLength = sizeof ( Packet ) + 4 + 4 + ( container->iCount * sizeof ( SkillInfo ) );
	}
}

void UpdateServerSkillInfoData ( )
{
	std::string query = FormatString ( "Select [SkillId],[FlinchChance],[FlinchPushbackRange],[PVPScaling] FROM SkillInfo WHERE SyncFlagInfo >= 1" );

	SQLConnection * pcDB = SQLCONNECTION ( DATABASEID_SkillDBNew, 6 );
	if ( pcDB->Open () )
	{
		if ( pcDB->Prepare ( query.c_str () ) )
		{
			if ( pcDB->Execute () )
			{
				int skillId;
				short flinchChance;
				short flinchPushbackRange;
				float pvpScaling;

				while ( pcDB->Fetch () )
				{
					pcDB->GetData ( 1, PARAMTYPE_Integer, &skillId );

					pcDB->GetData ( 2, PARAMTYPE_Short, &flinchChance );
					pcDB->GetData ( 3, PARAMTYPE_Short, &flinchPushbackRange );
					pcDB->GetData ( 4, PARAMTYPE_Float, &pvpScaling );

					auto it = mapSkillInfo.find((ESkillID)skillId);
					if ( it != mapSkillInfo.end() )
					{
						it->second.iFlinchChance = static_cast<BYTE>(flinchChance);
						it->second.iPushbackRange = static_cast<BYTE>(flinchPushbackRange);
						it->second.pvpScaling = pvpScaling;
					}
					else
					{
						WARN( "Skill info not mapped for : %d", skillId );
					}
				}
			}
		}
		pcDB->Close ();
	}
}

/// <summary>
/// Populate skill info container from SQL database to be sent to client after char load
/// </summary>
/// <param name="spContainer">The packet container for skill info</param>
/// <param name="eCharClass">The character class enum</param>
void AddToSkillInfoContainer ( PacketSkillInfoContainer * spContainer, enum ECharacterClass eCharClass )
{
	if ( eSkillIdFilterCharacterClass != ECharacterClass::CHARACTERCLASS_None &&
		 eSkillIdFilterCharacterClass != eCharClass )
	{
		return;
	}

	SkillInfo * sSkillInfo = new SkillInfo ();

	char szSkillName[32] = { 0 };
	char szSkillFileName[32] = { 0 };
	char szSkillIconName[32] = { 0 };
	char szSkillDescription[150] = { 0 };


	std::string query = FormatString ( "Select [SkillNum],[SkillId],[SkillLevelReq],[SkillType],[SkillName],[SkillDescription],[SkillFileName],[SkillIconName],[FlinchChance],[FlinchPushbackRange],[FuncPointer],[ManaPointer],[SkillMasteryBase],[SkillMasteryIncreasePerLevel],[SkillStaminaBase],[SkillStaminaIncreasePerLevel],[ItemAllowedType1],[ItemAllowedType2],[ItemAllowedType3],[ItemAllowedType4],[ItemAllowedType5],[ItemAllowedType6],[ItemAllowedType7],[ItemAllowedType8],[PVPScaling] FROM SkillInfo WHERE Class=%d", (int)eCharClass );


	if ( vSkillIdFilterList.size () > 0 )
	{
		std::ostringstream oss;
		copy ( vSkillIdFilterList.begin (), vSkillIdFilterList.end () - 1, std::ostream_iterator<int> ( oss, "," ) );
		oss << vSkillIdFilterList.back (); //append last one without trailing ,

		std::string result ( oss.str () );

		query += " AND SkillId IN (" + oss.str () + ")";
	}

	SQLConnection * pcDB = SQLCONNECTION ( DATABASEID_SkillDBNew, 7 );
	if ( pcDB->Open () )
	{
		if ( pcDB->Prepare ( query.c_str () ) )
		{
			if ( pcDB->Execute () )
			{
				while ( pcDB->Fetch () )
				{
					ZeroMemory ( sSkillInfo, sizeof ( SkillInfo ) );

					int col = 1;

					int iSkillLevelReq = 0;
					int iSkillType = 0;


					pcDB->GetData ( col++, PARAMTYPE_Integer, &sSkillInfo->iSkillNum );
					pcDB->GetData ( col++, PARAMTYPE_Integer, &sSkillInfo->iSkillId );
					pcDB->GetData ( col++, PARAMTYPE_Integer, &iSkillLevelReq );
					pcDB->GetData ( col++, PARAMTYPE_Integer, &iSkillType);

					sSkillInfo->iSkillReqLevel = static_cast<BYTE>( iSkillLevelReq );
					sSkillInfo->iSkillType = static_cast<BYTE>( iSkillType );

					pcDB->GetData ( col++, PARAMTYPE_String, szSkillName, 32 );
					pcDB->GetData ( col++, PARAMTYPE_String, szSkillDescription, 150 );
					pcDB->GetData ( col++, PARAMTYPE_String, szSkillFileName, 32 );
					pcDB->GetData ( col++, PARAMTYPE_String, szSkillIconName, 32 );

					STRINGCOPYLEN ( sSkillInfo->szSkillName, 32, szSkillName );
					STRINGCOPYLEN ( sSkillInfo->szSkillDescription, 150, szSkillDescription );
					STRINGCOPYLEN ( sSkillInfo->szSkillFileName, 32, szSkillFileName );
					STRINGCOPYLEN ( sSkillInfo->szSkillIconName, 32, szSkillIconName );

					pcDB->GetData ( col++, PARAMTYPE_Byte, &sSkillInfo->iFlinchChance );
					pcDB->GetData ( col++, PARAMTYPE_Byte, &sSkillInfo->iPushbackRange );

					pcDB->GetData ( col++, PARAMTYPE_Integer, &sSkillInfo->dwFuncPointer);
					pcDB->GetData ( col++, PARAMTYPE_Integer, &sSkillInfo->dwManaPointer);

					pcDB->GetData ( col++, PARAMTYPE_Short, &sSkillInfo->sSkillMasteryBase);
					pcDB->GetData ( col++, PARAMTYPE_Short, &sSkillInfo->sSkillMasteryPerLevel);

					pcDB->GetData ( col++, PARAMTYPE_Short, &sSkillInfo->sSkillUseStaminaBase );
					pcDB->GetData ( col++, PARAMTYPE_Short, &sSkillInfo->sSkillUseStaminaPerLevel );

					// Apply SkillSPCostPercent multiplier from server.ini
					if ( SKILL_SP_COST_PERCENT != 100 )
					{
						sSkillInfo->sSkillUseStaminaBase    = (short)(sSkillInfo->sSkillUseStaminaBase    * (SKILL_SP_COST_PERCENT / 100.0f));
						sSkillInfo->sSkillUseStaminaPerLevel = (short)(sSkillInfo->sSkillUseStaminaPerLevel * (SKILL_SP_COST_PERCENT / 100.0f));
					}

					pcDB->GetData ( col++, PARAMTYPE_Integer, &sSkillInfo->iaItemAllowedType[0]);
					pcDB->GetData ( col++, PARAMTYPE_Integer, &sSkillInfo->iaItemAllowedType[1]);
					pcDB->GetData ( col++, PARAMTYPE_Integer, &sSkillInfo->iaItemAllowedType[2]);
					pcDB->GetData ( col++, PARAMTYPE_Integer, &sSkillInfo->iaItemAllowedType[3]);
					pcDB->GetData ( col++, PARAMTYPE_Integer, &sSkillInfo->iaItemAllowedType[4]);
					pcDB->GetData ( col++, PARAMTYPE_Integer, &sSkillInfo->iaItemAllowedType[5]);
					pcDB->GetData ( col++, PARAMTYPE_Integer, &sSkillInfo->iaItemAllowedType[6]);
					pcDB->GetData ( col++, PARAMTYPE_Integer, &sSkillInfo->iaItemAllowedType[7]);

					pcDB->GetData ( col++, PARAMTYPE_Float,   &sSkillInfo->pvpScaling);

					CopyMemory ( &spContainer->saSkillInfo[spContainer->iCount], sSkillInfo, sizeof ( SkillInfo ) );
					spContainer->iCount++;
				}
			}
		}
		pcDB->Close ();
	}

	SAFE_DELETE ( sSkillInfo );
}

void SendSelfBuffSkill( User * pcCaster, ESkillID eSKillId, int iLevel1Based, int iDurationSec, int iParameter )
{
	if ( pcCaster == FALSE || pcCaster->IsValidAndInGame() == FALSE )
		return;

	PacketUseSkillSingle sPacket;
	ZeroMemory( &sPacket, sizeof( PacketUseSkillSingle ) );
	sPacket.iLength = sizeof( PacketUseSkillSingle );
	sPacket.iHeader = PKTHDR_UseSkillNew;
	sPacket.iSkillID = eSKillId;
	sPacket.iSkillLevel1Based = iLevel1Based;
	sPacket.sTimeLeftSec = iDurationSec;
	sPacket.iParameter = iParameter;
	sPacket.eCasterCharacterClass = pcCaster->pcUserData->sCharacterData.iClass;
	sPacket.lCasterID = pcCaster->GetID();
	sPacket.lTargetID = pcCaster->GetID();

	SENDPACKETRANGE( &sPacket, &pcCaster->pcUserData->sPosition, pcCaster->pcUserData->iMapID, DISTANCE_MAX_SKILL_VISUAL);
}

/// <summary>
/// Gather up all party members including self
/// and then send this packet to everyone within skill visual range
/// which in turn will show the visual animation effect of the party buff
/// </summary>
/// <param name="pcCaster">The caster of the skill</param>
/// <param name="eSKillId">Skill ID</param>
/// <param name="iLevel1Based">Skill level (1 based)</param>
/// <param name="bInclRaidMembers">Set to TRUE to include raid members</param>
void SendPartyBuffSkill( User * pcCaster, ESkillID eSKillId, int iLevel1Based, int iDurationSec, int iParameter, BOOL bInclRaidMembers)
{
	if ( pcCaster == FALSE || pcCaster->IsValidAndInGame() == FALSE )
		return;

	PacketUseSkillParty sPacket;
	ZeroMemory( &sPacket, sizeof( PacketUseSkillParty ) );
	sPacket.iLength = sizeof( PacketUseSkillParty );
	sPacket.iHeader = PKTHDR_UseSkillPartyBuffNew;
	sPacket.iSkillID = eSKillId;
	sPacket.iSkillLevel1Based = iLevel1Based;
	sPacket.sTimeLeftSec = iDurationSec;
	sPacket.iParameter = iParameter;
	sPacket.eCasterCharacterClass = pcCaster->pcUserData->sCharacterData.iClass;
	sPacket.lCasterID = pcCaster->GetID();

	for ( User * pcUser : PARTYHANDLER->GetPartyMembersIncludingSelf( pcCaster, bInclRaidMembers ) )
	{
		if ( pcUser->pcUserData->sPosition.WithinXZDistance( &pcCaster->pcUserData->sPosition, DISTANCE_XY_15_meters, DISTANCE_MISC_Y_EX ) )
		{
			sPacket.aiUserID[sPacket.iUserCount] = pcUser->GetID();
			sPacket.iUserCount++;
		}
	}

	if ( sPacket.iUserCount <= 0 )
		return;

	SENDPACKETRANGE( &sPacket, &pcCaster->pcUserData->sPosition, pcCaster->pcUserData->iMapID, DISTANCE_MAX_SKILL_VISUAL);
}

/// <summary>
/// Gather up all party members including self
/// and then send this packet to everyone within skill visual range
/// which in turn will show the visual animation effect of the party buff
/// </summary>
/// <param name="pcCaster">The caster of the skill</param>
/// <param name="eSKillId">Skill ID</param>
/// <param name="iLevel1Based">Skill level (1 based)</param>
/// <param name="vpBuffUsers">List of users</param>
void SendPartyBuffSkill( User * pcCaster, ESkillID eSKillId, int iLevel1Based, int iDurationSec, int iParameter, std::vector<User*> vpBuffUsers )
{
	PacketUseSkillParty sPacket;
	ZeroMemory( &sPacket, sizeof( PacketUseSkillParty ) );
	sPacket.iLength = sizeof( PacketUseSkillParty );
	sPacket.iHeader = PKTHDR_UseSkillPartyBuffNew;
	sPacket.iSkillID = eSKillId;
	sPacket.iSkillLevel1Based = iLevel1Based;
	sPacket.sTimeLeftSec = iDurationSec;
	sPacket.iParameter = iParameter;
	sPacket.eCasterCharacterClass = pcCaster->pcUserData->sCharacterData.iClass;
	sPacket.lCasterID = pcCaster->GetID();

	for( User * pBuffUser : vpBuffUsers )
	{
		if ( pBuffUser == NULL || pBuffUser->IsValidAndInGame() == FALSE )
			continue;

		sPacket.aiUserID[sPacket.iUserCount] = pBuffUser->GetID();
		sPacket.iUserCount++;
	}

	if ( sPacket.iUserCount <= 0 )
		return;

	SENDPACKETRANGE( &sPacket, &pcCaster->pcUserData->sPosition, pcCaster->pcUserData->iMapID, DISTANCE_MAX_SKILL_VISUAL);
}

int GetSkillIntValueAt ( User * pcUser, enum ESkillArrayPointer dwSkillArrayId, int iLevel1Based, BOOL bChainned )
{
	int value;

	if ( iLevel1Based == 0 )
		return 0;

	float fScaleFactor = 1.0f;

	if ( bChainned )
	{
		fScaleFactor = GetChainedSkillScale( dwSkillArrayId );
	}

	if ( pcUser && pcUser->GetGameLevel () == GAMELEVEL_Four )
	{
		if ( mapSkillDataGM.find ( dwSkillArrayId ) != mapSkillDataGM.end () )
		{
			if ( iLevel1Based >= 1 && iLevel1Based <= 10 )
			{
				value = mapSkillDataGM[dwSkillArrayId].iSkillValues[iLevel1Based - 1];

				//no scale factor. return it as it is
				if ( fScaleFactor == 1.0f )
					return value;
				else
					return static_cast<int>( round( static_cast<float>( value ) * fScaleFactor ) );
			}
			else
			{
				WARN ( "Skill level value '%d' is incorrect for found for %d", iLevel1Based, dwSkillArrayId );
				return 0;
			}
		}
	}

	if ( mapSkillData.find ( dwSkillArrayId ) == mapSkillData.end () )
	{
		LOGERROR ( "Skill data not found for %d", dwSkillArrayId );
		return 0;
	}

	if ( iLevel1Based >= 1 && iLevel1Based <= 10 )
	{
		value = mapSkillData[dwSkillArrayId].iSkillValues[iLevel1Based - 1];

		//no scale factor, return it as it is
		if ( fScaleFactor == 1.0f )
			return value;
		else
			return static_cast<int>( round( static_cast<float>( value ) * fScaleFactor ) );
	}
	else
	{
		WARN ( "Skill level value '%d' is incorrect for found for %d", iLevel1Based, dwSkillArrayId );
		return 0;
	}
}

float GetSkillFloatValueAt ( User * pcUser, enum ESkillArrayPointer dwSkillArrayId, int iLevel1Based, BOOL bChainned )
{
	if ( iLevel1Based == 0 )
		return 0;

	float fScaleFactor = GetChainedSkillScale( dwSkillArrayId );

	if ( pcUser->GetGameLevel () == GAMELEVEL_Four )
	{
		if ( mapSkillDataGM.find ( dwSkillArrayId ) != mapSkillDataGM.end () )
		{
			if ( iLevel1Based >= 1 && iLevel1Based <= 10 )
			{
				return mapSkillDataGM[dwSkillArrayId].fSkillValues[iLevel1Based - 1] * fScaleFactor;
			}
			else
			{
				WARN ( "Skill level value '%d' is incorrect for found for %d", iLevel1Based, dwSkillArrayId );
				return 0.0f;
			}
		}
	}

	if ( mapSkillData.find ( dwSkillArrayId ) == mapSkillData.end () )
	{
		LOGERROR ( "Skill data not found for %d", dwSkillArrayId );
		return 0.0f;
	}

	if ( iLevel1Based >= 1 && iLevel1Based <= 10 )
	{
		return mapSkillData[dwSkillArrayId].fSkillValues[iLevel1Based - 1] * fScaleFactor;
	}
	else
	{
		WARN ( "Skill level value '%d' is incorrect for found for %d", iLevel1Based, dwSkillArrayId );
		return 0.0f;
	}
}

BOOL SQLGetSkillValueIntArray ( SkillArrayData * psSkillArrayData, ESkillID eSkillID, DWORD dwAddressOrIndex, std::string columnName )
{
	if ( SQLSKILL->GetSkillValueIntArray ( eSkillID, columnName, psSkillArrayData->iSkillValues ) )
	{
		psSkillArrayData->dwSkillArrayAddressOrIndex = dwAddressOrIndex;
		psSkillArrayData->iSkillID = (int)eSkillID;

		return TRUE;
	}
	else
	{
		WARN ( "SQLGetIntValues FAILED for id: %d column: %s", eSkillID, columnName.c_str () );

		return FALSE;
	}
}

int GetUserSkillLevel1Based(User * pcUser, ESkillID eSKillId)
{
	if ( eSKillId == ESkillID::SKILLID_DefaultAttack )
		return 0;

	if ( mapSkillInfo.find ( eSKillId ) == mapSkillInfo.end () )
	{
		LOGERROR ( "GetUserSkillLevelFromSkillID - Skill ID is not valid: %d", eSKillId );
		return 0;
	}

	int iSkillIndex = mapSkillInfo[eSKillId].iSkillNum - 1;

	if ( iSkillIndex < 0 || iSkillIndex > 16 )
	{
		LOGERROR ( "GetUserSkillLevelFromSkillID - Skill index %d is out of range for skill id: %d", iSkillIndex, eSKillId );
		return 0;
	}

	int level = pcUser->pData.iLevelSkill[iSkillIndex];

	if ( level < 0 && level > 10 )
	{
		WARN( "Incorrect skill level: %d for char: %s", level, pcUser->GetCharacterName() );
		return 0;
	}

	return level;
}

void CacheSkillInfoFromContainer  (PacketSkillInfoContainer * container  )
{
	SkillInfo * data = NULL;
	ESkillID eSkillID = ESkillID::SKILLID_Unknown;

	for ( int i = 0; i < container->iCount; i++ )
	{
		data = &container->saSkillInfo[i];
		eSkillID = (ESkillID)data->iSkillId;

		//party buff skills
		switch ( eSkillID )
		{
			//Mechanic
			case ESkillID::SKILLID_ElectricalOverload:
			//Fighter
			case ESkillID::SKILLID_DemonsBane:

			//Archer
			case ESkillID::SKILLID_ForceofNature:

			//Pikeman

			//Assassin
			case ESkillID::SKILLID_Inpes:
			case ESkillID::SKILLID_Alas:

			//Atalanta
			case ESkillID::SKILLID_TriumphofValhalla:
			case ESkillID::SKILLID_HallofValhalla:

			//Knight
			case ESkillID::SKILLID_HolyValor:

			//Magician
			case ESkillID::SKILLID_EnchantWeapon:

			//Priestess
			case ESkillID::SKILLID_VirtualLife:
			case ESkillID::SKILLID_VirtualRegen:

			//Shaman
			case ESkillID::SKILLID_AdventMigal:
			{
				if ( sCommonSkillInfoContainerAllClasses.iCount < MAX_COMMON_SKILL_INFO_DATA )
				{

					SkillInfoCommon * commonData = &sCommonSkillInfoContainerAllClasses.saData[sCommonSkillInfoContainerAllClasses.iCount];

					commonData->iSkillId = data->iSkillId;
					STRINGCOPYLEN( commonData->szSkillName, 32, data->szSkillName );
					STRINGCOPYLEN( commonData->szSkillDescription, 150, data->szSkillDescription );
					STRINGCOPYLEN( commonData->szSkillFileName, 32, data->szSkillFileName );
					STRINGCOPYLEN( commonData->szSkillIconName, 32, data->szSkillIconName );

					sCommonSkillInfoContainerAllClasses.iCount++;
					sCommonSkillInfoContainerAllClasses.iLength = sizeof( Packet ) + sizeof( int ) + ( sizeof( SkillInfoCommon ) * sCommonSkillInfoContainerAllClasses.iCount );
				}
				else
				{
					WARN( "Memory overflow would have occured!" );
				}
			}
			break;
		}

		if ( mapSkillInfo.find ( eSkillID ) == mapSkillInfo.end () )
		{
			SkillInfo localData;
			CopyMemory ( &localData, data, sizeof ( SkillInfo ) );
			mapSkillInfo.insert ( { eSkillID, localData } );
		}
		else
		{
			ZeroMemory( &mapSkillInfo[eSkillID], sizeof( SkillInfo ) );
			CopyMemory( &mapSkillInfo[eSkillID], data, sizeof( SkillInfo ) );
		}
	}
}

void CacheSkillArrayDataFromContainer ( PacketSkillDataContainer * container )
{
	UINT uPosition = 0;

	for ( int i = 0; i < container->iCount; i++ )
	{
		SkillArrayData * data = &container->skillArrayData[i];

		int skillArrayPointerId = data->dwSkillArrayAddressOrIndex & 0x0FFFFFFF; //remove injected value at 0xF0000000;

		if ( mapSkillData.find ( skillArrayPointerId ) == mapSkillData.end () )
		{
			SkillArrayData localData;
			CopyMemory ( &localData, data, sizeof ( localData ) );
			localData.dwSkillArrayAddressOrIndex = skillArrayPointerId;
			mapSkillData.insert ( { skillArrayPointerId, localData } );
		}
		else
		{
			CopyMemory ( mapSkillData[skillArrayPointerId].iSkillValues, data->iSkillValues, 10 * sizeof ( int ) );
		}
	}
}

void CacheSkillArrayDataFromContainerForGMOnly ( PacketSkillDataContainer * container )
{
	UINT uPosition = 0;

	for ( int i = 0; i < container->iCount; i++ )
	{
		SkillArrayData * data = &container->skillArrayData[i];

		int skillArrayPointerId = data->dwSkillArrayAddressOrIndex & 0x0FFFFFFF; //remove injected value at 0xF0000000;

		if ( mapSkillDataGM.find ( skillArrayPointerId ) == mapSkillData.end () )
		{
			SkillArrayData localData;
			CopyMemory ( &localData, data, sizeof ( localData ) );
			localData.dwSkillArrayAddressOrIndex = skillArrayPointerId;
			mapSkillDataGM.insert ( { skillArrayPointerId, localData } );
		}
		else
		{
			CopyMemory ( mapSkillDataGM[skillArrayPointerId].iSkillValues, data->iSkillValues, 10 * sizeof ( int ) );
		}
	}
}

/// <summary>
/// use this if you want to render something obsolete
/// for example, if something is buried deep in ASM and hard to modify
/// set to values to zero, then use a different array instead.
/// </summary>
/// <param name="container"></param>
/// <param name="iSkillID"></param>
/// <param name="dwAddressOrIndex"></param>
void AppendZeroIntValuesToServerSkillDataBuffer ( PacketSkillDataContainer * container, enum ESkillID eSkillID, DWORD dwAddressOrIndex )
{
	//the filter list is not empty. If the list doesn't contain this skill, don't add it.
	if ( vSkillIdFilterList.size () > 0 && std::find ( vSkillIdFilterList.begin (), vSkillIdFilterList.end (), (int)eSkillID ) == vSkillIdFilterList.end () )
		return;

	int values[10] = {};

	SkillArrayData * data = &container->skillArrayData[container->iCount];

	ZeroMemory ( data, sizeof (SkillArrayData) );

	data->dwSkillArrayAddressOrIndex = dwAddressOrIndex | 0x10000000; //1 = int array
	data->iSkillID = eSkillID;

	container->iCount++;
	container->iLength = sizeof ( Packet ) + sizeof ( int ) + sizeof ( int ) + ( container->iCount * sizeof ( SkillArrayData ) );
}

/// <summary>
/// Append int values for integer based skill values to skill data container to be passed on to client
/// </summary>
/// <param name="container">The container</param>
/// <param name="piBuffPosition">Reference to buffer position</param>
/// <param name="iSkillID">Skill id</param>
/// <param name="dwAddressOrIndex">Address or index (most significant byte)</param>
/// <param name="column">The column name to fetch the values from</param>
void AppendIntValuesToServerSkillDataBuffer ( PacketSkillDataContainer * container, enum ESkillID eSkillID, DWORD dwAddressOrIndex, std::string columnName, BOOL bServerOnly, BOOL bIgnoreWarning )
{
	//the filter list is not empty. If the list doesn't contain this skill, don't add it.
	if ( vSkillIdFilterList.size () > 0 && std::find ( vSkillIdFilterList.begin (), vSkillIdFilterList.end (), (int)eSkillID ) == vSkillIdFilterList.end () )
		return;

	//std::cout << "AppendToServerSkillDataBuffer: " << piBuffPosition << " : " << container->iAmount << std::endl;

	if ( container->iCount < 100 )
	{
		int values[10];

		if ( SQLSKILL->GetSkillValueIntArray ( eSkillID, columnName, values, bIgnoreWarning ) )
		{
			// Apply skill MP/SP cost multiplier from server.ini
			if (columnName == "MPCost" && SKILL_MP_COST_PERCENT != 100)
			{
				for (int i = 0; i < 10; i++)
					values[i] = (int)(values[i] * (SKILL_MP_COST_PERCENT / 100.0f));
			}

			SkillArrayData arrayData;
			arrayData.dwSkillArrayAddressOrIndex = dwAddressOrIndex;
			arrayData.iSkillID = (int)eSkillID;
			CopyMemory ( &arrayData.iSkillValues, &values, 10 * sizeof ( int ) );

			//if it's not for server side only, it's also for client side.
			if ( bServerOnly == FALSE )
			{
				CopyMemory ( &container->skillArrayData[container->iCount], &arrayData, sizeof ( SkillArrayData ) );
				container->skillArrayData[container->iCount].dwSkillArrayAddressOrIndex |= 0x10000000; //1 = int array;
				container->iCount++;

				container->iLength = sizeof ( Packet ) + sizeof ( int ) + sizeof ( int ) + ( container->iCount * sizeof ( SkillArrayData ) );
			}

			if ( mapSkillData.find ( dwAddressOrIndex ) == mapSkillData.end () )
			{
				//add new
				mapSkillData.insert ( { dwAddressOrIndex, arrayData } );
			}
			else
			{
				//replace existing
				CopyMemory ( &mapSkillData[dwAddressOrIndex].iSkillValues, &values, 10 * sizeof ( int ) );
			}

			float scale = 1.0f;
			int classNum = -1;
			if ( SQLSKILL->GetSkillChainScaleValue( eSkillID, columnName, &classNum, &scale) )
			{
				auto it = mapSkillChainScale.find( dwAddressOrIndex );
				if ( it != mapSkillChainScale.end() )
				{
					//update
					it->second = std::tuple<int, float>(classNum, scale);
				}
				else
				{
					//only score non 1.0f values
					if ( scale != 1.0f )
					{
						//add new
						mapSkillChainScale.insert( { dwAddressOrIndex, std::tuple<int, float>( classNum, scale ) } );
					}
				}
			}
		}
	}
	else
	{
		WARN( "Too much Skill data for class %d", container->iCharacterClass );
		//Note - if this happens, need to split up the data two into packets
	}
}


/// <summary>
/// Append float values for decimal based skill values to skill data container to be passed on to client
/// </summary>
/// <param name="container">The container</param>
/// <param name="piBuffPosition">Reference to buffer position</param>
/// <param name="iSkillID">Skill id</param>
/// <param name="dwAddressOrIndex">Address or index (most significant byte)</param>
/// <param name="query">The SQL query</param>
void AppendFloatValuesToServerSkillDataBuffer ( PacketSkillDataContainer * container, enum ESkillID eSkillID, DWORD dwAddressOrIndex, std::string columnName, BOOL bServerOnly, BOOL bIgnoreWarning )
{
	//the filter list is not empty. If the list doesn't contain this skill, don't add it.
	if ( vSkillIdFilterList.size () > 0 && std::find ( vSkillIdFilterList.begin (), vSkillIdFilterList.end (), (int)eSkillID ) == vSkillIdFilterList.end () )
		return;

	if ( container->iCount < 100 )
	{
		float values[10];

		if ( SQLSKILL->GetSkillValueFloatArray ( eSkillID, columnName, values, bIgnoreWarning ) )
		{
			SkillArrayData arrayData;
			arrayData.dwSkillArrayAddressOrIndex = dwAddressOrIndex;
			arrayData.iSkillID = (int)eSkillID;
			CopyMemory ( &arrayData.fSkillValues, &values, 10 * sizeof ( float ) );

			//if it's not for server side only, it's also for client side.
			if ( bServerOnly == FALSE )
			{
				CopyMemory ( &container->skillArrayData[container->iCount], &arrayData, sizeof ( SkillArrayData ) );
				container->skillArrayData[container->iCount].dwSkillArrayAddressOrIndex |= 0x20000000; //2 = float array;
				container->iCount++;

				container->iLength = sizeof ( Packet ) + sizeof ( int ) + sizeof ( int ) + ( container->iCount * sizeof ( SkillArrayData ) );
			}

			if ( mapSkillData.find ( dwAddressOrIndex ) == mapSkillData.end () )
			{
				//add new
				mapSkillData.insert ( { dwAddressOrIndex, arrayData } );
			}
			else
			{
				//replace existing
				CopyMemory ( &mapSkillData[dwAddressOrIndex].fSkillValues, &values, 10 * sizeof ( float ) );
			}

			float scale = 1.0f;
			int classNum = -1;
			if ( SQLSKILL->GetSkillChainScaleValue( eSkillID, columnName, &classNum, &scale) )
			{
				//only score non 1.0f values
				if ( scale != 1.0f )
				{
					auto it = mapSkillChainScale.find( dwAddressOrIndex );
					if ( it != mapSkillChainScale.end() )
					{
						//update
						it->second = std::tuple<int, float>(classNum, scale);
					}
					else
					{
						//add new
						mapSkillChainScale.insert( { dwAddressOrIndex, std::tuple<int, float>( classNum, scale ) } );
					}
				}
			}
		}
	}
	else
	{
		WARN( "Too much Skill data for class %d", container->iCharacterClass );
		//Note - if this happens, need to split up the data two into packets
	}
}

int __cdecl GetBlessCastleBuff( User * pcUser )
{
	UserData * pcUserData = pcUser->pcUserData;

	if ( pcUserData && pcUserData->iMapID != MAPID_Bellatra)
	{
		if ( pcUserData->dwBlessSkillTime >= TICKCOUNT && pcUserData->dwBlessSkill_Code == 0x0A1 )
		{
			return 10;
		}
		else if ( pcUserData->sCharacterData.iClanID )
		{
			if ( pcUserData->sCharacterData.iClanID == BLESSCASTLESERVER->GetClanTop( 2 ) )
			{
				return 6;
			}

			else if ( pcUserData->sCharacterData.iClanID == BLESSCASTLESERVER->GetClanTop( 3 ) )
			{
				return 3;
			}
		}
	}
	return 0;
}

int GetBellatraAttackPowerBoost ( User * pcUser )
{
	UserData * pcUserData = pcUser->pcUserData;

	//This damage boost is for inside Bellatra
	if (pcUserData && pcUser && pcUserData->iMapID == MAPID_Bellatra)
	{
		int buffPercent = 0;

		if (pcUser->iBellatraRoomInitialPlayerCount == 1)
			buffPercent = 50;

		else if (pcUser->iBellatraRoomInitialPlayerCount == 2)
			buffPercent = 30;

		else if (pcUser->iBellatraRoomInitialPlayerCount == 3)
			buffPercent = 15;

		if (pcUserData->sCharacterData.iLevel >= 80)
		{
			//level 80 will receive a 20% extra buff, where level 90 will receive 10% and so on
			//level 80 solo will receive 70% dmg boost (combined)
			//and a level 120 player will only receive 30% buff (combined)
			//See the function calling this function. it's adding.
			buffPercent += (100 - pcUserData->sCharacterData.iLevel);
		}

		if (buffPercent > 0)
		{
			return buffPercent;
		}
		else
		{
			return 0; //no bonus damage
		}
	}

	return 0;
}

// 0x00555BA0
BOOL __cdecl BuffReceive( User * pcUser, PacketSkillPartyData * pPacket )
{
	if ( pPacket->iCount <= 0 )
		return TRUE;

	if ( pcUser && pcUser->IsValid () )
	{
		//Packet size vs. size buff?
		/*Depracated if( pPacket->iLength >= 0x28 )
		{
			int iSizeBuff = (pPacket->iLength - 0x24);
			pPacket->iCount *= 4;

			//Packet > buff ? Error
			if ( (DWORD)pPacket->iCount > (DWORD)iSizeBuff )
				return FALSE;
		}
		else
			return FALSE;*/

			//Add Buff Skill to User
		int iClass = pcUser->pcUserData->sCharacterData.iClass;
		for ( int i = 0; i < pPacket->iCount; i++ )
		{
			UserData * pcUserDataTarget = USERDATABYID ( pPacket->iaID[i] );
			User * pcUserTarget = USERDATATOUSER ( pcUserDataTarget );
			if ( pcUserTarget && pcUserTarget->IsValid() )
			{
				if ( iClass == CHARACTERCLASS_Knight )
				{
					//if ( KNIGHTSKILL->AddHolyValor ( pcUserTarget, pPacket ) == TRUE )
					//{
					//	if ( pcUserDataTarget != pcUser->pcUserData )
					//		SENDPACKET ( pcUserTarget, pPacket );
					//}
					//
					//if ( KNIGHTSKILL->AddHolyConviction ( pcUserTarget, pPacket ) == TRUE )
					//{
					//	if ( pcUserDataTarget != pcUser->pcUserData )
					//		SENDPACKET ( pcUserTarget, pPacket );
					//}
				}
			}
		}
	}

	return TRUE;
}

void __cdecl SendAttackRatting( UserData * pcUserData, UINT iAttackRating, UINT idEnemy )
{
	User * pcUser = USERDATATOUSER ( pcUserData );

	if ( pcUser == NULL || pcUser->IsValid () == FALSE )
		return;

	if ( pcUserData )
	{
		PacketAttackRating sPacket;
		sPacket.iLength = sizeof( PacketAttackRating );
		sPacket.iHeader = PKTHDR_AttackRatingSend;
		sPacket.iEnemyID = idEnemy;
		sPacket.iAttackRating = iAttackRating;

		SENDPACKET( pcUser, &sPacket );
	}
}

/// <summary>
/// Add buffs to base weapon damage before skills damage percentage get applied
/// Important Note - flat based buffs should be added AT the end (GetPostBuffedDamage)
/// otherwise they multiply during percentage based damage increase
/// </summary>
int GetFinalWeaponDamageFromBuffs( User * pcUser, BOOL bIsAreaSkill, EItemType eItemType, UxxxTarget * p_Target, int iBaseAttackPowerMin, int iBaseAttackPowerMax)
{
	UserData * pcUserData = pcUser->pcUserData;

	CHATSERVER->SendDebugChat( pcUser, CHATCOLOR_Error, " > Attack Power Base [%d - %d]", iBaseAttackPowerMin, iBaseAttackPowerMax );

	DEBUG ( "DAMAGE BUFF: Starting Attack Power [%d - %d]", iBaseAttackPowerMin, iBaseAttackPowerMax );

	int iValue;
	int iDamageMin = iBaseAttackPowerMin;
	int iDamageMax = iBaseAttackPowerMax;

	iValue = SHAMANSKILL->GetAdventMigalAttackPowerBoost ( pcUser );
	if ( iValue )
	{
		iDamageMin += ( iBaseAttackPowerMin * iValue ) / 100;
		iDamageMax += ( iBaseAttackPowerMax * iValue ) / 100;

		DEBUG ( "DAMAGE POST BUFF: Advent Migal [+%d%%] = [%d - %d]", iDamageMin, iDamageMax );
	}

	iValue = GetBlessCastleBuff( pcUser );
	if ( iValue )
	{
		iDamageMin += ( iBaseAttackPowerMin * iValue ) / 100;
		iDamageMax += ( iBaseAttackPowerMax * iValue ) / 100;
		DEBUG ( "DAMAGE BUFF: Bless Castle Buff [+%d%%] = [%d - %d]", iValue, iDamageMin, iDamageMax );
	}

	iValue = MECHANICIANSKILL->GetMaximizeAttackPowerBoost( pcUser );
	if ( iValue )
	{
		iDamageMin += ( iBaseAttackPowerMin * iValue ) / 100;
		iDamageMax += ( iBaseAttackPowerMax * iValue ) / 100;

		DEBUG ( "DAMAGE BUFF: Maximize [+%d%%] = [%d - %d]", iValue, iDamageMin, iDamageMax );
	}

	iValue = MECHANICIANSKILL->GetAutomationAttackPowerBoost ( pcUser, p_Target, 1, eItemType );
	if ( iValue )
	{
		iDamageMin += ( iBaseAttackPowerMin * iValue ) / 100;
		iDamageMax += ( iBaseAttackPowerMax * iValue ) / 100;

		DEBUG ( "DAMAGE BUFF: Automation [+%d%%] = [%d - %d]", iValue, iDamageMin, iDamageMax );
	}


	iValue = FIGHTERSKILL->GetBerserkerAttackPowerBoost( pcUser );
	if ( iValue )
	{
		iDamageMin += ( iBaseAttackPowerMin * iValue ) / 100;
		iDamageMax += ( iBaseAttackPowerMax * iValue ) / 100;

		DEBUG ( "DAMAGEBUFF: Berseker [+%d%%] = [%d - %d]", iValue, iDamageMin, iDamageMax );
	}

	iValue = PRIESTESSSKILL->GetSummonMuspellAttackPowerBoost( pcUser);
	if ( iValue )
	{
		iDamageMin += ( iBaseAttackPowerMin * iValue ) / 100;
		iDamageMax += ( iBaseAttackPowerMax * iValue ) / 100;

		DEBUG ( "DAMAGEBUFF : Muspell Summon [+%d%%] = [%d - %d]", iValue, iDamageMin, iDamageMax );
	}

	iValue = MAGICIANSKILL->GetSpiritElementalAttackPowerBoost ( pcUser, 1 );
	if ( iValue )
	{
		iDamageMin += ( iBaseAttackPowerMin * iValue ) / 100;
		iDamageMax += ( iBaseAttackPowerMax * iValue ) / 100;

		DEBUG ( "DAMAGE BUFF: Spirit Elemental [+%d%%] = [%d - %d]", iValue, iDamageMin, iDamageMax );
	}

	iValue = ARCHERSKILL->GetForceOfNatureAttackPowerBoost ( pcUser );
	if ( iValue )
	{
		iDamageMin += ( iBaseAttackPowerMin * iValue ) / 100;
		iDamageMax += ( iBaseAttackPowerMax * iValue ) / 100;

		DEBUG ( "DAMAGE BUFF: Force Of Nature [+%d%%] = [%d - %d]", iValue, iDamageMin, iDamageMax );
	}

	iValue = KNIGHTSKILL->GetGodsBlessingAttackPowerBoost ( pcUser );
	if ( iValue )
	{
		iDamageMin += ( iBaseAttackPowerMin * iValue ) / 100;
		iDamageMax += ( iBaseAttackPowerMax * iValue ) / 100;

		DEBUG ( "DAMAGE BUFF: God's Blessing [+%d%%] = [%d - %d]", iValue, iDamageMin, iDamageMax );
	}

	//max only
	iValue = ATALANTASKILL->GetTriumphofValhallaAttackPowerBoost( pcUser );
	if ( iValue )
	{
		iDamageMax += (iValue * iDamageMax) / 100;
		DEBUG ( "DAMAGE BUFF: Triumph of Valhalla [+%d, +%d%%] = [%d - %d]", 0, iValue, iDamageMin, iDamageMax );
	}

	iValue = SHAMANSKILL->GetChasingHuntAtkPowBoost( pcUser );
	if ( iValue )
	{
		iDamageMin += ( iBaseAttackPowerMin * iValue ) / 100;
		iDamageMax += ( iBaseAttackPowerMax * iValue ) / 100;

		DEBUG ( "DAMAGE BUFF: Chasing Hunt [+%d%%] = [%d - %d]", iValue, iDamageMin, iDamageMax );
	}

	iValue = GetBellatraAttackPowerBoost ( pcUser );
	if ( iValue )
	{
		iDamageMin += ( iBaseAttackPowerMin * iValue ) / 100;
		iDamageMax += ( iBaseAttackPowerMax * iValue ) / 100;

		DEBUG ( "DAMAGE BUFF: Bellatra [+%d%%] = [%d - %d]", iValue, iDamageMin, iDamageMax );
	}

	//note - iForceOrbDamageExtraPercentage is done here
	// as we want to take the base attack power when doing computations
	// the flat addition is added at the end.
	iValue = pcUser->iForceOrbDamageExtraPercentage;
	if ( iValue > 0 )
	{
		iDamageMin += ( iBaseAttackPowerMin * iValue ) / 100;
		iDamageMax += ( iBaseAttackPowerMax * iValue ) / 100;

		DEBUG ( "DAMAGE BUFF: Add Force Orb Percent [+%d%%] = [%d - %d]", iValue, iDamageMin, iDamageMax );
	}


	CompareServerAttackPowerWithClientVersion( pcUser, iDamageMin, iDamageMax );

	iValue = Dice::RandomI( iDamageMin, iDamageMax );

	DEBUG ( "DAMAGE BUFF: Final Average Attack Power [%d - %d] = %d", iDamageMin, iDamageMax, iValue );

	CHATSERVER->SendDebugChat( pcUser, CHATCOLOR_Error, "> Final Attack Power [%d - %d] = %d", iDamageMin, iDamageMax, iValue );

	return iValue;
}

/**
Triggered on attacks that inflicts damage on one target
*/
int __cdecl RecvSkillSingleTarget( UserData * pcUserData, UxxxTarget * p_Target, int * piAttackPower, PacketSingleTargetSkillDataEx * psPacketEx )
{
	User * pcUser = USERDATATOUSER ( pcUserData );

	if ( pcUser == NULL || pcUser->IsValid () == FALSE )
		return FALSE;

	if ( psPacketEx->eAttackState != EAttackState::State_0_Normal )
	{
		WARN( "RecvSkillSingleTarget - Skill's attack state is not 0: %d", psPacketEx->eAttackState );
	}

	if ( psPacketEx->iAttackProperty != EAttackProperty::None &&
		 psPacketEx->iAttackProperty != EAttackProperty::Undefined)
	{
		WARN( "RecvSkillSingleTarget - Skill's attack state is not 15 (None): %d", psPacketEx->eAttackState );
	}
	psPacketEx->eAttackState = EAttackState::State_0_Normal;
    psPacketEx->iAttackProperty = EAttackProperty::None;
    psPacketEx->sCriticalChance = pcUserData->sCharacterData.iCritical;

	pcUserData->sCharacterData.iAttackRating = psPacketEx->iAttackRating;

	int attackPower = *piAttackPower;
	attackPower = GetFinalWeaponDamageFromBuffs ( pcUser, FALSE, (EItemType)( psPacketEx->dwWeaponCode & 0xFFFF0000 ), p_Target, psPacketEx->AttackPowerMin, psPacketEx->AttackPowerMax );

	int iResult = 0;
	int iClass = pcUserData->sCharacterData.iClass;

	//clear
	psPacketEx->iaParametersEx[0] = 0;
	psPacketEx->iaParametersEx[1] = 0;

	UINT uSkillID = psPacketEx->iSkillCode & 0xFF;
	int iLevelSkill = ( psPacketEx->iSkillCode >> 8 ) & 0xF;
	UINT uUnkV = ( psPacketEx->iSkillCode >> 12 ) & 0x0F;

	if ( iClass == 1 ) // FS
	{
		iResult = FIGHTERSKILL->RecvSkillSingleTarget( pcUser, p_Target, &attackPower, psPacketEx );
	}
	else if ( iClass == 2 ) // MS
	{
		iResult = MECHANICIANSKILL->RecvSkillSingleTarget( pcUser, p_Target, &attackPower, psPacketEx );
	}
	else if ( iClass == 3 ) // AS
	{
		iResult = ARCHERSKILL->RecvSkillSingleTarget( pcUser, p_Target, &attackPower, psPacketEx );
	}
	else if ( iClass == 4 ) // PS
	{
		iResult = PIKEMANSKILL->RecvSkillSingleTarget( pcUser, p_Target, &attackPower, psPacketEx );
	}
	else if ( iClass == 5 ) // ATA
	{
		iResult = ATALANTASKILL->RecvSkillSingleTarget( pcUser, p_Target, &attackPower, psPacketEx );
	}
	else if ( iClass == 6 ) // KS
	{
		iResult = KNIGHTSKILL->RecvSkillSingleTarget( pcUser, p_Target, &attackPower, psPacketEx );
	}
	else if ( iClass == 7 ) // MGS
	{
		iResult = MAGICIANSKILL->RecvSkillSingleTarget( pcUser, p_Target, &attackPower, psPacketEx );
	}
	else if ( iClass == 8 ) // PRS
	{
		iResult = PRIESTESSSKILL->RecvSkillSingleTarget( pcUser, p_Target, &attackPower, psPacketEx );
	}
	else if ( iClass == 9 ) // ASS
	{
		iResult = ASSASSINSKILL->RecvSkillSingleTarget( pcUser, p_Target, &attackPower, psPacketEx );
	}
	else if ( iClass == 10 ) // SHA
	{
		iResult = SHAMANSKILL->RecvSkillSingleTarget ( pcUser, p_Target, &attackPower, psPacketEx );
	}

	//add flat force orb damage here
	if ( pcUser->iForceOrbDamage > 0 )
	{
		attackPower += pcUser->iForceOrbDamage;
	}

	*piAttackPower = attackPower;

	return iResult;
}


/// <summary>
/// Leaked code: int rsRecvProcessSkill(rsPLAYINFO *lpPlayInfo, smTRANS_COMMAND *lpTransCommand)
/// From smTRANSCODE_PROCESS_SKILL2 command
///
/// </summary>
/// <param name="pcUserData"></param>
/// <param name="packet"></param>
/// <returns></returns>
int __cdecl RecvBuffSkill( UserData * pcUserData, PacketSkillCast * packet )
{
	User * pcUser = USERDATATOUSER ( pcUserData );

	if ( pcUser == NULL || pcUser->IsValid () == FALSE )
		return FALSE;

	int iClass = pcUserData->sCharacterData.iClass;
	if ( iClass == 1 ) // FS
	{
		return FIGHTERSKILL->RecvBuffSkill( pcUser, packet );
	}
	else if ( iClass == 2 ) // MS
	{
		return MECHANICIANSKILL->RecvBuffSkill( pcUser, packet );
	}
	else if ( iClass == 3 ) // AS
	{
		return ARCHERSKILL->RecvBuffSkill( pcUser, packet );
	}
	else if ( iClass == 4 ) // PS
	{
		return PIKEMANSKILL->RecvBuffSkill( pcUser, packet );
	}
	else if ( iClass == 5 ) // ATA
	{
		return ATALANTASKILL->RecvBuffSkill( pcUser, packet );
	}
	else if ( iClass == 6 ) // KS
	{
		return KNIGHTSKILL->RecvBuffSkill( pcUser, packet );
	}
	else if ( iClass == 7 ) // MG
	{
		return MAGICIANSKILL->RecvBuffSkill( pcUser, packet );
	}
	else if ( iClass == 8 ) // PRS
	{
		return PRIESTESSSKILL->RecvBuffSkill( pcUser, packet );
	}
	else if ( iClass == 9 ) // ASS
	{
		return ASSASSINSKILL->RecvBuffSkill( pcUser, packet );
	}
	else if ( iClass == 10 ) // ASS
	{
		return SHAMANSKILL->RecvBuffSkill( pcUser, packet );
	}
	return 0;
}

/**
Triggered on attacks that does damage to one or more targets
Called from SkillRangeHandler
*/
int __cdecl RecvSkillAreaTarget( UserData * pcUserData, UnitData * pcUnitData,  PacketSkillDamageRange * psPacketRange, PacketMultiTargetSkillData * psPacketRangeEx )
{
	User * pcUser = USERDATATOUSER ( pcUserData );

	if ( pcUser == NULL || pcUser->IsValid () == FALSE )
		return FALSE;


	if ( psPacketRange->eAttackState != EAttackState::State_0_Normal )
	{
		WARN( "RecvSkillAreaTarget - Skill's attack state is not 0: %d", psPacketRange->eAttackState );
	}

	if ( psPacketRange->iAttackProperty != EAttackProperty::None &&
		 psPacketRange->iAttackProperty != EAttackProperty::Undefined)
	{
		WARN( "RecvSkillAreaTarget - Skill's attack state is not 15 (None): %d", psPacketRange->eAttackState );
	}

	psPacketRange->eAttackState = EAttackState::State_0_Normal;
	psPacketRange->iAttackProperty = EAttackProperty::None;

	// Check for CTRL key
	if ( !MAPSERVER->IsPVPMap( pcUserData->iMapID ) )
	{
//		if ( USERDATABYID( psPacketRangeEx->dwChkSum ) )
//			return 0;
	}

	DWORD dwSkillCode = psPacketRangeEx->iSkillID & 0xFF;
	int iSkillLevel1Based = ( ( psPacketRangeEx->iSkillID >> 8 ) & 0xF );
	int iSkillParam = ( psPacketRangeEx->iSkillID >> 12 ) & 0xF;

    /// todo iron/hardcpp should we send a invalid target ?
    UxxxTarget l_Target(nullptr, pcUnitData);

	psPacketRange->iCount |= 0xFF000000; // AoE
	psPacketRange->iDamage = GetFinalWeaponDamageFromBuffs ( pcUser, FALSE, (EItemType)( psPacketRangeEx->dwWeaponCode & 0xFFFF0000 ), &l_Target, psPacketRangeEx->sAttackPower.sMin, psPacketRangeEx->sAttackPower.sMax );

	int iClass = pcUserData->sCharacterData.iClass;


	BOOL bResult = FALSE;
	if ( iClass == 1 ) // FS
	{
		bResult = FIGHTERSKILL->RecvSkillAreaTarget( pcUser, &l_Target, psPacketRange, psPacketRangeEx );
	}
	else if ( iClass == 2 ) // MS
	{
		bResult = MECHANICIANSKILL->RecvSkillAreaTarget( pcUser, &l_Target, psPacketRange, psPacketRangeEx );
	}
	else if ( iClass == 3 ) // AS
	{
		bResult = ARCHERSKILL->RecvSkillAreaTarget( pcUser, &l_Target, psPacketRange, psPacketRangeEx );
	}
	else if ( iClass == 4 ) // PS
	{
		bResult = PIKEMANSKILL->RecvSkillAreaTarget( pcUser, &l_Target, psPacketRange, psPacketRangeEx );
	}
	else if ( iClass == 5 ) // ATA
	{
		bResult = ATALANTASKILL->RecvSkillAreaTarget( pcUser, &l_Target, psPacketRange, psPacketRangeEx );
	}
	else if ( iClass == 6 ) // KS
	{
		bResult = KNIGHTSKILL->RecvSkillAreaTarget( pcUser, &l_Target, psPacketRange, psPacketRangeEx );
	}
	else if ( iClass == 7 ) // MGS
	{
		bResult = MAGICIANSKILL->RecvSkillAreaTarget( pcUser, &l_Target, psPacketRange, psPacketRangeEx );
	}
	else if ( iClass == 8 ) // PRS
	{
		bResult = PRIESTESSSKILL->RecvSkillAreaTarget( pcUser, &l_Target, psPacketRange, psPacketRangeEx );
	}
	else if ( iClass == 9 ) // ASS
	{
		bResult = ASSASSINSKILL->RecvSkillAreaTarget( pcUser, &l_Target, psPacketRange, psPacketRangeEx );
	}
	else if ( iClass == 10 ) // SHA
	{
		bResult = SHAMANSKILL->RecvSkillAreaTarget( pcUser, &l_Target, psPacketRange, psPacketRangeEx );
	}

	//add flat force orb damage here
	if ( pcUser->iForceOrbDamage > 0 )
	{
		psPacketRange->iDamage += pcUser->iForceOrbDamage;
	}

	DWORD dwAnd;
	dwAnd = psPacketRange->iCount & 0xFF000000;
	if ( dwAnd == 0xFF000000 )
		psPacketRange->iCount = 256;
	else
		psPacketRange->iCount &= 0x000000FF;

	return bResult;
}

void CompareServerAttackPowerWithClientVersion (User * pcUser, int iAttackPowerMin, int iAttackPowerMax )
{
	if ( iAttackPowerMin == 0 && iAttackPowerMax == 0)
		return;

	if ( pcUser->iForceOrbDamage > 0 )
	{
		iAttackPowerMin += pcUser->iForceOrbDamage;
		iAttackPowerMax += pcUser->iForceOrbDamage;
	}

	//if there is a mismatch, it's likely because
	//some values are being added to the final attack power (base) in assembly
	//OR that we forgot to add the buff to the attack power
	if ( iAttackPowerMin != pcUser->iClientAttackPowerMin || iAttackPowerMax != pcUser->iClientAttackPowerMax )
	{
#ifdef DEV_MODE
		//todo - figure out why there is a mismatch when:
		// - a force orb is active (the calc is wrong on client side)
		// -
		WARN( "Attack power mismatch. %d - %d (server) is not equal to %d - %d (client) (class = %d)", iAttackPowerMin, iAttackPowerMax, pcUser->iClientAttackPowerMin, pcUser->iClientAttackPowerMax, pcUser->pcUserData->sCharacterData.iClass );
#endif
	}

}

void __cdecl SendSkillEffect( UserData * pcUserData, UserData * TargetPlayer, UINT skCode )
{
	int iClass = pcUserData->sCharacterData.iClass;
	if ( iClass == 8 )
	{
		if ( skCode == PriestessSkill::GlacialSpike )
		{
			int iSKLevel = 0;// pPlayer->Memory->LevelSkill14;
			SendPacketSkillEffect( pcUserData, TargetPlayer, skCode, iSKLevel );
		}
	}
}