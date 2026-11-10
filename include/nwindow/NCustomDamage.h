#pragma once
// NCustomDamage - the damage done to the target, over its head (custom, NW_CUSTOM builds only; not part of the stock image).
//
// The server tells the player each hit as system messages: "Critical hit!" (44) or "Magic critical hit!" (1280) first,
// when it is one, then "You did $s1 damage" (35). Neither says to whom: the number goes over the selected target (followed by
// its object id; where it was last seen when it is gone). Each number rises from over the head and fades in a little more
// than a second, white, yellow for a critical one. A check box of the shortcut bars window (Alt+S) turns it off; kept in
// system\NWindowCustom.ini ([Damage] Show = 1 / 0, on when missing).

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)

class NCGDevice;

namespace NCustomDamage
{
    void OnSystemMessage(INT ID, const TCHAR* Param1);   // NConsoleWnd::EndSystemMessageParam, before the text is made
    void Paint(NCGDevice* Device);                       // NCConsole::OnPaint: under every window
    UBOOL IsShown();
    void SetShown(UBOOL bShown);                         // kept in the ini file
}

#pragma pack(pop)
