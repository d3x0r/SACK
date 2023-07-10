#define NO_UNICODE_C
#include <stdhdrs.h>
#include <stdio.h>
#include <errno.h>
#include <filemon.h>
#define DO_LOGGING
#include <logging.h>
#define DEFINES_DEKWARE_INTERFACE
#define PLUGIN_MODULE
#include "plugin.h"

static int myTypeID; // supplied for uhmm... grins...
static int myTypeID2;

typedef struct mydatapath_tag {
	DATAPATH common;
	FILE *handle;
	struct {
		 BIT_FIELD bFirst:1;
		 BIT_FIELD bPriorEndLine:1;
		 BIT_FIELD bCloseAtEnd:1;
		 BIT_FIELD bRead : 1;
		 BIT_FIELD bRelay : 1;
		 BIT_FIELD bWrite : 1;
		 BIT_FIELD bUnicode : 1;
		 BIT_FIELD bUnicode8 : 1;
	} flags;
} MYDATAPATH, *PMYDATAPATH;

static PTEXT get_line(PMYDATAPATH pdp, FILE *source)
{
	#define WORKSPACE 256  // character for workspace
	char *filebuffer;
	wchar_t *wfilebuffer;
	TEXTCHAR *text;
	PTEXT workline=(PTEXT)NULL,pNew;
	size_t length = 0;
	if( !source ) {
		lprintf( "no file source to read from" );
		return NULL;
	}
	if( pdp->flags.bUnicode )
		wfilebuffer = NewArray( wchar_t, WORKSPACE );
	else
		filebuffer = NewArray( char, WORKSPACE );
	do
	{
		void *result;
		if( pdp->flags.bUnicode )
		{
			result = fgetws( wfilebuffer, WORKSPACE, source );
			if( result )
			{
				pNew = SegCreateFromWide( wfilebuffer );
			}
			else
				pNew = SegCreate( 0 );
		}
		else
		{
			result = sack_fgets( filebuffer, WORKSPACE, source );
			//lprintf( "File Read Line:%p (%s) %s", result, GetText(pdp->common.pName), filebuffer );
			if( result )
			{
				pNew = SegCreateFromChar( filebuffer );
			}
			else
				pNew = SegCreate( 0 );
		}
		// create a workspace to read input from the file.
		workline=SegAppend(workline, pNew);
		workline = pNew;
		text = GetText( workline );
		// SetEnd( workline );
		//Log1( "Doing get string at %ld", ftell( source ) );
		// read a line of input from the file.

		if( !result ) // if no input read.
		{
			//Log( "File ended!!!!!!!!!!!!!!!!!!!!!!!" );
			if (PRIORLINE(workline)) // if we've read some.
			{
				PTEXT t;
				workline=PRIORLINE(workline); // go back one.
				SegBreak(t = NEXTLINE(workline));
				LineRelease(t);  // destroy the current segment.
			}
			else
			{
				LineRelease(workline);				// destory only segment.
				workline = NULL;
			}
			break;  // get out of the loop- there is no more to read.
		}

		length = StrLen(text);  // get the length of the line.
		//Log2( "Read: %s(%d)", workline, length );
		//LogBinary( text, length );
		if( workline )
			workline->data.size = length;
	}
	while (text[length-1]!='\n' && text[length-1] != '\r' ); //while not at the end of the line.

	// clean up our temp buffer
	if( pdp->flags.bUnicode )
		Deallocate( wchar_t *, wfilebuffer );
	else
		Deallocate( char *, filebuffer );

	if( workline && (text[length-1]=='\n'||text[length-1]=='\r' ) )
		pdp->flags.bPriorEndLine = 1;
	else
		pdp->flags.bPriorEndLine = 0;

	if (workline)  // if I got a line, and there was some length to it.
		SetStart(workline);	// set workline to the beginning.
	return(workline);		// return the line read from the file.
}



