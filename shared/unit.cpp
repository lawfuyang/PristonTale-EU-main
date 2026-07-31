#include "stdafx.h"
#include "unit.h"

Unit pcaUnitServer[MAX_UNITS];


ECharacterGender UnitData::GetCharacterGender()
{
	if( this->sCharacterData.iClass == 3 || this->sCharacterData.iClass == 5 || this->sCharacterData.iClass == 8 || this->sCharacterData.iClass == 9 )
		return CHARACTERGENDER_Female;

	return CHARACTERGENDER_Male;
}

int UnitData::GetMaxStamina()
{
	return this->sCharacterData.sSP.sMax;
}

int UnitData::GetMaxMana()
{
	return this->sCharacterData.sMP.sMax;
}

int UnitData::GetMaxHealth()
{
#ifdef _GAME

	//is me?
	if ( this->iID == UNITDATA->iID )
		return this->sCharacterData.sHP.sMax;

	Unit * pcUnit = UNITDATATOUNIT( this );

	if ( pcUnit == NULL )
		return 0;

	return pcUnit->sHPLong.iMax;
#else

	if ( this->IsNonPlayerMonster() )
	{
		return this->pcUnit->sHPLong.iMax;
	}
	else
	{
		return this->sCharacterData.sHP.sMax;
	}

#endif
}



/// <summary>
/// Get percentage hp remaining in 0 - 100 range
/// </summary>
int UnitData::GetHealthRemainingPercentage()
{
	int max = GetMaxHealth();
	int curr = GetCurrentHealth();

	return (curr * max) / 100;
}

void UnitData::CheckHealthStatusAfterDamageReceived( UserData * pcAttacker )
{
#ifndef _GAME
	if ( GetCurrentHealth() <= 0 )
	{
		Animate ( ANIMATIONTYPE_Die );
		pcKillerData = pcAttacker;
	}
#endif
}


void UnitData::CheckHealthStatusAfterDamageOverTimeReceived( UserData * pcDoTCaster )
{
#ifndef _GAME
	if ( GetCurrentHealth() <= 0 )
	{
		Animate ( ANIMATIONTYPE_Die );

		//priority attacker takes priority if DoT kills.
		if ( pcPriorityAttacker )
		{
			pcKillerData = pcPriorityAttacker;
		}

		//otherwise use the DoT Caster
		else
		{
			pcKillerData = pcDoTCaster;
		}
	}
#endif
}

int UnitData::GetCurrentHealth()
{
#ifdef _GAME

	//is me?
	if ( this->iID == UNITDATA->iID )
		return CHARACTERGAME->GetCurrentHP();

	Unit * pcUnit = UNITDATATOUNIT( this );

	if ( pcUnit == NULL )
		return 0;

	return pcUnit->sHPLong.iMin;
#else

	if ( this->IsNonPlayerMonster() )
	{
		return this->pcUnit->sHPLong.iMin;
	}
	else
	{
		return this->sCharacterData.sHP.sCur;
	}


#endif // _GAME
}

void UnitData::SetCurrentHealthToMax ()
{
#ifndef _GAME

	if ( this->IsNonPlayerMonster() )
	{
		Unit * pcUnit = UNITDATATOUNIT( this );
		pcUnit->sHPLong.iMin = this->pcUnit->sHPLong.iMax;

		//don't use these HP values. Always set to 30000
		this->sCharacterData.sHP.sCur = MONSTER_HP_OVERRIDE;
		this->sCharacterData.sHP.sMax = MONSTER_HP_OVERRIDE;
		this->sHPBlocks.sCur = 0;
		this->sHPBlocks.sMax = 0;
	}
	else
	{
		this->sCharacterData.sHP.sCur = this->sCharacterData.sHP.sMax;
	}

#endif
}

void UnitData::InitHealthPoint( int iValue )
{
#ifndef _GAME

	if ( this->IsNonPlayerMonster() )
	{
		Unit * pcUnit = UNITDATATOUNIT( this );
		pcUnit->sHPLong.iMin = iValue;
		pcUnit->sHPLong.iMax = iValue;

		//don't use these HP values. Always set to 30000
		this->sCharacterData.sHP.sCur = MONSTER_HP_OVERRIDE;
		this->sCharacterData.sHP.sMax = MONSTER_HP_OVERRIDE;
		this->sHPBlocks.sCur = 0;
		this->sHPBlocks.sMax = 0;
	}
	else
	{
		this->sCharacterData.sHP.sCur = iValue;
		this->sCharacterData.sHP.sMax = iValue;
	}
#endif
}

void UnitData::SetCurrentHealth( int iValue )
{
#ifndef _GAME

	if ( iValue <= 0 )
	{
		this->sCharacterData.sHP.sCur = 0;
		this->pcUnit->sHPLong.iMin = 0;
		this->sHPBlocks.sCur = 0;
		this->sVirtualHP.sCur = 0;
		return;
	}

	if ( iValue >= GetMaxHealth() )
	{
		SetCurrentHealthToMax();
	}
	else
	{
		if ( this->IsNonPlayerMonster() )
		{
			this->pcUnit->sHPLong.iMin = iValue;

			//don't use these HP values as it causes unwanted behavior from ASM
			//Always set to 30000
			this->sCharacterData.sHP.sCur = MONSTER_HP_OVERRIDE;
			this->sCharacterData.sHP.sMax = MONSTER_HP_OVERRIDE;
			this->sHPBlocks.sCur = 0;
			this->sHPBlocks.sMax = 0;
		}
		else
		{
			this->sCharacterData.sHP.sCur = iValue;
		}
	}



#endif
}

/// <summary>
/// Take health. i.e. apply damage to health point
/// On damage, remove stun efffect if there is one.
/// </summary>
/// <param name="iValue">The total value</param>
void UnitData::TakeHealth( int iValue )
{
#ifndef _GAME
	if ( iValue <= 0 )
		return;

	const int iCurrent = GetCurrentHealth();
	const int iNewHealth = iCurrent - iValue;

	SetCurrentHealth( iNewHealth );
#endif
}

