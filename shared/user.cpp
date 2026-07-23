#include "stdafx.h"
#include "Coin.h"
#include "user.h"

User::User()
{
}

User::~User()
{
	SAFE_DELETE( psParty );
	SAFE_DELETE( mFinishedQuestData );
}

/// <summary>
/// Return the name of the player's character
/// </summary>
char * User::GetName()
{
	return CHARACTERSERVER->GetCharacterName( this );
}

void User::UpdateQuestWithValues( DWORD dwQuestID, EQuestStatus eStatus, DWORD dwValue1, DWORD dwValue2, DWORD dwValue3, DWORD dwValue4, DWORD dwValue5 )
{
}

void User::UpdateQuest( DWORD dwQuestID, EQuestStatus eStatus )
{

}


EQuestUserStatus User::GetQuestStatus( int iQuestID )
{
	if ( iQuestID < 0 )
	{
		WARN( "GetQuestStatus iQuestID %d is less than 0!", iQuestID ); //Use pcUser->saPacketQuestData for this
		return EQuestUserStatus::Error;
	}

	auto const it = mFinishedQuestData->find( iQuestID );

	if ( it != mFinishedQuestData->end() )
	{
		if ( SystemTimeToDWORD( &it->second.sDateFinished ) == 2764800 )  //2764800 means EndDate is 'NULL' in CharacterQuest db
		{
			WARN( "mFinishedQuestData should not contain quests that are in progress" ); //Use pcUser->saPacketQuestData for this
			return EQuestUserStatus::InProgress;
		}

		if ( QUESTSERVER->IsQuestExpired( it->second.eQuestType, it->second.sDateFinished ) )
			return EQuestUserStatus::Expired;

		return EQuestUserStatus::Done;
	}

	auto lsActiveQuest = QUESTSERVER->GetActiveQuestData( this, iQuestID );
	if ( lsActiveQuest )
	{
		//quest is finished?
		if ( QUESTSERVER->IsCompletedQuestRequirements( this, lsActiveQuest ) )
			return EQuestUserStatus::Completed;
		else
			return EQuestUserStatus::InProgress;
	}

	return EQuestUserStatus::NotStarted;
}

BOOL User::RestartQuest( DWORD dwQuestID )
{
	BOOL bRet = FALSE;
	return bRet;
}

char * User::GetCharacterName ()
{
	if ( pcUserData == NULL )
		return "";

	if ( !GAME_SERVER )
		return pcUserData->szCharacterName;

	return pcUserData->sCharacterData.szName;
}

BOOL User::CanSee( User * pcOtherUser )
{
	//Other User cloaked? Then cannot see other user
	if ( pcOtherUser->pcUserData->bHideMode )
		return FALSE;

	//Not in one of the two Maps that the User can see?
	EMapID eOtherMapID = pcOtherUser->GetMapID();

	//allow users from nav to see other uesrs in bellatra
	if (eOtherMapID == MAPID_Bellatra && eMapID == MAPID_NaviskoTown)
		return TRUE;


	if ( (eOtherMapID != eMapID) &&
		(eOtherMapID != eSecondMapID) )
		return FALSE;

	//Other User in Quest Arena?
	if ( (pcOtherUser->GetMapID() == MAPID_T5QuestArena) || (pcOtherUser->GetMapID() == MAPID_QuestArena) )
	{
		//Not a GM? Then cannot see other User
		if ( GetGameLevel() == GAMELEVEL_None )
			return FALSE;
	}

	return TRUE;
}

