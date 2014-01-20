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
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>

#ifdef WIN32
#include <memory.h>
#include <winsock2.h>
#else /* WIN32 */
#include <sys/param.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
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

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif /* HAVE_MALLOC_H */

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

#ifdef HAVE_STRINGS_H
# include <strings.h>
#else /* HAVE_STRINGS_H */
# include <string.h>
#endif /* HAVE_STRINGS_H */

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif /* HAVE_SYS_SELECT_H */

#include <snmp/libsnmp.h>
#if 0
#include "asn1.h"

#include "snmp-internal.h"
#include "snmp_impl.h"

#include "mibii.h"
#include "snmp_dump.h"
#include "snmp_error.h"
#include "snmp_vars.h"
#include "snmp_pdu.h"
#include "snmp_msg.h"

#include "snmp_session.h"

#include "snmp_api.h"
#include "snmp_client.h"
#include "snmp_api_error.h"
#include "snmp_api_util.h"
#include "snmp_extra.h"
#endif
extern int snmp_errno;

SNMP_NAMESPACE

//static char rcsid[] = 
//"$Id: snmp_api.c,v 1.1.1.1 2001/11/15 01:29:32 panther Exp $";

/*#define DEBUG_API 1*/

/*
 * RFC 1906: Transport Mappings for SNMPv2
 */


oid default_enterprise[] = {1, 3, 6, 1, 4, 1, 3, 1, 1}; /* enterprises.cmu.systems.cmuSNMP */

#define DEFAULT_COMMUNITY   "public"
#define DEFAULT_RETRIES	    4
#define DEFAULT_TIMEOUT	    2000000L
#define DEFAULT_REMPORT	    SNMP_PORT
#define DEFAULT_ENTERPRISE  default_enterprise
#define DEFAULT_TIME	    0

extern int snmp_errno;


struct session_list *Sessions = NULL;

/*
 * Get initial request ID for all transactions.
 */
static int Reqid = 0;
static int default_s_port = 0;
//static BYTE byBuffer[4096];

static void init_snmp(void) 
{
  struct servent *servp;
  struct timeval tv;

  if (Reqid) return;
  Reqid = 1; /* To stop other threads from doing this as well */

  servp = getservbyname("snmp", WIDE("udp"));
  if (servp)
    default_s_port = ntohs(servp->s_port);

  gettimeofday(&tv, (struct timezone *)0);

#ifdef WIN32
#define random rand
#define srandom srand
#endif /* WIN32 */
  srandom(tv.tv_sec ^ tv.tv_usec);

  Reqid = random();
}



/*
 * Free each element in the input request list.
 */
static void free_request_list(//rp)
    struct request_list *rp )
{
    struct request_list *orp;

    while(rp){
	orp = rp;
	rp = rp->next_request;
	if (orp->pdu != NULL)
	    snmp_free_pdu(orp->pdu);
	free((char *)orp);
    }
}

/**********************************************************************/

/*
 * Sets up the session with the snmp_session information provided
 * by the user.  Then opens and binds the necessary UDP port.
 * A handle to the created session is returned (this is different than
 * the pointer passed to snmp_open()).  On any error, NULL is returned
 * and snmp_errno is set to the appropriate error code.
 */
struct snmp_session *snmp_open(struct snmp_session *session)
{
  struct session_list *slp;

  slp = (struct session_list *)snmp_sess_open(session);
  if (!slp) return NULL;

  { /*MTCRITICAL_RESOURCE*/
    slp->next = Sessions;
    Sessions = slp;
  }
  return (slp->session);
}

