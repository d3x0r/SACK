
#include <stdhdrs.h>

#include "local.h"

#include "shaders.h"

IMAGE_NAMESPACE

PImageShaderTracker GetShaderInit( CTEXTSTR name, PTRSZVAL (CPROC*Setup)(PTRSZVAL), void (CPROC*Init)(PTRSZVAL,PImageShaderTracker), PTRSZVAL psvSetup )
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


void  SetShaderEnable( PImageShaderTracker tracker, void (CPROC*EnableShader)( PImageShaderTracker tracker,PTRSZVAL, va_list args ), PTRSZVAL psv )
{
	tracker->Enable = EnableShader;
}

void  SetShaderAppendTristrip( PImageShaderTracker tracker, void (CPROC*AppendTriStrip)( struct image_shader_op *op, int triangles, PTRSZVAL,va_list args ) )
{
	tracker->AppendTristrip = AppendTriStrip;
}

void CPROC  SetShaderOutput( PImageShaderTracker tracker, void (CPROC*FlushShader)( PImageShaderTracker tracker, PTRSZVAL, PTRSZVAL, int, int  ) )
{
	tracker->Output = FlushShader;
}
void CPROC  SetShaderReset( PImageShaderTracker tracker, void (CPROC*FlushShader)( PImageShaderTracker tracker, PTRSZVAL, PTRSZVAL  ) )
{
	tracker->Reset = FlushShader;
}
void CPROC  SetShaderOpInit( PImageShaderTracker tracker, PTRSZVAL (CPROC*InitOp)( PImageShaderTracker tracker, PTRSZVAL, int *existing_verts, va_list args  ) )
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
	glGetBooleanv( GL_DEPTH_TEST, &depth_enabled );
	LIST_FORALL( glSurface->shader_local.image_shader_operations, idx, struct image_shader_image_buffer *, image_shader_op )
	{
		// target image has a translation....
		if( image_shader_op->depth )
		{
			if( !depth_enabled )
			{
				depth_enabled = 1;
				glEnable( GL_DEPTH_TEST );
			}
		}
		else
		{
			if( depth_enabled )
			{
				depth_enabled = 0;
				glDisable( GL_DEPTH_TEST );
			}
		}
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
			SetLink( &glSurface->shader_local.image_shader_operations, idx, 0 );
		}
	}
	
	LIST_FORALL( glSurface->shader_local.shader_operations, idx, struct image_shader_op *, op )
	{
		lprintf( WIDE( "Shader %") _string_f WIDE( " %d -> %d  %d" ), op->tracker->name, op->from, op->to, op->to - op->from );
		if( op->tracker->Output )
			op->tracker->Output( op->tracker, op->tracker->psvInit, op->psvKey, op->from, op->to );
		if( op->tracker->Reset )
			op->tracker->Reset( op->tracker, op->tracker->psvInit, op->psvKey );
		Release( op );
		SetLink( &glSurface->shader_local.shader_operations, idx, 0 );
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
			lprintf( WIDE("Shader initialization failed to produce a program; marking shader broken so we don't retry") );
			tracker->flags.failed = 1;
			return;
		}
	}

	//xlprintf( LOG_NOISE+1 )( WIDE("Enable shader %s"), tracker->name );
	glUseProgram( tracker->glProgramId );
	CheckErrf( WIDE("Failed glUseProgram (%s)"), tracker->name );

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
			CheckErrf( WIDE(" (%s)"), tracker->name );
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
	lprintf( WIDE("---- Program %s(%d) -----"), tracker->name, program );

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
		lprintf( WIDE("attribute [%") _cstring_f WIDE("] %d %d %d"), tmp, index, size, type );
	}

	glGetProgramiv( program, GL_ACTIVE_UNIFORMS, &m );
	for( n = 0; n < m; n++ )
	{
		char tmp[64];
		int length;
		int size;
		unsigned int type;
		glGetActiveUniform( program, n, 64, &length, &size, &type, tmp );
		lprintf( WIDE("uniform [%")_cstring_f  WIDE("] %d %d"), tmp, size, type );
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
		lprintf( WIDE("unhandled error before shader: %d"), result );
	}

	//Obtain a valid handle to a vertex shader object.
	tracker->glVertexProgramId = glCreateShader(GL_VERTEX_SHADER);
	CheckErrf(WIDE("vertex shader fail"));

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
#ifdef USE_GLES2
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
			lprintf(WIDE("Vertex shader %s:'program A' failed compilation.\n")
					 , tracker->name );
			//Attempt to get the length of our error log.