static int CPROC Read( PDATAPATH pdpX )
{
	PMYDATAPATH pdp = (PMYDATAPATH)pdpX;
	PTEXT pLine;
	if( pdp->flags.bRelay || !pdp->flags.bRead )
	{
		return RelayInput( (PDATAPATH)pdp, NULL );
	}
	if( pdp->flags.bRead && !pdp->common.flags.Closed )
	{
		if( ( pLine = get_line( pdp, (FILE*)pdp->handle ) ) )
		{	
			if( !pdp->flags.bFirst && !pdp->flags.bPriorEndLine )
				pLine->flags |= TF_NORETURN;
			//{
			//	PTEXT tmp;
			//	tmp = BuildLine( pLine );
			//	Log1( "Read is: %s", GetText( tmp ) );
			//	LineRelease( tmp );
			//}
			EnqueLink( &pdp->common.Input, pLine );
			pdp->flags.bFirst = 0;
			return 1; // one block read.
		}
		else
		{
			if( pdp->flags.bCloseAtEnd )
				pdp->common.flags.Closed = 1;
		}
	}
	return 0;
}

static int CPROC Write( PDATAPATH pdpX )
{
	PMYDATAPATH pdp = (PMYDATAPATH)pdpX;
	if( !pdp->flags.bWrite )
	{
		return RelayOutput( (PDATAPATH)pdp, NULL );
	}
	else
	{
		PTEXT pLine, pOut, pSaveOut;
		int wrote = 0;
		while( ( pLine = (PTEXT)DequeLink( &pdp->common.Output ) ) )
		{
			PTEXT temp = NULL;
		  if( !( pLine->flags & TF_NORETURN ) )
		  {
				pLine = SegAppend( temp = SegCreate(0), pLine );
		  }
		  pSaveOut = pOut = BuildLine( pLine );
		  while( pOut )
		  {
				wrote++;
				if( GetTextSize( pOut ) == 0 )
					sack_fwrite( "\n", 1, 1, pdp->handle );
				else
					sack_fwrite( GetText( pOut ), GetTextSize( pOut ), 1, pdp->handle );
				pOut = NEXTLINE( pOut );
		  }
		  LineRelease( pSaveOut );
		  if( pdp->common.pPrior )
		  {
			  if( temp )
			  {
				  pLine = NEXTLINE( pLine );
				  LineRelease( SegGrab( temp ) );
			  }
			  EnqueLink( &pdp->common.pPrior->Output, pLine );
		  }
		  else
			  LineRelease( pLine );
		}
		if( pdp->common.pPrior &&
			 pdp->common.pPrior->Write )
			wrote += pdp->common.pPrior->Write( pdp->common.pPrior );
		if( wrote )
			fflush( pdp->handle );
		return wrote;
	}
}

static int CPROC Close( PDATAPATH pdpX )
{
	PMYDATAPATH pdp = (PMYDATAPATH)pdpX;
	if( pdp->flags.bRead && !pdp->flags.bWrite )
	{
		RelayOutput( (PDATAPATH)pdp, NULL );
	}
	if( !pdp->flags.bRead && pdp->flags.bWrite)
	{
		RelayInput( (PDATAPATH)pdp, NULL );
	}
	if( pdp->handle )
		sack_fclose( pdp->handle );
	pdp->common.Type = 0;
	return 0;
}

static int CPROC Seek( PDATAPATH pDataPath, PSENTIENT ps, PTEXT parameters )
{
	PMYDATAPATH pdp = (PMYDATAPATH)pDataPath;
	PTEXT op;
	op = GetParam( ps, &parameters );
	if( op )
	{
		if( TextLike( op, "start" ) )
		{
			sack_fseek( pdp->handle, 0, SEEK_SET );
		}
		else if( TextLike( op, "end" ) )
		{
			sack_fseek( pdp->handle, 0, SEEK_END );
		}
		else if( GetText( op )[0] == '+' )
		{
			uint32_t pos = (uint32_t)IntNumber( op );
			sack_fseek( pdp->handle, pos, SEEK_CUR );
		}
		else if( GetText( op )[0] == '-' )
		{
			uint32_t pos = (uint32_t)IntNumber( op );
			sack_fseek( pdp->handle, pos, SEEK_CUR );
		}
		else if( IsNumber( op ) )
		{
			uint32_t pos = (uint32_t)IntNumber( op );
			sack_fseek( pdp->handle, pos, SEEK_SET );
		}
		else
		{
			DECLTEXT( msg, "Seek parameter must be (start/end/+#/-#/#)" );
			if( !ps->CurrentMacro )
				EnqueLink( &ps->Command->Output, &msg );
		}
	}
	else
	{
		DECLTEXT( msg, "Seek parameter must be (start/end/+#/-#/#)" );
		if( !ps->CurrentMacro )
			EnqueLink( &ps->Command->Output, &msg );
	}
	return 0;
}

