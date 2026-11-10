/*=============================================================================
    NCustomShortCuts.cpp - extra shortcut bars (custom, NW_CUSTOM builds only). See NCustomShortCuts.h.
=============================================================================*/
#include "nwindow/NCustomShortCuts.h"
#include "nwindow/NCustomAutoPotion.h"
#include "nwindow/NCustomDamage.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCCheckBox.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

// the bars and their state (statics: the windows have no field of their own)
static NCExtraShortCutWnd* GBars[NCustomShortCuts::MaxBars];
static NCPushButton* GExpandButton = NULL;
static NCPushButton* GLockButton = NULL;
static UBOOL GLocked = 0;
static INT GShownBars = 0;                         // how many extra bars are shown (0 .. MaxBars)
static INT GSavedPages[NCustomShortCuts::MaxBars];  // the pages last written to the ini file
static INT GLayoutHorizontal = -1;                 // the main bar's orientation the set was last laid out for
static INT GLayoutMaxed = -1;                      // the expand button shows "minus" (all bars shown)

// the key sets: index 0 is the main bar, 1 .. MaxBars the extra bars
enum { KS_None, KS_F, KS_Num, KS_QWERTY, KS_ASDF, KS_ZXCV, KS_Count };
static const TCHAR* GKeySetNames[KS_Count] = { TEXT("None"), TEXT("F"), TEXT("Num"), TEXT("QWERTY"), TEXT("ASDF"), TEXT("ZXCV") };
static const INT GDefaultKeySets[NCustomShortCuts::MaxBars + 1] = { KS_F, KS_Num, KS_QWERTY, KS_None, KS_None };
// the physical keys of each row (scan codes, 0 = no key there): 1 .. =, Q .. ], A .. ' and the key by Enter, Z .. /
static const BYTE GKeyRows[KS_Count][12] = {
    { 0 },
    { 0 },
    { 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D },
    { 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B },
    { 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x2B },
    { 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0, 0 },
};
static INT GKeySets[NCustomShortCuts::MaxBars + 1];

static NCShortCutWnd* MainBar()
{
    if (!GConsole || !GConsole->Desktop)
        return NULL;
    return GDesktop->ShortCutWnd;
}

/*-----------------------------------------------------------------------------
    Settings: system\NWindowCustom.ini, next to l2.exe.
-----------------------------------------------------------------------------*/

static const TCHAR* IniPath()
{
    static TCHAR Path[MAX_PATH] = { 0 };
    if (!Path[0])
    {
        GetModuleFileNameW(NULL, Path, MAX_PATH);
        TCHAR* Slash = wcsrchr(Path, '\\');
        if (Slash)
            Slash[1] = 0;
        wcscat_s(Path, MAX_PATH, TEXT("NWindowCustom.ini"));
    }
    return Path;
}

static INT IniGetInt(const TCHAR* Key, INT Default)
{
    return (INT)GetPrivateProfileIntW(TEXT("ShortCut"), Key, Default, IniPath());
}

static void IniSetInt(const TCHAR* Key, INT Value)
{
    TCHAR Text[16];
    swprintf_s(Text, 16, TEXT("%d"), Value);
    WritePrivateProfileStringW(TEXT("ShortCut"), Key, Text, IniPath());
}

static void PageKey(TCHAR* Out, INT Index)
{
    swprintf_s(Out, 16, TEXT("Page%d"), Index + 1);
}

static void KeySetKey(TCHAR* Out, INT Bar)
{
    swprintf_s(Out, 16, TEXT("Bar%dKeys"), Bar + 1);
}

// the key sets from the file; a set two bars name goes to the first one
static void LoadKeySets()
{
    for (INT Bar = 0; Bar <= NCustomShortCuts::MaxBars; Bar++)
    {
        TCHAR Key[16], Value[16];
        KeySetKey(Key, Bar);
        GetPrivateProfileStringW(TEXT("ShortCut"), Key, GKeySetNames[GDefaultKeySets[Bar]], Value, 16, IniPath());
        INT Set = GDefaultKeySets[Bar];
        for (INT k = 0; k < KS_Count; k++)
            if (!_wcsicmp(Value, GKeySetNames[k]))
                Set = k;
        for (INT Other = 0; Other < Bar; Other++)
            if (Set != KS_None && GKeySets[Other] == Set)
                Set = KS_None;
        GKeySets[Bar] = Set;
    }
}

static void SaveKeySet(INT Bar)
{
    TCHAR Key[16];
    KeySetKey(Key, Bar);
    WritePrivateProfileStringW(TEXT("ShortCut"), Key, GKeySetNames[GKeySets[Bar]], IniPath());
}

/*-----------------------------------------------------------------------------
    NCExtraShortCutWnd.
-----------------------------------------------------------------------------*/

INT NCExtraShortCutWnd::BarIndex() const
{
    return NCustomShortCuts::IndexOf(this);
}

