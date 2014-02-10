
//#define DEFINE_DEFAULT_RENDER_INTERFACE
//#define USE_IMAGE_INTERFACE g.pii
//GetImageInterface()
#include <stdhdrs.h>
#include <image.h>
#include <sharemem.h>
#include <network.h>
#include <controls.h>
#include <timers.h>
#include <configscript.h>
#include <psi.h>
#ifdef __LINUX__
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/videodev.h>
#include <xvid.h>
#endif

#ifdef WIN32
#include "v4w.h"
#endif

#include "channels.h"
#include "streamstruct.h"
#include "compress.h"
#include "decompress.h"
#include "filters.h"

#ifdef __LINUX__
typedef int HANDLE;
#endif
#include "global.h"

GLOBAL g;

#ifdef __LINUX__
#define INVALID_HANDLE_VALUE -1
#endif

int EnqueFrameEx( SIMPLE_QUEUE *que, PTRSZVAL frame, char *qname, int line )
#define EnqueFrame(q,f) EnqueFrameEx(q,f,#q, __LINE__ )
{
	INDEX next_head = (que->head + 1)%que->size;
   //lprintf( WIDE("Enque %d into %s (%d)"), frame, qname, line );
	if( next_head != que->tail )
	{
      que->frames[que->head] = frame;
		que->head = next_head;
      return 1;
	}
   return 0;
}

PTRSZVAL DequeFrameEx( SIMPLE_QUEUE *que, char *quename, int line )
#define DequeFrame(q) DequeFrameEx(q,#q, __LINE__ )
{
	if( que->head != que->tail )
	{
		INDEX r = que->frames[que->tail];
      //lprintf( WIDE("%s(%d) results as %d"), quename, line, r );
		que->tail++;
		que->tail %= que->size;
      return r;
	}
   return INVALID_INDEX;
}

INDEX FramesQueued( SIMPLE_QUEUE *que )
{
	int len = que->head - que->tail;
	if( len < 0 )
      len += que->size;
   return len;
}

#ifdef __LINUX__
typedef struct v4l_data_tag
{
	HANDLE handle;
	struct {
		BIT_FIELD bFirstRead : 1;
		BIT_FIELD bNotMemMapped : 1;
	} flags;
	struct video_capability caps;
	struct video_mbuf mbuf;
	struct video_mmap *vmap;
	struct video_tuner tuner;
	struct video_audio audio;
	struct video_channel chan;
	struct video_picture picture;
	struct video_unit    unit;
   // for caputre directly to the framebuffer
	struct video_buffer  vbuf;
   // capture size parameters
   struct video_window vwin;
	INDEX curr_channel;
	_32 channels[256]; // array of valid channels?
	_32 freq;
	P_8 map; // pointer to mapped memory..
	Image *pFrames;
   SIMPLE_QUEUE done;
   SIMPLE_QUEUE ready;
	SIMPLE_QUEUE capture;
	PTHREAD pThread;
// nLastCycledFrame...
   // held until next copy..
   INDEX nFrame;
// an array of frames for count of frames...

} *PV4L_DATA, *PDEVICE_DATA;

//--------------------------------------------------------------------

int myioctlEx( HANDLE h, int op, char *pOp, PTRSZVAL data DBG_PASS )
{
	int ret = ioctl(h,op,(POINTER)data);
	int err;
	lprintf( "args : h:%d op:%s(%d), %" PTRSZVALf
           , h, pOp, op, data );
	if( ret == -1 )
	{
      err = errno;
		lprintf( DBG_FILELINEFMT "Failed to ioctl(%s): %s" DBG_RELAY
				 , pOp
				 , strerror(err)
				 );
		errno = err;
	}
	return ret;
}

#define ioctl(h,op,data) myioctlEx((h),(op),#op,(PTRSZVAL)(data) DBG_SRC)

