#include <stdhdrs.h>

#include "local.h"

#include "shaders.h"

IMAGE_NAMESPACE

static const char *gles_simple_v_shader_1_30 =
  // "precision mediump float;\n"
  // "precision mediump int;\n"
	"#version 430\n"
	"layout(location=0) in vec4 vPosition;\n"
	"layout(location=1) in vec2 in_texCoord;\n" 
	"uniform uBlock { mat4 modelView;\n" 
	 "mat4 worldView;\n" 
	 "mat4 Projection; };\n" 
	"layout(location=0) out vec2 out_texCoord;\n" 
    "void main() {\n"
    "  gl_Position = Projection * worldView * modelView * vPosition;\n" 
	 "out_texCoord = in_texCoord;\n" 
    "}\n"; 

static const char *gles_simple_p_shader_1_30 =
    // "precision mediump float;\n" 
   //"precision mediump float;\n"
	//"precision mediump int;\n"
	"#version 430\n"
	"layout(location=0) in vec2 out_texCoord;\n" 
	"layout(location=0) out vec4 fragColor;"
	" uniform sampler2D tex;\n"
	 "void main() {\n"
	 "  fragColor = texture( tex, out_texCoord );\n"
     "}\n" ;


//const char *gles_
static const char *gles_simple_v_shader_shaded_texture_1_30 =
   //"precision mediump float;\n"
	//"precision mediump int;\n"
	"#version 430\n"
	"layout(location=0) in vec4 vPosition;\n" 
	  "layout(location=1)  in vec2 in_texCoord;\n" 
	  "layout(location=2)  in vec4 in_Color;\n"
	"uniform uBlock { mat4 modelView;\n" 
	  "mat4 worldView;\n" 
	"mat4 Projection; };\n" 
	  "layout(location=0) out vec2 out_texCoord;\n" 
	  "layout(location=1) out vec4 out_Color;\n" 
	  "\n" 
    "void main() {\n"
    "  gl_Position = Projection * worldView * modelView * vPosition;\n" 
	  " out_texCoord = in_texCoord;\n" 
	  " out_Color = in_Color;\n" 
    "}"; 

static const char *gles_simple_p_shader_shaded_texture_1_30 =
    // "precision mediump float;\n" 
   //"precision mediump float;\n"
  // "precision mediump int;\n"
	"#version 430\n"
	"layout(location=0)  in vec4 out_Color;\n"
	 "layout(location=1)  in vec2 out_texCoord;\n" 
	"layout(location=0) out vec4 fragColor;"
	" uniform sampler2D tex;\n"
	 "void main() {\n"
	 "   fragColor = out_Color * texture( tex, out_texCoord );\n"
     "}\n" ;

struct private_shader_texture_data
{
	int texture;
	float *next_color;
	struct shader_buffer *vert_pos;
	struct shader_buffer *vert_color;
	struct shader_buffer *vert_texture_uv;
};

struct private_shader_data
{
	int color_attrib;
	int texture_attrib;
	int texture_uniform;
	PLIST vert_data;
};

static struct private_shader_texture_data *GetImageBuffer( struct private_shader_data *data, int image )
{
	INDEX idx;
	struct private_shader_texture_data *texture;
	LIST_FORALL( data->vert_data, idx, struct private_shader_texture_data *, texture )
	{
		if( texture->texture == image )
			break;
	}
	if( !texture )
	{
		texture = New( struct private_shader_texture_data );
		texture->texture = image;
		texture->vert_pos = CreateShaderBuffer( 3, 8, 16 );
		texture->vert_color = CreateShaderBuffer( 4, 8, 16 );
		texture->vert_texture_uv = CreateShaderBuffer( 2, 8, 16 );
		AddLink( &data->vert_data, texture );
	}
	return texture;
}

uintptr_t CPROC SimpleTextureShader_OpInit( PImageShaderTracker tracker, uintptr_t psv, int *existing_verts, va_list args )
{
	struct private_shader_data *data = (struct private_shader_data *)psv;
	int texture = va_arg( args, int );
	struct private_shader_texture_data *text_buffer = GetImageBuffer( data, texture );
	*existing_verts = text_buffer->vert_pos->used;
	return (uintptr_t)text_buffer;
}

