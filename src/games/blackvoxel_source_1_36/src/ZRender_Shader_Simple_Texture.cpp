
#  include "ZRender_Interface.h"

#  include "ZRender_Shader_Simple_Texture.h"

const char *ZRender_Shader_Simple_Texture::vertex_shader =
	   "precision mediump float;\n"
		"precision mediump int;\n"
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

const char *ZRender_Shader_Simple_Texture::pixel_shader =
    // "precision mediump float;\n" 
   "precision mediump float;\n"
	"precision mediump int;\n"
	 " varying vec2 out_texCoord;\n" 
	 " uniform sampler2D tex;\n" 
	 "void main() {\n"
	 "   gl_FragColor = texture2D( tex, out_texCoord );\n"
     "}\n" ;

void ZRender_Shader_Simple_Texture::DrawItems( int texture, struct shader_buffer *vert, struct shader_buffer *tex_uv )
{
	if( !vert->used )
		return;
	CheckErr();
	ImageEnableShader( shader );
	CheckErr();
	glEnableVertexAttribArray(pos_attrib);
	CheckErr();
	glEnableVertexAttribArray(texture_attrib);
	CheckErr();
	glVertexAttribPointer( pos_attrib, 3, GL_FLOAT, FALSE, 0, vert->data );
	CheckErr();
	glVertexAttribPointer( texture_attrib, 2, GL_FLOAT, FALSE, 0, tex_uv->data );
	CheckErr();
	glActiveTexture(GL_TEXTURE0 + 0);
	CheckErr();
	glBindTexture(GL_TEXTURE_2D + 0, texture);
	CheckErr();
	glUniform1i( texture_uniform, 0 );
	CheckErr();
	glDrawArrays( GL_TRIANGLES, 0, vert->used / 3 );
	CheckErr();

}

void ZRender_Shader_Simple_Texture::DrawFilledRect( ZVector3f p[4], ZVector4f &c )
{
	if( !box_buffer )
		box_buffer = ImageCreateShaderBuffer( 3, 24, 8 );
	box_buffer->used = 0;
	ImageAppendShaderData( box_buffer, p[0] );
	ImageAppendShaderData( box_buffer, p[1] );
	ImageAppendShaderData( box_buffer, p[2] );
	ImageAppendShaderData( box_buffer, p[2] );
	ImageAppendShaderData( box_buffer, p[3] );
	ImageAppendShaderData( box_buffer, p[0] );

	CheckErr();
	ImageEnableShader( shader );
	CheckErr();
	glEnableVertexAttribArray(0);
	CheckErr();
	glVertexAttribPointer( 0, 3, GL_FLOAT, FALSE, 0, box_buffer->data );
	CheckErr();
	glUniform4fv( color_attrib, 1, &c.r );
	CheckErr();
	glDrawArrays( GL_TRIANGLES, 0, 6 );
	CheckErr();

}

void ZRender_Shader_Simple_Texture::DrawLine( ZVector3f *p1, ZVector3f *p2, ZVector4f *c )
{
	box_buffer->used = 0;
	ImageAppendShaderData( box_buffer,  *p1 );
	ImageAppendShaderData( box_buffer,  *p2 );
	CheckErr();
	ImageEnableShader( shader );
	CheckErr();
	glEnableVertexAttribArray(0);
	CheckErr();
	glVertexAttribPointer( 0, 3, GL_FLOAT, FALSE, 0, box_buffer->data );
	CheckErr();
	glUniform4fv( color_attrib, 1, &c->r );
	CheckErr();
	glDrawArrays( GL_LINES, 0, 2 );
	CheckErr();
}
	
