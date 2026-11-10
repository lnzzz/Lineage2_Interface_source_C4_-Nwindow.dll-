/*=============================================================================
    NConsoleWndEvents.cpp - what the engine tells the console in the game: each
    UGameEngine::On* handler of a server packet calls one of its UL2ConsoleWnd slots (named
    after the handler). Here: the target, the HTML pages, the tutorial, the radar, the clan,
    the alliance, the friends, the party and its matching, the chat channels, the shortcuts,
    the buffs, the skills and the trainer, the restart menu, the quests, the system messages,
    the pet and the summon, the siege, a GM's views. Most pass the news on to a window of the
    desktop; the answers to requests show a system message, the offers a dialog box. The login
    stages' are in NConsoleWndLogin.cpp, the items' in NConsoleWndItems.cpp.
=============================================================================*/
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NConsoleWndC4.h"
#include "nwindow/NConsoleWndExt.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NDialogInfo.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NCGaraDialogBox.h"
#include "nwindow/NCEditBox.h"
#include "nwindow/NCStatusWnd.h"
#include "nwindow/NCAbnormalStatusWnd.h"
#include "nwindow/NCTutorialViewerWnd.h"
#include "nwindow/NCNPCHtmlViewer.h"
#include "nwindow/NCBoardWnd.h"
#include "nwindow/NCRadarWnd.h"
#include "nwindow/NCShortCutWnd.h"
#include "nwindow/NCMacroListWnd.h"
#include "nwindow/NCPartyWnd.h"
#include "nwindow/NCChatWnd.h"
#include "nwindow/NCMatchWnd.h"
#include "nwindow/NCMainWnd.h"
#include "nwindow/NCActionWnd.h"
#include "nwindow/NCPledgeWnd.h"
#include "nwindow/NCQuestWnd.h"
#include "nwindow/NCQuestGaraTreeWnd.h"
#include "nwindow/NCQuestGaraTreeCtrl.h"
#include "nwindow/NCGaraTVItem.h"
#include "nwindow/NCGMWnd.h"
#include "nwindow/NCMagicSkillWnd.h"
#include "nwindow/NCSkillWnd.h"
#include "nwindow/NCTrainWnd.h"
#include "nwindow/NCRestartMenuWnd.h"
#include "nwindow/NCPetWnd.h"
#include "nwindow/NCSummonedWnd.h"
#include "nwindow/NCProgressCtrl.h"
#include "nwindow/NCSiegeInfoMainWnd.h"
#include "nwindow/NCDetailStatusWnd.h"
#include "nwindow/NCMinimapWnd.h"
#include "nwindow/NCItemWnd.h"
#include "nwindow/NCSkillItemWnd.h"
#include "nwindow/NCInventoryWnd.h"
#include "nwindow/NCPetInventoryWnd.h"
#include "nwindow/NCTrainListBox.h"
#include "nwindow/NCLoadingWnd.h"
#if NW_CUSTOM
#include "nwindow/NCustomDamage.h"
#endif

#if NW_FEATURES
// phase 6: AutoShot (NWindow_C1 src/inject/AutoShot.cpp), CraftUi (NWindow_C1
// src/inject/craft/CraftUi.cpp)
#include "features/AutoShot.h"
#include "features/craft/CraftUi.h"
// phase 6: ChatColors (not in NWindow_C1: the Interlude colors of the chat types C1 draws white)
#include "features/ChatColors.h"
#endif

// system messages (systemmsg ids)
enum
{
    MSG_AskJoinPledge = 67,            // "$s1 has invited you to the $s2 Clan. Do you want to join?"
    MSG_ClanCreated = 189,             // "Clan has been created."
    MSG_ClanCreateFailed = 190,        // "You have failed to create a clan."
    MSG_ClanDispersed = 193,           // "Clan has dispersed."
    MSG_ClanDisperseFailed = 194,      // "You have failed to disperse the Clan."
    MSG_ClanWithdrawn = 197,           // "Withdrawn from the clan."
    MSG_ClanWithdrawFailed = 1120,     // "You have failed to withdraw from the $s1 Clan." (C1: 198)
    MSG_ClanMembershipEnded = 199,     // "Membership to this clan has been terminated."
    MSG_PartyWithdrawn = 200,          // "You have withdrawn from the party."
    MSG_PartyExpelled = 202,           // "You have been expelled from the party."
    MSG_PartyDispersed = 203,          // "The party has dispersed."
    MSG_AskPledgeWar = 207,            // "$s2 of the $s1 Clan requests declaration of war. Do
                                       //   you accept?"
    MSG_PledgeTruceDenied = 228,       // "Request to end war has been denied."
    MSG_AskPledgeTruce = 255,          // "The $s1 Clan has requested to end war. Do you agree?"
    MSG_PartyDeclined = 305,           // "The player declined to join your party."
    MSG_ClanJoinFailed = 308,          // "Failed to join the clan"
    MSG_ClanExpelDone = 309,           // "Succeeded in expelling a clan member."
    MSG_ClanExpelFailed = 310,         // "Failed to expel a clan member."
    MSG_PledgeWarAccepted = 311,       // "Clan war has been accepted."
    MSG_PledgeWarRefused = 312,        // "Clan war has been refused."
    MSG_PledgeTruceAccepted = 313,     // "The cease war request has been accepted."
    MSG_SurrenderFailed = 314,         // "Failed to surrender."
    MSG_PersonalSurrenderFailed = 315, // "Ffailed to personally surrender."
    MSG_PartyWithdrawFailed = 316,     // "Failed to withdraw from the party."
    MSG_PartyExpelFailed = 317,        // "Failed to expel a party member."
    MSG_PartyDisperseFailed = 318,     // "Failed to disperse the party."
    MSG_AskFriend = 516,               // "$s1 has invited you as a Friend."
    MSG_AllyAccepted = 517,            // "You have accepted the alliance."
    MSG_AllyInviteFailed = 518,        // "You have failed to invite a clan into the alliance."
    MSG_AllyWithdrawn = 519,           // "You have withdrawn from the alliance."
    MSG_AllyWithdrawFailed = 520,      // "You have failed to withdraw from the alliance."
    MSG_AllyExpelDone = 521,           // "You have succeeded in expelling a clan."
    MSG_AllyExpelFailed = 522,         // "You have failed to expel a clan."
    MSG_AllyDissolved = 523,           // "The alliance has dissolved."
    MSG_AllyDissolveFailed = 524,      // "You have failed to dissolve the alliance."
    MSG_FriendInvited = 525,           // "You have succeeded in inviting a Friend."
    MSG_FriendInviteFailed = 526,      // "You have failed to invite a Friend."
    MSG_AskJoinAlly = 527,             // "$s2, the alliance leader of $s1 has requested an
                                       //   alliance."
    MSG_AskJoinPartyLooter = 572,      // "Do you wish to consent to $s1's party invitation?
                                       //   (Item distribution: possession of the looter)"
    MSG_AskJoinPartyRandom = 573,      // "... (Item distribution: random distribution to party
                                       //   members)"
    MSG_AskJoinPartyRandomSpoil = 967, // C2: three more ways to share (random including spoil, by turn,
    MSG_AskJoinPartyByTurn = 968,      //   by turn including spoil)
    MSG_AskJoinPartyByTurnSpoil = 969,
    MSG_AskAllyWar = 665,              // "$s2% of $s1 alliance is applying for alliance war. Do
                                       //   you want to accept the challenge?"
    MSG_AskAllyTruce = 666,            // "A request for ceasefire has been received from $s1
                                       //   alliance. Do you want to agree to the cease-fire?"
};

// the dialog boxes opened here (NDialogInfo::ID; NCDialogBox sends the answer)
enum
{
    DIALOG_JOIN_PARTY = 0x0A,
    DIALOG_JOIN_PLEDGE = 0x0E,
    DIALOG_START_PLEDGE_WAR = 0x13,
    DIALOG_STOP_PLEDGE_WAR = 0x15,
    DIALOG_SYSTEM_MESSAGE = 0x1F,      // OK only
    DIALOG_JOIN_ALLY = 0x33,           // C2: C1's 0x2F..0x37 moved by 4 (new kinds before them)
    DIALOG_START_ALLY_WAR = 0x34,
    DIALOG_STOP_ALLY_WAR = 0x35,
    DIALOG_FRIEND_INVITE = 0x3B,
};

// sysstrings: the clan's status texts, the announcements' speaker
enum
{
    STR_Announcement = 328,
    STR_PledgeStatus1 = 339,
    STR_PledgeStatus4 = 340,
    STR_PledgeStatus3 = 341,
    STR_PledgeStatus2 = 349,
};

// Say2 types, the chat channels (names ours, after the protocol's)
enum
{
    SAY2_Shout = 1,
    SAY2_Tell = 2,
    SAY2_Party = 3,
    SAY2_Pledge = 4,
    SAY2_PetitionPlayer = 6,
    SAY2_PetitionGM = 7,
    SAY2_Market = 8,
    SAY2_Ally = 9,
    SAY2_Announcement = 10,
};

// interface sounds (FL2NetNotify::OnInterfacePlaySound; after the engine's list)
enum { SOUND_WindowOpen = 5, SOUND_WindowClose = 6 };

// NItemInfo as this client lays it out from +0x178C on (the same accessors as NConsoleWndItems.cpp: the header has the
// C2 offsets there, a new dword sits at +0x178C and the fields after it are 4 bytes later)
#define NINFO_C3(Info, Type, Off) (*(Type*)((BYTE*)&(Info) + (Off)))
static FORCEINLINE UBOOL& InfoStackable(NItemInfo& I) { return NINFO_C3(I, UBOOL, 0x1794); }   // C2 +0x1790
static FORCEINLINE INT& InfoC90(NItemInfo& I) { return NINFO_C3(I, INT, 0x179C); }             // C2 +0x1798 (-1)
static FORCEINLINE INT& InfoC17A0(NItemInfo& I) { return NINFO_C3(I, INT, 0x17A4); }           // C2 +0x17A0

// NPledgeInfo as this client builds it (0x100475A0 clears up to +0x634): three dwords more than the header's
// (NCPledgeWnd.h still has C2's 0x62C bytes); the engine's PledgeInfo +0xD0 / +0xD4 / +0xD8 go into them
struct NPledgeInfoC3 : public NPledgeInfo
{
    INT Unknown62C;
    INT Unknown630;
    INT Unknown634;
    NPledgeInfoC3() : Unknown62C(0), Unknown630(0), Unknown634(0) {}
};
NW_ASSERT_SIZE(NPledgeInfoC3, 0x638);

// a line of questname-*.dat as this client lays it out: 2000 records of 0xA0 bytes at +0x3D28 of the game data (C3: 1000 of 0x70 at
// +0x3D18); only the first fields are read here (NConsoleWndData.cpp has the whole view)
struct NQuestHeadC4
{
    INT bValid;               // +0x00 0: no quest here
    INT QuestID;              // +0x04
    INT Step;                 // +0x08
    BYTE _Unknown0C[0xA0 - 0x0C];
};
NW_ASSERT_SIZE(NQuestHeadC4, 0xA0);
#define GQuestHeadsC4 ((NQuestHeadC4*)((BYTE*)&GL2GameData + 0x3D28))

// the pawn the player's controller drives (as NConsoleWndNet.cpp's)
static FORCEINLINE APawn* PlayerPawn(UEngine* Engine)
{
    return Engine->Client->Viewports(0)->Actor->Pawn;
}

/*-----------------------------------------------------------------------------
    The target, the HTML pages, the tutorial, the radar.
-----------------------------------------------------------------------------*/

// 0x10071cf0 - a target was selected: its level against the player's. C2: with the GM window open, a selected
// player (not a creature: User +0x08 is 0) also goes into the GM window's name line
void NConsoleWnd::MyTargetSelected(INT LevelDiff)
{
    GetDesktop()->TargetStatusWnd->SetLevelDiff(LevelDiff);
    APawn* Pawn = PlayerPawn(Engine);
    if (Engine && Pawn && Pawn->Controller && Pawn->Controller->Target)
    {
        User* Target = FindUserByID(Pawn->Controller->Target->CreatureID);
        if (Target && !Target->Unknown8 && GetDesktop()->GMWnd->IsShowWindow())
        {
            GetDesktop()->GMWnd->NameEdit->ClearText();
            GetDesktop()->GMWnd->NameEdit->InsertText((TCHAR*)((BYTE*)Target + 0x1C), 1);
        }
    }
}
NW_HOOK_METHOD(0x10071cf0, NConsoleWnd, MyTargetSelected)

// 0x1007a2f0 - what an action's shortcut asks of the client: 4 the next target, 5 pick up,
// 6 assist
void NConsoleWnd::ReceiveAction(INT Action)
{
    switch (Action)
    {
    case 4:
    {
        User* Target = GetSelectedActor();
        if (Target && Target->Pawn)
            RequestTargetNext(200, Target->Pawn->CreatureID);
        else
            RequestTargetNext(200, -1);
        break;
    }
    case 5:
        RequestPickUp(200, -1);
        break;
    case 6:
        RequestAssist();
        break;
    }
}
NW_HOOK_METHOD(0x1007a2f0, NConsoleWnd, ReceiveAction)

// 0x10081520 (vslot 77) - an NPC's page and a second value: 0 shows it in the NPC window (+0xF8), titled with the sysstring 444
// (the slot 56 takes a string id); an item id (above 0) in the second window (+0xFC) titled with the item's name; the window
// takes the focus and the dialog sound plays. (The C2 build's craft / auto-shot message filters are not stock and are not here.)
void NConsoleWnd::NpcHtmlMessage(L2ParamStack& Params)
{
    TCHAR* Html = (TCHAR*)Params.Top();
    INT Title = (INT)Params.Top();
    GConsole->PlayInterfaceSound(SOUND_WindowOpen);
    if (Title == 0)
    {
        GetDesktop()->NpcHtmlWnd->ShowPage(Html);
        GetDesktop()->NpcHtmlWnd->ShowWindow();
        GetDesktop()->NpcHtmlWnd->SetFocus();
        nw::VirtualCall<void>(GetDesktop()->NpcHtmlWnd, 56, 0x1BC);
    }
    else if (Title > 0)
    {
        nw::Field<NCNPCHtmlViewer*>(Desktop, 0xFC)->ShowPage(Html);
        nw::Field<NCNPCHtmlViewer*>(Desktop, 0xFC)->ShowWindow();
        nw::Field<NCNPCHtmlViewer*>(Desktop, 0xFC)->SetFocus();
        nw::Field<NCNPCHtmlViewer*>(Desktop, 0xFC)->SetTitle(GetItemName(Title));
    }
}
NW_HOOK_METHOD(0x10081520, NConsoleWnd, NpcHtmlMessage)

