#include <stdhdrs.h>
#ifdef __LCC__
//#include <wincon.h>
#endif
// uncomment this to display [ ]'s around entered segments Win95!
// #define DEBUG_SEGMENTS

#include "plugin.h"

static int bDebug = FALSE;


typedef struct mydatapath_tag {
   DATAPATH     common;
   COMMAND_INFO CommandInfo;
   int bExtendedKey; // set true when key is true...
   char buffer[64];
   int bufhead, buftail;
   // after outputting this, need cursor position 
   // so that when inserting - meaningful cursor can be
   // maintained on display....
} MYDATAPATH, *PMYDATAPATH;

// NOTE: ONLY one(1) console window ever

int nLines = 240; // minus one so that the pause can fit on screen...
int CommandLengthShown;
int b95 = FALSE;
int bOutputPause;

//HANDLE hThread;  // since only one console may exist... these are single
uint32_t dwThread;  // global instances of thread data

//HANDLE hStdin, 1;
//INPUT_RECORD InputBuffer[16];
                 
//typedef struct _CONSOLE_SCREEN_BUFFER_INFO {
//   COORD dwSize;
//   COORD dwCursorPosition;
//   WORD wAttributes;
//   SMALL_RECT srWindow;
//   COORD dwMaximumWindowSize;
//} CONSOLE_SCREEN_BUFFER_INFO,*PCONSOLE_SCREEN_BUFFER_INFO;

typedef struct Position {
   int x, y;
}POS;
POS  LastCommandPos;
POS  CurrentCommand;
int  CommandLength;
LOGICAL bLastOutputWasCommand;
char blanks[81] = "                                                                                ";

int UndidWrap = FALSE;

POS readpos;
int bPosValid;

#if !defined( WIN32 ) && !defined( __QNX__ )
int stricmp(const char * dst, const char * src)
{
   int f,l;
	
	do {
	   if ( ((f = (unsigned char)(*(dst++))) >= 'A') && (f <= 'Z') )
	        f -= ('A' - 'a');
		
		  if ( ((l = (unsigned char)(*(src++))) >= 'A') && (l <= 'Z') )
		         l -= ('A' - 'a');
	} while ( f && (f == l) );
	
	  return(f - l);
	 }
	 
int strnicmp (
    const char * first,
    const char * last,
    size_t count
       )
{
    int f,l;

    if ( count )
	   {
	
	      do {
	     if ( ((f = (unsigned char)(*(first++))) >= 'A') &&
	   (f <= 'Z') )
	    f -= 'A' - 'a';
	 
	    if ( ((l = (unsigned char)(*(last++))) >= 'A') &&
		                 (l <= 'Z') )
		                            l -= 'A' - 'a';
						    
						                    } while ( --count && f && (f == l) );
									  
  return( f - l );
}

     return( 0 );
}
#endif

void UnixGetCursorPos( POS *pos )
{
   bPosValid = FALSE;
   write( 1, "\x1b[6n", 4 );
   while( !bPosValid ) 
   {
      Sleep( 20 );
   }
   pos->x = readpos.x;
   pos->y = readpos.y;
   if(bDebug)
   {
	char msg[256];
	int sz;
	sz = sprintf( msg, "Gos Pos %d, %d\n", pos->x, pos->y );
      write( 2, msg, sz );
   }
}

void UnixSetCursorPos( POS *pos )
{
   char buf[64];
   int sz;
   if(bDebug)
   {
	char msg[256];
	int sz;
	sz = sprintf( msg, "Set Pos %d, %d\n", pos->x, pos->y );
      write( 2, msg, sz );
   }
   sz = sprintf( buf, "\x1b[%d;%dH", pos->y, pos->x );
   write( 1, buf, sz );
}

char ColorMap[8] = { 0, 4, 2, 6, 1, 5, 3, 7 };

