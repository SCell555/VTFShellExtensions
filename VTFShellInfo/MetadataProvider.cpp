#include "Common.h"
#include "MetadataProvider.h"
#include "PropVariantSafe.h"
#include <propkey.h>

constexpr PROPERTYKEY CreatePropertyKey( const GUID& fmtid, DWORD pid )
{
	return PROPERTYKEY{ fmtid, pid };
}

enum
{
	ImageDepth = 160,
	MipMapCount,
	FaceCount,
	FrameCount,
	Flags,
	FormatName,
	Version
};

// {64258FCA-9579-4F73-B280-C0F0BDB866B8}
static constexpr  GUID CLSID_VTFShellInfoProps = { 0x64258fca, 0x9579, 0x4f73, { 0xb2, 0x80, 0xc0, 0xf0, 0xbd, 0xb8, 0x66, 0xb8 } };

static constexpr PROPERTYKEY PKEY_VTF_ImageDepth = CreatePropertyKey( CLSID_VTFShellInfoProps, ImageDepth );
static constexpr PROPERTYKEY PKEY_VTF_MipMapCount = CreatePropertyKey( CLSID_VTFShellInfoProps, MipMapCount );
static constexpr PROPERTYKEY PKEY_VTF_FaceCount = CreatePropertyKey( CLSID_VTFShellInfoProps, FaceCount );
static constexpr PROPERTYKEY PKEY_VTF_FrameCount = CreatePropertyKey( CLSID_VTFShellInfoProps, FrameCount );
static constexpr PROPERTYKEY PKEY_VTF_Flags = CreatePropertyKey( CLSID_VTFShellInfoProps, Flags );
static constexpr PROPERTYKEY PKEY_VTF_FormatName = CreatePropertyKey( CLSID_VTFShellInfoProps, FormatName );
static constexpr PROPERTYKEY PKEY_VTF_Version = CreatePropertyKey( CLSID_VTFShellInfoProps, Version );

MetadataProvider::MetadataProvider()
{
	DllAddRef();
	m_cRef = 1;
	m_pCache = nullptr;
}

MetadataProvider::~MetadataProvider()
{
	if ( m_pCache )
	{
		m_pCache->Release();
		m_pCache = nullptr;
	}
	DllRelease();
}

STDMETHODIMP MetadataProvider::QueryInterface( REFIID riid, void** ppvObject )
{
	static const QITAB qit[] =
	{
		QITABENT( MetadataProvider, IPropertyStore ),
		QITABENT( MetadataProvider, IPropertyStoreCapabilities ),
		QITABENT( MetadataProvider, IInitializeWithStream ),
		{ nullptr, 0 }
	};
	return QISearch( this, qit, riid, ppvObject );
}

STDMETHODIMP_( ULONG ) MetadataProvider::AddRef()
{
	const LONG cRef = InterlockedIncrement( &m_cRef );
	return cRef;
}

STDMETHODIMP_( ULONG ) MetadataProvider::Release()
{
	const LONG cRef = InterlockedDecrement( &m_cRef );
	if ( 0 == cRef )
		delete this;
	return cRef;
}

template <typename T, typename U>
HRESULT MetadataProvider::StoreIntoCache( const T& value, HRESULT( *func )( U, PROPVARIANT* ), REFPROPERTYKEY key )
{
	PropVariantSafe variant;
	if ( const auto hr = func( value, &variant.Get() ); hr != S_OK )
		return hr;
	if ( const auto hr = m_pCache->SetValueAndState( key, &variant.Get(), PSC_NORMAL ); hr != S_OK )
		return hr;
	return S_OK;
}

