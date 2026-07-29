#include <Windows.h>
#include "premiumstruct.h"

#define MAX_PKTSIZ				0x2000
#define PKTHDR_ENCMAX			0x10

#define PKTLENBFRHDR			4
#define PKTLEN					WORD
#define PKTENC					BYTE
#define PKTCRC					BYTE
#define PKTHDR					DWORD



#define MAX_SKILL_ARRAY_DATA			150
#define MAX_COMMON_SKILL_INFO_DATA		25


#define SLOW_SPEED_BASE 256
#define SERVER_CHAR_FRAME_STEP			320
#define SERVER_CHAR_SKIP				4
#define MAX_CONTINUE_SKILL				20
#define BURNING_TICKRATE_MS				500
#define POISON_MAXSTACK					50
#define POISON_DURATION_SEC				30
#define POISON_TICKRATE_MS				1000

//See https://docs.google.com/document/d/1V9Ozk5gN-RaBpgflo-8_1zZzde2h5y5np4n8r1t2Q9w/edit
#define DISTANCE_XY_152_meters			16777216 // (152 meters)  (((64*64)+(64*64)) * 2048)
#define DISTANCE_XY_107_meters			 8388608 // (107 meters)  (((64*64)+(64*64)) * 1024)
#define DISTANCE_XY_76_meters			 4194304 // ( 76 meters)  (((64*64)+(64*64)) *  512)
#define DISTANCE_XY_60_meters			 2621440 // ( 60 meters)  (((64*64)+(64*64)) *  320)
#define DISTANCE_XY_54_meters			 2097152 // ( 54 meters)  (((64*64)+(64*64)) *  256)
#define DISTANCE_XY_47_meters			 1638400 // ( 47 meters)  (((64*64)+(64*64)) *  200) iLureDistance uses this distance
#define DISTANCE_XY_41_meters			 1228800 // ( 41 meters)  (((64*64)+(64*64)) *  150)
#define DISTANCE_XY_38_meters			 1048576 // ( 38 meters)  (((64*64)+(64*64)) *  128)
#define DISTANCE_XY_33_meters			  819200 // ( 33 meters)  (((64*64)+(64*64)) *  100)
#define DISTANCE_XY_30_meters			  640000 // ( 30 meters)
#define DISTANCE_XY_27_meters			  524288 // ( 27 meters)  (((64*64)+(64*64)) *  64)
#define DISTANCE_XY_24_meters			  409600 // ( 24 meters)  (((64*64)+(64*64)) *  50)
#define DISTANCE_XY_20_meters			  291600 // ( 20 meters)
#define DISTANCE_XY_19_meters			  262144 // ( 19 meters)  (((64*64)+(64*64)) *  32)
#define DISTANCE_XY_15_meters			  164025 // ( 15 meters)
#define DISTANCE_XY_13_meters			  131072 // ( 13 meters)  (((64*64)+(64*64)) *  16)
#define DISTANCE_XY_10_meters			   65536 // ( 10 meters)  (((64*64)+(64*64)) *  8)
#define DISTANCE_XY_7_meters			   32768 // ( 7 meters)   (((64*64)+(64*64)) *  4)
#define DISTANCE_XY_5_meters			   16384 // ( 5 meters)   (((64*64)+(64*64)) *  2)
#define DISTANCE_XY_3_meters			    8192 // ( 3 meters)   (((64*64)+(64*64)) *  1)
#define DISTANCE_XY_05					    4096 // ( 1.5 meters) (((64*64)+(64*64)) *  0.5)
#define DISTANCE_XY_025					    2048 // ( 0.7 meters) (((64*64)+(64*64)) *  0.25)

#define DISTANCE_300						90000 //~11m

#define DISTANCE_MAX_UNIT_VIEWLIMIT				DISTANCE_XY_107_meters //107 meters

#define DISTANCE_MAX_UNIT_OUTOFRANGE			DISTANCE_XY_60_meters	//60 meters
#define DISTANCE_MAX_PLAYER_BASIC_VIEW			DISTANCE_XY_54_meters	//54 meters
#define DISTANCE_MAX_UNIT_BASIC_VIEW			DISTANCE_XY_54_meters	//54 meters
#define DISTANCE_MAX_PARTY						DISTANCE_XY_41_meters	//41 meters
#define DISTANCE_MAX_CHATRANGE					DISTANCE_XY_41_meters	//41 meters
#define DISTANCE_MAX_PLAYER_DETAILED_VIEW		DISTANCE_XY_33_meters	//33 meters
#define DISTANCE_MAX_UNIT_DETAILED_VIEW			DISTANCE_XY_33_meters	//33 meters
#define DISTANCE_MAX_PLAYER_BASIC_VIEW_RICT		DISTANCE_XY_33_meters	//33 meters
#define DISTANCE_MAX_SKILL_VISUAL				DISTANCE_XY_24_meters	//24 meters
#define DISTANCE_MAX_PET_RANGE					DISTANCE_XY_24_meters	//24 meters
#define DISTANCE_MAX_PLAYER_DETAILED_VIEW_RICT	DISTANCE_XY_24_meters	//24 meters
#define DISTANCE_MAX_BOSS						DISTANCE_XY_24_meters	//24 meters
#define DISTANCE_MAX_SKILL_RANGE				DISTANCE_XY_24_meters	//24 meters
#define DISTANCE_MAX_FROZENSANCTUARY_TREE		DISTANCE_XY_19_meters	//13 meters



#define DISTANCE_MISC						0x1000 //4096
#define DISTANCE_MISC_Y						300
#define DISTANCE_MISC_Y_EX					1000




#define SERVER_LEVEL_MAX				120
#define MAX_UNIT_PLAYDATA				75
#define MAX_QUESTPACKETDATA				15


enum EItemID;

struct HPStruct
{
	short								sMin;
	short								sMax;
};

struct StructFuryArenaBoss
{
	int      iCurrentBossHP;
	int      iMaxBossHP;
};


enum ECharacterType : int
{
	CHARACTERTYPE_None				= 0,

	CHARACTERTYPE_NPC				= 0x00,
	CHARACTERTYPE_Monster			= 0x01,
	CHARACTERTYPE_Player			= 0x80, //smCHAR_STATE_PLAYER
};


enum EClassFlag : int
{
	CLASSFLAG_Unknown				= -1,

	CLASSFLAG_None					= 0,
	CLASSFLAG_Fighter				= 2,
	CLASSFLAG_Mechanician			= 1,
	CLASSFLAG_Archer				= 4,
	CLASSFLAG_Pikeman				= 3,
	CLASSFLAG_Atalanta				= 0x00020000,
	CLASSFLAG_Knight				= 0x00010000,
	CLASSFLAG_Magician				= 0x00040000,
	CLASSFLAG_Priestess				= 0x00030000,
	CLASSFLAG_Assassin				= 0x00050100,
	CLASSFLAG_Shaman				= 0x00050000,
};

enum class EAllowedClassSpecFlag : int
{
	None = 0,

	Fighter			= 1 << 0,
	Mechanician		= 1 << 1,
	Archer          = 1 << 2,
	Pikeman         = 1 << 3,
	Assassin        = 1 << 4,

	Atalanta        = 1 << 8,
	Knight          = 1 << 9,
	Magician        = 1 << 10,
	Priestess       = 1 << 11,
	Shaman          = 1 << 12
};

enum ECharacterRank
{
	CHARACTERRANK_Error = -1,

	CHARACTERRANK_Rank1 = 0,
	CHARACTERRANK_Rank2 = 1,
	CHARACTERRANK_Rank3 = 2,
	CHARACTERRANK_Rank4 = 3,
	CHARACTERRANK_Rank5 = 4,
	CHARACTERRANK_Rank6 = 5,
	CHARACTERRANK_Rank7 = 6,
	CHARACTERRANK_Rank8 = 7,
};

enum ECharacterClass
{
	CHARACTERCLASS_None					= 0,

	CHARACTERCLASS_Fighter				= 1,
	CHARACTERCLASS_Mechanician			= 2,
	CHARACTERCLASS_Archer				= 3,
	CHARACTERCLASS_Pikeman				= 4,
	CHARACTERCLASS_Atalanta				= 5,
	CHARACTERCLASS_Knight				= 6,
	CHARACTERCLASS_Magician				= 7,
	CHARACTERCLASS_Priestess			= 8,
	CHARACTERCLASS_Assassin				= 9,
	CHARACTERCLASS_Shaman				= 10,
};

enum ECharacterRace
{
	CHARACTERRACE_Tempskron,
	CHARACTERRACE_Morion,
};

enum ECharacterTitleRank
{
	CHARACTERTITLERANK_White			= 0,
	CHARACTERTITLERANK_Green			= 1,
	CHARACTERTITLERANK_Blue				= 2,
	CHARACTERTITLERANK_Purple			= 3,
	CHARACTERTITLERANK_Orange			= 4,
	CHARACTERTITLERANK_Teal				= 5,
};

enum EMonsterType
{
	MONSTERTYPE_Normal					= 0x00,
	MONSTERTYPE_Undead					= 0x90,
	MONSTERTYPE_Mutant					= 0x91,
	MONSTERTYPE_Demon					= 0x92,
	//MONSTERTYPE_Mechanic				= 0x93, //Fused with Normal types now
	MONSTERTYPE_Netural					= 0x94, //Has no bonus dmg against this type, for test map etc.
	MONSTERTYPE_Summon					= 0xA0,	//smCHAR_MONSTER_SUMMON
	MONSTERTYPE_Pet						= 0xA1	//
};

enum EMonsterNature
{
	MONSTERNATURE_Neutral				= 0x80,
	MONSTERNATURE_Good					= 0x81,
	MONSTERNATURE_Evil					= 0x82,
};


enum EElementID : unsigned short
{
	Organic = 0,	//sITEMINFO_BIONIC		0	- Living body
	Earth = 1,		//sITEMINFO_EARTH		1	- Nature		-- not used
	Fire = 2,		//sITEMINFO_FIRE		2	- Fire
	Ice = 3,		//sITEMINFO_ICE			3	- Chill
	Lightning = 4,	//sITEMINFO_LIGHTING	4	- Lightning
	Poison = 5 ,	//sITEMINFO_POISON		5	- Posion
	Water = 6,		//sITEMINFO_WATER		6	- Water			-- not used
	Wind = 7,		//sITEMINFO_WIND		7	- Wind			-- not used
	None = 15,		//sITEMINFO_NONE		15	- None
};


enum class EDamageTextType : short
{
	Undefined	= -1,
	None		= 0,
	Defense		= 1,
	Block		= 2,
	Evade		= 3,
	Unknown		= 4,
	Normal		= 5,
	Critical	= 6,
	Gold		= 7, //for gold pickup
	Poison		= 8,
	Miss		= 9,
	PetAttack	= 10,
	Flinch		= 11,	  //debug only
	Burning		= 12,
	Freezing	= 13,
	Stun		= 14,
	Distortion	= 15,
	Curse		= 16,
	Healing		= 17,
	Damage		= 18,
};

enum EAttackState : short
{
	State_0_Normal = 0,				//Normal attacks has a chance to cause critical
	State_1_Knockback = 1,			//Pike wind (knock back)
	State_2_Stun = 2,				//Stun
	State_3_Slow = 3,				//U
	State_4_LifeSteal = 4,			//Life steal
	State_5_Spare = 5,				//Used to be shield strike
	State_6_Extinction = 6,			//Priestess Extinction skill
	State_7_Distortion = 7,			//??
	State_8_Cursed = 8,				//Curse (reduce attack power of monsters)
	State_9_DivineShield = 9,		//Divine Shield special effect

	State_100_Fire = 100,			//Rage of Zeram (Fire Attack)				Fire Resistance
	State_101_Brain = 101,			//Spark (brain attack)						Lightning Resistance
	State_103_DivineLightning = 103,	//Divine Lightning (50% undead weighted)		Lightning Resistance
	State_104_Water = 104,			//Ice (attack)								Ice resistance
	State_105_Electricity = 105,	//Encent Electricity						Lightning Resistance
	State_106_Poison = 106,			//Poison attack								Lightning Resistance

	State_128_Skill = 128,			//Skilla ttack (0x80)
	State_201_PullIn = 201,			//Complusion

	State_301_Charged = 301			//For Phoenix Shot
};

enum class EAttackProperty : short
{
	Unknown = -1,
	Undefined = 0,
	Organic = 1,	//sITEMINFO_BIONIC		Living body
	Earth = 2,		//sITEMINFO_EARTH		Nature
	Fire = 3,		//sITEMINFO_FIRE		Fire
	Ice = 4,		//sITEMINFO_ICE			Chill
	Lightning = 5,	//sITEMINFO_LIGHTING	Lightning
	Poison = 6 ,	//sITEMINFO_POISON		Posion
	Water = 7,		//sITEMINFO_WATER		Water
	Wind = 8,		//sITEMINFO_WIND		Wind
	Holy = 9,		//						Holy
	None = 15,		//sITEMINFO_NONE		None
};


