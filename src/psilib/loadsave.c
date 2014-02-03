#include "controlstruc.h"
#include <sharemem.h>
#include <configscript.h>
#include <procreg.h>

// VERSION 1
//   - initial cut of the software
// VERSION 2
//   - Adds the name of the control
//     as well as the numeric type.
#define CURRENT_VERSION 1

#ifdef __cplusplus
using namespace sack::containers::link_stack;
#endif
PSI_NAMESPACE

//---------------------------------------------------------------------------

TEXTCHAR *GetBorderTypeString( _32 BorderType )
{
	static TEXTCHAR string[256];
   snprintf( string, sizeof( string ), WIDE("0x%")_32fX WIDE(""), BorderType );
   return string;
}

//---------------------------------------------------------------------------

#if 0
static void SaveCommon( PSI_CONTROL pc, PVARTEXT out )
{
	char id[32];
	snprintf( id, sizeof( id ), PSI_ROOT_REGISTRY WIDE("/control/%d"), pc->nType );
	vtprintf( out, WIDE("\'%s\'"), GetRegisteredValue( id, WIDE("Type") ) );

	vtprintf( out, WIDE(" %d "), pc->nID );
	vtprintf( out, WIDE("(%d,%d)[%d,%d]")
						 , pc->rect.x
						 , pc->rect.y
						 , pc->rect.width
						 , pc->rect.height
						 );
	//vtprintf( out, WIDE(" %d "), /*GetBorderTypeString*/( pc->BorderType ) );
	vtprintf( out, WIDE("\'%s\'"), pc->caption );
}
#endif

//---------------------------------------------------------------------------

#if 0
static int SaveControlFile( PCONTROL pc, PVARTEXT out, int level )
{
	// + 1 includes the null - +3 & makes it integral number of dwords...
	char id[32];
	snprintf( id, sizeof( id ), PSI_ROOT_REGISTRY WIDE("/control/%d/rtti"), pc->nType );
	{
		int n;
		for( n = 0; n < level; n++ )
         vtprintf( out, WIDE("\t") );
	}
	vtprintf( out, WIDE("CONTROL ") );
	SaveCommon( (PSI_CONTROL)pc, out );
	{
      void (CPROC *Save)(PCONTROL,PVARTEXT);
		if( Save=GetRegisteredProcedure( id, void,Save,(PCONTROL,PVARTEXT)) )
         Save( pc, out );
	}
   vtprintf( out, WIDE("\n") );
	return TRUE;
}
#endif
//---------------------------------------------------------------------------

#if 0
static int SaveFrameFile( PSI_CONTROL pc, PVARTEXT out, int level )
{
	return 0;
#if 0
	char id[32];
	snprintf( id, sizeof( id ), PSI_ROOT_REGISTRY WIDE("/control/%d"), pc->nType );
	{
		int n;
		for( n = 0; n < level; n++ )
         vtprintf( out, WIDE("\t") );
	}
	vtprintf( out, WIDE("GROUP ") );
   SaveCommon( pc, out );
   vtprintf( out, WIDE("\n") );
	{
		PCONTROL pControl;
		for( pControl = pc->child; pControl; pControl = pControl->next )
		{
			if( pControl->child )
			{
				SaveFrameFile( pControl, out, level+1 );
			}
			else
			{
				SaveControlFile( pControl, out, level+1 );
			}
		}
	}
	vtprintf( out, WIDE("GROUP END ") );
	// for now I think this should be associated at end of grouping...
	{
      void (CPROC *Save)(PCONTROL,PVARTEXT);
		if( Save=GetRegisteredProcedure( id, void,Save,(PCONTROL,PVARTEXT)) )
			Save( (PSI_CONTROL)pc, out );
	}
   vtprintf( out, WIDE("\n") );
	return 1;
#endif
}
#endif

//---------------------------------------------------------------------------

PSI_PROC( int, SaveFrame )( PSI_CONTROL pFrame, CTEXTSTR file )
{
	return SaveXMLFrame( pFrame, file );
   /*
	FILE *out = fopen( file, WIDE("wb") );
	if( out )
	{
		PVARTEXT pvt = VarTextCreate();
		PTEXT p;
		SaveFrameFile( pFrame, pvt, 0 );
		p = VarTextGet( pvt );
		fwrite( GetText( p ), 1, GetTextSize( p ), out );
		fclose( out );
      VarTextDestroy( &pvt );
	}
	else
	{
		Log( WIDE("Failed to open output file to save frame!") );
		return FALSE;
	}
	return TRUE;
   */
}

//---------------------------------------------------------------------------

#if 0
static int LoadControlFile( PSI_CONTROL pFrame, FILE *in )
{
    // okay type of control already read.... "CTRL" magic already consumed...
   return TRUE;
}
#endif

