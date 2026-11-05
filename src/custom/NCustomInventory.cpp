/*=============================================================================
    NCustomInventory.cpp - the wide inventory with type filters and sorting (custom, NW_CUSTOM builds only). See
    NCustomInventory.h.
=============================================================================*/
#include "nwindow/NCustomInventory.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCEquipItemWnd.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCInventoryWnd.h"
#include "nwindow/NCInvenItemWnd.h"
#include "nwindow/NCItemWnd.h"
#include "nwindow/NCScrollWnd.h"
#include "nwindow/NCSliderBar.h"
#include "nwindow/NCTabControl.h"
#include "nwindow/NCustomShortCuts.h"
#include "nwindow/NItemC4.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

using namespace NCustomInventory;

// the kinds an item is sorted by (in this order) and the filters
enum { KIND_Money, KIND_Weapon, KIND_Armor, KIND_Jewel, KIND_Consumable, KIND_Material, KIND_Other };
enum { FILTER_All, FILTER_Weapon, FILTER_Armor, FILTER_Jewel, FILTER_Consumable, FILTER_Material, FILTER_Other, Filters };

// the tabs over the grid: the type filters of the "Item" page, then the "Quest" page; the sort button under the grid
enum
{
    Tabs = Filters + 1,
    TAB_Quest = Filters,
    TabH = 23,
    TabPad = 14,                        // a tab is its label and this much
    NID_INV_TAB = (INT)0xFF400001,      // + the tab
    NID_INV_SORT = (INT)0xFF400010,
    SortX = GridX,
    SortY = LowerY + 6,
};
static const TCHAR* GTabLabels[Tabs] = {
    TEXT("Todo"), TEXT("Armas"), TEXT("Armaduras"), TEXT("Joyas"), TEXT("Consumibles"), TEXT("Materiales"), TEXT("Otros"),
    TEXT("Quest"),
};

static NCInventoryWnd* GInventory;
static NCButton* GTabButtons[Tabs];   // no faces: PaintBack draws the tabs; the buttons take the clicks
static INT GTabX[Tabs + 1];           // the tabs' left edges (window pixels) and the right edge of the last; 0: not laid out
static NCButton* GSortButton;
static INT GTab = 0;
static INT GFilter = FILTER_All;
static TArray<INT> GOrderBefore;   // the "Item" page's object ids when a filter was chosen

/*-----------------------------------------------------------------------------
    The kinds of the etc items: system\NWindowItems.txt.
-----------------------------------------------------------------------------*/

struct NEtcKind
{
    INT ID;
    INT Kind;
};
static NEtcKind* GEtcKinds;
static INT GEtcKindCount = -1;   // -1: not read yet

static int CompareEtcKind(const void* A, const void* B)
{
    INT X = ((const NEtcKind*)A)->ID, Y = ((const NEtcKind*)B)->ID;
    return X < Y ? -1 : X > Y;
}

static void ReadEtcKinds()
{
    GEtcKindCount = 0;
    TCHAR Path[MAX_PATH];
    GetModuleFileNameW(NULL, Path, MAX_PATH);
    TCHAR* Slash = wcsrchr(Path, '\\');
    if (!Slash)
        return;
    wcscpy_s(Slash + 1, MAX_PATH - (Slash + 1 - Path), TEXT("NWindowItems.txt"));
    FILE* File = NULL;
    if (_wfopen_s(&File, Path, L"r") || !File)
        return;
    INT Capacity = 0;
    char Line[64];
    while (fgets(Line, sizeof(Line), File))
    {
        int ID;
        char Letter;
        if (sscanf_s(Line, "%d %c", &ID, &Letter, 1) != 2)
            continue;
        INT Kind = Letter == 'C' ? KIND_Consumable : Letter == 'M' ? KIND_Material : -1;
        if (Kind < 0)
            continue;
        if (GEtcKindCount == Capacity)
        {
            Capacity = Capacity ? Capacity * 2 : 1024;
            GEtcKinds = (NEtcKind*)realloc(GEtcKinds, Capacity * sizeof(NEtcKind));
        }
        GEtcKinds[GEtcKindCount].ID = ID;
        GEtcKinds[GEtcKindCount].Kind = Kind;
        GEtcKindCount++;
    }
    fclose(File);
    qsort(GEtcKinds, GEtcKindCount, sizeof(NEtcKind), CompareEtcKind);
}

