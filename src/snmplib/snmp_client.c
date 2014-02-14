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
 * $Id: snmp_client.c,v 1.1.1.1 2001/11/15 01:29:32 panther Exp $
 * 
 **********************************************************************/

/* Our autoconf variables */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>

#ifdef WIN32
#include <winsock2.h>
#else /* WIN32 */
#include <sys/param.h>
#include <netinet/in.h>
//#include <sys/socket.h>
#endif /* WIN32 */

#ifdef HAVE_STRINGS_H
# include <strings.h>
#else /* HAVE_STRINGS_H */
# include <string.h>
#endif /* HAVE_STRINGS_H */

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#ifdef HAVE_BSTRING_H
#include <bstring.h>
#endif /* HAVE_BSTRING_H */

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif /* HAVE_SYS_SELECT_H */

//static char rcsid[] =
//"$Id";
#include <snmp/libsnmp.h>
#if 0
#include "asn1.h"
#include "snmp_error.h"
#include "snmp_pdu.h"
#include "snmp_vars.h"

#include "snmp_session.h"
#include "snmp_api.h"
#include "snmp_client.h"
#include "snmp_api_error.h"
#endif
SNMP_NAMESPACE

/* Define these here, as they aren't defined normall under
 * cygnus Win32 stuff.
 */
#undef timerclear
#define timerclear(tvp) (tvp)->tv_sec = (tvp)->tv_usec = 0

/* #define DEBUG_CLIENT 1 */

extern int snmp_errno;

int snmp_synch_input(int Operation, 
		     struct snmp_session *Session, 
		     int RequestID, 
		     struct snmp_pdu *pdu,
		     void *magic)
{
  struct variable_list *var;
  struct synch_state *state = (struct synch_state *)magic;
  struct snmp_pdu *newpdu;

  struct variable_list *varPtr;

#ifdef DEBUG_CLIENT
  printf("CLIENT %x: Synchronizing input.\n", (unsigned int)pdu);
#endif

  /* Make sure this is the proper request
   */
  if (RequestID != state->reqid)
    return 0;
  state->waiting = 0;

  /* Did we receive a Get Response
   */
  if (Operation == RECEIVED_MESSAGE && pdu->command == SNMP_PDU_RESPONSE) {

    /* clone the pdu */
    state->pdu = newpdu = snmp_pdu_clone(pdu);
    newpdu->variables = 0;

    /* Clone all variables */
    var = pdu->variables;
    if (var != NULL) {
      newpdu->variables = snmp_var_clone(var);

      varPtr = newpdu->variables;

      /* While there are more variables */
      while(var->next_variable) {

	/* Clone the next one */
	varPtr->next_variable = snmp_var_clone(var->next_variable);

	/* And move on */
	var    = var->next_variable;
	varPtr = varPtr->next_variable;

      }
      varPtr->next_variable = NULL;
    }
    state->status = STAT_SUCCESS;
    snmp_errno = 0;  /* XX all OK when msg received ? */
    Session->s_snmp_errno = 0;
  } else if (Operation == TIMED_OUT) {
    state->pdu = NULL; /* XX from other lib ? */
    state->status = STAT_TIMEOUT;
    snmp_errno = SNMPERR_NO_RESPONSE;
    Session->s_snmp_errno = SNMPERR_NO_RESPONSE;
  }
  return 1;
}

int snmp_synch_response(struct snmp_session *Session, 
			struct snmp_pdu *PDU,
			struct snmp_pdu **ResponsePDUP)
{
  struct synch_state *state = Session->snmp_synch_state;
//  int block;
  DWORD dwStart;

  state->reqid = snmp_send(Session, PDU);
  if (state->reqid == 0) {
    *ResponsePDUP = NULL;
    snmp_free_pdu(PDU);
    return STAT_ERROR;
  }
  state->waiting = 1;
  state->status  = STAT_SUCCESS;
  dwStart = GetTickCount();
  while( state->waiting &&
         state->status == STAT_SUCCESS ) {
     // this does the timeout...
     if( ( GetTickCount() - dwStart ) > 5000 ) 
     {
        state->status = STAT_TIMEOUT;
        break; // and fail...
     }
     Sleep( 1 ); // let the rest of the box run for a while...
  }

  *ResponsePDUP = state->pdu; // not sure what I do with the first PDU...
  return state->status;
}

int snmp_sess_synch_response(void * sessp,
			struct snmp_pdu *PDU,
			struct snmp_pdu **ResponsePDUP)
{
  struct snmp_session *Session;
  struct synch_state *state;
//  int numfds, count;
//  fd_set fdset;
//  struct timeval timeout, *tvp;
//  int block;
  DWORD dwStart;

  Session = snmp_sess_session(sessp);
  state = Session->snmp_synch_state;

  state->reqid = snmp_sess_send(sessp, PDU);
  if (state->reqid == 0) {
    *ResponsePDUP = NULL;
    snmp_free_pdu(PDU);
    return STAT_ERROR;
  }
  state->waiting = 1;

   dwStart = GetTickCount();
  while(state->waiting) {
     // this does the timeout...
     if( ( GetTickCount() - dwStart ) > 5000 ) break; // and fail...
     Sleep( 10 ); // let the rest of the box run for a while...
  }

  *ResponsePDUP = state->pdu;
  return state->status;
}

void snmp_synch_reset(struct snmp_session *Session)
{
  if (Session && Session->snmp_synch_state)
     free(Session->snmp_synch_state);
}

void snmp_synch_setup(struct snmp_session *Session)
{
  struct synch_state *rp = (struct synch_state *)malloc(sizeof(struct synch_state));

  rp->waiting = 0;
  rp->pdu     = NULL;
  rp->status  = 0;
  Session->snmp_synch_state = rp;
//  Session->callback = snmp_synch_input;
//  Session->callback_magic = (void *)rp;
}
SNMP_NAMESPACE_END
// $Log: $
