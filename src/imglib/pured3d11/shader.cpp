// dx11 http://www.rastertek.com/dx11tut04.html

#define FIX_RELEASE_COM_COLLISION

#include <stdhdrs.h>
#include <imglib/imagestruct.h>
#include <D3Dcompiler.h>
#include "local.h"
#include "shaders.h"

IMAGE_NAMESPACE

PImageShaderTracker GetShader( CTEXTSTR name, void (CPROC*Init)(PImageShaderTracker) )
{
	PImageShaderTracker tracker;
	INDEX idx;
	LIST_FORALL( l.d3dActiveSurface->shaders, idx, PImageShaderTracker, tracker )
	{
		if( StrCaseCmp( tracker->name, name ) == 0 )
			return tracker;
	}
	if( Init )
	{
		tracker = New( ImageShaderTracker );
		MemSet( tracker, 0, sizeof( ImageShaderTracker ));
		tracker->flags.set_modelview = 1; // claim it is set, so modelview gets cleared on first draw.
		tracker->name = StrDup( name );
		tracker->Init = Init;
		if( Init )
			Init( tracker );
		AddLink( &l.d3dActiveSurface->shaders, tracker );
		return tracker;
	}
	return NULL;
}

void  SetShaderEnable( PImageShaderTracker tracker, void (CPROC*EnableShader)( PImageShaderTracker tracker, PTRSZVAL,va_list args ), PTRSZVAL psv )
{
	tracker->psv_userdata = psv;
	tracker->Enable = EnableShader;
}


void CloseShaders( struct d3dSurfaceData *d3dSurface )
{
	PImageShaderTracker tracker;
	INDEX idx;
	LIST_FORALL( d3dSurface->shaders, idx, PImageShaderTracker, tracker )
	{
		tracker->flags.set_matrix = 0;
		// all other things are indexes
		if( tracker->VertexProgram )
		{
			// the shaders are deleted as we read the common variable indexes
			tracker->VertexProgram->Release();
			tracker->FragProgram->Release();
			tracker->VertexProgram = NULL;
			tracker->FragProgram = NULL;
		}
	}
}



void ClearShaders( void )
{
	PImageShaderTracker tracker;
	INDEX idx;
	LIST_FORALL( l.d3dActiveSurface->shaders, idx, PImageShaderTracker, tracker )
	{
		tracker->flags.set_matrix = 0;
		if( tracker->flags.set_modelview )
		{
			//mWld3
			//PrintMatrix( (MATRIX)tracker->modelview );
			//g_d3d_device->SetVertexShaderConstantF( 8, (RCOORD*)tracker->modelview, 4 );
			tracker->flags.set_modelview = 0;
		}
	}
}


void EnableShader( PImageShaderTracker tracker, ... )
{

	if( !tracker )
		return;

	if( !tracker->VertexProgram )
	{
		if( tracker->flags.failed )
		{
			// nothing to enable; shader is failed
			return;
		}
		if( tracker->Init )
			tracker->Init( tracker );
		if( !tracker->VertexProgram )
		{
			lprintf( "Shader initialization failed to produce a program; marking shader broken so we don't retry" );
			tracker->flags.failed = 1;
			return;
		}
	}

	g_d3d_device_context->VSSetShader( tracker->VertexProgram, NULL, 0 );
	g_d3d_device_context->PSSetShader( tracker->FragProgram, NULL, 0 );

	if( tracker->Enable )
	{
		va_list args;
		va_start( args, tracker );
		tracker->Enable( tracker, tracker->psv_userdata, args );
	}
	g_d3d_device_context->IASetInputLayout(tracker->input_layout);

	// enable fills constant buffers; so update the buffers here
	if( tracker->vertex_constant_buffer )
		g_d3d_device_context->VSSetConstantBuffers( 0, 1, &tracker->vertex_constant_buffer );
	if( tracker->fragment_constant_buffer )
		g_d3d_device_context->PSSetConstantBuffers( 0, 1, &tracker->fragment_constant_buffer );
	if( tracker->geometry_constant_buffer )
		g_d3d_device_context->GSSetConstantBuffers( 0, 1, &tracker->geometry_constant_buffer );

}



