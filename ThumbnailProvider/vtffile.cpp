#include "vtffile.h"
#include <cstring>
#include <cmath>

#ifndef SHELLINFO_EXPORTS
#undef next_in
#include "zlib.h"
#endif

enum CubeMapFaceIndex_t
{
	CUBEMAP_FACE_RIGHT = 0,
	CUBEMAP_FACE_LEFT,
	CUBEMAP_FACE_BACK,	// NOTE: This face is in the +y direction?!?!?
	CUBEMAP_FACE_FRONT,	// NOTE: This face is in the -y direction!?!?
	CUBEMAP_FACE_UP,
	CUBEMAP_FACE_DOWN,

	// This is the fallback for low-end
	CUBEMAP_FACE_SPHEREMAP,

	// NOTE: Cubemaps have *7* faces; the 7th is the fallback spheremap
	CUBEMAP_FACE_COUNT
};

#define VTF_MAJOR_VERSION 7
#define VTF_MINOR_VERSION 6
#define VTF_MINOR_VERSION_MIN_SPHERE_MAP	1
#define VTF_MINOR_VERSION_MIN_VOLUME		2
#define VTF_MINOR_VERSION_MIN_RESOURCE		3
#define VTF_MINOR_VERSION_MIN_NO_SPHERE_MAP	5

#define FILE_BEGIN 0
#define FILE_END -1
namespace IO
{
	namespace Readers
	{
		class IReader
		{
		public:
			virtual vlBool Opened() const = 0;

			virtual vlBool Open() = 0;
			virtual vlVoid Close() = 0;

			virtual vlUInt GetStreamSize() const = 0;
			virtual vlUInt GetStreamPointer() const = 0;

			virtual vlUInt Seek( vlLong lOffset, vlUInt uiMode ) = 0;

			virtual vlBool Read( vlChar &cChar ) = 0;
			virtual vlUInt Read( vlVoid *vData, vlUInt uiBytes ) = 0;
		};
		class CMemoryReader : public IReader
		{
		private:
			vlBool bOpened;

			const vlVoid *vData;
			vlUInt uiBufferSize;

			vlUInt uiPointer;

		public:
			CMemoryReader( const vlVoid *vData, vlUInt uiBufferSize )
			{
				this->bOpened = vlFalse;

				this->vData = vData;
				this->uiBufferSize = uiBufferSize;
			}
			~CMemoryReader()
			{
			}

		public:
			virtual vlBool Opened() const
			{
				return this->bOpened;
			}

			virtual vlBool Open()
			{
				if ( vData == 0 )
				{
					return vlFalse;
				}

				this->uiPointer = 0;

				this->bOpened = vlTrue;

				return vlTrue;
			}
			virtual vlVoid Close()
			{
				this->bOpened = vlFalse;
			}

			virtual vlUInt GetStreamSize() const
			{
				if ( !this->bOpened )
				{
					return 0;
				}

				return this->uiBufferSize;
			}
			virtual vlUInt GetStreamPointer() const
			{
				if ( !this->bOpened )
				{
					return 0;
				}

				return this->uiPointer;
			}

			virtual vlUInt Seek( vlLong lOffset, vlUInt uiMode )
			{
				if ( !this->bOpened )
				{
					return 0;
				}

				switch ( uiMode )
				{
				case 0:
					this->uiPointer = 0;
					break;
				case 1:

					break;
				case -1:
					this->uiPointer = this->uiBufferSize;
					break;
				}

				vlLong lPointer = ( vlLong )this->uiPointer + lOffset;

				if ( lPointer < 0 )
				{
					lPointer = 0;
				}

				if ( lPointer >( vlLong )this->uiBufferSize )
				{
					lPointer = ( vlLong )this->uiBufferSize;
				}

				this->uiPointer = ( vlUInt )lPointer;

				return this->uiPointer;
			}

			vlBool Read( vlChar &cChar )
			{
				if ( !this->bOpened )
				{
					return vlFalse;
				}

				if ( this->uiPointer == this->uiBufferSize )
				{
					return vlFalse;
				}
				else
				{
					cChar = *( ( vlChar * )this->vData + this->uiPointer++ );

					return vlTrue;
				}
			}

			virtual vlUInt Read( vlVoid *vData, vlUInt uiBytes )
			{
				if ( !this->bOpened )
				{
					return 0;
				}

				if ( this->uiPointer == this->uiBufferSize )
				{
					return 0;
				}
				else if ( this->uiPointer + uiBytes > this->uiBufferSize ) // This right?
				{
					uiBytes = this->uiBufferSize - this->uiPointer;

					memcpy( vData, ( vlByte * )this->vData + this->uiPointer, uiBytes );

					this->uiPointer = this->uiBufferSize;

					return uiBytes;
				}
				else
				{
					memcpy( vData, ( vlByte * )this->vData + this->uiPointer, uiBytes );

					this->uiPointer += uiBytes;

					return uiBytes;
				}
			}
		};
	}
}

CVTFFile::CVTFFile()
{
	this->Header = 0;

	this->uiImageBufferSize = 0;
	this->lpImageData = 0;

	this->uiThumbnailBufferSize = 0;
	this->lpThumbnailImageData = 0;
}

CVTFFile::~CVTFFile()
{
	this->Destroy();
}

vlVoid CVTFFile::Destroy()
{
	if ( this->Header != 0 )
	{
		for ( vlUInt i = 0; i < this->Header->ResourceCount; i++ )
		{
			delete[]this->Header->Data[i].Data;
		}
	}

	delete this->Header;
	this->Header = 0;

	this->uiImageBufferSize = 0;
	delete[]this->lpImageData;
	this->lpImageData = 0;

	this->uiThumbnailBufferSize = 0;
	delete[]this->lpThumbnailImageData;
	this->lpThumbnailImageData = 0;
}

vlBool CVTFFile::IsPowerOfTwo( vlUInt uiSize )
{
	return uiSize > 0 && ( uiSize & ( uiSize - 1 ) ) == 0;
}

vlUInt CVTFFile::NextPowerOfTwo( vlUInt uiSize )
{
	if ( uiSize == 0 )
	{
		return 1;
	}

	if ( IsPowerOfTwo( uiSize ) )
	{
		return uiSize;
	}

	uiSize--;
	for ( vlUInt i = 1; i <= sizeof( vlUInt ) * 4; i <<= 1 )
	{
		uiSize = uiSize | ( uiSize >> i );
	}
	uiSize++;

	return uiSize;
}

vlBool CVTFFile::IsLoaded() const
{
	return this->Header != 0;
}


vlBool CVTFFile::Load( const vlVoid *lpData, vlUInt uiBufferSize, vlBool bHeaderOnly )
{
	IO::Readers::CMemoryReader i = IO::Readers::CMemoryReader( lpData, uiBufferSize );
	return this->Load( &i, bHeaderOnly );
}

