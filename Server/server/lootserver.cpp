#include "stdafx.h"
#include "lootserver.h"
#include "Logger.h"
#include "Utilities.h"


LootServer::LootServer()
{
	bLootDebug = false;
}

LootServer::~LootServer()
{
}

void LootServer::SQLUpdateDropTableFromDatabase()
{
	//Only for game-server
	if (LOGIN_SERVER)
		return;

	INFO("Reading in DropItem from SQL database..");

	std::lock_guard<std::mutex> guard(mDropTableMutex);

	mDropTable.clear();

	SQLConnection* pcDB = SQLCONNECTION(DATABASEID_GameDB, 6);
	if (pcDB->Open())
	{
		if (pcDB->Prepare("SELECT * FROM DropItem WHERE DropID > 0 ORDER BY DropID ASC, Chance DESC"))
		{
			if (pcDB->Execute())
			{
				LootServer::MonsterDropTable* sCurrentTable = nullptr;
				int iMonsterIdCurrent = 0;
				int iMonsterId = 0;
				char szItems[512] = { 0 };
				int iChance = 0;
				int iGoldMin = 0;
				int iGoldMax = 0;
				BOOL bFoundAny = FALSE;

				while (pcDB->Fetch())
				{
					bFoundAny = TRUE;

					pcDB->GetData(2, PARAMTYPE_Integer, &iMonsterId);
					pcDB->GetData(3, PARAMTYPE_String, szItems, 512);
					pcDB->GetData(4, PARAMTYPE_Integer, &iChance);
					pcDB->GetData(5, PARAMTYPE_Integer, &iGoldMin);
					pcDB->GetData(6, PARAMTYPE_Integer, &iGoldMax);

					if ( iMonsterIdCurrent != iMonsterId )
					{
						//DEBUG(" - Reading drop data for monster id: %d", iMonsterId);

						mDropTable.insert({ iMonsterId, LootServer::MonsterDropTable() });
						iMonsterIdCurrent = iMonsterId;
						sCurrentTable = &mDropTable[iMonsterId];
					}

					if (sCurrentTable)
					{

						if (STRINGCOMPAREI(szItems, "Gold"))
						{
							auto sGoldDrop = new LootServer::GoldDropDefinition();
							sGoldDrop->eDropType = LootServer::DROPTYPE_GOLD;
							sGoldDrop->iDropChance = iChance;
							sGoldDrop->iGoldMin = iGoldMin;
							sGoldDrop->iGoldMax = iGoldMax;

							sCurrentTable->iTotalDropChance += iChance;
							sCurrentTable->vDropDefinitions.push_back(sGoldDrop);
						}
						else if (STRINGCOMPAREI(szItems, "Air"))
						{
							auto sAirDrop = new LootServer::AirDropDefinition();
							sAirDrop->eDropType = LootServer::DROPTYPE_AIR;
							sAirDrop->iDropChance = iChance;

							sCurrentTable->iTotalDropChance += iChance;
							sCurrentTable->vDropDefinitions.push_back(sAirDrop);
						}
						else
						{
							auto sItemDropGroup = new LootServer::ItemDropDefinition();
							sItemDropGroup->eDropType = LootServer::DROPTYPE_ITEMS;
							sItemDropGroup->iDropChance = iChance;

							std::vector<std::string> vItems = split(szItems, ' ');
							for (std::string vItem : vItems)
							{
								ItemData* pItem = ITEMSERVER->FindItemPointerTable(vItem.c_str());
								if (pItem)
								{
									sItemDropGroup->vItemCodes.push_back(pItem->sBaseItemID.ToItemID());
								}
								else
								{
									WARN ( "Monster id: %d, Unknown item code: %s", iMonsterId, vItem.c_str () );
								}
							}


							sCurrentTable->iTotalDropChance += iChance;
							sCurrentTable->vDropDefinitions.push_back(sItemDropGroup);
						}
					}
				}
			}
		}

		pcDB->Close();
	}

	LOGGER->Flush();
}

