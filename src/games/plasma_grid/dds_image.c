
#include <stdhdrs.h>
#include <vectlib.h>

//#ifndef WIN32
enum D3D10_RESOURCE_DIMENSION { 
  D3D10_RESOURCE_DIMENSION_UNKNOWN    = 0,
  D3D10_RESOURCE_DIMENSION_BUFFER     = 1,
  D3D10_RESOURCE_DIMENSION_TEXTURE1D  = 2,
  D3D10_RESOURCE_DIMENSION_TEXTURE2D  = 3,
  D3D10_RESOURCE_DIMENSION_TEXTURE3D  = 4
};

enum DXGI_FORMAT { 
  DXGI_FORMAT_UNKNOWN                     = 0,
  DXGI_FORMAT_R32G32B32A32_TYPELESS       = 1,
  DXGI_FORMAT_R32G32B32A32_FLOAT          = 2,
  DXGI_FORMAT_R32G32B32A32_UINT           = 3,
  DXGI_FORMAT_R32G32B32A32_SINT           = 4,
  DXGI_FORMAT_R32G32B32_TYPELESS          = 5,
  DXGI_FORMAT_R32G32B32_FLOAT             = 6,
  DXGI_FORMAT_R32G32B32_UINT              = 7,
  DXGI_FORMAT_R32G32B32_SINT              = 8,
  DXGI_FORMAT_R16G16B16A16_TYPELESS       = 9,
  DXGI_FORMAT_R16G16B16A16_FLOAT          = 10,
  DXGI_FORMAT_R16G16B16A16_UNORM          = 11,
  DXGI_FORMAT_R16G16B16A16_UINT           = 12,
  DXGI_FORMAT_R16G16B16A16_SNORM          = 13,
  DXGI_FORMAT_R16G16B16A16_SINT           = 14,
  DXGI_FORMAT_R32G32_TYPELESS             = 15,
  DXGI_FORMAT_R32G32_FLOAT                = 16,
  DXGI_FORMAT_R32G32_UINT                 = 17,
  DXGI_FORMAT_R32G32_SINT                 = 18,
  DXGI_FORMAT_R32G8X24_TYPELESS           = 19,
  DXGI_FORMAT_D32_FLOAT_S8X24_UINT        = 20,
  DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS    = 21,
  DXGI_FORMAT_X32_TYPELESS_G8X24_UINT     = 22,
  DXGI_FORMAT_R10G10B10A2_TYPELESS        = 23,
  DXGI_FORMAT_R10G10B10A2_UNORM           = 24,
  DXGI_FORMAT_R10G10B10A2_UINT            = 25,
  DXGI_FORMAT_R11G11B10_FLOAT             = 26,
  DXGI_FORMAT_R8G8B8A8_TYPELESS           = 27,
  DXGI_FORMAT_R8G8B8A8_UNORM              = 28,
  DXGI_FORMAT_R8G8B8A8_UNORM_SRGB         = 29,
  DXGI_FORMAT_R8G8B8A8_UINT               = 30,
  DXGI_FORMAT_R8G8B8A8_SNORM              = 31,
  DXGI_FORMAT_R8G8B8A8_SINT               = 32,
  DXGI_FORMAT_R16G16_TYPELESS             = 33,
  DXGI_FORMAT_R16G16_FLOAT                = 34,
  DXGI_FORMAT_R16G16_UNORM                = 35,
  DXGI_FORMAT_R16G16_UINT                 = 36,
  DXGI_FORMAT_R16G16_SNORM                = 37,
  DXGI_FORMAT_R16G16_SINT                 = 38,
  DXGI_FORMAT_R32_TYPELESS                = 39,
  DXGI_FORMAT_D32_FLOAT                   = 40,
  DXGI_FORMAT_R32_FLOAT                   = 41,
  DXGI_FORMAT_R32_UINT                    = 42,
  DXGI_FORMAT_R32_SINT                    = 43,
  DXGI_FORMAT_R24G8_TYPELESS              = 44,
  DXGI_FORMAT_D24_UNORM_S8_UINT           = 45,
  DXGI_FORMAT_R24_UNORM_X8_TYPELESS       = 46,
  DXGI_FORMAT_X24_TYPELESS_G8_UINT        = 47,
  DXGI_FORMAT_R8G8_TYPELESS               = 48,
  DXGI_FORMAT_R8G8_UNORM                  = 49,
  DXGI_FORMAT_R8G8_UINT                   = 50,
  DXGI_FORMAT_R8G8_SNORM                  = 51,
  DXGI_FORMAT_R8G8_SINT                   = 52,
  DXGI_FORMAT_R16_TYPELESS                = 53,
  DXGI_FORMAT_R16_FLOAT                   = 54,
  DXGI_FORMAT_D16_UNORM                   = 55,
  DXGI_FORMAT_R16_UNORM                   = 56,
  DXGI_FORMAT_R16_UINT                    = 57,
  DXGI_FORMAT_R16_SNORM                   = 58,
  DXGI_FORMAT_R16_SINT                    = 59,
  DXGI_FORMAT_R8_TYPELESS                 = 60,
  DXGI_FORMAT_R8_UNORM                    = 61,
  DXGI_FORMAT_R8_UINT                     = 62,
  DXGI_FORMAT_R8_SNORM                    = 63,
  DXGI_FORMAT_R8_SINT                     = 64,
  DXGI_FORMAT_A8_UNORM                    = 65,
  DXGI_FORMAT_R1_UNORM                    = 66,
  DXGI_FORMAT_R9G9B9E5_SHAREDEXP          = 67,
  DXGI_FORMAT_R8G8_B8G8_UNORM             = 68,
  DXGI_FORMAT_G8R8_G8B8_UNORM             = 69,
  DXGI_FORMAT_BC1_TYPELESS                = 70,
  DXGI_FORMAT_BC1_UNORM                   = 71,
  DXGI_FORMAT_BC1_UNORM_SRGB              = 72,
  DXGI_FORMAT_BC2_TYPELESS                = 73,
  DXGI_FORMAT_BC2_UNORM                   = 74,
  DXGI_FORMAT_BC2_UNORM_SRGB              = 75,
  DXGI_FORMAT_BC3_TYPELESS                = 76,
  DXGI_FORMAT_BC3_UNORM                   = 77,
  DXGI_FORMAT_BC3_UNORM_SRGB              = 78,
  DXGI_FORMAT_BC4_TYPELESS                = 79,
  DXGI_FORMAT_BC4_UNORM                   = 80,
  DXGI_FORMAT_BC4_SNORM                   = 81,
  DXGI_FORMAT_BC5_TYPELESS                = 82,
  DXGI_FORMAT_BC5_UNORM                   = 83,
  DXGI_FORMAT_BC5_SNORM                   = 84,
  DXGI_FORMAT_B5G6R5_UNORM                = 85,
  DXGI_FORMAT_B5G5R5A1_UNORM              = 86,
  DXGI_FORMAT_B8G8R8A8_UNORM              = 87,
  DXGI_FORMAT_B8G8R8X8_UNORM              = 88,
  DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM  = 89,
  DXGI_FORMAT_B8G8R8A8_TYPELESS           = 90,
  DXGI_FORMAT_B8G8R8A8_UNORM_SRGB         = 91,
  DXGI_FORMAT_B8G8R8X8_TYPELESS           = 92,
  DXGI_FORMAT_B8G8R8X8_UNORM_SRGB         = 93,
  DXGI_FORMAT_BC6H_TYPELESS               = 94,
  DXGI_FORMAT_BC6H_UF16                   = 95,
  DXGI_FORMAT_BC6H_SF16                   = 96,
  DXGI_FORMAT_BC7_TYPELESS                = 97,
  DXGI_FORMAT_BC7_UNORM                   = 98,
  DXGI_FORMAT_BC7_UNORM_SRGB              = 99,
  DXGI_FORMAT_AYUV                        = 100,
  DXGI_FORMAT_Y410                        = 101,
  DXGI_FORMAT_Y416                        = 102,
  DXGI_FORMAT_NV12                        = 103,
  DXGI_FORMAT_P010                        = 104,
  DXGI_FORMAT_P016                        = 105,
  DXGI_FORMAT_420_OPAQUE                  = 106,
  DXGI_FORMAT_YUY2                        = 107,
  DXGI_FORMAT_Y210                        = 108,
  DXGI_FORMAT_Y216                        = 109,
  DXGI_FORMAT_NV11                        = 110,
  DXGI_FORMAT_AI44                        = 111,
  DXGI_FORMAT_IA44                        = 112,
  DXGI_FORMAT_P8                          = 113,
  DXGI_FORMAT_A8P8                        = 114,
  DXGI_FORMAT_B4G4R4A4_UNORM              = 115,
  DXGI_FORMAT_P208                        = 130,
  DXGI_FORMAT_V208                        = 131,
  DXGI_FORMAT_V408                        = 132,
  DXGI_FORMAT_FORCE_UINT                  = 0xffffffff
};