vlBool CVTFFile::Load( IO::Readers::IReader *Reader, vlBool bHeaderOnly )
{
	this->Destroy();

	try
	{
		if ( !Reader->Open() )
			throw 0;

		vlUInt uiFileSize = Reader->GetStreamSize();

		if ( uiFileSize < sizeof( SVTFFileHeader ) )
		{
			throw 0;
		}

		SVTFFileHeader FileHeader;

		memset( &FileHeader, 0, sizeof( SVTFFileHeader ) );
		if ( Reader->Read( &FileHeader, sizeof( SVTFFileHeader ) ) != sizeof( SVTFFileHeader ) )
		{
			throw 0;
		}

		if ( memcmp( FileHeader.TypeString, "VTF\0", 4 ) != 0 )
		{
			throw 0;
		}

		if ( FileHeader.Version[0] != VTF_MAJOR_VERSION || ( FileHeader.Version[1] < 0 || FileHeader.Version[1] > VTF_MINOR_VERSION ) )
		{
			throw 0;
		}

		if ( FileHeader.HeaderSize > sizeof( SVTFHeader ) )
		{
			throw 0;
		}

		Reader->Seek( 0, FILE_BEGIN );

		this->Header = new SVTFHeader;
		memset( this->Header, 0, sizeof( SVTFHeader ) );

		if ( Reader->Read( this->Header, FileHeader.HeaderSize ) != FileHeader.HeaderSize )
		{
			throw 0;
		}

		if ( this->Header->Version[0] < VTF_MAJOR_VERSION || ( this->Header->Version[0] == VTF_MAJOR_VERSION && this->Header->Version[1] < VTF_MINOR_VERSION_MIN_VOLUME ) )
		{
			this->Header->Depth = 1;
		}

		if ( !( this->Header->Version[0] > VTF_MAJOR_VERSION || ( this->Header->Version[0] == VTF_MAJOR_VERSION && this->Header->Version[1] >= VTF_MINOR_VERSION_MIN_RESOURCE ) ) )
		{
			this->Header->ResourceCount = 0;
		}

		if ( bHeaderOnly )
		{
			Reader->Close();
			return vlTrue;
		}

		this->uiImageBufferSize = CVTFFile::ComputeImageSize( this->Header->Width, this->Header->Height, this->Header->Depth, this->Header->MipCount, this->Header->ImageFormat ) * this->GetFaceCount() * this->GetFrameCount();

		if ( this->Header->LowResImageFormat != IMAGE_FORMAT_NONE )
		{
			this->uiThumbnailBufferSize = CVTFFile::ComputeImageSize( this->Header->LowResImageWidth, this->Header->LowResImageHeight, 1, this->Header->LowResImageFormat );
		}
		else
		{
			this->uiThumbnailBufferSize = 0;
		}

		vlUInt uiThumbnailBufferOffset = 0, uiImageDataOffset = 0, uiImageBufferSize = this->uiImageBufferSize;
		if ( this->Header->ResourceCount )
		{
			if ( this->Header->ResourceCount > VTF_RSRC_MAX_DICTIONARY_ENTRIES )
			{
				throw 0;
			}

			for ( vlUInt i = 0; i < this->Header->ResourceCount; i++ )
			{
				switch ( this->Header->Resources[i].Type )
				{
				case VTF_LEGACY_RSRC_LOW_RES_IMAGE:
					if ( this->Header->LowResImageFormat == IMAGE_FORMAT_NONE )
					{
						throw 0;
					}
					if ( uiThumbnailBufferOffset != 0 )
					{
						throw 0;
					}
					uiThumbnailBufferOffset = this->Header->Resources[i].Data;
					break;
				case VTF_LEGACY_RSRC_IMAGE:
					if ( uiImageDataOffset != 0 )
					{
						throw 0;
					}
					uiImageDataOffset = this->Header->Resources[i].Data;
					break;
				case VTF_RSRC_AUX_COMPRESSION_INFO:
				{
					if ( this->Header->Resources[i].Data + sizeof( vlUInt ) > uiFileSize )
					{
						throw 0;
					}

					vlUInt uiSize = 0;
					Reader->Seek( this->Header->Resources[i].Data, FILE_BEGIN );
					if ( Reader->Read( &uiSize, sizeof( vlUInt ) ) != sizeof( vlUInt ) )
					{
						throw 0;
					}

					if ( this->Header->Resources[i].Data + sizeof( vlUInt ) + uiSize > uiFileSize )
					{
						throw 0;
					}

					this->Header->Data[i].Size = uiSize;
					auto pCompressionInfo = this->Header->Data[i].Data = new vlByte[uiSize];
					if ( Reader->Read( this->Header->Data[i].Data, uiSize ) != uiSize )
					{
						throw 0;
					}

					vlUInt uiFrameCount = this->Header->Frames;
					vlUInt uiFaceCount = this->GetFaceCount();
					vlUInt uiSliceCount = this->Header->Depth;
					vlUInt uiMipCount = this->Header->MipCount;

					bool bIsAuxCompressed = false;
					if ( uiSize > sizeof( AuxCompressionInfoHeader_t ) )
						bIsAuxCompressed = ( (AuxCompressionInfoHeader_t *)pCompressionInfo )->m_CompressionLevel != 0;

					if ( bIsAuxCompressed )
					{
						uiImageBufferSize = 0;
						for ( vlInt iMip = uiMipCount - 1; iMip >= 0; --iMip )
						{
							for ( vlUInt iFrame = 0; iFrame < uiFrameCount; ++iFrame )
							{
								for ( vlUInt iFace = 0; iFace < uiFaceCount; ++iFace )
								{
									vlUInt infoOffset = GetAuxInfoOffset( iFrame, iFace, iMip );

									AuxCompressionInfoEntry_t *infoEntry = (AuxCompressionInfoEntry_t *)( pCompressionInfo + infoOffset );

									uiImageBufferSize += infoEntry->m_CompressedSize;
								}
							}
						}
					}

				}
					break;
				default:
					if ( ( this->Header->Resources[i].Flags & RSRCF_HAS_NO_DATA_CHUNK ) == 0 )
					{
						if ( this->Header->Resources[i].Data + sizeof( vlUInt ) > uiFileSize )
						{
							throw 0;
						}

						vlUInt uiSize = 0;
						Reader->Seek( this->Header->Resources[i].Data, FILE_BEGIN );
						if ( Reader->Read( &uiSize, sizeof( vlUInt ) ) != sizeof( vlUInt ) )
						{
							throw 0;
						}

						if ( this->Header->Resources[i].Data + sizeof( vlUInt ) + uiSize > uiFileSize )
						{
							throw 0;
						}

						this->Header->Data[i].Size = uiSize;
						this->Header->Data[i].Data = new vlByte[uiSize];
						if ( Reader->Read( this->Header->Data[i].Data, uiSize ) != uiSize )
						{
							throw 0;
						}
					}
					break;
				}
			}
		}
		else
		{
			uiThumbnailBufferOffset = this->Header->HeaderSize;
			uiImageDataOffset = uiThumbnailBufferOffset + this->uiThumbnailBufferSize;
		}

		if ( this->Header->HeaderSize > uiFileSize || uiThumbnailBufferOffset + this->uiThumbnailBufferSize > uiFileSize || uiImageDataOffset + uiImageBufferSize > uiFileSize )
		{
			throw 0;
		}

		if ( uiThumbnailBufferOffset == 0 )
		{
			this->Header->LowResImageFormat = IMAGE_FORMAT_NONE;
		}

		if ( this->Header->LowResImageFormat != IMAGE_FORMAT_NONE )
		{
			this->lpThumbnailImageData = new vlByte[this->uiThumbnailBufferSize];

			Reader->Seek( uiThumbnailBufferOffset, FILE_BEGIN );
			if ( Reader->Read( this->lpThumbnailImageData, this->uiThumbnailBufferSize ) != this->uiThumbnailBufferSize )
			{
				throw 0;
			}
		}

		if ( uiImageDataOffset == 0 )
		{
			this->Header->ImageFormat = IMAGE_FORMAT_NONE;
		}

		if ( this->Header->ImageFormat != IMAGE_FORMAT_NONE )
		{
			this->lpImageData = new vlByte[uiImageBufferSize];

			Reader->Seek( uiImageDataOffset, FILE_BEGIN );
			if ( Reader->Read( this->lpImageData, uiImageBufferSize ) != uiImageBufferSize )
			{
				throw 0;
			}
		}
	}
	catch ( ... )
	{
		Reader->Close();

		this->Destroy();

		return vlFalse;
	}

	Reader->Close();

	return vlTrue;
}

vlUInt CVTFFile::GetWidth() const
{
	if ( !this->IsLoaded() )
		return 0;

	return this->Header->Width;
}

vlUInt CVTFFile::GetHeight() const
{
	if ( !this->IsLoaded() )
		return 0;

	return this->Header->Height;
}

vlUInt CVTFFile::GetDepth() const
{
	if ( !this->IsLoaded() )
		return 0;

	return this->Header->Depth;
}

vlUInt CVTFFile::GetFrameCount() const
{
	if ( !this->IsLoaded() )
		return 0;

	return this->Header->Frames;
}

vlUInt CVTFFile::GetFlags() const
{
	if ( !this->IsLoaded() )
		return 0;

	return this->Header->Flags;
}

vlUInt CVTFFile::GetFaceCount() const
{
	if ( !this->IsLoaded() )
		return 0;

	return this->Header->Flags & TEXTUREFLAGS_ENVMAP ? ( this->Header->StartFrame != 0xffff && this->Header->Version[1] < VTF_MINOR_VERSION_MIN_NO_SPHERE_MAP ? CUBEMAP_FACE_COUNT : CUBEMAP_FACE_COUNT - 1 ) : 1;
}

vlUInt CVTFFile::GetMipmapCount() const
{
	if ( !this->IsLoaded() )
		return 0;

	return this->Header->MipCount;
}

vlByte *CVTFFile::GetData( vlUInt uiFrame, vlUInt uiFace, vlUInt uiSlice, vlUInt uiMipmapLevel ) const
{
	if ( !this->IsLoaded() )
		return 0;

	return this->lpImageData + this->ComputeDataOffset( uiFrame, uiFace, uiSlice, uiMipmapLevel, this->Header->ImageFormat );
}

