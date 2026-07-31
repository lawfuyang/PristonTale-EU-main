#include "stdafx.h"
#include "ItemHandler.h"

#include "itemhandlerstruct.h"

typedef int( __cdecl *t_LoadArmorChr ) ( const char * ArmorFileInx );
t_LoadArmorChr LoadArmorCharacter = ( t_LoadArmorChr )0x00457BA0;

typedef int( __cdecl *tfnSetItemCharacter )(EItemID, DWORD dwPositionModel, BOOL bActive);
tfnSetItemCharacter fnSetItemCharacter = (tfnSetItemCharacter)0x00458FF0;

typedef BOOL( __cdecl *tfnLoadItemImage )(Item * psIn, ItemData * psOut);
tfnLoadItemImage fnLoadItemImage = (tfnLoadItemImage)0x00505D10;



ItemData *						pItemTable = NULL;

int								iRollDiceTimerID = NULL;

DWORD						g_dwDelayCristal = 0;

extern void * allocIT;

// Pivot Table
static UINT PivotTableItemChr[]
{
	// Set 110
		0x01011600, 0x01011500, // Zecram Axe
		0x01021500, 0x01021400, // Injustice Talon
		0x01031700, 0x01031600, // Bane Hammer
		0x01041700, 0x01041300, // Celestial Staff
		0x01051800, 0x01051700, // Reaper Scythe
		0x01061800, 0x01061700, // Immortal Bow
		0x01071900, 0x01071800, // Extreme Sword
		0x01081700, 0x01081600, // Heretic Javelin

		// Set 115
		0x01011700, 0x01011500, // Dragon Axe
		0x01021700, 0x01021400, // Heretic Talon
		0x01031800, 0x01031600, // Dragon Hammer
		0x01041800, 0x01041300, // Elf Staff
		0x01051900, 0x01051700, // Dragon Spear
		0x01061900, 0x01061700, // Salamander Bow
		0x01071A00, 0x01071800, // Mirage Sword
		0x01081900, 0x01081600, // Salamander Javelin
		// Set 120
		0x01011800, 0x01011500, // Kronus Axe
		0x01011900, 0x01010100, // Legend Axe
		0x01021800, 0x01021400, // Kronus Talon
		0x01031900, 0x01031600, // Kronus Hammer
		0x01041900, 0x01041300, // Kronus Staff
		0x01042A00, 0x01040100, // Legend Wand
		0x01051A00, 0x01051700, // Kronus Spear
		0x01062A00, 0x01061700, // Kronus Bow
		0x01072B00, 0x01071800, // Kronus Sword
		0x01072C00, 0x01070100, // Legend Sword
		0x01081A00, 0x01081600, // Kronus Javelin

		// Mythology Weapons
		ITEMID_MythologyAxe, 0x01011500,
		ITEMID_MythologyTalon, 0x01021400,
		ITEMID_MythologyHammer, 0x01031600,
		ITEMID_MythologyStaff, 0x01041300,
		ITEMID_MythologySpear, 0x01051700,
		ITEMID_MythologyBow, 0x01061700,
		ITEMID_MythologySword, 0x01071800,
		ITEMID_MythologyJavelin, 0x01081600,

		// (Death) Deadly Weapons
		ITEMID_DeathAxe, 0x01011500,
		ITEMID_DeathTalon, 0x01021400,
		ITEMID_DeathHammer, 0x01031600,
		ITEMID_DeathStaff, 0x01041300,
		ITEMID_DeathSpear, 0x01051700,
		ITEMID_DeathBow, 0x01061700,
		ITEMID_DeathSword, 0x01071800,
		ITEMID_DeathJavelin, 0x01081600,

		// Abyss Weapons
		ITEMID_AbyssAxe, 0x01011500,
		ITEMID_AbyssTalon, 0x01021400,
		ITEMID_AbyssHammer, 0x01031600,
		ITEMID_AbyssStaff, 0x01041300,
		ITEMID_AbyssSpear, 0x01051700,
		ITEMID_AbyssBow, 0x01061700,
		ITEMID_AbyssSword, 0x01071800,
		ITEMID_AbyssJavelin, 0x01081600,

		// Infernal Weapons
		//ITEMID_InfernalAxe, 0x01011500,
		//ITEMID_InfernalTalon, 0x01021400,
		//ITEMID_InfernalHammer, 0x01031600,
		//ITEMID_InfernalStaff, 0x01041300,
		//ITEMID_InfernalSpear, 0x01051700,
		ITEMID_InfernalBow, 0x01061700,
		//ITEMID_InfernalSword, 0x01071800,
		//ITEMID_InfernalJavelin, 0x01081600,
		//
		//// Glacial Weapons
		//ITEMID_GlacialAxe, 0x01011500,
		//ITEMID_GlacialTalon, 0x01021400,
		//ITEMID_GlacialHammer, 0x01031600,
		//ITEMID_GlacialStaff, 0x01041300,
		//ITEMID_GlacialSpear, 0x01051700,
		//ITEMID_GlacialBow, 0x01061700,
		//ITEMID_GlacialSword, 0x01071800,
		//ITEMID_GlacialJavelin, 0x01081600,

		//Lunatic
		//ITEMID_LunaticAxe, 0x01011500,
		//ITEMID_LunaticTalon, 0x01021400,
		//ITEMID_LunaticHammer, 0x01031600,
		//ITEMID_LunaticStaff, 0x01041300,
		//ITEMID_LunaticSpear, 0x01051700,
		ITEMID_LunaticBow, 0x01061700,
		ITEMID_LunaticSword, 0x01071800,
		ITEMID_LunaticPhantom, ITEMTYPE_Phantom,
		//ITEMID_LunaticJavelin, 0x01081600,

		//Amentia
		//ITEMID_AmentiaAxe, 0x01011600,
		//ITEMID_AmentiaTalon, 0x01021500,
		//ITEMID_AmentiaHammer, 0x01031700,
		//ITEMID_AmentiaStaff, 0x01041400,
		//ITEMID_AmentiaSpear, 0x01051800,
		ITEMID_AmentiaBow, 0x01061700,
		ITEMID_AmentiaSword, 0x01071800,
		ITEMID_AmentiaPhantom, ITEMTYPE_Phantom,
		//ITEMID_AmentiaJavelin, 0x01081700,

		//Star Wars Items
		//Phantoms
		//ITEMID_StarWarsPhantom, ITEMTYPE_Phantom,
		//ITEMID_StarWarsPhantom90, ITEMTYPE_Phantom,
		//ITEMID_StarWarsPhantom100, ITEMTYPE_Phantom,
		//ITEMID_StarWarsPhantom110, ITEMTYPE_Phantom,
		//
		////Daggers
		//ITEMID_StarWarsDagger, ITEMTYPE_Dagger,
		//ITEMID_StarWarsDagger90, ITEMTYPE_Dagger,
		//ITEMID_StarWarsDagger100, ITEMTYPE_Dagger,
		//ITEMID_StarWarsDagger110, ITEMTYPE_Dagger,

		//Scythe - Function as Axe animations
		ITEMID_StarWarsScythe, 0x01011500,
		ITEMID_StarWarsScythe90, 0x01011500,
		ITEMID_StarWarsScythe100, 0x01011500,
		ITEMID_StarWarsScythe110, 0x01011500,

		//Staff
		ITEMID_StarWarsStaff, 0x01041300,
		ITEMID_StarWarsStaff90, 0x01041300,
		ITEMID_StarWarsStaff100, 0x01041300,
		ITEMID_StarWarsStaff110, 0x01041300,

		//Javelins
		ITEMID_StarWarsJavelin, 0x01081600,
		ITEMID_StarWarsJavelin90, 0x01081600,
		ITEMID_StarWarsJavelin100, 0x01081600,
		ITEMID_StarWarsJavelin110, 0x01081600,

		//Bows
		ITEMID_StarWarsBow, 0x01061700,
		ITEMID_StarWarsBow90, 0x01061700,
		ITEMID_StarWarsBow100, 0x01061700,
		ITEMID_StarWarsBow110, 0x01061700,

		// 2h swords events
		ITEMID_StarWarsLightSaber2H, 0x01071800,
		ITEMID_StarWarsLightSaber2H90, 0x01071800,
		ITEMID_StarWarsLightSaber2H100, 0x01071800,
		ITEMID_StarWarsLightSaber2H110, 0x01071800,

		// 1h swords events
		ITEMID_StarWarsLightSaber1H, 0x01070100,
		ITEMID_StarWarsLightSaber1H90, 0x01070100,
		ITEMID_StarWarsLightSaber1H100, 0x01070100,
		ITEMID_StarWarsLightSaber1H110, 0x01070100,

		// Set Beginner
		0x01012E00, 0x01011500, // Kronus Axe
		0x01022D00, 0x01021400, // Kronus Talon
		0x01042F00, 0x01041300, // Kronus Staff
		0x01052E00, 0x01051700, // Kronus Spear
		0x01062F00, 0x01061700, // Kronus Bow
		0x01073100, 0x01071800, // Kronus Sword
		0x01082E00, 0x01081600, // Kronus Javelin

		//Toys
		ITEMID_Guitar, 0x01071800,
		ITEMID_PickAxe, 0x01011500,
		ITEMID_LoveSword, 0x01071800,
		ITEMID_Rose, 0x01041300,

		ITEMID_MeatCleaver, 0x01011500,
};


ItemHandler::ItemHandler()
{
	ZeroMemory( psaModelDataDefault, sizeof( EXEModelData* ) * _countof( psaModelDataDefault ) );
}


ItemHandler::~ItemHandler()
{
}

ItemData * ItemHandler::FindItemTable( UINT uPosition )
{
	if ( pItemTable )
		return &pItemTable[ uPosition ];

	return NULL;
}

ItemData * ItemHandler::FindItemTableByCode( DWORD dwCode )
{
	for ( int i = 0; i < *(int*)0x04B06B98; i++ )
	{
		ItemData * psItemData = (ItemData*)((DWORD)allocIT + (i * 0x314));

		if ( psItemData->sBaseItemID.ToInt() == dwCode )
			return psItemData;
	}
	return NULL;
}

ItemData * ItemHandler::GetMouseItemHover()
{
	if (ITEMINDEX_INVENTORY >= 0)
	{
		if ( WAREHOUSEHANDLER->IsWithdrawalOnlyWarehouse () )
		{
			return NULL;
		}

		auto invItemData = &INVENTORYITEMS[ITEMINDEX_INVENTORY];

		if (invItemData->iItemSlotFlag == ITEMSLOTFLAG_LeftRightHand &&
			invItemData->iItemSlot &&
			invItemData->bValid &&
			invItemData->sItem.iChk1 == 0 && invItemData->sItem.iChk2 == 0) //the right slot of the 2-hander weapon doesn't have a valid sItem.
		{
			return &INVENTORYITEMS[INVENTORYITEMSLOT[0].iItemIndex - 1];
		}
		else
		{
			return invItemData;
		}
	}
	auto index = (*(int*)0x03653E14);
	if (index)
	{
		if (WINDOW_ISOPEN_PERSONALSHOP_ME)
		{
			ItemData* psItemCharShop = (ItemData*)(0x0363C114); //0x03642094
			ItemData shopItem = psItemCharShop[index - 1];
			return CGameInventory::GetInstance()->GetInventoryPersonalShopItem(&shopItem);
		}
		else
		{
			ItemData* psItemCharShop = (ItemData*)(0x03642094); //other chars
			return &psItemCharShop[index - 1];
		}
	}

	// Blacksmith
	if ( WINDOW_ISOPEN_NPC_SHOP )
	{
		int iX = (*(int*)0x003653DE4);
		int iY = (*(int*)0x003653DE8);

		Rectangle2D rBlacksmith( 21, 86, 200, 200 );

		if ( rBlacksmith.Inside( MOUSEHANDLER->GetPosition() ) )
		{
			ItemData * psItemShop = (ItemData*)(0x03653E80);
			for ( int i = 0; i < 30; i++ )
			{
				if ( psItemShop[i].bValid )
				{
					if ( psItemShop[i].sPosition.iX == iX && psItemShop[i].sPosition.iY == iY )
						return &psItemShop[i];
				}
			}
		}
	}

	return NULL;
}

ItemData * ItemHandler::GetItemEquippedByType( enum EItemType eType )
{
	for( int i = 0; i < 14; i++ )
	{
		if( INVENTORYITEMSLOT[ i ].iItemIndex )
		{
			ItemData * psItemData = &INVENTORYITEMS[ INVENTORYITEMSLOT[ i ].iItemIndex - 1 ];

			if( psItemData->sBaseItemID.ToItemType() == eType )
				return psItemData;
		}
	}

	return NULL;
}

ItemData * ItemHandler::GetCurrentQuestWeapon( )
{
	for ( int i = 0; i < INVENTORY_DATACOUNT; i++ )
	{
		ItemData * ps = INVENTORYITEMS + i;
		if ( ps->bValid && ps->sItem.eCraftType == EItemCraftType::ITEMCRAFTTYPE_QuestWeapon )
			return ps;

		ps = TEMPINVENTORYITEMS + i;
		if ( ps->bValid && ps->sItem.eCraftType == EItemCraftType::ITEMCRAFTTYPE_QuestWeapon )
			return ps;
	}

	if ( ITEMHANDLER->GetMouseItem() && ITEMHANDLER->GetMouseItem()->bValid &&
		 ITEMHANDLER->GetMouseItem()->sItem.eCraftType == EItemCraftType::ITEMCRAFTTYPE_QuestWeapon )
	{
		return ITEMHANDLER->GetMouseItem();
	}

	return NULL;
}

ItemData * ItemHandler::GetItemInventoryByCode( EItemID eID )
{
	for ( int i = 0; i < INVENTORY_DATACOUNT; i++ )
	{
		ItemData * ps = INVENTORYITEMS + i;
		if ( ps->bValid && ps->sBaseItemID.ToItemID() == eID )
			return ps;

		ps = TEMPINVENTORYITEMS + i;
		if ( ps->bValid && ps->sBaseItemID.ToItemID() == eID )
			return ps;
	}

	if ( ITEMHANDLER->GetMouseItem() && ITEMHANDLER->GetMouseItem()->bValid &&
		(eID != EItemID::ITEMID_None && ITEMHANDLER->GetMouseItem()->sBaseItemID.ToItemID() == eID) )
	{
		return ITEMHANDLER->GetMouseItem();
	}

	return NULL;
}