void SetAttr( PFORMAT pf )
{
   char buf[64];
   int sz;
   int attr;
   int fore;
   int back;
   if( pf->foreground & 8 )
   {
		if( pf->background & 8 ) 
		{
   	   sz = sprintf( buf, "\x1b[0m\x1b[%d;%d;5;1m", 30+ColorMap[pf->foreground&7]
                        , 40+ColorMap[pf->background&7] );
		}
		else
		{
   	   sz = sprintf( buf, "\x1b[0m\x1b[%d;%d;1m", 30+ColorMap[pf->foreground&7]
                        , 40+ColorMap[pf->background&7] );
		}
   }
   else
   {
		if( pf->background & 8 )
		{
   	   sz = sprintf( buf, "\x1b[0m\x1b[%d;%d;5m", 30+ColorMap[pf->foreground&7]
                        , 40+ColorMap[pf->background&7] );
		}
		else
		{
   	   sz = sprintf( buf, "\x1b[0m\x1b[%d;%dm", 30+ColorMap[pf->foreground&7]
                        , 40+ColorMap[pf->background&7] );
		}
   }   
   write( 1, buf, sz );
   if(bDebug)
   {
		char msg[256];
		int sz;
		sz = sprintf( msg, "Set Attribute\n" );
      write( 2, msg, sz );
   }
}

void MarkCommandBeginning( void )
{
   if( !bLastOutputWasCommand )
   {
      UnixGetCursorPos( &LastCommandPos );       
      UndidWrap = FALSE;
   }
}

// if update is set, this is being cleared only for a command update
// if this is NOT set, the output was cleared by a data output...
void ClearLastCommandOutput( LOGICAL bUpdate )
{
   int blanks_left;
   int linesback;
   uint32_t dwSize;
   if( bLastOutputWasCommand )
   {
    linesback = ( LastCommandPos.x + CommandLength ) / 80;
//                   LastCommandPos.dwSize.X; // must span ENTIRE buffer length...

      if( ( CurrentCommand.y + linesback ) >=
          ( 58 ) )
      {
         UndidWrap = ( ( 58 - 1 ) -
                       LastCommandPos.y );
         if( linesback > UndidWrap )
         {
            LastCommandPos.y -= linesback - UndidWrap;
            UndidWrap = linesback;
         }
      }

      if( bUpdate )
      {
            UnixSetCursorPos( &CurrentCommand );
         blanks_left = CommandLength - CommandLengthShown;
      }
      else
      {
            UnixSetCursorPos( &LastCommandPos );
         blanks_left = CommandLength;
      }

   if(bDebug)
   {
	char msg[256];
	int sz;
	sz = sprintf( msg, "Clearing %d blanks...\n", blanks_left );
      write( 2, msg, sz );
   }
      while( blanks_left )
      {
         if( blanks_left > 80 )
            dwSize = write( 1, blanks, 80 );
         else
            dwSize = write( 1, blanks, blanks_left );
         blanks_left -= dwSize;
      }

      if( bUpdate )
         UnixSetCursorPos( &CurrentCommand );
      else
      {
         UnixSetCursorPos( &LastCommandPos );
         CommandLengthShown = 0;
         //bLastOutputWasCommand = FALSE; // might as well be the case?
      }
   }
   // at this point expecting to output non command output..
   // -or- replace command output with new data..
   // but is positioned at the start of last command entry whether it was
   // longer than a line or not..
}


