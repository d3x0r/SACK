#ifndef IMAGE_SHADER_EXTENSION_INCLUDED
#define IMAGE_SHADER_EXTENSION_INCLUDED

#define __need___va_list
#include <stdarg.h>

#include <D3D10Shader.h>

#ifdef __ANDROID__
#define va_list __va_list
#else
#define va_list va_list
#endif

#include <image3d.h>

IMAGE_NAMESPACE

typedef struct image_shader_tracker ImageShaderTracker;

struct image_shader_tracker
{
	struct image_shader_flags
	{
		BIT_FIELD set_matrix : 1; // flag indicating we have set the matrix; this is cleared at the beginning of each enable of a context
		BIT_FIELD failed : 1; // shader compilation failed, abort enable; and don't reinitialize
		BIT_FIELD set_modelview : 1;
	} flags;
	CTEXTSTR name;

	ID3D10Buffer *vertexDecl;
	ID3D10PixelShader  * FragProgram;
	ID3D10VertexShader  * VertexProgram;

	int eye_point;
	int position_attrib;
	int color_attrib;
	int projection;
	int worldview;
	int modelview;
	PTRSZVAL psv_userdata;
	void (CPROC*Init)( PImageShaderTracker );
	void (CPROC*Enable)( PImageShaderTracker,PTRSZVAL,va_list);
};


PImageShaderTracker CPROC GetShader( CTEXTSTR name, void (*)(PImageShaderTracker) );
void CPROC  SetShaderEnable( PImageShaderTracker tracker, void (CPROC*EnableShader)( PImageShaderTracker tracker, PTRSZVAL, va_list args ), PTRSZVAL psv );
void CPROC SetShaderModelView( PImageShaderTracker tracker, RCOORD *matrix );

int CPROC CompileShaderEx( PImageShaderTracker shader
								 , char const *const* vertex_code, int vert_length
								 , char const *const*frag_code, int frag_length
								 , struct image_shader_attribute_order *, int nAttribs );
int CPROC CompileShader( PImageShaderTracker shader
							  , char const *const* vertex_code, int vert_blocks
							  , char const *const*frag_code, int frag_length );
void CPROC ClearShaders( void );

void CPROC EnableShader( PImageShaderTracker shader, ... );


// verts and a single color
void InitSuperSimpleShader( PImageShaderTracker tracker );

// verts, texture verts and a single texture
void InitSimpleTextureShader( PImageShaderTracker tracker );

// verts, texture_verts, texture and a single texture
void InitSimpleShadedTextureShader( PImageShaderTracker tracker );

//
void InitSimpleMultiShadedTextureShader( PImageShaderTracker tracker );

void DumpAttribs( PImageShaderTracker tracker, int program );

void CloseShaders( struct glSurfaceData *glSurface );
IMAGE_NAMESPACE_END

#endif
