/*
 * SNMP MiniClient:  Small client routines, for rapid creation of
 * SNMP applications.
 *
 */
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
 * Authors: Erikas Aras Napjus <erikas+@cmu.edu>
 *          Ryan Troll <ryan@andrew.cmu.edu>
 * 
 **********************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>

#ifdef WIN32
#include <string.h>
#include <winsock2.h>
#include <memory.h>
#else /* WIN32 */
#include <sys/time.h>
#include <netinet/in.h>
#endif /* WIN32 */

#ifdef HAVE_STRINGS_H
# include <strings.h>
#else /* HAVE_STRINGS_H */
# include <string.h>
#endif /* HAVE_STRINGS_H */

#ifdef HAVE_MALLOC_H
# include <malloc.h>
#endif /* HAVE_MALLOC_H */

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif /* HAVE_UNISTD_H */

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif /* HAVE_SYS_SELECT_H */

#include <snmp/libsnmp.h>
#if 0
/* CMU SNMP Library */
#include "asn1.h"
#include "snmp_pdu.h"
#include "snmp_session.h"
#include "snmp-internal.h"
#include "snmp_api_error.h"
#include "snmp_vars.h"
#include "snmp_api.h"
#include "snmp_client.h"
#include "snmp_error.h"
#include "snmp_extra.h"
#include "snmp_compat.h"

#include "mini-client.h"
#endif
SNMP_NAMESPACE
//static char rcsid[] = 
//"$Id: mini-client.c,v 1.1.1.1 2001/11/15 01:29:36 panther Exp $";

/**********************************************************************/

/* The return array
 */
static void **ReturnArray;
static int   *ReturnLens;
static int ArraySize = 0;

/* Shouldn't have to set it to NULL, as this is called when we are about
 * to fill it back in.
 */
#define RETURN_FREE(x) if (ReturnArray[x] != NULL) free(ReturnArray[x]);

void init_mini_snmp_client(int m)
{
  ArraySize = m;
  ReturnArray = (void **)malloc(sizeof(void *)*m);
  ReturnLens  = (int *)malloc(sizeof(int)*m);
  /* Set all pointers to NULL */
  memset(ReturnArray, '\0', sizeof(void *)*m);
  memset(ReturnLens,  '\0', sizeof(int)*m);
}

void close_mini_snmp_client(void)
{
  int x;
  for (x=0; x<ArraySize; x++) RETURN_FREE(x);
  free(ReturnArray);
  free(ReturnLens);
}

int snmp_mini_response_len(int i)
{
  return(ReturnLens[i]);
}

/**********************************************************************/

void *snmp_mini_open(char *gateway, int port, char *community)
{
  struct snmp_session session, *ss;

  memset((char *)&session, '\0', sizeof(struct snmp_session));
  session.community = (u_char *)community;
  session.community_len = strlen((char *)community);
  session.retries = SNMP_DEFAULT_RETRIES;
  session.timeout = SNMP_DEFAULT_TIMEOUT;
  session.peername = (char *) gateway;
  session.remote_port = port;
  snmp_synch_setup(&session);
  ss = snmp_open(&session);
  if (ss == NULL) {
    return NULL;
  }
  return((void *)ss);
}

void snmp_mini_close(void *ss)
{
  snmp_close((struct snmp_session *)ss);
}

/**********************************************************************/

/* snmpset - set a variable on the specified host
 *
 * WARNING: this will only set one variable at a time
 * WARNING: this will only reliably set INTEGER-style variables
 *
 * Returns 1 upon success, 0 otherwise.
 */
