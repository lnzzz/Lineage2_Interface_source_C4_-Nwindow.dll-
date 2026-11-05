/*=============================================================================
    NCEquipItemWnd.cpp - the inventory's paperdoll. Stock: 0x10060730 .. 0x10062357 (up to
    the tool tip's jump table), 0x100642d0 .. 0x10065056, 0x100661a0 .. 0x10067684 (the item
    pages' and the inventory's functions sit in between), plus the constructor (0x10068380)
    and the destructor (0x1005fd70).
=============================================================================*/
#include "nwindow/NCEquipItemWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCInvenItemWnd.h"
#include "nwindow/NCPetInventoryWnd.h"
#include "nwindow/NCInventoryWnd.h"
#include "nwindow/NCShortCutWnd.h"
#include "nwindow/NMacroEntry.h"
#include "nwindow/NClasses.gen.h"
#include "nwindow/NDialogInfo.h"
#include "nwindow/NDropDialogInfo.h"
#include "nwindow/NHennaData.h"
#include "nwindow/NItemC4.h"
#if NW_CUSTOM
#include "nwindow/NCustomInventory.h"
#endif
#if NW_FEATURES
// phase 6: ItemHoverTip (not in NWindow_C1: the hover tip of the item grids in the recent style)
#include "features/ItemHoverTip.h"
#endif

enum { ITEM_ADENA = 57 };
enum { SOUND_PickUp = 3 };

// NDialogInfo::ID of the questions (names ours)
enum
{
    DIALOG_DROP_ITEM = 4,
    DIALOG_DESTROY_ITEM = 5,
    DIALOG_CRYSTALLIZE_ITEM = 6,
    DIALOG_GIVE_TO_PET = 54,    // 0x36 (C1: 50)
};

// systemmsg
enum
{
    MSG_DropHowMany = 71,       // "How many $s1(s) do you want to discard?"
    MSG_DestroyHowMany = 73,    // "How many $s1(s) do you want to destroy?"
    MSG_Destroy = 74,           // "Destroy $s1. Do you want to continue?"
    MSG_CannotDropHere = 179,   // "You cannot discard items here."
    MSG_Crystallize = 336,      // "$s1 is crystallized. Do you want to continue?"
    MSG_Drop = 400,             // "Discard $s1. Do you wish to continue?"
    MSG_PetAdenaHowMuch = 575,  // "How much adena do you wish to transfer to your pet?"
    MSG_PetHowMuch = 576,       // "How much do you wish to transfer?"
};

// the console's test (0x1006b6c0), the shortcut selections the right button forgets (below) and the parent's drop icons
static FORCEINLINE NCDialogBox* MainDialogBox()
{
    return GDesktop->MainDialogBox;
}

// The shortcut bars forget a pending selection (C2: the bars' list sits behind the desktop, +0xF8; 0x10056a20 counts
// them, 0x10060ce0 gets one, 0x1008b4b0 clears its slots and the flag at +0x9D4).
static FORCEINLINE void ClearShortCutSelections()
{
    if (GConsole)
    {
        for (INT i = 0; i < GConsole->GetMacroCount(); i++)
        {
            if (GConsole->GetMacro(i) && GConsole->GetMacro(i)->bRunning)
                GConsole->GetMacro(i)->Reset();
        }
    }
}

// takes Item off when it is the one of Slot (OnRButtonDown, OnLButtonDblClick): by its body part bits, but an earring
// or a ring by the bit of the ear or the finger that wears it; the right button then clears the shortcut bars'
// selections (C4: the slots 15, 16 and 17 are the items drawn over several places: 16 sits at 5, 15 and 17 at 6)
static FORCEINLINE UBOOL UnEquipIfInSlot(NCEquipItemWnd* Wnd, NItemEntry* Item, INT Slot, UBOOL bClearShortCuts)
{
    NItemEntryC4* Entry = AsC4(Item);
    INT ItemSlot = Wnd->GetEquipSlot(Entry->BodyPart, Entry->ObjectID);
    if (ItemSlot == 16 || ItemSlot == 15 || ItemSlot == 17)
    {
        if (Slot != (ItemSlot == 16 ? 5 : 6))
            return 0;
        L2ParamStack Params(10);
        Params.PushBack((void*)(INT_PTR)Entry->BodyPart);
        GConsole->RequestUnEquipItem(Params);
        if (bClearShortCuts)
            ClearShortCutSelections();
        return 1;
    }
    if (Slot != ItemSlot)
        return 0;
    L2ParamStack Params(10);
    INT BodyPart = Entry->BodyPart;
    if (BodyPart & 6)
    {
        User* Me = GConsole->GetMyUser();
        if (Me->Equipped[1] == Entry->ObjectID)
            Params.PushBack((void*)2);
        if (Me->Equipped[2] == Entry->ObjectID)
            Params.PushBack((void*)4);
    }
    else if (Entry->BodyPart & 0x30)
    {
        User* Me = GConsole->GetMyUser();
        if (Me->Equipped[4] == Entry->ObjectID)
            Params.PushBack((void*)0x10);
        if (Me->Equipped[5] == Entry->ObjectID)
            Params.PushBack((void*)0x20);
    }
    else
        Params.PushBack((void*)(INT_PTR)BodyPart);
    GConsole->RequestUnEquipItem(Params);
    if (bClearShortCuts)
        ClearShortCutSelections();
    return 1;
}


