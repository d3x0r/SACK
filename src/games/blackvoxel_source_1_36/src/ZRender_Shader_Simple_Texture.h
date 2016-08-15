#ifndef ZRENDER_SHADER_SIMPLE_TEXTURE_DEFINED
#define ZRENDER_SHADER_SIMPLE_TEXTURE_DEFINED

#include "ZRender_Shader_Interface.h"

class ZRender_Shader_Simple_Texture: public ZRender_Shader_Interface
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

	int texture_attrib;
	int color_attrib;
	int pos_attrib;
	int texture_uniform;
	struct shader_buffer *box_buffer;
	struct shader_buffer *box_texture_buffer;

	public:

    ZRender_Shader_Simple_Texture( class ZRender_Interface *render );

	uintptr_t SetupShader( void );
	void InitShader( PImageShaderTracker tracker );

	void DrawLine( ZVector3f *p1, ZVector3f *p2, ZVector4f *c );
	void DrawBox( ZVector3f *p1, ZVector3f *p2, ZVector3f *p3, ZVector3f *p4
		, ZVector3f *p5, ZVector3f *p6, ZVector3f *p7, ZVector3f *p8
		, ZVector4f *c );
	void DrawFilledRect( ZVector3f p[4], ZVector4f &c );
	void DrawItems( int tex, struct shader_buffer *vert, struct shader_buffer *tex_uv );

};

#endif