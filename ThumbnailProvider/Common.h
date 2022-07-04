#pragma once

#define GDIPVER     0x0110

#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <thumbcache.h>
#include <strsafe.h>

STDAPI_(ULONG) DllAddRef();
STDAPI_(ULONG) DllRelease();

// {202C4699-A5D9-40EF-ACEC-F1A7455F935C}
#define szCLSID_VTFThumbnailProvider L"{202C4699-A5D9-40EF-ACEC-F1A7455F935C}"
static inline constexpr const GUID CLSID_VTFThumbnailProvider = { 0x202c4699, 0xa5d9, 0x40ef, { 0xac, 0xec, 0xf1, 0xa7, 0x45, 0x5f, 0x93, 0x5c } };