/*=============================================================================
    NCustomAudio.cpp - "Mute All Sounds" in the audio page of the options (custom, NW_CUSTOM builds only). See
    NCustomAudio.h.
=============================================================================*/
#include "nwindow/NCustomAudio.h"
#include "nwindow/NCAudioOptionWnd.h"
#include "nwindow/NCCheckBox.h"
#include "nwindow/NCComboBox.h"
#include "nwindow/NConsoleWnd.h"

enum
{
    NID_AUDIO_MUTEALL = (INT)0xFF100101,
    LevelOff = 5,        // the last item of a volume box ("Off")
    BoxCount = 4,
};

static NCCheckBox* GMuteBox = NULL;
static INT GKept[BoxCount] = { 0, 0, 0, 0 };   // the levels to give back when the box is unchecked

/*-----------------------------------------------------------------------------
    Settings: [Audio] of system\NWindowCustom.ini.
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

static const TCHAR* LevelKey(INT i)
{
    static const TCHAR* Keys[BoxCount] = { TEXT("Level0"), TEXT("Level1"), TEXT("Level2"), TEXT("Level3") };
    return Keys[i];
}

static UBOOL IsMutedSaved()
{
    return GetPrivateProfileIntW(TEXT("Audio"), TEXT("MuteAll"), 0, IniPath()) != 0;
}

static void LoadKept()
{
    for (INT i = 0; i < BoxCount; i++)
    {
        INT Level = GetPrivateProfileIntW(TEXT("Audio"), LevelKey(i), 0, IniPath());
        GKept[i] = Level >= 0 && Level <= LevelOff ? Level : 0;
    }
}

static void Save(UBOOL bMuted)
{
    TCHAR Buffer[16];
    WritePrivateProfileStringW(TEXT("Audio"), TEXT("MuteAll"), bMuted ? TEXT("1") : TEXT("0"), IniPath());
    if (!bMuted)
        return;
    for (INT i = 0; i < BoxCount; i++)
    {
        swprintf_s(Buffer, 16, TEXT("%d"), GKept[i]);
        WritePrivateProfileStringW(TEXT("Audio"), LevelKey(i), Buffer, IniPath());
    }
}

/*-----------------------------------------------------------------------------
    The page.
-----------------------------------------------------------------------------*/

static void Boxes(NCAudioOptionWnd* Wnd, NCComboBox* Out[BoxCount])
{
    Out[0] = Wnd->SoundCombo;
    Out[1] = Wnd->MusicCombo;
    Out[2] = Wnd->WavVoiceCombo;
    Out[3] = Wnd->OggVoiceCombo;
}

// muted: the four boxes show Off, greyed; otherwise they are back in use
static void ShowMuted(NCAudioOptionWnd* Wnd, UBOOL bMuted)
{
    NCComboBox* Combos[BoxCount];
    Boxes(Wnd, Combos);
    for (INT i = 0; i < BoxCount; i++)
    {
        if (!Combos[i])
            continue;
        if (bMuted)
            Combos[i]->SetSelected(LevelOff);
        Combos[i]->EnableWindow(!bMuted);
    }
}

// under the four volume boxes (y 11 .. 85), at the game page's check box column (x 25)
void NCustomAudio::OnCreate(NCAudioOptionWnd* Wnd)
{
    GMuteBox = new NCCheckBox(TEXT("Mute All Sounds"), 0, 0, 0);
    GMuteBox->ID = NID_AUDIO_MUTEALL;
    GMuteBox->SetPosition(25, 95, 150, 12);
    Wnd->CreateChild(GMuteBox, NWS_VISIBLE);
}

void NCustomAudio::OnDestroy(NCAudioOptionWnd* Wnd)
{
    if (GMuteBox)
    {
        Wnd->RemoveChildWindow(GMuteBox->Entry);
        delete GMuteBox;
        GMuteBox = NULL;
    }
}

// the page opens: the box shows the saved state (a cancelled change is gone); without an audio subsystem it is greyed
void NCustomAudio::OnSelectVolumes(NCAudioOptionWnd* Wnd)
{
    if (!GMuteBox)
        return;
    if (!GConsole->Engine->Audio)
    {
        GMuteBox->bChecked = 0;
        GMuteBox->bDisabled = 1;
        return;
    }
    GMuteBox->bDisabled = 0;
    GMuteBox->bChecked = IsMutedSaved();
    if (GMuteBox->bChecked)
        LoadKept();
    ShowMuted(Wnd, GMuteBox->bChecked);
}

// checked: the levels the boxes show are kept and the four go Off; unchecked: the kept levels come back
UBOOL NCustomAudio::OnCommand(NCAudioOptionWnd* Wnd, INT ID)
{
    if (ID != NID_AUDIO_MUTEALL || !GMuteBox)
        return 0;
    NCComboBox* Combos[BoxCount];
    Boxes(Wnd, Combos);
    if (GMuteBox->bChecked)
    {
        for (INT i = 0; i < BoxCount; i++)
            if (Combos[i])
                GKept[i] = Combos[i]->Selected >= 0 && Combos[i]->Selected <= LevelOff ? Combos[i]->Selected : 0;
        ShowMuted(Wnd, 1);
    }
    else
    {
        ShowMuted(Wnd, 0);
        for (INT i = 0; i < BoxCount; i++)
            if (Combos[i])
                Combos[i]->SetSelected(GKept[i]);
    }
    return 1;
}

// OK / Apply: the volumes are set from the boxes already; the state is saved
void NCustomAudio::OnApply(NCAudioOptionWnd* Wnd)
{
    if (!GMuteBox || !GConsole->Engine->Audio)
        return;
    Save(GMuteBox->bChecked);
}
