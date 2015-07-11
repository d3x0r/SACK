#include <stdhdrs.h>

#include <ffmpeg_interface.h>

struct compressed_phrase
{
	int phrase_len;
	_8 *phrase;
};

static struct {
	struct audio_device *output;
	PLINKQUEUE compressed_queue;
} test_local;

static void CPROC callback( PTRSZVAL psv, int max_level, POINTER data, size_t datalen )
{
	struct compressed_phrase *phrase = New( struct compressed_phrase );
	phrase->phrase_len = datalen;
	phrase->phrase = NewArray( _8, datalen );
	MemCpy( phrase->phrase, data, datalen );
	EnqueLink( &test_local.compressed_queue, phrase );
}

static PTRSZVAL CPROC OutputThread( PTHREAD thread )
{
	// delay 5 seconds.
	//WakeableSleep( 5000 );
	while( 1 )
	{
		struct compressed_phrase *phrase = (struct compressed_phrase *)DequeLink( &test_local.compressed_queue );
		if( phrase )
		{
			audio_PlaybackBuffer( test_local.output, phrase->phrase, phrase->phrase_len );
		}
		else
		{
			WakeableSleep( 50 );
		}
	}
}

SaneWinMain( argc, argv )
{
	PLIST list = NULL;
	INDEX idx;
	CTEXTSTR device;
	printf( "--------output devices----------\n" );
			audio_GetPlaybackDevices( &list );
			LIST_FORALL( list, idx, CTEXTSTR, device )
			{
				printf( "%d - %s\n", idx, device );
			}
			test_local.output = audio_OpenPlaybackDevice( (CTEXTSTR)GetLink( &list, 0 ) );
   printf( "--------input devices----------\n" );
	audio_GetCaptureDevices( &list );
	LIST_FORALL( list, idx, CTEXTSTR, device )
	{
		printf( "%d - %s\n", idx, device );
	}
	{
		struct audio_device *dev = audio_OpenCaptureDevice( (CTEXTSTR)GetLink( &list, 0 ), callback, 0 );
		if( dev )
		{
			ThreadTo( OutputThread, 0 );
			while( 1 )
				WakeableSleep( 10000 );
		}
	}
	return 0;
}
EndSaneWinMain()