ItemData * ItemHandler::GetItemInventoryByType( EItemType eType )
{
	if ( eType == EItemType::ITEMTYPE_None )
		return NULL;

	for ( int i = 0; i < INVENTORY_DATACOUNT; i++ )
	{
		ItemData * ps = INVENTORYITEMS + i;
		if ( ps->bValid && ps->sBaseItemID.ToItemType() == eType )
			return ps;

		ps = TEMPINVENTORYITEMS + i;
		if ( ps->bValid && ps->sBaseItemID.ToItemType() == eType )
			return ps;
	}

	if ( ITEMHANDLER->GetMouseItem() && ITEMHANDLER->GetMouseItem()->bValid &&
		(ITEMHANDLER->GetMouseItem()->sBaseItemID.ToItemType() == eType) )
	{
		return ITEMHANDLER->GetMouseItem();
	}

	return NULL;
}

/// <summary>
/// Get total items in player's inventory, including quest inventory
/// Mouse item will also be checked
/// </summary>
int ItemHandler::GetItemAmountByCode( EItemID eID, EItemCraftType eCraft )
{
	int iCount = 0;

	for ( int i = 0; i < INVENTORY_DATACOUNT; i++ )
	{
		ItemData * ps = INVENTORYITEMS + i;
		if ( ps->bValid )
		{
			if ((eID != EItemID::ITEMID_None && ps->sBaseItemID.ToItemID() == eID ) ||
				(eCraft != EItemCraftType::ITEMCRAFTTYPE_None && ps->sItem.eCraftType == eCraft))
				iCount++;
		}

		ps = TEMPINVENTORYITEMS + i;
		if ( ps->bValid )
		{
			if ((eID != EItemID::ITEMID_None && ps->sBaseItemID.ToItemID() == eID ) ||
				(eCraft != EItemCraftType::ITEMCRAFTTYPE_None && ps->sItem.eCraftType == eCraft))
				iCount++;
		}
	}

	if ( ITEMHANDLER->GetMouseItem() && ITEMHANDLER->GetMouseItem()->bValid &&
		 (eID != EItemID::ITEMID_None && ITEMHANDLER->GetMouseItem()->sBaseItemID.ToItemID() == eID  ||
		 ( eCraft != EItemCraftType::ITEMCRAFTTYPE_None && ITEMHANDLER->GetMouseItem()->sItem.eCraftType == eCraft)) )
	{
		iCount++;
	}

	return iCount;
}

int ItemHandler::RemoveItemsByCode( EItemID eID, EItemCraftType eCraft, int iTotalToRemove )
{
	int iCount = 0;
	ItemData * ps = NULL;

	for ( int i = 0; i < INVENTORY_DATACOUNT; i++ )
	{
		ps = INVENTORYITEMS + i;
		if ( ps->bValid )
		{
			if ( ( eID != EItemID::ITEMID_None && ps->sBaseItemID.ToItemID() == eID ) ||
				( eCraft != EItemCraftType::ITEMCRAFTTYPE_None && ps->sItem.eCraftType == eCraft ) )
			{
				ps->bValid = FALSE;
				ps->sItem.iChk1 = 0;

				iCount++;

				if ( iCount >= iTotalToRemove )
					return iCount;
			}
		}
	}

	for ( int i = 0; i < INVENTORY_DATACOUNT; i++ )
	{
		ps = TEMPINVENTORYITEMS + i;
		if ( ps->bValid )
		{
			if ( ( eID != EItemID::ITEMID_None && ps->sBaseItemID.ToItemID() == eID ) ||
				( eCraft != EItemCraftType::ITEMCRAFTTYPE_None && ps->sItem.eCraftType == eCraft ) )
			{
				ps->bValid = FALSE;
				ps->sItem.iChk1 = 0;

				iCount++;

				if ( iCount >= iTotalToRemove )
					return iCount;
			}
		}
	}

	if ( ITEMHANDLER->GetMouseItem() && ITEMHANDLER->GetMouseItem()->bValid &&
		(eID != EItemID::ITEMID_None && ITEMHANDLER->GetMouseItem()->sBaseItemID.ToItemID() == eID  ||
			( eCraft != EItemCraftType::ITEMCRAFTTYPE_None && ITEMHANDLER->GetMouseItem()->sItem.eCraftType == eCraft)) )
	{
		ITEMHANDLER->GetMouseItem()->bValid = FALSE;
		ITEMHANDLER->GetMouseItem()->sItem.iChk1 = 0;

		iCount++;
	}

	if ( iCount > 0 )
	{
		ValidateInventoryItems();
		CHECK_CHARACTER_CHECKSUM;
		CALLT( 0x004AE430, 0x035EBB20 );
		UpdateItemCharacterStatus();
		CALLT( 0x004AD2E0, 0x035EBB20 );
	}

	return iCount;
}

char * ItemHandler::GetArmorPatternName( class UnitData * pcUnit, DWORD dwCode )
{
	if ( dwCode == ITEMID_ChelseaSoccerTimed )
		dwCode = ITEMID_ChelseaSoccer;

	if ( dwCode == ITEMID_ManchesterSoccerTimed )
		dwCode = ITEMID_ManchesterSoccer;

	char * pszArmorName = NULL;
	for ( int i = 0; i < _countof( saCharacterBodyList ); i++ )
	{
		if ( saCharacterBodyList[i].eItemID == dwCode )
		{
			if ( saCharacterBodyList[i].iClass == 0 || saCharacterBodyList[i].iClass == pcUnit->sCharacterData.iClass )
			{
				pszArmorName = saCharacterBodyList[i].pszFileName;
				continue;
			}
		}
	}
	return pszArmorName;
}

/**
Get item aging price which is based on item original cost, aging price and if it has a spec or not
*/
int ItemHandler::GetItemAgingPrice(struct Item* pcItem, int p_CurrentAgeLevel)
{
	float originalPrice = static_cast<float>(pcItem->iSalePrice);
	float agingPrice = 0.0f;

	//Note - no need to multiple the original price because
	// it's already multiplied by 1.2f if it has a spec.
	agingPrice = (originalPrice * (p_CurrentAgeLevel + 1)) / 2.0f;

	//x 1.1 is related to BC tax
	//int cSHOP::haShopItemPrice(int Money)
	//todo - find BC tax rate and use that.
	//agingPrice *= 1.1;

	return static_cast<int>( agingPrice );
}


BOOL ItemHandler::GetItemPrice( class ItemData * pcItemData, PriceItem * psSellData )
{
	if ( !pcItemData || !psSellData )
		return FALSE;

	if ( pcItemData->iItemSlotFlag == ITEMSLOTFLAG_LeftRightHand )
	{
		if ( pcItemData->iItemSlot )
			pcItemData = &INVENTORYITEMS[INVENTORYITEMSLOT[0].iItemIndex - 1];
	}

	float fIntegrity[2];
	float fPrice;


	fPrice			= (float)pcItemData->sItem.iSalePrice;
	fIntegrity[0]	= pcItemData->sItem.sIntegrity.sCur == 0 ? 1.0f : (float)(int)pcItemData->sItem.sIntegrity.sCur;
	fIntegrity[1]	= pcItemData->sItem.sIntegrity.sMax == 0 ? 1.0f : (float)(int)pcItemData->sItem.sIntegrity.sMax;

	psSellData->iRealPrice		= (int)((fPrice*(fIntegrity[0] / fIntegrity[1])) + (fPrice - (fPrice*(fIntegrity[0] / fIntegrity[1])))*0.25);
	psSellData->iPrice			= psSellData->iRealPrice / 5;
	psSellData->iRepairPrice	= (pcItemData->sItem.iSalePrice - psSellData->iRealPrice) / 8;

	const EItemID itemID = pcItemData->sItem.sItemID.ToItemID();

	if (itemID == EItemID::ITEMID_TeleportCore ||
		itemID == EItemID::ITEMID_RebirthScroll ||
		itemID == EItemID::ITEMID_HairPotionA ||
		itemID == EItemID::ITEMID_HairPotionB ||
		itemID == EItemID::ITEMID_HairPotionC)
	{
		psSellData->iPrice = 0;
	}
	//DEBUG("psSellData->iRealPrice = %d", psSellData->iRealPrice);
	//DEBUG("psSellData->iPrice = %d", psSellData->iPrice);
	//DEBUG("psSellData->iRepairPrice = %d", psSellData->iRepairPrice);


	return TRUE;
}

NAKED void ItemHandler::DropItemToMap( ItemData * pcItemData )
{
	JMP( 0x004605B0 );
}

NAKED BOOL ItemHandler::UseItem( Item * psItem )
{
	JMP( 0x00622F30 );
}

/// <summary>
/// Delete current quest weapon if there is any
/// </summary>
BOOL ItemHandler::DeleteCurrentItemWeapon( )
{
	BOOL bFound = FALSE;

	for ( int i = 0; i < 200; i++ )
	{
		ItemData * ps = NULL;
		if ( i < 100 )
		{
			ps = INVENTORYITEMS + i;
		}
		else
		{
			ps = TEMPINVENTORYITEMS + (i - 100);
		}

		if ( ps && ps->bValid &&
			 ps->sItem.eCraftType == EItemCraftType::ITEMCRAFTTYPE_QuestWeapon )
		{
			ps->bValid = FALSE;

			bFound = TRUE;

			if ( i < 100 )
			{
				if ( ps->iItemSlot )
				{
					INVENTORYITEMSLOT[ps->iItemSlot - 1].iItemIndex = 0;
					if ( ps->iVisibleItemType )
						fnSetItemCharacter( ps->sBaseItemID.ToItemID(), ps->iVisibleItemType, FALSE );
				}
			}
		}
	}

	if ( bFound )
	{
		ValidateInventoryItems();
		CHECK_CHARACTER_CHECKSUM;
		CALLT( 0x004AE430, 0x035EBB20 );
		UpdateItemCharacterStatus();
		CALLT( 0x004AD2E0, 0x035EBB20 );
	}

	return bFound;
}

/// <summary>
/// Replace current quest weapon if there is one
/// </summary>
BOOL ItemHandler::ReplaceCurrentQuestWeapon( Item * psItem )
{
	BOOL bFound = FALSE;
	BOOL bUpdateIfFound = TRUE;

	for ( int i = 0; i < 200 && !bFound; i++ )
	{
		ItemData * ps = NULL;
		if ( i < 100 )
		{
			ps = INVENTORYITEMS + i;
		}
		else
		{
			ps = TEMPINVENTORYITEMS + (i - 100);
		}

		if ( ps && ps->bValid
			 && ps->sBaseItemID.ToItemID() == psItem->sItemID.ToItemID()
			 && ps->sItem.eCraftType == EItemCraftType::ITEMCRAFTTYPE_QuestWeapon )
		{
			//only replace item if age is higher or same
			//to prevent users abusing the system (recycle quest item)
			if ( psItem->sAgeLevel >= ps->sItem.sAgeLevel )
			{
				CopyMemory( &ps->sItem, psItem, sizeof( Item ) );
				bUpdateIfFound = FALSE;
			}

			psItem->iChk1 = 0;
			bFound = TRUE;
		}
	}

	if ( bFound )
	{
		ITEMINFOBOX->Invalidate();
		ITEMHANDLER->ResetInventoryItemChecksum ();
		CHARACTERGAME->UpdateWeight();
		ITEMHANDLER->UpdateItemCharacterStatus();
	}

	return bFound;
}

/// <summary>
/// Delete quest item
/// keywords: ItemQuest QuestItem DeleteItem
/// </summary>
int ItemHandler::DeleteItemQuest( EItemID eID, int iCount )
{
	int iCountDeleted = 0;
	for ( int i = 0; i < 200; i++ )
	{
		ItemData * ps = NULL;
		if ( i < 100 )
		{
			ps = INVENTORYITEMS + i;
		}
		else
		{
			ps = TEMPINVENTORYITEMS + (i - 100);
		}
		if ( ps && ps->bValid && ps->sBaseItemID.ToItemID() == eID )
		{
			ps->bValid = FALSE;
			if ( i < 100 )
			{
				if ( ps->iItemSlot )
				{
					INVENTORYITEMSLOT[ps->iItemSlot - 1].iItemIndex = 0;
					if ( ps->iVisibleItemType )
						fnSetItemCharacter( ps->sBaseItemID.ToItemID(), ps->iVisibleItemType, FALSE );
				}
			}

			iCountDeleted++;
			if ( iCount != 0 )
			{
				if ( iCountDeleted == iCount )
					break;
			}
		}
	}

	if ( iCountDeleted > 0 )
	{
		ValidateInventoryItems();
		CHECK_CHARACTER_CHECKSUM;
		CALLT( 0x004AE430, 0x035EBB20 );
		UpdateItemCharacterStatus();
		CALLT( 0x004AD2E0, 0x035EBB20 );
	}

	return iCountDeleted;
}

//See also BOOL CGameInventory::DeleteItem( int itemCode )
void ItemHandler::DeleteItem( ItemData * psItemData )
{
	psItemData->bValid = FALSE;
	if ( psItemData->iItemSlot )
	{
		INVENTORYITEMSLOT[psItemData->iItemSlot - 1].iItemIndex = 0;
		if ( psItemData->iVisibleItemType )
			fnSetItemCharacter( psItemData->sBaseItemID.ToItemID(), psItemData->iVisibleItemType, FALSE );
	}
	ValidateInventoryItems();
	CHECK_CHARACTER_CHECKSUM;
	CALLT( 0x004AE430, 0x035EBB20 );
	UpdateItemCharacterStatus();
	CALLT( 0x004AD2E0, 0x035EBB20 );
}


void ItemHandler::SetCharacterItem( EItemID iItemID, int iPosition, BOOL bActive )
{
	fnSetItemCharacter( iItemID, iPosition, bActive );
}

void ItemHandler::CheckPuzzleItemEvent()
{
	CALLT( 0x004B1810, 0x035EBB20 );
}

void ItemHandler::CheckRequirements()
{
	CALLT( 0x004BA670, 0x035EBB20 );
}

BOOL ItemHandler::LoadItemImage( Item * psIn, ItemData * psOut )
{
	return fnLoadItemImage( psIn, psOut );
}

void ItemHandler::OnLoadArmorCharacter()
{
	// Load Armor List
	for ( int i = 0; i < _countof( saCharacterBodyList ); i++ )
		LoadArmorCharacter( saCharacterBodyList[ i ].pszFileName );
}

/// <summary>
/// Leaked code: cINVENTORY::CheckDamage()
/// </summary>
void ItemHandler::CheckDamageItems()
{
	CALLT( 0x004AFC10, 0x035EBB20 );
}

