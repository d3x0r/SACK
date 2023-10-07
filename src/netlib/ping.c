//
// PING.C -- Ping program using ICMP and RAW Sockets
//
//#ifndef __LINUX__

#define LIBRARY_DEF
#include <stdhdrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sharemem.h> // lockedincrement, lockeddecrement.
#include <timers.h>

#include <network.h>
#include "ping.h"

#ifdef __LINUX__
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

// included not for pclient structure... but for OpenSocket() definition.
#include "netstruc.h"

SACK_NETWORK_NAMESPACE

// Internal Functions
void Ping(TEXTSTR pstrHost, int maxTTL);
void ReportError(PVARTEXT pInto, CTEXTSTR pstrFrom);
int  WaitForEchoReply(SOCKET s, uint32_t dwTime);
uint16_t in_cksum(uint16_t *addr, int len);

// ICMP Echo Request/Reply functions
int		SendEchoRequest(PVARTEXT, SOCKET, struct sockaddr_in*);
int   	RecvEchoReply( PVARTEXT, SOCKET, struct sockaddr_in*, uint8_t *);

#define MAX_HOPS     128 
#define MAX_NAME_LEN 255
typedef struct HopEntry_tag{
   uint32_t dwIP;                 // IP from returned
   uint32_t dwMinTime;
   uint32_t dwMaxTime;
   uint32_t dwAvgTime;
   uint32_t dwDropped;
//   uint32_t dwTime;
   TEXTSTR  pName;  // bRDNS resulting...
   int TTL;                    // returned TTL from destination...
} HOPENT, *PHOPENT;

volatile uint32_t dwThreadsActive;

uintptr_t CPROC RDNSThread( PTHREAD pThread )
{
   PHOPENT pHopEnt = (PHOPENT)GetThreadParam( pThread );
   struct hostent *phe;

   phe = gethostbyaddr( (char*)&pHopEnt->dwIP, 4, AF_INET );
	if( phe )
#ifdef _UNICODE
		pHopEnt->pName = CharWConvert( phe->h_name );
#else
		pHopEnt->pName = StrDup( phe->h_name );
#endif

   LockedDecrement( &dwThreadsActive );
   /*
#ifdef _WIN32
   ExitThread( dwThreadsActive );
#else
#endif
   */
   return 0;
}

// Ping()
// Calls SendEchoRequest() and
// RecvEchoReply() and prints results


