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
   WIDE( "    float2 Texture    : TEXCOORD0;\n" )
   WIDE( "};\n" )
   WIDE( "\n" )
   WIDE( "struct VS_OUTPUT\n" )
   WIDE( "{\n" )
   WIDE( "    float4  vPosition : POSITION;\n" )
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
   WIDE( "\n" )
   WIDE( "    return vout;\n" )
   WIDE( "}\n" );

static const CTEXTSTR gles_simple_p_shader =
	WIDE( "sampler2D Tex0;\n" )
   WIDE( "struct PS_INPUT\n" )
   WIDE( "{\n" )
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
   WIDE( "    pout.Color = tex2D(Tex0, v.Texture);\n" )
   WIDE( "    return pout;\n" )
   WIDE( "}\n" );


//const char *gles_
static const CTEXTSTR gles_simple_v_shader_shaded_texture =
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


static const CTEXTSTR gles_simple_p_shader_shaded_texture =
	WIDE( "sampler2D Tex0;\n" )
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
   WIDE( "    return pout;\n" )
   WIDE( "}\n" );


struct private_shader_data
{
	int texture_attrib;
	int texture;
};

static void CPROC SimpleTextureEnable( PImageShaderTracker tracker, PTRSZVAL psv_userdata, va_list args )
{
	IDirect3DVertexBuffer9 *verts = va_arg( args, IDirect3DVertexBuffer9 *);
	Image texture = va_arg( args, Image );
	struct private_shader_data *data = (struct private_shader_data *)psv_userdata;

	g_d3d_device->SetStreamSource( 0, verts, 0, sizeof( float ) * 5 );
	ReloadD3DTexture( texture, 0 );
	g_d3d_device->SetTexture( 0, texture->pActiveSurface );
}

void InitSimpleTextureShader( PImageShaderTracker tracker )
{
	const char *v_codeblocks[2];
	const char *p_codeblocks[2];
	struct private_shader_data *data = New( struct private_shader_data );
	struct image_shader_attribute_order attribs[] = { { 0, "vPosition" }, { 1, "in_TexCoord" } };
	D3DVERTEXELEMENT9 decl[] = {{0,
                             0,
                             D3DDECLTYPE_FLOAT3,
                             D3DDECLMETHOD_DEFAULT,
                             D3DDECLUSAGE_POSITION,
                             0},
                            {0,
                             12,
                             D3DDECLTYPE_FLOAT2,
                             D3DDECLMETHOD_DEFAULT,
                             D3DDECLUSAGE_TEXCOORD,
                             0},
                            D3DDECL_END()};
	g_d3d_device->CreateVertexDeclaration(decl, &tracker->vertexDecl);

	SetShaderEnable( tracker, SimpleTextureEnable, (PTRSZVAL)data );

	if( CompileShaderEx( tracker, gles_simple_v_shader, StrLen(gles_simple_v_shader)
		, gles_simple_p_shader, StrLen(gles_simple_p_shader), attribs, 2 ) )
	{
		//data->texture = glGetUniformLocation(tracker->glProgramId, "tex");
		//data->texture_attrib =  glGetAttribLocation(tracker->glProgramId, "in_texCoord" );
		//lprintf( "texture is really %d", data->texture_attrib );
		//lprintf( "position is really %d", glGetAttribLocation(tracker->glProgramId, "vPosition" ) );
	}

}

static void CPROC SimpleTextureEnable2( PImageShaderTracker tracker, PTRSZVAL psv, va_list args )
{
	IDirect3DVertexBuffer9 *verts = va_arg( args, IDirect3DVertexBuffer9 *);
	Image texture = va_arg( args, Image );
	float *color = va_arg( args, float *);

	struct private_shader_data *data = (struct private_shader_data *)psv;

	g_d3d_device->SetStreamSource( 0, verts, 0, sizeof( float ) * 5 );
	g_d3d_device->SetVertexShaderConstantF( 16, color, 1 );
	ReloadD3DTexture( texture, 0 );
	g_d3d_device->SetTexture( 0, texture->pActiveSurface );
}


void InitSimpleShadedTextureShader( PImageShaderTracker tracker )
{
	struct private_shader_data *data = New( struct private_shader_data );
	struct image_shader_attribute_order attribs[] = { { 0, "vPosition" }, { 1, "in_TexCoord" }, { 2, "in_Color" } };

	D3DVERTEXELEMENT9 decl[] = {{0,
                             0,
                             D3DDECLTYPE_FLOAT3,
                             D3DDECLMETHOD_DEFAULT,
                             D3DDECLUSAGE_POSITION,
                             0},
                            {0,
                             12,
                             D3DDECLTYPE_FLOAT2,
                             D3DDECLMETHOD_DEFAULT,
                             D3DDECLUSAGE_TEXCOORD,
                             0},
                            D3DDECL_END()};
	g_d3d_device->CreateVertexDeclaration(decl, &tracker->vertexDecl);

	SetShaderEnable( tracker, SimpleTextureEnable2, (PTRSZVAL)data );

	if( CompileShaderEx( tracker, gles_simple_v_shader_shaded_texture, StrLen(gles_simple_v_shader_shaded_texture)
		, gles_simple_p_shader_shaded_texture, StrLen(gles_simple_p_shader_shaded_texture), attribs, 3 ) )
	{
		//tracker->color_attrib = glGetUniformLocation(tracker->glProgramId, "in_Color" );
		//data->texture = glGetUniformLocation(tracker->glProgramId, "tex");
		//data->texture_attrib =  glGetAttribLocation(tracker->glProgramId, "in_texCoord" );
	}
}

IMAGE_NAMESPACE_END

