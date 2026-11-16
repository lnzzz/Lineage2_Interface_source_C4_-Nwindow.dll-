#pragma once
// NCustomHwid - L2 Ignition: tells the server which PC the client runs on (custom, NW_CUSTOM builds only; not part of the
// stock image).
//
// When the character list arrives (the game server's AUTHED state, before a character is chosen) the client sends the
// bypass "hwid 1 <smbios> <win> <disk>". Each field is the first 16 bytes of SHA-256("L2Ignition.hwid.1|<name>|<value>")
// in lowercase hex, or "-" when the PC gives nothing usable; the raw values never leave the PC:
//   smbios  "uuid=<the 16 raw bytes of the SMBIOS type 1 UUID, hex>" and/or "board=<type 2 serial number>", joined by ';'
//           (all-zero / all-FF / placeholder UUIDs and "To be filled by O.E.M."-like serials are left out)
//   win     "guid=<HKLM\SOFTWARE\Microsoft\Cryptography MachineGuid, lowercase>" (the 64-bit registry view)
//   disk    "disk=<serial number of the physical disk that holds Windows>"
// The server picks the PC's id from them (docs/hwid.md of the server repo).

#include "nwindow/NWindow.h"

namespace NCustomHwid
{
    void Send();   // NConsoleWnd::ReceiveCharacterInfo: every time the character list arrives (computed once)
}
