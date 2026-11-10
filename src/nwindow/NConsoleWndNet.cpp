/*=============================================================================
    NConsoleWndNet.cpp - the console's requests to the server, and what they act on (the
    player, its target, what the network handler knows). Almost all of them only pass their
    arguments on to the engine's network handler (Engine->NetHandler, a UL2NetHandler whose 228
    virtuals are pure in Engine.dll and have no names yet: the slot is given at each call).
    Within a family the slots follow the protocol's packet order (e.g. 195..201 are the private
    store packets 0x73..0x79), which names the requests no chat command names.
=============================================================================*/
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NConsoleWndLogin.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCChatWnd.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NCLobbyWnd.h"
#include "nwindow/NCLoginWnd.h"
#include "nwindow/NCLoadingWnd.h"
#include "nwindow/NCMessageWnd.h"
#include "nwindow/NCRestartMenuWnd.h"
#include "nwindow/NCItemWnd.h"
#include "nwindow/NConsoleWndExt.h"
#if NW_CUSTOM
#include "nwindow/NCustomShortCuts.h"
#endif

// interface sounds (FL2NetNotify::OnInterfacePlaySound; names ours)
enum { SOUND_UseShortcut = 1, SOUND_PickUp = 3 };

// the pawn the player's controller drives, and the actor under the mouse
static FORCEINLINE APawn* PlayerPawn(UEngine* Engine)
{
    return Engine->Client->Viewports(0)->Actor->Pawn;
}
static FORCEINLINE AActor* HoveredActor(UEngine* Engine)
{
    return static_cast<UGameEngine*>(Engine)->MouseHitActor;
}

// (GetMyLocation, 0x10057dd0 in C2, is in NConsoleWndMisc.cpp)

// 0x10064ee0 - a chat line (NetHandler slot 146: type, text, whisper target); with
// GIsOnlyL2Console (no server) it is only shown in the chat window. Returns 0.
UBOOL NConsoleWnd::SendChatMsgToServer(INT Type, TCHAR* Text, TCHAR* Target)
{
    guard(NConsoleWnd::SendChatMsgToServer);
    UL2NetHandler* NetHandler = Engine->NetHandler;
    if (GIsOnlyL2Console)
        static_cast<NCConsole*>(Desktop)->ChatWnd->AddChatMessage(Text, 0xFFDCDCDC, 0, 0);
    else
    {
        L2ParamStack Params(10);
        Params.PushBack((void*)(INT_PTR)Type);
        Params.PushBack(Text);
        Params.PushBack(Target);
        NetHandler->Say2(Params);
    }
    return 0;
    unguard;
}
NW_HOOK_METHOD(0x10064ee0, NConsoleWnd, SendChatMsgToServer)

// 0x10064fd0 - NetHandler slot 68
UBOOL NConsoleWnd::RequestAuthLogin(const TCHAR* Account, const TCHAR* Password)
{
    guard(NConsoleWnd::RequestAuthLogin);
    return Engine->NetHandler->RequestAuthLogin((TCHAR*)Account, (TCHAR*)Password) != 0;
    unguard;
}
NW_HOOK_METHOD(0x10064fd0, NConsoleWnd, RequestAuthLogin)

// 0x1005aaa0  NOT PORTED (no C2 counterpart). Not a virtual in this client any more (slot 362 is C4Virtual362, NConsoleWndResize.cpp:
// include/ue2/gen/UL2ConsoleWnd.vt.inc); nothing calls it, so the standalone has no body for it
#if NW_HOOK_MODE
void NConsoleWnd::RequestLogout()
{
    NW_UNPORTED_NORET();
}
#endif

// 0x10065120 - NetHandler +0x140 (RequestGameStart); when sent, the loading screen replaces the character selection until the
// server answers (AcceptGameStart). With the GM's server change on (+0x3C94) its place (slot 442) goes along; the loading
// texture id is at +0x3C64, the message line is the god window's +0xF0 and the stages' dialog box its +0xFC
UBOOL NConsoleWnd::RequestGameStart(INT Index)
{
    guard(NConsoleWnd::RequestGameStart);
    INT A = 0, B = 0, C = 0;
    SWORD bGM = 0;
    if (bGMChange)
    {
        nw::VirtualCall<void>(this, 442, &A, &B, &C);
        bGM = 1;
    }
    if (!Engine->NetHandler->RequestGameStart(Index, bGM, A, B, C))
        return 0;
    LobbyWnd->HideWindow();
    nw::Field<NCWnd*>(GodWnd, 0xF0)->HideWindow();
    LoadingWnd->SetLoadingTexture(LoadingScreenID);
    LoadingWnd->ShowWindow();
    nw::Field<NCWnd*>(GodWnd, 0xFC)->HideWindow();
    SetState(L2CS_Waiting);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10065120, NConsoleWnd, RequestGameStart)

// 0x10065260 - NetHandler slot 72
UBOOL NConsoleWnd::RequestCharacterDelete(INT Index)
{
    guard(NConsoleWnd::RequestCharacterDelete);
    if (!Engine->NetHandler->RequestCharacterDelete(Index))
        return 0;
    SetState(L2CS_Waiting);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10065260, NConsoleWnd, RequestCharacterDelete)

// 0x10065310 - NetHandler slot 73
UBOOL NConsoleWnd::RequestCharacterRestore(INT Index)
{
    guard(NConsoleWnd::RequestCharacterRestore);
    if (!Engine->NetHandler->RequestCharacterRestore(Index))
        return 0;
    SetState(L2CS_Waiting);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10065310, NConsoleWnd, RequestCharacterRestore)

// 0x100653c0 - NetHandler slot 74
void NConsoleWnd::RequestNewCharacter()
{
    guard(NConsoleWnd::RequestNewCharacter);
    Engine->NetHandler->RequestNewCharacter();
    unguard;
}
NW_HOOK_METHOD(0x100653c0, NConsoleWnd, RequestNewCharacter)

// 0x10065440 - NetHandler slot 75 (the creation's choices, by value: C2's record is 0x60 bytes, ret 0x60)
UBOOL NConsoleWnd::RequestCharacterCreate(CharacterInfo Info)
{
    guard(NConsoleWnd::RequestCharacterCreate);
    if (!Engine->NetHandler->RequestCharacterCreate(Info))
        return 0;
    SetState(L2CS_Waiting);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10065440, NConsoleWnd, RequestCharacterCreate)

// 0x100656c0 - NetHandler slot 70
void NConsoleWnd::RequestServerLogin(L2ParamStack& Params)
{
    guard(NConsoleWnd::RequestServerLogin);
    Engine->NetHandler->RequestServerLogin(Params);
    unguard;
}
NW_HOOK_METHOD(0x100656c0, NConsoleWnd, RequestServerLogin)

// 0x100662a0 - NetHandler slot 77
void NConsoleWnd::RequestItemList()
{
    guard(NConsoleWnd::RequestItemList);
    Engine->NetHandler->RequestItemList();
    unguard;
}
NW_HOOK_METHOD(0x100662a0, NConsoleWnd, RequestItemList)

// 0x10066320 - NetHandler +0x170 (RequestUseItem(L2ParamStack&)): the object id and whether Ctrl is held (the
// virtual 297 of this client); what it returns is returned
INT NConsoleWnd::RequestUseItem(INT ObjectID)
{
    L2ParamStack Params(10);
    Params.PushBack((void*)(INT_PTR)ObjectID);
    Params.PushBack((void*)(INT_PTR)nw::VirtualCall<UBOOL>(this, 319));
    return Engine->NetHandler->RequestUseItem(Params);
}
NW_HOOK_METHOD(0x10066320, NConsoleWnd, RequestUseItem)

// 0x100663e0 - NetHandler slot 153
void NConsoleWnd::RequestUnEquipItem(L2ParamStack& Params)
{
    Engine->NetHandler->RequestUnEquipItem(Params);
}
NW_HOOK_METHOD(0x100663e0, NConsoleWnd, RequestUnEquipItem)

// 0x10066780 - NetHandler slot 82
void NConsoleWnd::RequestTrade(INT CreatureID)
{
    guard(NConsoleWnd::RequestTrade);
    Engine->NetHandler->RequestTrade(CreatureID);
    unguard;
}
NW_HOOK_METHOD(0x10066780, NConsoleWnd, RequestTrade)

