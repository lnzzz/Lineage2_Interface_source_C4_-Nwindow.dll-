/*=============================================================================
    NConsoleWndLogin.cpp - what the engine tells the console before the game and around it
    (UGameEngine::On* handlers, one slot each): the login and its failures, the announcements,
    the server list, the character selection and creation, entering the world, the restart,
    the logout, a closed server, a blocked account, the protocol version.
=============================================================================*/
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NConsoleWndLogin.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NDialogInfo.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NCLoginWnd.h"
#include "nwindow/NCAuthWnd.h"
#include "nwindow/NCEulaWnd.h"
#include "nwindow/NCNoticeWnd.h"
#include "nwindow/NCWarningWnd.h"
#include "nwindow/NCServerSelectWnd.h"
#include "nwindow/NCEditBox.h"
#include "nwindow/NCLobbyWnd.h"
#include "nwindow/NCPawnCreateWnd.h"
#include "nwindow/NCLoadingWnd.h"
#include "nwindow/NCMessageWnd.h"
#include "nwindow/NCOptionWnd.h"
#include "nwindow/NCRestartMenuWnd.h"
#include "nwindow/NCGodWnd.h"
#include "nwindow/NConsoleWndExt.h"
#if NW_CUSTOM
#include "nwindow/NCustomHwid.h"
#endif

// system messages (systemmsg ids)
enum
{
    MSG_Disconnected = 0,              // "Your connection has been disconnected from the server."
    MSG_TooManyCharacters = 77,        // "You cannot create another character. Please delete the
                                       //   existing character and try again."
    MSG_NameExists = 79,               // "Name already exists."
    MSG_NameInvalid = 80,              // "Please insert 1~16 English characters."
    MSG_ProtocolQuit = 88,             // "Protocol version is different. Please quit the program."
    MSG_ProtocolContinue = 89,         // "Protocol version is different. Please continue."
    MSG_DisconnectedRetry = 127,       // "Your connection has been disconnected from the server.
                                       //   Please try again."
    MSG_CreateFailed = 128,            // "You have failed to create a character."
    MSG_DeleteFailed = 306,            // "You have failed to delete the character."
    MSG_AskRevive = 332,               // "Do you want to be restored?"
    MSG_TemporaryPassword = 396,       // "Please login after changing your temporary password."
    MSG_NoTimeLeft = 398,              // "You have no more time left on your account."
    MSG_SystemError = 399,             // "System error."
    MSG_BlockReason = 428,             // .. 443: a reason each, a bit of BlockedAccount's
    MSG_Withdrawn = 444,               // "The following account may not be accessed due to a
                                       //   withdrawal request that has been submitted." and
                                       //   445..447 after it
    MSG_LoginSystemError = 448,        // "System error, please log in again later."
    MSG_WrongPassword = 449,           // "Password does not match this acount."
    MSG_ConfirmAccount = 450,          // "Confirm your account information and log in again later."
    MSG_WrongAccount = 453,            // "Your account information is incorrect."
    MSG_ContactSupport = 454,          // "Please contact the Customer Support Center or the 1: 1
                                       //   inquiry service of the Lineage II official website."
    MSG_AccountInUse = 455,            // "The account is already in use. Access denied."
    MSG_AdultsOnly = 456,              // "Lineage II game services may only be used by adults 18
                                       //   years of age and older."
    MSG_Maintenance = 457,             // "Server under maintenance. Please try again later."
    MSG_TermExpired = 458,             // "Your usage term has been expired." 459, 460 after it
    MSG_AccessFailed = 461,            // "Access failed." 462, 463 after it
    MSG_ClanMemberDelete = 540,        // "Clan leaders cannot be deleted. DD Dissolve the clan and
                                       //   try again."
    MSG_ClanLeaderDelete = 541,        // "You cannot delete a clan member. Withdraw from the clan
                                       //   and try again."
    MSG_KoreaOnly = 621,               // "This server is reserved for players in Korea..."
    MSG_WeekTimeOver = 756,            // "This week's usage time has finished." (no reason of C2's AuthLoginFail has it)
    MSG_LoginReason31 = 1243,          // C2: the reasons 31 and 32 of AuthLoginFail (the security card's;
    MSG_LoginReason32 = 1242,          //   the texts are not known here)
    MSG_LoginReason33 = 1340,          // C3: the reasons 33 and 35 of AuthLoginFail (texts not read here)
    MSG_LoginReason15 = 1650,          // C4: the reason 15 of AuthLoginFail has a message of its own
    MSG_CreateFail4 = 204,             // C4: the reason 4 of CharacterCreateFail (the name line is typed again)
    MSG_LoginReason35 = 1407,
};

