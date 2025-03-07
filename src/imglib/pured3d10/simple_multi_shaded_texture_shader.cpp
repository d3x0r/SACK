#define FIX_RELEASE_COM_COLLISION
#include <stdhdrs.h>


#include "local.h"

#include "shaders.h"

IMAGE_NAMESPACE

//const char *gles_
static char *const gles_simple_v_multi_shader[] = {
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
		"}\n" };

static char *const gles_simple_p_multi_shader[] = {
	"sampler2D Tex0;\n"
       "float4 multishade_r;\n"
       "float4 multishade_g;\n"
       "float4 multishade_b;\n"
   "struct PS_INPUT\n"
   "{\n"
   "    float4  vDiffuse : COLOR0;\n"
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
   "    pout.Color = tex2D(Tex0, v.Texture)*v.vDiffuse;\n"
       "    float4 color = tex2D(Tex0, v.Texture);\n"
       "	pout.Color = float4( (color.b * multishade_b.r) + (color.g * multishade_g.r) + (color.r * multishade_r.r)\n"
       "		,(color.b * multishade_b.g) + (color.g * multishade_g.g) + (color.r * multishade_r.g)\n"
       "		,(color.b * multishade_b.b) + (color.g * multishade_g.b) + (color.r * multishade_r.b)\n"
       "		,color.r!=0.0?( color.a * multishade_r.a) :0.0\n"
       "                + color.g!=0.0?( color.a * multishade_g.a) :0.0\n"
       "                + color.b!=0.0?( color.a * multishade_b.a) :0.0\n"
       "                );\n"
   "    return pout;\n"
		"}\n" };



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
	ID3D10Buffer *verts = va_arg( args, ID3D10Buffer *);
	Image texture = va_arg( args, Image );
	float *r_color = va_arg( args, float *);
	float *g_color = va_arg( args, float *);
	float *b_color = va_arg( args, float *);

	struct private_mst_shader_data *data = (struct private_mst_shader_data *)tracker->psv_userdata;

	//g_d3d_device->SetStreamSource( 0, verts, 0, sizeof( float ) * 5 );
	//g_d3d_device->SetVertexShaderConstantF( 16, r_color, 1 );
	//g_d3d_device->SetVertexShaderConstantF( 20, g_color, 1 );
	//g_d3d_device->SetVertexShaderConstantF( 24, b_color, 1 );
	ReloadD3DTexture( texture, 0 );
	//g_d3d_device->SetTexture( 0, texture->pActiveSurface );

}


void InitSimpleMultiShadedTextureShader( PImageShaderTracker tracker )
{
	int result;
	struct private_mst_shader_data *data = New( struct private_mst_shader_data );
	struct image_shader_attribute_order attribs[] = { { 0, "vPosition" }, { 1, "in_TexCoord" } };
#if 0
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
#endif
	//g_d3d_device->CreateVertexDeclaration(decl, &tracker->vertexDecl);

	SetShaderEnable( tracker, SimpleMultiShadedTextureEnable, (PTRSZVAL)data );

	if( result = 0 )
	{
		lprintf( "unhandled error before shader" );
	}

	if( CompileShaderEx( tracker, gles_simple_v_multi_shader, 1
		, gles_simple_p_multi_shader, 1, attribs, 2 ) )
	{
		//data->r_color_attrib = glGetUniformLocation(tracker->glProgramId, "multishade_r" );
		//data->g_color_attrib = glGetUniformLocation(tracker->glProgramId, "multishade_g" );
		//data->b_color_attrib = glGetUniformLocation(tracker->glProgramId, "multishade_b" );
		//data->texture = glGetUniformLocation(tracker->glProgramId, "tex");
		//data->texture_attrib =  glGetAttribLocation(tracker->glProgramId, "in_texCoord" );
	}
}
IMAGE_NAMESPACE_END