enum class EItemFlag : unsigned char //max is 0xFF (8 bits total)
{
	None			= 0,
	ForceAllowDrop	= 0x01, //if the item was originally not allowed to be dropped, but can be dropped. Also it will hide from 'A' key press
	TreasureHuntingMode		= 0x02,
	UNUSED_3		= 0x04,
	UNUSED_4		= 0x08,
	UNUSED_5		= 0x10,
	UNUSED_6		= 0x20,
	UNUSED_7		= 0x40,
	UNUSED_8		= 0x80,
};


//                              Enumeratos
enum PacketsHeader
{
	//Net Server
	PKTHDR_NetIdentifier							= 0x7F000001,
	PKTHDR_NetExp									= 0x7F000002,
	PKTHDR_NetGetExp								= 0x7F000003,
	PKTHDR_NetGameMaster							= 0x7F000004,
	PKTHDR_NetGetGameMaster							= 0x7F000005,
	PKTHDR_NetClan									= 0x7F000006,
	PKTHDR_NetDisconnectUser						= 0x7F000007,
	PKTHDR_NetForceOrb								= 0x7F000008,
	PKTHDR_NetVersion								= 0x7F000009,
	PKTHDR_NetCharacterSync							= 0x7F00000A,
	PKTHDR_NetGiveExp								= 0x7F00000B,
	PKTHDR_NetPingPong								= 0x7F00000C,
	PKTHDR_NetBCStatusReq							= 0x7F00000D,
	PKTHDR_NetBCStatusSnd							= 0x7F00000E,
	PKTHDR_NetBCNewOwnerClanID						= 0x7F00000F,
	PKTHDR_NetBotAdd								= 0x7F000010,
	PKTHDR_NetPremiumEXP							= 0x7F000011,
	PKTHDR_NetUnloadUser							= 0x7F000012,
	PKTHDR_NetEXPEvent								= 0x7F000013,
	PKTHDR_NetTradeCoin								= 0x7F000014,
	PKTHDR_NetUsersOnline							= 0x7F000015,
	PKTHDR_NetQuestT5Data							= 0x7F000016,
	PKTHDR_NetQuestUpdateData						= 0x7F000017,
	PKTHDR_NetQuestUpdateDataPart					= 0x7F000018,
	PKTHDR_NetQuestCancelOrFinish					= 0x7F000019,
	PKTHDR_NetLoadPVPData							= 0x7F000020,
	PKTHDR_NetLoadQuestData							= 0x7F000021,
	PKTHDR_NetSyncFinishedQuestData					= 0x7F000022, //to game servers
	PKTHDR_NetPlayerWorldToken						= 0x7F000040,
	PKTHDR_NetPlayerInventory						= 0x7F000041,
	PKTHDR_NetPlayerGold     						= 0x7F000042,
	PKTHDR_NetPlayerGoldDiff  						= 0x7F000043,
	PKTHDR_NetPlayerThrow    						= 0x7F000044,
	PKTHDR_NetStartMaintenance						= 0x7F000045,
	PKTHDR_NetPlayerItemPut 						= 0x7F000046,
	PKTHDR_NetEasterEvent							= 0x7F000047,
	PKTHDR_NetAgingEvent							= 0x7F000048,
	PKTHDR_NetBellatraSoloRanking                   = 0x7F000049,
	PKTHDR_NetHallowenEvent							= 0x7F000050,
	PKTHDR_NetOpenWarehouse							= 0x7F000051,
	PKTHDR_NetOpenWarehouseSeasonal					= 0x7F000052,
	PKTHDR_NetChristmasEvent						= 0x7F000053,
	PKTHDR_NetFreeEventGirl							= 0x7F000054,
	PKTHDR_NetStopMaintenance						= 0x7F000056,
	PKTHDR_NetLevelUp								= 0x7F000058,
	PKTHDR_NetSetVersion							= 0x7F000059,
	PKTHDR_NetReloadCoinShop						= 0x7F000060,
	PKTHDR_NetClearTickRO							= 0x7F000061,
	PKTHDR_NetClearTickChristmas					= 0x7F000062,
	PKTHDR_NetKickAll								= 0x7F000063,
	PKTHDR_NetMuteSync								= 0x7F000064,
	PKTHDR_NetGetMuteSync							= 0x7F000065,
	PKTHDR_NetStarWarsEvent							= 0x7F000066,
	PKTHDR_NetBeeEvent								= 0x7F000067,
	PKTHDR_NetOpenItemDistributor					= 0x7F000068,
	PKTHDR_NetGameMasterCommand						= 0x7F000069,
	PKTHDR_NetPlayDataEx							= 0x7F000070, //game server to login server
	PKTHDR_NetRemoveFinishedQuestData				= 0x7F000071, //game server to login server or vice versa
	PKTHDR_NetSendItemData							= 0x7F000072, //game server to login server
	PKTHDR_NetMimicEvent							= 0x7F000073, //game server to login server

    PKTHDR_Connected                                = 0x48470080,

	//Packets Item Data
	PKTHDR_PerfectItemData							= 0x5A320002,
	PKTHDR_MixDescription							= 0x5A320001,
	PKTHDR_OpenMixItem								= 0x48470049,
	PKTHDR_RecvWarehouse							= 0x48470048,
	PKTHDR_SendWarehouse							= 0x48470047,
	PKTHDR_TradeData								= 0x48470041,
	PKTHDR_TradeRequest								= 0x48470042,
	PKTHDR_TradeItems								= 0x48470043, //Trade Time...( when trade your inventory item to player... )
	PKTHDR_TradeReady								= 0x48470044,
	PKTHDR_ThrowItem								= 0x48470053, //smTRANSCODE_THROWITEM
	PKTHDR_CheckItemUseSlot							= 0x48478930,
	PKTHDR_CheckItemUseSlotEx						= 0x48478931,

 	PKTHDR_Skill_Healing							= 0x484700D2, //smTRANSCODE_HEALING
	PKTHDR_Skill_HolyMind							= 0x484700D3, //smTRANSCODE_HOLYMIND
	PKTHDR_Skill_GrandHealing						= 0x484700D4, //smTRANSCODE_GRAND_HEALING


	PKTHDR_KeySet									= 0x4847FFFF,
	PKTHDR_Crash									= 0x4847FFFE,
	PKTHDR_CrashData								= 0x4847FFFD,

	// Packets Character Data
	PKTHDR_PacketFun								= 0x43550001,
	PKTHDR_SetExp									= 0x43550002,
	PKTHDR_AttackRatingSend							= 0x43550003,
	PKTHDR_DamageDebugInfo							= 0x43550004,
	PKTHDR_ServerInfo								= 0x43550005,
	PKTHDR_PlayDataEx								= 0x43550006,	//received from each player every 5 seconds
	PKTHDR_PremiumData								= 0x43550007,
	PKTHDR_SetItemTimer								= 0x43550008,
	PKTHDR_DisableGM								= 0x43550009,
	PKTHDR_QuestData								= 0x4355000A, ///< Obselete
	PKTHDR_UpdateQuestData							= 0x4355000B,
	PKTHDR_FinishQuest								= 0x4355000C,
	PKTHDR_RestartQuest								= 0x4355000D,
	PKTHDR_BellatraCrown							= 0x4355000E,
	PKTHDR_PVPDataUpdate							= 0x4355000F,
	PKTHDR_UpdateSWPhase							= 0x43550010,
	PKTHDR_CancelQuest								= 0x43550011,
	PKTHDR_CrystalUse								= 0x43550012,
	PKTHDR_CrystalUseDie							= 0x43550013,
	PKTHDR_UserOnline								= 0x43550014,
	PKTHDR_CancelForceOrb							= 0x43550015,
	PKTHDR_FireMeteorite							= 0x43550016,
	PKTHDR_NPCEasterEgg								= 0x43550017,
	PKTHDR_PvPKill									= 0x43550018,
	PKTHDR_NPCIhinRewardRequest						= 0x43550019,
	PKTHDR_CharacterHeadOldUse						= 0x4355001A,
	PKTHDR_BlessCastleIndividualScore				= 0x4355001B,
	PKTHDR_BlessCastleUserScoreUpdate				= 0x4355001C,
	PKTHDR_NPCRudolphChristmas						= 0x4355001D,
	PKTHDR_CancelItemTimer							= 0x4355001E,
	PKTHDR_NPCMixGuild								= 0x4355001F, //xxstr mix guild
	PKTHDR_NPCRankList								= 0x43550020, //xxstr rank list
	PKTHDR_TOP_LEVEL								= 0x44550022, //xxstr top level
	PKTHDR_WrapCamera								= 0x44550024,
	PKTHDR_BlessCastleTopClanSync					= 0x44550025,

	//jlm - seperated packets
	PKTHDR_NPCRankListAllPlayers					= 0x43550030,
	PKTHDR_NPCRankListPVP							= 0x43550031,
	PKTHDR_NPCRankListByClass						= 0x43550032,
	PKTHDR_NPCRankListClanSod						= 0x43550033,
	PKTHDR_NPCRankListIndividualSod					= 0x43550034,
	PKTHDR_NPCRankListBlessedCastle					= 0x43550035,
	PKTHDR_NPCRankListSoloSod						= 0x43550037,

	//Packets Protect
	PKTHDR_LogCheat									= 0x6F6A0001,
	PKTHDR_DCAccount								= 0x6A6A0001,
	PKTHDR_ReconnectedServer						= 0x6A6A0002,
	PKTHDR_SendToLoginServer						= 0x6A6A0003,
	PKTHDR_Microphone								= 0x6A6A0004,
	PKTHDR_LogDataSimple							= 0x6A6A0005,
	PKTHDR_CheckSum									= 0x6A6A0006,
	PKTHDR_LogDebugger								= 0x6A6A0008,
	PKTHDR_UndentifiedPacket						= 0x6A6A0007,
	PKTHDR_AddExp									= 0x6A6A0009,
	PKTHDR_LoseExp									= 0x6A6A000A,
	PKTHDR_GetItemData								= 0x6A6A000B,
	PKTHDR_SetItemData								= 0x6A6A000C,
	PKTHDR_SetGold									= 0x6A6A000D,
	PKTHDR_WindowList								= 0x6A6A000E,
	PKTHDR_SettingsData								= 0x6A6A000F,
	PKTHDR_RetryWarehouse							= 0x6A6A0010,
	PKTHDR_WarehouseSaveSuccess						= 0x6A6A0011,
	PKTHDR_DamageQuick								= 0x6A6A0012,
	PKTHDR_QuestNPCOpen								= 0x6A6A0013,
	PKTHDR_QuestNPCAccept							= 0x6A6A0014,
	PKTHDR_QuestStartData							= 0x6A6A0015,
	PKTHDR_QuestUpdateData							= 0x6A6A0016,
	PKTHDR_QuestCancel								= 0x6A6A0017,
	PKTHDR_QuestNPCFinish							= 0x6A6A0018,
	PKTHDR_QuestNPCItemList							= 0x6A6A0019,
	PKTHDR_QuestHandleFinished						= 0x6A6A001A,
	PKTHDR_CharacterDataEx							= 0x6A6A001B,
	PKTHDR_ItemSlotHandle							= 0x6A6A001C,
	PKTHDR_T5QuestArena								= 0x6A6A001D,
	PKTHDR_RequestParty								= 0x6A6A001E,
	PKTHDR_JoinParty								= 0x6A6A001F,
	PKTHDR_UpdateParty								= 0x6A6A0020,
	PKTHDR_UpdatePartyData							= 0x6A6A0021,
	PKTHDR_ActionParty								= 0x6A6A0022,
	PKTHDR_RequestRaid								= 0x6A6A0023,
	PKTHDR_JoinRaid									= 0x6A6A0024,
	PKTHDR_RollDiceDropItem							= 0x6A6A0025,
	PKTHDR_RollDiceRanking							= 0x6A6A0026,
	PKTHDR_QuestItemSwap							= 0x6A6A0027,
	PKTHDR_BossTimeUpdate							= 0x6A6A0028,
	PKTHDR_IntegrityUpdateItem						= 0x6A6A0029,
	PKTHDR_UpdatePetDataEx							= 0x6A6A002A,
	PKTHDR_DynamicQuestOpen							= 0x6A6A002B,
	PKTHDR_DynamicQuestOpenNPC						= 0x6A6A002C,
	PKTHDR_QuestHandleFinishedContainer				= 0x6A6A002D,
	PKTHDR_DamageQuickPVP							= 0x6A6A002E,
	PKTHDR_GoldPickup								= 0x6A6A002F,
	PKTHDR_CharacterCombatData						= 0x6A6A0030,	//mainly for pvp
	PKTHDR_QuestDataRequest                         = 0x6A6A0031,
	PKTHDR_QuestDataResponse                        = 0x6A6A0032,
	PKTHDR_QuestNamesRequest						= 0x6A6A0033,
	PKTHDR_QuestNamesResponse						= 0x6A6A0034,
	PKTHDR_NewQuestNamesPostLvlUp					= 0x6A6A0035, //addition quests post level up
	PKTHDR_NeverSinkTeleportRequest					= 0x6A6A0036,
	PKTHDR_NeverSinkOpenUI							= 0x6A6A0037,
	PKTHDR_MissingRequiredQuestItems				= 0x6A6A0038, //items didn't exist on client side on completion
	PKTHDR_RemoveFinishedQuest						= 0x6A6A0039,
	PKTHDR_RollDiceDropUserResult					= 0x6A6A0040,


