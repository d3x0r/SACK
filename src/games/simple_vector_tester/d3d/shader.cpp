
// Source mostly from
//  http://www.opengl.org/wiki/Tutorial:_OpenGL_3.1_The_First_Triangle_(C%2B%2B/Win)
//  ( heh well that's what it used to be, it's now the same vertexes, but DX11
//   ported as a SACK plugin module.
//  Updated to support modelview projection...
#define FIX_RELEASE_COM_COLLISION
#define MAKE_RCOORD_SINGLE
#define NO_FILEOP_ALIAS
#include <stdhdrs.h>
//#define USE_RENDER_INTERFACE l.pri
//#define USE_IMAGE_INTERFACE l.pii
#define _D3D11_DRIVER
#define USE_RENDER3D_INTERFACE l.pr3i
#define USE_IMAGE_3D_INTERFACE l.pi3i
#define NEED_VECTLIB_COMPARE

// define local instance.
#define TERRAIN_MAIN_SOURCE  
#include <vectlib.h>
#include <render.h>
#include <render3d.h>
#include <image3d.h>
#include <d3d11.h>

#include "local.h"




void InitPerspective( void )
{
	RCOORD n = 1;  // near
	RCOORD f = 30000;  // far	
	RCOORD lt = -1;
	RCOORD r = 1;
	RCOORD b = -1;
	RCOORD t = 1;

	l.projection[0] = ( 2*n / (r-lt));
	l.projection[4] = 0;
	l.projection[8] = (r+lt)/(r-lt);
	l.projection[12] = 0;

	l.projection[1] = 0;
	l.projection[5] = ( 2*n )/(t-b);
	l.projection[9] =  (t+b)/(t-b);
	l.projection[13] = 0;

	l.projection[2] = 0;
	l.projection[6] = 0;
	l.projection[10] = -(f+n)/(f-n);
	l.projection[14] = -(2*f*n)/(f-n);

	l.projection[3] = 0;
	l.projection[7] = 0;
	l.projection[11] = -1;
	l.projection[15] = 0;

}

void InitShader( PImageShaderTracker tracker, PTRSZVAL psv_old )
{
	const char *simple_color_vertex_source = 
		"struct VS_INPUT { float4 pos : POSITION; float4 in_Color: COLOR0; };\n"
	 	"struct VS_OUTPUT { float4 pos : POSITION; float4 ex_Color: COLOR0; };\n"
		"VS_OUTPUT main(VS_INPUT v)\n"
		"{\n"
		"   VS_OUTPUT vout;\n"
		"	vout.ex_Color = v.in_Color;\n"
		"   vout.pos = v.pos;\n"
		"	return vout;\n"
		"}\n";

	const char *simple_color_pixel_source = 
			"struct PS_INPUT\n"
			"{ \n"
			"   float4 vDiffuse: Diffuse;\n"
			"};\n"
			"\n"
			"struct PS_OUTPUT\n"
			"   {\n"
			"       float4 Color : SV_Target0;\n"
			"   };\n"
			"   \n"
			"   PS_OUTPUT main( PS_INPUT input )\n"
			"   {\n"
			"       PS_OUTPUT pout;\n"
			 "      pout.Color = input.vDiffuse;\n"
			"       return pout;\n"
			"	}\n";
	struct image_shader_attribute_order input_order[2];

	input_order[0].n = 0;
	input_order[0].name = "POSITION";
	input_order[0].size = sizeof( float ) * 3;
	input_order[0].format = DXGI_FORMAT_R32G32B32_FLOAT;
	input_order[0].input_class = D3D11_INPUT_PER_VERTEX_DATA;

	input_order[1].n = 1;
	input_order[1].name = "COLOR";
	input_order[1].size = sizeof( float ) * 4;
	input_order[1].format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	input_order[1].input_class = D3D11_INPUT_PER_VERTEX_DATA;

	if( ImageCompileShaderEx( tracker, &simple_color_vertex_source, 1, &simple_color_pixel_source, 1
		, input_order, 2 ) )
	{
		// this is simple, and all information is in the vertex buffer
		//ImageSetShaderEnable( tracker, NULL );
	}
}


struct vertex 
{
	float vert[3];
	float col[4];
};

