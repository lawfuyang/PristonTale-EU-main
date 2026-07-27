#include "stdafx.h"
#include "DamageHandler.h"
#include "ini.h"
#include "HNSSkill.h"

extern int __cdecl RecvSkillSingleTarget ( UserData * pcUserData, UxxxTarget * p_Target, int * piAttackPower, PacketSingleTargetSkillDataEx * psPacketEx );
extern int RecvSkillAreaTarget( UserData * pcUserData, UnitData * pcUnitData, PacketSkillDamageRange * psPacketRange, PacketMultiTargetSkillData * psPacketRangeEx );

#define BURNING_TICKRATE_MS 500

#define FIELD_STATE_CASTLE		0xA00
#define FIELD_STATE_ACTION		0xB00

NAKED BOOL CDamageHandler::SkillRangeMonsterSet( UnitData * pcUnitData, UserData * pcTarget, int iRange, int iDamagePercentDecrease )
{
	JMP( pfnSkillRangeMonsterSet );
}

/// <summary>
/// Used for decoding packets
/// </summary>
/// <param name="iUserID">The user ID of the attacker</param>
/// <param name="psPacketAttackDamageEx">The attack packet received from client</param>
NAKED BOOL CDamageHandler::DecodePacket ( UserData * lpPlayInfo, PacketSingleTargetSkillData * psPacketAttackDamageEx )
{
	JMP ( pfnDecodePacket );
}

/// <summary>
/// Skill Attack Conversion
/// Leaked: rsSetSkillPlus( rsPLAYINFO *pcUserData , smCHAR *pcUnitData , TRANS_ATTACKDATA *psSkillDataPacket , TRANS_ATTACKDATA2 *lpTransAttackData2 )
///	ASM: 0x00577980
/// </summary>
/// <param name="pcUserData"></param>
/// <param name="pcUnitData"></param>
/// <param name="psSkillDataPacket"></param>
/// <param name="lpTransAttackData2"></param>
/// <returns></returns>
NAKED BOOL CDamageHandler::SetSkillPlus ( UserData * lpPlayInfo, UnitData * lpChar, PacketAttackData * lpTransAttackData, PacketSingleTargetSkillData * lpTransAttackData2 )
{
	JMP ( pfnSetSkillPlus );
}


BOOL CDamageHandler::SendDamageBySocket( UnitData * pcThis, UnitData * pcUnitData, SocketData * pcSocketData, int iAttackID, int iDamageAdd, int iResistance )
{
	return (BOOL)CALLT_WITH_ARG5( pfnSendDamageBySocket, (DWORD)pcThis, (DWORD)pcUnitData, (DWORD)pcSocketData, iAttackID, iDamageAdd, iResistance );
}

DWORD	dm_GetDamgeChkSum_S2V(PacketAttackData * lpTransAttackData)
{
    DWORD	dwChkSum;

    dwChkSum = lpTransAttackData->iHeader;
    dwChkSum += lpTransAttackData->sPosition.iX ^ lpTransAttackData->sPosition.iY ^ lpTransAttackData->sPosition.iZ;
    dwChkSum ^= *(DWORD *)&lpTransAttackData->iAttackState + lpTransAttackData->iRange;
    dwChkSum ^= lpTransAttackData->iDamage << 5;
    dwChkSum ^= lpTransAttackData->dwChkSum;
    dwChkSum ^= ((DWORD *)lpTransAttackData->sRating)[0];
    dwChkSum ^= 0x80516809;

    return dwChkSum;
}

/// <summary>
/// Get the checksum of the attack packet
/// Leak: DWORD	dm_GetDamgeChkSum( TRANS_ATTACKDATA2 *lpTransAttackData2 )
/// ASM: 0x4050A0
/// </summary>
/// <param name="psPacketAttackDamageEx">The attack packet received from client</param>
DWORD CDamageHandler::GetDamageChecksum (  PacketSingleTargetSkillData * psPacketAttackDamageEx )
{
	DWORD	dwChkSum;

	dwChkSum = psPacketAttackDamageEx->iHeader;
	dwChkSum += psPacketAttackDamageEx->sPosition.iX ^ psPacketAttackDamageEx->sPosition.iY ^ psPacketAttackDamageEx->sPosition.iZ;
	dwChkSum += psPacketAttackDamageEx->AttackCount ^ psPacketAttackDamageEx->dwTime;
	dwChkSum ^= psPacketAttackDamageEx->AttackPowerMin;
	dwChkSum ^= psPacketAttackDamageEx->AttackParameter1;
	dwChkSum ^= psPacketAttackDamageEx->sCriticalChance;
	dwChkSum ^= psPacketAttackDamageEx->MotionEventNum;
	dwChkSum += psPacketAttackDamageEx->iSkillCode;
	dwChkSum ^= (0x68098051 + GAME_VERSION);

	return dwChkSum;
}

CDamageHandler::CDamageHandler()
{
}

CDamageHandler::~CDamageHandler()
{
}


/**
This function is called whenever a user has succesfully applied damage to a unit
See int	rsRecordCharDamage( rsPLAYINFO *pcUserData , smCHAR *pcUnitData , int Damage)
Note - player damage information to the unit is stored in pcUnitData->psaDamageUsersData
Note - Only boss units have pcUnitData->psaDamageUsersData. See MapServer::SpawnMonsterBoss
*/
int __cdecl CustomRecordCharacterDamage(UserData * pcUserData, UnitData * pcUnitData, int Damage)
{
	if ( pcUnitData->psaDamageUsersData == NULL && pcUnitData->psaSiegeWarDataList == NULL )
		return 0;

	if (pcUnitData && pcUnitData->sCharacterData.sMonsterClass == EMonsterClass::Boss &&
		pcUnitData->sCharacterData.iMonsterEffectID != MONSTEREFFECTID_CASTLE_TOWER_B &&
		pcUnitData->sCharacterData.iMonsterEffectID != MONSTEREFFECTID_CASTLE_CRYSTAL_N &&
		pcUnitData->sCharacterData.iMonsterEffectID != MONSTEREFFECTID_CASTLE_CRYSTAL_R &&
		pcUnitData->sCharacterData.iMonsterEffectID != MONSTEREFFECTID_CASTLE_CRYSTAL_G &&
		pcUnitData->sCharacterData.iMonsterEffectID != MONSTEREFFECTID_CASTLE_CRYSTAL_R )
	{

		Unit* pcUnit = UNITDATATOUNIT(pcUnitData);

		//Record last hit timestapm for each player
		//This is to determine if a player has killed a boss within 10 seconds
		auto it = pcUnit->mapLastDamageReceived.find(pcUserData->iID);
		if (it != pcUnit->mapLastDamageReceived.end())
		{
			it->second = TICKCOUNT;
		}
		else
		{
			pcUnit->mapLastDamageReceived.insert({ pcUserData->iID, TICKCOUNT });
		}
	}

	//Is final castle tower
	if ( pcUnitData && pcUnitData->sCharacterData.iMonsterEffectID == MONSTEREFFECTID_CASTLE_TOWER_B )
	{
		//Door is still alive and user is attacking the tower??
		//Set Damage to 0
		if ( BLESSCASTLEHANDLER->GetCastleDoorStatus () )
		{
			INFO ( "User %s attacked tower but door is still up", CHARACTERSERVER->GetCharacterName ( pcUserData ) );
			Damage = 0;

			//Also force crystal HP to max
			pcUnitData->SetCurrentHealthToMax();
		}

		//Crystal towers is still alive and user is attacking the tower??
		//Set Damage to 0
		else if ( BLESSCASTLEHANDLER->GetCrystalTowerStatus () )
		{
			INFO ( "User %s attacked tower but crystals are still up", CHARACTERSERVER->GetCharacterName ( pcUserData ) );
			Damage = 0;

			//Also force crystal HP to max
			pcUnitData->SetCurrentHealthToMax();
		}
	}
	else if ( pcUnitData &&
		      (pcUnitData->sCharacterData.iMonsterEffectID == MONSTEREFFECTID_CASTLE_CRYSTAL_N ||
		       pcUnitData->sCharacterData.iMonsterEffectID == MONSTEREFFECTID_CASTLE_CRYSTAL_R ||
		       pcUnitData->sCharacterData.iMonsterEffectID == MONSTEREFFECTID_CASTLE_CRYSTAL_G ||
		       pcUnitData->sCharacterData.iMonsterEffectID == MONSTEREFFECTID_CASTLE_CRYSTAL_B) )
	{
		//Door is still alive and user is attacking the tower??
		//Set Damage to 0
		if ( BLESSCASTLEHANDLER->GetCastleDoorStatus () )
		{
			INFO ( "User %s attacked a crystal but door is still up. Setting score to 0", CHARACTERSERVER->GetCharacterName ( pcUserData ) );
			Damage = 0;

			//Also force crystal HP to max
			pcUnitData->SetCurrentHealthToMax();
		}
	}

	//typedef signed int(__cdecl* fnRecordCharDamage)(UserData* pcUser, UnitData* pcUnitData, int Damage);
	//fnRecordCharDamage rsRecordCharDamage = (fnRecordCharDamage)0x00579A00;

	return DAMAGEHANDLER->RecordCharDamage (pcUserData, pcUnitData, Damage);
}


int	CDamageHandler::RecordCharDamage ( UserData * lpPlayInfo, UnitData * lpChar, int Damage )
{
	int cnt;
	int EmptyNum = -1;
	DWORD	dwClanCode;

	if ( lpChar->psaSiegeWarDataList &&
		BLESSCASTLEHANDLER->GetBlessCastleStatus() == TRUE && //BC is active
		BLESSCASTLEHANDLER->GetCastleDoorStatus() == FALSE)   //Door is down
	{
		//Bless Castle Crystal Tower (Clan Rank)
		if ( lpPlayInfo->iClanID && lpPlayInfo->iMapID == MAPID_BlessCastle )
		{
			lpPlayInfo->sBlessCastle_Damage += Damage;	// Add damage information

			for ( cnt = 0; cnt < MAX_DAMAGEDATA; cnt++ )
			{
				if ( lpChar->psaSiegeWarDataList[cnt].iID )
				{
					dwClanCode = CHARACTERSERVER->GetClanCode ( lpChar->psaSiegeWarDataList[cnt].iID );
					if ( dwClanCode == lpPlayInfo->iClanID )
					{
						lpChar->psaSiegeWarDataList[cnt].iDamage += Damage;
						lpChar->psaSiegeWarDataList[cnt].iCounter++;
						return TRUE;
					}
				}

				if ( EmptyNum < 0 && lpChar->psaSiegeWarDataList[cnt].iID == 0 )
				{
					EmptyNum = cnt;
				}
			}

			if ( EmptyNum >= 0 )
			{
				lpChar->psaSiegeWarDataList[EmptyNum].iID = lpPlayInfo->sCharacterData.iClanID;
				lpChar->psaSiegeWarDataList[EmptyNum].iDamage = Damage;
				lpChar->psaSiegeWarDataList[EmptyNum].iCounter++;

				return TRUE;
			}
		}

		return TRUE;
	}

	if ( lpChar->psaDamageUsersData == NULL )
		return FALSE;

	//General ranking (individual)
	for ( cnt = 0; cnt < MAX_DAMAGEDATA; cnt++ )
	{
		if ( lpChar->psaDamageUsersData[cnt].iID == lpPlayInfo->iID )
		{
			 lpChar->psaDamageUsersData[cnt].iDamage += Damage;
			 lpChar->psaDamageUsersData[cnt].iCounter++;
			 return TRUE;
		}

		if ( EmptyNum < 0 && lpChar->psaDamageUsersData[cnt].iID == 0 )
		{
			EmptyNum = cnt;
		}
	}

	if ( EmptyNum >= 0 )
	{
		lpChar->psaDamageUsersData[EmptyNum].iID = lpPlayInfo->iID;
		lpChar->psaDamageUsersData[EmptyNum].iDamage = Damage;
		lpChar->psaDamageUsersData[EmptyNum].iCounter++;

		return TRUE;
	}

	return FALSE;
}


/// <summary>
/// Svr_Damage.cpp
/// Leaked: rsRecvAttackData_Old ( rsPLAYINFO * pcUserData, TRANS_ATTACKDATA * psSkillDataPacket, smCHAR * lpTargetChar, int Type )
/// ASM : 0057CB90
/// Single Target Attack received
/// </summary>
NAKED BOOL CDamageHandler::SingleDamageRangeHandler( UserData * pcUserData, PacketAttackData * psSkillNormal, UnitData * psUnitData, int iType )
{
	//but it's all in ASM
	//and theres a number of skills related bug inside this ASM
	JMP( pfnSingleDamageRangeHandler );
}

