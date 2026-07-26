#include "stdafx.h"
#include "DLL.h"
#include <ShellAPI.h>



extern void __cdecl		ItemCreatorInitHook();
extern GameCore			* pGameCoreHandler;

extern void __cdecl SendNpcUnitClick ( UnitData * pcUnitData, int iState );
extern BOOL __cdecl SendSingleDamageUnit( UnitData * pcUnitData, int iDamageMin, int iDamageMax, int iState, int iResistance, int iSkillID, BOOL bChance, SocketData * pcSocketData );
extern BOOL __cdecl	SendAreaDamageUnit( int iX, int iY, int iZ, UnitData * pcUnitData, int iDamageMin, int iDamageMax, int iState, int iResistance, int iSkillID );

extern BOOL __cdecl RenderUnitData( UnitData * pcUnitData );
extern BOOL __cdecl FreeUnitData( UnitData * pcUnitData );

void __cdecl ReOrderVolume()
{
	if( SETTINGSHANDLER->GetModel()->GetSettings().bNoMusic )
		SOUNDHANDLER->SetVolumeMusic( 0 );

	if( SETTINGSHANDLER->GetModel()->GetSettings().bNoSoundEffects )
		SOUNDHANDLER->SetVolumeEffect( 0 );
	else
		SOUNDHANDLER->SetVolumeEffect( SETTINGSHANDLER->GetModel()->GetSettings().iEffectVolume );
}

void __cdecl LoadMusic( int iNum )
{
	SOUNDHANDLER->LoadMusic( iNum );
}

BOOL __cdecl ChangeMusic( int iID )
{
	return SOUNDHANDLER->ChangeBackgroundMusic( iID );
}

void __cdecl DrawTextGameHook( int FontObject, int x, int y, const char * Text, int isize, UINT uFormat, D3DCOLOR Color )
{
	RECT font_rect;
	SetRect( &font_rect, x, y, *(UINT*)0x3A9A30C + x, *(UINT*)0x3A9A310 + y );
	FONTHANDLER->Draw( FontObject, font_rect, Text, isize, uFormat, Color );
}

BOOL bFirstGetTextUsername = FALSE;

void WINAPI GetEditTextW( HWND hwnd, LPSTR lpText, INT Count )
{
	if( !hwnd )
	{
		return;
	}

	GetWindowTextA(hwnd, lpText, Count);

	if (bFirstGetTextUsername)
	{
		return;
	}

	int iCount = 0;
	switch ((DWORD)lpText)
	{
		case (DWORD)ELoginScreenInputBox::USERNAME:
			STRINGCOPYLEN(lpText, Count, SETTINGSHANDLER->GetModel()->GetSettings().szLastAccount);
			SetWindowTextA(hwnd, lpText);
			iCount = lstrlenA(lpText);
			SendMessageA(hwnd, EM_SETSEL, iCount, iCount);
			bFirstGetTextUsername = TRUE;
			break;
	}
}

bool __cdecl SummonCrystal()
{
	return ITEMHANDLER->DelayCristal();
}

BOOL __cdecl OnNpcQuestClick( DWORD dwCode )
{
	return QUESTGAMEHANDLER->OnClickNpc( dwCode );
}

void __cdecl OnClickOKQuestMessageBox()
{
	QUESTGAMEHANDLER->OnClickOkMessageBox( UIEventArgs{} );
}

BOOL __cdecl OnClickQuestWindow( DWORD dwQuestID )
{
	return QUESTGAMEHANDLER->OnClickQuestWindow( dwQuestID );
}

BOOL __cdecl OnDescriptionItemMixType( Item * psItemInfo )
{
	return TRUE;
}

BOOL __cdecl OnAddColorDescription( int iLine )
{
	return TRUE;
}

void __stdcall DrawTextDXHook( int FontObject, int x, int y, LPCTSTR Text, int size )
{
	std::string pText = Text;
	DrawTextGameHook( 0, x, y, Text, size, DT_WORDBREAK, *(UINT*)0x3A9A32C );
}

BOOL __cdecl IsDebuffSkill( UnitData * pcUnit, DWORD dwSkillCode )
{
	return SKILLMANAGERHANDLER->IsDebuffSkill( pcUnit, dwSkillCode );
}

BOOL __cdecl OnReceivePacket( void * pPacket )
{
	return FALSE;
}

void __cdecl OnLoadArmorCharacter()
{
//	ITEMHANDLER->OnLoadArmorCharacter();
}

BOOL __cdecl OnSetArmorCurrentUnit( UINT uItemCode )
{
	return ITEMHANDLER->OnSetArmorUnit( UNITDATA, uItemCode );
}

BOOL __cdecl OnSetArmorTargetUnit( UnitData * psUnit, UINT uPosition )
{
	return ITEMHANDLER->OnSetArmorTargetUnit( psUnit, uPosition );
}

int __cdecl GetAbsorptionPlayer()
{
	return UNITGAME->GetCharacterAbsorption();
}

void __cdecl OnCharacterUpdateData()
{
	CHARACTERGAME->OnCharacterUpdateData();
}

//when attacked
int __cdecl DefendChance( int iDefenseRating, int iAttackRating )
{
	//no longer used
	return 100; // return UNITGAME->DefendChance( iDefenseRating, iAttackRating, TRUE );
}

//when attacked
int __cdecl BlockChance( int iBlockRating, int iBlock )
{
	return UNITDATA->sCharacterData.iBlockRating;
}

int __cdecl GetWidthNormalFont( const char * Text )
{
	return FONTHANDLER->GetWidthText( 0, Text, lstrlenA( Text ) );
}

int __stdcall FormatSkillTimerInfo( struct Skill * pSkill )
{
	return SKILLMANAGERHANDLER->FormatSkillTimerInfo( pSkill );
}

INT64 __cdecl OnLoseExp()
{
	return UNITGAME->OnLoseEXP();
}

void __cdecl SetCharacterFormula( ItemData * pItem, InventoryItemData * pInventory, int * iLevelDamage, int * iSkillDamage, int * iAttackDamage )
{
	UNITGAME->SetCharacterFormula( pItem, pInventory, iLevelDamage, iSkillDamage, iAttackDamage );
}

void __cdecl OnDrawUI()
{
	UIHANDLER->Render();
}

BOOL __stdcall OnSendPacket( void * pPacket, BOOL bEncrypt )
{
	SocketData * sd = NULL;
	READTHIS( sd );
	if( sd )
		sd->SendPacket( (Packet*)pPacket, bEncrypt );
	return TRUE;
}

SocketData * __cdecl OnConnect( const char * pszIP, int iPort )
{
	return SOCKETGAME->Connect( pszIP, iPort );
}

void __cdecl OnSocketPacket( SocketData * sd, void * pPacket )
{
//	SOCKETGAME->SocketPacket( sd, (PacketReceiving*)pPacket );
}

void __stdcall OnSocketClose()
{
	SocketData * sd = NULL;
	READTHIS( sd );
	if( sd )
		SOCKETGAME->SocketClose( sd );
}

void __cdecl OnCharacterSetCharInfoEx()
{
	CHARACTERGAME->OnCharacterSetCharInfoEx();
}

void __cdecl OnWarpCastle( BOOL bLeader, int * piX, int * piZ )
{
	BLESSCASTLEHANDLER->WarpCastle( bLeader, piX, piZ );
}

int __cdecl OnClickPremium( ItemData * pcItemData )
{
	return ITEMHANDLER->OnUseItem( pcItemData );
}

int __cdecl LoadMaps()
{
	MAPGAME->Load();
	return 1;
}

void __cdecl OnLoginDraw()
{
	LOGINHANDLER->OnRender();
}

void __cdecl RenderSubServerBox()
{
	LOGINHANDLER->RenderServerSelectBox();
}

void __cdecl RenderBGLogin()
{
	LOGINHANDLER->RenderBGLogin();
}

char * __cdecl OnDescriptionItem( DWORD dwCode )
{
	char * pszDescription = NULL;
	return pszDescription;
}

int __stdcall ChangeMotion( int iMotion, int iAnimation )
{
	void * pptr = NULL;
	READTHIS( pptr );

	return CHARACTERGAME->OnMotionChange( (UnitData*)pptr, iMotion, iAnimation );
}

BOOL __cdecl ResetAndRecreateDevices()
{
	GRAPHICENGINE->sResolution.uWidth -= 1;
	return GRAPHICENGINE->Reset( SETTINGSHANDLER->GetModel()->GetSettings().bWindowed, SETTINGSHANDLER->GetModel()->GetSettings().dwWidth, SETTINGSHANDLER->GetModel()->GetSettings().dwHeight, TRUE );
}

void __cdecl OnLostDevice()
{
	GRAPHICENGINE->OnLostDevice();
}

void __cdecl Begin3D()
{
	GRAPHICENGINE->Begin3D();
}

void __cdecl End3D()
{
	GRAPHICENGINE->End3D();
}

void __cdecl ShowExpValueFormat( char * szBuf, int expType )
{
	// Retorna o valor da exp nos dados do char com virgulas ( ex: 1,000 )
	if( expType == 0 )
		wsprintfA( szBuf, "%sM", FormatNumber( CALL( 0x00497AB0 ) ) );
	else if( expType == 1 )
		wsprintfA( szBuf, "%s", FormatNumber( CALL( 0x00497AB0 ) ) );
	else if( expType == 2 )
		wsprintfA( szBuf, "%sM", FormatNumber( CALL( 0x00497AE0 ) ) );
	else if( expType == 3 )
		wsprintfA( szBuf, "%s", FormatNumber( CALL( 0x00497AE0 ) ) );
	else if( expType == 4 )
		wsprintfA( szBuf, "EXP     : %sM", FormatNumber( CALL_WITH_ARG4( 0x00768140, READDWORD( 0x020802E8 ), READDWORD( 0x020802E8 + 0x48 ), 0x0F4240, 0 ) ) );
	else if( expType == 5 )
	{
		UINT expv1 = *(UINT*)0x020802E8;
		if( expv1 > 999999 )
		{
			expv1 = expv1 / 1000000;
			wsprintfA( szBuf, "EXP     : %sM", FormatNumber( expv1 ) );
		}
		else
			wsprintfA( szBuf, "EXP     : %s", FormatNumber( expv1 ) );
	}
	// Gold
	else if( expType == 6 )
		wsprintfA( szBuf, "Gold    : %s", FormatNumber( READDWORD( 0x020802F0 ) ) );
}

void __cdecl OnSendPlayDataEx()
{
	UNITGAME->SendUnitDataEx();
}

void __cdecl OnReceiveCommand( char * pszBuff )
{
	CHATGAMEHANDLER->AnalyzeChat( pszBuff );
}

/// <summary>
/// This gets called whenever the current player receives a hit.
/// The type could be: normal (damage received), or evade, block, defense etc
/// 2012 src: SetStringEffect
/// </summary>
void __stdcall OnHitReceived( int iType, int iValue )
{
	DAMAGEINFOHANDLER->OnHitReceived( (EDamageTextType)iType, iValue );
}

int __cdecl OnItemPivot( int iPosition, UINT uItemID )
{
	return ITEMHANDLER->OnItemPivot( iPosition, uItemID );
}

extern BOOL bShowFps;
extern BOOL	bDebug;

void __cdecl DrawGMDebugCharacterInfo( int iDebug )
{
	//Render Character Debug Text
	if( iDebug == 1 && GM_MODE && *(int*)0x35E11D8 != 0 && DEBUG_GM )
		UNITGAME->RenderCharacterDebugText();
}

void __cdecl ExitGame()
{
	INI::CReader cReader( "game.ini" );

	if ( (char *)0x39033E8 )
	{
		cReader.WriteString ( "Game", "LastAccount", (char *)0x39033E8 );
	}
}

void __cdecl OnDrawUILogin()
{
	//CALL( 0x0059E7A0 ); //chaos
	//GameCore::GetInstance()->Render2D(); //chaos
}

BOOL __cdecl BeginSkill( UnitData * pcThis, int iSkillID, int iLevel, UnitData * pcUnitData, int iX, int iY, int iZ )
{
	SKILLMANAGERHANDLER->BeginSkill( pcThis, iSkillID, iLevel, pcUnitData, iX, iY, iZ );

	return TRUE;
}

/// <summary>
/// Leaked: int PlaySkillAttack( sSKILL *lpSkill , smCHAR *lpChar )
/// </summary>
bool __cdecl PlaySkillAttack( struct Skill * psSkill, class UnitData * pcUnitData )
{
	if ( psSkill == NULL )
		return false;

	if ( psSkill->UseSkillFlag )
	{
		SKILLMANAGERHANDLER->ResetBuff ( psSkill->iID, pcUnitData );
	}

	int result = SKILLMANAGERHANDLER->PlaySkillAttack( psSkill, pcUnitData );

	//yes - we do need to return '2' so that
	//basic attack can do damage. See SKILLMANAGERHANDLER->PlaySkillAttack
	if ( result == 2 )
		return false;

	if ( result == 1 )
		return true;

	bool bRes = false;
	__asm
	{
		PUSH pcUnitData;
		PUSH psSkill;
		MOV EAX, 0x004160D0;
		CALL EAX;
		ADD ESP, 0x08;
		MOV bRes, AL;
	}
	return bRes;
}

/// <summary>
/// If returning TRUE then the skill is only casted once.
/// Returning FALSE will allow OnPlaySkill to be casted multiple times.
/// </summary>
BOOL __cdecl OpenPlaySkill( struct Skill * psSkill )
{
	//CHATBOX->AddDebugMessage ( "__cdecl OpenPlaySkill" );

	if ( psSkill && psSkill->UseSkillFlag )
	{
		if ( UNITDATA->pcAttackTarget == NULL )
		{
			SKILLMANAGERHANDLER->ResetBuff ( psSkill->iID, UNITDATA );
		}
	}

	BOOL result = SKILLMANAGERHANDLER->OpenPlaySkill ( psSkill );

	if (result == TRUE )
		return TRUE; //returning TRUE will 'break' out of the right button down logic in Winmain.cpp

	if ( result == 2 )
		return FALSE; //returning FALSE will not 'break' out of the right button down logic in Winmain.cpp

	BOOL bRes = FALSE;
	__asm
	{
		PUSH psSkill;
		MOV EAX, 0x00415F80;
		CALL EAX;
		ADD ESP, 0x04;
		MOV bRes, EAX;
	}

	return bRes;
}

/// <summary>
/// This function is called during player or monster attack
/// </summary>
BOOL __cdecl EventSkill( class UnitData * pcThis )
{
	//In pvp mode, the player would be classied as monster
	//to determine if it's a 'player' monster (so you can attack it etc), check PkMode_CharState.
	//If it's not a player, then it's a monster for sure.
	if ( pcThis->sCharacterData.iType == CHARACTERTYPE_Monster &&
		 pcThis->PkMode_CharState != CHARACTERTYPE_Player)
	{
		if ( SKILLMANAGERHANDLER->EventSkillMonster( pcThis ) )
			return TRUE;
	}

	//players only!
	if ( pcThis->sCharacterData.iType == CHARACTERTYPE_Player ||  pcThis->PkMode_CharState == CHARACTERTYPE_Player )
	{
		if ( SKILLMANAGERHANDLER->EventSkill( pcThis ) )
			return TRUE;
	}

	//this in turn will call EventSkill_Monster() in ASM for other monster effects..
	//and for other skills..
	BOOL bRes = FALSE;
	__asm
	{
		MOV ECX, pcThis;
		MOV EAX, 0x004386F0;
		CALL EAX;
		MOV bRes, EAX;
	}

	//players only!
	if ( pcThis->sCharacterData.iType == CHARACTERTYPE_Player || pcThis->PkMode_CharState == CHARACTERTYPE_Player )
	{
		SKILLMANAGERHANDLER->EventSkillPostASM( pcThis );
	}

	return bRes;
}

BOOL __cdecl EndSkill( class UnitData * pcThis )
{
	if( SKILLMANAGERHANDLER->EndSkill( pcThis ) )
		return TRUE;
	__asm
	{
		MOV ECX, pcThis;
		MOV EAX, 0x004250F0;
		CALL EAX;
	}
	return TRUE;
}



BOOL __stdcall PrepareSkillInfo( Skill * psSkill, SkillBox * psSkillBox, int iIcon )
{
	int iOffset = 2;

	//Make a copy of the skill so that the original
	//skill stays unaffected.
	Skill * pSkillCopy = new Skill ();
	CopyMemory ( pSkillCopy, psSkill, sizeof ( Skill ) );

	int skillInfoOffset = SKILLINFOBOX->GetSkillInfoLevelOffset ();

	int SkillButtonIndex = *(int *)0x367DF88;

	//force preview of unlearnt skill
	if ( pSkillCopy->iLevel == 0 && skillInfoOffset == 0 && SkillButtonIndex != 0)
	{
		skillInfoOffset = 1;
	}

	if ( pSkillCopy->iLevel + skillInfoOffset < 1 ||
		 pSkillCopy->iLevel + skillInfoOffset > 10 )
	{
		skillInfoOffset = 0;
		SKILLINFOBOX->Invalidate();
	}

	//get adjusted level of skill
	pSkillCopy->iLevel = pSkillCopy->iLevel + skillInfoOffset;


	//this is the formula used to determine the skill cool down value.
	//UseSkillMastery is usually 10,000 if skill is fully trained / matured. When that's the case then 10000 / 100 = 100
	//So you could have
	// 250 + (3 * 10) - (10000 / 100)
	// 250 + (30) - (100)
	// 130 for Mastery, which is then capped at 70

	pSkillCopy->Mastery = pSkillCopy->sSkillInfo.iSkillMasteryBase + pSkillCopy->sSkillInfo.iSkillMasteryPerLevel * pSkillCopy->iLevel - ( pSkillCopy->UseSkillMastery / 100 );
	if ( pSkillCopy->Mastery <= 0 ) pSkillCopy->Mastery = 1;
	if ( pSkillCopy->Mastery > MAX_MASTERY ) pSkillCopy->Mastery = MAX_MASTERY;

	if ( SkillButtonIndex != 0 && SKILLINFOBOX->IsDifferentSkill ( pSkillCopy ) )
	{
		skillInfoOffset = psSkill->iLevel == 0 ? 1 : 0;
	}

	SKILLINFOBOX->SetSkillInfoLevelOffset ( skillInfoOffset );

	if( SKILLINFOBOX->CanPrepareShowSkill( pSkillCopy, Rectangle2D( psSkillBox->BoxRect.left, psSkillBox->BoxRect.top, psSkillBox->BoxRect.right, psSkillBox->BoxRect.bottom ) ) )
	{
		CALLT_WITH_ARG3( 0x004EDC90, 0x0367E148, (DWORD)pSkillCopy, (DWORD)psSkillBox, iIcon );
		SKILLDATAHANDLER->OnInformationHandler( pSkillCopy, psSkill->iLevel );

		SKILLINFOBOX->PrepareShowSkill( pSkillCopy, FALSE, Rectangle2D( psSkillBox->BoxRect.left, psSkillBox->BoxRect.top, psSkillBox->BoxRect.right, psSkillBox->BoxRect.bottom ) );
	}

	SAFE_DELETE ( pSkillCopy );

	return TRUE;
}

BOOL __stdcall PrepareItemInfo( ItemData * psItemData, int iFlag, int iIndex )
{ // 004C9370
	if( !psItemData )
		return FALSE;

	if( ITEMINFOBOX->IsViewingPerfectItem() )
		return FALSE;

	int iOriginalX = psItemData->sPosition.iX;
	int iOriginalY = psItemData->sPosition.iY;

	BOOL bShowBuyPrice = FALSE, bShowSellPrice = FALSE, bShowChecksums = FALSE;

	bShowBuyPrice = iFlag;

	if( iFlag == 3 )
		bShowSellPrice = TRUE;

	if (ITEMINFOBOX->IsViewingPerfectItem())
		return FALSE;

	if( ITEMINFOBOX->IsSameItem(psItemData, bShowBuyPrice, bShowSellPrice) == FALSE )
	{
		//DEBUG("ITEMINFOBOX->IsSameItem(psItemData, bShowBuyPrice, bShowSellPrice) == FALSE");

		if( ITEMINFOBOX->PrepareShowItem( psItemData, bShowBuyPrice, bShowSellPrice, bDebug ) )
			return TRUE;
	}

	return FALSE;
}

int __stdcall TMAnimationHook( int iFrame, int iAngleX, int iAngleY, int iAngleZ, struct FrameInfoData * psFrameInfo )
{
	void * pcThis;
	READTHIS( pcThis );
	ANIMATIONHANDLER->AddAnimationObject( pcThis, iFrame, iAngleX, iAngleY, iAngleZ, psFrameInfo );
	return 1;
}

struct EXEModelData * __stdcall ReadInx( char * pszFile )
{
	return Game::ReadInx( pszFile );
}

BOOL __cdecl IsTradeUse( int iItemID )
{
	return ITEMWINDOWHANDLER->IsTradeUse( iItemID );
}

BOOL __stdcall MixItem( class ItemData * pcItemData, BOOL bFlag )
{
	return MIXHANDLER->SetItemMix( pcItemData, bFlag );
}

BOOL __cdecl IsUseMix( int iItemID )
{
	if ( (iItemID & 0xFFFF0000) == 0x03070000 )
	{
		if ( (iItemID & 0xFFFFFF00) == ITEMID_SpecStoneCode )
		{
			TITLEBOX( "Use craft npc to respec with this stone!" );
			return FALSE;
		}
		return TRUE;
	}
	return ITEMWINDOWHANDLER->IsUseMixNPC( iItemID );
}

BOOL __stdcall SetItemToNPC( class ItemData * pcItemData, BOOL bFlag )
{
	return ITEMWINDOWHANDLER->SetItemToNPC( pcItemData, bFlag );
}

BOOL __stdcall CharacterCanEquipItem( class ItemData * pcItemData )
{
	void * p;
	READTHIS( p );
	ITEMHANDLER->CanEquipItem( p, pcItemData );
	return TRUE;
}

void __cdecl ForceOrbDraw( class UnitData * pcUnitData, Point3D * p, Point3D * pa, void * psFace )
{
//	FORCEORBHANDLER->Draw( pcUnitData, p, pa, psFace );
}

void __cdecl ForceOrbMain( class UnitData * pcUnitData )
{
	FORCEORBHANDLER->Main( pcUnitData );
}

void __stdcall DrawClanImage( void * pClanMark, char * pszClanName, int iX, int iY, BOOL bFrame )
{
}

void __stdcall OpenGuideBoxLevelUP( int iLevel, int iClass )
{
	//CHATBOX->AddMessage ( FormatString ( "You are now level %d!", iLevel ) );
	//SAVE;

	CGuideGame * p;
	READTHIS( p );
	p->guide_handle( iLevel, (ECharacterClass)iClass );
}

BOOL __stdcall OnClickItemSlotPut( ItemData * pcItem )
{
	return MANUFACTUREHANDLER->OnClickItemSlotPut( pcItem );
}

BOOL __stdcall OnCheckCanPutStoneSlotSocket( ItemData * pcItemData, BOOL bGem )
{
	return MIXHANDLER->CanPutStoneSlotSocket( pcItemData, bGem );
}

BOOL __stdcall OnCheckCanPutItemSlotSocket( ItemData * pcItemData, BOOL bGemNPC )
{
	return MIXHANDLER->CanPutItemSlotSocket( pcItemData, bGemNPC );
}

void __cdecl OnUseManaPotion( int iMP )
{
	CHARACTERGAME->OnUseManaPotion( iMP );
}

void __cdecl OnUseHPPotion( int iHP )
{
	CHARACTERGAME->OnUseHPPotion( iHP );
}

BOOL __stdcall AnimateModelGate( int iFrame, int iAngleX, int iAngleY, int iAngleZ, EXEFrame * pFrameInfo )
{//004769D0 (0x04AF818C)
	EXEModel * p;
	READTHIS( p );

	if ( p )
		return p->Animate( iFrame, iAngleX, iAngleY, iAngleZ, pFrameInfo );

	return TRUE;
}

BOOL __stdcall AnimateModelGate2( int iFrame, int iAngleX, int iAngleY, int iAngleZ, EXEModel * pcModelTalk, EXEFrame * pFrameInfo )
{//004769D0 (0x04AF818C)
	EXEModel * p;
	READTHIS( p );

	if ( p )
		return p->Animate( iFrame, iAngleX, iAngleY, iAngleZ, pcModelTalk, pFrameInfo );

	return TRUE;
}

BOOL __stdcall AnimateMeshGate( int iFrame, int iAngleX, int iAngleY, int iAngleZ, EXEFrame * psFrameInfo )
{//0x004759A0 (0x04AF8188)
	EXEMesh * p;
	READTHIS( p );

	if ( p )
		return p->Animate( iFrame, iAngleX, iAngleY, iAngleZ, psFrameInfo );

	return TRUE;
}

int __stdcall RenderGate( int i )
{//0x00484370 (0x04AF8190)
	EXERender * p;
	READTHIS( p );

	return p->Render( i );
}

int __stdcall RenderGate1()
{//0x004849B0 (0x04AF81E0)
	EXERender * p;
	READTHIS( p );

	return p->Render();
}

int __stdcall RenderModelGate( Point3D * psCamera, Point3D * psAngle, EXEFrame * psFrameInfo )
{//0x00477CD0 (0x04AF8198)
	EXEModel * p;
	READTHIS( p );

	return p->Render( psCamera, psAngle, psFrameInfo );
}