// the stock OnCreate makes the timer, the textures, the buttons and a table of slots and keys of its own: the tables go
// and the main bar's are used (the main bar is made first and destroyed last)
INT NCExtraShortCutWnd::OnCreate()
{
    NCShortCutWnd::OnCreate();
    if (Items)
    {
        INT* Block = (INT*)Items - 1;
        for (INT i = Block[0] - 1; i >= 0; i--)
            Items[i].DestroyItem();
        appFree(Block);
        Items = NULL;
    }
    if (Keys)
    {
        delete[] Keys;
        Keys = NULL;
    }
    NCShortCutWnd* Main = MainBar();
    Items = Main->Items;
    Keys = Main->Keys;
    RotateButton->HideWindow();
    return 1;
}

INT NCExtraShortCutWnd::OnDestroy()
{
    Items = NULL;
    Keys = NULL;
    return NCShortCutWnd::OnDestroy();
}

// the stock ShowWindow shows the children again: the turn button is the main bar's only
void NCExtraShortCutWnd::ShowWindow()
{
    NCShortCutWnd::ShowWindow();
    RotateButton->HideWindow();
}

// the title bar's drag (WM_MOVE) goes to the main bar: the set is laid out around it on the next frame
INT NCExtraShortCutWnd::OnMove(WPARAM wParam, LPARAM lParam)
{
    NCShortCutWnd* Main = MainBar();
    if (Main)
        return Main->OnMove(wParam, lParam);
    return 1;
}

INT NCExtraShortCutWnd::OnRotateClicked()
{
    return 1;
}

INT NCExtraShortCutWnd::OnPadClicked()
{
    return 1;
}

/*-----------------------------------------------------------------------------
    The set.
-----------------------------------------------------------------------------*/

INT NCustomShortCuts::IndexOf(const NCShortCutWnd* Wnd)
{
    for (INT i = 0; i < MaxBars; i++)
        if (Wnd && GBars[i] == Wnd)
            return i;
    return -1;
}

/*-----------------------------------------------------------------------------
    The lock button: the expand button's face (its "+" covered) with a padlock drawn on it, open or closed.
-----------------------------------------------------------------------------*/

class NCLockButton : public NCPushButton
{
public:
    static UClass* StaticClass() { return NCPushButton::StaticClass(); }

    NCLockButton() : NCPushButton(TEXT("L2UI_CH3.QUESTWND.QuestWndPlusBtn"), TEXT("L2UI_CH3.QUESTWND.QuestWndPlusBtn_over")) {}

    virtual INT OnPaint(NCGDevice* Device);
};
NW_ASSERT_SIZE(NCLockButton, 0x2C0);

// the face (15 x 15: the frame, the dark inside from 3, 2 to 12, 12) and on it the padlock: the body (8 x 5 with a 2 x 2
// key hole) and the shackle, in the body when closed, lifted with its right side out when open. Plain textures stretched
// (the device's DrawRect draws its white texture half transparent, whatever the colour)
INT NCLockButton::OnPaint(NCGDevice* Device)
{
    UTexture* Dark = FindTexture(TEXT("NWindow.BlackTexture"));
    UTexture* Light = FindTexture(TEXT("NWindow.WhiteTexture"));
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    DrawButton(Device);
    struct NBox { BYTE X, Y, W, H; };
    static const NBox Body[] = { { 4, 7, 8, 5 } }, Hole[] = { { 7, 8, 2, 2 } };
    static const NBox Closed[] = { { 6, 3, 4, 1 }, { 5, 4, 1, 3 }, { 10, 4, 1, 3 } };
    static const NBox Open[] = { { 6, 2, 4, 1 }, { 5, 3, 1, 4 }, { 10, 3, 1, 1 } };
    Device->DrawTexture(3, 2, 10, 11, 0, 0, 1, 1, Dark, 0xFF, 1);
    Device->DrawTexture(Body[0].X, Body[0].Y, Body[0].W, Body[0].H, 0, 0, 1, 1, Light, 0xFF, 1);
    Device->DrawTexture(Hole[0].X, Hole[0].Y, Hole[0].W, Hole[0].H, 0, 0, 1, 1, Dark, 0xFF, 1);
    const NBox* Shackle = GLocked ? Closed : Open;
    for (INT i = 0; i < 3; i++)
        Device->DrawTexture(Shackle[i].X, Shackle[i].Y, Shackle[i].W, Shackle[i].H, 0, 0, 1, 1, Light, 0xFF, 1);
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
}

static void UpdateLockButton(NCShortCutWnd* Main)
{
    if (!GLockButton)
        return;
    if (GLayoutHorizontal != Main->bHorizontal)
    {
        if (Main->bHorizontal)
            GLockButton->SetPosition(0x1E9, 1, 0xF, 0xF);
        else
            GLockButton->SetPosition(1, 0x1E9, 0xF, 0xF);
    }
    // the game pad button has that place while a game pad is in use; the pad layout has no room
    UBOOL bShow = Main->bPadMode == 0 && !(GL2UsingJoystick && GL2CanUseJoystick);
    if (bShow != (GLockButton->IsShowWindow() != 0))
    {
        if (bShow)
            GLockButton->ShowWindow();
        else
            GLockButton->HideWindow();
    }
}

UBOOL NCustomShortCuts::IsLocked()
{
    return GLocked;
}

