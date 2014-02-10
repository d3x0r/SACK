// fb_sd_lmouse.c

//////////////////////////////////////////////////////////
//							//
//	All Rights Reserved.				//
//							//
//////////////////////////////////////////////////////////

// Handle ps2 mouse

#define LMOUSE_HAVE_IMPS2
//#define LMOUSE_LDEBUG

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

#include <stdhdrs.h>
#include <sack_types.h>
#include <deadstart.h>
#include <timers.h>

#include "ghdr.h"  // gets the current compilation flags...
//#include <win_type.h>
//#include <mat_type.h>
//#include <fbox/fbox_util.h>
//#include <fbox/fb_sd.h>
//#include <fbox/pmsp.h>
//#include <fbox/arch.h>

typedef struct tag_FBOX_UTIL_MOUSE
{
  _32			mbuttons;
  int			xpos;
  int			ypos;
} FBOX_UTIL_MOUSE;

typedef FBOX_UTIL_MOUSE *PFBOX_UTIL_MOUSE;

typedef struct tag_fbox_sd_lmouse
{
  int			mode;
  int			handle;
  struct timeval	old_tv;
  int			mousebuff_size;
  _8			mousebuff[4];
  FBOX_UTIL_MOUSE	mouse;
} fbox_sd_lmouse;

typedef fbox_sd_lmouse *pfbox_sd_lmouse;

static pfbox_sd_lmouse fbox_util_sd_lmouse_state=NULL;

static const _8 fbox_sd_lmouse_reset[1] = { 0xFF };
static const _8 fbox_sd_lmouse_ps2_reset[1] = { 0xF6 };
static const _8 fbox_sd_lmouse_ps2_init[6] =
	{ 0xE6, 0xF4, 0xF3, 100, 0xE8, 3 };

// Write to mouse
static LOGICAL fbox_sd_lmouse_send(_8 const * buffer,int size)
{
  if(fbox_util_sd_lmouse_state)
  {
    if(fbox_util_sd_lmouse_state->handle>=0)
    {
      if(write(fbox_util_sd_lmouse_state->handle,buffer,size)==size)
	return(TRUE);
    }
  }
  return(FALSE);
}


#ifdef LMOUSE_HAVE_IMPS2
static const _8 fbox_sd_lmouse_set_imps2[6] =
	{ 0xF3,200,0xF3,100,0xF3,80 };
static const _8 fbox_sd_lmouse_basic_init[3] = { 0xF4, 0xF3, 100 };
static const _8 fbox_sd_lmouse_query[1] = { 0xF2 };

// Reset and flush mouse
static void fbox_sd_lmouse_flush(void)
{
  fd_set set;
  struct timeval tv;
  _8 buffer[64];
  if(fbox_util_sd_lmouse_state)
  {
    if(fbox_util_sd_lmouse_state->handle>=0)
    {
      FD_ZERO(&set);
      FD_SET(fbox_util_sd_lmouse_state->handle,&set);
      tv.tv_sec = tv.tv_usec = 0;
      while(select(fbox_util_sd_lmouse_state->handle+1,&set,NULL,NULL,
	&tv)>0)
      {
	read(fbox_util_sd_lmouse_state->handle,buffer,sizeof(buffer));
      }
    }
  }
}
#endif

