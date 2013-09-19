/*
 * This module keeps track of some very basic MIBII variables.  It will
 * be useful for implementing the SNMP MIB. [ RFC 1213 ]
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
 * Author: Ryan Troll <ryan+@andrew.cmu.edu>
 * 
 **********************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <snmp/libsnmp.h>
//#include "mibii.h"
SNMP_NAMESPACE

//static char rcsid[] = 
//"$Id: mibii.c,v 1.1.1.1 2001/11/15 01:29:36 panther Exp $";

static int _snmpInASNParseErrs = 0;

int snmpInASNParseErrs(void)
{
  return(_snmpInASNParseErrs);
}

int snmpInASNParseErrs_Add(int i)
{
  _snmpInASNParseErrs += i;
  return(_snmpInASNParseErrs);
}


static int _snmpInBadVersions = 0;

int snmpInBadVersions(void)
{
  return(_snmpInBadVersions);
}

int snmpInBadVersions_Add(int i)
{
  _snmpInBadVersions += i;
  return(_snmpInBadVersions);
}
SNMP_NAMESPACE_END
// $Log: $
