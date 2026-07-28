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

	// ---- Shields (only Mechanician, Knight, and Atalanta) ----
	if ( eItemType == ITEMTYPE_Shield )
	{
		return ( iClass == CHARACTERCLASS_Mechanician || iClass == CHARACTERCLASS_Knight || iClass == CHARACTERCLASS_Atalanta );
	}

	// ---- Orbs (only Magician / Priestess) ----
	if ( eItemType == ITEMTYPE_Orb )
	{
		return ( iClass == CHARACTERCLASS_Magician || iClass == CHARACTERCLASS_Priestess );
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

	return true; // boots, gauntlets, bracelets, rings, amulets — no restriction
}

// LOOT_MODE: Returns true if the item is acceptable — not a potion/crystal/core,
// class-usable, passes the strict weapon/armor signature check, and is an
// ilvl upgrade over the player's currently equipped item in that slot.
bool LootServer::IsItemAcceptableInLootMode( DWORD dwItemCode, ECharacterClass iClass, User* pcUser )
{
	DWORD eItemBase = dwItemCode & 0xFF000000;
	DWORD eItemType = dwItemCode & 0xFFFF0000;

	// Skip potions, crystals, cores, premium items in LootMode
	if ( eItemBase == ITEMBASE_Potion || eItemBase == ITEMBASE_Crystal || eItemBase == ITEMBASE_Core || eItemBase == ITEMBASE_Premium )
		return false;

	// Skip monster crystals & respec jewels
	if ( eItemType == ITEMTYPE_MonsterCrystal || eItemType == ITEMTYPE_Respec)
	{
		if ( LOOTSERVER->bLootDebug )
		{
			auto pDef = ITEMSERVER->FindItemDefByCode( dwItemCode );
			INFO("IsItemAcceptableInLootMode: Rejecting monster crystal & Respec jewel %s",
				pDef ? pDef->sItem.szItemName : "unknown");
		}
		return false;
	}

	auto pDef = ITEMSERVER->FindItemDefByCode( dwItemCode );
	if ( !pDef )
	{
		if ( LOOTSERVER->bLootDebug )
		{
			INFO("IsItemAcceptableInLootMode: Rejecting unknown item code %d", dwItemCode);
		}
		return false;
	}

	// Enforce weapon/armor/orb/shield signature — the hardcoded type→class
	// mapping in IsItemAcceptableForClass is authoritative for LOOT_MODE.
	// (We intentionally do NOT use CharacterClassCanUseItem here because
	//  per-item JobBitCodeRandom database flags are often incomplete —
	//  e.g. many scythes lack the Pikeman flag despite being scythes.)
	if ( !LootServer::IsItemAcceptableForClass( dwItemCode, iClass ) )
	{
		if ( LOOTSERVER->bLootDebug )
		{
			INFO("IsItemAcceptableInLootMode: Rejecting item %s (type 0x%08X) for class %d (signature mismatch)",
				pDef->sItem.szItemName, eItemType, iClass);
		}
		return false;
	}

	// Item code upgrade check: only accept if better than currently equipped
	// or already owned. Item codes are progressive (wp120 < wp121).
	// Same code only accepted if no LEGENDARY copy exists on the player.
	if ( pcUser && pcUser->pcUserData )
	{
		EItemID eEquipped = GetEquippedItemCode( pDef, pcUser );

		// Reject if a lower code than what's equipped (also checks ilvl for sheltoms)
		if ( eEquipped && dwItemCode < (DWORD)eEquipped )
		{
			if ( LOOTSERVER->bLootDebug )
			{
				INFO("IsItemAcceptableInLootMode: Rejecting item %s (code 0x%08X < equipped code 0x%08X)",
					pDef->sItem.szItemName, dwItemCode, (DWORD)eEquipped);
			}
			return false;
		}

		// Same or higher code — check inventory for any LEGENDARY copy
		DWORD eItemType = pDef->sItem.sItemID.ToItemType();
		for ( int i = 0; i < 316; i++ )
		{
			DropItemData& dropItem = pcUser->pcUserData->sIntentoryItems[i];
			if ( !dropItem.iItemID || !dropItem.iChk1 || !dropItem.iChk2 )
				continue;

			// Only check items of the same type
			DWORD eInvType = (DWORD)dropItem.iItemID & 0xFFFF0000;
			if ( eInvType != eItemType )
				continue;

			// Only reject if we find a LEGENDARY copy of this exact item code
			if ( (DWORD)dropItem.iItemID != dwItemCode )
				continue;

			ItemLoadData itemLoadData;
			ZeroMemory( &itemLoadData, sizeof( ItemLoadData ) );

			char szCode[128] = { 0 };
			STRINGFORMAT( szCode, "%d@%d", dropItem.iChk1, dropItem.iChk2 );

			if ( ITEMSERVER->OnLoadItemData( &itemLoadData, szCode ) &&
				 itemLoadData.sItem.eRarity >= EItemRarity::LEGENDARY )
			{
				if ( LOOTSERVER->bLootDebug )
				{
					INFO("IsItemAcceptableInLootMode: Rejecting item %s (already own LEGENDARY copy)",
						pDef->sItem.szItemName);
				}
				return false;
			}
		}
		// No LEGENDARY copy found → accept
	}

	return true;
}

