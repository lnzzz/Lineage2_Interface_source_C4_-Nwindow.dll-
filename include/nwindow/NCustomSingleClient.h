#pragma once
// NCustomSingleClient - L2 Ignition: one client per PC (custom, NW_CUSTOM builds only; not part of the stock image).
//
// The first client takes a named mutex (Global\, so a second Windows session on the same PC counts too) and keeps it
// until the process ends. A second client finds it taken, says so in a message box and closes before the login screen.
// Clients whose l2.ini points to this PC (ServerAddr 127.x / localhost / ::1) skip the lock: test copies against a
// local server, which may run several at once and next to a client of the real server.
//
// Only a speed bump: a player can close the mutex handle or use the stock NWindow.dll. The server side check is the
// HWID one (docs of the server repo).

#include "nwindow/NWindow.h"

namespace NCustomSingleClient
{
    void Check();   // NConsoleWnd's constructor (the engine loads NWindow.dll for it in UGameEngine::Init); once
}
