/*=============================================================================
    NCConsole.cpp - the desktop. Stock: 0x1003d740 .. 0x1003d7d1, 0x1003d7e0 .. 0x1003daa0,
    0x1003dac0 .. 0x1003e716, 0x1003e950 .. 0x10040f1e, 0x10041800 .. 0x10041b19,
    0x10041bc0 .. 0x100424d1, 0x10042530 .. 0x100427ba (HideWindow is NCChatWnd's body; the
    TArray<NChatBalloon> code at 0x10041b20 and 0x100424e0 is the template's).
=============================================================================*/
#include "nwindow/NCConsole.h"
#include "nwindow/NCAbnormalStatusWnd.h"
#include "nwindow/NCActionWnd.h"
#include "nwindow/NCBoardWnd.h"
#include "nwindow/NCBroadcastGMFenceWnd.h"
#include "nwindow/NCBroadcastGMWnd.h"
#include "nwindow/NCBroadcastObserverGMMsgWnd.h"
#include "nwindow/NCBroadcastObserverSpecialMsgWnd.h"
#include "nwindow/NCBroadcastObserverWnd.h"
#include "nwindow/NCChatWnd.h"
#include "nwindow/NCDeliverWnd.h"
#include "nwindow/NCDetailStatusWnd.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NCEditBox.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCGMWnd.h"
#include "nwindow/NCHtmlViewer.h"
#include "nwindow/NCInventoryWnd.h"
#include "nwindow/NCMagicSkillWnd.h"
#include "nwindow/NCMainWnd.h"
// C3: class gone, no counterpart: #include "NCMatchOptionWnd.h"
#include "nwindow/NCMatchWnd.h"
#include "nwindow/NCMenuWnd.h"
#include "nwindow/NCMinimapWnd.h"
#include "nwindow/NCNPCHtmlViewer.h"
#include "nwindow/NCOptionWnd.h"
#include "nwindow/NCPartyWnd.h"
#include "nwindow/NCPetitionWnd.h"
#include "nwindow/NCPetWnd.h"
#include "nwindow/NCPrivateBuyWnd.h"
#include "nwindow/NCPrivateShopWnd.h"
#include "nwindow/NCRadarWnd.h"
#include "nwindow/NCShopWnd.h"
#include "nwindow/NCShortCutWnd.h"
#if NW_CUSTOM
#include "nwindow/NCustomShortCuts.h"
#include "nwindow/NCustomInventory.h"
#endif
#include "nwindow/NCSiegeInfoMainWnd.h"
#include "nwindow/NCStatusWnd.h"
#include "nwindow/NCStoreWnd.h"
#include "nwindow/NCSummonedWnd.h"
#include "nwindow/NCSystemMenuWnd.h"
#include "nwindow/NCTestHtmlViewer.h"
#include "nwindow/NCTradeWnd.h"
#include "nwindow/NCTrainWnd.h"
#include "nwindow/NCTutorialViewerWnd.h"
#include "nwindow/NCVIPShopWnd.h"
#include "nwindow/NCCalcWnd.h"
#include "nwindow/NCEmpowerWnd.h"
#include "nwindow/NCGMSnoopManager.h"
#include "nwindow/NCGaraDialogBox.h"
#include "nwindow/NCHennaInfoWnd.h"
#include "nwindow/NCHennaListWnd.h"
#include "nwindow/NCMacroEditWnd.h"
#include "nwindow/NCMacroInfoWnd.h"
#include "nwindow/NCMacroListWnd.h"
#include "nwindow/NCManorShopWnd.h"
#include "nwindow/NCObserverWnd.h"
#include "nwindow/NCPetitionBeginWnd.h"
#include "nwindow/NCRecipeBookWnd.h"
#include "nwindow/NCRecipeBuyListWnd.h"
#include "nwindow/NCRecipeBuyManufactureWnd.h"
#include "nwindow/NCRecipeManufactureWnd.h"
#include "nwindow/NCRecipeShopWnd.h"
// C3: class gone, no counterpart: #include "NCRecipeTreeWnd.h"
#include "nwindow/NCXMasSealWnd.h"
#include "nwindow/NTownMapWnd.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/MessengerWnd.h"
#include "nwindow/NCCommandChannelWnd.h"
#include "nwindow/NCEffectButton.h"
#include "nwindow/NCHeroTowerContentWnd.h"
#include "nwindow/NCHeroTowerWnd.h"
#include "nwindow/NCItemEnchantWnd.h"
#include "nwindow/NCManorCropInfoChangeWnd.h"
#include "nwindow/NCManorCropInfoSettingWnd.h"
#include "nwindow/NCManorCropSellChangeWnd.h"
#include "nwindow/NCManorCropSellWnd.h"
#include "nwindow/NCManorInfoWnd.h"
#include "nwindow/NCManorSeedInfoChangeWnd.h"
#include "nwindow/NCManorSeedInfoSettingWnd.h"
#include "nwindow/NCMapCoverWnd.h"
#include "nwindow/NCOlympiadControlWnd.h"
#include "nwindow/NCOlympiadPlayerWnd.h"
#include "nwindow/NCOlympiadTargetWnd.h"
#include "nwindow/NCPartyRoomMakingWnd.h"
#include "nwindow/NCPartyRoomWnd.h"
#include "nwindow/NCQuestListInfoWnd.h"
#include "nwindow/NCRestartMenuWnd.h"
#include "nwindow/NCSSQMainBoard.h"
#include "nwindow/NCShopPreviewWnd.h"
#include "nwindow/NCZoneTitleWnd.h"
#include "nwindow/NCTabControl.h"
#include "nwindow/NCPledgeWnd.h"
#include "nwindow/NConsoleWndExt.h"
#include "nwindow/NClasses.gen.h"

#include <stdio.h>
#include <wchar.h>
#include <imm.h>

// OnCreate numbers the windows it makes, in order (NCWnd +0x78; name ours)
#if NW_HOOK_MODE
#define GNextWndNumber NW_GLOBAL(INT, 0x102C6BC0)   // C4 (name ours): the next window number, NCWnd::OnCreate's too
#else
extern INT GNextWndNumber;
#endif

// interface sounds of the toggles (names ours)
enum { SOUND_WindowOpen = 5, SOUND_WindowClose = 6, SOUND_Minimap = 8, SOUND_Petition = 10 };
// C3 added eight (the table of NConsoleWnd::PlayInterfaceSound has 19 names now)
enum
{
    SOUND_CharOpen = 11, SOUND_CharClose = 12, SOUND_InventoryOpen = 13, SOUND_InventoryClose = 14,
    SOUND_MapOpen = 15, SOUND_MapClose = 16, SOUND_SystemOpen = 17, SOUND_SystemClose = 18
};

// the player's petition window (NConsoleWnd::PetitionWnd is declared as an NCWnd)
static FORCEINLINE NCPetitionWnd* PlayerPetitionWnd()
{
    return static_cast<NCPetitionWnd*>(GConsole->PetitionWnd);
}

// the console's options window
#if 0  // C2: does not compile against the C2 headers (db/demote.csv)
static FORCEINLINE NCWnd* OptionWnd()
{
    return GConsole->OptionWnd;
}
#endif

// NCStoreWnd::GetMode; the stock code calls it on the NPC shop too, whose mode is at the same
// place (+0xBC)
static FORCEINLINE INT StoreWndMode(NCWnd* Store)
{
    return static_cast<NCStoreWnd*>(Store)->GetMode();
}

/*-----------------------------------------------------------------------------
    Construction.
-----------------------------------------------------------------------------*/

// 0x1005fef0 - the default constructor (the class's InternalConstructor): the windows OnCreate makes are cleared (the stock
// stores them one by one; the object comes zeroed all the same), two fields are 1 and the balloon list is empty. Which windows
// are cleared differs from the second constructor's: +0x1A4, +0x1A8, +0x24C and +0x250 here, +0x238 and +0x254 there
NCConsole::NCConsole()
{
    ChatWnd = NULL;
    NpcHtmlWnd = NULL;
    ::nw::Field<INT>(this, 0xFC) = 0;   // _UnknownFC
    HelpWnd = NULL;
    Owner = NULL;
    InventoryWnd = NULL;
    TargetStatusWnd = NULL;
    PlayerStatusWnd = NULL;
    AbnormalStatusWnd = NULL;
    NewWndE4 = NULL;
    MenuWnd = NULL;
    SystemMenuWnd = NULL;
    NewWndF8 = NULL;
    NewWndFC = NULL;
    NewWnd100 = NULL;
    ShortCutWnd = NULL;
    TrainWnd = NULL;
    MatchWnd = NULL;
    MapCoverWnd = NULL;
    BoardWnd = NULL;
    PetWnd = NULL;
    SummonedWnd = NULL;
    NewWnd134 = NULL;
    RecipeGaraTreeWnd = NULL;
    NewWnd13C = NULL;
    RecipeShopWnd = NULL;
    NewWnd144 = NULL;
    NewWnd14C = NULL;
    NewWnd150 = NULL;
    MessengerWnd = NULL;
    GMWnd = NULL;
    GMInventoryWnd = NULL;
    GMMagicSkillWnd = NULL;
    GMDetailStatusWnd = NULL;
    GMQuestWnd = NULL;
    GMStoreWnd = NULL;
    BroadcastGMWnd = NULL;
    BroadcastGMFenceWnd = NULL;
    BroadcastObserverWnd = NULL;
    NewWnd194 = NULL;
    NewWnd198 = NULL;
    BlackTex = NULL;
    BalloonTex = NULL;
    UnknownA8 = 1;
    Unknown150 = 1;
    BalloonTimer = 0;
    Ticks = 0;
    Unknown170[0] = 0;
    Unknown170[1] = 0;
    Unknown170[2] = 0;
    Unknown170[3] = 0;
}
NW_HOOK_CTOR(0x1005fef0, NCConsole)

// 0x100600e0 - the constructor NConsoleWnd uses: the same with the console as owner (+0x104, stored last)
NCConsole::NCConsole(NConsoleWnd* InOwner)
{
    ChatWnd = NULL;
    NpcHtmlWnd = NULL;
    ::nw::Field<INT>(this, 0xFC) = 0;   // _UnknownFC
    HelpWnd = NULL;
    InventoryWnd = NULL;
    TargetStatusWnd = NULL;
    PlayerStatusWnd = NULL;
    AbnormalStatusWnd = NULL;
    NewWndE4 = NULL;
    MenuWnd = NULL;
    SystemMenuWnd = NULL;
    NewWndF8 = NULL;
    NewWndFC = NULL;
    NewWnd100 = NULL;
    ShortCutWnd = NULL;
    TrainWnd = NULL;
    MatchWnd = NULL;
    MapCoverWnd = NULL;
    BoardWnd = NULL;
    PetWnd = NULL;
    SummonedWnd = NULL;
    NewWnd134 = NULL;
    RecipeGaraTreeWnd = NULL;
    NewWnd13C = NULL;
    NewWnd14C = NULL;
    NewWnd150 = NULL;
    MessengerWnd = NULL;
    GMWnd = NULL;
    GMInventoryWnd = NULL;
    GMMagicSkillWnd = NULL;
    GMDetailStatusWnd = NULL;
    GMQuestWnd = NULL;
    GMStoreWnd = NULL;
    BroadcastGMWnd = NULL;
    BroadcastGMFenceWnd = NULL;
    BroadcastObserverWnd = NULL;
    TestNpcHtmlWnd = NULL;
    NewWnd19C = NULL;
    BlackTex = NULL;
    BalloonTex = NULL;
    UnknownA8 = 1;
    Unknown150 = 1;
    BalloonTimer = 0;
    Ticks = 0;
    Unknown170[0] = 0;
    Unknown170[1] = 0;
    Unknown170[2] = 0;
    Unknown170[3] = 0;
    Owner = InOwner;
}
NW_HOOK_CTOR_ARGS(0x100600e0, NCConsole, (NConsoleWnd* A), (A))


/*-----------------------------------------------------------------------------
    Messages.
-----------------------------------------------------------------------------*/

// 0x1009e590 - the stock body is shared by every Exit slot (folded identical code, `appRequestExit(0); return 1`): NCAuthWnd::OnCancel
// hooks it, once; this is the same body as plain code (it was a NW_STOCK jump into that hook), so it carries no hook of its own
INT NCConsole::OnQuit()
{
    appRequestExit(0);
    return 1;
}

// 0x1005c4b0 (vslot 25) - NID_CONSOLE_QUIT quits, NWM_MOVETO walks (C2: no IsEnabled test first, as C1 had)
INT NCConsole::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_CONSOLE_QUIT)
            return OnQuit() != 0;
    }
    else if (Msg == (UINT)NWM_MOVETO)
        return OnMoveTo(wParam, lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x1005c4b0, NCConsole, WindowProc)

// 0x1005b610 - the player walks to the FVector wParam points at
INT NCConsole::OnMoveTo(WPARAM wParam, LPARAM lParam)
{
    Owner->MoveToLocation(*(FVector*)wParam, 0);
    return 1;
}
NW_HOOK_METHOD(0x1005b610, NCConsole, OnMoveTo)