// the dialog boxes opened here (NDialogInfo::ID; NCDialogBox sends the answer)
enum
{
    DIALOG_DISCONNECTED = 0x1D,        // OK only: the game ends
    DIALOG_REVIVE = 0x20,
};

// a system message's text
static FORCEINLINE const TCHAR* SysMsg(INT ID)
{
    return *GL2GameData.SystemMsg[ID].Message;
}

// the login box's mode, card and card key cleared, and the account line shown and given the focus (the box's fields +0xE8 / +0xEC /
// +0xF0, the account line +0xFC)
static FORCEINLINE void ResetAuthWnd(NConsoleWnd* Console)
{
    NCWnd* Auth = LoginChild(Console, 0xE8);
    nw::Field<INT>(Auth, 0xE8) = 0;
    nw::Field<INT>(Auth, 0xF0) = 0;
    nw::Field<INT>(Auth, 0xEC) = 0;
    nw::Field<NCWnd*>(Auth, 0xFC)->ShowWindow();
    nw::Field<NCWnd*>(Auth, 0xFC)->SetFocus();
}

// back to the login box, the server list and the other boxes of the login screen hidden (after the login failures);
// bReset: the box starts again (account mode)
static FORCEINLINE void ShowAuthWnd(NConsoleWnd* Console, UBOOL bReset = 1)
{
    LoginChild(Console, 0xF8)->HideWindow();
    LoginChild(Console, 0xEC)->HideWindow();
    LoginChild(Console, 0xF0)->HideWindow();
    LoginChild(Console, 0xF4)->HideWindow();
    LoginChild(Console, 0xE8)->ShowWindow();
    if (bReset)
        ResetAuthWnd(Console);
    Console->SetState(L2CS_Login);
}

// the voice of a stage: "SystemMsg_<country>.<name>" ("e" when the data is not the native language's)
static FORCEINLINE void PlayStageVoice(NConsoleWnd* Console, const TCHAR* Format)
{
    TCHAR Name[64] = { 0 };
    appSprintf(Name, Format, GIsNative ? (const TCHAR*)GCountryCode : TEXT("e"));
    Console->PlayVoiceName(Name);
}

// the login box of the stock (NCAuthWnd, +0xE8 of the login screen) back to its first state: the mode, the card and the card key
// cleared, the account line shown and focused (the stock folded it for StartNotice's second and third branches)
struct NAuthWndExt
{
    __declspec(noinline) void ResetMode();        // 0x1006EA10 (name ours)
};

// 0x1006ea10 (name ours)
void NAuthWndExt::ResetMode()
{
    NCWnd* Wnd = (NCWnd*)this;
    NCWnd* Account = nw::Field<NCWnd*>(Wnd, 0xFC);
    nw::Field<INT>(Wnd, 0xE8) = 0;
    nw::Field<INT>(Wnd, 0xF0) = 0;
    nw::Field<INT>(Wnd, 0xEC) = 0;
    Account->ShowWindow();
    nw::Field<NCWnd*>(Wnd, 0xFC)->SetFocus();
}
NW_HOOK_METHOD(0x1006ea10, NAuthWndExt, ResetMode)

/*-----------------------------------------------------------------------------
    The login.
-----------------------------------------------------------------------------*/

