#pragma once

//#include <set>

class SocketData;

struct PartyInfo;
struct PacketItemData;
struct QuestStatusData;
enum EQuestStatus;

#define PVPDAMAGE_MAX_QUEUE				256
#define PVPDAMAGE_TIME_DAMAGE			750
#define PVPDAMAGE_TIME_DAMAGE_MAX		1500

#define PVPBUFF_COUNT 3


#define USER_STATUS_UPDATE_GRACE			3000
#define USER_USER_STATUS_UPDATE_GRACE		10000



struct FinishedQuestData
{
	EQuestType								eQuestType;
	SYSTEMTIME								sDateFinished;
};

enum EGameLevel
{
	GAMELEVEL_None = 0,
	GAMELEVEL_One = 1,
	GAMELEVEL_Two = 2,
	GAMELEVEL_Three = 3,
	GAMELEVEL_Four = 4,
};

struct BlessCastleItemBuff
{
	DWORD			dwItemEndTime;
	int				iItemID;
	int				iValueData;
};


enum class PvPDamageBuffSlot : uint8_t
{
    Burning,
    Poison
};

#pragma pack(push, 1)
struct DamagePvPDataHandler
{
	bool Active;
	PvPDamageBuffSlot Slot;

	ID CasterID;

	ESkillID SkillID;
    int Damage;
	EAttackState State;
    int RepeatCount;
    int RepeatInterval;
	DWORD NextUpdate;

	int16_t	Param1;
    int16_t	Param2;

	/* OLD DATA
	BOOL						bActive;

	UserData					* pcTarget;
	int							iTargetID;

	int							iDamage;
	int							iSkillID;
	int							iParam1;
	int							iParam2;

	DWORD						dwTimeUpdate;
	*/
};
#pragma pack(pop)

struct	CastleMaster
{
	int		Flag;
	DWORD	dwCheckTime;
	int		SelectSkill;
};

//class rsPLAYINFO
class UserData
{
public:

	UserData();

	char						szOwner[32];				//0x0
	char						szAccountName[32];			//0x20
	char						szCharacterName[32];		//0x40

	DWORD						dwHashAccountName;			//0x60
	DWORD						dwHashCharacterName;		//0x64

	int							AdminOperMode;				//0x68
	int							bUse_InsertCommand;			//0x6C

	BOOL						bInGame;					//0x70
	DWORD						dwDataError;				//0x74
	int							bKeepAlive;					//0x78
	EMapID						iMapID;						//0x7C
	Point3D						sPosition;					//0x80
	Point3D						sAngle;						//0x8C
	char						szEmpty03b[0x0C];
	struct CharacterData		sCharacterData;				//0xA4
	int							iCharacterList;				//0x274

	DWORD						dwNoticeCounter;			//0x278

	BYTE						baUnitStatus[5][0x1000];	//0x27C				//srPlayBuff
	DWORD						dwaUnitStatusSize[5];		//0x527C			//srPlayBuffSize
	int							bNotMovingCounter;				//0x5290			//PlayBuffSleep

	DropItemData				sDropItems[64];				//0x5298
	DropItemData				sIntentoryItems[316];		//0x5598
	char						szEmpty[1440];				//0x6464	dont use it more...
	int							iWarehouseStatus;			//0x6A04	//0 = closed, 1 = normal, 2 = seasonal

	int							iaItemTradePotionID[16];	//0x6A08	???
	int							iTradePotion;				//0x6A48	???
	SellItemShopData			sItemShopList[32];			//0x6A4C
	int							iItemShopList;				//0x6C4C
	DWORD						dwItemShopTime;				//0x6C50