// 0x10066800 - NetHandler slot 83 (by value)
void NConsoleWnd::RequestAddTradeItem(NItemRequest Request)
{
    guard(NConsoleWnd::RequestAddTradeItem);
    Engine->NetHandler->RequestAddTradeItem(Request);
    unguard;
}
NW_HOOK_METHOD(0x10066800, NConsoleWnd, RequestAddTradeItem)

// 0x10066890 - NetHandler slot 84
void NConsoleWnd::RequestTradeDone(INT bConfirm)
{
    guard(NConsoleWnd::RequestTradeDone);
    Engine->NetHandler->RequestTradeDone(bConfirm);
    unguard;
}
NW_HOOK_METHOD(0x10066890, NConsoleWnd, RequestTradeDone)

// 0x100669c0 - NetHandler slot 90 (C2 only: the manor's seed purchase; the fmap paired the C1 RequestBuyItem with it)
void NConsoleWnd::RequestBuySeed(L2ParamStack& Params)
{
    guard(NConsoleWnd::RequestBuySeed);
    Engine->NetHandler->RequestBuySeed(Params);
    unguard;
}
NW_HOOK_METHOD(0x100669c0, NConsoleWnd, RequestBuySeed)

// 0x10066a40 - NetHandler slot 91 (C2 only: the manor's crop procurement)
void NConsoleWnd::RequestProcureCrop(L2ParamStack& Params)
{
    guard(NConsoleWnd::RequestProcureCrop);
    Engine->NetHandler->RequestProcureCrop(Params);
    unguard;
}
NW_HOOK_METHOD(0x10066a40, NConsoleWnd, RequestProcureCrop)

// 0x10066c10 - NetHandler slot 89
void NConsoleWnd::RequestBuyItem(L2ParamStack& Params)
{
    guard(NConsoleWnd::RequestBuyItem);
    Engine->NetHandler->RequestBuyItem(Params);
    unguard;
}
NW_HOOK_METHOD(0x10066c10, NConsoleWnd, RequestBuyItem)

// 0x10066c90 - NetHandler slot 88
void NConsoleWnd::RequestSellItem(L2ParamStack& Params)
{
    guard(NConsoleWnd::RequestSellItem);
    Engine->NetHandler->RequestSellItem(Params);
    unguard;
}
NW_HOOK_METHOD(0x10066c90, NConsoleWnd, RequestSellItem)

// 0x10066fd0 - NetHandler slot 138: the warehouse window's list in its deposit mode
void NConsoleWnd::RequestWarehouseDeposit(L2ParamStack& Params)
{
    Engine->NetHandler->SendWareHouseDepositList(Params);
}
NW_HOOK_METHOD(0x10066fd0, NConsoleWnd, RequestWarehouseDeposit)

// 0x10066fe0 - NetHandler slot 139: the same in its withdraw mode
void NConsoleWnd::RequestWarehouseWithdraw(L2ParamStack& Params)
{
    Engine->NetHandler->SendWareHouseWithdrawList(Params);
}
NW_HOOK_METHOD(0x10066fe0, NConsoleWnd, RequestWarehouseWithdraw)

// 0x100680b0 - NetHandler slot 200 (C2 only; name ours, after the slot's)
void NConsoleWnd::RequestRecipeItemDelete(INT P1)
{
    Engine->NetHandler->RequestRecipeItemDelete(P1);
}
NW_HOOK_METHOD(0x100680b0, NConsoleWnd, RequestRecipeItemDelete)

// 0x100680c0 - NetHandler slot 201 (C2 only; name ours, after the slot's)
void NConsoleWnd::RequestRecipeItemMakeInfo(INT P1)
{
    Engine->NetHandler->RequestRecipeItemMakeInfo(P1);
}
NW_HOOK_METHOD(0x100680c0, NConsoleWnd, RequestRecipeItemMakeInfo)

// 0x100680d0 - NetHandler slot 202 (C2 only; name ours, after the slot's)
void NConsoleWnd::RequestRecipeItemMakeSelf(INT P1)
{
    Engine->NetHandler->RequestRecipeItemMakeSelf(P1);
}
NW_HOOK_METHOD(0x100680d0, NConsoleWnd, RequestRecipeItemMakeSelf)

// 0x10068190 - NetHandler slot 211 (C2 only; name ours, after the slot's)
void NConsoleWnd::RequestHennaItemInfo(INT P1)
{
    Engine->NetHandler->RequestHennaItemInfo(P1);
}
NW_HOOK_METHOD(0x10068190, NConsoleWnd, RequestHennaItemInfo)

// 0x100681a0 - NetHandler slot 212 (C2 only; name ours, after the slot's)
void NConsoleWnd::RequestHennaEquip(INT P1)
{
    Engine->NetHandler->RequestHennaEquip(P1);
}
NW_HOOK_METHOD(0x100681a0, NConsoleWnd, RequestHennaEquip)

// 0x100681b0 - NetHandler slot 213 (C2 only; name ours, after the slot's)
void NConsoleWnd::RequestHennaUnequipInfo(INT P1)
{
    Engine->NetHandler->RequestHennaUnequipInfo(P1);
}
NW_HOOK_METHOD(0x100681b0, NConsoleWnd, RequestHennaUnequipInfo)

// 0x100681c0 - NetHandler slot 214 (C2 only; name ours, after the slot's)
void NConsoleWnd::RequestHennaUnequip(INT P1)
{
    Engine->NetHandler->RequestHennaUnequip(P1);
}
NW_HOOK_METHOD(0x100681c0, NConsoleWnd, RequestHennaUnequip)

// 0x1007a1e0 - NetHandler slot 142, then the pickup sound (the icon was dropped on the bar)
void NConsoleWnd::RequestShortCutReg(L2ParamStack& Params)
{
#if NW_CUSTOM
    if (NCustomShortCuts::IsLocked())   // custom: locked bars: nothing dropped on a bar is registered
        return;
#endif
    Engine->NetHandler->RequestShortCutReg(Params);
    PlayInterfaceSound(SOUND_PickUp);
}
NW_HOOK_METHOD(0x1007a1e0, NConsoleWnd, RequestShortCutReg)

// C1 only (the request of a used shortcut): the C3 image has UseShortCut(Slot, ...) (0x10076020) that sends it itself; an
// inert body
void NConsoleWnd::UseShortcut(L2ParamStack& Params)
{
}

// 0x10068280 - NetHandler slot 142
void NConsoleWnd::RequestShortCutDel(L2ParamStack& Params)
{
    Engine->NetHandler->RequestShortCutDel(Params);
}
NW_HOOK_METHOD(0x10068280, NConsoleWnd, RequestShortCutDel)

// 0x10068390 - NetHandler slot 143
void NConsoleWnd::RequestTargetCancel(L2ParamStack& Params)
{
    ::nw::TraceCall("NConsoleWnd::RequestTargetCancel", this, (int)&Params);
    Engine->NetHandler->RequestTargetCancel(Params);
}
NW_HOOK_METHOD(0x10068390, NConsoleWnd, RequestTargetCancel)

// 0x100683a0 - NetHandler slot 157
void NConsoleWnd::RequestActionUse(L2ParamStack& Params)
{
    ::nw::TraceCall("NConsoleWnd::RequestActionUse", this, (int)&Params);
    Engine->NetHandler->RequestActionUse(Params);
}
NW_HOOK_METHOD(0x100683a0, NConsoleWnd, RequestActionUse)

// 0x100683b0 - NetHandler slot 52 (target, origin, stand still): 0
void NConsoleWnd::RequestAttackForce(INT ObjectID, FVector Origin)
{
    Engine->NetHandler->Attack(ObjectID, Origin, 0);
}
NW_HOOK_METHOD(0x100683b0, NConsoleWnd, RequestAttackForce)

// RequestAttackStand: a C2 function (0x10054710 there) with no counterpart in this client; the map paired it with
// RequestAttack's address, whose arguments are not these (24 bytes popped): never called, never hooked. Standalone: no body
// (nothing calls it; a caller would be a link error instead of a wrong request)
#if NW_HOOK_MODE
NW_NAKED void NConsoleWnd::RequestAttackStand(INT ObjectID, FVector Origin) NW_STOCK(0x10071C00)
#endif

