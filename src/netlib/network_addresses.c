///////////////////////////////////////////////////////////////////////////
//
// Filename    -  network_address.C
//
// Description -  Network address support
//
// Author      -  James Buckeyne
//
// Create Date -  2019-06-26
//
///////////////////////////////////////////////////////////////////////////

//
//  DEBUG FLAGS IN netstruc.h
//
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE  // for features.h
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#endif

#define FIX_RELEASE_COM_COLLISION
#define NO_UNICODE_C
#include <stdhdrs.h>
#include <stddef.h>
#include <ctype.h>
#include <sack_types.h>
#include <deadstart.h>
#include <sqlgetoption.h>

#include "netstruc.h"
#include <network.h>

//#define DO_LOGGING // override no _DEBUG def to do loggings...
//#define NO_LOGGING // force neverlog....

#include <logging.h>
#include <procreg.h>
#ifdef __LINUX__
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <pthread.h>
#endif

#include <sharemem.h>
#include <timers.h>
#include <idle.h>

//for GetMacAddress
#ifdef __LINUX__
#include <net/if.h>
//#include <sys/timeb.h>

//*******************8
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <sys/un.h>
#include <arpa/inet.h>
#ifndef __ANDROID__
#include <ifaddrs.h>
#else
#include "android_ifaddrs.h"
#define EPOLLRDHUP EPOLLHUP
#define EPOLL_CLOEXEC 0
#endif
#ifdef __MAC__
#include <sys/event.h>
#include <sys/time.h>
#else
#include <sys/epoll.h>
#endif
//*******************8

#endif
#ifdef WIN32
#include <windows.h>
#include <stdio.h>
#ifdef __CYGWIN__
#include <mingw/tchar.h>
#else
#include <tchar.h>
#endif
#include <wincrypt.h>
#include <iphlpapi.h>
#endif

SACK_NETWORK_NAMESPACE

//----------------------------------------------------------------------------

#if !defined( __MAC__ ) && !defined( __EMSCRIPTEN__ )
#  define INCLUDE_MAC_SUPPORT
#endif

NETWORK_PROC( int, GetMacAddress)(PCLIENT pc, uint8_t* buf, size_t *buflen )//int get_mac_addr (char *device, unsigned char *buffer)
{
#ifdef INCLUDE_MAC_SUPPORT
#  ifdef __LINUX__
#    ifdef __THIS_CODE_GETS_MY_MAC_ADDRESS___
	int fd;
	struct ifreq ifr;

	fd = socket(PF_UNIX, SOCK_DGRAM, 0);
	if (fd == -1)
	{
		lprintf("Unable to create socket for pclient: %p", pc);
		return -1;
	}

	strcpy (ifr.ifr_name, GetNetworkLong(pc,GNL_IP));

	if (ioctl (fd, SIOCGIFFLAGS, &ifr) < 0)
	{
		close (fd);
		return -1;
	}

	if (ioctl (fd, SIOCGIFHWADDR, &ifr) < 0)
	{
		close (fd);
		return -1;
	}

	close (fd);

	memcpy (pc->hwClient, ifr.ifr_hwaddr.sa_data, 6);

	return 0;
#    endif
	   /* this code queries the arp table to figure out who the other side is */
	//int fd;
	struct arpreq arpr;
	struct ifconf ifc;
	MemSet( &arpr, 0, sizeof( arpr ) );
	lprintf( "this is broken." );
	MemCpy( &arpr.arp_pa, pc->saClient, sizeof( SOCKADDR ) );
	arpr.arp_ha.sa_family = AF_INET;
	{
		char ifbuf[256];
		ifc.ifc_len = sizeof( ifbuf );
		ifc.ifc_buf = ifbuf;
		ioctl( pc->Socket, SIOCGIFCONF, &ifc );
		{
			int i;
			struct ifreq *IFR;
			IFR = ifc.ifc_req;
			for( i = ifc.ifc_len / sizeof( struct ifreq); --i >=0; IFR++ )
			{
				printf( "IF: %s\n", IFR->ifr_name );
				strcpy( arpr.arp_dev, "eth0" );
			}
		}
	}
	DebugBreak();
	if( ioctl( pc->Socket, SIOCGARP, &arpr ) < 0 )
	{
		lprintf( "Error of some sort ... %s", strerror( errno ) );
		DebugBreak();
	}

	return 0;
#  endif
#  ifdef WIN32
	HRESULT hr;
	ULONG   ulLen;
	// I don't understand this useless cast - from size_t to ULONG?
	// isn't that the same thing?
	ulLen = (ULONG)(*buflen);

	//needs ws2_32.lib and iphlpapi.lib in the linker.
	hr = SendARP ( (IPAddr)GetNetworkLong(pc,GNL_MYIP), (IPAddr)GetNetworkLong(pc,GNL_MYIP), (PULONG)buf, &ulLen);
	(*buflen) = ulLen;
//  The second parameter of SendARP is a PULONG, which is typedef'ed to a pointer to
//  an unsigned long.  The pc->hwClient is a pointer to an array of uint8_t (unsigned chars),
//  actually defined in netstruc.h as uint8_t hwClient[6]; Well, in the end, they are all
//  just addresses, whether they be address to information of eight bits in length, or
//  of (sizeof(unsigned)) in length.  Although this may, in the future, throw a warning.
	//hr = SendARP (GetNetworkLong(pc,GNL_IP), 0, (PULONG)pc->hwClient, &ulLen);
	//lprintf ("Return %08x, length %8d\n", hr, ulLen);

	return hr == S_OK;
#  endif
#else
	return 0;
#endif
}