// The icons of the inventory window's trash and recipe boxes show their dragged picture while the carried item is over
// them (repeated inline in the stock OnMouseMove of the paperdoll and of the item pages): the boxes are the parent's
// (relative to its corner), the flags and the buttons too
static FORCEINLINE void UpdateDropIcons(NCWnd* Wnd, NCInventoryWnd* Inventory, INT X, INT Y)
{
    NRectF Box;
    Box.X = Inventory->TrashX + Inventory->ScreenX;
    Box.Y = Inventory->TrashY + Inventory->ScreenY;
    Box.W = Inventory->TrashW;
    Box.H = Inventory->TrashH;
    if (Wnd->PtInRect(X, Y, Box))
    {
        Inventory->bOverTrash = 1;
        Inventory->TrashButton->SetNormalTex(TEXT("L2UI_CH3.InventoryWnd.Inventory_trash_drag"));
        Inventory->TrashButton->SetPushedTex(TEXT("L2UI_CH3.InventoryWnd.Inventory_trash_drag"));
    }
    else
    {
        Inventory->bOverTrash = 0;
        Inventory->TrashButton->SetNormalTex(TEXT("L2UI_CH3.InventoryWnd.Inventory_trash"));
        Inventory->TrashButton->SetPushedTex(TEXT("L2UI_CH3.InventoryWnd.Inventory_trash"));
    }
    Box.X = Inventory->RecipeX + Inventory->ScreenX;
    Box.Y = Inventory->RecipeY + Inventory->ScreenY;
    Box.W = Inventory->RecipeW;
    Box.H = Inventory->RecipeH;
    if (Wnd->PtInRect(X, Y, Box))
    {
        Inventory->bOverRecipe = 1;
        Inventory->RecipeButton->SetNormalTex(TEXT("L2UI_CH3.InventoryWnd.Inventory_recipe_drag"));
        Inventory->RecipeButton->SetPushedTex(TEXT("L2UI_CH3.InventoryWnd.Inventory_recipe_drag"));
    }
    else
    {
        Inventory->bOverRecipe = 0;
        Inventory->RecipeButton->SetNormalTex(TEXT("L2UI_CH3.InventoryWnd.Inventory_recipe"));
        Inventory->RecipeButton->SetPushedTex(TEXT("L2UI_CH3.InventoryWnd.Inventory_recipe"));
    }
}


// the player's crystallize flag: 4 bytes after where UE2Engine.h puts User::bCanCrystallize (+0x1F8 in the stock code)
static FORCEINLINE INT CanCrystallizeFlag(User* Me)
{
    return ::nw::Field<INT>(Me, 0x1F8);
}

// delete of an item record: ~NItemEntry (0x10050030) and the memory back to GMalloc
static FORCEINLINE void DeleteEntry(NItemEntry* Item)
{
    Item->~NItemEntry();
    appFree(Item);
}

// 0x10099470 - 15 slots in a 5 x 3 grid; the two arrays are empty, nothing is selected (C4 leaves bEnchantMode alone)
NCEquipItemWnd::NCEquipItemWnd(INT InCellSize)
{
    CellSize = InCellSize;
    SelectTex = NULL;
    ToolTipTex = NULL;
    bDragging = 0;
    bNoDrag = 0;
    bHasUnknown1C98 = 0;
    Unknown104 = 5;
    Unknown108 = 3;
    NumSlots = 15;
    Selected = -1;
    HoverSlot = -1;
    HoverHenna = -1;
    SelectedIndex = -1;
}
NW_HOOK_CTOR_ARGS(0x10099470, NCEquipItemWnd, (INT A), (A))

// 0x1008f2e0 - the tattoos' and the items' arrays, then the window's
NW_HOOK_DTOR(0x1008f2e0, NCEquipItemWnd)


// 0x10093be0 (vslot 25)
INT NCEquipItemWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == WM_LBUTTONDBLCLK)
        return OnLButtonDblClick(wParam, lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x10093be0, NCEquipItemWnd, WindowProc)

// C2 CANDIDATE: the C2 function is not the same code as the C1 one; kept only while it matches the stock one (verify_data)
// 0x10093c40 (vslot 37) - the textures: the tool tip's, the selection's and the two frames, the two-slot item's cap and
// the tattoo's back
INT NCEquipItemWnd::OnCreate()
{
    NCWnd::OnCreate();
    ToolTipTex = FindTexture(TEXT("L2UI.tooltip.mini-tool01"));
    SelectTex = FindTexture(TEXT("l2ui.NWindow.item_click"));
    OutlineDownTex = FindTexture(TEXT("l2ui_ch3.InventoryWnd.Inventory_OutLine_down"));
    OutlineTex = FindTexture(TEXT("l2ui_ch3.InventoryWnd.Inventory_OutLine"));
    DualCapTex = FindTexture(TEXT("L2UI.InventoryWnd.Icon_dualcap"));
    HennaBackTex = FindTexture(TEXT("L2UI_CH3.InventoryWnd.Inventory_Mark"));
    return 1;
}
NW_HOOK_METHOD(0x10093c40, NCEquipItemWnd, OnCreate)

// 0x10093cd0 (vslot 74) - the console's answer (the slot an item with this body part sits in)
INT NCEquipItemWnd::GetEquipSlot(INT BodyPart, INT ObjectID)
{
    return GConsole->GetEquipSlot(BodyPart, ObjectID);
}
NW_HOOK_METHOD(0x10093cd0, NCEquipItemWnd, GetEquipSlot)


// the place of a slot's icon (0 .. 14): the stock 5 x 3 grid, or (custom) the wide inventory's two columns
static const INT GStockSlotX[5] = { 0x07, 0x2E, 0x55, 0x82, 0xA9 };
static const INT GStockSlotY[3] = { 0x07, 0x2D, 0x53 };
static FORCEINLINE void SlotXY(INT Slot, INT& X, INT& Y)
{
#if NW_CUSTOM
    NCustomInventory::SlotPos(Slot, X, Y);
#else
    X = GStockSlotX[Slot % 5];
    Y = GStockSlotY[Slot / 5];
#endif
}