// 0x1006f1d0 (vslot 32) - the login server refused: the reason's message, then the login box again (for the reasons
// without a message the stock shows an uninitialized buffer; empty here). C3: the reason 30 (the week's time) and the
// reasons 33 and 35; 31 leaves the box as it is. C4: the reason 15 has its own message (1650) and the failure ends with
// three calls of the GM server change's slots
void NConsoleWnd::AuthLoginFail(INT Reason)
{
    TCHAR Text[256];
    Text[0] = 0;
    switch (Reason)
    {
    case 1:
        appSprintf(Text, TEXT("%s"), SysMsg(MSG_LoginSystemError));
        break;
    case 2:
    case 3:
        appSprintf(Text, TEXT("%s %s"), SysMsg(MSG_WrongPassword), SysMsg(MSG_ConfirmAccount));
        break;
    case 5:
        appSprintf(Text, TEXT("%s %s"), SysMsg(MSG_WrongAccount), SysMsg(MSG_ContactSupport));
        break;
    case 7:
        appSprintf(Text, TEXT("%s"), SysMsg(MSG_AccountInUse));
        break;
    case 12:
        appSprintf(Text, TEXT("%s"), SysMsg(MSG_AdultsOnly));
        break;
    case 15:
        appSprintf(Text, TEXT("%s"), SysMsg(MSG_LoginReason15));
        break;
    case 16:
        appSprintf(Text, TEXT("%s"), SysMsg(MSG_Maintenance));
        break;
    case 17:
        appSprintf(Text, TEXT("%s"), SysMsg(MSG_TemporaryPassword));
        break;
    case 18:
        appSprintf(Text, TEXT("%s %s %s"), SysMsg(MSG_TermExpired), SysMsg(MSG_TermExpired + 1),
                   SysMsg(MSG_TermExpired + 2));
        break;
    case 19:
        appSprintf(Text, TEXT("%s"), SysMsg(MSG_NoTimeLeft));
        break;
    case 20:
        appSprintf(Text, TEXT("%s"), SysMsg(MSG_SystemError));
        break;
    case 21:
        appSprintf(Text, TEXT("%s"), SysMsg(MSG_AccessFailed));
        break;
    case 22:
        appSprintf(Text, TEXT("%s"), SysMsg(MSG_KoreaOnly));
        break;
    case 30:
        appSprintf(Text, TEXT("%s"), SysMsg(MSG_WeekTimeOver));
        break;
    case 31:
        appSprintf(Text, TEXT("%s"), SysMsg(MSG_LoginReason31));
        break;
    case 32:
        appSprintf(Text, TEXT("%s"), SysMsg(MSG_LoginReason32));
        break;
    case 33:
        appSprintf(Text, TEXT("%s"), SysMsg(MSG_LoginReason33));
        break;
    case 35:
        appSprintf(Text, TEXT("%s"), SysMsg(MSG_LoginReason35));
        break;
    case 4:
    case 6:
    case 8:
    case 9:
    case 10:
    case 11:
    case 13:
    case 14:
        appSprintf(Text, TEXT("%s %s %s"), SysMsg(MSG_AccessFailed), SysMsg(MSG_AccessFailed + 1),
                   SysMsg(MSG_AccessFailed + 2));
        break;
    }
    ShowMessageWnd(Text);
    ShowAuthWnd(this, Reason != 31);
    nw::VirtualCall<void>(this, 445, 0);   // SetGMValue(0)
    nw::VirtualCall<void>(this, 446);      // the GM step down, twice
    nw::VirtualCall<void>(this, 446);
}
NW_HOOK_METHOD(0x1006f1d0, NConsoleWnd, AuthLoginFail)

// 0x1006f5d0 (vslot 33) - after the login: the announcements, if there are any, else straight to the server list; the
// value after them goes to the box. With the GM's server change on (+0x3C94) only its next step (4) is taken. C4 has the
// kinds in this order: the text at GL2GameData +0xAA9D4 in the login screen's window at +0xF0 (the test server's text,
// +0xAA9E0, when GL2TestServer is set), a Chinese client's window at +0xF4 and, when there is no text, the login box alone:
// the first two leave the login box in account mode and hide it and the server list. The value after the announcements goes
// to the window's +0xF8 / +0xF0 and to the server list's key (0x10100370), as the stock does
void NConsoleWnd::StartNotice(L2ParamStack& Params)
{
    if (bGMChange)
    {
        nw::VirtualCall<void>(this, 441, 4);   // SetGMChangeServerState(4)
        return;
    }
    if (GL2GameData.NoticeText.Num() && GL2GameData.NoticeText.Num() - 1 > 0)
    {
        ResetAuthWnd(this);
        LoginChild(this, 0xE8)->HideWindow();
        LoginChild(this, 0xF8)->HideWindow();
        LoginChild(this, 0xF0)->ShowWindow();
        LoginChild(this, 0xF0)->SetFocus();
        static_cast<NCEulaWnd*>(LoginChild(this, 0xF0))->ClearText();
        if (GL2TestServer)
            static_cast<NCEulaWnd*>(LoginChild(this, 0xF0))->SetText(*nw::Field<FString>(&GL2GameData, 0xAA9E0));
        else
            static_cast<NCEulaWnd*>(LoginChild(this, 0xF0))->SetText(*GL2GameData.NoticeText);
        nw::Field<void*>(LoginChild(this, 0xF0), 0xF8) = Params.Top();
        ((NCLoginServerWndExt*)nw::Field<NCWnd*>(LoginChild(this, 0xF8), 0xE8))->SetKey(Params.Top());
        SetState(L2CS_Login);
    }
    else if (GLanguageType == 4)
    {
        ((NAuthWndExt*)LoginChild(this, 0xE8))->ResetMode();
        LoginChild(this, 0xE8)->HideWindow();
        LoginChild(this, 0xF8)->HideWindow();
        LoginChild(this, 0xF4)->ShowWindow();
        LoginChild(this, 0xF4)->SetFocus();
        nw::Field<void*>(LoginChild(this, 0xF4), 0xF0) = Params.Top();
        ((NCLoginServerWndExt*)nw::Field<NCWnd*>(LoginChild(this, 0xF8), 0xE8))->SetKey(Params.Top());
        SetState(L2CS_Login);
    }
    else
    {
        ((NAuthWndExt*)LoginChild(this, 0xE8))->ResetMode();
        ((NCLoginServerWndExt*)nw::Field<NCWnd*>(LoginChild(this, 0xF8), 0xE8))->SetKey(Params.Top());
        nw::VirtualCall<void>(this, 424);   // RequestServerList
    }
}
NW_HOOK_METHOD(0x1006f5d0, NConsoleWnd, StartNotice)

