#ifndef __IMAGE3D_EXTENSIONS_DEFINED__
#define __IMAGE3D_EXTENSIONS_DEFINED__

// define USE_IMAGE_3D_INTERFACE to be the 3d image interface you want to use

#include <image.h>
#include <vectlib.h>

IMAGE_NAMESPACE

typedef struct image_shader_tracker *PImageShaderTracker;

struct image_shader_attribute_order
{
	int n;
   char *name;  // opengl value;
};

typedef struct image_3d_interface_tag
{
	IMAGE_PROC_PTR( PImageShaderTracker, ImageGetShader)         ( CTEXTSTR name, void (CPROC*Init)(PImageShaderTracker) );
	IMAGE_PROC_PTR( int, ImageCompileShader )( PImageShaderTracker shader
														  , char **vertex_code, int vert_blocks
														  , char **frag_code, int frag_blocks );
	IMAGE_PROC_PTR( int, ImageCompileShaderEx )( PImageShaderTracker shader
															 , char **vertex_code, int vert_blocks
															 , char **frag_code, int frag_blocks
															 , struct image_shader_attribute_order *attribs, int nAttribs  );
	IMAGE_PROC_PTR( void, ImageEnableShader )( PImageShaderTracker tarcker, ... );
	IMAGE_PROC_PTR( void, ImageSetShaderEnable )( PImageShaderTracker tracker, void (CPROC*EnableShader)( PImageShaderTracker tracker, PTRSZVAL psv, va_list args ), PTRSZVAL psv );
	IMAGE_PROC_PTR( void, ImageSetShaderModelView )( PImageShaderTracker tracker, RCOORD *matrix );

} IMAGE_3D_INTERFACE, *PIMAGE_3D_INTERFACE;



#if !defined( FORCE_NO_INTERFACE ) && !defined( FORCE_NO_IMAGE_INTERFACE )

#define IMAGE3D_PROC_ALIAS(name) ((USE_IMAGE_3D_INTERFACE)->_##name)
#  define GetImage3dInterface() (PIMAGE_3D_INTERFACE)GetInterface( WIDE("image.3d") )

#  define ImageGetShader             IMAGE3D_PROC_ALIAS(ImageGetShader)
#  define ImageCompileShader         IMAGE3D_PROC_ALIAS(ImageCompileShader)
#  define ImageCompileShaderEx       IMAGE3D_PROC_ALIAS(ImageCompileShaderEx)
#  define ImageEnableShader          IMAGE3D_PROC_ALIAS(ImageEnableShader)
#  define ImageSetShaderEnable       IMAGE3D_PROC_ALIAS(ImageSetShaderEnable)
#  define ImageSetShaderModelView    IMAGE3D_PROC_ALIAS(ImageSetShaderModelView)

#endif
IMAGE_NAMESPACE_END

#endif // __IMAGE3D_EXTENSIONS_DEFINED__