// 0x1005f070 (vslot 36) - each second the balloons older than 5 seconds go (and off their
// pawns)
INT NCConsole::OnTimer(INT TimerID)
{
    if (TimerID == NTIMER_BALLOONS)
    {
        Ticks++;
        for (INT i = 0; i < Balloons.Num();)
        {
            NChatBalloon& Balloon = Balloons(i);
            if (Balloon.Tick + 5 < Ticks)
            {
                User* Speaker = GConsole->FindUserByID(Balloon.ObjectID);
                if (Speaker && Speaker->Pawn)
                {
                    Speaker->Pawn->ChatBalloon = NULL;
                    Speaker->Pawn->Unknown5C++;
                }
                if (Balloon.Text)
                {
                    delete[] Balloon.Text;
                    Balloon.Text = NULL;
                }
                Balloons.Remove(i, 1);
            }
            else
                i++;
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x1005f070, NCConsole, OnTimer)

// 0x1005b570 (vslot 62) - draws nothing: the clip of the desktop's rectangle (C3: floats, truncated) is set and
// released
INT NCConsole::OnPaint(NCGDevice* Device)
{
    guard(NCConsole::OnPaint);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1005b570, NCConsole, OnPaint)

/*-----------------------------------------------------------------------------
    Creation.
-----------------------------------------------------------------------------*/

// A child hidden before its creation message (inlined in the stock OnCreate for eight
// windows: the shortcut bar, the trainer, the party matching and its options, the minimap, the
// GM window and the two test pages; name ours): registered, hidden, then attached (entry,
// screen position, WM_CREATE)
static FORCEINLINE void CreateHiddenChild(NCWnd* Parent, NCWnd* Child, DWORD ChildStyle)
{
    NWndEntry* E = Parent->AddChildWindow(Child, ChildStyle);
    Child->HideWindow();
    Child->AttachEntry(E);
}

// A child hidden before its creation message, the attach inline (the shortcut bar, the trainer, the matching
// windows, the minimap, the town map and the observer window: name ours): registered, hidden, then attached
static FORCEINLINE void CreateChildHidden(NCWnd* Parent, NCWnd* Child, DWORD ChildStyle)
{
    NWndEntry* E = Parent->AddChildWindow(Child, ChildStyle);
    Child->HideWindow();
    Child->Entry = E;
    if (E->Parent)
    {
        NCWnd* Owner = E->Parent->Wnd;
        Child->ScreenX += Owner->ScreenX;
        Child->ScreenY += Owner->ScreenY;
    }
    Child->WindowProc(WM_CREATE, 0, 0);
}

// A child attached by the out-of-line AttachEntry (where the stock stopped inlining it: the manor shop, the GM
// windows and the last ones; name ours)
static FORCEINLINE void CreateChildCall(NCWnd* Parent, NCWnd* Child, DWORD ChildStyle)
{
    NWndEntry* E = Parent->AddChildWindow(Child, ChildStyle);
    Child->AttachEntry(E);
}

// The inline half of CreateChild after the entry came back (stock-inlined in OnCreate for most windows): the entry,
// the place made absolute and WM_CREATE
static FORCEINLINE void AttachInline(NCWnd* Parent, NCWnd* Child, NWndEntry* E)
{
    Child->Entry = E;
    if (E->Parent)
    {
        NCWnd* Owner = E->Parent->Wnd;
        Child->ScreenX += Owner->ScreenX;
        Child->ScreenY += Owner->ScreenY;
    }
    Child->WindowProc(WM_CREATE, 0, 0);
}

// NConsoleWnd::PetitionWnd, the player's petition window: the stock reads it at +0x3C00 (NConsoleWnd.h has it there, asserted, since
// wave B; it used to say +0x3C04, which is PetitionFeedBackWnd)
static FORCEINLINE NCWnd* ConsolePetitionWnd()
{
    return GConsole->PetitionWnd;
}

// 0x10060240 (vslot 37) - every in-game window, in the order of the stock (the generated text of the stock's straight-line
// code, one block per window: `new` of the class (its allocation and its constructor), keep the pointer at the desktop's field,
// place it (the positions are floats of the desktop's own rectangle or of the screen, truncated; C4: with the three pin
// arguments), register it as a child and attach it (the inline half of CreateChild, or the out-of-line AttachEntry), then
// what that window gets: hidden, numbered for WindowsInfo.ini, titled (C4: by the id of the system string, slot 56)). The
// fields are the offsets of NCConsole.h. The two test windows (NPC html and html, GIsOnlyL2Console / GIsTestHtml) and the
// textures and the timer end it
INT NCConsole::OnCreate()
{
    NCWnd* W = NULL;
    NWndEntry* E = NULL;
    NRectF Local;
    Local.X = 0;
    Local.Y = 0;
    Local.W = 0;
    Local.H = 0;
    GetLocalRect(&Local);

    // class 0x10310e10
    W = new NCZoneTitleWnd;
    nw::Field<NCWnd*>(this, 0x258) = W;   // ZoneTitleWnd
    W->SetPosition(0, 0, 0, 0, 0, 0, 3);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x258), 0xA);
    AttachInline(this, W, E);   // the inline attach

    // class 0x102d1c70
    W = new NCMainWnd;
    nw::Field<NCWnd*>(this, 0xE8) = W;   // MainWnd
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 0x100, 0x191, 1, 0, 5);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0xE8), 0x3002);
    AttachInline(this, W, E);   // the inline attach
    nw::Field<NCWnd*>(this, 0xE8)->Unknown78 = ++GNextWndNumber;

    // class 0x10302e70
    W = new NCSSQMainBoard;
    nw::Field<NCWnd*>(this, 0x25C) = W;   // SSQMainBoard
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 0x100, 0x191, 1, 0, 5);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x25C), 0x1002);
    AttachInline(this, W, E);   // the inline attach
    nw::Field<NCWnd*>(this, 0x25C)->HideWindow();
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x25C), 56, 0x372);   // SetTitle by id
    nw::Field<NCWnd*>(this, 0x25C)->Unknown78 = ++GNextWndNumber;

    // class 0x1030eed0
    W = new NCCalcWnd;
    nw::Field<NCWnd*>(this, 0xEC) = W;   // CalcWnd
    W->SetPosition((INT)(((Local.W - 256.0f) * 0.5f)), (INT)(((Local.H - 211.0f) * 0.5f)), 0x100, 0xD3, 1, 2, 2);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0xEC), 0x1002);
    AttachInline(this, W, E);   // the inline attach
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0xEC), 56, 0x29D);   // SetTitle by id
    nw::Field<NCWnd*>(this, 0xEC)->Unknown78 = ++GNextWndNumber;

    // class 0x102f5958
    W = new NCGMSnoopManager;
    nw::Field<NCWnd*>(this, 0x248) = W;   // GMSnoopManager
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x248), 0x6);
    AttachInline(this, W, E);   // the inline attach

    // class 0x102f68f0
    W = new NCGMPetitionWnd;
    nw::Field<NCWnd*>(this, 0x10C) = W;   // GMPetitionWnd
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 0x136, 0x19C, 1, 0, 5);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x10C), 0x2);
    AttachInline(this, W, E);   // the inline attach
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x10C), 56, 0x1D8);   // SetTitle by id
    nw::Field<NCWnd*>(this, 0x10C)->Unknown78 = ++GNextWndNumber;

    // class 0x102f63b8
    W = new NCPetitionBeginWnd;
    nw::Field<NCWnd*>(this, 0x110) = W;   // PetitionBeginWnd
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 0x100, 0x191, 1, 0, 5);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x110), 0x2);
    AttachInline(this, W, E);   // the inline attach
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x110), 56, 0x1D6);   // SetTitle by id
    nw::Field<NCWnd*>(this, 0x110)->Unknown78 = ++GNextWndNumber;

    // class 0x102d4638
    W = new NCEmpowerWnd;
    nw::Field<NCWnd*>(this, 0xF0) = W;   // EmpowerWnd
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 0x100, 0x191, 1, 0, 5);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0xF0), 0x1002);
    AttachInline(this, W, E);   // the inline attach
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0xF0), 56, 0x29C);   // SetTitle by id
    nw::Field<NCWnd*>(this, 0xF0)->Unknown78 = ++GNextWndNumber;

    // class 0x102c9448
    W = new NCNPCHtmlViewer;
    nw::Field<NCWnd*>(this, 0xF8) = W;   // NpcHtmlWnd
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 0x136, 0x191, 1, 0, 5);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0xF8), 0x1002);
    AttachInline(this, W, E);   // the inline attach
    nw::Field<NCWnd*>(this, 0xF8)->HideWindow();
    nw::Field<NCWnd*>(this, 0xF8)->Unknown78 = ++GNextWndNumber;
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0xF8), 56, 0x1BC);   // SetTitle by id
    nw::Field<UTexture*>(nw::Field<NCWnd*>(this, 0xF8), 0xEC) = FindTexture(TEXT("L2UI_ch3.NpcWnd.Npc1_back"), 1);

    // class 0x102c9448
    W = new NCNPCHtmlViewer;
    nw::Field<NCWnd*>(this, 0xFC) = W;
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 0x136, 0x191, 1, 0, 5);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0xFC), 0x1002);
    AttachInline(this, W, E);   // the inline attach
    nw::Field<NCWnd*>(this, 0xFC)->HideWindow();
    nw::Field<NCWnd*>(this, 0xFC)->Unknown78 = ++GNextWndNumber;
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0xFC), 56, 0x1BC);   // SetTitle by id
    nw::Field<UTexture*>(nw::Field<NCWnd*>(this, 0xFC), 0xEC) = FindTexture(TEXT("L2UI_ch3.NpcWnd.Npc1_back"), 1);

    // class 0x102c8498
    W = new NCBoardWnd;
    nw::Field<NCWnd*>(this, 0x174) = W;   // BoardWnd
    W->SetPosition((INT)(((Width * 0.5f) - 323.0f)), (INT)(((Height * 0.5f) - 265.0f)), 0x286, 0x212, 1, 2, 2);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x174), 0x3002);
    AttachInline(this, W, E);   // the inline attach
    nw::Field<NCWnd*>(this, 0x174)->HideWindow();
    nw::Field<NCWnd*>(this, 0x174)->Unknown78 = ++GNextWndNumber;
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x174), 56, 0x183);   // SetTitle by id

    // class 0x102dbdd0
    W = new NCInventoryWnd;
    nw::Field<NCWnd*>(this, 0x120) = W;   // InventoryWnd
#if NW_CUSTOM
    // custom: the wide inventory (NCustomInventory.h)
    W->SetPosition((INT)(((Local.W - (FLOAT)NCustomInventory::Width) - 46.0f)), (INT)(((Local.H * 0.5f) - 252.0f)),
                   NCustomInventory::Width, NCustomInventory::Height, 1, 1, 5);
#else
    W->SetPosition((INT)(((Local.W - 256.0f) - 46.0f)), (INT)(((Local.H * 0.5f) - 252.0f)), 0x100, 0x191, 1, 1, 5);