int snmp_mini_set_int(char *gateway, char *community, 
		      oid *query, int querylen,
		      int value, void *old_ss)
{
  struct snmp_session *ss;
  struct snmp_pdu *pdu, *response;
  int status;

  /* Use a cached session */
  if (old_ss == NULL) {
    ss = (struct snmp_session *)snmp_mini_open(gateway, SNMP_DEFAULT_REMPORT, community);
    if (ss == NULL)
      return(0);
  } else {
    ss = (struct snmp_session *)old_ss;
  }

  /* Create the set PDU */
  pdu = snmp_pdu_create(SNMP_PDU_SET);
  if (pdu == NULL)
    return(0);

  /* Add this OID. */
  snmp_add_null_var(pdu, query, querylen);

Retry:
  /* Set it */
  pdu->variables->type = INTEGER;
  pdu->variables->val.integer = (int *) malloc(sizeof(int));
  *(pdu->variables->val.integer) = value;
  pdu->variables->val_len = sizeof(int);

  /* Send the request */
  status = snmp_synch_response(ss, pdu, &response);
  if (status == STAT_SUCCESS){
    if (response->errstat != SNMP_ERR_NOERROR) {
      snmp_set_api_error(SNMPERR_PACKET_ERR);

      /* Remove the bad part of the request */
      pdu = snmp_fix_pdu(response, SNMP_PDU_GET);

	/* If there is still something to do, send it */
      if (pdu != NULL)
	goto Retry;
      else
	goto Failure;
    }

  } else if (status == STAT_TIMEOUT) {
    snmp_set_api_error(SNMPERR_NO_RESPONSE);
    goto Failure;

  } else {
    /* status == STAT_ERROR */
    snmp_set_api_error(SNMPERR_GENERR);
    goto Failure;
  }

  if (response)
    snmp_free_pdu(response);

  /* Only close the session if it wasn't cached */
  if (old_ss == NULL)
    snmp_mini_close(ss);

  return 1;

Failure:
  /* Only close the session if it wasn't cached */
  if (old_ss == NULL)
    snmp_mini_close(ss);
  return 0;
}

/**********************************************************************/

/* snmpsetstr - set a display string variable on the specified host
**
** (returns -1 on failure; otherwise 0)
*/
int snmp_mini_set_str(char *gateway, char *community, 
		      oid *query, int querylen,
		      char *value, int length,
		      void *old_ss)
{
  struct snmp_session *ss;
  struct snmp_pdu *pdu, *response;
  int status;
  
  /* Use a cached session */
  if (old_ss == NULL) {
    ss = (struct snmp_session *)snmp_mini_open(gateway, SNMP_DEFAULT_REMPORT, community);
    if (ss == NULL)
      return(0);
  } else {
    ss = (struct snmp_session *)old_ss;
  }

  pdu = snmp_pdu_create(SNMP_PDU_SET);
  if (pdu == NULL)
    return(0);

  snmp_add_null_var(pdu, query, querylen);
  
Retry:
  pdu->variables->type = STRING;
  pdu->variables->val.string = (u_char *) malloc(length);
  strcpy((char *)pdu->variables->val.string, value);
  pdu->variables->val_len = length;

  status = snmp_synch_response(ss, pdu, &response);
  if (status == STAT_SUCCESS) {
    if (response->errstat != SNMP_ERR_NOERROR) {
      pdu = snmp_fix_pdu(response, SNMP_PDU_GET);
      if (pdu != NULL)
	goto Retry;
      else
	goto Failure;
    }
  } else
    goto Failure;
  
  if (response)
    snmp_free_pdu(response);

  /* Only close the session if it wasn't cached */
  if (old_ss == NULL)
    snmp_mini_close(ss);
  return 1;

Failure:
  /* Only close the session if it wasn't cached */
  if (old_ss == NULL)
    snmp_mini_close(ss);
  return 0;
}

/**********************************************************************/

/* snmpget - requests specified variable(s)
 *
 * (this supports both multiple requests and all normal types)
 *
 * Returns a pointer to the answer array, or NULL.
 */