// Process a mouse byte
static void fbox_sd_lmouse_proc(_8 ch)
{
	struct timeval tv;
	if(!fbox_util_sd_lmouse_state)
		return;
	gettimeofday(&tv,NULL);
#ifdef LMOUSE_LDEBUG
	fbox_util_con_printf("Mode=%d Byte=%02X t=%d.%d\r\n",fbox_util_sd_lmouse_state->mode,
								ch,tv.tv_sec,tv.tv_usec);
#endif
	switch(fbox_util_sd_lmouse_state->mode)
	{
	default:
		fbox_util_sd_lmouse_state->mode = 0;
	case 0:			// ps2 get first
	mode_0:;
	if((ch&0xC0)!=0)
		break;
	fbox_util_sd_lmouse_state->old_tv = tv;
	fbox_util_sd_lmouse_state->mousebuff_size = 1;
	fbox_util_sd_lmouse_state->mousebuff[0] = ch;
	fbox_util_sd_lmouse_state->mode = 3;
	break;
#ifdef LMOUSE_HAVE_IMPS2
	case 1:			// check for imps2
		if((tv.tv_sec>fbox_util_sd_lmouse_state->old_tv.tv_sec+1)||
			((tv.tv_sec==fbox_util_sd_lmouse_state->old_tv.tv_sec+1)&&
			 (tv.tv_usec>=fbox_util_sd_lmouse_state->old_tv.tv_usec)))
		{
			fbox_util_sd_lmouse_state->mode = 0;
			goto mode_0;
		}
		if((ch==0xAA)||(ch==0xFA))
			break;
		fbox_sd_lmouse_send(fbox_sd_lmouse_ps2_init,
								  sizeof(fbox_sd_lmouse_ps2_init));
		if((ch==3)||(ch==4))
			fbox_util_sd_lmouse_state->mode = 2;
		else
		{
			fbox_util_sd_lmouse_state->mode = 0;
		}
		break;
	case 2:			// first imps2
	mode_2:;
	if((ch&0xC0)!=0)
		break;
	fbox_util_sd_lmouse_state->old_tv = tv;
	fbox_util_sd_lmouse_state->mousebuff_size = 1;
	fbox_util_sd_lmouse_state->mousebuff[0] = ch;
	fbox_util_sd_lmouse_state->mode = 4;
	break;
#endif
	case 3:			// continue ps2
		if((tv.tv_sec>fbox_util_sd_lmouse_state->old_tv.tv_sec+1)||
			((tv.tv_sec==fbox_util_sd_lmouse_state->old_tv.tv_sec+1)&&
			 (tv.tv_usec>=fbox_util_sd_lmouse_state->old_tv.tv_usec)))
		{
			fbox_util_sd_lmouse_state->mode = 0;
			goto mode_0;
      }
		fbox_util_sd_lmouse_state->mousebuff[
														 fbox_util_sd_lmouse_state->mousebuff_size++] = ch;
		if(fbox_util_sd_lmouse_state->mousebuff_size==3)
		{
			fbox_util_sd_lmouse_state->mouse.mbuttons =
				fbox_util_sd_lmouse_state->mousebuff[0]&7;
			fbox_util_sd_lmouse_state->mouse.xpos =
				fbox_util_sd_lmouse_state->mousebuff[1];
			fbox_util_sd_lmouse_state->mouse.ypos =
				fbox_util_sd_lmouse_state->mousebuff[2];
			if(fbox_util_sd_lmouse_state->mousebuff[0]&0x10)
				fbox_util_sd_lmouse_state->mouse.xpos -= 256;
			if(fbox_util_sd_lmouse_state->mousebuff[0]&0x20)
				fbox_util_sd_lmouse_state->mouse.ypos -= 256;
			fbox_util_sd_lmouse_state->mouse.ypos =
															  -fbox_util_sd_lmouse_state->mouse.ypos;
			fbox_util_sd_lmouse_state->mode = 0;
													  // dispatch the mouse?
			//printf( "Mouse1 at %d,%d %d\n"
			//		, fbox_util_sd_lmouse_state->mouse.xpos
			//		, fbox_util_sd_lmouse_state->mouse.ypos
			//		, fbox_util_sd_lmouse_state->mouse.mbuttons );
         GenerateMouseDeltaRaw( fbox_util_sd_lmouse_state->mouse.xpos
										, fbox_util_sd_lmouse_state->mouse.ypos
										, fbox_util_sd_lmouse_state->mouse.mbuttons
										);
			//fbox_util_disp_do_message(FBOX_UTIL_DISPT_LOW_MOUSE,0,
			//								  sizeof(fbox_util_sd_lmouse_state->mouse),
			//								  (LP_8)&fbox_util_sd_lmouse_state->mouse);
		}
		break;
#ifdef LMOUSE_HAVE_IMPS2
	case 4:			// continue imps2
		if((tv.tv_sec>fbox_util_sd_lmouse_state->old_tv.tv_sec+1)||
			((tv.tv_sec==fbox_util_sd_lmouse_state->old_tv.tv_sec+1)&&
			 (tv.tv_usec>=fbox_util_sd_lmouse_state->old_tv.tv_usec)))
		{
			fbox_util_sd_lmouse_state->mode = 2;
			goto mode_2;
		}
		fbox_util_sd_lmouse_state->mousebuff[
														 fbox_util_sd_lmouse_state->mousebuff_size++] = ch;
		if(fbox_util_sd_lmouse_state->mousebuff_size==4)
		{
			fbox_util_sd_lmouse_state->mouse.mbuttons =
				fbox_util_sd_lmouse_state->mousebuff[0]&7;
			fbox_util_sd_lmouse_state->mouse.xpos =
				fbox_util_sd_lmouse_state->mousebuff[1];
			fbox_util_sd_lmouse_state->mouse.ypos =
				fbox_util_sd_lmouse_state->mousebuff[2];
			if(fbox_util_sd_lmouse_state->mousebuff[0]&0x10)
				fbox_util_sd_lmouse_state->mouse.xpos -= 256;
			if(fbox_util_sd_lmouse_state->mousebuff[0]&0x20)
				fbox_util_sd_lmouse_state->mouse.ypos -= 256;
			fbox_util_sd_lmouse_state->mouse.ypos =
															  -fbox_util_sd_lmouse_state->mouse.ypos;
			switch(fbox_util_sd_lmouse_state->mousebuff[3]&0xF)
			{
			case 0x01:	// DY--;
				fbox_util_sd_lmouse_state->mouse.mbuttons |= 0x100;
				break;
			case 0x02:	// DX--;
				fbox_util_sd_lmouse_state->mouse.mbuttons |= 0x400;
				break;
			case 0x0E:	// DX++;
				fbox_util_sd_lmouse_state->mouse.mbuttons |= 0x800;
				break;
			case 0x0F:	// DY++;
				fbox_util_sd_lmouse_state->mouse.mbuttons |= 0x200;
				break;
			}
			fbox_util_sd_lmouse_state->mode = 2;
			//printf( "Mouse2 at %d,%d %d\n"
			//		, fbox_util_sd_lmouse_state->mouse.xpos
			//		, fbox_util_sd_lmouse_state->mouse.ypos
			//		, fbox_util_sd_lmouse_state->mouse.mbuttons );
         GenerateMouseDeltaRaw( fbox_util_sd_lmouse_state->mouse.xpos
										, fbox_util_sd_lmouse_state->mouse.ypos
										, fbox_util_sd_lmouse_state->mouse.mbuttons
										);
			//fbox_util_disp_do_message(FBOX_UTIL_DISPT_LOW_MOUSE,0,
			//								  sizeof(fbox_util_sd_lmouse_state->mouse),
			//								  (LP_8)&fbox_util_sd_lmouse_state->mouse);
		}
		break;
#endif
	}
}

													  // Select read call
