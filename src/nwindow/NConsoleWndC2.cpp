/*=============================================================================
    NConsoleWndC2.cpp - the virtuals of the console that only C2 has (slots of the C2 vtable the
    C1 console did not have): the manor lists, the recipes, the henna, the macros, the radar's
    and the shortcut bar's, the town map, the observer, the Christmas seal, the screen resize.
    Names are the ones of include/ue2/gen/UL2ConsoleWnd.vt.inc (a UGameEngine handler's, a guard
    string's, or ours: the note of tools/ue2_data/l2consolewnd_slots_c2.csv says which).

    The windows these slots feed are classes new in C2 (recipes, henna, the macro window...): they are
    reached from the desktop's pointers (NCConsole.h keeps them as NCWnd*, the Desk* helpers below give
    them back as the classes they are) and called through their own methods; nw::Field is left for the
    fields no header names yet.
=============================================================================*/
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NConsoleWndC4.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCAuthWnd.h"
#include "nwindow/NCXMasSealWnd.h"
#include "nwindow/NMacroEntry.h"
#include "nwindow/NCWnd.h"
#include "nwindow/NItemInfo.h"
#include "nwindow/NGameData.h"
#include "nwindow/NGameGuard.h"
#include "nwindow/NDialogInfo.h"
#include "nwindow/NCShortCutWnd.h"
#include "nwindow/NCSystemMenuWnd.h"
#include "nwindow/NCGMSnoopManager.h"
#include "nwindow/NCEmpowerWnd.h"
#include "nwindow/NCGMWnd.h"
#include "nwindow/NCHennaInfoWnd.h"
#include "nwindow/NCHennaListBox.h"
#include "nwindow/NCHennaListWnd.h"
#include "nwindow/NCInventoryWnd.h"
#include "nwindow/NCMacroListWnd.h"
#include "nwindow/NCRecipeBookWnd.h"
#include "nwindow/NCRecipeBuyListWnd.h"
#include "nwindow/NCRecipeBuyManufactureWnd.h"
#include "nwindow/NCRecipeListBox.h"
#include "nwindow/NCRecipeManufactureWnd.h"
#include "nwindow/NCRecipeShopWnd.h"
#include "nwindow/NCWarningDialogBox.h"
#include "nwindow/NTownMapWnd.h"
#include "nwindow/NRecipeData.h"
#include "nwindow/NCBroadcastObserverWnd.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGodWnd.h"
#include "nwindow/NCHeroTowerWnd.h"
#include "nwindow/NCManorCropInfoSettingWnd.h"
#include "nwindow/NCManorCropSellChangeWnd.h"
#include "nwindow/NCManorCropSellWnd.h"
#include "nwindow/NCManorInfoWnd.h"
#include "nwindow/NCManorSeedInfoSettingWnd.h"
#include "nwindow/NConsoleWndExt.h"
#include <stdio.h>

// the desktop's window at an offset (NCConsole.h declares the ones that are known; the new ones are not)
static FORCEINLINE NCWnd* DeskWnd(NConsoleWnd* Console, DWORD Offset)
{
    return nw::Field<NCWnd*>(Console->Desktop, Offset);
}

// the windows of the desktop that the C2 slots below feed, as the classes they are (NCConsole.h keeps the C2
// ones as NCWnd*: the offsets are the stock's)
// C3 (console-core): re-enabled, the helpers compile against the C3 headers
static FORCEINLINE NCRecipeBookWnd* DeskRecipeBook(NConsoleWnd* Console)       { return static_cast<NCRecipeBookWnd*>(Console->GetDesktop()->NewWnd134); }
static FORCEINLINE NCRecipeManufactureWnd* DeskRecipeManufacture(NConsoleWnd* Console) { return static_cast<NCRecipeManufactureWnd*>(Console->GetDesktop()->NewWnd13C); }
static FORCEINLINE NCRecipeShopWnd* DeskRecipeShop(NConsoleWnd* Console)       { return static_cast<NCRecipeShopWnd*>(Console->GetDesktop()->RecipeShopWnd); }
static FORCEINLINE NCRecipeBuyListWnd* DeskRecipeBuyList(NConsoleWnd* Console) { return static_cast<NCRecipeBuyListWnd*>(Console->GetDesktop()->NewWnd144); }
static FORCEINLINE NCRecipeBuyManufactureWnd* DeskRecipeBuyManufacture(NConsoleWnd* Console) { return static_cast<NCRecipeBuyManufactureWnd*>(Console->GetDesktop()->RecipeBuyManufactureWnd); }
static FORCEINLINE NCMacroListWnd* DeskMacroList(NConsoleWnd* Console)         { return static_cast<NCMacroListWnd*>(Console->GetDesktop()->NewWndF8); }
static FORCEINLINE NCHennaListWnd* DeskHennaList(NConsoleWnd* Console)         { return static_cast<NCHennaListWnd*>(Console->GetDesktop()->NewWnd14C); }
static FORCEINLINE NCHennaInfoWnd* DeskHennaInfo(NConsoleWnd* Console)         { return static_cast<NCHennaInfoWnd*>(Console->GetDesktop()->NewWnd150); }

// Window A takes the place of window B and keeps its own size, then is shown and focused: the dance of
// every list window of the recipes and the henna (SetPosition(B's screen X and Y, A's local W and H)).
// C3: the local rectangle is gone, A's size comes from GetLocalRect (floats, truncated: _ftol)
static FORCEINLINE void PlaceAtAndShow(NCWnd* A, NCWnd* B)
{
    NRectF Local;
    Local.X = 0;
    Local.Y = 0;
    Local.W = 0;
    Local.H = 0;
    INT X = B->ScreenX;
    INT Y = B->ScreenY;
    A->GetLocalRect(&Local);
    A->SetPosition(X, Y, (DWORD)(INT)Local.W, (DWORD)(INT)Local.H);
    A->ShowWindow();
    A->SetFocus();
}

/*-----------------------------------------------------------------------------
    Records of the stock code (names ours)
-----------------------------------------------------------------------------*/

// The anonymous members of the recipe windows that the receivers below reach (views; names ours)
struct NRecipeBookWndKind
{
    __declspec(noinline) void SetKind(INT Kind);        // 0x100F37F0 (ret 4)
};
// 0x100f37f0 - the kind of the book (+0x194): 0 titles it with the system string 1215 (0x4BF), 1 with 1214 (0x4BE) (the child at +0x7C
// takes the string id, 0x10011810)
void NRecipeBookWndKind::SetKind(INT Kind)
{
    nw::Field<INT>(this, 0x194) = Kind;
    if (Kind == 0)
        ((NCFrameCtrl*)nw::Field<void*>(this, 0x7C))->SetTitleID(0x4BF);
    else if (Kind == 1)
        ((NCFrameCtrl*)nw::Field<void*>(this, 0x7C))->SetTitleID(0x4BE);
}
NW_HOOK_METHOD(0x100f37f0, NRecipeBookWndKind, SetKind)

struct NRecipeManufactureWndExt
{
    __declspec(noinline) void SetField1D74(INT Value);  // 0x100F3940 (ret 4)
};
void NRecipeManufactureWndExt::SetField1D74(INT Value)
{
    nw::Field<INT>(this, 0x1D74) = Value;
}
NW_HOOK_METHOD(0x100f3940, NRecipeManufactureWndExt, SetField1D74)

struct NRecipeShopWndKind
{
    __declspec(noinline) void SetKind(INT Kind);        // 0x100EFBF0 (ret 4)
};
// 0x100efbf0 - the kind of the shop window (+0x11C): 1 titles it with the system string 1212 (0x4BC), else 1213 (0x4BD) (the child
// at +0x124 takes the string id)
void NRecipeShopWndKind::SetKind(INT Kind)
{
    nw::Field<INT>(this, 0x11C) = Kind;
    ((NCFrameCtrl*)nw::Field<void*>(this, 0x124))->SetTitleID(Kind == 1 ? 0x4BC : 0x4BD);
}
NW_HOOK_METHOD(0x100efbf0, NRecipeShopWndKind, SetKind)

