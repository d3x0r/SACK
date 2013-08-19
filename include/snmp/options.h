/* -*- c++ -*- */
#ifndef _SNMP_OPTIONS_H_
#define _SNMP_OPTIONS_H_

/***************************************************************************
 *
 *           Copyright 1998 by Carnegie Mellon University
 * 
 *                       All Rights Reserved
 * 
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that copyright notice and this permission
 * notice appear in supporting documentation, and that the name of
 * Carnegie Mellon University not be used in advertising or publicity
 * pertaining to distribution of the software without specific,
 * written prior permission.
 * 
 * CARNEGIE MELLON UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL Carnegie Mellon University BE LIABLE
 * FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 * 
 * Author: Ryan Troll <ryan+@andrew.cmu.edu>
 * 
 * $Id: options.h,v 1.1.1.1 2001/11/15 01:26:44 panther Exp $
 * 
 ***************************************************************************/

/* Maximum buffer length used in the MIB printing routines.
 *
 * Attempting to print a variable of length greater than this will cause your
 * application to crash.
 */
#define MAX_RETURN_BUFLEN 4096

/* Max length of a token read by the new MIB parser.
 */
#define TOKENSIZE 128

#endif /* _SNMP_OPTIONS_H_ */
// $Log: $
