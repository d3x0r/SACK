#define FIX_RELEASE_COM_COLLISION
#include <stdhdrs.h>

#include "local.h"

#include "shaders.h"

IMAGE_NAMESPACE

//const char *gles_
static const CTEXTSTR gles_simple_v_shader =

   WIDE( "struct VS_INPUT\n" )
   WIDE( "{\n" )
   WIDE( "    float4 vPosition : POSITION;\n" )
   WIDE( "};\n" )
   WIDE( "\n" )
   WIDE( "struct VS_OUTPUT\n" )
   WIDE( "{\n" )
   WIDE( "    float4  vPosition : POSITION;\n" )
   WIDE( "};\n" )
   WIDE( "\n" )
   WIDE( "float4x4 mWld1 : register(c0);\n" )
   WIDE( "float4x4 mWld2 : register(c4);\n" )
   WIDE( "float4x4 mWld3 : register(c8);\n" )
   WIDE( "float4x4 mWld4 : register(c12);\n" )
   WIDE( "\n" )
   WIDE( "VS_OUTPUT main(VS_INPUT v)\n" )
   WIDE( "{\n" )
   WIDE( "    VS_OUTPUT vout;\n" )
   WIDE( "\n" )
   WIDE( "    // Skin position (to world space)\n" )
   WIDE( "    float4 vPosition = \n" )
   WIDE( "        mul( (mWld1 * mWld2 /** mWld3*/), v.vPosition );\n" )
   WIDE( "    \n" )
   WIDE( "    // Output stuff\n" )
   WIDE( "    vout.vPosition    = vPosition;\n" )
   WIDE( "\n" )
   WIDE( "    return vout;\n" )
   WIDE( "}\n" );


static const CTEXTSTR gles_simple_p_shader =
   WIDE( "float4  vDiffuse : register( c16 );\n" )
   WIDE( "\n" )
   WIDE( "struct PS_OUTPUT\n" )
   WIDE( "{\n" )
   WIDE( "    float4 Color : COLOR0;\n" )
   WIDE( "};\n" )
   WIDE( "\n" )
   WIDE( "PS_OUTPUT main( void)\n" )
   WIDE( "{\n" )
   WIDE( "    PS_OUTPUT pout;\n" )
   WIDE( "    pout.Color = vDiffuse;\n" )
   WIDE( "    return pout;\n" )
   WIDE( "}\n" );



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

	if( CompileShader( tracker, gles_simple_v_shader, StrLen( gles_simple_v_shader )
		, gles_simple_p_shader, StrLen( gles_simple_p_shader ) ) )
	{
		SetShaderEnable( tracker, EnableSimpleShader, 0 );
	}
}
IMAGE_NAMESPACE_END