#endif
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x120), 0x3002);
    AttachInline(this, W, E);   // the inline attach
    nw::Field<NCWnd*>(this, 0x120)->Unknown78 = ++GNextWndNumber;
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x120), 56, 0x8A);   // SetTitle by id

    // class 0x102db368
    W = new NCItemEnchantWnd;
    nw::Field<NCWnd*>(this, 0x268) = W;
    W->SetPosition((INT)(((Local.W - 256.0f) - 46.0f)), (INT)(((Local.H * 0.5f) - 252.0f)), 0x100, 0x191, 1, 1, 5);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x268), 0x5002);
    AttachInline(this, W, E);   // the inline attach
    nw::Field<NCWnd*>(this, 0x268)->Unknown78 = ++GNextWndNumber;

    // class 0x1030a608
    W = new NCPlayerStatusWnd(0x64);
    nw::Field<NCWnd*>(this, 0x128) = W;   // PlayerStatusWnd
    W->SetPosition(0, 0, 0xAC, 0x54, 1, 0, 3);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x128), 0x85002);
    AttachInline(this, W, E);   // the inline attach
    nw::Field<NCWnd*>(this, 0x128)->Unknown78 = ++GNextWndNumber;

    // class 0x10309138
    W = new NCTargetStatusWnd(0x96);
    nw::Field<NCWnd*>(this, 0x124) = W;   // TargetStatusWnd
    W->SetPosition((INT)(((Local.W * 0.5f) - 86.0f)), 0, 0xAC, 0x2E, 1, 2, 3);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x124), 0x81022);
    AttachInline(this, W, E);   // the inline attach
    nw::Field<NCWnd*>(this, 0x124)->Unknown78 = ++GNextWndNumber;

    // class 0x1030b5a0
    W = new NCAbnormalStatusWnd(0x18);
    nw::Field<NCWnd*>(this, 0x12C) = W;   // AbnormalStatusWnd
    W->SetPosition(0xAC, 0, 0xFC, 0x32, 1, 0, 3);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x12C), 0x85022);
    AttachInline(this, W, E);   // the inline attach
    nw::Field<NCWnd*>(this, 0x12C)->Unknown78 = ++GNextWndNumber;

    // class 0x10309668
    W = new NCPartyWnd;
    nw::Field<NCWnd*>(this, 0x114) = W;   // PartyWnd
    W->SetPosition(0, 0x98, 0xAC, 0x2E, 1, 0, 3);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x114), 0x81002);
    AttachInline(this, W, E);   // the inline attach
    nw::Field<NCWnd*>(this, 0x114)->Unknown78 = ++GNextWndNumber;

    // class 0x10309ba0
    W = new NCPetStatusWnd(0x64);
    nw::Field<NCWnd*>(this, 0x118) = W;   // PetStatusWnd
    W->SetPosition(0, 0x4C, 0xAC, 0x2E, 1, 0, 3);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x118), 0x5002);
    AttachInline(this, W, E);   // the inline attach
    nw::Field<NCWnd*>(this, 0x118)->Unknown78 = ++GNextWndNumber;

    // class 0x1030b070
    W = new NCSummonedStatusWnd(0x64);
    nw::Field<NCWnd*>(this, 0x11C) = W;   // SummonedStatusWnd
    W->SetPosition(0, 0x4C, 0xAC, 0x2E, 1, 0, 3);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x11C), 0x5002);
    AttachInline(this, W, E);   // the inline attach
    nw::Field<NCWnd*>(this, 0x11C)->Unknown78 = ++GNextWndNumber;

    // class 0x1030c538
    W = new NCStoreWnd;
    nw::Field<NCWnd*>(this, 0x130) = W;   // StoreWnd
    W->SetPosition(0x50, 0x9C, 0x100, 0x191, 1, 0, 3);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x130), 0x1002);
    AttachInline(this, W, E);   // the inline attach
    nw::Field<NCWnd*>(this, 0x130)->Unknown78 = ++GNextWndNumber;
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x130), 56, 0x83);   // SetTitle by id

    // class 0x102d4100
    W = new NCDeliverWnd;
    nw::Field<NCWnd*>(this, 0x230) = W;   // DeliverWnd
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 0x100, 0x191, 1, 0, 5);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x230), 0x1002);
    AttachInline(this, W, E);   // the inline attach
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x230), 56, 0x22D);   // SetTitle by id
    nw::Field<NCWnd*>(this, 0x230)->Unknown78 = ++GNextWndNumber;

    // class 0x102d3698
    W = new NCSelectDeliverWnd;
    nw::Field<NCWnd*>(this, 0x234) = W;   // SelectDeliverWnd
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 0x100, 0xC9, 1, 0, 5);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x234), 0x1002);
    AttachInline(this, W, E);   // the inline attach
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x234), 56, 0x22E);   // SetTitle by id
    nw::Field<NCWnd*>(this, 0x234)->Unknown78 = ++GNextWndNumber;

    // class 0x10306ce0
    W = new NCShopWnd;
    nw::Field<NCWnd*>(this, 0x134) = W;   // ShopWnd
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 0x100, 0x191, 1, 0, 5);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x134), 0x1002);
    AttachInline(this, W, E);   // the inline attach
    nw::Field<NCWnd*>(this, 0x134)->Unknown78 = ++GNextWndNumber;
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x134), 56, 0x88);   // SetTitle by id

    // class 0x10306278
    W = new NCShopPreviewWnd;
    nw::Field<NCWnd*>(this, 0x138) = W;   // ShopPreviewWnd
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 0x100, 0x191, 1, 0, 5);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x138), 0x1002);
    AttachInline(this, W, E);   // the inline attach
    nw::Field<NCWnd*>(this, 0x138)->Unknown78 = ++GNextWndNumber;
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x138), 56, 0x34F);   // SetTitle by id

    // class 0x102df710
    W = new NCVIPShopWnd;
    nw::Field<NCWnd*>(this, 0x140) = W;   // VIPShopWnd
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 0x200, 0x191, 1, 0, 5);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x140), 0x1002);
    AttachInline(this, W, E);   // the inline attach
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x140), 56, 0x88);   // SetTitle by id
    nw::Field<NCWnd*>(this, 0x140)->Unknown78 = ++GNextWndNumber;

    // class 0x1030cfa0
    W = new NCTradeWnd;
    nw::Field<NCWnd*>(this, 0x14C) = W;   // TradeWnd
    W->SetPosition((INT)(((Local.W - 256.0f) - 46.0f)), (INT)(((Local.H * 0.5f) - 252.0f)), 0x100, 0x191, 1, 1, 5);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x14C), 0x1002);
    nw::Field<NCWnd*>(this, 0x14C)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x14C)->Unknown78 = ++GNextWndNumber;
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x14C), 56, 0x1BD);   // SetTitle by id

    // class 0x102e73c8
    W = new NCMenuWnd;
    nw::Field<NCWnd*>(this, 0x144) = W;   // MenuWnd
#if NW_CUSTOM
    // custom: wider for the two icons NCustomShortCuts::CreateMenuIcons adds
    W->SetPosition((INT)((Width - 173.0f - NCustomShortCuts::MenuIcons * NCustomShortCuts::MenuIconStep)), (INT)((Height - 46.0f)),
                   0xAD + NCustomShortCuts::MenuIcons * NCustomShortCuts::MenuIconStep, 0x2E, 1, 1, 4);
#else
    W->SetPosition((INT)((Width - 173.0f)), (INT)((Height - 46.0f)), 0xAD, 0x2E, 1, 1, 4);
