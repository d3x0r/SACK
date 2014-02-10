#define FIX_RELEASE_COM_COLLISION
#include <stdhdrs.h>

#include "local.h"

#include "shaders.h"

IMAGE_NAMESPACE

//const char *gles_
static const char * gles_simple_v_shader =
   "struct VS_INPUT\n"
   "{\n"
   "    float4 vPosition : POSITION;\n"
   "    float2 Texture    : TEXCOORD0;\n"
   "};\n"
   "\n"
   "struct VS_OUTPUT\n"
   "{\n"
   "    float4  vPosition : POSITION;\n"
   "    float2 Texture    : TEXCOORD0;\n"
   "};\n"
   "\n"
   "float4x4 mWld1;\n"
   "float4x4 mWld2;\n"
   "float4x4 mWld3;\n"
   "float4x4 mWld4;\n"
   "\n"
   "VS_OUTPUT main(VS_INPUT v)\n"
   "{\n"
   "    VS_OUTPUT vout;\n"
   "\n"
   "    // Skin position (to world space)\n"
   "    float4 vPosition = \n"
   "        mul(v.vPosition, mWld1) +\n"
   "        mul(v.vPosition, mWld2) +\n"
   "        mul(v.vPosition, mWld3) +\n"
   "        mul(v.vPosition, mWld4) ;\n"
   "    \n"
   "    // Output stuff\n"
   "    vout.vPosition    = vPosition;\n"
   "    vout.Texture = v.Texture;\n"
   "\n"
   "    return vout;\n"
   "}\n";

static const char *gles_simple_p_shader =
	"sampler2D Tex0;\n"
   "struct PS_INPUT\n"
   "{\n"
   "    float2 Texture    : TEXCOORD0;\n"
   "};\n"
   "\n"
   "struct PS_OUTPUT\n"
   "{\n"
   "    float4 Color : COLOR0;\n"
   "};\n"
   "\n"
   "PS_OUTPUT main( PS_INPUT v )\n"
   "{\n"
   "    PS_OUTPUT pout;\n"
   "    pout.Color = tex2D(Tex0, v.Texture);\n"
   "    return pout;\n"
   "}\n";


//const char *gles_
static const char *gles_simple_v_shader_shaded_texture =
   "struct VS_INPUT\n"
   "{\n"
   "    float4 vPosition : POSITION;\n"
   "    float4 vColor : COLOR0;\n"
   "    float2 Texture    : TEXCOORD0;\n"
   "};\n"
   "\n"
   "struct VS_OUTPUT\n"
   "{\n"
   "    float4  vPosition : POSITION;\n"
   "    float4  vDiffuse : COLOR0;\n"
   "    float2 Texture    : TEXCOORD0;\n"
   "};\n"
   "\n"
   "float4x4 mWld1;\n"
   "float4x4 mWld2;\n"
   "float4x4 mWld3;\n"
   "float4x4 mWld4;\n"
   "\n"
   "VS_OUTPUT main(VS_INPUT v)\n"
   "{\n"
   "    VS_OUTPUT vout;\n"
   "\n"
   "    // Skin position (to world space)\n"
   "    float4 vPosition = \n"
   "        mul(v.vPosition, mWld1) +\n"
   "        mul(v.vPosition, mWld2) +\n"
   "        mul(v.vPosition, mWld3) +\n"
   "        mul(v.vPosition, mWld4) ;\n"
   "    \n"
   "    // Output stuff\n"
   "    vout.vPosition    = vPosition;\n"
   "    vout.Texture = v.Texture;\n"
   "    vout.vDiffuse  = v.vColor;\n"
   "\n"
   "    return vout;\n"
   "}\n";