// return last captured frame
void CycleReadV4L( PDEVICE_DATA pDevice )
{
	if( pDevice->flags.bNotMemMapped )
	{
		int n;
		if( ( n = DequeFrame( &pDevice->done ) ) != INVALID_INDEX )
		{
         int result;
			result = read( pDevice->handle
							 , GetImageSurface( pDevice->pFrames[n] )
							 , pDevice->vwin.width*pDevice->vwin.height*(pDevice->picture.depth/8) );
			lprintf( "read resulted: %d", result );

			EnqueFrame( &pDevice->ready, n );
		}
	}
	else
	{
		if( pDevice->flags.bFirstRead )
		{
			int n;
			lprintf( "..." );
			while( ( n = DequeFrame( &pDevice->done ) ) != INVALID_INDEX )
			{
			// preserve the order of frames queued for capture...
				lprintf( WIDE("initial start queuing %d which is %d,%d")
						 , n
						 , pDevice->caps.maxwidth
						 , pDevice->caps.maxheight
						 );
				pDevice->vmap[n].format = VIDEO_PALETTE_RGB24;
				pDevice->vmap[n].frame  = n;
				pDevice->vmap[n].width  = pDevice->vwin.width;
				pDevice->vmap[n].height = pDevice->vwin.height;
											// mark all frames pending...
												// therefore none are ready.
				if( ioctl( pDevice->handle, VIDIOCMCAPTURE, pDevice->vmap + n) == -1 )
				{
					lprintf( WIDE("Failed to start capture first read.") );
				}
				else
				{
					lprintf( WIDE("Mark %d as to capture..."), n );
					EnqueFrame( &pDevice->capture, n );
				}
				pDevice->flags.bFirstRead = 0;
			}
		}
		else
		{
			INDEX nFrame;
			do
			{
				nFrame = DequeFrame( &pDevice->done );
						 // there's only one frame, have to wait for it to be done...
				if( nFrame == INVALID_INDEX && pDevice->mbuf.frames == 1 )
					goto check_capture_done;

				if( nFrame == INVALID_INDEX && ( FramesQueued( &pDevice->capture ) < 3 ) )
				{
					lprintf( WIDE("Dropping a frame...") );
					lprintf( WIDE("done %d %d"), pDevice->done.head, pDevice->done.tail );
					lprintf( WIDE("capture %d %d"), pDevice->capture.head, pDevice->capture.tail );
					lprintf( WIDE("ready %d %d"), pDevice->ready.head, pDevice->ready.tail );
				// dropping a frame...
				// no done frames, snag the next ready and grab it.
					nFrame = DequeFrame( &pDevice->ready );
					if( nFrame == INVALID_INDEX )
					{
						lprintf( WIDE("Fatal error - all frames are outstanding in process...") );
						lprintf( WIDE("done %d %d"), pDevice->done.head, pDevice->done.tail );
						lprintf( WIDE("capture %d %d"), pDevice->capture.head, pDevice->capture.tail );
						lprintf( WIDE("ready %d %d"), pDevice->ready.head, pDevice->ready.tail );
					}
				}
				if( nFrame != INVALID_INDEX )
				{
				// locked should only ever be one frame...
				// and that one we can skip - and claim a dropped frame.

				// begin queuing another frame to be filled...
				// the next frame will ahve already been queued...
				// and we will fall out and wait for ti - because
				// the very next one is the oldest one queued to wait.
				//lprintf( WIDE("queue wait on frame %d"), nFrame );
					if( ioctl(pDevice->handle
								, VIDIOCMCAPTURE
								, &pDevice->vmap[nFrame]) == -1 )
					{
					// already pending...
						lprintf( WIDE("Failed to capture next frame pending...") );
					}
					EnqueFrame( &pDevice->capture, nFrame );
				}
			} while( nFrame != INVALID_INDEX );
		}
	check_capture_done:
		{
			int retry = 1;
			int nFrame = DequeFrame( &pDevice->capture );
			while (retry ){
							  //lprintf( WIDE("Monitor frame %d wait."), nFrame );
				if( ioctl(pDevice->handle, VIDIOCSYNC, &nFrame ) == -1 )
				{
					switch( errno )
					{
					// this is an okay thing to have happen?
					// the read is lost? or what?

					case EINTR:
						lprintf( WIDE("interrupt woke ioctl...") );
						retry = 1;
						break;
					default:
						lprintf( WIDE("Fatal error... didn't wait for capture.") );
						retry = 0;
						return;
								//return NULL;
					}
				}
				else
					retry = 0;
			}
			EnqueFrame( &pDevice->ready, nFrame );
							//lprintf( WIDE("issuing wake to thread...") );
							//WakeThread( pDevice->pThread );
							//lprintf( WIDE("Monitor frame %d ready."), nFrame );
		}
	}
}

