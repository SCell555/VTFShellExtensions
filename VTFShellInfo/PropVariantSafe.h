#pragma once

#include <propvarutil.h>

#include <iostream>

class PropVariantSafe final
{
public:
	PropVariantSafe() { PropVariantInit( &m_Variant ); }
	PropVariantSafe( const PropVariantSafe& other ) { CopyUnsafe( other.m_Variant ); }
	PropVariantSafe( const PROPVARIANT& other ) { CopyUnsafe( other ); }
	PropVariantSafe( PropVariantSafe&& other )
	{
		m_Variant = other.m_Variant;
		PropVariantInit( &other.m_Variant );
	}
	~PropVariantSafe() { Clear(); }

	PropVariantSafe& operator=( const PropVariantSafe& other )
	{
		Clear();
		CopyUnsafe( other.m_Variant );
		return *this;
	}

	PropVariantSafe& operator=( const PROPVARIANT& other )
	{
		Clear();
		CopyUnsafe( other );
		return *this;
	}

	void Set( const PROPVARIANT& other )
	{
		Clear();
		CopyUnsafe( other );
	}

	void Clear()
	{
		if ( const auto hr = PropVariantClear( &m_Variant ); hr != S_OK )
			std::cerr << "Failed to PropVariantClear(): HRESULT = " << std::hex << hr << std::endl;
	}

	__forceinline PROPVARIANT& operator*() { return m_Variant; }
	__forceinline PROPVARIANT* operator->() { return &m_Variant; }

	[[nodiscard]] __forceinline PROPVARIANT& Get() { return m_Variant; }
	[[nodiscard]] __forceinline const PROPVARIANT& Get() const { return m_Variant; }

private:
	void CopyUnsafe( const PROPVARIANT& other )
	{
		if ( const auto hr = PropVariantCopy( &m_Variant, &other ); hr != S_OK )
			std::cerr << "Failed to PropVariantCopy(): HRESULT = " << std::hex << hr << std::endl;
	}

	PROPVARIANT m_Variant;
};