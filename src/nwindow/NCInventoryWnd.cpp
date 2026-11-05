/*=============================================================================
    NCInventoryWnd.cpp - the inventory. Stock: 0x100641f0 .. 0x10064289, 0x10065920 ..
    0x10066182, 0x100681a0 .. 0x10068373 and OnCreate (0x100684d0); the item pages' and the
    paperdoll's functions sit in between.
=============================================================================*/
#include "nwindow/NCInventoryWnd.h"
#include "nwindow/NCEquipItemWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCIcon.h"
#include "nwindow/NCInvenItemWnd.h"
#include "nwindow/NCInvenWeightWnd.h"
#include "nwindow/NCTabControl.h"
#include "nwindow/NItemC4.h"
#if NW_CUSTOM
#include "nwindow/NCustomInventory.h"
#endif

#include <stdio.h>
#include <wchar.h>

// ids (names ours)
enum
{
    NID_INVENTORY_FRAME = 0xF2,
    NID_INVENTORY_TAB = 0xFE001A,
    NID_INVENTORY_MANUFACTURE = 0xFE001C,
};

// sysstring
enum
{
    STR_Item = 2,
    STR_Quest = 118,
};

// NCInvenWeightWnd (UClass 0x102DA3D8, size 0xFC, constructor 0x10099840): the weight bar, new in C4
static NCWnd* NewInvenWeightWnd()
{
    return new NCInvenWeightWnd();
}

// NCTabControl::AddTab (0x1002DED0) takes nine arguments in C4 (a trailing 0): the textures are the inventory's tabs, 94 x 23,
// a gap of 1, no tool tip
static FORCEINLINE void AddInvenTab(NCTabControl* Tabs, INT TextID, NCWnd* Page)
{
    Tabs->AddTab(TextID, Page, 0x5E, 0x17, TEXT("L2UI_CH3.InventoryWnd.Inventory_tab2"),
                 TEXT("L2UI_CH3.InventoryWnd.Inventory_tab1"), 1, -1, 0);
}

// the stock code reads the player's crystallize flag 4 bytes after where UE2Engine.h puts User::bCanCrystallize (+0x1F8)
static FORCEINLINE INT CanCrystallize(User* Me)
{
    return ::nw::Field<INT>(Me, 0x1F8);
}

// 0x10099900 - the pages, the paperdoll and the buttons are made by OnCreate; the trash icon's box (208, 351; 34 x 34)
// and the recipe icon's (14, 351)
NCInventoryWnd::NCInventoryWnd()
{
    BackTex = NULL;
    TabCtrl = NULL;
    TrashY = 0;
    TrashX = 0;
    TrashW = 0;
    TrashH = 0;
    RecipeY = 0;
    RecipeX = 0;
    RecipeW = 0;
    RecipeH = 0;
    ItemWnd[0] = NULL;
    ItemWnd[1] = NULL;
    bOverRecipe = 0;
    bOverTrash = 0;
    TrashButton = NULL;
    RecipeButton = NULL;
    TrashW = 34.0f;
    TrashH = 34.0f;
    TrashX = 0xD0;
    TrashY = 0x15F;
    RecipeW = 34.0f;
    RecipeH = 34.0f;
    RecipeX = 0xE;
    RecipeY = 0x15F;
#if NW_CUSTOM
    TrashX += NCustomInventory::LowerShiftX;   // custom: the adena box is under the grid in the wide window
    TrashY += NCustomInventory::LowerShiftY;
    RecipeX += NCustomInventory::LowerShiftX;
    RecipeY += NCustomInventory::LowerShiftY;
#endif
}
NW_HOOK_CTOR(0x10099900, NCInventoryWnd)

// 0x10099a00 - the order list, then the window's
NW_HOOK_DTOR(0x10099a00, NCInventoryWnd)

// 0x10148f10 (vslot 25) - NControl::WindowProc's body, which the stock vtables of these windows share: NCWnd's, as 0 or 1
INT NCInventoryWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
#if NW_CUSTOM
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    return Msg == NWM_COMMAND && NCustomInventory::OnCommand(this, (INT)wParam);   // custom: the tabs and the sort button
#else
    return NCWnd::WindowProc(Msg, wParam, lParam) != 0;