static void CPROC SimpleTextureOutput( PImageShaderTracker tracker, uintptr_t psv_userdata, uintptr_t psvKey, int from, int to )
{
	struct private_shader_data *data	= (struct private_shader_data *)psv_userdata;
	struct private_shader_texture_data *texture;

	//glUniform4fv( tracker->color_attrib, 1, GL_FALSE, color );
	///////glEnableVertexAttribArray(0);
	///////glEnableVertexAttribArray(data->texture_attrib);
	///////glDisableVertexAttribArray(2);

	EnableShader( tracker );
	texture = (struct private_shader_texture_data *)psvKey;
	//LIST_FORALL( data->vert_data, idx, struct private_shader_texture_data *, texture )
	{

		//////glVertexAttribPointer( 0, 3, GL_FLOAT, FALSE, 0, texture->vert_pos->data );
		//////CheckErr();
		//////glVertexAttribPointer( data->texture_attrib, 2, GL_FLOAT, FALSE, 0, texture->vert_texture_uv->data );            
		//////CheckErr();
		//////glActiveTexture(GL_TEXTURE0 + 0);
		//////CheckErr();
		//////glBindTexture(GL_TEXTURE_2D+0, texture->texture);
		//////CheckErr();
		//////glUniform1i( data->texture_uniform, 0 );
		//////CheckErr();
		//lprintf( "Set data %p %p %d,%d", texture->vert_pos->data, texture->vert_texture_uv->data, from,to );
		//////glDrawArrays( GL_TRIANGLES, from, to - from );

		//texture->vert_pos->used = 0;
		//texture->vert_texture_uv->used = 0;
	}
}

static void CPROC SimpleTextureReset( PImageShaderTracker tracker, uintptr_t psv_userdata, uintptr_t psvKey )
{
	struct private_shader_data *data	= (struct private_shader_data *)psv_userdata;
	struct private_shader_texture_data *texture;

	texture = (struct private_shader_texture_data *)psvKey;
	//LIST_FORALL( data->vert_data, idx, struct private_shader_texture_data *, texture )
	{
		texture->vert_pos->used = 0;
		texture->vert_texture_uv->used = 0;
	}
}

void CPROC SimpleTexture_AppendTristrip( struct image_shader_op *op, int triangles, uintptr_t psv, va_list args )
{
	//struct private_shader_data *data = (struct private_shader_data *)psv;
	float *verts = va_arg( args, float *);
	float *texture_verts = va_arg( args, float *);
	int tri;
	struct private_shader_texture_data *text_buffer = (struct private_shader_texture_data *)psv;//GetImageBuffer( data, texture );
	for( tri = 0; tri < triangles; tri++ )
	{
		if( !( tri & 1 ) )
		{
			// 0,1,2 
			// 2,3,4
			// 4,5,6
			// 0, 2, 4, 6
			AppendShaderData( op, text_buffer->vert_pos, verts + ( tri +  0 ) * 3 );
			AppendShaderData( op, text_buffer->vert_texture_uv, texture_verts + 2 * (tri + 0 ));
			AppendShaderData( op, text_buffer->vert_pos, verts + ( tri + 1 ) * 3 );
			AppendShaderData( op, text_buffer->vert_texture_uv, texture_verts + 2 * (tri + 1 ));
			AppendShaderData( op, text_buffer->vert_pos, verts + ( tri + 2 ) * 3 );
			AppendShaderData( op, text_buffer->vert_texture_uv, texture_verts + 2 * (tri + 2 ));
		}
		else
		{
			// 2,1,3
			// 4,3,5
			AppendShaderData( op, text_buffer->vert_pos, verts + ( (tri-1) + 2 ) * 3 );
			AppendShaderData( op, text_buffer->vert_texture_uv, texture_verts + 2 * ((tri-1) + 2 ));
			AppendShaderData( op, text_buffer->vert_pos, verts + ( (tri-1) + 1 ) * 3 );
			AppendShaderData( op, text_buffer->vert_texture_uv, texture_verts + 2 * ((tri-1) + 1 ));
			AppendShaderData( op, text_buffer->vert_pos, verts + ( (tri-1) + 3 ) * 3 );
			AppendShaderData( op, text_buffer->vert_texture_uv, texture_verts + 2 * ((tri-1) + 3 ));
		}
	}
}

uintptr_t SetupSimpleTextureShader( uintptr_t psv )
{
	struct private_shader_data *data = New(struct private_shader_data );
	data->vert_data = NULL;
	return (uintptr_t)data;
}

