#ifndef _SNMP_CLIENT_H_
#define _SNMP_CLIENT_H_

/*
 * snmp_client.h
 */
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
SNMP_NAMESPACE

struct synch_state {
    int	waiting;
    int status;
/* status codes */
#define STAT_SUCCESS	0
#define STAT_ERROR	1
#define STAT_TIMEOUT 2
    int reqid;
    struct snmp_pdu *pdu;
};

#ifdef WIN32
#define DLLEXPORT EXPORT_METHOD
#else  /* WIN32 */
#define DLLEXPORT
#endif /* WIN32 */

/* Synchronize Input with Agent */
DLLEXPORT int  snmp_synch_input(int, struct snmp_session *, int,
				struct snmp_pdu *, void *);

/* Synchronize Response with Agent */
DLLEXPORT int  snmp_synch_response(struct snmp_session *, struct snmp_pdu *, 
				   struct snmp_pdu **);

/* Single session version of above */
DLLEXPORT int  snmp_sess_synch_response(void *, struct snmp_pdu *,
					struct snmp_pdu **);

/* Synchronize Setup */
DLLEXPORT void snmp_synch_setup(struct snmp_session *);
DLLEXPORT void snmp_synch_reset(struct snmp_session *);

SNMP_NAMESPACE_END

#endif /* _SNMP_CLIENT_H_ */

// $Log: $
