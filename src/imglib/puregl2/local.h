#ifndef IMGLIB_PUREGL_LOCAL_INCLUDED
#define IMGLIB_PUREGL_LOCAL_INCLUDED

#include <imglib/imagestruct.h>
#include <imglib/fontstruct.h>

#if defined( USE_GLES )
#include <GLES/gl.h>
#endif
#if defined( USE_GLES2 ) || defined( __EMSCRIPTEN__ )
//#include <GLES/gl.h>
#include <GLES3/gl3.h>
#else
#define USE_OPENGL
#if defined( _WIN32 ) || defined( __LINUX__ )
#  include <GL/glew.h>
#endif
#include <GL/gl.h>         // Header File For The OpenGL32 Library
//#include <../src/vidlib/glext.h>
#endif

IMAGE_NAMESPACE
struct glSurfaceData;
IMAGE_NAMESPACE_END

#include "../image_common.h"
#include "shaders.h"

IMAGE_NAMESPACE

struct glSurfaceData 
{
	PMatrix M_Projection;
	PTRANSFORM T_Camera;
	RCOORD *identity_depth;
	RCOORD *aspect;
	int index;
	struct gl_shader_data {
		GLuint multi_shader;
		struct {
			BIT_FIELD init_ok : 1;
			BIT_FIELD shader_ok : 1;
		} flags;
		GLuint inverse_shader;
	} shader;
	PLIST shaders;
	struct {
		PLIST tracked_shader_operations;
		PLIST shader_operations;
		struct image_shader_image_buffer *last_operation;
		PLIST image_shader_operations; // struct image_shader_image_buffer_op
	} shader_local;


};

#ifndef IMAGE_LIBRARY_SOURCE_MAIN
extern
#endif
struct local_puregl_image_data_tag {
	struct {
		BIT_FIELD projection_read : 1;
		BIT_FIELD worldview_read : 1;
	} flags;
	GLuint glImageIndex;
	PLIST glSurface; // list of struct glSurfaceData *
	struct glSurfaceData *glActiveSurface;
	RCOORD scale;
	PTRANSFORM camera; // active camera at begindraw

	float projection[16];
	MATRIX worldview;
	PImageShaderTracker simple_shader;
	PImageShaderTracker simple_texture_shader;
	PImageShaderTracker simple_shaded_texture_shader;
	PImageShaderTracker simple_multi_shaded_texture_shader;
	PImageShaderTracker simple_inverse_texture_shader;
	int glslVersion;
	GLboolean depth_enabled;

} local_puregl_image_data;
#define l local_puregl_image_data

void InitShader( void );

int CPROC IMGVER(ReloadOpenGlTexture)( Image child_image, int option );
int CPROC IMGVER(ReloadOpenGlShadedTexture)( Image child_image, int option, CDATA color );
int CPROC IMGVER(ReloadOpenGlMultiShadedTexture)( Image child_image, int option, CDATA r, CDATA g, CDATA b );

Image IMGVER(AllocateCharacterSpaceByFont)( Image target_image, SFTFont font, PCHARACTER character );
int IMGVER(ReloadOpenGlTexture)( Image image, int option );
void IMGVER(TranslateCoord)( Image image, int32_t *x, int32_t *y );
void CPROC IMGVER(MarkImageUpdated)( Image image );

void 	IMGVER(CleanupFontSurfaces)( void );

IMAGE_NAMESPACE_END

#endif