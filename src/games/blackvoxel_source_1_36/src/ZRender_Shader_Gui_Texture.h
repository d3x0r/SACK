#ifndef ZRENDER_SHADER_GUI_TEXTURE_DEFINED
#define ZRENDER_SHADER_GUI_TEXTURE_DEFINED

#include "ZRender_Shader_Interface.h"

class ZRender_Shader_Gui_Texture: public ZRender_Shader_Interface
{
	static const char *vertex_shader; 

	static const char *pixel_shader;

	struct simple_shader_op_data
	{
		//int color_attrib;
		struct shader_buffer *vert_pos;
		struct shader_buffer *vert_color;
	};

	struct simple_shader_data
	{
		int color_attrib;
		float *next_color;
		struct simple_shader_op_data data;
		//struct shader_buffer *vert_pos;
		//struct shader_buffer *vert_color;
	}data;

	PImageShaderTracker shader;
	int pos_attrib;
	int tex_coord_attrib;
	int texture_uniform;
	int color_uniform;
	public:
    ZRender_Shader_Gui_Texture( class ZRender_Interface *render );

	uintptr_t SetupShader( void );
	void InitShader( PImageShaderTracker tracker );

	void ZRender_Shader_Gui_Texture::Draw( int texture
											  , float *color
												, float *pos, float *uv );
};

#endif