vlVoid *CVTFFile::GetResourceData( vlUInt uiType, vlUInt &uiSize ) const
{
	if ( this->IsLoaded() )
	{
		if ( this->Header->Version[0] > VTF_MAJOR_VERSION || ( this->Header->Version[0] == VTF_MAJOR_VERSION && this->Header->Version[1] >= VTF_MINOR_VERSION_MIN_RESOURCE ) )
		{
			switch ( uiType )
			{
			case VTF_LEGACY_RSRC_LOW_RES_IMAGE:
				uiSize = this->uiThumbnailBufferSize;
				return this->lpThumbnailImageData;
				break;
			case VTF_LEGACY_RSRC_IMAGE:
				uiSize = this->uiImageBufferSize;
				return this->lpImageData;
				break;
			default:
				for ( vlUInt i = 0; i < this->Header->ResourceCount; i++ )
				{
					if ( this->Header->Resources[i].Type == uiType )
					{
						if ( this->Header->Resources[i].Flags & RSRCF_HAS_NO_DATA_CHUNK )
						{
							uiSize = sizeof( vlUInt );
							return &this->Header->Resources[i].Data;
						}
						else
						{
							uiSize = this->Header->Data[i].Size;
							return this->Header->Data[i].Data;
						}
					}
				}
				break;
			}
		}
	}

	uiSize = 0;
	return 0;
}

vlUInt CVTFFile::GetAuxInfoOffset( vlUInt iFrame, vlUInt iFace, vlUInt iMipLevel ) const
{
	vlUInt faceCount = GetFaceCount();
	return sizeof( AuxCompressionInfoHeader_t ) +
		( ( this->Header->MipCount - 1 - iMipLevel ) * this->Header->Frames * faceCount +
			iFrame * faceCount +
			iFace ) *
		sizeof( AuxCompressionInfoEntry_t );
}

const SVTFHeader& CVTFFile::GetHeader() const
{
	return *this->Header;
}

VTFImageFormat CVTFFile::GetFormat() const
{
	if ( !this->IsLoaded() )
		return IMAGE_FORMAT_NONE;

	return this->Header->ImageFormat;
}

static constexpr SVTFImageFormatInfo VTFImageFormatInfo[] =
{
	{ L"RGBA8888",			 32,  4,  8,  8,  8,  8, vlFalse,  vlTrue },		// IMAGE_FORMAT_RGBA8888,
	{ L"ABGR8888",			 32,  4,  8,  8,  8,  8, vlFalse,  vlTrue },		// IMAGE_FORMAT_ABGR8888,
	{ L"RGB888",			 24,  3,  8,  8,  8,  0, vlFalse,  vlTrue },		// IMAGE_FORMAT_RGB888,
	{ L"BGR888",			 24,  3,  8,  8,  8,  0, vlFalse,  vlTrue },		// IMAGE_FORMAT_BGR888,
	{ L"RGB565",			 16,  2,  5,  6,  5,  0, vlFalse,  vlTrue },		// IMAGE_FORMAT_RGB565,
	{ L"I8",				  8,  1,  0,  0,  0,  0, vlFalse,  vlTrue },		// IMAGE_FORMAT_I8,
	{ L"IA88",				 16,  2,  0,  0,  0,  8, vlFalse,  vlTrue },		// IMAGE_FORMAT_IA88
	{ L"P8",				  8,  1,  0,  0,  0,  0, vlFalse, vlFalse },		// IMAGE_FORMAT_P8
	{ L"A8",				  8,  1,  0,  0,  0,  8, vlFalse,  vlTrue },		// IMAGE_FORMAT_A8
	{ L"RGB888 Bluescreen",	 24,  3,  8,  8,  8,  0, vlFalse,  vlTrue },		// IMAGE_FORMAT_RGB888_BLUESCREEN
	{ L"BGR888 Bluescreen",	 24,  3,  8,  8,  8,  0, vlFalse,  vlTrue },		// IMAGE_FORMAT_BGR888_BLUESCREEN
	{ L"ARGB8888",			 32,  4,  8,  8,  8,  8, vlFalse,  vlTrue },		// IMAGE_FORMAT_ARGB8888
	{ L"BGRA8888",			 32,  4,  8,  8,  8,  8, vlFalse,  vlTrue },		// IMAGE_FORMAT_BGRA8888
	{ L"DXT1",				  4,  0,  0,  0,  0,  0,  vlTrue,  vlTrue },		// IMAGE_FORMAT_DXT1
	{ L"DXT3",				  8,  0,  0,  0,  0,  8,  vlTrue,  vlTrue },		// IMAGE_FORMAT_DXT3
	{ L"DXT5",				  8,  0,  0,  0,  0,  8,  vlTrue,  vlTrue },		// IMAGE_FORMAT_DXT5
	{ L"BGRX8888",			 32,  4,  8,  8,  8,  0, vlFalse,  vlTrue },		// IMAGE_FORMAT_BGRX8888
	{ L"BGR565",			 16,  2,  5,  6,  5,  0, vlFalse,  vlTrue },		// IMAGE_FORMAT_BGR565
	{ L"BGRX5551",			 16,  2,  5,  5,  5,  0, vlFalse,  vlTrue },		// IMAGE_FORMAT_BGRX5551
	{ L"BGRA4444",			 16,  2,  4,  4,  4,  4, vlFalse,  vlTrue },		// IMAGE_FORMAT_BGRA4444
	{ L"DXT1 One Bit Alpha",  4,  0,  0,  0,  0,  1,  vlTrue,  vlTrue },		// IMAGE_FORMAT_DXT1_ONEBITALPHA
	{ L"BGRA5551",			 16,  2,  5,  5,  5,  1, vlFalse,  vlTrue },		// IMAGE_FORMAT_BGRA5551
	{ L"UV88",				 16,  2,  8,  8,  0,  0, vlFalse,  vlTrue },		// IMAGE_FORMAT_UV88
	{ L"UVWQ8888",			 32,  4,  8,  8,  8,  8, vlFalse,  vlTrue },		// IMAGE_FORMAT_UVWQ8899
	{ L"RGBA16161616F",		 64,  8, 16, 16, 16, 16, vlFalse,  vlTrue },		// IMAGE_FORMAT_RGBA16161616F
	{ L"RGBA16161616",		 64,  8, 16, 16, 16, 16, vlFalse,  vlTrue },		// IMAGE_FORMAT_RGBA16161616
	{ L"UVLX8888",			 32,  4,  8,  8,  8,  8, vlFalse,  vlTrue },		// IMAGE_FORMAT_UVLX8888
	{ L"R32F",				 32,  4, 32,  0,  0,  0, vlFalse,  vlTrue },		// IMAGE_FORMAT_R32F
	{ L"RGB323232F",		 96, 12, 32, 32, 32,  0, vlFalse,  vlTrue },		// IMAGE_FORMAT_RGB323232F
	{ L"RGBA32323232F",		128, 16, 32, 32, 32, 32, vlFalse,  vlTrue },		// IMAGE_FORMAT_RGBA32323232F
	{ L"nVidia DST16",		 16,  2,  0,  0,  0,  0, vlFalse,  vlTrue },		// IMAGE_FORMAT_NV_DST16
	{ L"nVidia DST24",		 24,  3,  0,  0,  0,  0, vlFalse,  vlTrue },		// IMAGE_FORMAT_NV_DST24
	{ L"nVidia INTZ",		 32,  4,  0,  0,  0,  0, vlFalse,  vlTrue },		// IMAGE_FORMAT_NV_INTZ
	{ L"nVidia RAWZ",		 32,  4,  0,  0,  0,  0, vlFalse,  vlTrue },		// IMAGE_FORMAT_NV_RAWZ
	{ L"ATI DST16",			 16,  2,  0,  0,  0,  0, vlFalse,  vlTrue },		// IMAGE_FORMAT_ATI_DST16
	{ L"ATI DST24",			 24,  3,  0,  0,  0,  0, vlFalse,  vlTrue },		// IMAGE_FORMAT_ATI_DST24
	{ L"nVidia NULL",		 32,  4,  0,  0,  0,  0, vlFalse,  vlTrue },		// IMAGE_FORMAT_NV_NULL
	{ L"ATI1N",				  4,  0,  0,  0,  0,  0,  vlTrue,  vlTrue },		// IMAGE_FORMAT_ATI1N
	{ L"ATI2N",				  8,  0,  0,  0,  0,  0,  vlTrue,  vlTrue }			// IMAGE_FORMAT_ATI2N
};

SVTFImageFormatInfo const &CVTFFile::GetImageFormatInfo( VTFImageFormat ImageFormat )
{
	return VTFImageFormatInfo[ImageFormat];
}

vlUInt CVTFFile::ComputeImageSize( vlUInt uiWidth, vlUInt uiHeight, vlUInt uiDepth, VTFImageFormat ImageFormat )
{
	switch ( ImageFormat )
	{
	case IMAGE_FORMAT_DXT1:
	case IMAGE_FORMAT_DXT1_ONEBITALPHA:
		if ( uiWidth < 4 && uiWidth > 0 )
			uiWidth = 4;

		if ( uiHeight < 4 && uiHeight > 0 )
			uiHeight = 4;

		return ( ( uiWidth + 3 ) / 4 ) * ( ( uiHeight + 3 ) / 4 ) * 8 * uiDepth;
	case IMAGE_FORMAT_DXT3:
	case IMAGE_FORMAT_DXT5:
		if ( uiWidth < 4 && uiWidth > 0 )
			uiWidth = 4;

		if ( uiHeight < 4 && uiHeight > 0 )
			uiHeight = 4;

		return ( ( uiWidth + 3 ) / 4 ) * ( ( uiHeight + 3 ) / 4 ) * 16 * uiDepth;
	default:
		return uiWidth * uiHeight * uiDepth * CVTFFile::GetImageFormatInfo( ImageFormat ).uiBytesPerPixel;
	}
}