// 0x10065740 (vslot 44) - the server list follows (AddServerList), with the server last played on: the god window is told (it
// empties the message line, hides the options and starts the login screen over)
void NConsoleWnd::ReceiveServerList(INT LastServerID)
{
    guard(NConsoleWnd::ReceiveServerList);
    if (GodWnd)
        static_cast<NCGodWnd*>(GodWnd)->ReceiveServerList(LastServerID);
    unguard;
}
NW_HOOK_METHOD(0x10065740, NConsoleWnd, ReceiveServerList)

// 0x100657c0 (vslot 45) - a server of the list goes to the server list window (the container's +0xE8 child)
void NConsoleWnd::AddServerList(L2ParamStack& Params)
{
    guard(NConsoleWnd::AddServerList);
    nw::Field<NCServerSelectWnd*>(LoginChild(this, 0xF8), 0xE8)->AddServer(Params);
    unguard;
}
NW_HOOK_METHOD(0x100657c0, NConsoleWnd, AddServerList)

// 0x10065850 (vslot 46) - the list is complete; the GM's server change goes to its step 5
void NConsoleWnd::EndServerList()
{
    guard(NConsoleWnd::EndServerList);
    nw::Field<NCServerSelectWnd*>(LoginChild(this, 0xF8), 0xE8)->OnServerListEnd();
    nw::VirtualCall<void>(this, 441, 5);   // SetGMChangeServerState(5)
    unguard;
}
NW_HOOK_METHOD(0x10065850, NConsoleWnd, EndServerList)

// 0x1006bcd0 (vslot 352) - the server's ping to the server list (the login window's +0xF8 child's +0xE8 object), when there is one
void NConsoleWnd::SetServerPing(INT ServerID, DWORD Ping)
{
    NCWnd* Login = nw::Field<NCWnd*>(GodWnd, 0xEC);
    NCWnd* Box = nw::Field<NCWnd*>(nw::Field<NCWnd*>(Login, 0xF8), 0xE8);
    if (Box)
        static_cast<NCServerSelectWnd*>(Box)->SetPing(ServerID, Ping);
}
NW_HOOK_METHOD(0x1006bcd0, NConsoleWnd, SetServerPing)

// 0x1006ee00 (vslot 303) - the account is blocked, with the server's text: the message is built from four lines of
// systemmsg and takes the text
void NConsoleWnd::BlockedAccountWithMsg(TCHAR* Msg)
{
    TCHAR Format[256];
    TCHAR Text[256];
    if (Msg && *Msg)
    {
        appSprintf(Format, TEXT("%s %s %s %s"), SysMsg(MSG_Withdrawn), SysMsg(MSG_Withdrawn + 1),
                   SysMsg(MSG_Withdrawn + 2), SysMsg(MSG_Withdrawn + 3));
        appSprintf(Text, Format, Msg);
        ShowMessageWnd(Text);
    }
    ShowAuthWnd(this);
}
NW_HOOK_METHOD(0x1006ee00, NConsoleWnd, BlockedAccountWithMsg)

// 0x1006ef90 (vslot 304) - the account is blocked: the message of the lowest reason bit set (0..15), or the ninth's for
// the bits 16 and 17 (C3)
void NConsoleWnd::BlockedAccount(INT Reason)
{
    INT Bit;
    for (Bit = 0; Bit < 16; Bit++)
    {
        if (Reason & (1 << Bit))
        {
            ShowMessageWnd(SysMsg(MSG_BlockReason + Bit));
            break;
        }
    }
    if (Bit == 16 && (Reason & 0x30000))
        ShowMessageWnd(SysMsg(MSG_BlockReason + 9));
    ShowAuthWnd(this);
}
NW_HOOK_METHOD(0x1006ef90, NConsoleWnd, BlockedAccount)

