/*=============================================================================
    NCustomAutoPotion.cpp - automatic potions (custom, NW_CUSTOM builds only). See NCustomAutoPotion.h.
=============================================================================*/
#include "nwindow/NCustomAutoPotion.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCCheckBox.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCInventoryWnd.h"
#include "nwindow/NCInvenItemWnd.h"
#include "nwindow/NCItemWnd.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NCustomShortCuts.h"

// the rows
enum { POT_HP, POT_MP, POT_CP, POT_Count };

// the layout (window pixels) and the ids
enum
{
    APW_Width = 256,
    APW_Height = 228,
    APW_SlotX = 12,
    APW_TextX = 52,
    APW_LabelX = 108,
    APW_PrevX = 140,
    APW_ValueX = 172,        // the value's center, between the two arrows
    APW_NextX = 192,
    NID_APW_FRAME = (INT)0xFF200001,
    NID_APW_ON = (INT)0xFF200010,          // + the row
    NID_APW_BELOW_PREV = (INT)0xFF200020,  // + the row
    NID_APW_BELOW_NEXT = (INT)0xFF200030,
    NID_APW_EVERY_PREV = (INT)0xFF200040,
    NID_APW_EVERY_NEXT = (INT)0xFF200050,
};
static const INT GRowY[POT_Count] = { 28, 82, 136 };

// the times a row may wait between two uses (ms)
static const INT GIntervals[] = { 500, 1000, 2000, 3000, 5000, 10000, 15000, 20000, 30000 };
enum { NUM_INTERVALS = sizeof(GIntervals) / sizeof(GIntervals[0]) };

struct NAutoPotion
{
    INT ItemID;              // 0 = none
    UBOOL bOn;
    INT Below;               // percent (5 .. 95)
    INT Every;               // ms (one of GIntervals)
    DWORD LastUse;           // GetTickCount of the last use
    TCHAR Name[64];          // kept from the drop: the row shows it while the inventory has none
    TCHAR IconName[128];
};

static const TCHAR* GRowKeys[POT_Count] = { TEXT("HP"), TEXT("MP"), TEXT("CP") };
static const INT GDefaultBelow[POT_Count] = { 70, 50, 80 };
static const INT GDefaultEvery[POT_Count] = { 10000, 1000, 500 };   // HP potions: 9 s reuse on the server

static NAutoPotion GPotions[POT_Count];
static NCAutoPotionWnd* GWnd = NULL;
static UBOOL GShown = 0;
static NCCheckBox* GOnBoxes[POT_Count];
static NCButton* GArrows[POT_Count][4];   // below prev / next, every prev / next

/*-----------------------------------------------------------------------------
    Settings: [AutoPotion] of system\NWindowCustom.ini.
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

static void RowKey(TCHAR* Out, INT Row, const TCHAR* Name)
{
    swprintf_s(Out, 32, TEXT("%s%s"), GRowKeys[Row], Name);
}

static INT IniGet(INT Row, const TCHAR* Name, INT Default)
{
    TCHAR Key[32];
    RowKey(Key, Row, Name);
    return (INT)GetPrivateProfileIntW(TEXT("AutoPotion"), Key, Default, IniPath());
}

static void IniSetString(INT Row, const TCHAR* Name, const TCHAR* Value)
{
    TCHAR Key[32];
    RowKey(Key, Row, Name);
    WritePrivateProfileStringW(TEXT("AutoPotion"), Key, Value, IniPath());
}

static void IniSet(INT Row, const TCHAR* Name, INT Value)
{
    TCHAR Text[16];
    swprintf_s(Text, 16, TEXT("%d"), Value);
    IniSetString(Row, Name, Text);
}

static void Load()
{
    for (INT Row = 0; Row < POT_Count; Row++)
    {
        NAutoPotion& P = GPotions[Row];
        P.ItemID = IniGet(Row, TEXT("Item"), 0);
        P.bOn = IniGet(Row, TEXT("On"), 0) != 0;
        P.Below = IniGet(Row, TEXT("Below"), GDefaultBelow[Row]);
        if (P.Below < 5 || P.Below > 95)
            P.Below = GDefaultBelow[Row];
        P.Every = IniGet(Row, TEXT("Every"), GDefaultEvery[Row]);
        UBOOL bValid = 0;
        for (INT i = 0; i < NUM_INTERVALS; i++)
            if (GIntervals[i] == P.Every)
                bValid = 1;
        if (!bValid)
            P.Every = GDefaultEvery[Row];
        P.LastUse = 0;
        TCHAR Key[32];
        RowKey(Key, Row, TEXT("Name"));
        GetPrivateProfileStringW(TEXT("AutoPotion"), Key, TEXT(""), P.Name, 64, IniPath());
        RowKey(Key, Row, TEXT("Icon"));
        GetPrivateProfileStringW(TEXT("AutoPotion"), Key, TEXT(""), P.IconName, 128, IniPath());
    }
}

/*-----------------------------------------------------------------------------
    The inventory and the player.
-----------------------------------------------------------------------------*/

