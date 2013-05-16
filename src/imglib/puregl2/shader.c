
#include <stdhdrs.h>

#include "local.h"
#include "shaders.h"

PImageShaderTracker GetShader( CTEXTSTR name )
{
	PImageShaderTracker tracker;
	INDEX idx;
	LIST_FORALL( l.glActiveSurface->shaders, idx, PImageShaderTracker, tracker )
	{
		if( StrCaseCmp( tracker->name, name ) == 0 )
         return tracker;
	}
	tracker = New( ImageShaderTracker );
	MemSet( tracker, 0, sizeof( tracker ));
	tracker->name = StrDup( name );
	AddLink( &l.glActiveSurface->shaders, tracker );
   return tracker;
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
         xlprintf( LOG_NOISE+1 )( "Enable shader %s", tracker->name );
			glUseProgram( tracker->glProgramId );
			if( !tracker->flags.set_matrix )
			{
				if( !l.flags.worldview_read )
				{
					GetGLCameraMatrix( l.glActiveSurface->T_Camera, l.worldview );
					l.flags.worldview_read = 1;
				}
				if( !l.flags.projection_read )
				{
					glGetFloatv( GL_PROJECTION_MATRIX, l.projection );
					l.flags.projection_read = 1;
				}

				glUniformMatrix4fv( tracker->worldview, 1, GL_FALSE, (RCOORD*)l.worldview );
				CheckErr();
				glUniformMatrix4fv( tracker->projection, 1, GL_FALSE, l.projection );
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