#endif
}

// 0x10093f00 (ret 8) - the point is inside the trash icon's box (from this window's corner; the box is a position
// and a size in floats), edges included
UBOOL NCInventoryWnd::IsInTrash(INT X, INT Y)
{
    INT RelX = X - (TrashX + ScreenX);
    INT RelY = Y - (TrashY + ScreenY);
    if (RelX >= 0 && RelX <= (INT)TrashW && RelY >= 0 && RelY <= (INT)TrashH)
        return 1;
    return 0;
}
NW_HOOK_METHOD(0x10093f00, NCInventoryWnd, IsInTrash)

// 0x10093f60 (ret 8) - the same for the recipe icon's box
UBOOL NCInventoryWnd::IsInRecipe(INT X, INT Y)
{
    INT RelX = X - (RecipeX + ScreenX);
    INT RelY = Y - (RecipeY + ScreenY);
    if (RelX >= 0 && RelX <= (INT)RecipeW && RelY >= 0 && RelY <= (INT)RecipeH)
        return 1;
    return 0;
}
NW_HOOK_METHOD(0x10093f60, NCInventoryWnd, IsInRecipe)

// (the mouse handlers, slots 67 .. 69, are NCWnd's own: the stock vtable of this class points at 0x10036240, 0x10036350 and 0x10036410,
// so the class has no override)

// 0x10093ea0 (vslot 54) - minimizing: the icon shows the inventory's picture (and is brought in when the window was
// left of the screen), the window hides and the icon takes the focus
void NCInventoryWnd::Minimize()
{
    if (Icon)
        Icon->SetIcon(TEXT("L2UI_CH3.MenuIcon.Menuicon2"), 0xC3, 1);
    if (ScreenX < 0 && Icon)
        Icon->OffsetWindow(-ScreenX, 0);
    HideWindow();
    if (Icon)
    {
        Icon->ShowWindow();
        Icon->SetFocus();
    }
}
NW_HOOK_METHOD(0x10093ea0, NCInventoryWnd, Minimize)

// 0x100982c0 - the ids of the "Item" page in its order, written to the file of the character (..\iwo)
void NCInventoryWnd::SaveItemOrder()
{
    ItemOrder.Empty();
    INT Count = ItemWnd[0]->Items.Num();
    for (INT i = 0; i < Count; i++)
        ItemOrder.AddItem(AsC4(ItemWnd[0]->Items(i))->ID);
    if (::nw::VirtualCall<UBOOL>(GFileManager, 10, TEXT("..\\iwo"), 0))   // FFileManager::MakeDirectory
    {
        FArchive* Ar = GFileManager->CreateFileWriter(ItemOrderFile, FILEWRITE_EvenIfReadOnly, GNull);
        if (Ar)
        {
            for (INT i = 0; i < Count; i++)
                Ar->ByteOrderSerialize(&ItemOrder(i), 4);
            Ar->Close();
            delete Ar;
        }
    }
}
NW_HOOK_METHOD(0x100982c0, NCInventoryWnd, SaveItemOrder)

