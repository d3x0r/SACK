
#include <imglib/imagestruct.h>
#include <imglib/fontstruct.h>


#ifdef _WIN32 
#  define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <vulkan/vulkan.h>

IMAGE_NAMESPACE
struct vkSurfaceData;
IMAGE_NAMESPACE_END

#include "../image_common.h"
#include "../../vidlib/vulkan/vulkaninfo.h"
#include "shaders.h"

IMAGE_NAMESPACE

struct vkSurfaceData 
{
	PMatrix M_Projection;
	PTRANSFORM T_Camera;
	RCOORD *identity_depth;
	RCOORD *aspect;
	int index;
	struct vk_shader_data {
		uint32_t multi_shader;
		struct {
			BIT_FIELD init_ok : 1;
			BIT_FIELD shader_ok : 1;
		} flags;
		uint32_t inverse_shader;
	} shader;
	PLIST shaders;
	struct {
		PLIST tracked_shader_operations;
		PLIST shader_operations;
		struct image_shader_image_buffer *last_operation;
		PLIST image_shader_operations; // struct image_shader_image_buffer_op
	} shader_local;


};

struct thread_compiler {
	PTHREAD thread;
	shaderc_compiler_t compiler;
};

#ifndef IMAGE_LIBRARY_SOURCE_MAIN
extern
#endif
struct local_puregl_image_data_tag {
	struct {
		BIT_FIELD projection_read : 1;
		BIT_FIELD worldview_read : 1;
	} flags;
	uint32_t vkImageIndex;
	PLIST vkSurface; // list of struct vkSurfaceData *
	struct vkSurfaceData *vkActiveSurface;
	RCOORD scale;
	PTRANSFORM camera; // active camera at begindraw

	float projection[16];
	MATRIX worldview;
	PImageShaderTracker simple_shader;
	PImageShaderTracker simple_texture_shader;
	PImageShaderTracker simple_shaded_texture_shader;
	PImageShaderTracker simple_multi_shaded_texture_shader;
	PImageShaderTracker simple_inverse_texture_shader;

	PLIST compilerPool;
	//int glslVersion;
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
