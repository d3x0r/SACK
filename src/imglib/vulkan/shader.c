
#include <stdhdrs.h>

#include "local.h"

#include "shaders.h"


IMAGE_NAMESPACE

PImageShaderTracker GetShaderInit( CTEXTSTR name, uintptr_t (CPROC*Setup)(uintptr_t), void (CPROC*Init)(uintptr_t,PImageShaderTracker), uintptr_t psvSetup )
{
	PImageShaderTracker tracker;
	INDEX idx;
	if( !l.vkActiveSurface )
		return NULL;
	LIST_FORALL( l.vkActiveSurface->shaders, idx, PImageShaderTracker, tracker )
	{
		if( StrCaseCmp( tracker->name, name ) == 0 )
			return tracker;
	}
	if( Init )
	{
		tracker = New( ImageShaderTracker );
		MemSet( tracker, 0, sizeof( ImageShaderTracker ));
		{
			VkCommandPoolCreateInfo cpci;
			cpci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			cpci.pNext = NULL;
			cpci.flags = 0;
			//cpci.flags |= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT
			cpci.flags |= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			cpci.queueFamilyIndex = 0;
		}
		tracker->name = StrDup( name );
		tracker->Init = Init;
		tracker->Setup = Setup;
		tracker->psvSetup = psvSetup;
		if( tracker->Setup )
			tracker->psvInit =	tracker->Setup( tracker->psvSetup );
		if( tracker->Init )
			tracker->Init( tracker->psvInit, tracker );
		AddLink( &l.vkActiveSurface->shaders, tracker );
		return tracker;
	}
	return NULL;
}


void  SetShaderEnable( PImageShaderTracker tracker, void (CPROC*EnableShader)( PImageShaderTracker tracker,uintptr_t, va_list args ), uintptr_t psv )
{
	tracker->Enable = EnableShader;
}

void  SetShaderAppendTristrip( PImageShaderTracker tracker, void (CPROC*AppendTriStrip)( struct image_shader_op *op, int triangles, uintptr_t,va_list args ) )
{
	tracker->AppendTristrip = AppendTriStrip;
}

void CPROC  SetShaderOutput( PImageShaderTracker tracker, void (CPROC*FlushShader)( PImageShaderTracker tracker, uintptr_t, uintptr_t, int, int  ) )
{
	tracker->Output = FlushShader;
}
void CPROC  SetShaderReset( PImageShaderTracker tracker, void (CPROC*FlushShader)( PImageShaderTracker tracker, uintptr_t, uintptr_t  ) )
{
	tracker->Reset = FlushShader;
}
void CPROC  SetShaderOpInit( PImageShaderTracker tracker, uintptr_t (CPROC*InitOp)( PImageShaderTracker tracker, uintptr_t, int *existing_verts, va_list args  ) )
{
	tracker->InitShaderOp = InitOp;
}

void CloseShaders( struct vkSurfaceData *vkSurface )
{
	PImageShaderTracker tracker;
	INDEX idx;
	LIST_FORALL( vkSurface->shaders, idx, PImageShaderTracker, tracker )
	{
		tracker->flags.set_matrix = 0;
		// all other things are indexes
		if( tracker->vkProgramId )
		{
			// the shaders are deleted as we read the common variable indexes
			//  glDeleteProgram( tracker->vkProgramId );

			tracker->vkProgramId = 0;
		}
	}
}

