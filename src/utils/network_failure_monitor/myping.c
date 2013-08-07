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
#include "myping.h"

#ifdef __UNIX__
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

// included not for pclient structure... but for OpenSocket() definition.
#include "../../netlib/netstruc.h"


// Internal Functions
void Ping(TEXTSTR pstrHost, int maxTTL);
TEXTSTR ReportError(TEXTSTR pInto, TEXTSTR pstrFrom);
int  WaitForEchoReply(SOCKET s, _32 dwTime);
u_short in_cksum(u_short *addr, int len);

// ICMP Echo Request/Reply functions
int		SendEchoRequest(TEXTSTR, SOCKET, SOCKADDR*);
int   	RecvEchoReply( TEXTSTR, SOCKET, SOCKADDR_IN*, u_char *);

#define MAX_HOPS     128 
#define MAX_NAME_LEN 255
typedef struct HopEntry_tag{
   _32 dwIP;                 // IP from returned
   _32 dwMinTime;
   _32 dwMaxTime;
   _32 dwAvgTime;
   _32 dwDropped;
//   _32 dwTime;
   TEXTCHAR  pName[MAX_NAME_LEN];  // bRDNS resulting...
   int TTL;                    // returned TTL from destination...
} HOPENT, *PHOPENT;

_32 dwThreadsActive;

