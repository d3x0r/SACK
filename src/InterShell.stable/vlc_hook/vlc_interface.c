//#define DEBUG_LOCK_UNLOCK
//#define USE_PRE_1_1_0_VLC

#define USE_RENDER_INTERFACE l.pir
#define USE_IMAGE_INTERFACE l.pii

#define INVERT_DATA
#define VLC_INTERFACE_SOURCE

#include <stdhdrs.h>
#include <sqlgetoption.h>
#include <deadstart.h>
#include <idle.h>
#include <controls.h>
#include "../../contrib/libvlc/2.0.1/sdk/include/vlc/vlc.h"
//#include <vlc/vlc_version.h>
//#include <vlc/mediacontrol.h>
#include <filesys.h>
#include <sack_types.h>
#include "vlcint.h"

struct vlc_release
{
	_32 tick;
	struct my_vlc_interface *pmyi;
};

static struct {
	PRENDER_INTERFACE pir;
	PIMAGE_INTERFACE pii;
	TEXTSTR vlc_path;
	TEXTSTR vlc_config;
	PLIST vlc_releases;
	CRITICALSECTION buffer;
	CRITICALSECTION creation;
	struct
	{
		BIT_FIELD bLogTiming : 1;
		BIT_FIELD bHoldUpdates : 1; // rude global flag, stops all otuputs on all surfaces...
		BIT_FIELD bUpdating : 1; // set while an update is happening...
		BIT_FIELD bRequireDraw : 1;
	} flags;
	PLIST interfaces;
	int verbosity;
	int config_vlc_width;
	int config_vlc_height;
	Image current_image;
}l;


void ReleaseInstance( struct my_vlc_interface *pmyi );

void CPROC Cleaner( PTRSZVAL psv )
{
	struct vlc_release *release;
	INDEX idx;
	LIST_FORALL( l.vlc_releases, idx, struct vlc_release *, release )
	{
		if( ( release->tick + 250 ) < timeGetTime() )
		{
			ReleaseInstance( release->pmyi );
			SetLink( &l.vlc_releases, idx, NULL );
		}
	}
}

PRELOAD( InitInterfaces )
{
	TEXTCHAR vlc_path[256];

	l.pir = GetDisplayInterface();
	l.pii = GetImageInterface();
	l.flags.bRequireDraw = RequiresDrawAll();


	snprintf( vlc_path, sizeof( vlc_path ), WIDE( "%s/vlc" ), OSALOT_GetEnvironmentVariable( WIDE( "MY_LOAD_PATH" ) ) );
#ifndef __NO_OPTIONS__
	SACK_GetPrivateProfileString( WIDE( "vlc/config" ), WIDE( "vlc path" ), vlc_path, vlc_path, sizeof( vlc_path ), WIDE( "video.ini" ) );
	l.verbosity = SACK_GetPrivateProfileInt( WIDE( "vlc/config" ), WIDE( "log verbosity" ), 0, WIDE( "video.ini" ) );
	l.config_vlc_width = SACK_GetPrivateProfileInt( WIDE( "vlc/config" ), WIDE( "capture image width" ), 640, WIDE( "video.ini" ) );
	l.config_vlc_height = SACK_GetPrivateProfileInt( WIDE( "vlc/config" ), WIDE( "capture image height" ), 480, WIDE( "video.ini" ) );
	l.vlc_path = StrDup( vlc_path );
#else
   l.verbosity = 0;
   l.vlc_path = WIDE( "vlc" );
#endif
	l.vlc_config = WIDE( "@/vlc.cfg" );
#ifndef __NO_OPTIONS__
	SACK_GetPrivateProfileString( WIDE( "vlc/config" ), WIDE( "vlc config" ), l.vlc_config, vlc_path, sizeof( vlc_path ), WIDE( "video.ini" ) );
	l.vlc_config = ExpandPath( vlc_path );
#else
#endif

#ifndef __NO_OPTIONS__
	l.flags.bLogTiming = SACK_GetPrivateProfileInt( WIDE( "vlc/config" ), WIDE( "log timing" ), 0, WIDE( "video.ini" ) );
#endif
	lprintf( WIDE( "path is %s  config is %s" ), l.vlc_path, l.vlc_config );
	{
		int n;
		for( n = 0; l.vlc_path[n]; n++ )
			if( l.vlc_path[n] == '/' )
				l.vlc_path[n] = '\\';
	}
}

typedef void (CPROC *mylibvlc_callback_t )( const libvlc_event_t *, void * );


#define EXCEPT_PARAM
#define PASS_EXCEPT_PARAM
typedef int libvlc_exception_t;
typedef int mediacontrol_Exception;
typedef int mediacontrol_Instance;
typedef int mediacontrol_PositionKey;

static struct vlc_interface
{
#define declare_func(a,b,c) a (CPROC *b) c
#define setup_func(a,b,c) vlc.b=(a(CPROC*)c)LoadFunction( lib, _WIDE(#b) )

	declare_func( libvlc_instance_t *,libvlc_new,( int , const char *const *) );

	declare_func( libvlc_media_t *, libvlc_media_new, (
                                   libvlc_instance_t *,
																		const char *  ) );
	declare_func( libvlc_media_player_t *, libvlc_media_player_new_from_media, ( libvlc_media_t * ) );
	declare_func( void, libvlc_media_release,(
                                   libvlc_media_t * ) );
	declare_func( void, libvlc_media_player_play, ( libvlc_media_player_t * ) );
	declare_func( void, libvlc_media_player_pause, ( libvlc_media_player_t * ) );
	declare_func( void, libvlc_media_player_stop, ( libvlc_media_player_t * ) );
	declare_func( void, libvlc_release, ( libvlc_instance_t * ) );
	declare_func( void, libvlc_media_player_release, ( libvlc_media_player_t * ) );
	declare_func( _32, libvlc_media_player_get_position, ( libvlc_media_player_t *) );
   declare_func( libvlc_time_t, libvlc_media_player_get_length, ( libvlc_media_player_t *) );
	declare_func( libvlc_time_t, libvlc_media_player_get_time, ( libvlc_media_player_t *) );
   declare_func( void, libvlc_media_player_set_time, ( libvlc_media_player_t *, libvlc_time_t ) );
   /*
   declare_func( mediacontrol_Instance *, mediacontrol_new_from_instance,( libvlc_instance_t*,
																								  mediacontrol_Exception * ) );
   declare_func( mediacontrol_Exception *,
					 mediacontrol_exception_create,( void ) );
   declare_func( mediacontrol_StreamInformation *,
					 mediacontrol_get_stream_information,( mediacontrol_Instance *self,
																	  mediacontrol_PositionKey a_key,
																	  mediacontrol_Exception *exception ) );
	*/
   declare_func( void,
					 libvlc_media_list_add_media, ( libvlc_media_list_t *,
															 libvlc_media_t *   ) );
   declare_func( libvlc_media_list_t *,
					 libvlc_media_list_new,( libvlc_instance_t * ) );
   declare_func( void,
					 libvlc_media_list_release, ( libvlc_media_list_t * ) );
   declare_func( void,
    libvlc_media_list_player_set_media_player,(
                                     libvlc_media_list_player_t * ,
                                     libvlc_media_player_t *  ) );
   declare_func( void,
    libvlc_media_list_player_set_media_list,(
                                     libvlc_media_list_player_t * ,
                                     libvlc_media_list_t *  ) );
	declare_func( libvlc_media_t *, libvlc_media_new_location, (
                                   libvlc_instance_t *,
																		const char * ) );
	declare_func( libvlc_media_t *, libvlc_media_new_path, (
                                   libvlc_instance_t *,
																		const char * ) );
   declare_func( void,
    libvlc_media_list_player_play, ( libvlc_media_list_player_t *   ) );
   declare_func( void,
    libvlc_media_list_player_pause, ( libvlc_media_list_player_t *   ) );
   declare_func( void,
    libvlc_media_list_player_stop, ( libvlc_media_list_player_t *   ) );
   declare_func( void,
    libvlc_media_list_player_next, ( libvlc_media_list_player_t *  ) );
   declare_func( libvlc_media_list_player_t *,
    libvlc_media_list_player_new, ( libvlc_instance_t *  ) );
   declare_func( void,
    libvlc_media_list_player_release, ( libvlc_media_list_player_t * ) );
	declare_func( libvlc_media_player_t *, libvlc_media_player_new, ( libvlc_instance_t * ) );
	declare_func( void,  libvlc_media_list_player_play_item_at_index, (libvlc_media_list_player_t *, int ) );
   declare_func( libvlc_state_t,
					 libvlc_media_list_player_get_state, ( libvlc_media_list_player_t *  ) );
    declare_func( void, libvlc_video_set_callbacks, ( libvlc_media_player_t *mp,
    void *(CPROC *lock) (void *opaque, void **plane),
    void (CPROC *unlock) (void *opaque, void *picture, void *const *plane),
    void (CPROC *display) (void *opaque, void *picture),
    void *opaque ) );

   declare_func( int, libvlc_event_attach, ( libvlc_event_manager_t *,
                                         libvlc_event_type_t,
                                         mylibvlc_callback_t,
                                         void *  ) );
   declare_func( void, libvlc_event_detach, ( libvlc_event_manager_t *p_event_manager,
                                         libvlc_event_type_t i_event_type,
                                         mylibvlc_callback_t f_callback,
                                         void *user_data  ) );
   declare_func( libvlc_event_manager_t *,
					 libvlc_media_event_manager,( libvlc_media_t * p_md   ) );
	declare_func( libvlc_event_manager_t *, libvlc_media_player_event_manager, ( libvlc_media_player_t * ) );
   declare_func( int, libvlc_video_get_size, ( libvlc_media_player_t *p_mi, unsigned num,
															 unsigned *px, unsigned *py ) );
   declare_func( void,libvlc_media_parse, (libvlc_media_t *media) );

	//void (*raise)(libvlc_exception_t*);
#ifdef USE_PRE_1_1_0_VLC
#define RAISE(a,b) a.raise( b )
#else
#define RAISE(a,b)
#endif

   declare_func( void, libvlc_media_add_option, (
                                   libvlc_media_t * p_md,
																 const char * ppsz_options    ) );

   declare_func( void, libvlc_video_set_format, ( libvlc_media_player_t *mp, const char *chroma,
                              unsigned width, unsigned height,
                              unsigned pitch ) );


	void (*raiseEx)(libvlc_exception_t* DBG_PASS );

   declare_func( libvlc_event_manager_t *,
    libvlc_vlm_get_event_manager, ( libvlc_instance_t *p_instance ) );
   declare_func( int, libvlc_vlm_play_media, ( libvlc_instance_t *p_instance,
                                           const char *psz_name ) );

} vlc;

