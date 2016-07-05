/**
 *	@file    opensslSocket.h
 *	@version a90e925 (tag: 3-8-3-open)
 *
 *	BSD socket implementation for openssl layer to MatrixSSL.
 */
/*
 *	Copyright (c) 2013-2016 INSIDE Secure Corporation
 *	Copyright (c) PeerSec Networks, 2002-2011
 *	All Rights Reserved
 *
 *	The latest version of this code is available at http://www.matrixssl.org
 *
 *	This software is open source; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This General Public License does NOT permit incorporating this software
 *	into proprietary programs.  If you are unable to comply with the GPL, a
 *	commercial license for this software may be purchased from INSIDE at
 *	http://www.insidesecure.com/
 *
 *	This program is distributed in WITHOUT ANY WARRANTY; without even the
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *	See the GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *	http://www.gnu.org/copyleft/gpl.html
 */
/******************************************************************************/
#ifndef _h_SSLSOCKET
#define _h_SSLSOCKET

#ifdef __cplusplus
extern "C" {
#endif

#include "matrixsslApi.h"

#ifdef USE_MATRIX_OPENSSL_LAYER

#include	<fcntl.h>
#include	<unistd.h>
#include	<string.h>
#include	<errno.h>

/*
	OS specific includes and macros
*/
#ifdef WIN32
	#include	<windows.h>
	#include	<winsock.h>
	#define		fcntl(A, B, C)
	#define		MSG_NOSIGNAL	0
	#define		WOULD_BLOCK	WSAEWOULDBLOCK
	#define		getSocketError()  WSAGetLastError()
#else
	typedef		int	WSADATA;
	#define		WSAStartup(A, B)
	#define		WSACleanup()
	#define		getSocketError() errno
#endif

#ifdef POSIX
	#include	<sys/socket.h>
	#include	<netinet/in.h>
	#include	<netinet/tcp.h>
	#include	<arpa/inet.h>
#ifdef OSX
	#include	<sys/socket.h>
	#define		MSG_NOSIGNAL	0
#endif /* OSX */
#endif /* POSIX */

#define SOCKET_ERROR -1
#define		WOULD_BLOCK	EAGAIN

#ifdef NEEDED
	#define		MAKEWORD(A, B)
	#define		SOCKET_ERRORCKET -1
#endif /* old stuff */

/*
	sslRead and sslWrite status values
*/
#define SSLSOCKET_EOF			0x1
#define SSLSOCKET_CLOSE_NOTIFY	0x2

/*
	Socket apis
*/

extern void socketClose(int sock);
extern int socketWrite(int sock, unsigned char *out, int len);
extern int socketRead(int sock, unsigned char *in, int available);

extern int	socketListen(short port, int *err);
extern int	socketAccept(int listenfd, int *err);
extern int	socketConnect(char *ip, short port, int *err);
extern void		socketShutdown(int sock);

extern void setSocketBlock(int sock);
extern void setSocketNonblock(int sock);
extern void setSocketNodelay(int sock);


#ifdef __cplusplus
}
#endif

#endif /* USE_MATRIX_OPENSSL_LAYER */
#endif /* _h_SSLSOCKET */

/******************************************************************************/
