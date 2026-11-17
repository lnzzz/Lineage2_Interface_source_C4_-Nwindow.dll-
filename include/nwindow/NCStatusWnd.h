#pragma once
// NCStatusWnd - base of the HP / MP windows (UClass "CStatusWnd", 0x2FC): the player's
// (NCPlayerStatusWnd: bars, level, exp), the target's (NCTargetStatusWnd: name colored by the
// level difference, HP / MP of monsters and party members, clan and alliance) and one per
// party member (NCPartyStatusWnd, inside NCPartyWnd), the pet's (NCPetStatusWnd) and the
// summon's (NCSummonedStatusWnd). The base only keeps the textures and numbers the subclasses
// draw.
//
// Desktop (NCConsole) +0xC0 is the target window, +0xC4 the player window, +0xB4 the pet
// window, +0xB8 the summon window.

#include "nwindow/NCWnd.h"
#pragma pack(push, 4)   // the C2 layout puts 64-bit members at 4-aligned offsets

class NCButton;
class NCResizeFrame;
class NCProgressCtrl;

class NCStatusWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCStatusWnd, NCWnd, 0, 0x10308C08)

    UTexture* MPTex;                 // +0xE8
    UTexture* HPTex;                 // +0xEC
    UTexture* EXPTex;                // +0xF0
    UTexture* CPBarTex;              // +0xF4 (new in C4) "L2UI_CH3.SmallWnd.cpBar" (the target's and the party member's)
    UTexture* BackTex;               // +0xF8 "NWindow.WhiteTexture" (OnCreate), the subclasses
                                     //   load their own
    UTexture* BackTopTex;            // +0xFC target window: background (top, center, bottom)   // C2: offset derived, no evidence   // C2: offset derived, no evidence   // C2: offset derived, no evidence
    UTexture* BackCenterTex;         // +0x100   // C2: offset derived, no evidence   // C2: offset derived, no evidence   // C2: offset derived, no evidence
    UTexture* BackBottomTex;         // +0x104   // C2: offset derived, no evidence   // C2: offset derived, no evidence   // C2: offset derived, no evidence
    UTexture* FrameTopTex;           // +0x108 target window: left edge (top, center, bottom)   // C2: offset derived, no evidence   // C2: offset derived, no evidence   // C2: offset derived, no evidence
    UTexture* FrameCenterTex;        // +0x10C   // C2: offset derived, no evidence   // C2: offset derived, no evidence   // C2: offset derived, no evidence
    UTexture* FrameBottomTex;        // +0x110   // C2: offset derived, no evidence   // C2: offset derived, no evidence   // C2: offset derived, no evidence
    UTexture* UpBtnTex;              // +0x114 target window: loaded, never drawn (the toggle   // C2: offset derived, no evidence   // C2: offset derived, no evidence   // C2: offset derived, no evidence
    UTexture* UpBtnClickTex;         // +0x118   button loads its own)   // C2: offset derived, no evidence   // C2: offset derived, no evidence   // C2: offset derived, no evidence
    UTexture* DownBtnTex;            // +0x11C   // C2: offset derived, no evidence   // C2: offset derived, no evidence   // C2: offset derived, no evidence
    UTexture* DownBtnClickTex;       // +0x120   // C2: offset derived, no evidence   // C2: offset derived, no evidence   // C2: offset derived, no evidence
    UTexture* LeftBackTex;           // +0x124 player window: edges
    UTexture* RightBackTex;          // +0x128   // C2: offset derived, no evidence
    UTexture* MPBackTex;             // +0x12C empty bars   // C2: offset derived, no evidence
    UTexture* HPBackTex;             // +0x130   // C2: offset derived, no evidence   // C2: offset derived, no evidence
    UTexture* CPBarBackTex;          // +0x134 (new in C4) "L2UI_CH3.SmallWnd.CpBar_back" (the target's and the party member's)
    INT CP;                          // +0x138 (new in C4, name ours) the combat points: SetInfo pops them after the name, Clear zeroes them
    INT MaxCP;                       // +0x13C (new in C4, name ours)
    INT HP;                          // +0x140 party member (SetInfo)
    INT MaxHP;                       // +0x144
    INT MP;                          // +0x148
    INT MaxMP;                       // +0x14C
    INT ObjectID;                    // +0x150 target window: whom a click targets / attacks
    INT ClassID;                     // +0x154 (new in C3, name ours) party member: its class (SetInfo, UpdateInfo)   // C2: offset derived, no evidence
    INT BarWidth;                    // +0x158 constructor argument (the target window sets 132)   // C3: the constructor stores it here
    INT MaxExtraWidth;               // +0x15C player window: how far it can be widened (202)
    INT ExtraWidth;                  // +0x160 player window: widened by this (bars grow with it)   // C2: offset derived, no evidence
    TCHAR Name[256];                 // +0x164 party member, empty = free slot

    __declspec(noinline) NCStatusWnd(INT InBarWidth = 100);                // 0x10116aa0 (out of line: the derived constructors call it)
    // ~NCStatusWnd                                                  // 0x100d3740 (C1 only: no C2 match) (C1 only: no C2 match)

    virtual __declspec(noinline) INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam); // 25 0x10148f10 (folded; the
                                                                     //   derived WindowProcs call it, so out of line)
    virtual __declspec(noinline) INT OnCreate();                     // 37 0x1010d3f0 (called by the derived OnCreates: out of line)
    virtual INT OnDestroy();                                         // 38 0x1010d420 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // 62 0x1010f130
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam) { return 1; } // 68 0x100e6a50 (folded)

    // names ours
    UBOOL IsName(const TCHAR* InName);                               // 0x1010d4a0
    virtual __declspec(noinline) void Clear();                       // 74 0x1010d4d0 (C3: virtual; the party member's adds its buffs)
    __declspec(noinline) UBOOL IsEmpty();                            // 0x1010dbf0
};
NW_ASSERT_OFFSET(NCStatusWnd, MPTex, 0xE8);
NW_ASSERT_OFFSET(NCStatusWnd, LeftBackTex, 0x124);
NW_ASSERT_OFFSET(NCStatusWnd, HP, 0x140);
NW_ASSERT_OFFSET(NCStatusWnd, ObjectID, 0x150);
NW_ASSERT_OFFSET(NCStatusWnd, ExtraWidth, 0x160);
NW_ASSERT_OFFSET(NCStatusWnd, Name, 0x164);
NW_ASSERT_SIZE(NCStatusWnd, 0x364);

