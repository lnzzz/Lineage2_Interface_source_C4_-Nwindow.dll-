/*=============================================================================
    NCAbnormalStatusWnd.cpp - the buff / debuff icons. C2 stock: 0x100d2d20 .. 0x100d2dee,
    0x100d37c0 .. 0x100d3bf5, 0x100d8540 .. 0x100d85bf (NAbnormalEntry's constructor), 0x100d8560 ..
    0x100d8a62 (0x100d84b0 is the pointer array's destructor).
=============================================================================*/
#include "nwindow/NCAbnormalStatusWnd.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NItemEntryC4.h"


#if NW_FEATURES
// phase 6: BuffTimer (NWindow_C1 src/inject/BuffTimer.cpp)
#include "features/BuffTimer.h"
#endif

// title bar id
enum { NID_ABNORMALSTATUS_FRAME = 0xFFA0B3 };

#if NW_CUSTOM
// L2 Ignition: the first list holds up to 40 icons (four rows of ten; the stock keeps 31 and draws 30), since the
// server allows 24 buffs + 10 debuffs + toggles. The second list and the extra icon go in the row after the first's last.
enum { CUSTOM_MAX_STATUS = 40 };

// the rows the first list takes (0 when empty)
static FORCEINLINE INT CustomRows(INT Count)
{
    return (Count + 9) / 10;
}
#endif

// 0x10116530 - the lists are empty (their arrays by the members' constructors), the timers and the textures none; the
// hover index is -1. The extra icon's pointer and the fade timer are left as allocated.
NCAbnormalStatusWnd::NCAbnormalStatusWnd(INT InIconSize)
{
    TimerID = 0;
    ToolTipTex = NULL;
    TipTopTex = NULL;
    TipMidTex = NULL;
    TipBottomTex = NULL;
    IconSize = InIconSize;
    HoverIndex = -1;
}
NW_HOOK_CTOR_ARGS(0x10116530, NCAbnormalStatusWnd, (INT InIconSize), (InIconSize))


// stock body shared with NControl's (0x10004e90)
INT NCAbnormalStatusWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    return NCWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// whether a skill is a debuff (the console's 0x1006A9B0, name ours: the skill's record of that level has its debuff flag set)
static FORCEINLINE UBOOL IsDebuff(INT SkillID, INT Level)
{
    return ((NConsoleWndSkillExt*)GConsole)->IsDebuffSkill(SkillID, Level);
}

// the blink of an icon in its last 30 seconds (inlined in the stock OnTimer, for the list and for the extra icon):
// the alpha goes down by 25 until it is 0, then up until it is 255 (the direction flips at the ends)
static FORCEINLINE void FadeStep(NAbnormalEntry* E)
{
    if (E->Alpha < 0)
    {
        E->Alpha = 0;
        E->bFadeOut = 0;
    }
    else if (E->Alpha > 255)
    {
        E->Alpha = 255;
        E->bFadeOut = 1;
    }
    INT A = E->Alpha;
    if (E->bFadeOut)
        A -= 25;
    else
        A += 25;
    E->Alpha = A;
    if (A < 0)
    {
        E->Alpha = 0;
        E->bFadeOut = 0;
    }
    else if (A > 255)
    {
        E->Alpha = 255;
        E->bFadeOut = 1;
    }
}

// a second less on an icon (inlined in the stock OnTimer, for the list and for the extra icon): its tool tip shows the
// seconds left (not for a debuff: the number is -1 then) and is made again
static FORCEINLINE void CountDown(NAbnormalEntry* E)
{
    E->RemainTime--;
    EntryNumber(E) = E->RemainTime;
    if (IsDebuff(EntryID(E), EntryLevel(E)))
        EntryNumber(E) = -1;
    EntryToolTip(E)->Fill(E, NTT_SkillName, 0);
}