vlUInt CVTFFile::ComputeImageSize( vlUInt uiWidth, vlUInt uiHeight, vlUInt uiDepth, vlUInt uiMipmaps, VTFImageFormat ImageFormat )
{
	vlUInt uiImageSize = 0;

	for ( vlUInt i = 0; i < uiMipmaps; i++ )
	{
		uiImageSize += CVTFFile::ComputeImageSize( uiWidth, uiHeight, uiDepth, ImageFormat );

		uiWidth >>= 1;
		uiHeight >>= 1;
		uiDepth >>= 1;

		if ( uiWidth < 1 )
			uiWidth = 1;

		if ( uiHeight < 1 )
			uiHeight = 1;

		if ( uiDepth < 1 )
			uiDepth = 1;
	}

	return uiImageSize;
}

vlUInt CVTFFile::ComputeMipmapCount( vlUInt uiWidth, vlUInt uiHeight, vlUInt uiDepth )
{
	vlUInt uiCount = 0;

	while ( vlTrue )
	{
		uiCount++;

		uiWidth >>= 1;
		uiHeight >>= 1;
		uiDepth >>= 1;

		if ( uiWidth == 0 && uiHeight == 0 && uiDepth == 0 )
			break;
	}

	return uiCount;
}

vlVoid CVTFFile::ComputeMipmapDimensions( vlUInt uiWidth, vlUInt uiHeight, vlUInt uiDepth, vlUInt uiMipmapLevel, vlUInt &uiMipmapWidth, vlUInt &uiMipmapHeight, vlUInt &uiMipmapDepth )
{
	uiMipmapWidth = uiWidth >> uiMipmapLevel;
	uiMipmapHeight = uiHeight >> uiMipmapLevel;
	uiMipmapDepth = uiDepth >> uiMipmapLevel;

	if ( uiMipmapWidth < 1 )
		uiMipmapWidth = 1;

	if ( uiMipmapHeight < 1 )
		uiMipmapHeight = 1;

	if ( uiMipmapDepth < 1 )
		uiMipmapDepth = 1;
}

vlUInt CVTFFile::ComputeMipmapSize( vlUInt uiWidth, vlUInt uiHeight, vlUInt uiDepth, vlUInt uiMipmapLevel, VTFImageFormat ImageFormat )
{
	vlUInt uiMipmapWidth, uiMipmapHeight, uiMipmapDepth;
	CVTFFile::ComputeMipmapDimensions( uiWidth, uiHeight, uiDepth, uiMipmapLevel, uiMipmapWidth, uiMipmapHeight, uiMipmapDepth );

	return CVTFFile::ComputeImageSize( uiMipmapWidth, uiMipmapHeight, uiMipmapDepth, ImageFormat );
}

vlUInt CVTFFile::ComputeDataOffset( vlUInt uiFrame, vlUInt uiFace, vlUInt uiSlice, vlUInt uiMipLevel, VTFImageFormat ImageFormat ) const
{
	vlUInt uiOffset = 0;

	vlUInt uiFrameCount = this->GetFrameCount();
	vlUInt uiFaceCount = this->GetFaceCount();
	vlUInt uiSliceCount = this->GetDepth();
	vlUInt uiMipCount = this->GetMipmapCount();

	if ( uiFrame >= uiFrameCount )
	{
		uiFrame = uiFrameCount - 1;
	}

	if ( uiFace >= uiFaceCount )
	{
		uiFace = uiFaceCount - 1;
	}

	if ( uiSlice >= uiSliceCount )
	{
		uiSlice = uiSliceCount - 1;
	}

	if ( uiMipLevel >= uiMipCount )
	{
		uiMipLevel = uiMipCount - 1;
	}

	bool bIsAuxCompressed = false;
	vlUInt compressionInfoSize = 0;
	unsigned char *pCompressionInfo = (unsigned char *)GetResourceData( VTF_RSRC_AUX_COMPRESSION_INFO, compressionInfoSize );
	if ( pCompressionInfo != nullptr )
	{
		AuxCompressionInfoHeader_t *infoHeader = (AuxCompressionInfoHeader_t *)pCompressionInfo;

		if ( compressionInfoSize > sizeof( AuxCompressionInfoHeader_t ) )
			bIsAuxCompressed = infoHeader->m_CompressionLevel != 0;
	}

	if ( bIsAuxCompressed )
	{
		// For aux compression, we have to go through the compression info resource
		// to find the correct offset
		for ( vlInt iMip = uiMipCount - 1; iMip >= static_cast<vlInt>( uiMipLevel ); --iMip )
		{
			for ( vlUInt iFrame = 0; iFrame < uiFrameCount; ++iFrame )
			{
				for ( vlUInt iFace = 0; iFace < uiFaceCount; ++iFace )
				{
					vlUInt infoOffset = GetAuxInfoOffset( iFrame, iFace, iMip );

					AuxCompressionInfoEntry_t *infoEntry = (AuxCompressionInfoEntry_t *)( pCompressionInfo + infoOffset );

					if ( iMip == uiMipLevel && iFrame == uiFrame && iFace == uiFace )
						return uiOffset;

					uiOffset += infoEntry->m_CompressedSize;
				}
			}
		}

		return uiOffset;
	}

	for ( vlInt i = ( vlInt )uiMipCount - 1; i > ( vlInt )uiMipLevel; i-- )
	{
		uiOffset += this->ComputeMipmapSize( this->Header->Width, this->Header->Height, this->Header->Depth, i, ImageFormat ) * uiFrameCount * uiFaceCount;
	}

	vlUInt uiTemp1 = this->ComputeMipmapSize( this->Header->Width, this->Header->Height, this->Header->Depth, uiMipLevel, ImageFormat );
	vlUInt uiTemp2 = this->ComputeMipmapSize( this->Header->Width, this->Header->Height, 1, uiMipLevel, ImageFormat );

	uiOffset += uiTemp1 * uiFrame * uiFaceCount * uiSliceCount;
	uiOffset += uiTemp1 * uiFace * uiSliceCount;
	uiOffset += uiTemp2 * uiSlice;

	return uiOffset;
}

typedef struct Colour8888
{
	vlByte r;		// change the order of names to change the
	vlByte g;		// order of the output ARGB or BGRA, etc...
	vlByte b;		// Last one is MSB, 1st is LSB.
	vlByte a;
} Colour8888;

typedef struct Colour565
{
	vlUInt nBlue : 5;		// order of names changes
	vlUInt nGreen : 6;		// byte order of output to 32 bit
	vlUInt nRed : 5;
} Colour565;

typedef struct DXTAlphaBlockExplicit
{
	vlShort row[4];
} DXTAlphaBlockExplicit;

vlBool CVTFFile::DecompressDXT1( vlByte *src, vlByte *dst, vlUInt uiWidth, vlUInt uiHeight )
{
	vlUInt		x, y, i, j, k, Select;
	vlByte		*Temp;
	Colour565	*color_0, *color_1;
	Colour8888	colours[4], *col;
	vlUInt		bitmask, Offset;

	vlByte nBpp = 4;
	vlByte nBpc = 1;
	vlUInt iBps = nBpp * nBpc * uiWidth;

	Temp = src;

	for ( y = 0; y < uiHeight; y += 4 )
	{
		for ( x = 0; x < uiWidth; x += 4 )
		{
			color_0 = ( ( Colour565* )Temp );
			color_1 = ( ( Colour565* )( Temp + 2 ) );
			bitmask = ( ( vlUInt* )Temp )[1];
			Temp += 8;

			colours[0].r = color_0->nRed << 3;
			colours[0].g = color_0->nGreen << 2;
			colours[0].b = color_0->nBlue << 3;
			colours[0].a = 0xFF;

			colours[1].r = color_1->nRed << 3;
			colours[1].g = color_1->nGreen << 2;
			colours[1].b = color_1->nBlue << 3;
			colours[1].a = 0xFF;

			if ( *( ( vlUShort* )color_0 ) > *( ( vlUShort* )color_1 ) )
			{
				colours[2].b = ( 2 * colours[0].b + colours[1].b + 1 ) / 3;
				colours[2].g = ( 2 * colours[0].g + colours[1].g + 1 ) / 3;
				colours[2].r = ( 2 * colours[0].r + colours[1].r + 1 ) / 3;
				colours[2].a = 0xFF;

				colours[3].b = ( colours[0].b + 2 * colours[1].b + 1 ) / 3;
				colours[3].g = ( colours[0].g + 2 * colours[1].g + 1 ) / 3;
				colours[3].r = ( colours[0].r + 2 * colours[1].r + 1 ) / 3;
				colours[3].a = 0xFF;
			}
			else
			{
				colours[2].b = ( colours[0].b + colours[1].b ) / 2;
				colours[2].g = ( colours[0].g + colours[1].g ) / 2;
				colours[2].r = ( colours[0].r + colours[1].r ) / 2;
				colours[2].a = 0xFF;

				colours[3].b = ( colours[0].b + 2 * colours[1].b + 1 ) / 3;
				colours[3].g = ( colours[0].g + 2 * colours[1].g + 1 ) / 3;
				colours[3].r = ( colours[0].r + 2 * colours[1].r + 1 ) / 3;
				colours[3].a = 0x00;
			}

			for ( j = 0, k = 0; j < 4; j++ )
			{
				for ( i = 0; i < 4; i++, k++ )
				{
					Select = ( bitmask & ( 0x03 << k * 2 ) ) >> k * 2;
					col = &colours[Select];

					if ( ( ( x + i ) < uiWidth ) && ( ( y + j ) < uiHeight ) )
					{
						Offset = ( y + j ) * iBps + ( x + i ) * nBpp;
						dst[Offset + 0] = col->r;
						dst[Offset + 1] = col->g;
						dst[Offset + 2] = col->b;
						dst[Offset + 3] = col->a;
					}
				}
			}
		}
	}
	return vlTrue;
}

