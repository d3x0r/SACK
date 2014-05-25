//Requires a GUI environment, so if you are building with
//__NO_GUI__=1 this will never work.

#include <stdhdrs.h>
#include <network.h>
#include <configscript.h>
#include <sqlgetoption.h>
#include <stdio.h>
#include <stdlib.h>
#include <systray.h>
#include <salty_generator.h>
#define USE_IMAGE_INTERFACE g.pii
#define USE_RENDER_INTERFACE g.pdi
#include <render.h>
#include <image.h>
#include <timers.h>
#include <mt19937ar.h>
#include "vlcint.h"

struct video_player {
	PRENDERER surface;
	struct my_vlc_interface *vlc;
	_32 fade_in_time;
	int playing;  // if stopped, don't re-stop
	int paused;
};

typedef struct video_element {
	int type;
	struct video_player *player;
	Image image;
	_32 fade_in;
	_32 display_time; // how long to next image
};

typedef struct video_sequence {
	int sequence;

	int nImages; // convenience counter of the length of images.
	PLIST images;  // struct video_element *
	//PLIST image_type;
	int current_image; // overall counter, stepped appropriately.
	int prize_count;
};

typedef struct global_tag {
	PIMAGE_INTERFACE pii;
	PRENDER_INTERFACE pdi;
	struct
	{
		BIT_FIELD bBlacking : 1;
	}flags;

	_32 tick_to_switch;

	int default_show_time; // how long to show the iamge after fading in.
	int fade_in; // how long fade out is..
	int default_fade_in;
	int next_fade_in;

	// each display has a current image...
	int next_up; // index of the display that is next to show
	int currents[2];
	int is_up[2];
	PRENDERER displays[2];
	struct video_sequence *current_list[2];

	_32 target_in;
	_32 target_in_start;  //

	PRENDERER prior_up;
	struct video_element *prior_video;
	PLIST video_displays;

	PLIST playlists;  // struct video_sequence *
	int nPlayerLists;
	int nTotalImages;

	int x;
	int y;
	int w, h;

	PCLIENT host_socket;

	_32 next_chain;
	_32 want_next_chain;
	_32 last_received;

	LOGICAL attract_mode;
	_32 number_collector;
	_32 value_collector[64];
	int value_collect_index;
	int begin_card;

	int prize_total;
	struct mersenne_rng *rng;

	_32 _b;
} GLOBAL;
static GLOBAL g;


void CPROC Output( PTRSZVAL psv, PRENDERER display )
{
	if( g.is_up[psv] )
	{
		Image surface = GetDisplayImage( display );
		Image imgGraphic;
		struct video_element *element = (struct video_element*)GetLink( &g.current_list[psv]->images, g.currents[psv] );
		//lprintf( WIDE("Current on %d is %d"), psv, g.currents[psv] );
		imgGraphic = element->image;
		BlotScaledImage( surface, imgGraphic );
	}
}

struct video_sequence *GetSequenceEx( int ID, LOGICAL bCreate )
#define GetSequence(ID) GetSequenceEx( ID, TRUE )
{
	struct video_sequence *seq;
	INDEX idx;
	LIST_FORALL( g.playlists, idx, struct video_sequence *, seq )
	{
		if( seq->sequence == ID )
			return seq;
	}
	if( bCreate )
	{
		seq = New( struct video_sequence );
		MemSet( seq, 0, sizeof( struct video_sequence ) );
		seq->sequence = ID;
		g.nPlayerLists++;
		AddLink( &g.playlists, seq );
	}
	else
		seq = NULL;
	return seq;
}

void BeginNewChain( _32 tick )
{
	// if I'm in a transition, don't do this, otherwise we're not in
	// a transition, and can now start a new transition.
	if( !g.target_in )
	{
		struct video_sequence *seq = GetSequence( g.next_chain - 1 );
		struct video_element *element;
		if( seq->nImages )
		{
			//lprintf( "Begin New Chain.... %d", seq->sequence );
			g.current_list[g.next_up] = seq;
			g.target_in_start = tick;
			g.target_in = 0;  // clear this, so we start a new fade.
			seq->current_image = 0;  // reset current counter on sequence.
			if( seq->nImages )
			{
				element = (struct video_element *)GetLink( &seq->images, seq->current_image );
				g.fade_in = element->fade_in?element->fade_in:g.default_fade_in;
			}
		}
		else
		{
			// there is no next image... 
			//g.target_in_start = 0;
		}

		// acknowledge beginning a new sequence.
		g.next_chain = 0;
	}
}