void OutputText( PTEXT pText, LOGICAL bCommand, LOGICAL bSingle )
{
   uint32_t dwSize;
   if( bCommand )
   {
      // if last output was non-command this will grab the current position
      // to update cursor for ClearCommand
      MarkCommandBeginning();
      bLastOutputWasCommand = TRUE;
   }
   else
   {
      ClearLastCommandOutput( FALSE ); // remove any command written..
      bLastOutputWasCommand = FALSE;
   }
   if( !pText )
   {
   if(bDebug)
   {
	char msg[256];
	int sz;
	sz = sprintf( msg, "Output Newline!\n" );
      write( 2, msg, sz );
   }
      // format a newline ..
      write( 1, "\r\n", 2 );
      return;
   }

   for( ; pText ; pText = (bSingle?NULL:NEXTLINE( pText )) )
   {
      if( pText->flags & TF_INDIRECT )
      {
         OutputText( GetIndirect( pText ), bCommand, bSingle );
         continue;
      }
      if( pText->flags & TF_FORMAT )
      {
         int attr;
	   	SetAttr( (PFORMAT)(&pText->data.size) );
	   
         //attr = ((PFORMAT)(&pText->data.size))->foreground | 
         //      (( ((PFORMAT)(&pText->data.size) )->background ) << 4 );
         //SetConsoleTextAttribute( 1, attr );
         if(0)
         {
            POS pos;
            pos.x = ((PFORMAT)(&pText->data.size))->x;
            pos.y = ((PFORMAT)(&pText->data.size))->y;
            UnixSetCursorPos( &LastCommandPos );
         }
      }
      if( bCommand )
      {
         int len, newlen, ofs;
         CommandLength += ( len = GetTextSize( pText ) );
         if( CommandLength > CommandLengthShown )
         {
            ofs = 0;
            if( ( ofs = 
                     ( len - ( newlen = ( CommandLength 
                                        - CommandLengthShown ) ) ) ) >= 0 )
            {
               len = newlen; 
            }
            else
            {
               // len == len..
               ofs = 0;
            }
            // start from this partial..
            write( 1, GetText( pText ) + ofs, len );
   if(bDebug)
   {
	char msg[256];
	int sz;
	sz = sprintf( msg, "Output Command(%d/%d): %s\n", ofs, len, GetText( pText) );
      write( 2, msg, sz );
   }
         }
   if(bDebug)
   {
	char msg[256];
	int sz;
	sz = sprintf( msg, "Output Command Length: %d of %d\n", CommandLengthShown, CommandLength );
      write( 2, msg, sz );
   }
      }
      else
	{
         write( 1, GetText( pText ), GetTextSize( pText ) );
   if(bDebug)
   {
	char msg[256];
	int sz;
	sz = sprintf( msg, "Write Text(%d):%s\n", GetTextSize( pText ), GetText( pText) );
      write( 2, msg, sz );
   }
	}
   }
}

void WriteCommandText( PMYDATAPATH pdp );


int Write( PDATAPATH pdp )
{
   //PSENTIENT ps, 
   PMYDATAPATH pmdp = (PMYDATAPATH)pdp;
   PTEXT pCommand;
   int lines = 0;
   static int bOutOfPause;
   if( bOutputPause )
   {
      bOutOfPause = TRUE;
      return 1;
   }
   while( pCommand = DequeLink( pdp->ppOutput ) )
   {
      PTEXT pOutput;
      if( !( b95 && bOutOfPause ) || !b95 )
      {
         if( !(pCommand->flags & TF_NORETURN) )
            OutputText( NULL, FALSE, FALSE );
      }
      else
         bOutOfPause = FALSE;
      pOutput = BuildLine( pCommand );
      OutputText( pOutput, FALSE, FALSE );
      LineRelease( pOutput );
      LineRelease( pCommand ); // after successful output no longer need this..

      lines++;
      if( lines >= nLines )
      {
         DECLTEXT( msg95, "Press any key to continue" );
         DECLTEXT( msg, "Press return to continue" );
         // double spaces if prior input was NO_RETURN with return postfix
         // although it might not have any return and.. BLAH
         OutputText( NULL, FALSE, FALSE ); // newline prefix..
         if( b95 )
            OutputText( (PTEXT)&msg95, TRUE, FALSE );
         else
            OutputText( (PTEXT)&msg, TRUE, FALSE );
         bOutputPause = TRUE;
         break;
      }
   }
   while( LockedExchange( &pmdp->CommandInfo.CollectionBufferLock, 1 ) )
      Sleep(0);
   if( !bOutputPause )
      if( CommandLength ) // restore command after output is done..
         WriteCommandText( pmdp );
   pmdp->CommandInfo.CollectionBufferLock = 0;
   return 1;
}

int Close( PMYDATAPATH pdp )
{
   PTEXT pHistory;
   printf( "MUST END THREAD!\n" );
//   TerminateThread( dwThread, 0xD1E );

   pdp->common.Close = NULL;
   pdp->common.Write = NULL;
   pdp->common.Read = NULL;
   pdp->common.Type = 0;

   while( pHistory = DequeLink( &pdp->CommandInfo.InputHistory ) )
      LineRelease( pHistory );
   DeleteLinkQueue( &pdp->CommandInfo.InputHistory );

   LineRelease( pdp->CommandInfo.CollectionBuffer );
   pdp->CommandInfo.InputHistory = NULL;
   pdp->CommandInfo.nHistory = -1; // shouldn't matter what this gets set to..
   return 1;
}