/// <summary>
/// See int cINVENTORY::SetItemToChar(int CheckHackFlag) in leaked code
/// It also calculates damage, absorbs, weight, defense etc
/// It gets called a lot from all over the places in ASM
/// </summary>
void ItemHandler::UpdateItemCharacterStatus( BOOL bHackCheck )
{
	typedef void( __thiscall *tfnUpdateCharacterItemStatus )(DWORD dwClass, BOOL b);
	tfnUpdateCharacterItemStatus fnUpdateCharacterItemStatus = (tfnUpdateCharacterItemStatus)0x004BC8C0;
	fnUpdateCharacterItemStatus( 0x035EBB20, bHackCheck );
}

int ItemHandler::OnItemPivot( int iPosition, UINT uItemID )
{
	int iSizeList = ( sizeof( PivotTableItemChr ) / 4 );

	if ( iPosition > *( int* )0x04B06B98 )
		return 0;

	int ItemPosition = iPosition * 0x314;

	if ( (uItemID & 0xFFFF0000) == 0x010A0000 )
	{
		if ( (*(UINT*)((UINT)allocIT + ItemPosition) & 0xFFFF0000) == 0x010A0000 )
			return 1;
	}

	for ( int i = 0; i < iSizeList; i += 2 )
	{
		if ( PivotTableItemChr[ i ] == uItemID )
		{
			int iNextItem = PivotTableItemChr[ i + 1 ];
			if ( *( UINT* )( ( UINT )allocIT + ItemPosition ) == iNextItem )
				return 1;
		}
	}

	if ( *( UINT* )( ( UINT )allocIT + ItemPosition ) == uItemID )
		return 1;

	return 0;
}

BOOL ItemHandler::OnSetArmorUnit( UnitData * psUnit, UINT uItemCode )
{
	// Set item code to handler address
	( *( UINT* )0x0209EA44 ) = uItemCode;

	// Character data
	int iJobCode = psUnit->sCharacterData.iClass - 1;
	char * pszHeadCharacter = psUnit->sCharacterData.Player.szHeadModel;
	char * pszArmorName = GetArmorPatternName( psUnit, uItemCode );

	// Set Armor
	if ( pszArmorName )
	{
		psUnit->bNoMove = TRUE;
		SETPLAYERPATTERN( psUnit, pszArmorName, pszHeadCharacter );
		return TRUE;
	}
	return FALSE;
}

BOOL ItemHandler::OnSetArmorTargetUnit( UnitData * psUnit, UINT uPosition )
{
	ItemData * pItem = FindItemTable( uPosition );

	if ( pItem )
		return OnSetArmorUnit( psUnit, pItem->sBaseItemID.ToInt() );

	return FALSE;
}

/**
Return item level formatted string, including letter (A, B, C) where appropriate
*/
const char* ItemHandler::GetItemLevelWithLetter(ItemData* psItemData)
{
	//auto itemType = psItemData->sItem.sItemID.ToItemType();

	//const int itemIdLow = psItemData->sItem.sItemID.iIDLow;
	//std::string letter = "";

	//Only lvl 90 items has letters (A, B, C)
	//if (psItemData->sItem.iLevel >= 90)
	//{
	//	//...18, ...19, ...20
	//	if (itemType == ITEMTYPE_Claw ||
	//		itemType == ITEMTYPE_Axe ||
	//		itemType == ITEMTYPE_Shield)
	//	{
	//		if (itemIdLow == 18)
	//			letter = "A";
	//		else if (itemIdLow == 19)
	//			letter = "B";
	//		else if (itemIdLow == 20)
	//			letter = "C";
	//	}

	//	//...19, ...20, ...21
	//	else if (itemType == ITEMTYPE_Orb ||
	//		itemType == ITEMTYPE_Dagger ||
	//		itemType == ITEMTYPE_Hammer ||
	//		itemType == ITEMTYPE_Wand ||
	//		itemType == ITEMTYPE_Phantom ||
	//		itemType == ITEMTYPE_Scythe ||
	//		itemType == ITEMTYPE_Javelin)
	//	{
	//		if (itemIdLow == 19)
	//			letter = "A";
	//		else if (itemIdLow == 20)
	//			letter = "B";
	//		else if (itemIdLow == 21)
	//			letter = "C";
	//	}

	//	//...20, ...21, ...22
	//	else if (itemType == ITEMTYPE_Bow ||
	//		itemType == ITEMTYPE_Robe ||
	//		itemType == ITEMTYPE_Bow ||
	//		itemType == ITEMTYPE_Armor)
	//	{
	//		if (itemIdLow == 20)
	//			letter = "A";
	//		else if (itemIdLow == 21)
	//			letter = "B";
	//		else if (itemIdLow == 22)
	//			letter = "C";
	//	}

	//	//...21, ...22, ...23
	//	else if (itemType == ITEMTYPE_Sword)
	//	{
	//		if (itemIdLow == 21)
	//			letter = "A";
	//		else if (itemIdLow == 22)
	//			letter = "B";
	//		else if (itemIdLow == 23)
	//			letter = "C";
	//	}

	//	if (letter.size() > 0)
	//		return FormatString("%d (%s)", psItemData->sItem.iLevel, letter.c_str());
	//}

	return FormatString("%d", psItemData->sItem.iLevel);
}

ECharacterGender ItemHandler::GetItemGender( int iItemID )
{
	ECharacterGender e = ECharacterGender::CHARACTERGENDER_None;

	switch ( iItemID )
	{
		//Male
		case ITEMID_WeddingSuit:
		case ITEMID_WeddingSuit_Black_Gold:
		case ITEMID_WarriorsHanbokM:
		case ITEMID_KimonoCostumeM:
		case ITEMID_ChineseSuitM:
		case ITEMID_RomanticSuit:
		case ITEMID_ChristmasElegantCostumeM:
		case ITEMID_PermanentChristmasCloatCostumeTM:
		case ITEMID_EasterHopyCostume:
		case ITEMID_PartyCostumeMale:
		case ITEMID_IdasCostumeMale:
		case ITEMID_ThalesArmorMale:
		case ITEMID_Costume4ArmorMale:
		case ITEMID_Costume5ArmorMale:
		case ITEMID_WingArmorMale:
		case ITEMID_DarkWingArmorMale:
		case ITEMID_SummerClotheMale:
		case ITEMID_SpringCostumeMale:
		case ITEMID_YellowSpringCostumeMale:
		case ITEMID_Costume11ArmorMale:
		case ITEMID_BlackSuit:
		case ITEMID_Costume13ArmorMale:
		case ITEMID_CasualSchoolUniformMale:
		case ITEMID_ElegantSchoolUniformMale:
		case ITEMID_Costume16ArmorMale:
		case ITEMID_MarineCostumeMale:
		case ITEMID_HopyCostume:
		case ITEMID_GaiaArmorMale:
		case ITEMID_DarkIriaRobeMale:
		case ITEMID_DarkGaiaArmorMale:
		case ITEMID_DarkSeasonArmorMale:
		case ITEMID_DarkSeasonRobeMale:
		case ITEMID_IriaRobeMale:
		case ITEMID_MikalRobeMale:
		case ITEMID_MartanosRobeMale:
		case ITEMID_SilverCostumeMale:
		case ITEMID_VampireSuit:
		case ITEMID_SkeletonSuit:
		case ITEMID_PumpkinSuit:
		case ITEMID_CartolaHat:
		case ITEMID_SummerClotheTimedMale:
		case ITEMID_VampireSuit_NoTime:
			e = CHARACTERGENDER_Male;
			break;

		//Female
		case ITEMID_RomanticDress:
		case ITEMID_DarkSeasonRobeFemale:
		case ITEMID_DarkSeasonArmorFemale:
		case ITEMID_ChristmasElegantCostumeF:
		case ITEMID_PermanentChristmasCloatCostumeTF:
		case ITEMID_EasterCuepyCostume:
		case ITEMID_PartyCostumeFemale:
		case ITEMID_IdasCostumeFemale:
		case ITEMID_ThalesArmorFemale:
		case ITEMID_Costume4ArmorFemale:
		case ITEMID_Costume5ArmorFemale:
		case ITEMID_WingArmorFemale:
		case ITEMID_DarkWingArmorFemale:
		case ITEMID_SummerClotheFemale:
		case ITEMID_SpringCostumeFemale:
		case ITEMID_YellowSpringCostumeFemale:
		case ITEMID_Costume11ArmorFemale:
		case ITEMID_WhiteDress:
		case ITEMID_Costume13ArmorFemale:
		case ITEMID_CasualSchoolUniformFemale:
		case ITEMID_ElegantSchoolUniformFemale:
		case ITEMID_Costume16ArmorFemale:
		case ITEMID_MarineCostumeFemale:
		case ITEMID_CuepyCostume:
		case ITEMID_GaiaArmorFemale:
		case ITEMID_IriaRobeFemale:
		case ITEMID_DarkGaiaArmorFemale:
		case ITEMID_DarkIriaRobeFemale:
		case ITEMID_WeddingDress:
		case ITEMID_WeddingDress_Black_Gold:
		case ITEMID_WarriorsHanbokF:
		case ITEMID_KimonoCostumeF:
		case ITEMID_ChineseSuitF:
		case ITEMID_MikalRobeFemale:
		case ITEMID_MartanosRobeFemale:
		case ITEMID_SilverCostumeFemale:
		case ITEMID_Succubus:
		case ITEMID_WitchHat:
		case ITEMID_SkeletonDress:
		case ITEMID_PumpkinDress:
		case ITEMID_SummerClotheTimedFemale:
		case ITEMID_Succubus_NoTime:
			e = CHARACTERGENDER_Female;
			break;

		default:
			break;
	}

	if ( e == CHARACTERGENDER_None )
	{
		switch ( iItemID )
		{
			case ITEMID_ChineseSuitF:
			case ITEMID_ManchesterSoccer:
			case ITEMID_ManchesterSoccerTimed:
			case ITEMID_ChelseaSoccer:
			case ITEMID_ChelseaSoccerTimed:
				e = UNITDATA->GetCharacterGender();
				break;
		}
	}

	return e;
}

BOOL ItemHandler::CanEquipItem( void * pcClassItemData, ItemData * pcItemData ) // CanEquipItem ITEMTYPE_ORB - Chaos
{
	int iJobCode = UNITDATA->sCharacterData.iClass;
	EItemID iItemID = pcItemData->sItem.sItemID.ToItemID();
	int iGender = GetItemGender( iItemID );

	int iMask = iItemID & 0xFFFF0000;

	// TRUE = Can't use the item
	BOOL bNotUse	= FALSE;

	switch ( iMask )
	{
		case ITEMTYPE_Orb:
		case ITEMTYPE_Robe:
			if ( iJobCode != CHARACTERCLASS_Priestess && iJobCode != CHARACTERCLASS_Magician && iJobCode != CHARACTERCLASS_Shaman )
			{
				bNotUse = TRUE;
				if ( iItemID == ITEMID_SoccerShirt )
					bNotUse = FALSE;
			}
			if ( iItemID == ITEMID_SoccerShirt )
			{
				ItemData * psItemData = GetItemEquippedByType( ITEMTYPE_Costume );
				if ( psItemData && psItemData->bValid )
				{
					bNotUse = TRUE;
				}
				else if ( (psItemData = GetItemEquippedByType( ITEMTYPE_Costume2 )) && psItemData->bValid )
				{
					bNotUse = TRUE;
				}
				else if ( (psItemData = GetItemEquippedByType( ITEMTYPE_Costume3 )) && psItemData->bValid )
				{
					bNotUse = TRUE;
				}
				else if ( (psItemData = GetItemEquippedByType( ITEMTYPE_Costume4 )) && psItemData->bValid )
				{
					bNotUse = TRUE;
				}

				if ( bNotUse )
				{
					psItemData = GetItemEquippedByType( ITEMTYPE_Robe );
					if ( !psItemData || psItemData->sItem.sItemID.ToItemID() != ITEMID_SoccerShirt )
						bNotUse = FALSE;
				}
			}
			break;

		case ITEMTYPE_Armor:
			if ( iJobCode == CHARACTERCLASS_Priestess || iJobCode == CHARACTERCLASS_Magician || iJobCode == CHARACTERCLASS_Shaman )
			{
				bNotUse = TRUE;
				// This items can be used by the classes
				if ( iItemID == ITEMID_ChristmasCloatCostumeTM && (iJobCode == CHARACTERCLASS_Magician || iJobCode == CHARACTERCLASS_Shaman) )
					bNotUse = FALSE;
				if ( iItemID == ITEMID_ChristmasCloatCostumeTF && (iJobCode == CHARACTERCLASS_Priestess) )
					bNotUse = FALSE;

				// This items can be used by the classes
				if (iItemID == ITEMID_PermanentChristmasCloatCostumeTM && (iJobCode == CHARACTERCLASS_Magician || iJobCode == CHARACTERCLASS_Shaman))
					bNotUse = FALSE;
				if (iItemID == ITEMID_PermanentChristmasCloatCostumeTF && (iJobCode == CHARACTERCLASS_Priestess))
					bNotUse = FALSE;

				if ( iJobCode == CHARACTERCLASS_Magician || iJobCode == CHARACTERCLASS_Shaman )
				{
					if ( iItemID == ITEMID_WeddingSuit ||
						iItemID == ITEMID_WeddingSuit_Black_Gold ||
						iItemID == ITEMID_WarriorsHanbokM ||
						iItemID == ITEMID_KimonoCostumeM ||
						iItemID == ITEMID_ChineseSuitM ||
						iItemID == ITEMID_SilverCostumeMale ||
						iItemID == ITEMID_ChineseSuitF )
						bNotUse = FALSE;
				}

				if ( iJobCode == CHARACTERCLASS_Priestess )
				{
					if ( iItemID == ITEMID_WeddingDress ||
						iItemID == ITEMID_WeddingDress_Black_Gold ||
						iItemID == ITEMID_WarriorsHanbokF ||
						iItemID == ITEMID_KimonoCostumeF ||
						iItemID == ITEMID_ChineseSuitM ||
						iItemID == ITEMID_SilverCostumeFemale ||
						iItemID == ITEMID_ChineseSuitF )
						bNotUse = FALSE;
				}
			}

			if ( bNotUse == FALSE )
			{
				if ( iGender != CHARACTERGENDER_None && UNITDATA->GetCharacterGender() != iGender )
					bNotUse = TRUE;
			}
			break;

		case ITEMTYPE_Shield:
			if ( /*iJobCode == CHARACTERCLASS_Priestess || iJobCode == CHARACTERCLASS_Magician || */ iJobCode == CHARACTERCLASS_Shaman || iJobCode == CHARACTERCLASS_Assassin ) // Edit by Chaos ( Casters can now use Shields )
				bNotUse = TRUE;
			break;

		case ITEMTYPE_Dagger:
			if ( iJobCode != CHARACTERCLASS_Assassin )
				bNotUse = TRUE;
			break;

		case ITEMTYPE_Phantom:
			if ( iJobCode != CHARACTERCLASS_Shaman )
				bNotUse = TRUE;
			break;

		case ITEMTYPE_Costume:
		case ITEMTYPE_Costume2:
		case ITEMTYPE_Costume3:
		case ITEMTYPE_Costume4:
			if ( iGender != CHARACTERGENDER_None && UNITDATA->GetCharacterGender() != iGender )
				bNotUse = TRUE;

			if ( bNotUse == FALSE )
			{
				if ( (iItemID == ITEMID_MikalRobeMale || iItemID == ITEMID_MikalRobeFemale || //MAGIC CLASSES CAN USE
					iItemID == ITEMID_MartanosRobeMale || iItemID == ITEMID_MartanosRobeFemale) )
				{
					if ( iJobCode != CHARACTERCLASS_Priestess && iJobCode != CHARACTERCLASS_Magician && iJobCode != CHARACTERCLASS_Shaman )
						bNotUse = TRUE;
				}
				else if ( (iItemID == ITEMID_IdasCostumeMale || iItemID == ITEMID_IdasCostumeFemale || //MAGIC CLASSES CANNOT USE
					iItemID == ITEMID_ThalesArmorMale || iItemID == ITEMID_ThalesArmorFemale) )
				{
					if ( iJobCode == CHARACTERCLASS_Priestess || iJobCode == CHARACTERCLASS_Magician || iJobCode == CHARACTERCLASS_Shaman )
						bNotUse = TRUE;
				}
			}

			break;

		default:
			break;
	}

	if ( bNotUse )
		pcItemData->sItem.bCanNotUse = TRUE;

	return TRUE;
}

