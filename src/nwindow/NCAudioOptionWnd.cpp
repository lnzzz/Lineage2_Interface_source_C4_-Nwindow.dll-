/*=============================================================================
    NCAudioOptionWnd.cpp - the audio page of the options. Stock: 0x1007da00 .. 0x1007dd6a,
    0x10080210 .. 0x1008096f (WindowProc is NControl's body).
=============================================================================*/
#include "nwindow/NCAudioOptionWnd.h"
#include "nwindow/NCOptionWnd.h"
#include "nwindow/NCComboBox.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"
#if NW_CUSTOM
#include "nwindow/NCustomAudio.h"
#endif

// 0x100cb380
NCAudioOptionWnd::NCAudioOptionWnd()
{
    Unused9C[0] = NULL;
    Unused9C[1] = NULL;
    Unused9C[2] = NULL;
}
NW_HOOK_CTOR(0x100cb380, NCAudioOptionWnd)

// 0x100cb3e0
NW_HOOK_DTOR(0x100cb3e0, NCAudioOptionWnd)

// stock body shared with NControl's (0x10004e90). Custom: the "Mute All Sounds" box (NCustomAudio)
INT NCAudioOptionWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
#if NW_CUSTOM
    if (Msg == NWM_COMMAND)
        return NCustomAudio::OnCommand(this, (INT)wParam) != 0;
#endif
    return 0;
}

// 0x100c9530 (vslot 37) - four volume boxes (sound, music, the two voice ones), 125 x 17 at x 107, 19 px apart from y 11
INT NCAudioOptionWnd::OnCreate()
{
    NCWnd::OnCreate();
    SoundCombo = new NCComboBox;
    SoundCombo->ID = NID_AUDIO_SOUND;
    SoundCombo->SetPosition(107, 11, 125, 17);
    CreateChild(SoundCombo, NWS_VISIBLE);
    MusicCombo = new NCComboBox;
    MusicCombo->ID = NID_AUDIO_MUSIC;
    MusicCombo->SetPosition(107, 30, 125, 17);
    CreateChild(MusicCombo, NWS_VISIBLE);
    WavVoiceCombo = new NCComboBox;
    WavVoiceCombo->ID = NID_AUDIO_WAVVOICE;
    WavVoiceCombo->SetPosition(107, 49, 125, 17);
    CreateChild(WavVoiceCombo, NWS_VISIBLE);
    OggVoiceCombo = new NCComboBox;
    OggVoiceCombo->ID = NID_AUDIO_OGGVOICE;
    OggVoiceCombo->SetPosition(107, 68, 125, 17);
    CreateChild(OggVoiceCombo, NWS_VISIBLE);
#if NW_CUSTOM
    NCustomAudio::OnCreate(this);   // before InitVolumes: SelectVolumes sets the box
#endif
    InitVolumes();
    return 1;
}
NW_HOOK_METHOD(0x100c9530, NCAudioOptionWnd, OnCreate)

// a child window goes (repeated inline in the stock OnDestroy)
template <class T> static FORCEINLINE void DestroyChild(NCWnd* Parent, T*& Child)
{
    if (Child)
    {
        Parent->RemoveChildWindow(Child->Entry);
        delete Child;
        Child = NULL;
    }
}

// 0x100c7170 (vslot 38) - guarded; the four boxes and the three that were never made, then NCWnd's
INT NCAudioOptionWnd::OnDestroy()
{
    guard(NCAudioOptionWnd::OnDestroy);
#if NW_CUSTOM
    NCustomAudio::OnDestroy(this);
#endif
    DestroyChild(this, SoundCombo);
    DestroyChild(this, MusicCombo);
    DestroyChild(this, WavVoiceCombo);
    DestroyChild(this, OggVoiceCombo);
    DestroyChild(this, Unused9C[0]);
    DestroyChild(this, Unused9C[1]);
    DestroyChild(this, Unused9C[2]);
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100c7170, NCAudioOptionWnd, OnDestroy)