//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
int bUpdateToEnd = TRUE;
void WriteCommandText( PMYDATAPATH pdp )
{
// this is the routine which writes the command line currently entered..
   PTEXT pText;
   int diff = 0, atend = 1;
   // since we write the entire line - have to remove the prior line
   // and set the cursor to the start for input..
   ClearLastCommandOutput( bUpdateToEnd );
   bUpdateToEnd = TRUE;
   CommandLength = 0;

      pText = pdp->CommandInfo.CollectionBuffer;
      SetStart( pText );
      while( pText )
      {
           OutputText( pText, TRUE, TRUE );
         if( pText == pdp->CommandInfo.CollectionBuffer )
         {
            diff = GetTextSize( pText ) - pdp->CommandInfo.CollectionIndex;
            CommandLengthShown = CommandLength - diff;
            if( NEXTLINE( pText ) )
               atend = FALSE;
            else
               atend = TRUE;
		UnixGetCursorPos( &CurrentCommand );
              //GetConsoleScreenBufferInfo( 1, &CurrentCommand );
         }
         pText = NEXTLINE( pText );
      }
//      if( !CommandLengthShown )
//         CommandLengthShown = CommandLength;

   if( diff )
   {
      CurrentCommand.x -= diff;
      while( CurrentCommand.x < 0 )
      {
         CurrentCommand.y--;
         CurrentCommand.x += 80;
      }
   }
   if( !atend || diff )
   {
      UnixSetCursorPos( &CurrentCommand );
   }
}

