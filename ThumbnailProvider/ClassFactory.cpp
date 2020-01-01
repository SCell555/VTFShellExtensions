#define INITGUID
#include "Common.h"
#include "ClassFactory.h"

STDAPI CThumbnailProvider_CreateInstance( REFIID riid, void** ppvObject );

CClassFactory::CClassFactory()
{
    m_cRef = 1;
    DllAddRef();
}

CClassFactory::~CClassFactory()
{
    DllRelease();
}

STDMETHODIMP CClassFactory::QueryInterface( REFIID riid, void** ppvObject )
{
	static const QITAB qit[] =
	{
		QITABENT( CClassFactory, IClassFactory ),
		{ nullptr, 0 }
    };
	return QISearch( this, qit, riid, ppvObject );
}

STDMETHODIMP_(ULONG) CClassFactory::AddRef()
{
	const LONG cRef = InterlockedIncrement( &m_cRef );
    return cRef;
}

STDMETHODIMP_( ULONG ) CClassFactory::Release()
{
	const LONG cRef = InterlockedDecrement( &m_cRef );
	if ( 0 == cRef )
        delete this;
    return cRef;
}

STDMETHODIMP CClassFactory::CreateInstance( IUnknown* punkOuter, REFIID riid, void** ppvObject )
{
	if ( nullptr != punkOuter )
        return CLASS_E_NOAGGREGATION;

	return CThumbnailProvider_CreateInstance( riid, ppvObject );
}

STDMETHODIMP CClassFactory::LockServer( BOOL fLock )
{
    return E_NOTIMPL;
}

STDAPI DllGetClassObject( REFCLSID rclsid, REFIID riid, void** ppv )
{
	if ( nullptr == ppv )
        return E_INVALIDARG;

	if ( !IsEqualCLSID( CLSID_VTFThumbnailProvider, rclsid ) )
        return CLASS_E_CLASSNOTAVAILABLE;

	CClassFactory* pcf = new CClassFactory();
	if ( nullptr == pcf )
        return E_OUTOFMEMORY;

	const HRESULT hr = pcf->QueryInterface( riid, ppv );
    pcf->Release();
    return hr;
}