static void UpdateExpandButton(NCShortCutWnd* Main)
{
    if (!GExpandButton)
        return;
    INT Maxed = GShownBars >= NCustomShortCuts::MaxBars;
    UpdateLockButton(Main);
    if (GLayoutHorizontal != Main->bHorizontal)
    {
        if (Main->bHorizontal)
            GExpandButton->SetPosition(0x1E9, 0x1F, 0xF, 0xF);
        else
            GExpandButton->SetPosition(0x1F, 0x1E9, 0xF, 0xF);
    }
    if (GLayoutMaxed != Maxed)
    {
        if (Maxed)
        {
            GExpandButton->SetNormalTex(TEXT("L2UI_CH3.QUESTWND.QuestWndMinusBtn"));
            GExpandButton->SetPushedTex(TEXT("L2UI_CH3.QUESTWND.QuestWndMinusBtn_over"));
        }
        else
        {
            GExpandButton->SetNormalTex(TEXT("L2UI_CH3.QUESTWND.QuestWndPlusBtn"));
            GExpandButton->SetPushedTex(TEXT("L2UI_CH3.QUESTWND.QuestWndPlusBtn_over"));
        }
        GLayoutMaxed = Maxed;
    }
}

/*-----------------------------------------------------------------------------
    The custom windows' back and the menu bar's icons.
-----------------------------------------------------------------------------*/

void NCustomShortCuts::DrawPanel(NCWnd* Owner, NCGDevice* Device, INT X, INT Y, INT W, INT H)
{
    enum { TW = 256, TH = 108, M = 8 };   // the panel's part of the texture and the border kept unscaled
    UTexture* T = Owner->FindTexture(TEXT("L2UI_CH3.dialog.system_back"));
    if (!T || W < 2 * M || H < 2 * M)
        return;
    INT IW = W - 2 * M, IH = H - 2 * M;
    UTexture* Shade = Owner->FindTexture(TEXT("NWindow.BlackTexture"));   // the panel is translucent: what is behind stays dim
    if (Shade)
        Device->DrawTexture(X + 2, Y + 2, W - 4, H - 4, 0, 0, 1, 1, Shade, 0xC0, 1);
    Device->DrawTexture(X, Y, M, M, 0, 0, M, M, T, 0xFF, 1);
    Device->DrawTexture(X + M, Y, IW, M, M, 0, TW - 2 * M, M, T, 0xFF, 1);
    Device->DrawTexture(X + W - M, Y, M, M, TW - M, 0, M, M, T, 0xFF, 1);
    Device->DrawTexture(X, Y + M, M, IH, 0, M, M, TH - 2 * M, T, 0xFF, 1);
    Device->DrawTexture(X + M, Y + M, IW, IH, M, M, TW - 2 * M, TH - 2 * M, T, 0xFF, 1);
    Device->DrawTexture(X + W - M, Y + M, M, IH, TW - M, M, M, TH - 2 * M, T, 0xFF, 1);
    Device->DrawTexture(X, Y + H - M, M, M, 0, TH - M, M, M, T, 0xFF, 1);
    Device->DrawTexture(X + M, Y + H - M, IW, M, M, TH - M, TW - 2 * M, M, T, 0xFF, 1);
    Device->DrawTexture(X + W - M, Y + H - M, M, M, TW - M, TH - M, M, M, T, 0xFF, 1);
}

// two icons after the menu bar's four (34 x 34 at y 6, 37 apart from x 19): the shortcut bars' settings and the automatic
// potions; the bar is made wider by NCConsole::OnCreate
enum { NID_MENU_SHORTCUTS = (INT)0xFF300001, NID_MENU_AUTOPOTION = (INT)0xFF300002 };
static NCButton* GMenuIcons[NCustomShortCuts::MenuIcons];

void NCustomShortCuts::CreateMenuIcons(NCWnd* Menu)
{
    // the potion is an item icon (no frame, 32 x 32) and buttons draw 1:1: its button draws nothing and PaintMenuIcons
    // draws a stock icon's frame with the potion smaller inside
    static const TCHAR* Normal[MenuIcons] = { TEXT("L2UI_CH3.SystemMenuWnd.systemicon5"), TEXT("icon.etc_potion_scarlet_i00") };
    static const TCHAR* Pushed[MenuIcons] = { TEXT("L2UI_CH3.SystemMenuWnd.systemicon5_down"), TEXT("icon.etc_potion_scarlet_i00") };
    static const TCHAR* Tips[MenuIcons] = { TEXT("Barras de atajos (Alt+S)"), TEXT("Autopociones (Alt+A)") };
    static const INT IDs[MenuIcons] = { NID_MENU_SHORTCUTS, NID_MENU_AUTOPOTION };
    for (INT i = 0; i < MenuIcons; i++)
    {
        NCButton* Button = new NCButton(Normal[i], Pushed[i], 0);
        Button->ID = IDs[i];
        Button->SetPosition(19 + (4 + i) * MenuIconStep, 6, 34, 34);
        Menu->CreateChild(Button, NWS_VISIBLE | 0x20);   // as the stock icons (0x20: the tool tip style)
        Button->SetToolTipText(Tips[i], 1);
        if (i == 1)
            Button->NormalTex = Button->PushedTex = Button->OverTex = NULL;   // PaintMenuIcons draws it
        GMenuIcons[i] = Button;
    }
}