//----------------------------------------------------------------------------
int bEscape;
int codelen;
int nParams;
int nParamData[32];
int AnsiInput( PMYDATAPATH pdp, char next )
{
// return 0 - char used, do nothing
// return 1 - char used, update output
// return 2 - invalid sequence... clear buffer...
   if( !codelen && next == '[' )
   {
   	codelen++;
	nParams = 0;
	nParamData[nParams] = 0;	
	return 0;
   }
   if( codelen )
   {
	if( next == ';' )
	{
	   nParams++;
	   nParamData[nParams] = 0;
	   return 0;
	}
	if( next >= '0' && next <= '9' )
	{
	   nParamData[nParams] *= 10;
	   nParamData[nParams] += next - '0';
	   return 0;
	}
	switch( next )
	{
	case 'R':
	   readpos.y = nParamData[0];
	   readpos.x = nParamData[1];
	   bPosValid = TRUE;
	   bEscape = FALSE;
	   return 0;
	   break;
	case 'A'://UP
            RecallCommand( &pdp->CommandInfo, TRUE );
            bUpdateToEnd = FALSE;
	   bEscape = FALSE;
	    return 1;
            break;
	   
	case 'B'://down
            RecallCommand( &pdp->CommandInfo, FALSE );
            bUpdateToEnd = FALSE;
//            bOutput = TRUE;
	   bEscape = FALSE;
	    return 1;
            break;
	case 'C'://right
	      while( LockedExchange( &pdp->CommandInfo.CollectionBufferLock, 1 ) )
	         Sleep(0);
            if( ++pdp->CommandInfo.CollectionIndex >= (int)GetTextSize( pdp->CommandInfo.CollectionBuffer ) )
            {
               if( NEXTLINE( pdp->CommandInfo.CollectionBuffer ) )
               {
                  pdp->CommandInfo.CollectionBuffer = NEXTLINE( pdp->CommandInfo.CollectionBuffer );
                  pdp->CommandInfo.CollectionIndex = 0;
               }
               else
               {
                  // hmm well..set to last character in this buffer.
                  pdp->CommandInfo.CollectionIndex = GetTextSize( pdp->CommandInfo.CollectionBuffer );
               }
            }
	      bEscape = FALSE;
            WriteCommandText( pdp);
		pdp->CommandInfo.CollectionBufferLock = 0;
		return 1;
            break;
	case 'D'://left
	      while( LockedExchange( &pdp->CommandInfo.CollectionBufferLock, 1 ) )
	         Sleep(0);
            if( pdp->CommandInfo.CollectionIndex )
            {
               pdp->CommandInfo.CollectionIndex--;
            }
            else
            {
               while( PRIORLINE( pdp->CommandInfo.CollectionBuffer ) &&
                      pdp->CommandInfo.CollectionIndex == 0 )
               {
                  pdp->CommandInfo.CollectionBuffer = PRIORLINE( pdp->CommandInfo.CollectionBuffer );
                  pdp->CommandInfo.CollectionIndex = GetTextSize( pdp->CommandInfo.CollectionBuffer );
               }
               pdp->CommandInfo.CollectionIndex--;
            }
            WriteCommandText( pdp );
		bEscape = FALSE;
		pdp->CommandInfo.CollectionBufferLock = 0;
		return 1;
            break;
	case 'H'://home
	      while( LockedExchange( &pdp->CommandInfo.CollectionBufferLock, 1 ) )
	         Sleep(0);
            while( PRIORLINE( pdp->CommandInfo.CollectionBuffer ) )
               pdp->CommandInfo.CollectionBuffer = PRIORLINE( pdp->CommandInfo.CollectionBuffer );
            pdp->CommandInfo.CollectionIndex = 0;
//            bOutput = TRUE; // update current cursor position..
	   bEscape = FALSE;
		pdp->CommandInfo.CollectionBufferLock = 0;
	   return 1;
            break;
	case 'Y'://end
	      while( LockedExchange( &pdp->CommandInfo.CollectionBufferLock, 1 ) )
	         Sleep(0);
            while( NEXTLINE( pdp->CommandInfo.CollectionBuffer ) )
               pdp->CommandInfo.CollectionBuffer = NEXTLINE( pdp->CommandInfo.CollectionBuffer ); 
            pdp->CommandInfo.CollectionIndex = GetTextSize( pdp->CommandInfo.CollectionBuffer );
//            bOutput = TRUE;
	   bEscape = FALSE;
		pdp->CommandInfo.CollectionBufferLock = 0;
	   return 1;
            break;
	case 'U'://pgdn
	   break;
	case 'V'://pgup
	   break;
	case '@'://insert
            pdp->CommandInfo.CollectionInsert = !pdp->CommandInfo.CollectionInsert;
            break;
	case 'P'://delete
	   break;
	default:
	   bEscape = FALSE; // reneable normal processing - bad sequence..
	}	
   }
   return 2; // handle escape to clear line..
}

//----------------------------------------------------------------------------

