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

#ifdef WIN32
#include <memory.h>
#include <winsock2.h>
#else /* WIN32 */
#include <sys/param.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif /* WIN32 */

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

#include <snmp/libsnmp.h>
#if 0
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

SNMP_NAMESPACE

extern struct session_list *Sessions;

extern int snmp_errno;

/*#define DEBUG_API 1*/

/*=============================================================================
 *
 *	Name			: snmp_get_internal_session
 *
 *	Description	: Get the internal_session of the session
 *
 *	Parameters	: 
 *		- struct snmp_session *session           => session
 *		+ struct snmp_internal_session *internal => internal session
 *
 *	Return value: 
 *		0	=> I don't find it
 *		1	=> I find it
 *
 *=============================================================================
 */
int snmp_get_internal_session(struct snmp_session *session, struct snmp_internal_session *internal)
{
	struct session_list *slp;
	struct snmp_internal_session *isp;

	isp = NULL;

	for(slp = Sessions; slp; slp = slp->next)
	{
		if (slp->session == session)
		{
			isp = slp->internal;
			break;
		}
	}
	internal = isp;

	if (isp == NULL)
	{
		snmp_errno = SNMPERR_BAD_SESSION;
		return 0;
	}
	else
		return 1;
}


/*=============================================================================
 *
 *	Name			: snmp_get_request_list
 *
 *	Description	: Get the Request List of the session
 *
 *	Parameters	: 
 *		- struct snmp_session *session	=> session
 *		+ struct request_list *list		=> request list
 *
 *	Return value: 
 *		0	=> I don't find it
 *		1	=> I find it
 *
 *=============================================================================
 */
int snmp_get_request_list(struct snmp_session *session, struct request_list *list)
{
	struct session_list *slp;
	struct request_list *rqst;

	rqst = NULL;

	for(slp = Sessions; slp; slp = slp->next)
	{
		if (slp->session == session)
		{
			rqst = slp->internal->requests;
			break;
		}
	}
	list = rqst;

	if (rqst == NULL)
	{
		snmp_errno = SNMPERR_BAD_SESSION;
		return 0;
	}
	else
		return 1;
}


/*=============================================================================
 *
 *	Name			: snmp_modif_param_session
 *
 *	Description	: Modification of the parameters of the session execpt
 *					peername
 *					remote_port
 *					local_port
 *
 *	Parameters	: 
 *		- struct snmp_session *session	=> session
 *
 *	Return value: 
 *		0	=> I don't find it
 *		1	=> I find it
 *
 *=============================================================================
 */
int snmp_modif_param_session(struct snmp_session *session )
{
	struct session_list	*slp;
	char						*cp;
	
	for(slp = Sessions; slp; slp = slp->next)
	{
		if (slp->session == session)
			break;
   } 

	if (slp == NULL)
	{
		snmp_errno = SNMPERR_BAD_SESSION;
		return 0;
	}
	else
	{
		if ( strcmp((char *)slp->session->community , (char *)session->community) )
		{ 
			cp = (char *)malloc( (unsigned)session->community_len + 1 );
			if (cp!=NULL)
			{
				strcpy(cp , (char *)session->community);
				free((u_char *)slp->session->community);
				slp->session->community_len = session->community_len;
				slp->session->community = (u_char *)cp;
			}
			else 
			{
				snmp_errno = SNMPERR_OS_ERR;
				return 0;
			}
		}
		if (slp->session->Version != session->Version)
			slp->session->Version = session->Version;

		if (slp->session->retries != session->retries)
			slp->session->retries = session->retries;
		
		if (slp->session->timeout != session->timeout)
			slp->session->timeout = session->timeout;

		return 1;
	}
}


/*=============================================================================
 *
 *	Name			: snmp_get_socket_session
 *
 *	Description	: retrune the socket number of the session_
 *
 *	Parameters	: 
 *		- struct snmp_session *session	=> session_
 *
 *	Return value: 
 *		>0	=> Socket number
 *		-1	=> I find it
 *
 *=============================================================================
 */
PCLIENT snmp_get_socket_session(struct snmp_session *session_ )
{
	struct session_list *slp;
	
	for(slp = Sessions; slp; slp = slp->next)
	{
		if (slp->session == session_)
			break;
	}

	if (slp == NULL)
	{
		snmp_errno = SNMPERR_BAD_SESSION;
		return NULL;
	}
	else
	{
		return slp->internal->sd;
	}
}



/*=============================================================================
 *
 * Name        : snmp_select_info_session
 *
 * Description : return the next time of the closest request of the session
 *               if it exists...
 *
 *	Parameters  : 
 *    - struct snmp_session* session   => session
 *    - struct timeval*      timeout   => time of the next TimeOut Request
 *
 * Return value: 
 *    -1 => We don't find the session.
 *     0 => No more request for this sesion.
 *    >0 => Number of request for this session.
 *
 *=============================================================================
 */
