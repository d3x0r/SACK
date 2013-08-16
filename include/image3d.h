#ifndef __IMAGE3D_EXTENSIONS_DEFINED__
#define __IMAGE3D_EXTENSIONS_DEFINED__

// define USE_IMAGE_3D_INTERFACE to be the 3d image interface you want to use

#include <image.h>
#include <vectlib.h>

typedef struct image_shader_tracker *PImageShaderTracker;

struct image_shader_attribute_order
{
	int n;
   CTEXTSTR name;
};

typedef struct image_3d_interface_tag
{
	IMAGE_PROC_PTR( PImageShaderTracker, ImageGetShader)         ( CTEXTSTR name, void (CPROC*Init)(PImageShaderTracker) );
	IMAGE_PROC_PTR( int, ImageCompileShader )( PImageShaderTracker shader
														  , CTEXTSTR *vertex_code, int vert_blocks
														  , CTEXTSTR *frag_code, int frag_blocks );
	IMAGE_PROC_PTR( int, ImageCompileShaderEx )( PImageShaderTracker shader
															 , CTEXTSTR *vertex_code, int vert_blocks
															 , CTEXTSTR *frag_code, int frag_blocks
															 , struct image_shader_attribute_order *attribs, int nAttribs  );
	IMAGE_PROC_PTR( void, ImageEnableShader )( CTEXTSTR name, ... );
} IMAGE_3D_INTERFACE, *PIMAGE_3D_INTERFACE;



#if !defined( FORCE_NO_INTERFACE ) && !defined( FORCE_NO_IMAGE_INTERFACE )

#define IMAGE3D_PROC_ALIAS(name) ((USE_IMAGE_3D_INTERFACE)->_##name)
#  define GetImage3dInterface() (PIMAGE_3D_INTERFACE)GetInterface( WIDE("image.3d") )

#  define ImageGetShader             IMAGE3D_PROC_ALIAS(ImageGetShader)
#  define ImageCompileShader         IMAGE3D_PROC_ALIAS(ImageCompileShader)
#  define ImageCompileShaderEx       IMAGE3D_PROC_ALIAS(ImageCompileShaderEx)
#  define ImageEnableShader          IMAGE3D_PROC_ALIAS(ImageEnableShader)
#endif

#endif // __IMAGE3D_EXTENSIONS_DEFINED__