BOOL ItemHandler::EquipItem( UnitData * pcUnitData, EItemID iItemID, int iPositionID )
{
	return (BOOL)CALLT_WITH_ARG2( 0x00433990, (DWORD)pcUnitData, iItemID, iPositionID );
}

BOOL ItemHandler::EquipBody( UnitData * pcUnitData, int iPositionID )
{
	return (BOOL)CALL_WITH_ARG2( 0x00458340, (DWORD)pcUnitData, iPositionID );
}

BOOL ItemHandler::SetHairModel( DWORD dwCode, int iRank )
{
	HeadModelStruct * pHead				= NULL;
	HeadModelStruct * psaHeadModelList	= NULL;

	int iArrayCount = 0;

	switch ( UNITDATA->sCharacterData.iClass )
	{
		case CHARACTERCLASS_Fighter:
			iArrayCount			= _countof( saFighterHeadModel );
			psaHeadModelList	= saFighterHeadModel;
			break;

		case CHARACTERCLASS_Mechanician:
			iArrayCount			= _countof( saMechanicianHeadModel );
			psaHeadModelList	= saMechanicianHeadModel;
			break;

		case CHARACTERCLASS_Archer:
			iArrayCount			= _countof( saArcherHeadModel );
			psaHeadModelList	= saArcherHeadModel;
			break;

		case CHARACTERCLASS_Pikeman:
			iArrayCount			= _countof( saPikemanHeadModel );
			psaHeadModelList	= saPikemanHeadModel;
			break;

		case CHARACTERCLASS_Atalanta:
			iArrayCount			= _countof( saAtalantaHeadModel );
			psaHeadModelList	= saAtalantaHeadModel;
			break;

		case CHARACTERCLASS_Knight:
			iArrayCount			= _countof( saKnightHeadModel );
			psaHeadModelList	= saKnightHeadModel;
			break;

		case CHARACTERCLASS_Magician:
			iArrayCount			= _countof( saMagicianHeadModel );
			psaHeadModelList	= saMagicianHeadModel;
			break;

		case CHARACTERCLASS_Priestess:
			iArrayCount			= _countof( saPriestessHeadModel );
			psaHeadModelList	= saPriestessHeadModel;
			break;

		case CHARACTERCLASS_Assassin:
			iArrayCount			= _countof( saAssassinHeadModel );
			psaHeadModelList	= saAssassinHeadModel;
			break;

		case CHARACTERCLASS_Shaman:
			iArrayCount			= _countof( saShamanHeadModel );
			psaHeadModelList	= saShamanHeadModel;
			break;

		default:
			break;
	}

	for ( int i = 0; i < iArrayCount; i++ )
	{
		// matching sItemID?
		if ( psaHeadModelList[i].dwCode == dwCode )
		{
			// No need tier? save it
			if ( psaHeadModelList[i].dwTier == 0 )
			{
				pHead = &psaHeadModelList[i];
			}

			// Found head model with head band? Set pointer and stop loop list
			if ( psaHeadModelList[i].dwTier && psaHeadModelList[i].dwTier == iRank )
			{
				pHead = &psaHeadModelList[i];
				break;
			}
		}
	}

	// Found head model?
	if ( pHead )
	{
		// Set to character model
		UNITGAME->SetCharacterHeadModel( pHead->pszFile, TRUE );
		return TRUE;
	}

	int iNum = -1;
	switch ( dwCode )
	{
		case ITEMID_HairPotionA:
			iNum = 1;
			break;
		case ITEMID_HairPotionB:
			iNum = 2;
			break;
		case ITEMID_HairPotionC:
			iNum = 3;
			break;
		case ITEMID_HairPotionD:
			iNum = 4;
			break;
		case ITEMID_HairPotionE:
			iNum = 5;
			break;
		case ITEMID_HairPotionF:
			iNum = 6;
			break;
		case ITEMID_HairPotionG:
			iNum = 7;
			break;
		case ITEMID_HairPotionH:
			iNum = 8;
			break;
		case ITEMID_HairPotionI:
			iNum = 9;
			break;
		case ITEMID_HairPotionJ:
			iNum = 10;
			break;
		case ITEMID_HairPotionK:
			iNum = 11;
			break;
		case ITEMID_HairPotionL:
			iNum = 12;
			break;
		case ITEMID_HairPotionM:
			iNum = 13;
			break;
		case ITEMID_HairPotionN:
			iNum = 17;
			break;
		case ITEMID_HairPotionO:
			iNum = 18;
			break;
		case ITEMID_HairPotionP:
			iNum = 19;
			break;
		case ITEMID_HairPotionQ:
			iNum = 20;
			break;
		case ITEMID_HairPotionR:
			iNum = 21;
			break;
		default:
			break;
	}

	if ( iNum != -1 )
	{
		if ( iNum == 1 )
		{
			CHARACTERGAME->ResetHead( TRUE );
			SKILLMANAGERHANDLER->StartSkillEffect( UNITDATA->sPosition.iX, UNITDATA->sPosition.iY, UNITDATA->sPosition.iZ, 0, 0, 0, SKILL_UP1 );
			SKILLMANAGERHANDLER->SkillPlaySound( 0x1000, UNITDATA->sPosition.iX, UNITDATA->sPosition.iY, UNITDATA->sPosition.iZ );
			return TRUE;
		}

		BOOL bResult = (BOOL)CALLT_WITH_ARG1( 0x00494630, 0x035D0350, iNum );
		return bResult;
	}
	return FALSE;
}

EItemID ItemHandler::GetHairPotionIDFromPath( char * path )
{
	HeadModelStruct* psaHeadModelList = NULL;

	int iArrayCount = 0;

	switch (UNITDATA->sCharacterData.iClass)
	{
	case CHARACTERCLASS_Fighter:
		iArrayCount = _countof(saFighterHeadModel);
		psaHeadModelList = saFighterHeadModel;
		break;

	case CHARACTERCLASS_Mechanician:
		iArrayCount = _countof(saMechanicianHeadModel);
		psaHeadModelList = saMechanicianHeadModel;
		break;

	case CHARACTERCLASS_Archer:
		iArrayCount = _countof(saArcherHeadModel);
		psaHeadModelList = saArcherHeadModel;
		break;

	case CHARACTERCLASS_Pikeman:
		iArrayCount = _countof(saPikemanHeadModel);
		psaHeadModelList = saPikemanHeadModel;
		break;

	case CHARACTERCLASS_Atalanta:
		iArrayCount = _countof(saAtalantaHeadModel);
		psaHeadModelList = saAtalantaHeadModel;
		break;

	case CHARACTERCLASS_Knight:
		iArrayCount = _countof(saKnightHeadModel);
		psaHeadModelList = saKnightHeadModel;
		break;

	case CHARACTERCLASS_Magician:
		iArrayCount = _countof(saMagicianHeadModel);
		psaHeadModelList = saMagicianHeadModel;
		break;

	case CHARACTERCLASS_Priestess:
		iArrayCount = _countof(saPriestessHeadModel);
		psaHeadModelList = saPriestessHeadModel;
		break;

	case CHARACTERCLASS_Assassin:
		iArrayCount = _countof(saAssassinHeadModel);
		psaHeadModelList = saAssassinHeadModel;
		break;

	case CHARACTERCLASS_Shaman:
		iArrayCount = _countof(saShamanHeadModel);
		psaHeadModelList = saShamanHeadModel;
		break;

	default:
		break;
	}

	for (int i = 0; i < iArrayCount; i++)
	{
		// Matching path?
		if (STRINGCOMPAREI(psaHeadModelList[i].pszFile, path))
		{
			return (EItemID)psaHeadModelList[i].dwCode;
		}
	}

	return EItemID::ITEMID_None;
}

char * ItemHandler::GetHeadModel( DWORD dwCode, int iRank )
{
	HeadModelStruct * pHead = NULL;
	HeadModelStruct * psaHeadModelList	= NULL;

	int iArrayCount = 0;

	switch ( UNITDATA->sCharacterData.iClass )
	{
		case CHARACTERCLASS_Fighter:
			iArrayCount			= _countof( saFighterHeadModel );
			psaHeadModelList	= saFighterHeadModel;
			break;

		case CHARACTERCLASS_Mechanician:
			iArrayCount			= _countof( saMechanicianHeadModel );
			psaHeadModelList	= saMechanicianHeadModel;
			break;

		case CHARACTERCLASS_Archer:
			iArrayCount			= _countof( saArcherHeadModel );
			psaHeadModelList	= saArcherHeadModel;
			break;

		case CHARACTERCLASS_Pikeman:
			iArrayCount			= _countof( saPikemanHeadModel );
			psaHeadModelList	= saPikemanHeadModel;
			break;

		case CHARACTERCLASS_Atalanta:
			iArrayCount			= _countof( saAtalantaHeadModel );
			psaHeadModelList	= saAtalantaHeadModel;
			break;

		case CHARACTERCLASS_Knight:
			iArrayCount			= _countof( saKnightHeadModel );
			psaHeadModelList	= saKnightHeadModel;
			break;

		case CHARACTERCLASS_Magician:
			iArrayCount			= _countof( saMagicianHeadModel );
			psaHeadModelList	= saMagicianHeadModel;
			break;

		case CHARACTERCLASS_Priestess:
			iArrayCount			= _countof( saPriestessHeadModel );
			psaHeadModelList	= saPriestessHeadModel;
			break;

		case CHARACTERCLASS_Assassin:
			iArrayCount			= _countof( saAssassinHeadModel );
			psaHeadModelList	= saAssassinHeadModel;
			break;

		case CHARACTERCLASS_Shaman:
			iArrayCount			= _countof( saShamanHeadModel );
			psaHeadModelList	= saShamanHeadModel;
			break;

		default:
			break;
	}

	for ( int i = 0; i < iArrayCount; i++ )
	{
		// matching sItemID?
		if (psaHeadModelList[i].dwCode == dwCode)
		{
			// No need tier? save it
			if (psaHeadModelList[i].dwTier == 0)
			{
				pHead = &psaHeadModelList[i];
			}

			// Found head model with head band? Set pointer and stop loop list
			if (psaHeadModelList[i].dwTier && psaHeadModelList[i].dwTier == iRank)
			{
				pHead = &psaHeadModelList[i];
				break;
			}
		}
	}

	if (pHead)
	{
		return pHead->pszFile;
	}

	return NULL;
}

char * ItemHandler::OnDescriptionItem( DWORD dwCode )
{
	return NULL;
}

void ItemHandler::ValidateInventoryItems()
{
	typedef void( __thiscall *tfnValidateInventory )(DWORD pcClass);
	tfnValidateInventory ValidateInventory = (tfnValidateInventory)0x004AE020;
	ValidateInventory( 0x035EBB20 );
}

void ItemHandler::ResetInventoryItemChecksum()
{
	CALL( 0x0045D760 );
}

void ItemHandler::ResetItemMouse()
{
	CALLT( 0x004BB690, 0x035EBB20 );
}

void ItemHandler::HandleQuestFinish( EQuestID eQuestID )
{
	if ( eQuestID == QUESTID_Rankup_2_Tempskron )
	{
		ITEMHANDLER->DeleteItemQuest( ITEMID_QuestWolverineClaw );
		ITEMHANDLER->DeleteItemQuest( ITEMID_QuestWolverineTail );
		ITEMHANDLER->DeleteItemQuest( ITEMID_QuestWolverineHorn );
	}

	else if ( eQuestID == QUESTID_Rankup_2_Morion_3 || eQuestID == QUESTID_Rankup_2_Morion_4 )
	{
		ITEMHANDLER->DeleteItemQuest( ITEMID_QuestEmptyBottle );
		ITEMHANDLER->DeleteItemQuest( ITEMID_QuestHoneyQuest );
		ITEMHANDLER->DeleteItemQuest( ITEMID_QuestRoyalJelly );
	}

	else if ( eQuestID == QUESTID_Rankup_3_Morion_Vamp || eQuestID == QUESTID_Rankup_3_Tempskron_Vamp )
	{
		ITEMHANDLER->DeleteItemQuest( ITEMID_QuestVamp );
	}
}

#define ITEM_KIND_SPECIAL 8

