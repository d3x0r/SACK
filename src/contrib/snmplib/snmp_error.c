/*
 * The possible SNMP Error messages, from the SNMP Protocol Operations
 * [ RFC 1905 ]
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
 **********************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <snmp/libsnmp.h>
SNMP_NAMESPACE


//static char rcsid[] = 
//"$Id: snmp_error.c,v 1.1.1.1 2001/11/15 01:29:36 panther Exp $";

/* Definitions from RFC 1905, sections 4.2.* */
static char *error_string[19] = {

  "No error has occured.",
  "Response message would have been too large.",
  "There is no such variable name in this MIB.", /* ? */
  "The value given has the wrong type, length, or value", /* ? */
  "This variable is read only", /* ? */
  "A general failure occured.",

  /* SNMPv2 Set Errors */
  "Manager does not have access to this variable.",
  "Sent variable type doesn't match requested variable's type in the agent.",
  "Sent variable length doesn't match requested variable's length in the agent.",
  "Variable encoding does not match encoding specified by MIB.",
  "Variable value does not make sense.",

  "Variable may never be created.",
  "Specified falue for variable makes no sense.",
  "Unable to allocate resources to perform assignment.",
  "Commit failed.  All assignments have been undone.",
  "Commit failed, and undo failed.  Some assignments have been committed.",
  "AUTHORIZATIONERROR",
  "Requested Object Identifier is not writable."
  "Variable may not be created at this time.",

};

char *snmp_errstring(int errstat)
{
  if ((errstat <= (SNMP_ERR_INCONSISTENTNAME)) && 
      (errstat >= (SNMP_ERR_NOERROR))) {
    return error_string[errstat];
  } else {
    return "Unknown Error";
  }
}
SNMP_NAMESPACE_END
// $Log: $