// 0x10068dc0 (vslot 192) - the community board: the byte hides it when 0; else the bypass commands of its eight buttons, then the
// page; the board takes the focus
void NConsoleWnd::ShowBoard(INT bShow, L2ParamStack& Params)
{
    if (!(BYTE)bShow)
    {
        GetDesktop()->BoardWnd->HideWindow();
        return;
    }
    TCHAR* Command1 = (TCHAR*)Params.Top();
    TCHAR* Command2 = (TCHAR*)Params.Top();
    TCHAR* Command3 = (TCHAR*)Params.Top();
    TCHAR* Command4 = (TCHAR*)Params.Top();
    TCHAR* Command5 = (TCHAR*)Params.Top();
    TCHAR* Command6 = (TCHAR*)Params.Top();
    TCHAR* Command7 = (TCHAR*)Params.Top();
    TCHAR* Command8 = (TCHAR*)Params.Top();
    TCHAR* Html = (TCHAR*)Params.Top();
    if (Html)
    {
        static_cast<NCBoardWnd*>(GetDesktop()->BoardWnd)->LoadPage(Html);
        // NCBoardWnd's 8-command setter (0x1003B800)
        static_cast<NCBoardWnd*>(GetDesktop()->BoardWnd)->SetCommands(Command1, Command2, Command3, Command4, Command5,
                                                                       Command6, Command7, Command8);
        GetDesktop()->BoardWnd->ShowWindow();
        GetDesktop()->BoardWnd->SetFocus();
    }
}
NW_HOOK_METHOD(0x10068dc0, NConsoleWnd, ShowBoard)

// 0x10068d70 - a page of the tutorial (C3: the window takes the focus)
void NConsoleWnd::TutorialShowHtml(TCHAR* Html)
{
    GetDesktop()->TutorialWnd->ShowPage(Html);
    GetDesktop()->TutorialWnd->ShowWindow();
    GetDesktop()->TutorialWnd->SetFocus();
}
NW_HOOK_METHOD(0x10068d70, NConsoleWnd, TutorialShowHtml)

// the tutorial's question mark (the desktop's +0x17C; no header): a window of its own with its question number and three timers
// NTutorialMarkWnd (NConsoleWndExt.h)

// 0x10002210 (name ours) - the blink state cleared and the three timers (0xF0E0F0 with the interval of +0x2B4, 0xF0E0F1 every 10
// and 0xF0E0F2 every 50 ms) started; their numbers kept
void NTutorialMarkWnd::StartTimers()
{
    nw::Field<INT>(this, 0x2DC) = 0x32;
    nw::Field<INT>(this, 0x2D8) = 0;
    nw::Field<BYTE>(this, 0x2E0) = 1;
    nw::Field<INT>(this, 0x2B8) = SetTimer(0xF0E0F0, nw::Field<INT>(this, 0x2B4), NULL);
    nw::Field<INT>(this, 0x2D0) = SetTimer(0xF0E0F1, 10, NULL);
    nw::Field<INT>(this, 0x2D4) = SetTimer(0xF0E0F2, 0x32, NULL);
    nw::Field<BYTE>(this, 0x2E1) = 1;
}
NW_HOOK_METHOD(0x10002210, NTutorialMarkWnd, StartTimers)

// 0x10002280 (name ours) - the question the mark stands for, and the timers started
void NTutorialMarkWnd::SetQuestion(INT ID)
{
    nw::Field<INT>(this, 0x2C4) = ID;
    StartTimers();
}
NW_HOOK_METHOD(0x10002280, NTutorialMarkWnd, SetQuestion)

// 0x10068f20 - the tutorial's question mark, sent back when clicked: the window takes the question, is shown with the focus and
// the desktop does its 0x1005CF40 (the stock asks the global console)
void NConsoleWnd::TutorialShowQuestionMark(INT ID)
{
    ((NTutorialMarkWnd*)nw::Field<NCWnd*>(Desktop, 0x17C))->SetQuestion(ID);
    nw::Field<NCWnd*>(Desktop, 0x17C)->ShowWindow();
    nw::Field<NCWnd*>(Desktop, 0x17C)->SetFocus();
    GConsole->GetDesktop()->DockAboveChat();
}
NW_HOOK_METHOD(0x10068f20, NConsoleWnd, TutorialShowQuestionMark)

// 0x10068f80 - the client events the tutorial waits for (SendTutorialEvent)
void NConsoleWnd::TutorialEnableClientEvent(INT Events)
{
    TutorialEvents = Events;
}
NW_HOOK_METHOD(0x10068f80, NConsoleWnd, TutorialEnableClientEvent)

// 0x10068f90
void NConsoleWnd::TutorialCloseHtml()
{
    GetDesktop()->TutorialWnd->HideWindow();
}
NW_HOOK_METHOD(0x10068f90, NConsoleWnd, TutorialCloseHtml)

// 0x1005e5a0 (vslot 83) - what to do with a point on the radar (the desktop's +0x164) and the minimap (the window at
// +0xC8 of the desktop's +0x14C group): Param1 0 adds it (0x100DE780 / 0x100AF450), 1 removes it (0x100DE7C0 /
// 0x100AF490), 2 clears both (0x100DE820 / 0x100AF610); Param2 chooses the radar (0), the minimap (1) or both (2).
// C3: before an add or a clear the console tells itself (virtual 354: a flag of the character window's page, +0xDD) and
// an add also hands the point, as a vector, to its virtual 357
void NConsoleWnd::ShowRadar(INT Param1, INT Param2, INT X, INT Y, INT Z)
{
    NCWnd* Minimap = NULL;
    if (Param1 == 0)
    {
        nw::VirtualCall<void>(this, 376, 1);
        FVector Point((FLOAT)X, (FLOAT)Y, (FLOAT)Z);
        nw::VirtualCall<void>(this, 379, Point);
        if (Param2 == 0)
        {
            GetDesktop()->RadarWnd->AddTarget(X, Y, Z);
        }
        else if (Param2 == 1 || Param2 == 2)
        {
            if (Param2 == 2)
                GetDesktop()->RadarWnd->AddTarget(X, Y, Z);
            Minimap = nw::Field<NCWnd*>(nw::Field<NCWnd*>(GetDesktop(), 0x170), 0xE8);
            static_cast<NCMinimapWnd*>(Minimap)->AddTarget(X, Y, Z);
        }
    }
    else if (Param1 == 2)
    {
        nw::VirtualCall<void>(this, 376, 0);
        GetDesktop()->RadarWnd->ClearTargets();
        static_cast<NCMinimapWnd*>(nw::Field<NCWnd*>(nw::Field<NCWnd*>(GetDesktop(), 0x170), 0xE8))->ClearTargets();
    }
    else if (Param1 == 1)
    {
        if (Param2 == 0)
        {
            ((NCRadarWndExt*)GetDesktop()->RadarWnd)->RemoveTarget(X, Y, Z);
        }
        else if (Param2 == 1 || Param2 == 2)
        {
            if (Param2 == 2)
                ((NCRadarWndExt*)GetDesktop()->RadarWnd)->RemoveTarget(X, Y, Z);
            Minimap = nw::Field<NCWnd*>(nw::Field<NCWnd*>(GetDesktop(), 0x170), 0xE8);
            static_cast<NCMinimapWnd*>(Minimap)->RemoveTarget(X, Y, Z);
        }
    }
}
NW_HOOK_METHOD(0x100695c0, NConsoleWnd, ShowRadar)

// 0x10069790 (vslot 84) - a point off the radar and the minimap: the radar's RemoveTarget (0x100DE7C0) and the one of
// the window inside the desktop's +0x14C group (0x100AF490)
void NConsoleWnd::DeleteRadar(INT X, INT Y, INT Z)
{
    ((NCRadarWndExt*)GetDesktop()->RadarWnd)->RemoveTarget(X, Y, Z);
    static_cast<NCMinimapWnd*>(nw::Field<NCWnd*>(nw::Field<NCWnd*>(GetDesktop(), 0x170), 0xE8))->RemoveTarget(X, Y, Z);
}
NW_HOOK_METHOD(0x10069790, NConsoleWnd, DeleteRadar)

/*-----------------------------------------------------------------------------
    The clan: its members, the server's answers, the offers (a dialog box; refused at once
    when the other side is not known).
-----------------------------------------------------------------------------*/

// 0x100685c0 (vslot 88) - a new member list follows (AddPledgeMember). C3: the clan page is the main window's own child
// (+0xD0), no longer one of the action window's
void NConsoleWnd::ReceivePledgeMemberList()
{
    guard(NConsoleWnd::ReceivePledgeMemberList);
    nw::Field<NCPledgeWnd*>(GetDesktop()->MainWnd, 0xF0)->ClearMembers();
    unguard;
}
NW_HOOK_METHOD(0x100685c0, NConsoleWnd, ReceivePledgeMemberList)

// 0x10072010 (vslot 89) - name, level, class, two values and whether on line
void NConsoleWnd::AddPledgeMember(L2ParamStack& Params)
{
    guard(NConsoleWnd::AddPledgeMember);
    NPledgeMember Member;
    TCHAR* Name = (TCHAR*)Params.Top();
    if (Name)
        appStrcpy(Member.Name, Name);
    Member.Level = (INT)Params.Top();
    Member.ClassID = (INT)Params.Top();
    Member.Unknown208 = (INT)Params.Top();
    Member.Unknown20C = (INT)Params.Top();
    Member.bOnline = (INT)Params.Top();
    nw::Field<NCPledgeWnd*>(GetDesktop()->MainWnd, 0xF0)->AddMember(&Member);
    unguard;
}
NW_HOOK_METHOD(0x10072010, NConsoleWnd, AddPledgeMember)

// 0x10072100 (vslot 90) - as AddPledgeMember, into UpdateMember
void NConsoleWnd::UpdatePledgeMember(L2ParamStack& Params)
{
    guard(NConsoleWnd::UpdatePledgeMember);
    NPledgeMember Member;
    TCHAR* Name = (TCHAR*)Params.Top();
    if (Name)
        appStrcpy(Member.Name, Name);
    Member.Level = (INT)Params.Top();
    Member.ClassID = (INT)Params.Top();
    Member.Unknown208 = (INT)Params.Top();
    Member.Unknown20C = (INT)Params.Top();
    Member.bOnline = (INT)Params.Top();
    nw::Field<NCPledgeWnd*>(GetDesktop()->MainWnd, 0xF0)->UpdateMember(&Member);
    unguard;
}
NW_HOOK_METHOD(0x10072100, NConsoleWnd, UpdatePledgeMember)

// 0x10068650 (vslot 91)
void NConsoleWnd::DeletePledgeMember(TCHAR* Name)
{
    guard(NConsoleWnd::DeletePledgeMember);
    nw::Field<NCPledgeWnd*>(GetDesktop()->MainWnd, 0xF0)->DeleteMember(Name);
    unguard;
}
NW_HOOK_METHOD(0x10068650, NConsoleWnd, DeletePledgeMember)

// 0x100686e0 (vslot 92) - the player has no clan any more. C3: the main window's UpdatePledgeTab is 0x100565A0
void NConsoleWnd::DeleteAllPledgeMember()
{
    guard(NConsoleWnd::DeleteAllPledgeMember);
    nw::Field<NCPledgeWnd*>(GetDesktop()->MainWnd, 0xF0)->ClearMembers();
    nw::Field<NCPledgeWnd*>(GetDesktop()->MainWnd, 0xF0)->ClearInfo();
    GetDesktop()->MainWnd->UpdatePledgeTab();
    unguard;
}
NW_HOOK_METHOD(0x100686e0, NConsoleWnd, DeleteAllPledgeMember)

// The clan's status text (sysstring) by its status, into an NPledgeInfo
static FORCEINLINE void SetPledgeStatusText(NPledgeInfo& Pledge, INT Status)
{
    switch (Status)
    {
    case 0:
        Pledge.StatusText[0] = 0;
        break;
    case 1:
        if (*GL2GameData.SysString[STR_PledgeStatus1])
            appStrcpy(Pledge.StatusText, *GL2GameData.SysString[STR_PledgeStatus1]);
        break;
    case 2:
        if (*GL2GameData.SysString[STR_PledgeStatus2])
            appStrcpy(Pledge.StatusText, *GL2GameData.SysString[STR_PledgeStatus2]);
        break;
    case 3:
        if (*GL2GameData.SysString[STR_PledgeStatus3])
            appStrcpy(Pledge.StatusText, *GL2GameData.SysString[STR_PledgeStatus3]);
        break;
    case 4:
        if (*GL2GameData.SysString[STR_PledgeStatus4])
            appStrcpy(Pledge.StatusText, *GL2GameData.SysString[STR_PledgeStatus4]);
        break;
    }
}

// the clan's record as the clan windows take it
static FORCEINLINE void MakePledgeInfo(NPledgeInfo& Pledge, PledgeInfo& Info)
{
    Pledge.PledgeID = Info.PledgeID;
    if (Info.Name)
        appStrcpy(Pledge.Name, Info.Name);
    if (Info.LeaderName)
        appStrcpy(Pledge.LeaderName, Info.LeaderName);
    Pledge.CrestID = Info.CrestID;
    Pledge.Level = Info.Level;
    Pledge.bHasCastle = Info.bHasCastle;
    Pledge.bHasHideout = Info.bHasHideout;
    Pledge.Unknown94 = Info.Unknown90;
    Pledge.Unknown98 = Info.Unknown94;
    Pledge.Status = Info.Status;
    Pledge.UnknownE8 = Info.Unknown9C;
    SetPledgeStatusText(Pledge, Info.Status);
}

// 0x10001ad0 (vslot 83) - the stock body is the empty `ret 4` the linker folded for dozens of slots
void NConsoleWnd::ReceivePledgeExtendedInfo(PledgeInfo& Info)
{
    NW_TRACE("ReceivePledgeExtendedInfo id=%d name=%ls leader=%ls", Info.PledgeID, Info.Name, Info.LeaderName);
}

