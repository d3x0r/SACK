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
   "cbuffer globals {\n"
   "  float4x4 mWld1;\n"
   "  float4x4 mWld2;\n"
   " // float4x4 mWld3;\n"
   " // float4x4 mWld4;\n"
   "};\n"
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
	"cbuffer globals {float4  vDiffuse; };\n"
   "\n"
   "struct PS_OUTPUT\n"
   "{\n"
   "    float4 Color: SV_Target0;\n"
   "};\n"
   "\n"
   "PS_OUTPUT main( void)\n"
   "{\n"
   "    PS_OUTPUT pout;\n"
   "    pout.Color = vDiffuse;\n"
   "    return pout;\n"
		"}\n"
};

struct SimpleShaderData 
{
	float _color[4]; // prior color; to avoid mapping and setting the color without a change
};
struct vertex_constant_data
{
	float wld1[16];
	float wld2[16];
	//float wld3[16];
	//float wld4[16];
};

struct frag_constant_data
{
	float color[4];
};


void CPROC EnableSimpleShader( PImageShaderTracker tracker, PTRSZVAL psv, va_list args )
{
	float *color = va_arg( args, float * );
	D3D11_MAPPED_SUBRESOURCE vconst;
	D3D11_MAPPED_SUBRESOURCE fconst;

	if( !tracker->flags.set_matrix )
	{
		g_d3d_device_context->Map( tracker->vertex_constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &vconst );
	
		if( !l.flags.worldview_read )
		{
			GetGLCameraMatrix( l.d3dActiveSurface->T_Camera, l.worldview );
			l.flags.worldview_read = 1;
		}

		PrintMatrix( (MATRIX)l.worldview );
		PrintMatrix( l.d3dActiveSurface->M_Projection[0] );
		//mWld2
		MemCpy( ((struct vertex_constant_data *)vconst.pData)->wld1, l.d3dActiveSurface->M_Projection[0] , sizeof( float ) * 16 );
		MemCpy( ((struct vertex_constant_data *)vconst.pData)->wld2, l.worldview, sizeof( float ) * 16 );
				
		//mWld1
		//g_d3d_device->SetVertexShaderConstantF( 0, (float*)l.d3dActiveSurface->M_Projection, 4 );
		g_d3d_device_context->Unmap( tracker->vertex_constant_buffer, 0 );

		tracker->flags.set_matrix = 1;
	}

	struct SimpleShaderData *extra_data
		= (SimpleShaderData*)tracker->psv_userdata;

	if( MemCmp( color, extra_data->_color, sizeof( extra_data->_color ) ) )
	{
		g_d3d_device_context->Map( tracker->fragment_constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &fconst );
		MemCpy( ((struct frag_constant_data *)fconst.pData)->color, color, sizeof( float ) * 4 );
		MemCpy( extra_data->_color, color, sizeof( float ) * 4 );
		g_d3d_device_context->Unmap( tracker->fragment_constant_buffer, 0 );
	}

}

void InitSuperSimpleShader( PImageShaderTracker tracker )
{
	struct SimpleShaderData *extra_data;
	if( !tracker->psv_userdata )
	{
		extra_data = New( SimpleShaderData );
	}
	else
		extra_data = (SimpleShaderData*)tracker->psv_userdata;

	if( CompileShader( tracker, gles_simple_v_shader, 1
		, gles_simple_p_shader, 1 ) )
	{
		ID3D11Buffer * buffer;
		D3D11_BUFFER_DESC bufferDesc;
		bufferDesc.Usage            = D3D11_USAGE_DYNAMIC;
		bufferDesc.ByteWidth        = sizeof( struct vertex_constant_data );
		bufferDesc.BindFlags        = D3D11_BIND_CONSTANT_BUFFER;
		bufferDesc.CPUAccessFlags   = D3D11_CPU_ACCESS_WRITE;
		bufferDesc.MiscFlags        = 0;
		bufferDesc.StructureByteStride = sizeof( struct vertex_constant_data );

		g_d3d_device->CreateBuffer( &bufferDesc, NULL, &tracker->vertex_constant_buffer );

		bufferDesc.ByteWidth        = sizeof( struct frag_constant_data );
		bufferDesc.StructureByteStride = sizeof( struct frag_constant_data );

		g_d3d_device->CreateBuffer( &bufferDesc, NULL, &tracker->fragment_constant_buffer );


		SetShaderEnable( tracker, EnableSimpleShader, (PTRSZVAL)extra_data );
	}
}
IMAGE_NAMESPACE_END