int CPROC SetClose( PDATAPATH pdp, PSENTIENT ps, PTEXT parameters )
{
	PMYDATAPATH pmdp = (PMYDATAPATH)pdp;
	pmdp->flags.bCloseAtEnd = 1;
	return 0;
}

int CPROC SetRelay( PDATAPATH pdp, PSENTIENT ps, PTEXT parameters )
{
	PMYDATAPATH pmdp = (PMYDATAPATH)pdp;
	pmdp->flags.bRelay = 1;
	return 0;
}

int CPROC SetFollow( PDATAPATH pdp, PSENTIENT ps, PTEXT parameters )
{
	PMYDATAPATH pmdp = (PMYDATAPATH)pdp;
	pmdp->flags.bCloseAtEnd = 0;
	return 0;
}

//---------------------------------------------------------------------------

static void ClearCommandHold( struct sentient_tag *ps// sentient processing macro
						 , struct macro_state_tag *pms ) // saves extra peek...
{
	//Log( "Clearing commands for macro..." );
	ps->flags.bHoldCommands = FALSE;
	pms->pMacro->flags.un.macro.bUsed = FALSE;
	PopData( &ps->MacroStack );
}

//---------------------------------------------------------------------------

static PDATAPATH CPROC Open( PDATAPATH *pChannel, PSENTIENT ps, PTEXT parameters )
{
	PMYDATAPATH pdp = NULL;
	PTEXT pFile;
	struct {
		uint32_t read : 1;
		uint32_t write : 1;
	} flags;
	PTEXT saveparams=parameters, temp;
	flags.read = 0;
	flags.write = 0;
	while( saveparams=parameters,
			temp = GetParam( ps, &parameters ) )
	{
		if( TextLike( temp, "__input" ) || TextLike( temp, "__read" ) )
		{
			flags.read = 1;
		}
		else if( TextLike( temp, "__output" ) || TextLike( temp, "__write" ) )
		{
			flags.write = 1;
		}
		else
		{
			// otherwise here begins the filename...
			//Log1( "Invalid parameter to open file...(%s)", GetText( temp ) );
			break;
		}
	}
	//lprintf( "Open a file device...%s", GetText(parameters) );
	if( !flags.read && !flags.write )
		flags.read = 1; // read only default if nothing optioned
	parameters = saveparams;
	pFile = GetFileName( ps, &parameters );
	if( pFile )
	{
		pdp = CreateDataPath( pChannel, MYDATAPATH );
		Log1( "Opening a file: %s", GetText( pFile ) );
		if( flags.read && !flags.write )
			pdp->handle = sack_fopen( 0, GetText( pFile ), "rb" );
		else if( flags.read && flags.write )
			pdp->handle = sack_fopen( 0, GetText( pFile ), "ab+" );
		else if( flags.write && !flags.read )
			pdp->handle = sack_fopen( 0, GetText( pFile ), "wb" );
		else  // not write and not read
		{
			Log( "File opened without read or write - aborting." );
			DestroyDataPath( (PDATAPATH)pdp );
			return NULL;
		}

		if( !pdp->handle )
		{
			lprintf( "Failed: %d", errno );
			DestroyDataPath( (PDATAPATH)pdp );
			return NULL;
		}
		{
			char charbuf[64];
			int len_read; // could be really short
			int char_check;
			int ascii_unicode = 
#if UNICODE
				1
#else
				0
#endif
				;
			len_read = (int)sack_fread( charbuf, 1, 64, pdp->handle );
			if( len_read )
			{
				if( ( ((uint16_t*)charbuf)[0] == 0xFEFF )
					|| ( ((uint16_t*)charbuf)[0] == 0xFFFE )
					|| ( ((uint16_t*)charbuf)[0] == 0xFDEF ) )
					pdp->flags.bUnicode = 1;
				else if( ( charbuf[0] == 0xef ) && ( charbuf[1] == 0xbb ) && ( charbuf[0] == 0xbf ) )
				{
					pdp->flags.bUnicode8 = 1;
				}
				for( char_check = 0; char_check < len_read; char_check++ )
				{
					// every other byte is a 0 for flat unicode text...
					if( ( char_check & 1 ) && ( charbuf[char_check] != 0 ) )
					{
						ascii_unicode = 0;
						break;
					}
				}
				if( ascii_unicode )
				{
					pdp->flags.bUnicode = 1;
				}
				else
				{
					int ascii = 1;
					for( char_check = 0; char_check < len_read; char_check++ )
						if( charbuf[char_check] & 0x80 )
						{
							ascii = 0;
							break;
						}
					if( ascii )
					{
						// hmm this is probably a binary thing?
					}
				}
			}
			else
			{
#if UNICODE
				wchar_t typechar = 0xFEFF;
				sack_fseek( pdp->handle, 0, SEEK_SET );
				sack_fwrite( &typechar, 1, 2, pdp->handle );
				sack_fseek( pdp->handle, 0, SEEK_CUR );
#else
				sack_fseek( pdp->handle, 0, SEEK_SET );
#endif
			}
		}
		if( !flags.write )
		{
			sack_fseek( pdp->handle, 0, SEEK_SET );
		}
		//Log2( "Result: %p %d", pdp->handle, errno );
		LineRelease( pFile );
		if( !pdp->handle )
		{
			pdp->common.Type = 0; // abort connection on return...
		}
		else
		{
			//lprintf( "Setting typeID to %d %d", myTypeID, pdp->common.Type );
			//pdp->common.Type = myTypeID;
			SetDatapathType( &pdp->common, myTypeID );
			//pdp->common.Option = (int(*)(PDATAPATH,PSENTIENT,PTEXT))Options;
			pdp->common.Read = Read;
			pdp->common.Write = Write;
			pdp->common.Close = Close;
			pdp->common.flags.Data_Source = 1;
			pdp->common.flags.Dont_Relay_Prior = 1;
			pdp->flags.bFirst = 1;
			pdp->flags.bRead = flags.read;
			pdp->flags.bWrite = flags.write;
			if( &ps->Command == pChannel )
			{
				PMACROSTATE pms = (PMACROSTATE)PeekData( &ps->MacroStack );
				if( pms )
				{
					pms->MacroEnd = ClearCommandHold;
					ps->flags.bHoldCommands = TRUE;
				}
			}
		}
	}
	return (PDATAPATH)pdp;
}