	//Coin Shop
	PKTHDR_CoinShopNPC								= 0x48476A01,
	PKTHDR_CoinShopAmount							= 0x48476A02,
	PKTHDR_CoinShopOpenTab							= 0x48476A03,
	PKTHDR_CoinShopTab								= 0x48476A04,
	PKTHDR_CoinShopOpenItemData						= 0x48476A05,
	PKTHDR_CoinShopItemData							= 0x48476A06,
	PKTHDR_CoinShopBuy								= 0x48476A07,
	PKTHDR_OpenCoinShop								= 0x48476A08,
	PKTHDR_CoinServiceOpen							= 0x48476A09,
	PKTHDR_CoinServiceBuy							= 0x48476A0A,
	PKTHDR_CoinServiceAgingRecoveryOpen				= 0x48476A0B,
	PKTHDR_CoinTraded								= 0x48476A0C,
	PKTHDR_CoinTradeSet								= 0x48476A0D,
	PKTHDR_PlayerInfo2								= 0x48470032, //smTRANSCODE_PLAYERINFO2

	//Packets Event
	PKTHDR_KillsInSod								= 0x503220B1,

	PKTHDR_CHECKITEM								= 0x50320300, //smTRANSCODE_CHECKITEM
	PKTHDR_LIMIT_DAMAGE								= 0x50322050, //smTRANSCODE_LIMIT_DAMAGE
	PKTHDR_BLESSCASTLE_INFO							= 0x503220C0, //smTRANSCODE_BLESSCASTLE_INFO
	PKTHDR_BLESSCASTLE_TAX							= 0x503220D0, //smTRANSCODE_BLESSCASTLE_TAX

	//Packets Sync Data
	PKTHDR_ChatGame									= 0x48471001,
	PKTHDR_ChatMessage								= 0x48471005,   //smTRANSCODE_WHISPERMESSAGE (pops up a chat message above the char)
	PKTHDR_ChatItemLink								= 0x48471007,
	PKTHDR_PersonalShopMessage						= 0x48478AC0,
	PKTHDR_SyncData1								= 0x4847008B,
	PKTHDR_SyncData2								= 0x4847008E,
	PKTHDR_CharacterData							= 0x48470020,   //smTRANSCODE_PLAYERINFO    (can also be for units)
	PKTHDR_GetPlayerInfoData						= 0x48470021,   //smTRANSCODE_GETPLAYERINFO (can also be for units)
	PKTHDR_Ping										= 0x435A0007,
	PKTHDR_PingLoginServer							= 0x435A0008,
	PKTHDR_FuryArena								= 0x435A0009,
	PKTHDR_FuryArenaItemData						= 0x435A000A,
	PKTHDR_FuryArenaBossData						= 0x435A000B,
	PKTHDR_NPCLarryOpen								= 0x435A000C,
	PKTHDR_SkillStatus								= 0x435A000E,
	PKTHDR_UseSkillNew								= 0x435A000F,
	PKTHDR_NPCMarinaOpen							= 0x435A0010,
	PKTHDR_SaveAndClose								= 0x435A0011,
	PKTHDR_PhoenixPet								= 0x484700B0,
	PKTHDR_Warehouse								= 0x48470047,
	PKTHDR_Save										= 0x484700E8,
	PKTHDR_Disconnect								= 0x484700E6,
	PKTHDR_Reconnect								= 0x48478010,
	PKTHDR_ReconnectLogin							= 0x48478011,
	PKTHDR_UpdateLevelClan							= 0x48470038,
	PKTHDR_TitleBoxMessage							= 0x48478100,   //smTRANSCODE_MESSAGEBOX (title box style)
	PKTHDR_PlayDataChar								= 0x48470013,   //smTRANSCODE_TRANSPLAYDATA - data about other player's unit
	PKTHDR_UnitStatusContainer						= 0x48470014,	//smTRANSCODE_PLAYDATAGROUP - data about other player's unit
	PKTHDR_GameStatus								= 0x48470018,   //smTRANSCODE_PLAY_COMMAND
	PKTHDR_KillCharacter							= 0x50320E00,
	PKTHDR_RegenChar								= 0x50320B00,	//smTRANSCODE_VIRTURAL_POTION (heals char)
	PKTHDR_UpdatePetData							= 0x50320A40,	//smTRANSCODE_UPDATEL_SKILL (but only used for pets)
	PKTHDR_AttackData								= 0x48470030,	//smTRANSCODE_ATTACKDATA
	PKTHDR_AttackData2								= 0x48470040,	//smTRANSCODE_ATTACKDATA
	PKTHDR_GoldBellatraClan							= 0x48478A18,
	PKTHDR_OpenMyPlayerShop 						= 0x48478A90,
    PKTHDR_OpenPlayerShop 						    = 0x48478AA0,
	PKTHDR_PlayerShopConfig 						= 0x48478AB0,
	PKTHDR_PlayerShopTrade							= 0x48478AD0,
	PKTHDR_LastAttackDamage							= 0x50322030,   //smTRANSCODE_DISP_DAMAGE
	PKTHDR_SingleTargetSkillData					= 0x50322000,	//smTRANSCODE_ATTACK_DAMAGE (damage) - was PKTHDR_AttackDamage
	PKTHDR_MultiTargetSkillData						= 0x50322010,	//smTRANSCODE_RANGE_DAMAGE  (damage by ranged skill) - was PKTHDR_RangeDamage
	PKTHDR_AttackResult								= 0x50322020,	//smTRANSCODE_ATTACK_RESULT (used for crit aging..)
	PKTHDR_BuyItemNPCShop							= 0x50320210,	//smTRANSCODE_SHOPTITEM
	PKTHDR_SellItemNPCShop							= 0x50320212,
	PKTHDR_RepairItemNPCShop						= 0x50320213,
	PKTHDR_ForceOrbData								= 0x50320030,
	PKTHDR_NpcUnitClick								= 0x50320031,
	PKTHDR_QuestCommand                             = 0x50320220, //smTRANSCODE_QUEST_COMMAND

	// Packets Login
	PKTHDR_ServerList								= 0x484700C0,
	PKTHDR_SaveData									= 0x48470081,  //smTRANSCODE_RECORDDATA
	PKTHDR_SelectCharacter							= 0x48470082,
	PKTHDR_FailRecordData							= 0x48470083,  //smTRANSCODE_FAILRECORDDATA
	PKTHDR_UserInfo									= 0x48470086,
	PKTHDR_CreateCharacter							= 0x48470088,
	PKTHDR_DeleteCharacter							= 0x48470087,
	PKTHDR_Version									= 0x4847008A,
	PKTHDR_AccountLoginCode							= 0x48470023,

	PKTHDR_Shop_ItemList							= 0x48470054, // smTRANSCODE_SHOP_ITEMLIST

	PKTHDR_LoginUser								= 0x48480001,

	//Packet Skills
	PKTHDR_BuffSkillsParty							= 0x50320A00,
	PKTHDR_CancelDrasticSpirit						= 0x50320A11,
	PKTHDR_CancelSkill								= 0x50320A10,	//smTRANSCODE_CANCEL_SKILL
	PKTHDR_SkillCast								= 0x50320A20,	//smTRANSCODE_PROCESS_SKILL
	PKTHDR_SkillCastData							= 0x50320A30,	//smTRANSCODE_PROCESS_SKILL2 0x50320A30
	PKTHDR_EvasionSuccess							= 0x50320A50,
	PKTHDR_PartyUsePot								= 0x484700D0,
	PKTHDR_HolyMind									= 0x484700D3,
	PKTHDR_Wisp										= 0x484700C3,


	//Packets Unit
	PKTHDR_OpenNPC									= 0x48470024,
	PKTHDR_NpcData									= 0x48470070,
	PKTHDR_SetExpOld_DontUse						= 0x48470031,	//smTRANSCODE_ADDEXP (don't use, it actually sets exp in Asm)
	PKTHDR_TeleportEvent							= 0x4847005D,
	PKTHDR_UnitStatusMove							= 0x48470010,	//smTRANSCODE_PLAYDATA1
	PKTHDR_PlayData									= 0x48470013,	//smTRANSCODE_TRANSPLAYDATA
	PKTHDR_UnitInfoQuick							= 0x4847004C,	//smTRANSCODE_PLAYERINFO_Q
	PKTHDR_OpenEventGirl							= 0x4847004F,	//smTRANSCODE_OPEN_EVENTGIFT
	PKTHDR_OpenAgingMaster							= 0x4847004D,	//smTRANSCODE_OPEN_AGING
	PKTHDR_OpenStarPoint							= 0x48478A10,   //smTRANSCODE_OPEN_STARPOINT
	PKTHDR_OpenCollect								= 0x4847004A,   //smTRANSCODE_OPEN_COLLECT
	PKTHDR_OpenGiveMoney							= 0x48478A14,   //smTRANSCODE_OPEN_GIVEMONEY
	PKTHDR_OpenClanMenu								= 0x48478A00,   //smTRANSCODE_OPEN_CLANMENU
	PKTHDR_PublicPolling							= 0x50320A70,   //smTRANSCODE_PUBLIC_POLLING

	//Effect

	PTKHDR_Effect_Item								= 0x48478000,	//smTRANSCODE_EFFECT_ITEM
	PTKHDR_Effect_Weapon							= 0x48478002,	//smTRANSCODE_EFFECT_WEAPON	- calls SetTransEffectWeapon on client side (2012)
	PTKHDR_Effect_Stun								= 0x48478004,	//smTRANSCODE_EFFECT_STUN
	PTKHDR_Effect_Curse								= 0x48478006,	//smTRANSCODE_EFFECT_CURSE
	PTKHDR_Effect_Ice								= 0x48478008,	//smTRANSCODE_EFFECT_ICE
	PTKHDR_Effect_Poison							= 0x4847800A,	//smTRANSCODE_EFFECT_POISION
	PTKHDR_Effect_Vanish							= 0x4847800C,	//smTRANSCODE_EFFECT_VANISH
	PTKHDR_Effect_PersonalShop						= 0x4847800E,	//smTRANSCODE_EFFECT_PERSHOP
	PTKHDR_Effect_LowLevelPet						= 0x4847800F,	//smTRANSCODE_EFFECT_LOWPET
	PTKHDR_Effect_ClanManager						= 0x48479000,	//smTRANSCODE_EFFECT_CLANMGR
	PTKHDR_Effect_ForceOrb							= 0x48479010,	//smTRANSCODE_EFFECT_FORCEORB
	PTKHDR_Effect_BlessScore						= 0x48479012,	//smTRANSCODE_EFFECT_BLESS_SCORE
	PKTHDR_Effect_User								= 0x48479014,	//new
	PTKHDR_Effect_Debuff							= 0x48479015,	//new
	PTKHDR_Effect_KeepSkill							= 0x48479020,	//smTRANSCODE_EFFECT_KEEPSKILL

	//Packets Map
	PKTHDR_WarpGateField							= 0x48470040,   //smTRANSCODE_WARPFIELD
	PKTHDR_DeleteFlagPoint							= 0x48470061,
	PKTHDR_TheDeath									= 0x48478800,
	PKTHDR_PlayItem									= 0x48470050,   //show item on floor - smTRANSCODE_PLAYITEM
	PKTHDR_SaveThrowMoney    						= 0x48470056,	//received via game server
	PKTHDR_SaveThrowItem2    						= 0x48470057,	//gameserver to login server

	PKTHDR_UseUnionCore								= 0x50320206,
    PKTHDR_WingItem                                 = 0x50320208,
	PKTHDR_BellatraInformation						= 0x503220B0,

	//Packets Item
	PKTHDR_SocketItemDataToServer					= 0x48470025,
	PKTHDR_SocketItemDataToClient					= 0x48470026,
	PKTHDR_DeleteItemFromMap						= 0x48470051,   //Removes item from map
	PKTHDR_NewItem									= 0x48470052,   //smTRANSCODE_PUTITEM
    PKTHDR_GetItem                                  = 0x4847005A,
    PKTHDR_PaymentMoney                             = 0x4847005E,
	PKTHDR_UpgradeAgingItem							= 0x50320204,
	PKTHDR_AgingItem								= 0x50320202,	//smTRANSCODE_AGINGITEM (also used for maturing)
	PKTHDR_OpenSmelting								= 0x50325000,
	PKTHDR_OpenResetItem							= 0x484700A6,
	PKTHDR_OpenManufacture							= 0x50326000,
	PKTHDR_ManufactureItem							= 0x50326001,
	PKTHDR_MixItem									= 0x50320200,
	PKTHDR_ResetItem								= 0x50326006,
	PKTHDR_NPCItemBox								= 0x48478A80,