// NWM_COMMAND ids of the status windows' controls
enum
{
    NID_PLAYERSTATUS_RESIZE = 0xF20F04,  // right edge of the player window
    NID_TARGETSTATUS_CLOSE = 0xFA,
    NID_TARGETSTATUS_TOGGLE = 0xFB,      // shows / hides the clan and alliance lines
};

// NCPlayerStatusWnd - the player's name, level, HP / MP / exp bars (UClass "CPlayerStatusWnd",
// 0x308). The right edge widens it (the bars grow with it); a click targets the player.
class NCPlayerStatusWnd : public NCStatusWnd
{
    NW_DECLARE_CLASS(NCPlayerStatusWnd, NCStatusWnd, 0, 0x1030A608)

    NCWnd* HPBar;                    // +0x364 (C3) NCStatusBarCtrl "ps_hpbar" (ID 0xFFA103F0), grows with the window
    UTexture* PlayerHPBackTex;       // +0x368 "L2UI_CH3.PlayerStatusWnd.ps_hpbar_back"   // C2: offset derived, no evidence
    UTexture* PlayerMPBackTex;       // +0x36C "ps_mpbar_back"
    UTexture* PlayerEXPBackTex;      // +0x370 "ps_expbar_back"
    UTexture* CPBackTex;             // +0x374 "smallbar_cpback"
    UTexture* CPTex;                 // +0x378 "smallbar_cp"
    UTexture* LevelBackTex;          // +0x37C "ps_levelback"
    UTexture* SizeControl2Tex;       // +0x380 "ps_sizecontrol2"
    User* Player;                    // +0x384 set by each paint
    INT Unknown358;                  // +0x388 (the constructor clears it)
    NCResizeFrame* ResizeFrame;      // +0x38C
    NCWnd* NameCtrl;                 // +0x390 (new in C4) NCNameCtrl (its class is another file's): shows the player's name (SetText by each paint)