void BeginFadeIn( _32 tick_start, _32 tick )
{
	LOGICAL play_and_hold;
	struct video_sequence *current_seq = g.current_list[g.next_up];
	struct video_element *current_element = (struct video_element *)GetLink( &current_seq->images, current_seq->current_image );
	PRENDERER r;
	play_and_hold = ( current_seq->current_image == 0 ) 
		&& (current_seq->sequence ) 
		&& (current_seq->sequence < ( g.nPlayerLists - 1 ) );
	//lprintf( WIDE("Begin fading in the new image... %d  %d  %d"), current_seq->sequence, current_seq->current_image, play_and_hold );
	if( current_seq->sequence == 0 )
		g.attract_mode = 1;

	g.currents[g.next_up] = current_seq->current_image;
	
	g.target_in = g.target_in_start + (current_element->fade_in?current_element->fade_in:g.default_fade_in);

	if( current_element->type == 1 )
	{
		r = g.displays[g.next_up];
		g.is_up[g.next_up] = 1;
	}
	else
	{
		struct video_player *player = current_element->player;
		r = player->surface;
		PlayItem( player->vlc ); // start the video clip now... allow fadein.
		player->playing = 1;
		if( play_and_hold )
		{
			player->paused = 1;
			PauseItem( player->vlc );
		}
	}

	//lprintf( WIDE("Begin Showing %p"), r );
	//lprintf( WIDE("(!)Setting fade to..> %d"), 255 - (255*(tick-g.target_in_start))/(g.target_in-g.target_in_start) );
	SetDisplayFade( r, 255 - (255*(tick-g.target_in_start))/(g.target_in-g.target_in_start) );
	RestoreDisplay( r );

}

void ContinueFadeIn( _32 tick )
{
	//lprintf( "something %d %d %d", g.target_in, tick, g.target_in - tick );
	if( g.target_in && tick > g.target_in )
	{
		struct video_sequence *current_seq = g.current_list[g.next_up];
		struct video_element *current_element = (struct video_element *)GetLink( &current_seq->images, current_seq->current_image );
		// completed the fade, finish, set total opaque.
		int upd = 0;
		PRENDERER r;
		if( current_element->type == 1 )
		{
			upd = 1;
			r = g.displays[g.next_up];
			g.next_up++;
			if( g.next_up >= 2 ) // two displayss
				g.next_up = 0;

			g.is_up[g.next_up] = 0;
			g.target_in_start = tick + ( current_element->display_time?current_element->display_time:g.default_show_time );
		}
		else
		{
			r = current_element->player->surface;
			// wait until stop event to set when next fadin starts.
			g.target_in_start = current_element->display_time ? (tick+current_element->display_time) : 0;
		}
		//lprintf( WIDE("Fade is is complete, set alpha to 0 (opaque), and hide old display...%d"), current_seq->sequence );
		if( g.prior_video && ( g.prior_video->type == 2 ) )
		{
			if( g.prior_video->player->playing )
			{
				g.prior_video->player->playing = 0;
				StopItem( g.prior_video->player->vlc );
			}
		}
		SetDisplayFade( r, 0 );
		if( upd )
			UpdateDisplay( r );

		if( g.prior_up )
			HideDisplay( g.prior_up );

		g.prior_up = r;
		g.prior_video = current_element;

		current_seq->current_image++;
		//lprintf( WIDE("Image fully up - setup to show next image... %d,%d"), current_seq->current_image, current_seq->nImages );
		if( current_seq->current_image >= current_seq->nImages )
		{
			//lprintf( "out of images on this list... reset to attract" );
			current_seq->current_image = 0;
			if( current_seq->sequence )
			{
				//lprintf( "next is attract..." );
				// next up is chain 0. (plus 1 to inidicate chain change; 0 is no change)
				g.want_next_chain = 0 + 1;
				g.want_next_chain = g.nPlayerLists;
				//lprintf( "Setting nexup to getlink 0" );
				g.current_list[g.next_up] = (struct video_sequence*)GetLink( &g.playlists, 0 );
			}
			else
			{
				//lprintf( "next is still this... %d", current_seq->sequence );
				g.current_list[g.next_up] = current_seq;
			}

		}
		else
		{
			//lprintf( "next is  still this? %d", current_seq->sequence );
			g.current_list[g.next_up] = current_seq;
		}
		g.target_in = 0;
	}
	else
	{
		struct video_sequence *current_seq = g.current_list[g.next_up];
		struct video_element *current_element = (struct video_element *)GetLink( &current_seq->images, current_seq->current_image );
		int upd = 0;
		PRENDERER r;
		if( current_element->type == 1 )
		{
			upd = 1;
			r = g.displays[g.next_up];
		}
		else
		{
			struct video_player *player = current_element->player;
			r = player->surface;
		}
		//lprintf( WIDE("Setting fade to..> %d"), 255 - (255*(tick-g.target_in_start))/(g.target_in-g.target_in_start) );
		//lprintf( WIDE("Increasing %p"), r );
		SetDisplayFade( r, 255 - (255*(tick-g.target_in_start))/(g.target_in-g.target_in_start) );
		if( upd )
			UpdateDisplay( r );
	}
}

