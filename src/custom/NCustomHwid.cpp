/*=============================================================================
    NCustomHwid.cpp - L2 Ignition: tells the server which PC the client runs on (custom, NW_CUSTOM builds only). See
    NCustomHwid.h.
=============================================================================*/
#include "nwindow/NCustomHwid.h"
#include "nwindow/NConsoleWnd.h"
#include <winioctl.h>
#include <wincrypt.h>

static const char* HashPrefix = "L2Ignition.hwid.1|";

/*-----------------------------------------------------------------------------
    Small text helpers (narrow: SMBIOS and disk serials are bytes, the hash input is UTF-8).
-----------------------------------------------------------------------------*/

struct FHwidText
{
    char Data[512];
    INT Len;
    FHwidText() : Len(0) { Data[0] = 0; }
    void Add(const char* S, INT N = -1)
    {
        if (N < 0)
            N = lstrlenA(S);
        for (INT i = 0; i < N && Len < (INT)sizeof(Data) - 1; i++)
            Data[Len++] = S[i];
        Data[Len] = 0;
    }
    void AddHex(const BYTE* B, INT N)
    {
        static const char Digits[] = "0123456789abcdef";
        for (INT i = 0; i < N; i++)
        {
            char Pair[2] = { Digits[B[i] >> 4], Digits[B[i] & 15] };
            Add(Pair, 2);
        }
    }
};

// the text without the spaces around it (drivers pad serials with spaces)
static void Trimmed(const char* S, INT N, FHwidText& Out)
{
    while (N > 0 && (BYTE)*S <= ' ')
        S++, N--;
    while (N > 0 && (BYTE)S[N - 1] <= ' ')
        N--;
    Out.Add(S, N);
}

static UBOOL ContainsNoCase(const char* S, const char* Sub)
{
    INT SubLen = lstrlenA(Sub);
    for (; *S; S++)
    {
        INT i = 0;
        while (i < SubLen && S[i] && (S[i] | 0x20) == (Sub[i] | 0x20))
            i++;
        if (i == SubLen)
            return 1;
    }
    return 0;
}

// a serial number that names nothing: empty, too short, one repeated character or a vendor placeholder
static UBOOL IsJunkSerial(const char* S)
{
    INT N = lstrlenA(S);
    if (N < 4)
        return 1;
    UBOOL bSame = 1;
    for (INT i = 1; i < N; i++)
        if (S[i] != S[0] && S[i] != '-' && S[i] != '_' && S[i] != '.')
            bSame = 0;
    if (bSame)
        return 1;
    static const char* Placeholders[] = { "o.e.m", "to be filled", "default", "not specified", "not applicable",
                                          "not available", "serial", "none", "n/a", "unknown", "123456789", "xxxx" };
    for (INT i = 0; i < ARRAY_COUNT(Placeholders); i++)
        if (ContainsNoCase(S, Placeholders[i]))
            return 1;
    return 0;
}

/*-----------------------------------------------------------------------------
    The three components.
-----------------------------------------------------------------------------*/

// the n-th string (1-based) of an SMBIOS structure's string set, NULL for 0 or a missing one
static const char* SmbiosString(const BYTE* Strings, const BYTE* End, BYTE Index)
{
    if (!Index)
        return NULL;
    const char* S = (const char*)Strings;
    for (BYTE i = 1; (const BYTE*)S < End && *S; i++)
    {
        if (i == Index)
            return S;
        while ((const BYTE*)S < End && *S)
            S++;
        S++;
    }
    return NULL;
}