void FlushShaders( struct vkSurfaceData *vkSurface )
{
	struct image_shader_image_buffer *image_shader_op;
	struct image_shader_op *op;
	INDEX idx;
	INDEX idx2;
	LIST_FORALL( vkSurface->shader_local.image_shader_operations, idx, struct image_shader_image_buffer *, image_shader_op )
	{
		// target image has a translation....
		LIST_FORALL( image_shader_op->output, idx2, struct image_shader_op *, op )
		{
			//EnableShader( op->tracker );
			//lprintf( WIDE( "Shader %") _string_f WIDE( " %d -> %d  %d" ), op->tracker->name, op->from, op->to, op->to - op->from );

			if( op->tracker->Output )
				op->tracker->Output( op->tracker, op->tracker->psvInit, op->psvKey, op->from, op->to );
			if( image_shader_op->tracker->Reset )
					image_shader_op->tracker->Reset( op->tracker, op->tracker->psvInit, op->psvKey );
			//glDrawArrays( GL_TRIANGLES, op->from, op->to );

			//glDrawArrays( 
			Release( op );
			SetLink( &vkSurface->shader_local.image_shader_operations, idx, 0 );
		}
	}
	
	LIST_FORALL( vkSurface->shader_local.shader_operations, idx, struct image_shader_op *, op )
	{
		lprintf( WIDE( "Shader %") _string_f WIDE( " %d -> %d  %d" ), op->tracker->name, op->from, op->to, op->to - op->from );
		if( op->tracker->Output )
			op->tracker->Output( op->tracker, op->tracker->psvInit, op->psvKey, op->from, op->to );
		if( op->tracker->Reset )
			op->tracker->Reset( op->tracker, op->tracker->psvInit, op->psvKey );
		Release( op );
		SetLink( &vkSurface->shader_local.shader_operations, idx, 0 );
	}
	
	vkSurface->shader_local.last_operation = NULL;
}

void ClearShaders( void )
{
	PImageShaderTracker tracker;
	INDEX idx;
	LIST_FORALL( l.vkActiveSurface->shaders, idx, PImageShaderTracker, tracker )
	{
		tracker->flags.set_modelview = 1;
		tracker->flags.set_matrix = 0;
	}
}


void EnableShader( PImageShaderTracker tracker, ... )
{
	if( !tracker )
		return;
	if( !tracker->vkProgramId )
	{
		if( tracker->flags.failed )
		{
			// nothing to enable; shader is failed
			return;
		}
		if( tracker->Init )
			tracker->Init( tracker->psvInit, tracker );
		if( !tracker->vkProgramId )
		{
			lprintf( WIDE("Shader initialization failed to produce a program; marking shader broken so we don't retry") );
			tracker->flags.failed = 1;
			return;
		}
	}

	//xlprintf( LOG_NOISE+1 )( WIDE("Enable shader %s"), tracker->name );
	///////glUseProgram( tracker->vkProgramId );
	///////CheckErrf( WIDE("Failed glUseProgram (%s)"), tracker->name );

	if( tracker->flags.set_modelview && tracker->modelview >= 0 )
	{
		//glUseProgram( tracker->vkProgramId );
		//CheckErr();
		//lprintf( "Set modelview (identity)" );

		////glUniformMatrix4fv( tracker->modelview, 1, GL_FALSE, (float const*)VectorConst_I );
		CheckErr();
		tracker->flags.set_modelview = 0;
	}

	if( !tracker->flags.set_matrix )
	{
		if( !l.flags.worldview_read )
		{
			// T_Camera is the same as l.camera  (camera matrixes are really static)

			////GetGLCameraMatrix( l.glActiveSurface->T_Camera, l.worldview );
			l.flags.worldview_read = 1;
		}
		//PrintMatrix( l.worldview );
		if( tracker->worldview >=0 )
		{
			/////glUniformMatrix4fv( tracker->worldview, 1, GL_FALSE, (RCOORD*)l.worldview );
			CheckErrf( WIDE(" (%s)"), tracker->name );
		}
				
		//PrintMatrix( l.glActiveSurface->M_Projection );
		if( tracker->projection >=0 )
		{
			/////glUniformMatrix4fv( tracker->projection, 1, GL_FALSE, (RCOORD*)l.glActiveSurface->M_Projection );
			CheckErr();
		}
		tracker->flags.set_matrix = 1;

	}
	if( tracker->Enable )
	{
		va_list args;
		va_start( args, tracker );
		tracker->Enable( tracker, tracker->psvInit, args );
	}
}