//Test drop statistics by picking a loot from random
//then print it out to a CSV file
void LootServer::GenerateDropStats(std::string sMonsterName, const char * szSubFolder, int iRepeatCount, User* pcUser)
{
	//Only for game-server
	if (LOGIN_SERVER)
		return;


	if ( CreateDirectory ( "TestDrops", NULL ) ||
		 ERROR_ALREADY_EXISTS == GetLastError () )
	{
		//Check and create dir
	}
	else
	{
		return;
	}

	std::string folderPath = FormatString("TestDrops\\%s", szSubFolder);

	if (CreateDirectory(folderPath.c_str(), NULL) ||
		ERROR_ALREADY_EXISTS == GetLastError())
	{
		//Check and create dir
	}
	else
	{
		return;
	}

	CharacterData * lpCharacterData = UNITSERVER->GetCharacterDataByName ( sMonsterName.c_str () );

	if ( lpCharacterData == NULL )
	{
		WARN ( "GenerateDropStats - Monster name '%s' is not valid!", sMonsterName.c_str () );
		return;
	}


	int iNumDrops = lpCharacterData->psUnitInfo->iNumDrops;

	INFO ("Generating drop stats for %s (%d). Num drops = %d", sMonsterName.c_str(), lpCharacterData->iUniqueMonsterID, iNumDrops);

	//no drops?
	if ( iNumDrops == 0 )
	{
		WARN ( "GenerateDropStats - Monster '%s' has zero drop for base", sMonsterName.c_str () );
		return;
	}

	//+2 = for third eye + hunt mode
	iNumDrops += 2;

	//chance > item code > counter
	std::map<int, std::map<DWORD, int>> l_DropStats;

	DWORD itemCode = 0;

	MonsterDropTable* monsterDropTable = &mDropTable[lpCharacterData->iUniqueMonsterID];

	for (int i_RepeatNum = 0; i_RepeatNum < iRepeatCount; i_RepeatNum++)
	{
		for (int i_DropNum = 0; i_DropNum < iNumDrops; i_DropNum++)
		{
			BaseDropDefinition* baseDropDefinition = GetRandomDropDefinition( lpCharacterData->iUniqueMonsterID, pcUser );

			//monster id not found
			if (baseDropDefinition == nullptr)
				return;

			//drop type is air, return false also
			if (baseDropDefinition->eDropType == DROPTYPE_AIR)
			{
				itemCode = 0;
			}

			//drop type is gold
			if (baseDropDefinition->eDropType == DROPTYPE_GOLD)
			{
				itemCode = ITEMID_Gold;


			}

			if (baseDropDefinition->eDropType == DROPTYPE_ITEMS)
			{
				ItemDropDefinition* itemDropDef = (ItemDropDefinition*)baseDropDefinition;

				int count = itemDropDef->vItemCodes.size();
				int randomIndex = Dice::RandomI(0, count - 1);

				//pick item code from group
				itemCode = itemDropDef->vItemCodes[randomIndex];


			}

			if (l_DropStats.find(baseDropDefinition->iDropChance) == l_DropStats.end())
			{
				//not found
				l_DropStats.insert({ baseDropDefinition->iDropChance, std::map<DWORD,int>{} });
			}

			if (l_DropStats[baseDropDefinition->iDropChance].find(itemCode) == l_DropStats[baseDropDefinition->iDropChance].end())
			{
				//not found
				l_DropStats[baseDropDefinition->iDropChance].insert({ itemCode, 1 });
			}
			else
			{
				//increment by one
				l_DropStats[baseDropDefinition->iDropChance][itemCode]++;
			}
		}
	}

	std::string logFileName = FormatString("%s\\MonsterDropTest_%s_%d_summary.txt", folderPath.c_str(), sMonsterName.c_str(), iNumDrops, iRepeatCount);
	CLogger* logger = new CLogger(logFileName);

	//remove log file if it exists
	remove(logFileName.c_str());



	logger->WriteLine("");
	logger->WriteLine("");
	logger->WriteLine(Util::CurrentDateTime());
	logger->WriteLine("==========================================================================================");
	logger->WriteLine("==========================================================================================");
	logger->WriteLine("Monster name              = %s", sMonsterName.c_str());
	logger->WriteLine("Base drop count           = %d", iNumDrops - 2);
	logger->WriteLine("Max drop count            = %d", iNumDrops);
	logger->WriteLine("Number of repeats (kills) = %d", iRepeatCount);
	logger->WriteLine("Total number of drops     = %d", iNumDrops * iRepeatCount);
	logger->WriteLine("");
	logger->WriteLine("TEST SUMMARY");
	logger->WriteLine("-----------------------------------------------------------------");

	std::string itemName = "";
	std::string chanceStr = "";
	std::string emptyChanceStr = "";
	int iNumOfHits = 0;

	padTo(emptyChanceStr, 7);

	for (auto const & dropDef : monsterDropTable->vDropDefinitions)
	{
		float fDropChance = static_cast<float>(dropDef->iDropChance) / static_cast<float>(monsterDropTable->iTotalDropChance);
		chanceStr = FormatString("%.3f%%", fDropChance * 100.0f);

		padTo(chanceStr, 7);


		if (dropDef->eDropType == DROPTYPE_AIR)
		{
			itemName = "Air (Nothing)";

			if (l_DropStats[dropDef->iDropChance].find(0) != l_DropStats[dropDef->iDropChance].end())
				iNumOfHits = l_DropStats[dropDef->iDropChance][0];
			else
				iNumOfHits = 0;

			padTo(itemName, 30);

			std::string hitsStr = FormatString("%d hits", iNumOfHits);
			padTo(hitsStr, 11);

			if (iNumOfHits > 0)
			{
				float fHitChance = static_cast<float>(iNumOfHits) / static_cast<float>(iNumDrops * iRepeatCount);
				logger->WriteLine("%s : %s = %s (%.3f%%)", chanceStr.c_str(), itemName.c_str(), hitsStr.c_str(), fHitChance * 100.f);
			}
			else
			{
				logger->WriteLine("%s : %s = %s (0%%)", chanceStr.c_str(), itemName.c_str(), hitsStr.c_str());
			}
		}
		else if (dropDef->eDropType == DROPTYPE_GOLD)
		{
			GoldDropDefinition* goldDropDef = (GoldDropDefinition*)dropDef;
			itemName = FormatString("Gold (%d - %d)", goldDropDef->iGoldMin, goldDropDef->iGoldMax);

			if (l_DropStats[dropDef->iDropChance].find(ITEMID_Gold) != l_DropStats[dropDef->iDropChance].end())
				iNumOfHits = l_DropStats[dropDef->iDropChance][ITEMID_Gold];
			else
				iNumOfHits = 0;

			padTo(itemName, 30);

			std::string hitsStr = FormatString("%d hits", iNumOfHits);
			padTo(hitsStr, 11);

			if (iNumOfHits > 0)
			{
				float fHitChance = static_cast<float>(iNumOfHits) / static_cast<float>(iNumDrops * iRepeatCount);
				logger->WriteLine("%s : %s = %s (%.3f%%)", chanceStr.c_str(), itemName.c_str(), hitsStr.c_str(), fHitChance * 100.f);
			}
			else
			{
				logger->WriteLine("%s : %s = %s (0%%)", chanceStr.c_str(), itemName.c_str(), hitsStr.c_str());
			}
		}
		else if (dropDef->eDropType == DROPTYPE_ITEMS)
		{
			ItemDropDefinition* itemDropDef = (ItemDropDefinition*)dropDef;

			int iCounter = 0;
			int iTotalHitsInGroup = 0;
			BOOL bFirst = TRUE;

			set<DWORD> unique;
			for (DWORD i : itemDropDef->vItemCodes)
				unique.insert(i);

			itemDropDef->vItemCodes.assign(unique.begin(), unique.end());

			for (DWORD code : unique)
			{
				auto pDefItem = ITEMSERVER->FindItemDefByCode(code);

				if (pDefItem)
				{
					if (l_DropStats[dropDef->iDropChance].find(code) != l_DropStats[dropDef->iDropChance].end())
						iNumOfHits = l_DropStats[dropDef->iDropChance][code];
					else
						iNumOfHits = 0;

					iTotalHitsInGroup += iNumOfHits;

					if(pDefItem->sItem.iLevel > 0)
					{
						itemName = FormatString("%s (Lv. %d)", pDefItem->sItem.szItemName, pDefItem->sItem.iLevel);
					}
					else
					{
						itemName = FormatString("%s", pDefItem->sItem.szItemName);
					}

					padTo(itemName, 30);

					std::string hitsStr = FormatString("%d hits", iNumOfHits);
					padTo(hitsStr, 11);

					if (iNumOfHits > 0)
					{
						float fHitChance = static_cast<float>(iNumOfHits) / static_cast<float>(iNumDrops * iRepeatCount);
						logger->WriteLine("%s : %s = %s (%.3f%%)", bFirst ? chanceStr.c_str() : emptyChanceStr.c_str(), itemName.c_str(), hitsStr.c_str(), fHitChance * 100.f);
					}
					else
					{
						logger->WriteLine("%s : %s = %s (0%%)", bFirst ? chanceStr.c_str() : emptyChanceStr.c_str(), itemName.c_str(), hitsStr.c_str());
					}
				}
				else
				{
					logger->WriteLine("ERROR - code not found: %d", code);
				}

				iCounter++;
				bFirst = FALSE;
			}

			//more than 1 items in group?
			if (iCounter > 1 && iTotalHitsInGroup > 0)
			{
				std::string sSummaryItemName = ""; //FormatString("SUMMARY for group: (%.3f%%)", fDropChance * 100.0f);
				padTo(sSummaryItemName, 30);

				std::string hitsStr = FormatString("%d hits", iTotalHitsInGroup);
				padTo(hitsStr, 11);

				float fHitChance = static_cast<float>(iTotalHitsInGroup) / static_cast<float>(iNumDrops * iRepeatCount);
				logger->WriteLine("%s   %s = %s (%.3f%%) <-- Summary", emptyChanceStr.c_str(), sSummaryItemName.c_str(), hitsStr.c_str(), fHitChance * 100.f);

			}
		}
	}

	logger->WriteLine("-----------------------------------------------------------------");


	logger->Flush();
	logger->Close();
}