vlBool CVTFFile::DecompressDXT3( vlByte *src, vlByte *dst, vlUInt uiWidth, vlUInt uiHeight )
{
	vlUInt		x, y, i, j, k, Select;
	vlByte		*Temp;
	Colour565	*color_0, *color_1;
	Colour8888	colours[4], *col;
	vlUInt		bitmask, Offset;
	vlUShort	word;
	DXTAlphaBlockExplicit *alpha;

	vlByte nBpp = 4;
	vlByte nBpc = 1;
	vlUInt iBps = nBpp * nBpc * uiWidth;

	Temp = src;

	for ( y = 0; y < uiHeight; y += 4 )
	{
		for ( x = 0; x < uiWidth; x += 4 )
		{
			alpha = ( DXTAlphaBlockExplicit* )Temp;
			Temp += 8;
			color_0 = ( ( Colour565* )Temp );
			color_1 = ( ( Colour565* )( Temp + 2 ) );
			bitmask = ( ( vlUInt* )Temp )[1];
			Temp += 8;

			colours[0].r = color_0->nRed << 3;
			colours[0].g = color_0->nGreen << 2;
			colours[0].b = color_0->nBlue << 3;
			colours[0].a = 0xFF;

			colours[1].r = color_1->nRed << 3;
			colours[1].g = color_1->nGreen << 2;
			colours[1].b = color_1->nBlue << 3;
			colours[1].a = 0xFF;

			colours[2].b = ( 2 * colours[0].b + colours[1].b + 1 ) / 3;
			colours[2].g = ( 2 * colours[0].g + colours[1].g + 1 ) / 3;
			colours[2].r = ( 2 * colours[0].r + colours[1].r + 1 ) / 3;
			colours[2].a = 0xFF;

			colours[3].b = ( colours[0].b + 2 * colours[1].b + 1 ) / 3;
			colours[3].g = ( colours[0].g + 2 * colours[1].g + 1 ) / 3;
			colours[3].r = ( colours[0].r + 2 * colours[1].r + 1 ) / 3;
			colours[3].a = 0xFF;

			k = 0;
			for ( j = 0; j < 4; j++ )
			{
				for ( i = 0; i < 4; i++, k++ )
				{
					Select = ( bitmask & ( 0x03 << k * 2 ) ) >> k * 2;
					col = &colours[Select];

					if ( ( ( x + i ) < uiWidth ) && ( ( y + j ) < uiHeight ) )
					{
						Offset = ( y + j ) * iBps + ( x + i ) * nBpp;
						dst[Offset + 0] = col->r;
						dst[Offset + 1] = col->g;
						dst[Offset + 2] = col->b;
					}
				}
			}

			for ( j = 0; j < 4; j++ )
			{
				word = alpha->row[j];
				for ( i = 0; i < 4; i++ )
				{
					if ( ( ( x + i ) < uiWidth ) && ( ( y + j ) < uiHeight ) )
					{
						Offset = ( y + j ) * iBps + ( x + i ) * nBpp + 3;
						dst[Offset] = word & 0x0F;
						dst[Offset] = dst[Offset] | ( dst[Offset] << 4 );
					}

					word >>= 4;
				}
			}
		}
	}
	return vlTrue;
}

vlBool CVTFFile::DecompressDXT5( vlByte *src, vlByte *dst, vlUInt uiWidth, vlUInt uiHeight )
{
	vlUInt		x, y, i, j, k, Select;
	vlByte		*Temp;
	Colour565	*color_0, *color_1;
	Colour8888	colours[4], *col;
	vlUInt		bitmask, Offset;
	vlByte		alphas[8], *alphamask;
	vlUInt		bits;

	vlByte nBpp = 4;
	vlByte nBpc = 1;
	vlUInt iBps = nBpp * nBpc * uiWidth;

	Temp = src;

	for ( y = 0; y < uiHeight; y += 4 )
	{
		for ( x = 0; x < uiWidth; x += 4 )
		{
			alphas[0] = Temp[0];
			alphas[1] = Temp[1];
			alphamask = Temp + 2;
			Temp += 8;
			color_0 = ( ( Colour565* )Temp );
			color_1 = ( ( Colour565* )( Temp + 2 ) );
			bitmask = ( ( vlUInt* )Temp )[1];
			Temp += 8;

			colours[0].r = color_0->nRed << 3;
			colours[0].g = color_0->nGreen << 2;
			colours[0].b = color_0->nBlue << 3;
			colours[0].a = 0xFF;

			colours[1].r = color_1->nRed << 3;
			colours[1].g = color_1->nGreen << 2;
			colours[1].b = color_1->nBlue << 3;
			colours[1].a = 0xFF;

			colours[2].b = ( 2 * colours[0].b + colours[1].b + 1 ) / 3;
			colours[2].g = ( 2 * colours[0].g + colours[1].g + 1 ) / 3;
			colours[2].r = ( 2 * colours[0].r + colours[1].r + 1 ) / 3;
			colours[2].a = 0xFF;

			colours[3].b = ( colours[0].b + 2 * colours[1].b + 1 ) / 3;
			colours[3].g = ( colours[0].g + 2 * colours[1].g + 1 ) / 3;
			colours[3].r = ( colours[0].r + 2 * colours[1].r + 1 ) / 3;
			colours[3].a = 0xFF;

			k = 0;
			for ( j = 0; j < 4; j++ )
			{
				for ( i = 0; i < 4; i++, k++ )
				{
					Select = ( bitmask & ( 0x03 << k * 2 ) ) >> k * 2;
					col = &colours[Select];

					if ( ( ( x + i ) < uiWidth ) && ( ( y + j ) < uiHeight ) )
					{
						Offset = ( y + j ) * iBps + ( x + i ) * nBpp;
						dst[Offset + 0] = col->r;
						dst[Offset + 1] = col->g;
						dst[Offset + 2] = col->b;
					}
				}
			}

			if ( alphas[0] > alphas[1] )
			{
				alphas[2] = ( 6 * alphas[0] + 1 * alphas[1] + 3 ) / 7;
				alphas[3] = ( 5 * alphas[0] + 2 * alphas[1] + 3 ) / 7;
				alphas[4] = ( 4 * alphas[0] + 3 * alphas[1] + 3 ) / 7;
				alphas[5] = ( 3 * alphas[0] + 4 * alphas[1] + 3 ) / 7;
				alphas[6] = ( 2 * alphas[0] + 5 * alphas[1] + 3 ) / 7;
				alphas[7] = ( 1 * alphas[0] + 6 * alphas[1] + 3 ) / 7;
			}
			else
			{
				alphas[2] = ( 4 * alphas[0] + 1 * alphas[1] + 2 ) / 5;
				alphas[3] = ( 3 * alphas[0] + 2 * alphas[1] + 2 ) / 5;
				alphas[4] = ( 2 * alphas[0] + 3 * alphas[1] + 2 ) / 5;
				alphas[5] = ( 1 * alphas[0] + 4 * alphas[1] + 2 ) / 5;
				alphas[6] = 0x00;
				alphas[7] = 0xFF;
			}

			bits = *( ( int* )alphamask );
			for ( j = 0; j < 2; j++ )
			{
				for ( i = 0; i < 4; i++ )
				{
					if ( ( ( x + i ) < uiWidth ) && ( ( y + j ) < uiHeight ) )
					{
						Offset = ( y + j ) * iBps + ( x + i ) * nBpp + 3;
						dst[Offset] = alphas[bits & 0x07];
					}
					bits >>= 3;
				}
			}

			bits = *( ( int* )&alphamask[3] );
			for ( j = 2; j < 4; j++ )
			{
				for ( i = 0; i < 4; i++ )
				{
					if ( ( ( x + i ) < uiWidth ) && ( ( y + j ) < uiHeight ) )
					{
						Offset = ( y + j ) * iBps + ( x + i ) * nBpp + 3;
						dst[Offset] = alphas[bits & 0x07];
					}
					bits >>= 3;
				}
			}
		}
	}
	return vlTrue;
}

