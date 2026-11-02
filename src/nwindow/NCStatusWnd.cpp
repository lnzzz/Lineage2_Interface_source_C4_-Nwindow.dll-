/*=============================================================================
    NCStatusWnd.cpp - the HP / MP windows: base, player, target, party member, pet, summon.
    Stock: 0x100ae300 .. 0x100ae9ac, 0x100aeb70 .. 0x100aebe3, 0x100af2b0 .. 0x100b16e0.
=============================================================================*/
#include "nwindow/NCStatusWnd.h"
#include "nwindow/NCPartyWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCProgressCtrl.h"
#include "nwindow/NCStatusBarCtrl.h"
#include "nwindow/NWndCreate.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NGameData.h"
#include "nwindow/NLeftHelpers.h"

#include <stdio.h>
#include <wchar.h>

// the stock code calls the CRT's wcscpy; the intrinsic would copy inline
#pragma function(wcscpy)

// exp at the start of each level (index = level), a table of the stock image
#if NW_HOOK_MODE
#define GExpTable (reinterpret_cast<const INT*>(::nw::Addr(0x101CC91C)))  // C3: read from the stock OnPaint (0x10100566)
#else
extern const INT GExpTable[];
#endif

// interface sounds (FL2NetNotify::OnInterfacePlaySound)
enum { SOUND_WindowClose = 6 };

// title bar ids of the status windows, the summon's time bar
enum
{
    NID_PLAYERSTATUS_FRAME = 0xFFB000,
    NID_TARGETSTATUS_FRAME = 0xF8,
    NID_PETSTATUS_FRAME = 0xFFA0B1,
    NID_SUMMONSTATUS_FRAME = 0xFFA0B2,
    NID_SUMMONSTATUS_TIME = 0xF0A0B0,
    NID_SUMMONSTATUS_TOGGLE = 0xF0A0B1,  // C3: the buff icons' button
    NID_PETSTATUS_TOGGLE = 0xF0F00A1,    // C3
    NID_TARGETSTATUS_RESIZE = 0xF20F15,  // C4: the right edge
    NID_PETSTATUS_RESIZE = 0xF0F00A2,    // C4
    NID_SUMMONSTATUS_RESIZE = 0xF0A0B2,  // C4
};

/*-----------------------------------------------------------------------------
    Three anonymous functions of this lane, hooked here: a button's textures, a name label's text, a class group's name.
-----------------------------------------------------------------------------*/

// 0x10001630 (an anonymous method of NCButton, name ours; ret 0xC) - the button's normal and pushed textures, and a third
// (the one at +0x208, the button's own when none is given)
struct NCButtonTexturesView : public NCButton
{
    void SetTextures(UTexture* InNormal, UTexture* InPushed, UTexture* InThird)
    {
        ::nw::Field<UTexture*>(this, 0x204) = InPushed;
        ::nw::Field<UTexture*>(this, 0x200) = InNormal;
        ::nw::Field<UTexture*>(this, 0x208) = InThird;
        if (!InThird)
            ::nw::Field<UTexture*>(this, 0x208) = InNormal;
    }
};
NW_HOOK_METHOD(0x10001630, NCButtonTexturesView, SetTextures)

// 0x1002c760 (an anonymous method of the name label, class data 0x102C4EE8; name ours; ret 0x10) - the text and its color: the
// label keeps the text in a string at +0x1FC and lays it out in its tool tip record at +0x220 (0x400 wide; the stock
// 0x10032460 clears the record, sets the text and the width and lays it out), the painter (Kind, +0x1F4), the alignment
// (+0x1F8) and the color (+0x21C), as NCNameCtrl.h has them. L2 Ignition: the color went to +0x1F8 and the alignment to
// +0x21C, so the painter met neither alignment and drew no name (the stock stores the third argument at +0x1F8 and the
// fourth at +0x21C)
struct NCNameCtrlView : public NCWnd
{
    __declspec(noinline) void SetText(const TCHAR* Text, INT FlagA, INT FlagB, DWORD Color)   // out of line as the stock's: the paints call it
    {
        ::nw::Field<INT>(this, 0x1F8) = FlagB;
        FString& Str = ::nw::Field<FString>(this, 0x1FC);
        ::nw::Field<INT>(this, 0x1F4) = FlagA;
        Str = Text;
        ((NToolTipInfo*)((BYTE*)this + 0x220))->SetText(*Str, 0x400);   // 0x10032460
        ::nw::Field<DWORD>(this, 0x21C) = Color;
    }
};
NW_HOOK_METHOD(0x1002c760, NCNameCtrlView, SetText)

// the name label's SetText for the other windows (NWndCreate.h's SetNameText)
__declspec(noinline) void NameCtrlSetText(NCWnd* NameCtrl, const TCHAR* Text, INT FlagA, INT FlagB, DWORD Color)
{
    ((NCNameCtrlView*)NameCtrl)->SetText(Text, FlagA, FlagB, Color);
}

// 0x10074320 (an anonymous method of the console; the stock ignores its this) - the name of the group a class belongs to
// (sysstrings 874 .. 880, 175, 176; the groups 9 .. 15 repeat the first seven), NULL past group 15
static const TCHAR* __fastcall ClassGroupNameImpl(void*, void*, INT ClassID)
{
    switch (((NConsoleLeftFns*)GConsole)->GetClassGroup(ClassID))   // 0x1006B970
    {
    case 0:
    case 9:
        return *GL2GameData.SysString[874];
    case 1:
    case 10:
        return *GL2GameData.SysString[875];
    case 2:
    case 11:
        return *GL2GameData.SysString[876];
    case 3:
    case 12:
        return *GL2GameData.SysString[877];
    case 4:
    case 13:
        return *GL2GameData.SysString[878];
    case 5:
    case 14:
        return *GL2GameData.SysString[879];
    case 6:
    case 15:
        return *GL2GameData.SysString[880];
    case 7:
        return *GL2GameData.SysString[175];
    case 8:
        return *GL2GameData.SysString[176];
    }
    return NULL;
}
NW_HOOK_FUNC(0x10074320, ClassGroupNameImpl)

// 0x10074c70 (a stdcall function, ret 4; hooked as GetPartyStyleIconName in NConsoleWndAnon.cpp, where it is static: this is the same
// table for UpdateStyle) - the icon of a party member's class: by the class's group, "party_styleicon" 1 .. 7, 1_1 .. 1_3 and 2_3 .. 7_3;
// none for another group
static const TCHAR* PartyStyleIconName(INT ClassID)
{
    switch (((NConsoleLeftFns*)GConsole)->GetClassGroup(ClassID))
    {
    case 0: return TEXT("L2UI_CH3.PartyWnd.party_styleicon1");
    case 1: return TEXT("L2UI_CH3.PartyWnd.party_styleicon2");
    case 2: return TEXT("L2UI_CH3.PartyWnd.party_styleicon3");
    case 3: return TEXT("L2UI_CH3.PartyWnd.party_styleicon4");
    case 4: return TEXT("L2UI_CH3.PartyWnd.party_styleicon5");
    case 5: return TEXT("L2UI_CH3.PartyWnd.party_styleicon6");
    case 6: return TEXT("L2UI_CH3.PartyWnd.party_styleicon7");
    case 7: return TEXT("L2UI_CH3.PartyWnd.party_styleicon1_1");
    case 8: return TEXT("L2UI_CH3.PartyWnd.party_styleicon1_2");
    case 9: return TEXT("L2UI_CH3.PartyWnd.party_styleicon1_3");
    case 10: return TEXT("L2UI_CH3.PartyWnd.party_styleicon2_3");
    case 11: return TEXT("L2UI_CH3.PartyWnd.party_styleicon3_3");
    case 12: return TEXT("L2UI_CH3.PartyWnd.party_styleicon4_3");
    case 13: return TEXT("L2UI_CH3.PartyWnd.party_styleicon5_3");
    case 14: return TEXT("L2UI_CH3.PartyWnd.party_styleicon6_3");
    case 15: return TEXT("L2UI_CH3.PartyWnd.party_styleicon7_3");
    }
    return NULL;
}

// C2 NConsoleWnd::RequestAttack (0x10059f20, ret 0x18) takes two more words than C1's: the
// callers push 0, 0 after the position
static void RequestAttackC2(INT ObjectID, const FVector& Origin)
{
    GConsole->RequestAttack(ObjectID, Origin, 0, 0);
}

// C3: the console's IsCtrlDown is vslot 297 (+0x4a4); NConsoleWnd.h has it non-virtual for now
static UBOOL ConsoleIsCtrlDown()
{
    return ::nw::VirtualCall<UBOOL>(GConsole, 319);
}

// the name when it is not empty, else NULL (inlined at each use in the stock paints)
static FORCEINLINE const TCHAR* NonEmpty(const TCHAR* Text)
{
    return Text && *Text ? Text : NULL;
}

/*-----------------------------------------------------------------------------
    NCStatusWnd.
-----------------------------------------------------------------------------*/

// 0x10116aa0 - C3: the bar width goes to +0x128 (the new ClassID sits before it)
NCStatusWnd::NCStatusWnd(INT InBarWidth)
{
    MPTex = NULL;
    HPTex = NULL;
    EXPTex = NULL;
    BackTex = NULL;
    BarWidth = InBarWidth;
}
NW_HOOK_CTOR_ARGS(0x10116aa0, NCStatusWnd, (INT InBarWidth), (InBarWidth))

// 0x10116260 - the vtable, ConditionalDestroy, NCWnd
NW_HOOK_DTOR(0x10116260, NCStatusWnd)


// stock body shared with NControl's (0x10004e90)
INT NCStatusWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    return NCWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// C2 CANDIDATE: the C2 function is not the same code as the C1 one; kept only while it matches the stock one (verify_data)
// 0x1010d3f0 (vslot 37) - C3: Clear is a virtual (slot 68), the party member's adds its buffs
INT NCStatusWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("NWindow.WhiteTexture"));
    Clear();
    return 1;
}
NW_HOOK_METHOD(0x1010d3f0, NCStatusWnd, OnCreate)

// 0x1010d420 (vslot 38)
INT NCStatusWnd::OnDestroy()
{
    guard(NCStatusWnd::OnDestroy);
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1010d420, NCStatusWnd, OnDestroy)

