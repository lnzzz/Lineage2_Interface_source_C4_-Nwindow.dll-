/*=============================================================================
    NCGameOptionWnd.cpp - the game page of the options. C4 stock: 0x100c4970 .. 0x100c4a2f (WindowProc, OnReset),
    0x100c4a30 (ShowWindow), 0x100c77f0 (paint), 0x100c7a70 (OnDestroy), 0x100c7e50 .. 0x100c8c8f (ClearItems,
    LoadSettings, Apply, the system message toggle), 0x100c8c90 (the other PCs toggle), 0x100c97d0 (InitItems),
    0x100ca4b0 (OnCreate), 0x100cb430 (constructor).
=============================================================================*/
#include "nwindow/NCGameOptionWnd.h"
#include "nwindow/NCComboBox.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCShortCutWnd.h"
#include "nwindow/NCPartyWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCCheckBox.h"
#include "nwindow/NCChatWnd.h"
#include "nwindow/NCChatSystemMsgWnd.h"
#include "nwindow/NCOptionWnd.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NKeyCrypt.h"
#if NW_CUSTOM
#include "nwindow/NCustomTriumph.h"
#endif

// 0x100cb430 - NCWnd's constructor; the three never-made pages and the game pad box (only made with a game pad) cleared
NCGameOptionWnd::NCGameOptionWnd()
{
    Unused9C[0] = NULL;
    Unused9C[1] = NULL;
    Unused9C[2] = NULL;
    GamePadBox = NULL;
}
NW_HOOK_CTOR(0x100cb430, NCGameOptionWnd)

// 0x100cb490
NW_HOOK_DTOR(0x100cb490, NCGameOptionWnd)

// 0x100c4970 (vslot 25) - "Other PCs" (slot 75), the system message box (slot 74, a placeholder in NNewVirtuals.gen.cpp
// that is hooked below) and the reset button (slot 76)
INT NCGameOptionWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == (WPARAM)NID_GAMEOPT_OTHERPC)
            return OnOtherPCClicked() != 0;
        if (wParam == (WPARAM)NID_GAMEOPT_SYSMSG)
            return ::nw::VirtualCall<INT>(this, 74) != 0;
        if (wParam == (WPARAM)NID_GAMEOPT_RESET)
            return OnReset() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x100c4970, NCGameOptionWnd, WindowProc)

// a check box made as the stock code makes one (the label by sysstring number, checked, label right, no text parameter),
// 100 x 12 unless told, then its id and place
static FORCEINLINE NCCheckBox* NewOptionBox(NCWnd* Parent, INT LabelID, UBOOL bChecked, INT ID, INT X, INT Y, INT W = 100)
{
    NCCheckBox* Box = new NCCheckBox(LabelID, bChecked, 0, 0);
    Box->ID = ID;
    Box->SetPosition(X, Y, W, 12);
    Parent->CreateChild(Box, NWS_VISIBLE);
    return Box;
}