typedef struct tagSVTFImageConvertInfo
{
	vlUInt	uiBitsPerPixel;
	vlUInt	uiBytesPerPixel;
	vlUInt	uiRBitsPerPixel;
	vlUInt	uiGBitsPerPixel;
	vlUInt	uiBBitsPerPixel;
	vlUInt	uiABitsPerPixel;
	vlInt	iR;
	vlInt	iG;
	vlInt	iB;
	vlInt	iA;
	vlBool	bIsCompressed;
	vlBool	bIsSupported;
	VTFImageFormat Format;
} SVTFImageConvertInfo;

static SVTFImageConvertInfo VTFImageConvertInfo[] =
{
	{ 32,  4,  8,  8,  8,  8,	 0,	 1,	 2,	 3,	vlFalse,  vlTrue,	IMAGE_FORMAT_RGBA8888 },
	{ 32,  4,  8,  8,  8,  8,	 3,	 2,	 1,	 0, vlFalse,  vlTrue,	IMAGE_FORMAT_ABGR8888 },
	{ 24,  3,  8,  8,  8,  0,	 0,	 1,	 2,	-1, vlFalse,  vlTrue,	IMAGE_FORMAT_RGB888 },
	{ 24,  3,  8,  8,  8,  0,	 2,	 1,	 0,	-1, vlFalse,  vlTrue,	IMAGE_FORMAT_BGR888 },
	{ 16,  2,  5,  6,  5,  0,	 0,	 1,	 2,	-1, vlFalse,  vlTrue,	IMAGE_FORMAT_RGB565 },
	{ 8,  1,  8,  8,  8,  0,	 0,	-1,	-1,	-1, vlFalse,  vlTrue,	IMAGE_FORMAT_I8 },
	{ 16,  2,  8,  8,  8,  8,	 0,	-1,	-1,	 1, vlFalse,  vlTrue,	IMAGE_FORMAT_IA88 },
	{ 8,  1,  0,  0,  0,  0,	-1,	-1,	-1,	-1, vlFalse, vlFalse,	IMAGE_FORMAT_P8 },
	{ 8,  1,  0,  0,  0,  8,	-1,	-1,	-1,	 0, vlFalse,  vlTrue,	IMAGE_FORMAT_A8 },
	{ 24,  3,  8,  8,  8,  8,	 0,	 1,	 2,	-1, vlFalse,  vlTrue,	IMAGE_FORMAT_RGB888_BLUESCREEN },
	{ 24,  3,  8,  8,  8,  8,	 2,	 1,	 0,	-1, vlFalse,  vlTrue,	IMAGE_FORMAT_BGR888_BLUESCREEN },
	{ 32,  4,  8,  8,  8,  8,	 3,	 0,	 1,	 2, vlFalse,  vlTrue,	IMAGE_FORMAT_ARGB8888 },
	{ 32,  4,  8,  8,  8,  8,	 2,	 1,	 0,	 3, vlFalse,  vlTrue,	IMAGE_FORMAT_BGRA8888 },
	{ 4,  0,  0,  0,  0,  0,	-1,	-1,	-1,	-1,	 vlTrue,  vlTrue,	IMAGE_FORMAT_DXT1 },
	{ 8,  0,  0,  0,  0,  8,	-1,	-1,	-1,	-1,	 vlTrue,  vlTrue,	IMAGE_FORMAT_DXT3 },
	{ 8,  0,  0,  0,  0,  8,	-1,	-1,	-1,	-1,	 vlTrue,  vlTrue,	IMAGE_FORMAT_DXT5 },
	{ 32,  4,  8,  8,  8,  0,	 2,	 1,	 0,	-1, vlFalse,  vlTrue,	IMAGE_FORMAT_BGRX8888 },
	{ 16,  2,  5,  6,  5,  0,	 2,	 1,	 0,	-1, vlFalse,  vlTrue,	IMAGE_FORMAT_BGR565 },
	{ 16,  2,  5,  5,  5,  0,	 2,	 1,	 0,	-1, vlFalse,  vlTrue,	IMAGE_FORMAT_BGRX5551 },
	{ 16,  2,  4,  4,  4,  4,	 2,	 1,	 0,	 3, vlFalse,  vlTrue,	IMAGE_FORMAT_BGRA4444 },
	{ 4,  0,  0,  0,  0,  1,	-1,	-1,	-1,	-1,	 vlTrue,  vlTrue,	IMAGE_FORMAT_DXT1_ONEBITALPHA },
	{ 16,  2,  5,  5,  5,  1,	 2,	 1,	 0,	 3, vlFalse,  vlTrue,	IMAGE_FORMAT_BGRA5551 },
	{ 16,  2,  8,  8,  0,  0,	 0,	 1,	-1,	-1, vlFalse,  vlTrue,	IMAGE_FORMAT_UV88 },
	{ 32,  4,  8,  8,  8,  8,	 0,	 1,	 2,	 3, vlFalse,  vlTrue,	IMAGE_FORMAT_UVWQ8888 },
	{ 64,  8, 16, 16, 16, 16,	 0,	 1,	 2,	 3, vlFalse,  vlTrue,	IMAGE_FORMAT_RGBA16161616F },
	{ 64,  8, 16, 16, 16, 16,	 0,	 1,	 2,	 3, vlFalse,  vlTrue,	IMAGE_FORMAT_RGBA16161616 },
	{ 32,  4,  8,  8,  8,  8,	 0,	 1,	 2,	 3, vlFalse,  vlTrue,	IMAGE_FORMAT_UVLX8888 },
	{ 32,  4, 32,  0,  0,  0,	 0,	-1,	-1,	-1, vlFalse, vlFalse,	IMAGE_FORMAT_R32F },
	{ 96, 12, 32, 32, 32,  0,	 0,	 1,	 2,	-1, vlFalse, vlFalse,	IMAGE_FORMAT_RGB323232F },
	{ 128, 16, 32, 32, 32, 32,	 0,	 1,	 2,	 3, vlFalse, vlFalse,	IMAGE_FORMAT_RGBA32323232F },
	{ 16,  2, 16,  0,  0,  0,	 0,	-1,	-1,	-1, vlFalse,  vlTrue,	IMAGE_FORMAT_NV_DST16 },
	{ 24,  3, 24,  0,  0,  0,	 0,	-1,	-1,	-1, vlFalse,  vlTrue,	IMAGE_FORMAT_NV_DST24 },
	{ 32,  4,  0,  0,  0,  0,	-1,	-1,	-1,	-1, vlFalse, vlFalse,	IMAGE_FORMAT_NV_INTZ },
	{ 24,  3,  0,  0,  0,  0,	-1,	-1,	-1,	-1, vlFalse, vlFalse,	IMAGE_FORMAT_NV_RAWZ },
	{ 16,  2, 16,  0,  0,  0,	 0,	-1,	-1,	-1, vlFalse,  vlTrue,	IMAGE_FORMAT_ATI_DST16 },
	{ 24,  3, 24,  0,  0,  0,	 0,	-1,	-1,	-1, vlFalse,  vlTrue,	IMAGE_FORMAT_ATI_DST24 },
	{ 32,  4,  0,  0,  0,  0,	-1,	-1,	-1,	-1, vlFalse, vlFalse,	IMAGE_FORMAT_NV_NULL },
	{ 4,  0,  0,  0,  0,  0,	-1, -1, -1, -1,	 vlTrue, vlFalse,	IMAGE_FORMAT_ATI1N },
	{ 8,  0,  0,  0,  0,  0,	-1, -1, -1, -1,	 vlTrue, vlFalse,	IMAGE_FORMAT_ATI2N }
};