#endif
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x144), 0x85002);
    nw::Field<NCWnd*>(this, 0x144)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x144)->Unknown78 = ++GNextWndNumber;

    // class 0x102e7e30
    W = new NCSystemMenuWnd;
    nw::Field<NCWnd*>(this, 0x148) = W;   // SystemMenuWnd
    W->SetPosition((INT)((Width - 172.0f)), (INT)(((Height - 295.0f) - 46.0f)), 0xAC, 0x127, 1, 1, 4);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x148), 0x3002);
    nw::Field<NCWnd*>(this, 0x148)->AttachEntry(E);
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x148), 56, 0x1BE);   // SetTitle by id

    // class 0x102c9448
    W = new NCNPCHtmlViewer;
    nw::Field<NCWnd*>(this, 0x100) = W;   // HelpWnd
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 0x136, 0x137, 1, 0, 5);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x100), 0x1002);
    nw::Field<NCWnd*>(this, 0x100)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x100)->HideWindow();
    nw::Field<NCWnd*>(this, 0x100)->Unknown78 = ++GNextWndNumber;
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x100), 56, 0x91);   // SetTitle by id
    nw::Field<UTexture*>(nw::Field<NCWnd*>(this, 0x100), 0xEC) = FindTexture(TEXT("L2UI_ch3.NpcWnd.Npc2_back"), 1);

    // class 0x102ba690
    W = new NCGaraDialogBox;
    nw::Field<NCWnd*>(this, 0x240) = W;   // MainDialogBox
    W->SetPosition((INT)(((Width * 0.5f) - 128.0f)), (INT)(((Height * 0.5f) - 54.0f)), 0x100, 0x6C, 1, 2, 2);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x240), 0x22);
    nw::Field<NCWnd*>(this, 0x240)->AttachEntry(E);
    static_cast<NCGaraDialogBox*>(nw::Field<NCWnd*>(this, 0x240))->SetButtonTexts(0x8C, 0x8D);
    nw::Field<NCWnd*>(this, 0x240)->Unknown78 = ++GNextWndNumber;

    // class 0x102ba690
    W = new NCGaraDialogBox;
    nw::Field<NCWnd*>(this, 0x244) = W;   // SecondDialogBox
    W->SetPosition(0x15B, (INT)(((Height - 108.0f) - 2.0f)), 0x100, 0x6C, 1, 0, 4);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x244), 0x22);
    nw::Field<NCWnd*>(this, 0x244)->AttachEntry(E);
    static_cast<NCGaraDialogBox*>(nw::Field<NCWnd*>(this, 0x244))->SetButtonTexts(0xB8, 0xB9);
    nw::Field<NCWnd*>(this, 0x244)->Unknown78 = ++GNextWndNumber;

    // class 0x102e2b08
    W = new NCMacroListWnd;
    nw::Field<NCWnd*>(this, 0x150) = W;   // NewWndF8
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 0x100, 0x191, 1, 0, 5);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x150), 0x3022);
    nw::Field<NCWnd*>(this, 0x150)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x150)->HideWindow();
    nw::Field<NCWnd*>(this, 0x150)->Unknown78 = ++GNextWndNumber;
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x150), 56, 0x2C7);   // SetTitle by id

    // class 0x102e25d0
    W = new NCMacroInfoWnd;
    nw::Field<NCWnd*>(this, 0x158) = W;   // NewWnd100
    W->SetPosition(0x14, (INT)((((Height * 0.5f) - 252.0f) + 80.0f)), 0x100, 0xA1, 1, 0, 5);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x158), 0x2);
    nw::Field<NCWnd*>(this, 0x158)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x158)->HideWindow();
    nw::Field<NCWnd*>(this, 0x158)->Unknown78 = ++GNextWndNumber;
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x158), 56, 0x291);   // SetTitle by id

    // class 0x102e1b70
    W = new NCMacroEditWnd;
    nw::Field<NCWnd*>(this, 0x154) = W;   // NewWndFC
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 0x100, 0x191, 1, 0, 5);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x154), 0x3002);
    nw::Field<NCWnd*>(this, 0x154)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x154)->HideWindow();
    nw::Field<NCWnd*>(this, 0x154)->Unknown78 = ++GNextWndNumber;
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x154), 56, 0x2C7);   // SetTitle by id

    // class 0x10307210
    W = new NCShortCutWnd(0xC, 0xA, 0x20);
    nw::Field<NCWnd*>(this, 0x15C) = W;   // ShortCutWnd
    W->SetPosition((INT)((Width - 46.0f)), (INT)(((Height * 0.5f) - 252.0f)), 0x2E, 0x1F8, 1, 1, 2);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x15C), 0x85022);
    nw::Field<NCWnd*>(this, 0x15C)->HideWindow();
    nw::Field<NCWnd*>(this, 0x15C)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x15C)->Unknown78 = ++GNextWndNumber;

    // class 0x1030e470
    W = new NCTrainWnd;
    nw::Field<NCWnd*>(this, 0x160) = W;   // TrainWnd
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 0x100, 0x191, 1, 0, 5);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x160), 0x1002);
    nw::Field<NCWnd*>(this, 0x160)->HideWindow();
    nw::Field<NCWnd*>(this, 0x160)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x160)->Unknown78 = ++GNextWndNumber;

    // class 0x102e4f70
    W = new NCMatchWnd;
    nw::Field<NCWnd*>(this, 0x164) = W;   // MatchWnd
    W->SetPosition(0, 0x4C, 0x258, 0x1AF, 1, 0, 3);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x164), 0x3002);
    nw::Field<NCWnd*>(this, 0x164)->HideWindow();
    nw::Field<NCWnd*>(this, 0x164)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x164)->Unknown78 = ++GNextWndNumber;

    // class 0x102e5f00
    W = new NCPartyRoomMakingWnd;
    nw::Field<NCWnd*>(this, 0x168) = W;   // PartyRoomMakingWnd
    W->SetPosition((INT)(((Width * 0.5f) - 178.0f)), (INT)(((Height * 0.5f) - 70.0f)), 0x164, 0x8D, 1, 2, 2);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x168), 0x10002);
    nw::Field<NCWnd*>(this, 0x168)->HideWindow();
    nw::Field<NCWnd*>(this, 0x168)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x168)->Unknown78 = ++GNextWndNumber;

    // class 0x102e6e90
    W = new NCPartyRoomWnd;
    nw::Field<NCWnd*>(this, 0x16C) = W;   // PartyRoomWnd
    W->SetPosition(0, 0x4C, 0x258, 0x1AF, 1, 0, 3);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x16C), 0x3002);
    nw::Field<NCWnd*>(this, 0x16C)->HideWindow();
    nw::Field<NCWnd*>(this, 0x16C)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x16C)->Unknown78 = ++GNextWndNumber;

    // class 0x102e8dd0
    W = new NCMapCoverWnd;
    nw::Field<NCWnd*>(this, 0x170) = W;   // MapCoverWnd
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 0x14E, 0x17F, 1, 0, 5);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x170), 0x3002);
    nw::Field<NCWnd*>(this, 0x170)->HideWindow();
    nw::Field<NCWnd*>(this, 0x170)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x170)->Unknown78 = ++GNextWndNumber;
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x170), 56, 0x1BF);   // SetTitle by id

    // class 0x102e93c0
    W = new NTownMapWnd;
    nw::Field<NCWnd*>(this, 0x24C) = W;   // NewWnd194
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 0x14E, 0x162, 1, 0, 5);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x24C), 0x1002);
    nw::Field<NCWnd*>(this, 0x24C)->HideWindow();
    nw::Field<NCWnd*>(this, 0x24C)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x24C)->Unknown78 = ++GNextWndNumber;
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x24C), 56, 0x1BF);   // SetTitle by id

    // class 0x102e8898
    W = new NCObserverWnd;
    nw::Field<NCWnd*>(this, 0x250) = W;   // NewWnd198
    W->SetPosition((INT)(((Local.W * 0.5f) - 51.0f)), 0, 0x66, 0x20, 1, 2, 3);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x250), 0x5002);
    nw::Field<NCWnd*>(this, 0x250)->HideWindow();
    nw::Field<NCWnd*>(this, 0x250)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x250)->Unknown78 = ++GNextWndNumber;
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x250), 56, 0x298);   // SetTitle by id

    // class 0x102f7db8
    W = new NCPrivateShopWnd;
    nw::Field<NCWnd*>(this, 0x228) = W;   // PrivateShopWnd
    W->SetPosition((INT)(((Local.W - 256.0f) - 46.0f)), (INT)(((Local.H * 0.5f) - 252.0f)), 0x100, 0x191, 1, 1, 5);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x228), 0x1002);
    nw::Field<NCWnd*>(this, 0x228)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x228)->HideWindow();
    nw::Field<NCWnd*>(this, 0x228)->Unknown78 = ++GNextWndNumber;
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x228), 56, 0x254);   // SetTitle by id

    // class 0x102f6e20
    W = new NCPrivateBuyWnd;
    nw::Field<NCWnd*>(this, 0x22C) = W;   // PrivateBuyWnd
    W->SetPosition((INT)(((Local.W - 256.0f) - 46.0f)), (INT)(((Local.H * 0.5f) - 252.0f)), 0x100, 0x191, 1, 1, 5);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x22C), 0x1002);
    nw::Field<NCWnd*>(this, 0x22C)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x22C)->HideWindow();
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x22C), 56, 0x255);   // SetTitle by id

    // class 0x1030f408
    W = new NCTutorialViewerWnd;
    nw::Field<NCWnd*>(this, 0x178) = W;   // TutorialWnd
    W->SetPosition((INT)(((Local.W - 310.0f) - 20.0f)), 0x14, 0x136, 0x191, 1, 1, 3);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x178), 0x5002);
    nw::Field<NCWnd*>(this, 0x178)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x178)->HideWindow();
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x178), 56, 0x1C0);   // SetTitle by id
    nw::Field<NCWnd*>(this, 0x178)->Unknown78 = ++GNextWndNumber;

    // class 0x102b6d50
    W = new NCEffectButton(0, TEXT("L2UI_ch3.ConsoleBtn.TutorialBtn"), TEXT("L2UI_ch3.ConsoleBtn.TutorialBtn_down"), TEXT("L2UI_ch3.ConsoleBtn.TutorialBtn_light"), 0x1F4, 0);
    nw::Field<NCWnd*>(this, 0x17C) = W;   // TutorialQuestionWnd
    W->SetPosition(5, (INT)((((Local.H - 187.0f) - 32.0f) - 5.0f)), 0x20, 0x20, 1, 0, 4);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x17C), 0x80002);
    nw::Field<NCWnd*>(this, 0x17C)->AttachEntry(E);

    // class 0x102b6d50
    W = new NCEffectButton(1, TEXT("L2UI_ch3.ConsoleBtn.QuestBtn"), TEXT("L2UI_ch3.ConsoleBtn.QuestBtn_down"), TEXT("L2UI_ch3.ConsoleBtn.QuestBtn_light"), 0x1F4, 0);
    nw::Field<NCWnd*>(this, 0x180) = W;   // QuestBtn
    W->SetPosition(0x2A, (INT)((((Local.H - 187.0f) - 32.0f) - 5.0f)), 0x20, 0x20, 1, 0, 4);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x180), 0x80002);
    nw::Field<NCWnd*>(this, 0x180)->AttachEntry(E);

    // class 0x102b6d50
    W = new NCEffectButton(2, TEXT("L2UI_ch3.ConsoleBtn.MailBtn"), TEXT("L2UI_ch3.ConsoleBtn.MailBtn_down"), TEXT("L2UI_ch3.ConsoleBtn.MailBtn_light"), 0x1F4, 0);
    nw::Field<NCWnd*>(this, 0x184) = W;   // MailBtn
    W->SetPosition(0x4F, (INT)((((Local.H - 187.0f) - 32.0f) - 5.0f)), 0x20, 0x20, 1, 0, 4);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x184), 0x80002);
    nw::Field<NCWnd*>(this, 0x184)->AttachEntry(E);

    // class 0x102fb1b8
    W = new NCRadarWnd;
    nw::Field<NCWnd*>(this, 0x190) = W;   // RadarWnd
    W->SetPosition((INT)((Local.W - 102.0f)), 0, 0x66, 0x66, 1, 1, 3);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x190), 0x8000A);
    nw::Field<NCWnd*>(this, 0x190)->AttachEntry(E);

    // class 0x102f34f0
    W = new NCPetWnd;
    nw::Field<NCWnd*>(this, 0x188) = W;   // PetWnd
    W->SetPosition(0xAC, 0x4C, 0x100, 0xE5, 1, 0, 3);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x188), 0x3002);
    nw::Field<NCWnd*>(this, 0x188)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x188)->HideWindow();
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x188), 56, 0x1EE);   // SetTitle by id
    nw::Field<NCWnd*>(this, 0x188)->Unknown78 = ++GNextWndNumber;

    // class 0x102f2560
    W = new NCSummonedWnd;
    nw::Field<NCWnd*>(this, 0x18C) = W;   // SummonedWnd
    W->SetPosition(0xAC, 0x4C, 0x100, 0xC9, 1, 0, 3);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x18C), 0x3002);
    nw::Field<NCWnd*>(this, 0x18C)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x18C)->HideWindow();
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x18C), 56, 0x1EF);   // SetTitle by id
    nw::Field<NCWnd*>(this, 0x18C)->Unknown78 = ++GNextWndNumber;

    // class 0x102d1208
    W = new NCChatWnd;
    nw::Field<NCWnd*>(this, 0xF4) = W;   // ChatWnd
    W->SetPosition(0, (INT)((Height - 187.0f)), 0x15C, 0xBB, 1, 0, 4);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0xF4), 0x8100A);
    nw::Field<NCWnd*>(this, 0xF4)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0xF4)->Unknown78 = ++GNextWndNumber;

    // class 0x102ff008
    W = new NCRecipeBookWnd;
    nw::Field<NCWnd*>(this, 0x198) = W;   // NewWnd134
    W->SetPosition((INT)((((Local.W - 256.0f) - 46.0f) - 256.0f)), (INT)(((Local.H * 0.5f) - 252.0f)), 0x100, 0x191, 1, 1, 5);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x198), 0x1022);
    nw::Field<NCWnd*>(this, 0x198)->AttachEntry(E);
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x198), 56, 0x297);   // SetTitle by id
    nw::Field<NCWnd*>(this, 0x198)->Unknown78 = ++GNextWndNumber;

    // class 0x102fead8
    W = new NCRecipeGaraTreeWnd;
    nw::Field<NCWnd*>(this, 0x19C) = W;   // RecipeGaraTreeWnd
    W->SetPosition((INT)((((Local.W - 256.0f) - 46.0f) - 256.0f)), (INT)(((Local.H * 0.5f) - 252.0f)), 0x100, 0x191, 1, 1, 5);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x19C), 0x1002);
    nw::Field<NCWnd*>(this, 0x19C)->AttachEntry(E);
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x19C), 56, 0x296);   // SetTitle by id
    nw::Field<NCWnd*>(this, 0x19C)->Unknown78 = ++GNextWndNumber;

    // class 0x102fe078
    W = new NCRecipeManufactureWnd;
    nw::Field<NCWnd*>(this, 0x1A0) = W;   // NewWnd13C
    W->SetPosition((INT)((((Local.W - 256.0f) - 46.0f) - 256.0f)), (INT)(((Local.H * 0.5f) - 252.0f)), 0x100, 0x191, 1, 1, 5);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1A0), 0x1002);
    nw::Field<NCWnd*>(this, 0x1A0)->AttachEntry(E);
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x1A0), 56, 0x297);   // SetTitle by id
    nw::Field<NCWnd*>(this, 0x1A0)->Unknown78 = ++GNextWndNumber;

    // class 0x102fd0e0
    W = new NCRecipeShopWnd;
    nw::Field<NCWnd*>(this, 0x1A4) = W;   // RecipeShopWnd
    W->SetPosition((INT)(((Local.W - 256.0f) - 46.0f)), (INT)(((Local.H * 0.5f) - 252.0f)), 0x100, 0x191, 1, 1, 5);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1A4), 0x1002);
    nw::Field<NCWnd*>(this, 0x1A4)->AttachEntry(E);
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x1A4), 56, 0x297);   // SetTitle by id
    nw::Field<NCWnd*>(this, 0x1A4)->Unknown78 = ++GNextWndNumber;

    // class 0x102fbc20
    W = new NCRecipeBuyListWnd;
    nw::Field<NCWnd*>(this, 0x1A8) = W;   // NewWnd144
    W->SetPosition((INT)(((Local.W - 256.0f) - 46.0f)), (INT)(((Local.H * 0.5f) - 252.0f)), 0x100, 0x191, 1, 1, 5);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1A8), 0x1002);
    nw::Field<NCWnd*>(this, 0x1A8)->AttachEntry(E);
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x1A8), 56, 0x297);   // SetTitle by id
    nw::Field<NCWnd*>(this, 0x1A8)->Unknown78 = ++GNextWndNumber;

    // class 0x102fb6e8
    W = new NCRecipeBuyManufactureWnd;
    nw::Field<NCWnd*>(this, 0x1AC) = W;   // RecipeBuyManufactureWnd
    W->SetPosition((INT)(((Local.W - 256.0f) - 46.0f)), (INT)(((Local.H * 0.5f) - 252.0f)), 0x100, 0x191, 1, 1, 5);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1AC), 0x1002);
    nw::Field<NCWnd*>(this, 0x1AC)->AttachEntry(E);
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x1AC), 56, 0x297);   // SetTitle by id
    nw::Field<NCWnd*>(this, 0x1AC)->Unknown78 = ++GNextWndNumber;

    // class 0x102d9e98
    W = new NCHennaListWnd;
    nw::Field<NCWnd*>(this, 0x1B0) = W;   // NewWnd14C
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 0x100, 0x191, 1, 0, 5);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1B0), 0x1002);
    nw::Field<NCWnd*>(this, 0x1B0)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x1B0)->Unknown78 = ++GNextWndNumber;

    // class 0x102d9438
    W = new NCHennaInfoWnd;
    nw::Field<NCWnd*>(this, 0x1B4) = W;   // NewWnd150
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 0x100, 0x191, 1, 0, 5);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1B4), 0x1002);
    nw::Field<NCWnd*>(this, 0x1B4)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x1B4)->Unknown78 = ++GNextWndNumber;

    // class 0x103231a0
    W = new ::MessengerWnd;   // (the desktop has a field of this name)
    nw::Field<NCWnd*>(this, 0x1B8) = W;   // MessengerWnd
    W->SetPosition(0x180, 0xB4, 0x100, 0x191, 1, 0, 3);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1B8), 0x3002);
    nw::Field<NCWnd*>(this, 0x1B8)->AttachEntry(E);
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x1B8), 56, 0x3E7);   // SetTitle by id
    nw::Field<NCWnd*>(this, 0x1B8)->Unknown78 = ++GNextWndNumber;

    // class 0x102e4a40
    W = new NCManorShopWnd;
    nw::Field<NCWnd*>(this, 0x13C) = W;   // NewWndE4
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 0x100, 0x191, 1, 0, 5);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x13C), 0x1002);
    nw::Field<NCWnd*>(this, 0x13C)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x13C)->Unknown78 = ++GNextWndNumber;
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x13C), 56, 0x2E2);   // SetTitle by id

    // class 0x102d6aa0
    W = new NCGMWnd;
    nw::Field<NCWnd*>(this, 0x1BC) = W;   // GMWnd
    W->SetPosition((INT)(((Width - 256.0f) - 1.0f)), (INT)((((Height - 26.0f) - 1.0f) - 300.0f)), 0x100, 0x114, 1, 1, 4);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1BC), 0x2);
    nw::Field<NCWnd*>(this, 0x1BC)->HideWindow();
    nw::Field<NCWnd*>(this, 0x1BC)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x1BC)->Unknown78 = ++GNextWndNumber;

    // class 0x102d89d0
    W = new NCGMInventoryWnd;
    nw::Field<NCWnd*>(this, 0x1C0) = W;   // GMInventoryWnd
    W->SetPosition((INT)(((Local.W - 200.0f) - 1.0f)), 0x64, 0x100, 0x191, 1, 1, 3);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1C0), 0x2);
    nw::Field<NCWnd*>(this, 0x1C0)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x1C0)->Unknown78 = ++GNextWndNumber;

    // class 0x102d6038
    W = new NCGMMagicSkillWnd;
    nw::Field<NCWnd*>(this, 0x1C4) = W;   // GMMagicSkillWnd
    W->SetPosition((INT)(((Local.W - 208.0f) - 1.0f)), 0x64, 0x100, 0x163, 1, 1, 5);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1C4), 0x2);
    nw::Field<NCWnd*>(this, 0x1C4)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x1C4)->Unknown78 = ++GNextWndNumber;

    // class 0x102d7f68
    W = new NCGMDetailStatusWnd;
    nw::Field<NCWnd*>(this, 0x1C8) = W;   // GMDetailStatusWnd
    W->SetPosition(0, 0x64, 0x100, 0x163, 1, 0, 3);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1C8), 0x2);
    nw::Field<NCWnd*>(this, 0x1C8)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x1C8)->Unknown78 = ++GNextWndNumber;

    // class 0x102d6570
    W = new NCGMQuestWnd;
    nw::Field<NCWnd*>(this, 0x1CC) = W;   // GMQuestWnd
    W->SetPosition((INT)(((Local.W - 208.0f) - 1.0f)), 0x64, 0x100, 0x163, 1, 1, 3);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1CC), 0x22);
    nw::Field<NCWnd*>(this, 0x1CC)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x1CC)->Unknown78 = ++GNextWndNumber;

    // class 0x102d5b08
    W = new NCGMStoreWnd;
    nw::Field<NCWnd*>(this, 0x1D0) = W;   // GMStoreWnd
    W->SetPosition((INT)(((Local.W - 208.0f) - 1.0f)), 0x64, 0x100, 0x191, 1, 1, 3);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1D0), 0x22);
    nw::Field<NCWnd*>(this, 0x1D0)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x1D0)->Unknown78 = ++GNextWndNumber;

    // class 0x102d50a8
    W = new NCGMActionWnd;
    nw::Field<NCWnd*>(this, 0x1D4) = W;   // GMActionWnd
    W->SetPosition((INT)(((Local.W - 208.0f) - 1.0f)), 0x64, 0x100, 0x163, 1, 1, 3);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1D4), 0x2);
    nw::Field<NCWnd*>(this, 0x1D4)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x1D4)->Unknown78 = ++GNextWndNumber;

    // class 0x10307740
    W = new NCSiegeInfoMainWnd;
    nw::Field<NCWnd*>(this, 0x194) = W;   // SiegeInfoWnd
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 0x100, 0x191, 1, 1, 5);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x194), 0x1002);
    nw::Field<NCWnd*>(this, 0x194)->AttachEntry(E);
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x194), 56, 0x251);   // SetTitle by id
    nw::Field<NCWnd*>(this, 0x194)->Unknown78 = ++GNextWndNumber;

    // class 0x103108d8
    W = new NCXMasSealWnd;
    nw::Field<NCWnd*>(this, 0x254) = W;   // NewWnd19C
    W->SetPosition((INT)(((Local.W * 0.5f) - 167.0f)), (INT)(((Local.H * 0.5f) - 178.0f)), 0x14E, 0x162, 1, 0, 3);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x254), 0x1002);
    nw::Field<NCWnd*>(this, 0x254)->HideWindow();
    nw::Field<NCWnd*>(this, 0x254)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x254)->Unknown78 = ++GNextWndNumber;
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x254), 56, 2);   // SetTitle by id

    // class 0x10311880
    W = new NCBroadcastGMWnd;   // 0x101285F0
    nw::Field<NCWnd*>(this, 0x1D8) = W;   // BroadcastGMWnd
    W->SetPosition((INT)(((Width * 0.5f) - 350.0f)), (INT)(((Height * 0.5f) - 250.0f)), 0x2BC, 0x1F4, 1, 2, 2);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1D8), 0x2002);
    nw::Field<NCWnd*>(this, 0x1D8)->HideWindow();
    nw::Field<NCWnd*>(this, 0x1D8)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x1D8)->Unknown78 = ++GNextWndNumber;
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x1D8), 56, 0x450);   // SetTitle by id

    // class 0x10311348
    W = new NCBroadcastGMFenceWnd;   // 0x101229B0
    nw::Field<NCWnd*>(this, 0x1DC) = W;   // BroadcastGMFenceWnd
    W->SetPosition((INT)(((Width * 0.5f) - 250.0f)), (INT)(((Height * 0.5f) - 150.0f)), 0x1F4, 0xAA, 1, 1, 2);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1DC), 0x2);
    nw::Field<NCWnd*>(this, 0x1DC)->HideWindow();
    nw::Field<NCWnd*>(this, 0x1DC)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x1DC)->Unknown78 = ++GNextWndNumber;
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x1DC), 56, 0x451);   // SetTitle by id

    // class 0x10314760
    W = new NCBroadcastObserverWnd;   // 0x1012F5D0
    nw::Field<NCWnd*>(this, 0x1E0) = W;   // BroadcastObserverWnd
    W->SetPosition(0, 0, (INT)(Width), (INT)(Height), 1, 0, 3);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1E0), 0xA);
    nw::Field<NCWnd*>(this, 0x1E0)->HideWindow();
    nw::Field<NCWnd*>(this, 0x1E0)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x1E0)->Unknown78 = ++GNextWndNumber;
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x1E0), 56, 0x452);   // SetTitle by id

    // class 0x103122e8
    W = new NCBroadcastObserverGMMsgWnd;   // 0x101299B0
    nw::Field<NCWnd*>(this, 0x1E4) = W;   // BroadcastObserverGMMsgWnd
    if (GConsole->UIMode == 1)
        W->SetPosition((INT)(((Width * 0.5f) - 360.0f)), 0x342, 0x2D0, 0x58, 1, 2, 3);
    else
        W->SetPosition((INT)(((Width * 0.5f) - 360.0f)), (INT)(((Height * 0.5f) + 100.0f)), 0x2D0, 0x58, 1, 2, 5);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1E4), 0xA);
    nw::Field<NCWnd*>(this, 0x1E4)->AttachEntry(E);

    // class 0x10313cf8
    W = new NCBroadcastObserverSpecialMsgWnd;   // 0x1012CD70
    nw::Field<NCWnd*>(this, 0x1E8) = W;   // BroadcastObserverSpecialMsgWnd
    W->SetPosition((INT)(((Width * 0.5f) - 360.0f)), 0x186, 0x2D0, 0x140, 1, 2, 3);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1E8), 0xA);
    nw::Field<NCWnd*>(this, 0x1E8)->AttachEntry(E);

    // class 0x102f8d50
    W = new NCQuestListInfoWnd;
    nw::Field<NCWnd*>(this, 0x260) = W;
    W->SetPosition(0, (INT)(((Height * 0.5f) - 252.0f)), 0x258, 0x146, 1, 0, 5);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x260), 0x1022);
    nw::Field<NCWnd*>(this, 0x260)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x260)->Unknown78 = ++GNextWndNumber;
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x260), 56, 0x4B4);   // SetTitle by id

    // class 0x10317b88
    W = new NCManorInfoWnd;
    nw::Field<NCWnd*>(this, 0x1EC) = W;
    W->SetPosition((INT)(((Width * 0.5f) - 300.0f)), (INT)(((Height * 0.5f) - 163.0f)), 0x258, 0x146, 1, 2, 2);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1EC), 0x2);
    nw::Field<NCWnd*>(this, 0x1EC)->HideWindow();
    nw::Field<NCWnd*>(this, 0x1EC)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x1EC)->Unknown78 = ++GNextWndNumber;
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x1EC), 56, 0x2E2);   // SetTitle by id

    // class 0x103185f8
    W = new NCManorSeedInfoSettingWnd;
    nw::Field<NCWnd*>(this, 0x1F0) = W;
    W->SetPosition((INT)(((Width * 0.5f) - 300.0f)), (INT)(((Height * 0.5f) - 163.0f)), 0x258, 0x146, 1, 2, 2);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1F0), 0x2);
    nw::Field<NCWnd*>(this, 0x1F0)->HideWindow();
    nw::Field<NCWnd*>(this, 0x1F0)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x1F0)->Unknown78 = ++GNextWndNumber;
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x1F0), 56, 0x470);   // SetTitle by id

    // class 0x103151d0
    W = new NCManorCropInfoSettingWnd;
    nw::Field<NCWnd*>(this, 0x1F4) = W;
    W->SetPosition((INT)(((Width * 0.5f) - 300.0f)), (INT)(((Height * 0.5f) - 163.0f)), 0x258, 0x146, 1, 2, 2);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1F4), 0x2);
    nw::Field<NCWnd*>(this, 0x1F4)->HideWindow();
    nw::Field<NCWnd*>(this, 0x1F4)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x1F4)->Unknown78 = ++GNextWndNumber;
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x1F4), 56, 0x479);   // SetTitle by id

    // class 0x10316170
    W = new NCManorCropSellWnd;
    nw::Field<NCWnd*>(this, 0x1F8) = W;
    W->SetPosition((INT)(((Width * 0.5f) - 300.0f)), (INT)(((Height * 0.5f) - 163.0f)), 0x258, 0x146, 1, 2, 2);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1F8), 0x2);
    nw::Field<NCWnd*>(this, 0x1F8)->HideWindow();
    nw::Field<NCWnd*>(this, 0x1F8)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x1F8)->Unknown78 = ++GNextWndNumber;
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x1F8), 56, 0x480);   // SetTitle by id

    // class 0x10315c38
    W = new NCManorCropSellChangeWnd;
    nw::Field<NCWnd*>(this, 0x1FC) = W;
    W->SetPosition((INT)(((Width * 0.5f) - 168.0f)), (INT)(((Height * 0.5f) - 153.0f)), 0x150, 0x132, 1, 2, 2);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x1FC), 0x2);
    nw::Field<NCWnd*>(this, 0x1FC)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x1FC)->Unknown78 = ++GNextWndNumber;
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x1FC), 56, 0x480);   // SetTitle by id
    nw::Field<NCWnd*>(this, 0x1FC)->HideWindow();

    // class 0x103180c0
    W = new NCManorSeedInfoChangeWnd;
    nw::Field<NCWnd*>(this, 0x200) = W;
    W->SetPosition((INT)(((Width * 0.5f) - 128.0f)), (INT)(((Height * 0.5f) - 100.0f)), 0x100, 0xC9, 1, 2, 2);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x200), 0x2);
    nw::Field<NCWnd*>(this, 0x200)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x200)->Unknown78 = ++GNextWndNumber;
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x200), 56, 0x470);   // SetTitle by id
    nw::Field<NCWnd*>(this, 0x200)->HideWindow();

    // class 0x10314c98
    W = new NCManorCropInfoChangeWnd;
    nw::Field<NCWnd*>(this, 0x204) = W;
    W->SetPosition((INT)(((Width * 0.5f) - 128.0f)), (INT)(((Height * 0.5f) - 100.0f)), 0x100, 0xC9, 1, 2, 2);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x204), 0x2);
    nw::Field<NCWnd*>(this, 0x204)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x204)->Unknown78 = ++GNextWndNumber;
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x204), 56, 0x479);   // SetTitle by id
    nw::Field<NCWnd*>(this, 0x204)->HideWindow();

    // class 0x102cf810
    W = new NCCommandChannelWnd;
    nw::Field<NCWnd*>(this, 0x264) = W;   // NewWnd264
    W->SetPosition(0xC8, 0xC8, 0x15C, 0xBB, 1, 0, 3);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x264), 0x4002);
    nw::Field<NCWnd*>(this, 0x264)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x264)->Unknown78 = ++GNextWndNumber;

    // class 0x103195a0
    W = new NCHeroTowerWnd;
    nw::Field<NCWnd*>(this, 0x208) = W;
    W->SetPosition((INT)(((Width * 0.5f) - 300.0f)), (INT)(((Height * 0.5f) - 163.0f)), 0x258, 0x146, 1, 2, 2);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x208), 0x2);
    nw::Field<NCWnd*>(this, 0x208)->HideWindow();
    nw::Field<NCWnd*>(this, 0x208)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x208)->Unknown78 = ++GNextWndNumber;
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x208), 56, 0x4C6);   // SetTitle by id

    // class 0x10319068
    W = new NCHeroTowerContentWnd;
    nw::Field<NCWnd*>(this, 0x20C) = W;
    W->SetPosition((INT)(((Width * 0.5f) - 155.0f)), (INT)(((Height * 0.5f) - 200.0f)), 0x136, 0x191, 1, 2, 2);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x20C), 0x2);
    nw::Field<NCWnd*>(this, 0x20C)->HideWindow();
    nw::Field<NCWnd*>(this, 0x20C)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x20C)->Unknown78 = ++GNextWndNumber;
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x20C), 56, 2);   // SetTitle by id

    // class 0x10319ad8
    W = new NCOlympiadControlWnd;
    nw::Field<NCWnd*>(this, 0x210) = W;
    W->SetPosition((INT)(((Width * 0.5f) - 38.0f)), 0, 0x66, 0x38, 1, 2, 3);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x210), 0x5002);
    nw::Field<NCWnd*>(this, 0x210)->HideWindow();
    nw::Field<NCWnd*>(this, 0x210)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x210)->Unknown78 = ++GNextWndNumber;
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x210), 56, 2);   // SetTitle by id

    // class 0x1031a010
    W = new NCOlympiadPlayerWnd;
    nw::Field<NCWnd*>(this, 0x214) = W;
    W->SetPosition(0, 0, 0x15C, 0x2E, 1, 0, 3);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x214), 0x5002);
    nw::Field<NCWnd*>(this, 0x214)->HideWindow();
    nw::Field<NCWnd*>(this, 0x214)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x214)->Unknown78 = ++GNextWndNumber;
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x214), 56, 2);   // SetTitle by id

    // class 0x1031a010
    W = new NCOlympiadPlayerWnd;
    nw::Field<NCWnd*>(this, 0x218) = W;
    W->SetPosition((INT)((Width - 348.0f)), 0, 0x15C, 0x2E, 1, 1, 3);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x218), 0x5002);
    nw::Field<NCWnd*>(this, 0x218)->HideWindow();
    nw::Field<NCWnd*>(this, 0x218)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x218)->Unknown78 = ++GNextWndNumber;
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x218), 56, 2);   // SetTitle by id

    // class 0x1030b5a0
    W = new NCAbnormalStatusWnd(0x18);
    nw::Field<NCWnd*>(this, 0x21C) = W;
    W->SetPosition(0, 0x2F, 0xFC, 0x32, 1, 0, 3);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x21C), 0x5002);
    nw::Field<NCWnd*>(this, 0x21C)->HideWindow();
    nw::Field<NCWnd*>(this, 0x21C)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x21C)->Unknown78 = ++GNextWndNumber;
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x21C), 56, 2);   // SetTitle by id

    // class 0x1030b5a0
    W = new NCAbnormalStatusWnd(0x18);
    nw::Field<NCWnd*>(this, 0x220) = W;
    W->SetPosition((INT)((Width - 348.0f)), 0x2F, 0xFC, 0x32, 1, 1, 3);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x220), 0x5002);
    nw::Field<NCWnd*>(this, 0x220)->HideWindow();
    nw::Field<NCWnd*>(this, 0x220)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x220)->Unknown78 = ++GNextWndNumber;
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x220), 56, 2);   // SetTitle by id

    // class 0x1031a548
    W = new NCOlympiadTargetWnd;
    nw::Field<NCWnd*>(this, 0x224) = W;
    W->SetPosition((INT)(((Width * 0.5f) - 86.0f)), (INT)(((Height * 0.5f) - 23.0f)), 0xAC, 0x2E, 1, 2, 2);
    E = AddChildWindow(nw::Field<NCWnd*>(this, 0x224), 0x5002);
    nw::Field<NCWnd*>(this, 0x224)->HideWindow();
    nw::Field<NCWnd*>(this, 0x224)->AttachEntry(E);
    nw::Field<NCWnd*>(this, 0x224)->Unknown78 = ++GNextWndNumber;
    ::nw::VirtualCall<void>(nw::Field<NCWnd*>(this, 0x224), 56, 2);   // SetTitle by id

    if (!GIsOnlyL2Console)
    {
        // class 0x102e7900
        W = new NCRestartMenuWnd;
        nw::Field<NCWnd*>(this, 0x26C) = W;
        W->SetPosition((INT)(((Width * 0.5f) - 64.0f)), (INT)(((Height * 0.5f) - 64.0f)), 0x80, 0x80, 1, 2, 2);
        E = AddChildWindow(nw::Field<NCWnd*>(this, 0x26C), 0x2);
        nw::Field<NCWnd*>(this, 0x26C)->AttachEntry(E);
        nw::Field<NCWnd*>(this, 0x26C)->HideWindow();
    }
    if (GIsOnlyL2Console)
    {
        // class 0x102c9448
        W = new NCNPCHtmlViewer;
        nw::Field<NCWnd*>(this, 0x238) = W;   // TestNpcHtmlWnd
        W->SetPosition(0x3C, 0x14, 0x136, 0x191, 0, 0, 3);
        E = AddChildWindow(nw::Field<NCWnd*>(this, 0x238), 0x1022);
        nw::Field<NCWnd*>(this, 0x238)->HideWindow();
        nw::Field<NCWnd*>(this, 0x238)->AttachEntry(E);
        static_cast<NCNPCHtmlViewer*>(nw::Field<NCWnd*>(this, 0x238))->HtmlViewer->LoadHtmlFile((TCHAR*)TEXT("test.html"));
    }
    if (GIsTestHtml)
    {
        // class 0x102c9980
        W = new NCTestHtmlViewer;
        nw::Field<NCWnd*>(this, 0x23C) = W;   // TestHtmlWnd
        W->SetPosition((INT)((Local.W * 0.5f)), (INT)((Local.H * 0.5f)), 0xD0, 0x19C, 0, 0, 3);
        E = AddChildWindow(nw::Field<NCWnd*>(this, 0x23C), 0x22);
        nw::Field<NCWnd*>(this, 0x23C)->HideWindow();
        nw::Field<NCWnd*>(this, 0x23C)->AttachEntry(E);
    }
    nw::Field<UTexture*>(this, 0x274) = FindTexture(TEXT("NWindow.BlackTexture"), 1);
    nw::Field<UTexture*>(this, 0x278) = FindTexture(TEXT("NWindow.chat"), 1);
    nw::Field<INT>(this, 0x288) = SetTimer(0xFA0D05, 1000, NULL);