static INT KindOf(NItemEntry* Item)
{
    NItemEntryC4* E = AsC4(Item);
    switch (E->Type)
    {
    case 0: return KIND_Weapon;
    case 1: return KIND_Armor;
    case 2: return KIND_Jewel;
    case 4: return KIND_Money;
    }
    if (GEtcKindCount < 0)
        ReadEtcKinds();
    NEtcKind Key = { E->ID, 0 };
    NEtcKind* Found = GEtcKindCount ? (NEtcKind*)bsearch(&Key, GEtcKinds, GEtcKindCount, sizeof(NEtcKind), CompareEtcKind) : NULL;
    return Found ? Found->Kind : KIND_Other;
}

static UBOOL Matches(NItemEntry* Item)
{
    INT Kind = KindOf(Item);
    switch (GFilter)
    {
    case FILTER_Weapon: return Kind == KIND_Weapon;
    case FILTER_Armor: return Kind == KIND_Armor;
    case FILTER_Jewel: return Kind == KIND_Jewel;
    case FILTER_Consumable: return Kind == KIND_Consumable;
    case FILTER_Material: return Kind == KIND_Material;
    case FILTER_Other: return Kind == KIND_Other || Kind == KIND_Money;
    }
    return 1;
}

/*-----------------------------------------------------------------------------
    The pages: filtered, sorted, put back.
-----------------------------------------------------------------------------*/

static UBOOL IsFiltered(NCItemWnd* Page)
{
    return GInventory && Page == GInventory->ItemWnd[0] && GFilter != FILTER_All;
}

// the page's items in a new order (Order holds them all): the selection follows its item
static void Reorder(NCItemWnd* Page, NItemEntry** Order)
{
    NItemEntry* Selected = Page->Selected >= 0 && Page->Selected < Page->Items.Num() ? Page->Items(Page->Selected) : NULL;
    for (INT i = 0; i < Page->Items.Num(); i++)
    {
        Page->Items(i) = Order[i];
        if (Order[i] == Selected)
            Page->Selected = i;
    }
}

// rows for what shows; the view comes back when it is past them
static void SetRows(NCItemWnd* Page, INT Count)
{
    INT Rows = (Count + Page->Columns - 1) / Page->Columns;
    INT Top = Page->TopRow;
    if (Top > Rows - Page->VisibleRows)
        Top = Rows - Page->VisibleRows;
    if (Top < 0)
        Top = 0;
    if (Rows == Page->NumRows && Top == Page->TopRow)
        return;
    Page->NumRows = Rows;
    Page->TopRow = Top;
    Page->ScrollBar->SetRange(Rows, Page->VisibleRows);
    Page->ScrollBar->SetValue(Top);
}

INT NCustomInventory::Shown(NCItemWnd* Page)
{
    if (!IsFiltered(Page))
        return Page->Items.Num();
    INT Count = 0;
    for (INT i = 0; i < Page->Items.Num(); i++)
        if (Page->Items(i) && Matches(Page->Items(i)))
            Count++;
    return Count;
}

// the matching items first, each part in its order (not while one is carried: its index must stay)
void NCustomInventory::Arrange(NCItemWnd* Page)
{
    if (!IsFiltered(Page) || Page->bDragging)
        return;
    INT Count = Page->Items.Num();
    INT FirstOther = -1;
    UBOOL bNeeded = 0;
    for (INT i = 0; i < Count && !bNeeded; i++)
    {
        UBOOL bMatch = Page->Items(i) && Matches(Page->Items(i));
        if (!bMatch && FirstOther < 0)
            FirstOther = i;
        else if (bMatch && FirstOther >= 0)
            bNeeded = 1;
    }
    if (bNeeded)
    {
        NItemEntry** Order = (NItemEntry**)malloc(Count * sizeof(NItemEntry*));
        INT N = 0;
        for (INT Pass = 0; Pass < 2; Pass++)
            for (INT i = 0; i < Count; i++)
                if ((Page->Items(i) && Matches(Page->Items(i))) == (Pass == 0))
                    Order[N++] = Page->Items(i);
        Reorder(Page, Order);
        free(Order);
    }
    SetRows(Page, Shown(Page));
}

