#pragma once

#include "EXEMatrix.h"

#if defined(_GAME)
#include <mutex>
#endif

#define MONSTER_SERVER_MAX			800

#define	ATTACK_DAMAGE_MAX			100

#define UNIT_DEFAULT_LURE_DISTANCE_FAR	1638400


class EXEMesh;
class EXEModel;
class Map;

class UserData;

struct EXEModelData
{
	EXEModelData			* psUnused;

	EXEModelData			* psMotion;
	EXEModel				* pcModel;

	char					szModelName[64];
	int						iCount;

	DWORD					dwCheckSum;

	struct ModelData		* psModelData;

	DWORD					dwLastTimeUpdate;
};

struct DamageInfo
{
	EDamageTextType			iType;
	DWORD					dwTime;

	int						iValue;
	short					sParameter;
	int						iDecreaseY;
};



struct AttackDamageData
{
	int					iID;
	int					iDamage;
	int					iCounter;
};

enum EUnitDataMiscCommand : int
{
	COMMANDID_ShowSuccessAgingOrMixSkillAnimation,	//Player the skill animation effect on success mix / aging

};

enum ENPCID : int
{
	NPCID_EventGirl							= 1,
	NPCID_ItemDistributor					= 2,
	NPCID_Warehouse_DoNotUse				= 3,	//obsolete, replaced by 30 and 31
	NPCID_Force								= 4,
	NPCID_CastleControl						= 5,
	NPCID_Aging								= 6,
	NPCID_Smelting							= 7,
	NPCID_ClanControl						= 8,
	NPCID_Teleport							= 9,
	NPCID_SkillMaster						= 10,
	NPCID_Mixing							= 11,
	NPCID_Manufacturing						= 12,
	NPCID_WarpGate							= 13,
	NPCID_Bellatra							= 14,
	NPCID_ResetItem							= 15,
	NPCID_CoinShop							= 16,
	NPCID_FuryArenaNPC						= 17,
	NPCID_FuryArenaNPCEnter					= 18,
	NPCID_SocketSystemNPCDrill				= 19,
	NPCID_SocketSystemNPCStone				= 20,

	NPCID_NPCEasterEgg						= 21,

	NPCID_LarryQuests						= 22,
	NPCID_IhinEvent							= 23,
	NPCID_MarinaQuests						= 24,
	NPCID_RudolphChristmas					= 25,

	NPCID_MIXGUILD							= 26,
	NPCID_RANKLIST							= 27,
	NPCID_SOLORANKLIST						= 28,

	NPCID_Warehouse							= 30,
	NPCID_Warehouse_Seasonal				= 31,
};

enum ENPCEventCode : int
{
	NPCEVENTCODE_Force						= 200,
	NPCEVENTCODE_Smelting					= 2,
	NPCEVENTCODE_Manufacture				= 3,
	NPCEVENTCODE_ResetItem					= 4,
	NPCEVENTCODE_CoinShop					= 5,
	NPCEVENTCODE_FuryArenaNPCTeleportCT2	= 6,
	NPCEVENTCODE_FuryArenaNPCEnter			= 7,
	NPCEVENTCODE_SocketSystemNPCDrill		= 8,
	NPCEVENTCODE_SocketSystemNPCStone		= 9,
	NPCEVENTCODE_EasterNPC					= 10,
	NPCEVENTCODE_LarryQuestsNPC				= 11,
	NPCEVENTCODE_IhinEvent					= 12,
	NPCEVENTCODE_MarinaQuestsNPC			= 13,
	NPCEVENTCODE_RudolphChristmasNPC		= 14,
	NPCEVENTCODE_MixGuild					= 15,
	NPCEVENTCODE_RankList					= 16,
	NPCEVENTCODE_SoloRankList				= 17,

	NPCEVENTCODE_Warehouse					= 20,
	NPCEVENTCODE_WarehouseSeasonal			= 21,
};

//See character.h
//CHRMOTION_STATE_STAND etc
enum EAnimationType : int
{
	ANIMATIONTYPE_None					= 0x0000,	//0x00		0
	ANIMATIONTYPE_Idle					= 0x0040,	//0x40		64		CHRMOTION_STATE_STAND
	ANIMATIONTYPE_Walking				= 0x0050,	//0x50		80		CHRMOTION_STATE_WALK
	ANIMATIONTYPE_Running				= 0x0060,	//0x60		96		CHRMOTION_STATE_RUN
	ANIMATIONTYPE_Sprint				= 0x0070,	//0x70		112
	ANIMATIONTYPE_Falling				= 0x0080,	//0x80		128		CHRMOTION_STATE_FALLDOWN
	ANIMATIONTYPE_Attack				= 0x0100,	//0x100		256		CHRMOTION_STATE_ATTACK
	ANIMATIONTYPE_Flinch				= 0x0110,	//0x110		272		CHRMOTION_STATE_DAMAGE
	ANIMATIONTYPE_Die					= 0x0120,	//0x120		288		CHRMOTION_STATE_DEAD
	ANIMATIONTYPE_Potion				= 0x0140,	//0x140		320		CHRMOTION_STATE_EAT
	ANIMATIONTYPE_Skill					= 0x0150,	//0x150		336		CHRMOTION_STATE_SKILL
	ANIMATIONTYPE_FallLow				= 0x0170,	//0x170		368		CHRMOTION_STATE_FALLSTAND
	ANIMATIONTYPE_FallHigh				= 0x0180,	//0x180		384		CHRMOTION_STATE_FALLDAMAGE
	ANIMATIONTYPE_Revive				= 0x0200,	//0x200		512		CHRMOTION_STATE_RESTART
	ANIMATIONTYPE_Special				= 0x0210,	//0x210		528		CHRMOTION_STATE_WARP
	ANIMATIONTYPE_Yahoo					= 0x0220,	//0x220		544		CHRMOTION_STATE_YAHOO
	ANIMATIONTYPE_Taunt					= 0x0230,	//0x230		560
	ANIMATIONTYPE_Hammer				= 0x0300,	//0x300		768		CHRMOTION_STATE_HAMMER (For sound classification only)
};


