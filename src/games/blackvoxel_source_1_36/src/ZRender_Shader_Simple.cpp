
#  include "ZRender_Interface.h"

#  include "ZRender_Shader_Simple.h"

	const char *ZRender_Shader_Simple::vertex_shader =
	"precision mediump float;\n"
	"precision mediump int;\n"
	"uniform mat4 modelView;\n" 
	"uniform mat4 worldView;\n" 
	"uniform mat4 Projection;\n" 
	"attribute vec4 vPosition;" 
	//"attribute vec4 in_Color;\n"
	//"varying vec4 ex_Color;\n"
	"void main(void) {"
	"  gl_Position = Projection * worldView * modelView * vPosition;" 
	//"  ex_Color = in_Color;"
	"}"; 

	const char *ZRender_Shader_Simple::pixel_shader =
	"precision mediump float;\n"
	"precision mediump int;\n"
	"uniform  vec4 in_Color;\n"
	// "varying vec4 out_Color;" 
	"void main(void) {"
	"  gl_FragColor = in_Color;"
	"}" ;

void ZRender_Shader_Simple::DrawFilledRect( ZVector3f p[4], ZVector4f &c )
{
	if( !box_buffer )
		box_buffer = ImageCreateShaderBuffer( 3, 24, 8 );
	box_buffer->used = 0;
	ImageAppendShaderData( box_buffer, (float*)&p[0] );
	ImageAppendShaderData( box_buffer, (float*)&p[1] );
	ImageAppendShaderData( box_buffer, (float*)&p[2] );
	ImageAppendShaderData( box_buffer, (float*)&p[2] );
	ImageAppendShaderData( box_buffer, (float*)&p[3] );
	ImageAppendShaderData( box_buffer, (float*)&p[0] );

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

void ZRender_Shader_Simple::DrawLine( ZVector3f *p1, ZVector3f *p2, ZVector4f *c )
{
	box_buffer->used = 0;
	ImageAppendShaderData( box_buffer, (float*)p1 );
	ImageAppendShaderData( box_buffer, (float*)p2 );
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
	
void ZRender_Shader_Simple::DrawBox( ZVector3f *p1, ZVector3f *p2, ZVector3f *p3, ZVector3f *p4
	,ZVector3f *p5, ZVector3f *p6, ZVector3f *p7, ZVector3f *p8
	, ZVector4f *c )
{
	if( !box_buffer )
		box_buffer = ImageCreateShaderBuffer( 3, 24, 8 );
	box_buffer->used = 0;
	ImageAppendShaderData( box_buffer, (float*)p1 );
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
	glDrawArrays( GL_LINES, 0, 24 );
	CheckErr();
}


uintptr_t ZRender_Shader_Simple::SetupShader( void )
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

void ZRender_Shader_Simple::InitShader( PImageShaderTracker tracker )
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
		//ImageSetShaderOpInit( tracker, SimpleShader_OpInit );
		//ImageSetShaderOutput( tracker, SimpleShader_Output );
		//ImageSetShaderReset( tracker, SimpleShader_Reset );
		//ImageSetShaderAppendTristrip( tracker, SimpleShader_AppendTristrip );
	}
}

static void CPROC _InitShader( uintptr_t psvSetup, PImageShaderTracker tracker )
{
	ZRender_Shader_Simple *_this = (ZRender_Shader_Simple*)psvSetup;
	_this->InitShader( tracker );
}

ZRender_Shader_Simple::ZRender_Shader_Simple( ZRender_Interface *render )
{
	this->render = render;
	box_buffer = NULL;
	shader = ImageGetShaderInit( WIDE("BV Simple Shader")
		, _SetupShader
		, _InitShader, (uintptr_t)this );

}