struct NRecipeBuyListWndExt
{
    __declspec(noinline) void ClearList();              // 0x100EFDC0 (the list child's slot 77)
};
void NRecipeBuyListWndExt::ClearList()
{
    nw::VirtualCall<void>(nw::Field<void*>(this, 0xE8), 77);
}
NW_HOOK_METHOD(0x100efdc0, NRecipeBuyListWndExt, ClearList)

// the field setters the recipe shop and the sendable list share (the stock folded them): +0x108 and +0x110 of the window
struct NSetField108
{
    __declspec(noinline) void Set(INT Value);           // 0x10084BE0 (ret 4)
};
void NSetField108::Set(INT Value)
{
    nw::Field<INT>(this, 0x108) = Value;
}
NW_HOOK_METHOD(0x10084be0, NSetField108, Set)
struct NSetField110
{
    __declspec(noinline) void Set(INT Value);           // 0x10084BF0 (ret 4)
};
void NSetField110::Set(INT Value)
{
    nw::Field<INT>(this, 0x110) = Value;
}
NW_HOOK_METHOD(0x10084bf0, NSetField110, Set)

// What NCRecipeShopWnd's list takes for a recipe on sale (0xA20 bytes, built on the stack by
// AddRecipeShopSellItem and handed to the list window by address)
struct NRecipeSellEntry
{
    TCHAR Name[128];           // +0x000 the recipe item's name
    TCHAR IconName[128];       // +0x100 the product's icon
    TCHAR Description[1024];   // +0x200 the product's description
    INT Unknown00;             // +0xA00 (0)
    INT RecipeID;              // +0xA04
    INT ItemID;                // +0xA08 FL2RecipeData +0x20
    INT Unknown0C;             // +0xA0C FL2RecipeData +0x34
    INT P3;                    // +0xA10
    INT CrystalGrade;          // +0xA14 of the product (item data +0xBC for a weapon, +0x214 for an armor)
    INT P2;                    // +0xA18
    INT Unknown1C;             // +0xA1C (-1)
};
NW_ASSERT_SIZE(NRecipeSellEntry, 0xA20);

// What the henna list windows take for a dye (0x141C bytes, by address): the dye item's names and icon
// and the numbers of the HennaInfo
struct NHennaSendEntry
{
    TCHAR Name[256];           // +0x000 item data +0x40
    TCHAR IconName[256];       // +0x200 item data +0x34
    TCHAR Description[2048];   // +0x400 item data +0x44 (its additional name), or henna data +0x2C
    INT Unknown1400;           // +0x1400 (0)
    INT Info0;                 // +0x1404 HennaInfo +0x00
    INT ItemID;                // +0x1408 HennaInfo +0x04
    INT Info8;                 // +0x140C
    INT InfoC;                 // +0x1410
    INT Info10;                // +0x1414
    INT Unknown1418;           // +0x1418 (0)
};
NW_ASSERT_SIZE(NHennaSendEntry, 0x141C);

/*-----------------------------------------------------------------------------
    C2-only virtuals: the slots C2 added to the console's vtable. Those not ported yet are stubs that
    jump to the stock function (tools/verify_hooks.py wants every slot to reach its stock function).
-----------------------------------------------------------------------------*/

// 0x1006bfb0 (vslot 28) - a message on the warning dialog box of the window at +0x3950 (C3: it holds the options window
// and the warning dialog box; its own ShowWarningMessage 0x10085d20 sets the lock and the text and shows it modal),
// when there is one. The stock has a guard frame (its name string is outside the function)
void NConsoleWnd::ShowWarningDialog(INT P1, INT P2)
{
    guard(NConsoleWnd::ShowWarningDialog);
    if (GodWnd)
        static_cast<NCGodWnd*>(GodWnd)->ShowWarningMessage((const TCHAR*)P1, P2);
    unguard;
}
NW_HOOK_METHOD(0x1006bfb0, NConsoleWnd, ShowWarningDialog)

// 0x10061d00 (vslot 29) - the nProtect query (four values, by address) goes to NPGameLibApi::SendAuthQuery
// (0x10169780, cdecl; NGameGuard.h: in hook mode the library stays the original). C3: a fifth value (ret 0x14) is
// kept in a global of the stock (0x1027BA70, name ours). C4: the slot has the five values (include/ue2/gen/UL2ConsoleWnd.vt.inc was
// the C2's, with four: the stock pops 0x14). Hook mode: the body is the non-virtual GameGuardQueryX hooked at the slot's address and
// the slot's own stub jumps to it. Standalone: the slot IS GameGuardQueryX (the stock slot holds 0x1006d870 itself)
#if NW_HOOK_MODE
NW_NAKED void NConsoleWnd::GameGuardQuery(INT P1, INT P2, INT P3, INT P4, INT P5) NW_STOCK(0x1006D870)
#else
NW_SLOT_IS("?GameGuardQuery@NConsoleWnd@@UAEXHHHHH@Z", "?GameGuardQueryX@NConsoleWnd@@QAEXHHHHH@Z")   // NConsoleWnd::GameGuardQuery = NConsoleWnd::GameGuardQueryX (0x1006d870)
#endif

#if NW_HOOK_MODE
#define GGameGuardQueryExtra NW_GLOBAL(INT, 0x102D2C14)
#else
static INT GGameGuardQueryExtra;
#endif

// 0x1006d870 (ret 0x14; the slot 29 stub above jumps here) - while the GameGuard runs (the global at 0x102D2C0C) the four values go to
// NPGameLibApi::SendAuthQuery (0x10169780, cdecl, the library stays the stock's) by address, and the fifth is kept in the global at
// 0x102D2C14
#if NW_HOOK_MODE
#define GGameGuardRunningC2 NW_GLOBAL(INT, 0x102D2C0C)
#else
extern INT GGameGuardRunning;                           // src/standalone/NwGameGuard.cpp: 0, there is no GameMon object
#define GGameGuardRunningC2 GGameGuardRunning

// Standalone, no GameMon to ask: the query is answered at once, as GameMon's answer would be by the stock message handler
// (0x1006e560, the auth-answer case at 0x1006e5f6: by the fifth value of the query, kept at 0x102D2C14), with an answer of
// zeros. 0 replies to the game server (NetHandler 277 ReplyGameGuardQuery); 1 .. 4 keep the four dwords at Engine +0x360 ..
// +0x36C, which the engine's ResponseAuthGameGuard sends, and 3 (the login server's Init: UGameEngine::OnGameGuardQuery with 3)
// sends it (NetHandler 75), 4 asks for the login (NetHandler 74). Without the answer to 3 the login stops after the Init: the
// C4 login server waits for AuthGameGuard (packet 7) before RequestAuthLogin. The servers this client plays on check only the
// session id the engine puts in that packet (Lucera's AuthGameGuard; its game server ignores ReplyGameGuardQuery)
static void AnswerGameGuardQuery(NConsoleWnd* Console, INT Kind, const DWORD* Query)
{
#if NW_CUSTOM
    // custom: the query itself as the answer (what the L2 Ignition client patch did with the stock DLL: with zeros the
    // game server refused the session key on the January 2006 engine)
    const DWORD Answer[4] = { Query[0], Query[1], Query[2], Query[3] };
#else
    const DWORD Answer[4] = { 0, 0, 0, 0 };
#endif
    UL2NetHandler* Net = Console->Engine->NetHandler;
    if (Kind == 0)
    {
        Net->ReplyGameGuardQuery(Answer[0], Answer[1], Answer[2], Answer[3]);
        return;
    }
    if (Kind < 1 || Kind > 4)
        return;
    for (INT i = 0; i < 4; i++)
        nw::Field<DWORD>(Console->Engine, 0x360 + i * 4) = Answer[i];
    if (Kind == 3)
        Net->ResponseAuthGameGuard();
    else if (Kind == 4)
        Net->RequestLogin();
}
#endif
__declspec(safebuffers) void NConsoleWnd::GameGuardQueryX(INT P1, INT P2, INT P3, INT P4, INT P5)
{
    if (GGameGuardRunningC2)
    {
        struct { INT A, B, C, D; } Query = { P1, P2, P3, P4 };      // not an array: no stack cookie
        GGameGuardQueryExtra = P5;
        NGameGuard::SendAuthQuery((INT*)&Query);        // NPGameLibApi::SendAuthQuery 0x10169780 (a no-op without the library)
    }
#if !NW_HOOK_MODE
    else
    {
        GGameGuardQueryExtra = P5;
        const DWORD Query[4] = { (DWORD)P1, (DWORD)P2, (DWORD)P3, (DWORD)P4 };
        AnswerGameGuardQuery(this, P5, Query);
    }
#endif
}
NW_HOOK_METHOD(0x1006d870, NConsoleWnd, GameGuardQueryX)

