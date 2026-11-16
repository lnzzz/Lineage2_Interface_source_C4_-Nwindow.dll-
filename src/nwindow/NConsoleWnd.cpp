/*=============================================================================
    NConsoleWnd.cpp - the in-game console (the NWindow root): its construction and vtable,
    and the system message lines. The requests to the server are in NConsoleWndNet.cpp, the
    data getters in NConsoleWndData.cpp, the engine's callbacks in NConsoleWndEvents.cpp /
    NConsoleWndItems.cpp / NConsoleWndLogin.cpp, the windows in NConsoleWndWindows.cpp, the
    frame and the input in NConsoleWndInput.cpp, the text services in NConsoleWndText.cpp
    and the key actions in NConsoleWndActions.cpp.
=============================================================================*/
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NCItemWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NDialogInfo.h"
#if NW_CUSTOM
#include "nwindow/NCustomSingleClient.h"
#endif

typedef TArray<NTimer> TTimerArray;
typedef TArray<NSysMsgParam> TSysMsgParamArray;
typedef TArray<NManorEntry> TManorArray;

#if NW_HOOK_MODE
// The C++ constructor only enters the stock one (hooked below: ConstructConsole). Nothing in hook mode runs it: the stock
// InternalConstructor builds the object, and a C++ constructor would store our vtable, whose slots 61.. still differ from the
// stock's (VT DIFF) - the reason for ConstructConsole writing the stock vtable by hand
NConsoleWnd::NConsoleWnd()
{
    ::nw::StockCallAt<void>(0x100824F0, this);
}
#else
// Standalone: the real constructor (the engine's InternalConstructor is `new ((EInternal*)X) NConsoleWnd`). The compiler runs
// UL2ConsoleWnd's constructor (the Engine's, imported), stores our two vtables (the console's at +0 and FExec's at +0x34) and
// constructs the arrays (the three UI mode lists, then the system message parameter, manor and timer arrays: the declaration
// order is the stock's); the body is the rest of the stock constructor 0x100824F0, ConstructConsole below, store by store. The
// fields the stock does not write (the other windows, the pawn viewer's, the petition windows, the GM change) stay as the
// object came out of the allocator: zero (StaticAllocateObject clears it)
NConsoleWnd::NConsoleWnd()
{
    bInitialized = 0;
    WndMain = NULL;
    LobbyWnd = NULL;
    Desktop = NULL;
    DebugWnd = NULL;
    UnusedWnd = NULL;
    Wnd3C08 = NULL;
    MouseWheelDelta = 0;
    DeltaTime = 0;
    Unknown357C = 1;
    bSuspended = 0;
    bWheelHandled = 0;
    KeyCrypt = NULL;
    Unknown3580 = 0;
    bCtrlDown = 0;
    TutorialEvents = 0;
    LoadingScreenID = 10001;
    bNoChatOnEnter = 0;
    Unknown39E4 = 0;
    UIMode = 0;
    for (INT i = 0; i < 3; i++)
        ModeWnds[i].Empty();
    WordWrapperWnd = NULL;
    SavedL2Mark = 0;
    bL2MarkSaved = 0;
#if NW_CUSTOM
    NCustomSingleClient::Check();   // custom: a second client on the PC closes here, before the login screen
#endif
}
#endif
NW_CHECK_VTABLE_FOR(NConsoleWnd, UObject, 0x101b2428, 448)

#if NW_HOOK_MODE
// The Engine's UL2ConsoleWnd::UL2ConsoleWnd, called through its import slot: the class is abstract here (the C3 virtuals nobody
// ported are pure), so no placement new can run it. The stock constructor calls the same import (C4: its slot is 0x1019D470)
extern "C" void* nwUL2ConsoleWndCtor;
#pragma comment(linker, "/alternatename:_nwUL2ConsoleWndCtor=__imp_??0UL2ConsoleWnd@@QAE@XZ")