// 0x10098390 (name ours) - the file of the character read into the order list: the whole file in a buffer, then its
// integers. C4: the file is named after the character's name made unreadable: each byte of its ANSI form xor 0xF5 / 0xA1
// in turn, written in decimal (at most three digits) and joined
void NCInventoryWnd::ReadItemOrder()
{
    User* Me = GConsole->GetMyUser();
    if (Me)
    {
        ItemOrder.Empty();
        ANSICHAR Name[0x200];
        WideCharToMultiByte(CP_ACP, 0, Me->GetName(), -1, Name, 0x200, NULL, NULL);
        static const BYTE Key[2] = { 0xF5, 0xA1 };
        TCHAR Coded[0x200 * 3 + 1];
        INT CodedLen = 0;
        for (UINT i = 0; i < strlen(Name); i++)
        {
            Name[i] = (ANSICHAR)(Name[i] ^ Key[i & 1]);
            ANSICHAR Digits[4];
            _snprintf(Digits, 3, "%d", (INT)(BYTE)Name[i]);
            Digits[3] = 0;
            for (INT k = 0; Digits[k]; k++)
                Coded[CodedLen++] = (TCHAR)(BYTE)Digits[k];
        }
        Coded[CodedLen] = 0;
        appSprintf(ItemOrderFile, TEXT("..\\iwo\\%s_%d.dat"), Coded, GConsole->Unknown39D0);
        FArchive* File = GFileManager->CreateFileReader(ItemOrderFile, 0, GNull);
        if (File)
        {
            INT Size = File->TotalSize();
            BYTE* Buffer = (BYTE*)appMalloc(Size + 2, TEXT("new"));
            memset(Buffer, 0, Size + 2);
            File->Serialize(Buffer, Size);
            FArchive* Mem = GFileManager->CreateMemReader(Buffer, Size);
            if (Mem)
            {
                for (INT N = Size >> 2; N > 0; N--)
                {
                    INT Value = 0;
                    Mem->ByteOrderSerialize(&Value, 4);
                    ItemOrder.AddItem(Value);
                }
                Mem->Close();
            }
            File->Close();
            appFree(Buffer);
            if (Mem)
                delete Mem;
        }
    }
}
NW_HOOK_METHOD(0x10098390, NCInventoryWnd, ReadItemOrder)

// 0x10098610 (name ours) - the page's last item moves to the front (a new item shows first)
void NCInventoryWnd::MoveLastToFront()
{
    NCInvenItemWnd* Page = ItemWnd[0];
    INT Count = Page->Items.Num();
    if (Count > -1)
    {
        NItemEntry* Last = Page->Items(Count - 1);
        Page->Items.Remove(Count - 1, 1);
        Page->Items.Insert(0, 1);
        Page->Items(0) = Last;
    }
}
NW_HOOK_METHOD(0x10098610, NCInventoryWnd, MoveLastToFront)

// 0x10098670 (vslot 49) - the page's order is remembered when it has items
void NCInventoryWnd::HideWindow()
{
    NCWnd::HideWindow();
    if (ItemWnd[0]->Items.Num() > 0)
        SaveItemOrder();
}
NW_HOOK_METHOD(0x10098670, NCInventoryWnd, HideWindow)

// 0x10099010 (name ours) - the "Item" page back in the saved order: the items the file does not know (new ones) stay
// first, then the saved ids one by one: each is looked for from its own place on and swapped (by content) to the next free
// place. The scratch entry is built and destroyed by the stock constructor and destructor.
void NCInventoryWnd::RestoreItemOrder()
{
    NCInvenItemWnd* Page = ItemWnd[0];
    INT Count = Page->Items.Num();
    INT First = 0;
    for (INT i = 0; i < Count; i++)
    {
        INT Known = 0;
        for (INT j = 0; j < ItemOrder.Num(); j++)
        {
            if (AsC4(Page->Items(i))->ID == ItemOrder(j))
            {
                Known = 1;
                break;
            }
        }
        if (Known)
            break;
        First++;
    }
    for (INT k = 0; k < ItemOrder.Num(); k++)
    {
        if (k >= Page->Items.Num())
            continue;
        INT Found = k;
        INT bFound = 0;
        for (; Found < Page->Items.Num(); Found++)
        {
            if (ItemOrder(k) == AsC4(Page->Items(Found))->ID && First < Page->Items.Num())
            {
                bFound = 1;
                break;
            }
        }
        if (bFound)
        {
            BYTE TempBytes[sizeof(NItemEntry)];
            NItemEntry* Temp = (NItemEntry*)TempBytes;
            new ((void*)Temp) NItemEntry();   // NItemEntry::NItemEntry(), 0x1001F7F0
            Temp->Copy(Page->Items(First));
            Page->Items(First)->Copy(Page->Items(Found));
            Page->Items(Found)->Copy(Temp);
            Temp->~NItemEntry();   // 0x10050030
            First++;
        }
    }
}
NW_HOOK_METHOD(0x10099010, NCInventoryWnd, RestoreItemOrder)