// the potion icon: a stock icon's frame, its inside covered with the stock icons' dark blue (a corner of the bag icon,
// stretched), the potion at 26 x 26 in the middle (a pixel lower and to the right while pressed)
void NCustomShortCuts::PaintMenuIcons(NCWnd* Menu, NCGDevice* Device)
{
    enum { Size = 26 };
    INT X = 19 + 5 * MenuIconStep, Y = 6;
    UTexture* Frame = Menu->FindTexture(TEXT("L2UI_ch3.MenuIcon.MenuButton4"));
    UTexture* Blue = Menu->FindTexture(TEXT("L2UI_ch3.MenuIcon.MenuButton2"));
    UTexture* Potion = Menu->FindTexture(TEXT("icon.etc_potion_scarlet_i00"));
    if (Frame)
        Device->DrawTexture(X, Y, 34, 34, 0, 0, 34, 34, Frame, 0xFF, 1);
    if (Blue)
        Device->DrawTexture(X + 2, Y + 2, 30, 30, 3, 3, 4, 4, Blue, 0xFF, 1);
    NCButton* Button = GMenuIcons[1];
    INT Pressed = Button && Button->State == 1 ? 1 : 0;
    if (Potion)
        Device->DrawTexture(X + (34 - Size) / 2 + Pressed, Y + (34 - Size) / 2 + Pressed, Size, Size, 0, 0, 32, 32, Potion, 0xFF, 1);
}

void NCustomShortCuts::DestroyMenuIcons(NCWnd* Menu)
{
    for (INT i = 0; i < MenuIcons; i++)
    {
        if (GMenuIcons[i])
        {
            Menu->RemoveChildWindow(GMenuIcons[i]->Entry);
            delete GMenuIcons[i];
            GMenuIcons[i] = NULL;
        }
    }
}

UBOOL NCustomShortCuts::OnMenuCommand(WPARAM ID)
{
    if (ID == (WPARAM)NID_MENU_SHORTCUTS)
    {
        ToggleOptions();
        return 1;
    }
    if (ID == (WPARAM)NID_MENU_AUTOPOTION)
    {
        NCustomAutoPotion::Toggle();
        return 1;
    }
    return 0;
}

/*-----------------------------------------------------------------------------
    NCShortCutOptionWnd: the settings window.
-----------------------------------------------------------------------------*/

// the layout (window pixels) and the controls' ids
enum
{
    OPT_Width = 256,
    OPT_Height = 262,
    OPT_ChatY = 32,          // the Enter to chat check box
    OPT_BarsY = 76,          // the extra bars row
    OPT_FirstKeysY = 132,    // the first bar's key set row
    OPT_RowStep = 20,
    OPT_DamageY = 238,       // the damage check box, under the key sets
    OPT_ValueX = 150,        // the value's center, between the two arrows
    OPT_PrevX = 98,
    OPT_NextX = 190,
    NID_OPT_FRAME = (INT)0xFF100001,
    NID_OPT_ENTERCHAT = (INT)0xFF100002,
    NID_OPT_BARS_PREV = (INT)0xFF100003,
    NID_OPT_BARS_NEXT = (INT)0xFF100004,
    NID_OPT_AUTOPOTION = (INT)0xFF100005,
    NID_OPT_DAMAGE = (INT)0xFF100006,
    NID_OPT_KEYS_PREV = (INT)0xFF100010,   // + the bar (0 .. MaxBars)
    NID_OPT_KEYS_NEXT = (INT)0xFF100020,   // + the bar
};

static NCShortCutOptionWnd* GOptionWnd = NULL;
static UBOOL GOptionShown = 0;   // the window was opened (Alt+S, right click) and not closed since
static NCCheckBox* GEnterChatBox = NULL;
static NCCheckBox* GDamageBox = NULL;
static NCButton* GBarsArrows[2] = { NULL, NULL };
static NCButton* GAutoPotionButton = NULL;
static NCButton* GKeysArrows[NCustomShortCuts::MaxBars + 1][2];

// the texts (ASCII: the game font draws nothing else)
static const TCHAR* GKeySetTexts[KS_Count] = { TEXT("Sin teclas"), TEXT("F1 - F12"), TEXT("1 - 0"), TEXT("Q - P"), TEXT("A - L"), TEXT("Z - M") };

// the bar's set moves to the next (Dir 1) or previous (-1) one no other bar uses, and is saved
static void StepKeySet(INT Bar, INT Dir)
{
    INT Set = GKeySets[Bar];
    for (INT Tries = 0; Tries < KS_Count; Tries++)
    {
        Set = (Set + Dir + KS_Count) % KS_Count;
        UBOOL bTaken = 0;
        for (INT Other = 0; Other <= NCustomShortCuts::MaxBars; Other++)
            if (Other != Bar && Set != KS_None && GKeySets[Other] == Set)
                bTaken = 1;
        if (!bTaken)
            break;
    }
    GKeySets[Bar] = Set;
    SaveKeySet(Bar);
}

static NCButton* NewArrow(NCWnd* Parent, UBOOL bNext, INT ID, INT X, INT Y)
{
    NCButton* Button = bNext
        ? new NCButton(TEXT("L2UI_CH3.ShortcutWnd.shortcut_next"), TEXT("L2UI_CH3.ShortcutWnd.shortcut_next_down"), 0)
        : new NCButton(TEXT("L2UI_CH3.ShortcutWnd.shortcut_prev"), TEXT("L2UI_CH3.ShortcutWnd.shortcut_prev_down"), 0);
    Button->ID = ID;
    Button->SetPosition(X, Y, 14, 14);
    Parent->CreateChild(Button, NWS_VISIBLE);
    return Button;
}