/// <summary>
/// Override the ASM function rsRecvAttackData_Old (directly above)
/// ASM : 0x0057CB90
/// Packet header: smTRANSCODE_ATTACKDATA
/// This code pretty much handles all single-target skill attacks
/// </summary>
/// <param name="pcUser">The player who initated the attack</param>
/// <param name="lpTransAttackData">Some extra info for skill</param>
/// <param name="psSkillDataPacket">The attack data received from the player</param>
/// <param name="lpTargetChar">The target (can be a monster or a player)</param>
/// <param name="iType"></param>
/// <param name="eSkillID">The skill ID</param>
BOOL CDamageHandler::SingleTargetSkillDamageHandler ( User * pcUser, PacketAttackDataEx * lpTransAttackData, PacketSingleTargetSkillDataEx * psSkillDataPacket, UxxxTarget * p_Target, int Type, ESkillID eSkillID )
{
	UserData * pcUserData = pcUser->pcUserData;

	BOOL bDebugFunc = TRUE;

	DEBUGIF ( bDebugFunc, "" );
	DEBUGIF ( bDebugFunc, "" );
	DEBUGIF ( bDebugFunc, "SingleTargetSkillDamageHandler" );
	DEBUGIF ( bDebugFunc, "iType = %d", Type );
	DEBUGIF ( bDebugFunc, "psSkillDataPacket->iRange >> 8 = %d ", lpTransAttackData->iRange >> 8 );
	DEBUGIF ( bDebugFunc, "psSkillDataPacket->iDamage = %d", lpTransAttackData->iDamage );
	DEBUGIF ( bDebugFunc, "psSkillDataPacket->sLevel = %d", lpTransAttackData->sRating[0] );
	DEBUGIF ( bDebugFunc, "psSkillDataPacket->sAccuracy = %d", lpTransAttackData->sRating[1] );


	DWORD	dwTime;
	PacketChatMessage	TransChatMessage;

	PacketTransCommand		smTransCommand;
	int		x, y, z;
	int		cnt;
	EAttackProperty		regs;
	int     temp;
	int		totalHpStolen = 0;
	int		skillFlinchChance, skillFlinchPushbackRange;
	SocketData * lpsmSock;
	BOOL	bCrit;


	ZeroMemory( &TransChatMessage, sizeof( PacketChatMessage ) );
	ZeroMemory( &smTransCommand, sizeof( PacketTransCommand ) );

	skillFlinchChance = 0;
	skillFlinchPushbackRange = 0;

	int	Dmg1, Dmg2, Dmg3, Dmg4;
	Dmg1 = 0; Dmg2 = 0; Dmg3 = 0; Dmg4 = 0;

	lpsmSock = pcUserData->pcSocketData;
	dwTime = TICKCOUNT;
	bCrit = FALSE;


	int	skillCode, skillLevel1Based, skillParam;

	skillCode			= ( psSkillDataPacket->iSkillCode & 0xFF);
	skillParam			= ( psSkillDataPacket->iSkillCode >> 12 ) & 0xF;
	skillLevel1Based	= ( psSkillDataPacket->iSkillCode >> 8 )  & 0xF;

	//Record attack data
	pcUserData->Recv_AttackCount++;
	pcUserData->Recv_AttackDamage += lpTransAttackData->iDamage;

	//Player has cloak. skip attack
	if ( pcUserData->bHideMode && pcUserData->iGameLevel < 4 )
	{
		DEBUGIF ( bDebugFunc, "Player has cloak. Skipping attack" );
		return FALSE;
	}

	if (!p_Target->IsValid() || p_Target->GetCharacterType() != ECharacterType::CHARACTERTYPE_NPC )
		pcUserData->AttackCount++;

	//Attack timing calculation
	if ( ( TICKCOUNT - pcUserData->dwAttackTime ) > 5000 )
	{
		pcUserData->dwAttackTime = TICKCOUNT;
		if ( pcUserData->AttackCount >= 40 )
		{
			PacketLogCheat sPacket;
			sPacket.iCheatID = 1823;
			sPacket.LParam = pcUserData->AttackCount;
			LOGSERVER->OnLogCheat ( pcUserData, &sPacket, TRUE );

			pcUserData->AttackCount = 0;

			return FALSE;
		}

		pcUserData->AttackCount = 0;
	}

	temp = (DWORD)( lpTransAttackData->iDamage * 2002 + ( lpTransAttackData->sPosition.iX * lpTransAttackData->sPosition.iY * lpTransAttackData->sPosition.iZ ) );

	if ( !Type && lpTransAttackData->dwChkSum != temp )
	{
		PacketLogCheat sPacket;
		sPacket.iCheatID = 1820;
		smTransCommand.LParam = lpTransAttackData->iDamage;
		smTransCommand.EParam = lpTransAttackData->dwChkSum;
		smTransCommand.WParam = temp;
		LOGSERVER->OnLogCheat ( pcUserData, &sPacket, TRUE );

		return FALSE;
	}

	if ( abs ( ( (long)( TICKCOUNT - pcUserData->dwTimeLastPacket ) ) ) > 3 * 1000 )
	{
		DEBUGIF ( bDebugFunc, "No packet from player for at least 3s" );
		return FALSE;
	}

	//For hardcore events (disabled when attacking event monsters outside the line)
	if (p_Target->IsTargetUnit() && p_Target->unitData->PartyFlag == 0x100 )
	{
		if ( pcUserData->dwExclusiveNum != 0x100 )
		{
			WARN ( "Monster is from Belletra but player is not in Bellatra?" );
			return FALSE;
		}
	}

	if (p_Target->IsValid())
	{

		if (eSkillID == SKILLID_DefaultAttack  )
		{
			skillFlinchChance = 100;
			skillFlinchPushbackRange = 10;
		}
		else
		{
			auto skillInfo = GetSkillInfo( eSkillID );
			if ( skillInfo )
			{
				skillFlinchChance = skillInfo->iFlinchChance;
				skillFlinchPushbackRange = skillInfo->iPushbackRange;
			}
		}


		if (p_Target->IsTargetUnit())
			p_Target->unitData->dwLastActiveTime = TICKCOUNT;		//Last active time

		x = ( pcUserData->sPosition.iX - p_Target->GetPosition().iX ) >> FLOATNS;
		y = ( pcUserData->sPosition.iY - p_Target->GetPosition().iY ) >> FLOATNS;
		z = ( pcUserData->sPosition.iZ - p_Target->GetPosition().iZ ) >> FLOATNS;
		if ( ( x * x + z * z ) > DISTANCE_XY_20_meters || abs ( y ) > 100 )
		{
			WARN ( "Attack failed. Dist to mon too different. xz = %d, y = %d", ( x * x + z * z ), abs ( y ) );
			return FALSE;
		}

		x = ( lpTransAttackData->sPosition.iX - p_Target->GetPosition().iX ) >> FLOATNS;
		y = ( lpTransAttackData->sPosition.iY - p_Target->GetPosition().iY ) >> FLOATNS;
		z = ( lpTransAttackData->sPosition.iZ - p_Target->GetPosition().iZ ) >> FLOATNS;
		if ( ( x * x + z * z ) > DISTANCE_XY_20_meters || abs ( y ) > 100 )
		{
			WARN ( "Attack failed. Dist to mon too different. xz = %d, y = %d", ( x * x + z * z ), abs ( y ) );
			return FALSE;
		}

		if (p_Target->GetUnitAnimation() != EAnimationType::ANIMATIONTYPE_Die &&
			p_Target->GetUnitAnimation() != EAnimationType::ANIMATIONTYPE_Special )
		{
			Dmg1 = lpTransAttackData->iDamage;


			//Special case for Divine shield
			if ( psSkillDataPacket->eAttackState == EAttackState::State_9_DivineShield && p_Target->IsTargetUnit() )
			{
				CustomRecordCharacterDamage ( pcUser->pcUserData, p_Target->unitData, Dmg1 );

				p_Target->unitData->TakeHealth( Dmg1 );
				p_Target->unitData->CheckHealthStatusAfterDamageReceived( pcUser->pcUserData );

				//Give stolen hp if any
				GrantStolenHealthToUser( pcUser, Dmg1 );

				MAPSERVER->SendUnitDamageInfoTextToNearbyPlayers( pcUser->pcUserData, p_Target->unitData, Dmg1, FALSE );

				return true;
			}


			//Is a monster or a player (pvp)
			if (p_Target->GetCharacterType() == ECharacterType::CHARACTERTYPE_Monster ||
				p_Target->GetCharacterType() == ECharacterType::CHARACTERTYPE_Player)
			{
				//Last target monster record
				if (p_Target->IsTargetUnit())
				{
					pcUserData->pcLastAttack = p_Target->unitData;
                    pcUserData->lLastAttackID = p_Target->GetID();

					if ( psSkillDataPacket->eAttackState != EAttackState::State_2_Stun )
					{
						p_Target->unitData->iStunTimeLeft = 0;			//Disable neutral mode
					}
				}


				//In case of attribute attack
				regs = lpTransAttackData->iAttackProperty;

				cnt = lpTransAttackData->iDamage;

				//Apply damage buff based on monster type and skill
				ApplyMonsterTypeDamageBuffFromSkill( &cnt, p_Target, pcUser, skillLevel1Based, eSkillID );

				bCrit = ApplyCriticalDamage( eSkillID, &cnt, psSkillDataPacket->sCriticalChance, psSkillDataPacket->sCriticalDmgBoost, pcUserData, p_Target, (EAttackState)lpTransAttackData->iAttackState, psSkillDataPacket->iAttackProperty, FALSE );



				if ( bCrit && p_Target->IsTargetUnit() )
				{
					//this sends a packet to the client to set AttackCritcal to 1. This in turn ages the item via critical. Some items only a get aged via critical!
					//See int sinCheckAgingLevel(int Kind, bool bBillingUse)
					SENDPACKETCOMMAND( pcUser, PKTHDR_AttackResult, 0, 1 );
				}

				Dmg2 = cnt;

				AdjustDamageFromAbsorb( &cnt, p_Target);

				Dmg3 = cnt;

				//Check and apply extinction
				cnt += ApplyExtinction( psSkillDataPacket->eAttackState, &psSkillDataPacket->sElementalAttackSetting, p_Target, eSkillID, skillCode );

				//Check and apply life steal
				const int hpStolen = ApplyLifeSteal ( psSkillDataPacket->eAttackState, psSkillDataPacket->sLifeStealParam, p_Target, eSkillID, skillCode);

				if ( hpStolen > 0 )
				{
					totalHpStolen += hpStolen;
					cnt += hpStolen;
				}

				//Check and apply burning
				if ( psSkillDataPacket->iAttackProperty == EAttackProperty::Fire )
				{
					ApplyBurning( pcUser, &psSkillDataPacket->sElementalAttackSetting, p_Target, eSkillID, skillCode);
				}
				if ( psSkillDataPacket->eSecondaryAttackProperty == EAttackProperty::Fire )
				{
					ApplyBurning( pcUser, &psSkillDataPacket->sSecondaryElementalAttackSetting, p_Target, eSkillID,skillCode);
				}

				//Check and apply poison
				if ( psSkillDataPacket->iAttackProperty == EAttackProperty::Poison )
				{
					ApplyPoison( pcUser, &psSkillDataPacket->sElementalAttackSetting, p_Target, eSkillID, skillCode);
				}
				if ( psSkillDataPacket->eSecondaryAttackProperty == EAttackProperty::Poison )
				{
					ApplyPoison( pcUser, &psSkillDataPacket->sSecondaryElementalAttackSetting, p_Target, eSkillID, skillCode);
				}


				//Check and apply freezing fom ice skills
				if ( psSkillDataPacket->iAttackProperty == EAttackProperty::Ice )
				{
					ApplyFreezing( pcUser, &psSkillDataPacket->sElementalAttackSetting, p_Target, eSkillID, skillCode);
				}
				if ( psSkillDataPacket->eSecondaryAttackProperty == EAttackProperty::Ice )
				{
					ApplyFreezing( pcUser, &psSkillDataPacket->sSecondaryElementalAttackSetting, p_Target, eSkillID, skillCode);
				}

				//Check and apply curse
				if ( psSkillDataPacket->eAttackState == EAttackState::State_8_Cursed)
				{
					ApplyCurse( pcUser, &psSkillDataPacket->sElementalAttackSetting, p_Target, eSkillID, skillCode);
				}
				if ( psSkillDataPacket->eSecondaryAttackState == EAttackState::State_8_Cursed )
				{
					ApplyCurse( pcUser, &psSkillDataPacket->sSecondaryElementalAttackSetting, p_Target, eSkillID, skillCode);
				}

				//check and apply distortion
				if ( psSkillDataPacket->eAttackState == EAttackState::State_7_Distortion )
				{
					ApplyDistortion( pcUser, &psSkillDataPacket->sElementalAttackSetting, p_Target, eSkillID, skillCode);
				}
				if ( psSkillDataPacket->eSecondaryAttackState == EAttackState::State_7_Distortion )
				{
					ApplyDistortion( pcUser, &psSkillDataPacket->sSecondaryElementalAttackSetting, p_Target, eSkillID, skillCode);
				}

				//Check and apply lightning stun
				ApplyLightningStun( pcUser, psSkillDataPacket->iAttackProperty, &psSkillDataPacket->sElementalAttackSetting, p_Target, eSkillID, skillCode);

				//Check and apply javelin Frost
				ApplyFrostJavelin( pcUser, psSkillDataPacket->iAttackProperty, p_Target, eSkillID, skillCode);

				//Check and apply weapon enchantment - fire and ice only
				ApplyEnchantWeapon( pcUser, psSkillDataPacket->iAttackProperty, p_Target, FALSE, eSkillID, skillCode);

				//Adjust elemental damage based on monster resistence
				AdjustDamageFromMonsterResistence( psSkillDataPacket->iAttackProperty, &cnt, p_Target);


				//Check player damage against monster
				/*if ( p_Target->IsTargetUnit() &&
				     ( cnt > p_Target->unitData->GetMaxHealth() / 2 ) &&
					 ((pcUserData->sCharacterData.iLevel) < (p_Target->GetCharacterData()->iLevel + 20)) )
				{
					PacketLogCheat sPacket;
					sPacket.iCheatID = CHEATLOGID_HighDmgRecordedAgainstHigherLevelMon;
					sPacket.LParam = cnt;
					sPacket.SParam = p_Target->unitData->GetMaxHealth() / 2;
					LOGSERVER->OnLogCheat( pcUserData, &sPacket, TRUE, FALSE );

					return FALSE;
				}*/

				if ( cnt > 0 && p_Target->IsTargetUnit() )
				{
					//Record damage
					Dmg4 = cnt;

					ApplyPriorityAttacker(p_Target, pcUserData, cnt, eSkillID, skillCode);

					p_Target->unitData->TakeHealth( cnt );
					p_Target->unitData->CheckHealthStatusAfterDamageReceived( pcUserData );

					//remove stun
					if ( psSkillDataPacket->eAttackState != EAttackState::State_2_Stun )
					{
						p_Target->unitData->iStunTimeLeft = 0;			//Disable neutral mode
					}

					//Records damage info in monster data / castle towers for rewards etc
					CustomRecordCharacterDamage ( pcUserData, p_Target->unitData, cnt );

					ApplySkillFlinchAndPushback( pcUserData, bCrit, skillFlinchChance, skillFlinchPushbackRange, p_Target, &lpTransAttackData->sPosition, (EAttackState)lpTransAttackData->iAttackState, eSkillID, skillCode);

					//Give stolen hp if any
					GrantStolenHealthToUser( pcUser, totalHpStolen );

					//give hp via beserker
					ConvertDamageToHPFromBeserker( pcUser, cnt, FALSE );

					//heal hp via holy body buff
					ApplyPartyHealFromHolyBody( pcUser, cnt, FALSE );


					WRITEDWORD ( ESkillsVarsASM::ATTACK_DAMAGE, Dmg4 );

					MAPSERVER->SendUnitDamageInfoTextToNearbyPlayers( pcUserData, p_Target->unitData, Dmg4, bCrit );
				}


				//pvp mode
				else if (p_Target->IsTargetPlayer())
				{
					//Give hp based on damage for beserker. halved for AoE
					ConvertDamageToHPFromBeserker( pcUser, static_cast<int>( static_cast<float>( cnt ) / fGlobalDamageReduction ), TRUE );

					//Give stolen hp if any
					GrantStolenHealthToUser( pcUser, totalHpStolen );

					cnt = Dmg2; //use the damage value BEFORE player absorb is applied

					//reduce damage via resistence
					AdjustDamageFromMonsterResistence( psSkillDataPacket->iAttackProperty, &cnt, p_Target);

					SendDamagePvPData( pcUserData, p_Target->userData, cnt, psSkillDataPacket->iAttackRating, EAttackState::State_128_Skill, eSkillID, bCrit );
				}

				pcUserData->dwLastAttackTime = dwTime;

				DEBUGIF ( bDebugFunc, "%s Damaged (%d %d %d %d )", p_Target->GetName(), Dmg1, Dmg2, Dmg3, Dmg4 );
			}

			//Normal NPC
			else if (p_Target->IsTargetUnit() && p_Target->GetCharacterType() == ECharacterType::CHARACTERTYPE_NPC)
			{
				DEBUG( "Clicked on a NPC" );
				auto l_NPCUnitData = p_Target->unitData;
				// Wait for more than 1.5 seconds before opening
				if ( abs ( (long)pcUserData->dwLastAttackTime - (long)dwTime ) > 1500 )
				{
					pcUserData->dwLastAttackTime = dwTime;

					if ( l_NPCUnitData->sUnitInfo.QuestCode &&
						 l_NPCUnitData->sUnitInfo.QuestCode == lpTransAttackData->iQuestData >> 16 )
					{
						//Quest progress NPC
						//See int rsProcessQuest(rsPLAYINFO *pcUserData, smCHAR *pcUnitData, int QuestCode, int QuestParam) in leaked code
						if (CALL_WITH_ARG4 ( (DWORD)ENPCFunction::SEND_QUEST_PROGRESSION, (DWORD)pcUserData, (DWORD)l_NPCUnitData, lpTransAttackData->iQuestData >> 16, (DWORD)lpTransAttackData->iQuestData & 0xFFFF ) == TRUE )
							return TRUE;
					}

					UNITSERVER->OnUnitNpcClick ( pcUser, l_NPCUnitData);
					WARN( "Bad state - NPC Click is handled within damage func" );
				}
			}
		}
	}
	/* todo hardcpp/iron
	//PVP
	else
	{
		//Currently set to PK disabled mode
		//Find a character (you need to code and change it so that you can quickly find it later)
		lpPlayInfo2 = USERSERVER->GetUserdata( lpTransAttackData->dwTarObjectSerial );
		if ( lpPlayInfo2 )
		{
			if ( *(BOOL*)( ESeverConfigASM::ENABLE_PK ) || pcUserData->sCharacterData.iType != lpPlayInfo2->sCharacterData.iType )
			{
				//Only allow PvP if players are over lvl 10
				if ( pcUserData->iGameLevel == 4 || ( pcUserData->sCharacterData.iLevel > 10 && lpPlayInfo2->sCharacterData.iLevel > 10 ) )
				{
					//Sending another user attack packet (user attack)
					DAMAGEHANDLER->SendDamagePvPData ( pcUserData, lpPlayInfo2, lpTransAttackData->iDamage, EAttackState::State_128_Skill);
				}
			}
		}
	}
	*/
	return TRUE;
}

/// <summary>
/// Leaked: int RecvRangeAttack_User( rsPLAYINFO *pcUserData , TRANS_SKIL_ATTACKDATA *lpTransSkilAttackData , int AreaNum , int PlaySkillCode )
/// ASM address: 0x0057AFF0
/// int RecvRangeAttack( rsPLAYINFO *pcUserData , TRANS_SKIL_ATTACKDATA *lpTransSkilAttackData , int AreaNum , int PlaySkillCode )
/// Used for handling ranged attack against other monsters
/// </summary>
NAKED BOOL CDamageHandler::DamageRangeHandler( UserData * pcUserData, PacketSkillDamageRange * psRangeData, int iMapID, int iSkillID )
{
	JMP( pfnDamageRangeHandler );
}

void CDamageHandler::ConvertDamageToHPFromBeserker( User * pcUser, const int iDamage, BOOL bHalf )
{
	if ( pcUser->sBersekerDamageToHPPercent )
	{
		int hpRestore = ( iDamage * pcUser->sBersekerDamageToHPPercent ) / 100;

		if ( bHalf )
			hpRestore /= 2;

		SENDPACKETCOMMAND( pcUser, PKTHDR_RegenChar, 0, hpRestore );
		MAPSERVER->SendHealedAmountToNearbyPlayers( pcUser, pcUser, hpRestore );
		CHATSERVER->SendDebugChat( pcUser, CHATCOLOR_Error, "Beserker: Total HP restored: %d", hpRestore );
	}
}

void CDamageHandler::ApplyPartyHealFromHolyBody( User * pcUser, const int iDamage, BOOL bHalf )
{
	if ( pcUser->bHolyBodyLevel1Based )
	{
		const int percent = GetSkillIntValueAt( pcUser, ESkillArrayPointer::Knight_HolyBody_DmgToHealConversion_New, pcUser->bHolyBodyLevel1Based );
		const int distance = GetSkillIntValueAt( pcUser, ESkillArrayPointer::Knight_HolyBody_HealRadius_New, pcUser->bHolyBodyLevel1Based );

		int hpRestore = ( iDamage * percent ) / 100;

		if ( bHalf )
			hpRestore /= 2;

		for ( User * pPartyUser : PARTYHANDLER->GetPartyMembersIncludingSelf( pcUser, TRUE, distance * distance ) ) //this will include self too
		{
			int value = hpRestore;

			//half for others
			if ( pcUser != pPartyUser )
				value /= 2;

			if ( value <= 0 )
				continue;

			SENDPACKETCOMMAND( pPartyUser, PKTHDR_RegenChar, 0, value);
			MAPSERVER->SendHealedAmountToNearbyPlayers( pcUser, pPartyUser, value);

			CHATSERVER->SendDebugChat( pcUser, CHATCOLOR_Error, "Holy Body: %d HP restored for %s", value, pPartyUser->GetCharacterName() );
		}
	}
}



void CDamageHandler::GrantStolenHealthToUser( User * pcUser, const int iHpStolen)
{
	if ( iHpStolen > 0 )
	{
		SENDPACKETCOMMAND( pcUser, PKTHDR_RegenChar, 0, iHpStolen );
		MAPSERVER->SendHealedAmountToNearbyPlayers( pcUser, pcUser, iHpStolen );

		CHATSERVER->SendDebugChat( pcUser, CHATCOLOR_Error, "Total HP restored: %d", iHpStolen );
	}
}

void CDamageHandler::ApplyPullBackAndTargetHook ( ESkillID eSkillID, int iHookDurationSec, UxxxTarget * p_Target, UserData * pcUserData, Point3D * pcSkillPos )
{
	if (p_Target->IsTargetUnit() && !TESTMAPHANDLER->UnitCanMove(p_Target->unitData))
		return;

	if ((p_Target->IsTargetUnit() && p_Target->GetMonsterClass() == EMonsterClass::Boss) || iHookDurationSec <= 0 )
		return;

	switch ( eSkillID )
	{
		case ESkillID::SKILLID_Compulsion:
		{
			if (!p_Target->IsTargetUnit())
				break;

            /// todo iron
            auto pcUnitData = p_Target->unitData;
			pcUnitData->pcFocusTarget = NULL;
			pcUnitData->pcAttackTarget = NULL;

			pcUnitData->pcHookedTarget = pcUserData;
			pcUnitData->dwHookedTargetEndTime = TICKCOUNT + (iHookDurationSec * 100);

			//adjust pushback range based on position between player and unit
			int distXY = p_Target->GetPosition().GetPTDistanceXZ( pcSkillPos );
			distXY = (int)sqrt( distXY );

			distXY -= 20;

			if ( distXY > 0 )
			{
				int ang2 = MAPSERVER->GetAngle(p_Target->GetPosition().iX, p_Target->GetPosition().iZ, pcSkillPos->iX, pcSkillPos->iZ );
				int ang = ( ang2 + PTANGLE_180 ) & PTANGLE_Mask;
				pcUnitData->sAngle.iY = ang;
				pcUnitData->MoveAngle( -distXY + 40); //pull towards player
				pcUnitData->sAngle.iY = ang2;
			}

			break;
		}

		case ESkillID::SKILLID_ChasingHunt:
		{
			if (!p_Target->IsTargetUnit())
				break;

			/// todo iron
			auto pcUnitData = p_Target->unitData;
			pcUnitData->pcFocusTarget = NULL;
			pcUnitData->pcAttackTarget = NULL;

			pcUnitData->pcHookedTarget = pcUserData;
			pcUnitData->dwHookedTargetEndTime = TICKCOUNT + (iHookDurationSec * 100);

			//adjust pushback range based on position between player and unit
			int distXY = p_Target->GetPosition().GetPTDistanceXZ(pcSkillPos);
			distXY = (int)sqrt(distXY);

			distXY -= 20;

			if (distXY > 0)
			{
				int ang2 = MAPSERVER->GetAngle(p_Target->GetPosition().iX, p_Target->GetPosition().iZ, pcSkillPos->iX, pcSkillPos->iZ);
				int ang = (ang2 + PTANGLE_180) & PTANGLE_Mask;
				pcUnitData->sAngle.iY = ang;
				pcUnitData->MoveAngle(-distXY + 40); //pull towards player
				pcUnitData->sAngle.iY = ang2;
			}

			break;
		}
	}
}

/// <summary>
/// For flinch simulation use this sheet to play around with the values
/// https://docs.google.com/spreadsheets/d/1jDDHq79p4eyqC6RrxTRPYvjeyEKI_byOyOcDhcag9yg/edit#gid=0
/// to understand more about how it works.
/// SYSTEM 1 is more consistent with how the 2012 code works.
/// </summary>
BOOL CDamageHandler::ApplyFlinch( UserData * pcAttacker, UxxxTarget * p_Target, int iFlinchChance, ESkillID p_SkillID, int p_SkillCode, BOOL bForceFlinch )
{
	if ( p_Target->IsTargetUnit() && !TESTMAPHANDLER->UnitCanMove( p_Target->unitData ) )
		return FALSE;

	//This skill has no flinch (disabled)
	if ( iFlinchChance <= 0 )
		return FALSE;

	//this unit cannot be flinched (0 = immune)
	if (p_Target->IsTargetUnit() &&
		p_Target->unitData->sUnitInfo.iFlinchChance <= 0)
		return FALSE;

	//don't flinch for boss monsters
	if ( p_Target->GetMonsterClass() == EMonsterClass::Boss )
		return TRUE;

	//don't apply flinch if the unit is doing these animations
	if ( p_Target->GetUnitAnimation() == EAnimationType::ANIMATIONTYPE_Flinch ||			//already flinching
		 p_Target->GetUnitAnimation() == EAnimationType::ANIMATIONTYPE_Potion ||			//drinking potion?
		 p_Target->GetUnitAnimation() == EAnimationType::ANIMATIONTYPE_Skill ||				//doing skill
		 p_Target->GetUnitAnimation() == EAnimationType::ANIMATIONTYPE_Special )			//doing special
	{
		return FALSE;
	}

	//in pvp, don't apply flinch
	//if the player is doing skill or attacking
	if ( p_Target->IsTargetPlayer() &&
		 p_Target->GetUnitAnimation() == EAnimationType::ANIMATIONTYPE_Attack )
	{
		return FALSE;
	}

	const bool casterFlinchApplied = iFlinchChance > Dice::RandomI( 0, 99 );
	const bool targetFlinchApplied = p_Target->IsTargetUnit() ? p_Target->unitData->sUnitInfo.iFlinchChance > Dice::RandomI( 0, 99 ) : 50;

	if (casterFlinchApplied && targetFlinchApplied || bForceFlinch)
	{
		if ( p_Target->IsTargetUnit() )
		{
			p_Target->unitData->Animate( EAnimationType::ANIMATIONTYPE_Flinch );
			MAPSERVER->SendUnitDamageInfoTextToNearbyPlayers( pcAttacker, p_Target->unitData, 0, FALSE, 0, EDamageTextType::Flinch);
		}

		return TRUE;
	}

	return FALSE;
}

BOOL CDamageHandler::ApplyPushback(UxxxTarget * p_Target, Point3D * pcRefPos, int iPushbackRange, ESkillID p_SkillID, int p_SkillCode)
{
	if ( iPushbackRange == 0 || p_Target->IsTargetPlayer() )
		return FALSE;

	if ( p_Target->IsTargetUnit() && !TESTMAPHANDLER->UnitCanMove( p_Target->unitData ) )
		return FALSE;

	//disable pushback on bosses
	if ( p_Target->IsTargetUnit() && p_Target->GetCharacterData()->bExclusiveBoss || p_Target->GetCharacterData()->sMonsterClass == EMonsterClass::Boss )
		return FALSE;

	int ang2, ang;

	//In case of strong attack power or critical judgment
	//If it's right, it forces you to step back
	ang2 = MAPSERVER->GetAngle(p_Target->GetPosition().iX, p_Target->GetPosition().iZ, pcRefPos->iX, pcRefPos->iZ );
	ang = ( ang2 + PTANGLE_180 ) & PTANGLE_Mask;

	p_Target->unitData->bNoMove = FALSE;
	p_Target->unitData->sAngle.iY = ang;
	p_Target->unitData->MoveAngle( iPushbackRange );
	p_Target->unitData->sAngle.iY = ang2;

	DEBUG( "%s: Pushback range %d", p_Target->GetName(), iPushbackRange );

	return TRUE;
}

/// <summary>
/// Flinch
/// Critical hit = 100% chance
/// Non-Critical hit = some kind of odds/chance. (except for monsters that cannot be flinched, aka bosses)
///
/// Pushback
/// Critical hit  = Pushback value from DB
/// Non-Critical hit = No pushback
///
/// Exceptions
/// Diastrophism, CL
BOOL CDamageHandler::ApplySkillFlinchAndPushback( UserData * pcUserData, BOOL bCritSuccess, int iFlinchChance, int iPushbackRange, UxxxTarget * p_Target, Point3D * pcSkillPos, EAttackState eAttackState, ESkillID p_SkillID, int skillCode)
{
	if ( p_Target->IsTargetUnit() && !TESTMAPHANDLER->UnitCanMove( p_Target->unitData ) )
		return FALSE;

	BOOL bForceFlinch = FALSE;
	BOOL bForcePushback = FALSE;

	switch ( p_SkillID )
	{
		case SKILLID_Diastrophism:
		{
			//Dia always flinches and pushback
			bForceFlinch = TRUE;
			//bForcePushback = TRUE;
		}
		break;

		case SKILLID_ChainLightning:
		{
			//2012 source does this btw.
			bForceFlinch = p_Target->GetCharacterData()->sElementalDef[EElementID::Lightning] < 20;
		}
		break;

		//skills that area already handled
		case SKILLID_PikeWind:
		{
			return FALSE;
		}

		default:
		{
			//attack states that are already handled or ignored
			if (eAttackState == EAttackState::State_201_PullIn ||
				eAttackState == EAttackState::State_2_Stun)
			{
				return FALSE;
			}
		}
	}

	//always flinch on critical
	if ( bCritSuccess )
	{
		bForceFlinch = TRUE;
	}

	const BOOL bFlinched = ApplyFlinch( pcUserData, p_Target, iFlinchChance, p_SkillID, skillCode, bForceFlinch );

	//only apply pushback on flinch and on critical success
	//units only
	if ( bFlinched && (bCritSuccess || bForcePushback) && p_Target->IsTargetUnit() )
	{
		//apply pushback based on db value
		ApplyPushback( p_Target, pcSkillPos, iPushbackRange, p_SkillID, skillCode);
	}

	return TRUE;
}