// 0x1010f130 (vslot 62) - draws nothing
INT NCStatusWnd::OnPaint(NCGDevice* Device)
{
    guard(NCStatusWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1010f130, NCStatusWnd, OnPaint)

// 0x1010d4a0
UBOOL NCStatusWnd::IsName(const TCHAR* InName)
{
    if (!InName || !Name[0])
        return 0;
    return wcscmp(Name, InName) == 0;
}
NW_HOOK_METHOD(0x1010d4a0, NCStatusWnd, IsName)

// 0x1010d4d0 (vslot 74) - C4: the name and the six numbers (CP and its maximum are new in front of HP)
void NCStatusWnd::Clear()
{
    Name[0] = 0;
    CP = 0;
    MaxCP = 0;
    HP = 0;
    MaxHP = 0;
    MP = 0;
    MaxMP = 0;
}
NW_HOOK_METHOD(0x1010d4d0, NCStatusWnd, Clear)

// 0x1010dbf0 (name ours) - a free slot of the party window has no name (the callers, NCPartyWnd's, are ours too: the stock
// UpdateLayout keeps ecx across the call)
__declspec(noinline) UBOOL NCStatusWnd::IsEmpty()
{
    return Name[0] == 0;
}
NW_HOOK_METHOD(0x1010dbf0, NCStatusWnd, IsEmpty)

/*-----------------------------------------------------------------------------
    NCPlayerStatusWnd.
-----------------------------------------------------------------------------*/

// 0x10116be0 - C3: one more field cleared (+0x358)
NCPlayerStatusWnd::NCPlayerStatusWnd(INT InBarWidth)
    : NCStatusWnd(InBarWidth)
{
    Unknown358 = 0;
    ExtraWidth = 0;
}
NW_HOOK_CTOR_ARGS(0x10116be0, NCPlayerStatusWnd, (INT InBarWidth), (InBarWidth))

// 0x10116c40 - the vtable, ConditionalDestroy, NCStatusWnd
NW_HOOK_DTOR(0x10116c40, NCPlayerStatusWnd)


// 0x1010d770 (vslot 25) - the right edge reports how far it was dragged (C2 has no IsEnabled
// check of its own: NCStatusWnd's does it)
INT NCPlayerStatusWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCStatusWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND && (INT)wParam == NID_PLAYERSTATUS_RESIZE)
        return OnResize((INT)lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x1010d770, NCPlayerStatusWnd, WindowProc)

// 0x10116c90 (vslot 37) - C4: the title bar is 84 high, the HP bar sits at y 41 and is a status bar control with
// a label ("HP") of its own, there is a name control (new), the CP textures are the player window's own
INT NCPlayerStatusWnd::OnCreate()
{
    NCStatusWnd::OnCreate();
    MaxExtraWidth = 202;

    FrameCtrl = NewFrameCtrl(1, 1, 1);
    FrameCtrl->ID = NID_PLAYERSTATUS_FRAME;
    FrameCtrl->SetPosition(0, 0, 12, 84);
    CreateChild(FrameCtrl, NWS_VISIBLE | NWS_NOCLOSE);

    ResizeFrame = NewResizeFrame(1);
    ResizeFrame->ID = NID_PLAYERSTATUS_RESIZE;
    ResizeFrame->SetPosition((INT)(Width - 10.0f), 0, 10, (DWORD)(INT)Height);
    CreateChild(ResizeFrame, NWS_VISIBLE | 0x20);   // 0x20: meaning unknown

    // NCStatusBarCtrl(label, texture width, height, fill, back, ghost, overlay): 0x101163F0 (ret 0x1c)
    NCStatusBarCtrl* Bar = new NCStatusBarCtrl(TEXT("HP"), 8, 12, TEXT("L2UI_CH3.PlayerStatusWnd.ps_hpbar"),
        TEXT("L2UI_CH3.PlayerStatusWnd.ps_hpbar_back"), TEXT("L2UI_CH3.PlayerStatusWnd.ps_hpbarfill"),
        TEXT("L2UI_CH3.PlayerStatusWnd.ps_hpbarwarn1"));   // 0x101163F0
    HPBar = Bar;
    Bar->ID = (INT)0xFFA103F0;
    HPBar->SetPosition(16, 41, 150, 12);
    CreateChild(HPBar, NWS_VISIBLE | 0x4);   // 6

    NameCtrl = NewNameCtrl();
    NameCtrl->SetPosition(40, 9, (DWORD)(INT)(Width - 40.0f - 10.0f), 14);
    CreateChild(NameCtrl, NWS_VISIBLE | 0x20 | 0x4);   // 0x26
    ClearNameCtrlFlag(NameCtrl);

    BackTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.Smallwindow2_back2"));
    HPTex = FindTexture(TEXT("L2UI_CH3.PlayerStatusWnd.ps_hpbar"));
    MPTex = FindTexture(TEXT("L2UI_CH3.PlayerStatusWnd.ps_mpbar"));
    EXPTex = FindTexture(TEXT("L2UI_CH3.PlayerStatusWnd.ps_expbar"));
    LeftBackTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.Smallwindow2_back1"));
    RightBackTex = FindTexture(TEXT("L2UI_CH3.PlayerStatusWnd.ps_sizecontrol1"));
    PlayerHPBackTex = FindTexture(TEXT("L2UI_CH3.PlayerStatusWnd.ps_hpbar_back"));
    PlayerMPBackTex = FindTexture(TEXT("L2UI_CH3.PlayerStatusWnd.ps_mpbar_back"));
    PlayerEXPBackTex = FindTexture(TEXT("L2UI_CH3.PlayerStatusWnd.ps_expbar_back"));
    CPBackTex = FindTexture(TEXT("L2UI_CH3.PlayerStatusWnd.ps_cpbar_back"));
    CPTex = FindTexture(TEXT("L2UI_CH3.PlayerStatusWnd.ps_cpbar"));
    LevelBackTex = FindTexture(TEXT("L2UI_CH3.PlayerStatusWnd.ps_levelback"));
    SizeControl2Tex = FindTexture(TEXT("L2UI_CH3.PlayerStatusWnd.ps_sizecontrol2"));
    return 1;
}
NW_HOOK_METHOD(0x10116c90, NCPlayerStatusWnd, OnCreate)

// 0x1010f4c0 (vslot 38) - the base's first, then the right edge, the name control and the HP bar go
INT NCPlayerStatusWnd::OnDestroy()
{
    guard(NCPlayerStatusWnd::OnDestroy);
    NCStatusWnd::OnDestroy();
    DestroyChild(this, ResizeFrame);
    DestroyChild(this, NameCtrl);
    DestroyChild(this, HPBar);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1010f4c0, NCPlayerStatusWnd, OnDestroy)

