
#include <imglib/imagestruct.h>
#include <imglib/fontstruct.h>

#ifdef __ANDROID__
#include <gles/gl.h>
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
};


#ifndef IMAGE_LIBRARY_SOURCE_MAIN
extern
#endif
struct local_puregl_image_data_tag {
	GLuint glImageIndex;
	PLIST glSurface; // list of struct glSurfaceData *
	struct glSurfaceData *glActiveSurface;
	RCOORD scale;
	PTRANSFORM camera; // active camera at begindraw
} local_puregl_image_data;
#define l local_puregl_image_data




void InitShader( void );

int CPROC ReloadOpenGlTexture( Image child_image, int option );
int CPROC ReloadOpenGlShadedTexture( Image child_image, int option, CDATA color );
int CPROC ReloadOpenGlMultiShadedTexture( Image child_image, int option, CDATA r, CDATA g, CDATA b );

Image AllocateCharacterSpaceByFont( Image target_image, SFTFont font, PCHARACTER character );
int ReloadOpenGlTexture( Image image, int option );
void TranslateCoord( Image image, int32_t *x, int32_t *y );
void CPROC MarkImageUpdated( Image image );

IMAGE_NAMESPACE_END