struct my_vlc_interface
{
	CTEXTSTR name;
	libvlc_exception_t ex;
	libvlc_instance_t * inst;
	libvlc_media_player_t *mp;
	libvlc_media_list_player_t *mlp;
	libvlc_media_t *m;
	//libvlc_media_descriptor_t *md;
	libvlc_media_list_t *ml;
	libvlc_event_manager_t *mpev; // player event
	libvlc_event_manager_t *mev; // media event

	//mediacontrol_Instance *mc;
	//mediacontrol_Exception *mcex;
	//mediacontrol_StreamInformation *si;
	//int host_image_show;
	//int host_image_grab;

	//Image host_images[6];
	Image host_image;
	PSI_CONTROL host_control;
	PRENDERER host_surface;
	int nSurface;
	Image surface;

	int list_count;
	int list_index;

	PTHREAD waiting;
	struct {
		BIT_FIELD bDone : 1;
		BIT_FIELD bPlaying : 1;
		BIT_FIELD bNeedsStop : 1;
		BIT_FIELD bStarted : 1; // we told it to play, but it might not be playing yet
		BIT_FIELD direct_output : 1;
		BIT_FIELD transparent : 1;
		BIT_FIELD bAutoClose : 1;
		BIT_FIELD bWantPlay : 1; // play was issued before player finished starting (loading)
		BIT_FIELD bWantStop : 1;
		BIT_FIELD bWantPause : 1;
	} flags;

	PLIST controls;
	PLIST panels;

	_32 image_w, image_h;
	int images; // should be a count of frames in available and updated.
	PLINKQUEUE available_frames;
	PLINKQUEUE updated_frames;
	PTHREAD update_thread;

	void (CPROC*StopEvent)(PTRSZVAL psv);
	PTRSZVAL psvStopEvent;

};


static void CPROC _libvlc_callback_t( const libvlc_event_t *event, void *user )
{

}

static PTRSZVAL CPROC replay_thread( PTHREAD thread )
{
	struct my_vlc_interface *pmyi = (struct my_vlc_interface *)GetThreadParam( thread );
	if( pmyi->StopEvent )
	{
		pmyi->StopEvent( pmyi->psvStopEvent );
	}
	else
	{
		lprintf( WIDE( "trying to loop..." ) );
		vlc.libvlc_media_player_stop (pmyi->mp PASS_EXCEPT_PARAM);
		vlc.libvlc_media_player_play (pmyi->mp PASS_EXCEPT_PARAM);
	}
   return 0;

}

static
void*
CPROC lock_frame(
									 void* psv, void **something
									)
{
	struct my_vlc_interface *pmyi = (struct my_vlc_interface*)psv;
#ifdef DEBUG_LOCK_UNLOCK
	lprintf( WIDE( "LOCK." ) );
#endif
	if( pmyi->flags.direct_output && !l.flags.bRequireDraw )
	{
		CDATA *data;
		pmyi->host_image = GetDisplayImage( pmyi->host_surface );
		data = GetImageSurface( pmyi->host_image );
#ifdef INVERT_DATA
		data += pmyi->host_image->pwidth * (pmyi->host_image->height -1);
#endif
		(*something) = data;
	}
	else
	{
		Image capture = (Image)DequeLink( &pmyi->available_frames );
		CDATA *data;
#ifdef DEBUG_LOCK_UNLOCK
		lprintf( WIDE( "locking..." ) );
#endif
		if( !capture )
		{
			pmyi->images++;
			lprintf( WIDE( "Ran out of images... adding a new one... %d" ), pmyi->images );
			capture = MakeImageFile( pmyi->image_w, pmyi->image_h );
		}

		pmyi->surface = capture;
		pmyi->host_image = capture; // last captured image for unlock to enque.
		data = GetImageSurface( pmyi->host_image );

#ifdef INVERT_DATA
		data += pmyi->surface->pwidth * (pmyi->surface->height -1);
#endif
#ifdef DEBUG_LOCK_UNLOCK
		lprintf( WIDE( "Resulting surface %p" ), data );
#endif
		(*something) = data;
	}
	return NULL; // ID passed later for unlock, and used for display
}

static
#ifndef USE_PRE_1_1_0_VLC
void
#else
int
#endif
 CPROC unlock_frame(
#ifndef USE_PRE_1_1_0_VLC
										void* psv
									  , void *id, void *const *p_pixels
#else
                              PTRSZVAL psv
#endif
									  )
{
	struct my_vlc_interface *pmyi = (struct my_vlc_interface*)psv;
	// in this case, we want to update the control - it's already had its content filled.
#ifdef DEBUG_LOCK_UNLOCK
	lprintf( WIDE( "unlock" ) );
#endif
	if( pmyi->flags.direct_output && !l.flags.bRequireDraw )
	{
		//pmyi->surface = pmyi->host_image;
		//AdjustAlpha( pmyi);
		if( pmyi->flags.transparent )
			IssueUpdateLayeredEx( pmyi->host_surface, TRUE, 0, 0, pmyi->host_image->width, pmyi->host_image->height DBG_SRC );
		else
			Redraw( pmyi->host_surface );
	}
	else
	{
		EnqueLink( &pmyi->updated_frames, pmyi->host_image );
		if( !l.flags.bRequireDraw )
			WakeThread( pmyi->update_thread );
		else
			MarkImageDirty( pmyi->host_image );
	}
#ifdef USE_PRE_1_1_0_VLC
	return 0;
#endif

}

static void CPROC display_frame(void *data, void *id)
{
#ifdef DEBUG_LOCK_UNLOCK
	lprintf( WIDE( "DISPLAY FRAME" ) );
#endif
    /* VLC wants to display the video */
//    (void) data;
    //assert(id == NULL);
}




static void CPROC PlayerEvent( const libvlc_event_t *event, void *user )
{
	struct my_vlc_interface *pmyi = (struct my_vlc_interface *)user;
	//xlprintf(2100)( WIDE( "Event %d" ), event->type );
	switch( event->type )
	{
	default:
		xlprintf(900)( WIDE( "Unhandled Event %d" ), event->type );
		break;
	case libvlc_MediaPlayerBuffering:
      /* no work for us to do, nice to know though? */
		break;
	case libvlc_MediaPlayerVout:
      lprintf("VOUt - setup otuptu now..." );
		{
			unsigned x, y;
			Image image;

			if( vlc.libvlc_video_get_size( pmyi->mp, 0, &x, &y ) )
			{
				lprintf( WIDE( "error... no stream?" ) );
				x = 640;
				y = 480;
			}
			else
				lprintf( WIDE( "Stream size is %d,%d" ), x, y );

			image = MakeImageFile( pmyi->image_w = x, pmyi->image_h = y );
			EnqueLink( &pmyi->available_frames, image );
			
			//lprintf( WIDE( "vlc 1.1.x set callbacks, get ready." ) );
			vlc.libvlc_video_set_callbacks( pmyi->mp, lock_frame, unlock_frame, display_frame, pmyi );
			//lprintf( WIDE( "Output %d %d" ), pmyi->image_w, pmyi->image_h );
			vlc.libvlc_video_set_format(pmyi->mp, "RV32", pmyi->image_w, pmyi->image_h, -(signed)(pmyi->image_w*sizeof(CDATA)));
		}

      break;
	case libvlc_MediaPlayerPlaying:
		//xlprintf(LOG_ALWAYS)( WIDE( "Really playing." ) );
		pmyi->flags.bPlaying = 1;
		if( pmyi->flags.bWantPause )
		{
			pmyi->flags.bWantPause = 0;
			vlc.libvlc_media_player_pause (pmyi->mp PASS_EXCEPT_PARAM);
		}

		break;
	case libvlc_MediaPlayerLengthChanged:
		//lprintf( WIDE( "mangle the format(a)" ) );
		if(0)
		{
	case libvlc_MediaPlayerPositionChanged:
		   ;//lprintf( WIDE( "mangle the format(b)" ) );
		}
#if 0
		{
			unsigned x, y;
			Image image;
			int n;
			for( n = -1; n < 5; n++ )
			{
			if( vlc.libvlc_video_get_size( pmyi->mp, n, &x, &y ) )
			{
				lprintf( WIDE( "error... no stream?" ) );
				x = 640;
				y = 480;
			}
			else
				lprintf( WIDE( "Stream size is %d,%d" ), x, y );
			}
			pmyi->image_w = x;
			pmyi->image_h = y;
			//vlc.libvlc_video_set_format(pmyi->mp, WIDE( "RV32" ), pmyi->image_w, pmyi->image_h, pmyi->image_w*sizeof(CDATA));
		}
#endif
		break;

	case libvlc_MediaPlayerOpening:
		//lprintf( WIDE( "Media opening... that's progress?" ) );
		break;
	case libvlc_MediaPlayerEncounteredError:
		//lprintf( WIDE( "Media connection error... cleanup... (stop it, so we can restart it, clear playing and started)" ) );
		pmyi->flags.bStarted = 0;
		pmyi->flags.bPlaying = 0;
		pmyi->flags.bNeedsStop = 1;

		// http stream fails to connect we get this
		break;
	case libvlc_MediaPlayerStopped:
		//lprintf( WIDE( "Stopped." ) );
		if( pmyi->flags.bAutoClose )
		{
			pmyi->flags.bDone = 1;
			pmyi->flags.bPlaying = 0;
			pmyi->flags.bStarted = 0;
			if( pmyi->waiting )
				WakeThread( pmyi->waiting );
		}
		break;
	case libvlc_MediaPlayerEndReached:
		//lprintf( WIDE( "End reached." ) );
		if( pmyi->flags.bAutoClose )
		{
			//lprintf( WIDE( "Auto closing surface" ) );
			if(0)
			{
				struct vlc_release *vlc_release = New( struct vlc_release );
				vlc_release->pmyi = pmyi;
				vlc_release->tick = timeGetTime();
				if( !l.vlc_releases )
					AddTimer( 100, Cleaner, 0 );
				AddLink( &l.vlc_releases, vlc_release );
			}
			pmyi->flags.bDone = 1;
			pmyi->flags.bPlaying = 0;
			pmyi->flags.bStarted = 0;
			pmyi->flags.bNeedsStop = 1;
			if( pmyi->waiting )
				WakeThread( pmyi->waiting );
		}
		else
		{
			ThreadTo( replay_thread, (PTRSZVAL)pmyi );
		}
		break;
	}

}

static void AdjustAlpha( struct my_vlc_interface *pmyi )
{
	Image image = GetDisplayImage( pmyi->host_surface );
	PCDATA surface = GetImageSurface( image );
	_32 oo = image->pwidth;
	_32 width = image->width;
	_32 height = image->height;
	_32 r;
	for( r = 0; r < height; r++ )
	{
		_32 c;
		for( c = 0; c < width; c++ )
		{
	  // 	surface[c] = surface[c] & 0xFFFFFF;
#if 1
			CDATA p = surface[c];
			COLOR_CHANNEL cr = RedVal( p );
			COLOR_CHANNEL g = GreenVal( p );
			COLOR_CHANNEL b = BlueVal( p );
			COLOR_CHANNEL a;
			if( ((int)cr + (int)g + (int)b) > ( 3*220 ) )
				a = 0;
			else
				a =  ((cr<32?255:(240-cr)) +(g<32?255:(240-g)) +(b<32?255:(240-b) )) / 9;
			surface[c] = SetAlpha( p, a );
#endif
		}
		surface += oo;
	}
}

