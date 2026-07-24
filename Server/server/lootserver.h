#pragma once

class LootServer
{
public:

	enum EDropType : int
	{
		DROPTYPE_UNKNOWN = -1,
		DROPTYPE_AIR = 1,
		DROPTYPE_GOLD = 2,
		DROPTYPE_ITEMS = 3,
	};

	struct BaseDropDefinition
	{
		int iDropChance;
		EDropType eDropType;
	};

	struct GoldDropDefinition : BaseDropDefinition
	{
		int iGoldMin;
		int iGoldMax;
	};

	struct ItemDropDefinition : BaseDropDefinition
	{
		std::vector<DWORD> vItemCodes;
	};

	//Nothing
	struct AirDropDefinition : BaseDropDefinition
	{

	};

	struct MonsterDropTable
	{
		int iTotalDropChance = 0;
		std::vector<BaseDropDefinition*> vDropDefinitions;
	};

	LootServer();
	virtual ~LootServer();


	void					SQLUpdateDropTableFromDatabase();

	BOOL					SendQuestDropItemToUser( UnitData * pcUnitData, User * pcUser );
	BOOL					GetRandomItemForMonster(UnitData * pcUnitData, User * pcUser, Item* psItem);

	BOOL					DropDefinitionExistsForMonsterID ( int iMonsterDropId );

	void					GenerateDropStats( std::string sMonsterName, const char * szSubFolder = "Test", int iRepeatCount = 10000);
	void					GenerateDropStatsMap( int iMapID, const char * szSubFolder = "Test", int iRepeatCount = 10000);

	bool					bLootDebug;
private:




	BaseDropDefinition *	GetRandomDropDefinition(int iMonsterDropId, User* pcUser = nullptr);

	/// <summary>
	/// In LOOT_MODE, weapons are strictly filtered to the "signature" weapon type
	/// for each class (e.g. Pikeman → Scythe only, Fighter → Sword/Axe).
	/// Non-weapon items are always accepted (class filtering is done elsewhere).
	/// </summary>
	static bool				IsItemAcceptableForClass( DWORD dwItemCode, ECharacterClass iClass );
	static bool				IsItemAcceptableInLootMode( DWORD dwItemCode, ECharacterClass iClass, User* pcUser = nullptr );
	static int				GetEquippedItemLevel( DefinitionItem* pDef, User* pcUser );

	std::map<int, MonsterDropTable>				      mDropTable;
	std::mutex										  mDropTableMutex;
};