static LOGICAL DoPingExx( CTEXTSTR pstrHost
								,int maxTTL
								,uint32_t dwTime
								,int nCount
								,PVARTEXT pvtResult
								,LOGICAL bRDNS
								,void (*ResultCallback)( uint32_t dwIP, CTEXTSTR name, int min, int max, int avg, int drop, int hops )
								,void (*ResultCallbackEx)( uintptr_t psv, uint32_t dwIP, CTEXTSTR name, int min, int max, int avg, int drop, int hops )
								, uintptr_t psvUser )
{
	SOCKET	  rawSocket;
	struct hostent *lpHost;
#ifdef __LINUX__
	struct win_sockaddr_in saDest;
	struct win_sockaddr_in saSrc;
#else
	struct sockaddr_in saDest;
	struct sockaddr_in saSrc;
#endif
	uint64_t	     dwTimeSent;
	uint8_t     cTTL = 0;
	int        nLoop;
	int        nRet, nResult = 0;
	int        i;
	uint64_t      MinTime, MaxTime, AvgTime;
	uint32_t   Dropped;

	uint32_t     dwIP,_dwIP;
	static LOGICAL csInit;
   static CRITICALSECTION cs;
   static  HOPENT    Entry[MAX_HOPS];
   static  int       nEntry = 0;
   //char     *pResultStart;
   //pResultStart = pResult;
   if( !csInit )
   {
	   InitializeCriticalSec( &cs );
	   csInit = 1;
   }
   if( maxTTL < 0 )
   {
       if( pvtResult )
           vtprintf( pvtResult, "TTL Parameter Error ( <0 )\n" );
       return 0;
   }

   if( nCount < 0 )
   {
       if( pvtResult )
           vtprintf( pvtResult, "Count Parameter Error ( <0 )\n" );
       return 0;
   }

   if( maxTTL > MAX_HOPS ) 
       maxTTL = MAX_HOPS;

   if( maxTTL )
   {
       if( nCount > 10 ) // limit hit count on tracert....
           nCount = 10;
   }

   // Lookup host
#ifdef __LINUX__
   if( !inet_aton( pstrHost, (struct in_addr*)&dwIP ) )
#else
#  ifdef _UNICODE
	{
      char *tmp = WcharConvert( pstrHost );
		dwIP = inet_addr( tmp );
      Deallocate( char *, tmp );
	}
#  else
	dwIP = inet_addr( pstrHost );
#  endif
   if( dwIP == INADDR_NONE )
#endif
   {
       if( pvtResult )
			 vtprintf( pvtResult, "host was not numeric\n" );
#ifdef _UNICODE
		 {
          char *tmpname = WcharConvert( pstrHost );
			 lpHost = gethostbyname(tmpname);
			 Deallocate( char *, tmpname );
		 }
#else
		 lpHost = gethostbyname(pstrHost);
#endif
       if (lpHost)
		 {
#ifndef h_addr
#define h_addr h_addr_list[0]
#define H_ADDR_DEFINED
#endif
           dwIP = *((uint32_t *) lpHost->h_addr);
#ifdef H_ADDR_DEFINED
#  undef H_ADDR_DEFINED
#  undef h_addr
#endif
       }
       else
       {
           if( pvtResult )
               vtprintf( pvtResult, "(1)Host does not exist.(%s)\n", pstrHost );
       }
   }

   if( dwIP == 0xFFFFFFFF )
   {
       if( pvtResult )
           vtprintf( pvtResult, "Host does not exist.(%s)\n", pstrHost );
       return 0;
   }
   nEntry = 0;
   // Create a Raw socket

#ifdef WIN32
	rawSocket = INVALID_SOCKET;//OpenSocket(TRUE,FALSE, TRUE, 0);
	if( rawSocket == INVALID_SOCKET ) //-V547
	{
      //lprintf( "Bad 'smart' open.. fallback..." );
		rawSocket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	}
#else
	rawSocket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
#endif
   if (rawSocket == SOCKET_ERROR)
	{
      if( pvtResult )
			vtprintf( pvtResult, "Uhmm bad things happened for sockraw!\n" );
		else
			lprintf( "Uhmm bad things happened for sockraw!\n" );
#ifdef WIN32
		rawSocket = OpenSocket( TRUE, FALSE, TRUE, 0 );
		if( rawSocket == SOCKET_ERROR )
		{
       if( WSAGetLastError() == 10013 )
       {
           if( pvtResult )
               vtprintf( pvtResult, "User is not an administrator, cannot create a RAW socket.\n"
                        "Unable to override this.\n");
           return FALSE;
       }
       else
       {
           if( pvtResult )
               ReportError( pvtResult, "socket()");
       }
		 return FALSE;
		}
	{
		// promiscuous
		//WSAIoctl( rawSocket, SIO_RCV_ALL, NULL, 0, NULL, cbOutBuffer, lpcbBytesReturned, NULL, NULL );
	}
#endif
   }

   // Setup destination socket address
   saDest.sin_addr.S_un.S_addr = dwIP;
   saDest.sin_family = AF_INET;
   saDest.sin_port = 0;

   //vtprintf( pvtResult, "Version 1.0   ADA Software Developers, Inc.  Copyright 1999.\n" );
   // Tell the user what we're doing
   if( pvtResult )
   {
       vtprintf( pvtResult, "Pinging %s [%s] with %d bytes of data:\n",
                           pstrHost,
                           inet_ntoa(*(struct in_addr*)&saDest.sin_addr),
                           REQ_DATASIZE);

       if( maxTTL )
       {
           vtprintf( pvtResult, "Hop  Size Min(us) Max(us) Avg(us) Drop Hops? IP              Name\n" );
           vtprintf( pvtResult, "--- ----- ------- ------- ------- ---- ----- --------------- -------->\n" );
       }
       else
       {
           vtprintf( pvtResult, "Size  Min(us) Max(us) Avg(us) Drop Hops? IP              Name\n" );
           vtprintf( pvtResult, "----- ------- ------- ------- ---- ----- --------------- -------->\n" );
       }
   }
	EnterCriticalSec( &cs );

	// Ping multiple times
   _dwIP = 0;
   for( i = 0; i <= maxTTL && dwIP != _dwIP; i++ )
   {
      
      if( maxTTL )
      {
         if( !i )       // skip TTL 0...
            continue;
#ifndef IP_TTL
#if defined (__LINUX__)
#define IP_TTL 2
#else
#define IP_TTL 4
#endif
#endif
         setsockopt( rawSocket, IPPROTO_IP, IP_TTL, (const char*)&i, sizeof(int));
      }

      MinTime = (uint64_t)-1; // longer than EVER
      MaxTime = 0;
      AvgTime = 0;
      Dropped = 0;

      for(nLoop = 0; nLoop < nCount; nLoop++) 
      {
         // Send ICMP echo request
         dwTimeSent = GetCPUTick();
         if( SendEchoRequest(pvtResult, rawSocket, (struct sockaddr_in*)&saDest) <= 0)
         {
            closesocket( rawSocket );
				LeaveCriticalSec( &cs );
            return FALSE; // failed on send
         }

         nRet = WaitForEchoReply(rawSocket, dwTime);
         if (nRet == SOCKET_ERROR)
         {
             if( pvtResult )
                 ReportError( pvtResult, "select()");
             goto LoopBreakpoint;  // abort abort FAIL
         }
         else if (!nRet)
         {
            //AvgTime += dwTime;
            Dropped++; 
         }
         else
         {
            uint64_t dwTimeNow;
            dwTimeNow = GetCPUTick() - dwTimeSent;
            AvgTime += dwTimeNow;

            if( dwTimeNow > MaxTime )
               MaxTime = dwTimeNow;

            if( dwTimeNow < MinTime )
               MinTime = dwTimeNow;

            if( !RecvEchoReply( pvtResult, rawSocket, (struct sockaddr_in*)&saSrc, &cTTL) )
            {
                if( pvtResult )
                    ReportError( pvtResult, "recv()" );
				//VarTextEmpty( &pvtResult );
                //pResult = pResultStart;
                closesocket( rawSocket );
                LeaveCriticalSec( &cs );
                return FALSE;
            }
         }
      }      
      if( MinTime > MaxTime ) // no responces....
      {
         MinTime = 0;
         MaxTime = 0;
         Entry[nEntry].dwIP = 0;
      }
      else
      {
         Entry[nEntry].dwIP = _dwIP = saSrc.sin_addr.S_un.S_addr;
      }

      Entry[nEntry].TTL  = cTTL;
      Entry[nEntry].dwMaxTime = ConvertTickToMicrosecond( MaxTime );
      Entry[nEntry].dwMinTime = ConvertTickToMicrosecond( MinTime );
      if( nCount - Dropped )
          Entry[nEntry].dwAvgTime = ConvertTickToMicrosecond( AvgTime ) / ( nCount - Dropped );
      else
          Entry[nEntry].dwAvgTime = 0;
      Entry[nEntry].dwDropped = Dropped;
      Entry[nEntry].pName = 0;
      nEntry++;
   }
LoopBreakpoint:
	nRet = closesocket(rawSocket);
	if (nRet == SOCKET_ERROR)
		if( pvtResult )
			ReportError( pvtResult, "closesocket()");

   if( bRDNS )
   {
      for( i = 0; i < nEntry; i++ )
      {
         if( Entry[i].dwIP )
         {
            //uint32_t dwID;
            LockedIncrement( &dwThreadsActive );
            ThreadTo( RDNSThread, (uintptr_t)(Entry+i) );
            /*
#ifdef _WIN32
            CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)RDNSThread, Entry+i, 0, &dwID );
#else
            pthread_create( &dwID, NULL, (void*(*)(void*))RDNSThread, (void*)(Entry+i) );
#endif
            */
         }
      }

      while( dwThreadsActive ) 
         Sleep(0);
   }

   for( i = 0; i < nEntry; i++ )
   {
      char *pIPBuf;
      if( Entry[i].dwIP )  
      {
         saSrc.sin_addr.S_un.S_addr = Entry[i].dwIP;
         pIPBuf = inet_ntoa( *(struct in_addr*)&saSrc.sin_addr );
         nResult = TRUE;
      }
      else        
      {
         pIPBuf = (char*)"No Response.";
         Entry[i].pName = 0;
      }
      if( maxTTL )
      {
          TEXTCHAR Min[8], Max[8], Avg[8];
          if( ResultCallback )
              ResultCallback( Entry[i].dwIP
                             , Entry[i].pName
                             , Entry[i].dwMinTime
                             , Entry[i].dwMaxTime
                             , Entry[i].dwAvgTime
                             , Entry[i].dwDropped
                             , 256 - Entry[i].TTL );
          else if( ResultCallbackEx )
				 ResultCallbackEx( psvUser
									  , Entry[i].dwIP
                             , Entry[i].pName
                             , Entry[i].dwMinTime
                             , Entry[i].dwMaxTime
                             , Entry[i].dwAvgTime
                             , Entry[i].dwDropped
                             , 256 - Entry[i].TTL );
          if( pvtResult )
			 {
#ifdef _UNICODE
				 TEXTSTR tmp;
#endif
				 if( Entry[i].dwAvgTime )
					 tnprintf( Avg, sizeof( Avg ),"%7" _32f, Entry[i].dwAvgTime );
				 else
					 tnprintf( Avg, sizeof( Avg ),"    ***" );
				 if( Entry[i].dwMinTime )
					 tnprintf( Min,sizeof(Min), "%7" _32f, Entry[i].dwMinTime );
				 else
					 tnprintf( Min, sizeof( Min ),"    ***" );
				 if( Entry[i].dwMaxTime )
					 tnprintf( Max, sizeof( Max), "%7" _32f, Entry[i].dwMaxTime );
				 else
					 tnprintf( Max, sizeof( Max ),"    ***" );

				 vtprintf( pvtResult, "%3d %5d %s %s %s %4" _32f " %5d %15.15s %s\n",
							 i + 1,
							 REQ_DATASIZE,
							 Min,
							 Max,
							 Avg,
							 Entry[i].dwDropped,
							 256 - Entry[i].TTL,
#ifdef _UNICODE
							 tmp = CharWConvert( pIPBuf ),
#else
							 pIPBuf,
#endif
							 Entry[i].pName
							);
#ifdef _UNICODE
				 Deallocate( TEXTSTR, tmp );
#endif
			 }
		}
		else
		{
			if( ResultCallback )
				ResultCallback( Entry[i].dwIP
									, Entry[i].pName
									, Entry[i].dwMinTime
									, Entry[i].dwMaxTime
									, Entry[i].dwAvgTime
									, Entry[i].dwDropped
									, 256 - Entry[i].TTL );
			else if( ResultCallbackEx )
				ResultCallbackEx( psvUser
									 , Entry[i].dwIP
									 , Entry[i].pName
									 , Entry[i].dwMinTime
									 , Entry[i].dwMaxTime
									 , Entry[i].dwAvgTime
									 , Entry[i].dwDropped
									 , 256 - Entry[i].TTL );
			if( pvtResult )
				vtprintf( pvtResult, "%5d %7" _32f " %7" _32f " %7" _32f " %4" _32f " %5d %15.15s %s\n",
										 REQ_DATASIZE,
										 Entry[i].dwMinTime,
										 Entry[i].dwMaxTime,
										 Entry[i].dwAvgTime,
										 Entry[i].dwDropped,
										 256 - Entry[i].TTL,
										 pIPBuf,
										 Entry[i].pName
										);
		}
	}
	LeaveCriticalSec( &cs );
   return nResult;
}