PTRSZVAL CPROC RDNSThread( PTHREAD pThread )
{
   PHOPENT pHopEnt = (PHOPENT)GetThreadParam( pThread );
   struct hostent *phe;

   phe = gethostbyaddr( (char*)&pHopEnt->dwIP, 4, AF_INET );
   if( phe )
      strcpy( pHopEnt->pName,phe->h_name );

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
static SOCKADDR *sa;
SOCKET	  rawSocket;


void InitPing( CTEXTSTR address )
{
   NetworkStart();
	rawSocket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
   if (rawSocket == SOCKET_ERROR)
	{
		lprintf( WIDE("Uhmm bad things happened for sockraw!\n") );
       if( WSAGetLastError() == 10013 )
       {
			 lprintf(  WIDE("User is not an administrator, cannot create a RAW socket.\n")
						WIDE("Unable to override this.\n"));
           return;
       }
       else
       {
       }
       return;
   }
		{
         int i = 1;
#ifndef IP_TTL
#if defined (__LINUX__)
#define IP_TTL 2
#else
#define IP_TTL	7
#endif
#endif
         setsockopt( rawSocket, IPPROTO_IP, IP_TTL, (const char*)&i, sizeof(int));
		}
	sa = CreateSockAddress( address, 0 );



}


int MyDoPing( void )
{
	SOCKADDR_IN saDest;
	SOCKADDR_IN saSrc;
	u_char     cTTL;
	int        nLoop;
	int        nRet;
	int        i;
	_32   Dropped;

   static CRITICALSECTION cs;

   if (rawSocket == SOCKET_ERROR)
	{
       return FALSE;
   }

   // Tell the user what we're doing
	EnterCriticalSec( &cs );

	// Ping multiple times
   {
      Dropped = 0;

	retry:
      {
         // Send ICMP echo request
         //dwTimeSent = GetCPUTick();
         if( SendEchoRequest(NULL, rawSocket, sa) <= 0)
         {
               lprintf( "Fatal error." );
				LeaveCriticalSec( &cs );
            return FALSE; // failed on send
         }

         nRet = WaitForEchoReply(rawSocket, 100 );
         if (nRet == SOCKET_ERROR)
			{
            lprintf( "Fatal error." );
             goto LoopBreakpoint;  // abort abort FAIL
         }
         else if (!nRet)
			{
            //lprintf( "Drop." );
            //AvgTime += dwTime;
            Dropped++; 
         }
         else
         {
            //_64 dwTimeNow;
            //dwTimeNow = GetCPUTick() - dwTimeSent;
				//lprintf( "Suceess getting a packet?" );
            switch( RecvEchoReply( NULL, rawSocket, &saSrc, &cTTL) )
				{
				case 0:
				default:
					lprintf( "Unknown Packet or... Fatal error." );
					LeaveCriticalSec( &cs );
					return 0;
				case 1:
					// success.
					break;
				case 2:
					// host unreachable.  Eat this, and try again. (happens every 3 pings)
					goto retry;
					break;
            }
         }
		}
   }
LoopBreakpoint:

	LeaveCriticalSec( &cs );
   return !Dropped;
}

//
// Ping.h
//

#pragma pack(1)

#define ICMP_ECHOREPLY	0
#define ICMP_ECHOREQ	8

// IP Header -- RFC 791
typedef struct tagIPHDR
{
	u_char  VIHL;			// Version and IHL
	u_char	TOS;			// Type Of Service
	short	TotLen;			// Total Length
	short	ID;				// Identification
	short	FlagOff;		// Flags and Fragment Offset
	u_char	TTL;			// Time To Live
	u_char	Protocol;		// Protocol
	u_short	Checksum;		// Checksum
	struct	in_addr iaSrc;	// Internet Address - Source
	struct	in_addr iaDst;	// Internet Address - Destination
}IPHDR, *PIPHDR;


// ICMP Header - RFC 792
typedef struct tagICMPHDR
{
	u_char	Type;			// Type
	u_char	Code;			// Code
	u_short	Checksum;		// Checksum
	u_short	ID;				// Identification
	u_short	Seq;			// Sequence
	char	Data;			// Data
}ICMPHDR, *PICMPHDR;


#define REQ_DATASIZE 32		// Echo Request Data size

// ICMP Echo Request
typedef struct tagECHOREQUEST
{
	ICMPHDR icmpHdr;
	_64		dwTime;
	char	cData[REQ_DATASIZE];
}ECHOREQUEST, *PECHOREQUEST;


// ICMP Echo Reply
typedef struct tagECHOREPLY
{
	IPHDR	ipHdr;
	ECHOREQUEST	echoRequest;
	char    cFiller[256];
}ECHOREPLY, *PECHOREPLY;


#pragma pack()


// Fill in echo request header
// and send to destination
int SendEchoRequest(TEXTSTR pResult, SOCKET s,SOCKADDR *lpstToAddr)
{
	static ECHOREQUEST echoReq;
	static int nId = 1;
	static int nSeq = 1;
	int nRet;

	// Fill in echo request
	echoReq.icmpHdr.Type		= ICMP_ECHOREQ;
	echoReq.icmpHdr.Code		= 0;
	echoReq.icmpHdr.Checksum	= 0;
	echoReq.icmpHdr.ID			= nId++;
	echoReq.icmpHdr.Seq			= nSeq++;

	// Fill in some data to send
	for (nRet = 0; nRet < REQ_DATASIZE; nRet++)
		echoReq.cData[nRet] = ' '+nRet;

	// Save tick count when sent
	echoReq.dwTime				= GetCPUTick();

	// Put data in packet and compute checksum
	echoReq.icmpHdr.Checksum = in_cksum((u_short *)&echoReq, sizeof(ECHOREQUEST));

	// Send the echo request  								  
	nRet = sendto(s,						/* socket */
				 (TEXTSTR)&echoReq,			/* buffer */
				 sizeof(ECHOREQUEST),
				 0,							/* flags */
				 (SOCKADDR*)lpstToAddr, /* destination */
				 sizeof(SOCKADDR));   /* address length */

	return (nRet);
}


// RecvEchoReply()
// Receive incoming data
// and parse out fields
int RecvEchoReply(TEXTSTR pResult, SOCKET s, SOCKADDR_IN *lpsaFrom, u_char *pTTL)
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
					(TEXTSTR)&echoReply,	// buffer
					sizeof(ECHOREPLY),	// size of buffer
					0,					// flags
					(SOCKADDR*)lpsaFrom,	// From address
					&nAddrLen);			// pointer to address len
	// Check return value
	if (nRet == SOCKET_ERROR) 
	{
      return 0;
	}
	if( echoReply.echoRequest.icmpHdr.Type == 3 )
	{
		return 2;
	}
	if( echoReply.echoRequest.icmpHdr.Type == 11 )
	{
      // ttl timeout!
		return 3;
	}
   //DebugBreak();
// if( echoReply.ipHdr.
	// return time sent and IP TTL
	*pTTL = echoReply.ipHdr.TTL;
	return 1;   		
}

// What happened?
TEXTSTR ReportError(TEXTSTR pInto, TEXTSTR pWhere)
{
    return pInto + sprintf( pInto, WIDE("\n%s error: %d\n"),
                            pWhere, WSAGetLastError());
}


// WaitForEchoReply()
// Use select() to determine when
// data is waiting to be read
int WaitForEchoReply(SOCKET s, _32 dwTime)
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
u_short in_cksum(u_short *addr, int len)
{
	register int nleft = len;
	register u_short *w = addr;
	register u_short answer;
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
		u_short	u = 0;

		*(u_char *)(&u) = *(u_char *)w ;
		sum += u;
	}

	/*
	 * add back carry outs from top 16 bits to low 16 bits
	 */
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = ~sum;				/* truncate to 16 bits */
	return (answer);
}

