/*
 * Error routines concerning the error status of the SNMP API.
 *
 * Sometimes things don't work out the way we wanted.
 *
 */
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
 ***************************************************************************/
#include <sack_types.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <sys/types.h>

#ifdef HAVE_STRINGS_H
# include <strings.h>
#else /* HAVE_STRINGS_H */
# include <string.h>
#endif /* HAVE_STRINGS_H */

#ifdef WIN32
#include <memory.h>
#include <winsock2.h>
#else /* WIN32 */
#include <netinet/in.h>
#endif /* WIN32 */

#include <snmp/libsnmp.h>
#if 0
#include "asn1.h"
#include "snmp-internal.h"
#include "snmp_impl.h"
#include "snmp_api_error.h"
#include "snmp_pdu.h"
#include "snmp_session.h"
#endif
SNMP_NAMESPACE

//static char rcsid[] = 
//"$Id: snmp_api_error.c,v 1.1.1.1 2001/11/15 01:29:36 panther Exp $";

/***************************************************************************
 *
 ***************************************************************************/

int snmp_errno = 0;

static char *api_errors[17] = {
  "Unknown Error",
  "Generic Error", 
  "Invalid local port",
  "Unknown host",
  "Unknown session",
  "Too Long",

  "Encoding ASN.1 Information", /* 6 */
  "Decoding ASN.1 Information", /* 7 */
  "PDU Translation error",
  "OS Error",
  "Invalid Textual OID",
  
  "Unable to fix PDU",
  "Unsupported SNMP Type",
  "Unable to parse PDU",
  "Packet Error",
  "No Response From Host",


  "Unknown Error"
};

void snmp_set_api_error(int x)
{
  snmp_errno = x;
}

char *snmp_api_error(int err)
{
  int foo = (err * -1);
  if ((err > SNMPERR_GENERR) || (err < SNMPERR_LAST))
    foo=0;

  return(api_errors[foo]);
}

int snmp_api_errno(void)
{
  return(snmp_errno);
}

char *api_errstring(int snmp_errnumber)
{
  return(snmp_api_error(snmp_errnumber));
}

/*
 * snmp_error - return error data
 * Inputs :  address of errno, address of snmp_errno, address of string
 * Caller must free the string returned after use.
 */
void
snmp_error(//psess, p_errno, p_snmp_errno, p_str)
    struct snmp_session *psess,
    int * p_errno,
    int * p_snmp_errno,
    char ** p_str )
{
    char buf[512];
    int snmp_errnumber;

    if (psess == NULL) return;
    if (p_errno) *p_errno = psess->s_errno;
    if (p_snmp_errno) *p_snmp_errno = psess->s_snmp_errno;
    if (p_str == NULL) return;

    snmp_errnumber = psess->s_snmp_errno;
    if ((snmp_errnumber > SNMPERR_GENERR) || (snmp_errnumber < SNMPERR_LAST)){
	sprintf(buf, WIDE("Unknown Error %d"), -snmp_errnumber);
    } else {
	strcpy(buf, api_errors[-snmp_errnumber]);
    }

    /* append a useful system errno interpretation. */
    if (psess->s_errno)
        sprintf (&buf[strlen(buf)], WIDE(" (%s)"), strerror(psess->s_errno));
    *p_str = (char *)strdup(buf);
}
SNMP_NAMESPACE_END

// $Log: $