#if NW_CUSTOM
    NCustomShortCuts::Create(this);   // custom: the extra shortcut bars
#endif
    return 1;
}
NW_HOOK_METHOD(0x10060240, NCConsole, OnCreate)

// a child window goes (repeated inline in the stock OnDestroy)
template <class T> static FORCEINLINE void DestroyChild(NCWnd* Parent, T*& Child)
{
    if (Child)
    {
        Parent->RemoveChildWindow(Child->Entry);
        delete Child;
        Child = NULL;
    }
}

// 0x1005df20 (vslot 38) - the balloons and the windows the desktop made, then the timer. C4: the order of the stock (by
// offset below), two pairs of windows in a loop (+0x214 and +0x21C), and of the broadcast windows only the GM's, the
// observer's ones and the others the stock lists; the test windows only when their flags are set. Names of the offsets:
// NCConsole.h
#define DESTROY_AT(Off) DestroyChild(this, nw::Field<NCWnd*>(this, Off))
INT NCConsole::OnDestroy()
{
    for (INT i = 0; i < Balloons.Num(); i++)
        if (Balloons(i).Text)
            delete[] Balloons(i).Text;
    Balloons.Empty();
#if NW_CUSTOM
    NCustomShortCuts::Destroy(this);   // custom: the extra shortcut bars, before the main bar
#endif
    DESTROY_AT(0xf4); DESTROY_AT(0xf8); DESTROY_AT(0xfc); DESTROY_AT(0x100); DESTROY_AT(0x10c); DESTROY_AT(0x174); DESTROY_AT(0x150); DESTROY_AT(0x158);
    DESTROY_AT(0x154); DESTROY_AT(0x15c); DESTROY_AT(0x120); DESTROY_AT(0x124); DESTROY_AT(0x128); DESTROY_AT(0x12c); DESTROY_AT(0x130); DESTROY_AT(0x134);
    DESTROY_AT(0x138); DESTROY_AT(0x140); DESTROY_AT(0x144); DESTROY_AT(0x148); DESTROY_AT(0x14c); DESTROY_AT(0x240); DESTROY_AT(0x244); DESTROY_AT(0x160);
    DESTROY_AT(0x164); DESTROY_AT(0x168); DESTROY_AT(0x16c); DESTROY_AT(0x170); DESTROY_AT(0x24c); DESTROY_AT(0x188); DESTROY_AT(0x18c); DESTROY_AT(0x198);
    DESTROY_AT(0x19c); DESTROY_AT(0x1a0); DESTROY_AT(0x1a4); DESTROY_AT(0x1a8); DESTROY_AT(0x1ac); DESTROY_AT(0x1b0); DESTROY_AT(0x1b4); DESTROY_AT(0x1b8);
    DESTROY_AT(0x1bc); DESTROY_AT(0x1c0); DESTROY_AT(0x1c4); DESTROY_AT(0x1c8); DESTROY_AT(0x1cc); DESTROY_AT(0x1d0); DESTROY_AT(0x1d4); DESTROY_AT(0x228);
    DESTROY_AT(0x22c); DESTROY_AT(0xe8); DESTROY_AT(0x25c); DESTROY_AT(0x114); DESTROY_AT(0x178); DESTROY_AT(0x190); DESTROY_AT(0x230); DESTROY_AT(0x234);
    DESTROY_AT(0x194); DESTROY_AT(0x248); DESTROY_AT(0x110); DESTROY_AT(0xec); DESTROY_AT(0xf0); DESTROY_AT(0x13c); DESTROY_AT(0x258); DESTROY_AT(0x254);
    DESTROY_AT(0x250); DESTROY_AT(0x11c); DESTROY_AT(0x118); DESTROY_AT(0x1d8); DESTROY_AT(0x1dc); DESTROY_AT(0x1e0); DESTROY_AT(0x1e4); DESTROY_AT(0x1e8);
    DESTROY_AT(0x1ec); DESTROY_AT(0x1f0); DESTROY_AT(0x1f4); DESTROY_AT(0x1f8); DESTROY_AT(0x1fc); DESTROY_AT(0x200); DESTROY_AT(0x204); DESTROY_AT(0x17c);
    DESTROY_AT(0x180); DESTROY_AT(0x184); DESTROY_AT(0x260); DESTROY_AT(0x264); DESTROY_AT(0x208); DESTROY_AT(0x20c); DESTROY_AT(0x210); DESTROY_AT(0x268);
    DESTROY_AT(0x26c);
    for (INT i = 0; i < 2; i++)
    {
        DESTROY_AT(0x214 + i * 4);
        DESTROY_AT(0x21C + i * 4);
    }
    DESTROY_AT(0x224);
    if (GIsOnlyL2Console)
        DESTROY_AT(0x238);
    if (GIsTestHtml)
        DESTROY_AT(0x23C);
    if (BalloonTimer)
    {
        KillTimer(BalloonTimer);
        BalloonTimer = 0;
    }
    NCWnd::OnDestroy();
    return 1;
}
#undef DESTROY_AT
NW_HOOK_METHOD(0x1005df20, NCConsole, OnDestroy)

