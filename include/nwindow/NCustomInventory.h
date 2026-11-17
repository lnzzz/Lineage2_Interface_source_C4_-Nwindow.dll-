#pragma once
// NCustomInventory - the wide inventory (custom, NW_CUSTOM builds only; not part of the stock image).
//
// The inventory window is 640 x 372 instead of 256 x 401. On the left the paperdoll: the armor and the cloak in a column on
// the left, the jewels in a column on the right, the weapon and the shield under them, the character's 3D model between the
// columns and the tattoos under it. On the right a row of tabs (Todo, Armas, Armaduras, Joyas, Consumibles, Materiales,
// Otros, Quest) over a grid of 10 x 7 icons, and under the grid the "Ordenar" button, the item count and the stock adena /
// weight box with the crystallize and trash icons.
//
// The type tabs show only the items of one type on the "Item" page; "Ordenar" sorts the page shown by type, grade and item
// (the order is the one the page saves, ..\iwo). The kind of an etc item comes from system\NWindowItems.txt ("<item id>
// <C|M>" lines: consumable, material; the client build writes it from the server's items), weapons, armor and jewels from
// the item's own type. Its "<item id> F" lines are the weapons with the effect button (see PaintEffectButton) and its "<item id> H"
// lines the White Crystal helmets with the helmet button (PaintHelmetButton). A filter keeps the page's items that match first (in their order) and shows only those; the order
// the page had when the filter was chosen comes back with "Todo".
//
// The model is Engine.dll's FActorSceneNode (what UCanvas::DrawActor uses) with a camera of our own, drawn into its box with
// the render interface's viewport, after the canvas's batch of 2D tiles is drawn (else the window's back would cover it).

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)

class NCGDevice;
class NCInventoryWnd;
class NCItemWnd;
struct NItemEntry;

namespace NCustomInventory
{
    // the window and its parts (window pixels)
    enum
    {
        Width = 640,
        Height = 372,
        DollX = 4,                 // the paperdoll window (NCEquipItemWnd)
        DollY = 22,
        DollW = 232,
        DollH = Height - 26,
        SlotY = 14,                // the first slot's icon, in the paperdoll window (its picture 1 pixel around it)
        ModelX = 46,               // the model's box, in the paperdoll window: its line (3 pixels around it) from the first
        ModelY = SlotY + 2,        // slot's top to the last one's bottom, 3 pixels from the columns
        ModelW = 140,
        ModelH = 287,
        GridX = 248,
        GridY = 54,
        GridColumns = 10,
        GridRows = 7,
        TabX = 244,
        TabY = 24,
        TabRight = Width - 8,
        LowerX = Width - 260,      // the adena / weight box (the stock back from its y 320 on)
        LowerY = 306,
        LowerShiftX = LowerX,      // what the stock positions of that box move by
        LowerShiftY = LowerY - 340,
    };

    void CreateControls(NCInventoryWnd* Wnd);              // NCInventoryWnd::OnCreate, after the stock children
    void DestroyControls(NCInventoryWnd* Wnd);             // NCInventoryWnd::OnDestroy
    UBOOL OnCommand(NCInventoryWnd* Wnd, INT ID);          // NCInventoryWnd::WindowProc's NWM_COMMAND
    void PaintBack(NCInventoryWnd* Wnd, NCGDevice* Device); // NCInventoryWnd::OnPaint, instead of the stock back
    void PaintCount(NCInventoryWnd* Wnd, NCGDevice* Device, const TCHAR* Text);   // the "(used/most)" text
    void KeepOnScreen(NCInventoryWnd* Wnd);                // NCInventoryWnd::ShowWindow (a position saved for the narrow window)

    // the item pages (NCInvenItemWnd): how many items show (the matching ones come first), after putting them in place
    INT Shown(NCItemWnd* Page);
    void Arrange(NCItemWnd* Page);

    // the paperdoll (NCEquipItemWnd): a slot's icon (0 .. 14, NConsoleWnd::GetEquipSlot), a tattoo's box (26 x 26), in the
    // paperdoll window; the empty slots' pictures (the stock back's)
    void SlotPos(INT Slot, INT& X, INT& Y);
    void HennaPos(INT Index, INT& X, INT& Y);
    void PaintEmptySlots(NCWnd* Doll, NCGDevice* Device);

    // L2 Ignition: the effect button of the Triumph weapons (custom.TriumphEffectManager on the server). A star in the top
    // right corner of the weapon slot that asks the server for the effect picker of the worn weapon (bypass "triumphfx menu
    // <object id>"). The weapons that have it are the "<item id> F" lines of system\NWindowItems.txt. Weapon: the
    // paperdoll's weapon (NULL: none); X, Y: the mouse (screen pixels).
    void PaintEffectButton(NCWnd* Doll, NCGDevice* Device, NItemEntry* Weapon);
    UBOOL OverEffectButton(NCWnd* Doll, NItemEntry* Weapon, INT X, INT Y);    // remembers it for the paint
    UBOOL ClickEffectButton(NCWnd* Doll, NItemEntry* Weapon, INT X, INT Y);   // sends the request when the click is on it

    // L2 Ignition: the helmet button of the White Crystal helmets (custom.WhiteCrystalHelmetManager on the server). The same
    // star in the top right corner of the helmet slot, for the "<item id> H" lines of system\NWindowItems.txt: it asks for
    // the menu that shows or hides the costume helmet (bypass "wchelm menu <object id>"). Helmet: the paperdoll's helmet.
    void PaintHelmetButton(NCWnd* Doll, NCGDevice* Device, NItemEntry* Helmet);
    UBOOL OverHelmetButton(NCWnd* Doll, NItemEntry* Helmet, INT X, INT Y);
    UBOOL ClickHelmetButton(NCWnd* Doll, NItemEntry* Helmet, INT X, INT Y);
}

#pragma pack(pop)