static LOGICAL OutputAvailableFrame( struct my_vlc_interface *pmyi )
{
	Image output_image = (Image)DequeLink( &pmyi->updated_frames );
	if( output_image )
	{
		Image last_output_image = output_image;
#ifdef DEBUG_LOCK_UNLOCK
		lprintf( WIDE( "Updating..." ) );
#endif
		l.flags.bUpdating = 1;
		while( output_image = (Image)DequeLink( &pmyi->updated_frames ) )
		{
			EnqueLink( &pmyi->available_frames, last_output_image );
			last_output_image = output_image;
		}
		if( l.flags.bHoldUpdates )
		{
			l.flags.bUpdating = 0;
			return TRUE;
		}

		output_image = last_output_image;
		if( pmyi->host_image )
		{
			if( pmyi->controls )
			{
				INDEX idx;
				PSI_CONTROL pc;
				if( l.flags.bLogTiming )
					lprintf( WIDE( "updating controls..." ) );
				LIST_FORALL( pmyi->controls, idx, PSI_CONTROL, pc )
				{
					BeginUpdate( pc );
					if( !IsControlHidden( pc ) )
					{
						Image output = GetControlSurface( pc );
						if( output )
						{
							//AdjustAlpha( pmyi);
							if( l.flags.bLogTiming )
								lprintf( WIDE( "Output to control." ) );
							BlotScaledImageAlpha( output, output_image, ALPHA_TRANSPARENT );
							if( l.flags.bLogTiming )
								lprintf( WIDE( "output the control (screen)" ) );
							// may hide btween here and there...
							UpdateFrame( pc, 0, 0, 0, 0 );
							if( l.flags.bLogTiming )
								lprintf( WIDE( "Finished output." ) );
						}
					}
					else
					{
						if( l.flags.bLogTiming )
							lprintf( WIDE( "hidden control..." ) );
					}
					EndUpdate( pc );
					Relinquish(); // Sleep once... allows others to immediately continue
				}
			}
			if( pmyi->panels )
			{
				INDEX idx;
				PRENDERER render;
				if( l.flags.bLogTiming )
					lprintf( WIDE( "updating panels..." ) );
				LIST_FORALL( pmyi->panels, idx, PRENDERER, render )
				{
					if( IsDisplayHidden( render ) )
						continue;
					{
						Image output = GetDisplayImage( render );
						//lprintf( WIDE( "img %p" ), output );
						if( output )
						{
							if( l.flags.bLogTiming )
								lprintf( WIDE( "adjusting alpha..." ), output->width, output->height );

							if( pmyi->flags.transparent && !l.flags.bRequireDraw )
								AdjustAlpha( pmyi);
							if( l.flags.bLogTiming )
								lprintf( WIDE( "Ouptut to render. %d %d" ), output->width, output->height );
							BlotScaledImage( output, output_image );
							//BlotImage( output, pmyi->host_image, 0, 0 );
							//RedrawDisplay( renderer );
							if( l.flags.bLogTiming )
								lprintf( WIDE( "Force out." ) );
							if( !l.flags.bRequireDraw )
								UpdateDisplayPortion( render, 0, 0, 0, 0 );
							if( l.flags.bLogTiming )
								lprintf( WIDE( "Force out done." ) );

						}
					}
				}
			}
			if( l.flags.bLogTiming )
				lprintf( WIDE( "Update finished." ) );
		}
		else if( pmyi->host_control )
			UpdateFrame( pmyi->host_control, 0, 0, 0, 0 );
		else if( pmyi->host_surface )
		{
			if( l.flags.bRequireDraw )
			{
				BlotScaledImage( pmyi->host_image, output_image );
			}
			else
				UpdateDisplayPortion( pmyi->host_surface, 0, 0, 0, 0 );
		}

		if( l.current_image )
			EnqueLink( &pmyi->available_frames, l.current_image );
		l.current_image = output_image;
		l.flags.bUpdating = 0;
		return TRUE;
	}
	else
	{
		if( l.flags.bRequireDraw && l.current_image )
		{
			if( pmyi->panels )
			{
				INDEX idx;
				PRENDERER render;
				if( l.flags.bLogTiming )
					lprintf( WIDE( "updating panels..." ) );
				LIST_FORALL( pmyi->panels, idx, PRENDERER, render )
				{
					Image output = GetDisplayImage( render );
					BlotScaledImage( output, l.current_image );
				}
			}
			else if( pmyi->host_surface )
			{
				Image output = GetDisplayImage( pmyi->host_surface );
				BlotScaledImage( output, l.current_image );
			}
		}
	}
	return FALSE;
}

// this takes images from the queue of updated images and puts them on the display.
static PTRSZVAL CPROC UpdateThread( PTHREAD thread )
{
	struct my_vlc_interface *pmyi = (struct my_vlc_interface*)GetThreadParam( thread );
	while( 1 )
	{
		if( pmyi->flags.direct_output && !l.flags.bRequireDraw )
		{
			lprintf( WIDE( "Trigger redraw..." ) );
			Redraw( pmyi->host_surface );
		}
		else
		{
			if( OutputAvailableFrame( pmyi ) )
            ; // do nothing.  but still need to sleep if no output
			else
			{
#ifdef DEBUG_LOCK_UNLOCK
				lprintf( WIDE( "Sleeping waiting to update..." ) );
#endif
				WakeableSleep( SLEEP_FOREVER );
			}
		}
	}
}


void ReleaseInstance( struct my_vlc_interface *pmyi )
{
	//DestroyFrame( &pmyi->host_control );
	if( !pmyi->mlp )
	{
		//vlc.libvlc_media_player_stop (pmyi->mp PASS_EXCEPT_PARAM);
		//RAISE( vlc, &pmyi->ex );

		/* Free the media_player */
		lprintf( WIDE( "Releasing instance..." ) );
		vlc.libvlc_media_player_release (pmyi->mp);
		RAISE( vlc, &pmyi->ex );

		if( 0 && pmyi->mpev )
		{
			vlc.libvlc_event_detach( pmyi->mpev
										  , libvlc_MediaPlayerEndReached
										  , PlayerEvent
										  , pmyi
										  PASS_EXCEPT_PARAM);
			RAISE( vlc, &pmyi->ex );
			vlc.libvlc_event_detach( pmyi->mpev
										  , libvlc_MediaPlayerStopped
										  , PlayerEvent
										  , pmyi PASS_EXCEPT_PARAM );
			RAISE( vlc, &pmyi->ex );
		}

		vlc.libvlc_release (pmyi->inst);
		RAISE( vlc, &pmyi->ex );
	}
	if( pmyi->host_surface )
	{
		lprintf( WIDE( "------------- CLOSE DISPLAY ------------" ) );
      //HideDisplay( pmyi->host_surface );
		CloseDisplay( pmyi->host_surface );
	}
		lprintf( WIDE( "Released instance..." ) );

	DeleteLink( &l.interfaces, pmyi );
	Release( pmyi );
}

//
void LoadVLC( void )
{
	PVARTEXT pvt;
	TEXTCHAR *lib;
	int n;
	TEXTCHAR pathbuf2[256];
	if( vlc.libvlc_new )
		return;

	pvt = VarTextCreate();

	OSALOT_AppendEnvironmentVariable( WIDE( "PATH" ), l.vlc_path );
	snprintf( pathbuf2, sizeof( pathbuf2 ), WIDE( "%s\\plugins" ), l.vlc_path );
	for( n = 0; pathbuf2[n]; n++ )
		if( pathbuf2[n] == '/' )
         pathbuf2[n] = '\\';
	OSALOT_AppendEnvironmentVariable( WIDE( "PATH" ), pathbuf2 );
	vtprintf( pvt, WIDE( "%s\\libvlc.dll" ), l.vlc_path );
	lib = GetText( VarTextPeek( pvt ) );
#ifdef USE_PRE_1_1_0_VLC
	//vlc.raise = MyRaise;
	vlc.raiseEx = MyRaise;
#endif
#ifdef USE_PRE_1_1_0_VLC
	setup_func( void, libvlc_exception_init, ( libvlc_exception_t * ) );
#endif
	setup_func( libvlc_instance_t *,libvlc_new,( int , const char *const *) );
	if( !vlc.libvlc_new )
		return;
#ifndef USE_PRE_1_1_0_VLC
	setup_func( libvlc_media_t *, libvlc_media_new_path, ( libvlc_instance_t *p_instance,
																	 const char * psz_mrl
																	  ) );
	setup_func( libvlc_media_t *, libvlc_media_new_location, ( libvlc_instance_t *p_instance,
																	 const char * psz_mrl
																				  ) );
   setup_func( void, libvlc_video_set_format, ( libvlc_media_player_t *, const char *,
                              unsigned , unsigned ,
                              unsigned  ) );
#else
	setup_func( libvlc_media_t *, libvlc_media_new, ( libvlc_instance_t *p_instance,
																	 const char * psz_mrl
																	  ) );
#endif
	setup_func( void, libvlc_video_set_callbacks,( libvlc_media_player_t *,
    void *(*lock) (void *, void **),
    void (*unlock) (void *, void *, void *const *),
    void (*display) (void *, void *),
    void * ) );
   setup_func( void, libvlc_media_add_option, (
                                   libvlc_media_t * p_md,
                                   const char * ppsz_options  ) );
   setup_func( libvlc_media_player_t *, libvlc_media_player_new, ( libvlc_instance_t * ) );
	setup_func( libvlc_media_player_t *, libvlc_media_player_new_from_media, ( libvlc_media_t * ) );
	setup_func( void, libvlc_media_release,(
                                   libvlc_media_t * ) );
	setup_func( libvlc_media_player_t *, libvlc_media_player_new_from_media, ( libvlc_media_t * ) );
	setup_func( void, libvlc_media_release,(
                                   libvlc_media_t * ) );
	setup_func( void, libvlc_media_player_play, ( libvlc_media_player_t * ) );
	setup_func( void, libvlc_media_player_pause, ( libvlc_media_player_t * ) );
	setup_func( void, libvlc_media_player_stop, ( libvlc_media_player_t * ) );
	setup_func( void, libvlc_release, ( libvlc_instance_t * ) );
	setup_func( void, libvlc_media_player_release, ( libvlc_media_player_t * ) );
	setup_func( _32, libvlc_media_player_get_position, ( libvlc_media_player_t *) );
	setup_func( libvlc_time_t, libvlc_media_player_get_length, ( libvlc_media_player_t *) );
   setup_func( libvlc_time_t, libvlc_media_player_get_time, ( libvlc_media_player_t *) );
   /*
	setup_func( mediacontrol_Instance *,
					 mediacontrol_new_from_instance,( libvlc_instance_t*,
																mediacontrol_Exception * ) );
   setup_func( mediacontrol_Exception *,
					 mediacontrol_exception_create,( void ) );
   setup_func( mediacontrol_StreamInformation *,
					 mediacontrol_get_stream_information,( mediacontrol_Instance *self,
																	  mediacontrol_PositionKey a_key,
																	  mediacontrol_Exception *exception ) );
	*/
	setup_func( void,
				  libvlc_media_list_add_media, ( libvlc_media_list_t *,
														  libvlc_media_t *  ) );
   setup_func( void,
    libvlc_media_list_player_set_media_player,(
                                     libvlc_media_list_player_t * ,
                                     libvlc_media_player_t *  ) );
   setup_func( void,
    libvlc_media_list_player_set_media_list,(
                                     libvlc_media_list_player_t * ,
                                     libvlc_media_list_t *  ) );
   setup_func( void,
    libvlc_media_list_player_play, ( libvlc_media_list_player_t * ) );
   setup_func( void,
    libvlc_media_list_player_stop, ( libvlc_media_list_player_t *  ) );
   setup_func( libvlc_media_list_player_t *,
    libvlc_media_list_player_new, ( libvlc_instance_t *  ) );
   setup_func( void,
    libvlc_media_list_player_release, ( libvlc_media_list_player_t * ) );
   setup_func( libvlc_media_list_t *,
					 libvlc_media_list_new,( libvlc_instance_t * ) );
   setup_func( void,
					 libvlc_media_list_release, ( libvlc_media_list_t * ) );
   setup_func( void,  libvlc_media_list_player_play_item_at_index, (libvlc_media_list_player_t *, int ) );
   setup_func( void,
    libvlc_media_list_player_next, ( libvlc_media_list_player_t * ) );
	setup_func( libvlc_state_t,
    libvlc_media_list_player_get_state, ( libvlc_media_list_player_t *  ) );
   setup_func( int, libvlc_event_attach, ( libvlc_event_manager_t *,
                                         libvlc_event_type_t ,
                                         mylibvlc_callback_t ,
                                         void *  ) );
	setup_func( void, libvlc_event_detach, ( libvlc_event_manager_t *,
														 libvlc_event_type_t ,
														 mylibvlc_callback_t ,
														 void *    ) );
   setup_func( libvlc_event_manager_t *,
					 libvlc_media_event_manager,( libvlc_media_t * p_md   ) );
   setup_func( libvlc_event_manager_t *, libvlc_media_player_event_manager, ( libvlc_media_player_t *  ) );
   setup_func( int, libvlc_video_get_size, ( libvlc_media_player_t *p_mi, unsigned num,
                           unsigned *px, unsigned *py ) );
	setup_func( void,libvlc_media_parse, (libvlc_media_t *media) );
	setup_func( libvlc_event_manager_t *,
					 libvlc_vlm_get_event_manager, ( libvlc_instance_t *p_instance ) );
	setup_func( int, libvlc_vlm_play_media, ( libvlc_instance_t *p_instance,
															 const char *psz_name ) );

	VarTextDestroy( &pvt );
}