enum EToolEquipModeTown : int
{
	TOOLEQUIPMODETOWN_None				= 0,	//Wearing Weapon as if in Battle
	TOOLEQUIPMODETOWN_Back				= 1,	//Wearing Weapon on the Back
	TOOLEQUIPMODETOWN_BackCrossBow		= 2,	//Wearing Weapon on the Back for CrossBows
	TOOLEQUIPMODETOWN_BackBow			= 3,	//Wearing Weapon on the Back for Bows
};

enum ECharacterGender
{
	CHARACTERGENDER_None,
	CHARACTERGENDER_Male,
	CHARACTERGENDER_Female,
};

enum EActionPattern : int
{
	ACTIONMODE_ClientSelf				= 0,	//Main character
	ACTIONMODE_ServerMonster			= 5,	//Monster
	ACTIONMODE_ServerNPC				= 12,	//Npc
	ACTIONMODE_ClientUnit				= 99,	//Other client unit
	ACTIONMODE_ClientTarget				= 101,
};

enum EAnimationClassFlag : unsigned int
{
	ANIMATIONCLASSFLAG_None				= 0,

	ANIMATIONCLASSFLAG_Fighter			= 0x0001,
	ANIMATIONCLASSFLAG_Mechanician		= 0x0002,
	ANIMATIONCLASSFLAG_Archer			= 0x0004,
	ANIMATIONCLASSFLAG_Pikeman			= 0x0008,
	ANIMATIONCLASSFLAG_Atalanta			= 0x0010,
	ANIMATIONCLASSFLAG_Knight			= 0x0020,
	ANIMATIONCLASSFLAG_Magician			= 0x0040,
	ANIMATIONCLASSFLAG_Priestess		= 0x0080,
	ANIMATIONCLASSFLAG_Assassin			= 0x0100,
	ANIMATIONCLASSFLAG_Shaman			= 0x0200,
};

enum EPotionStatus : int
{
	POTIONSTATUS_HP						= 1,
	POTIONSTATUS_MP						= 2,
	POTIONSTATUS_SP						= 3,
	POTIONSTATUS_HPMPSP					= 4,
	POTIONSTATUS_EtherCore				= 0x40,

};

struct PlayBuff
{
	Point3D				  sPosition;				//0x00
	int					  iFrame;					//0x0C
	short				  saAngle[4];				//0x10	//[0] = unused, [1] = player angle (y), [2] = skill param, [3] = animation id
	short				  saEquippedItems[4];		//0x18  - Mounting item number
	ID					  lTargetID;				//0x20	- Target object code
}; //Size = 0x24

struct ModelAnimation
{
	enum EMapTypeFlag : unsigned int
	{
		MAPTYPEFLAG_None = 0,
		MAPTYPEFLAG_Town = 1,
		MAPTYPEFLAG_BattleField = 2,
	};

	EAnimationType		  iType;					//0x00	State;		Valid if the status is TRUE

	int					  iKey1;					//0x04	MotionKeyWord_1
	int					  iBeginFrame;				//0x08	StartFrame
	int					  iKey2;					//0x0C	MotionKeyWord_2
	int					  iEndFrame;				//0x10	EndFrame

	int					  iaEventFrame[4];			//0x14

	int					  iItemIndexCount;			//0x24

	WORD				  eaItemIndex[52];			//0x28	ItemCodeList[MOTION_TOOL_MAX];

	EAnimationClassFlag	  eClassFlag;				//0x5C	dwJobCodeBit
	unsigned char		  uaSkill[8];				//0x60	SkillCodeList[MOTION_SKIL_MAX];

	EMapTypeFlag		  eMapTypeFlag;				//0x68 OR Flag
	BOOL				  bLoop;					//0x6C

	BYTE				  uaValue[4];				//0x70

	int					  iSubFileFrameNumber;		//0x74
}; //Size = 0x78

struct ModelAnimationOld
{
	enum EMapTypeFlag : unsigned int
	{
		MAPTYPEFLAG_None = 0,
		MAPTYPEFLAG_Town = 1,
		MAPTYPEFLAG_BattleField = 2,
	};

	EAnimationType		  iType;			//0x00

	int					  iKey1;					//0x04
	int					  iBeginFrame;				//0x08
	int					  iKey2;					//0x0C
	int					  iEndFrame;				//0x10

	int					  iaEventFrame[4];			//0x14

	int					  iItemIndexCount;			//0x24

	BYTE				  eaItemIndex[52];			//0x28

	EAnimationClassFlag	  eClassFlag;				//0x5C
	unsigned char		  uaSkill[8];				//0x60

	EMapTypeFlag		  eMapTypeFlag;				//0x68 OR Flag
	BOOL				  bLoop;					//0x6C

	BYTE				  uaValue[4];				//0x70

	int					  iSubFileFrameNumber;		//0x74
}; //Size = 0x78

struct ModelData
{
	struct MeshQuality
	{
		int				  iCount;
		char			  szMeshName1[16];
		char			  szMeshName2[16];
		char			  szMeshName3[16];
		char			  szMeshName4[16];
	};