    NCPlayerStatusWnd(INT InBarWidth = 100);                               // 0x100d4990 (C1 only: no C2 match) (C1 only: no C2 match)
    // ~NCPlayerStatusWnd                                            // 0x100d49f0 (C1 only: no C2 match) (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x1010d770
    virtual INT OnCreate();                                          // 37 0x10116c90
    virtual INT OnDestroy();                                         // 38 0x1010f4c0
    virtual INT OnPaint(NCGDevice* Device);                          // 62 0x10113be0
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 68 0x1010d7d0
    // names ours
    virtual INT OnResize(INT Delta);                                 // 75 0x1010f5c0 (the stock WindowProc passes lParam of the edge's command)

    INT Resize(INT Delta);                                           // 0x100475f0 (name ours)
};
NW_ASSERT_OFFSET(NCPlayerStatusWnd, HPBar, 0x364);
NW_ASSERT_OFFSET(NCPlayerStatusWnd, Player, 0x384);
NW_ASSERT_OFFSET(NCPlayerStatusWnd, ResizeFrame, 0x38C);
NW_ASSERT_SIZE(NCPlayerStatusWnd, 0x394);

// NCTargetStatusWnd - the target (UClass "CTargetStatusWnd", 0x310): its name, HP / MP bars
// for monsters, siege NPCs, the player, its pet and party members; a door or other static
// object shows its name only. For players the window can be expanded to show the clan and
// the alliance. A click targets again (Ctrl: attacks); the close button cancels the target.
class NCTargetStatusWnd : public NCStatusWnd
{
    NW_DECLARE_CLASS(NCTargetStatusWnd, NCStatusWnd, 0, 0x10309138)

    NCButton* CloseButton;           // +0x364 "FrameCloseBtn"
    INT Unknown338;                  // +0x368   // C2: offset derived, no evidence
    INT LevelDiff;                   // +0x36C the player's level minus the target's (name color)
    INT HoverBuff;                   // +0x370 (new in C3, name ours) 1 + the index of the buff icon under the mouse, 0 = none
    UBOOL bExpanded;                 // +0x374 the buff icons / the clan and alliance lines are shown
    NCButton* ToggleButton;          // +0x378 "Target_InfoButton"
    UTexture* BackTex3[6];           // +0x37C (C3) "SmallWindow_Back1".."3" (the short window), "SmallWindow2_Back1".."3" (the tall one)   // C2: offset derived, no evidence
    NToolTipInfo BuffToolTip;        // +0x394 (new in C3) the tool tip of the buff icon under the mouse
    NCResizeFrame* ResizeFrame;      // +0x414 (new in C4) ID 0xF20F15: the right edge that widens the window (OnResize)
    NCWnd* NameCtrl;                 // +0x418 (new in C4) NCNameCtrl: the target's name (SetText by each paint)

    NCTargetStatusWnd(INT InBarWidth = 100);                               // 0x10116ae0
    // ~NCTargetStatusWnd                                            // 0x10116b80

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x1010d500
    virtual INT OnCreate();                                          // 37 0x10112d70
    virtual INT OnDestroy();                                         // 38 0x1010f1f0
    virtual INT OnPaint(NCGDevice* Device);                          // 62 0x101132a0
    virtual INT OnDrawToolTip(NCGDevice* Device);                    // 63 0x10115e70 (the db calls it OnPostPaint)
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);           // 67 0x10113af0
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 68 0x1010d5a0
    // names ours
    virtual void NewVirtual75();  // 75 0x1010f320 (only this client has it; defined in NNewVirtuals.gen.cpp): OnResize(INT Delta), see OnResizeImpl
    virtual INT OnClose();                                           // 76 0x1010d6f0
    virtual INT OnToggle();                                          // 77 0x1010d640

    void SetLevelDiff(INT Diff);                                     // 0x1010d630 (name ours)
    // 0x1010f320 (name ours): slot 75, which the stock WindowProc calls with the edge's lParam. The slot keeps the placeholder
    // NewVirtual75 (NNewVirtuals.gen.cpp defines it as a jump to this address, where the hook of this method sits)
    INT OnResizeImpl(INT Delta);
};
NW_ASSERT_OFFSET(NCTargetStatusWnd, CloseButton, 0x364);
NW_ASSERT_OFFSET(NCTargetStatusWnd, LevelDiff, 0x36C);
NW_ASSERT_OFFSET(NCTargetStatusWnd, bExpanded, 0x374);
NW_ASSERT_OFFSET(NCTargetStatusWnd, ToggleButton, 0x378);
NW_ASSERT_OFFSET(NCTargetStatusWnd, BackTex3, 0x37C);
NW_ASSERT_OFFSET(NCTargetStatusWnd, BuffToolTip, 0x394);
NW_ASSERT_SIZE(NCTargetStatusWnd, 0x41C);