int __stdcall RenderMeshGate( Point3D * psCamera, Point3D * psAngle )
{//0x00476F90 (0x04AF819C)
	EXEMesh * p;
	READTHIS( p );

	return p->PreRender( psCamera, psAngle );
}

BOOL __stdcall ClipFaceRender()
{
	EXERender * p;
	READTHIS( p );
	p->ClipFaceRender();
}

BOOL __cdecl OnRenderRightWeapon( UnitData * pcUnitData, EXEModel * pcModel, EXEMatrixI * psSourceMatrix, EXEMatrixI * psDestMatrix )
{
	return ITEMHANDLER->OnRenderRightWeapon( pcUnitData, pcModel, psSourceMatrix, psDestMatrix );
}

BOOL __cdecl OnRenderSecondWeapon( UnitData * pcUnitData, EXEModel * pcModel, EXEMatrixI * psSourceMatrix, EXEMatrixI * psDestMatrix, BOOL bRight )
{
	return ITEMHANDLER->OnRenderSecondWeapon( pcUnitData, pcModel, psSourceMatrix, psDestMatrix, bRight );
}

BOOL __stdcall AddFaceRender( EXERenderFace * psFace, int iValue )
{
	EXERender * p;
	READTHIS( p );
	return p->AddFaceRender( psFace, iValue );
}

/// <summary>
/// Override Expansion rendering code in smCHAR::DrawShadow()
/// </summary>
void ExpansionSkillRender( UnitData * pcUnitData )
{
	if ( (pcUnitData->sCharacterData.iClass == CHARACTERCLASS_Pikeman) &&
		(pcUnitData->bRendered) &&
		(pcUnitData->sActiveSkill.cSkillTinyID == PikemanSkill::Expansion || pcUnitData->sActiveSkill.cSkillTinyID == PikemanSkill::ChargingStrike ) &&
		(pcUnitData->sRightHandTool.pModel) &&
		(pcUnitData->GetAnimation() == ANIMATIONTYPE_Attack || pcUnitData->GetAnimation() == ANIMATIONTYPE_Skill ) )
	{
		int iLevel = pcUnitData->sActiveSkill.cLevel;

		if ( iLevel < 1 || iLevel > 10 )
			return;

		int iExpansionSize = pcUnitData->sActiveSkill.sParam;

		int a, r, s, maxSize, size;
		Point3D CameraPosi, CameraAngle;

		size = iExpansionSize;

		Point3D sAngle = pcUnitData->sAngle;
		sAngle.iY = (-sAngle.iY + PTANGLE_180) & PTANGLE_Mask;

		pcUnitData->sPosi = pcUnitData->sPosition;

		a = RENDERER->a;
		r = RENDERER->r;

		//RENDERER->a = -96 - 2 *  ( pcUnitData->iFrameCounter << 1 );
		RENDERER->a = -96 - ( pcUnitData->iFrameCounter << 1 );
		RENDERER->r = 128;

		if ( RENDERER->a < -192 )
			RENDERER->a = -192;

		pcUnitData->sRightHandTool.pModel->SetPosition( &pcUnitData->sPosi, &sAngle );
		pcUnitData->sRightHandTool.pModel->uFrame = -1;

		EXEMatrixI Mat;

		//0x48B7C0 = void smIdentityMatrix( smMATRIX &m )
		//Sets D3D matrix to Identiy (1's on diagonal, zero's elsewhere)
		CALL_WITH_ARG1( 0x48B7C0, (DWORD)&Mat );

		s = pcUnitData->iFrameCounter << 8;

		maxSize = ( size << 8 ) / 100;

		if ( s > maxSize )
			s = maxSize;

		s += (fONE/4);

		Mat._11 = fONE + s;
		Mat._22 = fONE + s;
		Mat._33 = fONE + s;

		CameraPosi.iX = UNITTARGETPOSITION_X;
		CameraPosi.iY = UNITTARGETPOSITION_Y;
		CameraPosi.iZ = UNITTARGETPOSITION_Z;

		CameraAngle.iX = UNITTARGETANGLE_X;
		CameraAngle.iY = UNITTARGETANGLE_Y;
		CameraAngle.iZ = UNITTARGETANGLE_Z;

		memcpy( &pcUnitData->sRightHandTool.pModel->paMesh[0]->sWorld , &pcUnitData->sRightHandMatrix, sizeof(EXEMatrixI) );

		//0x48B800 = void smMatrixMult( smMATRIX &m , smMATRIX &a, smMATRIX &b )
		// [C] = [A] * [B]
		CALL_WITH_ARG3( 0x48B800, (DWORD)&pcUnitData->sRightHandTool.pModel->paMesh[0]->sWorld, (DWORD)&Mat , (DWORD)&pcUnitData->sRightHandTool.pModel->paMesh[0]->sWorld );

		pcUnitData->sRightHandTool.pModel->Render( &CameraPosi , &CameraAngle );

		RENDERER->a = a;
		RENDERER->r = r;
	}
}


BOOL __cdecl IsWarehouseUse( ItemData * psItemData )
{
	return ITEMWINDOWHANDLER->IsWarehouseUse( psItemData );
}

BOOL __cdecl IsPersonalShopUse( ItemData * psItemData )
{
	return ITEMWINDOWHANDLER->IsPersonalShopUse( psItemData );
}

BOOL __cdecl TestScreen()
{
#ifdef TEST_SCREEN
//	TESTSCREEN->Render();
	return TRUE;

#endif

	return FALSE;
}

// Title Box
BOOL __stdcall SetEventTitleBox( char * pszText )
{ //0x004D1370

	DWORD dwAddr = 0x00958950;
	do
	{
		WRITEDWORD( dwAddr, FALSE );
		dwAddr += 0x94;
	}
	while ( dwAddr < 0x0095A5A4 );

	CTITLEBOX->SetText( pszText );
	return TRUE;
}

BOOL __cdecl IgnoreMousePointer()
{
	if ( MOUSEHANDLER->IsNoMove() || MOUSEHANDLER->IsDisabled() )
		return TRUE;

	return FALSE;
}

void __cdecl FrameUnitUpdate(UnitData * pcUnitData)
{
	if ( pcUnitData )
	{
		pcUnitData->iFrame += pcUnitData->iFrameStep;

		if ( pcUnitData->iFrame > pcUnitData->iEndFrame )
			pcUnitData->iFrame = pcUnitData->iEndFrame;
	}
}

BOOL __stdcall FinalRender()
{
	EXEMesh * pcThis = NULL;
	READTHIS( pcThis );

	if ( READDWORD( 0x004B0D9E0 ) == TRUE )
	{
		if ( pcThis )
			EXERenderCache::FinalRenderHandler( pcThis );
	}
	else
	{
		if ( pcThis )
			CALLT( 0x00476150, (DWORD)pcThis );
	}

	return TRUE;
}

void __cdecl RenderCacheData()
{
	EXERenderCache::RenderCacheData();
}

void __cdecl DrawEffects( int iX, int iY, int iZ, int iAX, int iAY, int iAZ )
{
	GameCore::GetInstance()->DrawEffect( iX, iY, iZ, iAX, iAY, iAZ );
}

int __stdcall RenderGlow3D( int iMaterial )
{
	EXERender * pc = NULL;
	READTHIS( pc );

	return pc->RenderGlow3D( iMaterial );
}

void __stdcall RenderEXE( int iX, int iY, int iZ, int iAX, int iAY, int iAZ )
{
	Render( iX, iY, iZ, iAX, iAY, iAZ );
}

int __cdecl LoadTexture( EXETextureHandle * ps, bool bMipmap, bool bSwap )
{
	return EXETexture::LoadTexture( ps, bMipmap, bSwap );
}

BOOL __cdecl AnimateMeshTree( EXEMesh * pcMesh, int iFrame, int iAX, int iAY, int iAZ )
{
	if ( pcMesh == NULL )
		return FALSE;

	EXEMesh * pc = pcMesh;
	EXEMesh * pcaMesh[128];

	int iCount = 0;

	while ( TRUE )
	{
		pcaMesh[iCount++] = pc;

		if ( pc->pParentMesh == NULL )
			break;

		if ( iCount == 128 )
			break;

		pc = pc->pParentMesh;
	}

	for ( int i = (iCount - 1); i >= 0; i-- )
	{
		pc = pcaMesh[i];
		if ( pc )
			pc->Animate( iFrame, iAX, iAY, iAZ );
	}

	return TRUE;
}

BOOL __cdecl AnimateMeshTreeNew( EXEMesh * pcMesh, int iFrame, int iAX, int iAY, int iAZ )
{
	if ( pcMesh == NULL )
		return FALSE;

	EXEMesh * pc = pcMesh;
	EXEMesh * pcaMesh[128];

	int iCount = 0;

	while ( TRUE )
	{
		pcaMesh[iCount++] = pc;

		if ( pc->pParentMesh == NULL )
			break;

		if ( iCount == 128 )
			break;

		pc = pc->pParentMesh;
	}

	for ( int i = (iCount - 1); i >= 0; i-- )
	{
		pc = pcaMesh[i];
		if ( pc )
			pc->Animate( iFrame, iAX, iAY, iAZ );
	}

	return TRUE;
}

void __cdecl UpdateBlockedPosition( UnitData * pcUnitData )
{
	MAPGAME->UpdateBlockedPosition( pcUnitData );
}

BOOL __cdecl SetPVPArea( UnitData * pcUnitData )
{
	return UNITGAME->SetPVPArea( pcUnitData );
}

BOOL __cdecl OnBuySkill()
{
	return SKILLMANAGERHANDLER->OnBuySkill();
}

BOOL __cdecl OpenWorldMapGate()
{//00509B10 (0x04AF8208)

	if (!ISSHOW_MINIMAP)
	{
		//just the hidden mini map appear again
		ISSHOW_MINIMAP = TRUE;
	}
	else
	{
		//toggle size
		MINIMAPHANDLER->bLargeMiniMap = !MINIMAPHANDLER->bLargeMiniMap;

		MINIMAPHANDLER->iMiniMapSize = MINIMAPHANDLER->bLargeMiniMap ? MINIMAP_LARGE_SIZE : MINIMAP_SMALL_SIZE;
		MINIMAPHANDLER->OnMiniMapSizeChanged();
	}



	return TRUE;
}

CharacterClassData * __cdecl GetCharacterClassDataGate( int iClass, int iRankup )
{//0044D0F0 (0x04AF8200)
	return GetCharacterClassData( iClass, iRankup );
}

void __cdecl HandleDebuffSkill( UnitData * pcUnitData, int iID, int iValue )
{
	SKILLMANAGERHANDLER->HandleDebuffSkill( pcUnitData, iID, iValue );
}


int __cdecl GetSkillRange( Skill * psSkill, int iAttackRange, int iAttackMode )
{

	WRITEDWORD( 0x00A1598C, iAttackRange );
	WRITEDWORD( 0x00A15988, iAttackMode );

	int iRange = SKILLMANAGERHANDLER->GetSkillRange( psSkill, iAttackRange >> 8, iAttackMode );

	if ( iRange == 0 )
	{
		iRange = CALL_WITH_ARG3 ( 0x0040EFF0, (DWORD)psSkill, iAttackRange, iAttackMode ); //int GetSkillDistRange( sSKILL *lpSkill , int AttackRange , int AttackMode )
	}

	SKILLMANAGERHANDLER->SetLastAttackRange( iRange );

	return iRange;
}

BOOL __cdecl IsUnitHighRange( UnitData * pcUnitData )
{
	BOOL bRet = FALSE;

	EMonsterEffectID iMonsterEffectID = pcUnitData->sCharacterData.iMonsterEffectID;

	switch ( iMonsterEffectID )
	{
		case MONSTEREFFECTID_FIGON:
		case MONSTEREFFECTID_WEB:
		case MONSTEREFFECTID_SKELETONRANGE:
		case MONSTEREFFECTID_IMP:
		case MONSTEREFFECTID_SCORPION:
		case MONSTEREFFECTID_NAZ:
		case MONSTEREFFECTID_NAZSENIOR:
		case MONSTEREFFECTID_Faugn:
		case MONSTEREFFECTID_Yagditha:
		case MONSTEREFFECTID_ITechnician:
		case MONSTEREFFECTID_LTechnician:
		case MONSTEREFFECTID_LEngineer:
		case MONSTEREFFECTID_ISoldier:
		case MONSTEREFFECTID_IBomber:
		case MONSTEREFFECTID_LGuardian:
		case MONSTEREFFECTID_IElite:
		case MONSTEREFFECTID_TULLA:
		case MONSTEREFFECTID_DevilShy:
		case MONSTEREFFECTID_Draxos:
		case MONSTEREFFECTID_Greedy:
		case MONSTEREFFECTID_Midranda:
		case MONSTEREFFECTID_FLAMEMAIDEN:
			bRet = TRUE;
			break;

		default:
			break;
	}

	return bRet;
}

void __stdcall SetInventoryItem( ItemData * psItem )
{
	CGameInventory::GetInstance()->SetInventoryItem( psItem );
}

BOOL ProcessSkillInParty( int iID, int iLevel, int iArea, int iP1, int iP2, int iP3, int iP4 )
{ //006234F0 (0x04AF8228)
	SKILLMANAGERHANDLER->ProcessSkillInParty( iID, iLevel, iArea, iP1, iP2, iP3, iP4 );
	return TRUE;
}

BOOL __stdcall GetInventoryItemGate( int iX, int iY, BOOL bPickup )
{ //004C4D30
	return CGameInventory::GetInstance()->GetInventoryItem( iX, iY, bPickup );
}

BOOL ShowTradeRequest( int iTargetID, char * pszCharacterName )
{//00506F80 (0x04AF822C)
	MESSAGEBOX->SetBoxType( CMessageBox::BOXTYPEID_YesNo );
	MESSAGEBOX->SetTitle( "Trade Request" );
	MESSAGEBOX->SetDescription( FormatString( "Do you want to accept %s's Trade Request?", pszCharacterName ) );
	MESSAGEBOX->SetEvent( std::bind( &HUDController::OnAcceptTradeRequest, HUDHANDLER, iTargetID ) );
	MESSAGEBOX->SetAutoClose( 10 );
	MESSAGEBOX->Show();

	return TRUE;
}

BOOL __cdecl PrivateMessageSocialWindow( char * pszCharacterName )
{//0045C700 (0x04AF8234)
	CHATBOX->OnEnableChatBox();
	CHATBOX->SetInputFieldText( FormatString( "/:%s ", pszCharacterName ) );
	return TRUE;
}

BOOL __cdecl CanGetItem( PacketItem * psPacket, int iPacketHeader )
{
	return CQUESTGAME->CanGetItem( psPacket, iPacketHeader );
}

BOOL __stdcall CanCarryGold( int iGold )
{
	return CHARACTERGAME->CanCarryGold( iGold );
}

BOOL __stdcall GetGoldPersonalShopLimit( int iGold )
{
	return ITEMWINDOWHANDLER->GetGoldPersonalShopLimit( iGold );
}

int __cdecl GetCanCarryGold()
{
	return CHARACTERGAME->GetCanCarryGold();
}

BOOL __cdecl IsSellItem(ItemData* pcItemData)
{
	return ITEMWINDOWHANDLER->IsSellItem(pcItemData);
}

void __cdecl LoadDefaultModels()
{
	ITEMHANDLER->LoadDefaultModels();
}

BOOL __stdcall BuyItemShop( ItemData * pcItemData, int iItemCount )
{
	return ITEMHANDLER->BuyItemShop( pcItemData, iItemCount );
}


void PatchSingleByte( PVOID address, BYTE val )
{
	DWORD l_OldFlags;
	VirtualProtect( address, 1, PAGE_EXECUTE_READWRITE, &l_OldFlags );
	*reinterpret_cast<BYTE *>(address) = val;
	VirtualProtect( (void *)address, 1, l_OldFlags, NULL );
}

void PatchWord( PVOID address, WORD val )
{
	DWORD l_OldFlags;
	VirtualProtect( address, 2, PAGE_EXECUTE_READWRITE, &l_OldFlags );
	*reinterpret_cast<WORD *>(address) = val;
	VirtualProtect( (void *)address, 2, l_OldFlags, NULL );
}

/**
index = 0 based
*/
void PatchTeleportCore(int baseAddress, int index, EMapID mapID)
{
	const int bytesPerTeleportGroup = 12; //4 + 4 + 4;
	const int groupAddress = baseAddress + (bytesPerTeleportGroup * index);
	PVOID mapIdAddress = (PVOID)(groupAddress + (4));	  //+4 = map id
	PVOID mapLevelAddress = (PVOID)(groupAddress + (4 + 4)); //+4 +4 = map level

	if (mapID == MAPID_Invalid)
	{
		PatchSingleByte(mapIdAddress, (BYTE)mapID);
		PatchSingleByte(mapLevelAddress, 150);
	}
	else
	{
		BYTE bMapLevel = (BYTE)MAPLEVEL(mapID);
		PatchSingleByte(mapIdAddress, (BYTE)mapID);
		PatchSingleByte(mapLevelAddress, bMapLevel);
	}
}

void PatchInt(PVOID address, int val)
{
	DWORD l_OldFlags;
	VirtualProtect(address, 4, PAGE_EXECUTE_READWRITE, &l_OldFlags);
	*reinterpret_cast<int*>(address) = val;
	VirtualProtect((void*)address, 4, l_OldFlags, NULL);
}

void PatchFloat ( PVOID address, float val )
{
	DWORD l_OldFlags;
	VirtualProtect ( address, 4, PAGE_EXECUTE_READWRITE, &l_OldFlags );
	*reinterpret_cast<float *>( address ) = val;
	VirtualProtect ( (void *)address, 4, l_OldFlags, NULL );
}

extern int __stdcall CustomSetForceOrb ( DWORD ItemCODE, int Time );
signed int __stdcall CustomSetForceOrb ( DWORD ItemCODE, int Time )
{
	//UNITGAME->Log ( "CustomSetForceOrb = %d", ItemCODE );

	return TRUE;
}

/// <summary>
/// A special version of
/// int dm_SelectRange( int x, int y, int z , int range , int UseAttackRating )
/// which excludes main target
/// </summary>
extern unsigned int __cdecl CustomGetRangeExcludingMainTarget ( int x, int y, int z, int range, int UserAttackRating );
unsigned int __cdecl CustomGetRangeExcludingMainTarget ( int x, int y, int z, int range, int UseAttackRating )
{
	return 0;
}




/// <summary>
/// Override calls to GetAttackSpeedMainFrame
/// Leaked:  int GetAttackSpeedMainFrame( int AttackSpeed )
/// ASM: 0x45F7D0
/// </summary>
extern unsigned int __cdecl CustomGetAttackSpeedMainFrame ( int iAttackSpeed, int iAddSpeed );
unsigned int __cdecl CustomGetAttackSpeedMainFrame ( int iAttackSpeed, int iAddSpeed )
{
	return SKILLMANAGERHANDLER->GetAttackSpeedMainFrame ( iAttackSpeed );
}

/// <summary>
/// Override calls to GetAttackFrameSpeed_Half
/// Leaked:  int GetAttackFrameSpeed_Half( int AttackSpeed , int AddSpeed )
/// ASM: 0x45F7D0
/// </summary>
extern unsigned int __cdecl CustomGetAttackFrameSpeed ( int iAttackSpeed, int iAddSpeed );
unsigned int __cdecl CustomGetAttackFrameSpeed ( int iAttackSpeed, int iAddSpeed )
{
	int result;

	result = SKILLMANAGERHANDLER->GetAttackSpeedMainFrame ( iAttackSpeed );

	if ( iAddSpeed > 10 )
		iAddSpeed = 10;

	result += 40 * iAddSpeed;

	return result >> 2;
}

/// <summary>
/// Override calls to PlaySkillLoop
/// Leaked:  int PlaySkillLoop( int SkillCode , smCHAR *lpChar )
/// ASM: 0x40F460
/// </summary>
unsigned int __cdecl CustomPlaySkillLoop ( int SKillCode, UnitData * lpChar )
{
	int result;

	if ( SKILLMANAGERHANDLER->PlaySkillLoop( (ActiveSkill)SKillCode, lpChar ) )
	{
		return TRUE; //doesn't do anything in ASM
	}

	typedef int(__cdecl * tfnPlaySkillLoop)( int SKillCode, UnitData * lpChar );
	tfnPlaySkillLoop fn = (tfnPlaySkillLoop)0x0040F460;

	return fn( SKillCode, lpChar );
}

void PatchCallsToFunction_PlaySkillLoop ()
{
	DWORD l_SubCalls[] =
	{
		0x0043E70F,
	};

	int l_SubCallsCount = sizeof ( l_SubCalls ) / sizeof ( l_SubCalls[0] );

	for ( int l_I = 0; l_I < l_SubCallsCount; ++l_I )
	{
		DWORD l_Address = l_SubCalls[l_I];
		DWORD l_PatchSize = 5;
		DWORD l_OldFlags;

		VirtualProtect ( (void *)l_Address, l_PatchSize, PAGE_EXECUTE_READWRITE, &l_OldFlags );
		*reinterpret_cast<BYTE *> ( l_Address ) = 0xE8;
		*reinterpret_cast<DWORD *>( l_Address + 1 ) = (DWORD)CustomPlaySkillLoop - (DWORD)l_Address - l_PatchSize;
		VirtualProtect ( (void *)l_Address, l_PatchSize, l_OldFlags, NULL );
	}
}


/// <summary>
/// Override calls to PlaySkillLoop
/// Leaked:  int PlaySkillLoop( int SkillCode , smCHAR *lpChar )
/// ASM: 0x40F460
/// </summary>
unsigned int __fastcall CustomPlayAttack( UnitData * p_This, void * p_PlaceHolder, int EventFlag )
{
	return p_This->PlayAttackOverride();
}

void PatchCallsToFunction_PlayAttack ()
{
	DWORD l_SubCalls[] =
	{
		0x00438055,
		0x0043B618,
		0x0043C9D8,
		0x0043C9F0,
		0x0043CA0A,
	};

	int l_SubCallsCount = sizeof ( l_SubCalls ) / sizeof ( l_SubCalls[0] );

	for ( int l_I = 0; l_I < l_SubCallsCount; ++l_I )
	{
		DWORD l_Address = l_SubCalls[l_I];
		DWORD l_PatchSize = 5;
		DWORD l_OldFlags;

		VirtualProtect ( (void *)l_Address, l_PatchSize, PAGE_EXECUTE_READWRITE, &l_OldFlags );
		*reinterpret_cast<BYTE *> ( l_Address ) = 0xE8;
		*reinterpret_cast<DWORD *>( l_Address + 1 ) = (DWORD)CustomPlayAttack - (DWORD)l_Address - l_PatchSize;
		VirtualProtect ( (void *)l_Address, l_PatchSize, l_OldFlags, NULL );
	}
}


/// <summary>
/// Override all calls to UnitData->Init()
/// so we can init existing values or new values if needed
/// </summary>
signed int __fastcall CustomUnitDataInit( UnitData * p_This, void * p_PlaceHolder )
{
	p_This->Init();
	p_This->iPoisonOverlay = 0; //has a value of 256 to start with
	p_This->bDamageReduction = 0;

	return TRUE;
}

void PatchCallsToFunction_UnitDataInit()
{
	DWORD l_SubCalls[] =
	{
		0x0042966B,
		0x00438DFC,
		0x00452595,
		0x005863CF,
		0x00586496,
		0x00586612,
		0x00586CC9,
		0x005886A2,
		0x005886D2,
		0x00589244,
		0x005DACDC,
		0x005DC3ED,
		0x005DC4EF,
		0x005DC658,
		0x005F8EA8,
		0x006204F9,
		0x0062D147
	};

	int l_SubCallsCount = sizeof ( l_SubCalls ) / sizeof ( l_SubCalls[0] );

	for ( int l_I = 0; l_I < l_SubCallsCount; ++l_I )
	{
		DWORD l_Address = l_SubCalls[l_I];
		DWORD l_PatchSize = 5;
		DWORD l_OldFlags;

		VirtualProtect ( (void *)l_Address, l_PatchSize, PAGE_EXECUTE_READWRITE, &l_OldFlags );
		*reinterpret_cast<BYTE *> ( l_Address ) = 0xE8;
		*reinterpret_cast<DWORD *>( l_Address + 1 ) = (DWORD)CustomUnitDataInit - (DWORD)l_Address - l_PatchSize;
		VirtualProtect ( (void *)l_Address, l_PatchSize, l_OldFlags, NULL );
	}
}


/// <summary>
/// void SkillCelestialMusPel(smCHAR* pChar, float time, float attackTime, int dwSkillCode)
/// </summary>
DWORD *__cdecl CustomSkillCelestialMusPel( UnitData * pChar, float time, float attackTime, int dwSkillCode )
{
	typedef int( __thiscall *t_SkillCelestialMusPel ) ( UnitData * pcUnitData, float fDuration, float fAttackDelay, int eSkillID );
	t_SkillCelestialMusPel SkillCelestialMusPel = ( t_SkillCelestialMusPel )0x005D48F0;


	//SkillCelestialMusPel( pChar, time, attackTime, dwSkillCode );

	return 0;
}

void PatchCallsToFunction_SkillCelestialMusPel()
{
	return;

	DWORD l_SubCalls[] =
	{
		0x00426EB2,
		0x004270D1,
		0x00427FE2,
		0x00428531,
	};

	int l_SubCallsCount = sizeof ( l_SubCalls ) / sizeof ( l_SubCalls[0] );

	for ( int l_I = 0; l_I < l_SubCallsCount; ++l_I )
	{
		DWORD l_Address = l_SubCalls[l_I];
		DWORD l_PatchSize = 5;
		DWORD l_OldFlags;

		VirtualProtect ( (void *)l_Address, l_PatchSize, PAGE_EXECUTE_READWRITE, &l_OldFlags );
		*reinterpret_cast<BYTE *> ( l_Address ) = 0xE8;
		*reinterpret_cast<DWORD *>( l_Address + 1 ) = (DWORD)CustomSkillCelestialMusPel - (DWORD)l_Address - l_PatchSize;
		VirtualProtect ( (void *)l_Address, l_PatchSize, l_OldFlags, NULL );
	}
}

