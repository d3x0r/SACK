#define NO_FILEOP_ALIAS
#include <stdhdrs.h>
#define USE_RENDER_INTERFACE l.pri
#define USE_IMAGE_INTERFACE l.pii
#define NEED_VECTLIB_COMPARE
#define MAKE_RCOORD_SINGLE
#include <sqlgetoption.h>
#include <math.h>
#include <render.h>
#include <render3d.h>
#include <vectlib.h>
#include <sharemem.h>
#include <psi.h>

#include <btBulletDynamicsCommon.h>

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


//const char *gles_
const CTEXTSTR gles_simple_v_shader =
    WIDE( "attribute vec4 vPosition;" )
	WIDE( "attribute vec4 in_Color;" )
	WIDE( "uniform mat4 modelView;\n" )
	WIDE( "uniform mat4 worldView;\n" )
	WIDE( "uniform mat4 Projection;\n" )
	WIDE( " varying vec4 vColor;" )
    WIDE("void main() {" )
    WIDE("  gl_Position = Projection * worldView * modelView * vPosition;" )
	WIDE( " vColor = in_Color;" )
    WIDE("}"); 

const CTEXTSTR gles_simple_p_shader =
    WIDE( "precision mediump float;" )
	WIDE( " varying vec4 vColor;" )
    WIDE( "void main() {" )
    WIDE( "  gl_FragColor = vColor;" )
    WIDE( "}" );