int CPROC GetCapturedFrame( PTRSZVAL psv, PCAPTURE_DEVICE pDevice )
{
	PDEVICE_DATA pDevData = (PDEVICE_DATA)psv;
	if( pDevData->nFrame != INVALID_INDEX )
		EnqueFrame( &pDevData->done, pDevData->nFrame );
   pDevData->nFrame = DequeFrame( &pDevData->ready );
	if( pDevData->nFrame != INVALID_INDEX )
	{
	//lprintf( WIDE("Process frame %d to compress..."), nFrame );
	//lprintf( WIDE("done %d %d"), pDeviceData->done.head, pDeviceData->done.tail );
	//lprintf( WIDE("capture %d %d"), pDeviceData->capture.head, pDeviceData->capture.tail );
	//lprintf( WIDE("ready %d %d"), pDeviceData->ready.head, pDeviceData->ready.tail );
		SetDeviceData( pDevice
						 , pDevData->pFrames[pDevData->nFrame]
						 , 768*480 );
      return 1;
	}
   return 0;
}
#endif

PTRSZVAL CPROC CycleCallbacks( PTHREAD pThread )
{
	PTRSZVAL psvDev = GetThreadParam(pThread);
	PCAPTURE_DEVICE pDevice = (PCAPTURE_DEVICE)psvDev;
   pDevice->pCallbackThread = pThread;
   while( 1 )
	{
		PCAPTURE_CALLBACK callback;
		for( callback = pDevice->callbacks; callback; callback = NextLink( callback ) )
		{
			//lprintf( WIDE("Process callback: %p"), callback );
			//lprintf( WIDE("Process callback: %p (%p)"), callback, callback->callback );
			if( !callback->callback( callback->psv, pDevice ) )
            break;
			//lprintf( WIDE("Processed callback: %p (%p)"), callback, callback->callback );
		}
		Relinquish();
	}
   return 0;
}

PCAPTURE_DEVICE CreateCaptureStream( void )
{
	PCAPTURE_DEVICE pcd = New( CAPTURE_DEVICE);
	MemSet( pcd, 0, sizeof( CAPTURE_DEVICE ) );
#ifndef WIN32
   /* this is driven by the camera on capture event... */
	ThreadTo( CycleCallbacks, (PTRSZVAL)pcd );
#endif
   return pcd;
}

#ifdef __LINUX__
PTRSZVAL CPROC CycleReadThread( PTHREAD pThread )
{
	PTRSZVAL psvDev = GetThreadParam(pThread);
   PDEVICE_DATA pDevice = (PDEVICE_DATA)psvDev;
	while(1)
	{
		CycleReadV4L( pDevice );
	}
   return 0;
}
#endif

//--------------------------------------------------------------------

#ifdef __LINUX__
void CloseV4L( PDEVICE_DATA *ppDevice )
{
	if( ppDevice )
	{
		PDEVICE_DATA pDevice = *ppDevice;
		if( pDevice->pFrames )
		{
			int n;
			for( n = 0; n < pDevice->mbuf.frames; n++ )
				UnmakeImageFile( pDevice->pFrames[n] );
			Release( pDevice->pFrames );
		}
		if( pDevice->vmap )
			Release( pDevice->vmap );
		if( pDevice->map != MAP_FAILED && pDevice->map )
			munmap( pDevice->map, pDevice->mbuf.size );
		close( pDevice->handle );
		(*ppDevice) = NULL;
	}
}
#endif