// 0x10071db0 (vslot 86) - the player's clan (or its new state): the clan page takes it. C3: the page's record
// (NPledgeInfo, 0x638 bytes) is copied field by field from the engine's, the status text is the page's business
void NConsoleWnd::ReceivePledgeInfo(PledgeInfo& Info)
{
    guard(NConsoleWnd::ReceivePledgeInfo);
    NPledgeInfoC3 Pledge;
    Pledge.PledgeID = Info.PledgeID;
    if (Info.Name)
        appStrcpy(Pledge.Name, Info.Name);
    if (Info.LeaderName)
        appStrcpy(Pledge.LeaderName, Info.LeaderName);
    Pledge.CrestID = Info.CrestID;
    Pledge.Level = Info.Level;
    Pledge.bHasCastle = Info.bHasCastle;
    Pledge.bHasHideout = Info.bHasHideout;
    Pledge.Unknown94 = Info.Unknown90;
    Pledge.Unknown98 = Info.Unknown94;
    Pledge.Status = Info.Status;
    Pledge.UnknownE8 = Info.Unknown9C;
    Pledge.Unknown62C = nw::Field<INT>(&Info, 0xD0);
    Pledge.Unknown630 = nw::Field<INT>(&Info, 0xD4);
    Pledge.Unknown634 = nw::Field<INT>(&Info, 0xD8);
    nw::Field<NCPledgeWnd*>(GetDesktop()->MainWnd, 0xF0)->SetPledgeInfo(&Pledge);
    GetDesktop()->MainWnd->UpdatePledgeTab();
    unguard;
}
NW_HOOK_METHOD(0x10071db0, NConsoleWnd, ReceivePledgeInfo)

// 0x10071f00 (vslot 87) - the clan's changing values only (no names, no id)
void NConsoleWnd::ReceivePledgeInfoUpdate(PledgeInfo& Info)
{
    guard(NConsoleWnd::ReceivePledgeInfoUpdate);
    NPledgeInfoC3 Pledge;
    Pledge.Level = Info.Level;
    Pledge.CrestID = Info.CrestID;
    Pledge.bHasCastle = Info.bHasCastle;
    Pledge.bHasHideout = Info.bHasHideout;
    Pledge.Unknown94 = Info.Unknown90;
    Pledge.Unknown98 = Info.Unknown94;
    Pledge.UnknownE8 = Info.Unknown9C;
    Pledge.Status = Info.Status;
    Pledge.Unknown62C = nw::Field<INT>(&Info, 0xD0);
    Pledge.Unknown630 = nw::Field<INT>(&Info, 0xD4);
    Pledge.Unknown634 = nw::Field<INT>(&Info, 0xD8);
    nw::Field<NCPledgeWnd*>(GetDesktop()->MainWnd, 0xF0)->UpdatePledgeInfo(&Pledge);
    GetDesktop()->MainWnd->UpdatePledgeTab();
    unguard;
}
NW_HOOK_METHOD(0x10071f00, NConsoleWnd, ReceivePledgeInfoUpdate)

// 0x100811c0 (vslot 94)
void NConsoleWnd::ReceiveCreatePledge(L2ParamStack& Params)
{
    if (Params.Top())
        ShowSystemMessage(MSG_ClanCreated, 0xFFB09B79);
    else
        ShowSystemMessage(MSG_ClanCreateFailed, 0xFFB09B79);
}
NW_HOOK_METHOD(0x100811c0, NConsoleWnd, ReceiveCreatePledge)

// 0x1007a360 - an invitation: the inviter's object id and the clan's name
void NConsoleWnd::ReceiveAskJoinPledge(L2ParamStack& Params)
{
    INT InviterID = (INT)Params.Top();
    TCHAR* PledgeName = (TCHAR*)Params.Top();
    User* Inviter = FindUserByID(InviterID);
    if (Inviter)
    {
        NDialogInfo Info;
        Info.ID = DIALOG_JOIN_PLEDGE;
        Info.bMainBox = 0;
        Info.bInput = 0;
        Info.SetSystemMessage(MSG_AskJoinPledge);
        Info.SetButtons(Inviter->GetName(), PledgeName);
        Info.bProgress = 1;
        ShowDialog(&Info);
    }
    else
    {
        L2ParamStack Answer(10);
        Answer.PushBack(NULL);
        // 0x1005D4C0 jumps to NetHandler +0x1C4 (Engine AnswerJoinPledge); the C2 name RequestAnswerJoinPledge is
        // the wrapper of the NEXT slot in this client (0x1005D4D0 -> +0x1C8)
        RequestAnswerJoinPledge(Answer);
    }
}
NW_HOOK_METHOD(0x1007a360, NConsoleWnd, ReceiveAskJoinPledge)

// 0x10081200 (vslot 96)
void NConsoleWnd::ReceiveOustPledgeMember(L2ParamStack& Params)
{
    if (Params.Top())
        ShowSystemMessage(MSG_ClanExpelDone, 0xFFB09B79);
    else
        ShowSystemMessage(MSG_ClanExpelFailed, 0xFFB09B79);
}
NW_HOOK_METHOD(0x10081200, NConsoleWnd, ReceiveOustPledgeMember)

// 0x10081240 (vslot 97)
void NConsoleWnd::ReceiveDismissPledge(L2ParamStack& Params)
{
    if (Params.Top())
        ShowSystemMessage(MSG_ClanDispersed, 0xFFB09B79);
    else
        ShowSystemMessage(MSG_ClanDisperseFailed, 0xFFB09B79);
}
NW_HOOK_METHOD(0x10081240, NConsoleWnd, ReceiveDismissPledge)

// 0x10081190 (vslot 98) - said only when it failed
void NConsoleWnd::ReceiveJoinPledge(L2ParamStack& Params)
{
    if (!Params.Top())
        ShowSystemMessage(MSG_ClanJoinFailed, 0xFFB09B79);
}
NW_HOOK_METHOD(0x10081190, NConsoleWnd, ReceiveJoinPledge)

// 0x10081280
void NConsoleWnd::ReceiveWithdrawalPledge(L2ParamStack& Params)
{
    if (Params.Top())
        ShowSystemMessage(MSG_ClanWithdrawn, 0xFFB09B79);
    else
        ShowSystemMessage(MSG_ClanWithdrawFailed, 0xFFB09B79);
}
NW_HOOK_METHOD(0x10081280, NConsoleWnd, ReceiveWithdrawalPledge)

// 0x100812c0 (vslot 100) - the player was expelled
void NConsoleWnd::ReceiveOustPledgeMessage(L2ParamStack& Params)
{
    ShowSystemMessage(MSG_ClanMembershipEnded, 0xFFB09B79);
}
NW_HOOK_METHOD(0x100812c0, NConsoleWnd, ReceiveOustPledgeMessage)

// 0x100812e0 (vslot 101) - the player's clan was dispersed (in red)
void NConsoleWnd::ReceiveDismissPledgeMessage(L2ParamStack& Params)
{
    if (Params.Top())
        ShowSystemMessage(MSG_ClanDispersed, 0xFFFF0000);
    else
        ShowSystemMessage(MSG_ClanDisperseFailed, 0xFFB09B79);
}
NW_HOOK_METHOD(0x100812e0, NConsoleWnd, ReceiveDismissPledgeMessage)

// 0x1007a470 - a clan declares war: its name and the name of who asks, sent back with the
// answer
void NConsoleWnd::ReceiveStartPledgeWar(L2ParamStack& Params)
{
    TCHAR* PledgeName = (TCHAR*)Params.Top();
    TCHAR* Name = (TCHAR*)Params.Top();
    if (PledgeName && Name)
    {
        NDialogInfo Info;
        Info.ID = DIALOG_START_PLEDGE_WAR;
        Info.bMainBox = 0;
        Info.bInput = 0;
        Info.bProgress = 1;
        Info.SetSystemMessage(MSG_AskPledgeWar);
        Info.SetButtons(PledgeName, Name);
        Info.SetParamText(Name);
        ShowDialog(&Info);
    }
    else
    {
        L2ParamStack Answer(10);
        Answer.PushBack(Name);
        Answer.PushBack(NULL);
        // 0x1005D500 -> NetHandler +0x1DC (Engine ReplyStartPledgeWar); the wrapper named RequestReplyStartPledgeWar is +0x1E0
        RequestReplyStartPledgeWar(Answer);
    }
}
NW_HOOK_METHOD(0x1007a470, NConsoleWnd, ReceiveStartPledgeWar)

// 0x10081320 (vslot 103) - the other clan's answer: 1 accepted, 0 refused
void NConsoleWnd::ReceiveReplyStartPledgeWar(L2ParamStack& Params)
{
    INT Result = (INT)Params.Top();
    if (Result > 0)
        ShowSystemMessage(MSG_PledgeWarAccepted, 0xFFB09B79);
    else if (Result == 0)
        ShowSystemMessage(MSG_PledgeWarRefused, 0xFFB09B79);
}
NW_HOOK_METHOD(0x10081320, NConsoleWnd, ReceiveReplyStartPledgeWar)

// 0x1007a590 - a clan asks to end the war: as ReceiveStartPledgeWar
void NConsoleWnd::ReceiveStopPledgeWar(L2ParamStack& Params)
{
    TCHAR* PledgeName = (TCHAR*)Params.Top();
    TCHAR* Name = (TCHAR*)Params.Top();
    if (PledgeName && Name)
    {
        NDialogInfo Info;
        Info.ID = DIALOG_STOP_PLEDGE_WAR;
        Info.bMainBox = 0;
        Info.bInput = 0;
        Info.bProgress = 1;
        Info.SetSystemMessage(MSG_AskPledgeTruce);
        Info.SetButtons(PledgeName, NULL);
        Info.SetParamText(Name);
        ShowDialog(&Info);
    }
    else
    {
        L2ParamStack Answer(10);
        Answer.PushBack(Name);
        Answer.PushBack(NULL);
        // 0x1005D520 -> NetHandler +0x1E4 (Engine ReplyStopPledgeWar); the wrapper named RequestReplyStopPledgeWar is +0x1E8
        RequestReplyStopPledgeWar(Answer);
    }
}
NW_HOOK_METHOD(0x1007a590, NConsoleWnd, ReceiveStopPledgeWar)

// 0x10081360 (vslot 105) - the other clan's answer: 1 accepted, 0 refused
void NConsoleWnd::ReceiveReplyStopPledgeWar(L2ParamStack& Params)
{
    INT Result = (INT)Params.Top();
    if (Result > 0)
        ShowSystemMessage(MSG_PledgeTruceAccepted, 0xFFB09B79);
    else if (Result == 0)
        ShowSystemMessage(MSG_PledgeTruceDenied, 0xFFB09B79);
}
NW_HOOK_METHOD(0x10081360, NConsoleWnd, ReceiveReplyStopPledgeWar)

// 0x100813a0 (vslot 106) - said only when it failed
void NConsoleWnd::ReceiveSurrenderPledgeWar(L2ParamStack& Params)
{
    if (!Params.Top())
        ShowSystemMessage(MSG_SurrenderFailed, 0xFFB09B79);
}
NW_HOOK_METHOD(0x100813a0, NConsoleWnd, ReceiveSurrenderPledgeWar)

// 0x100813d0 (vslot 107) - said only when it failed
void NConsoleWnd::ReceiveSurrenderPersonally(L2ParamStack& Params)
{
    if (!Params.Top())
        ShowSystemMessage(MSG_PersonalSurrenderFailed, 0xFFB09B79);
}
NW_HOOK_METHOD(0x100813d0, NConsoleWnd, ReceiveSurrenderPersonally)

/*-----------------------------------------------------------------------------
    The alliance, the friends.
-----------------------------------------------------------------------------*/

// 0x1007a6b0 - an invitation: the inviter's object id, the alliance's name, a value not used
// and its leader's name
void NConsoleWnd::ReceiveAskJoinAlly(L2ParamStack& Params)
{
    INT InviterID = (INT)Params.Top();
    TCHAR* AllyName = (TCHAR*)Params.Top();
    Params.Top();
    TCHAR* LeaderName = (TCHAR*)Params.Top();
    if (FindUserByID(InviterID))
    {
        NDialogInfo Info;
        Info.ID = DIALOG_JOIN_ALLY;
        Info.bMainBox = 0;
        Info.bInput = 0;
        Info.bProgress = 1;
        Info.SetSystemMessage(MSG_AskJoinAlly);
        Info.SetButtons(AllyName, LeaderName);
        ShowDialog(&Info);
    }
    else
    {
        L2ParamStack Answer(10);
        Answer.PushBack(NULL);
        RequestAnswerJoinAlly(Answer);
    }
}
NW_HOOK_METHOD(0x1007a6b0, NConsoleWnd, ReceiveAskJoinAlly)

// 0x10068870 - into the chat window
void NConsoleWnd::ReceiveDismissAlly(L2ParamStack& Params)
{
    if (Params.Top())
        GetDesktop()->AddSystemMessage(MSG_AllyDissolved, 0xFFB09B79);
    else
        GetDesktop()->AddSystemMessage(MSG_AllyDissolveFailed, 0xFFB09B79);
}
NW_HOOK_METHOD(0x10068870, NConsoleWnd, ReceiveDismissAlly)

// 0x10081400 (vslot 118)
void NConsoleWnd::ReceiveJoinAlly(L2ParamStack& Params)
{
    if (Params.Top())
        ShowSystemMessage(MSG_AllyAccepted, 0xFFB09B79);
    else
        ShowSystemMessage(MSG_AllyInviteFailed, 0xFFB09B79);
}
NW_HOOK_METHOD(0x10081400, NConsoleWnd, ReceiveJoinAlly)

// 0x10068830 - into the chat window
void NConsoleWnd::ReceiveOustAlly(L2ParamStack& Params)
{
    if (Params.Top())
        GetDesktop()->AddSystemMessage(MSG_AllyExpelDone, 0xFFB09B79);
    else
        GetDesktop()->AddSystemMessage(MSG_AllyExpelFailed, 0xFFB09B79);
}
NW_HOOK_METHOD(0x10068830, NConsoleWnd, ReceiveOustAlly)

// 0x100687f0 - into the chat window
void NConsoleWnd::ReceiveWithdrawAlly(L2ParamStack& Params)
{
    if (Params.Top())
        GetDesktop()->AddSystemMessage(MSG_AllyWithdrawn, 0xFFB09B79);
    else
        GetDesktop()->AddSystemMessage(MSG_AllyWithdrawFailed, 0xFFB09B79);
}
NW_HOOK_METHOD(0x100687f0, NConsoleWnd, ReceiveWithdrawAlly)

// 0x1007a7d0 - an alliance challenges the player's: its name and the name of who asks, sent
// back with the answer; without them the answer is -1
void NConsoleWnd::StartAllianceWar(TCHAR* AllianceName, TCHAR* Name)
{
    if (AllianceName && Name)
    {
        NDialogInfo Info;
        Info.ID = DIALOG_START_ALLY_WAR;
        Info.bMainBox = 0;
        Info.bInput = 0;
        Info.bProgress = 1;
        Info.SetSystemMessage(MSG_AskAllyWar);
        Info.SetButtons(AllianceName, Name);
        Info.SetParamText(Name);
        ShowDialog(&Info);
    }
    else
    {
        L2ParamStack Answer(10);
        Answer.PushBack(Name);
        Answer.PushBack((void*)-1);
        RequestReplyStartAllianceWar(Answer);
    }
}
NW_HOOK_METHOD(0x1007a7d0, NConsoleWnd, StartAllianceWar)