void ItemHandler::HandleQuestItem( Item * psItem )
{
	auto itemType = psItem->sItemID.ToItemType();
	auto itemID = psItem->sItemID.ToItemID();

	//quest item filtering
	//as existing quest monsters will always send old quest items
	//we don't want them to end up in the quest inventory
	if (psItem->eCraftType != EItemCraftType::ITEMCRAFTTYPE_QuestWeapon
		&& psItem->eCraftType != EItemCraftType::ITEMCRAFTTYPE_Quest
		&& itemType != ITEMTYPE_Wing
		&& itemID != ITEMID_TearsOfKalia //lvl 85 quest
		&& itemID != ITEMID_Roitra //level 90 quest
		&& itemID != ITEMID_QuestHoneyQuest
		&& itemID != ITEMID_QuestRecommendationLetter
		&& itemID != ITEMID_QuestEmptyBottle
		&& itemID != ITEMID_QuestRoyalJelly
		&& itemID != ITEMID_QuestVamp
		&& itemID != ITEMID_QuestWolverineClaw
		&& itemID != ITEMID_QuestWolverineHorn
		&& itemID != ITEMID_QuestWolverineTail)
	{
#ifdef DEV_MODE
		CHATBOX->AddDebugMessage( "Quest item ignored: %s", psItem->szItemName );
#endif
		return;
	}

	int iMaxItemCount = 1;

	//ignore wolverine tails if not doing tempskron quest
	if ( itemID == ITEMID_QuestWolverineClaw || itemID == ITEMID_QuestWolverineHorn || itemID == ITEMID_QuestWolverineTail )
	{
		if ( !CQUESTGAME->GetQuestInstance()->IsActiveQuest( QUESTID_Rankup_2_Tempskron ) )
			return;
	}

	if ( psItem->sItemID.ToItemID() == ITEMID_QuestHoneyQuest )
	{
		if ( !CQUESTGAME->GetQuestInstance()->IsActiveQuest( QUESTID_Rankup_2_Morion_2 ) )
			return;

		iMaxItemCount = CQUESTGAME->GetQuestInstance()->GetQuest( QUESTID_Rankup_2_Morion_2 )->GetItemsReqByItemID( ITEMID_QuestHoneyQuest );
	}

	if ( itemID == ITEMID_QuestVamp )
	{
		if ( !CQUESTGAME->GetQuestInstance()->IsActiveQuest( QUESTID_Rankup_3_Tempskron_Vamp ) )
			return;

		if ( !CQUESTGAME->GetQuestInstance()->IsActiveQuest( QUESTID_Rankup_3_Morion_Vamp ) )
			return;
	}



	//special case for wing, delete lower level wings
	if ( itemType == ITEMTYPE_Wing )
	{
		switch ( itemID )
		{
			case ITEMID_Wing_Imperial:		ITEMHANDLER->DeleteItemQuest( ITEMID_Wing_Chaos );   //yes - let it fall through to next case
			case ITEMID_Wing_Chaos:			ITEMHANDLER->DeleteItemQuest( ITEMID_Wing_Diamond );	//yes - let it fall through to next case
			case ITEMID_Wing_Diamond:		ITEMHANDLER->DeleteItemQuest( ITEMID_Wing_Gold );	//yes - let it fall through to next case
			case ITEMID_Wing_Gold:			ITEMHANDLER->DeleteItemQuest( ITEMID_Wing_Silver );	//yes - let it fall through to next case
			case ITEMID_Wing_Silver:		ITEMHANDLER->DeleteItemQuest( ITEMID_Wing_Metal );	//yes - let it fall through to next case
			case ITEMID_Wing_Metal:
				break;
		}

		auto eCurrentWing = ITEMHANDLER->GetItemInventoryByType( EItemType::ITEMTYPE_Wing );
		if ( eCurrentWing )
		{
			int iNewWingTier = itemID & 0x0000FFFF;
			int iCurrWingTier = itemID & 0x0000FFFF;

			//check new wing tier vs current
			if ( iCurrWingTier >= iNewWingTier )
				return;
		}
	}

	//special case for quest weapons
	if ( psItem->eCraftType == EItemCraftType::ITEMCRAFTTYPE_QuestWeapon )
	{
		//replace existing quest weapon if there is one
		if ( ITEMHANDLER->ReplaceCurrentQuestWeapon( psItem ) )
		{
			ResetInventoryItemChecksum ();
			CALLT ( 0x0050E510, 0x036932FC ); // Validate Inventory
			CHARACTERGAME->UpdateWeight ();
			CGameInventory::GetInstance()->CanChangeSpecialItem(2); //organize quest item inventory?
			SAVE;
			return;
		}
	}

	//For non-quest weapons
	else
	{
		const int iItemCount = ITEMHANDLER->GetItemAmountByCode( psItem->sItemID.ToItemID() );
		if ( iItemCount == iMaxItemCount )
			return;
		else if ( iItemCount > iMaxItemCount )
		{
			//remove duplciates
			ITEMHANDLER->DeleteItemQuest( psItem->sItemID.ToItemID(), iItemCount - iMaxItemCount );

			ResetInventoryItemChecksum ();
			CALLT ( 0x0050E510, 0x036932FC ); // Validate Inventory
			CHARACTERGAME->UpdateWeight ();
			CGameInventory::GetInstance()->CanChangeSpecialItem(2); //organize quest item inventory?
			SAVE;
			return;
		}

		psItem->sSpecialItemType[1] = ITEM_KIND_SPECIAL; //this will force item to go into quest inventory
	}


	ItemData sItemData;
	if ( ITEMHANDLER->LoadItemImage( psItem, &sItemData ) )
	{
		if ( sItemData.iPotionCount <= 0 )
			sItemData.iPotionCount = 1;

		if ( BackItemToInventory( &sItemData, ITEMCHECKINVENTORY_Potion ) )
		{
			ResetInventoryItemChecksum ();
			CALLT ( 0x0050E510, 0x036932FC ); // Validate Inventory
			CHARACTERGAME->UpdateWeight ();
			CGameInventory::GetInstance()->CanChangeSpecialItem(2); //organize quest item inventory?
			SAVE;
		}
	}

	//int sinSetQuestItem(sITEMINFO *sItemInfo )
	//if ( CALL_WITH_ARG1( 0x0050A0B0, (DWORD)psItem ) == 1 ) //rejects some quest items as we're not doing it the ASM way
	////{
		//CHATBOX->AddMessage( FormatString( "> Quest item '%s' has been added to your quest inventory", psItem->szItemName ) );
		//
		//ResetInventoryItemChecksum();
		//SAVE;
	//}
}

void ItemHandler::AutoItemToInventory( Item * psItem )
{
	if ( psItem->iChk1 != 0 &&
		 psItem->iChk2 != 0 )
	{
		ItemData sItemData;
		if ( ITEMHANDLER->LoadItemImage ( psItem, &sItemData ) )
		{
			const auto lItemType = psItem->sItemID.ToItemType();
			const auto lItemBase = psItem->sItemID.ToItemBase();

			if ( lItemType == EItemType::ITEMTYPE_GoldAndExp )
				return;

			ItemData * temp = ITEMHANDLER->FindItemTableByCode( psItem->sItemID.ToInt() );

			//Handle quest items
			if ( lItemType == EItemType::ITEMTYPE_Wing ||
				 lItemBase == EItemBase::ITEMBASE_Quest1 ||
				 lItemBase == EItemBase::ITEMBASE_Quest2 ||
				 psItem->eCraftType == EItemCraftType::ITEMCRAFTTYPE_QuestWeapon )
			{
				HandleQuestItem( psItem );
				return;
			}

			if ( !CHARACTERGAME->CheckInventoryWeight( temp ) )
			{
				TITLEBOX( "Weight limit exceeded" );

				//0x0062AFB0 = ThrowPutItem2( &lpTransItemInfo->Item , lpCurPlayer->pX, lpCurPlayer->pY , lpCurPlayer->pZ );
				CALL_WITH_ARG4( 0x0062AFB0, (DWORD)psItem, UNITDATA->sPosition.iX, UNITDATA->sPosition.iY, UNITDATA->sPosition.iZ );

				return;
			}

			// Check Space
			if ( lItemBase != EItemBase::ITEMBASE_Potion &&
				!CHARACTERGAME->CheckInventorySpace( temp ) &&
				!CHARACTERGAME->CheckEquipmentSpace( temp ) )
			{
				//No inventory space.
				//Do we have space in equipment slot?
				TITLEBOX( "Not enough space in inventory" );
				WINDOW_ISOPEN_INVENTORY = TRUE;

				ITEMHANDLER->CheckRequireItemToSet(&sItemData);
				if ( ITEMHANDLER->GetMouseItem() == NULL || ITEMHANDLER->GetMouseItem()->bValid == FALSE )
				{
					CopyMemory( ITEMHANDLER->GetMouseItem(), &sItemData, sizeof( ItemData ) );
					WRITEDWORD( 0x00999308, FALSE ); //hide cursor
				}
				else
				{
					//0x0062AFB0 = ThrowPutItem2( &lpTransItemInfo->Item , lpCurPlayer->pX, lpCurPlayer->pY , lpCurPlayer->pZ );
					CALL_WITH_ARG4( 0x0062AFB0, (DWORD)psItem, UNITDATA->sPosition.iX, UNITDATA->sPosition.iY, UNITDATA->sPosition.iZ );
				}

				return;
			}

			ITEMHANDLER->CheckRequireItemToSet(&sItemData);

			if ( (lItemBase == EItemBase::ITEMBASE_Potion && ITEMHANDLER->AutoSetPotion( &sItemData )) ||
				 ITEMHANDLER->BackItemToInventory( &sItemData, ITEMCHECKINVENTORY_None ) )
			{
				PLAYMINISOUND( sItemData.iInvItemSound );
				ITEMHANDLER->ResetInventoryItemChecksum ();
				CHARACTERGAME->UpdateWeight();
				ITEMHANDLER->UpdateItemCharacterStatus();

				SAVE;
			}
			else
			{
				CALL_WITH_ARG4( 0x0062AFB0, (DWORD)psItem, UNITDATA->sPosition.iX, UNITDATA->sPosition.iY, UNITDATA->sPosition.iZ );
			}
		}
	}
}

//int cINVENTORY::AutoSetPotion(sITEM *pItem)
BOOL ItemHandler::AutoSetPotion( ItemData * psItem )
{
	typedef BOOL( __thiscall *tfnAutoSetPotion )(DWORD dwClass, ItemData * psItemData);
	tfnAutoSetPotion fnAutoSetPotion = (tfnAutoSetPotion)0x004B9E00;
	return fnAutoSetPotion( 0x035EBB20, psItem );
}

//CheckRequireItemToSet
BOOL ItemHandler::CheckRequireItemToSet( ItemData * psItem )
{
	typedef BOOL( __thiscall *tfnCheckRequireItemToSet )(DWORD dwClass, ItemData * psItemData);
	tfnCheckRequireItemToSet fnCheckRequireItemToSet = (tfnCheckRequireItemToSet)0x004ACA90;
	return fnCheckRequireItemToSet( 0x035EBB20, psItem );
}

//AutoSetInvenItem
BOOL ItemHandler::BackItemToInventory( ItemData * psItem, EItemCheckInventory eCheck )
{
	typedef BOOL( __thiscall *tfnBackItemToInventory )(DWORD dwClass, ItemData * psItemData, int v);
	tfnBackItemToInventory fnBackItemToInventory = (tfnBackItemToInventory)0x004BB200;
	return fnBackItemToInventory( 0x035EBB20, psItem, eCheck );
}

void ItemHandler::ShowItemInfo( ItemData * pItem, BOOL bBottom )
{
	typedef void( __thiscall *tfnShowItemInfo )(DWORD pClass, ItemData * pItem, int iFlag, int index);
	tfnShowItemInfo fnShowItemInfo = (tfnShowItemInfo)0x004C9370;
	*(UINT*)0x036286EC = bBottom ? 2 : 1;
	fnShowItemInfo( 0x03628800, pItem, 0, 0 );
}

void ItemHandler::DrawColorBox( DWORD Color, int iX, int iY, int iW, int iH )
{
	typedef void( __cdecl *tfnDrawColorBox )(DWORD Color, int iX, int iY, int iW, int iH);
	tfnDrawColorBox fnDrawColorBox = (tfnDrawColorBox)0x00409F00;
	fnDrawColorBox( Color, iX, iY, iW, iH );
}

void ItemHandler::DrawSprite( int winX, int winY, int lpdds, int x, int y, int width, int height )
{
	typedef void( __cdecl *tfnDrawSprite )( int winX, int winY, int lpdds, int x, int y, int width, int height, int bltfast );
	tfnDrawSprite fnDrawSprite = (tfnDrawSprite)0x00505B20;
	fnDrawSprite( winX, winY, lpdds, x, y, width, height, 1 );
}

bool ItemHandler::DelayCristal()
{
	typedef bool( __thiscall *tfnCrystal )(DWORD item);
	tfnCrystal fnCrystal = (tfnCrystal)0x004B8510;
	if ( g_dwDelayCristal == 0 )
	{
		g_dwDelayCristal = GetCurrentTime() + 2000;
		return fnCrystal( 0x035EBB20 );
	}
	else
	{
		if ( g_dwDelayCristal > GetCurrentTime() )
		{
			TITLEBOX( "You are already summoning a pet monster" );
			return false;
		}
		else
		{
			g_dwDelayCristal = GetCurrentTime() + 2000;
			return fnCrystal( 0x035EBB20 );
		}
	}
	return false;
}

BOOL ItemHandler::OnQuestWeaponMatured( ItemData * psItem )
{
	if ( psItem && ( psItem->bValid || psItem->sItem.bMaturing ) && psItem->sItem.sMatureBar.sMax > 0 )
	{
		if ( psItem->sItem.eCraftType == EItemCraftType::ITEMCRAFTTYPE_QuestWeapon )
		{
			PacketQuestWeaponMature s;
			s.iHeader = PKTHDR_UpgradeQuestWeapon;
			s.iLength = sizeof( PacketQuestWeaponMature );
			CopyMemory( &s.sItem, &psItem->sItem, sizeof( Item ) );
			SENDPACKETL( &s );

			return TRUE;
		}
	}

	return FALSE;
}

BOOL ItemHandler::OnSendUpgradeAgingItem( ItemData * psItem )
{
	typedef void( __cdecl *tfnAgingUpgradeItem )(Item * psItem);
	tfnAgingUpgradeItem fnAgingUpgradeItem = (tfnAgingUpgradeItem)0x006228F0;

	if ( psItem && (psItem->bValid || psItem->sItem.bMaturing) && psItem->sItem.sMatureBar.sMax > 0)
	{
		if ( psItem->sItem.eCraftType == EItemCraftType::ITEMCRAFTTYPE_QuestWeapon )
		{
			//fnAgingUpgradeItem( &psItem->sItem );
			return TRUE;
		}
		else
		{
			if ( psItem->sItem.sMatureBar.sCur < 0 || psItem->sItem.sMatureBar.sCur >= psItem->sItem.sMatureBar.sMax )
			{
				psItem->sItem.sMatureBar.sCur = psItem->sItem.sMatureBar.sMax;
				psItem->sItem.bMaturing = TRUE;

				fnAgingUpgradeItem( &psItem->sItem );
				return TRUE;
			}
		}
		return TRUE;
	}
	return FALSE;
}