void CDamageHandler::AdjustDamageFromAbsorb(int * piDmg, UxxxTarget * p_Target)
{
	//Adjust power based on absorption
	*piDmg -= ( *piDmg * p_Target->GetCharacterData()->iAbsorbRating ) / 100;
}

/// <summary>
/// Apply damage buff by percentage based on skill id and monster type
/// </summary>
void CDamageHandler::ApplyMonsterTypeDamageBuffFromSkill( int * piDmg, UxxxTarget * p_Target, User * pcUser, int iSkillLevel1Based, ESkillID p_SkillID )
{
	int dmg = *piDmg;

	if ( dmg <= 0 )
		return;


	//apply damage boost vs demon
	if ( pcUser->bDemonBaneLevel1Based &&
		p_Target->IsTargetUnit() && p_Target->GetMonsterType() == EMonsterType::MONSTERTYPE_Demon )
	{
		int buff = GetSkillIntValueAt( pcUser, ESkillArrayPointer::Fighter_DemonBuff_DamageBoostVsDemon_New, pcUser->bDemonBaneLevel1Based );

		//half for non-fs
		if ( pcUser->pcUserData->sCharacterData.iClass != ECharacterClass::CHARACTERCLASS_Fighter )
			buff >>= 1;

		CHATSERVER->SendDebugChat( pcUser, CHATCOLOR_Error, "> [Demon Buff] Dmg Boost %d", buff );

		dmg += ( dmg * buff ) / 100;
	}

	//apply damage boost vs undead
	if ( pcUser->bHolyValorLevel1Based &&
		p_Target->IsTargetUnit() && p_Target->GetMonsterType() == EMonsterType::MONSTERTYPE_Undead )
	{
		int buff = KNIGHTSKILL->GetHolyValorBuff( pcUser );
		if ( buff > 0 )
		{
			CHATSERVER->SendDebugChat( pcUser, CHATCOLOR_Error, "> [Undead Buff] Dmg Boost %d", buff );
			dmg += ( dmg * buff ) / 100;
		}
	}

	switch ( p_SkillID )
	{
		case SKILLID_BombShot:
		{
			if (p_Target->IsTargetUnit() && p_Target->GetMonsterType() == MONSTERTYPE_Demon )
			{
				if ( iSkillLevel1Based )
				{
					int temp = GetSkillIntValueAt( pcUser, ESkillArrayPointer::Archer_BombShot_DMGDemons, iSkillLevel1Based );
					dmg += ( dmg * temp ) / 100;
				}
			}
			break;
		}

		case SKILLID_MechanicBomb:
		{
			//Mechanic Bomb (Add 50% to Mutant Mechanic Monster)
			if (p_Target->IsTargetUnit() && p_Target->GetMonsterType() == MONSTERTYPE_Mutant )
			{
				dmg += ( dmg * 50 ) / 100;
			}
			break;
		}

		case SKILLID_Spark:
		{
			//Spark (50% added to mechanic monsters)
			if (p_Target->IsTargetUnit() && p_Target->GetMonsterType() == MONSTERTYPE_Normal )
			{
				dmg += ( dmg * 50 ) / 100;
			}
			break;
		}

		case SKILLID_Expansion:
		{
			//Expansion (add 30% to Mutant Monster)
			if (p_Target->IsTargetUnit() && p_Target->GetMonsterType() == MONSTERTYPE_Mutant )
			{
				dmg += ( dmg * 30 ) / 100;
			}
			break;
		}

		case SKILLID_SwordBlast:
		{
			//Sword Blast (adds 30% normal and mutant monsters)
			if ((p_Target->IsTargetUnit() && p_Target->GetMonsterType() == MONSTERTYPE_Normal)
				|| (p_Target->IsTargetUnit() && p_Target->GetMonsterType() == MONSTERTYPE_Mutant ))
			{
				dmg += ( dmg * 30 ) / 100;
			}
			break;
		}

		case SKILLID_Piercing:
		{
			//Piercing (add 30% to Mutant Monster)
			if (p_Target->IsTargetUnit() && p_Target->GetMonsterType() == MONSTERTYPE_Mutant )
			{
				dmg += ( dmg * 30 ) / 100;
			}
			break;
		}

		case SKILLID_DivineLightning:
		{
			if (p_Target->IsTargetUnit() && p_Target->GetMonsterType() == MONSTERTYPE_Undead )
			{
				dmg += ( dmg * 50 ) / 100;
			}
			break;
		}

		case SKILLID_VigorBall:
		{
			if (p_Target->IsTargetUnit() && p_Target->GetMonsterType() == MONSTERTYPE_Undead )
			{
				dmg += ( dmg * 50 ) / 100;
			}
			break;
		}

		case SKILLID_ChainLightning:
		{
			if (p_Target->IsTargetUnit() && p_Target->GetMonsterType() == MONSTERTYPE_Undead )
			{
				dmg += ( dmg * 100 ) / 100;
			}
			break;
		}

		case SKILLID_LightningJavelin:
		{
			if (p_Target->IsTargetUnit() && p_Target->GetMonsterType() == MONSTERTYPE_Undead )
			{
				dmg += ( dmg * 50 ) / 100;
			}
			break;
		}

		case SKILLID_Impulsion:
		{
			//30% added to mechanic monsters
			if (p_Target->IsTargetUnit() && p_Target->GetMonsterType() == MONSTERTYPE_Normal )
			{
				dmg += ( dmg * 30 ) / 100;
			}
			break;
		}
	}

	*piDmg = dmg;
}

/// <summary>
/// Called every 1 second for user based on b64
/// </summary>
void CDamageHandler::UserTick1s( User * pcUser )
{
	ClearExpiredSkillBuffs( pcUser, FALSE);
	MAGICIANSKILL->ApplyEnchantWeaponRotation( pcUser );
}

/// <summary>
/// Clear expired skill buffs
/// This function gets called every ~ 1 second for for each user
/// </summary>
/// <param name="pcUser"></param>
void CDamageHandler::ClearExpiredSkillBuffs( User * pcUser, BOOL bForceExpiry )
{
	if ( pcUser == NULL || pcUser->IsValidAndInGame() == FALSE )
		return;

	ClearExpiredSkillBuffs( pcUser->pcUserData, bForceExpiry );



	USERSERVER->CheckAndUpdateSkillStatus( pcUser,		ESkillID::SKILLID_AdventMigal,
		pcUser->uAdventMigalEndTime,					pcUser->bAdventMigalLevel1Based,
		pcUser->sSkillBuffStatus.uAdventMigalTimeLeft,  pcUser->sSkillBuffStatus.bAdventMigalLevel1Based,
		bForceExpiry );

	USERSERVER->CheckAndUpdateSkillStatus( pcUser,		ESkillID::SKILLID_HolyValor,
		pcUser->uHolyValorEndTime,						pcUser->bHolyValorLevel1Based,
		pcUser->sSkillBuffStatus.uHolyValorTimeLeft,    pcUser->sSkillBuffStatus.bHolyValorLevel1Based,
		bForceExpiry );

	USERSERVER->CheckAndUpdateSkillStatus( pcUser,		ESkillID::SKILLID_HolyBody,
		pcUser->uHolyBodyEndTime,						pcUser->bHolyBodyLevel1Based,
		pcUser->sSkillBuffStatus.uHolyBodyTimeLeft,     pcUser->sSkillBuffStatus.bHolyBodyLevel1Based,
		bForceExpiry );

	USERSERVER->CheckAndUpdateSkillStatus( pcUser,			ESkillID::SKILLID_DrasticSpirit,
		pcUser->uDrasticSpiritEndTime,						pcUser->bDrasticSpiritLevel1Based,
		pcUser->sSkillBuffStatus.uDrasticSpiritTimeLeft,    pcUser->sSkillBuffStatus.bDrasticSpiritLevel1Based,
		bForceExpiry );

	//when Drastic Spirit expires, set the old holy body values to zero
	//the old values reduces damage from undead in ASM
	if ( pcUser->uDrasticSpiritEndTime == 0 )
	{
		pcUser->pcUserData->dwHolyBodyEndTime = 0;
		pcUser->pcUserData->iHolyBodyAbsorb = 0;
	}

	USERSERVER->CheckAndUpdateSkillStatus( pcUser,		ESkillID::SKILLID_Alas,
		pcUser->uAlasEndTime,							pcUser->bAlasLevel1Based,
		pcUser->sSkillBuffStatus.uAlasTimeLeft,			pcUser->sSkillBuffStatus.bAlasLevel1Based,
		bForceExpiry );

	USERSERVER->CheckAndUpdateSkillStatus( pcUser,		ESkillID::SKILLID_Inpes,
		pcUser->uInpesEndTime,							pcUser->bInpesLevel1Based,
		pcUser->sSkillBuffStatus.uInpesTimeLeft,		pcUser->sSkillBuffStatus.bInpesLevel1Based,
		bForceExpiry );

	USERSERVER->CheckAndUpdateSkillStatus( pcUser,			ESkillID::SKILLID_ForceofNature,
		pcUser->uForceOfNatureEndTime,						pcUser->bForceOfNature1Based,
		pcUser->sSkillBuffStatus.uForceOfNatureTimeLeft,	pcUser->sSkillBuffStatus.bForceOfNatureLevel1Based,
		bForceExpiry );

	USERSERVER->CheckAndUpdateSkillStatus( pcUser,		ESkillID::SKILLID_VirtualLife,
		pcUser->uVirtualLifeEndTime,					pcUser->bVirtualLifeLevel1Based,
		pcUser->sSkillBuffStatus.uVirtualLifeTimeLeft,  pcUser->sSkillBuffStatus.bVirtualLifeLevel1Based,
		bForceExpiry );

	USERSERVER->CheckAndUpdateSkillStatus( pcUser,		ESkillID::SKILLID_VirtualRegen,
		pcUser->uVirtualRegenEndTime,					pcUser->bVirtualRegenLevel1Based,
		pcUser->sSkillBuffStatus.uVirtualRegenTimeLeft, pcUser->sSkillBuffStatus.bVirtualRegenLevel1Based,
		bForceExpiry );

	USERSERVER->CheckAndUpdateSkillStatus( pcUser,				ESkillID::SKILLID_TriumphofValhalla,
		pcUser->uTriumphOfValhallaEndTime,						pcUser->bTriumphOfValhallaLevel1Based,
		pcUser->sSkillBuffStatus.uTriumphOfValhallaTimeLeft,	pcUser->sSkillBuffStatus.bTriumphOfValhallaLevel1Based,
		bForceExpiry );

	USERSERVER->CheckAndUpdateSkillStatus( pcUser,			ESkillID::SKILLID_HallofValhalla,
		pcUser->uHallOfValhallaEndTime,						pcUser->bHallOfValhallaLevel1Based,
		pcUser->sSkillBuffStatus.uHallOfValhallaTimeLeft,	pcUser->sSkillBuffStatus.bHallOfValhallaLevel1Based,
		bForceExpiry );

	USERSERVER->CheckAndUpdateSkillStatus( pcUser,		ESkillID::SKILLID_DemonsBane,
		pcUser->uDemonBaneEndTime,						pcUser->bDemonBaneLevel1Based,
		pcUser->sSkillBuffStatus.uDemonBaneTimeLeft,	pcUser->sSkillBuffStatus.bDemonBaneLevel1Based,
		bForceExpiry );

	USERSERVER->CheckAndUpdateSkillStatus( pcUser,		ESkillID::SKILLID_VenomThorn,
		pcUser->uVenomThornEndTime,						pcUser->bVenomLevel1Based,
		pcUser->sSkillBuffStatus.uVenomThornTimeLeft,   pcUser->sSkillBuffStatus.bVenomLevel1Based,
		bForceExpiry );

	USERSERVER->CheckAndUpdateSkillStatus(pcUser, ESkillID::SKILLID_ElectricalOverload,
		pcUser->uElectricalOverloadEndTime, pcUser->bElectricalOverloadLevel1Based,
		pcUser->sSkillBuffStatus.uElectricalOverloadTimeLeft, pcUser->sSkillBuffStatus.bElectricalOverloadLevel1Based,
		bForceExpiry);

	USERSERVER->CheckAndUpdateSkillStatus(pcUser, ESkillID::SKILLID_EnchantWeapon,
		pcUser->uEnchantWeaponEndTime, pcUser->bEnchantWeaponLevel1Based,
		pcUser->sSkillBuffStatus.uEnchantTimeLeft, pcUser->sSkillBuffStatus.bEnchantLevel1Based,
		bForceExpiry);

	if ( USERSERVER->CheckAndUpdateSkillStatus( pcUser, ESkillID::SKILLID_Blind,
		pcUser->uConcealEndTime,						pcUser->bConcealLevel1Based,
		pcUser->sSkillBuffStatus.uConcealTimeLeft,		pcUser->sSkillBuffStatus.bConcealLevel1Based,
		bForceExpiry ) == FALSE )
	{
		pcUser->pcUserData->saVanishParam[0] = 0;
		pcUser->pcUserData->saVanishParam[1] = 0;
	}

	//Lethal Sight
	if ( pcUser->uChasingHuntEndTime &&
	   (TIME > pcUser->uChasingHuntEndTime || bForceExpiry) )
	{
		pcUser->uChasingHuntEndTime = 0;
		pcUser->iChasingHuntAttackPowerBoost = 0;
	}

	//Lethal Sight
	if ( pcUser->uLethalSightTimeOut &&
	   (TIME > pcUser->uLethalSightTimeOut || bForceExpiry) )
	{
		pcUser->uLethalSightTimeOut = 0;
		pcUser->uLethalSightLevel	= 0;
		pcUser->pcUserData->dwAssassinEyeEndTime = 0;

		DAMAGEHANDLER->SendCancelSkill( pcUser, SKILLPLAYID_T51 );
	}


	if ( pcUser->uEnchantWeaponEndTime &&
		(TIME > pcUser->uEnchantWeaponEndTime || bForceExpiry) )
	{
		pcUser->uEnchantWeaponNextSwitchTime = 0;
		pcUser->uEnchantWeaponEndTime = 0;
		pcUser->eEnchantWeaponElement = EElementID::None;
		pcUser->sEnchantWeaponAtkPowerPercent = 0;
		pcUser->sEnchantWeaponParameter = 0;
		pcUser->bEnchantWeaponChance = 0;
		pcUser->bEnchantWeaponDurationSec = 0;
		pcUser->bEnchantWeaponLevel1Based = 0;

		DAMAGEHANDLER->SendCancelSkill( pcUser, SKILLID_EnchantWeapon );
	}


	if ( pcUser->uBerserkerEndTime &&
		(TIME > pcUser->uBerserkerEndTime || bForceExpiry) )
	{
		pcUser->uBerserkerEndTime = 0;
		pcUser->sBersekerDamageBoost = 0;
		pcUser->sBersekerDamageToHPPercent = 0;
	}

	if ( pcUser->uDancingSwordEndTime &&
		 (TIME > pcUser->uDancingSwordEndTime || bForceExpiry) )
	{
		pcUser->uDancingSwordEndTime = 0;
		pcUser->bDancingSwordLevel = 0;
		pcUser->eDancingSwordElement = EElementID::None;
		pcUser->bDancingSwordChainLevelEW = 0;
		pcUser->bDancingSwordChainLevelDR = 0;
	}

	if (pcUser->uFrostJavelinEndTime &&
		(TIME > pcUser->uFrostJavelinEndTime || bForceExpiry) )
	{
		pcUser->uFrostJavelinEndTime = 0;
		pcUser->sFrostJavelinAtkPowerPercent = 0;
		pcUser->bFrostJavelinChance = 0;
		pcUser->bFrostJavelinDurationSec = 0;
		pcUser->bFrostJavelinWeaponMonsterSlowdown = 0;
	}

	if ( bForceExpiry )
	{
		ZeroMemory( &pcUser->sSkillBuffStatus, sizeof( PacketSkillBuffStatus ) );

		if ( pcUser->uSkillBuffHash != 0 )
		{
			USERSERVER->SendSkillBuffStatus( pcUser, pcUser );
		}

		pcUser->uSkillBuffHash = 0;
	}
	else
	{
		if ( !CHARACTERSERVER->CharacterInTown( pcUser->pcUserData ) )
		{
			if ( USERSERVER->UpdateSkillBuffHash( pcUser ) )
			{
				//send on change to self only
				//for other users, it will be done via LoopUser
				USERSERVER->SendSkillBuffStatus( pcUser, pcUser );
			}
		}
	}
}

void CDamageHandler::ClearExpiredSkillBuffs( UserData * pcUserData, BOOL bForceExpiry )
{
	if ( pcUserData == NULL )
		return;

	if (pcUserData->dwVanishEndTime &&
		(TICKCOUNT > pcUserData->dwVanishEndTime || bForceExpiry))
	{
		pcUserData->dwVanishEndTime = 0;

		pcUserData->saVanishParam[0] = 0;
		pcUserData->saVanishParam[1] = 0;

		DAMAGEHANDLER->SendCancelSkill(  USERDATATOUSER(pcUserData), PikemanSkill::Vanish );
	}

	if ( pcUserData->dwAssassinEyeEndTime &&
		(TICKCOUNT > pcUserData->dwAssassinEyeEndTime || bForceExpiry) )
	{
		pcUserData->dwAssassinEyeEndTime = 0;
		pcUserData->AssassinEyeParam = 0;
	}

	if ( pcUserData->dwEnergyShieldEndTime &&
		(TICKCOUNT > pcUserData->dwEnergyShieldEndTime || bForceExpiry) )
	{
		pcUserData->dwEnergyShieldEndTime = 0;
		pcUserData->iEnergyShieldDamageReduce = 0;
	}

	if ( pcUserData->dwCurse_AttackTime &&
		(TICKCOUNT > pcUserData->dwCurse_AttackTime || bForceExpiry))
	{
		pcUserData->dwCurse_AttackTime = 0;
		pcUserData->dwCurse_AttackParam = 0;
	}


	for ( int i = 0; i < PVPBUFF_COUNT; i++ )
	{
		if ( pcUserData->saBCBuff[i].dwItemEndTime &&
			(TICKCOUNT > pcUserData->saBCBuff[i].dwItemEndTime || bForceExpiry) )
		{
			pcUserData->saBCBuff[i].dwItemEndTime = 0;
			pcUserData->saBCBuff[i].iItemID = 0;
			pcUserData->saBCBuff[i].iValueData = 0;
		}
	}
}

/// <summary>
/// Clear expired debuffs / buffs if any on this unit
/// </summary>
void CDamageHandler::ClearExpiredDebuffs( Unit * pcUnit )
{
	if ( pcUnit == NULL )
		return;

	ClearExpiredDebuffs( pcUnit->pcUnitData );


	if ( pcUnit->bCursed &&
		 pcUnit->pcUnitData->iCursedTimeLeft == 0 )
	{
		pcUnit->bCursed = FALSE;
		pcUnit->pcUnitData->iCursedTimeLeft = 0;
		pcUnit->pcUnitData->bDamageReduction = 0;
	}

	if ( pcUnit->sPoisonDamagePerStack &&
		 pcUnit->bPoisonCount == 0  )
	{
		DEBUG( "%s: Poison damage expired", pcUnit->GetName() );

		pcUnit->dwPoisonNextTime = 0;
		pcUnit->sPoisonDamagePerStack = 0;
		pcUnit->bPoisonStackCount = 0;
		pcUnit->bPoisonCount = 0;
		pcUnit->idPoisonCastedBy = 0;
	}

	if ( pcUnit->sBurningDamagePerTick &&
		 pcUnit->bBurningCount == 0  )
	{
		DEBUG( "%s: Burning damage expired", pcUnit->GetName() );

		pcUnit->dwBurningNextTime = 0;
		pcUnit->sBurningDamagePerTick = 0;
		pcUnit->bBurningCount = 0;
		pcUnit->idPoisonCastedBy = 0;
	}
}

/// <summary>
/// Clear expired debuffs / buffs if any on this unit
/// </summary>
void CDamageHandler::ClearExpiredDebuffs( UnitData * pcUnitData )
{
	if ( pcUnitData == NULL )
		return;

	if ( pcUnitData->iSlowTimeLeft <= 0 &&
		 pcUnitData->iIceOverlay )
	{
		pcUnitData->iIceOverlay = FALSE;
		pcUnitData->iSlowTimeLeft = 0;
		pcUnitData->iPlaySlowSpeed = 0;
	}

	if ( pcUnitData->iSlowTimeLeft <= 0 &&
		 pcUnitData->bDistortion )
	{
		pcUnitData->bDistortion = FALSE;
		pcUnitData->iSlowTimeLeft = 0;
		pcUnitData->iPlaySlowSpeed = 0;
		pcUnitData->bDamageReduction = 0;
	}

	if ( pcUnitData->dwAssassinEyeEndTime &&
		TICKCOUNT > pcUnitData->dwAssassinEyeEndTime )
	{
		DEBUG( "%s: Assassin's eye expired", pcUnitData->GetName() );

		pcUnitData->dwAssassinEyeEndTime = 0;
		pcUnitData->sAssassinEyeCritDmg = 0;
		pcUnitData->sAssassinEyeCritChance = 0;
	}
}