// 0x10071c00 - attack with Ctrl held (NetHandler +0xE8), else select (+0xEC). Two more words than C1: the callers push
// them after the position; either is replaced by the key's state when Ctrl / Shift is down. Nothing happens while
// the engine's +0x188 flag is set (RequestTargetNext sets it); Ctrl is the virtual 319 of this client. C4: the minimap's
// first landmark is deleted after the request
void NConsoleWnd::RequestAttack(INT ObjectID, FVector Origin, INT bForceAttack, INT bShift)
{
    if (!nw::Field<INT>(Engine, 0x188))
    {
        if (nw::VirtualCall<UBOOL>(this, 319))
            bForceAttack = nw::VirtualCall<UBOOL>(this, 319);
        if (Engine->Client->Viewports(0)->Input->KeyDown(IK_Shift))
            bShift = Engine->Client->Viewports(0)->Input->KeyDown(IK_Shift);
        if (bForceAttack)
            Engine->NetHandler->Attack(ObjectID, Origin, bShift);
        else
            Engine->NetHandler->Action(ObjectID, Origin, bShift & 0xFF);
        if (Engine && static_cast<UGameEngine*>(Engine)->LandMark)
            static_cast<UGameEngine*>(Engine)->LandMark->DeleteLandMark(0);
    }
}
NW_HOOK_METHOD(0x10071c00, NConsoleWnd, RequestAttack)

// 0x100683f0 - NetHandler slot 57
void NConsoleWnd::RequestSocialAction(INT Action)
{
    Engine->NetHandler->SocialAction(Action);
}
NW_HOOK_METHOD(0x100683f0, NConsoleWnd, RequestSocialAction)

// 0x10068400 - NetHandler slot 59
void NConsoleWnd::RequestChangeWaitType(UBOOL bStand)
{
    ::nw::TraceCall("NConsoleWnd::RequestChangeWaitType", this, bStand);
    Engine->NetHandler->ChangeWaitType(bStand);
}
NW_HOOK_METHOD(0x10068400, NConsoleWnd, RequestChangeWaitType)

// 0x10068410 - NetHandler slot 58
void NConsoleWnd::RequestChangeMoveType(UBOOL bRun)
{
    Engine->NetHandler->ChangeMoveType(bRun);
}
NW_HOOK_METHOD(0x10068410, NConsoleWnd, RequestChangeMoveType)

// 0x10068790 - NetHandler slot 119
void NConsoleWnd::RequestAllyInvite(L2ParamStack& Params)
{
    Engine->NetHandler->RequestJoinAlly(Params);
}
NW_HOOK_METHOD(0x10068790, NConsoleWnd, RequestAllyInvite)

// 0x100687a0 - NetHandler slot 120
void NConsoleWnd::RequestAnswerJoinAlly(L2ParamStack& Params)
{
    Engine->NetHandler->RequestAnswerJoinAlly(Params);
}
NW_HOOK_METHOD(0x100687a0, NConsoleWnd, RequestAnswerJoinAlly)

// 0x100687b0 - NetHandler slot 121
void NConsoleWnd::RequestOustAlly(L2ParamStack& Params)
{
    Engine->NetHandler->RequestOustAlly(Params);
}
NW_HOOK_METHOD(0x100687b0, NConsoleWnd, RequestOustAlly)

// 0x100687c0 - NetHandler slot 124
void NConsoleWnd::RequestAllyInfo()
{
    Engine->NetHandler->RequestAllyInfo();
}
NW_HOOK_METHOD(0x100687c0, NConsoleWnd, RequestAllyInfo)

// 0x100687d0 - NetHandler slot 122
void NConsoleWnd::RequestWithdrawAlly()
{
    Engine->NetHandler->RequestWithdrawAlly();
}
NW_HOOK_METHOD(0x100687d0, NConsoleWnd, RequestWithdrawAlly)

// 0x100687e0 - NetHandler slot 123
void NConsoleWnd::RequestDismissAlly()
{
    Engine->NetHandler->RequestDismissAlly();
}
NW_HOOK_METHOD(0x100687e0, NConsoleWnd, RequestDismissAlly)

// 0x10068990 - NetHandler slot 125
void NConsoleWnd::RequestFriendInvite(L2ParamStack& Params)
{
    Engine->NetHandler->RequestFriendInvite(Params);
}
NW_HOOK_METHOD(0x10068990, NConsoleWnd, RequestFriendInvite)

// 0x100689a0 - NetHandler slot 126
void NConsoleWnd::RequestFriendDel(L2ParamStack& Params)
{
    Engine->NetHandler->RequestFriendDel(Params);
}
NW_HOOK_METHOD(0x100689a0, NConsoleWnd, RequestFriendDel)

// 0x100689b0 - NetHandler slot 128
void NConsoleWnd::RequestAnswerFriendInvite(L2ParamStack& Params)
{
    Engine->NetHandler->RequestFriendAddReply(Params);
}
NW_HOOK_METHOD(0x100689b0, NConsoleWnd, RequestAnswerFriendInvite)

// 0x100689c0 - NetHandler slot 127
void NConsoleWnd::RequestFriendList()
{
    Engine->NetHandler->RequestFriendList();
}
NW_HOOK_METHOD(0x100689c0, NConsoleWnd, RequestFriendList)

// 0x100689d0 - NetHandler slot 129
void NConsoleWnd::RequestBlock(L2ParamStack& Params)
{
    Engine->NetHandler->RequestBlock(Params);
}
NW_HOOK_METHOD(0x100689d0, NConsoleWnd, RequestBlock)

// 0x10068a80 - NetHandler slot 130
void NConsoleWnd::RequestPartyInvite(L2ParamStack& Params)
{
    Engine->NetHandler->RequestJoinParty(Params);
}
NW_HOOK_METHOD(0x10068a80, NConsoleWnd, RequestPartyInvite)

// 0x10068a90 - NetHandler slot 131
void NConsoleWnd::RequestAnswerJoinParty(L2ParamStack& Params)
{
    Engine->NetHandler->RequestAnswerJoinParty(Params);
}
NW_HOOK_METHOD(0x10068a90, NConsoleWnd, RequestAnswerJoinParty)

// 0x10068aa0 - NetHandler slot 132
void NConsoleWnd::RequestPartyLeave(L2ParamStack& Params)
{
    Engine->NetHandler->RequestWithDrawalParty(Params);
}
NW_HOOK_METHOD(0x10068aa0, NConsoleWnd, RequestPartyLeave)

// 0x10068ab0 - NetHandler slot 133
void NConsoleWnd::RequestPartyDismiss(L2ParamStack& Params)
{
    Engine->NetHandler->RequestOustPartyMember(Params);
}
NW_HOOK_METHOD(0x10068ab0, NConsoleWnd, RequestPartyDismiss)

// The three C1 party matching requests (the configuration, the list, a detail): no such functions in the C3 image (the
// matching windows were redone); inert bodies
void NConsoleWnd::RequestPartyMatchConfig(L2ParamStack& Params)
{
}

void NConsoleWnd::RequestPartyMatchList(INT Status)
{
}

void NConsoleWnd::RequestPartyMatchDetail(INT ObjectID)
{
}

// 0x10068d30 - NetHandler slot 91
void NConsoleWnd::RequestLinkHtml(L2ParamStack& Params)
{
    Engine->NetHandler->RequestLinkHtml(Params);
}
NW_HOOK_METHOD(0x10068d30, NConsoleWnd, RequestLinkHtml)

// 0x10068d40 - NetHandler slot 93
void NConsoleWnd::RequestBypassToServer(L2ParamStack& Params)
{
    Engine->NetHandler->RequestBypassToServer(Params);
}
NW_HOOK_METHOD(0x10068d40, NConsoleWnd, RequestBypassToServer)

// 0x10068d50 - NetHandler slot 92
void NConsoleWnd::RequestBBSWrite(L2ParamStack& Params)
{
    Engine->NetHandler->RequestBBSWrite(Params);
}
NW_HOOK_METHOD(0x10068d50, NConsoleWnd, RequestBBSWrite)

// 0x10068d60 - NetHandler slot 163
void NConsoleWnd::RequestShowBoard(L2ParamStack& Params)
{
    Engine->NetHandler->RequestShowboard(Params);
}
NW_HOOK_METHOD(0x10068d60, NConsoleWnd, RequestShowBoard)

// 0x10068ec0 - NetHandler slot 94
void NConsoleWnd::RequestTutorialLinkHtml(L2ParamStack& Params)
{
    Engine->NetHandler->RequestTutorialLinkHtml(Params);
}
NW_HOOK_METHOD(0x10068ec0, NConsoleWnd, RequestTutorialLinkHtml)

// 0x10068ed0 - NetHandler slot 95
void NConsoleWnd::RequestTutorialPassCmdToServer(L2ParamStack& Params)
{
    Engine->NetHandler->RequestTutorialPassCmdToServer(Params);
}
NW_HOOK_METHOD(0x10068ed0, NConsoleWnd, RequestTutorialPassCmdToServer)

