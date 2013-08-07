
#include <stdhdrs.h>

#include "local.h"
#include "shaders.h"

PImageShaderTracker GetShader( CTEXTSTR name, void (CPROC*Init)(PImageShaderTracker) )
{
	PImageShaderTracker tracker;
	INDEX idx;
	LIST_FORALL( l.glActiveSurface->shaders, idx, PImageShaderTracker, tracker )
	{
		if( StrCaseCmp( tracker->name, name ) == 0 )
			return tracker;
	}
	tracker = New( ImageShaderTracker );
	MemSet( tracker, 0, sizeof( ImageShaderTracker ));
	tracker->name = StrDup( name );
	tracker->Init = Init;
	if( Init )
      Init( tracker );
	AddLink( &l.glActiveSurface->shaders, tracker );
	return tracker;
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



void ClearShaders( void )
{
	PImageShaderTracker tracker;
	INDEX idx;
	LIST_FORALL( l.glActiveSurface->shaders, idx, PImageShaderTracker, tracker )
	{
		tracker->flags.set_matrix = 0;
	}
}

void EnableShader( CTEXTSTR shader, ... )
{
	PImageShaderTracker tracker;
	INDEX idx;
	LIST_FORALL( l.glActiveSurface->shaders, idx, PImageShaderTracker, tracker )
	{
		if( StrCaseCmp( tracker->name, shader ) == 0 )
		{
			if( !tracker->glProgramId )
			{
				if( tracker->flags.failed )
				{
               // nothing to enable; shader is failed
					return;
				}
				if( tracker->Init )
					tracker->Init( tracker );
				if( !tracker->glProgramId )
				{
               lprintf( "Shader initialization failed to produce a program; marking shader broken so we don't retry" );
					tracker->flags.failed = 1;
               return;
				}
			}

			//xlprintf( LOG_NOISE+1 )( "Enable shader %s", tracker->name );
			glUseProgram( tracker->glProgramId );
			if( !tracker->flags.set_matrix )
			{
				if( !l.flags.worldview_read )
				{
					GetGLCameraMatrix( l.glActiveSurface->T_Camera, l.worldview );
					l.flags.worldview_read = 1;
				}

				//PrintMatrix( l.worldview );
				glUniformMatrix4fv( tracker->worldview, 1, GL_FALSE, (RCOORD*)l.worldview );
				CheckErr();
				
				//PrintMatrix( l.glActiveSurface->M_Projection );
				glUniformMatrix4fv( tracker->projection, 1, GL_FALSE, (RCOORD*)l.glActiveSurface->M_Projection );
				CheckErr();
				tracker->flags.set_matrix = 1;
			}
			if( tracker->Enable )
			{
				va_list args;
				va_start( args, shader );
				tracker->Enable( tracker, args );
			}
			break;
		}
	}
	if( !tracker )
      lprintf( "Failed to find shader %s", shader );
}


void SetupCommon( PImageShaderTracker tracker, CTEXTSTR position, CTEXTSTR color )
{

   tracker->position_attrib = glGetAttribLocation( tracker->glProgramId, position );

	tracker->eye_point
		=  glGetUniformLocation(tracker->glProgramId, "in_eye_point" );
	CheckErr();

	if( color )
	{
		tracker->color_attrib
			=  glGetAttribLocation(tracker->glProgramId, color );
		CheckErr();
	}
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

void DumpAttribs( int program )
{
	int n;
	int m;
	lprintf( "---- Program %d -----", program );

	glGetProgramiv( program, GL_ACTIVE_ATTRIBUTES, &m );
	for( n = 0; n < m; n++ )
	{
		TEXTCHAR tmp[64];
		size_t length;
		int size;
		int type;

		glGetActiveAttrib( program, n, 64, &length, &size, &type, tmp );
		lprintf( "attribute [%s] %d %d", tmp, size, type );
	}

	glGetProgramiv( program, GL_ACTIVE_UNIFORMS, &m );
	for( n = 0; n < m; n++ )
	{
		TEXTCHAR tmp[64];
		size_t length;
		int size;
		int type;

		glGetActiveUniform( program, n, 64, &length, &size, &type, tmp );
		lprintf( "uniform [%s] %d %d", tmp, size, type );
	}
}