/// <summary>
/// Take health. i.e. apply damage-over-time damage to health point
/// This function is similar to TakeHealth except that
/// it will always leave 1 HP remaining so that
/// the unit will get a valid 'killer' via normal attacks.
///
/// The function returns the adjusted damage value
/// </summary>
/// <param name="iValue">The total value</param>
int UnitData::ApplyDamageOverTime( int iValue, bool bAllowKill )
{
#ifndef _GAME
	if ( iValue <= 1 ) //1 to prevent bug of 1hp to 2hp due to code below
		return 0;

	int iCurr = GetCurrentHealth();
	int iNewHP = iCurr - iValue;

	//can't use 1 (as it seems to glinch out targeting on client side)
	if ( iNewHP < 2 )
	{
		iNewHP = bAllowKill ? 0 : 2;
	}

	SetCurrentHealth( iNewHP );

	return iCurr - iNewHP;
#else
	//Client builds compile out the body entirely; return no damage applied.
	return 0;
#endif
}

/// <summary>
/// Give health points (i.e. heal)
/// </summary>
/// <param name="iValue">The amount to heal</param>
void UnitData::GiveHealth( int iValue )
{
#ifndef _GAME
	if ( iValue <= 0 )
		return;

	const int iCurrentHP = GetCurrentHealth();

	SetCurrentHealth( iCurrentHP + iValue );
#endif
}

/// <summary>
/// Leak: int smCHAR::Main()
/// ASM: 0x00429930
/// </summary>
/// <returns></returns>
NAKED void UnitData::Main()
{
	JMP( pfnMain );
}

NAKED void UnitData::Init()
{
	JMP( pfnInit );
}

NAKED void UnitData::SetPosition( int iX, int iY, int iZ, int iAX, int iAY, int iAZ )
{
	JMP( pfnSetPosition );
}


/// <summary>
/// Leaked code: int smCHAR::BeginSkill( int SkilCode , int Level , smCHAR *lpTarChar , int x, int y, int z )
/// Skill start
/// </summary>
NAKED int UnitData::BeginSkill( int SkilCode , int Level , UnitData *lpTarChar , int x, int y, int z )
{
	JMP( pfnBeginSkill );
}


NAKED int UnitData::ResetEnergyGraph( int Num )
{
	JMP( pfnResetEnergyGraph );
}


DWORD UnitData::GetAttackTrans_XorCode()
{
#if defined(_GAME)
	return this->iID ^ UNITDATA->sCharacterData.iID ^ (((DWORD)this) >> 8);
#else
	return 1;
#endif
}


int UnitData::ResetAttackTrans()
{
	this->sAttackTrans.iHeader = GetAttackTrans_XorCode();
	return TRUE;
}

int UnitData::XorAttackTrans()
{
	int cnt, len;
	DWORD dwCode;

	len = (sizeof(PacketAttackData) / sizeof(DWORD)); // == 13
	DWORD* lpdwBuff = (DWORD*)&this->sAttackTrans;

	dwCode = GetAttackTrans_XorCode();
	//dwCoded = CALL_WITH_ARG1(0x00422150, (DWORD)pcUnitData); //will produce same result

	lpdwBuff[0] ^= dwCode;
	lpdwBuff[1] ^= dwCode;

	for (cnt = 2; cnt < len; cnt++)
	{
		lpdwBuff[cnt] ^= dwCode;
		dwCode += dwCode;
	}

	return TRUE;
}


////Find the distance of 2 points (return is the squared value of the distance)
int UnitData::GetDistanceDbl( int x1, int z1, int x2, int z2 )
{
	int ax,az;

	ax = (x1 - x2 ) >>FLOATNS;
	az = (z1 - z2 ) >>FLOATNS;
	ax *=ax;
	az *=az;

	return ax+az;
}

/// <summary>
/// Get monster accuracy.
/// 100% means 100% chance to hit
/// 0% means 0% chance to hit
/// The forumla is based on this sheet
/// https://docs.google.com/spreadsheets/d/14BGEPwN2556vnvSMDnqgBuBqOxbb6qDXMCxARAF-9vk/edit#gid=242932432
/// </summary>
int UnitData::GetMonsterAccuracy( int iAttackRating )
{
	const int baseline = 70; //70% accuracy is the base (if attack rating and defense is same)
	const int scaling = 25;
	const int max = 95;
	const int min = 30;

	const int accuracy = baseline - ( ( this->sCharacterData.iDefenseRating - iAttackRating ) / scaling );

	if ( accuracy > max )
		return max;

	if ( accuracy < min )
		return min;

	return accuracy;
}

/// <summary>
/// Begin skill on a target
/// </summary>
int UnitData::BeginSkill( UnitData * lpTarChar )
{
	return BeginSkill( this->sActiveSkill.iValue, 0, lpTarChar, 0, 0, 0 );
}

/// <summary>
/// Begin skill on no target (usually a buff)
/// </summary>
int UnitData::BeginSkill( )
{
	return BeginSkill( this->sActiveSkill.iValue, 0, 0, 0, 0, 0 );
}

/// <summary>
/// Leaked code: SetMotionFromCode
/// Change motion with motion classification code
/// </summary>
/// <param name="eAnimationType">The motion / animation code</param>
NAKED BOOL UnitData::Animate( EAnimationType eAnimationType )
{
	JMP( pfnAnimate );
}

NAKED BOOL UnitData::ChangeAnimationID( int iID, int iModelType )
{
	JMP( pfnChangeAnimationID );
}

//int smCHAR::MakeTransPlayData
//character.cpp online 10596
//iDelayTime = SendTime
//iSleep = pBuffStep
NAKED int UnitData::MakeUnitBufferData( char * pBuffer, int iDelayTime, int iSleep )
{
	JMP( pfnHandleBufferData );
}

/*
Set next destination coordinates
int smCHAR::SetNextTarget( int x, int y, int z )
*/
NAKED BOOL UnitData::NextTarget( int iX, int iY, int iZ )
{
	JMP( pfnSetNextTarget );
}

//int smCHAR::SetTargetPosi( int x, int z )
//for client only ACTIONMODE_ClientSelf (ActionPattern == 0)
NAKED BOOL UnitData::SetTargetPosi( int iX, int iZ )
{
	JMP( pfnSetTarget );
}

