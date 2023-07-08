
#include <stdhdrs.h>
#include <sharemem.h>
#include <../src/contrib/sexpat/expat.h>
#include <configscript.h>
#include <filesys.h>
#include "controlstruc.h"

#include <image.h>
// XML_SetUserData
// XML_GetUserData

//#define DEBUG_RESOURCE_NAME_LOOKUP

PSI_XML_NAMESPACE
// need to protect this ...
// ptu it in a structure...
static struct {
	CRITICALSECTION cs;
	void (CPROC*InitProc)(uintptr_t,PSI_CONTROL);
	uintptr_t psv; // psvInitProc
	PSI_CONTROL frame;
	struct {
		BIT_FIELD cs_initialized;
	} flags;
}l;

struct xml_userdata {
	XML_Parser xp;
	PSI_CONTROL pc;
};

static LOGICAL SetP( TEXTSTR *p, const XML_Char **atts )
{
	if( p[0] )
		Deallocate( TEXTSTR, p[0] );
	if( p[1] )
		Deallocate( TEXTSTR, p[1] );
	if( atts[0] )
	{
		p[0] = DupCStr( atts[0] );
		p[1] = DupCStr( atts[1] );
		return TRUE;
	}
	else
		return FALSE;
}

void XMLCALL start_tags( void *UserData
							  , const XML_Char *name
							  , const XML_Char **atts )
{
	struct xml_userdata *userdata = (struct xml_userdata *)UserData;
	uint32_t ID = -1;
	uint32_t x, y;
	uint32_t edit_set = 0;
	uint32_t disable_edit = 0;
	uint32_t width, height;
	TEXTSTR caption = NULL;
	uint32_t border = 0;
	LOGICAL border_set;
	TEXTSTR font = NULL;
	TEXTSTR control_data = NULL;
	TEXTSTR IDName = NULL;
	TEXTSTR type = NULL;
	PSI_CONTROL pc;
	TEXTSTR p[2];
	p[0] = NULL;
	p[1] = NULL;
	//CTEXTSTR *p = atts;
	//lprintf( "begin a tag %s with...", name );
	while( SetP( p, atts ) )
	{
		//lprintf( "begin a attrib %s=%s with...", p[0], p[1] );
		if( strcmp( p[0], "ID" ) == 0 )
		{
			ID = (int)IntCreateFromText( p[1] );
		}
		else if( strcmp( p[0], "IDName" ) == 0 )
		{
			IDName = StrDup( p[1] );
		}
		else if( strcmp( p[0], "border" ) == 0 )
		{
			border_set = TRUE;
			//border = (int)IntCreateFromText( p[1] );
			tscanf( p[1], "%" _32fx, &border );
		}
		else if( strcmp( p[0], "size" ) == 0 )
		{
			tscanf( p[1], "%" _32f "," "%" _32f, &width, &height );
		}
		else if( StrCmp( p[0], "position" ) == 0 )
		{
			tscanf( p[1], "%" _32f "," "%" _32f, &x, &y );
		}
		else if( strcmp( p[0], "caption" ) == 0 )
		{
			caption = StrDup( p[1] );
		}
		else if( strcmp( p[0], "font" ) == 0 )
		{
			font = StrDup( p[1] );
		}
		else if( strcmp( p[0], "PrivateData" ) == 0 )
		{
			control_data = StrDup( p[1] );
		}
		else if( strcmp( p[0], "type" ) == 0 )
		{
			type = StrDup( p[1] );
		}
		else if( strcmp( p[0], "edit" ) == 0 )
		{
			edit_set = 1;
			disable_edit = (int)IntCreateFromText( p[1] );
		}
		else
		{
			lprintf( "Unknown Att Pair = (%s=%s)", p[0], p[1] );
		}
		atts += 2;
	}
	if( IDName )
	{
		//lprintf( "Making a control... %s %s %s", type?type:"notype", caption?caption:"nocatpion", IDName );
		pc = MakeNamedCaptionedControlByName( userdata->pc
														, type
														, x, y
														, width, height
														, IDName
														, ID
														, caption );

		Release( IDName );
	}
	else
	{
		//lprintf( "Making a control... %s %s", type?type:"notype", caption?caption:"nocatpion" );
		pc = MakeNamedCaptionedControl( userdata->pc
														, type
														, x, y
														, width, height
														, ID
												, caption );
	}
	if( border )
	{
		if( pc->parent )
			SetCommonBorder( pc, border | BORDER_NOCAPTION );
		else
			SetCommonBorder( pc, border );
	}
	//lprintf( "control done..." );
	if( pc )
	{
		if( edit_set )
			pc->flags.bEditLoaded = 1; // mark that edit was loaded from the XML file.  the function of bEditSet can be used to fix setting edit before frame display.
		pc->flags.bEditSet = edit_set;
		pc->flags.bNoEdit = disable_edit;
	}
	if( caption ) 
		Release( caption );
	if( type )
		Release( type );
	if( !l.frame )
		l.frame = pc; // mark this as the frame to return
	if( font )
	{
		POINTER fontbuf = NULL;
		size_t fontlen;
		if( DecodeBinaryConfig( font, &fontbuf, &fontlen ) )
			SetCommonFont( pc, RenderFontData( (PFONTDATA)fontbuf ) );
		Release( font );
	}
	if( control_data )
		Release( control_data );
	// SetCommonFont()
	//
	userdata->pc = pc;
}