void **snmp_mini_get(char *gateway, char *community, 
		     int NumQueries, oid **Queries, int *QueryLens,
		     void *old_ss)
{
  struct snmp_session *ss;
  struct snmp_pdu *pdu, *response;
  struct variable_list *vars;
  int status;

  /* Number of items returned */
  int count;

  /* Use a cached session */
  if (old_ss == NULL) {
    ss = (struct snmp_session *)snmp_mini_open(gateway, SNMP_DEFAULT_REMPORT, community);
    if (ss == NULL)
      return(NULL);
  } else {
    ss = (struct snmp_session *)old_ss;
  }

  /* Create the get PDU */
  pdu = snmp_pdu_create(SNMP_PDU_GET);

  /* Add all of the requests */
  for(count = 0; count < NumQueries; count++) {
    snmp_add_null_var(pdu, Queries[count], QueryLens[count]);
  }
  
  /* Now send it */
  count = 0;
  status = snmp_synch_response(ss, pdu, &response);
  if (status == STAT_SUCCESS) {
    if (response->errstat == SNMP_ERR_NOERROR) {
      for (vars = response->variables; vars; vars = vars->next_variable) {
	RETURN_FREE(count);
	ReturnArray[count] = (void *) vars->val.string;
	ReturnLens[count] = vars->val_len;
	count++;

	/* This will stop the actual value pointer from being
	 * freed with the variable list is freed.
	 */
	vars->val.objid = NULL;
      }
    } else {
      snmp_set_api_error(SNMPERR_GENERR);
      goto Failure;
    }
  }
  else {
    if (status == STAT_TIMEOUT)
      snmp_set_api_error(SNMPERR_NO_RESPONSE);
    else
      snmp_set_api_error(SNMPERR_GENERR);
    goto Failure;
  }

  if (response)
    snmp_free_pdu(response);

  /* Only close the session if it wasn't cached */
  if (old_ss == NULL)
    snmp_mini_close(ss);

  return ReturnArray;

Failure:
  /* Only close the session if it wasn't cached */
  if (old_ss == NULL)
    snmp_mini_close(ss);
  return NULL;
}

/**********************************************************************/

/* snmpgetnext - requests the next variable after the specified oid
**
** WARNING: this only supports single queries at this time
** WARNING: this only works reliably with STRING-type variables
**
** (return value is 0 on failure; otherwise, length of string)
*/
int snmp_mini_getnext(char *gateway, char *community, 
		      oid *Query, int QueryLen,
		      oid *ResponseOID, void **ResponseData,
		      void *old_ss)
{
  struct snmp_session *ss;
  struct snmp_pdu *pdu, *response;
  struct variable_list *vars;
  int length;
  int status;

  /* Use a cached session */
  if (old_ss == NULL) {
    ss = (struct snmp_session *)snmp_mini_open(gateway, SNMP_DEFAULT_REMPORT, community);
    if (ss == NULL)
      return(0);
  } else {
    ss = (struct snmp_session *)old_ss;
  }

  pdu = snmp_pdu_create(SNMP_PDU_GETNEXT);
  if (pdu == NULL)
    return(0);

  snmp_add_null_var(pdu, Query, QueryLen);

retry:
  status = snmp_synch_response(ss, pdu, &response);
  if (status == STAT_SUCCESS) {
    if (response->errstat == SNMP_ERR_NOERROR) {
      for (vars = response->variables; vars; vars = vars->next_variable) {

	/* Copy the OID name */
	memcpy((char *)ResponseOID, (char *)vars->name, 	       
	       vars->name_length * sizeof(oid));
	length = vars->name_length;

	*ResponseData = (void *)vars->val.string;
	ReturnLens[0] = vars->val_len;
	vars->val.string = NULL; /* So snmp_var_free doesn't free it */
      }
    } else {
      snmp_set_api_error(SNMPERR_PACKET_ERR);
      if ((pdu = snmp_fix_pdu(response, SNMP_PDU_GET)) != NULL)
	goto retry;
      else
	goto Failure;
    }
  } else {
    if (status == STAT_TIMEOUT)
      snmp_set_api_error(SNMPERR_NO_RESPONSE);
    else 
      snmp_set_api_error(SNMPERR_GENERR);
    goto Failure;
  }
  
  if (response)
    snmp_free_pdu(response);


  /* Only close the session if it wasn't cached */
  if (old_ss == NULL)
    snmp_mini_close(ss);

  return length;

Failure:
  /* Only close the session if it wasn't cached */
  if (old_ss == NULL)
    snmp_mini_close(ss);
  return 0;
}
SNMP_NAMESPACE_END
// $Log: $