static int CPROC LoadFile( PSENTIENT ps, PTEXT parameters )
{
	PTEXT pVarName;
	PTEXT pFileName;
	PTEXT pVarValue;
	FILE *file;
	int size;
	// First parameter is a variable NAME
	// second parameter is a file name...
	pVarName = GetParam( ps, &parameters );
	if( !pVarName )
	{
		DECLTEXT( msg, "Must supply variable name as first parameter..." );
		if( !ps->CurrentMacro )
			EnqueLink( &ps->Command->Output, &msg );
		return 0;
	}
	pFileName = GetFileName( ps, &parameters );
	if( !pFileName )
	{
		DECLTEXT( msg, "Must supply file name to load as second parameter..." );
		if( !ps->CurrentMacro )
			EnqueLink( &ps->Command->Output, &msg );
		return 0;
	}
	
	file = sack_fopen( 0, GetText( pFileName ), "rb" );
	if( !file )
	{
		DECLTEXT( msg, "Could not open the file specified..." );
		if( !ps->CurrentMacro )
			EnqueLink( &ps->Command->Output, &msg );
		return 0;
	}
	sack_fseek( file, 0, SEEK_END );
	size = (int)sack_ftell( file );
	sack_fseek( file, 0, SEEK_SET );
	pVarValue = SegCreate( size );
	pVarValue->flags |= TF_BINARY;
	if( (int)sack_fread( GetText( pVarValue ), 1, size, file ) != size)
	{
		DECLTEXT( msg, "Failed to read full file..." );
		if( !ps->CurrentMacro )
			EnqueLink( &ps->Command->Output, &msg );
	}
	if( ps->CurrentMacro )
		ps->CurrentMacro->state.flags.bSuccess = 1;
	sack_fclose( file );
	// storing binary variables overwrites what is there, and does NOT
	// reallocate space for variable.
	AddBinary( ps, ps->Current, pVarName, pVarValue );
// LineRelease( pVarValue );
	return 1;
}

