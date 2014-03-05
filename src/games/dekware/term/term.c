#include <stdhdrs.h>
#include <logging.h>
#include <ctype.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <ctype.h>  // isdigit
#include <sharemem.h>
#include <timers.h>
#include <idle.h>

#ifndef WIN32
#define InterlockedDecrement(adr) ((*adr)--)
#define InterlockedIncrement(adr) ((*adr)++)
#endif
#define DEFINES_DEKWARE_INTERFACE
#include "termstruc.h"

#include <stdio.h>
#if 0
#ifdef _WIN32
	__declspec(dllimport)
#else
	extern
#endif
		int b95;
#endif

static INDEX iNetObject;
static int myTypeID;  // tcp client
static int myTypeID2; // tcp server
static int myTypeID3; // udp client
static int myTypeID4; // udp server


#if 0
PTEXT CPROC GetClientIP( PTRSZVAL psv, struct entity_tag *pe, PTEXT *lastvalue );
PTEXT CPROC GetServerIP( PTRSZVAL psv, struct entity_tag *pe, PTEXT *lastvalue );

static volatile_variable_entry vve_clientIP = { DEFTEXT( WIDE("client_ip") ), GetClientIP, NULL };
static volatile_variable_entry vve_serverIP = { DEFTEXT( WIDE("server_ip") ), GetServerIP, NULL };
#endif
//static volatile_variable_entry vve_clientIP;
//static volatile_variable_entry vve_serverIP;

#define MAX_CLIENTS 256

#define CONTROLLING_SENTIENCE 0
#define DATA_PATH 1

//--------------------------------------------------------------------------

int InitNetwork( PSENTIENT ps )
{
   static int bInit = FALSE;
   static int bIniting = FALSE;
   while( bIniting )
   {
      //Log( WIDE("Delay to init...\n"));
      Sleep(5);
   }
   bIniting = TRUE;
   //Log( WIDE("Start NetInit\n") );
   if( !bInit &&
       !NetworkWait( 0, MAX_CLIENTS, 32 ) )
   {
      DECLTEXT( msg, WIDE("Absolutely no network.") );
      if( ps->Command )
         EnqueLink( &ps->Command->Output, &msg );

      return FALSE;
   }
   bIniting = FALSE;
   bInit = TRUE;
   return TRUE;
}

//--------------------------------------------------------------------------

void CPROC ReadComplete( PCLIENT pc, POINTER pbuf, size_t nSize )
{
   PMYDATAPATH pdp;
   do
   {
      pdp = (PMYDATAPATH)GetNetworkLong( pc, DATA_PATH );
      if( !pdp )
      {
      	Log( WIDE("Terminal waiting for datapath to be present...") );
      	Relinquish();
      }
   }while( !pdp );
   if( pbuf )
   {
      ((P_8)pbuf)[nSize] = 0;
      pdp->Buffer->data.size = nSize;
#ifdef _UNICODE 
 		EnqueLink( &pdp->common.Input, SegCreateFromCharLen( (char*)pbuf, nSize ) );
#else
 		EnqueLink( &pdp->common.Input, SegDuplicate( pdp->Buffer ) );
#endif
      pdp->Buffer->data.size = 4096;
      WakeAThread( pdp->common.Owner );
   }
   ReadStream( pc, GetText(pdp->Buffer), GetTextSize( pdp->Buffer ) -1);
}

//---------------------------------------------------------------------------

static void CPROC TerminalCloseCallback( PCLIENT pc )
{
	PSENTIENT ps;
	PMYDATAPATH pdp;
	_32 tick = GetTickCount();
	// this code runs in the network thread, whose execution
	// path should not include ProcessCommand, therefore a
	// simple lock still works.
	do
	{
		ps = (PSENTIENT)GetNetworkLong( pc, CONTROLLING_SENTIENCE );
		pdp = (PMYDATAPATH)GetNetworkLong( pc, DATA_PATH );
	}while( ( !ps || !pdp ) && ( ( tick + 2000 ) > (Idle(),GetTickCount()) ) );
	if( ps && pdp )
	{
		// this code runs in the network thread, whose execution
		// path should not include ProcessCommand, therefore a
		// simple lock still works... there will be no self-collision
		while( LockedExchange( &ps->ProcessLock, 1 ) )
		{
			Idle();
		}

		// can't remove datapath cause we need to be able to retrive
		// any data which has been queued on it.
		pdp->common.flags.Closed = TRUE;
		//if(!ps->CurrentMacro)
		{
			//DECLTEXT( msg, WIDE("Terminal data connection closed.") );
			//EnqueLink( &pdp->common.Input, &msg );
			// do find macro, do command - else enque the message...
			if( !InvokeBehavior( WIDE("close_tcp"), ps->Current, ps, NULL ) )
			{
				// at least with this close method we can see it happen under /debug...
				QueueCommand( ps, WIDE("/CloseTerm") );
			}
		}
		pdp->handle = NULL; // will be closed after callback exits.
		WakeAThread( ps );
	}
	else
      lprintf( WIDE("Failed to get stuff... ") );
}
//---------------------------------------------------------------------------

static void CPROC ClientDisconnected( PCLIENT pc )
{
   PSENTIENT ps;
	PMYDATAPATH pdp;
   do
   {
      ps = (PSENTIENT)GetNetworkLong( pc, CONTROLLING_SENTIENCE );
		pdp = (PMYDATAPATH)GetNetworkLong( pc, DATA_PATH );
   }while( !ps || !pdp );

   if( ps )
   {
		if( !InvokeBehavior( WIDE("close_tcp"), ps->Current, ps, NULL ) )
		{
			// at least with this close method we can see it happen under /debug...
			DestroyEntity( ps->Current );
		}
	}
	else
      DebugBreak();
}