// 0x10065c90 (vslot 177) - the server's protocol version: 0 when it differs. With [URL] L2VersionCheck (1 when not
// set) the client logs out. A second value is kept (+0x3C74)
void NConsoleWnd::ReceiveVersionCheck(L2ParamStack& Params)
{
    BYTE bSame = (BYTE)(INT)Params.Top();
    INT Second = (INT)Params.Top();
    UBOOL bVersionCheck = 1;
    if (!GConfig->GetBool(TEXT("URL"), TEXT("L2VersionCheck"), bVersionCheck, NULL))
        bVersionCheck = 1;
    if (bVersionCheck)
    {
        if (!bSame)
        {
            Engine->NetHandler->SendLogOutPacket();  // RequestLogout's
            ShowMessageWnd(SysMsg(MSG_ProtocolQuit));
        }
    }
    else if (!bSame)
        ShowMessageWnd(SysMsg(MSG_ProtocolContinue));
    Unknown39D0 = Second;
}
NW_HOOK_METHOD(0x10065c90, NConsoleWnd, ReceiveVersionCheck)

/*-----------------------------------------------------------------------------
    The character selection and creation, entering the world.
-----------------------------------------------------------------------------*/

// 0x100635e0 (vslot 35) - the character list follows (AddCharacterInfo): the selection replaces the login screen
// (not while in the world): the login screen starts over and hides with the options window, the character
// creation hides, the selection is emptied and shown with the focus, the camera goes to the selection and the
// state is the lobby's. C3: the engine passes one argument (ret 4; include/ue2 still declares none, so the body is
// the non-virtual ReceiveCharacterInfoX hooked at the slot's address and the slot's own stub jumps to it): the
// voice "SystemMsg_k.char_start" is played when it is 0
#if NW_HOOK_MODE
NW_NAKED void NConsoleWnd::ReceiveCharacterInfo(INT bNoVoice) NW_STOCK(0x1006F8B0)
#else
// standalone: the slot IS ReceiveCharacterInfoX (the stock slot holds 0x1006f8b0 itself)
NW_SLOT_IS("?ReceiveCharacterInfo@NConsoleWnd@@UAEXH@Z", "?ReceiveCharacterInfoX@NConsoleWnd@@QAEXH@Z")   // NConsoleWnd::ReceiveCharacterInfo = ...X (0x1006f8b0)
#endif

// 0x1006f8b0 (vslot 35) - the character list follows (AddCharacterInfo): the selection replaces the login screen
// (not while in the world): the login screen starts over and hides with the options window, the character
// creation hides, the selection is emptied and shown with the focus, the camera goes to the selection and the
// state is the lobby's; the voice "SystemMsg_<country>.char_start" is played when bNoVoice is 0 (the engine passes one
// argument; include/ue2 still declares none, so the body is the non-virtual ReceiveCharacterInfoX hooked at the slot's
// address and the slot's own stub jumps to it). The GM's server change goes to its step 6
void NConsoleWnd::ReceiveCharacterInfoX(INT bNoVoice)
{
    guard(NConsoleWnd::ReceiveCharacterInfo);
#if NW_CUSTOM
    NCustomHwid::Send();   // custom: the PC's id goes to the server before a character is chosen
#endif
    if (State != L2CS_InGame)
    {
        ((NCLoginWnd*)LoginGroup(this))->ResetWindows();
        LoginGroup(this)->HideWindow();
        nw::Field<NCWnd*>(GodWnd, 0xF4)->HideWindow();   // the options window
        PawnCreateWnd->HideWindow();
        LobbyWnd->ClearCharacters();
        LobbyWnd->ShowWindow();
        LobbyWnd->SetFocus();
        MoveCameraByState(L2CS_None, L2CS_Lobby);
        SetState(L2CS_Lobby);
        if (!bNoVoice)
            PlayStageVoice(this, TEXT("SystemMsg_%s.char_start"));
        nw::VirtualCall<void>(this, 441, 6);   // SetGMChangeServerState(6)
    }
    unguard;
}
NW_HOOK_METHOD(0x1006f8b0, NConsoleWnd, ReceiveCharacterInfoX)