// 0x100991f0 (name ours) - the console calls it: only when the page has items
void NCInventoryWnd::ApplyItemOrder()
{
    if (ItemWnd[0]->Items.Num() > 0)
        RestoreItemOrder();
}
NW_HOOK_METHOD(0x100991f0, NCInventoryWnd, ApplyItemOrder)

// 0x10099000 (name ours) - the paperdoll's AddHenna (a tail jump in the stock code; the console's slot 137 jumps here)
void NCInventoryWnd::AddHenna(INT HennaID, INT bUsable)
{
    EquipWnd->AddHenna(HennaID, bUsable);
}
NW_HOOK_METHOD(0x10099000, NCInventoryWnd, AddHenna)

// 0x1009a5f0 (name ours) - the paperdoll's ClearHennas (a tail jump; the console's slot 136 jumps here)
void NCInventoryWnd::ClearHennas()
{
    EquipWnd->ClearHennas();
}
NW_HOOK_METHOD(0x1009a5f0, NCInventoryWnd, ClearHennas)

// 0x10095870 (vslot 62) - the back, the adena (the sum of the "Item" page's adena items), the adena and weight icons, and
// the slots used against the most the character may carry; the weight bar is the child window WeightWnd's in C4
INT NCInventoryWnd::OnPaint(NCGDevice* Device)
{
    guard(NCInventoryWnd::OnPaint);
    TCHAR Text[128];
    TCHAR Formatted[128];
    TCHAR CountText[68];
    SIZE Size;
    CountText[0] = 0;   // (the stock leaves it as it was when there is no player)
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
#if NW_CUSTOM
    NCustomInventory::PaintBack(this, Device);   // custom: the wide window's back; the adena box is under the grid
    const INT DX = NCustomInventory::LowerShiftX, DY = NCustomInventory::LowerShiftY;
#else
    if (BackTex)
        Device->DrawTexture(0, 0x14, 0x100, 0x17D, 0, 0, 0x100, 0x17D, BackTex, 0xFF, 1);
    const INT DX = 0, DY = 0;
#endif

    appSprintf(Text, TEXT("%d"), ItemWnd[0]->GetAdena());
    FormatNumber(Formatted, Text);
    appSprintf(Text, TEXT("%s"), Formatted);
    NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0, L2FONT_Normal);
    Device->DrawText(0xC7 + DX - Size.cx, 0x164 + DY, 0xFFDCDCDC, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    if (AdenaTex)
        Device->DrawTexture(0x62 + DX, 0x163 + DY, 0x10, 0xC, 0, 0, 0x10, 0xC, AdenaTex, 0xFF, 1);
    if (WeightIconTex)
        Device->DrawTexture(0x62 + DX, 0x174 + DY, 0x10, 0xC, 0, 0, 0x10, 0xC, WeightIconTex, 0xFF, 1);

    User* Me = GConsole->GetMyUser();
    if (Me)
    {
        if (CanCrystallize(Me))
        {
            if (!TrashButton->IsShowWindow())
                TrashButton->ShowWindow();
        }
        appSprintf(CountText, TEXT("(%d/%d)"), EquipWnd->Items.Num() + ItemWnd[1]->Items.Num() + ItemWnd[0]->Items.Num(),
                   (INT)::nw::Field<SWORD>(Me, 0x230));
    }
#if NW_CUSTOM
    NCustomInventory::PaintCount(this, Device, CountText);   // custom: in the adena / weight box
#else
    NCGDevice::GetTextExtent(&Size, CountText, 0, 0, 0, 0, 0, L2FONT_Normal);
    Device->DrawText((INT)((DOUBLE)Width - (DOUBLE)Size.cx - 5.0), 0xA2, 0xFFB09B79, CountText, 0, 0, 0, L2FONT_Normal, 0, 0,
                     0, 0, 0, 0, 0);
#endif
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10095870, NCInventoryWnd, OnPaint)

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