void SetupInterface( struct my_vlc_interface *pmyi )
{
	LoadVLC();
    AddLink( &l.interfaces, pmyi );

}


struct my_vlc_interface *FindInstance( CTEXTSTR input )
{
	INDEX idx;
    struct my_vlc_interface *i;

	LIST_FORALL( l.interfaces, idx, struct my_vlc_interface *, i )
	{
		if( StrCaseCmp( i->name, input ) == 0 )
			return i;
	}

	return NULL;
}

struct my_vlc_interface *CreateInstance( CTEXTSTR input )
{

	struct my_vlc_interface *pmyi = FindInstance(input);
	if( !pmyi )
	{
		pmyi = New( struct my_vlc_interface );
		MemSet( pmyi, 0, sizeof( struct my_vlc_interface ) );
		pmyi->name = StrDup( input );
		{
			PVARTEXT pvt;
			TEXTCHAR **argv;
			int argc;

			SetupInterface( pmyi );
			pvt = VarTextCreate();

			vtprintf( pvt,
						//WIDE( "--verbose=2" )
						WIDE( " --file-logging" )
						WIDE( " -I dummy --config=%s --plugin-path=%s\\%s" )
					  , l.vlc_config
					  , l.vlc_path
					  , WIDE( "plugins" )
					  );

			ParseIntoArgs( GetText( VarTextPeek( pvt ) ), &argc, &argv );
#ifdef USE_PRE_1_1_0_VLC
		   vlc.libvlc_exception_init (&pmyi->ex);
#endif
			/* init vlc modules, should be done only once */
#ifdef _UNICODE
		   {
			   char **real_argv;
			   int argn;
			   real_argv = NewArray( char *, argc );
				for( argn = 0; argn < argc; argn++ )
					real_argv[argn] = DupTextToChar( argv[argn] );
#define argv real_argv
#endif
			pmyi->inst = vlc.libvlc_new ( argc, (char const*const*)argv PASS_EXCEPT_PARAM);
#ifdef _UNICODE
		   }
#undef argv 
#endif
			RAISE( vlc, &pmyi->ex );
			VarTextDestroy( &pvt );
		}
	}
   return pmyi;
}

struct my_vlc_interface *CreateInstanceInEx( PSI_CONTROL pc, CTEXTSTR input, CTEXTSTR extra_opts )
{
	struct my_vlc_interface *pmyi;
	pmyi = FindInstance( input );
	if( !pmyi )
	{
		PVARTEXT pvt;
		TEXTCHAR **argv;
		int argc;
		Image surface = GetControlSurface( pc );
		//if( !( pmyi = controls_1 ) )
		{
			//Image image;
			pmyi = New( struct my_vlc_interface );
			MemSet( pmyi, 0, sizeof( struct my_vlc_interface ) );

			//image = MakeImageFile( pmyi->image_w = surface->width, pmyi->image_h = surface->height );
			
			//EnqueLink( &pmyi->available_frames, image );
			if( !l.flags.bRequireDraw )
				pmyi->update_thread = ThreadTo( UpdateThread, (PTRSZVAL)pmyi );

			pmyi->host_image = NULL;
			pmyi->host_control = NULL;//pc;
			pmyi->host_surface = NULL;
			pmyi->image_w = l.config_vlc_width;
			pmyi->image_h = l.config_vlc_height;
			pmyi->name = StrDup( input );
 			SetupInterface( pmyi );

#ifdef __64__
//#error blah.
#endif

			pvt = VarTextCreate();
			vtprintf( pvt, WIDE( "--verbose=%d" )
						//WIDE( " --file-logging" )
						WIDE( " --config=%s" )
						WIDE( " --no-osd" )
						//WIDE( " --file-caching=0" )
						//WIDE( " --plugin-path=%s\\plugins" )
						WIDE( " %s" )
						//, OSALOT_GetEnvironmentVariable( WIDE( "MY_LOAD_PATH" ) )
					  , l.verbosity
					  , l.vlc_config
					  //, l.vlc_path
					  , extra_opts?extra_opts:WIDE( "" )
					  );
			lprintf( WIDE( "Creating instance with %s" ), GetText( VarTextPeek( pvt ) ) );
			ParseIntoArgs( GetText( VarTextPeek( pvt ) ), &argc, &argv );

#ifdef USE_PRE_1_1_0_VLC
			vlc.libvlc_exception_init (&pmyi->ex);
#endif
			/* init vlc modules, should be done only once */
			if( vlc.libvlc_new )
			{
#ifdef _UNICODE
		   {
			   char **real_argv;
			   int argn;
			   real_argv = NewArray( char *, argc );
				for( argn = 0; argn < argc; argn++ )
					real_argv[argn] = DupTextToChar( argv[argn] );
#define argv real_argv
#endif
				pmyi->inst = vlc.libvlc_new ( argc, (char const*const*)argv PASS_EXCEPT_PARAM);
#ifdef _UNICODE
		   }
#undef argv 
#endif
				RAISE( vlc, &pmyi->ex );
			}
			VarTextDestroy( &pvt );

		}
	}

	return pmyi;
}


#ifdef _MSC_VER
static int EvalExcept( int n )
{
	switch( n )
	{
	case 		STATUS_ACCESS_VIOLATION:
		//if( l.flags.bLogKeyEvent )
			lprintf( WIDE( "Access violation - OpenGL layer at this moment.." ) );
	return EXCEPTION_EXECUTE_HANDLER;
	default:
		//if( l.flags.bLogKeyEvent )
			lprintf( WIDE( "Filter unknown : %08X" ), n );

		return EXCEPTION_CONTINUE_SEARCH;
	}
	return EXCEPTION_CONTINUE_EXECUTION;
}
#endif

void CPROC VLC_RedrawCallback( PTRSZVAL psvUser, PRENDERER self )
{
	// output to prenderer.
	struct my_vlc_interface *pmyi;
	pmyi = (struct my_vlc_interface*)psvUser;

	OutputAvailableFrame( pmyi );

}

struct my_vlc_interface *CreateInstanceOn( PRENDERER renderer, CTEXTSTR name, LOGICAL transparent, CTEXTSTR extra_opts )
{
	struct my_vlc_interface *pmyi;
	pmyi  = FindInstance( name );
	if( !pmyi )
	{
		PVARTEXT pvt;
		TEXTCHAR **argv;
		int argc;
		//Image image = MakeImageFile( pmyi->image_w = 352, pmyi->image_h = 240 );//GetDisplayImage( renderer );
		pmyi = New( struct my_vlc_interface );
		MemSet( pmyi, 0, sizeof( struct my_vlc_interface ) );

		if( renderer && l.flags.bRequireDraw )
		{
			//	pmyi->update_thread = ThreadTo( UpdateThread, (PTRSZVAL)pmyi );
			SetRedrawHandler( renderer, VLC_RedrawCallback, (PTRSZVAL)pmyi );
		}

		pmyi->host_control = NULL;
		pmyi->host_surface = renderer;
		if( renderer )
		{
			pmyi->host_image = renderer?GetDisplayImage( renderer ):NULL;
			pmyi->image_w = pmyi->host_image->width;
			pmyi->image_h = pmyi->host_image->height;
		}
		else
		{
			pmyi->host_image = NULL;
			pmyi->image_w = 0;
			pmyi->image_h = 0;
		}
		pmyi->flags.direct_output = 1;
		pmyi->flags.transparent = transparent;


		//EnqueLink( &pmyi->available_frames, pmyi->host_image );
		//pmyi->update_thread = ThreadTo( UpdateThread, (PTRSZVAL)pmyi );

		pmyi->name = StrDup( name );
		//pmyi->host_images = NewArray( Image, 6 );
		AddLink( &pmyi->panels, renderer );
		SetupInterface( pmyi );

		pvt = VarTextCreate();

#ifdef __64__
//#error blah.
#endif

		vtprintf( pvt, WIDE( "--verbose=%d" )
					//WIDE( " --file-logging" )
					WIDE( " --config=%s" )
					WIDE( " --no-osd" )
               //WIDE( " --skip-frames" )
					//WIDE( " --plugin-path=%s\\plugins" )
               //WIDE( " --drop-late-frames" )
               //WIDE( " --file-caching=0" )
					WIDE( " %s" )
                  , l.verbosity
				  , l.vlc_config
				  //, l.vlc_path
				  , extra_opts?extra_opts:WIDE( "" )
				  );

      lprintf( WIDE( "Creating instance with %s" ), GetText( VarTextPeek( pvt ) ) );
		ParseIntoArgs( GetText( VarTextPeek( pvt ) ), &argc, &argv );

#ifdef USE_PRE_1_1_0_VLC
		vlc.libvlc_exception_init (&pmyi->ex);
#endif
		/* init vlc modules, should be done only once */
#ifdef _MSC_VER
		__try
		{
#endif
#ifdef _UNICODE
		   {
			   char **real_argv;
			   int argn;
			   real_argv = NewArray( char *, argc );
				for( argn = 0; argn < argc; argn++ )
					real_argv[argn] = DupTextToChar( argv[argn] );
#define argv real_argv
#endif
			pmyi->inst = vlc.libvlc_new ( argc, (char const*const*)argv PASS_EXCEPT_PARAM);
#ifdef _UNICODE
		   }
#undef argv 
#endif
#ifdef _MSC_VER
		}
		__except( EvalExcept( GetExceptionCode() ) )
		{
									lprintf( WIDE( "Caught exception in libvlc_new" ) );
		}
#endif
		RAISE( vlc, &pmyi->ex );
		VarTextDestroy( &pvt );
   
	}
   return pmyi;
}