// the order of the page before the filter: the items it did not have (new ones) first, as the page puts new items
static void RestoreOrder(NCItemWnd* Page)
{
    INT Count = Page->Items.Num();
    NItemEntry** Order = (NItemEntry**)malloc(Count * sizeof(NItemEntry*));
    INT N = 0;
    for (INT i = 0; i < Count; i++)
    {
        INT j = 0;
        while (j < GOrderBefore.Num() && GOrderBefore(j) != Page->Items(i)->ObjectID)
            j++;
        if (j == GOrderBefore.Num())
            Order[N++] = Page->Items(i);
    }
    for (INT j = 0; j < GOrderBefore.Num(); j++)
        for (INT i = 0; i < Count; i++)
            if (Page->Items(i)->ObjectID == GOrderBefore(j))
            {
                Order[N++] = Page->Items(i);
                break;
            }
    if (N == Count)
        Reorder(Page, Order);
    free(Order);
}

static int CompareItems(const void* A, const void* B)
{
    NItemEntryC4* X = AsC4(*(NItemEntry* const*)A);
    NItemEntryC4* Y = AsC4(*(NItemEntry* const*)B);
    INT KX = KindOf((NItemEntry*)X), KY = KindOf((NItemEntry*)Y);
    if (KX != KY)
        return KX < KY ? -1 : 1;
    if (X->CrystalGrade != Y->CrystalGrade)
        return X->CrystalGrade > Y->CrystalGrade ? -1 : 1;   // the best grade first
    if (X->BodyPart != Y->BodyPart)
        return X->BodyPart < Y->BodyPart ? -1 : 1;
    if (X->ID != Y->ID)
        return X->ID < Y->ID ? -1 : 1;
    if (X->Enchant != Y->Enchant)
        return X->Enchant > Y->Enchant ? -1 : 1;
    return X->ObjectID < Y->ObjectID ? -1 : X->ObjectID > Y->ObjectID;
}

// the page the tab shows, sorted; the "Item" page keeps the order (its file, and the one "Todo" brings back)
static void SortPage()
{
    NCInvenItemWnd* Page = GInventory->TabCtrl->CurrentPage == GInventory->ItemWnd[1] ? GInventory->ItemWnd[1] : GInventory->ItemWnd[0];
    if (Page->bDragging || Page->Items.Num() < 2)
        return;
    Page->UnselectAll();
    Page->Selected = -1;
    qsort(&Page->Items(0), Page->Items.Num(), sizeof(NItemEntry*), CompareItems);
    if (Page == GInventory->ItemWnd[0])
    {
        GOrderBefore.Empty();
        for (INT i = 0; i < Page->Items.Num(); i++)
            GOrderBefore.AddItem(Page->Items(i)->ObjectID);
        GInventory->SaveItemOrder();
        Arrange(Page);
    }
}

static void SetFilter(INT Filter)
{
    NCInvenItemWnd* Page = GInventory->ItemWnd[0];
    if (Page->bDragging)
        return;
    GTab = Filter;
    if (GInventory->TabCtrl->CurrentPage != Page)
        GInventory->TabCtrl->SelectTab(0);
    if (Filter == GFilter)
        return;
    if (GFilter == FILTER_All)
    {
        GOrderBefore.Empty();
        for (INT i = 0; i < Page->Items.Num(); i++)
            GOrderBefore.AddItem(Page->Items(i)->ObjectID);
    }
    GFilter = Filter;
    Page->UnselectAll();
    Page->Selected = -1;
    Page->TopRow = 0;
    if (GFilter == FILTER_All)
    {
        RestoreOrder(Page);
        Page->UpdateScrollRange();
    }
    else
        Arrange(Page);
}

/*-----------------------------------------------------------------------------
    The character's model: Engine.dll's FActorSceneNode (what UCanvas::DrawActor uses) with a camera of our own.
-----------------------------------------------------------------------------*/

typedef void(__thiscall* FActorSceneNodeCtor)(void* Node, UViewport* Viewport, void* Target, AActor* Actor, AActor* ViewActor,
                                              FVector Origin, FRotator Rotation, FLOAT FOV);
typedef void(__thiscall* FActorSceneNodeRender)(void* Node, void* RI);
typedef void(__thiscall* FActorSceneNodeDtor)(void* Node);