void LootServer::GenerateDropStatsMap(int iMapID, const char * szSubFolder, int iRepeatCount, User* pcUser)
{
	//Only for game-server
	if (LOGIN_SERVER)
		return;


	if ( CreateDirectory ( "TestDrops", NULL ) ||
		ERROR_ALREADY_EXISTS == GetLastError () )
	{
		//Check and create dir
	}
	else
	{
		return;
	}

	std::string folderPath = FormatString("TestDrops\\%s", szSubFolder);

	if (CreateDirectory(folderPath.c_str(), NULL) ||
		ERROR_ALREADY_EXISTS == GetLastError())
	{
		//Check and create dir
	}
	else
	{
		return;
	}

	std::vector<string> MonsterNameList;

	SQLConnection * pcDB = SQLCONNECTION( DATABASEID_GameDB_Monsters, 2 );

	if ( pcDB->Open () )
	{

		if ( pcDB->Prepare ( FormatString ( "SELECT [Monster1],[Count1],[Monster2],[Count2],[Monster3],[Count3],[Monster4],[Count4],[Monster5],[Count5],[Monster6],[Count6],[Monster7],[Count7],[Monster8],[Count8],[Monster9],[Count9],[Monster10],[Count10],[Monster11],[Count11],[Monster12],[Count12] FROM MapMonster WHERE Stage=%d", iMapID ) ) )
		{

			if ( pcDB->Execute () && pcDB->Fetch () )
			{
				int j = 0;
				for ( int i = 0; i < 12; i++ )
				{
					string CurrentMon = "";
					int CurrentCount = 0;
					char  szMonsterName[150] = { 0 };

					pcDB->GetData ( j + 1, PARAMTYPE_String, szMonsterName, 150 );
					pcDB->GetData ( j + 2, PARAMTYPE_Integer, &CurrentCount );

					CurrentMon = std::string ( szMonsterName );
					if ( CurrentMon != "" )
					{
						for ( int i = 0; i < CurrentCount; i++ )
						{
							MonsterNameList.push_back ( CurrentMon );
						}
					}
					j += 2;
				}
			}
		}

		pcDB->Close();
	}


	INFO ("Generating drop stats for Map:%d ", iMapID);

	//Item Name > Item QTY > counter
	std::map<string, int> l_FinalDrops;
	int TotalDrops = 0;


	for (int i_RepeatNum = 0; i_RepeatNum < iRepeatCount; i_RepeatNum++)
	{
		int index = rand () % MonsterNameList.size ();

		CharacterData * lpCharacterData = UNITSERVER->GetCharacterDataByName ( MonsterNameList[index].c_str () );

		if ( lpCharacterData == NULL )
		{
			WARN ( "GenerateDropStats - Monster name '%s' is not valid!", MonsterNameList[index].c_str () );
			return;
		}

		//chance > item code > counter
		std::map<int, std::map<DWORD, int>> l_DropStats;

		int iNumDrops = lpCharacterData->psUnitInfo->iNumDrops;

		//no drops?
		if ( iNumDrops == 0 )
		{
			WARN ( "GenerateDropStats - Monster '%s' has zero drop for base", MonsterNameList[index].c_str () );
			return;
		}

		//+2 = for third eye + hunt mode
		iNumDrops += 2;

		TotalDrops += iNumDrops;

		DWORD itemCode = 0;

		MonsterDropTable * monsterDropTable = &mDropTable[lpCharacterData->iUniqueMonsterID];


		for (int i_DropNum = 0; i_DropNum < iNumDrops; i_DropNum++)
		{
			BaseDropDefinition* baseDropDefinition = GetRandomDropDefinition( lpCharacterData->iUniqueMonsterID, pcUser );

			//monster id not found
			if (baseDropDefinition == nullptr)
				return;

			//drop type is air, return false also
			if (baseDropDefinition->eDropType == DROPTYPE_AIR)
			{
				itemCode = 0;
			}

			//drop type is gold
			if (baseDropDefinition->eDropType == DROPTYPE_GOLD)
			{
				itemCode = ITEMID_Gold;

			}

			DefinitionItem* pDefItem;

			if (baseDropDefinition->eDropType == DROPTYPE_ITEMS)
			{
				ItemDropDefinition* itemDropDef = (ItemDropDefinition*)baseDropDefinition;

				int count = itemDropDef->vItemCodes.size();
				int randomIndex = Dice::RandomI(0, count - 1);

				//pick item code from group
				itemCode = itemDropDef->vItemCodes[randomIndex];

				pDefItem = ITEMSERVER->FindItemDefByCode(itemCode);

			}


			if (l_DropStats.find(baseDropDefinition->iDropChance) == l_DropStats.end())
			{
				//not found
				l_DropStats.insert({ baseDropDefinition->iDropChance, std::map<DWORD,int>{} });
			}

			if (l_DropStats[baseDropDefinition->iDropChance].find(itemCode) == l_DropStats[baseDropDefinition->iDropChance].end())
			{
				//not found
				l_DropStats[baseDropDefinition->iDropChance].insert({ itemCode, 1 });
			}
			else
			{
				//increment by one
				l_DropStats[baseDropDefinition->iDropChance][itemCode]++;
			}
		}

		set<DWORD> ItemNames;

		//Second Part of stuff
		std::string itemName = "";
		std::string chanceStr = "";
		std::string emptyChanceStr = "";
		int iNumOfHits = 0;

		for (auto const & dropDef : monsterDropTable->vDropDefinitions)
		{
			float fDropChance = static_cast<float>(dropDef->iDropChance) / static_cast<float>(monsterDropTable->iTotalDropChance);
			chanceStr = FormatString("%.3f%%", fDropChance * 100.0f);

			padTo(chanceStr, 7);

			if (dropDef->eDropType == DROPTYPE_AIR)
			{
				itemName = "Air (Nothing)";

				if (l_DropStats[dropDef->iDropChance].find(0) != l_DropStats[dropDef->iDropChance].end())
					iNumOfHits = l_DropStats[dropDef->iDropChance][0];
				else
					iNumOfHits = 0;

				if (l_FinalDrops.find(itemName) == l_FinalDrops.end())
					l_FinalDrops.insert({ itemName, iNumOfHits});
				else
					l_FinalDrops[itemName] += iNumOfHits;

			}
			else if (dropDef->eDropType == DROPTYPE_GOLD)
			{
				GoldDropDefinition* goldDropDef = (GoldDropDefinition*)dropDef;
				itemName = FormatString("Gold (%d - %d)", goldDropDef->iGoldMin, goldDropDef->iGoldMax);

				if (l_DropStats[dropDef->iDropChance].find(ITEMID_Gold) != l_DropStats[dropDef->iDropChance].end())
					iNumOfHits = l_DropStats[dropDef->iDropChance][ITEMID_Gold];
				else
					iNumOfHits = 0;

				if (l_FinalDrops.find(itemName) == l_FinalDrops.end())
					l_FinalDrops.insert({ itemName, iNumOfHits});
				else
					l_FinalDrops[itemName] += iNumOfHits;

			}
			else if (dropDef->eDropType == DROPTYPE_ITEMS)
			{
				ItemDropDefinition* itemDropDef = (ItemDropDefinition*)dropDef;

				int iCounter = 0;
				BOOL bFirst = TRUE;

				set<DWORD> unique;
				for (DWORD i : itemDropDef->vItemCodes)
					unique.insert(i);


				itemDropDef->vItemCodes.assign(unique.begin(), unique.end());

				for (DWORD code : unique)
				{
					auto pDefItem = ITEMSERVER->FindItemDefByCode(code);

					string itemName = (string)pDefItem->sItem.szItemName;

					if (pDefItem)
					{
						if (l_DropStats[dropDef->iDropChance].find(code) != l_DropStats[dropDef->iDropChance].end())
							iNumOfHits = l_DropStats[dropDef->iDropChance][code];
						else
							iNumOfHits = 0;

						if ( l_FinalDrops.find ( itemName ) == l_FinalDrops.end () )
							l_FinalDrops.insert ( { itemName, iNumOfHits } );
						else
							l_FinalDrops[itemName] += iNumOfHits;

					}

					iCounter++;
					bFirst = FALSE;
				}

			}
		}


	}

	std::string logFileName = FormatString( "%s\\MonsterDropTest_%d_summary.txt", folderPath.c_str(), iMapID, iRepeatCount );
	CLogger * logger = new CLogger( logFileName );

	//remove log file if it exists
	remove( logFileName.c_str() );



	logger->WriteLine( "" );
	logger->WriteLine( "" );
	logger->WriteLine( Util::CurrentDateTime() );
	logger->WriteLine( "==========================================================================================" );
	logger->WriteLine( "==========================================================================================" );
	logger->WriteLine( "Map ID = %d", iMapID );
	logger->WriteLine( "Number of repeats (kills) = %d", iRepeatCount );
	logger->WriteLine( "" );
	logger->WriteLine( "TEST SUMMARY" );
	logger->WriteLine( "-----------------------------------------------------------------" );


	for ( std::map<string, int>::iterator it = l_FinalDrops.begin(); it != l_FinalDrops.end(); ++it )
	{
		string ItemName = it->first;
		int iNumOfHits = it->second;

		char * cItemName = strcpy( new char[ItemName.length() + 1], ItemName.c_str() );

		padTo( ItemName, 30 );

		std::string hitsStr = FormatString( "%d hits", iNumOfHits );
		padTo( hitsStr, 11 );

		if ( iNumOfHits > 0 )
		{
			float fHitChance = static_cast<float>( iNumOfHits ) / static_cast<float>( TotalDrops * iRepeatCount );
			logger->WriteLine( " %s = %s (%.3f%%)", ItemName.c_str(), hitsStr.c_str(), fHitChance * 100.f );
		}
		else
		{
			logger->WriteLine( "%s = %s (0%%)", ItemName.c_str(), hitsStr.c_str() );
		}

	}

	logger->WriteLine( "-----------------------------------------------------------------" );


	logger->Flush();
	logger->Close();
}