//---------------------------------------------------------------------------
#if 0
static int DecodeControlInfo( POINTER rawinfo, _32 size, PSI_CONTROL pFrame )
{
    struct info_tag{
      _32 nType;
        _32 len;
      _32 nID;
        IMAGE_RECTANGLE rect;
        _32 attr;
        int BorderType;
      int extra;
        char caption[];
	 } *info = (struct info_tag *)rawinfo;

	 PCONTROL pc;
    int (CPROC *ControlInit)(PTRSZVAL,PCONTROL,_32);
    Log5( WIDE("Decoding control: %s %d %d %d %d"),info->caption
                                       , info->rect.x, info->rect.y
		  , info->rect.width, info->rect.height );
	 {
		 char procclass[64];
       sprintf( procclass, PSI_ROOT_REGISTRY WIDE("/control/%d"), info->nType );
		 ControlInit = GetRegisteredProcedure( procclass, int, Init, (PTRSZVAL,PCONTROL,_32) );
		 pc = RestoreControl( pFrame, info->rect.x, info->rect.y
							  , info->rect.width, info->rect.height
							  , info->nID
							  , (PTRSZVAL)((info + 1) + info->len) );
		 if( info->len )
		 {
			 lprintf( WIDE("Setting caption for control: %s"), info->caption );
			 SetCommonText( pc, info->caption );
		 }
   }
    // and now the control has been initialize, and is happy with its current
   // default state... now - to let the application put in its two cents...
    if( (sizeof( *info ) + info->len) != size )
    {
        Log2( WIDE("Unused bytes are approx %d for control type %d")
            , size - sizeof( *info ) + info->len
            , info->nType );
    }

   return size; // assume that the whole size of data was used...
}
#endif
static int DecodeFrameInfo( POINTER rawinfo, _32 size, PSI_CONTROL *pFrameResult, PSI_CONTROL hAbove, FrameInitProc InitProc, PTRSZVAL psv )
{
	struct info_tag{
		_32 Version;
		_32 nType;
		_32 len;
		_32 nID;
		IMAGE_RECTANGLE rect;
		int BorderType;
		TEXTCHAR caption[];
						/* caption + len == extra info for frame (if any)*/
						/* then after that info should be a CTRL thing, or a FRAM thing
						 which would be a child control within this frame... */
	} *info = (struct info_tag *)rawinfo;
	PTRSZVAL Begin, Current;
	if( !pFrameResult )
		return 0;
	Log5( WIDE("Decoding frame: %s %")_32fs WIDE(" %")_32fs WIDE(" %")_32f WIDE(" %")_32f WIDE("")
		 ,info->caption
		 , info->rect.x, info->rect.y
		 , info->rect.width, info->rect.height );
	(*pFrameResult) = CreateFrame( info->caption
										  , info->rect.x, info->rect.y
										  , info->rect.width, info->rect.height
										  , info->BorderType, hAbove );
						 // allow the application to setup things like the init routine
						 // for this dialog... which may or may not be the same as the
						 // InitProc which has been passed in...
	if( InitProc )
		InitProc( psv, *pFrameResult, info->nID );
	Begin = (PTRSZVAL)info;
	Current = (PTRSZVAL)(info+1) + info->len;
			  // the frame itself shouldn't have any other info ...
			  // but may - and here would be the place to put that init...(?)

	Current += 0; // plus any Frame private info which may be here...

	while( ( Current - Begin ) < size )
	{
		_32 magic = *(_32*)Current;
		_32 nextsize;
		Current += 4;
		if( magic == *(_32*)"FRAM" )
		{
			PSI_CONTROL pFrameResultBuffer;
			nextsize = *(_32*)Current;
			Current += 4;
			Current += DecodeFrameInfo( (POINTER)Current, nextsize, &pFrameResultBuffer, *pFrameResult, InitProc, psv );
		}
		else if( magic == *(_32*)"CTRL" )
		{
			nextsize = *(_32*)Current;
			Current += 4;
#if 0
			Current += DecodeControlInfo( (POINTER)Current, nextsize, *pFrameResult );
#endif
		}
		else
		{
			Log1( WIDE("Data to decode is broken - %4.4s is not FRAM or CTRL"), (char*)&magic );
			return 0;
		}
	}
	return Current - Begin;
}

//---------------------------------------------------------------------------

PSI_PROC( PSI_CONTROL, LoadFrameFromMemory )( POINTER info, _32 size, PSI_CONTROL hAbove, FrameInitProc InitProc, PTRSZVAL psv  )
{
	_32 magic;
	_32 this_size;
	P_8 buffer = (P_8)info; // index in bytes...
	magic = *(_32*)buffer;
	this_size = *(_32*)(buffer + 4);
	if( magic == *(_32*)"FRAM" )
	{
		PSI_CONTROL pFrame;
		if( DecodeFrameInfo( (POINTER)(buffer + 4), this_size, &pFrame, hAbove, InitProc, psv ) != this_size )
		{
			Log( WIDE("Decode length did not match legnth of data passed...") );
		}
		return pFrame;
	}
	Log1( WIDE("File has become corrupt %4.4s is not FRAM"), (char*)&magic );
	return NULL;
}

