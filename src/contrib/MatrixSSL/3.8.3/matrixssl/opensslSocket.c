/**
 *	@file    opensslSocket.c
 *	@version a90e925 (tag: 3-8-3-open)
 *
 *	BSD socket implementation for supporting SSL_read, SSL_write, etc....
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

#include <stdlib.h>
#include <stdio.h>

#include "opensslSocket.h"

#ifdef USE_MATRIX_OPENSSL_LAYER
/******************************************************************************/
/*
	Client side. Open a socket connection to a remote ip and port.
	This code is not specific to SSL.
*/
int socketConnect(char *ip, short port, int *err)
{
	struct sockaddr_in	addr;
	int					fd, rc;

	eeTrace("Enter socketConnect\n");
	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		psTraceInfo("Error creating socket\n");
		*err = getSocketError();
		return SOCKET_ERROR;
	}
/*
	Make sure the socket is not inherited by exec'd processes
	Set the REUSEADDR flag to minimize the number of sockets in TIME_WAIT
*/
	fcntl(fd, F_SETFD, FD_CLOEXEC);
	rc = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&rc, sizeof(rc));
	setSocketNodelay(fd);
/*
	Turn on blocking mode for the connecting socket
*/
	setSocketBlock(fd);

	memset((char *) &addr, 0x0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip);
	rc = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
#if WIN
	if (rc != 0) {
#else
	if (rc < 0) {
#endif
		*err = getSocketError();
		return SOCKET_ERROR;
	}
	eeTrace("Exit socketConnect\n");
	return fd;
}

/******************************************************************************/
/*
	Server side.  Set up a listen socket.  This code is not specific to SSL.
*/
int socketListen(short port, int *err)
{
	struct sockaddr_in	addr;
	int					fd;
	int32				rc;

	eeTrace("Enter socketListen\n");
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;
	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		psTraceInfo("Error creating listen socket\n");
		*err = getSocketError();
		return SOCKET_ERROR;
	}
/*
	Make sure the socket is not inherited by exec'd processes
	Set the REUSE flag to minimize the number of sockets in TIME_WAIT
*/
	fcntl(fd, F_SETFD, FD_CLOEXEC);
	rc = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&rc, sizeof(rc));

	if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		psTraceInfo("Can't bind socket. Port in use or insufficient privilege\n");
		*err = getSocketError();
		return SOCKET_ERROR;
	}
	if (listen(fd, SOMAXCONN) < 0) {
		psTraceInfo("Error listening on socket\n");
		*err = getSocketError();
		return SOCKET_ERROR;
	}
	printf("Listening for SSL connections on port %d\n", port);
	eeTrace("Exit socketListen\n");
	return fd;
}

/******************************************************************************/
/*
	Server side.  Accept a new socket connection off our listen socket.
	This code is not specific to SSL.
*/
int socketAccept(int listenfd, int32 *err)
{
	struct sockaddr_in	addr;
	int					fd;
	unsigned int		len;
	
	eeTrace("Enter socketAccept\n");
/*
	Wait(blocking)/poll(non-blocking) for an incoming connection
*/
	len = sizeof(addr);
	if ((fd = accept(listenfd, (struct sockaddr *)&addr, &len))
			== SOCKET_ERROR) {
		*err = getSocketError();
		if (*err != WOULD_BLOCK) {
			psTraceIntInfo("Error %d accepting new socket\n", (int)*err);
		}
		return SOCKET_ERROR;
	}
/*
	fd is the newly accepted socket. Disable Nagle on this socket.
	Set blocking mode as default
*/
/*	fprintf(stdout, "Connection received from %d.%d.%d.%d\n",
		addr.sin_addr.S_un.S_un_b.s_b1,
		addr.sin_addr.S_un.S_un_b.s_b2,
		addr.sin_addr.S_un.S_un_b.s_b3,
		addr.sin_addr.S_un.S_un_b.s_b4);
*/
	setSocketNodelay(fd);
	setSocketBlock(fd);
	eeTrace("Exit socketAccept\n");
	return fd;
}

