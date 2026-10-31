/*=============================================================================
    NCInvenItemWnd.cpp - a page of the inventory's items. Stock: 0x10062360 .. 0x100641e7,
    0x10065060 .. 0x1006590f, 0x10067690 .. 0x1006819f and 0x10068440 .. 0x100684c9 (the
    paperdoll and the inventory sit in between), plus the destructor (0x1005fe00) and
    WindowProc (0x10057b10).
=============================================================================*/
#include "nwindow/NCInvenItemWnd.h"
#if NW_CUSTOM
#include "nwindow/NCustomAutoPotion.h"
#endif
#include "nwindow/NCButton.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NCEquipItemWnd.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCInventoryWnd.h"
#include "nwindow/NCShortCutWnd.h"
#include "nwindow/NMacroEntry.h"
#include "nwindow/NClasses.gen.h"
#include "nwindow/NConsoleWndC4.h"
#include "nwindow/NDialogInfo.h"
#include "nwindow/NDropDialogInfo.h"
#include <stddef.h>

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
    MSG_MoveHowMany = 72,       // "How many $s1(s) do you want to move?"
    MSG_DestroyHowMany = 73,    // "How many $s1(s) do you want to destroy?"
    MSG_Destroy = 74,           // "Destroy $s1. Do you want to continue?"
    MSG_CannotDropHere = 179,   // "You cannot discard items here."
    MSG_Crystallize = 336,      // "$s1 is crystallized. Do you want to continue?"
    MSG_AreYouSure = 365,       // "Are you sure?"
    MSG_Drop = 400,             // "Discard $s1. Do you wish to continue?"
    MSG_Place = 651,            // "Place $s1 in the current location and direction..."
};

static FORCEINLINE NCDialogBox* MainDialogBox()
{
    return GDesktop->MainDialogBox;
}

// 0x10099790 - the item grid's defaults (NCItemWnd's constructor, then the vtable and no double click pending)
NCInvenItemWnd::NCInvenItemWnd()
{
    bDblClicked = 0;
}
NW_HOOK_CTOR(0x10099790, NCInvenItemWnd)

// C2 CANDIDATE: the C2 function is not the same code as the C1 one; kept only while it matches the stock one (verify_data)
// 0x100997b0 (ret 0x18) - the grid of the six arguments (columns, rows, most items, icon size, gaps) and the two frames
// the unselected icons are drawn in
NCInvenItemWnd::NCInvenItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize, INT InGapX, INT InGapY)
    : NCItemWnd(InColumns, InVisibleRows, InMaxItems, InCellSize, InGapX, InGapY)
{
    bDblClicked = 0;
    OutlineDownTex = FindTexture(TEXT("l2ui_ch3.InventoryWnd.Inventory_OutLine_down"), 1);
    OutlineTex = FindTexture(TEXT("l2ui_ch3.InventoryWnd.Inventory_OutLine"), 1);
}
NW_HOOK_CTOR_ARGS(0x100997b0, NCInvenItemWnd, (INT A, INT B, INT C, INT D, INT E, INT F), (A, B, C, D, E, F))

// 0x1008f380
NW_HOOK_DTOR(0x1008f380, NCInvenItemWnd)

// 0x10093ce0 (vslot 25) - the stock body is shared with NCPetInvenItemWnd's (the double click is the virtual at
// slot 73 in both); C2 has no dialog result message any more
INT NCInvenItemWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCItemWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == WM_LBUTTONDBLCLK)
        return OnLButtonDblClick(wParam, lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x10093ce0, NCInvenItemWnd, WindowProc)

// The shortcut bars forget a pending selection (C2: the bars' list sits behind the desktop, +0xF8; 0x10056a20
// counts them, 0x10060ce0 gets one, 0x1008b4b0 clears its slots and the flag at +0x9D4).
static void ClearShortCutSelections()
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