void CPROC tick( PTRSZVAL psv )
{

	_32 now = timeGetTime();

	if( g.next_chain )
	{
		BeginNewChain( now );
	}

	if( g.target_in_start )
	{
		if( now >= g.target_in_start )
		{
			if( !g.target_in )
			{
				//lprintf(" begin fade.." );
				BeginFadeIn( g.target_in_start, now );
			}
			else
			{
				//lprintf(" continue fade.." );
				ContinueFadeIn( now );
			}
		}
	}
}

static void CPROC OnStopFade( PTRSZVAL psv )
{
	struct video_player *player = (struct video_player*)psv;
	player->playing = 0;
	if( g.want_next_chain )
	{
		g.next_chain = g.want_next_chain;
		g.want_next_chain = 0;
	}
	//lprintf( "Video ended; begin chain: %d", g.next_chain );
	g.target_in_start = GetTickCount(); // tick now.
	StopItem( player->vlc );
}

static int CPROC MouseMethod( PTRSZVAL psvMouse, S_32 x, S_32 y , _32 b )
{
	struct video_player *player = (struct video_player *)psvMouse;
	if( player )
	{
		if( player->paused && MAKE_FIRSTBUTTON( b, g._b ) )
			PlayItem( player->vlc );
	}
	g._b = b;
	return 1;
}

static struct video_element *LoadVideo( struct video_sequence *seq, CTEXTSTR file )
{
	struct video_player *player = New( struct video_player );
	player->paused = 0;
	player->fade_in_time = g.next_fade_in?g.next_fade_in:g.fade_in;
	g.next_fade_in = 0;
	player->surface = OpenDisplaySizedAt( DISPLAY_ATTRIBUTE_LAYERED|DISPLAY_ATTRIBUTE_CHILD|/*DISPLAY_ATTRIBUTE_NO_MOUSE|*/DISPLAY_ATTRIBUTE_NO_AUTO_FOCUS
													, g.w //width
													, g.h //height
													, g.x //0
													, g.y //0
													);
	SetMouseHandler( player->surface, MouseMethod, (PTRSZVAL)player );
	player->vlc = PlayItemOnEx( player->surface, file, WIDE("--repeat --loop") );
	SetStopEvent( player->vlc, OnStopFade, (PTRSZVAL)player );

	// only used if there is only 1 video - just show video.
	AddLink( &g.video_displays, player );

	{
		struct video_element *element = New( struct video_element );
		element->image = NULL;
		element->player = player;
		element->type = 2;
		element->display_time = 0;
		AddLink( &seq->images, element );
		g.nTotalImages++;
		seq->nImages++;
		return element;
	}
}