// 0x1005c530 (vslot 48) - the desktop shows and every window goes back to its starting state: hidden except the shortcut
// bar, the chat, the player status, the menu, the radar and the windows the stock lists (C4: the order of the stock, the
// party room's icon pair, the buffs window by its counts, the restart menu by its flag); the messenger alone when the
// client starts as the messenger (GIsOnlyMSN)
void NCConsole::ShowWindow()
{
    NCWnd::ShowWindow();
    if (GIsOnlyMSN)
    {
        for (NWndListNode* Node = ChildHead; Node; Node = Node->Next)
            Node->Entry->Wnd->HideWindow();
        MessengerWnd->ShowWindow();
        return;
    }
    nw::Field<NCWnd*>(this, 0x194)->HideWindow();
    nw::Field<NCWnd*>(this, 0x230)->HideWindow();
    nw::Field<NCWnd*>(this, 0x234)->HideWindow();
    nw::Field<NCWnd*>(this, 0x10c)->HideWindow();
    if (ConsolePetitionWnd())
        ConsolePetitionWnd()->HideWindow();
    nw::Field<NCWnd*>(this, 0xe8)->HideWindow();
    nw::Field<NCWnd*>(this, 0x25c)->HideWindow();
    nw::Field<NCWnd*>(this, 0x174)->HideWindow();
    nw::Field<NCWnd*>(this, 0x150)->HideWindow();
    nw::Field<NCWnd*>(this, 0x158)->HideWindow();
    nw::Field<NCWnd*>(this, 0x154)->HideWindow();
    nw::Field<NCWnd*>(this, 0x100)->HideWindow();
    nw::Field<NCWnd*>(this, 0x15c)->ShowWindow();
    nw::Field<NCWnd*>(this, 0xf4)->ShowWindow();
    nw::Field<NCWnd*>(this, 0x120)->HideWindow();
    nw::Field<NCWnd*>(this, 0x124)->HideWindow();
    nw::Field<NCWnd*>(this, 0x128)->ShowWindow();
    nw::Field<NCWnd*>(this, 0x240)->HideWindow();
    nw::Field<NCWnd*>(this, 0x244)->HideWindow();
    nw::Field<NCWnd*>(this, 0x144)->ShowWindow();
    nw::Field<NCWnd*>(this, 0x148)->HideWindow();
    nw::Field<NCWnd*>(this, 0x130)->HideWindow();
    nw::Field<NCWnd*>(this, 0x134)->HideWindow();
    nw::Field<NCWnd*>(this, 0x138)->HideWindow();
    nw::Field<NCWnd*>(this, 0x140)->HideWindow();
    nw::Field<NCWnd*>(this, 0x14c)->HideWindow();
    nw::Field<NCWnd*>(this, 0xf8)->HideWindow();
    nw::Field<NCWnd*>(this, 0xfc)->HideWindow();
    nw::Field<NCWnd*>(this, 0x160)->HideWindow();
    nw::Field<NCWnd*>(this, 0x164)->HideWindow();
    nw::Field<NCWnd*>(this, 0x168)->HideWindow();
    nw::Field<NCWnd*>(this, 0x16c)->HideWindow();
    nw::Field<NCWnd*>(this, 0x170)->HideWindow();
    nw::Field<NCWnd*>(this, 0x178)->HideWindow();
    nw::Field<NCWnd*>(this, 0x190)->ShowWindow();
    nw::Field<NCWnd*>(this, 0x188)->HideWindow();
    nw::Field<NCWnd*>(this, 0x18c)->HideWindow();
    nw::Field<NCWnd*>(this, 0x198)->HideWindow();
    nw::Field<NCWnd*>(this, 0x19c)->HideWindow();
    nw::Field<NCWnd*>(this, 0x1a0)->HideWindow();
    nw::Field<NCWnd*>(this, 0x1a4)->HideWindow();
    nw::Field<NCWnd*>(this, 0x1a8)->HideWindow();
    nw::Field<NCWnd*>(this, 0x1ac)->HideWindow();
    nw::Field<NCWnd*>(this, 0x1b0)->HideWindow();
    nw::Field<NCWnd*>(this, 0x1b4)->HideWindow();
    nw::Field<NCWnd*>(this, 0x1b8)->HideWindow();
    static_cast<NMsnMessengerView*>(MessengerWnd)->HideAll();   // its two children are hidden
    nw::Field<NCWnd*>(this, 0x1bc)->HideWindow();
    nw::Field<NCWnd*>(this, 0x1c0)->HideWindow();
    nw::Field<NCWnd*>(this, 0x1c4)->HideWindow();
    nw::Field<NCWnd*>(this, 0x1c8)->HideWindow();
    nw::Field<NCWnd*>(this, 0x1cc)->HideWindow();
    nw::Field<NCWnd*>(this, 0x1d0)->HideWindow();
    nw::Field<NCWnd*>(this, 0x1d4)->HideWindow();
    nw::Field<NCWnd*>(this, 0x228)->HideWindow();
    nw::Field<NCWnd*>(this, 0x22c)->HideWindow();
    nw::Field<NCWnd*>(GConsole->GodWnd, 0xF4)->HideWindow();
    nw::Field<NCWnd*>(this, 0x110)->HideWindow();
    nw::Field<NCWnd*>(this, 0xec)->HideWindow();
    nw::Field<NCWnd*>(this, 0xf0)->HideWindow();
    nw::Field<NCWnd*>(this, 0x24c)->HideWindow();
    nw::Field<NCWnd*>(this, 0x13c)->HideWindow();
    nw::Field<NCWnd*>(this, 0x258)->HideWindow();
    nw::Field<NCWnd*>(this, 0x254)->HideWindow();
    nw::Field<NCWnd*>(this, 0x1d8)->HideWindow();
    nw::Field<NCWnd*>(this, 0x1dc)->HideWindow();
    nw::Field<NCWnd*>(this, 0x1e0)->HideWindow();
    nw::Field<NCWnd*>(this, 0x1e4)->HideWindow();
    nw::Field<NCWnd*>(this, 0x1e8)->HideWindow();
    nw::Field<NCWnd*>(this, 0x1ec)->HideWindow();
    nw::Field<NCWnd*>(this, 0x1f0)->HideWindow();
    nw::Field<NCWnd*>(this, 0x1f4)->HideWindow();
    nw::Field<NCWnd*>(this, 0x1f8)->HideWindow();
    nw::Field<NCWnd*>(this, 0x1fc)->HideWindow();
    nw::Field<NCWnd*>(this, 0x200)->HideWindow();
    nw::Field<NCWnd*>(this, 0x204)->HideWindow();
    nw::Field<NCWnd*>(this, 0x260)->HideWindow();
    nw::Field<NCWnd*>(this, 0x264)->HideWindow();
    nw::Field<NCWnd*>(this, 0x208)->HideWindow();
    nw::Field<NCWnd*>(this, 0x20c)->HideWindow();
    nw::Field<NCWnd*>(this, 0x224)->HideWindow();
    nw::Field<NCWnd*>(this, 0x210)->HideWindow();
    nw::Field<NCWnd*>(this, 0x180)->HideWindow();
    nw::Field<NCWnd*>(this, 0x184)->HideWindow();
    nw::Field<NCWnd*>(this, 0x17c)->HideWindow();
    nw::Field<NCWnd*>(this, 0x268)->HideWindow();
    for (INT i = 0; i < 2; i++)
    {
        nw::Field<NCWnd*>(this, 0x214 + i * 4)->HideWindow();
        nw::Field<NCWnd*>(this, 0x21C + i * 4)->HideWindow();
    }
    NCWnd* Buffs = nw::Field<NCWnd*>(this, 0x12C);
    if (nw::Field<INT>(Buffs, 0xEC) > 0 || nw::Field<INT>(Buffs, 0x100) != 0 || nw::Field<INT>(Buffs, 0xF8) > 0)
        Buffs->ShowWindow();
    else
        Buffs->HideWindow();
    NCWnd* Party = nw::Field<NCWnd*>(this, 0x114);
    if (Party && !nw::Field<INT>(Party, 0xF4))
        Party->HideWindow();
    NCWnd* Gm = nw::Field<NCWnd*>(this, 0x250);
    if (Gm)
    {
        APlayerController* Actor = GConsole->Engine->Client->Viewports(0)->Actor;
        ALineagePlayerController* Controller = CastToLineagePlayerController(Actor);
        if (Controller && !(nw::Field<BYTE>(Controller, 0xA30) & 1))
            Gm->HideWindow();
    }
    NCWnd* Restart = nw::Field<NCWnd*>(this, 0x26C);
    if (nw::Field<INT>(Restart, 0x114))
        Restart->ShowWindow();
    else
        Restart->HideWindow();
    NCWnd* Other = nw::Field<NCWnd*>(this, 0x264);
    if (nw::Field<INT>(Other, 0x100))
        Other->ShowWindow();
    if (GLanguageType == 0)
        ImmSetConversionStatus(ImmGetContext(GViewportHWnd), 1, 0);
    if (GIsTestHtml)
        nw::Field<NCWnd*>(this, 0x23C)->ShowWindow();
}
NW_HOOK_METHOD(0x1005c530, NCConsole, ShowWindow)