// L2 Ignition: the buff icons at the right of a party member's, the pet's and the summon's windows, laid out like the
// player's own buff window: rows of 10, up to 3 (30, what the server sends: PartySpelled), in the order the server lists
// them, 15 pixels from the window's top (the stock drew two rows of 10 at 16 pixels from y 5). A member's window is 46
// high: three rows of 15 take 45, so the members stay where they were.
enum { PARTY_BUFFS_PER_ROW = 10, PARTY_BUFF_ROWS = 3, PARTY_BUFF_SIZE = 15 };
inline INT PartyShownBuffs(INT Num)
{
    return Num < PARTY_BUFFS_PER_ROW * PARTY_BUFF_ROWS ? Num : PARTY_BUFFS_PER_ROW * PARTY_BUFF_ROWS;
}

// NCPartyStatusWnd - one party member (UClass "CPartyStatusWnd", 0x300): name, HP / MP bars.
// A click targets the member (Ctrl: attacks), a right click targets what the member targets.
class NCPartyStatusWnd : public NCStatusWnd
{
    NW_DECLARE_CLASS(NCPartyStatusWnd, NCStatusWnd, 0, 0x1030A0D0)

    INT MemberID;                    // +0x364 object id of the member (-1)
    TArray<INT> BuffSkillIDs;        // +0x368 (new in C3, name ours) the member's buffs: skill ids (AddBuff)
    TArray<UTexture*> BuffIcons;     // +0x374 (new in C3) their icon textures, one per skill id
    UTexture* DebuffFrameTex;        // +0x380 (new in C3) "L2UI_CH3.PlayerStatusWnd.debuff" drawn over the special ones
    NCWnd* NameCtrl;                 // +0x384 (new in C4) NCNameCtrl (its class is another file's): the member's name (SetText by each paint)
    INT LeaderID;                    // +0x388 (new in C3) object id of the party leader (SetInfo's first value)
    INT LootRule;                    // +0x38C (new in C3) the party's item distribution, 0..4 (SetInfo's third value)
    INT Unknown390;                  // +0x390 (new in C3, was Unknown35C) the constructor clears it
    NCButton* StyleIcon;             // +0x394 "party_styleicon1": the class group's icon (UpdateStyle sets its textures and tool tip)
    NCButton* LeaderIcon;            // +0x398 (new in C3) "party_leadericon", shown while MemberID == LeaderID (UpdateStyle)

    NCPartyStatusWnd(INT InBarWidth = 100);                                // 0x10117340

    virtual INT OnCreate();                                          // 37 0x10114d80
    virtual INT OnDestroy();                                         // 38 0x10110340
    virtual INT OnPaint(NCGDevice* Device);                          // 62 0x10115080
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 68 0x1010daa0
    virtual INT OnRButtonUp(WPARAM wParam, LPARAM lParam);           // 71 0x1010db60
    virtual void Clear();                                            // 74 0x10115620 (the base's, then the buffs)

    // names ours. C4: the member's record is popped from the packet's parameters here (C3: NCPartyWnd::AddMember popped
    // them and called SetInfo with nine arguments, and UpdateInfo took the later updates; C4 has neither)
    void SetInfo(L2ParamStack& Params);                              // 0x101103e0 (ret 4)
    __declspec(noinline) void UpdateStyle();                         // 0x1010dc00 shows the class icon and the leader icon, sets their tool tips
    __declspec(noinline) void ClearBuffs();                          // 0x10100ca0 (C1 only: no C2 match) (the same code as the end of Clear)
    void AddBuff(INT SkillID, INT Unused);                           // 0x101147f0 (ret 8)
};
NW_ASSERT_OFFSET(NCPartyStatusWnd, MemberID, 0x364);
NW_ASSERT_OFFSET(NCPartyStatusWnd, BuffSkillIDs, 0x368);
NW_ASSERT_OFFSET(NCPartyStatusWnd, BuffIcons, 0x374);
NW_ASSERT_OFFSET(NCPartyStatusWnd, DebuffFrameTex, 0x380);
NW_ASSERT_OFFSET(NCPartyStatusWnd, NameCtrl, 0x384);
NW_ASSERT_OFFSET(NCPartyStatusWnd, LeaderID, 0x388);
NW_ASSERT_OFFSET(NCPartyStatusWnd, StyleIcon, 0x394);
NW_ASSERT_OFFSET(NCPartyStatusWnd, LeaderIcon, 0x398);
NW_ASSERT_SIZE(NCPartyStatusWnd, 0x39C);