// Uses the item the way a double click and the right button do: an item with the flag at +0x1BA4 asks first (dialog 0x21,
// system message 0x31E with the item's name on the button), one with a message id (GetUnknown19AC) shows that message
// (dialog 0x45), any other is used at once. C4: the stock tests nothing else (the C3 body asked the console first: its address,
// remapped by the C3 -> C4 map, named NConsoleLeftFns::IsDwarf 0x100753D0, a test of the player's race that the stock never makes
// here; found by the differential test t_invpage.py, and by the audit lane's check_literals)
static FORCEINLINE void UseItem(NCInvenItemWnd* Wnd, NItemEntry* Item)
{
    if (Item && Item->Unknown1998)
    {
        // the stock keeps a 0x80-byte name buffer and two NDialogInfo slots (0xA0 each) in these frames (0x1C0 bytes); our
        // compiler lets the two dialogs share one slot, so the buffer is made as large as the second slot too: the frame stays
        // the stock's plus the /GS cookie (tools/port/check_frames.py), and a long item name has room
        TCHAR Name[64 + 80];
        appSprintf(Name, TEXT("%s"), Item->GetName());
        NDialogInfo Dialog;
        Dialog.Owner = Wnd->Entry;
        Dialog.ID = 0x21;
        Dialog.bMainBox = 1;
        Dialog.bInput = 0;
        Dialog.SetSystemMessage(0x31E);
        Dialog.SetButtons(Name, NULL);
        Dialog.SetParams(Item->ObjectID, -1);
        GConsole->ShowDialog(&Dialog);
    }
    else if (Item->GetUnknown19AC() > 0)
    {
        NDialogInfo Dialog;
        Dialog.Owner = Wnd->Entry;
        Dialog.ID = 0x45;
        Dialog.bMainBox = 1;
        Dialog.bInput = 0;
        Dialog.SetSystemMessage(Item->GetUnknown19AC());
        Dialog.SetParams(Item->ObjectID, -1);
        GConsole->ShowDialog(&Dialog);
    }
    else
        GConsole->RequestUseItem(Item->ObjectID);
}

// 0x10094be0 (vslot 70) - the right button uses the item under the mouse (the shortcut selections of the macros are
// forgotten) and drops what was carried
INT NCInvenItemWnd::OnRButtonDown(WPARAM wParam, LPARAM lParam)
{
    INT Index = HitTest(LOWORD(lParam), HIWORD(lParam), 1, 1);
    Selected = Index;
    if (Index >= 0 && Index < Items.Num())
    {
        UseItem(this, Items(Index));
        ClearShortCutSelections();
    }
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x10094be0, NCInvenItemWnd, OnRButtonDown)

// 0x10094e40 (vslot 68) - selects the item under the mouse and picks it up (unless bNoDrag, which lasts one click)
INT NCInvenItemWnd::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    INT Index = HitTest(LOWORD(lParam), HIWORD(lParam), 1, 1);
    HoverIndex = Index;
    Selected = Index;
    if (Index >= 0 && Index < Items.Num())
    {
        UnselectAll();
        Items(Selected)->Select();
        if (!bNoDrag)
        {
            bDragging = 1;
            GConsole->PlayInterfaceSound(SOUND_PickUp);
            return 1;
        }
        bNoDrag = 0;
    }
    return 1;
}
NW_HOOK_METHOD(0x10094e40, NCInvenItemWnd, OnLButtonDown)

// 0x10094ed0 (vslot 81) - a double click uses the item under the mouse; the release that follows moves nothing
INT NCInvenItemWnd::OnLButtonDblClick(WPARAM wParam, LPARAM lParam)
{
    bNoDrag = 1;
    bDragging = 0;
    INT Index = HitTest(LOWORD(lParam), HIWORD(lParam), 1, 1);
    Selected = Index;
    if (Index >= 0 && Index < Items.Num())
        UseItem(this, Items(Index));
    bDblClicked = 1;
    UnselectAll();
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x10094ed0, NCInvenItemWnd, OnLButtonDblClick)

// 0x100950e0 - the item with Info's object id takes Info's fields and a new tool tip; the entry built for that is
// never freed, as in the stock code
UBOOL NCInvenItemWnd::UpdateItem(NItemInfo* Info)
{
    for (INT i = 0; i < Items.Num(); i++)
    {
        NItemEntry* Found = Items(i);
        if (Info->ObjectID == Found->ObjectID)
        {
            Info->Icon = FindTexture(Info->IconName, 1);
            if (!Info->Icon)
                Info->Icon = FindTexture(TEXT("NWindow.BlackTexture"), 1);
            NItemEntry* Updated = NewItemEntry(Info);
            Updated->FillToolTip(2, 0);
            Found->Copy(Updated);
            return 1;
        }
    }
    return 0;
}
NW_HOOK_METHOD(0x100950e0, NCInvenItemWnd, UpdateItem)