//--------------------------------------------------------------------

#ifdef __LINUX__
void SetChannelV4L( PDEVICE_DATA pDevice, int chan )
{
	if(chan == -1)
		pDevice->chan.channel = 1;  //set to 1 for video line input
   else
		pDevice->chan.channel = 0;
	ioctl(pDevice->handle, VIDIOCSCHAN, &pDevice->chan);

	if( chan >= 0 )
	{
		int freq = freqs[chan];
		pDevice->tuner.mode = VIDEO_MODE_NTSC;
      //pDevice->tuner.flags=VIDEO_TUNER_NORM;
		ioctl(pDevice->handle, VIDIOCSTUNER, &pDevice->tuner);
		ioctl(pDevice->handle, VIDIOCSFREQ, &freq);
	}
}
#endif
//--------------------------------------------------------------------

//--------------------------------------------------------------------

#ifdef __LINUX__
int GetCaptureSizeV4L( PDEVICE_DATA pDevice, P_32 width, P_32 height )
{
	if( !pDevice )
		return FALSE;
	if( width )
		(*width) = pDevice->vwin.width;
	if( height )
		(*height) = pDevice->vwin.height;
	return TRUE;
}
#endif
//--------------------------------------------------------------------

#ifdef __LINUX__
PTRSZVAL OpenV4L( char *name )
{
	PDEVICE_DATA pDevice;
	if( !name )
	{
		char default_device[256];
		int n;
		for( n = 0; n < 16; n++ )
		{
		// woo fun with recursion!
			sprintf( default_device, WIDE("/devices/video%d"), n );
			pDevice = (PDEVICE_DATA)OpenV4L( default_device );
			if( !pDevice )
			{
				sprintf( default_device, WIDE("/dev/video%d"), n );
				pDevice = (PDEVICE_DATA)OpenV4L( default_device );
			}
			if( pDevice )
            break;
		}
		if( !pDevice )
			lprintf( WIDE("Failed to open default devices enumerated from 0 to 16") );
		return (PTRSZVAL)pDevice;
	}

	{
		int handle = open( name, O_RDONLY );
		if( handle == -1 )
		{
         lprintf( WIDE("attempted open of %s failed"), name );
			return (PTRSZVAL)NULL;
		}
		lprintf( WIDE("attempted open of %s success!"), name );
		pDevice = Allocate( sizeof( *pDevice ) );
		MemSet( pDevice, 0, sizeof( *pDevice ) );
		pDevice->flags.bFirstRead = 1;
		pDevice->handle = handle;

		ioctl(handle, VIDIOCGCAP, &pDevice->caps );
		lprintf( WIDE("Some interesting ranges: (%d,%d) (%d,%d)")
				 , pDevice->caps.minwidth
				 , pDevice->caps.minheight
				 , pDevice->caps.maxwidth
				 , pDevice->caps.maxheight
				 );

		ioctl(pDevice->handle, VIDIOCGCHAN, &pDevice->chan);
		if(pDevice->channels[pDevice->curr_channel])
			pDevice->chan.channel = 0;
		else
			pDevice->chan.channel = 1;  //set to 1 for video line input
		ioctl(pDevice->handle, VIDIOCSCHAN, &pDevice->chan);

		pDevice->tuner.tuner = 0;
		ioctl(pDevice->handle, VIDIOCGTUNER, &pDevice->tuner);
		pDevice->tuner.mode = VIDEO_MODE_NTSC;
		ioctl(pDevice->handle, VIDIOCSTUNER, &pDevice->tuner);

		//ioctl(pDevice->handle, VIDIOCSFREQ, &pDevice->freq);
		ioctl(pDevice->handle, VIDIOCGFREQ, &pDevice->freq);

		ioctl(pDevice->handle, VIDIOCGAUDIO, &pDevice->audio);
		pDevice->audio.flags &= ~VIDEO_AUDIO_MUTE;
		ioctl(pDevice->handle, VIDIOCSAUDIO, &pDevice->audio);

		ioctl( pDevice->handle, VIDIOCGUNIT, &pDevice->unit );
		lprintf( WIDE("related units: %d %d %d %d %d")
				 , pDevice->unit.video
				 , pDevice->unit.vbi
				 , pDevice->unit.radio
				 , pDevice->unit.audio
				 , pDevice->unit.teletext );

		ioctl( pDevice->handle, VIDIOCGWIN, &pDevice->vwin );
		lprintf( "Format parameters %d %d %d %d %d %d"
				 , pDevice->vwin.x
				 , pDevice->vwin.y
				 , pDevice->vwin.width
				 , pDevice->vwin.height
				 , pDevice->vwin.chromakey
				 , pDevice->vwin.flags );


		if( ioctl(pDevice->handle, VIDIOCGMBUF, &pDevice->mbuf) == -1 )
		{
			CloseV4L( &pDevice );
			return (PTRSZVAL)NULL;
		}
      lprintf( WIDE("Driver claims it has %d frames"), pDevice->mbuf.frames );
		MemSet( ( pDevice->pFrames = Allocate( sizeof( pDevice->pFrames[0] ) * pDevice->mbuf.frames ) )
  				, 0
 				, sizeof( pDevice->pFrames[0] ) * pDevice->mbuf.frames );
		pDevice->vmap = Allocate( sizeof( pDevice->vmap[0] )
										* pDevice->mbuf.frames );
      pDevice->ready.size = pDevice->mbuf.frames + 1;
      pDevice->done.size = pDevice->mbuf.frames + 1;
      pDevice->capture.size = pDevice->mbuf.frames + 1;
      pDevice->ready.frames = Allocate( sizeof( *pDevice->ready.frames ) * (pDevice->mbuf.frames + 1) );
      pDevice->done.frames = Allocate( sizeof( *pDevice->done.frames ) * (pDevice->mbuf.frames + 1) );
		pDevice->capture.frames = Allocate( sizeof( *pDevice->capture.frames ) * (pDevice->mbuf.frames + 1) );

    	 //pDevice->mbuf.frames = 2;
		ioctl( pDevice->handle, VIDIOCGPICT, &pDevice->picture );
		lprintf( WIDE("picture stats: b: %d h: %d col: %d con: %d wht: %d dep: %d pal: %d")
				 , pDevice->picture.brightness
				 , pDevice->picture.hue
				 , pDevice->picture.colour
				 , pDevice->picture.contrast
				 , pDevice->picture.whiteness
				 , pDevice->picture.depth
				 , pDevice->picture.palette );
      // must set depth for palette 32
      pDevice->picture.depth = 32;
		pDevice->picture.palette = VIDEO_PALETTE_RGB32;
		ioctl( pDevice->handle, VIDIOCSPICT, &pDevice->picture );

		{
			int n;
			lprintf( WIDE(" about to mmap with %u handle of %d" )
							 , pDevice->mbuf.size
							 , pDevice->handle

                 );

			pDevice->map = (P_8)mmap(0
									 , pDevice->mbuf.size//4096 //pDevice->mbuf.size/
									 , PROT_READ
									 , MAP_SHARED
									 , pDevice->handle
									 , 0
									 );
			if( pDevice->map == MAP_FAILED )
			{
            pDevice->flags.bNotMemMapped = TRUE;
            //lprintf( "Failed to mmap... %d %d %d(%d)", pDevice->mbuf.size, pDevice->handle, getpagesize(), pDevice->mbuf.size%getpagesize() );
				//perror( WIDE("Failed to mmap...") );
				//CloseV4L( &pDevice );
				//return (PTRSZVAL)NULL;
				for( n = 0; n < pDevice->mbuf.frames; n++ )
				{
					pDevice->pFrames[n] = NULL;
					pDevice->pFrames[n] = MakeImageFile( pDevice->vwin.width
																  , pDevice->vwin.height );
				}
			}
			else
			{
				for( n = 0; n < pDevice->mbuf.frames; n++ )
				{
					pDevice->pFrames[n] = NULL;
					pDevice->pFrames[n] = RemakeImage( pDevice->pFrames[n]
																, (PCOLOR)(pDevice->map + pDevice->mbuf.offsets[n])
																, pDevice->vwin.width
																, pDevice->vwin.height );
				}
			}
		}
 		{
			int n;
			for( n = 0; n < pDevice->mbuf.frames; n++ )
			{
            lprintf( "put %d in done", n );
            EnqueFrame( &pDevice->done, n );
			}
		}
      lprintf( "cycle read..." );
		ThreadTo( CycleReadThread, (PTRSZVAL)pDevice );
		return (PTRSZVAL)pDevice;
	}
}
#endif