// 0x100c6f40 (vslot 62) - C3: the four titles (sysstrings 216, 217, 977, 978: GL2GameData +0x664ac / +0x664b8 / +0x68858
// / +0x68864) right-aligned on x 100, at y 26, 45, 64, 83, in 0xFFDCDCDC
enum { SYSSTR_WavVoice = 977, SYSSTR_OggVoice = 978 };   // names ours
INT NCAudioOptionWnd::OnPaint(NCGDevice* Device)
{
    guard(NCOptionWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    SIZE Size;
    NCGDevice::GetTextExtent(&Size, *SysString(216), 0, 0, 0, 0, 0, L2FONT_Normal);
    Device->DrawText(98 - Size.cx, 14, 0xFFDCDCDC, *SysString(216), 0, 0, 0,
                     L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    NCGDevice::GetTextExtent(&Size, *SysString(217), 0, 0, 0, 0, 0, L2FONT_Normal);
    Device->DrawText(98 - Size.cx, 33, 0xFFDCDCDC, *SysString(217), 0, 0, 0,
                     L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    NCGDevice::GetTextExtent(&Size, *SysString(SYSSTR_WavVoice), 0, 0, 0, 0, 0, L2FONT_Normal);
    Device->DrawText(98 - Size.cx, 52, 0xFFDCDCDC, *SysString(SYSSTR_WavVoice), 0, 0, 0,
                     L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    NCGDevice::GetTextExtent(&Size, *SysString(SYSSTR_OggVoice), 0, 0, 0, 0, 0, L2FONT_Normal);
    Device->DrawText(98 - Size.cx, 71, 0xFFDCDCDC, *SysString(SYSSTR_OggVoice), 0, 0, 0,
                     L2FONT_Normal, 0, 0, 0, 0, 0, 0, 0);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100c6f40, NCAudioOptionWnd, OnPaint)

// 0x100c4240 - the four boxes
void NCAudioOptionWnd::ClearItems()
{
    SoundCombo->ClearItems();
    MusicCombo->ClearItems();
    WavVoiceCombo->ClearItems();
    OggVoiceCombo->ClearItems();
}
NW_HOOK_METHOD(0x100c4240, NCAudioOptionWnd, ClearItems)

// 0x100c47b0 - kept out of line: OnCreate calls it, as in the stock code. C4: the levels are sysstrings now (1233 .. 1237 for
// "5" .. "1") and 228 for "Off", for the four boxes
__declspec(noinline) void NCAudioOptionWnd::InitVolumes()
{
    SoundCombo->AddItem(0x4D1);
    SoundCombo->AddItem(0x4D2);
    SoundCombo->AddItem(0x4D3);
    SoundCombo->AddItem(0x4D4);
    SoundCombo->AddItem(0x4D5);
    SoundCombo->AddItem(0xE4);
    MusicCombo->AddItem(0x4D1);
    MusicCombo->AddItem(0x4D2);
    MusicCombo->AddItem(0x4D3);
    MusicCombo->AddItem(0x4D4);
    MusicCombo->AddItem(0x4D5);
    MusicCombo->AddItem(0xE4);
    WavVoiceCombo->AddItem(0x4D1);
    WavVoiceCombo->AddItem(0x4D2);
    WavVoiceCombo->AddItem(0x4D3);
    WavVoiceCombo->AddItem(0x4D4);
    WavVoiceCombo->AddItem(0x4D5);
    WavVoiceCombo->AddItem(0xE4);
    OggVoiceCombo->AddItem(0x4D1);
    OggVoiceCombo->AddItem(0x4D2);
    OggVoiceCombo->AddItem(0x4D3);
    OggVoiceCombo->AddItem(0x4D4);
    OggVoiceCombo->AddItem(0x4D5);
    OggVoiceCombo->AddItem(0xE4);
    SelectVolumes();
}
NW_HOOK_METHOD(0x100c47b0, NCAudioOptionWnd, InitVolumes)

// the level of a volume: [1, ...) 0, [0.8, 1) 1 ... [0, 0.2) 5 (repeated inline in the
// stock code); a negative volume selects nothing
static FORCEINLINE void SelectLevel(NCComboBox* Combo, FLOAT Volume)
{
    if (0.0f <= Volume && Volume < 0.2f)
        Combo->SetSelected(5);
    else if (0.2f <= Volume && Volume < 0.4f)
        Combo->SetSelected(4);
    else if (0.4f <= Volume && Volume < 0.6f)
        Combo->SetSelected(3);
    else if (0.6f <= Volume && Volume < 0.8f)
        Combo->SetSelected(2);
    else if (0.8f <= Volume && Volume < 1.0f)
        Combo->SetSelected(1);
    else if (1.0f <= Volume)
        Combo->SetSelected(0);
}

// 0x100c4270 - kept out of line: InitVolumes tail-calls it, as in the stock code. C3: the voices too (the last
// combo is selected without the else branches of the others: a negative volume selects nothing, as above)
__declspec(noinline) void NCAudioOptionWnd::SelectVolumes()
{
    if (GConsole->Engine->Audio)
        SelectLevel(SoundCombo, GConsole->Engine->Audio->GetSoundVolume());
    else
        SoundCombo->EnableWindow(0);
    if (GConsole->Engine->Audio)
        SelectLevel(MusicCombo, GConsole->Engine->Audio->GetMusicVolume());
    else
        MusicCombo->EnableWindow(0);
    if (GConsole->Engine->Audio)
        SelectLevel(WavVoiceCombo, GConsole->Engine->Audio->GetWavVoiceVolume());
    else
        WavVoiceCombo->EnableWindow(0);
    if (GConsole->Engine->Audio)
        SelectLevel(OggVoiceCombo, GConsole->Engine->Audio->GetOggVoiceVolume());
    else
        OggVoiceCombo->EnableWindow(0);
#if NW_CUSTOM
    NCustomAudio::OnSelectVolumes(this);
#endif
}
NW_HOOK_METHOD(0x100c4270, NCAudioOptionWnd, SelectVolumes)

// 0x100c7320 - Off also silences what plays (StopSound, StopAllMusic, StopAllVoice). Music back on from Off restarts
// it: the player controller's ResetMusicControll (reached through its pawn), then the lobby music is restored.
// C3: the two voice volumes (Option.ini WavVoiceVolume, OggVoiceVolume)
void NCAudioOptionWnd::Apply()
{
    TCHAR Buffer[256];
    if (GConsole->Engine->Audio)
    {
        switch (SoundCombo->Selected)
        {
        case 0:
            GConsole->Engine->Audio->SetSoundVolume(1.0f);
            break;
        case 1:
            GConsole->Engine->Audio->SetSoundVolume(0.8f);
            break;
        case 2:
            GConsole->Engine->Audio->SetSoundVolume(0.6f);
            break;
        case 3:
            GConsole->Engine->Audio->SetSoundVolume(0.4f);
            break;
        case 4:
            GConsole->Engine->Audio->SetSoundVolume(0.2f);
            break;
        case 5:
            GConsole->Engine->Audio->SetSoundVolume(0.0f);
            GConsole->Engine->Audio->StopSound((AActor*)NULL, (USound*)NULL);
            break;
        }
        appSprintf(Buffer, TEXT("%f"), GConsole->Engine->Audio->GetSoundVolume());
        GConfig->SetString(TEXT("Audio"), TEXT("SoundVolume"), Buffer, TEXT("Option.ini"));
    }
    if (GConsole->Engine->Audio)
    {
        INT Level = MusicCombo->Selected;
        FLOAT OldVolume = GConsole->Engine->Audio->GetMusicVolume();
        switch (Level)
        {
        case 0:
            GConsole->Engine->Audio->SetMusicVolume(1.0f);
            break;
        case 1:
            GConsole->Engine->Audio->SetMusicVolume(0.8f);
            break;
        case 2:
            GConsole->Engine->Audio->SetMusicVolume(0.6f);
            break;
        case 3:
            GConsole->Engine->Audio->SetMusicVolume(0.4f);
            break;
        case 4:
            GConsole->Engine->Audio->SetMusicVolume(0.2f);
            break;
        case 5:
            GConsole->Engine->Audio->SetMusicVolume(0.0f);
            GConsole->Engine->Audio->StopAllMusic(0.0f, 0);
            break;
        }
        if (GConsole->Engine->Audio->GetMusicVolume() > 0.0f && OldVolume == 0.0f)
        {
            APlayerController* Controller = GConsole->Engine->Client->Viewports(0)->Actor;
            if (Controller)
            {
                APawn* Pawn = Controller->Pawn;
                if (Pawn)
                {
                    AController* PawnController = Pawn->Controller;
                    if (PawnController)
                        PawnController->ResetMusicControll();
                }
            }
            GConsole->RestoreLobbyMusic();
        }
        appSprintf(Buffer, TEXT("%f"), GConsole->Engine->Audio->GetMusicVolume());
        GConfig->SetString(TEXT("Audio"), TEXT("MusicVolume"), Buffer, TEXT("Option.ini"));
    }
    if (GConsole->Engine->Audio)
    {
        switch (WavVoiceCombo->Selected)
        {
        case 0:
            GConsole->Engine->Audio->SetWavVoiceVolume(1.0f);
            break;
        case 1:
            GConsole->Engine->Audio->SetWavVoiceVolume(0.8f);
            break;
        case 2:
            GConsole->Engine->Audio->SetWavVoiceVolume(0.6f);
            break;
        case 3:
            GConsole->Engine->Audio->SetWavVoiceVolume(0.4f);
            break;
        case 4:
            GConsole->Engine->Audio->SetWavVoiceVolume(0.2f);
            break;
        case 5:
            GConsole->Engine->Audio->SetWavVoiceVolume(0.0f);
            GConsole->Engine->Audio->StopAllVoice(0.0f, 0);
            break;
        }
        appSprintf(Buffer, TEXT("%f"), GConsole->Engine->Audio->GetWavVoiceVolume());
        GConfig->SetString(TEXT("Audio"), TEXT("WavVoiceVolume"), Buffer, TEXT("Option.ini"));
    }
    if (GConsole->Engine->Audio)
    {
        switch (OggVoiceCombo->Selected)
        {
        case 0:
            GConsole->Engine->Audio->SetOggVoiceVolume(1.0f);
            break;
        case 1:
            GConsole->Engine->Audio->SetOggVoiceVolume(0.8f);
            break;
        case 2:
            GConsole->Engine->Audio->SetOggVoiceVolume(0.6f);
            break;
        case 3:
            GConsole->Engine->Audio->SetOggVoiceVolume(0.4f);
            break;
        case 4:
            GConsole->Engine->Audio->SetOggVoiceVolume(0.2f);
            break;
        case 5:
            GConsole->Engine->Audio->SetOggVoiceVolume(0.0f);
            GConsole->Engine->Audio->StopAllVoice(0.0f, 1);
            break;
        }
        appSprintf(Buffer, TEXT("%f"), GConsole->Engine->Audio->GetOggVoiceVolume());
        GConfig->SetString(TEXT("Audio"), TEXT("OggVoiceVolume"), Buffer, TEXT("Option.ini"));
    }
#if NW_CUSTOM
    NCustomAudio::OnApply(this);
#endif
}
NW_HOOK_METHOD(0x100c7320, NCAudioOptionWnd, Apply)

// 0x100c4940 (vslot 48) - the options window (the parent) takes the backdrop of this page
void NCAudioOptionWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    ((NCOptionWnd*)Entry->Parent->Wnd)->OptionBackTex = FindTexture(TEXT("L2ui_ch3.OptionWnd.option2_back"));
}
NW_HOOK_METHOD(0x100c4940, NCAudioOptionWnd, ShowWindow)

NW_CHECK_VTABLE(NCAudioOptionWnd, 0x101c0380, 74)