typedef void(__thiscall* FCanvasUtilFlush)(void* Util);

// the pawn drawn into a box of the screen, facing the viewer, over what the canvas has drawn so far: the canvas's batch of
// tiles is drawn first (FCanvasUtil::Flush, as Engine.dll's own DrawActor does; UCanvas +0xA8), then the render interface's
// viewport is the box with its depth cleared, the camera at the world's origin looking along +X, the pawn moved in front of
// it for the call and put back. The scene node takes its projection from the viewport's size, which is the box's meanwhile.
static void DrawPawn(UCanvas* Canvas, AActor* Pawn, INT X, INT Y, INT BoxW, INT BoxH)
{
    UViewport* Viewport = Canvas->Viewport;
    static FCanvasUtilFlush Flush;
    static FActorSceneNodeCtor Ctor;
    static FActorSceneNodeRender Render;
    static FActorSceneNodeDtor Dtor;
    if (!Ctor)
    {
        HMODULE Engine = GetModuleHandleW(L"Engine.dll");
        if (!Engine)
            return;
        Ctor = (FActorSceneNodeCtor)GetProcAddress(Engine,
            "??0FActorSceneNode@@QAE@PAVUViewport@@PAVFRenderTarget@@PAVAActor@@2VFVector@@VFRotator@@M@Z");
        Render = (FActorSceneNodeRender)GetProcAddress(Engine, "?Render@FActorSceneNode@@UAEXPAVFRenderInterface@@@Z");
        Dtor = (FActorSceneNodeDtor)GetProcAddress(Engine, "??1FActorSceneNode@@UAE@XZ");
        Flush = (FCanvasUtilFlush)GetProcAddress(Engine, "?Flush@FCanvasUtil@@QAEXXZ");
        if (!Ctor || !Render || !Dtor || !Flush)
        {
            Ctor = NULL;
            return;
        }
    }
    AActor* ViewActor = ::nw::Field<AActor*>(Viewport, 0x3C);
    void* RI = ::nw::Field<void*>(Viewport, 0x154);
    INT& SizeX = ::nw::Field<INT>(Viewport, 0x94);
    INT& SizeY = ::nw::Field<INT>(Viewport, 0x98);
    INT W = SizeX, H = SizeY;
    if (!ViewActor || !RI || X < 0 || Y < 0 || X + BoxW > W || Y + BoxH > H)
        return;
    // FRenderInterface (Engine.dll calls them through these slots): SetViewport +0x0C, Clear +0x10
    typedef void(__thiscall* SetViewportFn)(void* RI, INT X, INT Y, INT W, INT H);
    typedef void(__thiscall* ClearFn)(void* RI, UBOOL UseColor, DWORD Color, UBOOL UseDepth, FLOAT Depth, UBOOL UseStencil, DWORD Stencil);
    void** VTable = *(void***)RI;
    // Height: the pawn's height in world units (about); its origin (the collision's middle) is a little under the middle of
    // what is drawn, Raise of the height
    const FLOAT FOV = 30.0f, Height = 46.0f, Fill = 0.7f, Raise = 0.12f;
    FLOAT K = (FLOAT)tan(FOV * 0.5 * 3.14159265 / 180.0) / (BoxW * 0.5f);   // world units per pixel, a unit away
    FLOAT Dist = Height / (BoxH * Fill * K);
    FVector OldLocation = Pawn->Location;
    FRotator OldRotation = Pawn->Rotation;
    Pawn->Location.X = Dist;
    Pawn->Location.Y = 0;
    Pawn->Location.Z = Height * Raise;
    Pawn->Rotation.Pitch = 0;
    Pawn->Rotation.Yaw = 32768 - 2048;   // facing the viewer, a little turned
    Pawn->Rotation.Roll = 0;
    void* Util = ::nw::Field<void*>(Canvas, 0xA8);
    if (Util)
        Flush(Util);
    ((SetViewportFn)VTable[3])(RI, X, Y, BoxW, BoxH);
    ((ClearFn)VTable[4])(RI, 0, 0, 1, 1.0f, 0, 0);
    SizeX = BoxW;
    SizeY = BoxH;
    FVector Origin(0, 0, 0);
    FRotator Rotation(0, 0, 0);
    __declspec(align(16)) BYTE Node[0x800];   // FActorSceneNode (DrawActor keeps it in less than 0x200 bytes of its frame)
    Ctor(Node, Viewport, (BYTE*)Viewport + 0xDC, Pawn, ViewActor, Origin, Rotation, FOV);
    Render(Node, RI);
    Dtor(Node);
    SizeX = W;
    SizeY = H;
    ((SetViewportFn)VTable[3])(RI, 0, 0, W, H);
    Pawn->Location = OldLocation;
    Pawn->Rotation = OldRotation;
}