// 0x10098be0 (vslot 38) - the page order is saved, then the two pages, the paperdoll, the tabs, the weight bar and the two
// buttons go
INT NCInventoryWnd::OnDestroy()
{
    guard(NCInventoryWnd::OnDestroy);
    SaveItemOrder();
#if NW_CUSTOM
    NCustomInventory::DestroyControls(this);
#endif
    DestroyChild(this, ItemWnd[0]);
    DestroyChild(this, ItemWnd[1]);
    DestroyChild(this, EquipWnd);
    DestroyChild(this, TabCtrl);
    DestroyChild(this, WeightWnd);
    DestroyChild(this, TrashButton);
    DestroyChild(this, RecipeButton);
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10098be0, NCInventoryWnd, OnDestroy)

// 0x10098dc0 (vslot 48) - the current tab's page shows; the minimized icon (if any) goes back under the window and hides;
// the recipe icon shows with the crystallize flag; the saved order is read
void NCInventoryWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    TabCtrl->ShowCurrentPage();
    if (Icon)
    {
        INT X = Icon->ScreenX;
        INT Y = Icon->ScreenY;
        SetPosition(X, Y, (INT)Width, (INT)Height);
        Icon->SetPosition(0, 0, 0, 0);
        Icon->HideWindow();
    }
    if (GConsole->GetMyUser() && CanCrystallize(GConsole->GetMyUser()))
        RecipeButton->ShowWindow();
    else
        RecipeButton->HideWindow();
    ReadItemOrder();
#if NW_CUSTOM
    NCustomInventory::KeepOnScreen(this);
#endif
}
NW_HOOK_METHOD(0x10098dc0, NCInventoryWnd, ShowWindow)

// 0x10095b40 - names and descriptions again from the item tables (the language changed), then the tool tips
void NCInventoryWnd::ReloadTexts()
{
    for (INT i = 0; i < EquipWnd->Items.Num(); i++)
    {
        NItemEntryC4* Item = AsC4(EquipWnd->Items(i));
        if (!Item)
            continue;
        appStrcpy(Item->Name, GL2GameData.GetItemName(Item->ID));
        appStrcpy(Item->Description, GL2GameData.GetItemDescription(Item->ID));
        Item->ToolTip.Fill((NItemEntry*)Item, 2, 0);
    }
    for (INT i = 0; i < ItemWnd[0]->Items.Num(); i++)
    {
        NItemEntryC4* Item = AsC4(ItemWnd[0]->Items(i));
        if (!Item)
            continue;
        appStrcpy(Item->Name, GL2GameData.GetItemName(Item->ID));
        appStrcpy(Item->Description, GL2GameData.GetItemDescription(Item->ID));
        Item->ToolTip.Fill((NItemEntry*)Item, 2, 0);
    }
    for (INT i = 0; i < ItemWnd[1]->Items.Num(); i++)
    {
        NItemEntryC4* Item = AsC4(ItemWnd[1]->Items(i));
        if (!Item)
            continue;
        appStrcpy(Item->Name, GL2GameData.GetItemName(Item->ID));
        appStrcpy(Item->Description, GL2GameData.GetItemDescription(Item->ID));
        Item->ToolTip.Fill((NItemEntry*)Item, 2, 0);
    }
}
NW_HOOK_METHOD(0x10095b40, NCInventoryWnd, ReloadTexts)

// 0x10095cd0 - the quest page is not searched
NItemEntry* NCInventoryWnd::FindItem(INT ObjectID)
{
    INT Index;
    if (EquipWnd->FindItem(ObjectID, &Index))
        return EquipWnd->Items(Index);
    if (ItemWnd[0]->FindItem(ObjectID, &Index))
        return ItemWnd[0]->Items(Index);
    return NULL;
}
NW_HOOK_METHOD(0x10095cd0, NCInventoryWnd, FindItem)

// 0x10098d90
void NCInventoryWnd::ClearItems()
{
    EquipWnd->Clear();
    ItemWnd[0]->Clear();
    ItemWnd[1]->Clear();
}
NW_HOOK_METHOD(0x10098d90, NCInventoryWnd, ClearItems)