/// <summary>
/// Set stun effect.
/// This is typically from Enchant Weapon's lightning enchant or lighting skills or on critical hit.
/// The duration is already adjusted elsewhere. All we're checking is is existing element effects applied on unit
/// and that the unit isn't a boss.
/// </summary>
BOOL CDamageHandler::SetStun( User * pcAttacker, int iDurationSec, UxxxTarget * p_Target)
{
	/// todo iron/hardcpp how we apply this to pvp ?
	if (!p_Target->IsTargetUnit())
		return FALSE;

    /// todo iron
    auto pcUnitData = p_Target->unitData;

	//no stun duration
	if ( iDurationSec == 0 )
		return FALSE;

	//Disallow stun for boss
	if (p_Target->IsTargetUnit() && p_Target->GetMonsterClass() == EMonsterClass::Boss)
		return FALSE;

	//unit is burning or is posioned,
	//dont apply stun since damage is being applied over time
	if ( pcUnitData->pcUnit->bBurningCount || pcUnitData->pcUnit->bPoisonCount )
		return FALSE;

	//unit is frozen or distorted. don't apply stun either.
	//alternative option is to replace but might get messy
	//when multiple players are attacking unit
	if ( pcUnitData->iIceOverlay || pcUnitData->bDistortion )
		return FALSE;

	//stun duration is longer than current?
	// ~ monster updates is done at about ~ 16 times per second hence why it's * 16
	if ( iDurationSec * 16 > pcUnitData->iStunTimeLeft )
	{
		DEBUG( "%s is stunned for %d seconds", pcUnitData->GetName(), iDurationSec );

		if ( pcUnitData->iStunTimeLeft == 0 )
			pcUnitData->Animate( EAnimationType::ANIMATIONTYPE_Flinch );

		pcUnitData->iStunTimeLeft = iDurationSec * 16;
		pcUnitData->iMoveCount = 0;

		return TRUE;
	}

	return TRUE;
}

BOOL CDamageHandler::ApplyPikeWind( User * pcAttacker, PacketSkillDamageRange * psRangeData, UxxxTarget * p_Target, ESkillID p_SkillID, int p_SkillCode)
{
	/// push back does not get applied to pvp
    if (!p_Target->IsTargetUnit())
        return FALSE;

    auto pcUnitData = p_Target->unitData;

	if (p_Target->IsTargetUnit() && p_Target->GetMonsterClass() == EMonsterClass::Boss )
		return FALSE;

	if ( p_SkillID != SKILLID_PikeWind )
		return FALSE;

	int range = psRangeData->iRange;

	//adjust pushback range based on position between player and unit
	int distXY = p_Target->GetPosition().GetPTDistanceXZ( &psRangeData->sPosition );
	distXY = (int)sqrt( distXY );
	range -= distXY;

	if ( range < 1 )
		return FALSE;

	//set next code to idle
	//if this is used: pcUnitData->Animate( EAnimationType::ANIMATIONTYPE_Flinch ) the unit will 'jump' / 'teleport'
	pcUnitData->eNextMotionCode = EAnimationType::ANIMATIONTYPE_Idle;

	ApplyPushback(p_Target, &psRangeData->sPosition, range, p_SkillID, p_SkillCode );

	return TRUE;
}

/// <summary>
/// Apply stun effect from lightning attacks
/// but don't apply damage
/// </summary>
BOOL CDamageHandler::ApplyLightningStun( User * pcAttacker, EAttackProperty eAttackProperty, ElementalAttackSetting * sSetting, UxxxTarget * p_Target, ESkillID p_SkillID, int p_SkillCode)
{
	if ( eAttackProperty != EAttackProperty::Lightning )
		return FALSE;

	if ( sSetting->bDurationSec == 0 || sSetting->bChance == 0)
	{
#ifdef DEV_MODE
		WARN( "ApplyLightningStun has missing required parameters!" );
#endif
		return FALSE;
	}

	//Try and apply the stun!
	return (sSetting->bChance > Dice::RandomI(0, 99)) ? SetStun( pcAttacker, sSetting->bDurationSec, p_Target) : FALSE;
}

BOOL CDamageHandler::ApplyStun( User * pcAttacker, EAttackState eAttackState, ElementalAttackSetting * sSetting, UxxxTarget * p_Target, ESkillID p_SkillID, int p_SkillCode)
{
	//bosses don't take stun
	if (p_Target->IsTargetUnit() && p_Target->GetMonsterClass() == EMonsterClass::Boss)
		return FALSE;

	if ( eAttackState != EAttackState::State_2_Stun )
		return FALSE;

	if ( sSetting->bDurationSec == 0 || sSetting->bChance == 0)
	{
		WARN( "ApplyStun has missing required parameters!" );
		return FALSE;
	}

	int chance = GetResistanceAdjustedValue(p_Target, sSetting->bChance, EElementID::Organic );

	//chance to apply
	if ((chance > Dice::RandomI( 0, 99 )) == FALSE)
		return FALSE;

	//damage is based on attack power
	int dmg = pcAttacker->iLatestAttackPower;

	//adjust damage based on skill's damage percentage value
	//to yield the final damage
	dmg = ( dmg * sSetting->sPercentOfAP ) / 100;

	//Adjust damage based on monster's elemental resistance
	AdjustDamageFromMonsterResistence( EAttackProperty::Organic, &dmg, p_Target);

	int duration = sSetting->bDurationSec * 16; //iStunTimeLeft is based on * 16

	duration = GetResistanceAdjustedValue (p_Target, duration, EElementID::Organic );


	//only apply / refresh duration if longer
	//AND if damage is applied
	/// todo iron/hardcpp how we apply stun in pvp ?
	if (p_Target->IsTargetUnit() &&  duration > p_Target->unitData->iStunTimeLeft && dmg > 0 )
    {
        /// todo iron
        auto pcUnitData = p_Target->unitData;

		DEBUG( "%s: Stunned for %d seconds", pcUnitData->GetName(), duration );

		//only if NOT already stunned
        BOOL bIsCurrentlyStunned = pcUnitData->iStunTimeLeft > 0;
		if ( bIsCurrentlyStunned == FALSE )
		{
			pcUnitData->TakeHealth( dmg );
			pcUnitData->CheckHealthStatusAfterDamageReceived( pcAttacker->pcUserData );

			//record user's damage to this unit
			CustomRecordCharacterDamage( pcAttacker->pcUserData, pcUnitData, dmg );

			MAPSERVER->SendUnitDamageInfoTextToNearbyPlayers( pcAttacker->pcUserData, pcUnitData, dmg, FALSE, 0, EDamageTextType::Stun );

			pcUnitData->Animate( EAnimationType::ANIMATIONTYPE_Flinch );
		}

		//refresh duration regardless if the unit is stunned or not
		pcUnitData->iStunTimeLeft = duration;

		return TRUE;
	}

	return FALSE;
}

/// <summary>
/// Apply distortion effect
/// </summary>
BOOL CDamageHandler::ApplyDistortion( User * pcAttacker, ElementalAttackSetting * sSetting, UxxxTarget * p_Target, ESkillID p_SkillID, int p_SkillCode)
{
	//damage is based on attack power
	int dmg = pcAttacker->iLatestAttackPower;

	if (p_Target->IsTargetUnit() && p_Target->GetMonsterClass() == EMonsterClass::Boss)
		return FALSE;

	if ( sSetting->sPercentOfAP == 0 || sSetting->bChance == 0 || sSetting->bParameter == 0 || sSetting->bDurationSec == 0 )
	{
		WARN( "ApplyDistortion has missing required parameters!" );
		return FALSE;
	}

	int chance = GetResistanceAdjustedValue(p_Target, sSetting->bChance, EElementID::Organic );

	//chance to apply
	if ((chance > Dice::RandomI( 0, 99 )) == FALSE)
		return FALSE;

	//Adjust damage based on monster's elemental resistance
	AdjustDamageFromMonsterResistence( EAttackProperty::Organic, &dmg, p_Target);

	//adjust damage based on skill's damage percentage value
	//to yield the final damage
	dmg = ( dmg * sSetting->sPercentOfAP ) / 100;

	if (p_Target->IsTargetUnit() && dmg > 0 )
    {
        /// todo iron
        auto pcUnitData = p_Target->unitData;

		int slowSpeed = SLOW_SPEED_BASE - ( sSetting->bParameter * SLOW_SPEED_BASE ) / 100;

		//Apply 'slow', and only if the new value is higher than existing
		if ( slowSpeed > pcUnitData->iPlaySlowSpeed )
		{
			pcUnitData->iPlaySlowSpeed = slowSpeed;
			pcUnitData->iSlowTimeLeft = sSetting->bDurationSec * 16; //refresh duration on reapply

			//monster does not have distortion applied. Therefore
			//apply one-off damage on effect apply
			if ( pcUnitData->bDistortion == FALSE )
			{
				pcUnitData->TakeHealth( dmg );
				pcUnitData->CheckHealthStatusAfterDamageReceived( pcAttacker->pcUserData );

				//record user's damage to this unit
				CustomRecordCharacterDamage( pcAttacker->pcUserData, pcUnitData, dmg );

				MAPSERVER->SendUnitDamageInfoTextToNearbyPlayers( pcAttacker->pcUserData, pcUnitData, dmg, FALSE, 0, EDamageTextType::Distortion );
			}

			DEBUG( "%s: Distortion applied!", pcUnitData->GetName() );

			pcUnitData->bDamageReduction = static_cast<BYTE>(sSetting->sDamageReduction);
			pcUnitData->bDistortion = TRUE;
			pcUnitData->pcUnit->bCursed = FALSE;
			pcUnitData->iIceOverlay = FALSE;
		}
	}
	else if (p_Target->IsTargetPlayer() && dmg > 0)
	{
		SendDamagePvPData(pcAttacker->pcUserData, p_Target->userData, 1, 0, State_128_Skill, p_SkillID, -p_SkillCode, sSetting->bDurationSec * 16);
        DEBUG("[PVP] %s: Distortion applied!", p_Target->GetName());
	}

	return TRUE;
}


/// <summary>
/// Apply distortion effect
/// </summary>
BOOL CDamageHandler::ApplyCurse( User * pcAttacker, ElementalAttackSetting * sSetting, UxxxTarget * p_Target, ESkillID p_SkillID, int p_SkillCode)
{
	//damage is based on attack power
	int dmg = pcAttacker->iLatestAttackPower;

	if ( p_Target->IsTargetUnit() && p_Target->GetMonsterClass() == EMonsterClass::Boss)
		return FALSE;

	if ( sSetting->sPercentOfAP == 0 || sSetting->bChance == 0 || sSetting->bSpeedReduction == 0 || sSetting->bDurationSec == 0 || sSetting->sDamageReduction == 0 )
	{
		WARN( "ApplyCurse has missing required parameters!" );
		return FALSE;
	}


	int chance = GetResistanceAdjustedValue(p_Target, sSetting->bChance, EElementID::Organic );

	//chance to apply
	if ((chance > Dice::RandomI( 0, 99 )) == FALSE)
		return FALSE;

	//Adjust damage based on monster's elemental resistance
	AdjustDamageFromMonsterResistence( EAttackProperty::Organic, &dmg, p_Target);

	//adjust damage based on skill's damage percentage value
	//to yield the final damage
	dmg = ( dmg * sSetting->sPercentOfAP ) / 100;

	/// todo iron/hardcpp how we apply this to pvp ?
	if (p_Target->IsTargetUnit() && dmg > 0 )
    {
		/// todo iron
        auto pcUnitData = p_Target->unitData;
		int slowSpeed = SLOW_SPEED_BASE - ( sSetting->bSpeedReduction * SLOW_SPEED_BASE ) / 100;

		//Apply 'slow', and only if the new value is higher than existing
		if ( slowSpeed > pcUnitData->iPlaySlowSpeed )
		{
			pcUnitData->iPlaySlowSpeed = slowSpeed;
			pcUnitData->iSlowTimeLeft = sSetting->bDurationSec * 16; //refresh duration on reapply

			//monster does not have cursed applied. Therefore
			//apply one-off damage on effect apply
			if ( pcUnitData->pcUnit->bCursed == FALSE )
			{
				pcUnitData->TakeHealth( dmg );
				pcUnitData->CheckHealthStatusAfterDamageReceived( pcAttacker->pcUserData );

				//record user's damage to this unit
				CustomRecordCharacterDamage( pcAttacker->pcUserData, pcUnitData, dmg );

				MAPSERVER->SendUnitDamageInfoTextToNearbyPlayers( pcAttacker->pcUserData, pcUnitData, dmg, FALSE, 0, EDamageTextType::Curse );
			}

			DEBUG( "%s: Cursed applied!", pcUnitData->GetName() );

			pcUnitData->bDamageReduction = static_cast<BYTE>(sSetting->sDamageReduction);

			if (p_SkillID == ESkillID::SKILLID_Wisp || p_SkillID == ESkillID::SKILLID_Cripple )
				pcUnitData->pcUnit->bCursed = 2;
			else
				pcUnitData->pcUnit->bCursed = TRUE;

			pcUnitData->bDistortion = FALSE;
			pcUnitData->iIceOverlay = FALSE;
		}
	}

	return TRUE;
}

/// <summary>
/// Apply freezing effect
/// </summary>
BOOL CDamageHandler::ApplyFreezing( User * pcAttacker, ElementalAttackSetting * sSetting, UxxxTarget * p_Target, ESkillID p_SkillID, int p_SkillCode)
{
	//unit is burning, don't apply freezing effect
	//alternative is that we remove freezing effect but
	//don't apply burning damage unless attacked next time
	if (p_Target->IsTargetUnit() && p_Target->unitData->pcUnit->bBurningCount )
		return FALSE;

	if ( sSetting->sPercentOfAP == 0 || sSetting->bChance == 0 || sSetting->bParameter == 0 || sSetting->bDurationSec == 0 )
	{
		WARN( "ApplyFreeze has missing required parameters!" );
		return FALSE;
	}


	int chance = GetResistanceAdjustedValue(p_Target, sSetting->bChance, EElementID::Ice );

	//chance to apply
	if ((chance > Dice::RandomI( 0, 99 )) == FALSE)
		return FALSE;

	//damage is based on attack power
	int dmg = pcAttacker->iLatestAttackPower;


	//Adjust damage based on monster's elemental resistance
	AdjustDamageFromMonsterResistence( EAttackProperty::Ice, &dmg, p_Target);

	//adjust damage based on skill's damage percentage value
	//to yield the final burning damage
    dmg = (dmg * sSetting->sPercentOfAP) / 100;
    int slowSpeed = SLOW_SPEED_BASE - (sSetting->bSpeedReduction * SLOW_SPEED_BASE) / 100;

	/// todo iron/hardcpp how we apply this to pvp ?
	if (p_Target->IsTargetUnit() && dmg > 0 )
    {
		/// todo iron
        auto pcUnitData = p_Target->unitData;

		//slowness doesn't affect bosses
		if (p_Target->IsTargetUnit() && p_Target->GetMonsterClass() == EMonsterClass::Boss )
		{
			pcUnitData->TakeHealth( dmg );
			pcUnitData->CheckHealthStatusAfterDamageReceived( pcAttacker->pcUserData );
			CustomRecordCharacterDamage( pcAttacker->pcUserData, p_Target->unitData, dmg );
			MAPSERVER->SendUnitDamageInfoTextToNearbyPlayers( pcAttacker->pcUserData, pcUnitData, dmg, FALSE, 0, EDamageTextType::None );
		}
		else
		{
			//Apply 'slow', and only if the new value is higher than existing
			if ( slowSpeed > pcUnitData->iPlaySlowSpeed )
			{
				pcUnitData->iPlaySlowSpeed = slowSpeed;
				pcUnitData->iSlowTimeLeft = sSetting->bDurationSec * 16; //refresh duration on reapply

				//monster does not have freezing applied. Therefore
				//apply one-off damage on effect apply
				if ( pcUnitData->iIceOverlay == FALSE )
				{
					pcUnitData->TakeHealth( dmg );
					pcUnitData->CheckHealthStatusAfterDamageReceived( pcAttacker->pcUserData );

					//record user's damage to this unit
					CustomRecordCharacterDamage( pcAttacker->pcUserData, pcUnitData, dmg );

					MAPSERVER->SendUnitDamageInfoTextToNearbyPlayers( pcAttacker->pcUserData, pcUnitData, dmg, FALSE, 0, EDamageTextType::Freezing );
				}

				pcUnitData->pcUnit->bCursed = FALSE;
				pcUnitData->bDistortion = FALSE;
				pcUnitData->iIceOverlay = TRUE;
			}
		}


		DEBUG( "%s: Freezing dmg %d applied. Slow speed: %d", pcUnitData->GetName(), pcUnitData->pcUnit->sPoisonDamagePerStack, sSetting->bParameter );

		return TRUE;
	}

	//don't set to zero here as there could already be burning damage applied
	//It will get cleared out in ClearExpiredDebuffs

	return FALSE;
}

/// <summary>
/// Calculation of resistance properties
/// Based on: int GetResistanceAdjustedValue( int power , int ResCode ) from 2012 code
/// </summary>
int CDamageHandler::GetResistanceAdjustedValue(UxxxTarget * p_Target, int value, enum EElementID eElementID )
{
	int rs,result;

	rs = p_Target->GetCharacterData()->sElementalDef[eElementID];

	if ( rs < -100 ) rs = -100;
	if ( rs >= 100 ) rs = 100;
	rs = 100 - rs;

	result = ( value * rs ) / 100;

	if ( result < 0 )
		result = 0;

	return result;
}

int CDamageHandler::ApplyExtinction( EAttackState eAttackState, ElementalAttackSetting * sSetting,  UxxxTarget * p_Target, ESkillID p_SkillID, int p_SkillCode)
{
	if ( eAttackState != EAttackState::State_6_Extinction )
		return 0;

	//monsters only
	if ( p_Target->IsTargetPlayer() )
		return 0;

	//won't work on bosses
	if ( p_Target->unitData->sCharacterData.sMonsterClass == EMonsterClass::Boss )
		return 0;

	//undead only
	if ( p_Target->unitData->sCharacterData.iMonsterType != EMonsterType::MONSTERTYPE_Undead )
		return 0;

	if ((sSetting->bChance > Dice::RandomI( 0, 99 )) == FALSE)
		return FALSE;

	const int hpReductionCap = sSetting->sParameter;

	int hpReduction = ( p_Target->unitData->GetCurrentHealth() * sSetting->bParameter ) / 100;

	if ( hpReductionCap > 0 && hpReduction > hpReductionCap )
		return hpReductionCap;

	return hpReduction;
}

/// <summary>
/// Apply life steal based on damage
/// </summary>
int CDamageHandler::ApplyLifeSteal( EAttackState eAttackState, LifeStealParam sLifeStealParam, UxxxTarget * p_Target, ESkillID p_SkillID, int p_SkillCode)
{
	if ( eAttackState != EAttackState::State_4_LifeSteal )
		return 0;

	if ( sLifeStealParam.iLifeStealPercent == 0 )
		return 0;


	//the total damage that will be used
	//to calculate the amount of life to steal
	float hpStolen = (sLifeStealParam.iLifeStealPercent * p_Target->GetCurrentHealth()) / 100.0f;

	if ( p_Target->IsTargetUnit() && p_Target->unitData->sCharacterData.sMonsterClass == EMonsterClass::Boss )
	{
		//half it for bosses
		hpStolen -= ( hpStolen * 50 ) / 100;
	}

	hpStolen = static_cast<float>( GetResistanceAdjustedValue( p_Target, static_cast<int>( round( hpStolen ) ), EElementID::Organic ) );

	if ( sLifeStealParam.iLifeStealHealthPointCap > 0 && hpStolen > sLifeStealParam.iLifeStealHealthPointCap )
		hpStolen = static_cast<float>(sLifeStealParam.iLifeStealHealthPointCap);

	//apply global damage reduction in pvp to life steal
	if ( p_Target->IsTargetPlayer() )
	{
		hpStolen /= fGlobalDamageReduction;
	}

	return static_cast<int>( round( hpStolen ) );
}

BOOL CDamageHandler::ApplyEnchantWeapon ( User * pcAttacker, EAttackProperty eAttackProperty, UxxxTarget * p_Target, BOOL bMultiTarget, ESkillID p_SkillID, int p_SkillCode)
{
	//No enchantment weapon active.
	if ( pcAttacker->uEnchantWeaponEndTime == 0 )
		return FALSE;

	ElementalAttackSetting sEAS;
	ZeroMemory( &sEAS, sizeof( ElementalAttackSetting ) );
	sEAS.bChance = pcAttacker->bEnchantWeaponChance;
	sEAS.sPercentOfAP = pcAttacker->sEnchantWeaponAtkPowerPercent;
	sEAS.bDurationSec = pcAttacker->bEnchantWeaponDurationSec;

	//Fire weapon enchantment is active
	if ( pcAttacker->eEnchantWeaponElement == EElementID::Fire )
	{
		//only handled in single-target skills
		if ( bMultiTarget == TRUE )
			return FALSE;

		//Allowed skill attack property for fire weapon elemental
		if ( eAttackProperty == EAttackProperty::Undefined ||
			 eAttackProperty == EAttackProperty::None ||
			 eAttackProperty == EAttackProperty::Fire )
		{
			DEBUG( "Fire enchantment weapon applied!" );

			return ApplyBurning( pcAttacker, &sEAS, p_Target, p_SkillID, p_SkillCode);
		}
	}

	//Ice weapon enchantment is active
	else if ( pcAttacker->eEnchantWeaponElement == EElementID::Ice )
	{
		//only handled in single-target skills
		if ( bMultiTarget == TRUE )
			return FALSE;

		//Allowed skill attack property for ice weapon elemental
		if ( eAttackProperty == EAttackProperty::Undefined ||
			 eAttackProperty == EAttackProperty::None ||
			 eAttackProperty == EAttackProperty::Ice )
		{
			DEBUG( "Ice enchantment weapon applied!" );

			sEAS.bParameter = static_cast<BYTE>(pcAttacker->sEnchantWeaponParameter); //slow strength in %

			return ApplyFreezing(pcAttacker, &sEAS, p_Target, p_SkillID, p_SkillCode);
		}
	}

	//Lightning weapon enchantment is active
	else if ( pcAttacker->eEnchantWeaponElement == EElementID::Lightning )
	{
		//Only handled in multi-target skill code
		if ( bMultiTarget == FALSE )
			return FALSE;

		//Allowed skill attack property for lightning weapon elemental
		if ( eAttackProperty == EAttackProperty::Undefined ||
			 eAttackProperty == EAttackProperty::None)
		{
			DEBUG( "Lightning enchantment weapon applied!" );

			sEAS.bParameter = static_cast<BYTE>(pcAttacker->sEnchantWeaponParameter); //aoe range

			return ApplyEnchantWeaponLightningAoE( pcAttacker, &sEAS, p_Target, p_SkillID, p_SkillCode);
		}
	}

	return FALSE;
}

