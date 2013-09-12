
#include <stdhdrs.h>
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

void SetInputFile( char *file )
{
   script = fopen( file, "rt" );
}

void SetOutputFile( char *file )
{
   save = fopen( file, "wt" );
}

PTRSZVAL CPROC InputThread( PTHREAD pThread )
{
	char ch[2];
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
		inputpipe = open( "stockmarket.pipe", O_RDWR|O_CREAT|O_TRUNC, 0600 );
		ThreadTo( InputThread, 0 );
	}
	if( !save )
		save = fopen( "stockmarket.save", "wt" );
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
		lprintf( "will read 1 char" );
		while( bEnque || ( read( inputpipe, &ch, 1 ) < 1 ) )
		{
			lprintf( "Waiting for input event." );
			WakeableSleep( SLEEP_FOREVER );
			lprintf( "Input event received." );
		}
		lprintf( "Read input." );
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

void EnqueStrokes( const char *strokes )
{
	_32 savepos;
	int len = strlen( strokes );
	bEnque = 1;
	savepos = tell( inputpipe );
	lprintf( "write to input event pipe" );
	lseek( inputpipe, 0, SEEK_END);
	write( inputpipe, strokes, len );
	lseek( inputpipe, savepos, SEEK_SET );
	bEnque = 0;
	lprintf( "wake input thread" );
	WakeThread( pThreadInput );
}

int GetANumber( void )
{
	int accum = 0;
	int len = 0;
	char ch;
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
			printf( "\b \b" );
		len = printf( "%d", accum );
		fflush( stdout );
		if( !accum && ch == '0' )
         break;
	}
	printf( "\n" );
	g.flags.bChoiceNeedsEnter = 0;
   return accum;
}

int GetYesNo( void )
{
	char ch;
   // make sure the prompt has been issued
   fflush( stdout );
	g.flags.bChoiceNeedsEnter = 0;
	ch = GetCh();
   printf( "\n" );
	if( ch == 'y' || ch == 'Y' )
	{
		return TRUE;
	}
	return FALSE;
}


int GetAString( char *buffer, int bufsiz )
{
	int ofs = 0;
   int len = 0;
	char ch;
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
			printf( "\b \b" );
		len = printf( "%s", buffer );
		fflush( stdout );
	}
  // if( g.flags.bRemote )
  // {
  //    SendRemote( buffer, ofs );
  // }
   printf( "\n" );
	g.flags.bChoiceNeedsEnter = 0;
   buffer[ofs] = 0;
   return ofs;
}