BOOL ItemHandler::CheckQuestItemAging( Item * psItemInfo )
{
	if ( psItemInfo->sItemID.ToItemID() == ITEMID_PotionAgingQuestBee )
		return TRUE;

	if ( STRINGCOMPARE( psItemInfo->szItemName, "Demon Hunter" ) )
		return TRUE;

	if ( STRINGCOMPARE( psItemInfo->szItemName, "Ancient Revenge" ) )
		return TRUE;

	if ( STRINGCOMPARE( psItemInfo->szItemName, "Dark Skull" ) )
		return TRUE;

	if ( STRINGCOMPARE( psItemInfo->szItemName, "Devil Scythe" ) )
		return TRUE;

	if ( STRINGCOMPARE( psItemInfo->szItemName, "Mist" ) )
		return TRUE;

	if ( STRINGCOMPARE( psItemInfo->szItemName, "Platinum Sword" ) )
		return TRUE;

	if ( STRINGCOMPARE( psItemInfo->szItemName, "Anaconda" ) )
		return TRUE;

	if ( STRINGCOMPARE( psItemInfo->szItemName, "Platinum Dagger" ) )
		return TRUE;

	if ( STRINGCOMPARE( psItemInfo->szItemName, "Hutan Phantom" ) )
		return TRUE;

	return FALSE;
}


BOOL ItemHandler::OnMatureStone( DWORD dwCode )
{
	ItemData * psItem = NULL;

	if ( dwCode == ITEMID_TurquoiseStone )
	{
		// Weapon
		psItem = &INVENTORYITEMS[INVENTORYITEMSLOT[0].iItemIndex - 1];
		if ( !CheckQuestItemAging( &psItem->sItem ) )
		{
			psItem->sItem.sMatureBar.sCur += 50;
			if ( OnSendUpgradeAgingItem( psItem ) )
				return TRUE;
		}
		// Shield, Orb
		psItem = &INVENTORYITEMS[INVENTORYITEMSLOT[1].iItemIndex - 1];
		psItem->sItem.sMatureBar.sCur += 50;
		if ( OnSendUpgradeAgingItem( psItem ) )
			return TRUE;

		// Armor
		psItem = &INVENTORYITEMS[INVENTORYITEMSLOT[2].iItemIndex - 1];
		psItem->sItem.sMatureBar.sCur += 50;
		if ( OnSendUpgradeAgingItem( psItem ) )
			return TRUE;

	}
	else if ( dwCode == ITEMID_TopazStone )
	{
		// Weapon
		psItem = &INVENTORYITEMS[INVENTORYITEMSLOT[0].iItemIndex - 1];
		if ( !CheckQuestItemAging( &psItem->sItem ) )
		{
			psItem->sItem.sMatureBar.sCur += INT16_MAX;
			if ( OnSendUpgradeAgingItem( psItem ) )
				return TRUE;
		}
		// Shield, Orb
		psItem = &INVENTORYITEMS[INVENTORYITEMSLOT[1].iItemIndex - 1];
		psItem->sItem.sMatureBar.sCur += INT16_MAX;
		if ( OnSendUpgradeAgingItem( psItem ) )
			return TRUE;

		// Armor
		psItem = &INVENTORYITEMS[INVENTORYITEMSLOT[2].iItemIndex - 1];
		psItem->sItem.sMatureBar.sCur += INT16_MAX;
		if ( OnSendUpgradeAgingItem( psItem ) )
			return TRUE;
	}


	return FALSE;
}

BOOL ItemHandler::OnSkillStone( DWORD dwCode )
{
	BOOL bTrained = FALSE;
	if ( dwCode == ITEMID_EmeraldStone )
	{
		for ( int i = 0; i < 12; i++ )
		{
			if ( SKILLSCHARACTER[i+1].iFlag && SKILLSCHARACTER[i+1].iLevel && SKILLSCHARACTER[i+1].UseSkillCount < 10000 )
			{
				SKILLSCHARACTER[i+1].UseSkillCount += 3000; //30%
				SKILLMANAGERHANDLER->ReformSkill( NULL, i + 1 );
				bTrained = TRUE;
			}
		}

		if ( bTrained == FALSE )
			TITLEBOX( "Don't have skills to train!" );
	}
	if ( dwCode == ITEMID_RubyStone )
	{
		for ( int i = 0; i < 12; i++ )
		{
			if ( SKILLSCHARACTER[i + 1].iFlag && SKILLSCHARACTER[i + 1].iLevel && SKILLSCHARACTER[i + 1].UseSkillCount < 10000 )
			{
				SKILLSCHARACTER[i + 1].UseSkillCount += 1000; //10%
				SKILLMANAGERHANDLER->ReformSkill( NULL, i + 1 );
				bTrained = TRUE;
			}
		}
		if ( bTrained == FALSE )
			TITLEBOX( "Don't have skills to train!" );
	}
	return bTrained;
}

void ItemHandler::OnAcceptRollDiceItem()
{
	if( UNITDATA )
	{
		PacketRollDiceDrop sPacket;
		sPacket.iHeader = PKTHDR_RollDiceDropUserResult;
		sPacket.iLength = sizeof(PacketRollDiceDrop);
		sPacket.bAcceptItem = MESSAGEBOX->GetType();
		sPacket.iID = UNITDATA->iID;
		sPacket.eItemID = ITEMID_None;
		SENDPACKETG( &sPacket );
	}
}


BOOL ItemHandler::OnCheckCanItemDrill( ItemData * pcItemData, int iUnk )
{
	return FALSE;
}

BOOL ItemHandler::OnRenderRightWeapon( UnitData * pcUnitData, EXEModel * pcModel, EXEMatrixI * psSourceMatrix, EXEMatrixI * psDestMatrix )
{
	if ( pcUnitData->bRenderRightHand && (pcUnitData->sRightHandTool.eItemID & 0xFFFF0000) == ITEMTYPE_Phantom )
	{
		pcModel->uFrame = pcUnitData->iFrame;

		EXEMatrixI si;
		EXEMatrixIdentityI( si );
		si.i[0][0] += (256 / 3);
		si.i[1][1] += (256 / 3);
		si.i[2][2] += (256 / 3);

		CopyMemory( &pcModel->paMesh[0]->sWorld, &pcUnitData->sRightHandMatrix, sizeof( EXEMatrixI ) );
		pcModel->paMesh[0]->sWorld = EXEMatrixMultiplyI( si, pcModel->paMesh[0]->sWorld );
	}

	CopyMemory( psDestMatrix, psSourceMatrix, sizeof( EXEMatrixI ) );

	return pcUnitData->bRenderRightHand;
}

BOOL ItemHandler::OnRenderSecondWeapon( UnitData * pcUnitData, EXEModel * pcModel, EXEMatrixI * psSourceMatrix, EXEMatrixI * psDestMatrix, BOOL bRight )
{
	if ( (pcUnitData->sRightHandTool.eItemID & 0xFFFF0000) == ITEMTYPE_Dagger )
	{
		EXEMatrixI si;
		EXEMatrixIdentityI( si );

		pcModel->uFrame = -1;

		if ( bRight == FALSE )
			EXEMatrixRotateZI( si, (4096 / 2) & 4095 );

		CopyMemory( &pcModel->paMesh[0]->sWorld, &pcUnitData->sLeftHandMatrix, sizeof( EXEMatrixI ) );
		pcModel->paMesh[0]->sWorld = EXEMatrixMultiplyI( si, pcModel->paMesh[0]->sWorld );
	}

	CopyMemory( psDestMatrix, psSourceMatrix, sizeof( EXEMatrixI ) );

	return TRUE;
}