/******************************************************************************/
/*
	Set the socket to non blocking mode and perform a few extra tricks
	to make sure the socket closes down cross platform
*/
void socketClose(int sock)
{
	char	buf[32];

	eeTrace("Enter socketClose\n");
	if (sock != SOCKET_ERROR) {
		setSocketNonblock(sock);
		if (shutdown(sock, 1) >= 0) {
			while (recv(sock, buf, sizeof(buf), 0) > 0);
		}
		close(sock);
	}
	eeTrace("Exit socketClose\n");
}

//#define FRAG_TEST
#ifdef FRAG_TEST
/******************************************************************************/
/*
	INTERNAL FUNCTION
	Randomly send less than the requested number of bytes
	Simulates a slow or congested network interface that may fragment records
 */
static int32 fragSend(int fd, unsigned char *buf, int32 len, int32 flags)
{
	unsigned char   c;
	int32          nlen;
	struct timespec rqtp;

	rqtp.tv_sec = 0;
	rqtp.tv_nsec = 1000000000 >> 4; /* 1/16 second */
	nlen = len;
	if (nlen > 10) {
		psGetEntropy(&c, 1, NULL);
		nlen = (int32)c;
		if (nlen == 0) {
			nlen++;
		} else if (nlen > len) {
			//nlen = (unsigned char)len;
			nlen = 10;
		}
	}
	printf("sending fragment %d/%d bytes\n", nlen, len);
	len = send(fd, buf, nlen, flags);
	/* Sleep to give time for network to flush */
	nanosleep(&rqtp, NULL);
	return len;
}
#endif /* TEST  */

/******************************************************************************/
/*
	Perform a blocking write of data to a socket
*/
int socketWrite(int sock, unsigned char *out, int len)
{
	ssize_t bytes;

	eeTrace("Enter socketWrite\n");
	printf("Trying to write %d bytes on sock %d\n", len, sock);
#ifdef FRAG_TEST
	bytes = fragSend(sock, out, len, MSG_NOSIGNAL);
#else
	bytes = send(sock, out, len, MSG_NOSIGNAL);
#endif
	if (bytes == SOCKET_ERROR) {
		return -1;
	}
	printf("send return code %d\n", (int)bytes);
	eeTrace("Exit socketWrite\n");
	
	return (int)bytes;
}

/******************************************************************************/
/*
	Perform a blocking socket read
*/
int socketRead(int sock, unsigned char *in, int available)
{
	ssize_t		bytes;

	eeTrace("Enter socketRead\n");
	bytes = recv(sock, in, available, MSG_NOSIGNAL);
	if (bytes == SOCKET_ERROR) {
		return -1;
	}
	eeTrace("Exit socketRead\n");
	return (int)bytes;
}

/******************************************************************************/
/*
	Turn on socket blocking mode (and set CLOEXEC on LINUX for kicks).
*/
void setSocketBlock(int sock)
{
#ifdef WIN32
	int32		block = 0;
	ioctlsocket(sock, FIONBIO, &block);
#endif
#ifdef POSIX
	fcntl(sock, F_SETFL, fcntl(sock, F_GETFL) & ~O_NONBLOCK);
	fcntl(sock, F_SETFD, FD_CLOEXEC);
#endif
}

/******************************************************************************/
/*
	Turn off socket blocking mode.
*/
void setSocketNonblock(int sock)
{
#ifdef WIN32
	int32		block = 1;
	ioctlsocket(sock, FIONBIO, &block);
#endif
#ifdef POSIX
	fcntl(sock, F_SETFL, fcntl(sock, F_GETFL) | O_NONBLOCK);
#endif
}

/******************************************************************************/
/*
	Disable the Nagle algorithm for less latency in RPC
	http://www.faqs.org/rfcs/rfc896.html
	http://www.w3.org/Protocols/HTTP/Performance/Nagle/
*/
void setSocketNodelay(int sock)
{
#ifdef WIN32
	BOOL	tmp = TRUE;
#endif
#ifdef POSIX
	int32		tmp = 1;
#endif /* WIN32 */
	setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&tmp, sizeof(tmp));
}

#endif /* USE_MATRIX_OPENSSL_LAYER */
/******************************************************************************/