// 0x1007a8f0 - an alliance asks for a cease-fire: as StartAllianceWar, without the progress bar
void NConsoleWnd::StopAllianceWar(TCHAR* AllianceName, TCHAR* Name)
{
    if (AllianceName && Name)
    {
        NDialogInfo Info;
        Info.ID = DIALOG_STOP_ALLY_WAR;
        Info.bMainBox = 0;
        Info.bInput = 0;
        Info.SetSystemMessage(MSG_AskAllyTruce);
        Info.SetButtons(AllianceName, Name);
        Info.SetParamText(Name);
        ShowDialog(&Info);
    }
    else
    {
        L2ParamStack Answer(10);
        Answer.PushBack(Name);
        Answer.PushBack((void*)-1);
        RequestReplyStopAllianceWar(Answer);
    }
}
NW_HOOK_METHOD(0x1007a8f0, NConsoleWnd, StopAllianceWar)

// 0x10081440 (vslot 121)
void NConsoleWnd::ReceiveFriendAddRequestResult(L2ParamStack& Params)
{
    if (Params.Top())
        ShowSystemMessage(MSG_FriendInvited, 0xFFB09B79);
    else
        ShowSystemMessage(MSG_FriendInviteFailed, 0xFFB09B79);
}
NW_HOOK_METHOD(0x10081440, NConsoleWnd, ReceiveFriendAddRequestResult)

// 0x1007ad60 - who asks
void NConsoleWnd::ReceiveFriendAddRequest(L2ParamStack& Params)
{
    TCHAR* Name = (TCHAR*)Params.Top();
    NDialogInfo Info;
    Info.ID = DIALOG_FRIEND_INVITE;
    Info.bMainBox = 0;
    Info.bInput = 0;
    Info.bProgress = 1;
    Info.SetSystemMessage(MSG_AskFriend);
    Info.SetButtons(Name, NULL);
    ShowDialog(&Info);
}
NW_HOOK_METHOD(0x1007ad60, NConsoleWnd, ReceiveFriendAddRequest)

/*-----------------------------------------------------------------------------
    The party, its matching, the chat channels.
-----------------------------------------------------------------------------*/

// 0x1007ae20 (vslot 108) - an invitation: C3 pops the inviter's NAME (the engine resolved the user) and how the items
// are shared (0 who picks them up, 1 at random, 2 at random with the spoil, 3 by turn, 4 by turn with the spoil; for
// another value the stock leaves the text uninitialized, empty here); refused at once without a name
void NConsoleWnd::ReceiveAskJoinParty(L2ParamStack& Params)
{
    TCHAR Text[128];
    Text[0] = 0;
    TCHAR* InviterName = (TCHAR*)Params.Top();
    INT Distribution = (INT)Params.Top();
    switch (Distribution)
    {
    case 0:
        appSprintf(Text, TEXT("%s"), *GL2GameData.SystemMsg[MSG_AskJoinPartyLooter].Message);
        break;
    case 1:
        appSprintf(Text, TEXT("%s"), *GL2GameData.SystemMsg[MSG_AskJoinPartyRandom].Message);
        break;
    case 2:
        appSprintf(Text, TEXT("%s"), *GL2GameData.SystemMsg[MSG_AskJoinPartyRandomSpoil].Message);
        break;
    case 3:
        appSprintf(Text, TEXT("%s"), *GL2GameData.SystemMsg[MSG_AskJoinPartyByTurn].Message);
        break;
    case 4:
        appSprintf(Text, TEXT("%s"), *GL2GameData.SystemMsg[MSG_AskJoinPartyByTurnSpoil].Message);
        break;
    }
    if (InviterName)
    {
        NDialogInfo Info;
        Info.ID = DIALOG_JOIN_PARTY;
        Info.bMainBox = 0;
        Info.bInput = 0;
        Info.SetText(Text);
        Info.SetButtons(InviterName, NULL);
        Info.bProgress = 1;
        ShowDialog(&Info);
    }
    else
    {
        L2ParamStack Answer(10);
        Answer.PushBack(NULL);
        RequestAnswerJoinParty(Answer);
    }
}
NW_HOOK_METHOD(0x1007ae20, NConsoleWnd, ReceiveAskJoinParty)

// 0x10081480 (vslot 109) - said only when refused
void NConsoleWnd::ReceiveJoinParty(L2ParamStack& Params)
{
    if (!Params.Top())
        ShowSystemMessage(MSG_PartyDeclined, 0xFFB09B79);
}
NW_HOOK_METHOD(0x10081480, NConsoleWnd, ReceiveJoinParty)

// 0x10068ac0 - into the chat window
void NConsoleWnd::ReceiveWithdrawalParty(L2ParamStack& Params)
{
    if (Params.Top())
        GetDesktop()->AddSystemMessage(MSG_PartyWithdrawn, 0xFFB09B79);
    else
        GetDesktop()->AddSystemMessage(MSG_PartyWithdrawFailed, 0xFFB09B79);
}
NW_HOOK_METHOD(0x10068ac0, NConsoleWnd, ReceiveWithdrawalParty)

// 0x100814b0 (vslot 111) - said only when it failed
void NConsoleWnd::ReceiveOustPartyMember(L2ParamStack& Params)
{
    if (!Params.Top())
        ShowSystemMessage(MSG_PartyExpelFailed, 0xFFB09B79);
}
NW_HOOK_METHOD(0x100814b0, NConsoleWnd, ReceiveOustPartyMember)

// 0x100814e0 (vslot 112) - the player was expelled
void NConsoleWnd::ReceiveOustPartyMessage(L2ParamStack& Params)
{
    ShowSystemMessage(MSG_PartyExpelled, 0xFFB09B79);
}
NW_HOOK_METHOD(0x100814e0, NConsoleWnd, ReceiveOustPartyMessage)

// 0x10068b00 - into the chat window
void NConsoleWnd::ReceiveDismissParty(L2ParamStack& Params)
{
    if (Params.Top())
        GetDesktop()->AddSystemMessage(MSG_PartyDispersed, 0xFFB09B79);
    else
        GetDesktop()->AddSystemMessage(MSG_PartyDisperseFailed, 0xFFB09B79);
}
NW_HOOK_METHOD(0x10068b00, NConsoleWnd, ReceiveDismissParty)

// 0x10081500 (vslot 114) - the player's party was dispersed
void NConsoleWnd::ReceiveDismissPartyMessage(L2ParamStack& Params)
{
    ShowSystemMessage(MSG_PartyDispersed, 0xFFB09B79);
}
NW_HOOK_METHOD(0x10081500, NConsoleWnd, ReceiveDismissPartyMessage)

// 0x10068c60 (vslot 179) - a member joined. C4: the party window takes the packet itself (0x10115630 pops its values from
// Params; C3 popped twelve here and passed nine), and the party's chat tab comes with it (a no-op: the folded 0x100023c0)
void NConsoleWnd::PartyAddMember(L2ParamStack& Params)
{
    GetDesktop()->PartyWnd->AddMember(Params);
    GetDesktop()->ChatWnd->EnablePartyTab(true);
}
NW_HOOK_METHOD(0x10068c60, NConsoleWnd, PartyAddMember)

// 0x10068c90 (vslot 180) - as PartyAddMember for a member's update: the first value is the member's id, the window
// (0x101107c0) pops the rest (C3 popped eight here)
void NConsoleWnd::PartyUpdateMember(L2ParamStack& Params)
{
    INT MemberID = (INT)Params.Top();
    ((NCPartyRowListExt*)GetDesktop()->PartyWnd)->UpdateRow(MemberID, Params);
}
NW_HOOK_METHOD(0x10068c90, NConsoleWnd, PartyUpdateMember)

// 0x10068cc0 - the member's id and name
void NConsoleWnd::PartyDeleteMember(L2ParamStack& Params)
{
    INT MemberID = (INT)Params.Top();
    const TCHAR* Name = (const TCHAR*)Params.Top();
    GetDesktop()->PartyWnd->DeleteMember(MemberID, Name);
}
NW_HOOK_METHOD(0x10068cc0, NConsoleWnd, PartyDeleteMember)

// 0x10068d00 - the party is gone, and its chat tab with it
void NConsoleWnd::PartyDeleteAll(L2ParamStack& Params)
{
    GetDesktop()->PartyWnd->DeleteAll();
    GetDesktop()->ChatWnd->EnablePartyTab(false);
}
NW_HOOK_METHOD(0x10068d00, NConsoleWnd, PartyDeleteAll)

// The three C1 party matching packets (the list, a member, the detail): no such handlers in the C3 image (the
// matching windows were redone: NCMatchWnd and the party room windows have their own); inert bodies
void NConsoleWnd::ReceivePartyMatchList()
{
}

void NConsoleWnd::AddPartyMatchMember(L2ParamStack& Params)
{
}

void NConsoleWnd::ReceivePartyMatchDetail(L2ParamStack& Params)
{
}

// 0x10069b80
void NConsoleWnd::EnablePartyChat(bool bEnable)
{
    GetDesktop()->ChatWnd->EnablePartyTab(bEnable);
}
NW_HOOK_METHOD(0x10069b80, NConsoleWnd, EnablePartyChat)

// 0x10069ba0
void NConsoleWnd::EnablePledgeChat(bool bEnable)
{
    GetDesktop()->ChatWnd->EnablePledgeTab(bEnable);
}
NW_HOOK_METHOD(0x10069ba0, NConsoleWnd, EnablePledgeChat)

// 0x10069bc0
void NConsoleWnd::EnableAllyChat(bool bEnable)
{
    GetDesktop()->ChatWnd->EnableAllyTab(bEnable);
}
NW_HOOK_METHOD(0x10069bc0, NConsoleWnd, EnableAllyChat)

// 0x1007d200 (vslot 175) - a chat line: who says it (object id), its channel, the name and the text (run through the
// word filter first); coloured by the channel. 0 without a text. The channels 11 (the shout's colour, as a shout) and 14 (its own
// type, light grey) are C3's; C4 adds 15 (light yellow), 16, 17 and 18 (cyan, without a name), and the default colour is light grey
UBOOL NConsoleWnd::Say2(L2ParamStack& Params)
{
    INT ObjectID = (INT)Params.Top();
    INT Type = (INT)Params.Top();
    TCHAR* Name = (TCHAR*)Params.Top();
    TCHAR* Text = (TCHAR*)Params.Top();
    HasBadWord(Text);
    if (!Text)
        return 0;
    if (Type == SAY2_Shout)
        GetDesktop()->Say(ObjectID, Name, Text, 0xFFFF7200, 1);
    else if (Type == SAY2_Party)
        GetDesktop()->Say(ObjectID, Name, Text, 0xFF00FF00, Type);
    else if (Type == SAY2_Pledge)
        GetDesktop()->Say(ObjectID, Name, Text, 0xFF7D77FF, Type);
    else if (Type == SAY2_Tell)
    {
        GetDesktop()->Say(ObjectID, Name, Text, 0xFFFF00FF, Type);
        GetDesktop()->ChatWnd->AddTellName(Name);
    }
    else if (Type == SAY2_Market)
        GetDesktop()->Say(ObjectID, Name, Text, 0xFFEAA5F5, Type);
    else if (Type == SAY2_Ally)
        GetDesktop()->Say(ObjectID, Name, Text, 0xFF77FF99, Type);
    else if (Type == SAY2_PetitionPlayer)
        GetDesktop()->Say(ObjectID, Name, Text, 0xFF80FFFF, Type);
    else if (Type == SAY2_PetitionGM)
        GetDesktop()->Say(ObjectID, Name, Text, 0xFF80FFFF, Type);
    else if (Type == SAY2_Announcement)
        GetDesktop()->Say(ObjectID, *GL2GameData.SysString[STR_Announcement], Text, 0xFF80FFFF, Type);
    else if (Type == 11)
        GetDesktop()->Say(ObjectID, Name, Text, 0xFFFF7200, 1);
    else if (Type == 14)
        GetDesktop()->Say(ObjectID, Name, Text, 0xFFDCDCDC, Type);
    else if (Type == 15)
        GetDesktop()->Say(ObjectID, Name, Text, 0xFFFFE87B, Type);
    else if (Type == 16)
        GetDesktop()->Say(ObjectID, Name, Text, 0xFF7B7DF2, Type);
    else if (Type == 18)
        GetDesktop()->Say(ObjectID, NULL, Text, 0xFF00FFFF, Type);
    else if (Type == 17)
        GetDesktop()->Say(ObjectID, Name, Text, 0xFF408CFF, Type);
    else
        GetDesktop()->Say(ObjectID, Name, Text, 0xFFDCDCDC, 0);
    return 1;
}
NW_HOOK_METHOD(0x1007d200, NConsoleWnd, Say2)

// 0x10064cd0 - an NPC speaks: its object id, the channel, its npc id (+ 1000000) and the text;
// as Say2, with the NPC's name (announcements go to the petition channel's type). C4: three more channels (15, 16 and
// 18: drawn as the petition GM channel, type 7, in their own colours)
UBOOL NConsoleWnd::NpcSay(L2ParamStack& Params)
{
    guard(NConsoleWnd::NpcSay);
    INT ObjectID = (INT)Params.Top();
    INT Type = (INT)Params.Top();
    INT NpcID = (INT)Params.Top();
    TCHAR* Text = (TCHAR*)Params.Top();
    if (!Text || !*Text)
        return 0;
    TCHAR* Name = GL2GameData.GetNpcName(NpcID - 1000000);
    if (Type == SAY2_Shout)
        GetDesktop()->Say(ObjectID, Name, Text, 0xFFFF7200, Type);
    else if (Type == SAY2_Party)
        GetDesktop()->Say(ObjectID, Name, Text, 0xFF00FF00, Type);
    else if (Type == SAY2_Pledge)
        GetDesktop()->Say(ObjectID, Name, Text, 0xFF7D77FF, Type);
    else if (Type == SAY2_Tell)
    {
        GetDesktop()->Say(ObjectID, Name, Text, 0xFFFF00FF, Type);
        GetDesktop()->ChatWnd->AddTellName(Name);
    }
    else if (Type == SAY2_Market)
        GetDesktop()->Say(ObjectID, Name, Text, 0xFFEAA5F5, Type);
    else if (Type == SAY2_Ally)
        GetDesktop()->Say(ObjectID, Name, Text, 0xFF77FF99, Type);
    else if (Type == SAY2_PetitionPlayer)
        GetDesktop()->Say(ObjectID, Name, Text, 0xFF80FFFF, Type);
    else if (Type == SAY2_PetitionGM)
        GetDesktop()->Say(ObjectID, Name, Text, 0xFF80FFFF, Type);
    else if (Type == SAY2_Announcement)
        GetDesktop()->Say(ObjectID, *GL2GameData.SysString[STR_Announcement], Text, 0xFF80FFFF,
                          SAY2_PetitionGM);
    else if (Type == 15)
        GetDesktop()->Say(ObjectID, Name, Text, 0xFFFFE87B, SAY2_PetitionGM);
    else if (Type == 16)
        GetDesktop()->Say(ObjectID, Name, Text, 0xFF7B7DF2, SAY2_PetitionGM);
    else if (Type == 18)
        GetDesktop()->Say(ObjectID, Name, Text, 0xFF7B7DF2, SAY2_PetitionGM);
    else
        GetDesktop()->Say(ObjectID, Name, Text, 0xFFDCDCDC, 0);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10064cd0, NConsoleWnd, NpcSay)

