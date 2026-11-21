/*=============================================================================
    NCustomGraphics.cpp - L2 Ignition: the anisotropic filter and HD textures switches of the video page (custom,
    NW_CUSTOM builds only). See NCustomGraphics.h.
=============================================================================*/
#include "nwindow/NCustomGraphics.h"
#include "nwindow/NCCheckBox.h"
#include "nwindow/NCVideoOptionWnd.h"
#include "nwindow/NConsoleWnd.h"
#include <d3d9.h>

class USystem;
CORE_API extern USystem* GSys;     // ?GSys@@3PAVUSystem@@A

enum
{
    NID_VIDEO_ANISOTROPY = (INT)0xFF100301,
    NID_VIDEO_HDTEXTURES = (INT)0xFF100302,
    SystemPaths = 0x64,             // USystem::Paths, TArray<FString> (Core's USystem::StaticConstructor; appFindPackageFile
                                    // walks +0x64 / +0x68)
    D3DDeviceOffset = 0x4758,       // UD3DRenderDevice: the IDirect3DDevice9* (D3DDrv SetRes, SetUseTrilinear)
    SetSamplerStateSlot = 69,       // IDirect3DDevice9 vtable (D3DDrv calls [vtable+0x114])
    MaxSamplers = 16,
    MaxHookedVtables = 4,
};

static const TCHAR* HDPath = TEXT("../TexturesHD/*.utx");

static NCCheckBox* GAnisoBox = NULL;
static NCCheckBox* GHDBox = NULL;

/*-----------------------------------------------------------------------------
    Settings: [Graphics] of system\NWindowCustom.ini.
-----------------------------------------------------------------------------*/

static const TCHAR* SystemDir()
{
    static TCHAR Dir[MAX_PATH] = { 0 };
    if (!Dir[0])
    {
        GetModuleFileNameW(NULL, Dir, MAX_PATH);
        TCHAR* Slash = wcsrchr(Dir, '\\');
        if (Slash)
            Slash[1] = 0;
    }
    return Dir;
}

static const TCHAR* IniPath()
{
    static TCHAR Path[MAX_PATH] = { 0 };
    if (!Path[0])
    {
        wcscpy_s(Path, MAX_PATH, SystemDir());
        wcscat_s(Path, MAX_PATH, TEXT("NWindowCustom.ini"));
    }
    return Path;
}

static UBOOL Saved(const TCHAR* Key)
{
    return GetPrivateProfileIntW(TEXT("Graphics"), Key, 0, IniPath()) != 0;
}

static void Save(const TCHAR* Key, UBOOL bOn)
{
    WritePrivateProfileStringW(TEXT("Graphics"), Key, bOn ? TEXT("1") : TEXT("0"), IniPath());
}

// system\..\TexturesHD: the HD pack is installed
static UBOOL HDFolderExists()
{
    TCHAR Path[MAX_PATH];
    wcscpy_s(Path, MAX_PATH, SystemDir());
    wcscat_s(Path, MAX_PATH, TEXT("..\\TexturesHD"));
    DWORD Attr = GetFileAttributesW(Path);
    return Attr != INVALID_FILE_ATTRIBUTES && (Attr & FILE_ATTRIBUTE_DIRECTORY);
}

/*-----------------------------------------------------------------------------
    HD textures: the pack's folder first in the package paths, once, before the world loads.
-----------------------------------------------------------------------------*/

static void AddHDPath()
{
    TArray<FString>& Paths = *(TArray<FString>*)((BYTE*)GSys + SystemPaths);
    // the layout check: the stock list holds about ten "../Dir/*.ext" lines
    if (Paths.Num() <= 0 || Paths.Num() > 64 || appStrstr(*Paths(0), TEXT("/*.")) == NULL)
    {
        GLog->Logf(TEXT("NCustomGraphics: unexpected package paths (%d), HD textures off"), Paths.Num());
        return;
    }
    for (INT i = 0; i < Paths.Num(); i++)
        if (Paths(i) == HDPath)
            return;
    Paths.Insert(0);
    memset(&Paths(0), 0, sizeof(FString));   // an empty FString (Insert leaves the slot as it was)
    Paths(0) = HDPath;
    GLog->Logf(TEXT("NCustomGraphics: HD textures on, %s first of %d package paths"), HDPath, Paths.Num());
}

/*-----------------------------------------------------------------------------
    Anisotropic filter: IDirect3DDevice9::SetSamplerState wrapped.
-----------------------------------------------------------------------------*/