// Event Bind of premium usage confirmation window MESSAGEBOX->SetEvent(std::bind)
void ItemHandler::OnConfirmPremiumUsage ( ItemData pcItemData )
{
	if ( MESSAGEBOX->GetType () )
	{
		HandlePremiumConfirmation ( TRUE, pcItemData );
	}
}
// Handle data if usage of premium item is confirmed with "OK" Button
void ItemHandler::HandlePremiumConfirmation ( bool bAccepted, ItemData pcItemData )
{
	ItemData	*pc = &pcItemData;
	EItemID		iItemID = pc->sItem.sItemID.ToItemID ();
	EItemType	iItemType = pc->sItem.sItemID.ToItemType ();
	// Indicates if the item is to be processed further
	BOOL bSpecialItem = FALSE;
	BOOL bIsHairTint = FALSE;

	if ( bAccepted )
	{
		// Prevent nullptr in ItemData
		if ( pc )
		{
			// Check for all Items of type Premium
			if ( iItemType == ITEMTYPE_Premium1 || ITEMTYPE_Premium2 )
			{
				// If no buff is already active, go and start a new item timer and apply the buff

				// EXP Potions 24-72h
				if (iItemID == ITEMID_ExpUp24H && ITEM_TIMER(ITEMTIMERTYPE_ExpUp) == NULL ||
					iItemID == ITEMID_ExpUp1H && ITEM_TIMER(ITEMTIMERTYPE_ExpUp) == NULL ||
					iItemID == ITEMID_ExpUp48H && ITEM_TIMER(ITEMTIMERTYPE_ExpUp) == NULL ||
					iItemID == ITEMID_ExpUp72H && ITEM_TIMER(ITEMTIMERTYPE_ExpUp) == NULL)
				{
					ITEMTIMERHANDLER->ProcessPacket(&PacketNewItemTimer(pc->sItem.sItemID.ToItemID(), (DWORD)pc->sItem.iChk1, (DWORD)pc->sItem.iChk2));
					bSpecialItem = TRUE;
				}
				// Third Eyes 24-72h
				else if (iItemID == ITEMID_ThirdEye24H && ITEM_TIMER(ITEMTIMERTYPE_ThirdEye) == NULL ||
					iItemID == ITEMID_ThirdEye1H && ITEM_TIMER(ITEMTIMERTYPE_ThirdEye) == NULL ||
					iItemID == ITEMID_ThirdEye48H && ITEM_TIMER(ITEMTIMERTYPE_ThirdEye) == NULL ||
					iItemID == ITEMID_ThirdEye72H && ITEM_TIMER(ITEMTIMERTYPE_ThirdEye) == NULL)
				{
					ITEMTIMERHANDLER->ProcessPacket(&PacketNewItemTimer(pc->sItem.sItemID.ToItemID(), (DWORD)pc->sItem.iChk1, (DWORD)pc->sItem.iChk2));
					bSpecialItem = TRUE;
				}
				// Mana Down 12h
				else if (iItemID == ITEMID_MPDown12H && ITEM_TIMER(ITEMTIMERTYPE_MPDown) == NULL) {
					ITEMTIMERHANDLER->ProcessPacket(&PacketNewItemTimer(pc->sItem.sItemID.ToItemID(), (DWORD)pc->sItem.iChk1, (DWORD)pc->sItem.iChk2));
					bSpecialItem = TRUE;
				}
				// Sixth Sense
				else if (iItemID == ITEMID_SixthSense1H && ITEM_TIMER(ITEMTIMERTYPE_SixthSense) == NULL) {
					ITEMTIMERHANDLER->ProcessPacket(&PacketNewItemTimer(pc->sItem.sItemID.ToItemID(), (DWORD)pc->sItem.iChk1, (DWORD)pc->sItem.iChk2));
					bSpecialItem = TRUE;
				}

				// Stamina Down 12h
				else if (iItemID == ITEMID_SPDown12H && ITEM_TIMER(ITEMTIMERTYPE_SPDown) == NULL) {
					ITEMTIMERHANDLER->ProcessPacket(&PacketNewItemTimer(pc->sItem.sItemID.ToItemID(), (DWORD)pc->sItem.iChk1, (DWORD)pc->sItem.iChk2));
					bSpecialItem = TRUE;
				}
				// Vampire Tooth (HP Regen 12h)
				else if (iItemID == ITEMID_HPUp12H && ITEM_TIMER(ITEMTIMERTYPE_HPUp) == NULL) {
					ITEMTIMERHANDLER->ProcessPacket(&PacketNewItemTimer(pc->sItem.sItemID.ToItemID(), (DWORD)pc->sItem.iChk1, (DWORD)pc->sItem.iChk2));
					bSpecialItem = TRUE;
				}
				// Mana Up (MP Regen 12h)
				else if (iItemID == ITEMID_MPUp12H && ITEM_TIMER(ITEMTIMERTYPE_MPUp) == NULL) {
					ITEMTIMERHANDLER->ProcessPacket(&PacketNewItemTimer(pc->sItem.sItemID.ToItemID(), (DWORD)pc->sItem.iChk1, (DWORD)pc->sItem.iChk2));
					bSpecialItem = TRUE;
				}
				// Phoenix Egg (Random)
				else if (iItemID == ITEMID_PhoenixEgg && ITEM_TIMER(ITEMTIMERTYPE_Phoenix) == NULL) {
					ITEMTIMERHANDLER->ProcessPacket(&PacketNewItemTimer(pc->sItem.sItemID.ToItemID(), (DWORD)pc->sItem.iChk1, (DWORD)pc->sItem.iChk2));
					bSpecialItem = TRUE;
				}
				// Phoenix Egg (Fire)
				else if (iItemID == ITEMID_PhoenixFire && ITEM_TIMER(ITEMTIMERTYPE_PhoenixFire) == NULL) {
					ITEMTIMERHANDLER->ProcessPacket(&PacketNewItemTimer(pc->sItem.sItemID.ToItemID(), (DWORD)pc->sItem.iChk1, (DWORD)pc->sItem.iChk2));
					bSpecialItem = TRUE;
				}
				// Phoenix Egg (Ice)
				else if (iItemID == ITEMID_PhoenixIce && ITEM_TIMER(ITEMTIMERTYPE_PhoenixIce) == NULL) {
					ITEMTIMERHANDLER->ProcessPacket(&PacketNewItemTimer(pc->sItem.sItemID.ToItemID(), (DWORD)pc->sItem.iChk1, (DWORD)pc->sItem.iChk2));
					bSpecialItem = TRUE;
				}
				// Phoenix Egg (Healing)
				else if (iItemID == ITEMID_PhoenixHealing && ITEM_TIMER(ITEMTIMERTYPE_PhoenixHeal) == NULL) {
					ITEMTIMERHANDLER->ProcessPacket(&PacketNewItemTimer(pc->sItem.sItemID.ToItemID(), (DWORD)pc->sItem.iChk1, (DWORD)pc->sItem.iChk2));
					bSpecialItem = TRUE;
				}
				// Phoenix Egg (Lightning)
				else if (iItemID == ITEMID_PhoenixLightning && ITEM_TIMER(ITEMTIMERTYPE_PhoenixLightning) == NULL) {
					ITEMTIMERHANDLER->ProcessPacket(&PacketNewItemTimer(pc->sItem.sItemID.ToItemID(), (DWORD)pc->sItem.iChk1, (DWORD)pc->sItem.iChk2));
					bSpecialItem = TRUE;
				}

				// Phoenix Egg (Fire)
				else if (iItemID == ITEMID_PhoenixFireSmall && ITEM_TIMER(ITEMTIMERTYPE_PhoenixFireSmall) == NULL) {
					ITEMTIMERHANDLER->ProcessPacket(&PacketNewItemTimer(pc->sItem.sItemID.ToItemID(), (DWORD)pc->sItem.iChk1, (DWORD)pc->sItem.iChk2));
					bSpecialItem = TRUE;
				}
				// Phoenix Egg (Ice)
				else if (iItemID == ITEMID_PhoenixIceSmall && ITEM_TIMER(ITEMTIMERTYPE_PhoenixIceSmall) == NULL) {
					ITEMTIMERHANDLER->ProcessPacket(&PacketNewItemTimer(pc->sItem.sItemID.ToItemID(), (DWORD)pc->sItem.iChk1, (DWORD)pc->sItem.iChk2));
					bSpecialItem = TRUE;
				}
				// Phoenix Egg (Healing)
				else if (iItemID == ITEMID_PhoenixHealingSmall && ITEM_TIMER(ITEMTIMERTYPE_PhoenixHealingSmall) == NULL) {
					ITEMTIMERHANDLER->ProcessPacket(&PacketNewItemTimer(pc->sItem.sItemID.ToItemID(), (DWORD)pc->sItem.iChk1, (DWORD)pc->sItem.iChk2));
					bSpecialItem = TRUE;
				}
				// Phoenix Egg (Lightning)
				else if (iItemID == ITEMID_PhoenixLightningSmall && ITEM_TIMER(ITEMTIMERTYPE_PhoenixLightningSmall) == NULL) {
					ITEMTIMERHANDLER->ProcessPacket(&PacketNewItemTimer(pc->sItem.sItemID.ToItemID(), (DWORD)pc->sItem.iChk1, (DWORD)pc->sItem.iChk2));
					bSpecialItem = TRUE;
				}
				// Gravity Stone (Weight +250 72h)
				else if (iItemID == ITEMID_GravityStone72H && ITEM_TIMER(ITEMTIMERTYPE_WeightStone) == NULL) {
					ITEMTIMERHANDLER->ProcessPacket(&PacketNewItemTimer(pc->sItem.sItemID.ToItemID(), (DWORD)pc->sItem.iChk1, (DWORD)pc->sItem.iChk2));
					bSpecialItem = TRUE;
				}
				// Honey Damage (event +10 atk damage)
				else if (iItemID == ITEMID_HONEYDAMAGE && ITEM_TIMER(ITEMTIMERTYPE_HoneyDamage) == NULL) {
					ITEMTIMERHANDLER->ProcessPacket(&PacketNewItemTimer(pc->sItem.sItemID.ToItemID(), (DWORD)pc->sItem.iChk1, (DWORD)pc->sItem.iChk2));
					bSpecialItem = TRUE;
				}
				// Honey Fast (event 10% EXP)
				else if (iItemID == ITEMID_HONEYFAST && ITEM_TIMER(ITEMTIMERTYPE_HoneyFast) == NULL) {
					ITEMTIMERHANDLER->ProcessPacket(&PacketNewItemTimer(pc->sItem.sItemID.ToItemID(), (DWORD)pc->sItem.iChk1, (DWORD)pc->sItem.iChk2));
					bSpecialItem = TRUE;
				}
				// Honey Smart (event +5MP Regen +50 MP)
				else if (iItemID == ITEMID_HONEYSMART && ITEM_TIMER(ITEMTIMERTYPE_HoneySmart) == NULL) {
					ITEMTIMERHANDLER->ProcessPacket(&PacketNewItemTimer(pc->sItem.sItemID.ToItemID(), (DWORD)pc->sItem.iChk1, (DWORD)pc->sItem.iChk2));
					bSpecialItem = TRUE;
				}
				// Honey Defense (event +10% Absorb)
				else if (iItemID == ITEMID_HONEYDEFENSE && ITEM_TIMER(ITEMTIMERTYPE_HoneyDefense) == NULL) {
					ITEMTIMERHANDLER->ProcessPacket(&PacketNewItemTimer(pc->sItem.sItemID.ToItemID(), (DWORD)pc->sItem.iChk1, (DWORD)pc->sItem.iChk2));
					bSpecialItem = TRUE;
				}

				// Server Wide Buffs
				// SHPRegen
				else if (iItemID == ITEMID_SHPRegen && ITEM_TIMER(ITEMTIMERTYPE_SHPRegen) == NULL) {
					ITEMTIMERHANDLER->ProcessPacket(&PacketNewItemTimer(pc->sItem.sItemID.ToItemID(), (DWORD)pc->sItem.iChk1, (DWORD)pc->sItem.iChk2));
					bSpecialItem = TRUE;
				}
				// SSPRegen
				else if (iItemID == ITEMID_SHPRegen && ITEM_TIMER(ITEMTIMERTYPE_SSPRegen) == NULL) {
					ITEMTIMERHANDLER->ProcessPacket(&PacketNewItemTimer(pc->sItem.sItemID.ToItemID(), (DWORD)pc->sItem.iChk1, (DWORD)pc->sItem.iChk2));
					bSpecialItem = TRUE;
				}
				// SMPRegen
				else if (iItemID == ITEMID_SHPRegen && ITEM_TIMER(ITEMTIMERTYPE_SMPRegen) == NULL) {
					ITEMTIMERHANDLER->ProcessPacket(&PacketNewItemTimer(pc->sItem.sItemID.ToItemID(), (DWORD)pc->sItem.iChk1, (DWORD)pc->sItem.iChk2));
					bSpecialItem = TRUE;
				}
				// SMPDown
				else if (iItemID == ITEMID_SMPDown && ITEM_TIMER(ITEMTIMERTYPE_SMPDown) == NULL) {
					ITEMTIMERHANDLER->ProcessPacket(&PacketNewItemTimer(pc->sItem.sItemID.ToItemID(), (DWORD)pc->sItem.iChk1, (DWORD)pc->sItem.iChk2));
					bSpecialItem = TRUE;
				}
				// SSPDown
				else if (iItemID == ITEMID_SSPDown && ITEM_TIMER(ITEMTIMERTYPE_SSPDown) == NULL) {
					ITEMTIMERHANDLER->ProcessPacket(&PacketNewItemTimer(pc->sItem.sItemID.ToItemID(), (DWORD)pc->sItem.iChk1, (DWORD)pc->sItem.iChk2));
					bSpecialItem = TRUE;
				}
				// SBonusDamage
				else if (iItemID == ITEMID_SBonusDamage && ITEM_TIMER(ITEMTIMERTYPE_SBonusDamage) == NULL) {
					ITEMTIMERHANDLER->ProcessPacket(&PacketNewItemTimer(pc->sItem.sItemID.ToItemID(), (DWORD)pc->sItem.iChk1, (DWORD)pc->sItem.iChk2));
					bSpecialItem = TRUE;
				}
				// SBonusAbsorb
				else if (iItemID == ITEMID_SBonusAbsorb && ITEM_TIMER(ITEMTIMERTYPE_SBonusAbsorb) == NULL) {
					ITEMTIMERHANDLER->ProcessPacket(&PacketNewItemTimer(pc->sItem.sItemID.ToItemID(), (DWORD)pc->sItem.iChk1, (DWORD)pc->sItem.iChk2));
					bSpecialItem = TRUE;
				}
				// SMoveSpeed
				else if (iItemID == ITEMID_SMoveSpeed && ITEM_TIMER(ITEMTIMERTYPE_SMoveSpeed) == NULL) {
					ITEMTIMERHANDLER->ProcessPacket(&PacketNewItemTimer(pc->sItem.sItemID.ToItemID(), (DWORD)pc->sItem.iChk1, (DWORD)pc->sItem.iChk2));
					bSpecialItem = TRUE;
				}
				// SExpBuff
				else if (iItemID == ITEMID_SExpBuff && ITEM_TIMER(ITEMTIMERTYPE_SExpBuff) == NULL) {
					ITEMTIMERHANDLER->ProcessPacket(&PacketNewItemTimer(pc->sItem.sItemID.ToItemID(), (DWORD)pc->sItem.iChk1, (DWORD)pc->sItem.iChk2));
					bSpecialItem = TRUE;
				}
				// SHPRegen
				else if (iItemID == ITEMID_SDropBuff && ITEM_TIMER(ITEMTIMERTYPE_SDropBuff) == NULL) {
					ITEMTIMERHANDLER->ProcessPacket(&PacketNewItemTimer(pc->sItem.sItemID.ToItemID(), (DWORD)pc->sItem.iChk1, (DWORD)pc->sItem.iChk2));
					bSpecialItem = TRUE;
				}
				// END SERVER WIDE BUFFS

				// Resistance Flasks
				else if (iItemID == ITEMID_IceResistance && ITEM_TIMER(ITEMTIMERTYPE_IceResistanceBuff) == NULL) {
				ITEMTIMERHANDLER->ProcessPacket(&PacketNewItemTimer(pc->sItem.sItemID.ToItemID(), (DWORD)pc->sItem.iChk1, (DWORD)pc->sItem.iChk2));
				bSpecialItem = TRUE;
				}
				// END Resistance Flasks

				// PetSystem (Wieght add)
				else if (iItemID == ITEMID_PetSystem_1H && ITEM_TIMER(ITEMTIMERTYPE_PetSystem) == NULL ||
				iItemID == ITEMID_PetSystem_24H && ITEM_TIMER(ITEMTIMERTYPE_PetSystem) == NULL ||
				iItemID == ITEMID_PetSystem_48H && ITEM_TIMER(ITEMTIMERTYPE_PetSystem) == NULL ||
				iItemID == ITEMID_PetSystem_72H && ITEM_TIMER(ITEMTIMERTYPE_PetSystem) == NULL) {
					ITEMTIMERHANDLER->ProcessPacket(&PacketNewItemTimer(pc->sItem.sItemID.ToItemID(), (DWORD)pc->sItem.iChk1, (DWORD)pc->sItem.iChk2));
					bSpecialItem = TRUE;
				}

				// Big Heads & Hats
				else if ( ITEMTIMERHANDLER->GetHead() == NULL &&
				   (iItemID == ITEMID_BigHeadHappyness ||
					iItemID == ITEMID_BigHeadLove ||
					iItemID == ITEMID_BigHeadValentine ||
					iItemID == ITEMID_BigHeadSadness ||
					iItemID == ITEMID_BigHeadShyness ||
					iItemID == ITEMID_BigHeadAngry ||
					iItemID == ITEMID_BigHeadSurprised ||
					iItemID == ITEMID_BigHeadSensual ||
					iItemID == ITEMID_CartolaHat ||
					iItemID == ITEMID_WitchHat ||
					iItemID == ITEMID_ChristmasGreenHat ||
					iItemID == ITEMID_ChristmasRedHat ||
					iItemID == ITEMID_SheepHat ||
					iItemID == ITEMID_GiraffeHat ||
					iItemID == ITEMID_SoccerHat ||
					iItemID == ITEMID_SoccerHatSpeed) )
				{
					// Save the old head model
					ITEMTIMERHANDLER->ProcessPacket( &PacketOldHeadUse( TRUE ) );
					ITEMTIMERHANDLER->ProcessPacket( &PacketNewItemTimer( iItemID, pc->sItem.iChk1, pc->sItem.iChk2 ) );
					bSpecialItem = TRUE;
				}

				// Hair tint potions
				else if ( ITEMTIMERHANDLER->GetHead() == NULL &&
				   (iItemID == ITEMID_HairPotionA ||
					iItemID == ITEMID_HairPotionB ||
					iItemID == ITEMID_HairPotionC ||
					iItemID == ITEMID_HairPotionD ||
					iItemID == ITEMID_HairPotionE ||
					iItemID == ITEMID_HairPotionF ||
					iItemID == ITEMID_HairPotionG ||
					iItemID == ITEMID_HairPotionH ||
					iItemID == ITEMID_HairPotionI ||
					iItemID == ITEMID_HairPotionJ ||
					iItemID == ITEMID_HairPotionK ||
					iItemID == ITEMID_HairPotionL ||
					iItemID == ITEMID_HairPotionM ||
					iItemID == ITEMID_HairPotionN ||
					iItemID == ITEMID_HairPotionO ||
					iItemID == ITEMID_HairPotionP ||
					iItemID == ITEMID_HairPotionQ ||
					iItemID == ITEMID_HairPotionR ||
					iItemID == ITEMID_HairPotionS ||
					iItemID == ITEMID_HairPotionT) )
				{
					if (ITEMHANDLER->SetHairModel(iItemID, UNITDATA->sCharacterData.iRank))
					{
						//Save the old head model
						ITEMTIMERHANDLER->ProcessPacket(&PacketOldHeadUse(TRUE));
						STRINGCOPY(CHARACTERGAME->sCharacterDataEx.szOldHead, UNITDATA->sCharacterData.Player.szHeadModel);
						bSpecialItem = TRUE;
					}
				}
				// If the item is already in use, decline usage again.
				else
				{
					if (ITEMTIMERHANDLER->GetHead () != NULL && iItemID == ITEMID_HairPotionA || iItemID == ITEMID_HairPotionB || iItemID == ITEMID_HairPotionC || iItemID == ITEMID_HairPotionD
						|| iItemID == ITEMID_HairPotionE || iItemID == ITEMID_HairPotionF || iItemID == ITEMID_HairPotionG || iItemID == ITEMID_HairPotionH || iItemID == ITEMID_HairPotionI
						|| iItemID == ITEMID_HairPotionJ || iItemID == ITEMID_HairPotionK || iItemID == ITEMID_HairPotionL || iItemID == ITEMID_HairPotionM || iItemID == ITEMID_HairPotionN
						|| iItemID == ITEMID_HairPotionO || iItemID == ITEMID_HairPotionP || iItemID == ITEMID_HairPotionQ || iItemID == ITEMID_HairPotionR
						|| iItemID == ITEMID_HairPotionS || iItemID == ITEMID_HairPotionT)
					{
						TITLEBOX ( "Can not use %s while a big head effect is active.", pc->sItem.szItemName );
					}
					else
					{
						TITLEBOX ( "%s buff is already active!", pc->sItem.szItemName );
					}
				}
			}
			// Process the item after use. Remove it from Inventory, set validity false, update weight and validate new inventory before calling SAVE();
			if ( bSpecialItem )
			{
				pc->sItem.iPotionCount = 300;
				ItemHandler::DropItemToMap ( pc );
				pc->bValid = FALSE;
				CHARACTERGAME->UpdateWeight ();
				ValidateInventoryItems ();
				SAVE;
			}
		}
	}
	// Clear the item data from memory
	ZeroMemory ( pc, sizeof ( ItemData ) );
}

