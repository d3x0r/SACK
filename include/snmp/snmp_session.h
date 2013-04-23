/* -*- c++ -*- */
#ifndef _SNMP_SESSION_H_
#define _SNMP_SESSION_H_

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
 * $Id: snmp_session.h,v 1.1.1.1 2001/11/15 01:26:44 panther Exp $
 * 
 **********************************************************************/
SNMP_NAMESPACE
struct snmp_session {
  int Version; /* SNMP Version for this session */


    u_char  *community;	/* community for outgoing requests. */
    int	    community_len;  /* Length of community name. */
    int	    retries;	/* Number of retries before timeout. */
    int    timeout;    /* Number of uS until first timeout, then exponential backoff */
    char    *peername;	/* Domain name or dotted IP address of default peer */
    u_short remote_port;/* UDP port number of peer. */
    u_short local_port; /* My UDP port number, 0 for default, picked randomly */
  /* This isn't used, but is here so that libraries compiled with this
   * in place still work.
   */
    u_char    *(*authenticator)();

  /* Function to interpret incoming data */
    int	    (*callback)(int, struct snmp_session *, int,
			struct snmp_pdu *, void *); 

    /* Pointer to data that the callback function may consider important */
    void    *callback_magic;
#ifdef REUSE_STRUCTURE
    /* not used in V1-only setups. */
    #define snmp_synch_state authenticator
    /* re-use input-only members to hold error codes. */
    #define s_snmp_errno remote_port
    #define s_errno local_port
#else /* REUSE_STRUCTURE */
    struct synch_state * snmp_synch_state;
    int     s_errno;        /* copy of system errno */
    int     s_snmp_errno;   /* copy of library errno */
#endif /* REUSE_STRUCTURE */
};

typedef int (*snmp_callback) (int, struct snmp_session *, int, struct snmp_pdu *, void *);

#define RECEIVED_MESSAGE   1
#define TIMED_OUT	   2
SNMP_NAMESPACE_END

#endif /* _SNMP_SESSION_H_ */
// $Log: $
