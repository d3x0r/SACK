#ifndef _SNMP_H_
#define _SNMP_H_

/*
 * Definitions for the Simple Network Management Protocol (RFC 1067).
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
 * $Id: snmp.h,v 1.1.1.1 2001/11/15 01:26:44 panther Exp $
 * 
 **********************************************************************/

#include <sys/types.h>
#include <netinet/in.h>

/* These come first */
#include <snmp/asn1.h>
#include <snmp/snmp_error.h>
#include <snmp/mibii.h>
#include <snmp/snmp_extra.h>
#include <snmp/snmp_dump.h>

/* The various modules */
#include <snmp/snmp_vars.h>
#include <snmp/snmp_pdu.h>
#include <snmp/snmp_msg.h>

/* I didn't touch this */
#include <snmp/snmp_session.h>

/* Other functions */
#include <snmp/snmp_coexist.h>
#include <snmp/version.h>
#include <snmp/snmp_api_error.h>
#include <snmp/mini-client.h>

/* Other stuff I didn't touch */
#include <snmp/snmp_impl.h>
#include <snmp/snmp_api.h>
#include <snmp/snmp_client.h>
#include <snmp/snmp-internal.h>
#include <snmp/mib.h>
#include <snmp/parse.h>
#include <snmp/snmp_compat.h>

#endif /* _SNMP_H_ */
// $Log: $
