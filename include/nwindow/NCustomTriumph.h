#pragma once
// NCustomTriumph - L2 Ignition: whether the player sees the effects of the Triumph weapons (custom, NW_CUSTOM builds only;
// not part of the stock image).
//
// Two check boxes in the game page of the options, under the zone title one: the effects of the player's own Triumph
// weapons and those of the other players' weapons. The server draws them (custom.TriumphEffectManager sends a copy of the
// weapon with the effect), so the choice goes to it as the bypass "triumphfx view <own 0|1> <others 0|1>": with OK /
// Apply while in the world, and once after each entry to the world (the server forgets it with the character). Saved in
// system\NWindowCustom.ini ([Triumph] ShowOwn, ShowOthers; 1 = shown, the default).

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)

class NCGameOptionWnd;

namespace NCustomTriumph
{
    void OnCreate(NCGameOptionWnd* Wnd);          // NCGameOptionWnd::OnCreate, before InitItems (it loads the settings)
    void OnDestroy(NCGameOptionWnd* Wnd);         // NCGameOptionWnd::OnDestroy
    void OnLoadSettings(NCGameOptionWnd* Wnd);    // NCGameOptionWnd::LoadSettings: the boxes show the saved choice
    void OnApply(NCGameOptionWnd* Wnd);           // NCGameOptionWnd::Apply: saved and, in the world, sent
    void Tick();                                  // every frame (NCustomShortCuts::Tick): the choice after entering the world
}

#pragma pack(pop)
