/* $Id: msg.h,v 1.2 2003/03/25 08:45:55 panther Exp $ */

/*	
 *   IPC package for CygWin
 *
 *   Copyright (C) 1997 Philippe CHAPUY
 *   Copyright (C) 1998 Ludovic LANGE
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 *   HISTORY:
 *   --------
 *
 *   13/05/1998 : Version 1.00 released
 *                First public release
 *                adress any comments to llange@capgemini.fr
 *
 */
 
 
/*
 * Philippe Chapuy, le 20/05/97
 */
#ifndef _CYGWIN_MSG_H
#define _CYGWIN_MSG_H


#ifdef MSG_SOURCE
#define MSG_PROC(type,name) __declspec(dllexport) type name
#else
#define MSG_PROC(type,name) __declspec(dllimport) type name
#endif


#if defined( __LCC__ ) || defined( _MSC_VER ) || defined( __WATCOMC__ )
#include <sys/timeb.h>
#include "sys/ipc.h"
#define	EIDRM 36	/* Identifier removed */
#define	ENOMSG 35	/* No message of desired type */
#else
#include "ipc.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif 

/* msgrcv options */
#define MSG_NOERROR     010000  /* no error if message is too big */
#define MSG_EXCEPT      020000  /* recv any msg except of specified type.*/

#define MSGMNI   25   /* <= 1K */     /* max # of msg queue identifiers */
#define MSGMAXMESS  150   /* <= 1K */     /* max # of msg queue identifiers */
#define MSGMAX  1024   /* <= 1024 */   /* max size of message (bytes) */
#define MSGMNB MSGMAX*MSGMAXMESS   /* ? */        /* default max size of a message queue */

/* one msg structure for each message */
struct msg {
    struct msg *msg_next;   /* next message on queue */
    long  msg_type;          
    char *msg_spot;         /* message text address, remplit a l'init PCPC */
    time_t msg_stime;       /* msgsnd time */
    short msg_ts;           /* message text size */
    short state;            /* 1 si occupe, 0 si libre */
    char  data[MSGMAX] ;
};
/* one msqid structure for each queue on the system */
struct msqid_ds {
    struct ipc_perm msg_perm;
    struct msg *msg_first;  /* first message on queue */
    struct msg *msg_last;   /* last message in queue */
    time_t msg_stime;       /* last msgsnd time */
    time_t msg_rtime;       /* last msgrcv time */
    time_t msg_ctime;       /* last change time */
    struct wait_queue *wwait;
    struct wait_queue *rwait;
    ushort msg_cbytes;      /* current number of bytes on queue */
    ushort msg_qnum;        /* number of messages in queue */
    int    msg_qbytes;      /* max number of bytes on queue */
    ushort msg_lspid;       /* pid of last msgsnd */
    ushort msg_lrpid;       /* last receive pid */
    struct msg msg[MSGMAXMESS] ;
/*    char   data[MSGMAXMESS][MSGMAX] ;*/
};

/* message buffer for msgsnd and msgrcv calls */
struct msgbuf {
    long mtype;         /* type of message */
    char mtext[1024];      /* message text */
};

/* buffer for msgctl calls IPC_INFO, MSG_INFO */
struct msginfo {
    int msgpool;
    int msgmap; 
    int msgmax; 
    int msgmnb; 
    int msgmni; 
    int msgssz; 
    int msgtql; 
    ushort  msgseg; 
};

/* unused */
#define MSGPOOL (MSGMNI*MSGMNB/1024)  /* size in kilobytes of message pool */
#define MSGTQL  MSGMNB            /* number of system message headers */
#define MSGMAP  MSGMNB            /* number of entries in message map */
#define MSGSSZ  16                /* message segment size */
#define __MSGSEG ((MSGPOOL*1024)/ MSGSSZ) /* max no. of segments */
#define MSGSEG (__MSGSEG <= 0xffff ? __MSGSEG : 0xffff)


/* ipcs ctl commands */
#define MSG_STAT 11
#define MSG_INFO 12

MSG_PROC(int, msgget )(key_t key, int msgflg);
MSG_PROC(int, msgsnd )(int msqid, struct msgbuf *msgp, size_t msgsz, int msgflg);
MSG_PROC(int, msgrcv )(int msqid, struct msgbuf *msgp, size_t msgsz, long msgtyp,
		       int msgflg);
MSG_PROC(int, msgctl )(int msqid, int cmd, struct msqid_ds *buf);

#ifdef __cplusplus
}
#endif 


#endif /* _CYGWIN_MSG_H */
// $Log: msg.h,v $
// Revision 1.2  2003/03/25 08:45:55  panther
// Added CVS logging tag
//