void KeyEventProc(PMYDATAPATH pdp, char event)
{
   // this must here gather keystrokes and pass them forward into the 
   // opened sentience..
//   PTEXT temp;
   int bOutput = FALSE;
   while( LockedExchange( &pdp->CommandInfo.CollectionBufferLock, 1 ) )
      Sleep(0);
   if( event )	
   {
      PTEXT pLine;
      DECLTEXT( key, " " );
      if( bOutputPause )
      {
         bOutputPause = FALSE;
         pdp->CommandInfo.CollectionBufferLock = 0;
         return;
      }
      switch( key.data.data[0] = event )
      {
	
      /*
      case 0: // ignore?  handle specially??
         switch( event.wVirtualKeyCode )
         {
         case VK_HOME:
            while( PRIORLINE( pdp->CommandInfo.CollectionBuffer ) )
               pdp->CommandInfo.CollectionBuffer = PRIORLINE( pdp->CommandInfo.CollectionBuffer );
            pdp->CommandInfo.CollectionIndex = 0;
            bOutput = TRUE; // update current cursor position..
            break;
         case VK_END:
         case VK_UP:
            RecallCommand( &pdp->CommandInfo, TRUE );
            bUpdateToEnd = FALSE;
            bOutput = TRUE;
            break;
         case VK_DOWN:
            RecallCommand( &pdp->CommandInfo, FALSE );
            bUpdateToEnd = FALSE;
            bOutput = TRUE;
            break;
         case VK_INSERT:
            pdp->CommandInfo.CollectionInsert = !pdp->CommandInfo.CollectionInsert;
            break;
         case VK_DELETE:
            key.data.data[0] = '\x7f';
            goto normal_process;
            break;
         case VK_LEFT:
            if( pdp->CommandInfo.CollectionIndex )
            {
               pdp->CommandInfo.CollectionIndex--;
            }
            else
            {
               while( PRIORLINE( pdp->CommandInfo.CollectionBuffer ) &&
                      pdp->CommandInfo.CollectionIndex == 0 )
               {
                  pdp->CommandInfo.CollectionBuffer = PRIORLINE( pdp->CommandInfo.CollectionBuffer );
                  pdp->CommandInfo.CollectionIndex = GetTextSize( pdp->CommandInfo.CollectionBuffer );
               }
               pdp->CommandInfo.CollectionIndex--;
            }
            WriteCommandText( pdp );
            break;
         case VK_RIGHT:
            if( ++pdp->CommandInfo.CollectionIndex >= (int)GetTextSize( pdp->CommandInfo.CollectionBuffer ) )
            {
               if( NEXTLINE( pdp->CommandInfo.CollectionBuffer ) )
               {
                  pdp->CommandInfo.CollectionBuffer = NEXTLINE( pdp->CommandInfo.CollectionBuffer );
                  pdp->CommandInfo.CollectionIndex = 0;
               }
               else
               {
                  // hmm well..set to last character in this buffer.
                  pdp->CommandInfo.CollectionIndex = GetTextSize( pdp->CommandInfo.CollectionBuffer );
               }
            }
            WriteCommandText( pdp);
            break;
         }
         break;
*/    
         if(0)
         {
      case '\r':
            // should be okay cause we're using gather for historical lines too..
            key.data.data[0] = '\n'; // carriage return = linefeed 
//----------
            // this is the only spot in which command is not
            // erased before output.. do not know what this 
            // will look like ... so cleat last output was command..
//----------
/*
            while( pdp->CommandInfo.CollectionBuffer && NEXTLINE( pdp->CommandInfo.CollectionBuffer ) )
               pdp->CommandInfo.CollectionBuffer = NEXTLINE( pdp->CommandInfo.CollectionBuffer );
            pdp->CollectionIndex = GetTextSize( pdp->CommandInfo.CollectionBuffer ); //put return at end.
*/
            bLastOutputWasCommand = FALSE; // fake Clear Ouptut..
            goto normal_process;  // do not output return.. extra lines otherwise
                     // output is always prefix linefed..

         }
      case 9:
         key.data.data[0] = ' ';
	   if(0){
	case '\x7f':
	   key.data.data[0] = '\b';
	   }
      case '\b':
	case 27:	
         bUpdateToEnd = FALSE; // need update ALL not just to end   
      default:
normal_process:
        // for( ; event.wRepeatCount; event.wRepeatCount-- )
         {
            pLine = GatherLine( &pdp->CommandInfo.CollectionBuffer
                              , &pdp->CommandInfo.CollectionIndex
                              , pdp->CommandInfo.CollectionInsert
                              , TRUE
                              , (PTEXT)&key );
            if( pLine )
            {
               PTEXT pCommand, _pCommand;
               pCommand = burst( &pdp->common.Partial, pLine );
               EnqueHistory( &pdp->CommandInfo, pLine );
               _pCommand = pCommand;
               while( pCommand )
               {
                  if( !(pCommand->flags & IS_DATA_FLAGS ) &&
                      TextIs( pCommand, ";" ) )
                  {
                     if( _pCommand != pCommand )
                     {
                        EnqueLink( &pdp->common.Input, _pCommand );
                        SegBreak( pCommand );
                        _pCommand = NEXTLINE( pCommand );
                        LineRelease( SegGrab( pCommand ) );
                        pCommand = _pCommand;
                        continue;
                     }
                  }
                  pCommand = NEXTLINE( pCommand );
               }
               if( _pCommand )
                  EnqueLink( &pdp->common.Input, _pCommand );
            }
         }
         bOutput = TRUE;
         break;
      }
   }
   else 
	bOutput = TRUE;
do_output:      
      // call to clear and re-write the current buffer..
   if( bOutput ) // so we don't output on shift, control, etc..
      WriteCommandText( pdp );
   pdp->CommandInfo.CollectionBufferLock = 0;
}