// the place of a tattoo's 26 x 26 box: the stock column (x 215, 27 apart from y 8), or (custom) a row under the model
static FORCEINLINE void HennaXY(INT Index, INT& X, INT& Y)
{
#if NW_CUSTOM
    NCustomInventory::HennaPos(Index, X, Y);
#else
    X = 0xD7;
    Y = Index * 0x1B + 8;
#endif
}

// the frame of a selected item, or the two of the others, at (X, Y) of 34 / 35 pixels (the paperdoll's frames)
static FORCEINLINE void DrawSlotFrame(NCGDevice* Device, NCEquipItemWnd* Wnd, NItemEntry* Item, INT X, INT Y)
{
    if (Item->IsSelected())
        Device->DrawTexture(X, Y, 0x22, 0x22, 0, 0, 0x22, 0x22, Wnd->SelectTex, 0xFF, 1);
    else
    {
        Device->DrawTexture(X, Y, 0x22, 0x22, 0, 0, 0x22, 0x22, Wnd->OutlineDownTex, 0xFF, 1);
        Device->DrawTexture(X, Y, 0x23, 0x23, 0, 0, 0x23, 0x23, Wnd->OutlineTex, 0xFF, 1);
    }
}

// 0x10094080 (vslot 62) - each item at the place of its slot (the console says which: 0 .. 14 a 5 x 3 grid 39 / 38 pixels
// apart from (7, 7); 15 the chest over 6 and 11, 16 the two-handed weapon over 5 and 7, C4: 17 a four-piece set over 6, 11,
// 10 and 12: one icon per piece with the cap over all but the first); a bow (weapon type 6) or a dual weapon (10) in
// slot 5 when the item with the flag is worn; the frame of a selected item, or the two of the others; then the tattoos'
// boxes (as many as the class may wear) with their icons, a dark veil over those not usable
INT NCEquipItemWnd::OnPaint(NCGDevice* Device)
{
    guard(NCEquipItemWnd::OnPaint);
    Device->SetClip(ScreenX - 1, ScreenY - 1, (INT)(Width + 2.0f), (INT)(Height + 2.0f));
    Device->BeginPaint(ScreenX, ScreenY);
#if NW_CUSTOM
    NCustomInventory::PaintEmptySlots(this, Device);   // custom: the stock back has them on the stock places
#endif
    INT X = 0;
    INT Y = 0;
    for (INT i = 0; i < Items.Num(); i++)
    {
        NItemEntry* Item = Items(i);
        if (!Item)
            continue;
        NItemEntryC4* E = AsC4(Item);
        INT Slot = GetEquipSlot(E->BodyPart, E->ObjectID);
        UBOOL bTwoSlot = 0;
        switch (Slot)
        {
        case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
        case 8: case 9: case 10: case 11: case 12: case 13: case 14:
            SlotXY(Slot, X, Y);
            break;
        case 15:
        case 16:
        case 17:
            bTwoSlot = 1;
            break;
        default:
            continue;
        }
        if ((E->UnknownC30 == 6 || E->UnknownC30 == 10) && bHasUnknown1C98)
        {
            SlotXY(5, X, Y);
            bTwoSlot = 0;
        }
        if (!bTwoSlot)
        {
            Device->DrawTexture(X, Y, CellSize, CellSize, 0, 0, CellSize, CellSize, Item->GetIcon(), 0xFF, 1);
            DrawSlotFrame(Device, this, Item, X - 1, Y - 1);
            continue;
        }
        // an item over several places: the icons of its second to fifth names, the missing ones taken from the one before
        UTexture* Icon1 = NULL;
        UTexture* Icon2 = NULL;
        UTexture* Icon3 = NULL;
        UTexture* Icon4 = NULL;
        if (E->IconName[1][0] != 0)
            Icon1 = E->Icon[1];
        if (E->IconName[2][0] != 0)
            Icon2 = E->Icon[2];
        if (E->IconName[3][0] != 0)
            Icon3 = E->Icon[3];
        if (E->IconName[4][0] != 0)
            Icon4 = E->Icon[4];
        if (!Icon1)
        {
            Icon1 = E->Icon[0];
            Icon4 = Icon3 = Icon2 = Icon1;
        }
        else if (!Icon2)
            Icon4 = Icon3 = Icon2 = Icon1;
        else if (!Icon3)
            Icon4 = Icon3 = Icon2;
        else if (!Icon4)
            Icon4 = Icon3;
        INT X5, Y5, X6, Y6, X7, Y7, X10, Y10, X11, Y11, X12, Y12;
        SlotXY(5, X5, Y5);
        SlotXY(6, X6, Y6);
        SlotXY(7, X7, Y7);
        SlotXY(10, X10, Y10);
        SlotXY(11, X11, Y11);
        SlotXY(12, X12, Y12);
        if (Slot == 16)
        {
            Device->DrawTexture(X5, Y5, CellSize, CellSize, 0, 0, CellSize, CellSize, Icon1, 0xFF, 1);
            Device->DrawTexture(X7, Y7, CellSize, CellSize, 0, 0, CellSize, CellSize, Icon2, 0xFF, 1);
            Device->DrawTexture(X7, Y7, CellSize, CellSize, 0, 0, CellSize, CellSize, DualCapTex, 0xFF, 1);
            DrawSlotFrame(Device, this, Item, X5 - 1, Y5 - 1);
        }
        else if (Slot == 15)
        {
            Device->DrawTexture(X6, Y6, CellSize, CellSize, 0, 0, CellSize, CellSize, Icon1, 0xFF, 1);
            Device->DrawTexture(X11, Y11, CellSize, CellSize, 0, 0, CellSize, CellSize, Icon2, 0xFF, 1);
            Device->DrawTexture(X11, Y11, CellSize, CellSize, 0, 0, CellSize, CellSize, DualCapTex, 0xFF, 1);
            DrawSlotFrame(Device, this, Item, X6 - 1, Y6 - 1);
        }
        else if (Slot == 17)
        {
            Device->DrawTexture(X6, Y6, CellSize, CellSize, 0, 0, CellSize, CellSize, Icon1, 0xFF, 1);
            Device->DrawTexture(X11, Y11, CellSize, CellSize, 0, 0, CellSize, CellSize, Icon2, 0xFF, 1);
            Device->DrawTexture(X11, Y11, CellSize, CellSize, 0, 0, CellSize, CellSize, DualCapTex, 0xFF, 1);
            Device->DrawTexture(X10, Y10, CellSize, CellSize, 0, 0, CellSize, CellSize, Icon3, 0xFF, 1);
            Device->DrawTexture(X10, Y10, CellSize, CellSize, 0, 0, CellSize, CellSize, DualCapTex, 0xFF, 1);
            Device->DrawTexture(X12, Y12, CellSize, CellSize, 0, 0, CellSize, CellSize, Icon4, 0xFF, 1);
            Device->DrawTexture(X12, Y12, CellSize, CellSize, 0, 0, CellSize, CellSize, DualCapTex, 0xFF, 1);
            DrawSlotFrame(Device, this, Item, X6 - 1, Y6 - 1);
        }
    }
    if (HennaBackTex)
    {
        INT Count = GConsole->GetHennaSlotCount();
        for (INT i = 0; i < Count; i++)
        {
            INT HX, HY;
            HennaXY(i, HX, HY);
            Device->DrawTexture(HX, HY, 0x1A, 0x1A, 0, 0, 0x1A, 0x1A, HennaBackTex, 0xFF, 1);
            if (i < Hennas.Num() && AsC4((NItemEntry*)Hennas(i))->Icon[0])
            {
                Device->DrawStretchedTex(HX + 1, HY + 1, 0x18, 0x18, AsC4((NItemEntry*)Hennas(i))->Icon[0], 1, 0xFF);
                if (!AsC4((NItemEntry*)Hennas(i))->UnknownC34)
                    Device->DrawTexture(HX + 2, HY + 1, 0x18, 0x18, 0, 0, 0x18, 0x18,
                                        FindTexture(TEXT("NWindow.ChatBack")), 0xFF, 1);
            }
        }
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10094080, NCEquipItemWnd, OnPaint)


// 0x10099510 (vslot 63) - the tool tip of the slot under the mouse (the item whose slot it is, a two-slot item by either
// of its slots): the title box of an item that is not selected, the full box of a selected one, placed beside the slot; then
// the tattoo under the mouse
INT NCEquipItemWnd::OnDrawToolTip(NCGDevice* Device)
{
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
#if NW_CUSTOM
    INT X = 0, Y = 0;
    if (HoverSlot >= 0 && HoverSlot < 15)
        SlotXY(HoverSlot, X, Y);
    Y += 4;
#else
    INT Row = HoverSlot / 5;
    INT Col = HoverSlot % 5;
    INT Y = (CellSize + 6) * Row + 0xB;
    INT X;
    if (Col == 3)
        X = CellSize * 3 + 0x22;
    else if (Col == 4)
        X = CellSize * 4 + 0x29;
    else
        X = (CellSize + 7) * Col + 7;
#endif
    for (INT i = 0; i < Items.Num(); i++)
    {
        INT Slot = GetEquipSlot(AsC4(Items(i))->BodyPart, AsC4(Items(i))->ObjectID);
        UBOOL bMatch;
        if (HoverSlot == Slot)
            bMatch = 1;
        else if (Slot == 16)
            bMatch = HoverSlot == 5;
        else if (Slot == 15 || Slot == 17)
            bMatch = HoverSlot == 6;
        else
            bMatch = 0;
        if (bMatch)
        {
            if (!Items(i)->IsSelected())
            {
                if (ToolTipSkin)
                {
                    NToolTipRect Wnd;
                    Wnd.X = ScreenX;
                    Wnd.Y = ScreenY;
                    Wnd.W = Width;
                    Wnd.H = Height;
                    ToolTipSkin->Draw(Device, AsC4(Items(i))->ToolTip, X, Y, Wnd);
                }
            }
            else if (ToolTipSkin)
            {
                NToolTipRect Wnd;
                Wnd.X = ScreenX;
                Wnd.Y = ScreenY;
                Wnd.W = Width;
                Wnd.H = Height;
                ToolTipSkin->DrawFull(Device, AsC4(Items(i))->ToolTip, X, Y, Wnd);
            }
            break;
        }
    }
    if (HoverHenna >= 0 && HoverHenna < Hennas.Num() && Hennas(HoverHenna) && ToolTipSkin)
    {
        NToolTipRect Wnd;
        Wnd.X = ScreenX;
        Wnd.Y = ScreenY;
        Wnd.W = Width;
        Wnd.H = Height;
#if NW_CUSTOM
        INT HX, HY;
        HennaXY(HoverHenna, HX, HY);
        ToolTipSkin->DrawFull(Device, AsC4((NItemEntry*)Hennas(HoverHenna))->ToolTip, HX + 1, HY, Wnd);
#else
        ToolTipSkin->DrawFull(Device, AsC4((NItemEntry*)Hennas(HoverHenna))->ToolTip, 0xD8, HoverHenna * 0x1A + 8, Wnd);
#endif
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
}
NW_HOOK_METHOD(0x10099510, NCEquipItemWnd, OnDrawToolTip)

// 0x10094860
void NCEquipItemWnd::UnselectAll()
{
    for (INT i = 0; i < Items.Num(); i++)
        Items(i)->Unselect();
}
NW_HOOK_METHOD(0x10094860, NCEquipItemWnd, UnselectAll)

// 0x10094890 (ret 8) - the slot whose 32 x 32 box (from (7, 7), the columns 0 / 39 / 78 / 123 / 162 apart and the rows
// 0 / 38 / 76) holds the point, -1 outside
INT NCEquipItemWnd::HitTest(INT X, INT Y)
{
    static const INT ColumnX[5] = { 0, 0x27, 0x4E, 0x7B, 0xA2 };
    static const INT RowY[3] = { 0, 0x26, 0x4C };
    NRectF Box;
    Box.W = 32.0f;
    Box.H = 32.0f;
    for (INT i = 0; i < 15; i++)
    {
#if NW_CUSTOM
        INT SX, SY;
        SlotXY(i, SX, SY);
        Box.X = ScreenX + SX;
        Box.Y = ScreenY + SY;
#else
        Box.X = ScreenX + 7 + ColumnX[i % 5];
        Box.Y = ScreenY + 7 + RowY[i / 5];
#endif
        if (PtInRect(X, Y, Box))
            return i;
    }
    return -1;
}
NW_HOOK_METHOD(0x10094890, NCEquipItemWnd, HitTest)


// 0x10094a40 - the item with Info's object id takes Info's fields and a new tool tip; the entry built for that is
// never freed, as in the stock code. C4: only the first of the five icons is looked up again.
UBOOL NCEquipItemWnd::UpdateItem(NItemInfo* Info)
{
    NItemInfoC4* New = AsC4(Info);
    for (INT i = 0; i < Items.Num(); i++)
    {
        NItemEntry* Found = Items(i);
        if (New->ObjectID == AsC4(Found)->ObjectID)
        {
            New->Icon[0] = FindTexture(New->IconName[0]);
            if (!New->Icon[0])
                New->Icon[0] = FindTexture(TEXT("NWindow.BlackTexture"));
            NItemEntry* Updated = NewItemEntry(Info);
            AsC4(Updated)->ToolTip.Fill(Updated, 2, 0);
            Found->Copy(Updated);
            return 1;
        }
    }
    return 0;
}
NW_HOOK_METHOD(0x10094a40, NCEquipItemWnd, UpdateItem)

// 0x10094b40 (C4: the object id of an entry is at +0x1B1C)
UBOOL NCEquipItemWnd::HasItem(INT ObjectID)
{
    for (INT i = 0; i < Items.Num(); i++)
        if (ObjectID == AsC4(Items(i))->ObjectID)
            return 1;
    return 0;
}
NW_HOOK_METHOD(0x10094b40, NCEquipItemWnd, HasItem)

// 0x10094b90
UBOOL NCEquipItemWnd::FindItem(INT ObjectID, INT* Index)
{
    for (INT i = 0; i < Items.Num(); i++)
    {
        if (ObjectID == AsC4(Items(i))->ObjectID)
        {
            *Index = i;
            return 1;
        }
    }
    return 0;
}
NW_HOOK_METHOD(0x10094b90, NCEquipItemWnd, FindItem)

// 0x10095fc0 (vslot 68) - selects the item of the slot under the mouse and picks it up (unless bNoDrag, which lasts one
// click); an item drawn over several slots (15, 16, 17) unselects all on the way. C4: slot 17 behaves as 15.
INT NCEquipItemWnd::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    guard(NCEquipItemWnd::OnLButtonDown);
    INT Slot = HitTest(LOWORD(lParam), HIWORD(lParam));
    HoverSlot = Slot;
    Selected = Slot;
    UBOOL bFound = 0;
    for (INT i = 0; i < Items.Num(); i++)
    {
        INT ItemSlot = GetEquipSlot(AsC4(Items(i))->BodyPart, AsC4(Items(i))->ObjectID);
        if (ItemSlot == 16)
        {
            UnselectAll();
            if (Selected != 5)
                continue;
        }
        else if (ItemSlot == 15 || ItemSlot == 17)
        {
            UnselectAll();
            if (Selected != 6)
                continue;
        }
        else if (Selected == ItemSlot)
            UnselectAll();
        else
            continue;
        Items(i)->Select();
        SelectedIndex = i;
        bFound = 1;
        break;
    }
    if (!bFound)
        SelectedIndex = -1;
    if (!bNoDrag)
    {
        bDragging = 1;
        GConsole->PlayInterfaceSound(SOUND_PickUp);
    }
    else
        bNoDrag = 0;
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10095fc0, NCEquipItemWnd, OnLButtonDown)

// 0x100949b0 (name ours) - the tattoo under the mouse (its box, relative to the window), -1 outside
INT NCEquipItemWnd::HitTestHenna(INT X, INT Y)
{
    INT RX = X - ScreenX;
    INT RY = Y - ScreenY;
    for (INT i = 0; i < Hennas.Num(); i++)
    {
        if (PtInRect(RX, RY, Hennas(i)->Box))
            return i;
    }
    return -1;
}
NW_HOOK_METHOD(0x100949b0, NCEquipItemWnd, HitTestHenna)

// 0x10096150 (vslot 61) - the slot and the tattoo under the mouse; while an item is carried the cursor is its icon, and
// over the inventory's trash or recipe icon that icon shows its dragged picture
INT NCEquipItemWnd::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    INT X = LOWORD(lParam);
    INT Y = HIWORD(lParam);
    HoverSlot = HitTest(X, Y);
    HoverHenna = HitTestHenna(X, Y);
    if (IsFocused())
    {
        if (bDragging && Selected >= 0 && Selected < NumSlots && SelectedIndex < Items.Num() && SelectedIndex >= 0)
        {
            SetCursor(4, Items(SelectedIndex)->GetIcon());
            LockCursor();
        }
        NCInventoryWnd* Inventory = (NCInventoryWnd*)Entry->Parent->Wnd;
        if (!Inventory->IsA(NCInventoryWnd::StaticClass()))
            return 1;
        UpdateDropIcons(this, Inventory, X, Y);
    }
    return 1;
}
NW_HOOK_METHOD(0x10096150, NCEquipItemWnd, OnMouseMove)

