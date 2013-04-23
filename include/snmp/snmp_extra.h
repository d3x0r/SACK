/* -*- c++ -*- */
#ifndef _SNMP_EXTRA_H_
#define _SNMP_EXTRA_H_

/**********************************************************************
 *
 *           Copyright 1998 by Carnegie Mellon University
 * 
 *                       All Rights Reserved
 * 
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of CMU not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 * 
 * CMU DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * CMU BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 * 
 * Author: Ryan Troll <ryan+@andrew.cmu.edu>
 * 
 * $Id: snmp_extra.h,v 1.1.1.1 2001/11/15 01:26:44 panther Exp $
 * 
 **********************************************************************/

/*
 * Applications that use this library on Windows NT
 * are responsible for starting and stopping the WOSA.
 * All applications are encouraged to invoke
 * SOCK_STARTUP before the first SNMP library call,
 * and SOCK_CLEANUP just before every exit() or abort() call.
 */
 
#ifdef WIN32
#define DLLEXPORT EXPORT_METHOD
#define SOCK_STARTUP winsock_startup()
#define winsock_start winsock_startup
#define SOCK_CLEANUP winsock_cleanup()
#else  /* WIN32 */
#define DLLEXPORT
#define SOCK_STARTUP
#define SOCK_CLEANUP
#endif /* WIN32 */

SNMP_NAMESPACE

DLLEXPORT char   *uptime_string(u_int, char *);
DLLEXPORT u_int   myaddress(void);

DLLEXPORT int  mib_TxtToOid(char *, oid **, int *);      /* .1.3.6   to OID */ 
DLLEXPORT int  mib_OidToTxt(oid *, int, char *, int);    /* OID to .1.3.6   */

#ifdef WIN32
DLLEXPORT char * winsock_startup(void);
DLLEXPORT void winsock_cleanup(void);

/* do not export gettimeofday nor setenv from the library. */
#define gettimeofday snmp_gettime
DLLEXPORT int snmp_gettime(struct timeval *, struct timezone *);
#endif /* WIN32 */

SNMP_NAMESPACE_END

#endif /* _SNMP_EXTRA_H_ */
// $Log: $