/*-----------------------------------------------------------------------------
    The shortcuts, the buffs, the skills, the trainer, the restart menu, the quests.
-----------------------------------------------------------------------------*/

// 0x10068290 - a new list follows (AddShortCutItem)
void NConsoleWnd::ReceiveShortCutList()
{
    GetDesktop()->ShortCutWnd->ClearShortCuts();
}
NW_HOOK_METHOD(0x10068290, NConsoleWnd, ReceiveShortCutList)

// 0x100805e0 (vslot 159) - a shortcut: its kind (1 item, 2 skill, 3 action, 4 macro, 5 recipe (new in C4)), its slot, then an item's object id
// and owner (1 the inventory (the desktop's +0x120), 2 the pet's (+0x188 -> +0x1CC)), a skill's id and level, an action's, a macro's or
// a recipe's id (the recipe by its index: the name of its item and the icon of its product), and whose shortcut it is. An item not found
// is not set; for another kind the stock passes an uninitialized owner (0 here). The item's crystal grade, warning flag and message
// go along (its fields are those of the item entry of this client: +0x1B14 type, +0x1B20 id, +0x1B28 count, +0x1B44 enchant, +0x1B4C
// stackable, +0x1B64 crystal grade, +0x1BA4 warning, +0x1BB8 message), and an item's own type replaces the shortcut's
void NConsoleWnd::AddShortCutItem(L2ParamStack& Params)
{
    if (!nw::Field<NCShortCutWnd*>(Desktop, 0x15C))
        return;
    INT Owner = 0;
    NItemInfo InfoRaw;
    NConsoleItemInfoC4& Info = reinterpret_cast<NConsoleItemInfoC4&>(InfoRaw);
    Info.Unknown1710 = (INT)Params.Top();
    Info.UnknownC90 = (INT)Params.Top();
    switch (Info.Unknown1710)
    {
    case 1:
    {
        Info.ObjectID = (INT)Params.Top();
        Owner = (INT)Params.Top();
        BYTE* Item;
        if (Owner == 1)
            Item = (BYTE*)GetDesktop()->InventoryWnd->FindItem(Info.ObjectID);          // NCInventoryWnd::FindItem
        else if (Owner == 2)
            Item = (BYTE*)((NCPetInventoryWnd*)nw::Field<void*>(nw::Field<void*>(Desktop, 0x188), 0x1CC))->GetItem(Info.ObjectID);   // NCPetInventoryWnd::GetItem
        else
            return;
        if (!Item)
            return;
        Info.ID = nw::Field<INT>(Item, 0x1B20);
        if ((TCHAR*)(Item + 0x14) && ((TCHAR*)(Item + 0x14))[0])
            appStrcpy(Info.Name, (TCHAR*)(Item + 0x14));
        if (((TCHAR*)(Item + 0x214))[0])
            appStrcpy(Info.AdditionalName, (TCHAR*)(Item + 0x214));
        if (((TCHAR*)(Item + 0x414))[0])
            appStrcpy(Info.IconName, (TCHAR*)(Item + 0x414));
        Info.Enchant = nw::Field<INT>(Item, 0x1B44);
        Info.bStackable = nw::Field<INT>(Item, 0x1B4C);
        Info.Count = nw::Field<INT>(Item, 0x1B28);
        Info.UnknownC34[7] = nw::Field<INT>(Item, 0x1B64);
        Info.Unknown17A0 = nw::Field<INT>(Item, 0x1BA4);
        Info.Unknown1788 = nw::Field<INT>(Item, 0x1BB8);
        Info.Param = nw::Field<INT>(Item, 0x1B14);
        break;
    }
    case 2:
        Info.ID = (INT)Params.Top();
        Info.UnknownC20 = (INT)Params.Top();
        Owner = (INT)Params.Top();
        if (GetSkillIconName(Info.ID, Info.UnknownC20))
            appStrcpy(Info.IconName, GetSkillIconName(Info.ID, Info.UnknownC20));
        if (GetSkillName(Info.ID, Info.UnknownC20))
            appStrcpy(Info.Name, GetSkillName(Info.ID, Info.UnknownC20));
        Info.UnknownC34[12] = GetSkillMpCost(Info.ID, Info.UnknownC20);
        break;
    case 3:
        Info.ID = (INT)Params.Top();
        Owner = (INT)Params.Top();
        if (GetActionIconName(Info.ID))
            appStrcpy(Info.IconName, GetActionIconName(Info.ID));
        if (GetActionName(Info.ID))
            appStrcpy(Info.Name, GetActionName(Info.ID));
        break;
    case 4:
        Info.ID = (INT)Params.Top();
        Owner = (INT)Params.Top();
        if (GetMacroIconType(Info.ID) >= 0)                                     // the macro's icon index
            appSprintf(Info.IconName, TEXT("%s%d"), TEXT("L2UI.MacroWnd.Macro_Icon"),
                       GetMacroIconType(Info.ID) + 1);
        if (GetMacroName(Info.ID))                                              // the macro's name
            appStrcpy(Info.Name, GetMacroName(Info.ID));
        break;
    case 5:
    {
        Info.ObjectID = (INT)Params.Top();
        Owner = (INT)Params.Top();
        BYTE* Recipe = (BYTE*)GL2GameData.GetRecipeDataByIndex(Info.ObjectID);
        if (Recipe)
        {
            appStrcpy(Info.Name, GetItemName(*(INT*)(Recipe + 0x20)));
            appStrcpy(Info.IconName, GetIconName(*(INT*)(Recipe + 0x28)));
        }
        break;
    }
    }
    nw::Field<NCShortCutWnd*>(Desktop, 0x15C)->SetShortCut(&InfoRaw, Owner);
}
NW_HOOK_METHOD(0x100805e0, NConsoleWnd, AddShortCutItem)

// 0x100682d0 (vslot 160) - the shortcuts of an inventory item (C2: a slot of its own, C1 had only the pet's)
void NConsoleWnd::UpdateShortCutItem(INT ObjectID)
{
    if (GetDesktop()->ShortCutWnd)
        GetDesktop()->ShortCutWnd->UpdateItem(ObjectID);
}
NW_HOOK_METHOD(0x100682d0, NConsoleWnd, UpdateShortCutItem)

// 0x100682f0 (vslot 161) - the shortcuts of a pet inventory item
void NConsoleWnd::UpdatePetShortCutItem(INT ObjectID)
{
    if (GetDesktop()->ShortCutWnd)
        GetDesktop()->ShortCutWnd->UpdatePetItem(ObjectID);
}
NW_HOOK_METHOD(0x100682f0, NConsoleWnd, UpdatePetShortCutItem)

// 0x10068310 (vslot 162)
void NConsoleWnd::ReceiveShortCutDelete(INT Slot)
{
    if (GetDesktop()->ShortCutWnd)
        GetDesktop()->ShortCutWnd->DeleteShortCut(Slot);
}
NW_HOOK_METHOD(0x10068310, NConsoleWnd, ReceiveShortCutDelete)

// 0x10068070 (vslot 164) - the skill list the server sent: NetHandler +0x274 (RequestMagicSkillList)
void NConsoleWnd::ReceiveMagicSkillList(L2ParamStack& Params)
{
    nw::VirtualCall<void>(Engine->NetHandler, 0x274 / 4, &Params);
}
NW_HOOK_METHOD(0x10068070, NConsoleWnd, ReceiveMagicSkillList)

// 0x10068330 (vslot 165) - a new list of the buffs follows (AddAbnormalStatus)
void NConsoleWnd::ReceiveAbnormalStatus()
{
    GetDesktop()->AbnormalStatusWnd->ClearStatus();
}
NW_HOOK_METHOD(0x10068330, NConsoleWnd, ReceiveAbnormalStatus)

// 0x10071420 - a buff: the skill's id, its level and the seconds left (C2: bigger texts in the record, and the
// window's AddStatus is 0x100d8650)
void NConsoleWnd::AddAbnormalStatus(L2ParamStack& Params)
{
    if (UIMode == 2)
        return;
    FAbnormalStatus Status;
    Status.SkillID = (INT)Params.Top();
    Status.Level = (INT)Params.Top();
    Status.RemainTime = (INT)Params.Top();
    if (GetSkillIconName(Status.SkillID, Status.Level))
        appStrcpy(Status.IconName, GetSkillIconName(Status.SkillID, Status.Level));
    if (GetSkillName(Status.SkillID, Status.Level))
        appStrcpy(Status.Name, GetSkillName(Status.SkillID, Status.Level));
    if (GetSkillDesc(Status.SkillID, Status.Level))
        appStrcpy(Status.Desc, GetSkillDesc(Status.SkillID, Status.Level));
    GetDesktop()->AbnormalStatusWnd->AddStatus(&Status);
}
NW_HOOK_METHOD(0x10071420, NConsoleWnd, AddAbnormalStatus)

// A skill's icon, name and description for the skill windows (each getter is asked twice)
static FORCEINLINE void SetSkillTexts(NConsoleWnd* Console, NItemInfo& Info)
{
    if (Console->GetSkillIconName(Info.ID, Info.UnknownC20))
        appStrcpy(Info.IconName, Console->GetSkillIconName(Info.ID, Info.UnknownC20));
    if (Console->GetSkillName(Info.ID, Info.UnknownC20))
        appStrcpy(Info.Name, Console->GetSkillName(Info.ID, Info.UnknownC20));
    if (Console->GetSkillDesc(Info.ID, Info.UnknownC20))
        appStrcpy(Info.Description, Console->GetSkillDesc(Info.ID, Info.UnknownC20));
}

// 0x100705c0 - a skill of the player: passive or not, its level and id (level 0: none). C4: the enchanted route's name goes to the
// additional name (the skill record's +0x68 flag)
void NConsoleWnd::ReceiveSkillList(L2ParamStack& Params)
{
    NItemInfo InfoRaw;
    NConsoleItemInfoC4& Info = reinterpret_cast<NConsoleItemInfoC4&>(InfoRaw);
    Info.Param = (INT)Params.Top();
    Info.UnknownC20 = (INT)Params.Top();
    Info.ID = (INT)Params.Top();
    if (Info.UnknownC20 > 0)
    {
        if (GetSkillIconName(Info.ID, Info.UnknownC20))
            appStrcpy(Info.IconName, GetSkillIconName(Info.ID, Info.UnknownC20));
        if (GetSkillName(Info.ID, Info.UnknownC20))
            appStrcpy(Info.Name, GetSkillName(Info.ID, Info.UnknownC20));
        if (GetSkillDesc(Info.ID, Info.UnknownC20))
            appStrcpy(Info.Description, GetSkillDesc(Info.ID, Info.UnknownC20));
        FL2MagicSkillData* Skill = GL2GameData.GetMSData(Info.ID, Info.UnknownC20);
        if (Skill && Skill->bEnchanted)
            appStrcpy(Info.AdditionalName, *Skill->EnchantName);
        static_cast<NCMagicSkillWnd*>(GetDesktop()->MainWnd->SkillWnd)->SkillWnd->AddSkill(&InfoRaw);
    }
}
NW_HOOK_METHOD(0x100705c0, NConsoleWnd, ReceiveSkillList)

// 0x10074820 (vslot 245) - a skill's cool time (its id and level, the whole time and the time left, in seconds) starts on the macros, on the
// shortcut slots and on the skill window's icon: the running macro's current line is the skill (the macro window is the desktop's +0x150,
// the macros are 0xA3C bytes and more: the cool time icon at +0x80, running at +0xA34, the current line +0xA48, the id +0xA3C, the lines of
// 0x70 bytes from +0xD8), the slot holding that macro (type 4) starts with it. The shortcut bar (the desktop's +0x15C) has slots of 0x1CC0
// bytes (the type at +0x1B18, the id +0x1B20, the level +0x1B24, the cool time +0x1C40); the skill window (+0xE8 -> +0xF4 -> +0xEC) lists
// its icons at +0x1F4 (items +0x12C, cool times +0x178)
void NConsoleWnd::SkillCoolTime(INT SkillID, INT Level, INT Total, INT Remaining)
{
    FLOAT TotalTime = (FLOAT)Total;
    if (!(TotalTime > 0.f) || Total < Remaining)
        return;
    UBOOL bMacro = 0;
    INT MacroIndex = 0;
    for (INT i = 0; i < nw::Field<INT>(nw::Field<NCWnd*>(Desktop, 0x150), 0x130); i++)
    {
        BYTE* Macro = nw::Field<BYTE**>(nw::Field<NCWnd*>(Desktop, 0x150), 0x12C)[i];
        if (nw::Field<INT>(Macro, 0xA34))
        {
            INT Cur = nw::Field<INT>(Macro, 0xA48);
            bMacro = 1;
            MacroIndex = i;
            if (nw::Field<INT>(Macro, (Cur + 2) * 0x70) == SkillID && nw::Field<INT>(Macro, Cur * 0x70 + 0xE8) == Level)
                ((NCCoolTimeIcon*)(Macro + 0x80))->Start(TotalTime, Remaining);
        }
    }
    if (nw::Field<NCWnd*>(Desktop, 0x15C)->IsShowWindow())
    {
        INT Num = nw::Field<INT>(nw::Field<NCWnd*>(Desktop, 0x15C), 0x26C) * nw::Field<INT>(nw::Field<NCWnd*>(Desktop, 0x15C), 0x268);
        for (INT i = 0, Off = 0; i < Num; i++, Off += 0x1CC0)
        {
            if (nw::Field<NCWnd*>(Desktop, 0x150) && bMacro)
            {
                BYTE* Slot = nw::Field<BYTE*>(nw::Field<NCWnd*>(Desktop, 0x15C), 0x200) + Off;
                BYTE* Macro = nw::Field<BYTE**>(nw::Field<NCWnd*>(Desktop, 0x150), 0x12C)[MacroIndex];
                if (nw::Field<INT>(Slot, 0x1B18) == 4 && nw::Field<INT>(Slot, 0x1B20) == nw::Field<INT>(Macro, 0xA3C))
                    ((NCCoolTimeIcon*)(Slot + 0x1C40))->Start(TotalTime, Remaining);
            }
            BYTE* Slot = nw::Field<BYTE*>(nw::Field<NCWnd*>(Desktop, 0x15C), 0x200) + Off;
            if (nw::Field<INT>(Slot, 0x1B18) == 2 && nw::Field<INT>(Slot, 0x1B20) == SkillID && nw::Field<INT>(Slot, 0x1B24) == Level)
                ((NCCoolTimeIcon*)(Slot + 0x1C40))->Start(TotalTime, Remaining);
        }
    }
    if (static_cast<NCMagicSkillWnd*>(GetDesktop()->MainWnd->SkillWnd)->SkillWnd->IsShowWindow())
    {
        INT Count = nw::Field<NCItemWnd*>(static_cast<NCMagicSkillWnd*>(GetDesktop()->MainWnd->SkillWnd)->SkillWnd, 0x1F4)->GetItemCount();
        for (INT i = 0; i < Count; i++)
        {
            NCItemWnd* List = nw::Field<NCItemWnd*>(static_cast<NCMagicSkillWnd*>(GetDesktop()->MainWnd->SkillWnd)->SkillWnd, 0x1F4);
            BYTE* Item = nw::Field<BYTE**>(List, 0x12C)[i];
            if (nw::Field<INT>(Item, 0x1B20) == SkillID && nw::Field<INT>(Item, 0x1B24) == Level)
                nw::Field<NCCoolTimeIcon**>(List, 0x178)[i]->Start(TotalTime, Remaining);
        }
    }
}
NW_HOOK_METHOD(0x10074820, NConsoleWnd, SkillCoolTime)