static PTRSZVAL CPROC AddVideo( PTRSZVAL psv, arg_list args )
{
	PARAM( args, S_64, list_id );
	PARAM( args, S_64, fade_in );
	PARAM( args, S_64, play_length );
	PARAM( args, CTEXTSTR, filename );
	struct video_sequence *seq = GetSequence( (int)list_id );
	//lprintf( WIDE( "adding video %s" ), filename );
	g.next_fade_in = (_32)play_length;

	if( StrCaseStr( filename, WIDE(".jpg") ) ||
		StrCaseStr( filename, WIDE(".jpeg") ) ||
		StrCaseStr( filename, WIDE(".bmp") ) ||
		StrCaseStr( filename, WIDE(".png") ) ||
		StrCaseStr( filename, WIDE(".tga") ) )
	{
		Image img = LoadImageFile( filename );
		if( img )
		{
			struct video_element *element = New( struct video_element );
			element->image = img;
			element->player = NULL;
			element->type = 1;

			element->display_time = (_32)play_length;
			element->fade_in = (_32)fade_in;

			AddLink( &seq->images, element );
			g.nTotalImages++;
			seq->nImages++;
		}
	}
	else
	{
		struct video_element *element = LoadVideo( seq, filename );

		element->display_time = (_32)play_length;
		element->fade_in = (_32)fade_in;
	}
	return psv;
}

static PTRSZVAL CPROC SetDefaultShowTime( PTRSZVAL psv, arg_list args )
{
	PARAM( args, S_64, delay );
	g.default_show_time = (int)delay;
	return psv;
}

static PTRSZVAL CPROC SetDefaultFadeInTime( PTRSZVAL psv, arg_list args )
{
	PARAM( args, S_64, delay );
	g.default_fade_in = (int)delay;
	return psv;
}

static PTRSZVAL CPROC SetDefaultPrizeCount( PTRSZVAL psv, arg_list args )
{
	PARAM( args, S_64, index );
	PARAM( args, S_64, count );
	TEXTCHAR value[32];
	struct video_sequence *seq = GetSequence( (int)index );
	tnprintf( value, 32, "%d", (int)index );
	seq->prize_count = SACK_GetProfileInt( "Prize Counts", value, count );
	g.prize_total += seq->prize_count;
	return psv;
}

static void AddRules( PCONFIG_HANDLER pch );

static PTRSZVAL CPROC ProcessConfig( PTRSZVAL psv, arg_list args )
{
	PARAM( args, size_t, length );
	PARAM( args, CPOINTER, data );
	P_8 realdata;
	size_t reallength;
	PCONFIG_HANDLER pch = CreateConfigurationHandler();
	AddRules( pch );
	SRG_DecryptRawData( (P_8)data, length, &realdata, &reallength );
	ProcessConfigurationInput( pch, (CTEXTSTR)realdata, reallength, 0 );
	DestroyConfigurationEvaluator( pch );
	return psv;
}

static void AddRules( PCONFIG_HANDLER pch )
{
	AddConfigurationMethod( pch, WIDE("config=%B"), ProcessConfig );
	AddConfigurationMethod( pch, WIDE("%i,%i,%i,%m"), AddVideo );
	AddConfigurationMethod( pch, WIDE("default show time=%i"), SetDefaultShowTime );
	AddConfigurationMethod( pch, WIDE("default fade in time=%i"), SetDefaultFadeInTime );
	AddConfigurationMethod( pch, WIDE("default prize count %i=%i"), SetDefaultPrizeCount );
}

static void ReadConfigFile( CTEXTSTR filename )
{
	PCONFIG_HANDLER pch = CreateConfigurationHandler();
	AddRules( pch );
	ProcessConfigurationFile( pch, filename, 0 );
	DestroyConfigurationHandler( pch );
}