BOOL User::CanSee( Unit * pcOtherUnit )
{
	//In Quest Arena?
	if ( (pcUserData->iMapID == MAPID_T5QuestArena) || (pcUserData->iMapID == MAPID_QuestArena) )
	{
		if ( pcOtherUnit->pcUnitData->bTestMapUnit && GetGameLevel () != GAMELEVEL_Four )
			return FALSE;

		//Monster is not for this User?
		if ( pcOtherUnit->pcUnitData->lExclusiveTargetID != pcUserData->iID )
		{
			//Not a GM?
			if ( GetGameLevel() == GAMELEVEL_None )
			{
				//Not a Pet? Then probably someone else's Monster
				if ( pcOtherUnit->pcUnitData->sCharacterData.iMonsterType != MONSTERTYPE_Summon )
					return FALSE;

				//Not your Pet? Then probably someone else's Pet
				if ( pcOtherUnit->pcUnitData->pcOwner != pcUserData )
					return FALSE;
			}
		}
	}

	//Not in one of the two Maps that the User can see?
	EMapID eOtherMapID = pcOtherUnit->GetMapID();
	if ( (eOtherMapID != eMapID) &&
		(eOtherMapID != eSecondMapID) )
		return FALSE;

	//Is NPC?
	if ( pcOtherUnit->pcUnitData->sCharacterData.iType == CHARACTERTYPE_NPC )
	{
		//Is Hidden NPC?
		if ( (pcOtherUnit->pcUnitData->sCharacterData.iLevel & 2) )
		{
			//User not a GM? then NPC stays hidden
			if ( GetGameLevel() == GAMELEVEL_None )
				return FALSE;
		}
	}

	return TRUE;
}


UserData::UserData()
{
}

int UserData::GetCurrentHealth()
{
	return this->sCharacterData.sHP.sCur;
}

int UserData::GetMaxHealth()
{
	return this->sCharacterData.sHP.sMax;
}

BOOL UserData::IsDead()
{
	if ( GetCurrentHealth() <= 0 )
		return TRUE;

	return FALSE;
}

void User::UnInit()
{
	if ( mFinishedQuestData )
		mFinishedQuestData->clear();

	SAFE_DELETE( psParty );
	SAFE_DELETE( mFinishedQuestData );
	SAFE_DELETE( pcUserDataSpyoner );

	LastDroppedItemsThrowMapHack.clear();
	PlayerItemPutHashs.clear();

	ZeroMemory(&pData, sizeof(PacketPlayDataEx));

	ZeroMemory(&sPremiumData, sizeof(PacketItemPremiumData));

	ZeroMemory(&LastDroppedItem, sizeof(DropItemData));

	ZeroMemory(&sTempItem, sizeof(PacketItemData));

	ZeroMemory(&sWarehouseItems, sizeof(DropItemData) * _countof(sWarehouseItems));
	ZeroMemory(&sWarehouseItems_Seasonal, sizeof(DropItemData) * _countof( sWarehouseItems_Seasonal ));
	ZeroMemory(&sLastInventoryItems, sizeof(DropItemData) * _countof( sLastInventoryItems ));

	ZeroMemory(&saTradeItems, sizeof(DropItemData) * _countof(saTradeItems));

	ZeroMemory(&sItemBox, sizeof(ItemBoxSlot));

	ZeroMemory(&sCharacterDataEx, sizeof(CharacterDataEx));

	ZeroMemory(&saPacketQuestData, sizeof(PacketQuestUpdateData) * MAX_QUESTPACKETDATA);

	ZeroMemory(&sSettingsData, sizeof(PacketSettingsData));

	ZeroMemory ( &sSkillBuffContainer, sizeof ( PacketSkillBuffStatusContainer ) );
	ZeroMemory ( &sDamageInfoContainer, sizeof ( PacketDebugDamageInfoContainer ) );
	ZeroMemory ( &sSkillBuffStatus, sizeof ( PacketSkillBuffStatus ) );

	ZeroMemory(&sUnitStatusContainer, sizeof(PacketUnitStatusContainer));

	ZeroMemory(&sUnitStatusContainerM, sizeof(PacketUnitStatusContainer));

	for (int i = 0; i < 5; i++)
		ZeroMemory(&baUnitStatusNew[i], 0x2000);

	ZeroMemory( uaUnitSkillHash, sizeof( UINT ) * 1024 );

	bInUse = FALSE;
	PlayerAuthed = false;
}

