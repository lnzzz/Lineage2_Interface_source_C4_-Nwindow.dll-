/*=============================================================================
    NCVideoOptionWnd.cpp - the video page of the options. C4 (read from the stock 0x100C3F90 .. 0x100C99E0): the color
    cursor and glow boxes are gone, a post processing combo and a screen shot quality combo are new, the headings moved,
    the system strings of the lists are ids (AddItem(INT)) and the render device's SetRes at slot 27 takes SIX integers
    after the viewport (the header's slot 27 is right; slot 28 takes five).
=============================================================================*/
#include "nwindow/NCVideoOptionWnd.h"
#include "nwindow/NCOptionWnd.h"
#include "nwindow/NCComboBox.h"
#include "nwindow/NCCheckBox.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

#if NW_FEATURES
// phase 6: WindowFix (the "Full Screen" box, docs/notas/F6_WindowFix.md)
#include "features/WindowFix.h"
#endif

// 0x100cb2d0 - NCWnd's fields by the base constructor; the three spare pointers
NCVideoOptionWnd::NCVideoOptionWnd()
{
    UnusedD0[0] = NULL;
    UnusedD0[1] = NULL;
    UnusedD0[2] = NULL;
}
NW_HOOK_CTOR(0x100cb2d0, NCVideoOptionWnd)

// 0x100cb330
NW_HOOK_DTOR(0x100cb330, NCVideoOptionWnd)

// 0x100c3f90 (vslot 25) - C4: the post processing combo reports through slot 76 (OnPostProcessChanged)
INT NCVideoOptionWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_VIDEO_RESOLUTION)
            return OnResolutionChanged(lParam) != 0;
        if (wParam == NID_VIDEO_REFRESH)
            return OnRefreshChanged(lParam) != 0;
        if (wParam == NID_VIDEO_POSTPROCESS)
            return ::nw::VirtualCall<INT>(this, 76, (INT)lParam) != 0;
        if (wParam == (WPARAM)NID_VIDEO_LOWERDETAIL)
            return OnLowerDetailClicked() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x100c3f90, NCVideoOptionWnd, WindowProc)

// 0x100c4050 (slot 76, name ours) - a post processing kind of 3 .. 5 takes the anti-aliasing out (selection 0, box
// disabled); any other kind frees it
INT NCVideoOptionWnd::OnPostProcessChanged(INT Index)
{
    if (Index >= 3 && Index <= 5)
    {
        AntialiasCombo->SetSelected(0);
        AntialiasCombo->EnableWindow(0);
        return 1;
    }
    AntialiasCombo->EnableWindow(1);
    return 1;
}
NW_HOOK_METHOD(0x100c4050, NCVideoOptionWnd, OnPostProcessChanged)

// one combo box of the page (repeated inline in the stock OnCreate): 125 x 17 at x 107
static FORCEINLINE NCComboBox* NewCombo(NCWnd* Parent, INT ID, INT Y, INT Flags = NWS_VISIBLE)
{
    NCComboBox* Combo = new NCComboBox;
    Combo->ID = ID;
    Combo->SetPosition(107, Y, 125, 17);
    Parent->CreateChild(Combo, Flags);
    return Combo;
}

// one check box, labelled by its sysstring line (the stock constructor 0x100058C0: the label id, unchecked, label on the
// right, no text parameter: four arguments)
static FORCEINLINE NCCheckBox* NewBox(NCWnd* Parent, INT LabelID, INT ID, INT X, INT Y, INT W)
{
    NCCheckBox* Box = new NCCheckBox(LabelID, 0, 0, 0);
    Box->ID = ID;
    Box->SetPosition(X, Y, W, 17);
    Parent->CreateChild(Box, NWS_VISIBLE);
    return Box;
}

// 0x100c99e0 (vslot 37) - C4: 125 x 17 combos at x 107 (the resolution one and the new post processing and screen shot
// ones with the style bit 0x10); the boxes name themselves (the page draws no labels for them), the lower detail box on the
// left under the range combos, the shadow and ground detail boxes side by side and the texture filter box on the right
// of them. The order of creation is the stock's (it is not the order of the fields)
INT NCVideoOptionWnd::OnCreate()
{
    NCWnd::OnCreate();
    ResolutionCombo = NewCombo(this, NID_VIDEO_RESOLUTION, 11, NWS_VISIBLE | 0x10);   // 0x10: meaning unknown
    RefreshCombo = NewCombo(this, NID_VIDEO_REFRESH, 30);
    GammaCombo = NewCombo(this, NID_VIDEO_GAMMA, 49);
    TextureCombo = NewCombo(this, NID_VIDEO_TEXTURE, 68);
    ModelingCombo = NewCombo(this, NID_VIDEO_MODELING, 87);
    AnimationCombo = NewCombo(this, NID_VIDEO_ANIMATION, 106);
    TerrainRangeCombo = NewCombo(this, NID_VIDEO_TERRAINRANGE, 139);
    PawnRangeCombo = NewCombo(this, NID_VIDEO_PAWNRANGE, 158);
    ActorLimitCombo = NewCombo(this, NID_VIDEO_ACTORLIMIT, 177);
    LowerDetailBox = NewBox(this, 370, NID_VIDEO_LOWERDETAIL, 25, 199, 100);
    ReflectionCombo = NewCombo(this, NID_VIDEO_REFLECTION, 229);
    AntialiasCombo = NewCombo(this, NID_VIDEO_ANTIALIAS, 248);
    ScreenShotCombo = NewCombo(this, NID_VIDEO_SCREENSHOT, 317, NWS_VISIBLE | 0x10);
    PostProcessCombo = NewCombo(this, NID_VIDEO_POSTPROCESS, 336, NWS_VISIBLE | 0x10);
    ShadowBox = NewBox(this, 212, NID_VIDEO_SHADOW, 25, 270, 109);
    GroundDetailBox = NewBox(this, 213, NID_VIDEO_GROUNDDETAIL, 135, 270, 109);
    TextureFilterBox = NewBox(this, 464, NID_VIDEO_TEXTUREFILTER, 135, 287, 100);
    InitItems();
    return 1;
}
NW_HOOK_METHOD(0x100c99e0, NCVideoOptionWnd, OnCreate)

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