HRESULT MetadataProvider::Initialize( IStream* pstream, DWORD grfMode )
{
	if ( m_pCache )
		return HRESULT_FROM_WIN32( ERROR_ALREADY_INITIALIZED );

	if ( grfMode & STGM_READWRITE )
		return STG_E_ACCESSDENIED;

	STATSTG stat;
	if ( pstream->Stat( &stat, STATFLAG_NONAME ) != S_OK )
		return S_FALSE;

	ULONG len;
	const vlUInt size = static_cast<vlUInt>( min( stat.cbSize.QuadPart, sizeof( SVTFHeader ) ) );
	byte* data = new byte[size];
	if ( pstream->Read( data, size, &len ) != S_OK )
	{
		delete[] data;
		return S_FALSE;
	}

	CVTFFile vtfFile;
	const bool succsess = vtfFile.Load( data, size, true );
	delete[] data;

	if ( !succsess )
		return S_FALSE;

	// Initialize cache
	if ( const auto hr = PSCreateMemoryPropertyStore( IID_PPV_ARGS( &m_pCache ) ); hr != S_OK )
		return hr;

	if ( const auto hr = StoreIntoCache( vtfFile.GetWidth(), InitPropVariantFromUInt32, PKEY_Image_HorizontalSize ); hr != S_OK )
		return hr;

	if ( const auto hr = StoreIntoCache( vtfFile.GetHeight(), InitPropVariantFromUInt32, PKEY_Image_VerticalSize ); hr != S_OK )
		return hr;

	if ( const auto hr = StoreIntoCache( vtfFile.GetDepth(), InitPropVariantFromUInt32, PKEY_VTF_ImageDepth ); hr != S_OK )
		return hr;

	if ( const auto hr = StoreIntoCache( vtfFile.GetMipmapCount(), InitPropVariantFromUInt32, PKEY_VTF_MipMapCount ); hr != S_OK )
		return hr;

	if ( const auto hr = StoreIntoCache( vtfFile.GetFaceCount(), InitPropVariantFromUInt32, PKEY_VTF_FaceCount ); hr != S_OK )
		return hr;

	if ( const auto hr = StoreIntoCache( vtfFile.GetFrameCount(), InitPropVariantFromUInt32, PKEY_VTF_FrameCount ); hr != S_OK )
		return hr;

	if ( const auto hr = StoreIntoCache( vtfFile.GetFlags(), InitPropVariantFromUInt32, PKEY_VTF_Flags ); hr != S_OK )
		return hr;

	const auto& fmtInfo = CVTFFile::GetImageFormatInfo( vtfFile.GetFormat() );
	if ( const auto hr = StoreIntoCache( fmtInfo.uiBitsPerPixel, InitPropVariantFromUInt32, PKEY_Image_BitDepth ); hr != S_OK )
		return hr;

	if ( const auto hr = StoreIntoCache( fmtInfo.lpName, InitPropVariantFromString, PKEY_VTF_FormatName ); hr != S_OK )
		return hr;

	auto& ver = vtfFile.GetHeader().Version;
	if ( const auto hr = StoreIntoCache( ver[0] + ver[1] / 10.0, InitPropVariantFromDouble, PKEY_VTF_Version); hr != S_OK)
		return hr;

	wchar_t buf[64];
	swprintf_s( buf, L"%dx%d", vtfFile.GetWidth(), vtfFile.GetHeight() );
	if ( const auto hr = StoreIntoCache( buf, InitPropVariantFromString, PKEY_Image_Dimensions ); hr != S_OK )
		return hr;

	return S_OK;
}

HRESULT MetadataProvider::GetCount( DWORD* cProps )
{
	return m_pCache->GetCount( cProps );
}

HRESULT MetadataProvider::GetAt( DWORD iProp, PROPERTYKEY* pkey )
{
	return m_pCache->GetAt( iProp, pkey );
}

HRESULT MetadataProvider::GetValue( REFPROPERTYKEY key, PROPVARIANT* pv )
{
	return m_pCache->GetValue( key, pv );
}

HRESULT MetadataProvider::SetValue( REFPROPERTYKEY key, REFPROPVARIANT propvar )
{
	return STG_E_ACCESSDENIED;
}

HRESULT MetadataProvider::Commit()
{
	return E_NOTIMPL;
}

HRESULT MetadataProvider::IsPropertyWritable( REFPROPERTYKEY key )
{
	return S_FALSE;
}

STDAPI MetadataProvider_CreateInstance( REFIID riid, void** ppvObject )
{
	*ppvObject = nullptr;

	MetadataProvider* ptp = new MetadataProvider();
	if ( !ptp )
		return E_OUTOFMEMORY;

	const HRESULT hr = ptp->QueryInterface( riid, ppvObject );
	ptp->Release();
	return hr;
}