//---------------------------------------------------------------------------

//static void ClearLastCommandOutput( void )
//{
//}

//---------------------------------------------------------------------------

static void CPROC ServerRecieve( PCLIENT pc, POINTER pBuffer, size_t nSize )
{
   PMYDATAPATH pdp;
   PTEXT Buffer;
   do
   {
      pdp = (PMYDATAPATH)GetNetworkLong( pc, DATA_PATH );
   }while( !pdp );
   Buffer = pdp->Buffer;

   if( pBuffer )
   {
      PTEXT pBuild;
		Buffer->data.size = nSize;
		pBuild = SegDuplicate( Buffer );
		//lprintf( WIDE("Received: %s"), GetText( pBuild ) );
		EnqueLink( &pdp->common.Input, pBuild );
      WakeAThread( pdp->common.Owner );
   }
   Buffer->data.size = 4096;
   ReadTCP( pc
          , Buffer->data.data
          , Buffer->data.size );
   return;
}

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#ifndef _WIN32
#define __stdcall
#endif
// unsigned short __stdcall ntohs (unsigned short netshort);
//struct servent * __stdcall getservbyname(const TEXTCHAR * name,
//                                                 const TEXTCHAR * proto);

//---------------------------------------------------------------------------

static TEXTCHAR byOutputBuf[1492];
static int ofs;

static int CPROC TerminalTransmit( PDATAPATH pdpX )
{
   PMYDATAPATH pdp = (PMYDATAPATH)pdpX;
   size_t len;
   PTEXT tokens, pdel;
   //if( pdp->common.Type == myTypeID ||
   //    pdp->common.Type == myTypeID2)
   {
      ofs = 0;
      while( ( pdel = tokens = (PTEXT)DequeLink( &pdp->common.Output ) ) )
      {
      	{
				//Log2( WIDE("Sending %d bytes to connection FORMATTED! %08x"), GetTextSize( tokens ), NEXTLINE(tokens ) );
	         if( pdp->bCommandOut && !(tokens->flags & TF_NORETURN ))
   	      {
      	      if(( ofs + 2) < sizeof(byOutputBuf) )
         	   {
	         PutInLeadReturn:
   	            byOutputBuf[ofs++] = '\r';
      	         byOutputBuf[ofs++] = '\n';
         	      byOutputBuf[ofs] = 0;
            	}
	            else
   	         {
      	         SendTCP( pdp->handle, byOutputBuf, ofs );
         	      ofs = 0;
            	   goto PutInLeadReturn;
	            }
   	      }
      	   {
					PTEXT pSend, pSeg;
					//PTEXT first;

					if( !(tokens->flags & TF_BINARY) )
					{
						pSeg = pSend = BuildLine( tokens );
					}
					else
					{
						pSeg = tokens;
						pSend = NULL;
					}
					//len = LineLength( pSend );
				RequeueToBuffer:
					while( pSeg )
					{
						len = GetTextSize( pSeg );
						if( ofs + len >= sizeof( byOutputBuf ) )
						{
							if( ofs )
							{
								lprintf( WIDE("sending (%d) %s"), ofs, byOutputBuf );
								SendTCP( pdp->handle, byOutputBuf, ofs );
								ofs = 0;
								goto RequeueToBuffer;
							}
							else
							{
								SendTCP( pdp->handle, GetText(pSeg), GetTextSize( pSeg ) );
							}
						}
						else
						{
							MemCpy( byOutputBuf+ofs
									, GetText( pSeg )
									, len );
							//lprintf( WIDE("Enque block...") );
							//LogBinary( byOutputBuf + ofs, len );
							ofs += len;
						}
						pSeg = NEXTLINE( pSeg );
					}
					if( pSend )
						LineRelease( pSend );
				}
			}
			LineRelease( pdel );
		}
		if( ofs )
			SendTCP( pdp->handle, byOutputBuf, ofs );
	}
#if 0
   else
   {
      Log( WIDE("We're so fucked...\n") );
      DebugBreak();
	}
#endif
   return 0;
}

//---------------------------------------------------------------------------

static int CPROC Close( PDATAPATH pdpClose )
{
	PMYDATAPATH pdp = (PMYDATAPATH)pdpClose;
   //if( pdp->common.Type == myTypeID ||
   //    pdp->common.Type == myTypeID2 )
   {
      PSENTIENT ps = pdp->common.Owner;
		if( pdp->handle )
			RemoveClientEx( pdp->handle, TRUE, TRUE );
      pdp->common.Type = 0; // HT_CLOSED
      pdp->common.Close = NULL;
      pdp->common.Read = NULL;
      pdp->common.Write = NULL;
      LineRelease( pdp->Buffer );
      pdp->handle = NULL;
      if( pdp->CommandInfo.InputHistory )
      {
         PTEXT pLine;
         while( ( pLine = (PTEXT)DequeLink( &pdp->CommandInfo.InputHistory ) ) )
            LineRelease( pLine );
         DeleteLinkQueue( &pdp->CommandInfo.InputHistory );
      }
		{
			if( ps->Command )
			{
				// do find macro, do command - else enque the message...
				QueueCommand( ps, WIDE("/CloseTerm") );
			}
		}
      return 0;
   }
   return 1;
}
//---------------------------------------------------------------------------
static int CPROC AutoRelay( PDATAPATH pdp )
{
	RelayInput( pdp, NULL );
   return 0;
}

