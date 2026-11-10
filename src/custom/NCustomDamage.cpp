/*=============================================================================
    NCustomDamage.cpp - the damage done to the target, over its head (custom, NW_CUSTOM builds only). See NCustomDamage.h.
=============================================================================*/
#include "nwindow/NCustomDamage.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

enum
{
    MSG_CriticalHit = 44,
    MSG_MagicCriticalHit = 1280,
    MSG_YouDidDamage = 35,
    MaxHits = 32,
    LifeMs = 1200,
    CritWindowMs = 500,   // a critical message counts for the damage that follows it within this time
    DigitW = 11,          // a large digit's place in its texture (NCGDevice::DrawSpecialDigit type 2: 11 x 14, in a row)
    DigitH = 14,
};

static const FLOAT Rise = 20.0f;      // world units the number rises
static const FLOAT OverHead = 4.0f;   // world units over half the collision height (the names float at 7)
static const FLOAT Spread = 6.0f;     // world units of sideways offset, so that close hits do not cover each other
static const FLOAT Scale = 1.25f;     // the digits' size on the screen, times their texture's
static const FLOAT CritScale = 1.85f;  // the same for a critical hit

struct NHit
{
    INT ObjectID;
    FVector Last;       // the top of the head, last seen
    DWORD Start;        // GetTickCount; 0 = free
    FLOAT OffX, OffY;
    TCHAR Text[16];
    UBOOL bCritical;
};

static NHit GHits[MaxHits];
static INT GNext = 0;
static DWORD GCriticalAt = 0;
static INT GShown = -1;   // -1: not read yet

/*-----------------------------------------------------------------------------
    Settings: [Damage] of system\NWindowCustom.ini.
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

UBOOL NCustomDamage::IsShown()
{
    if (GShown < 0)
        GShown = GetPrivateProfileIntW(TEXT("Damage"), TEXT("Show"), 1, IniPath()) != 0;
    return GShown;
}

void NCustomDamage::SetShown(UBOOL bShown)
{
    GShown = bShown != 0;
    WritePrivateProfileStringW(TEXT("Damage"), TEXT("Show"), GShown ? TEXT("1") : TEXT("0"), IniPath());
    if (!GShown)
        for (INT i = 0; i < MaxHits; i++)
            GHits[i].Start = 0;
}

/*-----------------------------------------------------------------------------
    The hits.
-----------------------------------------------------------------------------*/

// over the pawn's head, a little over where the game puts the names (NCLobbyWnd: half the collision height and 7)
static FVector HeadOf(APawn* Pawn)
{
    FVector Loc = Pawn->Location;
    return FVector(Loc.X, Loc.Y, Loc.Z + Pawn->CollisionHeight * 0.5f + OverHead);
}

void NCustomDamage::OnSystemMessage(INT ID, const TCHAR* Param1)
{
    if (!IsShown() || !GConsole || GConsole->State != L2CS_InGame)
        return;
    if (ID == MSG_CriticalHit || ID == MSG_MagicCriticalHit)
    {
        DWORD Now = GetTickCount();
        GCriticalAt = Now ? Now : 1;
        return;
    }
    if (ID != MSG_YouDidDamage || !Param1)
        return;
    User* Target = GConsole->GetSelectedActor();
    DWORD Now = GetTickCount();
    UBOOL bCritical = GCriticalAt && Now - GCriticalAt < (DWORD)CritWindowMs;
    GCriticalAt = 0;
    if (!Target || !Target->Pawn)
        return;
    NHit& Hit = GHits[GNext];
    GNext = (GNext + 1) % MaxHits;
    Hit.ObjectID = Target->ObjectID;
    Hit.Last = HeadOf(Target->Pawn);
    Hit.Start = Now ? Now : 1;   // 0 is a free place
    Hit.OffX = ((INT)(Now * 7919u % 1000u) / 500.0f - 1.0f) * Spread;
    Hit.OffY = ((INT)(Now * 104729u % 1000u) / 500.0f - 1.0f) * Spread;
    Hit.bCritical = bCritical;
    appStrncpy(Hit.Text, Param1, 16);
}

typedef void(__thiscall* FCameraSceneNodeCtor)(void* Node, UViewport* Viewport, void* Target, AActor* ViewActor, FVector Location,
                                               FRotator Rotation, FLOAT FOV);
typedef void(__thiscall* FCameraSceneNodeDtor)(void* Node);
typedef void(__thiscall* FSceneNodeProject)(void* Node, FPlane* Out, FVector Point);
typedef void(__thiscall* FCanvasUtilDrawTile)(void* Util, FLOAT X1, FLOAT Y1, FLOAT X2, FLOAT Y2, FLOAT U1, FLOAT V1, FLOAT U2,
                                              FLOAT V2, FLOAT Z, UMaterial* Material, DWORD Color);
static const DWORD DigitBlendRva = 0x8302A8;   // Engine.dll: the FinalBlend UCanvas::DrawSpecialDigit draws its digits with