	//others
	PKTHDR_RecordResult								= 0x48470084,
	PKTHDR_SendGameServer						    = 0x50320110,
	PKTHDR_DamageEncodeMem							= 0x50322F00,
	PKTHDR_DamageEncodeMem2							= 0x50322EC0,

	PKTHDR_NETWORK_QUALITY							= 0x484700E0,

	PKTHDR_CONNECTED								= 0x48470080,
	PKTHDR_DAMAGE_ENCODE_MEM2						= 0x50322EC0,
	PKTHDR_UPDATE_SERVER_PARAM						= 0x503220F0,
	PKTHDR_PROCESSINFO								= 0x484700E4,

	PKTHDR_SET_BLACKLIST							= 0x48470033,	//smTRANSCODE_SET_BLACKLIST - Hacking User Registration 2
	PKTHDR_SET_BLACKLIST2							= 0x48478400,	//smTRANSCODE_SET_BLACKLIST2 - Hacking User Registration 2
	PKTHDR_SET_BLACKLIST3							= 0x48478500,	//smTRANSCODE_SET_BLACKLIST3 - Hacking User Registration 3
	PKTHDR_HACKTRAP						            = 0x50320a90,	//smTRANSCODE_HACKTRAP
	PKTHDR_USEITEM_CODE				                = 0x4847005b,   //smTRANSCODE_USEITEM_CODE
	PKTHDR_MAKE_LINKCORE					        = 0x50320205,   //smTRANSCODE_MAKE_LINKCORE

	PKTHDR_PROCESS_TIMEMAX							= 0x50320500,   //smTRANSCODE_PROCESS_TIMEMAX
	PKTHDR_DAMAGE_ENCODE_MEM						= 0x50322F00,
	PKTHDR_SEND_GAMESERVER							= 0x50320110,

	//NEW
	//NOTE - max value should be 0x7FFFFFFF

	PTKHDR_EventGirlReset							= 0x6FFF0001,
	PKTHDR_RankingListRequest						= 0x6FFF0002,
	PKTHDR_NumfOfInitialBellatraPlayers				= 0x6FFF0003,
	PKTHDR_ExtraUserData							= 0x6FFF0004,
	PKTHDR_SoloRankingListRequest					= 0x6FFF0005,
	PKTHDR_PremiumDataSync							= 0x6FFF0006,	//for syncing timers of premium items to client
	PKTHDR_MiscUnitCommand							= 0x6FFF0007,
	PKTHDR_ForceOrbCancel							= 0x6FFF0009,
	PKTHDR_MapSpawnMarkers							= 0x6FFF000A,	//for map spawn markers
	PKTHDR_ServerSkillData							= 0x6FFF000B,	//server skill data
	PKTHDR_SkillBuffStatus							= 0x6FFF000D,
	PKTHDR_HalloweenEventStatus						= 0x6FFF000E,
	PKTHDR_TitleList								= 0x6FFF000F,
	PKTHDR_SetTitle									= 0x6FFF0010,
	PKTHDR_RecoverItem								= 0x6FFF0011,
	PKTHDR_EnterBellatraCastle						= 0x6FFF0012,
    PKTHDR_GameTimeSync                             = 0x6FFF0013,
	PKTHDR_BellatraCrownFull						= 0x6FFF0014,
	PKTHDR_BellatraCrownSolo						= 0x6FFF0015,
	PKTHDR_ServerSkillInfo							= 0x6FFF0016,
	PKTHDR_DamageInfoContainer						= 0x6FFF0017,
	PKTHDR_NewItemNPC								= 0x6FFF0018,
	PKTHDR_ServerSkillChainData						= 0x6FFF0019,
	PKTHDR_UseSkillPartyBuffNew						= 0x6FFF0020,
	PKTHDR_ServerCommonSkillInfo					= 0x6FFF0021,
	PKTHDR_QuickHealthUpdate						= 0x6FFF0022,
	PKTHDR_ClearUnitDataFromMap						= 0x6FFF0023,
	PKTHDR_NpcQuestStatus							= 0x6FFF0024,
	PKTHDR_NewItemAuto								= 0x6FFF0025,
	PKTHDR_MapIndicators							= 0x6FFF0026,
	PKTHDR_QuestMonsterName							= 0x6FFF0027,
	PKTHDR_QuestMonsterNameReq						= 0x6FFF0028,
	PKTHDR_UpgradeQuestWeapon						= 0x6FFF0029,
	PKTHDR_MonsterKill								= 0x6FFF0030,
	PKTHDR_SendStablePet							= 0x6FFF0031,
	PKTHDR_SendStablePet_Change_Name				= 0x6FFF0032,
	PKTHDR_SendStablePet_ActivePet					= 0x6FFF0033,

	PKTHDR_Warning_Skill							= 0x50322080,   //smTRANSCODE_WARNING_SKILL
	PKTHDR_Client_Error								= 0x48478900,   //smTRANSCODE_CLIENT_ERROR
	PKTHDR_Resistance								= 0x50320a60,   //smTRANSCODE_RESISTANCE

	PKTHDR_SendDataServer							= 0x50320100,   //smTRANSCODE_SEND_DATASERVER - Transfer connection to data server

	PKTHDR_Unknown1									= 0x50320040,
	PKTHDR_Unknown2									= 0x50326070,
	PKTHDR_Unknown3									= 0x50320120,
	PKTHDR_Unknown4									= 0x7f000051,  //related to WH
	PKTHDR_Unknown5									= 0x48470109,
	PKTHDR_Unknown6									= 0x48470045,  //related to skill master
	PKTHDR_Unknown7									= 0x50320039,  //related to item deletion when expired

	PKTHDR_Unknown8									= 0x50320C00,
	PKTHDR_Unknown9									= 0x50326030,
	PKTHDR_Unknown10								= 0x50326002,
	PKTHDR_Unknown11								= 0x50320800,
	PKTHDR_Unknown12								= 0x50326040,



	PKTHDR_CommandUser								= 0x4847005c,  //smTRANSCODE_COMMAND_USER

	PKTHDR_YahooMotion								= 0x50320E10,  //smTRANSCODE_YAHOO_MOTION - Execute yay action

	//1345454352 -> 0x50320110 ?? --> SEND_GAMESERVER
	//1345466112 -> 0x50322F00 ?? --> DAMAGE_ENCODE_MEM
	//1345466048 -> 0x50322EC0 ?? --> DAMAGE_ENCODE_MEM2
};

struct Packet //Header (Size = 0x08)
{
	PKTLEN	  iLength;				//Packet Length
	PKTENC	  iEncKeyIndex = 0;		//Packet Encryption Key Index
	PKTENC	  iEncrypted = 0;		//Packet Encrypted
	PKTHDR	  iHeader;				//Packet Header
};


struct PacketRankingListBlessedCastle : Packet
{
	char										szNameClan[32];
	char										szClanLeader[32];
	int											iClanBlessIconID;
};

struct PacketRankingListClanSodList : Packet
{
	char										szClanSodName[10][32];
	int											iClanSodMemberCount[10];
	int											iClanSodPoints[10];
};

struct PacketRankingListIndividualSod : Packet
{
	char										szSODPlayer[10][32];
	int											iSODPlayerPoints[10];
};

struct PacketRankingListPVP : Packet
{
	//dados top pvp
	char										szPVPCharName[10][32];
	//char										szPVPCharClass[10][32];
	//char										szPVPCharLevel[10][32];
	int											iPVPKills[10];
	int											iPVPDeaths[10];
};

struct PacketRankingListByClass : Packet
{
	int											iClass;
	int											iCount;
	char										szCharacterName[50][32];
	int											iLevel[50];
};

struct PacketRankingListAllPlayer : Packet
{
	int											iRankingType;
	int											iCount;
	char										szCharacterName[50][32];
	int											iClass[50];
	int											iLevel[50];
	uint64_t									iExperience[50];
};

struct PacketSoloRankingList : Packet
{
	int											iRankingType;
	int											iCount;
	char										szCharacterName[30][32];
	int											iClass[30];
	int											iLevel[30];
	int											iKills[30];
	int											iScore[30];
};

struct PacketRankingListRequest : Packet
{
	short										iRankingType;
};

struct ItemListMix
{
	int											iTypeMix;
	//SHELTONS
	int											iLucidy;
	int											iSereneo;
	int											iFadeo;
	int											iSparky;
	int											iRaident;
	int											iTransparo;
	int											iMurky;
	int											iDevine;
	int											iCelesto;
	int											iMirage;
	int											iInferna;
	int											iEnigma;
	int											iBellum;

	//char										Description[32];
};
struct ItemMixDesc
{
	char										Description[32];

};

#define MIXLIST_FUNCTION_TOTAL					70

//PacketMixGuild xxstr
struct PacketMixGuild : Packet
{
	int											iCount;
	ItemListMix									sMixList[MIXLIST_FUNCTION_TOTAL];
	ItemMixDesc									sMixDesc[MIXLIST_FUNCTION_TOTAL];
};

struct PacketSettingsData : Packet
{
	BOOL									bInital;			//initial load?
	BOOL									bNoTradeChat;
	BOOL									bNoPartyRequest;
	BOOL									bNoTradeRequest;
	BOOL									bNoWhisper;
	BYTE									bWeaponEnchantPerference; //0 = RNG
};

struct PacketCrash : Packet
{
	DWORD									dwExceptionCode;
	DWORD									dwExceptionFlags;
	PVOID									pvExceptionAddress;

	DWORD									EAX;
	DWORD									ECX;
	DWORD									EDX;
	DWORD									EBX;
	DWORD									ESP;
	DWORD									EBP;
	DWORD									ESI;
	DWORD									EDI;
	DWORD									EIP;

	DWORD									dwVersion;

	char									szAccountName[32];
};

#define CRASHDATA_SIZE 0x1F00

struct PacketCrashData : Packet
{
	BOOL									bFirst;
	char									szAccountName[32];
	int										iDataLen;
	DWORD									dwTick;
	BYTE									baData[CRASHDATA_SIZE];
};

// Save
struct	PacketSaveData : Packet
{
	int										iCount;
	int										iTotal;
	int										iDataSize;

	char									szData[8160];
};

struct	PacketGoldPickup : Packet
{
	int										iAmount;
};

struct MapIndicator
{
	enum class Type
	{
		None = 0,
		NextMapArrow = 1, //iValue is next map id
		HellSpawnMain = 2,
		HellSpawnSecondary = 3,
		BossIcon = 4,
	};

	int iValue;
	Type eType;
	int iCompactPosX;
	int iCompactPosZ;
	int iAngleY;
};

#define MAX_MAP_INDICATORS 30
struct PacketMapIndicators : Packet
{
	int iMapID;
	int iCount;
	MapIndicator mapIndicators[MAX_MAP_INDICATORS];
};

// Effects
struct PacketFireMeteorite : Packet
{
	struct Point3D
	{
		int											iX;
		int											iY;
		int											iZ;
	};

	Point3D									sBegin;
	Point3D									sEnd;
	int										iDelay;
	BOOL									bSmallMeteor;
	int										iCount;
};

// Packets TGF
struct PacketFuryArenaBossData : Packet
{
	StructFuryArenaBoss						sHP;
};

#define EASTER_ITEMS_MAX			3

struct PacketNPCEasterEgg : Packet
{
	enum EAction
	{
		ACTION_None,
		ACTION_GetReward,
	};

	int										iAction;

	int										iaItemID[EASTER_ITEMS_MAX];
	DWORD									dwaHead[EASTER_ITEMS_MAX];
	DWORD									dwaCheckSum[EASTER_ITEMS_MAX];
};



#define PUZZLE_ITEMS_MAX	3


struct PacketNPCPuzzle : Packet
{
	int										iaItemID[EASTER_ITEMS_MAX];
	DWORD									dwaHead[EASTER_ITEMS_MAX];
	DWORD									dwaCheckSum[EASTER_ITEMS_MAX];
};

struct PacketNPCRudolphChristmas : Packet
{

	int										iItemID;
	DWORD									dwHead;
	DWORD									dwCheckSum;
};


struct PacketDamageQuick : Packet
{
	BOOL											bPercentile;
	BOOL											bBaseCurrentHP;
	int												iDamage;
	BOOL											bPVP;
};

struct PacketDamageQuickDelayed : PacketDamageQuick
{
	int												iMillisecondDelay;
	int												dwClientTimeToExecute;
};



struct PacketStageItemData : Packet
{
	struct StageItemData
	{
		BOOL								bActive;
		EItemFlag							eItemFlags;
		BYTE								eRarity;
		BYTE								bSpare2;
		BYTE								bSpare3;
		int									iItemID;
		int									iX;
		int									iY;
		int									iZ;
		char								szName[32];
		int									iClass; //lpStgArea
	};
	BOOL									bActive;
	int										iCount;
	StageItemData							saItems[50];
};

