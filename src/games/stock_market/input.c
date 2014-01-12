
#include <stdhdrs.h>
#include <filesys.h>
#include <stdio.h>
#ifdef _WIN32
#include <io.h>
#include <conio.h>
#else
// ?? fcntl.h
#endif
#include <fcntl.h>
#include <timers.h>
#include "input.h"
#include "global.h"
static FILE *script, *save;
static int inputpipe;
static int bEnque;
static PTHREAD pThreadInput;

void SetInputFile( TEXTCHAR *file )
{
   script = sack_fopen( 0, file, WIDE("rt") );
}

void SetOutputFile( TEXTCHAR *file )
{
   save = sack_fopen( 0, file, WIDE("wt") );
}

PTRSZVAL CPROC InputThread( PTHREAD pThread )
{
	TEXTCHAR ch[2];
	ch[1] = 0;

	while( 1 )
	{
#ifndef __LINUX__
#if _MSC_VER>=1600
		ch[0] = _getch();
#else
		ch[0] = getch();
#endif
#else
		ch[0] = fgetc(stdin);
#endif
		if( ch[0] == '\r' )
			ch[0] = '\n';
		EnqueStrokes( ch );
	}
	return 0;
}

int GetCh( void )
{
	int ch;
	//if( g.pCurrentPlayer && g.pCurrentPlayer->flags.bRemote )
	//   return 0;
	if( !pThreadInput )
		pThreadInput = MakeThread();
	if( !inputpipe )
	{
		inputpipe = open( WIDE("stockmarket.pipe"), O_RDWR|O_CREAT|O_TRUNC, 0600 );
		ThreadTo( InputThread, 0 );
	}
	if( !save )
		save = sack_fopen( 0, WIDE("stockmarket.save"), WIDE("wt") );
	if( script )
	{
		ch = fgetc( script );
		if( ch == -1 )
		{
			fclose( script );
			script = NULL;
		}
	}
	if( !script )
	{
		ch = 0; // clear upper bits.
		lprintf( WIDE("will read 1 TEXTCHAR") );
		while( bEnque || ( read( inputpipe, &ch, 1 ) < 1 ) )
		{
			lprintf( WIDE("Waiting for input event.") );
			WakeableSleep( SLEEP_FOREVER );
			lprintf( WIDE("Input event received.") );
		}
		lprintf( WIDE("Read input.") );
	}
	if( ch == '\r' )
		fputc( '\n', save );
	else
		fputc( ch, save );
	fflush( save );
	return ch;
}

#ifdef __LINUX__
off_t tell(int fd)
{
   return lseek( fd, 0, SEEK_CUR );
}
#endif

void EnqueStrokes( const TEXTCHAR *strokes )
{
	_32 savepos;
	int len = strlen( strokes );
	bEnque = 1;
	savepos = tell( inputpipe );
	lprintf( WIDE("write to input event pipe") );
	lseek( inputpipe, 0, SEEK_END);
	write( inputpipe, strokes, len );
	lseek( inputpipe, savepos, SEEK_SET );
	bEnque = 0;
	lprintf( WIDE("wake input thread") );
	WakeThread( pThreadInput );
}

int GetANumber( void )
{
	int accum = 0;
	int len = 0;
	TEXTCHAR ch;
	// make sure the prompt has been issued
	fflush( stdout );
	g.flags.bChoiceNeedsEnter = 1;
	while( 1 )
	{ 
		ch = GetCh();
		if( ch == '\b' )
		{
			accum /= 10;
		}
		if( ch == '\r' || ch == '\n' )
			break;
		if( ch == 3 )
			exit(1);
		if( ch >= '0' && ch <= '9' )
		{
			accum *= 10;
			accum += ch - '0';
		}
		while( len-- )
			printf( WIDE("\b \b") );
		len = printf( WIDE("%d"), accum );
		fflush( stdout );
		if( !accum && ch == '0' )
         break;
	}
	printf( WIDE("\n") );
	g.flags.bChoiceNeedsEnter = 0;
   return accum;
}

int GetYesNo( void )
{
	TEXTCHAR ch;
   // make sure the prompt has been issued
   fflush( stdout );
	g.flags.bChoiceNeedsEnter = 0;
	ch = GetCh();
   printf( WIDE("\n") );
	if( ch == 'y' || ch == 'Y' )
	{
		return TRUE;
	}
	return FALSE;
}


int GetAString( TEXTCHAR *buffer, int bufsiz )
{
	int ofs = 0;
   int len = 0;
	TEXTCHAR ch;
   fflush( stdout );
	g.flags.bChoiceNeedsEnter = 1;
	while( ofs < bufsiz )
	{
		buffer[ofs] = GetCh();
		if( buffer[ofs] == '\b' )
		{
			ofs--;
         buffer[ofs] = 0;
		}
		else if( buffer[ofs] == '\r' || buffer[ofs] == '\n' )
		{
         ofs++;
			break;
		}
		else
         buffer[++ofs] = 0;
		while( len-- )
			printf( WIDE("\b \b") );
		len = printf( WIDE("%s"), buffer );
		fflush( stdout );
	}
  // if( g.flags.bRemote )
  // {
  //    SendRemote( buffer, ofs );
  // }
   printf( WIDE("\n") );
	g.flags.bChoiceNeedsEnter = 0;
   buffer[ofs] = 0;
   return ofs;
}