NAKED BOOL UnitData::SetTargetEx( int iX, int iY, int iZ )
{
	JMP( pfnSetTargetEx );
}

//int smCHAR::SetTargetPosi2( int x, int z )
//Set y angle
NAKED int UnitData::SetAngleByPosition( int iX, int iZ )
{
	JMP( pfnSetAngleByPosition );
}

/// <summary>
/// Leaked: GetAttackPoint
/// </summary>>
/// <returns></returns>
NAKED BOOL UnitData::GetAnimationAttackPosition( int * iX, int * iY, int * iZ )
{
	JMP( pfnGetAnimAttackPosition );
}

NAKED void UnitData::LoadModel( char * pszModelFilePath )
{
	JMP( pfnLoadModel );
}

//character.cpp line 8656
//int smCHAR::PlayAttack( int EventFlag )
//Attack execution
//Default value for bUpdateTime is FALSE
//PlayAttack causes damage to be dealt
NAKED void UnitData::PlayAttackObsolete( BOOL bUpdateTime )
{
	JMP( pfnUpdateAttack );
}

int UnitData::GetResistancePower( int power , int ResCode )
{
	int rs,result;

	rs = this->sCharacterData.sElementalDef[ResCode];

	if ( rs<0 ) rs = 0;
	if ( rs>=100 ) rs=100;
	rs = 100-rs;

	result = (power*rs)/100;

	if ( result<=0 ) result=4;

	return result;
}