void AppendShaderTristripQuad( struct image_shader_op *op, ... )
{
	if( !op )
		return;

	if( op->tracker->AppendTristrip )
	{
		va_list args;
		va_start( args, op );
		//struct image_shader_op *op = GetShaderOp( 
		//l.glActiveSurface->shader_local.last_operation = op;
		op->tracker->AppendTristrip( op, 2, op->psvKey, args );
		//l.glActiveSurface->shader_local.last_operation = NULL;
	}
}

void AppendShaderTristrip( struct image_shader_op * op, int triangles, ... )
{
	if( !op )
		return;

	if( op->tracker->AppendTristrip )
	{
		va_list args;
		va_start( args, triangles );
		//l.glActiveSurface->shader_local.last_operation = op;
		op->tracker->AppendTristrip( op, triangles, op->psvKey, args );
		//l.glActiveSurface->shader_local.last_operation = NULL;
	}
}



static void SetupCommon( PImageShaderTracker tracker )
{
	tracker->projection
		= glGetUniformLocation(tracker->vkProgramId, "Projection");
	CheckErr();
	tracker->worldview
		= glGetUniformLocation(tracker->vkProgramId, "worldView");
	CheckErr();
	tracker->modelview
		= glGetUniformLocation(tracker->vkProgramId, "modelView");
	CheckErr();

	if( tracker->vkFragProgramId )
	{
		glDeleteShader( tracker->vkFragProgramId );
		tracker->vkFragProgramId = 0;
	}
	if( tracker->vkVertexProgramId )
	{
		glDeleteShader( tracker->vkVertexProgramId );
		tracker->vkVertexProgramId = 0;
	}
}

void DumpAttribs( PImageShaderTracker tracker, int program )
{
	int n;
	int m;
	lprintf( WIDE("---- Program %s(%d) -----"), tracker->name, program );

	///////glGetProgramiv( program, GL_ACTIVE_ATTRIBUTES, &m );
	for( n = 0; n < m; n++ )
	{
		char tmp[64];
		int length;
		int size;
		unsigned int type;
		int index;

		///////glGetActiveAttrib( program, n, 64, &length, &size, &type, tmp );
		///////index = glGetAttribLocation(program, tmp );
		lprintf( WIDE("attribute [%") _cstring_f WIDE("] %d %d %d"), tmp, index, size, type );
	}

	///////glGetProgramiv( program, GL_ACTIVE_UNIFORMS, &m );
	for( n = 0; n < m; n++ )
	{
		char tmp[64];
		int length;
		int size;
		unsigned int type;
		///////glGetActiveUniform( program, n, 64, &length, &size, &type, tmp );
		lprintf( WIDE("uniform [%")_cstring_f  WIDE("] %d %d"), tmp, size, type );
	}
}


struct thread_compiler *GetCompiler() {
	PTHREAD me = MakeThread();
	struct thread_compiler *compiler;
	INDEX idx;
	LIST_FORALL( l.compilerPool, idx, struct thread_compiler*, compiler ) {
		if( compiler->thread == me )
			break;
	}
	if( !compiler ) {
		compiler = New(struct thread_compiler);
		compiler->thread = me;
		compiler->compiler = shaderc_compiler_initialize();
		AddLink( &l.compilerPool, compiler );
	}
	return compiler;
}

//      shaderc_compiler_t compiler = shaderc_compiler_initialize();
//      shaderc_compilation_result_t result = shaderc_compile_into_spv(
//          compiler, "#version 450\nvoid main() {}", 27
//          , shaderc_glsl_vertex_shader, "main.vert", "main"
//          , nullptr);
//      // Do stuff with compilation results.
//      shaderc_result_release(result);
//      shaderc_compiler_release(compiler);

static struct image_shader_thread_instance * GetInstance( PImageShaderTracker tracker ) {
	struct image_shader_thread_instance *inst;
	PTHREAD me = MakeThread();
	INDEX idx;
	LIST_FORALL( tracker->instances, idx, struct image_shader_thread_instance *, inst ) {
		if( inst->thread == me )
			break;
	}
	if( !inst ) {
		inst = New( struct image_shader_thread_instance );
		MemSet( inst, 0, sizeof( struct image_shader_thread_instance ) );
		inst->thread = me;
	}
	return inst;
}