// 0x100951e0
INT NCInvenItemWnd::GetAdena()
{
    INT Adena = 0;
    for (INT i = 0; i < Items.Num(); i++)
    {
        NItemEntry* Item = Items(i);
        if (Item && Item->ID == ITEM_ADENA)
            Adena += Item->Count;
    }
    return Adena;
}
NW_HOOK_METHOD(0x100951e0, NCInvenItemWnd, GetAdena)

// 0x10095210 (vslot 62; the guard's name is NCItemWnd's) - the icons from the page's first row, 5 pixels apart, a row of
// CellSize + 3; a selected one in the selection frame, the others in two frames of their own (34 and 35 pixels)
INT NCInvenItemWnd::OnPaint(NCGDevice* Device)
{
    guard(NCItemWnd::OnPaint);
    Device->SetClip(ScreenX - 1, ScreenY - 1, (INT)(Width + 2.0f), (INT)(Height + 3.0f));
    Device->BeginPaint(ScreenX, ScreenY);
    INT X = 1;
    INT Y = 1;
    INT First = TopRow * Columns;
    for (INT Index = First; Index < Items.Num(); Index++)
    {
        NItemEntry* Item = Items(Index);
        if (!Item)
            continue;
        if (Index % Columns == 0)
        {
            X = 1;
            if (Index != TopRow * Columns)
                Y = Y + CellSize + 3;
            if ((CellSize + 3) * VisibleRows + 1 <= Y)
                break;
        }
        else
            X = X + CellSize + 5;
        if (Item->GetIcon())
            Device->DrawTexture(X, Y, CellSize, CellSize, 0, 0, CellSize, CellSize, Item->GetIcon(), 0xFF, 1);
        if (Item->IsSelected())
            Device->DrawTexture(X - 1, Y - 1, 0x22, 0x22, 0, 0, 0x22, 0x22, SelectTex, 0xFF, 1);
        else
        {
            Device->DrawTexture(X - 1, Y - 1, 0x22, 0x22, 0, 0, 0x22, 0x22, OutlineDownTex, 0xFF, 1);
            Device->DrawTexture(X - 1, Y - 1, 0x23, 0x23, 0, 0, 0x23, 0x23, OutlineTex, 0xFF, 1);
        }
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10095210, NCInvenItemWnd, OnPaint)

// 0x10095430 (vslot 67) - the item under the mouse; while an item is carried the cursor is its icon, and over the
// inventory's trash or recipe icon that icon shows its dragged picture (the window keeps the flags)
INT NCInvenItemWnd::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    INT X = LOWORD(lParam);
    INT Y = HIWORD(lParam);
    HoverIndex = HitTest(X, Y, 1, 1);
    if (IsFocused())
    {
        if (bDragging && Selected >= 0 && Selected < Items.Num())
        {
            SetCursor(4, Items(Selected)->GetIcon());
            LockCursor();
        }
        NCInventoryWnd* Inventory = (NCInventoryWnd*)Entry->Parent->Wnd;
        if (!Inventory->IsA(NClass::NCInventoryWnd()))
            return 1;
        NRectF Box;
        Box.X = Inventory->TrashX + Inventory->ScreenX;
        Box.Y = Inventory->TrashY + Inventory->ScreenY;
        Box.W = Inventory->TrashW;
        Box.H = Inventory->TrashH;
        if (PtInRect(X, Y, Box))
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
        if (PtInRect(X, Y, Box))
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
    return 1;
}
NW_HOOK_METHOD(0x10095430, NCInvenItemWnd, OnMouseMove)

// the player's crystallize flag: 4 bytes after where UE2Engine.h puts User::bCanCrystallize (stock 0x10097D5C: mov ecx, [eax + 0x1F8])
static FORCEINLINE INT CanCrystallizeFlag(User* Me)
{
    return ::nw::Field<INT>(Me, 0x1F8);
}

// the item table's record of an item (stock 0x10097D38 / 0x10097E71: FindByKey 0x101514A0 on the table pointer GL2GameData + 0x1818,
// the key is the address of the entry's ID); NULL when the table has none
static FORCEINLINE NViewItemDataC4* FindItemData(NItemEntry* Item)
{
    return (NViewItemDataC4*)GL2GameData.ItemTable->FindRef(Item->ID);
}

static_assert(offsetof(NItemEntry, ID) == 0x1B20, "NItemEntry::ID is +0x1B20 in the stock (the drop dialogs read it there)");

// 0x100976c0 (vslot 69; the stock's guard-less body; ret 8) - the release of the item carried (unless a double click has just
// used it): inside the page it moves to the place under the mouse (the last one when that is past the end); outside it the
// window under the cursor decides: the paperdoll puts it on (the console's use request), the shortcut bar registers it, the pet's
// window gets it (an amount asks first), the inventory's trash icon asks to destroy it, its recipe icon to crystallize it (when
// the player may), the console (the ground: no actor under the cursor) asks to discard it or, for the etc items of kind 8, to
// place it; the recipe book's item window ignores it. C4: the enchant scroll part of C3 is gone from this function.
INT NCInvenItemWnd::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    INT X = LOWORD(lParam);
    INT Y = HIWORD(lParam);
    if (Selected < 0 || Selected >= Items.Num())
    {
        bDragging = 0;
        return 1;
    }
    if (bDragging && !bDblClicked)
    {
        NItemEntry* Item = Items(Selected);
        if (!Item)
        {
            bDragging = 0;
            return 1;
        }
        if (IsInWindow(X, Y))
        {
            INT Target = HitTest(X, Y, 1, 1);
            INT Current = Selected;
            if (Target != Current && Target >= 0 && Current >= 0 && Current < Items.Num())
            {
                if (Target >= Items.Num())
                {
                    Items.Remove(Current, 1);
                    Items.AddItem(Item);
                    Selected = Items.Num() - 1;
                    bDblClicked = 0;
                    bDragging = 0;
                    return 1;
                }
                Items.Remove(Current, 1);
                Items.Insert(Target, 1);
                Items(Target) = Item;
                bDblClicked = 0;
                bDragging = 0;
                return 1;
            }
        }
        else
        {
            NWndEntry* Found = FindWindowAt(X, Y);
            if (Found && Found->Wnd)
            {
                NCWnd* Target = Found->Wnd;
#if NW_CUSTOM
                if (NCustomAutoPotion::OnDropItem(Target, X, Y, Item))   // custom: onto the automatic potions window
                {
                    UnselectAll();
                    Selected = -1;
                    bDblClicked = 0;
                    bDragging = 0;
                    return 1;
                }
#endif
                if (Target->IsA(NClass::NCEquipItemWnd()))
                {
                    // onto the paperdoll: the item is used (put on); the slot under the mouse is asked and not used, as in the stock
                    ((NCEquipItemWnd*)Target)->HitTest(X, Y);
                    GConsole->RequestUseItem(Items(Selected)->ObjectID);
                    UnselectAll();
                    Selected = -1;
                    bDblClicked = 0;
                    bDragging = 0;
                    return 1;
                }
                else if (Target->IsA(NClass::NCShortCutWnd()))
                {
                    INT ShortCut = ((NCShortCutWnd*)Target)->SlotAtPoint(X, Y);
                    if (ShortCut >= 0)
                    {
                        L2ParamStack Params(10);
                        Params.PushBack((void*)1);
                        Params.PushBack((void*)(INT_PTR)ShortCut);
                        Params.PushBack((void*)(INT_PTR)Items(Selected)->ObjectID);
                        Params.PushBack((void*)1);
                        GConsole->RequestShortCutReg(Params);
                    }
                    UnselectAll();
                    Selected = -1;
                    bDblClicked = 0;
                    bDragging = 0;
                    return 1;
                }
                else if (Target->IsA(NClass::NCPetWnd()))
                {
                    if (!Item->IsStackable() || Item->Count <= 1)
                    {
                        L2ParamStack Params(10);
                        Params.PushBack((void*)(INT_PTR)Items(Selected)->ObjectID);
                        Params.PushBack((void*)1);
                        GConsole->RequestGiveItemToPet(Params);
                        bDblClicked = 0;
                        bDragging = 0;
                        return 1;
                    }
                    TCHAR Name[64];
                    appSprintf(Name, TEXT("%s"), Item->GetName());
                    NDialogInfo Info;
                    Info.Owner = Entry;
                    Info.ID = DIALOG_GIVE_TO_PET;
                    Info.Type = 3;
                    Info.bMainBox = 1;
                    Info.bInput = 1;
                    Info.SetSystemMessage(0x48);
                    Info.SetButtons(Name, NULL);
                    Info.Flag18 = Item->ID;
                    Info.Unknown74 = Item->Count;
                    Info.SetParams(Item->ObjectID, -1);
                    GConsole->ShowDialog(&Info);
                }
                else if (Target->IsA(NClass::NCButton()) && Target->Entry->Parent && Target->Entry->Parent->Wnd
                         && Target->Entry->Parent->Wnd->IsA(NClass::NCInventoryWnd()))
                {
                    // onto one of the inventory's icons (the trash or the recipe one): both lose their carried-over picture first
                    NCInventoryWnd* Inventory = (NCInventoryWnd*)Target->Entry->Parent->Wnd;
                    Inventory->bOverTrash = 0;
                    Inventory->bOverRecipe = 0;
                    Inventory->TrashButton->SetNormalTex(TEXT("L2UI_CH3.InventoryWnd.Inventory_trash"));
                    Inventory->TrashButton->SetPushedTex(TEXT("L2UI_CH3.InventoryWnd.Inventory_trash"));
                    Inventory->RecipeButton->SetNormalTex(TEXT("L2UI_CH3.InventoryWnd.Inventory_recipe"));
                    Inventory->RecipeButton->SetPushedTex(TEXT("L2UI_CH3.InventoryWnd.Inventory_recipe"));
                    if (Inventory->IsInTrash(X, Y))
                    {
                        TCHAR Name[64];
                        if (!Item->IsStackable() || Item->Count <= 1)
                        {
                            appSprintf(Name, TEXT("%s"), Item->MakeDisplayName());
                            NDialogInfo Info;
                            Info.Owner = Entry;
                            Info.ID = DIALOG_DESTROY_ITEM;
                            Info.bMainBox = 1;
                            Info.bInput = 0;
                            Info.SetSystemMessage(MSG_Destroy);
                            Info.SetButtons(Name, NULL);
                            Info.Flag18 = Item->ID;
                            Info.SetParams(Item->ObjectID, -1);
                            GConsole->ShowDialog(&Info);
                        }
                        else
                        {
                            appSprintf(Name, TEXT("%s"), Item->MakeDisplayName());
                            NDialogInfo Info;
                            Info.Owner = Entry;
                            Info.ID = DIALOG_DESTROY_ITEM;
                            Info.Type = 3;
                            Info.bMainBox = 1;
                            Info.bInput = 1;
                            Info.SetSystemMessage(MSG_DestroyHowMany);
                            Info.SetButtons(Name, NULL);
                            Info.Flag18 = Item->ID;
                            Info.Unknown74 = Item->Count;
                            Info.SetParams(Item->ObjectID, -1);
                            GConsole->ShowDialog(&Info);
                        }
                    }
                    else if (Inventory->IsInRecipe(X, Y))
                    {
                        // the crystal count of the item's record sits at +0x74 (stock 0x10097D72)
                        NViewItemDataC4* Data = FindItemData(Item);
                        User* Me = GConsole->GetMyUser();
                        if (CanCrystallizeFlag(Me) && Data && ::nw::Field<INT>(Data, 0x74) != 0)
                        {
                            TCHAR Name[64];
                            appSprintf(Name, TEXT("%s"), Item->MakeDisplayName());
                            NDialogInfo Info;
                            Info.Owner = Entry;
                            Info.ID = DIALOG_CRYSTALLIZE_ITEM;
                            Info.bMainBox = 1;
                            Info.bInput = 0;
                            Info.SetSystemMessage(0x150);
                            Info.SetButtons(Name, NULL);
                            Info.SetParams(Item->ObjectID, -1);
                            GConsole->ShowDialog(&Info);
                        }
                    }
                }
                else if (!Target->IsA(NClass::NCRecipeBookItemWnd()) && Target->IsA(NClass::NCConsole()))
                {
                    // onto the game's screen (the ground): nothing happens while an actor is targeted
                    if (!GConsole->GetTargetedActor(1))
                    {
                        TCHAR Name[64];
                        appSprintf(Name, TEXT("%s"), Item->GetName());
                        DropLocation = static_cast<UGameEngine*>(GConsole->Engine)->MouseHitLocation;
                        // an etc item of kind 8 is placed, not thrown away
                        UBOOL bPlace = 0;
                        NViewItemDataC4* Data = FindItemData(Item);
                        if (Data && Data->Class == ITEMCLASS_Etc && Data->EtcType == 8)
                            bPlace = 1;
                        if (Item->IsStackable() && Item->Count > 1 && !bPlace)
                        {
                            NDialogInfo Info;
                            Info.Owner = Entry;
                            Info.ID = DIALOG_DROP_ITEM;
                            Info.Type = 3;
                            Info.bMainBox = 1;
                            Info.bInput = 1;
                            Info.SetSystemMessage(MSG_DropHowMany);
                            Info.SetButtons(Name, NULL);
                            Info.Flag18 = Item->ID;
                            Info.Unknown74 = Item->Count;
                            Info.SetParams(Item->ObjectID, -1);
                            Info.Location = static_cast<UGameEngine*>(GConsole->Engine)->MouseHitLocation;
                            GConsole->ShowDialog(&Info);
                        }
                        else if (bPlace)
                        {
                            NDialogInfo Info;
                            Info.Owner = Entry;
                            Info.ID = DIALOG_DROP_ITEM;
                            Info.bMainBox = 1;
                            Info.bInput = 0;
                            Info.SetSystemMessage(MSG_Place);
                            Info.SetButtons(Name, NULL);
                            Info.SetParams(Item->ObjectID, -1);
                            Info.Location = static_cast<UGameEngine*>(GConsole->Engine)->MouseHitLocation;
                            GConsole->ShowDialog(&Info);
                        }
                        else
                        {
                            NDialogInfo Info;
                            Info.Owner = Entry;
                            Info.ID = DIALOG_DROP_ITEM;
                            Info.bMainBox = 1;
                            Info.bInput = 0;
                            Info.SetSystemMessage(MSG_Drop);
                            Info.SetButtons(Name, NULL);
                            Info.SetParams(Item->ObjectID, -1);
                            Info.Location = static_cast<UGameEngine*>(GConsole->Engine)->MouseHitLocation;
                            GConsole->ShowDialog(&Info);
                        }
                    }
                }
            }
        }
    }
    bDblClicked = 0;
    bDragging = 0;
    return 1;
}
NW_HOOK_METHOD(0x100976c0, NCInvenItemWnd, OnLButtonUp)

