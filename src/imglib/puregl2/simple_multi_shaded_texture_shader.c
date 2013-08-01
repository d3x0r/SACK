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
static const CTEXTSTR gles_simple_v_multi_shader =
	WIDE( "uniform mat4 modelView;\n" )
	WIDE( "uniform mat4 worldView;\n" )
	WIDE( "uniform mat4 Projection;\n" )
	WIDE( "attribute vec2 in_texCoord;\n" )
    WIDE( "attribute vec4 vPosition;" )
	WIDE( " varying vec2 out_texCoord;\n" )
	//WIDE( "in  vec4 in_Color;\n")
	//WIDE( "out vec4 ex_Color;\n")
    WIDE( "void main(void) {" )
    WIDE( "  gl_Position = Projection * worldView * vPosition;" )
	WIDE( "out_texCoord = in_texCoord;\n" )
	//WIDE( "  ex_Color = in_Color;" )
    WIDE( "}"); 


static const CTEXTSTR gles_simple_p_multi_shader =
	    WIDE( " varying vec2 out_texCoord;\n" )
       WIDE( "uniform sampler2D tex;\n" )
       WIDE( "uniform vec4 multishade_r;\n" )
       WIDE( "uniform vec4 multishade_g;\n" )
       WIDE( "uniform vec4 multishade_b;\n" )
       WIDE( "\n" )
       WIDE( "void main(void)\n" )
       WIDE( "{\n" )
       WIDE( "    vec4 color = texture2D(tex, out_texCoord);\n" )
       WIDE( "	gl_FragColor = vec4( (color.b * multishade_b.r) + (color.g * multishade_g.r) + (color.r * multishade_r.r),\n" )
       WIDE( "		(color.b * multishade_b.g) + (color.g * multishade_g.g) + (color.r * multishade_r.g),\n" )
       WIDE( "		(color.b * multishade_b.b) + (color.g * multishade_g.b) + (color.r * multishade_r.b),\n" )
       WIDE( "		  color.r!=0.0?( color.a * multishade_r.a) :0.0\n" )
       WIDE( "                + color.g!=0.0?( color.a * multishade_g.a) :0.0\n" )
       WIDE( "                + color.b!=0.0?( color.a * multishade_b.a) :0.0\n" )
       WIDE( "                )\n" )
       WIDE( "		;\n" )
				WIDE( "}\n" );


struct private_mst_shader_data
{
	int texture_attrib;
	int texture;
	int r_color_attrib;
	int g_color_attrib;
	int b_color_attrib;
};


static void CPROC SimpleMultiShadedTextureEnable( PImageShaderTracker tracker, va_list args )
{
	float *verts = va_arg( args, float *);
	int texture = va_arg( args, int );
	float *texture_verts = va_arg( args, float *);
	float *r_color = va_arg( args, float *);
	float *g_color = va_arg( args, float *);
	float *b_color = va_arg( args, float *);

	struct private_mst_shader_data *data = (struct private_mst_shader_data *)tracker->psv_userdata;

	glEnableVertexAttribArray(0);	CheckErr();
	glVertexAttribPointer( 0, 3, GL_FLOAT, FALSE, 0, verts );  
	CheckErr();
	
	glEnableVertexAttribArray(data->texture_attrib);	CheckErr();
	glVertexAttribPointer( data->texture_attrib, 2, GL_FLOAT, FALSE, 0, texture_verts );  
	CheckErr();

	glUniform1i(data->texture, 0); //Texture unit 0 is for base images.
 
	//When rendering an objectwith this program.
	glActiveTexture(GL_TEXTURE0 + 0);
	CheckErr();
	glBindTexture(GL_TEXTURE_2D, texture);
	CheckErr();
	//glBindSampler(0, linearFiltering);
	CheckErr();

	glUniform4fv( data->r_color_attrib, 1, r_color );
	CheckErr();
	glUniform4fv( data->g_color_attrib, 1, g_color );
	CheckErr();
	glUniform4fv( data->b_color_attrib, 1, b_color );
	CheckErr();


}


void InitSimpleMultiShadedTextureShader( PImageShaderTracker tracker )
{
	GLint result;
	const char *codeblocks[2];
	struct private_mst_shader_data *data = New( struct private_mst_shader_data );

	tracker->psv_userdata = (PTRSZVAL)data;
	tracker->Enable = SimpleMultiShadedTextureEnable;

		if( result = glGetError() )
	{
		lprintf( "unhandled error before shader" );
	}

		tracker->glProgramId = glCreateProgram();

		//Obtain a valid handle to a vertex shader object.
		tracker->glVertexProgramId = glCreateShader(GL_VERTEX_SHADER);

		codeblocks[0] = gles_simple_v_multi_shader;
		codeblocks[1] = NULL;

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
			glGetShaderiv(tracker->glFragProgramId, GL_COMPILE_STATUS, &result);
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
				lprintf( "message: %s", buffer );


				if (final > length)
				{
					//The buffer does not contain all the shader log information.
					printf("Shader Log contained more information!\n");
				}
		
			}
		}

		tracker->glFragProgramId = glCreateShader(GL_FRAGMENT_SHADER);
		codeblocks[0] = gles_simple_p_multi_shader;
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
		glBindAttribLocation(tracker->glProgramId, 1, "in_TexCoord" );
		CheckErr();

		glLinkProgram(tracker->glProgramId);
		CheckErr();

		{
			glGetProgramiv(tracker->glProgramId, GL_LINK_STATUS, &result  );
			if( !result )
			{
				GLint length;
				GLsizei final;
				char *buffer;
				//We failed to compile.
				lprintf("Shader failed linking...\n");
				//Attempt to get the length of our error log.
#ifdef USE_GLES2
				glGetShaderiv(tracker->glProgramId, GL_INFO_LOG_LENGTH, &length);
#else
				glGetObjectParameterivARB(tracker->glProgramId, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
#endif
				buffer = NewArray( char, length );
				//Create a buffer.
					
				//Used to get the final length of the log.
#ifdef USE_GLES2
				glGetShaderInfoLog( tracker->glProgramId, length, &final, buffer);
#else
				glGetInfoLogARB(tracker->glProgramId, length, &final, buffer);
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
		glUseProgram(tracker->glProgramId);
		CheckErr();

		SetupCommon( tracker, "vPosition", "in_Color" );
		
		data->r_color_attrib = glGetUniformLocation(tracker->glProgramId, "multishade_r" );
		CheckErr();
		data->g_color_attrib = glGetUniformLocation(tracker->glProgramId, "multishade_g" );
		CheckErr();
		data->b_color_attrib = glGetUniformLocation(tracker->glProgramId, "multishade_b" );
		CheckErr();
		data->texture = glGetUniformLocation(tracker->glProgramId, "tex");
		CheckErr();
		data->texture_attrib =  glGetAttribLocation(tracker->glProgramId, "in_texCoord" );
		CheckErr();

		DumpAttribs( tracker->glProgramId );
}

