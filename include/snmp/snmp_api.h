#ifndef _SNMP_API_H_
#include <snmp/libsnmp.h>
#ifndef _SNMP_API_H_

#define _SNMP_API_H_

/***********************************************************
	Copyright 1998 by Carnegie Mellon University

                      All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of CMU not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

CMU DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
CMU BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.
******************************************************************/

/*
 * snmp_api.h - API for access to snmp.
 */



/*
 * Set fields in session and pdu to the following to get a default or unconfigured value.
 */
#define SNMP_DEFAULT_COMMUNITY_LEN  0	 /* to get a default community name */
#define SNMP_DEFAULT_RETRIES	    (-1) /* If specified, default to values */
#define SNMP_DEFAULT_TIMEOUT	    (-1) /* listed in snmp_api.c */
#define SNMP_DEFAULT_REMPORT	    0
#define SNMP_DEFAULT_PEERNAME	    NULL
#define SNMP_DEFAULT_ENTERPRISE_LENGTH	0
#define SNMP_DEFAULT_TIME	    0
#define SNMP_DEFAULT_MAXREPETITIONS 5
#define SNMP_DEFAULT_MACREPEATERS   0

/* For backwards compatibility. :( */
#define OLD_SNMP_DEFAULT_RETRIES	    3
#define OLD_SNMP_DEFAULT_TIMEOUT	    1

#ifdef WIN32
#define DLLEXPORT EXPORT_METHOD
#else  /* WIN32 */
#define DLLEXPORT
#endif /* WIN32 */


SNMP_NAMESPACE


  /* Parse the buffer pointed to by arg3, of length arg4, into pdu arg2.
   *
   * Returns the community of the incoming PDU, or NULL
   */
DLLEXPORT u_char *snmp_parse(struct snmp_session *, struct snmp_pdu *,
			     u_char *, int);

/* Encode pdu arg2 into buffer arg3.  arg4 contains the size of
 * the buffer.
 */
DLLEXPORT int snmp_build(struct snmp_session *, struct snmp_pdu *,
			 u_char *, int *);

/*
 * struct snmp_session *snmp_open(session)
 *	struct snmp_session *session;
 * 
 * Sets up the session with the snmp_session information provided
 * by the user.  Then opens and binds the necessary UDP port.
 * A handle to the created session is returned (this is different than
 * the pointer passed to snmp_open()).  On any error, NULL is returned
 * and snmp_errno is set to the appropriate error code.
 */
DLLEXPORT struct snmp_session *snmp_open(struct snmp_session *);

/*
 * int snmp_close(session)
 *     struct snmp_session *session;
 * 
 * Close the input session.  Frees all data allocated for the session,
 * dequeues any pending requests, and closes any sockets allocated for
 * the session.  Returns 0 on error, 1 otherwise.
 */
DLLEXPORT int snmp_close(struct snmp_session *);


/*
 * int snmp_send(session, pdu)
 *     struct snmp_session *session;
 *     struct snmp_pdu	*pdu;
 * 
 * Sends the input pdu on the session after calling snmp_build to create
 * a serialized packet.  If necessary, set some of the pdu data from the
 * session defaults.  Add a request corresponding to this pdu to the list
 * of outstanding requests on this session, then send the pdu.
 * Returns the request id of the generated packet if applicable, otherwise 1.
 * On any error, 0 is returned.
 * The pdu is freed by snmp_send() unless a failure occured.
 */
DLLEXPORT int snmp_send(struct snmp_session *, struct snmp_pdu *);

/* see snmp_session.h for definition of callback function */
DLLEXPORT int snmp_async_send(struct snmp_session * , struct snmp_pdu * ,
			      snmp_callback , void * );


/*
 * void snmp_read(fdset)
 *     fd_set  *fdset;
 * 
 * Checks to see if any of the fd's set in the fdset belong to
 * snmp.  Each socket with it's fd set has a packet read from it
 * and snmp_parse is called on the packet received.  The resulting pdu
 * is passed to the callback routine for that session.  If the callback
 * routine returns successfully, the pdu and it's request are deleted.
 */
DLLEXPORT void snmp_read(fd_set *);