static int CPROC StoreFile( PSENTIENT ps, PTEXT parameters )
{
	PTEXT pVarName;
	PTEXT pFileName, pSave;
	FILE *file;
	// First parameter is a variable NAME
	// second parameter is a file name...
	pSave = parameters;
	pVarName = GetParam( ps, &parameters );
	if( !pVarName || pSave == pVarName )
	{
		DECLTEXT( msg, "Must use variable as first parameter..." );
		EnqueLink( &ps->Command->Output, &msg );
		return 0;
	}
	if( !(GetTextFlags(pVarName) & TF_BINARY ) )
	{
		DECLTEXT( msg, "Can only store binary variables as file..." );
		EnqueLink( &ps->Command->Output, &msg );
		return 0;
	}
	pFileName = GetFileName( ps, &parameters );
	if( !pFileName )
	{
		DECLTEXT( msg, "Must supply file name to store as second parameter..." );
		EnqueLink( &ps->Command->Output, &msg );
		return 0;
	}

	file = sack_fopen( 0, GetText( pFileName ), "wb" );
	if( !file )
	{
		DECLTEXT( msg, "Could not create the file specified..." );
		EnqueLink( &ps->Command->Output, &msg );
		return 0;
	}

	sack_fwrite( GetText( pVarName ), GetTextSize( pVarName ), 1, file );

	sack_fclose( file );
	return 1;
}

//--------------------------------------------------------------------------

static PTEXT CPROC WriteLineInput( PDATAPATH pdp, PTEXT pLine )
{
	FILE *f = ((PMYDATAPATH)pdp)->handle;
	PTEXT pWrite, p;
	if( pLine )
	{
		p = pWrite = BuildLine( pLine );
		if( !(pLine->flags & TF_NORETURN ) )
		{  	
			sack_fwrite( "\r\n", 1, 2, f );
		}
		while( p )
		{
			sack_fwrite( GetText( p )
					, GetTextSize( p )
					, 1
					, f );
			p = NEXTLINE( p );
		}
		sack_fflush( f );
		LineRelease( pWrite );
	}
	return pLine;
}

//--------------------------------------------------------------------------

static PTEXT CPROC WriteLineOutput( PDATAPATH pdp, PTEXT pLine )
{
	FILE *f = ((PMYDATAPATH)pdp)->handle;
	PTEXT pWrite, p;
	if( pLine )
	{
		p = pWrite = BuildLine( pLine );
		if( !(pLine->flags & TF_NORETURN ) )
		{
			sack_fwrite( "\r\n", 1, 2, f );
		}
		while( p )
		{
			//if( !(p->flags & TF_FORMAT ) )
			sack_fwrite( GetText( p )
					, GetTextSize( p )
					, 1
					, f );
			p = NEXTLINE( p );
		}
		sack_fflush( f );
		LineRelease( pWrite );
	}
	return pLine;
}

//--------------------------------------------------------------------------

static int CPROC WriteLog( PDATAPATH pdp )
{
	RelayOutput( pdp, WriteLineOutput );
	return 0;
}

//--------------------------------------------------------------------------

static int CPROC ReadLog( PDATAPATH pdp )
{
	RelayInput( pdp, WriteLineInput );
	return 0;
}

//--------------------------------------------------------------------------

static int CPROC CloseLogPath( PDATAPATH pdp )
{
	FILE *f = ((PMYDATAPATH)pdp)->handle;
	if( f )
		sack_fclose( f );
	((PMYDATAPATH)pdp)->handle = NULL;
	return 0;
}

//--------------------------------------------------------------------------

static int CPROC CloseLogDefault( PDATAPATH pdp )
{
	FILE *f = ((PMYDATAPATH)pdp)->handle;
	DestroyDataPath( pdp->pPrior );
	if( f )
		sack_fclose( f );
	((PMYDATAPATH)pdp)->handle = NULL;
	return 0;
}

//--------------------------------------------------------------------------

static int CPROC OpenLog( PSENTIENT ps, PTEXT parameters )
{
	PTEXT pFileName;
	PMYDATAPATH pmdp;
	FILE *file;
	//if( !ps->Data )
	//{
	//	DECLTEXT( msg, "Data datapath is not open - cannot log." );
	//	EnqueLink( &ps->Command->Output, &msg );
	//}

	pFileName = GetFileName( ps, &parameters );
	file = sack_fopen( 0, GetText( pFileName ), "a+b" );
	if( file )
	{
		DECLTEXT( name, "$Log" );
		//pmdp = CreateDataPath( &ps->Data, MYDATAPATH );
		pmdp = CreateDataPath( &ps->Command, MYDATAPATH );
		pmdp->common.Type = myTypeID2;
		pmdp->common.Read  = ReadLog;
		pmdp->common.Write = WriteLog;
		pmdp->common.Close = CloseLogDefault;
		pmdp->common.pName = (PTEXT)&name;
		pmdp->handle = file;
	}
	return 0;
}