static void PickPrize( void )
{
	_32 rand = genrand_int32( g.rng );
	_32 value = ( (_64)g.prize_total * (_64)rand ) / 0xFFFFFFFF;
	struct video_sequence *seq;
	INDEX idx;
	//lprintf( "got numbr %d %d", rand, value );
	LIST_FORALL( g.playlists, idx, struct video_sequence *, seq )
	{
		//lprintf( "is %d < %d", value, seq->prize_count );
		if( value < seq->prize_count )
		{
			//lprintf( "want_next_chain = %d", 1 + seq->sequence );
			//g.want_next_chain = 1 + seq->sequence;
			g.want_next_chain = 0;  // will already be setting up this chain...
			seq->prize_count--;
			g.prize_total--;
			//lprintf( "pick prize; set next to %d", seq->sequence );
			g.current_list[g.next_up] = seq;
			seq->current_image = 0;
			g.target_in_start = timeGetTime();
			{
				TEXTCHAR value[32];
				tnprintf( value, 32, "%d", (int)seq->sequence );
				SACK_WriteProfileInt( "Prize Counts", value, seq->prize_count );
			}
			break;
		}
		value -= seq->prize_count;
	}
}

static LOGICAL CPROC PressSomeKey( PTRSZVAL psv, _32 key )
{
	static _32 _tick, tick;
	static int reset = 0;
	static int reset2 = 0;
	static int reset3 = 0;
	static int reset4 = 0;
	tick = timeGetTime();
	//lprintf( "got key %08x  (%d)  %d ", key, psv, tick - _tick );
	if( !_tick || ( _tick < ( tick - 2000 ) ) )
	{
		//lprintf( "late enough" );
		reset4 = 0;
		reset3 = 0;
		reset2 = 0;
		reset = 0;
		g.begin_card = 0;
		g.number_collector = 0;
	}
	_tick = tick;

	{
		//lprintf( "continue sequence... begin new collections" );
		if( psv < 10 )
		{
			// reset to new value
			g.number_collector = ( g.number_collector * 10 ) + psv;
			g.value_collector[g.value_collect_index++] = psv + '0';
			g.value_collector[g.value_collect_index] = 0;
			//lprintf( "new value %d (%s)", g.number_collector, g.value_collector );
			if( g.attract_mode )
			{
				if( g.number_collector == 13579 )
				{
					g.attract_mode = 0;
					// last one is the screen to show to swipe card...
					g.want_next_chain = g.nPlayerLists;
					//lprintf( "want_next_chain = %d", g.want_next_chain);
					g.target_in_start = tick;
					//lprintf( "next is still this...(last)"  );

					g.current_list[g.next_up] = (struct video_sequence*)GetLink( &g.playlists, g.nPlayerLists - 1 );
				}

			}
		}
		else if( psv == 10 )
		{
			//lprintf( "Begin swipe..." );
			if( !g.attract_mode )
			{
				g.begin_card = 1;
				g.number_collector = 0;
				g.value_collect_index = 0;
				g.value_collector[g.value_collect_index] = 0;
			}
		}
		else if( psv == 11 ) // '?'
		{
			//lprintf( "end card with (%s)", g.value_collector );
			if( !g.attract_mode )
			{
				PickPrize();
			}
			//else
			//	lprintf( "Still in attract?" );
		}
	}
	return TRUE;
}