// Party
struct PartyUserData
{
	char									szName[32];
	char									szModel[64];
	char									szHead[64];

	DWORD									dwObjectID;
	int										iLevel;
	CurMax									sHP;
	int										iJobCode;
	int										iGold;
	int										iX;
	int										iZ;
};
struct PartyUserInfo
{
	DWORD									dwObjectID;
	int										iLevel;
	CurMax									sHP;
	int										iJobCode;
	int										iGold;
	int										iX;
	int										iZ;
};

struct PacketPartyPlayInfo : Packet
{
	DWORD									dwObjectID;
	DWORD									dwTemp;

	int										iUsersCount;
	PartyUserInfo							sData[8];

	struct
	{
		CurMax								sHP;
		CurMax								sMP;
		CurMax								sSP;
	} New[8];
};
struct PacketPartyUsers : Packet
{
	DWORD									dwObjectID;
	DWORD									dwTemp[8];

	int										iUsersCount;
	PartyUserData							sData[8];

	struct
	{
		CurMax								sHP;
		CurMax								sMP;
		CurMax								sSP;
	} New[8];
};

struct PacketPartyPotion : Packet
{
	int										iMP;
	int										iHP;
	int										iSTM;
	int										iUnk;
};

//			LoginServer
struct _sTRANS_SERVER_INFO
{
	char									szServerName[ 0x20 ]; // 0x0
	char									szServerIp1[ 0x14 ]; // 0x20
	char									szServerIp2[ 0x14 ]; // 0x34
	char									szServerIp3[ 0x14 ]; // 0x48
	DWORD									dwServerPort1; // 0x5C
	DWORD									dwServerPort2; // 0x60
	DWORD									dwServerPort3; // 0x64
	DWORD									dwUnknown; // 0x64

};

struct PacketLoginChannel : Packet
{
	char									szServerName[ 0x10 ]; // 0x8
	DWORD									dwServerTime; // 0x18
	int										iClanTicket; // 0x1C
	DWORD									dwTemp; // 0x20
	int										iClanServerCount; // 0x24
	int										iServerCount; // 0x28
	_sTRANS_SERVER_INFO						ServerInfo[ 0x4 ]; // 0x2C

};

struct PacketServerList : Packet
{
	struct Header
	{
		char				szServerName[16];
		DWORD				dwTime;
		int					iTicket;
		DWORD				dwUnknown;
		int					iClanServerIndex;
		int					iGameServers;
	};

	struct Server
	{
		char				szName[32];
		char				szaIP[3][20];
		int					iaPort[4];
	};

	Header					sHeader;
	Server					sServers[4];
};

struct PacketTradeData : Packet
{
	int										iSenderID;
	int										iReceiverID;

	DWORD									dwCheckSum;

	DWORD									dwaTempData[4];

	BYTE									baData[4000];

};

struct PacketTradeSuccess : Packet
{
	int										iSenderID;
	int										iReceiverID;

	DWORD									dwCheckSum;

	DWORD									dwaTempData[4];

	BYTE									baData[4000];
};

struct PacketTradeRequest : Packet
{
	int										iType;
	int										iSenderID;
	int										iReceiverID;
};

struct PacketClanGoldReceive : Packet
{
	int										iGoldSend;
	int										iGold;
};

struct PacketReceiveItemIhin : Packet
{
	int										iCount; // Count + 80 : Security
};

struct PacketLoginUser : Packet
{

	DWORD									dwUnk[3];

	char									szUserID[32];
	char									szPassword[65];
	char									szMacAddr[20];
	char									szPCName[32];
	DWORD									dwSerialHD;

	char									szVideoName[256];

	char									szHardwareID[40];

	UINT									uWidthScreen;
	UINT									uHeightScreen;

	int										iSystemOS;
	int										iVersion;
};

struct PacketSyncTimeData : Packet
{
	DWORD									dwNetCount;
	DWORD									dwTime;
	int										iUnk;
	int										iUnk1;
};

struct	_TRANS_CHAR_INFO
{
	char	szName[32];				//name
	char	szModelName[64];		//Model name
	char	szModelName2[64];		//Model Name2
	DWORD	JobCode;				//Job code
	int		iLevel;					//Level
	DWORD	Brood;					//Tribe
	DWORD	dwArmorCode;			//Armor code
	int		StartField;				//Game field to start
	int		PosX, PosZ;				//Logout coordinates
	DWORD	dwTemp[13];				//Spare buffer
};









struct smTRANS_SELL_ITEM : Packet {
	int	ItemKind;
	int	ItemCounter;
	int	Ext[4];
};



struct PacketUserWarpEffect : Packet
{
	int										iX;
	int										iID;
	int										iY;
	int										iZ;
};

struct PacketSendUseUnionCore : Packet
{
	int										iY;
	int										iX;
	int										iZ;
	int										iMapID;

	DWORD									dwItemHead;
	DWORD									dwItemID;
	DWORD									dwItemChecksum;
	DWORD									dwUnk;
};


struct PacketPersonalShopMessageEffect : Packet
{
	DWORD									dwMSGCode;

	int										ColorBlink = 0;
	short									sColors[4];
	DWORD									DispEffect = 0;
	DWORD									BlinkScale = 0;
};

struct PacketForceOrbData : Packet
{
	BOOL										bIsInCityStage;
	ItemPremium									sForceOrb;
};


struct	PacketUserInfo : Packet
{

	char									szUserID[ 32 ];
	int										CharCount;

	_TRANS_CHAR_INFO						sCharacterData[ 6 ];
};

/// <summary>
/// Multi-purpose container containing multiple packets
/// </summary>
struct PacketContainer : Packet
{
	int				  iAmount;
	BYTE			  baBuffer[0x2000 - 0x4 - 0x8];
};

#define DAMAGEDEBUGCONTAINER_LENGTH	0x1FF6 // 0x2000 - 0x08 = header, 0x02 = short
#define DAMAGEDEBUGCONTAINER_MAXCOUNT  ((int)(DAMAGEDEBUGCONTAINER_LENGTH / sizeof(PacketDebugDamageInfo)) - 1)

/// <summary>
/// This packet holds info about the damage text
/// IMPORTANT - if you ADD anything extra to this packet
/// update DAMAGEDEBUGCONTAINER_MAXCOUNT otherwise leak / issues will happen!
/// </summary>
struct PacketDebugDamageInfo : Packet
{
	ID										dwObjectID;
	EDamageTextType							sTypeAction;
	short									sParameter;
	int										iValue;
	ID										dwAttackerID;
};


struct PacketDebugDamageInfoContainer : Packet
{
    USHORT			  sAmount;
    BYTE			  baBuffer[DAMAGEDEBUGCONTAINER_LENGTH];
};


struct PacketReconnectUser : Packet
{
	int										iUnk;
	DWORD									dwObjectID;
	char									szUserID[32];
};

struct PacketEvasion : Packet
{
	int										iSkillID;
	int										iUnk;
	DWORD									dwObjectID;
};

struct CharacterFile : Packet
{

	char									szHeader[8];

	void									* sCharInfo;
};

#define _XMIN_FURYARENA		-1050946
#define _XMAX_FURYARENA		-806751
#define _ZMIN_FURYARENA		-11170306
#define _ZMAX_FURYARENA		-10928372

enum EFuryArenaState
{
	FURYARENASTATEID_OpenNPC,
	FURYARENASTATEID_RequestEnterArena,
	FURYARENASTATEID_Enter,
	FURYARENASTATEID_TeleportCT2,
	FURYARENASTATEID_OpenNPCEntrance,
	FURYARENASTATEID_Status,
	FURYARENASTATEID_Exchange,
	FURYARENASTATEID_EXP,

};

struct PacketFuryArenaRequest : Packet
{
	EFuryArenaState							eState;
	int										iEventID;
	DWORD									dwTimeLeft;
	int										p1;
	int										p2;
	int										p3;
	int										p4;
	int										p5;
};

struct SPrizeData
{
	BOOL										bActive;
	char										szCode[32];
	DWORD										dwItemCode;
	int											iQuantity;
	int											iPercent;
};

struct PacketWrapCamera : Packet
{
	int										iMap;
	int										iX;
	int										iZ;
	int										iTurn;
	int										iAngle;
	int										iZoom;
};

/// <summary>
/// smEFFECT_ITEM
/// Total size: 8 ints
/// </summary>
struct PacketBuffData : Packet
{
	union
	{
		//32 bytes total!
		struct SDebuff
		{
			BYTE			bPoison;
			BYTE			bBurning;
			BYTE			bDistortion;
			BYTE			bCurse;

			BYTE			bSpare1;
			BYTE			bIceOverlay;
			BYTE			bSpare3;
			BYTE			bSpare4;

			int				iStunCount;
			int				iDamageReduction;
			int				iaSpare[4]; //for other debuff effects
		} Debuff;

		//32 bytes total!
		struct SBuff
		{
			BYTE			bVirtualRegen;
			BYTE			bVirtualLife;

			BYTE			bSpare[30];
		} Buff;

	};
};

struct PlayBuffer
{
	int										iX;
	int										iY;
	int										iZ;
	DWORD									dwFrame;
	short									saAngle[4];
	short									sStickItems[4];
	DWORD									dwTargetSerial;
};

struct PlayBufferData
{
	int										iCount;
	PlayBuffer								sPlayBuff;
};

struct PacketPlayDataMinor : Packet
{
	DWORD									dwObjectID;
	int										iX;
	int										iY;
	int										iZ;
	short									saAngle[4];
	DWORD									dwFrame;
	DWORD									dwAutoCharCode;
};

struct PacketPlayData : Packet
{
	DWORD									dwObjectID;
	DWORD									dwTarget;

	int										iPlayBuffCount;
	int										iStartPosition;

	short									sHp[2];
	DWORD									dwAutoCharCode;
	short									sMapID;
	short									sUnk;
	BYTE									bUpdateInfo[4];
	BYTE									bEventInfo[4];

	CurMax									sMP;
	CurMax									sSP;

	bool									bPartyLeader;

	short									sSecondMapID;

	short									saSpeed[2];

	IMinMax									sHPLong;

	short									sAngleY;
	short									sSize;

	int										iLevel;

	BYTE									bInvisibleInMiniMap;
	BYTE									bSpare1;
	BYTE									bSpare2;

	BYTE									bLootFilterEnabled;
	int										iLootFilterFlag; //user loot filters
	short									sEquipmentMinLevel;

	BYTE									baPadding[0x22];
};

struct PacketPlayDataGroup : Packet
{
	int										iPlayDataCount;
	DWORD									dwSendCode;
};
struct PacketPlayDataGroupBot : Packet
{
	int										iPlayDataCount;
	DWORD									dwSendCode;
	PacketPlayData							sPlayData;
	PlayBufferData							sPlayBuffData;

	struct PacketItemShopBot : Packet
	{
		int									iCheckSum;
		int									iUnk[5];
	};

	PacketItemShopBot						sPacket;
};

struct PacketPlayDataGroupBot2 : Packet
{
	int										iPlayDataCount;
	DWORD									dwSendCode;
	PacketPlayDataMinor						sPlayDataMinor;
};

struct PacketCheatLog : Packet
{

	int										iUnk;
	int										iLogCode;

	int										iOldCode1;
	int										iOldCode2;

	int										iExUnk;
	int										iExLogCode;
	int										iCode1;
	int										iCode2;
};

//See also PacketTransCommand
struct PacketCommand : Packet
{
	int										p2;
	int										p1;
	int										p3;
	int										p4;
};

struct PacketExtraUserInfo : Packet
{
	int										iUniqueUserID;
	short									sBellatraSoloCrown;
	short									iClanSodRanking;
	short									iForceOrbId;
	BOOL									bHideMeFromMiniMap;
};

/// <summary>
/// User effect for self or others
/// </summary>
struct PacketUserEffect : Packet
{
	int										iUniqueUserID;
	char									szTitle[25];
	short									sTitleRarity;
	short									sPlayerAuraId;
	short									sDialogSkinId;
};

struct PacketCommandEx : Packet
{
	int										p2;
	int										p1;
	int										p3;
	int										p4;

	int										p6;
	int										p5;
	int										p7;
	int										p8;
};

struct PacketCreateCharacter : Packet
{
	int										iUnknown;
	int										iCode;
	int										iUnk;
	char									szUserID[ 32 ];
	char									szCharname[ 32 ];
};

struct PacketSelectCharacter : Packet
{

	int dwReserved1;
	int dwReserved2;
	int dwReserved3;
	char									szCharName[32];
};
struct PacketVersion : Packet
{

	BOOL									bServerFull;
	int										iVersion;
	int										iUnk2;
};

struct PacketBellatraCrown : Packet
{
	int										iBellatraCrown;
};

struct PacketBellatraCrownFull : Packet
{
	int										iBellatraCrownGoldClanId;
	int										iBellatraCrownSilverClanId;
	int										iBellatraCrownBronzeClanId;
};




struct PacketBellatraInformation : Packet
{
	int					iRound;
	int					iType;
	int					iUnk;
	int					iValue;
};