void InitSimpleTextureShader( uintptr_t psv_data, PImageShaderTracker tracker )
{
	const char *v_codeblocks[2];
	const char *p_codeblocks[2];
	struct private_shader_data *data = (struct private_shader_data *)psv_data;
	struct image_shader_attribute_order attribs[] = { { 0, "vPosition" }, { 1, "in_TexCoord" } };

	//SetShaderEnable( tracker, SimpleTextureEnable );
	SetShaderOutput( tracker, SimpleTextureOutput );
	SetShaderReset( tracker, SimpleTextureReset );
	SetShaderOpInit( tracker, SimpleTextureShader_OpInit );
	SetShaderAppendTristrip( tracker, SimpleTexture_AppendTristrip );

	v_codeblocks[0] = gles_simple_v_shader_1_30;
	v_codeblocks[1] = NULL;
	p_codeblocks[0] = gles_simple_p_shader_1_30;
	p_codeblocks[1] = NULL;

	if( CompileShaderEx( tracker, v_codeblocks, 1, p_codeblocks, 1, attribs, 2 ) )
	{
		if( !data )
		{
			data = New( struct private_shader_data );
			data->vert_data = NULL;
		}
		///////data->texture_uniform = glGetUniformLocation(tracker->glProgramId, "tex");
		///////data->texture_attrib =  glGetAttribLocation(tracker->glProgramId, "in_texCoord" );
		//data->color_attrib =  glGetAttribLocation(tracker->glProgramId, "in_color" );
		//return (uintptr_t)data;
	}
	//return 0;
}

uintptr_t CPROC SimpleTextureShader_OpInit2( PImageShaderTracker tracker, uintptr_t psv, int *existing_verts, va_list args )
{
	struct private_shader_data *data = (struct private_shader_data *)psv;
	int texture = va_arg( args, int );
	float *color = va_arg( args, float * );
	struct private_shader_texture_data *text_buffer = GetImageBuffer( data, texture );
	text_buffer->next_color = color;
	*existing_verts = text_buffer->vert_pos->used;

	return (uintptr_t)text_buffer;
}

static void CPROC SimpleTextureOutput2( PImageShaderTracker tracker, uintptr_t psv, uintptr_t psvKey, int from, int to )
{
	struct private_shader_data *data = (struct private_shader_data *)psv;
	struct private_shader_texture_data *texture;
	///////glEnableVertexAttribArray(0);	CheckErr();
	///////glEnableVertexAttribArray(data->texture_attrib);	CheckErr();
	///////glEnableVertexAttribArray(data->color_attrib);	CheckErr();
	EnableShader( tracker );
	//lprintf( "Specific is %p", psvKey );
	texture = (struct private_shader_texture_data *)psvKey;
	//LIST_FORALL( data->vert_data, idx, struct private_shader_texture_data *, texture )
	{
		//////glVertexAttribPointer( 0, 3, GL_FLOAT, FALSE, 0, texture->vert_pos->data );  
		//////CheckErr();
		//////glVertexAttribPointer( data->texture_attrib, 2, GL_FLOAT, FALSE, 0, texture->vert_texture_uv->data );  
		//////CheckErr();
		//////glVertexAttribPointer( data->color_attrib, 4, GL_FLOAT, FALSE, 0, texture->vert_color->data );  
		//////CheckErr();

		//When rendering an objectwith this program.
		//////glActiveTexture(GL_TEXTURE0 + 0);
		//////CheckErr();
		//////glBindTexture(GL_TEXTURE_2D+0, texture->texture);
		//////CheckErr();
		//////glUniform1i( data->texture_uniform, 0 );
		//////CheckErr();
		//lprintf( "Set data %p %p %p %d,%d", texture->vert_pos->data, texture->vert_color->data,  texture->vert_texture_uv->data, from,to );
		//////glDrawArrays( GL_TRIANGLES, from, to - from );
	}
}

static void CPROC SimpleTextureReset2( PImageShaderTracker tracker, uintptr_t psv, uintptr_t psvKey )
{
	struct private_shader_data *data = (struct private_shader_data *)psv;
	struct private_shader_texture_data *texture;
	//lprintf( "Specific is %p", psvKey );
	texture = (struct private_shader_texture_data *)psvKey;
	//LIST_FORALL( data->vert_data, idx, struct private_shader_texture_data *, texture )
	texture->vert_pos->used = 0;
	texture->vert_color->used = 0;
	texture->vert_texture_uv->used = 0;
}