int CompileShaderEx( PImageShaderTracker tracker
					  , char const*const*vertex_code, int vertex_blocks
					  , char const*const*frag_code, int frag_blocks
					  , struct image_shader_attribute_order *attrib_order, int nAttribs )
{
	uint32_t result=123;
	struct thread_compiler *compiler = GetCompiler();
	struct image_shader_thread_instance *instance = GetInstance( tracker );
	shaderc_compilation_result_t vresult;
	shaderc_compilation_result_t fresult;
	char *code;
	int n;
	int blocklen = 0;
	int len = 0;
	for( n = 0; n < vertex_blocks; n++ ) {
		len += StrLen( vertex_code[n] );
	}
	code = NewArray( char, len + 1 );
	len = 0;
	for( n = 0; n < vertex_blocks; n++ ) {
		MemCpy( code + len, vertex_code[n], blocklen = StrLen( vertex_code[n] ) );
		len += blocklen;
	}

	vresult = shaderc_compile_into_spv(
		compiler->compiler
		, code, len
		, shaderc_glsl_vertex_shader
		, "vert_source"
		, "main"
		, NULL );
	Deallocate( char*, code );

	if( shaderc_result_get_num_errors( vresult ) ) {
		const char *err = shaderc_result_get_error_message( vresult );
		lprintf( "Errors found compiling vertex shader: %s", err );
		shaderc_result_release( vresult );
		return 0;
	}

	len = 0;
	for( n = 0; n < frag_blocks; n++ ) {
		len += StrLen( frag_code[n] );
	}
	code = NewArray( char, len + 1 );
	len = 0;
	for( n = 0; n < frag_blocks; n++ ) {
		MemCpy( code + len, frag_code[n], blocklen = StrLen( frag_code[n] ) );
		len += blocklen;
	}

	fresult = shaderc_compile_into_spv(
		compiler->compiler
		, code, len
		, shaderc_glsl_fragment_shader
		, "frag_source"
		, "main"
		, NULL );
	Deallocate( char*, code );

	if( shaderc_result_get_num_errors( fresult ) ) {
		const char *err = shaderc_result_get_error_message( fresult );
		lprintf( "Errors found compiling vertex shader: %s", err );
		shaderc_result_release( fresult );
		return 0;
	}



	instance->spv_vertex = (uint32_t*)shaderc_result_get_bytes( vresult );
	instance->spv_fragment = (uint32_t*)shaderc_result_get_bytes( fresult );

	shaderc_result_release( vresult );
	shaderc_result_release( fresult );

	return 1;
	//Now, compile the shader source. 
	//Note that glShaderSource takes an array of chars. This is so that one can load multiple vertex shader files at once.
	//This is similar in function to linking multiple C++ files together. Note also that there can only be one "void main" definition
	//In all of the linked source files that are compiling with this funciton.

#if PORTED 

#ifdef USE_GLES2
	glAttachShader(tracker->vkProgramId, tracker->glVertexProgramId );
#else
	glAttachObjectARB(tracker->vkProgramId, tracker->glVertexProgramId );
#endif
	CheckErrf(WIDE("attach fail"));
#ifdef USE_GLES2
	glAttachShader(tracker->vkProgramId, tracker->glFragProgramId );
#else
	glAttachObjectARB(tracker->vkProgramId, tracker->glFragProgramId );
#endif
	CheckErrf( WIDE(" attach2 fail") );

	{
		int n;
		for( n = 0; n < nAttribs; n++ )
		{
#ifdef UNICODE
			lprintf( WIDE("Bind Attrib Location: %d %S"), attrib_order[n].n, attrib_order[n].name );
#else
			lprintf( WIDE("Bind Attrib Location: %d %s"), attrib_order[n].n, attrib_order[n].name );
#endif
			glBindAttribLocation(tracker->vkProgramId, attrib_order[n].n, attrib_order[n].name );
			CheckErrf( WIDE("bind attrib location") );
		}
	}


	glLinkProgram(tracker->vkProgramId);
	CheckErr();
	glUseProgram(tracker->vkProgramId);
	CheckErr();
	SetupCommon( tracker );
#endif
	DumpAttribs( tracker, tracker->vkProgramId );
	return tracker->vkProgramId;
}


