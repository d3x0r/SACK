
#include <imglib/imagestruct.h>
#include <imglib/fontstruct.h>

#if defined( USE_GLES2 )
//#include <gles/gl.h>
#include <GLES2/gl2.h>
#else
#include <GL/glew.h>
#include <GL/gl.h>         // Header File For The OpenGL32 Library
#endif

IMAGE_NAMESPACE

struct glSurfaceData {
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
};

struct glSurfaceImageData {
	struct {
		BIT_FIELD updated : 1;
	} flags;
	GLuint glIndex;
};

#ifndef IMAGE_LIBRARY_SOURCE_MAIN
extern
#endif
struct local_puregl_image_data_tag {
	struct {
		BIT_FIELD projection_read : 1;
		BIT_FIELD worldview_read : 1;
	} flags;
	PTREEROOT shade_cache;
	GLuint glImageIndex;
	PLIST glSurface; // list of struct glSurfaceData *
	struct glSurfaceData *glActiveSurface;
	RCOORD scale;
	PTRANSFORM camera; // active camera at begindraw

	float projection[16];
	MATRIX worldview;
} local_puregl_image_data;
#define l local_puregl_image_data

struct shade_cache_element {
	CDATA r,grn,b;
	Image image;
	_32 age;
	LOGICAL inverted;
};

struct shade_cache_image
{
	PLIST elements;
	Image image;
};

// use this if opengl shaders are missing.
Image GetShadedImage( Image image, CDATA red, CDATA green, CDATA blue );
Image GetInvertedImage( Image image );


void InitShader( void );

int CPROC ReloadOpenGlTexture( Image child_image, int option );
int CPROC ReloadOpenGlShadedTexture( Image child_image, int option, CDATA color );
int CPROC ReloadOpenGlMultiShadedTexture( Image child_image, int option, CDATA r, CDATA g, CDATA b );

Image AllocateCharacterSpaceByFont( SFTFont font, PCHARACTER character );
int ReloadOpenGlTexture( Image image, int option );
void TranslateCoord( Image image, S_32 *x, S_32 *y );
struct glSurfaceImageData * MarkImageUpdated( Image image );

IMAGE_NAMESPACE_END