void CPROC SimpleTexture_AppendTristrip2( struct image_shader_op *op, int triangles, uintptr_t psv, va_list args )
{
	//struct private_shader_data *data = (struct private_shader_data *)psv;
	float *verts = va_arg( args, float *);
	float *texture_verts = va_arg( args, float *);
	int tri;
	struct private_shader_texture_data *text_buffer = (struct private_shader_texture_data *)psv;//GetImageBuffer( data, texture );
	for( tri = 0; tri < triangles; tri++ )
	{
		if( !( tri & 1 ) )
		{
			// 0,1,2 
			// 2,3,4
			// 4,5,6
			// 0, 2, 4, 6
			AppendShaderData( op, text_buffer->vert_pos, verts + ( tri +  0 ) * 3 );
			AppendShaderData( op, text_buffer->vert_color, text_buffer->next_color);
			AppendShaderData( op, text_buffer->vert_texture_uv, texture_verts + 2 * (tri + 0 ));
			AppendShaderData( op, text_buffer->vert_pos, verts + ( tri + 1 ) * 3 );
			AppendShaderData( op, text_buffer->vert_color, text_buffer->next_color);
			AppendShaderData( op, text_buffer->vert_texture_uv, texture_verts + 2 * (tri + 1 ));
			AppendShaderData( op, text_buffer->vert_pos, verts + ( tri + 2 ) * 3 );
			AppendShaderData( op, text_buffer->vert_color, text_buffer->next_color);
			AppendShaderData( op, text_buffer->vert_texture_uv, texture_verts + 2 * (tri + 2 ));
		}
		else
		{
			// 2,1,3
			// 4,3,5
			AppendShaderData( op, text_buffer->vert_pos, verts + ( (tri-1) + 2 ) * 3 );
			AppendShaderData( op, text_buffer->vert_color, text_buffer->next_color);
			AppendShaderData( op, text_buffer->vert_texture_uv, texture_verts + 2 * ((tri-1) + 2 ));
			AppendShaderData( op, text_buffer->vert_pos, verts + ( (tri-1) + 1 ) * 3 );
			AppendShaderData( op, text_buffer->vert_color, text_buffer->next_color);
			AppendShaderData( op, text_buffer->vert_texture_uv, texture_verts + 2 * ((tri-1) + 1 ));
			AppendShaderData( op, text_buffer->vert_pos, verts + ( (tri-1) + 3 ) * 3 );
			AppendShaderData( op, text_buffer->vert_color, text_buffer->next_color);
			AppendShaderData( op, text_buffer->vert_texture_uv, texture_verts + 2 * ((tri-1) + 3 ));
		}
	}
}

uintptr_t SetupSimpleShadedTextureShader( uintptr_t psv )
{
	struct private_shader_data *data = New(struct private_shader_data );
	data->vert_data = NULL;
	return (uintptr_t)data;
}


void InitSimpleShadedTextureShader( uintptr_t psv, PImageShaderTracker tracker )
{
	const char *v_codeblocks[2];
	const char *p_codeblocks[2];
	struct private_shader_data *data = (struct private_shader_data *)psv;
	struct image_shader_attribute_order attribs[] = { { 0, "vPosition" }, { 1, "in_TexCoord" }, { 2, "in_Color" } };

	//SetShaderEnable( tracker, SimpleTextureEnable2 );
	SetShaderAppendTristrip( tracker, SimpleTexture_AppendTristrip2 );
	SetShaderOutput( tracker, SimpleTextureOutput2 );
	SetShaderReset( tracker, SimpleTextureReset2 );
	SetShaderOpInit( tracker, SimpleTextureShader_OpInit2 );

	v_codeblocks[0] = gles_simple_v_shader_shaded_texture_1_30;
	v_codeblocks[1] = NULL;
	p_codeblocks[0] = gles_simple_p_shader_shaded_texture_1_30;
	p_codeblocks[1] = NULL;

	if( CompileShaderEx( tracker, v_codeblocks, 1, p_codeblocks, 1, attribs, 3 ) )
	{
		//////data->color_attrib = glGetAttribLocation(tracker->glProgramId, "in_Color" );
		//////data->texture_attrib =  glGetAttribLocation(tracker->glProgramId, "in_texCoord" );
		//////data->texture_uniform = glGetUniformLocation(tracker->glProgramId, "tex");
		//return (uintptr_t)data;
	}
  // return 0;
}

IMAGE_NAMESPACE_END