NETWORK_PROC( LOGICAL, DoPing )( CTEXTSTR pstrHost,
             int maxTTL, 
             uint32_t dwTime, 
             int nCount, 
             PVARTEXT pvtResult, 
             LOGICAL bRDNS, 
             void (*ResultCallback)( uint32_t dwIP, CTEXTSTR name, int min, int max, int avg, int drop, int hops ) )
{
   return DoPingExx( pstrHost, maxTTL, dwTime, nCount, pvtResult, bRDNS, ResultCallback, NULL, 0 );
}

NETWORK_PROC( LOGICAL, DoPingEx )( CTEXTSTR pstrHost,
											 int maxTTL,
											 uint32_t dwTime,
											 int nCount,
											 PVARTEXT pvtResult,
											 LOGICAL bRDNS,
											 void (*ResultCallback)( uintptr_t psv, uint32_t dwIP, CTEXTSTR name, int min, int max, int avg, int drop, int hops )
											, uintptr_t psvUser )
{
   return DoPingExx( pstrHost, maxTTL, dwTime, nCount, pvtResult, bRDNS, NULL, ResultCallback, psvUser );
}
// SendEchoRequest()
// Fill in echo request header
// and send to destination
int SendEchoRequest(PVARTEXT pvtResult, SOCKET s,struct sockaddr_in *lpstToAddr)
{
	static ECHOREQUEST echoReq;
	static int nId = 1;
	static int nSeq = 1;
	int nRet;

	// Fill in echo request
	echoReq.icmpHdr.Type		= ICMP_ECHOREQ;
	echoReq.icmpHdr.Code		= 0;
	echoReq.icmpHdr.Checksum	= 0;
	echoReq.icmpHdr.ID		= (uint16_t)(nId++);
	echoReq.icmpHdr.Seq		= (uint16_t)(nSeq++);

	// Fill in some data to send
	for (nRet = 0; nRet < REQ_DATASIZE; nRet++)
		echoReq.cData[nRet] = (char)(' '+nRet);

	// Save tick count when sent
	echoReq.dwTime				= GetCPUTick();

	// Put data in packet and compute checksum
	echoReq.icmpHdr.Checksum = in_cksum((uint16_t *)&echoReq, sizeof(ECHOREQUEST));

	// Send the echo request  								  
	nRet = sendto(s,						/* socket */
				 (char*)&echoReq,			/* buffer */
				 sizeof(ECHOREQUEST),
				 0,							/* flags */
				 (SOCKADDR*)lpstToAddr, /* destination */
				 sizeof(SOCKADDR));   /* address length */

	if (nRet == SOCKET_ERROR) 
		if( pvtResult )
			ReportError( pvtResult, "sendto()");
	return (nRet);
}


