/*
 * Extra functions.  Just for fun.
 *
 * These functions were commenly used in SNMP Applications, so they were
 * put here.
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
#include <ctype.h>
#include <sys/types.h>

#ifdef __BORLANDC__
#define _timeb timeb
#define _ftime ftime
#endif /* __BORLANDC__ */

#ifdef WIN32
//#include <winsock2.h>
//#include <ws2tcpip.h>
#include <sys/timeb.h>
#else /* WIN32 */
#include <netinet/in.h>
//#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#endif /* WIN32 */

#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif /* HAVE_SYS_SOCKIO_H */

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#ifdef HAVE_STRINGS_H
# include <strings.h>
#else /* HAVE_STRINGS_H */
# include <string.h>
#endif /* HAVE_STRINGS_H */

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

#include <snmp/libsnmp.h>
SNMP_NAMESPACE

//static char rcsid[] =
//"$Id: snmp_extra.c,v 1.1.1.1 2001/11/15 01:29:32 panther Exp $";


/* Turn timeticks into a string.  Returns a pointer to the string.
 */
char *uptime_string(u_int timeticks, char *buf)
{
  int seconds, minutes, hours, days;

  timeticks /= 100;
  days = timeticks / (60 * 60 * 24);
  timeticks %= (60 * 60 * 24);

  hours = timeticks / (60 * 60);
  timeticks %= (60 * 60);

  minutes = timeticks / 60;
  seconds = timeticks % 60;

  if (days == 0) {
    sprintf(buf, WIDE("%d:%02d:%02d"), hours, minutes, seconds);
  } else if (days == 1) {
    sprintf(buf, WIDE("%d day, %d:%02d:%02d"), days, hours, minutes, seconds);
  } else {
    sprintf(buf, WIDE("%d days, %d:%02d:%02d"), days, hours, minutes, seconds);
  }

  return(buf);
}

#define NUM_NETWORKS	16   /* max number of interfaces to check */
#ifndef IFF_LOOPBACK
#define IFF_LOOPBACK 0
#endif
#define LOOPBACK    0x7f000001

#ifdef WIN32

char *winsock_startup(void)
{
#ifdef LOG_DEBUG
   OutputDebugString( WIDE("Winsock Startup in SNMPLIB\n"));
#endif
  if( NetworkWait(NULL,0, 8 ) )
     return NULL;
  else
     return "Unable to start network.";
}

void winsock_cleanup(void)
{
   NetworkQuit();
}

int gettimeofday(//tv, tz)
struct timeval *tv,
struct timezone *tz)
{
    struct _timeb timebuffer;

    _ftime(&timebuffer);
    tv->tv_usec = timebuffer.millitm;
    tv->tv_sec = timebuffer.time;
    return(1);
}

#else /* WIN32 */
/*
u_int myaddress(void)
{
  PCLIENT sd;
  struct ifconf ifc;
  struct ifreq conf[NUM_NETWORKS], *ifrp, ifreq;
  struct sockaddr_in *in_addr;
  int count;
  int interfaces;		// number of interfaces returned by ioctl 

  sd = ConnectUDP( NULL, 0, 
                   ;
  if (sd < 0) {
    snmp_set_api_error(SNMPERR_OS_ERR);
    return(0);
  }

  ifc.ifc_len = sizeof(conf);
  ifc.ifc_buf = (caddr_t)conf;
  if (ioctl(sd, SIOCGIFCONF, (char *)&ifc) < 0){
    close(sd);
    snmp_set_api_error(SNMPERR_OS_ERR);
    return(0);
  }

  ifrp = ifc.ifc_req;
  interfaces = ifc.ifc_len / sizeof(struct ifreq);
  for(count = 0; count < interfaces; count++, ifrp++){
    //    ifreq = *ifrp;
    ifrp = &ifreq;
    if (ioctl(sd, SIOCGIFFLAGS, (char *)&ifreq) < 0)
      continue;
    in_addr = (struct sockaddr_in *)&ifrp->ifr_addr;
    if ((ifreq.ifr_flags & IFF_UP)
	&& (ifreq.ifr_flags & IFF_RUNNING)
	&& !(ifreq.ifr_flags & IFF_LOOPBACK)
	&& in_addr->sin_addr.s_addr != LOOPBACK){
      close(sd);
      return(in_addr->sin_addr.s_addr);
    }
  }
  close(sd);
  snmp_set_api_error(SNMPERR_OS_ERR);
  return(0);
}
*/
#endif /* WIN32 */

/***************************************************************************
 * 
 * Convert between various OID formats
 *
 ***************************************************************************/
int mib_TxtToOid(char *Buf, oid **OidP, int *LenP)
{
  char *bufp;
  oid *NewOid;
  char *S;
  int len = 0;
  int index = 0;
  int val = 0;

  /* Calculate initial length */
  for (bufp = Buf; *bufp; bufp++)
    if (*bufp == '.')
      len++;
  if (Buf[0] != '.') len++;

  *LenP = len;
  *OidP = (oid *)malloc(sizeof(oid) * len);
  if (*OidP == NULL) {
    snmp_set_api_error(SNMPERR_OS_ERR);
    return(0);
  }
  NewOid = *OidP;
  
  /* Now insert bytes */
  S = Buf;
  if (*S == '.') S++;
  
#ifdef DEBUG_MIB_TTO
    printf("**********\n");
    printf("Txt: '%s'\n", Buf);
    printf("Len: (%d)\n", len);
#endif
  while(1) {

    /* Keep this value around */
    val = atoi(S);
    while(!((*S == '\0') || (*S == '.'))) {
      if (!isdigit((int)*S)) {
	snmp_set_api_error(SNMPERR_INVALID_TXTOID);	
	return(0);
      }
      S++;
    }
#ifdef DEBUG_MIB_TTO
    printf("%.2d: %d\n", index, val);
#endif
    /* End of a number. */
    NewOid[index++] = val;
    val = 0;

    /* End of string? */
    if (*S == '\0') {
#ifdef DEBUG_MIB_TTO
    printf("Returning.\n");
#endif
      return(1);
    }

    /* Nope.  On to the next. */
    S++;
  }
}

int mib_OidToTxt(oid *O, int OidLen, char *Buf, int BufLen)
{
  int x;
  int Len;
  char *bufp;
  int MaxLen = (BufLen - 3); /* Safety margin */

  bufp = Buf;
  Len = 0;

  for (x=0; x<OidLen; x++) {

    sprintf(bufp, WIDE(".%u"), O[x]);
    Len += strlen(bufp);
    bufp += strlen(bufp);

    if (Len > MaxLen) {
      snmp_set_api_error(SNMPERR_TOO_LONG);
      return(0);
    }
  }

  return(1);
}
SNMP_NAMESPACE_END
// $Log: $