	char				  szModelPath[64];					//ASE
	union
	{
		struct
		{
			int			  iID;
			int			  iWidth;
			int			  iHeight;
			int			  iDeathModelID;
			ModelData	* pDeathModelData;
		};

		char			  szBoneModelFilePath[64];			//ASE
	};
	char				  szDeathModelDataFilePath[64];		//INI

	MeshQuality			  sHighMeshQuality;
	MeshQuality			  sMediumMeshQuality;
	MeshQuality			  sLowMeshQuality;

	ModelAnimation		  saModelAnimation[512];
	int					  iNumModelAnimation;

	DWORD				  dwObfuscator1;
	DWORD				  dwObfuscator2;
	char				  szModelDataAnimationFile[64];		//INI

	char				  szTalkModelDataFilePath[64];		//INI
	char				  szTalkBoneModelFilePath[64];		//ASE
	ModelAnimation		  saTalkModelAnimation[30];
	unsigned int		  iTalkNumModelAnimation;

	int					  iaTalkUnknown1[30];
	int					  iaTalkUnknown2[100];
	int					  iaTalkUnknown3[30];
	int					  iaTalkUnknown4[2][100];
}; //Size = 0x1060C

struct ModelDataOld
{
	struct MeshQuality
	{
		int				  iCount;
		char			  szMeshName1[16];
		char			  szMeshName2[16];
		char			  szMeshName3[16];
		char			  szMeshName4[16];
	};

	char				  szModelPath[64];					//ASE
	union
	{
		struct
		{
			int			  iID;
			int			  iWidth;
			int			  iHeight;
			int			  iDeathModelID;
			ModelData	* pDeathModelData;
		};

		char			  szBoneModelFilePath[64];			//ASE
	};
	char				  szDeathModelDataFilePath[64];		//INI

	MeshQuality			  sHighMeshQuality;
	MeshQuality			  sMediumMeshQuality;
	MeshQuality			  sLowMeshQuality;

	ModelAnimationOld	  saModelAnimation[512];
	int					  iNumModelAnimation;

	DWORD				  dwObfuscator1;
	DWORD				  dwObfuscator2;
	char				  szModelDataAnimationFile[64];		//INI

	char				  szTalkModelDataFilePath[64];		//INI
	char				  szTalkBoneModelFilePath[64];		//ASE
	ModelAnimationOld	  saTalkModelAnimation[30];
	unsigned int		  iTalkNumModelAnimation;

	int					  iaTalkUnknown1[30];
	int					  iaTalkUnknown2[100];
	int					  iaTalkUnknown3[30];
	int					  iaTalkUnknown4[2][100];
}; //Size = 0x1060C

struct UnitTool
{
	EXEMesh							* pBone;	//smOBJ3D *ObjBip;
	EXEModel						* pModel;	//smPAT3D *PatTool;
	EItemID							  eItemID;  //DWORD	dwItemCode;

	int								  iSizeMax;
	int								  iSizeMin;

	int								  iBlinkSpeed;		//ColorBlink - 0 = disabled. [1,inf], higher is faster blink speed - ColorBlink
	short							  saBlinkColor[4];	//sColors[4] - BGRA - sColors
	BOOL							  bStarEffect;		//DispEffect - Stars circling around Weapon - DispEffect
	int								  iBlinkScale;		//Obsolete - BlinkScale
	int								  iUnusedZero;		//Always 0 - EffectKind

	int								  iOverlayTextureIndex;					//TexMixCode
	int								  iOverlayTextureTransformationType;	//TexScroll
}; //Size = 0x34

class Unit;

//character.h
//class smCHAR
class UnitData
{
private:

#if defined(_GAME)
	IMPFNC									pfnCloseUnitData			= 0x0041F3C0;
	IMPFNC									pfnHandleBufferData			= 0x00423440;
	IMPFNC									pfnAnimate					= 0x00429680;
	IMPFNC									pfnSetNextTarget			= 0x00000000;
	IMPFNC									pfnMain						= 0x0043CB20;
	IMPFNC									pfnSetPosition				= 0x0041F600;
	IMPFNC									pfnSetAngleByPosition		= 0x00000000;
	IMPFNC									pfnLoadModel				= 0x00000000;
	IMPFNC									pfnRender					= 0x00429AF0;
	IMPFNC									pfnGetAnimAttackPosition	= 0x004294D0;	//Leaked: GetAttackPoint
	IMPFNC									pfnInit						= 0x0041EF00;
	IMPFNC									pfnChangeAnimationID		= 0x0041FA04;
	IMPFNC									pfnUpdateAttack				= 0x0042B600;	//int smCHAR::PlayAttack( int EventFlag )
	IMPFNC									pfnMoveAngle				= 0x0044FFB4;	//int smCHAR::MoveAngle2( int step )
	IMPFNC									pfnCheckShootingTest		= 0x0041FDB0;	//int smCHAR::CheckShootingTest(smCHAR *lpChrTarget)
	IMPFNC									pfnSetTarget				= 0x00433E20;
	IMPFNC									pfnSetTargetEx				= 0x00000000;
	IMPFNC									pfnBeginSkill				= 0x0042CB40;
	IMPFNC									pfnResetEnergyGraph			= 0x0045D910;
#elif defined(_SERVER)
	IMPFNC									pfnCloseUnitData			= 0x004167A0;
	IMPFNC									pfnHandleBufferData			= 0x0041B780;
	IMPFNC									pfnAnimate					= 0x00416E30;
	IMPFNC									pfnSetNextTarget			= 0x00416A90;
	IMPFNC									pfnMain						= 0x00429930;
	IMPFNC									pfnSetPosition				= 0x00000000;
	IMPFNC									pfnSetAngleByPosition		= 0x00416A50;
	IMPFNC									pfnLoadModel				= 0x00429350;
	IMPFNC									pfnRender					= 0x00000000;
	IMPFNC									pfnGetAnimAttackPosition	= 0x00000000;
	IMPFNC									pfnInit						= 0x00000000;
	IMPFNC									pfnChangeAnimationID		= 0x00000000;
	IMPFNC									pfnUpdateAttack				= 0x00000000;
	IMPFNC									pfnMoveAngle				= 0x0041EB40;
	IMPFNC									pfnCheckShootingTest		= 0x00000000;
	IMPFNC									pfnSetTarget				= 0x0041E670;
	IMPFNC									pfnSetTargetEx				= 0x004454A0;
	IMPFNC									pfnBeginSkill				= 0x00000000;
	IMPFNC									pfnResetEnergyGraph			= 0x00000000;
#endif

public:
	//Attributes
	DWORD							dwHeader;