// The shortcut selections of the macros are forgotten, as the stock paperdoll does after each request (repeated inline)

// The request that takes the item off when it is dropped on the inventory page: the body part, or for a ring / earring
// the part of the finger / ear that holds it (a worn pair is told apart by the object id; the else-if of the fingers is
// the stock's)
static FORCEINLINE void RequestTakeOff(NItemEntry* Item)
{
    NItemEntryC4* Entry = AsC4(Item);
    L2ParamStack Params(10);
    INT BodyPart = Entry->BodyPart;
    if (BodyPart & 6)
    {
        User* Me = GConsole->GetMyUser();
        if (Me->Equipped[1] == Entry->ObjectID)
            Params.PushBack((void*)2);
        if (Me->Equipped[2] == Entry->ObjectID)
            Params.PushBack((void*)4);
    }
    else if (BodyPart & 0x30)
    {
        User* Me = GConsole->GetMyUser();
        if (Me->Equipped[4] == Entry->ObjectID)
            Params.PushBack((void*)0x10);
        else if (Me->Equipped[5] == Entry->ObjectID)
            Params.PushBack((void*)0x20);
    }
    else
        Params.PushBack((void*)(INT_PTR)BodyPart);
    GConsole->RequestUnEquipItem(Params);
}

// the UClass pointers the stock code compares a window with in OnLButtonUp (NClasses.gen.h still has the C3 ones)
#if NW_HOOK_MODE
#define EQUIP_CLASS_InvenItemWnd    ((UClass*)::nw::Addr(0x102DB898))
#define EQUIP_CLASS_ShortCutWnd     ((UClass*)::nw::Addr(0x10307210))
#define EQUIP_CLASS_PetInvenItemWnd ((UClass*)::nw::Addr(0x102F2FC0))
#define EQUIP_CLASS_Button          ((UClass*)::nw::Addr(0x102B5DB8))
#define EQUIP_CLASS_Console         ((UClass*)::nw::Addr(0x102D21A0))
#else
#define EQUIP_CLASS_InvenItemWnd    (NCInvenItemWnd::StaticClass())
#define EQUIP_CLASS_ShortCutWnd     (NCShortCutWnd::StaticClass())
#define EQUIP_CLASS_PetInvenItemWnd (NCPetInvenItemWnd::StaticClass())
#define EQUIP_CLASS_Button          (NCButton::StaticClass())
#define EQUIP_CLASS_Console         (NCConsole::StaticClass())
#endif