int ItemHandler::OnUseItem ( ItemData * pcItemData )
{ //Return 0: Deny Use 1: Allow Use 2: Used
	EItemID iItemID = pcItemData->sItem.sItemID.ToItemID ();
	EItemType iItemType = pcItemData->sItem.sItemID.ToItemType ();
	ItemData eventData = *pcItemData;

	//Crystals
	if ( iItemType == ITEMTYPE_Crystal )
	{
		//Battle Town
		if ( MAP_ID == MAPID_BattleTown )
			return 0;

		//T5 Quest Arena
		if ( MAP_ID == MAPID_T5QuestArena )
			return 0;

		//Bless Castle
		if ( MAP_ID == MAPID_BlessCastle )
		{
			if ( iItemID == ITEMID_RicartenGuardCrystal )
				return 1;
			if ( iItemID == ITEMID_RicartenMilitiaGuardCrystal )
				return 1;
			if ( iItemID == ITEMID_RoyalBlessGuardCrystal )
				return 1;
			if ( iItemID == ITEMID_GuardianSaintCrystal )
				return 1;

			TITLEBOX ( "You can only use guardian saint and castle monster crystals in bless castle" );
			return 0;
		}
	}

	//Cores
	if ( iItemType == ITEMTYPE_EtherCore )
	{
		if ( MAP_ID == MAPID_CursedTempleF2 )
		{
			if ( GRANDFURYHANDLER->InsideArena ( UNITDATA->sPosition.iX, UNITDATA->sPosition.iZ ) )
			{
				TITLEBOX ( "You cannot use ether cores inside the arena!" );
				return 0;
			}
			return 1;
		}
	}

	int iRet = 0;
	BOOL bSpecialItem = FALSE;

	switch ( iItemID )
	{
		// Hair Tint Potions
		case ITEMID_HairPotionA: case ITEMID_HairPotionB: case ITEMID_HairPotionC: case ITEMID_HairPotionD: case ITEMID_HairPotionE: case ITEMID_HairPotionF:
		case ITEMID_HairPotionG: case ITEMID_HairPotionH: case ITEMID_HairPotionI: case ITEMID_HairPotionJ: case ITEMID_HairPotionK: case ITEMID_HairPotionL:
		case ITEMID_HairPotionM: case ITEMID_HairPotionN: case ITEMID_HairPotionO: case ITEMID_HairPotionP: case ITEMID_HairPotionQ: case ITEMID_HairPotionR:
		case ITEMID_HairPotionS: case ITEMID_HairPotionT:

		// Big Heads
		case ITEMID_BigHeadHappyness: case ITEMID_BigHeadLove: case ITEMID_BigHeadValentine: case ITEMID_BigHeadSadness:
		case ITEMID_BigHeadShyness: case ITEMID_BigHeadAngry: case ITEMID_BigHeadSurprised: case ITEMID_BigHeadSensual:

		// Hats
		case ITEMID_CartolaHat: case ITEMID_WitchHat: case ITEMID_ChristmasGreenHat: case ITEMID_ChristmasRedHat:
		case ITEMID_SheepHat: case ITEMID_GiraffeHat: case ITEMID_SoccerHat: case ITEMID_SoccerHatSpeed:

		// Power Ups (EXP, Third Eye, MP & SP Regen, WeightStones etc
		case ITEMID_ExpUp1H: case ITEMID_ExpUp24H: case ITEMID_ExpUp48H: case ITEMID_ExpUp72H:
		case ITEMID_PetSystem_1H: case ITEMID_PetSystem_24H: case ITEMID_PetSystem_48H: case ITEMID_PetSystem_72H:
		case ITEMID_ThirdEye1H: case ITEMID_ThirdEye24H: case ITEMID_ThirdEye48H: case ITEMID_ThirdEye72H:
		case ITEMID_MPDown12H: case ITEMID_SPDown12H: case ITEMID_MPUp12H: case ITEMID_HPUp12H: case ITEMID_HONEYDAMAGE: case ITEMID_HONEYFAST: case ITEMID_HONEYSMART: case ITEMID_HONEYDEFENSE:
		case ITEMID_PhoenixEgg: case ITEMID_PhoenixFire: case ITEMID_PhoenixIce: case ITEMID_PhoenixHealing: case ITEMID_PhoenixLightning: case ITEMID_PhoenixFireSmall: case ITEMID_PhoenixIceSmall: case ITEMID_PhoenixHealingSmall: case ITEMID_PhoenixLightningSmall:
		case ITEMID_GravityStone72H: case ITEMID_SixthSense1H: case ITEMID_IceResistance:

		// Server Wide Buffs
		case ITEMID_SHPRegen: case ITEMID_SSPRegen: case ITEMID_SMPRegen: case ITEMID_SMPDown: case ITEMID_SSPDown: case ITEMID_SBonusDamage: case ITEMID_SBonusAbsorb: case ITEMID_SMoveSpeed: case ITEMID_SExpBuff: case ITEMID_SDropBuff:

		// Create Window to ask player to confirm Premium Item usage
		MESSAGEBOX->SetBoxType ( CMessageBox::BOXTYPEID_OkCancel );
		MESSAGEBOX->SetTitle ( pcItemData->sItem.szItemName );
		MESSAGEBOX->SetDescription ( "Are you sure you want to use this premium item? " );
		MESSAGEBOX->SetEvent ( std::bind ( &ItemHandler::OnConfirmPremiumUsage, this, eventData ) );
		MESSAGEBOX->Show ();

		// Return 0 and skip default action further processing see ItemHandler::OnConfirmPremiumUsage
		iRet = 0;
		break;

		case ITEMID_TopazStone:
		case ITEMID_TurquoiseStone:
		if ( ITEMHANDLER->OnMatureStone ( iItemID ) )
		{
			iRet = 2;
			bSpecialItem = TRUE;
		}
		else
		{
			TITLEBOX ( "Don't have items to mature!" );
			iRet = 1;
		}
		break;

		case ITEMID_EmeraldStone:
		case ITEMID_RubyStone:
		if ( ITEMHANDLER->OnSkillStone ( iItemID ) )
		{
			iRet = 2;
			bSpecialItem = TRUE;
		}
		else
		{
			TITLEBOX ( "Don't have skills to train!" );
			iRet = 1;
		}
		break;

		case ITEMID_RottenLantern:
		case ITEMID_EvilLantern:
		case ITEMID_InfernalLantern:
		// Check iWeight
		if ( ( UNITDATA->sCharacterData.sWeight.sCur + 25 ) > UNITDATA->sCharacterData.sWeight.sMax )
		{
			TITLEBOX ( "Weight limit exceeded. Need at least 25 or more free weight" );
		}
		else if ( !CHARACTERGAME->CheckInventorySpace ( ITEMHANDLER->FindItemTableByCode ( ITEMID_BrazilianSoccer ) ) )
		{
			TITLEBOX ( "Not enough space in inventory" );
		}
		else
		{
			if ( HALLOWEENHANDLER->EventIsAlive () == FALSE )
			{
				TITLEBOX ( "Halloween Event ended!" );
			}
			else
			{
				iRet = 2;

				bSpecialItem = TRUE;
			}
		}
		break;

		default:
		iRet = 1;
		break;
	}

	if ( bSpecialItem )
	{
		pcItemData->sItem.iPotionCount = 300;

		ItemHandler::DropItemToMap ( pcItemData );

		pcItemData->bValid = FALSE;

		CHARACTERGAME->UpdateWeight ();
		ValidateInventoryItems ();

		SAVE;
	}

	return iRet;
}

void ItemHandler::ProcessItemSlotAction( ESlotTypeHandle iAction, ESlotTypeHandleWhere iWhere, int iItemID, int iChk1, int iChk2 )
{
	PacketSlotItemHandle s;
	s.iLength		= sizeof( PacketSlotItemHandle );
	s.iHeader		= PKTHDR_ItemSlotHandle;
	s.iTypeHandle	= iAction;
	s.iTypeWhere	= iWhere;
	s.iItemID		= iItemID;
	s.iChk1			= iChk1;
	s.iChk2			= iChk2;
	SENDPACKETL( &s );
}

void ItemHandler::SendGoldUse( int iGold )
{
	CALL_WITH_ARG1( 0x00629B00, iGold );
}

void ItemHandler::ProcessGold()
{
	CALL( 0x00628080 );
}

void ItemHandler::FormatDropItemGold( DropItemView * ps )
{
	STRINGCOPY( ps->szName, FormatNumber(atoi(ps->szName)) );
	STRINGCONCAT( ps->szName, " Gold" );

	ps->bFormatted = 0x1;
}


void ItemHandler::HandlePacket( PacketNPCItemBox * psPacket )
{
	//Check Weight
	if ( UNITDATA->sCharacterData.sWeight.sCur > UNITDATA->sCharacterData.sWeight.sMax )
		TITLEBOX( "You are overweighted" );
	else if ( psPacket->bItem )
	{
		CGameInventory::GetInstance()->SetInventoryItem( &psPacket->sItem );
		ResetInventoryItemChecksum();

		if (psPacket->sItem.sItemID.ToItemBase() == ITEMBASE_Potion )
			CGameInventory::ResetPotion();

		SAVE;
	}

	if ( psPacket->iItemID )
	{
		ZeroMemory( &MESSAGEBOX_ITEMBOX->cItemData, sizeof( ItemData ) );
		MESSAGEBOX_ITEMBOX->cItemData.sBaseItemID.SetItemID( (EItemID)psPacket->iItemID );

		//Check Weight of an Item
		if ( (UNITDATA->sCharacterData.sWeight.sCur + psPacket->sWeight) > UNITDATA->sCharacterData.sWeight.sMax )
			TITLEBOX( "You are overweighted to receive this item" );
		else if ( CHARACTERGAME->CheckInventorySpace( &MESSAGEBOX_ITEMBOX->cItemData, FALSE ) == FALSE )
			TITLEBOX( "Not enough space in inventory" );
		else
		{
			//Set Item Message Box for accept
			ZeroMemory( MESSAGEBOX_ITEMBOX->szItemName, _countof( MESSAGEBOX_ITEMBOX->szItemName ) );
			STRINGCOPY( MESSAGEBOX_ITEMBOX->szMessage, psPacket->szMessage );

			MESSAGEBOX_ITEMBOX->bActive = TRUE;
			MESSAGEBOX_ITEMBOX->dwReserved[0] = psPacket->dwReserved[0];

			WINDOW_ISOPEN_INVENTORY = TRUE;

			if ( MESSAGEBOX_ITEMBOX->cItemData.sBaseItemID.ToItemType() == ITEMTYPE_GoldAndExp )
			{
				//Gold? Set Price
				if ( MESSAGEBOX_ITEMBOX->cItemData.sBaseItemID.ToItemID() == ITEMID_Gold )
					MESSAGEBOX_ITEMBOX->cItemData.sItem.iSalePrice = psPacket->iSpecialization;

				//Exp? Set Exp on old X Position
				if ( MESSAGEBOX_ITEMBOX->cItemData.sBaseItemID.ToItemID() == ITEMID_Experience )
					MESSAGEBOX_ITEMBOX->cItemData.sOldPosition.iX = psPacket->iSpecialization;

				//Load Image
				CGameInventory::GetInstance()->LoadItemImageEXPGold( &MESSAGEBOX_ITEMBOX->cItemData );
			}
			else
			{
				if ( MESSAGEBOX_ITEMBOX->cItemData.sBaseItemID.ToItemBase() == ITEMBASE_Potion )
					MESSAGEBOX_ITEMBOX->cItemData.sItem.iPotionCount = psPacket->iSpecialization;

				CHARACTERGAME->CheckInventorySpace( &MESSAGEBOX_ITEMBOX->cItemData, FALSE );

				STRINGCOPY( MESSAGEBOX_ITEMBOX->szItemName, psPacket->szItemName );
			}
		}
	}
}

void ItemHandler::HandlePacket( PacketItemShopList * psPacket )
{
	iUniqueShopID = psPacket->iUniqueID;
	iNPCID = psPacket->iNPCID;
}


void ItemHandler::LoadDefaultModels()
{
	for ( int i = 0; i < 10; i++ )
	{
		EXEModelData * ps = Game::ReadInx( saCharacterBodyLoadDefault[i].pszFileName );
		if ( ps && ps->psModelData )
		{
			for ( int j = 0; j < ps->psModelData->iNumModelAnimation; j++ )
			{
				if ( ps->psModelData->saModelAnimation[j].iType == ANIMATIONTYPE_Idle )
				{
					psaModelDataDefault[i] = &ps->psModelData->saModelAnimation[j];
					break;
				}
			}
		}
	}
}

ModelAnimation * ItemHandler::GetModelDefault( int iClass )
{
	if ( iClass > 0 && iClass <= 10 )
		return psaModelDataDefault[iClass - 1];

	return NULL;
}


BOOL ItemHandler::BuyItemShop( ItemData * pcItemData, int iItemCount )
{
	BOOL bRes = TRUE;

	if ( (UNITDATA->sCharacterData.sWeight.sCur + pcItemData->sItem.iWeight) > UNITDATA->sCharacterData.sWeight.sMax )
	{
		TITLEBOX( "Weight limit exceeded" );
		bRes = FALSE;
	}
	else if ( !CHARACTERGAME->CheckInventorySpace( pcItemData ) )
	{
		TITLEBOX( "Not enough space in inventory" );
		bRes = FALSE;
	}
	else
	{
		int iPrice = pcItemData->sItem.iSalePrice * iItemCount;

		BOOL bResPrice = FALSE;

		__asm
		{
			PUSH iPrice;
			MOV ECX, 0x03653E40;
			MOV EAX, 0x004E59C0;
			CALL EAX;
			MOV bResPrice, EAX;
		}

		if ( bResPrice == FALSE )
		{
			TITLEBOX( "Not enough gold" );
			bResPrice = FALSE;
			bRes = FALSE;
		}
	}

	if ( bRes )
	{
		PacketBuyNPCShop sPacket;
		sPacket.iLength = sizeof( PacketBuyNPCShop );
		sPacket.iHeader = PKTHDR_BuyItemNPCShop;
		sPacket.iUniqueID = iUniqueShopID;
		sPacket.iNPCID = iNPCID;

		sPacket.iCount = iItemCount;
		CopyMemory( &sPacket.sItemData, pcItemData, sizeof( ItemData ) );
		SENDPACKETL( &sPacket );
		SENDPACKETG( &sPacket );
	}

	return bRes;
}