/*-----------------------------------------------------------------------------
    The window.
-----------------------------------------------------------------------------*/

void NCustomInventory::CreateControls(NCInventoryWnd* Wnd)
{
    GInventory = Wnd;
    for (INT i = 0; i < Tabs; i++)
    {
        NCButton* Button = new NCButton(TEXT("L2UI_ch3.button.bigbutton"), TEXT("L2UI_ch3.button.bigbutton_down"), 0);
        Button->ID = NID_INV_TAB + i;
        Button->SetPosition(TabX, TabY, 1, TabH);   // laid out at the first paint (the labels' widths need the font)
        Wnd->CreateChild(Button, NWS_VISIBLE);
        Button->NormalTex = Button->PushedTex = Button->OverTex = NULL;
        GTabButtons[i] = Button;
    }
    GTabX[Tabs] = 0;
    GSortButton = new NCButton(TEXT("L2UI_ch3.button.bigbutton"), TEXT("L2UI_ch3.button.bigbutton_down"), 0);
    GSortButton->ID = NID_INV_SORT;
    GSortButton->SetPosition(SortX, SortY, 96, 23);   // the art's size (buttons draw 1:1)
    Wnd->CreateChild(GSortButton, NWS_VISIBLE);
    GSortButton->SetText(TEXT("Ordenar"));
    GFilter = FILTER_All;
    GTab = 0;
}

template <class T> static void DestroyControl(NCWnd* Parent, T*& Child)
{
    if (Child)
    {
        Parent->RemoveChildWindow(Child->Entry);
        delete Child;
        Child = NULL;
    }
}

void NCustomInventory::DestroyControls(NCInventoryWnd* Wnd)
{
    for (INT i = 0; i < Tabs; i++)
        DestroyControl(Wnd, GTabButtons[i]);
    DestroyControl(Wnd, GSortButton);
    GInventory = NULL;
}

UBOOL NCustomInventory::OnCommand(NCInventoryWnd* Wnd, INT ID)
{
    if (Wnd != GInventory)
        return 0;
    if (ID == NID_INV_SORT)
    {
        SortPage();
        return 1;
    }
    if (ID < NID_INV_TAB || ID >= NID_INV_TAB + Tabs)
        return 0;
    if (ID - NID_INV_TAB == TAB_Quest)
    {
        if (Wnd->ItemWnd[0]->bDragging)
            return 1;
        GTab = TAB_Quest;
        Wnd->TabCtrl->SelectTab(1);
    }
    else
        SetFilter(ID - NID_INV_TAB);
    return 1;
}

// the tabs' places: each its label and TabPad, shrunk alike when the row is too narrow
static void LayoutTabs()
{
    INT Widths[Tabs], Total = 0;
    for (INT i = 0; i < Tabs; i++)
    {
        SIZE Size;
        NCGDevice::GetTextExtent(&Size, GTabLabels[i], 0, 0, 0, 0, 0, L2FONT_Normal);
        Widths[i] = Size.cx;
        Total += Size.cx;
    }
    INT Room = TabRight - TabX - Total;
    INT Pad = Room / Tabs < TabPad ? Room / Tabs : TabPad;
    INT X = TabX;
    for (INT i = 0; i < Tabs; i++)
    {
        GTabX[i] = X;
        INT W = Widths[i] + Pad;
        GTabButtons[i]->SetPosition(X, TabY, W, TabH);
        X += W;
    }
    GTabX[Tabs] = X;
}

// a rectangle of the screen filled with a part of a texture, but for the hole (window pixels; a hole of no size: none)
struct NHole
{
    INT X, Y, W, H;
};