enum DDS_PIXELFORMAT_FLAGS{
DDPF_ALPHAPIXELS = 0x1, //Texture contains alpha data; dwRGBAlphaBitMask contains valid data.
DDPF_ALPHA = 0x2, //	Used in some older DDS files for alpha channel only uncompressed data (dwRGBBitCount contains the alpha channel bitcount; dwABitMask contains valid data)	0x2
DDPF_FOURCC = 0x4, //	Texture contains compressed RGB data; dwFourCC contains valid data.	0x4
DDPF_RGB = 0x40,   //	Texture contains uncompressed RGB data; dwRGBBitCount and the RGB masks (dwRBitMask, dwGBitMask, dwBBitMask) contain valid data.	0x40
DDPF_YUV = 0x200,  //	Used in some older DDS files for YUV uncompressed data (dwRGBBitCount contains the YUV bit count; dwRBitMask contains the Y mask, dwGBitMask contains the U mask, dwBBitMask contains the V mask)	
DDPF_LUMINANCE = 0x20000//	Used in some older DDS files for single channel color uncompressed data (dwRGBBitCount contains the luminance channel bit count; dwRBitMask contains the channel mask). Can be combined with DDPF_ALPHAPIXELS for a two channel DDS file.	
};

enum DDS_HEADER_FLAGS {
	DDSD_CAPS	=	0x1, // Required in every .dds file.
DDSD_HEIGHT	= 0x2, // Required in every .dds file.	
DDSD_WIDTH = 0x4, //	Required in every .dds file.	0x4
DDSD_PITCH = 0x8, //	Required when pitch is provided for an uncompressed texture.	0x8
DDSD_PIXELFORMAT = 0x1000, //	Required in every .dds file.	0x1000
DDSD_MIPMAPCOUNT = 0x20000, //	Required in a mipmapped texture.	0x20000
DDSD_LINEARSIZE = 0x80000, //	Required when pitch is provided for a compressed texture.	0x80000
DDSD_DEPTH = 0x800000, //	Required in a depth texture.	0x800000
};