SaneWinMain(argc, argv )
//int main( int argc, char **argv )
{
	_32 width, height;
	S_32 x, y;
	g.pdi = GetDisplayInterface();
	g.pii = GetImageInterface();
	RegisterIcon( NULL );

	GetDisplaySizeEx( 0, &x, &y, &width, &height );
	g.w = width;
	g.h = height;
	g.x = x;
	g.y = y;

	{
		_32 tick = timeGetTime();
		g.rng = init_by_array( &tick, 1 );
	}

	BindEventToKey( NULL, KEY_0, 0, PressSomeKey, (PTRSZVAL)0 );
	BindEventToKey( NULL, KEY_1, 0, PressSomeKey, (PTRSZVAL)1 );
	BindEventToKey( NULL, KEY_2, 0, PressSomeKey, (PTRSZVAL)2 );
	BindEventToKey( NULL, KEY_3, 0, PressSomeKey, (PTRSZVAL)3 );
	BindEventToKey( NULL, KEY_4, 0, PressSomeKey, (PTRSZVAL)4 );
	BindEventToKey( NULL, KEY_5, 0, PressSomeKey, (PTRSZVAL)5 );
	BindEventToKey( NULL, KEY_6, 0, PressSomeKey, (PTRSZVAL)6 );
	BindEventToKey( NULL, KEY_7, 0, PressSomeKey, (PTRSZVAL)7 );
	BindEventToKey( NULL, KEY_8, 0, PressSomeKey, (PTRSZVAL)8 );
	BindEventToKey( NULL, KEY_9, 0, PressSomeKey, (PTRSZVAL)9 );
	BindEventToKey( NULL, KEY_9, 0, PressSomeKey, (PTRSZVAL)9 );
	BindEventToKey( NULL, KEY_5, KEY_MOD_SHIFT, PressSomeKey, (PTRSZVAL)10 );
	BindEventToKey( NULL, KEY_SEMICOLON, 0, PressSomeKey, (PTRSZVAL)10 );
	BindEventToKey( NULL, KEY_SLASH, KEY_MOD_SHIFT, PressSomeKey, (PTRSZVAL)11 );

	{
		int state = 0;
		int arg;
		g.default_fade_in = 500;
		g.default_show_time = 1000;
		for( arg = 1; arg < argc; arg++ )
		{

			if( argv[arg][0] == WIDE('-') )
			{
				switch( state )
				{
				case 0:
					g.x = atoi( argv[arg]+1 );
					break;
				case 1:
					g.y = atoi( argv[arg]+1 );
					break;
				case 2:
					g.w = atoi( argv[arg]+1 );
					break;
				case 3:
					g.h = atoi( argv[arg]+1 );
					break;
				case 4:
					g.default_show_time = atoi( argv[arg]+1 );
					break;
				case 5:
					g.default_fade_in = atoi( argv[arg]+1 );
					break;
				case 6:
					g.next_fade_in = atoi( argv[arg]+1 );
					state--; // stay at state 6
					break;
				}
				state++;
			}
			else if( argv[arg][0] == WIDE('@') )
			{
				ReadConfigFile( argv[arg] + 1 );
			}
			else
				lprintf( WIDE("Unhandled argument: %s"), argv[arg] );
		}
	}

	{
		struct video_sequence *sequence;
		sequence = GetSequence( 0 );


		if( g.nTotalImages )
		{
			g.displays[0] = OpenDisplaySizedAt( DISPLAY_ATTRIBUTE_LAYERED|DISPLAY_ATTRIBUTE_CHILD|/*DISPLAY_ATTRIBUTE_NO_MOUSE|*/DISPLAY_ATTRIBUTE_NO_AUTO_FOCUS
														 , g.w //width
														 , g.h //height
														 , g.x //0
														 , g.y //0
														 );
			g.displays[1] = OpenDisplaySizedAt( DISPLAY_ATTRIBUTE_LAYERED|DISPLAY_ATTRIBUTE_CHILD|/*DISPLAY_ATTRIBUTE_NO_MOUSE|*/DISPLAY_ATTRIBUTE_NO_AUTO_FOCUS
														 , g.w //width
														 , g.h //height
														 , g.x //0
														 , g.y //0
														 );
			SetMouseHandler( g.displays[1], MouseMethod, (PTRSZVAL)0 );
			SetMouseHandler( g.displays[1], MouseMethod, (PTRSZVAL)0 );

			SetRedrawHandler( g.displays[0], Output, 0 );
			SetRedrawHandler( g.displays[1], Output, 1 );

			AddTimer( 33, tick, 0 );

			if( sequence->nImages )
			{
				g.want_next_chain = 0;
				g.attract_mode = 1;
				g.next_chain = 1;
				g.target_in_start = timeGetTime();
			}
			else
			{
				// we'll have to wait with 0 displays shown.  In the first group we have no images.
			}

			while( 1 )
				WakeableSleep( 10000 );
		}
		else
		{
			lprintf( WIDE("No Images to display, exiting") );
			printf( WIDE("No Images to display, exiting\n") );
		}
	}
	return 0;
}
EndSaneWinMain()

