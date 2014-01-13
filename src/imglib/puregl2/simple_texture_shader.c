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
     "attribute vec4 vPosition;\n" 
	 "attribute vec2 in_texCoord;\n" 
	 "uniform mat4 modelView;\n" 
	 "uniform mat4 worldView;\n" 
	 "uniform mat4 Projection;\n" 
	 " varying vec2 out_texCoord;\n" 
    "void main() {\n"
    "  gl_Position = Projection * worldView * modelView * vPosition;\n" 
	 "out_texCoord = in_texCoord;\n" 
    "}\n"; 

static const char *gles_simple_p_shader =
    // "precision mediump float;\n" 
	 " varying vec2 out_texCoord;\n" 
	 " uniform sampler2D tex;\n" 
	 "void main() {\n"
	 "   gl_FragColor = texture2D( tex, out_texCoord );\n"
     "}\n" ;


//const char *gles_
static const char *gles_simple_v_shader_shaded_texture =
     "attribute vec4 vPosition;\n" 
	  "attribute vec2 in_texCoord;\n" 
	  "uniform mat4 modelView;\n" 
	  "uniform mat4 worldView;\n" 
	  "uniform mat4 Projection;\n" 
	  "varying vec2 out_texCoord;\n" 
	  "\n" 
    "void main() {\n"
    "  gl_Position = Projection * worldView * modelView * vPosition;\n" 
	  " out_texCoord = in_texCoord;\n" 
    "}"; 

static const char *gles_simple_p_shader_shaded_texture =
    // "precision mediump float;\n" 
	 "uniform vec4 in_Color;\n" 
	 " varying vec2 out_texCoord;\n" 
	 " uniform sampler2D tex;\n" 
	 "void main() {\n"
	 "   gl_FragColor = in_Color * texture2D( tex, out_texCoord );\n"
     "}\n" ;


struct private_shader_data
{
	int texture_attrib;
	int texture;
};

static void CPROC SimpleTextureEnable( PImageShaderTracker tracker, PTRSZVAL psv_userdata, va_list args )
{
	float *verts = va_arg( args, float *);
	int texture = va_arg( args, int);
	float *texture_verts = va_arg( args, float *);
	struct private_shader_data *data	= (struct private_shader_data *)psv_userdata;

	//glUniform4fv( tracker->color_attrib, 1, GL_FALSE, color );
	glEnableVertexAttribArray(0);

	glVertexAttribPointer( 0, 3, GL_FLOAT, FALSE, 0, verts );
	CheckErr();
	glEnableVertexAttribArray(data->texture_attrib);	CheckErr();
	glVertexAttribPointer( data->texture_attrib, 2, GL_FLOAT, FALSE, 0, texture_verts );            
	CheckErr();
	glActiveTexture(GL_TEXTURE0 + 0);
	CheckErr();
	glTexParameteri(GL_TEXTURE_2D+0, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D+0, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glBindTexture(GL_TEXTURE_2D+0, texture);
	CheckErr();
	glUniform1i( data->texture, 0 );
	CheckErr();


}

void InitSimpleTextureShader( PImageShaderTracker tracker )
{
	const char *v_codeblocks[2];
	const char *p_codeblocks[2];
	struct private_shader_data *data = New( struct private_shader_data );
	struct image_shader_attribute_order attribs[] = { { 0, "vPosition" }, { 1, "in_TexCoord" } };

	SetShaderEnable( tracker, SimpleTextureEnable, (PTRSZVAL)data );

	v_codeblocks[0] = gles_simple_v_shader;
	v_codeblocks[1] = NULL;
	p_codeblocks[0] = gles_simple_p_shader;
	p_codeblocks[1] = NULL;
	if( CompileShaderEx( tracker, v_codeblocks, 1, p_codeblocks, 1, attribs, 2 ) )
	{
		data->texture = glGetUniformLocation(tracker->glProgramId, "tex");
		data->texture_attrib =  glGetAttribLocation(tracker->glProgramId, "in_texCoord" );
	}

}

static void CPROC SimpleTextureEnable2( PImageShaderTracker tracker, PTRSZVAL psv, va_list args )
{
	float *verts = va_arg( args, float *);
	int texture = va_arg( args, int );
	float *texture_verts = va_arg( args, float *);
	float *color = va_arg( args, float *);

	struct private_shader_data *data = (struct private_shader_data *)psv;

	glEnableVertexAttribArray(0);	CheckErr();
	glVertexAttribPointer( 0, 3, GL_FLOAT, FALSE, 0, verts );  
	CheckErr();
	
	glEnableVertexAttribArray(data->texture_attrib);	CheckErr();
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
	const char *v_codeblocks[2];
	const char *p_codeblocks[2];
	struct private_shader_data *data = New( struct private_shader_data );
	struct image_shader_attribute_order attribs[] = { { 0, "vPosition" }, { 1, "in_TexCoord" }, { 2, "in_Color" } };

	SetShaderEnable( tracker, SimpleTextureEnable2, (PTRSZVAL)data );

	v_codeblocks[0] = gles_simple_v_shader_shaded_texture;
	v_codeblocks[1] = NULL;
	p_codeblocks[0] = gles_simple_p_shader_shaded_texture;
	p_codeblocks[1] = NULL;

	if( CompileShaderEx( tracker, v_codeblocks, 1, p_codeblocks, 1, attribs, 3 ) )
	{
		tracker->color_attrib = glGetUniformLocation(tracker->glProgramId, "in_Color" );
		data->texture = glGetUniformLocation(tracker->glProgramId, "tex");
		data->texture_attrib =  glGetAttribLocation(tracker->glProgramId, "in_texCoord" );
	}
}

IMAGE_NAMESPACE_END