// 0x100ca4b0 (vslot 37) - C4: the page is 256 x 400; the language box, the six name boxes, the chat options, the game pad
// box (only with a game pad), the tracking, color cursor, arrow, zone title and system message boxes, the initialize
// button, the transparency box, the party loot box and the key encryption box; InitItems fills the lists. Check boxes
// by the four-argument constructor 0x100058C0 (the key encryption one by the text's: 0x10005720)
INT NCGameOptionWnd::OnCreate()
{
    NCWnd::OnCreate();
    LanguageCombo = new NCComboBox;
    LanguageCombo->ID = NID_GAMEOPT_LANGUAGE;
    LanguageCombo->SetPosition(87, 46, 125, 17);
    CreateChild(LanguageCombo, NWS_VISIBLE);
    NameBoxes[NAMEBOX_Player] = NewOptionBox(this, 436, 1, NID_GAMEOPT_PLAYER, 84, 85);
    NameBoxes[NAMEBOX_Monster] = NewOptionBox(this, 437, 1, NID_GAMEOPT_MONSTER, 84, 99);
    NameBoxes[NAMEBOX_OtherPC] = NewOptionBox(this, 438, 1, NID_GAMEOPT_OTHERPC, 84, 116);
    NameBoxes[NAMEBOX_Clan] = NewOptionBox(this, 439, 1, NID_GAMEOPT_CLAN, 95, 133);
    NameBoxes[NAMEBOX_Party] = NewOptionBox(this, 440, 1, NID_GAMEOPT_PARTY, 95, 150);
    NameBoxes[NAMEBOX_General] = NewOptionBox(this, 441, 1, NID_GAMEOPT_GENERAL, 95, 167);
    EnterChatBox = NewOptionBox(this, 442, 1, NID_GAMEOPT_ENTERCHAT, 25, 200);
    OldChatBox = NewOptionBox(this, 566, 1, NID_GAMEOPT_OLDCHAT, 135, 200);
    if (GL2CanUseJoystick)
        GamePadBox = NewOptionBox(this, 606, 1, NID_GAMEOPT_GAMEPAD, 135, 217);
    TrackingBox = NewOptionBox(this, 463, 1, NID_GAMEOPT_TRACKING, 25, 240);
    ColorCursorBox = NewOptionBox(this, 305, 0, NID_GAMEOPT_COLORCURSOR, 135, 240);
    ArrowBox = NewOptionBox(this, 928, 1, NID_GAMEOPT_ARROW, 25, 257);
    ZoneTitleBox = NewOptionBox(this, 1196, 1, NID_GAMEOPT_ZONETITLE, 135, 257);
    SystemMsgBox = NewOptionBox(this, 1197, 1, NID_GAMEOPT_SYSMSG, 25, 327, 150);
    SystemMsgDamageBox = NewOptionBox(this, 357, 1, NID_GAMEOPT_SYSMSGDAMAGE, 36, 344, 70);
    SystemMsgItemBox = NewOptionBox(this, 796, 1, NID_GAMEOPT_SYSMSGITEM, 120, 344, 130);

    ResetButton = new NCButton(TEXT("L2UI_ch3.button.smallbutton2"), TEXT("L2UI_ch3.button.smallbutton2_down"), 0);
    ResetButton->ID = NID_GAMEOPT_RESET;
    ResetButton->SetPosition(88, 11, 66, 21);
    CreateChild(ResetButton, NWS_VISIBLE);
    ResetButton->SetTextID(0x1DF);   // "Initialize"

    TransparencyBox = NewOptionBox(this, 927, 1, NID_GAMEOPT_TRANSPARENCY, 164, 15);

    PartyLootCombo = new NCComboBox;
    PartyLootCombo->ID = NID_GAMEOPT_PARTYLOOT;
    PartyLootCombo->SetPosition(87, 374, 125, 17);
    CreateChild(PartyLootCombo, NWS_VISIBLE | 0x10);   // 0x10: meaning unknown

    KeyCryptBox = new NCCheckBox(*GL2GameData.SysString[926], 1, 0, 0);
    KeyCryptBox->ID = NID_GAMEOPT_KEYCRYPT;
    KeyCryptBox->SetPosition(25, 217, 100, 12);
    CreateChild(KeyCryptBox, NWS_VISIBLE);
#if NW_CUSTOM
    NCustomTriumph::OnCreate(this);   // custom: the Triumph weapons' effects, before InitItems loads the settings
#endif
    InitItems();
    return 1;
}
NW_HOOK_METHOD(0x100ca4b0, NCGameOptionWnd, OnCreate)

// a child window goes (repeated inline in the stock OnDestroy)
template <class T> static FORCEINLINE void DestroyChild(NCWnd* Parent, T*& Child)
{
    if (Child)
    {
        Parent->RemoveChildWindow(Child->Entry);
        if (Child)
            delete Child;
        Child = NULL;
    }
}