// stock body shared with NCChatWnd's (0x1003dab0)
void NCConsole::HideWindow()
{
    NCWnd::HideWindow();
}

/*-----------------------------------------------------------------------------
    Toggles and resets (the menus, the shortcut keys, the console).
-----------------------------------------------------------------------------*/

// (repeated inline in the stock CloseWindows)
static FORCEINLINE void HideIfShown(NCWnd* Wnd)
{
    if (Wnd->IsShowWindow())
        Wnd->HideWindow();
}

// 0x1005b640 - the private stores are left (and closed on the server when set up), the recipe shop and the trade
// cancelled, the party room left, the options and petition windows hidden, then the whole interface shown or hidden.
// C4: two flags (names ours) decide it, and there is no replay test any more: shown, it is hidden when bToggle or not
// bShow; hidden, it is shown when bToggle or bShow
void NCConsole::ToggleInterface(bool bToggle, bool bShow)
{
    if (PrivateShopWnd->IsShowWindow())
    {
        if (nw::Field<INT>(PrivateShopWnd, 0x11C) == 1)   // the store's mode
            GConsole->RequestPrivateStoreQuitSell();
        PrivateShopWnd->Reset();
    }
    if (PrivateBuyWnd->IsShowWindow())
    {
        if (nw::Field<INT>(PrivateBuyWnd, 0x114) == 1)    // the store's mode
            GConsole->RequestPrivateStoreQuitBuy();
        PrivateBuyWnd->Reset();
    }
    if (RecipeShopWnd->IsShowWindow())
    {
        GConsole->RequestRecipeShopManageQuit();   // the recipe shop manage request (0x10068100; the C4 slots moved by one against the old header names)
        static_cast<NCRecipeShopWnd*>(RecipeShopWnd)->ClearLists();
    }
    if (TradeWnd->IsShowWindow())
        GConsole->RequestTradeDone(0);
    if (PartyRoomWnd->IsShowWindow())
        static_cast<NCPartyRoomWnd*>(PartyRoomWnd)->RequestExit();
    NCWnd* Options = nw::Field<NCWnd*>(GConsole->GodWnd, 0xF4);   // the window at +0x3BE0 holds the options window
    if (Options)
        Options->HideWindow();
    if (ConsolePetitionWnd())
        ConsolePetitionWnd()->HideWindow();
    if (IsShowWindow())
    {
        if (bToggle || !bShow)
            HideWindow();
    }
    else if (bToggle || bShow)
        ShowWindow();
}
NW_HOOK_METHOD(0x1005b640, NCConsole, ToggleInterface)

// 0x1005b790 - the dialogs, shops, stores, menus and the radar close (C3: the shop preview, the board of the SSQ and
// the main window too; the petition window only when there is one)
void NCConsole::CloseWindows()
{
    HideIfShown(InventoryWnd);
    HideIfShown(ShopWnd);
    HideIfShown(ShopPreviewWnd);
    HideIfShown(VIPShopWnd);
    HideIfShown(StoreWnd);
    HideIfShown(TradeWnd);
    HideIfShown(SystemMenuWnd);
    HideIfShown(HelpWnd);
    HideIfShown(MainDialogBox);
    HideIfShown(SecondDialogBox);
    HideIfShown(RadarWnd);
    HideIfShown(PrivateShopWnd);
    HideIfShown(PrivateBuyWnd);
    HideIfShown(MainWnd);
    HideIfShown(SSQMainBoard);
    if (ConsolePetitionWnd())
        HideIfShown(ConsolePetitionWnd());
    HideIfShown(GMPetitionWnd);
    HideIfShown(DeliverWnd);
    HideIfShown(SelectDeliverWnd);
    HideIfShown(SiegeInfoWnd);
}
NW_HOOK_METHOD(0x1005b790, NCConsole, CloseWindows)



// 0x100bd7c0 - the map cover window's reset: its first child (+0xE8) is reset when there is one (a jump to the child's own
// 0x100bd3d0, which empties the three lists). A method of NCMapCoverWnd (its header is not this lane's): a stand-in type
// carries `this`
struct NMapCoverWndReset
{
    void Reset();
};
void NMapCoverWndReset::Reset()
{
    void* Child = ::nw::Field<void*>(this, 0xE8);
    if (Child)
        static_cast<NCMinimapWnd*>(Child)->ClearTargets();
}
NW_HOOK_METHOD(0x100bd7c0, NMapCoverWndReset, Reset)

// 0x1005ba30 - leaving the game: the dialogs, the chat, the shortcuts, the buffs, the party room, the clan page, the radar
// and both petition windows empty (C3: also the character window's last page; C4: the map cover and a window made at
// +0x264, new in C4). The calls on the windows whose classes are not declared are the stock's own bodies
void NCConsole::Reset()
{
    MainDialogBox->Reset();
    SecondDialogBox->Reset();
    ChatWnd->ClearAll();
    ShortCutWnd->ClearShortCuts();
    ((NCAbnormalStatusWndExt*)AbnormalStatusWnd)->ClearAll();
    MatchWnd->ClearList();
    nw::Field<NCActionWnd*>(MainWnd, 0xEC)->ResetPledgeWnd();
    nw::Field<NCPledgeWnd*>(MainWnd, 0xF0)->Reset();
    RadarWnd->ClearTargets();
    GMPetitionWnd->Clear();                                                 // NCPetitionWnd::Clear
    static_cast<NCPetitionWnd*>(ConsolePetitionWnd())->Clear();
    ((NCFlagWndExt*)nw::Field<void*>(MainWnd, 0xF8))->ClearFlags();
    ((NMapCoverWndReset*)MapCoverWnd)->Reset();
    static_cast<NCCommandChannelWnd*>(NewWnd264)->Reset();                  // the window at +0x264 (0x10051C20)
}
NW_HOOK_METHOD(0x1005ba30, NCConsole, Reset)

// C1 only: the C3 image has no function of this name (the petition windows are toggled inline by their callers); an
// inert body, so that a stray call does nothing instead of stopping the game
void NCConsole::TogglePetition()
{
}

// 0x1005baf0
void NCConsole::ToggleSystemMenu()
{
    if (SystemMenuWnd->IsShowWindow())
    {
        GConsole->PlayInterfaceSound(SOUND_SystemClose);    // C3: its own sounds (C2: window close / open)
        SystemMenuWnd->HideWindow();
    }
    else
    {
        GConsole->PlayInterfaceSound(SOUND_SystemOpen);
        SystemMenuWnd->ShowWindow();
        SystemMenuWnd->SetFocus();
    }
}
NW_HOOK_METHOD(0x1005baf0, NCConsole, ToggleSystemMenu)

// 0x1005bb50 - not while a shop (mode 1), a warehouse (mode 0), a trade or the freight is
// open; opening asks the server for the item list (its answer shows the window)
void NCConsole::ToggleInventory()
{
    if (ShopWnd->IsShowWindow() && StoreWndMode(ShopWnd) == 1)
        return;
    if (StoreWnd->IsShowWindow() && !StoreWndMode(StoreWnd))
        return;
    if (TradeWnd->IsShowWindow() || DeliverWnd->IsShowWindow())
        return;
    if (InventoryWnd->IsShowWindow())
    {
        GConsole->PlayInterfaceSound(SOUND_InventoryClose);    // C3: its own sounds (C2: window close / open)
        InventoryWnd->HideWindow();
    }
    else
    {
        GConsole->RequestItemList();
        GConsole->PlayInterfaceSound(SOUND_InventoryOpen);
    }
}
NW_HOOK_METHOD(0x1005bb50, NCConsole, ToggleInventory)

// 0x1005bc00 - the messenger window (+0x18C, new in C3): hidden when shown, else shown and focused (tail calls)
void NCConsole::ToggleChat()
{
    if (MessengerWnd->IsShowWindow())
        MessengerWnd->HideWindow();
    else
    {
        MessengerWnd->ShowWindow();
        MessengerWnd->SetFocus();
    }
}
NW_HOOK_METHOD(0x1005bc00, NCConsole, ToggleChat)

// 0x1005bc90 - the help page; bMacro (C2, name ours) opens the macro help instead
void NCConsole::ToggleHelp(UBOOL bMacro)
{
    if (HelpWnd->IsShowWindow())
    {
        HelpWnd->HideWindow();
        return;
    }
    HelpWnd->LoadFile((TCHAR*)(bMacro ? TEXT("..\\L2text\\help_macro.htm") : TEXT("..\\L2text\\help.htm")));
    HelpWnd->ShowWindow();
    HelpWnd->SetFocus();
}
NW_HOOK_METHOD(0x1005bc90, NCConsole, ToggleHelp)

// C1 only: no function of this name in the C3 image (the map paired it with 0x10004EA0, a one-argument setter folded
// with NCComboBox's: not NCConsole's); nobody calls it, an inert body so that a stray call cannot run that setter
void NCConsole::TogglePartyMatch()
{
}

// 0x1005bcf0 - C3: the map cover window (+0x170) takes a mode number into its child's +0x1AC, then is hidden when
// shown (the map close sound), else shown and focused (the same sound)
void NCConsole::ToggleMinimap(INT Mode)
{
    if (!MapCoverWnd)
        return;
    nw::Field<INT>(nw::Field<BYTE*>(MapCoverWnd, 0xE8), 0x1AC) = Mode;
    if (MapCoverWnd->IsShowWindow())
    {
        GConsole->PlayInterfaceSound(SOUND_MapClose);
        MapCoverWnd->HideWindow();
        return;
    }
    MapCoverWnd->ShowWindow();
    MapCoverWnd->SetFocus();
    GConsole->PlayInterfaceSound(SOUND_MapClose);
}
NW_HOOK_METHOD(0x1005bcf0, NCConsole, ToggleMinimap)

// C1 only: no function of this name in the C3 image (the matching windows were redone); an inert body
void NCConsole::ToggleMatchOption()
{
}

// 0x1005bd70 - opening asks the server for the board unless a command is pending (the flag of the board's html page,
// +0x17C); the tab control (+0xF8) goes back to its first tab (C3 SetTab takes two arguments)
void NCConsole::ToggleBoard()
{
    if (BoardWnd->IsShowWindow())
    {
        GConsole->PlayInterfaceSound(SOUND_WindowClose);
        BoardWnd->HideWindow();
        return;
    }
    if (nw::Field<INT>(nw::Field<BYTE*>(BoardWnd, 0xE8), 0x17C))
        return;
    L2ParamStack Params(10);
    GConsole->RequestShowBoard(Params);
    nw::Field<NCTabControl*>(BoardWnd, 0xF8)->SetTab(0, 0);   // NCTabControl::SetTab
    BoardWnd->HideWindow();
    GConsole->PlayInterfaceSound(SOUND_WindowOpen);
    nw::Field<INT>(nw::Field<BYTE*>(BoardWnd, 0xE8), 0x17C) = 1;
}
NW_HOOK_METHOD(0x1005bd70, NCConsole, ToggleBoard)

// 0x1005be70 - the macro window (+0xF8): shown and focused with the sound of opening a window (5), or hidden with the
// sound of closing it (6)
void NCConsole::ToggleMacroList()
{
    if (NewWndF8->IsShowWindow())
    {
        GConsole->PlayInterfaceSound(6);
        NewWndF8->HideWindow();
        return;
    }
    NewWndF8->ShowWindow();
    NewWndF8->SetFocus();
    GConsole->PlayInterfaceSound(5);
}
NW_HOOK_METHOD(0x1005be70, NCConsole, ToggleMacroList)

