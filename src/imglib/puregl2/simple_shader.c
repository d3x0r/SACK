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

struct simple_shader_data
{
	int color_attrib;
	struct shader_buffer *vert_pos;
	struct shader_buffer *vert_color;
};

void CPROC SimpleShader_Enable( PImageShaderTracker tracker, PTRSZVAL psv, va_list args )
{
	//float *verts = va_arg( args, float * );
	//float *color = va_arg( args, float * );

	//glEnableVertexAttribArray(0);
	//glVertexAttribPointer( 0, 3, GL_FLOAT, FALSE, 0, verts );
	//CheckErr();
	//glUniform4fv( psv, 1, color );
	//CheckErr();
}

void CPROC SimpleShader_Flush( PImageShaderTracker tracker, PTRSZVAL psv )
{
	struct simple_shader_data *data = (struct simple_shader_data *)psv;
	EnableShader( tracker );
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer( 0, 3, GL_FLOAT, FALSE, 0, data->vert_pos->data );
	glVertexAttribPointer( 1, 4, GL_FLOAT, FALSE, 0, data->vert_color->data );
	//CheckErr();
	//glUniform4fv( psv, 1, color );
	//CheckErr();
	glDrawArrays( GL_TRIANGLES, 0, data->vert_pos->used );
	CheckErr();
	data->vert_pos->used = 0;
	data->vert_color->used = 0;
}

void CPROC SimpleShader_AppendTristrip( PImageShaderTracker tracker, int triangles, PTRSZVAL psv, va_list args )
{
	struct simple_shader_data *data = (struct simple_shader_data *)psv;
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
			AppendShaderData( data->vert_pos, verts + ( tri +  0 ) * 3 );
			AppendShaderData( data->vert_color, color);
			AppendShaderData( data->vert_pos, verts + ( tri + 1 ) * 3 );
			AppendShaderData( data->vert_color, color);
			AppendShaderData( data->vert_pos, verts + ( tri + 2 ) * 3 );
			AppendShaderData( data->vert_color, color);
		}
		else
		{
			// 2,1,3
			// 4,3,5
			AppendShaderData( data->vert_pos, verts + ( (tri-1) + 2 ) * 3 );
			AppendShaderData( data->vert_color, color);
			AppendShaderData( data->vert_pos, verts + ( (tri-1) + 1 ) * 3 );
			AppendShaderData( data->vert_color, color);
			AppendShaderData( data->vert_pos, verts + ( (tri-1) + 3 ) * 3 );
			AppendShaderData( data->vert_color, color);
		}
	}
}

void CPROC SimpleShader_AppendTristripQuad( PImageShaderTracker tracker, int triangles, PTRSZVAL psv, va_list args )
{
	SimpleShader_AppendTristrip( tracker, 2, psv, args );
}

PTRSZVAL InitSuperSimpleShader( PImageShaderTracker tracker, PTRSZVAL psv )
{
	const char *v_codeblocks[2];
	const char *p_codeblocks[2];
	int color_attrib;

	v_codeblocks[0] = gles_simple_v_shader;
	v_codeblocks[1] = NULL;
	p_codeblocks[0] = gles_simple_p_shader;
	p_codeblocks[1] = NULL;
	if( CompileShader( tracker, v_codeblocks, 1, p_codeblocks, 1 ) )
	{
		struct simple_shader_data *data = psv;
		if( data == NULL )
			data = New( struct simple_shader_data );
		data->color_attrib = glGetUniformLocation(tracker->glProgramId, "in_Color" );
		data->vert_pos = CreateBuffer( 3, 8, 16 );
		data->vert_color = CreateBuffer( 4, 8, 16 );

		SetShaderEnable( tracker, SimpleShader_Enable );
		SetShaderFlush( tracker, SimpleShader_Flush );
		SetShaderAppendTristrip( tracker, SimpleShader_AppendTristrip );
		return (PTRSZVAL)data;
	}
}

IMAGE_NAMESPACE_END