// 0x10065a00 (vslot 36) - a character of the list goes to the selection (not while in the world)
void NConsoleWnd::AddCharacterInfo(User Info)
{
    guard(NConsoleWnd::AddCharacterInfo);
    if (!LobbyWnd)
        appFailAssert("m_pLobbyWnd", "Src\\NConsoleWindow.cpp", 0x82F);
    if (State != L2CS_InGame)
        LobbyWnd->AddCharacter(Info);
    unguard;
}
NW_HOOK_METHOD(0x10065a00, NConsoleWnd, AddCharacterInfo)

// 0x1006fa20 (vslot 37) - into the world: the manor list is asked for, the desktop replaces the loading screen, and the screen
// takes the game's resolution if it differs (the result is not read). The four checks are the stock's own (names of its
// members, lines of Src\\NConsoleWindow.cpp), and the game's size follows what the viewport got when SetRes succeeded
void NConsoleWnd::AcceptGameStart(INT Result)
{
    guard(NConsoleWnd::AcceptGameStart);
    if (!Desktop)
        appFailAssert("m_pConsole", "Src\\NConsoleWindow.cpp", 0x83A);
    if (!LoadingWnd)
        appFailAssert("m_pLoadingWnd", "Src\\NConsoleWindow.cpp", 0x83C);
    if (!Engine->Client->Viewports(0))
        appFailAssert("m_Engine->Client->Viewports(0)", "Src\\NConsoleWindow.cpp", 0x83D);
    if (!Engine->GRenDev)
        appFailAssert("m_Engine->GRenDev", "Src\\NConsoleWindow.cpp", 0x83E);
    RequestManorList();
    LoadingWnd->HideWindow();
    Desktop->ShowWindow();
    UViewport* Viewport = Engine->Client->Viewports(0);
    INT ScreenX = GSEKScreenX;
    if (Viewport->SizeX != ScreenX)
    {
        if (Engine->GRenDev->SetRes(Viewport, ScreenX, GSEKScreenY, GColorBits / 8, GRefreshRate,
                                    Unknown3720, 1))   // +0x3C58: fullscreen
        {
            GSEKScreenX = Engine->Client->Viewports(0)->SizeX;
            GSEKScreenY = nw::Field<INT>(Engine->Client->Viewports(0), 0x98);   // UViewport::SizeY
        }
    }
    SetState(L2CS_InGame);
    unguard;
}
NW_HOOK_METHOD(0x1006fa20, NConsoleWnd, AcceptGameStart)

// 0x10065b40 - only the failures are answered
void NConsoleWnd::AcceptCharacterDelete(INT Result)
{
    guard(NConsoleWnd::AcceptCharacterDelete);
    switch (Result)
    {
    case 1:
        ShowMessageWnd(SysMsg(MSG_DeleteFailed));
        break;
    case 2:
        ShowMessageWnd(SysMsg(MSG_ClanLeaderDelete));
        break;
    case 3:
        ShowMessageWnd(SysMsg(MSG_ClanMemberDelete));
        break;
    }
    unguard;
}
NW_HOOK_METHOD(0x10065b40, NConsoleWnd, AcceptCharacterDelete)

// 0x1006fbd0 (vslot 39) - the creation may start: its screen replaces the selection, with the creation's voice
void NConsoleWnd::AcceptNewCharacter(INT Result)
{
    guard(NConsoleWnd::AcceptNewCharacter);
    if (Result)
    {
        LobbyWnd->HideWindow();
        PawnCreateWnd->Reset();
        PawnCreateWnd->ShowWindow();
        MoveCameraByState(L2CS_None, L2CS_Create);
        SetState(L2CS_Create);
        PlayStageVoice(this, TEXT("SystemMsg_%s.char_creation"));
    }
    else
    {
        SetState(L2CS_Lobby);
        ShowMessageWnd(SysMsg(MSG_CreateFailed));
    }
    unguard;
}
NW_HOOK_METHOD(0x1006fbd0, NConsoleWnd, AcceptNewCharacter)

// 0x1006fd20 (vslot 40) - only a failure is answered (a success plays the lobby's voice)
void NConsoleWnd::AcceptCharacterCreate(INT Result)
{
    guard(NConsoleWnd::AcceptCharacterCreate);
    if (!Result)
        ShowMessageWnd(SysMsg(MSG_CreateFailed));
    else
        PlayStageVoice(this, TEXT("SystemMsg_%s.char_lobby"));
    unguard;
}
NW_HOOK_METHOD(0x1006fd20, NConsoleWnd, AcceptCharacterCreate)