//--------------------------------------------------------------------

void AddCaptureCallback( PCAPTURE_DEVICE pDevice, int (CPROC *callback)(PTRSZVAL psv, PCAPTURE_DEVICE pDev ), PTRSZVAL psv )
{
	PCAPTURE_CALLBACK pcc = New( CAPTURE_CALLBACK );
	pcc->callback = callback;
	pcc->psv = psv;
	pcc->next = NULL;
   pcc->me = NULL;
   lprintf( WIDE("Added callback... %p (%p)"), pcc, callback );
   LinkLast( pDevice->callbacks, PCAPTURE_CALLBACK, pcc );
}

//--------------------------------------------------------------------

int CPROC DisplayAFrame( PTRSZVAL psv, PCAPTURE_DEVICE pDevice )
{
	static _32 tick;
	_32 newtick = GetTickCount();
   static _32 lastloss;
   static _32 frames; frames++;
	if( tick )
	{
		if( newtick - tick > 50 )
		{
			//lprintf( WIDE("Lost something? %d (%d?) %d %d"), newtick - tick, (newtick-tick)/33, frames, frames-lastloss );
			lastloss = frames;
		}
	}
   tick = newtick;
		  //PFRAME pFrame = (PFRAME)psv;
	{
		//PRENDERER pRender = (PRENDERER)psv;
		//Image image;
      		//GetDeviceData( pDevice, (POINTER*)&image, NULL );
		//if( image )
		//	BlotImage( GetDisplayImage( pRender ), image, 0, 0 );
		//lprintf( WIDE("Displayed frame...") );
								// if I blotcolor over the region of the overlay, then I can draw it
								// zero alpha and zero color auto full transparent under overlay.
								//BlotImage( GetDisplayImage( pRender ), pDevice->pResultFrame, 0, 0 );

							// restore second device's frame...
							// need to perhaps open clipping regions for smart drawing
							// through the display layer...
		if(pDevice && pDevice->data)
		{
			INDEX idx;
			PSI_CONTROL pc;
			LIST_FORALL( g.controls, idx, PSI_CONTROL, pc )
			{
				if( IsControlHidden( pc ) )
					continue;
				//lprintf( "Blotting output..." );
				BlotScaledImage( GetControlSurface( pc )
									, (Image)pDevice->data
									 //										 , 0, 0
									 //										 , 240, 180
									);
				SmudgeCommon( pc );
			}
		}
		//UpdateDisplay( pRender );
		//UpdateFrame( pFrame, 0, 0, -1, -1 );
	}
   return 1;
}