//--------------------------------------------------------------------------

static int CPROC CloseLog( PSENTIENT ps, PTEXT parameters )
{
	PMYDATAPATH pmdp;
	
	pmdp = (PMYDATAPATH)FindDatapath( ps, myTypeID2 );
	if( pmdp )
	{
		pmdp->common.Close = CloseLogPath;
		DestroyDataPath( (PDATAPATH)pmdp );
	}
	else
	{
		DECLTEXT( msg, "Logging is not active on the datapath." );
		EnqueLink( &ps->Command->Output, &msg );
		return 0;
	}
	return 0;
}

int CPROC FileChanged( uintptr_t psvSent
							, CTEXTSTR file
							, uint64_t size
							, uint64_t time
							, LOGICAL bCreated
							, LOGICAL bDirectory
							, LOGICAL bDeleted )
{
	DECLTEXT( pathtype, "PATH" );
	DECLTEXT( filetype, "FILE" );
	PTEXT args = SegAppend( SegCreateFromText( file ), bDirectory?SegCreateIndirect( (PTEXT)&pathtype ):SegCreateIndirect( (PTEXT)&filetype ) );
	PSENTIENT ps = (PSENTIENT)psvSent;
	PMACROSTATE pms;
	if( file )
	{
		if( bCreated )
		{
			pms = InvokeBehavior( "create", ps->Current, ps, args );
		}
		// a file might just be created and deleted?
		// so pass this on down...
		else if( bDeleted )
		{
			pms = InvokeBehavior( "delete", ps->Current, ps, args );
		}
		// then an update is only relavent if neither create nor destroy happened
		if( !bCreated && !bDeleted )
		{
			pms = InvokeBehavior( "update", ps->Current, ps, args );
		}

		if( pms )
		{
			PMACROSTATE oldpms = ps->CurrentMacro;
			ps->CurrentMacro = pms;
			AddVariable( ps, ps->Current, SegCreateFromText( "argname_stuff" ), SegCreateFromText( "args" ) );
			ps->CurrentMacro = oldpms;
		}
	}
	return TRUE;
}

int CPROC CreateFileMonitor( PSENTIENT ps, PENTITY pe, PTEXT parameters )
{
	PSENTIENT psNew = NULL;
	PTEXT pFile = GetParam( ps, &parameters );
	if( pFile )
	{
#ifndef __ANDROID__
		PMONITOR pMonitor;
		if( ( pMonitor = MonitorFiles( GetText( pFile ), 100 ) ) )
		{
			psNew = pe->pControlledBy;
			if( !psNew )
				psNew = CreateAwareness( pe );
			AddExtendedFileChangeCallback( pMonitor, NULL, FileChanged, (uintptr_t)psNew );
			// should figure out a way to define what the parameters
			// received by the behavior method are...
			// or maybe behaviors are parameterless and rely on having
			// internal variables or volatile variables set.
			AddBehavior( pe, "create", "A file has been created %1 is file; %2 is <PATH/FILE>"/*, "name type"*/ );
			AddBehavior( pe, "delete", "A file has been deleted %1 is file; %2 is <PATH/FILE>" );
			AddBehavior( pe, "update", "A file has changed %1 is file; %2 is <PATH/FILE>" );
			UnlockAwareness( psNew );
		}
#endif
	}
	return 0;
}

//PDATAPATH Open( PDATAPATH *pChannel, PSENTIENT ps, PTEXT parameters );
int CPROC OpenLog( PSENTIENT ps, PTEXT parameters );
int CPROC CloseLog( PSENTIENT ps, PTEXT parameters );
int CPROC LoadFile( PSENTIENT ps, PTEXT parameters );
int CPROC StoreFile( PSENTIENT ps, PTEXT parameters );

	
extern _OptionHandler SetClose, SetFollow, SetRelay, Seek;
#define NUM_OPTIONS ( sizeof( FileOptions ) / sizeof( FileOptions[0] ))
	option_entry FileOptions[] = { { DEFTEXT( "close" ), 2, 5, DEFTEXT( "Set close on end of file(not follow)." ), SetClose }
										  , { DEFTEXT( "follow" ), 2, 6, DEFTEXT( "Set follow end of file(not close at end)." ), SetFollow }
										  , { DEFTEXT( "relay" ), 2, 5, DEFTEXT( "Override read and relay input" ), SetRelay }
										  , { DEFTEXT( "seek" ), 2, 4, DEFTEXT( "Set current file position" ), Seek }

	};