BOOL LootServer::DropDefinitionExistsForMonsterID( int iMonsterID )
{
	return mDropTable.find( iMonsterID ) != mDropTable.end();
}

// ------------------------------------------------------------------
// LOOT_MODE: Strict weapon→class signature mapping.
// Weapons, shields, armor/robes are filtered per class.
// ------------------------------------------------------------------
bool LootServer::IsItemAcceptableForClass( DWORD dwItemCode, ECharacterClass iClass )
{
	DWORD eItemBase = dwItemCode & 0xFF000000;
	DWORD eItemType = dwItemCode & 0xFFFF0000;

	// ---- Weapons ----
	if ( eItemBase == ITEMBASE_Weapon )
	{
		switch ( iClass )
		{
		case CHARACTERCLASS_Fighter:
			return ( eItemType == ITEMTYPE_Axe );

		case CHARACTERCLASS_Mechanician:
			return ( eItemType == ITEMTYPE_Claw || eItemType == ITEMTYPE_Hammer );

		case CHARACTERCLASS_Archer:
			return ( eItemType == ITEMTYPE_Bow );

		case CHARACTERCLASS_Atalanta:
			return ( eItemType == ITEMTYPE_Javelin );

		case CHARACTERCLASS_Pikeman:
			return ( eItemType == ITEMTYPE_Scythe );

		case CHARACTERCLASS_Knight:
			return ( eItemType == ITEMTYPE_Sword );

		case CHARACTERCLASS_Magician:
		case CHARACTERCLASS_Priestess:
			return ( eItemType == ITEMTYPE_Wand || eItemType == ITEMTYPE_Orb );

		case CHARACTERCLASS_Assassin:
			return ( eItemType == ITEMTYPE_Dagger );

		case CHARACTERCLASS_Shaman:
			return ( eItemType == ITEMTYPE_Phantom );

		default:
			return true;
		}
	}

	// ---- Shields (only Mechanician and Atalanta) ----
	if ( eItemType == ITEMTYPE_Shield )
	{
		return ( iClass == CHARACTERCLASS_Mechanician || iClass == CHARACTERCLASS_Knight || iClass == CHARACTERCLASS_Atalanta );
	}

	// ---- Armor vs Robes ----
	if ( eItemType == ITEMTYPE_Armor || eItemType == ITEMTYPE_Robe )
	{
		switch ( iClass )
		{
		case CHARACTERCLASS_Magician:
		case CHARACTERCLASS_Priestess:
		case CHARACTERCLASS_Shaman:
			return ( eItemType == ITEMTYPE_Robe );

		default:
			return ( eItemType == ITEMTYPE_Armor );
		}
	}

	return true; // boots, gauntlets, bracelets, rings, amulets, orbs — no restriction
}