// 0x10113be0 (vslot 62) - C4: the window is 84 high, drawn from pieces (the left edge, the middle stretched to the width,
// the right edge, the level's frame, the handle of the right edge); the HP bar is a control of its own (HPBar: told
// the HP and the maximum by each paint), the name is the name control's; the bars under it are drawn here: CP, MP and
// exp (12 pixels high each), BarW = ExtraWidth + 150 pixels wide at most (their texture is 8 texels); exp in percent
// with two decimals, the level (a level of two digits is drawn digit by digit) and "CP now / max" and "MP now / max" in
// the large digits (the CP line is new).
INT NCPlayerStatusWnd::OnPaint(NCGDevice* Device)
{
    guard(NCPlayerStatusWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (LeftBackTex)
        Device->DrawTexture(12, 0, 16, 84, 0, 0, 16, 76, LeftBackTex, 0xFF, 1);
    if (BackTex)
        Device->DrawTexture(28, 0, (INT)(Width - 12.0f - 16.0f - 4.0f), 84, 0, 0, 16, 76, BackTex, 0xFF, 1);
    if (RightBackTex)
        Device->DrawTexture((INT)(Width - 4.0f), 0, 4, 84, 0, 0, 4, 76, RightBackTex, 0xFF, 1);
    if (LevelBackTex)
        Device->DrawTexture(16, 6, 22, 20, 0, 0, 22, 20, LevelBackTex, 0xFF, 1);
    if (SizeControl2Tex)
        Device->DrawTexture(ExtraWidth + 167, 4, 9, 13, 0, 0, 9, 13, SizeControl2Tex, 0xFF, 0);
    Player = GConsole->GetMyUser();
    if (Player)
    {
        // the HP bar control takes the HP and the maximum (0x10112bb0, ret 8)
        ((NCStatusBarCtrl*)HPBar)->SetValues(Player->HP, Player->MaxHP);

        INT BarW = ExtraWidth + 150;
        INT MPW = 0;
        INT CPW = 0;
        INT EXPW = 0;
        INT MaxCP = nw::Field<INT>(Player, 0x214);   // C4 (names ours): the CP and its maximum
        INT CP = nw::Field<INT>(Player, 0x218);
        if (Player->MaxMP)
            MPW = (INT)((DOUBLE)Player->MP * BarW / Player->MaxMP);
        if (MaxCP)
            CPW = (INT)((DOUBLE)CP * BarW / MaxCP);
        INT NextExp = GExpTable[Player->Level + 1];
        if (NextExp)
            EXPW = (INT)((DOUBLE)(Player->Exp - GExpTable[Player->Level]) * BarW / (NextExp - GExpTable[Player->Level]));

        Device->DrawTexture(16, 27, BarW, 12, 0, 0, 8, 12, CPBackTex, 0xFF, 1);
        Device->DrawTexture(16, 55, BarW, 12, 0, 0, 8, 12, PlayerMPBackTex, 0xFF, 1);
        Device->DrawTexture(16, 69, BarW, 12, 0, 0, 8, 12, PlayerEXPBackTex, 0xFF, 1);
        if (CPW > 0 && CPW <= BarW)
            Device->DrawTexture(16, 27, CPW, 12, 0, 0, 8, 12, CPTex, 0xFF, 1);
        else if (CPW > BarW)
            Device->DrawTexture(16, 27, BarW, 12, 0, 0, 8, 12, CPTex, 0xFF, 1);
        if (MPW > 0 && MPW <= BarW)
            Device->DrawTexture(16, 55, MPW, 12, 0, 0, 8, 12, MPTex, 0xFF, 1);
        else if (MPW > BarW)
            Device->DrawTexture(16, 55, BarW, 12, 0, 0, 8, 12, MPTex, 0xFF, 1);
        if (EXPW > 0 && EXPW <= BarW)
            Device->DrawTexture(16, 69, EXPW, 12, 0, 0, 8, 12, EXPTex, 0xFF, 1);
        else if (EXPW > BarW)
            Device->DrawTexture(16, 69, BarW, 12, 0, 0, 8, 12, EXPTex, 0xFF, 1);

        TCHAR Buf[64];
        SIZE Size;
        INT LevelExp = GExpTable[Player->Level];
        _swprintf(Buf, TEXT("%.2f%s"),
                  (DOUBLE)(Player->Exp - LevelExp) * 100.0 / (GExpTable[Player->Level + 1] - LevelExp), TEXT("%"));
        NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
        DrawDigit(Device, 1, BarW / 2 - Size.cx / 2 + 12, 71, 0xFFDCDCDC, Buf);

        SetNameText(NameCtrl, Player->GetName(), 0, 0, 0xFFDCDCDC);

        if (Player->Level <= 9)
        {
            appSprintf(Buf, TEXT("%d"), Player->Level);
            NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
            DrawDigit(Device, 1, 27 - Size.cx / 2, 12, 0xFFB09B79, Buf);
        }
        else
        {
            appSprintf(Buf, TEXT("%d"), Player->Level);
            TCHAR Digit[2];
            Digit[0] = Buf[0];
            Digit[1] = 0;
            DrawDigit(Device, 1, 20, 12, 0xFFB09B79, Digit);
            Digit[0] = Buf[1];
            Digit[1] = 0;
            DrawDigit(Device, 1, 27, 12, 0xFFB09B79, Digit);
        }

        DrawDigit(Device, 1, 25, 29, 0xFFDCDCDC, TEXT("CP"));
        appSprintf(Buf, TEXT("%s"), TEXT("/"));
        NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
        DrawDigit(Device, 1, BarW / 2 - Size.cx / 2 + 16, 29, 0xFFDCDCDC, Buf);
        appSprintf(Buf, TEXT("%d"), CP);
        NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
        DrawDigit(Device, 1, BarW / 2 - Size.cx + 8, 29, 0xFFDCDCDC, Buf);
        appSprintf(Buf, TEXT("%d"), MaxCP);
        NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
        DrawDigit(Device, 1, BarW / 2 + 20, 29, 0xFFDCDCDC, Buf);

        DrawDigit(Device, 1, 25, 57, 0xFFDCDCDC, TEXT("MP"));
        appSprintf(Buf, TEXT("%s"), TEXT("/"));
        NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
        DrawDigit(Device, 1, BarW / 2 - Size.cx / 2 + 16, 57, 0xFFDCDCDC, Buf);
        appSprintf(Buf, TEXT("%d"), Player->MP);
        NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
        DrawDigit(Device, 1, BarW / 2 - Size.cx + 8, 57, 0xFFDCDCDC, Buf);
        appSprintf(Buf, TEXT("%d"), Player->MaxMP);
        NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
        DrawDigit(Device, 1, BarW / 2 + 20, 57, 0xFFDCDCDC, Buf);
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10113be0, NCPlayerStatusWnd, OnPaint)

// 0x1010d7d0 (vslot 68) - targets the player
INT NCPlayerStatusWnd::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    GConsole->TargetMyself();
    return 1;
}
NW_HOOK_METHOD(0x1010d7d0, NCPlayerStatusWnd, OnLButtonDown)

// The shared part of the three resizing windows' OnResize: the stock bodies (player, target, pet, summon) all clamp
// ExtraWidth to 0 .. MaxExtraWidth (202) and adjust Delta by what was cut; false when nothing changes
static FORCEINLINE UBOOL ApplyExtraWidth(INT& ExtraWidth, INT MaxExtraWidth, INT& Delta)
{
    if (Delta > 0)
    {
        if (ExtraWidth >= MaxExtraWidth)
            return 0;
        INT NewExtra = ExtraWidth + Delta;
        ExtraWidth = NewExtra;
        if (NewExtra > MaxExtraWidth)
        {
            Delta += MaxExtraWidth - NewExtra;
            ExtraWidth = MaxExtraWidth;
        }
    }
    else
    {
        if (Delta >= 0)
            return 0;
        if (ExtraWidth <= 0)
            return 0;
        INT NewExtra = ExtraWidth + Delta;
        ExtraWidth = NewExtra;
        if (NewExtra < 0)
        {
            Delta -= NewExtra;
            ExtraWidth = 0;
        }
    }
    return 1;
}

// 0x1010f5c0 (vslot 75) - the right edge was dragged by Delta: the window grows or shrinks within 0 .. 202 pixels, the
// edge, the HP bar and (C4) the name control follow
INT NCPlayerStatusWnd::OnResize(INT Delta)
{
    NRectF Rect = {0, 0, 0.0f, 0.0f};
    GetLocalRect(&Rect);
    if (!ApplyExtraWidth(ExtraWidth, MaxExtraWidth, Delta))
        return 1;
    SetWindowSize((FLOAT)Delta + Rect.W, Rect.H);
    NRectF EdgeRect = {0, 0, 0.0f, 0.0f};
    ResizeFrame->GetLocalRect(&EdgeRect);
    ResizeFrame->SetPosition((INT)(Width - EdgeRect.W), 0, (DWORD)(INT)EdgeRect.W, (DWORD)(INT)EdgeRect.H);
    NRectF BarRect = {0, 0, 0.0f, 0.0f};
    HPBar->GetLocalRect(&BarRect);
    HPBar->SetWindowSize((FLOAT)Delta + BarRect.W, 12.0f);
    NRectF NameRect = {0, 0, 0.0f, 0.0f};
    NameCtrl->GetLocalRect(&NameRect);
    NameCtrl->SetWindowSize((FLOAT)Delta + NameRect.W, NameRect.H);
    return 1;
}
NW_HOOK_METHOD(0x1010f5c0, NCPlayerStatusWnd, OnResize)

// 0x100475f0 - the console restores the saved width with it
INT NCPlayerStatusWnd::Resize(INT Delta)
{
    return OnResize(Delta);
}
NW_HOOK_METHOD(0x100475f0, NCPlayerStatusWnd, Resize)

NW_CHECK_VTABLE(NCPlayerStatusWnd, 0x101cdea8, 76)

/*-----------------------------------------------------------------------------
    NCTargetStatusWnd.
-----------------------------------------------------------------------------*/

// 0x10116ae0 - the base's, the tool tip of the buff icon under the mouse (a member: NToolTipInfo's constructor), then
// the level difference, the buff icon under the mouse and the expanded flag cleared (C4: ExtraWidth too)
NCTargetStatusWnd::NCTargetStatusWnd(INT InBarWidth)
    : NCStatusWnd(InBarWidth)
{
    LevelDiff = 0;
    bExpanded = 0;
    HoverBuff = 0;
    ExtraWidth = 0;
}
NW_HOOK_CTOR_ARGS(0x10116ae0, NCTargetStatusWnd, (INT InBarWidth), (InBarWidth))

// 0x10116b80 - the buff tool tip goes, then NCStatusWnd
NW_HOOK_DTOR(0x10116b80, NCTargetStatusWnd)


// 0x1010d500 (vslot 25) - C4: the right edge's command (the window widens) is new
INT NCTargetStatusWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCStatusWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if ((INT)wParam == NID_TARGETSTATUS_CLOSE)
            return OnClose() != 0;
        if ((INT)wParam == NID_TARGETSTATUS_TOGGLE)
            return OnToggle() != 0;
        if ((INT)wParam == NID_TARGETSTATUS_RESIZE)
            return ::nw::VirtualCall<INT>(this, 75, (INT)lParam) != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x1010d500, NCTargetStatusWnd, WindowProc)

// 0x10112d70 (vslot 37) - C4: the bars follow the window's width (it can be widened), the tall window's bottom is
// "_ex", there are CP textures, the right edge and the name control are new
INT NCTargetStatusWnd::OnCreate()
{
    NCStatusWnd::OnCreate();
    BarWidth = (INT)(Width - 22.0f);
    BackTex3[0] = FindTexture(TEXT("L2UI_CH3.SmallWnd.SmallWindow_Back1"));
    BackTex3[1] = FindTexture(TEXT("L2UI_CH3.SmallWnd.SmallWindow_Back2"));
    BackTex3[2] = FindTexture(TEXT("L2UI_CH3.SmallWnd.SmallWindow_Back3_ex"));
    BackTex3[3] = FindTexture(TEXT("L2UI_CH3.SmallWnd.SmallWindow2_Back1"));
    BackTex3[4] = FindTexture(TEXT("L2UI_CH3.SmallWnd.SmallWindow2_Back2"));
    BackTex3[5] = FindTexture(TEXT("L2UI_CH3.SmallWnd.SmallWindow2_Back3_ex"));
    FrameTopTex = FindTexture(TEXT("L2UI_CH3.FRAMECTRL.SmallBar1"));
    FrameCenterTex = FindTexture(TEXT("L2UI_CH3.FRAMECTRL.SmallBar2"));
    FrameBottomTex = FindTexture(TEXT("L2UI_CH3.FRAMECTRL.SmallBar3"));
    UpBtnTex = FindTexture(TEXT("L2UI_CH3.TargetWnd.Target_InfoButton_Down"));
    UpBtnClickTex = FindTexture(TEXT("L2UI_CH3.TargetWnd.Target_InfoButton_Down"));
    DownBtnTex = FindTexture(TEXT("L2UI_CH3.TargetWnd.Target_InfoButton"));
    DownBtnClickTex = FindTexture(TEXT("L2UI_CH3.TargetWnd.Target_InfoButton"));
    HPTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.HpBar"));
    MPTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.MpBar"));
    CPBarTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.cpBar"));
    HPBackTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.HpBar_back"));
    MPBackTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.MpBar_back"));
    CPBarBackTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.CpBar_back"));

    ResizeFrame = NewResizeFrame(1);
    ResizeFrame->ID = NID_TARGETSTATUS_RESIZE;
    ResizeFrame->SetPosition((INT)(Width - 10.0f), 0, 10, (DWORD)(INT)Height);
    CreateChild(ResizeFrame, NWS_VISIBLE | 0x20);   // 0x20: meaning unknown

    CloseButton = NewButton(TEXT("L2UI_CH3.FrameCtrl.FrameCloseBtn"), TEXT("L2UI_CH3.FrameCtrl.FrameCloseOnBtn"), 0);
    CloseButton->ID = NID_TARGETSTATUS_CLOSE;
    CloseButton->SetPosition((INT)(Width - 19.0f), 6, 15, 15);
    CreateChild(CloseButton, NWS_VISIBLE);

    FrameCtrl = NewFrameCtrl(0, 0, 0);   // not decorated: the stock pushes edi (0) three times; (1, 0, 0) made the
                                         // frame a close button of its own, a second X beside the toggle button
    FrameID = NID_TARGETSTATUS_FRAME;
    FrameCtrl->ID = NID_TARGETSTATUS_FRAME;
    FrameCtrl->SetPosition(0, 19, 12, (DWORD)(INT)(Height - 19.0f));
    CreateChild(FrameCtrl, NWS_VISIBLE);

    ToggleButton = NewButton(TEXT("L2UI_CH3.TargetWnd.Target_InfoButton"), TEXT("L2UI_CH3.TargetWnd.Target_InfoButton"), 0);
    ToggleButton->ID = NID_TARGETSTATUS_TOGGLE;
    ToggleButton->SetPosition(1, 6, 11, 13);
    CreateChild(ToggleButton, NWS_VISIBLE);

    NameCtrl = NewNameCtrl();
    NameCtrl->SetPosition(12, 9, (DWORD)(INT)(Width - 30.0f), 14);
    CreateChild(NameCtrl, NWS_VISIBLE | 0x20 | 0x4);   // 0x26
    ClearNameCtrlFlag(NameCtrl);
    return 1;
}
NW_HOOK_METHOD(0x10112d70, NCTargetStatusWnd, OnCreate)

// 0x1010f1f0 (vslot 38) - C4: the right edge and the name control go too
INT NCTargetStatusWnd::OnDestroy()
{
    guard(NCTargetStatusWnd::OnDestroy);
    DestroyChild(this, CloseButton);
    DestroyChild(this, ToggleButton);
    DestroyChild(this, ResizeFrame);
    DestroyChild(this, NameCtrl);
    NCStatusWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1010f1f0, NCTargetStatusWnd, OnDestroy)