struct PacketDeleteCharacter : Packet
{
	int										iUnknown;
	int										iCode;
	int										iUnk;
	char									szUserID[ 32 ];
	char									szCharname[ 32 ];
};



#define SKILL_COUNT_CHARACTER 16

struct CharacterDataPacket
{
	int										iStrength;			//Èû
	int										iSpirit;			//Á¤½Å·Â
	int										iTalent;			//Àç´É
	int										iAgility;
	int										iHealth;			//°Ç°­
	int										iAccuracy;			//»ó´ëÀû ¸íÁß·ü
	int										iAttackRating;	//¸íÁß·ü
	int										iMinDamage;	//Å¸°Ý¼öÄ¡ ( ÃÖ¼Ò / ÃÖ´ë )
	int										iMaxDamage;	//Å¸°Ý¼öÄ¡ ( ÃÖ¼Ò / ÃÖ´ë )
	int										iAttackSpeed;		//°ø°Ý¼Óµµ
};

struct PacketSetCharacterGold : Packet
{
	DWORD									dwGold;
};

struct PacketUpdateClanLevel : Packet
{
	DWORD									dwLevel;
	DWORD									dwCodeOP;
	DWORD									dwClanCode;
	DWORD									dwUpdateInfo;
};


struct PacketPlayDataEx : Packet
{
	int										iLevelSkill[ SKILL_COUNT_CHARACTER ];
	CharacterDataPacket						sCharacterData;
	int										iCritical;
	int										iLevelCharacter;
	BOOL									DebugInfo;
	BOOL									bGameMaster;
	char									szMacAddr[ 20 ];
	DWORD									dwServerID;
	DWORD									dwQuestLevelLog;
	DWORD									dwGold;
	DWORD									dwSpeed;
	int										iBellatraCrown;
	int										iMapID;
	UINT									uSerialHD;
	ID										userID;
};

struct ChecksumFunction
{
	DWORD										dwSize;
	DWORD										dwAddress;
	DWORD										dwCheckSum;
};

#define CHECKSUM_FUNCTION_TOTAL					400

struct PacketChecksumFunctionList : Packet
{
	BYTE										bKey;
	ChecksumFunction							sCheckSum[CHECKSUM_FUNCTION_TOTAL];
};

#define WINDOW_CHEATLIST_TOTAL					50

struct PacketWindowList : Packet
{
	int											iCount;
	char										szaWindowName[WINDOW_CHEATLIST_TOTAL][64];
};

struct PacketAttackRating : Packet
{
	int										iEnemyID;
	int										iAttackRating;
};

struct PacketWarpGateField : Packet
{
	int										iZ;
	int										iStage;
	int										iX;
};

struct PacketNumberOfInitialBellatraPlayers : Packet
{
	int										iCount;
};

struct PacketServerInfo : Packet
{
	int										iUsers;
	int										iMaxUsers;
	int										iBossTime;
	int										iServerVersion;
	short									sSiegeWarDay;
	short									sSiegeWarHour;
	BYTE									bSiegeWarType;
	BOOL									bGameMaster;
	SYSTEMTIME								sServerTime;
};

enum EPhoenixType
{
	PHOENIXTYPEID_None,
	PHOENIXTYPEID_Fire,
	PHOENIXTYPEID_Ice,
	PHOENIXTYPEID_Lightning,
	PHOENIXTYPEID_Healing,
	PHOENIXTYPEID_ElementHealing	= 2,
	PHOENIXTYPEID_DeleteMask		= 4855374,
};

struct PacketPhoenixPet : Packet
{
	int										iElementType;			// 2 = Healing
	EPhoenixType							eType;
	int										iUnk;
	DWORD									dwMask;					// 4855374 = Delete Phoenix
};

struct PacketDeleteFlagPoint : Packet
{
	int										iX;
	int										iZ;
};

struct PacketSetPremium : Packet
{
	DWORD									dwCode;
};

struct PacketExperience : Packet
{
	INT64									iAddExp;
};

struct MonsterKill : Packet
{
	int										iMonsterID;
	int										iKillerID;
	int										SParam;
	int										EParam;
};


struct PacketLoseExperience : Packet
{
	INT64									iSubExp;
	int										iLevel;
	int										iMapID;

	BOOL									bRessurectionItem;
};

struct PacketMixDescription : Packet
{
	char									szMixDescription[ 150 ];
};

struct PacketCrystalUse : Packet
{
	DWORD									dwObjectID;
	DWORD									dwItemID;
	DWORD									dwTime;
};

struct PacketCrystalUseDie : Packet
{
	DWORD									dwObjectID;
};

struct BlessCastleStatusShort
{
	int										iClanID1;
	int										iClanID2;
	int										iClanID3;
	int										iTax;
	int										iSkill;
	int										iaTowerType[6];
	int										iaGuardAmount[3];
};

struct BlessCastleStatusSettingsData
{
	int										iUnk[6];
	int										iTax;
	short									saTowerType[12];	//1 = ice, 2 = lightning, 3 = fire
	unsigned char							caGuardAmount[4];	//max = 20
	int										iSkill;				//1 = absorb, 2 = damage, 3 = evasion
	int										iPrice;
	DWORD									iClanID;
	DWORD									dwUnk[4];
};

#define MAX_TOP_DAMAGEDATA		10
#define MAX_DAMAGEDATA			100

enum ESiegeWarMode : int
{
	SIEGEWARMODE_Normal,
	SIEGEWARMODE_TheConquest,
	SIEGEWARMODE_TheMassacre,
};

struct PacketBlessCastleTopIndividual : Packet
{
	char				szCharacterName[3][32];

	int					iClass[3];

	int					iDamage[3];

	int					iMode;
};

struct PacketBlessCastleUserScoreUpdate : Packet
{
	int					iUserScore;

	int					iMode;
};

#define MAX_BLESSCASTLE_CLANCROWN		3
struct PacketBlessCastleTopClanSync : Packet
{
	int						iaClanID[MAX_BLESSCASTLE_CLANCROWN];
};

struct PacketBlessCastleTopData : Packet
{
	int					tCode;

	DWORD				dwObjectSerial;
	DWORD				dwNameCode;

	Point3D				sPosition;

	DWORD				dwCharCode;
	DWORD				dwUpdateTime;
	DWORD				dwTotalDamage;

	BYTE				baCrystalTowerCount[4];
	DWORD				dwFree[2];

	union
	{
		int				iaUserID[MAX_TOP_DAMAGEDATA];
		int				iaClanID[MAX_TOP_DAMAGEDATA];
	};

	int					iaDamage[MAX_TOP_DAMAGEDATA];
	int					iaCounter[MAX_TOP_DAMAGEDATA];
};

struct BlessCastleStatusData
{
	int										iCastleMode;
	DWORD									iClanID;
	int										iDefenseLevel;
	DWORD									dwTimeLimit;
	int										iCounter;

	int										iTax;
	int										iTaxNow;

	DWORD									dwBattleOverTime;

	int										iFountainPotionCount;
	DWORD									dwFountainTime;
	DWORD									dwFountainLastUseTime;
	int										iFountainCountUsers;

	DWORD									dwStartTime;
	short									saTimeSec[2];
	int										iSkill;
	int										iUnk[10];
	BlessCastleStatusSettingsData			sSettings;
	PacketBlessCastleTopData				sPacketTopData;
	struct AttackDamageData					* psaDamageData;

	BYTE									baSoldierCount[4];
	BYTE									baCrystalTowerCount[4];
	class UnitData							* pcUnitDataTower;
	DWORD									dwScoreLogTime;
};

struct BlessCastlePVPLeague
{
	int						iMode;
	DWORD					dwBeginTime;
	DWORD					dwEndTime;
	DWORD					dwRestartTime;
};

struct PacketNetBCStatus : Packet
{
	BlessCastleStatusShort					sBlessCastleStatus;
};

struct PacketBlessCastleStatus : Packet
{
	BlessCastleStatusShort					sBlessCastleStatus;
};

struct PacketHealing : Packet
{
	//LParam - 0
	//WParam - Healing amount
	//SParam - Target ID
	//EParam - Caster ID
	int iNotUsed;
	int iHealAmount;
	int iTargetID;
	int iCasterID;
};

struct PacketGrandHealing : Packet
{
	int iHealAmount;
	int iSkillLevel;
	int iRange;
	int iCasterID;
};

/**
TRANS_ATTACKDATA
For single target only
*/
struct PacketAttackData : Packet
{
	DWORD		dwDestObjectSerial;		 //0x08	Unique number of the attacking character
	DWORD		dwTarObjectSerial;		 //0x0C	Unique number of the target character

	Point3D		sPosition;				 //0x10	Location
	union
	{
		int				iAttackState;	 //For monsters (in assembly) this is used for multiple things..
		int				iQuestData;		 //For monsters with quest ids
	};
	int			iRange;					 //0x20	AttackSize / Attack range
	int			iDamage;				 //0x24	Power / Attack power
	DWORD		dwChkSum;				 //0x28
	short		sRating[ 2 ];			 //0x2C	[0] - level, [1] - accuracy
	DWORD		dwDamageChkSum;			 //0x30
};

/**
TRANS_ATTACKDATA
For single target only
*/
struct PacketAttackDataEx : PacketAttackData
{
	EAttackState		eAttackState;	 //0x1C	EAttackState from EAttackState enum
	EAttackProperty		iAttackProperty; //0x1E	Property (resistance, elemental damage etc)
};

//Note - this struct has to be 8 bytes total
struct ElementalAttackSetting
{
	BYTE							bSpare;			//spare

	BYTE							bDurationSec;	//duration
	BYTE							bChance;		//Chance to apply elemental attack (0 - 100)

	//parameters (1 byte)
	union
	{
		BYTE						bSpeedReduction; //speed reduction in % (0 - 100)
		BYTE						bParameter;
	};

	//parameters (2 bytes)
	union
	{
		short						sParameter;		  //parameter
		short						sLifeStealPercent; //life steal
		short						sDamageReduction; //damage reduction in % (0 - 100)
		short						sStack;			  //first 1 byte = num stack given, last 1 byte = max stack given
	};

	//unused / spare
	short							sPercentOfAP;	  //percentage of base attack damage (0 - 100)
};


struct LifeStealParam
{
	int						iLifeStealPercent; //life steal
	int						iLifeStealHealthPointCap;
};

struct PacketNPCClick : Packet
{
	DWORD		dwTarObjectSerial;		//Unique number of the target character
	int			iNPCId;
	int			iQuestData;				//for monsters / npcs that have quest codes associated to them
};

/**
TRANS_ATTACKDATA2
Damage data received from player
Melee basic and skill attack
Ranged basic attack
*/
//was PacketAttackDamageEx
struct PacketSingleTargetSkillData : Packet
{
	DWORD									dwChkSum;

	DWORD									dwDestObjectSerial;		//Unique number of the attacking character
	DWORD									dwTarObjectSerial;		//Unique number of the target character

	Point3D									sPosition;				//Location

	EAttackState							eAttackState;			// EAttackState
	EAttackProperty							iAttackProperty;		// AttackProperty

	int										iRange;					// AttackSize - Attack Range

	short									AttackPowerMin;			// Power - Attack Power (min)
	short									AttackPowerMax;			// Power - Attack Power (max)
	union
	{
		short								AttackParameter1;		// Attack parameter 1
		short								WeaponAttackPowerMin;	// Weapon Attack Power min (not used for anything on server side??)
	};
	union
	{
		short								AttackParameter2;		// Attack parameter 2
		short								WeaponAttackPowerMax;	// Weapon Attack Power max (not used for anything on server side??)
	};
	short									sCriticalChance;		// Critical chance
	short									sCriticalDmgBoost;		// Critical damage boost
	int										iSkillCode;				// Skill code (tiny skill id incl params)
	DWORD									dwTime;					// Client time
	int										AttackCount;			// Attack counter
	short									MotionEventNum;			// Attack motion counter of the skill
	short									sPrimaryStats;			//
	DWORD									dwWeaponCode;			// Main attack weapon
	short									StageId;				// Current stage / map id
	short									spare2;					//2 bytes spare

	//8 bytes total
	union
	{
		ElementalAttackSetting				sElementalAttackSetting;
		LifeStealParam						sLifeStealParam;
		short								saParametersEx[4];
		int									iaParametersEx[2];
	};

	int										iQuestData;				// For monsters / npcs that have quest codes associated to them
	int										iAttackRating;			// New - Attack rating of player
};

/// <summary>
/// Extended version of PacketSingleTargetSkillData
/// for serverside use only
/// </summary>
struct PacketSingleTargetSkillDataEx : PacketSingleTargetSkillData
{
	EAttackState						eSecondaryAttackState;
	EAttackProperty						eSecondaryAttackProperty;
	ElementalAttackSetting				sSecondaryElementalAttackSetting;
};


/**
TRANS_SKIL_ATTACKDATA
*/
struct PacketSkillDamageRange : Packet
{
	int										iTargetReceiveDamageID;	//Unique number of the attacking character