	BOOL                            bVisible;
	BOOL                            bVisibleEquipment;
	class Stage                     * pcStage;
	ID		                        iID;
	BOOL                            bRemoteControlled;

	EXEModel                        * pcBodyMeshContainer;	//Pattern;			//primary pattern
	EXEModel                        * pcHeadMeshContainer;	//Pattern2;			//hair pattern
	EXEModel                        * pcBoneContainer;		//AnimPattern
	EXEModel                        * pcTalkBoneContainer;	//TalkPattern;		//facial expression pattern. (Dynamic pattern is lpDinaPattern or lpDinaPattern2 depending on ini)

	EXEModelData                    * pcBody;			//lpDinaPattern;		//Dynamic Pattern Pointer (Pattern Manager)
	EXEModelData                    * pcHead;			//lpDinaPattern2;	//Dynamic Pattern Pointer Head (Pattern Manager)

	EXEModelData                    * pcLeftHandTool; //lpDinaLeftPattern;		//left weapon
	EXEModelData                    * pcRightHandTool; //lpDinaRightPattern;	//right weapon

	UnitTool						sLeftHandTool;	//HvLeftHand - tool worn on the left hand
	UnitTool						sRightHandTool;	//HvRightHand - tool worn on the right hand

	EXEMatrixI						sLeftHandMatrix;	//HvLeftHand_Matrix - Copy the left-handed weapon matrix
	EXEMatrixI						sRightHandMatrix;	//HvRightHand_Matrix - Copy Right Hand Weapon Matrix

	BOOL                            bRenderLeftHand;	//Rend_HvLeftHand - presence or absence of weapon rendering
	BOOL                            bRenderRightHand;	//Rend_HvRightHand - presence or absence of weapon rendering

#if defined(_GAME)
	EXEMatrixI						sWingMatrix;
	EXEMatrixI						sWingSkillMatrix;
#endif

	EXEMesh                         * pcaBackBone[3];
	EXEMesh                         * pcAttackBone;
	EXEMesh                         * pcShieldBone;

#if defined(_GAME)
	int								iPadding63[3];

	EXEMesh							* pcaWeaponDaggerBone[2];
#endif

	int                             iRange;				//AttackToolRange
	union
	{
		int							iFireOverlay;				//AttackAnger
		int							iHits;
	};
	int                             iIceOverlay;				//AttackIce - Received Ice Attack

	BOOL                            bActive;					//Flag

	Point3D							sPosition;					//Location (pX,pY,pZ)
	Point3D							sAttackPosition;			//Attack location (AttackX,AttackY,AttackZ)
	int                             iFloorHeight;				//Floor height coordinates
	int                             iLastHeight;				//Dropped height
	int                             iLoadedMapIndex;			//Floor field number
	int                             iMapType;					//Attribute value of floor field

	BOOL                            bNoMove;					//PatLoading - Currently loading

	int                             iModelWidth;				//Character width
	int                             iModelHeight;				//Character height

	int                             iShadowSize;				//Character size step (0 ~ 3)

	BOOL                            bCollision;					//OverLapPosi - Location overlaps with other characters
	BOOL                            bCheckCollision;			//OverLapPosi2 - Location overlaps with other characters
	UnitData						* pcCollisionUnitData;		//Point of overlapping character

	Point3D							sPosi;
	Point3D							sAngle;

	BOOL                            bRunMode;					//0 = walk, 1 = run
	BOOL                            bWalking;					//true if moving
	int                             iMoveCount;					//Distance limit counter when moving automatically when there is no data received during communication

	int                             iTargetX;
	int								iTargetZ;
	int                             iTargetMoveCount;

	int                             iWaterHeight;

	EItemID                         eWeaponItemID;					//dwActionItemCode - Code related to main movement
	EToolEquipModeTown              eToolEquipModeTown;				//dwItemSetting - Item setting method (0-no classification 1-village field division)
	BOOL							bIsWeaponTwoHanded;				//dwActionItemTwoHand - Two-handed weapon shank

	short                           saEquippedItems[4];				//wStickItems[4] - Mounting item number

	BOOL                            bProjectileMode;				//Fired Attack Mode (TRUE)
	int                             iProjectileType;				//Whether it is currently fired
	int                             iProjectileType2;				//Launch division
	Point3D                         sProjectilePosition;			//Firing position
	Point3D                         sProjectileAngle;				//Firing direction
	int                             iProjectileCount;				//Launch counter

	UnitData						* pcTarget;						//chrAttackTarget - Attack target character
	int                             iTargetCritical;				//AttackCritcal - Critical attack
	int                             iTargetCriticalCount;			//AttackCriticalCount - Critical attack counter (for continuous attack)
	int                             iAttackExp;						//Attack experience