struct my_vlc_interface *CreateInstanceAt( CTEXTSTR address, CTEXTSTR instance_name, CTEXTSTR name, LOGICAL transparent )
{
	struct my_vlc_interface *pmyi;
	pmyi  = FindInstance( instance_name );
	if( !pmyi )
	{
		PVARTEXT pvt;
		TEXTCHAR **argv;
		int argc;
		//Image image = MakeImageFile( pmyi->image_w = 352, pmyi->image_h = 240 );//GetDisplayImage( renderer );
		pmyi = New( struct my_vlc_interface );
		MemSet( pmyi, 0, sizeof( struct my_vlc_interface ) );
		pmyi->host_control = NULL;
		pmyi->host_surface = NULL;
		pmyi->host_image = NULL;
		pmyi->image_w = 0;
		pmyi->image_h = 0;
		pmyi->flags.direct_output = 0;
		pmyi->flags.transparent = transparent;

		pmyi->name = StrDup( name );

		SetupInterface( pmyi );

		pvt = VarTextCreate();

		vtprintf( pvt, WIDE( "--verbose=%d" )
					//WIDE( " --file-logging" )
					WIDE( " -I dummy --config=%s" )
					WIDE( " --no-osd" )
					WIDE( " --sout=#transcode{vcodec=theo,vb=800,scale=1,acodec=vorb,ab=128,channels=2,samplerate=44100}:http{mux=ogg,dst=:1234/}" )
					WIDE( " --no-sout-rtp-sap" )
					WIDE( " --no-sout-standard-sap" )
					WIDE( " --sout-all" )
					WIDE( " --sout-keep" )
					//WIDE( " --noaudio" )
               //WIDE( " --skip-frames" )
					WIDE( " --plugin-path=%s\\plugins" )
               //WIDE( " --drop-late-frames" )
               //WIDE( " --file-caching=0" )
               WIDE( " %s" )
                  , l.verbosity
				  , l.vlc_config
				  , l.vlc_path
				  , WIDE( "" )//extra_opts
				  );

		lprintf( WIDE( "Creating instance with %s" ), GetText( VarTextPeek( pvt ) ) );
		ParseIntoArgs( GetText( VarTextPeek( pvt ) ), &argc, &argv );

		/* init vlc modules, should be done only once */
#ifdef _MSC_VER
		__try
		{
#endif
#ifdef _UNICODE
		   {
			   char **real_argv;
			   int argn;
			   real_argv = NewArray( char *, argc );
				for( argn = 0; argn < argc; argn++ )
					real_argv[argn] = DupTextToChar( argv[argn] );
#define argv real_argv
#endif
			pmyi->inst = vlc.libvlc_new ( argc, (char const*const*)argv PASS_EXCEPT_PARAM);
#ifdef _UNICODE
		   }
#undef argv 
#endif
#ifdef _MSC_VER
		}
		__except( EvalExcept( GetExceptionCode() ) )
		{
									lprintf( WIDE( "Caught exception in libvlc_new" ) );
		}
#endif
		RAISE( vlc, &pmyi->ex );
		VarTextDestroy( &pvt );
   
	}
   return pmyi;
}

ATEXIT( unload_vlc_interface )
{
	INDEX idx;
	struct my_vlc_interface *pmyi;
	LIST_FORALL( l.interfaces, idx, struct my_vlc_interface*, pmyi )
	{
		//DestroyFrame( &pmyi->host_control );
#ifdef __WATCOMC__
#ifndef __cplusplus
		_try {
#endif
#endif
			lprintf( WIDE( "Doing stop 2..." ) );
			if( pmyi->flags.bPlaying )
				vlc.libvlc_media_list_player_stop( pmyi->mlp PASS_EXCEPT_PARAM );
			RAISE( vlc, &pmyi->ex );
#ifdef __WATCOMC__
#ifndef __cplusplus
		}
		_except( EXCEPTION_EXECUTE_HANDLER )
		{
			lprintf( WIDE( "Caught exception in stop media list player" ) );
			return;
			;
		}
#endif
#endif
		lprintf( WIDE( "Doing stop..." ) );
		if( pmyi->flags.bPlaying )
		{
			vlc.libvlc_media_player_stop (pmyi->mp PASS_EXCEPT_PARAM);
			RAISE( vlc, &pmyi->ex );
		}

		lprintf( WIDE( "doing release..." ) );
		/* Free the media_player */
		if( pmyi->mp )
			vlc.libvlc_media_player_release (pmyi->mp);
		if( pmyi->ml )
			vlc.libvlc_media_list_release( pmyi->ml );
		if( pmyi->mlp )
			vlc.libvlc_media_list_player_release( pmyi->mlp );

		lprintf( WIDE( "releasing instance..." ) );
		if( vlc.libvlc_release )
		{
			vlc.libvlc_release (pmyi->inst);
			RAISE( vlc, &pmyi->ex );
		}
	}
	/* Stop playing */

}

void PlayItem( struct my_vlc_interface *pmyi )
{
	//lprintf( WIDE("Signal thread to PLAY") );
	/* play the media_player */
	pmyi->flags.bWantPlay = 1;
	WakeThread( pmyi->waiting );
}

void StopItem( struct my_vlc_interface *pmyi )
{
	//lprintf( WIDE("Signal thread to STop") );
	/* play the media_player */
	pmyi->flags.bWantStop = 1;
	WakeThread( pmyi->waiting );

	//ReleaseInstance( pmyi );
}

void PauseItem( struct my_vlc_interface *pmyi )
{
	//lprintf( WIDE("Signal thread to Pause") );
	/* pause the media_player */
	pmyi->flags.bWantPause = 1;
	WakeThread( pmyi->waiting );
}


void SetStopEvent( struct my_vlc_interface *pmyi, void (CPROC*StopEvent)( PTRSZVAL psv ), PTRSZVAL psv )
{
	pmyi->StopEvent = StopEvent;
	pmyi->psvStopEvent = psv;
}


void BindAllEvents( struct my_vlc_interface *pmyi )
{
	int n;
	for( n = 1; n <= libvlc_MediaStateChanged; n++ )
	{
		//	vlc.libvlc_event_attach( pmyi->mpev, n
		//									  , PlayerEvent, pmyi PASS_EXCEPT_PARAM );
	}
	for( n = libvlc_MediaPlayerMediaChanged; n <= libvlc_MediaPlayerLengthChanged; n++ )
	{
		int result;
		if( n == libvlc_MediaPlayerPositionChanged
			|| n == libvlc_MediaPlayerTimeChanged
		  )
		{
			// these are noisy events.
			continue;
		}
		result = vlc.libvlc_event_attach( pmyi->mpev, n
												  , PlayerEvent, pmyi PASS_EXCEPT_PARAM );
      if( result )
			lprintf( WIDE("attach event %d %d"), n, result );
	}
	for( n = libvlc_MediaListItemAdded; n <= libvlc_MediaListWillDeleteItem; n++ )
	{
//		vlc.libvlc_event_attach( pmyi->mpev, n
//									  , PlayerEvent, pmyi PASS_EXCEPT_PARAM );
	}
	for( n = libvlc_MediaListViewItemAdded; n <= libvlc_MediaListViewWillDeleteItem; n++ )
	{
	//	vlc.libvlc_event_attach( pmyi->mpev, n
	//								  , PlayerEvent, pmyi PASS_EXCEPT_PARAM );
	}

	for( n = libvlc_MediaListPlayerPlayed; n <= libvlc_MediaListPlayerStopped; n++ )
	{
	//	vlc.libvlc_event_attach( pmyi->mpev, n
	//								  , PlayerEvent, pmyi PASS_EXCEPT_PARAM );
	}

}


