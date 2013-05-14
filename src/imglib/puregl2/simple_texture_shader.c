#include <stdhdrs.h>

#ifdef __ANDROID__
#include <GLES/gl.h>
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
    WIDE( "attribute vec4 vPosition;\n" )
	WIDE( "attribute vec4 in_Color;\n" )
	WIDE( "attribute vec2 in_texCoord;\n" )
	WIDE( "uniform mat4 modelView;\n" )
	WIDE( "uniform mat4 worldView;\n" )
	WIDE( "uniform mat4 Projection;\n" )
	WIDE( " varying vec4 vColor;\n" )
	WIDE( " varying vec2 out_texCoord;\n" )
    WIDE("void main() {\n" )
    WIDE("  gl_Position = Projection * worldView * vPosition;\n" )
	WIDE( " vColor = in_Color;\n" )
	WIDE( "out_texCoord = in_texCoord;\n" )
    WIDE("}\n"); 

static const CTEXTSTR gles_simple_p_shader =
    //WIDE( "precision mediump float;\n" )
	WIDE( " varying vec4 vColor;\n" )
	WIDE( " varying vec2 out_texCoord;\n" )
	WIDE( " uniform sampler2D tex;\n" )
	WIDE( "void main() {\n" )
	WIDE( "   gl_FragColor = texture2D( tex, out_texCoord );\n" )
    WIDE( "}\n" );


//const char *gles_
static const CTEXTSTR gles_simple_v_shader_shaded_texture =
    WIDE( "attribute vec4 vPosition;\n" )
	WIDE( "attribute vec2 in_texCoord;\n" )
	WIDE( "uniform mat4 modelView;\n" )
	WIDE( "uniform mat4 worldView;\n" )
	WIDE( "uniform mat4 Projection;\n" )
	WIDE( "varying vec2 out_texCoord;\n" )
	WIDE( " \n" )
    WIDE("void main() {\n" )
    WIDE("  gl_Position = Projection * worldView * vPosition;\n" )
	WIDE( "out_texCoord = in_texCoord;\n" )
    WIDE("}"); 

static const CTEXTSTR gles_simple_p_shader_shaded_texture =
    //WIDE( "precision mediump float;\n" )
	WIDE( "uniform vec4 in_Color;\n" )
	WIDE( " varying vec2 out_texCoord;\n" )
	WIDE( " uniform sampler2D tex;\n" )
	WIDE( "void main() {\n" )
	WIDE( "   gl_FragColor = in_Color * texture2D( tex, out_texCoord );\n" )
    WIDE( "}\n" );


struct private_shader_data
{
	int texture_attrib;
	int texture;
};

static void CPROC SimpleTextureEnable( PImageShaderTracker tracker, va_list args )
{
	float *verts = va_arg( args, float *);
	int texture = va_arg( args, int);
	float *texture_verts = va_arg( args, float *);
	struct private_shader_data *data = (struct private_shader_data *)tracker->psv_userdata;

	//glUniform4fv( tracker->color_attrib, 1, GL_FALSE, color );
	glEnableVertexAttribArray(0);

	glVertexAttribPointer( 0, 3, GL_FLOAT, FALSE, 0, verts );
	CheckErr();
	glEnableVertexAttribArray(data->texture_attrib);	CheckErr();
	glVertexAttribPointer( data->texture_attrib, 2, GL_FLOAT, FALSE, 0, texture_verts );            
	CheckErr();
	glActiveTexture(GL_TEXTURE0 + 0);
	CheckErr();
	glBindTexture(GL_TEXTURE_2D+0, texture);
	CheckErr();
	glUniform1i( data->texture, 0 );
	CheckErr();


}