//--------------------------------------------------------------------

int CPROC DisplayASmallFrame( PTRSZVAL psv, PCAPTURE_DEVICE pDevice )
{
	static _32 tick;
	_32 newtick = GetTickCount();
   static _32 lastloss;
   static _32 frames; frames++;
	if( tick )
	{
		if( newtick - tick > 50 )
		{
			//lprintf( WIDE("Lost something? %d (%d?) %d %d"), newtick - tick, (newtick-tick)/33, frames, frames-lastloss );
			lastloss = frames;
		}
	}
   tick = newtick;
	//PFRAME pFrame = (PFRAME)psv;
	{
		//PRENDERER pRender = (PRENDERER)psv;
		//BlotImage( GetFrameSurface( pFrame ), pDevice->pCurrentFrame, 0, 0 );
		//lprintf( WIDE("Display frame...") );
		//BlotScaledImageSizedTo( GetDisplayImage( pRender )
//									 , (Image)pDevice->data
//									 , 0, 0
//									 , 240, 180 );
//		UpdateDisplayPortion( pRender, 0, 0,  240, 180 );
		//UpdateFrame( pFrame, 0, 0, -1, -1 );
	}
   return 0;
}

//---------------------------------------------------------------------------

typedef struct codec_tag {
	PCAPTURE_DEVICE pDevice;
	PCOMPRESS pCompress;
   // data is the data frame shared between compress and decompress gadgets
   POINTER data;
	PDECOMPRESS pDecompress;
	_64 bytes;
   _32 start;
} *PCODEC;


