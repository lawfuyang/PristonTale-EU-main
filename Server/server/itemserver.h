#pragma once
#include "SocketSystemHandler.h"

static const int CRC_ITEMSHOP_UNIQUEID = 0x66827A;


enum AgeStoneList
{
	AgingStone = 0x080B0A00,
	CooperOre = 0x080B0B00,
	MaxAge = 0x080C5200,
};

enum class AgingResultType : int
{
	Destruction			 = -1,	//Break
	Unknown				 = 0,	//Dont use, but does +1
	MinusOne			 = 1,	//-1
	MinusTwo			 = 2,	//-2
	PlusOne				 = 3,	//+1
	PlusTwo				 = 4,	//+2
	MinusOneNoBreakEvent = 5,	//-1
	MinusOneCopperOre	 = 6,	//-1
};

enum class EForceOrbSendType : int
{
	Unknown = 0,
	Load = 1,		//Load force orb - will override
	Sync = 2,		//Sync timers only
	Cancel = 3		//Will cancel force orb
};

enum class ERarityChanceModType : int
{
	None = 0,
	Boss = 1,
	EventMimic = 2,
	EventMimicHighLevel = 3,
	BossHighLevel = 4
};

enum class ERarityBaselineMod : int
{
	None = 90,
	Common = 87,
	Uncommon = 85,
	Rare = 81,
	Epic = 76,
	Legendary = 72
};

struct ItemRarity
{
	EItemRarity								eRarity;
	int										iRarityGroup;
	int										Chance;
};

struct StatModifier
{
	int										iStrength;
	int										iSpirit;
	int										iTalent;
	int										iAgility;
	int										iHealth;
};

struct ItemRarityChanceGroup
{
	int										iRarityGroup;
	int										MinLevel;
	int										MaxLevel;
};

struct ItemRarityChanceMod
{
	float										fModCommon;
	float										fModUncommon;
	float										fModRare;
	float										fModEpic;
	float										fModLegendary;
};

struct ItemSpecMod
{
	int										iSpecClass01;
	int										iSpecClass02;
	int										iSpecClass03;
	int										iSpecClass04;
	int										iSpecClass05;
	int										iSpecClass06;
	int										iSpecClass07;
	int										iSpecClass08;
	int										iSpecClass09;
	int										iSpecClass10;
	char									sItemType[12];
	StatModifier							eStatModifier;
};

struct ItemBaselineMod
{
	int										iClass;
	StatModifier							eStatModifier;
};


#define SOCKETSYSTEMHANDLER						ItemServer::pcSocketSystemHandler

class ItemServer
{

protected:

	static const DWORD								pfnCheckMixItem			= 0x004461E0;	//Item aging certification confirmation.	int CheckMixItem( sITEMINFO *lpItemInfo )
	static const DWORD								pfnGetMixItemForm		= 0x00446170;	//Item aging authentication code.			short GetMixItemForm( sITEMINFO *lpItemInfo )
	static const DWORD								pfnCheckItemForm		= 0x00445FA0;	//Item authentication check
	static const DWORD								pfnDeleteItem			= 0x00550EA0;
	static const DWORD								pfnAddItem				= 0x00568410;
	static const DWORD								pfnIncreaseAgingState	= 0x004BD540;
	static const DWORD								pfnSaveItemData			= 0x005555B0;
	static const DWORD								pfnRecordAgingLog		= 0x00553D70;
	static const DWORD								pfnRecordItemChangeLog	= 0x00554C90;
	static const DWORD								pfnCreateDefItem		= 0x0043C580;
	static const DWORD								pfnCreateItemCount		= 0x00552890;
	static const DWORD								pfnIncreaseAging		= 0x004BD540;
	static const DWORD								pfnSetAgingChecksum		= 0x004461C0;

	typedef											INT( __cdecl * tfnCheckMixItem)(Item * psItem);
	typedef											UINT( __cdecl * tfnGetMixItemForm)(Item* psItem);

