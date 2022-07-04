#include "Common.h"
#include "ThumbnailProvider.h"
#include "gdiplus.h"

using namespace Gdiplus;
CThumbnailProvider::CThumbnailProvider()
{
	DllAddRef();
	m_cRef = 1;
	m_pSite = nullptr;
}

CThumbnailProvider::~CThumbnailProvider()
{
	if ( m_pSite )
	{
		m_pSite->Release();
		m_pSite = nullptr;
	}
	DllRelease();
}

STDMETHODIMP CThumbnailProvider::QueryInterface( REFIID riid, void** ppvObject )
{
	static const QITAB qit[] =
	{
		QITABENT( CThumbnailProvider, IInitializeWithStream ),
		QITABENT( CThumbnailProvider, IThumbnailProvider ),
		QITABENT( CThumbnailProvider, IObjectWithSite ),
		{ nullptr, 0 }
	};
	return QISearch( this, qit, riid, ppvObject );
}

STDMETHODIMP_( ULONG ) CThumbnailProvider::AddRef()
{
	const LONG cRef = InterlockedIncrement( &m_cRef );
	return cRef;
}

STDMETHODIMP_( ULONG ) CThumbnailProvider::Release()
{
	const LONG cRef = InterlockedDecrement( &m_cRef );
	if ( 0 == cRef )
		delete this;
	return cRef;
}

STDMETHODIMP CThumbnailProvider::Initialize( IStream* pstm, DWORD grfMode )
{
	if ( m_texture.IsLoaded() )
		return HRESULT_FROM_WIN32( ERROR_ALREADY_INITIALIZED );

	if ( grfMode & STGM_READWRITE )
		return STG_E_ACCESSDENIED;

	STATSTG stat;
	if ( pstm->Stat( &stat, STATFLAG_NONAME ) != S_OK )
		return S_FALSE;

	ULONG len;
	const vlUInt size = static_cast<vlUInt>( stat.cbSize.QuadPart );
	byte* data = new byte[size];
	if ( pstm->Read( data, size, &len ) != S_OK )
	{
		delete[] data;
		return S_FALSE;
	}

	const bool succsess = m_texture.Load( data, size );
	delete[] data;
	return succsess ? S_OK : S_FALSE;
}

STDMETHODIMP CThumbnailProvider::GetThumbnail( UINT cx, HBITMAP* phbmp, WTS_ALPHATYPE* pdwAlpha )
{
	*phbmp = nullptr;
	*pdwAlpha = WTSAT_UNKNOWN;
	ULONG_PTR token;
	GdiplusStartupInput input;
	if ( GdiplusStartup( &token, &input, nullptr ) == Ok )
	{
		vlUInt32 compressedSize = 0;
		if ( vlUInt size; auto res = static_cast<byte*>( m_texture.GetResourceData( VTF_RSRC_AUX_COMPRESSION_INFO, size ) ) )
		{
			compressedSize = size > sizeof( AuxCompressionInfoHeader_t ) && reinterpret_cast<AuxCompressionInfoHeader_t*>( res )->m_CompressionLevel != 0 ? reinterpret_cast<AuxCompressionInfoEntry_t*>( res + m_texture.GetAuxInfoOffset( 0, 0, 0 ) )->m_CompressedSize : 0;
		}
		const vlUInt w = m_texture.GetWidth(), h = m_texture.GetHeight();
		byte* pConverted = new byte[CVTFFile::ComputeImageSize( w, h, 1, IMAGE_FORMAT_BGRA8888 )];
		CVTFFile::Convert( m_texture.GetData(), pConverted, w, h, m_texture.GetFormat(), IMAGE_FORMAT_BGRA8888, compressedSize );
		Bitmap* pBitmap = new Bitmap( w, h, w * 4, PixelFormat32bppARGB, pConverted ); // delete?
		if ( pBitmap )
		{
			Graphics xGraphics( pBitmap );
			const Color color( 255, 255, 255 );
			pBitmap->GetHBITMAP( color, phbmp );
			*pdwAlpha = WTSAT_ARGB;
		}
		delete pBitmap;
		delete[] pConverted;
	}
	GdiplusShutdown( token );
	if ( *phbmp != nullptr )
		return NOERROR;
	return E_NOTIMPL;

}

STDMETHODIMP CThumbnailProvider::GetSite( REFIID riid, void** ppvSite )
{
	if ( m_pSite )
		return m_pSite->QueryInterface( riid, ppvSite );
	return E_NOINTERFACE;
}

STDMETHODIMP CThumbnailProvider::SetSite( IUnknown* pUnkSite )
{
	if ( m_pSite )
	{
		m_pSite->Release();
		m_pSite = nullptr;
	}

	m_pSite = pUnkSite;
	if ( m_pSite )
		m_pSite->AddRef();
	return S_OK;
}

STDAPI CThumbnailProvider_CreateInstance( REFIID riid, void** ppvObject )
{
	*ppvObject = nullptr;

	CThumbnailProvider* ptp = new CThumbnailProvider();
	if ( !ptp )
		return E_OUTOFMEMORY;

	const HRESULT hr = ptp->QueryInterface( riid, ppvObject );
	ptp->Release();
	return hr;
}