void InitSuperSimpleShader( void )
{
	GLint result;
		const char *codeblocks[2];
		l.shader.extra_simple_shader.shader = glCreateProgram();

		//Obtain a valid handle to a vertex shader object.
		l.shader.extra_simple_shader.vert_shader = glCreateShader(GL_VERTEX_SHADER);

		codeblocks[0] = gles_simple_v_shader;
		codeblocks[1] = NULL;

		//codeblocks[1] = gles_simple_p_shader;
		//Now, compile the shader source. 
		//Note that glShaderSource takes an array of chars. This is so that one can load multiple vertex shader files at once.
		//This is similar in function to linking multiple C++ files together. Note also that there can only be one "void main" definition
		//In all of the linked source files that are compiling with this funciton.
		glShaderSource(
			l.shader.extra_simple_shader.vert_shader, //The handle to our shader
			1, //The number of files.
			codeblocks, //An array of const char * data, which represents the source code of theshaders
			NULL); //An array of string leng7ths. For have null terminated strings, pass NULL.
	 
		//Attempt to compile the shader.
		glCompileShader(l.shader.extra_simple_shader.vert_shader);
		{
			//Error checking.
#ifdef __ANDROID__
			glGetObjectParameterivARB(l.shader.extra_simple_shader.vert_shader, GL_COMPILE_STATUS, &result);
#else
			glGetObjectParameterivARB(l.shader.extra_simple_shader.vert_shader, GL_OBJECT_COMPILE_STATUS_ARB, &result);
#endif
			if (!result)
			{
				GLint length;
				GLsizei final;
				char *buffer;
				//We failed to compile.
				lprintf("Vertex shader 'program A' failed compilation.\n");
				//Attempt to get the length of our error log.
				glGetObjectParameterivARB(l.shader.extra_simple_shader.vert_shader, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
				buffer = NewArray( char, length );
				//Create a buffer.
					
				//Used to get the final length of the log.
				glGetInfoLogARB(l.shader.extra_simple_shader.vert_shader, length, &final, buffer);
				//Convert our buffer into a string.
				lprintf( "message: %s", buffer );


				if (final > length)
				{
					//The buffer does not contain all the shader log information.
					printf("Shader Log contained more information!\n");
				}
		
			}
		}

		l.shader.extra_simple_shader.frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
		codeblocks[0] = gles_simple_p_shader;
		glShaderSource(
			l.shader.extra_simple_shader.frag_shader, //The handle to our shader
			1, //The number of files.
			codeblocks, //An array of const char * data, which represents the source code of theshaders
			NULL); //An array of string lengths. For have null terminated strings, pass NULL.
	 
		//Attempt to compile the shader.
		glCompileShader(l.shader.extra_simple_shader.frag_shader);

		{
			//Error checking.
			glGetObjectParameterivARB(l.shader.extra_simple_shader.frag_shader, GL_OBJECT_COMPILE_STATUS_ARB, &result);
			if (!result)
			{
				GLint length;
				GLsizei final;
				char *buffer;
				//We failed to compile.
				lprintf("Vertex shader 'program B' failed compilation.\n");
				//Attempt to get the length of our error log.
				glGetObjectParameterivARB(l.shader.extra_simple_shader.frag_shader, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
				buffer = NewArray( char, length );
				//Create a buffer.
					
				//Used to get the final length of the log.
				glGetInfoLogARB(l.shader.extra_simple_shader.frag_shader, length, &final, buffer);
				//Convert our buffer into a string.
				lprintf( "message: %s", buffer );


				if (final > length)
				{
					//The buffer does not contain all the shader log information.
					printf("Shader Log contained more information!\n");
				}
		
			}
		}

		glAttachObjectARB(l.shader.extra_simple_shader.shader, l.shader.extra_simple_shader.vert_shader );
		CheckErr();
		glAttachObjectARB(l.shader.extra_simple_shader.shader, l.shader.extra_simple_shader.frag_shader );
		CheckErr();
		
		glBindAttribLocation(l.shader.extra_simple_shader.shader, 0, "vPosition");
		CheckErr();
		glBindAttribLocation(l.shader.extra_simple_shader.shader, 1, "in_Color");
		CheckErr();

		glLinkProgram(l.shader.extra_simple_shader.shader);
		CheckErr();
		glUseProgram(l.shader.extra_simple_shader.shader);
		CheckErr();

		lprintf( "X = %d", glGetAttribLocation( l.shader.extra_simple_shader.shader, "vPosition" ) );
		lprintf( "X = %d", glGetAttribLocation( l.shader.extra_simple_shader.shader, "in_Color" ) );

		l.shader.extra_simple_shader.eye_point
			=  glGetUniformLocation(l.shader.extra_simple_shader.shader, "in_eye_point" );
		CheckErr();

		l.shader.extra_simple_shader.global_ambient 
			=  glGetUniformLocation(l.shader.extra_simple_shader.shader, "vColor");
		CheckErr();

		l.shader.extra_simple_shader.projection
			= glGetUniformLocation(l.shader.extra_simple_shader.shader, "Projection");
		CheckErr();
		l.shader.extra_simple_shader.worldview
			= glGetUniformLocation(l.shader.extra_simple_shader.shader, "worldView");
		CheckErr();
		l.shader.extra_simple_shader.modelview
			= glGetUniformLocation(l.shader.extra_simple_shader.shader, "modelView");
		CheckErr();

		// projection should be constant?
		glUniformMatrix4fv( l.shader.extra_simple_shader.projection, 1, GL_FALSE, l.projection );
		CheckErr();
		glUniform4f( l.shader.extra_simple_shader.global_ambient, 0.5, 0.5, 0.5, 1.0 );
		CheckErr();

}


void InitShader( void )
{
	GLint result;

#ifndef __ANDROID__
		// should be able to do this without glew.
		if (GLEW_OK != glewInit() )
		{
			// okay let's just init glew.
			return;
		}
#endif
	InitSuperSimpleShader();

	const char *common_vertex_transform_source = "#version 150\n"
	                                         "uniform mat4 worldView;\n"
	                                         "uniform mat4 modelView;\n"
	                                         "uniform mat4 Projection;\n"
	                                         "uniform vec3 in_eye_point;\n"
	                                         "in  vec3 in_Position;\n"
											 "in  vec3 in_Normal;\n"
	                                   
	                                         "uniform  vec4 in_GlobalAmbient;\n"

	                                         "uniform  vec4 in_MaterialAmbient;\n"

	                                         "uniform  vec3 in_LightPosition1;\n"
	                                         "uniform  vec4 in_LightAmbient1;\n"
	                                         
	                                         "uniform  vec3 in_LightPosition2;\n"
	                                         "uniform  vec4 in_LightAmbient2;\n"
	                                         
	                                         "void setpos(void){ \n"
											 "  gl_Position = Projection * worldView * modelView * vec4(in_Position,1.0);\n"
											 "}\n"
											 "\n";
	
	const char *layer_texture_source = "in  vec3 in_Tangent;\n"
									   "in  vec3 in_Texture;\n"
									   ;
	
	const char *simple_color_vertex_source = "\n"
	                                         "// input parameters for our vertex shader\n"
	                                         "out vec4 diffuse,ambient;\n"
											 "out vec3 normal,lightDir,lightDir2,halfVector,halfVector2;\n"
	                                         "\n"
	                                         "\n"
	                                         "/**\n"
	                                         " * Vertex Shader\n"
	                                         " */\n"
	                                         "void main() {\n"
	                                         "  /**\n"
	                                         "   * We transform each vertex by the world view projection matrix to bring\n"
	                                         "   * it from world space to projection space.\n"
	                                         "   *\n"
	                                         "   * We return its color unchanged.\n"
	                                         "   */\n"
											 //"  vec3 eye_point = -vec3( worldView[3] );\n"
											 "  setpos();\n"
											 "  vec3 world_vertex = vec3( modelView * vec4( in_Position, 1.0 ) );\n"
											 "  normal = normalize(mat3(modelView) * in_Normal);\n"
											 "  lightDir = in_LightPosition1 - world_vertex;\n"
											 "  lightDir2 = in_LightPosition2 - world_vertex;\n"

											 "  vec3 tmpV = in_eye_point - world_vertex;\n"
											 "  halfVector = normalize( (lightDir+tmpV)/length(lightDir+tmpV) );\n"
											 "  halfVector2 = normalize( (lightDir2+tmpV)/length(lightDir2+tmpV) );\n"

											 "  /* Compute the diffuse, ambient and globalAmbient terms */\n"
											 //"  diffuse = in_MaterialDiffuse * in_LightDiffuse1 * in_LightDiffuse2;\n"
											 "  ambient = in_MaterialAmbient * in_LightAmbient1 + in_MaterialAmbient * in_LightAmbient2;\n"
											 "  ambient += in_GlobalAmbient * in_MaterialAmbient;\n"
	
											 "}\n";

	const char *simple_color_pixel_source =  "#version 140\n"
	                                         "// #o3d SplitMarker\n"
	                                         "/**\n"
	                                         " * Pixel Shader - pixel shader does nothing but return the color.\n"
	                                         " */\n"
	                                         "uniform  vec4 in_MaterialDiffuse;\n"
											 "uniform vec4 in_MaterialSpecular;\n"
	                                         "uniform  float in_MaterialShine;\n"
	                                         "uniform  vec4 in_LightSpecular1;\n"
	                                         "uniform  vec4 in_LightSpecular2;\n"
											 "uniform  vec4 in_LightDiffuse1;\n"
	                                         "uniform  vec4 in_LightDiffuse2;\n"
	                                         "in vec4 diffuse,ambient;\n"
											 "in vec3 normal,lightDir,halfVector,lightDir2,halfVector2;\n"
	                                         "out vec4 out_Color;\n"
	                                         "void main() {\n"
											 "    vec3 n;\n"
											 "	  float NdotL,NdotHV;\n"
		
											 "    /* The ambient term will always be present */\n"
											 "	  vec4 color = ambient;\n"
		
											 "		/* a fragment shader can't write a varying variable, hence we need\n"
											 "		a new variable to store the normalized interpolated normal */\n"
											 "      n = normalize( normal );\n"
											 "		/* compute the dot product between normal and ldir */\n"
											 "		NdotL = max(dot(n,normalize(lightDir)),0.0);\n"
											 "	if (NdotL > 0.0) {\n"
											 "			color += in_MaterialDiffuse * in_LightDiffuse1 * NdotL;\n"
											 "			NdotHV = max(dot(n,halfVector),0.0);\n"
											 "			color += in_MaterialSpecular * \n"
											 "					in_LightSpecular1 * \n"
											 "					pow(NdotHV, in_MaterialShine);\n"
											 "		}\n"
											 "		NdotL = max(dot(n,normalize(lightDir2)),0.0);\n"
											 "	if (NdotL > 0.0) {\n"
											 "			color += in_MaterialDiffuse * in_LightDiffuse2 * NdotL;\n"
											 "			NdotHV = max(dot(n,halfVector2),0.0);\n"
											 "			color += in_MaterialSpecular * \n"
											 "					in_LightSpecular2 * \n"
											 "					pow(NdotHV, in_MaterialShine);\n"
											 "		}\n"
											 "	out_Color = color;\n"
	                                         "}\n";


	const char *bump_texture_color_vertex_source = "\n"
	                                         "// input parameters for our vertex shader\n"
	                                         "out vec4 diffuse,ambient;\n"
											 "out vec2 texcoord;\n"
											 "out vec3 lightDir,lightDir2,halfVector,halfVector2;\n"
	                                         "\n"
	                                         "void main() {\n"
											 //"  vec3 eye_point = -vec3( worldView[3] );\n"
											 "  setpos();\n"

											 "  texcoord = in_Texture.xy;\n"
											 "  mat4 inv_modelView = inverse( modelView );\n"
											 
											 //"  vec3 world_vertex = in_Position;\n"
											 "  mat3 rotmat    = (mat3(in_Tangent,cross(in_Tangent,in_Normal),in_Normal));\n"
											 "  vec3 eye_point = rotmat * ( (inv_modelView * vec4( in_eye_point, 1.0 )).xyz - in_Position );\n"

											 "  //Rotate the light into tangent space\n"
											 "  lightDir = rotmat * ( (inv_modelView * vec4( in_LightPosition1, 1.0 ) ).xyz - in_Position);\n"
											 "  lightDir2 = rotmat * ( (inv_modelView * vec4( in_LightPosition2, 1.0 ) ).xyz - in_Position );\n"

											 "  halfVector = normalize( (lightDir+eye_point)/length(lightDir+eye_point) );\n"
											 "  halfVector2 = normalize( (lightDir2+eye_point)/length(lightDir2+eye_point) );\n"

											 "  /* Compute the diffuse, ambient and globalAmbient terms */\n"
											 //"  diffuse = in_MaterialDiffuse * in_LightDiffuse1 * in_LightDiffuse2;\n"
											 "  ambient = in_MaterialAmbient * in_LightAmbient1 + in_MaterialAmbient * in_LightAmbient2;\n"
											 "  ambient += in_GlobalAmbient * in_MaterialAmbient;\n"

											 "}\n";

	const char *bump_texture_color_pixel_source =  "#version 140\n"
	                                         "// #o3d SplitMarker\n"
	                                         "/**\n"
	                                         " * Pixel Shader - pixel shader does nothing but return the color.\n"
	                                         " */\n"
	                                         "uniform sampler2D colorMap;\n"
	                                         "uniform sampler2D normalMap;\n"
	                                         "uniform  vec4 in_MaterialDiffuse;\n"
											 "uniform vec4 in_MaterialSpecular;\n"
	                                         "uniform  float in_MaterialShine;\n"
	                                         "uniform  vec4 in_LightSpecular1;\n"
	                                         "uniform  vec4 in_LightSpecular2;\n"
											 "uniform  vec4 in_LightDiffuse1;\n"
	                                         "uniform  vec4 in_LightDiffuse2;\n"
	                                         "in vec4 diffuse,ambient;\n"
											 "in vec2 texcoord;\n"
											 "in vec3 lightDir,halfVector,lightDir2,halfVector2;\n"
	                                         "out vec4 out_Color;\n"
	                                         "void main() {\n"
											 "    vec3 n;\n"
											 "	  float NdotL,NdotHV;\n"
		
											 "    /* The ambient term will always be present */\n"
											 "    vec4 base = texture2D(colorMap, texcoord);\n"
											 "	  vec4 color = mix( ambient, base, base.a );\n"
											 "    vec3 bump = normalize( texture2D(normalMap, texcoord).xyz * 2.0 - 1.0);\n"
											 "    bump = mix( vec3(0.0,0.0,1.0), bump, base.a );\n"
		
											 "		/* a fragment shader can't write a varying variable, hence we need\n"
											 "		a new variable to store the normalized interpolated normal */\n"
											 "      n = normalize( lightDir );\n"
											 "		/* compute the dot product between normal and ldir */\n"
											 "		NdotL = max(dot(bump,normalize(n)),0.0);\n"
											 "	if (NdotL > 0.0) {\n"
											 "			color += in_MaterialDiffuse * in_LightDiffuse1 * NdotL;\n"
											 "			NdotHV = max(dot(bump,halfVector),0.0);\n"
											 "			color += in_MaterialSpecular * \n"
											 "					in_LightSpecular1 * \n"
											 "					pow(NdotHV, in_MaterialShine);\n"
											 "		}\n"
											 "      n = normalize( lightDir2 );\n"
											 "		NdotL = max(dot(bump,normalize(n)),0.0);\n"
											 "	if (NdotL > 0.0) {\n"
											 "			color += in_MaterialDiffuse * in_LightDiffuse2 * NdotL;\n"
											 "			NdotHV = max(dot(bump,halfVector2),0.0);\n"
											 "			color += in_MaterialSpecular * \n"
											 "					in_LightSpecular2 * \n"
											 "					pow(NdotHV, in_MaterialShine);\n"
											 "		}\n"
											 "	out_Color = color;\n"
	                                         "}\n";



	{
		const char *codeblocks[2];

#ifndef __ANDROID__
		glEnable(GL_FRAGMENT_PROGRAM_ARB);
		glEnable(GL_VERTEX_PROGRAM_ARB);
#endif
		l.shader.simple_shader.shader = glCreateProgram();

		//Obtain a valid handle to a vertex shader object.
		l.shader.simple_shader.vert_shader = glCreateShader(GL_VERTEX_SHADER);

		codeblocks[0] = common_vertex_transform_source;
		codeblocks[1] = simple_color_vertex_source;
		//Now, compile the shader source. 
		//Note that glShaderSource takes an array of chars. This is so that one can load multiple vertex shader files at once.
		//This is similar in function to linking multiple C++ files together. Note also that there can only be one "void main" definition
		//In all of the linked source files that are compiling with this funciton.
		glShaderSource(
			l.shader.simple_shader.vert_shader, //The handle to our shader
			2, //The number of files.
			codeblocks, //An array of const char * data, which represents the source code of theshaders
			NULL); //An array of string leng7ths. For have null terminated strings, pass NULL.
	 
		//Attempt to compile the shader.
		glCompileShader(l.shader.simple_shader.vert_shader);
		{
			//Error checking.
#ifdef __ANDROID__
			glGetObjectParameterivARB(l.shader.simple_shader.vert_shader, GL_COMPILE_STATUS, &result);
#else
			glGetObjectParameterivARB(l.shader.simple_shader.vert_shader, GL_OBJECT_COMPILE_STATUS_ARB, &result);
#endif
			if (!result)
			{
				GLint length;
				GLsizei final;
				char *buffer;
				//We failed to compile.
				lprintf("Vertex shader 'program A' failed compilation.\n");
				//Attempt to get the length of our error log.
				glGetObjectParameterivARB(l.shader.simple_shader.vert_shader, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
				buffer = NewArray( char, length );
				//Create a buffer.
					
				//Used to get the final length of the log.
				glGetInfoLogARB(l.shader.simple_shader.vert_shader, length, &final, buffer);
				//Convert our buffer into a string.
				lprintf( "message: %s", buffer );


				if (final > length)
				{
					//The buffer does not contain all the shader log information.
					printf("Shader Log contained more information!\n");
				}
		
			}
		}

		l.shader.simple_shader.frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
		codeblocks[0] = simple_color_pixel_source;
		glShaderSource(
			l.shader.simple_shader.frag_shader, //The handle to our shader
			1, //The number of files.
			codeblocks, //An array of const char * data, which represents the source code of theshaders
			NULL); //An array of string lengths. For have null terminated strings, pass NULL.
	 
		//Attempt to compile the shader.
		glCompileShader(l.shader.simple_shader.frag_shader);

		{
			//Error checking.
			glGetObjectParameterivARB(l.shader.simple_shader.frag_shader, GL_OBJECT_COMPILE_STATUS_ARB, &result);
			if (!result)
			{
				GLint length;
				GLsizei final;
				char *buffer;
				//We failed to compile.
				lprintf("Vertex shader 'program B' failed compilation.\n");
				//Attempt to get the length of our error log.
				glGetObjectParameterivARB(l.shader.simple_shader.frag_shader, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
				buffer = NewArray( char, length );
				//Create a buffer.
					
				//Used to get the final length of the log.
				glGetInfoLogARB(l.shader.simple_shader.frag_shader, length, &final, buffer);
				//Convert our buffer into a string.
				lprintf( "message: %s", buffer );


				if (final > length)
				{
					//The buffer does not contain all the shader log information.
					printf("Shader Log contained more information!\n");
				}
		
			}
		}

		glAttachObjectARB(l.shader.simple_shader.shader, l.shader.simple_shader.vert_shader );
		glAttachObjectARB(l.shader.simple_shader.shader, l.shader.simple_shader.frag_shader );
		
		glBindAttribLocation(l.shader.simple_shader.shader, 0, "in_Position");
		glBindAttribLocation(l.shader.simple_shader.shader, 1, "in_Normal");
		glBindAttribLocation(l.shader.simple_shader.shader, 2, "in_Color");

		glLinkProgram(l.shader.simple_shader.shader);
		glUseProgram(l.shader.simple_shader.shader);

		l.shader.simple_shader.eye_point
			=  glGetUniformLocation(l.shader.simple_shader.shader, "in_eye_point" );

		l.shader.simple_shader.global_ambient 
			=  glGetUniformLocation(l.shader.simple_shader.shader, "in_GlobalAmbient");

		l.shader.simple_shader.material.shine
			=  glGetUniformLocation(l.shader.simple_shader.shader, "in_MaterialShine");
		l.shader.simple_shader.material.ambient
			=  glGetUniformLocation(l.shader.simple_shader.shader, "in_MaterialAmbient");
		l.shader.simple_shader.material.diffuse
			=  glGetUniformLocation(l.shader.simple_shader.shader, "in_MaterialDiffuse");
		l.shader.simple_shader.material.specular
			=  glGetUniformLocation(l.shader.simple_shader.shader, "in_MaterialSpecular");

		l.shader.simple_shader.light[0].position
			= glGetUniformLocation(l.shader.simple_shader.shader, "in_LightPosition1");
		l.shader.simple_shader.light[0].ambient
			= glGetUniformLocation(l.shader.simple_shader.shader, "in_LightAmbient1");
		l.shader.simple_shader.light[0].diffuse
			= glGetUniformLocation(l.shader.simple_shader.shader, "in_LightDiffuse1");
		l.shader.simple_shader.light[0].specular
			= glGetUniformLocation(l.shader.simple_shader.shader, "in_LightSpecular1");

		l.shader.simple_shader.light[1].position
			= glGetUniformLocation(l.shader.simple_shader.shader, "in_LightPosition2");
		l.shader.simple_shader.light[1].ambient
			= glGetUniformLocation(l.shader.simple_shader.shader, "in_LightAmbient2");
		l.shader.simple_shader.light[1].diffuse
			= glGetUniformLocation(l.shader.simple_shader.shader, "in_LightDiffuse2");
		l.shader.simple_shader.light[1].specular
			= glGetUniformLocation(l.shader.simple_shader.shader, "in_LightSpecular2");

		l.shader.simple_shader.projection
			= glGetUniformLocation(l.shader.simple_shader.shader, "Projection");
		l.shader.simple_shader.worldview
			= glGetUniformLocation(l.shader.simple_shader.shader, "worldView");
		l.shader.simple_shader.modelview
			= glGetUniformLocation(l.shader.simple_shader.shader, "modelView");

		// projection should be constant?
		glUniformMatrix4fv( l.shader.simple_shader.projection, 1, GL_FALSE, l.projection );
		glUniform4f( l.shader.simple_shader.global_ambient, 0.5, 0.5, 0.5, 1.0 );
	}


	{
		const char *codeblocks[3];
		GLhandleARB fragmentHandle;

#ifndef __ANDROID__
		glEnable(GL_FRAGMENT_PROGRAM_ARB);
#endif
		l.shader.normal_shader.shader = glCreateProgramObjectARB();

		//Obtain a valid handle to a vertex shader object.
		l.shader.normal_shader.vert_shader = glCreateShaderObjectARB(GL_VERTEX_SHADER);

		codeblocks[0] = common_vertex_transform_source;
		codeblocks[1] = layer_texture_source;
		codeblocks[2] = bump_texture_color_vertex_source;
		//Now, compile the shader source. 
		//Note that glShaderSource takes an array of chars. This is so that one can load multiple vertex shader files at once.
		//This is similar in function to linking multiple C++ files together. Note also that there can only be one "void main" definition
		//In all of the linked source files that are compiling with this funciton.
		glShaderSource(
			l.shader.normal_shader.vert_shader, //The handle to our shader
			3, //The number of files.
			codeblocks, //An array of const char * data, which represents the source code of theshaders
			NULL); //An array of string leng7ths. For have null terminated strings, pass NULL.
	 
		//Attempt to compile the shader.
		glCompileShader(l.shader.normal_shader.vert_shader);
		{
			//Error checking.
			glGetObjectParameterivARB(l.shader.normal_shader.vert_shader, GL_OBJECT_COMPILE_STATUS_ARB, &result);
			if (!result)
			{
				GLint length;
				GLsizei final;
				char *buffer;
				//We failed to compile.
				lprintf("Vertex shader 'program A' failed compilation.\n");
				//Attempt to get the length of our error log.
				glGetObjectParameterivARB(l.shader.normal_shader.vert_shader, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
				buffer = NewArray( char, length );
				//Create a buffer.
					
				//Used to get the final length of the log.
				glGetInfoLogARB(l.shader.normal_shader.vert_shader, length, &final, buffer);
				//Convert our buffer into a string.
				lprintf( "message: %s", buffer );


				if (final > length)
				{
					//The buffer does not contain all the shader log information.
					printf("Shader Log contained more information!\n");
				}
		
			}
		}

		fragmentHandle = glCreateShaderObjectARB(GL_FRAGMENT_SHADER);
		codeblocks[0] = bump_texture_color_pixel_source;
		glShaderSource(
			fragmentHandle, //The handle to our shader
			1, //The number of files.
			codeblocks, //An array of const char * data, which represents the source code of theshaders
			NULL); //An array of string lengths. For have null terminated strings, pass NULL.
	 
		//Attempt to compile the shader.
		glCompileShader(fragmentHandle);

		{
			//Error checking.
			glGetObjectParameterivARB(fragmentHandle, GL_OBJECT_COMPILE_STATUS_ARB, &result);
			if (!result)
			{
				GLint length;
				GLsizei final;
				char *buffer;
				//We failed to compile.
				lprintf("Vertex shader 'program B' failed compilation.\n");
				//Attempt to get the length of our error log.
				glGetObjectParameterivARB(fragmentHandle, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
				buffer = NewArray( char, length );
				//Create a buffer.
					
				//Used to get the final length of the log.
				glGetInfoLogARB(fragmentHandle, length, &final, buffer);
				//Convert our buffer into a string.
				lprintf( "message: %s", buffer );


				if (final > length)
				{
					//The buffer does not contain all the shader log information.
					printf("Shader Log contained more information!\n");
				}
		
			}
		}

		glAttachObjectARB(l.shader.normal_shader.shader, l.shader.normal_shader.vert_shader );
		glAttachObjectARB(l.shader.normal_shader.shader, fragmentHandle );

		glBindAttribLocation(l.shader.normal_shader.shader, 0, "in_Position");
		glBindAttribLocation(l.shader.normal_shader.shader, 1, "in_Normal");
		glBindAttribLocation(l.shader.normal_shader.shader, 2, "in_Color");
		glBindAttribLocation(l.shader.normal_shader.shader, 3, "in_Tangent");
		glBindAttribLocation(l.shader.normal_shader.shader, 5, "in_Texture");

		glLinkProgram(l.shader.normal_shader.shader);

		glUseProgram( l.shader.normal_shader.shader);
		result = glGetError();

		l.shader.normal_shader.eye_point
			=  glGetUniformLocation(l.shader.normal_shader.shader, "in_eye_point" );

		l.shader.normal_shader.global_ambient 
			=  glGetUniformLocation(l.shader.normal_shader.shader, "in_GlobalAmbient");

		l.shader.normal_shader.material.shine
			=  glGetUniformLocation(l.shader.normal_shader.shader, "in_MaterialShine");
		l.shader.normal_shader.material.ambient
			=  glGetUniformLocation(l.shader.normal_shader.shader, "in_MaterialAmbient");
		l.shader.normal_shader.material.diffuse
			=  glGetUniformLocation(l.shader.normal_shader.shader, "in_MaterialDiffuse");
		l.shader.normal_shader.material.specular
			=  glGetUniformLocation(l.shader.normal_shader.shader, "in_MaterialSpecular");


		l.shader.normal_shader.light[0].position
			= glGetUniformLocation(l.shader.normal_shader.shader, "in_LightPosition1");
		l.shader.normal_shader.light[0].ambient
			= glGetUniformLocation(l.shader.normal_shader.shader, "in_LightAmbient1");
		l.shader.normal_shader.light[0].diffuse
			= glGetUniformLocation(l.shader.normal_shader.shader, "in_LightDiffuse1");
		l.shader.normal_shader.light[0].specular
			= glGetUniformLocation(l.shader.normal_shader.shader, "in_LightSpecular1");

		l.shader.normal_shader.light[1].position
			= glGetUniformLocation(l.shader.normal_shader.shader, "in_LightPosition2");
		l.shader.normal_shader.light[1].ambient
			= glGetUniformLocation(l.shader.normal_shader.shader, "in_LightAmbient2");
		l.shader.normal_shader.light[1].diffuse
			= glGetUniformLocation(l.shader.normal_shader.shader, "in_LightDiffuse2");
		l.shader.normal_shader.light[1].specular
			= glGetUniformLocation(l.shader.normal_shader.shader, "in_LightSpecular2");

		l.shader.normal_shader.projection
			= glGetUniformLocation(l.shader.normal_shader.shader, "Projection");
		l.shader.normal_shader.worldview
			= glGetUniformLocation(l.shader.normal_shader.shader, "worldView");
		l.shader.normal_shader.modelview
			= glGetUniformLocation(l.shader.normal_shader.shader, "modelView");

		// elements is 4

		// really ambient texture(?)
		l.shader.normal_shader.diffuseTextureUniform 
			= glGetUniformLocation(l.shader.normal_shader.shader,"colorMap");
		l.shader.normal_shader.normalTextureUniform 
			= glGetUniformLocation(l.shader.normal_shader.shader,"normalMap");

		//l.shader.normal_shader.shadowMapUniform = glGetUniformLocation(l.shader.normal_shader.shader,"ShadowMap");
		l.shader.normal_shader.invRadiusUniform = glGetUniformLocation(l.shader.normal_shader.shader,"invRadius");
		result = glGetError();

		l.shader.normal_shader.shadowMapBackUniform = glGetUniformLocation(l.shader.normal_shader.shader,"BackShadowMap");

		l.shader.normal_shader.specularTextureUniform = glGetUniformLocation(l.shader.normal_shader.shader,"specularTexture");

		// projection should be constant?
		glUniformMatrix4fv( l.shader.normal_shader.projection, 1, GL_FALSE, l.projection );
		glUniform4f( l.shader.normal_shader.global_ambient, 0.5, 0.5, 0.5, 1.0 );
	}
         CheckErr();

	glUseProgram( 0 );
         CheckErr();
#ifndef __ANDROID__
	glDisable(GL_FRAGMENT_PROGRAM_ARB);
	glDisable(GL_VERTEX_PROGRAM_ARB);
#endif
}

void SetMaterial( void )
{
   lprintf( "Set materiall..." );
#ifndef __ANDROID__
	glEnable(GL_FRAGMENT_PROGRAM_ARB);
	glEnable(GL_VERTEX_PROGRAM_ARB);
#endif
	{
      lprintf( "Use program" );
		glUseProgram( l.shader.simple_shader.shader );

		float spec[4];
		float diff[4];
		spec[0] = 0.5;//l.values[MAT_SPECULAR0] / 256.0f;
		spec[1] = 0.5;//l.values[MAT_SPECULAR1] / 256.0f;
		spec[2] = 0.5;//l.values[MAT_SPECULAR2] / 256.0f;
		spec[3] = 1.0f;
		glUniform4fv( l.shader.simple_shader.material.specular, 1, spec );


		//glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec );
		//glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 64/*l.values[MAT_SHININESS]/2*/ ); // 0-128
		glUniform1f( l.shader.simple_shader.material.shine, l.values[MAT_SHININESS]/2 );

		diff[0] = 0.5;
		diff[1] = 0.5;
		diff[2] = 0.45;
		diff[3] = 1.0f;
		glUniform4fv( l.shader.simple_shader.material.diffuse, 1, diff );
		//glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diff );
	}
	{
		glUseProgram( l.shader.normal_shader.shader );

		float spec[4];
		float diff[4];
		spec[0] = 0.5;//l.values[MAT_SPECULAR0] / 256.0f;
		spec[1] = 0.5;//l.values[MAT_SPECULAR1] / 256.0f;
		spec[2] = 0.5;//l.values[MAT_SPECULAR2] / 256.0f;
		spec[3] = 1.0f;
		glUniform4fv( l.shader.normal_shader.material.specular, 1, spec );


		//glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec );
		//glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 64/*l.values[MAT_SHININESS]/2*/ ); // 0-128
		glUniform1f( l.shader.normal_shader.material.shine, l.values[MAT_SHININESS]/2 );

		diff[0] = 0.5;
		diff[1] = 0.5;
		diff[2] = 0.45;
		diff[3] = 1.0f;
		glUniform4fv( l.shader.normal_shader.material.diffuse, 1, diff );
		//glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diff );

	}
      lprintf( "Disable program" );
	glUseProgram( 0 );
#ifndef __ANDROID__
	glDisable(GL_FRAGMENT_PROGRAM_ARB);
	glDisable(GL_VERTEX_PROGRAM_ARB);
#endif
}

void SetLights( void )
{
#ifndef __ANDROID__
		glEnable(GL_FRAGMENT_PROGRAM_ARB);
		glEnable(GL_VERTEX_PROGRAM_ARB);
#endif
      lprintf( "Use program" );
		glUseProgram( l.shader.simple_shader.shader );
			{
				GLfloat lightpos[] = {-1000, 1000, 0., 0.};
				GLfloat lightdir[] = {-.5, -1., -10., 0.};
				GLfloat lightamb[] = {l.values[LIT_AMBIENT0]/256.0f, l.values[LIT_AMBIENT1]/256.0f, l.values[LIT_AMBIENT2]/256.0f, 1.0};
				//GLfloat lightamb[] = {0.1, 0.1, 0.1, 1.0};
				GLfloat lightdif[] = {l.values[LIT_DIFFUSE0]/256.0f, l.values[LIT_DIFFUSE1]/256.0f, l.values[LIT_DIFFUSE2]/256.0f, 1.0};
				//GLfloat lightdif[] = {0.1, 0.1, 0.1, 0.1};
				GLfloat lightspec[] = {l.values[LIT_SPECULAR0]/256.0f, l.values[LIT_SPECULAR1]/256.0f, l.values[LIT_SPECULAR2]/256.0f, 1.0};
				//GLfloat lightspec[] = {0.1, 0.1, 0.1, 0.1};
				glUniform3fv( l.shader.simple_shader.light[0].position, 1, lightpos );
				glUniform4fv( l.shader.simple_shader.light[0].ambient, 1, lightamb );
				glUniform4fv( l.shader.simple_shader.light[0].specular, 1, lightspec );
				glUniform4fv( l.shader.simple_shader.light[0].diffuse, 1, lightdif );
				//glUniform3fv( l.shader.simple_shader.light[0].direction, 1, lightdir );

			}

			{
				GLfloat lightpos[] = {1000, -1000, -10., 0.};
				GLfloat lightdir[] = {-.5, -1., -10., 0.};
				GLfloat lightamb[] = {1.0, 1.0, 1.0, 1.0};
				//GLfloat lightamb[] = {0.1, 0.1, 0.1, 1.0};
				GLfloat lightdif[] = {1.0, 1.0, 1.0, 1.0};
				//GLfloat lightdif[] = {0.1, 0.1, 0.1, 0.1};
				GLfloat lightspec[] = {1.0, 1.0, 1.0, 1.0};
				//GLfloat lightspec[] = {0.1, 0.1, 0.1, 0.1};

				glUniform3fv( l.shader.simple_shader.light[1].position, 1, lightpos );
				glUniform4fv( l.shader.simple_shader.light[1].ambient, 1, lightamb );
				glUniform4fv( l.shader.simple_shader.light[1].specular, 1, lightspec );
				glUniform4fv( l.shader.simple_shader.light[1].diffuse, 1, lightdif );
				//glUniform3fv( l.shader.simple_shader.light[1].direction, 1, lightdir );
			}

		glUseProgram( l.shader.normal_shader.shader );
			{
				GLfloat lightpos[] = {-1000, 1000, 0., 0.};
				GLfloat lightdir[] = {-.5, -1., -10., 0.};
				GLfloat lightamb[] = {l.values[LIT_AMBIENT0]/256.0f, l.values[LIT_AMBIENT1]/256.0f, l.values[LIT_AMBIENT2]/256.0f, 1.0};
				//GLfloat lightamb[] = {0.1, 0.1, 0.1, 1.0};
				GLfloat lightdif[] = {l.values[LIT_DIFFUSE0]/256.0f, l.values[LIT_DIFFUSE1]/256.0f, l.values[LIT_DIFFUSE2]/256.0f, 1.0};
				//GLfloat lightdif[] = {0.1, 0.1, 0.1, 0.1};
				GLfloat lightspec[] = {l.values[LIT_SPECULAR0]/256.0f, l.values[LIT_SPECULAR1]/256.0f, l.values[LIT_SPECULAR2]/256.0f, 1.0};
				//GLfloat lightspec[] = {0.1, 0.1, 0.1, 0.1};
				glUniform3fv( l.shader.normal_shader.light[0].position, 1, lightpos );
				glUniform4fv( l.shader.normal_shader.light[0].ambient, 1, lightamb );
				glUniform4fv( l.shader.normal_shader.light[0].specular, 1, lightspec );
				glUniform4fv( l.shader.normal_shader.light[0].diffuse, 1, lightdif );
				//glUniform3fv( l.shader.normal_shader.light[0].direction, 1, lightdir );

			}

			{
				GLfloat lightpos[] = {1000, -1000, -10., 0.};
				GLfloat lightdir[] = {-.5, -1., -10., 0.};
				GLfloat lightamb[] = {1.0, 1.0, 1.0, 1.0};
				//GLfloat lightamb[] = {0.1, 0.1, 0.1, 1.0};
				GLfloat lightdif[] = {1.0, 1.0, 1.0, 1.0};
				//GLfloat lightdif[] = {0.1, 0.1, 0.1, 0.1};
				GLfloat lightspec[] = {1.0, 1.0, 1.0, 1.0};
				//GLfloat lightspec[] = {0.1, 0.1, 0.1, 0.1};

				glUniform3fv( l.shader.normal_shader.light[1].position, 1, lightpos );
				glUniform4fv( l.shader.normal_shader.light[1].ambient, 1, lightamb );
				glUniform4fv( l.shader.normal_shader.light[1].specular, 1, lightspec );
				glUniform4fv( l.shader.normal_shader.light[1].diffuse, 1, lightdif );
				//glUniform3fv( l.shader.normal_shader.light[1].direction, 1, lightdir );
			}

      lprintf( "disable" );
	glUseProgram( 0 );
#ifndef __ANDROID__
	glDisable(GL_FRAGMENT_PROGRAM_ARB);
	glDisable(GL_VERTEX_PROGRAM_ARB);
#endif
}

// useful for purely static objects.
struct SACK_3D_Surface *CreateBumpTextureFragment( int verts
									, PCVECTOR *shape_array
									, PCVECTOR *normal_array
									, PCVECTOR *tangent_array
									, PCVECTOR *colors
									, PCVECTOR *texture_coord_array  // ignored
									)
{
	if( glGenBuffersARB )
	{
		GLint result;
		struct SACK_3D_Surface *surface = New( struct SACK_3D_Surface );
		surface->verts = verts;	
		surface->vertices = shape_array;
		surface->elements = NewArray( GLuint, verts );
		surface->pdl_VBOTexture = CreateDataList( sizeof( GLuint ) );

		glGenVertexArrays(1, &surface->VAOobject); // Create our Vertex Array Object  

		glBindVertexArray(surface->VAOobject); // Bind our Vertex Array Object so we can use it  
  
  
		{
			glGenBuffers( 1, &surface->VBOvertices );
			glBindBuffer(GL_ARRAY_BUFFER,surface->VBOvertices);			
			glBufferData(GL_ARRAY_BUFFER, verts * 3 * sizeof(RCOORD), shape_array,GL_STATIC_DRAW);
			glVertexAttribPointer((GLuint)0, 3, GL_DOUBLE, GL_FALSE, 0, 0); 
			glVertexPointer( 3,	GL_DOUBLE, 0, 0 );
			glEnableVertexAttribArray( 0 );
			

			glGenBuffers( 1, &surface->VBOnormal );
			glBindBuffer(GL_ARRAY_BUFFER,surface->VBOnormal);			
			glBufferData(GL_ARRAY_BUFFER, verts * 3 * sizeof(RCOORD), normal_array, GL_STATIC_DRAW);
			glVertexAttribPointer((GLuint)1, 3, GL_DOUBLE, GL_TRUE, 0, 0); 
			glNormalPointer( GL_DOUBLE, 0, 0 ); // invalid_enum
			glEnableVertexAttribArray( 1 );

			if( 0 )
			{
				VECTOR *c;
				int n;
				c = NewArray( VECTOR, verts );
				for( n = 0; n < verts; n++ )
				{
					c[n][0] = 0.8;
					c[n][1] = 0.3;
					c[n][2] = 0.5;
					c[n][3] = 1.0;
				}
				glGenBuffers( 1, &surface->VBOcolor );
				glBindBuffer( GL_ARRAY_BUFFER, surface->VBOcolor );
				glBufferData( GL_ARRAY_BUFFER, verts * 4 * sizeof( RCOORD ), c, GL_STATIC_DRAW );
				glVertexAttribPointer((GLuint)2, 3, GL_DOUBLE, GL_TRUE, 0, 0); 
				glColorPointer( 4, GL_DOUBLE, 0, 0 );
				glEnableVertexAttribArray( 2 );
			}

			glGenBuffers(1,&surface->VBOTangent);			
			glBindBuffer(GL_ARRAY_BUFFER,surface->VBOTangent);			
			glBufferData(GL_ARRAY_BUFFER,verts * 3 * sizeof(RCOORD),tangent_array,GL_STATIC_DRAW);
			glVertexAttribPointer((GLuint)3, 3, GL_DOUBLE, GL_FALSE, 0, 0); // Set up our vertex attributes pointer  
			glEnableVertexAttribArray( 3 );

			glGenBuffers(1,&surface->VBOelements);			
			glBindBuffer(GL_ARRAY_BUFFER,surface->VBOelements);			
			{				
				int n;				
				for( n= 0; n < verts; n++ )					
					surface->elements[n] = n;			
			}			
			glBufferData(GL_ARRAY_BUFFER,verts * sizeof(unsigned int),surface->elements,GL_STATIC_DRAW);		
			glVertexAttribPointer((GLuint)4, 1, GL_INT, GL_FALSE, 0, 0); // Set up our vertex attributes pointer  
			glEnableVertexAttribArray( 4 );

		}

		glBindVertexArray(0); // Bind our Vertex Array Object so we can use it  

		return surface;
	}
	return NULL;
}


INDEX AddBumpTextureFragmentTexture( struct SACK_3D_Surface *surface
									, INDEX texture_id
													, PCVECTOR *texture_coord_array
													)
{
	GLuint VBOTexture;
	if( glGenBuffers )
	{
		glGenBuffers( 1, &VBOTexture );
		glBindBuffer(GL_ARRAY_BUFFER,VBOTexture);
		glBufferData(GL_ARRAY_BUFFER,surface->verts * 3 * sizeof(RCOORD),texture_coord_array,GL_STATIC_DRAW);
	}
	SetDataItem( &surface->pdl_VBOTexture, texture_id, &VBOTexture );
	return texture_id;
}

void RenderBumpTextureFragment( Image texture
								, Image bump
								, btScalar *m
								, INDEX texture_id
								, float *background
									, struct SACK_3D_Surface *surface
				)
{
	int result;
	LOGICAL SetShader = FALSE;

	glBindVertexArray(surface->VAOobject); // Bind our Vertex Array Object so we can use it  	

	if( texture )
	{
		int texture_texture;
		int bump_texture;
		if( texture )
			texture_texture = ReloadTexture( texture, 0 );
		else
			texture_texture = 0;
		if( bump )
			bump_texture = ReloadTexture( bump, 0 );
		else
			bump_texture = 0;

		if( 0)
		{
			SetShader = TRUE;
			SetLights();
#ifndef __ANDROID__
		 	glEnable(GL_FRAGMENT_PROGRAM_ARB);
		 	glEnable(GL_VERTEX_PROGRAM_ARB);
#endif
      lprintf( "Use program" );
			glUseProgram( l.shader.normal_shader.shader);

			glVertexAttrib4fv((GLuint)2, background); // set constant color attribute
			#ifdef BT_USE_DOUBLE_PRECISION
			glUniformMatrix4dv
#else
			glUniformMatrix4fv
#endif
				( l.shader.normal_shader.modelview, 1, GL_FALSE, m );
			glUniformMatrix4fv( l.shader.normal_shader.worldview, 1, GL_FALSE, l.worldview );
   			// set with the index of the texture loaded below...

			glActiveTexture( GL_TEXTURE1 );
			glBindTexture(GL_TEXTURE_2D, bump_texture );
			glActiveTexture( GL_TEXTURE0 );
			glBindTexture(GL_TEXTURE_2D, texture_texture );

			glUniform1i(l.shader.normal_shader.diffuseTextureUniform, 0 );
			glUniform1i(l.shader.normal_shader.normalTextureUniform, 1 );

			glUniform1f( l.shader.normal_shader.invRadiusUniform, 0.0001f );

			glUniform4fv( l.shader.normal_shader.material.ambient, 1, background );


		}
		// VBO, created and populated once, texture coordinate never change
		glBindBuffer(GL_ARRAY_BUFFER,*(GLuint*)GetDataItem( &surface->pdl_VBOTexture, texture_id ) );
		// swap into vao this vbo ?
		glVertexAttribPointer((GLuint)5, 3, GL_DOUBLE, GL_FALSE, 0, 0); 
		glEnableVertexAttribArray( 5 );			
	}
	else if( 1 )
	{
		SetShader = TRUE;
#ifndef __ANDROID__
		glEnable(GL_VERTEX_PROGRAM_ARB);
		glEnable(GL_FRAGMENT_PROGRAM_ARB);
#endif
      lprintf( "Use program" );
		glUseProgram( l.shader.simple_shader.shader );
		glUniform4fv( l.shader.simple_shader.material.ambient, 1, background );
		//glUniform4fv( l.shader.simple_shader.material.diffuse, 1, background );
		glVertexAttrib4fv((GLuint)2, background); // set constant color attribute
		glDisableVertexAttribArray( 5 );

	}

	
	glDrawArrays( GL_TRIANGLE_STRIP, 0, surface->verts );


	if( SetShader )
	{
      lprintf( "disable program" );
		glUseProgram( 0 );
#ifndef __ANDROID__
		glDisable(GL_FRAGMENT_PROGRAM_ARB);
		glDisable(GL_VERTEX_PROGRAM_ARB);
#endif
	}

	glBindVertexArray(0);
}