NETWORK_PROC( PLIST, GetMacAddresses)( void )//int get_mac_addr (char *device, unsigned char *buffer)
{
#ifdef INCLUDE_MAC_SUPPORT
#ifdef __LINUX__
#ifdef __THIS_CODE_GETS_MY_MAC_ADDRESS___
	int fd;
	struct ifreq ifr;

	fd = socket(PF_UNIX, SOCK_DGRAM, 0);
	if (fd == -1)
	{
		lprintf("Unable to create socket for pclient: %p", pc);
		return -1;
	}

	strcpy (ifr.ifr_name, GetNetworkLong(pc,GNL_IP));

	if (ioctl (fd, SIOCGIFFLAGS, &ifr) < 0)
	{
		close (fd);
		return -1;
	}

	if (ioctl (fd, SIOCGIFHWADDR, &ifr) < 0)
	{
		close (fd);
		return -1;
	}

	close (fd);

	memcpy (pc->hwClient, ifr.ifr_hwaddr.sa_data, 6);

	return 0;
#endif
	/* this code queries the arp table to figure out who the other side is */
	//int fd;
	struct arpreq arpr;
	MemSet( &arpr, 0, sizeof( arpr ) );
#if 0
	lprintf( "this is broken." );
	MemCpy( &arpr.arp_pa, pc->saClient, sizeof( SOCKADDR ) );
	arpr.arp_ha.sa_family = AF_INET;
	{
		char ifbuf[256];
		ifc.ifc_len = sizeof( ifbuf );
		ifc.ifc_buf = ifbuf;
		ioctl( pc->Socket, SIOCGIFCONF, &ifc );
		{
			int i;
			struct ifreq *IFR;
			IFR = ifc.ifc_req;
			for( i = ifc.ifc_len / sizeof( struct ifreq); --i >=0; IFR++ )
			{
				printf( "IF: %s\n", IFR->ifr_name );
				strcpy( arpr.arp_dev, "eth0" );
			}
		}
	}
	DebugBreak();
	if( ioctl( pc->Socket, SIOCGARP, &arpr ) < 0 )
	{
		lprintf( "Error of some sort ... %s", strerror( errno ) );
		DebugBreak();
	}
#endif

	return 0;
#endif
#ifdef WIN32

	HRESULT hr;
	ULONG   ulLen;
	uint8_t hwClient[6];
	ulLen = 6;

	//needs ws2_32.lib and iphlpapi.lib in the linker.
	hr = SendARP ((IPAddr)GetNetworkLong(NULL,GNL_IP), 0x100007f, (PULONG)&hwClient, &ulLen);
//  The second parameter of SendARP is a PULONG, which is typedef'ed to a pointer to
//  an unsigned long.  The pc->hwClient is a pointer to an array of uint8_t (unsigned chars),
//  actually defined in netstruc.h as uint8_t hwClient[6]; Well, in the end, they are all
//  just addresses, whether they be address to information of eight bits in length, or
//  of (sizeof(unsigned)) in length.  Although this may, in the future, throw a warning.
	//hr = SendARP (GetNetworkLong(pc,GNL_IP), 0, (PULONG)pc->hwClient, &ulLen);
	lprintf ("Return %08x, length %8d\n", hr, ulLen);

	return 0;
#endif
#else
	return 0;

#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

LOGICAL IsAddressV6( SOCKADDR *addr )
{
	if( addr->sa_family == AF_INET6 && SOCKADDR_LENGTH( addr ) == 28 )
		return TRUE;
	return FALSE;
}

const char * GetAddrString( SOCKADDR *addr )
{
	static char buf[256];
	//lprintf( "addr family is: %d", addr->sa_family );
	if( addr->sa_family == AF_INET )
		snprintf( buf, 256, "%d.%d.%d.%d"
			, *(((unsigned char *)addr) + 4),
			*(((unsigned char *)addr) + 5),
			*(((unsigned char *)addr) + 6),
			*(((unsigned char *)addr) + 7) );
	else if( addr->sa_family == AF_INET6 )
	{
		int first0 = 8;
		int last0 = 0;
		int after0 = 0;
		int n;
		int ofs = 0;
		uint32_t piece;
		for( n = 0; n < 8; n++ ) {
			piece = (*(((unsigned short *)((unsigned char*)addr + 8 + (n * 2)))));
			if( piece ) {
				if( first0 < 8 )
					after0 = 1;
				if( !ofs ) {
					ofs += snprintf( buf + ofs, 256 - ofs, "%x", ntohs( piece ) );
				}
				else {
					//lprintf( last0, n );
					if( last0 == 4 && first0 == 0 )
						if( piece == 0xFFFF ) {
							snprintf( buf, 256, "::ffff:%d.%d.%d.%d",
								(*((unsigned char*)addr + 20)),
								(*((unsigned char*)addr + 21)),
								(*((unsigned char*)addr + 22)),
								(*((unsigned char*)addr + 23)) );
							break;
						}
					ofs += snprintf( buf + ofs, 256 - ofs, ":%x", ntohs( piece ) );
				}
			}
			else {
				if( !after0 ) {
					if( first0 > n ) {
						first0 = n;
						ofs += snprintf( buf + ofs, 256 - ofs, ":" );
					}
					if( last0 < n )
						last0 = n;
				}
				if( last0 < n )
					ofs += snprintf( buf + ofs, 256 - ofs, ":%x", ntohs( piece ) );
			}
		}
		if( !after0 && first0 < 8 )
			ofs += snprintf( buf + ofs, 256 - ofs, ":" );
	}
	else
		snprintf( buf, 256, "unknown protocol" );

	return buf;
}

const char * GetAddrName( SOCKADDR *addr )
{
	char * tmp = ((char**)addr)[-1];
	if( !tmp )
	{
		const char *buf = GetAddrString( addr );
		((char**)addr)[-1] = strdup( buf );
	}
	return ((char**)addr)[-1];
}

void SetAddrName( SOCKADDR *addr, const char *name )
{
	((uintptr_t*)addr)[-1] = (uintptr_t)strdup( name );
}

//---------------------------------------------------------------------------

SOCKADDR *AllocAddrEx( DBG_VOIDPASS )
{
	SOCKADDR *lpsaAddr=(SOCKADDR*)AllocateEx( MAGIC_SOCKADDR_LENGTH + 2 * sizeof( uintptr_t ) DBG_RELAY );
	memset( lpsaAddr, 0, MAGIC_SOCKADDR_LENGTH );
	//initialize socket length to something identifiable?
	((uintptr_t*)lpsaAddr)[0] = 3;
	((uintptr_t*)lpsaAddr)[1] = 0; // string representation of address

	lpsaAddr = (SOCKADDR*)( ( (uintptr_t)lpsaAddr ) + sizeof(uintptr_t) * 2 );
	return lpsaAddr;
}

//----------------------------------------------------------------------------

int GetAddressParts( SOCKADDR *sa, uint32_t *pdwIP, uint16_t *pdwPort )
{
	int result = TRUE;
	if( sa )
	{
		if( sa->sa_family == AF_INET ) {
			if( pdwIP )
				(*pdwIP) = (uint32_t)(((SOCKADDR_IN*)sa)->sin_addr.S_un.S_addr);
		}
		else if( sa->sa_family == AF_INET6 ) {
			if( pdwIP )
				memcpy( pdwIP, &(((SOCKADDR_IN*)sa)->sin_addr.S_un.S_addr), 16 ); //-V512
		}
		else
			result = FALSE;
		if( (sa->sa_family == AF_INET) || (sa->sa_family = AF_INET6) ) {
			if( pdwPort )
				(*pdwPort) = ntohs((uint16_t)( (SOCKADDR_IN*)sa)->sin_port);
		}
		else
			result = FALSE;
	}
	return result;
}

//----------------------------------------------------------------------------

SOCKADDR* DuplicateAddressEx( SOCKADDR *pAddr DBG_PASS ) // return a copy of this address...
{
	POINTER tmp = (POINTER)( ( (uintptr_t)pAddr ) - 2*sizeof(uintptr_t) );
	SOCKADDR *dup = AllocAddrEx( DBG_VOIDRELAY );
	POINTER tmp2 = (POINTER)( ( (uintptr_t)dup ) - 2*sizeof(uintptr_t) );
	MemCpy( tmp2, tmp, SOCKADDR_LENGTH( pAddr ) + 2*sizeof(uintptr_t) );
	if( ((char**)( ( (uintptr_t)pAddr ) - sizeof(char*) ))[0] )
		( (char**)( ( (uintptr_t)dup ) - sizeof( char* ) ) )[0]
				= strdup( ((char**)( ( (uintptr_t)pAddr ) - sizeof( char* ) ))[0] );
	return dup;
}

//---------------------------------------------------------------------------

NETWORK_PROC( SOCKADDR *,CreateAddress_hton)( uint32_t dwIP,uint16_t nHisPort)
{
	SOCKADDR_IN *lpsaAddr=(SOCKADDR_IN*)AllocAddr();
	if (!lpsaAddr)
		return(NULL);
	SET_SOCKADDR_LENGTH( lpsaAddr, IN_SOCKADDR_LENGTH );
	lpsaAddr->sin_family       = AF_INET;         // InetAddress Type.
	lpsaAddr->sin_addr.S_un.S_addr  = htonl(dwIP);
	lpsaAddr->sin_port         = htons(nHisPort);
	return((SOCKADDR*)lpsaAddr);
}

//---------------------------------------------------------------------------
#if defined( __LINUX__ ) && !defined( __CYGWIN__ )
#  ifndef __LINUX__
#    define UNIX_PATH_MAX	 108
struct sockaddr_un {
#    ifdef __MAC__
	u_char   sa_len;
#    endif
	sa_family_t  sun_family;		/* AF_UNIX */
	char	       sun_path[UNIX_PATH_MAX]; /* pathname */
};
#  endif
NETWORK_PROC( SOCKADDR *,CreateUnixAddress)( CTEXTSTR path )
{
	struct sockaddr_un *lpsaAddr;
#ifdef UNICODE
	char *tmp_path = CStrDup( path );
#endif
   lpsaAddr=(struct sockaddr_un*)AllocAddr();
	if (!lpsaAddr)
		return(NULL);
	((uintptr_t*)lpsaAddr)[-1] = StrLen( path ) + 1;
	lpsaAddr->sun_family = PF_UNIX;
#ifdef UNICODE
	strncpy( lpsaAddr->sun_path, tmp_path, 107 );
	Deallocate( char*, tmp_path );
#else
	strncpy( lpsaAddr->sun_path, path, 107 );
#endif

#ifdef __MAC__
	lpsaAddr->sun_len = 2+strlen( lpsaAddr->sun_path );
#endif
	return((SOCKADDR*)lpsaAddr);
}
#else
NETWORK_PROC( SOCKADDR *,CreateUnixAddress)( CTEXTSTR path )
{
	lprintf( "-- CreateUnixAddress -- not available. " );
	return NULL;
}
#endif
//---------------------------------------------------------------------------

SOCKADDR *CreateAddress( uint32_t dwIP,uint16_t nHisPort)
{
	SOCKADDR_IN *lpsaAddr=(SOCKADDR_IN*)AllocAddr();
	if (!lpsaAddr)
		return(NULL);
	SET_SOCKADDR_LENGTH( lpsaAddr, IN_SOCKADDR_LENGTH );
	lpsaAddr->sin_family	    = AF_INET;         // InetAddress Type.
	lpsaAddr->sin_addr.S_un.S_addr  = dwIP;
	lpsaAddr->sin_port         = htons(nHisPort);
	return((SOCKADDR*)lpsaAddr);
}

//---------------------------------------------------------------------------

SOCKADDR *CreateRemote( CTEXTSTR lpName,uint16_t nHisPort)
{
	SOCKADDR_IN *lpsaAddr;
	int conversion_success = FALSE;
	char *tmpName = NULL;
#ifdef UNICODE
	char *_lpName = CStrDup( lpName );
#  define lpName _lpName
#endif
#ifndef WIN32
	PHOSTENT phe;
	// a IP type name will never have a / in it, therefore
	// we can assume it's a unix type address....
	if( lpName && StrChr( lpName, '/' ) )
		return CreateUnixAddress( lpName );
#endif
	if( lpName[0] == '[' && lpName[StrLen( lpName ) - 1] == ']' ) {
		size_t len;
		tmpName = NewArray( char, len = StrLen( lpName ) );
		memcpy( tmpName, lpName + 1, len - 2 );
		tmpName[len - 2] = 0;
		lpName = tmpName;
	}

	lpsaAddr=(SOCKADDR_IN*)AllocAddr();
	if( !lpsaAddr )
	{
#ifdef UNICODE
		Deallocate( char *, _lpName );
#endif
		return(NULL);
	}
	SetAddrName( (SOCKADDR*)lpsaAddr, lpName );

	// if it's a numeric name... attempt to use as an address.
#ifdef __LINUX__
	if( lpName &&
		( lpName[0] >= '0' && lpName[0] <= '9' )
	  && StrChr( lpName, '.' ) )
	{
		if( inet_pton( AF_INET, lpName, &lpsaAddr->sin_addr ) > 0 )
		{
			SET_SOCKADDR_LENGTH( lpsaAddr, IN_SOCKADDR_LENGTH );
			lpsaAddr->sin_family       = AF_INET;         // InetAddress Type.
			conversion_success = TRUE;
		}
	}
	else if( lpName
		   && ( ( lpName[0] >= '0' && lpName[0] <= '9' )
		      || ( lpName[0] >= 'a' && lpName[0] <= 'f' )
		      || ( lpName[0] >= 'A' && lpName[0] <= 'F' )
		      || lpName[0] == ':'
		      || ( lpName[0] == '[' && lpName[StrLen( lpName ) - 1] == ']' ) )
		   && StrChr( lpName, ':' )!=StrRChr( lpName, ':' ) )
	{
		//lprintf( "Converting name:", lpName );
		if( inet_pton( AF_INET6, lpName, (struct in6_addr*)(&lpsaAddr->sin_addr+1) ) > 0 )
		{
			//lprintf( "This iset in the wrong place?" );
			((struct sockaddr_in6*)lpsaAddr)->sin6_scope_id = 0;
			SET_SOCKADDR_LENGTH( lpsaAddr, IN6_SOCKADDR_LENGTH );
			lpsaAddr->sin_family       = AF_INET6;         // InetAddress Type.
			conversion_success = TRUE;
		}
	}
#endif
	if( !conversion_success )
	{
		if( lpName )
		{
#ifndef h_addr
#define h_addr h_addr_list[0]
#define H_ADDR_DEFINED
#endif
#ifdef WIN32
			{
				struct addrinfo *result;
				struct addrinfo *test;
				int error;
				//lprintf( "Or we do getaddrinfo... %s", lpName );
				error = getaddrinfo( lpName, NULL, NULL, (struct addrinfo**)&result );
				if( error == 0 )
				{
					for( test = result; test; test = test->ai_next )
					{
						//SOCKADDR *tmp;
						//AddLink( &globalNetworkData.addresses, tmp = AllocAddr() );
						//lprintf( "Copy addr: %d", test->ai_addrlen );
						MemCpy( lpsaAddr, test->ai_addr, test->ai_addrlen );
						SET_SOCKADDR_LENGTH( lpsaAddr, test->ai_addrlen );
						break;
					}
				}
				else
					lprintf( "getaddrinfo Error: %d for [%s]", error, lpName );
			}
#else //WIN32

			char *tmp = CStrDup( lpName );
#ifdef __EMSCRIPTEN__
			if(!(phe=gethostbyname(tmp) ) )
#else
         if(1)
#endif
			{
#if !defined( __EMSCRIPTEN__ )
				if( !(phe=gethostbyname2(tmp,AF_INET6) ) )
#endif
				{
#if !defined( __EMSCRIPTEN__ )
					if( !(phe=gethostbyname2(tmp,AF_INET) ) )
#endif
					{
 						// could not find the name in the host file.
						lprintf( "Could not Resolve to %s  %s", tmp, lpName );
						ReleaseAddress((SOCKADDR*)lpsaAddr);
						Deallocate( char*, tmp );
						if( tmpName ) Deallocate( char*, tmpName );
						return(NULL);
					}
#if !defined( __EMSCRIPTEN__ )
					else
					{
						lprintf( "Strange, gethostbyname failed, but AF_INET worked... %s", tmp );
						SET_SOCKADDR_LENGTH( lpsaAddr, IN_SOCKADDR_LENGTH );
						lpsaAddr->sin_family = AF_INET;
						memcpy( &lpsaAddr->sin_addr.S_un.S_addr,           // save IP address from host entry.
							    phe->h_addr,
						       phe->h_length);
					}
#endif
				}
#if !defined( __EMSCRIPTEN__ )
				else
				{
					SET_SOCKADDR_LENGTH( lpsaAddr, IN6_SOCKADDR_LENGTH );
					lpsaAddr->sin_family = AF_INET6;         // InetAddress Type.
					//lprintf( "This copy:%d", phe->h_length );
#  if inline_note_never_included
					{
						__SOCKADDR_COMMON (sin6_);
						n_port_t sin6_port;        /* Transport layer port # */
						uint32_t sin6_flowinfo;     /* IPv6 flow information */
						struct in6_addr sin6_addr;  /* IPv6 address */
						uint32_t sin6_scope_id;     /* IPv6 scope-id */
					};
#  endif

					memcpy( ((struct sockaddr_in6*)lpsaAddr)->sin6_addr.s6_addr,           // save IP address from host entry.
							 phe->h_addr,
							 phe->h_length);
				}
#endif
			}
			else
			{
				Deallocate( char *, tmp );
				SET_SOCKADDR_LENGTH( lpsaAddr, IN_SOCKADDR_LENGTH );
				lpsaAddr->sin_family = AF_INET;         // InetAddress Type.
				//lprintf( "gethostbyname2:...." );
				memcpy( &lpsaAddr->sin_addr.S_un.S_addr,           // save IP address from host entry.
					 phe->h_addr,
					 phe->h_length);
			}
#endif
#ifdef H_ADDR_DEFINED
#  undef H_ADDR_DEFINED
#  undef h_addr
#endif
		}
		else
		{
			lpsaAddr->sin_family      = AF_INET;         // InetAddress Type.
			lpsaAddr->sin_addr.S_un.S_addr = 0;
			SET_SOCKADDR_LENGTH( lpsaAddr, IN_SOCKADDR_LENGTH );
		}
	}
#ifdef UNICODE
	Deallocate( char *, _lpName );
#  undef lpName
#endif
	//lprintf( "Resulting thing" );
	//DumpAddr( "RESULT ADDRESS", (SOCKADDR*)lpsaAddr );
	// put in his(destination) port number...
	if( tmpName ) Deallocate( char*, tmpName );
	lpsaAddr->sin_port         = htons(nHisPort);
	return((SOCKADDR*)lpsaAddr);
}

//----------------------------------------------------------------------------

#ifdef __cplusplus
namespace udp {
#endif
NETWORK_PROC( void, DumpAddrEx)( CTEXTSTR name, SOCKADDR *sa DBG_PASS )
	{
		if( !sa ) { _lprintf(DBG_RELAY)( "%s: NULL", name ); return; }
		LogBinary( (uint8_t *)sa, SOCKADDR_LENGTH( sa ) );
		if( sa->sa_family == AF_INET ) {
			_lprintf(DBG_RELAY)( "%s: (%s) %d.%d.%d.%d:%d ", name
			       , ( ((uintptr_t*)sa)[-1] & 0xFFFF0000 )?( ((char**)sa)[-1] ) : "no name"
			       //*(((unsigned char *)sa)+0),
			       //*(((unsigned char *)sa)+1),
			       ,*(((unsigned char *)sa)+4),
			       *(((unsigned char *)sa)+5),
			       *(((unsigned char *)sa)+6),
			       *(((unsigned char *)sa)+7)
			       , ntohs( *(((unsigned short *)((unsigned char*)sa + 2))) )
			);
		} else if( sa->sa_family == AF_INET6 )
		{
			lprintf( "Socket address binary: %s", name );
			_lprintf(DBG_RELAY)( "%s: (%s) %03d %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x "
					 , name
					, ( ((uintptr_t*)sa)[-1] & 0xFFFF0000 )?( ((char**)sa)[-1] ) : "no name"
					 , ntohs(*(((unsigned short *)((unsigned char*)sa+2))))
					 , ntohs(*(((unsigned short *)((unsigned char*)sa+8))))
					 , ntohs(*(((unsigned short *)((unsigned char*)sa+10))))
					 , ntohs(*(((unsigned short *)((unsigned char*)sa+12))))
					 , ntohs(*(((unsigned short *)((unsigned char*)sa+14))))
					 , ntohs(*(((unsigned short *)((unsigned char*)sa+16))))
					 , ntohs(*(((unsigned short *)((unsigned char*)sa+18))))
					 , ntohs(*(((unsigned short *)((unsigned char*)sa+20))))
					 , ntohs(*(((unsigned short *)((unsigned char*)sa+22))))
					 );
		}

}
#ifdef __cplusplus
}
#endif
//----------------------------------------------------------------------------

NETWORK_PROC( SOCKADDR *, SetAddressPort )( SOCKADDR *pAddr, uint16_t nDefaultPort )
{
	if( pAddr )
		((SOCKADDR_IN *)pAddr)->sin_port = htons(nDefaultPort);
	return pAddr;
}

//----------------------------------------------------------------------------

NETWORK_PROC( SOCKADDR *, SetNonDefaultPort )( SOCKADDR *pAddr, uint16_t nDefaultPort )
{
	if( pAddr && !((SOCKADDR_IN *)pAddr)->sin_port )
		((SOCKADDR_IN *)pAddr)->sin_port = htons(nDefaultPort);
	return pAddr;
}

//----------------------------------------------------------------------------

NETWORK_PROC( SOCKADDR *,CreateSockAddress)(CTEXTSTR name, uint16_t nDefaultPort )
{
// blah... should process a ip:port - but - default port?!
	uint32_t bTmpName = 0;
	char * tmp;
	SOCKADDR *sa = NULL;
	char *port;
	uint16_t wPort;
	CTEXTSTR portName = name;

#ifdef UNICODE
	char *_name = CStrDup( name );
#  define name _name
#endif
	if( name[0] == '[' ) { //-V595
		while( portName[0] && portName[0] != ']' )
			portName++;
		if( portName[0] ) portName++;
	}
	if( name && portName[0] && ( port = (char*)strrchr( portName, ':' ) ) )
	{
		tmp = StrDup( name );
		bTmpName = 1;
		port = tmp + (port-name);
		name = tmp;
		//Log1( "Found ':' assuming %s is IP:PORT", name );
		*port = 0;
		port++;
		if( port[0] )  // a trailing : could be IPV6 abbreviation.
		{
			if( isdigit( *port ) )
			{
				wPort = (short)atoi( port ); //-V595
			}
			else
			{
				struct servent *se;
				se = getservbyname( port, NULL );
				if( !se )
				{
#ifdef UNICODE
#define FMT "S"
#else
#define FMT "s"
#endif
					Log1( "Could not resolve \"%" FMT "\" as a valid service name", port );
					//return NULL;
					wPort = nDefaultPort;
				}
				else
					wPort = htons(se->s_port);
				//Log1( "port alpha - name resolve to %d", wPort );
			}
		}
		else
			wPort = nDefaultPort;
#ifdef UNICODE
#  undef name
#endif
		sa = CreateRemote( name, wPort );
		if( port )
		{
			port[-1] = ':';  // incase we obliterated it
		}
	}
	else  // no port specification...
	{
		//Log1( "%s does not have a ':'", name );
		sa = CreateRemote( name, nDefaultPort );
	}
#ifdef UNICODE
	Deallocate( char *, _name );
#endif
	if( bTmpName ) Deallocate( char*, tmp );
	return sa;
}

//----------------------------------------------------------------------------

SOCKADDR *CreateLocal(uint16_t nMyPort)
{
	char lpHostName[HOSTNAME_LEN];
	if (gethostname(lpHostName,HOSTNAME_LEN))
	{
		return(NULL);
	}
	return CreateRemote( "0.0.0.0", nMyPort );
}

//----------------------------------------------------------------------------

LOGICAL CompareAddressEx( SOCKADDR *sa1, SOCKADDR *sa2, int method )
{
	if( method == SA_COMPARE_FULL )
	{
		if( sa1 && sa2 )
		{
			if( ((SOCKADDR_IN*)sa1)->sin_family == ((SOCKADDR_IN*)sa2)->sin_family )
			{
				switch( ((SOCKADDR_IN*)sa1)->sin_family )
				{
				case AF_INET:
					{
						SOCKADDR_IN *sin1 = (SOCKADDR_IN*)sa1;
						SOCKADDR_IN *sin2 = (SOCKADDR_IN*)sa2;
						if( MemCmp( sin1, sin2, sizeof( SOCKADDR_IN ) ) == 0 )
							return 1;
					}
					break;
				default:
					xlprintf( LOG_ALWAYS )( "unhandled address type passed to compare, resulting FAILURE" );
					return 0;
				}
			}
		}
	}
	else
	{
		if( sa1 && sa2 )
		{
			if( ((SOCKADDR_IN*)sa1)->sin_family == ((SOCKADDR_IN*)sa2)->sin_family )
			{
				switch( ((SOCKADDR_IN*)sa1)->sin_family )
				{
				case AF_INET:
					{
						if( MemCmp( &((SOCKADDR_IN*)sa1)->sin_addr, &((SOCKADDR_IN*)sa2)->sin_addr, sizeof( ((SOCKADDR_IN*)sa2)->sin_addr ) ) == 0 )
							return 1;
					}
					break;
				default:
					xlprintf( LOG_ALWAYS )( "unhandled address type passed to compare, resulting FAILURE" );
					return 0;
				}
			}
		}
	}
	return 0;
}

//----------------------------------------------------------------------------

LOGICAL CompareAddress( SOCKADDR *sa1, SOCKADDR *sa2 )
{
	return CompareAddressEx( sa1, sa2, SA_COMPARE_FULL );
}

//----------------------------------------------------------------------------

PLIST GetLocalAddresses( void )
{
	return globalNetworkData.addresses;
}

//----------------------------------------------------------------------------

LOGICAL IsThisAddressMe( SOCKADDR *addr, uint16_t myport )
{
	struct interfaceAddress *test_addr;
	INDEX idx;

	LIST_FORALL( globalNetworkData.addresses, idx, struct interfaceAddress *, test_addr )
	{
		if( ((SOCKADDR_IN*)addr)->sin_family == ((SOCKADDR_IN*)test_addr->sa)->sin_family )
		{
			switch( ((SOCKADDR_IN*)addr)->sin_family )
			{
			case AF_INET:
				{
					if( MemCmp( &((SOCKADDR_IN*)addr)->sin_addr, &((SOCKADDR_IN*)test_addr->sa)->sin_addr, sizeof(((SOCKADDR_IN*)addr)->sin_addr)  ) == 0 )
					{
						return TRUE;
					}
				}
				break;
			default:
				lprintf( "Unknown comparison" );
			}
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------

LOGICAL IsBroadcastAddressForInterface( struct interfaceAddress *address, SOCKADDR *addr ) {
	if( addr->sa_family == AF_INET ) {
      //lprintf( "can test for broadcast... %08x %08x %08x", ( ((uint32_t*)(address->saMask->sa_data+2))[0] | ((uint32_t*)(addr->sa_data+2))[0] ), ((uint32_t*)address->saMask->sa_data)[0] , ((uint32_t*)addr->sa_data)[0] );
		if( ( ((uint32_t*)(address->saMask->sa_data+2))[0] | ((uint32_t*)(addr->sa_data+2))[0] ) == 0xFFFFFFFFU )
         return TRUE;
	}
   return FALSE;
}

//----------------------------------------------------------------------------

struct interfaceAddress* GetInterfaceForAddress( SOCKADDR *addr )
{
	struct interfaceAddress *test_addr;
	INDEX idx;
	if( !globalNetworkData.addresses )
		LoadNetworkAddresses();
	LIST_FORALL( globalNetworkData.addresses, idx, struct interfaceAddress *, test_addr )
	{
		if( ((SOCKADDR_IN*)addr)->sin_family == ((SOCKADDR_IN*)test_addr->sa)->sin_family )
		{
			switch( ((SOCKADDR_IN*)addr)->sin_family )
			{
			case AF_INET:
				{
					if( MemCmp( &((SOCKADDR_IN*)addr)->sin_addr, test_addr->sa->sa_data + 2, sizeof(((SOCKADDR_IN*)addr)->sin_addr)  ) == 0 )
					{
						return test_addr;
					}
				}
				break;
			case AF_INET6:
				{
					if( MemCmp( &((SOCKADDR_IN*)addr)->sin_addr, test_addr->sa->sa_data + 2, 16 ) == 0 )
					{
						return test_addr;
					}
				}
				break;
			default:
				lprintf( "Unknown comparison" );
			}
		}
	}
	return NULL;
}

//----------------------------------------------------------------------------

SOCKADDR* GetBroadcastAddressForInterface( SOCKADDR *addr )
{
	struct interfaceAddress *test_addr;
	INDEX idx;
	if( !globalNetworkData.addresses )
		LoadNetworkAddresses();
	LIST_FORALL( globalNetworkData.addresses, idx, struct interfaceAddress *, test_addr )
	{
		if( ((SOCKADDR_IN*)addr)->sin_family == ((SOCKADDR_IN*)test_addr->sa)->sin_family )
		{
			switch( ((SOCKADDR_IN*)addr)->sin_family )
			{
			case AF_INET:
				{
					if( MemCmp( &((SOCKADDR_IN*)addr)->sin_addr, test_addr->sa->sa_data + 2, sizeof(((SOCKADDR_IN*)addr)->sin_addr)  ) == 0 )
					{
						return test_addr->saBroadcast;
					}
				}
				break;
			case AF_INET6:
				{
					if( MemCmp( &((SOCKADDR_IN*)addr)->sin_addr, test_addr->sa->sa_data + 2, 16 ) == 0 )
					{
						return test_addr->saBroadcast;
					}
				}
				break;
			default:
				lprintf( "Unknown comparison" );
			}
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------

SOCKADDR* GetInterfaceAddressForBroadcast( SOCKADDR *addr )
{
	struct interfaceAddress *test_addr;
	INDEX idx;
	if( !globalNetworkData.addresses )
		LoadNetworkAddresses();

	LIST_FORALL( globalNetworkData.addresses, idx, struct interfaceAddress *, test_addr )
	{
		if( ((SOCKADDR_IN*)addr)->sin_family == ((SOCKADDR_IN*)test_addr->sa)->sin_family )
		{
			switch( ((SOCKADDR_IN*)addr)->sin_family )
			{
			case AF_INET:
				{
					if( MemCmp( &((SOCKADDR_IN*)addr)->sin_addr, test_addr->saBroadcast->sa_data + 2, 4 ) == 0 )
					{
						return test_addr->sa;
					}
				}
				break;
			case AF_INET6:
				{
					if( MemCmp( &((SOCKADDR_IN*)addr)->sin_addr, test_addr->saBroadcast->sa_data + 2, 16  ) == 0 )
					{
						return test_addr->sa;
					}
				}
				break;
			default:
				lprintf( "Unknown comparison" );
			}
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------

void ReleaseAddress(SOCKADDR *lpsaAddr)
{
	// sockaddr is often skewed from what I would expect it. (contains its own length)
	if( lpsaAddr )
	{
		/* strdup is used for the addr part so use free instead of release */
		free( ((POINTER*)( ( (uintptr_t)lpsaAddr ) - sizeof(uintptr_t) ))[0] );
		Deallocate(POINTER, (POINTER)( ( (uintptr_t)lpsaAddr ) - 2 * sizeof(uintptr_t) ));
	}
}

//----------------------------------------------------------------------------
// creates class C broadcast address
SOCKADDR *CreateBroadcast(uint16_t nPort)
{
	SOCKADDR_IN *bcast=(SOCKADDR_IN*)AllocAddr();
	SOCKADDR *lpMyAddr;
	if (!bcast)
		return(NULL);
	lpMyAddr = CreateLocal(0);
	SET_SOCKADDR_LENGTH( bcast, IN_SOCKADDR_LENGTH );
	bcast->sin_family	    = AF_INET;
	bcast->sin_addr.S_un.S_addr  = ((SOCKADDR_IN*)lpMyAddr)->sin_addr.S_un.S_addr;
	bcast->sin_addr.S_un.S_un_b.s_b4 = 0xFF; // Fake a subnet broadcast address
	bcast->sin_port        = htons(nPort);
	ReleaseAddress(lpMyAddr);
	return((SOCKADDR*)bcast);
}

//----------------------------------------------------------------------------

void DumpSocket( PCLIENT pc )
{
	DumpAddr( "REMOT", pc->saClient );
	DumpAddr( "LOCAL", pc->saSource );
	return;
}

#ifdef __cplusplus
namespace udp {
#endif
#undef DumpAddr
NETWORK_PROC( void, DumpAddr)( CTEXTSTR name, SOCKADDR *sa )
{
	DumpAddrEx( name, sa DBG_SRC );
}

#ifdef __cplusplus
}
#endif


//----------------------------------------------------------------------------

CTEXTSTR GetSystemName( void )
{
	// start the network with defaults... we're able to reallocate later.
#ifdef __ANDROID__
#if 0
	// dont' actually have to start winsock; but we do have to do work to get our IP
	int sock_startup = socket( AF_INET, SOCK_RAW, 0);
	if( sock_startup == -1 )
		sock_startup = socket( AF_INET, SOCK_DGRAM, 0);
	if( sock_startup == -1 )
		sock_startup = socket( AF_INET, SOCK_STREAM, 0);
	if( sock_startup >= 0 )
	{
		struct ifconf buffer;
		struct ifreq ifr[10];
		int ifc_num;
		int n;
		buffer.ifc_len = sizeof(ifr);
		buffer.ifc_ifcu.ifcu_buf = (char*)ifr;
		ioctl( sock_startup, SIOCGIFCONF, &buffer);
		ifc_num = buffer.ifc_len / sizeof(struct ifreq);
#define INT_TO_ADDR(_addr) \
(_addr & 0xFF), \
(_addr >> 8 & 0xFF), \
(_addr >> 16 & 0xFF), \
(_addr >> 24 & 0xFF)

		for( n = 0; n < ifc_num; n++ )
		{
			int sd, ifc_num, addr, bcast, mask, network;
			lprintf( "interface %d : %s", n, ifr[n].ifr_name );
			if (ifr[n].ifr_addr.sa_family != AF_INET)
			{
				continue;
			}

			/* display the interface name */
			lprintf("%d) interface: %s\n", n+1, ifr[n].ifr_name);

			/* Retrieve the IP address, broadcast address, and subnet mask. */
			if (ioctl(sd, SIOCGIFADDR, &ifr[n]) == 0)
			{
				addr = ((struct sockaddr_in *)(&ifr[n].ifr_addr))->sin_addr.s_addr;
				lprintf("%d) address: %d.%d.%d.%d\n", n+1, INT_TO_ADDR(addr));
			}
			if (ioctl(sd, SIOCGIFBRDADDR, &ifr[n]) == 0)
			{
				bcast = ((struct sockaddr_in *)(&ifr[n].ifr_broadaddr))->sin_addr.s_addr;
				lprintf("%d) broadcast: %d.%d.%d.%d\n", n+1, INT_TO_ADDR(bcast));
			}
			if (ioctl(sd, SIOCGIFNETMASK, &ifr[n]) == 0)
			{
				mask = ((struct sockaddr_in *)(&ifr[n].ifr_netmask))->sin_addr.s_addr;
				lprintf("%d) netmask: %d.%d.%d.%d\n", n+1, INT_TO_ADDR(mask));
			}

			/* Compute the current network value from the address and netmask. */
			network = addr & mask;
			lprintf("%d) network: %d.%d.%d.%d\n", n+1, INT_TO_ADDR(network));
		}
		close( sock_startup );
	}
	else
#endif
		globalNetworkData.system_name = "No Name Available";
#else
	NetworkStart();
#endif
	return globalNetworkData.system_name;
}

#undef NetworkLock
#undef NetworkUnlock

NETWORK_PROC( void, GetNetworkAddressBinary )( SOCKADDR *addr, uint8_t **data, size_t *datalen ) {
	if( addr ) {
		size_t namelen;
		size_t addrlen = SOCKADDR_LENGTH( addr );
		const char * tmp = ((const char**)addr)[-1];
		if( !( (uintptr_t)tmp & 0xFFFF0000 ) )
		{
			lprintf( "corrupted sockaddr." );
			DebugBreak();
		}
		if( tmp )
		{
			namelen = StrLen( tmp );
		}
		else
			namelen = 0;
		(*datalen) = namelen + 1 + 1 + SOCKADDR_LENGTH( addr );
		(*data) = NewArray( uint8_t, (*datalen) );
		MemCpy( (*data), tmp, namelen + 1 );
		(*data)[namelen+1] = (uint8_t)addrlen;
		MemCpy( (*data) + namelen + 1, addr, addrlen );
	}
}

NETWORK_PROC( SOCKADDR *, MakeNetworkAddressFromBinary )( uintptr_t *data, size_t datalen ) {
	SOCKADDR *addr = AllocAddr();
	size_t namelen = strlen( (const char*)data );
	if( namelen ) // if empty name, don't include it.
		SetAddrName( addr, (const char*)data );
	SET_SOCKADDR_LENGTH( addr, data[1] );
	MemCpy( addr, data + 2, data[1] );
	return addr;
}


#ifdef __LINUX__

void LoadNetworkAddresses( void ) {
	struct ifaddrs *addrs, *tmp;
	struct interfaceAddress *ia;

	getifaddrs( &addrs );
	tmp = addrs;

	ia = New( struct interfaceAddress );
	ia->sa = CreateRemote( "0.0.0.0", 0 );
	ia->saMask = NULL;
	ia->saBroadcast = CreateRemote( "255.255.255.255", 0 );
	AddLink( &globalNetworkData.addresses, ia );

	for( ; tmp; tmp = tmp->ifa_next )
	{
		SOCKADDR *dup;
		if( !tmp->ifa_addr )
			continue;
#  ifndef __MAC__
		if( tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_PACKET )
			continue;
#  endif
		ia = New( struct interfaceAddress );
		dup = AllocAddr();

		if( tmp->ifa_addr->sa_family == AF_INET6 ) {
			continue;
			//memcpy( dup, tmp->ifa_addr, IN6_SOCKADDR_LENGTH );
			//SET_SOCKADDR_LENGTH( dup, IN6_SOCKADDR_LENGTH );
		}
		else {
			memcpy( dup, tmp->ifa_addr, IN_SOCKADDR_LENGTH );
			SET_SOCKADDR_LENGTH( dup, IN_SOCKADDR_LENGTH );
		}
		ia->sa = dup;

		dup = AllocAddr();

		if( tmp->ifa_addr->sa_family == AF_INET6 ) {
			//memcpy( dup, tmp->ifa_netmask, IN6_SOCKADDR_LENGTH );
			//SET_SOCKADDR_LENGTH( dup, IN6_SOCKADDR_LENGTH );
		}
		else {
			if( tmp->ifa_netmask ) {
				memcpy( dup, tmp->ifa_netmask, IN_SOCKADDR_LENGTH );
				SET_SOCKADDR_LENGTH( dup, IN_SOCKADDR_LENGTH );
			} else {
				memset( dup, 0, IN_SOCKADDR_LENGTH );
				SET_SOCKADDR_LENGTH( dup, IN_SOCKADDR_LENGTH );
			}
		}
		ia->saMask = dup;

		ia->saBroadcast = AllocAddr();
		ia->saBroadcast->sa_family = ia->sa->sa_family;
		ia->saBroadcast->sa_data[0] = 0;
		ia->saBroadcast->sa_data[1] = 0;
		ia->saBroadcast->sa_data[2] = (ia->sa->sa_data[2] & ia->saMask->sa_data[2]) | (~ia->saMask->sa_data[2]);
		ia->saBroadcast->sa_data[3] = (ia->sa->sa_data[3] & ia->saMask->sa_data[3]) | (~ia->saMask->sa_data[3]);
		ia->saBroadcast->sa_data[4] = (ia->sa->sa_data[4] & ia->saMask->sa_data[4]) | (~ia->saMask->sa_data[4]);
		ia->saBroadcast->sa_data[5] = (ia->sa->sa_data[5] & ia->saMask->sa_data[5]) | (~ia->saMask->sa_data[5]);
		SET_SOCKADDR_LENGTH( ia->saBroadcast, SOCKADDR_LENGTH( ia->sa ) );
		AddLink( &globalNetworkData.addresses, ia );
	}

	freeifaddrs( addrs );
}

#endif

#ifdef _WIN32
#if 0

#ifdef WIN32
	{
		struct addrinfo *result;
		struct addrinfo *test;
#ifdef _UNICODE
		char *tmp = WcharConvert( globalNetworkData.system_name );
		getaddrinfo( tmp, NULL, NULL, (struct addrinfo**)&result );
		Deallocate( char*, tmp );
#else
		getaddrinfo( globalNetworkData.system_name, NULL, NULL, (struct addrinfo**)&result );
#endif
		for( test = result; test; test = test->ai_next )
		{
			//if( test->ai_family == AF_INET )
			{
				SOCKADDR *tmp;
				AddLink( &globalNetworkData.addresses, tmp = AllocAddr() );
				((uintptr_t*)tmp)[-1] = test->ai_addrlen;
				MemCpy( tmp, test->ai_addr, test->ai_addrlen );
				//lprintf( "initialize addres..." );
				//DumpAddr( "blah", tmp );
			}
		}
	}
#endif
#endif

void LoadNetworkAddresses( void ) {
	// Declare and initialize variables
	PIP_INTERFACE_INFO pInfo;
	pInfo = (IP_INTERFACE_INFO *) malloc( sizeof(IP_INTERFACE_INFO) );
	DWORD dwRetVal = 0;

	PIP_ADAPTER_INFO pAdapterInfo;
	PIP_ADAPTER_INFO pAdapter = NULL;

	ULONG ulOutBufLen = sizeof (IP_ADAPTER_INFO);
	pAdapterInfo = New(IP_ADAPTER_INFO);
	if (pAdapterInfo == NULL) {
		lprintf("Error allocating memory needed to call GetAdaptersinfo\n");
		free( pInfo );
		return;
	}
	// Make an initial call to GetAdaptersInfo to get
	// the necessary size into the ulOutBufLen variable
	if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
		Deallocate( PIP_ADAPTER_INFO, pAdapterInfo);
		pAdapterInfo = (IP_ADAPTER_INFO *) NewArray(uint8_t, ulOutBufLen);
		if (pAdapterInfo == NULL) {
			lprintf("Error allocating memory needed to call GetAdaptersinfo\n");
			free( pInfo );
			return;
		}
	}

	if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR) {
		pAdapter = pAdapterInfo;
		while (pAdapter) {

			PIP_ADDR_STRING ipadd = &pAdapter->IpAddressList;
			for( ; ipadd; ipadd = ipadd->Next ) {
			/*
			typedef struct _IP_ADDR_STRING {
			  struct _IP_ADDR_STRING  *Next;
			  IP_ADDRESS_STRING      IpAddress;
			  IP_MASK_STRING         IpMask;
			  DWORD                  Context;
			} IP_ADDR_STRING, *PIP_ADDR_STRING;
			*/
				if( StrCmp( ipadd->IpAddress.String, "0.0.0.0" ) == 0 )
					continue;
				struct interfaceAddress *ia = New( struct interfaceAddress );
				ia->sa = CreateRemote( ipadd->IpAddress.String, 0 );
				ia->saMask = CreateRemote( ipadd->IpMask.String, 0 );
				ia->saBroadcast = AllocAddr();
				ia->saBroadcast->sa_family = ia->sa->sa_family;
				ia->saBroadcast->sa_data[0] = 0;
				ia->saBroadcast->sa_data[1] = 0;
				ia->saBroadcast->sa_data[2] = (ia->sa->sa_data[2] & ia->saMask->sa_data[2]) | (~ia->saMask->sa_data[2]);
				ia->saBroadcast->sa_data[3] = (ia->sa->sa_data[3] & ia->saMask->sa_data[3]) | (~ia->saMask->sa_data[3]);
				ia->saBroadcast->sa_data[4] = (ia->sa->sa_data[4] & ia->saMask->sa_data[4]) | (~ia->saMask->sa_data[4]);
				ia->saBroadcast->sa_data[5] = (ia->sa->sa_data[5] & ia->saMask->sa_data[5]) | (~ia->saMask->sa_data[5]);
				AddLink( &globalNetworkData.addresses, ia );
			}

			pAdapter = pAdapter->Next;
		}
	}
	else {
		lprintf( "GetAdaptersInfo failed with error: %d\n", dwRetVal );
	}
#if 0
https://msdn.microsoft.com/en-us/library/windows/desktop/aa365915%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396
    PIP_ADAPTER_ADDRESSES pAddresses = NULL;
    ULONG outBufLen = 0;
    ULONG Iterations = 0;

    PIP_ADAPTER_ADDRESSES pCurrAddresses = NULL;
    PIP_ADAPTER_UNICAST_ADDRESS pUnicast = NULL;
    PIP_ADAPTER_ANYCAST_ADDRESS pAnycast = NULL;
    PIP_ADAPTER_MULTICAST_ADDRESS pMulticast = NULL;
    IP_ADAPTER_DNS_SERVER_ADDRESS *pDnServer = NULL;
    IP_ADAPTER_PREFIX *pPrefix = NULL;

  do {

        pAddresses = (IP_ADAPTER_ADDRESSES *) MALLOC(outBufLen);
        if (pAddresses == NULL) {
            printf
                ("Memory allocation failed for IP_ADAPTER_ADDRESSES struct\n");
            exit(1);
        }

        dwRetVal =
            GetAdaptersAddresses(AF_UNSPEC
		, GAA_FLAG_SKIP_DNS_SERVER|GAA_FLAG_SKIP_FRIENDLY_NAME|GAA_FLAG_INCLUDE_ALL_INTERFACES
		, NULL, pAddresses, &outBufLen);

        if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
            FREE(pAddresses);
            pAddresses = NULL;
        } else {
            break;
        }

        Iterations++;

    } while ((dwRetVal == ERROR_BUFFER_OVERFLOW) && (Iterations < MAX_TRIES));
#endif

}

#endif

SACK_NETWORK_NAMESPACE_END
