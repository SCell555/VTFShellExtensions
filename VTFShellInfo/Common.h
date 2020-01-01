#pragma once

#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <thumbcache.h>
#include <strsafe.h>

STDAPI_( ULONG ) DllAddRef();
STDAPI_( ULONG ) DllRelease();

// {202C4699-A5D9-40EF-ACEC-F1A7455F935C}
#define szCLSID_VTFShellInfo L"{E7AABB8F-BDBD-4BF6-9E0E-6D5BC4AC5FC9}"
static inline constexpr const GUID CLSID_VTFShellInfo = { 0xe7aabb8f, 0xbdbd, 0x4bf6, { 0x9e, 0xe, 0x6d, 0x5b, 0xc4, 0xac, 0x5f, 0xc9 } };