BOOL UnitData::PlayAttackOverride( int EventFlag )
{
#if defined(_GAME)
	UnitData * lpCurPlayer = UNITDATA;
	CharacterData * smCharInfo = &this->sCharacterData;

	int cnt,len,act,total;
	BOOL block = FALSE;
	int chance;
	UnitData *lpChar;
	int	HolyBody_Undead = 0;
	int x,y,z;
	Point3D	Pos1;//,Pos2;
	int temp, lvl;
	int Param1,Param2;
	int	bStunCount;


	PacketAttackData *lpTransAttackData;

	lpTransAttackData = &this->sAttackTrans;


	bool bIsASkillAttack = ( lpTransAttackData->iAttackState & 0xFF ) == 0x80;


	CHARACTERGAME->XorAttackTrans( this, lpTransAttackData );

	if ( !lpTransAttackData->iHeader || ( RESTARTCOUNTER > 0 && RESTARTCOUNTER < 1000 ) ||
		( USEETHERCORECODE && lpCurPlayer->GetAnimation() == EAnimationType::ANIMATIONTYPE_Potion )) {

		this->sAttackTrans.iHeader = this->GetAttackTrans_XorCode();

		if ( RESTARTCOUNTER )
		{
			CALL_WITH_ARG1( 0x4291F0, 0 ); //Ptect_IncAttackCount(0);
		}

		return FALSE;
	}

	if ( lpTransAttackData->dwDamageChkSum != CHARACTERGAME->GetDamgeChkSum_S2V(lpTransAttackData) )
	{
		//Code check error
		this->ResetAttackTrans();
		CALL_WITH_ARG3( 0x627EE0, 8580, lpTransAttackData->iDamage, 0 ); //SendSetHackUser3( 8580, lpTransAttackData->iDamage, 0 );

		return FALSE;
	}

	CALL_WITH_ARG1( 0x4291F0, -1 ); //Ptect_IncAttackCount( -1 );


	if ( lpCurPlayer->GetAnimation() != EAnimationType::ANIMATIONTYPE_Die && this->bVisible )
	{
		if ( smCharInfo->iType == ECharacterType::CHARACTERTYPE_Player || this->PkMode_CharState==ECharacterType::CHARACTERTYPE_Player)
		{
			//PK mode
			len = 0;
			cnt = 0;
		}
		else
		{
			len = this->GetDistanceDbl(lpCurPlayer->sPosition.iX, lpCurPlayer->sPosition.iZ, this->sPosition.iX, this->sPosition.iZ);
			cnt = this->GetDistanceDbl(lpCurPlayer->sPosition.iX, lpCurPlayer->sPosition.iZ, lpTransAttackData->sPosition.iX, lpTransAttackData->sPosition.iZ);
		}

		if ( bIsASkillAttack ||			//skill attack
			smCharInfo->iMonsterEffectID == snCHAR_SOUND_FIGON ||
			smCharInfo->iMonsterEffectID == snCHAR_SOUND_WEB ||
			smCharInfo->iMonsterEffectID == snCHAR_SOUND_SKELETONRANGE ||
			smCharInfo->iMonsterEffectID == snCHAR_SOUND_IMP ||
			smCharInfo->iMonsterEffectID == snCHAR_SOUND_SCORPION ||
			smCharInfo->iMonsterEffectID == snCHAR_SOUND_NAZ ||
			smCharInfo->iMonsterEffectID == snCHAR_SOUND_NAZSENIOR || //Park Jae-won-Endless Tower 3rd Floor (New Field Monster)
			smCharInfo->iMonsterEffectID == snCHAR_SOUND_SHADOW ||
			smCharInfo->iMonsterEffectID == snCHAR_SOUND_BEEDOG ||
			smCharInfo->iMonsterEffectID == snCHAR_SOUND_MUTANTTREE ||
			smCharInfo->iMonsterEffectID == snCHAR_SOUND_SUCCUBUS ||
			smCharInfo->iMonsterEffectID == snCHAR_SOUND_BERSERKER ||
			smCharInfo->iMonsterEffectID == snCHAR_SOUND_STYGIANLORD ||
			smCharInfo->iMonsterEffectID == snCHAR_SOUND_AVELIN_ARCHER ||
			smCharInfo->iMonsterEffectID == snCHAR_SOUND_AVELIN_QUEEN ||
			smCharInfo->iMonsterEffectID == snCHAR_SOUND_DMACHINE ||
			smCharInfo->iMonsterEffectID == snCHAR_SOUND_SADNESS ||
			smCharInfo->iMonsterEffectID == snCHAR_SOUND_RUNICGUARDIAN ||
			smCharInfo->iMonsterEffectID == snCHAR_SOUND_BABEL ||
			smCharInfo->iMonsterEffectID == snCHAR_SOUND_HYPERMACHINE ||
			smCharInfo->iMonsterEffectID == snCHAR_SOUND_TURTLE_CANNON ||
			smCharInfo->iMonsterEffectID == snCHAR_SOUND_FROZEN ||
			smCharInfo->iMonsterEffectID == snCHAR_SOUND_CHAOSCARA ||
			smCharInfo->iMonsterEffectID == snCHAR_SOUND_COLDEYE ||
			smCharInfo->iMonsterEffectID == snCHAR_SOUND_M_LORD ||
			smCharInfo->iMonsterEffectID == snCHAR_SOUND_KELVEZU ||
			smCharInfo->iMonsterEffectID == snCHAR_SOUND_DARKGUARD ||
			smCharInfo->iMonsterEffectID == snCHAR_SOUND_DARKMAGE ||
			smCharInfo->iMonsterEffectID == snCHAR_SOUND_FIREWORM ||
			smCharInfo->iMonsterEffectID == snCHAR_SOUND_SETO ||
			smCharInfo->iMonsterEffectID == snCHAR_SOUND_NPC_MAGICMASTER ||
			smCharInfo->iMonsterEffectID == snCHAR_SOUND_NPC_SKILLMASTER ||
			smCharInfo->iMonsterEffectID == snCHAR_SOUND_CASTLE_CRYSTAL_R ||
			smCharInfo->iMonsterEffectID == snCHAR_SOUND_CASTLE_CRYSTAL_G ||
			smCharInfo->iMonsterEffectID == snCHAR_SOUND_CASTLE_CRYSTAL_B ||
			smCharInfo->iMonsterEffectID == snCHAR_SOUND_GOBLINSHAMAN )
		{
			//ranged attack
			if ( len<(460*460) )
				len = 0;

			cnt = 0;

			if ( smCharInfo->iMonsterEffectID==snCHAR_SOUND_WEB )
			{
				lpCurPlayer->iSlowTimeLeft = 70*2;			//Slow movement for 2 seconds
				lpCurPlayer->iPlaySlowSpeed = 180;
			}

			if ( !EventFlag )
			{
				this->dwAttackPlayTime = PLAYTIME + 1100;
				this->XorAttackTrans();
				return TRUE;
			}
		}

		if ( lpTransAttackData->sRating[0] < 0 )
		{
			Param1 = -lpTransAttackData->sRating[0];
			Param2 = lpTransAttackData->sRating[1];

			lpTransAttackData->sRating[0] = 0;
			lpTransAttackData->sRating[1] = 0;
		}
		else
		{
			Param1 = 0;
			Param2 = 0;
		}


		//In the case of the main character monster skin
		if ( lpCurPlayer->sCharacterData.iType == ECharacterType::CHARACTERTYPE_Monster )
			act = 1;
		else
			act = 0;

		//skill attack
		if ( bIsASkillAttack )
		{
			x = ( lpTransAttackData->sPosition.iX - lpCurPlayer->sPosition.iX ) >> FLOATNS;
			y = ( lpTransAttackData->sPosition.iY - lpCurPlayer->sPosition.iY ) >> FLOATNS;
			z = ( lpTransAttackData->sPosition.iZ - lpCurPlayer->sPosition.iZ ) >> FLOATNS;

			if ( ( x * x + y * y + z * z ) > ( 140 * 140 ) )
			{
				cnt = lpTransAttackData->iRange;
			}

			act = 1;			//Monster's skill attack (100% hit)
		}

		int rangeCheck = this->sCharacterData.iType == ECharacterType::CHARACTERTYPE_Player || this->PkMode_CharState == ECharacterType::CHARACTERTYPE_Player ? lpTransAttackData->iRange : ( (smCharInfo->iAttackRange >> 8) * (smCharInfo->iAttackRange >> 8) );

		if ( cnt < rangeCheck &&
			CHARACTERGAME->GetCurrentHP() > 0 &&
			( len < (460 * 460) || act ) &&
			this->DisplayAlpha==0 )
		{
			lpChar = this;


			if ( lpChar )
			{
				if ( CHARACTERGAME->GetEvadeRating( lpChar->sCharacterData.iLevel ) > Dice::RandomI( 0, 99 ) )
				{
					//Additional evasion success
					DAMAGEINFOHANDLER->OnHitReceived( EDamageTextType::Evade ); //SetStringEffect_448FD8
					block = TRUE;
				}

				if ( block == FALSE )
				{
					if ( CHARACTERGAME->GetBlockRating( lpChar->sCharacterData.iLevel ) > Dice::RandomI(0, 99) )
					{
						//Block success
						DAMAGEINFOHANDLER->OnHitReceived( EDamageTextType::Block );
						block = TRUE;

						if ( abs(lpChar->sCharacterData.iLevel-lpCurPlayer->sCharacterData.iLevel) <= AGING_SUB_LEVEL )
						{
							//aging applied to level 7 difference or less
							CALL_WITH_ARG2( 0x508380, SIN_AGING_BLOCK , false); //sinCheckAgingLevel( SIN_AGING_BLOCK , false);
						}

						if ( (lpCurPlayer->dwDispAppSkill & SKILL_APPMASK_SPARK_SHIELD) && lpChar )
						{
							if ( lpCurPlayer->sCharacterData.iClass == ECharacterClass::CHARACTERCLASS_Mechanician )
							{
								ActiveSkill sActiveSkill;
								sActiveSkill.cSkillTinyID = MechanicanSkill::SparkShield;
								sActiveSkill.cLevel = SKILLMANAGERHANDLER->GetLevelSkill( ESkillID::SKILLID_SparkShield );

								SKILLMANAGERHANDLER->SendTransDamage( lpChar, sActiveSkill );
							}

							//Divine Inholation
							if ( lpCurPlayer->sCharacterData.iClass == ECharacterClass::CHARACTERCLASS_Knight )
							{
								cnt = ( lpTransAttackData->iAttackState >> 16 ) & 0xFF;
								if ( cnt > 0 )
								{
									CHARACTERGAME->SetCurrentHP( CHARACTERGAME->GetCurrentHP() + cnt );

									DAMAGEINFOHANDLER->SetDefenseInfo( EDamageTextType::Healing, cnt );

									if ( lpCurPlayer->sLeftHandTool.pModel )
									{
										int x, y, z;
										if ( CALLT_WITH_ARG4( 0x429430, (DWORD)lpCurPlayer, (DWORD)&lpCurPlayer->sLeftHandTool, (DWORD)&x, (DWORD)&y, (DWORD)&z ) )
										{
											Pos1.iX = x;
											Pos1.iY = y;
											Pos1.iZ = z;

											CALL_WITH_ARG1( 0x5B3570, (DWORD)&Pos1 ); //AssaParticle_DivineInhalationDefence( &Pos1 );

											ActiveSkill sActiveSkill;
											sActiveSkill.cSkillTinyID = KnightSkill::DivineShield;
											sActiveSkill.cLevel = SKILLMANAGERHANDLER->GetLevelSkill( ESkillID::SKILLID_DivineShield );
											sActiveSkill.sParam = cnt > 65000 ? 65000 : cnt;

											SKILLMANAGERHANDLER->SendTransDamage( lpChar, sActiveSkill );
										}
									}
								}
							}
						}
					}
				}
			}

			//block rate calculation
			if ( lpChar && ( block == FALSE || act ) )
			{
				if ( sizeof( PacketAttackData ) == lpTransAttackData->iLength )
					cnt = lpTransAttackData->sRating[1];
				else
					cnt = this->sCharacterData.iAttackRating;

				//Monster attempts to hit
				if ( lpCurPlayer->GetMonsterAccuracy(cnt) > Dice::RandomI(0, 99) || act )
				{
					cnt = lpTransAttackData->iDamage;

					if ( (lpCurPlayer->dwDispAppSkill & SKILL_APPMASK_HOLY_BODY) )
					{
						if ( smCharInfo->iMonsterType == EMonsterType::MONSTERTYPE_Undead )
						{
							HolyBody_Undead = TRUE;
						}
					}

					if ( lpCurPlayer->sCharacterData.iClass == ECharacterClass::CHARACTERCLASS_Magician )
					{
						//Confirmation of energy shield application
						temp = lpTransAttackData->iAttackState >> 16;
						if ( temp>0 )
						{
							CALL_WITH_ARG1( 0x5BD9D0, (DWORD)lpCurPlayer ); //AssaParticle_EnergyShieldDefence( lpCurPlayer );
						}
					}

					if (lpCurPlayer->sCharacterData.iClass == ECharacterClass::CHARACTERCLASS_Priestess)
					{
						if (lpChar->sCharacterData.iMonsterType == EMonsterType::MONSTERTYPE_Undead &&
							(lpCurPlayer->dwDispAppSkill & SKILL_APPMASK_SPARK_SHIELD) != 0)
						{
							ActiveSkill sActiveSkill;
							sActiveSkill.cSkillTinyID = PriestessSkill::HolyReflection;
							sActiveSkill.cLevel = SKILLMANAGERHANDLER->GetLevelSkill(ESkillID::SKILLID_HolyReflection);

							SKILLMANAGERHANDLER->SendTransDamage(lpChar, sActiveSkill);
						}
					}

					if ( lpCurPlayer->sCharacterData.iClass == ECharacterClass::CHARACTERCLASS_Priestess )
					{
						//Summon Muspell
						if ( lpChar->sCharacterData.iMonsterType==EMonsterType::MONSTERTYPE_Undead &&
							 (lpCurPlayer->dwDispAppSkill & SKILL_APPMASK_GODLY_SHIELD) != 0 )
						{
							temp = lpTransAttackData->iAttackState>>16;
							if ( temp>0 )
							{
								CHARACTERGAME->SetCurrentHP( CHARACTERGAME->GetCurrentHP() + temp );
							}
						}
					}

					bStunCount = lpCurPlayer->iStunTimeLeft;
					lpCurPlayer->iStunTimeLeft = 0;

					/////////////////// PKMODE ///////////////////
					//@todo HardCPP / Iron for PVP
					//switch( Param1 )
					//{
					//	case SKILL_PLAY_ROAR:
					//	case SKILL_PLAY_SHIELD_STRIKE:
					//		if ( !bStunCount )
					//		{
					//			lpCurPlayer->iStunTimeLeft = lpCurPlayer->GetResistancePower( Param2*70 , EElementID::Organic );
					//			AssaParticle_Stune( lpCurPlayer , Param2+1 );
					//		}
					//		break;
					//
					//	case SKILL_PLAY_GROUND_PIKE:
					//		lpCurPlayer->iSlowTimeLeft = lpCurPlayer->GetResistancePower( Param2*70 , EElementID::Ice );
					//		lpCurPlayer->iPlaySlowSpeed = 180;
					//		lpCurPlayer->iIceOverlay = 1;
					//		break;
					//
					//	case SKILL_PLAY_GLACIAL_SPIKE:
					//	case SKILL_PLAY_ICE_CRYSTAL:		//Siege Ice Tower
					//		lpCurPlayer->iSlowTimeLeft = lpCurPlayer->GetResistancePower( Param2*70 , EElementID::Ice );
					//		lpCurPlayer->iPlaySlowSpeed = 200;
					//		lpCurPlayer->iIceOverlay = 1;
					//		break;
					//
					//	case SKILL_PLAY_DISTORTION:
					//		lpCurPlayer->iSlowTimeLeft = Param2*70;
					//		lpCurPlayer->iPlaySlowSpeed = 210;
					//		lpCurPlayer->bDistortion = lpCurPlayer->iSlowTimeLeft;
					//		lpCurPlayer->iIceOverlay = 0;
					//		break;
					//
					//	case SKILL_PLAY_ENCHANT_WEAPON:
					//		lpCurPlayer->iSlowTimeLeft = lpCurPlayer->GetResistancePower( 100 , EElementID::Ice );
					//		lpCurPlayer->iPlaySlowSpeed = Param2 * 70;
					//		lpCurPlayer->iIceOverlay = 1;
					//		break;
					//
					//	case SKILL_PLAY_DANCING_SWORD:
					//		lpCurPlayer->iSlowTimeLeft = lpCurPlayer->GetResistancePower( 140 , EElementID::Ice );
					//		lpCurPlayer->iPlaySlowSpeed = Param2*70;
					//		lpCurPlayer->iIceOverlay = 1;
					//		break;
					//
					//	case SKILL_PLAY_FROST_JAVELIN:
					//		lpCurPlayer->iSlowTimeLeft = lpCurPlayer->GetResistancePower( 210 , EElementID::Ice );
					//		lpCurPlayer->iPlaySlowSpeed = Param2*70;
					//		lpCurPlayer->iIceOverlay = 1;
					//		break;
					//
					//	//Siege Stupa
					//	case SKILL_PLAY_FIRE_CRYSTAL:
					//	{
					//		cnt = lpCurPlayer->GetResistancePower( cnt, EElementID::Fire );
					//		break;
					//	}
					//		//Siege Electric Tower
					//	case SKILL_PLAY_LIGHTNING_CRYSTAL:
					//	{
					//		if ( !bStunCount )
					//		{
					//			lpCurPlayer->iStunTimeLeft = lpCurPlayer->GetResistancePower( Param2 * 70, EElementID::Lightning );
					//			AssaParticle_Stune( lpCurPlayer, Param2 + 1 );
					//		}
					//		break;
					//	}
					//}

					if ( this->PkMode_CharState == smCHAR_STATE_PLAYER )
						cnt = ( cnt - ( ( lpCurPlayer->sCharacterData.iAbsorbRating * PKMODE_ABSORB ) / 100 ) );
					else
						cnt = ( cnt - lpCurPlayer->sCharacterData.iAbsorbRating );


					if ( cnt<=0 )
						cnt = 1;


					switch( smCharInfo->iMonsterEffectID )
					{
						case snCHAR_SOUND_COLDEYE:
						case snCHAR_SOUND_FROZEN:
						case snCHAR_SOUND_ICEGOLEM:
						{
							if ( bIsASkillAttack )						//skill attack
							{
								lpCurPlayer->iSlowTimeLeft = lpCurPlayer->GetResistancePower( 70 * 4, EElementID::Ice );

								lpCurPlayer->iPlaySlowSpeed = 200;
								lpCurPlayer->iIceOverlay = 1;
							}
							break;
						}

						case snCHAR_SOUND_FROST:
						case snCHAR_SOUND_DEATHKNIGHT:
						{
							///skill attack
							if ( bIsASkillAttack )
							{
								lpCurPlayer->iSlowTimeLeft = lpCurPlayer->GetResistancePower( 70 * 6, EElementID::Ice );

								lpCurPlayer->iPlaySlowSpeed = 200;
								lpCurPlayer->iIceOverlay = 1;
							}
							break;
						}

						case snCHAR_SOUND_CHAOSCARA:
						{
							//Chaos Kara skill
							if ( Param1 == SKILL_PLAY_CHAOSCARA_VAMP )
							{
								CHARACTERGAME->SetCurrentMP( CHARACTERGAME->GetCurrentMP() - cnt );
							}
							break;
						}

						case snCHAR_SOUND_STINGRAY:
						{
							if ( this->pcTarget && bIsASkillAttack )
							{
								//skill attack
								CALL_WITH_ARG2( 0x5B9A90, (DWORD)this->pcTarget, STINGRAY_POSION_TIME ); //ParkAssaParticle_Posion( this->pcTarget, STINGRAY_POSION_TIME );		//Poisonous expression (character)

								if ( this->pcTarget == lpCurPlayer )
								{
									len = lpCurPlayer->GetResistancePower( 70 * STINGRAY_POSION_TIME, EElementID::Poison );
									lpCurPlayer->sPoisonDamage = 1;
									lpCurPlayer->sPoisonCounter = len;
								}
							}
							break;
						}

						case snCHAR_SOUND_KINGBAT:
						{
							if ( this->pcTarget )
							{
								CALL_WITH_ARG2( 0x5B9A90, (DWORD)this->pcTarget, STINGRAY_POSION_TIME ); //ParkAssaParticle_Posion( this->pcTarget, STINGRAY_POSION_TIME );		//Poisonous expression (character)

								if ( this->pcTarget == lpCurPlayer )
								{
									len = lpCurPlayer->GetResistancePower( 70 * STINGRAY_POSION_TIME, EElementID::Poison );
									lpCurPlayer->sPoisonDamage = 1;
									lpCurPlayer->sPoisonCounter = len;
								}
							}
							break;
						}

						case snCHAR_SOUND_DEVIL_BIRD:
						{
							if ( this->pcTarget )
							{
								CALL_WITH_ARG1( 0x5B12C0, (DWORD)this->pcTarget ); //ParkDevilBirdAttackNormal( this->pcTarget );
							}
							break;
						}

						case snCHAR_SOUND_BLOODYKNIGHT:
						{
							if ( this->pcTarget && bIsASkillAttack )
							{
								//skill attack
								CALL_WITH_ARG1( 0x5B1780, (DWORD)this->pcTarget); //ParkAssaParticle_BloodySkillStun( this->pcTarget );
								lpCurPlayer->iStunTimeLeft = 70 * 3;
							}
							break;
						}

						case snCHAR_SOUND_HELLHOUND:
						{
							if ( this->pcTarget && bIsASkillAttack )
							{
								//skill attack
								CALL_WITH_ARG1( 0x5B1800, (DWORD)this->pcTarget); //ParkAssaParticle_HellHoundSkillTaget( this->pcTarget );
							}
							break;
						}

						case snCHAR_SOUND_CHIMERA:
						{
							if ( this->pcTarget && bIsASkillAttack )
							{
								//skill attack
								lpCurPlayer->iStunTimeLeft = 70 * 3;
							}
							break;
						}

					}


					//////////////////////////////////////////////

					//absorption rate calculation


					if ( abs(lpChar->sCharacterData.iLevel - lpCurPlayer->sCharacterData.iLevel) <= AGING_SUB_LEVEL )
					{
						//aging applied to level 7 difference or lower
						//	sinCheckAgingLevel( SIN_AGING_DEFENSE , false);
						CALL_WITH_ARG2( 0x508380, SIN_AGING_DEFENSE_ARMOR, false );		//sinCheckAgingLevel( SIN_AGING_DEFENSE_ARMOR, false);
						CALL_WITH_ARG2( 0x508380, SIN_AGING_DEFENSE_ORB, false );		//sinCheckAgingLevel( SIN_AGING_DEFENSE_ORB, false);
						CALL_WITH_ARG2( 0x508380, SIN_AGING_DEFENSE_BOOTS, false );		//sinCheckAgingLevel( SIN_AGING_DEFENSE_BOOTS , false);   //Park Jae-won-Aging item added (boots)
						CALL_WITH_ARG2( 0x508380, SIN_AGING_DEFENSE_GLOVES, false );	//sinCheckAgingLevel( SIN_AGING_DEFENSE_GLOVES , false);  //Park Jae-won-Aging item added (gloves)
						CALL_WITH_ARG2( 0x508380, SIN_AGING_DEFENSE_ARMLETS, false );	//sinCheckAgingLevel( SIN_AGING_DEFENSE_ARMLETS , false); //Jaewon Park-Added Aging Item (Amrit)
					}

					reinterpret_cast<int(*)()>(0x45BC90)();  /// CheckCharForm();

					CHARACTERGAME->SetCurrentHP( CHARACTERGAME->GetCurrentHP() - cnt );
					DAMAGEINFOHANDLER->OnHitReceived( EDamageTextType::Normal, cnt );

					if ( smCharInfo->iMonsterEffectID == snCHAR_SOUND_SUCCUBUS )
					{
						if ( lpTransAttackData->sRating[0] > 75 )
						{
							//Succubus
							if ( lpTransAttackData->sRating[0] < 80 )
							{
								temp = ( ( lpCurPlayer->sCharacterData.sSP.sMax * 6 ) / 100 );
								CHARACTERGAME->SetCurrentSP( CHARACTERGAME->GetCurrentSP() - temp );
							}

							//Incubus
							else
							{
								temp = ( ( lpCurPlayer->sCharacterData.sMP.sMax * 6 ) / 100 );
								CHARACTERGAME->SetCurrentMP( CHARACTERGAME->GetCurrentMP() - temp );
							}
						}
					}

					//Blood splashing particles
					SKILLMANAGERHANDLER->StartSkillEffectOld( lpCurPlayer->sPosition.iX, lpCurPlayer->sPosition.iY + 16 * fONE, lpCurPlayer->sPosition.iZ, EFFECT_DAMAGE1 );
					SKILLMANAGERHANDLER->SetDynLight( lpCurPlayer->sPosition.iX, lpCurPlayer->sPosition.iY + 16 * fONE, lpCurPlayer->sPosition.iZ, 160, 0, 0, 0, 180 );

					if ( ( (lpCurPlayer->dwDispAppSkill&SKILL_APPMASK_PHYSICAL_ABSORB) && (lpCurPlayer->dwDispAppSkill&SKILL_APPMASK_HOLY_BODY)==0 ) || HolyBody_Undead )
					{
						//Fiji Clup Soap applied effect
						SKILLMANAGERHANDLER->StartSkillEffectOld( lpCurPlayer->sPosition.iX, lpCurPlayer->sPosition.iY + 24 * fONE, lpCurPlayer->sPosition.iZ, EFFECT_PHYSICAL_ABSORB_DAMAGE );
					}

					if ( cnt > 10  &&
						 lpCurPlayer->GetAnimation() != EAnimationType::ANIMATIONTYPE_Flinch &&
						 lpCurPlayer->GetAnimation() != EAnimationType::ANIMATIONTYPE_Potion &&
						 lpCurPlayer->GetAnimation() != EAnimationType::ANIMATIONTYPE_Attack &&
						 lpCurPlayer->GetAnimation() != EAnimationType::ANIMATIONTYPE_Skill)
					{
						if ( this->sCharacterData.iLevel > 100 )
						{
							//100% chance
							lpCurPlayer->Animate( EAnimationType::ANIMATIONTYPE_Flinch );
							PlayUnitSound( lpCurPlayer );
						}
						else if ( this->sCharacterData.iLevel > 50 )
						{
							//75% chance
							if ( Dice::RandomI( 0, 99 ) > 25 )
							{
								lpCurPlayer->Animate( EAnimationType::ANIMATIONTYPE_Flinch );
								PlayUnitSound( lpCurPlayer );
							}
						}
						else
						{
							//50% chance
							if ( Dice::RandomI( 0, 99 ) > 50 )
							{
								lpCurPlayer->Animate( EAnimationType::ANIMATIONTYPE_Flinch );
								PlayUnitSound( lpCurPlayer );
							}
						}

					}



					//Temporary processing
					if ( CHARACTERGAME->GetCurrentHP() <= 0 )
					{
						if ( CHANGEJOB4_CODE == SIN_QUEST_CODE_CHANGEJOB4 &&
							lpCurPlayer->iLoadedMapIndex >= 0 &&
							FIELDGAME[ lpCurPlayer->iLoadedMapIndex ]->iMapType == QUEST_ARENA_FIELD ) {

							//Quest Battleground
							PrepareUnitDataHUD( this );

							CALL_WITH_ARG1( 0x4D9B90, 0 ); //sinBattleNpcResult( 0 ); Lost by the 3rd full-time master

							CHARACTERGAME->SetCurrentHP( 0 );
							lpCurPlayer->Animate( EAnimationType::ANIMATIONTYPE_Die );
							PlayUnitSound( lpCurPlayer );
							ResetAttackTrans();
							return TRUE;
						}

						lpCurPlayer->Animate( EAnimationType::ANIMATIONTYPE_Die );
						CHARACTERGAME->SetCurrentHP( 0 );

						PlayUnitSound( lpCurPlayer );

						CALL_WITH_ARG5(0x623400, 0x50320E00 , this->iID ,0,0,0 );  //SendCommand_AreaServer( smTRANSCODE_DEAD_USER , dwObjectSerial ,0,0,0 );
						CALL_WITH_ARG5(0x623460, 0x50320E00 , this->iID ,1,0,0 );  //SendCommand_DataServer( 0x50320E00 , this->iID ,1,0,0 );

						this->iEnchantWeaponType = 0;
						this->dwEnchantWeaponTimeOut = 0;

						if ( this->sCharacterData.iType != ECharacterType::CHARACTERTYPE_Player )
						{
							UNITGAME->OnLoseEXP(); //DeadPlayerExp( lpCurPlayer );				//Experience reduced due to death (except for PK)
						}

						CALL( 0x4D92F0 ); //	sinCheckQuest85Die(); 0 If you die while performing a quest


						TIMERSKILLHANDLER->OnCharacterKilled();

						CHARACTERGAME->Save();

						if ( this->sCharacterData.iType == ECharacterType::CHARACTERTYPE_Player )
						{
							CALL_WITH_ARG1( 0x6288F0, this->iID ); //PlayerKilling( dwObjectSerial );
						}

						ResetAttackTrans();	//lpTransAttackData->code = 0;

						CALL_WITH_ARG1( 0x40B1E0, 0 ); //SetFullZoomMap(0);	//To prevent opening the map when pluto died


						return TRUE;
					}

					reinterpret_cast<void(*)()>(0x45BC70)();  //ReformCharForm(); - Character information authentication
				}
				else
				{
					//Evasion (defense) success
					DAMAGEINFOHANDLER->OnHitReceived( EDamageTextType::Defense );
				}
			}

			BATTLETIME = PLAYTIME;			//Battle time log
		}
	}

	ResetAttackTrans();	//lpTransAttackData->code = 0;
#endif

	return TRUE;
}