// 0x10068ee0 - NetHandler slot 96
void NConsoleWnd::RequestTutorialQuestionMark(INT ID)
{
    Engine->NetHandler->RequestTutorialQuestionMarkPressed(ID);
}
NW_HOOK_METHOD(0x10068ee0, NConsoleWnd, RequestTutorialQuestionMark)

// 0x10068fb0 - the item nearest to the pet within Range (NetHandler slot 210, a float) is
// picked up by it (slot 189 with the item's object id, its first field). The stock also tests
// the address of the item's field +0x10 for NULL, which it never is.
void NConsoleWnd::PetPickUp(INT Range, INT Param)
{
    Item* Found = Engine->NetHandler->GetPetNearestItem((FLOAT)Range, Param);
    if (Found)
        Engine->NetHandler->RequestPetGetItem(Found->ObjectID);
}
NW_HOOK_METHOD(0x10068fb0, NConsoleWnd, PetPickUp)

// 0x10068ff0 - NetHandler slot 190
void NConsoleWnd::RequestPetUseItem(INT ObjectID)
{
    Engine->NetHandler->RequestPetUseItem(ObjectID);
}
NW_HOOK_METHOD(0x10068ff0, NConsoleWnd, RequestPetUseItem)

// 0x10069000 - NetHandler slot 191
void NConsoleWnd::RequestGiveItemToPet(L2ParamStack& Params)
{
    Engine->NetHandler->RequestGiveItemToPet(Params);
}
NW_HOOK_METHOD(0x10069000, NConsoleWnd, RequestGiveItemToPet)

// 0x10069010 - NetHandler slot 192
void NConsoleWnd::RequestGetItemFromPet(L2ParamStack& Params)
{
    Engine->NetHandler->RequestGetItemFromPet(Params);
}
NW_HOOK_METHOD(0x10069010, NConsoleWnd, RequestGetItemFromPet)

// 0x10069020 - NetHandler slot 194
void NConsoleWnd::RequestChangePetName(TCHAR* Name)
{
    Engine->NetHandler->RequestChangePetName(Name);
}
NW_HOOK_METHOD(0x10069020, NConsoleWnd, RequestChangePetName)

// 0x10069140 - NetHandler slot 78 (C2: a second argument, passed on)
void NConsoleWnd::RequestPetition(TCHAR* Text, INT Type)
{
    Engine->NetHandler->RequestPetition(Text, Type);
}
NW_HOOK_METHOD(0x10069140, NConsoleWnd, RequestPetition)

// 0x10069150 - NetHandler slot 79
void NConsoleWnd::RequestPetitionCancel()
{
    Engine->NetHandler->RequestPetitionCancel();
}
NW_HOOK_METHOD(0x10069150, NConsoleWnd, RequestPetitionCancel)

// 0x10069160 - NetHandler slot 80
void NConsoleWnd::RequestGMList()
{
    Engine->NetHandler->RequestGMList();
}
NW_HOOK_METHOD(0x10069160, NConsoleWnd, RequestGMList)

// The GM window's queries about a character: NetHandler slot 81 (name, what).

// 0x10069170
void NConsoleWnd::RequestGMInventory(TCHAR* Name)
{
    Engine->NetHandler->RequestGMCommand(Name, 5);
}
NW_HOOK_METHOD(0x10069170, NConsoleWnd, RequestGMInventory)

// 0x10069190
void NConsoleWnd::RequestGMSkills(TCHAR* Name)
{
    Engine->NetHandler->RequestGMCommand(Name, 3);
}
NW_HOOK_METHOD(0x10069190, NConsoleWnd, RequestGMSkills)

// 0x100691b0
void NConsoleWnd::RequestGMStatus(TCHAR* Name)
{
    Engine->NetHandler->RequestGMCommand(Name, 1);
}
NW_HOOK_METHOD(0x100691b0, NConsoleWnd, RequestGMStatus)

// 0x100691d0
void NConsoleWnd::RequestGMQuests(TCHAR* Name)
{
    Engine->NetHandler->RequestGMCommand(Name, 4);
}
NW_HOOK_METHOD(0x100691d0, NConsoleWnd, RequestGMQuests)

// 0x100691f0
void NConsoleWnd::RequestGMWarehouse(TCHAR* Name)
{
    Engine->NetHandler->RequestGMCommand(Name, 6);
}
NW_HOOK_METHOD(0x100691f0, NConsoleWnd, RequestGMWarehouse)

// 0x10069210
void NConsoleWnd::RequestGMPledge(TCHAR* Name)
{
    Engine->NetHandler->RequestGMCommand(Name, 2);
}
NW_HOOK_METHOD(0x10069210, NConsoleWnd, RequestGMPledge)

// The private stores: selling (NetHandler slots 195..201, packets 0x73..0x79) and buying
// (202..208, packets 0x90..0x96).

// 0x100698a0 - "/vendor": manage the selling store
void NConsoleWnd::RequestVendor()
{
    Engine->NetHandler->RequestPrivateStoreManage();
}
NW_HOOK_METHOD(0x100698a0, NConsoleWnd, RequestVendor)

// 0x100698b0 - the managed selling list (item, amount, price...)
void NConsoleWnd::RequestSetPrivateStoreListSell(L2ParamStack& Params)
{
    Engine->NetHandler->SetPrivateStoreList(Params);
}
NW_HOOK_METHOD(0x100698b0, NConsoleWnd, RequestSetPrivateStoreListSell)

// 0x100698c0 - the managing window closed without opening the store
void NConsoleWnd::RequestPrivateStoreManageCancel()
{
    Engine->NetHandler->RequestPrivateStoreManageCancel();
}
NW_HOOK_METHOD(0x100698c0, NConsoleWnd, RequestPrivateStoreManageCancel)

// 0x100698d0
void NConsoleWnd::RequestPrivateStoreQuitSell()
{
    Engine->NetHandler->RequestPrivateStoreQuit();
}
NW_HOOK_METHOD(0x100698d0, NConsoleWnd, RequestPrivateStoreQuitSell)

// 0x100698e0
void NConsoleWnd::SetPrivateStoreMsg(TCHAR* Text)
{
    Engine->NetHandler->SetPrivateStoreMsg(Text);
}
NW_HOOK_METHOD(0x100698e0, NConsoleWnd, SetPrivateStoreMsg)

// 0x100698f0 - buying from another player's store
void NConsoleWnd::RequestPrivateStoreBuy(L2ParamStack& Params)
{
    Engine->NetHandler->SendPrivateStoreBuyList(Params);
}
NW_HOOK_METHOD(0x100698f0, NConsoleWnd, RequestPrivateStoreBuy)

// 0x10069a80 - "/buy": manage the buying store
void NConsoleWnd::RequestBuy()
{
    Engine->NetHandler->RequestPrivateStoreBuyManage();
}
NW_HOOK_METHOD(0x10069a80, NConsoleWnd, RequestBuy)

// 0x10069a90
void NConsoleWnd::RequestSetPrivateStoreListBuy(L2ParamStack& Params)
{
    Engine->NetHandler->SetPrivateStoreBuyList(Params);
}
NW_HOOK_METHOD(0x10069a90, NConsoleWnd, RequestSetPrivateStoreListBuy)

// 0x10069aa0
void NConsoleWnd::RequestPrivateStoreBuyManageCancel()
{
    Engine->NetHandler->RequestPrivateStoreBuyManageCancel();
}
NW_HOOK_METHOD(0x10069aa0, NConsoleWnd, RequestPrivateStoreBuyManageCancel)

// 0x10069ab0
void NConsoleWnd::RequestPrivateStoreQuitBuy()
{
    Engine->NetHandler->RequestPrivateStoreBuyQuit();
}
NW_HOOK_METHOD(0x10069ab0, NConsoleWnd, RequestPrivateStoreQuitBuy)

// 0x10069ac0
void NConsoleWnd::SetPrivateStoreBuyMsg(TCHAR* Text)
{
    Engine->NetHandler->SetPrivateStoreBuyMsg(Text);
}
NW_HOOK_METHOD(0x10069ac0, NConsoleWnd, SetPrivateStoreBuyMsg)

// 0x10069ad0 - selling to another player's buying store
void NConsoleWnd::RequestPrivateStoreSell(L2ParamStack& Params)
{
    Engine->NetHandler->SendPrivateStoreBuyBuyList(Params);
}
NW_HOOK_METHOD(0x10069ad0, NConsoleWnd, RequestPrivateStoreSell)