// 0x10098170 - 0 when the page is full
UBOOL NCInvenItemWnd::AddItem(NItemInfo* Info)
{
    if (MaxItems <= Items.Num())
        return 0;
    Info->Icon = FindTexture(Info->IconName, 1);
    if (!Info->Icon)
        Info->Icon = FindTexture(TEXT("NWindow.BlackTexture"), 1);
    NItemEntry* Item = NewItemEntry(Info);
    Item->FillToolTip(2, 0);
    Items.AddItem(Item);
    UpdateScrollRange();
    return 1;
}
NW_HOOK_METHOD(0x10098170, NCInvenItemWnd, AddItem)

// 0x100989c0 - the entry is deleted and the scroll range updated
UBOOL NCInvenItemWnd::RemoveItem(INT ObjectID)
{
    for (INT i = 0; i < Items.Num(); i++)
    {
        if (Items(i)->ObjectID == ObjectID)
        {
            NItemEntry* Item = Items(i);
            Items.Remove(i);
            if (Item)
            {
                Item->~NItemEntry();   // the stock calls the destructor and gives the memory back to GMalloc by hand
                appFree(Item);
            }
            UpdateScrollRange();
            return 1;
        }
    }
    return 0;
}
NW_HOOK_METHOD(0x100989c0, NCInvenItemWnd, RemoveItem)

// stock body shared with NCPetInvenItemWnd (0x10091c70), which keeps the address
UBOOL NCInvenItemWnd::HasItem(INT ObjectID)
{
    for (INT i = 0; i < Items.Num(); i++)
        if (ObjectID == Items(i)->ObjectID)
            return 1;
    return 0;
}

// stock body shared with NCPetInvenItemWnd (0x10091cc0), which keeps the address
UBOOL NCInvenItemWnd::FindItem(INT ObjectID, INT* Index)
{
    for (INT i = 0; i < Items.Num(); i++)
    {
        if (ObjectID == Items(i)->ObjectID)
        {
            *Index = i;
            return 1;
        }
    }
    return 0;
}

NW_CHECK_VTABLE(NCInvenItemWnd, 0x101b4300, 82)