void GetDeviceData( PCAPTURE_DEVICE pDevice, POINTER *data, INDEX *length )
{
	(*data) = pDevice->data;
	if( length )
		(*length) = pDevice->length;
}

void SetDeviceDataEx( PCAPTURE_DEVICE pDevice, POINTER data, INDEX length
						  , ReleaseBitStreamData release,PTRSZVAL psv)
{
	if( pDevice->release )
      pDevice->release( pDevice->psv, data, length );
	pDevice->data = data;
	pDevice->length = length;
	pDevice->release = release;
   pDevice->psv = psv;
}

PTRSZVAL CPROC SetNetworkBroadcast( PTRSZVAL psv, arg_list args )
{
	PARAM( args, char*, addr );
   lprintf( WIDE("~~~~~~~") );
   g.saBroadcast = CreateSockAddress( addr, 0 );
   return psv;
}

PTRSZVAL CPROC SetDisplayFilterSize( PTRSZVAL psv, arg_list args )
{
   PARAM( args, S_64, x );
   PARAM( args, S_64, y );
   PARAM( args, S_64, width );
	PARAM( args, S_64, height );
   g.display_x = x;
   g.display_y = y;
	g.display_width = width;
	g.display_height = height;

   return psv;
}

void ReadConfig( char *name )
{
	PCONFIG_HANDLER pch = CreateConfigurationEvaluator();
	AddConfigurationMethod( pch, WIDE("display at (%i,%i) %i by %i"), SetDisplayFilterSize );
	AddConfigurationMethod( pch, WIDE("broadcast %m"), SetNetworkBroadcast );
   ProcessConfigurationFile( pch, name, 0 );

}