/// <summary>
/// Apply enchant weapon lightning area of effect to nearby targets
/// </summary>
BOOL CDamageHandler::ApplyEnchantWeaponLightningAoE( User * pcAttacker, ElementalAttackSetting * sSetting, UxxxTarget * p_Target, ESkillID p_SkillID, int p_SkillCode)
{
	if ( sSetting->sPercentOfAP == 0 || sSetting->bChance == 0 || sSetting->bDurationSec == 0 || sSetting->bParameter == 0)
	{
		WARN( "ApplyEnchantWeaponLightningAoE has missing required parameters!" );
		return FALSE;
	}

	int chance = GetResistanceAdjustedValue(p_Target, sSetting->bChance, EElementID::Lightning );

	//chance to apply
	if ((chance > Dice::RandomI( 0, 99 )) == FALSE)
		return FALSE;

	int dmg = pcAttacker->iLatestAttackPower;

	//adjust damage based on skill's damage percentage value
	//to yield the damage damage
	dmg = ( dmg * sSetting->sPercentOfAP ) / 100;

	//Adjust damage based on monster's elemental resistance
	AdjustDamageFromMonsterResistence( EAttackProperty::Lightning, &dmg, p_Target);

	//damage successfully applied
	/// todo iron/hardcpp how we apply this to pvp ?
	if ( p_Target->IsTargetUnit() && dmg > 0 )
	{
        /// todo iron
        auto pcUnitData = p_Target->unitData;

		BOOL bAlreadyStunned = pcUnitData->iStunTimeLeft > 0;

		//attempt to set stun
		if ( SetStun( pcAttacker, sSetting->bDurationSec, p_Target ) )
		{
			//only apply damage if the unit wasn't already stunned
			if ( bAlreadyStunned == FALSE )
			{
				//stun successfully applied. apply initial lightning damage
				pcUnitData->TakeHealth( dmg );
				pcUnitData->CheckHealthStatusAfterDamageReceived( pcAttacker->pcUserData );

				//record user's damage to this unit
				CustomRecordCharacterDamage( pcAttacker->pcUserData, pcUnitData, dmg );

				MAPSERVER->SendUnitDamageInfoTextToNearbyPlayers( pcAttacker->pcUserData, pcUnitData, dmg, FALSE, 0, EDamageTextType::Stun );
			}
		}
	}

	return TRUE;
}

BOOL CDamageHandler::ApplyFrostJavelin(User* pcAttacker, EAttackProperty eAttackProperty, UxxxTarget * p_Target, ESkillID p_SkillID, int p_SkillCode)
{
	// if Skill has ended
	if (pcAttacker->uFrostJavelinEndTime == 0)
	{
		return FALSE;
	}

	if (eAttackProperty == EAttackProperty::Ice || eAttackProperty == EAttackProperty::None || eAttackProperty == EAttackProperty::Unknown)
	{
		ElementalAttackSetting sEAS;

		ZeroMemory(&sEAS, sizeof(ElementalAttackSetting));
		sEAS.bChance = pcAttacker->bFrostJavelinChance;
		sEAS.sPercentOfAP = pcAttacker->sFrostJavelinAtkPowerPercent;
		sEAS.bDurationSec = pcAttacker->bFrostJavelinDurationSec;

		sEAS.bParameter = pcAttacker->bFrostJavelinWeaponMonsterSlowdown; //slow strength in %

		return ApplyFreezing(pcAttacker, &sEAS, p_Target, p_SkillID, p_SkillCode);
	}

	return FALSE;
}


BOOL CDamageHandler::ApplyBurning( User * pcAttacker, ElementalAttackSetting * sSetting, UxxxTarget * p_Target, ESkillID p_SkillID, int p_SkillCode)
{
	//unit is chilled. don't apply burning
	//alternative is to cancel ice, but don't
	//apply burning unless casted again
	if (p_Target->IsTargetUnit() && p_Target->unitData->iIceOverlay )
		return TRUE;

	if ( sSetting->sPercentOfAP == 0 || sSetting->bChance == 0 || sSetting->bDurationSec == 0)
	{
		return FALSE;
	}

	int chance = GetResistanceAdjustedValue(p_Target, sSetting->bChance, EElementID::Fire );

	//chance to apply burning
	if ((chance > Dice::RandomI( 0, 99 )) == FALSE)
		return FALSE;

	//Burning damage is based on player's base attack power
	int dmg = pcAttacker->iLatestAttackPower;

	//Adjust damage based on monster's elemental resistance
	AdjustDamageFromMonsterResistence( EAttackProperty::Fire, &dmg, p_Target);

	//adjust damage based on skill's damage percentage value
	//to yield the final poison damage
	dmg = ( dmg * sSetting->sPercentOfAP ) / 100;

	int l_BurningCount = (sSetting->bDurationSec * 1000) / BURNING_TICKRATE_MS; //refresh duration;

	if (p_Target->IsTargetUnit() && dmg > 0 )
	{
		auto pcUnitData = p_Target->unitData;
		pcUnitData->pcUnit->bBurningCount = l_BurningCount;

		 //don't set next time if it's already set
		if (pcUnitData->pcUnit->dwBurningNextTime == 0 )
			pcUnitData->pcUnit->dwBurningNextTime = TICKCOUNT + BURNING_TICKRATE_MS;

		//remove stun effect
		pcUnitData->iStunTimeLeft = 0;

		//set highest burning damage per stack
		if (dmg > pcUnitData->pcUnit->sBurningDamagePerTick )
			pcUnitData->pcUnit->sBurningDamagePerTick = dmg;

		if ( pcUnitData->pcUnit->idBurningCastedBy == 0 )
			pcUnitData->pcUnit->idBurningCastedBy = pcAttacker->GetID();

		DEBUG( "%s: Burning dmg %d applied", p_Target->GetName(), pcUnitData->pcUnit->sBurningDamagePerTick);

		return TRUE;
	}
	else if (p_Target->IsTargetPlayer() && dmg > 0)
	{
		RefreshPvPDamageBuff(	pcAttacker->pcUserData, p_Target->userData,
								dmg, State_128_Skill, p_SkillID,
								PvPDamageBuffSlot::Burning, l_BurningCount, BURNING_TICKRATE_MS,
								-p_SkillCode, sSetting->bDurationSec);

        DEBUG("[PVP] %s: Burning dmg applied", p_Target->GetName());

        return TRUE;
	}

	//don't set to zero here as there could already be burning damage applied
	//It will get cleared out in ClearExpiredDebuffs

	return FALSE;
}


BOOL CDamageHandler::ApplyPoison( User * pcAttacker, ElementalAttackSetting * sSetting, UxxxTarget * p_Target, ESkillID p_SkillID, int p_SkillCode)
{
	if ( sSetting->sPercentOfAP == 0 || sSetting->bChance == 0 || sSetting->bDurationSec == 0)
	{
		WARN( "ApplyPoison has missing required parameters!" );
		return FALSE;
	}

	//damage is based on user's base attack power
	int dmg = pcAttacker->iLatestAttackPower;

	int numStack = sSetting->sStack & 0xFF;
	int maxStack = (sSetting->sStack >> 8) & 0xFF;

	if ( numStack == 0 || maxStack == 0)
	{
		WARN( "ApplyPoison has missing required parameters for stack!" );
		return FALSE;
	}

	int chance = GetResistanceAdjustedValue(p_Target, sSetting->bChance, EElementID::Poison );

	//chance to apply poison
	if ((chance > Dice::RandomI( 0, 99 )) == FALSE)
		return FALSE;

	//Adjust damage based on monster's elemental resistance
	AdjustDamageFromMonsterResistence( EAttackProperty::Poison, &dmg, p_Target);

	//adjust damage based on skill's damage percentage value
	//to yield the final burning damage
	dmg = ( dmg * sSetting->sPercentOfAP ) / 100;

	if (p_Target->IsTargetUnit() && dmg > 0 )
    {
        auto pcUnitData = p_Target->unitData;
		pcUnitData->pcUnit->bPoisonCount = sSetting->bDurationSec;

		//don't set next time if it's already set otherwise
		//it will never tick at all if due to high attack
		if (pcUnitData->pcUnit->dwPoisonNextTime == 0 )
			pcUnitData->pcUnit->dwPoisonNextTime = TICKCOUNT + POISON_TICKRATE_MS; //1s interval

		//remove stun effect
		pcUnitData->iStunTimeLeft = 0;

		//set highest poison damage per stack
		if (dmg > pcUnitData->pcUnit->sPoisonDamagePerStack )
			pcUnitData->pcUnit->sPoisonDamagePerStack = dmg;

		//increase damage stack if allowed
		if ( pcUnitData->pcUnit->bPoisonStackCount < maxStack )
		{
			pcUnitData->pcUnit->bPoisonStackCount += numStack;

			if ( pcUnitData->pcUnit->bPoisonStackCount > maxStack )
				pcUnitData->pcUnit->bPoisonStackCount = maxStack;
		}

		if ( pcUnitData->pcUnit->idPoisonCastedBy == 0 )
		{
			pcUnitData->pcUnit->idPoisonCastedBy = pcAttacker->GetID();
		}

		DEBUG( "%s: Poison dmg %d applied. Stack: %d", pcUnitData->GetName(), pcUnitData->pcUnit->sPoisonDamagePerStack, pcUnitData->pcUnit->bPoisonStackCount );

		return TRUE;
	}
	else if (p_Target->IsTargetPlayer() && dmg > 0)
	{
        RefreshPvPDamageBuff(	pcAttacker->pcUserData, p_Target->userData,
								dmg, State_128_Skill, p_SkillID,
								PvPDamageBuffSlot::Poison, sSetting->bDurationSec, POISON_TICKRATE_MS,
								-p_SkillCode, sSetting->bDurationSec);

        DEBUG("[PVP] %s: Poison dmg applied", p_Target->GetName());

        return TRUE;
	}
	//don't set to zero here as there could already be burning damage applied
	//It will get cleared out in ClearExpiredDebuffs

	return FALSE;
}

int CDamageHandler::GetAdjustedCriticalDamageForUnit(UxxxTarget * p_Target, int dmg)
{
	int temp;

	//Apply crit damage
	if (p_Target->IsTargetUnit() && p_Target->unitData->sAssassinEyeCritDmg )
	{
		temp = ( dmg * ( 170 + p_Target->unitData->sAssassinEyeCritDmg ) ) / 100; //Adjusted Critical Attack Weight
		DEBUG( "%s: Assassin eye's increased crit dmg (%d). Before %d, After: %d", p_Target->GetName(), p_Target->unitData->sAssassinEyeCritDmg, dmg, temp);
		dmg = temp;
	}
	else
	{
		temp = ( dmg * 170 ) / 100;	//Normal Critical Attack Weight
		DEBUG( "%s: Increased crit dmg. Before %d, After: %d", p_Target->GetName(), dmg, temp);
		dmg = temp;
	}

	return dmg;
}

/// <summary>
/// Attempt to apply critical damage. If successful adjust damage
/// and return TRUE
/// </summary>
BOOL CDamageHandler::ApplyCriticalDamage( ESkillID eSkillID, int * piDmg, int iCritChance, int iCritDamageBoost, UserData * pcUserAttacker, UxxxTarget * p_Target, EAttackState sAttackState, EAttackProperty eAttackProperty, BOOL bMultiTargetSkill )
{
	//only normal attacks have chance to cause critical
	switch (sAttackState)
	{
		case EAttackState::State_0_Normal:
		case EAttackState::State_100_Fire:
		case EAttackState::State_101_Brain:
		case EAttackState::State_103_DivineLightning:
		case EAttackState::State_104_Water:
		case EAttackState::State_105_Electricity:
		case EAttackState::State_106_Poison:
			break;

		default:
			return FALSE;
	}

	//only normal attack property has a chance to crit
	//if ( eAttackProperty != EAttackProperty::None && eAttackProperty != EAttackProperty::Undefined )
	//	return FALSE;

	if ( iCritChance == 0 || pcUserAttacker == NULL || p_Target == NULL)
		return FALSE;

	int dmg = *piDmg;

	if ( dmg <= 0 )
		return FALSE;

	//Get adjusted crit chance (0 - 100)
	int chance = GetAdjustedCriticalChance( pcUserAttacker, p_Target, iCritChance );

	//chance to apply critical
	if ( ( chance > Dice::RandomI( 0, 99 ) ) == FALSE )
		return FALSE;

	dmg = GetAdjustedCriticalDamageForUnit(p_Target, dmg);

	DEBUG( "%s: Critical damage applied: %d", p_Target->GetName(), dmg );

	*piDmg = dmg;

	return TRUE;
}

void CDamageHandler::ApplyPriorityAttacker(UxxxTarget * p_Target, UserData * pcAttacker, int iDamage, ESkillID p_SkillID, int p_SkillCode)
{
	/// todo iron/hardcpp implement this for pvp ?
	if (!p_Target->IsTargetUnit())
		return;

    /// todo iron
    auto pcUnitData = p_Target->unitData;

	User * pcUser = USERDATATOUSER( pcUnitData->pcPriorityAttacker );
	if ( pcUser == NULL || pcUser->IsValidAndInGame() == FALSE )
	{
		//priority attacker user no longer exist, remove priority attacker
		pcUnitData->pcPriorityAttacker = NULL;
	}
	else
	{
		//if the current priority attacker is more than 30 meters away, he is no longer the priority attacker
		if ( !pcUnitData->sPosition.WithinXZDistance( &pcUser->pcUserData->sPosition, DISTANCE_XY_30_meters, DISTANCE_MISC_Y_EX ) )
		{
			pcUnitData->pcPriorityAttacker = NULL;
		}
	}


	//Priority gain by recording the first attacker
	// if there is no existing 'last attacker'
	if ( pcUnitData->pcPriorityAttacker == NULL )
	{
		//Pre-attack registration
		pcUnitData->pcPriorityAttacker = pcAttacker;
		pcUnitData->iDamageLastAttacker = 0;
		pcUnitData->dwDamageLastTime = TICKCOUNT;
	}
	else
	{
		//is the same priority attacker? if so, refresh tick
		if ( pcUnitData->pcPriorityAttacker == pcAttacker )
		{
			//First attacker attack record
			pcUnitData->iDamageLastAttacker += iDamage;
			pcUnitData->dwDamageLastTime = TICKCOUNT;
		}

		//Priority attacker did not attack target for at least 15 seconds? this
		//attacker no longer has priority.
		else if ( ( TICKCOUNT - pcUnitData->dwDamageLastTime ) > 15000 )
		{
			pcUnitData->pcPriorityAttacker = NULL;
		}
	}
}

void CDamageHandler::AdjustDamageFromMonsterResistence( EAttackProperty property, int *piDmg, UxxxTarget * p_Target)
{
	if ( property == EAttackProperty::None || property == EAttackProperty::Unknown )
		return;

	int dmg = *piDmg;

	if ( dmg <= 0 )
		return;

	int res = 0;

	switch (property)
	{
		case EAttackProperty::Fire:			res = p_Target->GetCharacterData()->sElementalDef[EElementID::Fire];		break;
		case EAttackProperty::Lightning:    res = p_Target->GetCharacterData()->sElementalDef[EElementID::Lightning];	break;
		case EAttackProperty::Ice:			res = p_Target->GetCharacterData()->sElementalDef[EElementID::Ice];			break;
		case EAttackProperty::Poison:		res = p_Target->GetCharacterData()->sElementalDef[EElementID::Poison];		break;
		case EAttackProperty::Water:		res = p_Target->GetCharacterData()->sElementalDef[EElementID::Water];		break;
		case EAttackProperty::Organic:		res = p_Target->GetCharacterData()->sElementalDef[EElementID::Organic];		break;
		case EAttackProperty::Earth:		res = p_Target->GetCharacterData()->sElementalDef[EElementID::Earth];		break;
		case EAttackProperty::Wind:			res = p_Target->GetCharacterData()->sElementalDef[EElementID::Wind];		break;
	}

	//no resistence. full damage
	if ( res == 0 )
		return;

	//100% resistence. no damage
	else if ( res >= 100 )
		res = 100;

	//-ve cap
	else if ( res <= -100 )
		res = -100;


	dmg = dmg - ( ( dmg * res ) / 100 );

	*piDmg = dmg;
}