static void FillExcept(NCGDevice* Device, UTexture* T, INT X, INT Y, INT W, INT H, INT U, INT V, INT UL, INT VL, const NHole& Hole)
{
    INT HX0 = Hole.X > X ? Hole.X : X, HY0 = Hole.Y > Y ? Hole.Y : Y;
    INT HX1 = Hole.X + Hole.W < X + W ? Hole.X + Hole.W : X + W, HY1 = Hole.Y + Hole.H < Y + H ? Hole.Y + Hole.H : Y + H;
    if (HX0 >= HX1 || HY0 >= HY1)
    {
        Device->DrawTexture(X, Y, W, H, U, V, UL, VL, T, 0xFF, 1);
        return;
    }
    if (HY0 > Y)
        Device->DrawTexture(X, Y, W, HY0 - Y, U, V, UL, VL, T, 0xFF, 1);
    if (Y + H > HY1)
        Device->DrawTexture(X, HY1, W, Y + H - HY1, U, V, UL, VL, T, 0xFF, 1);
    if (HX0 > X)
        Device->DrawTexture(X, HY0, HX0 - X, HY1 - HY0, U, V, UL, VL, T, 0xFF, 1);
    if (X + W > HX1)
        Device->DrawTexture(HX1, HY0, X + W - HX1, HY1 - HY0, U, V, UL, VL, T, 0xFF, 1);
}

// a nine-part box from the stock back's item panel (its corners and edges kept, its empty right strip stretched inside, but
// not over the hole); without the inside, only its frame
static void DrawItemPanel(NCGDevice* Device, UTexture* T, INT X, INT Y, INT W, INT H, const NHole* Hole = NULL, UBOOL bInside = 1)
{
    // the top edge: rows 158 .. 161 (its line is 159 and 160) right of the stock tabs' holes, which leave the rest of it clear
    // the left edge: columns 0 .. 3 (its line is 1 and 2), so that each edge has its line 1 pixel in, 2 thick
    // the top right corner: the stock one is cut diagonally into the window's frame, so it is put together from the top edge
    // (3 x 3), the right edge's first row under it and the right edge's outer column (the canvas draws nothing for a mirrored,
    // negative width, part)
    enum { SX = 2, SY = 160, SW = 254, SH = 157, M = 4, CX = 232, CY = 172, CS = 16, TopY = 158, TopX = 206, TopW = 44, LX = 0 };
    static const NHole NoHole = { 0, 0, 0, 0 };
    INT IW = W - 2 * M, IH = H - 2 * M;
    Device->DrawTexture(X, Y, M, M, LX, TopY, M, M, T, 0xFF, 1);
    Device->DrawTexture(X + M, Y, IW, M, TopX, TopY, TopW, M, T, 0xFF, 1);
    Device->DrawTexture(X + W - M, Y, M - 1, M - 1, TopX, TopY, M - 1, M - 1, T, 0xFF, 1);
    Device->DrawTexture(X + W - M, Y + M - 1, M - 1, 1, SX + SW - M, SY + M, M - 1, 1, T, 0xFF, 1);
    Device->DrawTexture(X + W - 1, Y, 1, M, SX + SW - 1, SY + M, 1, M, T, 0xFF, 1);
    Device->DrawTexture(X, Y + M, M, IH, LX, SY + M, M, SH - 2 * M, T, 0xFF, 1);
    if (bInside)
        FillExcept(Device, T, X + M, Y + M, IW, IH, CX, CY, CS, CS, Hole ? *Hole : NoHole);
    Device->DrawTexture(X + W - M, Y + M, M, IH, SX + SW - M, SY + M, M, SH - 2 * M, T, 0xFF, 1);
    Device->DrawTexture(X, Y + H - M, M, M, LX, SY + SH - M, M, M, T, 0xFF, 1);
    Device->DrawTexture(X + M, Y + H - M, IW, M, SX + M, SY + SH - M, SW - 2 * M, M, T, 0xFF, 1);
    Device->DrawTexture(X + W - M, Y + H - M, M, M, SX + SW - M, SY + SH - M, M, M, T, 0xFF, 1);
}

