﻿#pragma once


typedef unsigned char	vlBool;
typedef char			vlChar;
typedef unsigned char	vlByte;
typedef signed short	vlShort;
typedef unsigned short	vlUShort;
typedef signed int		vlInt;
typedef unsigned int	vlUInt;
typedef unsigned char	vlUInt8;
typedef unsigned short	vlUInt16;
typedef unsigned int	vlUInt32;
typedef unsigned long	vlUInt64;
typedef signed long		vlLong;
typedef unsigned long	vlULong;
typedef float			vlSingle;
typedef double			vlDouble;
typedef void			vlVoid;
typedef vlSingle		vlFloat;

#define vlFalse			0
#define vlTrue			1

typedef enum tagVTFResourceEntryTypeFlag
{
	RSRCF_HAS_NO_DATA_CHUNK = 0x02
} VTFResourceEntryTypeFlag;

#define MAKE_VTF_RSRC_ID(a, b, c) ((vlUInt)(((vlByte)a) | ((vlByte)b << 8) | ((vlByte)c << 16)))
#define MAKE_VTF_RSRC_IDF(a, b, c, d) ((vlUInt)(((vlByte)a) | ((vlByte)b << 8) | ((vlByte)c << 16) | ((vlByte)d << 24)))
typedef enum tagVTFResourceEntryType
{
	VTF_LEGACY_RSRC_LOW_RES_IMAGE = MAKE_VTF_RSRC_ID( 0x01, 0, 0 ),
	VTF_LEGACY_RSRC_IMAGE = MAKE_VTF_RSRC_ID( 0x30, 0, 0 ),
	VTF_RSRC_SHEET = MAKE_VTF_RSRC_ID( 0x10, 0, 0 ),
	VTF_RSRC_CRC = MAKE_VTF_RSRC_IDF( 'C', 'R', 'C', RSRCF_HAS_NO_DATA_CHUNK ),
	VTF_RSRC_TEXTURE_LOD_SETTINGS = MAKE_VTF_RSRC_IDF( 'L', 'O', 'D', RSRCF_HAS_NO_DATA_CHUNK ),
	VTF_RSRC_TEXTURE_SETTINGS_EX = MAKE_VTF_RSRC_IDF( 'T', 'S', 'O', RSRCF_HAS_NO_DATA_CHUNK ),
	VTF_RSRC_KEY_VALUE_DATA = MAKE_VTF_RSRC_ID( 'K', 'V', 'D' ),
	VTF_RSRC_AUX_COMPRESSION_INFO = MAKE_VTF_RSRC_ID( 'A', 'X', 'C' ),
	VTF_RSRC_MAX_DICTIONARY_ENTRIES = 32
} VTFResourceEntryType;