template<typename T>
vlVoid GetShiftAndMask( const SVTFImageConvertInfo& Info, T &uiRShift, T &uiGShift, T &uiBShift, T &uiAShift, T &uiRMask, T &uiGMask, T &uiBMask, T &uiAMask )
{
	if ( Info.iR >= 0 )
	{
		if ( Info.iG >= 0 && Info.iG < Info.iR )
			uiRShift += ( T )Info.uiGBitsPerPixel;

		if ( Info.iB >= 0 && Info.iB < Info.iR )
			uiRShift += ( T )Info.uiBBitsPerPixel;

		if ( Info.iA >= 0 && Info.iA < Info.iR )
			uiRShift += ( T )Info.uiABitsPerPixel;

		uiRMask = ( T )( ~0 ) >> ( T )( ( sizeof( T ) * 8 ) - Info.uiRBitsPerPixel ); // Mask is for down shifted values.
	}

	if ( Info.iG >= 0 )
	{
		if ( Info.iR >= 0 && Info.iR < Info.iG )
			uiGShift += ( T )Info.uiRBitsPerPixel;

		if ( Info.iB >= 0 && Info.iB < Info.iG )
			uiGShift += ( T )Info.uiBBitsPerPixel;

		if ( Info.iA >= 0 && Info.iA < Info.iG )
			uiGShift += ( T )Info.uiABitsPerPixel;

		uiGMask = ( T )( ~0 ) >> ( T )( ( sizeof( T ) * 8 ) - Info.uiGBitsPerPixel );
	}

	if ( Info.iB >= 0 )
	{
		if ( Info.iR >= 0 && Info.iR < Info.iB )
			uiBShift += ( T )Info.uiRBitsPerPixel;

		if ( Info.iG >= 0 && Info.iG < Info.iB )
			uiBShift += ( T )Info.uiGBitsPerPixel;

		if ( Info.iA >= 0 && Info.iA < Info.iB )
			uiBShift += ( T )Info.uiABitsPerPixel;

		uiBMask = ( T )( ~0 ) >> ( T )( ( sizeof( T ) * 8 ) - Info.uiBBitsPerPixel );
	}

	if ( Info.iA >= 0 )
	{
		if ( Info.iR >= 0 && Info.iR < Info.iA )
			uiAShift += ( T )Info.uiRBitsPerPixel;

		if ( Info.iG >= 0 && Info.iG < Info.iA )
			uiAShift += ( T )Info.uiGBitsPerPixel;

		if ( Info.iB >= 0 && Info.iB < Info.iA )
			uiAShift += ( T )Info.uiBBitsPerPixel;

		uiAMask = ( T )( ~0 ) >> ( T )( ( sizeof( T ) * 8 ) - Info.uiABitsPerPixel );
	}
}

template<typename T>
T Shrink( const T& S, const T& SourceBits, const T& DestBits )
{
	if ( SourceBits == 0 || DestBits == 0 )
		return 0;

	return S >> ( SourceBits - DestBits );
}

template<typename T>
T Expand( T S, T SourceBits, T DestBits )
{
	if ( SourceBits == 0 || DestBits == 0 )
		return 0;

	T D = 0;

	while ( DestBits >= SourceBits )
	{
		D <<= SourceBits;
		D |= S;
		DestBits -= SourceBits;
	}

	if ( DestBits )
	{
		S >>= SourceBits - DestBits;
		D <<= DestBits;
		D |= S;
	}

	return D;
}

template<typename T, typename U>
vlBool ConvertTemplated( vlByte *lpSource, vlByte *lpDest, vlUInt uiWidth, vlUInt uiHeight, const SVTFImageConvertInfo& SourceInfo, const SVTFImageConvertInfo& DestInfo )
{
	vlUInt16 uiSourceRShift = 0, uiSourceGShift = 0, uiSourceBShift = 0, uiSourceAShift = 0;
	vlUInt16 uiSourceRMask = 0, uiSourceGMask = 0, uiSourceBMask = 0, uiSourceAMask = 0;

	vlUInt16 uiDestRShift = 0, uiDestGShift = 0, uiDestBShift = 0, uiDestAShift = 0;
	vlUInt16 uiDestRMask = 0, uiDestGMask = 0, uiDestBMask = 0, uiDestAMask = 0;

	GetShiftAndMask<vlUInt16>( SourceInfo, uiSourceRShift, uiSourceGShift, uiSourceBShift, uiSourceAShift, uiSourceRMask, uiSourceGMask, uiSourceBMask, uiSourceAMask );
	GetShiftAndMask<vlUInt16>( DestInfo, uiDestRShift, uiDestGShift, uiDestBShift, uiDestAShift, uiDestRMask, uiDestGMask, uiDestBMask, uiDestAMask );

	vlByte *lpSourceEnd = lpSource + ( uiWidth * uiHeight * SourceInfo.uiBytesPerPixel );
	for ( ; lpSource < lpSourceEnd; lpSource += SourceInfo.uiBytesPerPixel, lpDest += DestInfo.uiBytesPerPixel )
	{
		vlUInt i;
		T Source = 0;
		for ( i = 0; i < SourceInfo.uiBytesPerPixel; i++ )
		{
			Source |= ( T )lpSource[i] << ( ( T )i * 8 );
		}

		vlUInt16 SR = 0, SG = 0, SB = 0, SA = ~0;
		vlUInt16 DR = 0, DG = 0, DB = 0, DA = ~0;

		if ( uiSourceRMask )
			SR = ( vlUInt16 )( Source >> ( T )uiSourceRShift ) & uiSourceRMask;

		if ( uiSourceGMask )
			SG = ( vlUInt16 )( Source >> ( T )uiSourceGShift ) & uiSourceGMask;

		if ( uiSourceBMask )
			SB = ( vlUInt16 )( Source >> ( T )uiSourceBShift ) & uiSourceBMask;

		if ( uiSourceAMask )
			SA = ( vlUInt16 )( Source >> ( T )uiSourceAShift ) & uiSourceAMask;

		if ( uiSourceRMask && uiDestRMask )
		{
			if ( DestInfo.uiRBitsPerPixel < SourceInfo.uiRBitsPerPixel )
				DR = Shrink<vlUInt16>( SR, SourceInfo.uiRBitsPerPixel, DestInfo.uiRBitsPerPixel );
			else if ( DestInfo.uiRBitsPerPixel > SourceInfo.uiRBitsPerPixel )
				DR = Expand<vlUInt16>( SR, SourceInfo.uiRBitsPerPixel, DestInfo.uiRBitsPerPixel );
			else
				DR = SR;
		}

		if ( uiSourceGMask && uiDestGMask )
		{
			if ( DestInfo.uiGBitsPerPixel < SourceInfo.uiGBitsPerPixel )
				DG = Shrink<vlUInt16>( SG, SourceInfo.uiGBitsPerPixel, DestInfo.uiGBitsPerPixel );
			else if ( DestInfo.uiGBitsPerPixel > SourceInfo.uiGBitsPerPixel )
				DG = Expand<vlUInt16>( SG, SourceInfo.uiGBitsPerPixel, DestInfo.uiGBitsPerPixel );
			else
				DG = SG;
		}

		if ( uiSourceBMask && uiDestBMask )
		{
			if ( DestInfo.uiBBitsPerPixel < SourceInfo.uiBBitsPerPixel )
				DB = Shrink<vlUInt16>( SB, SourceInfo.uiBBitsPerPixel, DestInfo.uiBBitsPerPixel );
			else if ( DestInfo.uiBBitsPerPixel > SourceInfo.uiBBitsPerPixel )
				DB = Expand<vlUInt16>( SB, SourceInfo.uiBBitsPerPixel, DestInfo.uiBBitsPerPixel );
			else
				DB = SB;
		}

		if ( uiSourceAMask && uiDestAMask )
		{
			if ( DestInfo.uiABitsPerPixel < SourceInfo.uiABitsPerPixel )
				DA = Shrink<vlUInt16>( SA, SourceInfo.uiABitsPerPixel, DestInfo.uiABitsPerPixel );
			else if ( DestInfo.uiABitsPerPixel > SourceInfo.uiABitsPerPixel )
				DA = Expand<vlUInt16>( SA, SourceInfo.uiABitsPerPixel, DestInfo.uiABitsPerPixel );
			else
				DA = SA;
		}

		U Dest = ( ( U )( DR & uiDestRMask ) << ( U )uiDestRShift ) | ( ( U )( DG & uiDestGMask ) << ( U )uiDestGShift ) | ( ( U )( DB & uiDestBMask ) << ( U )uiDestBShift ) | ( ( U )( DA & uiDestAMask ) << ( U )uiDestAShift );
		for ( i = 0; i < DestInfo.uiBytesPerPixel; i++ )
		{
			lpDest[i] = ( vlByte )( ( Dest >> ( ( T )i * 8 ) ) & 0xff );
		}
	}

	return vlTrue;
}