// a tab: the stock inventory tab (its 94 x 23 art) cut in three and stretched, the label in the middle
static void DrawTab(NCInventoryWnd* Wnd, NCGDevice* Device, INT Index)
{
    UBOOL bOn = Index == GTab;
    UTexture* T = Wnd->FindTexture(bOn ? TEXT("L2UI_CH3.InventoryWnd.Inventory_tab1") : TEXT("L2UI_CH3.InventoryWnd.Inventory_tab2"));
    INT X = GTabX[Index], W = GTabX[Index + 1] - X;
    enum { E = 6, AW = 94 };
    if (T)
    {
        Device->DrawTexture(X, TabY, E, TabH, 0, 0, E, TabH, T, 0xFF, 1);
        Device->DrawTexture(X + E, TabY, W - 2 * E, TabH, E, 0, AW - 2 * E, TabH, T, 0xFF, 1);
        Device->DrawTexture(X + W - E, TabY, E, TabH, AW - E, 0, E, TabH, T, 0xFF, 1);
    }
    SIZE Size;
    NCGDevice::GetTextExtent(&Size, GTabLabels[Index], 0, 0, 0, 0, 0, L2FONT_Normal);
    Device->DrawText(X + (W - Size.cx) / 2, TabY + 6, bOn ? 0xFFFFFFFF : 0xFFB09B79, GTabLabels[Index], 0, 0, 0, L2FONT_Normal,
                     0, 0, 0, 0, 0, 0, 0);
}

// the stock back (256 x 381 of a 256 x 512 texture) cut up: the window's fill (a plain part of it, stretched), the paperdoll's
// panel with the model's box framed, the grid's panel with an empty cell (as the stock back has them: 37 x 35
// apart, the icon 1 pixel right of and 2 below the cell's corner) for each place of the grid, the adena / weight box and the
// tabs; then the model over all that
void NCustomInventory::PaintBack(NCInventoryWnd* Wnd, NCGDevice* Device)
{
    UTexture* T = Wnd->BackTex;
    if (!T)
        return;
    NHole Hole = { DollX + ModelX, DollY + ModelY, ModelW, ModelH };
    Device->DrawTexture(0, 20, Width, Height - 20, 210, 142, 30, 14, T, 0xFF, 1);
    DrawItemPanel(Device, T, DollX, DollY, DollW, DollH);
    // the model's box: a 1 pixel line like the slots' (3 pixels around the box), a piece of the first slot's top border for
    // the top and bottom lines and of its left border for the sides (along the line, so that the filter keeps its colour)
    {
        enum { HU = 20, HV = 16, VU = 14, VV = 22, L = 8, A = 3 };
        INT BX = Hole.X - A, BY = Hole.Y - A, BW = Hole.W + 2 * A, BH = Hole.H + 2 * A;
        Device->DrawTexture(BX, BY, BW, 1, HU, HV, L, 1, T, 0xFF, 1);
        Device->DrawTexture(BX, BY + BH - 1, BW, 1, HU, HV, L, 1, T, 0xFF, 1);
        Device->DrawTexture(BX, BY + 1, 1, BH - 2, VU, VV, 1, L, T, 0xFF, 1);
        Device->DrawTexture(BX + BW - 1, BY + 1, 1, BH - 2, VU, VV, 1, L, T, 0xFF, 1);
    }
    DrawItemPanel(Device, T, GridX - 8, GridY - 4, Width - GridX + 4, LowerY - GridY + 2);
    for (INT Row = 0; Row < GridRows; Row++)
        for (INT Col = 0; Col < GridColumns; Col++)
            Device->DrawTexture(GridX + Col * 37, GridY - 1 + Row * 35, 37, 35, 9, 168, 37, 35, T, 0xFF, 1);
    Device->DrawTexture(LowerX, LowerY, 256, 61, 0, 320, 256, 61, T, 0xFF, 1);
    if (!GTabX[Tabs])
        LayoutTabs();
    for (INT i = 0; i < Tabs; i++)
        if (i != GTab)
            DrawTab(Wnd, Device, i);
    DrawTab(Wnd, Device, GTab);   // the chosen one over its neighbours

    User* Me = GConsole->GetMyUser();
    if (Me && Me->Pawn && Device->Canvas && Device->Canvas->Viewport)
        DrawPawn(Device->Canvas, (AActor*)Me->Pawn, Wnd->ScreenX + Hole.X, Wnd->ScreenY + Hole.Y, Hole.W, Hole.H);
}

