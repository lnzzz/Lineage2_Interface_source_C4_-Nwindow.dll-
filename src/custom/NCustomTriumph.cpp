/*=============================================================================
    NCustomTriumph.cpp - L2 Ignition: whether the player sees the effects of the Triumph weapons (custom, NW_CUSTOM
    builds only). See NCustomTriumph.h.
=============================================================================*/
#include "nwindow/NCustomTriumph.h"
#include "nwindow/NCCheckBox.h"
#include "nwindow/NCGameOptionWnd.h"
#include "nwindow/NConsoleWnd.h"

enum
{
    NID_GAMEOPT_TRIUMPH_OWN = (INT)0xFF100201,
    NID_GAMEOPT_TRIUMPH_OTHERS = (INT)0xFF100202,
    SendDelay = 3000,   // ms in the world before the choice goes (the server has the character by then)
};

static NCCheckBox* GOwnBox = NULL;
static NCCheckBox* GOthersBox = NULL;
static INT GLastState = -1;     // the console's state the frame before
static DWORD GEnteredAt = 0;    // when the world was entered, 0: nothing to send

/*-----------------------------------------------------------------------------
    Settings: [Triumph] of system\NWindowCustom.ini.
-----------------------------------------------------------------------------*/

static const TCHAR* IniPath()
{
    static TCHAR Path[MAX_PATH] = { 0 };
    if (!Path[0])
    {
        GetModuleFileNameW(NULL, Path, MAX_PATH);
        TCHAR* Slash = wcsrchr(Path, '\\');
        if (Slash)
            Slash[1] = 0;
        wcscat_s(Path, MAX_PATH, TEXT("NWindowCustom.ini"));
    }
    return Path;
}

static UBOOL Saved(const TCHAR* Key)
{
    return GetPrivateProfileIntW(TEXT("Triumph"), Key, 1, IniPath()) != 0;
}

static void Save(const TCHAR* Key, UBOOL bShown)
{
    WritePrivateProfileStringW(TEXT("Triumph"), Key, bShown ? TEXT("1") : TEXT("0"), IniPath());
}

static void Send(UBOOL bOwn, UBOOL bOthers)
{
    TCHAR Bypass[64];
    appSprintf(Bypass, TEXT("triumphfx view %d %d"), bOwn ? 1 : 0, bOthers ? 1 : 0);
    L2ParamStack Params(10);
    Params.PushBack((void*)Bypass);
    GConsole->RequestBypassToServer(Params);
}

/*-----------------------------------------------------------------------------
    The game page.
-----------------------------------------------------------------------------*/

// under the zone title box (y 257), where C4 dropped the screen shot quality box, at the page's check box column (x 25)
void NCustomTriumph::OnCreate(NCGameOptionWnd* Wnd)
{
    GOwnBox = new NCCheckBox(TEXT("Ver efectos de mis armas Triumph"), 1, 0, 0);
    GOwnBox->ID = NID_GAMEOPT_TRIUMPH_OWN;
    GOwnBox->SetPosition(25, 281, 220, 12);
    Wnd->CreateChild(GOwnBox, NWS_VISIBLE);
    GOthersBox = new NCCheckBox(TEXT("Ver efectos de armas Triumph de otros"), 1, 0, 0);
    GOthersBox->ID = NID_GAMEOPT_TRIUMPH_OTHERS;
    GOthersBox->SetPosition(25, 298, 220, 12);
    Wnd->CreateChild(GOthersBox, NWS_VISIBLE);
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

void NCustomTriumph::OnDestroy(NCGameOptionWnd* Wnd)
{
    DestroyBox(Wnd, GOwnBox);
    DestroyBox(Wnd, GOthersBox);
}

// the page shows the saved choice (a cancelled change is gone)
void NCustomTriumph::OnLoadSettings(NCGameOptionWnd* Wnd)
{
    if (GOwnBox)
        GOwnBox->bChecked = Saved(TEXT("ShowOwn"));
    if (GOthersBox)
        GOthersBox->bChecked = Saved(TEXT("ShowOthers"));
}

void NCustomTriumph::OnApply(NCGameOptionWnd* Wnd)
{
    if (!GOwnBox || !GOthersBox)
        return;
    Save(TEXT("ShowOwn"), GOwnBox->bChecked);
    Save(TEXT("ShowOthers"), GOthersBox->bChecked);
    if (GConsole && GConsole->State == L2CS_InGame && GConsole->GetMyUser())
        Send(GOwnBox->bChecked, GOthersBox->bChecked);
}

// after each entry to the world (a new character on the server shows every effect): the saved choice, if it hides any.
// A teleport's loading screen counts as an entry too; sending the same choice again changes nothing on the server.
void NCustomTriumph::Tick()
{
    INT State = GConsole ? GConsole->State : -1;
    if (State == L2CS_InGame && GLastState != L2CS_InGame)
        GEnteredAt = GetTickCount() | 1;
    GLastState = State;
    if (!GEnteredAt || State != L2CS_InGame || !GConsole->GetMyUser() || GetTickCount() - GEnteredAt < SendDelay)
        return;
    GEnteredAt = 0;
    UBOOL bOwn = Saved(TEXT("ShowOwn")), bOthers = Saved(TEXT("ShowOthers"));
    if (!bOwn || !bOthers)
        Send(bOwn, bOthers);
}