//---------------------------------------------------------------------------
static void ClearCommandHold( struct sentient_tag *ps// sentient processing macro
                   , struct macro_state_tag *pMacState ) // saves extra peek...
{
	//Log( WIDE("Clearing commands for macro...") );
	ps->flags.bHoldCommands = FALSE;
   PopData( &ps->MacroStack );
}

//---------------------------------------------------------------------------
static void CPROC ServerContacted( PCLIENT pListen,PCLIENT pNew  )
{
   PSENTIENT ps;
   PMYDATAPATH pdp, pdpserver;
   int bCommand;
   do
   {
      ps = (PSENTIENT)GetNetworkLong( pListen, CONTROLLING_SENTIENCE );
      pdpserver = (PMYDATAPATH)GetNetworkLong( pListen, DATA_PATH );
   }while( !ps || !pdpserver );
   if( pdpserver == (PMYDATAPATH)ps->Command )
      bCommand = TRUE;
   else
      bCommand = FALSE;

   {
		PENTITY pe;
      //PTEXT tmp;
      PSENTIENT psNew;

		pe = Duplicate( ps->Current );

      //AddVolatileVariable( pe, &vve_clientIP, (PTRSZVAL)pNew );
		//AddVolatileVariable( pe, &vve_serverIP, (PTRSZVAL)pNew );

		// this should make this thing's creator the createor fo the socket...
		// however, this means that the refernce to this object from
		// the real creator needs to be killed...
		{
			// oh - this is for purposes of output from this
         // object...
			//DeleteLink( &pe->pCreatedBy->pCreated, pe );
         //AddLink( &ps->Current->pCreatedBy, pe );
			//pe->pCreatedBy = ps->Current->pCreatedBy;
		}

		psNew = CreateAwareness( pe );
		//Assimilate( psNew, tmp = SegCreateFromText( WIDE("net device") ) );
      //LineRelease( tmp );
		psNew->flags.bHoldCommands = TRUE;
		if( bCommand )
		{
			// and we really should keep it around...
			// even though this is eally a fancy forced-opening...
			//DestroyDataPath( psNew->Command ); // new sentients have one already...
			pdp = CreateDataPath( &psNew->Command, MYDATAPATH );
			pdp->common.pName = SegCreateFromText( WIDE("cmd") );
		}
		else
		{
			DeleteLinkQueue( &psNew->Command->Output );
			pdp = CreateDataPath( &psNew->Data, MYDATAPATH );
			pdp->common.pName = SegCreateFromText( WIDE("data") );
		}
		pdp->common.Owner = psNew;
		{
			PMACROSTATE pms;
			// case doesn't matter, and we'll prove it.
			pms = InvokeBehavior( WIDE("accept_tcp"), ps->Current, psNew, NULL );
			if( !pms )
			{
				PMACRO match;
				match = LocateMacro( psNew->Current, WIDE("Accept") );
				if( match )
				{
					Log( WIDE("Invoking Accept macro..") );
					InvokeMacro( psNew, match, NULL );
					pms = (PMACROSTATE)PeekData( &psNew->MacroStack );
				}
			}
			if( pms )
				pms->MacroEnd = ClearCommandHold;

		}
		// else psNew->Data does not exist....
		//EnqueLink( psNew->Command->ppInput, burst( (PTEXT)&accept ) );
		//SetDatapathType( &pdp->common, myTypeID );
		pdp->common.Close = Close;
		pdp->common.Write = TerminalTransmit;
		pdp->common.Read = AutoRelay; // not needed cause data appears magically...
		pdp->bCommandOut = bCommand; // output from command stream very likely...
		pdp->handle = pNew;
		pdp->Buffer = SegCreate( 4096 ); // buffer size...
		pdp->CommandInfo.InputHistory = CreateLinkQueue();
		pdp->CommandInfo.nHistory = -1;

		pdp->common.flags = pdpserver->common.flags;
		pdp->flags = pdpserver->flags;

		SetNetworkReadComplete( pNew, ServerRecieve );
		SetNetworkCloseCallback( pNew, ClientDisconnected );
		SetNetworkLong( pNew, CONTROLLING_SENTIENCE, (PTRSZVAL)psNew );
		SetNetworkLong( pNew, DATA_PATH, (PTRSZVAL)pdp );
		UnlockAwareness( psNew );
	}
	// upon return if read callback is defined, then it will be called NULL, 0
}