// 0x1006f5b0 (vslot 31) - the security card box of the login window (the god window's +0xEC group, its +0xE8) takes the key
void NConsoleWnd::SCCheckStart(INT P1, INT P2)
{
    NCWnd* Login = nw::Field<NCWnd*>(GodWnd, 0xEC);
    if (Login)
        static_cast<NCAuthWnd*>(nw::Field<NCWnd*>(Login, 0xE8))->SetCardKey(P1, P2);
}
NW_HOOK_METHOD(0x1006f5b0, NConsoleWnd, SCCheckStart)

// 0x10066540 (vslot 60) - the inventory window (+0xCC) does its geo check (0x10080440; it takes nothing: the
// stock pops nothing, so UGameEngine::OnCheckGeo's vector is not passed on)
void NConsoleWnd::CheckGeo()
{
    GetDesktop()->InventoryWnd->ApplyItemOrder();
}
NW_HOOK_METHOD(0x10066540, NConsoleWnd, CheckGeo)

// 0x100681d0 (vslot 145) - C2 only: NetHandler slot 215 (the henna list)
void NConsoleWnd::RequestHennaItemList()
{
    Engine->NetHandler->RequestHennaItemList();
}
NW_HOOK_METHOD(0x100681d0, NConsoleWnd, RequestHennaItemList)

// 0x100681e0 (vslot 146) - C2 only: NetHandler slot 216
void NConsoleWnd::RequestHennaUnequipList()
{
    Engine->NetHandler->RequestHennaUnequipList();
}
NW_HOOK_METHOD(0x100681e0, NConsoleWnd, RequestHennaUnequipList)

// 0x1006c290 (vslot 439) - C2 only: NetHandler's +0x3f0
void NConsoleWnd::RequestObserverEndPacket(L2ParamStack& Params)
{
    Engine->NetHandler->RequestObserverEndPacket(Params);
}
NW_HOOK_METHOD(0x1006c290, NConsoleWnd, RequestObserverEndPacket)

// 0x1006c2a0 (vslot 440) - the pledge power request: the network handler's slot 253 (+0x3F4), when
// there is an engine and a handler (the stock tail-jumps)
void NConsoleWnd::RequestPledgePower(INT P1, INT P2, BYTE* P3)
{
    if (Engine && Engine->NetHandler)
        Engine->NetHandler->RequestPledgePower(P1, P2, P3);
}
NW_HOOK_METHOD(0x1006c2a0, NConsoleWnd, RequestPledgePower)

/*-----------------------------------------------------------------------------
    Recipes: the recipe book (+0x134, the list at +0x13C), the shops of recipes (+0x140, +0x144, +0x148).
-----------------------------------------------------------------------------*/

// The item info of a recipe, as the recipe lists want it (C4 views): the recipe's item (FL2RecipeData +0x20: name and description,
// weight) and its product (+0x28: icon, crystal grade)
static FORCEINLINE void FillRecipeItemInfo(NConsoleItemInfoC4& Info, BYTE* Recipe)
{
    Info.ID = *(INT*)(Recipe + 0x20);
    Info.UnknownC20 = *(INT*)(Recipe + 0x24);
    Info.ObjectID = *(INT*)(Recipe + 0x1C);
    NViewItemDataC4* Item = (NViewItemDataC4*)NItemTable::Find((INT*)(Recipe + 0x20));
    if (Item)
    {
        appStrcpy(Info.Name, *Item->Name);
        appStrcpy(Info.Description, *Item->Description);
        Info.UnknownC28 = Item->Weight;
    }
    NViewItemDataC4* Product = (NViewItemDataC4*)NItemTable::Find((INT*)(Recipe + 0x28));
    if (Product)
    {
        appStrcpy(Info.IconName, *Product->IconName);
        if (Product->Class == ITEMCLASS_C4_Weapon)
            Info.UnknownC34[7] = Product->CrystalGrade;
        else if (Product->Class == ITEMCLASS_C4_Armor)
            Info.UnknownC34[7] = Product->ArmorCrystalGrade;
    }
}

// 0x10070740 (vslot 127; ret 8) - the recipe book starts over (P1 its kind, then the list emptied and P2 kept at +0x190) and takes the
// place of the manufacture window (the desktop's +0x1A0); the book is the desktop's +0x198
void NConsoleWnd::ReceiveRecipeBookItemList(INT P1, INT P2)
{
    ((NRecipeBookWndKind*)DeskWnd(this, 0x198))->SetKind(P1);
    static_cast<NCRecipeBookWnd*>(DeskWnd(this, 0x198))->Clear();
    ((NCRecipeBookWndExt*)DeskWnd(this, 0x198))->SetField16C(P2);
    PlaceAtAndShow(DeskWnd(this, 0x198), DeskWnd(this, 0x1A0));
}
NW_HOOK_METHOD(0x10070740, NConsoleWnd, ReceiveRecipeBookItemList)

// 0x100799d0 (vslot 128) - a recipe of the book (the recipe by its index)
void NConsoleWnd::AddRecipeBookItem(INT RecipeID)
{
    BYTE* Recipe = (BYTE*)GL2GameData.GetRecipeDataByIndex(RecipeID);
    if (!Recipe)
        return;
    NItemInfo InfoRaw;
    NConsoleItemInfoC4& Info = reinterpret_cast<NConsoleItemInfoC4&>(InfoRaw);
    FillRecipeItemInfo(Info, Recipe);
    DeskRecipeBook(this)->AddRecipe(&InfoRaw);
}
NW_HOOK_METHOD(0x100799d0, NConsoleWnd, AddRecipeBookItem)

