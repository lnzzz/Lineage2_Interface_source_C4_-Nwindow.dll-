#pragma once
// NCustomAutoPotion - automatic potions (custom, NW_CUSTOM builds only; not part of the stock image).
//
// Three rows: HP, MP and CP. A potion is chosen by dragging it from the inventory onto a row (a right click on the row's
// slot clears it); each row has its own switch, the percentage under which the potion is used and the time it waits
// between two uses (the server does not refuse a healing potion while its effect lasts: every use spends one; the HP
// potions have a 9 s reuse on the server, the MP and CP ones none). The potion goes by its item id (any stack of it in
// the inventory), through NConsoleWnd::RequestUseItem, only in the game and alive. Kept in system\NWindowCustom.ini
// ([AutoPotion] HPItem, HPOn, HPBelow, HPEvery (ms), the same for MP and CP). The window opens with Alt+A or the button of
// the shortcut bars window.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)

class NCConsole;
struct NItemEntry;

// the window: no field of its own, under NCWnd's UClass; its controls are statics of NCustomAutoPotion.cpp
class NCAutoPotionWnd : public NCWnd
{
public:
    static UClass* StaticClass() { return NCWnd::StaticClass(); }

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 the controls' NWM_COMMAND
    virtual INT OnCreate();                                          // 37 title bar, check boxes, arrows
    virtual INT OnDestroy();                                         // 38
    virtual void ShowWindow();                                       // 48 only when asked (the desktop shows all its children)
    virtual void HideWindow();                                       // 49
    virtual INT OnPaint(NCGDevice* Device);                          // 62 back, slots, names, values
    virtual INT OnRButtonDown(WPARAM wParam, LPARAM lParam);         // 70 a row's slot is cleared
};
NW_ASSERT_SIZE(NCAutoPotionWnd, 0xE8);

namespace NCustomAutoPotion
{
    void Create(NCConsole* Desktop);           // NCustomShortCuts::Create
    void Destroy(NCConsole* Desktop);          // NCustomShortCuts::Destroy
    void Tick();                               // every frame: the potions due are used
    void Toggle();                             // the window shown / hidden
    // NCInvenItemWnd::OnLButtonUp: an inventory item released over the window goes to the row under it (1: taken)
    UBOOL OnDropItem(NCWnd* Target, INT X, INT Y, NItemEntry* Item);
}

#pragma pack(pop)