// 0x10068080 (vslot 184) - the skill list follows (ReceiveSkillList)
void NConsoleWnd::ShowSkillWnd()
{
    static_cast<NCMagicSkillWnd*>(GetDesktop()->MainWnd->SkillWnd)->SkillWnd->ClearLists();
}
NW_HOOK_METHOD(0x10068080, NConsoleWnd, ShowSkillWnd)

// 0x10067f70 (vslot 198) - the trainer's list follows (AddAcquireSkillInfo): C3 shows the list, then focuses the window
void NConsoleWnd::ShowTrainListWnd()
{
    GetDesktop()->TrainWnd->Reset();
    GetDesktop()->TrainWnd->ShowWindow();
    GetDesktop()->TrainWnd->ShowList();
    GetDesktop()->TrainWnd->SetFocus();
}
NW_HOOK_METHOD(0x10067f70, NConsoleWnd, ShowTrainListWnd)

// 0x10068000 (vslot 205) - the trainer's window is hidden
void NConsoleWnd::HideTrainWnd()
{
    GetDesktop()->TrainWnd->HideWindow();
}
NW_HOOK_METHOD(0x10068000, NConsoleWnd, HideTrainWnd)

// 0x10083740 (vslot 199; ret 4) - the trainer's details of a skill: a kind flag, its id, level and SP, then (kind 0) one value at +0x4C8 or (kind
// 1) two at +0x4B8 and +0x4C4, with the skill data's texts, MP cost and range; the window takes the focus. The record (0x4D0 bytes, the flag at
// +0x4C0) is built in place and destroyed explicitly, as the stock does (NTrainSkillInfo: 0x1006E920, 0x10082320)
void NConsoleWnd::ShowTrainInfoWnd(L2ParamStack& Params)
{
    BYTE InfoBuf[sizeof(NTrainSkillInfo)];
    NTrainSkillInfo& Info = *(NTrainSkillInfo*)InfoBuf;
    ::new ((void*)InfoBuf) NTrainSkillInfo;
    UBOOL bKind = Params.Top() != 0;
    nw::Field<INT>(&Info, 0x4C0) = bKind;
    Info.SkillID = (INT)Params.Top();
    Info.Level = (INT)Params.Top();
    Info.SP = (INT)Params.Top();
    if (!bKind)
        nw::Field<INT>(&Info, 0x4C8) = (INT)Params.Top();
    else
    {
        Info.Unknown2B8 = (INT)Params.Top();
        nw::Field<INT>(&Info, 0x4C4) = (INT)Params.Top();
    }
    if (GetSkillIconName(Info.SkillID, Info.Level))
        appStrcpy(Info.IconName, GetSkillIconName(Info.SkillID, Info.Level));
    if (GetSkillName(Info.SkillID, Info.Level))
        appStrcpy(Info.Name, GetSkillName(Info.SkillID, Info.Level));
    if (GetSkillDesc(Info.SkillID, Info.Level))
        ConvertNewLines(GetSkillDesc(Info.SkillID, Info.Level), Info.Desc);
    if (GetSkillTypeText(Info.SkillID, Info.Level))
        appStrcpy(Info.Unknown20C, GetSkillTypeText(Info.SkillID, Info.Level));
    if (GConsole->GetSkillMpCost(Info.SkillID, Info.Level))
        Info.MpCost = GConsole->GetSkillMpCost(Info.SkillID, Info.Level);
    if (GConsole->GetSkillRange(Info.SkillID, Info.Level))
        Info.Range = GConsole->GetSkillRange(Info.SkillID, Info.Level);
    GetDesktop()->TrainWnd->InfoWnd->Reset();
    GetDesktop()->TrainWnd->SetSkillInfo(&Info);
    GetDesktop()->TrainWnd->ShowWindow();
    GetDesktop()->TrainWnd->ShowInfo();
    GetDesktop()->TrainWnd->SetFocus();
    Info.~NTrainSkillInfo();
}
NW_HOOK_METHOD(0x10083740, NConsoleWnd, ShowTrainInfoWnd)

// 0x100703f0 - a skill the trainer teaches. The packet: a kind flag, the skill id, the level, (a value that is dropped when the kind is 0),
// the SP it costs, one more value (the kind 1's at +0x1494, the kind 0's at +0x149C) and the last. The entry is 0x14A8 bytes; the window's
// AddEntry (0x1011E260) takes it
void NConsoleWnd::AddAcquireSkillInfo(L2ParamStack& Params)
{
    NTrainEntryC4 Entry;
    ((NTrainEntryExt*)&Entry)->Construct();                    // the entry's constructor
    UBOOL bKind = Params.Top() != 0;
    Entry.bKind = bKind;
    Entry.SkillID = (INT)Params.Top();
    Entry.Level = (INT)Params.Top();
    if (!bKind)
        Params.Top();
    Entry.Value1490 = (INT)Params.Top();
    if (!bKind)
        Entry.Value149C = (INT)Params.Top();
    else
        Entry.Value1494 = (INT)Params.Top();
    Entry.Value1498 = (INT)Params.Top();
    if (GetSkillName(Entry.SkillID, Entry.Level))
        appStrcpy(Entry.Name, GetSkillName(Entry.SkillID, Entry.Level));
    if (GetSkillDesc(Entry.SkillID, Entry.Level))
        appStrcpy(Entry.Description, GetSkillDesc(Entry.SkillID, Entry.Level));
    if (GetSkillTypeText(Entry.SkillID, Entry.Level))
        appStrcpy(Entry.TypeText, GetSkillTypeText(Entry.SkillID, Entry.Level));
    if (GetSkillIconName(Entry.SkillID, Entry.Level))
        appStrcpy(Entry.IconName, GetSkillIconName(Entry.SkillID, Entry.Level));
    GetDesktop()->TrainWnd->AddEntry((const NTrainEntry*)&Entry);
}
NW_HOOK_METHOD(0x100703f0, NConsoleWnd, AddAcquireSkillInfo)

// 0x1006fe20 (vslot 202) - the player died: the restart menu with its places. The packet's stack gets one more value (0: the
// stock pops five, the server sends four), the target is cancelled, the first dialog box reset, the second one's progress bar
// (+0x23C, NControl::NotifyParent) told 0, the buff window cleared (and its extra icon); the menu (the desktop's +0x26C) takes the
// places when there is any (it shows itself)
void NConsoleWnd::ShowRestartMenu(L2ParamStack& Params)
{
    L2ParamStack Cancel(10);
    Params.PushBack((void*)0);
    RequestTargetCancel(Cancel);
    GetDesktop()->MainDialogBox->Reset();
    static_cast<NControl*>(nw::Field<NCWnd*>(GetDesktop()->SecondDialogBox, 0x23C))->NotifyParent(0);
    GetDesktop()->AbnormalStatusWnd->ClearStatus();
    GetDesktop()->AbnormalStatusWnd->ClearExtraStatus();   // NCAbnormalStatusWnd::ClearExtraStatus
    INT bVillage = (INT)Params.Top();
    INT bHideaway = (INT)Params.Top();
    INT bCastle = (INT)Params.Top();
    INT bSiegeHQ = (INT)Params.Top();
    INT bFixed = (INT)Params.Top();
    if (bVillage || bHideaway || bCastle || bSiegeHQ || bFixed)
        static_cast<NCRestartMenuWnd*>(nw::Field<NCWnd*>(Desktop, 0x26C))->SetRestartPoints(bVillage, bHideaway, bCastle, bSiegeHQ, bFixed);   // NCRestartMenuWnd::SetRestartPoints
}
NW_HOOK_METHOD(0x1006fe20, NConsoleWnd, ShowRestartMenu)

// 0x10066090 (vslot 206) - the player was teleported: C3 hides the loading window (the C2 build's restart menu is
// the one at +0x3950's +0xCC now)
void NConsoleWnd::TeleportToLocation()
{
    LoadingWnd->HideWindow();
}
NW_HOOK_METHOD(0x10066090, NConsoleWnd, TeleportToLocation)

// 0x10069f90 - copied and dropped
void NConsoleWnd::UpdatePlayerInfo(L2ParamStack& Params)
{
    L2ParamStack Copy(10);
    Copy = Params;
}
NW_HOOK_METHOD(0x10069f90, NConsoleWnd, UpdatePlayerInfo)

// The record of a quest's steps that NCQuestGaraTreeWnd's AddQuest allocates and never uses (0x18 bytes, names ours): the quest id
// (the constructor leaves it), two numbers and the step numbers
struct NQuestStepRecord
{
    INT QuestID;               // +0x00
    INT Unknown04;             // +0x04
    INT Unknown08;             // +0x08
    TArray<INT> Steps;         // +0x0C

    NQuestStepRecord()         // 0x100DB750 (the number fields cleared, the array emptied)
    {
        Unknown04 = 0;
        Unknown08 = 0;
        Steps.Empty(sizeof(INT));
    }
};
NW_ASSERT_SIZE(NQuestStepRecord, 0x18);
NW_HOOK_CTOR(0x100ebb80, NQuestStepRecord)

// The bit of step number Index as the stock builds it: (int)pow(2.0, Index) through _ftol, which keeps the low 32 bits of the 64-bit
// integer (bit 31 is 0x80000000, Index 32 and up give 0)
static INT QuestStepBit(INT Index)
{
    return Index < 32 ? (INT)(1u << Index) : 0;
}

// The quest record the Engine hands out (FL2QuestData, declared but not defined in UE2Engine.h) as far as the quest tree reads it
// (names ours; offsets from 0x100DB7B0)
struct NQuestRecord
{
    BYTE _Unknown00[0x0C];
    FString Name;              // +0x0C the quest's name
    FString StepText;          // +0x18 the text of this step (the name is shown when it is empty)
    INT Unknown24;             // +0x24 (new in C4)
    FString Details;           // +0x28 the details of the step
    TArray<INT> ItemIDs;       // +0x34 the items the step lists
    TArray<INT> ItemNeeded;    // +0x40 how many of each
    INT Unknown4C;             // +0x4C (new in C4)
    INT MapPos[3];             // +0x50 the place on the map
    INT LevelMin;              // +0x5C
    INT LevelMax;              // +0x60
    INT IconSet;               // +0x64
    FString MapPlace;          // +0x68
    INT Unknown74;             // +0x74
};
NW_ASSERT_OFFSET(NQuestRecord, Name, 0x0C);
NW_ASSERT_OFFSET(NQuestRecord, Details, 0x28);
NW_ASSERT_OFFSET(NQuestRecord, ItemIDs, 0x34);
NW_ASSERT_OFFSET(NQuestRecord, MapPos, 0x50);
NW_ASSERT_OFFSET(NQuestRecord, MapPlace, 0x68);

// NCQuestGaraTreeCtrlExt and NCQuestGaraTreeWndExt (NConsoleWndExt.h): the two quest tree classes with the functions the db has as anonymous

