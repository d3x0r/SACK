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
    WIDE( "  gl_Position = Projection * worldView * vPosition;" )
	//WIDE( "  ex_Color = in_Color;" )
    WIDE( "}"); 

static const CTEXTSTR gles_simple_p_shader =
	WIDE( "uniform  vec4 in_Color;\n" )
	//WIDE( "varying vec4 out_Color;" )
    WIDE( "void main(void) {" )
	WIDE( "  gl_FragColor = in_Color;" )
    WIDE( "}" );


void CPROC EnableSimpleShader( PImageShaderTracker tracker, va_list args )
{
	float *verts = va_arg( args, float * );
	float *color = va_arg( args, float * );

	glEnableVertexAttribArray(0);	glVertexAttribPointer( 0, 3, GL_FLOAT, FALSE, 0, verts );  
	CheckErr();
	glUniform4fv( tracker->color_attrib, 1, color );
	CheckErr();

}

void InitSuperSimpleShader( PImageShaderTracker tracker )
{
	GLint result;
	const char *codeblocks[2];

	tracker->psv_userdata = 0;
	tracker->Enable = EnableSimpleShader;

		tracker->glProgramId = glCreateProgram();

		//Obtain a valid handle to a vertex shader object.
		tracker->glVertexProgramId = glCreateShader(GL_VERTEX_SHADER);

		codeblocks[0] = gles_simple_v_shader;
		codeblocks[1] = NULL;

		//codeblocks[1] = gles_simple_p_shader;
		//Now, compile the shader source. 
		//Note that glShaderSource takes an array of chars. This is so that one can load multiple vertex shader files at once.
		//This is similar in function to linking multiple C++ files together. Note also that there can only be one "void main" definition
		//In all of the linked source files that are compiling with this funciton.
		glShaderSource(
			tracker->glVertexProgramId, //The handle to our shader
			1, //The number of files.
			codeblocks, //An array of const char * data, which represents the source code of theshaders
			NULL); //An array of string leng7ths. For have null terminated strings, pass NULL.
	 
		//Attempt to compile the shader.
		glCompileShader(tracker->glVertexProgramId);
		{
			//Error checking.
#ifdef USE_GLES2
			glGetShaderiv(tracker->glVertexProgramId, GL_COMPILE_STATUS, &result);
#else
			glGetObjectParameterivARB(tracker->glVertexProgramId, GL_OBJECT_COMPILE_STATUS_ARB, &result);
#endif
			if (!result)
			{
				GLint length;
				GLsizei final;
				char *buffer;
				//We failed to compile.
				lprintf("Vertex shader 'program A' failed compilation.\n");
				//Attempt to get the length of our error log.
#ifdef USE_GLES2
            lprintf( "length starts at %d", length );
				glGetShaderiv(tracker->glVertexProgramId, GL_INFO_LOG_LENGTH, &length);

#else
				glGetObjectParameterivARB(tracker->glVertexProgramId, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
#endif
				buffer = NewArray( char, length );
				//Create a buffer.
					
				//Used to get the final length of the log.
#ifdef USE_GLES2
				glGetShaderInfoLog( tracker->glVertexProgramId, length, &final, buffer);
#else
				glGetInfoLogARB(tracker->glVertexProgramId, length, &final, buffer);
#endif
				//Convert our buffer into a string.
				lprintf( "message: (%d of %d)%s",  final, length, buffer );


				if (final > length)
				{
					//The buffer does not contain all the shader log information.
					printf("Shader Log contained more information!\n");
				}
		
			}
		}

		tracker->glFragProgramId = glCreateShader(GL_FRAGMENT_SHADER);
		codeblocks[0] = gles_simple_p_shader;
		glShaderSource(
			tracker->glFragProgramId, //The handle to our shader
			1, //The number of files.
			codeblocks, //An array of const char * data, which represents the source code of theshaders
			NULL); //An array of string lengths. For have null terminated strings, pass NULL.
	 
		//Attempt to compile the shader.
		glCompileShader(tracker->glFragProgramId);

		{
			//Error checking.
#ifdef USE_GLES2
			glGetShaderiv(tracker->glFragProgramId, GL_COMPILE_STATUS, &result);
#else
			glGetObjectParameterivARB(tracker->glFragProgramId, GL_OBJECT_COMPILE_STATUS_ARB, &result);
#endif
			if (!result)
			{
				GLint length;
				GLsizei final;
				char *buffer;
				//We failed to compile.
				lprintf("Vertex shader 'program B' failed compilation.\n");
				//Attempt to get the length of our error log.
#ifdef USE_GLES2
				glGetShaderiv(tracker->glVertexProgramId, GL_INFO_LOG_LENGTH, &length);
#else
				glGetObjectParameterivARB(tracker->glFragProgramId, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
#endif
				buffer = NewArray( char, length );
				//Create a buffer.
					
				//Used to get the final length of the log.
#ifdef USE_GLES2
				glGetShaderInfoLog( tracker->glFragProgramId, length, &final, buffer);
#else
				glGetInfoLogARB(tracker->glFragProgramId, length, &final, buffer);
#endif
				//Convert our buffer into a string.
				lprintf( "message: %s", buffer );


				if (final > length)
				{
					//The buffer does not contain all the shader log information.
					printf("Shader Log contained more information!\n");
				}
		
			}
		}
#ifdef USE_GLES2
		glAttachShader(tracker->glProgramId, tracker->glVertexProgramId );
#else
		glAttachObjectARB(tracker->glProgramId, tracker->glVertexProgramId );
#endif
		CheckErr();
#ifdef USE_GLES2
		glAttachShader(tracker->glProgramId, tracker->glFragProgramId );
#else
		glAttachObjectARB(tracker->glProgramId, tracker->glFragProgramId );
#endif
		CheckErr();

		glBindAttribLocation(tracker->glProgramId, 0, "vPosition" );
		CheckErr();
		glBindAttribLocation(tracker->glProgramId, 1, "in_Color" );
		CheckErr();

		glLinkProgram(tracker->glProgramId);
		CheckErr();
		glUseProgram(tracker->glProgramId);
		CheckErr();

      SetupCommon( tracker, "vPosition", "in_Color" );
	  // SetupCommon reads color_attrib as an attribute, instead of a uniform, so get the uniform instead.
	  tracker->color_attrib = glGetUniformLocation(tracker->glProgramId, "in_Color" );

	  DumpAttribs( tracker->glProgramId );
}