// the first stack of the item in the inventory's "Item" page, NULL when there is none
static NItemEntry* FindPotion(INT ItemID)
{
    if (!ItemID || !GConsole || !GConsole->Desktop)
        return NULL;
    NCInventoryWnd* Inventory = GDesktop->InventoryWnd;
    if (!Inventory || !Inventory->ItemWnd[0])
        return NULL;
    TArray<NItemEntry*>& Items = Inventory->ItemWnd[0]->Items;
    for (INT i = 0; i < Items.Num(); i++)
        if (Items(i) && Items(i)->ID == ItemID && Items(i)->Count > 0)
            return Items(i);
    return NULL;
}

// the item's icon texture name (NItemEntry +0x414, as the shortcut bar reads it)
static const TCHAR* EntryIconName(NItemEntry* E)
{
    return (const TCHAR*)((BYTE*)E + 0x414);
}

// the row's current and maximum points, 0 / 0 when unknown
static void RowPoints(User* Player, INT Row, INT& Cur, INT& Max)
{
    Cur = Max = 0;
    if (Row == POT_HP)
    {
        Cur = Player->HP;
        Max = Player->MaxHP;
    }
    else if (Row == POT_MP)
    {
        Cur = Player->MP;
        Max = Player->MaxMP;
    }
    else
    {
        Max = nw::Field<INT>(Player, 0x214);   // C4: the CP's maximum and the CP (NCPlayerStatusWnd::OnPaint)
        Cur = nw::Field<INT>(Player, 0x218);
    }
}

void NCustomAutoPotion::Tick()
{
    if (!GConsole || GConsole->State != L2CS_InGame)
        return;
    User* Player = GConsole->GetMyUser();
    if (!Player || Player->HP <= 0)
        return;
    DWORD Now = GetTickCount();
    for (INT Row = 0; Row < POT_Count; Row++)
    {
        NAutoPotion& P = GPotions[Row];
        if (!P.bOn || !P.ItemID)
            continue;
        if (P.LastUse && Now - P.LastUse < (DWORD)P.Every)
            continue;
        INT Cur, Max;
        RowPoints(Player, Row, Cur, Max);
        if (Max <= 0 || (DOUBLE)Cur * 100.0 >= (DOUBLE)Max * P.Below)
            continue;
        NItemEntry* Item = FindPotion(P.ItemID);
        if (!Item)
            continue;
        GConsole->RequestUseItem(Item->ObjectID);
        P.LastUse = Now;
    }
}

/*-----------------------------------------------------------------------------
    NCAutoPotionWnd.
-----------------------------------------------------------------------------*/

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

INT NCAutoPotionWnd::OnCreate()
{
    NCWnd::OnCreate();
    NCFrameCtrl* Frame = new NCFrameCtrl(1, 0, 0);   // decorated: the close button
    FrameCtrl = Frame;
    FrameID = NID_APW_FRAME;
    FrameCtrl->ID = NID_APW_FRAME;
    FrameCtrl->SetPosition(0, 0, (DWORD)(INT)Width, 20);
    CreateChild(FrameCtrl, NWS_VISIBLE);
    SetTitle(TEXT("Autopociones"));
    for (INT Row = 0; Row < POT_Count; Row++)
    {
        INT Y = GRowY[Row];
        GOnBoxes[Row] = new NCCheckBox(TEXT("Auto"), GPotions[Row].bOn, 0, 0);
        GOnBoxes[Row]->ID = NID_APW_ON + Row;
        GOnBoxes[Row]->SetPosition(APW_TextX, Y + 18, 50, 12);
        CreateChild(GOnBoxes[Row], NWS_VISIBLE);
        GArrows[Row][0] = NewArrow(this, 0, NID_APW_BELOW_PREV + Row, APW_PrevX, Y + 17);
        GArrows[Row][1] = NewArrow(this, 1, NID_APW_BELOW_NEXT + Row, APW_NextX, Y + 17);
        GArrows[Row][2] = NewArrow(this, 0, NID_APW_EVERY_PREV + Row, APW_PrevX, Y + 33);
        GArrows[Row][3] = NewArrow(this, 1, NID_APW_EVERY_NEXT + Row, APW_NextX, Y + 33);
    }
    return 1;
}

