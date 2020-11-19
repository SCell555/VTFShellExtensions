#define INITGUID
#include "Common.h"

static LONG g_cRef = 0;

struct REGKEY_DELETEKEY
{
	HKEY hKey;
	LPCWSTR lpszSubKey;
};

struct REGKEY_SUBKEY_AND_VALUE
{
	HKEY hKey;
	LPCWSTR lpszSubKey;
	LPCWSTR lpszValue;
	DWORD dwType;
	DWORD_PTR dwData;
};

static HRESULT CreateRegistryKeys( REGKEY_SUBKEY_AND_VALUE* aKeys, ULONG cKeys );
static HRESULT DeleteRegistryKeys( REGKEY_DELETEKEY* aKeys, ULONG cKeys );

STDAPI DllCanUnloadNow()
{
	return g_cRef ? S_FALSE : S_OK;
}

STDAPI_( ULONG ) DllAddRef()
{
	LONG cRef = InterlockedIncrement( &g_cRef );
	return cRef;
}

STDAPI_( ULONG ) DllRelease()
{
	LONG cRef = InterlockedDecrement( &g_cRef );
	if ( 0 > cRef )
		cRef = 0;
	return cRef;
}

static void RemoveLastDir( wchar_t* path )
{
	// Last char
	const size_t len = wcslen( path );
	const wchar_t* src = path + ( len ? len - 1 : 0 );

	// back up until a \ or the start
	while ( src != path && *( src - 1 ) != L'\\' )
		src--;

	path[src - path] = 0;
}