	ActiveSkill                     sActiveSkill;					//Skill attack code
	int                             iAttackEffect;					//Show special effects when attacking

	int								iLastSkillParam;				//LastSkillParam - The latest skill attack classification code (added for soldering because of chaos)
	BYTE							bDamageReduction;				//was iRecvExp /RecvExp - not used
	BYTE							baSpare[3];						//Spare


	Point3D                         sMoveToPosition;				//Next move target
	Point3D                         sSpawnPosition;					//Surrounding location not to get away from this place (only nearby ..)
	int                             iLureDistance;					//DistAroundDbl - Perimeter boundary distance limit
	int                             iTargetTraceMode;				//Tracking mode value (use server)

	Point3D                         sMoveBeginPosition;				//Coordinate at the start of movement
	DWORD                           iMoveBeginCounter;

	EAnimationType                  eNextMotionCode;				//Schedule next action

	BOOL                            bIsCharging;

	int								iTalkFrame;						//Facial expression frame
	int								iTalkSoundIndex;

	int 							iStartFrame;
	int 							iEndFrame;
	int								iAction;						//action
	int 							iFrame;							//frame
	int								iFrameCnt;						//FrameCnt
	int 							iFrameStep;						//FrameStep - Frame change value (FPS)
	int                             iMovementSpeed;					//MoveSpeed - Movement value
	int                             iAttackSpeed;					//AttackSpeed - Speed of attack

#if defined(_GAME)
	int								iLastMovementSpeed;
#endif

	EActionPattern                  eActionPattern;					//Action pattern

	DWORD                           dwEventFrameExt[4];				//Event frame extension

#if defined(_GAME)
	int								iPoisonOverlay;					//unused / spare
	int								iSpareUnused;					//unused / spare
#else
	int								iAliveCounter;					//NearPlayCount - Check if there are other users nearby (NPC / Monster) - server only
	int                             iDeathCounter;					//Counter to reappear after monster time  - server only
#endif


	int                             iAutoMoveStep;					//Step counter when moving automatically
	short                           saMoveStepCount[2];				//Moving Step Loop Center

	int                             iLoopCounter;					//Counter
	int                             iFrameCounter;					//Counter used from start to end of motion

	BOOL                            bRendered;
	RECT                           	rRenderRectangle;
	Point3D                         sRenderPoint;
	BOOL                            bFlagShow;

	char                            szMessage[256];					//szChatMessage - Chat string
	DWORD                           dwMessageEndTime;				//dwChatMessageTimer - Display chat text and record end time

	char                            szPersonalShop[128];			//srTransBuff[TRANS_BUFF_SIZE];
	int								iPersonalShop;					//dwTradeMsgCode


	BYTE                            baUnitInfo[0x1000];				//srTransBuff - Buffer for waiting to send data
	UINT                            uUnitInfoSize;					//srTransBuffSize - Buffer size for waiting for data to be transmitted

	int                             iServerCode;					//ServerCode - Applicable character server code

	int                             iUnknownPointer;				//pointer to smWINSOCK

	int								iTransSendWait;					//Wait for communication port
	int								iTransLastSendCnt;				//Communication last transmission time
	int								iTransLastSendTime;				//Time difference of last transmission of communication (for speed check)
	int								iTransMinCnt;					//Minimum communication counter
	int								iTransLevel;					//Critical steps in communication
	int								iTransDelayMax;					//Maximum communication delay (retry after this time)

	PlayBuff						saPlayBuff[256];				//Stores the progress of play in the buffer and buffers it during communication
	UINT                            iPlayBuffCnt;					//PlayBuff Counter
	UINT                            iPlayBuffPosiEnd;				//PlayBuff buffering end position



	DWORD                           dwLastActiveTime;				//dwLastTransTime - Last communication time
	int                             iLastPlayDataType;				//Data type of last communication

#if defined(_GAME)
	CharacterData					sCharacterData;					//User information
	UnitInfo						sUnitInfo;						//Monster related structures
#else
	CharacterData					sCharacterData;
	UnitInfo						sUnitInfo;
#endif

	int                             iAnimDisplayMode;				//Operation Display Mode (0-All Display 1-One Display)
	int                             iPatDispMode;					//Pattern display mode (0-non-display 1-main 2-subman 3-all)
	int                             iMotionSelectFrame;				//Motion number of the current frame (0, 1)

	ModelData						* psBody;						//smMotionInfo - Frame and information for each operation of the model
	ModelData						* psHead;						//smMotionInfo2 - If there are two models, the primary model outputs data if there is no data
	ModelAnimation					* psModelAnimation;				//MotionInfo

	ModelData						* psTalkModelData;
	ModelAnimation					* psTalkModelAnimation;

	EAnimationType					iOldAnimationState;				//Previous motion / animation attribute information
	ID								lTargetID;						//dwTarget - When the opponent is an action aimed at the main character

	UserData                        * pcOwner;						//lpMasterPlayInfo - Host character (for server)
	UserData                        * pcFocusTarget;				//lpTargetPlayInfo - Target character (for server)
	UnitData                        * pcAttackTarget;				//lpTargetChar - Target character (for client)

	BOOL                            iAttackUserFlag;				//AttackUserFlag - Monster attacks the user

	DWORD                           dwIgnoreOthersTime;				//dwTargetLockTime - No change of target
	ID                           	lExclusiveTargetID;
	UserData                        * pcExclusiveTarget;			//lpLinkPlayInfo - connected player

	UnitData						* pcExclusiveUnitTarget;		//lpLinkChar - connected character