// 0x100ebbe0 (name ours: NCQuestGaraTreeCtrl::AddQuestStep, ret 0x10) - one step of a quest into the tree. At step 1 the quest's own entry first
// (kind 1: the name, the levels, the plus and minus pictures, a child of the root entry), which is what the next steps hang from: Node is that
// entry (or the one the caller got from the step before). The step's entry (kind 2, a child of Node: the step's text, or the name when it has
// none, and the map place; the step number in its +8) gets the details as its child (kind 3: the details' text and the items of the step with
// their pictures, names, ids and counts). bMore is stored in both as bTextCount. The answer is Node; 0 when the Engine has no such quest step
NCGaraTVItem* NCQuestGaraTreeCtrlExt::AddQuestStep(INT QuestID, INT Step, NCGaraTVItem* Node, INT bMore)
{
    NQuestRecord* Quest = (NQuestRecord*)GL2GameData.GetQuestData(QuestID, Step);
    // the records are built in place and destroyed explicitly, as the stock does (the constructor 0x10014BB0 and the destructor 0x10014D20 are
    // NGaraTVItemData's own)
    if (Step == 1)
    {
        BYTE HeaderBuf[sizeof(NGaraTVItemData)];
        NGaraTVItemData& Header = *(NGaraTVItemData*)HeaderBuf;
        ::new ((void*)HeaderBuf) NGaraTVItemData;
        appStrcpy(Header.Text, *Quest->Name);
        Header.LevelMax = Quest->LevelMax;
        Header.IconSet = Quest->IconSet;
        Header.Kind = Step;
        Header.LevelMin = Quest->LevelMin;
        Header.IconH = 14;
        Header.IconW = 14;
        Header.Icon = FindTexture(TEXT("L2UI_CH3.QUESTWND.QuestWndPlusBtn"));
        Header.ExpandedIcon = FindTexture(TEXT("L2UI_CH3.QUESTWND.QuestWndMinusBtn"));
        Header.QuestID = QuestID;
        NCGaraTVItem* Entry = QuestTree->AddItem(&Header, RootItem);
        Entry->ParentItem = RootItem;
        Node = Entry;
        Header.~NGaraTVItemData();
    }
    if (!Quest)
        return NULL;
    BYTE StepBuf[sizeof(NGaraTVItemData)];
    NGaraTVItemData& StepData = *(NGaraTVItemData*)StepBuf;
    ::new ((void*)StepBuf) NGaraTVItemData;
    StepData.Kind = 2;
    StepData.IconH = 14;
    StepData.IconW = 14;
    StepData.Icon = FindTexture(TEXT("L2UI_CH3.QUESTWND.QuestWndDownBtn"));
    StepData.ExpandedIcon = FindTexture(TEXT("L2UI_CH3.QUESTWND.QuestWndUpBtn"));
    StepData.bTextCount = (BYTE)bMore;
    StepData.MapPos[0] = Quest->MapPos[0];
    StepData.MapPos[1] = Quest->MapPos[1];
    StepData.MapPos[2] = Quest->MapPos[2];
    StepData.MapPlace = Quest->MapPlace;
    StepData.QuestID = QuestID;
    StepData.IconSet = Step;
    if (Quest->StepText.Len() > 0)
        appStrcpy(StepData.Text, *Quest->StepText);
    else
        appStrcpy(StepData.Text, *Quest->Name);
    StepData.Unknown1DD4 = Quest->Unknown74;
    NCGaraTVItem* StepNode = QuestTree->AddItem(&StepData, Node);
    BYTE DetailBuf[sizeof(NGaraTVItemData)];
    NGaraTVItemData& Detail = *(NGaraTVItemData*)DetailBuf;
    ::new ((void*)DetailBuf) NGaraTVItemData;
    Detail.Kind = 3;
    appStrcpy(Detail.Text2, *Quest->Details);
    Detail.IconH = 32;
    Detail.IconW = 32;
    Detail.Count = Quest->ItemIDs.Num();
    Detail.bTextCount = (BYTE)bMore;
    Detail.QuestID = QuestID;
    Detail.IconSet = Step;
    for (INT j = 0; j < Quest->ItemIDs.Num(); j++)
    {
        Detail.ItemNeeded[j] = Quest->ItemNeeded(j);
        NViewItemDataC4* Item = (NViewItemDataC4*)NItemTable::Find(&Quest->ItemIDs(j));
        Detail.ItemIcon[j] = FindTexture(*Item->IconName);
        Detail.ItemName[j] = *Item->Name;
        Detail.ItemID[j] = Item->ItemID;
    }
    QuestTree->AddItem(&Detail, StepNode);
    Detail.~NGaraTVItemData();
    StepData.~NGaraTVItemData();
    return Node;
}
NW_HOOK_METHOD(0x100ebbe0, NCQuestGaraTreeCtrlExt, AddQuestStep)

// 0x100ebf90 (name ours: NCQuestGaraTreeWnd::AddQuest, ret 8; `this` is the main window's quest tab or the GM's) - the quest QuestID at
// step Step goes into the tree, one entry for each step it has reached. Step is either a number (the steps 1 .. Step) or, with bit 31
// set, a mask of steps (bit k is step k + 1; up to step 30). Each entry is added by the tree control's 0x100DB7B0 with the id, the
// step, the entry made before and whether another follows; the count shown in "(n/15)" goes up by one. The stock also allocates a
// record it never reads (kept)
void NCQuestGaraTreeWndExt::AddQuest(INT QuestID, INT Step)
{
    NQuestStepRecord* Record = (NQuestStepRecord*)appMalloc(sizeof(NQuestStepRecord), TEXT("new"));
    if (Record)
        Record = new (Record) NQuestStepRecord;
    Record->QuestID = QuestID;
    TArray<INT> Steps;
    INT Mask = Step;
    if (!(Step & (INT)0x80000000))
    {
        Mask = 0;
        for (INT i = 0; i < Step; i++)
            Mask |= QuestStepBit(i);
    }
    for (INT k = 0; k < 30; k++)
        if (Mask & QuestStepBit(k))
            Steps(Steps.Add()) = k + 1;
    void* Previous = NULL;
    for (INT i = 0; i < Steps.Num(); i++)
        Previous = ((NCQuestGaraTreeCtrlExt*)TreeCtrl)->AddQuestStep(QuestID, Steps(i), (NCGaraTVItem*)Previous, (INT)(i != Steps.Num() - 1));
    NumQuests++;
}
NW_HOOK_METHOD(0x100ebf90, NCQuestGaraTreeWndExt, AddQuest)

// 0x10067270 (vslot 171) - a new list follows (AddQuestID). The quest window is the main window's +0xD8; its list is
// emptied by 0x100DB610 (shared with the GM's quest window)
void NConsoleWnd::ReceiveQuestList()
{
    guard(NConsoleWnd::ReceiveQuestList);
    ((NCQuestGaraTreeWndExt*)GetDesktop()->MainWnd->QuestWnd)->ResetTree();
    unguard;
}
NW_HOOK_METHOD(0x10067270, NConsoleWnd, ReceiveQuestList)

// 0x10067390 (vslot 173) - a quest of the player at its step: the quest window takes the id and the step when the
// questname-*.dat has a line with that id (C4 stock: 2000 lines of 0xA0 bytes at +0x3D28 of the game data; the name is not read)
void NConsoleWnd::AddQuestID(INT QuestID, INT Step)
{
    guard(NConsoleWnd::AddQuestID);
    for (INT i = 0; i < 2000; i++)
    {
        NQuestHeadC4& Line = GQuestHeadsC4[i];
        if (Line.bValid && Line.QuestID == QuestID)
        {
            ((NCQuestGaraTreeWndExt*)GetDesktop()->MainWnd->QuestWnd)->AddQuest(QuestID, Step);
            break;
        }
    }
    unguard;
}
NW_HOOK_METHOD(0x10067390, NConsoleWnd, AddQuestID)

/*-----------------------------------------------------------------------------
    The system messages with parameters: each parameter comes as a text (AddSystemMessageParam)
    before the message's id (EndSystemMessageParam).
-----------------------------------------------------------------------------*/

// 0x10081610 (vslot 189) - a parameter by its type: 0 text, 1 number, 2 NPC (id + 1000000), 3 item,
// 4 skill and level, 5 castle
void NConsoleWnd::AddSystemMessageParam(L2ParamStack& Params)
{
    NSysMsgParam Param;
    Param.Text = NULL;
    Param.Len = 0;
    switch ((INT)Params.Top())
    {
    case 0:
        Param.Set((TCHAR*)Params.Top());
        break;
    case 1:
        Param.SetNumber((INT)Params.Top());
        break;
    case 2:
        Param.Set(GL2GameData.GetNpcName((INT)Params.Top() - 1000000));
        break;
    case 3:
        Param.Set(GetItemName((INT)Params.Top()));
        break;
    case 4:
    {
        INT SkillID = (INT)Params.Top();
        INT Level = (INT)Params.Top();
        Param.Set(GetSkillName(SkillID, Level));
        break;
    }
    case 5:
        Param.Set(GetCastleName((INT)Params.Top()));
        break;
    }
    SysMsgParams.AddItem(Param);
}
NW_HOOK_METHOD(0x10081610, NConsoleWnd, AddSystemMessageParam)

// 0x10081700 (ret 0x10; slot 191) - the message: "$sN" takes the text of parameter N; "%" and a particle (0xC774, 0xC744, 0xC740, 0xC640)
// take the form that fits the last parameter's end. P4 (the stock's last value): with the message id 0 it hides the main dialog box
// (nothing else is done, the parameters stay); else the text goes to a dialog box of the kind 0x46 (with the id as its parameter)
// instead of the chat. Shown in the chat window (the message's type and color as the chat's) or, for the ones of type 4, in a
// dialog box (0x1F); with the sound of its line and the voice (the record's third text); then the parameters are freed. The text can
// be asked for instead (P2 set: copied to the buffer P3, nothing shown); UGameEngine::OnEndSystemMessageParam passes 0, 0, 0. An id
// the table does not have is a line of the chat in an error message
void NConsoleWnd::EndSystemMessageParam(L2ParamStack& Params, INT P2, INT P3, INT P4)
{
    INT ID = (INT)Params.Top();
    TCHAR Text[1024] = TEXT("");
    INT NumParams = SysMsgParams.Num();
    INT Last = -1;
    INT Len = 0;
    if (P4 && ID == 0)
    {
        nw::Field<NCWnd*>(GetDesktop(), 0x240)->HideWindow();
        return;
    }
#if NW_CUSTOM
    if (!P2)   // custom: the damage over the target's head
        NCustomDamage::OnSystemMessage(ID, NumParams ? SysMsgParams(0).Text : NULL);
#endif
    if (!GL2GameData.SystemMsg[ID].Unknown0 || ID < 0 || ID >= 3000)
    {
        TCHAR Error[254];
        appSprintf(Error, TEXT("\xC54C \xC218 \xC5C6\xB294 System Message format : %d"), ID);   // "(unknown) System Message format : %d" in Korean
        GetDesktop()->AddSystemText(Error, (INT)0xFFB09B79, (DWORD)0xFFDCDCDC);
    }
    else
    {
        BYTE MsgMem[sizeof(FL2SystemMsgData)];
        FL2SystemMsgData& Msg = *(FL2SystemMsgData*)MsgMem;
        ((NSystemMsgFns*)MsgMem)->CopyFrom(&GL2GameData.SystemMsg[ID]);   // NSystemMsgFns::CopyFrom
        const TCHAR* Format = *Msg.Message;
        for (INT i = 0; i < Msg.Message.Len(); )
        {
            TCHAR C = Format[i];
            if (C != '$' && C != '%')
            {
                Text[Len++] = C;
                i++;
            }
            else if (C == '%' && Format[i + 1] != 0xC774 && Format[i + 1] != 0xC744 && Format[i + 1] != 0xC740 && Format[i + 1] != 0xC640)
            {
                Text[Len++] = C;   // a "%" that is no particle's start
                i++;
            }
            else if (C == '$')
            {
                Last = Format[i + 2] - '1';
                if (Last < NumParams && SysMsgParams(Last).Text)
                {
                    appStrcpy(Text + Len, SysMsgParams(Last).Text);
                    Len += SysMsgParams(Last).Len;
                }
                i += 3;
            }
            else
            {
                TCHAR Particle = Format[i + 1];
                if (Last < 0)
                {
                    TCHAR Form = Particle == 0xC774 ? (TCHAR)0xC774 : (Particle == 0xC744 ? (TCHAR)0xB97C : (Particle == 0xC740 ? (TCHAR)0xB294 : (TCHAR)0xC640));
                    Text[Len++] = Form;
                    i += 2;
                }
                else if (((NSysMsgParam*)SysMsgParams.GetData())[Last].Text)
                {
                    NSysMsgParam& Param = ((NSysMsgParam*)SysMsgParams.GetData())[Last];
                    UBOOL bNoFinal = HasNoFinalConsonant(Param.Text[Param.Len - 1]);
                    if (Particle == 0xC774)
                        Text[Len++] = bNoFinal ? 0xAC00 : 0xC774;
                    else if (Particle == 0xC744)
                        Text[Len++] = bNoFinal ? 0xB97C : 0xC744;
                    else if (Particle == 0xC740)
                        Text[Len++] = bNoFinal ? 0xB294 : 0xC740;
                    else
                        Text[Len++] = bNoFinal ? 0xC640 : 0xACFC;
                    i += 2;
                }
                else
                    i++;
            }
        }
        Text[Len] = 0;
        if (P4)
        {
            NDialogInfo Info;
            Info.ID = 0x46;
            Info.bMainBox = 1;
            Info.bInput = 0;
            Info.SetParams(ID, -1);
            Info.SetText(Text);
            GConsole->ShowDialog(&Info);
            if ((char)P2 && P3)
                appStrcpy((TCHAR*)P3, Text);
        }
        else if ((char)P2 == 0)
        {
            if (GL2GameData.SystemMsg[ID].Type == 4)
            {
                NDialogInfo Info;
                Info.ID = 0x1F;
                Info.bMainBox = 1;
                Info.bInput = 0;
                Info.Owner = NULL;
                Info.SetText(Text);
                ShowDialog(&Info);
            }
            else
            {
                GetDesktop()->AddSystemText(Text, GL2GameData.SystemMsg[ID].Type, GL2GameData.SystemMsg[ID].Unknown14);
                GConsole->PlaySoundName(*GL2GameData.SystemMsg[ID].Message2);
                GConsole->PlayVoiceName(*GL2GameData.SystemMsg[ID].Message3);
            }
        }
        else if (P3)
            appStrcpy((TCHAR*)P3, Text);
        ((NSystemMsgFns*)MsgMem)->DestroyStrings();   // NSystemMsgFns::DestroyStrings
    }
    for (INT i = 0; i < NumParams; i++)
    {
        NSysMsgParam& Param = SysMsgParams(i);
        if (Param.Text)
        {
            GMalloc->Free(Param.Text);
            Param.Text = NULL;
        }
        Param.Len = 0;
    }
    SysMsgParams.Empty();
}
NW_HOOK_METHOD(0x10081700, NConsoleWnd, EndSystemMessageParam)

/*-----------------------------------------------------------------------------
    The pet and the summon.
-----------------------------------------------------------------------------*/

// 0x1007afd0 - the window of the summon (1) or of the pet
void NConsoleWnd::ShowPetStatusWnd(INT Type)
{
    if (Type == 1)
    {
        GetDesktop()->SummonedWnd->ShowWindow();
        PlayInterfaceSound(SOUND_WindowOpen);
        GetDesktop()->SummonedWnd->SetFocus();
    }
    else
    {
        GetDesktop()->PetWnd->ShowWindow();
        PlayInterfaceSound(SOUND_WindowOpen);
        GetDesktop()->PetWnd->SetFocus();
    }
}
NW_HOOK_METHOD(0x1007afd0, NConsoleWnd, ShowPetStatusWnd)

// 0x10069030 (vslot 237) - its bars beside the party's: the summon's (1) or the pet's; C3: the window's 0x10101020
// (summon) / 0x10100CA0 (pet) runs between the show and the focus
void NConsoleWnd::ShowPetPartyWnd(INT Type)
{
    if (Type == 1)
    {
        if (!GetDesktop()->SummonedStatusWnd->IsShowWindow())
        {
            GetDesktop()->SummonedStatusWnd->ShowWindow();
            GetDesktop()->SummonedStatusWnd->ClearBuffs();
            GetDesktop()->SummonedStatusWnd->SetFocus();
        }
    }
    else
    {
        if (!GetDesktop()->PetStatusWnd->IsShowWindow())
        {
            GetDesktop()->PetStatusWnd->ShowWindow();
            ((NCPartyStatusWnd*)GetDesktop()->PetStatusWnd)->ClearBuffs();   // 0x101155E0: the party member window's, shared with the pet's
            GetDesktop()->PetStatusWnd->SetFocus();
        }
    }
}
NW_HOOK_METHOD(0x10069030, NConsoleWnd, ShowPetPartyWnd)