// 0x1010eef0 (vslot 36) - two timers. Every second the time of every icon runs down (the extra icon's too); every 100 ms
// the icons in their last 30 seconds change their alpha
INT NCAbnormalStatusWnd::OnTimer(INT InTimerID)
{
    if (InTimerID == TIMER_AbnormalStatus)
    {
        for (INT i = 0; i < Statuses.Num(); i++)
        {
            NAbnormalEntry* Status = Statuses(i);
            if (Status->RemainTime > 0)
                CountDown(Status);
        }
        if (ExtraStatus && ExtraStatus->RemainTime > 0)
        {
            CountDown(ExtraStatus);
            return 1;
        }
    }
    else if (InTimerID == TIMER_AbnormalFade)
    {
        for (INT i = 0; i < Statuses.Num(); i++)
        {
            NAbnormalEntry* Status = Statuses(i);
            if (Status->RemainTime >= 0 && Status->RemainTime <= 30)
                FadeStep(Status);
        }
        if (ExtraStatus && ExtraStatus->RemainTime >= 0 && ExtraStatus->RemainTime <= 30)
        {
            INT A = ExtraStatus->Alpha;
            if (A == 0)
                ExtraStatus->bFadeOut = 0;
            if (A == 255)
                ExtraStatus->bFadeOut = 1;
            if (ExtraStatus->bFadeOut)
                A -= 25;
            else
                A += 25;
            ExtraStatus->Alpha = A;
            if (A < 0)
            {
                ExtraStatus->Alpha = 0;
                ExtraStatus->bFadeOut = 0;
                return 1;
            }
            if (A > 255)
            {
                ExtraStatus->Alpha = 255;
                ExtraStatus->bFadeOut = 1;
            }
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x1010eef0, NCAbnormalStatusWnd, OnTimer)

// 0x10112c10 (vslot 37) - the two timers, the textures (the debuff frame is new in C3) and the title bar (a vertical
// one, small)
INT NCAbnormalStatusWnd::OnCreate()
{
    NCWnd::OnCreate();
    TimerID = SetTimer(TIMER_AbnormalStatus, 1000, NULL);
    FadeTimerID = SetTimer(TIMER_AbnormalFade, 100, NULL);
    ToolTipTex = FindTexture(TEXT("L2UI.tooltip.mini-tool01"));
    TipTopTex = FindTexture(TEXT("L2UI.Tooltip.Tool02"));
    TipMidTex = FindTexture(TEXT("L2UI.Tooltip.Tool01"));
    TipBottomTex = FindTexture(TEXT("L2UI.Tooltip.Tool03"));
    BackTex = FindTexture(TEXT("L2UI.EtcWndBack.AbnormalBack"));
    DebuffTex = FindTexture(TEXT("L2UI_CH3.PlayerStatusWnd.debuff"));

    FrameCtrl = new NCFrameCtrl(1, 1, 1);
    FrameID = NID_ABNORMALSTATUS_FRAME;
    FrameCtrl->ID = NID_ABNORMALSTATUS_FRAME;
    FrameCtrl->SetPosition(0, 0, 12, 26);
    CreateChild(FrameCtrl, NWS_VISIBLE);
    return 1;
}
NW_HOOK_METHOD(0x10112c10, NCAbnormalStatusWnd, OnCreate)

// 0x10117970 (vslot 38) - both timers killed, the records go (the stock code destroys only their tool tip and frees
// them: all they own), the base last
INT NCAbnormalStatusWnd::OnDestroy()
{
    guard(NCAbnormalStatusWnd::OnDestroy);
    if (TimerID)
    {
        KillTimer(TimerID);
        TimerID = 0;
    }
    if (FadeTimerID)
    {
        KillTimer(FadeTimerID);
        FadeTimerID = 0;
    }
    for (INT i = 0; i < Statuses.Num(); i++)
    {
        NAbnormalEntry* E = Statuses(i);
        if (E)
        {
            EntryToolTip(E)->~NToolTipInfo();
            appFree(E);
        }
    }
    Statuses.Empty();
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10117970, NCAbnormalStatusWnd, OnDestroy)

// stock body shared with NCChatWnd's (0x1003dab0)
void NCAbnormalStatusWnd::HideWindow()
{
    NCWnd::HideWindow();
}

// NCGDevice::DrawSpecialDigit(Mode, X, Y, Color, Text, Alpha) (0x10013350, ret 0x18): the alpha is a C4 argument
static FORCEINLINE void DrawDigits(NCGDevice* Device, INT Mode, INT X, INT Y, DWORD Color, const TCHAR* Text, INT Alpha)
{
    Device->DrawSpecialDigit(Mode, X, Y, Color, (TCHAR*)Text, (BYTE)Alpha);
}

// the seconds left under the icon of a record (a buff of the first list, last minute; the extra icon centers its own)
static FORCEINLINE UBOOL ShowsTime(NAbnormalEntry* E)
{
    return E->RemainTime < 60 && E->RemainTime >= 0 && !IsDebuff(EntryID(E), EntryLevel(E));
}

// an icon of the first list (spelled out three times in the stock paint, one per row of ten): the record keeps the
// icon's place (inside the back) for its tool tip, the back (26 x 26) at (BackX, BackY), the icon inside it (24 x 24,
// with the record's alpha: it blinks in its last 30 seconds), the seconds left in the small digits (not for a debuff),
// the debuff frame over it
static FORCEINLINE void DrawListIcon(NCGDevice* Device, UTexture* BackTex, UTexture* DebuffTex, NAbnormalEntry* E,
                                     INT BackX, INT BackY)
{
    E->IconX = BackX + 1;
    E->IconY = BackY + 1;
    Device->DrawTexture(BackX, BackY, 26, 26, 0, 0, 26, 26, BackTex, 0xFF, 1);
    if (E->bVisible)
        Device->DrawStretchedTex(E->IconX, E->IconY, 24, 24, E->Icon, 1, (BYTE)E->Alpha);
    if (ShowsTime(E))
    {
        TCHAR Buf[4];
        appSprintf(Buf, TEXT("%d"), E->RemainTime);
        DrawDigits(Device, 2, E->IconX + (E->RemainTime < 10 ? 7 : 2), E->IconY + 10, 0xFFDCDCDC, Buf, 0xCC);
    }
    if (IsDebuff(EntryID(E), EntryLevel(E)) && E->bVisible)
        Device->DrawTexture(E->IconX, E->IconY, 24, 24, 0, 0, 26, 26, DebuffTex, (BYTE)E->Alpha, 1);
}

// an icon of the second list: the back and the icon in it, fully opaque, no seconds and no debuff frame
static FORCEINLINE void DrawSecondIcon(NCGDevice* Device, UTexture* BackTex, NAbnormalEntry* E, INT BackX, INT BackY)
{
    Device->DrawTexture(BackX, BackY, 26, 26, 0, 0, 26, 26, BackTex, 0xFF, 1);
    if (E->bVisible)
        Device->DrawStretchedTex(BackX + 1, BackY + 1, 24, 24, E->Icon, 1, 0xFF);
    E->IconX = BackX + 1;
    E->IconY = BackY + 1;
}

// the extra icon: always in the first column of the row under the first list (C4: its place for the tool tip is the
// icon's own), with its seconds left centered over the icon's lower half
static FORCEINLINE void DrawExtraIcon(NCGDevice* Device, UTexture* BackTex, NAbnormalEntry* E, INT BackY)
{
    Device->DrawTexture(12, BackY, 26, 26, 0, 0, 26, 26, BackTex, 0xFF, 1);
    E->IconY = BackY + 1;
}

// 0x1010e560 (vslot 62) - the window hides itself when it holds nothing. The first list: up to 30 icons, ten to a row
// (rows at y 0, 25, 50; 26 pixels apart from x 12); then the extra icon in the first column of the row after the list;
// then the second list under the first one (shifted a column right when there is an extra icon). C4 adds the seconds
// left under the icons that are about to go (a minute) and a fourth row for the extra icon.
INT NCAbnormalStatusWnd::OnPaint(NCGDevice* Device)
{
    guard(NCAbnormalStatusWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (Statuses.Num() == 0 && Statuses2.Num() == 0 && ExtraStatus == NULL)
        HideWindow();
    for (INT i = 0; i < Statuses.Num(); i++)
    {
        NAbnormalEntry* E = Statuses(i);
        if (!E || !E->Icon)
            continue;
#if NW_CUSTOM
        if (i < CUSTOM_MAX_STATUS)
            DrawListIcon(Device, BackTex, DebuffTex, E, (i % 10) * 26 + 12, (i / 10) * 25);
#else
        if (i < 10)
            DrawListIcon(Device, BackTex, DebuffTex, E, i * 26 + 12, 0);
        else if (i < 20)
            DrawListIcon(Device, BackTex, DebuffTex, E, i * 26 - 248, 25);
        else if (i < 30)
            DrawListIcon(Device, BackTex, DebuffTex, E, i * 26 - 508, 50);
#endif
    }
    if (ExtraStatus)
    {
        INT Count = Statuses.Num();
#if NW_CUSTOM
        DrawExtraIcon(Device, BackTex, ExtraStatus, CustomRows(Count) * 25);
#else
        if (Count == 0)
            DrawExtraIcon(Device, BackTex, ExtraStatus, 0);
        else if (Count <= 10)
            DrawExtraIcon(Device, BackTex, ExtraStatus, 25);
        else if (Count <= 20)
            DrawExtraIcon(Device, BackTex, ExtraStatus, 50);
        else if (Count <= 30)
            DrawExtraIcon(Device, BackTex, ExtraStatus, 75);
#endif
        ExtraStatus->IconX = 13;
        if (ExtraStatus->bVisible)
            Device->DrawStretchedTex(ExtraStatus->IconX, ExtraStatus->IconY, 24, 24, ExtraStatus->Icon, 1,
                                     (BYTE)ExtraStatus->Alpha);
        if (ShowsTime(ExtraStatus))
        {
            TCHAR Buf[4];
            SIZE Size;
            appSprintf(Buf, TEXT("%d"), ExtraStatus->RemainTime);
            NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
            DrawDigits(Device, 1, ExtraStatus->IconX - Size.cx / 2 + 12, ExtraStatus->IconY + 8, 0xFFDCDCDC, Buf, 0xFF);
        }
    }
    for (INT i = 0; i < Statuses2.Num(); i++)
    {
        NAbnormalEntry* E = Statuses2(i);
        if (!E || !E->Icon)
            continue;
        INT Count = Statuses.Num();
        INT BackX = i * 26 + (ExtraStatus ? 38 : 12);
#if NW_CUSTOM
        DrawSecondIcon(Device, BackTex, E, BackX, CustomRows(Count) * 25);
#else
        if (Count == 0)
            DrawSecondIcon(Device, BackTex, E, BackX, 0);
        else if (Count <= 10)
            DrawSecondIcon(Device, BackTex, E, BackX, 25);
        else if (Count <= 20)
            DrawSecondIcon(Device, BackTex, E, BackX, 50);
        else if (Count <= 30)
            DrawSecondIcon(Device, BackTex, E, BackX, 75);
#endif
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1010e560, NCAbnormalStatusWnd, OnPaint)

// the record in cell Index of the grid ten wide that HitTest numbers (the first list from the top, then the second list
// under it from the row after the first's last, the extra icon before it; the stock code spells it out for each of
// the first list's lengths): NULL when the cell is empty
static NAbnormalEntry* GridEntry(NCAbnormalStatusWnd* Wnd, INT Index)
{
    INT Count = Wnd->Statuses.Num();
    INT Count2 = Wnd->Statuses2.Num();
    INT Rows;
#if NW_CUSTOM
    Rows = CustomRows(Count) * 10;
#else
    if (Count == 0)
        Rows = 0;
    else if (Count <= 10)
        Rows = 10;
    else if (Count <= 20)
        Rows = 20;
    else
        Rows = 30;
#endif
    if (Index < Rows)
        return Index < Count ? Wnd->Statuses(Index) : NULL;
    INT Slot = Index - Rows;
    if (Wnd->ExtraStatus)
    {
        if (Slot == 0)
            return Wnd->ExtraStatus;
        Slot--;
    }
    return Slot < Count2 ? Wnd->Statuses2(Slot) : NULL;
}

/// 0x10116640 (vslot 63) - the tool tip of the icon under the mouse, in the window's tool tip skin over the icon's place
// the paint left in its record (the full box). Sets IconSize to 26 each time (HitTest divides by it); a HoverIndex out of
// 0 .. 40 ends before the clip is released, as in the stock code.
INT NCAbnormalStatusWnd::OnDrawToolTip(NCGDevice* Device)
{
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, (INT)Width, (INT)Height);
    IconSize = 26;
#if NW_CUSTOM
    if (HoverIndex < 0 || HoverIndex >= CUSTOM_MAX_STATUS + 10)
        return 1;
#else
    if (HoverIndex < 0 || HoverIndex > 40)
        return 1;
#endif
    NAbnormalEntry* E = GridEntry(this, HoverIndex);
    if (E && E->bVisible)
    {
        NRectF Screen = {0, 0, 0.0f, 0.0f};
        GConsole->Desktop->GetLocalRect(&Screen);   // (the stock calls it and does not use the rectangle)
        if (ToolTipSkin)
        {
            NToolTipRect Wnd;
            Wnd.X = ScreenX;
            Wnd.Y = ScreenY;
            Wnd.W = Width;
            Wnd.H = Height;
            ToolTipSkin->DrawFull(Device, *EntryToolTip(E), E->IconX, E->IconY, Wnd);
        }
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
}
NW_HOOK_METHOD(0x10116640, NCAbnormalStatusWnd, OnDrawToolTip)

// 0x1010d220 (vslot 68; name ours) - C3: the window's size from what it holds, with the title bar to match (12 wide,
// as high as the window). A row holds ten icons of 26 pixels (12 pixels of title bar before them) and is 26 high; a row
// is 25 pixels further down. The first list takes as many rows as it has tens; the second list and the extra icon go
// under it: a first list of up to ten and a second one (with the extra) longer than it makes the window as wide as
// the second one, else it is as wide as the first. More than ten icons make it ten wide (272).
void NCAbnormalStatusWnd::UpdateLayout()
{
    INT Count = Statuses.Num();
    INT Count2 = Statuses2.Num();
    if (Count <= 10)
    {
        if (!ExtraStatus && Count2 <= 0)
        {
            SetWindowSize((FLOAT)(Count * 26 + 12), 26.0f);
            FrameCtrl->SetFrameSize(12, 26);
            return;
        }
        if (Count2 + (ExtraStatus != NULL) > Count)
        {
            INT H = Count ? 51 : 26;
            INT W = ExtraStatus ? Count2 * 26 + 38 : Count2 * 26 + 12;
            SetWindowSize((FLOAT)W, (FLOAT)H);
            FrameCtrl->SetFrameSize(12, H);
            return;
        }
        SetWindowSize((FLOAT)(Count * 26 + 12), 51.0f);
        FrameCtrl->SetFrameSize(12, 51);
        return;
    }
    if (Count <= 20)
    {
        if (!ExtraStatus && Count2 <= 0)
        {
            SetWindowSize(272.0f, 51.0f);
            FrameCtrl->SetFrameSize(12, 51);
            return;
        }
        SetWindowSize(272.0f, 76.0f);
        FrameCtrl->SetFrameSize(12, 76);
        return;
    }
    if (Count <= 30 && !ExtraStatus && Count2 <= 0)
    {
        SetWindowSize(272.0f, 76.0f);
        FrameCtrl->SetFrameSize(12, 76);
        return;
    }
#if NW_CUSTOM
    // more than 30: four rows, and a fifth for the second list / the extra icon
    if (Count > 30)
    {
        INT H = (ExtraStatus || Count2 > 0) ? 126 : 101;
        SetWindowSize(272.0f, (FLOAT)H);
        FrameCtrl->SetFrameSize(12, H);
        return;
    }
#endif
    SetWindowSize(272.0f, 101.0f);
    FrameCtrl->SetFrameSize(12, 101);
}
NW_HOOK_METHOD(0x1010d220, NCAbnormalStatusWnd, UpdateLayout)

// 0x1010ee50 (vslot 67) - the base first (it drags a dragable window), then the icon under the mouse
INT NCAbnormalStatusWnd::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    guard(NCAbnormalStatusWnd::OnMouseMove);
    NCWnd::OnMouseMove(wParam, lParam);
    HoverIndex = HitTest(LOWORD(lParam), HIWORD(lParam));
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1010ee50, NCAbnormalStatusWnd, OnMouseMove)

/// 0x1010d1c0 - the icon under a screen point, row by row of ten; the title bar is not taken off, so the hit areas sit
// 12 pixels left of the icons, and there are four rows
INT NCAbnormalStatusWnd::HitTest(INT X, INT Y)
{
    INT RX = X + (-12 - ScreenX);
    INT RY = Y - ScreenY;
#if NW_CUSTOM
    if (RX < 0 || RX > (IconSize * 5 + 5) * 2 || RY < 0 || RY > IconSize * 5 + 5)
        return -1;
#else
    if (RX < 0 || RX > (IconSize * 5 + 5) * 2 || RY < 0 || RY > IconSize * 4 + 4)
        return -1;
#endif
    return RX / (IconSize + 1) + RY / (IconSize + 1) * 10;
}
NW_HOOK_METHOD(0x1010d1c0, NCAbnormalStatusWnd, HitTest)

// a new record from the console's one (the stock spells it out in AddStatus, AddStatus2 and SetExtraStatus): memory by
// the global allocator with the tag "new" and NAbnormalEntry's constructor; the icon by its texture name (the black texture
// when missing)
static FORCEINLINE NAbnormalEntry* NewEntry(NCAbnormalStatusWnd* Wnd, FAbnormalStatus* Status)
{
    NAbnormalEntry* Entry = (NAbnormalEntry*)appMalloc(sizeof(NAbnormalEntry), TEXT("new"));
    if (Entry)
        Entry = new ((void*)Entry) NAbnormalEntry;
    if (Status->IconName)
        Entry->Icon = Wnd->FindTexture(Status->IconName);
    if (!Entry->Icon)
        Entry->Icon = Wnd->FindTexture(TEXT("NWindow.BlackTexture"));
    return Entry;
}

// the name and the description from the skill tables and the tool tip of the skill (the end of the three adds)
static FORCEINLINE void FillEntryTexts(NAbnormalEntry* Entry, FAbnormalStatus* Status)
{
    if (GConsole->GetSkillName(Status->SkillID, Status->Level))
        appStrcpy(EntryName(Entry), GConsole->GetSkillName(Status->SkillID, Status->Level));
    if (GConsole->GetSkillDesc(Status->SkillID, Status->Level))
        appStrcpy(EntryDescription(Entry), GConsole->GetSkillDesc(Status->SkillID, Status->Level));
    EntryToolTip(Entry)->Fill(Entry, NTT_SkillName, 0);
}

// the name of the enchanted route of the skill's record (C4), kept as the additional name
static FORCEINLINE void FillEnchantName(NAbnormalEntry* Entry)
{
    FL2MagicSkillData* Skill = GL2GameData.GetMSData(EntryID(Entry), EntryLevel(Entry));
    if (Skill && Skill->bEnchanted)
        appStrcpy(EntryAdditionalName(Entry), *Skill->EnchantName);
}

// 0x10115aa0 - a record from the console's one: the icon (by its texture name, the black texture when missing), the
// time, the skill id and level, the name and the description from the skill tables, the skill's tool tip (the seconds
// left are its number); the window is opened (outside a replay) and the layout of the lists redone (vslot 74, UpdateLayout);
// 0 when the window holds 31 already
INT NCAbnormalStatusWnd::AddStatus(FAbnormalStatus* Status)
{
#if NW_CUSTOM
    if (Statuses.Num() >= CUSTOM_MAX_STATUS)
        return 0;
#else
    if (Statuses.Num() > 30)
        return 0;
#endif
    if (!GL2ReplayMode)
    {
        if (!IsShowWindow())
            ShowWindow();
    }
    NAbnormalEntry* Entry = NewEntry(this, Status);
    EntryLevel(Entry) = Status->Level;
    Entry->RemainTime = Status->RemainTime;
    EntryID(Entry) = Status->SkillID;
    Entry->RemainTime = Status->RemainTime;
    EntryNumber(Entry) = Status->RemainTime;
    FillEnchantName(Entry);
    FillEntryTexts(Entry, Status);
    INT i = Statuses.Add();
    Statuses(i) = Entry;
    UpdateLayout();
    return 1;
}
NW_HOOK_METHOD(0x10115aa0, NCAbnormalStatusWnd, AddStatus)

// 0x10115cb0 - the same for the second list (10 at most; shown outside a replay too), without the enchanted route's name
// and with no number in the tool tip
INT NCAbnormalStatusWnd::AddStatus2(FAbnormalStatus* Status)
{
    if (Statuses2.Num() > 9)
        return 0;
    if (!IsShowWindow())
        ShowWindow();
    NAbnormalEntry* Entry = NewEntry(this, Status);
    EntryLevel(Entry) = Status->Level;
    Entry->RemainTime = Status->RemainTime;
    EntryID(Entry) = Status->SkillID;
    EntryNumber(Entry) = -1;
    FillEntryTexts(Entry, Status);
    INT i = Statuses2.Add();
    Statuses2(i) = Entry;
    UpdateLayout();
    return 1;
}
NW_HOOK_METHOD(0x10115cb0, NCAbnormalStatusWnd, AddStatus2)

// 0x10117a70 - the extra icon: an empty name clears it, else a record as AddStatus makes one (the one it replaces is not
// freed)
INT NCAbnormalStatusWnd::SetExtraStatus(FAbnormalStatus* Status)
{
    if (!IsShowWindow())
        ShowWindow();
    if (Status == NULL || Status->Name[0] == 0)
        ClearExtraStatus();
    else
    {
        NAbnormalEntry* Entry = NewEntry(this, Status);
        EntryLevel(Entry) = Status->Level;
        Entry->RemainTime = Status->RemainTime;
        EntryID(Entry) = Status->SkillID;
        EntryNumber(Entry) = Status->RemainTime;
        FillEnchantName(Entry);
        FillEntryTexts(Entry, Status);
        ExtraStatus = Entry;
        UpdateLayout();
    }
    return 1;
}
NW_HOOK_METHOD(0x10117a70, NCAbnormalStatusWnd, SetExtraStatus)

// a record goes (~NItemEntry, 0x10050030: its tool tip; the memory back to the allocator)
static FORCEINLINE void FreeEntry(NAbnormalEntry* E)
{
    static_cast<NItemEntry*>(E)->~NItemEntry();
    appFree(E);
}

// 0x10116a10 - the records go and the layout is redone (vslot 74)
void NCAbnormalStatusWnd::ClearStatus()
{
    for (INT i = 0; i < Statuses.Num(); i++)
    {
        if (Statuses(i))
            FreeEntry(Statuses(i));
    }
    Statuses.Empty();
    UpdateLayout();
}
NW_HOOK_METHOD(0x10116a10, NCAbnormalStatusWnd, ClearStatus)

// 0x10116960 - the same for the second list
void NCAbnormalStatusWnd::ClearStatus2()
{
    for (INT i = 0; i < Statuses2.Num(); i++)
    {
        if (Statuses2(i))
            FreeEntry(Statuses2(i));
    }
    Statuses2.Empty();
    UpdateLayout();
}
NW_HOOK_METHOD(0x10116960, NCAbnormalStatusWnd, ClearStatus2)

// 0x101169d0 - the extra icon goes (and the layout is redone when there was one)
void NCAbnormalStatusWnd::ClearExtraStatus()
{
    if (ExtraStatus)
    {
        FreeEntry(ExtraStatus);
        ExtraStatus = NULL;
        UpdateLayout();
    }
}
NW_HOOK_METHOD(0x101169d0, NCAbnormalStatusWnd, ClearExtraStatus)

// 0x101165d0 - the two buff lists go, then NCWnd
NW_HOOK_DTOR(0x101165d0, NCAbnormalStatusWnd)

// 0x101159d0 - the destructor instance of the two buff lists (the stock calls it for each from the window's)
typedef TArray<NAbnormalEntry*> TAbnormalEntryArray;
NW_HOOK_DTOR(0x101159d0, TAbnormalEntryArray)

// 0x10115a60 - an item entry (the stock's constructor, 0x1001F7F0) with the blink state: visible, fading out, 0 s, opaque
NAbnormalEntry::NAbnormalEntry() : RemainTime(0), Alpha(255), bVisible(1), bFadeOut(1)
{
}
NW_HOOK_CTOR(0x10115A60, NAbnormalEntry)

NW_CHECK_VTABLE(NCAbnormalStatusWnd, 0x101cdc40, 75)