PTRSZVAL CPROC ThreadThing( PTHREAD thread )
{
	{
		//PRENDERER pRender;
      g.pDev[0] = CreateCaptureStream();
#ifdef __LINUX__
		{
			PTRSZVAL psvCap;
			psvCap = OpenV4L( NULL );
			//psvCap = OpenV4L( NULL );
			if( !psvCap )
			{
				return 0;
			}
			AddCaptureCallback( g.pDev[0], GetCapturedFrame, psvCap );
			GetCaptureSizeV4L( (PDEVICE_DATA)psvCap, &width, &height );
			//SetChannelV4L( (PDEVICE_DATA)psvCap, 45 );
		}
#else
		{
			PTRSZVAL psvCap;
         psvCap = OpenV4W( g.pDev[0] );
		}
//  		AddCaptureCallback( g.pDev[0], GetNetworkCapturedFrame, OpenNetworkCapture( NULL ) );
//  		AddCaptureCallback( g.pDev[0], DecompressFrame
//  								, OpenDecompressor( g.pDev[0]
//  														, width, height ) );
#endif
		//pRender = OpenDisplaySizedAt( 0, g.display_width, g.display_height, g.display_x, g.display_y );
      		//UpdateDisplay( pRender );
		AddCaptureCallback( g.pDev[0], DisplayAFrame, (PTRSZVAL)0 );
#ifdef __LINUX__
		//g.pDev[1] = OpenV4L( NULL );
		if( !g.pDev[1] )
		{
         lprintf( WIDE("Failed to open second capture device") );
		}
		else
		{
			AddCaptureCallback( g.pDev[1], DisplayASmallFrame, (PTRSZVAL)0 );
		}
#endif
		//if(0)
#ifdef __LINUX__
      // apply capture-compressor
		{
			int n;
         for( n = 0; n < 2; n++ )
			{
#ifndef __LINUX64__
				if( g.pDev[n] )
				{
					PCODEC codec = Allocate( sizeof( *codec ) );
					codec->pDevice = g.pDev[n];
					codec->data = NULL;
					codec->bytes = 0;
               codec->start = GetTickCount();
//  					codec->pCompress = OpenCompressor( (PCAPTURE_DEVICE)g.pDev[n]
//  																, width, height );
//  					codec->pDecompress = OpenDecompressor( (PCAPTURE_DEVICE)g.pDev[n]
//  																	 , width, height );
//					AddCaptureCallback( g.pDev[n], CompressFrame, (PTRSZVAL)codec->pCompress );
				}
#endif
			}
		}
#ifndef __LINUX64__
		{
//  			AddCaptureCallback( g.pDev[0], RenderNetworkFrame, OpenNetworkRender( NULL ) );
		}
#endif
#endif

   //SetAllocateLogging( TRUE );
		{
			char whatever[2];
#ifdef WIN32
         g.capture_thread = GetCurrentThreadId();
         g.flags.bRunning = 1;
         Start(); // v4w start();
			{
				MSG msg;
				while( !g.flags.bExit && GetMessage( &msg, NULL, 0, 0 ) )
					DispatchMessage( &msg );
			}
         Stop();
			lprintf( "video capture thread is exiting." );
         g.flags.bRunning = 0;
#endif
         if( !g.flags.bExit )
			{
				do{

					lprintf( WIDE("Entering wait...") );
					fgets( whatever, 2, stdin );
				} while( whatever[0] != 'q' );
			}
		}
      return 0;
      // these should call all the callback destruction calls...
		//CloseCapture( g.pDev[0] );
		//CloseCapture( g.pDev[1] );
		//CloseDisplay( pRender );
	}
}

static int inited;
void MainInit( char *MyConfig )
{
	if( !inited )
	{
		_32 width, height;
      //g.pii = GetImageInterface();
		//SetSystemLog( SYSLOG_FILE, stdout );
		//SystemLogTime( SYSLOG_TIME_HIGH|SYSLOG_TIME_DELTA );
		//SetBlotMethod( BLOT_C );
		//SetAllocateLogging( TRUE );
		NetworkWait(NULL,16, 16 );
		ReadConfig( MyConfig );
		//SetAllocateLogging( TRUE );
		ThreadTo( ThreadThing, 0 );
		inited = 1;
	}
}


EasyRegisterControl( "Video Control", 0 );

static int OnCreateCommon( "Video Control" )( PSI_CONTROL pc )
{
	MainInit( "stream.conf" );
   AddLink( &g.controls, pc );
   return TRUE;
}

static int OnDrawCommon( "Video Control" )( PSI_CONTROL pc )
{
   return 0;
}



PRELOAD( capture_window_registration )
{
}

#ifndef __LIBRARY__
int main( int argc, char **argv )
{

	MainInit(argc < 2 ? "stream.conf":argv[1]);
	while( 1 )
      Relinquish();
	return 0;
}

#endif