// 0x1005bed0 - the macro window the editor key opens, the same way (C3: the window made second after the list, at +0x134;
// C2 toggled the one at +0xFC)
void NCConsole::ToggleMacroEdit()
{
    if (NewWnd100->IsShowWindow())
    {
        GConsole->PlayInterfaceSound(6);
        NewWnd100->HideWindow();
        return;
    }
    NewWnd100->ShowWindow();
    NewWnd100->SetFocus();
    GConsole->PlayInterfaceSound(5);
}
NW_HOOK_METHOD(0x1005bed0, NCConsole, ToggleMacroEdit)

// 0x1005c210 - the Christmas seal window (+0x1EC): hidden when shown, else shown and focused (tail calls)
void NCConsole::ToggleXMasSeal()
{
    if (NewWnd19C->IsShowWindow())
        NewWnd19C->HideWindow();
    else
    {
        NewWnd19C->ShowWindow();
        NewWnd19C->SetFocus();
    }
}
NW_HOOK_METHOD(0x1005c210, NCConsole, ToggleXMasSeal)

// 0x1005bf90 - the GM window's name box takes the focus
void NCConsole::ToggleGMWnd()
{
    if (GMWnd->IsShowWindow())
    {
        GMWnd->HideWindow();
        return;
    }
    GMWnd->ShowWindow();
    GMWnd->NameEdit->SetFocus();
}
NW_HOOK_METHOD(0x1005bf90, NCConsole, ToggleGMWnd)

// the GM view windows: hidden, or the server asked about the player Name and the window
// shown (repeated in the six stock toggles)
#define NCCONSOLE_GM_TOGGLE(Method, Wnd, Request)                                             \
    void NCConsole::Method(TCHAR* Name)                                                       \
    {                                                                                         \
        if (Wnd->IsShowWindow())                                                              \
        {                                                                                     \
            Wnd->HideWindow();                                                                \
            return;                                                                           \
        }                                                                                     \
        GConsole->Request(Name);                                                              \
        Wnd->ShowWindow();                                                                    \
        Wnd->SetFocus();                                                                      \
    }

// 0x1005bfd0
NCCONSOLE_GM_TOGGLE(ToggleGMInventory, GMInventoryWnd, RequestGMInventory)
NW_HOOK_METHOD(0x1005bfd0, NCConsole, ToggleGMInventory)
// 0x1005c030
NCCONSOLE_GM_TOGGLE(ToggleGMMagicSkill, GMMagicSkillWnd, RequestGMSkills)
NW_HOOK_METHOD(0x1005c030, NCConsole, ToggleGMMagicSkill)
// 0x1005c090
NCCONSOLE_GM_TOGGLE(ToggleGMDetailStatus, GMDetailStatusWnd, RequestGMStatus)
NW_HOOK_METHOD(0x1005c090, NCConsole, ToggleGMDetailStatus)
// 0x1005c0f0
NCCONSOLE_GM_TOGGLE(ToggleGMQuest, GMQuestWnd, RequestGMQuests)
NW_HOOK_METHOD(0x1005c0f0, NCConsole, ToggleGMQuest)
// 0x1005c150
NCCONSOLE_GM_TOGGLE(ToggleGMStore, GMStoreWnd, RequestGMWarehouse)
NW_HOOK_METHOD(0x1005c150, NCConsole, ToggleGMStore)
// 0x1005c1b0
NCCONSOLE_GM_TOGGLE(ToggleGMAction, GMActionWnd, RequestGMPledge)
NW_HOOK_METHOD(0x1005c1b0, NCConsole, ToggleGMAction)

// 0x1005d500 - a tab of the character window (NCMainWnd::ShowTab), or with -1 the window shown (and focused) or hidden
void NCConsole::ToggleCharacter(INT Tab)
{
    if (Tab != -1)
    {
        MainWnd->ShowTab(Tab);
        return;
    }
    if (MainWnd->IsShowWindow())
    {
        GConsole->PlayInterfaceSound(SOUND_CharClose);
        MainWnd->HideWindow();
    }
    else
    {
        MainWnd->ShowWindow();
        MainWnd->SetFocus();
        GConsole->PlayInterfaceSound(SOUND_CharOpen);
    }
}
NW_HOOK_METHOD(0x1005d500, NCConsole, ToggleCharacter)

/*-----------------------------------------------------------------------------
    Chat.
-----------------------------------------------------------------------------*/

// 0x1005c250 - a system message (type 5) into the chat window in the color given, when its line has a text
void NCConsole::AddSystemMessage(INT MsgID, DWORD Color)
{
    FL2SystemMsgData& Msg = GL2GameData.SystemMsg[MsgID];
    if (*Msg.Message)
        ChatWnd->AddChatMessage(*Msg.Message, Color, 5, 0);
}
NW_HOOK_METHOD(0x1005c250, NCConsole, AddSystemMessage)

// 0x1005c2b0 - a system line (type 5), when not empty
void NCConsole::AddSystemText(const TCHAR* Text, INT SubType, DWORD Color)
{
    if (Text && *Text)
        ChatWnd->AddChatMessage(Text, Color, 5, SubType);
}
NW_HOOK_METHOD(0x1005c2b0, NCConsole, AddSystemText)

// 0x1005cf40 (new in C4; name ours) - the three icon buttons above the chat window (the tutorial's question mark, the
// quest and the mail buttons: NCEffectButtons made by OnCreate) sit side by side at 5, 42 and 79, 37 px above its top (130
// px more while its system message list shows). Called when the chat is resized
void NCConsole::DockAboveChat()
{
    NRectF Screen;
    NRectF Chat;
    Chat.X = 0;
    Chat.Y = 0;
    Chat.W = 0.0f;
    Chat.H = 0.0f;
    Screen.X = 0;
    Screen.Y = 0;
    Screen.W = 0.0f;
    Screen.H = 0.0f;
    GetLocalRect(&Screen);
    ChatWnd->GetLocalRect(&Chat);
    if (ChatWnd->SystemList->IsShowWindow())
    {
        nw::Field<NCWnd*>(this, 0x17C)->SetScreenPos(5, Chat.Y - 167, (POINT*)&Screen);
        QuestBtn->SetScreenPos(42, Chat.Y - 167, (POINT*)&Screen);
        MailBtn->SetScreenPos(79, Chat.Y - 167, (POINT*)&Screen);
    }
    else
    {
        nw::Field<NCWnd*>(this, 0x17C)->SetScreenPos(5, Chat.Y - 37, (POINT*)&Screen);
        QuestBtn->SetScreenPos(42, Chat.Y - 37, (POINT*)&Screen);
        MailBtn->SetScreenPos(79, Chat.Y - 37, (POINT*)&Screen);
    }
}
NW_HOOK_METHOD(0x1005cf40, NCConsole, DockAboveChat)

// 0x1005d580 - "Name: Text" of the chat types but the petition ones (6, 7: into the GM petition window, or the player's, shown
// and focused, when the GM one is hidden). C4: type 18 is the bare text; type 14 goes to the party room window (+0x16C, its
// chat list), 15 and 16 to a window of its own (+0x264, shown first); the other types to the chat. Then the speaker's balloon:
// its first 18 characters ("..." after a longer text) for 5 seconds, a newer one replacing it - not when the speaker has a
// private store (User +0x200)
void NCConsole::Say(INT ObjectID, const TCHAR* Name, const TCHAR* Text, DWORD Color, INT Type)
{
    TCHAR Line[1024];
    if (Type == 0x12)
        appSprintf(Line, TEXT("%s"), Text);
    else
    {
        appSprintf(Line, TEXT("%s: %s"), Name, Text);
        if (Type == 6 || Type == 7)
        {
            GMPetitionWnd->AddMessage(Line, Color, Type, 0);   // NCPetitionWnd::AddMessage (the GM's)
            if (!GMPetitionWnd->IsShowWindow())
            {
                ConsolePetitionWnd()->ShowWindow();
                ConsolePetitionWnd()->SetFocus();
                static_cast<NCPetitionWnd*>(ConsolePetitionWnd())->AddMessage(Line, Color, Type, 0);   // the player's
            }
            return;
        }
    }
    if (Type == 0xE)
        static_cast<NCPartyRoomWnd*>(PartyRoomWnd)->AddChatLine(Line, Color);   // the room's chat list takes the line
    else if (Type == 0xF || Type == 0x10)
    {
        nw::Field<NCWnd*>(this, 0x264)->ShowWindow();
        static_cast<NCCommandChannelWnd*>(nw::Field<NCWnd*>(this, 0x264))->AddChatLine(Line, Color, 0, 0);
    }
    else
        ChatWnd->AddChatMessage(Line, Color, Type, 0);
    User* Speaker = GConsole->FindUserByID(ObjectID);
    if (Speaker && Speaker->PrivateStoreType)
        return;
    NChatBalloon Balloon;
    TCHAR* Short = (TCHAR*)appMalloc(23 * sizeof(TCHAR), TEXT("new"));
    if (wcslen(Text) < 19)
        appStrcpy(Short, Text);
    else
    {
        wcsncpy(Short, Text, 19);
        Short[18] = '.';   // C4: the dots replace the 19th character (C3: they followed it)
        Short[19] = '.';
        Short[20] = '.';
        Short[21] = 0;
    }
    Balloon.Tick = Ticks;
    Balloon.ObjectID = ObjectID;
    Balloon.Text = Short;
    INT i;
    for (i = 0; i < Balloons.Num(); i++)
        if (Balloons(i).ObjectID == ObjectID)
            break;
    if (i < Balloons.Num())
    {
        if (Balloons(i).Text)
            delete[] Balloons(i).Text;
        Balloons(i) = Balloon;
    }
    else
        Balloons(Balloons.Add()) = Balloon;
    Speaker = GConsole->FindUserByID(ObjectID);
    if (Speaker && Speaker->Pawn)
    {
        if (!Speaker->Pawn->ChatBalloon)
            Speaker->Pawn->Unknown5C++;
        Speaker->Pawn->ChatBalloon = Short;
    }
}
NW_HOOK_METHOD(0x1005d580, NCConsole, Say)

// 0x1005cb80 (vslot 68) - "Link <bypass>": to the server (the tutorial pages' links go their own way). "Write
// <board> <a> <b> <control> [<control> [<control>]]": a board post, the texts of the page's controls named; boards 4,
// 5 and 7 take "-" for the second text, 4 and 7 for the third. C3: a missing third text is no post at all (returns
// 0 without sending; C2 sent it). C4: the same text (the class of the tutorial viewer is at 0x102C89E0)
INT NCConsole::OnHtmlCommand(const TCHAR* Command, NCHtmlViewer* Source)
{
    const TCHAR* Str = Command;
    if (ParseCommand(&Str, TEXT("Link")))
    {
        TCHAR Bypass[256] = { 0 };
        if (ParseToken(Str, Bypass, 256, 1))
        {
            L2ParamStack Params(10);
            Params.PushBack(Bypass);
            if (Source->IsA(NCTutorialHtmlViewer::StaticClass()))
                GConsole->RequestTutorialLinkHtml(Params);
            else
                GConsole->RequestLinkHtml(Params);
        }
        return 1;
    }
    if (!ParseCommand(&Str, TEXT("Write")))
        return 0;
    L2ParamStack Params(10);
    TCHAR Board[256], Arg1[256], Arg2[256], Control1[256], Control2[256], Control3[256];
    if (!ParseToken(Str, Board, 256, 1))
        return 0;
    Params.PushBack(Board);
    if (!ParseToken(Str, Arg1, 256, 1))
        return 0;
    Params.PushBack(Arg1);
    if (!ParseToken(Str, Arg2, 256, 1))
        return 0;
    Params.PushBack(Arg2);
    if (!ParseToken(Str, Control1, 256, 1) || !Source)
        return 0;
    TCHAR* Text = Source->GetControlValue(Control1);
    if (!Text)
        return 0;
    Params.PushBack(Text);
    if (appStrcmp(Board, TEXT("4")) && appStrcmp(Board, TEXT("5")) && appStrcmp(Board, TEXT("7")))
    {
        if (!ParseToken(Str, Control2, 256, 1))
            return 0;
        Text = Source->GetControlValue(Control2);
        if (!Text)
            return 0;
        Params.PushBack(Text);
    }
    else
    {
        appStrcpy(Control2, TEXT("-"));
        Params.PushBack(Control2);
    }
    if (appStrcmp(Board, TEXT("4")) && appStrcmp(Board, TEXT("7")))
    {
        if (ParseToken(Str, Control3, 256, 1))
        {
            Text = Source->GetControlValue(Control3);
            if (!Text)
                return 0;
            Params.PushBack(Text);
        }
    }
    else
    {
        appStrcpy(Control3, TEXT("-"));
        Params.PushBack(Control3);
    }
    GConsole->RequestBBSWrite(Params);
    return 1;
}
NW_HOOK_METHOD(0x1005cb80, NCConsole, OnHtmlCommand)

// 0x1005c480 - a balloon's destructor (the text is freed by hand; Say's unwind code)
NW_HOOK_DTOR(0x1005c480, NChatBalloon)

// 0x10060080 - the chat balloons go, then NCWnd
NW_HOOK_DTOR(0x10060080, NCConsole)

// 0x1005c2e0 (vslot 47) - guarded: the desktop is resized, and the broadcast observer window with it
void NCConsole::SetWindowSize(FLOAT W, FLOAT H)
{
    guard(NCConsole::SetWindowSize);
    NCWnd::SetWindowSize(W, H);
    if (BroadcastObserverWnd)
        BroadcastObserverWnd->SetWindowSize(W, H);
    unguard;
}
NW_HOOK_METHOD(0x1005c2e0, NCConsole, SetWindowSize)

NW_CHECK_VTABLE(NCConsole, 0x101aed40, 75)
