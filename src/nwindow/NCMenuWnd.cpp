/*=============================================================================
    NCMenuWnd.cpp - the main menu bar. Stock: 0x10076db0 .. 0x10076ef5, 0x10077ab0 ..
    0x10077f67, plus OnDestroy 0x100367a0 (the body NCActionWnd and NCServerSelectWnd share).
=============================================================================*/
#include "nwindow/NCMenuWnd.h"
#if NW_CUSTOM
#include "nwindow/NCustomShortCuts.h"
#endif
#include "nwindow/NCConsole.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

// 0x100b8370
NCMenuWnd::NCMenuWnd()
{
    BackTex = NULL;
    SystemButton = NULL;
    InventoryButton = NULL;
    CharacterButton = NULL;
    CommunityButton = NULL;
}
NW_HOOK_CTOR(0x100b8370, NCMenuWnd)

// 0x100b83e0
NW_HOOK_DTOR(0x100b83e0, NCMenuWnd)

// 0x100b6920 (vslot 25) - C3: the Community button's handler is virtual slot 71 (NewVirtual71 here, whose body is
// OnCommunity: it always returns 1)
INT NCMenuWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_MENU_INVENTORY)
            return OnInventory() != 0;
        if (wParam == NID_MENU_CHARACTER)
            return OnCharacter() != 0;
        if (wParam == NID_MENU_SYSTEM)
            return OnSystem() != 0;
        if (wParam == NID_MENU_COMMUNITY)
        {
            NewVirtual77();
            return 1;
        }
#if NW_CUSTOM
        if (NCustomShortCuts::OnMenuCommand(wParam))   // custom: the bars' settings and the automatic potions
            return 1;
#endif
    }
    return 0;
}
NW_HOOK_METHOD(0x100b6920, NCMenuWnd, WindowProc)

// one icon of the bar (repeated inline in the stock OnCreate)
static FORCEINLINE NCButton* CreateMenuIcon(NCWnd* Parent, const TCHAR* NormalTexName,
                                const TCHAR* PushedTexName, INT ID, INT X, INT ToolTipID)
{
    NCButton* Button = new NCButton(NormalTexName, PushedTexName, 0);
    Button->ID = ID;
    Button->SetPosition(X, 6, 34, 34);   // C3: y 6 (C2: 5)
    Parent->CreateChild(Button, NWS_VISIBLE | 0x20);   // 0x20: meaning unknown
    Button->SetToolTip(ToolTipID, 1);
    return Button;
}

// 0x100ab3c0 (vslot 37) - NCWnd::OnCreate is not called. C3: three textures of the small window skin, a bar of 12 x 46
// (the frame control takes three arguments now: the stock constructor 0x10010450 is called) and four 34 x 34 icons at
// y 6, each with its tool tip; after each one the stock clears a field of the CHARACTER button (+0x298: the source
// names the first button every time)
static NCFrameCtrl* NewMenuFrame()
{
    return new NCFrameCtrl(1, 1, 1);   // the constructor 0x100126E0 on the object `new` allocates
}

// 0x100b7de0 (vslot 37) - NCWnd::OnCreate is not called. Three textures of the small window skin (looked up with the second
// argument in C4), a bar of 12 x 46 and four 34 x 34 icons at y 6, each with its tool tip; after each one the stock clears the
// click-sound flag of the CHARACTER button (+0x2B0: the source names the first button every time)
INT NCMenuWnd::OnCreate()
{
    BackTex = FindTexture(TEXT("L2UI_ch3.SmallWnd.Smallwindow_back2"), 1);
    BackLeftTex = FindTexture(TEXT("L2UI_ch3.SmallWnd.Smallwindow_back1"), 1);
    BackRightTex = FindTexture(TEXT("L2UI_ch3.SmallWnd.Smallwindow_back3"), 1);
    FrameCtrl = NewMenuFrame();
    FrameID = NID_MENU_FRAME;
    FrameCtrl->ID = NID_MENU_FRAME;
    FrameCtrl->SetPosition(0, 0, 12, 46);
    CreateChild(FrameCtrl, NWS_VISIBLE);

    CharacterButton = CreateMenuIcon(this, TEXT("L2UI_ch3.MenuIcon.MenuButton1"),
                                     TEXT("L2UI_ch3.MenuIcon.MenuButton1_down"), NID_MENU_CHARACTER, 19, 0x1B2);
    CharacterButton->bClickSound = 0;
    InventoryButton = CreateMenuIcon(this, TEXT("L2UI_ch3.MenuIcon.MenuButton2"),
                                     TEXT("L2UI_ch3.MenuIcon.MenuButton2_down"), NID_MENU_INVENTORY, 56, 0xC3);
    CharacterButton->bClickSound = 0;
    CommunityButton = CreateMenuIcon(this, TEXT("L2UI_ch3.MenuIcon.MenuButton3"),
                                     TEXT("L2UI_ch3.MenuIcon.MenuButton3_down"), NID_MENU_COMMUNITY, 93, 0x39D);
    CharacterButton->bClickSound = 0;
    SystemButton = CreateMenuIcon(this, TEXT("L2UI_ch3.MenuIcon.MenuButton4"),
                                  TEXT("L2UI_ch3.MenuIcon.MenuButton4_down"), NID_MENU_SYSTEM, 130, 0xC1);
    CharacterButton->bClickSound = 0;
#if NW_CUSTOM
    NCustomShortCuts::CreateMenuIcons(this);   // custom: two more icons (NCConsole::OnCreate makes the bar wider)
#endif
    return 1;
}
NW_HOOK_METHOD(0x100b7de0, NCMenuWnd, OnCreate)

