/*
 * RFC 1908: Coexistence between SNMPv1 and SNMPv2
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
 * Author: Ryan Troll <ryan+@andrew.cmu.edu>
 * 
 **********************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#ifdef WIN32
#include <winsock2.h>
#else /* WIN32 */
#include <sys/types.h>
#include <sys/param.h>
#include <netinet/in.h>
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

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif /* HAVE_MALLOC_H */

#include <snmp/libsnmp.h>
#if 0
#include "asn1.h"
#include "snmp_vars.h"
#include "snmp_pdu.h"
#include "snmp_error.h"
#include "snmp_api_error.h"
#endif
SNMP_NAMESPACE

//static char rcsid[] = 
//"$Id: coexistance.c,v 1.1.1.1 2001/11/15 01:29:34 panther Exp $";

/*
 * RFC 1908: Coexistence between SNMPv1 and SNMPv2
 *
 * These convert:
 *
 *   V1 PDUs from an ** AGENT **   to V2 PDUs for an ** MANAGER **
 *   V2 PDUs from an ** MANAGER ** to V1 PDUs for an ** AGENT **
 *
 * We will never convert V1 information from a manager into V2 PDUs.  V1
 * requests are always honored by V2 agents, and the responses will be 
 * valid V1 responses.  (I think. XXXXX)
 *
 */
int snmp_coexist_V2toV1(struct snmp_pdu *PDU)
{
  /* Per 3.1.1:
   */
  switch (PDU->command) {
    
  case SNMP_PDU_GET:
  case SNMP_PDU_GETNEXT:
  case SNMP_PDU_SET:
    return(1);
    break;

  case SNMP_PDU_GETBULK:
    PDU->non_repeaters = 0;
    PDU->max_repetitions = 0;
    PDU->command = SNMP_PDU_GETNEXT;
    return(1);
    break;

  default:
#ifdef STDERR_OUTPUT
    fprintf(stderr, WIDE("Unable to translate PDU %d to SNMPv1!\n"), PDU->command);
#endif
    snmp_set_api_error(SNMPERR_PDU_TRANSLATION);
    return(0);
  }
}

static oid SysUptime[]   = { 1, 3, 6, 1, 2, 1, 1, 3, 0 };
/* SNMPv2:           .1 .3 .6 .1 .6
 * snmpModules:                     .3
 * snmpMib:                            .1
 * snmpMIBObjects:                        .1
 * snmpTrap:                                 .4
 */
static oid TrapOid[]     = { 1, 3, 6, 1, 6, 3, 1, 1, 4, 1, 0};
static oid EnterOid[]    = { 1, 3, 6, 1, 6, 3, 1, 1, 4, 3, 0};
static oid snmpTrapsOid[]= { 1, 3, 6, 1, 6, 3, 1, 1, 5};

int snmp_coexist_V1toV2(struct snmp_pdu *PDU)
{
  /* Per 3.1.2:
   */
  switch (PDU->command) {
    
  case SNMP_PDU_RESPONSE:

    if (PDU->errstat == SNMP_ERR_TOOBIG) {
      snmp_var_free(PDU->variables);
      PDU->variables = NULL;
    }
    return(1);
    break;

  case TRP_REQ_MSG:
    {
      struct variable_list *VarP;
      oid NewOid[MAX_NAME_LEN];
      oid NewOidLen = 0;

      /* Fake SysUptime */
      VarP = snmp_var_new(SysUptime, sizeof(SysUptime) / sizeof(oid));
      if (VarP == NULL)
	return(0);
      VarP->type = ASN_INTEGER;
      *VarP->val.integer = PDU->time;

      /* Fake SNMPTrapOid */
      VarP->next_variable = snmp_var_new(TrapOid, sizeof(TrapOid) / sizeof(oid));
      if (VarP == NULL) {
	snmp_var_free(VarP);
	return(0);
      }

      if (PDU->trap_type == SNMP_TRAP_ENTERPRISESPECIFIC) {
	memcpy(NewOid, PDU->enterprise, PDU->enterprise_length);
	NewOidLen = PDU->enterprise_length;
	NewOid[NewOidLen++] = 0;
	NewOid[NewOidLen++] = PDU->specific_type;
      } else {

	/* Trap prefix */
	NewOidLen = sizeof(snmpTrapsOid) / sizeof(oid);
	memcpy(NewOid, snmpTrapsOid, NewOidLen);

	switch (PDU->trap_type) {
	case SNMP_TRAP_COLDSTART:
	  NewOid[NewOidLen++] = 1;
	  break;
	case SNMP_TRAP_WARMSTART:
	  NewOid[NewOidLen++] = 2;
	  break;
	case SNMP_TRAP_LINKDOWN:
	  NewOid[NewOidLen++] = 3; /* RFC 1573 */
	  break;
	case SNMP_TRAP_LINKUP:
	  NewOid[NewOidLen++] = 4; /* RFC 1573 */
	  break;
	case SNMP_TRAP_AUTHENTICATIONFAILURE:
	  NewOid[NewOidLen++] = 5;
	  break;
	case SNMP_TRAP_EGPNEIGHBORLOSS:
	  NewOid[NewOidLen++] = 6; /* RFC 1213 */
	  break;
	default:
#ifdef STDERR_OUTPUT
	  fprintf(stderr, WIDE("Unable to translate v1 trap type %d!\n"),
		  PDU->trap_type);
#endif
	  snmp_set_api_error(SNMPERR_PDU_TRANSLATION);
	  return(0);
	}
      }

      /* ASSERT:  NewOid contains the new OID. */
      VarP->next_variable->type = ASN_OBJECT_ID;
      VarP->next_variable->val_len = NewOidLen;
      VarP->next_variable->val.string = (u_char*)malloc(sizeof(oid)*NewOidLen);
      if (VarP->next_variable->val.string == NULL) {
	snmp_set_api_error(SNMPERR_OS_ERR);
	return(0);
      }
      memcpy((char *)VarP->next_variable->val.string, 
	     (char *)NewOid, 
	     sizeof(oid)*NewOidLen);
      
      /* Prepend this list */
      VarP->next_variable->next_variable = PDU->variables;
      PDU->variables = VarP;

      /* Finally, append the last piece */

      for(VarP = PDU->variables; VarP->next_variable; VarP=VarP->next_variable)
	; /* Find the last element of the list. */

      VarP->next_variable = snmp_var_new(EnterOid, 
					 sizeof(EnterOid) / sizeof(oid));
      if (VarP->next_variable == NULL)
	return(0);
      VarP->next_variable->type = ASN_OBJECT_ID;
      VarP->next_variable->val_len = PDU->enterprise_length;
      VarP->next_variable->val.string = (u_char *)PDU->enterprise;

      PDU->command = SNMP_PDU_V2TRAP;

    } /* End of trap conversion */
    return(1);
    break;
  default:
#ifdef STDERR_OUTPUT
    fprintf(stderr, WIDE("Unable to translate PDU %d to SNMPv2!\n"), PDU->command);
#endif
    snmp_set_api_error(SNMPERR_PDU_TRANSLATION);
    return(0);
  }
}
SNMP_NAMESPACE_END

// $Log: $