// LOOT_MODE: Returns true if the item is acceptable — not a potion/crystal/core,
// class-usable, passes the strict weapon/armor signature check, and is an
// ilvl upgrade over the player's currently equipped item in that slot.
bool LootServer::IsItemAcceptableInLootMode( DWORD dwItemCode, ECharacterClass iClass, User* pcUser )
{
	DWORD eItemBase = dwItemCode & 0xFF000000;

	// Skip potions, crystals, and cores in LootMode
	if ( eItemBase == ITEMBASE_Potion || eItemBase == ITEMBASE_Crystal || eItemBase == ITEMBASE_Core )
		return false;

	auto pDef = ITEMSERVER->FindItemDefByCode( dwItemCode );
	if ( !pDef )
		return false;

	if ( pDef->JobBitCodeRandomCount == 0 )
		return true;

	if ( !ITEMSERVER->CharacterClassCanUseItem( iClass, pDef ) ||
	     !LootServer::IsItemAcceptableForClass( dwItemCode, iClass ) )
		return false;

	// ilvl upgrade check: reject if not better than currently equipped
	if ( pcUser )
	{
		int iEquippedLevel = GetEquippedItemLevel( pDef, pcUser );
		if ( iEquippedLevel > 0 && pDef->sItem.iLevel <= iEquippedLevel )
		{
			if ( LOOTSERVER->bLootDebug )
			{
				INFO("IsItemAcceptableInLootMode: Rejecting item %s (ilvl %d) for player %s (equipped ilvl %d)",
					pDef->sItem.szItemName, pDef->sItem.iLevel, pcUser->GetName(), iEquippedLevel);
			}
			return false;
		}
	}

	return true;
}