void XMLCALL end_tags( void *UserData
							, const XML_Char *name )
{
	struct xml_userdata *userdata = (struct xml_userdata *)UserData;
	if( userdata->pc )
		userdata->pc = userdata->pc->parent;
}

//-------------------------------------------------------------------------
static struct {
	CTEXTSTR pFile;
	uint32_t nLine;
} current_loading;
#if defined( _DEBUG ) || defined( _DEBUG_INFO )
void * MyAllocate( size_t s ) { return AllocateEx( s, current_loading.pFile, current_loading.nLine ); }
#else
void * MyAllocate( size_t s ) { return AllocateEx( s ); }
#endif
void *MyReallocate( void *p, size_t s ) { return Reallocate( p, s ); }
void MyRelease( void *p ) { Release( p ); }

static XML_Memory_Handling_Suite XML_memhandler;// = { MyAllocate, MyReallocate, MyRelease };
//-------------------------------------------------------------------------

// expected character buffer of appropriate size.
PSI_CONTROL ParseXMLFrameEx( PSI_CONTROL parent, POINTER buffer, size_t size DBG_PASS )
{
	POINTER xml_buffer;
	struct xml_userdata userdata;
	l.frame = NULL;
#  ifdef USE_INTERFACES
	GetMyInterface();
	if( !g.MyImageInterface )
		return NULL;
#endif
	//lprintf( "Beginning parse frame..." );
#if DBG_AVAILABLE
	current_loading.pFile = pFile;
	current_loading.nLine = nLine;
#endif
	XML_memhandler.malloc_fcn = MyAllocate;
	XML_memhandler.realloc_fcn = MyReallocate;
	XML_memhandler.free_fcn = MyRelease;
	userdata.xp = XML_ParserCreate_MM( NULL, &XML_memhandler, NULL );
	userdata.pc = NULL;
	XML_SetElementHandler( userdata.xp, start_tags, end_tags );
	XML_SetUserData( userdata.xp, &userdata );
	xml_buffer = XML_GetBuffer( userdata.xp, size );
	MemCpy( xml_buffer, buffer, size );
	if( XML_ParseBuffer( userdata.xp, size, TRUE ) == XML_STATUS_ERROR )
	{
		lprintf( "Error in XML parse %d  at line %" _size_f "(%" _size_f ")", XML_GetErrorCode( userdata.xp ),XML_GetCurrentLineNumber( userdata.xp ), XML_GetCurrentColumnNumber( userdata.xp ) );
	}
	XML_ParserFree( userdata.xp );
	userdata.xp = 0;
	//lprintf( "Parse done..." );
	return l.frame;
}

