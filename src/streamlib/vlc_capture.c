
#define DEFINE_DEFAULT_RENDER_INTERFACE
#define USE_IMAGE_INTERFACE GetImageInterface()
#include <stdhdrs.h>
#include <psi.h>
#include <sqlgetoption.h>
#include "../InterShell/vlc_hook/vlcint.h"


#ifdef __LINUX__
#define INVALID_HANDLE_VALUE -1
#endif

//--------------------------------------------------------------------

typedef struct MyControl *PMY_CONTROL;
typedef struct MyControl MY_CONTROL;
struct MyControl
{
	PRENDERER surface;
	struct {
		BIT_FIELD bShown : 1;
	} flags;
};

EasyRegisterControl( WIDE("Video Control"), sizeof( MY_CONTROL) );

static int OnDrawCommon( WIDE("Video Control") )( PSI_CONTROL pc )
{
	return 1;
}

static int OnCreateCommon( WIDE("Video Control") )( PSI_CONTROL pc )
{
	MyValidatedControlData( PMY_CONTROL, control, pc );
	TEXTCHAR device[512];
	SACK_GetPrivateProfileString( WIDE("Video Stream"), WIDE("VLC Device"), WIDE("dshow://")
										 , device, sizeof( device ), WIDE("video.ini") );
   /*
	SACK_GetPrivateProfileString( WIDE("Video Stream"), WIDE("VLC Device Opts"),
										" :dshow-vdev=\"Hauppauge WinTV Capture\""
										" :dshow-adev=\"none\""
										" :dshow-size=\"\""
										" :dshow-caching=200"
										" :dshow-chroma=\"\""
										" :dshow-fps=0.000000"
										" :no-dshow-config"
										" :no-dshow-tuner"
										" :dshow-tuner-channel=0"
										" :dshow-tuner-country=0"
										" :dshow-tuner-input=0"
										" :dshow-video-input=1"
										" :dshow-audio-input=-1"
										" :dshow-video-output=-1"
										" :dshow-audio-output=-1"
										 , device_opts, sizeof( device_opts ), "video.ini" );
   */
	{
		int32_t x = 0;
		int32_t y = 0;
		//Image surface = GetControlSurface( pc );
		//GetPhysicalCoordinate( pc, &x, &y, FALSE );
 		//control->surface = OpenDisplayAboveSizedAt( DISPLAY_ATTRIBUTE_LAYERED|DISPLAY_ATTRIBUTE_CHILD|DISPLAY_ATTRIBUTE_NO_MOUSE
		//														, surface->width, surface->height, x, y
		//                                           , GetFrameRenderer( GetFrame( pc ) )
		 //  									  );
	}
	//PlayItemOnEx( control->surface, device, NULL /*device_opts*/ );
	SetCommonTransparent( pc, FALSE );
	PlayItemInEx( pc, device, NULL /*device_opts*/ );
	return TRUE;
}

PUBLIC( void, link_vlc_stream )( void )
{
}



static void OnHideCommon( WIDE("Video Control") )( PSI_CONTROL pc )
{
	MyValidatedControlData( PMY_CONTROL, control, pc );
	if( control )
	{
		lprintf( WIDE("hiding render display") );
		HideDisplay( control->surface );
	}
}

static void OnRevealCommon( WIDE("Video Control") )( PSI_CONTROL pc )
{
	MyValidatedControlData( PMY_CONTROL, control, pc );
	if( control )
	{
		if( control->flags.bShown )
		{
			lprintf( WIDE("restoring") );
			RestoreDisplay( control->surface );
		}
		else
		{
			lprintf( WIDE("first-showing") );
			UpdateDisplay( control->surface );
			control->flags.bShown = 1;
		}
	}

}