// Returns the ilvl of the player's equipped item in the same slot as pDef,
// or 0 if nothing is equipped or the slot is not tracked.
int LootServer::GetEquippedItemLevel( DefinitionItem* pDef, User* pcUser )
{
	if ( !pDef || !pcUser )
		return 0;

	DWORD eItemType = pDef->sItem.sItemID.ToItemType();
	DWORD eItemBase = eItemType & 0xFF000000;

	EItemID eEquipped = (EItemID)0;

	if ( eItemBase == ITEMBASE_Weapon )
		eEquipped = pcUser->pcUserData->eWeaponEquipped;
	else if ( eItemType == ITEMTYPE_Shield )
		eEquipped = pcUser->pcUserData->eShieldEquipped;
	else switch ( eItemType )
	{
	case ITEMTYPE_Armor:		eEquipped = pcUser->eArmorEquipped;		break;
	case ITEMTYPE_Boots:		eEquipped = pcUser->eBootsEquipped;		break;
	case ITEMTYPE_Gauntlets:	eEquipped = pcUser->eGauntletsEquipped;	break;
	case ITEMTYPE_Bracelets:	eEquipped = pcUser->eBraceletEquipped;	break;
	case ITEMTYPE_Ring:
	case ITEMTYPE_Ring2:
		// Compare against the lower-level ring so either ring slot gets an upgrade
		{
			auto pRingR = ITEMSERVER->FindItemDefByCode( pcUser->eRingRightEquipped );
			auto pRingL = ITEMSERVER->FindItemDefByCode( pcUser->eRingLeftEquipped );
			int iRingR = pRingR ? pRingR->sItem.iLevel : 0;
			int iRingL = pRingL ? pRingL->sItem.iLevel : 0;
			return ( iRingR > 0 && iRingL > 0 ) ? min( iRingR, iRingL ) : max( iRingR, iRingL );
		}
	case ITEMTYPE_Orb:			eEquipped = pcUser->eOrbEquipped;			break;
	case ITEMTYPE_Robe:			eEquipped = pcUser->eRobeEquipped;			break;
	case ITEMTYPE_Amulet:		eEquipped = pcUser->eAmuletEquipped;		break;
	default: return 0;
	}

	if ( !eEquipped )
		return 0;

	auto pEquippedDef = ITEMSERVER->FindItemDefByCode( eEquipped );
	return pEquippedDef ? pEquippedDef->sItem.iLevel : 0;
}