// 0x10070840 (vslot 129; ret 4) - the make info of a recipe: the packet's five values (the recipe's index, then four numbers); the
// book (+0x198) hides, the manufacture window (+0x1A0) is reset and takes the recipe, keeps the numbers (+0x1D54, +0x1D58, +0x1D5C
// and +0x1D74) and takes the place of the book
void NConsoleWnd::ReceiveRecipeItemMakeInfo(L2ParamStack& Params)
{
    INT P1 = (INT)Params.Top();
    INT P2 = (INT)Params.Top();
    INT P3 = (INT)Params.Top();
    INT P4 = (INT)Params.Top();
    INT P5 = (INT)Params.Top();
    BYTE* Recipe = (BYTE*)GL2GameData.GetRecipeDataByIndex(P1);
    if (!Recipe)
        return;
    DeskWnd(this, 0x198)->HideWindow();
    static_cast<NCRecipeManufactureWnd*>(DeskWnd(this, 0x1A0))->Reset();
    static_cast<NCRecipeManufactureWnd*>(DeskWnd(this, 0x1A0))->SetRecipe((NRecipeData*)Recipe);
    nw::Field<INT>(DeskWnd(this, 0x1A0), 0x1D58) = P3;
    nw::Field<INT>(DeskWnd(this, 0x1A0), 0x1D54) = P2;
    nw::Field<INT>(DeskWnd(this, 0x1A0), 0x1D5C) = P4;
    ((NRecipeManufactureWndExt*)DeskWnd(this, 0x1A0))->SetField1D74(P5);
    PlaceAtAndShow(DeskWnd(this, 0x1A0), DeskWnd(this, 0x198));
}
NW_HOOK_METHOD(0x10070840, NConsoleWnd, ReceiveRecipeItemMakeInfo)

// 0x100709b0 (vslot 130; ret 0xC) - the shop's manage list starts over with its kind (P1) and two numbers (P2, P3 at +0x114, +0x118),
// then shows and takes the focus; the shop window is the desktop's +0x1A4
void NConsoleWnd::ReceiveRecipeShopManageList(INT P1, INT P2, INT P3)
{
    ((NRecipeShopWndKind*)DeskWnd(this, 0x1A4))->SetKind(P1);
    static_cast<NCRecipeShopWnd*>(DeskWnd(this, 0x1A4))->ClearLists();
    nw::Field<INT>(DeskWnd(this, 0x1A4), 0x114) = P2;
    nw::Field<INT>(DeskWnd(this, 0x1A4), 0x118) = P3;
    DeskWnd(this, 0x1A4)->ShowWindow();
    DeskWnd(this, 0x1A4)->SetFocus();
}
NW_HOOK_METHOD(0x100709b0, NConsoleWnd, ReceiveRecipeShopManageList)

// 0x10079b00 (vslot 131) - a recipe the shop can sell (the second number is kept in the entry)
void NConsoleWnd::AddRecipeShopManageItem(INT RecipeID, INT P2)
{
    BYTE* Recipe = (BYTE*)GL2GameData.GetRecipeDataByIndex(RecipeID);
    if (!Recipe)
        return;
    NItemInfo InfoRaw;
    NConsoleItemInfoC4& Info = reinterpret_cast<NConsoleItemInfoC4&>(InfoRaw);
    FillRecipeItemInfo(Info, Recipe);
    Info.UnknownC90 = P2;
    DeskRecipeShop(this)->AddRecipeItem(&InfoRaw);
}
NW_HOOK_METHOD(0x10079b00, NConsoleWnd, AddRecipeShopManageItem)

// 0x10079c40 (vslot 132) - a recipe the shop has registered for sale
void NConsoleWnd::AddRecipeShopRegisterdItem(INT RecipeID, INT P2, INT P3)
{
    BYTE* Recipe = (BYTE*)GL2GameData.GetRecipeDataByIndex(RecipeID);
    if (!Recipe)
        return;
    NItemInfo InfoRaw;
    NConsoleItemInfoC4& Info = reinterpret_cast<NConsoleItemInfoC4&>(InfoRaw);
    FillRecipeItemInfo(Info, Recipe);
    Info.UnknownC1C = P3;
    Info.UnknownC90 = P2;
    DeskRecipeShop(this)->AddProductionItem(&InfoRaw);
}
NW_HOOK_METHOD(0x10079c40, NConsoleWnd, AddRecipeShopRegisterdItem)

// 0x10059270 (vslot 129) - the list of recipes a shop sells starts over: the window at +0x144 (0x100c11b0,
// 0x10070a30 (vslot 133) - the list of recipes a shop sells starts over: the buy list window (+0x1A8) is emptied and takes the owner
// (+0xFC), two numbers (+0x100, +0x104) and the fourth (+0x108), over the buy manufacture window (+0x1AC)
void NConsoleWnd::ReceiveRecipeShopSellList(INT P1, INT P2, INT P3, INT P4)
{
    ((NRecipeBuyListWndExt*)DeskWnd(this, 0x1A8))->ClearList();
    static_cast<NCRecipeBuyListWnd*>(DeskWnd(this, 0x1A8))->SetShopOwner(P1);
    nw::Field<INT>(DeskWnd(this, 0x1A8), 0x100) = P2;
    nw::Field<INT>(DeskWnd(this, 0x1A8), 0x104) = P3;
    ((NSetField108*)DeskWnd(this, 0x1A8))->Set(P4);
    PlaceAtAndShow(DeskWnd(this, 0x1A8), DeskWnd(this, 0x1AC));
}
NW_HOOK_METHOD(0x10070a30, NConsoleWnd, ReceiveRecipeShopSellList)

// 0x10079d90 (vslot 134) - a recipe the shop sells, with its price and the player's count
void NConsoleWnd::AddRecipeShopSellItem(INT RecipeID, INT P2, INT P3)
{
    BYTE* Recipe = (BYTE*)GL2GameData.GetRecipeDataByIndex(RecipeID);
    if (!Recipe)
        return;
    NRecipeSellEntry Entry;
    Entry.ItemID = *(INT*)(Recipe + 0x20);
    Entry.P3 = P3;
    Entry.Unknown0C = *(INT*)(Recipe + 0x34);
    Entry.Name[0] = 0;
    Entry.IconName[0] = 0;
    Entry.Description[0] = 0;
    Entry.Unknown00 = 0;
    Entry.CrystalGrade = 0;
    Entry.Unknown1C = -1;
    Entry.RecipeID = RecipeID;
    Entry.P2 = P2;
    NViewItemDataC4* Product = (NViewItemDataC4*)NItemTable::Find((INT*)(Recipe + 0x28));
    if (Product)
    {
        appStrcpy(Entry.Name, *Product->Name);
        appStrcpy(Entry.Description, *Product->Description);
        appStrcpy(Entry.IconName, *Product->IconName);
        if (Product->Class == ITEMCLASS_C4_Weapon)
            Entry.CrystalGrade = Product->CrystalGrade;
        else if (Product->Class == ITEMCLASS_C4_Armor)
            Entry.CrystalGrade = Product->ArmorCrystalGrade;
    }
    DeskRecipeBuyList(this)->AddRecipe((const NRecipeRow*)&Entry);   // NRecipeSellEntry has the layout of NRecipeRow
}
NW_HOOK_METHOD(0x10079d90, NConsoleWnd, AddRecipeShopSellItem)

// 0x10064290 (vslot 134) - the info of a recipe in a shop (the recipe is the second value, by its index): the buy
// manufacture window (+0x180) shows it (SetRecipe, 0x100E0FF0) with the other values (+0x1B30, +0x1B2C, +0x1B28,
// +0x1B38) over the buy list (+0x17C), which hides. C3: a sixth value (ret 0x18) goes to SetRecipe as its second
// argument (the stub of the header has one). C4: the slot has the six values (include/ue2/gen/UL2ConsoleWnd.vt.inc was the C2's,
// with five: the stock pops 0x18). Hook mode: the body is the non-virtual ReceiveRecipeShopItemInfoX hooked at the slot's address
// and the slot's own stub jumps to it. Standalone: the slot IS ReceiveRecipeShopItemInfoX (the stock slot holds 0x10070b50 itself)
#if NW_HOOK_MODE
NW_NAKED void NConsoleWnd::ReceiveRecipeShopItemInfo(INT P1, INT P2, INT P3, INT P4, INT P5, INT P6) NW_STOCK(0x10070B50)
#else
NW_SLOT_IS("?ReceiveRecipeShopItemInfo@NConsoleWnd@@UAEXHHHHHH@Z", "?ReceiveRecipeShopItemInfoX@NConsoleWnd@@QAEXHHHHHH@Z")   // NConsoleWnd::ReceiveRecipeShopItemInfo = ...X (0x10070b50)
#endif