// 0x100824f0 (thiscall, returns this; stock frame with an exception handler, 355 bytes in two pieces) - the engine's console made
// first (its import), then the two vtables (the stock ones: see the header; C4: the second is at +0x34), the fields of the console:
// the three lists of windows of the UI modes (one eh_vector_constructor_iterator in the stock), the pointers of the stages'
// windows, the system message parameter, manor and timer arrays, the numbers (the loading screen 10001, the unknown 1), and the
// three mode lists emptied. Every store is read off the C4 stock; the fields the stock does not write (the other windows, the pawn
// viewer's, the petition windows, the GM change) are left as the object came out of the allocator: zero
NConsoleWnd* NConsoleWnd::ConstructConsole()
{
    ((void(__fastcall*)(void*, void*))nwUL2ConsoleWndCtor)(this, NULL);                 // UL2ConsoleWnd::UL2ConsoleWnd
    *(void**)this = ::nw::Addr(0x101B2428);
    *(void**)((BYTE*)this + 0x34) = ::nw::Addr(0x101B2424);
    bInitialized = 0;
    for (INT i = 0; i < 3; i++)
        ::new ((void*)&ModeWnds[i]) NWndPtrArray;
    WndMain = NULL;
    LobbyWnd = NULL;
    Desktop = NULL;
    DebugWnd = NULL;
    UnusedWnd = NULL;
    Wnd3C08 = NULL;
    ::new ((void*)&SysMsgParams) TSysMsgParamArray;
    ::new ((void*)&ManorList) TManorArray;
    ::new ((void*)&Timers) TTimerArray;
    MouseWheelDelta = 0;
    DeltaTime = 0;
    Unknown357C = 1;
    bSuspended = 0;
    bWheelHandled = 0;
    KeyCrypt = NULL;
    Unknown3580 = 0;
    bCtrlDown = 0;
    TutorialEvents = 0;
    LoadingScreenID = 10001;
    bNoChatOnEnter = 0;
    Unknown39E4 = 0;
    UIMode = 0;
    for (INT i = 0; i < 3; i++)
        ModeWnds[i].Empty();
    WordWrapperWnd = NULL;
    SavedL2Mark = 0;
    bL2MarkSaved = 0;
    return this;
}
NW_HOOK_METHOD(0x100824f0, NConsoleWnd, ConstructConsole)

// 0x10082900 (thiscall; stock frame with an exception handler, 162 bytes) - the stock vtables first (ConditionalDestroy calls the
// object's Destroy through the vtable), ConditionalDestroy, the timer, manor and system message parameter arrays, the three mode
// lists (the last one first: eh_vector_destructor_iterator), then the engine's destructor (the non-virtual one, through its import)
void NConsoleWnd::DestroyConsole()
{
    *(void**)this = ::nw::Addr(0x101B2428);
    *(void**)((BYTE*)this + 0x34) = ::nw::Addr(0x101B2424);
    ConditionalDestroy();
    Timers.~TTimerArray();
    ManorList.~TManorArray();
    SysMsgParams.~TSysMsgParamArray();
    for (INT i = 2; i >= 0; i--)
        ModeWnds[i].~NWndPtrArray();
    this->UL2ConsoleWnd::~UL2ConsoleWnd();
}
NW_HOOK_METHOD(0x10082900, NConsoleWnd, DestroyConsole)
// Standalone: no ConstructConsole / DestroyConsole. The destructor is the real one NW_DECLARE_CLASS gives the class (virtual,
// `ConditionalDestroy()` first, as the stock's second line), followed by the compiler's own: the arrays in reverse order (the
// timers, the manors, the system message parameters, the three mode lists from the last) and the engine's UL2ConsoleWnd destructor
#endif

// the dialog box of a system message (NDialogInfo::ID): OK only
enum { DIALOG_SYSTEM_MESSAGE = 0x1F };