// Shared helper: returns the equipped EItemID for the slot matching pDef's type.
EItemID LootServer::GetEquippedItemCode( DefinitionItem* pDef, User* pcUser )
{
	if ( !pDef || !pcUser )
		return (EItemID)0;

	DWORD eItemType = pDef->sItem.sItemID.ToItemType();
	DWORD eItemBase = eItemType & 0xFF000000;

	if ( eItemBase == ITEMBASE_Weapon )
		return pcUser->eWeaponEquipped;

	switch ( eItemType )
	{
	case ITEMTYPE_Shield:		return pcUser->eShieldEquipped;
	case ITEMTYPE_Armor:		return pcUser->eArmorEquipped;
	case ITEMTYPE_Boots:		return pcUser->eBootsEquipped;
	case ITEMTYPE_Gauntlets:	return pcUser->eGauntletsEquipped;
	case ITEMTYPE_Bracelets:	return pcUser->eBraceletEquipped;
	case ITEMTYPE_Ring:			return pcUser->eRingLeftEquipped;
	case ITEMTYPE_Ring2:		return pcUser->eRingRightEquipped;
	case ITEMTYPE_Orb:			return pcUser->eOrbEquipped;
	case ITEMTYPE_Robe:			return pcUser->eRobeEquipped;
	case ITEMTYPE_Amulet:		return pcUser->eAmuletEquipped;
	case ITEMTYPE_Sheltom: 		return pcUser->eSheltomEquipped;
	default:					return (EItemID)0;
	}
}

static const int kMaxRetries = 1000;

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
						if ( LOOTSERVER->bLootDebug )
						{
							INFO("GetRandomDropDefinition: No usable items in group for monster in LOOT_MODE, retrying...");
						}
						break;
					}

					if ( LOOTSERVER->bLootDebug )
					{
						INFO("GetRandomDropDefinition: Non-item drop");
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
			for ( int iRetry = 0; iRetry < kMaxRetries; iRetry++ )
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

				// Force LEGENDARY rarity for all LOOT_MODE drops
				if (ITEMSERVER->IsItemAbleToHaveRarity(psItem) && psItem->eRarity < EItemRarity::LEGENDARY)
				{
					ServerCore::FixItemBasedOnRarity(psItem, psItem->eRarity, TRUE);     // reverse current rarity
					psItem->eRarity = EItemRarity::LEGENDARY;
					ServerCore::FixItemBasedOnRarity(psItem, EItemRarity::LEGENDARY, FALSE); // apply legendary
				}

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