static void ReadSmbios(FHwidText& Out)
{
    UINT Size = GetSystemFirmwareTable('RSMB', 0, NULL, 0);
    if (Size < 8)
        return;
    BYTE* Raw = (BYTE*)HeapAlloc(GetProcessHeap(), 0, Size);
    if (!Raw)
        return;
    if (GetSystemFirmwareTable('RSMB', 0, Raw, Size) == Size)
    {
        // RawSMBIOSData: 4 bytes of versions, the table's DWORD length, then the table
        const BYTE* P = Raw + 8;
        const BYTE* End = P + *(DWORD*)(Raw + 4);
        if (End > Raw + Size)
            End = Raw + Size;
        const BYTE* Uuid = NULL;
        const char* Board = NULL;
        while (P + 4 <= End)
        {
            BYTE Type = P[0], Len = P[1];
            if (Len < 4 || P + Len > End)
                break;
            const BYTE* Strings = P + Len;
            const BYTE* Q = Strings;
            while (Q + 1 < End && (Q[0] || Q[1]))
                Q++;
            if (Type == 1 && Len >= 0x18)
                Uuid = P + 8;
            else if (Type == 2 && Len >= 8 && !Board)
                Board = SmbiosString(Strings, Q + 1, P[7]);
            else if (Type == 127)
                break;
            P = Q + 2;
        }
        if (Uuid)
        {
            // all 00, all FF and the 03000200-0400-0500-0006-000700080009 that many boards ship with name no board
            static const BYTE Placeholder[16] = { 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 0, 8, 0, 9 };
            UBOOL bZero = 1, bOnes = 1;
            for (INT i = 0; i < 16; i++)
            {
                bZero &= Uuid[i] == 0;
                bOnes &= Uuid[i] == 0xFF;
            }
            if (!bZero && !bOnes && memcmp(Uuid, Placeholder, 16))
            {
                Out.Add("uuid=");
                Out.AddHex(Uuid, 16);
            }
        }
        if (Board)
        {
            FHwidText Serial;
            Trimmed(Board, lstrlenA(Board), Serial);
            if (!IsJunkSerial(Serial.Data))
            {
                if (Out.Len)
                    Out.Add(";");
                Out.Add("board=");
                Out.Add(Serial.Data);
            }
        }
    }
    HeapFree(GetProcessHeap(), 0, Raw);
}

static void ReadMachineGuid(FHwidText& Out)
{
    HKEY Key;
    // the client is 32-bit: MachineGuid lives in the 64-bit view only
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Cryptography"), 0,
                      KEY_QUERY_VALUE | KEY_WOW64_64KEY, &Key) != ERROR_SUCCESS)
        return;
    WCHAR Guid[64] = { 0 };
    DWORD Bytes = sizeof(Guid) - sizeof(WCHAR), Type = 0;
    if (RegQueryValueExW(Key, TEXT("MachineGuid"), NULL, &Type, (BYTE*)Guid, &Bytes) == ERROR_SUCCESS && Type == REG_SZ)
    {
        char Narrow[64] = { 0 };
        INT N = 0;
        for (const WCHAR* S = Guid; *S && N < 63; S++)
            if (*S > ' ' && *S < 0x80)
                Narrow[N++] = (char)((*S >= 'A' && *S <= 'Z') ? *S + 32 : *S);
        Narrow[N] = 0;
        if (N >= 8)
        {
            Out.Add("guid=");
            Out.Add(Narrow);
        }
    }
    RegCloseKey(Key);
}