	UserData                        * pcPriorityAttacker;				//lpExpAttackPlayInfo
	int                             iDamageLastAttacker;			//ExpAttackLife
	DWORD                          	dwDamageLastTime;				//dwExpAttackTime

	PacketAttackData				sAttackTrans;					//AttackTrans - Attack data buffer
	DWORD                           dwAttackPlayTime;				//dwAttackPlayTime - Attack Time

	EPotionStatus                   iPotionStatus;					//PotionLog - Potion usage history
	int                             iLevelStatus;					//Level change record
	int                             iCriticalStatus;				//Critical Log

	BOOL                            bShowHP;						//EnableStateBar - Health bar drawing options

	int                             iMotionRecordCount;             //MotionRecordCount
	int                             iSpawnPointIndex;               //OpenStartPostion

	int                             DisplayAlpha;					//DisplayAlpha
	int                             RendAlpha;						//RendAlpha
	BOOL                            bRenderLast;					//RenderLatter

	int                           	dwDispAppSkill;					//dwDispAppSkill
	ModelAnimation::EMapTypeFlag    dwDispAppSkillMask;				//dwDispAppSkillMask

	BOOL                            bHideRightWeapon;				//Weapons hidden

	int                             iMotionLoopCnt;					//MotionLoop - Repeat Motion Count
	int                             iMotionLoopSpeed;				//MotionLoopSpeed - Frame rate for repeating motion
	int                             iMotionEvent;					//MotionEvent - Motion event flag

	int                             iEnchantWeaponType;				//WeaponEffect - Elemental Effect for Weapons
	DWORD                           dwEnchantWeaponTimeOut;			//dwWeaponEffectTime - Weapon Property Effect Counter
	int                             iEnchantWeaponLevel;			//EnchantEffect_Point - Skill EnchantWeapon's Skill Points

	UserData                        * pcHookedTarget;				//lpCompulsionPlayInfo
	DWORD                           dwHookedTargetEndTime;			//dwCompulsionTime

	DWORD                           dwAssassinEyeEndTime;
	short 							sAssassinEyeCritDmg;
	short 							sAssassinEyeCritChance;

	int                             iClanInfoIndex;
	DWORD                           dwClanUpdateTime;

	int                           	iBellatraCrown;
	BOOL                            bBlessCastleCrown;

	Map								* pcMap;			//lpExt1
	UserData						* pcKillerData;		//lpExt2
	void							* pcBellatraKiller;	//lpExt3

	union
	{
		BOOL						bWearingTwoHandedWeapon;
		void						* pcMapSpawn;				//lpExt4
	};
	Point3D                         sPetPosition;			//HoSkillPos
	int                             iPetID;					//HoSkillCode
	BOOL                            bAggressive;			//HoSkillMode
	DWORD							PartyFlag;				//PartyFlag (was dwExclusiveNum)
    DWORD							PartyParam;				//PartyParam (was iBellatraRoom)
	union
	{
		int                         iSlowTimeLeft;			//PlaySlowCount - Slow attack
		int                         iCursedTimeLeft;
	};
	int                             iPlaySlowSpeed;			//PlaySlowSpeed - Slowing down. 256 (SLOW_SPEED_BASE) is normal. 0 = not moving
	int                             iStunTimeLeft;			//PlayStunCount - Neutral status counter. Multiply stun seconds by 16.
	int                             iStopMotionCounter;		//PlayStopCount - Motion stop counter
	short							sHolyMind[2];			//PlayHolyMind[2] - Damage weakening function ([0] reduced damage% [1] retention time)
	short                           sSuccubus[2];			//PlayHolyPower[2] - Damage enhancement function ([0] enhanced damage% [1] retention time)
	short                           sPoisonDamage;			//PlayPoison[2]
	short							sPoisonCounter;
	short                           sHolyIncantation[2];

	int                             iVanishTimeLeft;
	int                             bBitterOrdeal;
	BOOL                            bNoShadow;
	int                             bDistortion;
	BOOL                            bNoDeath;

#if defined(_GAME)
	int                             iBlessHealLimit;
	int                             iRivera;
	int                             iWisp;
	int                             iBlindSkill;
	int                             iCurseLazy;
#endif

	BOOL							bCheckFlagIce;

	int                             iPoisonOverlay_DoNotUse;				//DispPoison

	DWORD                           dwForceOfNatureTimeOut;
#if defined(_GAME)
	DWORD                           dwHallOfValhallaTimeOut;


	DWORD                           uAdventMigalEndTime;
	DWORD                           dwAdventMidrandaTime;
#endif

	int                             iAreaDamageArea;				//AttackSkillRange;		//Monster uses skill range attack

#if defined(_GAME)
	int                             iHammerAttackCount;				//EventAttackCount - client side only it seems
	DWORD                           lHammerUserID;					//dwEventAttackParam - client side only it seems
#else
	Unit							*pcUnit;						//Pointer to Unit class for this UnitData
	DWORD							dwSpare;
#endif

	short                           sAddColorEffect[4];
	int                             iCurHPBar;

	DWORD                           dwUpdateCharInfoTime;			//Update attempt time
	DWORD                           dwLastRecvAttackTime;			//Last attack time

	int                             iTryAttackCount;				//Number of attack attempts

#if defined(_GAME)
	EItemID							eForceOrbItemID;				//dwForceOrbCode (i.e. ITEMID_BellumForce)
#else
	BOOL							bNoCountInSpawn;
#endif

	DWORD							dwForceOrbTimeOut;				//dwForceOrbTime (this is when it times out. it's TICKCOUNT + Time Remaining = Timeout.

#if defined(_GAME)
	DWORD							dwLifeBoosterCode;
	DWORD                           dwLifeBoosterTime;