enum DDS_HEADER_CAPS {
DDSCAPS_COMPLEX = 0x8,     //	Optional; must be used on any file that contains more than one surface (a mipmap, a cubic environment map, or mipmapped volume texture).	
DDSCAPS_MIPMAP = 0x400000, //	Optional; should be used for a mipmap.	
DDSCAPS_TEXTURE = 0x1000,  // Required	
};

enum DDS_HEADER_CAPS2 {
DDSCAPS2_CUBEMAP = 0x200, //	Required for a cube map.	
DDSCAPS2_CUBEMAP_POSITIVEX = 0x400, //	Required when these surfaces are stored in a cube map.	
DDSCAPS2_CUBEMAP_NEGATIVEX = 0x800, //	Required when these surfaces are stored in a cube map.	
DDSCAPS2_CUBEMAP_POSITIVEY = 0x1000, //	Required when these surfaces are stored in a cube map.	
DDSCAPS2_CUBEMAP_NEGATIVEY = 0x2000, //	Required when these surfaces are stored in a cube map.	
DDSCAPS2_CUBEMAP_POSITIVEZ = 0x4000, //	Required when these surfaces are stored in a cube map.	
DDSCAPS2_CUBEMAP_NEGATIVEZ = 0x8000, //	Required when these surfaces are stored in a cube map.	
DDSCAPS2_VOLUME = 0x200000, //	Required for a volume texture.	
};

typedef struct _DDS_PIXELFORMAT {
  DWORD dwSize;
  DWORD dwFlags;
  DWORD dwFourCC; // 'D','X','1','0
  DWORD dwRGBBitCount;
  DWORD dwRBitMask;
  DWORD dwGBitMask;
  DWORD dwBBitMask;
  DWORD dwABitMask;
}DDS_PIXELFORMAT;

typedef struct _DDS_HEADER {
  DWORD           dwSize;
  DWORD           dwFlags;
  DWORD           dwHeight;
  DWORD           dwWidth;
  DWORD           dwPitchOrLinearSize;
  DWORD           dwDepth;
  DWORD           dwMipMapCount;
  DWORD           dwReserved1[11];
  DDS_PIXELFORMAT ddspf;
  DWORD           dwCaps;
  DWORD           dwCaps2;
  DWORD           dwCaps3;
  DWORD           dwCaps4;
  DWORD           dwReserved2;
} DDS_HEADER;
//#else

// https://msdn.microsoft.com/en-us/library/windows/desktop/bb943983(v=vs.85).aspx
typedef struct {
  enum DXGI_FORMAT              dxgiFormat;
  enum D3D10_RESOURCE_DIMENSION resourceDimension;
  UINT                     miscFlag;
  UINT                     arraySize;
  UINT                     miscFlags2;
} DDS_HEADER_DXT10;