typedef enum tagVTFImageFormat
{
	IMAGE_FORMAT_RGBA8888 = 0,
	IMAGE_FORMAT_ABGR8888,
	IMAGE_FORMAT_RGB888,
	IMAGE_FORMAT_BGR888,
	IMAGE_FORMAT_RGB565,
	IMAGE_FORMAT_I8,
	IMAGE_FORMAT_IA88,
	IMAGE_FORMAT_P8,
	IMAGE_FORMAT_A8,
	IMAGE_FORMAT_RGB888_BLUESCREEN,
	IMAGE_FORMAT_BGR888_BLUESCREEN,
	IMAGE_FORMAT_ARGB8888,
	IMAGE_FORMAT_BGRA8888,
	IMAGE_FORMAT_DXT1,
	IMAGE_FORMAT_DXT3,
	IMAGE_FORMAT_DXT5,
	IMAGE_FORMAT_BGRX8888,
	IMAGE_FORMAT_BGR565,
	IMAGE_FORMAT_BGRX5551,
	IMAGE_FORMAT_BGRA4444,
	IMAGE_FORMAT_DXT1_ONEBITALPHA,
	IMAGE_FORMAT_BGRA5551,
	IMAGE_FORMAT_UV88,
	IMAGE_FORMAT_UVWQ8888,
	IMAGE_FORMAT_RGBA16161616F,
	IMAGE_FORMAT_RGBA16161616,
	IMAGE_FORMAT_UVLX8888,
	IMAGE_FORMAT_R32F,
	IMAGE_FORMAT_RGB323232F,
	IMAGE_FORMAT_RGBA32323232F,
	IMAGE_FORMAT_NV_DST16,
	IMAGE_FORMAT_ATI_DST16,
	IMAGE_FORMAT_ATI_DST24,
	IMAGE_FORMAT_NV_NULL,
	IMAGE_FORMAT_ATI2N,
	IMAGE_FORMAT_ATI1N,
	IMAGE_FORMAT_RGBA1010102,
	IMAGE_FORMAT_BGRA1010102,
	IMAGE_FORMAT_R16F,
	IMAGE_FORMAT_D16,
	IMAGE_FORMAT_D15S1,
	IMAGE_FORMAT_D32,
	IMAGE_FORMAT_D24S8,
	IMAGE_FORMAT_LINEAR_D24S8,
	IMAGE_FORMAT_D24X8,
	IMAGE_FORMAT_D24X4S4,
	IMAGE_FORMAT_D24FS8,
	IMAGE_FORMAT_D16_SHADOW,
	IMAGE_FORMAT_D24X8_SHADOW,
	IMAGE_FORMAT_LINEAR_BGRX8888,
	IMAGE_FORMAT_LINEAR_RGBA8888,
	IMAGE_FORMAT_LINEAR_ABGR8888,
	IMAGE_FORMAT_LINEAR_ARGB8888,
	IMAGE_FORMAT_LINEAR_BGRA8888,
	IMAGE_FORMAT_LINEAR_RGB888,
	IMAGE_FORMAT_LINEAR_BGR888,
	IMAGE_FORMAT_LINEAR_BGRX5551,
	IMAGE_FORMAT_LINEAR_I8,
	IMAGE_FORMAT_LINEAR_RGBA16161616,
	IMAGE_FORMAT_LINEAR_A8,
	IMAGE_FORMAT_LINEAR_DXT1,
	IMAGE_FORMAT_LINEAR_DXT3,
	IMAGE_FORMAT_LINEAR_DXT5,
	IMAGE_FORMAT_LE_BGRX8888,
	IMAGE_FORMAT_LE_BGRA8888,
	IMAGE_FORMAT_DXT1_RUNTIME,
	IMAGE_FORMAT_DXT5_RUNTIME,
	IMAGE_FORMAT_DXT3_RUNTIME,
	IMAGE_FORMAT_INTZ,
	IMAGE_FORMAT_R8,
	IMAGE_FORMAT_BC7,
	IMAGE_FORMAT_BC6H,
	IMAGE_FORMAT_COUNT,
	IMAGE_FORMAT_NONE = -1
} VTFImageFormat;
typedef enum tagVTFImageFlag
{
	TEXTUREFLAGS_POINTSAMPLE = 0x00000001,
	TEXTUREFLAGS_TRILINEAR = 0x00000002,
	TEXTUREFLAGS_CLAMPS = 0x00000004,
	TEXTUREFLAGS_CLAMPT = 0x00000008,
	TEXTUREFLAGS_ANISOTROPIC = 0x00000010,
	TEXTUREFLAGS_HINT_DXT5 = 0x00000020,
	TEXTUREFLAGS_SRGB = 0x00000040, // Originally internal to VTex as TEXTUREFLAGS_NOCOMPRESS.
	TEXTUREFLAGS_DEPRECATED_NOCOMPRESS = 0x00000040,
	TEXTUREFLAGS_NORMAL = 0x00000080,
	TEXTUREFLAGS_NOMIP = 0x00000100,
	TEXTUREFLAGS_NOLOD = 0x00000200,
	TEXTUREFLAGS_MINMIP = 0x00000400,
	TEXTUREFLAGS_PROCEDURAL = 0x00000800,
	TEXTUREFLAGS_ONEBITALPHA = 0x00001000, //!< Automatically generated by VTex.
	TEXTUREFLAGS_EIGHTBITALPHA = 0x00002000, //!< Automatically generated by VTex.
	TEXTUREFLAGS_ENVMAP = 0x00004000,
	TEXTUREFLAGS_RENDERTARGET = 0x00008000,
	TEXTUREFLAGS_DEPTHRENDERTARGET = 0x00010000,
	TEXTUREFLAGS_NODEBUGOVERRIDE = 0x00020000,
	TEXTUREFLAGS_SINGLECOPY = 0x00040000,
	TEXTUREFLAGS_UNUSED0 = 0x00080000, //!< Originally internal to VTex as TEXTUREFLAGS_ONEOVERMIPLEVELINALPHA.
	TEXTUREFLAGS_DEPRECATED_ONEOVERMIPLEVELINALPHA = 0x00080000,
	TEXTUREFLAGS_UNUSED1 = 0x00100000, //!< Originally internal to VTex as TEXTUREFLAGS_PREMULTCOLORBYONEOVERMIPLEVEL.
	TEXTUREFLAGS_DEPRECATED_PREMULTCOLORBYONEOVERMIPLEVEL = 0x00100000,
	TEXTUREFLAGS_UNUSED2 = 0x00200000, //!< Originally internal to VTex as TEXTUREFLAGS_NORMALTODUDV.
	TEXTUREFLAGS_DEPRECATED_NORMALTODUDV = 0x00200000,
	TEXTUREFLAGS_UNUSED3 = 0x00400000, //!< Originally internal to VTex as TEXTUREFLAGS_ALPHATESTMIPGENERATION.
	TEXTUREFLAGS_DEPRECATED_ALPHATESTMIPGENERATION = 0x00400000,
	TEXTUREFLAGS_NODEPTHBUFFER = 0x00800000,
	TEXTUREFLAGS_UNUSED4 = 0x01000000, //!< Originally internal to VTex as TEXTUREFLAGS_NICEFILTERED.
	TEXTUREFLAGS_DEPRECATED_NICEFILTERED = 0x01000000,
	TEXTUREFLAGS_CLAMPU = 0x02000000,
	TEXTUREFLAGS_VERTEXTEXTURE = 0x04000000,
	TEXTUREFLAGS_SSBUMP = 0x08000000,
	TEXTUREFLAGS_UNUSED5 = 0x10000000, //!< Originally TEXTUREFLAGS_UNFILTERABLE_OK.
	TEXTUREFLAGS_DEPRECATED_UNFILTERABLE_OK = 0x10000000,
	TEXTUREFLAGS_BORDER = 0x20000000,
	TEXTUREFLAGS_DEPRECATED_SPECVAR_RED = 0x40000000,
	TEXTUREFLAGS_DEPRECATED_SPECVAR_ALPHA = 0x80000000,
	TEXTUREFLAGS_LAST = 0x20000000,
	TEXTUREFLAGS_COUNT = 30
} VTFImageFlag;

