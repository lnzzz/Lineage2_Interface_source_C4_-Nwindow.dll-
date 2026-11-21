#pragma once
// NCustomGraphics - L2 Ignition: graphics upgrades the player turns on or off from the video page of the options (custom,
// NW_CUSTOM builds only; not part of the stock image). Both are off by default: the client looks like the stock one.
//
// - Anisotropic filter (applied at once): the stock D3DDrv only reaches anisotropy through LevelOfAnisotropy of l2.ini
//   (1 in the shipped one) and its SetRes / SetUseTrilinear / state cache set the min filter to linear behind it. The
//   device's SetSamplerState (IDirect3DDevice9 vtable slot 69) is wrapped instead: while the box is on, a linear min
//   filter becomes anisotropic and the max anisotropy the device's (up to 16x); point filtering (Texture filter off) and
//   the mag and mip filters are left alone. Turning the box on or off sets every sampler again.
// - HD textures (applied at the next start): system\..\TexturesHD holds packages with the same names as the stock ones;
//   with the box on, "../TexturesHD/*.utx" goes first in GSys->Paths (l2.ini [Core.System] Paths, which
//   appFindPackageFile walks on every package load) before the world loads, so those packages win over ../Textures.
//   Without the folder the box is greyed out.
//
// Saved in system\NWindowCustom.ini [Graphics] Anisotropy, HDTextures (0 / 1).

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)

class NCVideoOptionWnd;

namespace NCustomGraphics
{
    void OnCreate(NCVideoOptionWnd* Wnd);         // NCVideoOptionWnd::OnCreate, before InitItems (it loads the settings)
    void OnDestroy(NCVideoOptionWnd* Wnd);        // NCVideoOptionWnd::OnDestroy
    void OnLoadSettings(NCVideoOptionWnd* Wnd);   // NCVideoOptionWnd::LoadSettings: the boxes show the saved choice
    void OnApply(NCVideoOptionWnd* Wnd);          // NCVideoOptionWnd::Apply: saved; the filter changes at once
    void Tick();                                  // every console tick from the login screen on: the HD path once, the
                                                  // sampler hook on the current device
}

#pragma pack(pop)