typedef struct {
	DDS_HEADER dds;
	DDS_HEADER_DXT10 dds10;
} DDS_HEADER_WITH_DXT10;

//#include <dds.h>

typedef union u_u32_s23e8 {
	float f;
	unsigned int i;
} u_u32_s23e8;

#define  shift  13
#define  shiftSign  16
 static int32_t const infN = 0x7F800000; // flt32 infinity
        static int32_t const maxN = 0x477FE000; // max flt16 normal as a flt32
        static int32_t const minN = 0x38800000; // min flt16 normal as a flt32
        static int32_t const signN = 0x80000000; // flt32 sign bit

#define infC (infN >> shift)
       #define nanN ( (infC + 1) << shift) // minimum flt16 nan as a flt32
        #define maxC ( maxN >> shift)
        #define minC (minN >> shift)
        #define signC ( signN >> shiftSign) // flt16 sign bit

        static int32_t const mulN = 0x52000000; // (1 << 23) / minN
        static int32_t const mulC = 0x33800000; // minN / (1 << (23 - shift))

        static int32_t const subC = 0x003FF; // max flt32 subnormal down shifted
        static int32_t const norC = 0x00400; // min flt32 normal down shifted

        #define maxD ( infC - maxC - 1)
        #define minD ( minC - subC - 1)
union Bits
        {
            float f;
            int32_t si;
            uint32_t ui;
        };
float revert( _16 value )
{
        {
            union Bits s;
            union Bits v;
			int32_t sign ;
			int32_t mask ;
            v.ui = value;
            sign = v.si & signC;
            v.si ^= sign;
            sign <<= shiftSign;
            v.si ^= ((v.si + minD) ^ v.si) & -(v.si > subC);
            v.si ^= ((v.si + maxD) ^ v.si) & -(v.si > maxC);
            s.si = mulC;
            s.f *= v.si;
            mask = -(norC > v.si);
            v.si <<= shift;
            v.si ^= (s.si ^ v.si) & mask;
            v.si |= sign;
            return v.f;
        }}

//#endif
_16 convert(float f)
{
  u_u32_s23e8 x;
  unsigned short _h;
  register int e;
  register int s;
  register int m;
  x.f = f;

  e = (x.i >> 23) & 0x000000ff;
  s = (x.i >> 16) & 0x00008000;
  m = x.i & 0x007fffff;

  e = e - 127;
  if (e == 128) {
    // infinity or NAN; preserve the leading bits of mantissa
    // because they tell whether it's a signaling or quiet NAN
    _h = s | (31 << 10) | (m >> 13);
  } else if (e > 15) {
    // overflow to infinity
    _h = s | (31 << 10);
  } else if (e > -15) {
    // normalized case
    if ((m & 0x00003fff) == 0x00001000) {
      // tie, round down to even
      _h = s | ((e+15) << 10) | (m >> 13);
    } else {
      // all non-ties, and tie round up to even
      //   (note that a mantissa of all 1's will round up to all 0's with
      //   the exponent being increased by 1, which is exactly what we want;
      //   for example, "0.5-epsilon" rounds up to 0.5, and 65535.0 rounds
      //   up to infinity.)
      _h = s | ((e+15) << 10) + ((m + 0x00001000) >> 13);
    }
  } else if (e > -25) {
    // convert to subnormal
    m |= 0x00800000; // restore the implied bit
    e = -14 - e; // shift count
    m >>= e; // M now in position but 2^13 too big
    if ((m & 0x00003fff) == 0x00001000) {
      // tie round down to even
    } else {
      // all non-ties, and tie round up to even
      m += (1 << 12); // m += 0x00001000
    }
    m >>= 13;
    _h = s | m;
  } else {
    // zero, or underflow
    _h = s;
  }
  return _h;
}

void ReadImage( CTEXTSTR filename )
{
	FILE *file;
	char mime[4];
	DDS_HEADER dds;
	float *data;

	file = sack_fopen( 0, filename, "rb" );
	if( file )
	{
		sack_fread( mime, 1, 4, file );
		sack_fread( &dds, 1, sizeof( DDS_HEADER ), file );
		data = NewArray( float, dds.dwWidth * dds.dwHeight );
		{
			int r, c;
			int imin = 100000 ;
			int imax = -100000;
			float min = 10000000;
			float max = -10000000;
			for( r = 0; r < dds.dwHeight; r++ )
				for( c = 0; c < dds.dwWidth; c++ )
				{
					S_16 val;
					float f;
					sack_fread( &val, 1, 2, file );
					f = revert( val );
					data[ ( r * dds.dwWidth ) + c ] = f;
					if( f > max )
						max = f;
					if( f < min )
						min = f;
					if( val < imin )
						imin = val;
					if( val > imax )
						imax = val;
					lprintf( "Val is %d %g", val, f );
				}
			lprintf( "min %g max %g", min, max );
		}
	}
	sack_fclose( file );

}