// 0x100c5190 (vslot 38) - guarded; the children in the stock's order (the three spare ones in the middle of the end)
INT NCVideoOptionWnd::OnDestroy()
{
    guard(NCVideoOptionWnd::OnDestroy);
    DestroyChild(this, ResolutionCombo);
    DestroyChild(this, RefreshCombo);
    DestroyChild(this, GammaCombo);
    DestroyChild(this, TextureCombo);
    DestroyChild(this, ModelingCombo);
    DestroyChild(this, AnimationCombo);
    DestroyChild(this, ShadowBox);
    DestroyChild(this, GroundDetailBox);
    DestroyChild(this, PostProcessCombo);
    DestroyChild(this, TerrainRangeCombo);
    DestroyChild(this, PawnRangeCombo);
    DestroyChild(this, ActorLimitCombo);
    DestroyChild(this, ReflectionCombo);
    DestroyChild(this, LowerDetailBox);
    DestroyChild(this, TextureFilterBox);
    DestroyChild(this, UnusedD0[0]);
    DestroyChild(this, UnusedD0[1]);
    DestroyChild(this, UnusedD0[2]);
    DestroyChild(this, AntialiasCombo);
    DestroyChild(this, ScreenShotCombo);
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100c5190, NCVideoOptionWnd, OnDestroy)

// a heading right-aligned on x 98, in the skin's text color (repeated inline in the stock OnPaint)
static FORCEINLINE void DrawLabel(NCGDevice* Device, INT TextID, INT Y)
{
    SIZE Size;
    NCGDevice::GetTextExtent(&Size, *GL2GameData.SysString[TextID], 0, 0, 0, 0, 0, L2FONT_Normal);
    Device->DrawText(98 - Size.cx, Y, 0xFFDCDCDC, *GL2GameData.SysString[TextID], 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0,
                     0, 0);
}

// the same, when the text pointer is not NULL
static FORCEINLINE void DrawOptionLabel(NCGDevice* Device, INT TextID, INT Y)
{
    if (*GL2GameData.SysString[TextID])
        DrawLabel(Device, TextID, Y);
}