	int							iSaveItems;					//0x6C54   when item dropping this gets updated
	int							iSaveGold;					//0x6C58  -1 after save game again..
	int							iInventoryGold;				//0x6C5C
	DWORD						dwRecvTimePlayBuff;			//Time received
	DWORD						srSendPlayBuffCnt;			//Sent buffer counter
	DWORD						srRecvPlayBuffCnt;			//Buffer counter received
	UINT						uUnitStatusCounter;			//0x6C6C
	int							iID;						//0x6C70  dwObjectSerial
	EChatColor					iChatColor;					//0x6C74  ChatMsgMode
	char						szChat[256];				//0x6C78  szChatMessage[256];
	char						szTradeMessage[128];		//		  szTradeMessage[128];
	DWORD						dwTradeMsgCode;				//		  dwTradeMsgCode
	SocketData *				pcSocketData;				//0x6DFC  lpsmSock
	DWORD						dwTimeLastPacket;			//0x6E00  dwLastRecvTime
	DWORD						dwTimeLastStatusUpdate;		//0x6E04  dwLastMotionRecvTime
	char						szEmpty06[0x08];
	void *						psUnknown1;					//0x6E10

															//Equipment
	EItemID						eShieldEquipped;			//0x6E14
	EItemID						eWeaponEquipped;			//0x6E18

	EGameLevel					iGameLevel;					//0x6E1C  AdminMode
	int							bHideMode;					//0x6E20 //HideMode - Transparent mode (when the administrator is invisible to others)
	int							bShowDebugInfo;				//0x6E24

	DWORD						dwTimeOnline;				//0x6E28  dwConnectTime
	int							iLastMaskCountUser;			//0x6E2C  LastTransPlayUserCount;	Another player number last sent
	int							iLastMaskCountMonster;		//0x6E30  LastTransAutoPlayCount;	Another automatic character number last sent

															//6eac = free
															//6e98 = free
															//6e9c = free
															//6ea0 = free
															//6ea4 = free
															//6ea8 = free

	char						padddd[0x8];

	int							iUnitStatusCounter;			//0x6E3C

	char  paddddd[0x08];

	int							iUnitHitCounter;			//0x6E48    HitMonsterCount;  Attacked Monster Counter

	class UnitData *			pcLastAttack;				//0x6E4C	lpChrTarget;    Records the monsters that the user recently attacked (priority transmission)
	int							lLastAttackID;				//0x6E50	dwTarObjSerial; Monster ID number that the user recently attacked

	int							UserPacketLoop;				//0x6E54	Number of packets transmitted during user 1 looping (transmission statistics)
	int							BefUserPacketLoop;			//0x6E58	Number of packets transmitted during the previous looping (transmission statistics)

	int							AttackCount;				//0x6E5C	Attack counter
	DWORD						dwAttackTime;				//0x6e60	Attack time
	DWORD						dwLastAttackTime;			//0x6E64	Last attack time
	int							DamagePacketCount;			//0x6E68	Attack packet counter
	DWORD						dwDamagePacketTime;			//0x6e6C	Client-side time that the damage packet was sent

	UINT						uUnitKillCounter;			//0x6E70	MonsterKillCounter
	DWORD						dwLastTradeChksum;			//0x6E74	Checksum at last transaction
	DWORD						dwLastTradeTime;			//0x6E78	Time at last transaction

	short						sLowLevel_PresentItem[2];	//0x6E7C	Low-level item gift

	DWORD						dwStartUser_PresentItem_Time;	//0x6E80	Start user item gift
	DWORD						dwStartUser_PresentItem_Count;	//0x6E84	Start user item gift

	////////////////////////////////////////////////
	// Attack sending and receiving data

	int							Send_AttackCount;				//0x6E88
	int							Send_AttackDamage;				//0x6E8C
	int							Recv_AttackCount;				//0x6E90
	int							Recv_AttackDamage;				//0x6E94

	/////////////// Macro detection ////////////////////

	DWORD						dwMacroTime;
	int							Macro_PosX;
	int							Macro_PosZ;
	int							Macro_AngY;
	DWORD						dwMacroAngTime;
	DWORD						dwMacroChkTime;

	UINT						uaUpdateCounter1_statusNum[1024];		//0x6EB0
	UINT						uaUpdateCounter2_userID[1024];		//0x6EB0 + 0x1000
	USHORT						uaUpdateCounter3_IdleCounter[1024];		//0x6EB0 + 0x1000 + 0x1000
	USHORT						uaUpdateCounter4_DelaySend[1024];		//0x6EB0 + 0x1000 + 0x1000 + 0x800
	UCHAR						uaUpdateCounter5[4096];		//0x6EB0 + 0x1000 + 0x1000 + 0x800 + 0x800