#ifdef USE_GLES2
			lprintf( WIDE("length starts at %d"), length );
			glGetShaderiv(tracker->glVertexProgramId, GL_INFO_LOG_LENGTH, &length);

#else
			glGetObjectParameterivARB(tracker->glVertexProgramId, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
#endif
			buffer = NewArray( char, length );
			//Create a buffer.
			buffer[0] = 0;
					
			//Used to get the final length of the log.
#ifdef USE_GLES2
			glGetShaderInfoLog( tracker->glVertexProgramId, length, &final, buffer);
#else
			glGetInfoLogARB(tracker->glVertexProgramId, length, &final, buffer);
#endif
			//Convert our buffer into a string.
			lprintf( WIDE("message: (%d of %d)%s"),  final, length, DupCStr(buffer) );


			if (final > length)
			{
				//The buffer does not contain all the shader log information.
				lprintf(WIDE("Shader Log contained more information!\n"));
			}
			Deallocate( char*, buffer );
		}
	}

	tracker->glFragProgramId = glCreateShader(GL_FRAGMENT_SHADER);
	CheckErrf(WIDE("create shader"));
	glShaderSource(
		tracker->glFragProgramId, //The handle to our shader
		frag_blocks, //The number of files.
		(const GLchar **)frag_code, //An array of const char * data, which represents the source code of theshaders
		(const GLint *)NULL); //An array of string lengths. For have null terminated strings, pass NULL.
	CheckErrf(WIDE("set source fail"));
	 
	//Attempt to compile the shader.
	glCompileShader(tracker->glFragProgramId);
	CheckErrf(WIDE("compile fail %d"), tracker->glFragProgramId);

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
			lprintf(WIDE("Vertex shader %s:'program B' failed compilation.\n")
					 , tracker->name );
			//Attempt to get the length of our error log.
#ifdef USE_GLES2
			glGetShaderiv(tracker->glFragProgramId, GL_INFO_LOG_LENGTH, &length);
#else
			glGetObjectParameterivARB(tracker->glFragProgramId, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
#endif
			buffer = NewArray( char, length );
			buffer[0] = 0;
			//Create a buffer.
					
			//Used to get the final length of the log.
#ifdef USE_GLES2
			glGetShaderInfoLog( tracker->glFragProgramId, length, &final, buffer);
#else
			glGetInfoLogARB(tracker->glFragProgramId, length, &final, buffer);
#endif
			//Convert our buffer into a string.
			lprintf( WIDE("message: %s"), buffer );


			if (final > length)
			{
				//The buffer does not contain all the shader log information.
				printf(WIDE("Shader Log contained more information!\n"));
			}
			Deallocate( char*, buffer );
		
		}
	}
	tracker->glProgramId = glCreateProgram();
	CheckErrf(WIDE("create fail %d"), tracker->glProgramId);
#ifdef USE_GLES2
	glAttachShader(tracker->glProgramId, tracker->glVertexProgramId );
#else
	glAttachObjectARB(tracker->glProgramId, tracker->glVertexProgramId );
#endif
	CheckErrf(WIDE("attach fail"));
#ifdef USE_GLES2
	glAttachShader(tracker->glProgramId, tracker->glFragProgramId );
#else
	glAttachObjectARB(tracker->glProgramId, tracker->glFragProgramId );
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
			glBindAttribLocation(tracker->glProgramId, attrib_order[n].n, attrib_order[n].name );
			CheckErrf( WIDE("bind attrib location") );
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
		CheckErrf( WIDE("SetModelView for (%s)"), tracker->name );

		glUniformMatrix4fv( tracker->modelview, 1, GL_FALSE, matrix );
		CheckErrf( WIDE("SetModelView for (%s)"), tracker->name );

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

static struct image_shader_op * GetShaderOp(PImageShaderTracker tracker, PTRSZVAL psvKey )
{
	struct image_shader_op *op;

	//lprintf( WIDE("append to %p %p  %d"), tracker, psvKey, depth_value );
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
	PTRSZVAL psvKey;
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
		image_shader_op = New( struct image_shader_image_buffer );
		if( l.glActiveSurface->shader_local.last_operation )
			l.glActiveSurface->shader_local.last_operation->last_op = NULL;
		image_shader_op->target = target;
		image_shader_op->tracker = tracker;
		image_shader_op->depth = depth;
		image_shader_op->output = NULL;
		image_shader_op->last_op = NULL;
		AddLink( &l.glActiveSurface->shader_local.image_shader_operations, image_shader_op );
		l.glActiveSurface->shader_local.last_operation = image_shader_op;
	}
	{
		va_list args;
		PTRSZVAL psvKey;
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


IMAGE_NAMESPACE_END