// 0x10069ff0 - NetHandler slot 248 (C2 only; name ours, after the slot's)
void NConsoleWnd::BypassUserCmd(L2ParamStack& Params)
{
    Engine->NetHandler->BypassUserCmd(Params);
}
NW_HOOK_METHOD(0x10069ff0, NConsoleWnd, BypassUserCmd)

// 0x1006a000 - NetHandler slot 247
void NConsoleWnd::RequestDwarvenManufacture()
{
    Engine->NetHandler->RequestRemainTime();
}
NW_HOOK_METHOD(0x1006a000, NConsoleWnd, RequestDwarvenManufacture)

// 0x1006a1a0 - "Previous" on the character selection: back to the login screen (the logout and the reconnect, NetHandler
// +0x280 and +0x2D4). The desktop does its LogoutMessenger first and the login stages' dialog box is hidden
void NConsoleWnd::ReturnToLogin()
{
    ((NCConsoleExt*)Desktop)->LogoutMessenger();
    LobbyWnd->ClearCharacters();
    LobbyWnd->HideWindow();
    Engine->NetHandler->SendLogOutPacket();
    Engine->NetHandler->AuthReconnect();
    ((NCLoginWnd*)LoginGroup(this))->ResetWindows();
    LoginGroup(this)->ShowWindow();
    nw::Field<NCWnd*>(GodWnd, 0xFC)->HideWindow();
    nw::Field<NCMessageWnd*>(GodWnd, 0xF0)->SetText(TEXT("Copyright 2002 NCsoft Corporation. All Rights Reserved."));
    MoveCameraByState(L2CS_None, L2CS_Login);
    SetState(L2CS_Login);
}
NW_HOOK_METHOD(0x1006a1a0, NConsoleWnd, ReturnToLogin)

// 0x1006a740 - the item lying on the ground with that object id, by the network handler's +0x9C (the C3 engine's
// slots before the users' moved). Named GetServerID before: it takes the object id (UpdateCursor pushes it)
Item* NConsoleWnd::FindItemByID(INT ObjectID)
{
    return nw::VirtualCall<Item*>(Engine->NetHandler, 0x9C / 4, ObjectID);
}
NW_HOOK_METHOD(0x1006a740, NConsoleWnd, FindItemByID)

// 0x1006a750 - NetHandler slot 31
PetInfo NConsoleWnd::GetPetInfo()
{
    return Engine->NetHandler->GetPetInfo();
}
NW_HOOK_METHOD(0x1006a750, NConsoleWnd, GetPetInfo)

// 0x1006a770 - NetHandler slot 32
User* NConsoleWnd::FindUserByName(TCHAR* Name)
{
    return Engine->NetHandler->GetUser(Name);
}
NW_HOOK_METHOD(0x1006a770, NConsoleWnd, FindUserByName)

// 0x1006a780 - NetHandler slot 33 (not inlined: the stock calls it)
__declspec(noinline) User* NConsoleWnd::FindUserByID(INT ObjectID)
{
    return Engine->NetHandler->GetUser(ObjectID);
}
NW_HOOK_METHOD(0x1006a780, NConsoleWnd, FindUserByID)

// 0x1006a790 - NetHandler slot 34
FServerStaticObject* NConsoleWnd::FindStaticObjectInfo(INT CreatureID)
{
    return Engine->NetHandler->GetStaticObject(CreatureID);
}
NW_HOOK_METHOD(0x1006a790, NConsoleWnd, FindStaticObjectInfo)

// 0x1006a7a0 - the pawn under the mouse; with bCheck, only a player's (its User record,
// NetHandler slot 33, without Unknown8)
AActor* NConsoleWnd::GetTargetedActor(UBOOL bCheck)
{
    guard(NConsoleWnd::GetTargetedActor);
    if (!Engine || !HoveredActor(Engine) || !HoveredActor(Engine)->IsA(APawn::StaticClass()))
        return NULL;
    AActor* Actor = HoveredActor(Engine);
    if (bCheck)
    {
        User* Info = Engine->NetHandler->GetUser(Actor->CreatureID);
        if (!Info || Info->Unknown8)
            return NULL;
    }
    return Actor;
    unguard;
}
NW_HOOK_METHOD(0x1006a7a0, NConsoleWnd, GetTargetedActor)

// 0x1006a8d0 - NetHandler slot 38
PledgeInfo* NConsoleWnd::GetPledgeInfo(INT PledgeID)
{
    return Engine->NetHandler->GetPledgeInfo(PledgeID);
}
NW_HOOK_METHOD(0x1006a8d0, NConsoleWnd, GetPledgeInfo)

// 0x1006a8e0 - NetHandler +0xAC (C4; the clan's crest by id)
UTexture* NConsoleWnd::GetPledgeCrestTexture(INT PledgeID)
{
    return ::nw::VirtualCall<UTexture*>(Engine->NetHandler, 0xAC / 4, PledgeID);
}
NW_HOOK_METHOD(0x1006a8e0, NConsoleWnd, GetPledgeCrestTexture)

// 0x1006a8f0 - NetHandler +0xB0 (C4; the alliance's crest by id)
UTexture* NConsoleWnd::GetAllyCrestTexture(INT AllyID)
{
    return ::nw::VirtualCall<UTexture*>(Engine->NetHandler, 0xB0 / 4, AllyID);
}
NW_HOOK_METHOD(0x1006a8f0, NConsoleWnd, GetAllyCrestTexture)

// 0x1006a900 / 0x1006a910 - the same thunks one slot further (C4: NetHandler slots 45 / 46, the clan's / alliance's crest by
// its id); NCGDevice::DrawPledgeCrest / DrawAllyCrest and the siege list paint call these, not 0x1006A8E0 / 0x1006A8F0
UTexture* NConsoleWnd::GetPledgeCrestTex(INT PledgeID)
{
    return ::nw::VirtualCall<UTexture*>(Engine->NetHandler, 45, PledgeID);
}
NW_HOOK_METHOD(0x1006a900, NConsoleWnd, GetPledgeCrestTex)

UTexture* NConsoleWnd::GetAllyCrestTex(INT AllyID)
{
    return ::nw::VirtualCall<UTexture*>(Engine->NetHandler, 46, AllyID);
}
NW_HOOK_METHOD(0x1006a910, NConsoleWnd, GetAllyCrestTex)

// 0x1006ff50 - NetHandler slot 87 (by value)
void NConsoleWnd::RequestDropItem(NItemRequest Request, FVector Location)
{
    guard(NConsoleWnd::RequestDropItem);
    Engine->NetHandler->RequestDropItem(Request, Location);
    unguard;
}
NW_HOOK_METHOD(0x1006ff50, NConsoleWnd, RequestDropItem)

// The actions on a target: NetHandler slot 53 (object id, where the player stands, Shift held)
// selects it, slot 52 (same) attacks it.

// 0x10071820 - a click on the player's status window
void NConsoleWnd::TargetMyself()
{
    APawn* Pawn = PlayerPawn(Engine);
    if (!Pawn)
        return;
    FVector Origin = Pawn->Location;
    Engine->NetHandler->Action(PlayerPawn(Engine)->CreatureID, Origin,
                               (UBOOL)Engine->Client->Viewports(0)->Input->KeyDown(IK_Shift));
}
NW_HOOK_METHOD(0x10071820, NConsoleWnd, TargetMyself)

// 0x100718b0
void NConsoleWnd::RequestTarget(INT ObjectID)
{
    APawn* Pawn = PlayerPawn(Engine);
    if (!Pawn)
        return;
    FVector Origin = Pawn->Location;
    Engine->NetHandler->Action(ObjectID, Origin,
                               (UBOOL)Engine->Client->Viewports(0)->Input->KeyDown(IK_Shift));
}
NW_HOOK_METHOD(0x100718b0, NConsoleWnd, RequestTarget)

// 0x10071930 - the item nearest to the player within Range (NetHandler slot 211, a float); the
// stock tests the address of its field +0x10 too, never NULL
void NConsoleWnd::RequestPickUp(INT Range, INT ObjectID)
{
    Item* Found = Engine->NetHandler->GetNearestItem((FLOAT)Range, ObjectID);
    if (!Found)
        return;
    Engine->NetHandler->Action(Found->ObjectID, PlayerPawn(Engine)->Location,
                               (UBOOL)Engine->Client->Viewports(0)->Input->KeyDown(IK_Shift));
}
NW_HOOK_METHOD(0x10071930, NConsoleWnd, RequestPickUp)

