
#include <render.h>
#include <controls.h>

#ifdef FFMPEG_INTERFACE_SOURCE
#define FFMPEG_INTERFACE_EXPORT EXPORT_METHOD
#else
/* Defines how external functions are referenced
   (dllimport/export/extern)                     */
#define FFMPEG_INTERFACE_EXPORT IMPORT_METHOD
#endif
/* The API type of FFMPEG_INTERFACE functions - default to CPROC. */
#define FFMPEG_INTERFACEAPI CPROC

#ifdef __cplusplus
/* A symbol to define the sub-namespace of FFMPEG_INTERFACE_NAMESPACE  */
#define _FFMPEG_INTERFACE_NAMESPACE namespace streamlib { namespace ffmpeg {
/* A macro to end just the FFMPEG_INTERFACE sub namespace. */
#define _FFMPEG_INTERFACE_NAMESPACE_END } }
#else
#define _FFMPEG_INTERFACE_NAMESPACE 
#define _FFMPEG_INTERFACE_NAMESPACE_END
#endif
/* FFMPEG_INTERFACE full namespace  */
#define FFMPEG_INTERFACE_NAMESPACE TEXT_NAMESPACE _FFMPEG_INTERFACE_NAMESPACE
/* Macro to use to define where http utility namespace ends. */
#define FFMPEG_INTERFACE_NAMESPACE_END _FFMPEG_INTERFACE_NAMESPACE_END TEXT_NAMESPACE_END


SACK_NAMESPACE
   _FFMPEG_INTERFACE_NAMESPACE


struct ffmpeg_file;

// Return a file; return NULL if the file is not understood
// Renderer callback will be called once the size of the video frame is known.
FFMPEG_INTERFACE_EXPORT struct ffmpeg_file * FFMPEG_INTERFACEAPI ffmpeg_LoadFile( CTEXTSTR filename
                                                                                , PRENDERER (CPROC*getDisplay)(PTRSZVAL psv,_32 w, _32 h), PTRSZVAL psvGetDisplay 
                                                                                , PSI_CONTROL output_control
                                                                                , void (CPROC*video_position_update)( PTRSZVAL psv, _64 tick ), PTRSZVAL psvUpdate
                                                                                , void (CPROC*video_ended)( PTRSZVAL psv ), PTRSZVAL psvEnded
                                                                                , void (CPROC*play_error)( CTEXTSTR message )
                                                                                );
// specify additional routines for pre- and post- rendering to a control.
// pre_render is called before the video frame is written to a control
// post_render is caled after the video frame is written to a control and before update to the screen.
FFMPEG_INTERFACE_EXPORT void FFMPEG_INTERFACEAPI ffmpeg_SetPrePostDraw( struct ffmpeg_file *file, void (*pre_render)(PSI_CONTROL), void (*post_render)(PSI_CONTROL) );

// call this if the control passed to render a file into has changed size.  Updates internal scaling.
FFMPEG_INTERFACE_EXPORT void FFMPEG_INTERFACEAPI ffmpeg_UpdateControlLayout( struct ffmpeg_file *file );

// Releases all resources associated with the player except the renderer.
// The renderer should be kept by the caller until after this is unloaded, in case 
// one last frame goes out.
FFMPEG_INTERFACE_EXPORT void                 FFMPEG_INTERFACEAPI ffmpeg_UnloadFile( struct ffmpeg_file *file );

//  ffmpeg_PlayFile( file ); 
// Starts a file that has previously been loaded.
// the player does not show the renderer (if restore is required)
// if the player has been paused, this will also resume play
// returns asynch to player playing/resuming
FFMPEG_INTERFACE_EXPORT void FFMPEG_INTERFACEAPI ffmpeg_PlayFile( struct ffmpeg_file *file );

//  ffmpeg_StopFile( file );
// Sets the file to the end-condition; as if the file reached end of file...
// this triggers normal media-ended callbacks
FFMPEG_INTERFACE_EXPORT void FFMPEG_INTERFACEAPI ffmpeg_StopFile( struct ffmpeg_file *file );

// ffmpeg_SetPositionUpdateCallback( file, callback, callback_user_value );
// setup the internal half second update routine to point to this routine...
// used in case this routine is not avaiable, or known, at media load time
FFMPEG_INTERFACE_EXPORT void FFMPEG_INTERFACEAPI ffmpeg_SetPositionUpdateCallback( struct ffmpeg_file *file, void (CPROC*video_position_update)( PTRSZVAL psv, _64 tick ), PTRSZVAL psvUpdate );

// pauses a file that is playing
// returns asynch to player pausing; it's more of a suggestion, but will be respected at the next
// packet process.
FFMPEG_INTERFACE_EXPORT void FFMPEG_INTERFACEAPI ffmpeg_PauseFile( struct ffmpeg_file *file );

// seeks the position of the file
// This is passed a value between 0-1; expressed as an epic of 
//  1 - 1 000 000 000 
// which gives millisecond positioning for files under 11 days 13 hours in length
// (the next range down would have been millisecond range for files 16:40 minutes in length)
// per-epic  (percent)
FFMPEG_INTERFACE_EXPORT void FFMPEG_INTERFACEAPI ffmpeg_SeekFile( struct ffmpeg_file *file, S_64 perepic );

// adjust video offset... provides ability to adjust tracking externally.
// frames is the number of frames to delay or retard the video stream.
FFMPEG_INTERFACE_EXPORT void FFMPEG_INTERFACEAPI ffmpeg_AdjustVideo( struct ffmpeg_file *file, int frames );

// Get the list of available audio devices.
// PLIST list = NULL; // make sure to init list to NULL.
// audio_GetDevices( &list );
//  list is filled with CTEXTSTR that are available device names.
FFMPEG_INTERFACE_EXPORT void FFMPEG_INTERFACEAPI audio_GetCaptureDevices( PLIST *ppList );
FFMPEG_INTERFACE_EXPORT void FFMPEG_INTERFACEAPI audio_GetPlaybackDevices( PLIST *ppList );

FFMPEG_INTERFACE_EXPORT struct audio_device * FFMPEG_INTERFACEAPI audio_OpenCaptureDevice( CTEXTSTR devname, void (CPROC*callback)( PTRSZVAL psvInst, POINTER data, size_t ), PTRSZVAL psvInst );
FFMPEG_INTERFACE_EXPORT struct audio_device * FFMPEG_INTERFACEAPI audio_OpenPlaybackDevice( CTEXTSTR devname );
FFMPEG_INTERFACE_EXPORT void FFMPEG_INTERFACEAPI audio_PlaybackBuffer( struct audio_device *ad, POINTER data, size_t samples );

FFMPEG_INTERFACE_EXPORT void FFMPEG_INTERFACEAPI audio_SuspendCapture( struct audio_device *device );
FFMPEG_INTERFACE_EXPORT void FFMPEG_INTERFACEAPI audio_ResumeCapture( struct audio_device *device );


_FFMPEG_INTERFACE_NAMESPACE_END
SACK_NAMESPACE_END
#ifdef __cplusplus
	using namespace sack::streamlib::ffmpeg;
#endif
