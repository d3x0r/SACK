/*
 * Copyright 1992 by Jutta Degener and Carsten Bormann, Technische
 * Universitaet Berlin.  See the accompanying file "COPYRIGHT" for
 * details.  THERE IS ABSOLUTELY NO WARRANTY FOR THIS SOFTWARE.
 */

/*$Header: /home/kbs/jutta/src/gsm/gsm-1.0/inc/RCS/gsm.h,v 1.11 1996/07/05 18:02:56 jutta Exp $*/

#ifndef	GSM_H
#define	GSM_H

#define	NeedFunctionPrototypes	1

#ifdef _NO_PROTO
#	undef	NeedFunctionPrototypes
#endif

#ifdef NeedFunctionPrototypes
#   include	<stdio.h>		/* for FILE * 	*/
#endif

#undef GSM_P
#if NeedFunctionPrototypes
#	define	GSM_P( protos )	protos
#else
#	define  GSM_P( protos )	   protos
#endif

/*
 *	Interface
 */

typedef struct gsm_state * 	gsm;
typedef short		   	gsm_signal;		/* signed 16 bit */
typedef unsigned char		gsm_byte;
typedef gsm_byte 		gsm_frame[33];		/* 33 * 8 bits	 */

#define	GSM_MAGIC		0xD		  	/* 13 kbit/s RPE-LTP */

#define	GSM_PATCHLEVEL		10
#define	GSM_MINOR		0
#define	GSM_MAJOR		1

#define	GSM_OPT_VERBOSE		1
#define	GSM_OPT_FAST		2
#define	GSM_OPT_LTP_CUT		3
#define	GSM_OPT_WAV49		4
#define	GSM_OPT_FRAME_INDEX	5
#define	GSM_OPT_FRAME_CHAIN	6

#ifdef __cplusplus
#define GSM_EXTERN extern "C"
#else
#define GSM_EXTERN extern 
#endif

GSM_EXTERN gsm  gsm_create 	GSM_P((void));
GSM_EXTERN void gsm_destroy GSM_P((gsm));	

GSM_EXTERN int  gsm_print   GSM_P((FILE *, gsm, gsm_byte  *));
GSM_EXTERN int  gsm_option  GSM_P((gsm, int, int *));

GSM_EXTERN void gsm_encode  GSM_P((gsm, gsm_signal *, gsm_byte  *));
GSM_EXTERN int  gsm_decode  GSM_P((gsm, gsm_byte   *, gsm_signal *));

GSM_EXTERN int  gsm_explode GSM_P((gsm, gsm_byte   *, gsm_signal *));
GSM_EXTERN void gsm_implode GSM_P((gsm, gsm_signal *, gsm_byte   *));

#undef	GSM_P

#endif	/* GSM_H */