	typedef											INT( __cdecl *tfnDeleteItem )(UserData * pcUserData, DWORD dwCode, DWORD dwHead, DWORD dwChkSum);
	typedef											INT( __cdecl *tfnAddItem )(UserData * pcUserData, DWORD dwCode, DWORD dwHead, DWORD dwChkSum);
	typedef											BOOL( __cdecl *tfnCheckItem )(Item * psItem);
	typedef											BOOL( __cdecl *tfnIncreaseAgingState )(Item * psItem);
	typedef											BOOL( __cdecl *tfnSaveItemData )(Item * psItem);
	typedef											void( __cdecl *tfnRecordAgingLog )(UserData * pcUserData, Item * psItemSource, Item * psItemDest );
	typedef											void( __cdecl *tfnRecordItemChangeLog )(UserData * pcUserData, DWORD dwItemCode, DWORD	dwHead, DWORD dwChkSum, DWORD dwNewItemCode, DWORD dwNewHead, DWORD dwNewChkSum, int Flag);
	typedef											void( __cdecl *tfnCreateDefItem )(Item * psItem, DefinitionItem * psDefItem, int iSpeck);
	typedef											void( __cdecl *tfnCreateItemCount )(UserData * pcUserData, DWORD dwItemCode, int iCount);
	typedef											void( __cdecl *tfnIncreaseAging )(Item * psItem);
	typedef											void( __cdecl *tfnSetAgingChecksum )(Item * psItem);

	static std::map<int, int> s_ItemBuyPriceMap;
	static std::map<int, int> s_ItemBuyMaxIntegrityMap;
	static std::map<int, int> s_ItemIdToLevelReqMap;


public:
#ifdef ITEMFIXES
	std::vector<ItemFix*>									FixesList;
#endif
	std::vector<ItemRarity*>								RarityList;
	std::vector<ItemRarityChanceGroup*>						RarityChanceGroupList;
	std::vector<ItemSpecMod*>								ItemSpecModList;
	std::vector<ItemBaselineMod*>							ItemBaselineModList;

	std::map<ERarityChanceModType, ItemRarityChanceMod>		mRarityChanceMod;

	void StoreItemMiscData( DWORD p_ID, int p_Price, int p_MaxIntegrity, int p_LevelReq );
	BOOL IsItemAbleToHaveRarity( Item * psItem );
	BOOL IsBossOrSpecialItem( Item * psItem );
	BOOL IsBossOrSpecialItem( DWORD p_ID );
	int GetItemLevelRequirement( DWORD p_ID );
	int GetItemBuyPrice( DWORD p_ID, bool p_HasSpec, int iItemRarityNum );
	int GetItemAgingPrice( struct Item * pcItem, int p_CurrentAgeLevel );
	int GetItemRepairPrice( DWORD p_ID, bool p_HasSpec, SHORT p_Durability, SHORT p_DurabilityMax, int iItemRarityNum );
	int GetItemRepairPrice( DWORD p_ID, DWORD p_Price, bool p_HasSpec, SHORT p_Durability, SHORT p_DurabilityMax );
	int GetItemSellPrice( DWORD p_ID, bool p_HasSpec, SHORT p_Durability, SHORT p_DurabilityMax, int iItemRarityNum );
	int GetItemSellPrice( DWORD p_ID, DWORD p_Price, bool p_HasSpec, SHORT p_Durability, SHORT p_DurabilityMax );

private:

	DefinitionItem								OldItemDefinition[ITEM_SERVER_MAX];

	// Startup cache: save/load OldItemDefinition + main ItemList from disk
	bool										TryLoadItemsFromCache();
	void										SaveItemsToCache();

	PremiumItem									* GetPremiumInfo( DWORD dwCode );