// NCPetStatusWnd - the pet (UClass "CPetStatusWnd", 0x3D0): name, HP / MP bars and the food
// bar, from what the network handler keeps about the pet (read again by each paint). Shown
// only while the player has a pet (not a summon); a click right of the title bar acts on it.
class NCPetStatusWnd : public NCStatusWnd
{
    NW_DECLARE_CLASS(NCPetStatusWnd, NCStatusWnd, 0, 0x10309BA0)

    NCButton* ToggleButton;          // +0x364 (C3) "pet_button" (ID 0xF0F00A1): buff icons on / off
    TArray<INT> BuffSkillIDs;        // +0x368 (C3, name ours) the pet's buffs: skill ids
    TArray<UTexture*> BuffIcons;     // +0x374 (C3) their icon textures
    UTexture* FedTex;                // +0x380 "L2UI_CH3.SmallWnd.FATIGUEBAR"
    UTexture* FedBackTex;            // +0x384 "L2UI_CH3.SmallWnd.FATIGUEBAR_BACK"
    UTexture* PetFrameTopTex;        // +0x388 "L2UI_CH3.FrameCtrl.smallbar1"
    UTexture* PetFrameCenterTex;     // +0x38C "smallbar2"
    UTexture* PetFrameBottomTex;     // +0x390 "smallbar3"
    NCWnd* NameCtrl;                 // +0x394 (new in C4) NCNameCtrl: the pet's name (SetText by each paint)
    PetInfo Pet;                     // +0x398 (0xAC) copy of the handler's record, read again by each paint
    INT Unknown410;                  // +0x44C (0) the constructor clears it
    INT Unknown414[2];               // +0x450 (no stock code touches them)   // C2: offset derived, no evidence
    INT Unknown41C;                  // +0x458 SetInfo
    INT Unknown420;                  // +0x45C
    INT Unknown424;                  // +0x460
    UBOOL bBuffs;                    // +0x464 (name ours) buff icons shown (OnToggle)
    NCResizeFrame* ResizeFrame;      // +0x468 (new in C4) ID 0xF0F00A2: the right edge that widens the window

    NCPetStatusWnd(INT InBarWidth = 100);                                  // 0x100d54d0 (C1 only: no C2 match) (C1 only: no C2 match)
    // ~NCPetStatusWnd                                               // 0x100d5540 (C1 only: no C2 match) (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x1010d7f0 (folded)
    virtual INT OnCreate();                                          // 37 0x10114400
    virtual void ShowWindow();                                       // 48 0x1010d950
    virtual INT OnPaint(NCGDevice* Device);                          // 62 0x1010f770
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 68 0x1010d8f0

    void SetInfo(L2ParamStack& Params);                              // 0x1010d870 (name ours)
    virtual void NewVirtual75();  // 75 0x1010fb80 (only this client has it; defined in NNewVirtuals.gen.cpp): OnResize(INT Delta), see OnResizeImpl
    virtual INT OnToggle();                                          // 76 0x1010d990 (C3, name ours) the button's command
    INT OnResizeImpl(INT Delta);                                     // 0x1010fb80 (name ours), see NCTargetStatusWnd
    virtual INT OnDestroy();                                         // 38 0x10115ff0 (guard name: NCSummonedStatusWnd's)
    __declspec(noinline) void ClearBuffs();                          // the code of NCPartyStatusWnd's (0x10100ca0)
};
NW_ASSERT_OFFSET(NCPetStatusWnd, BuffSkillIDs, 0x368);
NW_ASSERT_OFFSET(NCPetStatusWnd, BuffIcons, 0x374);
NW_ASSERT_OFFSET(NCPetStatusWnd, FedTex, 0x380);
NW_ASSERT_OFFSET(NCPetStatusWnd, Pet, 0x398);
NW_ASSERT_OFFSET(NCPetStatusWnd, Unknown410, 0x44C);
NW_ASSERT_OFFSET(NCPetStatusWnd, Unknown41C, 0x458);
NW_ASSERT_OFFSET(NCPetStatusWnd, NameCtrl, 0x394);
NW_ASSERT_OFFSET(NCPetStatusWnd, bBuffs, 0x464);
NW_ASSERT_OFFSET(NCPetStatusWnd, ResizeFrame, 0x468);
NW_ASSERT_SIZE(NCPetStatusWnd, 0x46C);