	DWORD                           dwManaBoosterCode;
	DWORD                           dwManaBoosterTime;

	DWORD                           dwStaminaBoosterCode;
	DWORD                           dwStaminaBoosterTime;

	BOOL							bAddFrameUnit;

	int								iPadding[26];


#endif

	ECharacterType					PkMode_CharState;    //For PVP mode. In PVP - the CharacterData->iType will read 'Monster' (so you can target the player etc) with this being 'Player'
	int                             DontMoveFlag;	     //if 2 then angle = 0
	ID		                        iClanID;

	int                             iHPBlocks;
	CurMax                          sHPBlocks;
	CurMax                          sVirtualHP; //Unit * pcUnit

#if defined(_GAME)
	int								iUnknown70;
#endif

	short                           sBlessCastle_Damage; //sBlessCastle_Damage[0]
	short							sBlessCastle_PVPKills; //sBlessCastle_Damage[1]
	AttackDamageData                * psaDamageUsersData;	//lpAttackDamageList- User damage received
	AttackDamageData                * psaSiegeWarDataList;  //lpAttackDamageList_BlessCastle - For Blessed Castle

#if defined(_GAME)
	int								iPlaySeal;

	int								iPlayBlind;

	void							* pcaUnitDataTargetList;


	int								iPadding13[185];

#else
	UnitData					  * pcFocusingTarget;
	int								iFocusingLastID;

	int								iPadding13[177];

	BOOL							bTestMapUnit = FALSE;	//Cannot been seen by non-GM

#endif

	ECharacterGender				GetCharacterGender();
	EAnimationType					GetAnimation() { return psModelAnimation ? psModelAnimation->iType : ANIMATIONTYPE_None; }
	BOOL							IsDead() { return (GetAnimation() == ANIMATIONTYPE_Die); }
	BOOL							IsAttackAnimation() { return ((GetAnimation() == ANIMATIONTYPE_Attack) || (GetAnimation() == ANIMATIONTYPE_Skill)); }
	BOOL							IsLowAnimationType() { return (GetAnimation() < ANIMATIONTYPE_Attack); }

									//These are monsters that players actually attack. It excludes summons
	inline BOOL						IsNonPlayerMonster() { return this->sCharacterData.iType == ECharacterType::CHARACTERTYPE_Monster && this->PkMode_CharState != ECharacterType::CHARACTERTYPE_Player && this->sCharacterData.iMonsterType != EMonsterType::MONSTERTYPE_Summon; }

	inline BOOL						IsSummon() { return this->sCharacterData.iMonsterType == EMonsterType::MONSTERTYPE_Summon; }
	inline BOOL						IsNonPlayer() { return !IsPlayer(); }
	inline BOOL						IsPlayer() { return this->sCharacterData.iType == CHARACTERTYPE_Player || this->PkMode_CharState == CHARACTERTYPE_Player; }
	inline BOOL						IsNPC() { return this->sCharacterData.iType == CHARACTERTYPE_NPC; }


	int								GetHealthRemainingPercentage();
	int								GetMaxStamina();
	int								GetMaxMana();
	int								GetMaxHealth();
	void							CheckHealthStatusAfterDamageReceived( UserData * pcAttacker );
	void							CheckHealthStatusAfterDamageOverTimeReceived( UserData * pcDoTCaster );
	int								GetCurrentHealth();
	void							SetCurrentHealthToMax( );
	void							InitHealthPoint( int iValue );
	void							SetCurrentHealth( int iValue );
	void							TakeHealth( int iValue );
	int								ApplyDamageOverTime( int iValue, bool bAllowKill );
	void							GiveHealth( int iValue );

#ifndef _GAME
	//Flags this unit's status as changed so it replicates to clients immediately.
	//Member function only - does not affect UnitData's fixed layout.
	void							MarkStatusDirty();
#endif

	char							* GetName() { return this->sCharacterData.szName; };

	void							Main();
	void							Init();
	void							Free();

	DWORD							GetAttackTrans_XorCode();
	int								ResetAttackTrans();
	int								XorAttackTrans();
	int								GetDistanceDbl( int x1, int z1, int x2, int z2 );

	int								GetMonsterAccuracy( int iAttackRating );

	void							OverrideMoveAngle2 ( int iDistance );
	void							MoveAngle( int iDistance );
	BOOL							CheckShootingTest ( UnitData * pcUnitTarget );

	UnitData *						FindNearestMonster( UnitData * lpCurChar);

	void							SetPosition( int iX, int iY, int iZ, int iAX = 0, int iAY = 0, int iAZ = 0 );
	BOOL							Animate( EAnimationType eAnimationType );
	int								BeginSkill( int SkilCode , int Level , UnitData *lpTarChar , int x, int y, int z );
	int								BeginSkill( UnitData * lpTarChar );
	int								BeginSkill();

	int								ResetEnergyGraph( int Num );

	BOOL							ChangeAnimationID( int iID, int iModelType = 0 );

	int								MakeUnitBufferData( char * pBuffer, int iDelayTime, int iSleep );
	BOOL							NextTarget( int iX, int iY, int iZ );

	BOOL							SetTargetPosi( int iX, int iZ );
	BOOL							SetTargetEx( int iX, int iY, int iZ );

	int								SetAngleByPosition( int iX, int iZ );

	BOOL							GetAnimationAttackPosition( int * iX, int * iY, int * iZ );

	void							LoadModel( char * pszModelFilePath );