	void										SQLUpdateHeadModel ( SQLConnection * pcDB, User * pcUser, char * headModel, int iSqlCharacterId );
	void										SQLUpdateItemTimer ( SQLConnection * pcDB, User * pcUser, int iType, DWORD dwTimeLeft );
	void										SQLDeleteItemTimer ( SQLConnection * pcDB, User * pcUser, int iItemID );
	void										SQLDeleteItemTimerByTimerType ( SQLConnection * pcDB, User * pcUser, enum EItemTimerType eItemTimerType );
	void										SQLAddItemTimer ( SQLConnection * pcDB, int iCharID, char * pszCharName, int iType, EItemID iItemID, DWORD dwTimeTotal );

	EItemRarity									ChooseItemRarity(int iLevel, ERarityChanceModType eModType);
	void										SetRarityBoostBasedOnItemBase(DefinitionItem* itemDef, const EItemRarity& itemRarity);
	void										SetItemFixes ( DefinitionItem * itemDef, EItemRarity itemRarity, BOOL isPerfect = FALSE, int iPrefixID = 0, int iSuffixID = 0 );
	short										GetRandomStatValue ( short iMin, short iMax );
	float										GetRandomStatValue ( float iMin, float iMax );
	void										SetRarityBoostArmors(DefinitionItem* itemDef, const int statsBoostMultiplier, const int itemLevel);
	void										SetRarityBoostBoots(DefinitionItem* itemDef, const int statsBoostMultiplier, const int itemLevel);
	void										SetRarityBoostRobes(DefinitionItem* itemDef, const int statsBoostMultiplier, const int itemLevel);
	void										SetRarityBoostGauntlets(DefinitionItem* itemDef, const int statsBoostMultiplier, const int itemLevel);
	void										SetRarityBoostBracelets(DefinitionItem* itemDef, const int statsBoostMultiplier, const int itemLevel);
	void										SetRarityBoostAmulets(DefinitionItem* itemDef, const int statsBoostMultiplier, const int itemLevel);
	void										SetRarityBoostRings(DefinitionItem* itemDef, const int statsBoostMultiplier, const int itemLevel);
	void										SetRarityBoostShields(DefinitionItem* itemDef, const int statsBoostMultiplier, const int itemLevel);
	void										SetRarityBoostOrbs(DefinitionItem* itemDef, const int statsBoostMultiplier, const int itemLevel);


protected:
	CompressedItem * pTableNewItem = NULL;


public:
	ItemServer();
	virtual ~ItemServer();

	static CSocketSystemHandler					* pcSocketSystemHandler;

	BOOL										CheckQuestOrEventItemAging( const char * pszItemName, int iItemID = 0 );

	void										CompressItemDataInTable ( ItemData * pItem );
	void										CreateItemMemoryTable ( const char * pszTableName, DefinitionItem * pDefItem, BOOL bOutputToItemListDat );
	void										CreateItemMemoryTable ();
	void										UpdateItemRarityChanceGroup ();
	void										GetBaselineMod();
	void										GetItemSpecMod();
	void										UpdateItemRarityMods ();
	void										SetRarityDB (int iRarityGroup, int iRarity, int iChance );
	int											GetRarityChance ( int iRarityGroup , int iRarity);
	void										UpdateItemRarity ();
#ifdef ITEMFIXES
	void										UpdateItemFixes (SQLConnection * pcDB);
#endif
	ItemData *									FindItemPointerTable( const char * szCode );
	ItemData *									FindItemPointerTable( UINT uCode );
	DefinitionItem *							FindItemDefByCode( DWORD dwCode );
	DefinitionItem *							FindOldItemDefByCode( DWORD dwCode );
	DefinitionItem *							FindQuestWeaponDefByCode( const char * pszCode );
	DefinitionItem *							FindItemDefByCode( char * pszCode );
	DefinitionItem *							FindOldItemDefByCode ( char * pszCode );
	DefinitionItem *							FindItemDefByLevelBase( int iItemBase, int iLevel );
	int											FindItemPositionTable( char * pszName );
	BOOL 										FindItemName( UINT uCode, char * szBufName );
	BOOL 										FindItemName( char * pszCode, char * szBufName );
	int											GetItemIDByItemCode( char * pszCode );