// 0x1006c310 - the interface sound of a name (name ours): none for no name or "None" (C2: the
// sounds are named in the systemmsg lines now, not numbered)
void NConsoleWnd::PlaySoundName(const TCHAR* Name)
{
    if (Name && *Name && appStrcmp(Name, TEXT("None")))
        Engine->OnInterfacePlaySound((TCHAR*)Name);
}
NW_HOOK_METHOD(0x1006c310, NConsoleWnd, PlaySoundName)

// 0x10074e60 - the interface sound of a number (name ours): the stock keeps the names in a table at 0x102054e0 (the
// callers push the number) and jumps to PlaySoundName. C3: 19 names (C2: 11; the first one changed, the new ones are
// the character, inventory, map and system windows' own open / close sounds)
void NConsoleWnd::PlayInterfaceSound(INT SoundID)
{
    static const TCHAR* const SoundNames[19] =
    {
        TEXT("InterfaceSound.click_01"), TEXT("itemsound.click_2"), TEXT("itemsound.click_failed"),
        TEXT("itemsound.pickup"), TEXT("itemsound.trash_basket"), TEXT("itemsound.window_open"),
        TEXT("itemsound.window_close"), TEXT("itemsound.quest_tutorial"),
        TEXT("itemSound.itemdrop_etc_paper"), TEXT("itemSound.cooltime_end"), TEXT("itemSound.petition"),
        TEXT("InterfaceSound.charstat_open_01"), TEXT("InterfaceSound.charstat_close_01"),
        TEXT("InterfaceSound.inventory_open_01"), TEXT("InterfaceSound.inventory_close_01"),
        TEXT("InterfaceSound.map_open_01"), TEXT("InterfaceSound.map_close_01"),
        TEXT("InterfaceSound.system_open_01"), TEXT("InterfaceSound.system_close_01")
    };
    PlaySoundName(SoundNames[SoundID]);
}
NW_HOOK_METHOD(0x10074e60, NConsoleWnd, PlayInterfaceSound)

// 0x1006c350 - the voice of a system message (name ours; new in C3): the interface sound's sibling, through the
// engine's OnInterfacePlayVoice (slot 329): none for no name or "None"
void NConsoleWnd::PlayVoiceName(const TCHAR* Name)
{
    if (Name && *Name && appStrcmp(Name, TEXT("None")))
        Engine->OnInterfacePlayVoice((TCHAR*)Name);
}
NW_HOOK_METHOD(0x1006c350, NConsoleWnd, PlayVoiceName)

// 0x1007d380 - a system message into the chat window (with the sound and the voice of its line), or in a dialog
// box for the ones of type 4. The line's validity is read before the id is checked. The color the caller passes
// is not read: C2 and C3 take it from the line (Msg.Unknown14). C3: the voice (an FString at +0x24 of the line,
// 12 bytes that include/ue2 declares as plain data) follows the sound; both are played through the global console
void NConsoleWnd::ShowSystemMessage(INT ID, DWORD Color)
{
    FL2SystemMsgData& Msg = GL2GameData.SystemMsg[ID];
    if (!Msg.Unknown0 || ID < 0 || ID >= 3000)
        return;
    if (Msg.Type == 4)
    {
        // the local dialog info, its destructor called out of line (0x10006E20) as the stock does: our compiler
        // would inline the ~FString of its six texts here and into the callers' verify
        BYTE InfoMem[sizeof(NDialogInfo)];
        NDialogInfo* Info = ::new ((void*)InfoMem) NDialogInfo;
        Info->ID = DIALOG_SYSTEM_MESSAGE;
        Info->bMainBox = 1;
        Info->bInput = 0;
        Info->Owner = NULL;
        Info->SetSystemMessage(ID);
        ShowDialog(Info);
        Info->~NDialogInfo();
    }
    else
    {
        GetDesktop()->AddSystemMessage(ID, Msg.Unknown14);
        GConsole->PlaySoundName(*Msg.Message2);
        GConsole->PlayVoiceName(**(FString*)((BYTE*)&Msg + 0x24));
    }
}
NW_HOOK_METHOD(0x1007d380, NConsoleWnd, ShowSystemMessage)
