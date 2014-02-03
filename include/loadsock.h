/* Includes networking as appropriate for the target platform. Providing
   compatibility definitions as are lacking between platforms...
   or perhaps appropriate name aliasing to the correct types.            */
#ifndef INCLUDED_SOCKET_LIBRARY
#define INCLUDED_SOCKET_LIBRARY

#if defined( _WIN32 ) || defined( __CYGWIN__ )
//#ifndef __cplusplus_cli
#ifdef UNDER_CE
#define USE_WSA_EVENTS
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <sack_types.h>

#if defined( MINGW_SUX )
/* Address information */
typedef struct addrinfoA {
    int             ai_flags;
    int             ai_family;
    int             ai_socktype;
    int             ai_protocol;
    size_t          ai_addrlen;
    char            *ai_canonname;
    struct sockaddr *ai_addr;
    struct addrinfoA *ai_next;
} ADDRINFOA;
typedef ADDRINFOA   *PADDRINFOA;
typedef struct addrinfoW {
    int                 ai_flags;
    int                 ai_family;
    int                 ai_socktype;
    int                 ai_protocol;
    size_t              ai_addrlen;
    PWSTR               ai_canonname;
    struct sockaddr     *ai_addr;
    struct addrinfoW    *ai_next;
} ADDRINFOW;
typedef ADDRINFOW   *PADDRINFOW;
#ifdef UNICODE
typedef ADDRINFOW   ADDRINFOT;
typedef ADDRINFOW   *PADDRINFOT;
#else
typedef ADDRINFOA   ADDRINFOT;
typedef ADDRINFOA   *PADDRINFOT;
#endif
typedef ADDRINFOA   ADDRINFO;
typedef ADDRINFOA   *LPADDRINFO;

#endif

#ifdef __CYGWIN__
// just need this simple symbol
typedef int socklen_t;
#endif
//#endif
#elif defined( __LINUX__ )
#include <sack_types.h>
#if defined( FBSD )
#include <sys/types.h>
#endif
#include <netinet/in.h> // INADDR_ANY/NONE
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <netdb.h>
#include <errno.h>
#include <net/if.h>
#define SOCKET int
#define SOCKADDR struct sockaddr
#define SOCKET_ERROR -1
//#define HWND int // unused params...
#define WSAEWOULDBLOCK EAGAIN
#define INVALID_SOCKET -1
#define WSAAsynchSelect( a,b,c,d ) (0)
#define WSAGetLastError()  (errno) 
#define closesocket(s) close(s)
typedef struct hostent *PHOSTENT;

#ifndef __LINUX__
#define INADDR_ANY (-1)
#define INADDR_NONE (0)
#endif

struct win_in_addr {
        union {
                struct { _8 s_b1,s_b2,s_b3,s_b4; } S_un_b;
                struct { _16 s_w1,s_w2; } S_un_w;
                _32 S_addr;
        } S_un;
   
#define s_addr  S_un.S_addr
                                /* can be used for most tcp & ip code */
#define s_host  S_un.S_un_b.s_b2
                                /* host on imp */
#define s_net   S_un.S_un_b.s_b1
                                /* network */
#define s_imp   S_un.S_un_w.s_w2
                                /* imp */
#define s_impno S_un.S_un_b.s_b4
                                /* imp # */
#define s_lh    S_un.S_un_b.s_b3
                                /* logical host */
};


struct win_sockaddr_in {
        short   sin_family;
        _16 sin_port;
        struct  win_in_addr sin_addr;
        char    sin_zero[8];
};

typedef struct win_sockaddr_in SOCKADDR_IN;
#endif


#endif

// $Log: loadsock.h,v $
// Revision 1.7  2005/01/27 08:09:25  panther
// Linux cleaned.
//
// Revision 1.6  2003/06/04 11:38:01  panther
// Define PACKED
//
// Revision 1.5  2003/03/25 08:38:11  panther
// Add logging
//