static void ReadDiskSerial(FHwidText& Out)
{
    // the physical disk that holds the Windows directory; no administrator rights needed (both requests are
    // FILE_ANY_ACCESS on handles opened with no access)
    WCHAR Sys[MAX_PATH];
    if (!GetSystemDirectoryW(Sys, MAX_PATH) || Sys[1] != ':')
        return;
    WCHAR Path[32];
    wsprintfW(Path, TEXT("\\\\.\\%c:"), Sys[0]);
    HANDLE Volume = CreateFileW(Path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (Volume == INVALID_HANDLE_VALUE)
        return;
    BYTE ExtentsBuf[sizeof(VOLUME_DISK_EXTENTS) + 8 * sizeof(DISK_EXTENT)];
    DWORD Got = 0;
    BOOL bOk = DeviceIoControl(Volume, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, ExtentsBuf, sizeof(ExtentsBuf),
                               &Got, NULL);
    CloseHandle(Volume);
    VOLUME_DISK_EXTENTS* Extents = (VOLUME_DISK_EXTENTS*)ExtentsBuf;
    if (!bOk || !Extents->NumberOfDiskExtents)
        return;

    wsprintfW(Path, TEXT("\\\\.\\PhysicalDrive%u"), Extents->Extents[0].DiskNumber);
    HANDLE Disk = CreateFileW(Path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (Disk == INVALID_HANDLE_VALUE)
        return;
    STORAGE_PROPERTY_QUERY Query;
    ZeroMemory(&Query, sizeof(Query));
    Query.PropertyId = StorageDeviceProperty;
    Query.QueryType = PropertyStandardQuery;
    BYTE Desc[1024];
    ZeroMemory(Desc, sizeof(Desc));
    Got = 0;
    if (DeviceIoControl(Disk, IOCTL_STORAGE_QUERY_PROPERTY, &Query, sizeof(Query), Desc, sizeof(Desc) - 1, &Got, NULL))
    {
        STORAGE_DEVICE_DESCRIPTOR* D = (STORAGE_DEVICE_DESCRIPTOR*)Desc;
        if (D->SerialNumberOffset && D->SerialNumberOffset < Got)
        {
            const char* S = (const char*)Desc + D->SerialNumberOffset;
            FHwidText Serial;
            Trimmed(S, lstrlenA(S), Serial);
            if (!IsJunkSerial(Serial.Data))
            {
                Out.Add("disk=");
                Out.Add(Serial.Data);
            }
        }
    }
    CloseHandle(Disk);
}

/*-----------------------------------------------------------------------------
    The hash and the bypass.
-----------------------------------------------------------------------------*/

// first 16 bytes of SHA-256(prefix + name + '|' + value) as 32 lowercase hex digits; "-" for no value or no hash
static void HashField(const char* Name, const FHwidText& Value, char* Out)
{
    lstrcpyA(Out, "-");
    if (!Value.Len)
        return;
    FHwidText Input;
    Input.Add(HashPrefix);
    Input.Add(Name);
    Input.Add("|");
    Input.Add(Value.Data, Value.Len);

    HCRYPTPROV Prov = 0;
    if (!CryptAcquireContextW(&Prov, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
        return;
    HCRYPTHASH Hash = 0;
    BYTE Digest[32];
    DWORD DigestLen = sizeof(Digest);
    if (CryptCreateHash(Prov, CALG_SHA_256, 0, 0, &Hash))
    {
        if (CryptHashData(Hash, (const BYTE*)Input.Data, Input.Len, 0)
            && CryptGetHashParam(Hash, HP_HASHVAL, Digest, &DigestLen, 0) && DigestLen == 32)
        {
            FHwidText Hex;
            Hex.AddHex(Digest, 16);
            lstrcpyA(Out, Hex.Data);
        }
        CryptDestroyHash(Hash);
    }
    CryptReleaseContext(Prov, 0);
}

void NCustomHwid::Send()
{
    static TCHAR Bypass[160] = { 0 };
    if (!Bypass[0])
    {
        FHwidText Smbios, Win, Disk;
        ReadSmbios(Smbios);
        ReadMachineGuid(Win);
        ReadDiskSerial(Disk);
        char H1[40], H2[40], H3[40];
        HashField("smbios", Smbios, H1);
        HashField("win", Win, H2);
        HashField("disk", Disk, H3);
        wsprintfW(Bypass, TEXT("hwid 1 %S %S %S"), H1, H2, H3);
    }
    if (!GConsole)
        return;
    L2ParamStack Params(10);
    Params.PushBack((void*)Bypass);
    GConsole->RequestBypassToServer(Params);
}
