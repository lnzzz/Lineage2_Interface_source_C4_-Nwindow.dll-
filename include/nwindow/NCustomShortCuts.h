#pragma once
// NCustomShortCuts - extra shortcut bars (custom, NW_CUSTOM builds only; not part of the stock image).
//
// An extra bar owns no slots: it is an NCShortCutWnd that shows another page of the main bar's 120-slot table (the
// server keeps the slots as Page * 12 + position, pages 0..9, so nothing changes on the server). The bars stack beside
// the main bar (to its left when it is vertical, above it when it is horizontal; the other side when the screen has no
// room), take its orientation, hide with it and move with it. The Expand button on the main bar shows one more bar per
// click and hides them all after the last.
//
// Each bar (the main one too) has a key set: F1 .. F12, the number row, the QWERTY row, the ASDF row, the ZXCV row or
// none (defaults: main F, first extra numbers, second extra QWERTY). The rows are physical keys (scan codes), so any
// keyboard layout works, and the labels show the layout's characters. Every set but F works only while the chat is in
// "Enter to chat" mode (the keys type otherwise); then those letters no longer open windows (Alt + letter still does).
// A right click on a bar outside its slots (or Alt+S) opens the settings window (NCShortCutOptionWnd). How many bars are shown,
// each bar's page and each bar's key set are kept in system\NWindowCustom.ini ([ShortCut] Bars, Page1..4, Bar1Keys ..
// Bar5Keys = F / Num / QWERTY / ASDF / ZXCV / None).
//
// The lock button (left of the turn button on the main bar) locks every bar: nothing can be dragged off a bar (to move or
// delete a shortcut) or dropped on one (from the skills, the inventory, the actions, the macros ...); the shortcuts are still
// used by click and key. Kept as [ShortCut] Locked = 0 / 1.

#include "nwindow/NCShortCutWnd.h"
#pragma pack(push, 4)

class NCConsole;

// the extra bar: no field of its own, under the main bar's UClass (CShortCutWnd)
class NCExtraShortCutWnd : public NCShortCutWnd
{
public:
    static UClass* StaticClass() { return NCShortCutWnd::StaticClass(); }

    INT BarIndex() const;              // 0 .. NCustomShortCuts::MaxBars - 1 (kept outside the object)

    NCExtraShortCutWnd() : NCShortCutWnd(12, 10, 32) {}

    virtual INT OnCreate();                                          // 37 the stock tables are freed, the main bar's are used
    virtual INT OnDestroy();                                         // 38 the main bar's tables are left alone
    virtual void ShowWindow();                                       // 48 the turn button stays hidden
    virtual INT OnMove(WPARAM wParam, LPARAM lParam);                // 72 a drag moves the main bar (the set follows)
    virtual INT OnRotateClicked();                                   // 80 the main bar decides the orientation
    virtual INT OnPadClicked();                                      // 81 no game pad layout
};
NW_ASSERT_SIZE(NCExtraShortCutWnd, 0x294);

// the settings window (Alt+S, or a right click on a bar outside its slots): Enter to chat on / off (Option.ini, as the
// game options do), how many extra bars are shown and each bar's key set. No field of its own, under NCWnd's UClass;
// its controls are statics of NCustomShortCuts.cpp
class NCShortCutOptionWnd : public NCWnd
{
public:
    static UClass* StaticClass() { return NCWnd::StaticClass(); }

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 the controls' NWM_COMMAND
    virtual INT OnCreate();                                          // 37 title bar, check box, arrows
    virtual INT OnDestroy();                                         // 38
    virtual void ShowWindow();                                       // 48 only when asked (the desktop shows all its children)
    virtual void HideWindow();                                       // 49 (its close button too)
    virtual INT OnPaint(NCGDevice* Device);                          // 62 back, labels and values
};
NW_ASSERT_SIZE(NCShortCutOptionWnd, 0xE8);

namespace NCustomShortCuts
{
    enum { MaxBars = 4, NID_SHORTCUT_EXPAND = (INT)0xFF00000A, NID_SHORTCUT_LOCK = (INT)0xFF00000B };

    void Create(NCConsole* Desktop);           // NCConsole::OnCreate, after the stock windows
    void Destroy(NCConsole* Desktop);          // NCConsole::OnDestroy, before the stock windows
    void Tick();                               // NConsoleWnd::Tick, every frame
    UBOOL OnCommand(NCShortCutWnd* Wnd, WPARAM ID);                  // NCShortCutWnd::WindowProc's NWM_COMMAND
    UBOOL OnKey(UINT Msg, WPARAM wParam, LPARAM lParam);             // NConsoleWnd::HookShortCutKey, before the main bar
    UTexture* KeyTexture(NCShortCutWnd* Wnd, INT Index);             // NCShortCutWnd::PaintBar: the F-key texture, or NULL
    void PaintKeyLabels(NCShortCutWnd* Wnd, NCGDevice* Device);      // NCShortCutWnd::PaintBar: the labels of other key sets
    UBOOL OnRightClick(NCShortCutWnd* Wnd);                          // NCShortCutWnd::OnRButtonDown outside the slots
    void ToggleOptions();                                            // the settings window shown / hidden
    // the back of the custom windows: the dialogs' panel (L2UI_CH3.dialog.system_back, 256 x 108) cut in nine, any size
    void DrawPanel(NCWnd* Owner, NCGDevice* Device, INT X, INT Y, INT W, INT H);
    // the menu bar's icons (NCMenuWnd: OnCreate, OnDestroy, WindowProc), after its four
    enum { MenuIcons = 2, MenuIconStep = 37 };
    void CreateMenuIcons(NCWnd* Menu);
    void DestroyMenuIcons(NCWnd* Menu);
    void PaintMenuIcons(NCWnd* Menu, NCGDevice* Device);             // NCMenuWnd::OnPaint: the potion icon's frame
    UBOOL OnMenuCommand(WPARAM ID);
    INT IndexOf(const NCShortCutWnd* Wnd);     // the extra bar's index, -1 for any other window
    UBOOL IsLocked();                          // the bars are locked: no drag off a bar, no shortcut registered by a drop
}

#pragma pack(pop)