// 0x10096370 (vslot 69; the stock calls it OnLButtonDown in its guard names) - the release of a carried item: dropped on
// the inventory page the item is taken off, on the shortcut bar it gets a shortcut, on the pet's items it is given to the
// pet (an amount asks first), on the trash icon it asks to be destroyed, on the recipe icon to be crystallized (when the
// player may), on the ground (no actor under the cursor) it asks to be discarded. C4: the enchant scroll part is gone.
INT NCEquipItemWnd::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    INT X = LOWORD(lParam);
    INT Y = HIWORD(lParam);
    if (Selected < 0 || Selected >= NumSlots)
    {
        bDragging = 0;
        return 1;
    }
    // the item of the selected slot
    INT Index = 0;
    if (Items.Num() > 0)
    {
        UBOOL bFound;
        do
        {
            INT Slot = GetEquipSlot(AsC4(Items(Index))->BodyPart, AsC4(Items(Index))->ObjectID);
            if (Slot == 16)
                bFound = Selected == 5;
            else if (Slot == 15 || Slot == 17)
                bFound = Selected == 6;
            else
                bFound = Selected == Slot;
        } while (!bFound && ++Index < Items.Num());
    }
    if (Index == Items.Num())
    {
        bDragging = 0;
        return 1;
    }
    NItemEntry* Item = Items(Index);
    if (bDragging && Item)
    {
        NItemEntryC4* E = AsC4(Item);
        NWndEntry* Found = FindWindowAt(X, Y);
        if (Found && Found->Wnd)
        {
            NCWnd* Target = Found->Wnd;
            if (Target->IsA(EQUIP_CLASS_InvenItemWnd))
            {
                // onto the inventory page: taken off (unless it is one of the items that stay on)
                if (E->Unknown1C98 == 0)
                {
                    RequestTakeOff(Items(Index));
                    UnselectAll();
                    Selected = -1;
                    bDragging = 0;
                    return 1;
                }
            }
            else if (Target->IsA(EQUIP_CLASS_ShortCutWnd))
            {
                INT ShortCut = ((NCShortCutWnd*)Target)->SlotAtPoint(X, Y);
                if (ShortCut >= 0)
                {
                    L2ParamStack Params(10);
                    Params.PushBack((void*)1);
                    Params.PushBack((void*)(INT_PTR)ShortCut);
                    Params.PushBack((void*)(INT_PTR)AsC4(Items(Index))->ObjectID);
                    Params.PushBack((void*)1);
                    GConsole->RequestShortCutReg(Params);
                }
                UnselectAll();
                Selected = -1;
                bDragging = 0;
                return 1;
            }
            else if (Target->IsA(EQUIP_CLASS_PetInvenItemWnd))
            {
                if (!Item->IsStackable() || E->Count < 2)
                {
                    L2ParamStack Params(10);
                    Params.PushBack((void*)(INT_PTR)AsC4(Items(Selected))->ObjectID);
                    Params.PushBack((void*)1);
                    GConsole->RequestGiveItemToPet(Params);
                    bDragging = 0;
                    return 1;
                }
                TCHAR Name[64];
                appSprintf(Name, TEXT("%s"), Item->GetName());
                NDialogInfo Info;
                Info.ID = 0x36;
                Info.Type = 3;
                Info.Owner = Entry;
                Info.bMainBox = 1;
                Info.bInput = 1;
                Info.SetSystemMessage(0x23F);
                Info.SetButtons(Name, NULL);
                Info.Flag18 = E->ID;
                Info.Unknown74 = E->Count;
                Info.SetParams(E->ObjectID, -1);
                GConsole->ShowDialog(&Info);
            }
            else if (Target->IsA(EQUIP_CLASS_Button))
            {
                NWndEntry* Parent = Target->Entry->Parent;
                if (Parent && Parent->Wnd && Parent->Wnd->IsA(NCInventoryWnd::StaticClass()))
                {
                    NCInventoryWnd* Inventory = (NCInventoryWnd*)Parent->Wnd;
                    Inventory->bOverTrash = 0;
                    Inventory->bOverRecipe = 0;
                    Inventory->TrashButton->SetNormalTex(TEXT("L2UI_CH3.InventoryWnd.Inventory_trash"));
                    Inventory->TrashButton->SetPushedTex(TEXT("L2UI_CH3.InventoryWnd.Inventory_trash"));
                    Inventory->RecipeButton->SetNormalTex(TEXT("L2UI_CH3.InventoryWnd.Inventory_recipe"));
                    Inventory->RecipeButton->SetPushedTex(TEXT("L2UI_CH3.InventoryWnd.Inventory_recipe"));
                    if (Inventory->IsInTrash(X, Y))
                    {
                        TCHAR Name[64];
                        if (Item->IsStackable() && E->Count > 1)
                        {
                            appSprintf(Name, TEXT("%s"), Item->MakeDisplayName());
                            NDialogInfo Info;
                            Info.ID = 5;
                            Info.Type = 3;
                            Info.Owner = Entry;
                            Info.bMainBox = 1;
                            Info.bInput = 1;
                            Info.SetSystemMessage(0x49);
                            Info.SetButtons(Name, NULL);
                            Info.Unknown74 = E->Count;
                            Info.Flag18 = E->ID;
                            Info.SetParams(E->ObjectID, -1);
                            GConsole->ShowDialog(&Info);
                        }
                        else
                        {
                            appSprintf(Name, TEXT("%s"), Item->MakeDisplayName());
                            NDialogInfo Info;
                            Info.ID = 5;
                            Info.Owner = Entry;
                            Info.bMainBox = 1;
                            Info.bInput = 0;
                            Info.SetSystemMessage(0x4A);
                            Info.SetButtons(Name, NULL);
                            Info.Flag18 = E->ID;
                            Info.SetParams(E->ObjectID, -1);
                            GConsole->ShowDialog(&Info);
                        }
                    }
                    else if (Inventory->IsInRecipe(X, Y))
                    {
                        // the item table (GL2GameData + 0x1818) and its record of the item: the crystal count sits at +0x74
                        void* Data = GL2GameData.ItemTable->FindRef(E->ID);   // 0x101514A0 in the stock (NItemMapByKey::FindByKey)
                        User* Me = GConsole->GetMyUser();
                        if (CanCrystallizeFlag(Me) && Data && ::nw::Field<INT>(Data, 0x74) != 0)
                        {
                            TCHAR Name[64];
                            appSprintf(Name, TEXT("%s"), Item->MakeDisplayName());
                            NDialogInfo Info;
                            Info.ID = 6;
                            Info.Owner = Entry;
                            Info.bMainBox = 1;
                            Info.bInput = 0;
                            Info.SetSystemMessage(0x150);
                            Info.SetButtons(Name, NULL);
                            Info.SetParams(E->ObjectID, -1);
                            GConsole->ShowDialog(&Info);
                        }
                    }
                }
            }
            else if (Target->IsA(EQUIP_CLASS_Console))
            {
                if (!GConsole->GetTargetedActor(1))
                {
                    TCHAR Name[64];
                    appSprintf(Name, TEXT("%s"), Item->GetName());
                    DropLocation = static_cast<UGameEngine*>(GConsole->Engine)->MouseHitLocation;
                    if (Item->IsStackable() && E->Count > 1)
                    {
                        NDialogInfo Info;
                        Info.ID = 4;
                        Info.Type = 3;
                        Info.Owner = Entry;
                        Info.bMainBox = 1;
                        Info.bInput = 1;
                        Info.SetSystemMessage(0x47);
                        Info.SetButtons(Name, NULL);
                        Info.Flag18 = E->ID;
                        Info.Unknown74 = E->Count;
                        Info.SetParams(E->ObjectID, -1);
                        Info.Location = DropLocation;
                        GConsole->ShowDialog(&Info);
                    }
                    else
                    {
                        NDialogInfo Info;
                        Info.ID = 4;
                        Info.Owner = Entry;
                        Info.bMainBox = 1;
                        Info.bInput = 0;
                        Info.SetSystemMessage(400);
                        Info.SetButtons(Name, NULL);
                        Info.SetParams(E->ObjectID, -1);
                        Info.Location = DropLocation;
                        GConsole->ShowDialog(&Info);
                    }
                }
            }
        }
    }
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x10096370, NCEquipItemWnd, OnLButtonUp)