EXTERN_C IMAGE_DOS_HEADER __ImageBase;
STDAPI DllRegisterServer()
{
	WCHAR szModule[MAX_PATH];

	ZeroMemory( szModule, sizeof( szModule ) );
	GetModuleFileName( reinterpret_cast<HINSTANCE>( &__ImageBase ), szModule, ARRAYSIZE( szModule ) );

	//uncomment the following
	REGKEY_SUBKEY_AND_VALUE keys[] = {
		{ HKEY_CLASSES_ROOT, L"CLSID\\" szCLSID_VTFShellInfo, nullptr, REG_SZ, reinterpret_cast<DWORD_PTR>( L"VTF Shell Info Provider" ) },
		{ HKEY_CLASSES_ROOT, L"CLSID\\" szCLSID_VTFShellInfo L"\\InprocServer32", nullptr, REG_SZ, reinterpret_cast<DWORD_PTR>( szModule ) },
		{ HKEY_CLASSES_ROOT, L"CLSID\\" szCLSID_VTFShellInfo L"\\InprocServer32", L"ThreadingModel", REG_SZ, reinterpret_cast<DWORD_PTR>( L"Apartment" ) },
		{ HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\PropertySystem\\PropertyHandlers\\.vtf", nullptr, REG_SZ, reinterpret_cast<DWORD_PTR>( szCLSID_VTFShellInfo ) },
		{ HKEY_CLASSES_ROOT, L"SystemFileAssociations\\.vtf", L"ImageOptionFlags", REG_DWORD, 1 },
		{ HKEY_CLASSES_ROOT, L"SystemFileAssociations\\.vtf", L"ExtendedTileInfo", REG_SZ, reinterpret_cast<DWORD_PTR>( L"prop:System.ItemType;VTFShellInfo.FormatName;*System.Image.Dimensions" ) },
		{ HKEY_CLASSES_ROOT, L"SystemFileAssociations\\.vtf", L"FullDetails", REG_SZ, reinterpret_cast<DWORD_PTR>( L"prop:System.Image.HorizontalSize;System.Image.VerticalSize;VTFShellInfo.ImageDepth;VTFShellInfo.MipMapCount;VTFShellInfo.FaceCount;VTFShellInfo.FrameCount;VTFShellInfo.Flags;System.Image.BitDepth;VTFShellInfo.FormatName;System.Image.Dimensions;System.PropGroup.FileSystem;System.ItemNameDisplay;System.ItemType;System.ItemFolderPathDisplay;System.Size;System.DateCreated;System.DateModified;System.FileAttributes;*System.OfflineAvailability;*System.OfflineStatus;*System.SharedWith;*System.FileOwner;*System.ComputerName" ) },
		{ HKEY_CLASSES_ROOT, L"SystemFileAssociations\\.vtf", L"InfoTip", REG_SZ, reinterpret_cast<DWORD_PTR>( L"prop:System.ItemType;VTFShellInfo.FormatName;*System.Image.Dimensions;*System.Size" ) },
		{ HKEY_CLASSES_ROOT, L"SystemFileAssociations\\.vtf", L"PreviewDetails", REG_SZ, reinterpret_cast<DWORD_PTR>( L"prop:VTFShellInfo.ImageDepth;VTFShellInfo.MipMapCount;VTFShellInfo.FaceCount;VTFShellInfo.FrameCount;VTFShellInfo.Flags;System.Image.BitDepth;VTFShellInfo.FormatName;*System.Image.Dimensions;*System.Size;*System.OfflineAvailability;*System.OfflineStatus;*System.DateCreated;*System.SharedWith" ) }
	};
	if ( const auto hr = CreateRegistryKeys( keys, ARRAYSIZE( keys ) ); hr != S_OK )
		return hr;
	RemoveLastDir( szModule );
	wcsncat_s( szModule, L"VTFShellInfo.propdesc", MAX_PATH );
	if ( const auto hr = PSRegisterPropertySchema( szModule ); hr != S_OK )
		return hr;

	SHChangeNotify( SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr );
	return S_OK;
}

STDAPI DllUnregisterServer()
{
	WCHAR szModule[MAX_PATH];
	ZeroMemory( szModule, sizeof( szModule ) );
	GetModuleFileName( reinterpret_cast<HINSTANCE>( &__ImageBase ), szModule, ARRAYSIZE( szModule ) );
	RemoveLastDir( szModule );
	wcsncat_s( szModule, L"VTFShellInfo.propdesc", MAX_PATH );

	if ( const auto hr = PSUnregisterPropertySchema( szModule ); hr != S_OK )
		return hr;

	REGKEY_DELETEKEY keys[] = {
		{ HKEY_CLASSES_ROOT, L"CLSID\\" szCLSID_VTFShellInfo L"\\InprocServer32" },
		{ HKEY_CLASSES_ROOT, L"CLSID\\" szCLSID_VTFShellInfo },
		{ HKEY_CLASSES_ROOT, L"SystemFileAssociations\\.vtf" },
		{ HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\PropertySystem\\PropertyHandlers\\.vtf" }
	};
	const auto result = DeleteRegistryKeys( keys, ARRAYSIZE( keys ) );
	SHChangeNotify( SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr );
	return result;
}

static HRESULT CreateRegistryKey( REGKEY_SUBKEY_AND_VALUE* pKey )
{
	size_t cbData;
	LPVOID pvData = nullptr;
	HRESULT hr = S_OK;

	switch ( pKey->dwType )
	{
	case REG_DWORD:
		pvData = reinterpret_cast<LPDWORD>( &pKey->dwData );
		cbData = sizeof( DWORD );
		break;

	case REG_SZ:
	case REG_EXPAND_SZ:
		hr = StringCbLength( reinterpret_cast<LPCWSTR>( pKey->dwData ), STRSAFE_MAX_CCH, &cbData );
		if ( SUCCEEDED( hr ) )
		{
			pvData = const_cast<LPWSTR>( reinterpret_cast<LPCWSTR>( pKey->dwData ) );
			cbData += sizeof( WCHAR );
		}
		break;

	default:
		hr = E_INVALIDARG;
	}

	if ( SUCCEEDED( hr ) )
	{
		const LSTATUS status = SHSetValue( pKey->hKey, pKey->lpszSubKey, pKey->lpszValue, pKey->dwType, pvData, static_cast<DWORD>( cbData ) );
		if ( NOERROR != status )
			hr = HRESULT_FROM_WIN32( status );
	}

	return hr;
}

static HRESULT CreateRegistryKeys( REGKEY_SUBKEY_AND_VALUE* aKeys, ULONG cKeys )
{
	HRESULT hr = S_OK;

	for ( ULONG iKey = 0; iKey < cKeys; iKey++ )
	{
		const HRESULT hrTemp = CreateRegistryKey( &aKeys[iKey] );
		if ( FAILED( hrTemp ) )
			hr = hrTemp;
	}
	return hr;
}


static HRESULT DeleteRegistryKeys( REGKEY_DELETEKEY* aKeys, ULONG cKeys )
{
	HRESULT hr = S_OK;

	for ( ULONG iKey = 0; iKey < cKeys; iKey++ )
	{
		const LSTATUS status = RegDeleteTree( aKeys[iKey].hKey, aKeys[iKey].lpszSubKey );
		if ( NOERROR != status )
			hr = HRESULT_FROM_WIN32( status );
	}
	return hr;
}