/*
 * int snmp_select_info(numfds, fdset, timeout, block)
 * int *numfds;
 * fd_set   *fdset;
 * struct timeval *timeout;
 * int *block;
 *
 * Returns info about what snmp requires from a select statement.
 * numfds is the number of fds in the list that are significant.
 * All file descriptors opened for SNMP are OR'd into the fdset.
 * If activity occurs on any of these file descriptors, snmp_read
 * should be called with that file descriptor set.
 *
 * The timeout is the latest time that SNMP can wait for a timeout.  The
 * select should be done with the minimum time between timeout and any other
 * timeouts necessary.  This should be checked upon each invocation of select.
 * If a timeout is received, snmp_timeout should be called to check if the
 * timeout was for SNMP.  (snmp_timeout is idempotent)
 *
 * Block is 1 if the select is requested to block indefinitely, rather than time out.
 * If block is input as 1, the timeout value will be treated as undefined, but it must
 * be available for setting in snmp_select_info.  On return, if block is true, the value
 * of timeout will be undefined.
 *
 * snmp_select_info returns the number of open sockets.  (i.e. The number of sessions open)
 */
DLLEXPORT int snmp_select_info(int *, fd_set *, struct timeval *, int *);

/*
 * void snmp_timeout();
 * 
 * snmp_timeout should be called whenever the timeout from snmp_select_info expires,
 * but it is idempotent, so snmp_timeout can be polled (probably a cpu expensive
 * proposition).  snmp_timeout checks to see if any of the sessions have an
 * outstanding request that has timed out.  If it finds one (or more), and that
 * pdu has more retries available, a new packet is formed from the pdu and is
 * resent.  If there are no more retries available, the callback for the session
 * is used to alert the user of the timeout.
 */
DLLEXPORT void snmp_timeout(void);


/*
 * This routine must be supplied by the application:
 *
 * int callback(operation, session, reqid, pdu, magic)
 * int operation;
 * struct snmp_session *session;    The session authenticated under.
 * int reqid;			    The request id of this pdu (0 for TRAP)
 * struct snmp_pdu *pdu;	    The pdu information.
 * void *magic			    A link to the data for this routine.
 *
 * Returns 1 if request was successful, 0 if it should be kept pending.
 * Any data in the pdu must be copied because it will be freed elsewhere.
 * Operations are defined below:
 */

DLLEXPORT void snmp_api_stats(void *);

/* caller must free returned string */
DLLEXPORT void snmp_error(struct snmp_session *, int *, int *, char **);

/*
 * single session API.
 *
 * These functions perform similar actions as snmp_XX functions,
 * but operate on a single session only.
 *
 * Synopsis:

	void * sessp;
	struct snmp_session session, *ss;
	struct snmp_pdu *pdu, *response;

	snmp_sess_init(&session);
	session.retries = ...
	session.remote_port = ...
	snmp_synch_setup(&session);
	sessp = snmp_sess_open(&session);
	ss = snmp_sess_session(sessp);
	if (ss == NULL)
		exit(1);
	...
	if (ss->community) free(ss->community);
	ss->community = strdup(gateway);
	ss->community_len = strlen(gateway);
	...
	snmp_sess_synch_response(sessp, pdu, &response);
	...
	snmp_synch_reset(&session);
	snmp_sess_close(sessp);

 * See also:
 * snmp_sess_synch_response, in snmp_client.h.

 * Notes:
 *  1. Invoke snmp_sess_pointer after snmp_open.
 *  2. snmp_sess_pointer return value is an opaque pointer.
 *  3. Do NOT free memory returned by snmp_sess_pointer.
 *  4. Replace snmp_send(ss,pdu) with snmp_sess_send(sessp,pdu)
 */

DLLEXPORT void   snmp_sess_init      (struct snmp_session *);
DLLEXPORT void * snmp_sess_open      (struct snmp_session *);
DLLEXPORT struct snmp_session * snmp_sess_session   (void *);

/* use return value from snmp_sess_open as void * parameter */

DLLEXPORT int    snmp_sess_send      (void *, struct snmp_pdu *);
DLLEXPORT int    snmp_sess_async_send(void *, struct snmp_pdu *,
                                         snmp_callback, void *);
DLLEXPORT int    snmp_sess_select_info(void *, int *, fd_set *,
                                         struct timeval *, int *);
#ifdef __LINUX__
#define CALLBACK
typedef char *PBYTE, BYTE;
typedef unsigned long DWORD;
#endif
void CPROC snmp_sess_read( PCLIENT pc,
                              PBYTE pBuf, int nSize,
                              struct sockaddr_in *sa);

// DLLEXPORT void   snmp_sess_read      (void *, fd_set *);
DLLEXPORT void   snmp_sess_timeout   (void *);
DLLEXPORT int    snmp_sess_close     (void *);

/* caller must free returned string */
DLLEXPORT void   snmp_sess_error     (void *, int *, int *, char **);

/* end single session API */

SNMP_NAMESPACE_END

#endif /* _SNMP_API_H_ */
#endif /* _SNMP_API_H_ */
// $Log: $