/// <summary>
/// Overrides calls to int smCHAR::MoveAngle2( int step ) in assembly
/// </summary>
/// <param name="iDistance">The move distance</param>
int __fastcall CustomMoveAngle2(UnitData * p_This, void * p_PlaceHolder, int iDistance )
{
	Skill* psSkill = TIMERSKILLHANDLER->FindSkillTimer( ETimerID::Inpes );

	if ( psSkill && psSkill->iLevel )
	{
		int percent = SKILLMANAGERHANDLER->GetSkillIntValue( ESkillArrayPointer::Assassin_Inpes_MovementSpeed_New, psSkill->iLevel - 1 );

		if ( percent )
		{
			//half for party members
			if ( p_This->sCharacterData.iClass != ECharacterClass::CHARACTERCLASS_Assassin)
				percent >>= 1;

			iDistance += ( iDistance * percent ) / 100;
		}
	}

	p_This->MoveAngle( iDistance );
	return TRUE;
}

void PatchCallsToFunction_MoveAngle2()
{
	DWORD l_SubCalls[] =
	{
		0x004346B1,
		0x004347F3,
		0x0043480F,
		0x0043D026,
	};

	int l_SubCallsCount = sizeof ( l_SubCalls ) / sizeof ( l_SubCalls[0] );

	for ( int l_I = 0; l_I < l_SubCallsCount; ++l_I )
	{
		DWORD l_Address = l_SubCalls[l_I];
		DWORD l_PatchSize = 5;
		DWORD l_OldFlags;

		VirtualProtect ( (void *)l_Address, l_PatchSize, PAGE_EXECUTE_READWRITE, &l_OldFlags );
		*reinterpret_cast<BYTE *> ( l_Address ) = 0xE8;
		*reinterpret_cast<DWORD *>( l_Address + 1 ) = (DWORD)CustomMoveAngle2 - (DWORD)l_Address - l_PatchSize;
		VirtualProtect ( (void *)l_Address, l_PatchSize, l_OldFlags, NULL );
	}
}

/// <summary>
/// Overrides calls to int smCHAR::SavePlayBuff( ) in assembly
/// </summary>
int __fastcall MakePlayBuffFromRecvData(UnitData* pcUnitData, void* p_PlaceHolder, PlayBuff* StartBuff, PlayBuff* EndBuff, int len )
{
	return CHARACTERGAME->MakePlayBuffFromRecvData(pcUnitData, StartBuff, EndBuff, len);
}

void PatchCallsToFunction_MakePlayBuffFromRecvData()
{
	DWORD l_SubCalls[] =
	{
		0x00434C13,
		0x00434C7B,
		0x0043C846,
		0x0043C8AE,
		0x0043C8CA,
		0x0043C932
	};

	int l_SubCallsCount = sizeof(l_SubCalls) / sizeof(l_SubCalls[0]);

	for ( int l_I = 0; l_I < l_SubCallsCount; ++l_I )
	{
		DWORD l_Address = l_SubCalls[l_I];
		DWORD l_PatchSize = 5;
		DWORD l_OldFlags;

		VirtualProtect((void*)l_Address, l_PatchSize, PAGE_EXECUTE_READWRITE, &l_OldFlags);
		*reinterpret_cast<BYTE*> ( l_Address ) = 0xE8;
		*reinterpret_cast<DWORD*>( l_Address + 1 ) = (DWORD)MakePlayBuffFromRecvData - (DWORD)l_Address - l_PatchSize;
		VirtualProtect((void*)l_Address, l_PatchSize, l_OldFlags, NULL);
	}
}


/// <summary>
/// Overrides calls to int smCHAR::SavePlayBuff( ) in assembly
/// </summary>
int __fastcall CustomSavePlayBuff(UnitData* p_This, void* p_PlaceHolder)
{
	p_This->iPlayBuffCnt = ( p_This->iPlayBuffCnt + 1 ) & PLAYBUFF_MASK;

	//p_This->sAngle.iX and p_This->sAngle.iZ isn't really used, so they can be repurposed

	p_This->saPlayBuff[p_This->iPlayBuffCnt].saAngle[0] = 0;
	p_This->saPlayBuff[p_This->iPlayBuffCnt].saAngle[1] = p_This->sAngle.iY;
	p_This->saPlayBuff[p_This->iPlayBuffCnt].saAngle[2] = static_cast<short>( p_This->sActiveSkill.sParam );
	p_This->saPlayBuff[p_This->iPlayBuffCnt].saAngle[3] = p_This->iAction;

	p_This->saPlayBuff[p_This->iPlayBuffCnt].sPosition.iX = p_This->sPosition.iX;
	p_This->saPlayBuff[p_This->iPlayBuffCnt].sPosition.iY = p_This->sPosition.iY;
	p_This->saPlayBuff[p_This->iPlayBuffCnt].sPosition.iZ = p_This->sPosition.iZ;

	p_This->saPlayBuff[p_This->iPlayBuffCnt].iFrame = ( p_This->iMotionSelectFrame << CHAR_FRAME_SELSHIFT ) | p_This->iFrame;

	if (p_This->pcTarget )
		p_This->saPlayBuff[p_This->iPlayBuffCnt].lTargetID = p_This->pcTarget->iID;
	else
		p_This->saPlayBuff[p_This->iPlayBuffCnt].lTargetID = NULL;

	p_This->saPlayBuff[p_This->iPlayBuffCnt].saEquippedItems[smPLAYBUFF_ITEM_BODY]  = p_This->saEquippedItems[smPLAYBUFF_ITEM_BODY];
	p_This->saPlayBuff[p_This->iPlayBuffCnt].saEquippedItems[smPLAYBUFF_ITEM_LEFT]  = p_This->saEquippedItems[smPLAYBUFF_ITEM_LEFT];
	p_This->saPlayBuff[p_This->iPlayBuffCnt].saEquippedItems[smPLAYBUFF_ITEM_RIGHT] = p_This->saEquippedItems[smPLAYBUFF_ITEM_RIGHT];

	//Note - we will put the skill parameter (USHORT) in saAngle[2]
	p_This->saPlayBuff[p_This->iPlayBuffCnt].saEquippedItems[smPLAYBUFF_ITEM_SKIL] = (p_This->sActiveSkill.cSkillTinyID << 8) | p_This->sActiveSkill.cLevel; //we will lose the parameter, but we'll put it in saAngle[2]

	return p_This->iPlayBuffCnt;
}


void PatchCallsToFunction_SavePlayBuff()
{
	DWORD l_SubCalls[] =
	{
		0x0043EB27,
		0x004415F9,
	};

	int l_SubCallsCount = sizeof(l_SubCalls) / sizeof(l_SubCalls[0]);

	for ( int l_I = 0; l_I < l_SubCallsCount; ++l_I )
	{
		DWORD l_Address = l_SubCalls[l_I];
		DWORD l_PatchSize = 5;
		DWORD l_OldFlags;

		VirtualProtect((void*)l_Address, l_PatchSize, PAGE_EXECUTE_READWRITE, &l_OldFlags);
		*reinterpret_cast<BYTE*> ( l_Address ) = 0xE8;
		*reinterpret_cast<DWORD*>( l_Address + 1 ) = (DWORD)CustomSavePlayBuff - (DWORD)l_Address - l_PatchSize;
		VirtualProtect((void*)l_Address, l_PatchSize, l_OldFlags, NULL);
	}
}

/// <summary>
/// Overrides calls to int smCHAR::SavePlayBuff2( ) in assembly
/// </summary>
int __fastcall CustomSavePlayBuff2(UnitData* p_This, void* p_PlaceHolder)
{
	p_This->iPlayBuffCnt = ( p_This->iPlayBuffCnt + 1 ) & PLAYBUFF_MASK;

	//p_This->sAngle.iX and p_This->sAngle.iZ isn't really used, so they can be repurposed

	p_This->saPlayBuff[p_This->iPlayBuffCnt].saAngle[0] = 0; //spare
	p_This->saPlayBuff[p_This->iPlayBuffCnt].saAngle[1] = p_This->sAngle.iY;
	p_This->saPlayBuff[p_This->iPlayBuffCnt].saAngle[2] = static_cast<short>( p_This->sActiveSkill.sParam );
	p_This->saPlayBuff[p_This->iPlayBuffCnt].saAngle[3] = p_This->iAction;

	p_This->saPlayBuff[p_This->iPlayBuffCnt].sPosition.iX = p_This->sPosition.iX;
	p_This->saPlayBuff[p_This->iPlayBuffCnt].sPosition.iY = p_This->sPosition.iY;
	p_This->saPlayBuff[p_This->iPlayBuffCnt].sPosition.iZ = p_This->sPosition.iZ;

	p_This->saPlayBuff[p_This->iPlayBuffCnt].iFrame = ( p_This->iMotionSelectFrame << CHAR_FRAME_SELSHIFT ) | p_This->iFrame;

	p_This->saPlayBuff[p_This->iPlayBuffCnt].lTargetID = NULL;

	if ( p_This->pcAttackTarget )
		p_This->saPlayBuff[p_This->iPlayBuffCnt].lTargetID = p_This->pcAttackTarget->iID;

	if ( p_This->pcFocusTarget )
		p_This->saPlayBuff[p_This->iPlayBuffCnt].lTargetID = p_This->pcFocusTarget->iID;

	p_This->saPlayBuff[p_This->iPlayBuffCnt].saEquippedItems[smPLAYBUFF_ITEM_BODY] = p_This->saEquippedItems[smPLAYBUFF_ITEM_BODY];
	p_This->saPlayBuff[p_This->iPlayBuffCnt].saEquippedItems[smPLAYBUFF_ITEM_LEFT] = p_This->saEquippedItems[smPLAYBUFF_ITEM_LEFT];
	p_This->saPlayBuff[p_This->iPlayBuffCnt].saEquippedItems[smPLAYBUFF_ITEM_RIGHT] = p_This->saEquippedItems[smPLAYBUFF_ITEM_RIGHT];


	//Note - we will put the skill parameter (USHORT) in saAngle[2]
	p_This->saPlayBuff[p_This->iPlayBuffCnt].saEquippedItems[smPLAYBUFF_ITEM_SKIL] = ( p_This->sActiveSkill.cSkillTinyID << 8 ) | p_This->sActiveSkill.cLevel; //we will lose the parameter, but we'll put it in saAngle[2]

	return p_This->iPlayBuffCnt;
}


void PatchCallsToFunction_SavePlayBuff2()
{
	DWORD l_SubCalls[] =
	{
		0x0043EB9F,
		0x0043F4E3,
	};

	int l_SubCallsCount = sizeof(l_SubCalls) / sizeof(l_SubCalls[0]);

	for ( int l_I = 0; l_I < l_SubCallsCount; ++l_I )
	{
		DWORD l_Address = l_SubCalls[l_I];
		DWORD l_PatchSize = 5;
		DWORD l_OldFlags;

		VirtualProtect((void*)l_Address, l_PatchSize, PAGE_EXECUTE_READWRITE, &l_OldFlags);
		*reinterpret_cast<BYTE*> ( l_Address ) = 0xE8;
		*reinterpret_cast<DWORD*>( l_Address + 1 ) = (DWORD)CustomSavePlayBuff2 - (DWORD)l_Address - l_PatchSize;
		VirtualProtect((void*)l_Address, l_PatchSize, l_OldFlags, NULL);
	}
}


void PatchCallsToFunction_SelectRangeToExcludeMainTarget ()
{
	DWORD l_SubCalls[] =
	{
		0x00456D34,
	};

	int l_SubCallsCount = sizeof ( l_SubCalls ) / sizeof ( l_SubCalls[0] );

	for ( int l_I = 0; l_I < l_SubCallsCount; ++l_I )
	{
		DWORD l_Address = l_SubCalls[l_I];
		DWORD l_PatchSize = 5;
		DWORD l_OldFlags;

		VirtualProtect ( (void *)l_Address, l_PatchSize, PAGE_EXECUTE_READWRITE, &l_OldFlags );
		*reinterpret_cast<BYTE *> ( l_Address ) = 0xE8;
		*reinterpret_cast<DWORD *>( l_Address + 1 ) = (DWORD)CustomGetRangeExcludingMainTarget - (DWORD)l_Address - l_PatchSize;
		VirtualProtect ( (void *)l_Address, l_PatchSize, l_OldFlags, NULL );
	}
}


void PatchCallsToFunction_GetAttackFrameSpeed ()
{
	DWORD l_SubCalls[] =
	{
		0x0041017C,
		0x00410C02,
		0x004118F1,
		0x00411D8E,
		0x00411E9C,
		0x00412A85,
		0x00412EAE,
		0x0041308F,
		0x0041380B,
		0x00415317,
	};

	int l_SubCallsCount = sizeof ( l_SubCalls ) / sizeof ( l_SubCalls[0] );

	for ( int l_I = 0; l_I < l_SubCallsCount; ++l_I )
	{
		DWORD l_Address = l_SubCalls[l_I];
		DWORD l_PatchSize = 5;
		DWORD l_OldFlags;

		VirtualProtect ( (void *)l_Address, l_PatchSize, PAGE_EXECUTE_READWRITE, &l_OldFlags );
		*reinterpret_cast<BYTE *> ( l_Address ) = 0xE8;
		*reinterpret_cast<DWORD *>( l_Address + 1 ) = (DWORD)CustomGetAttackFrameSpeed - (DWORD)l_Address - l_PatchSize;
		VirtualProtect ( (void *)l_Address, l_PatchSize, l_OldFlags, NULL );
	}
}

void PatchCallsToFunction_GetAttackSpeedMainFrame ()
{
	DWORD l_SubCalls[] =
	{
		0x0043E7D5,
	};

	int l_SubCallsCount = sizeof ( l_SubCalls ) / sizeof ( l_SubCalls[0] );

	for ( int l_I = 0; l_I < l_SubCallsCount; ++l_I )
	{
		DWORD l_Address = l_SubCalls[l_I];
		DWORD l_PatchSize = 5;
		DWORD l_OldFlags;

		VirtualProtect ( (void *)l_Address, l_PatchSize, PAGE_EXECUTE_READWRITE, &l_OldFlags );
		*reinterpret_cast<BYTE *> ( l_Address ) = 0xE8;
		*reinterpret_cast<DWORD *>( l_Address + 1 ) = (DWORD)CustomGetAttackSpeedMainFrame - (DWORD)l_Address - l_PatchSize;
		VirtualProtect ( (void *)l_Address, l_PatchSize, l_OldFlags, NULL );
	}
}

//460AE0 = function ContGameExp
//43DE59 = callee
signed int __cdecl CustomContGameExp ( UnitData * lpChar )
{
	//DEBUGINFO ( "CustomContGameExp" );

	if ( MAP_ID == MAPID_GardenOfFreedom ||
		 MAP_ID == MAPID_CastleOfTheLost ||
		 MAP_ID == MAPID_Bellatra ||
		 MAP_ID == MAPID_BlessCastle )
	{
		//DEBUGINFO ( "CustomContGameExp returns FALSE" );

		return FALSE;
	}

	typedef signed int ( __cdecl * tfnContGameExp )( UnitData * pThis );
	tfnContGameExp fnContGameExp = (tfnContGameExp)0x00460AE0;

	return fnContGameExp ( lpChar );

	/*Level = lpChar->sCharacterData.iLevel;
	if ( Level >= 11 )
	{
		LowExp = ExpLevelTable[Level - 1];
		HighExp = ExpLevelTable[Level];
		exp = ( HighExp - LowExp ) / 100;

		//lpChar->smCharInfo.Exp ^= CodeXorExp;
		exp64 = CodeXorCharInfo_Exp ();

		if ( exp64 > LowExp ) {
			ep = exp64;
			exp64 -= exp;
			if ( exp64 < LowExp )
				exp64 = LowExp;

			SetExp64 ( &lpCurPlayer->smCharInfo, exp64 );

			ep = ep - exp64;
			SetTotalSubExp ( (int)ep );
		}

		CodeXorCharInfo_Exp ();
		//lpChar->smCharInfo.Exp ^= CodeXorExp;

		Money = lpChar->smCharInfo.Money / 100;
		lpChar->smCharInfo.Money -= Money;
		if ( lpChar->smCharInfo.Money < 0 ) {
			Money = -Money;
			lpChar->smCharInfo.Money = 0;
		}

		SetTotalSubMoney ( Money );


		ReformCharForm ();
		ResetInvenMoney ();
	}*/

	return TRUE;
}


//0x004688E0
//int rsRECORD_DBASE::ResotrRecordData( smCHAR_INFO *lpCharInfo , sITEM *lpItems , sITEM *lpItems2 , sITEM *lpMouseItem )
signed int __fastcall CustomRestoreRecordData ( DWORD * p_This, void * p_PlaceHolder, CharacterData * lpCharInfo, ItemData * lpItems, ItemData * lpItems2, ItemData * lpMouseItem )
{
	//TransRecordData.GameSaveInfo

	auto recordData = (PacketCharacterRecordData *)( p_This );

	DWORD pp = (DWORD)&recordData->sCharacterSaveData;
	*(WORD *)( pp + 0x9c ) = 0; //Correct offset of wForceOrbUsing[0]
	*(WORD *)( pp + 0x9e ) = 0; //Correct offset of wForceOrbUsing[1]
	//keyword = force orb

	typedef BOOL ( __thiscall * tfnRestoreRecordData )( DWORD * pThis, CharacterData * lpCharInfo, ItemData * lpItems, ItemData * lpItems2, ItemData * lpMouseItem );
	tfnRestoreRecordData fnRestoreRecordData = (tfnRestoreRecordData)0x004688E0;

	BOOL bResult = fnRestoreRecordData (p_This, lpCharInfo, lpItems, lpItems2, lpMouseItem );

	return bResult;
}

signed int __fastcall CustomRepairInvenItem(DWORD p_This, void* p_PlaceHolder, ItemData* p_Item, int p_RepairCost)
{
	if ( p_Item->iItemSlotFlag == ITEMSLOTFLAG_LeftRightHand && p_Item->iItemSlot )
	{
		p_Item = &INVENTORYITEMS[INVENTORYITEMSLOT[0].iItemIndex - 1];

		if ( p_Item == NULL )
			return 0;
	}


	reinterpret_cast<int(*)()>(0x45BC90)();                         /// CheckCharForm();

	int l_CurrentMoney = PLAYERDATA->iGold;

	short l_IntegrityMax = p_Item->sItem.sIntegrity.sMax;

	if (!l_IntegrityMax || l_CurrentMoney - p_RepairCost <= 0 || p_Item->sItem.sIntegrity.sCur >= l_IntegrityMax)
	{
		return 0;
	}

	//PriceItem priceItem;
	//ITEMHANDLER->GetItemPrice(p_Item, &priceItem);

	//DEBUG("Repair cost for item %s = %d or %d", p_Item->sItem.szItemName, p_RepairCost, priceItem.iRepairPrice);

	PacketRepairNPCShop l_RepairPacket;
	l_RepairPacket.iHeader = PKTHDR_RepairItemNPCShop;
	l_RepairPacket.iLength = sizeof(l_RepairPacket);
	l_RepairPacket.Money = p_RepairCost;
	memcpy(&l_RepairPacket.sItemData, p_Item, sizeof(ItemData));

	reinterpret_cast<int(*)(int, int)>  (0x508780)(p_RepairCost, 0);    /// sinMinusMoney();
	p_Item->sItem.sIntegrity.sCur = l_IntegrityMax;
	reinterpret_cast<void(*)()>         (0x45BC70)();                   /// ReformCharForm();

	SOCKETL->SendPacket(&l_RepairPacket);

	ITEMINFOBOX->Invalidate(); //to show updated item integrity

	return 1;
}
extern int __fastcall CustomTradeLButtonDown(DWORD* p_UserData, void* p_PlaceHolder, int p_MouseX, int p_MouseY);

signed int __fastcall CustomTradeLButtonDown(DWORD* p_This, void* p_PlaceHolder, int p_MouseX, int p_MouseY)
{
	//DEBUG("CustomTradeLButtonDown");

	typedef BOOL(__thiscall* tfnTradeLButtonDown)(DWORD* pTrade, int iMouseX, int iMouseY);
	tfnTradeLButtonDown fnTradeLButtonDown = (tfnTradeLButtonDown)0x0051DE80;

	//Aging window is open? (0x036834F0)
	//Item in aging window is not valid / set? (0x03686CD4);
	if ((*(BOOL*)(0x036834F0)) && (*(BOOL*)(0x03686CD4)) == FALSE)
	{
		if ( ITEMHANDLER->GetMouseItem () && ITEMHANDLER->GetMouseItem ()->bValid )
		{
			auto mouseIteMData = ITEMHANDLER->GetMouseItem ();
			int itemCode = mouseIteMData->sItem.sItemID.ToInt ();

			//disallow costumes in aging window
			if ( mouseIteMData->sItem.sItemID.ToItemType () == ITEMTYPE_Costume ||
				 mouseIteMData->sItem.sItemID.ToItemType () == ITEMTYPE_Costume2 ||
				 mouseIteMData->sItem.sItemID.ToItemType () == ITEMTYPE_Costume3 ||
				 mouseIteMData->sItem.sItemID.ToItemType () == ITEMTYPE_Costume4 ||
				 mouseIteMData->sItem.sItemID.ToItemID () == ITEMID_ChristmasCloatCostumeMM ||
				 mouseIteMData->sItem.sItemID.ToItemID () == ITEMID_ChristmasCloatCostumeMF ||
				 mouseIteMData->sItem.sItemID.ToItemID () == ITEMID_ChristmasCloatCostumeTM ||
				 mouseIteMData->sItem.sItemID.ToItemID () == ITEMID_ChristmasCloatCostumeTF ||
				 mouseIteMData->sItem.sItemID.ToItemID() == ITEMID_PermanentChristmasCloatCostumeMM ||
				 mouseIteMData->sItem.sItemID.ToItemID() == ITEMID_PermanentChristmasCloatCostumeMF ||
				 mouseIteMData->sItem.sItemID.ToItemID() == ITEMID_PermanentChristmasCloatCostumeTM ||
				 mouseIteMData->sItem.sItemID.ToItemID() == ITEMID_PermanentChristmasCloatCostumeTF ||
				 mouseIteMData->sItem.sItemID.ToItemID () == ITEMID_StarWarsShirt ||
				 mouseIteMData->sItem.sItemID.ToItemID () == ITEMID_StarWarsStaff ||
				 mouseIteMData->sItem.sItemID.ToItemID () == ITEMID_StarWarsStaff90 ||
				 mouseIteMData->sItem.sItemID.ToItemID () == ITEMID_StarWarsStaff100 ||
				 mouseIteMData->sItem.sItemID.ToItemID () == ITEMID_StarWarsStaff110 ||
				 mouseIteMData->sItem.sItemID.ToItemID () == ITEMID_StarWarsScythe ||
				 mouseIteMData->sItem.sItemID.ToItemID () == ITEMID_StarWarsScythe90 ||
				 mouseIteMData->sItem.sItemID.ToItemID () == ITEMID_StarWarsScythe100 ||
				 mouseIteMData->sItem.sItemID.ToItemID () == ITEMID_StarWarsScythe110 ||
				 mouseIteMData->sItem.sItemID.ToItemID () == ITEMID_StarWarsBow ||
				 mouseIteMData->sItem.sItemID.ToItemID () == ITEMID_StarWarsBow90 ||
				 mouseIteMData->sItem.sItemID.ToItemID () == ITEMID_StarWarsBow100 ||
				 mouseIteMData->sItem.sItemID.ToItemID () == ITEMID_StarWarsBow110 ||
				 mouseIteMData->sItem.sItemID.ToItemID () == ITEMID_StarWarsLightSaber1H ||
				 mouseIteMData->sItem.sItemID.ToItemID () == ITEMID_StarWarsLightSaber1H90 ||
				 mouseIteMData->sItem.sItemID.ToItemID () == ITEMID_StarWarsLightSaber1H100 ||
				 mouseIteMData->sItem.sItemID.ToItemID () == ITEMID_StarWarsLightSaber1H110 ||
				 mouseIteMData->sItem.sItemID.ToItemID () == ITEMID_StarWarsLightSaber2H	||
				 mouseIteMData->sItem.sItemID.ToItemID () == ITEMID_StarWarsLightSaber2H90 ||
				 mouseIteMData->sItem.sItemID.ToItemID () == ITEMID_StarWarsLightSaber2H100 ||
				 mouseIteMData->sItem.sItemID.ToItemID () == ITEMID_StarWarsLightSaber2H110 ||
				 mouseIteMData->sItem.sItemID.ToItemID () == ITEMID_StarWarsPhantom ||
				 mouseIteMData->sItem.sItemID.ToItemID () == ITEMID_StarWarsPhantom90 ||
				 mouseIteMData->sItem.sItemID.ToItemID () == ITEMID_StarWarsPhantom100  ||
				 mouseIteMData->sItem.sItemID.ToItemID () == ITEMID_StarWarsPhantom110 ||
				 mouseIteMData->sItem.sItemID.ToItemID () == ITEMID_StarWarsDagger ||
				 mouseIteMData->sItem.sItemID.ToItemID () == ITEMID_StarWarsDagger90 ||
				 mouseIteMData->sItem.sItemID.ToItemID () == ITEMID_StarWarsDagger100 ||
				 mouseIteMData->sItem.sItemID.ToItemID () == ITEMID_StarWarsDagger110 ||
				 mouseIteMData->sItem.sItemID.ToItemID () == ITEMID_StarWarsJavelin ||
				 mouseIteMData->sItem.sItemID.ToItemID () == ITEMID_StarWarsJavelin90 ||
				 mouseIteMData->sItem.sItemID.ToItemID () == ITEMID_StarWarsJavelin100 ||
				 mouseIteMData->sItem.sItemID.ToItemID () == ITEMID_StarWarsJavelin110)
			{
				ITEMWINDOWHANDLER->SetTradeItemColorBox ( ETradeColorBox::TRADECOLORBOX_NotSetItem );
				//( *(DWORD *)0x036EFCA4 ) = 0;
				//return FALSE;
			}

			else if (AGINGWINDOWHANDLER->IsHalfPrice())
			{
				if ((itemCode & 0xFFFF0000) == EItemType::ITEMTYPE_Sheltom || (itemCode & 0xFFFF0000) == EItemType::ITEMTYPE_Respec)
				{
					//don't process
				}
				else
				{
					//only check if item doesn't have aging or has no craft (clean)
					if (((mouseIteMData->sItem.eCraftType == ITEMCRAFTTYPE_Aging && mouseIteMData->sItem.bMaturing == FALSE && mouseIteMData->sItem.sAgeLevel < 20) ||
						  mouseIteMData->sItem.eCraftType == ITEMCRAFTTYPE_None) )
					{



						int iAgingPrice = ITEMHANDLER->GetItemAgingPrice(&mouseIteMData->sItem, mouseIteMData->sItem.sAgeLevel);
						iAgingPrice = static_cast<int>(static_cast<float>(iAgingPrice) / 2.0f);

						//DEBUG("IS AN ITEM");

						AGINGWINDOWHANDLER->UpdateItemPrice(iAgingPrice);


						if ((CHARACTERGAME->CanUseGold(iAgingPrice) && (UNITDATA->sCharacterData.iGold - iAgingPrice >= 0)) == FALSE)
						{
							ITEMWINDOWHANDLER->SetTradeItemColorBox(ETradeColorBox::TRADECOLORBOX_NotEnoughGold);

							//DEBUG("NOT ENOUGH GOLD");
						}
					}
				}
			}
		}
	}

	return fnTradeLButtonDown(p_This, p_MouseX, p_MouseY);
}

