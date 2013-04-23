/* -*- c++ -*- */
#ifndef _SNMP_MINI_CLIENT_H_
#define _SNMP_MINI_CLIENT_H_

/***************************************************************************
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
 * $Id: mini-client.h,v 1.1.1.1 2001/11/15 01:26:44 panther Exp $
 * 
 ***************************************************************************/

#ifdef WIN32
#define DLLEXPORT EXPORT_METHOD
#else  /* WIN32 */
#define DLLEXPORT
#endif /* WIN32 */

SNMP_NAMESPACE

  /* Arg:  Max number of items returned in any request */
DLLEXPORT void init_mini_snmp_client(int);
DLLEXPORT void close_mini_snmp_client(void);
DLLEXPORT int  snmp_mini_response_len(int); 

/* Args:  Gateway, Port, Community */
DLLEXPORT void *snmp_mini_open(char *, int, char *);
/* Args: Returned pointer */
DLLEXPORT void  snmp_mini_close(void *);

DLLEXPORT int snmp_mini_set_int(char *, char *, oid *, int, int, void *);
DLLEXPORT int snmp_mini_set_str(char *, char *, oid *, int, char *, int, void *);

DLLEXPORT void **snmp_mini_get(char *, char *, int, oid **, int *, void *);

/* Gateway, Community, QueryP, QueryLen, ResponseP, ResponseDataP, S */
DLLEXPORT int    snmp_mini_getnext(char *, char *, oid *, int, oid *, void **, void *);

SNMP_NAMESPACE_END

#endif /* _SNMP_MINI_CLIENT_H_ */
// $Log: $
