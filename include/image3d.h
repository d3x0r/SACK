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

struct shader_buffer {
	float *data;
	int dimensions;
	int used;
	int avail;
	int expand_by;
};

typedef struct image_3d_interface_tag
{
	IMAGE_PROC_PTR( PImageShaderTracker, ImageGetShaderInit)         ( CTEXTSTR name
		, uintptr_t (CPROC*Setup)(uintptr_t psvInit)
		, void (CPROC*Init)(uintptr_t psvSetup,PImageShaderTracker)
		, uintptr_t psvInit );	
	IMAGE_PROC_PTR( int, ImageCompileShader )( PImageShaderTracker shader
														  , char const*const*vertex_code, int vert_blocks
														  , char const*const*frag_code, int frag_blocks );
	IMAGE_PROC_PTR( int, ImageCompileShaderEx )( PImageShaderTracker shader
															 , char const*const*vertex_code, int vert_blocks
															 , char const*const*frag_code, int frag_blocks
															 , struct image_shader_attribute_order *attribs, int nAttribs 
															 );
	IMAGE_PROC_PTR( void, ImageEnableShader )( PImageShaderTracker tracker, ... );
	IMAGE_PROC_PTR( void, ImageSetShaderEnable )( PImageShaderTracker tracker, void (CPROC*EnableShader)( PImageShaderTracker tracker, uintptr_t psv, va_list args ), uintptr_t );
	IMAGE_PROC_PTR( void, ImageSetShaderModelView )( PImageShaderTracker tracker, RCOORD *matrix );
	IMAGE_PROC_PTR( struct image_shader_op *, ImageBeginShaderOp )(PImageShaderTracker tracker, ... );
	// same parameters as the enable...
	IMAGE_PROC_PTR( void, AppendShaderTristripQuad )( struct image_shader_op * tracker, ... );
	// shader specific append; so it can deal with tracking source images for grouping
	IMAGE_PROC_PTR( void, SetShaderAppendTristrip )( PImageShaderTracker tracker, void (CPROC*AppendTristrip)( struct image_shader_op *op, int triangles,uintptr_t,va_list args ) );
	IMAGE_PROC_PTR( void, SetShaderOutput )( PImageShaderTracker tracker, void (CPROC*Flush)( PImageShaderTracker tracker, uintptr_t, uintptr_t, int, int ) );
	IMAGE_PROC_PTR( void, SetShaderReset )( PImageShaderTracker tracker, void (CPROC*Reset)( PImageShaderTracker tracker, uintptr_t, uintptr_t ) );
	// same parameters as the enable...
	IMAGE_PROC_PTR( void, AppendShaderTristrip )( struct image_shader_op * tracker, int triangles, ... );
	//IMAGE_PROC_PTR( struct image_shader_op *, BeginShaderOp)(PImageShaderTracker tracker );
	//IMAGE_PROC_PTR( void, ClearShaderOp) (struct image_shader_op *op );
	//IMAGE_PROC_PTR( void, AppendShaderOpTristrip )( struct image_shader_op *op, int triangles, ... );
	IMAGE_PROC_PTR( struct shader_buffer *, ImageCreateShaderBuffer )( int dimensions, int start_size, int expand_by );
	IMAGE_PROC_PTR( size_t, ImageAppendShaderData )( struct shader_buffer *buffer, float *element );

} IMAGE_3D_INTERFACE, *PIMAGE_3D_INTERFACE;



#if !defined( FORCE_NO_INTERFACE ) && !defined( FORCE_NO_IMAGE_INTERFACE )

#  define IMAGE3D_PROC_ALIAS(name) ((USE_IMAGE_3D_INTERFACE)->_##name)

#  define GetImage3dInterface() (PIMAGE_3D_INTERFACE)GetInterface( WIDE("image.3d") )

#  define ImageGetShaderInit(a,b,c,d)             IMAGE3D_PROC_ALIAS(ImageGetShaderInit)(a,b,c,d)
#  define ImageGetShader(a)             IMAGE3D_PROC_ALIAS(ImageGetShaderInit)(a,NULL,NULL,0)
#  define ImageCompileShader         IMAGE3D_PROC_ALIAS(ImageCompileShader)
#  define ImageCompileShaderEx       IMAGE3D_PROC_ALIAS(ImageCompileShaderEx)
#  define ImageEnableShader          IMAGE3D_PROC_ALIAS(ImageEnableShader)
#  define ImageSetShaderEnable       IMAGE3D_PROC_ALIAS(ImageSetShaderEnable)
#  define ImageSetShaderOpInit       IMAGE3D_PROC_ALIAS(ImageSetShaderOpInit)
#  define ImageSetShaderOutput       IMAGE3D_PROC_ALIAS(ImageSetShaderOutput)
#  define ImageSetShaderFlush       IMAGE3D_PROC_ALIAS(ImageSetShaderFlush)
#  define ImageSetShaderModelView    IMAGE3D_PROC_ALIAS(ImageSetShaderModelView)
#  define ImageAppendShaderTristripQuad    IMAGE3D_PROC_ALIAS(ImageAppendShaderTristripQuad)
#  define ImageSetShaderAppendTristrip    IMAGE3D_PROC_ALIAS(ImageSetShaderAppendTristrip)
#  define ImageAppendShaderTristrip    IMAGE3D_PROC_ALIAS(ImageAppendShaderTristrip)
#  define ImageBeginShaderOp                IMAGE3D_PROC_ALIAS(BeginShaderOp)
#  define ImageClearShaderOp                IMAGE3D_PROC_ALIAS(ClearShaderOp)
#  define ImageAppendShaderOpTristrip       IMAGE3D_PROC_ALIAS(AppendShaderOpTristrip)
#  define ImageCreateShaderBuffer           IMAGE3D_PROC_ALIAS(ImageCreateShaderBuffer)
#  define ImageAppendShaderData             IMAGE3D_PROC_ALIAS(ImageAppendShaderData)
#endif
IMAGE_NAMESPACE_END

#endif // __IMAGE3D_EXTENSIONS_DEFINED__