#pragma pack(1)
struct SVTFResource
{
	union
	{
		vlUInt Type;
		struct
		{
			vlByte ID[3];	//!< Unique resource ID
			vlByte Flags;	//!< Resource flags
		};
	};
	vlUInt Data;	//!< Resource data (e.g. for a  CRC) or offset from start of the file
};
struct SVTFResourceData
{
	vlUInt Size;	//!< Resource data buffer size
	vlByte *Data;	//!< Resource data bufffer
};
struct SVTFFileHeader
{
	vlChar			TypeString[4];					//!< "Magic number" identifier- "VTF\0".
	vlUInt			Version[2];						//!< Version[0].version[1] (currently 7.2)
	vlUInt			HeaderSize;						//!< Size of the header struct (currently 80 bytes)
};
struct SVTFHeader_70 : public SVTFFileHeader
{
	vlUShort		Width;							//!< Width of the largest image
	vlUShort		Height;							//!< Height of the largest image
	vlUInt			Flags;							//!< Flags for the image
	vlUShort		Frames;							//!< Number of frames if animated (1 for no animation)
	vlUShort		StartFrame;						//!< Start frame (always 0)
	vlByte			Padding0[4];					//!< Reflectivity padding (16 byte alignment)
	vlSingle		Reflectivity[3];				//!< Reflectivity vector
	vlByte			Padding1[4];					//!< Reflectivity padding (8 byte packing)
	vlSingle		BumpScale;						//!< Bump map scale
	VTFImageFormat	ImageFormat;					//!< Image format index
	vlByte			MipCount;						//!< Number of MIP levels (including the largest image)
	VTFImageFormat	LowResImageFormat;				//!< Image format of the thumbnail image
	vlByte			LowResImageWidth;				//!< Thumbnail image width
	vlByte			LowResImageHeight;				//!< Thumbnail image height
};
struct SVTFHeader_71 : public SVTFHeader_70
{
};
struct SVTFHeader_72 : public SVTFHeader_71
{
	vlUShort		Depth;							//!< Depth of the largest image
};
struct SVTFHeader_73 : public SVTFHeader_72
{
	vlByte		Padding2[3];
	vlUInt		ResourceCount;							//!< Number of image resources
};
struct SVTFHeader_74 : public SVTFHeader_73 {};
__declspec( align( 16 ) ) struct SVTFHeader_74_A : public SVTFHeader_74 {};
struct SVTFHeader : public SVTFHeader_74_A
{
	vlByte				Padding3[8];
	SVTFResource		Resources[VTF_RSRC_MAX_DICTIONARY_ENTRIES];
	SVTFResourceData	Data[VTF_RSRC_MAX_DICTIONARY_ENTRIES];
};
typedef struct tagSVTFImageFormatInfo
{
	const wchar_t *lpName;			//!< Enumeration text equivalent.
	vlUInt	uiBitsPerPixel;			//!< Format bits per pixel.
	vlUInt	uiBytesPerPixel;		//!< Format bytes per pixel.
	vlUInt	uiRedBitsPerPixel;		//!< Format red bits per pixel.  0 for N/A.
	vlUInt	uiGreenBitsPerPixel;	//!< Format green bits per pixel.  0 for N/A.
	vlUInt	uiBlueBitsPerPixel;		//!< Format blue bits per pixel.  0 for N/A.
	vlUInt	uiAlphaBitsPerPixel;	//!< Format alpha bits per pixel.  0 for N/A.
	vlBool	bIsCompressed;			//!< Format is compressed (DXT).
	vlBool	bIsSupported;			//!< Format is supported by VTFLib.
} SVTFImageFormatInfo;
#pragma pack()

