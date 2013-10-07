#define NO_SHLWAPI
#include <stdhdrs.h>
#include "termstruc.h"


/* HTTP Methods possible....

       Method         = WIDE("OPTIONS")                ; Section 9.2
                      | WIDE("GET")                    ; Section 9.3
                      | WIDE("HEAD")                   ; Section 9.4
                      | WIDE("POST")                   ; Section 9.5
                      | WIDE("PUT")                    ; Section 9.6
                      | WIDE("DELETE")                 ; Section 9.7
                      | WIDE("TRACE")                  ; Section 9.8
                      | WIDE("CONNECT")                ; Section 9.9
HTTP/1.1 MUST include
host: hostname
*/
static PTEXT GetToEnd( PSENTIENT ps, PTEXT *parameters )
{
   PTEXT seg;
   PTEXT line = NULL;
   PTEXT result = NULL;
   while( ( seg = GetParam( ps, parameters ) ) )
   {
      line = SegAppend( line, SegDuplicate( seg ) );
	}
	if( line )
	{
		line->flags &= ~(TF_SQUOTE|TF_QUOTE);
		result = BuildLine( line );
		LineRelease( line );
	}
   return result;
}

static PTEXT ParseURL( PTEXT pURL )
{
   TEXTCHAR *pSlash, *text;
   PTEXT pHost, pURI;

   text = GetText( pURL );

   if( text[0] == '\"' ) // this is ugly - not sure how this happens...
   {
      size_t len;
      text++;
      len = strlen( text );
      text[len-1] = 0; // early terminate last quote...
   }
   if( !strnicmp( text, WIDE("http://"), 7 ) )
   {
      text += 7;
   }

   pSlash = strchr( text, '/' );


   if( pSlash )
   {
      *pSlash = 0;
      pHost = SegCreateFromText( text );
      *pSlash = '/';
      SegAppend( pHost, pURI = SegCreateFromText( pSlash ) );
   }
   else
   {
      pHost = SegCreateFromText( text );
      SegAppend( pHost, SegCreateFromText( WIDE("/") ) );
   }
   return pHost;
}

static void CPROC RequestReadComplete( PCLIENT pc, POINTER pBuffer, size_t nSize )
{
   PTEXT *pResult;
   pResult = (PTEXT*)GetNetworkLong( pc, 0 );
   if( pBuffer )
   {
      PTEXT pNew;
      pNew = SegCreate( nSize );
      pNew->flags |= TF_BINARY;
      MemCpy( GetText( pNew ), pBuffer, nSize );
      *pResult = SegAppend( *pResult, pNew );
   }
   else
   {
      pBuffer = Allocate( 4096 );
      SetNetworkLong( pc, 2, (_32)pBuffer );
   }
   ReadTCP( pc, pBuffer, 4096 );
}

static void CPROC ReqeustComplete( PCLIENT pc )
{
   int *bClosed;
   bClosed = (int*)GetNetworkLong( pc, 1 );
   // get rid of the buffer...
   Release( (POINTER)GetNetworkLong( pc, 2 ) );
   *bClosed = TRUE;
}

static PTEXT SplitHTTPHeader( PTEXT pResult )
{
   PTEXT pCurrent, pStart;
   PTEXT pLine = NULL,
         pHeader = NULL;
   TEXTCHAR *c, *l;
   size_t size, pos, start, len;
   int bLine;
   pCurrent = pResult;
   pStart = pCurrent; // at lest is this block....
   bLine = 0;
   len = 0;
   while( pCurrent )
   {
      size = GetTextSize( pCurrent );
      c = GetText( pCurrent );
      if( bLine < 4 )
      {
         start = 0; // new packet and still collecting header....
         for( pos = 0; pos < size; pos++ )
         {
            if( c[pos] == '\r' )
               bLine++;
            else if( c[pos] == '\n' )
               bLine++;
            else // non end of line character....
            {
FinalCheck:
               if( bLine >= 2 ) // had an end of line...
               {
                  pLine = SegCreate( pos - start - bLine );
                  MemCpy( l = GetText( pLine ), c + start, pos - start - bLine);
                  l[pos-start-bLine] = 0;
                  pHeader = SegAppend( pHeader, pLine );
                  // could perhaps append a newline segment block...
                  // but probably do not need such a thing....
                  // since the return should be assumed as a continuous
                  // stream of datas....
                  start = pos;
                  if( bLine == 2 )
                     bLine = 0;
               }
               // may not receive anything other than header information?
               if( bLine == 4 )
               {
                  // end of header
                  // copy the previous line out...
                  pStart = pCurrent;
                  len = size - pos; // remaing size
                  break;
               }
            }
         }
         if( pos == size &&
             bLine == 4 &&
             start != pos )
            goto FinalCheck;
      }
      else
         len += size;
      pCurrent = NEXTLINE( pCurrent );
      /* Move the remaining data into a single binary data packet...*/
   }
   pLine = SegCreate( len );
   pLine->flags |= TF_BINARY;
   pCurrent = pStart;
   c = GetText( pLine );
   pos = 0;
   while( pCurrent )
   {
      MemCpy( c + pos, GetText( pCurrent ) + start, len = GetTextSize( pCurrent ) - start );
      pos += len;
      start = 0;
      pCurrent = NEXTLINE( pCurrent );
   }
   pHeader = SegAppend( pHeader, pLine );
   return pHeader;
}

