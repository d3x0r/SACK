
#ifdef VLC_INTERFACE_SOURCE
#define XTRN EXPORT_METHOD
#else
#define XTRN IMPORT_METHOD
#endif

#include <controls.h>

XTRN struct my_vlc_interface *PlayItemAgainst( Image image, CTEXTSTR opts );
XTRN void PlayItem( struct my_vlc_interface * );
XTRN void StopItem( struct my_vlc_interface *);
XTRN void SetStopEvent( struct my_vlc_interface *, void (CPROC*StopEvent)( PTRSZVAL psv ), PTRSZVAL psv );

// target is a PSI_CONTROL
XTRN struct my_vlc_interface *PlayItemIn( PSI_CONTROL pc, CTEXTSTR input );
XTRN struct my_vlc_interface * PlayItemInEx( PSI_CONTROL pc, CTEXTSTR input, CTEXTSTR extra_opts );

// target is a renderer
XTRN struct my_vlc_interface *PlayItemOn( PRENDERER renderer, CTEXTSTR input );
XTRN struct my_vlc_interface * PlayItemOnEx( PRENDERER renderer, CTEXTSTR input, CTEXTSTR extra_opts );
XTRN struct my_vlc_interface * PlayItemOnExx( PRENDERER renderer, CTEXTSTR input, CTEXTSTR extra_opts, int transparent );

// target is a network stream - no surface required
XTRN struct my_vlc_interface *PlayItemAt( CTEXTSTR input );
XTRN struct my_vlc_interface * PlayItemAtEx( CTEXTSTR input, CTEXTSTR extra_opts );
XTRN struct my_vlc_interface * PlayItemAtExx( CTEXTSTR input, CTEXTSTR extra_opts, int transparent );

// this can be done for shutdown purposes...
XTRN void HoldUpdates( void );
XTRN void ReleaseUpdates( void );

XTRN void PlayList( PLIST files, S_32 x, S_32 y, _32 w, _32 h );
XTRN void PlaySoundFile( CTEXTSTR file );

XTRN void StopItemIn( PSI_CONTROL pc );