	BOOL										IsDropableItem( int iItemID );

	void										RecoverItemFromItemCreateLog ( User * pcUser, int iDatabaseId );

	int											SpecStoneToCharacterSpec( int iItemID );

	BOOL										IsDeluxeStone( int iItemID );

	static CharacterClassData					* GetItemSpecClassJob( int iClass, DefinitionItem * psDefItem );
	BOOL 										CharacterClassCanUseItem( ECharacterClass iClass, DefinitionItem * psDefItem );

	BOOL										DeleteItemInventory( UserData * pcUserData, Item* sItem);
	BOOL										DeleteItemInventory( UserData * pcUserData, DWORD dwCode, DWORD dwHead, DWORD dwChkSum );
	BOOL										AddItemInventory( UserData * pcUserData, Item* sItem );
	BOOL										AddItemInventory( UserData * pcUserData, DWORD dwCode, DWORD dwHead, DWORD dwChkSum );
	bool 										HasItemInInventory( UserData * pcUserData, ItemData* sItemData, BOOL bCheckLastInventorySnapshot = FALSE);
	EItemID 									GetWarpGateWing( User * pcUser );
	bool 										HasItemInInventory( UserData * pcUserData, EItemID p_ItemID, DWORD p_Chk1, DWORD p_Chk2, BOOL bCheckLastInventorySnapshot = FALSE);
    bool                                        IsPotion(DWORD dwCode)
    {
        return (dwCode & 0xFF000000) == (0x04010000 & 0xFF000000);
    }
	void										SaveItemDataToDatabase( Item * psItem );
	UINT										GetMixItemForm(Item * psItem);
	BOOL										CheckMixItem(Item * psItem);
	BOOL										CheckItemForm(Item * psItem);
#ifdef ITEMFIXES
	void										CreateItem( Item * psItem, DefinitionItem * psDefItem, EItemSource eItemSource, int iSpeck = 0, int iSpecAtkRating = 0, int iAgeLevel = 0, const EItemRarity itemRarity = EItemRarity::NONE, BOOL isPerfect = FALSE, int iPrefixID = 0, int iSuffixID = 0);
#else
	void										CreateItem( Item * psItem, DefinitionItem * psDefItem, EItemSource eItemSource, int iSpeck = 0, int iSpecAtkRating = 0, int iAgeLevel = 0, const EItemRarity itemRarity = EItemRarity::NONE);
#endif
	void										CreateItem1( User * pcUser, EItemID eItem, EItemSource eItemSource, int iCount );
	void										CreatePerfectItem( Item * psOut, DefinitionItem * psIn, EItemSource eItemSource, int iSpeck = 0, int iSpecAtkRating = 0, int iAgeLevel = 0 );


	StatModifier								GetNSStatMod(EItemType iItemType);
	StatModifier								GetItemSpecStatMod(int iSpec, char* chItemType);

	char*										GetTypeChar(EItemType iItemType);

	int											GetForceDamageByIndex ( int iForceIndex );
	int											GetForceDurationByIndex ( int iForceIndex );
	int											GetForceDamageExtraPercentageByIndex ( int iForceIndex );

	int 										EncodeCompress ( BYTE * lpSrcData, BYTE * lpDestData, int size );
	int 										DecodeCompress ( BYTE * lpSrcData, BYTE * lpDestData );

	int 										EncodeCompress ( BYTE * lpSrcData, BYTE * lpDestData, int size, int DestSize );
	int 										DecodeCompress ( BYTE * lpSrcData, BYTE * lpDestData, int dCount );

	int											AddPotion( UserData * pcUserData, int iItemID, int iCount );

	BOOL										GetDropPotion( UserData * pcUserData, int iItemID );
	BOOL										AddDropPotion( UserData * pcUserData, int iItemID );

	void										CreatePremium( User * pcUser, EItemID iItemID, int iType, DWORD dwTime, BOOL send);