// 0x100658f0 (vslot 34) - the creation was refused: 0 the creation failed, 1 too many characters (the choices start
// over), 2 the name is taken, 3 the name is not valid, 4 (new) another message (204): the name is typed again
void NConsoleWnd::CharacterCreateFail(INT Reason)
{
    switch (Reason)
    {
    case 0:
        ShowMessageWnd(SysMsg(MSG_CreateFailed));
        break;
    case 1:
        ShowMessageWnd(SysMsg(MSG_TooManyCharacters));
        static_cast<NCPawnSetupWnd*>(nw::Field<NCWnd*>(PawnCreateWnd, 0xE8))->Reset();
        break;
    case 2:
    case 3:
    case 4:
        ShowMessageWnd(SysMsg(Reason == 2 ? MSG_NameExists : (Reason == 3 ? MSG_NameInvalid : MSG_CreateFail4)));
        nw::VirtualCall<void>(nw::Field<NCWnd*>(nw::Field<NCWnd*>(PawnCreateWnd, 0xE8), 0x108), 76);   // the name edit line emptied
        nw::Field<NCWnd*>(nw::Field<NCWnd*>(PawnCreateWnd, 0xE8), 0x108)->SetFocus();
        break;
    }
    SetState(L2CS_Create);
}
NW_HOOK_METHOD(0x100658f0, NConsoleWnd, CharacterCreateFail)

// 0x10066150 - the template of a class for the creation
void NConsoleWnd::AddRecommandedData(RecommandedDataStr Data)
{
    guard(NConsoleWnd::AddRecommandedData);
    PawnCreateWnd->SetRecommendedData(Data);
    unguard;
}
NW_HOOK_METHOD(0x10066150, NConsoleWnd, AddRecommandedData)

// 0x100661e0
void NConsoleWnd::EndRecommandedData()
{
    guard(NConsoleWnd::EndRecommandedData);
    PawnCreateWnd->EndRecommendedData();
    unguard;
}
NW_HOOK_METHOD(0x100661e0, NConsoleWnd, EndRecommandedData)

/*-----------------------------------------------------------------------------
    The restart, the revival, the logout, a closed server.
-----------------------------------------------------------------------------*/

// 0x10065da0 (guard name RestartBegin; vslot 188) - the restart was accepted (first): the god window is told, the game's windows
// give way to the loading screen: the desktop reset and hidden, the window at +0x26C hidden (C3Virtual202), the warning box at
// +0x3C08 (the stock calls its slot with no argument), the login stages' dialog box (+0x3C00) and the petition window (+0x3C04)
void NConsoleWnd::RestartResetWindows()
{
    guard(NConsoleWnd::RestartBegin);
    if (GodWnd)
        static_cast<NCGodWnd*>(GodWnd)->RestartBegin();
    GetDesktop()->Reset();
    Desktop->HideWindow();
    nw::VirtualCall<void>(this, 203);      // C3Virtual202
    nw::VirtualCall<void>(this, 418, -1);  // C4Virtual418(-1): the warning box hidden (the stock gives it the -1 that its frame's first push left on the stack)
    PetitionWnd->HideWindow();
    PetitionFeedBackWnd->HideWindow();
    LoadingWnd->ShowWindow();
    unguard;
}
NW_HOOK_METHOD(0x10065da0, NConsoleWnd, RestartResetWindows)

// 0x10065e80 (vslot 189) - (then) back to the character selection: the lobby's child is told first,
// the engine travels to the lobby map ("Lobby?L2Restart", from the [DefaultPlayer] URL) and the selection is shown again; the GM's
// server change goes to its step 3
void NConsoleWnd::RestartToLobby()
{
    if (nw::Field<void*>(LobbyWnd, 0xE8))
        ((NCLobbyWndExt*)nw::Field<void*>(LobbyWnd, 0xE8))->ClearSlots();
    FString Error;
    FURL DefaultURL(NULL);
    DefaultURL.LoadURLConfig(TEXT("DefaultPlayer"), TEXT("User"));
    FURL URL(&DefaultURL, TEXT("Lobby"), TRAVEL_Partial);
    URL.AddOption(TEXT("L2Restart"));
    if (!static_cast<UGameEngine*>(Engine)->Browse(URL, NULL, Error))
        GError->Logf(LocalizeError(TEXT("FailedBrowse"), TEXT("Engine")), *Error);
    SpawnDefaultCharacter();
    LoadingWnd->HideWindow();
    LobbyWnd->ClearCharacters();
    LobbyWnd->ShowWindow();
    LobbyWnd->SetFocus();
    SetState(L2CS_Lobby);
    MoveCameraByState(L2CS_None, L2CS_Lobby);
    nw::VirtualCall<void>(this, 441, 3);   // SetGMChangeServerState(3)
}
NW_HOOK_METHOD(0x10065e80, NConsoleWnd, RestartToLobby)

