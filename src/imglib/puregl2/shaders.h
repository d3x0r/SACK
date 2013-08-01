
#define __need___va_list
#include <stdarg.h>

#ifdef USE_GLES2
#include <GLES2/gl2.h>
#endif

#ifdef _MSC_VER
#include <GL/GLU.h>
#define CheckErr()  				{    \
					GLenum err = glGetError();  \
					if( err )                   \
						lprintf( "err=%d (%s)",err, gluErrorString( err ) ); \
				}                               
#else
#define CheckErr()  				{    \
					GLenum err = glGetError();  \
					if( err )                   \
						lprintf( "err=%d ",err ); \
				}                               
#define CheckErrf(f,...)  				{    \
					GLenum err = glGetError();  \
					if( err )                   \
					lprintf( "err=%d "f,err,##__VA_ARGS__ ); \
				}                               
#endif

#ifdef __ANDROID__
#define va_list __va_list
#else
#define va_list va_list
#endif


typedef struct image_shader_tracker ImageShaderTracker;
typedef struct image_shader_tracker *PImageShaderTracker;

struct image_shader_tracker
{
	struct image_shader_flags
	{
		BIT_FIELD set_matrix : 1;
	} flags;
	CTEXTSTR name;
	int glProgramId;
	int glVertexProgramId;
	int glFragProgramId;

	int eye_point;
	int position_attrib;
	int color_attrib;
	int projection;
	int worldview;
	int modelview;
	PTRSZVAL psv_userdata;
	void (CPROC*Enable)( PImageShaderTracker,va_list);
};



PImageShaderTracker GetShader( CTEXTSTR name );
void ClearShaders( void );

void EnableShader( CTEXTSTR shader, ... );

// this part is very specific to a shader....
void SetupCommon( PImageShaderTracker tracker, CTEXTSTR position, CTEXTSTR color );


// verts and a single color
void InitSuperSimpleShader( PImageShaderTracker tracker );

// verts, texture verts and a single texture
void InitSimpleTextureShader( PImageShaderTracker tracker );

// verts, texture_verts, texture and a single texture
void InitSimpleShadedTextureShader( PImageShaderTracker tracker );

void DumpAttribs( int program );