// 0x10096da0 (vslot 64) - the right button takes off the item of the slot under the mouse (the shortcut selections of
// the macros are forgotten) and drops what was carried
INT NCEquipItemWnd::OnRButtonDown(WPARAM wParam, LPARAM lParam)
{
    Selected = HitTest(LOWORD(lParam), HIWORD(lParam));
    if (Selected >= 0 && Selected < NumSlots)
    {
        for (INT i = 0; i < Items.Num(); i++)
            if (UnEquipIfInSlot(this, Items(i), Selected, 1))
                break;
    }
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x10096da0, NCEquipItemWnd, OnRButtonDown)

// 0x10097240 (vslot 64) - takes off the item of the slot under the mouse; the release that follows moves nothing
INT NCEquipItemWnd::OnLButtonDblClick(WPARAM wParam, LPARAM lParam)
{
    bNoDrag = 1;
    Selected = HitTest(LOWORD(lParam), HIWORD(lParam));
    if (Selected >= 0 && Selected < NumSlots)
    {
        for (INT i = 0; i < Items.Num(); i++)
            if (UnEquipIfInSlot(this, Items(i), Selected, 0))
                break;
    }
    UnselectAll();
    Selected = -1;
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x10097240, NCEquipItemWnd, OnLButtonDblClick)