INT NCShortCutOptionWnd::OnCreate()
{
    NCWnd::OnCreate();
    NCFrameCtrl* Frame = new NCFrameCtrl(1, 0, 0);   // decorated: the close button
    FrameCtrl = Frame;
    FrameID = NID_OPT_FRAME;
    FrameCtrl->ID = NID_OPT_FRAME;
    FrameCtrl->SetPosition(0, 0, (DWORD)(INT)Width, 20);
    CreateChild(FrameCtrl, NWS_VISIBLE);
    SetTitle(TEXT("Barras de atajos"));

    GEnterChatBox = new NCCheckBox(TEXT("Chat con Enter"), GIsEnterChatting, 0, 0);
    GEnterChatBox->ID = NID_OPT_ENTERCHAT;
    GEnterChatBox->SetPosition(16, OPT_ChatY, 200, 12);
    CreateChild(GEnterChatBox, NWS_VISIBLE);

    GDamageBox = new NCCheckBox(TEXT("Mostrar el dano sobre el objetivo"), NCustomDamage::IsShown(), 0, 0);
    GDamageBox->ID = NID_OPT_DAMAGE;
    GDamageBox->SetPosition(16, OPT_DamageY, 220, 12);
    CreateChild(GDamageBox, NWS_VISIBLE);

    GAutoPotionButton = new NCButton(TEXT("L2UI_ch3.button.bigbutton"), TEXT("L2UI_ch3.button.bigbutton_down"), 0);
    GAutoPotionButton->ID = NID_OPT_AUTOPOTION;
    GAutoPotionButton->SetPosition(80, OPT_BarsY + 22, 96, 23);   // the art's size (buttons draw 1:1): the label centred on it
    CreateChild(GAutoPotionButton, NWS_VISIBLE);
    GAutoPotionButton->SetText(TEXT("Autopociones"));

    GBarsArrows[0] = NewArrow(this, 0, NID_OPT_BARS_PREV, OPT_PrevX, OPT_BarsY);
    GBarsArrows[1] = NewArrow(this, 1, NID_OPT_BARS_NEXT, OPT_NextX, OPT_BarsY);
    for (INT Bar = 0; Bar <= NCustomShortCuts::MaxBars; Bar++)
    {
        INT Y = OPT_FirstKeysY + Bar * OPT_RowStep;
        GKeysArrows[Bar][0] = NewArrow(this, 0, NID_OPT_KEYS_PREV + Bar, OPT_PrevX, Y);
        GKeysArrows[Bar][1] = NewArrow(this, 1, NID_OPT_KEYS_NEXT + Bar, OPT_NextX, Y);
    }
    return 1;
}

template <class T> static void DestroyOptionChild(NCWnd* Parent, T*& Child)
{
    if (Child)
    {
        Parent->RemoveChildWindow(Child->Entry);
        delete Child;
        Child = NULL;
    }
}

INT NCShortCutOptionWnd::OnDestroy()
{
    DestroyOptionChild(this, GEnterChatBox);
    DestroyOptionChild(this, GDamageBox);
    DestroyOptionChild(this, GAutoPotionButton);
    DestroyOptionChild(this, GBarsArrows[0]);
    DestroyOptionChild(this, GBarsArrows[1]);
    for (INT Bar = 0; Bar <= NCustomShortCuts::MaxBars; Bar++)
    {
        DestroyOptionChild(this, GKeysArrows[Bar][0]);
        DestroyOptionChild(this, GKeysArrows[Bar][1]);
    }
    NCWnd::OnDestroy();
    return 1;
}

// the desktop's ShowWindow shows every child: this window shows only when it was opened
void NCShortCutOptionWnd::ShowWindow()
{
    if (!GOptionShown)
        return;
    NCWnd::ShowWindow();
    if (GEnterChatBox)
        GEnterChatBox->bChecked = GIsEnterChatting;   // the game options may have changed it
}

void NCShortCutOptionWnd::HideWindow()
{
    GOptionShown = 0;
    NCWnd::HideWindow();
}