/* The "spin-free" version of snmp_open */
void *snmp_sess_open(struct snmp_session * in_session)
{
  struct session_list *slp;
  struct snmp_internal_session *isp;
  struct snmp_session *session;
  u_char *cp, *tmpcp;
  PCLIENT sd;
  int tmplen;
//  u_int addr;
//  struct sockaddr_in me;
//  struct hostent *hp;

  if (Reqid == 0)
    init_snmp();

  /* Copy session structure and link into list */
  slp = (struct session_list *)malloc(sizeof(struct session_list));
  if (slp == NULL) {
    snmp_set_api_error(SNMPERR_OS_ERR);
    in_session->s_snmp_errno = SNMPERR_OS_ERR;
    return(NULL);
  }
  memset((char *)slp, '\0', sizeof(struct session_list));

  /* Internal session */
  isp = (struct snmp_internal_session *)malloc(sizeof(struct snmp_internal_session));
  if (isp == NULL) {
    snmp_set_api_error(SNMPERR_OS_ERR);
    in_session->s_snmp_errno = SNMPERR_OS_ERR;
    snmp_sess_close(slp);
    return(NULL);
  }
  memset((char *)isp, '\0', sizeof(struct snmp_internal_session));

  slp->internal     = isp;
  slp->internal->sd = NULL;  /* mark it not set */

  /* The actual session */
  slp->session = (struct snmp_session *)malloc(sizeof(struct snmp_session));
  if (slp->session == NULL) {
    snmp_set_api_error(SNMPERR_OS_ERR);
    in_session->s_snmp_errno = SNMPERR_OS_ERR;
    snmp_sess_close(slp);
    return(NULL);
  }

  memcpy((char *)slp->session, (char *)in_session, sizeof(struct snmp_session));
  session = slp->session;

  /*
   * session now points to the new structure that still contains pointers to
   * data allocated elsewhere.  Some of this data is copied to space malloc'd
   * here, and the pointer replaced with the new one.
   */

  if (session->peername != NULL) {
    cp = (u_char *)malloc((unsigned)strlen(session->peername) + 1);
    if (cp == NULL) {
      snmp_set_api_error(SNMPERR_OS_ERR);
      in_session->s_snmp_errno = SNMPERR_OS_ERR;
      snmp_sess_close(slp);
      return(NULL);
    }

    strcpy((char *)cp, session->peername);
    session->peername = (char *)cp;
  }

  /* Fill in defaults if necessary */
  tmpcp = session->community;
  tmplen = session->community_len;
  if (tmplen == 0) {
	  tmpcp = (u_char*)DEFAULT_COMMUNITY;
	  tmplen = strlen((char*)tmpcp);
  }
  cp = (u_char *)malloc(tmplen + 1);
#if 0
  if (session->community_len != SNMP_DEFAULT_COMMUNITY_LEN) {
    cp = (u_char *)malloc((unsigned)session->community_len);
    if (cp)
      memcpy((char *)cp, (char *)session->community, session->community_len);
  } else {
    session->community_len = strlen(DEFAULT_COMMUNITY);
    cp = (u_char *)malloc((unsigned)session->community_len);
    if (cp)
      memcpy((char *)cp, (char *)DEFAULT_COMMUNITY, 
	     session->community_len);
  }
#endif
  if (cp == NULL) {
    snmp_set_api_error(SNMPERR_OS_ERR);
    in_session->s_snmp_errno = SNMPERR_OS_ERR;
    snmp_sess_close(slp);
    return(NULL);
  }

  memmove(cp, tmpcp, tmplen);
  session->community_len = tmplen;
  session->community = cp;	/* replace pointer with pointer to new data */

  if ((session->retries == SNMP_DEFAULT_RETRIES) ||
      (session->retries == OLD_SNMP_DEFAULT_RETRIES))
    session->retries = DEFAULT_RETRIES;
  if ((session->timeout == SNMP_DEFAULT_TIMEOUT) ||
      (session->timeout == OLD_SNMP_DEFAULT_TIMEOUT))
    session->timeout = DEFAULT_TIMEOUT;
  isp->requests = NULL;

  /* Set up connections */
  if( !session->remote_port )
     session->remote_port = default_s_port;
  sd = ConnectUDP( NULL, 0, 
                   session->peername, session->remote_port, 
                   (cReadCompleteEx)snmp_sess_read,  // automatic callback when data received...
                   NULL );
//  sd = socket(AF_INET, SOCK_DGRAM, 0);
  if (!sd ) {
    snmp_set_api_error(SNMPERR_OS_ERR);
    in_session->s_snmp_errno = SNMPERR_OS_ERR;
    in_session->s_errno = errno;
    snmp_sess_close(slp);
    return(NULL);
  }
  SetNetworkLong( sd, 0, (DWORD)slp );
//   SetNetworkLong( sd, 1, malloc( 4096 ) ); buffer is set in read...

  isp->sd = sd;

#ifdef SO_BSDCOMPAT
  /* Patch for Linux.  Without this, UDP packets that fail get an ICMP
   * response.  Linux turns the failed ICMP response into an error message
   * and return value, unlike all other OS's.
   */
  {
    int one=1;
    setsockopt(sd, SOL_SOCKET, SO_BSDCOMPAT, &one, sizeof(one));
  }
#endif /* SO_BSDCOMPAT */
/*
  isp->sd = sd;
  if (session->peername != SNMP_DEFAULT_PEERNAME) {
    if ((addr = inet_addr(session->peername)) != -1) {
      memcpy((char *)&isp->addr.sin_addr, (char *)&addr, 
	     sizeof(isp->addr.sin_addr));
    } else {
      hp = gethostbyname(session->peername);
      if (hp == NULL){
	snmp_errno = SNMPERR_BAD_ADDRESS;
	in_session->s_snmp_errno = SNMPERR_BAD_ADDRESS;
	in_session->s_errno = errno;
	snmp_sess_close(slp);
	return(NULL);
      } else {
	memcpy((char *)&isp->addr.sin_addr, (char *)hp->h_addr, 
	       hp->h_length);
      }
    }

    isp->addr.sin_family = AF_INET;
    if (session->remote_port == SNMP_DEFAULT_REMPORT) {
      if (default_s_port) {
      	isp->addr.sin_port = default_s_port;
      } else {
      	isp->addr.sin_port = htons(SNMP_PORT);
      }
    } else {
      isp->addr.sin_port = htons(session->remote_port);
    }
  } else {
    isp->addr.sin_addr.s_addr = SNMP_DEFAULT_ADDRESS;
  }

  memset(&me, '\0', sizeof(me));
  me.sin_family = AF_INET;
  me.sin_addr.s_addr = INADDR_ANY;
  me.sin_port = htons(session->local_port);
  if (bind(sd, (struct sockaddr *)&me, sizeof(me)) != 0) {
    snmp_errno = SNMPERR_BAD_LOCPORT;
    in_session->s_snmp_errno = SNMPERR_BAD_LOCPORT;
    in_session->s_errno = errno;
    snmp_sess_close(slp);
    return(NULL);
  }
*/
  return((void *)slp);
}