int CompileShader( PImageShaderTracker tracker, char const*const*vertex_code, int vertex_blocks, char const*const* frag_code, int frag_blocks )
{
	return CompileShaderEx( tracker, vertex_code, vertex_blocks, frag_code, frag_blocks, NULL, 0 );
}

void SetShaderModelView( PImageShaderTracker tracker, RCOORD *matrix )
{
	if( tracker )
	{
		///////glUseProgram(tracker->vkProgramId);
		///////CheckErrf( WIDE("SetModelView for (%s)"), tracker->name );

		///////glUniformMatrix4fv( tracker->modelview, 1, GL_FALSE, matrix );
		///////CheckErrf( WIDE("SetModelView for (%s)"), tracker->name );

		// modelview already set for shader... so do not set it on enable?
		tracker->flags.set_modelview = 0;
	}
}

struct shader_buffer *CreateShaderBuffer( int dimensions, int start_size, int expand_by )
{
	struct shader_buffer *buffer = New( struct shader_buffer );
	if( !start_size )
		start_size = 16;
	if( !dimensions )
		dimensions = 3;
	buffer->used = 0;
	buffer->dimensions = dimensions;
	buffer->avail = start_size;
	buffer->expand_by = expand_by;
	buffer->data = NewArray( float, buffer->dimensions * buffer->avail );
	return buffer;
}

static void ExpandShaderBuffer( struct shader_buffer *buffer )
{
	float *newbuf;
	int new_size;
	if( buffer->expand_by )
		new_size = buffer->avail + buffer->expand_by;
	else
		new_size = buffer->avail * 2;
	newbuf = NewArray( float, buffer->dimensions * new_size );
	MemCpy( newbuf, buffer->data, sizeof( float ) * buffer->dimensions * buffer->avail );
	Release( buffer->data );
	buffer->avail = new_size;
	buffer->data = newbuf;
}

// this also builds a list of which shaders
// and how much of those shaders are used in order...
void AppendShaderData( struct image_shader_op *op, struct shader_buffer *buffer, float *data )
{
	if( buffer->used == buffer->avail )
		ExpandShaderBuffer( buffer );
	//lprintf( WIDE( "Set position %p" ), buffer->data + buffer->dimensions * buffer->used );
	MemCpy( buffer->data + buffer->dimensions * buffer->used
			, data
			, sizeof( float ) * buffer->dimensions );
	buffer->used++;
	op->to = buffer->used;
}

size_t AppendShaderBufferData( struct shader_buffer *buffer, float *data )
{
	if( buffer->used == buffer->avail )
		ExpandShaderBuffer( buffer );
	//lprintf( WIDE( "Set position %p" ), buffer->data + buffer->dimensions * buffer->used );
	MemCpy( buffer->data + buffer->dimensions * buffer->used
			, data
			, sizeof( float ) * buffer->dimensions );
	return ++buffer->used;
}

static struct image_shader_op * GetShaderOp(PImageShaderTracker tracker, uintptr_t psvKey )
{
	struct image_shader_op *op;

	//lprintf( WIDE("append to %p %p  %d"), tracker, psvKey, depth_value );
	{
		INDEX idx;
		struct image_shader_op *last_use;
		struct image_shader_op *found_use = NULL;
		LIST_FORALL( l.vkActiveSurface->shader_local.tracked_shader_operations, idx, struct image_shader_op *, last_use )
		{
			// if it's found, have to keep going, because we want to find the last one
			if( last_use->tracker == tracker && last_use->psvKey == tracker->psvInit )
			{
				found_use = last_use;
				break;
			}
		}
		if( !found_use )
		{
			op = New( struct image_shader_op );
			op->tracker = tracker;
			op->psvKey = psvKey;
			op->from = found_use?found_use->to : 0;
			op->to = found_use?found_use->to : 0;
			AddLink( &l.vkActiveSurface->shader_local.tracked_shader_operations, op );
		}
		else
			op = found_use;
	}
	/*
	lprintf( WIDE("%") _string_f WIDE(" buffer %p %p is used %d(%d)  %d" )
					, tracker->name
					, buffer, buffer->data
					, buffer->used, buffer->avail
					, op->to );
	*/
	return op;
}

