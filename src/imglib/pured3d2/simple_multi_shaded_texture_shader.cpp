#define FIX_RELEASE_COM_COLLISION
#include <stdhdrs.h>


#include "local.h"

#include "shaders.h"

IMAGE_NAMESPACE

//const char *gles_
static const CTEXTSTR gles_simple_v_multi_shader =
   WIDE( "struct VS_INPUT\n" )
   WIDE( "{\n" )
   WIDE( "    float4 vPosition : POSITION;\n" )
   WIDE( "    float4 vColor : COLOR0;\n" )
   WIDE( "    float2 Texture    : TEXCOORD0;\n" )
   WIDE( "};\n" )
   WIDE( "\n" )
   WIDE( "struct VS_OUTPUT\n" )
   WIDE( "{\n" )
   WIDE( "    float4  vPosition : POSITION;\n" )
   WIDE( "    float4  vDiffuse : COLOR0;\n" )
   WIDE( "    float2 Texture    : TEXCOORD0;\n" )
   WIDE( "};\n" )
   WIDE( "\n" )
   WIDE( "float4x4 mWld1;\n" )
   WIDE( "float4x4 mWld2;\n" )
   WIDE( "float4x4 mWld3;\n" )
   WIDE( "float4x4 mWld4;\n" )
   WIDE( "\n" )
   WIDE( "VS_OUTPUT main(VS_INPUT v)\n" )
   WIDE( "{\n" )
   WIDE( "    VS_OUTPUT vout;\n" )
   WIDE( "\n" )
   WIDE( "    // Skin position (to world space)\n" )
   WIDE( "    float4 vPosition = \n" )
   WIDE( "        mul(v.vPosition, mWld1) +\n" )
   WIDE( "        mul(v.vPosition, mWld2) +\n" )
   WIDE( "        mul(v.vPosition, mWld3) +\n" )
   WIDE( "        mul(v.vPosition, mWld4) ;\n" )
   WIDE( "    \n" )
   WIDE( "    // Output stuff\n" )
   WIDE( "    vout.vPosition    = vPosition;\n" )
   WIDE( "    vout.Texture = v.Texture;\n" )
   WIDE( "    vout.vDiffuse  = v.vColor;\n" )
   WIDE( "\n" )
   WIDE( "    return vout;\n" )
   WIDE( "}\n" );

static const CTEXTSTR gles_simple_p_multi_shader =
	WIDE( "sampler2D Tex0;\n" )
       WIDE( "float4 multishade_r;\n" )
       WIDE( "float4 multishade_g;\n" )
       WIDE( "float4 multishade_b;\n" )
   WIDE( "struct PS_INPUT\n" )
   WIDE( "{\n" )
   WIDE( "    float4  vDiffuse : COLOR0;\n" )
   WIDE( "    float2 Texture    : TEXCOORD0;\n" )
   WIDE( "};\n" )
   WIDE( "\n" )
   WIDE( "struct PS_OUTPUT\n" )
   WIDE( "{\n" )
   WIDE( "    float4 Color : COLOR0;\n" )
   WIDE( "};\n" )
   WIDE( "\n" )
   WIDE( "PS_OUTPUT main( PS_INPUT v )\n" )
   WIDE( "{\n" )
   WIDE( "    PS_OUTPUT pout;\n" )
   WIDE( "    pout.Color = tex2D(Tex0, v.Texture)*v.vDiffuse;\n" )
       WIDE( "    float4 color = tex2D(Tex0, v.Texture);\n" )
       WIDE( "	pout.Color = float4( (color.b * multishade_b.r) + (color.g * multishade_g.r) + (color.r * multishade_r.r)\n" )
       WIDE( "		,(color.b * multishade_b.g) + (color.g * multishade_g.g) + (color.r * multishade_r.g)\n" )
       WIDE( "		,(color.b * multishade_b.b) + (color.g * multishade_g.b) + (color.r * multishade_r.b)\n" )
       WIDE( "		,color.r!=0.0?( color.a * multishade_r.a) :0.0\n" )
       WIDE( "                + color.g!=0.0?( color.a * multishade_g.a) :0.0\n" )
       WIDE( "                + color.b!=0.0?( color.a * multishade_b.a) :0.0\n" )
       WIDE( "                );\n" )
   WIDE( "    return pout;\n" )
   WIDE( "}\n" );



struct private_mst_shader_data
{
	int texture_attrib;
	int texture;
	int r_color_attrib;
	int g_color_attrib;
	int b_color_attrib;
};


static void CPROC SimpleMultiShadedTextureEnable( PImageShaderTracker tracker, PTRSZVAL psv, va_list args )
{
	IDirect3DVertexBuffer9 *verts = va_arg( args, IDirect3DVertexBuffer9 *);
	Image texture = va_arg( args, Image );
	float *r_color = va_arg( args, float *);
	float *g_color = va_arg( args, float *);
	float *b_color = va_arg( args, float *);

	struct private_mst_shader_data *data = (struct private_mst_shader_data *)tracker->psv_userdata;

	g_d3d_device->SetStreamSource( 0, verts, 0, sizeof( float ) * 5 );
	g_d3d_device->SetVertexShaderConstantF( 16, r_color, 1 );
	g_d3d_device->SetVertexShaderConstantF( 20, g_color, 1 );
	g_d3d_device->SetVertexShaderConstantF( 24, b_color, 1 );
	ReloadD3DTexture( texture, 0 );
	g_d3d_device->SetTexture( 0, texture->pActiveSurface );

}


void InitSimpleMultiShadedTextureShader( PImageShaderTracker tracker )
{
	int result;
	struct private_mst_shader_data *data = New( struct private_mst_shader_data );
	struct image_shader_attribute_order attribs[] = { { 0, "vPosition" }, { 1, "in_TexCoord" } };

	D3DVERTEXELEMENT9 decl[] = {{0,
                             0,
                             D3DDECLTYPE_FLOAT3,
                             D3DDECLMETHOD_DEFAULT,
                             D3DDECLUSAGE_POSITION,
                             0},
	                        {0,
                             28,
                             D3DDECLTYPE_FLOAT2,
                             D3DDECLMETHOD_DEFAULT,
                             D3DDECLUSAGE_TEXCOORD,
                             0},
                            D3DDECL_END()};
	g_d3d_device->CreateVertexDeclaration(decl, &tracker->vertexDecl);

	SetShaderEnable( tracker, SimpleMultiShadedTextureEnable, (PTRSZVAL)data );

	if( result = 0 )
	{
		lprintf( "unhandled error before shader" );
	}

	if( CompileShaderEx( tracker, gles_simple_v_multi_shader, StrLen(gles_simple_v_multi_shader)
		, gles_simple_p_multi_shader, StrLen(gles_simple_p_multi_shader), attribs, 2 ) )
	{
		//data->r_color_attrib = glGetUniformLocation(tracker->glProgramId, "multishade_r" );
		//data->g_color_attrib = glGetUniformLocation(tracker->glProgramId, "multishade_g" );
		//data->b_color_attrib = glGetUniformLocation(tracker->glProgramId, "multishade_b" );
		//data->texture = glGetUniformLocation(tracker->glProgramId, "tex");
		//data->texture_attrib =  glGetAttribLocation(tracker->glProgramId, "in_texCoord" );
	}
}
IMAGE_NAMESPACE_END

