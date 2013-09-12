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


//const char *gles_
static const CTEXTSTR gles_simple_v_shader =
	WIDE( "uniform mat4 modelView;\n" )
	WIDE( "uniform mat4 worldView;\n" )
	WIDE( "uniform mat4 Projection;\n" )
    WIDE( "attribute vec4 vPosition;" )
	//WIDE( "in  vec4 in_Color;\n")
	//WIDE( "out vec4 ex_Color;\n")
    WIDE( "void main(void) {" )
    WIDE( "  gl_Position = Projection * worldView * modelView * vPosition;" )
	//WIDE( "  ex_Color = in_Color;" )
    WIDE( "}"); 

static const CTEXTSTR gles_simple_p_shader =
	WIDE( "uniform  vec4 in_Color;\n" )
	//WIDE( "varying vec4 out_Color;" )
    WIDE( "void main(void) {" )
	WIDE( "  gl_FragColor = in_Color;" )
    WIDE( "}" );


void CPROC EnableSimpleShader( PImageShaderTracker tracker, PTRSZVAL psv, va_list args )
{
	float *verts = va_arg( args, float * );
	float *color = va_arg( args, float * );

	glEnableVertexAttribArray(0);
	glVertexAttribPointer( 0, 3, GL_FLOAT, FALSE, 0, verts );
	CheckErr();
	glUniform4fv( psv, 1, color );
	CheckErr();

}

void InitSuperSimpleShader( PImageShaderTracker tracker )
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
		color_attrib = glGetUniformLocation(tracker->glProgramId, "in_Color" );
		SetShaderEnable( tracker, EnableSimpleShader, color_attrib );
	}
}