vlBool CVTFFile::Convert( vlByte *lpSource, vlByte *lpDest, vlUInt uiWidth, vlUInt uiHeight, VTFImageFormat SourceFormat, VTFImageFormat DestFormat, vlUInt32 uiCompressedSize )
{
	const SVTFImageConvertInfo& SourceInfo = VTFImageConvertInfo[SourceFormat];
	const SVTFImageConvertInfo& DestInfo = VTFImageConvertInfo[DestFormat];

	if ( !SourceInfo.bIsSupported || !DestInfo.bIsSupported )
	{
		return vlFalse;
	}

#ifndef SHELLINFO_EXPORTS
	struct DelAtEndOfScope
	{
		~DelAtEndOfScope()
		{
			delete[] ptr;
		}

		vlByte* ptr = nullptr;
	} delMe;

	if ( uiCompressedSize != 0 )
	{
		z_stream zStream;
		memset( &zStream, 0, sizeof( zStream ) );
		if ( inflateInit( &zStream ) != Z_OK )
			return vlFalse;

		zStream.next_in = lpSource;
		zStream.avail_in = uiCompressedSize;
		zStream.total_out = 0;

		vlUInt32 size = CVTFFile::ComputeImageSize( uiWidth, uiHeight, 1, SourceFormat );
		vlByte* pConverted = new vlByte[size];
		while ( zStream.avail_in )
		{
			zStream.next_out = pConverted + zStream.total_out;
			zStream.avail_out = size - zStream.total_out;

			int zRet = inflate( &zStream, Z_NO_FLUSH );
			bool zFailure = ( zRet != Z_OK ) && ( zRet != Z_STREAM_END );
			if ( zFailure || ( ( zRet == Z_STREAM_END ) && ( zStream.total_out != size ) ) )
			{
				inflateEnd( &zStream );
				return vlFalse;
			}
		}

		inflateEnd( &zStream );
		delMe.ptr = lpSource = pConverted;
	}
#endif

	if ( SourceFormat == DestFormat )
	{
		memcpy( lpDest, lpSource, CVTFFile::ComputeImageSize( uiWidth, uiHeight, 1, DestFormat ) );
		return vlTrue;
	}

	if ( SourceFormat == IMAGE_FORMAT_RGB888 && DestFormat == IMAGE_FORMAT_RGBA8888 )
	{
		vlByte *lpLast = lpSource + CVTFFile::ComputeImageSize( uiWidth, uiHeight, 1, SourceFormat );
		for ( ; lpSource < lpLast; lpSource += 3, lpDest += 4 )
		{
			lpDest[0] = lpSource[0];
			lpDest[1] = lpSource[1];
			lpDest[2] = lpSource[2];
			lpDest[3] = 255;
		}
		return vlTrue;
	}

	if ( SourceFormat == IMAGE_FORMAT_RGBA8888 && DestFormat == IMAGE_FORMAT_RGB888 )
	{
		vlByte *lpLast = lpSource + CVTFFile::ComputeImageSize( uiWidth, uiHeight, 1, SourceFormat );
		for ( ; lpSource < lpLast; lpSource += 4, lpDest += 3 )
		{
			lpDest[0] = lpSource[0];
			lpDest[1] = lpSource[1];
			lpDest[2] = lpSource[2];
		}
		return vlTrue;
	}

	if ( SourceInfo.bIsCompressed || DestInfo.bIsCompressed )
	{
		vlByte *lpSourceRGBA = lpSource;
		vlBool bResult = vlTrue;

		if ( SourceFormat != IMAGE_FORMAT_RGBA8888 )
		{
			lpSourceRGBA = new vlByte[CVTFFile::ComputeImageSize( uiWidth, uiHeight, 1, IMAGE_FORMAT_RGBA8888 )];
		}

		switch ( SourceFormat )
		{
		case IMAGE_FORMAT_RGBA8888:
			break;
		case IMAGE_FORMAT_DXT1:
		case IMAGE_FORMAT_DXT1_ONEBITALPHA:
			bResult = CVTFFile::DecompressDXT1( lpSource, lpSourceRGBA, uiWidth, uiHeight );
			break;
		case IMAGE_FORMAT_DXT3:
			bResult = CVTFFile::DecompressDXT3( lpSource, lpSourceRGBA, uiWidth, uiHeight );
			break;
		case IMAGE_FORMAT_DXT5:
			bResult = CVTFFile::DecompressDXT5( lpSource, lpSourceRGBA, uiWidth, uiHeight );
			break;
		default:
			bResult = CVTFFile::Convert( lpSource, lpSourceRGBA, uiWidth, uiHeight, SourceFormat, IMAGE_FORMAT_RGBA8888, 0 );
			break;
		}

		if ( bResult )
		{
			switch ( DestFormat )
			{
			case IMAGE_FORMAT_DXT1:
			case IMAGE_FORMAT_DXT1_ONEBITALPHA:
			case IMAGE_FORMAT_DXT3:
			case IMAGE_FORMAT_DXT5:
				break;
			default:
				bResult = CVTFFile::Convert( lpSourceRGBA, lpDest, uiWidth, uiHeight, IMAGE_FORMAT_RGBA8888, DestFormat, 0 );
				break;
			}
		}

		if ( lpSourceRGBA != lpSource )
		{
			delete[]lpSourceRGBA;
		}

		return bResult;
	}
	else
	{
		if ( SourceInfo.uiBytesPerPixel <= 1 )
		{
			if ( DestInfo.uiBytesPerPixel <= 1 )
				return ConvertTemplated<vlUInt8, vlUInt8>( lpSource, lpDest, uiWidth, uiHeight, SourceInfo, DestInfo );
			else if ( DestInfo.uiBytesPerPixel <= 2 )
				return ConvertTemplated<vlUInt8, vlUInt16>( lpSource, lpDest, uiWidth, uiHeight, SourceInfo, DestInfo );
			else if ( DestInfo.uiBytesPerPixel <= 4 )
				return ConvertTemplated<vlUInt8, vlUInt32>( lpSource, lpDest, uiWidth, uiHeight, SourceInfo, DestInfo );
			else if ( DestInfo.uiBytesPerPixel <= 8 )
				return ConvertTemplated<vlUInt8, vlUInt64>( lpSource, lpDest, uiWidth, uiHeight, SourceInfo, DestInfo );
		}
		else if ( SourceInfo.uiBytesPerPixel <= 2 )
		{
			if ( DestInfo.uiBytesPerPixel <= 1 )
				return ConvertTemplated<vlUInt16, vlUInt8>( lpSource, lpDest, uiWidth, uiHeight, SourceInfo, DestInfo );
			else if ( DestInfo.uiBytesPerPixel <= 2 )
				return ConvertTemplated<vlUInt16, vlUInt16>( lpSource, lpDest, uiWidth, uiHeight, SourceInfo, DestInfo );
			else if ( DestInfo.uiBytesPerPixel <= 4 )
				return ConvertTemplated<vlUInt16, vlUInt32>( lpSource, lpDest, uiWidth, uiHeight, SourceInfo, DestInfo );
			else if ( DestInfo.uiBytesPerPixel <= 8 )
				return ConvertTemplated<vlUInt16, vlUInt64>( lpSource, lpDest, uiWidth, uiHeight, SourceInfo, DestInfo );
		}
		else if ( SourceInfo.uiBytesPerPixel <= 4 )
		{
			if ( DestInfo.uiBytesPerPixel <= 1 )
				return ConvertTemplated<vlUInt32, vlUInt8>( lpSource, lpDest, uiWidth, uiHeight, SourceInfo, DestInfo );
			else if ( DestInfo.uiBytesPerPixel <= 2 )
				return ConvertTemplated<vlUInt32, vlUInt16>( lpSource, lpDest, uiWidth, uiHeight, SourceInfo, DestInfo );
			else if ( DestInfo.uiBytesPerPixel <= 4 )
				return ConvertTemplated<vlUInt32, vlUInt32>( lpSource, lpDest, uiWidth, uiHeight, SourceInfo, DestInfo );
			else if ( DestInfo.uiBytesPerPixel <= 8 )
				return ConvertTemplated<vlUInt32, vlUInt64>( lpSource, lpDest, uiWidth, uiHeight, SourceInfo, DestInfo );
		}
		else if ( SourceInfo.uiBytesPerPixel <= 8 )
		{
			if ( DestInfo.uiBytesPerPixel <= 1 )
				return ConvertTemplated<vlUInt64, vlUInt8>( lpSource, lpDest, uiWidth, uiHeight, SourceInfo, DestInfo );
			else if ( DestInfo.uiBytesPerPixel <= 2 )
				return ConvertTemplated<vlUInt64, vlUInt16>( lpSource, lpDest, uiWidth, uiHeight, SourceInfo, DestInfo );
			else if ( DestInfo.uiBytesPerPixel <= 4 )
				return ConvertTemplated<vlUInt64, vlUInt32>( lpSource, lpDest, uiWidth, uiHeight, SourceInfo, DestInfo );
			else if ( DestInfo.uiBytesPerPixel <= 8 )
				return ConvertTemplated<vlUInt64, vlUInt64>( lpSource, lpDest, uiWidth, uiHeight, SourceInfo, DestInfo );
		}
		return vlFalse;
	}
}