/// <summary>
/// Leaked: int RecvRangeAttack( rsPLAYINFO *pcUserData , TRANS_SKIL_ATTACKDATA *lpTransSkilAttackData , int AreaNum , int PlaySkillCode )
/// ASM address: 0x0057AFF0
/// </summary>
/// <param name="iMapID">map id</param>
/// <param name="iPlaySkillCode">especial id containing tiny skill id and params</param>
/// <param name="eSkillID">The full skill id</param>
/// <returns></returns>
BOOL CDamageHandler::MultiTargetSkillHandler ( User * pcUser, PacketSkillDamageRange * psRangeData, PacketMultiTargetSkillData * psSkillDataPacket, int iMapID, int iPlaySkillCode, ESkillID eSkillID )
{
	UserData * pcAttacker = pcUser->pcUserData;

	if ( pcAttacker == NULL )
		return FALSE;

	//Distance inspection (player and target distance)
	if ( pcAttacker->sPosition.WithinXZYDistance ( &psRangeData->sPosition, DISTANCE_XY_15_meters, 100 ) == FALSE )
		return FALSE;

	int pow;
	int totalHpStolen;
	int	dDist;
	int	Dmg1, Dmg2, Dmg3, Dmg4, Dmg5, Dmg6, Dmg7;
	int skillFlinchChance = 0, skillFlinchPushbackRange = 0;
	int	skillCode, skillLevel1Based, skillParam;

	totalHpStolen = 0;
	BOOL bCrit = FALSE;

	Map * pcMap = MAPSDATA + iMapID;

	skillCode			= ( psSkillDataPacket->iSkillID & 0xFF);
	skillParam			= ( psSkillDataPacket->iSkillID >> 12 ) & 0xF;
	skillLevel1Based	= ( psSkillDataPacket->iSkillID >> 8 )  & 0xF;

	DEBUG ( FormatString ( "MultiTargetSkillHandler skillCode = %d, skillParam = %d, skillLevel1Based = %d", skillCode, skillParam, skillLevel1Based ) );

	dDist = psRangeData->iRange + 40; //40 = extra leeway due to latency
	dDist *= dDist;

	//special case
	if ( psSkillDataPacket->iSkillID != TINYSKILLID_ENCHANT_WEAPON_LIGHTING )
	{
		if ( ( skillLevel1Based >= 1 && skillLevel1Based <= 10 ) == FALSE )
		{
			WARN( "Skill level (%d) is not within 1-10", skillLevel1Based );
			return FALSE;
		}
	}

	if ( iMapID < 0 || iMapID >= NUM_MAPS )
	{
		WARN( "Map ID (%d) is invalid", iMapID );
		return FALSE;
	}

	if ( psSkillDataPacket->iTargetCount < 0 || psSkillDataPacket->iTargetCount > 128 )
	{
		WARN ( "Target count (%d) is invalid", psSkillDataPacket->iTargetCount );
		return FALSE;
	}

	auto skillInfo = GetSkillInfo( eSkillID );
	if ( skillInfo )
	{
		skillFlinchChance = skillInfo->iFlinchChance;
		skillFlinchPushbackRange = skillInfo->iPushbackRange;
	}

	DEBUG ( "Skill flinch chance = %d, flinch Pushback = %d", skillFlinchChance, skillFlinchPushbackRange );

	if ( psRangeData->eAttackState == EAttackState::State_0_Normal ||
		 psRangeData->eAttackState == EAttackState::State_1_Knockback ||
		 psRangeData->eAttackState == EAttackState::State_2_Stun ||
		 psRangeData->eAttackState == EAttackState::State_4_LifeSteal ||
		 psRangeData->eAttackState == EAttackState::State_6_Extinction ||
		 psRangeData->eAttackState == EAttackState::State_7_Distortion||
		 psRangeData->eAttackState == EAttackState::State_8_Cursed ||
		 psRangeData->eAttackState == EAttackState::State_100_Fire ||
		 psRangeData->eAttackState == EAttackState::State_101_Brain ||
		 psRangeData->eAttackState == EAttackState::State_103_DivineLightning ||
		 psRangeData->eAttackState == EAttackState::State_104_Water ||
		 psRangeData->eAttackState == EAttackState::State_105_Electricity ||
		 psRangeData->eAttackState == EAttackState::State_106_Poison ||
		 psRangeData->eAttackState == EAttackState::State_201_PullIn )
    {
        for (int cnt = 0; cnt < psSkillDataPacket->iTargetCount; cnt++)
		{
			bCrit = FALSE;

			auto l_UnitData = UNITSERVER->GetUnit(psSkillDataPacket->iaTargetID[cnt]);
			UxxxTarget l_Target(l_UnitData == nullptr ? USERDATABYID(psSkillDataPacket->iaTargetID[cnt]) : nullptr, l_UnitData);

			//Monster is valid and is alive? or is a player
			if (
				(l_Target.IsTargetUnit() && l_Target.IsUnitActive() && l_Target.GetCharacterType() && l_Target.GetUnitAnimation() != ANIMATIONTYPE_Die)
				|| (l_Target.IsTargetPlayer() && (l_Target.userData->iMapID == MAPID_BlessCastle || l_Target.userData->iMapID == MAPID_BattleTown))
				)
			{
				if (l_Target.GetID() != psSkillDataPacket->iaTargetID[cnt] )
					continue;

				///Set first target as the target character
				if (l_Target.IsTargetUnit() && !pcAttacker->pcLastAttack )
					pcAttacker->pcLastAttack = l_Target.unitData;

				if (l_Target.IsUnitPetOf(pcAttacker))
					continue;	//Your summons mob receives no damage

				//Distance inspection. i.e. is monster in range of the skill?
				if (l_Target.GetPosition().WithinXZYDistance( &psRangeData->sPosition, dDist, 100 ) == FALSE )
					continue;

				//get skill attack power (already adjusted)
				pow = psRangeData->iDamage;


				Dmg1 = pow;

				//Apply damage buff based on monster type and skill
				ApplyMonsterTypeDamageBuffFromSkill( &pow, &l_Target, pcUser, skillLevel1Based, eSkillID );

				Dmg2 = pow;

				//Adjust damage based on monster's resistence
				AdjustDamageFromMonsterResistence( psRangeData->iAttackProperty, &pow , &l_Target);

				Dmg3 = pow;

				//Check and apply critical hit
				bCrit = ApplyCriticalDamage( eSkillID, &pow, psSkillDataPacket->sCriticalChance, psSkillDataPacket->sCriticalDmgBoost, pcAttacker, &l_Target, psRangeData->eAttackState, psRangeData->iAttackProperty, TRUE );

				if ( bCrit && l_Target.IsTargetUnit() )
				{
					//this sends a packet to the client to set AttackCritcal to 1. This in turn ages the item via critical. Some items only a get aged via critical!
					//See int sinCheckAgingLevel(int Kind, bool bBillingUse)
					SENDPACKETCOMMAND( pcUser, PKTHDR_AttackResult, 0, 1 );
				}

				Dmg4 = pow;

				//Adjust final damage based on monster's absorb rating
				AdjustDamageFromAbsorb( &pow, &l_Target);

				Dmg5 = pow;

				//Check and apply extinction
				pow += ApplyExtinction( psRangeData->eAttackState, &psSkillDataPacket->sElementalAttackSetting, &l_Target, eSkillID, skillCode );

				//Check and apply life steal
				const int hpStolen = ApplyLifeSteal ( psRangeData->eAttackState, psSkillDataPacket->sLifeStealParam, &l_Target, eSkillID, skillCode);

				if ( hpStolen > 0 )
				{
					totalHpStolen += hpStolen;
					pow += hpStolen;
				}

				Dmg6 = pow;

				//Attempt to apply stun from skill
				ApplyStun( pcUser, psRangeData->eAttackState, &psSkillDataPacket->sElementalAttackSetting, &l_Target, eSkillID, skillCode);

				//Check and apply burning damage from fire skills
				if ( psRangeData->iAttackProperty == EAttackProperty::Fire ||
						psSkillDataPacket->eSecondaryAttackProperty == EAttackProperty::Fire )
				{
					ApplyBurning( pcUser, &psSkillDataPacket->sElementalAttackSetting, &l_Target, eSkillID, skillCode);
				}

				//Check and apply poison
				if ( psRangeData->iAttackProperty == EAttackProperty::Poison ||
					psSkillDataPacket->eSecondaryAttackProperty == EAttackProperty::Poison )
				{
					ApplyPoison( pcUser, &psSkillDataPacket->sElementalAttackSetting, &l_Target, eSkillID, skillCode);
				}

				//Check and apply freezing fom ice skills
				if ( psRangeData->iAttackProperty == EAttackProperty::Ice ||
					psSkillDataPacket->eSecondaryAttackProperty == EAttackProperty::Ice )
				{
					ApplyFreezing( pcUser, &psSkillDataPacket->sElementalAttackSetting, &l_Target, eSkillID, skillCode);
				}

				//Check and apply curse
				if ( psRangeData->eAttackState == EAttackState::State_8_Cursed ||
					psSkillDataPacket->eSecondaryAttackState == EAttackState::State_8_Cursed )
				{
					ApplyCurse( pcUser, &psSkillDataPacket->sElementalAttackSetting, &l_Target, eSkillID, skillCode);
				}

				//check and apply distortion
				if ( psRangeData->eAttackState == EAttackState::State_7_Distortion ||
						psSkillDataPacket->eSecondaryAttackState == EAttackState::State_7_Distortion )
				{
					ApplyDistortion( pcUser, &psSkillDataPacket->sElementalAttackSetting, &l_Target, eSkillID, skillCode);
				}

				//Check and apply lightning stun
				ApplyLightningStun( pcUser, psRangeData->iAttackProperty, &psSkillDataPacket->sElementalAttackSetting, &l_Target, eSkillID, skillCode);

				ApplyFrostJavelin( pcUser, psRangeData->iAttackProperty, &l_Target, eSkillID, skillCode);

				//Check and apply enchant weapon - lightning AoE only
				ApplyEnchantWeapon(pcUser, psRangeData->iAttackProperty, &l_Target, TRUE, eSkillID, skillCode);

				//Adjust elemental damage based on monster resistence
				AdjustDamageFromMonsterResistence( psRangeData->iAttackProperty, &pow, &l_Target);

				//Final damge
				Dmg7 = pow;

				DEBUG( "%s Dmg (%d %d %d %d %d %d %d)", l_Target.GetCharacterData()->szName, Dmg1, Dmg2, Dmg3, Dmg4, Dmg5, Dmg6, Dmg7 );


				//attack is successful?
				if ( pow > 0 && l_Target.IsTargetUnit() )
				{
					ApplyPriorityAttacker(&l_Target, pcAttacker, cnt, eSkillID, skillCode );

					//take off hp (units only)
					l_Target.unitData->TakeHealth( pow );
					l_Target.unitData->CheckHealthStatusAfterDamageReceived( pcAttacker );

					if ( psRangeData->eAttackState != EAttackState::State_2_Stun )
					{
						//remove stun
						l_Target.unitData->iStunTimeLeft = 0;
					}

					//record user's damage to this unit
					CustomRecordCharacterDamage ( pcAttacker, l_Target.unitData, pow );

					//Apply general flinch and pushback for other skills where needed
					//the flinch and pushback values are from the database
					ApplySkillFlinchAndPushback( pcAttacker, bCrit, skillFlinchChance, skillFlinchPushbackRange, &l_Target, &psRangeData->sPosition, psRangeData->eAttackState, eSkillID, skillCode);

					//Check and apply pike wind
					ApplyPikeWind (pcUser, psRangeData, &l_Target, eSkillID, skillCode);

					//Apply pullback and target hook
					ApplyPullBackAndTargetHook( eSkillID, psSkillDataPacket->iaParametersEx[0], &l_Target, pcAttacker, &psRangeData->sPosition );

					//Give hp based on damage for beserker. halved for AoE
					ConvertDamageToHPFromBeserker( pcUser, pow, TRUE );

					//heal hp via holy body buff. halved for AoE
					ApplyPartyHealFromHolyBody( pcUser, pow, TRUE );



					WRITEDWORD ( ESkillsVarsASM::ATTACK_DAMAGE, pow ); //Iron - no idea what this is used for..

					MAPSERVER->SendUnitDamageInfoTextToNearbyPlayers( pcAttacker, l_Target.unitData, pow, bCrit );
				}

				//pvp mode
				else if (l_Target.IsTargetPlayer())
				{
					//Give hp based on damage for beserker. halved for AoE
					ConvertDamageToHPFromBeserker( pcUser, static_cast<int>(round(static_cast<float>(pow) / fGlobalDamageReduction)), TRUE );

					pow = Dmg4; //use the damage value BEFORE player absorb is applied
					AdjustDamageFromMonsterResistence( psRangeData->iAttackProperty, &pow, &l_Target);
					SendDamagePvPData( pcAttacker, l_Target.userData, pow, psSkillDataPacket->sAttackRating, EAttackState::State_128_Skill, eSkillID, bCrit );
				}
			}
		}

		if ( totalHpStolen > 0 )
		{
			//Give stolen hp if any
			GrantStolenHealthToUser( pcUser, totalHpStolen );
		}

		return TRUE;
	}
	else
	{
		//Other attack states are handled in ASM
		// for now.

		return DamageRangeHandler ( pcAttacker, psRangeData, iMapID, skillCode );
	}
}

/// <summary>
/// Get Adjusted critical chance based on skill
/// ASM: 0x00577E60
/// Leaked: int GetAdjustedCriticalChance( rsPLAYINFO *pcUserData , smCHAR *pcUnitData , int Critical_Hit )
/// </summary>
int CDamageHandler::GetAdjustedCriticalChance ( UserData * pcUserAttacker, UxxxTarget * p_Target, int Critical_Hit )
{
	if ( Critical_Hit == 0 )
		return 0;

	int result;

	//Critical Chance + 25% of (level difference of player and unit)
	//					i.e. 50 level difference will give extra 12% crit chance
	result = Critical_Hit + ( ( ( pcUserAttacker->sCharacterData.iLevel - p_Target->GetCharacterData()->iLevel ) * 25 ) / 100 );

	if (p_Target->IsTargetUnit() && p_Target->unitData->sAssassinEyeCritChance > 0 )
	{
		DEBUG( "%s: Add Assassin eye crit chance (%d). Before %d%%, After: %d%%", p_Target->GetName(), p_Target->unitData->sAssassinEyeCritChance, result, (p_Target->unitData->sAssassinEyeCritChance + result) );
		result += p_Target->unitData->sAssassinEyeCritChance;
	}

	//max chance is 100%
	if ( result > 100 )
	{
		result = 100;
	}

	DEBUG( "Critical chance: Original: %d, Final: %d", Critical_Hit, result );

	return result;
}

/// <summary>
/// Handles all single target attacks
/// PKTHDR_SingleTargetSkillData / smTRANSCODE_ATTACK_DAMAGE
/// Leaked: int rsRecvAttackData ( rsPLAYINFO * pcUserData, TRANS_ATTACKDATA2 * lpTransAttackData )
/// ASM: 0x57D910
/// </summary>
BOOL CDamageHandler::HandlePacket(User * pcUser, PacketSingleTargetSkillData* psSkillsDataPacket )
{
	DEBUG( "" );
	DEBUG( "CDamageHandler::HandlePacket --> PacketSingleTargetSkillData (Single target)" );

	PacketAttackDataEx TransAttackData;
	UserData * pcUserData = pcUser->pcUserData;
	PacketTransCommandEx smTransCommand;
	short sDefAttack[2];
	int	pow;
	DWORD dwCode;


	DecodePacket ( pcUserData, psSkillsDataPacket );

	if ( psSkillsDataPacket->dwChkSum != GetDamageChecksum ( psSkillsDataPacket ) )
	{
		WARN( "AttackData - Received checksum (%d) vs calculated checksum mismatch (%d)", psSkillsDataPacket->dwChkSum, GetDamageChecksum( psSkillsDataPacket ) );

		PacketLogCheat sPacket;
		sPacket.iCheatID = 8500;
		sPacket.LParam = 1;
		LOGSERVER->OnLogCheat ( pcUserData, &sPacket, TRUE, TRUE );

		return FALSE;
	}


	psSkillsDataPacket->iHeader = 0; // to make sure it doesn't get processed in asm


	dwCode = psSkillsDataPacket->iSkillCode & 0xFF;

	ESkillID  eSkillID = GetSkillIDFromTinyID ( pcUserData->sCharacterData.iClass, static_cast<BYTE>(dwCode) );


	if ( eSkillID != ESkillID::SKILLID_Falcon &&
		 eSkillID != ESkillID::SKILLID_MagneticSphere &&
		 eSkillID != ESkillID::SKILLID_ChargingStrike &&
		 eSkillID != ESkillID::SKILLID_ShadowMaster &&
		 eSkillID != ESkillID::SKILLID_DivinePiercing )
	{
		//Falcon, excluded due to possible reordering
		if ( psSkillsDataPacket->AttackCount <= pcUserData->DamagePacketCount ||
			 psSkillsDataPacket->dwTime <= pcUserData->dwDamagePacketTime )
		{
			//Damage client side counting, time error
			return FALSE;
		}

		pcUserData->DamagePacketCount = psSkillsDataPacket->AttackCount;
		pcUserData->dwDamagePacketTime = psSkillsDataPacket->dwTime;
	}

	//Disabling this code as it seems to be reducing damage if you have a force orb active.
	//the sDamageLimit sMin and sMax are incorrect
	//if ( psSkillsDataPacket->AttackPowerMin > pcUserData->sDamageLimit.sMin ||
	//	 psSkillsDataPacket->AttackPowerMax > pcUserData->sDamageLimit.sMax ||
	//	 psSkillsDataPacket->sCriticalChance > 50 )
	//{
	//	PacketLogCheat sPacket;
	//	sPacket.iCheatID = 8510;
	//	sPacket.WxParam = 1;
	//	sPacket.LParam = psSkillsDataPacket->AttackPowerMin;
	//	sPacket.SParam = psSkillsDataPacket->AttackPowerMax;
	//	sPacket.EParam = psSkillsDataPacket->sCriticalChance;
	//	sPacket.LxParam = pcUserData->sDamageLimit.sMin;
	//	sPacket.SxParam = pcUserData->sDamageLimit.sMax;
	//	sPacket.ExParam = pcUserData->sCriticalLimit[0];
	//	LOGSERVER->OnLogCheat ( pcUserData, &sPacket, TRUE, FALSE );
	//
	//	psSkillsDataPacket->AttackPowerMin = pcUserData->sDamageLimit.sMin;
	//	psSkillsDataPacket->AttackPowerMax = pcUserData->sDamageLimit.sMax;
	//	psSkillsDataPacket->sCriticalChance = pcUserData->sCriticalLimit[0];
	//}



	pcUser->iClientAttackPowerMin = psSkillsDataPacket->iaParametersEx[0];
	pcUser->iClientAttackPowerMax = psSkillsDataPacket->iaParametersEx[1];

	psSkillsDataPacket->iaParametersEx[0] = 0;
	psSkillsDataPacket->iaParametersEx[1] = 0;

	TransAttackData.sPosition.iX = psSkillsDataPacket->sPosition.iX;
	TransAttackData.sPosition.iY = psSkillsDataPacket->sPosition.iY;
	TransAttackData.sPosition.iZ = psSkillsDataPacket->sPosition.iZ;

	TransAttackData.iRange = psSkillsDataPacket->iRange;
	TransAttackData.dwTarObjectSerial = psSkillsDataPacket->dwTarObjectSerial;
	TransAttackData.dwDestObjectSerial = psSkillsDataPacket->dwDestObjectSerial;

	sDefAttack[0] = psSkillsDataPacket->AttackPowerMin;
	sDefAttack[1] = psSkillsDataPacket->AttackPowerMax;

	pcUser->iLatestAttackPower = Dice::RandomI ( sDefAttack[0], sDefAttack[1] );


	DEBUG( "Latest attack power: %d", pcUser->iLatestAttackPower );

	TransAttackData.iDamage = pcUser->iLatestAttackPower;
	TransAttackData.iQuestData = psSkillsDataPacket->iQuestData;
	TransAttackData.eAttackState = psSkillsDataPacket->eAttackState;
	TransAttackData.iAttackProperty = psSkillsDataPacket->iAttackProperty;

	TransAttackData.sRating[0] = pcUserData->sCharacterData.iLevel;
	TransAttackData.sRating[1] = pcUserData->sCharacterData.iAttackRating;

	UnitData * l_UnitTarget = nullptr;
	if ( psSkillsDataPacket->StageId >= 0 )
		l_UnitTarget = UNITSERVER->UnitDataByIDMap ( psSkillsDataPacket->dwTarObjectSerial, psSkillsDataPacket->StageId );
	else
		l_UnitTarget = UNITSERVER->GetUnit ( psSkillsDataPacket->dwTarObjectSerial );

    UxxxTarget l_Target(l_UnitTarget == nullptr ? USERDATABYID(psSkillsDataPacket->dwTarObjectSerial) : nullptr, l_UnitTarget);

	PacketSingleTargetSkillDataEx psSkillsDataPacketEx;
	ZeroMemory( &psSkillsDataPacketEx, sizeof( PacketSingleTargetSkillDataEx ) );
	CopyMemory( &psSkillsDataPacketEx, psSkillsDataPacket, sizeof( PacketSingleTargetSkillData ) );

	//Adjust skill based on database values
	if ( RecvSkillSingleTarget ( pcUserData, &l_Target, &TransAttackData.iDamage, &psSkillsDataPacketEx ) == FALSE )
		return FALSE;

	pow = TransAttackData.iDamage;

	if ( l_Target.IsTargetUnit() && SetSkillPlus ( pcUserData, l_Target.unitData, &TransAttackData, psSkillsDataPacket ) == FALSE )
		return FALSE;

	if ( !psSkillsDataPacket->iSkillCode )
	{
		//General attack
		if ( READDWORD(EEventASM::EVENT_CHILD_VAR) )
		{
			//Children's Event (Attack weighted when Ulken is a character)
			if ( pcUserData->sCharacterData.iShadowSize > 0x1000 && pcUserData->sCharacterData.iShadowSize <= 0x1002 )
			{
				pow = ( TransAttackData.iDamage * 15 ) / 100;	//Damage increased by 15%

				DEBUG ( "DAMAGE: Children's Event active Damage. %d -> %d", pow, TransAttackData.iDamage, TransAttackData.iDamage - pow );

				TransAttackData.iDamage += pow;
			}
		}
	}

	//Attacker is cursed, therefore the damage is reduced
	//@todo - move this elsewhere
	if ( pcUserData->dwCurse_AttackParam )
	{
		pow = ( TransAttackData.iDamage * pcUserData->dwCurse_AttackParam ) / 100;
		DEBUG ( "DAMAGE: Player is cursed. Reduced damage (-%d). %d -> %d", pow, TransAttackData.iDamage, TransAttackData.iDamage - pow );
		TransAttackData.iDamage -= pow;
	}


	if (l_Target.IsValid())
	{
		//only enable for BC for now.
		if ( l_Target.IsTargetPlayer() )
		{
			if ( l_Target.userData->iMapID != MAPID_BlessCastle && l_Target.userData->iMapID != MAPID_BattleTown )
			{
				return FALSE;
			}
		}

		TransAttackData.iAttackProperty = psSkillsDataPacket->iAttackProperty;
		TransAttackData.eAttackState = psSkillsDataPacket->eAttackState;
		TransAttackData.iRange = psSkillsDataPacket->iRange;

		SingleTargetSkillDamageHandler ( pcUser, &TransAttackData, &psSkillsDataPacketEx, &l_Target, TRUE, eSkillID );

		/// todo iron
		if ( l_Target.IsTargetUnit() &&
			 l_Target.unitData->dwIgnoreOthersTime == 0 && (
			 l_Target.GetMonsterEffectID() == MONSTEREFFECTID_DEATHKNIGHT ||
			 l_Target.GetMonsterEffectID() == MONSTEREFFECTID_BABEL ||
			 l_Target.GetMonsterEffectID() == MONSTEREFFECTID_CASTLE_CRYSTAL_R ||
			 l_Target.GetMonsterEffectID() == MONSTEREFFECTID_CASTLE_CRYSTAL_G ||
			 l_Target.GetMonsterEffectID() == MONSTEREFFECTID_CASTLE_CRYSTAL_B ) )
		{
			//Change target to user who attacked Babel or Crystal ranged
			int dist, x, y, z;

            x = (pcUserData->sPosition.iX - l_Target.GetPosition().iX) >> FLOATNS;
            y = (pcUserData->sPosition.iY - l_Target.GetPosition().iY) >> FLOATNS;
            z = (pcUserData->sPosition.iZ - l_Target.GetPosition().iZ) >> FLOATNS;
			dist = x * x + y * y + z * z;

			if ( dist <= (l_Target.GetCharacterData()->iAttackRange * l_Target.GetCharacterData()->iAttackRange ) )
			{
				l_Target.unitData->pcFocusTarget = pcUserData;
				l_Target.unitData->dwIgnoreOthersTime = TICKCOUNT + 4000;
			}
		}
	}

	if ( pcUserData->iGameLevel > 2 )
	{
		SENDPACKETCOMMAND ( pcUser, PKTHDR_LastAttackDamage, 0, TransAttackData.iDamage );
	}

	return TRUE;
}