static int HandleCommand(WIDE("HTTP"), WIDE("Perform http command operation..."))( PSENTIENT ps, PTEXT parameters )
{
   // HTTP GET <varname> <URL>
   // HTTP <OTHER> <...>
   // this retreives the resource at the URL
   // it will burst the URL into seperate parts to format a HTTP/1.1
   // request...
   // the first option determines the operation
   // variable will always be assigned per object, never within
   // the current script.
   // content of the variable will be binary.
	PTEXT operation;
   int InitNetwork( PSENTIENT ps );

   if( !InitNetwork(ps) )
   {
      return 0;
   }
   operation = GetParam( ps, &parameters );
   if( operation )
   {
      if( TextLike( operation, WIDE("get") ) )
      {
         PTEXT varname;
         varname = GetParam( ps, &parameters );
         if( varname )
         {
            PTEXT pURL;
            PTEXT pHost, pURI;
            SOCKADDR *sa;
            PCLIENT pc;
            int bClosed = FALSE;
            PTEXT pResult = NULL;
            pURL = GetToEnd( ps, &parameters );
            pHost = ParseURL( pURL );
            LineRelease( pURL );
            if( pHost )
            {
               pURI = SegGrab( NEXTLINE( pHost ) );
            }
            Log1( WIDE("Going to get from: %s(:80)"), GetText( pHost ) );
			   sa = CreateSockAddress( GetText( pHost ), 80 );
            if( sa )
            {
               pc = OpenTCPClientAddrEx( sa, RequestReadComplete, ReqeustComplete, NULL );
               if( pc )
               {
                  TEXTCHAR Request[290];
                  SetNetworkLong( pc, 0, (_32)&pResult );
                  SetNetworkLong( pc, 1, (_32)&bClosed );
                  SendTCP( pc, Request,
								  snprintf( Request, sizeof( Request ), WIDE("GET %s HTTP/1.1\r\n"), GetText( pURI ) ) );
						Log1( WIDE("Sent: %s"), Request );
                  SendTCP( pc, Request,
                           snprintf( Request, sizeof( Request ), WIDE("Host: %s\r\n"), GetText( pHost ) ) );
						Log1( WIDE("Sent: %s"), Request );
                  SendTCP( pc, Request,
                           snprintf( Request, sizeof( Request ), WIDE("User-Agent: Dekware %s\r\n"), DekVersion ) );
						Log1( WIDE("Sent: %s"), Request );
                  SendTCP( pc, Request,
                           snprintf( Request, sizeof( Request ), WIDE("Connection: close\r\n\r\n") ) );
						Log1( WIDE("Sent: %s"), Request );
                  while( !bClosed ) Sleep( 100 );
                  // should have pReuslt here that is a valid page resource...
                  {
                     PTEXT pVal;
                     AddVariable( ps, ps->Current, varname, pVal = SplitHTTPHeader( pResult ) );
                     LineRelease( pVal ); // is duplicated/substituted on AddVar
                  }
                  LineRelease( pResult );
                  Sleep(1); // why is this sleep here?!
                  if( ps->CurrentMacro )
                     ps->CurrentMacro->state.flags.bSuccess = TRUE;
               }
            }
            else
            {
               DECLTEXT( msg, WIDE("Could not resolve host address.") );
					Log( WIDE("Could not resolve host address.") );
               if( !ps->CurrentMacro )
                  EnqueLink( &ps->Command->Output, &msg );
            }
            LineRelease( pURI );
            LineRelease( pHost );
         }
      }
      else
      {
         DECLTEXT( msg, WIDE("Only method of HTTP supported is GET.") );
         EnqueLink( &ps->Command->Output, &msg );
      }
//      else if( TextLike( operation, "
   }
   else
   {
      DECLTEXT( msg, WIDE("HTTP requires METHOD, VarName, and address") );
      EnqueLink( &ps->Command->Output, &msg );
   }
   return 0;
}


void SendBinaryWork( PLINKQUEUE *pplq, PTEXT segment )
{
	while( segment )
	{
		if( segment->flags & TF_INDIRECT )
		{
			SendBinaryWork( pplq, GetIndirect( segment ) );
		}
      else
			EnqueLink( pplq, SegDuplicate( segment ) );
      segment = NEXTLINE( segment );
	}
}

static int HandleCommand(WIDE("SendData"), WIDE("Send binary data to socket connection"))( PSENTIENT ps, PTEXT parameters )
{
	PTEXT segment;
	while( segment = GetParam( ps, &parameters ) )
	{
      SendBinaryWork( &ps->Data->Output, segment );
	}

   return 0;
}

static int HandleCommand(WIDE("ReadData"), WIDE("Read binary data from socket connection"))( PSENTIENT ps, PTEXT parameters )
{
   return 0;
}