LootServer::BaseDropDefinition * LootServer::GetRandomDropDefinition( int iMonsterId, User* pcUser )
{
	//Only for game-server
	if ( LOGIN_SERVER )
		return nullptr;

	std::lock_guard<std::mutex> guard(mDropTableMutex);

	auto it = mDropTable.find( iMonsterId );
	if ( it == mDropTable.end() )
	{
		WARN( "Drop table not found for monster drop id: %d", iMonsterId );
		return nullptr;
	}

	MonsterDropTable * monsterDropTable = &mDropTable[iMonsterId];

	const int kMaxRetries = 100;
	// LOOT_MODE: filter gold/non-class drops at the definition level.
	// Retry up to kMaxRetries times to find a suitable drop definition.
	if ( LOOT_MODE && pcUser )
	{
		ECharacterClass iPlayerClass = pcUser->pcUserData->sCharacterData.iClass;

		for ( int iRetry = 0; iRetry < kMaxRetries; iRetry++ )
		{
			int iRand = Dice::RandomI( 0, monsterDropTable->iTotalDropChance );
			int iTotal = 0;

			for ( BaseDropDefinition * v : monsterDropTable->vDropDefinitions )
			{
				iTotal += v->iDropChance;
				if ( iRand <= iTotal )
				{
					// Skip gold & air entirely
					if ( v->eDropType == DROPTYPE_GOLD || v->eDropType == DROPTYPE_AIR )
					{
						break;
					}

					// Item group: check if at least one item is usable by this class
					if ( v->eDropType == DROPTYPE_ITEMS )
					{
						ItemDropDefinition* itemDropDef = reinterpret_cast<ItemDropDefinition*>(v);
						for ( DWORD dwCode : itemDropDef->vItemCodes )
						{
							if ( IsItemAcceptableInLootMode( dwCode, iPlayerClass, pcUser ) )
							{
								if ( LOOTSERVER->bLootDebug )
								{
									INFO("GetRandomDropDefinition: Found usable item for monster in LOOT_MODE: %s", ITEMSERVER->FindItemDefByCode(dwCode)->sItem.szItemName);
								}
								return v; // found a usable item in this group
							}
						}

						// No usable items in this group — retry
						break;
					}

					break; // Non-item drop (shouldn't happen in LOOT_MODE), retry
				}
			}
		}

		if ( LOOTSERVER->bLootDebug )
		{
			INFO("GetRandomDropDefinition: No suitable drop found for monster in LOOT_MODE");
		}

		// just return nothing to not pollute the ground
		return nullptr;
	}

	// Default: pure random without filtering
	int iRand = Dice::RandomI( 0, monsterDropTable->iTotalDropChance );
	int iTotal = 0;

	for ( BaseDropDefinition * v : monsterDropTable->vDropDefinitions )
	{
		iTotal += v->iDropChance;
		if ( iRand <= iTotal )
		{
			return v;
		}
	}

	return nullptr;
}

BOOL LootServer::SendQuestDropItemToUser( UnitData * pcUnitData, User * pcUser )
{
	switch ( pcUnitData->sCharacterData.iUniqueMonsterID )
	{
		case QUESTMONSTERID_BeeDog:
		{
			if ( Dice::RandomI( 1, 2 ) == 1 ) //1 in 2 chance
			{
				ITEMSERVER->SendItemData( pcUser, ITEMID_QuestHoneyQuest, EItemSource::QuestKill );
			}
		}
		break;

		case QUESTMONSTERID_MinigueSilver:
		{
			if ( Dice::RandomI( 1, 2 ) == 1 ) //1 in 2 chance
			{
				ITEMSERVER->SendItemData( pcUser, ITEMID_QuestVamp, EItemSource::QuestKill );
			}
		}
		break;

		case QUESTMONSTERID_BronzeWolverine: ITEMSERVER->SendItemData( pcUser, ITEMID_QuestWolverineTail, EItemSource::QuestKill );	break;
		case QUESTMONSTERID_SilverWolverine: ITEMSERVER->SendItemData( pcUser, ITEMID_QuestWolverineClaw, EItemSource::QuestKill );	break;
		case QUESTMONSTERID_GoldenWolverine: ITEMSERVER->SendItemData( pcUser, ITEMID_QuestWolverineHorn, EItemSource::QuestKill );	break;
	}

	return TRUE;
}