template <class T> static void DestroyChildOf(NCWnd* Parent, T*& Child)
{
    if (Child)
    {
        Parent->RemoveChildWindow(Child->Entry);
        delete Child;
        Child = NULL;
    }
}

INT NCAutoPotionWnd::OnDestroy()
{
    for (INT Row = 0; Row < POT_Count; Row++)
    {
        DestroyChildOf(this, GOnBoxes[Row]);
        for (INT i = 0; i < 4; i++)
            DestroyChildOf(this, GArrows[Row][i]);
    }
    NCWnd::OnDestroy();
    return 1;
}

// the desktop's ShowWindow shows every child: this window shows only when it was opened
void NCAutoPotionWnd::ShowWindow()
{
    if (!GShown)
        return;
    NCWnd::ShowWindow();
    for (INT Row = 0; Row < POT_Count; Row++)
        if (GOnBoxes[Row])
            GOnBoxes[Row]->bChecked = GPotions[Row].bOn;
}

void NCAutoPotionWnd::HideWindow()
{
    GShown = 0;
    NCWnd::HideWindow();
}

static void StepInterval(NAutoPotion& P, INT Dir)
{
    INT Index = 0;
    for (INT i = 0; i < NUM_INTERVALS; i++)
        if (GIntervals[i] == P.Every)
            Index = i;
    Index += Dir;
    if (Index < 0)
        Index = 0;
    if (Index >= NUM_INTERVALS)
        Index = NUM_INTERVALS - 1;
    P.Every = GIntervals[Index];
}

// every change is kept at once
INT NCAutoPotionWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg != NWM_COMMAND)
        return 0;
    INT ID = (INT)wParam;
    INT Group = ID & ~0xF;
    INT Row = ID & 0xF;
    if (Row >= POT_Count)
        return 0;
    NAutoPotion& P = GPotions[Row];
    if (Group == NID_APW_ON)
    {
        P.bOn = GOnBoxes[Row]->bChecked;
        P.LastUse = 0;
        IniSet(Row, TEXT("On"), P.bOn);
    }
    else if (Group == NID_APW_BELOW_PREV || Group == NID_APW_BELOW_NEXT)
    {
        P.Below += Group == NID_APW_BELOW_NEXT ? 5 : -5;
        if (P.Below < 5)
            P.Below = 5;
        if (P.Below > 95)
            P.Below = 95;
        IniSet(Row, TEXT("Below"), P.Below);
    }
    else if (Group == NID_APW_EVERY_PREV || Group == NID_APW_EVERY_NEXT)
    {
        StepInterval(P, Group == NID_APW_EVERY_NEXT ? 1 : -1);
        IniSet(Row, TEXT("Every"), P.Every);
    }
    else
        return 0;
    return 1;
}

// the row whose band (the slot's height and the two lines beside it) holds the window point, -1 none
static INT RowAt(INT RelY)
{
    for (INT Row = 0; Row < POT_Count; Row++)
        if (RelY >= GRowY[Row] - 2 && RelY <= GRowY[Row] + 48)
            return Row;
    return -1;
}

static void ClearRow(INT Row)
{
    NAutoPotion& P = GPotions[Row];
    P.ItemID = 0;
    P.Name[0] = 0;
    P.IconName[0] = 0;
    IniSet(Row, TEXT("Item"), 0);
    IniSetString(Row, TEXT("Name"), TEXT(""));
    IniSetString(Row, TEXT("Icon"), TEXT(""));
}

// a right click on a row's slot clears it
INT NCAutoPotionWnd::OnRButtonDown(WPARAM wParam, LPARAM lParam)
{
    INT RelX = LOWORD(lParam) - ScreenX;
    INT Row = RowAt(HIWORD(lParam) - ScreenY);
    if (Row >= 0 && RelX >= APW_SlotX && RelX < APW_SlotX + 34)
        ClearRow(Row);
    return 1;
}

UBOOL NCustomAutoPotion::OnDropItem(NCWnd* Target, INT X, INT Y, NItemEntry* Item)
{
    if (!GWnd || !GShown || !Target || !Item)
        return 0;
    if (Target != GWnd && !(Target->Entry && Target->Entry->Parent && Target->Entry->Parent->Wnd == GWnd))
        return 0;
    INT Row = RowAt(Y - GWnd->ScreenY);
    if (Row < 0)
        return 1;   // over the window but no row: nothing (the item is not dropped on the ground)
    NAutoPotion& P = GPotions[Row];
    P.ItemID = Item->ID;
    appStrncpy(P.Name, Item->GetName() ? Item->GetName() : TEXT(""), 64);
    appStrncpy(P.IconName, EntryIconName(Item), 128);
    P.LastUse = 0;
    IniSet(Row, TEXT("Item"), P.ItemID);
    IniSetString(Row, TEXT("Name"), P.Name);
    IniSetString(Row, TEXT("Icon"), P.IconName);
    GConsole->PlayInterfaceSound(1);
    return 1;
}