// NCSummonedStatusWnd - the summon (UClass "CSummonedStatusWnd", 0x3BC): name, HP / MP bars
// and a bar for the time it has left, run by the console (StartTimer, SetRemaining). Shown only
// while the player has a summon (not a pet).
class NCSummonedStatusWnd : public NCStatusWnd
{
    NW_DECLARE_CLASS(NCSummonedStatusWnd, NCStatusWnd, 0, 0x1030B070)

    TArray<INT> BuffSkillIDs;        // +0x364 (C3, name ours) the summon's buffs: skill ids
    TArray<UTexture*> BuffIcons;     // +0x370 (C3) their icon textures
    UTexture* SummonFrameTopTex;     // +0x37C "L2UI_CH3.FrameCtrl.smallbar1"
    UTexture* SummonFrameCenterTex;  // +0x380 "smallbar2"
    UTexture* SummonFrameBottomTex;  // +0x384 "smallbar3"
    NCProgressCtrl* TimeBar;         // +0x388 (ID 0xF0A0B0)
    NCButton* ToggleButton;          // +0x38C (C3) "pet_button" (ID 0xF0A0B1): buff icons on / off
    UBOOL bBuffs;                    // +0x390 (name ours) buff icons shown (OnToggle)
    NCWnd* NameCtrl;                 // +0x394 (new in C4) NCNameCtrl: the summon's name (SetText by each paint)
    PetInfo Summon;                  // +0x398 (0xAC) copy of the handler's record, read again by each paint
    UBOOL bTimerStarted;             // +0x44C (StartTimer sets it)
    INT Unknown450;                  // +0x450 (0) the constructor clears it; nothing else reads it
    NCResizeFrame* ResizeFrame;      // +0x454 (new in C4) ID 0xF0A0B2: the right edge that widens the window

    NCSummonedStatusWnd(INT InBarWidth = 100);                             // 0x100d5950 (C1 only: no C2 match) (C1 only: no C2 match)
    // ~NCSummonedStatusWnd                                          // 0x100d59c0 (C1 only: no C2 match) (C1 only: no C2 match)

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 25 0x1010d9b0 (folded)
    virtual INT OnCreate();                                          // 37 0x10114850
    virtual INT OnDestroy();                                         // 38 0x101160f0
    virtual void ShowWindow();                                       // 48 0x1010da60
    virtual INT OnPaint(NCGDevice* Device);                          // 62 0x1010fdf0
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 68 0x1010d8f0

    // names ours
    void SetRemaining(DWORD Remaining);                              // 0x1010da50
    void StartTimer(DWORD Max);                                      // 0x10116220
    virtual void NewVirtual75();  // 75 0x10110180 (only this client has it; defined in NNewVirtuals.gen.cpp): OnResize(INT Delta), see OnResizeImpl
    INT OnResizeImpl(INT Delta);                                     // 0x10110180 (name ours), see NCTargetStatusWnd
    virtual INT OnToggle();                                          // 76 0x1010da30 (C3, name ours) the button's command
    __declspec(noinline) void ClearBuffs();                          // 0x10114ce0 (name ours)
    void AddBuff(INT SkillID, INT Unused);                           // 0x10114d20 (name ours; ret 8)
};
NW_ASSERT_OFFSET(NCSummonedStatusWnd, BuffSkillIDs, 0x364);
NW_ASSERT_OFFSET(NCSummonedStatusWnd, TimeBar, 0x388);
NW_ASSERT_OFFSET(NCSummonedStatusWnd, bBuffs, 0x390);
NW_ASSERT_OFFSET(NCSummonedStatusWnd, Summon, 0x398);
NW_ASSERT_OFFSET(NCSummonedStatusWnd, bTimerStarted, 0x44C);
NW_ASSERT_OFFSET(NCSummonedStatusWnd, Unknown450, 0x450);
NW_ASSERT_OFFSET(NCSummonedStatusWnd, NameCtrl, 0x394);
NW_ASSERT_OFFSET(NCSummonedStatusWnd, ResizeFrame, 0x454);
NW_ASSERT_SIZE(NCSummonedStatusWnd, 0x458);

#pragma pack(pop)