	int							uUnk;

	BOOL						bBlock;						//0xAEB4
	DWORD						dwUnknownso;				//0xAEB8
	int							iPacketCounter;				//0xAEBC
	DWORD						dwTickPacketCounter;		//0xAEC0
	char						szEmpty07aA[0x04];
	CharacterFile				* psCharacterFile;			//0xAEC8	lpRecordDataBuff
	DWORD						dwSaveCharacterData;		//0xAECC	dwRecordDataTime
	DWORD						dwExclusiveNum;				//0xAED0	EventMode
	int							iBellatraScore;				//0xAED4	EventScore
	int							iBellatraKills;				//0xAED8	EventCount
	DWORD						dwEventTime;				//			Event-related time
	short						sBellatraRoomID;			//0xAEE0	sEventParam #1
	short						sZero;						//			sEventParam #2
	char						cUnknown0800;				//0xAEE4
	BYTE						bServerIndexID;				//0xAEE5
	char						cUnknown0801[2];			//0xAEE6
	UnitData					* pcHolyIncantatedUnitData;	//0xAEE8

															//Settings
	bool						bDisabledWhisper;			//0xAEEC
	bool						bDisabledTrade;				//0xAEED
	bool						bDisabledParty;				//0xAEEE
	bool						bThirdEye;					//0xAEEF

															//EXP and GOLD
	DWORD						bAFK;						//0xAEF0
	int							iCompulsionMagneticRange;	//0xAEF4
	LARGE_INTEGER				liEXP;						//0xAEF8
	DWORD						dwPadding5[2];				//0xAF00
	Point3D						sOldPosition;				//0xAF08
	int							iBlockSaveEXP;				//0xAF14
	int							iGoldIn;
	int							iGoldOut;
															//Unit Status
	UINT						uNewUnitStatus;				//0xAF20
	UINT						uLastUnitStatus;			//0xAF24

	DWORD						dwDisconnectTime;			//0xAF28 if non-zero then no save
	int							iVersion;					//0xAF2C
	DWORD						dwCharacterUpdateTime;		//0xAF30

	char						szEmpty08b[0x003C];			//

	BOOL						bBlockAction;				//0xAF70 Blocks Further Action of User

	char						szEmpty08c[0x0024];

	DWORD						dwUnknownDecodeVar1;		//0xAF98 - used in 0x5798F0 (dm_DecodePacket)
	DWORD						dwUnknownDecodeVar2;		//0xAF9C - used in 0x5798F0 (dm_DecodePacket)

	char						szEmpty08d[0x8];			//

															//Party

	BOOL						bUnused11;					//Depracated 0xAFA8
	DWORD						dwUnused12;					//Depracated
	void 					  * pUnused13;					//Depracated
	ID							iUnused14;					//Depracated
	void					  * pUnused15[6];				//Depracated
	ID							iaUnused16[6];				//Depracated
	int							iUnused17;					//Depracated
	int							iUnused18;					//Depracated

	ULONG						iClanID;					//0xAFF0
	int							iTicket;					//0xAFF4
	int							iEXPPotion;					//0xAFF8

															//B474 = TIME Might Stone
															//B478 = iItemSlotFlag Might Stone
															//B47C = sDamage Boost Might Stone

	char						szEmpty09[0x2D4];

	MinMax						sDamageLimit;				//0xB2D0
	short						sCriticalLimit[2];			//0xB2D4

	char						szEmpty09a[0x30];