// 0x100719c0 - the next creature within Range after CurrentID (NetHandler +0x3C4); C3 sets the engine's +0x17C flag
// (read by RequestAttack: it ignores the attack while it is set) before the action
void NConsoleWnd::RequestTargetNext(INT Range, INT CurrentID)
{
    User* Next = Engine->NetHandler->GetNextEnemy((FLOAT)Range, CurrentID);
    if (!Next || !Next->Pawn)
        return;
    nw::Field<INT>(Engine, 0x188) = 1;   // C4: +0x188 (C3 +0x17C)
    Engine->NetHandler->Action(Next->ObjectID, PlayerPawn(Engine)->Location,
                               (UBOOL)Engine->Client->Viewports(0)->Input->KeyDown(IK_Shift));
}
NW_HOOK_METHOD(0x100719c0, NConsoleWnd, RequestTargetNext)

// 0x10071a60 - what the party member targets (C2: the member's target may be a static object, found by the network
// handler's slot 35; the action starts from the player's own place: the Origin argument is not read)
void NConsoleWnd::AssistMember(INT ObjectID, FVector Origin)
{
    User* Member = FindUserByID(ObjectID);
    if (!Member || !Member->Pawn)
        return;
    AController* Controller = Member->Pawn->Controller;
    if (!Controller || !Controller->Target)
        return;
    User* Target = FindUserByID(Controller->Target->CreatureID);
    INT TargetObjectID;
    if (Target)
        TargetObjectID = Target->ObjectID;
    else
    {
        FServerStaticObject* StaticObject = Engine->NetHandler->GetStaticObject(Member->Pawn->Controller->Target->CreatureID);
        if (!StaticObject)
            return;
        TargetObjectID = StaticObject->Unknown8;
    }
    Engine->NetHandler->Action(TargetObjectID, PlayerPawn(Engine)->Location,
                               (UBOOL)Engine->Client->Viewports(0)->Input->KeyDown(IK_Shift));
}
NW_HOOK_METHOD(0x10071a60, NConsoleWnd, AssistMember)

// 0x10071ba0 - a click on the pet / summon status window
void NConsoleWnd::RequestAction(INT ObjectID, FVector Origin)
{
    Engine->NetHandler->Action(ObjectID, Origin,
                               (UBOOL)Engine->Client->Viewports(0)->Input->KeyDown(IK_Shift));
}
NW_HOOK_METHOD(0x10071ba0, NConsoleWnd, RequestAction)

// 0x10073e80 - the player's User record (not inlined: the stock calls it)
__declspec(noinline) User* NConsoleWnd::GetMyUser()
{
    APawn* Pawn = PlayerPawn(Engine);
    if (!Pawn)
        return NULL;
    return FindUserByID(Pawn->CreatureID);
}
NW_HOOK_METHOD(0x10073e80, NConsoleWnd, GetMyUser)

// 0x10073ed0 - the User record of the player's target (not inlined: the stock calls it)
__declspec(noinline) User* NConsoleWnd::GetSelectedActor()
{
    guard(NConsoleWnd::GetSelectedActor);
    APawn* Pawn = PlayerPawn(Engine);
    if (Engine && Pawn && Pawn->Controller && Pawn->Controller->Target)
        return FindUserByID(Pawn->Controller->Target->CreatureID);
    return NULL;
    unguard;
}
NW_HOOK_METHOD(0x10073ed0, NConsoleWnd, GetSelectedActor)

// 0x10073f80 - the player's target as an actor (doors and the other static objects have no
// User record)
AActor* NConsoleWnd::GetSelectedStaticActor()
{
    APawn* Pawn = PlayerPawn(Engine);
    if (Engine && Pawn && Pawn->Controller && Pawn->Controller->Target)
        return Pawn->Controller->Target;
    return NULL;
}
NW_HOOK_METHOD(0x10073f80, NConsoleWnd, GetSelectedStaticActor)

// 0x1006a3a0  NOT PORTED (no C2 counterpart); nothing calls it, so the standalone has no body for it
#if NW_HOOK_MODE
UBOOL NConsoleWnd::HasSelectedTarget()
{
    NW_UNPORTED_NORET();
}
#endif

// C2 CANDIDATE: the C2 function is not the same code as the C1 one; kept only while it matches the stock one (verify_data)
// 0x10074740 - move there (NetHandler slot 90: the player's id, 3 and the point in whole units,
// or 2 and no height)
UBOOL NConsoleWnd::MoveToLocation(FVector Loc, UBOOL bNoZ)
{
    APawn* Pawn = PlayerPawn(Engine);
    if (bNoZ)
        Engine->NetHandler->RequestTeleport(Pawn->CreatureID, 2, (INT)Loc.X, (INT)Loc.Y, 0);
    else
        Engine->NetHandler->RequestTeleport(Pawn->CreatureID, 3, (INT)Loc.X, (INT)Loc.Y,
                                            (INT)Loc.Z);
    return 1;
}
NW_HOOK_METHOD(0x10074740, NConsoleWnd, MoveToLocation)

// 0x1007a210 - what the target targets (C2: a static object's too: slot 35 of the network handler)
void NConsoleWnd::RequestAssist()
{
    User* Target = GetSelectedActor();
    if (!Target || !Target->Pawn)
        return;
    AController* Controller = Target->Pawn->Controller;
    if (!Controller || !Controller->Target)
        return;
    User* AssistTarget = FindUserByID(Controller->Target->CreatureID);
    INT ObjectID;
    if (AssistTarget)
        ObjectID = AssistTarget->ObjectID;
    else
    {
        FServerStaticObject* StaticObject = Engine->NetHandler->GetStaticObject(Target->Pawn->Controller->Target->CreatureID);
        if (!StaticObject)
            return;
        ObjectID = StaticObject->Unknown8;
    }
    Engine->NetHandler->Action(ObjectID, PlayerPawn(Engine)->Location,
                               (UBOOL)Engine->Client->Viewports(0)->Input->KeyDown(IK_Shift));
}
NW_HOOK_METHOD(0x1007a210, NConsoleWnd, RequestAssist)


/*-----------------------------------------------------------------------------
    The virtual requests: UL2ConsoleWnd slots the engine leaves pure and the console's own
    (272..284), which the windows call through the vtable.
-----------------------------------------------------------------------------*/

// 0x10065640 - NetHandler slot 69
void NConsoleWnd::RequestServerList()
{
    guard(NConsoleWnd::RequestServerList);
    Engine->NetHandler->RequestServerList();
    unguard;
}
NW_HOOK_METHOD(0x10065640, NConsoleWnd, RequestServerList)

// 0x10066030 (vslot 427) - the answer to a revive request: nothing in this client (the stock is a `ret 0xC`)
void NConsoleWnd::AnswerReviveRequest(UBOOL bYes, INT Param1, INT Param2)
{
}
NW_HOOK_METHOD(0x10066030, NConsoleWnd, AnswerReviveRequest)

// 0x10066610 - NetHandler slot 171: the item the enchant scroll goes on
void NConsoleWnd::RequestEnchantItem(INT ObjectID)
{
    guard(NConsoleWnd::RequestEnchantItem);
    Engine->NetHandler->RequestEnchantItem(ObjectID);
    unguard;
}
NW_HOOK_METHOD(0x10066610, NConsoleWnd, RequestEnchantItem)

// 0x100666e0 - NetHandler slot 165
void NConsoleWnd::RequestDestroyItem(NItemRequest Request)
{
    guard(NConsoleWnd::RequestDestroyItem);
    Engine->NetHandler->RequestDestroyItem(Request);
    unguard;
}
NW_HOOK_METHOD(0x100666e0, NConsoleWnd, RequestDestroyItem)

// 0x10066770 - NetHandler slot 166
void NConsoleWnd::RequestCrystallizeItem(INT ObjectID, INT Count)
{
    Engine->NetHandler->RequestCrystallizeItem(ObjectID, Count);
}
NW_HOOK_METHOD(0x10066770, NConsoleWnd, RequestCrystallizeItem)

// 0x10066910 - NetHandler slot 156: the answer to a trade offer
void NConsoleWnd::AnswerTradeRequest(L2ParamStack& Params)
{
    Engine->NetHandler->AnswerTradeRequest(Params);
}
NW_HOOK_METHOD(0x10066910, NConsoleWnd, AnswerTradeRequest)