// 0x100986a0 (vslot 38) - the entries go (their tool tips' destructor and the memory; the tattoos are left to Clear), then NCWnd's
INT NCEquipItemWnd::OnDestroy()
{
    guard(NCEquipItemWnd::OnDestroy);
    for (INT i = 0; i < Items.Num(); i++)
    {
        NItemEntry* Item = Items(i);
        if (Item)
        {
            AsC4(Item)->ToolTip.~NToolTipInfo();   // inline in the stock code
            appFree(Item);
        }
    }
    Items.Empty();
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100986a0, NCEquipItemWnd, OnDestroy)

// 0x10097480 - the five icons come from their names ("NWindow.BlackTexture" for those missing), written back into Info; 0
// when the slots are taken. Every add checks again whether an item with Unknown1C98 is equipped.
UBOOL NCEquipItemWnd::AddItem(NItemInfo* Info)
{
    if (Items.Num() >= NumSlots)
        return 0;
    NItemInfoC4* New = AsC4(Info);
    for (INT k = 0; k < 5; k++)
    {
        if (New->IconName[k][0])
            New->Icon[k] = FindTexture(New->IconName[k]);
    }
    for (INT k = 0; k < 5; k++)
    {
        if (!New->Icon[k])
            New->Icon[k] = FindTexture(TEXT("NWindow.BlackTexture"));
    }
    NItemEntry* Item = NewItemEntry(Info);
    AsC4(Item)->ToolTip.Fill(Item, 2, 0);
    Items.AddItem(Item);
    for (INT i = 0; i < Items.Num(); i++)
    {
        if (AsC4(Items(i))->Unknown1C98)
        {
            bHasUnknown1C98 = 1;
            break;
        }
        bHasUnknown1C98 = 0;
    }
    return 1;
}
NW_HOOK_METHOD(0x10097480, NCEquipItemWnd, AddItem)

