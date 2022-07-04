#pragma once

#include "vtffile.h"
#include <propkeydef.h>
#include <propsys.h>
#include <ShObjIdl.h>

typedef struct tagPROPVARIANT PROPVARIANT;

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
	volatile LONG m_cRef;

	template<typename T, typename U>
	HRESULT StoreIntoCache( const T& value, HRESULT( *func )( U, PROPVARIANT* ), REFPROPERTYKEY key );
	IPropertyStoreCache* m_pCache;
};