void NCustomInventory::PaintCount(NCInventoryWnd* Wnd, NCGDevice* Device, const TCHAR* Text)
{
    // in the adena / weight box, centered in its empty left part (from its inside's edge to the adena icon) and between the
    // adena and the weight rows
    enum { Left = LowerX + 10, Right = LowerX + 0x60, Y = LowerY + 24 };
    SIZE Size;
    NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0, L2FONT_Normal);
    Device->DrawText((Left + Right - Size.cx) / 2, Y, 0xFFB09B79, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
}

/*-----------------------------------------------------------------------------
    The paperdoll's places.
-----------------------------------------------------------------------------*/

// the slots (NConsoleWnd::GetEquipSlot) by column (0 left, 1 right) and row: the armor and the cloak on the left, the jewels
// on the right, the weapon and the shield on the last row
static const BYTE GSlotPlace[15][2] = {
    { 0, 6 },   // 0 underwear
    { 0, 0 },   // 1 helmet
    { 0, 5 },   // 2 cloak
    { 1, 0 },   // 3 hair accessory
    { 1, 1 },   // 4 necklace
    { 0, 7 },   // 5 weapon
    { 0, 1 },   // 6 chest
    { 1, 7 },   // 7 shield
    { 1, 2 },   // 8 earring
    { 1, 3 },   // 9 earring
    { 0, 3 },   // 10 gloves
    { 0, 2 },   // 11 legs
    { 0, 4 },   // 12 boots
    { 1, 4 },   // 13 ring
    { 1, 5 },   // 14 ring
};
// the stock places of the slots' icons (the stock paperdoll window at (8, 31) of the inventory, the back drawn from y 20)
static const BYTE GStockSlot[15][2] = {
    { 0x07, 0x07 }, { 0x2E, 0x07 }, { 0x55, 0x07 }, { 0x82, 0x07 }, { 0xA9, 0x07 },
    { 0x07, 0x2D }, { 0x2E, 0x2D }, { 0x55, 0x2D }, { 0x82, 0x2D }, { 0xA9, 0x2D },
    { 0x07, 0x53 }, { 0x2E, 0x53 }, { 0x55, 0x53 }, { 0x82, 0x53 }, { 0xA9, 0x53 },
};

void NCustomInventory::SlotPos(INT Slot, INT& X, INT& Y)
{
    if (Slot < 0 || Slot >= 15)
        Slot = 0;
    X = GSlotPlace[Slot][0] ? DollW - 39 : 7;
    Y = SlotY + GSlotPlace[Slot][1] * 37;
}

void NCustomInventory::HennaPos(INT Index, INT& X, INT& Y)
{
    X = ModelX + ModelW / 2 - 40 + Index * 27;
    Y = ModelY + ModelH + 8;
}

// each slot's picture from the stock back (its 34 x 34 box, where the icon is 1 pixel right of and 2 below the corner),
// 1 pixel lower: the icon 1 pixel inside on every side, and the frame an equipped item draws (1 pixel around the icon) on
// the picture's
void NCustomInventory::PaintEmptySlots(NCWnd* Doll, NCGDevice* Device)
{
    UTexture* T = Doll->FindTexture(TEXT("L2UI_CH3.InventoryWnd.Inventory_Back"));
    if (!T)
        return;
    for (INT Slot = 0; Slot < 15; Slot++)
    {
        INT X, Y;
        SlotPos(Slot, X, Y);
        Device->DrawTexture(X - 1, Y - 1, 34, 34, 8 + GStockSlot[Slot][0] - 1, 11 + GStockSlot[Slot][1] - 2, 34, 34, T, 0xFF, 1);
    }
}

// a place saved for the narrow window can leave the wide one past the screen's edge
void NCustomInventory::KeepOnScreen(NCInventoryWnd* Wnd)
{
    INT ScreenW = (INT)GDesktop->Width, ScreenH = (INT)GDesktop->Height;
    INT X = Wnd->ScreenX, Y = Wnd->ScreenY;
    if (X > ScreenW - Width)
        X = ScreenW - Width;
    if (Y > ScreenH - Height)
        Y = ScreenH - Height;
    if (X < 0)
        X = 0;
    if (Y < 0)
        Y = 0;
    if (X != Wnd->ScreenX || Y != Wnd->ScreenY)
        Wnd->SetPosition(X, Y, Width, Height);
}