static const char *gles_simple_p_shader_shaded_texture =
	"cbuffer global{ Texture2D Tex0; };\n"
   "struct PS_INPUT\n"
   "{\n"
   "    float4  vDiffuse : SV_Target0;\n"
   "    float2 Texture    : TEXCOORD0;\n"
   "};\n"
   "\n"
   "struct PS_OUTPUT\n"
   "{\n"
   "    float4 Color : COLOR0;\n"
   "};\n"
   "\n"
   "SamplerState MeshTextureSampler\n"
   "{\n"
   "    Filter = MIN_MAG_MIP_LINEAR;\n"
   "    AddressU = Wrap;\n"
   "    AddressV = Wrap;\n"
   "};\n"
   "\n"
   "PS_OUTPUT main( PS_INPUT v )\n"
   "{\n"
   "    PS_OUTPUT pout;\n"
   "    pout.Color = Tex0.Sample(MeshTextureSampler,v.Texture)*v.vDiffuse;\n"
   "    return pout;\n"
   "}\n";


struct private_shader_data
{
	int texture_attrib;
	int texture;
};

static void CPROC SimpleTextureEnable( PImageShaderTracker tracker, PTRSZVAL psv_userdata, va_list args )
{
	ID3D10Buffer *verts = va_arg( args, ID3D10Buffer *);
	Image texture = va_arg( args, Image );
	struct private_shader_data *data = (struct private_shader_data *)psv_userdata;

	//g_d3d_device->SetStreamSource( 0, verts, 0, sizeof( float ) * 5 );
	ReloadD3DTexture( texture, 0 );
	//g_d3d_device->SetTexture( 0, texture->pActiveSurface );
}

void InitSimpleTextureShader( PImageShaderTracker tracker )
{
	const char *v_codeblocks[2];
	const char *p_codeblocks[2];
	struct private_shader_data *data = New( struct private_shader_data );
	struct image_shader_attribute_order attribs[] = { { 0, "vPosition" }, { 1, "in_TexCoord" } };
#if 0
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

   v_codeblocks[0] = gles_simple_v_shader;
	p_codeblocks[0] = gles_simple_p_shader;

	if( CompileShaderEx( tracker, v_codeblocks, 1
		, p_codeblocks, 1, attribs, 2 ) )
	{
		//data->texture = glGetUniformLocation(tracker->glProgramId, "tex");
		//data->texture_attrib =  glGetAttribLocation(tracker->glProgramId, "in_texCoord" );
		//lprintf( "texture is really %d", data->texture_attrib );
		//lprintf( "position is really %d", glGetAttribLocation(tracker->glProgramId, "vPosition" ) );
	}
#endif

}

static void CPROC SimpleTextureEnable2( PImageShaderTracker tracker, PTRSZVAL psv, va_list args )
{
	ID3D10Buffer *verts = va_arg( args, ID3D10Buffer *);
	Image texture = va_arg( args, Image );
	float *color = va_arg( args, float *);

	struct private_shader_data *data = (struct private_shader_data *)psv;

	//g_d3d_device->SetStreamSource( 0, verts, 0, sizeof( float ) * 5 );
	//g_d3d_device->SetVertexShaderConstantF( 16, color, 1 );
	ReloadD3DTexture( texture, 0 );
	//g_d3d_device->SetTexture( 0, texture->pActiveSurface );
}


void InitSimpleShadedTextureShader( PImageShaderTracker tracker )
{
	struct private_shader_data *data = New( struct private_shader_data );
	struct image_shader_attribute_order attribs[] = { 
		{ 0, "vPosition" }
		, { 1, "in_TexCoord" }
		, { 2, "in_Color" } 
	};
#if 0
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
#endif
	//g_d3d_device->CreateVertexDeclaration(decl, &tracker->vertexDecl);

	SetShaderEnable( tracker, SimpleTextureEnable2, (PTRSZVAL)data );

	const char *v_codeblocks[2];
	const char *p_codeblocks[2];
   v_codeblocks[0] = gles_simple_v_shader_shaded_texture;
	p_codeblocks[0] = gles_simple_p_shader_shaded_texture;
	if( CompileShaderEx( tracker, v_codeblocks, 1
		, p_codeblocks, 1, attribs, 3 ) )
	{
		//tracker->color_attrib = glGetUniformLocation(tracker->glProgramId, "in_Color" );
		//data->texture = glGetUniformLocation(tracker->glProgramId, "tex");
		//data->texture_attrib =  glGetAttribLocation(tracker->glProgramId, "in_texCoord" );
	}
}

IMAGE_NAMESPACE_END