// 0x10070b50 (vslot 134) - the info of a recipe in a shop (the recipe is the second value, by its index): the buy manufacture window
// (+0x1AC) shows it (SetRecipe, 0x100F2220) with the other values (+0x1D5C, +0x1D58, +0x1D54, +0x1D64) over the buy list (+0x1A8),
// which is emptied and hides. A sixth value (ret 0x18) goes to SetRecipe as its second argument (the stub of the header has one); the
// base's slot has five (include/ue2 is the C2's), so the body is the non-virtual ReceiveRecipeShopItemInfoX hooked at the slot's address
// and the slot's own stub jumps to it
void NConsoleWnd::ReceiveRecipeShopItemInfoX(INT P1, INT P2, INT P3, INT P4, INT P5, INT P6)
{
    BYTE* Recipe = (BYTE*)GL2GameData.GetRecipeDataByIndex(P2);
    if (!Recipe)
        return;
    ((NRecipeBuyListWndExt*)DeskWnd(this, 0x1A8))->ClearList();
    DeskWnd(this, 0x1A8)->HideWindow();
    static_cast<NCRecipeBuyManufactureWnd*>(DeskWnd(this, 0x1AC))->Reset();
    nw::Field<INT>(DeskWnd(this, 0x1AC), 0x1D5C) = P1;
    nw::Field<INT>(DeskWnd(this, 0x1AC), 0x1D58) = P4;
    nw::Field<INT>(DeskWnd(this, 0x1AC), 0x1D54) = P3;
    nw::Field<INT>(DeskWnd(this, 0x1AC), 0x1D64) = P5;
    static_cast<NCRecipeBuyManufactureWnd*>(DeskWnd(this, 0x1AC))->SetRecipe((NRecipeData*)Recipe, P6);
    PlaceAtAndShow(DeskWnd(this, 0x1AC), DeskWnd(this, 0x1A8));
}
NW_HOOK_METHOD(0x10070b50, NConsoleWnd, ReceiveRecipeShopItemInfoX)

/*-----------------------------------------------------------------------------
    Macros: the macro window (+0xF8, the list of macros at its +0xC4 and the 24 records it keeps from +0xD4,
    0x3FC bytes each) and what feeds it.
-----------------------------------------------------------------------------*/

// 0x10079ed0 (vslot 136) - one macro of the list the server sends (P1 macros in all: C1 had none of this).
// While the list is being received (Unknown357C is set) the first macro empties the window's list (every macro
// object deleted, the 24 records cleared); each macro is copied to the window's record of its number and
// handed to a new macro object (0x1008f840, filled by 0x1008b380 and added by AddMacro); Unknown3580 counts
// them, and the last one (or P1 = P2 = 0) ends with UpdateMacroShortCut(P5, P4)
void NConsoleWnd::ReceiveMacroListInfo(char P1, char P2, MacroInfo Macro, char P4, INT P5)
{
    if (Unknown357C)
    {
        Unknown3580 = 0;
        if (Desktop && GetDesktop()->NewWndF8)
        {
            // C4: the macro window is the desktop's +0x150 (C3: +0x12C) and its slots start at +0x13C (C3: +0x11C),
            // the five cleared fields of a slot sit 0x60, 4, 0x1E and 0x76 into it (the stock's +0x19C, +0x140,
            // +0x15A, +0x1B2); the numbers of the C3 body wrote over other objects of the heap
            BYTE* SlotBase = (BYTE*)DeskMacroList(this)->Slots;
            INT Count = GetMacroCount();
            for (; Count > 0; Count--)
            {
                INT Last = GetMacroCount() - 1;
                if (GetMacro(Last))
                {
                    NMacroEntry* Old = GetMacro(Last);
                    if (Old)
                    {
                        Old->~NMacroEntry();
                        GMalloc->Free(Old);
                    }
                }
                DeskMacroList(this)->Macros.Remove(Last, 1);                         // TPtrArray::Remove (0x1001D1B0)
            }
            for (INT i = 0; i < 0x5FA0; i += 0x3FC)
            {
                appMemset(SlotBase + i + 0x60, 0, 10);
                appMemset(SlotBase + i + 0x04, 0, 0x1A);
                appMemset(SlotBase + i + 0x1E, 0, 0x42);
                for (INT k = 0; k < 0x390; k += 0x4C)
                    appMemset(SlotBase + i + 0x76 + k, 0, 0x42);
            }
        }
        if (!P1 && !P2)
        {
            UpdateMacroShortCut(P5, P4);
            return;
        }
        Unknown357C = 0;
    }
    void* Mem = GMalloc->Malloc(0xAB4, TEXT("new"));
    NMacroEntry* Object = Mem ? ::new (Mem) NMacroEntry : NULL;                      // the macro object's constructor (0x1008F840)
    if (Object)
    {
        appMemcpy((BYTE*)DeskMacroList(this)->Slots + GetMacroCount() * 0x3FC, &Macro, 0x3FC);   // the stock: window +0x13C + count * 0x3FC
        NMacroSlot* Slot = (NMacroSlot*)&Macro;                                      // the slot by value: the id, then the data
        Object->Set(Slot->ID, Slot->Info);
        AddMacro(Object);
    }
    Unknown3580++;
    if (Unknown3580 < (INT)P1)
        return;
    Unknown357C = 1;
    UpdateMacroShortCut(P5, P4);
}
NW_HOOK_METHOD(0x10079ed0, NConsoleWnd, ReceiveMacroListInfo)

// 0x100682b0 (vslot 159) - the end of the macro list (and of its shortcuts): the shortcut bar's window takes it
// (NCShortCutWnd::UpdateMacro)
void NConsoleWnd::UpdateMacroShortCut(INT P1, INT P2)
{
    NCShortCutWnd* ShortCut = GetDesktop()->ShortCutWnd;
    if (ShortCut)
        ShortCut->UpdateMacro(P1, P2);
}
NW_HOOK_METHOD(0x100682b0, NConsoleWnd, UpdateMacroShortCut)

// The macro runner (the macro window, +0xF8) is told what happens: its running macros (the list at +0xC4) that
// wait for the event (type 3 at their step) are let go, by a delay in the step (+0x90) of one second or a
// thousandth
// 0x10068420 (vslot 247) - a skill was used (1, 0) or cancelled (0, 1): 0x1008c450
void NConsoleWnd::MacroSkillUse(INT P1, INT P2)
{
    DeskMacroList(this)->OnSkillCoolTime(P1, P2);
}
NW_HOOK_METHOD(0x10068420, NConsoleWnd, MacroSkillUse)

// 0x10068460 (vslot 228) - the target changed: the macro window's OnTargetDone, then (C3) the broadcast observer window
// (+0x1B4) sets its target up when the desktop has one (a tail call)
void NConsoleWnd::MacroTargetSelected()
{
    static_cast<NCMacroListWnd*>(DeskWnd(this, 0x150))->OnTargetDone();   // (kept out of line, as the stock calls it)
    NCConsole* Console = static_cast<NCConsole*>(Desktop);
    if (Console && Console->BroadcastObserverWnd)
        static_cast<NCBroadcastObserverWnd*>(Console->BroadcastObserverWnd)->SetUpTarget();
}
NW_HOOK_METHOD(0x10068460, NConsoleWnd, MacroTargetSelected)

// 0x10068440 (vslot 249) - an item was picked up: 0x1008c330
void NConsoleWnd::MacroItemPickedUp()
{
    DeskMacroList(this)->OnPickUpDone();
}
NW_HOOK_METHOD(0x10068440, NConsoleWnd, MacroItemPickedUp)