// a number of the large digits, Scale times their size: UCanvas::DrawSpecialDigit draws each digit with FCanvasUtil::DrawTile
// (UCanvas +0xA8) over a FinalBlend of its own (alpha blended, kept in a global), and nothing larger. That one is set up for
// the large digits by a call of it off the screen, then each digit is drawn stretched with it, in the color (its alpha too)
static void DrawNumber(NCGDevice* Device, FLOAT X, FLOAT Y, const TCHAR* Text, DWORD Color, FLOAT Scale)
{
    static FCanvasUtilDrawTile DrawTile;
    static UMaterial** Blend;
    if (!DrawTile)
    {
        HMODULE Engine = GetModuleHandleW(L"Engine.dll");
        if (!Engine)
            return;
        DrawTile = (FCanvasUtilDrawTile)GetProcAddress(Engine, "?DrawTile@FCanvasUtil@@QAEXMMMMMMMMMPAVUMaterial@@VFColor@@@Z");
        Blend = (UMaterial**)((BYTE*)Engine + DigitBlendRva);
        if (!DrawTile)
            return;
    }
    void* Util = ::nw::Field<void*>(Device->Canvas, 0xA8);
    Device->DrawSpecialDigit(2, -100, -100, Color, (TCHAR*)TEXT("0"), 0xFF);
    if (!Util || !*Blend)
        return;
    FLOAT W = DigitW * Scale, H = DigitH * Scale;
    for (; *Text; Text++, X += W)
    {
        if (*Text < '0' || *Text > '9')
            continue;
        FLOAT U = (FLOAT)((*Text - '0') * DigitW);
        DrawTile(Util, X, Y, X + W, Y + H, U, 0.0f, U + DigitW, (FLOAT)DigitH, 0.0f, *Blend, Color);
    }
}

// each hit rises (fast at first) and fades in its last 40%, the oldest first, so the newest is on top. The point over the head
// goes to the screen as UCanvas::Draw3DCoordText does it: a FCameraSceneNode of the viewport's player controller (+0x3C; its
// location +0x1BC, rotation +0x1C8, FOV +0x3B4 and view actor +0x5A8), FSceneNode::Project (-1 .. 1, y up) and the viewport's
// size (+0x94, +0x98); a point behind the camera (w <= 0) is not drawn. The number is the game's large digits
// (L2UI_CH3.Etc.XLargeNumber), larger (DrawNumber), centered on the point.
void NCustomDamage::Paint(NCGDevice* Device)
{
    if (!IsShown() || !GConsole || GConsole->State != L2CS_InGame || !Device->Canvas)
        return;
    INT k;
    for (k = 0; k < MaxHits && !GHits[k].Start; k++)
        ;
    if (k == MaxHits)
        return;
    static FCameraSceneNodeCtor Ctor;
    static FCameraSceneNodeDtor Dtor;
    static FSceneNodeProject Project;
    if (!Ctor)
    {
        HMODULE Engine = GetModuleHandleW(L"Engine.dll");
        if (!Engine)
            return;
        Ctor = (FCameraSceneNodeCtor)GetProcAddress(Engine,
            "??0FCameraSceneNode@@QAE@PAVUViewport@@PAVFRenderTarget@@PAVAActor@@VFVector@@VFRotator@@M@Z");
        Dtor = (FCameraSceneNodeDtor)GetProcAddress(Engine, "??1FCameraSceneNode@@UAE@XZ");
        Project = (FSceneNodeProject)GetProcAddress(Engine, "?Project@FSceneNode@@QAE?AVFPlane@@VFVector@@@Z");
        if (!Ctor || !Dtor || !Project)
        {
            Ctor = NULL;
            return;
        }
    }
    UViewport* Viewport = Device->Canvas->Viewport;
    AActor* Controller = Viewport ? ::nw::Field<AActor*>(Viewport, 0x3C) : NULL;
    if (!Controller)
        return;
    FLOAT SizeX = (FLOAT)::nw::Field<INT>(Viewport, 0x94), SizeY = (FLOAT)::nw::Field<INT>(Viewport, 0x98);
    static __declspec(align(16)) BYTE Node[0x800];
    Ctor(Node, Viewport, (BYTE*)Viewport + 0xDC, ::nw::Field<AActor*>(Controller, 0x5A8), ::nw::Field<FVector>(Controller, 0x1BC),
         ::nw::Field<FRotator>(Controller, 0x1C8), ::nw::Field<FLOAT>(Controller, 0x3B4));
    DWORD Now = GetTickCount();
    for (k = 0; k < MaxHits; k++)
    {
        NHit& Hit = GHits[(GNext + k) % MaxHits];
        if (!Hit.Start)
            continue;
        INT Age = (INT)(Now - Hit.Start);
        if (Age < 0)
            Age = 0;
        if (Age >= LifeMs)
        {
            Hit.Start = 0;
            continue;
        }
        User* Target = GConsole->FindUserByID(Hit.ObjectID);
        if (Target && Target->Pawn)
            Hit.Last = HeadOf(Target->Pawn);
        FLOAT T = Age / (FLOAT)LifeMs;
        FLOAT Up = Rise * (1.0f - (1.0f - T) * (1.0f - T));
        FPlane P;
        Project(Node, &P, FVector(Hit.Last.X + Hit.OffX, Hit.Last.Y + Hit.OffY, Hit.Last.Z + Up));
        if (P.W <= 0.0f)
            continue;
        INT Alpha = T < 0.6f ? 255 : (INT)(255.0f * (1.0f - (T - 0.6f) / 0.4f));
        FLOAT S = Hit.bCritical ? CritScale : Scale;
        FLOAT X = (FLOAT)(INT)((P.X + 1.0f) * 0.5f * SizeX - appStrlen(Hit.Text) * DigitW * S * 0.5f);
        FLOAT Y = (FLOAT)(INT)((1.0f - P.Y) * 0.5f * SizeY - DigitH * S * 0.5f);
        DrawNumber(Device, X, Y, Hit.Text, ((DWORD)Alpha << 24) | (Hit.bCritical ? 0x00FFD800 : 0x00FFFFFF), S);
    }
    Dtor(Node);
}