typedef HRESULT (STDMETHODCALLTYPE* SetSamplerStateFn)(IDirect3DDevice9*, DWORD, D3DSAMPLERSTATETYPE, DWORD);

static UBOOL GAniso = 0;            // the box's saved state
static DWORD GMaxAniso = 0;         // the device's max anisotropy (up to 16), 0: no anisotropic min filter
static IDirect3DDevice9* GDevice = NULL;
static void** GVtables[MaxHookedVtables];
static SetSamplerStateFn GOriginals[MaxHookedVtables];
static INT GHooked = 0;
// what the engine last asked for each sampler (the device has the adjusted value)
static DWORD GWantMin[MaxSamplers];
static DWORD GWantMax[MaxSamplers];

static SetSamplerStateFn Original(IDirect3DDevice9* Dev)
{
    void** Vtable = *(void***)Dev;
    for (INT i = 0; i < GHooked; i++)
        if (GVtables[i] == Vtable)
            return GOriginals[i];
    return NULL;
}

static DWORD Adjust(D3DSAMPLERSTATETYPE Type, DWORD Value)
{
    if (!GAniso || GMaxAniso < 2)
        return Value;
    if (Type == D3DSAMP_MINFILTER && (Value == D3DTEXF_LINEAR || Value == D3DTEXF_ANISOTROPIC))
        return D3DTEXF_ANISOTROPIC;
    if (Type == D3DSAMP_MAXANISOTROPY)
        return GMaxAniso;
    return Value;
}

static HRESULT STDMETHODCALLTYPE HookSetSamplerState(IDirect3DDevice9* Dev, DWORD Sampler, D3DSAMPLERSTATETYPE Type,
                                                     DWORD Value)
{
    SetSamplerStateFn Orig = Original(Dev);
    if (!Orig)
        return D3DERR_INVALIDCALL;   // not reached: only hooked vtables lead here
    if (Sampler < MaxSamplers)
    {
        if (Type == D3DSAMP_MINFILTER)
            GWantMin[Sampler] = Value;
        else if (Type == D3DSAMP_MAXANISOTROPY)
            GWantMax[Sampler] = Value;
    }
    return Orig(Dev, Sampler, Type, Adjust(Type, Value));
}

// the D3D device of the render device, NULL while there is none or it is not D3DDrv's
static IDirect3DDevice9* CurrentDevice()
{
    static void* D3DVtable = (void*)-1;
    if (D3DVtable == (void*)-1)
    {
        HMODULE D3DDrv = GetModuleHandleW(TEXT("D3DDrv.dll"));
        D3DVtable = D3DDrv ? (void*)GetProcAddress(D3DDrv, "??_7UD3DRenderDevice@@6BUObject@@@") : NULL;
    }
    if (!D3DVtable || !GConsole || !GConsole->Engine || !GConsole->Engine->GRenDev)
        return NULL;
    BYTE* RenDev = (BYTE*)GConsole->Engine->GRenDev;
    if (*(void**)RenDev != D3DVtable)
        return NULL;
    return *(IDirect3DDevice9**)(RenDev + D3DDeviceOffset);
}

// every sampler again, from what the engine asked for
static void ApplyAll(IDirect3DDevice9* Dev)
{
    SetSamplerStateFn Orig = Original(Dev);
    if (!Orig)
        return;
    for (DWORD s = 0; s < MaxSamplers; s++)
    {
        Orig(Dev, s, D3DSAMP_MINFILTER, Adjust(D3DSAMP_MINFILTER, GWantMin[s]));
        Orig(Dev, s, D3DSAMP_MAXANISOTROPY, Adjust(D3DSAMP_MAXANISOTROPY, GWantMax[s]));
    }
}

// a new device: its caps and, when the hook was just put in (the engine set the samplers before it), their state
static void TakeDevice(IDirect3DDevice9* Dev, UBOOL bReadSamplers)
{
    GDevice = Dev;
    D3DCAPS9 Caps;
    GMaxAniso = 0;
    if (SUCCEEDED(Dev->GetDeviceCaps(&Caps)) && (Caps.TextureFilterCaps & D3DPTFILTERCAPS_MINFANISOTROPIC))
        GMaxAniso = Caps.MaxAnisotropy < 16 ? Caps.MaxAnisotropy : 16;
    for (DWORD s = 0; bReadSamplers && s < MaxSamplers; s++)
    {
        DWORD Value;
        GWantMin[s] = SUCCEEDED(Dev->GetSamplerState(s, D3DSAMP_MINFILTER, &Value)) ? Value : D3DTEXF_LINEAR;
        GWantMax[s] = SUCCEEDED(Dev->GetSamplerState(s, D3DSAMP_MAXANISOTROPY, &Value)) ? Value : 1;
    }
    GLog->Logf(TEXT("NCustomGraphics: D3D device %08X, max anisotropy %d, filter %s"), (DWORD)(size_t)Dev, GMaxAniso,
               GAniso ? TEXT("on") : TEXT("off"));
    ApplyAll(Dev);
}