/*-----------------------------------------------------------------------------
    Henna: the dye lists (+0x14C the list, +0x150 the info of one), the inventory (+0xCC), the GM's views.
-----------------------------------------------------------------------------*/

// 0x10070cc0 (vslot 136) - the list of dyes the player can engrave starts over (P1 of them; the engrave flag +0xD4 is
// 0): the list window (+0x184) takes the place of the info window (+0x188)
void NConsoleWnd::ReceiveHennaEquipList(INT P1, INT P2)
{
    DeskHennaList(this)->Clear();
    nw::Field<INT>(DeskWnd(this, 0x1B0), 0xF8) = P1;
    nw::Field<INT>(DeskWnd(this, 0x1B0), 0xF4) = 0;
    NRectF Local;
    Local.X = 0;
    Local.Y = 0;
    Local.W = 0;
    Local.H = 0;
    INT X = DeskWnd(this, 0x1B4)->ScreenX;
    INT Y = DeskWnd(this, 0x1B4)->ScreenY;
    DeskWnd(this, 0x1B4)->HideWindow();
    DeskWnd(this, 0x1B0)->GetLocalRect(&Local);
    DeskWnd(this, 0x1B0)->SetPosition(X, Y, (DWORD)(INT)Local.W, (DWORD)(INT)Local.H);
    DeskWnd(this, 0x1B0)->ShowWindow();
    DeskWnd(this, 0x1B0)->SetFocus();
}
NW_HOOK_METHOD(0x10070cc0, NConsoleWnd, ReceiveHennaEquipList)

// 0x10071070 (vslot 141) - the same for the dyes to remove (+0xD4 is 1)
void NConsoleWnd::ReceiveHennaUnequipList(INT P1, INT P2)
{
    DeskHennaList(this)->Clear();
    nw::Field<INT>(DeskWnd(this, 0x1B0), 0xF8) = P1;
    nw::Field<INT>(DeskWnd(this, 0x1B0), 0xF4) = 1;
    NRectF Local;
    Local.X = 0;
    Local.Y = 0;
    Local.W = 0;
    Local.H = 0;
    INT X = DeskWnd(this, 0x1B4)->ScreenX;
    INT Y = DeskWnd(this, 0x1B4)->ScreenY;
    DeskWnd(this, 0x1B4)->HideWindow();
    DeskWnd(this, 0x1B0)->GetLocalRect(&Local);
    DeskWnd(this, 0x1B0)->SetPosition(X, Y, (DWORD)(INT)Local.W, (DWORD)(INT)Local.H);
    DeskWnd(this, 0x1B0)->ShowWindow();
    DeskWnd(this, 0x1B0)->SetFocus();
}
NW_HOOK_METHOD(0x10071070, NConsoleWnd, ReceiveHennaUnequipList)

// What both lists take for a dye: the names and the icon (from the item data, or the henna data) and the
// five numbers of the HennaInfo
static FORCEINLINE void ClearHennaEntry(NHennaSendEntry& Entry)
{
    Entry.Name[0] = 0;
    Entry.IconName[0] = 0;
    Entry.Description[0] = 0;
    Entry.Unknown1400 = 0;
    Entry.Info0 = 0;
    Entry.ItemID = 0;
    Entry.Info8 = 0;
    Entry.InfoC = 0;
    Entry.Info10 = 0;
    Entry.Unknown1418 = 0;
}

static FORCEINLINE void SetHennaEntryNumbers(NHennaSendEntry& Entry, const HennaInfo& Info)
{
    Entry.Info0 = Info.HennaID;
    Entry.Info8 = Info.Param08;
    Entry.InfoC = Info.Param0C;
    Entry.ItemID = Info.ItemID;
    Entry.Info10 = Info.Param10;
}

// 0x10070dd0 (vslot 134) - a dye to engrave (the list window +0x14C adds it, 0x1007aef0): its item's names
void NConsoleWnd::AddHennaEquipInfo(HennaInfo Info)
{
    BYTE* Item = (BYTE*)GL2GameData.GetItemData(Info.ItemID);
    if (!Item)
        return;
    NHennaSendEntry Entry;
    ClearHennaEntry(Entry);
    appStrcpy(Entry.Name, *nw::Field<L2FName>(Item, 0x50));
    appStrcpy(Entry.Description, *nw::Field<L2FName>(Item, 0x54));
    appStrcpy(Entry.IconName, *nw::Field<L2FName>(Item, 0x3C));
    SetHennaEntryNumbers(Entry, Info);
    DeskHennaList(this)->AddEntry((const NHennaListEntry*)&Entry);   // NHennaSendEntry has the layout of NHennaListEntry
}
NW_HOOK_METHOD(0x10070dd0, NConsoleWnd, AddHennaEquipInfo)

// 0x10071180 (vslot 143) - a dye to remove: its henna data's texts (FStrings at +8, +0x14 and +0x2C)
void NConsoleWnd::AddHennaUnequipInfo(HennaInfo Info)
{
    BYTE* Henna = (BYTE*)GL2GameData.GetHennaData(Info.HennaID);
    if (!Henna)
        return;
    NHennaSendEntry Entry;
    ClearHennaEntry(Entry);
    appStrcpy(Entry.Name, **(FString*)(Henna + 0x8));
    appStrcpy(Entry.Description, **(FString*)(Henna + 0x2C));
    appStrcpy(Entry.IconName, **(FString*)(Henna + 0x14));
    SetHennaEntryNumbers(Entry, Info);
    DeskHennaList(this)->AddEntry((const NHennaListEntry*)&Entry);   // NHennaSendEntry has the layout of NHennaListEntry
}
NW_HOOK_METHOD(0x10071180, NConsoleWnd, AddHennaUnequipInfo)

// 0x10070ef0 (vslot 138) - the info of a dye to engrave: the list window hides, the info window (+0x188) takes the dye
// with P2 (+0x39A4) and the engrave flag (+0x39A0 is 0), in the place of the list window
void NConsoleWnd::ReceiveHennaItemInfo(HennaInfo Info, INT P2)
{
    DeskHennaList(this)->Clear();
    DeskWnd(this, 0x1B0)->HideWindow();
    DeskHennaInfo(this)->ClearParams();
    nw::Field<INT>(DeskWnd(this, 0x1B4), 0x39A4) = P2;
    nw::Field<INT>(DeskWnd(this, 0x1B4), 0x39A0) = 0;
    DeskHennaInfo(this)->SetHenna((const NHennaParam*)&Info);   // HennaInfo has the layout of NHennaParam
    PlaceAtAndShow(DeskWnd(this, 0x1B4), DeskWnd(this, 0x1B0));
}
NW_HOOK_METHOD(0x10070ef0, NConsoleWnd, ReceiveHennaItemInfo)

// 0x100712e0 (vslot 143) - the same for a dye to remove (+0x39A0 is 1)
void NConsoleWnd::ReceiveHennaUnequipItemInfo(HennaInfo Info, INT P2)
{
    DeskHennaList(this)->Clear();
    DeskWnd(this, 0x1B0)->HideWindow();
    DeskHennaInfo(this)->ClearParams();
    nw::Field<INT>(DeskWnd(this, 0x1B4), 0x39A4) = P2;
    nw::Field<INT>(DeskWnd(this, 0x1B4), 0x39A0) = 1;
    DeskHennaInfo(this)->SetHenna((const NHennaParam*)&Info);   // HennaInfo has the layout of NHennaParam
    PlaceAtAndShow(DeskWnd(this, 0x1B4), DeskWnd(this, 0x1B0));
}
NW_HOOK_METHOD(0x100712e0, NConsoleWnd, ReceiveHennaUnequipItemInfo)