// 0x10098270 - equipped items (NItemInfo::UnknownC8C) to the paperdoll, quest items (type 3) to the "Quest" page, the rest to
// the "Item" page
UBOOL NCInventoryWnd::AddItem(NItemInfo* Info)
{
    NItemInfoC4* New = AsC4(Info);
    INT Page = New->Param == 3;
    if (New->UnknownC8C)
    {
        EquipWnd->AddItem(Info);
        return 1;
    }
    if (ItemWnd[Page])
        ItemWnd[Page]->AddItem(Info);
    return 1;
}
NW_HOOK_METHOD(0x10098270, NCInventoryWnd, AddItem)

// 0x10098e90 - an item added (1), changed (2) or removed (3): an equipped item (NItemInfo::UnknownC8C) goes to the paperdoll,
// a quest item (type 3) to the "Quest" page, the rest to the "Item" page, where a new item shows first (MoveLastToFront); a
// changed item moves between the paperdoll and its page when it was put on or taken off.
UBOOL NCInventoryWnd::UpdateItem(NItemInfo* Info, INT Change)
{
    NItemInfoC4* New = AsC4(Info);
    INT Page = New->Param == 3;
    if (Change == 1)
    {
        if (New->UnknownC8C)
        {
            EquipWnd->AddItem(Info);
            return 1;
        }
    }
    else if (Change == 2)
    {
        if (New->UnknownC8C)
        {
            INT ObjectID = New->ObjectID;
            if (EquipWnd->HasItem(ObjectID))
            {
                EquipWnd->UpdateItem(Info);
                return 1;
            }
            ItemWnd[0]->RemoveItem(ObjectID);
            EquipWnd->AddItem(Info);
            return 1;
        }
        INT ObjectID = New->ObjectID;
        NCInvenItemWnd* Wnd = ItemWnd[Page];
        if (Wnd->HasItem(ObjectID))
        {
            Wnd->UpdateItem(Info);
            return 1;
        }
        EquipWnd->RemoveItem(ObjectID);
    }
    else
    {
        if (Change == 3)
        {
            if (New->UnknownC8C)
            {
                if (EquipWnd)
                    EquipWnd->RemoveItem(New->ObjectID);
            }
            else
                ItemWnd[Page]->RemoveItem(New->ObjectID);
        }
        return 1;
    }
    ItemWnd[Page]->AddItem(Info);
    if (Page == 0)
        MoveLastToFront();
    return 1;
}
NW_HOOK_METHOD(0x10098e90, NCInventoryWnd, UpdateItem)

// 0x10099a60 (vslot 37) - the textures, the small title bar (id 0xF2), the paperdoll (8, 31; 240 x 122), the two item pages
// (9, 189; 236 x 139: 6 x 4 icons of 32 pixels, 250 at most), the two tabs, the weight bar (C4: a window of its own, at
// 117, 372; 85 x 12) and the recipe and trash buttons. (C3's manufacture button is gone.)
INT NCInventoryWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI_CH3.InventoryWnd.Inventory_Back"));
    AdenaTex = FindTexture(TEXT("L2UI.WindowIcon.WindowIconAdena"));
    WeightIconTex = FindTexture(TEXT("L2UI.WindowIcon.Weight"));
    OutlineTex = FindTexture(TEXT("L2UI_CH3.Etc.menu_outline"));
    OutlineDownTex = FindTexture(TEXT("L2UI_CH3.Etc.menu_outline_down"));

    FrameCtrl = new NCFrameCtrl(1, 0, 0);
    FrameCtrl->ID = NID_INVENTORY_FRAME;
#if NW_CUSTOM
    // custom: the wide window (NCustomInventory.h): the paperdoll on the left, the pages in a grid on the right, the adena box
    // under the grid; the stock tabs are kept (they switch the pages) but out of sight: NCustomInventory draws its own
    using namespace NCustomInventory;
    const INT FrameW = NCustomInventory::Width, PageX = GridX, PageY = GridY;
    const INT PageW = GridColumns * 37 + 14, PageH = GridRows * 35 - 1, Columns = GridColumns, Rows = GridRows;
    const INT TabsX = 0, TabsY = -4000, DX = LowerShiftX, DY = LowerShiftY;
    const INT EquipX = DollX, EquipY = DollY, EquipW = DollW, EquipH = DollH;