// 0x100b7360 (vslot 38) - C2: its own copy of the body, with a guard (C1 shared one with NCActionWnd and
// NCServerSelectWnd); the four buttons go in the order of their fields
INT NCMenuWnd::OnDestroy()
{
    guard(NCMenuWnd::OnDestroy);
#if NW_CUSTOM
    NCustomShortCuts::DestroyMenuIcons(this);
#endif
    if (SystemButton)
    {
        RemoveChildWindow(SystemButton->Entry);
        delete SystemButton;
        SystemButton = NULL;
    }
    if (InventoryButton)
    {
        RemoveChildWindow(InventoryButton->Entry);
        delete InventoryButton;
        InventoryButton = NULL;
    }
    if (CharacterButton)
    {
        RemoveChildWindow(CharacterButton->Entry);
        delete CharacterButton;
        CharacterButton = NULL;
    }
    if (CommunityButton)
    {
        RemoveChildWindow(CommunityButton->Entry);
        delete CommunityButton;
        CommunityButton = NULL;
    }
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100b7360, NCMenuWnd, OnDestroy)

// 0x100b7260 (vslot 62) - C3: the bar is three pieces of 46 px: the left end at 12, the middle stretched from 28 to
// Width - 16 (the width less 12 and 32) and the right end at Width - 16
INT NCMenuWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->DrawTexture(12, 0, 16, 46, 0, 0, 16, 46, BackLeftTex);
    Device->DrawTexture(28, 0, (INT)(Width - 12.0f - 32.0f), 46, 0, 0, 16, 46, BackTex);
    Device->DrawTexture((INT)(Width - 16.0f), 0, 16, 46, 0, 0, 16, 46, BackRightTex);
#if NW_CUSTOM
    NCustomShortCuts::PaintMenuIcons(this, Device);   // custom: the frame of the potion icon
#endif
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x100b7260, NCMenuWnd, OnPaint)

// 0x100b69d0 (vslot 74)
INT NCMenuWnd::OnSystem()
{
    GDesktop->ToggleSystemMenu();
    return 1;
}
NW_HOOK_METHOD(0x100b69d0, NCMenuWnd, OnSystem)

// 0x100b69f0 (vslot 75)
INT NCMenuWnd::OnCharacter()
{
    GDesktop->ToggleCharacter(-1);
    return 1;
}
NW_HOOK_METHOD(0x100b69f0, NCMenuWnd, OnCharacter)

// 0x100b6a10 (vslot 76)
INT NCMenuWnd::OnInventory()
{
    GDesktop->ToggleInventory();
    return 1;
}
NW_HOOK_METHOD(0x100b6a10, NCMenuWnd, OnInventory)

// 0x100b6a30 (vslot 71, NewVirtual71) - the Community button: a request to the server with an empty parameter stack
// of 10 (NConsoleWnd 0x1005d020: the header calls it RequestDeleteMacro, its name is a guess)
INT NCMenuWnd::OnCommunity()
{
    L2ParamStack Params(10);
    GConsole->RequestDeleteMacro(Params);
    return 1;
}
NW_HOOK_METHOD(0x100b6a30, NCMenuWnd, OnCommunity)

NW_CHECK_VTABLE(NCMenuWnd, 0x101bd178, 78)