// 0x1007b040 - (no handler) all the pet's and the summon's windows hidden, the summon's
// timer stopped
void NConsoleWnd::HideSummonWnd()
{
    GetDesktop()->PetWnd->HideWindow();
    GetDesktop()->SummonedWnd->HideWindow();
    GetDesktop()->PetStatusWnd->HideWindow();
    GetDesktop()->SummonedStatusWnd->HideWindow();
    NCSummonedStatusWnd* Summoned = GetDesktop()->SummonedStatusWnd;
    Summoned->TimeBar->Stop();
    Summoned->TimeBar->Reset();
    nw::Field<UBOOL>(Summoned, 0x44C) = 0;   // C4: bTimerStarted is at +0x44C (the header says +0x450)
    PlayInterfaceSound(SOUND_WindowClose);
}
NW_HOOK_METHOD(0x1007b040, NConsoleWnd, HideSummonWnd)

// 0x1007b0d0 - the same, in another order
void NConsoleWnd::HidePetWnd()
{
    GetDesktop()->PetStatusWnd->HideWindow();
    GetDesktop()->SummonedStatusWnd->HideWindow();
    GetDesktop()->PetWnd->HideWindow();
    GetDesktop()->SummonedWnd->HideWindow();
    NCSummonedStatusWnd* Summoned = GetDesktop()->SummonedStatusWnd;
    Summoned->TimeBar->Stop();
    Summoned->TimeBar->Reset();
    nw::Field<UBOOL>(Summoned, 0x44C) = 0;
    PlayInterfaceSound(SOUND_WindowClose);
}
NW_HOOK_METHOD(0x1007b0d0, NConsoleWnd, HidePetWnd)

// 0x100690e0
void NConsoleWnd::ShowPetNameBtn(INT bShow)
{
    GetDesktop()->PetWnd->SetCanName(bShow);
}
NW_HOOK_METHOD(0x100690e0, NConsoleWnd, ShowPetNameBtn)

// 0x10069100 (vslot 241) - the pet's (the type is not used)
void NConsoleWnd::ReceivePetStatusInfo(INT Type, L2ParamStack& Params)
{
    GetDesktop()->PetStatusWnd->SetInfo(Params);
}
NW_HOOK_METHOD(0x10069100, NConsoleWnd, ReceivePetStatusInfo)

// 0x10069120 - a new list of the pet's items follows (AddPetInventoryItem)
void NConsoleWnd::ReceivePetItemList()
{
    GetDesktop()->PetWnd->ClearInventory();
}
NW_HOOK_METHOD(0x10069120, NConsoleWnd, ReceivePetItemList)

// 0x100727a0 - the summon's time left; the first time, its whole time
void NConsoleWnd::SetSummonRemainTime(INT Remaining, INT Total)
{
    NCSummonedStatusWnd* Wnd = GetDesktop()->SummonedStatusWnd;
    if (nw::Field<INT>(Wnd, 0x44C))   // the timer's flag: C4 +0x44C (C3 +0x410)
        Wnd->SetRemaining(Remaining);
    else
        Wnd->StartTimer(Total);
}
NW_HOOK_METHOD(0x100727a0, NConsoleWnd, SetSummonRemainTime)

/*-----------------------------------------------------------------------------
    The siege, the minimap, a GM's views.
-----------------------------------------------------------------------------*/

// 0x100721f0 (vslot 263) - a castle's siege: its id, whether the player's clan owns it, the owner clan's id, name and
// leader, its alliance's id and name, the server's time and the siege's (C3: the window takes the focus)
void NConsoleWnd::ReceiveSiegeInfo(L2ParamStack& Params)
{
    NSiegeCastleInfo Info;
    Info.CastleID = (INT)Params.Top();
    Info.bIsOwner = (INT)Params.Top();
    Info.PledgeID = (INT)Params.Top();
    appStrcpy(Info.ClanName, (TCHAR*)Params.Top());
    appStrcpy(Info.LordName, (TCHAR*)Params.Top());
    Info.AllianceID = (INT)Params.Top();
    appStrcpy(Info.AllianceName, (TCHAR*)Params.Top());
    Info.CurrentTime = (INT)Params.Top();
    Info.SiegeTime = (INT)Params.Top();
    GetDesktop()->SiegeInfoWnd->ShowWindow();
    GetDesktop()->SiegeInfoWnd->SetFocus();
    static_cast<NCSiegeInfoMainWnd*>(GetDesktop()->SiegeInfoWnd)->SetCastleInfo(&Info);
}
NW_HOOK_METHOD(0x100721f0, NConsoleWnd, ReceiveSiegeInfo)

// 0x10072310 - a clan of the attacking side: its id, name and leader, crest, the time it
// applied, its alliance's id, name and leader
void NConsoleWnd::ReceiveSiegeAttackerList(L2ParamStack& Params)
{
    NSiegeClanInfo Info;
    Info.PledgeID = (INT)Params.Top();
    appStrcpy(Info.ClanName, (TCHAR*)Params.Top());
    appStrcpy(Info.LeaderName, (TCHAR*)Params.Top());
    Info.CrestID = (INT)Params.Top();
    Info.ApplyTime = (INT)Params.Top();
    Info.AllianceID = (INT)Params.Top();
    appStrcpy(Info.AllianceName, (TCHAR*)Params.Top());
    appStrcpy(Info.AllianceLeader, (TCHAR*)Params.Top());
    static_cast<NCSiegeInfoMainWnd*>(GetDesktop()->SiegeInfoWnd)->AddAttacker(&Info);
}
NW_HOOK_METHOD(0x10072310, NConsoleWnd, ReceiveSiegeAttackerList)

// 0x10072400 - a clan of the defending side: as an attacker's, with its status after the time
void NConsoleWnd::ReceiveSiegeDefenderList(L2ParamStack& Params)
{
    NSiegeClanInfo Info;
    Info.PledgeID = (INT)Params.Top();
    appStrcpy(Info.ClanName, (TCHAR*)Params.Top());
    appStrcpy(Info.LeaderName, (TCHAR*)Params.Top());
    Info.CrestID = (INT)Params.Top();
    Info.ApplyTime = (INT)Params.Top();
    Info.Type = (INT)Params.Top();
    Info.AllianceID = (INT)Params.Top();
    appStrcpy(Info.AllianceName, (TCHAR*)Params.Top());
    appStrcpy(Info.AllianceLeader, (TCHAR*)Params.Top());
    static_cast<NCSiegeInfoMainWnd*>(GetDesktop()->SiegeInfoWnd)->AddDefender(&Info);
}
NW_HOOK_METHOD(0x10072400, NConsoleWnd, ReceiveSiegeDefenderList)

// 0x10072500 (vslot 266) - a date the castle's lord can choose
void NConsoleWnd::AddSelectableTime(INT Time)
{
    static_cast<NCSiegeInfoMainWnd*>(GetDesktop()->SiegeInfoWnd)->DateInfo->AddTime(Time);
}
NW_HOOK_METHOD(0x10072500, NConsoleWnd, AddSelectableTime)

// 0x10072520
void NConsoleWnd::EndSiegeAttackerList()
{
    static_cast<NCSiegeInfoMainWnd*>(GetDesktop()->SiegeInfoWnd)->AttackerInfo->UpdateButtons();
}
NW_HOOK_METHOD(0x10072520, NConsoleWnd, EndSiegeAttackerList)

// 0x10072540
void NConsoleWnd::EndSiegeDefenderList()
{
    static_cast<NCSiegeInfoMainWnd*>(GetDesktop()->SiegeInfoWnd)->DefenderInfo->UpdateButtons();
}
NW_HOOK_METHOD(0x10072540, NConsoleWnd, EndSiegeDefenderList)

// 0x10068a40
void NConsoleWnd::ClearAttackerList()
{
    static_cast<NCSiegeInfoMainWnd*>(GetDesktop()->SiegeInfoWnd)->AttackerInfo->ClearList();
}
NW_HOOK_METHOD(0x10068a40, NConsoleWnd, ClearAttackerList)

// 0x10068a60
void NConsoleWnd::ClearDefenderList()
{
    static_cast<NCSiegeInfoMainWnd*>(GetDesktop()->SiegeInfoWnd)->DefenderInfo->ClearList();
}
NW_HOOK_METHOD(0x10068a60, NConsoleWnd, ClearDefenderList)

// C1 only: no function of this name in the C3 image (the map window has no item marks); an inert body
void NConsoleWnd::ShowMapItem(INT ItemID)
{
}

// C2 CANDIDATE: the C2 function is not the same code as the C1 one; kept only while it matches the stock one (verify_data)
// 0x10069230 - the character a GM looks at
void NConsoleWnd::GMViewCharacterInfo(User& Info)
{
    GetDesktop()->GMDetailStatusWnd->SetTarget(Info);
}
NW_HOOK_METHOD(0x10069230, NConsoleWnd, GMViewCharacterInfo)

// 0x100727d0 (vslot 276) - the clan of the character a GM looks at
void NConsoleWnd::GMViewReceivePledgeInfo(PledgeInfo& Info)
{
    guard(NConsoleWnd::GMViewReceivePledgeInfo);
    NPledgeInfoC3 Pledge;
    Pledge.PledgeID = Info.PledgeID;
    if (Info.Name)
        appStrcpy(Pledge.Name, Info.Name);
    if (Info.LeaderName)
        appStrcpy(Pledge.LeaderName, Info.LeaderName);
    Pledge.CrestID = Info.CrestID;
    Pledge.Level = Info.Level;
    Pledge.bHasCastle = Info.bHasCastle;
    Pledge.bHasHideout = Info.bHasHideout;
    Pledge.Unknown94 = Info.Unknown90;
    Pledge.Unknown98 = Info.Unknown94;
    Pledge.Status = Info.Status;
    Pledge.UnknownE8 = Info.Unknown9C;
    Pledge.Unknown62C = nw::Field<INT>(&Info, 0xD0);
    Pledge.Unknown630 = nw::Field<INT>(&Info, 0xD4);
    Pledge.Unknown634 = nw::Field<INT>(&Info, 0xD8);
    GetDesktop()->GMActionWnd->PledgeWnd->SetPledgeInfo(&Pledge);
    unguard;
}
NW_HOOK_METHOD(0x100727d0, NConsoleWnd, GMViewReceivePledgeInfo)

// 0x10069260 - its clan's member list follows (GMViewAddPledgeMember)
void NConsoleWnd::GMViewReceivePledgeMemberList()
{
    guard(NConsoleWnd::ReceivePledgeMemberList);
    GetDesktop()->GMActionWnd->PledgeWnd->ClearMembers();
    unguard;
}
NW_HOOK_METHOD(0x10069260, NConsoleWnd, GMViewReceivePledgeMemberList)

// 0x10072910 (vslot 278) - as AddPledgeMember, into the GM's clan page
void NConsoleWnd::GMViewAddPledgeMember(L2ParamStack& Params)
{
    guard(NConsoleWnd::AddPledgeMember);
    NPledgeMember Member;
    TCHAR* Name = (TCHAR*)Params.Top();
    if (Name)
        appStrcpy(Member.Name, Name);
    Member.Level = (INT)Params.Top();
    Member.ClassID = (INT)Params.Top();
    Member.Unknown208 = (INT)Params.Top();
    Member.Unknown20C = (INT)Params.Top();
    Member.bOnline = (INT)Params.Top();
    GetDesktop()->GMActionWnd->PledgeWnd->AddMember(&Member);
    unguard;
}
NW_HOOK_METHOD(0x10072910, NConsoleWnd, GMViewAddPledgeMember)

// 0x100692f0 - its skill list follows (GMViewSkillList): the lists are emptied by the stock 0x100A1150 (a tail call)
void NConsoleWnd::GMViewShowSkillWnd()
{
    GetDesktop()->GMMagicSkillWnd->SkillWnd->ClearLists();
}
NW_HOOK_METHOD(0x100692f0, NConsoleWnd, GMViewShowSkillWnd)

// 0x10072a00 - as ReceiveSkillList, into the GM's skill window
void NConsoleWnd::GMViewSkillList(L2ParamStack& Params)
{
    NItemInfo Info;
    Info.Param = (INT)Params.Top();
    Info.UnknownC20 = (INT)Params.Top();
    Info.ID = (INT)Params.Top();
    if (Info.UnknownC20 > 0)
    {
        SetSkillTexts(this, Info);
        GetDesktop()->GMMagicSkillWnd->SkillWnd->AddSkill(&Info);
    }
}
NW_HOOK_METHOD(0x10072a00, NConsoleWnd, GMViewSkillList)

// 0x10072b40 - a magic of the list: its level and id
void NConsoleWnd::GMViewMagicList(L2ParamStack& Params)
{
    NItemInfo Info;
    Info.UnknownC20 = (INT)Params.Top();
    Info.ID = (INT)Params.Top();
    if (Info.UnknownC20 > 0)
    {
        SetSkillTexts(this, Info);
        GetDesktop()->GMMagicSkillWnd->SkillWnd->AddSkill(&Info);
    }
}
NW_HOOK_METHOD(0x10072b40, NConsoleWnd, GMViewMagicList)

// 0x10069310 (vslot 283) - its quest list follows (GMViewAddQuestID)
void NConsoleWnd::GMViewReceiveQuestList()
{
    guard(NConsoleWnd::ReceiveQuestList);
    ((NCQuestGaraTreeWndExt*)GetDesktop()->GMQuestWnd)->ResetTree();
    unguard;
}
NW_HOOK_METHOD(0x10069310, NConsoleWnd, GMViewReceiveQuestList)

// 0x10069390 (vslot 284) - as AddQuestID, into the GM's quest window
void NConsoleWnd::GMViewAddQuestID(INT QuestID, INT Step)
{
    guard(NConsoleWnd::AddQuestID);
    for (INT i = 0; i < 2000; i++)
    {
        NQuestHeadC4& Line = GQuestHeadsC4[i];
        if (Line.bValid && Line.QuestID == QuestID)
        {
            ((NCQuestGaraTreeWndExt*)GetDesktop()->GMQuestWnd)->AddQuest(QuestID, Step);
            break;
        }
    }
    unguard;
}
NW_HOOK_METHOD(0x10069390, NConsoleWnd, GMViewAddQuestID)
