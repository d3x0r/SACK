/*
 * SNMP Network Packet Debugging
 *
 * Dumps the network packets to stdout.
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

#include <stdio.h>
#include <sys/types.h>
#include <ctype.h>

#ifdef WIN32
#include <winsock2.h>
#else /* WIN32 */
#include <netinet/in.h>
//#include <sys/socket.h>
#include <arpa/inet.h>
#endif /* WIN32 */

#include <snmp/libsnmp.h>
SNMP_NAMESPACE
//static char rcsid[] = 
//"$Id: snmp_dump.c,v 1.1.1.1 2001/11/15 01:29:34 panther Exp $";

static int _snmp_dump_packet = 0;

void snmp_dump_packet(int i)
{
  _snmp_dump_packet = i;
}

void snmp_dump(u_char *buf, int buflen, 
	       char *how, struct in_addr who)
{
  int count, row;

  /* Return if there's nothing to do */
  if (_snmp_dump_packet == 0)
    return;

  printf("%s %u bytes <-> %s:\n", 
	 how, buflen, 
	 inet_ntoa(who));

  count = 0;
  while(count < buflen) {
    row = 0;
    for(;count + row < buflen && row < 16; row++){
      printf("%02X ", buf[count + row]);
    }
    while(row++ < 16)
      printf("   ");
    printf("  ");
    row = 0;
    for(;count + row < buflen && row < 16; row++){
      if (isprint(buf[count + row]))
	printf("%c", buf[count + row]);
      else
	printf(".");
    }
    printf("\n");
    count += row;
  }
  printf("\n\n");
}
SNMP_NAMESPACE_END
// $Log: $