static int fbox_sd_lmouse_read(void *cargo,int handle)
{
	_8 buffer[256];
	int ix,iy;
	ix = read(handle,buffer,sizeof(buffer));
	for(iy=0;iy<ix;iy++)
		fbox_sd_lmouse_proc(buffer[iy]);
	return(0);
}

			// Handle close of mouse handle
static int fbox_sd_lmouse_close(void *cargo,int handle)
{
	if(fbox_util_sd_lmouse_state)
	{
		free(fbox_util_sd_lmouse_state);
		fbox_util_sd_lmouse_state = NULL;
	}
	return(0);
}

PTRSZVAL CPROC Dofbox_sd_lmouse_read( PTHREAD thread )
{
	PTRSZVAL psv = GetThreadParam( thread );
	fd_set fdRead;
	FD_ZERO( &fdRead );
   FD_SET( psv, &fdRead );
	while( select( psv+1, &fdRead, NULL, NULL, NULL ) )
	{
      fbox_sd_lmouse_read( NULL, psv );
	}
   return 0;
}

static CTEXTSTR fbox_sd_lmouse_devs[] =
{
	"/dev/psaux",
	"/dev/usbmouse",
	"/dev/input/mice",
	NULL,
};

		 // Setup mouse
LOGICAL fbox_sd_lmouse_setup(CTEXTSTR dev)
{
	int handle,ix;
	CTEXTSTR name;
	if(!fbox_util_sd_lmouse_state)
	{
		if(dev)
		{
			name = dev;
			handle = open(dev,O_RDWR /* |O_NONBLOCK */ );
		}
		else
			handle = -1;
		for(ix=0;(handle<0)&&(fbox_sd_lmouse_devs[ix]);ix++)
		{
			name = fbox_sd_lmouse_devs[ix];
			handle = open(fbox_sd_lmouse_devs[ix],O_RDWR /* |O_NONBLOCK */ );
		}
		if(handle<0)
			return(FALSE);
		fbox_util_sd_lmouse_state = New(fbox_sd_lmouse);
		if(!fbox_util_sd_lmouse_state)
		{
			close(handle);
			return(FALSE);
		}
		memset(fbox_util_sd_lmouse_state,0,sizeof(fbox_sd_lmouse));
		fbox_util_sd_lmouse_state->handle = handle;
		if(getenv("DO_MOUSE_RESET"))
		{
			fbox_sd_lmouse_send(fbox_sd_lmouse_reset,
									  sizeof(fbox_sd_lmouse_reset));
			usleep(500000);
		}
#ifdef LMOUSE_HAVE_IMPS2
		if(!getenv("NO_IMPS2"))
		{
			fbox_sd_lmouse_flush();
			fbox_sd_lmouse_send(fbox_sd_lmouse_basic_init,
									  sizeof(fbox_sd_lmouse_basic_init));
			if(fbox_sd_lmouse_send(fbox_sd_lmouse_basic_init,
										  sizeof(fbox_sd_lmouse_basic_init)))
      {
			if(fbox_sd_lmouse_send(fbox_sd_lmouse_set_imps2,
										  sizeof(fbox_sd_lmouse_set_imps2)))
			{
				fbox_sd_lmouse_flush();
				if(fbox_sd_lmouse_send(fbox_sd_lmouse_query,
											  sizeof(fbox_sd_lmouse_query)))
				{
					fbox_util_sd_lmouse_state->mode = 1;
					gettimeofday(&fbox_util_sd_lmouse_state->old_tv,NULL);
				}
			}
      }
		}
		else
#endif
		{
			fbox_sd_lmouse_send(fbox_sd_lmouse_ps2_reset,
									  sizeof(fbox_sd_lmouse_ps2_reset));
			fbox_sd_lmouse_send(fbox_sd_lmouse_ps2_init,
									  sizeof(fbox_sd_lmouse_ps2_init));
		}
      ThreadTo( Dofbox_sd_lmouse_read, handle );
      /*
		if(!fbox_util_select_add(NULL,handle,
										 FBOX_UTIL_ID('l','m','s','e'),fbox_sd_lmouse_read,NULL,
										 NULL,fbox_sd_lmouse_close))
		{
			close(handle);
			free(fbox_util_sd_lmouse_state);
			fbox_util_sd_lmouse_state = NULL;
			return(FALSE);
		}
      */
	}
  return(TRUE);
}

#ifdef __RAW_FRAMEBUFFER__
PRELOAD( Dofbox_sd_lmouse_setup )
{
   fbox_sd_lmouse_setup( NULL );
}
#endif