// Enter to chat as the game options set it (the global and Option.ini [Game] EnterChatting); the arrows step the extra bars
// (0 .. MaxBars) and the key sets. Everything is kept at once
INT NCShortCutOptionWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg != NWM_COMMAND)
        return 0;
    INT ID = (INT)wParam;
    if (ID == NID_OPT_ENTERCHAT)
    {
        GIsEnterChatting = GEnterChatBox->bChecked;
        GConfig->SetBool(TEXT("Game"), TEXT("EnterChatting"), GIsEnterChatting, TEXT("Option.ini"));
        return 1;
    }
    if (ID == NID_OPT_DAMAGE)
    {
        NCustomDamage::SetShown(GDamageBox->bChecked);
        return 1;
    }
    if (ID == NID_OPT_AUTOPOTION)
    {
        NCustomAutoPotion::Toggle();
        return 1;
    }
    if (ID == NID_OPT_BARS_PREV || ID == NID_OPT_BARS_NEXT)
    {
        GShownBars += ID == NID_OPT_BARS_NEXT ? 1 : -1;
        if (GShownBars < 0)
            GShownBars = 0;
        if (GShownBars > NCustomShortCuts::MaxBars)
            GShownBars = NCustomShortCuts::MaxBars;
        IniSetInt(TEXT("Bars"), GShownBars);
        return 1;
    }
    if (ID >= NID_OPT_KEYS_PREV && ID <= NID_OPT_KEYS_PREV + NCustomShortCuts::MaxBars)
    {
        StepKeySet(ID - NID_OPT_KEYS_PREV, -1);
        return 1;
    }
    if (ID >= NID_OPT_KEYS_NEXT && ID <= NID_OPT_KEYS_NEXT + NCustomShortCuts::MaxBars)
    {
        StepKeySet(ID - NID_OPT_KEYS_NEXT, 1);
        return 1;
    }
    return 0;
}

static void DrawLabel(NCGDevice* Device, INT X, INT Y, const TCHAR* Text, DWORD Color = 0xFFDCDCDC)
{
    Device->DrawText(X, Y, Color, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
}

// a value centered between its two arrows
static void DrawValue(NCGDevice* Device, INT Y, const TCHAR* Text, DWORD Color)
{
    SIZE Size;
    NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0, L2FONT_Normal);
    DrawLabel(Device, OPT_ValueX - Size.cx / 2, Y, Text, Color);
}

INT NCShortCutOptionWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    NCustomShortCuts::DrawPanel(this, Device, 0, 18, (INT)Width, (INT)Height - 18);
    DrawLabel(Device, 32, OPT_ChatY + 16, TEXT("(letras y numeros requieren Enter)"), 0xFFA0A0A0);
    DrawLabel(Device, 16, OPT_BarsY + 1, TEXT("Barras extra"));
    TCHAR Text[16];
    appSprintf(Text, TEXT("%d"), GShownBars);
    DrawValue(Device, OPT_BarsY + 1, Text, 0xFFFFFFFF);
    for (INT Bar = 0; Bar <= NCustomShortCuts::MaxBars; Bar++)
    {
        INT Y = OPT_FirstKeysY + Bar * OPT_RowStep + 1;
        UBOOL bShown = Bar == 0 || Bar <= GShownBars;
        DWORD Color = bShown ? 0xFFDCDCDC : 0xFF808080;   // the rows of hidden bars dimmed
        appSprintf(Text, TEXT("Barra %d"), Bar + 1);
        DrawLabel(Device, 16, Y, Text, Color);
        INT Set = GKeySets[Bar];
        UBOOL bNeedsChat = Set != KS_None && Set != KS_F && !GIsEnterChatting;
        DrawValue(Device, Y, GKeySetTexts[Set], bNeedsChat ? 0xFFB08040 : (bShown ? 0xFFFFFFFF : 0xFF808080));
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}

void NCustomShortCuts::ToggleOptions()
{
    if (!GOptionWnd)
        return;
    if (GOptionShown)
        GOptionWnd->HideWindow();
    else
    {
        GOptionShown = 1;
        GOptionWnd->ShowWindow();
        GDesktop->SetHeadWindow(GOptionWnd->Entry);
    }
}

void NCustomShortCuts::Create(NCConsole* Desktop)
{
    NCShortCutWnd* Main = Desktop->ShortCutWnd;
    if (!Main)
        return;
    LoadKeySets();
    GShownBars = IniGetInt(TEXT("Bars"), 0);
    if (GShownBars < 0 || GShownBars > MaxBars)
        GShownBars = 0;
    for (INT i = 0; i < MaxBars; i++)
    {
        TCHAR Key[16];
        PageKey(Key, i);
        INT Page = IniGetInt(Key, i + 1);
        if (Page < 0 || Page >= Main->Pages)
            Page = (i + 1) % Main->Pages;
        GSavedPages[i] = Page;

        NCExtraShortCutWnd* W = new NCExtraShortCutWnd;
        GBars[i] = W;   // before OnCreate (BarIndex)
        W->SetPosition(Main->ScreenX, Main->ScreenY, 0x2E, 0x1F8, 0, 0, 3);
        NWndEntry* E = Desktop->AddChildWindow(W, 0x85022);   // the main bar's style
        W->HideWindow();
        W->AttachEntry(E);   // WM_CREATE -> OnCreate
        W->Unknown78 = 0;    // no number: WindowsInfo.ini leaves it alone
        W->Page = Page;
    }

    GExpandButton = new NCPushButton(TEXT("L2UI_CH3.QUESTWND.QuestWndPlusBtn"), TEXT("L2UI_CH3.QUESTWND.QuestWndPlusBtn_over"));
    GExpandButton->ID = NID_SHORTCUT_EXPAND;
    GExpandButton->SetPosition(0x1F, 0x1E9, 0xF, 0xF);
    Main->CreateChild(GExpandButton, NWS_VISIBLE);
    GLocked = IniGetInt(TEXT("Locked"), 0) != 0;
    GLockButton = new NCLockButton;
    GLockButton->ID = NID_SHORTCUT_LOCK;
    GLockButton->SetPosition(1, 0x1E9, 0xF, 0xF);
    Main->CreateChild(GLockButton, NWS_VISIBLE);
    GLockButton->SetToolTipText(TEXT("Bloquear / desbloquear las barras"), 1);
    GLayoutHorizontal = -1;
    GLayoutMaxed = -1;

    // the settings window, in the middle of the screen
    GOptionWnd = new NCShortCutOptionWnd;
    GOptionWnd->SetPosition((INT)(Desktop->Width * 0.5f) - OPT_Width - 10, (INT)(Desktop->Height * 0.5f) - OPT_Height / 2,
                            OPT_Width, OPT_Height, 0, 0, 3);
    NWndEntry* E = Desktop->AddChildWindow(GOptionWnd, 0x1002);   // on top, like the desktop's dialogs
    GOptionWnd->AttachEntry(E);
    GOptionWnd->HideWindow();   // after the registration: before it the window stays flagged as shown
    GOptionWnd->Unknown78 = 0;

    NCustomAutoPotion::Create(Desktop);
}