int snmp_select_info_session(struct snmp_session *session_, struct timeval* timeout)
{
	struct session_list*          slp;
   struct snmp_internal_session* isp;
   struct timeval                now,earliest;
   struct request_list*          rp;
   int                           requests = 0;
	
	for(slp = Sessions; slp; slp = slp->next)
	{
		if (slp->session == session_)
			break;
	}
	if (slp == NULL)
	{
		snmp_errno = SNMPERR_BAD_SESSION;
		return -1;
	}
	else
	{

      timerclear(&earliest);

      isp = slp->internal;

      if (isp->requests)
      {
	      requests++;
	      /* found another session with outstanding requests */
	      for(rp = isp->requests; rp; rp = rp->next_request)
         {
		      if( !timerisset(&earliest) || timercmp(&rp->expire, &earliest, <) )
		         earliest = rp->expire;
	      }
	   }

#ifdef DEBUG_API
printf("LIBSNMP:  Select Info:  %d requests pending.\n", requests);
#endif

      if (requests == 0)	/* if none are active, skip arithmetic */
	      return requests;

      gettimeofday(&now, (struct timezone *)0);

      earliest.tv_sec--;	/* adjust time to make arithmetic easier */
      earliest.tv_usec += 1000000L;
      earliest.tv_sec -= now.tv_sec;
      earliest.tv_usec -= now.tv_usec;
      while (earliest.tv_usec >= 1000000L)
      {
         earliest.tv_usec -= 1000000L;
	      earliest.tv_sec += 1;
      }
      if (earliest.tv_sec < 0)
      {
	      earliest.tv_sec = 0;
	      earliest.tv_usec = 0;
      }

	   *timeout = earliest;
   }

#ifdef DEBUG_API
printf("LIBSNMP:  Select Info: Earliest request expires in %d secs.\n", earliest.tv_sec);
#endif

   return requests;
}




/*=============================================================================
 *
 * Name        : snmp_timeout_session
 *
 * Description : retranmit the TimeOut request of the session_ if it is possible,
 *               unless call the callBack and delete it from the API.
 *
 *	Parameters  : 
 *    - struct snmp_session *session	=> session
 *
 * Return value: 
 *    0 => We don't find the session.
 *    1 => We find the session.
 *
 *=============================================================================
 */
int snmp_timeout_session(struct snmp_session *session_ )
{
	struct session_list*          slp;
   struct snmp_session*          sp;
   struct snmp_internal_session* isp;
   struct timeval                now;
   struct request_list *rp, *orp, *freeme = NULL;
	
	for(slp = Sessions; slp; slp = slp->next)
	{
		if (slp->session == session_)
			break;
	}
	if (slp == NULL)
	{
		snmp_errno = SNMPERR_BAD_SESSION;
		return 0;
	}
	else
	{
      gettimeofday(&now, (struct timezone *)0);
      sp  = slp->session;
      isp = slp->internal;
      orp = NULL;

#ifdef DEBUG_API
printf("LIBSNMP:  Checking session %s\n", (sp->peername != NULL) ? sp->peername : "<NULL>");
#endif
      for(rp = isp->requests; rp; rp = rp->next_request) 
      {
#ifdef DEBUG_API
printf("LIBSNMP:  Checking session request %d, expire at %u, Retry %d/%d\n", rp->request_id, rp->expire.tv_sec, rp->retries, sp->retries);
#endif
         if (freeme != NULL) 
         {
	         /* frees rp's after the for loop goes on to the next_request */
	         free((char *)freeme);
	         freeme = NULL;
         }

         if (timercmp(&rp->expire, &now, <)) 
         {
#ifdef DEBUG_API
printf("LIBSNMP:  API found an expired request.\n");
#endif
	         /* this timer has expired */
	         if (rp->retries >= sp->retries) 
            {
#ifdef DEBUG_API
printf("LIBSNMP:  No more chances, call the CallBack and delete this entry.\n");
#endif
   	         /* No more chances, delete this entry */
               sp->callback(TIMED_OUT, sp, rp->pdu->reqid, rp->pdu, sp->callback_magic);

               if (orp == NULL) 
               {
	               isp->requests = rp->next_request;
	            } 
               else 
               {
	               orp->next_request = rp->next_request;
	            }
	            snmp_free_pdu(rp->pdu);
	            freeme = rp;
	            continue;	/* don't update orp below */
	         } 
            else 
            {
		         u_char         packet[PACKET_LENGTH];
		         int            length = PACKET_LENGTH;
		         struct timeval tv;

               /* retransmit this pdu */
   		      rp->retries++;

#ifdef DEBUG_API
printf("LIBSNMP:  Retransmitting, %d try.\n",rp->retries);
#endif

		         rp->timeout <<= 1;
		         if (snmp_build(sp , rp->pdu, packet, &length) < 0)
               {
#ifdef STDERR_OUTPUT
                  fprintf(stderr, WIDE("Error building packet\n"));
#endif
               }
		         snmp_dump(packet, length, WIDE("sending"), rp->pdu->address.sin_addr);

		         gettimeofday(&tv, (struct timezone *)0);
//		         if (sendto(isp->sd, (char *)packet, length, 0, (struct sockaddr *)&rp->pdu->address, sizeof(rp->pdu->address)) < 0)
               if( !SendUDP( isp->sd, (char *)packet, length ) )
               {
#ifdef STDERR_OUTPUT
			         perror("sendto");
#endif
		         }
		         rp->time = tv;
		         tv.tv_usec += rp->timeout;
		         tv.tv_sec += tv.tv_usec / 1000000L;
		         tv.tv_usec %= 1000000L;
		         rp->expire = tv;
		      }
	      }
	      orp = rp;
	   }
	   if (freeme != NULL)
      {
	      free((char *)freeme);
	      freeme = NULL;
	   }
   }
   return 1;
}
SNMP_NAMESPACE_END
// $Log: $