/*
JLM - commented out for now. We won't allow item aging for sub-gear
BUT this incomplete code could be userful later if we want
to have more control over the aging window.

extern int __stdcall CustomSetAgingItemAreaCheck(ItemData* p_ItemData);

signed int __stdcall CustomSetAgingItemAreaCheck(ItemData* p_ItemData)
{
	typedef signed int(__stdcall* fnSetAgingItemAreaCheck)(ItemData* pcItemData);
	fnSetAgingItemAreaCheck rsSetAgingItemAreaCheck = (fnSetAgingItemAreaCheck)0x0050E950;

	//auto before = ITEMWINDOWHANDLER->GetTradeItemColorBox();

	//signed int result1 = rsSetAgingItemAreaCheck(p_ItemData);

	//auto after = ITEMWINDOWHANDLER->GetTradeItemColorBox();

	ITEMWINDOWHANDLER->SetTradeItemColorBox(ETradeColorBox::TRADECOLORBOX_None);
	(*(DWORD*)0x036EFCA4) = 0;
	// Init Trade Color Box
	CALL(0x0050BDC0);

	RECT sRect{ 58, 193 + (*(int*)0x0082D3A0), 58 + (3 * 22), 193 + (*(int*)0x0082D3A0) + (4 * 22) };
	ITEMWINDOWHANDLER->SetTradeRect(&sRect);

	// Inside box?
	Point2D* psPoint = MOUSEHANDLER->GetPosition();

	if (sRect.left <= psPoint->iX && sRect.right > psPoint->iX && sRect.top <= psPoint->iY && sRect.bottom > psPoint->iY)
	{
		// Set Trade Color sBox
		RECT* psRect = (RECT*)(0x036EFCAC);
		psRect->left = sRect.left;
		psRect->top = sRect.top;
		psRect->right = sRect.right - sRect.left;
		psRect->bottom = sRect.bottom - sRect.top;


		// Place Item in the new position
		p_ItemData->sPlacePosition.iX = sRect.left + ((sRect.right - sRect.left) - p_ItemData->iWidth) / 2;
		p_ItemData->sPlacePosition.iY = sRect.top + ((sRect.bottom - sRect.top) - p_ItemData->iHeight) / 2;
		p_ItemData->iItemSlot = 0;
		ITEMWINDOWHANDLER->SetTradeItemColorBox(ETradeColorBox::TRADECOLORBOX_SetItem);

		if ((*(DWORD*)0x036EFCA4) = ITEMWINDOWHANDLER->CollideItemTrade(psRect, 0, 2))
		{
			ITEMWINDOWHANDLER->SetTradeItemColorBox(ETradeColorBox::TRADECOLORBOX_NotSetItem);
			return FALSE;
		}
	}

	return TRUE;
}*/

void PatchCallToFunction(DWORD l_CallAddress, DWORD l_localFunctionAddress)
{
	DWORD l_PatchSize = 5;
	DWORD l_OldFlags;


	VirtualProtect((void *)l_CallAddress, l_PatchSize, PAGE_EXECUTE_READWRITE, &l_OldFlags);
	*reinterpret_cast<DWORD *>(l_CallAddress + 1) = l_localFunctionAddress - (DWORD)l_CallAddress - l_PatchSize;
	VirtualProtect((void *)l_CallAddress, l_PatchSize, l_OldFlags, NULL);
}

void PatchCallToThisFunction(DWORD l_CallAddress, DWORD l_localFunctionAddress)
{
	DWORD l_PatchSize = 5;
	DWORD l_OldFlags;

	VirtualProtect((void*)l_CallAddress, l_PatchSize, PAGE_EXECUTE_READWRITE, &l_OldFlags);
	/// So here for each address to patch in array l_SubGoldCalls
	/// We write 0xE8
	/// Then the current relative distance to our function. i see
	*reinterpret_cast<BYTE*> (l_CallAddress) = 0xE8;
	*reinterpret_cast<DWORD*>(l_CallAddress + 1) = l_localFunctionAddress - (DWORD)l_CallAddress - l_PatchSize;
	VirtualProtect((void*)l_CallAddress, l_PatchSize, l_OldFlags, NULL);
}

signed int __cdecl CustomEnterBellatraCastle(int p_Money)
{
    Packet l_Packet;
    l_Packet.iHeader = PKTHDR_EnterBellatraCastle;
    l_Packet.iLength = sizeof(Packet);

    SOCKETG->SendPacket(&l_Packet);

    return 0;
}

/// <summary>
/// Leaked func: int scITEM::SetTransItem( TRANS_ITEM *lpTransItem )
/// Set the received item
/// </summary>
int __fastcall CustomSetTransItem(DropItemView * pDropItemView, void * pEmptyPlaceHolder, PacketStageItemData::StageItemData * pStageItemData )
{
	//if (pDropItemView->iItemID.ToInt() != 0 )
		//CHATBOX->AddMessage( FormatString("CustomSetTransItem: %s", pStageItemData->szName) );


	UNITDATA->dwLastActiveTime = TICKCOUNT;

	ZeroMemory( pDropItemView, sizeof( DropItemView ) );

	pDropItemView->bFormatted = 0x0;
	pDropItemView->bActive = TRUE;
	pDropItemView->eItemFlags = pStageItemData->eItemFlags;
	pDropItemView->eRarity = (EItemRarity)pStageItemData->eRarity;

	pDropItemView->dwLastUpdate = READDWORD( 0xCF4750 );

	lstrcpy( pDropItemView->szName, pStageItemData->szName );

	CALLT_WITH_ARG3( 0x428F40, (DWORD)pDropItemView, pStageItemData->iX, pStageItemData->iY, pStageItemData->iZ ); //SetPosi

	pDropItemView->eItemFlags = pStageItemData->eItemFlags;
	pDropItemView->iClass = pStageItemData->iClass;
	pDropItemView->iItemID = pStageItemData->iItemID;

	pDropItemView->sAngle.iY = ( ( pDropItemView->sPosition.iX + pDropItemView->sPosition.iZ ) >> 2 ) & PTANGLE_Mask;


	if ( (pDropItemView->eItemFlags & EItemFlag::TreasureHuntingMode) == EItemFlag::TreasureHuntingMode ||
		 (pDropItemView->eItemFlags & EItemFlag::ForceAllowDrop) == EItemFlag::ForceAllowDrop)
	{
		lstrcpy( pDropItemView->szModelName, "Image\\sinImage\\Items\\DropItem\\itSP107.ASE" );
	}
	else
	{
		//0x505C60 = sinGetItemInfo - get model name from item id
		if ( CALL_WITH_ARG2( 0x505C60, (DWORD)pDropItemView->iItemID.ToInt(), (DWORD)&pDropItemView->szModelName ) == FALSE ||  pDropItemView->iItemID.ToInt() == 0)
		{
			lstrcpy( pDropItemView->szModelName, "char\\flag\\flag.ini" );
		}
		else
		{
			std::string fn = pDropItemView->szModelName;
			std::string fnWithoutExt = fn.substr( 0, fn.find_last_of( '.' ) );
			std::string fileSearch1 = fnWithoutExt + ".smd";
			std::string fileSearch2 = fnWithoutExt + ".ase";

			HANDLE hFile;
			WIN32_FIND_DATA sFindData;

			//first check smd (more common)
			hFile = FindFirstFileA( fileSearch1.c_str(), &sFindData );
			if ( hFile == INVALID_HANDLE_VALUE )
			{
				FindClose( hFile );

				//check ase
				hFile = FindFirstFileA( fileSearch2.c_str(), &sFindData );
				if ( hFile == INVALID_HANDLE_VALUE )
				{
#ifdef DEV_MODE
					CHATBOX->AddMessage( FormatString( "ERROR> Invalid drop image: %s", pDropItemView->szModelName ) );
#endif

					lstrcpy( pDropItemView->szModelName, "Image\\sinImage\\Items\\DropItem\\itSP107.ASE" );        //todo - use a different image?
				}
			}

			FindClose( hFile );
		}
	}



	if ( !pDropItemView->bHidden )
	{
		CALL_WITH_ARG2( 0x43C4A0, (DWORD)pDropItemView, (DWORD)pDropItemView->szModelName ); // 0x43C4A0 = AddLoaderItem
	}

	return TRUE;

	//typedef BOOL ( __thiscall * tfnSetTransItem )( DropItemView * pDropItemView, PacketStageItemData::StageItemData *  pStageItemData );
	//tfnSetTransItem fnSetTransItem = (tfnSetTransItem)0x0043CA70;
	//return fnSetTransItem (pDropItemView, pStageItemData );
}



/// <summary>
/// Custom sinThrowItemToFeild function
/// to bypass the assembly one.
/// See int sinThrowItemToFeild(sITEM *pItem) in leaked code
/// Address of function is 4605B0
/// </summary>
/// <param name="p_Money"></param>
/// <returns></returns>
signed int __cdecl CustomThrowItemToField(ItemData * pItem)
{
	//CHATBOX->AddMessage(FormatString("Caught item: %s, %d", pItem->szBaseName, pItem.));
	return 1;
}

/// <summary>
/// Patch all 64 calls in assembly for sinThrowItemToFeild
/// </summary>
void PatchThrowItemToFieldFunctions()
{
	//disable for now.
	return;

	//these are all the calls to sinThrowItemToFeild
	DWORD l_Calls[] = {

		0x00419077,0x00493351,0x004962A9,0x00496521,0x004B84AB,0x004B8815,0x004B88E7,0x004B90AC,0x004B93F9,0x004B9651,
		0x004B983B,0x004B9F38,0x004BA627,0x004BBC51,0x004BBD90,0x004BBF7C,0x004BC753,0x004BC7AA,0x004BC7DC,0x004BC891,
		0x004C16AA,0x004C19A0,0x004C1CD2,0x004C1F6B,0x004C1FF0,0x004C253D,0x004C260C,0x004C292E,0x004C338D,0x004C38EB,
		0x004CE679,0x004CE6C1,0x004CE716,0x004CE763,0x004D3905,0x004D4CFD,0x004D4D63,0x004D4ED9,0x00509C4B,0x0050A49C,
		0x0050A4E9,0x0050A551,0x0050C288,0x0050C398,0x0050C3AB,0x0050C479,0x0050DA6B,0x0050E772,0x00510016,0x005102ED,
		0x005104F1,0x005105A0,0x00510624,0x005115C1,0x005117F2,0x0051190D,0x00511E92,0x0051253B,0x00512F62,0x00513B72,
		0x00519AC9,0x006226DF,0x0062D87E,0x0062F0D1
	};
	int l_CallsCount = sizeof(l_Calls) / sizeof(l_Calls[0]);

	for (int l_I = 0; l_I < l_CallsCount; ++l_I)
	{
		PatchCallToThisFunction(l_Calls[l_I], (DWORD)CustomThrowItemToField);
	}
}

#define RECV_TRANSITEM_QUE_MASK 255



/// <summary>
/// Custom CheckRecvTrnsItemQue function
/// to inject code to check if there is space in inventory
/// This code is called after:
///  - picking up an item that was dropped via monster kill
///  - picking up an item that was dropped via player
/// Does not get called when
///  - buying item from NPC shop
///  - withdrawing item from wh
///  - withdrawing item from item distributor
///  - getting loot from boss
///  - trading items from / to players
/// Important note:
/// - if returned FALSE here, the item will not be 'picked' up and processed by server
/// </summary>
signed int CustomCheckRecvTrnsItemQue()
{
	DropItemView * lpMsTraceItem_CF48A8 = ( *(DropItemView **)0x00CF48A8 );

	if ( lpMsTraceItem_CF48A8 )
	{
		EItemType lItemType = lpMsTraceItem_CF48A8->iItemID.ToItemType();
		ItemData * temp = ITEMHANDLER->FindItemTableByCode( lpMsTraceItem_CF48A8->iItemID.ToInt() );

		//check weight limit
		if ( lItemType != EItemType::ITEMTYPE_GoldAndExp &&
			!CHARACTERGAME->CheckInventoryWeight( temp ) )
		{
			TITLEBOX( "Weight limit exceeded" );
			return FALSE;
		}

		//skip space checks for gold, health, mana, stamina
		if ( lItemType != EItemType::ITEMTYPE_GoldAndExp &&
			 lItemType != EItemType::ITEMTYPE_HealthPotion &&
			 lItemType != EItemType::ITEMTYPE_ManaPotion &&
			 lItemType != EItemType::ITEMTYPE_StaminaPotion )
		{
			// Check Space
			if ( !CHARACTERGAME->CheckInventorySpace( temp ) )
			{
				//No inventory space.
				//Do we have space in equipment slot?
				if ( !CHARACTERGAME->CheckEquipmentSpace( temp ) )
				{
					TITLEBOX( "Not enough space in inventory" );
					return FALSE;
				}
			}
		}
	}

	struct TRANS_ITEMINFO
	{
		int	size,code;
		Item Item;
		int x,y,z;
		DWORD	dwSeCode[4];
	};

	DWORD cnt;
	int mCnt;

	DWORD RecvTransItemQue_Pop = READDWORD( 0x03A977BC );
	DWORD RecvTransItemQue_Push = READDWORD( 0x03A977B8 );

	TRANS_ITEMINFO * Recv_TransItemInfo_Que = (TRANS_ITEMINFO *)0x39C8750;

	for ( cnt = RecvTransItemQue_Pop; cnt < RecvTransItemQue_Push; cnt++ )
	{
		mCnt = cnt & RECV_TRANSITEM_QUE_MASK;

		if ( Recv_TransItemInfo_Que[mCnt].Item.sItemID.ToInt() != 0 )
		{
			return FALSE;
		}
	}

	return TRUE;
}

void PatchBytes(DWORD p_Address, std::vector<BYTE> bytes)
{
	DWORD l_OldFlags;
	VirtualProtect((void*)p_Address, bytes.size(), PAGE_EXECUTE_READWRITE, &l_OldFlags);
	for ( size_t l_I = 0; l_I < bytes.size(); ++l_I )
	{
		*reinterpret_cast<BYTE *>( p_Address + l_I ) = (BYTE)bytes[l_I];
	}
	VirtualProtect((void*)p_Address, bytes.size(), l_OldFlags, NULL);
}


void PatchMemoryNOP(DWORD p_Address, std::size_t p_Size)
{
	DWORD l_OldFlags;
	VirtualProtect((void*)p_Address, p_Size, PAGE_EXECUTE_READWRITE, &l_OldFlags);
	for (size_t l_I = 0; l_I < p_Size; ++l_I)
		*reinterpret_cast<BYTE*>(p_Address + l_I) = 0x90;	///< NOP
	VirtualProtect((void*)p_Address, p_Size, l_OldFlags, NULL);
}

void OnLoadServerSkillDataContainer ( PacketSkillDataContainer * psSkillDataContainer )
{
	if ( psSkillDataContainer->iCharacterClass == UNITDATA->sCharacterData.iClass )
	{
		CHATBOX->AddDebugMessage ( FormatString ( "Load %d skill data for %s", psSkillDataContainer->iCount, GetCharacterClassStringByClassEnum ( (ECharacterClass)psSkillDataContainer->iCharacterClass ) ) );
	}

	for ( int i = 0; i < psSkillDataContainer->iCount; i++ )
	{
		SkillArrayData * data = &psSkillDataContainer->skillArrayData[i];
		SKILLMANAGERHANDLER->LoadServerSkillData ( data );
	}

	if ( psSkillDataContainer->iCharacterClass == UNITDATA->sCharacterData.iClass )
	{
		//Reset skill info box
		SKILLINFOBOX->Invalidate ();
	}
}

void OnLoadServerSkillChainDataContainer( PacketGenericContainer * psContainer )
{
	CHATBOX->AddDebugMessage( "Load skill chain data: %d", psContainer->iCount );

	UINT uPosition = 0;

	for ( int i = 0; i < psContainer->iCount; i++ )
	{
		SkillChainScaleData * data = reinterpret_cast<SkillChainScaleData *>( psContainer->baBuffer + uPosition );

		SKILLMANAGERHANDLER->LoadServerSkillChainData ( data );
		uPosition += sizeof( SkillChainScaleData );
	}
}

/// <summary>
/// Common skill info for all players
/// Usually party buffs etc
/// </summary>
void OnLoadServerCommonSkillInfoContainer( PacketCommonSkillInfoContainer * psContainer )
{
	CHATBOX->AddDebugMessage ( "Load %d common skill info", psContainer->iCount );

	for ( int i = 0; i < psContainer->iCount; i++ )
	{
		SkillInfoCommon * data = &psContainer->saData[i];
		SKILLMANAGERHANDLER->LoadServerCommonSkillInfo ( data );
	}
}

/// <summary>
/// Skill info for current character.
/// This should be called after OnLoadServerCommonSkillInfoContainer
/// </summary>
void OnLoadServerSkillInfoContainer ( PacketSkillInfoContainer * psSkillInfoContainer )
{
	if ( psSkillInfoContainer->iCharacterClass == UNITDATA->sCharacterData.iClass )
	{
		CHATBOX->AddDebugMessage ( "Load %d skill info for %s", psSkillInfoContainer->iCount, GetCharacterClassStringByClassEnum ( (ECharacterClass)psSkillInfoContainer->iCharacterClass ) );
	}

	for ( int i = 0; i < psSkillInfoContainer->iCount; i++ )
	{
		SkillInfo * data = &psSkillInfoContainer->saSkillInfo[i];
		SKILLMANAGERHANDLER->LoadServerSkillInfo ( data );
	}

	//also update skil cooldown values for self
	//otherwise anti-cheat will get triggered.
	if ( psSkillInfoContainer->iCharacterClass == UNITDATA->sCharacterData.iClass )
	{
		CAntiCheat::UpdateSkillCooldown ();

		//will also call ReformSkillInfo. This is needed to avoid cheat detection too
		SKILLMANAGERHANDLER->SaveCheckSkill ();

		//CALL cSKILL::Init() which in turn will call LoadUseSkillImage
		//Calling Init will also set the MatIcon id for skill buttons (not just for timers)
		//CALLT ( 0x504AE0, 0x367E148 );

		CALLT( 0x4ED7F0, 0x367E148 ); //0x4ED7F0 is cSKILL::SearchUseSkill() where 0x367E148 is the Skill object. This will reload mats for skill button icons
		CALLT( 0x4E9860, 0x367E148 ); //0x4E9860 is cSKILL::Load() where 0x367E148 is the Skill object.


		Skill * sDefaultSkill = (Skill *)SKILLSCHARACTER + 0; //first skill is the default skill. @todo - restore first skill from last session.

		WRITEDWORD( 0x3673B68, (DWORD)sDefaultSkill );    //sinSkill.pLeftSkill = &sinSkill.UseSkill[0];
		WRITEDWORD( 0x3670DEC, 1 ); //sinSkill.UseSkill[0].MousePosi = 1;
		WRITEDWORD (0x3670DF0, 1)   //sinSkill.pLeftSkill->Position = 1;


		//Reset skill info box
		SKILLINFOBOX->Invalidate ();
	}
}

BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		//For ExpansionSkillRender
		PatchBytes( 0x00421160, std::vector<BYTE>{0x60, 0x55, 0xFF, 0x15, 0x98, 0x83, 0xAF, 0x04, 0x83, 0xC4, 0x04, 0x61, 0xE9, 0x87, 0x01, 0x00, 0x00, 0x90, 0x90} );

		//0x04AF8398 is the

		//PatchMemoryNOP( 0x00421160, 19 );

		PatchCallsToFunction_PlayAttack();
		PatchCallsToFunction_UnitDataInit();
		PatchCallsToFunction_PlaySkillLoop();
		PatchCallsToFunction_GetAttackFrameSpeed ();
		PatchCallsToFunction_GetAttackSpeedMainFrame ();
		PatchCallsToFunction_SelectRangeToExcludeMainTarget ();
		PatchCallsToFunction_MoveAngle2 ();
		PatchCallsToFunction_SavePlayBuff ();
		PatchCallsToFunction_SavePlayBuff2 ();
		PatchCallsToFunction_MakePlayBuffFromRecvData ();
		PatchCallToFunction(0x00453263, (DWORD)CustomCheckRecvTrnsItemQue);

		//Patch SetTransItem to give us more control
		PatchCallToThisFunction(0x0062D482, (DWORD)CustomSetTransItem);

		//Disable drain of mana upon hit when energy shield is active
		PatchMemoryNOP( 0x0042BDC2, 5 );

		//Remove StopAssaCodeEffect calls for HOLY_ACTION and DRASTIC_ACTION from ASM
		PatchMemoryNOP( 0x005B78EB, 0x18 );
		PatchMemoryNOP( 0x005B79B9, 5 );
		PatchMemoryNOP( 0x005B7FEB, 0x18 );
		PatchMemoryNOP( 0x005B8095, 5 );
		PatchMemoryNOP( 0x005B80A2, 5 );
		PatchMemoryNOP( 0x005B78F2, 5 );
		PatchMemoryNOP( 0x005B78FE, 5 );


		//Disable loading of timer mat in cSKill::Init() as this is handled elsewhere now.
		//PatchMemoryNOP( 0x00504B10, 0x41 );
		//00504B10	0x41	85 F6 74 2F 56 68 BC 6F 7C 00 8D 44 24 14 68 10 E7 7B 00 50 FF D7 6A 01 6A 00 6A 00 6A 00 8D 4C 24 2C 6A 00 51 E8 06 56 F8 FF 83 C4 28 89 86 84 00 00 00 81 C6 10 02 00 00 81 FE 9C 23 99 00 7C BF 	90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90

		//DMG Boost:
		//PatchInt ( (PVOID)0x00993E58, 222 );

		/*PatchInt ( (PVOID)0x00993BD8, 223 ); //Archer_ScoutHawk_Rotations = 0x00993BD8

		//prevent XorAttackTrans from being called via PlayAttack in assembly
		//since the packet is already decoded
		//PatchMemoryNOP( 0x0042B61B, 5 );
		//PatchMemoryNOP( 0x0042B94D, 5 );
		PatchInt ( (PVOID)0x00993A20, 113 ); //Knight_SwordBlast_Range = 0x00993A20,
		PatchInt ( (PVOID)0x00998278, 224 ); //Knight_SwordBlast_Range = 0x00998278,
		PatchInt ( (PVOID)0x00998DE0, 335 ); //Knight_SwordBlast_Range = 0x00998DE0,

		//disable SendSetHackUser3 (8580) which spams the server
		PatchMemoryNOP( 0x0042B68C, 5 );


		PatchInt((PVOID)0x009987C8,111); //Shaman_DarkBolt_DMGBoost = 0x009987C8,
		PatchInt((PVOID)0x04B07140,112); //Shaman_DarkBolt_Range = 0x04B07140,
		PatchInt((PVOID)0x009987A0,113); //Shaman_DarkBolt_UseMana = 0x009987A0,


		PatchInt((PVOID)0x00998840,111); //Shaman_DarkWave_DMGBoost = 0x00998840,
		PatchInt((PVOID)0x009988B8,112); //Shaman_DarkWave_Range = 0x009988B8,
		PatchInt((PVOID)0x00998890,113); //Shaman_DarkWave_ATKRange = 0x00998890,
		PatchInt((PVOID)0x00998818,114); //Shaman_DarkWave_UseMana = 0x00998818,

		PatchInt((PVOID)0x00998930,111); //Shaman_Inertia_SpeedReduct = 0x00998930,
		PatchInt((PVOID)0x00998980,112); //Shaman_Inertia_Area = 0x00998980,
		PatchInt((PVOID)0x00998908,113); //Shaman_Inertia_Duration = 0x00998908,
		PatchInt((PVOID)0x009988E0,114); //Shaman_Inertia_UseMana = 0x009988E0,

		PatchInt((PVOID)0x04B0D950,1111); //Shaman_InnerPeace_MPPotionBoost = 0x04B0D950,
		PatchInt((PVOID)0x009989A8,112); //Shaman_InnerPeace_AddMana = 0x009989A8,


		PatchInt((PVOID)0x009989F8,111); //Shaman_SpiritualFlare_DMGBoost = 0x009989F8,
		PatchInt((PVOID)0x00998A70,112); //Shaman_SpiritualFlare_Range = 0x00998A70,
		PatchInt((PVOID)0x00998A48,113); //Shaman_SpiritualFlare_ATKRange = 0x00998A48,
		PatchInt((PVOID)0x009989D0,114); //Shaman_SpiritualFlare_UseMana = 0x009989D0,

		PatchInt((PVOID)0x00998AC0,111); //Shaman_SpiritualManacle_Duration = 0x00998AC0,
		PatchInt((PVOID)0x00998A98,112); //Shaman_SpiritualManacle_UseMana = 0x00998A98,


		PatchInt((PVOID)0x04B0DBA4,999); //Shaman_ChasingHunt_AddAtkPowMin = 0x04B0DBA4,
		PatchInt((PVOID)0x04B0DBCC,112); //Shaman_ChasingHunt_AddAtkPowMax = 0x04B0DBCC,
		PatchInt((PVOID)0x00998B38,113); //Shaman_ChasingHunt_Range = 0x00998B38,
		PatchInt((PVOID)0x00998B10,114); //Shaman_ChasingHunt_Duration = 0x00998B10,
		PatchInt((PVOID)0x00998AE8,115); //Shaman_ChasingHunt_UseMana = 0x00998AE8,

		PatchInt((PVOID)0x00998BB0,111); //Shaman_AdventMigal_AddATKPow = 0x00998BB0,
		PatchInt((PVOID)0x00998B88,112); //Shaman_AdventMigal_Duration = 0x00998B88,
		PatchInt((PVOID)0x00998B60,113); //Shaman_AdventMigal_UseMana = 0x00998B60,

		PatchInt((PVOID)0x00998C50,111); //Shaman_Rainmaker_ReduceDamage = 0x00998C50,
		PatchInt((PVOID)0x04B06F50,667); //Shaman_Rainmaker_AddEvasion = 0x04B06F50,
		PatchInt((PVOID)0x00998C28,113); //Shaman_Rainmaker_ReduceMP = 0x00998C28,
		PatchInt((PVOID)0x00998C00,114); //Shaman_Rainmaker_Duration = 0x00998C00,
		PatchInt((PVOID)0x00998BD8,115); //Shaman_Rainmaker_UseMana = 0x00998BD8,

		PatchInt((PVOID)0x00998CC8,111); //Shaman_PhantomCall_DMGBoost = 0x00998CC8,
		PatchInt((PVOID)0x00998CA0,112); //Shaman_PhantomCall_Range = 0x00998CA0,
		PatchInt((PVOID)0x00998C78,113); //Shaman_PhantomCall_UseMana = 0x00998C78,

		PatchInt((PVOID)0x00998D40,111); //Shaman_Haunt_LifeAbsorb = 0x00998D40,
		PatchInt((PVOID)0x00998D18,112); //Shaman_Haunt_UseMana = 0x00998D18,

		PatchInt((PVOID)0x00998DE0,111); //Shaman_Scratch_DMGBoost = 0x00998DE0,
		PatchInt((PVOID)0x00998DB8,112); //Shaman_Scratch_UseMana = 0x00998DB8,

		PatchInt((PVOID)0x00998F70,111); //Shaman_SummonHestian_Duration =  0x00998F70,
		PatchInt((PVOID)0x00998EA8,112); //Shaman_SummonHestian_ATKPowMin = 0x00998EA8,
		PatchInt((PVOID)0x00998ED0,119); //Shaman_SummonHestian_ATKPowMax = 0x00998ED0,
		PatchInt((PVOID)0x00998E80,113); //Shaman_SummonHestian_AtkRating = 0x00998E80,
		PatchInt((PVOID)0x00998E58,114); //Shaman_SummonHestian_HP = 0x00998E58,
		PatchInt((PVOID)0x00998E30,115); //Shaman_SummonHestian_UseMana = 0x00998E30,


		PatchInt((PVOID)0x00998F48,111); //Shaman_Judgement_DMGBoost = 0x00998F48,
		PatchInt((PVOID)0x04B07460,556); //Shaman_Judgement_AddCritical = 0x04B07460,
		PatchInt((PVOID)0x00998F20,113); //Shaman_Judgement_Range = 0x00998F20,
		PatchInt((PVOID)0x00998EF8,114); //Shaman_Judgement_UseMana = 0x00998EF8,

		PatchInt((PVOID)0x00998FC0,111); //Shaman_AdventMidranda_ATKSPDBoost = 0x00998FC0,
		PatchInt((PVOID)0x00998FE8,112); //Shaman_AdventMidranda_Duration = 0x00998FE8,
		PatchInt((PVOID)0x00998F98,113); //Shaman_AdventMidranda_UseMana = 0x00998F98,

		PatchInt((PVOID)0x00999060,111); //Shaman_MourningPray_DMGBoost = 0x00999060,
		PatchInt((PVOID)0x00999038,112); //Shaman_MourningPray_ATKRange = 0x00999038,
		PatchInt((PVOID)0x00999010,113); //Shaman_MourningPray_UseMana = 0x00999010,




		PatchInt((PVOID)0x04B0CB40,111); //Priestess_Healing_LifeRecovery = 0x04B0CB40,
		PatchInt((PVOID)0x00996900,112); //Priestess_Healing_LifeRecoveryPercent = 0x00996900,
		PatchInt((PVOID)0x009968D4,113); //Priestess_Healing_UseMana = 0x009968D4,


		PatchInt((PVOID)0x04B0CB70,222); //Priestess_HolyBolt_DMGBoost = 0x04B0CB70,
		PatchInt((PVOID)0x00996978,112); //Priestess_HolyBolt_DMGBoostPercent = 0x00996978,
		PatchInt((PVOID)0x00996950,113); //Priestess_HolyBolt_UseMana = 0x00996950,

		PatchInt((PVOID)0x04B0CBA0,555); //Priestess_MultiSpark_DMG = 0x04B0CBA0,
		PatchInt((PVOID)0x009969F0,112); //Priestess_MultiSpark_DMGPercent = 0x009969F0,
		//PatchInt((PVOID)//new,113);    //Priestess_MultiSpark_NumSparksMin = //new,
		PatchInt((PVOID)0x00996A40,114); //Priestess_MultiSpark_NumSparksMax = 0x00996A40,
		PatchInt((PVOID)0x009969C8,115); //Priestess_MultiSpark_UseMana = 0x009969C8,


		PatchInt((PVOID)0x00996A90,111); //Priestess_HolyMind_ReduceDamage = 0x00996A90,
		//PatchInt((PVOID)//new,112);    //Priestess_HolyMind_Duration = //new,
		PatchInt((PVOID)0x00996A68,113); //Priestess_HolyMind_UseMana = 0x00996A68,

		PatchInt((PVOID)0x04B0D8B0,999); //Priestess_Meditation_MPPotionBoost = 0x04B0D8B0,
		PatchInt((PVOID)0x00996AE0,112); //Priestess_Meditation_AddMPRegen = 0x00996AE0 // float,

		//PatchInt((PVOID)//need new and hook in asm,111); //Priestess_DivineLightning_ATKRange = //need new and hook in asm,
		PatchInt((PVOID)0x04B0CBD0,112); //Priestess_DivineLightning_DMGBoost = 0x04B0CBD0,
		PatchInt((PVOID)0x00996B58,113); //Priestess_DivineLightning_Numberofbolts = 0x00996B58,
		PatchInt((PVOID)0x00996B80,114); //Priestess_DivineLightning_UseMana = 0x00996B80,

		PatchInt((PVOID)0x00996BA8,111); //Priestess_HolyReflection_ReflectDMG = 0x00996BA8,
		PatchInt((PVOID)0x00996BD0,112); //Priestess_HolyReflection_Duration = 0x00996BD0,
		PatchInt((PVOID)0x00996BF8,113); //Priestess_HolyReflection_UseMana = 0x00996BF8,

		PatchInt((PVOID)0x04B0CC00,333); //Priestess_GrandHealing_LifeRecovery = 0x04B0CC00,
		PatchInt((PVOID)0x00996C20,112); //Priestess_GrandHealing_LifeRecoveryPercent = 0x00996C20,
		PatchInt((PVOID)0x00996C70,113); //Priestess_GrandHealing_Range = 0x00996C70,
		PatchInt((PVOID)0x00996C98,114); //Priestess_GrandHealing_UseMana = 00996C98,

		PatchInt((PVOID)0x04B06C70,888); //Priestess_VigorBall_AtkRtgBoost = 0x04B06C70,
		PatchInt((PVOID)0x00996CC0,112); //Priestess_VigorBall_DMGBoost = 0x00996CC0,
		PatchInt((PVOID)0x00996D10,113); //Priestess_VigorBall_UseMana = 0x00996D10,

		PatchInt((PVOID)0x00996D38,111); //Priestess_Resurrection_SuccessRate = 0x00996D38,
		PatchInt((PVOID)0x04B0DE60,778); //Priestess_Resurrection_MaxPlayers = 0x04B0DE60,
		PatchInt((PVOID)0x00996D88,113); //Priestess_Resurrection_UseMana = 0x00996D88,

		PatchInt((PVOID)0x00996DB0,111); //Priestess_Extinction_SuccessRate = 0x00996DB0,
		PatchInt((PVOID)0x00996DD8,112); //Priestess_Extinction_HPReduction = 0x00996DD8,
		PatchInt((PVOID)0x00996E00,113); //Priestess_Extinction_UseMana = 0x00996E00,

		PatchInt((PVOID)0x00996E28,111); //Priestess_VirtualLife_IncreaseLife = 0x00996E28,
		PatchInt((PVOID)0x00996E50,112); //Priestess_VirtualLife_Duration = 0x00996E50,
		PatchInt((PVOID)0x00996E78,113); //Priestess_VirtualLife_UseMana = 0x00996E78,

		PatchInt((PVOID)0x00996EA0,111); //Priestess_GlacialSpike_ATKPowMin = 0x00996EA0,
		PatchInt((PVOID)0x00996EC8,112); //Priestess_GlacialSpike_ATKPowMax = 0x00996EC8,
		PatchInt((PVOID)0x00996EF0,113); //Priestess_GlacialSpike_ATKRange = 0x00996EF0,
		PatchInt((PVOID)0x04B0BA20,669); //Priestess_GlacialSpike_FreezeTime = 0x04B0BA20,
		PatchInt((PVOID)0x00996F18,115); //Priestess_GlacialSpike_UseMana = 0x00996F18,

		PatchFloat((PVOID)0x00996F40,111.0f); //Priestess_RegenerationField_AddLifeRegen = 0x00996F40,
		PatchFloat((PVOID)0x00996F68,112.0f); //Priestess_RegenerationField_AddManaRegen = 0x00996F68,
		PatchInt((PVOID)0x00996F90,113); //Priestess_RegenerationField_Duration = 0x00996F90,
		PatchInt((PVOID)0x00996FB8,114); //Priestess_RegenerationField_Range = 0x00996FB8,
		PatchInt((PVOID)0x00996FE0,115); //Priestess_RegenerationField_UseMana = 0x00996FE0,

		PatchInt((PVOID)0x00997008,111); //Priestess_ChainLightning_DMGBoost = 0x00997008,
		PatchInt((PVOID)0x00997058,112); //Priestess_ChainLightning_Numberofbolts = 0x00997058,
		PatchInt((PVOID)0x00997080,113); //Priestess_ChainLightning_Range = 0x00997080,
		PatchInt((PVOID)0x009970A8,114); //Priestess_ChainLightning_UseMana = 0x009970A8,

		PatchInt((PVOID)0x009970D0,111); //Priestess_SummonMuspell_MuspellDMG = 0x009970D0,
		PatchInt((PVOID)0x04B0DC64,889); //Priestess_SummonMuspell_AddAtkPowMin = 0x04B0DC64,
		PatchInt((PVOID)0x04B0DC8C,113); //Priestess_SummonMuspell_AddAtkPowMax = 0x04B0DC8C,
		PatchInt((PVOID)0x04B0DCB4,114); //Priestess_SummonMuspell_VLReduct = 0x04B0DCB4,
		PatchInt((PVOID)0x00997120,115); //Priestess_SummonMuspell_AtkDelay = 0x00997120,
		PatchInt((PVOID)0x00997170,111); //Priestess_SummonMuspell_AddEvasion = 0x00997170,
		PatchInt((PVOID)0x00997198,112); //Priestess_SummonMuspell_Duration = 0x00997198,
		PatchInt((PVOID)0x009971C0,113); //Priestess_SummonMuspell_UseMana = 0x009971C0,
		PatchInt((PVOID)0x00997148,114); //Priestess_SummonMuspell_UndeadABS = 0x00997148,



		//Duration - all of them are 300
		PatchInt ( (PVOID)0x009926C0, 1 );
		PatchInt ( (PVOID)0x00992800, 2 );
		PatchInt ( (PVOID)0x00992918, 3 );
		PatchInt ( (PVOID)0x009929B8, 4 );
		PatchInt ( (PVOID)0x00992AD0, 5 );
		PatchInt ( (PVOID)0x00992C10, 6 );
		PatchInt ( (PVOID)0x00992D78, 7 );
		PatchInt ( (PVOID)0x00992E18, 8 );
		PatchInt ( (PVOID)0x00992E68, 9 );


		PatchInt ( (PVOID)0x00995550, 22 );
		PatchInt ( (PVOID)0x00995668, 23 );
		PatchInt ( (PVOID)0x009957F8, 24 );



		PatchInt ( (PVOID)0x009979E0, 34 );
		PatchInt ( (PVOID)0x00997C10, 35 );

		PatchInt ( (PVOID)0x009990D8, 43 );


		PatchInt ( (PVOID)0x00995C30, 111 ); //Magician_Agony_HPTaken = 0x00995C30,
		PatchInt ( (PVOID)0x04B0BCEC, 112 ); //Magician_Agony_MPAdded = 0x4b0bcec,
		PatchInt ( (PVOID)0x00995C08, 113 ); //Magician_Agony_UseMana = 0x00995C08,


		PatchInt ( (PVOID)0x04B0CA50, 111 ); //Magician_FireBolt_DMGBoost = 0x04B0CA50,
		PatchInt ( (PVOID)0x00995C80, 333 ); //Magician_FireBolt_DMGBoostPercent = 0x00995C80,
		PatchInt ( (PVOID)0x009961F8, 113 ); //Magician_FireBolt_Range = 0x009961F8 //new from unused asm array
		PatchInt ( (PVOID)0x00995C58, 114 ); //Magician_FireBolt_UseMana = 0x00995C58,

		PatchInt ( (PVOID)0x04B06C44, 888 ); //Magician_Zenith_AddAbsorb = 0x04B06C44,

		PatchInt ( (PVOID)0x00995CF8, 112 ); //Magician_Zenith_AddElement = 0x00995CF8,
		PatchInt ( (PVOID)0x00995CF8, 112 ); //Magician_Zenith_AddElement = 0x00995CF8,
		PatchInt ( (PVOID)0x00995D20, 113 ); //Magician_Zenith_Duration = 0x00995D20,
		PatchInt ( (PVOID)0x00995CD0, 114 ); //Magician_Zenith_UseMana = 0x00995CD0,

		PatchInt ( (PVOID)0x04B0CA80, 111 ); //Magician_FireBall_DMGBoost = 0x04B0CA80,
		PatchInt ( (PVOID)0x00995D70, 112 ); //Magician_FireBall_DMGBoostPercent = 0x00995D70,
		PatchInt ( (PVOID)0x00995DE8, 113 ); //Magician_FireBall_Range = 0x00995DE8,
		PatchInt ( (PVOID)0x00995DC0, 114 ); //Magician_FireBall_Area = 0x00995DC0,
		PatchInt ( (PVOID)0x00995D48, 115 ); //Magician_FireBall_UseMana = 0x00995D48,

		PatchInt ( (PVOID)0x04B0D910, 444 ); //Magician_MentalMastery_MPPotionBoost = 0x04B0D910,

		PatchInt ( (PVOID)0x00995E10, 112 ); //Magician_MentalMastery_AddMPRegen = 0x00995E10,

		PatchInt ( (PVOID)0x00995E38, 111 ); //Magician_Watornado_DMGBoost = 0x00995E38,
		PatchInt ( (PVOID)0x00995EB0, 112 ); //Magician_Watornado_Range = 0x00995EB0 ,
		PatchInt ( (PVOID)0x00995E88, 113 ); //Magician_Watornado_Area = 0x00995E88,
		PatchInt ( (PVOID)0x00995ED8, 114 ); //Magician_Watornado_UseMana = 0x00995ED8,

		PatchInt ( (PVOID)0x00995F50, 111 ); //Magician_EnchantWeapon_FireDamageMinMax = 0x00995F50, (min-max)
		PatchInt ( (PVOID)0x00995F00, 112 ); //Magician_EnchantWeapon_IceDamageMinMax = 0x00995F00, (min-max)
		PatchInt ( (PVOID)0x00995FA0, 113 ); //Magician_EnchantWeapon_LightDamageMinMax = 0x0995FA0, (min-max)
		PatchInt ( (PVOID)0x00995FF0, 114 ); //Magician_EnchantWeapon_Duration = 0x00995FF0,
		PatchInt ( (PVOID)0x00996018, 115 ); //Magician_EnchantWeapon_UseMana = 0x00996018,


		PatchInt ( (PVOID)0x00996040, 111 ); //Magician_DeathRay_DMGBoost = 0x00996040,
		PatchInt ( (PVOID)0x00996090, 112 ); //Magician_DeathRay_Range = 0x00996090,
		PatchInt ( (PVOID)0x009960B8, 113 ); //Magician_DeathRay_UseMana = 0x009960B8,


		PatchInt ( (PVOID)0x009960E0, 111 ); //Magician_EnergyShield_ReduceDamage = 0x009960E0,
		PatchInt ( (PVOID)0x00996108, 112 ); //Magician_EnergyShield_Duration = 0x00996108,
		PatchInt ( (PVOID)0x00996130, 113 ); //Magician_EnergyShield_UseMana = 0x00996130,

		PatchInt ( (PVOID)0x00996158, 111 ); //Magician_Diastrophism_DMGBoost = 0x00996158,
		PatchInt ( (PVOID)0x009961A8, 112 ); //Magician_Diastrophism_Area = 0x009961A8,
		PatchInt ( (PVOID)0x009961D0, 113 ); //Magician_Diastrophism_UseMana = 0x009961D0,

		PatchInt ( (PVOID)0x00996220, 112 ); //Magician_SpiritElemental_Duration = 0x00996220,
		PatchInt ( (PVOID)0x00996248, 113 ); //Magician_SpiritElemental_UseMana = 0x00996248,
		PatchInt ( (PVOID)0x04B0DC04, 114 ); //Magician_SpiritElemental_IncreaseAtkPowMin = 0x04B0DC04,
		PatchInt ( (PVOID)0x04B0DC2C, 115 ); //Magician_SpiritElemental_IncreaseAtkPoweMax = 0x04B0DC2C,

		PatchInt ( (PVOID)0x009962E8, 111 ); //Magician_DancingSword_FireDamage = 0x009962E8,
		PatchInt ( (PVOID)0x00996298, 112 ); //Magician_DancingSword_IceDamage = 0x00996298,
		PatchInt ( (PVOID)0x00996338, 113 ); //Magician_DancingSword_AtkDelay = 0x00996338,
		PatchInt ( (PVOID)0x00996360, 114 ); //Magician_DancingSword_Duration = 0x00996360,
		PatchInt ( (PVOID)0x00996388, 115 ); //Magician_DancingSword_UseMana = 0x00996388,

		PatchInt ( (PVOID)0x00996608, 111); //Magician_FireElemental_Duration = 0x00996608, //from unused asm array
		PatchInt ( (PVOID)0x009963B0, 112 ); //Magician_FireElemental_ATKPowMinMax = 0x009963B0, //min-max
		PatchInt ( (PVOID)0x00996400, 113 ); //Magician_FireElemental_AtkRating = 0x00996400,
		PatchInt ( (PVOID)0x00996428, 114 ); //Magician_FireElemental_HP = 0x00996428,
		PatchInt ( (PVOID)0x00996450, 115 ); //Magician_FireElemental_UseMana = 0x00996450,

		PatchInt ( (PVOID)0x00996478, 111 ); //Magician_FlameWave_DMGBoost = 0x00996478,
		PatchInt ( (PVOID)0x009964C8, 112 ); //Magician_FlameWave_Area = 0x009964C8,
		PatchInt ( (PVOID)0x009964F0, 113 ); //Magician_FlameWave_UseMana = 0x009964F0,

		PatchInt ( (PVOID)0x00996518, 111 ); //Magician_Distortion_SpeedReduct = 0x00996518,
		PatchInt ( (PVOID)0x00996568, 112 ); //Magician_Distortion_Area = 0x00996568,
		PatchInt ( (PVOID)0x00996590, 113 ); //Magician_Distortion_Duration = 0x00996590,

		PatchInt ( (PVOID)0x009965B8, 114 ); //Magician_Distortion_UseMana = 0x009965B8,

		PatchInt ( (PVOID)0x009965E0, 111 ); //Magician_Meteorite_DMGBoost = 0x009965E0,
		PatchInt ( (PVOID)0x00996630, 112 ); //Magician_Meteorite_Area = 0x00996630,
		PatchInt ( (PVOID)0x00996658, 113 ); //Magician_Meteorite_UseMana = 0x00996658,





		PatchInt ( (PVOID)0x00993250, 111 ); //Knight_SwordBlast_DMGBoost = 0x00993250 //use this, and override skill info box
		PatchInt ( (PVOID)0x00993278, 112 ); //Knight_SwordBlast_DMGBoostPercent = 0x00993278, //use this, and override skill info box
		PatchInt ( (PVOID)0x00993200, 113 ); //Knight_SwordBlast_Range = 0x00993200,
		PatchInt ( (PVOID)0x00993200, 114 ); //Knight_SwordBlast_UseMana = 0x00993200,



		PatchInt ( (PVOID)0x009932C8, 111 ); //Knight_HolyBody_DMGReduction = 0x009932C8,
		PatchInt ( (PVOID)0x009932F0, 112 ); //Knight_HolyBody_Duration = 0x009932F0,
		PatchInt ( (PVOID)0x009932A0, 113 ); //Knight_HolyBody_UseMana = 0x009932A0,



		PatchInt ( (PVOID)0x00993318, 111 ); //Knight_PhysicalTraining_AddStamina = 0x00993318,

		PatchInt ( (PVOID)0x00993368, 111 ); //Knight_DoubleCrash_DMGBoost = 0x00993368,
		PatchInt ( (PVOID)0x00993390, 112 ); //Knight_DoubleCrash_AddCritical = 0x00993390,
		PatchInt ( (PVOID)0x00993340, 113 ); //Knight_DoubleCrash_UseMana = 0x00993340,


		//PatchInt ( (PVOID)1 //add new,111); //Knight_HolyValor_PartyArea = 1 //use new - to override static value in asm,
		PatchInt ( (PVOID)0x009933B8, 112 ); //Knight_HolyValor_FinalDMGBoost = 0x009933B8,

		PatchInt ( (PVOID)0x009933E0, 113 ); //Knight_HolyValor_Duration = 0x009933E0,
		PatchInt ( (PVOID)0x00993408, 114 ); //Knight_HolyValor_UseMana = 0x00993408,

		PatchInt ( (PVOID)0x00993430, 111 ); //Knight_Brandish_Area = 0x00993430,
		PatchInt ( (PVOID)0x00993458, 112 ); //Knight_Brandish_DMGBoost = 0x00993458,

		PatchInt ( (PVOID)0x00993480, 113 ); //Knight_Brandish_UseMana = 0x00993480,


		PatchInt ( (PVOID)0x009934A8, 111 ); //Knight_Piercing_DMGBoost = 0x009934A8,
		PatchInt ( (PVOID)0x009934D0, 112 ); //Knight_Piercing_Area = 0x009934D0,
		PatchInt ( (PVOID)0x009934F8, 114 ); //Knight_Piercing_UseMana = 0x009934F8,

		PatchInt ( (PVOID)0x00993520, 111 ); //Knight_DrasticSpirit_AddDefense = 0x00993520,
		PatchInt ( (PVOID)0x00993548, 112 ); //Knight_DrasticSpirit_Duration = 0x00993548,
		PatchInt ( (PVOID)0x00993570, 113 ); //Knight_DrasticSpirit_UseMana = 0x00993570,

		PatchInt ( (PVOID)0x00993598, 111 ); //Knight_SwordMastery_AddWeaponDamagePercent = 0x00993598,


		PatchInt ( (PVOID)0x009935E8, 111 ); //Knight_DivineShield_AddBlock = 0x009935E8,
		PatchInt ( (PVOID)0x009935C0, 112 ); //Knight_DivineShield_ConvertLife = 0x009935C0,
		PatchInt ( (PVOID)0x00993610, 113 ); //Knight_DivineShield_Duration = 0x00993610,
		PatchInt ( (PVOID)0x00993638, 114 ); //Knight_DivineShield_UseMana = 0x00993638,


		PatchInt ( (PVOID)0x00993660, 111 ); //Knight_HolyIncantation_SuccessRate = 0x00993660,
		PatchInt ( (PVOID)0x00993688, 112 ); //Knight_HolyIncantation_LifeIncrease = 0x00993688,
		PatchInt ( (PVOID)0x009936B0, 113 ); //Knight_HolyIncantation_Duration = 0x009936B0,
		PatchInt ( (PVOID)0x009936D8, 114 ); //Knight_HolyIncantation_UseMana = 0x009936D8,

		PatchInt ( (PVOID)0x00993700, 111 ); //Knight_GrandCross_AtkRtgBoost = 0x00993700,
		PatchInt ( (PVOID)0x00993728, 112 ); //Knight_GrandCross_DMGBoost = 0x00993728,
		PatchInt ( (PVOID)0x00993750, 113 ); //Knight_GrandCross_UndeadDamage = 0x00993750,
		PatchInt ( (PVOID)0x00993778, 114 ); //Knight_GrandCross_UseMana = 0x00993778,

		PatchInt ( (PVOID)0x00993930, 111 ); //Knight_SwordofJustice_DMGBoost = 0x00993930,
		PatchInt ( (PVOID)0x00993958, 112 ); //Knight_SwordofJustice_Area = 0x00993958,
		PatchInt ( (PVOID)0x00993980, 113 ); //Knight_SwordofJustice_UseMana = 0x00993980,

		PatchInt ( (PVOID)0x00993840, 111 ); //Knight_GodlyShield_ShieldABSBoost = 0x00993840,
		PatchInt ( (PVOID)0x00993868, 112 ); //Knight_GodlyShield_Duration = 0x00993868,

		PatchInt ( (PVOID)0x00993890, 113 ); //Knight_GodlyShield_UseMana = 0x00993890,
		PatchInt ( (PVOID)0x009938B8, 111 ); //Knight_GodsBlessing_AddATKPow = 0x009938B8,
		PatchInt ( (PVOID)0x009938E0, 112 ); //Knight_GodsBlessing_Duration = 0x009938E0,
		PatchInt ( (PVOID)0x00993908, 113 ); //Knight_GodsBlessing_UseMana = 0x00993908,

		PatchInt ( (PVOID)0x04B06C18, 444 ); //Knight_DivinePiercing_AddCritical = 0x04B06C18,
		PatchInt ( (PVOID)0x009937A0, 112 ); //Knight_DivinePiercing_DMGBoost = 0x009937A0,
		PatchInt ( (PVOID)0x009937F0, 113 ); //Knight_DivinePiercing_Numberofattacks = 0x009937F0,
		PatchInt ( (PVOID)0x00993818, 114 ); //Knight_DivinePiercing_UseMana = 0x00993818,












		PatchInt ( (PVOID)0x009947E0, 111 ); //Atalanta_ShieldStrike_ATKPowMinMax = 0x009947E0, (min-max)
		PatchInt ( (PVOID)0x009947B8, 113 ); //Atalanta_ShieldStrike_UseMana = 0x009947B8,

		PatchInt ( (PVOID)0x04B0B7F8, 555 ); //Atalanta_Farina_DMGBoost = 0x04B0B7F8,
		PatchInt ( (PVOID)0x00994880, 112 ); //Atalanta_Farina_AddATKSpeed = 0x00994880,
		PatchInt ( (PVOID)0x009948A8, 113 ); //Atalanta_Farina_AddAtkRtg = 0x009948A8,
		PatchInt ( (PVOID)0x00994858, 114 ); //Atalanta_Farina_UseMana = 0x00994858,




		PatchInt ( (PVOID)0x00994920, 111 ); //Atalanta_ThrowingMastery_DMGBoost = 0x00994920,

		PatchInt ( (PVOID)0x00994920, 111 ); //Atalanta_BiggerSpear_DMGBoost = 0x00994920, (min-max)
		PatchInt ( (PVOID)0x009948F8, 112 ); //Atalanta_BiggerSpear_UseMana = 0x009948F8,


		PatchInt ( (PVOID)0x00994970, 111 ); //Atalanta_Windy_AtkRtgBoost = 0x00994970,
		PatchInt ( (PVOID)0x00994998, 113 ); //Atalanta_Windy_Duration = 0x00994998,
		PatchInt ( (PVOID)0x009949C0, 114 ); //Atalanta_Windy_UseMana = 0x009949C0,

		PatchInt ( (PVOID)0x009949E8, 111 ); //Atalanta_TwistJavelin_DMGBoost = 0x009949E8,
		PatchInt ( (PVOID)0x00994A10, 112 ); //Atalanta_TwistJavelin_AtkRtgBoost = 0x00994A10,
		PatchInt ( (PVOID)0x00994A38, 113 ); //Atalanta_TwistJavelin_UseMana = 0x00994A38,

		PatchInt ( (PVOID)0x00994A60, 111 ); //Atalanta_SoulSucker_LifeAbsorb = 0x00994A60,
		PatchInt ( (PVOID)0x00994A88, 112 ); //Atalanta_SoulSucker_UseMana = 0x00994A88,

		PatchInt ( (PVOID)(0x00994920 + ( 10 * 4 )), 112 ); //Atalanta_FireJavelin_DMGBoost = 0x00994920 + (10 * 4), //Offset from Bigger Spear
		PatchInt ( (PVOID)0x00994B00, 112 ); //Atalanta_FireJavelin_UseMana = 0x00994B00,


		PatchInt ( (PVOID)0x00994B28, 111 ); //Atalanta_SplitJavelin_NumberOfAttacks = 0x00994B28,
		PatchInt ( (PVOID)0x00994B50, 112 ); //Atalanta_SplitJavelin_DMGBoost = 0x00994B50,
		PatchInt ( (PVOID)0x00994B78, 113 ); //Atalanta_SplitJavelin_AtkRtgBoost = 0x00994B78,
		PatchInt ( (PVOID)0x00994BA0, 114 ); //Atalanta_SplitJavelin_UseMana = 0x00994BA0,

		PatchInt ( (PVOID)0x00994BC8, 111 ); //Atalanta_TriumphofValhalla_MaxAtkPow = 0x00994BC8,
		PatchInt ( (PVOID)0x00994BF0, 112 ); //Atalanta_TriumphofValhalla_Duration = 0x00994BF0,
		PatchInt ( (PVOID)0x00994C18, 113 ); //Atalanta_TriumphofValhalla_UseMana = 0x00994C18,


		PatchInt ( (PVOID)0x00994C40, 113 ); //Atalanta_LightningJavelin_DMGBoost = 0x00994C40,
		PatchInt ( (PVOID)0x00994C90, 112 ); //Atalanta_LightningJavelin_UseMana = 0x00994C90,

		PatchInt ( (PVOID)0x00994CB8, 111 ); //Atalanta_StormJavelin_DMGBoost = 0x00994CB8,
		PatchInt ( (PVOID)0x00994CE0, 112 ); //Atalanta_StormJavelin_UseMana = 0x00994CE0,


		PatchInt ( (PVOID)0x00994D08, 111 ); //Atalanta_HallofValhalla_AddEvasion = 0x00994D08,
		PatchInt ( (PVOID)0x00994D30, 112 ); //Atalanta_HallofValhalla_Duration = 0x00994D30,
		PatchInt ( (PVOID)0x00994D58, 113 ); //Atalanta_HallofValhalla_UseMana = 0x00994D58,

		PatchInt ( (PVOID)0x00994D80, 111 ); //Atalanta_ExtremeRage_DMGBoost = 0x00994D80,
		PatchInt ( (PVOID)0x00994DA8, 112 ); //Atalanta_ExtremeRage_Area = 0x00994DA8,
		PatchInt ( (PVOID)0x00994DD0, 113 ); //Atalanta_ExtremeRage_UseMana = 0x00994DD0,

		PatchInt ( (PVOID)0x00994DF8, 111 ); //Atalanta_FrostJavelin_AtkPowMinMax = 0x00994DF8, (min-max)
		PatchInt ( (PVOID)0x00994E48, 112 ); //Atalanta_FrostJavelin_SpeedMonDown = 0x00994E48,
		PatchInt ( (PVOID)0x00994E70, 113 ); //Atalanta_FrostJavelin_FreezeTime = 0x0994E70,
		PatchInt ( (PVOID)0x00994E98, 114 ); //Atalanta_FrostJavelin_Duration = 0x00994E98,
		PatchInt ( (PVOID)0x00994EC0, 115 ); //Atalanta_FrostJavelin_UseMana = 0x00994EC0,

		PatchInt ( (PVOID)0x00994EE8, 111 ); //Atalanta_Vengeance_DMGBoost = 0x00994EE8,
		PatchInt ( (PVOID)0x00994F38, 112 ); //Atalanta_Vengeance_AddRange = 0x00994F38,
		PatchInt ( (PVOID)0x00994F10, 113 ); //Atalanta_Vengeance_AddCritical = 0x00994F10,
		PatchInt ( (PVOID)0x00994F60, 114 ); //Atalanta_Vengeance_UseMana = 0x00994F60,




		PatchInt ( (PVOID)0x00997F80, 111 ); //Assassin_Stinger_DMGBoost = 0x00997F80,
		PatchInt ( (PVOID)0x00997F58, 112 ); //Assassin_Stinger_UseMana = 0x00997F58,

		PatchInt ( (PVOID)0x00997FF8, 111 ); //Assassin_RunningHit_DMGBoost = 0x00997FF8,
		PatchInt ( (PVOID)0x00997FD0, 112 ); //Assassin_RunningHit_UseMana = 0x00997FD0,

		PatchInt ( (PVOID)0x00998020, 111 ); //Assassin_DualWieldMastery_WPAtkPowAdd = 0x00998020,

		PatchInt ( (PVOID)0x00998098, 111 ); //Assassin_Wisp_DecreaseDMG = 0x00998098,
		PatchInt ( (PVOID)0x00998070, 112 ); //Assassin_Wisp_Range = 0x00998070,
		PatchInt ( (PVOID)0x00998048, 114 ); //Assassin_Wisp_UseMana = 0x00998048,

		PatchInt ( (PVOID)0x009980E8, 111 ); //Assassin_VenomThorn_DMGBoost = 0x009980E8,
		PatchInt ( (PVOID)0x009980C0, 112 ); //Assassin_VenomThorn_UseMana = 0x009980C0,

		PatchInt ( (PVOID)0x00998160, 111 ); //Assassin_Alas_AddEvasion = 0x00998160,
		PatchInt ( (PVOID)0x00998138, 112 ); //Assassin_Alas_Duration = 0x0x00998138,
		PatchInt ( (PVOID)0x00998110, 113 ); //Assassin_Alas_UseMana = 0x00998110,

		PatchInt ( (PVOID)0x009981B0, 111 ); //Assassin_SoulShock_Circle = 0x009981B0,
		PatchInt ( (PVOID)0x009981D8, 112 ); //Assassin_SoulShock_Duration = 0x009981D8,
		PatchInt ( (PVOID)0x00998188, 113 ); //Assassin_SoulShock_UseMana = 0x00998188,

		PatchInt ( (PVOID)0x00998228, 111 ); //Assassin_AttackMastery_WPAtkRtgBoost = 0x00998228,
		PatchInt ( (PVOID)0x00998200, 112 ); //Assassin_AttackMastery_WPBlockBoost = 0x00998200,

		PatchInt ( (PVOID)0x00998278, 111 ); //Assassin_SoreBlade_DMGBoost = 0x00998278,
		PatchInt ( (PVOID)0x00998250, 112 ); //Assassin_SoreBlade_UseMana = 0x00998250,

		PatchInt ( (PVOID)0x04B07040, 111 ); //Assassin_GustSlash_Area = 0x04B07040,
		PatchInt ( (PVOID)0x009982C8, 112 ); //Assassin_GustSlash_DMGBoost = 0x009982C8,
		PatchInt ( (PVOID)0x009982A0, 113 ); //Assassin_GustSlash_UseMana = 0x009982A0,

		PatchInt ( (PVOID)0x00998318, 111 ); //Assassin_Inpes_ATKSPDBoost = 0x00998318,
		PatchInt ( (PVOID)0x00998340, 112 ); //Assassin_Inpes_Duration = 0x0x00998340,
		PatchInt ( (PVOID)0x009982F0, 113 ); //Assassin_Inpes_UseMana = 0x009982F0,

		PatchInt ( (PVOID)0x009983B8, 111 ); //Assassin_Blind_AfterDamage = 0x009983B8,
		PatchInt ( (PVOID)0x009983E0, 112 ); //Assassin_Blind_MonsterSight = 0x009983E0,
		PatchInt ( (PVOID)0x00998390, 113 ); //Assassin_Blind_Duration = 0x0x00998390,
		PatchInt ( (PVOID)0x00998368, 114 ); //Assassin_Blind_UseMana = 0x00998368,

		PatchInt ( (PVOID)0x00998430, 111 ); //Assassin_FrostWind_DMGBoost = 0x00998430,
		PatchInt ( (PVOID)0x00998408, 112 ); //Assassin_FrostWind_UseMana = 0x00998408,

		PatchInt ( (PVOID)0x00998458, 111 ); //Assassin_FatalMastery_WPCritBoost = 0x00998458,

		PatchInt ( (PVOID)0x009984D0, 111 ); //Assassin_Polluted_DMGBoost = 0x009984D0,
		PatchInt ( (PVOID)0x00998548, 112 ); //Assassin_Polluted_Maxhits = 0x00998548,
		PatchInt ( (PVOID)0x00998520, 113 ); //Assassin_Polluted_Duration = 0x00998520,
		PatchInt ( (PVOID)0x009984A8, 114 ); //Assassin_Polluted_Range = 0x009984A8,
		PatchInt ( (PVOID)0x00998480, 115 ); //Assassin_Polluted_UseMana = 0x00998480,

		PatchInt ( (PVOID)0x00998598, 111 ); //Assassin_NinjaShadow_DMGBoost = 0x00998598,
		PatchInt ( (PVOID)0x00998570, 112 ); //Assassin_NinjaShadow_UseMana = 0x00998570,





		PatchInt ( (PVOID)0x009974E0, 111 ); //Pikeman_PikeWind_ATKPowMinMax = 0x009974E0, //min-max
		PatchInt ( (PVOID)0x00997530, 113 ); //Pikeman_PikeWind_Pushback = 0x00997530,
		PatchInt ( (PVOID)0x00997558, 114 ); //Pikeman_PikeWind_UseMana = 0x00997558,

		PatchInt ( (PVOID)0x00997580, 111 ); //Pikeman_IceAttribute_Frost = 0x00997580,

		PatchInt ( (PVOID)0x009975A8, 111 ); //Pikeman_CriticalHit_AddCriticalDMG = 0x009975A8,
		PatchInt ( (PVOID)0x009975D0, 112 ); //Pikeman_CriticalHit_UseMana = 0x009975D0,

		PatchInt ( (PVOID)0x00997620, 111 ); //Pikeman_JumpingCrash_WPAtkRtgBoost = 0x00997620,
		PatchInt ( (PVOID)0x00997648, 112 ); //Pikeman_JumpingCrash_DMGBoost = 0x00997648,
		PatchInt ( (PVOID)0x009975F8, 113 ); //Pikeman_JumpingCrash_UseMana = 0x009975F8,

		PatchInt ( (PVOID)0x00997710, 111 ); //Pikeman_GroundPike_FreezeTime = 0x00997710,
		PatchInt ( (PVOID)0x00997670, 112 ); //Pikeman_GroundPike_FrostAttackMinMax = 0x00997670, //min-max
		PatchInt ( (PVOID)0x009976C0, 114 ); //Pikeman_GroundPike_Range = 0x009976C0,
		PatchInt ( (PVOID)0x009976E8, 115 ); //Pikeman_GroundPike_UseMana = 0x009976E8,

		PatchInt ( (PVOID)0x00997738, 111 ); //Pikeman_Tornado_DMGBoost = 0x00997738,
		PatchInt ( (PVOID)0x00997788, 112 ); //Pikeman_Tornado_Range = 0x00997788,
		PatchInt ( (PVOID)0x009977B0, 113 ); //Pikeman_Tornado_UseMana = 0x009977B0,

		PatchInt ( (PVOID)0x009977D8, 111 ); //Pikeman_WeaponBlockMastery_WPBlockBoost = 0x009977D8,

		PatchInt ( (PVOID)0x00997800, 111 ); //Pikeman_Expansion_AddSize = 0x00997800,
		PatchInt ( (PVOID)0x00997828, 112 ); //Pikeman_Expansion_DMGBoost = 0x00997828,
		PatchInt ( (PVOID)0x00997850, 113 ); //Pikeman_Expansion_UseMana = 0x00997850,

		PatchInt ( (PVOID)0x00997878, 111 ); //Pikeman_VenomSpear_DMGBoost = 0x00997878,
		PatchInt ( (PVOID)0x009978C8, 112 ); //Pikeman_VenomSpear_Numberofspears = 0x009978C8,
		PatchInt ( (PVOID)0x009978F0, 113 ); //Pikeman_VenomSpear_Duration = 0x009978F0,
		PatchInt ( (PVOID)0x00997918, 114 ); //Pikeman_VenomSpear_Range = 0x00997918,
		PatchInt ( (PVOID)0x00997940, 115 ); //Pikeman_VenomSpear_UseMana = 0x00997940,

		PatchInt ( (PVOID)0x00997990, 111 ); //Pikeman_Vanish_AfterDamage = 0x00997990,
		PatchInt ( (PVOID)0x009979B8, 112 ); //Pikeman_Vanish_MonsterSight = 0x009979B8,
		PatchInt ( (PVOID)0x009979E0, 113 ); //Pikeman_Vanish_Duration = 0x009979E0,
		PatchInt ( (PVOID)0x00997A08, 114 ); //Pikeman_Vanish_UseMana = 0x00997A08,

		PatchInt ( (PVOID)0x00997A30, 111 ); //Pikeman_CriticalMastery_WPCritBoost = 0x00997A30,

		PatchInt ( (PVOID)0x00997A58, 111 ); //Pikeman_ChainLancer_DMGBoost = 0x00997A58,
		PatchInt ( (PVOID)0x00997A80, 112 ); //Pikeman_ChainLancer_UseMana = 0x00997A80,

		PatchInt ( (PVOID)0x00997AA8, 111 ); //Pikeman_AssassinsEye_AddCriticalDMG = 0x00997AA8,

		PatchInt ( (PVOID)0x00997AD0, 112 ); //Pikeman_AssassinsEye_MonsterCritical = 0x00997AD0,
		PatchInt ( (PVOID)0x00997AF8, 113 ); //Pikeman_AssassinsEye_Duration = 0x0997AF8,
		PatchInt ( (PVOID)0x00997B20, 114 ); //Pikeman_AssassinsEye_UseMana = 0x00997B20,

		PatchInt ( (PVOID)0x00997B48, 111 ); //Pikeman_ChargingStrike_DMGBoost = 0x00997B48,
		PatchInt ( (PVOID)0x00997B70, 112 ); //Pikeman_ChargingStrike_ChargedDMGBst = 0x00997B70,
		PatchInt ( (PVOID)0x00997B98, 113 ); //Pikeman_ChargingStrike_RaiseSpeed = 0x00997B98,



		PatchInt ( (PVOID)0x00997BC0, 114 ); //Pikeman_ChargingStrike_UseMana = 0x00997BC0,

		PatchInt ( (PVOID)0x00997BE8, 111 ); //Pikeman_Vague_AddEvasion = 0x00997BE8,
		PatchInt ( (PVOID)0x00997C10, 112 ); //Pikeman_Vague_Duration = 0x00997C10,
		PatchInt ( (PVOID)0x00997C38, 113 ); //Pikeman_Vague_UseMana = 0x00997C38,

		PatchInt ( (PVOID)0x00997C60, 111 ); //Pikeman_ShadowMaster_DMGBoost = 0x00997C60,
		PatchInt ( (PVOID)0x00997CB0, 112 ); //Pikeman_ShadowMaster_Numofhits = 0x00997CB0,
		PatchInt ( (PVOID)0x00997CD8, 113 ); //Pikeman_ShadowMaster_UseMana = 0x00997CD8,

















		PatchInt ( (PVOID)0x00992698, 111 ); //Mech_ExtremeShield_BlockBoost = 0x00992698,
		PatchInt ( (PVOID)0x009926C0, 112 ); //Mech_ExtremeShield_Duration = 0x009926C0,
		PatchInt ( (PVOID)0x009926E8, 113 ); //Mech_ExtremeShield_UseMana = 0x009926E8,

		PatchInt ( (PVOID)0x04B0CB10, 111 ); //Mech_MechanicBomb_Damage = 0x04B0CB10,
		PatchInt ( (PVOID)0x00992710, 112 ); //Mech_MechanicBomb_DamagePercent = 0x00992710,
		PatchInt ( (PVOID)0x00992760, 113 ); //Mech_MechanicBomb_Area = 0x00992760, <<wrong
		PatchInt ( (PVOID)0x00992788, 114 ); //Mech_MechanicBomb_UseMana = 0x00992788,

		PatchInt ( (PVOID)0x00992850, 111 ); //Mech_PoisonAttribute_Poison = 0x00992850,

		PatchInt ( (PVOID)0x009927B0, 111 ); //Mech_PhysicalAbsorption_ABSBoost = 0x009927B0,
		PatchInt ( (PVOID)0x00992800, 112 ); //Mech_PhysicalAbsorption_Duration = 0x00992800,
		PatchInt ( (PVOID)0x00992828, 113 ); //Mech_PhysicalAbsorption_UseMana = 0x00992828,

		PatchInt ( (PVOID)0x009928A0, 111 ); //Mech_GreatSmash_Damage = 0x009928A0,
		PatchInt ( (PVOID)0x00992760, 112 ); //Mech_GreatSmash_Area = 0x00992760,
		PatchInt ( (PVOID)0x009928C8, 113 ); //Mech_GreatSmash_UseMana = 0x009928C8,


		PatchInt ( (PVOID)0x009928F0, 111 ); //Mech_Maximize_AddATKPow = 0x009928F0,
		PatchInt ( (PVOID)0x04B0C034, 778 ); //Mech_Maximize_AtkRtgBoost = 0x04B0C034,
		PatchInt ( (PVOID)0x00992918, 113 ); //Mech_Maximize_Duration = 0x00992918,
		PatchInt ( (PVOID)0x00992940, 114 ); //Mech_Maximize_UseMana = 0x00992940,

		PatchInt ( (PVOID)0x00992968, 111 ); //Mech_Automation_DMGBoost = 0x00992968,
		PatchInt ( (PVOID)0x00992990, 112 ); //Mech_Automation_AddATKSpeed = 0x00992990,


		PatchInt ( (PVOID)0x00992A08, 111 ); //Mech_Spark_DMGBoost = 0x00992A08,
		PatchInt ( (PVOID)0x00992A58, 112 ); //Mech_Spark_Area = 0x00992A58,
		PatchInt ( (PVOID)0x00992A80, 113 ); //Mech_Spark_UseMana = 0x00992A80,

		PatchInt ( (PVOID)0x00992AA8, 111 ); //Mech_MetalArmor_AddDefense = 0x00992AA8,
		PatchInt ( (PVOID)0x00992AD0, 112 ); //Mech_MetalArmor_Duration = 0x00992AD0,
		PatchInt ( (PVOID)0x00992AF8, 113 ); //Mech_MetalArmor_UseMana = 0x00992AF8,

		PatchInt ( (PVOID)0x00992B20, 111 ); //Mech_GrandSmash_AtkRtgBoost = 0x00992B20,
		PatchInt ( (PVOID)0x00992B48, 112 ); //Mech_GrandSmash_DMGBoost = 0x00992B48,
		PatchInt ( (PVOID)0x00992B70, 113 ); //Mech_GrandSmash_UseMana = 0x00992B70,

		PatchInt ( (PVOID)0x00992B98, 111 ); //Mech_MechanicWeaponMastery_DMGBoost = 0x00992B98,

		PatchInt ( (PVOID)0x00992BC0, 111 ); //Mech_SparkShield_SparkDamage = 0x00992BC0,
		PatchInt ( (PVOID)0x00992BE8, 112 ); //Mech_SparkShield_AddDefense = 0x00992BE8,
		PatchInt ( (PVOID)0x00992C10, 113 ); //Mech_SparkShield_Duration = 0x00992C10,
		PatchInt ( (PVOID)0x00992C38, 114 ); //Mech_SparkShield_UseMana = 0x00992C38,

		PatchInt ( (PVOID)0x00992C60, 111 ); //Mech_Impulsion_DMGBoost = 0x00992C60,
		PatchInt ( (PVOID)0x00992CB0, 112 ); //Mech_Impulsion_Area = 0x00992CB0,
		PatchInt ( (PVOID)0x00992CD8, 113 ); //Mech_Impulsion_MaxVolts = 0x00992CD8,
		PatchInt ( (PVOID)0x00992D00, 114 ); //Mech_Impulsion_UseMana = 0x00992D00,


		PatchInt ( (PVOID)0x00992d00, 111 ); //Mech_Compulsion_AddAbsorb =
		PatchInt ( (PVOID)0x00992D28, 112 ); //Mech_Compulsion_AddAbsorbPercent_Old = 0x00992D28,
		PatchInt ( (PVOID)0x00992D50, 113 ); //Mech_Compulsion_Range = 0x00992D50,
		PatchInt ( (PVOID)0x00992D78, 114 ); //Mech_Compulsion_Duration = 0x00992D78,
		PatchInt ( (PVOID)0x00992DA0, 115 ); //Mech_Compulsion_UseMana = 0x00992DA0,

		PatchInt ( (PVOID)0x00992DC8, 111 ); //Mech_MagneticSphere_SpheresDMG = 0x00992DC8,
		PatchInt ( (PVOID)0x00992E18, 112 ); //Mech_MagneticSphere_Range = 0x00992E18,
		PatchInt ( (PVOID)0x00992E40, 113 ); //Mech_MagneticSphere_AtkDelay = 0x00992E40,
		PatchInt ( (PVOID)0x00992E68, 114 ); //Mech_MagneticSphere_Duration = 0x00992E68,
		PatchInt ( (PVOID)0x00992E90, 115 ); //Mech_MagneticSphere_UseMana = 0x00992E90,

		PatchInt ( (PVOID)0x00992EB8, 113 ); //Mech_MetalGolem_ATKPowMinMax = 0x00992EB8, //min-max
		PatchInt ( (PVOID)0x00992F08, 111 ); //Mech_MetalGolem_AtkRating = 0x00992F08,
		PatchInt ( (PVOID)0x00992F30, 112 ); //Mech_MetalGolem_Defense = 0x00992F30,
		PatchInt ( (PVOID)0x00992F58, 113 ); //Mech_MetalGolem_HP = 0x00992F58,
		PatchInt ( (PVOID)0x00992F80, 114 ); //Mech_MetalGolem_UseMana = 0x00992F80,










		PatchInt ( (PVOID)0x00995230, 111 ); //Fighter_MeleeMastery_WPAtkPowAdd = 0x00995230,

		PatchInt ( (PVOID)0x00995258, 111 ); //Fighter_FireAttribute_Flame = 0x00995258,

		PatchInt ( (PVOID)0x00995280, 111 ); //Fighter_Raving_DMGBoost = 0x00995280,
		PatchInt ( (PVOID)0x009952A8, 112 ); //Fighter_Raving_Numofhits = 0x009952A8,
		PatchInt ( (PVOID)0x009952D0, 113 ); //Fighter_Raving_HPDecrease = 0x009952D0, //float
		PatchInt ( (PVOID)0x009952F8, 114 ); //Fighter_Raving_UseMana = 0x009952F8,


		PatchInt ( (PVOID)0x00995320, 111 ); //Fighter_Impact_AddAtkRtg = 0x00995320,
		PatchInt ( (PVOID)0x00995348, 112 ); //Fighter_Impact_DMGBoost = 0x00995348,
		PatchInt ( (PVOID)0x00995370, 113 ); //Fighter_Impact_UseMana = 0x00995370,

		PatchInt ( (PVOID)0x00995398, 111 ); //Fighter_TripleImpact_DMGBoost = 0x00995398,
		PatchInt ( (PVOID)0x009953C0, 112 ); //Fighter_TripleImpact_Numofhits = 0x009953C0,
		PatchInt ( (PVOID)0x009953E8, 113 ); //Fighter_TripleImpact_UseMana = 0x009953E8,

		PatchInt ( (PVOID)0x00995410, 111 ); //Fighter_BrutalSwing_DMGBoost = 0x00995410,
		PatchInt ( (PVOID)0x00995438, 112 ); //Fighter_BrutalSwing_AddCritical = 0x00995438,
		PatchInt ( (PVOID)0x00995460, 113 ); //Fighter_BrutalSwing_UseMana = 0x00995460,

		PatchInt ( (PVOID)0x00995488, 111 ); //Fighter_Roar_Circle = 0x00995488,
		PatchInt ( (PVOID)0x009954B0, 112 ); //Fighter_Roar_Duration = 0x009954B0,
		PatchInt ( (PVOID)0x009954D8, 113 ); //Fighter_Roar_UseMana = 0x009954D8,

		PatchInt ( (PVOID)0x00995500, 111 ); //Fighter_RageofZecram_Damage = 0x00995500,
		PatchInt ( (PVOID)0x00995528, 112 ); //Fighter_RageofZecram_UseMana = 0x00995528,

		PatchInt ( (PVOID)0x00995578, 111 ); //Fighter_Concentration_AtkRtgBoost = 0x00995578,
		PatchInt ( (PVOID)0x00995550, 112 ); //Fighter_Concentration_Duration = 0x00995550,
		PatchInt ( (PVOID)0x009955A0, 113 ); //Fighter_Concentration_UseMana = 0x009955A0,

		PatchInt ( (PVOID)0x009955F0, 111 ); //Fighter_AvengingCrash_AddAtkRtg = 0x009955F0,
		PatchInt ( (PVOID)0x009955C8, 112 ); //Fighter_AvengingCrash_DMGBoost = 0x009955C8,
		PatchInt ( (PVOID)0x00995618, 113 ); //Fighter_AvengingCrash_UseMana = 0x00995618,

		PatchInt ( (PVOID)0x00995640, 111 ); //Fighter_SwiftAxe_ATKSPDBoost = 0x00995640,
		PatchInt ( (PVOID)0x00995668, 112 ); //Fighter_SwiftAxe_Duration = 0x00995668,
		PatchInt ( (PVOID)0x00995690, 113 ); //Fighter_SwiftAxe_UseMana = 0x00995690,

		PatchInt ( (PVOID)0x009956B8, 111 ); //Fighter_BoneCrash_DMGBoost = 0x009956B8,
		PatchInt ( (PVOID)0x009956E0, 112 ); //Fighter_BoneCrash_+DMGDemons = 0x009956E0,
		PatchInt ( (PVOID)0x00995708, 113 ); //Fighter_BoneCrash_UseMana = 0x00995708,

		PatchInt ( (PVOID)0x00995730, 111 ); //Fighter_Destroyer_DMGBoost = 0x00995730,
		PatchInt ( (PVOID)0x00995758, 112 ); //Fighter_Destroyer_AddCritical = 0x00995758,
		PatchInt ( (PVOID)0x00995780, 113 ); //Fighter_Destroyer_UseMana = 0x00995780,

		PatchInt ( (PVOID)0x009957A8, 111 ); //Fighter_Berserker_AddATKPow = 0x009957A8,
		PatchInt ( (PVOID)0x009957D0, 112 ); //Fighter_Berserker_DecreaseAbs = 0x009957D0,
		PatchInt ( (PVOID)0x009957F8, 113 ); //Fighter_Berserker_Duration = 0x009957F8,
		PatchInt ( (PVOID)0x00995820, 114 ); //Fighter_Berserker_UseMana = 0x00995820,

		PatchInt ( (PVOID)0x00995848, 111 ); //Fighter_CycloneStrike_DMGBoost = 0x00995848,
		PatchInt ( (PVOID)0x009958C0, 112 ); //Fighter_CycloneStrike_Maxhits = 0x009958C0,
		PatchInt ( (PVOID)0x009958E8, 113 ); //Fighter_CycloneStrike_Range = 0x009958E8,
		PatchInt ( (PVOID)0x00995910, 114 ); //Fighter_CycloneStrike_UseMana = 0x00995910,

		PatchInt ( (PVOID)0x00995938, 111 ); //Fighter_BoostHealth_HPBoost = 0x00995938,
		PatchInt ( (PVOID)0x00995960, 112 ); //Fighter_BoostHealth_UseMana = 0x00995960,

		//Scout Hawk
		PatchInt ( (PVOID)0x00993BB0, 222 ); //Archer_ScoutHawk_AddAtkRtg = 0x00993BB0
		PatchInt ( (PVOID)0x00993BD8, 223 ); //Archer_ScoutHawk_Rotations = 0x00993BD8

		//Shooting Mastery
		PatchInt ( (PVOID)0x00993C28, 111 ); //Archer_ShootingMastery_DMGBoost = 0x00993C28

		//Wind Arrow
		PatchInt ( (PVOID)0x00993C50, 111 ); //Archer_WindArrow_AddATKSpeed = 0x00993C50
		PatchInt ( (PVOID)0x00993C78, 112 ); //Archer_WindArrow_DMGBoost = 0x00993C78

		//Perfect Aim
		PatchInt ( (PVOID)0x00993CC8, 111 ); //Archer_PerfectAim_AddAtkRtg = 0x00993CC8
		PatchInt ( (PVOID)0x00993CF0, 112 ); //Archer_PerfectAim_DMGBoost = 0x00993CF0

		//Dion's Eye
		PatchInt ( (PVOID)0x00993D68, 333 ); //Archer_DionsEye_AddAtkRtg = 0x00993D68

		//Falcon
		PatchInt ( (PVOID)0x04B0CAB0, 111 ); //Archer_Falcon_Damage = 0x04B0CAB0
		PatchInt ( (PVOID)0x00993D90, 223 ); //Archer_Falcon_DamagePercent = 0x00993D90
		PatchInt ( (PVOID)0x00993DE0, 15 ); //Archer_Falcon_Duration = 0x00993DE0

		//Arrow of Rage
		PatchInt ( (PVOID)0x00993E58, 111 ); //Archer_ArrowofRage_DMGBoost = 0x00993E58
		PatchInt ( (PVOID)0x00993E80, 112 ); //Archer_ArrowofRage_Area = 0x00993E80

		//Avalanche
		PatchInt ( (PVOID)0x00993ED0, 111 ); //Archer_Avalanche_AddATKSpeed = 0x00993ED0
		PatchInt ( (PVOID)0x00993EF8, 223 ); //Archer_Avalanche_DMGBoost = 0x00993EF8
		PatchInt ( (PVOID)0x00993F20, 113 ); //Archer_Avalanche_NumberofArrows = 0x00993F20

		//Fire & Light Damage
		PatchInt ( (PVOID)0x04B0CAE0, 666 ); //Archer_ElementalShot_FireLightDamage = 0x04B0CAE0

		//Fire & Light Damage Percent
		PatchInt ( (PVOID)0x00993FC0, 445 ); //Archer_ElementalShot_FireLightDamagePercent = 0x00993FC0

		//Falcon
		PatchInt ( (PVOID)0x00994088, 111 ); //Archer_GoldenFalcon_FalconDamage = 0x00994088
		PatchInt ( (PVOID)0x009940D8, 16 );  //Archer_Falcon_Duration = 0x009940D8
		PatchFloat ( (PVOID)0x009940B0, 112.0 ); //Archer_GoldenFalcon_HPRegen = 0x009940B0 //float


		//Bomb Shot
		PatchInt ( (PVOID)0x00994128, 333 ); //Archer_BombShot_Damage = 0x00994128
		PatchInt ( (PVOID)0x00994150, 112 ); //Archer_BombShot_DMGDemons = 0x00994150
		PatchInt ( (PVOID)0x009941C8, 113 ); //Archer_BombShot_Area = 0x009941C8

		//Perforation
		PatchInt ( (PVOID)0x00994290, 111 ); //Archer_Perforation_ShotRange = 0x00994290
		PatchInt ( (PVOID)0x00994218, 112 ); //Archer_Perforation_DMGBoost = 0x00994218

		//Wolverine
		PatchInt ( (PVOID)0x009942B8, 113 ); //Archer_Wolverine_ATKPow = 0x009942B8 //{min,max} list
		PatchInt ( (PVOID)0x00994308, 222 ); //Archer_Wolverine_AtkRating = 0x00994308,
		PatchInt ( (PVOID)0x00994330, 112 ); //Archer_Wolverine_Defense = 0x00994330,
		PatchInt ( (PVOID)0x00994358, 113 ); //Archer_Wolverine_HP = 0x00994358,

		//Evasion Mastery
		PatchInt ( (PVOID)0x009943A8, 222 ); //Archer_EvasionMastery_IncreaseEvasion = 0x009943A8,

		//Phoenix Shot
		PatchInt ( (PVOID)0x00994448, 111 ); //Archer_PhoenixShot_ShotRange = 0x00994448,
		PatchInt ( (PVOID)0x009943D0, 112 ); //Archer_PhoenixShot_DMGBoost = 0x009943D0,
		PatchInt ( (PVOID)0x009943F8, 224 ); //Archer_PhoenixShot_RaiseSpeed = 0x009943F8,

		//Force of Nature (group buff)
		PatchInt ( (PVOID)0x00994470, 555 ); //Archer_ForceofNature_AddATKPow = 0x00994470,
		PatchInt ( (PVOID)0x00994498, 112 ); //Archer_ForceofNature_AddAtkRtg = 0x00994498,
		PatchInt ( (PVOID)0x009944E8, 17 );  //Archer_ForceofNature_Duration = 0x009944E8,*/

		DWORD l_PatchAddress = 0x41D34C;
		size_t l_PatchSize = 4 + 1;

		DWORD l_OldFlags;
		VirtualProtect((void*)l_PatchAddress, l_PatchSize, PAGE_EXECUTE_READWRITE, &l_OldFlags);
		for (size_t l_I = 0; l_I < l_PatchSize; ++l_I)
			*reinterpret_cast<BYTE*>(l_PatchAddress + l_I) = 0x90; ///< ASM NOP
		VirtualProtect((void*)l_PatchAddress, l_PatchSize, l_OldFlags, NULL);

		//l_PatchAddress = 0x4E8941;
		//VirtualProtect((void*)l_PatchAddress, l_PatchSize, PAGE_EXECUTE_READWRITE, &l_OldFlags);
		//auto buffer = (char*)l_PatchAddress;
		//buffer[0] = 0x83;
		//buffer[1] = 0xE0;
		//buffer[2] = 0x00;
		//VirtualProtect((void*)l_PatchAddress, l_PatchSize, l_OldFlags, NULL);

		//////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////

		/// RepairInvenItem call 1/2
		l_PatchAddress = 0x4C5275;
		l_PatchSize = 4 + 1;
		VirtualProtect((void*)l_PatchAddress, l_PatchSize, PAGE_EXECUTE_READWRITE, &l_OldFlags);
		*reinterpret_cast<BYTE*>(l_PatchAddress) = 0xE8;
		*reinterpret_cast<DWORD*>(l_PatchAddress + 1) = (DWORD)CustomRepairInvenItem - (DWORD)l_PatchAddress - l_PatchSize;
		VirtualProtect((void*)l_PatchAddress, l_PatchSize, l_OldFlags, NULL);

		/// RepairInvenItem call 2/2
		l_PatchAddress = 0x4E93A1;
		l_PatchSize = 4 + 1;
		VirtualProtect((void*)l_PatchAddress, l_PatchSize, PAGE_EXECUTE_READWRITE, &l_OldFlags);
		*reinterpret_cast<BYTE*>(l_PatchAddress) = 0xE8;
		*reinterpret_cast<DWORD*>(l_PatchAddress + 1) = (DWORD)CustomRepairInvenItem - (DWORD)l_PatchAddress - l_PatchSize;
		VirtualProtect((void*)l_PatchAddress, l_PatchSize, l_OldFlags, NULL);

		//////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////


		/// Trade L Button Down call 1/1
		l_PatchAddress = 0x4CFE9A;
		l_PatchSize = 4 + 1;
		VirtualProtect((void*)l_PatchAddress, l_PatchSize, PAGE_EXECUTE_READWRITE, &l_OldFlags);
		*reinterpret_cast<BYTE*>(l_PatchAddress) = 0xE8;
		*reinterpret_cast<DWORD*>(l_PatchAddress + 1) = (DWORD)CustomTradeLButtonDown - (DWORD)l_PatchAddress - l_PatchSize;
		VirtualProtect((void*)l_PatchAddress, l_PatchSize, l_OldFlags, NULL);

		//////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////


		/// Set Aging Item Area Check call 1/1
		//l_PatchAddress = 0x51DA37;
		//l_PatchSize = 4 + 1;
		//VirtualProtect((void*)l_PatchAddress, l_PatchSize, PAGE_EXECUTE_READWRITE, &l_OldFlags);
		//*reinterpret_cast<BYTE*>(l_PatchAddress) = 0xE8;
		//*reinterpret_cast<DWORD*>(l_PatchAddress + 1) = (DWORD)CustomSetAgingItemAreaCheck - (DWORD)l_PatchAddress - l_PatchSize;
		//VirtualProtect((void*)l_PatchAddress, l_PatchSize, l_OldFlags, NULL);

		//invalidate the code for swift axe
		//so that we can control this elsewhere in our code
		//PatchSingleByte ( (PVOID)0x0043E808, 0xE9 );
		//PatchSingleByte ( (PVOID)0x0043E809, 0xD2 );
		//PatchSingleByte ( (PVOID)0x0043E80A, 0x00 );

		//////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////

		l_PatchAddress = 0x469122;
		l_PatchSize = 4 + 1;
		VirtualProtect ( (void *)l_PatchAddress, l_PatchSize, PAGE_EXECUTE_READWRITE, &l_OldFlags );
		*reinterpret_cast<BYTE *>( l_PatchAddress ) = 0xE8;
		*reinterpret_cast<DWORD *>( l_PatchAddress + 1 ) = (DWORD)CustomSetForceOrb - (DWORD)l_PatchAddress - l_PatchSize;
		VirtualProtect ( (void *)l_PatchAddress, l_PatchSize, l_OldFlags, NULL );

		//////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////


		l_PatchAddress = 0x62DAEB;
		l_PatchSize = 4 + 1;
		VirtualProtect ( (void *)l_PatchAddress, l_PatchSize, PAGE_EXECUTE_READWRITE, &l_OldFlags );
		*reinterpret_cast<BYTE *>( l_PatchAddress ) = 0xE8;
		*reinterpret_cast<DWORD *>( l_PatchAddress + 1 ) = (DWORD)CustomRestoreRecordData - (DWORD)l_PatchAddress - l_PatchSize;
		VirtualProtect ( (void *)l_PatchAddress, l_PatchSize, l_OldFlags, NULL );

		//////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////


		l_PatchAddress = 0x43DE59;
		l_PatchSize = 4 + 1;
		VirtualProtect ( (void *)l_PatchAddress, l_PatchSize, PAGE_EXECUTE_READWRITE, &l_OldFlags );
		*reinterpret_cast<BYTE *>( l_PatchAddress ) = 0xE8;
		*reinterpret_cast<DWORD *>( l_PatchAddress + 1 ) = (DWORD)CustomContGameExp - (DWORD)l_PatchAddress - l_PatchSize;
		VirtualProtect ( (void *)l_PatchAddress, l_PatchSize, l_OldFlags, NULL );

		//////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////

		//l_PatchAddress = 0x416030;
		//l_PatchSize = 4 + 1;
		//VirtualProtect((void *)l_PatchAddress, l_PatchSize, PAGE_EXECUTE_READWRITE, &l_OldFlags);
		//*reinterpret_cast<BYTE *>(l_PatchAddress) = 0xE8;
		//*reinterpret_cast<DWORD *>(l_PatchAddress + 1) = (DWORD)OpenPlaySkill_Priestress - (DWORD)l_PatchAddress - l_PatchSize;
		//VirtualProtect((void *)l_PatchAddress, l_PatchSize, l_OldFlags, NULL);

		//////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////

		//l_PatchAddress = 0x4161C6;
		//l_PatchSize = 4 + 1;
		//VirtualProtect((void *)l_PatchAddress, l_PatchSize, PAGE_EXECUTE_READWRITE, &l_OldFlags);
		//*reinterpret_cast<BYTE *>(l_PatchAddress) = 0xE8;
		//*reinterpret_cast<DWORD *>(l_PatchAddress + 1) = (DWORD)CheckPlaySkill_Priestress - (DWORD)l_PatchAddress - l_PatchSize;
		//VirtualProtect((void *)l_PatchAddress, l_PatchSize, l_OldFlags, NULL);

		//////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////


		//Fix incorrect duration for:
		// - SCROLL_EVASION (Athanasia)
		// - SCROLL_CRITICAL (Deadly Edge)
		// - SCROLL_INVULNERABILITY (Avail Of Evade)
		//PatchSingleByte ( (PVOID)( 0x004DE5DB + 1 ), 60 );
		//PatchSingleByte ( (PVOID)( 0x004DE5EA + 1 ), 60 );
		//PatchSingleByte ( (PVOID)( 0x004DE5F9 + 1 ), 60 );

		//For enchant weapon fix. Replace 0x2B with 0xF0
		//The reason for this is because 0x2B is not unique where
		// each class has 0x2B for skill tiny id. The 'hack' is to
		// give it a unique id of 0xF0 so that it will be recognized by the server
		// as weapon enchantment. See RecvSkillAreaTarget on server side

		BYTE b = READBYTE ( 0x456D55 );

		PatchSingleByte ( (PVOID)0x456D55, 100 );


		//////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////


		PatchSingleByte((PVOID)(0x62A31E + 6), MAPLEVEL(MAPID_BlessCastle));
		PatchSingleByte((PVOID)0x955CD0, MAPLEVEL(MAPID_BlessCastle));

		PatchSingleByte((PVOID)0x825334, MAPLEVEL(MAPID_LandOfChaos)); //Wrap gate


		//Bypass code to disallow dropping of preimum items for specific items
		//we will use bDisallowDrop flag instead (controlled via db)
		PatchInt((PVOID)0x4605E8, 0);
		PatchInt((PVOID)0x4BC67A, 0);
		PatchInt((PVOID)0x4605FD, 0);
		PatchInt((PVOID)0x4BC69E, 0);

		//Update levels in teleport core
		//See int TelePort_FieldNum[TELEPORT_FIELDMAX][3] in HaPremimumItem.cpp


		//Teleport core page 0 / haTeleport_Field[1].sin
		int iPage1Address = 0x04B0CF70;
		PatchTeleportCore(iPage1Address, 0, MAPID_RicartenTown);
		PatchTeleportCore(iPage1Address, 1, MAPID_PillaiTown);
		PatchTeleportCore(iPage1Address, 2, MAPID_RoadToTheWind);
		PatchTeleportCore(iPage1Address, 3, MAPID_ValleyOfTranquility);
		PatchTeleportCore(iPage1Address, 4, MAPID_LandOfDusk);
		PatchTeleportCore(iPage1Address, 5, MAPID_ForestOfSpirits);
		PatchTeleportCore(iPage1Address, 6, MAPID_AcasiaForest);
		PatchTeleportCore(iPage1Address, 7, MAPID_BambooForest);
		PatchTeleportCore(iPage1Address, 8, MAPID_GardenOfFreedom);
		PatchTeleportCore(iPage1Address, 9, MAPID_RefugeOfTheAncients);


		//Teleport core page 2 / haTeleport_Field[2].sin
		int iPage2Address = 0x04B0CFE8;
		PatchTeleportCore(iPage2Address, 0, MAPID_CastleOfTheLost);
		PatchTeleportCore(iPage2Address, 1, MAPID_RuinenVillage);
		PatchTeleportCore(iPage2Address, 2, MAPID_CursedLand);
		PatchTeleportCore(iPage2Address, 3, MAPID_ForgottenLand);
		PatchTeleportCore(iPage2Address, 4, MAPID_NaviskoTown);
		PatchTeleportCore(iPage2Address, 5, MAPID_Oasis);
		PatchTeleportCore(iPage2Address, 6, MAPID_AncientPrisonF1);
		PatchTeleportCore(iPage2Address, 7, MAPID_AncientPrisonF2);
		PatchTeleportCore(iPage2Address, 8, MAPID_MushroomCave);
		PatchTeleportCore(iPage2Address, 9, MAPID_AncientsBattlefield);



		//Teleport core page 3 / haTeleport_Field[3].sin
		int iPage3Address = 0x04B0D060;
		PatchTeleportCore(iPage3Address, 0, MAPID_ForbiddenLand);
		PatchTeleportCore(iPage3Address, 1, MAPID_BeehiveCave);
		PatchTeleportCore(iPage3Address, 2, MAPID_AncientPrisonF3);
		PatchTeleportCore(iPage3Address, 3, MAPID_DarkSanctuary);
		PatchTeleportCore(iPage3Address, 4, MAPID_GreedyLake);
		PatchTeleportCore(iPage3Address, 5, MAPID_CursedTempleF1);
		PatchTeleportCore(iPage3Address, 6, MAPID_CursedTempleF2);
		PatchTeleportCore(iPage3Address, 7, MAPID_RailwayOfChaos);
		PatchTeleportCore(iPage3Address, 8, MAPID_HeartOfPerum);
		PatchTeleportCore(iPage3Address, 9, MAPID_Eura);

		//Teleport core page 4 / haTeleport_Field[4].sin
		int iPage4Address = 0x04B0D0D8;
		PatchTeleportCore(iPage4Address, 0, MAPID_GallubiaValley);
		PatchTeleportCore(iPage4Address, 1, MAPID_FrozenSanctuary);
		PatchTeleportCore(iPage4Address, 2, MAPID_KelvezuLair);
		PatchTeleportCore(iPage4Address, 3, MAPID_LandOfChaos);
		PatchTeleportCore(iPage4Address, 4, MAPID_LostTemple);
		PatchTeleportCore(iPage4Address, 5, MAPID_EndlessTowerF1);
		PatchTeleportCore(iPage4Address, 6, MAPID_EndlessTowerF2);
		PatchTeleportCore(iPage4Address, 7, MAPID_EndlessTowerF3);
		PatchTeleportCore(iPage4Address, 8, MAPID_IceMineF1);
		PatchTeleportCore(iPage4Address, 9, MAPID_SecretLaboratory);

		//Teleport core page 5 / haTeleport_Field[5].sin
		int iPage5Address = 0x04B0D150;
		PatchTeleportCore(iPage5Address, 0, MAPID_AncientWeapon);
		PatchTeleportCore(iPage5Address, 1, MAPID_SeaOfAbyss1);
		PatchTeleportCore(iPage5Address, 2, MAPID_HearOfFire);
		PatchTeleportCore(iPage5Address, 3, MAPID_SeaOfAbyss2);
		PatchTeleportCore(iPage5Address, 4, MAPID_SecretLab2);
		PatchTeleportCore(iPage5Address, 5, MAPID_Atlantis);
		PatchTeleportCore(iPage5Address, 6, MAPID_BattleTown);
		PatchTeleportCore(iPage5Address, 7, MAPID_CursedTempleF3);
		PatchTeleportCore(iPage5Address, 8, MAPID_AncientDungeonF1);
		PatchTeleportCore(iPage5Address, 9, MAPID_AncientDungeonF2);

		//Teleport core page 6 / haTeleport_Field[6].sin
		int iPage6Address = 0x04B0D1C8;
		PatchTeleportCore(iPage6Address, 0, MAPID_AncientDungeonF3);
		PatchTeleportCore(iPage6Address, 1, MAPID_MysteryForest1);
		PatchTeleportCore(iPage6Address, 2, MAPID_MysteryForest2);
		PatchTeleportCore(iPage6Address, 3, MAPID_MysteryForest3);
		PatchTeleportCore(iPage6Address, 4, MAPID_MysteryDesert1);
		PatchTeleportCore(iPage6Address, 5, MAPID_MysteryDesert2);
		PatchTeleportCore(iPage6Address, 6, MAPID_MysteryDesert3);
		PatchTeleportCore(iPage6Address, 7, MAPID_ForgottenTempleF1);
		PatchTeleportCore(iPage6Address, 8, MAPID_ForgottenTempleF2);
		PatchTeleportCore(iPage6Address, 9, MAPID_WindForest1);

		//Patch Clever Newter (FieldLimitLevel_Table[])
		PatchInt( (PVOID)0x00825310, MAPLEVEL( MAPID_HeartOfPerum ) );
		PatchInt( (PVOID)0x00825314, MAPLEVEL( MAPID_Eura ) );

		//2nd location for Iron 2 for Clever Newter
		PatchInt( (PVOID)0x00450175, MAPLEVEL( MAPID_HeartOfPerum ) );

		//42
		//int iaFieldLimitLevel_Table[] = {
		//	0,0,0,0,					//Forest Ricarten
		//	0,0,0,0,					//Wasteland
		//	0,0,0,						//Desert Navisco
		//	55,55,40,50,60,				//Desert 3,4, Dungeon 1,2,3
		//	1000,						//Alkagibang (core prohibition-1000)
		//	0,0,0,0,0,					//Fall Forest, Pillay
		//	70,75,						//temples 1 and 2
		//	55,55,65,					//Mushroom Cave Beehive Cave Temple
		//	80,85,90,0,90,0,			//Iron 1, 2, Yura, Bellatra Ice (90)
		//	0,70,90,90,					//Lake Gredy in Thameskron Castle (90) Ice 2 Field
		//	95,100,1000,100,100		    //Lost Island (95) Lost Temple (100), Twisted Forest, Endless Tower 1st floor, Endless Tower 2nd floor, };
		//}

		//FieldLimitLevel_Table

		//CopyMemory( (int *)0x008252A0, &iaFieldLimitLevel_Table, sizeof( int ) * 16 );




		int iaMasteryIncreaIndexNew[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, };




		//skill mastery increase number
		//original values: int sinMasteryIncreaIndex[16] = {5,5,5,5,7,7,7,7,9,9,9,9,14,15,16,17};
		CopyMemory( (int *)0x009BBEF8, &iaMasteryIncreaIndexNew, sizeof( int ) * 16 );


		//skill prices
		int* piaPrice = ((int*)0x009BBEA8);
		/*
		piaPrice = base skill cost
		500
		600
		700
		800
		1000
		1200
		1400
		1600
		2000
		2500
		3000
		4000
		5000
		6000
		7000
		8000
		10000
		12000
		14000
		16000
		*/


		int* piaPriceP = ((int*)0x009BBE58);
		/*
		piaPriceP (skill index) * skill level

			1000
			1500
			2000
			3000
			5000
			7000
			9000
			10000
			12000
			15000
			20000
			25000
			30000
			35000
			40000
			50000
			60000
			70000
			80000*/


		float discount = 0.4f; //reduce skill price for now while skills are broken

