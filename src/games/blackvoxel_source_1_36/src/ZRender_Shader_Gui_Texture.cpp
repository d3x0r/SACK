
#  include "ZRender_Interface.h"

#  include "ZRender_Shader_Simple.h"

	const char const *ZRender_Shader_Gui_Texture::vertex_shader =
	"precision mediump float;\n"
	"precision mediump int;\n"
	"attribute vec4 in_Position;\n" 
	 "attribute vec2 in_texCoord;\n" 
	 " uniform vec3 in_Color;\n" 
	//"attribute vec4 in_Color;\n"
	//"varying vec4 ex_Color;\n"
	 " varying vec2 out_texCoord;\n" 
	 " varying vec4 out_Color;\n" 
	"void main(void) {\n"
	"  gl_Position = in_Position;\n" 
	"  gl_Position.y = -gl_Position.y;\n"
	"  out_texCoord = in_texCoord;\n"
	"  out_Color.xyz = in_Color;\n"
	"  out_Color.w = 1.0;\n"
	"}"; 

	const char const *ZRender_Shader_Gui_Texture::pixel_shader =
	"precision mediump float;\n"
	"precision mediump int;\n"
	 " varying vec2 out_texCoord;\n" 
	 " uniform sampler2D tex;\n" 
	 "varying vec4 out_Color;" 
	"void main(void) {\n"
	 "   gl_FragColor = out_Color * texture2D( tex, out_texCoord );\n"
	"}" ;

void ZRender_Shader_Gui_Texture::Draw( int texture
									  , float *color
								 , float *pos
								 , float *uv
								 )
{

	ImageEnableShader( shader );
	CheckErr();
	glEnableVertexAttribArray(pos_attrib);
	CheckErr();
	glEnableVertexAttribArray(tex_coord_attrib);
	CheckErr();
	glVertexAttribPointer( pos_attrib, 3, GL_FLOAT, FALSE, 0, pos );
	CheckErr();
	glVertexAttribPointer( tex_coord_attrib, 2, GL_FLOAT, FALSE, 0, uv );
	CheckErr();
	glActiveTexture(GL_TEXTURE0 + 0);
	CheckErr();
	glBindTexture(GL_TEXTURE_2D + 0, texture);
	CheckErr();
	glUniform1i( texture_uniform, 0 );
	CheckErr();
	glUniform3fv( color_uniform, 1, color );
	CheckErr();

	glDrawArrays( GL_TRIANGLES, 0, 6 );
	CheckErr();
}


uintptr_t ZRender_Shader_Gui_Texture::SetupShader( void )
{
	// no instance for simple tracker
	//data.data.vert_pos = ImageCreateShaderBuffer( 3, 8, 16 );
	//data.data.vert_color = ImageCreateShaderBuffer( 4, 8, 16 );
	return (uintptr_t)&data;
}

static uintptr_t CPROC _SetupShader( uintptr_t psvInit )
{
	ZRender_Shader_Simple *_this = (ZRender_Shader_Simple*)psvInit;
	_this->SetupShader();
	return psvInit;
}

void ZRender_Shader_Gui_Texture::InitShader( PImageShaderTracker tracker )
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
		pos_attrib = glGetAttribLocation( glProgramId, "in_Position" );
		CheckErr();
		tex_coord_attrib = glGetAttribLocation( glProgramId, "in_texCoord" );
		CheckErr();
		color_uniform = glGetUniformLocation( glProgramId, "in_Color" );
		CheckErr();
		texture_uniform = glGetUniformLocation( glProgramId, "tex" );
		CheckErr();
		//ImageSetShaderOpInit( tracker, SimpleShader_OpInit );
		//ImageSetShaderOutput( tracker, SimpleShader_Output );
		//ImageSetShaderReset( tracker, SimpleShader_Reset );
		//ImageSetShaderAppendTristrip( tracker, SimpleShader_AppendTristrip );
	}
}

static void CPROC _InitShader( uintptr_t psvSetup, PImageShaderTracker tracker )
{
	ZRender_Shader_Gui_Texture *_this = (ZRender_Shader_Gui_Texture*)psvSetup;
	_this->InitShader( tracker );
}

ZRender_Shader_Gui_Texture::ZRender_Shader_Gui_Texture( ZRender_Interface *render )
{
	this->render = render;
	shader = ImageGetShaderInit( WIDE("BV GUI Shader")
		, _SetupShader
		, _InitShader, (uintptr_t)this );

}