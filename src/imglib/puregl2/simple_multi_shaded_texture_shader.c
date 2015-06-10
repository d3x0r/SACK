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
static const char *gles_simple_v_multi_shader =
   "precision mediump float;\n"
	"precision mediump int;\n"
	 "uniform mat4 modelView;\n" 
	 "uniform mat4 worldView;\n" 
	 "uniform mat4 Projection;\n" 
	 "attribute vec2 in_texCoord;\n" 
     "attribute vec4 vPosition;" 
	 " varying vec2 out_texCoord;\n" 
	// "in  vec4 in_Color;\n"
	// "out vec4 ex_Color;\n"
     "void main(void) {"
     "  gl_Position = Projection * worldView * modelView * vPosition;"
	 "out_texCoord = in_texCoord;\n" 
	// "  ex_Color = in_Color;" 
     "}"; 


static const char *gles_simple_p_multi_shader =
   "precision mediump float;\n"
	"precision mediump int;\n"
	     " varying vec2 out_texCoord;\n" 
        "uniform sampler2D tex;\n" 
        "uniform vec4 multishade_r;\n" 
        "uniform vec4 multishade_g;\n" 
        "uniform vec4 multishade_b;\n" 
        "\n" 
        "void main(void)\n"
        "{\n" 
        "    vec4 color = texture2D(tex, out_texCoord);\n"
        "	gl_FragColor = vec4( (color.b * multishade_b.r) + (color.g * multishade_g.r) + (color.r * multishade_r.r),\n"
        "		(color.b * multishade_b.g) + (color.g * multishade_g.g) + (color.r * multishade_r.g),\n"
        "		(color.b * multishade_b.b) + (color.g * multishade_g.b) + (color.r * multishade_r.b),\n"
        "		  color.r!=0.0?( color.a * multishade_r.a ):0.0\n"
        "                + color.g!=0.0?( color.a * multishade_g.a ):0.0\n"
        "                + color.b!=0.0?( color.a * multishade_b.a ):0.0\n"
        "                )\n"
        "		;\n" 
				 "}\n" ;

struct private_mst_shader_texture_data
{
	int texture;
	struct shader_buffer *vert_pos;
	struct shader_buffer *vert_color_r;
	struct shader_buffer *vert_color_g;
	struct shader_buffer *vert_color_b;
	struct shader_buffer *vert_texture_uv;
};

struct private_mst_shader_data
{
	int texture_attrib;
	int texture;
	int r_color_attrib;
	int g_color_attrib;
	int b_color_attrib;
	PLIST vert_data;
};


static void CPROC SimpleMultiShadedTextureOutput( PImageShaderTracker tracker, PTRSZVAL psv, PTRSZVAL psvKey, int from, int to )
{

	struct private_mst_shader_data *data = (struct private_mst_shader_data *)psv;
	struct private_mst_shader_texture_data *texture = (struct private_mst_shader_texture_data *)psvKey;
	
	glEnableVertexAttribArray(0);	CheckErr();
	glVertexAttribPointer( 0, 3, GL_FLOAT, FALSE, 0, texture->vert_pos->data );  
	CheckErr();
	
	glEnableVertexAttribArray(data->texture_attrib);	CheckErr();
	glVertexAttribPointer( data->texture_attrib, 2, GL_FLOAT, FALSE, 0, texture->vert_pos->data );  
	CheckErr();

	glUniform1i(data->texture, 0); //Texture unit 0 is for base images.
 
	//When rendering an objectwith this program.
	glActiveTexture(GL_TEXTURE0 + 0);
	CheckErr();
	glBindTexture(GL_TEXTURE_2D, texture->texture);
	CheckErr();
	//glBindSampler(0, linearFiltering);
	CheckErr();

	glUniform4fv( data->r_color_attrib, 1, texture->vert_color_r->data );
	CheckErr();
	glUniform4fv( data->g_color_attrib, 1, texture->vert_color_g->data );
	CheckErr();
	glUniform4fv( data->b_color_attrib, 1, texture->vert_color_b->data );
	CheckErr();

		glDrawArrays( GL_TRIANGLES, from, to - from );

}

static void CPROC SimpleMultiShadedTextureReset( PImageShaderTracker tracker, PTRSZVAL psv, PTRSZVAL psvKey )
{
	struct private_mst_shader_texture_data *texture = (struct private_mst_shader_texture_data *)psvKey;

	struct private_mst_shader_data *data = (struct private_mst_shader_data *)psv;

	texture->vert_pos->used = 0;
	texture->vert_texture_uv->used = 0;
	texture->vert_color_r->used = 0;
	texture->vert_color_g->used = 0;
	texture->vert_color_b->used = 0;
}

PTRSZVAL SetupSimpleMultiShadedTextureShader( PTRSZVAL psv )
{
	struct private_mst_shader_data *data = New(struct private_mst_shader_data );
	return (PTRSZVAL)data;
}


void InitSimpleMultiShadedTextureShader( PTRSZVAL psvInst, PImageShaderTracker tracker )
{
	GLint result;
	const char *v_codeblocks[2];
	const char *p_codeblocks[2];
	struct private_mst_shader_data *data = (struct private_mst_shader_data*)psvInst;
	struct image_shader_attribute_order attribs[] = { { 0, "vPosition" }, { 1, "in_TexCoord" } };

	//SetShaderEnable( tracker, SimpleMultiShadedTextureEnable, (PTRSZVAL)data );

	if( result = glGetError() )
	{
		lprintf( WIDE("unhandled error before shader") );
	}

	v_codeblocks[0] = gles_simple_v_multi_shader;
	v_codeblocks[1] = NULL;
	p_codeblocks[0] = gles_simple_p_multi_shader;
	p_codeblocks[1] = NULL;
	if( CompileShaderEx( tracker, v_codeblocks, 1, p_codeblocks, 1, attribs, 2 ) )
	{
		if( !data )
		{
			data = New( struct private_mst_shader_data );
		}
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
		//return (PTRSZVAL)data;
	}
}

IMAGE_NAMESPACE_END
