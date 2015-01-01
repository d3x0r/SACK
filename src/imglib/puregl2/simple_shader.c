#include <stdhdrs.h>

#ifdef USE_GLES2
//#include <GLES/gl.h>
#include <GLES2/gl2.h>
#else
#define GLEW_NO_GLU
#include <GL/glew.h>
#include <GL/gl.h>
//#include <GL/glu.h>
#endif

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
	"attribute  vec4 ex_Color;\n"
	// "varying vec4 out_Color;" 
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
	//struct shader_buffer *vert_pos;
	//struct shader_buffer *vert_color;
};

PTRSZVAL CPROC SimpleShader_OpInit( PImageShaderTracker tracker, PTRSZVAL psv, va_list args )
{
	struct simple_shader_op_data *newOp = New( struct simple_shader_op_data );
	newOp->vert_pos = CreateShaderBuffer( 3, 8, 16 );
	newOp->vert_color = CreateShaderBuffer( 4, 8, 16 );
	return (PTRSZVAL)newOp;
}

void CPROC SimpleShader_Flush( PImageShaderTracker tracker, PTRSZVAL psv, PTRSZVAL psvKey, int from, int to )
{
	struct simple_shader_data *data = (struct simple_shader_data *)psv;
	struct simple_shader_op_data *shaderOp = (struct simple_shader_op_data *)psvKey;
;
	EnableShader( tracker );
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glDisableVertexAttribArray(2);

	glVertexAttribPointer( 0, 3, GL_FLOAT, FALSE, 0, shaderOp->vert_pos->data );
	glVertexAttribPointer( 1, 4, GL_FLOAT, FALSE, 0, shaderOp->vert_color->data );
	//lprintf( WIDE("Set data %p %p  %d,%d"), data->vert_pos->data, data->vert_color->data, from,to );
	//CheckErr();
	//glUniform4fv( psv, 1, color );
	//CheckErr();
	glDrawArrays( GL_TRIANGLES, from, to- from);
	CheckErr();
	shaderOp->vert_pos->used = 0;
	shaderOp->vert_color->used = 0;
}

void CPROC SimpleShader_AppendTristrip( struct image_shader_op *op, int triangles, PTRSZVAL psv, va_list args )
{
	struct simple_shader_op_data *data = (struct simple_shader_op_data *)psv;
	float *verts = va_arg( args, float * );
	float *color = va_arg( args, float * );
	int tri;
	for( tri = 0; tri < triangles; tri++ )
	{
		if( !( tri & 1 ) )
		{
			// 0,1,2 
			// 2,3,4
			// 4,5,6
			// 0, 2, 4, 6
			AppendShaderData( op, data->vert_pos, verts + ( tri +  0 ) * 3 );
			AppendShaderData( op, data->vert_color, color);
			AppendShaderData( op, data->vert_pos, verts + ( tri + 1 ) * 3 );
			AppendShaderData( op, data->vert_color, color);
			AppendShaderData( op, data->vert_pos, verts + ( tri + 2 ) * 3 );
			AppendShaderData( op, data->vert_color, color);
		}
		else
		{
			// 2,1,3
			// 4,3,5
			AppendShaderData( op, data->vert_pos, verts + ( (tri-1) + 2 ) * 3 );
			AppendShaderData( op, data->vert_color, color);
			AppendShaderData( op, data->vert_pos, verts + ( (tri-1) + 1 ) * 3 );
			AppendShaderData( op, data->vert_color, color);
			AppendShaderData( op, data->vert_pos, verts + ( (tri-1) + 3 ) * 3 );
			AppendShaderData( op, data->vert_color, color);
		}
	}
}

void CPROC SimpleShader_AppendTristripQuad( PImageShaderTracker tracker, int triangles, PTRSZVAL psv, va_list args )
{
	SimpleShader_AppendTristrip( tracker, 2, psv, args );
}

PTRSZVAL SetupSuperSimpleShader( void )
{
	return 0;
}

void InitSuperSimpleShader( PImageShaderTracker tracker, PTRSZVAL psv )
{
	const char *v_codeblocks[2];
	const char *p_codeblocks[2];
	
	v_codeblocks[0] = gles_simple_v_shader;
	v_codeblocks[1] = NULL;
	p_codeblocks[0] = gles_simple_p_shader;
	p_codeblocks[1] = NULL;
	if( CompileShader( tracker, v_codeblocks, 1, p_codeblocks, 1 ) )
	{
		//struct simple_shader_data *data = (struct simple_shader_data *)psv;
		
		//if( data == NULL )
		{
			//data = (struct simple_shader_data *)SimpleShader_OpInit( tracker, psv, NULL );
			//data = New( struct simple_shader_data );
			//data->vert_pos = CreateShaderBuffer( 3, 8, 16 );
			//data->vert_color = CreateShaderBuffer( 4, 8, 16 );
		}
		///SetShaderEnable( tracker, SimpleShader_Enable );
		SetShaderOpInit( tracker, SimpleShader_OpInit );
		SetShaderFlush( tracker, SimpleShader_Flush );
		SetShaderAppendTristrip( tracker, SimpleShader_AppendTristrip );
		//return (PTRSZVAL)data;
	}
	//return 0;
}

IMAGE_NAMESPACE_END