// 0x10071030 (vslot 136) - the player's dyes: the inventory window (+0xCC) takes them over (0x100807a0) and the
// character window's child at +0x98 keeps the record (+0xCC)
void NConsoleWnd::ReceiveHennaInfo(HennaInfo Info, INT P2)
{
    GetDesktop()->InventoryWnd->ClearHennas();
    nw::Field<HennaInfo>(nw::Field<NCWnd*>(DeskWnd(this, 0xE8), 0xE8), 0x12C) = Info;
}
NW_HOOK_METHOD(0x10071030, NConsoleWnd, ReceiveHennaInfo)

// 0x10068150 (vslot 141) - the inventory window's call (0x10080250)
void NConsoleWnd::AddHennaInfo(INT P1, INT P2)
{
    GetDesktop()->InventoryWnd->AddHenna(P1, P2);
}
NW_HOOK_METHOD(0x10068150, NConsoleWnd, AddHennaInfo)

// 0x10068170 (vslot 147) - the GM's inventory window (+0x158): 0x10073020
void NConsoleWnd::GMAddHennaInfo(INT P1, INT P2)
{
    GetDesktop()->GMInventoryWnd->AddHenna(P1, P2);
}
NW_HOOK_METHOD(0x10068170, NConsoleWnd, GMAddHennaInfo)

// 0x100712a0 (vslot 144) - the GM's view of the player's dyes: the inventory window takes them over
// (0x10073030) and the GM's detail window (+0x160) keeps the record (+0xCC)
void NConsoleWnd::GMReceiveHennaInfo(HennaInfo Info, INT P2)
{
    GetDesktop()->GMInventoryWnd->ClearHennas();
    nw::Field<HennaInfo>(DeskWnd(this, 0x1C8), 0x12C) = Info;
}
NW_HOOK_METHOD(0x100712a0, NConsoleWnd, GMReceiveHennaInfo)

/*-----------------------------------------------------------------------------
    The calculator, the party, the shortcut bar's pad, the snoop, the town map, the observer, the Christmas
    seal, the quit dialog.
-----------------------------------------------------------------------------*/

// 0x10069840 (vslot 273) - the calculator: code 0x1129 shows its window (+0xCC) and the one inside it, which takes
// the focus (C3; C2 only showed them)
void NConsoleWnd::ShowCalc(INT Code)
{
    if (Code == 0x1129)
    {
        DeskWnd(this, 0xEC)->ShowWindow();
        nw::Field<NCWnd*>(DeskWnd(this, 0xEC), 0xEC)->ShowWindow();
        nw::Field<NCWnd*>(DeskWnd(this, 0xEC), 0xEC)->SetFocus();
    }
}
NW_HOOK_METHOD(0x10069840, NConsoleWnd, ShowCalc)

// 0x10065ac0 (vslot 303) - a number the party window keeps (+0xA8), 0 without a desktop or a party window
INT NConsoleWnd::GetPartyMemberCount()
{
    if (Desktop)
    {
        NCWnd* Party = DeskWnd(this, 0x114);                                           // C3: NCConsole::PartyWnd
        if (Party)
            return nw::Field<INT>(Party, 0xF4);                                       // NCPartyWnd.h puts NumMembers at 0xD8: the stock reads +0xD4
    }
    return 0;
}
NW_HOOK_METHOD(0x10065ac0, NConsoleWnd, GetPartyMemberCount)

// 0x1006c080 (vslot 354) - the shortcut bar's pad, left half held or not (C1's SetPadShift1)
void NConsoleWnd::SetPadHoldL(BYTE bHold)
{
    GetDesktop()->ShortCutWnd->SetPadHoldL(bHold != 0);
}
NW_HOOK_METHOD(0x1006c080, NConsoleWnd, SetPadHoldL)

// 0x1006c0a0 (vslot 355) - the right half (C1's SetPadShift2)
void NConsoleWnd::SetPadHoldR(BYTE bHold)
{
    GetDesktop()->ShortCutWnd->SetPadHoldR(bHold != 0);
}
NW_HOOK_METHOD(0x1006c0a0, NConsoleWnd, SetPadHoldR)

// 0x10069bf0 (vslot 306) - a line a GM snoops (a player's chat): the snoop id and the name of the snooped player go
// to the snoop manager (+0x190, OpenSnoop 0x100b7550), and the line "name: text" in the color of its chat type
// (AddLine 0x100b74f0: the snoop id comes first, five arguments), unless the type is none of the known ones
void NConsoleWnd::Snoop(L2ParamStack& Params)
{
    INT Id = (INT)Params.Top();
    TCHAR* TargetName = (TCHAR*)Params.Top();
    Params.Top();
    INT Type = (INT)Params.Top();
    TCHAR* Name = (TCHAR*)Params.Top();
    TCHAR* Text = (TCHAR*)Params.Top();
    if (!Text)
        return;
    NCGMSnoopManager* Snoops = (NCGMSnoopManager*)DeskWnd(this, 0x248);              // C3: NCConsole::GMSnoopManager
    Snoops->OpenSnoop(Id, TargetName);
    TCHAR Line[1024];
    _swprintf(Line, TEXT("%s: %s"), Name, Text);
    DWORD Color;
    if (Type == 1)
        Color = 0xFFFF7200;
    else if (Type == 3)
        Color = 0xFF00FF00;
    else if (Type == 4)
        Color = 0xFF7D77FF;
    else if (Type == 2)
        Color = 0xFFFF00FF;
    else if (Type == 8)
        Color = 0xFFEAA5F5;
    else if (Type == 9)
        Color = 0xFF77FF99;
    else if (Type == 0)
        Color = 0xFFDCDCDC;
    else
        return;
    Snoops->AddLine(Id, Line, Color, Type, 0);
}
NW_HOOK_METHOD(0x10069bf0, NConsoleWnd, Snoop)

// 0x1006c0c0 (vslot 335) - the town map: three values, taken even when there is no window (+0x1E4) or the first is 0; the
// window takes them (NTownMapWnd::SetMap), shows and takes the focus (C3)
void NConsoleWnd::ShowTownMap(L2ParamStack& Params)
{
    void* P1 = Params.Top();
    void* P2 = Params.Top();
    void* P3 = Params.Top();
    NCWnd* Map = DeskWnd(this, 0x24C);
    if (Map && P1)
    {
        static_cast<NTownMapWnd*>(Map)->SetMap((const TCHAR*)P1, (INT)P2, (INT)P3);
        DeskWnd(this, 0x24C)->ShowWindow();
        DeskWnd(this, 0x24C)->SetFocus();
    }
}
NW_HOOK_METHOD(0x1006c0c0, NConsoleWnd, ShowTownMap)

// 0x1006c130 (guard name ReceiveObserverModeOnPacket; vslot 358) - the observer window (the desktop's +0x250) is shown with the focus;
// the console first keeps the value of GIsL2Mark and clears it (SaveL2Mark)
void NConsoleWnd::ObserverStart()
{
    guard(NConsoleWnd::ReceiveObserverModeOnPacket);
    if (nw::Field<NCWnd*>(Desktop, 0x250))
    {
        SaveL2Mark(GIsL2Mark);
        nw::Field<NCWnd*>(Desktop, 0x250)->ShowWindow();
        nw::Field<NCWnd*>(Desktop, 0x250)->SetFocus();
    }
    unguard;
}
NW_HOOK_METHOD(0x1006c130, NConsoleWnd, ObserverStart)

// 0x1006c1f0 (guard name ReceiveObserverModeOffPacket; vslot 359) - and hidden, GIsL2Mark given back
void NConsoleWnd::ObserverEnd()
{
    guard(NConsoleWnd::ReceiveObserverModeOffPacket);
    if (nw::Field<NCWnd*>(Desktop, 0x250))
    {
        RestoreL2Mark();
        nw::Field<NCWnd*>(Desktop, 0x250)->HideWindow();
    }
    unguard;
}
NW_HOOK_METHOD(0x1006c1f0, NConsoleWnd, ObserverEnd)