static void
_snmp_free(char * cp)
{
    if (cp)
	free(cp);
}

/*
 * Close the input session.  Frees all data allocated for the session,
 * dequeues any pending requests, and closes any sockets allocated for
 * the session.  Returns 0 on error, 1 otherwise.
 */
int
snmp_sess_close(void * sessp)
{
    struct session_list *slp = (struct session_list *)sessp;

    if (slp == NULL)
	return 0;

    if (slp->internal) {
	   if (slp->internal->sd )
	   {
         DWORD dwLong;
#ifdef LOG_DEBUG
         TCHAR msg[256];
#endif
         dwLong = GetNetworkLong( slp->internal->sd, 1 );
#ifdef LOG_DEBUG
         wsprintf( msg, WIDE("Closing Session Removing NetBuf: %08x\n"), dwLong );
         OutputDebugString( msg );
#endif
         free( (void*)dwLong );

   	   RemoveClient(slp->internal->sd);
	   }
	   free_request_list(slp->internal->requests);
    }

    _snmp_free((char *)slp->session->peername);
    _snmp_free((char *)slp->session->community);
    _snmp_free((char *)slp->session);
    _snmp_free((char *)slp->internal);
    _snmp_free((char *)slp);

    return 1;
}


int snmp_close(struct snmp_session *session)
{
  struct session_list *slp = NULL, *oslp = NULL;

  { /*MTCRITICAL_RESOURCE*/
    if (Sessions->session == session){	/* If first entry */
      slp = Sessions;
      Sessions = slp->next;
    } else {
	for(slp = Sessions; slp; slp = slp->next){
	    if (slp->session == session){
		if (oslp)   /* if we found entry that points here */
		    oslp->next = slp->next;	/* link around this entry */
		break;
	    }
	    oslp = slp;
	}
    }
  } /*END MTCRITICAL*/

  if (slp == NULL) {
    snmp_errno = SNMPERR_BAD_SESSION;
    return(0);
    }
  return(snmp_sess_close((void *)slp));
}

/*
 * Takes a session and a pdu and serializes the ASN PDU into the area
 * pointed to by packet.  out_length is the size of the data area available.
 * Returns the length of the encoded packet in out_length.  If an error
 * occurs, -1 is returned.  If all goes well, 0 is returned.
 */
int
snmp_build(//session, pdu, packet, out_length)
    struct snmp_session	*session,
    struct snmp_pdu	*pdu,
    u_char *packet,
    int			*out_length)
{
    u_char *bufp;

    bufp = snmp_msg_Encode(packet, out_length,
			   session->community, session->community_len, 
			   session->Version,
			   pdu);

    if (bufp == NULL)
      return(-1);

    return(0);
}

/*
 * Parses the packet recieved on the input session, and places the data into
 * the input pdu.  length is the length of the input packet.  If any errors
 * are encountered, NULL is returned.  If not, the community is.
 */
