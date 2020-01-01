#pragma once

#include "vtffile.h"

class CThumbnailProvider : public IThumbnailProvider, IObjectWithSite, IInitializeWithStream
{
private:
    LONG m_cRef;
    IUnknown* m_pSite;
	CVTFFile m_texture;

    ~CThumbnailProvider();

public:
    CThumbnailProvider();

    //  IUnknown methods
	STDMETHOD( QueryInterface )( REFIID, void** ) override;
	STDMETHOD_( ULONG, AddRef )() override;
	STDMETHOD_( ULONG, Release )() override;

    //  IInitializeWithSteam methods
	STDMETHOD( Initialize )( IStream*, DWORD ) override;

    //  IThumbnailProvider methods
	STDMETHOD( GetThumbnail )( UINT, HBITMAP*, WTS_ALPHATYPE* ) override;

    //  IObjectWithSite methods
	STDMETHOD( GetSite )( REFIID, void** ) override;
	STDMETHOD( SetSite )( IUnknown* ) override;
};