int CompileShaderEx( PImageShaderTracker tracker
						 , char const *const* vertex_code_blocks, int vertex_blocks
						 , char const *const* frag_code_blocks, int frag_blocks
						 , struct image_shader_attribute_order *attrib_order, int nAttribs )
{
	HRESULT result=123;
	char * vertex_code;
	size_t vertex_length;
	char * frag_code;
	size_t frag_length;

	int n;
	size_t length = 0;
	size_t new_length = 0;
	for( n = 0; n < vertex_blocks; n++ )
		length += CStrLen( vertex_code_blocks[n] );
	vertex_code = NewArray( char, length + 1 );
	length = 0;
	for( n = 0; n < vertex_blocks; n++ )
	{
		
		MemCpy( vertex_code + length, vertex_code_blocks[n], ( ( new_length = CStrLen( vertex_code_blocks[n] ) ) + 1 ) );
		length += new_length;
	}
	vertex_length = length;


	for( n = 0; n < frag_blocks; n++ )
		length += CStrLen( frag_code_blocks[n] );
	frag_code = NewArray( char, length + 1 );
	length = 0;
	for( n = 0; n < frag_blocks; n++ )
	{
		MemCpy( frag_code + length, frag_code_blocks[n], ( ( new_length = CStrLen( frag_code_blocks[n] ) ) + 1 ) );
		length += new_length;
	}
	frag_length = length;

	// example of defines
	//D3D_SHADER_MACRO Shader_Macros[1] = { "zero", "0"  };
	// required if shader uses #include
	// #define D3D_COMPILE_STANDARD_FILE_INCLUDE ((ID3DInclude*)(UINT_PTR)1)
	ID3DBlob *vert_blob;
	ID3DBlob *frag_blob;
	ID3DBlob *errors;
	char *tmp;
	char *vs_trylist[] = { "vs_5_0", "vs_4_0", "vs_3_0", "vs_2_0", NULL };
	char *ps_trylist[] = { "ps_5_0", "ps_4_0", "ps_3_0", "ps_2_0", NULL };
	for( n = 0; vs_trylist[n] && !tracker->VertexProgram; n++ )
	{
		result = D3DCompile( vertex_code
								  , vertex_length  //in       SIZE_T SrcDataSize,
								  , DupTextToChar( tracker->name )  //in_opt   LPCSTR pSourceName,  /* used for error message output */
								  , NULL      //in_opt   const D3D_SHADER_MACRO *pDefines,
								  , NULL      //in_opt   ID3DInclude *pInclude,
								 , ("main")       //in       LPCSTR pEntrypoint,  /* ignored by d3dcompile*/
								 , vs_trylist[n]   // in       LPCSTR pTarget,
								 , 0          // in       UINT Flags1,  // comile options
								 , 0          //  in       UINT Flags2, /* unused for source compiles*/
								 , &vert_blob //  out      ID3DBlob **ppCode,
								 , &errors    // out_opt  ID3DBlob **ppErrorMsgs
								 );
		if( result )
		{
			lprintf( "Vertex Shader Error with mode %S : %S", vs_trylist[n], errors->GetBufferPointer() );
		}
		else
		{
			//LogBinary( vert_blob->GetBufferPointer(), vert_blob->GetBufferSize() );
			result = g_d3d_device->CreateVertexShader(vert_blob->GetBufferPointer(), vert_blob->GetBufferSize()
														, NULL /* ID3D11ClassLinkage*  */
													  , &tracker->VertexProgram);
			if( result )
			{
				lprintf( "failed to create vertex shader from compled shader blob mode %S:%08x", vs_trylist[n], result );
				vert_blob->Release();
				vert_blob = NULL;
			}
		}
	}
	if( !tracker->VertexProgram )
		return 0;
	for( n = 0; ps_trylist[n] && !tracker->FragProgram; n++ )
	{
		result = D3DCompile(
								  frag_code
								  , frag_length  //in       SIZE_T SrcDataSize,
								  , DupTextToChar( tracker->name )  //in_opt   LPCSTR pSourceName,  /* used for error message output */
								  , NULL      //in_opt   const D3D_SHADER_MACRO *pDefines,
								  , NULL      //in_opt   ID3DInclude *pInclude,
								 , ("main")     //in       LPCSTR pEntrypoint,  /* ignored by d3dcompile*/
								 , ps_trylist[n] // in       LPCSTR pTarget,
								 , 0          // in       UINT Flags1,  // comile options
								 , 0          //  in       UINT Flags2, /* unused for source compiles*/
								 , &frag_blob //  out      ID3DBlob **ppCode,
								 , &errors    // out_opt  ID3DBlob **ppErrorMsgs
								 );

		if( !result )
		{
			//LogBinary( frag_blob->GetBufferPointer(), vert_blob->GetBufferSize() );
			result = g_d3d_device->CreatePixelShader(frag_blob->GetBufferPointer(), frag_blob->GetBufferSize()
				, NULL /* ID3D11ClassLinkage */
												  , &tracker->FragProgram);
			if( result )
			{
				lprintf( "failed to create fragment shader from compled shader blob mode %S : %08x", ps_trylist[n], result );
				frag_blob->Release();
				frag_blob = NULL;
			}
		}
		else
			lprintf( "Fragment Shader error mode %S : %S", ps_trylist[n], errors->GetBufferPointer() ); 
	
	}
	if( !tracker->FragProgram )
		return 0;

	if( nAttribs && attrib_order )
	{
		int n;
		int offset = 0;
		D3D11_INPUT_ELEMENT_DESC *ied = NewArray( D3D11_INPUT_ELEMENT_DESC, nAttribs );
		MemSet( ied, 0, sizeof( D3D11_INPUT_ELEMENT_DESC ) * nAttribs );
		for( n = 0; n < nAttribs; n++ )
		{
			ied[n].SemanticName = attrib_order[n].name;
			ied[n].Format = (DXGI_FORMAT)attrib_order[n].format;
			ied[n].AlignedByteOffset = offset;
			ied[n].InputSlotClass = (D3D11_INPUT_CLASSIFICATION)attrib_order[n].input_class;
			offset += attrib_order[n].size;
		}
		result = g_d3d_device->CreateInputLayout(ied, 2, vert_blob->GetBufferPointer(), vert_blob->GetBufferSize(), &tracker->input_layout);
		if( result )
		{
			lprintf( "layout failed to match vertex shader..." );
		}

	}

	if( vert_blob )
	{
		vert_blob->Release();
		vert_blob = NULL;
	}
	if( frag_blob )
	{
		frag_blob->Release();
		frag_blob = NULL;
	}


	return (tracker != NULL);
}


int CompileShader( PImageShaderTracker tracker, char const *const* vertex_code, int vertex_blocks
					                      , char const *const* frag_code, int frag_length
					  )
{
   return CompileShaderEx( tracker, vertex_code, vertex_blocks, frag_code, frag_length, NULL, 0 );
}

void SetShaderModelView( PImageShaderTracker tracker, RCOORD *matrix )
{
	if( tracker )
	{
		//glUseProgram(tracker->glProgramId);
		//CheckErrf( "SetModelView for (%s)", tracker->name );

		//glUniformMatrix4fv( tracker->modelview, 1, GL_FALSE, matrix );
		//CheckErrf( "SetModelView for (%s)", tracker->name );

		tracker->flags.set_modelview = 1;
	}
}

IMAGE_NAMESPACE_END