static void DrawLabel(NCGDevice* Device, INT X, INT Y, const TCHAR* Text, DWORD Color = 0xFFDCDCDC)
{
    Device->DrawText(X, Y, Color, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
}

static void DrawValue(NCGDevice* Device, INT Y, const TCHAR* Text)
{
    SIZE Size;
    NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0, L2FONT_Normal);
    DrawLabel(Device, APW_ValueX - Size.cx / 2, Y, Text, 0xFFFFFFFF);
}


INT NCAutoPotionWnd::OnPaint(NCGDevice* Device)
{
    static const DWORD RowColors[POT_Count] = { 0xFFE05050, 0xFF5080FF, 0xFFE0B040 };
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    NCustomShortCuts::DrawPanel(this, Device, 0, 18, (INT)Width, (INT)Height - 18);
    UTexture* Frame = FindTexture(TEXT("L2UI_CH3.Etc.menu_outline"));
    for (INT Row = 0; Row < POT_Count; Row++)
    {
        NAutoPotion& P = GPotions[Row];
        INT Y = GRowY[Row];
        NItemEntry* Item = FindPotion(P.ItemID);
        UTexture* Icon = Item ? Item->GetIcon() : (P.IconName[0] ? FindTexture(P.IconName) : NULL);
        if (Icon)
            Device->DrawTexture(APW_SlotX + 1, Y + 1, 32, 32, 0, 0, 32, 32, Icon, Item ? 0xFF : 0x60, 1);
        Device->DrawTexture(APW_SlotX, Y, 34, 34, 0, 0, 34, 34, Frame, 0xFF, 1);

        TCHAR Text[128];
        DrawLabel(Device, APW_TextX, Y + 1, GRowKeys[Row], RowColors[Row]);
        if (!P.ItemID)
            DrawLabel(Device, APW_TextX + 24, Y + 1, TEXT("Arrastrar una pocion"), 0xFF808080);
        else
        {
            if (Item)
                appSprintf(Text, TEXT("%s (%d)"), P.Name, Item->Count);
            else
                appSprintf(Text, TEXT("%s (0)"), P.Name);
            DrawLabel(Device, APW_TextX + 24, Y + 1, Text, Item ? 0xFFDCDCDC : 0xFFB04040);
        }
        DrawLabel(Device, APW_LabelX, Y + 18, TEXT("Bajo"));
        appSprintf(Text, TEXT("%d%%"), P.Below);
        DrawValue(Device, Y + 18, Text);
        DrawLabel(Device, APW_LabelX, Y + 34, TEXT("Cada"));
        if (P.Every < 1000)
            appSprintf(Text, TEXT("%d.%d s"), P.Every / 1000, (P.Every % 1000) / 100);
        else
            appSprintf(Text, TEXT("%d s"), P.Every / 1000);
        DrawValue(Device, Y + 34, Text);
    }
    DrawLabel(Device, 12, 192, TEXT("Arrastrar pociones desde el inventario."), 0xFFA0A0A0);
    DrawLabel(Device, 12, 206, TEXT("Clic derecho en la casilla: quitar."), 0xFFA0A0A0);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}

/*-----------------------------------------------------------------------------
    The window's life.
-----------------------------------------------------------------------------*/

void NCustomAutoPotion::Create(NCConsole* Desktop)
{
    Load();
    GWnd = new NCAutoPotionWnd;
    GWnd->SetPosition((INT)(Desktop->Width * 0.5f) + 10, (INT)(Desktop->Height * 0.5f) - APW_Height / 2,
                      APW_Width, APW_Height, 0, 0, 3);
    NWndEntry* E = Desktop->AddChildWindow(GWnd, 0x1002);   // on top, like the desktop's dialogs
    GWnd->AttachEntry(E);
    GWnd->HideWindow();
    GWnd->Unknown78 = 0;   // no number: WindowsInfo.ini leaves it alone
}

void NCustomAutoPotion::Destroy(NCConsole* Desktop)
{
    if (GWnd)
    {
        Desktop->RemoveChildWindow(GWnd->Entry);
        delete GWnd;
        GWnd = NULL;
    }
}

void NCustomAutoPotion::Toggle()
{
    if (!GWnd)
        return;
    if (GShown)
        GWnd->HideWindow();
    else
    {
        GShown = 1;
        GWnd->ShowWindow();
        GDesktop->SetHeadWindow(GWnd->Entry);
    }
}