u_char *snmp_parse(struct snmp_session *session, 
	       struct snmp_pdu *pdu, 
	       u_char *data, 
	       int length)
{
    u_char Community[128];
    u_char *bufp;
    int CommunityLen = 128;

    /* Decode the entire message. */
    data = snmp_msg_Decode(data, &length, 
			   Community, &CommunityLen, 
			   &session->Version, pdu);
    if (data == NULL)
	return(NULL);

   bufp = (u_char *)malloc(CommunityLen+1);
   if (bufp == NULL)
    return(NULL);

   strcpy((char *)bufp, (char *)Community);
   return(bufp);
}

/*
 * Sends the input pdu on the session after calling snmp_build to create
 * a serialized packet.  If necessary, set some of the pdu data from the
 * session defaults.  Add a request corresponding to this pdu to the list
 * of outstanding requests on this session, then send the pdu.
 * Returns the request id of the generated packet if applicable, otherwise 1.
 * On any error, 0 is returned.
 * The pdu is freed by snmp_send() unless a failure occured.
 */
int snmp_send(struct snmp_session *session, struct snmp_pdu *pdu)
{
  return(snmp_async_send(session, pdu, NULL, NULL));
}

int snmp_sess_send(void *sessp, struct snmp_pdu *pdu)
{
  return(snmp_sess_async_send(sessp, pdu, NULL, NULL));
}


/*
 * int snmp_async_send(session, pdu, callback, cb_data)
 *     struct snmp_session *session;
 *     struct snmp_pdu	*pdu;
 *     snmp_callback callback;
 *     void   *cb_data;
 * 
 * Sends the input pdu on the session after calling snmp_build to create
 * a serialized packet.  If necessary, set some of the pdu data from the
 * session defaults.  Add a request corresponding to this pdu to the list
 * of outstanding requests on this session and store callback and data, 
 * then send the pdu.
 * Returns the request id of the generated packet if applicable, otherwise 1.
 * On any error, 0 is returned.
 * The pdu is freed by snmp_send() unless a failure occured.
 */
void *snmp_sess_pointer(struct snmp_session *session);

int
snmp_async_send(struct snmp_session *session, struct snmp_pdu *pdu,
                snmp_callback callback,
                void *cb_data)
{
  void *sessp = snmp_sess_pointer(session);

  if (sessp == NULL) return 0;

  return(snmp_sess_async_send(sessp, pdu, callback, cb_data));
}