	DWORD						dwAutomationEndTime;		//0xB308
	int							iAutomationDamageBoost;		//0xB30C
	DWORD						dwMaximizeEndTime;			//0xB310
	int							iMaximizeMaxDamageBoost;	//0xB314
	DWORD						dwPhysicalAbsorbEndTime;	//0xB318
	int							iPhysicalAbsorbBoost;		//0xB31C
	DWORD						dwExtremeShieldEndTime;		//0xB320
	int							iExtremeShield;				//0xB324
	DWORD						dwHolyBodyEndTime;			//0xB328
	int							iHolyBodyAbsorb;			//0xB32C
	DWORD						dwHolyValorEndTime_Obsolete;			//0xB330
	int							iHolyValorParam_Obsolete;			//0xB334
	DWORD						dwHolyReflectionEndTime;	//0xB338
	int							iHolyReflectionDamage;		//0xB33C
	DWORD						dwEnchantWeaponEndTime_DontUse;		//0xB340
	int							iEnchantWeaponAttr_DontUse;			//0xB344
	DWORD						dwMetalArmorEndTime;		//0xB348
	int							iMetalArmorParam;			//0xB34C
	DWORD						dwSparkShieldEndTime;		//0xB350
	int							iSparkShieldDamage;			//0xB354
	DWORD						dwFalconEndTime;			//0xB358
	short						saFalconDamage[2];			//0xB35C
	DWORD						dwDivineShieldEndTime;		//0xB360	//dwSkill_DivineInhalation_Time
	short						sDivineShieldAbsorb;		//0xB364	//dwSkill_DivineInhalation_Param
	short						sDivineShieldAttackRating;	//0xB366
	DWORD						dwTriumphOfValhallaEndTime_Obsolete; //0xB368
	int 						iTriumphOfValhallaDamage_Obsolete;	//0xB36C
	DWORD						dwVirtualLifeEndTime;		//0xB370
	int							iVirtualLifeData;			//0xB374
	DWORD						dwEnergyShieldEndTime;		//0xB378
	int							iEnergyShieldDamageReduce;		//0xB37C
	DWORD						dwSpiritElementalEndTime;	//0xB380
	union
	{
		int						iSpiritElementalBoost;		//0xB384
		MinMax					sSpiritElementalBoost;
	};
	DWORD						dwDancingSwordEndTime_NotUsed;		//0xB388
	int							iDancingSwordParam_NotUsed;			//0xB38C
	DWORD						dwDancingSwordDelayTime;	//0xB390 ????
	DWORD						dwVanishEndTime;			//0xB394
	short						saVanishParam[2];			//0xB398
	DWORD						dwHookedTargetEndTime;		//0xB39C
	DWORD						iCompulsionParam;			//0xB3A0
	DWORD						dwMagneticSphereEndTime;	//0xB3A4
	int							iMagneticSphereParam;		//0xB3A8
	DWORD						dwBerserkerEndTime_Obsolete;//0xB3AC
	int 						iBerserkerParam_Obsolete;	//0xB3B0
	DWORD						dwAssassinEyeEndTime;		//0xB3B4
	DWORD						AssassinEyeParam;			//0xB3B8 - add crit damage. not used?
	DWORD						dwVagueEndTime;				//0xB3BC
	int							iVagueParam;				//0xB3C0
	DWORD						dwForceOfNatureEndTime;		//0xB3C4
	unsigned short				saForceOfNatureParam[2];	//0xB3C8
	DWORD						dwGodlyShiedEndTime;		//0xB3CC
	int							iGodlyShiedParam;			//0xB3D0
	DWORD						GodsBlessingTime;			//0xB3D4
	DWORD						GodsBlessingParam;			//0xB3D8
	DWORD						HallOfValhallaTimeObsolete;			//0xB3DC
	unsigned short				HallOfValhallaParamObsolete[0x2];	//0xB3E0
	DWORD						FrostTime_DoNotUse;			//0xB3E4
	DWORD						FrostParam_DoNotUse;		//0xB3E8
	DWORD						RegenerationFieldTime;		//0xB3EC
	DWORD						SummonMuspellTime;			//0xB3F0
	DWORD						SummonMuspellParam;			//0xB3F4
	DWORD						dwCurse_AttackTime;			//0xB3F8
	DWORD						dwCurse_AttackParam;		//0xB3FC
	DWORD						dwCurse_DefenceTime;		//0xB400
	DWORD						dwCurse_DefenceParam;		//0xB404
	unsigned char				bSkillPoint[0x10];			//0xB408
	DWORD						ForceOrbCode;					//0xB418 - Spare. No longer used in ASM
	int							ForceOrbTime;					//0xB41C - Spare. No longer used in ASM
	DWORD						ForceOrbDamage;					//0xB420 - Spare. No longer used in ASM
	DWORD						dwForceOrb_SaveCode_DoNotUse;	//0xB424 - Spare. No longer used in ASM
	DWORD						dwForceOrb_SaveTime_DoNotUse;	//0xB428 - Spare. No longer used in ASM
	DWORD						dwForceOrb_SaveDamage_DoNotUse; //0xB42C - Spare. No longer used in ASM
	DWORD						dwForceOrbCityTime_DoNotUse;	//0xB430 - Spare. No longer used in ASM
	class UnitData				* pcPetData;					//0xB434 - lpLinkChar
	DWORD						dwObjectPetID;					//0xB438 - dwLinkCharCode
	short						sNumDeath;						//0xB43C
	short						sPVPKills;						//0xB43E
	DWORD						dwCheck_MasterClanTime;			//0xB440
	CastleMaster				sCastleMaster;					//0xB444
	DWORD						dwBlessSkill_Code;				//0xB450
	DWORD						dwBlessSkillTime;				//0xB454
	DWORD						dwBlessSkillParam;				//0xB458
	BlessCastleItemBuff			saBCBuff[PVPBUFF_COUNT];		//0xB45C
	int							iVerifiedLogin;					//0xB480
	int							* iUnkUnused001;				//0xB484
	DWORD						dwHolyIncantationID;			//0xB488
	CWarehouseBase				* pcWarehouse;					//0xB48C
	int 						iWarehouseDataSize;				//0xB490
	int  						iWarehouseTotalSize;			//0xB494
	int							iWarehouseCountPage;			//0xB498
	BOOL						bDisconnectFunction;			//0xB49C
	int							dwunk;							//0xB4A0
	DWORD						dwT4RoomTime;					//0xB4A4
	short						sBlessCastle_Damage; // 0xB43C //sBlessCastle_Damage[0];
	short						sBlessCastle_Kills; // 0xB43E //sBlessCastle_Damage[1];
	DWORD						dwObjectIDT4;					//0xB442
	class UnitData				* pcUnitDataT4;					//0xB4A8
	DWORD						dwObjectPetSystemID;			//0xB4AC - dwLinkCharCode
	class UnitData				* pcPetSystem;					//0xB4B0 - lpLinkChar
	BYTE						bTickCountPetSystem;			//0xB4B4
	BYTE						bPetSystemSkinID;				//0xB4B5
	BYTE						bPetSystemSummomTimeDelay;		//0xB4B6
	char						dwUnknown10[1];					//0xB4B7
	DWORD						dwUnknown9[20];					//0xB4B8