// 0x10004020 (vslot 43; ret 8) - the stock slot is the shared empty body of two arguments (EmptyTwoArgs, NLeftHelpers.cpp), which the
// linker folded for the empty virtuals that pop two values: nothing happens
#if NW_HOOK_MODE
NW_NAKED void NConsoleWnd::ReviveRequest(INT Param1, INT Param2) NW_STOCK(0x10004020)
#else
void NConsoleWnd::ReviveRequest(INT Param1, INT Param2)
{
}
#endif

// 0x10076ad0 - the server let the player go: the window positions are saved and the game
// ends
void NConsoleWnd::AcceptLogOut()
{
    SaveWindowPositions();
    appRequestExit(0);
}
NW_HOOK_METHOD(0x10076ad0, NConsoleWnd, AcceptLogOut)

#if NW_FEATURES
// phase 6: ReturnToLogin (included here, below the check()s: their __LINE__ stays the faithful
// build's)
#include "features/ReturnToLogin.h"
#endif

// the stock body is AcceptLogOut's (folded)
void NConsoleWnd::QuitGame()
{
#if NW_FEATURES
    // phase 6: ReturnToLogin (NWindow_C1 src/inject/ReturnToLogin.cpp): OK on the
    // "disconnected" box (the only caller) goes back to the login screen from the next Tick
    // instead of ending the game
    if (nwf::ReturnToLogin_QuitGame())
        return;
#endif
    SaveWindowPositions();
    appRequestExit(0);
}

// 0x10076ae0 (vslot 42) - the server closed the connection: in the world a dialog box whose OK ends the game, before
// it the login stages' own box (the god window's +0xFC). The desktop's messenger window is closed and hidden first, and the
// message is set for the reasons 0, 1 and 2 only (system message 0x7F)
void NConsoleWnd::ServerClose(INT Reason)
{
    // the stock keeps one dialog info per branch (two 0xA0 byte slots in a frame of 0x140 bytes) and destroys it
    // out of line (0x10006E20); the same here, so the frame is not smaller than the stock one
    BYTE InfoMemA[sizeof(NDialogInfo)];
    BYTE InfoMemB[sizeof(NDialogInfo)];
    if (State == L2CS_InGame)
    {
        ((NCConsoleExt*)Desktop)->LogoutMessenger();
        if (static_cast<NCConsole*>(Desktop)->MessengerWnd)
            static_cast<NCConsole*>(Desktop)->MessengerWnd->HideWindow();
        NDialogInfo* Info = ::new ((void*)InfoMemA) NDialogInfo;
        Info->ID = DIALOG_DISCONNECTED;
        Info->bMainBox = 1;
        Info->bInput = 0;
        Info->Owner = NULL;
        if (Reason == 0 || Reason == 1 || Reason == 2)
            Info->SetSystemMessage(MSG_DisconnectedRetry);
        ShowDialog(Info);
        Info->~NDialogInfo();
    }
    else
    {
        ((NCConsoleExt*)Desktop)->LogoutMessenger();
        if (static_cast<NCConsole*>(Desktop)->MessengerWnd)
            static_cast<NCConsole*>(Desktop)->MessengerWnd->HideWindow();
        NDialogInfo* Info = ::new ((void*)InfoMemB) NDialogInfo;
        Info->ID = DIALOG_DISCONNECTED;
        Info->bInput = 0;
        Info->Owner = NULL;
        if (Reason == 0 || Reason == 1 || Reason == 2)
            Info->SetSystemMessage(MSG_DisconnectedRetry);
        NCDialogBox* Box = nw::Field<NCDialogBox*>(GodWnd, 0xFC);
        Box->SetInfo(Info);
        nw::Field<NCWnd*>(GodWnd, 0xFC)->ShowWindow();
        nw::Field<NCWnd*>(GodWnd, 0xFC)->SetFocus();
        Info->~NDialogInfo();
    }
}
NW_HOOK_METHOD(0x10076ae0, NConsoleWnd, ServerClose)

// 0x10065c00 - the console's own: the connection is gone (on the message line)
void NConsoleWnd::ReceiveServerClose()
{
    guard(NConsoleWnd::ReceiveServerClose);
    ShowMessageWnd(SysMsg(MSG_Disconnected));
    unguard;
}
NW_HOOK_METHOD(0x10065c00, NConsoleWnd, ReceiveServerClose)