void InitSimpleTextureShader( PImageShaderTracker tracker )
{
	GLint result;
	const char *codeblocks[2];
	struct private_shader_data *data = New( struct private_shader_data );

	tracker->psv_userdata = (PTRSZVAL)data;
	tracker->Enable = SimpleTextureEnable;

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
#ifdef __ANDROID__
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
#ifdef __ANDROID__
				glGetShaderiv(tracker->glVertexProgramId, GL_INFO_LOG_LENGTH, &length);
#else
				glGetObjectParameterivARB(tracker->glVertexProgramId, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
#endif
				buffer = NewArray( char, length );
				//Create a buffer.
					
				//Used to get the final length of the log.
#ifdef __ANDROID__
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
#ifdef __ANDROID__
			glGetShaderiv(tracker->glVertexProgramId, GL_COMPILE_STATUS, &result);
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
#ifdef __ANDROID__
				glGetShaderiv(tracker->glVertexProgramId, GL_INFO_LOG_LENGTH, &length);
#else
				glGetObjectParameterivARB(tracker->glFragProgramId, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
#endif
				buffer = NewArray( char, length );
				//Create a buffer.
					
				//Used to get the final length of the log.
#ifdef __ANDROID__
				glGetShaderInfoLog( tracker->glVertexProgramId, length, &final, buffer);
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

#ifdef __ANDROID__
		glAttachShader(tracker->glProgramId, tracker->glVertexProgramId );
#else
		glAttachObjectARB(tracker->glProgramId, tracker->glVertexProgramId );
#endif
		CheckErr();
#ifdef __ANDROID__
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
		glUseProgram(tracker->glProgramId);
		CheckErr();

      SetupCommon( tracker, "vPosition", "in_Color" );

	  data->texture = glGetUniformLocation(tracker->glProgramId, "tex");
	  data->texture_attrib =  glGetAttribLocation(tracker->glProgramId, "in_texCoord" );

		DumpAttribs( tracker->glProgramId );
}

static void CPROC SimpleTextureEnable2( PImageShaderTracker tracker, va_list args )
{
	float *verts = va_arg( args, float *);
	int texture = va_arg( args, int );
	float *texture_verts = va_arg( args, float *);
	float *color = va_arg( args, float *);

	struct private_shader_data *data = (struct private_shader_data *)tracker->psv_userdata;

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
	glBindTexture(GL_TEXTURE_2D+0, texture);
	CheckErr();
	glUniform1i( data->texture, 0 );
	CheckErr();
	//glBindSampler(0, linearFiltering);
	CheckErr();

	CheckErr();
	glUniform4fv( tracker->color_attrib, 1, color );
	CheckErr();


}


void InitSimpleShadedTextureShader( PImageShaderTracker tracker )
{
	GLint result;
	const char *codeblocks[2];
	struct private_shader_data *data = New( struct private_shader_data );

	tracker->psv_userdata = (PTRSZVAL)data;
	tracker->Enable = SimpleTextureEnable2;

		tracker->glProgramId = glCreateProgram();

		//Obtain a valid handle to a vertex shader object.
		tracker->glVertexProgramId = glCreateShader(GL_VERTEX_SHADER);

		codeblocks[0] = gles_simple_v_shader_shaded_texture;
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
#ifdef __ANDROID__
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
#ifdef __ANDROID__
				glGetShaderiv(tracker->glVertexProgramId, GL_INFO_LOG_LENGTH, &length);
#else
				glGetObjectParameterivARB(tracker->glVertexProgramId, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
#endif
				buffer = NewArray( char, length );
				//Create a buffer.
					
				//Used to get the final length of the log.
#ifdef __ANDROID__
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
		codeblocks[0] = gles_simple_p_shader_shaded_texture;
		glShaderSource(
			tracker->glFragProgramId, //The handle to our shader
			1, //The number of files.
			codeblocks, //An array of const char * data, which represents the source code of theshaders
			NULL); //An array of string lengths. For have null terminated strings, pass NULL.
	 
		//Attempt to compile the shader.
		glCompileShader(tracker->glFragProgramId);

		{
			//Error checking.
#ifdef __ANDROID__
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
#ifdef __ANDROID__
				glGetShaderiv(tracker->glVertexProgramId, GL_INFO_LOG_LENGTH, &length);
#else
				glGetObjectParameterivARB(tracker->glFragProgramId, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
#endif
				buffer = NewArray( char, length );
				//Create a buffer.
					
				//Used to get the final length of the log.
#ifdef __ANDROID__
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

#ifdef __ANDROID__
		glAttachShader(tracker->glProgramId, tracker->glVertexProgramId );
#else
		glAttachObjectARB(tracker->glProgramId, tracker->glVertexProgramId );
#endif
		CheckErr();
#ifdef __ANDROID__
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
		glUseProgram(tracker->glProgramId);
		CheckErr();

		SetupCommon( tracker, "vPosition", "in_Color" );

		tracker->color_attrib = glGetUniformLocation(tracker->glProgramId, "in_Color" );
		data->texture = glGetUniformLocation(tracker->glProgramId, "tex");
		data->texture_attrib =  glGetAttribLocation(tracker->glProgramId, "in_texCoord" );

		DumpAttribs( tracker->glProgramId );
}