	int							GetLevel() { return this->sCharacterData.iLevel; };
	int							GetGold() { return this->iInventoryGold; };

	int							GetCurrentHealth();
	int							GetMaxHealth();
	BOOL						IsDead();

};

#define MAX_THROW_GOLD_TIME_PER_PERIOD	6
#define MAX_THROW_GOLD_TIME_PERIOD		(10 * 1000)

struct GoldThrowHistory
{
	int Count;
	DWORD LastTime;
};

enum class EPlayerMovementStatus : UINT
{
	Moving					= 0,
	Standing_1_Sec			= 1,
	Standing_4_Sec			= 2,
	Standing_8_Sec			= 3,
	AFK						= 4,
};


class User
{
public:
	BOOL									bInUse;
	class UserData						  * pcUserData;
	UINT									uIndex;
	BOOL									bNetServer;
	BOOL									bClosingSocket;

	BOOL									bDisconnectNet;
	DWORD									dwIP;

	int										iAccountID;				//id from UserInfo table (from main db only)
	int										iCharacterID;			//id from CharacterInfo table. It will be same between Babel and Seasonal. See CharacterInfo Insert query

	UINT									uSerialHD;

	EMapID									 eMapID;				//Map that the Player is currently in
	EMapID									 eSecondMapID;			//Second Map that the Player has loaded

	BOOL									b8;   //8 times per second
	BOOL									b16;  //4 times per second
	BOOL									b32;  //2 times per second
	BOOL									b64;  //1 times per second