// 0x1006c2c0 (vslot 313) - the pledge's power: the window at +0xA0 takes the three values (0x100721f0)
void NConsoleWnd::PledgePower(INT P1, INT P2, BYTE* P3)
{
    if (Desktop && DeskWnd(this, 0xF0))                                              // C3: NCConsole::EmpowerWnd
        static_cast<NCEmpowerWnd*>(DeskWnd(this, 0xF0))->SetPower(P1, P2, P3);
}
NW_HOOK_METHOD(0x1006c2c0, NConsoleWnd, PledgePower)

// 0x10074d60 (vslot 316) - the window was closed in the game: the dialog 0x1C asks (system message 0x7D) to
// quit, answered to the system menu; the global console's ShowDialog opens it (the stock calls it by the
// global, not by this)
void NConsoleWnd::ShowQuitDialog()
{
    NDialogInfo Info;
    Info.ID = 0x1C;
    Info.bMainBox = 1;
    Info.bInput = 0;
    Info.Owner = GetDesktop()->SystemMenuWnd->Entry;                                        // C3: NCConsole::SystemMenuWnd
    Info.SetSystemMessage(0x7D);
    GConsole->ShowDialog(&Info);
}
NW_HOOK_METHOD(0x10074d60, NConsoleWnd, ShowQuitDialog)

// 0x10001ad0 (vslot 318) - empty (the shared empty function)
// (declared inline in the header)

// 0x1006c500 (vslot 319) - the Christmas seal: the window at +0x19C takes the value (0x100dfb40) and the desktop
// shows or hides it (0x1004c440)
void NConsoleWnd::ShowXMasSeal(INT P1)
{
    static_cast<NCXMasSealWnd*>(DeskWnd(this, 0x254))->SetItem(P1);                 // C4: NCConsole +0x254 (C3 +0x1EC)
    static_cast<NCConsole*>(Desktop)->ToggleXMasSeal();
}
NW_HOOK_METHOD(0x1006c500, NConsoleWnd, ShowXMasSeal)

/*-----------------------------------------------------------------------------
    C4 only: the handlers of the manor, hero and Olympiad packets (UGameEngine::On* calls them through the engine's
    unnamed slots 212 .. 227; the names are the stock guard strings). Each is a non-virtual method hooked at the slot's stock
    function (the slot itself is NewVirtualN in NNewVirtuals.gen.cpp).
-----------------------------------------------------------------------------*/

// 0x100678c0 (vslot 212, C4 only) - the manor info window (NCManorInfoWnd) takes the packet (0x10137730 pops its values) when it exists
void NConsoleWnd::OnShowSeedInfo(L2ParamStack& Params)
{
    guard(NConsoleWnd::OnShowSeedInfo);
    if (Desktop)
    {
        NCWnd* Wnd = DeskWnd(this, 0x1EC);
        if (Wnd)
            static_cast<NCManorInfoWnd*>(Wnd)->OnShowSeedInfo(&Params);
    }
    unguard;
}
NW_HOOK_METHOD(0x100678c0, NConsoleWnd, OnShowSeedInfo)

// 0x10067950 (vslot 213, C4 only) - the manor info window takes the packet (0x10137800 pops its values) when it exists
void NConsoleWnd::OnShowCropInfo(L2ParamStack& Params)
{
    guard(NConsoleWnd::OnShowCropInfo);
    if (Desktop)
    {
        NCWnd* Wnd = DeskWnd(this, 0x1EC);
        if (Wnd)
            static_cast<NCManorInfoWnd*>(Wnd)->OnShowCropInfo(&Params);
    }
    unguard;
}
NW_HOOK_METHOD(0x10067950, NConsoleWnd, OnShowCropInfo)

// 0x100679e0 (vslot 214, C4 only) - the manor info window takes the packet (0x101378C0 pops its values) when it exists
void NConsoleWnd::OnShowManorDefaultInfo(L2ParamStack& Params)
{
    guard(NConsoleWnd::OnShowManorDefaultInfo);
    if (Desktop)
    {
        NCWnd* Wnd = DeskWnd(this, 0x1EC);
        if (Wnd)
            static_cast<NCManorInfoWnd*>(Wnd)->OnShowManorDefaultInfo(&Params);
    }
    unguard;
}
NW_HOOK_METHOD(0x100679e0, NConsoleWnd, OnShowManorDefaultInfo)

// 0x10067a70 (vslot 215, C4 only) - the seed setting window (NCManorSeedInfoSettingWnd) takes the packet (0x10139D40 pops its values) when it exists
void NConsoleWnd::OnShowSeedSetting(L2ParamStack& Params)
{
    guard(NConsoleWnd::OnShowSeedSetting);
    if (Desktop)
    {
        NCWnd* Wnd = DeskWnd(this, 0x1F0);
        if (Wnd)
            static_cast<NCManorSeedInfoSettingWnd*>(Wnd)->OnShowSeedSetting(&Params);
    }
    unguard;
}
NW_HOOK_METHOD(0x10067a70, NConsoleWnd, OnShowSeedSetting)

// 0x10067b00 (vslot 216, C4 only) - the crop setting window (NCManorCropInfoSettingWnd) takes the packet (0x10131AD0 pops its values) when it exists
void NConsoleWnd::OnShowCropSetting(L2ParamStack& Params)
{
    guard(NConsoleWnd::OnShowCropSetting);
    if (Desktop)
    {
        NCWnd* Wnd = DeskWnd(this, 0x1F4);
        if (Wnd)
            static_cast<NCManorCropInfoSettingWnd*>(Wnd)->OnShowCropSetting(&Params);
    }
    unguard;
}
NW_HOOK_METHOD(0x10067b00, NConsoleWnd, OnShowCropSetting)

// 0x10067b90 (vslot 217, C4 only) - the crop sell window (NCManorCropSellWnd) takes the packet (0x101349E0 pops its values) when it exists
void NConsoleWnd::OnShowSellCropList(L2ParamStack& Params)
{
    guard(NConsoleWnd::OnShowSellCropList);
    if (Desktop)
    {
        NCWnd* Wnd = DeskWnd(this, 0x1F8);
        if (Wnd)
            static_cast<NCManorCropSellWnd*>(Wnd)->OnShowSellCropList(&Params);
    }
    unguard;
}
NW_HOOK_METHOD(0x10067b90, NConsoleWnd, OnShowSellCropList)

// 0x10067c20 (vslot 218, C4 only) - the crop sell change window (NCManorCropSellChangeWnd) takes the packet (0x10133720 pops its values) when it exists
void NConsoleWnd::OnShowProcureCropDetail(L2ParamStack& Params)
{
    guard(NConsoleWnd::OnShowProcureCropDetail);
    if (Desktop)
    {
        NCWnd* Wnd = DeskWnd(this, 0x1FC);
        if (Wnd)
            static_cast<NCManorCropSellChangeWnd*>(Wnd)->OnShowProcureCropDetail(&Params);
    }
    unguard;
}
NW_HOOK_METHOD(0x10067c20, NConsoleWnd, OnShowProcureCropDetail)

// 0x10067cb0 (vslot 219, C4 only) - the hero tower window (NCHeroTowerWnd) takes the packet (0x1013BCB0 pops its values) when it exists
void NConsoleWnd::OnHeroList(L2ParamStack& Params)
{
    guard(NConsoleWnd::OnHeroList);
    if (Desktop)
    {
        NCWnd* Wnd = DeskWnd(this, 0x208);
        if (Wnd)
            static_cast<NCHeroTowerWnd*>(Wnd)->OnHeroList(Params);
    }
    unguard;
}
NW_HOOK_METHOD(0x10067cb0, NConsoleWnd, OnHeroList)