int
snmp_sess_async_send(void *sessp, struct snmp_pdu *pdu,
                snmp_callback callback,
                void *cb_data)
{
    struct session_list *slp = (struct session_list *)sessp;
    struct snmp_session *session;
    struct snmp_internal_session *isp = NULL;
    u_char  packet[PACKET_LENGTH];
    int length = PACKET_LENGTH;
    struct request_list *rp;
    struct timeval tv;

    session = slp->session; isp = slp->internal;
    session->s_snmp_errno = 0;
    session->s_errno = 0;

    if( pdu->command == SNMP_PDU_GET || 
	     pdu->command == SNMP_PDU_GETNEXT ||
	     pdu->command == SNMP_PDU_RESPONSE || 
	     pdu->command == SNMP_PDU_SET) {
      if (pdu->reqid == SNMP_DEFAULT_REQID)
	   { /*MTCRITICAL_RESOURCE*/
	       pdu->reqid = ++Reqid;
	   }
    } else if (pdu->command == SNMP_PDU_INFORM || 
	       pdu->command == SNMP_PDU_GETBULK ||
	       pdu->command == SNMP_PDU_V2TRAP) {

      if (session->Version != SNMP_VERSION_2){
	      snmp_errno = SNMPERR_GENERR;
	      session->s_snmp_errno = SNMPERR_GENERR1;
	      return 0;
      }
      if (pdu->reqid == SNMP_DEFAULT_REQID)
	   { /*MTCRITICAL_RESOURCE*/
	       pdu->reqid = ++Reqid;
	   }

    } else {
	/* fill in trap defaults */
      pdu->reqid = 1;	/* give a bogus non-error reqid for traps */
      if (pdu->enterprise_length == SNMP_DEFAULT_ENTERPRISE_LENGTH) {
	      pdu->enterprise = (oid *)malloc(sizeof(DEFAULT_ENTERPRISE));
	      memcpy( (char *)pdu->enterprise, 
                 (char *)DEFAULT_ENTERPRISE, 
	              sizeof(DEFAULT_ENTERPRISE));
	      pdu->enterprise_length = sizeof(DEFAULT_ENTERPRISE)/sizeof(oid);
      }
      if (pdu->time == SNMP_DEFAULT_TIME)
      	pdu->time = DEFAULT_TIME;
    }
    // one had better open a session to the correct addresss..
    // do NOT attempt to use PDU address or otherwise...
/*
    if (pdu->address.sin_addr.s_addr == SNMP_DEFAULT_ADDRESS) {

      if (isp->addr.sin_addr.s_addr != SNMP_DEFAULT_ADDRESS) {
	memcpy((char *)&pdu->address, (char *)&isp->addr, 		 
	       sizeof(pdu->address));
      } else {
	snmp_errno = SNMPERR_BAD_ADDRESS;
	session->s_snmp_errno = SNMPERR_BAD_ADDRESS;
	return 0;
      }
    }
*/	
    if (snmp_build(session, pdu, packet, &length) < 0) {
      snmp_errno = SNMPERR_GENERR;
      session->s_snmp_errno = SNMPERR_GENERR2;
      return 0;
    }

    snmp_dump(packet, length, WIDE("sending"), pdu->address.sin_addr);

    gettimeofday(&tv, (struct timezone *)0);

//        sendto(isp->sd, (char *)packet, length, 0, 
//	       (struct sockaddr *)&pdu->address, sizeof(pdu->address)) < 0){
    if( !SendUDP( isp->sd, (char *)packet, length ) ) // same address as original...
    {
      snmp_errno = SNMPERR_GENERR;
      session->s_snmp_errno = SNMPERR_GENERR3;
      session->s_errno = errno;
      return 0;
    }

#ifdef DEBUG_API
    printf("LIBSNMP:  Sent PDU %s, Reqid %d\n", 
	   snmp_pdu_type(pdu), pdu->reqid);
#endif

    if (pdu->command == SNMP_PDU_GET || 
	pdu->command == SNMP_PDU_GETNEXT ||
	pdu->command == SNMP_PDU_SET || 
	pdu->command == SNMP_PDU_GETBULK ||
	pdu->command == SNMP_PDU_INFORM) {

#ifdef DEBUG_API
printf("LIBSNMP:  Setting up to recieve a response for reqid %d\n", 
       pdu->reqid);
#endif

      /* set up to expect a response */
      rp = (struct request_list *)malloc(sizeof(struct request_list));
      if (rp == NULL) {
          snmp_errno = SNMPERR_GENERR;
          session->s_snmp_errno = SNMPERR_GENERR4;
          return 0;
      }
      memset((char *)rp, '\0', sizeof(struct request_list));
      /* XX isp needs lock iff multiple threads can handle this session */
      rp->next_request = isp->requests;
      isp->requests = rp;

      rp->pdu = pdu;
      rp->request_id = pdu->reqid;
      
      rp->retries = 0;
      rp->timeout = session->timeout;
      rp->time = tv;
      tv.tv_usec += rp->timeout;
      tv.tv_sec += tv.tv_usec / 1000000L;
      tv.tv_usec %= 1000000L;
      rp->expire = tv;
    }

    return(pdu->reqid);
}

/*
 * Checks to see if any of the fd's set in the fdset belong to
 * snmp.  Each socket with it's fd set has a packet read from it
 * and snmp_parse is called on the packet received.  The resulting pdu
 * is passed to the callback routine for that session.  If the callback
 * routine returns successfully, the pdu and it's request are deleted.
 */
void
snmp_read(fd_set *fdset)
{
//    struct session_list *slp;

//    for(slp = Sessions; slp; slp = slp->next){
//        if (FD_ISSET(slp->internal->sd, fdset))
//	    snmp_sess_read((void *)slp, fdset);
//    }
}