	Point3D									sPosition;				// x,y,z - Location
	EAttackState							eAttackState;			// EAttackState
	EAttackProperty							iAttackProperty;		// AttackProperty
	int										iRange;					// AttackSize - Attack range - based on sPosition which is usually the target
	int										iDamage;				// Power - Attack power
	DWORD									dwChkSum;				// Tamper proof code

	int										iCount;
	int										iaTargetID[ 128 ];		//Unique number of the target character
};

/**
TRANS_SKIL_ATTACKDATA2
Ranged skill attack
Was PacketRangedSkillDamageEx
*/
struct PacketMultiTargetSkillData : Packet
{
	int										iTargetReceiveDamageID;		//Unique number of the attacking character (dwDestObjectSerial)

	DWORD									dwChkSum;

	Point3D									sPosition;					//Location

	int										iDamageState;
	int										iRange;						//Attack range
	MinMax									sAttackPower;				//Attack power (Base)
	short									WeaponAttackPower[2];		//Attack Power (Additional Attack)
	short									sCriticalChance;			//Critical chance
	short									sCriticalDmgBoost;			//Critical damage boost
	int										iSkillID;					//Skill code (use sActiveSkill)
	DWORD									dwTime;						//Client time
	int										iDamageCount;				//Attack counter
	short									MotionCount[ 2 ];			//Attack motion counter
	DWORD									dwWeaponCode;				//Main attack weapon

	short									sMapID;
	EAttackProperty							eSecondaryAttackProperty;	//Usually from chained skills, if we don't want to change the primary attack property

	//8 bytes total
	union
	{
		ElementalAttackSetting				sElementalAttackSetting;
		LifeStealParam						sLifeStealParam;
		short								saParametersEx[4];
		int									iaParametersEx[2];
	};

	EAttackState							eSecondaryAttackState;	//Usually from chainned skills, if we don't want to change the primary attack state
	short									sAttackRange;

	short									sAttackRating;
	short									sPrimaryStats;
	ID										iTargetID;

	int										iTargetCount;
	int										iaTargetID[ 128 ];			///Unique number of the target character (max 128)


};

struct PacketUserOnline : Packet
{
	int										iType;
	int										iID;
	BOOL									bOnline;
	char									szName[32];
};

struct PacketHolyMind : Packet
{
	int										iTime;
	int										iAbsorbPercent;
	DWORD									dwObjectID;
};

struct PacketTeleportEvent : Packet
{
	int										iTeleportEventID;
};

struct PacketWisp : Packet
{
	int										iTime;
	int										iAbsorbPercent;
	DWORD									dwObjectID;
};

struct PacketSkillCancel : Packet
{
	int				iUnk;
	int				iPlayID;
	int				iUnk2;
	int				iUnk3;
};


struct PacketSimple : Packet
{
	int										iUnk;
	int										iUnk2;
	int										iUnk3;
	int										iUnk4;
};

struct PacketEventGirl : Packet
{
	BOOL									bFreeGold;
};

struct PacketAgingEvent : Packet
{
	BOOL									bFreeAging;
	BOOL									bNoBreak;
	BOOL									bHalfPrice;
};

struct PacketEventGirlReset : Packet
{
	BYTE				bResetType; //0 = full stats reset, 1 = full skill resets, 2 = individual stat reset
	BOOL				baReset[5] = { FALSE, FALSE, FALSE, FALSE, FALSE };
	int					iTotalCost;
};

struct PacketEventGirlResetResult : PacketEventGirlReset
{
	BOOL				bResult; //true if success
};

struct PacketDeadCharacter : Packet
{
	int										iUnk;
	int										dwObjectID;
};


struct PacketPvPData : Packet
{
	int										iPVPKills;
	int										iPVPDeaths;
	int										iPVPKillStreak;
	BOOL									bKill;
	int										iID;
};

enum EPvPKillType
{
	PVPKILLTYPE_None,
	PVPKILLTYPE_DoubleKill,
	PVPKILLTYPE_TripleKill,
	PVPKILLTYPE_QuadraKill,
	PVPKILLTYPE_PentaKill,
	PVPKILLTYPE_Shutdown,
	PVPKILLTYPE_Rampage,
	PVPKILLTYPE_Unstoppable,
	PVPKILLTYPE_COUNT,

};

struct PacketPvPKillType : Packet
{
	char				szKillerName[32];
	char				szVictimName[32];

	int					iKillerClass;
	int					iVictimClass;

	int					iKillType;
};


struct PacketHalloweenEventStatus : Packet
{
	BOOL									bStatus;
};

struct PacketUpdatePetData : Packet
{
	BOOL									bAutoAttack;
	int										dwSkillID;
};

struct PacketUpdatePetDataEx : Packet
{
	int										iTargetID;
	int										iLastTargetID;
};

struct SkillArrayData
{
	int										iSkillID;
	DWORD									dwSkillArrayAddressOrIndex;

	union
	{
		int									iSkillValues[10];
		float								fSkillValues[10];
	};
};

/// <summary>
/// Skill info for party buffs etc
/// </summary>
struct SkillInfoCommon
{
	int									iSkillId;
	char                                szSkillName[32];			//Skill name
	char                                szSkillFileName[32];		//Skill file name (usually for skill icons)
	char                                szSkillIconName[32];		//Skill icon name (usually for timers)
	char                                szSkillDescription[150];	//Skill description
};

struct PacketSkillDataContainer : Packet
{
	int				  iCharacterClass;
	int				  iCount;

	SkillArrayData    skillArrayData[MAX_SKILL_ARRAY_DATA]; //100 skills attributes total per class (only 4800 total)
};

struct PacketCommonSkillInfoContainer : Packet
{
	int				  iCount;

	SkillInfoCommon   saData[MAX_COMMON_SKILL_INFO_DATA]; //25 common skill info data for all classes
};


struct SkillInfo
{
	int									iSkillNum;					//1-based
	char                                szSkillName[32];			//Skill name
	char                                szSkillFileName[32];		//Skill file name
	char                                szSkillIconName[32];		//Skill icon name
	char                                szSkillDescription[150];	//Skill description
	int									iSkillId;
	BYTE								iSkillReqLevel;
	BYTE								iSkillType;
	DWORD								dwFuncPointer;
	DWORD								dwManaPointer;
	BYTE								iFlinchChance;				//0 - 100 - chance to flinch.   0 = no flinch
	BYTE								iPushbackRange;				//0 - 100 - flinch pushback range. 0 = no pushback
	short                               sSkillUseStaminaBase;		//0 = stamina value static
	short                               sSkillUseStaminaPerLevel;	//1 = increase/decrease value each level skill
	short                               sSkillMasteryBase;			//0 = mastery base
	short                               sSkillMasteryPerLevel;		//1 = increase/decrease value each skill level
    int									iaItemAllowedType[8];
    float								pvpScaling;
};



struct SkillChainScaleData
{
	DWORD	dwSkillArrayPointer;
	float	fScale; //scale factor for this skill array pointer
};

//General multi-purpose container
struct PacketGenericContainer : Packet
{
	int				  iCount;
	int				  iParam;
	BYTE			  baBuffer[0x1FF0]; //0x2000 - 0x8 - 4 - 4
};

struct PacketSkillInfoContainer : Packet
{
	int		  iCharacterClass;
	int		  iCount;
	SkillInfo saSkillInfo[16]; //Tier 1 to Tier 4 skills
};


struct PacketLogCheat : Packet
{
	int										LParam;
	int										iCheatID;
	int										SParam;
	int										EParam;

	int										LxParam;
	int										WxParam;
	int										SxParam;
	int										ExParam;

#if defined(_SERVER)
	char									szBuffer1[256] = {0};
	char									szBuffer2[32] = {0};
#endif
};

//smTRANS_COMMAND
struct PacketTransCommand : Packet
{
	int LParam; //0x08 - 8
	int WParam; //0x0C - 12
	int SParam; //0x10 - 16
	int EParam; //0x14 - 20
};
struct PacketTransCommandEx : Packet
{
    int LParam;
    int WParam;
    int SParam;
    int EParam;
    int LxParam;
    int WxParam;
    int SxParam;
    int ExParam;
};

struct PacketLogCheatEx : Packet
{
	int										LParam;
	int										iCheatID;
	int										SParam;
	int										EParam;

	int										LxParam;
	int										WxParam;
	int										SxParam;
	int										ExParam;

	char									szParam[256];
};

//                      Siege War

struct PacketSiegeWarPhase : Packet
{
	int										iPhaseID;
};

enum ELogCheat
{
	CHEATLOGID_DelaySkillHack						 = 52,
	CHEATLOGID_UnsureButUsed						 = 54,
	//CHEATLOGID_UnsureButUsed3						 = 80, -- reviewing.
	CHEATLOGID_ValidateCharacterError				 = 81,
	CHEATLOGID_CopiedItemError						 = 99,
	CHEATLOGID_UnsureButUsed2						 = 123,
	CHEATLOGID_CopiedItem							 = 1000,
	CHEATLOGID_CopiedItemWarehouse					 = 1010,
	CHEATLOGID_WarehouseGold						 = 1011,
	CHEATLOGID_WarehouseNewClone					 = 1012,
	CHEATLOGID_WarehouseBug							 = 1013,
	CHEATLOGID_HackDetected							 = 1100,
	CHEATLOGID_FocusChanged							 = 1101,

	CHEATLOGID_GoldLimit							 = 1150,
	CHEATLOGID_SpeedHack							 = 1200,
	CHEATLOGID_TimeErrorSpeedHack					 = 1250,
	CHEATLOGID_TimeMismatch							 = 1400,
	CHEATLOGID_AttackRatioError						 = 1500,
	CHEATLOGID_PotionCheckError						 = 1510,
	CHEATLOGID_PotionCountError						 = 1530,

	CHEATLOGID_CharInfoSaveError					 = 1600,
	CHEATLOGID_DefaultAttackRatingError				 = 1820,
	CHEATLOGID_DefaultAttackSizeError				 = 1821,
	CHEATLOGID_ContinuousAttackError				 = 1823,
	CHEATLOGID_SkillAttackRatingError				 = 1830,
	CHEATLOGID_SkillContinuousAttackError			 = 1833,
	CHEATLOGID_RestrictedAreaTrespassed				 = 1840,
	CHEATLOGID_WeightError							 = 1900,

	CHEATLOGID_ItemError							 = 1950,
	CHEATLOGID_CopiedItemRecall						 = 1960,
	CHEATLOGID_ForcedPenaltyBoot					 = 2000,
	CHEATLOGID_SavedItemError						 = 2400,
	CHEATLOGID_ContinuousSaveFailedError			 = 2700,
	CHEATLOGID_AccountCharacterError				 = 2800,
	CHEATLOGID_TradeAuthorizationError				 = 4000,
	CHEATLOGID_MoneyTransferError					 = 4100,
	CHEATLOGID_MultipleConnectionsIP				 = 5000,
	CHEATLOGID_TooManyPackets						 = 5100,
	CHEATLOGID_GameServerIPError					 = 5200,
	CHEATLOGID_StartCharacterError					 = 6000,
	CHEATLOGID_ServerMoneyError1					 = 6610,
	CHEATLOGID_ServerMoneyError2					 = 6611,
	CHEATLOGID_ServerMoneyError3					 = 6612,
	CHEATLOGID_ItemCodeError						 = 6800,
	CHEATLOGID_ItemTempCodeError					 = 6810,
	CHEATLOGID_CharacterStateError1					 = 6900,
	CHEATLOGID_CharacterSkillPointError				 = 6910,
	CHEATLOGID_CharacterWeightError					 = 6920,
	CHEATLOGID_ClientProcessTimeOut					 = 7000,
	CHEATLOGID_InventoryItemError					 = 7010,
	CHEATLOGID_CharacterModelError					 = 7020,
	CHEATLOGID_JobCodeError							 = 7030,
	CHEATLOGID_HighDmgRecordedAgainstHigherLevelMon  = 7031,

	CHEATLOGID_ClientAttackDefenseError				 = 7100,
	CHEATLOGID_ClientEnergyBarError					 = 7110,
	CHEATLOGID_CopiedItemFromFloor					 = 7130,
	CHEATLOGID_TriedConnectDisableIP				 = 7140,
	CHEATLOGID_ItemErrorInventory					 = 7150,
	CHEATLOGID_MoneyErrorInventory					 = 7160,