//---------------------------------------------------------------------------
#if 0
// please use LoadXMLFrame();
PSI_PROC( PSI_CONTROL, LoadFrameFromFile )( FILE *in, PSI_CONTROL hAbove, FrameInitProc InitProc, PTRSZVAL psv  )
{
	_32 magic;
	_32 size;
	PSI_CONTROL pFrame;
	POINTER buffer;
	while( fread( &magic, 1, sizeof( magic ), in ) )
	{
		if( magic == *(_32*)"FRAM" )
		{
			fread( &size, 1, sizeof( size ), in );
			buffer = Allocate( size );
			fread( buffer, 1, size, in );
			DecodeFrameInfo( buffer, size, &pFrame, hAbove, InitProc, psv );
			return pFrame;
		}
		else
		{
			Log1( WIDE("File has become corrupt %4.4s is not FRAM"), (char*)&magic );
		}
	}
	return pFrame;
}
#endif

//---------------------------------------------------------------------------
PLINKSTACK readstack;
//---------------------------------------------------------------------------

PSI_PROC( PSI_CONTROL, CreateCommonExxx)( PSI_CONTROL pContainer
											  , CTEXTSTR pTypeName
											  , _32 nType
											  , int x, int y
											  , int w, int h
													 , _32 nID
                                         , CTEXTSTR pIDName
											  , CTEXTSTR caption
													, _32 ExtraBorderType
                                        , PTEXT parameters
													, POINTER extra_param
												DBG_PASS );

PTRSZVAL CPROC GroupRead( PTRSZVAL psv, arg_list args )
{
	PARAM( args, TEXTCHAR *, pTypeName );
	PARAM( args, S_64, ID );
	PARAM( args, S_64, x );
	PARAM( args, S_64, y );
	PARAM( args, S_64, width );
	PARAM( args, S_64, height );
	PARAM( args, TEXTCHAR *, caption );
	PARAM( args, TEXTCHAR *, more );
	PSI_CONTROL pc;
	pc = CreateCommonExxx( (PSI_CONTROL)PeekLink( &readstack )
							  , pTypeName, 0
							  , (int)x, (int)y, (int)width, (int)height, (int)ID, NULL
							  , caption
							  , 0 // border extra
							  , SegCreateFromText( more )
							  , NULL
								DBG_SRC
							  );
   PushLink( &readstack, pc );
   lprintf( WIDE("Have to deal with %s"), more );
   return (PTRSZVAL)pc;
}

//---------------------------------------------------------------------------
PTRSZVAL CPROC GroupEnd( PTRSZVAL psv, arg_list args )
{
#ifdef __cplusplus
	::containers::link_stack::
#endif

	PopLink( &readstack );
   return 0;
}

//---------------------------------------------------------------------------

PTRSZVAL CPROC ControlRead( PTRSZVAL psv, arg_list args )
{
	PARAM( args, TEXTCHAR *, pTypeName );
	PARAM( args, S_64, ID );
   PARAM( args, S_64, x );
   PARAM( args, S_64, y );
   PARAM( args, S_64, width );
	PARAM( args, S_64, height );
   PARAM( args, TEXTCHAR *, caption );
   PARAM( args, TEXTCHAR *, more );
	PSI_CONTROL pc;
	pc = CreateCommonExxx( (PSI_CONTROL)PeekLink( &readstack )
						, pTypeName, 0
							  , (int)x, (int)y, (int)width, (int)height, (int)ID, NULL
                        , caption, 0
						, SegCreateFromText( more )
						, NULL
						 DBG_SRC
						);
   lprintf( WIDE("Have to deal with %s"), more );
   return psv;
}

PSI_PROC( PSI_CONTROL, LoadFrame )( CTEXTSTR file, PSI_CONTROL hAbove, FrameInitProc InitProc, PTRSZVAL psv )
{
	PCONFIG_HANDLER pch = CreateConfigurationHandler();
   PSI_CONTROL frame;
	AddConfigurationMethod( pch, WIDE("GROUP \'%m\' %i (%i,%i)[%i,%i] \'%m\' %m")
								 , GroupRead );
	AddConfigurationMethod( pch, WIDE("GROUP END")
								 , GroupEnd );
   AddConfigurationMethod( pch, WIDE("CONTROL  \'%m\' %i (%i,%i)[%i,%i] \'%m\' %m"), ControlRead );
	ProcessConfigurationFile( pch, file, (PTRSZVAL)&frame );
   DestroyConfigurationHandler( pch );
	//return pFrame;
   return NULL;
}

PSI_NAMESPACE_END