/* Same as snmp_read, but works just one session. */
void CPROC snmp_sess_read( PCLIENT pc
								  , PBYTE pBuf, int nSize
								  , struct sockaddr_in *sa)
{
    struct session_list *slp = (struct session_list*)GetNetworkLong( pc, 0 );
    struct snmp_session *sp;
    struct snmp_internal_session *isp;
//    u_char packet[PACKET_LENGTH];
    struct snmp_pdu *pdu;
    struct request_list *rp, *orp = NULL;
    u_char *bufp;

    if( pBuf )
    {
       sp = slp->session; 
       isp = slp->internal;
       sp->s_snmp_errno = 0;
       sp->s_errno = 0;

  	    snmp_dump(pBuf, nSize, WIDE("received"), sa->sin_addr);

	   pdu = snmp_pdu_create(0);
	   pdu->address = *sa;

	   /* Parse the incoming packet */
	   bufp = snmp_parse(sp, pdu, pBuf, nSize);
	   if (bufp == NULL) {
	     snmp_free_pdu(pdu);
        // get ready for another read...
        doUDPRead( pc, (PBYTE)GetNetworkLong( pc, 1), 4096 );
	     return;
	   }
	   if (sp->community)
	     free(sp->community);
	   sp->community = bufp;
	   sp->community_len = strlen((char*)bufp);

	   if (pdu->command == SNMP_PDU_RESPONSE) {
	     for(rp = isp->requests; rp; rp = rp->next_request) {
	       if (rp->request_id == pdu->reqid) {
             sp->snmp_synch_state->waiting = 0;  // not waiting anymore...
             sp->snmp_synch_state->pdu = pdu;
	         if ((sp -> callback == NULL) ||
		          (sp->callback(RECEIVED_MESSAGE, sp, 
				                  pdu->reqid, pdu, 
				                  sp->callback_magic) == 1)) {
		         /* successful, so delete request */
		               orp = rp;
		               if (isp->requests == orp){
		                 /* first in list */
		                 isp->requests = orp->next_request;
		               } else {
		           for(rp = isp->requests; rp; rp = rp->next_request){
		             if (rp->next_request == orp){
		               /* link around it */
		               rp->next_request = orp->next_request;	
		               break;
		             }
		           }
		         }
		         snmp_free_pdu(orp->pdu);
		         free((char *)orp);
		         /* there shouldn't be another req with the same reqid */
		         break;  
	         }
	       }
	     }
	   } else if (pdu->command == SNMP_PDU_GET || 
		      pdu->command == SNMP_PDU_GETNEXT ||
		      pdu->command == TRP_REQ_MSG || 
		      pdu->command == SNMP_PDU_SET ||
		      pdu->command == SNMP_PDU_GETBULK ||
		      pdu->command == SNMP_PDU_INFORM ||
		      pdu->command == SNMP_PDU_V2TRAP) {
#ifdef LOG_DEBUG
        OutputDebugString( WIDE("You're asking for trouble, aren't you?! issuing commands to a manager shame shame") );
#endif
	     if (sp->callback)
	       sp->callback(RECEIVED_MESSAGE, sp, pdu->reqid, 
			    pdu, sp->callback_magic);
	   }
      // we have to keep this pdu for later disposal....
//	   snmp_free_pdu(pdu);
   }
    {
       PBYTE pBuffer;
       pBuffer = (PBYTE)GetNetworkLong( pc, 1 );
       if( !pBuffer )
       {
#ifdef LOG_DEBUG
          TCHAR msg[256];
#endif
          pBuffer = (PBYTE)malloc( 4096 );
#ifdef LOG_DEBUG
          wsprintf( msg, WIDE("Alloced NetBufer: %08x\n"), pBuffer );
          OutputDebugString( msg );
#endif
          SetNetworkLong( pc, 1, (DWORD)pBuffer );
       }
       doUDPRead( pc, pBuffer, 4096 );
    }
}

/*
 * Returns info about what snmp requires from a select statement.
 * numfds is the number of fds in the list that are significant.
 * All file descriptors opened for SNMP are OR'd into the fdset.
 * If activity occurs on any of these file descriptors, snmp_read
 * should be called with that file descriptor set
 *
 * The timeout is the latest time that SNMP can wait for a timeout.  The
 * select should be done with the minimum time between timeout and any other
 * timeouts necessary.  This should be checked upon each invocation of select.
 * If a timeout is received, snmp_timeout should be called to check if the
 * timeout was for SNMP.  (snmp_timeout is idempotent)
 *
 * Block is 1 if the select is requested to block indefinitely, rather than time out.
 * If block is input as 1, the timeout value will be treated as undefined, but it must
 * be available for setting in snmp_select_info.  On return, if block is true, the value
 * of timeout will be undefined.
 *
 * snmp_select_info returns the number of open sockets.  (i.e. The number of sessions open)
 */
int
snmp_select_info(int *numfds, fd_set *fdset, 
		 struct timeval *timeout, int *block)
{
    return snmp_sess_select_info((void *)0, numfds, fdset, timeout, block);
}

/* Same as snmp_select_info, but works just one session. */
int
snmp_sess_select_info(void *sessp, int *numfds, fd_set *fdset,
		      struct timeval *timeout, int *block)
{
    struct session_list *slptest = (struct session_list *)sessp;
    struct session_list *slp;
    struct snmp_internal_session *isp;
    struct request_list *rp;
    struct timeval now, earliest;
    int active = 0, requests = 0;