// the color of the name by the player's level minus the target's (the stock tests ascend from -9): red when the target
// is much stronger, blue when much weaker (spelled out at its one use)
static FORCEINLINE DWORD LevelDiffColor(INT Diff)
{
    if (Diff <= -9)
        return 0xFFFF0000;
    if (Diff <= -6)
        return 0xFFFF9191;
    if (Diff <= -3)
        return 0xFFFAFE91;
    if (Diff <= 2)
        return 0xFFDCDCDC;
    if (Diff <= 5)
        return 0xFFA2FFAB;
    if (Diff <= 8)
        return 0xFFA2A8FC;
    return 0xFF0000FF;
}

// the NPC table's record of an NPC (0x1012b460 inlined: the hash chain of the key); the C3 record of an NPC
// ends its first part with the list of the skills the target window shows as icons (names ours): a TArray<INT> at
// +0x4C (data) / +0x50 (count). The stock code does not test the result (an NPC the window shows is in the table)
static FORCEINLINE BYTE* FindNpcRecord(const INT& NpcID)   // (C4: the key goes by reference: the stock pushes the address of User +0x14)
{
    return (BYTE*)GL2GameData.NpcTable->FindRef(NpcID);
}

// 0x101132a0 (vslot 62) - C4: the window's back (three pieces, 46 high; 76 when expanded), the frame, then by what the
// target is: nothing (the window hides itself), a door or other static object (its name, and the bar of its hit points
// when it has any), a creature. A monster, the player, its own pet or summon and a siege NPC show the name colored by the
// level difference and the HP bar (the MP bar too, except for the monsters); anything else its name alone. The names are
// set on the name control now (the paint no longer draws them). A player, expanded, has the clan and the alliance
// (crests and names); an NPC, expanded, the icons of its skills (8 to a row) from the NPC table. Without a target the
// window hides itself, leaving the clip and the origin set, as in the stock code.
INT NCTargetStatusWnd::OnPaint(NCGDevice* Device)
{
    guard(NCTargetStatusWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (!bExpanded)
    {
        if (BackTex3[0])
            Device->DrawTexture(12, 0, 16, (INT)Height, 0, 0, 16, 46, BackTex3[0], 0xFF, 1);
        if (BackTex3[1])
            Device->DrawTexture(28, 0, (INT)(Width - 32.0f - 12.0f), (INT)Height, 0, 0, 16, 46, BackTex3[1], 0xFF, 1);
        if (BackTex3[2])
            Device->DrawTexture((INT)(Width - 16.0f), 0, 19, (INT)Height, 0, 0, 19, 46, BackTex3[2], 0xFF, 0);
    }
    else
    {
        if (BackTex3[3])
            Device->DrawTexture(12, 0, 16, (INT)Height, 0, 0, 16, 76, BackTex3[3], 0xFF, 1);
        if (BackTex3[4])
            Device->DrawTexture(28, 0, (INT)(Width - 32.0f - 12.0f), (INT)Height, 0, 0, 16, 76, BackTex3[4], 0xFF, 1);
        if (BackTex3[5])
            Device->DrawTexture((INT)(Width - 16.0f), 0, 19, (INT)Height, 0, 0, 19, 76, BackTex3[5], 0xFF, 0);
    }
    if (FrameTopTex)
        Device->DrawTexture(0, 0, 12, 8, 0, 0, 12, 8, FrameTopTex, 0xFF, 1);
    if (FrameCenterTex)
        Device->DrawTexture(0, 8, 12, (INT)(Height - 16.0f), 0, 0, 12, 8, FrameCenterTex, 0xFF, 1);
    if (FrameBottomTex)
        Device->DrawTexture(0, (INT)(Height - 8.0f), 12, 8, 0, 0, 12, 8, FrameBottomTex, 0xFF, 1);

    User* Target = GConsole->GetSelectedActor();
    User* Me = GConsole->GetMyUser();
    AActor* StaticActor = GConsole->GetSelectedStaticActor();
    if (!Target)
    {
        if (!StaticActor)
        {
            HideWindow();
            return 1;
        }
        // a door or other static object: its name, and (C4) the bar of its hit points
        FServerStaticObject* Info = GConsole->FindStaticObjectInfo(nw::Field<INT>(StaticActor, 0x60));
        if (Info)
        {
            TCHAR* ObjectName = GL2GameData.GetStaticObjectName(Info->StaticObjectID);
            if (ObjectName)
                SetNameText(NameCtrl, ObjectName, 0, 1, 0xFFDCDCDC);
            if (Info->Unknown0 == 1)
            {
                INT HPWidth = 0;
                if (Info->Unknown10)
                    HPWidth = (INT)((DOUBLE)BarWidth * Info->UnknownC / Info->Unknown10);
                if (HPWidth >= BarWidth)
                    HPWidth = BarWidth;
                if (HPWidth >= 0)
                {
                    Device->DrawTexture(16, 26, BarWidth, 6, 0, 0, 8, 6, HPBackTex, 0xFF, 1);
                    Device->DrawTexture(16, 26, HPWidth, 6, 0, 0, 8, 6, HPTex, 0xFF, 1);
                }
            }
        }
    }
    else
    {
        // a monster, the player, its own pet or summon, a siege NPC: the bars
        UBOOL bShowBars = 0;
        if (Target->Unknown8 && !Target->Unknown0C && Target->Unknown10)
            bShowBars = 1;
        else if (Me && Target == Me)
            bShowBars = 1;
        else if (Target->Unknown8 && Target->Unknown0C && Target->ObjectID == GConsole->GetPetInfo().ObjectID)
            bShowBars = 1;
        else if (Target->Unknown8 && ((NConsoleLeftFns*)GConsole)->NpcShowsBars(Target->NpcID))
            bShowBars = 1;   // 0x1007d850 (C3: 0x10073b90; NConsoleLeftFns::NpcShowsBars): the NPC record's flag

        if (bShowBars)
        {
            INT HPWidth = 0;
            INT MPWidth = 0;
            if (Target->MaxHP)
                HPWidth = (INT)((DOUBLE)BarWidth * Target->HP / Target->MaxHP);
            if (Target->MaxMP)
                MPWidth = (INT)((DOUBLE)BarWidth * Target->MP / Target->MaxMP);
            if (HPWidth >= BarWidth)
                HPWidth = BarWidth;
            if (MPWidth >= BarWidth)
                MPWidth = BarWidth;
            DWORD Color = LevelDiffColor(LevelDiff);
            SetNameText(NameCtrl, Target->GetName(), 0, 1, Color);
            if (HPWidth >= 0)
            {
                Device->DrawTexture(16, 26, BarWidth, 6, 0, 0, 8, 6, HPBackTex, 0xFF, 1);
                Device->DrawTexture(16, 26, HPWidth, 6, 0, 0, 8, 6, HPTex, 0xFF, 1);
            }
            // a monster has no MP bar
            if ((!Target->Unknown8 || Target->Unknown0C || !Target->Unknown10) && MPWidth >= 0)
            {
                Device->DrawTexture(16, 33, BarWidth, 6, 0, 0, 8, 6, MPBackTex, 0xFF, 1);
                Device->DrawTexture(16, 33, MPWidth, 6, 0, 0, 8, 6, MPTex, 0xFF, 1);
            }
        }
        else
            SetNameText(NameCtrl, Target->GetName(), 0, 1, 0xFFDCDCDC);

        if (Target->Unknown8)
        {
            // an NPC: the icons of its skills when expanded
            if (bExpanded)
            {
                BYTE* Npc = FindNpcRecord(Target->NpcID);
                INT Count = nw::Field<INT>(Npc, 0x50);   // (the stock does not test the record)
                for (INT i = 0; i < Count; i++)
                {
                    const TCHAR* IconName = GConsole->GetSkillIconName(nw::Field<INT*>(Npc, 0x4C)[i], 1);
                    if (IconName)
                    {
                        UTexture* Icon = FindTexture(IconName);
                        if (Icon)
                            Device->DrawStretchedTex((i % 8 + 1) * 18, i / 8 * 18 + 40, 15, 15, 0, 0, 32, 32, Icon, 0xFF);
                    }
                }
            }
        }
        else if (bExpanded)
        {
            // a player: the clan and the alliance
            Device->DrawText(20, 43, 0xFFA3A3A3, *SysString(604), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
            Device->DrawText(20, 59, 0xFFA3A3A3, *SysString(605), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
            UBOOL bCrest = 0;
            PledgeInfo* Pledge = GConsole->GetPledgeInfo(Target->PledgeID);
            if (!Pledge)
                Device->DrawText(45, 43, 0xFF808080, *SysString(431), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);   // no clan
            else
            {
                // C4 puts the crest textures of the record at +0xE0 and +0xE4 (the header's PledgeInfo is C2's)
                if (nw::Field<UTexture*>(Pledge, 0xE0))
                {
                    Device->DrawPledgeCrest(45, 43, Target->PledgeID);
                    bCrest = 1;
                }
                const TCHAR* PledgeName = Pledge->Name;
                if (PledgeName && *PledgeName)
                    Device->DrawText(bCrest ? 63 : 45, 43, 0xFFB09B79, PledgeName, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0,
                                     0);
                UBOOL bAllyCrest = 0;
                if (nw::Field<UTexture*>(Pledge, 0xE4))
                {
                    Device->DrawAllyCrest(53, 59, Target->PledgeID);
                    bAllyCrest = 1;
                }
                const TCHAR* AllyName = Pledge->AllyName;
                if (AllyName && *AllyName)
                {
                    Device->DrawText(bAllyCrest ? 63 : 45, 59, 0xFFB09B79, AllyName, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0,
                                     0, 0);
                    goto Done;
                }
            }
            Device->DrawText(45, 59, 0xFF808080, *SysString(591), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);   // no alliance
        }
    }
Done:
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x101132a0, NCTargetStatusWnd, OnPaint)

// 0x1010d5a0 (vslot 68) - targets again; with Ctrl, attacks
INT NCTargetStatusWnd::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    User* Me = GConsole->GetMyUser();
    if (Me)
    {
        FVector Origin = Me->Pawn->Location;
        if (ConsoleIsCtrlDown())
            RequestAttackC2(ObjectID, Origin);
        else
            GConsole->RequestTarget(ObjectID);
    }
    return 1;
}
NW_HOOK_METHOD(0x1010d5a0, NCTargetStatusWnd, OnLButtonDown)

// 0x10113af0 (vslot 67) - which of the NPC's skill icons (16 x 16, 8 to a row from (18, 40), 18 apart) is under the
// mouse, as 1 + its index (0: none); only for an NPC target with the window expanded. The mouse position is taken
// relative to the window in 16 bits (a position left of or above the window wraps to a large one: no hit). The base's
// (the drag of a dragable window) is not called.
INT NCTargetStatusWnd::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    User* Target = GConsole->GetSelectedActor();
    HoverBuff = 0;
    if (Target && Target->Unknown8 && bExpanded)
    {
        INT X = (WORD)((WORD)LOWORD(lParam) - (WORD)ScreenX);
        INT Y = (WORD)((WORD)HIWORD(lParam) - (WORD)ScreenY);
        BYTE* Npc = FindNpcRecord(Target->NpcID);
        INT Count = nw::Field<INT>(Npc, 0x50);   // (the stock does not test the record)
        for (INT i = 0; i < Count; i++)
        {
            INT IconX = (i % 8) * 18;
            INT IconY = i / 8 * 18;
            if (IconX + 18 <= X && X <= IconX + 33 && IconY + 40 <= Y && Y <= IconY + 55)
            {
                HoverBuff = i + 1;
                return 1;
            }
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x10113af0, NCTargetStatusWnd, OnMouseMove)

// 0x10115e70 (vslot 63) - the tool tip of the NPC skill icon under the mouse: the skill's name and description (level 1)
// in the window's tool tip skin, over the icon's place (the full box)
INT NCTargetStatusWnd::OnDrawToolTip(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    User* Target = GConsole->GetSelectedActor();
    if (Target && Target->Unknown8 && bExpanded)
    {
        BYTE* Npc = FindNpcRecord(Target->NpcID);
        if (HoverBuff > 0)   // (the stock does not test Npc)
        {
            const TCHAR* Desc = GConsole->GetSkillDesc(nw::Field<INT*>(Npc, 0x4C)[HoverBuff - 1], 1);
            const TCHAR* Name = GConsole->GetSkillName(nw::Field<INT*>(Npc, 0x4C)[HoverBuff - 1], 1);
            if (Name && Desc)
            {
                BuffToolTip.SetNameDesc(Name, Desc, NTT_Text);
                INT Index = HoverBuff - 1;
                INT X = (Index % 8 + 1) * 18;
                INT Y = Index / 8 * 18 + 40;
                if (ToolTipSkin)
                {
                    NToolTipRect Wnd;
                    Wnd.X = ScreenX;
                    Wnd.Y = ScreenY;
                    Wnd.W = Width;
                    Wnd.H = Height;
                    ToolTipSkin->DrawFull(Device, BuffToolTip, X, Y, Wnd);
                }
            }
        }
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x10115e70, NCTargetStatusWnd, OnDrawToolTip)

// 0x1010d6f0 (vslot 76) - the close button: the server drops the target (C4: the parameter list carries a 1)
INT NCTargetStatusWnd::OnClose()
{
    L2ParamStack Params(10);
    Params.PushBack((void*)1);
    GConsole->RequestTargetCancel(Params);
    GConsole->PlayInterfaceSound(SOUND_WindowClose);
    return 1;
}
NW_HOOK_METHOD(0x1010d6f0, NCTargetStatusWnd, OnClose)

// 0x1010f320 (slot 75 of the stock vtable, where the header keeps the placeholder NewVirtual75 that jumps here) - the right
// edge was dragged by Delta: the window grows or shrinks within 0 .. 202 pixels, the edge, the close button, the bar width and
// the name control follow
INT NCTargetStatusWnd::OnResizeImpl(INT Delta)
{
    NRectF Rect = {0, 0, 0.0f, 0.0f};
    GetLocalRect(&Rect);
    MaxExtraWidth = 202;
    if (!ApplyExtraWidth(ExtraWidth, MaxExtraWidth, Delta))
        return 1;
    SetWindowSize((FLOAT)Delta + Rect.W, Rect.H);
    NRectF EdgeRect = {0, 0, 0.0f, 0.0f};
    ResizeFrame->GetLocalRect(&EdgeRect);
    ResizeFrame->SetPosition((INT)(Width - EdgeRect.W), 0, (DWORD)(INT)EdgeRect.W, (DWORD)(INT)EdgeRect.H);
    CloseButton->SetPosition((INT)(Width - 19.0f), 6, 15, 15);
    NRectF NameRect = {0, 0, 0.0f, 0.0f};
    BarWidth = (INT)(Width - 22.0f);
    NameCtrl->GetLocalRect(&NameRect);
    NameCtrl->SetWindowSize((FLOAT)Delta + NameRect.W, NameRect.H);
    return 1;
}
NW_HOOK_METHOD(0x1010f320, NCTargetStatusWnd, OnResizeImpl)

// C2 CANDIDATE: the C2 function is not the same code as the C1 one; kept only while it matches the stock one (verify_data)
// 0x1010d640 (vslot 77) - doubles or halves the height for the buff icons: 46 (collapsed) or 76 (expanded)
INT NCTargetStatusWnd::OnToggle()
{
    FLOAT H;
    if (bExpanded)
    {
        ToggleButton->SetNormalTex(TEXT("L2UI_CH3.TargetWnd.Target_InfoButton"));
        ToggleButton->SetPushedTex(TEXT("L2UI_CH3.TargetWnd.Target_InfoButton"));
        H = 46.0f;
    }
    else
    {
        ToggleButton->SetNormalTex(TEXT("L2UI_CH3.TargetWnd.Target_InfoButton_Down"));
        ToggleButton->SetPushedTex(TEXT("L2UI_CH3.TargetWnd.Target_InfoButton_Down"));
        H = 76.0f;
    }
    SetWindowSize(Width, H);
    bExpanded = !bExpanded;
    FrameCtrl->SetPosition(0, 19, 12, (DWORD)(INT)(Height - 19.0f));
    return 1;
}
NW_HOOK_METHOD(0x1010d640, NCTargetStatusWnd, OnToggle)

// 0x1010d630 - from the console with each new target
void NCTargetStatusWnd::SetLevelDiff(INT Diff)
{
    LevelDiff = Diff;
}
NW_HOOK_METHOD(0x1010d630, NCTargetStatusWnd, SetLevelDiff)

NW_CHECK_VTABLE(NCTargetStatusWnd, 0x101cdd70, 78)

/*-----------------------------------------------------------------------------
    NCPartyStatusWnd.
-----------------------------------------------------------------------------*/

// 0x10117340 - C4: -1 for the member, the two buff arrays emptied, one more field cleared (+0x390)
NCPartyStatusWnd::NCPartyStatusWnd(INT InBarWidth)
    : NCStatusWnd(InBarWidth)
{
    MemberID = -1;
    BuffSkillIDs.Empty();
    BuffIcons.Empty();
    Unknown390 = 0;
}
NW_HOOK_CTOR_ARGS(0x10117340, NCPartyStatusWnd, (INT InBarWidth), (InBarWidth))

// 0x10117410 - the buff icons and skill ids go, then NCStatusWnd
NW_HOOK_DTOR(0x10117410, NCPartyStatusWnd)


// 0x10114d80 (vslot 37) - C4: the class group has one icon (its textures are changed by UpdateStyle) instead of nine,
// the small windows are 11 x 11 at (4, 8), and the member's name is a name control (new)
INT NCPartyStatusWnd::OnCreate()
{
    NCStatusWnd::OnCreate();
    LeftBackTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.SmallWindow_back1"));
    BackTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.SmallWindow_back2"));
    RightBackTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.SmallWindow_back3_ex"));
    HPTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.HpBar"));
    MPTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.MpBar"));
    CPBarTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.cpBar"));
    HPBackTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.HpBar_back"));
    MPBackTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.MpBar_back"));
    CPBarBackTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.CpBar_back"));

    StyleIcon = NewButton(TEXT("L2UI_CH3.PartyWnd.party_styleicon1"), TEXT("L2UI_CH3.PartyWnd.party_styleicon1"), 2);
    StyleIcon->SetPosition(4, 8, 11, 11);
    StyleIcon->ID = (INT)0xFF0001F2;
    CreateChild(StyleIcon, NWS_VISIBLE | 0x20);   // 0x20: meaning unknown
    StyleIcon->HideWindow();

    DebuffFrameTex = FindTexture(TEXT("L2UI_CH3.PlayerStatusWnd.debuff"));

    LeaderIcon = NewButton(TEXT("L2UI_CH3.PartyWnd.party_leadericon"), TEXT("L2UI_CH3.PartyWnd.party_leadericon"), 2);
    LeaderIcon->SetPosition(4, 8, 11, 11);
    LeaderIcon->ID = (INT)0xFF000FF2;
    CreateChild(LeaderIcon, NWS_VISIBLE | 0x20);
    LeaderIcon->HideWindow();

    NameCtrl = NewNameCtrl();
    NameCtrl->SetPosition(0, 8, (DWORD)(INT)Width, 14);
    CreateChild(NameCtrl, NWS_VISIBLE | 0x20 | 0x4);   // 0x26
    ClearNameCtrlFlag(NameCtrl);
    return 1;
}
NW_HOOK_METHOD(0x10114d80, NCPartyStatusWnd, OnCreate)

// 0x10110340 (vslot 38) - the class icon, the leader icon and the name control go, then the base's
INT NCPartyStatusWnd::OnDestroy()
{
    DestroyChild(this, StyleIcon);
    DestroyChild(this, LeaderIcon);
    DestroyChild(this, NameCtrl);
    NCStatusWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10110340, NCPartyStatusWnd, OnDestroy)

// the console's tests of a skill id (its skill data record, level 1): 0x1005f6c0 (name ours) says the
// record has its +0x48 set, which sends the icon to the second pass of the paint (framed)
static __declspec(noinline) UBOOL IsFramedBuff(INT SkillID)   // out of line as the stock's 0x1006A9B0
{
    // 0x1006A9B0 (ret 8, `this` not read; hooked as NConsoleWndSkillExt::IsDebuffSkill in NCBroadcastObserverBuffWnd.cpp): the skill
    // record of that level has its debuff flag set
    FL2MagicSkillData* Data = GL2GameData.GetMSData(SkillID, 1);
    return Data && Data->bDebuff ? 1 : 0;
}

// 0x10115080 (vslot 62) - C4: the name (the name control's), the CP / HP / MP bars (Width - 10 pixels wide at most),
// then the buffs in two rows of ten at the right (the framed ones last, over the others), when the party window's
// button has them switched on
INT NCPartyStatusWnd::OnPaint(NCGDevice* Device)
{
    guard(NCPartyStatusWnd::OnPaint);
    INT HPWidth = 0, MPWidth = 0, CPWidth = 0;
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->DrawTexture(0, 0, 16, 46, 0, 0, 16, 46, LeftBackTex, 0xFF, 1);
    Device->DrawStretchedTex(16, 0, (INT)(Width - 16.0f - 16.0f), (INT)Height, 0, 0, 16, 46, BackTex, 0xFF);
    Device->DrawTexture((INT)(Width - 16.0f), 0, 19, 46, 0, 0, 19, 46, RightBackTex, 0xFF, 0);
    if (Name && Name[0])
    {
        if (MaxHP != 0)
        {
            if (HP >= MaxHP)
                HP = MaxHP;
            HPWidth = HP * (INT)(Width - 10.0f) / MaxHP;
        }
        if (MaxMP != 0)
        {
            if (MP >= MaxMP)
                MP = MaxMP;
            MPWidth = MP * (INT)(Width - 10.0f) / MaxMP;
        }
        if (MaxCP != 0)
        {
            if (CP >= MaxCP)
                CP = MaxCP;
            CPWidth = CP * (INT)(Width - 10.0f) / MaxCP;
        }
        SetNameText(NameCtrl, Name, 0, 1, 0xFFDCDCDC);
        Device->DrawTexture(4, 21, (INT)(Width - 10.0f), 6, 0, 0, 8, 6, CPBarBackTex, 0xFF, 1);
        if (CPWidth > 0)
            Device->DrawTexture(4, 21, CPWidth, 5, 0, 0, 8, 6, CPBarTex, 0xFF, 1);
        Device->DrawTexture(4, 28, (INT)(Width - 10.0f), 6, 0, 0, 8, 6, HPBackTex, 0xFF, 1);
        if (HPWidth > 0)
            Device->DrawTexture(4, 28, HPWidth, 5, 0, 0, 8, 6, HPTex, 0xFF, 1);
        Device->DrawTexture(4, 35, (INT)(Width - 10.0f), 6, 0, 0, 8, 6, MPBackTex, 0xFF, 1);
        if (MPWidth > 0)
            Device->DrawTexture(4, 35, MPWidth, 5, 0, 0, 8, 6, MPTex, 0xFF, 1);
        Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
        if (((NCPartyWnd*)Entry->Parent->Wnd)->bToggle)
        {
            TArray<INT> XPos;
            TArray<INT> YPos;
            INT i = 0;
            while (i < BuffIcons.Num())
            {
                if (i < 10)
                {
                    if (!IsFramedBuff(BuffSkillIDs(i)))
                        Device->DrawTexture((INT)((FLOAT)(i << 4) + Width + 1.0f), 5, 16, 16, 0, 0, 32, 32,
                                            BuffIcons(i), 0xFF, 0);
                    else
                    {
                        XPos.AddItem((INT)((FLOAT)(i << 4) + Width + 1.0f));
                        YPos.AddItem(5);
                    }
                }
                else if (i < 20)
                {
                    if (!IsFramedBuff(BuffSkillIDs(i)))
                        Device->DrawTexture((INT)((FLOAT)((i - 10) << 4) + Width + 1.0f), 21, 16, 16, 0, 0, 32,
                                            32, BuffIcons(i), 0xFF, 0);
                    else
                    {
                        XPos.AddItem((INT)((FLOAT)((i - 10) << 4) + Width + 1.0f));
                        YPos.AddItem(21);
                    }
                }
                i++;
            }
            INT k = 0;
            for (i = 0; i < BuffIcons.Num(); i++)
            {
                if (IsFramedBuff(BuffSkillIDs(i)))
                {
                    Device->DrawTexture(XPos(k), YPos(k), 16, 16, 0, 0, 32, 32, BuffIcons(i), 0xFF, 0);
                    Device->DrawTexture(XPos(k), YPos(k), 16, 16, 0, 0, 26, 26, DebuffFrameTex, 0xFF, 0);
                    k++;
                }
            }
        }
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10115080, NCPartyStatusWnd, OnPaint)

// 0x1010daa0 (vslot 68) - targets the member; with Ctrl, attacks it (C4: not on the last 10 pixels at the right)
INT NCPartyStatusWnd::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    if ((FLOAT)(SWORD)LOWORD(lParam) > (FLOAT)ScreenX + Width - 10.0f)
        return 0;
    User* Me = GConsole->GetMyUser();
    if (Me)
    {
        FVector Origin = Me->Pawn->Location;
        if (ConsoleIsCtrlDown())
            RequestAttackC2(MemberID, Origin);
        else
            GConsole->RequestTarget(MemberID);
    }
    return 1;
}
NW_HOOK_METHOD(0x1010daa0, NCPartyStatusWnd, OnLButtonDown)

// 0x1010db60 (vslot 71) - targets what the member targets (signed coordinates here)
INT NCPartyStatusWnd::OnRButtonUp(WPARAM wParam, LPARAM lParam)
{
    if (PtInRect((SWORD)LOWORD(lParam), (SWORD)HIWORD(lParam), Rect))
    {
        User* Me = GConsole->GetMyUser();
        if (Me)
            GConsole->AssistMember(MemberID, Me->Pawn->Location);
    }
    return 1;
}
NW_HOOK_METHOD(0x1010db60, NCPartyStatusWnd, OnRButtonUp)

// 0x101103e0 (ret 4; sub_101103e0 in the db) - a member joins or changes: C4 pops 14 values from the packet here (C3: the
// caller popped them). Leader, member, loot rule, name, CP and its maximum, HP, its maximum, MP, its maximum, one that
// is dropped, the class, two that are dropped; a record without a name changes nothing
void NCPartyStatusWnd::SetInfo(L2ParamStack& Params)
{
    INT InLeaderID = (INT)Params.Top();
    INT InMemberID = (INT)Params.Top();
    INT InLootRule = (INT)Params.Top();
    TCHAR* InName = (TCHAR*)Params.Top();
    INT InCP = (INT)Params.Top();
    INT InMaxCP = (INT)Params.Top();
    INT InHP = (INT)Params.Top();
    INT InMaxHP = (INT)Params.Top();
    INT InMP = (INT)Params.Top();
    INT InMaxMP = (INT)Params.Top();
    Params.Top();
    INT InClassID = (INT)Params.Top();
    Params.Top();
    Params.Top();
    if (InName)
    {
        MemberID = InMemberID;
        wcscpy(Name, InName);
        MP = InMP;
        HP = InHP;
        MaxHP = InMaxHP;
        CP = InCP;
        MaxMP = InMaxMP;
        ClassID = InClassID;
        LootRule = InLootRule;
        MaxCP = InMaxCP;
        LeaderID = InLeaderID;
        UpdateStyle();
    }
}
NW_HOOK_METHOD(0x101103e0, NCPartyStatusWnd, SetInfo)

// 0x1010dc00 (name ours) - C4: the one class icon gets the textures of the member's class group (tool tip
// "<group> - <class>"), the leader's icon sits right of the name while the member leads (tool tip: the loot rule)
__declspec(noinline) void NCPartyStatusWnd::UpdateStyle()
{
    if (Name[0] == 0)
        return;
    TCHAR Tip[512];
    TCHAR Rule[512];
    ((NConsoleLeftFns*)GConsole)->GetClassGroup(ClassID);   // 0x1006B970, the class group; not used
    // sub_10074c70 (ret 4): the name of the group's icon texture; sub_10001630 (ret 0xC): the button's textures
    UTexture* IconA = FindTexture(PartyStyleIconName(ClassID));
    UTexture* IconB = FindTexture(PartyStyleIconName(ClassID));
    ((NCButtonTexturesView*)StyleIcon)->SetTextures(IconB, IconA, 0);
    appSprintf(Tip, TEXT("%s - %s"), ClassGroupNameImpl(NULL, NULL, ClassID),   // 0x10074320
               GConsole->GetClassNameString(ClassID));
    StyleIcon->SetToolTipText(Tip, 1);
    SIZE Size;
    NCGDevice::GetTextExtent(&Size, Name, 0, 0, 0, 0, 0);
    INT X = (INT)(Width * 0.5f - Size.cx / 2 - 11.0f);
    if (MemberID == LeaderID)
    {
        LeaderIcon->ShowWindow();
        LeaderIcon->SetPosition(X, 8, 0, 0);
    }
    else
        LeaderIcon->HideWindow();
    Rule[0] = 0;
    switch (LootRule)
    {
    case 0:
        appSprintf(Rule, TEXT("%s : %s"), *GL2GameData.SysString[489], *GL2GameData.SysString[487]);
        break;
    case 1:
        appSprintf(Rule, TEXT("%s : %s"), *GL2GameData.SysString[489], *GL2GameData.SysString[488]);
        break;
    case 2:
        appSprintf(Rule, TEXT("%s : %s"), *GL2GameData.SysString[489], *GL2GameData.SysString[798]);
        break;
    case 3:
        appSprintf(Rule, TEXT("%s : %s"), *GL2GameData.SysString[489], *GL2GameData.SysString[799]);
        break;
    case 4:
        appSprintf(Rule, TEXT("%s : %s"), *GL2GameData.SysString[489], *GL2GameData.SysString[800]);
        break;
    }
    LeaderIcon->SetToolTipText(Rule, 1);
}
NW_HOOK_METHOD(0x1010dc00, NCPartyStatusWnd, UpdateStyle)

// 0x10115620 (vslot 74) - the base's clearing, then the buffs
void NCPartyStatusWnd::Clear()
{
    NCStatusWnd::Clear();
    ClearBuffs();
}
NW_HOOK_METHOD(0x10115620, NCPartyStatusWnd, Clear)

// 0x101155e0 (sub_101155e0 in the db; the pet window's OnDestroy and the console call it too) - the buffs forgotten
__declspec(noinline) void NCPartyStatusWnd::ClearBuffs()
{
    BuffSkillIDs.Empty();
    BuffIcons.Empty();
}
NW_HOOK_METHOD(0x101155e0, NCPartyStatusWnd, ClearBuffs)

// 0x101147f0 (name ours; ret 8: C4 passes a second value that this body does not read) - a buff of the member: its skill
// id and the icon texture of its skill
void NCPartyStatusWnd::AddBuff(INT SkillID, INT Unused)
{
    BuffSkillIDs.AddItem(SkillID);
    BuffIcons.AddItem(FindTexture(GConsole->GetSkillIconName(SkillID, 1)));
}
NW_HOOK_METHOD(0x101147f0, NCPartyStatusWnd, AddBuff)

NW_CHECK_VTABLE(NCStatusWnd, 0x101cd9e0, 75)
NW_CHECK_VTABLE(NCPartyStatusWnd, 0x101ce5d0, 75)

/*-----------------------------------------------------------------------------
    NCPetStatusWnd.
-----------------------------------------------------------------------------*/

// 0x10117080 - C4: the two buff arrays emptied, the record's id -1, its name and two numbers cleared, ExtraWidth too
NCPetStatusWnd::NCPetStatusWnd(INT InBarWidth)
    : NCStatusWnd(InBarWidth)
{
    Pet.Name[0] = 0;
    Pet.UnknownB0 = 0;
    Pet.ObjectID = -1;
    Pet.UnknownA8 = 0;
    Unknown410 = 0;
    BuffSkillIDs.Empty();
    BuffIcons.Empty();
    ExtraWidth = 0;
}
NW_HOOK_CTOR_ARGS(0x10117080, NCPetStatusWnd, (INT InBarWidth), (InBarWidth))

// 0x10117170 - the buff icons and skill ids go, then NCStatusWnd
NW_HOOK_DTOR(0x10117170, NCPetStatusWnd)


// 0x1010d7f0 (vslot 25) - the buff button, and (C4) the right edge
INT NCPetStatusWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCStatusWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if ((INT)wParam == NID_PETSTATUS_TOGGLE)
            return OnToggle() != 0;
        if ((INT)wParam == NID_PETSTATUS_RESIZE)
            return ::nw::VirtualCall<INT>(this, 75, (INT)lParam) != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x1010d7f0, NCPetStatusWnd, WindowProc)

// the stock code calls NCPartyStatusWnd's ClearBuffs (0x101155e0, hooked there): the same two arrays at +0x368 / +0x374
__declspec(noinline) void NCPetStatusWnd::ClearBuffs()
{
    BuffSkillIDs.Empty();
    BuffIcons.Empty();
}

// 0x1010d990 (vslot 69; name ours) - the buff icons on / off
INT NCPetStatusWnd::OnToggle()
{
    bBuffs = !bBuffs;
    return 1;
}
NW_HOOK_METHOD(0x1010d990, NCPetStatusWnd, OnToggle)

// 0x10114400 (vslot 37) - C4: the bars follow the window's width (it can be widened), the right edge and the name
// control are new, the back of the tall window ends in "_ex"
INT NCPetStatusWnd::OnCreate()
{
    NCStatusWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.smallwindow_back2"));
    LeftBackTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.smallwindow_back1"));
    RightBackTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.smallwindow_back3_ex"));
    HPTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.HPBAR"));
    MPTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.MPBAR"));
    HPBackTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.HPBAR_BACK"));
    MPBackTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.MPBAR_BACK"));
    FedTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.FATIGUEBAR"));
    FedBackTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.FATIGUEBAR_BACK"));
    PetFrameTopTex = FindTexture(TEXT("L2UI_CH3.FrameCtrl.smallbar1"));
    PetFrameCenterTex = FindTexture(TEXT("L2UI_CH3.FrameCtrl.smallbar2"));
    PetFrameBottomTex = FindTexture(TEXT("L2UI_CH3.FrameCtrl.smallbar3"));
    BarWidth = (INT)(Width - 22.0f);

    ResizeFrame = NewResizeFrame(1);
    ResizeFrame->ID = NID_PETSTATUS_RESIZE;
    ResizeFrame->SetPosition((INT)(Width - 10.0f), 0, 10, (DWORD)(INT)Height);
    CreateChild(ResizeFrame, NWS_VISIBLE | 0x20);   // 0x20: meaning unknown

    FrameCtrl = NewFrameCtrl(0, 0, 0);
    FrameCtrl->SetPosition(0, 17, 12, (DWORD)(INT)Height);
    FrameCtrl->ID = NID_PETSTATUS_FRAME;
    CreateChild(FrameCtrl, NWS_VISIBLE);

    ToggleButton = NewButton(TEXT("L2UI_CH3.Etc.pet_button"), TEXT("L2UI_CH3.Etc.pet_button_down"), 0);
    ToggleButton->SetPosition(1, 6, 11, 13);
    ToggleButton->ID = NID_PETSTATUS_TOGGLE;
    CreateChild(ToggleButton, NWS_VISIBLE | 0x20);
    ToggleButton->SetToolTip(881, 1);

    NameCtrl = NewNameCtrl();
    NameCtrl->SetPosition(12, 6, (DWORD)(INT)(Width - 12.0f), 14);
    CreateChild(NameCtrl, NWS_VISIBLE | 0x20 | 0x4);   // 0x26
    ClearNameCtrlFlag(NameCtrl);
    return 1;
}
NW_HOOK_METHOD(0x10114400, NCPetStatusWnd, OnCreate)

// 0x10115ff0 (vslot 38) - the buffs forgotten, the buff button, the name control and the right edge go, then the base's
INT NCPetStatusWnd::OnDestroy()
{
    guard(NCPetStatusWnd::OnDestroy);
    ClearBuffs();
    DestroyChild(this, ToggleButton);
    DestroyChild(this, NameCtrl);
    DestroyChild(this, ResizeFrame);
    NCStatusWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10115ff0, NCPetStatusWnd, OnDestroy)

// 0x1010fb80 (slot 75 of the stock vtable, see NCTargetStatusWnd::OnResizeImpl) - the right edge was dragged by Delta
INT NCPetStatusWnd::OnResizeImpl(INT Delta)
{
    NRectF Rect = {0, 0, 0.0f, 0.0f};
    GetLocalRect(&Rect);
    MaxExtraWidth = 202;
    if (!ApplyExtraWidth(ExtraWidth, MaxExtraWidth, Delta))
        return 1;
    SetWindowSize((FLOAT)Delta + Rect.W, Rect.H);
    NRectF EdgeRect = {0, 0, 0.0f, 0.0f};
    ResizeFrame->GetLocalRect(&EdgeRect);
    ResizeFrame->SetPosition((INT)(Width - EdgeRect.W), 0, (DWORD)(INT)EdgeRect.W, (DWORD)(INT)EdgeRect.H);
    BarWidth = (INT)(Width - 22.0f);
    NRectF NameRect = {0, 0, 0.0f, 0.0f};
    NameCtrl->GetLocalRect(&NameRect);
    NameCtrl->SetWindowSize((FLOAT)Delta + NameRect.W, NameRect.H);
    return 1;
}
NW_HOOK_METHOD(0x1010fb80, NCPetStatusWnd, OnResizeImpl)

// 0x1010d950 (vslot 48) - hidden again unless the player has a pet (C2 also hides it while
// the record has no object id)
void NCPetStatusWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    PetInfo Info = GConsole->GetPetInfo();
    if (Info.ObjectID == -1 || Info.Type != 2)
        HideWindow();
}
NW_HOOK_METHOD(0x1010d950, NCPetStatusWnd, ShowWindow)

// 0x1010f770 (vslot 62) - C4: the frame on the left, the window's three-part background (the right piece is 19 wide
// now), the name (the name control's), the HP / MP / fatigue bars (BarWidth pixels, the record read again) and the buff
// icons in two rows of ten
INT NCPetStatusWnd::OnPaint(NCGDevice* Device)
{
    guard(NCPetStatusWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->DrawTexture(0, 0, 12, 8, 0, 0, 12, 8, PetFrameTopTex, 0xFF, 1);
    Device->DrawTexture(0, 8, 12, (INT)(Height - 16.0f), 0, 0, 12, 8, PetFrameCenterTex, 0xFF, 1);
    Device->DrawTexture(0, (INT)(Height - 8.0f), 12, 8, 0, 0, 12, 8, PetFrameBottomTex, 0xFF, 1);
    INT H = (INT)Height;
    Device->DrawTexture(12, 0, 16, H, 0, 0, 16, H, LeftBackTex, 0xFF, 1);
    Device->DrawStretchedTex(28, 0, (INT)(Width - 16.0f - 16.0f - 12.0f), (INT)Height, 0, 0, 16, (INT)Height,
                             BackTex, 0xFF);
    INT H2 = (INT)Height;
    Device->DrawTexture((INT)(Width - 16.0f), 0, 19, H2, 0, 0, 19, H2, RightBackTex, 0xFF, 0);
    Pet = GConsole->GetPetInfo();
    INT HPWidth = 0, MPWidth = 0, FedWidth = 0;
    if (Pet.MaxHP != 0)
        HPWidth = Pet.HP * BarWidth / Pet.MaxHP;
    if (Pet.MaxMP != 0)
        MPWidth = Pet.MP * BarWidth / Pet.MaxMP;
    if (Pet.MaxFed != 0)
        FedWidth = Pet.Fed * BarWidth / Pet.MaxFed;
    if (Pet.Name[0])
        SetNameText(NameCtrl, Pet.Name, 0, 1, 0xFFDCDCDC);
    Device->DrawTexture(16, 19, BarWidth, 5, 0, 0, 8, 6, HPBackTex, 0xFF, 1);
    if (HPWidth > 0)
        Device->DrawTexture(16, 19, HPWidth, 6, 0, 0, 8, 6, HPTex, 0xFF, 1);
    Device->DrawTexture(16, 26, BarWidth, 5, 0, 0, 8, 6, MPBackTex, 0xFF, 1);
    if (MPWidth > 0)
        Device->DrawTexture(16, 26, MPWidth, 6, 0, 0, 8, 6, MPTex, 0xFF, 1);
    Device->DrawTexture(16, 33, BarWidth, 5, 0, 0, 8, 6, FedBackTex, 0xFF, 1);
    if (FedWidth > 0)
        Device->DrawTexture(16, 33, FedWidth, 6, 0, 0, 8, 6, FedTex, 0xFF, 1);
    if (bBuffs)
    {
        for (INT i = 0; i < BuffIcons.Num(); i++)
        {
            if (i < 10)
                Device->DrawStretchedTex((INT)((FLOAT)(i << 4) + Width + 1.0f), 5, 16, 16, BuffIcons(i), 0, 0xFF);
            else if (i < 20)
                Device->DrawStretchedTex((INT)((FLOAT)((i - 10) << 4) + Width + 1.0f), 21, 16, 16, BuffIcons(i), 0,
                                         0xFF);
        }
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1010f770, NCPetStatusWnd, OnPaint)

// 0x1010d8f0 (vslot 68) - right of the title bar: acts on the pet (as a click on it)
INT NCPetStatusWnd::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    if ((SWORD)LOWORD(lParam) > ScreenX + 13)
    {
        User* Me = GConsole->GetMyUser();
        if (Me)
            GConsole->RequestAction(Pet.ObjectID, Me->Pawn->Location);
    }
    return 1;
}
NW_HOOK_METHOD(0x1010d8f0, NCPetStatusWnd, OnLButtonDown)

// 0x1010d870 - from the console (vslot 171); the paint reads the network handler's record instead of these
void NCPetStatusWnd::SetInfo(L2ParamStack& Params)
{
    ObjectID = (INT)Params.Top();
    TCHAR* InName = (TCHAR*)Params.Top();
    if (InName)
        wcscpy(Name, InName);
    HP = (INT)Params.Top();
    MaxHP = (INT)Params.Top();
    MP = (INT)Params.Top();
    MaxMP = (INT)Params.Top();
    Unknown41C = (INT)Params.Top();
    Unknown420 = (INT)Params.Top();
    Unknown424 = (INT)Params.Top();
}
NW_HOOK_METHOD(0x1010d870, NCPetStatusWnd, SetInfo)

NW_CHECK_VTABLE(NCPetStatusWnd, 0x101ce360, 77)

/*-----------------------------------------------------------------------------
    NCSummonedStatusWnd.
-----------------------------------------------------------------------------*/

// 0x101171e0 - C4: the record's id -1, its name and two numbers cleared, Unknown450, the buff arrays emptied, ExtraWidth
NCSummonedStatusWnd::NCSummonedStatusWnd(INT InBarWidth)
    : NCStatusWnd(InBarWidth)
{
    Summon.Name[0] = 0;
    Summon.UnknownB0 = 0;
    Summon.ObjectID = -1;
    Summon.UnknownA8 = 0;
    Unknown450 = 0;
    BuffSkillIDs.Empty();
    BuffIcons.Empty();
    ExtraWidth = 0;
}
NW_HOOK_CTOR_ARGS(0x101171e0, NCSummonedStatusWnd, (INT InBarWidth), (InBarWidth))

// 0x101172d0 - the buff icons and skill ids go, then NCStatusWnd
NW_HOOK_DTOR(0x101172d0, NCSummonedStatusWnd)


// 0x1010d9b0 (vslot 25) - the buff button, and (C4) the right edge
INT NCSummonedStatusWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCStatusWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if ((INT)wParam == NID_SUMMONSTATUS_TOGGLE)
            return OnToggle() != 0;
        if ((INT)wParam == NID_SUMMONSTATUS_RESIZE)
            return ::nw::VirtualCall<INT>(this, 75, (INT)lParam) != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x1010d9b0, NCSummonedStatusWnd, WindowProc)

// 0x1010da30 (vslot 69; name ours) - the buff icons on / off
INT NCSummonedStatusWnd::OnToggle()
{
    bBuffs = !bBuffs;
    return 1;
}
NW_HOOK_METHOD(0x1010da30, NCSummonedStatusWnd, OnToggle)

// 0x10114850 (vslot 37) - C4: like the pet's, with the time bar (the fatigue bar's textures, 10000 at most) in place of
// the food bar
INT NCSummonedStatusWnd::OnCreate()
{
    NCStatusWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.smallwindow_back2"));
    LeftBackTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.smallwindow_back1"));
    RightBackTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.smallwindow_back3_ex"));
    HPTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.HPBAR"));
    MPTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.MPBAR"));
    HPBackTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.HPBAR_BACK"));
    MPBackTex = FindTexture(TEXT("L2UI_CH3.SmallWnd.MPBAR_BACK"));
    SummonFrameTopTex = FindTexture(TEXT("L2UI_CH3.FrameCtrl.smallbar1"));
    SummonFrameCenterTex = FindTexture(TEXT("L2UI_CH3.FrameCtrl.smallbar2"));
    SummonFrameBottomTex = FindTexture(TEXT("L2UI_CH3.FrameCtrl.smallbar3"));

    ResizeFrame = NewResizeFrame(1);
    ResizeFrame->ID = NID_SUMMONSTATUS_RESIZE;
    ResizeFrame->SetPosition((INT)(Width - 10.0f), 0, 10, (DWORD)(INT)Height);
    CreateChild(ResizeFrame, NWS_VISIBLE | 0x20);   // 0x20: meaning unknown

    FrameCtrl = NewFrameCtrl(0, 0, 0);
    FrameCtrl->SetPosition(0, 17, 12, (DWORD)(INT)Height);
    FrameCtrl->ID = NID_SUMMONSTATUS_FRAME;
    CreateChild(FrameCtrl, NWS_VISIBLE);

    TimeBar = NewProgressCtrl(TEXT("L2UI_CH3.SmallWnd.FATIGUEBAR_BACK"), TEXT("L2UI_CH3.SmallWnd.FATIGUEBAR"), 0);
    TimeBar->ID = NID_SUMMONSTATUS_TIME;
    TimeBar->SetPosition(16, 33, 150, 6);
    CreateChild(TimeBar, NWS_VISIBLE);
    TimeBar->SetMax(10000);
    TimeBar->Reset();

    ToggleButton = NewButton(TEXT("L2UI_CH3.Etc.pet_button"), TEXT("L2UI_CH3.Etc.pet_button_down"), 0);
    ToggleButton->SetPosition(1, 6, 11, 13);
    ToggleButton->ID = NID_SUMMONSTATUS_TOGGLE;
    CreateChild(ToggleButton, NWS_VISIBLE | 0x20);
    ToggleButton->SetToolTip(881, 1);

    NameCtrl = NewNameCtrl();
    NameCtrl->SetPosition(12, 6, (DWORD)(INT)(Width - 12.0f), 14);
    CreateChild(NameCtrl, NWS_VISIBLE | 0x20 | 0x4);   // 0x26
    ClearNameCtrlFlag(NameCtrl);
    BarWidth = (INT)(Width - 22.0f);
    return 1;
}
NW_HOOK_METHOD(0x10114850, NCSummonedStatusWnd, OnCreate)

// 0x10114ce0 (name ours) - the buffs forgotten; out of line like the stock code
__declspec(noinline) void NCSummonedStatusWnd::ClearBuffs()
{
    BuffSkillIDs.Empty();
    BuffIcons.Empty();
}
NW_HOOK_METHOD(0x10114ce0, NCSummonedStatusWnd, ClearBuffs)

// 0x10114d20 (name ours; ret 8, the second value is not read) - a buff of the summon: its skill id and the icon texture
// of its skill
void NCSummonedStatusWnd::AddBuff(INT SkillID, INT Unused)
{
    BuffSkillIDs.AddItem(SkillID);
    BuffIcons.AddItem(FindTexture(GConsole->GetSkillIconName(SkillID, 1)));
}
NW_HOOK_METHOD(0x10114d20, NCSummonedStatusWnd, AddBuff)

// 0x101160f0 (vslot 38) - the buffs forgotten, the time bar, the buff button, the right edge and the name control go,
// then the base's
INT NCSummonedStatusWnd::OnDestroy()
{
    guard(NCSummonedStatusWnd::OnDestroy);
    ClearBuffs();
    DestroyChild(this, TimeBar);
    DestroyChild(this, ToggleButton);
    DestroyChild(this, ResizeFrame);
    DestroyChild(this, NameCtrl);
    NCStatusWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x101160f0, NCSummonedStatusWnd, OnDestroy)

// 0x10110180 (slot 75 of the stock vtable, see NCTargetStatusWnd::OnResizeImpl) - the right edge was dragged by Delta
INT NCSummonedStatusWnd::OnResizeImpl(INT Delta)
{
    NRectF Rect = {0, 0, 0.0f, 0.0f};
    GetLocalRect(&Rect);
    MaxExtraWidth = 202;
    if (!ApplyExtraWidth(ExtraWidth, MaxExtraWidth, Delta))
        return 1;
    SetWindowSize((FLOAT)Delta + Rect.W, Rect.H);
    NRectF EdgeRect = {0, 0, 0.0f, 0.0f};
    ResizeFrame->GetLocalRect(&EdgeRect);
    ResizeFrame->SetPosition((INT)(Width - EdgeRect.W), 0, (DWORD)(INT)EdgeRect.W, (DWORD)(INT)EdgeRect.H);
    NRectF BarRect = {0, 0, 0.0f, 0.0f};
    TimeBar->GetLocalRect(&BarRect);
    TimeBar->SetWindowSize((FLOAT)Delta + BarRect.W, 6.0f);
    NRectF NameRect = {0, 0, 0.0f, 0.0f};
    BarWidth = (INT)(Width - 22.0f);
    NameCtrl->GetLocalRect(&NameRect);
    NameCtrl->SetWindowSize((FLOAT)Delta + NameRect.W, NameRect.H);
    return 1;
}
NW_HOOK_METHOD(0x10110180, NCSummonedStatusWnd, OnResizeImpl)

// 0x1010da60 (vslot 48) - hidden again unless the player has a summon (C2 also hides it while
// the record has no object id)
void NCSummonedStatusWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    PetInfo Info = GConsole->GetPetInfo();
    if (Info.ObjectID == -1 || Info.Type != 1)
        HideWindow();
}
NW_HOOK_METHOD(0x1010da60, NCSummonedStatusWnd, ShowWindow)

// 0x1010fdf0 (vslot 62) - C4: like the pet's, the background 46 high (the right piece 19 wide), the bars 5 high and
// BarWidth pixels wide; the time bar is a child, the name the name control's
INT NCSummonedStatusWnd::OnPaint(NCGDevice* Device)
{
    guard(NCSummonedStatusWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->DrawTexture(0, 0, 12, 8, 0, 0, 12, 8, SummonFrameTopTex, 0xFF, 1);
    Device->DrawTexture(0, 8, 12, (INT)(Height - 16.0f), 0, 0, 12, 8, SummonFrameCenterTex, 0xFF, 1);
    Device->DrawTexture(0, (INT)(Height - 8.0f), 12, 8, 0, 0, 12, 8, SummonFrameBottomTex, 0xFF, 1);
    Device->DrawTexture(12, 0, 16, 46, 0, 0, 16, 46, LeftBackTex, 0xFF, 1);
    Device->DrawStretchedTex(28, 0, (INT)(Width - 16.0f - 16.0f - 12.0f), (INT)Height, 0, 0, 16, 46, BackTex, 0xFF);
    Device->DrawTexture((INT)(Width - 16.0f), 0, 19, 46, 0, 0, 19, 46, RightBackTex, 0xFF, 0);
    Summon = GConsole->GetPetInfo();
    INT HPWidth = 0, MPWidth = 0;
    if (Summon.MaxHP != 0)
        HPWidth = Summon.HP * BarWidth / Summon.MaxHP;
    if (Summon.MaxMP != 0)
        MPWidth = Summon.MP * BarWidth / Summon.MaxMP;
    if (Summon.Name[0])
        SetNameText(NameCtrl, Summon.Name, 0, 1, 0xFFDCDCDC);
    Device->DrawTexture(16, 19, BarWidth, 5, 0, 0, 8, 6, HPBackTex, 0xFF, 1);
    if (HPWidth > 0)
        Device->DrawTexture(16, 19, HPWidth, 5, 0, 0, 8, 6, HPTex, 0xFF, 1);
    Device->DrawTexture(16, 26, BarWidth, 5, 0, 0, 8, 6, MPBackTex, 0xFF, 1);
    if (MPWidth > 0)
        Device->DrawTexture(16, 26, MPWidth, 5, 0, 0, 8, 6, MPTex, 0xFF, 1);
    if (bBuffs)
    {
        for (INT i = 0; i < BuffIcons.Num(); i++)
        {
            if (i < 10)
                Device->DrawStretchedTex((INT)((FLOAT)(i << 4) + Width + 1.0f), 5, 16, 16, BuffIcons(i), 0, 0xFF);
            else if (i < 20)
                Device->DrawStretchedTex((INT)((FLOAT)((i - 10) << 4) + Width + 1.0f), 21, 16, 16, BuffIcons(i), 0,
                                         0xFF);
        }
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1010fdf0, NCSummonedStatusWnd, OnPaint)

// 0x100fabd0 (vslot 62) - stock body shared with NCPetStatusWnd's (the linker folded them: the record sits at +0x364
// in both, and that one's hook is the address's): right of the title bar, acts on the summon
INT NCSummonedStatusWnd::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    if ((SWORD)LOWORD(lParam) > ScreenX + 13)
    {
        User* Me = GConsole->GetMyUser();
        if (Me)
            GConsole->RequestAction(Summon.ObjectID, Me->Pawn->Location);
    }
    return 1;
}

// 0x1010da50 - from the console (vslot 229) once the timer runs
void NCSummonedStatusWnd::SetRemaining(DWORD Remaining)
{
    TimeBar->SetRemaining(Remaining);
}
NW_HOOK_METHOD(0x1010da50, NCSummonedStatusWnd, SetRemaining)

// 0x10116220 - the buffs are forgotten, the bar runs for Max
void NCSummonedStatusWnd::StartTimer(DWORD Max)
{
    ClearBuffs();
    bTimerStarted = 1;
    TimeBar->SetMax(Max);
    TimeBar->Start();
}
NW_HOOK_METHOD(0x10116220, NCSummonedStatusWnd, StartTimer)

NW_CHECK_VTABLE(NCSummonedStatusWnd, 0x101ce498, 77)
