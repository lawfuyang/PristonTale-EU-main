#include "stdafx.h"
#include "itemserver.h"
#include "Logger.h"
#include "Utilities.h"

DWORD pTableNewItem = 0;
DWORD pTableNewItemSoma = 0x10;

extern ItemData								* pAllocItemTable;
UINT										iTableItemCompressPosition = 0;


void * allocIT = NULL;
UINT ItemV = 0;

void refIT ();
void endIT ();


ItemData * pAllocItemTable = NULL;

CSocketSystemHandler * ItemServer::pcSocketSystemHandler;

std::map<int, int> ItemServer::s_ItemBuyPriceMap;
std::map<int, int> ItemServer::s_ItemBuyMaxIntegrityMap;
std::map<int, int> ItemServer::s_ItemIdToLevelReqMap;

static DWORD iaSheltomAgingList[AGING_MAX][12]
{
	{3, 3, 4, 4, 5, 0, 0, 0, 0, 0, 0, 0},					// 1
	{3, 3, 4, 4, 5, 5, 0, 0, 0, 0, 0, 0},					// 2
	{3, 3, 4, 4, 5, 5, 6, 0, 0, 0, 0, 0},					// 3
	{3, 3, 4, 4, 5, 5, 6, 6, 0, 0, 0, 0},					// 4
	{3, 3, 4, 4, 5, 5, 6, 6, 7, 0, 0, 0},					// 5
	{3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 0, 0},					// 6
	{3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 0},					// 7
	{3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8},					// 8
	{4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 0},					// 9
	{4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9},					// 10
	{5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 0},					// 11
	{5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10},					// 12
	{6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 0},				// 13
	{6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11},				// 14
	{7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 0},				// 15
	{7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12},				// 16
	{8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 0},			// 17
	{8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13},			// 18
	{9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 0 },			// 19
	{9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14 },		// 20
};




static int ForceDamageTable[]
{
		2,		// Lucidy
		4,		// Sereneo
		7,		// Fadeo
		10,		// Sparky
		15,		// Raident
		25,		// Transparo
		40,		// Murky
		60,		// Devine
		80,		// Celesto
		100,	// Mirage
		120,	// Inferna
		140,	// Enigma
		160,	// Bellum
		180,	// Oredo
};


//Should also match client
static int ForceDurationTable[]
{
		500,		// Lucidy
		500,		// Sereneo
		500,		// Fadeo
		500,		// Sparky
		700,		// Raident
		900,		// Transparo
		1200,		// Murky
		1400,		// Devine
		1600,		// Celesto
		1800,		// Mirage
		2000,		// Inferna
		2400,		// Enigma
		3000,		// Bellum
		3600,		// Oredo
};

static int ForceDamagePercentTable[]
{
		0,		// Lucidy
		0,		// Sereneo
		0,		// Fadeo
		0,		// Sparky
		0,		// Raident
		0,		// Transparo
		10,		// Murky
		10,		// Devine
		10,		// Celesto
		10,		// Mirage
		10,		// Inferna
		10,		// Enigma
		10,		// Bellum
		10,		// Oredo
};



ItemServer::ItemServer()
{
	ItemServer::pcSocketSystemHandler = new CSocketSystemHandler();
}


ItemServer::~ItemServer()
{
}

float RandomFloat( float a, float b )
{
	float random = ( ( float )rand( ) ) / ( float )RAND_MAX;
	float diff = b - a;
	float r = random * diff;
	return a + r;
}


BOOL ItemServer::FindItemName( UINT uCode, char * szBufName )
{
	STRINGCOPYLEN( szBufName, 32, "NULL" );

	char szCharSpecial[2] = { 0x27, 0 };

	if ( !pAllocItemTable )
		return FALSE;

	for ( UINT u = 0; u < iTableItemCompressPosition; u++ )
	{
		if ( pAllocItemTable[u].sBaseItemID.ToItemID() == uCode )
		{
			STRINGCOPYLEN( szBufName, 32, pAllocItemTable[u].szBaseName );

			char * pszCharSpecial = strstr( szBufName, szCharSpecial );

			if ( pszCharSpecial )
				*(char*)&pszCharSpecial[0] = ' ';

			return TRUE;
		}
	}

	return FALSE;
}
BOOL ItemServer::FindItemName( char * pszCode, char * szBufName )
{
	STRINGCOPYLEN( szBufName, 32, "NULL" );
	char szCharSpecial[2] = { 0x27, 0 };
	if ( !pAllocItemTable )
		return FALSE;

	for ( UINT u = 0; u < iTableItemCompressPosition; u++ )
	{
		if ( STRINGCOMPAREI(pAllocItemTable[u].szInventoryName, pszCode) )
		{
			STRINGCOPYLEN( szBufName, 32, pAllocItemTable[u].szBaseName );
			char * pszCharSpecial = strstr( szBufName, szCharSpecial );
			if ( pszCharSpecial )
				*(char*)&pszCharSpecial[0] = ' ';
			return TRUE;
		}
	}

	return FALSE;
}

int ItemServer::GetItemIDByItemCode( char * pszCode )
{
	if ( !pAllocItemTable )
		return 0;

	for ( UINT u = 0; u < iTableItemCompressPosition; u++ )
	{
		if ( STRINGCOMPAREI(pAllocItemTable[u].szInventoryName, pszCode) )
		{
			return pAllocItemTable[u].sBaseItemID.ToItemID();
		}
	}
	return 0;
}

BOOL ItemServer::IsDropableItem( int iItemID )
{
	BOOL bRet = TRUE;

	switch ( iItemID )
	{
		case ITEMID_BeginnerAxe:
		case ITEMID_BeginnerClaw:
		case ITEMID_BeginnerHammer:
		case ITEMID_BeginnerJavelin:
		case ITEMID_BeginnerScythe:
		case ITEMID_BeginnerBoots:
		case ITEMID_BeginnerGauntlet:
		case ITEMID_BeginnerBracelet:
		case ITEMID_BeginnerBow:
		case ITEMID_BeginnerDagger:
		case ITEMID_BeginnerPhantom:
		case ITEMID_BeginnerStaff:
		case ITEMID_BeginnerSword:
		case ITEMID_GrumpyArmor:
		case ITEMID_GrumpyRobe:
			bRet = FALSE;
			break;

		default:
			break;
	}

	return bRet;
}

int ItemServer::SpecStoneToCharacterSpec( int iItemID )
{
	int iSpec = 0;
	switch ( iItemID )
	{
		case ITEMID_DeluxeSpecStoneFS:
		case ITEMID_SpecStoneFS:
			iSpec = CHARACTERCLASS_Fighter;
			break;

		case ITEMID_DeluxeSpecStoneMS:
		case ITEMID_SpecStoneMS:
			iSpec = CHARACTERCLASS_Mechanician;
			break;

		case ITEMID_DeluxeSpecStoneAS:
		case ITEMID_SpecStoneAS:
			iSpec = CHARACTERCLASS_Archer;
			break;

		case ITEMID_DeluxeSpecStonePS:
		case ITEMID_SpecStonePS:
			iSpec = CHARACTERCLASS_Pikeman;
			break;

		case ITEMID_DeluxeSpecStoneATA:
		case ITEMID_SpecStoneATA:
			iSpec = CHARACTERCLASS_Atalanta;
			break;

		case ITEMID_DeluxeSpecStoneKS:
		case ITEMID_SpecStoneKS:
			iSpec = CHARACTERCLASS_Knight;
			break;

		case ITEMID_DeluxeSpecStoneMG:
		case ITEMID_SpecStoneMG:
			iSpec = CHARACTERCLASS_Magician;
			break;

		case ITEMID_DeluxeSpecStonePRS:
		case ITEMID_SpecStonePRS:
			iSpec = CHARACTERCLASS_Priestess;
			break;

		case ITEMID_DeluxeSpecStoneASS:
		case ITEMID_SpecStoneASS:
			iSpec = CHARACTERCLASS_Assassin;
			break;

		case ITEMID_DeluxeSpecStoneSHA:
		case ITEMID_SpecStoneSHA:
			iSpec = CHARACTERCLASS_Shaman;
			break;

		default:
			break;
	}
	return iSpec;
}

BOOL ItemServer::IsDeluxeStone( int iItemID )
{
	BOOL bRet = FALSE;

	switch ( iItemID )
	{
		case ITEMID_DeluxeSpecStoneFS:
		case ITEMID_DeluxeSpecStoneMS:
		case ITEMID_DeluxeSpecStoneAS:
		case ITEMID_DeluxeSpecStonePS:
		case ITEMID_DeluxeSpecStoneATA:
		case ITEMID_DeluxeSpecStoneKS:
		case ITEMID_DeluxeSpecStoneMG:
		case ITEMID_DeluxeSpecStonePRS:
		case ITEMID_DeluxeSpecStoneASS:
		case ITEMID_DeluxeSpecStoneSHA:
			bRet = TRUE;
			break;

		default:
			break;
	}
	return bRet;
}

BOOL ItemServer::CharacterClassCanUseItem(ECharacterClass iClass, DefinitionItem* psDefItem)
{
	auto classFlag = CharacterClassToClassFlag(iClass);

	for (int j = 0; j < psDefItem->JobBitCodeRandomCount; j++)
	{
		if (psDefItem->JobBitCodeRandom[j] == classFlag)
		{
			return TRUE;
		}
	}

	return FALSE;
}


CharacterClassData * ItemServer::GetItemSpecClassJob( int iClass, DefinitionItem * psDefItem )
{
	int iCounter = _countof( saCharacterClassData );

	int iJobID = iClass;


	if ( (iJobID >= 100 || iJobID == 0) && Dice::RandomI(1,100) < 50 )
	{
		if ( psDefItem->JobBitCodeRandomCount > 0 )
			iJobID = ClassFlagToCharacterClass( (EClassFlag)psDefItem->JobBitCodeRandom[rand() % psDefItem->JobBitCodeRandomCount] );
	}


	for ( int i = 0; i < iCounter; i++ )
	{
		CharacterClassData * ps = saCharacterClassData + i;

		if ( ps->iClass == iJobID )
			return ps;
	}

	return NULL;
}

ItemData * ItemServer::FindItemPointerTable ( const char * szCode )
{
	if ( !pAllocItemTable )
		return NULL;

	for ( UINT u = 0; u < iTableItemCompressPosition; u++ )
	{
		if ( STRINGCOMPAREI ( pAllocItemTable[u].szInventoryName, szCode ) )
			return &pAllocItemTable[u];
	}

	return NULL;
}

ItemData * ItemServer::FindItemPointerTable ( UINT uCode )
{
	if ( !pAllocItemTable )
		return NULL;

	for ( UINT u = 0; u < iTableItemCompressPosition; u++ )
	{
		if ( pAllocItemTable[u].sBaseItemID.ToItemID () == uCode )
			return &pAllocItemTable[u];
	}

	return NULL;
}

DefinitionItem *	ItemServer::FindItemDefByCode( DWORD dwCode )
{
	DefinitionItem * pDefItem = *( DefinitionItem** )0x07A9EB10;

	for ( UINT i = 0; i < NUM_ITEMS; i++ )
	{
		if ( pDefItem[i].sItem.sItemID.ToItemID() == dwCode )
			return &pDefItem[i];
	}
	return NULL;
}


DefinitionItem * ItemServer::FindOldItemDefByCode ( DWORD dwCode )
{
	DefinitionItem * pDefItem = *(DefinitionItem **)0x07A9EB10;

	for ( UINT i = 0; i < NUM_ITEMS; i++ )
	{
		if ( OldItemDefinition[i].sItem.sItemID.ToItemID () == dwCode )
		{
			return &OldItemDefinition[i];
		}
	}
	return NULL;
}

DefinitionItem *	ItemServer::FindQuestWeaponDefByCode( const char * pszCode )
{
	DefinitionItem * pDefItem = *( DefinitionItem** )0x07A9EB10;

	ItemData * psItemData = FindItemPointerTable( pszCode );
	if ( psItemData )
	{
		for ( UINT i = 0; i < NUM_ITEMS; i++ )
		{
			if ( pDefItem[i].sItem.sItemID.ToItemID() == psItemData->sBaseItemID.ToItemID() &&
				 pDefItem[i].sItem.iItemUniqueID == UNIQUEID_QUEST_WEAPON )
			{
				return &pDefItem[i];
			}
		}
	}
	return NULL;
}

DefinitionItem *	ItemServer::FindItemDefByCode( char * pszCode )
{
	DefinitionItem * pDefItem = *( DefinitionItem** )0x07A9EB10;

	ItemData * psItemData = FindItemPointerTable( pszCode );
	if ( psItemData )
	{
		for ( UINT i = 0; i < NUM_ITEMS; i++ )
		{
			if ( pDefItem[i].sItem.sItemID.ToItemID() == psItemData->sBaseItemID.ToItemID() )
				 return &pDefItem[i];
		}
	}
	return NULL;
}

DefinitionItem * ItemServer::FindOldItemDefByCode ( char * pszCode )
{
	ItemData * psItemData = FindItemPointerTable ( pszCode );
	if ( psItemData )
	{
		for ( UINT i = 0; i < NUM_ITEMS; i++ )
		{
			if ( OldItemDefinition[i].sItem.sItemID.ToItemID () == psItemData->sBaseItemID.ToItemID () )
				return &OldItemDefinition[i];
		}
	}
	return NULL;
}

DefinitionItem * ItemServer::FindItemDefByLevelBase( int iItemBase, int iLevel )
{
	DefinitionItem * pDefItem = *(DefinitionItem**)0x07A9EB10;

	for ( UINT i = 0; i < NUM_ITEMS; i++ )
	{
		if ( pDefItem[i].sItem.sItemID.ToItemType() == iItemBase && pDefItem[i].sItem.iLevel == iLevel )
			return &pDefItem[i];
	}
	return NULL;
}

int ItemServer::FindItemPositionTable( char * pszName )
{
	if ( !pAllocItemTable )
		return -1;

	for ( UINT u = 0; u < iTableItemCompressPosition; u++ )
	{
		if ( STRINGCOMPAREI( pAllocItemTable[u].szBaseName, pszName ) )
			return u;
	}
	return -1;
}

void ItemServer::CompressItemDataInTable ( ItemData * pItem )
{
	pTableNewItem[iTableItemCompressPosition].sItemID = pItem->sBaseItemID;
	STRINGCOPY ( pTableNewItem[iTableItemCompressPosition].szBaseName, pItem->szBaseName );
	STRINGCOPY ( pTableNewItem[iTableItemCompressPosition].szInventoryName, pItem->szInventoryName );
	pTableNewItem[iTableItemCompressPosition].iWidth = pItem->iWidth;
	pTableNewItem[iTableItemCompressPosition].iHeight = pItem->iHeight;
	STRINGCOPY ( pTableNewItem[iTableItemCompressPosition].szDropFolder, pItem->szCategory );
	pTableNewItem[iTableItemCompressPosition].iClass = pItem->iItemSlotFlag;
	STRINGCOPY ( pTableNewItem[iTableItemCompressPosition].szDropName, pItem->szModelName );
	pTableNewItem[iTableItemCompressPosition].iModelPosition = pItem->iVisibleItemType;
	pTableNewItem[iTableItemCompressPosition].iSound = pItem->iInvItemSound;
	pTableNewItem[iTableItemCompressPosition].iWeaponClass = pItem->iItemWpnDmg;

}

void ItemServer::CreateItemMemoryTable ()
{
	if ( pAllocItemTable )
		return;

	ZeroMemory ( &OldItemDefinition, ITEM_SERVER_MAX * sizeof ( DefinitionItem ) );
	CreateItemMemoryTable ( "ItemListOld", &OldItemDefinition[0], FALSE );

	DefinitionItem * pDefItem = *(DefinitionItem **)0x07A9EB10;
	ZeroMemory ( (void *)pDefItem, ITEM_SERVER_MAX * sizeof ( DefinitionItem ) );

	// Create memory for Item Table
	{
		pAllocItemTable = (ItemData *)VirtualAlloc ( NULL, 0x314 * ( 1500 + 4 ), MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE );
		pTableNewItem = (CompressedItem *)VirtualAlloc ( NULL, 0x80 * ( 1500 + 4 ), MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE );
	}

	CreateItemMemoryTable ( "ItemList", pDefItem, TRUE );

}

void ItemServer::CreateItemMemoryTable ( const char * pszTableName, DefinitionItem * pDefItem, BOOL bIsLatestItems )
{
	DefinitionItem DefItem;

	SQLConnection * pcDB = SQLCONNECTION ( DATABASEID_GameDB, 2 );

	if ( bIsLatestItems )
		NUM_ITEMS = 0;

	unsigned int itemID;

	if ( pcDB->Open () )
	{
		//avoid using * so that we don't run into issues if adding a new column
		//or rearranging the column order
		std::string query = "SELECT TOP (%d) [ID],[IDCode],[Name],[nullcode],[CodeIMG1],[CodeIMG2],[Width],[Height]"
			",[DropFolder],[ClassItem],[ModelPosition],[Sound],[WeaponClass],[QuestID],[QuestR],[QuestG],[QuestB]"
			",[QuestTransparency],[QuestFlashingTime],[ReqLevel],[ReqStrengh],[ReqSpirit],[ReqTalent],[ReqAgility],[ReqHealth]"
			",[IntegrityMin],[IntegrityMax],[Weight],[Price],[OrganicMin],[OrganicMax],[FireMin],[FireMax],[FrostMin]"
			",[FrostMax],[LightningMix],[LightningMax],[PoisonMin],[PoisonMax],[ATKPow1Min],[ATKPow1Max],[ATKPow2Min]"
			",[ATKPow2Max],[Range],[ATKSpeed],[ATKRatingMin],[ATKRatingMax],[Critical],[BlockMin],[BlockMax],[AbsorbMin]"
			",[AbsorbMax],[DefenseMin],[DefenseMax],[PotionSpace],[PotionCount],[RegenerationHPMin],[RegenerationHPMax]"
			",[RegenerationMPMin],[RegenerationMPMax],[RegenerationSTMMin],[RegenerationSTMMax],[AddHPMin],[AddHPMax]"
			",[AddMPMin],[AddMPMax],[AddSTMMin],[AddSTMMax],[RecoveryHPMin],[RecoveryHPMax],[RecoveryMPMin],[RecoveryMPMax]"
			",[RecoverySTMMin],[RecoverySTMMax],[RunSpeedMin],[RunSpeedMax],[PrimarySpec],[AddSpecClass1],[AddSpecClass2]"
			",[AddSpecClass3],[AddSpecClass4],[AddSpecClass5],[AddSpecClass6],[AddSpecClass7],[AddSpecClass8],[AddSpecClass9]"
			",[AddSpecClass10],[AddSpecClass11],[AddSpecClass12],[AddSpecRunSpeedMin],[AddSpecRunSpeedMax],[AddSpecAbsorbMin]"
			",[AddSpecAbsorbMax],[AddSpecDefenseMin],[AddSpecDefenseMax],[AddSpecAtkSpeed],[AddSpecCritical],[AddSpecAtkPowerMin]"
			",[AddSpecAtkPowerMax],[AddSpecAtkRatingMin],[AddSpecAtkRatingMax],[AddSpecHPRegen],[AddSpecMPRegenMin],[AddSpecMPRegenMax]"
			",[AddSpecSTMRegen],[AddSpecBlock],[AddSpecRange],[CannotDrop]"
			" FROM %s"
			" ORDER BY ID ASC";

		if ( pcDB->Prepare ( FormatString(query.c_str(), ITEM_SERVER_MAX, pszTableName ) ) )
		{
			if ( pcDB->Execute () && pcDB->Fetch () )
			{
				for ( int i = 0; i < ITEM_SERVER_MAX; i++ )
				{
					// Code
					pcDB->GetData ( 2, PARAMTYPE_Integer, &itemID );

					// NAME
					pcDB->GetData ( 3, PARAMTYPE_String, pDefItem[i].sItem.szItemName, 32 );

					pDefItem[i].sItem.sItemID = (ItemID)itemID;

					// Item Table vars
					int iCode = 0;
					char szLastCategory[10] = { 0 };
					char szDropItem[64] = { 0 };
					int iWidth = 0;
					int iHeight = 0;
					char szItemPath[64] = { 0 };
					int iClass = 0;
					int iModelPosition = 0;
					int iSoundIndex = 0;
					int iWeaponClass = 0;
					{
						// Code
						iCode = pDefItem[i].sItem.sItemID.ToItemID ();

						// Last Category
						pcDB->GetData ( 5, PARAMTYPE_String, szLastCategory, 10 );

						// Drop Item Image
						pcDB->GetData ( 6, PARAMTYPE_String, szDropItem, 64 );

						// Width x Height
						pcDB->GetData ( 7, PARAMTYPE_Integer, &iWidth );
						pcDB->GetData ( 8, PARAMTYPE_Integer, &iHeight );

						// Path File Item
						pcDB->GetData ( 9, PARAMTYPE_String, szItemPath, 64 );

						// Class Item
						pcDB->GetData ( 10, PARAMTYPE_Integer, &iClass );

						// Model Position
						pcDB->GetData ( 11, PARAMTYPE_Integer, &iModelPosition );

						// Sound
						pcDB->GetData ( 12, PARAMTYPE_Integer, &iSoundIndex );

						// Weapon Class
						pcDB->GetData ( 13, PARAMTYPE_Integer, &iWeaponClass );
					}

					pcDB->GetData ( 14, PARAMTYPE_Integer, &pDefItem[i].sItem.iItemUniqueID );

					// Item Table
					{
						// Is in table?
						if ( !pDefItem[i].sItem.iItemUniqueID && bIsLatestItems )
						{
							// Code
							pAllocItemTable[iTableItemCompressPosition].sBaseItemID = pDefItem[i].sItem.sItemID;

							// Name
							STRINGCOPY ( pAllocItemTable[iTableItemCompressPosition].szBaseName, pDefItem[i].sItem.szItemName );

							// Last Category
							STRINGCOPY ( pAllocItemTable[iTableItemCompressPosition].szInventoryName, szLastCategory );

							// Drop Item Image
							STRINGCOPY ( pAllocItemTable[iTableItemCompressPosition].szModelName, szDropItem );

							// Width x Height
							pAllocItemTable[iTableItemCompressPosition].iWidth = iWidth;
							pAllocItemTable[iTableItemCompressPosition].iHeight = iHeight;

							// Path File Item
							STRINGCOPY ( pAllocItemTable[iTableItemCompressPosition].szCategory, szItemPath );

							// Class Item
							pAllocItemTable[iTableItemCompressPosition].iItemSlotFlag = (EItemSlotFlag)iClass;

							// Model Position
							pAllocItemTable[iTableItemCompressPosition].iVisibleItemType = (EItemSlotFlag)iModelPosition;

							// Sound
							pAllocItemTable[iTableItemCompressPosition].iInvItemSound = (EItemInvSound)iSoundIndex;

							// Weapon Class
							pAllocItemTable[iTableItemCompressPosition].iItemWpnDmg = iWeaponClass;

							ItemV += 0x314;

							CompressItemDataInTable ( &pAllocItemTable[iTableItemCompressPosition] );
							iTableItemCompressPosition++;
						}
					}

					// Quest
					{
						// R
						pcDB->GetData ( 15, PARAMTYPE_Short, &pDefItem[i].sItem.sEffectColor[0] );

						// G
						pcDB->GetData ( 16, PARAMTYPE_Short, &pDefItem[i].sItem.sEffectColor[1]);

						// B
						pcDB->GetData ( 17, PARAMTYPE_Short, &pDefItem[i].sItem.sEffectColor[2] );

						// A
						pcDB->GetData ( 18, PARAMTYPE_Short, &pDefItem[i].sItem.sEffectColor[3] );

						// Flashing Time
						pcDB->GetData ( 19, PARAMTYPE_Short, &pDefItem[i].sItem.sEffectBlink[0] );


					}

					// Req. Level
					pcDB->GetData ( 20, PARAMTYPE_Integer, &pDefItem[i].sItem.iLevel );

					// Req. Strength
					pcDB->GetData ( 21, PARAMTYPE_Integer, &pDefItem[i].sItem.iStrength );

					// Req. Spirit
					pcDB->GetData ( 22, PARAMTYPE_Integer, &pDefItem[i].sItem.iSpirit );

					// Req. Talent
					pcDB->GetData ( 23, PARAMTYPE_Integer, &pDefItem[i].sItem.iTalent );

					// Req. Agility
					pcDB->GetData ( 24, PARAMTYPE_Integer, &pDefItem[i].sItem.iAgility );

					// Req. Health
					pcDB->GetData ( 25, PARAMTYPE_Integer, &pDefItem[i].sItem.iHealth );

					// Integrity Min
					pcDB->GetData ( 26, PARAMTYPE_Short, &pDefItem[i].IntegrityMin );

					// Integrity Max
					pcDB->GetData ( 27, PARAMTYPE_Short, &pDefItem[i].IntegrityMax );

					// Weight
					pcDB->GetData ( 28, PARAMTYPE_Integer, &pDefItem[i].sItem.iWeight );

					// Price
					pcDB->GetData ( 29, PARAMTYPE_Integer, &pDefItem[i].sItem.iSalePrice );

					// Organic
					pcDB->GetData ( 30, PARAMTYPE_Short, &pDefItem[i].OrganicMin );
					pcDB->GetData ( 31, PARAMTYPE_Short, &pDefItem[i].OrganicMax );

					// Fire
					pcDB->GetData ( 32, PARAMTYPE_Short, &pDefItem[i].FireMin );
					pcDB->GetData ( 33, PARAMTYPE_Short, &pDefItem[i].FireMax );

					// Frost
					pcDB->GetData ( 34, PARAMTYPE_Short, &pDefItem[i].FrostMin );
					pcDB->GetData ( 35, PARAMTYPE_Short, &pDefItem[i].FrostMax );

					// Lightning
					pcDB->GetData ( 36, PARAMTYPE_Short, &pDefItem[i].LightningMin );
					pcDB->GetData ( 37, PARAMTYPE_Short, &pDefItem[i].LightningMax );

					// Poison
					pcDB->GetData ( 38, PARAMTYPE_Short, &pDefItem[i].PoisonMin );
					pcDB->GetData ( 39, PARAMTYPE_Short, &pDefItem[i].PoisonMax );

					// Attack Power 1
					pcDB->GetData ( 40, PARAMTYPE_Short, &pDefItem[i].AttackPower1Min );
					pcDB->GetData ( 41, PARAMTYPE_Short, &pDefItem[i].AttackPower1Max );

					// Attack Power 2
					pcDB->GetData ( 42, PARAMTYPE_Short, &pDefItem[i].AttackPower2Min );
					pcDB->GetData ( 43, PARAMTYPE_Short, &pDefItem[i].AttackPower2Max );

					// Shooting Range
					pcDB->GetData ( 44, PARAMTYPE_Integer, &pDefItem[i].sItem.iAttackRange );

					// Attack Speed
					pcDB->GetData ( 45, PARAMTYPE_Integer, &pDefItem[i].sItem.iAttackSpeed );

					// Attack Rating
					pcDB->GetData ( 46, PARAMTYPE_Short, &pDefItem[i].AttackRatingMin );
					pcDB->GetData ( 47, PARAMTYPE_Short, &pDefItem[i].AttackRatingMax );

					// Critical
					pcDB->GetData ( 48, PARAMTYPE_Integer, &pDefItem[i].sItem.iCritical );

					// Block
					pcDB->GetData ( 49, PARAMTYPE_Float, &pDefItem[i].BlockRatingMin );
					pcDB->GetData ( 50, PARAMTYPE_Float, &pDefItem[i].BlockRatingMax );

					// Absorb
					pcDB->GetData ( 51, PARAMTYPE_Float, &pDefItem[i].AbsorbMin );
					pcDB->GetData ( 52, PARAMTYPE_Float, &pDefItem[i].AbsorbMax );

					// Defense
					pcDB->GetData ( 53, PARAMTYPE_Short, &pDefItem[i].DefenseMin );
					pcDB->GetData ( 54, PARAMTYPE_Short, &pDefItem[i].DefenseMax );

					// Potions
					pcDB->GetData ( 55, PARAMTYPE_Integer, &pDefItem[i].sItem.iPotionStorage );

					// Potions
					pcDB->GetData ( 56, PARAMTYPE_Integer, &pDefItem[i].sItem.iPotionCount );

					// Regeneration HP, MP, STM
					pcDB->GetData ( 57, PARAMTYPE_Float, &pDefItem[i].HPRegenMin );
					pcDB->GetData ( 58, PARAMTYPE_Float, &pDefItem[i].HPRegenMax );
					pcDB->GetData ( 59, PARAMTYPE_Float, &pDefItem[i].MPRegenMin );
					pcDB->GetData ( 60, PARAMTYPE_Float, &pDefItem[i].MPRegenMax );
					pcDB->GetData ( 61, PARAMTYPE_Float, &pDefItem[i].STMRegenMin );
					pcDB->GetData ( 62, PARAMTYPE_Float, &pDefItem[i].STMRegenMax );

					// Add HP
					pcDB->GetData ( 63, PARAMTYPE_Integer, &pDefItem[i].AddHPMin );
					pcDB->GetData ( 64, PARAMTYPE_Integer, &pDefItem[i].AddHPMax );

					// Add MP
					pcDB->GetData ( 65, PARAMTYPE_Integer, &pDefItem[i].AddMPMin );
					pcDB->GetData ( 66, PARAMTYPE_Integer, &pDefItem[i].AddMPMax );

					// Add STM
					pcDB->GetData ( 67, PARAMTYPE_Integer, &pDefItem[i].AddSTMMin );
					pcDB->GetData ( 68, PARAMTYPE_Integer, &pDefItem[i].AddSTMMax );

					// Recovery HP
					pcDB->GetData ( 69, PARAMTYPE_Short, &pDefItem[i].sItem.sHPRecovery.sMin );
					pcDB->GetData ( 70, PARAMTYPE_Short, &pDefItem[i].sItem.sHPRecovery.sMax );

					// Recovery MP
					pcDB->GetData ( 71, PARAMTYPE_Short, &pDefItem[i].sItem.sMPRecovery.sMin );
					pcDB->GetData ( 72, PARAMTYPE_Short, &pDefItem[i].sItem.sMPRecovery.sMax );

					// Recovery STM
					pcDB->GetData ( 73, PARAMTYPE_Short, &pDefItem[i].sItem.sSPRecovery.sMin );
					pcDB->GetData ( 74, PARAMTYPE_Short, &pDefItem[i].sItem.sSPRecovery.sMax);

					// Run Speed
					pcDB->GetData ( 75, PARAMTYPE_Float, &pDefItem[i].RunSpeedMin );
					pcDB->GetData ( 76, PARAMTYPE_Float, &pDefItem[i].RunSpeedMax );

					// Specs
					pcDB->GetData ( 77, PARAMTYPE_Integer, &pDefItem[i].sItem.eSpecialization );
					pDefItem[i].sItem.eSpecialization = CharacterClassToClassFlag ( (ECharacterClass)pDefItem[i].sItem.eSpecialization ); //main spec

					for ( int j = 0; j < 12; j++ )
					{
						pcDB->GetData ( 78 + j, PARAMTYPE_Integer, &pDefItem[i].JobBitCodeRandom[pDefItem[i].JobBitCodeRandomCount] );
						if ( pDefItem[i].JobBitCodeRandom[pDefItem[i].JobBitCodeRandomCount] )
						{
							pDefItem[i].JobBitCodeRandom[pDefItem[i].JobBitCodeRandomCount] = CharacterClassToClassFlag ( (ECharacterClass)( j + 1 ) );
							pDefItem[i].JobBitCodeRandomCount++;
						}
					}

					// Spec Data
					{
						// Run Speed
						pcDB->GetData ( 90, PARAMTYPE_Float, &pDefItem[i].AddSpecRunSpeedMin );
						pcDB->GetData ( 91, PARAMTYPE_Float, &pDefItem[i].AddSpecRunSpeedMax );

						// Absorb
						pcDB->GetData ( 92, PARAMTYPE_Float, &pDefItem[i].AddSpecAbsorbMin );
						pcDB->GetData ( 93, PARAMTYPE_Float, &pDefItem[i].AddSpecAbsorbMax );

						// Defense
						pcDB->GetData ( 94, PARAMTYPE_Short, &pDefItem[i].AddSpecDefenseMin );
						pcDB->GetData ( 95, PARAMTYPE_Short, &pDefItem[i].AddSpecDefenseMax );

						// Attack Speed
						pcDB->GetData ( 96, PARAMTYPE_Integer, &pDefItem[i].sItem.sSpecData.iSpecAttackSpeed );

						// Critical
						pcDB->GetData ( 97, PARAMTYPE_Integer, &pDefItem[i].sItem.sSpecData.iSpecCritical );

						// Attack Power
						pcDB->GetData ( 98, PARAMTYPE_Short, &pDefItem[i].sItem.sSpecData.iSpecAttackPowerDivMin );
						pcDB->GetData ( 99, PARAMTYPE_Short, &pDefItem[i].sItem.sSpecData.iSpecAttackPowerDiv );

						// Attack Rating
						pcDB->GetData ( 100, PARAMTYPE_Integer, &pDefItem[i].SpecAttackRatingMin );
						pcDB->GetData ( 101, PARAMTYPE_Integer, &pDefItem[i].SpecAttackRatingMax );

						// HP Regen
						pcDB->GetData ( 102, PARAMTYPE_Float, &pDefItem[i].sItem.sSpecData.fSpecHPRegen );

						// MP Regen
						pcDB->GetData ( 103, PARAMTYPE_Float, &pDefItem[i].AddSpecMPRegenMin );
						pcDB->GetData ( 104, PARAMTYPE_Float, &pDefItem[i].AddSpecMPRegenMax );

						// STM Regen
						pcDB->GetData ( 105, PARAMTYPE_Float, &pDefItem[i].sItem.sSpecData.fSpecSPRegen );

						// Block
						pcDB->GetData ( 106, PARAMTYPE_Float, &pDefItem[i].sItem.sSpecData.fSpecBlockRating );

						// Shooting Range
						pcDB->GetData ( 107, PARAMTYPE_Integer, &pDefItem[i].sItem.sSpecData.iSpecAttackRange );

					}

					// Cannot Drop
					pcDB->GetData ( 108, PARAMTYPE_Integer, &pDefItem[i].sItem.bDisallowDrop );


					ItemServer::StoreItemMiscData ( pDefItem[i].sItem.sItemID.ToItemID (), pDefItem[i].sItem.iSalePrice, pDefItem[i].IntegrityMax, pDefItem[i].sItem.iLevel );

					typedef void ( __cdecl * t_SetLowerItem ) ( DefinitionItem * pItem );
					t_SetLowerItem SetLowerItem = (t_SetLowerItem)0x00439C70;

					memcpy ( &DefItem, &pDefItem[i], sizeof ( DefinitionItem ) );

					//code if you want to change rarity of npc shop items
					//const EItemBase itemBase = DefItem.sItem.sItemID.ToItemBase();
					//const EItemType itemType = DefItem.sItem.sItemID.ToItemType();
					//
					//if ( ( itemBase == EItemBase::ITEMBASE_Weapon || itemType == EItemType::ITEMTYPE_Armor || itemType == EItemType::ITEMTYPE_Robe
					//	|| itemType == EItemType::ITEMTYPE_Amulet || itemType == EItemType::ITEMTYPE_Shield || itemType == EItemType::ITEMTYPE_Boots
					//	|| itemType == EItemType::ITEMTYPE_Gauntlets || itemType == EItemType::ITEMTYPE_Bracelets
					//	|| itemType == EItemType::ITEMTYPE_Ring || itemType == EItemType::ITEMTYPE_Ring2 || itemType == EItemType::ITEMTYPE_Orb ))
					//{
					//	DefItem.sItem.eRarity = EItemRarity::COMMON;
					//	SetRarityBoostBasedOnItemBase( &DefItem, DefItem.sItem.eRarity );
					//}

					SetLowerItem ( &DefItem );

					DefItem.sItem.iBackupKey = 0;
					DefItem.sItem.iBackupChk = 0;
					DefItem.sItem.tTime = 0;

					typedef int ( __cdecl * t_EncodeCompress ) ( BYTE * lpSrcData, BYTE * lpDestData, int size );
					t_EncodeCompress EncodeCompress = (t_EncodeCompress)0x00445480;

					pDefItem[i].DefCompressDataLen = EncodeCompress ( (BYTE *)&DefItem.sItem, (BYTE *)pDefItem[i].DefCompressData, sizeof ( Item ) );

					if ( bIsLatestItems )
						NUM_ITEMS += 1;

					if ( !pcDB->Fetch () )
						break;
				}
			}
		}
		pcDB->Close ();
	}

	if ( bIsLatestItems )
	{
		allocIT = (void *)pAllocItemTable;
		endIT ();
		refIT ();

		// Create Item table compressed
		{
			FILE * fp;
			INI::CReader cReader ( "server.ini" );

			char szFilePath[256];
			STRINGFORMAT ( szFilePath, "%s\\items.dat", cReader.ReadString ( "Database", "NetFolder" ).c_str () );

			fopen_s ( &fp, szFilePath, "wb" );
			if ( fp != NULL )
			{
				fseek ( fp, 0, SEEK_SET );
				fwrite ( (void *)pTableNewItem, 1, 0x80 * ( ( *(UINT *)0x07AAC890 ) + 3 ), fp );
				fclose ( fp );
			}
		}
	}
}

/**
OnServer.cpp
int	rsDeleteInvenItem(rsPLAYINFO *lpPlayInfo, DWORD dwCode, DWORD dwHead, DWORD dwChkSum)
*/
BOOL ItemServer::DeleteItemInventory(UserData* pcUserData, Item* sItem)
{
	if ( GAME_SERVER )
	{
		//because the login server holds the character data, including inventory
		WARN( "DeleteItemInventory should be called on Login server only" );
	}

	return DeleteItemInventory(pcUserData, sItem->sItemID.ToInt(), sItem->iChk1, sItem->iChk2);
}


/**
OnServer.cpp
int	rsDeleteInvenItem(rsPLAYINFO *lpPlayInfo, DWORD dwCode, DWORD dwHead, DWORD dwChkSum)
*/
BOOL ItemServer::DeleteItemInventory( UserData * pcUserData, DWORD dwCode, DWORD dwHead, DWORD dwChkSum )
{
	if ( GAME_SERVER )
	{
		//because the login server holds the character data, including inventory
		WARN( "DeleteItemInventory should be called on Login server only" );
	}

	tfnDeleteItem fnDeleteItem = (tfnDeleteItem)pfnDeleteItem;
	INT iResult = fnDeleteItem( pcUserData, dwCode, dwHead, dwChkSum );
	if ( iResult >= 0 )
		return TRUE;
	return FALSE;
}

/*
OnServer.cpp
int	rsAddInvenItem(rsPLAYINFO *lpPlayInfo, DWORD dwCode, DWORD dwHead, DWORD dwChkSum)
*/
BOOL ItemServer::AddItemInventory(UserData* pcUserData, Item * sItem)
{
	if ( GAME_SERVER )
	{
		//because the login server holds the character data, including inventory
		WARN( "AddItemInventory should be called on Login server only" );
	}

	return AddItemInventory(pcUserData, sItem->sItemID.ToInt(), sItem->iChk1, sItem->iChk2);
}

/*
OnServer.cpp
int	rsAddInvenItem(rsPLAYINFO *lpPlayInfo, DWORD dwCode, DWORD dwHead, DWORD dwChkSum)
*/
BOOL ItemServer::AddItemInventory( UserData * pcUserData, DWORD dwCode, DWORD dwHead, DWORD dwChkSum )
{
	if ( GAME_SERVER )
	{
		//because the login server holds the character data, including inventory
		WARN( "AddItemInventory should be called on Login server only!" );
	}

	tfnAddItem fnAddItem = (tfnAddItem)pfnAddItem;
	INT iResult = fnAddItem( pcUserData, dwCode, dwHead, dwChkSum );
	if ( iResult >= 0 )
		return TRUE;

	return FALSE;
}

bool ItemServer::HasItemInInventory( UserData * pcUserData, ItemData * pcItemData, BOOL bCheckLastInventorySnapshot )
{
	if ( pcItemData == NULL )
		return false;

	return HasItemInInventory( pcUserData, pcItemData->sItem.sItemID.ToItemID(), pcItemData->sItem.iChk1, pcItemData->sItem.iChk2 );
}

/// <summary>
///
/// </summary>
/// <param name="pcUserData"></param>
/// <param name="p_ItemID"></param>
/// <param name="p_Chk1"></param>
/// <param name="p_Chk2"></param>
/// <param name="bCheckLastInventorySnapshot">Set to true to check the inventory state prior to the last PKTHDR_NetItemInventory to address some timing related issues such as throw item</param>
/// <returns></returns>
bool ItemServer::HasItemInInventory(UserData * pcUserData, EItemID p_ItemID, DWORD p_Chk1, DWORD p_Chk2, BOOL bCheckLastInventorySnapshot)
{
    bool l_ItemFound = false;
    int l_FindCount = 0;

    int l_ItemSlot = -1;
    for (l_ItemSlot = 0; l_ItemSlot < 316; l_ItemSlot++)
    {
        DropItemData & ps = pcUserData->sIntentoryItems[l_ItemSlot];

        if (ps.iItemID == p_ItemID && ps.iChk1 == p_Chk1 && ps.iChk2 == p_Chk2)
        {
            l_ItemFound = true;
            break;
        }
    }

	if ( l_ItemFound == false && bCheckLastInventorySnapshot )
	{
		User * pcUser = USERDATATOUSER ( pcUserData );

		for ( l_ItemSlot = 0; l_ItemSlot < 316; l_ItemSlot++ )
		{
			DropItemData & ps = pcUser->sLastInventoryItems[l_ItemSlot];

			if ( ps.iItemID == p_ItemID && ps.iChk1 == p_Chk1 && ps.iChk2 == p_Chk2 )
			{
				l_ItemFound = true;
				break;
			}
		}
	}



    return l_ItemFound;
}


EItemID ItemServer::GetWarpGateWing(User * pcUser )
{
	if ( pcUser == NULL || pcUser->IsValidAndInGame() == FALSE )
		return EItemID::ITEMID_None;

	bool l_ItemFound = false;
	int l_FindCount = 0;

	int l_ItemSlot = -1;
	for (l_ItemSlot = 0; l_ItemSlot < 316; l_ItemSlot++)
	{
		DropItemData & ps = pcUser->pcUserData->sIntentoryItems[l_ItemSlot];
		if (((ItemID)ps.iItemID).ToItemType() == EItemType::ITEMTYPE_Wing )
		{
			return (EItemID)ps.iItemID;
		}
	}

	return EItemID::ITEMID_None;
}

/**
Item authentication check
OnServer.cpp
CheckItemForm( sITEMINFO *lpItemInfo );
*/
BOOL ItemServer::CheckItemForm( Item * psItem )
{
	tfnCheckItem fnCheckItem = (tfnCheckItem)pfnCheckItemForm;
	return fnCheckItem( psItem );
}

//Item aging authentication code
//short GetMixItemForm( sITEMINFO *lpItemInfo )
//unsigned int __cdecl GetMixItemForm(int a1)
UINT ItemServer::GetMixItemForm(Item* psItem)
{
	tfnGetMixItemForm fnGetMixItemForm = (tfnGetMixItemForm)pfnGetMixItemForm;
	return fnGetMixItemForm(psItem);
}

//Item aging authentication code
//short GetMixItemForm( sITEMINFO *lpItemInfo )
//unsigned int __cdecl GetMixItemForm(int a1)
BOOL ItemServer::CheckMixItem(Item* psItem)
{
	tfnCheckMixItem fnCheckMixItem = (tfnCheckMixItem)pfnCheckMixItem;
	return fnCheckMixItem(psItem);
}
#ifdef ITEMFIXES
void ItemServer::CreateItem(Item* psItem, DefinitionItem* psDefItem, EItemSource eItemSource, int iSpeck, int iSpecAtkRating, int iAgeLevel, const EItemRarity itemRarity, BOOL isPerfect, int iPrefixID, int iSuffixID)
#else
void ItemServer::CreateItem(Item* psItem, DefinitionItem* psDefItem, EItemSource eItemSource, int iSpeck, int iSpecAtkRating, int iAgeLevel, const EItemRarity itemRarity)
#endif
{
	if ( psItem == NULL )
		return;

	// Copy the definition so we wouldn't add the values for each weapon that spawns.
	DefinitionItem definitionItemCopy = *psDefItem;

	const EItemID itemID = definitionItemCopy.sItem.sItemID.ToItemID();

	EItemRarity generatedItemRarity = itemRarity;

	if (ITEMSERVER->IsItemAbleToHaveRarity(&psDefItem->sItem) &&
		generatedItemRarity == EItemRarity::NONE )
	{
		ERarityChanceModType eModType = ERarityChanceModType::None;

		if ( eItemSource == EItemSource::BossKill )
			if (definitionItemCopy.sItem.iLevel >= 108)
				eModType = ERarityChanceModType::BossHighLevel;
			else
				eModType = ERarityChanceModType::Boss;

		if ( eItemSource == EItemSource::MimicKill )
			if (definitionItemCopy.sItem.iLevel >= 108)
				eModType = ERarityChanceModType::EventMimicHighLevel;
			else
				eModType = ERarityChanceModType::EventMimic;

		generatedItemRarity = ChooseItemRarity(definitionItemCopy.sItem.iLevel, eModType);
	}

	if (IsBossOrSpecialItem(itemID))
	{
		generatedItemRarity = EItemRarity::LEGENDARY;
	}
	else
	{
		SetRarityBoostBasedOnItemBase(&definitionItemCopy, generatedItemRarity);
#ifdef ITEMFIXES
		SetItemFixes ( &definitionItemCopy, generatedItemRarity, isPerfect, iPrefixID, iSuffixID );
#endif
	}

	//https://redmine.pristontale.eu/issues/150
	//for RARE or higher, ensure a spec
	//max 25 attempts
	//todo - rewrite CreateItem so we don't have to do this ugly hack..
	if (generatedItemRarity >= EItemRarity::RARE)
	{
		for (int i_Attempt = 0; i_Attempt < 25; i_Attempt++)
		{
			tfnCreateDefItem fnCreateItem = (tfnCreateDefItem)pfnCreateDefItem;
			fnCreateItem(psItem, &definitionItemCopy, iSpeck);

			//got a spec? If so, break out
			if (psItem->eSpecialization != 0)
			{
				break;
			}
		}
	}
	else
	{
		tfnCreateDefItem fnCreateItem = (tfnCreateDefItem)pfnCreateDefItem;
		fnCreateItem(psItem, &definitionItemCopy, iSpeck);
	}

	psItem->eRarity = generatedItemRarity;
	psItem->eItemOriginalSource = eItemSource;
	psItem->eItemLastSource = eItemSource;

	//If item matches one of these types
	switch (definitionItemCopy.sItem.sItemID.ToItemType())
	{
		case EItemType::ITEMTYPE_Axe:
		case EItemType::ITEMTYPE_Claw:
		case EItemType::ITEMTYPE_Hammer:
		case EItemType::ITEMTYPE_Scythe:
		case EItemType::ITEMTYPE_Sword:
		case EItemType::ITEMTYPE_Dagger:
		case EItemType::ITEMTYPE_Bow:
		case EItemType::ITEMTYPE_Javelin:
		case EItemType::ITEMTYPE_Wand:
		case EItemType::ITEMTYPE_Phantom:
		case EItemType::ITEMTYPE_Armor:
		case EItemType::ITEMTYPE_Robe:
		case EItemType::ITEMTYPE_Shield:
		case EItemType::ITEMTYPE_Orb:
		case EItemType::ITEMTYPE_Amulet:
		case EItemType::ITEMTYPE_Ring:
		case EItemType::ITEMTYPE_Sheltom:
		case EItemType::ITEMTYPE_Gauntlets:
		case EItemType::ITEMTYPE_Bracelets:
		case EItemType::ITEMTYPE_Boots:
		{

			int iTotalStats = iaCharTotalStats[psItem->iLevel];

			int iClass = ClassFlagToCharacterClass((EClassFlag)psItem->eSpecialization);

			StatModifier CurrentSpecStatMod;
			StatModifier CurrentClassStatMod{};

			int iRarityBaselineMod;

			switch (psItem->eRarity)
			{
				case EItemRarity::NONE:
					iRarityBaselineMod = (int)ERarityBaselineMod::None;
					break;
				case EItemRarity::COMMON:
					iRarityBaselineMod = (int)ERarityBaselineMod::Common;
					break;
				case EItemRarity::UNCOMMON:
					iRarityBaselineMod = (int)ERarityBaselineMod::Uncommon;
					break;
				case EItemRarity::RARE:
					iRarityBaselineMod = (int)ERarityBaselineMod::Rare;
					break;
				case EItemRarity::EPIC:
					iRarityBaselineMod = (int)ERarityBaselineMod::Epic;
					break;
				case EItemRarity::LEGENDARY:
					iRarityBaselineMod = (int)ERarityBaselineMod::Legendary;
					break;
			}


			//Class Stats Default (in case it doesnt match any criteria below, to ensure an item always has some kind of stat req)
			CurrentClassStatMod.iStrength = 25;
			CurrentClassStatMod.iSpirit = 23;
			CurrentClassStatMod.iTalent = 28;
			CurrentClassStatMod.iAgility = 23;
			CurrentClassStatMod.iHealth = 0;

			if (psItem->eSpecialization == 0)
			{
				//For NS items assign the baseline stats of the associated class of the item, or the average baseline if used by multiple classes
				for (ItemBaselineMod* CurrentClassBaselineMod : ItemBaselineModList)
				{
					switch (definitionItemCopy.sItem.sItemID.ToItemType())
					{
						case EItemType::ITEMTYPE_Scythe:
						{
							if (CurrentClassBaselineMod->iClass == 4)
							{
								CurrentClassStatMod = CurrentClassBaselineMod->eStatModifier;
								break;
							}
						} break;
						case EItemType::ITEMTYPE_Axe:
						{
							if (CurrentClassBaselineMod->iClass == 1)
							{
								CurrentClassStatMod = CurrentClassBaselineMod->eStatModifier;
								break;
							}
						} break;
						case EItemType::ITEMTYPE_Claw:
						{
							if (CurrentClassBaselineMod->iClass == 2)
							{
								CurrentClassStatMod = CurrentClassBaselineMod->eStatModifier;
								break;
							}
						} break;
						case EItemType::ITEMTYPE_Hammer:
						{
							if (CurrentClassBaselineMod->iClass == 2)
							{
								CurrentClassStatMod = CurrentClassBaselineMod->eStatModifier;
								break;
							}
						} break;
						case EItemType::ITEMTYPE_Sword:
						{
							if (CurrentClassBaselineMod->iClass == 6)
							{
								CurrentClassStatMod = CurrentClassBaselineMod->eStatModifier;
								break;
							}
						} break;
						case EItemType::ITEMTYPE_Dagger:
						{
							if (CurrentClassBaselineMod->iClass == 9)
							{
								CurrentClassStatMod = CurrentClassBaselineMod->eStatModifier;
								break;
							}
						} break;
						case EItemType::ITEMTYPE_Bow:
						{
							if (CurrentClassBaselineMod->iClass == 3)
							{
								CurrentClassStatMod = CurrentClassBaselineMod->eStatModifier;
								break;
							}
						} break;
						case EItemType::ITEMTYPE_Javelin:
						{
							if (CurrentClassBaselineMod->iClass == 5)
							{
								CurrentClassStatMod = CurrentClassBaselineMod->eStatModifier;
								break;
							}
						} break;
						case EItemType::ITEMTYPE_Wand:
						{
							if (CurrentClassBaselineMod->iClass == 7)
							{
								CurrentClassStatMod = CurrentClassBaselineMod->eStatModifier;
								break;
							}
						} break;
						case EItemType::ITEMTYPE_Phantom:
						{
							if (CurrentClassBaselineMod->iClass == 10)
							{
								CurrentClassStatMod = CurrentClassBaselineMod->eStatModifier;
								break;
							}
						} break;
						case EItemType::ITEMTYPE_Armor:
						{
							CurrentClassStatMod.iStrength =	30		;
							CurrentClassStatMod.iSpirit =	16		;
							CurrentClassStatMod.iTalent =	28		;
							CurrentClassStatMod.iAgility =	25		;
							CurrentClassStatMod.iHealth =	0		;
						} break;
						case EItemType::ITEMTYPE_Robe:
						{
							CurrentClassStatMod.iStrength = 14;
							CurrentClassStatMod.iSpirit = 39;
							CurrentClassStatMod.iTalent = 30;
							CurrentClassStatMod.iAgility = 17;
							CurrentClassStatMod.iHealth =	0		;
						} break;
						case EItemType::ITEMTYPE_Shield:
						{
							CurrentClassStatMod.iStrength = 28;
							CurrentClassStatMod.iSpirit = 16;
							CurrentClassStatMod.iTalent = 27;
							CurrentClassStatMod.iAgility = 25;
							CurrentClassStatMod.iHealth =	0		;
						} break;
						case EItemType::ITEMTYPE_Orb:
						{
							CurrentClassStatMod.iStrength = 13;
							CurrentClassStatMod.iSpirit = 40;
							CurrentClassStatMod.iTalent = 31;
							CurrentClassStatMod.iAgility = 18;
							CurrentClassStatMod.iHealth =	0		;
						} break;
						case EItemType::ITEMTYPE_Amulet:
						case EItemType::ITEMTYPE_Ring:
						case EItemType::ITEMTYPE_Sheltom:
						case EItemType::ITEMTYPE_Gauntlets:
						case EItemType::ITEMTYPE_Bracelets:
						case EItemType::ITEMTYPE_Boots:
						{
							CurrentClassStatMod.iStrength = 25;
							CurrentClassStatMod.iSpirit = 23;
							CurrentClassStatMod.iTalent = 28;
							CurrentClassStatMod.iAgility = 23;
							CurrentClassStatMod.iHealth =	0		;
						} break;
					}

				}

				CurrentSpecStatMod = GetNSStatMod(definitionItemCopy.sItem.sItemID.ToItemType());
			}
			else
			{
				for (ItemBaselineMod* CurrentClassBaselineMod : ItemBaselineModList)
				{
					if (CurrentClassBaselineMod->iClass == iClass)
					{
						CurrentClassStatMod = CurrentClassBaselineMod->eStatModifier;
						break;
					}
				}

				char* chTypeName = GetTypeChar(definitionItemCopy.sItem.sItemID.ToItemType());
				CurrentSpecStatMod = GetItemSpecStatMod(iClass, chTypeName);

			}

			ITEMSERVER->ReformItem(psItem);
		
		} break;
	}

	//JLM - Fix issue where Lvl/1 has a high chance of happening
	//I called GetItem 10 times, and all swords had level/1 for spec attack rate???
	//level /1 should be more rare than level /2 and level /3
	if ( psItem->sSpecData.iSpecAttackRatingDiv > 0)
	{
		if (iSpecAtkRating >= 1 && iSpecAtkRating <= 3)
		{
			psItem->sSpecData.iSpecAttackRatingDiv = iSpecAtkRating;
		}
		else
		{
			int ran = Dice::RandomI(0, 99);

			//20% chance (0 - 19)
			if (ran < 20)
			{
				psItem->sSpecData.iSpecAttackRatingDiv = 1;
			}

			//30% chance (20 - 49)
			else if (ran < 50)
			{
				psItem->sSpecData.iSpecAttackRatingDiv = 2;
			}

			//50% chance (50 - 99)
			else
			{
				psItem->sSpecData.iSpecAttackRatingDiv = 3;
			}
		}

		ITEMSERVER->ReformItem(psItem);
	}

	//DEBUG( "Item '%s' created with eItemFlag of %d", psItem->szItemName, psItem->eItemFlags );

	if (psItem->eCraftType == ITEMCRAFTTYPE_None &&
		psItem->sMixColorId != 0)
	{
		psItem->sMixColorId = 0;
		psItem->sMixUniqueID1 = 0;
		ITEMSERVER->ReformItem(psItem);
	}


	if (iAgeLevel >= 1 && iAgeLevel <= 20)
	{
		psItem->eCraftType = ITEMCRAFTTYPE_Aging;
		AGEHANDLER->OnUpAgeHandler(psItem, iAgeLevel); //will also validate item
	}
}

StatModifier ItemServer::GetNSStatMod(EItemType iItemType)
{
	StatModifier ReturnStatModifier;

	switch (iItemType)
	{
	case EItemType::ITEMTYPE_Scythe:
	case EItemType::ITEMTYPE_Axe:
	{
		ReturnStatModifier.iStrength =	92;
		ReturnStatModifier.iSpirit =	50;
		ReturnStatModifier.iTalent =	100;
		ReturnStatModifier.iAgility =	56;
		ReturnStatModifier.iHealth =	0;
	} break;
	case EItemType::ITEMTYPE_Claw:
	{
		ReturnStatModifier.iStrength =	108		;
		ReturnStatModifier.iSpirit =	40		;
		ReturnStatModifier.iTalent =	100		;
		ReturnStatModifier.iAgility =	53		;
		ReturnStatModifier.iHealth =	109		;
	} break;
	case EItemType::ITEMTYPE_Hammer:
	{
		ReturnStatModifier.iStrength =	161		;
		ReturnStatModifier.iSpirit =	53		;
		ReturnStatModifier.iTalent =	114		;
		ReturnStatModifier.iAgility =	51		;
		ReturnStatModifier.iHealth =	0		;
	} break;
	case EItemType::ITEMTYPE_Sword:
	{
		ReturnStatModifier.iStrength =	92		;
		ReturnStatModifier.iSpirit =	65		;
		ReturnStatModifier.iTalent =	100		;
		ReturnStatModifier.iAgility =	62		;
		ReturnStatModifier.iHealth =	0		;
	} break;
	case EItemType::ITEMTYPE_Dagger:
	{
		ReturnStatModifier.iStrength =	92		;
		ReturnStatModifier.iSpirit =	65		;
		ReturnStatModifier.iTalent =	100		;
		ReturnStatModifier.iAgility =	59		;
		ReturnStatModifier.iHealth =	0		;
	} break;
	case EItemType::ITEMTYPE_Bow:
	case EItemType::ITEMTYPE_Javelin:
	{
		ReturnStatModifier.iStrength =	60		;
		ReturnStatModifier.iSpirit =	62		;
		ReturnStatModifier.iTalent =	100		;
		ReturnStatModifier.iAgility =	100		;
		ReturnStatModifier.iHealth =	0		;
	} break;
	case EItemType::ITEMTYPE_Wand:
	{
		ReturnStatModifier.iStrength =	53		;
		ReturnStatModifier.iSpirit =	90		;
		ReturnStatModifier.iTalent =	100		;
		ReturnStatModifier.iAgility =	48		;
		ReturnStatModifier.iHealth =	0		;
	} break;
	case EItemType::ITEMTYPE_Phantom:
	{
		ReturnStatModifier.iStrength =	62		;
		ReturnStatModifier.iSpirit =	90		;
		ReturnStatModifier.iTalent =	100		;
		ReturnStatModifier.iAgility =	48		;
		ReturnStatModifier.iHealth =	0		;
	} break;
	case EItemType::ITEMTYPE_Armor:
	{
		ReturnStatModifier.iStrength =	92		;
		ReturnStatModifier.iSpirit =	0		;
		ReturnStatModifier.iTalent =	87		;
		ReturnStatModifier.iAgility =	0		;
		ReturnStatModifier.iHealth =	0		;
	} break;
	case EItemType::ITEMTYPE_Robe:
	{
		ReturnStatModifier.iStrength = 0;
		ReturnStatModifier.iSpirit = 100;
		ReturnStatModifier.iTalent = 80;
		ReturnStatModifier.iAgility = 0;
		ReturnStatModifier.iHealth =	0		;
	} break;
	case EItemType::ITEMTYPE_Shield:
	{
		ReturnStatModifier.iStrength = 86;
		ReturnStatModifier.iSpirit = 0;
		ReturnStatModifier.iTalent = 76;
		ReturnStatModifier.iAgility = 0;
		ReturnStatModifier.iHealth =	0		;
	} break;
	case EItemType::ITEMTYPE_Orb:
	{
		ReturnStatModifier.iStrength = 0;
		ReturnStatModifier.iSpirit = 80;
		ReturnStatModifier.iTalent = 70;
		ReturnStatModifier.iAgility = 0;
		ReturnStatModifier.iHealth =	0		;
	} break;
	case EItemType::ITEMTYPE_Amulet:
	{
		ReturnStatModifier.iStrength = 0;
		ReturnStatModifier.iSpirit = 75;
		ReturnStatModifier.iTalent = 45;
		ReturnStatModifier.iAgility = 0;
		ReturnStatModifier.iHealth =	0		;
	} break;
	case EItemType::ITEMTYPE_Ring:
	{
		ReturnStatModifier.iStrength = 0;
		ReturnStatModifier.iSpirit = 70;
		ReturnStatModifier.iTalent = 50;
		ReturnStatModifier.iAgility = 0;
		ReturnStatModifier.iHealth =	0		;
	} break;
	case EItemType::ITEMTYPE_Sheltom:
	{
		ReturnStatModifier.iStrength = 0;
		ReturnStatModifier.iSpirit = 80;
		ReturnStatModifier.iTalent = 35;
		ReturnStatModifier.iAgility = 0;
		ReturnStatModifier.iHealth =	0		;
	} break;
	case EItemType::ITEMTYPE_Gauntlets:
	{
		ReturnStatModifier.iStrength = 80;
		ReturnStatModifier.iSpirit = 51;
		ReturnStatModifier.iTalent = 84;
		ReturnStatModifier.iAgility = 77;
		ReturnStatModifier.iHealth =	0		;
	} break;
	case EItemType::ITEMTYPE_Bracelets:
	{
		ReturnStatModifier.iStrength = 84;
		ReturnStatModifier.iSpirit = 51;
		ReturnStatModifier.iTalent = 79;
		ReturnStatModifier.iAgility = 70;
		ReturnStatModifier.iHealth =	0		;
	} break;
	case EItemType::ITEMTYPE_Boots:
	{
		ReturnStatModifier.iStrength = 77;
		ReturnStatModifier.iSpirit = 51;
		ReturnStatModifier.iTalent = 87;
		ReturnStatModifier.iAgility = 86;
		ReturnStatModifier.iHealth =	0		;
	} break;
	}

	return ReturnStatModifier;
}

StatModifier ItemServer::GetItemSpecStatMod(int iSpec, char * chItemType)
{
	StatModifier ReturnItemSpecStatMod;

	for (ItemSpecMod * CurrentMod : ItemSpecModList)
	{
		if (strcmp(CurrentMod->sItemType, chItemType) == 0)
		{
			if (iSpec == CHARACTERCLASS_Fighter && CurrentMod->iSpecClass01 == 1)
			{
				ReturnItemSpecStatMod = CurrentMod->eStatModifier;
				break;
			}
			else if (iSpec == CHARACTERCLASS_Mechanician && CurrentMod->iSpecClass02 == 1)
			{
				ReturnItemSpecStatMod = CurrentMod->eStatModifier;
				break;
			}
			else if (iSpec == CHARACTERCLASS_Archer && CurrentMod->iSpecClass03 == 1)
			{
				ReturnItemSpecStatMod = CurrentMod->eStatModifier;
				break;
			}
			else if (iSpec == CHARACTERCLASS_Pikeman && CurrentMod->iSpecClass04 == 1)
			{
				ReturnItemSpecStatMod = CurrentMod->eStatModifier;
				break;
			}
			else if (iSpec == CHARACTERCLASS_Atalanta && CurrentMod->iSpecClass05 == 1)
			{
				ReturnItemSpecStatMod = CurrentMod->eStatModifier;
				break;
			}
			else if (iSpec == CHARACTERCLASS_Knight && CurrentMod->iSpecClass06 == 1)
			{
				ReturnItemSpecStatMod = CurrentMod->eStatModifier;
				break;
			}
			else if (iSpec == CHARACTERCLASS_Magician && CurrentMod->iSpecClass07 == 1)
			{
				ReturnItemSpecStatMod = CurrentMod->eStatModifier;
				break;
			}
			else if (iSpec == CHARACTERCLASS_Priestess && CurrentMod->iSpecClass08 == 1)
			{
				ReturnItemSpecStatMod = CurrentMod->eStatModifier;
				break;
			}
			else if (iSpec == CHARACTERCLASS_Assassin && CurrentMod->iSpecClass09 == 1)
			{
				ReturnItemSpecStatMod = CurrentMod->eStatModifier;
				break;
			}
			else if (iSpec == CHARACTERCLASS_Shaman && CurrentMod->iSpecClass10 == 1)
			{
				ReturnItemSpecStatMod = CurrentMod->eStatModifier;
				break;
			}
		}
	}

	return ReturnItemSpecStatMod;
}

char * ItemServer::GetTypeChar(EItemType iItemType)
{
	char * chReturnValue = "";

	switch (iItemType)
	{
	case EItemType::ITEMTYPE_Axe:
	case EItemType::ITEMTYPE_Hammer:
	case EItemType::ITEMTYPE_Scythe:
	case EItemType::ITEMTYPE_Sword:
	case EItemType::ITEMTYPE_Dagger:
		chReturnValue = "WeaponMelee";
		break;
	case EItemType::ITEMTYPE_Claw:
		chReturnValue = "WeaponClaw";
		break;
	case EItemType::ITEMTYPE_Bow:
	case EItemType::ITEMTYPE_Javelin:
		chReturnValue = "WeaponRange";
		break;
	case EItemType::ITEMTYPE_Wand:
	case EItemType::ITEMTYPE_Phantom:
		chReturnValue = "WeaponMagic";
		break;
	case EItemType::ITEMTYPE_Armor:
		chReturnValue = "Armor";
		break;
	case EItemType::ITEMTYPE_Robe:
		chReturnValue = "Robe";
		break;
	case EItemType::ITEMTYPE_Shield:
		chReturnValue = "Shield";
		break;
	case EItemType::ITEMTYPE_Orb:
		chReturnValue = "Orb";
		break;
	case EItemType::ITEMTYPE_Amulet:
		chReturnValue = "Amulet";
		break;
	case EItemType::ITEMTYPE_Ring:
		chReturnValue = "Ring";
		break;
	case EItemType::ITEMTYPE_Sheltom:
		chReturnValue = "Sheltom";
		break;
	case EItemType::ITEMTYPE_Gauntlets:
		chReturnValue = "Gauntlets";
		break;
	case EItemType::ITEMTYPE_Bracelets:
		chReturnValue = "Bracelets";
		break;
	case EItemType::ITEMTYPE_Boots:
		chReturnValue = "Boots";
		break;
	}

	return chReturnValue;
}


void ItemServer::CreateItem1( User * pcUser, EItemID eItem, EItemSource eItemSource, int iCount )
{
	if ( pcUser == NULL || pcUser->IsValidAndInGame () == FALSE )
		return;

	tfnCreateItemCount fnCreateItemCount = (tfnCreateItemCount)pfnCreateItemCount;
	fnCreateItemCount ( pcUser->pcUserData, eItem, iCount );

}

void ItemServer::CreatePerfectItem( Item * psOut, DefinitionItem * psIn, EItemSource eItemSource, int iSpeck, int iSpecAtkRating, int iAgeLevel )
{
	*( UINT* )0x8B70264 = 1;
	*( UINT* )0x8B70268 = iSpeck;

	CreateItem( psOut, psIn, eItemSource, iSpeck, iSpecAtkRating, iAgeLevel );

	*( UINT* )0x8B70264 = 0;
	*( UINT* )0x8B70268 = 0;
	psOut->iItemSpecialType = 1;
}

/// <summary>
/// Added potion to server potion
/// For cheat monitoring
/// int rsAddServerPotion(rsPLAYINFO *lpPlayInfo, DWORD dwPotionCode, int PotionCount)
int ItemServer::AddPotion( UserData * pcUserData, int iItemID, int iCount )
{
	return (int)CALL_WITH_ARG3( 0x005510F0, (DWORD)pcUserData, iItemID, iCount );
}

BOOL ItemServer::GetDropPotion( UserData * pcUserData, int iItemID )
{
	for ( int i = 0; i < pcUserData->iTradePotion; i++ )
	{
		if ( pcUserData->iaItemTradePotionID[i] == iItemID )
			return TRUE;
	}

	return FALSE;
}

BOOL ItemServer::AddDropPotion( UserData * pcUserData, int iItemID )
{
	if ( pcUserData->iTradePotion >= 16 )
		return FALSE;

	if ( GetDropPotion( pcUserData, iItemID ) == TRUE )
		return TRUE;

	pcUserData->iaItemTradePotionID[pcUserData->iTradePotion++] = iItemID;

	return TRUE;
}


void ItemServer::CreatePremium( User * pcUser, EItemID iItemID, int iType, DWORD dwTime, BOOL send )
{
	//don't use isValidAndInGame here, as new premiums is created at char creation
	if ( pcUser == NULL || pcUser->pcUserData == NULL || pcUser->pcUserData->pcSocketData == NULL )
		return;

	PacketNewItemTimer s;
	s.iLength		= sizeof( PacketNewItemTimer );
	s.iHeader		= PKTHDR_SetItemTimer;
	s.iItemID		= iItemID;
	s.iType			= (EItemTimerType)iType;
	s.dwTimeLeft	= dwTime;
	s.dwTimeTotal	= dwTime;

	DWORD i1, i2;

	EItemID e;

	if ( SQLGetItemTimer( pcUser, s.iType, e, i1, i2 ) == FALSE )
	{
		if (AddItemTimer(pcUser, s.iType, iItemID, dwTime))
		{
			INFO ( "Premium %d created for player '%s'", iItemID,  pcUser->GetCharacterName() );

			if ( send )
			{
				SENDPACKET ( pcUser, &s, TRUE );
			}
		}
	}
	else
	{
		WARN ( "CreatePremium failed for player '%s'. Item already exists in database: %d", pcUser->GetCharacterName(), iItemID );
	}
}

void ItemServer::SetAgingItem( ItemData * psItemData )
{
	//std::cout << "ItemServer::SetAgingItem" << std::endl;

	tfnIncreaseAging fnIncreaseAging = (tfnIncreaseAging)pfnIncreaseAging;
	fnIncreaseAging( &psItemData->sItem );
	psItemData->sItem.eCraftType = EItemCraftType::ITEMCRAFTTYPE_Aging;
	tfnSetAgingChecksum fnSetAgingChecksum = (tfnSetAgingChecksum)pfnSetAgingChecksum;
	fnSetAgingChecksum( &psItemData->sItem );
}

int ItemServer::DecodeItemsData( void * pItemData, void * pItemDest, int iSize )
{
	typedef int( __cdecl *tfnDecodeItems ) (void * pItemData, void * pItemDest, int iSize);
	tfnDecodeItems fnDecodeItems = (tfnDecodeItems)0x004453F0;
	return fnDecodeItems( pItemData, pItemDest, iSize );
}

int ItemServer::EncodeItemsData( void * pItemData, void * pItemDest, int iSize )
{
	typedef int( __cdecl *tfnEncodeItems ) (void * pItemData, void * pItemDest, int iSize);
	tfnEncodeItems fnEncodeItems = (tfnEncodeItems)0x00445480;
	return fnEncodeItems( pItemData, pItemDest, iSize );
}

int ItemServer::EncodeItemsData( void * pItemData, void * pItemDest, int iSize, int iSizeDest )
{
	typedef int( __cdecl *tfnEncodeItems ) (void * pItemData, void * pItemDest, int iSize, int iSizeDest);
	tfnEncodeItems fnEncodeItems = (tfnEncodeItems)0x004451E0;
	return fnEncodeItems( pItemData, pItemDest, iSize, iSizeDest );
}

/**
This will in turn call BOOL __cdecl OnSaveItemData( Item * psItem )
in DLL.cpp which basically saves the item to database (ItemCreateLog)
*/
void ItemServer::SaveItemDataToDatabase( Item * psItem )
{
	tfnSaveItemData fnSaveItemData = (tfnSaveItemData)pfnSaveItemData;
	fnSaveItemData( psItem );
}


PremiumItem * ItemServer::GetPremiumInfo( DWORD dwCode )
{

	return NULL;
}

void ItemServer::SendItemTimer( User * pcUser, EItemID iItemID, EItemTimerType iType, DWORD dwTimeLeft, DWORD dwTimeTotal )
{
	if ( pcUser && pcUser->pcUserData )
	{
		PacketNewItemTimer s;
		s.iLength		= sizeof( PacketNewItemTimer );
		s.iHeader		= PKTHDR_SetItemTimer;
		s.iItemID		= iItemID;
		s.iType			= iType;
		s.dwTimeLeft	= dwTimeLeft;
		s.dwTimeTotal	= dwTimeTotal;
		SENDPACKET( pcUser, &s );
	}
}

BOOL ItemServer::CheckQuestOrEventItemAging( const char * pszItemName, int iItemID )
{
	if ( iItemID != 0 )
	{
		switch ( iItemID )
		{
			case ITEMID_PotionAgingQuestBee:

			case ITEMID_QuestWeapon_DemonHunter:
			case ITEMID_QuestWeapon_AncientRevenge:
			case ITEMID_QuestWeapon_DarkSkull:
			case ITEMID_QuestWeapon_DevilScythe:
			case ITEMID_QuestWeapon_Mist:
			case ITEMID_QuestWeapon_PlatinumSword:
			case ITEMID_QuestWeapon_Anaconda:
			case ITEMID_QuestWeapon_PlatinumDagger:
			case ITEMID_QuestWeapon_HutanPhantom:
				return TRUE;

			case ITEMID_StarWarsBow:
			case ITEMID_StarWarsBow90:
			case ITEMID_StarWarsBow100:
			case ITEMID_StarWarsBow110:
			case ITEMID_StarWarsDagger:
			case ITEMID_StarWarsDagger90:
			case ITEMID_StarWarsDagger100:
			case ITEMID_StarWarsDagger110:
			case ITEMID_StarWarsJavelin:
			case ITEMID_StarWarsJavelin90:
			case ITEMID_StarWarsJavelin100:
			case ITEMID_StarWarsJavelin110:
			case ITEMID_StarWarsPhantom:
			case ITEMID_StarWarsPhantom90:
			case ITEMID_StarWarsPhantom100:
			case ITEMID_StarWarsPhantom110:
			case ITEMID_StarWarsScythe:
			case ITEMID_StarWarsScythe90:
			case ITEMID_StarWarsScythe100:
			case ITEMID_StarWarsScythe110:
			case ITEMID_StarWarsStaff:
			case ITEMID_StarWarsStaff90:
			case ITEMID_StarWarsStaff100:
			case ITEMID_StarWarsStaff110:
			case ITEMID_StarWarsLightSaber1H:
			case ITEMID_StarWarsLightSaber1H90:
			case ITEMID_StarWarsLightSaber1H100:
			case ITEMID_StarWarsLightSaber1H110:
			case ITEMID_StarWarsLightSaber2H:
			case ITEMID_StarWarsLightSaber2H90:
			case ITEMID_StarWarsLightSaber2H100:
			case ITEMID_StarWarsLightSaber2H110:
				return TRUE;
		}
	}


	if ( (pszItemName != NULL) && (pszItemName[0] != 0) )
	{
		if ( STRINGCOMPAREI( pszItemName, "Demon Hunter" ) )
			return TRUE;
		if ( STRINGCOMPAREI( pszItemName, "Ancient Revenge" ) )
			return TRUE;
		if ( STRINGCOMPAREI( pszItemName, "Dark Skull" ) )
			return TRUE;
		if ( STRINGCOMPAREI( pszItemName, "Devil Scythe" ) )
			return TRUE;
		if ( STRINGCOMPAREI( pszItemName, "Mist" ) )
			return TRUE;
		if ( STRINGCOMPAREI( pszItemName, "Platinum Sword" ) )
			return TRUE;
		if ( STRINGCOMPAREI( pszItemName, "Anaconda" ) )
			return TRUE;
		if ( STRINGCOMPAREI( pszItemName, "Platinum Dagger" ) )
			return TRUE;
		if ( STRINGCOMPAREI( pszItemName, "Hutan Phantom" ) )
			return TRUE;
	}

	return FALSE;
}

void ItemServer::OnSetItemPerfect( Item * pItem )
{
	if ( !pItem )
		return;

	int iCharacterClass = ClassFlagToCharacterClass((EClassFlag)pItem->eSpecialization);

	// Set Status
	{
		// Get Minor
		int iLevel		= low( saItemRequeriments[iCharacterClass].sLevel[0],    saItemRequeriments[iCharacterClass].sLevel[1] );
		int iStrength	= low( saItemRequeriments[iCharacterClass].sStrength[0], saItemRequeriments[iCharacterClass].sStrength[1] );
		int iSpirit		= low( saItemRequeriments[iCharacterClass].sSpirit[0],   saItemRequeriments[iCharacterClass].sSpirit[1] );
		int iTalent		= low( saItemRequeriments[iCharacterClass].sTalent[0],   saItemRequeriments[iCharacterClass].sTalent[1] );
		int iAgility	= low( saItemRequeriments[iCharacterClass].sAgility[0],  saItemRequeriments[iCharacterClass].sAgility[1] );
		int iHealth		= low( saItemRequeriments[iCharacterClass].sHealth[0],   saItemRequeriments[iCharacterClass].sHealth[1] );


		if ( pItem->iLevel && iLevel )
			pItem->iLevel += ( pItem->iLevel * iLevel ) / 100;

		if ( pItem->iStrength && iStrength )
			pItem->iStrength += ( pItem->iStrength * iStrength ) / 100;

		if ( pItem->iSpirit && iSpirit )
			pItem->iSpirit += ( pItem->iSpirit * iSpirit ) / 100;

		if ( pItem->iTalent && iTalent )
			pItem->iTalent += ( pItem->iTalent * iTalent ) / 100;

		if ( pItem->iAgility && iAgility )
			pItem->iAgility += ( pItem->iAgility * iAgility ) / 100;

		if ( pItem->iHealth && iHealth )
			pItem->iHealth += ( pItem->iHealth * iHealth ) / 100;
	}

	return;
}


BOOL ItemServer::CheckAndAdjustItemRequirements ( Item * pItem, DefinitionItem * pDefItem )
{
	if ( !pItem )
		return FALSE;

	BOOL bAdjustedAnything = FALSE;

	//Here Ashe


	int iStrengthLow = pDefItem->sItem.iStrength;
	int iSpiritLow = pDefItem->sItem.iSpirit;
	int iTalentLow = pDefItem->sItem.iTalent;
	int iAgilityLow = pDefItem->sItem.iAgility;
	int iHealthLow = pDefItem->sItem.iHealth;

	int iStrengthHigh = pDefItem->sItem.iStrength;
	int iSpiritHigh = pDefItem->sItem.iSpirit;
	int iTalentHigh = pDefItem->sItem.iTalent;
	int iAgilityHigh = pDefItem->sItem.iAgility;
	int iHealthHigh = pDefItem->sItem.iHealth;

	ECharacterClass iCharacterClass = ClassFlagToCharacterClass ( (EClassFlag)pItem->eSpecialization );
	const char * classStr = GetCharacterClassStringByClassEnum ( iCharacterClass );

	if ( pItem->eSpecialization > 0 )
	{
		int iClassLevelMin      = saItemRequeriments[iCharacterClass].sLevel[0];

		int iClassStrengthMin   = saItemRequeriments[iCharacterClass].sStrength[0];
		int iClassSpiritMin     = saItemRequeriments[iCharacterClass].sSpirit[0];
		int iClassTalentMin     = saItemRequeriments[iCharacterClass].sTalent[0];
		int iClassAgilityMin    = saItemRequeriments[iCharacterClass].sAgility[0];
		int iClassHealthMin     = saItemRequeriments[iCharacterClass].sHealth[0];

		int iClassLevelMax      = saItemRequeriments[iCharacterClass].sLevel[1];
		int iClassStrengthMax   = saItemRequeriments[iCharacterClass].sStrength[1];
		int iClassSpiritMax     = saItemRequeriments[iCharacterClass].sSpirit[1];
		int iClassTalentMax     = saItemRequeriments[iCharacterClass].sTalent[1];
		int iClassAgilityMax    = saItemRequeriments[iCharacterClass].sAgility[1];
		int iClassHealthMax     = saItemRequeriments[iCharacterClass].sHealth[1];

		if ( pDefItem->sItem.iStrength > 0 )
		{
			if ( iClassStrengthMin )
				iStrengthLow += ( pDefItem->sItem.iStrength * iClassStrengthMin ) / 100;

			if ( iClassStrengthMax )
				iStrengthHigh += ( pDefItem->sItem.iStrength * iClassStrengthMax ) / 100;
		}

		if ( pDefItem->sItem.iSpirit > 0 )
		{
			if ( iClassSpiritMin )
				iSpiritLow += ( pDefItem->sItem.iSpirit * iClassSpiritMin ) / 100;

			if ( iClassSpiritMax )
				iSpiritHigh += ( pDefItem->sItem.iSpirit * iClassSpiritMax ) / 100;
		}

		if ( pDefItem->sItem.iTalent > 0 )
		{
			if ( iClassTalentMin )
				iTalentLow += ( pDefItem->sItem.iTalent * iClassTalentMin ) / 100;

			if ( iClassTalentMax )
				iTalentHigh += ( pDefItem->sItem.iTalent * iClassTalentMax ) / 100;
		}

		if ( pDefItem->sItem.iAgility > 0 )
		{
			if ( iClassAgilityMin )
				iAgilityLow += ( pDefItem->sItem.iAgility * iClassAgilityMin ) / 100;

			if ( iClassAgilityMax )
				iAgilityHigh += ( pDefItem->sItem.iAgility * iClassAgilityMax ) / 100;
		}

		if ( pDefItem->sItem.iHealth > 0 )
		{
			if ( iClassHealthMin )
				iHealthLow += ( pDefItem->sItem.iHealth * iClassHealthMin ) / 100;

			if ( iClassHealthMax )
				iHealthHigh += ( pDefItem->sItem.iHealth * iClassHealthMax ) / 100;
		}
	}

	if ( ( pItem->iStrength || pDefItem->sItem.iStrength ) && ( pItem->iStrength < iStrengthLow || pItem->iStrength > iStrengthHigh ) )
	{
		int iNewValue = Dice::RandomI ( iStrengthLow, iStrengthHigh );
		if ( iNewValue >= 0 )
		{
			//ignore Pole
			if ( pItem->sItemID.ToInt() != 17105152 )
			{
				if ( iCharacterClass > 0 )
					DEBUG( "%sItem strength fixed for item %s: %d --> %d for class %s", ServerCore::GetCharOrAccountName(), pItem->szItemName, pItem->iStrength, iNewValue, classStr );
				else
					DEBUG( "%sItem strength fixed for item %s: %d --> %d", ServerCore::GetCharOrAccountName(), pItem->szItemName, pItem->iStrength, iNewValue );
			}

			pItem->iStrength = iNewValue;
			bAdjustedAnything = TRUE;
		}
		else
		{
			WARN ( "New strength value is less than 0" );
		}
	}

	if ( ( pItem->iSpirit || pDefItem->sItem.iSpirit ) && ( pItem->iSpirit < iSpiritLow || pItem->iSpirit > iSpiritHigh ) )
	{
		int iNewValue = Dice::RandomI ( iSpiritLow, iSpiritHigh );
		if ( iNewValue >= 0 )
		{
			if ( iCharacterClass > 0 )
				DEBUG( "%sItem spirit fixed for item %s: %d --> %d for class %s", ServerCore::GetCharOrAccountName(), pItem->szItemName, pItem->iSpirit, iNewValue, classStr );
			else
				DEBUG( "%sItem spirit fixed for item %s: %d --> %d", ServerCore::GetCharOrAccountName(), pItem->szItemName, pItem->iSpirit, iNewValue );

			pItem->iSpirit = iNewValue;
			bAdjustedAnything = TRUE;
		}
		else
		{
			WARN ( "New spirit Value is less than 0" );
		}
	}

	if ( ( pItem->iTalent || pDefItem->sItem.iTalent ) && ( pItem->iTalent < iTalentLow || pItem->iTalent > iTalentHigh ) )
	{
		int iNewValue = Dice::RandomI ( iTalentLow, iTalentHigh );
		if ( iNewValue >= 0 )
		{
			if ( iCharacterClass > 0 )
				DEBUG( "%sItem talent fixed for item %s: %d --> %d for class %s", ServerCore::GetCharOrAccountName(), pItem->szItemName, pItem->iTalent, iNewValue, classStr );
			else
				DEBUG( "%sItem talent fixed for item %s: %d --> %d", ServerCore::GetCharOrAccountName(), pItem->szItemName, pItem->iTalent, iNewValue );


			pItem->iTalent = iNewValue;
			bAdjustedAnything = TRUE;
		}
		else
		{
			WARN ( "New talent value is less than 0" );
		}
	}

	if ( ( pItem->iAgility || pDefItem->sItem.iAgility ) && ( pItem->iAgility < iAgilityLow || pItem->iAgility > iAgilityHigh ) )
	{
		int iNewValue = Dice::RandomI ( iAgilityLow, iAgilityHigh );
		if ( iNewValue >= 0 )
		{
			if ( iCharacterClass > 0 )
				DEBUG ( "%sItem agility fixed for item %s: %d --> %d for class %s", ServerCore::GetCharOrAccountName(), pItem->szItemName, pItem->iAgility, iNewValue, classStr );
			else
				DEBUG( "%sItem agility fixed for item %s: %d --> %d", ServerCore::GetCharOrAccountName(), pItem->szItemName, pItem->iAgility, iNewValue );

			pItem->iAgility = iNewValue;
			bAdjustedAnything = TRUE;
		}
		else
		{
			WARN ( "New agility Value is less than 0" );
		}
	}

	if ( ( pItem->iHealth || pDefItem->sItem.iHealth ) && ( pItem->iHealth < iHealthLow || pItem->iHealth > iHealthHigh ) )
	{
		int iNewValue = Dice::RandomI ( iHealthLow, iHealthHigh );
		if ( iNewValue >= 0 )
		{
			if ( iCharacterClass > 0 )
				DEBUG( "%sItem health fixed for item %s: %d --> %d for class %s", ServerCore::GetCharOrAccountName(), pItem->szItemName, pItem->iHealth, iNewValue, classStr );
			else
				DEBUG( "%sItem health fixed for item %s: %d --> %d", ServerCore::GetCharOrAccountName(), pItem->szItemName, pItem->iHealth, iNewValue );

			pItem->iHealth = iNewValue;
			bAdjustedAnything = TRUE;
		}
		else
		{
			WARN ( "New health Value is less than 0" );
		}
	}

	return bAdjustedAnything;
}

/// <summary>
/// Get aging type
/// </summary>
/// <param name="iAgingLevel">The age level</param>
/// <param name="iAgeStoneType">0 = none, 1 = aging stone, 2 = copper ore, 3 = max age</param>
/// <returns></returns>
AgingResultType ItemServer::GetAgingResultType ( int iAgingLevel,	int iAgeStoneType,
												 int iFailChance,	int iPlus2Chance,
												 int iMinus1Chance, int iMinus2Chance,
												 int iBreakChance,	BOOL bNoBreakEvent)
{
	if ( GAME_SERVER )
	{
		return AgingResultType::Unknown;
	}

	int iAgeTotal = AGING_MAX;

	//iRandChance = 0 - 99 (inclusive)
	int iRandChance = Dice::RandomI ( 0, 99 );

	//Aging failed? (5% - 65% depending on age)
	if ( iRandChance < iFailChance )
	{
		//0% - 99% (inclusive)
		int iChanceMode = Dice::RandomI ( 0, 99 );

		bool bPassed = false;

		// ( -1 age ) - 50% chance (0-49)
		if ( ( iChanceMode < iMinus1Chance ) )
		{
			return AgingResultType::MinusOne;
		}

		//Take off 50%. Now it's 0% - 49% remaining
		iChanceMode -= iMinus1Chance;

		// ( -2 age ) = 34% chance (0-33)
		if ( iChanceMode < iMinus2Chance )
		{
			return AgingResultType::MinusTwo;
		}

		//Take off 33%. Now it's 0% - 16% remaining
		iChanceMode -= iMinus2Chance;

		//Have copper ore?
		if ( iAgeStoneType == 2 )
		{
			return AgingResultType::MinusOneCopperOre;
		}

		//Bugger. It's breaking time lads
		else
		{
			//do we have a non-break event?
			if ( bNoBreakEvent )
			{
				return AgingResultType::MinusOneNoBreakEvent;
			}
			else
			{
				return AgingResultType::Destruction;
			}
		}

	}

	//Aging was successful!
	else
	{
		//Re-throw for +2
		//iChanceMode = 0 - 99 (inclusive)
		int iChanceMode = Dice::RandomI ( 0, 99 );

		//Do we have double age?
		//+2 is usually < 15% chance
		if ( iChanceMode < iPlus2Chance )
		{
			//can we double age? (check vs max age)
			if ( ( iAgingLevel + 1 ) <= iAgeTotal )
			{
				return AgingResultType::PlusTwo; //+2
			}

			//do +1 instead.
			else
			{
				return AgingResultType::PlusOne; //+1
			}
		}
		else
		{
			return AgingResultType::PlusOne; //+1
		}
	}
}

void ItemServer::RecoverItemFromItemCreateLog ( User * pcUser, int iDatabaseId )
{
	//std::cout << "RecoverItemFromItemCreateLog" << std::endl;

	PacketRecoverItem * packet = new PacketRecoverItem ();
	ZeroMemory ( packet, sizeof ( PacketRecoverItem ) );
	packet->iHeader = PKTHDR_RecoverItem;
	packet->iLength = sizeof ( PacketRecoverItem );
	packet->iStatus = 0; //fail


	SQLConnection * pcDB = SQLCONNECTION ( DATABASEID_LogDB, 15 );
	if ( pcDB->Open () )
	{
		if ( pcDB->Prepare( "SELECT Item FROM ItemCreateLog WHERE ID=?" ) )
		{
			pcDB->BindParameterInput ( 1, PARAMTYPE_Integer, &iDatabaseId );

			if ( pcDB->Execute() && pcDB->Fetch () )
			{
				Item item;

				pcDB->GetBinaryData( 1, &item, sizeof( Item ) );

				CopyMemory ( &packet->sItem, &item, sizeof ( Item ) );
				packet->iStatus = 1;
			}
			else
			{
				std::cout << "RecoverItemFromItemCreateLog - FAIL" << std::endl;
			}

		}
		pcDB->Close();
	}

	if (packet->iStatus == 1 )
	{
		ITEMSERVER->AddItemInventory(pcUser->pcUserData, &packet->sItem);
		CHATSERVER->SendChat ( pcUser, EChatColor::CHATCOLOR_Normal, "GM> Item recovered!" );
	}
	else
	{
		CHATSERVER->SendChat ( pcUser, EChatColor::CHATCOLOR_Normal, "GM> Item recovery failed!" );
	}

	SENDPACKET ( pcUser, packet);
}

/// <summary>
/// Test aging simulation x 1000 for each age
/// </summary>
void ItemServer::TestAging ( int iAgingStoneType )
{
	if ( GAME_SERVER )
		return;

	if ( CreateDirectory ( "TestAging", NULL ) ||
		ERROR_ALREADY_EXISTS == GetLastError () )
	{
		//Check and create dir
	}
	else
	{
		return;
	}

	const int iRepeatCount = 10000;

	std::string logFileName = "";

	if ( iAgingStoneType == 1 ) //aging stone
	{
		logFileName = FormatString ( "TestAging\\AgingStoneResult.txt" );
	}
	else if ( iAgingStoneType == 2 ) //copper ore
	{
		logFileName = FormatString ( "TestAging\\CopperOreResult.txt");
	}
	else
	{
		logFileName = FormatString ( "TestAging\\NormalResult.txt");
	}


	CLogger * logger = new CLogger ( logFileName );

	int iFailChance = 0;
	int iFailNormalChance = 0;
	int iPlus2Chance = 0;
	int iMinus2Chance = 0;
	int iMinus1Chance = 0;
	int iBreakChance = 0;
	int iFailAgeStoneChance = 0;
	BOOL bAgingNoBreakEvent = FALSE;

	logger->WriteLine ( "" );
	logger->WriteLine ( "" );
	logger->WriteLine ( Util::CurrentDateTime () );
	logger->WriteLine ( "==========================================================================================" );
	logger->WriteLine ( "==========================================================================================" );
	logger->WriteLine ( "%d x Aging result:", iRepeatCount );
	logger->WriteLine ( "" );

	std::map<AgingResultType, int> vTestResult;

	for ( int i_AgingLevel = 1; i_AgingLevel <= 20; i_AgingLevel++ )
	{
		vTestResult.clear ();

		SQLConnection * pcDB = SQLCONNECTION ( DATABASEID_GameDB_Aging, 1 );
		if ( pcDB->Open () )
		{
			if ( pcDB->Prepare ( "SELECT * FROM AgeList WHERE AgeNumber=?" ) )
			{
				pcDB->BindParameterInput ( 1, PARAMTYPE_Integer, &i_AgingLevel );
				if ( pcDB->Execute () && pcDB->Fetch () )
				{
					pcDB->GetData ( 3, PARAMTYPE_Integer, &iFailNormalChance );		//5% - 65% depend in age
					pcDB->GetData ( 4, PARAMTYPE_Integer, &iPlus2Chance );
					pcDB->GetData ( 5, PARAMTYPE_Integer, &iMinus2Chance);			//typically 33%
					pcDB->GetData ( 6, PARAMTYPE_Integer, &iMinus1Chance );			//typically 50%
					pcDB->GetData ( 7, PARAMTYPE_Integer, &iBreakChance );			//typically 16%
					pcDB->GetData ( 8, PARAMTYPE_Integer, &iFailAgeStoneChance );	//at the moment this is 0% !

				}
			}
			pcDB->Close ();
		}

		//Aging stone is 'Aging stone?'
		//Use the fail chances for aging stone from DB
		if ( iAgingStoneType == 1 )
		{
			iFailChance = iFailAgeStoneChance;
		}
		else
		{
			iFailChance = iFailNormalChance;
		}

		for ( int i = 0; i < iRepeatCount; i++ )
		{
			AgingResultType eResult = GetAgingResultType ( i_AgingLevel, iAgingStoneType, iFailChance, iPlus2Chance, iMinus1Chance, iMinus2Chance, iBreakChance, bAgingNoBreakEvent);

			if ( eResult == AgingResultType::MinusOneCopperOre || eResult == AgingResultType::MinusOneNoBreakEvent )
				 eResult = AgingResultType::MinusOne;

			if ( vTestResult.find ( eResult ) == vTestResult.end () )
				vTestResult.insert ( { eResult, 0 } );

			vTestResult[eResult]++;
		}


		logger->WriteLine ( "Going.. +%d", i_AgingLevel );
		logger->WriteLine ( "-------------------------------" );


		std::string sResultStr = "";
		std::string sResultValueStr = "";

		for ( auto testResult : vTestResult )
		{
			AgingResultType eResult = testResult.first;

			switch ( eResult )
			{
				case AgingResultType::PlusOne:		sResultStr = "Plus one (+1)"; break;
				case AgingResultType::PlusTwo:		sResultStr = "Plus two (+2)"; break;
				case AgingResultType::MinusOne:		sResultStr = "Minus one (-1)"; break;
				case AgingResultType::MinusTwo:		sResultStr = "Minus two (-2)"; break;
				case AgingResultType::Destruction:	sResultStr = "Breakage"; break;
				default:						    sResultStr = "Unknown"; break;
			}

			sResultValueStr = FormatString("%d", testResult.second);

			padToLeft ( sResultStr, 14 );
			padTo ( sResultValueStr, 5);

			float fChance = static_cast<float>( testResult.second ) / static_cast<float>( iRepeatCount );
			logger->WriteLine ( "%s = %s (%.3f%%)", sResultStr.c_str (), sResultValueStr.c_str(), fChance * 100.f );
		}

		logger->WriteLine ( "-------------------------------" );
		logger->WriteLine ( "" );
	}

	logger->Flush ();
	logger->Close ();
}

//Log aging results into database
void ItemServer::SQLLogAgingResult (User * pcUser, AgingResultType eAgingResultType, int iAgeLevel)
{
	LOGSERVER->SQLLogEvent ( pcUser, LogServer::ELogEventType::LOGEVENTID_AgingResult, (int)eAgingResultType, iAgeLevel );
}

//Note - This function is only called via login server
int ItemServer::GetAgingType( UserData * pcUserData )
{
	//std::cout << "int ItemServer::GetAgingType( UserData * pcUserData )" << std::endl;

	if ( pcUserData )
	{
		User * pcUser = USERDATATOUSER ( pcUserData );

		UINT uStone = *(UINT *)0x786BAE4;

		*(int *)0x0789066C = 0;

		switch ( uStone )
		{
		case AgeStoneList::AgingStone:
		{
			*(int *)0x789066C = 2; //have it same as Cooper ore for now since Aging Stone is 100% aging (which is what we don't want)
		} break;

		case AgeStoneList::CooperOre:
		{
			*(int *)0x789066C = 2;
		} break;

		case AgeStoneList::MaxAge:
		{
			*(int *)0x789066C = 3;
		} break;

		default:
			break;
		}

		Item * pItem = (Item *)0x0786B838;

		int iAgingLevel = (int)pItem->sAgeLevel + 1;

		int iAgeTotal = AGING_MAX;

		( *(WORD *)0x8B80DA4 ) = 0;
		if ( *(int *)0x789066C == 3 ) //max age
		{
			( *(WORD *)0x8B80DA4 ) = 20;
			return 5;
		}

		int iFailNormalChance = 0;
		int iPlus2Chance = 0;
		int iMinus2Chance = 0;
		int iMinus1Chance = 0;
		int iBreakChance = 0;
		int iFailAgeStoneChance = 0;

		SQLConnection * pcDB = SQLCONNECTION ( DATABASEID_GameDB_Aging, 2 );
		if ( pcDB->Open () )
		{
			if ( pcDB->Prepare ( "SELECT * FROM AgeList WHERE AgeNumber=?" ) )
			{
				pcDB->BindParameterInput ( 1, PARAMTYPE_Integer, &iAgingLevel );
				if ( pcDB->Execute () && pcDB->Fetch () )
				{
					pcDB->GetData ( 3, PARAMTYPE_Integer, &iFailNormalChance ); //5 - 65
					pcDB->GetData ( 4, PARAMTYPE_Integer, &iPlus2Chance );
					pcDB->GetData ( 5, PARAMTYPE_Integer, &iMinus2Chance ); //typically 33%
					pcDB->GetData ( 6, PARAMTYPE_Integer, &iMinus1Chance ); //typically 50%
					pcDB->GetData ( 7, PARAMTYPE_Integer, &iBreakChance ); //typically 16%
					pcDB->GetData ( 8, PARAMTYPE_Integer, &iFailAgeStoneChance );

				}
			}
			pcDB->Close ();
		}

		int iFailChance = iFailNormalChance;
		if ( *(int *)0x789066C == 1 ) // if have AgeStone
		{
			iFailChance = iFailAgeStoneChance; // change to new chance
		}

		AgingResultType eAgingResultType;

		// Always aging success: never fail, never break, always +2
		if (ALWAYS_AGING_SUCCESS)
		{
			// 85% chance +1, 15% chance +2
			eAgingResultType = AgingResultType::PlusTwo;
		}
		else
		{
			eAgingResultType = GetAgingResultType ( iAgingLevel, *(int *)0x789066C,
				iFailChance, iPlus2Chance, iMinus1Chance, iMinus2Chance, iBreakChance, EVENT_AGING_NOBREAK );
		}

		SQLLogAgingResult ( pcUser, eAgingResultType, iAgingLevel );

		//+1
		if ( eAgingResultType == AgingResultType::PlusOne )
		{
			ITEMSERVER->UpdateIntegrity ( pItem, -1 );
			ITEMSERVER->SaveItemDataToDatabase ( pItem );

			USERSERVER->SendUserMiscCommandToOtherNearbyPlayers ( pcUser, EUnitDataMiscCommand::COMMANDID_ShowSuccessAgingOrMixSkillAnimation );


			return (int)AgingResultType::PlusOne;
		}
		//+2
		else if ( eAgingResultType == AgingResultType::PlusTwo )
		{
			if ( iAgingLevel < 10 )
			{
				//CHATSERVER->SendChatAllEx ( CHATCOLOR_Global, "Arcane Moriph> %s double aged '%s' to +%d successfully! Congratz!", CHARACTERSERVER->GetCharacterName ( pcUserData ), (char *)0x0786B854, ( iAgingLevel + 1 ) );
			}
			else
			{
				CHATSERVER->SendChatAllEx ( CHATCOLOR_Global, "Arcane Moriph> %s double aged '%s' to +%d successfully! Amazing!!", CHARACTERSERVER->GetCharacterName ( pcUserData ), (char *)0x0786B854, ( iAgingLevel + 1 ) );
			}

			USERSERVER->SendUserMiscCommandToOtherNearbyPlayers ( pcUser, EUnitDataMiscCommand::COMMANDID_ShowSuccessAgingOrMixSkillAnimation );

			ITEMSERVER->UpdateIntegrity ( pItem, -1 );
			ITEMSERVER->SaveItemDataToDatabase ( pItem );

			return (int)AgingResultType::PlusTwo;
		}
		//-1
		else if ( eAgingResultType == AgingResultType::MinusOne )
		{
			if ( iAgingLevel <= 9 )
				ITEMSERVER->UpdateIntegrity ( pItem, -3 );
			else
				ITEMSERVER->UpdateIntegrity ( pItem, -5 );

			return (int)AgingResultType::MinusOne;
		}
		//-1 (due to copper ore)
		else if ( eAgingResultType == AgingResultType::MinusOneCopperOre )
		{
			CHATSERVER->SendChat ( USERDATATOUSER ( pcUserData ), EChatColor::CHATCOLOR_Normal,
				"Arcane Moriph> Item breakage avoided due to Copper Ore. Phew..!" );

			if ( iAgingLevel <= 9 )
				ITEMSERVER->UpdateIntegrity ( pItem, -3 );
			else
				ITEMSERVER->UpdateIntegrity ( pItem, -5 );

			return (int)AgingResultType::MinusOne;
		}
		//-1 (due to aging event)
		else if ( eAgingResultType == AgingResultType::MinusOneNoBreakEvent )
		{
			CHATSERVER->SendChat ( USERDATATOUSER ( pcUserData ), EChatColor::CHATCOLOR_Normal,
				"Arcane Moriph> Item breakage avoided due to Event. Phew..!" );

			if ( iAgingLevel <= 9 )
				ITEMSERVER->UpdateIntegrity ( pItem, -3 );
			else
				ITEMSERVER->UpdateIntegrity ( pItem, -5 );

			return (int)AgingResultType::MinusOne;
		}
		//-2
		else if ( eAgingResultType == AgingResultType::MinusTwo )
		{
			if ( iAgingLevel <= 9 )
				ITEMSERVER->UpdateIntegrity ( pItem, -3 );
			else
				ITEMSERVER->UpdateIntegrity ( pItem, -5 );

			return (int)AgingResultType::MinusTwo;
		}
		else if ( eAgingResultType == AgingResultType::Destruction )
		{
			CHATSERVER->SendChatAllEx ( CHATCOLOR_Global, "Arcane Moriph> %s broke '%s' going to +%d! Better luck next time!", CHARACTERSERVER->GetCharacterName ( pcUserData ), (char *)0x0786B854, iAgingLevel );
			AGEHANDLER->OnBrokeItemHandler ( USERDATATOUSER ( pcUserData ), pItem );

			return (int)AgingResultType::Destruction;
		}
		else
		{
			LOGERROR ( "ItemServer::GetAgingType: Unknown aging result type: %d", eAgingResultType );
			return (int)eAgingResultType;
		}
	}
	else
	{
		LOGERROR ( "ItemServer::GetAgingType: pcUserData is NULL" );
		return (int)AgingResultType::MinusOne;
	}
}



BOOL ItemServer::OnUpgradeAgingItem( UserData * pcUserData, PacketItem * psPacket )
{
	//std::cout << "OnUpgradeAgingItem( UserData * pcUserData, PacketItem * psPacket )" << std::endl;

	Item sItem;

	Item * psItem = &psPacket->sItem;

	BOOL bMatureHack = FALSE;

	if ( pcUserData )
	{
		if ( !CheckItemForm( psItem ) )
			return FALSE;

		if ( !DeleteItemInventory( pcUserData, psItem->sItemID.ToItemID(), psItem->iChk1, psItem->iChk2 ) )
			return FALSE;

		if ( psItem->iItemUniqueID == UNIQUEID_QUEST_WEAPON || psItem->eCraftType == EItemCraftType::ITEMCRAFTTYPE_QuestWeapon )
		{
			WARN( "[OnUpgradeAgingItem] Unexpected state" );
			return FALSE;
		}

		CopyMemory( &sItem, psItem, sizeof( Item ) );

		// Check Mature Hack
		if ( !CheckQuestOrEventItemAging( psItem->szItemName, psItem->sItemID.ToInt() ) )
		{
			// Not correct? Hack
			if ( psItem->bMaturing != TRUE )
				bMatureHack = TRUE;

			// Not mature? Hack
			if ( psItem->sMatureBar.sCur == 0 || (psItem->sMatureBar.sCur != psItem->sMatureBar.sMax) )
				bMatureHack = TRUE;

			//Have some allowance
			if ( bMatureHack )
			{
				if ( abs(psItem->sMatureBar.sMax - psItem->sMatureBar.sCur) <= 5 )
					bMatureHack = FALSE;
			}

			// More aging or mature hack flag?
			if ( (pcUserData->iGameLevel < GAMELEVEL_Four) && (psItem->sAgeLevel >= AGING_MAX || psItem->sAgeLevel < 0 || bMatureHack) )
			{
				// Set Log
				PacketLogCheat sPacket;
				sPacket.iCheatID	= CHEATLOGID_MatureHack;
				sPacket.SParam		= psItem->bMaturing;
				sPacket.EParam		= psItem->sMatureBar.sCur;
				sPacket.LxParam		= psItem->sMatureBar.sMax;
				sPacket.WxParam		= psItem->sAgeLevel;
				sPacket.LParam		= psItem->sItemID.ToItemID();
				sPacket.SxParam		= psItem->iChk1;
				sPacket.ExParam		= psItem->iChk2;
				SaveItemDataToDatabase( psItem );
				LOGSERVER->OnLogCheat( pcUserData, &sPacket );
				return FALSE;
			}
		}
		psItem->tTime = 0;
		tfnIncreaseAgingState fnIncreaseAgingState = (tfnIncreaseAgingState)pfnIncreaseAgingState;
		fnIncreaseAgingState( psItem );

		AddItemInventory( pcUserData, psItem->sItemID.ToItemID(), psItem->iChk1, psItem->iChk2 );

		tfnRecordAgingLog fnRecordAgingLog = (tfnRecordAgingLog)pfnRecordAgingLog;
		fnRecordAgingLog( pcUserData, &sItem, &psPacket->sItem );

		tfnRecordItemChangeLog fnRecordItemChangeLog = (tfnRecordItemChangeLog)pfnRecordItemChangeLog;
		fnRecordItemChangeLog( pcUserData, sItem.sItemID.ToItemID(), sItem.iChk1, sItem.iChk2, psItem->sItemID.ToItemID(), psItem->iChk1, psItem->iChk2, 0x0C );

		SaveItemDataToDatabase( psItem );

		SENDPACKET( USERDATATOUSER(pcUserData), psPacket );
		return TRUE;
	}
	return FALSE;
}

int ItemServer::GetForceDamageByIndex (int iForceIndex)
{
	if ( iForceIndex >= 0 && iForceIndex <= 13 )
	{
		return ForceDamageTable[iForceIndex];
	}

	WARN ( "Invalid ForceIndex for GetForceDamageByIndex: %d", iForceIndex );
	return 0;
}

int ItemServer::GetForceDurationByIndex ( int iForceIndex )
{
	if ( iForceIndex >= 0 && iForceIndex <= 13 )
	{
		return ForceDurationTable[iForceIndex];
	}

	WARN ( "Invalid ForceIndex for GetForceDurationByIndex: %d", iForceIndex );
	return 0;

}

int ItemServer::GetForceDamageExtraPercentageByIndex ( int iForceIndex )
{
	if ( iForceIndex >= 0 && iForceIndex <= 13 )
	{
		return ForceDamagePercentTable[iForceIndex];
	}

	WARN ( "Invalid ForceIndex for GetForceDamageExtraPercentageByIndex: %d", iForceIndex );
	return 0;
}

int ItemServer::DecodeCompress ( BYTE * lpSrcData, BYTE * lpDestData )
{
	return DecodeCompress ( lpSrcData, lpDestData, 0x7FFFF );
}

int ItemServer::EncodeCompress ( BYTE * lpSrcData, BYTE * lpDestData, int size )
{
	return EncodeCompress ( lpSrcData, lpDestData, size, 0x7FFFF );
}

int ItemServer::EncodeCompress ( BYTE * lpSrcData, BYTE * lpDestData, int size, int DestSize )
{
	int EncCounter;
	int cnt;
	int Counter;
	int pSet;
	BYTE c1, c2;
	int lsize;

	Counter = 0;
	EncCounter = 4;
	lsize = size - 1;

	while ( TRUE )
	{
		if ( Counter >= size ) break;
		if ( EncCounter >= DestSize ) break;

		c1 = lpSrcData[Counter];
		if ( Counter < lsize ) c2 = lpSrcData[Counter + 1];
		else c2 = c1;

		if ( c1 == 0 && c2 == 0 )
		{
			for ( cnt = 0; cnt < 0x7F; cnt++ )
			{
				if ( Counter >= size ) break;
				if ( lpSrcData[Counter] != 0 ) break;
				Counter++;
			}
			if ( EncCounter >= DestSize ) break;
			lpDestData[EncCounter++] = cnt | 0x80;
		}
		else
		{
			pSet = EncCounter;
			EncCounter++;

			for ( cnt = 0; cnt < 0x7F; cnt++ )
			{
				c1 = lpSrcData[Counter];
				if ( Counter < lsize ) c2 = lpSrcData[Counter + 1];
				else c2 = c1;

				if ( Counter >= size ) break;
				if ( EncCounter >= DestSize ) break;
				if ( c1 == 0 && c2 == 0 ) break;
				lpDestData[EncCounter++] = lpSrcData[Counter++];
			}
			lpDestData[pSet] = cnt;
		}
	}

	( (int *)lpDestData )[0] = EncCounter;

	return EncCounter;
}

int ItemServer::DecodeCompress ( BYTE * lpSrcData, BYTE * lpDestData, int dCount )
{
	int DecCounter;
	int cnt;
	int Counter;
	int size;
	int len;

	size = ( (int *)lpSrcData )[0];

	Counter = 4;
	DecCounter = 0;

	while ( TRUE )
	{
		if ( Counter >= size ) break;
		if ( lpSrcData[Counter] & 0x80 )
		{
			len = lpSrcData[Counter++] & 0x7F;
			for ( cnt = 0; cnt < len; cnt++ )
			{
				if ( DecCounter >= dCount ) return Counter;
				lpDestData[DecCounter++] = 0;
			}

		}
		else
		{
			len = lpSrcData[Counter++] & 0x7F;
			for ( cnt = 0; cnt < len; cnt++ )
			{
				if ( DecCounter >= dCount ) return Counter;
				lpDestData[DecCounter++] = lpSrcData[Counter++];
			}
		}
	}

	return Counter;
}


//int rsRecvAgingItem(rsPLAYINFO *lpPlayInfo, sCRAFTITEM_SERVER *pCraftItem_Server)
BOOL ItemServer::OnAgingItem( User * pcUser, PacketAgingItem * psPacket )
{
	//This function call is only done via login server
	if ( GAME_SERVER )
		return FALSE;

	if ( pcUser == NULL || pcUser->IsValidAndInGame() == FALSE )
		return FALSE;

#if defined (DEV_MODE) && defined (DEBUG_INVENTORY)
	INFO( "OnAgingItem: %s (%d, %d)", psPacket->sItem.szBaseName, psPacket->sItem.sItem.iChk1, psPacket->sItem.sItem.iChk2 );
#endif

	//std::cout << "OnAgingItem( UserData * pcUserData, PacketAgingItem * psPacket )" << std::endl;

	if ( psPacket->iLength != sizeof( PacketAgingItem ) )
		return FALSE;

	ItemData * psItem	= &psPacket->sItem;

	DWORD * pdwSheltom	= psPacket->SheltomCode;

	/*if (!CheckValidItem(&psItem->sItem))
		return FALSE;*/

	BOOL bAgeHack		= FALSE;
	int iSheltomCount	= 0;

	short sAgeLevel		= psItem->sItem.sAgeLevel;

	BOOL bSheltom[12]	= { 0,0,0,0,0,0,0,0,0,0,0,0 };

	// Check Sheltom Hack
	{
		// Hack Level?
		if ( sAgeLevel >= AGING_MAX || sAgeLevel < 0 )
			bAgeHack = TRUE;

		for ( int i = 0; i < 12; i++ )
		{
			for ( int j = 0; j < 12; j++ )
			{
				if ( !bSheltom[j] && pdwSheltom[j] == (iaSheltomAgingList[sAgeLevel][i] ? ((iaSheltomAgingList[sAgeLevel][i]*256) | 0x02350000) : 0) )
				{
					if ( pdwSheltom[j] )
						iSheltomCount++;

					bSheltom[j] = TRUE;
					break;
				}
			}
		}

		for ( int i = 0; i < 12; i++ )
		{
			if ( bSheltom[i] != TRUE )
			{
				bAgeHack = TRUE;
				break;
			}
		}

		if ( CheckQuestOrEventItemAging( psItem->sItem.szItemName, psItem->sItem.sItemID.ToInt() ) )
			bAgeHack = FALSE;

		if ( bAgeHack && pcUser->GetGameLevel() < GAMELEVEL_Four )
		{
			PacketLogCheat sPacket;
			sPacket.iCheatID	= CHEATLOGID_AgingHack;
			sPacket.LParam		= sAgeLevel;
			sPacket.SParam		= iSheltomCount;
			sPacket.EParam		= psItem->sItem.sItemID.ToItemID();
			sPacket.LxParam		= psItem->sItem.iChk1;
			sPacket.WxParam		= psItem->sItem.iChk2;
			SaveItemDataToDatabase( &psItem->sItem );
			LOGSERVER->OnLogCheat( pcUser->pcUserData, &sPacket );
			return FALSE;
		}
	}



	if (psItem->sItem.eCraftType != ITEMCRAFTTYPE_QuestWeapon)
	{
		int l_ItemAgePrice = 0;

		//std::cout << "EVENT_AGINGFREE = " << EVENT_AGINGFREE << std::endl;
		//std::cout << "EVENT_AGING_HALFPRICE = " << EVENT_AGING_HALFPRICE << std::endl;
		//std::cout << "EVENT_AGING_NOBREAK = " << EVENT_AGING_NOBREAK << std::endl;

		//is free aging?
		if (!EVENT_AGINGFREE)
		{
			int l_ItemBuyPrice = ItemServer::GetItemBuyPrice(psPacket->sItem.sBaseItemID.ToItemID(), psPacket->sItem.sItem.eSpecialization != 0, (int)psPacket->sItem.sItem.eRarity);
			l_ItemAgePrice = ((sAgeLevel + 1) * l_ItemBuyPrice) / 2;

			//std::cout << "Packet price " << psItem->sItem.iSalePrice << std::endl;
			//std::cout << "server price " << l_ItemBuyPrice << std::endl;
			//std::cout << "age price " << l_ItemAgePrice << std::endl;

			/// Overwrite, if not the same value, the checksum will fail
			if ( psItem->sItem.iSalePrice != l_ItemBuyPrice )
			{
				WARN( "Item (%s) sale price (%d) is different than the buy price (%d)", psItem->sItem.szItemName, psItem->sItem.iSalePrice, l_ItemBuyPrice );

				//also need to remove the old item, and then add the new one
				//it is needed otherwise cheat id 8000 is the result
				if ( ITEMSERVER->DeleteItemInventory( pcUser->pcUserData, &psItem->sItem ) )
				{
					psItem->sItem.iSalePrice = l_ItemBuyPrice;

					//also need to reform the item if the sale price is changed
					// or is wrong (i.e. npc item lists item at wrong price)
					ITEMSERVER->ReformItem ( &psItem->sItem );

					//Now add the new item to the server's item inventory for this player
					ITEMSERVER->AddItemInventory( pcUser->pcUserData, &psItem->sItem );
				}
				else
				{
					WARN( "OnAgingItem - Item does not exist in inventory: %s (%d, %d)", psItem->sItem.szItemName, psItem->sItem.iChk1, psItem->sItem.iChk2 );
				}
			}


			//std::cout << "AGING COST: " << l_ItemAgePrice << std::endl;

			if (EVENT_AGING_HALFPRICE)
			{
				l_ItemAgePrice /= 2;
			}

			//std::cout << "AGING HALF PRICE: " << l_ItemAgePrice << std::endl;

			//l_ItemAgePrice = ItemServer::GetItemAgingPrice(&psPacket->sItem.sItem, psPacket->sItem.sItem.sAgeLevel);
			//l_ItemAgePrice = static_cast<int>(static_cast<float>(l_ItemAgePrice) / 2.0f);

			//_ItemAgePrice = ItemServer::GetItemAgingPrice(&psPacket->sItem.sItem, psPacket->sItem.sItem.sAgeLevel);

			/// Overwrite, if not the same value, the checksum will fail
			//psItem->sItem.iSalePrice = l_ItemAgePrice;
		}



		//std::cout << "USER GOLD IS: " << pcUserData->GetGold() << std::endl;


		//There is a aging price?
		if (l_ItemAgePrice > 0)
		{
			//User has gold?
			if (pcUser->pcUserData->GetGold() >= l_ItemAgePrice)
			{
				//all good.
				CHARACTERSERVER->GiveGOLD(pcUser, -l_ItemAgePrice, EVENT_AGING_HALFPRICE ? WHEREID_AgingHalfPrice : WHEREID_Aging);
			}
			else
			{
				PacketLogCheat sPacket;
				sPacket.iCheatID = CHEATLOGID_AgingFailedNotEnoughMoney;
				sPacket.LParam = pcUser->pcUserData->GetGold();
				sPacket.SParam = l_ItemAgePrice;
				LOGSERVER->OnLogCheat(pcUser->pcUserData, &sPacket);

				//Note - Returning false without disconnection causes the client to hang in the aging window
				//the disconnection happens via OnLogCheat
				return FALSE;
			}
		}
	}

	//int rsRecvAgingItem(rsPLAYINFO *lpPlayInfo, sCRAFTITEM_SERVER *pCraftItem_Server)
	//OnServer.cpp

	//std::cout << "tfnRecvAgingItem fnRecvAgingItem = (tfnRecvAgingItem)0x005690B0;" << std::endl;
	//std::cout << "item " << psPacket->sItem.sItem.szItemName << std::endl;

	//std::cout << CALL_WITH_ARG1(0x4461E0, (DWORD)&psPacket->sItem.sItem) << std::endl;

	//std::cout << CALL_WITH_ARG1(0x446170, (DWORD)&psPacket->sItem.sItem) << std::endl;

	//std::cout << "ITEMSERVER->CheckMixItem: " << ITEMSERVER->CheckMixItem(&psPacket->sItem.sItem) << std::endl;
	//std::cout << "ITEMSERVER->CheckItemForm: " << ITEMSERVER->CheckItemForm(&psPacket->sItem.sItem) << std::endl;

	//0x445AC0 is rsGetItemForm_Server
	//ccItem.sinRecvAgingItem ?
	DWORD dwCode = CALL_WITH_ARG1 ( 0x445AC0, (DWORD)&psPacket->sItem.sItem );

	//DWORD compare = psPacket->sItem.sItem.dwTime;


//	std::cout << "dwCode: " << dwCode << std::endl;
	//std::cout << "compare: " << compare << std::endl;


	typedef void( __cdecl *tfnRecvAgingItem )(UserData * pcUserData, PacketAgingItem * psPacket);

	if ( ITEMSERVER->HasItemInInventory( pcUser->pcUserData, &psPacket->sItem ) == FALSE)
	{
		WARN( "OnAgingItem - User does not have item (%s) in inventory (%d, %d)", psPacket->sItem.szInventoryName, psPacket->sItem.sItem.iChk1, psPacket->sItem.sItem.iChk2 );
	}


	//int rsRecvAgingItem(rsPLAYINFO *lpPlayInfo, sCRAFTITEM_SERVER *pCraftItem_Server)
	//todo - WRITE THIS UP
	tfnRecvAgingItem fnRecvAgingItem = (tfnRecvAgingItem)0x005690B0;
	fnRecvAgingItem( pcUser->pcUserData, psPacket );

	return TRUE;
}


void ItemServer::ProcessIntegrity( User * pcUser, EIntegrityUpdate eUpdate )
{
	PacketUpdateIntegrity sPacket;
	sPacket.iHeader = PKTHDR_IntegrityUpdateItem;
	sPacket.iLength = sizeof(PacketUpdateIntegrity);
	sPacket.eUpdate = eUpdate;
	SENDPACKET( pcUser, &sPacket );
}

void ItemServer::UpdateIntegrity( Item * psItem, int iValueCur, int iValueMax )
{
	if( psItem )
	{
		if( psItem->sIntegrity.sMax <= 0 )
			return;

		if( iValueCur )
		{
			psItem->sIntegrity.sCur += iValueCur;
			if( psItem->sIntegrity.sCur <= 0 )
			{
				psItem->sIntegrity.sCur = 0;
				psItem->bCanNotUse = TRUE;
			}
			else if( psItem->sIntegrity.sCur >= psItem->sIntegrity.sMax )
				psItem->sIntegrity.sCur = psItem->sIntegrity.sMax;
		}

		if( iValueMax )
		{
			psItem->sIntegrity.sMax += iValueMax;
			if( psItem->sIntegrity.sMax <= 0 )
				psItem->sIntegrity.sMax = 0;
		}
	}
}

DWORD ItemServer::OnPhoenixPet( UserData * pcUserData, DWORD dwCode, BOOL bDelete, int iElementType )
{
	if ( pcUserData )
	{
		PacketActivePremium sPacket;
		sPacket.iLength			= sizeof( PacketActivePremium );
		sPacket.iHeader			= PKTHDR_SetItemTimer;
		sPacket.sPremium.dwCode = dwCode;
		SENDPACKET( USERDATATOUSER(pcUserData), &sPacket, TRUE );
	}
	return dwCode;
}


//Get item certification
//See int ReformItem( sITEMINFO *lpItemInfo )
//Was: ValidateItem(..)
BOOL ItemServer::ReformItem( ItemData * psItem )
{
	typedef void( __cdecl *tfnValidateItem )(Item * psItem);
	tfnValidateItem ValidateItem = (tfnValidateItem)0x0044A8A0;
	ValidateItem( &psItem->sItem );
	return TRUE;
}

//Get item certification
//See int ReformItem( sITEMINFO *lpItemInfo )
//Was: ValidateItem(..)
BOOL ItemServer::ReformItem( Item * psItem )
{
	typedef void( __cdecl *tfnValidateItem )(Item * psItem);
	tfnValidateItem ValidateItem = (tfnValidateItem)0x0044A8A0;
	ValidateItem( psItem );
	return TRUE;
}

//reason why we have to do it like this is because ASM
// take these as params:  eax, edx, edi
DWORD ItemServer::GetReformItemCode(Item * psItem, int iKey)
{
	__asm
	{
		PUSH EDI;
		PUSH EDX;
		MOV EAX, iKey;
		MOV EDI, psItem;
		MOV EDX, 0x00445670;
		CALL EDX;
		POP EDX;
		POP EDI;
	}
}

//don't use
BOOL ItemServer::ReformItemNew( Item * lpItemInfo, DWORD bTimeOverride )
{
	DWORD dwChecksum;
	DWORD dwItemFormKey = ( *(DWORD *)0x01BADA58 );
	DWORD dwItemFormVersion = ( *(DWORD *)0x00619E70 );
	DWORD dwServerTime = ( *(DWORD *)0x07AC9D6C );
	DWORD dwCurrentTime = GetCurrentTime();

	//int ReformMixItem( sITEMINFO *lpItemInfo )
	typedef int( __cdecl *tfnReformMixItem )(Item * psItem );
	tfnReformMixItem ReformMixItem  = (tfnReformMixItem)0x004461C0;

	//int ReformMixItem( sITEMINFO *lpItemInfo )
	typedef int( __cdecl *tfnrsReformItem_Server )(Item * psItem );
	tfnrsReformItem_Server rsReformItem_Server  = (tfnrsReformItem_Server)0x00445AD0;

	//if (bUpdateTime || lpItemInfo->dwTime == 0)
	//	lpItemInfo->dwTime = dwServerTime;

	lpItemInfo->iChk1 = dwItemFormKey + dwCurrentTime;


	dwChecksum = GetReformItemCode( lpItemInfo, lpItemInfo->iChk1 ); //445670

	lpItemInfo->iChk2 = dwChecksum;
	lpItemInfo->dwVersion = dwItemFormVersion;

	if ( bTimeOverride != 0 )
		lpItemInfo->tTime = bTimeOverride;
	else
		lpItemInfo->tTime = dwServerTime;

	if ( !lpItemInfo->iBackupKey || !lpItemInfo->iBackupChk )
	{
		lpItemInfo->iBackupKey = lpItemInfo->iChk1;
		lpItemInfo->iBackupChk = lpItemInfo->iChk2;
	}

	//tfnSetAgingChecksum fnSetAgingChecksum = (tfnSetAgingChecksum)pfnSetAgingChecksum;
	//fnSetAgingChecksum( lpItemInfo );
	ReformMixItem( lpItemInfo );			//Get item aging certification (4461C0)
	rsReformItem_Server( lpItemInfo );		//Receive item server authentication only (445AD0)

	lpItemInfo->iUnknown02 = lpItemInfo->iChk2 + lpItemInfo->sItemID.ToInt();

	(*(DWORD *)0x01BADA58 )++;

	return TRUE;
}

//rsReformItem_Server
void ItemServer::ValidateItemTime( Item * psItem )
{
	typedef void( __cdecl *tfnValidateItemTime )(Item * ps);
	tfnValidateItemTime fnValidateItemTime = (tfnValidateItemTime)0x00445AD0;
	fnValidateItemTime( psItem );
}

void ItemServer::RegisterItemSecCode( UserData * pcUserData, PacketItem * psPacket )
{
	typedef void( __cdecl *tfnRegisItemSecCode )(UserData * pcUserData, PacketItem * psPacket, int v);
	tfnRegisItemSecCode RegisItemSecCode = (tfnRegisItemSecCode)0x00551AB0;
	RegisItemSecCode( pcUserData, psPacket, TRUE );
}

void ItemServer::SendItemDataToInventory ( UserData * pcUserData, Item * psItem )
{
	//Is user alive and have item?
	if ( pcUserData && psItem )
	{
		PacketItem sPacket;
		ZeroMemory ( &sPacket, sizeof ( PacketItem ) );
		sPacket.iLength = sizeof ( PacketItem );
		sPacket.iHeader = PKTHDR_NewItemNPC;
		CopyMemory ( &sPacket.sItem, psItem, sizeof ( Item ) );

		if ( psItem->sItemID.ToItemID () == ITEMID_Gold )
		{
			pcUserData->iGoldOut += psItem->iGold;
			sPacket.iX = TICKCOUNT;
			sPacket.iY = sPacket.sItem.iChk2;
			sPacket.iZ = pcUserData->iGoldOut ^ ( sPacket.iX + sPacket.iY );
		}

		RegisterItemSecCode ( pcUserData, &sPacket );
		SaveItemDataToDatabase ( &sPacket.sItem );
		SENDPACKET ( USERDATATOUSER ( pcUserData ), &sPacket );
	}
}

void ItemServer::SendPotions( User * pcUser, EItemID pItemID, int iCount, EItemSource eItemSource, BOOL bAutoPlace )
{
	DefinitionItem * psItemDef = ITEMSERVER->FindItemDefByCode( pItemID );

	if( !psItemDef )
		return;

	if ( psItemDef->sItem.sItemID.ToItemBase() != EItemBase::ITEMBASE_Potion )
		return;

	Item sItem;
	ITEMSERVER->CreateItem( &sItem, psItemDef, eItemSource );
	sItem.iPotionCount = iCount;
	ITEMSERVER->ReformItem( &sItem );
	SendItemData( pcUser->pcUserData, &sItem, bAutoPlace );

	ITEMSERVER->AddPotion( pcUser->pcUserData, psItemDef->sItem.sItemID.ToInt(), iCount );
}

void ItemServer::SendItemData( User * pcUser, int iItemID, EItemSource eItemSource )
{
	if ( pcUser == NULL || pcUser->IsValidAndInGame() == FALSE )
		return;

	auto psItemDef = ITEMSERVER->FindItemDefByCode ( iItemID );

	if ( !psItemDef )
		return;

	if ( GAME_SERVER )
	{
		//WARN( "SendItemData is called on GAME SERVER. Should be called on LOGIN_SERVER" );

		PacketNetSendItemData sPacket;
		sPacket.iHeader = PKTHDR_NetSendItemData;
		sPacket.iLength = sizeof( PacketNetSendItemData );
		sPacket.dwUserID = pcUser->pcUserData->iID;
		sPacket.iItemID = iItemID;
		sPacket.eItemSource = eItemSource;

		NETSERVER->SendPacketToLoginServer( &sPacket );
	}
	else
	{
		Item sItem;
		ITEMSERVER->CreateItem( &sItem, psItemDef, eItemSource );
		ITEMSERVER->SendItemData( pcUser, &sItem, TRUE );
		ITEMSERVER->AddItemInventory( pcUser->pcUserData, &sItem );

		if ( eItemSource == EItemSource::QuestKill )
		{
			CHATSERVER->SendChatEx( pcUser, EChatColor::CHATCOLOR_Normal, "> Quest item received '%s'", sItem.szItemName );
		}
	}
}

void ItemServer::SendItemData( User * pcUser, Item * psItem, BOOL bAuto )
{
	if ( pcUser == NULL || pcUser->IsValidAndInGame() == FALSE )
		return;

	SendItemData( pcUser->pcUserData, psItem, bAuto );
}

/// <summary>
/// This code gets called when user clicked on an item on floor
/// which in turns sends the PKTHDR_NewItem packet
/// to put the item in player's inventory
/// </summary>
void ItemServer::SendItemData( UserData * pcUserData, Item * psItem, BOOL bAuto )
{
	//Is user alive and have item?
	if( pcUserData && psItem )
	{
		DEBUG( "SendItemData: %s", psItem->szItemName );

		PacketItem sPacket;
		ZeroMemory( &sPacket, sizeof(PacketItem) );
		sPacket.iLength	= sizeof(PacketItem);
		sPacket.iHeader = bAuto ? PKTHDR_NewItemAuto : PKTHDR_NewItem;
		CopyMemory( &sPacket.sItem, psItem, sizeof( Item ) );

		if( psItem->sItemID.ToItemID() == ITEMID_Gold )
		{
			pcUserData->iGoldOut += psItem->iGold;
			sPacket.iX = TICKCOUNT;
			sPacket.iY = sPacket.sItem.iChk2;
			sPacket.iZ = pcUserData->iGoldOut ^(sPacket.iX + sPacket.iY);
		}

		if ( ( sPacket.sItem.eItemFlags & EItemFlag::TreasureHuntingMode ) == EItemFlag::TreasureHuntingMode )
		{
			if ( pcUserData->iGameLevel == GAMELEVEL_None )
			{
				sPacket.sItem.eItemFlags ^= EItemFlag::TreasureHuntingMode;
				DEBUG( "Item had treasure hunting flag enabled. Disabling" );
			}
		}

		if ( ( sPacket.sItem.eItemFlags & EItemFlag::ForceAllowDrop ) == EItemFlag::ForceAllowDrop )
		{
			if ( pcUserData->iGameLevel == GAMELEVEL_None )
			{
				sPacket.sItem.eItemFlags ^= EItemFlag::ForceAllowDrop;
				sPacket.sItem.bDisallowDrop = TRUE;
				DEBUG( "Item was force dropped. Disabling force drop. %d", sPacket.sItem.eItemFlags );
				ITEMSERVER->ReformItem( &sPacket.sItem );
			}
		}

		RegisterItemSecCode( pcUserData, &sPacket );
		SaveItemDataToDatabase( &sPacket.sItem );
		SENDPACKET( USERDATATOUSER(pcUserData), &sPacket );
	}
}

/**
Save item to database (ItemCreateLog)
*/
BOOL ItemServer::OnSaveItemData( Item * psItem )
{
	EItemBase iItemBase = psItem->sItemID.ToItemBase();

	if ( ((psItem->iChk1 == 0) && (psItem->iChk2 == 0)) || (psItem->sItemID.ToItemID() == ITEMID_None) )
		return FALSE;

	if ( (iItemBase != ITEMBASE_Weapon) && (iItemBase != ITEMBASE_Defense) && (iItemBase != ITEMBASE_Sheltom) )
		return FALSE;

	SQLConnection * pcDB = SQLCONNECTION( DATABASEID_LogDB, 16 );
	if ( pcDB->Open() )
	{
		if ( pcDB->Prepare( "INSERT INTO ItemCreateLog([ItemID],[Code1],[Code2],[Item],[Date]) VALUES(?,?,?,?,GETDATE())" ) )
		{
			int iItemID = psItem->sItemID.ToInt();

			pcDB->BindParameterInput( 1, PARAMTYPE_Integer, &iItemID );
			pcDB->BindParameterInput( 2, PARAMTYPE_Integer, &psItem->iChk1 );
			pcDB->BindParameterInput( 3, PARAMTYPE_Integer, &psItem->iChk2 );
			pcDB->BindParameterInput( 4, PARAMTYPE_Binary, psItem, sizeof( Item ) );

			pcDB->Execute();
		}
		pcDB->Close();

		return TRUE;
	}

	return FALSE;
}

BOOL ItemServer::OnLoadItemData( ItemLoadData * psItem, const char * pszCode )
{
	BOOL bRet = FALSE;
	std::vector<std::string> v = split( pszCode, '@' );

	if ( v.size() == 2 )
	{
		int iCode1 = atoi( v[0].c_str() );
		int iCode2 = atoi( v[1].c_str() );

		SQLConnection * pcDB = SQLCONNECTION( DATABASEID_LogDB, 17 );
		if( pcDB->Open() )
		{
			if( pcDB->Prepare( "SELECT TOP 1 Item FROM ItemCreateLog WHERE (Code1=?) AND (Code2=?)" ) )
			{
				pcDB->BindParameterInput( 1, PARAMTYPE_Integer, &iCode1 );
				pcDB->BindParameterInput( 2, PARAMTYPE_Integer, &iCode2 );

				if( pcDB->Execute() && pcDB->Fetch() )
				{
					pcDB->GetBinaryData( 1, &psItem->sItem, sizeof( Item ) );
					bRet = TRUE;
				}
			}
			pcDB->Close();
		}
	}

	return bRet;
}


ItemPremium	* ItemServer::GetPremiumUse( User * pcUser, EItemTimerType eType )
{
	if ( pcUser == NULL )
		return NULL;

	for ( int i = 0; i < PREMIUM_ITEM_TOTAL; i++ )
	{
		if ( pcUser->sPremiumData.saItemList[i].eType == eType )
		{
			if ( pcUser->sPremiumData.saItemList[i].dwTimeLeft )
				return &pcUser->sPremiumData.saItemList[i];
			else
				return NULL;
		}
	}

	return NULL;
}

ItemPremium * ItemServer::GetPremiumUse( User * pcUser, EItemID eID )
{
	if ( pcUser == NULL )
		return NULL;

	for ( int i = 0; i < PREMIUM_ITEM_TOTAL; i++ )
	{
		if ( pcUser->sPremiumData.saItemList[ i ].dwCode == eID )
			return &pcUser->sPremiumData.saItemList[ i ];
	}

	return NULL;
}

BOOL ItemServer::IsHead(int iType)
{
	BOOL bRet = FALSE;

	switch (iType)
	{
	case ITEMTIMERTYPE_BigHead:
	case ITEMTIMERTYPE_BigHeadHappiness:
	case ITEMTIMERTYPE_BigHeadLove:
	case ITEMTIMERTYPE_BigHeadValentine:
	case ITEMTIMERTYPE_BigHeadSadness:
	case ITEMTIMERTYPE_BigHeadShyness:
	case ITEMTIMERTYPE_BigHeadAngry:
	case ITEMTIMERTYPE_BigHeadSurprised:
	case ITEMTIMERTYPE_BigHeadSensual:
	case ITEMTIMERTYPE_CartolaHat:
	case ITEMTIMERTYPE_WitchHat:
	case ITEMTIMERTYPE_ChristmasGreenHat:
	case ITEMTIMERTYPE_ChristmasRedHat:
	case ITEMTIMERTYPE_SheepHat:
	case ITEMTIMERTYPE_EasterSheepHat:
	case ITEMTIMERTYPE_GiraffeHat:
	case ITEMTIMERTYPE_SoccerHat:
	case ITEMTIMERTYPE_SoccerHatSpeed:
	case ITEMTIMERTYPE_PumpkinHat:
		bRet = TRUE;
		break;

	default:
		break;
	}

	return bRet;
}

BOOL ItemServer::SQLLoadPremiumData(User * pcUser, EItemTimerType eItemTimerType)
{
	EItemID iItemID = ITEMID_None;
	DWORD dwTimeLeft = 0, dwTimeTotal = 0;

	if ( SQLGetItemTimer( pcUser, eItemTimerType, iItemID, dwTimeLeft, dwTimeTotal ) )
	{
		pcUser->sPremiumData.saItemList[pcUser->sPremiumData.iCount].eType			= eItemTimerType;
		pcUser->sPremiumData.saItemList[pcUser->sPremiumData.iCount].dwCode			= iItemID;
		pcUser->sPremiumData.saItemList[pcUser->sPremiumData.iCount].dwTimeLeft		= dwTimeLeft;
		pcUser->sPremiumData.saItemList[pcUser->sPremiumData.iCount].dwTimeTotal		= dwTimeTotal;
		pcUser->sPremiumData.iCount++;

		UpdateForceOrb( pcUser, eItemTimerType, iItemID );

		return TRUE;
	}

	UpdateUserEXPBuff(pcUser);

	return FALSE;
}

/// <summary>
/// Load premiums from database and sent to user
/// </summary>
/// <param name="pcUser"></param>
void ItemServer::OnLoadUser( User * pcUser )
{
	//Note - only used by game servers

	ZeroMemory( &pcUser->sPremiumData, sizeof( PacketItemPremiumData ) );
	pcUser->sPremiumData.iSize		= sizeof( PacketItemPremiumData );
	pcUser->sPremiumData.iHeader	= PKTHDR_PremiumData;

	EItemID iItemID = ITEMID_None;
	int iType = 0;
	DWORD dwTimeLeft = 0, dwTimeTotal = 0;

	BOOL bHasAnyHeadModifications = FALSE;

	SQLLoadPremiumData( pcUser, ITEMTIMERTYPE_ExpUp );

	SQLLoadPremiumData( pcUser, ITEMTIMERTYPE_ThirdEye );
	SQLLoadPremiumData( pcUser, ITEMTIMERTYPE_MPDown );
	SQLLoadPremiumData( pcUser, ITEMTIMERTYPE_MPUp );
	SQLLoadPremiumData( pcUser, ITEMTIMERTYPE_HPUp );
	SQLLoadPremiumData( pcUser, ITEMTIMERTYPE_SPDown );
	SQLLoadPremiumData( pcUser, ITEMTIMERTYPE_SixthSense );
	SQLLoadPremiumData( pcUser, ITEMTIMERTYPE_PetSystem );

	if (SQLLoadPremiumData( pcUser, ITEMTIMERTYPE_BigHead ) ||
		SQLLoadPremiumData( pcUser, ITEMTIMERTYPE_CartolaHat ) ||
		SQLLoadPremiumData( pcUser, ITEMTIMERTYPE_WitchHat ) ||
		SQLLoadPremiumData( pcUser, ITEMTIMERTYPE_GiraffeHat ) ||
		SQLLoadPremiumData( pcUser, ITEMTIMERTYPE_SheepHat ) ||
		SQLLoadPremiumData( pcUser, ITEMTIMERTYPE_EasterSheepHat ) ||
		SQLLoadPremiumData( pcUser, ITEMTIMERTYPE_SoccerHat ) ||
		SQLLoadPremiumData( pcUser, ITEMTIMERTYPE_SoccerHatSpeed ) ||
		SQLLoadPremiumData( pcUser, ITEMTIMERTYPE_ChristmasGreenHat ) ||
		SQLLoadPremiumData( pcUser, ITEMTIMERTYPE_ChristmasRedHat ) ||
		SQLLoadPremiumData( pcUser, ITEMTIMERTYPE_BigHeadHappiness ) ||
		SQLLoadPremiumData( pcUser, ITEMTIMERTYPE_BigHeadLove ) ||
		SQLLoadPremiumData( pcUser, ITEMTIMERTYPE_BigHeadSadness ) ||
		SQLLoadPremiumData( pcUser, ITEMTIMERTYPE_BigHeadShyness ) ||
		SQLLoadPremiumData( pcUser, ITEMTIMERTYPE_BigHeadAngry ) ||
		SQLLoadPremiumData( pcUser, ITEMTIMERTYPE_BigHeadSurprised ) ||
		SQLLoadPremiumData( pcUser, ITEMTIMERTYPE_BigHeadSensual ))
	{
		bHasAnyHeadModifications = TRUE;
	}

	SQLLoadPremiumData( pcUser, ITEMTIMERTYPE_PhoenixFire );
	SQLLoadPremiumData( pcUser, ITEMTIMERTYPE_PhoenixIce );
	SQLLoadPremiumData( pcUser, ITEMTIMERTYPE_PhoenixLightning );
	SQLLoadPremiumData( pcUser, ITEMTIMERTYPE_PhoenixHeal );
	SQLLoadPremiumData( pcUser, ITEMTIMERTYPE_PhoenixFireSmall);
	SQLLoadPremiumData( pcUser, ITEMTIMERTYPE_PhoenixIceSmall);
	SQLLoadPremiumData( pcUser, ITEMTIMERTYPE_PhoenixLightningSmall);
	SQLLoadPremiumData( pcUser, ITEMTIMERTYPE_PhoenixHealingSmall);
	SQLLoadPremiumData( pcUser, ITEMTIMERTYPE_WeightStone );
	SQLLoadPremiumData( pcUser, ITEMTIMERTYPE_BCBuff );
	SQLLoadPremiumData( pcUser, ITEMTIMERTYPE_ForceOrb );
	SQLLoadPremiumData( pcUser, ITEMTIMERTYPE_HoneyDamage );
	SQLLoadPremiumData( pcUser, ITEMTIMERTYPE_HoneyFast );
	SQLLoadPremiumData( pcUser, ITEMTIMERTYPE_HoneySmart );
	SQLLoadPremiumData( pcUser, ITEMTIMERTYPE_HoneyDefense );
	SQLLoadPremiumData( pcUser, ITEMTIMERTYPE_IceResistanceBuff );

	SQLLoadPremiumData( pcUser, ITEMTIMERTYPE_ChelseaSoccer );
	SQLLoadPremiumData( pcUser, ITEMTIMERTYPE_ManchesterSoccer );

	//Server wide Buff Loading
	SQLLoadPremiumData(pcUser, ITEMTIMERTYPE_SHPRegen);
	SQLLoadPremiumData(pcUser, ITEMTIMERTYPE_SSPRegen);
	SQLLoadPremiumData(pcUser, ITEMTIMERTYPE_SMPRegen);
	SQLLoadPremiumData(pcUser, ITEMTIMERTYPE_SMPDown);
	SQLLoadPremiumData(pcUser, ITEMTIMERTYPE_SSPDown);
	SQLLoadPremiumData(pcUser, ITEMTIMERTYPE_SBonusDamage);
	SQLLoadPremiumData(pcUser, ITEMTIMERTYPE_SBonusAbsorb);
	SQLLoadPremiumData(pcUser, ITEMTIMERTYPE_SMoveSpeed);
	SQLLoadPremiumData(pcUser, ITEMTIMERTYPE_SExpBuff);
	SQLLoadPremiumData(pcUser, ITEMTIMERTYPE_SDropBuff);

	if (EVENT_VALENTINESDAY)
	{
		int eventType = CEventServer::ServerEventID::Valentine;
		int timerType = ITEMTIMERTYPE_BigHeadValentine;
		int itemID = ITEMID_BigHeadValentine;


		if (!SQLLoadPremiumData(pcUser, ITEMTIMERTYPE_BigHeadValentine) && bHasAnyHeadModifications == false)
		{
			SQLConnection * pcDB = SQLCONNECTION( DATABASEID_LogDB, 18 );
			if (pcDB->Open())
			{
				//check if given out
				if (pcDB->Prepare("SELECT TOP(1) FROM EventLog WHERE EventType=? AND CharacterID=?"))
				{
					pcDB->BindParameterInput(1, PARAMTYPE_Integer, &eventType);
					pcDB->BindParameterInput(2, PARAMTYPE_Integer, &pcUser->iCharacterID);

					BOOL bFound = FALSE;

					if (pcDB->Execute() && pcDB->Fetch())
					{
						//event item already given out
						// but user must have cancelled it

						bFound = TRUE;

					}

					if (bFound)
					{
						pcDB->Close();
					}
					else
					{
						pcDB->Clear();

						if (pcDB->Prepare("INSERT INTO EventLog ([CharacterID],[CharacterName],[EventType],[EventValue],[Date]) VALUES (?,?,8,0,GETDATE())"))
						{
							pcDB->BindParameterInput(1, PARAMTYPE_Integer, &pcUser->iCharacterID);
							pcDB->BindParameterInput(2, PARAMTYPE_String, CHARACTERSERVER->GetCharacterName(pcUser->pcUserData));

							pcDB->Execute();
						}

						pcDB->Close();

						SQLAddItemTimer( pcUser->iCharacterID, CHARACTERSERVER->GetCharacterName( pcUser->pcUserData ), iType, (EItemID)itemID, dwTimeTotal );

						pcUser->sPremiumData.saItemList[pcUser->sPremiumData.iCount].eType			= (EItemTimerType)iType;
						pcUser->sPremiumData.saItemList[pcUser->sPremiumData.iCount].dwCode			= ITEMID_BigHeadValentine;
						pcUser->sPremiumData.saItemList[pcUser->sPremiumData.iCount].dwTimeLeft		= ITEMTIMERTIME_12H;
						pcUser->sPremiumData.saItemList[pcUser->sPremiumData.iCount].dwTimeTotal	= ITEMTIMERTIME_12H;
						pcUser->sPremiumData.iCount++;
					}
				}
			}
		}
	}



	SENDPACKET( pcUser, &pcUser->sPremiumData );

	//From this point onwards, the only times when client receive premium data is
	//when the server sends a sync.
	pcUser->sPremiumData.iHeader = PKTHDR_PremiumDataSync;
}

/// <summary>
/// User logged out or disconnected. Update timers in database.
/// </summary>
void ItemServer::OnUnLoadUser( User * pcUser )
{
	SyncPremiumTimersToDatabase( pcUser );
}

void ItemServer::SyncPremiumTimersToDatabase ( User * pcUser )
{
	//DEBUG( "SyncPremiumTimersToDatabase" );

	for ( int i = 0; i < PREMIUM_ITEM_TOTAL; i++ )
	{
		ItemPremium * psItem = &pcUser->sPremiumData.saItemList[i];

		if ( psItem->dwCode == 0 )
			continue;

		if ( psItem->eType == ITEMTIMERTYPE_None )
			continue;

		SQLUpdateItemTimer( pcUser, psItem->eType, psItem->dwTimeLeft );
	}
}


void ItemServer::SyncPremiumTimersToClient ( User * pcUser )
{
	//DEBUG( "SyncPremiumTimersToClientAnd" );

	BOOL bAnyToSync = FALSE;

	for ( int i = 0; i < PREMIUM_ITEM_TOTAL; i++ )
	{
		ItemPremium * psItem = &pcUser->sPremiumData.saItemList[i];

		if ( psItem->dwCode == 0 )
			continue;

		if ( psItem->eType == ITEMTIMERTYPE_None )
			continue;

		bAnyToSync = TRUE;
		break;
	}

	if ( bAnyToSync )
	{
		SENDPACKET ( pcUser, &pcUser->sPremiumData );
	}
}


void ItemServer::OnUseCrystal( UserData * pcUserData, UnitData * pcUnitData, DWORD dwItemID, DWORD dwTime )
{
	PacketCrystalUse sPacket;
	sPacket.iLength		= sizeof( PacketCrystalUse );
	sPacket.iHeader		= PKTHDR_CrystalUse;
	sPacket.dwObjectID	= pcUnitData->iID;
	sPacket.dwItemID	= dwItemID;
	if ( dwTime == 0 )
		sPacket.dwTime = (pcUnitData->dwUpdateCharInfoTime - TICKCOUNT) / 1000; // 6min 50sec
	else
		sPacket.dwTime = dwTime;
	SENDPACKET( USERDATATOUSER(pcUserData), &sPacket );
}

void ItemServer::OnUseCrystalDie( UserData * pcUserData, UnitData * pcUnitData )
{
	PacketCrystalUseDie sPacket;
	sPacket.iLength		= sizeof( PacketCrystalUseDie );
	sPacket.iHeader		= PKTHDR_CrystalUseDie;
	sPacket.dwObjectID	= pcUnitData->iID;
	SENDPACKET( USERDATATOUSER(pcUserData), &sPacket );
}

void ItemServer::SQLSyncItemTimersToPrimary ( User * pcUser )
{
	//Not really needed. Just import all the data into primary server on merge
	/*if ( GAME_SERVER && SERVER_IS_SEASONAL )
	{
		SQLConnection * pcDB = SQLCONNECTION ( DATABASEID_UserDB_LocalToPrimary );

		for ( int i = 0; i < PREMIUM_ITEM_TOTAL; i++ )
		{
			ItemPremium * psItem = &pcUser->sPremiumData.saItemList[i];

			if ( psItem->eType != ITEMTIMERTYPE_None && psItem->dwTimeLeft > 2 )
			{
				SQLUpdateItemTimer ( SQLCONNECTION ( DATABASEID_UserDB_LocalToPrimary ), pcUser, psItem->eType, psItem->dwTimeLeft );
			}
		}
	}*/
}

BOOL ItemServer::SQLGetItemTimer( User * pcUser, int iType, EItemID & iItemID, DWORD & dwTimeLeft, DWORD & dwTimeTotal )
{
	//Note - before char gets loaded into game the user id is 0
	// so don't use pcUser->IsValidAndInGame
	if ( pcUser == NULL || pcUser->pcUserData == NULL || pcUser->pcUserData->pcSocketData == NULL )
		return FALSE;

	if ( pcUser->iCharacterID == 0 )
	{
		LOGERROR("ItemServer::SQLGetItemTimer: Character ID is 0 for player %s", CHARACTERSERVER->GetCharacterName(pcUser));
		return FALSE;
	}

	BOOL bRet = FALSE;

	SQLConnection * pcDB = SQLCONNECTION( DATABASEID_UserDB_LocalServer_Timers, 1 );
	if( pcDB->Open() )
	{
		if( pcDB->Prepare( "SELECT ItemID, TimeLeft, TimeTotal FROM CharacterItemTimer WHERE CharacterID=? AND ItemTimerType=?" ) )
		{
			pcDB->BindParameterInput( 1, PARAMTYPE_Integer, &pcUser->iCharacterID );
			pcDB->BindParameterInput( 2, PARAMTYPE_Integer, &iType );

			if( (pcDB->Execute()) && (pcDB->Fetch()) )
			{
				int iID;
				pcDB->GetData( 1, PARAMTYPE_Integer, &iID);
				pcDB->GetData( 2, PARAMTYPE_Integer, &dwTimeLeft );
				pcDB->GetData( 3, PARAMTYPE_Integer, &dwTimeTotal );

				iItemID = (EItemID)iID;

				bRet = TRUE;
			}
		}
		pcDB->Close();
	}

	return bRet;
}

void ItemServer::SQLUpdateHeadModel ( User * pcUser, char * headModel, int iSqlCharacterId )
{
	SQLUpdateHeadModel ( SQLCONNECTION ( DATABASEID_UserDB_LocalServer_CharInfo ),
		pcUser,
		pcUser->pcUserData->sCharacterData.Player.szHeadModel,
		pcUser->iCharacterID );

	if ( SERVER_IS_SEASONAL )
	{
		SQLUpdateHeadModel ( SQLCONNECTION ( DATABASEID_UserDB_PrimaryServerSyncer ),
			pcUser,
			pcUser->pcUserData->sCharacterData.Player.szHeadModel,
			pcUser->iCharacterID );
	}
}


void ItemServer::StoreItemMiscData(DWORD p_ID, int p_Price, int p_MaxIntegrity, int p_LevelReq)
{
	s_ItemBuyPriceMap[p_ID]			= p_Price;
	s_ItemBuyMaxIntegrityMap[p_ID]	= p_MaxIntegrity;
	s_ItemIdToLevelReqMap[p_ID]	= p_LevelReq;
}

BOOL ItemServer::IsItemAbleToHaveRarity( Item * psItem )
{
	if ( psItem->iItemUniqueID == UNIQUEID_QUEST_WEAPON )
		return FALSE;

	const EItemID itemID = psItem->sItemID.ToItemID();
	const EItemBase itemBase = psItem->sItemID.ToItemBase();
	const EItemType itemType = psItem->sItemID.ToItemType();

	if (itemBase != EItemBase::ITEMBASE_Weapon
		&& itemType != EItemType::ITEMTYPE_Armor
		&& itemType != EItemType::ITEMTYPE_Robe
		&& itemType != EItemType::ITEMTYPE_Amulet
		&& itemType != EItemType::ITEMTYPE_Shield
		&& itemType != EItemType::ITEMTYPE_Boots
		&& itemType != EItemType::ITEMTYPE_Gauntlets
		&& itemType != EItemType::ITEMTYPE_Bracelets
		&& itemType != EItemType::ITEMTYPE_Ring
		&& itemType != EItemType::ITEMTYPE_Ring2
		&& itemType != EItemType::ITEMTYPE_Orb
		&& !ITEMSERVER->IsBossOrSpecialItem(itemID))
	{
		return FALSE;
	}

	return TRUE;
}

BOOL ItemServer::IsBossOrSpecialItem( Item * psItem )
{
	return IsBossOrSpecialItem( psItem->sItemID.ToItemID() );
}

BOOL ItemServer::IsBossOrSpecialItem( DWORD p_ID )
{
	switch ( p_ID )
	{
		//boss items
		case EItemID::ITEMID_MokovaBoots:
		case EItemID::ITEMID_DraxosBoots:
		case EItemID::ITEMID_ValentoRing:
		case EItemID::ITEMID_KelvezuAmulet:
		case EItemID::ITEMID_DevilShyRing:
		case EItemID::ITEMID_GreedyGauntlets:
		case EItemID::ITEMID_TullaAmulet:
		case EItemID::ITEMID_AbyssRing:
		case EItemID::ITEMID_IgnisBracelets:
			return TRUE;


		//special items
		case EItemID::ITEMID_Season4CostumeArmor:
		case EItemID::ITEMID_Season3CostumeArmor:
		case EItemID::ITEMID_Season2CostumeArmor:
		case EItemID::ITEMID_Season1CostumeArmor:
			return TRUE;
	}

	return FALSE;
}


int ItemServer:: GetItemLevelRequirement( DWORD p_ID )
{
	if (s_ItemIdToLevelReqMap.find( p_ID ) == s_ItemIdToLevelReqMap.end())
		return 0;

	return s_ItemIdToLevelReqMap[p_ID];
}

int ItemServer::GetItemBuyPrice(DWORD p_ID, bool p_HasSpec, int iItemRarityNum)
{
	if (s_ItemBuyPriceMap.find(p_ID) == s_ItemBuyPriceMap.end())
		return std::numeric_limits<int>::max();

	float buyPrice = static_cast<float>( s_ItemBuyPriceMap[p_ID] );

	if (p_HasSpec)
		buyPrice *= 1.2f;

	//boss or special items have no price change even though
	//they have Legendary status
	if ( !IsBossOrSpecialItem( p_ID ) )
	{
		if ( iItemRarityNum > 0 )
		{
			buyPrice = ( buyPrice / 2.0f ) * ( iItemRarityNum );
		}
	}

	return static_cast<int>( round( buyPrice ) );
}

int ItemServer::GetItemAgingPrice(struct Item* pcItem, int p_CurrentAgeLevel)
{
	float originalPrice = static_cast<float>(pcItem->iSalePrice);
	float agingPrice = 0.0f;

	//Note - no need to multiple the original price because
	//It's already multiplied by 1.2f if it has a spec.
	agingPrice = (originalPrice * (p_CurrentAgeLevel + 1)) / 2.0f;

	//x 1.1 is related to BC tax
	//int cSHOP::haShopItemPrice(int Money)
	//todo - find BC tax rate and use that.
	//agingPrice *= 1.1;

	//Apply Bless Castle Tax to match what client would 'see'
	//agingPrice += ( agingPrice * BLESSCASTLESERVER->GetCurrentTax () ) / 100;

	return static_cast<int>( round( agingPrice ) );
}

int ItemServer::GetItemRepairPrice(DWORD p_ID, bool p_HasSpec, SHORT p_Durability, SHORT p_DurabilityMax, int iItemRarityNum)
{
	if (s_ItemBuyPriceMap.find(p_ID) == s_ItemBuyPriceMap.end())
		return std::numeric_limits<int>::max();

	int l_TemplateMaxDurability = s_ItemBuyMaxIntegrityMap[p_ID];

	if (l_TemplateMaxDurability < 1)
		l_TemplateMaxDurability = 1;

	if (p_DurabilityMax > l_TemplateMaxDurability)
		p_DurabilityMax = l_TemplateMaxDurability;

	if (p_Durability > p_DurabilityMax)
		p_Durability = p_DurabilityMax;

	float l_FDurability[2];
	l_FDurability[0] = static_cast<float>( std::fmax( p_Durability, 1 ) );
	l_FDurability[1] = static_cast<float>( std::fmax( p_DurabilityMax, 1 ) );

	float l_BuyPrice		= static_cast<float>(GetItemBuyPrice(p_ID, p_HasSpec, iItemRarityNum));


	float l_PureSellPrice	= static_cast<float>(static_cast<int>((l_BuyPrice * (l_FDurability[0] / l_FDurability[1])) + (l_BuyPrice - (l_BuyPrice*(l_FDurability[0] / l_FDurability[1]))) * 0.25));


	int l_RepairPrice = (static_cast<int>(l_BuyPrice) - static_cast<int>(l_PureSellPrice)) / 8;

#ifdef DEV_MODE
	INFO( "GetItemRepairPrice - BuyPrice = %0.1f", l_BuyPrice );
	INFO( "GetItemRepairPrice - PureSellPrice = %0.1f", l_PureSellPrice );
	INFO( "GetItemRepairPrice - RepairPrice = %d", l_RepairPrice );
#endif
	return l_RepairPrice;
}

int ItemServer::GetItemRepairPrice(DWORD p_ID, DWORD p_Price, bool p_HasSpec, SHORT p_Durability, SHORT p_DurabilityMax)
{
	if (s_ItemBuyPriceMap.find(p_ID) == s_ItemBuyPriceMap.end())
		return std::numeric_limits<int>::max();

	int l_TemplateMaxDurability = s_ItemBuyMaxIntegrityMap[p_ID];

	if (l_TemplateMaxDurability < 1)
		l_TemplateMaxDurability = 1;

	if (p_DurabilityMax > l_TemplateMaxDurability)
		p_DurabilityMax = l_TemplateMaxDurability;

	if (p_Durability > p_DurabilityMax)
		p_Durability = p_DurabilityMax;

	float l_FDurability[2];
	l_FDurability[0] = static_cast<float>( std::fmax( p_Durability, 1 ) );
	l_FDurability[1] = static_cast<float>( std::fmax( p_DurabilityMax, 1 ) );

	float l_BuyPrice        = static_cast<float>(p_Price) * (p_HasSpec ? 1.2f : 1.0f);
	float l_PureSellPrice	= static_cast<float>(static_cast<int>((l_BuyPrice * (l_FDurability[0] / l_FDurability[1])) + (l_BuyPrice - (l_BuyPrice*(l_FDurability[0] / l_FDurability[1]))) * 0.25));

	return (static_cast<int>(l_BuyPrice) - static_cast<int>(l_PureSellPrice)) / 8;
}

int ItemServer::GetItemSellPrice(DWORD p_ID, bool p_HasSpec, SHORT p_Durability, SHORT p_DurabilityMax, int iItemRarityNum)
{
	if (s_ItemBuyPriceMap.find(p_ID) == s_ItemBuyPriceMap.end())
		return std::numeric_limits<int>::max();

	int l_TemplateMaxDurability = s_ItemBuyMaxIntegrityMap[p_ID];

	if (l_TemplateMaxDurability < 1)
		l_TemplateMaxDurability = 1;

	if (p_DurabilityMax > l_TemplateMaxDurability)
		p_DurabilityMax = l_TemplateMaxDurability;

	if (p_Durability > p_DurabilityMax)
		p_Durability = p_DurabilityMax;

	float l_FDurability[2];
	l_FDurability[0] = static_cast<float>( std::fmax( p_Durability, 1 ) );
	l_FDurability[1] = static_cast<float>( std::fmax( p_DurabilityMax, 1 ) );

	float l_BuyPrice = static_cast<float>(GetItemBuyPrice(p_ID, p_HasSpec, iItemRarityNum));

	int l_PureSellPrice = static_cast<int>( round( ( ( l_BuyPrice * ( l_FDurability[0] / l_FDurability[1] ) ) + ( l_BuyPrice - ( l_BuyPrice * ( l_FDurability[0] / l_FDurability[1] ) ) ) * 0.25f ) ) );
	l_PureSellPrice = l_PureSellPrice / 5;

	int l_PriceMax = static_cast<int>(l_BuyPrice) / 5;

	return (l_PriceMax < l_PureSellPrice) ? l_PriceMax : l_PureSellPrice;
}

int ItemServer::GetItemSellPrice(DWORD p_ID, DWORD p_Price, bool p_HasSpec, SHORT p_Durability, SHORT p_DurabilityMax)
{
	if (s_ItemBuyPriceMap.find(p_ID) == s_ItemBuyPriceMap.end())
		return std::numeric_limits<int>::max();

	int l_TemplateMaxDurability = s_ItemBuyMaxIntegrityMap[p_ID];

	if (l_TemplateMaxDurability < 1)
		l_TemplateMaxDurability = 1;

	if (p_DurabilityMax > l_TemplateMaxDurability)
		p_DurabilityMax = l_TemplateMaxDurability;

	if (p_Durability > p_DurabilityMax)
		p_Durability = p_DurabilityMax;

	float l_FDurability[2];
	l_FDurability[0] = static_cast<float>( std::fmax( p_Durability, 1 ) );
	l_FDurability[1] = static_cast<float>( std::fmax( p_DurabilityMax, 1 ) );

	float l_BuyPrice = static_cast<float>(p_Price) * (p_HasSpec ? 1.2f : 1.0f);

	float l_PureSellPrice = ((l_BuyPrice * (l_FDurability[0] / l_FDurability[1])) + (l_BuyPrice - (l_BuyPrice*(l_FDurability[0] / l_FDurability[1]))) * 0.25f);
	l_PureSellPrice = l_PureSellPrice / 5.0f;

	float l_PriceMax = l_BuyPrice / 5.0f;

	return static_cast<int>( round( ( l_PriceMax < l_PureSellPrice ) ? l_PriceMax : l_PureSellPrice ) );
}


void ItemServer::SQLUpdateHeadModel ( SQLConnection * pcDB, User * pcUser, char * headModel, int iSqlCharacterId )
{
	if ( pcDB->Open () )
	{
		if ( pcDB->Prepare ( "UPDATE CharacterInfo SET OldHead=? WHERE ID=?" ) )
		{
			pcDB->BindParameterInput ( 1, PARAMTYPE_String, headModel, 64 );
			pcDB->BindParameterInput ( 2, PARAMTYPE_Integer, &iSqlCharacterId );
			pcDB->ExecuteUpdate ();
		}

		pcDB->Close ();
	}
}

const std::string ItemServer::SQLGetOldHeadModel (  User * pcUser )
{
	SQLConnection * pcDB = SQLCONNECTION ( DATABASEID_UserDB_LocalServer_CharInfo );

	char szHead[64] = { 0 };

	if ( pcDB->Open () )
	{
		if ( pcDB->Prepare ( "SELECT OldHead FROM CharacterInfo WHERE ID=?" ) )
		{
			pcDB->BindParameterInput ( 1, PARAMTYPE_Integer, &pcUser->iCharacterID );

			if ( pcDB->Execute () && pcDB->Fetch () )
			{
				pcDB->GetData ( 1, PARAMTYPE_String, szHead, _countof ( szHead ) );

				if ( IsNull ( szHead ) )
				{
					ZeroMemory ( szHead, _countof ( szHead ) );
				}
			}
		}

		pcDB->Close ();
	}

	return std::string(szHead);
}



void ItemServer::SQLUpdateItemTimer ( User * pcUser, int iType, DWORD dwTimeLeft )
{
	SQLUpdateItemTimer ( SQLCONNECTION ( DATABASEID_UserDB_LocalServer_Timers, 2 ), pcUser, iType, dwTimeLeft );

	//Not really needed. Just import all the data into primary server on merge
	/*if ( SERVER_IS_SEASONAL )
	{
		SQLUpdateItemTimer ( SQLCONNECTION ( DATABASEID_UserDB_PrimarySyncer ), pcUser, iType, dwTimeLeft );
	}*/
}

void ItemServer::SQLUpdateItemTimer ( SQLConnection * pcDB, User * pcUser, int iType, DWORD dwTimeLeft )
{
	if ( pcUser->iCharacterID == 0 )
	{
		WARN("ItemServer::SQLUpdateItemTimer: Character ID is 0 for player %s", CHARACTERSERVER->GetCharacterName(pcUser));
		return;
	}

	if( pcDB->Open() )
	{
		if( pcDB->Prepare( "UPDATE CharacterItemTimer SET TimeLeft=? WHERE CharacterID=? AND ItemTimerType=? AND TimeLeft>=?" ) )
		{
			pcDB->BindParameterInput( 1, PARAMTYPE_Integer, &dwTimeLeft );
			pcDB->BindParameterInput( 2, PARAMTYPE_Integer, &pcUser->iCharacterID );
			pcDB->BindParameterInput( 3, PARAMTYPE_Integer, &iType );
			pcDB->BindParameterInput( 4, PARAMTYPE_Integer, &dwTimeLeft );

			if (!pcDB->ExecuteUpdate())
			{
				DEBUG( "Update failed for iType = %d, dwTimeLeft = %d", iType, dwTimeLeft );
			}
		}

		pcDB->Close();
	}
}

void ItemServer::SQLDeleteItemTimerByTimerType ( User * pcUser, enum EItemTimerType eItemTimerType )
{
	//DEBUG( "SQLDeleteItemTimerByTimerType" );

	SQLDeleteItemTimerByTimerType ( SQLCONNECTION ( DATABASEID_UserDB_LocalServer_Timers, 3 ), pcUser, eItemTimerType );

	//Not really needed. Just import all the data into primary server on merge
	/*if ( SERVER_IS_SEASONAL )
	{
		SQLDeleteItemTimer ( SQLCONNECTION ( DATABASEID_UserDB_LocalToPrimary ), pcUser, psItem->dwCode );
	}*/
}

void ItemServer::SQLDeleteItemTimerByTimerType ( SQLConnection * pcDB, User * pcUser, enum EItemTimerType eItemTimerType )
{
	//std::cout << "SQLDeleteItemTimerByTimerType: " << eItemTimerType <<  std::endl;

	if ( pcUser->iCharacterID == 0 )
	{
		WARN("ItemServer::SQLDeleteItemTimerByTimerType: Character ID is 0 for player %s", CHARACTERSERVER->GetCharacterName(pcUser));
		return;
	}

	if ( pcDB->Open () )
	{
		if ( pcDB->Prepare ( "DELETE FROM CharacterItemTimer WHERE CharacterID=? AND ItemTimerType=?" ) )
		{
			pcDB->BindParameterInput ( 1, PARAMTYPE_Integer, &pcUser->iCharacterID );
			pcDB->BindParameterInput ( 2, PARAMTYPE_Integer, &eItemTimerType );

			pcDB->Execute ();
		}

		pcDB->Close ();
	}
}

void ItemServer::SQLDeleteItemTimer ( User * pcUser, int iItemID )
{
	SQLDeleteItemTimer ( SQLCONNECTION ( DATABASEID_UserDB_LocalServer_Timers, 4 ), pcUser, iItemID );

	//Not really needed. Just import all the data into primary server on merge
	/*if ( SERVER_IS_SEASONAL )
	{
		SQLDeleteItemTimer ( SQLCONNECTION ( DATABASEID_UserDB_LocalToPrimary ), pcUser, psItem->dwCode );
	}*/
}

void ItemServer::SQLDeleteItemTimer( SQLConnection * pcDB, User * pcUser, int iItemID )
{
	if ( pcUser->iCharacterID == 0 )
	{
		WARN("ItemServer::SQLDeleteItemTimer: Character ID is 0 for player %s", CHARACTERSERVER->GetCharacterName(pcUser));
		return;
	}

	if( pcDB->Open() )
	{
		if( pcDB->Prepare( "DELETE FROM CharacterItemTimer WHERE CharacterID=? AND ItemID=?" ) )
		{
			pcDB->BindParameterInput( 1, PARAMTYPE_Integer, &pcUser->iCharacterID );
			pcDB->BindParameterInput( 2, PARAMTYPE_Integer, &iItemID );

			pcDB->Execute();
		}

		pcDB->Close();
	}
}

void ItemServer::SQLAddItemTimer ( int iCharID, char * pszCharName, int iType, EItemID iItemID, DWORD dwTimeTotal )
{
	SQLAddItemTimer ( SQLCONNECTION ( DATABASEID_UserDB_LocalServer_Timers, 5 ), iCharID, pszCharName, iType, iItemID, dwTimeTotal );
}

void ItemServer::SQLAddItemTimer ( SQLConnection * pcDB, int iCharID, char * pszCharName, int iType, EItemID iItemID, DWORD dwTimeTotal )
{
	if ( iCharID == 0 )
	{
		LOGERROR("ItemServer::SQLAddItemTimer: Character ID is 0 for player %s", pszCharName);
		return;
	}

	if ( pcDB->Open () )
	{
		if ( pcDB->Prepare ( "INSERT INTO CharacterItemTimer([CharacterID],[CharacterName],[ItemTimerType],[ItemID],[TimeLeft],[TimeTotal]) VALUES(?,?,?,?,?,?)" ) )
		{
			pcDB->BindParameterInput ( 1, PARAMTYPE_Integer, &iCharID );
			pcDB->BindParameterInput ( 2, PARAMTYPE_String, pszCharName, 32 );
			pcDB->BindParameterInput ( 3, PARAMTYPE_Integer, &iType );
			pcDB->BindParameterInput ( 4, PARAMTYPE_Integer, &iItemID );
			pcDB->BindParameterInput ( 5, PARAMTYPE_Integer, &dwTimeTotal );
			pcDB->BindParameterInput ( 6, PARAMTYPE_Integer, &dwTimeTotal );

			if ( !pcDB->Execute () )
			{
				WARN ( "SQLAddItemTimer FAILED for item: %d for user: %s", iType, pszCharName);
			}
		}
		pcDB->Close ();
	}
}


void ItemServer::UpdateItemRarityMods()
{
	mRarityChanceMod.clear();

	auto pcDB = SQLCONNECTION( DATABASEID_GameDB, 3 );
	if ( pcDB->Open() )
	{
		if ( pcDB->Prepare( "SELECT [Type],[ModCommon],[ModUncommon],[ModRare],[ModEpic],[ModLegendary] FROM RarityChanceMod" ) )
		{
			if ( pcDB->Execute() )
			{
				while ( pcDB->Fetch() )
				{
					ItemRarityChanceMod mod{};

					int iType;
					pcDB->GetData ( 1, PARAMTYPE_Integer, &iType );
					pcDB->GetData ( 2, PARAMTYPE_Float, &mod.fModCommon );
					pcDB->GetData ( 3, PARAMTYPE_Float, &mod.fModUncommon );
					pcDB->GetData ( 4, PARAMTYPE_Float, &mod.fModRare );
					pcDB->GetData ( 5, PARAMTYPE_Float, &mod.fModEpic );
					pcDB->GetData ( 6, PARAMTYPE_Float, &mod.fModLegendary );

					mRarityChanceMod.insert( { (ERarityChanceModType)iType, mod } );
				}
			}
		}

		pcDB->Close();
	}
}

void ItemServer::UpdateItemRarityChanceGroup ()
{
	for (ItemRarityChanceGroup* ps : RarityChanceGroupList)
	{
		SAFE_DELETE(ps);
	}
	RarityChanceGroupList.clear ();

	auto pcDB = SQLCONNECTION( DATABASEID_GameDB, 3 );
	if ( pcDB->Open () )
	{
		if ( pcDB->Prepare ( "SELECT [ID],[MinLevel],[MaxLevel] FROM RarityChanceGroup" ) )
		{
			if ( pcDB->Execute ())
			{
				while ( pcDB->Fetch () )
				{
					ItemRarityChanceGroup* CurrentItemRarityChanceGroup = new ItemRarityChanceGroup();

					int iID = 0;
					pcDB->GetData ( 1, PARAMTYPE_Integer, &iID );
					CurrentItemRarityChanceGroup->iRarityGroup = iID ;

					int iMinLevel = 0;
					pcDB->GetData ( 2, PARAMTYPE_Integer, &iMinLevel );
					CurrentItemRarityChanceGroup->MinLevel = iMinLevel ;

					int iMaxLevel = 0;
					pcDB->GetData ( 3, PARAMTYPE_Integer, &iMaxLevel );
					CurrentItemRarityChanceGroup->MaxLevel = iMaxLevel;

					RarityChanceGroupList.push_back ( CurrentItemRarityChanceGroup );
				}
			}
		}
		pcDB->Close();
	}

}

void ItemServer::GetBaselineMod()
{
	for (ItemBaselineMod* ps : ItemBaselineModList)
	{
		SAFE_DELETE(ps);
	}
	ItemBaselineModList.clear();

	auto pcDB = SQLCONNECTION(DATABASEID_GameDB, 3);
	if (pcDB->Open())
	{
		if (pcDB->Prepare("SELECT [Class],[PercentBaselineStrength],[PercentBaselineSpirit],[PercentBaselineTalent],[PercentBaselineAgility],[PercentBaselineHealth] FROM BaselineMod"))
		{
			if (pcDB->Execute())
			{
				while (pcDB->Fetch())
				{
					ItemBaselineMod* CurrentBaselineMod = new ItemBaselineMod();

					pcDB->GetData(1, PARAMTYPE_Integer, &CurrentBaselineMod->iClass);
					pcDB->GetData(2, PARAMTYPE_Integer, &CurrentBaselineMod->eStatModifier.iStrength);
					pcDB->GetData(3, PARAMTYPE_Integer, &CurrentBaselineMod->eStatModifier.iSpirit);
					pcDB->GetData(4, PARAMTYPE_Integer, &CurrentBaselineMod->eStatModifier.iTalent);
					pcDB->GetData(5, PARAMTYPE_Integer, &CurrentBaselineMod->eStatModifier.iAgility);
					pcDB->GetData(6, PARAMTYPE_Integer, &CurrentBaselineMod->eStatModifier.iHealth);

					ItemBaselineModList.push_back(CurrentBaselineMod);
				}
			}
		}
		pcDB->Close();
	}

}

void ItemServer::GetItemSpecMod()
{
	for (ItemSpecMod* ps : ItemSpecModList)
	{
		SAFE_DELETE(ps);
	}
	ItemSpecModList.clear();

	auto pcDB = SQLCONNECTION(DATABASEID_GameDB, 3);
	if (pcDB->Open())
	{
		if (pcDB->Prepare("SELECT [AddSpecClass01],[AddSpecClass02],[AddSpecClass03],[AddSpecClass04],[AddSpecClass05],[AddSpecClass06],[AddSpecClass07],[AddSpecClass08],[AddSpecClass09],[AddSpecClass10],[ItemType],[PercentReqStrength],[PercentReqSpirit],[PercentReqTalent],[PercentReqAgility],[PercentReqHealth] FROM ItemSpecMod"))
		{
			if (pcDB->Execute())
			{
				while (pcDB->Fetch())
				{
					ItemSpecMod* CurrentItemSpecMod = new ItemSpecMod();

					pcDB->GetData(1, PARAMTYPE_Integer, &CurrentItemSpecMod->iSpecClass01);
					pcDB->GetData(2, PARAMTYPE_Integer, &CurrentItemSpecMod->iSpecClass02);
					pcDB->GetData(3, PARAMTYPE_Integer, &CurrentItemSpecMod->iSpecClass03);
					pcDB->GetData(4, PARAMTYPE_Integer, &CurrentItemSpecMod->iSpecClass04);
					pcDB->GetData(5, PARAMTYPE_Integer, &CurrentItemSpecMod->iSpecClass05);
					pcDB->GetData(6, PARAMTYPE_Integer, &CurrentItemSpecMod->iSpecClass06);
					pcDB->GetData(7, PARAMTYPE_Integer, &CurrentItemSpecMod->iSpecClass07);
					pcDB->GetData(8, PARAMTYPE_Integer, &CurrentItemSpecMod->iSpecClass08);
					pcDB->GetData(9, PARAMTYPE_Integer, &CurrentItemSpecMod->iSpecClass09);
					pcDB->GetData(10, PARAMTYPE_Integer, &CurrentItemSpecMod->iSpecClass10);

					pcDB->GetData(11, PARAMTYPE_String, CurrentItemSpecMod->sItemType, 12);

					pcDB->GetData(12, PARAMTYPE_Integer, &CurrentItemSpecMod->eStatModifier.iStrength);
					pcDB->GetData(13, PARAMTYPE_Integer, &CurrentItemSpecMod->eStatModifier.iSpirit);
					pcDB->GetData(14, PARAMTYPE_Integer, &CurrentItemSpecMod->eStatModifier.iTalent);
					pcDB->GetData(15, PARAMTYPE_Integer, &CurrentItemSpecMod->eStatModifier.iAgility);
					pcDB->GetData(16, PARAMTYPE_Integer, &CurrentItemSpecMod->eStatModifier.iHealth);

					ItemSpecModList.push_back(CurrentItemSpecMod);
				}
			}
		}
		pcDB->Close();
	}

}

void ItemServer::SetRarityDB (int iRarityGroup, int iRarity, int iChance )
{
	auto pcDB = SQLCONNECTION( DATABASEID_GameDB, 4 );
	if ( pcDB->Open () )
	{
		if ( pcDB->Prepare ( "UPDATE RarityChance SET Chance=? WHERE RarityChanceGroup=? AND Rarity=?" ) )
		{
			pcDB->BindParameterInput ( 1, PARAMTYPE_Integer, &iChance );
			pcDB->BindParameterInput ( 2, PARAMTYPE_Integer, &iRarityGroup );
			pcDB->BindParameterInput ( 3, PARAMTYPE_Integer, &iRarity );
			pcDB->ExecuteUpdate ();
		}

		pcDB->Close ();
	}

}

int ItemServer::GetRarityChance ( int iRarityGroup, int iRarity )
{

	int ReturnChance = 0;
	int iReturnRarityGroup = 0;

	for ( ItemRarityChanceGroup * CurrentGroup : RarityChanceGroupList )
	{
		if ( iRarityGroup = CurrentGroup->iRarityGroup )
		{
			for ( ItemRarity * CurrentRarity : RarityList )
			{
				int iThisRarity = static_cast<int>( CurrentRarity->eRarity );
				if ( iThisRarity = iRarity )
				{
					ReturnChance = CurrentRarity->Chance;
					break;
				}
			}
		}
	}

	return ReturnChance;
}

void ItemServer::UpdateItemRarity ()
{
	UpdateItemRarityChanceGroup ( );
	UpdateItemRarityMods();

	for (ItemRarity* ps : RarityList)
	{
		SAFE_DELETE(ps);
	}
	RarityList.clear ();

	auto pcDB = SQLCONNECTION( DATABASEID_GameDB, 5 );
	if ( pcDB->Open () )
	{
		if ( pcDB->Prepare ( "SELECT [RarityChanceGroup],[Rarity],[Chance] FROM RarityChance" ) )
		{
			if ( pcDB->Execute ())
			{
				while ( pcDB->Fetch () )
				{
					ItemRarity* CurrentItemRarity = new ItemRarity();

					int iRarityChanceGroup = 0;
					pcDB->GetData ( 1, PARAMTYPE_Integer, &iRarityChanceGroup );
					CurrentItemRarity->iRarityGroup = iRarityChanceGroup;

					int iRarityLevel = 0;
					pcDB->GetData ( 2, PARAMTYPE_Integer, &iRarityLevel );
					CurrentItemRarity->eRarity = EItemRarity ( iRarityLevel );

					int iChance = 0;
					pcDB->GetData ( 3, PARAMTYPE_Integer, &iChance );
					CurrentItemRarity->Chance = iChance;

					RarityList.push_back ( CurrentItemRarity );
				}
			}
		}
		pcDB->Close();
	}
}

#ifdef ITEMFIXES
void ItemServer::UpdateItemFixes (SQLConnection * pcDB)
{
	for (ItemFix* ps : FixesList)
	{
		SAFE_DELETE(ps);
	}
	FixesList.clear ();

	if ( pcDB->Open () )
	{
		if ( pcDB->Prepare ( "SELECT [id],[Name],[MinLevel],[MaxLevel],[MinRarity],[MaxRarity],[isPrefix],[isSuffix],[Chance],[Axes],[Bows],[Javelins],[Daggers],[Wands],[Scythes],[Swords],[Claws],[Hammers],[Phantoms],[Armors],[Robes],[Boots],[Gauntlets],[Bracelets],[Amulets],[Rings],[Shields],[Orbs],[OrganicMin],[OrganicMax],[FireMin],[FireMax],[FrostMin],[FrostMax],[LightningMix],[LightningMax],[PoisonMin],[PoisonMax],[ATKPowMin],[ATKPowMax],[RangeMin],[RangeMax],[ATKRatingMin],[ATKRatingMax],[CriticalMin],[CriticalMax],[BlockMin],[BlockMax],[AbsorbMin],[AbsorbMax],[DefenseMin],[DefenseMax],[RegenerationHPMin],[RegenerationHPMax],[RegenerationMPMin],[RegenerationMPMax],[AddHPMin],[AddHPMax],[AddMPMin],[AddMPMax],[RunSpeedMin],[RunSpeedMax] FROM ItemFixes" ) )
		{
			if ( pcDB->Execute ())
			{
				while ( pcDB->Fetch () )
				{
					ItemFix* CurrentItemFix = new ItemFix();

					pcDB->GetData ( 1, PARAMTYPE_Short, &CurrentItemFix->iID );
					pcDB->GetData ( 2, PARAMTYPE_String, CurrentItemFix->sName, 32 );
					pcDB->GetData ( 3, PARAMTYPE_Short, &CurrentItemFix->iMinLevel );
					pcDB->GetData ( 4, PARAMTYPE_Short, &CurrentItemFix->iMaxLevel );
					pcDB->GetData ( 5, PARAMTYPE_Short, &CurrentItemFix->iMinRarity );
					pcDB->GetData ( 6, PARAMTYPE_Short, &CurrentItemFix->iMaxRarity );
					pcDB->GetData ( 7, PARAMTYPE_Short, &CurrentItemFix->isPrefix );
					pcDB->GetData ( 8, PARAMTYPE_Short, &CurrentItemFix->isSuffix );
					pcDB->GetData ( 9, PARAMTYPE_Short, &CurrentItemFix->iChance );
					pcDB->GetData ( 10, PARAMTYPE_Short, &CurrentItemFix->iAxes );
					pcDB->GetData ( 11, PARAMTYPE_Short, &CurrentItemFix->iBows );
					pcDB->GetData ( 12, PARAMTYPE_Short, &CurrentItemFix->iJavelins );
					pcDB->GetData ( 13, PARAMTYPE_Short, &CurrentItemFix->iDaggers );
					pcDB->GetData ( 14, PARAMTYPE_Short, &CurrentItemFix->iWands );
					pcDB->GetData ( 15, PARAMTYPE_Short, &CurrentItemFix->iScythes );
					pcDB->GetData ( 16, PARAMTYPE_Short, &CurrentItemFix->iSwords );
					pcDB->GetData ( 17, PARAMTYPE_Short, &CurrentItemFix->iClaws );
					pcDB->GetData ( 18, PARAMTYPE_Short, &CurrentItemFix->iHammers );
					pcDB->GetData ( 19, PARAMTYPE_Short, &CurrentItemFix->iPhantoms );
					pcDB->GetData ( 20, PARAMTYPE_Short, &CurrentItemFix->iArmors );
					pcDB->GetData ( 21, PARAMTYPE_Short, &CurrentItemFix->iRobes );
					pcDB->GetData ( 22, PARAMTYPE_Short, &CurrentItemFix->iBoots );
					pcDB->GetData ( 23, PARAMTYPE_Short, &CurrentItemFix->iGauntlets );
					pcDB->GetData ( 24, PARAMTYPE_Short, &CurrentItemFix->iBracelets );
					pcDB->GetData ( 25, PARAMTYPE_Short, &CurrentItemFix->iAmulets );
					pcDB->GetData ( 26, PARAMTYPE_Short, &CurrentItemFix->iRings );
					pcDB->GetData ( 27, PARAMTYPE_Short, &CurrentItemFix->iShields );
					pcDB->GetData ( 28, PARAMTYPE_Short, &CurrentItemFix->iOrbs );
					pcDB->GetData ( 29, PARAMTYPE_Short, &CurrentItemFix->iOrganicMin );
					pcDB->GetData ( 30, PARAMTYPE_Short, &CurrentItemFix->iOrganicMax );
					pcDB->GetData ( 31, PARAMTYPE_Short, &CurrentItemFix->iFireMin );
					pcDB->GetData ( 32, PARAMTYPE_Short, &CurrentItemFix->iFireMax );
					pcDB->GetData ( 33, PARAMTYPE_Short, &CurrentItemFix->iFrostMin );
					pcDB->GetData ( 34, PARAMTYPE_Short, &CurrentItemFix->iFrostMax );
					pcDB->GetData ( 35, PARAMTYPE_Short, &CurrentItemFix->iLightningMin );
					pcDB->GetData ( 36, PARAMTYPE_Short, &CurrentItemFix->iLightningMax );
					pcDB->GetData ( 37, PARAMTYPE_Short, &CurrentItemFix->iPoisonMin );
					pcDB->GetData ( 38, PARAMTYPE_Short, &CurrentItemFix->iPoisonMax );
					pcDB->GetData ( 39, PARAMTYPE_Short, &CurrentItemFix->iATKPowMin );
					pcDB->GetData ( 40, PARAMTYPE_Short, &CurrentItemFix->iATKPowMax );
					pcDB->GetData ( 41, PARAMTYPE_Short, &CurrentItemFix->iRangeMin );
					pcDB->GetData ( 42, PARAMTYPE_Short, &CurrentItemFix->iRangeMax );
					pcDB->GetData ( 43, PARAMTYPE_Short, &CurrentItemFix->iATKRatingMin );
					pcDB->GetData ( 44, PARAMTYPE_Short, &CurrentItemFix->iATKRatingMax );
					pcDB->GetData ( 45, PARAMTYPE_Float, &CurrentItemFix->fCriticalMin );
					pcDB->GetData ( 46, PARAMTYPE_Float, &CurrentItemFix->fCriticalMax );
					pcDB->GetData ( 47, PARAMTYPE_Float, &CurrentItemFix->fBlockMin );
					pcDB->GetData ( 48, PARAMTYPE_Float, &CurrentItemFix->fBlockMax );
					pcDB->GetData ( 49, PARAMTYPE_Float, &CurrentItemFix->fAbsorbMin );
					pcDB->GetData ( 50, PARAMTYPE_Float, &CurrentItemFix->fAbsorbMax );
					pcDB->GetData ( 51, PARAMTYPE_Short, &CurrentItemFix->iDefenseMin );
					pcDB->GetData ( 52, PARAMTYPE_Short, &CurrentItemFix->iDefenseMax );
					pcDB->GetData ( 53, PARAMTYPE_Float, &CurrentItemFix->fRegenerationHPMin );
					pcDB->GetData ( 54, PARAMTYPE_Float, &CurrentItemFix->fRegenerationHPMax );
					pcDB->GetData ( 55, PARAMTYPE_Float, &CurrentItemFix->fRegenerationMPMin );
					pcDB->GetData ( 56, PARAMTYPE_Float, &CurrentItemFix->fRegenerationMPMax );
					pcDB->GetData ( 57, PARAMTYPE_Short, &CurrentItemFix->iAddHPMin );
					pcDB->GetData ( 58, PARAMTYPE_Short, &CurrentItemFix->iAddHPMax );
					pcDB->GetData ( 59, PARAMTYPE_Short, &CurrentItemFix->iAddMPMin );
					pcDB->GetData ( 60, PARAMTYPE_Short, &CurrentItemFix->iAddMPMax );
					pcDB->GetData ( 61, PARAMTYPE_Float, &CurrentItemFix->fRunSpeedMin );
					pcDB->GetData ( 62, PARAMTYPE_Float, &CurrentItemFix->fRunSpeedMax );

					FixesList.push_back ( CurrentItemFix );
				}
			}
		}
		pcDB->Close();
	}
}
#endif

#ifdef ITEMFIXES
void ItemServer::SetItemFixes(DefinitionItem* itemDef, EItemRarity itemRarity, BOOL isPerfect, int iPrefixID, int iSuffixID)
{
	const EItemType itemType = itemDef->sItem.sItemID.ToItemType();

	int iItemLevel = itemDef->sItem.iLevel;
	int iTotalPrefixChance = 0;
	int iTotalSuffixChance = 0;
	std::vector<ItemFix*> CurrentPrefixes;
	std::vector<ItemFix*> CurrentSuffixes;
	ItemFix* ReturnItemPrefix = new ItemFix;
	ItemFix * ReturnItemSuffix = new ItemFix;


	if ( iPrefixID == 0 )
	{
		for ( ItemFix * CurrentFix : FixesList )
		{
			if ( iItemLevel >= CurrentFix->iMinLevel && iItemLevel <= CurrentFix->iMaxLevel && CurrentFix->isPrefix == 1 &&
				itemRarity >= CurrentFix->iMinRarity && itemRarity <= CurrentFix->iMaxRarity )
			{
				if ( itemType == EItemType::ITEMTYPE_Bow)
				{
					if ( CurrentFix->iBows == 1 )
					{
						iTotalPrefixChance += CurrentFix->iChance;
						CurrentPrefixes.push_back ( CurrentFix );
					}
				}
				else if ( itemType == EItemType::ITEMTYPE_Axe)
				{
					if ( CurrentFix->iAxes == 1 )
					{
						iTotalPrefixChance += CurrentFix->iChance;
						CurrentPrefixes.push_back ( CurrentFix );
					}
				}
				else if ( itemType == EItemType::ITEMTYPE_Javelin)
				{
					if ( CurrentFix->iJavelins == 1 )
					{
						iTotalPrefixChance += CurrentFix->iChance;
						CurrentPrefixes.push_back ( CurrentFix );
					}
				}
				else if ( itemType == EItemType::ITEMTYPE_Dagger)
				{
					if ( CurrentFix->iDaggers == 1 )
					{
						iTotalPrefixChance += CurrentFix->iChance;
						CurrentPrefixes.push_back ( CurrentFix );
					}
				}
				else if ( itemType == EItemType::ITEMTYPE_Wand)
				{
					if ( CurrentFix->iWands == 1 )
					{
						iTotalPrefixChance += CurrentFix->iChance;
						CurrentPrefixes.push_back ( CurrentFix );
					}
				}
				else if ( itemType == EItemType::ITEMTYPE_Scythe)
				{
					if ( CurrentFix->iScythes == 1 )
					{
						iTotalPrefixChance += CurrentFix->iChance;
						CurrentPrefixes.push_back ( CurrentFix );
					}
				}
				else if ( itemType == EItemType::ITEMTYPE_Sword)
				{
					if ( CurrentFix->iSwords == 1 )
					{
						iTotalPrefixChance += CurrentFix->iChance;
						CurrentPrefixes.push_back ( CurrentFix );
					}
				}
				else if ( itemType == EItemType::ITEMTYPE_Claw)
				{
					if ( CurrentFix->iClaws == 1 )
					{
						iTotalPrefixChance += CurrentFix->iChance;
						CurrentPrefixes.push_back ( CurrentFix );
					}
				}
				else if ( itemType == EItemType::ITEMTYPE_Hammer)
				{
					if ( CurrentFix->iHammers == 1 )
					{
						iTotalPrefixChance += CurrentFix->iChance;
						CurrentPrefixes.push_back ( CurrentFix );
					}
				}
				else if ( itemType == EItemType::ITEMTYPE_Phantom)
				{
					if ( CurrentFix->iPhantoms == 1 )
					{
						iTotalPrefixChance += CurrentFix->iChance;
						CurrentPrefixes.push_back ( CurrentFix );
					}
				}
				else if ( itemType == EItemType::ITEMTYPE_Armor)
				{
					if ( CurrentFix->iArmors == 1 )
					{
						iTotalPrefixChance += CurrentFix->iChance;
						CurrentPrefixes.push_back ( CurrentFix );
					}
				}
				else if ( itemType == EItemType::ITEMTYPE_Robe)
				{
					if ( CurrentFix->iRobes == 1 )
					{
						iTotalPrefixChance += CurrentFix->iChance;
						CurrentPrefixes.push_back ( CurrentFix );
					}
				}
				else if ( itemType == EItemType::ITEMTYPE_Boots)
				{
					if ( CurrentFix->iBoots == 1 )
					{
						iTotalPrefixChance += CurrentFix->iChance;
						CurrentPrefixes.push_back ( CurrentFix );
					}
				}
				else if ( itemType == EItemType::ITEMTYPE_Gauntlets)
				{
					if ( CurrentFix->iGauntlets == 1 )
					{
						iTotalPrefixChance += CurrentFix->iChance;
						CurrentPrefixes.push_back ( CurrentFix );
					}
				}
				else if ( itemType == EItemType::ITEMTYPE_Bracelets)
				{
					if ( CurrentFix->iBracelets == 1 )
					{
						iTotalPrefixChance += CurrentFix->iChance;
						CurrentPrefixes.push_back ( CurrentFix );
					}
				}
				else if ( itemType == EItemType::ITEMTYPE_Amulet)
				{
					if ( CurrentFix->iAmulets == 1 )
					{
						iTotalPrefixChance += CurrentFix->iChance;
						CurrentPrefixes.push_back ( CurrentFix );
					}
				}
				else if ( itemType == EItemType::ITEMTYPE_Ring)
				{
					if ( CurrentFix->iRings == 1 )
					{
						iTotalPrefixChance += CurrentFix->iChance;
						CurrentPrefixes.push_back ( CurrentFix );
					}
				}
				else if ( itemType == EItemType::ITEMTYPE_Shield)
				{
					if ( CurrentFix->iShields == 1 )
					{
						iTotalPrefixChance += CurrentFix->iChance;
						CurrentPrefixes.push_back ( CurrentFix );
					}
				}
				else if ( itemType == EItemType::ITEMTYPE_Orb)
				{
					if ( CurrentFix->iOrbs == 1 )
					{
						iTotalPrefixChance += CurrentFix->iChance;
						CurrentPrefixes.push_back ( CurrentFix );
					}
				}
			}
		}

		//Get a Prefix based on chance
		if ( iTotalPrefixChance > 0 )
		{
			int iChance = 0;

			const int rarirtyChance = Dice::RandomI(0, iTotalPrefixChance);

			for ( ItemFix * PossiblePrefix : CurrentPrefixes )
			{
				iChance += PossiblePrefix->iChance;

				if ( iChance >= rarirtyChance )
				{
					ReturnItemPrefix = PossiblePrefix;
					break;
				}
			}

		}
	}
	else
	{
		for ( ItemFix * CurrentFix : FixesList )
		{
			if ( CurrentFix->iID == iPrefixID )
			{
				ReturnItemPrefix = CurrentFix;
			}
		}
	}

	if ( iSuffixID == 0 )
	{
		for ( ItemFix * CurrentFix : FixesList )
		{
			if ( iItemLevel >= CurrentFix->iMinLevel && iItemLevel <= CurrentFix->iMaxLevel && CurrentFix->isSuffix == 1 &&
				itemRarity >= CurrentFix->iMinRarity && itemRarity <= CurrentFix->iMaxRarity )
			{
				if ( itemType == EItemType::ITEMTYPE_Bow)
				{
					if ( CurrentFix->iBows == 1 )
					{
						iTotalSuffixChance += CurrentFix->iChance;
						CurrentSuffixes.push_back ( CurrentFix );
					}
				}
				else if ( itemType == EItemType::ITEMTYPE_Axe)
				{
					if ( CurrentFix->iAxes == 1 )
					{
						iTotalSuffixChance += CurrentFix->iChance;
						CurrentSuffixes.push_back ( CurrentFix );
					}
				}
				else if ( itemType == EItemType::ITEMTYPE_Javelin)
				{
					if ( CurrentFix->iJavelins == 1 )
					{
						iTotalSuffixChance += CurrentFix->iChance;
						CurrentSuffixes.push_back ( CurrentFix );
					}
				}
				else if ( itemType == EItemType::ITEMTYPE_Dagger)
				{
					if ( CurrentFix->iDaggers == 1 )
					{
						iTotalSuffixChance += CurrentFix->iChance;
						CurrentSuffixes.push_back ( CurrentFix );
					}
				}
				else if ( itemType == EItemType::ITEMTYPE_Wand)
				{
					if ( CurrentFix->iWands == 1 )
					{
						iTotalSuffixChance += CurrentFix->iChance;
						CurrentSuffixes.push_back ( CurrentFix );
					}
				}
				else if ( itemType == EItemType::ITEMTYPE_Scythe)
				{
					if ( CurrentFix->iScythes == 1 )
					{
						iTotalSuffixChance += CurrentFix->iChance;
						CurrentSuffixes.push_back ( CurrentFix );
					}
				}
				else if ( itemType == EItemType::ITEMTYPE_Sword)
				{
					if ( CurrentFix->iSwords == 1 )
					{
						iTotalSuffixChance += CurrentFix->iChance;
						CurrentSuffixes.push_back ( CurrentFix );
					}
				}
				else if ( itemType == EItemType::ITEMTYPE_Claw)
				{
					if ( CurrentFix->iClaws == 1 )
					{
						iTotalSuffixChance += CurrentFix->iChance;
						CurrentSuffixes.push_back ( CurrentFix );
					}
				}
				else if ( itemType == EItemType::ITEMTYPE_Hammer)
				{
					if ( CurrentFix->iHammers == 1 )
					{
						iTotalSuffixChance += CurrentFix->iChance;
						CurrentSuffixes.push_back ( CurrentFix );
					}
				}
				else if ( itemType == EItemType::ITEMTYPE_Phantom)
				{
					if ( CurrentFix->iPhantoms == 1 )
					{
						iTotalSuffixChance += CurrentFix->iChance;
						CurrentSuffixes.push_back ( CurrentFix );
					}
				}
				else if ( itemType == EItemType::ITEMTYPE_Armor)
				{
					if ( CurrentFix->iArmors == 1 )
					{
						iTotalSuffixChance += CurrentFix->iChance;
						CurrentSuffixes.push_back ( CurrentFix );
					}
				}
				else if ( itemType == EItemType::ITEMTYPE_Robe)
				{
					if ( CurrentFix->iRobes == 1 )
					{
						iTotalSuffixChance += CurrentFix->iChance;
						CurrentSuffixes.push_back ( CurrentFix );
					}
				}
				else if ( itemType == EItemType::ITEMTYPE_Boots)
				{
					if ( CurrentFix->iBoots == 1 )
					{
						iTotalSuffixChance += CurrentFix->iChance;
						CurrentSuffixes.push_back ( CurrentFix );
					}
				}
				else if ( itemType == EItemType::ITEMTYPE_Gauntlets)
				{
					if ( CurrentFix->iGauntlets == 1 )
					{
						iTotalSuffixChance += CurrentFix->iChance;
						CurrentSuffixes.push_back ( CurrentFix );
					}
				}
				else if ( itemType == EItemType::ITEMTYPE_Bracelets)
				{
					if ( CurrentFix->iBracelets == 1 )
					{
						iTotalSuffixChance += CurrentFix->iChance;
						CurrentSuffixes.push_back ( CurrentFix );
					}
				}
				else if ( itemType == EItemType::ITEMTYPE_Amulet)
				{
					if ( CurrentFix->iAmulets == 1 )
					{
						iTotalSuffixChance += CurrentFix->iChance;
						CurrentSuffixes.push_back ( CurrentFix );
					}
				}
				else if ( itemType == EItemType::ITEMTYPE_Ring)
				{
					if ( CurrentFix->iRings == 1 )
					{
						iTotalSuffixChance += CurrentFix->iChance;
						CurrentSuffixes.push_back ( CurrentFix );
					}
				}
				else if ( itemType == EItemType::ITEMTYPE_Shield)
				{
					if ( CurrentFix->iShields == 1 )
					{
						iTotalSuffixChance += CurrentFix->iChance;
						CurrentSuffixes.push_back ( CurrentFix );
					}
				}
				else if ( itemType == EItemType::ITEMTYPE_Orb)
				{
					if ( CurrentFix->iOrbs == 1 )
					{
						iTotalSuffixChance += CurrentFix->iChance;
						CurrentSuffixes.push_back ( CurrentFix );
					}
				}
			}
		}

		//Get a Suffix based on chance
		if ( iTotalSuffixChance > 0 )
		{
			int iChance = 0;

			const int rarirtyChance = Dice::RandomI(0, iTotalSuffixChance);

			for ( ItemFix * PossibleSuffix : CurrentSuffixes )
			{
				iChance += PossibleSuffix->iChance;

				if ( iChance >= rarirtyChance )
				{
					ReturnItemSuffix = PossibleSuffix;
					break;
				}
			}
		}
	}
	else
	{
		for ( ItemFix * CurrentFix : FixesList )
		{
			if ( CurrentFix->iID == iSuffixID )
			{
				ReturnItemSuffix = CurrentFix;
			}
		}
	}


	//Apply Stat Changes
	if ( ReturnItemPrefix->iID != 0 && !(isPerfect) )
	{
		itemDef->sItem.sPrefixData.iID = ReturnItemPrefix->iID;
		strcpy(itemDef->sItem.sPrefixData.sName, ReturnItemPrefix->sName);

		itemDef->sItem.sPrefixData.iATKPow = GetRandomStatValue ( ReturnItemPrefix->iATKPowMin, ReturnItemPrefix->iATKPowMax );
		itemDef->sItem.sPrefixData.iATKRating = GetRandomStatValue ( ReturnItemPrefix->iATKRatingMin, ReturnItemPrefix->iATKRatingMax );
		itemDef->sItem.sPrefixData.iRange = GetRandomStatValue ( ReturnItemPrefix->iRangeMin, ReturnItemPrefix->iRangeMax );
		itemDef->sItem.sPrefixData.fBlock = GetRandomStatValue ( ReturnItemPrefix->fBlockMin, ReturnItemPrefix->fBlockMax );
		itemDef->sItem.sPrefixData.fAbsorb = GetRandomStatValue ( ReturnItemPrefix->fAbsorbMin, ReturnItemPrefix->fAbsorbMax );
		itemDef->sItem.sPrefixData.iDefense = GetRandomStatValue ( ReturnItemPrefix->iDefenseMin, ReturnItemPrefix->iDefenseMax );
		itemDef->sItem.sPrefixData.fRegenerationHP = GetRandomStatValue ( ReturnItemPrefix->fRegenerationHPMin, ReturnItemPrefix->fRegenerationHPMax );
		itemDef->sItem.sPrefixData.fRegenerationMP = GetRandomStatValue ( ReturnItemPrefix->fRegenerationMPMin, ReturnItemPrefix->fRegenerationMPMax );
		itemDef->sItem.sPrefixData.iAddHP = GetRandomStatValue ( ReturnItemPrefix->iAddHPMin, ReturnItemPrefix->iAddHPMax );
		itemDef->sItem.sPrefixData.iAddMP = GetRandomStatValue ( ReturnItemPrefix->iAddMPMin, ReturnItemPrefix->iAddMPMax );
		itemDef->sItem.sPrefixData.fRunSpeed = GetRandomStatValue ( ReturnItemPrefix->fRunSpeedMin, ReturnItemPrefix->fRunSpeedMax );
		itemDef->sItem.sPrefixData.fCritical = GetRandomStatValue ( ReturnItemPrefix->fCriticalMin, ReturnItemPrefix->fCriticalMax );
		itemDef->sItem.sPrefixData.iOrganic = GetRandomStatValue ( ReturnItemPrefix->iOrganicMin, ReturnItemPrefix->iOrganicMax );
		itemDef->sItem.sPrefixData.iFire = GetRandomStatValue ( ReturnItemPrefix->iFireMin, ReturnItemPrefix->iFireMax );
		itemDef->sItem.sPrefixData.iFrost = GetRandomStatValue ( ReturnItemPrefix->iFrostMin, ReturnItemPrefix->iFrostMax );
		itemDef->sItem.sPrefixData.iLightning = GetRandomStatValue ( ReturnItemPrefix->iLightningMin, ReturnItemPrefix->iLightningMax );
		itemDef->sItem.sPrefixData.iPoison = GetRandomStatValue ( ReturnItemPrefix->iPoisonMin, ReturnItemPrefix->iPoisonMax );

	}

	if ( ReturnItemPrefix->iID != 0 && isPerfect )
	{
		itemDef->sItem.sPrefixData.iID = ReturnItemPrefix->iID;
		strcpy(itemDef->sItem.sPrefixData.sName, ReturnItemPrefix->sName);

		itemDef->sItem.sPrefixData.iATKPow = ReturnItemPrefix->iATKPowMax ;
		itemDef->sItem.sPrefixData.iATKRating = ReturnItemPrefix->iATKRatingMax ;
		itemDef->sItem.sPrefixData.iRange = ReturnItemPrefix->iRangeMax ;
		itemDef->sItem.sPrefixData.fBlock =  ReturnItemPrefix->fBlockMax ;
		itemDef->sItem.sPrefixData.fAbsorb = ReturnItemPrefix->fAbsorbMax ;
		itemDef->sItem.sPrefixData.iDefense =  ReturnItemPrefix->iDefenseMax ;
		itemDef->sItem.sPrefixData.fRegenerationHP =  ReturnItemPrefix->fRegenerationHPMax ;
		itemDef->sItem.sPrefixData.fRegenerationMP =  ReturnItemPrefix->fRegenerationMPMax ;
		itemDef->sItem.sPrefixData.iAddHP =  ReturnItemPrefix->iAddHPMax ;
		itemDef->sItem.sPrefixData.iAddMP =  ReturnItemPrefix->iAddMPMax ;
		itemDef->sItem.sPrefixData.fRunSpeed = ReturnItemPrefix->fRunSpeedMax ;
		itemDef->sItem.sPrefixData.fCritical = ReturnItemPrefix->fCriticalMax ;
		itemDef->sItem.sPrefixData.iOrganic =  ReturnItemPrefix->iOrganicMax ;
		itemDef->sItem.sPrefixData.iFire =  ReturnItemPrefix->iFireMax ;
		itemDef->sItem.sPrefixData.iFrost =  ReturnItemPrefix->iFrostMax ;
		itemDef->sItem.sPrefixData.iLightning = ReturnItemPrefix->iLightningMax ;
		itemDef->sItem.sPrefixData.iPoison =  ReturnItemPrefix->iPoisonMax ;
	}

	if ( ReturnItemSuffix->iID != 0 && !(isPerfect) )
	{
		itemDef->sItem.sSuffixData.iID = ReturnItemSuffix->iID;
		strcpy(itemDef->sItem.sSuffixData.sName, ReturnItemSuffix->sName);

		itemDef->sItem.sSuffixData.iATKPow = GetRandomStatValue ( ReturnItemSuffix->iATKPowMin, ReturnItemSuffix->iATKPowMax );
		itemDef->sItem.sSuffixData.iATKRating = GetRandomStatValue ( ReturnItemSuffix->iATKRatingMin, ReturnItemSuffix->iATKRatingMax );
		itemDef->sItem.sSuffixData.iRange = GetRandomStatValue ( ReturnItemSuffix->iRangeMin, ReturnItemSuffix->iRangeMax );
		itemDef->sItem.sSuffixData.fBlock = GetRandomStatValue ( ReturnItemSuffix->fBlockMin, ReturnItemSuffix->fBlockMax );
		itemDef->sItem.sSuffixData.fAbsorb = GetRandomStatValue ( ReturnItemSuffix->fAbsorbMin, ReturnItemSuffix->fAbsorbMax );
		itemDef->sItem.sSuffixData.iDefense = GetRandomStatValue ( ReturnItemSuffix->iDefenseMin, ReturnItemSuffix->iDefenseMax );
		itemDef->sItem.sSuffixData.fRegenerationHP = GetRandomStatValue ( ReturnItemSuffix->fRegenerationHPMin, ReturnItemSuffix->fRegenerationHPMax );
		itemDef->sItem.sSuffixData.fRegenerationMP = GetRandomStatValue ( ReturnItemSuffix->fRegenerationMPMin, ReturnItemSuffix->fRegenerationMPMax );
		itemDef->sItem.sSuffixData.iAddHP = GetRandomStatValue ( ReturnItemSuffix->iAddHPMin, ReturnItemSuffix->iAddHPMax );
		itemDef->sItem.sSuffixData.iAddMP = GetRandomStatValue ( ReturnItemSuffix->iAddMPMin, ReturnItemSuffix->iAddMPMax );
		itemDef->sItem.sSuffixData.fRunSpeed = GetRandomStatValue ( ReturnItemSuffix->fRunSpeedMin, ReturnItemSuffix->fRunSpeedMax );
		itemDef->sItem.sSuffixData.fCritical = GetRandomStatValue ( ReturnItemSuffix->fCriticalMin, ReturnItemSuffix->fCriticalMax );
		itemDef->sItem.sSuffixData.iOrganic = GetRandomStatValue ( ReturnItemSuffix->iOrganicMin, ReturnItemSuffix->iOrganicMax );
		itemDef->sItem.sSuffixData.iFire = GetRandomStatValue ( ReturnItemSuffix->iFireMin, ReturnItemSuffix->iFireMax );
		itemDef->sItem.sSuffixData.iFrost = GetRandomStatValue ( ReturnItemSuffix->iFrostMin, ReturnItemSuffix->iFrostMax );
		itemDef->sItem.sSuffixData.iLightning = GetRandomStatValue ( ReturnItemSuffix->iLightningMin, ReturnItemSuffix->iLightningMax );
		itemDef->sItem.sSuffixData.iPoison = GetRandomStatValue ( ReturnItemSuffix->iPoisonMin, ReturnItemSuffix->iPoisonMax );
	}

	if ( ReturnItemSuffix->iID != 0 && isPerfect )
	{
		itemDef->sItem.sSuffixData.iID = ReturnItemSuffix->iID;
		strcpy(itemDef->sItem.sSuffixData.sName, ReturnItemSuffix->sName);

		itemDef->sItem.sSuffixData.iATKPow = ReturnItemSuffix->iATKPowMax ;
		itemDef->sItem.sSuffixData.iATKRating = ReturnItemSuffix->iATKRatingMax ;
		itemDef->sItem.sSuffixData.iRange = ReturnItemSuffix->iRangeMax ;
		itemDef->sItem.sSuffixData.fBlock =  ReturnItemSuffix->fBlockMax ;
		itemDef->sItem.sSuffixData.fAbsorb = ReturnItemSuffix->fAbsorbMax ;
		itemDef->sItem.sSuffixData.iDefense =  ReturnItemSuffix->iDefenseMax ;
		itemDef->sItem.sSuffixData.fRegenerationHP =  ReturnItemSuffix->fRegenerationHPMax ;
		itemDef->sItem.sSuffixData.fRegenerationMP =  ReturnItemSuffix->fRegenerationMPMax ;
		itemDef->sItem.sSuffixData.iAddHP =  ReturnItemSuffix->iAddHPMax ;
		itemDef->sItem.sSuffixData.iAddMP =  ReturnItemSuffix->iAddMPMax ;
		itemDef->sItem.sSuffixData.fRunSpeed = ReturnItemSuffix->fRunSpeedMax ;
		itemDef->sItem.sSuffixData.fCritical = ReturnItemSuffix->fCriticalMax ;
		itemDef->sItem.sSuffixData.iOrganic =  ReturnItemSuffix->iOrganicMax ;
		itemDef->sItem.sSuffixData.iFire =  ReturnItemSuffix->iFireMax ;
		itemDef->sItem.sSuffixData.iFrost =  ReturnItemSuffix->iFrostMax ;
		itemDef->sItem.sSuffixData.iLightning = ReturnItemSuffix->iLightningMax ;
		itemDef->sItem.sSuffixData.iPoison =  ReturnItemSuffix->iPoisonMax ;
	}

	if ( ReturnItemPrefix->iID == 0 )
	{
		itemDef->sItem.sPrefixData.iID = 0;
		strcpy(itemDef->sItem.sPrefixData.sName, "");

		itemDef->sItem.sPrefixData.iATKPow = 0 ;
		itemDef->sItem.sPrefixData.iATKRating = 0 ;
		itemDef->sItem.sPrefixData.iRange = 0 ;
		itemDef->sItem.sPrefixData.fBlock =  0 ;
		itemDef->sItem.sPrefixData.fAbsorb = 0 ;
		itemDef->sItem.sPrefixData.iDefense =  0 ;
		itemDef->sItem.sPrefixData.fRegenerationHP =  0 ;
		itemDef->sItem.sPrefixData.fRegenerationMP =  0 ;
		itemDef->sItem.sPrefixData.iAddHP =  0;
		itemDef->sItem.sPrefixData.iAddMP =  0;
		itemDef->sItem.sPrefixData.fRunSpeed = 0;
		itemDef->sItem.sPrefixData.fCritical = 0;
		itemDef->sItem.sPrefixData.iOrganic =  0 ;
		itemDef->sItem.sPrefixData.iFire =  0;
		itemDef->sItem.sPrefixData.iFrost =  0 ;
		itemDef->sItem.sPrefixData.iLightning = 0 ;
		itemDef->sItem.sPrefixData.iPoison =  0 ;
	}

	if ( ReturnItemSuffix->iID == 0 )
	{
		itemDef->sItem.sSuffixData.iID = 0;
		strcpy(itemDef->sItem.sSuffixData.sName, "");

		itemDef->sItem.sSuffixData.iATKPow = 0 ;
		itemDef->sItem.sSuffixData.iATKRating = 0 ;
		itemDef->sItem.sSuffixData.iRange = 0 ;
		itemDef->sItem.sSuffixData.fBlock =  0 ;
		itemDef->sItem.sSuffixData.fAbsorb = 0 ;
		itemDef->sItem.sSuffixData.iDefense =  0 ;
		itemDef->sItem.sSuffixData.fRegenerationHP =  0 ;
		itemDef->sItem.sSuffixData.fRegenerationMP =  0 ;
		itemDef->sItem.sSuffixData.iAddHP =  0;
		itemDef->sItem.sSuffixData.iAddMP =  0;
		itemDef->sItem.sSuffixData.fRunSpeed = 0;
		itemDef->sItem.sSuffixData.fCritical = 0;
		itemDef->sItem.sSuffixData.iOrganic =  0 ;
		itemDef->sItem.sSuffixData.iFire =  0;
		itemDef->sItem.sSuffixData.iFrost =  0 ;
		itemDef->sItem.sSuffixData.iLightning = 0 ;
		itemDef->sItem.sSuffixData.iPoison =  0 ;
	}

	//SAFE_DELETE(ReturnItemPrefix);
	//SAFE_DELETE(ReturnItemSuffix);
}
#endif
short ItemServer::GetRandomStatValue (short iMin, short iMax)
{
	const int RandomValue = Dice::RandomI(iMin, iMax);
	return RandomValue;
}

float ItemServer::GetRandomStatValue (float fMin, float fMax)
{
	if ( fMin > 0 )
		int test = 1;
	int iMin = static_cast<int>( floor( fMin * 10 ) );
	int iMax = static_cast<int>( floor( fMax * 10 ) );

	const int RandomValue = Dice::RandomI(iMin, iMax);
	float ReturnValue = (float)RandomValue / 10.0f ;

	return ReturnValue;
}

void ItemServer::SetRarityBoostBasedOnItemBase(DefinitionItem* itemDef, const EItemRarity& itemRarity )
{
	if (itemRarity == EItemRarity::NONE)
	{
		return;
	}

	const EItemBase itemBase = itemDef->sItem.sItemID.ToItemBase();
	const EItemType itemType = itemDef->sItem.sItemID.ToItemType();
	int statsBoostMultiplier = itemRarity - 1;
	const int itemLevel = itemDef->sItem.iLevel;

	itemDef->sItem.iSalePrice = ( itemDef->sItem.iSalePrice / 2 ) * ( statsBoostMultiplier + 1 );

	int attackPowerBoost = 0;
	int rangeBoost = 0;
	int attackRatingBoost = 0;
	int criticalBoost = 0;

	if (itemLevel >= 0  && itemLevel < 103)
	{
		attackPowerBoost = 3;
		rangeBoost = 3;
		attackRatingBoost = 10;
		criticalBoost = 1;
	}
	else if (itemLevel >= 103 && itemLevel < 113)
	{
		attackPowerBoost = 4;
		rangeBoost = 4;
		attackRatingBoost = 13;
		criticalBoost = 1;
	}
	else if (itemLevel >= 113)
	{
		attackPowerBoost = 5;
		rangeBoost = 5;
		attackRatingBoost = 20;
		criticalBoost = 1;
	}

	if (itemBase == EItemBase::ITEMBASE_Weapon)
	{
		itemDef->AttackPower1Min += attackPowerBoost * statsBoostMultiplier;
		itemDef->AttackPower1Max += attackPowerBoost * statsBoostMultiplier;
		itemDef->AttackPower2Min += attackPowerBoost * statsBoostMultiplier;
		itemDef->AttackPower2Max += attackPowerBoost * statsBoostMultiplier;

		itemDef->sItem.iAttackRange += rangeBoost * statsBoostMultiplier;

		itemDef->AttackRatingMin += attackRatingBoost * statsBoostMultiplier;
		itemDef->AttackRatingMax += attackRatingBoost * statsBoostMultiplier;

		itemDef->sItem.iCritical += criticalBoost * statsBoostMultiplier;
	}

	switch (itemType)
	{
		case EItemType::ITEMTYPE_Armor:
			this->SetRarityBoostArmors(itemDef, statsBoostMultiplier, itemLevel);
			break;
		case EItemType::ITEMTYPE_Boots:
			this->SetRarityBoostBoots(itemDef, statsBoostMultiplier, itemLevel);
			break;
		case EItemType::ITEMTYPE_Robe:
			this->SetRarityBoostRobes(itemDef, statsBoostMultiplier, itemLevel);
			break;
		case EItemType::ITEMTYPE_Gauntlets:
			this->SetRarityBoostGauntlets(itemDef, statsBoostMultiplier, itemLevel);
			break;
		case EItemType::ITEMTYPE_Bracelets:
			this->SetRarityBoostBracelets(itemDef, statsBoostMultiplier, itemLevel);
			break;
		case EItemType::ITEMTYPE_Amulet:
			this->SetRarityBoostAmulets(itemDef, statsBoostMultiplier, itemLevel);
			break;
		case EItemType::ITEMTYPE_Ring:
			this->SetRarityBoostRings(itemDef, statsBoostMultiplier, itemLevel);
			break;
		case EItemType::ITEMTYPE_Shield:
			this->SetRarityBoostShields(itemDef, statsBoostMultiplier, itemLevel);
			break;
		case EItemType::ITEMTYPE_Orb:
			this->SetRarityBoostOrbs(itemDef, statsBoostMultiplier, itemLevel);
			break;
	}
}

#define RARITY_TOTAL_CHANCE 10000000

EItemRarity ItemServer::ChooseItemRarity(int iLevel, ERarityChanceModType eModType)
{
	int iReturnRarityGroup = 0;
	for ( ItemRarityChanceGroup * CurrentGroup : RarityChanceGroupList )
	{
		if ( ( iLevel >= CurrentGroup->MinLevel ) && ( iLevel <= CurrentGroup->MaxLevel ) )
		{
			iReturnRarityGroup = CurrentGroup->iRarityGroup;
		}
	}

	int CommonChance = 0;
	int UncommonChance = 0;
	int RareChance = 0;
	int EpicChance = 0;
	int LegendaryChance = 0;

	for ( ItemRarity * CurrentRarity : RarityList )
	{
		if ( CurrentRarity->iRarityGroup == iReturnRarityGroup )
		{
			if ( CurrentRarity->eRarity == EItemRarity::UNCOMMON )
				UncommonChance = CurrentRarity->Chance;
			else if ( CurrentRarity->eRarity == EItemRarity::RARE )
				RareChance = CurrentRarity->Chance;
			else if ( CurrentRarity->eRarity == EItemRarity::EPIC )
				EpicChance = CurrentRarity->Chance;
			else if ( CurrentRarity->eRarity == EItemRarity::LEGENDARY )
				LegendaryChance = CurrentRarity->Chance;
		}
	}

	CommonChance = RARITY_TOTAL_CHANCE - LegendaryChance - EpicChance - RareChance - UncommonChance; //remainder

	if ( eModType != ERarityChanceModType::None )
	{
		auto it = mRarityChanceMod.find( eModType );
		if ( it != mRarityChanceMod.end() )
		{
			CommonChance		= static_cast<int>( round( it->second.fModCommon * CommonChance ) );
			UncommonChance		= static_cast<int>( round( it->second.fModUncommon * UncommonChance ) );
			RareChance			= static_cast<int>( round( it->second.fModRare * RareChance ) );
			EpicChance			= static_cast<int>( round( it->second.fModEpic * EpicChance ) );
			LegendaryChance		= static_cast<int>( round( it->second.fModLegendary * LegendaryChance ) );
		}
	}

	UncommonChance	+= CommonChance;
	RareChance		+= UncommonChance;
	EpicChance		+= RareChance;
	LegendaryChance += EpicChance;

	const int chance = Dice::RandomI(1, LegendaryChance); //note '1' is correct, the range is inclusive

	if ( chance <= CommonChance )
	{
		return EItemRarity::COMMON;
	}
	else if ( chance <= UncommonChance )
	{
		return EItemRarity::UNCOMMON;
	}
	else if ( chance <= RareChance )
	{
		return EItemRarity::RARE;
	}
	else if ( chance <= EpicChance )
	{
		return EItemRarity::EPIC;
	}
	else if ( chance <= LegendaryChance )
	{
		return EItemRarity::LEGENDARY;
	}

	WARN( "ChooseItemRarity Logic error" );

	return EItemRarity::COMMON;
}

void ItemServer::SetRarityBoostArmors(DefinitionItem* itemDef, const int statsBoostMultiplier, const int itemLevel)
{
	if ((itemDef->sItem.iStrength + (-3 * statsBoostMultiplier)) >= 0)
	{
		itemDef->sItem.iStrength += (-3 * statsBoostMultiplier);
	}


	if (itemLevel >= 0 && itemLevel < 103)
	{
		itemDef->AbsorbMin += static_cast<float>(1.4 * statsBoostMultiplier);
		itemDef->AbsorbMax += static_cast<float>(1.4 * statsBoostMultiplier);

		itemDef->DefenseMin += (15 * statsBoostMultiplier);
		itemDef->DefenseMax += (15 * statsBoostMultiplier);
	}
	else if (itemLevel >= 103 && itemLevel <= 113)
	{
		itemDef->AbsorbMin += static_cast<float>(4 * statsBoostMultiplier);
		itemDef->AbsorbMax += static_cast<float>(4 * statsBoostMultiplier);

		itemDef->DefenseMin += (25 * statsBoostMultiplier);
		itemDef->DefenseMax += (25 * statsBoostMultiplier);
	}
}

void ItemServer::SetRarityBoostBoots(DefinitionItem* itemDef, const int statsBoostMultiplier, const int itemLevel)
{
	if ((itemDef->sItem.iAgility + (-3 * statsBoostMultiplier)) >= 0)
	{
		itemDef->sItem.iAgility += (-3 * statsBoostMultiplier);
	}

	if (itemLevel >= 0 && itemLevel < 103)
	{
		itemDef->AbsorbMin += static_cast<float>(0.7 * statsBoostMultiplier);
		itemDef->AbsorbMax += static_cast<float>(0.7 * statsBoostMultiplier);

		itemDef->DefenseMin += (7 * statsBoostMultiplier);
		itemDef->DefenseMax += (7 * statsBoostMultiplier);
	}
	else if (itemLevel >= 103 && itemLevel <= 113)
	{
		itemDef->AbsorbMin += static_cast<float>(1.6 * statsBoostMultiplier);
		itemDef->AbsorbMax += static_cast<float>(1.6 * statsBoostMultiplier);

		itemDef->DefenseMin += (20 * statsBoostMultiplier);
		itemDef->DefenseMax += (20 * statsBoostMultiplier);
	}
}

void ItemServer::SetRarityBoostRobes(DefinitionItem* itemDef, const int statsBoostMultiplier, const int itemLevel)
{

	if (itemLevel >= 0 && itemLevel < 103)
	{
		itemDef->AbsorbMin += static_cast<float>(1.4 * statsBoostMultiplier);
		itemDef->AbsorbMax += static_cast<float>(1.4 * statsBoostMultiplier);

		itemDef->DefenseMin += (15 * statsBoostMultiplier);
		itemDef->DefenseMax += (15 * statsBoostMultiplier);
	}
	else if (itemLevel >= 103 && itemLevel <= 113)
	{
		itemDef->AbsorbMin += static_cast<float>(4 * statsBoostMultiplier);
		itemDef->AbsorbMax += static_cast<float>(4 * statsBoostMultiplier);

		itemDef->DefenseMin += (25 * statsBoostMultiplier);
		itemDef->DefenseMax += (25 * statsBoostMultiplier);
	}
}

void ItemServer::SetRarityBoostGauntlets(DefinitionItem* itemDef, const int statsBoostMultiplier, const int itemLevel)
{

	if (itemLevel >= 0 && itemLevel  < 103)
	{
		itemDef->AbsorbMin += static_cast<float>(0.7 * statsBoostMultiplier);
		itemDef->AbsorbMax += static_cast<float>(0.7 * statsBoostMultiplier);

		itemDef->DefenseMin += (7 * statsBoostMultiplier);
		itemDef->DefenseMax += (7 * statsBoostMultiplier);
	}
	else if (itemLevel >= 103 && itemLevel <= 113)
	{
		itemDef->AbsorbMin += static_cast<float>(1.6 * statsBoostMultiplier);
		itemDef->AbsorbMax += static_cast<float>(1.6 * statsBoostMultiplier);

		itemDef->DefenseMin += (20 * statsBoostMultiplier);
		itemDef->DefenseMax += (20 * statsBoostMultiplier);
	}
}

void ItemServer::SetRarityBoostBracelets(DefinitionItem* itemDef, const int statsBoostMultiplier, const int itemLevel)
{

	if (itemLevel >= 0 && itemLevel  < 103)
	{
		itemDef->AttackRatingMin += (5 * statsBoostMultiplier);
		itemDef->AttackRatingMax += (5 * statsBoostMultiplier);

		itemDef->DefenseMin += (6 * statsBoostMultiplier);
		itemDef->DefenseMax += (6 * statsBoostMultiplier);

		itemDef->sItem.iPotionStorage += (3 * statsBoostMultiplier);
	}
	else if (itemLevel >= 103 && itemLevel <= 113)
	{
		itemDef->AttackRatingMin += (12 * statsBoostMultiplier);
		itemDef->AttackRatingMax += (12 * statsBoostMultiplier);

		itemDef->DefenseMin += (14 * statsBoostMultiplier);
		itemDef->DefenseMax += (14 * statsBoostMultiplier);

		itemDef->sItem.iPotionStorage += (4 * statsBoostMultiplier);
	}
}

void ItemServer::SetRarityBoostAmulets(DefinitionItem* itemDef, const int statsBoostMultiplier, const int itemLevel)
{
	if ((itemDef->sItem.iSpirit + (-3 * statsBoostMultiplier)) >= 0)
	{
		itemDef->sItem.iSpirit += (-3 * statsBoostMultiplier);
	}

	if (itemLevel >= 0 && itemLevel < 103)
	{
		itemDef->AddHPMin += (2 * statsBoostMultiplier);
		itemDef->AddHPMax += (2 * statsBoostMultiplier);

		itemDef->AddMPMin += (2 * statsBoostMultiplier);
		itemDef->AddMPMax += (2 * statsBoostMultiplier);
	}
	else if (itemLevel >= 103 && itemLevel <= 113)
	{
		itemDef->AddHPMin += (3 * statsBoostMultiplier);
		itemDef->AddHPMax += (3 * statsBoostMultiplier);

		itemDef->AddMPMin += (4 * statsBoostMultiplier);
		itemDef->AddMPMax += (4 * statsBoostMultiplier);
	}
}

void ItemServer::SetRarityBoostRings(DefinitionItem* itemDef, const int statsBoostMultiplier, const int itemLevel)
{
	if ((itemDef->sItem.iSpirit + (-3 * statsBoostMultiplier)) >= 0)
	{
		itemDef->sItem.iSpirit += (-3 * statsBoostMultiplier);
	}

	if (itemLevel >= 0 && itemLevel < 103)
	{
		itemDef->AddHPMin += (1 * statsBoostMultiplier);
		itemDef->AddHPMax += (1 * statsBoostMultiplier);

		itemDef->AddSTMMin += (2 * statsBoostMultiplier);
		itemDef->AddSTMMax += (2 * statsBoostMultiplier);
	}
	else if (itemLevel >= 103 && itemLevel <= 113)
	{
		itemDef->AddHPMin += (3 * statsBoostMultiplier);
		itemDef->AddHPMax += (3 * statsBoostMultiplier);

		itemDef->AddSTMMin += (4 * statsBoostMultiplier);
		itemDef->AddSTMMax += (4 * statsBoostMultiplier);
	}
}

void ItemServer::SetRarityBoostShields(DefinitionItem* itemDef, const int statsBoostMultiplier, const int itemLevel)
{
	if ((itemDef->sItem.iStrength + (-3 * statsBoostMultiplier)) >= 0)
	{
		itemDef->sItem.iStrength += (-3 * statsBoostMultiplier);
	}

	if (itemLevel >= 0 && itemLevel < 103)
	{
		itemDef->AbsorbMin += static_cast<float>(0.5 * statsBoostMultiplier);
		itemDef->AbsorbMax += static_cast<float>(0.5 * statsBoostMultiplier);

		itemDef->DefenseMin += (7 * statsBoostMultiplier);
		itemDef->DefenseMax += (7 * statsBoostMultiplier);

		itemDef->BlockRatingMin += static_cast<float>(.3 * statsBoostMultiplier);
		itemDef->BlockRatingMax += static_cast<float>(.3 * statsBoostMultiplier);
	}
	else if (itemLevel >= 103 && itemLevel <= 110)
	{
		itemDef->AbsorbMin += static_cast<float>(.7 * statsBoostMultiplier);
		itemDef->AbsorbMax += static_cast<float>(.7 * statsBoostMultiplier);

		itemDef->DefenseMin += (13 * statsBoostMultiplier);
		itemDef->DefenseMax += (13 * statsBoostMultiplier);

		itemDef->BlockRatingMin += static_cast<float>(.3 * statsBoostMultiplier);
		itemDef->BlockRatingMax += static_cast<float>(.3 * statsBoostMultiplier);
	}
	else if (itemLevel >= 110 && itemLevel <= 113)
	{
		itemDef->AbsorbMin += static_cast<float>(1 * statsBoostMultiplier);
		itemDef->AbsorbMax += static_cast<float>(1 * statsBoostMultiplier);

		itemDef->DefenseMin += (20 * statsBoostMultiplier);
		itemDef->DefenseMax += (20 * statsBoostMultiplier);

		itemDef->BlockRatingMin += static_cast<float>(.5 * statsBoostMultiplier);
		itemDef->BlockRatingMax += static_cast<float>(.5 * statsBoostMultiplier);
	}
}

void ItemServer::SetRarityBoostOrbs(DefinitionItem* itemDef, const int statsBoostMultiplier, const int itemLevel)
{


	if (itemLevel >= 0 && itemLevel < 103)
	{
		itemDef->BlockRatingMin += static_cast<float>(0.3 * statsBoostMultiplier);
		itemDef->BlockRatingMax += static_cast<float>(0.3 * statsBoostMultiplier);

		itemDef->AbsorbMin += static_cast<float>(0.5 * statsBoostMultiplier);
		itemDef->AbsorbMax += static_cast<float>(0.5 * statsBoostMultiplier);

		itemDef->DefenseMin += (7 * statsBoostMultiplier);
		itemDef->DefenseMax += (7 * statsBoostMultiplier);
	}
	else if (itemLevel >= 103 && itemLevel <= 110)
	{
		itemDef->BlockRatingMin += static_cast<float>(0.3 * statsBoostMultiplier);
		itemDef->BlockRatingMax += static_cast<float>(0.3 * statsBoostMultiplier);

		itemDef->AbsorbMin += static_cast<float>(0.7 * statsBoostMultiplier);
		itemDef->AbsorbMax += static_cast<float>(0.7 * statsBoostMultiplier);

		itemDef->DefenseMin += (13 * statsBoostMultiplier);
		itemDef->DefenseMax += (13 * statsBoostMultiplier);
	}
	else if (itemLevel >= 110 && itemLevel <= 113)
	{
		itemDef->BlockRatingMin += static_cast<float>(0.5 * statsBoostMultiplier);
		itemDef->BlockRatingMax += static_cast<float>(0.5 * statsBoostMultiplier);

		itemDef->AbsorbMin += static_cast<float>(1 * statsBoostMultiplier);
		itemDef->AbsorbMax += static_cast<float>(1 * statsBoostMultiplier);

		itemDef->DefenseMin += (20 * statsBoostMultiplier);
		itemDef->DefenseMax += (20 * statsBoostMultiplier);

	}
}


BOOL ItemServer::AddItemTimer( User * pcUser, int iType, EItemID iItemID, DWORD dwTimeTotal )
{
	//DEBUG( "AddItemTimer" );

	//Note - before char gets loaded into game the user id is 0
	// so don't use pcUser->IsValidAndInGame
	if ( pcUser == NULL || pcUser->pcUserData == NULL || pcUser->pcUserData->pcSocketData == NULL )
		return FALSE;

	//New Item Index
	int iIndex = -1;

	EItemTimerType eType = (EItemTimerType)iType;

	for ( int i = 0; i < PREMIUM_ITEM_TOTAL; i++ )
	{
		//Find New Item Slot
		if ( pcUser->sPremiumData.saItemList[i].eType == ITEMTIMERTYPE_None )
		{
			if ( iIndex == ( -1 ) )
			{
				iIndex = i;
			}
		}
		else
		{
			//Is the type same?
			if ( (pcUser->sPremiumData.saItemList[i].eType == iType) ||
				 (ItemTimerIsSame( pcUser->sPremiumData.saItemList[i].eType, iType ) == TRUE) )
			{
				//Set Index
				iIndex = i;

				//Can stack?
				if ( ItemTimerCanStack( iType ) )
				{
					iIndex = i;

					//Delete Old
					SQLDeleteItemTimer( SQLCONNECTION ( DATABASEID_UserDB_LocalServer_Timers, 6 ), pcUser, pcUser->sPremiumData.saItemList[i].dwCode );
					break;
				}
			}
		}
	}

	if ( iIndex != -1 )
	{
		pcUser->sPremiumData.saItemList[iIndex].dwCode				= iItemID;
		pcUser->sPremiumData.saItemList[iIndex].dwTimeLeft			= dwTimeTotal;
		pcUser->sPremiumData.saItemList[iIndex].dwTimeTotal			= dwTimeTotal;
		pcUser->sPremiumData.saItemList[iIndex].eType				= eType;

		if ( iIndex >= pcUser->sPremiumData.iCount - 1 )
		{
			pcUser->sPremiumData.iCount = iIndex + 1;
		}

		UpdateForceOrb( pcUser, eType, iItemID );
		UpdateUserEXPBuff(pcUser);

		//if it's an existing item, send buff again to client to update timer
		//if ( bExisting )
		//{
		//	SENDPACKET(pcUser, &pcUser->sPremiumData);
		//}

		if (    eType == ITEMTIMERTYPE_ExpUp
			 || eType == ITEMTIMERTYPE_ThirdEye
			 || eType == ITEMTIMERTYPE_MPDown
			 || eType == ITEMTIMERTYPE_SPDown
			 || eType == ITEMTIMERTYPE_HPUp
			 || eType == ITEMTIMERTYPE_MPUp
			 || eType == ITEMTIMERTYPE_SixthSense
			 || eType == ITEMTIMERTYPE_ForceOrb
			 || eType == ITEMTIMERTYPE_SHPRegen
			 || eType == ITEMTIMERTYPE_SSPRegen
			 || eType == ITEMTIMERTYPE_SMPRegen
			 || eType == ITEMTIMERTYPE_SMPDown
			 || eType == ITEMTIMERTYPE_SSPDown
			 || eType == ITEMTIMERTYPE_SBonusDamage
			 || eType == ITEMTIMERTYPE_SBonusAbsorb
			 || eType == ITEMTIMERTYPE_SMoveSpeed
			 || eType == ITEMTIMERTYPE_SExpBuff
			 || eType == ITEMTIMERTYPE_SDropBuff
			 || eType == ITEMTIMERTYPE_IceResistanceBuff
			 || eType == ITEMTIMERTYPE_PetSystem )
		{
			//Remove the old one if there is one
			SQLDeleteItemTimerByTimerType ( pcUser, eType );
		}

		SQLAddItemTimer(pcUser->iCharacterID, CHARACTERSERVER->GetCharacterName(pcUser->pcUserData), iType, iItemID, dwTimeTotal);

		return TRUE;
	}
	else
	{
		WARN ( "WARN: Timer not found for %d", iIndex );
	}

	return FALSE;
}

/// <summary>
/// Called whenever a premium item or a force orb has expired
/// </summary>
void ItemServer::OnPremiumItemCancelledOrExpired( User * pcUser, ItemPremium * psItem, BOOL bExpired )
{
	//DEBUG( "OnPremiumItemExpired" );

	if ( psItem->dwCode == 0 || psItem->eType == EItemTimerType::ITEMTIMERTYPE_None )
		return;

	if ( psItem->eType == ITEMTIMERTYPE_ForceOrb )
	{
		ITEMSERVER->RemoveForceOrb( pcUser );
		ITEMSERVER->SQLDeleteItemTimerByTimerType( pcUser, ITEMTIMERTYPE_ForceOrb );
	}
	else
	{
		SQLDeleteItemTimer( pcUser, psItem->dwCode );

		if ( bExpired && psItem->eType == EItemTimerType::ITEMTIMERTYPE_ExpUp )
		{
			CHATSERVER->SendTitleBox( pcUser, "Your EXP potion has expired!" );
		}
	}

	LOGSERVER->OnLogItemEx( pcUser, ITEMLOGID_PremiumTimerOver, "[%s](%s) Item Premium Over [%d][0x%08X][%d]", pcUser->pcUserData->szAccountName,
		CHARACTERSERVER->GetCharacterName( pcUser->pcUserData ), psItem->eType, psItem->dwCode, psItem->dwTimeTotal );

	psItem->eType = EItemTimerType::ITEMTIMERTYPE_None;
	psItem->dwCode = 0;

	UpdateUserEXPBuff(pcUser);
}


BOOL ItemServer::OnPremiumItem( DWORD dwCode, int iX, int iY, int iZ, UserData * pcUserData )
{
	EItemTimerType eType = (EItemTimerType)0;

	if ( eType == ITEMTIMERTYPE_SkillStone )
		return FALSE;

	if ( eType == ITEMTIMERTYPE_MatureStone )
		return FALSE;

	User * pcUser = USERDATATOUSER( pcUserData );

	DefinitionItem * psItemDef = NULL;
	Item sItem;
	int iRandom = -1;

	DWORD dwTotalTime = 0;

	PacketPartyPotion sPacket;

	switch ( (EItemID)dwCode )
	{
		case ITEMID_Gingerbread:
			ZeroMemory( &sPacket, sizeof( PacketPartyPotion ) );
			sPacket.iLength = sizeof( PacketPartyPotion );
			sPacket.iHeader = PKTHDR_PartyUsePot;
			sPacket.iHP		= Dice::RandomI( 700, 2000 );
			sPacket.iMP		= Dice::RandomI( 700, 2000 );
			sPacket.iSTM	= Dice::RandomI( 700, 2000 );
			SENDPACKET( USERDATATOUSER(pcUserData), &sPacket, TRUE );

			//Is in Party?
			if( ((pcUser) && pcUser->bParty && pcUser->psParty) && pcUser->psParty->iMembersCount > 1 )
			{
				for( size_t i = 0; i < MAX_PARTY_MEMBERS; i++ )
				{
					User * p = i == 0 ? pcUser->psParty->pcLeader : pcUser->psParty->pcaMembers[i-1];
					if( p && p->pcUserData && p->pcUserData->pcSocketData && p != pcUser )
					{
						int dX = 0, dZ = 0;
						dX = (p->pcUserData->sPosition.iX - pcUserData->sPosition.iX) >> 8;
						dZ = (p->pcUserData->sPosition.iZ - pcUserData->sPosition.iZ) >> 8;

						int iDist = dX*dX + dZ*dZ;
						if( iDist < DISTANCE_MAX_PARTY )
							SENDPACKET( p, &sPacket, TRUE );
					}
				}
			}
			break;

		case ITEMID_RottenLantern:
		{
			iRandom = Dice::RandomI(0, 99);

			//15%
			if ( iRandom < 15 )
			{
				psItemDef = ITEMSERVER->FindItemDefByCode ( ITEMID_RaidentForce );
			}
			//15%
			else if ( iRandom < 30 )
			{
				psItemDef = ITEMSERVER->FindItemDefByCode ( ITEMID_TransparoForce );
			}
			//15%
			else if ( iRandom < 45 )
			{
				ITEMSERVER->CreateItem1 ( pcUser, ITEMID_ManaMystic, EItemSource::HalloweenEvent, 25 );
			}
			//15%
			else if ( iRandom < 60 )
			{
				psItemDef = ITEMSERVER->FindItemDefByCode ( ITEMID_TurquoiseStone );
			}
			//15%
			else if ( iRandom < 75 )
			{
				psItemDef = ITEMSERVER->FindItemDefByCode ( ITEMID_EmeraldStone );
			}
			//15%
			else if ( iRandom < 90 )
			{
				psItemDef = ITEMSERVER->FindItemDefByCode ( ITEMID_TeleportCore );
			}
			//5%
			else if ( iRandom < 95 )
			{
				psItemDef = ITEMSERVER->FindItemDefByCode ( ITEMID_Transparo );
			}
			//5%
			else
			{
				psItemDef = ITEMSERVER->FindItemDefByCode ( ITEMID_Murky );
			}


			if ( !psItemDef )
				return FALSE;

			ITEMSERVER->CreateItem( &sItem, psItemDef, EItemSource::HalloweenEvent );
			ITEMSERVER->SendItemData( pcUserData, &sItem );
			ITEMSERVER->AddItemInventory( pcUser->pcUserData, &sItem );
			break;
		}

		case ITEMID_EvilLantern:
		{
			iRandom = Dice::RandomI(0, 99);

			//15%
			if ( iRandom < 15 )
			{
				psItemDef = ITEMSERVER->FindItemDefByCode ( ITEMID_MurkyForce );
			}
			//15%
			else if ( iRandom < 30 )
			{
				psItemDef = ITEMSERVER->FindItemDefByCode ( ITEMID_DevineForce );
			}
			//15%
			else if ( iRandom < 45 )
			{
				ITEMSERVER->CreateItem1 ( pcUser, ITEMID_ManaMystic, EItemSource::HalloweenEvent, 25 );
			}
			//15%
			else if ( iRandom < 60 )
			{
				psItemDef = ITEMSERVER->FindItemDefByCode ( ITEMID_TurquoiseStone );
			}
			//15%
			else if ( iRandom < 75 )
			{
				psItemDef = ITEMSERVER->FindItemDefByCode ( ITEMID_WitchHat );
			}
			//15%
			else if ( iRandom < 90 )
			{
				psItemDef = ITEMSERVER->FindItemDefByCode ( ITEMID_CartolaHat );
			}
			//5%
			else if ( iRandom < 95 )
			{
				psItemDef = ITEMSERVER->FindItemDefByCode ( ITEMID_Devine );
			}
			//5%
			else
			{
				psItemDef = ITEMSERVER->FindItemDefByCode ( ITEMID_Celesto );
			}

			if ( !psItemDef )
				return FALSE;

			ITEMSERVER->CreateItem( &sItem, psItemDef, EItemSource::HalloweenEvent);
			ITEMSERVER->SendItemData( pcUserData, &sItem );
			ITEMSERVER->AddItemInventory( pcUser->pcUserData, &sItem );

			break;
		}

		case ITEMID_InfernalLantern:
		{
			iRandom = Dice::RandomI ( 0, 99 );

			//15%
			if ( iRandom < 15 )
			{
				psItemDef = ITEMSERVER->FindItemDefByCode ( ITEMID_CelestoForce );
			}
			//15%
			else if ( iRandom < 30 )
			{
				psItemDef = ITEMSERVER->FindItemDefByCode ( ITEMID_MirageForce );

			}
			//15%
			else if ( iRandom < 45 )
			{
				ITEMSERVER->CreateItem1 ( pcUser, ITEMID_ManaGrand, EItemSource::HalloweenEvent, 25 );
			}
			//15%
			else if ( iRandom < 60 )
			{
				psItemDef = ITEMSERVER->FindItemDefByCode ( ITEMID_CartolaHat );
			}
			//15%
			else if ( iRandom < 75 )
			{
				psItemDef = ITEMSERVER->FindItemDefByCode ( ITEMID_WitchHat );
			}
			//10%
			else if ( iRandom < 85 )
			{
				psItemDef = ITEMSERVER->FindItemDefByCode ( ITEMID_VampireSuit );
			}
			//10%
			else if ( iRandom < 95 )
			{
				psItemDef = ITEMSERVER->FindItemDefByCode ( ITEMID_Succubus );
			}
			//5%
			else
			{
				psItemDef = ITEMSERVER->FindItemDefByCode ( ITEMID_Mirage );
			}

			if ( !psItemDef )
				return FALSE;

			ITEMSERVER->CreateItem ( &sItem, psItemDef, EItemSource::HalloweenEvent );
			ITEMSERVER->SendItemData ( pcUserData, &sItem );
			ITEMSERVER->AddItemInventory( pcUser->pcUserData, &sItem );
			break;
		}

		default:
			break;
	}

	if ( dwTotalTime > 0 )
		AddItemTimer( USERDATATOUSER( pcUserData ), eType, (EItemID)dwCode, dwTotalTime );

	return FALSE;
}



BOOL ItemServer::OnResetItem( User * pcUser, struct PacketResetItem * psPacket )
{
	BOOL bResult = FALSE;

	Item sItem;
	if ( psPacket->dwCode == ITEMID_CleanItem )
	{
		if ( !ITEMSERVER->DeleteItemInventory( pcUser->pcUserData, psPacket->dwCode, psPacket->dwHead, psPacket->dwCheckSum ) )
		{
			WARN( "OnResetItem failed - Item doesn't exist in player inventory" );

			psPacket->iResult = 0;
			psPacket->dwCode = 0;
			SENDPACKET( pcUser, psPacket );
			return FALSE;
		}

		if ( !ITEMSERVER->DeleteItemInventory( pcUser->pcUserData, psPacket->sItemData.sBaseItemID.ToItemID(), psPacket->sItemData.sItem.iChk1, psPacket->sItemData.sItem.iChk2 ) )
		{
			WARN( "OnResetItem failed - Item doesn't exist in player inventory" );

			psPacket->iResult = 0;
			psPacket->dwCode = 0;
			SENDPACKET( pcUser, psPacket );
			return FALSE;
		}

		if ( psPacket->sItemData.sItem.eCraftType == EItemCraftType::ITEMCRAFTTYPE_Mixing )
		{
			//Does the item have the new mix definition? If so, restore the original item
			//otherwise if the old mix id is '1' then use the below code instead (creates a new item)
			if ( psPacket->sItemData.sItem.eMixTypeName != EMixTypeName::MIXTYPENAME_LegacyDoNotUse )
			{
				MIXHANDLER->SetClearMixStatus( &psPacket->sItemData.sItem, psPacket->sItemData.sItem.sMixUniqueID1 );

				psPacket->sItemData.sItem.eCraftType = EItemCraftType::ITEMCRAFTTYPE_None;
				psPacket->sItemData.sItem.sMixUniqueID1 = 0;
				psPacket->sItemData.sItem.sMixColorId = 0;
				psPacket->sItemData.sItem.eMixEffect = 0;
				psPacket->sItemData.sItem.eMixTypeName = EMixTypeName::MIXTYPENAME_None;

				ReformItem( &psPacket->sItemData.sItem );

				//std::cout << "psPacket->sItemData.sItem.iBackupKey after: " << psPacket->sItemData.sItem.iBackupKey << std::endl;
				//std::cout << "psPacket->sItemData.sItem.iBackupChk after: " << psPacket->sItemData.sItem.iBackupChk << std::endl;

				if ( ITEMSERVER->AddItemInventory( pcUser->pcUserData, &psPacket->sItemData.sItem ) )
				{
					INFO( "Clear mix successful" );

					psPacket->iResult = TRUE;
					psPacket->dwCode = 0;
					SENDPACKET( pcUser, psPacket );

					bResult = TRUE;
				}
				else
				{
					WARN ( "Clear mix unsuccessful" );
				}
			}

			//legacy mix, recreate item
			else
			{
				DefinitionItem * psItem = ITEMSERVER->FindItemDefByCode( psPacket->sItemData.sBaseItemID.ToItemID() );
				ItemData * psItemData = ITEMSERVER->FindItemPointerTable( psPacket->sItemData.sBaseItemID.ToItemID() );
				if ( psItem && psItemData )
				{
					ITEMSERVER->CreateItem( &sItem, psItem, EItemSource::CleanStone, ClassFlagToCharacterClass( (EClassFlag)psPacket->sItemData.sItem.eSpecialization ) );

					CopyMemory( &sItem.sSpecData, &psPacket->sItemData.sItem.sSpecData, sizeof( SpecItemData ) );
					ZeroMemory( &psPacket->sItemData, sizeof( ItemData ) );
					CopyMemory( &psPacket->sItemData, psItemData, sizeof( ItemData ) );
					CopyMemory( &psPacket->sItemData.sItem, &sItem, sizeof( Item ) );
					ITEMSERVER->ReformItem( &psPacket->sItemData );

					if ( ITEMSERVER->AddItemInventory( pcUser->pcUserData, &psPacket->sItemData.sItem ) )
					{
						INFO( "Clear legacy mix successful" );

						psPacket->iResult = TRUE;
						psPacket->dwCode = 0;
						SENDPACKET( pcUser, psPacket );
						bResult = TRUE;
					}
					else
					{
						WARN( "Clear legacy mix unsuccessful" );
					}
				}
				else
				{
					WARN( "Item Definition not found for item id: %d", psPacket->sItemData.sBaseItemID.ToItemID() );
				}
			}
		}
		else if ( psPacket->sItemData.sItem.eCraftType == EItemCraftType::ITEMCRAFTTYPE_Aging )
		{
			//std::cout << "psPacket->sItemData.sItem.sAgeLevel before: " << psPacket->sItemData.sItem.sAgeLevel << std::endl;

			while ( psPacket->sItemData.sItem.sAgeLevel > 0 )
			{
				AGEHANDLER->OnDownAge( &psPacket->sItemData.sItem );
			}
			//std::cout << "psPacket->sItemData.sItem.sAgeLevel after: " << psPacket->sItemData.sItem.sAgeLevel << std::endl;

			psPacket->sItemData.sItem.eCraftType = EItemCraftType::ITEMCRAFTTYPE_None;
			psPacket->sItemData.sItem.sAgeLevel = 0;

			//level req for downage

			ReformItem( &psPacket->sItemData.sItem );

			if ( ITEMSERVER->AddItemInventory( pcUser->pcUserData, &psPacket->sItemData.sItem ) )
			{
				INFO( "Clear aged item successful" );

				psPacket->iResult = TRUE;
				psPacket->dwCode = 0;
				SENDPACKET( pcUser, psPacket );

				bResult = TRUE;
			}
			else
			{
				WARN ( "Clear aged item unsuccessful" );
			}
		}
		else
		{
			WARN ( "Clear item unsuccessful - unknown craft type: %d", psPacket->sItemData.sItem.eCraftType );
		}
	}
	else
	{
		WARN ( "Unknown reset item: %d", psPacket->dwCode );
	}

	if ( bResult )
	{
		ITEMSERVER->SaveItemDataToDatabase( &psPacket->sItemData.sItem );
		{
			LogItem sLog;
			sLog.ItemCount = 1;
			sLog.Item[0].dwCode = psPacket->sItemData.sBaseItemID.ToItemID();
			sLog.Item[0].dwHead = psPacket->sItemData.sItem.iChk1;
			sLog.Item[0].dwCheckSum = psPacket->sItemData.sItem.iChk2;
			sLog.Flag = ITEMLOGID_CleanStoneSuccess;
			sLog.IP = pcUser->pcUserData->pcSocketData->lIP;
			STRINGCOPY( sLog.UserID, pcUser->pcUserData->szAccountName );
			STRINGCOPY( sLog.CharName, CHARACTERSERVER->GetCharacterName( pcUser->pcUserData ) );
			LOGSERVER->OnLogItem( LogServer::LOGTYPEID_LogItem, 1, &sLog );
		}
	}
	else
	{
		LogItem sLog;
		sLog.ItemCount = 1;
		sLog.Item[0].dwCode = psPacket->sItemData.sBaseItemID.ToItemID();
		sLog.Item[0].dwHead = psPacket->sItemData.sItem.iChk1;
		sLog.Item[0].dwCheckSum = psPacket->sItemData.sItem.iChk2;
		sLog.Flag = ITEMLOGID_CleanStoneFail;
		sLog.IP = pcUser->pcUserData->pcSocketData->lIP;
		STRINGCOPY( sLog.UserID, pcUser->pcUserData->szAccountName );
		STRINGCOPY( sLog.CharName, CHARACTERSERVER->GetCharacterName( pcUser->pcUserData ) );
		LOGSERVER->OnLogItem( LogServer::LOGTYPEID_LogItem, 1, &sLog );

		psPacket->iResult = 0;
		SENDPACKET(pcUser, psPacket);
	}

	return bResult;
}

BOOL ItemServer::OnSocketItem( User * pcUser, PacketSocketSystem * psPacket )
{
	SOCKETSYSTEMHANDLER->HandlePacket( pcUser, psPacket );
	return TRUE;
}

void ItemServer::OnManufactureItem( User * pcUser, struct PacketManufactureItem * psPacket )
{
// 	if (!CheckQuestOrEventItemAging(psPacket->sItemData.sItem.szItemName, psPacket->sItemData.sItem.sItemID.ToInt()) && psPacket->sItemData.sItem.iSalePrice < 0)
// 		psPacket->sItemData.sItem.iSalePrice = 0;
//
// 	//Price
// 	int iPrice = psPacket->sItemData.sItem.iSalePrice;
//
// 	for ( int i = 0; i < 3; i++ )
// 	{
// 		if ( psPacket->iaRuneID[i] && ((psPacket->iaRuneID[i] & 0xFFFF0000) != ITEMID_RuneCodeManufacture1 &&
// 			(psPacket->iaRuneID[i] & 0xFFFFFF00) != ITEMID_SpecStoneCode && psPacket->iaRuneID[i] != ITEMID_Jera) )
// 			return;
// 	}
//
// 	//Respec
// 	{
// 		int iJera = 0;
// 		int iItemID = 0;
//
// 		for ( int i = 0; i < 3; i++ )
// 		{
// 			if ( psPacket->iaRuneID[i] == ITEMID_Jera )
// 				iJera++;
// 			if ( (psPacket->iaRuneID[i] & 0xFFFFFF00) == ITEMID_SpecStoneCode )
// 				iItemID = psPacket->iaRuneID[i];
//
// 		}
//
// 		if ( iItemID != 0 && iJera == 2 )
// 		{
// 			int iSpec		= SpecStoneToCharacterSpec( iItemID );
// 			BOOL bDeluxe	= IsDeluxeStone( iItemID );
//
// 			if ( iSpec > 0 )
// 			{
// 				// Get Item Data and Spec
// 				DefinitionItem * psItem = ITEMSERVER->FindItemDefByCode( psPacket->sItemData.sItem.sItemID.ToItemID() );
//
// 				Item sItem;
//
// 				if ( psItem )
// 				{
// 					ItemData * psItemData = ITEMSERVER->FindItemPointerTable( psPacket->sItemData.sItem.sItemID.ToItemID() );
// 					if ( psItemData )
// 					{
// 						DWORD dwItemTimerCreate = psPacket->sItemData.sItem.tTime;
//
// 						// Get Old Code
// 						DWORD dwHeadOld = 0, dwCheckSumOld = 0;
// 						dwHeadOld = psPacket->sItemData.sItem.iChk1;
// 						dwCheckSumOld = psPacket->sItemData.sItem.iChk2;
//
// 						//Successful?
// 						if ( (psPacket->sItemData.sItem.bPerfectItem) )
// 							ITEMSERVER->CreatePerfectItem( &sItem, psItem, iSpec );
// 						else
// 							ITEMSERVER->CreateItem( &sItem, psItem, iSpec );
//
// 						SpecItemData sSpecData;
// 						if ( (psPacket->sItemData.sItem.eSpecialization == 0) )
// 							CopyMemory( &sSpecData, &sItem.sSpecData, sizeof( SpecItemData ) );
// 						else
// 							CopyMemory( &sSpecData, &psPacket->sItemData.sItem.sSpecData, sizeof( SpecItemData ) );
//
// 						int iStrength	= sItem.iStrength;
// 						int iSpirit		= sItem.iSpirit;
// 						int iTalent		= sItem.iTalent;
// 						int iAgility	= sItem.iAgility;
// 						int iHealth		= sItem.iHealth;
//
// 						if ( bDeluxe == FALSE )
// 						{
// 							//Upgrade Status
// 							ITEMSERVER->UpgradeNewItem( &psPacket->sItemData.sItem, &sItem );
//
// 							ZeroMemory( &psPacket->sItemData, sizeof( ItemData ) );
// 							CopyMemory( &psPacket->sItemData, psItemData, sizeof( ItemData ) );
// 							CopyMemory( &psPacket->sItemData.sItem, &sItem, sizeof( Item ) );
// 						}
// 						else
// 						{
// 							psPacket->sItemData.sItem.iStrength = iStrength;
// 							psPacket->sItemData.sItem.iSpirit	= iSpirit;
// 							psPacket->sItemData.sItem.iTalent	= iTalent;
// 							psPacket->sItemData.sItem.iAgility	= iAgility;
// 							psPacket->sItemData.sItem.iHealth	= iHealth;
//
// 							ZeroMemory( &sItem, sizeof( Item ) );
//
// 							ITEMSERVER->CreatePerfectItem( &sItem, psItem, iSpec );
//
// 							CopyMemory( &psPacket->sItemData.sItem.sSpecData, &sItem.sSpecData, sizeof( SpecItemData ) );
//
// 							psPacket->sItemData.sItem.eSpecialization = sItem.eSpecialization;
// 						}
//
// 						ITEMSERVER->ValidateItem( &psPacket->sItemData );
// 						psPacket->sItemData.sItem.tTime = dwItemTimerCreate;
// 						ITEMSERVER->ValidateItemTime( &psPacket->sItemData.sItem );
//
// 						//Save New Item Data
// 						ITEMSERVER->SaveItemData( &psPacket->sItemData.sItem );
// 						{
// 							LogItem sLog;
// 							sLog.ItemCount = 1;
// 							sLog.Item[1].dwCode = psPacket->sItemData.sItem.sItemID.ToItemID();
// 							sLog.Item[1].dwHead = psPacket->sItemData.sItem.iChk1;
// 							sLog.Item[1].dwCheckSum = psPacket->sItemData.sItem.iChk2;
// 							sLog.Item[0].dwCode = psPacket->sItemData.sItem.sItemID.ToItemID();
// 							sLog.Item[0].dwHead = dwHeadOld;
// 							sLog.Item[0].dwCheckSum = dwCheckSumOld;
// 							sLog.Flag = ITEMLOGID_GetRecon;
// 							sLog.IP = pcUser->pcUserData->pcSocketData->lIP;
// 							STRINGCOPY( sLog.UserID, pcUser->pcUserData->szAccountName );
// 							STRINGCOPY( sLog.CharName, CHARACTERSERVER->GetCharacterName( pcUser->pcUserData ) );
// 							LOGSERVER->OnLogItem( LogServer::LOGTYPEID_LogItem, 1, &sLog );
// 						}
//
// 						LogItem sLog;
// 						ZeroMemory( &sLog, sizeof( LogItem ) );
// 						sLog.ItemCount = 3;
// 						sLog.Flag = ITEMLOGID_ManufactureLost;
// 						sLog.IP = pcUser->pcUserData->pcSocketData->lIP;
// 						STRINGCOPY( sLog.UserID, pcUser->pcUserData->szAccountName );
// 						STRINGCOPY( sLog.CharName, CHARACTERSERVER->GetCharacterName( pcUser->pcUserData ) );
//
// 						// Checksum
// 						for ( int i = 0; i < 3; i++ )
// 						{
// 							if ( ITEMSERVER->DeleteItemInventory( pcUser->pcUserData, psPacket->iaRuneID[i], psPacket->iaChk1[i], psPacket->iaChk2[i] ) < 0 )
// 							{
// 								// TODO
// 								sLog.ItemCount = 1;
// 								sLog.Flag = ITEMLOGID_ManufactureFail;
// 								sLog.Item[0].dwCode = psPacket->iaRuneID[i];
// 								sLog.Item[0].dwHead = psPacket->iaChk1[i];
// 								sLog.Item[0].dwCheckSum = psPacket->iaChk2[i];
// 								LOGSERVER->OnLogItem( LogServer::LOGTYPEID_LogItem, 1, &sLog );
//
// 								PacketLogCheat sCheat;
// 								sCheat.iCheatID = CHEATLOGID_ManufactureItemFake;
// 								sCheat.LParam = psPacket->iaRuneID[i];
// 								sCheat.SParam = psPacket->iaChk1[i];
// 								sCheat.EParam = psPacket->iaChk2[i];
// 								LOGSERVER->OnLogCheat( pcUser->pcUserData, &sCheat );
// 								return;
// 							}
//
// 							{
// 								sLog.Item[i].dwCode = psPacket->iaRuneID[i];
// 								sLog.Item[i].dwHead = psPacket->iaChk1[i];
// 								sLog.Item[i].dwCheckSum = psPacket->iaChk2[i];
// 							}
// 						}
//
// 						if ( ITEMSERVER->DeleteItemInventory( pcUser->pcUserData, psPacket->sItemData.sItem.sItemID.ToInt(), dwHeadOld, dwCheckSumOld ) < 0 )
// 						{
// 							// TODO
// 							sLog.ItemCount = 1;
// 							sLog.Flag = ITEMLOGID_ManufactureFail;
// 							sLog.Item[0].dwCode = psPacket->sItemData.sItem.sItemID.ToInt();
// 							sLog.Item[0].dwHead = psPacket->sItemData.sItem.iChk1;
// 							sLog.Item[0].dwCheckSum = psPacket->sItemData.sItem.iChk2;
// 							LOGSERVER->OnLogItem( LogServer::LOGTYPEID_LogItem, 1, &sLog );
//
// 							PacketLogCheat sCheat;
// 							sCheat.iCheatID = CHEATLOGID_ManufactureItemFake;
// 							sCheat.LParam = psPacket->sItemData.sItem.sItemID.ToInt();
// 							sCheat.SParam = psPacket->sItemData.sItem.iChk1;
// 							sCheat.EParam = psPacket->sItemData.sItem.iChk2;
// 							LOGSERVER->OnLogCheat( pcUser->pcUserData, &sCheat );
// 							return;
// 						}
//
// 						LOGSERVER->OnLogItem( LogServer::LOGTYPEID_LogItem, 1, &sLog );
//
// 						//Create New Item
// 						if ( ITEMSERVER->AddItemInventory( pcUser->pcUserData, psPacket->sItemData.sItem.sItemID.ToItemID(), psPacket->sItemData.sItem.iChk1, psPacket->sItemData.sItem.iChk2 ) < 0 )
// 						{
//
// 						}
//
// 						psPacket->iResult = TRUE;
// 						CHARACTERSERVER->GiveGOLD( pcUser, -iPrice );
// 					}
// 				}
// 			}
//
// 			psPacket->iLength = sizeof( PacketManufactureItem );
// 			psPacket->iHeader = PKTHDR_ManufactureItem;
// 			SENDPACKET( pcUser, psPacket );
// 			return;
// 		}
//
// 	}
//
// 	if ( psPacket->iRecipeID && ((psPacket->iRecipeID & 0xFFFF0000) != ITEMID_RecipeCode) )
// 		return;
//
// 	HALLOWEENHANDLER->OnManufactureItem( pcUser, psPacket );
// 	XMASHANDLER->OnManufactureItem( pcUser, psPacket );
}

void ItemServer::OnManufactureWeightPrice( User * pcUser )
{


}

void ItemServer::PHGetPerfectItem( User * pcUser, struct PacketGetItemPerfectView * psPacket )
{
	PacketItemPerfectView sPacket;
	sPacket.iDefenseOverride = 0;

	ItemData * psItemData			= FindItemPointerTable( psPacket->dwCode );

	DefinitionItem * psItemDefData	= FindItemDefByCode( psPacket->dwCode );

	if ( psItemData && psItemDefData )
	{
		ECharacterClass itemSpec = (ECharacterClass)psPacket->iItemSpec;

#ifdef ITEMFIXES
		int iPrefixID = 0;
		int iSuffixID = 0;
		iPrefixID = psPacket->iPrefixID;
		iSuffixID = psPacket->iSuffixID;
#endif


		if (itemSpec == 0) //no spec
		{
			EItemType itemType = psItemDefData->sItem.sItemID.ToItemType();

			//default specs

			//Can this character class use the item? If so, use that class spec
			if (ITEMSERVER->CharacterClassCanUseItem(pcUser->pcUserData->sCharacterData.iClass, psItemDefData))
			{
				itemSpec = pcUser->pcUserData->sCharacterData.iClass;
			}

			//Otherwise use default spec for eappch item
			else
			{
				switch (itemType)
				{
					case ITEMTYPE_Axe:		itemSpec = CHARACTERCLASS_Fighter;		 break;
					case ITEMTYPE_Javelin:	itemSpec = CHARACTERCLASS_Atalanta;		 break;
					case ITEMTYPE_Hammer:	itemSpec = CHARACTERCLASS_Mechanician;	 break;
					case ITEMTYPE_Bow:		itemSpec = CHARACTERCLASS_Archer;		 break;
					case ITEMTYPE_Sword:	itemSpec = CHARACTERCLASS_Knight;		 break;
					case ITEMTYPE_Claw:		itemSpec = CHARACTERCLASS_Mechanician;	 break;
					case ITEMTYPE_Dagger:	itemSpec = CHARACTERCLASS_Assassin;		 break;
					case ITEMTYPE_Phantom:	itemSpec = CHARACTERCLASS_Shaman;		 break;
					case ITEMTYPE_Scythe:	itemSpec = CHARACTERCLASS_Pikeman;		 break;
					case ITEMTYPE_Wand:		itemSpec = CHARACTERCLASS_Magician;		 break;
					case ITEMTYPE_Orb:		itemSpec = CHARACTERCLASS_Magician;		 break;
					case ITEMTYPE_Robe:		itemSpec = CHARACTERCLASS_Priestess;	 break;
					case ITEMTYPE_Armor:	itemSpec = CHARACTERCLASS_Fighter;		 break;
					case ITEMTYPE_Shield:	itemSpec = CHARACTERCLASS_Knight;		 break;
				}
			}
		}
		else
		{
			itemSpec = ClassFlagToCharacterClass((EClassFlag)itemSpec);
		}

		CopyMemory( &sPacket.cItemData, psItemData, sizeof( ItemData ) );
		*( UINT* )0x8B70264 = 1;
		*( UINT* )0x8B70268 = itemSpec;
#ifdef ITEMFIXES
		ITEMSERVER->CreateItem( &sPacket.cItemData.sItem, psItemDefData, EItemSource::PerfectView, itemSpec, 1,0,EItemRarity::NONE,TRUE,iPrefixID,iSuffixID);
#else
		ITEMSERVER->CreateItem( &sPacket.cItemData.sItem, psItemDefData, EItemSource::PerfectView, itemSpec, 1, 0, psPacket->eItemRarity );
#endif
		*( UINT* )0x8B70264 = 0;
		*( UINT* )0x8B70268 = 0;

		ITEMSERVER->OnSetItemPerfect(&sPacket.cItemData.sItem);

		Item sItemUp;
		CopyMemory( &sItemUp, &sPacket.cItemData.sItem, sizeof( Item ) );

		if ( psPacket->iMixType != 0 )
		{
			sItemUp.eCraftType = ITEMCRAFTTYPE_Mixing;
			sItemUp.eMixTypeName = (EMixTypeName)psPacket->iMixType;
			sItemUp.sMixUniqueID1 = psPacket->sUniqueMixId;
			sItemUp.sMixColorId = psPacket->sMixColor;
			sItemUp.eMixEffect = psPacket->iMixEffect;
		}

		else if ( psPacket->sAgeLevel > 0 )
		{
			sItemUp.eCraftType = ITEMCRAFTTYPE_Aging;
			sItemUp.sAgeLevel = psPacket->sAgeLevel;
		}

		int iPerfectBaseDefenseValue = sPacket.cItemData.sItem.iDefenseRating;

		ITEMSERVER->UpgradeNewItem( &sItemUp, &sPacket.cItemData.sItem );

		//Item has a defense value. Correct display bug by using this
		// value to calculate the difference
		if ( psPacket->iDefenseValue > 0 &&
			 sItemUp.eCraftType == ITEMCRAFTTYPE_Aging &&
			 sItemUp.sAgeLevel > 0)
		{
			Item sItemTemp;

			//copy the perfect aged item to temp
			CopyMemory ( &sItemTemp, &sPacket.cItemData.sItem, sizeof ( Item ) );

			//then override defense with the client value
			sItemTemp.iDefenseRating = psPacket->iDefenseValue;

			//deage back to 0 to get the original defense value for the original item
			AGEHANDLER->OnDownAgeHandler ( &sItemTemp, sItemUp.sAgeLevel - 1 );

			//get the true difference between base perfect defense and the original item defense
			int iDifference = iPerfectBaseDefenseValue - sItemTemp.iDefenseRating;

			//compute correct perfect defense value
			int iCorrectPerfectDefenseValue = psPacket->iDefenseValue + iDifference;

			//override the perfect defense rating with the correct one
			//sPacket.cItemData.sItem.iDefenseRating = iCorrectPerfectDefenseValue;

			sPacket.iDefenseOverride = iCorrectPerfectDefenseValue;

			//generate new checksums
			ITEMSERVER->ReformItem ( &sPacket.cItemData.sItem );
		}


		sPacket.iLength = sizeof( PacketItemPerfectView );
		sPacket.iHeader = PKTHDR_PerfectItemData;
		SENDPACKET( pcUser, &sPacket );
	}
}

BOOL ItemServer::IsItemFilteredIn( User * pcUser,  ELootFilter e )
{
	return (pcUser->iLootFilterFlags & e) == e;
}

/// <summary>
/// Returns true if user can view the loot, otherwise false
/// </summary>
BOOL ItemServer::CanViewDropItem( User * pcUser, Loot * psStageItemData )
{
	if (pcUser->bLootFilterEnabled == FALSE)
		return TRUE;

	ItemID sItemID( psStageItemData->sLootItem.sItem.sItemID );

	auto itemBase = sItemID.ToItemBase();
	auto itemType = sItemID.ToItemType();

	if ( itemBase == ITEMBASE_Potion )
	{
		if (itemType  == ITEMTYPE_HealthPotion)
			return IsItemFilteredIn(pcUser, LOOTFILTER_HPPotion);

		if (itemType  == ITEMTYPE_ManaPotion)
			return  IsItemFilteredIn(pcUser, LOOTFILTER_MPPotion);

		if (itemType  == ITEMTYPE_StaminaPotion)
			return IsItemFilteredIn(pcUser, LOOTFILTER_SPPotion);

		return TRUE;
	}
	else if ( sItemID.ToItemID() == ITEMID_Gold )
	{
		return IsItemFilteredIn(pcUser, LOOTFILTER_Gold);
	}
	else if ( itemType  == ITEMTYPE_Amulet || (itemType  == ITEMTYPE_Ring) || (itemType  == ITEMTYPE_Ring2) )
	{
		if (IsItemFilteredIn(pcUser, LOOTFILTER_Jewellery))
		{
			if (psStageItemData->sLootItem.sItem.eSpecialization == CLASSFLAG_None && IsItemFilteredIn(pcUser, LOOTFILTER_NonSpecJewellery) == FALSE)
				return FALSE;

			return TRUE;
		}

		return FALSE;
	}
	else if ( itemType  == ITEMTYPE_Sheltom )
	{
		if (IsItemFilteredIn(pcUser, LOOTFILTER_Sheltoms))
		{
			if (psStageItemData->sLootItem.sItem.eSpecialization == CLASSFLAG_None && IsItemFilteredIn(pcUser, LOOTFILTER_NonSpecSheltoms) == FALSE)
				return FALSE;

			return TRUE;
		}

		return FALSE;
	}
	else if ( itemType  == ITEMTYPE_ForceOrb )
	{
		return IsItemFilteredIn(pcUser, LOOTFILTER_ForceOrbs);
	}
	else if ( itemType == ITEMTYPE_MonsterCrystal )
	{
		return IsItemFilteredIn(pcUser, LOOTFILTER_Crystals);
	}

	//Gear
	else if (
		(itemBase == ITEMBASE_Defense) ||
		(itemType == ITEMTYPE_Orb) ||
		(itemType == ITEMTYPE_Bracelets) ||
		(itemBase == ITEMBASE_Weapon))
	{

		if (IsItemFilteredIn(pcUser, LOOTFILTER_Gear))
		{
			if (psStageItemData->sLootItem.sItem.eSpecialization == CLASSFLAG_None && IsItemFilteredIn(pcUser, LOOTFILTER_NonSpecGear) == FALSE)
				return FALSE;

			if (pcUser->sEquipmentMinLevel > 0 )
			{
				if (ItemServer::GetItemLevelRequirement(sItemID.ToItemID()) < pcUser->sEquipmentMinLevel)
				{
					return FALSE;
				}
			}

			return TRUE;
		}

		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

void ItemServer::HandlePacket( User * pcUser, PacketGetItem * psPacket )
{
	if( psPacket->iLength != sizeof(PacketGetItem) )
		return;

	//Valid Pointer?
	Map * pcMap = MAPSDATA + pcUser->GetMapID();
	if( !pcMap )
		return;

	//It's a valid map?
	if( !MAPSERVER->IsValidMap( pcMap ) )
		return;

	//Item Found?
	Loot * pcLootItem = (Loot*)MAPSERVER->FindItem( pcMap, psPacket->sPosition.iX >> 8, psPacket->sPosition.iY >> 8, psPacket->sPosition.iZ >> 8 );

	if( pcLootItem && pcLootItem->bInUse)
	{
		float fTotalGoldAmount = 0;

		//Bellatra Item
		if( pcLootItem->sLootItem.sItem.sItemID.ToItemType() == ITEMTYPE_Bellatra )
		{
			CALL_WITH_ARG2( 0x005600F0, (DWORD)pcLootItem->sLootItem.sItem.sItemID.ToItemID(), (DWORD)pcUser->pcUserData );
			MAPSERVER->DelItem( pcMap, pcLootItem );
			return;
		}


		if( pcLootItem->eLootType != ELootType::None &&
			pcLootItem->sLootItem.sItem.sItemID.ToItemID() != ITEMID_None )
		{
			//Item is new and is gold
			if (psPacket->iHeader == PKTHDR_NewItem &&
				pcLootItem->sLootItem.sItem.sItemID.ToItemID() == ITEMID_Gold)
			{

				//Pick up all nearby golds based on radius
				int iNearbyGold = MAPSERVER->CollectAllNearbyGold ( pcMap, &pcLootItem->sPosition );
				fTotalGoldAmount = static_cast<float>(pcLootItem->sLootItem.sItem.iGold);
				fTotalGoldAmount += iNearbyGold;

				//std::cout << "Nearby gold collected: " << iNearbyGold << std::endl;

				PacketGoldPickup s;
				s.iLength = sizeof(s);
				s.iHeader = PKTHDR_GoldPickup;

				if (pcUser->bParty && pcUser->psParty && pcUser->psParty->pcLeader)
				{
					std::vector<User*> pcUsersInRange;

					for (size_t i = 0; i < MAX_PARTY_MEMBERS; i++)
					{
						User* p = i == 0 ? pcUser->psParty->pcLeader : pcUser->psParty->pcaMembers[i - 1];
						if (p && p->pcUserData && p->pcUserData->pcSocketData)
						{
							if ( pcUser->pcUserData->sPosition.WithinXZDistance ( &p->pcUserData->sPosition, DISTANCE_MAX_PARTY, DISTANCE_MISC_Y_EX ) )
							{
								pcUsersInRange.push_back ( p );
							}
						}
					}

					int iGoldAmountPerUser = static_cast<int>(round(fTotalGoldAmount / (pcUsersInRange.size() * 1.0f))) + 1;

					s.iAmount = iGoldAmountPerUser;
					//std::cout << "Num of users in party: " << pcUsersInRange.size() << std::endl;
					//std::cout << "iGoldAmountPerUser:    " << iGoldAmountPerUser << std::endl;


					//Multiple users in range
					if (pcUsersInRange.size() > 1)
					{
						for (User* u : pcUsersInRange)
						{
							//send packet for gold pickup for all players in range,
							//including the player who picked up the gold
							SENDPACKET(u, &s, FALSE);

							//std::cout << "Giving " << iGoldAmountPerUser << " gold to user: " << CHARACTERSERVER->GetCharacterName(u->pcUserData) << std::endl;

							CHARACTERSERVER->GiveGOLD(u, iGoldAmountPerUser, WHEREID_GiveMoneyShared);
						}

						char szMessage[256] = { 0 };

						STRINGFORMAT(szMessage, "> %d party members gained %s Gold!",
							pcUsersInRange.size(),
							FormatNumber(iGoldAmountPerUser));

						PARTYHANDLER->SendChatParty(pcUser, szMessage, CHATCOLOR_Notice, true);
					}

					//Only one user in range.
					else
					{
						//send packet for gold pickup for player who picked up the gold
						SENDPACKET(pcUser, &s, FALSE);

						CHARACTERSERVER->GiveGOLD(pcUser, iGoldAmountPerUser, WHEREID_GiveMoneyShared);

						char szMessage[256] = { 0 };

						STRINGFORMAT(szMessage, "> Gained %s Gold!",
							FormatNumber(iGoldAmountPerUser));

						PARTYHANDLER->SendChatParty(pcUser, szMessage, CHATCOLOR_Notice, true);
					}

					pcUsersInRange.clear();
				}

				//User is not in a party. He gets all of the gold from pick up
				else
				{
					s.iAmount = static_cast<int>( fTotalGoldAmount );
					SENDPACKET(pcUser, &s, FALSE);

					CHARACTERSERVER->GiveGOLD(pcUser, s.iAmount, WHEREID_GetItem);
				}
			}
			else
			{
				//Item Drop
				SendItemData(pcUser->pcUserData, &pcLootItem->sLootItem.sItem);
			}
		}
		else
		{
			//Item Drop
			SendItemData(pcUser->pcUserData, &pcLootItem->sLootItem.sItem);
		}



		if (pcUser->bParty && pcUser->psParty && pcUser->psParty->pcLeader)
		{
			if (pcLootItem->sLootItem.sItem.sItemID.ToItemID() != ITEMID_Gold)
			{
				char szMessage[256] = { 0 };

				STRINGFORMAT(szMessage, "> %s gained %s!",
					pcUser->pcUserData->sCharacterData.szName,
					pcLootItem->sLootItem.sItem.szItemName);

				PARTYHANDLER->SendChatParty(pcUser, szMessage, CHATCOLOR_Notice, true);
			}
		}
		else
		{
			CHATSERVER->SendChatEx(pcUser, CHATCOLOR_Notice, "> Gained %s!",
				pcLootItem->sLootItem.sItem.sItemID.ToItemID() == ITEMID_Gold ?
				FormatString("%s Gold", FormatNumber(static_cast<long long>(fTotalGoldAmount))) :
				pcLootItem->sLootItem.sItem.szItemName);
		}

		MAPSERVER->DelItem( pcMap, pcLootItem );
	}
}

BOOL ItemServer::SQLLoadItemBox( User * pcUser )
{
	if( !GAME_SERVER )
	{
		UserData * pcUserData = pcUser->pcUserData;

		if( (pcUserData != NULL) && (pcUserData->szAccountName[0] != 0) )
		{
			ZeroMemory( &pcUser->sItemBox, sizeof( ItemBoxSlot ) );

			SQLConnection * pcDB = SQLCONNECTION( DATABASEID_UserDB_LocalServer_ItemBox );

			if( pcDB->Open() != FALSE )
			{
				if( pcDB->Prepare( "SELECT ID, ItemCode, ItemSpec, CoinShop, CAST(CASE WHEN Item IS NULL THEN 0 ELSE 1 END AS INT) AS HasItem FROM ItemBox WHERE AccountName=? AND DateReceived IS NULL" ) )
				{
					pcDB->BindParameterInput( 1, PARAMTYPE_String, pcUserData->szAccountName, _countof( pcUserData->szAccountName ) );

					if( pcDB->Execute() )
					{
						int i = 0;
						while( (i < MAX_ITEMSINITEMBOX) && (pcDB->Fetch()) )
						{
							ItemBoxItem * psItemBoxItem = &pcUser->sItemBox.saItemBox[i];

							psItemBoxItem->bValid = TRUE;
							pcDB->GetData( 1, PARAMTYPE_Integer, &psItemBoxItem->iID );

							char szItemCode[32] = { 0 };
							pcDB->GetData( 2, PARAMTYPE_String, szItemCode, _countof( szItemCode ) );
							psItemBoxItem->iItemID = ITEMSERVER->GetItemIDByItemCode( szItemCode );
							// Item error check
							if ( psItemBoxItem->iItemID == 0 )
								continue;

							pcDB->GetData( 3, PARAMTYPE_Integer, &psItemBoxItem->iSpecialization );
							pcDB->GetData( 4, PARAMTYPE_Integer, &psItemBoxItem->bCoinShop );
							pcDB->GetData( 5, PARAMTYPE_Integer, &psItemBoxItem->bHasItem );

							i++;
						}

						pcUser->sItemBox.iNumItems = i;
					}
				}

				pcDB->Close();
			}

			return TRUE;
		}

		return FALSE;
	}

	return FALSE;
}

void ItemServer::SQLItemLogBox( char * pszAccountName, ItemLogBox * psItemLogBox )
{
	SQLConnection * pcDB = SQLCONNECTION( DATABASEID_UserDB_LocalServer_ItemBox );

	if( pcDB->Open() != FALSE )
	{
		if( pcDB->Prepare( "UPDATE ItemBox SET DateReceived=GETDATE() WHERE ID=?" ) )
		{
			pcDB->BindParameterInput( 1, PARAMTYPE_Integer, &psItemLogBox->iID );

			pcDB->ExecuteUpdate();
		}

		pcDB->Close();
	}
}

void ItemServer::SQLItemLogBox( User * pcUser, Item * psItem, int iID, int iCount )
{

	ItemLogBox l = { 0 };
	l.iSize = sizeof( l );

	StringCchCopyA( l.sPlayer.szAccountName, _countof( l.sPlayer.szAccountName ), pcUser->pcUserData->szAccountName );
	StringCchCopyA( l.sPlayer.szCharacterName, _countof( l.sPlayer.szCharacterName ), pcUser->pcUserData->szCharacterName );
	l.sPlayer.iIP.S_un.S_addr = inet_addr( pcUser->pcUserData->pcSocketData->szIP );

	l.iActionCode = ITEMLOGID_BuyShop;

	l.iID = iID;
	l.sItem.iItemID = psItem->sItemID.ToItemID();
	l.sItem.iChecksum1 = psItem->iChk1;

	if( (l.sItem.iItemID == ITEMID_Gold) || (l.sItem.iItemID == ITEMID_Experience) )
		l.sItem.iCount = iCount;
	else
		l.sItem.iChecksum2 = psItem->iChk2;

	SQLItemLogBox( pcUser->pcUserData->szAccountName, &l );
}

int ItemServer::SQLItemBoxCount( char * pszAccountName )
{
	if( (pszAccountName == NULL) || (pszAccountName[0] == 0) )
		return 0;

	int i = 0;
	SQLConnection * pcDB = SQLCONNECTION( DATABASEID_UserDB_LocalServer_ItemBox );

	if ( pcDB->Open() != FALSE )
	{
		if ( pcDB->Prepare( "SELECT COUNT(ID) FROM ItemBox WHERE AccountName=? AND DateReceived IS NULL" ) )
		{
			pcDB->BindParameterInput( 1, PARAMTYPE_String, pszAccountName, 32 );
			if ( pcDB->Execute() && pcDB->Fetch() )
				pcDB->GetData( 1, PARAMTYPE_Integer, &i );
		}
		pcDB->Close();
	}
	return i;
}

BOOL ItemServer::GetItemBoxItem( char * pszAccountName, int iID, Item * psItem )
{
	BOOL bResult = FALSE;
	SQLConnection * pcDB = SQLCONNECTION( DATABASEID_UserDB_LocalServer_ItemBox );

	if ( pcDB->Open() )
	{
		if ( pcDB->Prepare( "SELECT Item FROM ItemBox WHERE ID=? AND AccountName=?" ) )
		{
			pcDB->BindParameterInput( 1, EDatabaseDataType::PARAMTYPE_Integer, &iID );
			pcDB->BindParameterInput( 2, EDatabaseDataType::PARAMTYPE_String, pszAccountName, 32 );

			if ( pcDB->Execute() && pcDB->Fetch() )
			{
				Item item;
				pcDB->GetBinaryData( 1, &item, sizeof( Item ) );

				CopyMemory( psItem, &item, sizeof( Item ) );

				bResult = TRUE;
			}
		}

		pcDB->Close();
	}

	return bResult;
}

BOOL ItemServer::OpenItemBox( UserData * pcUserData, BOOL bRetrieveItem )
{
	//LOGIN_SERVER only
	if (GAME_SERVER)
		return FALSE;

	User * pcUser = USERDATATOUSER( pcUserData );

	if (pcUser == NULL || pcUserData == NULL || pcUserData->pcSocketData == NULL)
		return FALSE;

	if( SQLLoadItemBox( pcUser ) )
	{
		PacketNPCItemBox p;
		ZeroMemory( &p, sizeof( PacketNPCItemBox ) );
		p.iLength = sizeof( p );
		p.iHeader = PKTHDR_NPCItemBox;

		//Retrieve Item
		if( bRetrieveItem != FALSE )
		{
			for( int i = 0; i < pcUser->sItemBox.iNumItems; i++ )
			{
				ItemBoxItem * psItemBoxItem = &pcUser->sItemBox.saItemBox[i];

				if( psItemBoxItem->bValid )
				{
					psItemBoxItem->bValid = FALSE;

					if ( ( psItemBoxItem->iItemID & 0xFFFF0000 ) == ITEMTYPE_MoneyChest )
					{
						int iGold = psItemBoxItem->iSpecialization;

						if ( ( iGold > 0 ) && ( iGold <= 20'000'000 ) ) //20kk is max gold for rainbow chest
						{
							CHATSERVER->SendChatEx ( pcUser, EChatColor::CHATCOLOR_Global, "Distributor> %s gp added to your inventory!", FormatNumber ( iGold ) );
							CHARACTERSERVER->GiveGOLD ( pcUser, iGold );
							SENDPACKETBLANK( pcUser, PKTHDR_Save, TRUE );

						}
					}
					else if( psItemBoxItem->iItemID == ITEMID_Gold )
					{
						int iGold = psItemBoxItem->iSpecialization;

						if (( iGold > 0 ) && ( iGold <= 100'000 ))
						{
							CHARACTERSERVER->GiveGOLD( pcUser, iGold );
						}
					}
					else if( psItemBoxItem->iItemID == ITEMID_Experience )
					{
						int iEXP = psItemBoxItem->iSpecialization;

						if (iEXP > 0)
						{
							CHARACTERSERVER->GiveEXP( pcUser, iEXP );
						}
					}
					else
					{
						DefinitionItem * itm = ITEMSERVER->FindItemDefByCode( psItemBoxItem->iItemID );
						if( itm != NULL )
						{
							p.bItem		= TRUE;
							p.sWeight	= (short)itm->sItem.iWeight;

							if( (psItemBoxItem->iItemID & 0xFF000000) == ITEMBASE_Potion )
							{
								CreateItem( &p.sItem, itm, psItemBoxItem->bCoinShop ? EItemSource::CoinShop : EItemSource::ItemBox, 0 );
								p.sItem.iPotionCount	= psItemBoxItem->iSpecialization;
								p.sWeight				= p.sItem.iPotionCount;
							}
							else
							{
								if ( psItemBoxItem->bHasItem )
								{
									if ( GetItemBoxItem( pcUserData->szAccountName, psItemBoxItem->iID, &p.sItem ) == FALSE )
									{
										LOGERROR( "Failed to get item (%d) from ItemBox for account %s", psItemBoxItem->iID, pcUserData->szAccountName );
										continue;
									}
								}
								else
								{
									CreateItem( &p.sItem, itm, psItemBoxItem->bCoinShop ? EItemSource::CoinShop : EItemSource::ItemBox, psItemBoxItem->iSpecialization );
								}


								if ( psItemBoxItem->bCoinShop )
								{
									INFO( "Set sale price of coin shop item to 0 for: %s", p.sItem.szItemName );

									p.sItem.iSalePrice = 0;

									if ( pcUser->GetGameLevel() >= GAMELEVEL_One &&
										 p.sItem.bDisallowDrop == TRUE)
									{
										DEBUG( "Force allow for item: %s", p.szItemName );
										p.sItem.bDisallowDrop = FALSE;
										p.sItem.eItemFlags |= EItemFlag::ForceAllowDrop;
									}

									ITEMSERVER->ReformItem(&p.sItem);
								}
							}

							//Put Item In Server Inventory
							ITEMSERVER->AddItemInventory( pcUserData, p.sItem.sItemID.ToItemID(), p.sItem.iChk1, p.sItem.iChk2 );
						}
					}

					//Log and check received in DB
					SQLItemLogBox( pcUser, &p.sItem, psItemBoxItem->iID, psItemBoxItem->iCount );

					break;
				}
			}
		}

		//Show next Item
		for( int i = 0; i < pcUser->sItemBox.iNumItems; i++ )
		{
			ItemBoxItem * psItemBoxItem = &pcUser->sItemBox.saItemBox[i];

			if( psItemBoxItem->bValid != FALSE )
			{
				DefinitionItem * itm = ITEMSERVER->FindItemDefByCode( psItemBoxItem->iItemID );
				if( itm != NULL )
				{
					StringCchCopyA( p.szItemName, _countof( p.szItemName ), itm->sItem.szItemName );
					StringCchCopyA( p.szMessage, _countof( p.szMessage ), "Click on YES, to receive your item!" );
					p.iItemID			= psItemBoxItem->iItemID;
					p.iSpecialization	= psItemBoxItem->iSpecialization;
					p.sWeight			= (short)itm->sItem.iWeight;
				}

				break;
			}
		}

		SENDPACKET( USERDATATOUSER(pcUserData), &p );

		return TRUE;
	}

	return FALSE;
}

BOOL ItemServer::AddItemOpenBox( UserData * pcUserData, char * pszItemName, int iSpec, int iQuantity, BOOL bCoinShop, Item * psItem )
{
	return AddItemOpenBox( pcUserData->szAccountName, pszItemName, iSpec, iQuantity, bCoinShop, psItem );
}

BOOL ItemServer::AddItemOpenBox( char * pszAccountName, char * pszItemName, int iSpec, int iQuantity, BOOL bCoinShop, Item * psItem )
{
	char szItemName[32] = { 0 };
	STRINGCOPY( szItemName, pszItemName );
	upper( szItemName );

	if ( lstrlenA( pszItemName ) >= 2 )
	{
		{
			SQLConnection * pcDB = SQLCONNECTION( DATABASEID_UserDB_LocalServer_ItemBox );

			if ( pcDB->Open() != FALSE )
			{
				BOOL bQuantitySpec = FALSE;

				if ( szItemName[0] == 'P' && (szItemName[1] == 'L' || szItemName[1] == 'M' || szItemName[1] == 'S') )
					bQuantitySpec = TRUE;

				if ( STRINGCOMPAREI( szItemName, "GG102" ) || STRINGCOMPAREI( szItemName, "GG101" ) )
					bQuantitySpec = TRUE;

				if ( bQuantitySpec )
				{
					if ( pcDB->Prepare( "INSERT INTO ItemBox([AccountName],[ItemCode],[ItemSpec],[CoinShop],[DateReceived]) VALUES(?,?,?,?,NULL)" ) )
					{
						pcDB->BindParameterInput( 1, PARAMTYPE_String, pszAccountName, 32 );
						pcDB->BindParameterInput( 2, PARAMTYPE_String, szItemName, 32 );
						pcDB->BindParameterInput( 3, PARAMTYPE_Integer, &iQuantity );
						pcDB->BindParameterInput( 4, PARAMTYPE_Integer, &bCoinShop );
						pcDB->Execute();
					}
					pcDB->Clear();
				}
				else if ( psItem )
				{
					if ( pcDB->Prepare( "INSERT INTO ItemBox([AccountName],[ItemCode],[ItemSpec],[CoinShop],[Item],[DateReceived]) VALUES(?,?,?,?,?,NULL)" ) )
					{
						pcDB->BindParameterInput( 1, PARAMTYPE_String, pszAccountName, 32 );
						pcDB->BindParameterInput( 2, PARAMTYPE_String, szItemName, 32 );
						pcDB->BindParameterInput( 3, PARAMTYPE_Integer, &psItem->eSpecialization );
						pcDB->BindParameterInput( 4, PARAMTYPE_Integer, &bCoinShop );
						pcDB->BindParameterInput( 5, PARAMTYPE_Binary,  psItem, sizeof (Item) );
						pcDB->Execute();
					}
					pcDB->Clear();
				}
				else
				{
					for ( int i = 0; i < iQuantity; i++ )
					{
						if ( pcDB->Prepare( "INSERT INTO ItemBox([AccountName],[ItemCode],[ItemSpec],[CoinShop],[DateReceived]) VALUES(?,?,?,?,NULL)" ) )
						{
							pcDB->BindParameterInput( 1, PARAMTYPE_String, pszAccountName, 32 );
							pcDB->BindParameterInput( 2, PARAMTYPE_String, szItemName, 32 );
							pcDB->BindParameterInput( 3, PARAMTYPE_Integer, &iSpec );
							pcDB->BindParameterInput( 4, PARAMTYPE_Integer, &bCoinShop );
							pcDB->Execute();
						}
						pcDB->Clear();
					}
				}
				pcDB->Close();
				return TRUE;
			}
		}
	}
	return FALSE;
}

void ItemServer::PHCheckItemUsing( UserData * pcUserData, PacketListUsingItem * psPacket )
{
	BOOL bCanUse = TRUE;
	DWORD dwCode = 0;
	int iPosition = 0;

	if ( psPacket->iItemListCount > _countof( psPacket->sItemList ) )
		psPacket->iItemListCount = _countof( psPacket->sItemList );

	for ( int i = 0; i < psPacket->iItemListCount; i++ )
	{
		dwCode		= psPacket->sItemList[i].iItemID & 0xFFFF0000;
		iPosition	= (int)(psPacket->sItemList[i].sPerformance[0] & 0xFF);
		bCanUse		= TRUE;

		if ( psPacket->sItemList[i].sPerformance[1] > 4 )
			psPacket->sItemList[i].sPerformance[1] = 4;

		// Weapon?
		if ( (dwCode & 0xFF000000) == ITEMBASE_Weapon )
			dwCode = ITEMTYPE_Axe;

		// Potion?
		if ( (dwCode & 0xFF000000) == ITEMBASE_Potion )
			dwCode = ITEMTYPE_ManaPotion;

		switch ( dwCode )
		{
			// Weapon
			case ITEMTYPE_Axe:
				if ( iPosition != ITEMSLOT_RightHand )
					bCanUse = FALSE;
				break;

			// Armor, Robe
			case ITEMTYPE_Armor:
			case ITEMTYPE_Robe:
				if ( psPacket->sItemList[i].iItemID == ITEMID_ChristmasCloatCostumeTM ||
					 psPacket->sItemList[i].iItemID == ITEMID_ChristmasCloatCostumeMM ||
					 psPacket->sItemList[i].iItemID == ITEMID_ChristmasCloatCostumeTF ||
					 psPacket->sItemList[i].iItemID == ITEMID_ChristmasCloatCostumeMF ||
					 psPacket->sItemList[i].iItemID == ITEMID_LunarCostume ||
					 psPacket->sItemList[i].iItemID == ITEMID_GaiaArmorMale ||
				 	 psPacket->sItemList[i].iItemID == ITEMID_GaiaArmorFemale ||
					 psPacket->sItemList[i].iItemID == ITEMID_IriaRobeMale ||
					 psPacket->sItemList[i].iItemID == ITEMID_IriaRobeFemale ||
					 psPacket->sItemList[i].iItemID == ITEMID_SilverCostumeMale ||
					 psPacket->sItemList[i].iItemID == ITEMID_SilverCostumeFemale ||
					 psPacket->sItemList[i].iItemID == ITEMID_DarkGaiaArmorMale ||
					 psPacket->sItemList[i].iItemID == ITEMID_DarkGaiaArmorFemale ||
					 psPacket->sItemList[i].iItemID == ITEMID_DarkIriaRobeMale ||
					 psPacket->sItemList[i].iItemID == ITEMID_DarkIriaRobeFemale ||
					 psPacket->sItemList[i].iItemID == ITEMID_WeddingSuit ||
					 psPacket->sItemList[i].iItemID == ITEMID_WeddingSuit_Black_Gold ||
					 psPacket->sItemList[i].iItemID == ITEMID_WeddingDress ||
					 psPacket->sItemList[i].iItemID == ITEMID_WeddingDress_Black_Gold ||
					 psPacket->sItemList[i].iItemID == ITEMID_WarriorsHanbokM ||
					 psPacket->sItemList[i].iItemID == ITEMID_WarriorsHanbokF ||
					 psPacket->sItemList[i].iItemID == ITEMID_KimonoCostumeM ||
					 psPacket->sItemList[i].iItemID == ITEMID_KimonoCostumeF ||
					 psPacket->sItemList[i].iItemID == ITEMID_ChineseSuitM ||
					 psPacket->sItemList[i].iItemID == ITEMID_ChineseSuitF )
				{
					if ( iPosition == ITEMSLOT_Costume )
						break;
				}
				if ( iPosition != ITEMSLOT_Armor )
					bCanUse = FALSE;
				break;

			// Boots
			case ITEMTYPE_Boots:
				if ( iPosition != ITEMSLOT_Boots )
					bCanUse = FALSE;
				break;

			// Gauntles
			case ITEMTYPE_Gauntlets:
				if ( iPosition != ITEMSLOT_Gauntlets )
					bCanUse = FALSE;
				break;

			// Shield, Orb
			case ITEMTYPE_Shield:
			case ITEMTYPE_Orb:
				if ( iPosition != ITEMSLOT_LeftHand )
					bCanUse = FALSE;
				break;

			// Potion
			case ITEMTYPE_ManaPotion:
			case ITEMTYPE_StaminaPotion:
			case ITEMTYPE_HealthPotion:
				if ( iPosition < ITEMSLOT_PotionOne || iPosition > ITEMSLOT_PotionThree )
					bCanUse = FALSE;
				break;

			// Sheltom
			case ITEMTYPE_Sheltom:
				if ( iPosition != ITEMSLOT_Sheltom )
					bCanUse = FALSE;
				break;

			// Amulet
			case ITEMTYPE_Amulet:
				if ( iPosition != ITEMSLOT_Amulet )
					bCanUse = FALSE;
				break;

			// Bracelets
			case ITEMTYPE_Bracelets:
				if ( iPosition != ITEMSLOT_Bracelets )
					bCanUse = FALSE;
				break;

			// Ring
			case ITEMTYPE_Ring:
			case ITEMTYPE_Ring2:
				if ( iPosition != ITEMSLOT_RingRight && iPosition != ITEMSLOT_RingLeft )
					bCanUse = FALSE;
				break;

			// Costume
			case ITEMTYPE_Costume:
			case ITEMTYPE_Costume2:
			case ITEMTYPE_Costume3:
			case ITEMTYPE_Costume4:
				if ( iPosition != ITEMSLOT_Costume )
					bCanUse = FALSE;
				break;

			default:
				break;
		}

		// Cannot use on that slot?
		if ( bCanUse == FALSE )
		{
			// Log it
			PacketLogCheat sPacket;
			sPacket.iCheatID	= CHEATLOGID_ItemPositionError;
			sPacket.LParam		= psPacket->sItemList[i].iItemID;
			sPacket.SParam		= 0;
			sPacket.EParam		= (int)psPacket->sItemList[i].sPerformance[0];
			LOGSERVER->OnLogCheat( pcUserData, &sPacket );
			return;
		}

		for ( int j = 0; j < psPacket->iItemListCount; j++ )
		{
			if ( i != j )
			{
				// Same item in 2 or more slots in use?
				if ( psPacket->sItemList[i].sPerformance[0] == psPacket->sItemList[j].sPerformance[0] )
				{
					// Log it
					PacketLogCheat sPacket;
					sPacket.iCheatID	= CHEATLOGID_ItemPositionError;
					sPacket.LParam		= psPacket->sItemList[i].iItemID;
					sPacket.SParam		= psPacket->sItemList[j].iItemID;
					sPacket.EParam		= (int)psPacket->sItemList[j].sPerformance[0];
					LOGSERVER->OnLogCheat( pcUserData, &sPacket );
					return;
				}
			}
		}
	}
}

void ItemServer::PHCheckItemUsingEx( UserData * pcUserData, PacketListUsingItem * psPacket )
{
	BOOL bCanUse = TRUE;
	DWORD dwCode = 0;
	int iWPCount = 0, iDSCount = 0;
	DWORD dwWPCode = 0, dwDSCode = 0;
	int iPosition = 0;

	if ( psPacket->iItemListCount > _countof( psPacket->sItemList ) )
		psPacket->iItemListCount = _countof( psPacket->sItemList );

	for ( int i = 0; i < psPacket->iItemListCount; i++ )
	{
		dwCode		= psPacket->sItemList[i].iItemID & 0xFFFF0000;
		iPosition	= (int)(psPacket->sItemList[i].sPerformance[0] & 0xFF);
		bCanUse		= TRUE;

		if ( psPacket->sItemList[i].sPerformance[1] > 4 )
			psPacket->sItemList[i].sPerformance[1] = 4;

		// Weapon?
		if ( (dwCode & 0xFF000000) == ITEMBASE_Weapon )
			dwCode = ITEMTYPE_Axe;

		// Potion?
		if ( (dwCode & 0xFF000000) == ITEMBASE_Potion )
			dwCode = ITEMTYPE_ManaPotion;

		switch ( dwCode )
		{
			// Weapon
			case ITEMTYPE_Axe:
				if ( iPosition != ITEMSLOT_RightHand )
					bCanUse = FALSE;

				iWPCount++;
				dwWPCode = psPacket->sItemList[i].iItemID;
				break;

			// Armor, Robe
			case 0x02010000:
			case 0x02050000:
				if ( psPacket->sItemList[i].iItemID == ITEMID_ChristmasCloatCostumeTM ||
					psPacket->sItemList[i].iItemID == ITEMID_ChristmasCloatCostumeMM ||
					psPacket->sItemList[i].iItemID == ITEMID_ChristmasCloatCostumeTF ||
					psPacket->sItemList[i].iItemID == ITEMID_ChristmasCloatCostumeMF ||
					psPacket->sItemList[i].iItemID == ITEMID_LunarCostume ||
					psPacket->sItemList[i].iItemID == ITEMID_GaiaArmorMale ||
					psPacket->sItemList[i].iItemID == ITEMID_GaiaArmorFemale ||
					psPacket->sItemList[i].iItemID == ITEMID_IriaRobeMale ||
					psPacket->sItemList[i].iItemID == ITEMID_IriaRobeFemale ||
					psPacket->sItemList[i].iItemID == ITEMID_SilverCostumeMale ||
					psPacket->sItemList[i].iItemID == ITEMID_SilverCostumeFemale ||
					psPacket->sItemList[i].iItemID == ITEMID_DarkGaiaArmorMale ||
					psPacket->sItemList[i].iItemID == ITEMID_DarkGaiaArmorFemale ||
					psPacket->sItemList[i].iItemID == ITEMID_DarkIriaRobeMale ||
					psPacket->sItemList[i].iItemID == ITEMID_DarkIriaRobeFemale ||
					psPacket->sItemList[i].iItemID == ITEMID_WeddingSuit ||
					psPacket->sItemList[i].iItemID == ITEMID_WeddingDress ||
					psPacket->sItemList[i].iItemID == ITEMID_WarriorsHanbokM ||
					psPacket->sItemList[i].iItemID == ITEMID_WarriorsHanbokF ||
					psPacket->sItemList[i].iItemID == ITEMID_KimonoCostumeM ||
					psPacket->sItemList[i].iItemID == ITEMID_KimonoCostumeF ||
					psPacket->sItemList[i].iItemID == ITEMID_ChineseSuitM ||
					psPacket->sItemList[i].iItemID == ITEMID_ChineseSuitF )
				{
					if ( iPosition == ITEMSLOT_Costume )
						break;
				}
				if ( iPosition != ITEMSLOT_Armor )
					bCanUse = FALSE;
				break;

			// Boots
			case 0x02020000:
				if ( iPosition != ITEMSLOT_Boots )
					bCanUse = FALSE;
				break;

			// Gauntles
			case 0x02030000:
				if ( iPosition != ITEMSLOT_Gauntlets )
					bCanUse = FALSE;
				break;

			// Shield, Orb
			case 0x02040000:
			case 0x03030000:
				if ( iPosition != ITEMSLOT_LeftHand )
					bCanUse = FALSE;

				iDSCount++;
				dwDSCode = psPacket->sItemList[i].iItemID;
				break;

			// Potion
			case ITEMTYPE_ManaPotion:
			case ITEMTYPE_StaminaPotion:
			case ITEMTYPE_HealthPotion:
				if ( iPosition < ITEMSLOT_PotionOne || iPosition > ITEMSLOT_PotionThree )
					bCanUse = FALSE;
				break;

			// Sheltom
			case 0x02350000:
				if ( iPosition != ITEMSLOT_Sheltom )
					bCanUse = FALSE;
				break;

			// Amulet
			case 0x03010000:
				if ( iPosition != ITEMSLOT_Amulet )
					bCanUse = FALSE;
				break;

			// Bracelets
			case 0x03020000:
				if ( iPosition != ITEMSLOT_Bracelets )
					bCanUse = FALSE;
				break;

			// Ring
			case 0x03040000:
			case 0x03050000:
				if ( iPosition != ITEMSLOT_RingRight && iPosition != ITEMSLOT_RingLeft )
					bCanUse = FALSE;
				break;

			// Costume
			case 0x02100000:
			case 0x02110000:
			case 0x02060000:
			case 0x02070000:
				if ( iPosition != ITEMSLOT_Costume )
					bCanUse = FALSE;
				break;

			default:
				break;
		}

		// Cannot use on that slot?
		if ( bCanUse == FALSE )
		{
			// Log it
			PacketLogCheat sPacket;
			sPacket.iCheatID	= CHEATLOGID_ItemPositionError;
			sPacket.LParam		= psPacket->sItemList[i].iItemID;
			sPacket.SParam		= 0;
			sPacket.EParam		= (int)psPacket->sItemList[i].sPerformance[0];
			LOGSERVER->OnLogCheat( pcUserData, &sPacket );
			return;
		}

		for ( int j = 0; j < psPacket->iItemListCount; j++ )
		{
			if ( i != j )
			{
				// Same item in 2 or more slots in use?
				if ( psPacket->sItemList[i].sPerformance[0] == psPacket->sItemList[j].sPerformance[0] )
				{
					// Log it
					PacketLogCheat sPacket;
					sPacket.iCheatID	= CHEATLOGID_ItemPositionError;
					sPacket.LParam		= psPacket->sItemList[i].iItemID;
					sPacket.SParam		= psPacket->sItemList[j].iItemID;
					sPacket.EParam		= (int)psPacket->sItemList[j].sPerformance[0];
					LOGSERVER->OnLogCheat( pcUserData, &sPacket );
					return;
				}
			}
		}
	}

	if ( iWPCount && iDSCount )
	{
		for ( UINT i = 0; i < NUM_ITEMS; i++ )
		{
			if ( pAllocItemTable[i].sBaseItemID.ToItemID() == dwWPCode )
			{
				if ( pAllocItemTable[i].IsTwoHandedWeapon() )
				{
					// Log it
					PacketLogCheat sPacket;
					sPacket.iCheatID	= CHEATLOGID_ItemPositionError;
					sPacket.LParam		= dwWPCode;
					sPacket.SParam		= dwDSCode;
					sPacket.EParam		= -1;
					LOGSERVER->OnLogCheat( pcUserData, &sPacket );
					return;
				}
			}
		}
	}
}

void ItemServer::UpgradeNewItem( Item * psItemOld, Item * psItemNew )
{
	if ( psItemOld->eCraftType == ITEMCRAFTTYPE_Mixing )
	{
		psItemNew->eCraftType = ITEMCRAFTTYPE_Mixing;

		MIXHANDLER->SetItemMixByUniqueID ( psItemNew, psItemOld->sMixUniqueID1 );

		psItemNew->eMixTypeName = psItemOld->eMixTypeName;
		psItemNew->eMixEffect = psItemOld->eMixEffect;
		psItemNew->sMixUniqueID1 = psItemOld->sMixUniqueID1;
		psItemNew->sMixColorId = (USHORT)MIXHANDLER->GetMixColorFromMixType(psItemOld->eMixTypeName);

		ITEMSERVER->ReformItem ( psItemNew );
	}
	else if ( psItemOld->eCraftType == ITEMCRAFTTYPE_Aging )
	{
		if ( psItemNew->sItemID.ToItemType () == ITEMTYPE_Boots || psItemNew->sItemID.ToItemType () == ITEMTYPE_Gauntlets || psItemNew->sItemID.ToItemType () == ITEMTYPE_Bracelets )
		{
			psItemNew->eCraftType = ITEMCRAFTTYPE_None;
			psItemNew->sAgeLevel = 0;
			psItemNew->bMaturing = false;
			psItemNew->sMatureBar.sCur = 0;
			psItemNew->sMatureBar.sMax = 0;

			ITEMSERVER->ReformItem ( psItemNew );
			return;
		}

		psItemNew->eCraftType = ITEMCRAFTTYPE_Aging;
		AGEHANDLER->OnUpAgeHandler( psItemNew, (int)psItemOld->sAgeLevel );
		psItemNew->bMaturing = psItemOld->bMaturing;
		psItemNew->sMatureBar.sCur = psItemOld->sMatureBar.sCur;
		psItemNew->sMatureBar.sMax = psItemOld->sMatureBar.sMax;

		ITEMSERVER->ReformItem( psItemNew );
	}
}

#define	sinITEM_MASK1	0xFF000000 //base
#define	sinITEM_MASK2	0xFFFF0000 //type
#define	sinITEM_MASK3	0x0000FFFF

/// <summary>
/// Leaked: rsRecvBuyShopItem
/// ASM: 0x56A260
/// </summary>
int ItemServer::RecvBuyShopItem ( User * pcUser, PacketBuyNPCShop * lpTransBuyShopItem )
{
	int money;

	if ( pcUser == NULL || pcUser->IsValidAndInGame () == FALSE )
		return FALSE;


	//this code isn't used.
	//HandlePacket( pcUser, lpTransBuyShopItem );
	//if ( lpTransBuyShopItem->iHeader == 0 )
	//	return FALSE;


	UserData * pcUserData = pcUser->pcUserData;

	//disallow buying of mana
	if ( ( lpTransBuyShopItem->sItemData.sItem.sItemID.ToInt() & sinITEM_MASK2 ) == ITEMTYPE_ManaPotion )
		return FALSE;

	BOOL bItemIsValid = ITEMSERVER->CheckItemForm ( &lpTransBuyShopItem->sItemData.sItem );

	//check if item is valid
	if ( bItemIsValid == FALSE )
	{
		WARN ( "Player (%s) trying to buy invalid item %d at sale price %d",
			CHARACTERSERVER->GetCharacterName ( pcUser->pcUserData ),
			lpTransBuyShopItem->sItemData.sItem.sItemID.ToItemID (),
			lpTransBuyShopItem->sItemData.sItem.iSalePrice );

		PacketLogCheat sPacket;
		sPacket.iCheatID = CHEATLOGID_NPCBuyItemNotValid;
		sPacket.LParam = lpTransBuyShopItem->sItemData.sItem.iSalePrice;
		LOGSERVER->OnLogCheat ( pcUserData, &sPacket, TRUE, TRUE );  //d/c and ban

		return FALSE;
	}

	//spam attempt
	if ( lpTransBuyShopItem->iCount < 0 || lpTransBuyShopItem->iCount > 10000 )
	{
		WARN ( "Player (%s) trying to buy %d x of item %d at a price of %d per unit",
			CHARACTERSERVER->GetCharacterName(pcUser->pcUserData),
			lpTransBuyShopItem->iCount,
			lpTransBuyShopItem->sItemData.sItem.sItemID.ToItemID (),
			lpTransBuyShopItem->sItemData.sItem.iSalePrice );

		PacketLogCheat sPacket;
		sPacket.iCheatID = CHEATLOGID_NPCBuyItemCountIsNotValid;
		sPacket.LParam = lpTransBuyShopItem->sItemData.sItem.iSalePrice;
		sPacket.SParam = lpTransBuyShopItem->iCount;
		LOGSERVER->OnLogCheat ( pcUser->pcUserData, &sPacket, TRUE, TRUE );  //d/c and ban

		return FALSE;
	}

	BOOL bIsPotion = lpTransBuyShopItem->iCount > 0 && ( lpTransBuyShopItem->sItemData.sItem.sItemID.ToItemBase () ) == ITEMBASE_Potion;

	int iPlayerGold = pcUserData->GetGold ();




	int iBaseBuyPriceDef = ITEMSERVER->GetItemBuyPrice ( lpTransBuyShopItem->sItemData.sItem.sItemID.ToItemID (), false, (int)lpTransBuyShopItem->sItemData.sItem.eRarity );
	int iOriginalSalePrice = iBaseBuyPriceDef; //this is the original sale price seen on client side. The price is always same regardless if there is spec or not

	DEBUG ( "Bless Castle Tax: %d ", BLESSCASTLESERVER->GetCurrentTax () );
	DEBUG ( "iOriginalSalePrice: %d ", iOriginalSalePrice );

	//Apply Bless Castle Tax to match what client would 'see'
	iOriginalSalePrice += ( iBaseBuyPriceDef * BLESSCASTLESERVER->GetCurrentTax () ) / 100;

	DEBUG ( "iOriginalSalePrice after: %d ", iOriginalSalePrice );

	ItemData * psItemData = FindItemPointerTable ( lpTransBuyShopItem->sItemData.sBaseItemID.ToInt () );
	if ( psItemData == NULL )
	{
		WARN ( "NPC Buy Item id is invalid: %d at sale price of %d",
			   lpTransBuyShopItem->sItemData.sItem.sItemID.ToItemID (),
			   lpTransBuyShopItem->sItemData.sItem.iSalePrice );

		PacketLogCheat sPacket;
		sPacket.iCheatID = CHEATLOGID_NPCBuyItemIdNotFound;
		sPacket.LParam = psItemData->sItem.iSalePrice;
		sPacket.SParam = lpTransBuyShopItem->sItemData.sItem.iSalePrice;
		LOGSERVER->OnLogCheat ( pcUser->pcUserData, &sPacket, TRUE, TRUE ); // d/c and ban

		return FALSE;
	}

	if ( !STRINGCOMPAREI ( psItemData->szBaseName, lpTransBuyShopItem->sItemData.sItem.szItemName ) )
	{
		WARN ( "NPC Buy Item name does not match: %s vs %s",
			psItemData->szBaseName,
			lpTransBuyShopItem->sItemData.sItem.szItemName );

		PacketLogCheat sPacket;
		sPacket.iCheatID = CHEATLOGID_NPCBuyItemNameMismatch;
		sPacket.LParam = lpTransBuyShopItem->sItemData.sBaseItemID.ToInt ();
		sPacket.SParam = lpTransBuyShopItem->sItemData.sItem.sItemID.ToInt ();
		sPacket.EParam = (int)lpTransBuyShopItem->sItemData.sItem.szItemName;
		LOGSERVER->OnLogCheat ( pcUser->pcUserData, &sPacket, TRUE, TRUE ); // d/c and ban

		return FALSE;
	}

	//should match
	if ( iBaseBuyPriceDef != lpTransBuyShopItem->sItemData.sItem.iSalePrice )
	{
		WARN ( "NPC Buy Item price does not match: %d vs %d",
			iBaseBuyPriceDef,
			lpTransBuyShopItem->sItemData.sItem.iSalePrice );

		PacketLogCheat sPacket;
		sPacket.iCheatID = CHEATLOGID_NPCBuyItemPriceMismatch;
		sPacket.LParam = iBaseBuyPriceDef;
		sPacket.SParam = lpTransBuyShopItem->sItemData.sItem.iSalePrice;
		LOGSERVER->OnLogCheat ( pcUser->pcUserData, &sPacket, TRUE, TRUE ); // d/c and ban

		return FALSE;
	}

	if ( bIsPotion )
	{
		//Note - for potions, it needs to be based on 'iBaseBuyPriceDef'
		money = iOriginalSalePrice * lpTransBuyShopItem->iCount;
	}
	else
	{
		money = iOriginalSalePrice;
	}

	//player does not have enough gold?
	if ( iPlayerGold < money )
	{
		WARN ( "Player (%s) did not have enough buy to purchase NPC item id %d at price of %d",
			CHARACTERSERVER->GetCharacterName ( pcUser->pcUserData ),
			money,
			iPlayerGold );

		PacketLogCheat sPacket;
		sPacket.iCheatID = bIsPotion ? ELogCheat::CHEATLOGID_NPCBuyPotionNotEnoughGold : ELogCheat::CHEATLOGID_NPCBuyItemNotEnoughGold;
		sPacket.LParam = lpTransBuyShopItem->sItemData.sItem.iSalePrice;
		sPacket.EParam = iPlayerGold;
		LOGSERVER->OnLogCheat ( pcUserData, &sPacket, TRUE );
		return FALSE;
	}

	int iItemId = lpTransBuyShopItem->sItemData.sItem.sItemID.ToItemID ();
	bool isSpec = lpTransBuyShopItem->sItemData.sItem.eSpecialization != 0;

	const EItemBase itemBase = lpTransBuyShopItem->sItemData.sItem.sItemID.ToItemBase();
	const EItemType itemType = lpTransBuyShopItem->sItemData.sItem.sItemID.ToItemType();

	if ( ( itemBase == EItemBase::ITEMBASE_Weapon || itemType == EItemType::ITEMTYPE_Armor || itemType == EItemType::ITEMTYPE_Robe
		|| itemType == EItemType::ITEMTYPE_Amulet || itemType == EItemType::ITEMTYPE_Shield || itemType == EItemType::ITEMTYPE_Boots
		|| itemType == EItemType::ITEMTYPE_Gauntlets || itemType == EItemType::ITEMTYPE_Bracelets
		|| itemType == EItemType::ITEMTYPE_Ring || itemType == EItemType::ITEMTYPE_Ring2 || itemType == EItemType::ITEMTYPE_Orb ) )
	{
		if ( lpTransBuyShopItem->sItemData.sItem.eRarity == EItemRarity::NONE )
		{
			lpTransBuyShopItem->sItemData.sItem.eRarity = EItemRarity::COMMON;
		}
	}

	//update sale price to correct price (x 1.2 for spec and x 1.0 for non-spec) so checks in other place will work (aging etc)
	lpTransBuyShopItem->sItemData.sItem.iSalePrice = ITEMSERVER->GetItemBuyPrice ( iItemId, isSpec, (int)lpTransBuyShopItem->sItemData.sItem.eRarity );

	//special cases for prices
	switch ( iItemId )
	{
		case EItemID::ITEMID_RebirthScroll:
		case EItemID::ITEMID_TeleportCore:
		case EItemID::ITEMID_HairPotionA:
		case EItemID::ITEMID_HairPotionB:
		case EItemID::ITEMID_HairPotionC:
		{
			lpTransBuyShopItem->sItemData.iSalePrice = 0;
			lpTransBuyShopItem->sItemData.sItem.iSalePrice = 0;
			break;
		}
	}

	ITEMSERVER->ReformItem ( &lpTransBuyShopItem->sItemData.sItem );

	ITEMSERVER->AddItemInventory ( pcUserData, &lpTransBuyShopItem->sItemData.sItem );

	ITEMSERVER->SaveItemDataToDatabase ( &lpTransBuyShopItem->sItemData.sItem );

	LOGSERVER->OnLogItemEx ( USERDATATOUSER( pcUserData ), ITEMLOGID_BuyShop, "[%s](%s) Brought Item from Shop  ( [%s][0x%08X] %d x %d )",
		pcUserData->szAccountName,
		CHARACTERSERVER->GetCharacterName ( pcUserData ),
		lpTransBuyShopItem->sItemData.sItem.szItemName,
		lpTransBuyShopItem->sItemData.sItem.sItemID.ToInt (),
		lpTransBuyShopItem->sItemData.sItem.iChk1,
		lpTransBuyShopItem->sItemData.sItem.iChk2);


	if ( bIsPotion )
	{
		ITEMSERVER->AddPotion ( pcUserData, lpTransBuyShopItem->sItemData.sItem.sItemID.ToItemID (), lpTransBuyShopItem->iCount );

		switch ( lpTransBuyShopItem->sItemData.sItem.sItemID.ToItemType() )
		{
			case ITEMTYPE_HealthPotion:
				WRITEDWORD ( 0x7AC5680, READDWORD ( 0x7AC5680 ) + money ); //CurrencyLog.int64_ShopPotionMoney[0] //for Bless Castle
				break;
			case ITEMTYPE_ManaPotion:
				WRITEDWORD ( 0x7AC5678, READDWORD ( 0x7AC5680 ) + money ); //CurrencyLog.int64_ShopPotionMoney[1] //for Bless Castle
				break;
			case ITEMTYPE_StaminaPotion:
				WRITEDWORD ( 0x7AC5688, READDWORD ( 0x7AC5680 ) + money ); //CurrencyLog.int64_ShopPotionMoney[2] //for Bless Castle
				break;
		}

		pcUserData->pcSocketData->SendPacket ( (Packet *)lpTransBuyShopItem, TRUE );
		//ITEMSERVER->CreateItem ( pcUser, lpTransBuyShopItem->sItemData.sItem.sItemID.ToItemID(), lpTransBuyShopItem->iCount );
	}
	else
	{
		WRITEDWORD ( 0x7AC5670, READDWORD ( 0x7AC5670 ) + money ); //CurrencyLog.int64_ShopMoney[2] //for Bless Castle
		ITEMSERVER->SendItemDataToInventory ( pcUserData, &lpTransBuyShopItem->sItemData.sItem );
	}

	DEBUG ( "Player (%s) purchased NPC item id %d at price of %d",
		CHARACTERSERVER->GetCharacterName ( pcUser->pcUserData ),
		lpTransBuyShopItem->sItemData.sItem.sItemID.ToItemID (),
		money );

	//Note - this needs to be done AFTER sending of the above packet, otherwise the client will get 'Not enough gold'
	// because calling GiveGOLD will actually update the player's gold amount on client side where the one in ASM doesn't (I think)
	CHARACTERSERVER->GiveGOLD ( USERDATATOUSER ( pcUserData ), -money, WHEREID_BuyItem );

	return TRUE;
}


/// <summary>
/// Sell item to NPC
/// </summary>
BOOL ItemServer::HandlePacket(User * pcUser, PacketSellNPCShop * psPacket)
{
	bool l_ItemFound = HasItemInInventory(pcUser->pcUserData, psPacket->sItem.sItemID.ToItemID(), psPacket->sItem.iChk1, psPacket->sItem.iChk2);

	/// SCAM ATTEMPT
	if (!l_ItemFound)
	{
		WARN( "Corrected PacketSellNPCShop item not found: [%d] [%d] [%d]", psPacket->sItem.sItemID.ToItemID(), psPacket->sItem.iChk1, psPacket->sItem.iChk2 );

		PacketLogCheat sPacket;
		sPacket.iCheatID	= CHEATLOGID_GenerateGoldHack;
		sPacket.LParam		= 0;
		sPacket.SParam		= 0;
		sPacket.WxParam		= 8;
		LOGSERVER->OnLogCheat(pcUser->pcUserData, &sPacket);

		if ( pcUser->GetGameLevel() != GAMELEVEL_Four )
		{
			NETSERVER->DisconnectUser( pcUser->pcUserData );

			SENDPACKETBLANK( USERDATATOUSER( pcUser->pcUserData ), PKTHDR_Disconnect );
			WNDPROC_SOCKETCLOSE( SOCKETSERVER->GetHWND(), pcUser->pcUserData->pcSocketData );
		}

		return FALSE;
	}

	int l_SellPrice = 0;
	const EItemID itemID = psPacket->sItem.sItemID.ToItemID();

	/// Temp fix duplicate items
	switch (psPacket->sItem.sItemID.ToItemID())
	{
		case 16844800:
		case 16910336:
		case 17041664:
		case 17107200:
		case 17172992:
		case 17238528:
		case 17303808:
		case 17369344:
		case 17434880:
		{
			int l_ExceptedPriceA = GetItemSellPrice(itemID,  20000, psPacket->sItem.eSpecialization != 0, psPacket->sItem.sIntegrity.sCur, psPacket->sItem.sIntegrity.sMax) * psPacket->Count;
			int l_ExceptedPriceB = GetItemSellPrice(itemID, 200000, psPacket->sItem.eSpecialization != 0, psPacket->sItem.sIntegrity.sCur, psPacket->sItem.sIntegrity.sMax) * psPacket->Count;

			if (psPacket->Money != l_ExceptedPriceA && psPacket->Money != l_ExceptedPriceB)
				l_SellPrice = GetItemSellPrice(psPacket->sItem.sItemID.ToItemID(), psPacket->sItem.eSpecialization != 0, psPacket->sItem.sIntegrity.sCur, psPacket->sItem.sIntegrity.sMax, (int)psPacket->sItem.eRarity) * psPacket->Count;
			break;
		}


			default:
			l_SellPrice = GetItemSellPrice(psPacket->sItem.sItemID.ToItemID(), psPacket->sItem.eSpecialization != 0, psPacket->sItem.sIntegrity.sCur, psPacket->sItem.sIntegrity.sMax, (int)psPacket->sItem.eRarity) * psPacket->Count;
			break;
	}

    if (psPacket->Count < 0 || psPacket->Count > 1000 || l_SellPrice == std::numeric_limits<int>::max())
	{
		WARN("Player(%s) trying to sell %d x the item %d at a price of %d gold",
			pcUser->pcUserData->szCharacterName, psPacket->Count, psPacket->sItem.sItemID.ToItemID(), l_SellPrice);

		PacketLogCheat sPacket;
		sPacket.iCheatID	= CHEATLOGID_GenerateGoldHack;
		sPacket.LParam		= l_SellPrice;
		sPacket.SParam		= psPacket->Money;
		sPacket.WxParam		= 10;
		LOGSERVER->OnLogCheat(pcUser->pcUserData, &sPacket);

		if ( pcUser->GetGameLevel() != GAMELEVEL_Four )
		{
			NETSERVER->DisconnectUser( pcUser->pcUserData );

			SENDPACKETBLANK( USERDATATOUSER( pcUser->pcUserData ), PKTHDR_Disconnect );
			WNDPROC_SOCKETCLOSE( SOCKETSERVER->GetHWND(), pcUser->pcUserData->pcSocketData );
		}

		return FALSE;
	}
	else if (l_SellPrice != std::numeric_limits<int>::max())
	{
        /// This is doed on server.exe
		///pcUser->pcUserData->iInventoryGold += l_SellPrice * psPacket->Count;

		LOGSERVER->OnLogItemEx(pcUser, ITEMLOGID_SellShop, "[%s](%s) Sell item %d count %d for %d gold",
			pcUser->pcUserData->szAccountName, pcUser->pcUserData->szCharacterName, psPacket->sItem.sItemID.ToItemID(), psPacket->Count, l_SellPrice);
	}

	return TRUE;
}

BOOL ItemServer::HandlePacket(User * pcUser, PacketRepairNPCShop * psPacket)
{
	//std::cout << "BOOL ItemServer::HandlePacket(User * pcUser, PacketRepairNPCShop * psPacket)" << std::endl;

    bool l_ItemFound = HasItemInInventory(pcUser->pcUserData, psPacket->sItemData.sItem.sItemID.ToItemID(), psPacket->sItemData.sItem.iChk1, psPacket->sItemData.sItem.iChk2);


	/// SCAM ATTEMPT
	if (!l_ItemFound)
	{
		WARN ( "PacketRepairNPCShop item not found in player inventory" );

		PacketLogCheat sPacket;
		sPacket.iCheatID	= CHEATLOGID_GenerateGoldHack;
		sPacket.LParam		= 0;
		sPacket.SParam		= 0;
		sPacket.WxParam		= 8;
		LOGSERVER->OnLogCheat(pcUser->pcUserData, &sPacket);

		if ( pcUser->GetGameLevel() != GAMELEVEL_Four )
		{
			NETSERVER->DisconnectUser( pcUser->pcUserData );

			SENDPACKETBLANK( USERDATATOUSER( pcUser->pcUserData ), PKTHDR_Disconnect );
			WNDPROC_SOCKETCLOSE( SOCKETSERVER->GetHWND(), pcUser->pcUserData->pcSocketData );
		}

		return FALSE;
	}

    int l_RepairPrice = 0;

	/// Temp fix duplicate items
	switch (psPacket->sItemData.sItem.sItemID.ToItemID())
	{
		//quest item
		//the quest item has same id as the original item..
		//i.e. Demon Hunter and Double Headed Axe both have 16844800
		case 16844800:
		case 16910336:
		case 17041664:
		case 17107200:
		case 17172992:
		case 17238528:
		case 17303808:
		case 17369344:
		case 17434880:
		{
			int l_ExceptedPriceA = GetItemRepairPrice(psPacket->sItemData.sItem.sItemID.ToItemID(),  20000, psPacket->sItemData.sItem.eSpecialization != 0, psPacket->sItemData.sItem.sIntegrity.sCur, psPacket->sItemData.sItem.sIntegrity.sMax);
			int l_ExceptedPriceB = GetItemRepairPrice(psPacket->sItemData.sItem.sItemID.ToItemID(), 200000, psPacket->sItemData.sItem.eSpecialization != 0, psPacket->sItemData.sItem.sIntegrity.sCur, psPacket->sItemData.sItem.sIntegrity.sMax);

            if (psPacket->Money != l_ExceptedPriceA && abs(l_ExceptedPriceA - psPacket->Money) > 1000 && psPacket->Money != l_ExceptedPriceB && abs(l_ExceptedPriceB - psPacket->Money) > 1000)
                l_RepairPrice = GetItemRepairPrice(psPacket->sItemData.sItem.sItemID.ToItemID(), psPacket->sItemData.sItem.eSpecialization != 0, psPacket->sItemData.sItem.sIntegrity.sCur, psPacket->sItemData.sItem.sIntegrity.sMax, (int)psPacket->sItemData.sItem.eRarity);
            else if (psPacket->Money == l_ExceptedPriceA || abs(l_ExceptedPriceA - psPacket->Money) < 1000)
                l_RepairPrice = l_ExceptedPriceA;
            else if (psPacket->Money == l_ExceptedPriceB || abs(l_ExceptedPriceB - psPacket->Money) < 1000)
                l_RepairPrice = l_ExceptedPriceB;

			break;
		}

		default:
            l_RepairPrice = GetItemRepairPrice(psPacket->sItemData.sItem.sItemID.ToItemID(), psPacket->sItemData.sItem.eSpecialization != 0, psPacket->sItemData.sItem.sIntegrity.sCur, psPacket->sItemData.sItem.sIntegrity.sMax, (int)psPacket->sItemData.sItem.eRarity);
			break;
	}

#ifdef DEV_MODE

	INFO( "PacketRepairNPCShop - l_RepairPrice = %d", l_RepairPrice );
	INFO( "PacketRepairNPCShop - psPacket->Money = %d", psPacket->Money );
	INFO( "PacketRepairNPCShop - Server vs Client diff: %d", abs( l_RepairPrice - psPacket->Money ) );

	if ( abs( l_RepairPrice - psPacket->Money ) > 1 )
	{
		WARN( "Repair price is different! Server: %d, Client: %d", l_RepairPrice, psPacket->Money );
	}

#endif

    if ((l_RepairPrice != psPacket->Money && abs(l_RepairPrice - psPacket->Money) > 5000) || l_RepairPrice == std::numeric_limits<int>::max())
    {
		LOGERROR("Player(%s) trying to repair item %s (%d) at a price of %d gold instead of %d gold",
            pcUser->pcUserData->szCharacterName, psPacket->sItemData.sItem.szItemName, psPacket->sItemData.sItem.sItemID.ToInt(), psPacket->Money, l_RepairPrice);

        PacketLogCheat sPacket;
        sPacket.iCheatID    = CHEATLOGID_GenerateGoldHack;
        sPacket.LParam      = l_RepairPrice;
        sPacket.SParam      = psPacket->Money;
        sPacket.WxParam     = 12;
        LOGSERVER->OnLogCheat(pcUser->pcUserData, &sPacket);

		if ( pcUser->GetGameLevel() != GAMELEVEL_Four )
		{
			NETSERVER->DisconnectUser( pcUser->pcUserData );

			SENDPACKETBLANK( USERDATATOUSER( pcUser->pcUserData ), PKTHDR_Disconnect );
			WNDPROC_SOCKETCLOSE( SOCKETSERVER->GetHWND(), pcUser->pcUserData->pcSocketData );
		}

        return FALSE;
    }
    else
    {
        /// Ignore negative money here because client doesn't have the right price
        if (l_RepairPrice > pcUser->pcUserData->GetGold())
            l_RepairPrice = pcUser->pcUserData->GetGold();

        psPacket->Money = l_RepairPrice;

        /// Save money
        USERSERVER->SubServerUserGold(pcUser->pcUserData, l_RepairPrice, WHEREID_RepairItem);
    }

    return TRUE;
}

void ItemServer::HandlePacket( User * pcUser, PacketNewItemTimer * psPacket )
{
	DWORD dwTime = 0;

	psPacket->dwTimeLeft = psPacket->dwTimeTotal = 0;

	//For Delete...
	int iOldItemID = psPacket->iItemID;

	//Hour
	switch ( psPacket->iItemID )
	{
		case ITEMID_HONEYDAMAGE:
		case ITEMID_HONEYSMART:
		case ITEMID_HONEYDEFENSE:
		case ITEMID_ThirdEye1H:
		case ITEMID_ExpUp1H:
		case ITEMID_PetSystem_1H:
			psPacket->dwTimeTotal = ITEMTIMERTIME_1H;
			break;

		//SERVER WIDE BUFFS
		case ITEMID_SixthSense1H:
		case ITEMID_SHPRegen:
		case ITEMID_SSPRegen:
		case ITEMID_SMPRegen:
		case ITEMID_SMPDown:
		case ITEMID_SSPDown:
		case ITEMID_SBonusDamage:
		case ITEMID_SBonusAbsorb:
		case ITEMID_SMoveSpeed:
		case ITEMID_SExpBuff:
		case ITEMID_SDropBuff:
		case ITEMID_IceResistance:
			psPacket->dwTimeTotal = ITEMTIMERTIME_1H;
			break;

		case ITEMID_SoccerHatSpeed:
			psPacket->dwTimeTotal = ITEMTIMERTIME_3H;
			break;

		case ITEMID_ChelseaSoccerTimed:
		case ITEMID_ManchesterSoccerTimed:
		case ITEMID_HONEYFAST:
			psPacket->dwTimeTotal = ITEMTIMERTIME_6H;
			break;

		case ITEMID_BigHeadLove:
		case ITEMID_BigHeadSadness:
		case ITEMID_BigHeadHappyness:
		case ITEMID_BigHeadShyness:
		case ITEMID_BigHeadAngry:
		case ITEMID_BigHeadSurprised:
		case ITEMID_BigHeadSensual:
			psPacket->dwTimeTotal = ITEMTIMERTIME_72H;
			break;

		case ITEMID_SheepHat:
		case ITEMID_GiraffeHat:
		case ITEMID_SoccerHat:
			psPacket->dwTimeTotal = ITEMTIMERTIME_72H;
			break;

		case ITEMID_MPDown12H:
		case ITEMID_MPUp12H:
		case ITEMID_SPDown12H:
		case ITEMID_HPUp12H:
		case ITEMID_CartolaHat:
		case ITEMID_WitchHat:
		case ITEMID_ChristmasGreenHat:
		case ITEMID_ChristmasRedHat:
		//case ITEMID_EasterSheepHat:
			psPacket->dwTimeTotal = ITEMTIMERTIME_12H;
			break;


		case ITEMID_ExpUp24H:
		case ITEMID_ThirdEye24H:
		case ITEMID_BigHeadValentine:
		case ITEMID_PhoenixFire:
		case ITEMID_PhoenixIce:
		case ITEMID_PhoenixHealing:
		case ITEMID_PhoenixLightning:
		case ITEMID_PhoenixFireSmall:
		case ITEMID_PhoenixIceSmall:
		case ITEMID_PhoenixHealingSmall:
		case ITEMID_PhoenixLightningSmall:
		case ITEMID_PetSystem_24H:
			psPacket->dwTimeTotal = ITEMTIMERTIME_24H;
			break;

		case ITEMID_ExpUp48H:
		case ITEMID_ThirdEye48H:
		case ITEMID_PetSystem_48H:
			psPacket->dwTimeTotal = ITEMTIMERTIME_48H;
			break;

		case ITEMID_ExpUp72H:
		case ITEMID_ThirdEye72H:
		case ITEMID_GravityStone72H:
		case ITEMID_PhoenixEgg:
		case ITEMID_PetSystem_72H:
			psPacket->dwTimeTotal = ITEMTIMERTIME_72H;
			break;

		default:
			break;
	}

	//Used for PHPet...
	int iRandom = Dice::RandomI( 0, 100 );

	//Type
	switch ( psPacket->iItemID )
	{
		case ITEMID_HONEYDAMAGE:
		psPacket->iType = ITEMTIMERTYPE_HoneyDamage;
		break;

		case ITEMID_HONEYFAST:
		psPacket->iType = ITEMTIMERTYPE_HoneyFast;
		break;

		case ITEMID_HONEYSMART:
		psPacket->iType = ITEMTIMERTYPE_HoneySmart;
		break;

		case ITEMID_HONEYDEFENSE:
		psPacket->iType = ITEMTIMERTYPE_HoneyDefense;
		break;

		case ITEMID_ExpUp1H:
		case ITEMID_ExpUp24H:
		case ITEMID_ExpUp48H:
		case ITEMID_ExpUp72H:
			psPacket->iType = ITEMTIMERTYPE_ExpUp;
			break;

		case ITEMID_ThirdEye1H:
		case ITEMID_ThirdEye24H:
		case ITEMID_ThirdEye48H:
		case ITEMID_ThirdEye72H:
			psPacket->iType = ITEMTIMERTYPE_ThirdEye;
			break;

		case ITEMID_SixthSense1H:
			psPacket->iType = ITEMTIMERTYPE_SixthSense;
			break;

		case ITEMID_SHPRegen:
			psPacket->iType = ITEMTIMERTYPE_SHPRegen;
			break;

		case ITEMID_SSPRegen:
			psPacket->iType = ITEMTIMERTYPE_SHPRegen;
			break;

		case ITEMID_SMPRegen:
			psPacket->iType = ITEMTIMERTYPE_SMPRegen;
			break;

		case ITEMID_SMPDown:
			psPacket->iType = ITEMTIMERTYPE_SMPDown;
			break;

		case ITEMID_SSPDown:
			psPacket->iType = ITEMTIMERTYPE_SSPDown;
			break;

		case ITEMID_SBonusDamage:
			psPacket->iType = ITEMTIMERTYPE_SBonusDamage;
			break;

		case ITEMID_SBonusAbsorb:
			psPacket->iType = ITEMTIMERTYPE_SBonusAbsorb;
			break;

		case ITEMID_SMoveSpeed:
			psPacket->iType = ITEMTIMERTYPE_SMoveSpeed;
			break;

		case ITEMID_SExpBuff:
			psPacket->iType = ITEMTIMERTYPE_SExpBuff;
			break;

		case ITEMID_SDropBuff:
			psPacket->iType = ITEMTIMERTYPE_SDropBuff;
			break;

		case ITEMID_MPDown12H:
			psPacket->iType = ITEMTIMERTYPE_MPDown;
			break;

		case ITEMID_SPDown12H:
			psPacket->iType = ITEMTIMERTYPE_SPDown;
			break;

		case ITEMID_HPUp12H:
			psPacket->iType = ITEMTIMERTYPE_HPUp;
			break;

		case ITEMID_MPUp12H:
			psPacket->iType = ITEMTIMERTYPE_MPUp;
			break;

		case ITEMID_IceResistance:
			psPacket->iType = ITEMTIMERTYPE_IceResistanceBuff;
			break;

		case ITEMID_GravityStone72H:
			psPacket->iType = ITEMTIMERTYPE_WeightStone;
			break;

		case ITEMID_CartolaHat:
			psPacket->iType = ITEMTIMERTYPE_CartolaHat;
			break;

		case ITEMID_WitchHat:
			psPacket->iType = ITEMTIMERTYPE_WitchHat;
			break;

		case ITEMID_SheepHat:
			psPacket->iType = ITEMTIMERTYPE_SheepHat;
			break;

		//case ITEMID_EasterSheepHat:
			psPacket->iType = ITEMTIMERTYPE_EasterSheepHat;
			break;

		case ITEMID_GiraffeHat:
			psPacket->iType = ITEMTIMERTYPE_GiraffeHat;
			break;

		case ITEMID_SoccerHat:
			psPacket->iType = ITEMTIMERTYPE_SoccerHat;
			break;

		case ITEMID_SoccerHatSpeed:
			psPacket->iType = ITEMTIMERTYPE_SoccerHatSpeed;
			break;

		case ITEMID_ChelseaSoccerTimed:
			psPacket->iType = ITEMTIMERTYPE_ChelseaSoccer;
			break;

		case ITEMID_ManchesterSoccerTimed:
			psPacket->iType = ITEMTIMERTYPE_ManchesterSoccer;
			break;

		case ITEMID_ChristmasRedHat:
			psPacket->iType = ITEMTIMERTYPE_ChristmasRedHat;
			break;

		case ITEMID_ChristmasGreenHat:
			psPacket->iType = ITEMTIMERTYPE_ChristmasGreenHat;
			break;

		case ITEMID_BigHeadLove:
			psPacket->iType = ITEMTIMERTYPE_BigHeadLove;
			break;

		case ITEMID_BigHeadValentine:
			psPacket->iType = ITEMTIMERTYPE_BigHeadValentine;

		case ITEMID_BigHeadSadness:
			psPacket->iType = ITEMTIMERTYPE_BigHeadSadness;
			break;

		case ITEMID_BigHeadHappyness:
			psPacket->iType = ITEMTIMERTYPE_BigHeadHappiness;
			break;

		case ITEMID_BigHeadShyness:
			psPacket->iType = ITEMTIMERTYPE_BigHeadShyness;
			break;

		case ITEMID_BigHeadAngry:
			psPacket->iType = ITEMTIMERTYPE_BigHeadAngry;
			break;

		case ITEMID_BigHeadSurprised:
			psPacket->iType = ITEMTIMERTYPE_BigHeadSurprised;
			break;

		case ITEMID_BigHeadSensual:
			psPacket->iType = ITEMTIMERTYPE_BigHeadSensual;
			break;

		case ITEMID_PhoenixEgg:
			psPacket->iType			= ITEMTIMERTYPE_PhoenixFire;
			psPacket->iItemID		= ITEMID_PhoenixFire;

			if ( iRandom < 25 )
			{
				psPacket->iType		= ITEMTIMERTYPE_PhoenixIce;
				psPacket->iItemID	= ITEMID_PhoenixIce;
			}
			else if ( iRandom < 50 )
			{
				psPacket->iType		= ITEMTIMERTYPE_PhoenixLightning;
				psPacket->iItemID	= ITEMID_PhoenixLightning;
			}
			else if ( iRandom < 75 )
			{
				psPacket->iType		= ITEMTIMERTYPE_PhoenixHeal;
				psPacket->iItemID	= ITEMID_PhoenixHealing;
			}
			break;

		case ITEMID_PhoenixIce:
			psPacket->iType = ITEMTIMERTYPE_PhoenixIce;
			psPacket->iItemID = ITEMID_PhoenixIce;
			break;

		case ITEMID_PhoenixHealing:
			psPacket->iType = ITEMTIMERTYPE_PhoenixHeal;
			psPacket->iItemID = ITEMID_PhoenixHealing;
			break;

		case ITEMID_PhoenixLightning:
			psPacket->iType = ITEMTIMERTYPE_PhoenixLightning;
			psPacket->iItemID = ITEMID_PhoenixLightning;
			break;

		case ITEMID_PhoenixFire:
			psPacket->iType = ITEMTIMERTYPE_PhoenixFire;
			psPacket->iItemID = ITEMID_PhoenixFire;
			break;

		case ITEMID_PhoenixIceSmall:
			psPacket->iType = ITEMTIMERTYPE_PhoenixIceSmall;
			psPacket->iItemID = ITEMID_PhoenixIceSmall;
			break;

		case ITEMID_PhoenixHealingSmall:
			psPacket->iType = ITEMTIMERTYPE_PhoenixHealingSmall;
			psPacket->iItemID = ITEMID_PhoenixHealingSmall;
			break;

		case ITEMID_PhoenixLightningSmall:
			psPacket->iType = ITEMTIMERTYPE_PhoenixLightningSmall;
			psPacket->iItemID = ITEMID_PhoenixLightningSmall;
			break;

		case ITEMID_PhoenixFireSmall:
			psPacket->iType = ITEMTIMERTYPE_PhoenixFireSmall;
			psPacket->iItemID = ITEMID_PhoenixFireSmall;
			break;
		case ITEMID_PetSystem_1H:
		case ITEMID_PetSystem_24H:
		case ITEMID_PetSystem_48H:
		case ITEMID_PetSystem_72H:
			psPacket->iType = ITEMTIMERTYPE_PetSystem;
			break;
		default:
			break;
	}



	//Have Time?
	if ( psPacket->dwTimeTotal > 0 )
	{
		//Update Time Left
		psPacket->dwTimeLeft = psPacket->dwTimeTotal;

		//Delete Item From Server
		//JLM - setting to false as it's causing too many false negatives
		if (false && !DeleteItemInventory( pcUser->pcUserData, iOldItemID, psPacket->iChk1, psPacket->iChk2 ))
		{
			//Item Name
			char szItemName[32] = { 0 };
			if ( FindItemName( iOldItemID, szItemName ) )
			{
				//Log It...
				PacketLogCheat sPacket;
				sPacket.iCheatID	= CHEATLOGID_ItemTimerFakeInventory;
				sPacket.LParam		= iOldItemID;
				sPacket.SParam		= psPacket->iChk1;
				sPacket.EParam		= psPacket->iChk2;
				sPacket.LxParam		= (int)szItemName;
				LOGSERVER->OnLogCheat( pcUser->pcUserData, &sPacket );
			}

			return;
		}

		if (    psPacket->iType == ITEMTIMERTYPE_SHPRegen
			 || psPacket->iType == ITEMTIMERTYPE_SSPRegen
			 || psPacket->iType == ITEMTIMERTYPE_SMPRegen
			 || psPacket->iType == ITEMTIMERTYPE_SMPDown
			 || psPacket->iType == ITEMTIMERTYPE_SSPDown
			 || psPacket->iType == ITEMTIMERTYPE_SBonusDamage
			 || psPacket->iType == ITEMTIMERTYPE_SBonusAbsorb
			 || psPacket->iType == ITEMTIMERTYPE_SMoveSpeed
			 || psPacket->iType == ITEMTIMERTYPE_SExpBuff
			 || psPacket->iType == ITEMTIMERTYPE_SDropBuff    )
		{
			std::string buffName = "";

			switch ( psPacket->iType )
			{
				case ITEMTIMERTYPE_SHPRegen		   :  buffName = "HP Regen"; break;
				case ITEMTIMERTYPE_SSPRegen		   :  buffName = "SP Regen"; break;
				case ITEMTIMERTYPE_SMPRegen		   :  buffName = "MP Regen"; break;
				case ITEMTIMERTYPE_SMPDown		   :  buffName = "MP Down"; break;
				case ITEMTIMERTYPE_SSPDown		   :  buffName = "SP Down"; break;
				case ITEMTIMERTYPE_SBonusDamage	   :  buffName = "Damage"; break;
				case ITEMTIMERTYPE_SBonusAbsorb	   :  buffName = "Absorb"; break;
				case ITEMTIMERTYPE_SMoveSpeed	   :  buffName = "Move Speed"; break;
				case ITEMTIMERTYPE_SExpBuff		   :  buffName = "Exp"; break;
				case ITEMTIMERTYPE_SDropBuff	   :  buffName = "Drop"; break;
			}

			for ( int i = 0; i < PLAYERS_MAX; i++ )
			{
				User * pcUserLoop = USERDATATOUSER(&USERSDATA[i]);

				if ( pcUserLoop == NULL || pcUserLoop->IsValidAndInGame() == FALSE )
					continue;

				//Add To User
				if ( AddItemTimer(pcUserLoop, psPacket->iType, psPacket->iItemID, psPacket->dwTimeTotal) )
				{
					//Notify New Item to User...
					SENDPACKET(pcUserLoop, psPacket);

					char* pszCharName = CHARACTERSERVER->GetCharacterName(pcUserLoop->pcUserData);
					CHATSERVER->SendTitleBox(pcUserLoop, "Player %s activated a server wide %s buff!", CHARACTERSERVER->GetCharacterName(pcUser->pcUserData), buffName.c_str());
				}
			}
		}
		else
		{
			//Add To User
			if ( AddItemTimer(pcUser, psPacket->iType, psPacket->iItemID, psPacket->dwTimeTotal) )
			{
				//Notify New Item to User...
				SENDPACKET(pcUser, psPacket);
			}
		}
	}
}

void ItemServer::HandlePacket( User * pcUser, PacketCancelItemTimer * psPacket )
{
	for ( int i = 0; i < PREMIUM_ITEM_TOTAL; i++ )
	{
		ItemPremium * psItem = &pcUser->sPremiumData.saItemList[i];

		if ( (psItem->eType == psPacket->iType) && (psItem->dwCode == psPacket->iItemID) )
		{
			OnPremiumItemCancelledOrExpired( pcUser, psItem, FALSE );
			break;
		}
	}
}


void ItemServer::HandlePacket( User * pcUser, PacketOldHeadUse * psPacket )
{
	if ( psPacket->bUpdate )
	{
		SQLUpdateHeadModel (pcUser,
			pcUser->pcUserData->sCharacterData.Player.szHeadModel,
			pcUser->iCharacterID );
	}
	else
	{
		auto szHead = SQLGetOldHeadModel ( pcUser );

		STRINGCOPY ( pcUser->pcUserData->sCharacterData.Player.szHeadModel, szHead.c_str() );
		STRINGCOPY ( psPacket->szHeadModel, szHead.c_str() );

		SENDPACKET ( pcUser, psPacket );

	}
}

void ItemServer::HandlePacket( User * pcUser, PacketSlotItemHandle * psPacket )
{
	char * pszCharacterName = CHARACTERSERVER->GetCharacterName( pcUser->pcUserData );

	char * pszAction = "Get";
	char * pszWhere = "null";

	int iLogID = ITEMLOGID_GetPerfectize;

	BOOL bPut = (psPacket->iTypeHandle == ESlotTypeHandle::ITEMSLOTHANDLE_Put) ? TRUE : FALSE;

	if ( psPacket->iTypeHandle == ESlotTypeHandle::ITEMSLOTHANDLE_Put )
		pszAction = "Put";

	if ( psPacket->iTypeWhere == ESlotTypeHandleWhere::ITEMSLOTHANDLEWHERE_Perfectize )
	{
		pszWhere = "Perfectize";

		iLogID = ITEMLOGID_GetPerfectize;
		if ( bPut )
			iLogID = ITEMLOGID_PutPerfectize;
	}
	else if ( psPacket->iTypeWhere == ESlotTypeHandleWhere::ITEMSLOTHANDLEWHERE_Swap )
	{
		pszWhere = "Swap";

		iLogID = ITEMLOGID_GetSwap;
		if ( bPut )
			iLogID = ITEMLOGID_PutSwap;
	}

	char szItemName[32] = { 0 };
	ITEMSERVER->FindItemName( psPacket->iItemID, szItemName );

	LOGSERVER->OnLogItemEx( pcUser, iLogID, "[%s](%s) %s item on %s ( [%s][0x%08X] %d x %d )",
		pcUser->pcUserData->szAccountName, pszCharacterName, pszAction, pszWhere, szItemName, psPacket->iItemID, psPacket->iChk1, psPacket->iChk2 );
}


void ItemServer::HandlePacket( User * pcUser, PacketBuyNPCShop * psPacket )
{
	if ( pcUser && pcUser->pcUserData )
	{
		if ( GAME_SERVER )
		{
			if ( psPacket->iUniqueID != (pcUser->GetID() ^ CRC_ITEMSHOP_UNIQUEID) )
			{
				//Log It...
				PacketLogCheat sPacket;
				sPacket.iCheatID = CHEATLOGID_ItemFakeBoughtShopID;
				sPacket.LParam = psPacket->sItemData.sItem.sItemID.ToInt();
				sPacket.SParam = psPacket->sItemData.sItem.iChk1;
				sPacket.EParam = psPacket->sItemData.sItem.iChk2;
				STRINGCOPY( sPacket.szBuffer1, psPacket->sItemData.sItem.szItemName );
				sPacket.WxParam = psPacket->iUniqueID;
				sPacket.SxParam = (pcUser->GetID() ^ CRC_ITEMSHOP_UNIQUEID);
				LOGSERVER->OnLogCheat( pcUser->pcUserData, &sPacket );
				psPacket->iHeader = 0;
				return;
			}

			bool bValidItem = false;

			int iNPCID = psPacket->iNPCID ^ CRC_ITEMSHOP_UNIQUEID;
			UnitData * pcUnitData = UNITSERVER->GetUnit( iNPCID );
			if ( pcUnitData && pcUnitData->bActive )
			{
				if ( pcUnitData->sUnitInfo.SellAttackItemCount )
				{
					for ( int i = 0; i < pcUnitData->sUnitInfo.SellAttackItemCount; i++ )
					{
						DefinitionItem * psDefinitionItem = (DefinitionItem *)pcUnitData->sUnitInfo.SellAttackItem[i];
						if ( psDefinitionItem )
						{
							if ( psDefinitionItem->sItem.sItemID.ToItemID() == psPacket->sItemData.sItem.sItemID.ToItemID() )
							{
								bValidItem = true;
								break;
							}
						}
					}
				}

				if ( bValidItem == false )
				{
					if ( pcUnitData->sUnitInfo.SellDefenceItemCount )
					{
						for ( int i = 0; i < pcUnitData->sUnitInfo.SellDefenceItemCount; i++ )
						{
							DefinitionItem * psDefinitionItem = (DefinitionItem *)pcUnitData->sUnitInfo.SellDefenceItem [i] ;
							if ( psDefinitionItem )
							{
								if ( psDefinitionItem->sItem.sItemID.ToItemID() == psPacket->sItemData.sItem.sItemID.ToItemID() )
								{
									bValidItem = true;
									break;
								}
							}
						}
					}
				}

				if ( bValidItem == false )
				{
					if ( pcUnitData->sUnitInfo.SellEtcItemCount )
					{
						for ( int i = 0; i < pcUnitData->sUnitInfo.SellEtcItemCount; i++ )
						{
							DefinitionItem * psDefinitionItem = (DefinitionItem *)pcUnitData->sUnitInfo.SellEtcItem[i];
							if ( psDefinitionItem )
							{
								if ( psDefinitionItem->sItem.sItemID.ToItemID() == psPacket->sItemData.sItem.sItemID.ToItemID() )
								{
									bValidItem = true;
									break;
								}
							}
						}
					}
				}

				if ( bValidItem == false )
					if ( (psPacket->sItemData.sItem.sItemID.ToItemBase() == ITEMBASE_Potion) || (psPacket->sItemData.sItem.sItemID.ToItemBase() == ITEMBASE_Core) )
						bValidItem = true;
			}

			if ( bValidItem == false )
			{
				PacketLogCheat sPacket;
				sPacket.iCheatID = CHEATLOGID_ItemFakeBoughtShopItem;
				sPacket.LParam = psPacket->sItemData.sItem.sItemID.ToInt();
				sPacket.SParam = psPacket->sItemData.sItem.iChk1;
				sPacket.EParam = psPacket->sItemData.sItem.iChk2;
				STRINGCOPY( sPacket.szBuffer1, psPacket->sItemData.sItem.szItemName );
				LOGSERVER->OnLogCheat( pcUser->pcUserData, &sPacket );
				psPacket->iHeader = 0;
				return;
			}

			if ( LOGIN_SERVER == FALSE )
				psPacket->iHeader = 0;
		}
	}
}


BOOL ItemServer::SendItemShopList( User * pcUser, UnitData * pcUnitData )
{
	PacketItemShopList sPacket;
	sPacket.iLength = sizeof( PacketItemShopList );
	sPacket.iHeader = PKTHDR_Shop_ItemList;
	sPacket.iUniqueID = (pcUser->GetID() ^ CRC_ITEMSHOP_UNIQUEID);
	sPacket.iNPCID = (pcUnitData->iID ^ CRC_ITEMSHOP_UNIQUEID);


	if ( pcUnitData->sUnitInfo.SellAttackItemCount )
	{
		int iLength = 32;
		sPacket.iItemCount = 0;

		int iBufferLength = 0;

		for ( int i = 0; i < pcUnitData->sUnitInfo.SellAttackItemCount; i++ )
		{
			DefinitionItem * psDefinitionItem = (DefinitionItem *)pcUnitData->sUnitInfo.SellAttackItem[i];

			if ( psDefinitionItem )
			{
				if ( (psDefinitionItem->DefCompressDataLen + iLength) < MAX_PKTSIZ)
				{
					CopyMemory( sPacket.szBuffer + iBufferLength, psDefinitionItem->DefCompressData, psDefinitionItem->DefCompressDataLen );
					iLength += psDefinitionItem->DefCompressDataLen;
					iBufferLength += psDefinitionItem->DefCompressDataLen;
					sPacket.iItemCount++;
				}
			}
		}

		sPacket.iLength = iLength;
		sPacket.iShopType = 1;
		SENDPACKET( pcUser, &sPacket );
	}

	if ( pcUnitData->sUnitInfo.SellDefenceItemCount )
	{
		int iLength = 32;
		sPacket.iItemCount = 0;

		int iBufferLength = 0;

		for ( int i = 0; i < pcUnitData->sUnitInfo.SellDefenceItemCount; i++ )
		{
			DefinitionItem * psDefinitionItem = (DefinitionItem *)pcUnitData->sUnitInfo.SellDefenceItem[i];
			if ( psDefinitionItem )
			{
				if ( (psDefinitionItem->DefCompressDataLen + iLength) < MAX_PKTSIZ)
				{
					CopyMemory( sPacket.szBuffer + iBufferLength, psDefinitionItem->DefCompressData, psDefinitionItem->DefCompressDataLen );
					iLength += psDefinitionItem->DefCompressDataLen;
					iBufferLength += psDefinitionItem->DefCompressDataLen;
					sPacket.iItemCount++;
				}
			}
		}

		sPacket.iLength = iLength;
		sPacket.iShopType = 2;
		SENDPACKET( pcUser, &sPacket );
	}

	if ( pcUnitData->sUnitInfo.SellEtcItemCount )
	{
		int iLength = 32;
		sPacket.iItemCount = 0;

		int iBufferLength = 0;

		for ( int i = 0; i < pcUnitData->sUnitInfo.SellEtcItemCount; i++ )
		{
			DefinitionItem * psDefinitionItem = (DefinitionItem *)pcUnitData->sUnitInfo.SellEtcItem[i];
			if ( psDefinitionItem )
			{
				if ( (psDefinitionItem->DefCompressDataLen + iLength) < MAX_PKTSIZ)
				{
					CopyMemory( sPacket.szBuffer + iBufferLength, psDefinitionItem->DefCompressData, psDefinitionItem->DefCompressDataLen );
					iLength += psDefinitionItem->DefCompressDataLen;
					iBufferLength += psDefinitionItem->DefCompressDataLen;
					sPacket.iItemCount++;
				}
			}
		}

		sPacket.iLength = iLength;
		sPacket.iShopType = 3;
		SENDPACKET( pcUser, &sPacket );
	}

	return TRUE;
}

bool ItemServer::IsItemTimerFreezedMap( EMapID iMapID, EItemTimerType iType )
{
	//special cases, don't freeze these timers.. ever
	switch ( iType )
	{
		case EItemTimerType::ITEMTIMERTYPE_SHPRegen:
		case EItemTimerType::ITEMTIMERTYPE_SSPRegen:
		case EItemTimerType::ITEMTIMERTYPE_SMPRegen:
		case EItemTimerType::ITEMTIMERTYPE_SMPDown:
		case EItemTimerType::ITEMTIMERTYPE_SSPDown:
		case EItemTimerType::ITEMTIMERTYPE_SBonusDamage:
		case EItemTimerType::ITEMTIMERTYPE_SBonusAbsorb:
		case EItemTimerType::ITEMTIMERTYPE_SMoveSpeed:
		case EItemTimerType::ITEMTIMERTYPE_SExpBuff:
		case EItemTimerType::ITEMTIMERTYPE_SDropBuff:
			return false;
	}

	bool bRet = false;

	//Otherwise if in town, freeze everything else (including force orb)
	if ( iMapID == MAPID_RicartenTown ||
		 iMapID == MAPID_PillaiTown ||
		 iMapID == MAPID_Eura ||
		 iMapID == MAPID_Atlantis ||
		 iMapID == MAPID_NaviskoTown )
	{
		return true;
	}

	switch ( iType )
	{
		case ITEMTIMERTYPE_PetSystem:
		case ITEMTIMERTYPE_ExpUp:
		case ITEMTIMERTYPE_ThirdEye:
		case ITEMTIMERTYPE_HoneyFast: // < exp
		case ITEMTIMERTYPE_SixthSense: // 10% chance additional drop
			if ((iMapID == MAPID_BattleTown) || (iMapID == MAPID_BlessCastle) || (iMapID == MAPID_GhostCastle) || (iMapID == MAPID_Bellatra) )
				return true;

		case ITEMTIMERTYPE_Phoenix:
		case ITEMTIMERTYPE_PhoenixFire:
		case ITEMTIMERTYPE_PhoenixIce:
		case ITEMTIMERTYPE_PhoenixLightning:
		case ITEMTIMERTYPE_PhoenixHeal:
			if ( (iMapID == MAPID_BattleTown) || (iMapID == MAPID_BlessCastle) || (iMapID == MAPID_Bellatra) )
				return true;

		case ITEMTIMERTYPE_MPDown:
		case ITEMTIMERTYPE_SPDown:
		case ITEMTIMERTYPE_HPUp:
		case ITEMTIMERTYPE_MPUp:
		case ITEMTIMERTYPE_BigHead:
		case ITEMTIMERTYPE_WeightStone:
		case ITEMTIMERTYPE_MatureStone:
		case ITEMTIMERTYPE_SkillStone:
		case ITEMTIMERTYPE_CartolaHat:
		case ITEMTIMERTYPE_WitchHat:
		case ITEMTIMERTYPE_SheepHat:
		case ITEMTIMERTYPE_GiraffeHat:
		case ITEMTIMERTYPE_SoccerHat:
		case ITEMTIMERTYPE_SoccerHatSpeed:
		case ITEMTIMERTYPE_BigHeadHappiness:
		case ITEMTIMERTYPE_BigHeadLove:
		case ITEMTIMERTYPE_BigHeadSadness:
		case ITEMTIMERTYPE_BigHeadShyness:
		case ITEMTIMERTYPE_BigHeadAngry:
		case ITEMTIMERTYPE_BigHeadSurprised:
		case ITEMTIMERTYPE_BigHeadSensual:
		case ITEMTIMERTYPE_BCBuff:
		case ITEMTIMERTYPE_PumpkinHat:
		case ITEMTIMERTYPE_HoneyDamage:
		case ITEMTIMERTYPE_HoneySmart:
		case ITEMTIMERTYPE_HoneyDefense:
		case ITEMTIMERTYPE_IceResistanceBuff:
			if ( ( iMapID == MAPID_Bellatra) )
				return true;

		default:
			break;
	}

	return false;
}


void ItemServer::OnForceOrbActivated( User * pcUser, int iForceOrbIndex, EItemID eItemId )
{
	if ( pcUser == NULL )
		return;

	DEBUG( "OnForceOrbActivated %d", iForceOrbIndex );

	//DEBUG( "OnForceOrbActivated - index = %d", iForceOrbIndex );

	//13 = up to and including Oredo Force
	if ( iForceOrbIndex < 0 || iForceOrbIndex > 13 )
	{
		WARN( "Force orb index %d is out of range", iForceOrbIndex );
		return;
	}

	CreatePremium( pcUser, eItemId, ITEMTIMERTYPE_ForceOrb, ITEMSERVER->GetForceDurationByIndex( iForceOrbIndex ), true );

	//std::cout << "added: " << pcUserData->ForceOrbCode << ", Duration: " << iDurationSecond  << std::endl;

	//ITEMSERVER->SQLDeleteItemTimerByTimerType ( pcUser, ITEMTIMERTYPE_ForceOrb );	//delete existing timer if there is one
	//ITEMSERVER->SQLAddItemTimer ( CHARACTERSERVER->GetCharacterName(pcUser->pcUserData), (int)ITEMTIMERTYPE_ForceOrb, pcUser->eForceOrbItemID, iDurationSecond );



	//lpPlayInfo->dwForceOrb_SaveCode = lpTransCommandEx->WParam;
	//lpPlayInfo->dwForceOrb_SaveTime = dwPlayServTime + ForceDuration[cnt] * 1000;
	//lpPlayInfo->dwForceOrb_SaveDamage = ForceDamagePercentTable[cnt];

}


void ItemServer::UpdateForceOrb( User * pcUser, EItemTimerType iType, EItemID eItemId )
{
	if ( iType != ITEMTIMERTYPE_ForceOrb )
		return;

	DEBUG( "UpdateForceOrb" );

	int lowId = eItemId & 0x0000FFFF;
	int forceIndex = ( lowId >> 8 ) - 1;

	pcUser->eForceOrbItemID					= eItemId;
	pcUser->iForceOrbDamage					= ForceDamageTable[forceIndex];
	pcUser->iForceOrbDamageExtraPercentage	= ForceDamagePercentTable[forceIndex];
	pcUser->bForceOrbActive					= TRUE;
}

void ItemServer::RemoveForceOrb( User * pcUser )
{
	DEBUG( "RemoveForceOrb" );

	pcUser->eForceOrbItemID					= EItemID::ITEMID_None;
	pcUser->iForceOrbDamage					= 0;
	pcUser->iForceOrbDamageExtraPercentage	= 0;
	pcUser->bForceOrbActive					= FALSE;
}

void ItemServer::UpdateUserEXPBuff( User* pcUser )
{
	int iExpBuffValue = 0;
	const int iMaxValue = 45;

	for ( int i = 0; i < PREMIUM_ITEM_TOTAL; i++ )
	{
		//Find New Item Slot
		if ( pcUser->sPremiumData.saItemList[i].eType == ITEMTIMERTYPE_ExpUp )
		{
			iExpBuffValue += 30;
		}
		else if ( pcUser->sPremiumData.saItemList[i].eType == ITEMTIMERTYPE_HoneyFast )
		{
			iExpBuffValue += 10;
		}
		else if ( pcUser->sPremiumData.saItemList[i].eType == ITEMTIMERTYPE_SExpBuff )
		{
			iExpBuffValue += 5;
		}
	}

	if ( iExpBuffValue > iMaxValue )
	{
		WARN("pcUser->iEXpPotion %d exceeded the max value of %d for %s", iExpBuffValue, iMaxValue, CHARACTERSERVER->GetCharacterName(pcUser->pcUserData));
		iExpBuffValue = iMaxValue;
	}

	if ( pcUser->iEXPPotion != iExpBuffValue )
	{
		INFO("Update user %s exp buff value: %d -> %d", CHARACTERSERVER->GetCharacterName(pcUser->pcUserData), pcUser->iEXPPotion, iExpBuffValue);
		pcUser->iEXPPotion = iExpBuffValue;
		NETSERVER->SendEXPPotion(pcUser->pcUserData, pcUser->iEXPPotion);
	}
}


void refMem ( DWORD addr, DWORD value )
{
	DWORD old;
	VirtualProtect ( (void *)addr, 4, PAGE_EXECUTE_READWRITE, &old );
	*(DWORD *)addr = value;
};

/*
Altera todas as referencias para a nova a Tabela;
*/
void refIT ()
{
	refMem ( 0x00415E17, (DWORD)allocIT );
	refMem ( 0x00417090, (DWORD)allocIT );
	refMem ( 0x0041E5F9, (DWORD)allocIT );
	refMem ( 0x0041FD7B, (DWORD)allocIT );
	refMem ( 0x0041FDBB, (DWORD)allocIT );
	refMem ( 0x0042A15C, (DWORD)allocIT );
	refMem ( 0x0042E067, (DWORD)allocIT );
	refMem ( 0x0042E0A3, (DWORD)allocIT );
	refMem ( 0x0043686C, (DWORD)allocIT );
	refMem ( 0x00438A7C, (DWORD)allocIT );
	refMem ( 0x00438D6E, (DWORD)allocIT );
	refMem ( 0x00438F04, (DWORD)allocIT );
	refMem ( 0x0043901F, (DWORD)allocIT );
	refMem ( 0x004390C5, (DWORD)allocIT );
	refMem ( 0x00439165, (DWORD)allocIT );
	refMem ( 0x00442A1F, (DWORD)allocIT );
	refMem ( 0x0044FCE9, (DWORD)allocIT );
	refMem ( 0x00480CCF, (DWORD)allocIT );
	refMem ( 0x00480D45, (DWORD)allocIT );
	refMem ( 0x00485918, (DWORD)allocIT );
	refMem ( 0x00485994, (DWORD)allocIT );
	refMem ( 0x0048E63A, (DWORD)allocIT );
	refMem ( 0x0048E65E, (DWORD)allocIT );
	refMem ( 0x0048E844, (DWORD)allocIT );
	refMem ( 0x0048E866, (DWORD)allocIT );
	refMem ( 0x0048EE0A, (DWORD)allocIT );
	refMem ( 0x0048EE2E, (DWORD)allocIT );
	refMem ( 0x00498945, (DWORD)allocIT );
	refMem ( 0x0049896F, (DWORD)allocIT );
	refMem ( 0x004A47D3, (DWORD)allocIT );
	refMem ( 0x004A4826, (DWORD)allocIT );
	refMem ( 0x004A4870, (DWORD)allocIT );
	refMem ( 0x004A49CD, (DWORD)allocIT );
	refMem ( 0x004A4A3C, (DWORD)allocIT );
	refMem ( 0x004BB844, (DWORD)allocIT );
	refMem ( 0x004BB8FC, (DWORD)allocIT );
	refMem ( 0x004BB94F, (DWORD)allocIT );
	refMem ( 0x004BB97C, (DWORD)allocIT );
	refMem ( 0x004BE3D7, (DWORD)allocIT );
	refMem ( 0x004BE407, (DWORD)allocIT );
	refMem ( 0x004C1954, (DWORD)allocIT );
	refMem ( 0x004C1E30, (DWORD)allocIT );
	refMem ( 0x0054801D, (DWORD)allocIT );
	refMem ( 0x005594F3, (DWORD)allocIT );
	refMem ( 0x0055B672, (DWORD)allocIT );
	refMem ( 0x005819FB, (DWORD)allocIT );
	refMem ( 0x0043683B, (DWORD)allocIT + 0x24 );
	refMem ( 0x00438A50, (DWORD)allocIT + 0x24 );
	refMem ( 0x00438D38, (DWORD)allocIT + 0x24 );
	refMem ( 0x00438ED3, (DWORD)allocIT + 0x24 );
	refMem ( 0x00438FE8, (DWORD)allocIT + 0x24 );
	refMem ( 0x00439092, (DWORD)allocIT + 0x24 );
	refMem ( 0x00439132, (DWORD)allocIT + 0x24 );
	refMem ( 0x0043B3E4, (DWORD)allocIT + 0x24 );
	refMem ( 0x0044FCB6, (DWORD)allocIT + 0x24 );
	refMem ( 0x00480CEE, (DWORD)allocIT + 0x24 );
	refMem ( 0x00480D6B, (DWORD)allocIT + 0x24 );
	refMem ( 0x0048594E, (DWORD)allocIT + 0x24 );
	refMem ( 0x0048E686, (DWORD)allocIT + 0x24 );
	refMem ( 0x0048E88A, (DWORD)allocIT + 0x24 );
	refMem ( 0x0048EE55, (DWORD)allocIT + 0x24 );
	refMem ( 0x004A47F5, (DWORD)allocIT + 0x24 );
	refMem ( 0x004A4849, (DWORD)allocIT + 0x24 );
	refMem ( 0x004A49F8, (DWORD)allocIT + 0x24 );
	refMem ( 0x004BB926, (DWORD)allocIT + 0x24 );
	refMem ( 0x004BE248, (DWORD)allocIT + 0x24 );
	refMem ( 0x004BE300, (DWORD)allocIT + 0x24 );
	refMem ( 0x004C1976, (DWORD)allocIT + 0x24 );
	refMem ( 0x004C1E5E, (DWORD)allocIT + 0x24 );
	refMem ( 0x005594C1, (DWORD)allocIT + 0x24 );
	refMem ( 0x00485982, (DWORD)allocIT + 0x34 );
	refMem ( 0x0048E75E, (DWORD)allocIT + 0x34 );
	refMem ( 0x0048E95B, (DWORD)allocIT + 0x34 );
	refMem ( 0x004C1E52, (DWORD)allocIT + 0x34 );
	refMem ( 0x004C1EEF, (DWORD)allocIT + 0x34 );
	refMem ( 0x0048598B, (DWORD)allocIT + 0x38 );
	refMem ( 0x0048E76A, (DWORD)allocIT + 0x38 );
	refMem ( 0x0048E961, (DWORD)allocIT + 0x38 );
	refMem ( 0x004C1E58, (DWORD)allocIT + 0x38 );
	refMem ( 0x004C1EE3, (DWORD)allocIT + 0x38 );
	refMem ( 0x00480CF5, (DWORD)allocIT + 0x3C );
	refMem ( 0x00480D72, (DWORD)allocIT + 0x3C );
	refMem ( 0x00485955, (DWORD)allocIT + 0x3C );
	refMem ( 0x0048E68D, (DWORD)allocIT + 0x3C );
	refMem ( 0x0048E891, (DWORD)allocIT + 0x3C );
	refMem ( 0x0048EE5C, (DWORD)allocIT + 0x3C );
	refMem ( 0x004A47FC, (DWORD)allocIT + 0x3C );
	refMem ( 0x004A4850, (DWORD)allocIT + 0x3C );
	refMem ( 0x004A49FF, (DWORD)allocIT + 0x3C );
	refMem ( 0x004BB931, (DWORD)allocIT + 0x3C );
	refMem ( 0x004C197D, (DWORD)allocIT + 0x3C );
	refMem ( 0x004C1E65, (DWORD)allocIT + 0x3C );
	refMem ( 0x0041E628, (DWORD)allocIT + 0x7C );
	refMem ( 0x00490A4D, (DWORD)allocIT + 0x7C );
	refMem ( 0x00548042, (DWORD)allocIT + 0x7C );
	refMem ( 0x0055B798, (DWORD)allocIT + 0x7C );
	refMem ( 0x004BB865, (DWORD)allocIT + 0x80 );
	refMem ( 0x00480D08, (DWORD)allocIT + 0xFC );
	refMem ( 0x00480D22, (DWORD)allocIT + 0xFC );
	refMem ( 0x00480D28, (DWORD)allocIT + 0xFC );
	refMem ( 0x00480D85, (DWORD)allocIT + 0xFC );
	refMem ( 0x00480D9F, (DWORD)allocIT + 0xFC );
	refMem ( 0x00480DA5, (DWORD)allocIT + 0xFC );
	refMem ( 0x00480DD5, (DWORD)allocIT + 0xFC );
	refMem ( 0x004851AF, (DWORD)allocIT + 0xFC );
	refMem ( 0x00485976, (DWORD)allocIT + 0xFC );
	refMem ( 0x0048E6A7, (DWORD)allocIT + 0xFC );
	refMem ( 0x0048E6C4, (DWORD)allocIT + 0xFC );
	refMem ( 0x0048E71A, (DWORD)allocIT + 0xFC );
	refMem ( 0x0048E764, (DWORD)allocIT + 0xFC );
	refMem ( 0x0048E781, (DWORD)allocIT + 0xFC );
	refMem ( 0x0048E8AB, (DWORD)allocIT + 0xFC );
	refMem ( 0x0048E8C8, (DWORD)allocIT + 0xFC );
	refMem ( 0x0048E918, (DWORD)allocIT + 0xFC );
	refMem ( 0x0048E967, (DWORD)allocIT + 0xFC );
	refMem ( 0x0048E97E, (DWORD)allocIT + 0xFC );
	refMem ( 0x0048EE76, (DWORD)allocIT + 0xFC );
	refMem ( 0x0048EE93, (DWORD)allocIT + 0xFC );
	refMem ( 0x0048EE99, (DWORD)allocIT + 0xFC );
	refMem ( 0x004A488C, (DWORD)allocIT + 0xFC );
	refMem ( 0x004A4892, (DWORD)allocIT + 0xFC );
	refMem ( 0x004A48BE, (DWORD)allocIT + 0xFC );
	refMem ( 0x004A48C4, (DWORD)allocIT + 0xFC );
	refMem ( 0x004A4A16, (DWORD)allocIT + 0xFC );
	refMem ( 0x004A4A30, (DWORD)allocIT + 0xFC );
	refMem ( 0x004A4A36, (DWORD)allocIT + 0xFC );
	refMem ( 0x004BB9C7, (DWORD)allocIT + 0xFC );
	refMem ( 0x004BB9F1, (DWORD)allocIT + 0xFC );
	refMem ( 0x004BBA11, (DWORD)allocIT + 0xFC );
	refMem ( 0x004BBA17, (DWORD)allocIT + 0xFC );
	refMem ( 0x004C1990, (DWORD)allocIT + 0xFC );
	refMem ( 0x004C19AA, (DWORD)allocIT + 0xFC );
	refMem ( 0x004C19CF, (DWORD)allocIT + 0xFC );
	refMem ( 0x004C1A03, (DWORD)allocIT + 0xFC );
	refMem ( 0x004C1A09, (DWORD)allocIT + 0xFC );
	refMem ( 0x004C1E88, (DWORD)allocIT + 0xFC );
	refMem ( 0x004C1EA2, (DWORD)allocIT + 0xFC );
	refMem ( 0x004C1EE9, (DWORD)allocIT + 0xFC );
	refMem ( 0x004C1F27, (DWORD)allocIT + 0xFC );
	refMem ( 0x004C1F48, (DWORD)allocIT + 0xFC );
	refMem ( 0x00490C87, (DWORD)allocIT + 0x118 );
	refMem ( 0x00415E2D, (DWORD)allocIT + ItemV - 4 );
	refMem ( 0x0041E60B, (DWORD)allocIT + ItemV - 4 );
	refMem ( 0x00442A54, (DWORD)allocIT + ItemV - 4 );
	refMem ( 0x00480CDE, (DWORD)allocIT + ItemV - 4 );
	refMem ( 0x00480D5B, (DWORD)allocIT + ItemV - 4 );
	refMem ( 0x0048592B, (DWORD)allocIT + ItemV - 4 );
	refMem ( 0x0048E64B, (DWORD)allocIT + ItemV - 4 );
	refMem ( 0x0048E853, (DWORD)allocIT + ItemV - 4 );
	refMem ( 0x0048EE1B, (DWORD)allocIT + ItemV - 4 );
	refMem ( 0x004967A6, (DWORD)allocIT + ItemV - 4 );
	refMem ( 0x0049895F, (DWORD)allocIT + ItemV - 4 );
	refMem ( 0x004A47E2, (DWORD)allocIT + ItemV - 4 );
	refMem ( 0x004A49DC, (DWORD)allocIT + ItemV - 4 );
	refMem ( 0x004BB853, (DWORD)allocIT + ItemV - 4 );
	refMem ( 0x004BB90B, (DWORD)allocIT + ItemV - 4 );
	refMem ( 0x004BB9D3, (DWORD)allocIT + ItemV - 4 );
	refMem ( 0x004C1963, (DWORD)allocIT + ItemV - 4 );
	refMem ( 0x004C1E3F, (DWORD)allocIT + ItemV - 4 );
	refMem ( 0x0054802C, (DWORD)allocIT + ItemV - 4 );
	refMem ( 0x0055B693, (DWORD)allocIT + ItemV - 4 );
	refMem ( 0x00581A53, (DWORD)allocIT + ItemV - 4 );
	refMem ( 0x00436859, (DWORD)allocIT + ItemV + 0x20 );
	refMem ( 0x00438A69, (DWORD)allocIT + ItemV + 0x20 );
	refMem ( 0x00438D55, (DWORD)allocIT + ItemV + 0x20 );
	refMem ( 0x00438EF0, (DWORD)allocIT + ItemV + 0x20 );
	refMem ( 0x00439009, (DWORD)allocIT + ItemV + 0x20 );
	refMem ( 0x004390AF, (DWORD)allocIT + ItemV + 0x20 );
	refMem ( 0x0043914F, (DWORD)allocIT + ItemV + 0x20 );
	refMem ( 0x0043B401, (DWORD)allocIT + ItemV + 0x20 );
	refMem ( 0x0044FCD9, (DWORD)allocIT + ItemV + 0x20 );
	refMem ( 0x004BE26B, (DWORD)allocIT + ItemV + 0x20 );
	refMem ( 0x004BE31F, (DWORD)allocIT + ItemV + 0x20 );
	refMem ( 0x005594D6, (DWORD)allocIT + ItemV + 0x20 );
	refMem ( 0x004980EE, (DWORD)allocIT + ItemV + 0x24 );
	refMem ( 0x00442BAC, (DWORD)allocIT + 1 );

};

/*
Finaliza a Tabela;
*/
void endIT ()
{
	*(DWORD *)( (DWORD)allocIT + ItemV ) = 0x0A;
	*(DWORD *)( (DWORD)allocIT + ItemV + 4 ) = 0x64;
	*(DWORD *)( (DWORD)allocIT + ItemV + 8 ) = 0x03E8;
	*(DWORD *)( (DWORD)allocIT + ItemV + 0x0C ) = 0x2710;
	*(DWORD *)( (DWORD)allocIT + ItemV + 0x10 ) = 0x0186A0;
	*(DWORD *)( (DWORD)allocIT + ItemV + 0x14 ) = 0x0F4240;
	*(DWORD *)( (DWORD)allocIT + ItemV + 0x18 ) = 0x989680;
	*(DWORD *)( (DWORD)allocIT + ItemV + 0x1C ) = 0x05F5E100;
	*(DWORD *)( (DWORD)allocIT + ItemV + 0x20 ) = 0x3B9ACA00;
	*(DWORD *)( (DWORD)allocIT + ItemV + 0x24 ) = 0x2710;
	*(DWORD *)( (DWORD)allocIT + ItemV + 0x2C ) = 0x12;
	*(DWORD *)( (DWORD)allocIT + ItemV + 0x30 ) = 7;
	*(DWORD *)( (DWORD)allocIT + ItemV + 0x34 ) = 0x0C;
};