void ZRender_Shader_Simple_Texture::DrawBox( ZVector3f *p1, ZVector3f *p2, ZVector3f *p3, ZVector3f *p4
	,ZVector3f *p5, ZVector3f *p6, ZVector3f *p7, ZVector3f *p8
	, ZVector4f *c )
{
	if( !box_buffer )
		box_buffer = ImageCreateShaderBuffer( 3, 24, 8 );
	box_buffer->used = 0;
	ImageAppendShaderData( box_buffer,  (float*)p1 );
	ImageAppendShaderData( box_buffer,  (float*)p2 );
	ImageAppendShaderData( box_buffer,  (float*)p2 );
	ImageAppendShaderData( box_buffer,  (float*)p3 );
	ImageAppendShaderData( box_buffer,  (float*)p3 );
	ImageAppendShaderData( box_buffer,  (float*)p4 );
	ImageAppendShaderData( box_buffer,  (float*)p4 );
	ImageAppendShaderData( box_buffer,  (float*)p1 );

	ImageAppendShaderData( box_buffer,  (float*)p5 );
	ImageAppendShaderData( box_buffer,  (float*)p6 );
	ImageAppendShaderData( box_buffer,  (float*)p6 );
	ImageAppendShaderData( box_buffer,  (float*)p7 );
	ImageAppendShaderData( box_buffer,  (float*)p7 );
	ImageAppendShaderData( box_buffer,  (float*)p8 );
	ImageAppendShaderData( box_buffer,  (float*)p8 );
	ImageAppendShaderData( box_buffer,  (float*)p5 );

	ImageAppendShaderData( box_buffer,  (float*)p1 );
	ImageAppendShaderData( box_buffer,  (float*)p5 );
	ImageAppendShaderData( box_buffer,  (float*)p2 );
	ImageAppendShaderData( box_buffer,  (float*)p6 );
	ImageAppendShaderData( box_buffer,  (float*)p3 );
	ImageAppendShaderData( box_buffer,  (float*)p7 );
	ImageAppendShaderData( box_buffer,  (float*)p4 );
	ImageAppendShaderData( box_buffer,  (float*)p8 );

	CheckErr();
	ImageEnableShader( shader );
	CheckErr();
	glEnableVertexAttribArray(0);
	CheckErr();
	glVertexAttribPointer( 0, 3, GL_FLOAT, FALSE, 0, box_buffer->data );
	CheckErr();
	glUniform4fv( color_attrib, 1, &c->r );
	CheckErr();
	glDrawArrays( GL_LINES, 0, 12 );
	CheckErr();
}


PTRSZVAL ZRender_Shader_Simple_Texture::SetupShader( void )
{
	// no instance for simple tracker
	//data.data.vert_pos = ImageCreateShaderBuffer( 3, 8, 16 );
	//data.data.vert_color = ImageCreateShaderBuffer( 4, 8, 16 );
	return (PTRSZVAL)&data;
}

static PTRSZVAL CPROC _SetupShader( PTRSZVAL psvInit )
{
	ZRender_Shader_Simple_Texture *_this = (ZRender_Shader_Simple_Texture*)psvInit;
	_this->SetupShader();
	return psvInit;
}

void ZRender_Shader_Simple_Texture::InitShader( PImageShaderTracker tracker )
{
	//struct simple_shader_data *data = (struct simple_shader_data *)psv;
	const char *v_codeblocks[2];
	const char *p_codeblocks[2];
	
	v_codeblocks[0] = vertex_shader;
	v_codeblocks[1] = NULL;
	p_codeblocks[0] = pixel_shader;
	p_codeblocks[1] = NULL;
	if( glProgramId = ImageCompileShader( tracker, v_codeblocks, 1, p_codeblocks, 1 ) )
	{
		color_attrib = glGetUniformLocation( glProgramId, "in_Color" );
		pos_attrib = glGetAttribLocation( glProgramId, "vPosition" );
		texture_attrib = glGetAttribLocation( glProgramId, "in_texCoord" );
		texture_uniform = glGetUniformLocation( glProgramId, "tex" );
		//ImageSetShaderOpInit( tracker, SimpleShader_OpInit );
		//ImageSetShaderOutput( tracker, SimpleShader_Output );
		//ImageSetShaderReset( tracker, SimpleShader_Reset );
		//ImageSetShaderAppendTristrip( tracker, SimpleShader_AppendTristrip );
	}
}

static void CPROC _InitShader( PTRSZVAL psvSetup, PImageShaderTracker tracker )
{
	ZRender_Shader_Simple_Texture *_this = (ZRender_Shader_Simple_Texture*)psvSetup;
	_this->InitShader( tracker );
}

ZRender_Shader_Simple_Texture::ZRender_Shader_Simple_Texture( ZRender_Interface *render )
{
	this->render = render;
	box_buffer = NULL;
	shader = ImageGetShaderInit( WIDE("BV Simple Texture Shader")
		, _SetupShader
		, _InitShader, (PTRSZVAL)this );

} 