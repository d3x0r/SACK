#define USE_RENDER3D_INTERFACE l.pr3di

#include <render3d.h>
#include <imglib/imagestruct.h>
#include <imglib/fontstruct.h>

IMAGE_NAMESPACE

struct d3dSurfaceData {
	PTRANSFORM T_Camera;
   RCOORD *aspect;
	RCOORD *identity_depth;
	int index;
};

//-------------- register order mappings for FVF vertices
// D3DVSDE_POSITION      D3DFVF_XYZ  v0
// D3DVSDE_BLENDWEIGHT   D3DFVF_XYZRHW v1
// D3DVSDE_BLENDINDICES  D3DFVF_XYZB1-5 V2
// D3DVSDE_NORMAL        D3DFVF_NORMAL v3
// D3DVSDE_PSIZE         D3DFVF_PSIZE  v4
// D3DVSDE_DIFFUSE       D3DFVF_DIFFUSE v5
// D3DVSDE_SPECULAR      D3DFVF_SPECULAR v6
// D3DVSDE_TEXCOORD0-7   D3DFVF_TEX1-8  v7-v14
// D3DVSDE_POSITION2     v15
// D3DVSDE_NORMAL2       v16






// these two go together...
// the structure for points we're using and the type of color spec (or normals or lots of other good things)
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE)
struct D3DVERTEX
{
float fX,
      fY,
      fZ;
DWORD dwColor;
};
#define D3DFVF_CUSTOMTEXTUREDVERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE| D3DFVF_TEX1)
struct D3DTEXTUREDVERTEX
{
float fX,
      fY,
      fZ;
DWORD dwColor;
float fU1, //first tex coords
      fV1;
};

struct d3dSurfaceImageData {
	struct {
		BIT_FIELD updated : 1;
	} flags;
	LPDIRECT3DTEXTURE9 d3tex;
	IDirect3DBaseTexture9 *d3dTexture;
};

#ifndef IMAGE_MAIN
extern
#endif
struct {
	struct {
		int multi_shader;
		struct {
			BIT_FIELD init_ok : 1;
			BIT_FIELD shader_ok : 1;
		} flags;
	} shader;
	PTREEROOT shade_cache;
	int glImageIndex;
	PLIST d3dSurfaces; // list of struct glSurfaceData *
	struct d3dSurfaceData *d3dActiveSurface;
	PRENDER3D_INTERFACE pr3di;
} local_image_data;
#define l local_image_data

struct shade_cache_element {
	CDATA r,grn,b;
	Image image;
	_32 age;
};

struct shade_cache_image
{
	PLIST elements;
	Image image;
};

// use this if opengl shaders are missing.
Image GetShadedImage( Image image, CDATA red, CDATA green, CDATA blue );


void InitShader( void );
 
Image AllocateCharacterSpaceByFont( SFTFont font, PCHARACTER character );
IDirect3DBaseTexture9 *ReloadD3DTexture( Image image, int option );
void TranslateCoord( Image image, S_32 *x, S_32 *y );
struct d3dSurfaceImageData * MarkImageUpdated( Image image );

IMAGE_NAMESPACE_END