NAKED BOOL UnitData::Render()
{
	JMP( pfnRender );
}


NAKED void UnitData::Free()
{
	JMP( pfnCloseUnitData );
}

//See //Move (precision)
//int smCHAR::MoveAngle2( int step )
NAKED void UnitData::MoveAngle( int iDistance )
{
	JMP( pfnMoveAngle );
}
NAKED BOOL UnitData::CheckShootingTest ( UnitData * pcTarget )
{
	JMP ( pfnCheckShootingTest );
}

void Unit::Init( Unit * pcUnit, UnitData * pcUnitData )
{
	ZeroMemory( pcUnit, sizeof( Unit ) );
	pcUnit->iID = pcUnitData->iID;
	pcUnit->pcUnitData = pcUnitData;

	CopyMemory( &pcUnit->sLastRunningPosition, &pcUnitData->sPosition, sizeof( Point3D ) );
}


void Unit::Load()
{
	iID						= pcUnitData->iID;


	sLastRunningPosition	= Point3D( 0, 0, 0 );

	dwTimePositionUpdate	= 0;

	uLastUpdate				= 0;

#if defined(_GAME)
	cMutex					= new std::mutex();


	sPlayerAuraId = 0;
	sDialogSkinId = 0;


	iEvadeRating = 0;

	bInvisibleInMiniMap = FALSE;
	sBellatraSoloCrown = 0;

	bPartyLeader			= false;

#else


	pcUnitData->pcUnit			= this;


	mapLastDamageReceived.clear();

	bCursed = FALSE;

	bHealItself					= FALSE;
	bReturningToSpawn			= FALSE;
	bHasTargetFocus				= FALSE;
	dwReturningToSpawnTimeout	= 0;
	dwLastFocusOrAttackTime		= 0;
	iIdleWithFocusCounter		= 0;
	dwLastNonIdleTime			= 0;
	dwLastWalkOrRunPos			= Point3D ( 0, 0, 0 );
	iStuckCounter				= 0;
	sReturningToSpawnPosition	= Point3D ( 0, 0, 0 );
	bTestMap					= FALSE;

	sPoisonDamagePerStack		= 0;
	dwPoisonNextTime			= 0;
	bPoisonStackCount			= 0;
	bPoisonCount				= 0;
	idPoisonCastedBy			= 0;

	sBurningDamagePerTick		= 0;
	dwBurningNextTime			= 0;
	bBurningCount				= 0;
	idBurningCastedBy			= 0;


#endif

	//Skills
	bFierceWind				= FALSE;

	bLethalSight			= FALSE;

	bHolyConviction			= FALSE;

	bWizardTrance			= FALSE;

	bVirtualLifeLevel1Based			= 0;
	bVirtualRegenLevel1Based		= 0;
	bDemonsBaneLevel1Based			= 0;
	bHallOfValhallaLevel1Based		= 0;
	bTriumphOfValhallaLevel1Based	= 0;
	bAlasLevel1Based				= 0;
	bInpesLevel1Based				= 0;
	bAdventMigalLevel1Based			= 0;

	bHolyValorLevel1Based			= 0;
	bHolyBodyLevel1Based			= 0;
	bDrasticSpiritLevel1Based		= 0;
	bElectricalOverloadLevel1Based	= 0;



	bPartyLeader			= false;
}

void Unit::Load( UnitData * pcUnitData )
{
	UnLoad ();

	this->pcUnitData = pcUnitData;

	Load();
}

void Unit::UnLoad ()
{
#ifdef _GAME
	if ( vDamageInfoList.size () > 0 )
	{
		for ( auto &v : vDamageInfoList )
		{
			SAFE_DELETE ( v ); //free pointer
		}

		vDamageInfoList.clear();
	}
#else
	if ( this->pcUnitData )
	{
		this->pcUnitData->pcUnit = NULL;
	}
#endif
}

UINT Unit::GetArrayPosition( UnitData * pcUnitData )
{
	UINT u = -1;

#if defined(_SERVER)
	UINT uaUnits = *(DWORD*)0x7AC3E78;
	u = ((UINT)pcUnitData - uaUnits) / 0x4B3C;
#endif

	return u;
}

int Unit::GetMapLevel()
{
	if ( pcUnitData->pcMap == NULL )
		return 0;

	return pcUnitData->pcMap->pcBaseMap->iLevel;
}

EMapID Unit::GetMapID()
{
	if ( pcUnitData->pcMap == NULL )
		return MAPID_Invalid;

	return pcUnitData->pcMap->pcBaseMap->iMapID;
}