#else
    const INT FrameW = 0x100, PageX = 9, PageY = 0xBD, PageW = 0xEC, PageH = 0x8B, Columns = 6, Rows = 4;
    const INT TabsX = 0xC, TabsY = 0x9F, DX = 0, DY = 0;
    const INT EquipX = 8, EquipY = 0x1F, EquipW = 0xF0, EquipH = 0x7A;
#endif
    FrameCtrl->SetPosition(0, 0, FrameW, 0x14);
    CreateChild(FrameCtrl, NWS_VISIBLE);

    EquipWnd = new NCEquipItemWnd(0x20);
    EquipWnd->SetPosition(EquipX, EquipY, EquipW, EquipH);
    CreateChild(EquipWnd, NWS_VISIBLE | 0x20);

    ItemWnd[0] = new NCInvenItemWnd(Columns, Rows, 0xFA, 0x20, 5, 3);
    ItemWnd[0]->SetPosition(PageX, PageY, PageW, PageH);
    CreateChild(ItemWnd[0], NWS_VISIBLE | 0x20);

    ItemWnd[1] = new NCInvenItemWnd(Columns, Rows, 0xFA, 0x20, 5, 3);
    ItemWnd[1]->SetPosition(PageX, PageY, PageW, PageH);
    CreateChild(ItemWnd[1], NWS_VISIBLE | 0x20);

    TabCtrl = new NCTabControl();
    TabCtrl->ID = NID_INVENTORY_TAB;
    TabCtrl->SetPosition(TabsX, TabsY, 0xBD, 0x17);
    CreateChild(TabCtrl, NWS_VISIBLE);

    WeightWnd = NewInvenWeightWnd();
    WeightWnd->SetPosition(0x75 + DX, 0x174 + DY, 0x55, 0xC);
    CreateChild(WeightWnd, NWS_VISIBLE | 0x20);

    AddInvenTab(TabCtrl, STR_Item, ItemWnd[0]);
    AddInvenTab(TabCtrl, STR_Quest, ItemWnd[1]);

    RecipeTex = FindTexture(TEXT("L2UI_CH3.InventoryWnd.Inventory_recipe"));
    RecipeDragTex = FindTexture(TEXT("L2UI_CH3.InventoryWnd.Inventory_recipe_drag"));
    TrashTex = FindTexture(TEXT("L2UI_CH3.InventoryWnd.Inventory_trash"));
    TrashDragTex = FindTexture(TEXT("L2UI_CH3.InventoryWnd.Inventory_trash_drag"));

    RecipeButton = new NCButton(TEXT("L2UI_CH3.InventoryWnd.Inventory_recipe"),
                                TEXT("L2UI_CH3.InventoryWnd.Inventory_recipe"), 0);
    RecipeButton->SetPosition(RecipeX, RecipeY, (DWORD)(INT)RecipeW, (DWORD)(INT)RecipeH);
    CreateChild(RecipeButton, NWS_VISIBLE | 0x20);
    RecipeButton->SetToolTip(0x37B, 1);

    TrashButton = new NCButton(TEXT("L2UI_CH3.InventoryWnd.Inventory_trash"),
                               TEXT("L2UI_CH3.InventoryWnd.Inventory_trash"), 0);
    TrashButton->SetPosition(TrashX, TrashY, (DWORD)(INT)TrashW, (DWORD)(INT)TrashH);
    CreateChild(TrashButton, NWS_VISIBLE | 0x20);
    TrashButton->HideWindow();
    TrashButton->SetToolTip(0x37A, 1);
#if NW_CUSTOM
    NCustomInventory::CreateControls(this);
#endif
    return 1;
}
NW_HOOK_METHOD(0x10099a60, NCInventoryWnd, OnCreate)

NW_CHECK_VTABLE(NCInventoryWnd, 0x101b6be8, 74)