	UINT									iSecondTick; //play time in seconds (roughly). use iSecondTick % 60 for ~ 1 minute interval updates etc
	BOOL									bTenSeconds; //flag to allow process
	BOOL									bOneMin; //flag to allow process

	struct PacketPlayDataEx					pData;
	BOOL									bReceivedExp;
	int										iEXPPotion;

	int										iClientAttackPowerMin;
	int										iClientAttackPowerMax;

	int										iPVPKills;
	int										iPVPDeaths;
	int										iPVPKillStreak;
	BOOL									bResetKillStreak;
	int										iKillSafe;
	int										iLastKillSafeID;
	DWORD									dwKillSafeTimeOut;
	DWORD									dwLastKillIP;

	struct PacketItemPremiumData			sPremiumData;
	DWORD									dwTimeSync;
	DWORD									dwTimeSavePremium;
	struct PacketItemData					sTempItem;

	int										iCurrentStage;
	int										iLastStage;
	//DWORD									dwEndForceTime;

	uint64_t 								uUserLoadedTime;				//Time in seconds epoch 1970 when the user has loaded (after char selection)

	int										iCountPremiumUpdate;
	BOOL									bCheckedGameMaster;
	class UserData						  * pcUserDataSpyoner;
	int										iWarehouseError;
	int										iWarehouseGold;

	DropItemData							sLastInventoryItems[316];		//Snapshot of inventory items before the one in UserData gets replaced via PKTHDR_NetPlayerInventory

#if defined (DEV_MODE) && defined (DEBUG_INVENTORY)
	DropItemData							sLastDropItemsTemp[316];
	BOOL									bLastDropItemsValidTemp;
#endif

	DropItemData							sWarehouseItems[420];
	DropItemData							sWarehouseItems_Seasonal[420];	//Seasonal warehouse for post-merge.
	DropItemData							saTradeItems[20];

	ItemBoxSlot								sItemBox;
	BOOL									bCharacterExLoaded;
	CharacterDataEx							sCharacterDataEx;

	int										iDefenseRating;
	short									sBlockChance;
	short									sEvadeChance;

	BOOL									bQuestLoaded;

	PacketQuestUpdateData					saPacketQuestData[MAX_QUESTPACKETDATA];

	DWORD									dwChatTradeTime;
	BOOL									bDebugChat;
	BOOL									bDisableWarnAndErrorRelaysToClient;
	PacketSettingsData						sSettingsData;

	//BOOL									bUnbrokeAge;

	BOOL									bMuted;
	DWORD									dwUnMuteExpiryTime;

	//T5
	BOOL									bInT5QuestArena;
	int										iRoundT5QuestArena;
	DWORD									dwTimeT5QuestArenaUpdate;
	DWORD									dwTimeT5QuestArenaSpawnUpdate;

	UINT									uQuestT5ID;
	USHORT									sQuestT5ProgressValue[2];

	BOOL									bInGrandFury;
	DWORD									dwGrandFuryTickEnter;




	int										iChasingHuntAttackPowerBoost;
	time_t									uChasingHuntEndTime;

	int										iHolyMindAbsorb;
	DWORD									dwHolyMindTime;

	int										iSummonMuspellDamageBoost;

	int										iWispAbsorb;
	DWORD									dwWispTime;

	UINT									uLethalSightLevel;
	UINT									uLethalSightRange;
	UINT									uLethalSightCritical;
	UINT									uLethalSightDamageBoost;
	time_t									uLethalSightTimeOut;

	UINT									uFierceWindLevel;
	DWORD									dwFierceWindTimeOut;

	UINT									uHolyConvictionLevel;
	DWORD									dwHolyConvictionTimeOut;

	UINT									uWizardTranceLevel;
	DWORD									dwWizardTranceTimeOut;

	time_t									uEnchantWeaponEndTime;
	time_t									uEnchantWeaponNextSwitchTime;
	BYTE									bEnchantWeaponLevel1Based;
	BYTE									bEnchantWeaponRotationCounter;
	EElementID								eEnchantWeaponElement;
	short									sEnchantWeaponAtkPowerPercent;
	short									sEnchantWeaponParameter;
	BYTE									bEnchantWeaponChance;
	BYTE									bEnchantWeaponDurationSec;