void NCustomShortCuts::Destroy(NCConsole* Desktop)
{
    NCustomAutoPotion::Destroy(Desktop);
    if (GOptionWnd)
    {
        Desktop->RemoveChildWindow(GOptionWnd->Entry);
        delete GOptionWnd;
        GOptionWnd = NULL;
    }
    NCShortCutWnd* Main = Desktop->ShortCutWnd;
    if (GExpandButton && Main)
    {
        Main->RemoveChildWindow(GExpandButton->Entry);
        delete GExpandButton;
    }
    GExpandButton = NULL;
    if (GLockButton && Main)
    {
        Main->RemoveChildWindow(GLockButton->Entry);
        delete GLockButton;
    }
    GLockButton = NULL;
    for (INT i = 0; i < MaxBars; i++)
    {
        if (GBars[i])
        {
            Desktop->RemoveChildWindow(GBars[i]->Entry);
            delete GBars[i];
            GBars[i] = NULL;
        }
    }
}

void NCustomShortCuts::Tick()
{
    NCustomAutoPotion::Tick();
    NCWnd* Menu = GConsole && GConsole->Desktop ? (NCWnd*)GDesktop->MenuWnd : NULL;
    if (Menu && GDesktop->Width > 0 && Menu->ScreenX + Menu->Width > GDesktop->Width)
        Menu->SetPosition((INT)(GDesktop->Width - Menu->Width), Menu->ScreenY, 0, 0, 0, 0, 3);   // a place saved for the narrower bar
    NCShortCutWnd* Main = MainBar();
    if (!Main || !GBars[0])
        return;
    UpdateExpandButton(Main);
    UBOOL bRelayout = GLayoutHorizontal != Main->bHorizontal;
    GLayoutHorizontal = Main->bHorizontal;

    UBOOL bMainShown = Main->IsShowWindow() && Main->bPadMode == 0;
    INT Step = 0x2E;
    for (INT i = 0; i < MaxBars; i++)
    {
        NCExtraShortCutWnd* W = GBars[i];
        if (!W)
            continue;
        UBOOL bShow = bMainShown && i < GShownBars;
        if (!bShow)
        {
            if (W->IsShowWindow())
                W->HideWindow();
            continue;
        }
        if (bRelayout || W->bHorizontal != Main->bHorizontal)
        {
            W->bHorizontal = Main->bHorizontal;
            W->UpdateLayout();
            W->RotateButton->HideWindow();
        }
        INT X = Main->ScreenX;
        INT Y = Main->ScreenY;
        if (!Main->bHorizontal)
        {
            // to the left; to the right when the shown set does not fit
            if (Main->ScreenX - GShownBars * Step >= 0)
                X = Main->ScreenX - (i + 1) * Step;
            else
                X = Main->ScreenX + (i + 1) * Step;
        }
        else
        {
            // above; below when the shown set does not fit
            if (Main->ScreenY - GShownBars * Step >= 0)
                Y = Main->ScreenY - (i + 1) * Step;
            else
                Y = Main->ScreenY + (i + 1) * Step;
        }
        if (W->ScreenX != X || W->ScreenY != Y)
            W->SetPosition(X, Y, 0, 0, 0, 0, 3);
        if (!W->IsShowWindow())
            W->ShowWindow();
        if (W->Page != GSavedPages[i])
        {
            TCHAR Key[16];
            PageKey(Key, i);
            IniSetInt(Key, W->Page);
            GSavedPages[i] = W->Page;
        }
    }
}

UBOOL NCustomShortCuts::OnCommand(NCShortCutWnd* Wnd, WPARAM ID)
{
    if (Wnd != MainBar())
        return 0;
    if (ID == (WPARAM)NID_SHORTCUT_LOCK)
    {
        Wnd->bDragging = 0;
        GLocked = !GLocked;
        IniSetInt(TEXT("Locked"), GLocked);
        return 1;
    }
    if (ID != (WPARAM)NID_SHORTCUT_EXPAND)
        return 0;
    Wnd->bDragging = 0;
    GShownBars = GShownBars >= MaxBars ? 0 : GShownBars + 1;
    IniSetInt(TEXT("Bars"), GShownBars);
    return 1;
}