/**
Svr_Damage.cpp
int rsRecvRangeAttackData( rsPLAYINFO *pcUserData , TRANS_SKIL_ATTACKDATA2 *lpTransRangeAttackData )
*/
BOOL CDamageHandler::HandlePacket( User * pcUser, PacketMultiTargetSkillData * psSkillsDataPacket )
{
	DEBUG( "CDamageHandler::HandlePacket --> PacketMultiTargetSkillData" );

	if ( pcUser->IsValid () == FALSE )
		return FALSE;

	UserData * pcUserData = pcUser->pcUserData;

	DEBUG ( "CDamageHandler::HandlePacket" );
	DEBUG ( "psSkillDataPacket->iSkillID = %d", psSkillsDataPacket->iSkillID );
	DEBUG ( "psSkillDataPacket->iSkillID & 0xFF = %d", psSkillsDataPacket->iSkillID & 0xFF );


	PacketSkillDamageRange	sPacketRange;

	ZeroMemory( &sPacketRange, sizeof( PacketSkillDamageRange ) );

	int iSkillID			= 0;

	UnitData * pcUnitData	= NULL;

	//Damage Timer
	pcUserData->DamagePacketCount = psSkillsDataPacket->iDamageCount;
	pcUserData->dwAttackTime = psSkillsDataPacket->dwTime;

	//store final client attack power (min - max)
	//for comparing vs server attack power (and throw warning if mismatch)
	pcUser->iClientAttackPowerMin = psSkillsDataPacket->iaParametersEx[0];
	pcUser->iClientAttackPowerMax = psSkillsDataPacket->iaParametersEx[1];

	psSkillsDataPacket->iaParametersEx[0] = 0;
	psSkillsDataPacket->iaParametersEx[1] = 0;


	pcUser->iLatestAttackPower = Dice::RandomI ( psSkillsDataPacket->sAttackPower.sMin, psSkillsDataPacket->sAttackPower.sMax );

	DEBUG( "Latest attack power: %d", pcUser->iLatestAttackPower );

	//User Dead? Not Handle Damage...
	if ( pcUserData->IsDead() )
		return FALSE;

	//Have Single Target?
	if ( psSkillsDataPacket->iTargetID )
	{
		//WARN( "Skill has single target set but it is no longer processed: %d", psSkillsDataPacket->iSkillID );
		psSkillsDataPacket->iTargetID = 0;
	}

	//Max range attacks
	sPacketRange.iRange = 200;
	sPacketRange.iTargetReceiveDamageID = psSkillsDataPacket->iTargetReceiveDamageID;
	sPacketRange.sPosition = psSkillsDataPacket->sPosition;

	//Check For Overflow
	if ( psSkillsDataPacket->iTargetCount < 0 )
		psSkillsDataPacket->iTargetCount = 0;
	else if ( psSkillsDataPacket->iTargetCount > 128 )
		psSkillsDataPacket->iTargetCount = 128;

	//Copy Data
	CopyMemory( sPacketRange.iaTargetID, psSkillsDataPacket->iaTargetID, psSkillsDataPacket->iTargetCount * sizeof( int ) );
	sPacketRange.iCount = psSkillsDataPacket->iTargetCount;

	int iSkillCode = psSkillsDataPacket->iSkillID & 0xFF;
	ESkillID eSkillID;

	if ( iSkillCode == TINYSKILLID_ENCHANT_WEAPON_LIGHTING )
	{
		if ( pcUser->eEnchantWeaponElement == EElementID::Lightning )
		{
			eSkillID = ESkillID::SKILLID_EnchantWeaponLightningAoE;
			sPacketRange.iDamage = 0;
		}
		else
		{
			WARN( "Unknown state for iSkillCode: %d, element: %d", iSkillCode, pcUser->eEnchantWeaponElement );
			return FALSE;
		}
	}
	else
	{
		eSkillID = GetSkillIDFromTinyID ( pcUserData->sCharacterData.iClass, (BYTE)iSkillCode );

		if ( eSkillID == ESkillID::SKILLID_DefaultAttack )
		{
			CHATSERVER->SendGMError ( pcUserData, "eSkillID is not set for tiny id: %d", iSkillCode );
			return FALSE;
		}

		//This calls the following function in HNSSkill.cpp
		//int __cdecl RecvSkillAreaTarget( UserData * pcUser, UnitData * pcUnitData, PacketAttackData * psSkillDataPacket, PacketSkillDamageRange * psPacketRange, PacketMultiTargetSkillData * psPacketRangeEx )
		//Note - if it's an area skill then iCount will get set to 256
		if ( RecvSkillAreaTarget( pcUserData, pcUnitData, &sPacketRange, psSkillsDataPacket ) == FALSE )
			return FALSE;
	}

	//Debug
	CHATSERVER->SendDebugChat( pcUser, EChatColor::CHATCOLOR_Error, "> Range Damage: Count[%d]", psSkillsDataPacket->iTargetCount );

	//Have Targets?
	if ( psSkillsDataPacket->iTargetCount > 0 )
	{
		//Handle Damage to Targets
		CDamageHandler::MultiTargetSkillHandler( pcUser, &sPacketRange, psSkillsDataPacket, (int)psSkillsDataPacket->sMapID, psSkillsDataPacket->iSkillID, eSkillID );
	}

	return TRUE;
}

BOOL CDamageHandler::UpdateBCPVPScoring( UserData * pcKiller, UserData * pcVictim, int iDamage )
{
	//JLM
	// NOTE - this area of code is ONLY related
	// to Blessed Castle SCORING
	// the Damage reduction here (/10)
	// is so that the BC score doesn't get out of control

	AttackDamageData * psaDamageData = NULL;

	BlessCastleStatusData * psBCStatus = BLESSCASTLESERVER->psBlessCastleStatusData;

	//attacker / victim both in same clan? If so, don't apply damage to score.

	//only check if BC is started
	if ( BLESSCASTLEHANDLER->GetBlessCastleStatus () )
	{
		if ( ( pcKiller->iClanID == 0 ) || ( pcKiller->iClanID == pcVictim->iClanID ) )
		{
			if ( pcKiller->iClanID != psBCStatus->iClanID )
			{
				return FALSE;
			}
		}
	}

	//Door is still alive?
	if ( BLESSCASTLEHANDLER->GetCastleDoorStatus () )
		return FALSE;

	//Only if tower is alive
	if ( psBCStatus->pcUnitDataTower )
	{
		if ( psBCStatus->pcUnitDataTower->bActive &&
			(psBCStatus->pcUnitDataTower->sCharacterData.iMonsterEffectID == MONSTEREFFECTID_CASTLE_TOWER_B) &&
			(psBCStatus->pcUnitDataTower->GetCurrentHealth() > 0) )
		{
			psaDamageData = psBCStatus->psaDamageData;
		}
	}

	if ( psaDamageData == NULL )
		return FALSE;

	//std::cout << "POINTS: iDamage Original: " << iDamage << "\n";

	//subtract dmg from char abs
	iDamage -= pcVictim->sCharacterData.iAbsorbRating;

	//std::cout << "POINTS: iDamage (post abs): " << iDamage << "\n";

	iDamage /= 10;

	//std::cout << "POINTS: iDamage (post div 10): " << iDamage << "\n";

	//Area Damage Reduction (score only)
	if ( IsAreaDamage() )
		iDamage /= 5;

	if ( iDamage < 1 )
		iDamage = 1;

	pcKiller->sBlessCastle_Damage += iDamage;

	if ( pcKiller->iClanID && pcKiller->iMapID == MAPID_BlessCastle )
	{
		BOOL bSuccess = FALSE;

		int iSlot = -1;

		for ( int i = 0; i < ATTACK_DAMAGE_MAX; i++ )
		{
			if ( psaDamageData[i].iID )
			{
				int iClanID = BLESSCASTLESERVER->GetClanCodeByID( psaDamageData[i].iID );

				if ( iClanID == pcKiller->iClanID )
				{
					psaDamageData[i].iDamage += iDamage;
					psaDamageData[i].iCounter++;
					bSuccess = TRUE;
					break;
				}
			}

			if ( (iSlot < 0) && (psaDamageData[i].iID == 0) )
				iSlot = i;
		}

		if ( (bSuccess == FALSE) && (iSlot >= 0) )
		{
			psaDamageData[iSlot].iID = pcKiller->sCharacterData.iClanID;
			psaDamageData[iSlot].iDamage = iDamage;
			psaDamageData[iSlot].iCounter++;
		}
	}

	return TRUE;
}


BOOL CDamageHandler::IsMonsterAreaDamage( UnitData * pcUnitData )
{
	if ( pcUnitData->sCharacterData.iMonsterEffectID == MONSTEREFFECTID_Faugn )
	{
		if ( pcUnitData->sPosition.WithinXZDistance( &pcUnitData->pcFocusTarget->sPosition, DISTANCE_XY_41_meters, DISTANCE_MISC_Y_EX) )
		{
			CDamageHandler::SendDamageBySocket( pcUnitData, NULL, pcUnitData->pcFocusTarget->pcSocketData );
		}
	}

	return FALSE;
}

BOOL CDamageHandler::SendDamagePvPData(UserData * p_Attacker, UserData * p_Target, int p_Damage, int p_AttackRating, EAttackState p_State, ESkillID p_SkillID, BOOL p_Critical, int16_t p_Param1, int16_t p_Param2)
{
	const BOOL debugPvP = TRUE;

	User * pcAttacker = USERDATATOUSER( p_Attacker );
	User * pcTarget = USERDATATOUSER( p_Target );

	//both attackers and target should be valid
	if ( pcAttacker == NULL || pcAttacker->IsValidAndInGame() == FALSE ||
		pcTarget == NULL || pcTarget->IsValidAndInGame() == FALSE )
	{
		return FALSE;
	}

	if ( pcTarget->GetMapID() == EMapID::MAPID_BlessCastle &&
		pcTarget->pcUserData->sPosition.iZ > CastleBattleZoneDamage_LineZ)
	{
		DEBUG( "Ignoring attack. Target is in safe zone" );
		return FALSE;
	}

	if ( pcAttacker->GetMapID() == EMapID::MAPID_BlessCastle &&
		pcAttacker->pcUserData->sPosition.iZ > CastleBattleZoneAttacker_LineZ)
	{
		DEBUG( "Ignoring attack. Attacker is in safe zone" );
		return FALSE;
	}

	/// Blessed Castle is running and the attacker and the target are both in the same clan?
	auto l_BCStatus = BLESSCASTLESERVER->psBlessCastleStatusData;
	auto l_BCRunning = l_BCStatus && l_BCStatus->iCastleMode >= 1;
	if (   l_BCRunning
		&& p_Attacker->iClanID != 0 && p_Target->iClanID != 0
		&& p_Attacker->iClanID == p_Target->iClanID)
	{
		return FALSE;
	}

	const int attackerLevel = pcAttacker->pcUserData->GetLevel();
	const int targetLevel = pcTarget->pcUserData->GetLevel();

	const int attackerAbs = pcAttacker->pcUserData->sCharacterData.iAbsorbRating;
	const int targetAbs = pcTarget->pcUserData->sCharacterData.iAbsorbRating;

	const int levelDiff = 100 - attackerLevel;

	DEBUGIF(debugPvP, "PVP: Level difference to 100: %d", levelDiff );

	float fScale = 1.0;

	//increase scale more for lower level
	// as they are significantly weaker
	// NOTE - level diff > 0 MEANS lower level (+ve)
	if		(levelDiff > 20 )	fScale = 1.20f;
	else if (levelDiff > 15 )	fScale = 1.15f;
	else if (levelDiff > 10 )	fScale = 1.10f;
	else if (levelDiff > 5 )	fScale = 1.05f;
	else if (levelDiff > 0 )	fScale = 1.00f;
	else if (levelDiff > -5 )	fScale = 0.95f;
	else if (levelDiff > -10 )	fScale = 0.90f;


	DEBUGIF(debugPvP, "PVP: Level difference scaling: %.0f", fScale );

	const float fLevelDiff = static_cast<float>(levelDiff);

	float dmgScale = 1.0f + ((fLevelDiff * fLevelBasedPvpDamageScale * fScale) / 100.0f);
	float absScale = 1.0f + ((fLevelDiff * fLevelBasedPvpAbsScale * fScale) /  100.0f);

	DEBUGIF(debugPvP, "PVP: Level based damage scaling: %.2f", dmgScale );
	DEBUGIF(debugPvP, "PVP: Level based absorb scaling: %.2f", absScale );

	float fVictimAbs = targetAbs / absScale;

	DEBUGIF(debugPvP, "PVP: Absorb original: %d", targetAbs );
	DEBUGIF(debugPvP, "PVP: Absorb post level based abs reduction (%.2f%%): %.0f", absScale, fVictimAbs );

	fVictimAbs /= fGlobalAbsReduction;

	DEBUGIF(debugPvP, "PVP: Absorb post global abs reduction (%.2f%%): %.0f", fGlobalAbsReduction, fVictimAbs );

	double damage = static_cast<double>(p_Damage);

	DEBUGIF(debugPvP, "PVP: Damage original: %.0f", damage );

	//scale damage based on level diff
	damage *= dmgScale;

	DEBUGIF(debugPvP, "PVP: Damage post level scaling (%.2f%%): %.0f", dmgScale, damage );

	/// apply flat PVP scaling from skill info
	auto l_SKillInfo	= GetSkillInfo(p_SkillID);
	if ( l_SKillInfo )
	{
		damage *= l_SKillInfo->pvpScaling;

		DEBUG( "PVP: Damage post skill scaling (%.2f%%): %.0f", l_SKillInfo->pvpScaling, damage );
	}

	//adjust damage from victim abs
	damage -= fVictimAbs;

	DEBUGIF(debugPvP, "PVP: Damage post target abs (%.2f): %.0f", fVictimAbs, damage );

	//General damage reduction
	damage /= fGlobalDamageReduction;

	DEBUGIF(debugPvP, "PVP: Damage post global reduction (%.2f): %.0f", fGlobalDamageReduction, damage );

	if ( damage < 5 )
	{
		damage = Dice::RandomI( 1, 5 );
	}

	return SendDamagePvPRaw(pcAttacker, pcTarget, static_cast<int>(damage), p_AttackRating, EAttackState::State_128_Skill, p_SkillID, p_Critical, p_Param1, p_Param2);
}

/// <summary>
/// Get the attack success chance based on defense rating and attack rating
/// Returns 0-100 where 0 = miss and 100 is 100% hit
/// See https://docs.google.com/spreadsheets/d/14BGEPwN2556vnvSMDnqgBuBqOxbb6qDXMCxARAF-9vk/edit#gid=1386445470
/// </summary>
int CDamageHandler::GetPlayerAttackChancePVP(int iAttackerAttackRating, int iTargetDefenseRating)
{
	const int baseline = 90; //90% accuracy is the base for players (if attack rating and defense is same)
	const int scalingAbove = 50;
	const int scalingBelow = 50;
	const int max = 100;
	const int min = 50;

	//if difference is +ve then monster's attack rating is higher
	//if difference is -ve then monster's attack rating is lower
	const int diff = iAttackerAttackRating - iTargetDefenseRating;
	const int accuracy = baseline + ( diff / (diff > 0 ? scalingAbove : scalingBelow) );

	if ( accuracy > max )
		return max;

	if ( accuracy < min )
		return min;

	return accuracy;
}

BOOL CDamageHandler::SendDamagePvPRaw(User * p_Attacker, User * p_Target, int p_Damage, int p_AttackRating, EAttackState p_State, ESkillID p_SkillID, BOOL p_Critical, int16_t p_Param1, int16_t p_Param2)
{
	PacketDebugDamageInfo l_InfoPacket;
	ZeroMemory( &l_InfoPacket, sizeof(PacketDebugDamageInfo) );
	l_InfoPacket.iHeader = PKTHDR_DamageDebugInfo;
	l_InfoPacket.iLength = sizeof( PacketDebugDamageInfo );
	l_InfoPacket.dwAttackerID = p_Attacker->GetID();
	l_InfoPacket.dwObjectID = p_Target->GetID();

	//check block
	if ( p_Target->sBlockChance > 0 &&
		p_Target->sBlockChance > Dice::RandomI( 0, 99 ) )
	{
		l_InfoPacket.sTypeAction = EDamageTextType::Block;
		SENDPACKETRANGE( &l_InfoPacket, &p_Target->pcUserData->sPosition, p_Target->pcUserData->iMapID, DISTANCE_XY_20_meters );
		return FALSE;
	}

	//check evade
	if ( p_Target->sEvadeChance  > 0 &&
		p_Target->sEvadeChance > Dice::RandomI( 0, 99 ) )
	{
		l_InfoPacket.sTypeAction = EDamageTextType::Evade;
		SENDPACKETRANGE( &l_InfoPacket, &p_Target->pcUserData->sPosition, p_Target->pcUserData->iMapID, DISTANCE_XY_20_meters );
		return FALSE;
	}

	//Check defense
	if ( p_Target->iDefenseRating > 0 &&
		p_AttackRating > 0 &&
		( GetPlayerAttackChancePVP(p_AttackRating, p_Target->iDefenseRating) > Dice::RandomI( 0, 99 )) == FALSE)
	{
		l_InfoPacket.sTypeAction = EDamageTextType::Defense;
		SENDPACKETRANGE( &l_InfoPacket, &p_Target->pcUserData->sPosition, p_Target->pcUserData->iMapID, DISTANCE_XY_20_meters, FALSE );
		return FALSE;
	}

	l_InfoPacket.sTypeAction = p_Critical ? EDamageTextType::Critical : EDamageTextType::Normal;
	l_InfoPacket.iValue = p_Damage;
	SENDPACKETRANGE( &l_InfoPacket, &p_Target->pcUserData->sPosition, p_Target->pcUserData->iMapID, DISTANCE_XY_20_meters, FALSE, p_Target->pcUserData );

	l_InfoPacket.sTypeAction = EDamageTextType::Damage;
	SENDPACKET( p_Target, &l_InfoPacket );


	PacketDamageQuick l_QuickDamagePacket;
	l_QuickDamagePacket.iLength = sizeof( PacketDamageQuick );
	l_QuickDamagePacket.iHeader = PKTHDR_DamageQuick;
	l_QuickDamagePacket.bPercentile = FALSE;
	l_QuickDamagePacket.bBaseCurrentHP = FALSE;
	l_QuickDamagePacket.iDamage = p_Damage;
	//l_QuickDamagePacket.idAttacker = p_Attacker->iID;
	//l_QuickDamagePacket.sLevel = p_Attacker->sCharacterData.iLevel;
	//l_QuickDamagePacket.iAttackRating = p_Attacker->sCharacterData.iAttackRating;
	//l_QuickDamagePacket.eAttackState = p_State;
	//l_QuickDamagePacket.eSkillID = p_SkillID;
	//l_QuickDamagePacket.sParam1 = p_Param1;
	//l_QuickDamagePacket.sParam2 = p_Param2;

	SENDPACKET( p_Target, &l_QuickDamagePacket );


	return TRUE;


	//PacketAttackData	TransAttackData;
	//int	temp;
	//
	//
	//
	//TransAttackData.iHeader = PKTHDR_AttackData;
	//TransAttackData.iLength = sizeof(PacketAttackData);
	//
	//TransAttackData.sPosition.iX = p_Target->sPosition.iX;
	//TransAttackData.sPosition.iY = p_Target->sPosition.iY;
	//TransAttackData.sPosition.iZ = p_Target->sPosition.iZ;
	//
	//TransAttackData.eAttackState	= p_State;
	//TransAttackData.iRange			= 32 * fONE;
	//TransAttackData.iDamage			= p_Damage;
	//
	//
	//if (p_Param1 == 0 && p_Param2 == 0)
	//{
	//	TransAttackData.sRating[0] = p_Attacker->sCharacterData.iLevel;
	//	TransAttackData.sRating[1] = p_Attacker->sCharacterData.iAttackRating;
	//}
	//else
	//{
	//	TransAttackData.sRating[0] = p_Param1;
	//	TransAttackData.sRating[1] = p_Param2;
	//}
	//
	//TransAttackData.dwDestObjectSerial	 = p_Attacker->iID;
	//TransAttackData.dwTarObjectSerial	= p_Target->iID;
	//TransAttackData.dwChkSum = TransAttackData.iDamage * 2002 +
	//	(TransAttackData.sPosition.iX * TransAttackData.sPosition.iY * TransAttackData.sPosition.iZ);//anti-tamper code
	//
	//if (p_Target->pcSocketData)
	//{
	//	//attack data log
	//	p_Target->Send_AttackCount++;
	//	p_Target->Send_AttackDamage += TransAttackData.iDamage;
	//
	//	//Character PK Record
	//	DAMAGEHANDLER->UpdateBCPVPScoring(p_Attacker, p_Target, TransAttackData.iDamage);
	//
	//	TransAttackData.dwDamageChkSum = dm_GetDamgeChkSum_S2V(&TransAttackData);		//S2C attack checksum
	//
	//	p_Target->pcSocketData->SendPacket(&TransAttackData);
	//}
	//
	//return TRUE;
}

BOOL CDamageHandler::RefreshPvPDamageBuff(UserData* p_Attacker, UserData* p_Target, int p_Damage, EAttackState p_State, ESkillID p_SkillID, PvPDamageBuffSlot p_Slot, int p_RepeatCount, int p_Interval, int16_t p_Param1, int16_t p_Param2)
{
	User* l_TargetUser = USERDATATOUSER(p_Target);
	if (!l_TargetUser)
		return false;

	/// Look for any existing slot first
	DamagePvPDataHandler* l_WorkingSlot = nullptr;
	for (int l_SlotI = 0; l_SlotI < PVPDAMAGE_MAX_QUEUE; l_SlotI++)
	{
		DamagePvPDataHandler* l_Slot = l_TargetUser->sPvPDamageData + l_SlotI;

		if (!l_Slot->Active || l_Slot->Slot != p_Slot)
			continue;

		l_WorkingSlot = l_Slot;
		break;
	}

	/// If no slot found, found any available one
    for (int l_SlotI = 0; l_SlotI < PVPDAMAGE_MAX_QUEUE; l_SlotI++)
    {
        DamagePvPDataHandler* l_Slot = l_TargetUser->sPvPDamageData + l_SlotI;

        if (l_Slot->Active)
            continue;

        l_WorkingSlot = l_Slot;
        break;
    }

	if (!l_WorkingSlot)
        return false;

	bool l_WasActive = l_WorkingSlot->Active;

	l_WorkingSlot->Active			= true;
	l_WorkingSlot->Slot				= p_Slot;
	l_WorkingSlot->CasterID			= p_Attacker->iID;
	l_WorkingSlot->SkillID			= p_SkillID;
	l_WorkingSlot->State			= p_State;
	l_WorkingSlot->RepeatCount		= p_RepeatCount;
	l_WorkingSlot->RepeatInterval	= p_Interval;
    l_WorkingSlot->Param1			= p_Param1;
    l_WorkingSlot->Param2			= p_Param2;

	if (!l_WasActive)
	{
		l_WorkingSlot->NextUpdate	= TICKCOUNT + p_Interval;
		l_WorkingSlot->Damage		= p_Damage;
	}
	else if (l_WasActive && p_Damage > l_WorkingSlot->Damage)
		l_WorkingSlot->Damage		= p_Damage;

	return false;
}

BOOL CDamageHandler::UpdateDamagePvPDamageBuffs(User * p_User)
{
	auto l_CurrentTickCount = TICKCOUNT;
	for (int l_SlotI = 0; l_SlotI < PVPDAMAGE_MAX_QUEUE; l_SlotI++)
	{
		DamagePvPDataHandler * l_Slot = p_User->sPvPDamageData + l_SlotI;

		if (!l_Slot->Active)
			continue;

		/// Cancel debuff is user is dead
		if (p_User->pcUserData->IsDead())
		{
			l_Slot->Active = false;
			continue;
		}

		if (l_CurrentTickCount < l_Slot->NextUpdate)
			continue;

		l_Slot->RepeatCount--;

		/// Cancel buff
		if (l_Slot->RepeatCount < 0)
        {
            l_Slot->Active = false;
            continue;
		}

		l_Slot->NextUpdate = l_CurrentTickCount + l_Slot->RepeatInterval;

		auto l_Attacker = USERDATABYID(l_Slot->CasterID);
		if (l_Attacker)
		{
			auto l_Damage = l_Slot->Damage;
			UxxxTarget l_Target(p_User->pcUserData, nullptr);

			AdjustDamageFromAbsorb(&l_Damage, &l_Target);
			switch (l_Slot->Slot)
			{
				case PvPDamageBuffSlot::Burning:
					AdjustDamageFromMonsterResistence(EAttackProperty::Fire, &l_Damage, &l_Target);
					break;

				case PvPDamageBuffSlot::Poison:
					AdjustDamageFromMonsterResistence(EAttackProperty::Poison, &l_Damage, &l_Target);
					break;

			}

			SendDamagePvPData(l_Attacker, p_User->pcUserData, l_Damage, 0, l_Slot->State, l_Slot->SkillID, l_Slot->Param1, l_Slot->Param2);
		}

	}

	return TRUE;
}