	time_t                                  uFrostJavelinEndTime;
	short                                   sFrostJavelinAtkPowerPercent;
	BYTE                                    bFrostJavelinWeaponMonsterSlowdown;
	BYTE                                    bFrostJavelinChance;
	BYTE                                    bFrostJavelinDurationSec;

	time_t									uDancingSwordEndTime;
	BYTE									bDancingSwordLevel;
	EElementID								eDancingSwordElement;
	BYTE									bDancingSwordChainLevelEW; //Enchant Weapon
	BYTE									bDancingSwordChainLevelDR; //Death Ray

	time_t									uBerserkerEndTime;
	short									sBersekerDamageBoost;
	short									sBersekerDamageToHPPercent;

	time_t									uConcealEndTime;
	BYTE									bConcealLevel1Based;

	time_t									uDemonBaneEndTime;
	BYTE									bDemonBaneLevel1Based;

	time_t									uAlasEndTime;
	BYTE									bAlasLevel1Based;

	time_t									uHolyValorEndTime;
	BYTE									bHolyValorLevel1Based;

	time_t									uHolyBodyEndTime;
	BYTE									bHolyBodyLevel1Based;

	time_t									uDrasticSpiritEndTime;
	BYTE									bDrasticSpiritLevel1Based;

	time_t									uForceOfNatureEndTime;
	BYTE									bForceOfNature1Based;

	time_t									uInpesEndTime;
	BYTE									bInpesLevel1Based;

	time_t									uAdventMigalEndTime;
	BYTE									bAdventMigalLevel1Based;

	time_t									uHallOfValhallaEndTime;
	BYTE									bHallOfValhallaLevel1Based;

	time_t									uTriumphOfValhallaEndTime;
	BYTE									bTriumphOfValhallaLevel1Based;

	time_t									uVirtualLifeEndTime;
	BYTE									bVirtualLifeLevel1Based;

	time_t									uVirtualRegenEndTime;
	BYTE									bVirtualRegenLevel1Based;

	time_t									uVenomThornEndTime;
	BYTE									bVenomLevel1Based;

	time_t									uElectricalOverloadEndTime;
	BYTE									bElectricalOverloadLevel1Based;

	time_t									uTitleSetTimeOut;

	int										iTradeCoin;
	int										iTradeGold;

	int										iBlessCastleScore;

	struct PacketDebugDamageInfoContainer	sDamageInfoContainer;
	struct PacketSkillBuffStatusContainer	sSkillBuffContainer;

	struct PacketSkillBuffStatus			sSkillBuffStatus;
	UINT									uSkillBuffHash;
	BOOL									bForceSkillBuffUpdate;

	BOOL									bInvalidateSkillBuffStatus;

	//Unit Status
	UINT									uPriorityCounter;
	EPlayerMovementStatus					uMovementStatus;

	//Status Send Buffers
	UINT									uBufferPosition;
	PacketUnitStatusContainer				sUnitStatusContainer;
	UINT									uBufferPositionM;
	PacketUnitStatusContainer				sUnitStatusContainerM;
	time_t									uUnitStatusTime;

	DamagePvPDataHandler					sPvPDamageData[PVPDAMAGE_MAX_QUEUE];

	BYTE									baUnitStatusNew[5][0x2000]; //0-3 is for during movement / combat; 4 is for idle / standing only

	UINT									uaUnitSkillHash[1024];

	//Party
	BOOL									bParty;
	PartyInfo							  * psParty;

	int										iBellatraRoomInitialPlayerCount;
	short									sBellatraSoloCrown;

	char									szTitle[25];
	short									sTitleRarity;
	short									sDialogSkinId;
	short									sClassRanking;

	DWORD									dwFuryLeaveTime; //Time that user will leave the fury arena

	BOOL									bInvisibleInMiniMap; //Allows user to hide themselves from minimap, but they can't get exp (to prevent cheating using bot)

	BYTE									bLootFilterEnabled;
	int										iLootFilterFlags;
	short									sEquipmentMinLevel;