struct image_shader_op * BeginShaderOp(PImageShaderTracker tracker, ... )
{
	struct image_shader_op *shader_op;

	va_list args;
	uintptr_t psvKey;
	int existing;
	va_start( args, tracker );
	psvKey = tracker->InitShaderOp( tracker, tracker->psvInit, &existing, args );
	shader_op  = GetShaderOp( tracker, psvKey );
	return shader_op;
}

struct image_shader_op * BeginImageShaderOp(PImageShaderTracker tracker, Image target, ... )
{
	struct image_shader_op *isibo;
	struct image_shader_image_buffer *image_shader_op;
	if( !tracker )
		return NULL;

	if( l.vkActiveSurface->shader_local.last_operation 
		&& l.vkActiveSurface->shader_local.last_operation->tracker == tracker
		&& l.vkActiveSurface->shader_local.last_operation->target == target
		&& l.vkActiveSurface->shader_local.last_operation->depth == target->depthTest
		)
		image_shader_op = l.vkActiveSurface->shader_local.last_operation;
	else
	{
		image_shader_op = New( struct image_shader_image_buffer );
		if( l.vkActiveSurface->shader_local.last_operation )
			l.vkActiveSurface->shader_local.last_operation->last_op = NULL;
		image_shader_op->target = target;
		image_shader_op->tracker = tracker;
		image_shader_op->depth = target->depthTest;
		image_shader_op->output = NULL;
		image_shader_op->last_op = NULL;
		AddLink( &l.vkActiveSurface->shader_local.image_shader_operations, image_shader_op );
		l.vkActiveSurface->shader_local.last_operation = image_shader_op;
	}
	{
		va_list args;
		uintptr_t psvKey;
		int existing_verts;
		va_start( args, target );
		psvKey = tracker->InitShaderOp( tracker, tracker->psvInit, &existing_verts, args );
		if( image_shader_op->last_op &&
			image_shader_op->last_op->psvKey == psvKey )
			isibo = image_shader_op->last_op;
		else
		{
			isibo = New( struct image_shader_op );
			isibo->from = existing_verts;
			isibo->to = existing_verts;
			isibo->tracker = tracker;
			isibo->psvKey = psvKey;
			image_shader_op->last_op = isibo;
			AddLink( &image_shader_op->output, isibo );
		}
		//shader_op  = GetShaderOp( tracker, psvKey );
	}
	return isibo;
}


void ClearShaderOp(struct image_shader_op *op )
{
}

void AppendImageShaderOpTristrip( struct image_shader_op *op, int triangles, ... )
{
	va_list args;
	va_start( args, triangles );
	if( op )
		op->tracker->AppendTristrip( op, triangles, op->psvKey, args );	
}

void SetShaderDepth( Image pImage, LOGICAL enable ) {
	pImage->depthTest = enable;
}

int GetShaderUniformLocation( PImageShaderTracker shader, const char *uniformName ) {
	//glGetUniformLocation( shader->vkVertexProgramId, "in_eye_point" );
}

void SetUniform4f( int uniformId, RCOORD v1, RCOORD v2, RCOORD v3, RCOORD v4 ) {
	//glUniform4f( )
}

void SetUniform3fv( int uniformId, int n, RCOORD *v1 ) {
	//glUniform4f( )
}


void SetUniform4fv( int uniformId, int n, RCOORD *v1 ) {
	//glUniform4f( )
}

void SetUniform1f( int uniformId, RCOORD v1 ) {
	//glUniform4f( )
}


IMAGE_NAMESPACE_END