void WriteImage( CTEXTSTR filename, RCOORD *data, int width, int height, int pitch )
{
	FILE *file;
	DDS_HEADER dds;
	memset( &dds, 0, sizeof( dds ) );
/*
  DWORD           dwSize;=0x7c
  DWORD           dwFlags;0x2100f  DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH|DDSD_PITCH | DDSD_PIXELFORMAT|DDSD_MIPMAPCOUNT
  DWORD           dwHeight;  0x200
  DWORD           dwWidth;   0x200
  DWORD           dwPitchOrLinearSize;  0x400
  DWORD           dwDepth;   1
  DWORD           dwMipMapCount;  10
  DWORD           dwReserved1[11];   // 0's
  DDS_PIXELFORMAT ddspf;    
	DWORD dwSize;    0x20
	DWORD dwFlags;   0x020000   DDPF_LUMINANCE
	DWORD dwFourCC; // 0
	DWORD dwRGBBitCount; 0x10
	DWORD dwRBitMask;    0xFFFF
	DWORD dwGBitMask;   0
	DWORD dwBBitMask;   0
	DWORD dwABitMask;   0
  DWORD           dwCaps;  0x401008   DDSCAPS_MIPMAP|DDSCAPS_TEXTURE|DDSCAPS_COMPLEX
  DWORD           dwCaps2;  0
  DWORD           dwCaps3;  0
  DWORD           dwCaps4;  0
  DWORD           dwReserved2; 0 0
*/


	dds.dwSize = sizeof( DDS_HEADER );
	dds.ddspf.dwSize = sizeof( DDS_PIXELFORMAT );
	dds.dwFlags = DDSD_HEIGHT|DDSD_WIDTH|DDSD_PITCH|DDSD_PIXELFORMAT|DDSD_CAPS;
	dds.dwHeight = height;
	dds.dwWidth = width;
	dds.dwPitchOrLinearSize = 2 * width; // bytes per line
	dds.dwDepth = 1;  // depth of a volume texture
	dds.dwMipMapCount = 1; // mip maps in texture
	dds.dwCaps = DDSCAPS_TEXTURE;
	dds.dwCaps2 = 0;
	dds.dwCaps3 = 0; // unused
	dds.dwCaps4 = 0; // unused
	dds.ddspf.dwFlags = DDPF_LUMINANCE;
	dds.ddspf.dwFourCC = 0; // D3DFMT_G16R16                        
	//dds.ddspf.dwFourCC = 36; // D3DFMT_A16B16G16R16         
	//dds.ddspf.dwFourCC = 111; // D3DFMT_R16F
	dds.ddspf.dwRGBBitCount = 16; // Number of bits in an RGB (possibly including alpha) format. Valid when dwFlags includes DDPF_RGB, DDPF_LUMINANCE, or DDPF_YUV.
	dds.ddspf.dwRBitMask = 0xFFFF; // Red (or lumiannce or Y) mask for reading color data. For instance, given the A8R8G8B8 format, the red mask would be 0x00ff0000.
	dds.ddspf.dwGBitMask = 0x00; // Green (or U) mask for reading color data. For instance, given the A8R8G8B8 format, the green mask would be 0x0000ff00.
	dds.ddspf.dwBBitMask = 0x00; // Blue (or V) mask for reading color data. For instance, given the A8R8G8B8 format, the blue mask would be 0x000000ff.
	dds.ddspf.dwABitMask = 0x00; // Alpha mask for reading alpha data. dwFlags must include DDPF_ALPHAPIXELS or DDPF_ALPHA. For instance, given the A8R8G8B8 format, the alpha mask would be 0xff000000.
	file = sack_fopen( 0, filename, "wb" );
	sack_fwrite( "DDS ", 1, 4, file );
	sack_fwrite( &dds, 1, sizeof( DDS_HEADER ), file );
	{
		int r, c;
		for( r = 0; r < height; r++ )
			for( c = 0; c < width; c++ )
			{
				float f = data[ r * pitch + c ];
				S_32 value_x = (_64)((f * 65535));
				_16 value ;
				if( value_x > 65535 )
					value = 65535;
				else if( value_x < 0 )
					value = 0;
				else
					value = value_x;
				//_16 value = convert( f );
				sack_fwrite( &value, 1, 2, file );
			}
	}
	sack_fclose( file );
}