PRELOAD( RegisterRoutines ) // PUBLIC( TEXTCHAR *, RegisterRoutines )( void )
{
	if( DekwareGetCoreInterface( DekVersion ) ) {
	myTypeID = RegisterDeviceOpts( "file", "File based operations...", Open, FileOptions, NUM_OPTIONS );
	myTypeID2 = RegisterDevice( "log", "datapath logging device", NULL );
	RegisterRoutine( "IO/File", "LoadFile", "Load file as a binary variable", LoadFile );
	RegisterRoutine( "IO/File", "StoreFile", "Save File from binary variable", StoreFile );
	RegisterRoutine( "IO/File", "SaveFile", "Save File from binary variable", StoreFile );
	RegisterRoutine( "IO/File", "Log", "Begin logging the data datapath to a file", OpenLog );
	RegisterRoutine( "IO/File", "EndLog", "End Logging the data datapath to a file", CloseLog );
	RegisterObject( "file monitor", "File monitor object... provides on new, modify, deleted", CreateFileMonitor );
	//return DekVersion;
	}
}

PUBLIC( void, UnloadPlugin )( void ) // this routine is called when /unload is invoked
{
	UnregisterDevice( "file" );
	UnregisterDevice( "log" );
	UnregisterRoutine( "LoadFile" );
	UnregisterRoutine( "StoreFile" );
	UnregisterRoutine( "SaveFile" );
	UnregisterRoutine( "Log" );
	UnregisterRoutine( "Endlog" );
}


								 // $Log: file.c,v $
								 // Revision 1.36  2005/06/10 10:32:19  d3x0r
								 // Modify type of option functions....
								 //
								 // Revision 1.35  2005/05/30 11:51:32  d3x0r
								 // Remove many messages...
								 //
								 // Revision 1.34  2005/04/20 20:19:40  d3x0r
								 // Progress on file monitor extensions
								 //
								 // Revision 1.33  2005/03/02 19:58:32  d3x0r
								 // Reduce the rate at which files are scanned
								 //
								 // Revision 1.32  2005/02/24 03:10:27  d3x0r
								 // Fix behaviors to work better... now need to register terminal close and a couple other behaviors...
								 //
								 // Revision 1.31  2005/02/23 12:34:38  d3x0r
								 // Okay file monitor object almost works, have to restore more behavior workings... but This does seem to work well.
								 //
								 // Revision 1.30  2005/02/23 11:38:58  d3x0r
								 // Modifications/improvements to get MSVC to build.
								 //
								 // Revision 1.29  2005/02/22 12:28:48  d3x0r
								 // Final bit of sweeping changes to use CPROC instead of undefined proc call type
								 //
								 // Revision 1.28  2005/02/21 12:07:49  d3x0r
								 // Okay so finally we should make everything CPROC type pointers... have for a long time decided to avoid this, but for compatibility with other libraries this really needs to be done.  Increase version number to 2.4
								 //
								 // Revision 1.27  2005/01/17 09:01:14  d3x0r
								 // checkpoint ...
								 //
// Revision 1.26  2004/05/12 08:20:48  d3x0r
// Implement file ops with standard options handling
//
// Revision 1.25  2004/05/04 07:30:26  d3x0r
// Checkpoint for everything else.
//
// Revision 1.24  2004/04/05 21:15:49  d3x0r
// Sweeping update - macros, probably some logging enabled
//
// Revision 1.23  2003/10/29 00:52:09  panther
// Remove file loggings
//
// Revision 1.22  2003/10/26 12:38:16  panther
// Updates for newest scheduler
//
// Revision 1.21  2003/08/20 08:05:48  panther
// Fix for watcom build, file usage, stuff...
//
// Revision 1.20  2003/08/15 13:13:13  panther
// Define name for log device.  Remove superfluous logging
//
// Revision 1.19  2003/03/25 08:59:01  panther
// Added CVS logging
//