	// Equipped item tracking for LootFilter ilvl comparison
	EItemID									eArmorEquipped;
	EItemID									eBootsEquipped;
	EItemID									eGauntletsEquipped;
	EItemID									eBraceletEquipped;
	EItemID									eRingRightEquipped;
	EItemID									eRingLeftEquipped;
	EItemID									eOrbEquipped;
	EItemID									eRobeEquipped;
	EItemID									eAmuletEquipped;

	DropItemData							LastDroppedItem;

	BOOL									bTeasureHuntingMode;

	std::map<int, FinishedQuestData>		*mFinishedQuestData;

	bool PlayerAuthed;
	GoldThrowHistory m_GoldThrowHistory;
	DWORD LastInventoryHash;
	DWORD LastGoldHash;
	std::vector<DropItemData> LastDroppedItemsThrowMapHack;
	BOOL TradeWindowOpen;
    std::vector<uint32_t> PlayerItemPutHashs;
    uint32_t GetItemGoldLastTime;
    uint32_t GetItemGoldTotal;
    bool WasInTwoSideTrade;


	EItemID		eForceOrbItemID;					//Force orb item id
	BOOL		bForceOrbActive;					//Bool flag to determine if force orb is activated or not
	int			iForceOrbDamage;					//Force orb damage
	int			iForceOrbDamageExtraPercentage;		//Force orb damage extra percentage

	DWORD		dwIgnoreNpcClickTime;

	time_t		uNextClanIdCheckTime;				//Periodically update clan id

	int			iLatestAttackPower;					//store latest base attack power of the attacker, to be used for various calculations like enchant weapon

public:
											User();
										   ~User();

    BOOL									IsValid () { return bInUse && pcUserData && pcUserData->iID != 0 && pcUserData->pcSocketData != 0; };
	BOOL									IsInGame() { return pcUserData->iID != 0 ? TRUE : FALSE; };
	BOOL									IsConnected() { return pcUserData->pcSocketData != 0 ? TRUE : FALSE; };

	BOOL									IsValidAndInGame () { return pcUserData && pcUserData->pcSocketData && pcUserData->iID != 0; };

	int										GetID() { return pcUserData->iID; }
	EMapID									GetMapID() { return pcUserData->iMapID; }

	EGameLevel								GetGameLevel() { return pcUserData->iGameLevel; }

	char *									GetName();

	void									SetGameLevel( EGameLevel eLevel ) { pcUserData->iGameLevel = eLevel; }

	void									Start( SocketData * sd ) { pcUserData->pcSocketData = sd; };

	void									Stop() { bInUse = FALSE; pcUserData->pcSocketData = NULL; bNetServer = FALSE; };

	void									UpdateQuestWithValues( DWORD dwQuestID, EQuestStatus eStatus, DWORD dwValue1 = 0, DWORD dwValue2 = 0, DWORD dwValue3 = 0, DWORD dwValue4 = 0, DWORD dwValue5 = 0 );
	void									UpdateQuest( DWORD dwQuestID, EQuestStatus eStatus );

	EQuestUserStatus						GetQuestStatus( int iQuestID );

	BOOL									RestartQuest( DWORD dwQuestID );

	char 									* GetCharacterName ( );
	int 								    GetCharacterLevel() { return pcUserData == NULL ? 0 : pcUserData->sCharacterData.iLevel; };
	ECharacterClass						    GetCharacterClass() { return pcUserData == NULL ? ECharacterClass::CHARACTERCLASS_None : pcUserData->sCharacterData.iClass; };

	BOOL									CanSee( User * pcOtherUser );
	BOOL									CanSee( Unit * pcOtherUnit );

	void									Init();
	void									UnInit();
};

struct PacketBellatraResult : Packet
{
	int					iUnk;
	DWORD				dwTime;
	BOOL				bShow;

	short				saTeamRound[2];

	DWORD				dwUnk[15];

	int					iResultCount;

	struct BellatraResult
	{
		char			szName[32];
		UserData		* pcUserData;
		int				iClass;
		int				iClanID;
		int				iScore;
		int				iMonster;
		DWORD			dwUnk[4];
	};

	BellatraResult		saResult[32];
};