/*-----------------------------------------------------------------------------
    Key sets.
-----------------------------------------------------------------------------*/

// the bar of a key-set index (0 the main bar), NULL when it is not made
static NCShortCutWnd* BarAt(INT Bar)
{
    if (Bar == 0)
        return MainBar();
    return GBars[Bar - 1];
}

static INT KeySetBar(const NCShortCutWnd* Wnd)
{
    if (Wnd && Wnd == MainBar())
        return 0;
    INT Index = NCustomShortCuts::IndexOf(Wnd);
    return Index >= 0 ? Index + 1 : -1;
}

// the slot position (0 .. 11) the key has in the set, -1 none; the rows go by the scan code (lParam bits 16 .. 23), not by
// extended keys (the keypad's / and the like)
static INT KeyIndex(INT Set, WPARAM wParam, LPARAM lParam)
{
    if (Set == KS_F)
        return (wParam >= VK_F1 && wParam <= VK_F12) ? (INT)(wParam - VK_F1) : -1;
    if (Set == KS_None || (lParam & 0x01000000))
        return -1;
    BYTE Scan = (BYTE)((lParam >> 16) & 0xFF);
    if (!Scan)
        return -1;
    for (INT i = 0; i < 12; i++)
        if (GKeyRows[Set][i] == Scan)
            return i;
    return -1;
}

// a key of a set some shown bar has goes to that bar's slot (as F1 .. F12 there): the F set always, the others while the chat
// needs Enter and no edit box has the focus. The main bar's own F keys stay with the stock code
UBOOL NCustomShortCuts::OnKey(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg == WM_SYSKEYDOWN && wParam == 'S' && GConsole->UIMode == 0)   // Alt+S: the settings window
    {
        ToggleOptions();
        return 1;
    }
    if (Msg == WM_SYSKEYDOWN && wParam == 'A' && GConsole->UIMode == 0)   // Alt+A: the automatic potions
    {
        NCustomAutoPotion::Toggle();
        return 1;
    }
    if (Msg != WM_KEYDOWN && Msg != WM_KEYUP)
        return 0;
    UBOOL bTyping = !GIsEnterChatting || (GFocusEntry && GFocusEntry->Wnd && GFocusEntry->Wnd->vfn032());
    for (INT Bar = 0; Bar <= MaxBars; Bar++)
    {
        INT Set = GKeySets[Bar];
        if (Set == KS_None || (Set != KS_F && bTyping))
            continue;
        if (Set == KS_F && Bar == 0)
            continue;
        NCShortCutWnd* W = BarAt(Bar);
        if (!W || !W->IsShowWindow() || W->bPadMode != 0)
            continue;
        INT Index = KeyIndex(Set, wParam, lParam);
        if (Index < 0)
            continue;
        if (Msg == WM_KEYDOWN)
            W->OnShortCutKeyDown(VK_F1 + Index, lParam);
        else
            W->OnShortCutKeyUp(VK_F1 + Index, lParam);
        return 1;
    }
    return 0;
}

UTexture* NCustomShortCuts::KeyTexture(NCShortCutWnd* Wnd, INT Index)
{
    INT Bar = KeySetBar(Wnd);
    if (Bar < 0 || GKeySets[Bar] == KS_F)
        return Wnd->KeyTex[Index];
    return NULL;
}

// the character of a row key in the keyboard layout in use ("" for none)
static void KeyLabel(INT Set, INT Index, TCHAR* Out)
{
    Out[0] = 0;
    BYTE Scan = GKeyRows[Set][Index];
    if (!Scan)
        return;
    UINT VK = MapVirtualKeyW(Scan, MAPVK_VSC_TO_VK);
    UINT Char = MapVirtualKeyW(VK, MAPVK_VK_TO_CHAR) & 0x7FFFFFFF;   // the high bit marks a dead key
    if (!Char)
        return;
    Out[0] = (TCHAR)towupper((wint_t)Char);
    Out[1] = 0;
}

// the labels where the F-key textures would be (PaintBar has the device's origin at the bar); the sets that need Enter to
// chat are drawn only while it is on
void NCustomShortCuts::PaintKeyLabels(NCShortCutWnd* Wnd, NCGDevice* Device)
{
    INT Bar = KeySetBar(Wnd);
    if (Bar < 0)
        return;
    INT Set = GKeySets[Bar];
    if (Set == KS_None || Set == KS_F || !GIsEnterChatting)
        return;
    INT Pos = 0x21;
    for (INT i = 0; i < Wnd->Cols; i++)
    {
        TCHAR Label[2];
        KeyLabel(Set, i, Label);
        if (Label[0])
        {
            if (Wnd->bHorizontal)
                Device->DrawText(Pos, 7, 0xFFDCDCDC, Label, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
            else
                Device->DrawText(7, Pos, 0xFFDCDCDC, Label, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
        }
        Pos += Wnd->IconSize + 5;
        if (i == 3 || i == 7)
            Pos += 5;
    }
}

// a right click on a bar outside its slots opens (or closes) the settings window
UBOOL NCustomShortCuts::OnRightClick(NCShortCutWnd* Wnd)
{
    if (KeySetBar(Wnd) < 0)
        return 0;
    ToggleOptions();
    return 1;
}