	void										SetAgingItem( ItemData * psItemData );

	int											DecodeItemsData( void * pItemData, void * pItemDest, int iSize );
	int											EncodeItemsData( void * pItemData, void * pItemDest, int iSize );
	int											EncodeItemsData( void * pItemData, void * pItemDest, int iSize, int iSizeDest );

	void										OnSetItemPerfect ( Item * pItem );
	BOOL										CheckAndAdjustItemRequirements ( Item * pItem, DefinitionItem* pDefItem);

	void										TestAging ( int iAgingStoneType );
	AgingResultType								GetAgingResultType( int iAgingLevel, int iAgeStoneType, int iFailChance, int iPlus2Chance, int iMinus1Chance, int iMinus2Chance, int iBreakChance, BOOL bNoBreakEvent );
	void										SQLLogAgingResult ( User * pcUser, AgingResultType eAgingResultType, int iAgeLevel );
	int											GetAgingType( UserData * pcUserData );

	BOOL										OnUpgradeAgingItem( UserData * pcUserData, PacketItem * psPacket );
	BOOL										OnAgingItem( User * pcUser, PacketAgingItem * psPacket );

	void										ProcessIntegrity( User * pcUser, EIntegrityUpdate eUpdate );
	void										UpdateIntegrity( Item * psItem, int iValueCur, int iValueMax = 0 );

	DWORD										OnPhoenixPet( UserData * pcUserData, DWORD dwCode, BOOL bDelete, int iElementType = PHOENIXTYPEID_None );
	BOOL										OnPremiumItem( DWORD dwCode, int iX, int iY, int iZ, UserData * pcUserData );
	void										SendItemTimer(User* pcUser, EItemID iItemID, EItemTimerType iType, DWORD dwTimeLeft, DWORD dwTimeTotal);

	BOOL										ReformItem( ItemData * psItem );
	BOOL										ReformItem( Item * psItem );
	DWORD										GetReformItemCode( Item * psItem, int iKey );
	BOOL										ReformItemNew( Item * psItem, DWORD bTimeOverride = 0 );
	void										ValidateItemTime( Item * psItem );
	void										RegisterItemSecCode( UserData * pcUserData, PacketItem * psItem );

	void										SendItemDataToInventory ( UserData * pcUserData, Item * psItem );
	void										SendPotions( User * pcUser, EItemID pItemID, int iCount, EItemSource eItemSource, BOOL bAuto );
	void										SendItemData( User * pcUser, int iItemID, EItemSource eItemSource );
	void										SendItemData( User * pcUser, Item * psItem, BOOL bAuto = FALSE );
	void										SendItemData( UserData * pcUserData, Item * psItem, BOOL bAuto = FALSE );

	BOOL										OnSaveItemData( Item * psItem );
	BOOL										OnLoadItemData( ItemLoadData * psItem, const char * pszCode );

	ItemPremium									* GetPremiumUse( User * pcUserData, EItemTimerType eType );
	ItemPremium									* GetPremiumUse( User * pcUserData, EItemID eID );

	BOOL										IsHead(int iType);

	void										OnLoadUser( User * pcUser );
	void										OnUnLoadUser( User * pcUser );

	BOOL										SQLLoadPremiumData(User * pcUser, EItemTimerType eItemTimerType);

	void										OnUseCrystal( UserData * pcUserData, UnitData * pcUnitData, DWORD dwItemID, DWORD dwTime = 0 );
	void										OnUseCrystalDie( UserData * pcUserData, UnitData * pcUnitData );

	void										SQLSyncItemTimersToPrimary ( User * pcUser );
	BOOL										SQLGetItemTimer( User * pcUser, int iType, EItemID & iItemID, DWORD & dwTimeLeft, DWORD & dwTimeTotal );
	void										SQLUpdateHeadModel ( User * pcUser, char * headModel, int iSqlCharacterId );
	const std::string							SQLGetOldHeadModel ( User * pcUser );
	void										SQLUpdateItemTimer(  User * pcUser, int iType, DWORD dwTimeLeft );
	void										SQLDeleteItemTimer( User * pcUser, int iItemID );
	void										SQLDeleteItemTimerByTimerType ( User * pcUser, enum EItemTimerType eItemTimerType );
	void										SQLAddItemTimer( int iCharID, char * pszCharName, int iType, EItemID iItemID, DWORD dwTimeTotal );