// 0x100c7a70 (vslot 38) - guarded; the children go in the stock's order (the name boxes, the language box, the button, the
// three never-made windows, the chat and tracking boxes, party loot, the game pad, then the rest)
INT NCGameOptionWnd::OnDestroy()
{
    guard(NCGameOptionWnd::OnDestroy);
    for (INT i = 0; i < NAMEBOX_MAX; i++)
        DestroyChild(this, NameBoxes[i]);
    DestroyChild(this, LanguageCombo);
    DestroyChild(this, ResetButton);
    DestroyChild(this, Unused9C[0]);
    DestroyChild(this, Unused9C[1]);
    DestroyChild(this, Unused9C[2]);
    DestroyChild(this, EnterChatBox);
    DestroyChild(this, OldChatBox);
    DestroyChild(this, TrackingBox);
    DestroyChild(this, PartyLootCombo);
    if (GL2CanUseJoystick)
        DestroyChild(this, GamePadBox);
    DestroyChild(this, TransparencyBox);
    DestroyChild(this, ArrowBox);
    DestroyChild(this, ColorCursorBox);
    DestroyChild(this, ZoneTitleBox);
    DestroyChild(this, SystemMsgBox);
    DestroyChild(this, SystemMsgDamageBox);
    DestroyChild(this, SystemMsgItemBox);
    DestroyChild(this, KeyCryptBox);
#if NW_CUSTOM
    NCustomTriumph::OnDestroy(this);
#endif
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100c7a70, NCGameOptionWnd, OnDestroy)

// 0x100c4a30 (vslot 48) - the party loot rule cannot be changed by one who is not the leader (User +0x19C); the options
// window's second texture (its +0xEC) is set on each show
void NCGameOptionWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    User* Me = GConsole->GetMyUser();
    if (!Me || Me->Unknown188 == 0)
        PartyLootCombo->EnableWindow(1);
    else
        PartyLootCombo->EnableWindow(0);
    ((NCOptionWnd*)Entry->Parent->Wnd)->OptionBackTex = FindTexture(TEXT("L2ui_ch3.OptionWnd.option3_back"));
}
NW_HOOK_METHOD(0x100c4a30, NCGameOptionWnd, ShowWindow)