struct my_vlc_interface * PlayItemInEx( PSI_CONTROL pc, CTEXTSTR input, CTEXTSTR extra_opts )
{
	struct my_vlc_interface *pmyi = FindInstance( input );
	if( !pmyi )
	{
		pmyi = CreateInstanceInEx( pc, input, extra_opts );
		if( !pmyi->inst )
			return NULL;
		AddLink( &pmyi->controls, pc );
		//pmyi->ml = vlc.libvlc_media_list_new( pmyi->inst PASS_EXCEPT_PARAM );
		//RAISE( vlc, &pmyi->ex );

#ifndef USE_PRE_1_1_0_VLC
		if( StrStr( input, WIDE( "://" ) ) )
			pmyi->m = vlc.libvlc_media_new_location (pmyi->inst, DupTextToChar( input ) PASS_EXCEPT_PARAM);
		else
			pmyi->m = vlc.libvlc_media_new_path (pmyi->inst, DupTextToChar( input ) PASS_EXCEPT_PARAM);
#else
		pmyi->m = vlc.libvlc_media_new (pmyi->inst, url_name PASS_EXCEPT_PARAM);
#endif
		RAISE( vlc, &pmyi->ex );

		//lprintf( WIDE( "Begin parse..." ) );
      //  this just hangs...
		//vlc.libvlc_media_parse( pmyi->m );
		//lprintf( WIDE( "..." ) );

		if( extra_opts )
		{
			lprintf( WIDE( "Adding options: %s" ), extra_opts );
			//vlc.libvlc_media_add_option( pmyi->m, extra_opts PASS_EXCEPT_PARAM );
			RAISE( vlc, &pmyi->ex );
		}

		if( input && input[0] )
		{
#if use_media_list
			pmyi->ml = vlc.libvlc_media_list_new( pmyi->inst PASS_EXCEPT_PARAM );
			RAISE( vlc, &pmyi->ex );

			pmyi->mev = vlc.libvlc_media_event_manager( pmyi->m PASS_EXCEPT_PARAM );
			RAISE( vlc, &pmyi->ex );

			vlc.libvlc_media_list_add_media( pmyi->ml, pmyi->m PASS_EXCEPT_PARAM );
			RAISE( vlc, &pmyi->ex );

			pmyi->mlp = vlc.libvlc_media_list_player_new( pmyi->inst PASS_EXCEPT_PARAM);
			RAISE( vlc, &pmyi->ex );

			/* Create a media player playing environement */
			pmyi->mp = vlc.libvlc_media_player_new( pmyi->inst PASS_EXCEPT_PARAM);
			RAISE( vlc, &pmyi->ex );

			vlc.libvlc_media_list_player_set_media_list( pmyi->mlp, pmyi->ml PASS_EXCEPT_PARAM );
			RAISE( vlc, &pmyi->ex );

			vlc.libvlc_media_list_player_set_media_player( pmyi->mlp, pmyi->mp PASS_EXCEPT_PARAM );
			RAISE( vlc, &pmyi->ex );

			//vlc.libvlc_media_list_player_play_item_at_index( pmyi->mlp, 0 PASS_EXCEPT_PARAM );
			//RAISE( vlc, &pmyi->ex );
#else
			/* Create a media player playing environement */
			pmyi->mp = vlc.libvlc_media_player_new_from_media( pmyi->m PASS_EXCEPT_PARAM);
			RAISE( vlc, &pmyi->ex );
#endif
			pmyi->mpev = vlc.libvlc_media_player_event_manager( pmyi->mp PASS_EXCEPT_PARAM );
		}
		else
		{
			pmyi->mpev = vlc.libvlc_vlm_get_event_manager( pmyi->inst );
		}

		vlc.libvlc_video_set_callbacks( pmyi->mp, lock_frame, unlock_frame, display_frame, pmyi );
		BindAllEvents( pmyi );

      if( 0 )
		{
			unsigned x, y;
			Image image;

			if( vlc.libvlc_video_get_size( pmyi->mp, 0, &x, &y ) )
			{
				lprintf( WIDE( "error... no stream?" ) );
				x = 640;
				y = 480;
			}
			else
				lprintf( WIDE( "Stream size is %d,%d" ), x, y );

			image = MakeImageFile( pmyi->image_w = x, pmyi->image_h = y );
			EnqueLink( &pmyi->available_frames, image );
			
			//lprintf( WIDE( "vlc 1.1.x set callbacks, get ready." ) );
			//lprintf( WIDE( "Output %d %d" ), pmyi->image_w, pmyi->image_h );
			vlc.libvlc_video_set_format(pmyi->mp, "RV32", pmyi->image_w, pmyi->image_h, -(signed)(pmyi->image_w*sizeof(CDATA)));
		}



		vlc.libvlc_media_release (pmyi->m);

		//vlc.libvlc_media_list_player_play_item_at_index( pmyi->mlp, 0 PASS_EXCEPT_PARAM );
		vlc.libvlc_media_player_play( pmyi->mp PASS_EXCEPT_PARAM);

		pmyi->flags.bStarted = 1;
	}
	else
	{
		if( !pmyi->inst )
			return NULL;
		if( !pmyi->flags.bStarted )
		{
			if( pmyi->flags.bNeedsStop )
			{
				vlc.libvlc_media_player_stop (pmyi->mp PASS_EXCEPT_PARAM);
				pmyi->flags.bNeedsStop = 0;
			}
			lprintf( WIDE("play") );
			vlc.libvlc_media_player_play( pmyi->mp PASS_EXCEPT_PARAM);
			pmyi->flags.bStarted = 1;
		}

		{
			if( FindLink( &pmyi->controls, pc ) == INVALID_INDEX )
			{
				AddLink( &pmyi->controls, pc );
			}
		}
	}
	lprintf( WIDE( "Setup to play item in control is done... should be playing soon." ) );
	return pmyi;
}

struct my_vlc_interface * PlayItemIn( PSI_CONTROL pc, CTEXTSTR url_name )
{
	return PlayItemInEx( pc, url_name, NULL );
}

void StopItemIn( PSI_CONTROL pc )
{
	INDEX idx;
	struct my_vlc_interface *pmyi;
	LIST_FORALL( l.interfaces, idx, struct my_vlc_interface*, pmyi )
	{
		INDEX idx2;
		PSI_CONTROL pc_check;
		LIST_FORALL( pmyi->controls, idx2, PSI_CONTROL, pc_check )
		{
			if( pc_check == pc )
			{
				lprintf( WIDE( "Stopped item in %p" ), pc );
				SetLink( &pmyi->controls, idx2, NULL );
				break;
			}
		}

		if( pc_check )
			break;
	}

	if( pmyi )
	{
		int controls = 0;
		INDEX idx2;
		PSI_CONTROL pc_check;
		LIST_FORALL( pmyi->controls, idx2, PSI_CONTROL, pc_check )
		{
			controls++;
		}
		if( !controls )
		{
			StopItem( pmyi );
		}
	}
}


struct on_thread_params
{
	int done;
	int transparent;
	PRENDERER renderer;
	CTEXTSTR input;
	CTEXTSTR extra_opts;
	struct my_vlc_interface *pmyi;
};

PTRSZVAL CPROC PlayItemOnThread( PTHREAD thread )
{
	PTRSZVAL psv = GetThreadParam( thread );
	struct on_thread_params *parms = (struct on_thread_params*)psv;
	struct my_vlc_interface *pmyi;
	//TEXTCHAR *start, *end;

	TEXTCHAR buffer[566];
	GetCurrentPath( buffer, sizeof(buffer) );
	pmyi = FindInstance( parms->input );
	if( !pmyi )
	{
		pmyi = CreateInstanceOn( parms->renderer, parms->input, parms->transparent, parms->extra_opts );
		if( !vlc.libvlc_new )
			return 0;
		parms->pmyi = pmyi;
		if( parms->input && parms->input[0] )
		{
			if( StrStr( parms->input, WIDE( "://" ) ) )
				pmyi->m = vlc.libvlc_media_new_location (pmyi->inst, DupTextToChar( parms->input) PASS_EXCEPT_PARAM);
			else
				pmyi->m = vlc.libvlc_media_new_path (pmyi->inst, DupTextToChar( parms->input) PASS_EXCEPT_PARAM);
			RAISE( vlc, &pmyi->ex );
		}
		parms->done = 1;

		if( pmyi->m )
		{
			/* Create a media player playing environement */
			pmyi->mp = vlc.libvlc_media_player_new_from_media( pmyi->m PASS_EXCEPT_PARAM);
			RAISE( vlc, &pmyi->ex );


			pmyi->mpev = vlc.libvlc_media_player_event_manager( pmyi->mp PASS_EXCEPT_PARAM );
			//vlc.libvlc_event_attach( pmyi->mpev, libvlc_MediaPlayerEndReached, PlayerEvent, pmyi PASS_EXCEPT_PARAM );

			vlc.libvlc_media_release (pmyi->m);

			//WakeableSleep( 1000 );
			/* play the media_player */
			//if( pmyi->flags.bPlaying )
			{
				//lprintf( WIDE( "Instance play." ) );
				//lprintf( WIDE( "PLAY (no, delay to application)" ) );
				//vlc.libvlc_media_player_play( pmyi->mp PASS_EXCEPT_PARAM);
			}

			RAISE( vlc, &pmyi->ex );
		}
		else
		{
         //pmyi->mp = vlc.libvlc_media_player_new( pmyi->inst );
			//RAISE( vlc, &pmyi->ex );
			//pmyi->mpev = vlc.libvlc_media_player_event_manager( pmyi->mp PASS_EXCEPT_PARAM );
		}

		if( pmyi->mp )
		{
			lprintf( WIDE( "vlc 1.1.x set callbacks, get ready." ) );
			vlc.libvlc_video_set_callbacks( pmyi->mp, lock_frame, unlock_frame, display_frame, pmyi );
			lprintf( WIDE( "Output %d %d" ), pmyi->image_w, pmyi->image_h );
			vlc.libvlc_video_set_format(pmyi->mp, "RV32", pmyi->image_w, pmyi->image_h, -(signed)(pmyi->image_w*sizeof(CDATA)));

			BindAllEvents( pmyi );
		}
		pmyi->waiting = thread;
		pmyi->flags.bStarted = 1;

		while( !pmyi->flags.bDone )
		{
			//lprintf( WIDE( "Waiting for done..." ) );
			if( pmyi->m && pmyi->mp )
			{
				if( pmyi->flags.bWantStop )
				{
					//lprintf( WIDE( "STOP (catch up to application wanting play)" ) );
					vlc.libvlc_media_player_stop( pmyi->mp PASS_EXCEPT_PARAM);
					pmyi->flags.bWantStop = 0;
				}
				else if( pmyi->flags.bWantPlay )
				{
					//lprintf( WIDE( "PLAY (catch up to application wanting play)" ) );
					vlc.libvlc_media_player_play( pmyi->mp PASS_EXCEPT_PARAM);
					pmyi->flags.bWantPlay = 0;
				}
				else if( pmyi->flags.bPlaying && pmyi->flags.bWantPause )
				{
					//lprintf( WIDE( "PAUSE (catch up to application wanting play)" ) );
					vlc.libvlc_media_player_pause (pmyi->mp PASS_EXCEPT_PARAM);
					pmyi->flags.bWantPause = 0;
				}
				else
					WakeableSleep( 10000 );
			}
			else
				WakeableSleep( 10000 );
		}
		//lprintf( WIDE( "Vdieo ended... cleanup." ) );
		ReleaseInstance( pmyi );
	}
	else
	{
		lprintf( WIDE( "Device already open... adding renderer." ) );
		// probably another thread is already running this...
		AddLink( &pmyi->panels, parms->renderer );
		parms->pmyi = pmyi;
		parms->done = 1;
	}
	return (PTRSZVAL)pmyi;
}

struct my_vlc_interface * PlayItemOn( PRENDERER renderer, CTEXTSTR input )
{
	struct on_thread_params parms;
	parms.renderer = renderer;
	parms.input = input;
	parms.extra_opts = NULL;
	parms.transparent = 0;
	parms.done = 0;
	ThreadTo( PlayItemOnThread, (PTRSZVAL)&parms );
	while( !parms.done )
		Relinquish();
	return parms.pmyi;
}