	CHEATLOGID_ServerItemErrorInventory				 = 8000,
	CHEATLOGID_ItemErrorInventoryRecord				 = 8001,
	CHEATLOGID_ServerMoneyErrorInventory			 = 8010,
	CHEATLOGID_ServerToServerItemError				 = 8020,
	CHEATLOGID_WrongSavingCharacterName				 = 8030,
	CHEATLOGID_ItemPositionError					 = 8040,
	CHEATLOGID_ServerInventoryUsedFull				 = 8050,
	CHEATLOGID_UsedItemCodeWarning					 = 8060,
	CHEATLOGID_ServerInventoryCopiedItem			 = 8070,
	CHEATLOGID_ServerCopiedItemWarehouse			 = 8071,
	CHEATLOGID_MemoryBufferSavingError				 = 8080,
	CHEATLOGID_WarningAutoMouse						 = 8100,
	CHEATLOGID_WarningMacroMouse					 = 8102,
	CHEATLOGID_WarningAvgDamageDefense				 = 8110,
	CHEATLOGID_WarningAvgDamageAttack				 = 8120,
	CHEATLOGID_AgingFailedCopiedItemSheltom			 = 8200,
	CHEATLOGID_AgingFailedCopiedItem				 = 8210,
	CHEATLOGID_AgingFailedNotEnoughMoney			 = 8211,	//User didn't have enough money on server, somehow..
	CHEATLOGID_ReconnectServer						 = 8300,
	CHEATLOGID_CheckedInventoryData					 = 8400,
	CHEATLOGID_TeleportFieldHack					 = 8401,
	CHEATLOGID_DamagePacketError					 = 8500,
	CHEATLOGID_LimitDamageOver						 = 8510,
	CHEATLOGID_ClientWarningMotionSpeed				 = 8520,
	CHEATLOGID_ClientWarningSkillAttack				 = 8530,
	CHEATLOGID_ClientWarningSkillForgery			 = 8540,
	CHEATLOGID_ClientWarningSkillForgery2			 = 8550,
	CHEATLOGID_InitialCharacterLevelError			 = 8560,
	CHEATLOGID_TooManyUpdatedCharInfo				 = 8570,
	CHEATLOGID_ClientDamagePacketError				 = 8580,
	CHEATLOGID_WarningInvincibleMode				 = 8600,
	CHEATLOGID_FindThreadHack						 = 8720,
	CHEATLOGID_CharacterStateError2					 = 8730,
	CHEATLOGID_ServerPotionError					 = 8800,
	CHEATLOGID_ServerPotionMovingError				 = 8810,
	CHEATLOGID_LevelQuestCodeWarning				 = 8820,
	CHEATLOGID_ShopBuyForgeryItem					 = 8830,
	CHEATLOGID_ServerMoneyOverflow					 = 8840,
	CHEATLOGID_SkillUsedLevelError					 = 8850,
	CHEATLOGID_CheckPlayFieldWarning				 = 8860,
	CHEATLOGID_CompareClanCodeError					 = 8870,
	CHEATLOGID_ModelFileError						 = 8880,
	CHEATLOGID_WarningCharacterReloading			 = 8890,
	CHEATLOGID_FieldNPCWarning						 = 8900,
	CHEATLOGID_LevelHack							 = 8901,
	CHEATLOGID_ItemMeshError						 = 8910,
	CHEATLOGID_IllegallyServerConnect				 = 8920,

	CHEATLOGID_MatureHack							 = 8950,
	CHEATLOGID_AgingHack							 = 8951,
	CHEATLOGID_FakeGM								 = 8952,
	CHEATLOGID_RunSpeedHack							 = 8953,
	CHEATLOGID_CheckSumFunctionError				 = 8954,
	CHEATLOGID_WindowHack							 = 8955,
	CHEATLOGID_StateCharacterHack					 = 8956,
	CHEATLOGID_DebuggerHack							 = 8957,
	CHEATLOGID_GMReason								 = 8958,
	CHEATLOGID_BellatraGoldHackLeader				 = 8959,
	CHEATLOGID_DefenseMultHack						 = 8960,
	CHEATLOGID_RegenFormulaHack						 = 8961,
	CHEATLOGID_EasterEggFakeItem					 = 8962,
	CHEATLOGID_RespecFakeItem						 = 8963,
	CHEATLOGID_NPCItemShopFakeItem					 = 8964,
	CHEATLOGID_ItemTimerFakeInventory				 = 8965,
	CHEATLOGID_ManufactureItemFake					 = 8966,
	CHEATLOGID_SkillCooldown						 = 8967,
	CHEATLOGID_ChristmasRudolphFakeBox				 = 8968,
	CHEATLOGID_ProcessHook							 = 8969,
	CHEATLOGID_ModuleSyncError						 = 8970,
	CHEATLOGID_EXEModuleSyncError					 = 8971,
	CHEATLOGID_MultipleWindowHackProcess			 = 8972,
	CHEATLOGID_ItemFakeBoughtShopID					 = 8973,
	CHEATLOGID_ItemFakeBoughtShopItem				 = 8974,


	CHEATLOGID_MemorySaveNameError1					 = 10000,
	CHEATLOGID_MemorySaveNameError2					 = 10001,
	CHEATLOGID_MemorySaveNameError3					 = 10002,
	CHEATLOGID_MemorySaveNameError4					 = 10003,
	CHEATLOGID_MemorySaveNameError5					 = 10005,
	CHEATLOGID_InitialSaveMemoryError				 = 10006,
	CHEATLOGID_SaveBufferPointError					 = 10007,
	CHEATLOGID_CharacterAccountMismatch				 = 10010,
	CHEATLOGID_AuthenticationError					 = 10020,
	CHEATLOGID_ShutdownService						 = 11000,
	CHEATLOGID_EventHighScore						 = 21000,


	CHEATLOGID_GoldHackNPC							 = 11037,
	CHEATLOGID_GoldHackWH							 = 11034,
	CHEATLOGID_MixingItemHack						 = 99000,
	CHEATLOGID_TimingSpeedHack						 = 99001,
	CHEATLOGID_ThrowGoldHack 						 = 99002,
	CHEATLOGID_TradeGoldHack						 = 99003,
	CHEATLOGID_GenerateGoldHack						 = 99004,
	CHEATLOGID_OnDeathCheat  						 = 99005,
	CHEATLOGID_OnDeathCheatExp						 = 99006,
	CHEATLOGID_GoldLimitReached 					 = 99007,
	CHEATLOGID_StatsHacking      					 = 99008,
	CHEATLOGID_InvalidWHItem						 = 99009,
	CHEATLOGID_DuplicateItemsFoundInWHa 			 = 99010,
	CHEATLOGID_DuplicateItemsFoundInWHb				 = 99011,
	CHEATLOGID_DuplicateWHItemFoundInPlayerInventory = 99012,
	CHEATLOGID_MixingItemNotFound					 = 99013,

	CHEATLOGID_NPCBuyItemNotEnoughGold				 = 99020,
	CHEATLOGID_NPCBuyItemNotValid					 = 99021,
	CHEATLOGID_NPCBuyItemCountIsNotValid			 = 99022,
	CHEATLOGID_NPCBuyItemNameMismatch				 = 99023,
	CHEATLOGID_NPCBuyItemPriceMismatch				 = 99024,
	CHEATLOGID_NPCBuyItemIdNotFound					 = 99025,
	CHEATLOGID_NPCBuyPotionNotEnoughGold			 = 99026,
};

/* Net Server */
struct PacketNetIdentifier : Packet
{
	int										iServerID;
	int										iPort;

	UINT									uPasswordNUM; //= 0x33445566
};
struct PacketNetUnloadUser : Packet
{
	DWORD									dwObjectID;
};

/// <summary>
/// For syncing game server / login server w.r.t player mute status
/// </summary>
struct PacketNetMuteSync : Packet
{
	DWORD									dwObjectID;
	BOOL									bMuted;
	DWORD									dwUnMuteExpiryTime;
};

struct PacketNetDisconnectUser : Packet
{
	DWORD									dwObjectID;
};
struct PacketNetForceOrbTime : Packet
{
	DWORD									dwObjectID;
	BOOL									bUpdate;
	DWORD									dwTime;
};
struct PacketNetExp : Packet
{
	UINT									uObjectID;
	INT64									iExp;
};



struct PacketNetExpEvent : Packet
{
	int						iPercent;
};

struct PacketNetUsersOnline : Packet
{
	int						iID;
	int						iUsersOnline;
};

struct PacketNetEXPPotion : Packet
{
	DWORD									dwObjectID;
	int										iEXPPotion;
};

struct PacketNetClan : Packet
{
	UINT									uObjectID;
	DWORD									dwClan;
};
struct PacketNetGetExp : Packet
{
	UINT									uObjectID;
};
struct PacketNetPingPong : Packet
{
	int										iNothing;
};
struct PacketNetCharacterSync : Packet
{
	DWORD									dwObjectID;
	char									szCharName[32];
};
struct PacketNetGameMaster : Packet
{
	UINT									uObjectID;
	int										iGameMasterLevel;
};
struct PacketNetGetGameMaster : Packet
{
	UINT									uObjectID;
	int										iGameMasterLevel;
	char									szName[32];
};

struct PacketNetVersion : Packet
{
	int										iVersion;
};

struct PacketNetBCNewOwnerClanID : Packet
{
	int									iClanID1;
	int									iClanID2;
	int									iClanID3;

};

struct PacketNetT5QuestData : Packet
{
	int										iID;

	UINT									uQuestID;
	USHORT									sQuestValue[2];
};


struct PacketNetPlayerWorldToken : Packet
{
	char Token[65];
	char TokenPass[65];
};

struct DropItemData
{
	int										iItemID;
	int										iChk1;
	DWORD									iChk2;
};

struct PacketNetPlayerInventory : Packet
{
	int ObjectID;
	DropItemData sIntentoryItems[316];
};

struct PacketNetPlayerGold : Packet
{
	int ObjectID;
	int iGoldIn;
	int iGoldOut;
	int iInventoryGold;
	int iSaveGold;
	int sCharacterData_iGold;
};

struct PacketNetPlayerGoldDiff : Packet
{
    int ObjectID;
    int Diff;
};

struct PacketNetAskPlayerData : Packet
{
    int ObjectID;
};


struct PacketNetPlayerThrow : Packet
{
	int ObjectID;
	DropItemData Item;
	int Gold;
};

struct PacketNetPlayerItemPut : Packet
{
    int ObjectID;
    int ExceptedHash;
};

struct PacketNetEvent : Packet
{
	BOOL Enabled;
};

struct PacketNetAgingEvent : Packet
{
	BOOL FreeEnabled;
	BOOL NoBreakEnabled;
	BOOL HalfPrice;
};

struct PacketNetStartMaintenance : Packet
{
	int Seconds;
};

struct PacketNetTradeCoin : Packet
{
	int										iID;
	int										iReceiverID;
};

enum EItemRarity;

struct PacketGetItemPerfectView : Packet
{
	DWORD		 dwCode;
	int			 iItemSpec;
	short		 sAgeLevel;
	int			 iMixType;
	USHORT		 sMixColor;
	short		 sUniqueMixId;
	int			 iMixEffect;
	EItemRarity	 eItemRarity;
#ifdef ITEMFIXES
	int			 iPrefixID;
	int			 iSuffixID;
#endif
	int			 iDefenseValue; //to fix display bug due to % based aging
};

struct PacketListUsingItem : Packet
{
	struct ItemUsing
	{
		int									iItemID;
		short								sPerformance[8];
	};
	int										iItemListCount;
	ItemUsing								sItemList[16];
};

//Timers

struct PacketOldHeadUse : Packet
{
	char				szHeadModel[64];

	BOOL				bUpdate;

	PacketOldHeadUse() {};
	PacketOldHeadUse( BOOL bUpdateHead ) { bUpdate = bUpdateHead; }
};

struct PacketNewItemTimer : Packet
{
	EItemTimerType		iType;

	EItemID				iItemID;

	int					iChk1;
	int					iChk2;

	DWORD				dwTimeLeft;
	DWORD				dwTimeTotal;

	PacketNewItemTimer( EItemID iItemID, int iChk1, int iChk2 ) { this->iItemID = iItemID; this->iChk1 = iChk1; this->iChk2 = iChk2; }
	PacketNewItemTimer() { }
};

struct PacketCancelItemTimer : Packet
{
	EItemTimerType		iType;

	EItemID				iItemID;
};

//Record
struct PacketRecordGameDataUpdate : Packet
{
	BOOL				bUpdate;
};


//Quest Arena T5
struct PacketQuestArenaTier5 : Packet
{
	enum EType
	{
		TYPEID_None,
		TYPEID_EnterArena,
		TYPEID_Round,
		TYPEID_FinishQuest,
		TYPEID_Lost,
	};

	int						iType;

	int						iRound;
	DWORD					dwTimeLeft;

	PacketQuestArenaTier5() { this->iType = 0; this->iRound = 0; this->dwTimeLeft = 0; };
	PacketQuestArenaTier5( int iType, int iRound, DWORD dwTimeLeft ) { this->iType = iType; this->iRound = iRound; this->dwTimeLeft = dwTimeLeft; };
	~PacketQuestArenaTier5() {};
};

struct PacketQuickHealthUpdate : Packet
{
	ID dwID;
	int sHP;
};

struct PacketNeverSinkTeleportRequest : Packet
{
	ID fieldID;
};

struct PacketNeverSinkOpenUI : Packet
{

};

struct PacketQuestNamesRequest : Packet
{

};