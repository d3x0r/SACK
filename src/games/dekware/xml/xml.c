#include <stdhdrs.h>
#include <../src/sexpat/expat.h>
#define DEFINES_DEKWARE_INTERFACE
#define PLUGIN_MODULE
#include "plugin.h"




static XML_Parser xp;

void XMLCALL start_tags( void *UserData
							  , const XML_Char *name
							  , const XML_Char **atts )
{
   const char **p = atts;
   PENTITY pe = CreateEntityIn( (PENTITY)UserData, SegCreateFromText( name ) );
	lprintf( "begin a tag %s with...", name );
	while( p && *p )
	{
		PTEXT deleteme;
      lprintf( "%s = %s", p[0], p[1] );
		AddVariableExxx( NULL, pe
						  , SegCreateFromText( p[0] )
						  , burst( deleteme = SegCreateFromText( p[1] ) )
						  , FALSE, FALSE, TRUE DBG_SRC );
      LineRelease( deleteme );
      p += 2;
	}
   XML_SetUserData( xp, pe );
}

void XMLCALL end_tags( void *UserData
							, const XML_Char *name )
{
   PENTITY pc = (PENTITY)UserData;
	lprintf( "Ended a tag %s..", name );
   if( pc )
		XML_SetUserData( xp, FindContainer( pc ) );
}

//-------------------------------------------------------------------------
void * MyAllocate( size_t s ) { return Allocate( s ); }
void *MyReallocate( void *p, size_t s ) { return Reallocate( p, s ); }
void MyRelease( void *p ) { Release( p ); }

XML_Memory_Handling_Suite XML_memhandler;//= { MyAllocate, MyReallocate, MyRelease };
//-------------------------------------------------------------------------

int CPROC LoadXML( PSENTIENT ps, PTEXT parameters )
{
   PTEXT filename = GetFileName( ps, &parameters );
	FILE *in = fopen( GetText( filename ), "rb" );
   LineRelease( filename );
	if( in )
	{
      int len;
		POINTER buffer;
		while( xp )
			Relinquish();
		XML_memhandler.malloc_fcn = MyAllocate;
		XML_memhandler.realloc_fcn = MyReallocate;
      XML_memhandler.free_fcn = MyRelease;
		xp = XML_ParserCreate_MM( NULL, &XML_memhandler, NULL );
		XML_SetElementHandler( xp, start_tags, end_tags );
      XML_SetUserData( xp, (void*)ps->Current );
		buffer = XML_GetBuffer( xp, 10000 );
		while( len = fread( buffer, 1, 10000, in ) )
			XML_ParseBuffer( xp, len, len < 10000?TRUE:FALSE );
		XML_ParserFree( xp );
      xp = NULL;
      fclose( in );
      // if in a macro, set the macro success state...
	}
	else
	{
		// if not in a macro, echo a message
	}
   return 0;
}

PUBLIC( char *, RegisterRoutines )( void )
{
	// RegisterRoutine
	// RegisterDevice
	// RegisterObject
	// AddBehavior
	// Volatile Variables may also be able to be registered for everyone (such as now, time, me)
   RegisterRoutine( "xmlload", "Load an xml file in objects.", LoadXML );
   // This defined value needs to be returned...
	return DekVersion;
}

//-----------------------------------------------------------------------------

PUBLIC( void, UnloadPlugin )( void ) // this routine is called when /unload is invoked
{
	// UnregisterRoutine
	// UnregisterDevice
   UnregisterRoutine( "xmlload" );
}