struct my_vlc_interface * PlayItemOnExx( PRENDERER renderer, CTEXTSTR input, CTEXTSTR extra_opts, int transparent )
{
	struct on_thread_params parms;
	parms.renderer = renderer;
	parms.input = input;
	parms.extra_opts = extra_opts;
	parms.transparent = transparent;
	parms.done = 0;

	ThreadTo( PlayItemOnThread, (PTRSZVAL)&parms );
	while( !parms.done )
      Relinquish();
	return parms.pmyi;
}

struct my_vlc_interface * PlayItemOnEx( PRENDERER renderer, CTEXTSTR input, CTEXTSTR extra_opts )
{
	struct on_thread_params parms;
	parms.renderer = renderer;
	parms.input = input;
	parms.extra_opts = extra_opts;
	parms.transparent = 0;
	parms.done = 0;
	ThreadTo( PlayItemOnThread, (PTRSZVAL)&parms );
	while( !parms.done )
		Relinquish();
	return parms.pmyi;
}

PTRSZVAL CPROC PlayItemAtThread( PTHREAD thread )
{
	PTRSZVAL psv = GetThreadParam( thread );
	struct on_thread_params *parms = (struct on_thread_params*)psv;
	struct my_vlc_interface *pmyi;

	TEXTCHAR buffer[566];
	GetCurrentPath( buffer, sizeof(buffer) );
	pmyi = FindInstance( parms->input );
	if( !pmyi )
	{
		lprintf( WIDE( "Creating item http..." ) );
		pmyi = CreateInstanceAt( WIDE( ":1234" ), WIDE( "fake name" ), parms->input, parms->transparent );
		if( parms->input && parms->input[0] )
		{
#ifndef USE_PRE_1_1_0_VLC
			if( StrStr( parms->input, WIDE( "://" ) ) )
				pmyi->m = vlc.libvlc_media_new_location (pmyi->inst, DupTextToChar( parms->input ) PASS_EXCEPT_PARAM);
			else
				pmyi->m = vlc.libvlc_media_new_path (pmyi->inst, DupTextToChar( parms->input ) PASS_EXCEPT_PARAM);
#else
			pmyi->m = vlc.libvlc_media_new (pmyi->inst, parms->input PASS_EXCEPT_PARAM);
#endif
			RAISE( vlc, &pmyi->ex );
			parms->done = 1;

			/* Create a media player playing environement */
			pmyi->mp = vlc.libvlc_media_player_new_from_media( pmyi->m PASS_EXCEPT_PARAM);
			RAISE( vlc, &pmyi->ex );

#ifndef USE_PRE_1_1_0_VLC
			lprintf( WIDE( "vlc 1.1.x set callbacks, get ready." ) );
			vlc.libvlc_video_set_callbacks( pmyi->mp, lock_frame, unlock_frame, display_frame, pmyi );
			lprintf( WIDE( "Output %d %d" ), pmyi->image_w, pmyi->image_h );
			vlc.libvlc_video_set_format(pmyi->mp, "RV32", pmyi->image_w, pmyi->image_h, -(signed)(pmyi->image_w*sizeof(CDATA)));
#endif
			pmyi->mpev = vlc.libvlc_media_player_event_manager( pmyi->mp PASS_EXCEPT_PARAM );
		}
		else
		{
			pmyi->mpev = vlc.libvlc_vlm_get_event_manager( pmyi->inst );
		}

		parms->done = 1;

		BindAllEvents( pmyi );
		//vlc.libvlc_event_attach( pmyi->mpev, libvlc_MediaPlayerEndReached, PlayerEvent, pmyi PASS_EXCEPT_PARAM );

      if( pmyi->m )
			vlc.libvlc_media_release (pmyi->m);

		if( pmyi->mp )
		{
			lprintf( WIDE( "PLAY" ) );
			vlc.libvlc_media_player_play( pmyi->mp PASS_EXCEPT_PARAM);
		}

		RAISE( vlc, &pmyi->ex );
		pmyi->waiting = thread;
		parms->done = 1;
		while( !pmyi->flags.bDone )
		{
			lprintf( WIDE( "Waiting for done..." ) );
			WakeableSleep( 10000 );
		}
		lprintf( WIDE( "Vdieo ended... cleanup." ) );
		ReleaseInstance( pmyi );
	}
	else
	{
		parms->done = 1;
	}
	return 0;
}

struct my_vlc_interface * PlayItemAt( CTEXTSTR input )
{
	struct on_thread_params parms;
	parms.input = input;
	parms.extra_opts = NULL;
	parms.transparent = 0;
	parms.done = 0;
	ThreadTo( PlayItemAtThread, (PTRSZVAL)&parms );
	while( !parms.done )
		Relinquish();
	return NULL;
}


struct my_vlc_interface * PlayItemAtExx( CTEXTSTR input, CTEXTSTR extra_opts, int transparent )
{
	struct on_thread_params parms;
	parms.input = input;
	parms.extra_opts = extra_opts;
	parms.transparent = transparent;
	parms.done = 0;

	ThreadTo( PlayItemAtThread, (PTRSZVAL)&parms );
	while( !parms.done )
		Relinquish();
	return NULL;
}

struct my_vlc_interface * PlayItemAtEx( CTEXTSTR input, CTEXTSTR extra_opts )
{
	struct on_thread_params parms;
	parms.input = input;
	parms.extra_opts = extra_opts;
	parms.transparent = 0;
	parms.done = 0;
	ThreadTo( PlayItemAtThread, (PTRSZVAL)&parms );
	while( !parms.done )
		Relinquish();
	return NULL;
}

void PlayUsingMediaList( struct my_vlc_interface *pmyi, PLIST files )
{
	CTEXTSTR file_to_play;
	INDEX idx;
	pmyi->ml = vlc.libvlc_media_list_new( pmyi->inst PASS_EXCEPT_PARAM );
	RAISE( vlc, &pmyi->ex );

	LIST_FORALL( files, idx, CTEXTSTR, file_to_play )
	{
#ifndef USE_PRE_1_1_0_VLC
		if( StrStr( file_to_play, WIDE( "://" ) ) )
			pmyi->m = vlc.libvlc_media_new_location (pmyi->inst, DupTextToChar( file_to_play ) PASS_EXCEPT_PARAM);
		else
			pmyi->m = vlc.libvlc_media_new_path (pmyi->inst, DupTextToChar( file_to_play ) PASS_EXCEPT_PARAM);
#else
		pmyi->m = vlc.libvlc_media_new (pmyi->inst, file_to_play PASS_EXCEPT_PARAM);
#endif
		RAISE( vlc, &pmyi->ex );

		pmyi->mev = vlc.libvlc_media_event_manager( pmyi->m PASS_EXCEPT_PARAM );
		RAISE( vlc, &pmyi->ex );

		vlc.libvlc_media_list_add_media( pmyi->ml, pmyi->m PASS_EXCEPT_PARAM );
		RAISE( vlc, &pmyi->ex );

		vlc.libvlc_media_release (pmyi->m);
		RAISE( vlc, &pmyi->ex );
		pmyi->list_count++;
	}



	pmyi->mlp = vlc.libvlc_media_list_player_new( pmyi->inst PASS_EXCEPT_PARAM);
	RAISE( vlc, &pmyi->ex );

	/* Create a media player playing environement */
	pmyi->mp = vlc.libvlc_media_player_new( pmyi->inst PASS_EXCEPT_PARAM);
	RAISE( vlc, &pmyi->ex );

	vlc.libvlc_media_list_player_set_media_list( pmyi->mlp, pmyi->ml PASS_EXCEPT_PARAM );
	RAISE( vlc, &pmyi->ex );

	vlc.libvlc_media_list_player_set_media_player( pmyi->mlp, pmyi->mp PASS_EXCEPT_PARAM );
	RAISE( vlc, &pmyi->ex );


	vlc.libvlc_media_list_player_play_item_at_index( pmyi->mlp, pmyi->list_index PASS_EXCEPT_PARAM );
	RAISE( vlc, &pmyi->ex );
	pmyi->list_index++;
	if( pmyi->list_index == pmyi->list_count )
		pmyi->list_index = 0;


      //DebugBreak();
      //pmyi->mcex = vlc.mediacontrol_exception_create();
		//pmyi->mc = vlc.mediacontrol_new_from_instance( pmyi->inst, pmyi->mcex );
      //pmyi->si = vlc.mediacontrol_get_stream_information( pmyi->mc, 0, pmyi->mcex );

		//t_length = vlc.libvlc_media_player_get_length( pmyi->mp PASS_EXCEPT_PARAM );
      //lprintf( WIDE( "length is %ld" ), t_length );
	//vlc.raise( &pmyi->ex );
	pmyi->mpev = vlc.libvlc_media_player_event_manager( pmyi->mp PASS_EXCEPT_PARAM );

	vlc.libvlc_event_attach( pmyi->mpev, libvlc_MediaPlayerEndReached, PlayerEvent, pmyi PASS_EXCEPT_PARAM );

	//vlc.libvlc_media_list_player_next( pmyi->mlp PASS_EXCEPT_PARAM );
   //RAISE( vlc, &pmyi->ex );

	{
		int played = 0;
		//libvlc_time_t t_current;
		//libvlc_time_t t_length;
		do
		{
#if 0
         libvlc_state_t state = vlc.libvlc_media_list_player_get_state( pmyi->mlp PASS_EXCEPT_PARAM );
			lprintf( WIDE( "... %d" ), state );
#ifdef __WATCOMC__
			_try {
#endif
				t_current = vlc.libvlc_media_player_get_time( pmyi->mp PASS_EXCEPT_PARAM );
#ifdef __WATCOMC__
			}
			_except( EXCEPTION_EXECUTE_HANDLER )
			{
				lprintf( WIDE( "Caught exception in get_time" ) );
            return;
				;
			}
#endif

         lprintf( WIDE( "..." ) );
			RAISE( vlc, &pmyi->ex );
         lprintf( WIDE( "..." ) );
			if( t_current )
			{
            played = 1;
				//if( t_length == 0 )
				{
         lprintf( WIDE( "..." ) );
					t_length = vlc.libvlc_media_player_get_length( pmyi->mp PASS_EXCEPT_PARAM );
         lprintf( WIDE( "..." ) );
					RAISE( vlc, &pmyi->ex );
         lprintf( WIDE( "..." ) );
				}
				if( t_current > 15000  &&(  t_current < ( t_length - 15000 )) )
				{
#if defined( __WATCOMC__ )
#ifndef __cplusplus
						_try
						{
#endif
#endif
						vlc.libvlc_media_list_player_next( pmyi->mlp PASS_EXCEPT_PARAM );
#if defined( __WATCOMC__ )
#ifndef __cplusplus
					}
					_except( EXCEPTION_EXECUTE_HANDLER )
					{
						vlc.libvlc_media_list_player_play_item_at_index( pmyi->mlp, 0 PASS_EXCEPT_PARAM );
						//lprintf( WIDE( "Caught exception in video output window" ) );
						;
					}
#endif
#endif
					{

					}
					//vlc.libvlc_media_list_player_pause (pmyi->mlp PASS_EXCEPT_PARAM);
					RAISE( vlc, &pmyi->ex );
					//vlc.libvlc_media_player_set_time( pmyi->mp, t_length - 15000 PASS_EXCEPT_PARAM );
					//vlc.libvlc_media_list_player_play (pmyi->mlp PASS_EXCEPT_PARAM);
					//RAISE( vlc, &pmyi->ex );
				}

				//lprintf( WIDE( "now is %lld %lld" ), t_length, t_current );
			}
			else
				lprintf( WIDE( "current is 0..." ) );
			if( !t_current && played )
			{
				played = 0;
				//lprintf( WIDE( "..." ) );
				//vlc.libvlc_media_list_player_play_item_at_index( pmyi->mlp, index++ PASS_EXCEPT_PARAM );
				if( index == count )
					index = 0;
			}
			lprintf( WIDE( "..." ) );
#endif
			IdleFor( 250 );//+ ( t_length - t_current ) / 5 );
         lprintf( WIDE( "Tick..." ) );
		}
		while( 1  );//|| ( t_length - t_current ) > 50 );
	}
}


