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
	int format;
	int size;
	int input_class;
};

typedef struct image_3d_interface_tag
{
	IMAGE_PROC_PTR( PImageShaderTracker, ImageGetShader)         ( CTEXTSTR name, void (CPROC*Init)(PImageShaderTracker,PTRSZVAL) );	
	IMAGE_PROC_PTR( int, ImageCompileShader )( PImageShaderTracker shader
														  , char const*const*vertex_code, int vert_blocks
														  , char const*const*frag_code, int frag_blocks );
	IMAGE_PROC_PTR( int, ImageCompileShaderEx )( PImageShaderTracker shader
															 , char const*const*vertex_code, int vert_blocks
															 , char const*const*frag_code, int frag_blocks
															 , struct image_shader_attribute_order *attribs, int nAttribs 
															 );
	IMAGE_PROC_PTR( void, ImageEnableShader )( PImageShaderTracker tarcker, ... );
	IMAGE_PROC_PTR( void, ImageSetShaderEnable )( PImageShaderTracker tracker, void (CPROC*EnableShader)( PImageShaderTracker tracker, PTRSZVAL psv, va_list args ) );
	IMAGE_PROC_PTR( void, ImageSetShaderModelView )( PImageShaderTracker tracker, RCOORD *matrix );
	// same parameters as the enable...
	IMAGE_PROC_PTR( void, AppendShaderTristripQuad )( PImageShaderTracker tracker, ... );
	// shader specific append; so it can deal with tracking source images for grouping
	IMAGE_PROC_PTR( void, SetShaderAppendTristrip )( PImageShaderTracker tarcker, void (CPROC*AppendTristrip)( PImageShaderTracker tracker, int triangles,PTRSZVAL,va_list args ) );
	// same parameters as the enable...
	IMAGE_PROC_PTR( void, AppendShaderTristrip )( PImageShaderTracker tracker, int triangles, ... );
	IMAGE_PROC_PTR( void, SetShaderFlush )( PImageShaderTracker tarcker, void (CPROC*Flush)( PImageShaderTracker tracker, PTRSZVAL, PTRSZVAL, int, int ) );


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
#  define ImageAppendShaderTristripQuad    IMAGE3D_PROC_ALIAS(ImageAppendShaderTristripQuad)
#  define ImageSetShaderAppendTristrip    IMAGE3D_PROC_ALIAS(ImageSetShaderAppendTristrip)
#  define ImageAppendShaderTristrip    IMAGE3D_PROC_ALIAS(ImageAppendShaderTristrip)

#endif
IMAGE_NAMESPACE_END

#endif // __IMAGE3D_EXTENSIONS_DEFINED__
