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
	float *color = va_arg( args, float * );

	//g_d3d_device->SetStreamSource( 0, verts, 0, sizeof( float ) * 3 );
	//g_d3d_device_context->SetVertexShaderConstantF( 16, color, 1 );
}

void InitSuperSimpleShader( PImageShaderTracker tracker )
{



	if( CompileShader( tracker, gles_simple_v_shader, 1
		, gles_simple_p_shader, 1 ) )
	{
		SetShaderEnable( tracker, EnableSimpleShader, 0 );
	}

#if 0
	if ( FAILED( D3DX10CreateEffectFromFile(    "basicEffect.fx",
                                            NULL,
                                            NULL,
                                            "fx_4_0",
                                            D3D10_SHADER_ENABLE_STRICTNESS,
                                            0,
                                            g_d3d_device,
                                            NULL, NULL,
                                            &tracker->effect,
                                            NULL, NULL  ) ) ) 
	{
		lprintf( WIDE("Could not load effect file!")); 
		return;// fatalError("Could not load effect file!");
	}
 
	ID3D10EffectTechnique *pBasicTechnique = tracker->effect->GetTechniqueByName("Render");
 
	//create matrix effect pointers
	ID3D10EffectVariable *pViewMatrixEffectVariable = tracker->effect->GetVariableByName( "View" )->AsMatrix();
	ID3D10EffectVariable *pProjectionMatrixEffectVariable = tracker->effect->GetVariableByName( "Projection" )->AsMatrix();
	ID3D10EffectVariable *pWorldMatrixEffectVariable = tracker->effect->GetVariableByName( "World" )->AsMatrix();

	D3D10_INPUT_ELEMENT_DESC layout[1] = 	{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 }
		};

	UINT numElements = 2;
	D3D10_PASS_DESC PassDesc;
	pBasicTechnique->GetPassByIndex( 0 )->GetDesc( &PassDesc );
  
	ID3D10InputLayout *pVertexLayout;
	if ( FAILED( g_d3d_device->CreateInputLayout( layout,
												numElements,
												PassDesc.pIAInputSignature,
												PassDesc.IAInputSignatureSize,
												&pVertexLayout ) ) ) 
	{
		lprintf( WIDE("Could not create Input Layout!"));
		return;
	}
 
	// Set the input layout
	g_d3d_device->IASetInputLayout( pVertexLayout );

	//g_d3d_device->CreateVertexDeclaration(decl, &tracker->vertexDecl);

#endif

}
IMAGE_NAMESPACE_END