	void										OnPremiumItemCancelledOrExpired (User * pcUser, ItemPremium * psItem, BOOL bExpired );

	void										SyncPremiumTimersToDatabase ( User * pcUser );
	void										SyncPremiumTimersToClient ( User * pcUser );

	BOOL										AddItemTimer ( User * pcUser, int iType, EItemID iItemID, DWORD dwTimeTotal );

	BOOL										OnResetItem( User * pcUser, struct PacketResetItem * psPacket );

	BOOL										OnSocketItem( User * pcUser, struct PacketSocketSystem * psPacket );

	void										OnManufactureItem( User * pcUser, struct PacketManufactureItem * psPacket );
	void										OnManufactureWeightPrice( User * pcUser );

	void										PHGetPerfectItem( User * pcUser, struct PacketGetItemPerfectView * psPacket );

	BOOL										IsItemFilteredIn( User * pcUser, ELootFilter e );
	BOOL										CanViewDropItem( User * pcUser, Loot * psStageItemData );

	void										HandlePacket( User * pcUser, PacketGetItem * psPacket );

	BOOL										SQLLoadItemBox( User * pcUser );
	void										SQLItemLogBox( char * pszAccountName, ItemLogBox * psItemLogBox );
	void										SQLItemLogBox( User * pcUser, Item * psItem, int iID, int iCount );
	int											SQLItemBoxCount( char * pszAccountName );
	BOOL										OpenItemBox( UserData * pcUserData, BOOL bRetrieveItem );
	BOOL										GetItemBoxItem( char * pszAccountName, int iID, Item * psItem );

	BOOL										AddItemOpenBox( UserData * pcUserData, char * pszItemName, int iSpec = 0, int iQuantity = 1, BOOL bCoinShop = FALSE, Item * psItem = NULL );
	BOOL										AddItemOpenBox( char * pszAccountName, char * pszItemName, int iSpec = 0, int iQuantity = 1, BOOL bCoinShop = FALSE, Item * psItem = NULL );

	void										PHCheckItemUsing( UserData * pcUserData, PacketListUsingItem * psPacket );
	void										PHCheckItemUsingEx( UserData * pcUserData, PacketListUsingItem * psPacket );

	void										UpgradeNewItem( Item * psItemOld, Item * psItemNew );
	int											RecvBuyShopItem ( User * pcUser, PacketBuyNPCShop * lpTransBuyShopItem );

	void										HandlePacket( User * pcUser, PacketBuyNPCShop * psPacket );

	BOOL										HandlePacket( User * pcUser, PacketSellNPCShop * psPacket );
	BOOL										HandlePacket( User * pcUser, PacketRepairNPCShop * psPacket );

	void										HandlePacket( User * pcUser, PacketNewItemTimer * psPacket );
	void										HandlePacket( User * pcUser, PacketCancelItemTimer * psPacket );

	void										HandlePacket( User * pcUser, PacketOldHeadUse * psPacket );

	void										HandlePacket( User * pcUser, PacketSlotItemHandle * psPacket );

	BOOL										SendItemShopList( User * pcUser, UnitData * pcUnitData );

	bool										IsItemTimerFreezedMap( EMapID iMapID, EItemTimerType iType );

	void										OnForceOrbActivated( User * pcUser, int iForceOrbIndex, EItemID eItemId );
	void										UpdateForceOrb( User * pcUser, EItemTimerType iType, EItemID eItemId  );
	void										RemoveForceOrb( User * pcUser );

	void										UpdateUserEXPBuff( User* pcUser );

};