PSI_CONTROL LoadXMLFrameOverExx( PSI_CONTROL parent, CTEXTSTR file, LOGICAL create DBG_PASS )
//PSI_CONTROL  LoadXMLFrame( char *file )
{
	POINTER buffer;
	size_t size;
	TEXTSTR delete_filename = NULL;
	TEXTSTR filename = (TEXTSTR)file; // assume this is the name until later
	PSI_CONTROL frame;
	GetMyInterface();
#  ifdef USE_INTERFACES
	if( !g.MyImageInterface )
		return NULL;
#endif
	if( !l.flags.cs_initialized )
	{
		InitializeCriticalSec( &l.cs );
		l.flags.cs_initialized = 1;
	}
	EnterCriticalSec( &l.cs );
	// enter critical section!
	l.frame = NULL;
#if DBG_AVAILABLE
	current_loading.pFile = pFile;
	current_loading.nLine = nLine;
#endif
	size = 0;
//#ifdef UNDER_CE
	{
		static TEXTCHAR tmpbuf[512];
		//TEXTSTR tmp;
		FILE *file_read = sack_fopen( 0, file, "rb" );
		if( !file_read ) {
			snprintf( tmpbuf, 511, "%%frames%%/%s", file );
			file_read = sack_fopen( 0, tmpbuf, "rb" );
		}
		if( !file_read ) {
			snprintf( tmpbuf, 511, "@/../share/SACK/frames/%s", file );
			file_read = sack_fopen( 0, tmpbuf, "rb" );
		}
		if( file_read )
		{
			size = sack_fsize( file_read );
			buffer = Allocate( size );
			sack_fread( buffer, size, 1, file_read );
			sack_fclose( file_read );
			//lprintf( "loaded frame blob %s %d %p", file, size, buffer );
		}
		else
			buffer = NULL;
	}
//#else
//	buffer = OpenSpace( NULL, file, &size );
//#endif
	if( !buffer || !size )
	{
		// attempt secondary open within frames/*
		size = 0;
		{
			INDEX group;
			FILE *file_read = sack_fopen( group = GetFileGroup( "PSI Frames", "./frames" ), file, "rb" );
			if( file_read )
			{
				size = sack_fsize( file_read );
				buffer = Allocate( size );
				sack_fread( buffer, size, 1, file_read );
				sack_fclose( file_read );
				//lprintf( "loaded font blob %s %d %p", file, zz, buffer );
			}
			else
				buffer = NULL;
		}
	}
	if( buffer && size )
	{
		ParseXMLFrame( parent, buffer, size );
		Release( buffer );
	}

	if( create && !l.frame )
	{
		//create_editable_dialog:
		{
			PSI_CONTROL frame;
			frame = CreateFrame( file
									 , 0, 0
									 , 420, 250, 0, NULL );
			frame->save_name = StrDup( filename );
			DisplayFrameOver( frame, parent );
			EditFrame( frame, TRUE );
			CommonWaitEndEdit( &frame ); // edit may result in destroying (cancelling) the whole thing
			if( frame )
			{
				// save it, and result with it...
				SaveXMLFrame( frame, filename );
				if( delete_filename )
					Release(delete_filename );
				LeaveCriticalSec( &l.cs );
				return frame;
			}
			if( delete_filename )
				Release(delete_filename );
			//DestroyControl( &frame );
			LeaveCriticalSec( &l.cs );
			return NULL;
		}
	}
	// yes this is an assignment
	if( frame = l.frame )
		l.frame->save_name = StrDup( filename );
	if( delete_filename )
		Release(delete_filename );
	LeaveCriticalSec( &l.cs );
	return frame;
}

PSI_CONTROL LoadXMLFrameOverEx( PSI_CONTROL parent, CTEXTSTR file DBG_PASS )
{
	return LoadXMLFrameOverExx( parent, file, TRUE DBG_RELAY );
}


PSI_CONTROL LoadXMLFrameEx( CTEXTSTR file DBG_PASS )
//PSI_CONTROL  LoadXMLFrame( char *file )
{
   return LoadXMLFrameOverEx( NULL, file DBG_RELAY );
}
PSI_XML_NAMESPACE_END