void CDamageHandler::SendCancelSkill( User * pcUser, int iPlayID )
{
	PacketSkillCancel s;
	s.iLength	= sizeof( PacketSkillCancel );
	s.iHeader	= PKTHDR_SkillCast;
	s.iPlayID	= iPlayID;
	s.iUnk		= 0;
	s.iUnk2		= 0;
	s.iUnk3		= 0;
	SENDPACKET( pcUser, &s );
}

BOOL CDamageHandler::CanHitTargetAreaDamage( UserData * pcUserData, UnitData * pcUnitData )
{
	User * pcUser = USERDATATOUSER( pcUserData );

	if ( pcUser->GetMapID() != pcUnitData->pcMap->pcBaseMap->iMapID )
		return FALSE;

	if ( pcUser->bInT5QuestArena )
	{
		if ( pcUser->GetMapID() == MAPID_T5QuestArena )
		{
			if ( pcUnitData->lExclusiveTargetID )
			{
				if ( pcUnitData->lExclusiveTargetID != pcUser->GetID() )
					return FALSE;
			}
		}
	}

	return TRUE;
}

void CDamageHandler::LoadNewPVPScalingSettings()
{
	INFO("Loading PVP Scaling settings..");

	// first, create a file instance
	mINI::INIFile file("PVPScaling.ini");

	// next, create a structure that will hold data
	mINI::INIStructure ini;

	// now we can read the file
	if (file.read(ini))
	{
		fGlobalDamageReduction = static_cast<float>(atof(&ini["General"]["dGlobalDamageReduction"][0]) );
		fGlobalAbsReduction = static_cast<float>(atof(&ini["General"]["dGlobalAbsReduction"][0]) );
		fLevelBasedPvpDamageScale = static_cast<float>(atof(&ini["Level Based Scaling"]["dLevelBasedPvpDamageScale"][0]) );
		fLevelBasedPvpAbsScale = static_cast<float>( atof( &ini["Level Based Scaling"]["dLevelBasedPvpAbsScale"][0] ) );

		INFO("LoadNewPVPScalingSettings");
		INFO(" - dGlobalDamageReduction:    %d", fGlobalDamageReduction);
		INFO(" - dGlobalAbsReduction:       %d", fGlobalAbsReduction);
		INFO(" - dLevelBasedPvpDamageScale: %d", fGlobalAbsReduction);
		INFO(" - dLevelBasedPvpAbsScale:    %d", fLevelBasedPvpAbsScale);
	}
	else
	{
		LOGERROR( "PVPScaling.ini not found!" );
	}
}


void CDamageHandler::UpdatePVPScalingIni(char* iniSection, char* iniKey, double iniValue)
{
	mINI::INIFile file("PVPScaling.ini");
	mINI::INIStructure ini;
	if (file.read(ini))
	{
		char buffer[5];
		sprintf(buffer, "%.2f", iniValue);
		ini[iniSection][iniKey] = buffer;
		file.write(ini);
	}
}

void CDamageHandler::UpdatePVPScalingIni(char* iniSection, char* iniKey, char* iniValue)
{
	mINI::INIFile file("PVPScaling.ini");
	mINI::INIStructure ini;
	if (file.read(ini))
	{
		ini[iniSection][iniKey] = iniValue;
		file.write(ini);
	}
}

//todo
//need to get current equipped weapon damage (including age)
/*
BOOL CDamageHandler::UpdateUserAttackPower( User * pcUser )
{
	int iItemIndex = pInventory[0].iItemIndex - 1;
	int iItemClass = pItem[iItemIndex].iItemWpnDmg;
	int iItemSpec = pItem[iItemIndex].sItem.eSpecialization;

	CharacterData * playerData = PLAYERDATA;

	const DWORD	pdwInventory = 0x035EBB20;
	const DWORD	pdwStrength = pdwInventory + 0x39C4C;
	const DWORD	pdwSpirit = pdwInventory + 0x39C58;
	const DWORD	pdwTalent = pdwInventory + 0x39C54;
	const DWORD	pdwAgility = pdwInventory + 0x39C5C;
	const DWORD	pdwHealth = pdwInventory + 0x39C50;

	int iStrength = playerData->iStrength + ( *(int *)pdwStrength );
	int iSpirit = playerData->iSpirit + ( *(int *)pdwSpirit );
	int iTalent = playerData->iTalent + ( *(int *)pdwTalent );
	int iAgility = playerData->iAgility + ( *(int *)pdwAgility );
	int iHealth = playerData->iHealth + ( *(int *)pdwHealth );


	int iMinDamage = 1 + ( ( iStrength + 130 ) / 130 ) + ( ( iTalent + iAgility ) / 40 );
	int iMaxDamage = 2 + ( ( iStrength + 130 ) / 130 ) + ( ( iTalent + iAgility ) / 40 );

	ItemData * pcWeaponItemData = ITEMSERVER->FindItemPointerTable( pcUser->pcUserData->eWeaponEquipped );

	if ( pcWeaponItemData )
	{
		int iWeaponDamageMin = pcWeaponItemData.item
		int iWeaponDamageMax = 0;
	}



	switch ( playerData->iClass )
	{
		case CHARACTERCLASS_Fighter:
			if ( iItemClass == WEAPONCLASS_Melee )
			{
				iMinDamage = 1 + ( ( iWeaponDamage[0] * ( iStrength + 260 ) / 260 ) ) + ( ( iWeaponDamage[0] * ( iSpirit + 520 ) / 520 ) ) + ( ( iWeaponDamage[0] * ( iTalent + 400 ) / 400 ) ) + ( ( iWeaponDamage[0] * ( iAgility + 520 ) / 520 ) ) - iWeaponDamage[0];
				iMaxDamage = 3 + ( ( iWeaponDamage[1] * ( iStrength + 260 ) / 260 ) ) + ( ( iWeaponDamage[1] * ( iSpirit + 520 ) / 520 ) ) + ( ( iWeaponDamage[1] * ( iTalent + 400 ) / 400 ) ) + ( ( iWeaponDamage[1] * ( iAgility + 520 ) / 520 ) ) - iWeaponDamage[1];
				if ( iItemSpec == CLASSFLAG_Fighter )
				{
					iMinDamage += ( iWeaponDamage[0] + iWeaponDamage[1] ) / 16;
					if ( pItem[iItemIndex].sItem.sSpecData.iSpecAttackPowerDiv > 0 )
						iMaxDamage += playerData->iLevel / pItem[iItemIndex].sItem.sSpecData.iSpecAttackPowerDiv;
				}
			}
			break;

		case CHARACTERCLASS_Mechanician:
			if ( iItemClass == WEAPONCLASS_Melee || iItemClass == WEAPONCLASS_Ranged )
			{
				if ( iItemClass == WEAPONCLASS_Ranged )
				{
					iMinDamage = 1 + ( ( iWeaponDamage[0] * ( iStrength + 560 ) / 560 ) ) + ( ( iWeaponDamage[0] * ( iSpirit + 600 ) / 600 ) ) + ( ( iWeaponDamage[0] * ( iTalent + 560 ) / 560 ) ) + ( ( iWeaponDamage[0] * ( iAgility + 320 ) / 320 ) ) - iWeaponDamage[0];
					iMaxDamage = 3 + ( ( iWeaponDamage[1] * ( iStrength + 560 ) / 560 ) ) + ( ( iWeaponDamage[1] * ( iSpirit + 600 ) / 600 ) ) + ( ( iWeaponDamage[1] * ( iTalent + 560 ) / 560 ) ) + ( ( iWeaponDamage[1] * ( iAgility + 320 ) / 320 ) ) - iWeaponDamage[1];
				}
				else
				{
					iMinDamage = 1 + ( ( iWeaponDamage[0] * ( iStrength + 320 ) / 320 ) ) + ( ( iWeaponDamage[0] * ( iSpirit + 600 ) / 600 ) ) + ( ( iWeaponDamage[0] * ( iTalent + 560 ) / 560 ) ) + ( ( iWeaponDamage[0] * ( iAgility + 560 ) / 560 ) ) - iWeaponDamage[0];
					iMaxDamage = 3 + ( ( iWeaponDamage[1] * ( iStrength + 320 ) / 320 ) ) + ( ( iWeaponDamage[1] * ( iSpirit + 600 ) / 600 ) ) + ( ( iWeaponDamage[1] * ( iTalent + 560 ) / 560 ) ) + ( ( iWeaponDamage[1] * ( iAgility + 560 ) / 560 ) ) - iWeaponDamage[1];
				}

				if ( iItemSpec == CLASSFLAG_Mechanician )
				{
					iMinDamage += ( iWeaponDamage[0] + iWeaponDamage[1] ) / 16;
					if ( pItem[iItemIndex].sItem.sSpecData.iSpecAttackPowerDiv > 0 )
						iMaxDamage += playerData->iLevel / pItem[iItemIndex].sItem.sSpecData.iSpecAttackPowerDiv;
				}
			}
			break;

		case CHARACTERCLASS_Archer:
			if ( iItemClass == WEAPONCLASS_Ranged )
			{
				iMinDamage = 2 + ( ( iWeaponDamage[0] * ( iStrength + 400 ) / 400 ) ) + ( ( iWeaponDamage[0] * ( iSpirit + 520 ) / 520 ) ) + ( ( iWeaponDamage[0] * ( iTalent + 460 ) / 460 ) ) + ( ( iWeaponDamage[0] * ( iAgility + 260 ) / 260 ) ) - iWeaponDamage[0];
				iMaxDamage = 3 + ( ( iWeaponDamage[1] * ( iStrength + 400 ) / 400 ) ) + ( ( iWeaponDamage[1] * ( iSpirit + 520 ) / 520 ) ) + ( ( iWeaponDamage[1] * ( iTalent + 460 ) / 460 ) ) + ( ( iWeaponDamage[1] * ( iAgility + 260 ) / 260 ) ) - iWeaponDamage[1];
				if ( iItemSpec == CLASSFLAG_Archer )
				{
					iMinDamage += ( iWeaponDamage[0] + iWeaponDamage[1] ) / 16;
					if ( pItem[iItemIndex].sItem.sSpecData.iSpecAttackPowerDiv > 0 )
						iMaxDamage += playerData->iLevel / pItem[iItemIndex].sItem.sSpecData.iSpecAttackPowerDiv;
				}
			}
			break;

		case CHARACTERCLASS_Pikeman:
			if ( iItemClass == WEAPONCLASS_Melee )
			{
				iMinDamage = 1 + ( ( iWeaponDamage[0] * ( iStrength + 240 ) / 240 ) ) + ( ( iWeaponDamage[0] * ( iSpirit + 480 ) / 480 ) ) + ( ( iWeaponDamage[0] * ( iTalent + 420 ) / 420 ) ) + ( ( iWeaponDamage[0] * ( iAgility + 400 ) / 400 ) ) - iWeaponDamage[0];
				iMaxDamage = 3 + ( ( iWeaponDamage[1] * ( iStrength + 240 ) / 240 ) ) + ( ( iWeaponDamage[1] * ( iSpirit + 480 ) / 480 ) ) + ( ( iWeaponDamage[1] * ( iTalent + 420 ) / 420 ) ) + ( ( iWeaponDamage[1] * ( iAgility + 400 ) / 400 ) ) - iWeaponDamage[1];
				if ( iItemSpec == CLASSFLAG_Pikeman )
				{
					iMinDamage += ( iWeaponDamage[0] + iWeaponDamage[1] ) / 16;
					if ( pItem[iItemIndex].sItem.sSpecData.iSpecAttackPowerDiv > 0 )
						iMaxDamage += playerData->iLevel / pItem[iItemIndex].sItem.sSpecData.iSpecAttackPowerDiv;
				}
			}
			break;

		case CHARACTERCLASS_Atalanta:
			if ( iItemClass == WEAPONCLASS_Ranged )
			{
				iMinDamage = 2 + ( ( iWeaponDamage[0] * ( iStrength + 400 ) / 400 ) ) + ( ( iWeaponDamage[0] * ( iSpirit + 460 ) / 460 ) ) + ( ( iWeaponDamage[0] * ( iTalent + 520 ) / 520 ) ) + ( ( iWeaponDamage[0] * ( iAgility + 260 ) / 260 ) ) - iWeaponDamage[0];
				iMaxDamage = 3 + ( ( iWeaponDamage[1] * ( iStrength + 400 ) / 400 ) ) + ( ( iWeaponDamage[1] * ( iSpirit + 460 ) / 460 ) ) + ( ( iWeaponDamage[1] * ( iTalent + 520 ) / 520 ) ) + ( ( iWeaponDamage[1] * ( iAgility + 260 ) / 260 ) ) - iWeaponDamage[1];
				if ( iItemSpec == CLASSFLAG_Atalanta )
				{
					iMinDamage += ( iWeaponDamage[0] + iWeaponDamage[1] ) / 16;
					if ( pItem[iItemIndex].sItem.sSpecData.iSpecAttackPowerDiv > 0 )
						iMaxDamage += playerData->iLevel / pItem[iItemIndex].sItem.sSpecData.iSpecAttackPowerDiv;
				}
			}
			break;

		case CHARACTERCLASS_Knight:
			if ( iItemClass == WEAPONCLASS_Melee )
			{
				iMinDamage = 1 + ( ( iWeaponDamage[0] * ( iStrength + 300 ) / 300 ) ) + ( ( iWeaponDamage[0] * ( iSpirit + 520 ) / 520 ) ) + ( ( iWeaponDamage[0] * ( iTalent + 440 ) / 440 ) ) + ( ( iWeaponDamage[0] * ( iAgility + 600 ) / 600 ) ) - iWeaponDamage[0];
				iMaxDamage = 3 + ( ( iWeaponDamage[1] * ( iStrength + 300 ) / 300 ) ) + ( ( iWeaponDamage[1] * ( iSpirit + 520 ) / 520 ) ) + ( ( iWeaponDamage[1] * ( iTalent + 440 ) / 440 ) ) + ( ( iWeaponDamage[1] * ( iAgility + 600 ) / 600 ) ) - iWeaponDamage[1];
				if ( iItemSpec == CLASSFLAG_Knight )
				{
					iMinDamage += ( iWeaponDamage[0] + iWeaponDamage[1] ) / 16;
					if ( pItem[iItemIndex].sItem.sSpecData.iSpecAttackPowerDiv > 0 )
						iMaxDamage += playerData->iLevel / pItem[iItemIndex].sItem.sSpecData.iSpecAttackPowerDiv;
				}
			}
			break;

		case CHARACTERCLASS_Magician:
			if ( iItemClass == WEAPONCLASS_Magic )
			{
				iMinDamage = 2 + ( ( iWeaponDamage[0] * ( iStrength + 520 ) / 520 ) ) + ( ( iWeaponDamage[0] * ( iSpirit + 260 ) / 260 ) ) + ( ( iWeaponDamage[0] * ( iTalent + 440 ) / 440 ) ) + ( ( iWeaponDamage[0] * ( iAgility + 480 ) / 480 ) ) - iWeaponDamage[0];
				iMaxDamage = 3 + ( ( iWeaponDamage[1] * ( iStrength + 520 ) / 520 ) ) + ( ( iWeaponDamage[1] * ( iSpirit + 260 ) / 260 ) ) + ( ( iWeaponDamage[1] * ( iTalent + 440 ) / 440 ) ) + ( ( iWeaponDamage[1] * ( iAgility + 480 ) / 480 ) ) - iWeaponDamage[1];
				if ( iItemSpec == CLASSFLAG_Magician )
				{
					iMinDamage += ( iWeaponDamage[0] + iWeaponDamage[1] ) / 16;
					if ( pItem[iItemIndex].sItem.sSpecData.iSpecAttackPowerDiv > 0 )
						iMaxDamage += playerData->iLevel / pItem[iItemIndex].sItem.sSpecData.iSpecAttackPowerDiv;
				}
			}
			break;

		case CHARACTERCLASS_Priestess:
			if ( iItemClass == WEAPONCLASS_Magic )
			{
				iMinDamage = 2 + ( ( iWeaponDamage[0] * ( iStrength + 480 ) / 480 ) ) + ( ( iWeaponDamage[0] * ( iSpirit + 240 ) / 240 ) ) + ( ( iWeaponDamage[0] * ( iTalent + 400 ) / 400 ) ) + ( ( iWeaponDamage[0] * ( iAgility + 480 ) / 480 ) ) - iWeaponDamage[0];
				iMaxDamage = 3 + ( ( iWeaponDamage[1] * ( iStrength + 480 ) / 480 ) ) + ( ( iWeaponDamage[1] * ( iSpirit + 240 ) / 240 ) ) + ( ( iWeaponDamage[1] * ( iTalent + 400 ) / 400 ) ) + ( ( iWeaponDamage[1] * ( iAgility + 480 ) / 480 ) ) - iWeaponDamage[1];
				if ( iItemSpec == CLASSFLAG_Priestess )
				{
					iMinDamage += ( iWeaponDamage[0] + iWeaponDamage[1] ) / 16;
					if ( pItem[iItemIndex].sItem.sSpecData.iSpecAttackPowerDiv > 0 )
						iMaxDamage += playerData->iLevel / pItem[iItemIndex].sItem.sSpecData.iSpecAttackPowerDiv;
				}
			}
			break;

		case CHARACTERCLASS_Assassin:
			if ( iItemClass == WEAPONCLASS_Melee )
			{
				iMinDamage = 1 + ( ( iWeaponDamage[0] * ( iStrength + 240 ) / 240 ) ) + ( ( iWeaponDamage[0] * ( iSpirit + 480 ) / 480 ) ) + ( ( iWeaponDamage[0] * ( iTalent + 420 ) / 420 ) ) + ( ( iWeaponDamage[0] * ( iAgility + 400 ) / 400 ) ) - iWeaponDamage[0];
				iMaxDamage = 3 + ( ( iWeaponDamage[1] * ( iStrength + 240 ) / 240 ) ) + ( ( iWeaponDamage[1] * ( iSpirit + 480 ) / 480 ) ) + ( ( iWeaponDamage[1] * ( iTalent + 420 ) / 420 ) ) + ( ( iWeaponDamage[1] * ( iAgility + 400 ) / 400 ) ) - iWeaponDamage[1];
				if ( iItemSpec == CLASSFLAG_Assassin )
				{
					iMinDamage += ( iWeaponDamage[0] + iWeaponDamage[1] ) / 16;
					if ( pItem[iItemIndex].sItem.sSpecData.iSpecAttackPowerDiv > 0 )
						iMaxDamage += playerData->iLevel / pItem[iItemIndex].sItem.sSpecData.iSpecAttackPowerDiv;
				}
			}
			break;

		case CHARACTERCLASS_Shaman:
			if ( iItemClass == WEAPONCLASS_Magic )
			{
				iMinDamage = 2 + ( ( iWeaponDamage[0] * ( iStrength + 520 ) / 520 ) ) + ( ( iWeaponDamage[0] * ( iSpirit + 260 ) / 260 ) ) + ( ( iWeaponDamage[0] * ( iTalent + 440 ) / 440 ) ) + ( ( iWeaponDamage[0] * ( iAgility + 480 ) / 480 ) ) - iWeaponDamage[0];
				iMaxDamage = 3 + ( ( iWeaponDamage[1] * ( iStrength + 520 ) / 520 ) ) + ( ( iWeaponDamage[1] * ( iSpirit + 260 ) / 260 ) ) + ( ( iWeaponDamage[1] * ( iTalent + 440 ) / 440 ) ) + ( ( iWeaponDamage[1] * ( iAgility + 480 ) / 480 ) ) - iWeaponDamage[1];
				if ( iItemSpec == CLASSFLAG_Shaman )
				{
					iMinDamage += ( iWeaponDamage[0] + iWeaponDamage[1] ) / 16;
					if ( pItem[iItemIndex].sItem.sSpecData.iSpecAttackPowerDiv > 0 )
						iMaxDamage += playerData->iLevel / pItem[iItemIndex].sItem.sSpecData.iSpecAttackPowerDiv;
				}
			}
			break;

		default:
			break;
	}

	SKILLMANAGERHANDLER->SetWeaponDamageBuffFromSkill( iSkillDamage, iWeaponDamage );

	//Add passive skill buffs
	iMinDamage += iSkillDamage[0];
	iMaxDamage += iSkillDamage[1];

	//Add extra damage if using matching spec based weapon
	iMinDamage += *iLevelDamage;
	iMaxDamage += *iLevelDamage;

#if defined (DEV_MODE) && defined (GM_MODE)
	if ( MAP_ID == MAPID_QuestArena )
	{
		int iAvg = ( iMinDamage + iMaxDamage ) / 2;
		playerData->iMinDamage = iAvg;
		playerData->iMaxDamage = iAvg;
	}
	else
	{
		playerData->iMinDamage = iMinDamage;
		playerData->iMaxDamage = iMaxDamage;
	}
#else
	playerData->iMinDamage = iMinDamage;
	playerData->iMaxDamage = iMaxDamage;
#endif

	//store base attack power
	CHARACTERGAME->sCharacterDataEx.iBaseAttackPowerMin = playerData->iMinDamage;
	CHARACTERGAME->sCharacterDataEx.iBaseAttackPowerMax = playerData->iMaxDamage;
}*/