	void							PlayAttackObsolete( BOOL bUpdateTime = FALSE );
	BOOL							PlayAttackOverride( BOOL bUpdateTIme = FALSE );

	int								GetResistancePower( int power, int ResCode );

	BOOL							Render();


}; //Size = 0x4D98


class Unit
{
private:
	static void									Init( Unit * pcUnit, UnitData * pcUnitData );

public:

	UINT										uIndex;

	UnitData									* pcUnitData = NULL;
	int											iID;

	UINT										uLastUpdate;

	IMinMax										sHPLong;

	Point3D										sLastRunningPosition;
	DWORD										dwTimePositionUpdate;



	//Skills T5
	BOOL										bFierceWind;
	BOOL										bLethalSight;
	BOOL										bHolyConviction;
	BOOL										bWizardTrance;

	BYTE										bVirtualLifeLevel1Based;
	BYTE										bVirtualRegenLevel1Based;
	BYTE										bDemonsBaneLevel1Based;
	BYTE										bHallOfValhallaLevel1Based;
	BYTE										bTriumphOfValhallaLevel1Based;
	BYTE										bAlasLevel1Based;
	BYTE										bInpesLevel1Based;
	BYTE										bAdventMigalLevel1Based;
	BYTE										bHolyValorLevel1Based;
	BYTE										bHolyBodyLevel1Based;
	BYTE										bDrasticSpiritLevel1Based;
	BYTE										bElectricalOverloadLevel1Based;

	//Party
	bool										bPartyLeader;

	BYTE										baUnitBufferNew[0x2000];



#if defined(_GAME)
	std::mutex	*								cMutex;

	PacketAttackData							sLatestBasicAttackTrans;
	PacketAttackData							sLatestSkillAttackTrans;

	DWORD										dwLastSkillAttackReceived;
	DWORD										dwLastBasicAttackReceived;
	int											iLastSkillAttackEvent;
	int											iLastBasicAttackEvent;

	DWORD										dwLastReceivedAttackTrans = 0;
	DWORD										dwLastReceivedSkillAttackTrans = 0;
	DWORD										dwLastReceivedBasicAttackTrans = 0;


	int											iEvadeRating = 0;

	char										szTitle[25];
	short										sTitleRarity = 0;
	short										sPlayerAuraId = 0;
	short										sDialogSkinId = 0;

	BOOL										bInvisibleInMiniMap;
	short										sBellatraSoloCrown;

	std::vector<DamageInfo*>					vDamageInfoList;

#else
	BOOL										bHealItself = FALSE;
	BOOL										bReturningToSpawn = FALSE;
	BOOL										bHasTargetFocus = FALSE;
	DWORD										dwReturningToSpawnTimeout = 0;
	DWORD										dwLastFocusOrAttackTime = 0;
	int											iIdleWithFocusCounter = 0;
	DWORD										dwLastNonIdleTime = 0;
	Point3D										dwLastWalkOrRunPos;
	int											iStuckCounter = 0;
	Point3D										sReturningToSpawnPosition;
	BOOL										bTestMap = 0;

	//related to poison effect
	short										sPoisonDamagePerStack = 0;
	DWORD										dwPoisonNextTime = 0;
	BYTE										bPoisonStackCount = 0;
	BYTE										bPoisonCount = 0;
	ID											idPoisonCastedBy = 0;

	//related to burning effect
	short										sBurningDamagePerTick = 0;
	DWORD										dwBurningNextTime = 0;
	BYTE										bBurningCount = 0;
	ID											idBurningCastedBy = 0;

	BOOL										bCursed = FALSE;

#endif


	std::map<DWORD, DWORD>						mapLastDamageReceived;

	void										Load();
	void										UnLoad ();

	void										Load( UnitData * pcUnitData );

	Point3D										& GetPosition() { return pcUnitData->sPosition; }

	int											GetID() { return iID; }

	char										* GetName() { return pcUnitData->sCharacterData.szName; }

	int											GetCurrentHealth() { return pcUnitData->GetCurrentHealth(); };

	EMonsterEffectID							GetMonsterEffectID() { return pcUnitData->sCharacterData.iMonsterEffectID; }

	static UINT									GetArrayPosition( UnitData * pcUnitData );

	EMapID										GetMapID();
	int											GetMapLevel();
};

/// <summary>
/// STG_CHAR_INFO
/// </summary>
struct MonsterMapSpawnHandle
{
	CharacterData	sCharacterData;
	int				iPositionState;

	BOOL			bFixed;

	Point3D			sPosition;
};

struct PacketUnitStatusMove : Packet
{
	ID					  lID;
	Point3D				  sPosition;
	short				  sSpare;
	short				  sAngleY;
	short				  sMapID;
	short				  sAnimationType;
	int					  iFrame;
	DWORD				  dwHashCharacterName;
}; //Size = 0x28

struct PacketGameStatusBase : Packet
{
	int					  iStatusID;
	ID					  lID;
};

struct PacketGameStatusBabelStormMap : PacketGameStatusBase
{
	EMapID				  eBabelStormMapID;
};

struct PacketGameStatusBlessCastle : PacketGameStatusBase
{
	DWORD				  dwNum1;
	int					  iClanID1;
	DWORD				  dwNum3;
	DWORD				  dwNum4;
	short				  sConRate;
	short				  sBattlePoints;
	DWORD				  dwNum6;

	int					  iClanID2;
	int					  iClanID3;
};

struct PacketGameStatusPvPMap : PacketGameStatusBase
{
	EMapID				  ePvPMapID;
};

struct PacketUnitStatusContainer : Packet
{
	int					  iAmount;
	int					  iCounter;

	BYTE				  baBuffer[0x1FF0];
};