// 0x10098770 - the item is deleted; bHasUnknown1C98 stays as it was
UBOOL NCEquipItemWnd::RemoveItem(INT ObjectID)
{
    for (INT i = 0; i < Items.Num(); i++)
    {
        if (AsC4(Items(i))->ObjectID == ObjectID)
        {
            NItemEntry* Item = Items(i);
            Items.Remove(i);
            if (Item)
                DeleteEntry(Item);
            return 1;
        }
    }
    return 0;
}
NW_HOOK_METHOD(0x10098770, NCEquipItemWnd, RemoveItem)

// 0x10098840 - a tattoo of the game data: its name, text and icon (the black texture when it has none), usable or not, at the
// next of the boxes of the column (x 216, y 27 each from 8, 24 x 24); the tool tip is its name's
void NCEquipItemWnd::AddHenna(INT HennaID, INT bUsable)
{
    FL2HennaData* Data = GL2GameData.GetHennaData(HennaID);
    if (Data)
    {
        NHennaEntry* Henna = (NHennaEntry*)appMalloc(sizeof(NHennaEntry), TEXT("new"));
        if (Henna)
            Henna = new ((void*)Henna) NHennaEntry();
        NItemEntryC4* Entry = AsC4((NItemEntry*)Henna);
        appStrcpy(Entry->Name, *Data->Name);
        appStrcpy(Entry->Description, *Data->Text);
        appStrcpy(Entry->IconName[0], *Data->IconName);
        Entry->UnknownC34 = bUsable;
        Entry->Icon[0] = FindTexture(Entry->IconName[0]);
        if (!Entry->Icon[0])
            Entry->Icon[0] = FindTexture(TEXT("NWindow.BlackTexture"));
#if NW_CUSTOM
        INT HX, HY;
        HennaXY(Hennas.Num(), HX, HY);
        Henna->Box.Y = HY;
        Henna->Box.X = HX + 1;
#else
        Henna->Box.Y = Hennas.Num() * 0x1B + 8;
        Henna->Box.X = 0xD8;
#endif
        Henna->Box.W = 24.0f;
        Henna->Box.H = 24.0f;
        Entry->ToolTip.Fill(Henna, NTT_Name, 0);
        Hennas.AddItem(Henna);
    }
}
NW_HOOK_METHOD(0x10098840, NCEquipItemWnd, AddHenna)

// 0x1009a590 (name ours) - every tattoo deleted
void NCEquipItemWnd::ClearHennas()
{
    for (INT i = 0; i < Hennas.Num(); i++)
        if (Hennas(i))
            DeleteEntry(Hennas(i));
    Hennas.Empty();
}
NW_HOOK_METHOD(0x1009a590, NCEquipItemWnd, ClearHennas)

// 0x100987e0
void NCEquipItemWnd::Clear()
{
    for (INT i = 0; i < Items.Num(); i++)
        if (Items(i))
            DeleteEntry(Items(i));
    Items.Empty();
}
NW_HOOK_METHOD(0x100987e0, NCEquipItemWnd, Clear)

// stock body shared with NCEditBox's (0x100a5d70)
void NCEquipItemWnd::ShowWindow()
{
    NCWnd::ShowWindow();
}

// stock body shared with NCChatWnd's (0x1003dab0)
void NCEquipItemWnd::HideWindow()
{
    NCWnd::HideWindow();
}

NW_CHECK_VTABLE(NCEquipItemWnd, 0x101b41d0, 76)