// 0x10067150 - NetHandler slot 173 (C2: not in the Thai HTML mode either)
UBOOL NConsoleWnd::RequestQuestList()
{
    guard(NConsoleWnd::RequestQuestList);
    if (GIsOnlyL2Console || GL2ThaiHtmlMode)
        return 0;
    return Engine->NetHandler->RequestQuestList();
    unguard;
}
NW_HOOK_METHOD(0x10067150, NConsoleWnd, RequestQuestList)

// 0x100671f0 - NetHandler slot 170
void NConsoleWnd::RequestDestroyQuest(INT QuestID)
{
    guard(NConsoleWnd::RequestDestroyQuest);
    Engine->NetHandler->RequestDestroyQuest(QuestID);
    unguard;
}
NW_HOOK_METHOD(0x100671f0, NConsoleWnd, RequestDestroyQuest)

// 0x10067fc0 - NetHandler slot 172: what a skill of the trainer needs
void NConsoleWnd::RequestAcquireSkillInfo(L2ParamStack& Params)
{
    Engine->NetHandler->RequestAcquireSkillInfo(Params);
}
NW_HOOK_METHOD(0x10067fc0, NConsoleWnd, RequestAcquireSkillInfo)

// 0x10067fd0 - NetHandler slot 173
void NConsoleWnd::RequestAcquireSkill(L2ParamStack& Params)
{
    Engine->NetHandler->RequestAcquireSkill(Params);
}
NW_HOOK_METHOD(0x10067fd0, NConsoleWnd, RequestAcquireSkill)

// 0x100688b0 - NetHandler slot 175
void NConsoleWnd::RequestStartAllianceWar(L2ParamStack& Params)
{
    Engine->NetHandler->RequestStartAllianceWar(Params);
}
NW_HOOK_METHOD(0x100688b0, NConsoleWnd, RequestStartAllianceWar)

// 0x100688c0 - NetHandler slot 176
void NConsoleWnd::RequestReplyStartAllianceWar(L2ParamStack& Params)
{
    Engine->NetHandler->ReplyStartAllianceWar(Params);
}
NW_HOOK_METHOD(0x100688c0, NConsoleWnd, RequestReplyStartAllianceWar)

// 0x100688d0 - NetHandler slot 177
void NConsoleWnd::RequestStopAllianceWar(L2ParamStack& Params)
{
    Engine->NetHandler->RequestStopAllianceWar(Params);
}
NW_HOOK_METHOD(0x100688d0, NConsoleWnd, RequestStopAllianceWar)

// 0x100688e0 - NetHandler slot 178
void NConsoleWnd::RequestReplyStopAllianceWar(L2ParamStack& Params)
{
    Engine->NetHandler->ReplyStopAllianceWar(Params);
}
NW_HOOK_METHOD(0x100688e0, NConsoleWnd, RequestReplyStopAllianceWar)

// 0x100688f0 - NetHandler slot 179
void NConsoleWnd::RequestSurrenderAllianceWar(L2ParamStack& Params)
{
    Engine->NetHandler->RequestSurrenderAllianceWar(Params);
}
NW_HOOK_METHOD(0x100688f0, NConsoleWnd, RequestSurrenderAllianceWar)

// 0x10068900 - NetHandler slot 187: the freight
void NConsoleWnd::RequestPackageSend(L2ParamStack& Params)
{
    Engine->NetHandler->RequestPackageSend(Params);
}
NW_HOOK_METHOD(0x10068900, NConsoleWnd, RequestPackageSend)

// 0x10068910 - NetHandler slot 188
void NConsoleWnd::RequestPackageSendableItemList(L2ParamStack& Params)
{
    Engine->NetHandler->RequestPackageSendableItemList(Params);
}
NW_HOOK_METHOD(0x10068910, NConsoleWnd, RequestPackageSendableItemList)

// 0x100689e0 - NetHandler slot 180 (no caller in NWindow)
void NConsoleWnd::vfn184(L2ParamStack& Params)
{
    Engine->NetHandler->RequestCastleSiegeInfo(Params);
}
NW_HOOK_METHOD(0x100689e0, NConsoleWnd, vfn184)

// 0x100689f0 - NetHandler slot 181
void NConsoleWnd::RequestSiegeAttackerList(L2ParamStack& Params)
{
    Engine->NetHandler->RequestCastleSiegeAttackerList(Params);
}
NW_HOOK_METHOD(0x100689f0, NConsoleWnd, RequestSiegeAttackerList)

// 0x10068a00 - NetHandler slot 182
void NConsoleWnd::RequestSiegeDefenderList(L2ParamStack& Params)
{
    Engine->NetHandler->RequestCastleSiegeDefenderList(Params);
}
NW_HOOK_METHOD(0x10068a00, NConsoleWnd, RequestSiegeDefenderList)

// 0x10068a10 - NetHandler slot 183
void NConsoleWnd::RequestJoinSiege(L2ParamStack& Params)
{
    Engine->NetHandler->RequestJoinCastleSiege(Params);
}
NW_HOOK_METHOD(0x10068a10, NConsoleWnd, RequestJoinSiege)

// 0x10068a20 - NetHandler slot 184
void NConsoleWnd::RequestConfirmSiegeWaitingList(L2ParamStack& Params)
{
    Engine->NetHandler->RequestConfirmCastleSiegeWaitingList(Params);
}
NW_HOOK_METHOD(0x10068a20, NConsoleWnd, RequestConfirmSiegeWaitingList)

// 0x10068a30 - NetHandler slot 185: the siege date
void NConsoleWnd::RequestSetSiegeTime(L2ParamStack& Params)
{
    Engine->NetHandler->RequestSetCastleSiegeTime(Params);
}
NW_HOOK_METHOD(0x10068a30, NConsoleWnd, RequestSetSiegeTime)

// 0x10068ef0 - the tutorial's client events: each pending one (TutorialEnableClientEvent) is sent once (NetHandler
// +0x1A8). C3: the pending bits are at +0x39BC (the header's TutorialEvents is at +0x39C0)
void NConsoleWnd::SendTutorialEvent(INT Event)
{
    if (TutorialEvents & Event)
    {
        Engine->NetHandler->RequestTutorialClientEvent(Event);
        TutorialEvents ^= Event;
    }
}
NW_HOOK_METHOD(0x10068ef0, NConsoleWnd, SendTutorialEvent)

// 0x10069b70 - NetHandler slot 186
void NConsoleWnd::RequestBuyVIPItem(L2ParamStack& Params)
{
    Engine->NetHandler->RequestMultiSellChoose(Params);
}
NW_HOOK_METHOD(0x10069b70, NConsoleWnd, RequestBuyVIPItem)

// 0x1006a010 - NetHandler slot 164: a GM command
void NConsoleWnd::BuilderCommand(L2ParamStack& Params)
{
    Engine->NetHandler->SendBypassBuildCmd(Params);
}
NW_HOOK_METHOD(0x1006a010, NConsoleWnd, BuilderCommand)

// 0x1006b6b0 - the engine header gave this slot (246) a stack argument that the function does not have in this client (it has none
// now: include/ue2/gen/UL2ConsoleWnd.vt.inc): the stock entry is hooked as NConsoleWnd::C4Virtual246 (NConsoleWndC4.cpp). Hook mode:
// this stub only keeps the virtual defined; standalone: the slot IS C4Virtual246 (the stock slot holds 0x1006b6b0 itself)
#if NW_HOOK_MODE
NW_NAKED void NConsoleWnd::vfn176() NW_STOCK(0x1006b6b0)
#else
NW_SLOT_IS("?vfn176@NConsoleWnd@@UAEXXZ", "?C4Virtual246@NConsoleWnd@@QAEXXZ")   // NConsoleWnd::vfn176 = NConsoleWnd::C4Virtual246 (0x1006b6b0)
#endif

// RequestRestart: the C3 twin of RequestRestartPoint (0x2b0 against 0x2b4); this client has one function for both
// (0x10065530), hooked as RequestRestartPoint below
#if NW_HOOK_MODE
NW_NAKED void NConsoleWnd::RequestRestart(L2ParamStack& Params) NW_STOCK(0x10065530)
#else
void NConsoleWnd::RequestRestart(L2ParamStack& Params)
{
    RequestRestartPoint(Params);
}
#endif