    timerclear(&earliest);
    /*
     * For each request outstanding, add it's socket to the fdset,
     * and if it is the earliest timeout to expire, mark it as lowest.
     * If a single session is specified, do just for that session.
     */
    if (slptest) slp = slptest; else slp = Sessions;
    for(; slp; slp = slp->next){

	active++;
	isp = slp->internal;
   /*
	if ((isp->sd + 1) > *numfds)
	    *numfds = (isp->sd + 1);
	FD_SET(isp->sd, fdset);
   */
#ifdef DEBUG_API
      printf("LIBSNMP:  select():  Adding port %d\n", isp->sd);
#endif
	if (isp->requests){
	    /* found another session with outstanding requests */
	    requests++;
	    for(rp = isp->requests; rp; rp = rp->next_request){
		if (!timerisset(&earliest) || 
		    timercmp(&rp->expire, &earliest, <))
		    earliest = rp->expire;
	    }
	}
	if (slp == slptest) break;
    }
#ifdef DEBUG_API
      printf("LIBSNMP:  Select Info:  %d active, %d requests pending.\n",
	     active, requests);
#endif

    if (requests == 0)	/* if none are active, skip arithmetic */
    {
	*block = 1; /* can block - timeout value is undefined if no requests*/
	return active;
    }

    /*
     * Now find out how much time until the earliest timeout.  This
     * transforms earliest from an absolute time into a delta time, the
     * time left until the select should timeout.
     */
    gettimeofday(&now, (struct timezone *)0);
#ifdef DEBUG_API
      printf("LIBSNMP:  Select Info: Earliest request expires in %d secs.\n",
	     earliest.tv_sec);
#endif
    earliest.tv_sec--;	/* adjust time to make arithmetic easier */
    earliest.tv_usec += 1000000L;
    earliest.tv_sec -= now.tv_sec;
    earliest.tv_usec -= now.tv_usec;
    while (earliest.tv_usec >= 1000000L){
	earliest.tv_usec -= 1000000L;
	earliest.tv_sec += 1;
    }
    if (earliest.tv_sec < 0){
	earliest.tv_sec = 0;
	earliest.tv_usec = 0;
    }

    /* if it was blocking before or our delta time is less, reset timeout */
    if (*block == 1 || timercmp(&earliest, timeout, <)){
	*timeout = earliest;
	*block = 0;
    }
    return active;
}

/*
 * snmp_timeout should be called whenever the timeout from snmp_select_info 
 * expires, but it is idempotent, so snmp_timeout can be polled (probably a 
 * cpu expensive proposition).  snmp_timeout checks to see if any of the 
 * sessions have an outstanding request that has timed out.  If it finds one 
 * (or more), and that pdu has more retries available, a new packet is formed
 * from the pdu and is resent.  If there are no more retries available, the 
 * callback for the session is used to alert the user of the timeout.
 */
void snmp_timeout(void)
{
    struct session_list *slp;

    for(slp = Sessions; slp; slp = slp->next){
	    snmp_sess_timeout((void *)slp);
    }
}

void
snmp_sess_timeout(void *sessp)
{
  struct session_list *slp = (struct session_list*)sessp;
  struct snmp_session *sp;
  struct snmp_internal_session *isp;
  struct request_list *rp, *orp = NULL, *freeme = NULL;
  struct timeval now;

  sp = slp->session; isp = slp->internal;

  gettimeofday(&now, (struct timezone *)0);

  /*
   * For each request outstanding, check to see if it has expired.
   */
#ifdef DEBUG_API
printf("LIBSNMP:  Checking session %s\n", 
       (sp->peername != NULL) ? sp->peername : "<NULL>");
#endif
    for(rp = isp->requests; rp; rp = rp->next_request) {
#ifdef DEBUG_API
printf("LIBSNMP:  Checking session request %d, expire at %u, Retry %d/%d\n", 
       rp->request_id, rp->expire.tv_sec, rp->retries, sp->retries);
#endif

      if (freeme != NULL) {
	/* frees rp's after the for loop goes on to the next_request */
	free((char *)freeme);
	freeme = NULL;
      }

      if (timercmp(&rp->expire, &now, <)) {

#ifdef DEBUG_API
printf("LIBSNMP:  Expired.\n");
#endif

	/* this timer has expired */
	if (rp->retries >= sp->retries) {
	  /* No more chances, delete this entry */
	  if (sp->callback)
	    sp->callback(TIMED_OUT, sp, rp->pdu->reqid, 
			 rp->pdu, sp->callback_magic);
	  if (orp == NULL) {
	    isp->requests = rp->next_request;
	  } else {
	    orp->next_request = rp->next_request;
	  }
	  snmp_free_pdu(rp->pdu);
	  freeme = rp;
	  continue;	/* don't update orp below */
	} else {
		    u_char  packet[PACKET_LENGTH];
		    int length = PACKET_LENGTH;
		    struct timeval tv;

		    /* retransmit this pdu */
		    rp->retries++;
#ifdef DEBUG_API
printf("LIBSNMP:  Retransmitting (%d, timeout %d).\n", 
       rp->retries, rp->timeout);
#endif

		    if (snmp_build(sp, rp->pdu, packet, &length) < 0){
		      /* Error for application to process */
		      break;
		    }

		    snmp_dump(packet, length, 
			      "sending", rp->pdu->address.sin_addr);

		    gettimeofday(&tv, (struct timezone *)0);
         if( !SendUDP( isp->sd, (char*)packet,length) )
//		    if (sendto(isp->sd, (char *)packet, length, 0, (struct sockaddr *)&rp->pdu->address, sizeof(rp->pdu->address)) < 0)
         {
			   snmp_errno = SNMPERR_GENERR;
			   sp->s_snmp_errno = SNMPERR_GENERR;
			   sp->s_errno = errno;
			   /* Error for application to process */
			   break;
		    }
		    /* XXXX tv sets now for better timercmp above ? */
		    rp->time = tv;
		    tv.tv_usec += rp->timeout;
		    tv.tv_sec += tv.tv_usec / 1000000L;
		    tv.tv_usec %= 1000000L;
		    rp->expire = tv;
		}
	    }
	    orp = rp;
	}
	if (freeme != NULL){
	    free((char *)freeme);
	    freeme = NULL;
	}
}