struct AuxCompressionInfoHeader_t
{
	vlUInt32 m_CompressionLevel; // -1 = default compression, 0 = no compression, 1-9 = specific compression from lowest to highest
};

struct AuxCompressionInfoEntry_t
{
	vlUInt32 m_CompressedSize; // Size of compressed face image data
};

namespace IO
{
	namespace Readers
	{
		class IReader;
	}
}
class CVTFFile
{
private:
	SVTFHeader * Header;

	vlUInt uiImageBufferSize;
	vlByte *lpImageData;

	vlUInt uiThumbnailBufferSize;
	vlByte *lpThumbnailImageData;

public:
	CVTFFile();

	~CVTFFile();

	vlVoid Destroy();

	vlBool IsLoaded() const;

	vlBool Load( const vlVoid *lpData, vlUInt uiBufferSize, vlBool bHeaderOnly = vlFalse );

private:
	static vlBool IsPowerOfTwo( vlUInt uiSize );
	static vlUInt NextPowerOfTwo( vlUInt uiSize );

	vlBool Load( IO::Readers::IReader *Reader, vlBool bHeaderOnly );

public:
	vlUInt GetWidth() const;
	vlUInt GetHeight() const;
	vlUInt GetDepth() const;
	vlUInt GetMipmapCount() const;
	vlUInt GetFaceCount() const;
	vlUInt GetFrameCount() const;
	vlUInt GetFlags() const;
	VTFImageFormat GetFormat() const;

	vlByte *GetData( vlUInt uiFrame = 0, vlUInt uiFace = 0, vlUInt uiSlice = 0, vlUInt uiMipmapLevel = 0 ) const;

	vlVoid *GetResourceData( vlUInt uiType, vlUInt &uiSize ) const;

	vlUInt GetAuxInfoOffset( vlUInt iFrame, vlUInt iFace, vlUInt iMipLevel ) const;

	const SVTFHeader& GetHeader() const;

public:
	static SVTFImageFormatInfo const &GetImageFormatInfo( VTFImageFormat ImageFormat );

	static vlUInt ComputeImageSize( vlUInt uiWidth, vlUInt uiHeight, vlUInt uiDepth, VTFImageFormat ImageFormat );
	static vlUInt ComputeImageSize( vlUInt uiWidth, vlUInt uiHeight, vlUInt uiDepth, vlUInt uiMipmaps, VTFImageFormat ImageFormat );

	static vlUInt ComputeMipmapCount( vlUInt uiWidth, vlUInt uiHeight, vlUInt uiDepth );
	static vlVoid ComputeMipmapDimensions( vlUInt uiWidth, vlUInt uiHeight, vlUInt uiDepth, vlUInt uiMipmapLevel, vlUInt &uiMipmapWidth, vlUInt &uiMipmapHeight, vlUInt &uiMipmapDepth );
	static vlUInt ComputeMipmapSize( vlUInt uiWidth, vlUInt uiHeight, vlUInt uiDepth, vlUInt uiMipmapLevel, VTFImageFormat ImageFormat );

private:
	vlUInt ComputeDataOffset( vlUInt uiFrame, vlUInt uiFace, vlUInt uiSlice, vlUInt uiMipmapLevel, VTFImageFormat ImageFormat ) const;

public:
	static vlBool Convert( vlByte *lpSource, vlByte *lpDest, vlUInt uiWidth, vlUInt uiHeight, VTFImageFormat SourceFormat, VTFImageFormat DestFormat, vlUInt32 uiCompressedSize );

private:
	static vlBool DecompressDXT1( vlByte *src, vlByte *dst, vlUInt uiWidth, vlUInt uiHeight );
	static vlBool DecompressDXT3( vlByte *src, vlByte *dst, vlUInt uiWidth, vlUInt uiHeight );
	static vlBool DecompressDXT5( vlByte *src, vlByte *dst, vlUInt uiWidth, vlUInt uiHeight );
	static vlBool DecompressATI1N( vlByte *src, vlByte *dst, vlUInt uiWidth, vlUInt uiHeight );
	static vlBool DecompressATI2N( vlByte *src, vlByte *dst, vlUInt uiWidth, vlUInt uiHeight );
	static vlBool DecompressBC6H( vlByte *src, vlByte *dst, vlUInt uiWidth, vlUInt uiHeight );
	static vlBool DecompressBC7( vlByte *src, vlByte *dst, vlUInt uiWidth, vlUInt uiHeight );
};