int bThreadStarted = FALSE;
void *ConsoleInputThreadDispatch( PMYDATAPATH pdp )
{
   char c;
   while( TRUE )
   {
	if( pdp->bufhead != pdp->buftail )
	{
	   int newtail;
	   KeyEventProc( pdp, pdp->buffer[pdp->buftail] );
	   newtail = (pdp->buftail+1)&63;
	   pdp->buftail = newtail;
	}
	else
	   Sleep(20);
   }
  // ExitThread( 0xE0F );
  return 0;
}
void *ConsoleInputThread( PMYDATAPATH pdp )
{
   char c;
   while( TRUE )
   {   
      if( read( 0, &c, 1 ) > 0 )
      {
	   int newhead;
	   if( bEscape )
	   {
   		switch( AnsiInput( pdp, c )  )
   		{
		case 1:
		   c = 0;
		   goto enque_flush;
	         break;
		case 2:
	         c = 27; // fill in escape...
		   goto enque_flush;
		case 0:
		   continue; // character used... no enque...
		}
	   }
	   if( c == 27 )
	   {
	      bEscape = TRUE;
		codelen = 0;
		continue;
	   }
enque_flush:	   
	   pdp->buffer[pdp->bufhead] = c;
	   newhead = (pdp->bufhead+1) & 63;
	   if( newhead == pdp->buftail )
	       printf( "Input buffer overflow!!!" );
	   pdp->bufhead = newhead;
      }
   }
  // ExitThread( 0xE0F );
  return 0;
}

extern int myTypeID; 

PDATAPATH Open( PSENTIENT ps, PTEXT parameters )
{
   // no parameters needed for this..
   PMYDATAPATH pdp;

   uint32_t fdwMode, fdwSaveOldMode; 
//   if( hStdin || 1 ) // console device is open here..
//      return NULL;
//   if( AllocConsole() )
   {
      LineRelease( parameters );
//      GetConsoleScreenBufferInfo( 1, &LastCommandPos );
//      nLines = LastCommandPos.dwSize.Y - 1;
      pdp = (PMYDATAPATH)CreateDataPath( sizeof( MYDATAPATH ) - sizeof( DATAPATH ) );
      
      pdp->common.Type = myTypeID;
      pdp->common.Close = (int (*)( struct datapath_tag *pdp ))Close;
      pdp->common.Read = NULL;
      pdp->common.Write = Write;
      pdp->common.flags.KeepCR = FALSE;

      pdp->CommandInfo.InputHistory = CreateLinkQueue();
      pdp->CommandInfo.nHistory = -1;
      pdp->CommandInfo.bRecallBegin = FALSE;

      pdp->CommandInfo.CollectionIndex = 0;

      pdp->CommandInfo.CollectionInsert = TRUE;
      pdp->CommandInfo.CollectionBufferLock = FALSE;
      pthread_create( &dwThread, NULL, (void*(*)(void*))ConsoleInputThread, pdp );
      pthread_create( &dwThread, NULL, (void*(*)(void*))ConsoleInputThreadDispatch, pdp );
	
	UnixGetCursorPos( &LastCommandPos );
	
      return (PDATAPATH)pdp;
   }
   return NULL;
}

int SetLines( PSENTIENT ps, PTEXT parameters )
{
   PTEXT pLines;
   pLines = GetParam( ps, &parameters );
   if( pLines )
   {
      if( IsNumber( pLines ) )
         nLines = IntNumber( pLines );
      else
      {
         DECLTEXT( msg, "Parameter to LINES command was not a number." );
         EnqueLink( ps->Command->ppOutput, &msg );
      }
   }
   else
   {
      DECLTEXT( msg, "Must specify the number of LINES." );
      EnqueLink( ps->Command->ppOutput, &msg );
   }

   return 0;
}

/*
int GetCommand( PSENTIENT ps, PTEXT parameters )
{
}
*/
// $Log: qnxcon.c,v $
// Revision 1.2  2003/03/25 08:59:03  panther
// Added CVS logging
//