#ifdef SKILLS_COST_DISCOUNT
		discount = 0.2f;
#endif

		for (int i = 0; i < 20; i++)
		{
			piaPrice[i] = static_cast<int>(piaPrice[i] * discount);
			piaPriceP[i] = static_cast<int>(piaPriceP[i] * discount);
		}

		int* piaPrice1 = ((int*)0x009BBEA8);
		int* piaPricePa = ((int*)0x009BBE58);


		//////////////////////////////////////////////////////////////////////////
	    //////////////////////////////////////////////////////////////////////////
		// FORCES

		//THE COMMENT VALUES ARE ORIGINAL VALUES IN Game.exe
		//IDEALLY THEY SHOULD MATCH DATABASE VALUES

		//fo101 - Lucidy Force
		//0x0082D738 01 00 00 00                             dd 1
		//0x0082D73C 28 00 00 00                             dd 40
		PatchInt((PVOID)0x0082D738, 1);
		PatchInt((PVOID)0x0082D73C, 200);

		//fo102 - Sereneo Force
		//0x0082D740 01 00 00 00                             dd 1
		//0x0082D744 28 00 00 00                             dd 40
		PatchInt((PVOID)0x0082D740, 1);
		PatchInt((PVOID)0x0082D744, 200);

		//fo103 - Fadeo Force
		//0x0082D748 01 00 00 00                             dd 1
		//0x0082D74C 28 00 00 00                             dd 40
		PatchInt((PVOID)0x0082D748, 1);
		PatchInt((PVOID)0x0082D74C, 200);

		//fo104 - Sparky Force
		//0x0082D750 01 00 00 00                             dd 1
		//0x0082D754 28 00 00 00                             dd 40
		PatchInt((PVOID)0x0082D750, 1);
		PatchInt((PVOID)0x0082D754, 200);

		//fo105 - Radiant Force
		//0x0082D758 23 00 00 00                             dd 35
		//0x0082D75C C8 00 00 00                             dd 200
		PatchInt((PVOID)0x0082D758, 10);
		PatchInt((PVOID)0x0082D75C, 200);

		//fo106 - Transparo Force
		//0x0082D760 28 00 00 00                             dd 40
		//0x0082D764 C8 00 00 00                             dd 200
		PatchInt((PVOID)0x0082D760, 20);
		PatchInt((PVOID)0x0082D764, 200);

		//fo107 - Murky Force
		//0x0082D768 2D 00 00 00                             dd 45
		//0x0082D76C C8 00 00 00                             dd 200
		PatchInt((PVOID)0x0082D768, 30);
		PatchInt((PVOID)0x0082D76C, 200);

		//fo108 - Devine Force
		//0x0082D770 32 00 00 00                             dd 50
		//0x0082D774 C8 00 00 00                             dd 200
		PatchInt((PVOID)0x0082D770, 40);
		PatchInt((PVOID)0x0082D774, 200);

		//fo109 - Celesto Force
		//0x0082D778 37 00 00 00                             dd 55
		//0x0082D77C C8 00 00 00                             dd 200
		PatchInt((PVOID)0x0082D778, 50);
		PatchInt((PVOID)0x0082D77C, 200);

		//fo110 - Mirage Force
		//0x0082D780 3C 00 00 00                             dd 60
		//0x0082D784 C8 00 00 00                             dd 200
		PatchInt((PVOID)0x0082D780, 60);
		PatchInt((PVOID)0x0082D784, 200);

		//fo111 - Inferna Force
		//0x0082D788 46 00 00 00                             dd 70
		//0x0082D78C C8 00 00 00                             dd 200
		PatchInt((PVOID)0x0082D788, 70);
		PatchInt((PVOID)0x0082D78C, 200);

		//fo112 - Enigma Force
		//0x0082D790 5A 00 00 00                             dd 90
		//0x0082D794 C8 00 00 00                             dd 200
		PatchInt((PVOID)0x0082D790, 80);
		PatchInt((PVOID)0x0082D794, 200);

		//fo113 - Bellum Force
		//0x0082D798 5A 00 00 00                             dd 90
		//0x0082D79C C8 00 00 00                             dd 200
		PatchInt((PVOID)0x0082D798, 90);
		PatchInt((PVOID)0x0082D79C, 200);

		//fo114 - Oredo Force
		//0x0082D7A0 64 00 00 00                             dd 100
		//0x0082D7A4 C8 00 00 00                             dd 200
		PatchInt((PVOID)0x0082D7A0, 100);
		PatchInt((PVOID)0x0082D7A4, 200);

        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////

        /// BellatraCastle Enter
        l_PatchAddress = 0x4D3895;
        l_PatchSize = 5;
        VirtualProtect((void *)l_PatchAddress, l_PatchSize, PAGE_EXECUTE_READWRITE, &l_OldFlags);
        *reinterpret_cast<DWORD *>(l_PatchAddress + 1) = (DWORD)CustomEnterBellatraCastle - (DWORD)l_PatchAddress - l_PatchSize;
        VirtualProtect((void *)l_PatchAddress, l_PatchSize, l_OldFlags, NULL);

        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////

		/// Stupid warehouse system
		l_PatchAddress = 0x622225;
		l_PatchSize = 4 + 1;
		VirtualProtect((void*)l_PatchAddress, l_PatchSize, PAGE_EXECUTE_READWRITE, &l_OldFlags);
		*reinterpret_cast<BYTE*>(l_PatchAddress) = 0xE9;
		*reinterpret_cast<DWORD*>(l_PatchAddress + 1) = 0x00000090;
		VirtualProtect((void*)l_PatchAddress, l_PatchSize, l_OldFlags, NULL);


		/// Skip buggy gold check on client and instead do the checks on server
		/// the gold check results in 'Too many items in warehouse'
		/// cmp ebx ebx
		/// if (CompWareHouseMoney = CompWareHouseMoney)
		/// where ebx is CompWarehouseMoney
		/// so the conditional statement always will be true
		//Address	Length	Original bytes	Patched bytes
		//006221CD	0x2		3B DA 			39 DB
		PatchSingleByte((PVOID)(0x6221CD), 0x39);
		PatchSingleByte((PVOID)(0x6221CE), 0xDB);

		//////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////


		//Swift Axe - Increase speed cap from 423 to 700 at two places.
		//uncomment out once seasonal is finished
		//PatchSingleByte ( (PVOID)( 0x43E882 ), 0xBC );
		//PatchSingleByte ( (PVOID)( 0x43E883 ), 0x02 );
		//PatchSingleByte ( (PVOID)( 0x43E898 ), 0xBC );
		//PatchSingleByte ( (PVOID)( 0x43E899 ), 0x02 );


		//////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////


		PatchThrowItemToFieldFunctions();

		//Patch Buy Item FN
		PatchWord( (PVOID)(0x004E7830), 0x25FF );
		PatchInt( (PVOID)(0x004E7832), 0x04AF8250 );
		PatchSingleByte( (PVOID)(0x004E7836), 0x90 );

		//Patch Add Force Orb Item Effect FN
		PatchWord( (PVOID)(0x005C16B0), 0x25FF );
		PatchInt( (PVOID)(0x005C16B2), 0x04AF8254 );
		PatchSingleByte( (PVOID)(0x005C16B6), 0x90 );

		//Crown Render
		PatchSingleByte( (PVOID)(0x0042132C), 0xE9 );
		PatchInt( (PVOID)(0x0042132D), 0x00000281 );
		PatchSingleByte( (PVOID)(0x00421331), 0x90 );

		//Remove Monster Freeze Effect
		//PatchInt( (PVOID)(0x0042CAEC), 0x0042C5F3 ); //disable for now. we will think of a better solution
		//PatchInt( (PVOID)(0x0042CAF0), 0x0042C5F3 ); //one proposed solution is half the freezing effect (50%)

		//Devil Shy Exception
		PatchMemoryNOP( 0x0051E903, 6 );

		// Hall of Valhalla can't be used if Triumph of Valhalla is active
		// Disable GetAssaUseEffect(lpCurPlayer,SKILL_TRIUMPH_OF_VALHALLA)==FALSE
		// in SkillSub.cpp
        PatchMemoryNOP(0x0040DB85, 22);

		//allow rendering of swift axe effect on any weapons (originally only axe)
		PatchMemoryNOP( 0x00547B43, 0x14 );

		// Delete Log
		DeleteFileA( "save\\logs\\init.log" );

		(*(DWORD*)0x04AF8030) = (DWORD)&DefendChance;
		(*(DWORD*)0x04AF8034) = (DWORD)&BlockChance;
		(*(DWORD*)0x04AF8038) = (DWORD)&OnSendPlayDataEx;
		(*(DWORD*)0x04AF803C) = (DWORD)&ShowExpValueFormat;
		(*(DWORD*)0x04AF8040) = (DWORD)&OnReceivePacket;
		(*(DWORD*)0x04AF8044) = (DWORD)&DX::Graphic::BeginScene;
		(*(DWORD*)0x04AF8048) = (DWORD)&DX::Graphic::EndScene;
		(*(DWORD*)0x04AF804C) = (DWORD)&OnDrawUI;
		(*(DWORD*)0x04AF8050) = (DWORD)&GRAPHICENGINE->TakenScreenShot;
		(*(DWORD*)0x04AF8054) = (DWORD)&LoadMaps;
		(*(DWORD*)0x04AF8058) = (DWORD)&OnClickPremium;
		(*(DWORD*)0x04AF805C) = (DWORD)&DrawTextDXHook;
		(*(DWORD*)0x04AF8064) = (DWORD)&OnLoadArmorCharacter;
		(*(DWORD*)0x04AF8068) = (DWORD)&OnSetArmorCurrentUnit;
		(*(DWORD*)0x04AF806C) = (DWORD)&OnSetArmorTargetUnit;
		(*(DWORD*)0x04AF8070) = (DWORD)&OnLoginDraw;
		(*(DWORD*)0x04AF8074) = (DWORD)&OnLoseExp;
		(*(DWORD*)0x04AF8078) = (DWORD)&GetAbsorptionPlayer;
		(*(DWORD*)0x04AF807C) = (DWORD)&DrawGMDebugCharacterInfo;
		(*(DWORD*)0x04AF8080) = (DWORD)&OnReceiveCommand;
		(*(DWORD*)0x04AF8084) = (DWORD)&LoadMusic;
		(*(DWORD*)0x04AF8088) = (DWORD)&OnItemPivot;
		(*(DWORD*)0x04AF808C) = (DWORD)&ReOrderVolume;
		(*(DWORD*)0x04AF8090) = (DWORD)&LoadTexture;
		(*(DWORD*)0x04AF8094) = (DWORD)&OnHitReceived;
		(*(DWORD*)0x04AF80A4) = (DWORD)&GetWidthNormalFont;
		(*(DWORD*)0x04AF80A8) = (DWORD)&SetCharacterFormula;
		(*(DWORD*)0x04AF80AC) = (DWORD)&Begin3D;
		(*(DWORD*)0x04AF80B0) = (DWORD)&End3D;
		(*(DWORD*)0x04AF80B4) = (DWORD)&OnDescriptionItem;
		(*(DWORD*)0x04AF80B8) = (DWORD)&GRAPHICENGINE->ShowLoading;
		(*(DWORD*)0x04AF80BC) = (DWORD)&GRAPHICENGINE->HideLoading;
		(*(DWORD*)0x04AF80C0) = (DWORD)&GRAPHICENGINE->UpdateLoading;
		(*(DWORD*)0x04AF80C4) = (DWORD)&OnCharacterUpdateData;
		(*(DWORD*)0x04AF80C8) = (DWORD)&GameCore::SendLoginData;
		(*(DWORD*)0x04AF80CC) = (DWORD)&IsDebuffSkill;
		(*(DWORD*)0x04AF80D0) = (DWORD)&FormatSkillTimerInfo;
		(*(DWORD*)0x04AF80D4) = (DWORD)&GRAPHICENGINE->DrawLoadingImage;
		(*(DWORD*)0x04AF80D8) = (DWORD)&CharacterGame::ChecksumCheck;
		(*(DWORD*)0x04AF80DC) = (DWORD)&ANTICHEATHANDLER->CheckThread;
		(*(DWORD*)0x04AF80E0) = (DWORD)&GetEditTextW;
		(*(DWORD*)0x04AF80E4) = (DWORD)&GameCore::GameRun;
		(*(DWORD*)0x04AF80E8) = (DWORD)&RenderSubServerBox;
		(*(DWORD*)0x04AF80EC) = (DWORD)&SummonCrystal;
		(*(DWORD*)0x04AF80F0) = (DWORD)&OnNpcQuestClick;
		(*(DWORD*)0x04AF80F4) = (DWORD)&OnClickOKQuestMessageBox;
		(*(DWORD*)0x04AF80F8) = (DWORD)&OnClickQuestWindow;
		(*(DWORD*)0x04AF80FC) = (DWORD)&OnDescriptionItemMixType;
		(*(DWORD*)0x04AF8100) = (DWORD)&OnAddColorDescription;
		(*(DWORD*)0x04AF8104) = (DWORD)&OnSendPacket;
		(*(DWORD*)0x04AF8108) = (DWORD)&OnConnect;
		(*(DWORD*)0x04AF810C) = (DWORD)&OnSocketPacket;
		(*(DWORD*)0x04AF8110) = (DWORD)&OnSocketClose;
		(*(DWORD*)0x04AF8114) = (DWORD)&OnCharacterSetCharInfoEx;
		(*(DWORD*)0x04AF8118) = (DWORD)&OnWarpCastle;
		(*(DWORD*)0x04AF811C) = (DWORD)&ChangeMotion;
		(*(DWORD*)0x04AF8120) = (DWORD)&GameCore::OnHandlerFunctions;
		(*(DWORD*)0x04AF8124) = (DWORD)&Game::InternetOpenUrlClan;
		(*(DWORD*)0x04AF8128) = (DWORD)&ExitGame;
		(*(DWORD*)0x04AF812C) = (DWORD)&BeginSkill; // BeginSkill
		(*(DWORD*)0x04AF8130) = (DWORD)&OpenPlaySkill; // OpenPlaySkill
		(*(DWORD*)0x04AF8134) = (DWORD)&PlaySkillAttack; // PlaySkillAttack
		(*(DWORD*)0x04AF8138) = (DWORD)&EventSkill; // EventSkill
		(*(DWORD*)0x04AF813C) = (DWORD)&EndSkill; // EndSkill
		(*(DWORD*)0x04AF8140) = (DWORD)&PrepareSkillInfo;
		(*(DWORD*)0x04AF8144) = (DWORD)&PrepareItemInfo;
		(*(DWORD*)0x04AF8148) = (DWORD)&TMAnimationHook;
		(*(DWORD*)0x04AF814C) = (DWORD)&ReadInx;
		(*(DWORD*)0x04AF8150) = (DWORD)&IsTradeUse;
		(*(DWORD*)0x04AF8154) = (DWORD)&UI::ItemInfoBox::IsExpiredItemTime;
		(*(DWORD*)0x04AF8158) = (DWORD)&CharacterCanEquipItem;
		(*(DWORD*)0x04AF815C) = (DWORD)&ForceOrbDraw;
		(*(DWORD*)0x04AF8160) = (DWORD)&ForceOrbMain;
		(*(DWORD*)0x04AF8164) = (DWORD)&DrawClanImage;
		(*(DWORD*)0x04AF8168) = (DWORD)&MixItem;
		(*(DWORD*)0x04AF816C) = (DWORD)&IsUseMix;
		(*(DWORD*)0x04AF8170) = (DWORD)&SetItemToNPC;
		(*(DWORD*)0x04AF8174) = (DWORD)&OnLostDevice;
		(*(DWORD*)0x04AF8178) = (DWORD)&ChangeMusic;
		(*(DWORD*)0x04AF817C) = (DWORD)&OnClickItemSlotPut;
		(*(DWORD*)0x04AF8180) = (DWORD)&OnCheckCanPutStoneSlotSocket;
		(*(DWORD*)0x04AF8184) = (DWORD)&OnCheckCanPutItemSlotSocket;
		(*(DWORD*)0x04AF8188) = (DWORD)&OnUseManaPotion;
		(*(DWORD*)0x04AF818C) = (DWORD)&AnimateModelGate;
		(*(DWORD*)0x04AF8190) = (DWORD)&RenderGate;
		(*(DWORD*)0x04AF8194) = (DWORD)&ClipFaceRender;
		(*(DWORD*)0x04AF8198) = (DWORD)&RenderModelGate;
		(*(DWORD*)0x04AF819C) = (DWORD)&RenderMeshGate;
		(*(DWORD*)0x04AF81A0) = (DWORD)&AnimateMeshGate;
		(*(DWORD*)0x04AF81A4) = (DWORD)&OnRenderRightWeapon;
		(*(DWORD*)0x04AF81A8) = (DWORD)&OnRenderSecondWeapon;
		(*(DWORD*)0x04AF81AC) = (DWORD)&AddFaceRender;
		(*(DWORD*)0x04AF81B0) = (DWORD)&RenderBGLogin;
		(*(DWORD*)0x04AF81B4) = (DWORD)&IsWarehouseUse;
		(*(DWORD*)0x04AF81B8) = (DWORD)&IsPersonalShopUse;
		(*(DWORD*)0x04AF81BC) = (DWORD)&AnimateMeshTree;
		(*(DWORD*)0x04AF81C0) = (DWORD)&SetEventTitleBox;
		(*(DWORD*)0x04AF81C4) = (DWORD)&IgnoreMousePointer;
		(*(DWORD*)0x04AF81C8) = (DWORD)&FrameUnitUpdate;
		(*(DWORD*)0x04AF81CC) = (DWORD)&FinalRender;
		(*(DWORD*)0x04AF81D0) = (DWORD)&RenderCacheData;
		(*(DWORD*)0x04AF81D4) = (DWORD)&DrawEffects;
		(*(DWORD*)0x04AF81D8) = (DWORD)&OnDrawUILogin;
		(*(DWORD*)0x04AF81DC) = (DWORD)&RenderGlow3D;
		(*(DWORD*)0x04AF81E0) = (DWORD)&RenderGate1;
		(*(DWORD*)0x04AF81E4) = (DWORD)&RenderEXE;
		(*(DWORD*)0x04AF81E8) = (DWORD)&RenderStageMesh;
		(*(DWORD*)0x04AF81EC) = (DWORD)&AnimateModelGate2;
		(*(DWORD*)0x04AF81F0) = (DWORD)&UpdateBlockedPosition;
		(*(DWORD*)0x04AF81F4) = (DWORD)&SendAreaDamageUnit;
		(*(DWORD*)0x04AF81F8) = (DWORD)&SetPVPArea;
		(*(DWORD*)0x04AF81FC) = (DWORD)&OnBuySkill;
		(*(DWORD*)0x04AF8200) = (DWORD)&GetCharacterClassDataGate;
		(*(DWORD*)0x04AF8204) = (DWORD)&HandleDebuffSkill;
		(*(DWORD*)0x04AF8208) = (DWORD)&OpenWorldMapGate;
		(*(DWORD*)0x04AF820C) = (DWORD)&GetSkillRange;
		(*(DWORD*)0x04AF8210) = (DWORD)&OnUseHPPotion;
		(*(DWORD*)0x04AF8214) = (DWORD)&SendSingleDamageUnit;
		(*(DWORD*)0x04AF8218) = (DWORD)&RenderUnitData;
		(*(DWORD*)0x04AF821C) = (DWORD)&IsUnitHighRange;
		(*(DWORD*)0x04AF8220) = (DWORD)&FreeUnitData;
		(*(DWORD*)0x04AF8224) = (DWORD)&SetInventoryItem;
		(*(DWORD*)0x04AF8228) = (DWORD)&ProcessSkillInParty;
		(*(DWORD*)0x04AF822C) = (DWORD)&ShowTradeRequest;
		(*(DWORD*)0x04AF8230) = (DWORD)&OpenGuideBoxLevelUP;
		(*(DWORD*)0x04AF8234) = (DWORD)&PrivateMessageSocialWindow;
		(*(DWORD*)0x04AF8238) = (DWORD)&CanGetItem;
		(*(DWORD*)0x04AF823C) = (DWORD)&CanCarryGold;
		(*(DWORD*)0x04AF8240) = (DWORD)&GetGoldPersonalShopLimit;
		(*(DWORD*)0x04AF8244) = (DWORD)&GetCanCarryGold;
		(*(DWORD*)0x04AF8248) = (DWORD)&IsSellItem;
		(*(DWORD*)0x04AF824C) = (DWORD)&LoadDefaultModels;
		(*(DWORD *)0x04AF8250) = (DWORD)&BuyItemShop;
		(*(DWORD *)0x04AF8254) = (DWORD)&AddForceOrbCharacterEffect;
		(*(DWORD *)0x04AF8398) = (DWORD)&ExpansionSkillRender;

		ItemCreatorInitHook();
	}

	return TRUE;
}

