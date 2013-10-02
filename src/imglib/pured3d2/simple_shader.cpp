#define FIX_RELEASE_COM_COLLISION
#include <stdhdrs.h>

#include "local.h"

#include "shaders.h"

IMAGE_NAMESPACE

//const char *gles_
	static char const * gles_simple_v_shader[]  = {

   "struct VS_INPUT\n"
   "{\n"
   "    float4 vPosition : POSITION;\n"
   "};\n"
   "\n"
   "struct VS_OUTPUT\n"
   "{\n"
   "    float4  vPosition : POSITION;\n"
   "};\n"
   "\n"
   "float4x4 mWld1 : register(c0);\n"
   "float4x4 mWld2 : register(c4);\n"
   "float4x4 mWld3 : register(c8);\n"
   "float4x4 mWld4 : register(c12);\n"
   "\n"
   "VS_OUTPUT main(VS_INPUT v)\n"
   "{\n"
   "    VS_OUTPUT vout;\n"
   "\n"
   "    // Skin position (to world space)\n"
   "    float4 vPosition = \n"
   "        mul( (mWld1 * mWld2 /** mWld3*/), v.vPosition );\n"
   "    \n"
   "    // Output stuff\n"
   "    vout.vPosition    = vPosition;\n"
   "\n"
   "    return vout;\n"
			"}\n"
	};


static char const * gles_simple_p_shader[] = {
   "float4  vDiffuse : register( c16 );\n"
   "\n"
   "struct PS_OUTPUT\n"
   "{\n"
   "    float4 Color : COLOR0;\n"
   "};\n"
   "\n"
   "PS_OUTPUT main( void)\n"
   "{\n"
   "    PS_OUTPUT pout;\n"
   "    pout.Color = vDiffuse;\n"
   "    return pout;\n"
		"}\n"
};



void CPROC EnableSimpleShader( PImageShaderTracker tracker, PTRSZVAL psv, va_list args )
{
	IDirect3DVertexBuffer9  *verts = va_arg( args, IDirect3DVertexBuffer9 * );
	float *color = va_arg( args, float * );

	g_d3d_device->SetStreamSource( 0, verts, 0, sizeof( float ) * 3 );
	g_d3d_device->SetVertexShaderConstantF( 16, color, 1 );
}

void InitSuperSimpleShader( PImageShaderTracker tracker )
{
	D3DVERTEXELEMENT9 decl[] = {{0,
                             0,
                             D3DDECLTYPE_FLOAT3,
                             D3DDECLMETHOD_DEFAULT,
                             D3DDECLUSAGE_POSITION,
                             0}
                            , D3DDECL_END()};
	g_d3d_device->CreateVertexDeclaration(decl, &tracker->vertexDecl);

	if( CompileShader( tracker, gles_simple_v_shader, 1
		, gles_simple_p_shader, 1 ) )
	{
		SetShaderEnable( tracker, EnableSimpleShader, 0 );
	}
}
IMAGE_NAMESPACE_END