void User::Init()
{
	PlayerAuthed = false;
	WasInTwoSideTrade = false;
	TradeWindowOpen = false;
	GetItemGoldLastTime = 0;
	GetItemGoldTotal = 0;
	LastInventoryHash = 0xFFFFFFF;
	LastGoldHash = 0xFFFFFFF;
	m_GoldThrowHistory.Count = 0;
	m_GoldThrowHistory.LastTime = 0;

	mFinishedQuestData = new std::map<int, FinishedQuestData>();

	bParty = FALSE;
	psParty = new PartyInfo();
	ZeroMemory( psParty, sizeof( PartyInfo ) );

	CALLT(0x00567530, uIndex);
	if (pcUserData->psCharacterFile)
		pcUserData->psCharacterFile->iLength = 0;

	pcUserDataSpyoner = NULL;

	bNetServer = FALSE;
	bClosingSocket = FALSE;

	bDisconnectNet = FALSE;
	dwIP = 0;

	iAccountID = -1;
	iCharacterID = -1;

	uSerialHD = 0;

	eMapID = MAPID_Invalid;
	eSecondMapID = MAPID_Invalid;

	b16 = FALSE;
	b32 = FALSE;
	b64 = FALSE;

	iSecondTick = 0;
	bOneMin = FALSE;
	bTenSeconds = FALSE;


	bReceivedExp = FALSE;

	iClientAttackPowerMin = 0;
	iClientAttackPowerMax = 0;

	iEXPPotion = 0;

	iPVPKills = 0;
	iPVPDeaths = 0;
	iPVPKillStreak = 0;
	bResetKillStreak = FALSE;
	iKillSafe = 0;
	iLastKillSafeID = -1;
	dwKillSafeTimeOut = 0;
	dwLastKillIP = 0;

	bInvisibleInMiniMap = 0;

	bLootFilterEnabled = 0;
	iLootFilterFlags = 0;
	sEquipmentMinLevel = 0;
	eArmorEquipped = (EItemID)0;
	eBootsEquipped = (EItemID)0;
	eGauntletsEquipped = (EItemID)0;
	eBraceletEquipped = (EItemID)0;
	eRingRightEquipped = (EItemID)0;
	eRingLeftEquipped = (EItemID)0;
	eOrbEquipped = (EItemID)0;
	eRobeEquipped = (EItemID)0;
	eAmuletEquipped = (EItemID)0;
	bTeasureHuntingMode = FALSE;


	dwTimeSync = 0;
	dwTimeSavePremium = 0;
	uUserLoadedTime = 0;


	iWarehouseError = 0;
	iWarehouseGold = 0;

#if defined (DEV_MODE) && defined (DEBUG_INVENTORY)
	ZeroMemory(&sLastDropItemsTemp, sizeof(DropItemData) * _countof( sLastDropItemsTemp ));
	bLastDropItemsValidTemp = 0;
#endif


	bCharacterExLoaded = FALSE;

	iDefenseRating = 0;
	sBlockChance = 0;
	sEvadeChance = 0;

	szTitle[0] = { 0 };
	sTitleRarity = 0; //0 = no title
	sDialogSkinId = 0;
	sClassRanking = 0;

	bQuestLoaded = FALSE;

	dwChatTradeTime = FALSE;

	bDebugChat = FALSE;
	bDisableWarnAndErrorRelaysToClient = FALSE;

	bMuted = FALSE;

	bInT5QuestArena = FALSE;
	iRoundT5QuestArena = 0;
	dwTimeT5QuestArenaUpdate = 0;
	dwTimeT5QuestArenaSpawnUpdate = 0;

	uQuestT5ID = 0;
	sQuestT5ProgressValue[0] = 0;
	sQuestT5ProgressValue[1] = 0;

	bInGrandFury = FALSE;
	dwGrandFuryTickEnter = 0;

	uConcealEndTime = 0;
	bConcealLevel1Based = 0;

	bAdventMigalLevel1Based = 0;
	uAdventMigalEndTime = 0;

	iChasingHuntAttackPowerBoost = 0;
	uChasingHuntEndTime = 0;

	iHolyMindAbsorb = 0;
	dwHolyMindTime = 0;

	iSummonMuspellDamageBoost = 0;

	iWispAbsorb = 0;
	dwWispTime = 0;

	uLethalSightLevel = 0;
	uLethalSightRange = 0;
	uLethalSightCritical = 0;
	uLethalSightDamageBoost = 0;
	uLethalSightTimeOut = 0;

	uFierceWindLevel = 0;
	dwFierceWindTimeOut = 0;

	uHolyConvictionLevel = 0;
	dwHolyConvictionTimeOut = 0;

	uWizardTranceLevel = 0;
	dwWizardTranceTimeOut = 0;

	uTitleSetTimeOut = 0;

	uEnchantWeaponEndTime = 0;
	bEnchantWeaponLevel1Based = 0;
	uEnchantWeaponNextSwitchTime = 0;
	bEnchantWeaponRotationCounter = 0;
	eEnchantWeaponElement = EElementID::None;
	sEnchantWeaponAtkPowerPercent = 0;
	sEnchantWeaponParameter = 0;
	bEnchantWeaponChance = 0;
	bEnchantWeaponDurationSec = 0;

	uDancingSwordEndTime = 0;
	bDancingSwordLevel = 0;
	eDancingSwordElement = EElementID::None;
	bDancingSwordChainLevelEW = 0;
	bDancingSwordChainLevelDR = 0;

	uBerserkerEndTime = 0;
	sBersekerDamageBoost = 0;
	sBersekerDamageToHPPercent = 0;

	uDemonBaneEndTime = 0;
	bDemonBaneLevel1Based = 0;

	uVenomThornEndTime = 0;
	bVenomLevel1Based = 0;

	uElectricalOverloadEndTime = 0;
	bElectricalOverloadLevel1Based = 0;

	uHallOfValhallaEndTime = 0;
	bHallOfValhallaLevel1Based = 0;

	uTriumphOfValhallaEndTime = 0;
	bTriumphOfValhallaLevel1Based = 0;

	uVirtualLifeEndTime = 0;
	bVirtualLifeLevel1Based = 0;

	uVirtualRegenEndTime = 0;
	bVirtualRegenLevel1Based = 0;

	uAlasEndTime = 0;
	bAlasLevel1Based = 0;

	uHolyValorEndTime = 0;
	bHolyValorLevel1Based = 0;

	uHolyBodyEndTime = 0;
	bHolyBodyLevel1Based = 0;

	uDrasticSpiritEndTime = 0;
	bDrasticSpiritLevel1Based = 0;

	uForceOfNatureEndTime = 0;
	bForceOfNature1Based = 0;

	uInpesEndTime = 0;
	bInpesLevel1Based = 0;

	iTradeCoin = 0;
	iTradeGold = 0;

	iBlessCastleScore = 0;

	uSkillBuffHash = 0;
	bForceSkillBuffUpdate = 0;

	bInvalidateSkillBuffStatus = 0;
	bForceSkillBuffUpdate = 0;

	uMovementStatus = EPlayerMovementStatus::Moving;

	iForceOrbDamage = 0;
	iForceOrbDamageExtraPercentage = 0;
	bForceOrbActive = FALSE;
	eForceOrbItemID = EItemID::ITEMID_None;

	dwIgnoreNpcClickTime = 0;
	uNextClanIdCheckTime = 0;
	iLatestAttackPower = 0;

	uBufferPosition = 0;
	uBufferPositionM = 0;
	uUnitStatusTime = 0;



	bInUse = TRUE;
}