/*-----------------------------------------------------------------------------
    The thin wrappers over the network handler of this client (each is `mov ecx, [this+0x54]; mov ecx, [ecx+0x64]; jmp [vtable+X]`).
    The C3 addresses of the C3 names are not the C4 ones: this client has one wrapper more before RequestMakeMacro (0x10067ff0,
    +0x2f0), the recipe shop's wrappers sit 0x10 lower and the clan's slots are 0x10 higher; the places and the argument counts
    here come from the stock callers (NCMacroEditWnd::OnSave, NCRecipeBuyListWnd::OnSelectRecipe, NCGaraDialogBox::OnOK ...).
-----------------------------------------------------------------------------*/

// 0x10065530 - NetHandler +0x2B4 the restart menu (RequestRestart, the C3 twin at +0x2b0, is folded into it in this client)
void NConsoleWnd::RequestRestartPoint(L2ParamStack& Params)
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x2B4 / 4, &Params);
}
NW_HOOK_METHOD(0x10065530, NConsoleWnd, RequestRestartPoint)

// 0x10068020 - NetHandler +0x264 the macro editor saves a macro
void NConsoleWnd::RequestMakeMacro(L2ParamStack& Params)
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x264 / 4, &Params);
}
NW_HOOK_METHOD(0x10068020, NConsoleWnd, RequestMakeMacro)

// 0x10068030 - NetHandler +0x26C C3 name kept (NCMenuWnd calls it; the engine header calls the slot RequestOpenMinimap)
void NConsoleWnd::RequestDeleteMacro(L2ParamStack& Params)
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x26C / 4, &Params);
}
NW_HOOK_METHOD(0x10068030, NConsoleWnd, RequestDeleteMacro)

// 0x10068040 - NetHandler +0x268 C3 name kept (the engine header calls the slot RequestDeleteMacro: the macro list asks it)
void NConsoleWnd::RequestMagicSkillUse(L2ParamStack& Params)
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x268 / 4, &Params);
}
NW_HOOK_METHOD(0x10068040, NConsoleWnd, RequestMagicSkillUse)

// 0x10068050 - NetHandler +0x278 C3 name kept (the chat commands call it; the engine header calls the slot RequestMagicSkillUse)
void NConsoleWnd::RequestSkillList(L2ParamStack& Params)
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x278 / 4, &Params);
}
NW_HOOK_METHOD(0x10068050, NConsoleWnd, RequestSkillList)

// 0x100680a0 - NetHandler +0x354 a parameter stack now (C3: none)
void NConsoleWnd::RequestRecipeBookOpen(L2ParamStack& Params)
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x354 / 4, &Params);
}
NW_HOOK_METHOD(0x100680a0, NConsoleWnd, RequestRecipeBookOpen)

// 0x100680e0 - NetHandler +0x368
void NConsoleWnd::RequestRecipeShopMessageSet(TCHAR* P1)
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x368 / 4, P1);
}
NW_HOOK_METHOD(0x100680e0, NConsoleWnd, RequestRecipeShopMessageSet)

// 0x100680f0 - NetHandler +0x36C
void NConsoleWnd::RequestRecipeShopListSet(L2ParamStack& P1)
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x36C / 4, &P1);
}
NW_HOOK_METHOD(0x100680f0, NConsoleWnd, RequestRecipeShopListSet)

// 0x10068100 - NetHandler +0x370
void NConsoleWnd::RequestRecipeShopManageQuit()
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x370 / 4);
}
NW_HOOK_METHOD(0x10068100, NConsoleWnd, RequestRecipeShopManageQuit)

// 0x10068110 - NetHandler +0x374
void NConsoleWnd::RequestRecipeShopManageCancel()
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x374 / 4);
}
NW_HOOK_METHOD(0x10068110, NConsoleWnd, RequestRecipeShopManageCancel)

// 0x10068120 - NetHandler +0x378
void NConsoleWnd::RequestRecipeShopSellList(INT P1)
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x378 / 4, P1);
}
NW_HOOK_METHOD(0x10068120, NConsoleWnd, RequestRecipeShopSellList)

// 0x10068130 - NetHandler +0x37C
void NConsoleWnd::RequestRecipeShopMakeInfo(INT P1, INT P2)
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x37C / 4, P1, P2);
}
NW_HOOK_METHOD(0x10068130, NConsoleWnd, RequestRecipeShopMakeInfo)

// 0x10068140 - NetHandler +0x380
void NConsoleWnd::RequestRecipeShopMakeDo(INT P1, INT P2, INT P3)
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x380 / 4, P1, P2, P3);
}
NW_HOOK_METHOD(0x10068140, NConsoleWnd, RequestRecipeShopMakeDo)

// 0x100684f0 - NetHandler +0x1D0 the clan creation dialog (was RequestPledgeMemberList)
void NConsoleWnd::RequestCreatePledge(L2ParamStack& Params)
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x1D0 / 4, &Params);
}
NW_HOOK_METHOD(0x100684f0, NConsoleWnd, RequestCreatePledge)

// 0x10068500 - NetHandler +0x1D4 the answer to an invitation (was RequestNet103)
void NConsoleWnd::RequestAnswerJoinPledge(L2ParamStack& Params)
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x1D4 / 4, &Params);
}
NW_HOOK_METHOD(0x10068500, NConsoleWnd, RequestAnswerJoinPledge)

// 0x10068510 - NetHandler +0x1D8 (was RequestAnswerJoinPledge)
void NConsoleWnd::RequestOustPledgeMember(L2ParamStack& Params)
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x1D8 / 4, &Params);
}
NW_HOOK_METHOD(0x10068510, NConsoleWnd, RequestOustPledgeMember)

// 0x10068520 - NetHandler +0x1DC no argument (was RequestOustPledgeMember)
void NConsoleWnd::RequestDismissPledge()
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x1DC / 4);
}
NW_HOOK_METHOD(0x10068520, NConsoleWnd, RequestDismissPledge)

// 0x10068530 - NetHandler +0x1E8 (was RequestNet106)
void NConsoleWnd::RequestStartPledgeWar(L2ParamStack& Params)
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x1E8 / 4, &Params);
}
NW_HOOK_METHOD(0x10068530, NConsoleWnd, RequestStartPledgeWar)

// 0x10068540 - NetHandler +0x1EC (was RequestStartPledgeWar)
void NConsoleWnd::RequestReplyStartPledgeWar(L2ParamStack& Params)
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x1EC / 4, &Params);
}
NW_HOOK_METHOD(0x10068540, NConsoleWnd, RequestReplyStartPledgeWar)

// 0x10068550 - NetHandler +0x1F0 (was RequestReplyStartPledgeWar)
void NConsoleWnd::RequestStopPledgeWar(L2ParamStack& Params)
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x1F0 / 4, &Params);
}
NW_HOOK_METHOD(0x10068550, NConsoleWnd, RequestStopPledgeWar)

// 0x10068560 - NetHandler +0x1F4 (was RequestStopPledgeWar)
void NConsoleWnd::RequestReplyStopPledgeWar(L2ParamStack& Params)
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x1F4 / 4, &Params);
}
NW_HOOK_METHOD(0x10068560, NConsoleWnd, RequestReplyStopPledgeWar)

// 0x10068570 - NetHandler +0x1F8 (was RequestReplyStopPledgeWar)
void NConsoleWnd::RequestSurrenderPledgeWar(L2ParamStack& Params)
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x1F8 / 4, &Params);
}
NW_HOOK_METHOD(0x10068570, NConsoleWnd, RequestSurrenderPledgeWar)

// 0x10068580 - NetHandler +0x200 (was RequestSurrenderPledgeWar)
void NConsoleWnd::RequestSurrenderPersonally(L2ParamStack& Params)
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x200 / 4, &Params);
}
NW_HOOK_METHOD(0x10068580, NConsoleWnd, RequestSurrenderPersonally)

// 0x10068590 - NetHandler +0x204 (was RequestSurrenderPersonally)
void NConsoleWnd::RequestGiveNickName(L2ParamStack& Params)
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x204 / 4, &Params);
}
NW_HOOK_METHOD(0x10068590, NConsoleWnd, RequestGiveNickName)

// 0x100685a0 - NetHandler +0x1E0 the invitation (was RequestGiveNickName)
void NConsoleWnd::RequestJoinPledge(L2ParamStack& Params)
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x1E0 / 4, &Params);
}
NW_HOOK_METHOD(0x100685a0, NConsoleWnd, RequestJoinPledge)

// 0x100685b0 - NetHandler +0x1E4 no argument (was RequestJoinPledge)
void NConsoleWnd::RequestWithdrawalPledge()
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x1E4 / 4);
}
NW_HOOK_METHOD(0x100685b0, NConsoleWnd, RequestWithdrawalPledge)