static void OnDraw3d( WIDE("Simple Shader Array") )( PTRSZVAL psvView )
{
	int result;
	{
			static ID3D11Buffer *pQuadVB;
			if( !pQuadVB )
			{
				D3D11_BUFFER_DESC bufferDesc;
				bufferDesc.Usage            = D3D11_USAGE_DYNAMIC;
				bufferDesc.ByteWidth        = sizeof( struct vertex ) * 6;
				bufferDesc.BindFlags        = D3D11_BIND_VERTEX_BUFFER;
				bufferDesc.CPUAccessFlags   = D3D11_CPU_ACCESS_WRITE;
				bufferDesc.MiscFlags        = 0;
				bufferDesc.StructureByteStride = sizeof( struct vertex );
	
				g_d3d_device->CreateBuffer( &bufferDesc, NULL/*&InitData*/, &pQuadVB);
			}

			D3D11_MAPPED_SUBRESOURCE resource;
			g_d3d_device_context->Map( pQuadVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource );
			struct vertex *pData;
			pData = (struct vertex*)resource.pData;

		{
			// First simple object
			
			pData[0].vert[0] =-0.3; pData[0].vert[1] = 0.5; pData[0].vert[2] =-1.0;
			pData[1].vert[0] =-0.8; pData[1].vert[1] =-0.5; pData[1].vert[2] =-1.0;
			pData[2].vert[0] = 0.2; pData[2].vert[1] =-0.5; pData[2].vert[2]= -1.0;

			pData[0].col[0] = 1.0; pData[0].col[1] = 0.0; pData[0].col[2] = 0.0;
			pData[1].col[0] = 0.0; pData[1].col[1] = 1.0; pData[1].col[2] = 0.0;
			pData[2].col[0] = 0.0; pData[2].col[1] = 0.0; pData[1].col[2] = 1.0;

			pData[0].col[3] = 1.0; 
			pData[1].col[3] = 1.0; 
			pData[2].col[4] = 1.0;

			g_d3d_device_context->Unmap( pQuadVB, 0 );

			ImageEnableShader( l.shader );
			unsigned int stride = sizeof( struct vertex );
			unsigned int offset = 0;
			g_d3d_device_context->IASetVertexBuffers(0, 1, &pQuadVB, &stride, &offset);
			g_d3d_device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			g_d3d_device_context->Draw( 3, 0 );

			//------------------------ Triangle 2 --------------------------------

			g_d3d_device_context->Map( pQuadVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource );
			pData = (struct vertex*)resource.pData;
			// Second simple object

			pData[0].vert[0] =-0.2; pData[0].vert[1] = 0.5; pData[0].vert[2] =-1.0;
			pData[1].vert[0] = 0.3; pData[1].vert[1] =-0.5; pData[1].vert[2] =-1.0;
			pData[2].vert[0] = 0.8; pData[2].vert[1] = 0.5; pData[2].vert[2]= -1.0;
			pData[0].col[0] = 1.0; pData[0].col[1] = 0.0; pData[0].col[2] = 0.0;
			pData[1].col[0] = 0.0; pData[1].col[1] = 1.0; pData[1].col[2] = 0.0;
			pData[2].col[0] = 0.0; pData[2].col[1] = 0.0; pData[1].col[2] = 1.0;

			pData[0].col[3] = 1.0; 
			pData[1].col[3] = 1.0; 
			pData[2].col[4] = 1.0;

			g_d3d_device_context->Unmap( pQuadVB, 0 );

			g_d3d_device_context->IASetVertexBuffers(0, 1, &pQuadVB, &stride, &offset);
			ImageEnableShader( l.shader );

			g_d3d_device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			g_d3d_device_context->Draw( 3, 0 );

			{
				int n;
				for( n = 0; n < 9; n++ )
				{
					//vert[n] = 10* vert[n];
					//vert2[n] = 10* vert2[n];
				}
			}
		}

	}
}

static void OnBeginDraw3d( WIDE( "Simple Shader Array" ) )( PTRSZVAL psv,PTRANSFORM camera )
{

}

static void OnFirstDraw3d( WIDE( "Simple Shader Array" ) )( PTRSZVAL psvInit )
{
	l.shader = ImageGetShader(	WIDE("test_d3d_shader"), InitShader );

}

static PTRSZVAL OnInit3d( WIDE( "Simple Shader Array" ) )( PMatrix projection, PTRANSFORM camera, RCOORD *identity_depth, RCOORD *aspect )
{
	l.pi3i = GetImage3dInterface();
	l.pr3i = GetRender3dInterface();
	// keep the camera as a 
	return (PTRSZVAL)camera;
}