static void HookDevice()
{
    IDirect3DDevice9* Dev = CurrentDevice();
    if (!Dev)
        return;
    void** Vtable = *(void***)Dev;
    UBOOL bNewHook = 0;
    if (!Original(Dev))
    {
        if (GHooked >= MaxHookedVtables)
            return;
        DWORD Old;
        if (!VirtualProtect(&Vtable[SetSamplerStateSlot], sizeof(void*), PAGE_EXECUTE_READWRITE, &Old))
            return;
        GVtables[GHooked] = Vtable;
        GOriginals[GHooked] = (SetSamplerStateFn)Vtable[SetSamplerStateSlot];
        GHooked++;
        Vtable[SetSamplerStateSlot] = (void*)&HookSetSamplerState;
        VirtualProtect(&Vtable[SetSamplerStateSlot], sizeof(void*), Old, &Old);
        bNewHook = 1;
    }
    if (bNewHook || Dev != GDevice)
        TakeDevice(Dev, bNewHook);
}

/*-----------------------------------------------------------------------------
    Every tick.
-----------------------------------------------------------------------------*/

void NCustomGraphics::Tick()
{
    static UBOOL bStarted = 0;
    if (!bStarted && GSys)
    {
        bStarted = 1;
        GAniso = Saved(TEXT("Anisotropy"));
        if (Saved(TEXT("HDTextures")) && HDFolderExists())
            AddHDPath();
    }
    if (bStarted)
        HookDevice();
}

/*-----------------------------------------------------------------------------
    The video page.
-----------------------------------------------------------------------------*/

// at the page's check box column (x 25), on the texture filter box's row (y 287) and the gap under it (the screen shot
// quality combo starts at y 317)
void NCustomGraphics::OnCreate(NCVideoOptionWnd* Wnd)
{
    GAnisoBox = new NCCheckBox(TEXT("Filtro anisotropico"), 0, 0, 0);
    GAnisoBox->ID = NID_VIDEO_ANISOTROPY;
    GAnisoBox->SetPosition(25, 287, 109, 12);
    Wnd->CreateChild(GAnisoBox, NWS_VISIBLE);
    GHDBox = new NCCheckBox(TEXT("Texturas HD (al reiniciar)"), 0, 0, 0);
    GHDBox->ID = NID_VIDEO_HDTEXTURES;
    GHDBox->SetPosition(25, 304, 200, 12);
    Wnd->CreateChild(GHDBox, NWS_VISIBLE);
}

template <class T> static void DestroyBox(NCWnd* Parent, T*& Box)
{
    if (Box)
    {
        Parent->RemoveChildWindow(Box->Entry);
        delete Box;
        Box = NULL;
    }
}

void NCustomGraphics::OnDestroy(NCVideoOptionWnd* Wnd)
{
    DestroyBox(Wnd, GAnisoBox);
    DestroyBox(Wnd, GHDBox);
}

// the page shows the saved choice (a cancelled change is gone); a device without anisotropy or a client without the HD
// pack greys its box out
void NCustomGraphics::OnLoadSettings(NCVideoOptionWnd* Wnd)
{
    if (GAnisoBox)
    {
        GAnisoBox->bChecked = Saved(TEXT("Anisotropy"));
        GAnisoBox->EnableWindow(!GDevice || GMaxAniso >= 2);
    }
    if (GHDBox)
    {
        GHDBox->bChecked = Saved(TEXT("HDTextures"));
        GHDBox->EnableWindow(HDFolderExists());
    }
}

void NCustomGraphics::OnApply(NCVideoOptionWnd* Wnd)
{
    if (!GAnisoBox || !GHDBox)
        return;
    Save(TEXT("Anisotropy"), GAnisoBox->bChecked);
    Save(TEXT("HDTextures"), GHDBox->bChecked);
    UBOOL bAniso = GAnisoBox->bChecked ? 1 : 0;
    if (bAniso != GAniso)
    {
        GAniso = bAniso;
        IDirect3DDevice9* Dev = CurrentDevice();
        if (Dev && Dev == GDevice)
            ApplyAll(Dev);
    }
}
