#pragma once

enum EGameLevel;

#define MAX_SERVERINFO		5
#define MAX_GAMEMASTERINFO	20

#define WAREHOUSE_ITEM_MAX 415



#define NUM_ITEMS			(*(UINT*)0x07AAC890)

enum EServerType
{
	SERVERTYPE_Unknown = 0,
	SERVERTYPE_Login = 1,
	SERVERTYPE_Game = 2,
	SERVERTYPE_Multi = 3,
};

enum EUsersOnline
{
	USERSONLINE_Insert = 0,
	USERSONLINE_Delete = 1,
	USERSONLINE_DeleteAll = 2
};

//Address to server config variables in ASM
enum class ESeverConfigASM
{
	ENABLE_PK	 = 0x845A80,	//rsServerConfig.Enable_PK
	PK_FIELD	 = 0x84831C		//rsServerConfig.Enable_PKField
};

struct ServerInfo
{
	EServerType		iType;
	char			szName[32];
	in_addr			lIP;
	char			szIP[32];
	in_addr			lIP2;
	char			szIP2[32];
	in_addr			lIP3;
	char			szIP3[32];
	in_addr			lNetIP;
	char			szNetIP[32];
	int				iPort;
	int				iMaxUsers;
};

class ServerCore
{
private:
	static const	DWORD		pfnHash = 0x00415F80;


	static void								FixItemBasedOnRarity(Item* item, const EItemRarity& itemRarity, BOOL bReverse = FALSE);
	static void								SetRarityBoostArmors(Item* item, const int statsBoostMultiplier, const int itemLevel);
	static void								SetRarityBoostBoots(Item* item, const int statsBoostMultiplier, const int itemLevel);
	static void								SetRarityBoostRobes(Item* item, const int statsBoostMultiplier, const int itemLevel);
	static void								SetRarityBoostGauntlets(Item* item, const int statsBoostMultiplier, const int itemLevel);
	static void								SetRarityBoostBracelets(Item* item, const int statsBoostMultiplier, const int itemLevel);
	static void								SetRarityBoostAmulets(Item* item, const int statsBoostMultiplier, const int itemLevel);
	static void								SetRarityBoostRings(Item* item, const int statsBoostMultiplier, const int itemLevel);
	static void								SetRarityBoostShields(Item* item, const int statsBoostMultiplier, const int itemLevel);
	static void								SetRarityBoostOrbs(Item* item, const int statsBoostMultiplier, const int itemLevel);

public:
	static DWORD *				pdwTickCount;
	static DWORD *				pdwUnitsOnline;
	static DWORD *				pdwUsersOnline;


	static inline DWORD			GetTickCounts()
	{
		return (*pdwTickCount);
	}
	static inline void			SetTickCounts( DWORD dw )
	{
		(*pdwTickCount) = dw;
	}
	static inline DWORD			GetUnitsOnline()
	{
		return (*pdwUnitsOnline);
	}
	static inline void			SetUnitsOnline( DWORD dw )
	{
		(*pdwUnitsOnline) = dw;
	}
	static inline DWORD			GetUsersOnline()
	{
		return (*pdwUsersOnline);
	}
	static inline void			SetUsersOnline( DWORD dw )
	{
		(*pdwUsersOnline) = dw;
	}
	static void					LoadDirty();
	static void					UnLoadDirty();
	static long					Hash( char * s );

	static void					DumpItemDataToSQL(Item* psItem, SQLConnection* psSql, BOOL bIsInventory, const char* pszAccountName, const char* pszCharName);

	static const char *		    GetCharOrAccountName();

	static BOOL					CheckAndAdjustFloat ( const char * szItemName, ItemID pItemID, const char * label, float * pFloat, float minValue, float maxValue, float minValueOld, float maxValueOld );
	static BOOL					CheckAndAdjustFloat ( const char * szItemName, ItemID pItemID, const char * label, float * pFloat, float value );
	static BOOL					CheckAndAdjustInt ( const char * szItemName, ItemID pItemID, const char * label, int * pFloat, int minValue, int maxValue, int minValueOld, int maxValueOld );
	static BOOL					CheckAndAdjustInt ( const char * szItemName, ItemID pItemID, const char * label, int * pFloat, int value );
	static BOOL					CheckAndAdjustShort ( const char * szItemName, ItemID pItemID, const char * label, short * pFloat, short minValue, short maxValue, short minValueOld, short maxValueOld );
	static BOOL					CheckAndAdjustShort ( const char * szItemName, ItemID pItemID, const char * label, short * pFloat, short value );
	static BOOL					CheckAndUpdateStats ( Item * psItem );
	static BOOL					UpdateItemsToUncommon(Item* psItem);
	static BOOL					UpdateItemsMisc(Item* psItem);

	static void					UpdateCharacterItemsLevelData( const char * pszFilePath, const char* pszAccountName, const char* pszCharName, BOOL bUpgradeItems, BOOL bDumpToSQL );
	static void					UpdateCharacterItemsLevelWarehouse( const char * pszFilePath, const char* pszName, BOOL bUpgradeItems, BOOL bDumpToSQL);
	static void					UpdateCharacterItemsLevel( const char * pszBaseFolder, const char * pszName );


	static void					WarehouseTest();
	static void					UpdateCharacterLevelAll ( );
	static void					UpdateCharacterItemsLevelAll( BOOL bSkipBackups = FALSE );
};