// RecvEchoReply()
// Receive incoming data
// and parse out fields
int RecvEchoReply(PVARTEXT pvtResult, SOCKET s, struct sockaddr_in *lpsaFrom, uint8_t *pTTL)
{
	ECHOREPLY echoReply;
	int nRet;
#ifdef __LINUX__
	socklen_t
#else
   int
#endif
		nAddrLen = sizeof(struct sockaddr_in);

	// Receive the echo reply	
	nRet = recvfrom(s,					// socket
					(char*)&echoReply,	// buffer
					sizeof(ECHOREPLY),	// size of buffer
					0,					// flags
					(SOCKADDR*)lpsaFrom,	// From address
					&nAddrLen);			// pointer to address len

	// Check return value
	if (nRet == SOCKET_ERROR) 
	{
		if( pvtResult )
			ReportError( pvtResult, "recvfrom()");
      return 0;
	}
// if( echoReply.ipHdr.
	// return time sent and IP TTL
	*pTTL = echoReply.ipHdr.TTL;
	return 1;   		
}

// What happened?
void ReportError(PVARTEXT pInto, CTEXTSTR pWhere)
{
	vtprintf( pInto, "\n%s error: %d\n",
                            pWhere, WSAGetLastError());
}


// WaitForEchoReply()
// Use select() to determine when
// data is waiting to be read
int WaitForEchoReply(SOCKET s, uint32_t dwTime)
{
	struct timeval Timeout;
	fd_set readfds;
	FD_ZERO( &readfds );
	FD_SET( s, &readfds );
	Timeout.tv_sec = dwTime / 1000; // longer than a second is too long
	Timeout.tv_usec = ( dwTime % 1000 ) * 1000;

	return(select(1, &readfds, NULL, NULL, &Timeout));
}


//
// Mike Muuss' in_cksum() function
// and his comments from the original
// ping program
//
// * Author -
// *	Mike Muuss
// *	U. S. Army Ballistic Research Laboratory
// *	December, 1983

/*
 *			I N _ C K S U M
 *
 * Checksum routine for Internet Protocol family headers (C Version)
 *
 */
uint16_t in_cksum(uint16_t *addr, int len)
{
	register int nleft = len;
	register uint16_t *w = addr;
	register uint16_t answer;
	register int sum = 0;

	/*
	 *  Our algorithm is simple, using a 32 bit accumulator (sum),
	 *  we add sequential 16 bit words to it, and at the end, fold
	 *  back all the carry bits from the top 16 bits into the lower
	 *  16 bits.
	 */
	while( nleft > 1 )  {
		sum += *w++;
		nleft -= 2;
	}

	/* mop up an odd byte, if necessary */
	if( nleft == 1 ) {
		uint16_t	u = 0;

		*(uint8_t *)(&u) = *(uint8_t *)w ;
		sum += u;
	}

	/*
	 * add back carry outs from top 16 bits to low 16 bits
	 */
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = (uint16_t)(~sum);				/* truncate to 16 bits */
	return (answer);
}
SACK_NETWORK_NAMESPACE_END