// a label right-aligned on x Right (repeated inline in the stock OnPaint)
static FORCEINLINE void DrawOptionLabel(NCGDevice* Device, INT TextID, INT Right, INT Y)
{
    SIZE Size;
    NCGDevice::GetTextExtent(&Size, *NCObject::SysString(TextID), 0, 0, 0, 0, 0);
    Device->DrawText(Right - Size.cx, Y, 0xFFDCDCDC, *NCObject::SysString(TextID), 0, 0, 0,
                     L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
}

// 0x100c77f0 (vslot 62) - the party loot box is disabled while the player is in a party or the party room window shows;
// four headings, the first at (24, 15) (the stock measures it all the same), the others right-aligned, in 0xFFDCDCDC
INT NCGameOptionWnd::OnPaint(NCGDevice* Device)
{
    guard(NCOptionWnd::OnPaint);   // the stock's guard carries the options window's name
    if (GDesktop->PartyWnd->NumMembers < 1 && !GDesktop->PartyRoomWnd->IsShowWindow())
        PartyLootCombo->EnableWindow(1);
    else
        PartyLootCombo->EnableWindow(0);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    SIZE Size;
    NCGDevice::GetTextExtent(&Size, *NCObject::SysString(478), 0, 0, 0, 0, 0);
    Device->DrawText(24, 15, 0xFFDCDCDC, *NCObject::SysString(478), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    DrawOptionLabel(Device, 329, 79, 49);    // Language
    DrawOptionLabel(Device, 435, 76, 82);    // Display
    DrawOptionLabel(Device, 486, 79, 377);   // Party Loot
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100c77f0, NCGameOptionWnd, OnPaint)

// 0x100c8c90 (vslot 75) - "Other PCs" switches its three sub-boxes
INT NCGameOptionWnd::OnOtherPCClicked()
{
    if (NameBoxes[NAMEBOX_OtherPC]->bChecked)
    {
        NameBoxes[NAMEBOX_Clan]->bDisabled = 0;
        NameBoxes[NAMEBOX_Party]->bDisabled = 0;
        NameBoxes[NAMEBOX_General]->bDisabled = 0;
    }
    else
    {
        NameBoxes[NAMEBOX_Clan]->bDisabled = 1;
        NameBoxes[NAMEBOX_Party]->bDisabled = 1;
        NameBoxes[NAMEBOX_General]->bDisabled = 1;
    }
    return 1;
}
NW_HOOK_METHOD(0x100c8c90, NCGameOptionWnd, OnOtherPCClicked)

// 0x100c8c40 (slot 74, a stock placeholder in NNewVirtuals.gen.cpp, so hooked through a stand-in class; name ours) - the
// system message box switches its two sub-boxes (damage and item lines) like "Other PCs" does
struct NGameOptionFns : public NCGameOptionWnd
{
    __declspec(noinline) INT OnSystemMsgClicked();
};
INT NGameOptionFns::OnSystemMsgClicked()
{
    if (SystemMsgBox->bChecked)
    {
        SystemMsgDamageBox->bDisabled = 0;
        SystemMsgItemBox->bDisabled = 0;
    }
    else
    {
        SystemMsgDamageBox->bDisabled = 1;
        SystemMsgItemBox->bDisabled = 1;
    }
    return 1;
}
NW_HOOK_METHOD(0x100c8c40, NGameOptionFns, OnSystemMsgClicked)

// 0x100c4a10 (vslot 76) - the windows back in their places: the console's virtual 362 (C4Virtual362, 0x10073AC0)
INT NCGameOptionWnd::OnReset()
{
    ::nw::VirtualCall<void>(GConsole, 362);
    return 1;
}
NW_HOOK_METHOD(0x100c4a10, NCGameOptionWnd, OnReset)

// The nProtect key encryption driver's calls: the stock makes them with the console's NKeyCrypt object (+0x3C60) in ecx, which
// none of the three reads, through its own linker thunks (GetLoadStartup 0x1004C060 = a jump to ordinal 41 of npkcrypt.dll,
// LoadAtStartup 0x1004BEB0 = ordinal 26, with one argument) and IsAdministrator 0x1004BF50; here the real functions of
// NKeyCrypt.cpp (the same ones in both builds: they load npkcrypt.dll by ordinal)
static INT KeyCryptGetLoadStartup() { return NKeyCrypt::GetLoadStartup(); }
static void KeyCryptLoadAtStartup(INT bLoad) { NKeyCrypt::LoadAtStartup(bLoad); }
static INT KeyCryptIsAdmin() { return NKeyCrypt::IsAdministrator(); }

// 0x100c7f60 - the settings from Option.ini into the globals and the boxes. C4: the game pad box is checked (and, without
// a pad, disabled) in both branches, as the stock code does; the zone title, the system message windows, the color cursor
// and the screen shot quality (read, no box) are new
void NCGameOptionWnd::LoadSettings()
{
    UBOOL bNative = 1;
    UBOOL bOtherPC = 0;
    if (!GConfig->GetBool(TEXT("Game"), TEXT("IsNative"), bNative, TEXT("Option.ini")))
        bNative = 1;
    if (!GConfig->GetBool(TEXT("Game"), TEXT("MyName"), GL2ShowMyName, TEXT("Option.ini")))
        GL2ShowMyName = 1;
    if (!GConfig->GetBool(TEXT("Game"), TEXT("NPCName"), GL2ShowNPCName, TEXT("Option.ini")))
        GL2ShowNPCName = 1;
    if (!GConfig->GetBool(TEXT("Game"), TEXT("GroupName"), bOtherPC, TEXT("Option.ini")))
        bOtherPC = 1;
    if (!GConfig->GetBool(TEXT("Game"), TEXT("PledgeMemberName"), GL2ShowPledgeMemberName, TEXT("Option.ini")))
        GL2ShowPledgeMemberName = 1;
    if (!GConfig->GetBool(TEXT("Game"), TEXT("PartyMemberName"), GL2ShowPartyMemberName, TEXT("Option.ini")))
        GL2ShowPartyMemberName = 1;
    if (!GConfig->GetBool(TEXT("Game"), TEXT("OtherPCName"), GL2ShowOtherPCName, TEXT("Option.ini")))
        GL2ShowOtherPCName = 1;
    if (!GConfig->GetBool(TEXT("Game"), TEXT("EnterChatting"), GIsEnterChatting, TEXT("Option.ini")))
        GIsEnterChatting = 0;
    if (!GConfig->GetBool(TEXT("Game"), TEXT("OldChatting"), GIsOldChat, TEXT("Option.ini")))
        GIsOldChat = 1;
    if (!GConfig->GetInt(TEXT("Game"), TEXT("PartyLooting"), GL2PartyLooting, TEXT("Option.ini")))
        GL2PartyLooting = 0;
    if (!GConfig->GetInt(TEXT("Game"), TEXT("ScreenShotQuality"), GL2ScreenShotQuality, TEXT("Option.ini")))
        GL2ScreenShotQuality = 0;
    if (!GConfig->GetBool(TEXT("Game"), TEXT("ShowZoneTitle"), GL2ShowZoneTitle, TEXT("Option.ini")))
        GL2ShowZoneTitle = 1;
    if (!GConfig->GetBool(TEXT("Game"), TEXT("SystemMsgWnd"), GL2SystemMsgWnd, TEXT("Option.ini")))
        GL2SystemMsgWnd = 1;
    if (!GConfig->GetBool(TEXT("Game"), TEXT("SystemMsgWndDamage"), GL2SystemMsgWndDamage, TEXT("Option.ini")))
        GL2SystemMsgWndDamage = 1;
    if (!GConfig->GetBool(TEXT("Game"), TEXT("SystemMsgWndExpendableItem"), GL2SystemMsgWndExpendableItem,
                          TEXT("Option.ini")))
        GL2SystemMsgWndExpendableItem = 1;
    NameBoxes[NAMEBOX_OtherPC]->bChecked = bOtherPC;
    NameBoxes[NAMEBOX_Player]->bChecked = GL2ShowMyName;
    NameBoxes[NAMEBOX_Monster]->bChecked = GL2ShowNPCName;
    NameBoxes[NAMEBOX_Clan]->bChecked = GL2ShowPledgeMemberName;
    NameBoxes[NAMEBOX_Party]->bChecked = GL2ShowPartyMemberName;
    NameBoxes[NAMEBOX_General]->bChecked = GL2ShowOtherPCName;
    OnOtherPCClicked();
    if (!bOtherPC)
    {
        GL2ShowOtherPCName = 0;
        GL2ShowPartyMemberName = 0;
        GL2ShowPledgeMemberName = 0;
    }
    TrackingBox->bChecked = GL2UseAutoTrackingPawn;
    LanguageCombo->SetSelected(bNative ? 0 : 1);
    if (!GConfig->GetBool(TEXT("Game"), TEXT("UseJoystick"), GL2UsingJoystick, TEXT("Option.ini")))
        GL2UsingJoystick = 1;
    if (!GL2CanUseJoystick)
    {
        if (GamePadBox)
            GamePadBox->bChecked = 1;
        if (GamePadBox)
            GamePadBox->bDisabled = 1;
    }
    if (!GL2UsingJoystick)
    {
        if (GamePadBox)
            GamePadBox->bChecked = 1;
        GDesktop->ShortCutWnd->PadButton->HideWindow();
    }
    else
    {
        if (GamePadBox)
            GamePadBox->bChecked = 1;
        GDesktop->ShortCutWnd->PadButton->ShowWindow();
    }
    if (!GIsEnterChatting)
        EnterChatBox->bChecked = 0;
    else
        EnterChatBox->bChecked = 1;
    if (!GConfig->GetBool(TEXT("Game"), TEXT("TransparencyMode"), GIsTransparencyMode, TEXT("Option.ini")))
        GIsTransparencyMode = 1;
    TransparencyBox->bChecked = GIsTransparencyMode;
    if (!GConfig->GetBool(TEXT("Game"), TEXT("ArrowMode"), GIsArrowMode, TEXT("Option.ini")))
        GIsArrowMode = 1;
    ArrowBox->bChecked = GIsArrowMode;
    if (!GL2UseColorCursor)
        ColorCursorBox->bChecked = 0;
    else
        ColorCursorBox->bChecked = 1;
    ZoneTitleBox->bChecked = GL2ShowZoneTitle;
    SystemMsgBox->bChecked = GL2SystemMsgWnd;
    SystemMsgDamageBox->bChecked = GL2SystemMsgWndDamage;
    SystemMsgItemBox->bChecked = GL2SystemMsgWndExpendableItem;
    if (GL2UseKeyCrypt && KeyCryptGetLoadStartup())
        KeyCryptBox->bChecked = 1;
    else
        KeyCryptBox->bChecked = 0;
    if (GL2UseKeyCrypt && KeyCryptIsAdmin())
        KeyCryptBox->bDisabled = 0;
    else
        KeyCryptBox->bDisabled = 1;
#if NW_CUSTOM
    NCustomTriumph::OnLoadSettings(this);
#endif
}
NW_HOOK_METHOD(0x100c7f60, NCGameOptionWnd, LoadSettings)

// 0x100c8530 - the boxes into the globals and Option.ini. C4: the language changes the tables again with the new Clear and the
// zone name banner shown again; the party loot rule only while the combo is enabled (else it goes back to the console's rule,
// +0x3C6C); the color cursor, the zone title and the system message windows are saved (and the chat window's system list
// shown or hidden); the screen shot quality is no longer written here
void NCGameOptionWnd::Apply()
{
    INT Language = LanguageCombo->Selected;
    UBOOL bSavedNative = 1;
    if (!GConfig->GetBool(TEXT("Game"), TEXT("IsNative"), bSavedNative, TEXT("Option.ini")))
        bSavedNative = 1;
    if (Language == 0)
        GIsNative = 1;
    else if (Language == 1)
        GIsNative = 0;
    if (bSavedNative != GIsNative)
    {
        GL2GameData.Clear();
        GL2GameData.Load(0);
        // the stock's cast of the first viewport's actor to the player controller (0x1005C450)
        ALineagePlayerController* Controller = CastToLineagePlayerController(GConsole->Engine->Client->Viewports(0)->Actor);   // 0x1005C450
        Controller->InvalidateZoneName();
        GConsole->ReloadGameTexts();
    }
    GL2ShowMyName = NameBoxes[NAMEBOX_Player]->bChecked;
    GL2ShowNPCName = NameBoxes[NAMEBOX_Monster]->bChecked;
    if (NameBoxes[NAMEBOX_OtherPC]->bChecked)
    {
        GL2ShowPledgeMemberName = NameBoxes[NAMEBOX_Clan]->bChecked;
        GL2ShowPartyMemberName = NameBoxes[NAMEBOX_Party]->bChecked;
        GL2ShowOtherPCName = NameBoxes[NAMEBOX_General]->bChecked;
    }
    else
    {
        GL2ShowOtherPCName = 0;
        GL2ShowPartyMemberName = 0;
        GL2ShowPledgeMemberName = 0;
    }
    GIsTransparencyMode = TransparencyBox->bChecked;
    GIsArrowMode = ArrowBox->bChecked;
    GL2UseAutoTrackingPawn = TrackingBox->bChecked;
    GIsEnterChatting = EnterChatBox->bChecked;
    GIsOldChat = OldChatBox->bChecked;
    GL2ShowZoneTitle = ZoneTitleBox->bChecked;
    GL2SystemMsgWnd = SystemMsgBox->bChecked;
    GL2SystemMsgWndDamage = SystemMsgDamageBox->bChecked;
    GL2SystemMsgWndExpendableItem = SystemMsgItemBox->bChecked;
    GConfig->SetBool(TEXT("Game"), TEXT("IsNative"), GIsNative, TEXT("Option.ini"));
    GConfig->SetBool(TEXT("Game"), TEXT("MyName"), GL2ShowMyName, TEXT("Option.ini"));
    GConfig->SetBool(TEXT("Game"), TEXT("NPCName"), GL2ShowNPCName, TEXT("Option.ini"));
    GConfig->SetBool(TEXT("Game"), TEXT("GroupName"), NameBoxes[NAMEBOX_OtherPC]->bChecked, TEXT("Option.ini"));
    GConfig->SetBool(TEXT("Game"), TEXT("PledgeMemberName"), NameBoxes[NAMEBOX_Clan]->bChecked, TEXT("Option.ini"));
    GConfig->SetBool(TEXT("Game"), TEXT("PartyMemberName"), NameBoxes[NAMEBOX_Party]->bChecked, TEXT("Option.ini"));
    GConfig->SetBool(TEXT("Game"), TEXT("OtherPCName"), NameBoxes[NAMEBOX_General]->bChecked, TEXT("Option.ini"));
    GConfig->SetBool(TEXT("Game"), TEXT("AutoTrackingPawn"), GL2UseAutoTrackingPawn, TEXT("Option.ini"));
    GConfig->SetBool(TEXT("Game"), TEXT("EnterChatting"), GIsEnterChatting, TEXT("Option.ini"));
    GConfig->SetBool(TEXT("Game"), TEXT("OldChatting"), GIsOldChat, TEXT("Option.ini"));
    GConfig->SetBool(TEXT("Game"), TEXT("TransparencyMode"), GIsTransparencyMode, TEXT("Option.ini"));
    GConfig->SetBool(TEXT("Game"), TEXT("ArrowMode"), GIsArrowMode, TEXT("Option.ini"));
    // the console's party item rule (C4 +0x3C6C, NConsoleWnd::PartyItemRule)
    INT& PartyItemRule = GConsole->PartyItemRule;
    if (PartyLootCombo->IsEnabled())
    {
        PartyItemRule = PartyLootCombo->Selected;
        GL2PartyLooting = PartyLootCombo->Selected;
        GConfig->SetInt(TEXT("Game"), TEXT("PartyLooting"), GL2PartyLooting, TEXT("Option.ini"));
    }
    else
        PartyLootCombo->SetSelected(PartyItemRule);
    if (!ColorCursorBox->bChecked)
    {
        GL2UseColorCursor = 0;
        GConfig->SetString(TEXT("Video"), TEXT("UseColorCursor"), TEXT("false"), TEXT("Option.ini"));
        SetCursor(0, NULL);
        LockCursor();
    }
    else
    {
        GL2UseColorCursor = 1;
        GConfig->SetString(TEXT("Video"), TEXT("UseColorCursor"), TEXT("true"), TEXT("Option.ini"));
    }
    GConfig->SetBool(TEXT("Game"), TEXT("ShowZoneTitle"), GL2ShowZoneTitle, TEXT("Option.ini"));
    GConfig->SetBool(TEXT("Game"), TEXT("SystemMsgWnd"), GL2SystemMsgWnd, TEXT("Option.ini"));
    GConfig->SetBool(TEXT("Game"), TEXT("SystemMsgWndDamage"), GL2SystemMsgWndDamage, TEXT("Option.ini"));
    GConfig->SetBool(TEXT("Game"), TEXT("SystemMsgWndExpendableItem"), GL2SystemMsgWndExpendableItem, TEXT("Option.ini"));
    NCChatListBox* SystemList;
    if (GL2SystemMsgWnd)
    {
        GDesktop->ChatWnd->SystemList->ShowWindow();
        SystemList = GDesktop->ChatWnd->SystemList;
    }
    else
    {
        GDesktop->ChatWnd->SystemList->HideWindow();
        SystemList = GDesktop->ChatWnd->SystemList;
    }
    static_cast<NCChatSystemMsgWnd*>(SystemList)->RearrangePos();   // lays the chat window's system list out again (0x10053FD0)
    if (GL2CanUseJoystick)
    {
        if (!GamePadBox->bChecked)
        {
            GL2UsingJoystick = 0;
            GDesktop->ShortCutWnd->PadButton->HideWindow();
            NCShortCutWnd* Bar = GDesktop->ShortCutWnd;
            if (Bar->bPadMode == 1)
            {
                Bar->bPadMode = 0;
                GDesktop->ShortCutWnd->RotateButton->ShowWindow();
                GDesktop->ShortCutWnd->UpdateLayout();
            }
        }
        else
        {
            GL2UsingJoystick = 1;
            GDesktop->ShortCutWnd->PadButton->ShowWindow();
        }
        GConfig->SetBool(TEXT("Game"), TEXT("UseJoystick"), GL2UsingJoystick, TEXT("Option.ini"));
    }
    if (GL2UseKeyCrypt && KeyCryptBox->bChecked)
        KeyCryptLoadAtStartup(1);   // the key encryption driver at startup
    else
        KeyCryptLoadAtStartup(0);
#if NW_CUSTOM
    NCustomTriumph::OnApply(this);
#endif
}
NW_HOOK_METHOD(0x100c8530, NCGameOptionWnd, Apply)

// 0x100c7e50 - the boxes cleared (C4: the new boxes too, the screen shot box gone)
void NCGameOptionWnd::ClearItems()
{
    for (INT i = 0; i < NAMEBOX_MAX; i++)
        NameBoxes[i]->bChecked = 0;
    LanguageCombo->ClearItems();
    EnterChatBox->bChecked = 0;
    OldChatBox->bChecked = 0;
    TrackingBox->bChecked = 0;
    PartyLootCombo->ClearItems();
    TransparencyBox->bChecked = 0;
    ArrowBox->bChecked = 0;
    ColorCursorBox->bChecked = 0;
    ZoneTitleBox->bChecked = 0;
    SystemMsgBox->bChecked = 0;
    SystemMsgDamageBox->bChecked = 0;
    SystemMsgItemBox->bChecked = 0;
    if (GL2CanUseJoystick)
        GamePadBox->bChecked = 0;
    KeyCryptBox->bChecked = 0;
}
NW_HOOK_METHOD(0x100c7e50, NCGameOptionWnd, ClearItems)

// 0x100c97d0 - the language list is read from localization.ini (the client's own language, then English; the combo is
// disabled when GEnableEngSelection is off), the party loot box has five rules (sysstrings 487, 488, 798 .. 800, by id now)
void NCGameOptionWnd::InitItems()
{
    INT ClientLanguage = 0;
    if (!GConfig->GetInt(TEXT("LanguageSet"), TEXT("Language"), ClientLanguage, TEXT("localization.ini")))
    {
        ClientLanguage = 0;
        LanguageCombo->AddItem(TEXT("Korean"));
        LanguageCombo->AddItem(TEXT("English"));
        if (!GEnableEngSelection)
            LanguageCombo->EnableWindow(0);
    }
    else if (ClientLanguage == 0)
    {
        LanguageCombo->AddItem(TEXT("Korean"));
        LanguageCombo->AddItem(TEXT("English"));
        if (!GEnableEngSelection)
            LanguageCombo->EnableWindow(0);
    }
    else if (ClientLanguage == 1)
        LanguageCombo->AddItem(TEXT("English"));
    else
    {
        const TCHAR* Own = NULL;
        if (ClientLanguage == 2)
            Own = TEXT("Japanese");
        else if (ClientLanguage == 3)
            Own = TEXT("Chinese(Taiwan)");
        else if (ClientLanguage == 4)
            Own = TEXT("China");
        else if (ClientLanguage == 5)
            Own = TEXT("Thai");
        if (Own)
        {
            LanguageCombo->AddItem(Own);
            LanguageCombo->AddItem(TEXT("English"));
            if (!GEnableEngSelection)
                LanguageCombo->EnableWindow(0);
        }
    }
    PartyLootCombo->AddItem(487);   // Finders Keepers
    PartyLootCombo->AddItem(488);   // Random
    PartyLootCombo->AddItem(798);
    PartyLootCombo->AddItem(799);
    PartyLootCombo->AddItem(800);
    LoadSettings();
    if (EnterChatBox)
        EnterChatBox->bChecked = GIsEnterChatting != 0;
    if (OldChatBox)
        OldChatBox->bChecked = GIsOldChat != 0;
    PartyLootCombo->SetSelected(GL2PartyLooting);
    GConsole->PartyItemRule = PartyLootCombo->Selected;   // the console's party item rule (+0x3C6C)
}
NW_HOOK_METHOD(0x100c97d0, NCGameOptionWnd, InitItems)

NW_CHECK_VTABLE(NCGameOptionWnd, 0x101c04a8, 77)
