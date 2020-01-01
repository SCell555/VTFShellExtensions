#pragma once

#include "vtffile.h"
#include <propkeydef.h>
#include <propsys.h>
#include <ShObjIdl.h>

class MetadataProvider : public IPropertyStore, IPropertyStoreCapabilities, IInitializeWithStream
{
	~MetadataProvider();

public:
	MetadataProvider();

	//  IUnknown methods
	STDMETHOD( QueryInterface )( REFIID, void** );
	STDMETHOD_( ULONG, AddRef )();
	STDMETHOD_( ULONG, Release )();

	STDMETHOD( Initialize )( IStream* pstream, DWORD grfMode ) override;
	STDMETHOD( GetCount )( DWORD* cProps ) override;
	STDMETHOD( GetAt )( DWORD iProp, PROPERTYKEY* pkey ) override;
	STDMETHOD( GetValue )( REFPROPERTYKEY key, PROPVARIANT* pv ) override;
	STDMETHOD( SetValue )( REFPROPERTYKEY key, REFPROPVARIANT propvar ) override;
	STDMETHOD( Commit )() override;
	STDMETHOD( IsPropertyWritable )( REFPROPERTYKEY key ) override;

private:
	template<typename T, typename InitFunc>
	HRESULT StoreIntoCache( const T& value, InitFunc&& func, REFPROPERTYKEY key );
	IPropertyStoreCache* m_pCache;
	LONG m_cRef;
};