void SetPriority( _32 proirity_class )
		{
#ifdef _WIN32
			HANDLE hToken, hProcess;
			TOKEN_PRIVILEGES tp;
			OSVERSIONINFO osvi;
			DWORD dwPriority;
			osvi.dwOSVersionInfoSize = sizeof( osvi );
			GetVersionEx( &osvi );
			if( osvi.dwPlatformId  == VER_PLATFORM_WIN32_NT )
			{
				//b95 = FALSE;
				// allow shutdown priviledges....
				// wonder if /shutdown will work wihtout _this?
				if( DuplicateHandle( GetCurrentProcess(), GetCurrentProcess()
										 , GetCurrentProcess(), &hProcess, 0
										 , FALSE, DUPLICATE_SAME_ACCESS  ) )
					if( OpenProcessToken( hProcess, TOKEN_ADJUST_PRIVILEGES, &hToken ) )
					{
						tp.PrivilegeCount = 1;
						if( LookupPrivilegeValue( NULL
														, SE_SHUTDOWN_NAME
														, &tp.Privileges[0].Luid ) )
						{
							tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
							AdjustTokenPrivileges( hToken, FALSE, &tp, 0, NULL, NULL );
						}
						else
							GetLastError();
					}
					else
						GetLastError();
				else
					GetLastError();
				CloseHandle( hProcess );
				CloseHandle( hToken );

				dwPriority = proirity_class;
			}
			else
			{
				//b95 = TRUE;
				lprintf( WIDE( "platform failed." ) );
				dwPriority = NORMAL_PRIORITY_CLASS;
			}
			if( !SetThreadPriority( GetCurrentThread(), dwPriority ) )
			{
            lprintf( WIDE( "Priority filed!" ) );
			}
			//SetPriorityClass( GetCurrentProcess(), dwPriority );
#endif
		}


void PlayList( PLIST files, S_32 x, S_32 y, _32 w, _32 h )
{
	CTEXTSTR file_to_play;
   INDEX idx;
	PRENDERER transparent[2];
   int npmyi_to_play = 0; // which is the one that is waiting to play.
   int npmyi_playing = -1; // which is the one that is waiting to play.
	struct my_vlc_interface *ppmyi[2];
   struct my_vlc_interface *pmyi;

	//pmyi->list_count = 0;
	//pmyi->list_index = 0;
start:
	LIST_FORALL( files, idx, CTEXTSTR, file_to_play )
	{
		lprintf( WIDE( "waiting for available queue..." ) );
		while( npmyi_to_play == npmyi_playing )
			WakeableSleep( 1000 );
		lprintf( WIDE( "New display..." ) );
#ifdef _WIN32
		SetPriority( THREAD_PRIORITY_IDLE );
#endif
		transparent[npmyi_to_play] = OpenDisplaySizedAt( DISPLAY_ATTRIBUTE_LAYERED, w, h, x, y );
		DisableMouseOnIdle( transparent[npmyi_to_play], TRUE );

		UpdateDisplay( transparent[npmyi_to_play] );
		lprintf( WIDE( "New isntance..." ) );
		ppmyi[npmyi_to_play] = CreateInstanceOn( transparent[npmyi_to_play], file_to_play, 1, NULL );
		pmyi = ppmyi[npmyi_to_play];
		lprintf( WIDE( "New media..." ) );

#ifndef USE_PRE_1_1_0_VLC
		if( StrStr( file_to_play, WIDE( "://" ) ) )
			pmyi->m = vlc.libvlc_media_new_location (pmyi->inst, DupTextToChar( file_to_play ) PASS_EXCEPT_PARAM);
		else
			pmyi->m = vlc.libvlc_media_new_path (pmyi->inst, DupTextToChar( file_to_play ) PASS_EXCEPT_PARAM);
#else
		pmyi->m = vlc.libvlc_media_new (pmyi->inst, file_to_play PASS_EXCEPT_PARAM);
#endif
		RAISE( vlc, &ppmyi[npmyi_to_play]->ex);

		//lprintf( WIDE( "New media player from media..." ) );
		/* Create a media player playing environement */
		pmyi->mp = vlc.libvlc_media_player_new_from_media( pmyi->m PASS_EXCEPT_PARAM);
		RAISE( vlc, &pmyi->ex );
#ifndef USE_PRE_1_1_0_VLC
		vlc.libvlc_video_set_callbacks( pmyi->mp, lock_frame, unlock_frame, display_frame, pmyi );
		vlc.libvlc_video_set_format(pmyi->mp, "RV32", pmyi->image_w, pmyi->image_h, pmyi->image_w*sizeof(CDATA));
#endif
		//lprintf( WIDE( "Release media..." ) );
		vlc.libvlc_media_release (pmyi->m);

		//lprintf( WIDE( "New event manager..." ) );
		pmyi->mpev = vlc.libvlc_media_player_event_manager( pmyi->mp PASS_EXCEPT_PARAM );
		vlc.libvlc_event_attach( pmyi->mpev, libvlc_MediaPlayerEndReached, PlayerEvent, pmyi PASS_EXCEPT_PARAM );
		RAISE( vlc, &pmyi->ex );


#ifdef _WIN32
		SetPriority( THREAD_PRIORITY_NORMAL );
#endif
		if( npmyi_playing >= 0 )
		{
			pmyi->waiting = MakeThread();
			while( !pmyi->flags.bDone )
			{
				//lprintf( WIDE( "Sleeping until video is done." ) );
				WakeableSleep( 1000 );
			}
		}
		pmyi->flags.bDone = FALSE;
		pmyi->flags.bPlaying = FALSE; // set on first lock of buffer.
		//lprintf( WIDE( "------- BEGIN PLAY ----------- " ) );
		vlc.libvlc_media_player_play (pmyi->mp PASS_EXCEPT_PARAM);
		if( npmyi_playing >= 0 )
		{
			ReleaseInstance( pmyi );
		}
		/* play the media_player */
		RAISE( vlc, &pmyi->ex );

		npmyi_playing = npmyi_to_play;
		npmyi_to_play++;
		if( npmyi_to_play == 2 )
         npmyi_to_play = 0;
	}
   goto start;

	// PlayUsingMediaList( pmyi, files );

}


struct on_sound_thread_params
{
	int done;
	PRENDERER renderer;
	CTEXTSTR input;
};

PTRSZVAL CPROC PlaySoundItemOnThread( PTHREAD thread )
{
	PTRSZVAL psv = GetThreadParam( thread );
	struct on_sound_thread_params *parms = (struct on_sound_thread_params*)psv;
	struct my_vlc_interface *pmyi;
	pmyi = CreateInstance( parms->input );

#ifndef USE_PRE_1_1_0_VLC
	if( StrStr( parms->input, WIDE( "://" ) ) )
		pmyi->m = vlc.libvlc_media_new_location (pmyi->inst, DupTextToChar( parms->input) PASS_EXCEPT_PARAM);
	else
		pmyi->m = vlc.libvlc_media_new_path (pmyi->inst,DupTextToChar( parms->input)PASS_EXCEPT_PARAM);
#else
	pmyi->m = vlc.libvlc_media_new (pmyi->inst, parms->input PASS_EXCEPT_PARAM);
#endif
	RAISE( vlc, &pmyi->ex );

	parms->done = 1;
	Relinquish();

	/* Create a media player playing environement */
	pmyi->mp = vlc.libvlc_media_player_new_from_media( pmyi->m PASS_EXCEPT_PARAM);
	RAISE( vlc, &pmyi->ex );

#ifndef USE_PRE_1_1_0_VLC
	vlc.libvlc_video_set_callbacks( pmyi->mp, lock_frame, unlock_frame, display_frame, pmyi );
	vlc.libvlc_video_set_format(pmyi->mp, "RV32", pmyi->image_w, pmyi->image_h, pmyi->image_w*sizeof(CDATA));
#endif
	pmyi->mpev = vlc.libvlc_media_player_event_manager( pmyi->mp PASS_EXCEPT_PARAM );
   // hrm... sound files don't know mediaendreaached
   //vlc.libvlc_event_attach( pmyi->mpev, libvlc_MediaPlayerEndReached, PlayerEvent, pmyi PASS_EXCEPT_PARAM );
   vlc.libvlc_event_attach( pmyi->mpev, libvlc_MediaPlayerStopped, PlayerEvent, pmyi PASS_EXCEPT_PARAM );
	vlc.libvlc_media_release (pmyi->m);

	/* play the media_player */
	lprintf( WIDE("play"));
    vlc.libvlc_media_player_play (pmyi->mp PASS_EXCEPT_PARAM);
    RAISE( vlc, &pmyi->ex );

	 pmyi->waiting = thread;
	 while( !pmyi->flags.bDone )
	 {
       lprintf( WIDE( "Sleeping until video is done." ) );
		 WakeableSleep( 10000 );
	 }
    lprintf( WIDE( "Vdieo ended... cleanup." ) );
	 ReleaseInstance( pmyi );
    return 0;
}



void PlaySoundFile( CTEXTSTR file )
{
	struct on_thread_params parms;
	parms.renderer = NULL;
	parms.transparent = 0;
	parms.input = file;
	parms.done = 0;
	ThreadTo( PlaySoundItemOnThread, (PTRSZVAL)&parms );
   // wait for the parameters to be read...
	while( !parms.done )
      Relinquish();

}

void PlaySoundData( POINTER data, size_t length )
{

}

void HoldUpdates( void )
{
   l.flags.bHoldUpdates = 1;
	while( l.flags.bUpdating )
		Relinquish();
   return;
}

void ReleaseUpdates( void )
{
   // next frame update will wake the updating thread...and it will be able to process agian.
   l.flags.bHoldUpdates = 0;
}