// 0x100c4b40 (vslot 62) - guarded; C4: the headings of the first nine controls are drawn when they have a text (the
// resolution at y 14 .. the PC/NPC limit at 180), the others unconditionally (reflections 232, anti-aliasing 251, the new
// screen shot quality 320 and post processing 339)
INT NCVideoOptionWnd::OnPaint(NCGDevice* Device)
{
    guard(NCVideoOptionWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    DrawOptionLabel(Device, 209, 14);    // Resolution
    DrawOptionLabel(Device, 351, 33);    // Refresh Rate
    DrawOptionLabel(Device, 210, 52);    // Gamma
    DrawOptionLabel(Device, 211, 71);    // Texture
    DrawOptionLabel(Device, 594, 90);    // Modeling Detail
    DrawOptionLabel(Device, 607, 109);   // Animation
    DrawOptionLabel(Device, 214, 142);   // Terrain Range
    DrawOptionLabel(Device, 215, 161);   // PC/NPC Range
    DrawOptionLabel(Device, 540, 180);   // PC/NPC Limit
    DrawLabel(Device, 352, 232);         // Reflections
    DrawLabel(Device, 868, 251);         // Anti-Aliasing
    DrawLabel(Device, 979, 320);         // (C4) the screen shot quality
    DrawLabel(Device, 1229, 339);        // (C4) the post processing
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100c4b40, NCVideoOptionWnd, OnPaint)

// 0x100c5650 (vslot 74) - the rates of the picked mode; the one in use stays selected, else the last
INT NCVideoOptionWnd::OnResolutionChanged(INT Index)
{
    INT Widths[100], Heights[100], Bits[100], Rates[100];
    // the stock buffer is 256 TCHARs and the stock spills Bits[Index] to the frame; ours keeps it in a register, so
    // two more TCHARs bring the frame back to the stock size plus the /GS cookie (tools/port/check_frames.py)
    TCHAR Text[258];
    INT Rate = GRefreshRate;
    INT ModeCount = GConsole->Engine->GRenDev->GetRes(100, Widths, Heights, Bits);
    if (Index >= 0 && Index < ModeCount)
    {
        INT RateIndex = RefreshCombo->Selected;
        INT RateCount = GConsole->Engine->GRenDev->GetRefreshRate(100, Widths[Index], Heights[Index], Bits[Index],
                                                                  Rates);
        if (RateIndex >= 0 && RateIndex < RateCount)
            Rate = Rates[RateIndex];
        RefreshCombo->ClearItems();
        for (INT i = 0; i < RateCount; i++)
        {
            appSprintf(Text, TEXT("%dHz"), Rates[i]);
            RefreshCombo->AddItem(Text);
        }
        INT j;
        for (j = 0; j < RateCount; j++)
            if (Rates[j] == Rate)
            {
                RefreshCombo->SetSelected(j);
                break;
            }
        if (j == RateCount)
            RefreshCombo->SetSelected(RateCount - 1);
    }
    return 1;
}
NW_HOOK_METHOD(0x100c5650, NCVideoOptionWnd, OnResolutionChanged)

// 0x100c6d60 (vslot 77) - "Lower Detail" fixes the detail controls (and the post processing and the reflections off);
// free again, the reflections still need pixel shader 1.2 and the post processing the vertex and pixel shaders 2.0.
// The stock's last branch (shaders under 2.0) clears the REFLECTION selection, not the post processing one: kept
INT NCVideoOptionWnd::OnLowerDetailClicked()
{
    if (LowerDetailBox->bChecked)
    {
        TextureCombo->EnableWindow(0);
        ModelingCombo->EnableWindow(0);
        AnimationCombo->EnableWindow(0);
        ShadowBox->bDisabled = 1;
        GroundDetailBox->bDisabled = 1;
        PostProcessCombo->EnableWindow(0);
        PostProcessCombo->SetSelected(0);
        TerrainRangeCombo->EnableWindow(0);
        PawnRangeCombo->EnableWindow(0);
        ReflectionCombo->EnableWindow(0);
        ReflectionCombo->SetSelected(0);
        return 1;
    }
    TextureCombo->EnableWindow(1);
    ModelingCombo->EnableWindow(1);
    AnimationCombo->EnableWindow(1);
    ShadowBox->bDisabled = 0;
    GroundDetailBox->bDisabled = 0;
    TerrainRangeCombo->EnableWindow(1);
    PawnRangeCombo->EnableWindow(1);
    if (GConsole->Engine->GRenDev->GetRenderCaps()->PixelShaderVersion >= 12)
        ReflectionCombo->EnableWindow(1);
    else
    {
        ReflectionCombo->EnableWindow(0);
        ReflectionCombo->SetSelected(0);
    }
    INT VertexShader, PixelShader;   // names ours
    GConsole->Engine->GRenDev->GetShaderVersion(VertexShader, PixelShader);
    if (VertexShader >= 20 && PixelShader >= 20)
    {
        PostProcessCombo->EnableWindow(1);
        return 1;
    }
    PostProcessCombo->EnableWindow(0);
    ReflectionCombo->SetSelected(0);
    return 1;
}
NW_HOOK_METHOD(0x100c6d60, NCVideoOptionWnd, OnLowerDetailClicked)

// 0x100c40a0 - the modes and rates of the device, the one in use selected
void NCVideoOptionWnd::InitModes()
{
    INT Widths[100], Heights[100], Bits[100], Rates[100];
    TCHAR Text[100];
    ResolutionCombo->ClearItems();
    RefreshCombo->ClearItems();
    INT Count = GConsole->Engine->GRenDev->GetRes(100, Widths, Heights, Bits);
    for (INT i = 0; i < Count; i++)
    {
        appSprintf(Text, TEXT("%d*%d %dbit"), Widths[i], Heights[i], Bits[i]);
        ResolutionCombo->AddItem(Text);
        if (GSEKScreenX == Widths[i] && GSEKScreenY == Heights[i] && GColorBits == Bits[i])
            ResolutionCombo->SetSelected(i);
    }
    Count = GConsole->Engine->GRenDev->GetRefreshRate(100, GSEKScreenX, GSEKScreenY, GColorBits, Rates);
    for (INT j = 0; j < Count; j++)
    {
        appSprintf(Text, TEXT("%dHz"), Rates[j]);
        RefreshCombo->AddItem(Text);
        if (GRefreshRate == Rates[j])
            RefreshCombo->SetSelected(j);
    }
}
NW_HOOK_METHOD(0x100c40a0, NCVideoOptionWnd, InitModes)

// a check box follows its setting (repeated inline in the stock code)
static FORCEINLINE void SetBox(NCCheckBox* Box, UBOOL bValue)
{
    if (bValue)
        Box->bChecked = 1;
    else
        Box->bChecked = 0;
}

// 0x100c57a0 - the controls follow the globals. C4: the post processing combo (an out of range kind is reset to 0 in
// the global too), no color cursor box, "Lower Detail" fixes the detail controls (and the post processing off), and at the
// end the anti-aliasing level (only with a multisampling device and a post processing kind that is not 3 .. 5)
void NCVideoOptionWnd::LoadSettings()
{
    INT Widths[100], Heights[100], Bits[100], Rates[100];
    INT Count = GConsole->Engine->GRenDev->GetRes(100, Widths, Heights, Bits);
    for (INT i = 0; i < Count; i++)
        if (GSEKScreenX == Widths[i] && GSEKScreenY == Heights[i] && GColorBits == Bits[i])
            ResolutionCombo->SetSelected(i);
    Count = GConsole->Engine->GRenDev->GetRefreshRate(100, GSEKScreenX, GSEKScreenY, GColorBits, Rates);
    for (INT j = 0; j < Count; j++)
        if (GRefreshRate == Rates[j])
            RefreshCombo->SetSelected(j);

    UClient* Client = GConsole->Engine->Client;
    if (Client)
    {
        FLOAT Gamma = Client->Gamma;
        if (Gamma >= 1.2f)
            GammaCombo->SetSelected(0);
        else if (Gamma >= 1.0f && Gamma < 1.2f)
            GammaCombo->SetSelected(1);
        else if (Gamma >= 0.8f && Gamma < 1.0f)
            GammaCombo->SetSelected(2);
        else if (Gamma >= 0.6f && Gamma < 0.8f)
            GammaCombo->SetSelected(3);
        else if (Gamma < 0.6f)
            GammaCombo->SetSelected(4);
    }

    if (GL2OriginPawnCR < 1.5f)
        PawnRangeCombo->SetSelected(4);
    else if (GL2OriginPawnCR >= 1.5f && GL2OriginPawnCR < 2.0f)
        PawnRangeCombo->SetSelected(3);
    else if (GL2OriginPawnCR >= 2.0f && GL2OriginPawnCR < 2.5f)
        PawnRangeCombo->SetSelected(2);
    else if (GL2OriginPawnCR >= 2.5f && GL2OriginPawnCR < 3.0f)
        PawnRangeCombo->SetSelected(1);
    else if (GL2OriginPawnCR >= 3.0f)
        PawnRangeCombo->SetSelected(0);

    if (GL2OriginTerrainCR < 5.0f)
        TerrainRangeCombo->SetSelected(4);
    else if (GL2OriginTerrainCR >= 5.0f && GL2OriginTerrainCR < 6.0f)
        TerrainRangeCombo->SetSelected(3);
    else if (GL2OriginTerrainCR >= 6.0f && GL2OriginTerrainCR < 7.0f)
        TerrainRangeCombo->SetSelected(2);
    else if (GL2OriginTerrainCR >= 7.0f && GL2OriginTerrainCR < 8.0f)
        TerrainRangeCombo->SetSelected(1);
    else if (GL2OriginTerrainCR >= 8.0f)
        TerrainRangeCombo->SetSelected(0);

    SetBox(GroundDetailBox, GL2RenderDeco);
    INT PostProcess = GL2PostProcessType;
    if (PostProcess < 0 || PostProcess > 5)
    {
        PostProcessCombo->SetSelected(0);
        GL2PostProcessType = 0;
    }
    else
        PostProcessCombo->SetSelected(PostProcess);
    SetBox(ShadowBox, GIsL2Projector);
    INT Texture = GL2TextureDetail;
    if (Texture >= 2)
        Texture = 2;
    else if (Texture <= 0)
        Texture = 0;
    TextureCombo->SetSelected(Texture);
    ModelingCombo->SetSelected(GL2UseReducePawnLOD != 0);
    AnimationCombo->SetSelected(GL2UseSkipAnim != 0);
    if (GIsL2WaterEffect && GIsL2WaterEffectType == 1)
        ReflectionCombo->SetSelected(1);
    else if (GIsL2WaterEffect && GIsL2WaterEffectType == 2)
        ReflectionCombo->SetSelected(2);
    else
        ReflectionCombo->SetSelected(0);
    SetBox(TextureFilterBox, GL2UseTrilinear);
    if (GConsole->Engine->GRenDev->GetRenderCaps()->PixelShaderVersion < 12)
    {
        ReflectionCombo->EnableWindow(0);
        ReflectionCombo->SetSelected(0);
    }
    else
        ReflectionCombo->EnableWindow(1);

    if (GL2RenderActorLimitOpt >= 6)
        ActorLimitCombo->SetSelected(0);
    else if (GL2RenderActorLimitOpt == 5)
        ActorLimitCombo->SetSelected(1);
    else if (GL2RenderActorLimitOpt == 4)
        ActorLimitCombo->SetSelected(2);
    else if (GL2RenderActorLimitOpt == 3)
        ActorLimitCombo->SetSelected(3);
    else if (GL2RenderActorLimitOpt == 2)
        ActorLimitCombo->SetSelected(4);
    else if (GL2RenderActorLimitOpt <= 1)
        ActorLimitCombo->SetSelected(5);

    if (GL2KeepMinFrameRate)
    {
        LowerDetailBox->bChecked = 1;
        TextureCombo->EnableWindow(0);
        ModelingCombo->EnableWindow(0);
        AnimationCombo->EnableWindow(0);
        ShadowBox->bDisabled = 1;
        GroundDetailBox->bDisabled = 1;
        TerrainRangeCombo->EnableWindow(0);
        PawnRangeCombo->EnableWindow(0);
        ReflectionCombo->EnableWindow(0);
        ReflectionCombo->SetSelected(0);
        PostProcessCombo->EnableWindow(0);
        PostProcessCombo->SetSelected(0);
    }
    else
    {
        LowerDetailBox->bChecked = 0;
        TextureCombo->EnableWindow(1);
        ModelingCombo->EnableWindow(1);
        AnimationCombo->EnableWindow(1);
        ShadowBox->bDisabled = 0;
        GroundDetailBox->bDisabled = 0;
        TerrainRangeCombo->EnableWindow(1);
        PawnRangeCombo->EnableWindow(1);
        if (GConsole->Engine->GRenDev->GetRenderCaps()->PixelShaderVersion >= 12)
            ReflectionCombo->EnableWindow(1);
        else
        {
            ReflectionCombo->EnableWindow(0);
            ReflectionCombo->SetSelected(0);
        }
        INT VertexShader, PixelShader;   // names ours
        GConsole->Engine->GRenDev->GetShaderVersion(VertexShader, PixelShader);
        if (VertexShader >= 20 && PixelShader >= 20)
            PostProcessCombo->EnableWindow(1);
        else
        {
            PostProcessCombo->SetSelected(0);
            PostProcessCombo->EnableWindow(0);
        }
    }
    if (GConsole->Engine->GRenDev->GetMultiSample() > 0 && (GL2PostProcessType < 3 || GL2PostProcessType > 5))
    {
        AntialiasCombo->SetSelected(GL2Antialiasing);
        AntialiasCombo->EnableWindow(1);
        return;
    }
    AntialiasCombo->SetSelected(0);
    AntialiasCombo->EnableWindow(0);
}
NW_HOOK_METHOD(0x100c57a0, NCVideoOptionWnd, LoadSettings)

#if NW_FEATURES
// phase 6 - the "Full Screen" box follows the viewport, not the last click: Alt+Enter can have
// switched the mode since the page was built, and OK would otherwise switch it back
void NCVideoOptionWnd::SyncModeBox()
{
    NCCheckBox* Box = static_cast<NCCheckBox*>(UnusedD0[0]);
    if (Box)
        Box->bChecked = nwf::WindowFix_IsFullscreen(Box->bChecked != 0);
}
#endif

// a setting saved as its number (repeated inline in the stock Apply)
static FORCEINLINE void SaveInt(const TCHAR* Section, const TCHAR* Key, INT Value)
{
    GConfig->SetString(Section, Key, appItoa(Value), TEXT("Option.ini"));
}

// a setting saved "%f" through the caller's buffer (repeated inline in the stock Apply)
static FORCEINLINE void SaveFloat(TCHAR* Text, const TCHAR* Section, const TCHAR* Key, FLOAT Value)
{
    appSprintf(Text, TEXT("%f"), Value);
    GConfig->SetString(Section, Key, Text, TEXT("Option.ini"));
}

// the fog of a terrain range step (env.int [FOG]), with its defaults (repeated inline in the stock Apply)
static FORCEINLINE void LoadFogRange(const TCHAR* StartKey, const TCHAR* EndKey, FLOAT DefaultStart, FLOAT DefaultEnd,
                                     FLOAT& Start, FLOAT& End)
{
    if (!GConfig->GetFloat(TEXT("FOG"), StartKey, Start, TEXT("env.int")))
        Start = DefaultStart;
    if (!GConfig->GetFloat(TEXT("FOG"), EndKey, End, TEXT("env.int")))
        End = DefaultEnd;
}

// 0x100c5e70 - guarded; the page saved into the globals, the device and Option.ini. C4: no color cursor, the post processing
// kind (Video / PostProc) after the PC/NPC range, no glow, and the screen shot quality (Game / ScreenShotQuality) at the end;
// SetRes of the device's slot 27 takes the viewport and SIX integers (the header is right: D3DDrv's slot 27 is the six-INT
// SetRes, ret 0x1C), the slot 28 one the viewport and five
void NCVideoOptionWnd::Apply()
{
    guard(NCVideoOptionWnd::Apply);
    TCHAR Text[260];
    UBOOL bFullscreen = GConsole->Engine->Client->Viewports(0)->IsFullscreen();

    INT OldTextureDetail = GL2TextureDetail;
    if (GL2KeepMinFrameRate)
        OldTextureDetail = 2;
    if (!LowerDetailBox->bChecked)
    {
        GConfig->SetString(TEXT("Video"), TEXT("IsKeepMinFrameRate"), TEXT("false"), TEXT("Option.ini"));
        GL2KeepMinFrameRate = 0;
    }
    else
    {
        GConfig->SetString(TEXT("Video"), TEXT("IsKeepMinFrameRate"), TEXT("true"), TEXT("Option.ini"));
        GL2KeepMinFrameRate = 1;
    }

    UBOOL bOldTrilinear = GConsole->Engine->GRenDev->IsUseTrilinear();
    if (!TextureFilterBox->bChecked)
    {
        GL2UseTrilinear = 0;
        GConfig->SetString(TEXT("Video"), TEXT("UseTrilinear"), TEXT("false"), TEXT("Option.ini"));
    }
    else
    {
        GL2UseTrilinear = 1;
        GConfig->SetString(TEXT("Video"), TEXT("UseTrilinear"), TEXT("true"), TEXT("Option.ini"));
    }
    if (bOldTrilinear != GL2UseTrilinear)
        GConsole->Engine->GRenDev->SetUseTrilinear(GL2UseTrilinear);

    INT Texture = TextureCombo->Selected;
    if (Texture == 0)
        GL2TextureDetail = 0;
    else if (Texture == 1)
        GL2TextureDetail = 1;
    else
        GL2TextureDetail = 2;
    SaveInt(TEXT("Video"), TEXT("TextureDetail"), GL2TextureDetail);
    INT NewTextureDetail = GL2TextureDetail;
    if (GL2KeepMinFrameRate)
        NewTextureDetail = 2;
    if (OldTextureDetail != NewTextureDetail)
        GConsole->Engine->GRenDev->DeleteAllTexture();

    GL2UseReducePawnLOD = ModelingCombo->Selected != 0;
    SaveInt(TEXT("Video"), TEXT("ModelDetail"), GL2UseReducePawnLOD);
    GL2UseSkipAnim = AnimationCombo->Selected != 0;
    SaveInt(TEXT("Video"), TEXT("SkipAnim"), GL2UseSkipAnim);

    INT Widths[100], Heights[100], Bits[100], Rates[100];
    INT Mode = ResolutionCombo->Selected;
    INT ModeCount = GConsole->Engine->GRenDev->GetRes(100, Widths, Heights, Bits);
    if (Mode >= 0 && Mode < ModeCount)
    {
        INT RateIndex = RefreshCombo->Selected;
        INT NewX = Widths[Mode];
        INT NewY = Heights[Mode];
        INT NewBits = Bits[Mode];
        INT RateCount = GConsole->Engine->GRenDev->GetRefreshRate(100, NewX, NewY, NewBits, Rates);
        if (RateIndex >= 0 && RateIndex < RateCount)
        {
            INT Rate = Rates[RateIndex];
            if (GSEKScreenX != NewX || GSEKScreenY != NewY || GColorBits != NewBits || GRefreshRate != Rate)
            {
                // slot 27 is SetRes(UViewport*, six INTs) (D3DDrv.dll: ?SetRes@UD3DRenderDevice@@UAEHPAVUViewport@@HHHHHH@Z, ret 0x1C;
                // the stock pushes seven values here, the last one is 1; slot 28 is the five-INT one, ret 0x18). The wave B text
                // passed six values (no trailing 1) through VirtualCall: the callee pops 0x1C, so the stack ended 4 bytes high
                if (GConsole->Engine->GRenDev->SetRes(GConsole->Engine->Client->Viewports(0), NewX, NewY, NewBits / 8, Rate,
                                                      bFullscreen, 1))
                {
                    GColorBits = NewBits;
                    GRefreshRate = Rate;
                }
                else if (GConsole->Engine->GRenDev->SetRes(GConsole->Engine->Client->Viewports(0), 1024, 768, 4, 60,
                                                           bFullscreen, 1))
                {
                    GColorBits = 32;
                    GRefreshRate = 60;
                    InitModes();
                }
            }
            SaveInt(TEXT("Video"), TEXT("GamePlayViewportX"), GSEKScreenX);
            SaveInt(TEXT("Video"), TEXT("GamePlayViewportY"), GSEKScreenY);
            SaveInt(TEXT("Video"), TEXT("ColorBits"), GColorBits);
            SaveInt(TEXT("Video"), TEXT("RefreshRate"), GRefreshRate);
        }
    }

    UClient* Client = GConsole->Engine->Client;
    if (Client)
    {
        FLOAT Gamma = Client->Gamma;
        switch (GammaCombo->Selected)
        {
        case 0: Gamma = 1.2f; break;
        case 1: Gamma = 1.0f; break;
        case 2: Gamma = 0.8f; break;
        case 3: Gamma = 0.6f; break;
        case 4: Gamma = 0.4f; break;
        }
        if (Gamma != Client->Gamma)
        {
            Client->Gamma = Gamma;
            SaveFloat(Text, TEXT("Video"), TEXT("Gamma"), Gamma);
            GConsole->Engine->UpdateGamma();
        }
    }

    switch (PawnRangeCombo->Selected)
    {
    case 0: GL2OriginPawnCR = 3.0f; break;
    case 1: GL2OriginPawnCR = 2.5f; break;
    case 2: GL2OriginPawnCR = 2.0f; break;
    case 3: GL2OriginPawnCR = 1.5f; break;
    case 4: GL2OriginPawnCR = 1.0f; break;
    }
    SaveFloat(Text, TEXT("ClippingRange"), TEXT("Pawn"), GL2OriginPawnCR);

    INT PostProcess = PostProcessCombo->Selected;
    if (PostProcess >= 0 && PostProcess < 6)
        GL2PostProcessType = PostProcess;
    GConfig->SetInt(TEXT("Video"), TEXT("PostProc"), PostProcess, TEXT("Option.ini"));

    switch (TerrainRangeCombo->Selected)
    {
    case 0:
        GL2OriginTerrainCR = 8.0f;
        GL2OriginStaticMeshCR = 4.0f;
        GL2OriginStaticMeshLodCR = 6.0f;
        GL2ActorCR = 6.0f;
        break;
    case 1:
        GL2OriginTerrainCR = 7.0f;
        GL2OriginStaticMeshCR = 3.5f;
        GL2OriginStaticMeshLodCR = 5.5f;
        GL2ActorCR = 5.5f;
        break;
    case 2:
        GL2OriginTerrainCR = 6.0f;
        GL2OriginStaticMeshCR = 3.0f;
        GL2OriginStaticMeshLodCR = 5.0f;
        GL2ActorCR = 5.0f;
        break;
    case 3:
        GL2OriginTerrainCR = 5.0f;
        GL2OriginStaticMeshCR = 2.5f;
        GL2OriginStaticMeshLodCR = 4.5f;
        GL2ActorCR = 4.5f;
        break;
    case 4:
        GL2OriginTerrainCR = 4.0f;
        GL2OriginStaticMeshCR = 2.0f;
        GL2OriginStaticMeshLodCR = 4.0f;
        GL2ActorCR = 4.0f;
        break;
    }
    SaveFloat(Text, TEXT("ClippingRange"), TEXT("Terrain"), GL2OriginTerrainCR);
    SaveFloat(Text, TEXT("ClippingRange"), TEXT("Actor"), GL2ActorCR);
    SaveFloat(Text, TEXT("ClippingRange"), TEXT("StaticMesh"), GL2OriginStaticMeshCR);
    SaveFloat(Text, TEXT("ClippingRange"), TEXT("StaticMeshLod"), GL2OriginStaticMeshLodCR);

    if (!GroundDetailBox->bChecked)
    {
        GL2RenderDeco = 0;
        GConfig->SetString(TEXT("Video"), TEXT("RenderDeco"), TEXT("false"), TEXT("Option.ini"));
    }
    else
    {
        GL2RenderDeco = 1;
        GConfig->SetString(TEXT("Video"), TEXT("RenderDeco"), TEXT("true"), TEXT("Option.ini"));
    }
    if (!ShadowBox->bChecked)
    {
        GIsL2Projector = 0;
        GConfig->SetString(TEXT("Video"), TEXT("PawnShadow"), TEXT("false"), TEXT("Option.ini"));
    }
    else
    {
        GIsL2Projector = 1;
        GConfig->SetString(TEXT("Video"), TEXT("PawnShadow"), TEXT("true"), TEXT("Option.ini"));
    }

    INT Reflection = ReflectionCombo->Selected;
    if (!Reflection)
    {
        GIsL2WaterEffect = 0;
        GConfig->SetString(TEXT("L2WaterEffect"), TEXT("IsUseEffect"), TEXT("false"), TEXT("Option.ini"));
    }
    else
    {
        GIsL2WaterEffect = 1;
        GConfig->SetString(TEXT("L2WaterEffect"), TEXT("IsUseEffect"), TEXT("true"), TEXT("Option.ini"));
    }
    GIsL2WaterEffectType = Reflection;
    SaveInt(TEXT("L2WaterEffect"), TEXT("EffectType"), Reflection);

    if (!LowerDetailBox->bChecked)
    {
        GConfig->SetString(TEXT("Video"), TEXT("IsKeepMinFrameRate"), TEXT("false"), TEXT("Option.ini"));
        GL2KeepMinFrameRate = 0;
    }
    else
    {
        GConfig->SetString(TEXT("Video"), TEXT("IsKeepMinFrameRate"), TEXT("true"), TEXT("Option.ini"));
        GL2KeepMinFrameRate = 1;
    }

    // the fog follows the terrain range; "Lower Detail" takes the nearest
    FLOAT FogStart = 1.5f;
    FLOAT FogEnd = 5.5f;
    if (GL2KeepMinFrameRate || GL2OriginTerrainCR < 5.0f)
        LoadFogRange(TEXT("StartRange1"), TEXT("EndRange1"), 0.5f, 3.5f, FogStart, FogEnd);
    else if (GL2OriginTerrainCR >= 5.0f && GL2OriginTerrainCR < 6.0f)
        LoadFogRange(TEXT("StartRange2"), TEXT("EndRange2"), 1.0f, 4.5f, FogStart, FogEnd);
    else if (GL2OriginTerrainCR >= 6.0f && GL2OriginTerrainCR < 7.0f)
        LoadFogRange(TEXT("StartRange3"), TEXT("EndRange3"), 1.5f, 5.5f, FogStart, FogEnd);
    else if (GL2OriginTerrainCR >= 7.0f && GL2OriginTerrainCR < 8.0f)
        LoadFogRange(TEXT("StartRange4"), TEXT("EndRange4"), 2.0f, 6.5f, FogStart, FogEnd);
    else if (GL2OriginTerrainCR >= 8.0f)
        LoadFogRange(TEXT("StartRange5"), TEXT("EndRange5"), 2.5f, 7.5f, FogStart, FogEnd);
    GL2FogStart = FogStart;
    GL2FogEnd = FogEnd;

    switch (ActorLimitCombo->Selected)
    {
    case 0: GL2RenderActorLimitOpt = 6; GL2RenderActorLimit = 10000; break;
    case 1: GL2RenderActorLimitOpt = 5; GL2RenderActorLimit = 400; break;
    case 2: GL2RenderActorLimitOpt = 4; GL2RenderActorLimit = 300; break;
    case 3: GL2RenderActorLimitOpt = 3; GL2RenderActorLimit = 200; break;
    case 4: GL2RenderActorLimitOpt = 2; GL2RenderActorLimit = 100; break;
    case 5: GL2RenderActorLimitOpt = 1; GL2RenderActorLimit = 50; break;
    }
    appSprintf(Text, TEXT("%d"), GL2RenderActorLimitOpt);
    GConfig->SetString(TEXT("Video"), TEXT("RenderActorLimited"), Text, TEXT("Option.ini"));

    INT Level = AntialiasCombo->Selected;
    if (GL2Antialiasing != Level)
    {
        GL2Antialiasing = Level;
        GConfig->SetInt(TEXT("Video"), TEXT("AntiAliasing"), GL2Antialiasing, TEXT("Option.ini"));
        // the device takes the new level with the mode it has now (the 5-integer SetRes of slot 28; the names are ours, by
        // how the stock hands the values over)
        INT RefreshRate, CurrentX, CurrentY, Unused, ColorBytes;
        GConsole->Engine->GRenDev->GetResParams(RefreshRate, CurrentX, CurrentY, Unused, ColorBytes);
        GConsole->Engine->GRenDev->SetRes(GConsole->Engine->Client->Viewports(0), CurrentX, CurrentY, ColorBytes,
                                          RefreshRate, 1);
    }

    GL2ScreenShotQuality = ScreenShotCombo->Selected;
    GConfig->SetInt(TEXT("Game"), TEXT("ScreenShotQuality"), GL2ScreenShotQuality, TEXT("Option.ini"));
    unguard;
}
NW_HOOK_METHOD(0x100c5e70, NCVideoOptionWnd, Apply)

// 0x100c5580 - every control back to empty / off
void NCVideoOptionWnd::ClearItems()
{
    ResolutionCombo->ClearItems();
    RefreshCombo->ClearItems();
    GammaCombo->ClearItems();
    TextureCombo->ClearItems();
    ModelingCombo->ClearItems();
    AnimationCombo->ClearItems();
    ShadowBox->bChecked = 0;
    GroundDetailBox->bChecked = 0;
    PostProcessCombo->ClearItems();
    TerrainRangeCombo->ClearItems();
    PawnRangeCombo->ClearItems();
    ActorLimitCombo->ClearItems();
    ReflectionCombo->ClearItems();
    LowerDetailBox->bChecked = 0;
    TextureFilterBox->bChecked = 0;
    AntialiasCombo->ClearItems();
    ScreenShotCombo->ClearItems();
}
NW_HOOK_METHOD(0x100c5580, NCVideoOptionWnd, ClearItems)

// the gamma and the limit boxes list "5" .. "1" (the system strings 0x4D1 .. 0x4D5), the range boxes the same digits
// from 0x4D6 (repeated inline in the stock InitItems)
static FORCEINLINE void AddLevels(NCComboBox* Combo, INT FirstID)
{
    Combo->AddItem(FirstID);
    Combo->AddItem(FirstID + 1);
    Combo->AddItem(FirstID + 2);
    Combo->AddItem(FirstID + 3);
    Combo->AddItem(FirstID + 4);
}

// 0x100c90c0 - the lists; C4: the system strings as ids, the post processing combo lists None / 867 / 1246 (and three more
// when the device takes texture format 0x10), the screen shot quality four levels (the one in GL2ScreenShotQuality
// selected); the anti-aliasing combo lists None / 2x / 4x as far as the device multisamples (and is disabled without)
void NCVideoOptionWnd::InitItems()
{
    INT Widths[100], Heights[100], Bits[100], Rates[100];
    TCHAR Text[100];
    INT Count = GConsole->Engine->GRenDev->GetRes(100, Widths, Heights, Bits);
    for (INT i = 0; i < Count; i++)
    {
        appSprintf(Text, TEXT("%d*%d %dbit"), Widths[i], Heights[i], Bits[i]);
        ResolutionCombo->AddItem(Text);
    }
    Count = GConsole->Engine->GRenDev->GetRefreshRate(100, GSEKScreenX, GSEKScreenY, GColorBits, Rates);
    for (INT j = 0; j < Count; j++)
    {
        appSprintf(Text, TEXT("%dHz"), Rates[j]);
        RefreshCombo->AddItem(Text);
    }
    AddLevels(GammaCombo, 0x4D1);
    TextureCombo->AddItem(0xE0);
    TextureCombo->AddItem(0xE1);
    TextureCombo->AddItem(0xE2);
    ModelingCombo->AddItem(0xE0);
    ModelingCombo->AddItem(0xE2);
    AnimationCombo->AddItem(0xE0);
    AnimationCombo->AddItem(0xE2);
    PostProcessCombo->AddItem(0x365);
    PostProcessCombo->AddItem(0x363);
    PostProcessCombo->AddItem(0x4DE);
    if (GConsole->Engine->GRenDev->SupportsTextureFormat((ETextureFormat)0x10))
    {
        PostProcessCombo->AddItem(0x4CE);
        PostProcessCombo->AddItem(0x4CF);
        PostProcessCombo->AddItem(0x4D0);
    }
    AddLevels(TerrainRangeCombo, 0x4D6);
    AddLevels(PawnRangeCombo, 0x4D6);
    ActorLimitCombo->AddItem(0x1B);
    AddLevels(ActorLimitCombo, 0x4D1);
    ReflectionCombo->AddItem(0x1B);
    ReflectionCombo->AddItem(0x161);
    ReflectionCombo->AddItem(0x162);
    INT Samples = GConsole->Engine->GRenDev->GetMultiSample();
    if (Samples == 0)
    {
        AntialiasCombo->AddItem(0x365);
        AntialiasCombo->EnableWindow(0);
    }
    else if (Samples == 1)
    {
        AntialiasCombo->EnableWindow(1);
        AntialiasCombo->AddItem(0x365);
        AntialiasCombo->AddItem(0x366);
    }
    else if (Samples == 2)
    {
        AntialiasCombo->EnableWindow(1);
        AntialiasCombo->AddItem(0x365);
        AntialiasCombo->AddItem(0x366);
        AntialiasCombo->AddItem(0x367);
    }
    ScreenShotCombo->AddItem(0x3D4);
    ScreenShotCombo->AddItem(0x3D5);
    ScreenShotCombo->AddItem(0x3D6);
    ScreenShotCombo->AddItem(0x3D7);
    ScreenShotCombo->SetSelected(GL2ScreenShotQuality);
    LoadSettings();
}
NW_HOOK_METHOD(0x100c90c0, NCVideoOptionWnd, InitItems)

// 0x100c4210 (vslot 48) - the options window (the parent) takes the backdrop of this page
void NCVideoOptionWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    ((NCOptionWnd*)Entry->Parent->Wnd)->OptionBackTex = FindTexture(TEXT("L2ui_ch3.OptionWnd.option1_back"));
}
NW_HOOK_METHOD(0x100c4210, NCVideoOptionWnd, ShowWindow)

NW_CHECK_VTABLE(NCVideoOptionWnd, 0x101c0248, 78)
