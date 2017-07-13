#include <stdhdrs.h>


#include "local.h"

#include "shaders.h"

IMAGE_NAMESPACE

//const char *gles_
static const char *gles_simple_v_shader =
	"precision mediump float;\n"
	"precision mediump int;\n"
	"uniform mat4 modelView;\n" 
	"uniform mat4 worldView;\n" 
	"uniform mat4 Projection;\n" 
	"attribute vec4 vPosition;" 
	"attribute vec4 in_Color;\n"
	"varying vec4 ex_Color;\n"
	"void main(void) {"
	"  gl_Position = Projection * worldView * modelView * vPosition;" 
	"  ex_Color = in_Color;"
	"}"; 

static const char *gles_simple_p_shader =
	"precision mediump float;\n"
	"precision mediump int;\n"
	//"uniform  vec4 in_Color;\n"
	"varying vec4 ex_Color;"
	"void main(void) {"
	"  gl_FragColor = ex_Color;"
	"}" ;

static const char *gles_simple_v_shader_1_30 =
	//"precision mediump float;\n"
	//"precision mediump int;\n"
	"uniform mat4 modelView;\n" 
	"uniform mat4 worldView;\n" 
	"uniform mat4 Projection;\n" 
	"attribute vec4 vPosition;" 
	"attribute vec4 in_Color;\n"
	"varying vec4 ex_Color;\n"
	"void main(void) {"
	"  gl_Position = Projection * worldView * modelView * vPosition;" 
	"  ex_Color = in_Color;"
	"}"; 

static const char *gles_simple_p_shader_1_30 =
	//"precision mediump float;\n"
	//"precision mediump int;\n"
	//"uniform  vec4 in_Color;\n"
	"varying vec4 ex_Color;"
	"void main(void) {"
	"  gl_FragColor = ex_Color;"
	"}" ;

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
};

uintptr_t CPROC SimpleShader_OpInit( PImageShaderTracker tracker, uintptr_t psv, int *existing_verts, va_list args )
{
	struct simple_shader_data *shader_data = (struct simple_shader_data*)psv;
	float *color = va_arg( args, float * );
	shader_data->next_color = color;
	*existing_verts = shader_data->data.vert_pos->used;
	return (uintptr_t)psv;
	/*
	struct simple_shader_op_data *newOp = New( struct simple_shader_op_data );
	newOp->vert_pos = CreateShaderBuffer( 3, 8, 16 );
	newOp->vert_color = CreateShaderBuffer( 4, 8, 16 );
	return (uintptr_t)newOp;
	*/
}

void CPROC SimpleShader_Output( PImageShaderTracker tracker, uintptr_t psv, uintptr_t psvKey, int from, int to )
{
	struct simple_shader_data *data = (struct simple_shader_data *)psv;
	//struct simple_shader_op_data *shaderOp = (struct simple_shader_op_data *)psvKey;
;
	EnableShader( tracker );
	//////glEnableVertexAttribArray(0);
	//////glEnableVertexAttribArray(1);
	//////glDisableVertexAttribArray(2);

	//////glVertexAttribPointer( 0, 3, GL_FLOAT, FALSE, 0, data->data.vert_pos->data );
	//////glVertexAttribPointer( 1, 4, GL_FLOAT, FALSE, 0, data->data.vert_color->data );
	//lprintf( WIDE("Set data %p %p  %d,%d"), data->vert_pos->data, data->vert_color->data, from,to );
	//CheckErr();
	//glUniform4fv( psv, 1, color );
	//CheckErr();
	//////glDrawArrays( GL_TRIANGLES, from, to- from);
	CheckErr();
}

void CPROC SimpleShader_Reset( PImageShaderTracker tracker, uintptr_t psv, uintptr_t psvKey )
{
	struct simple_shader_data *data = (struct simple_shader_data *)psv;
	//struct simple_shader_op_data *shaderOp = (struct simple_shader_op_data *)psvKey;

	data->data.vert_pos->used = 0;
	data->data.vert_color->used = 0;
}

void CPROC SimpleShader_AppendTristrip( struct image_shader_op *op, int triangles, uintptr_t psv, va_list args )
{
	struct simple_shader_data *data = (struct simple_shader_data *)psv;
	float *verts = va_arg( args, float * );
	//float *color = va_arg( args, float * );
	int tri;
	for( tri = 0; tri < triangles; tri++ )
	{
		if( !( tri & 1 ) )
		{
			// 0,1,2 
			// 2,3,4
			// 4,5,6
			// 0, 2, 4, 6
			AppendShaderData( op, data->data.vert_pos, verts + ( tri +  0 ) * 3 );
			AppendShaderData( op, data->data.vert_color, data->next_color);
			AppendShaderData( op, data->data.vert_pos, verts + ( tri + 1 ) * 3 );
			AppendShaderData( op, data->data.vert_color, data->next_color);
			AppendShaderData( op, data->data.vert_pos, verts + ( tri + 2 ) * 3 );
			AppendShaderData( op, data->data.vert_color, data->next_color);
		}
		else
		{
			// 2,1,3
			// 4,3,5
			AppendShaderData( op, data->data.vert_pos, verts + ( (tri-1) + 2 ) * 3 );
			AppendShaderData( op, data->data.vert_color, data->next_color);
			AppendShaderData( op, data->data.vert_pos, verts + ( (tri-1) + 1 ) * 3 );
			AppendShaderData( op, data->data.vert_color, data->next_color);
			AppendShaderData( op, data->data.vert_pos, verts + ( (tri-1) + 3 ) * 3 );
			AppendShaderData( op, data->data.vert_color, data->next_color);
		}
	}
}


uintptr_t SetupSuperSimpleShader( uintptr_t psv )
{
	struct simple_shader_data *data = New( struct simple_shader_data );
	MemSet( data, 0, sizeof( struct simple_shader_data ) );
	// no instance for simple tracker
	data->data.vert_pos = CreateShaderBuffer( 3, 8, 16 );
	data->data.vert_color = CreateShaderBuffer( 4, 8, 16 );
	return (uintptr_t)data;
}

void InitSuperSimpleShader( uintptr_t psv, PImageShaderTracker tracker )
{
	struct simple_shader_data *data = (struct simple_shader_data *)psv;
	const char *v_codeblocks[2];
	const char *p_codeblocks[2];

	if( TRUE ) //////l.glslVersion < 140 )
	{
		v_codeblocks[0] = gles_simple_v_shader_1_30;
		v_codeblocks[1] = NULL;
		p_codeblocks[0] = gles_simple_p_shader_1_30;
		p_codeblocks[1] = NULL;
	}
	else
	{
		v_codeblocks[0] = gles_simple_v_shader;
		v_codeblocks[1] = NULL;
		p_codeblocks[0] = gles_simple_p_shader;
		p_codeblocks[1] = NULL;
	}
	if( CompileShader( tracker, v_codeblocks, 1, p_codeblocks, 1 ) )
	{
		///////data->color_attrib = glGetAttribLocation(tracker->glProgramId, "in_Color" );
		SetShaderOpInit( tracker, SimpleShader_OpInit );
		SetShaderOutput( tracker, SimpleShader_Output );
		SetShaderReset( tracker, SimpleShader_Reset );
		SetShaderAppendTristrip( tracker, SimpleShader_AppendTristrip );
	}
}

IMAGE_NAMESPACE_END
