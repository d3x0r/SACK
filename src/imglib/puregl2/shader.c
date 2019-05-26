
#include <stdhdrs.h>

#ifndef __LINUX__
#  include <GL/glew.h>
#endif
#include "local.h"
#include "shaders.h"

IMAGE_NAMESPACE

PImageShaderTracker GetShaderInit( CTEXTSTR name, uintptr_t (CPROC*Setup)(uintptr_t), void (CPROC*Init)(uintptr_t,PImageShaderTracker), uintptr_t psvSetup )
{
	PImageShaderTracker tracker;
	INDEX idx;
	if( !l.glActiveSurface )
		return NULL;
	LIST_FORALL( l.glActiveSurface->shaders, idx, PImageShaderTracker, tracker )
	{
		if( StrCaseCmp( tracker->name, name ) == 0 )
			return tracker;
	}
	if( Init )
	{
		tracker = New( ImageShaderTracker );
		MemSet( tracker, 0, sizeof( ImageShaderTracker ));
		tracker->name = StrDup( name );
		tracker->Init = Init;
		tracker->Setup = Setup;
		tracker->psvSetup = psvSetup;
		if( tracker->Setup )
			tracker->psvInit =	tracker->Setup( tracker->psvSetup );
		if( tracker->Init )
			tracker->Init( tracker->psvInit, tracker );
		AddLink( &l.glActiveSurface->shaders, tracker );
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
void CPROC  SetShaderResetOp( PImageShaderTracker tracker, void (CPROC* ResetOp)( PImageShaderTracker tracker, uintptr_t, uintptr_t  ) )
{
	tracker->ResetOp = ResetOp;
}
void CPROC  SetShaderReset( PImageShaderTracker tracker, void( CPROC* ResetShader )(PImageShaderTracker tracker, uintptr_t) )
{
	tracker->ResetShader = ResetShader;
}
void CPROC  SetShaderOpInit( PImageShaderTracker tracker, uintptr_t (CPROC*InitOp)( PImageShaderTracker tracker, uintptr_t, int *existing_verts, va_list args  ) )
{
	tracker->InitShaderOp = InitOp;
}

void CloseShaders( struct glSurfaceData *glSurface )
{
	PImageShaderTracker tracker;
	INDEX idx;
	LIST_FORALL( glSurface->shaders, idx, PImageShaderTracker, tracker )
	{
		tracker->flags.set_matrix = 0;
		// all other things are indexes
		if( tracker->glProgramId )
		{
			// the shaders are deleted as we read the common variable indexes
			glDeleteProgram( tracker->glProgramId );
			tracker->glProgramId = 0;
		}
	}
}

void FlushShaders( struct glSurfaceData *glSurface )
{
	struct image_shader_image_buffer *image_shader_op;
	struct image_shader_op *op;

	INDEX idx;
	INDEX idx2;
	GLboolean depth_enabled;
	PLIST trackers = NULL;
	glGetBooleanv( GL_DEPTH_TEST, &depth_enabled );
	LIST_FORALL( glSurface->shader_local.image_shader_operations, idx, struct image_shader_image_buffer *, image_shader_op )
	{
		// target image has a translation....
		if( image_shader_op->depth )
		{
			if( !depth_enabled )
			{
				depth_enabled = 1;
				glEnable( GL_DEPTH_TEST ); CheckErr();
			}
		}
		else
		{
			if( depth_enabled )
			{
				depth_enabled = 0;
				glDisable( GL_DEPTH_TEST ); CheckErr();
			}
		}
		if( FindLink( &trackers, image_shader_op->tracker ) == INVALID_INDEX ) {
			AddLink( &trackers, image_shader_op->tracker );
		}
		LIST_FORALL( image_shader_op->output, idx2, struct image_shader_op *, op )
		{
			if( op->tracker->Output )
				op->tracker->Output( op->tracker, op->tracker->psvInit, op->psvKey, op->from, op->to );
			if( image_shader_op->tracker->ResetOp )
				image_shader_op->tracker->ResetOp( op->tracker, op->tracker->psvInit, op->psvKey );
			Release( op );
		}

		if( image_shader_op->output )
			DeleteList( &image_shader_op->output );
		Release( image_shader_op );
		SetLink( &glSurface->shader_local.image_shader_operations, idx, 0 );
	}
	
	LIST_FORALL( glSurface->shader_local.shader_operations, idx, struct image_shader_op *, op )
	{
		lprintf( "Shader %" _string_f " %d -> %d  %d", op->tracker->name, op->from, op->to, op->to - op->from );
		if( FindLink( &trackers, op->tracker ) == INVALID_INDEX ) {
			AddLink( &trackers, image_shader_op->tracker );
		}
		if( op->tracker->Output )
			op->tracker->Output( op->tracker, op->tracker->psvInit, op->psvKey, op->from, op->to );
		if( op->tracker->ResetOp )
			op->tracker->ResetOp( op->tracker, op->tracker->psvInit, op->psvKey );
		Release( op );
		SetLink( &glSurface->shader_local.shader_operations, idx, 0 );
	}
	{
		struct image_shader_tracker* tracker;
		INDEX idx;
		LIST_FORALL( trackers, idx, struct image_shader_tracker*, tracker ) {
			if( tracker->ResetShader )
				tracker->ResetShader( tracker, tracker->psvInit );
		}
	}

	glSurface->shader_local.last_operation = NULL;
}

void ClearShaders( void )
{
	PImageShaderTracker tracker;
	INDEX idx;
	LIST_FORALL( l.glActiveSurface->shaders, idx, PImageShaderTracker, tracker )
	{
		tracker->flags.set_modelview = 1;
		tracker->flags.set_matrix = 0;
	}
}


void EnableShader( PImageShaderTracker tracker, ... )
{
	if( !tracker )
		return;
	if( !tracker->glProgramId )
	{
		if( tracker->flags.failed )
		{
			// nothing to enable; shader is failed
			return;
		}
		if( tracker->Init )
			tracker->Init( tracker->psvInit, tracker );
		if( !tracker->glProgramId )
		{
			lprintf( "Shader initialization failed to produce a program; marking shader broken so we don't retry" );
			tracker->flags.failed = 1;
			return;
		}
	}

	//xlprintf( LOG_WARNING+1 )( "Enable shader %s", tracker->name );
	glUseProgram( tracker->glProgramId );
	CheckErrf( "Failed glUseProgram (%s)", tracker->name );

	if( tracker->flags.set_modelview && tracker->modelview >= 0 )
	{
		//glUseProgram( tracker->glProgramId );
		//CheckErr();
		//lprintf( "Set modelview (identity)" );
		glUniformMatrix4fv( tracker->modelview, 1, GL_FALSE, (float const*)VectorConst_I );
		CheckErr();
		tracker->flags.set_modelview = 0;
	}

	if( !tracker->flags.set_matrix )
	{
		if( !l.flags.worldview_read )
		{
			// T_Camera is the same as l.camera  (camera matrixes are really static)
			GetGLCameraMatrix( l.glActiveSurface->T_Camera, l.worldview );
			l.flags.worldview_read = 1;
		}
		//PrintMatrix( l.worldview );
		if( tracker->worldview >=0 )
		{
			glUniformMatrix4fv( tracker->worldview, 1, GL_FALSE, (RCOORD*)l.worldview );
			CheckErrf( " (%s)", tracker->name );
		}
				
		//PrintMatrix( l.glActiveSurface->M_Projection );
		if( tracker->projection >=0 )
		{
			glUniformMatrix4fv( tracker->projection, 1, GL_FALSE, (RCOORD*)l.glActiveSurface->M_Projection );
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
#if 0
	// check for memory leaks.
	{
		static int n;
		n++;
		if( (n % 3000) == 0 ) {
			DebugDumpMem();
		}
	}
#endif
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
		= glGetUniformLocation(tracker->glProgramId, "Projection");
	CheckErr();
	tracker->worldview
		= glGetUniformLocation(tracker->glProgramId, "worldView");
	CheckErr();
	tracker->modelview
		= glGetUniformLocation(tracker->glProgramId, "modelView");
	CheckErr();

	if( tracker->glFragProgramId )
	{
		glDeleteShader( tracker->glFragProgramId );
		tracker->glFragProgramId = 0;
	}
	if( tracker->glVertexProgramId )
	{
		glDeleteShader( tracker->glVertexProgramId );
		tracker->glVertexProgramId = 0;
	}
}

void DumpAttribs( PImageShaderTracker tracker, int program )
{
	int n;
	int m;
	lprintf( "---- Program %s(%d) -----", tracker->name, program );

	glGetProgramiv( program, GL_ACTIVE_ATTRIBUTES, &m );
	for( n = 0; n < m; n++ )
	{
		char tmp[64];
		int length;
		int size;
		unsigned int type;
		int index;

		glGetActiveAttrib( program, n, 64, &length, &size, &type, tmp );
		index = glGetAttribLocation(program, tmp );
		lprintf( "attribute [%" _cstring_f "] %d %d %d", tmp, index, size, type );
	}

	glGetProgramiv( program, GL_ACTIVE_UNIFORMS, &m );
	for( n = 0; n < m; n++ )
	{
		char tmp[64];
		int length;
		int size;
		unsigned int type;
		glGetActiveUniform( program, n, 64, &length, &size, &type, tmp );
		lprintf( "uniform [%"_cstring_f  "] %d %d", tmp, size, type );
	}
}


int CompileShaderEx( PImageShaderTracker tracker
					  , char const*const*vertex_code, int vertex_blocks
					  , char const*const*frag_code, int frag_blocks
					  , struct image_shader_attribute_order *attrib_order, int nAttribs )
{
	GLint result=123;

	if( result = glGetError() )
	{
		lprintf( "unhandled error before shader: %d", result );
	}

	//Obtain a valid handle to a vertex shader object.
	tracker->glVertexProgramId = glCreateShader(GL_VERTEX_SHADER);
	CheckErrf("vertex shader fail");

	//Now, compile the shader source. 
	//Note that glShaderSource takes an array of chars. This is so that one can load multiple vertex shader files at once.
	//This is similar in function to linking multiple C++ files together. Note also that there can only be one "void main" definition
	//In all of the linked source files that are compiling with this funciton.
	glShaderSource(
		tracker->glVertexProgramId, //The handle to our shader
		vertex_blocks, //The number of files.
		(const GLchar **)vertex_code, //An array of const char * data, which represents the source code of theshaders
		(const GLint *)NULL); //An array of string leng7ths. For have null terminated strings, pass NULL.
	 
	//Attempt to compile the shader.
	glCompileShader(tracker->glVertexProgramId);
	{
		//Error checking.
#if defined( USE_GLES2 ) || defined( __EMSCRIPTEN__ )
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
			lprintf("Vertex shader %s:'program A' failed compilation.\n"
					 , tracker->name );
			//Attempt to get the length of our error log.
#if defined( USE_GLES2 ) || defined( __EMSCRIPTEN__ )
			lprintf( "length starts at %d", length );
			glGetShaderiv(tracker->glVertexProgramId, GL_INFO_LOG_LENGTH, &length);

#else
			glGetObjectParameterivARB(tracker->glVertexProgramId, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
#endif
			buffer = NewArray( char, length );
			//Create a buffer.
			buffer[0] = 0;
					
			//Used to get the final length of the log.
#if defined( USE_GLES2 ) || defined( __EMSCRIPTEN__ )
			glGetShaderInfoLog( tracker->glVertexProgramId, length, &final, buffer);
#else
			glGetInfoLogARB(tracker->glVertexProgramId, length, &final, buffer);
#endif
			//Convert our buffer into a string.
			lprintf( "message: (%d of %d)%s",  final, length, DupCStr(buffer) );


			if (final > length)
			{
				//The buffer does not contain all the shader log information.
				lprintf("Shader Log contained more information!\n");
			}
			Deallocate( char*, buffer );
		}
	}

	tracker->glFragProgramId = glCreateShader(GL_FRAGMENT_SHADER);
	CheckErrf("create shader");
	glShaderSource(
		tracker->glFragProgramId, //The handle to our shader
		frag_blocks, //The number of files.
		(const GLchar **)frag_code, //An array of const char * data, which represents the source code of theshaders
		(const GLint *)NULL); //An array of string lengths. For have null terminated strings, pass NULL.
	CheckErrf("set source fail");
	 
	//Attempt to compile the shader.
	glCompileShader(tracker->glFragProgramId);
	CheckErrf("compile fail %d", tracker->glFragProgramId);

	{
		//Error checking.
#if defined( USE_GLES2 ) || defined( __EMSCRIPTEN__ )
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
			lprintf("Vertex shader %s:'program B' failed compilation.\n"
					 , tracker->name );
			lprintf("Vertex shader %s\n"
					 , vertex_code[0] );
			lprintf("Vertex shader %s\n"
					 , frag_code[0] );
			//Attempt to get the length of our error log.
#if defined( USE_GLES2 ) || defined( __EMSCRIPTEN__ )
			glGetShaderiv(tracker->glFragProgramId, GL_INFO_LOG_LENGTH, &length);
#else
			glGetObjectParameterivARB(tracker->glFragProgramId, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
#endif
			buffer = NewArray( char, length );
			buffer[0] = 0;
			//Create a buffer.
					
			//Used to get the final length of the log.
#if defined( USE_GLES2 ) || defined( __EMSCRIPTEN__ )
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
			Deallocate( char*, buffer );
		
		}
	}
	tracker->glProgramId = glCreateProgram();
	CheckErrf("create fail %d", tracker->glProgramId);
#if defined( USE_GLES2 ) || defined( __EMSCRIPTEN__ )
	glAttachShader(tracker->glProgramId, tracker->glVertexProgramId );
#else
	glAttachObjectARB(tracker->glProgramId, tracker->glVertexProgramId );
#endif
	CheckErrf("attach fail");
#if defined( USE_GLES2 ) || defined( __EMSCRIPTEN__ )
	glAttachShader(tracker->glProgramId, tracker->glFragProgramId );
#else
	glAttachObjectARB(tracker->glProgramId, tracker->glFragProgramId );
#endif
	CheckErrf( " attach2 fail" );

	{
		int n;
		for( n = 0; n < nAttribs; n++ )
		{
#ifdef UNICODE
			lprintf( "Bind Attrib Location: %d %S", attrib_order[n].n, attrib_order[n].name );
#else
			lprintf( "Bind Attrib Location: %d %s", attrib_order[n].n, attrib_order[n].name );
#endif
			glBindAttribLocation(tracker->glProgramId, attrib_order[n].n, attrib_order[n].name );
			CheckErrf( "bind attrib location" );
		}
	}


	glLinkProgram(tracker->glProgramId);
	CheckErr();
	glUseProgram(tracker->glProgramId);
	CheckErr();
	SetupCommon( tracker );

	DumpAttribs( tracker, tracker->glProgramId );
	return tracker->glProgramId;
}


int CompileShader( PImageShaderTracker tracker, char const*const*vertex_code, int vertex_blocks, char const*const* frag_code, int frag_blocks )
{
	return CompileShaderEx( tracker, vertex_code, vertex_blocks, frag_code, frag_blocks, NULL, 0 );
}

void SetShaderModelView( PImageShaderTracker tracker, RCOORD *matrix )
{
	if( tracker )
	{
		glUseProgram(tracker->glProgramId);
		CheckErrf( "SetModelView for (%s)", tracker->name );

		glUniformMatrix4fv( tracker->modelview, 1, GL_FALSE, matrix );
		CheckErrf( "SetModelView for (%s)", tracker->name );

		// modelview already set for shader... so do not set it on enable?
		tracker->flags.set_modelview = 0;
	}
}

struct shader_buffer *CreateShaderBuffer_( int dimensions, int start_size, int expand_by DBG_PASS )
{
	struct shader_buffer *buffer = (struct shader_buffer*)HeapAllocateEx( 0, sizeof( struct shader_buffer ) DBG_RELAY );
	if( !start_size )
		start_size = 16;
	if( !dimensions )
		dimensions = 3;
	buffer->used = 0;
	buffer->dimensions = dimensions;
	buffer->avail = start_size;
	buffer->expand_by = expand_by;
	buffer->data = (float*)HeapAllocateEx( 0, sizeof( float ) * (buffer->dimensions * buffer->avail) DBG_RELAY);
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
	if( new_size > 10000 ) DebugBreak();
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
	//lprintf( "Set position %p", buffer->data + buffer->dimensions * buffer->used );
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
	//lprintf( "Set position %p", buffer->data + buffer->dimensions * buffer->used );
	MemCpy( buffer->data + buffer->dimensions * buffer->used
			, data
			, sizeof( float ) * buffer->dimensions );
	return ++buffer->used;
}

static struct image_shader_op * GetShaderOp(PImageShaderTracker tracker, uintptr_t psvKey )
{
	struct image_shader_op *op;

	//lprintf( "append to %p %p  %d", tracker, psvKey, depth_value );
	{
		INDEX idx;
		struct image_shader_op *last_use;
		struct image_shader_op *found_use = NULL;
		LIST_FORALL( l.glActiveSurface->shader_local.tracked_shader_operations, idx, struct image_shader_op *, last_use )
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
			//glGetBooleanv( GL_DEPTH_TEST, &op->depth_enabled );
			AddLink( &l.glActiveSurface->shader_local.tracked_shader_operations, op );
		}
		else
			op = found_use;
	}
	/*
	lprintf( "%" _string_f " buffer %p %p is used %d(%d)  %d"
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
	GLboolean depth;
	if( !tracker )
		return NULL;
	glGetBooleanv(GL_DEPTH_TEST, &depth ); 

	if( l.glActiveSurface->shader_local.last_operation 
		&& l.glActiveSurface->shader_local.last_operation->tracker == tracker
		&& l.glActiveSurface->shader_local.last_operation->target == target
		&& l.glActiveSurface->shader_local.last_operation->depth == depth
		)
		image_shader_op = l.glActiveSurface->shader_local.last_operation;
	else
	{
		INDEX imageOp;
		LIST_FORALL( l.glActiveSurface->shader_local.image_shader_operations, imageOp,
			struct image_shader_image_buffer*, image_shader_op ) {
			if( ( image_shader_op->tracker == tracker  )
				&& (image_shader_op->target == target)
				&& (image_shader_op->depth == depth) )
				break;
		}
		if( !image_shader_op ) {
			image_shader_op = New( struct image_shader_image_buffer );
			if( l.glActiveSurface->shader_local.last_operation )
				l.glActiveSurface->shader_local.last_operation->last_op = NULL;
			image_shader_op->target = target;
			image_shader_op->tracker = tracker;
			image_shader_op->depth = depth;
			image_shader_op->output = NULL;
			image_shader_op->last_op = NULL;
			AddLink( &l.glActiveSurface->shader_local.image_shader_operations, image_shader_op );
		}
		l.glActiveSurface->shader_local.last_operation = image_shader_op;
	}
	{
		va_list args;
		uintptr_t psvKey;
		int existing_verts;
		va_start( args, target );
		psvKey = tracker->InitShaderOp( tracker, tracker->psvInit, &existing_verts, args );
		if( image_shader_op->last_op &&
			image_shader_op->last_op->psvKey == psvKey ) {
			//lprintf( "Exisiting isibo is still this one...," );
			isibo = image_shader_op->last_op;
			
		}
		else
		{
			INDEX oldOp;
			LIST_FORALL( image_shader_op->output, oldOp, struct image_shader_op *, isibo ) {
				if( isibo->psvKey == psvKey ) {
					//lprintf( "using deeper isibo is still this one...," );
					image_shader_op->last_op = isibo;
					break;
				}
			}
			if( !isibo ) {
				isibo = New( struct image_shader_op );
				isibo->from = existing_verts;
				isibo->to = existing_verts;
				isibo->tracker = tracker;
				isibo->psvKey = psvKey;
				image_shader_op->last_op = isibo;
				AddLink( &image_shader_op->output, isibo );
			}
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
	int r = glGetUniformLocation( shader->glProgramId, uniformName );
	CheckErr();
	return r;
}

void SetUniform4f( int uniformId, float v1, float v2, float v3, float v4 ) {
	glUniform4f( uniformId, v1, v2, v3, v4 );
	CheckErr();
}


void SetUniform4fv( int uniformId, int n, RCOORD *v1 ) {
	glUniform4fv( uniformId, n, v1 );
	CheckErr();
}

void SetUniform3fv( int uniformId, int n, RCOORD *v1 ) {
	glUniform3fv( uniformId, n, v1 );
	CheckErr();
}

void SetUniform1f( int uniformId, RCOORD v1 ) {
	glUniform1f( uniformId, v1 );
	CheckErr();
}

void SetUniformMatrix4fv( int uniformId, int n, int sign, RCOORD *v1 ) {
	glUniformMatrix4fv( uniformId, n, sign, v1 );
	CheckErr();

}

IMAGE_NAMESPACE_END