/* Print some API stats */
void snmp_api_stats(void *outP)
{
  struct session_list *slp;
  struct request_list *rp;
  struct snmp_internal_session *isp;
  FILE *out = (FILE *)outP;

  int active = 0;
  int requests = 0;
  int count = 0;
  int rcount = 0;

  fprintf(out, WIDE("LIBSNMP: Session List Dump\n"));
  fprintf(out, WIDE("LIBSNMP: ----------------------------------------\n"));
  for(slp = Sessions; slp; slp = slp->next){

    isp = slp->internal;
    active++;
    count++;
    fprintf(out, WIDE("LIBSNMP: %2d: Host %s\n"), count, 
	    (slp->session->peername == NULL) ? "NULL" : slp->session->peername);

    if (isp->requests) {
      /* found another session with outstanding requests */
      requests++;
      rcount=0;
      for (rp=isp->requests; rp; rp=rp->next_request) {
	rcount++;
	{
	  struct hostent *hp;
	  hp = gethostbyaddr((char *)&(rp->pdu->address), 
			     sizeof(u_int), AF_INET);
	  fprintf(out, WIDE("LIBSNMP: %2d: ReqId %d (%s) (%s)\n"), 
		  rcount, rp->request_id, snmp_pdu_type(rp->pdu),
		  (hp == NULL) ? "NULL" : hp->h_name);
	}
      }
    }
    fprintf(out, WIDE("LIBSNMP: ----------------------------------------\n"));
  }
  fprintf(out, WIDE("LIBSNMP: Session List: %d active, %d have requests pending.\n"),
	  active, requests);
}

/**********************************************************************/

/*
 * snmp_sess_error - same as snmp_error for single session API use.
 */
void snmp_sess_error(void *sessp, int *p_errno, 
		     int *p_snmp_errno, char **p_str)
{
  struct session_list *slp = (struct session_list*)sessp;
 
  if ((slp) && (slp->session))
    snmp_error(slp->session, p_errno, p_snmp_errno, p_str);
}


void snmp_sess_init(struct snmp_session * session)
{
  init_snmp();

  /* initialize session to default values */
 
  memset(session, 0, sizeof(struct snmp_session));
  session->remote_port = SNMP_DEFAULT_REMPORT;
  session->timeout = SNMP_DEFAULT_TIMEOUT;
  session->retries = SNMP_DEFAULT_RETRIES;
}

/*
 * returns NULL or internal pointer to session
 * use this pointer for the other snmp_sess* routines,
 * which guarantee action will occur ONLY for this given session.
 */
void *snmp_sess_pointer(struct snmp_session *session)
{
  struct session_list *slp;

  for (slp = Sessions; slp; slp = slp->next) {
    if (slp->session == session) {
      break;
    }
  }
  if (slp == NULL) {
    snmp_errno = SNMPERR_BAD_SESSION;
    return(NULL);
  }
  return((void *)slp);
}

/*
 * Input : an opaque pointer, returned by snmp_sess_open.
 * returns NULL or pointer to session.
 */
struct snmp_session *snmp_sess_session(void *sessp)
{
  struct session_list *slp = (struct session_list *)sessp;
  if (slp == NULL) return(NULL);
  return (slp->session);
}

SNMP_NAMESPACE_END
 
// $Log: $
