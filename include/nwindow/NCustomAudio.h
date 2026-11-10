#pragma once
// NCustomAudio - "Mute All Sounds" in the audio page of the options (custom, NW_CUSTOM builds only; not part of the stock
// image).
//
// A check box under the four volume boxes. Checking it keeps the levels the boxes show, sets the four to Off and greys
// them; unchecking it gives the kept levels back. Like every option, it takes effect with OK / Apply (the stock Apply sets
// the volumes from the boxes, Off stops what plays); then the state and the kept levels are saved in
// system\NWindowCustom.ini ([Audio] MuteAll = 1 / 0, Level0 .. Level3 = sound, music and the two voices, 0 = "5" .. 5 = Off).
// Cancel leaves everything as it was: the page shows the saved state the next time it opens.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)

class NCAudioOptionWnd;

namespace NCustomAudio
{
    void OnCreate(NCAudioOptionWnd* Wnd);                // NCAudioOptionWnd::OnCreate, after the volume boxes
    void OnDestroy(NCAudioOptionWnd* Wnd);               // NCAudioOptionWnd::OnDestroy, before the boxes go
    void OnSelectVolumes(NCAudioOptionWnd* Wnd);         // NCAudioOptionWnd::SelectVolumes, after the boxes show the volumes
    UBOOL OnCommand(NCAudioOptionWnd* Wnd, INT ID);      // NCAudioOptionWnd::WindowProc, NWM_COMMAND: 1 = the check box
    void OnApply(NCAudioOptionWnd* Wnd);                 // NCAudioOptionWnd::Apply, after the volumes are set
}

#pragma pack(pop)