//---------------------------------------------------------------------------
static int CPROC serverwrite( PDATAPATH ps )
{
   RelayOutput( ps, NULL );
   return 0;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

static PDATAPATH OnInitDevice( WIDE("tcpserver"), WIDE("Telnet server socket connection...")  )( PDATAPATH *pChannel, PSENTIENT ps, PTEXT parameters )
{
   PMYDATAPATH pdp;
   PTEXT pAddress;
   if( !InitNetwork(ps) )
   {
      return NULL;
   }

	pdp = CreateDataPath( pChannel, MYDATAPATH );

	pAddress = GetParam( ps, &parameters );
	if( pAddress )
	{
		SOCKADDR *sa;
		sa = CreateSockAddress( GetText( pAddress ), 23 );
		pdp->handle = OpenTCPListenerAddrEx( sa, ServerContacted );
		ReleaseAddress( sa );
		if( !pdp->handle )
		{
			DECLTEXT( msg, WIDE("Couldn't listen at network address\n") );
			EnqueLink( &ps->Command->Output, &msg );
			DestroyDataPath( (PDATAPATH)pdp );
			return NULL;
		}
		else
		{
			AddBehavior( ps->Current, WIDE("accept_tcp"), WIDE("This is a new connected, just now accepted") );
			AddBehavior( ps->Current, WIDE("close_tcp"), WIDE("Datapath has closed (multiple same-type datapaths fail") );
		}
		pdp->common.Type = 1;//myTypeID2;
		pdp->common.Close = Close;
		pdp->common.Write = serverwrite;
		pdp->common.Read = NULL; // not needed cause data appears on a separate thread...
		//pdp->common.flags.KeepCR = TRUE;
		SetNetworkLong( pdp->handle, CONTROLLING_SENTIENCE, (PTRSZVAL)ps );
		SetNetworkLong( pdp->handle, DATA_PATH, (PTRSZVAL)pdp );
   }
   return (PDATAPATH)pdp;
}

//---------------------------------------------------------------------------

static void  CPROC TerminalConnect( PCLIENT pClient, int nError )
{
	if( nError )
	{
   		Log1( WIDE("Aborted connect after return : %d"), nError );
		TerminalCloseCallback( pClient ); // calls remove client..
	}
	else
		ReadComplete( pClient, NULL, 0 ); // intial read queue...
}

//---------------------------------------------------------------------------
static PDATAPATH OnInitDevice(WIDE("tcp"), WIDE("Telnet type clear text socket connection..."))( PDATAPATH *pChannel, PSENTIENT ps, PTEXT parameters )
{
   SOCKADDR *sa;
   PTEXT pDestination;
   PMYDATAPATH pdp;


   pdp = CreateDataPath( pChannel, MYDATAPATH );
   pdp->Buffer = SegCreate( 4096 );

   if( !InitNetwork(ps) )
   {
      return NULL;
   }

	pDestination = BuildLineExx( GetParam( ps, &parameters ), TRUE, NULL DBG_SRC );
	Log1( WIDE("Address: \"%s\""), GetText( pDestination ) );
	sa = CreateSockAddress( GetText( pDestination ), 23 );
   LineRelease( pDestination );
   if( sa )
   {
      pdp->handle = OpenTCPClientAddrExx( sa, ReadComplete,
                                          TerminalCloseCallback,
                                          NULL,
                                          TerminalConnect );
		ReleaseAddress( sa );
      if( pdp->handle )
      {
         SetTCPNoDelay( pdp->handle, TRUE );
         pdp->common.Type = 1;//myTypeID;
         pdp->common.Close = Close;
         pdp->common.Write = TerminalTransmit;
         pdp->common.Read = NULL; // not needed cause data appears magically...
			pdp->common.flags.Data_Source = 1;

         //pdp->common.flags.KeepCR = TRUE;
         pdp->nState = 0;
         pdp->nParams = 0;
         pdp->ParamSet[0] = 0;
         pdp->flags.bNewLine = FALSE; // start with NEWLINE...
         SetNetworkLong( pdp->handle, CONTROLLING_SENTIENCE, (PTRSZVAL)ps );
         SetNetworkLong( pdp->handle, DATA_PATH, (PTRSZVAL)pdp );
      }
      else
      {
	   	Log( WIDE("Aborted during connect...") );
	   	LineRelease( pdp->Buffer );
	   	DestroyDataPath( (PDATAPATH)pdp );
	   	pdp = NULL;
      }
   }
   return (PDATAPATH)pdp;
}


//---------------------------------------------------------------------------

// UDP Receive message
static void UDPReceive( PCLIENT pc, TEXTSTR pBuffer, int nSize, SOCKADDR *sa )
{
   PMYDATAPATH pdp;
   pdp = (PMYDATAPATH)GetNetworkLong( pc, DATA_PATH );
   if( pdp )
	{
		PTEXT pSeg;
		if( pBuffer && nSize )
		{
		   pdp->Buffer->data.data[nSize] = 0;
   	  	pdp->Buffer->data.size = nSize;
   	  	if( sa )
   	  	{
				PTEXT pData = SegAppend( pSeg = SegCreate( sizeof( SOCKADDR ) )
	 						           , SegDuplicate( pdp->Buffer ) );

				MemCpy( GetText( pSeg ), sa, sizeof( SOCKADDR ) );
				pSeg->flags |= TF_ADDRESS;

		 		EnqueLink( &pdp->common.Input
		 					, pData );
			}
			else
		 		EnqueLink( &pdp->common.Input
	 						, SegDuplicate( pdp->Buffer ) );
		}
   		pdp->Buffer->data.size = 4096;
		ReadUDP( pc, pdp->Buffer->data.data, pdp->Buffer->data.size );
	}
}

//---------------------------------------------------------------------------

// UDP Send Message ( provide destination as part of the command... )

static int CPROC UDPTransmit( PDATAPATH pdpX )
{
   PMYDATAPATH pdp = (PMYDATAPATH)pdpX;
   PTEXT pLine, pWord, pSend, pAddr;
   // ugg...
   while( ( pLine = (PTEXT)DequeLink( &pdp->common.Output ) ) )
   {
      TEXTCHAR *pAddrData;
      pWord = pLine;
      if( ( pWord->flags & TF_ADDRESS ) == TF_ADDRESS )
      {
         // assume this is a valid network address... if nto well it won't work.
         pAddr = pWord;
         pAddrData = pAddr->data.data;
         pWord = NEXTLINE( pWord );
      }
      else
      {
         pAddr = NULL;
         pAddrData = NULL;
      }
      pSend = BuildLine( pWord );
      if( pSend )
      {
         if( !SendUDPEx( pdp->handle
                       , pSend->data.data
                       , pSend->data.size
                       , (SOCKADDR*)pAddrData ) )
         {
            //DECLTEXT( msg, WIDE("Send Failed...") );
         }
      }
      LineRelease( pSend );
		LineRelease( pLine );
   }
   return 0;
}

//---------------------------------------------------------------------------

// Provide close method for UDP DATAPATH...

//---------------------------------------------------------------------------

// udp
static PDATAPATH OnInitDevice(WIDE("udpserver"), WIDE("UDP datagram connection..."))( PDATAPATH *pChannel, PSENTIENT ps, PTEXT parameters )
{
   // blah - uhmm like stuff....
   PMYDATAPATH pdp;
   PTEXT pDestination;
   SOCKADDR *sa;

   pdp = CreateDataPath( pChannel, MYDATAPATH );
   pdp->Buffer = SegCreate( 4096 );

   if( !InitNetwork(ps) )
   {
      return NULL;
   }

   pDestination = GetParam( ps, &parameters );
   sa = CreateSockAddress( GetText( pDestination ), 23 );

   if( sa )
   {
      pdp->handle = ServeUDPAddr( sa, NULL, NULL );
      ReleaseAddress( sa );
      if( !pdp->handle )
      {
         DECLTEXT( msg, WIDE("Could not open UDP Receiver") );
         EnqueLink( &ps->Command->Output, &msg );
         Release( pdp->Buffer );
         DestroyDataPath( (PDATAPATH)pdp );
         pdp = NULL;
      }
      else
      {
         pdp->common.Type = 1;//myTypeID4;
         // think the default close will work for UDP stuff too?
         pdp->common.Close = Close;
         pdp->common.Write = UDPTransmit;
         pdp->common.Read = NULL; // not needed cause data appears magically...
         //pdp->common.flags.KeepCR = TRUE;
         SetNetworkLong( pdp->handle, CONTROLLING_SENTIENCE, (PTRSZVAL)ps );
         SetNetworkLong( pdp->handle, DATA_PATH, (PTRSZVAL)pdp );
         SetNetworkReadComplete( pdp->handle, (cReadComplete)UDPReceive );
         UDPReceive( pdp->handle, NULL, 0, NULL ); // intial read queue...
      }

   }

   return (PDATAPATH)pdp;
}

//---------------------------------------------------------------------------

static PDATAPATH OnInitDevice(WIDE("udp"), WIDE("UDP datagram connection..."))( PDATAPATH *pChannel, PSENTIENT ps, PTEXT parameters )
{
   PMYDATAPATH pdp;
   PTEXT pDestination, pSource;
   SOCKADDR *sa, *saFrom;

   pdp = CreateDataPath( pChannel, MYDATAPATH );
   pdp->Buffer = SegCreate( 4096 );

   if( !InitNetwork(ps) )
   {
      return NULL;
   }

   pDestination = GetParam( ps, &parameters );
   sa = CreateSockAddress( GetText( pDestination ), 23 );
   pSource = GetParam( ps, &parameters );
   if( pSource )
   {
	   saFrom = CreateSockAddress( GetText( pSource ), 23 );
   }
   else
      saFrom = NULL;
   if( sa )
   {
      pdp->handle = ConnectUDPAddr( saFrom, sa, NULL, NULL );
      ReleaseAddress( sa );
      if( saFrom )
         ReleaseAddress( saFrom );
      if( !pdp->handle )
      {
         DECLTEXT( msg, WIDE("Could not open UDP Socket...") );
         EnqueLink( &ps->Command->Output, &msg );
         Release( pdp->Buffer );
         DestroyDataPath( (PDATAPATH)pdp );
         pdp = NULL;
      }
      else
      {
         pdp->common.Type = 1;//myTypeID4;
         // think the default close will work for UDP stuff too?
         pdp->common.Close = Close;
         pdp->common.Write = UDPTransmit;
         pdp->common.Read = NULL; // not needed cause data appears magically...
         //pdp->common.flags.KeepCR = TRUE;
         SetNetworkLong( pdp->handle, CONTROLLING_SENTIENCE, (PTRSZVAL)ps );
         SetNetworkLong( pdp->handle, DATA_PATH, (PTRSZVAL)pdp );
         SetNetworkReadComplete( pdp->handle, (cReadComplete)UDPReceive );
         UDPReceive( pdp->handle, NULL, 0, NULL ); // intial read queue...
      }

   }

   return (PDATAPATH)pdp;
}

//---------------------------------------------------------------------------

static int HandleCommand( WIDE("Network"), WIDE("getaddr") , WIDE("Get network address from message..."))( PSENTIENT ps, PTEXT parameters )
{
   return 0;
}

//---------------------------------------------------------------------------

static int HandleCommand(WIDE("Network"), WIDE("getip") , WIDE("Get network address from message..."))( PSENTIENT ps, PTEXT parameters )   // input is %textaddr %outvar
{
   PTEXT var1, var2, save, pPort;
   save = parameters;
   var1 = GetParam( ps, &parameters );
   if( var1 == save )
   {
      DECLTEXT( msg, WIDE("Invalid variable reference...") );
      EnqueLink( &ps->Command->Output, &msg );
//      return 0;
   }
   else
	{
		while( var1->flags & TF_INDIRECT )
	      var1 = GetIndirect( var1 );
   }
   save = parameters;
   var2 = GetParam( ps, &parameters );
   if( var2 == save )
   {
      DECLTEXT( msg, WIDE("Invalid variable reference...") );
      EnqueLink( &ps->Command->Output, &msg );
      return 0;
   }

   if( var1 && var2 )
   {
      pPort = NULL;
      if( var1->flags & TF_BINARY )
      {
         SOCKADDR *sa;
         sa = (SOCKADDR*)GetText( var1 );
         if( sa->sa_family != AF_INET )
         {
            DECLTEXT( msg, WIDE("Binary data is not an INET address...") );
            EnqueLink( &ps->Command->Output, &msg );
            return 0;
         }
         else
         {
            pPort = SegCreate( 15 );
            pPort->data.size = snprintf( pPort->data.data, pPort->data.size*sizeof(TEXTCHAR), WIDE("%s")
												  , inet_ntoa( (
#ifdef __LINUX__
																	 *(struct in_addr*)&
#endif
																	 ((SOCKADDR_IN*)sa)->sin_addr) ) );
         }
      }
      else
      {
         TEXTCHAR *p, *port;
         p = GetText( var1 );
         if( ( port = strchr( p, ':' ) ) )
         {
            port++;
            pPort = SegCreateFromText( port );
         }
         else
         {
            DECLTEXT( msg, WIDE("Dunno how to interpret as an IP:PORT...") );
            EnqueLink( &ps->Command->Output, &msg );
         }
      }
      if( pPort )
      {
         LineRelease( GetIndirect( var2 ) );
         SetIndirect( var2, pPort );
      }
   }
   else
   {
      DECLTEXT( msg, WIDE("Parameters to GetIP are wrong... /getip ip:port %into") );
      EnqueLink( &ps->Command->Output, &msg );
      return 0;
   }
   return 0;
}

//---------------------------------------------------------------------------

static int HandleCommand(WIDE("Network"), WIDE("getport") , WIDE("Get network address from message..."))( PSENTIENT ps, PTEXT parameters ) // input is %textaddr %outvar
{
   PTEXT var1, var2, save, pPort;
   save = parameters;
   var1 = GetParam( ps, &parameters );
   if( var1 == save )
   {
      DECLTEXT( msg, WIDE("Invalid variable reference...") );
      EnqueLink( &ps->Command->Output, &msg );
//      return 0;
   }
   else
      var1 = GetIndirect( var1 );
   save = parameters;
   var2 = GetParam( ps, &parameters );
   if( var2 == save )
   {
      DECLTEXT( msg, WIDE("Invalid variable reference...") );
      EnqueLink( &ps->Command->Output, &msg );
      return 0;
   }
   if( var1 && var2 )
   {
      pPort = NULL;
      if( var1->flags & TF_BINARY )
      {
         SOCKADDR *sa;
         sa = (SOCKADDR*)GetText( var1 );
         if( sa->sa_family != AF_INET )
         {
            DECLTEXT( msg, WIDE("Binary data is not an INET address...") );
            EnqueLink( &ps->Command->Output, &msg );
            return 0;
         }
         else
         {
            pPort = SegCreate( 5 );
            pPort->data.size = snprintf( pPort->data.data, pPort->data.size*sizeof(TEXTCHAR), WIDE("%d")
                             , ntohs( *(unsigned short*)sa->sa_data ) );
         }
      }
      else
      {
         TEXTCHAR *p, *port;
         p = GetText( var1 );
         if( ( port = strchr( p, ':' ) ) )
         {
            port++;
            pPort = SegCreateFromText( port );
         }
         else
         {
            DECLTEXT( msg, WIDE("Dunno how to interpret as an IP:PORT...") );
            EnqueLink( &ps->Command->Output, &msg );
         }
      }
      if( pPort )
      {
         LineRelease( GetIndirect( var2 ) );
         SetIndirect( var2, pPort );
      }
   }
   else
   {
      DECLTEXT( msg, WIDE("Parameters to GetIP are wrong... /getip ip:port %into") );
      EnqueLink( &ps->Command->Output, &msg );
      return 0;
   }
   return 0;
}

//---------------------------------------------------------------------------

//static int CPROC BuildAddress( PSENTIENT ps, PTEXT parameters ) // input is %textvar %outvar
//{
//
//   return 0;
//}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

static int HandleCommand(WIDE("Network"), WIDE("whois"), WIDE("Perform whois query on listed names."))( PSENTIENT ps, PTEXT parameters )
{
	PTEXT temp;
#undef byOutput // ... yuck.
	PTEXT pOutput;
	PVARTEXT pvt = VarTextCreate();
	if( InitNetwork( ps ) )
	{
#ifndef __ANDROID__
		while( ( temp = GetParam( ps, &parameters ) ) )
		{
			DoWhois( GetText( temp ), NULL, pvt );
			//#endif
			pOutput = VarTextGet( pvt );
			//pOutput = SegCreateFromText( byOutput );
			{
				PTEXT partial = NULL, out;
				out = GatherLineEx( &partial, NULL, FALSE, FALSE, TRUE, pOutput );
				while( out )
				{
					PTEXT end;
					end = out;
					SetEnd( end );
					if( end )
						GetText( end )[GetTextSize(end)-1] = 0;
					EnqueLink( &ps->Command->Output, out );
					out = GatherLineEx( &partial, NULL, FALSE, FALSE, TRUE, NULL );
				}
				if( partial )
				{
					EnqueLink( &ps->Command->Output, partial );
				}
			}
			LineRelease( pOutput );
			//EnqueLink( &ps->Command->Output, pOutput );
		}
		VarTextDestroy( &pvt );
#endif
	}
	return 0;
}

static int HandleCommand(WIDE("Network"), WIDE("ping"), WIDE("Ping a network address..."))( PSENTIENT ps, PTEXT parameters )
{
   PTEXT temp;
   PVARTEXT pvt = VarTextCreate();
   if( InitNetwork(ps) )
   {
      	temp = GetParam( ps, &parameters );
#ifndef __ANDROID__
		DoPing( GetText( temp ), 0, 2500, 1, pvt, FALSE, NULL );
		temp = VarTextGet( pvt );
		{
			PTEXT partial = NULL, out;
			for( out = GatherLineEx( &partial, NULL, FALSE, FALSE, TRUE, temp );
				  out;
				  out = GatherLineEx( &partial, NULL, FALSE, FALSE, TRUE, NULL ) )
			{
				// GatherLine keeps carriage returns in the buffer, so we get rid of those before enquing.
				PTEXT end;
				end = out;
				SetEnd( end );
				if( end )
					GetText( end )[GetTextSize(end)-1] = 0;
				EnqueLink( &ps->Command->Output, out );
			}
			// the last partial line won't have a carriage return (because if it did, it wouldn't be partial)
			if( partial )
			{
				EnqueLink( &ps->Command->Output, partial );
			}
		}
#endif
		LineRelease( temp );
	}
	VarTextDestroy( &pvt );
	return 0;
}

static int HandleCommand(WIDE("Network"), WIDE("trace"), WIDE("Route trace a network address..."))( PSENTIENT ps, PTEXT parameters )
{
   PTEXT temp;
   PVARTEXT pvt = VarTextCreate();
   if( InitNetwork(ps) )
   {
      temp = GetParam( ps, &parameters );
#ifndef __ANDROID__
      DoPing( GetText( temp ), 32, 2500, 1, pvt, FALSE, NULL );
#endif
      temp = VarTextGet( pvt );
		{
         PTEXT partial = NULL, out;
         out = GatherLineEx( &partial, NULL, FALSE, FALSE, TRUE, temp );
         while( out )
         {
            	PTEXT end;
            	end = out;
            	SetEnd( end );
            	if( end )
	            	GetText( end )[GetTextSize(end)-1] = 0;
            EnqueLink( &ps->Command->Output, out );
            out = GatherLineEx( &partial, NULL, FALSE, FALSE, TRUE, NULL );
         }
         if( partial )
         {
            EnqueLink( &ps->Command->Output, partial );
         }
		}
		LineRelease( temp );
      //EnqueLink( &ps->Command->Output, temp );
   }
	VarTextDestroy( &pvt );
	return 0;
}

PSENTIENT psScanning;
_32 dwThreadCount;
TEXTCHAR *pAddr;
_16 wPortFrom = 1
   , wPortNow = 0
   , wPortTo = 2048;
_32 dwMaxThreads = 100;
//_32 dwThreadCount;
PTEXT pHolder;

// perhaps consider keeping the connection alive
// long enough to get the first read from the socket...

static PTRSZVAL CPROC ThreadProc( PTHREAD pThread )
{
   PTRSZVAL dwPort = GetThreadParam( pThread );
   PCLIENT pc;
   pc = OpenTCPClientEx( pAddr, (_16)dwPort, NULL, NULL, NULL );
   if( pc )
   {
      PTEXT pOutput;
      TEXTCHAR byOutput[256];
      snprintf( byOutput, sizeof( byOutput ), WIDE("Connected to address at port: %ld"), dwPort );
      pOutput = SegCreateFromText( byOutput );
      EnqueLink( &psScanning->Command->Output, pOutput );
      RemoveClient( pc );
   }
   LockedDecrement( &dwThreadCount );
   return 0;
}

PTRSZVAL CPROC PortScanner( PTHREAD thread )
{
   for( wPortNow = wPortFrom; wPortNow < wPortTo; wPortNow++ )
   {
      if( dwThreadCount >= dwMaxThreads )
         while( dwThreadCount > (dwMaxThreads/2) )
            Sleep(20); // wait for some completes...

      if( dwThreadCount < dwMaxThreads )
      {
			LockedIncrement( &dwThreadCount ); // mark NOW.
         ThreadTo( ThreadProc, wPortNow );
      }

   }
   while( dwThreadCount ) // wiat for all threads to finish...
      Sleep(5);
   {
      PTEXT pOutput;
      TEXTCHAR byOutput[256];
      snprintf( byOutput, sizeof( byOutput ), WIDE("Checked ports %d - %d"), wPortFrom, wPortNow );
      pOutput = SegCreateFromText( byOutput );
      EnqueLink( &psScanning->Command->Output, pOutput );
   }
   LineRelease( pHolder );
   psScanning = 0;
   return 0;
}

static int HandleCommand(WIDE("Network"), WIDE("portscan"), WIDE("scan first 2000 ports at an address..."))( PSENTIENT ps, PTEXT parameters )
{
   PTEXT temp;
   PSENTIENT psOld;
   if( b95 )
      dwMaxThreads = 10;
   else
      dwMaxThreads = 100;
   if( psScanning )
   {
      PTEXT pOutput;
      TEXTCHAR byOutput[256];
      snprintf( byOutput, sizeof( byOutput ), WIDE("Port scan running to %s ports %d - %d now on %d"),
                  pAddr, wPortFrom, wPortTo, wPortNow );
      pOutput = SegCreateFromText( byOutput );
      EnqueLink( &psScanning->Command->Output, pOutput );
      return 0;
   }
   do
   {
      while( psScanning )
         Sleep(0);
      psOld = (PSENTIENT)LockedExchangePtrSzVal( &psScanning, ps );
   }
   while( psOld );


   if( InitNetwork( ps ) )
   {
      temp = GetParam( ps, &parameters );
      if( temp )
      {
         pHolder = SegDuplicate( temp );
			pAddr = GetText( pHolder );
         ThreadTo( PortScanner, 0 );
      }
   }
   return 0;
}


static PTEXT DeviceVolatileVariableGet( WIDE("net object"), WIDE("client_ip"), WIDE("current client side IP") )( PENTITY pe, PTEXT *ppLastValue )
//PTEXT GetClientIP( PTRSZVAL psv, struct entity_tag *pe, PTEXT *ppLastValue )
{
   PCLIENT pc = (PCLIENT)GetLink( &pe->pPlugin, iNetObject );
	static SOCKADDR_IN saSrc;
   TEXTCHAR *addr;
	saSrc.sin_addr.s_addr = (_32)GetNetworkLong( pc, GNL_IP );
	addr = DupCharToText( inet_ntoa( *(struct in_addr*)&saSrc.sin_addr ) );
	if( *ppLastValue )
		LineRelease( *ppLastValue );
	*ppLastValue = SegCreateFromText( addr );
	return *ppLastValue;
}

static PTEXT DeviceVolatileVariableGet( WIDE("net object"), WIDE("server_ip"), WIDE("current remote side IP") )( PENTITY pe, PTEXT *ppLastValue )
//PTEXT GetServerIP( PTRSZVAL psv, struct entity_tag *pe, PTEXT *ppLastValue )
{
   PCLIENT pc = (PCLIENT)GetLink( &pe->pPlugin, iNetObject );
	static SOCKADDR_IN saSrc;
   TEXTCHAR *addr;
	saSrc.sin_addr.s_addr = (_32)GetNetworkLong( pc, GNL_MYIP );
	addr = DupCharToText( inet_ntoa( *(struct in_addr*)&saSrc.sin_addr ) );
	if( *ppLastValue )
		LineRelease( *ppLastValue );
	*ppLastValue = SegCreateFromText( addr );
	return *ppLastValue;
}

PUBLIC( TEXTCHAR *, RegisterRoutines )( void )
{
	iNetObject = RegisterExtension( WIDE("net object") );
	return DekVersion;
}

PUBLIC( void, UnloadPlugin )( void ) // this routine is called when /unload is invoked
{
//   UnregisterRoutine( WIDE("send") );
   UnregisterRoutine( WIDE("getaddr") );
   UnregisterRoutine( WIDE("getip") );
   UnregisterRoutine( WIDE("getport") );
   UnregisterRoutine( WIDE("ping") );
   UnregisterRoutine( WIDE("trace") );
   UnregisterRoutine( WIDE("portscan") );
   UnregisterRoutine( WIDE("whois") );
   UnregisterRoutine( WIDE("ReadData") );
   UnregisterRoutine( WIDE("SendData") );
   UnregisterRoutine( WIDE("HTTP") );
   UnregisterDevice( WIDE("tcp") );
   UnregisterDevice( WIDE("udp") );
   UnregisterDevice( WIDE("tcpserver") );
   UnregisterDevice( WIDE("udpserver") );
   NetworkQuit();
}


// $Log: term.c,v $
// Revision 1.45  2005/08/08 09:33:20  d3x0r
// Define some ON behaviors for easier event definition.  Fix idling for sentient and datapath attached to socket.
//
// Revision 1.44  2005/04/15 07:24:36  d3x0r
// Fixed owner of datapath on cloned device.  Don't close the default system path - we expect it so we
//
// Revision 1.43  2005/02/21 12:09:00  d3x0r
// Okay so finally we should make everything CPROC type pointers... have for a long time decided to avoid this, but for compatibility with other libraries this really needs to be done.  Increase version number to 2.4
//
// Revision 1.42  2005/01/27 07:30:52  d3x0r
// Linux cleaned.
//
// Revision 1.41  2005/01/27 07:29:32  d3x0r
// Linux cleaned.
//
// Revision 1.40  2005/01/19 08:55:46  d3x0r
// Oh - those missing symbols - conflicts within its own shared library with static declarations
//
// Revision 1.39  2005/01/18 02:47:02  d3x0r
// Mods to protect symbols from overwrites.... (declare much things static, rename others)
//
// Revision 1.38  2004/12/14 23:40:15  d3x0r
// Minor tweaks to remove logging messages, also streamlined some redundant updates
//
// Revision 1.37  2004/12/06 10:51:39  d3x0r
// Silly fix to closing data channels, I really like this hyper-thread thing!
//
// Revision 1.36  2004/09/27 16:06:57  d3x0r
// Checkpoint - all seems well.
//
// Revision 1.35  2004/05/04 07:30:27  d3x0r
// Checkpoint for everything else.
//
// Revision 1.34  2004/01/20 07:32:38  d3x0r
// Fix some server issues
//
// Revision 1.33  2003/10/26 12:37:45  panther
// Fix some typecasting, Update to newest scheduling
//
// Revision 1.32  2003/09/28 23:54:19  panther
// Updates to new client only network close.  Fix to portal script
//
// Revision 1.31  2003/08/02 17:39:15  panther
// Implement some of the behavior methods.
//
// Revision 1.30  2003/07/28 09:07:39  panther
// Fix makefiles, fix cprocs on netlib interfaces... fix a couple badly formed functions
//
// Revision 1.29  2003/04/20 01:20:13  panther
// Updates and fixes to window-like console.  History, window logic
//
// Revision 1.28  2003/04/13 22:28:51  panther
// Seperate computation of lines and rendering... mark/copy works (wincon)
//
// Revision 1.27  2003/04/12 20:51:19  panther
// Updates for new window handling module - macro usage easer...
//
// Revision 1.26  2003/03/31 14:46:48  panther
// Handle binary output packets
//
// Revision 1.25  2003/03/26 07:22:57  panther
// update for multiple end of line buildline
//
// Revision 1.24  2003/03/25 08:59:03  panther
// Added CVS logging
//
