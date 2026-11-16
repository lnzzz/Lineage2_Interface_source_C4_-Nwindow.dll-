/*=============================================================================
    NCustomSingleClient.cpp - L2 Ignition: one client per PC (custom, NW_CUSTOM builds only). See
    NCustomSingleClient.h.
=============================================================================*/
#include "nwindow/NCustomSingleClient.h"

// Global\: one name for every Windows session of the PC. Never closed: Windows releases it when the client ends
static const TCHAR* MutexName = TEXT("Global\\L2Ignition.SingleClient");

// l2.ini [URL] ServerAddr (the login server) is this PC: a test copy against a local server
static UBOOL IsLocalServer(const TCHAR* Addr)
{
    while (*Addr == ' ' || *Addr == '\t')
        Addr++;
    return !_wcsnicmp(Addr, TEXT("127."), 4) || !_wcsicmp(Addr, TEXT("localhost")) || !_wcsicmp(Addr, TEXT("::1"))
        || !_wcsicmp(Addr, TEXT("[::1]"));
}

static BOOL CALLBACK HideOwnWindow(HWND Wnd, LPARAM)
{
    DWORD Pid = 0;
    GetWindowThreadProcessId(Wnd, &Pid);
    if (Pid == GetCurrentProcessId() && IsWindowVisible(Wnd))
        ShowWindow(Wnd, SW_HIDE);
    return TRUE;
}

void NCustomSingleClient::Check()
{
    static UBOOL bChecked = 0;
    if (bChecked)
        return;
    bChecked = 1;

    TCHAR Addr[256] = { 0 };
    GConfig->GetString(TEXT("URL"), TEXT("ServerAddr"), Addr, ARRAY_COUNT(Addr), NULL);
    if (IsLocalServer(Addr))
    {
        GLog->Logf(TEXT("SingleClient: ServerAddr %s is this PC, no lock"), Addr);
        return;
    }

    HANDLE Mutex = CreateMutexW(NULL, FALSE, MutexName);
    DWORD Error = GetLastError();
    // taken by this user (ALREADY_EXISTS) or by another Windows user of the PC (ACCESS_DENIED: the other's DACL)
    UBOOL bTaken = Mutex ? Error == ERROR_ALREADY_EXISTS : Error == ERROR_ACCESS_DENIED;
    if (!bTaken)
    {
        GLog->Logf(TEXT("SingleClient: lock %s (ServerAddr %s, error %d)"), Mutex ? TEXT("taken") : TEXT("unavailable"),
                   Addr, Error);
        return;   // the handle stays open for the whole run; with no mutex at all (another error) nobody is blocked
    }

    GLog->Logf(TEXT("SingleClient: another client is open, closing"));
    EnumWindows(HideOwnWindow, 0);
    MessageBoxW(NULL,
                TEXT("Ya hay un cliente de L2 Ignition abierto en esta PC.\n\n")
                TEXT("Solo se puede jugar con un cliente a la vez."),
                TEXT("L2 Ignition"), MB_OK | MB_ICONWARNING | MB_TOPMOST | MB_SETFOREGROUND);
    // nothing of the run is worth keeping (Option.ini is written on a clean exit only) and the engine is half built
    TerminateProcess(GetCurrentProcess(), 0);
}