BOOL LootServer::GetRandomItemForMonster(UnitData * pcUnitData, User* pcUser, Item* psItem)
{
	//Only for game-server
	if (LOGIN_SERVER)
		return FALSE;

	int iMonsterDropId = pcUnitData->sUnitInfo.iUniqueMonsterID;
	BOOL bIsBoss = pcUnitData->sCharacterData.sMonsterClass == EMonsterClass::Boss;

	EItemSource eItemSource = bIsBoss ? EItemSource::BossKill : EItemSource::MonsterKill;

	if ( EVENTSERVER->IsEventMimicMonster( pcUnitData ) )
		eItemSource = EItemSource::MimicKill;

	//no drops in BC
	if ( pcUnitData->pcMap->pcBaseMap->iMapID == MAPID_BlessCastle )
		return FALSE;

	BaseDropDefinition* baseDropDefinition = GetRandomDropDefinition(iMonsterDropId, pcUser);

	//monster id not found
	if (baseDropDefinition == nullptr)
	{
		return FALSE;
	}

	//Note - here we can add drops related to events

	//drop type is air, return false also
	if (baseDropDefinition->eDropType == DROPTYPE_AIR)
	{
		return FALSE;
	}


	//drop type is gold
	if (baseDropDefinition->eDropType == DROPTYPE_GOLD)
	{
		GoldDropDefinition* goldDropDef = reinterpret_cast<GoldDropDefinition*>(baseDropDefinition);

		psItem->sItemID = EItemID::ITEMID_Gold;

		int iGold = Dice::RandomI(goldDropDef->iGoldMin, goldDropDef->iGoldMax);

		if (pcUser && pcUser->sBellatraSoloCrown > 0)
		{
			if (pcUser->sBellatraSoloCrown == 1 || pcUser->sBellatraSoloCrown == 4) //4 = 1st place with humor
			{
				iGold += static_cast<int>(static_cast<float>(iGold) * 0.5f); //50% extra gold
			}
			else if (pcUser->sBellatraSoloCrown == 2 || pcUser->sBellatraSoloCrown == 5) //5 = 2nd place with humor
			{
				iGold += static_cast<int>(static_cast<float>(iGold) * 0.3f); //30% extra gold
			}
			else if (pcUser->sBellatraSoloCrown == 3 || pcUser->sBellatraSoloCrown == 6) //6 = 3rd place with humor
			{
				iGold += static_cast<int>(static_cast<float>(iGold) * 0.2f); //20% extra gold
			}
		}

		psItem->iGold = iGold;

		STRINGCOPY(psItem->szItemName, FormatString("%d Gold", iGold));
		ITEMSERVER->ReformItem(psItem);

		//std::cout << "GOLD loot dropped: " << iGold << std::endl;

		return TRUE;
	}

	if (baseDropDefinition->eDropType == DROPTYPE_ITEMS)
	{
		ItemDropDefinition* itemDropDef = (ItemDropDefinition*)baseDropDefinition;

		// In LOOT_MODE, retry until we land on an acceptable item (not potion/crystal/core)
		DWORD dwItemCode = 0;
		if ( LOOT_MODE && pcUser )
		{
			int iPlayerClass = pcUser->pcUserData->sCharacterData.iClass;
			const int kMaxItemRetries = 100;
			for ( int iRetry = 0; iRetry < kMaxItemRetries; iRetry++ )
			{
				int count = itemDropDef->vItemCodes.size();
				int randomIndex = Dice::RandomI( 0, count - 1 );
				DWORD dwCandidate = itemDropDef->vItemCodes[randomIndex];
				if ( IsItemAcceptableInLootMode( dwCandidate, (ECharacterClass)iPlayerClass, pcUser ) )
				{
					dwItemCode = dwCandidate;
					break;
				}
			}

			if ( !dwItemCode )
			{
				if ( LOOTSERVER->bLootDebug )
				{
					INFO("GetRandomItemForMonster: No acceptable item found for monster");
				}
				return FALSE; // all retries exhausted, no acceptable item in group
			}
		}
		else
		{
			int count = itemDropDef->vItemCodes.size();
			int randomIndex = Dice::RandomI( 0, count - 1 );
			dwItemCode = itemDropDef->vItemCodes[randomIndex];
		}

		auto pDefItem = ITEMSERVER->FindItemDefByCode(dwItemCode);

		if (pDefItem && (pDefItem->sItem.iItemUniqueID == FALSE))
		{
			int iSpec = 0;
			int iPlayerClass = (pcUser && LOOT_MODE) ? pcUser->pcUserData->sCharacterData.iClass : 0;

			if (LOOT_MODE && pcUser)
			{
				iSpec = 100;
			}
			else
			{
				DWORD eItemBase = dwItemCode & 0xFF000000;
				DWORD eItemType = dwItemCode & 0xFFFF0000;

				if ((eItemType == ITEMTYPE_Armor || eItemType == ITEMTYPE_Boots || eItemType == ITEMTYPE_Gauntlets || eItemType == ITEMTYPE_Shield || eItemType == ITEMTYPE_Robe) ||
					(eItemBase == ITEMBASE_Weapon) ||
					(eItemType == ITEMTYPE_Bracelets || eItemType == ITEMTYPE_Orb || eItemType == ITEMTYPE_Ring || eItemType == ITEMTYPE_Ring2))
				{
					if (pDefItem->sItem.iLevel < 40)
					{
						if (Dice::RandomI(0, 99) < 50) iSpec = 100;
					}
					else if (pDefItem->sItem.iLevel < 80)
					{
						if (Dice::RandomI(0, 99) < 40) iSpec = 100;
					}
					else if (pDefItem->sItem.iLevel < 100)
					{
						if (Dice::RandomI(0, 99) < 30) iSpec = 100;
					}
				}
			}

			if (LOOT_MODE && pcUser)
			{
				ITEMSERVER->CreatePerfectItem(psItem, pDefItem, eItemSource, iPlayerClass);
				ITEMSERVER->ReformItem(psItem);
			}
			else
			{
				ITEMSERVER->CreateItem(psItem, pDefItem, eItemSource, iPlayerClass, iSpec);
			}
		}

		return TRUE;
	}


	return FALSE;



//see	        UnitServer::OnSetDrop( UserData * pcUserData, UnitData * pcUnitData )
//see also BOOL UnitServer::HandleKill( UnitData * pcUnitData, UserData * pcUserData )




}