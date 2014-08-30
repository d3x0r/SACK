
#include <stdhdrs.h>
#include <sqlgetoption.h>
#define DEFINES_INTERSHELL_INTERFACE
#define USES_INTERSHELL_INTERFACE
#define USE_RENDER_INTERFACE ffl.pdi
#define USE_IMAGE_INTERFACE ffl.pii
#include <render.h>
#include <salty_generator.h>
#include <ffmpeg_interface.h>
#include <mt19937ar.h>
#include "../../widgets/include/banner.h"
#include "../../intershell_registry.h"
#include "../../intershell_export.h"

RegisterControlWithBorderEx( WIDE("FF_Attract"), sizeof( struct attract_control *), BORDER_NONE, FF_Attract );
RegisterControlWithBorderEx( WIDE("FF_Grid"), sizeof( struct game_grid_control *), BORDER_NONE, FF_Grid );
RegisterControlWithBorderEx( WIDE("FF_Grid_Cell"), sizeof( struct game_cell_control *), BORDER_NONE, FF_GridCell );
RegisterControlWithBorderEx( WIDE("FF_Scoreboard"), sizeof( struct attract_control *), BORDER_NONE, FF_Scoreboard );

struct game_grid_control
{
	PSI_CONTROL pc;
	Image background;
	Image foreground;
};

struct game_cell_control
{
	LOGICAL playing;
	PSI_CONTROL pc;
	Image sticker;
	Image Prize;
	struct ffmpeg_file *file;
	struct ffmpeg_file *sound_file;
	//S_32 cx, cy;
	//_32 cw, ch;
	S_32 bx, by;
	_32 bw, bh;
	S_32 fx, fy;
	_32 fw, fh;
};

static struct fantasy_football_local
{
	CTEXTSTR attract;
	CTEXTSTR downs[4];
	CTEXTSTR helmets[32];
	CTEXTSTR helmet_sticker_name[32];
	CTEXTSTR helmet_sound;
	struct 
	{
		struct game_grid_control control;
		struct game_cell_control cell[32];
		CTEXTSTR background;
		CTEXTSTR foreground;
		FRACTION offset_x, offset_y;
		FRACTION cell_width, cell_height;
	}grid;
	TEXTCHAR card_end_char;
	TEXTCHAR card_begin_char;
	_64 enable_code;

	struct prize_data
	{
		struct {
			TEXTCHAR name;
			int value;
			Image image;
		} value_images;
		struct prize_grid_data {
			int value;
			int count;
		} *grid;
		int grid_prizes_avail;
		int grid_prizes_used;
		struct prize_line_data {
			int picks;
			int *payouts; // dynamic array payouts[line][pick]
			int count; // how many this line can be picked
		} *lines;
		struct {
			int *value; // current shuffled values 
		} prize_line;
		struct {
			int value;
		} *grid_prizes;
		int avail_lines; // how many lines allocated
		int number_lines; // how many lines used
		int total_lines; // sum of total of counts of lines
		int total_games;
	} prizes;
	PTREEROOT prize_shuffle;
	int current_down;

	LOGICAL attract_mode;
	_32 number_collector;
	_32 value_collector[64];
	int value_collect_index;
	int begin_card;


	PIMAGE_INTERFACE pii;
	PRENDER_INTERFACE pdi;

	struct {
		BIT_FIELD prize_line_mode : 1;
	} flags;

	struct {
		CTEXTSTR movie_name;
		struct ffmpeg_file *movie_file;
		CTEXTSTR static_name;
		Image static_image;
		CTEXTSTR fontname;
		_32 delay_draw;
		_32 delay_finish;

		_32 tick_draw; // when drawing starts?
		_32 tick_finish;
		struct {
			SFTFont font;
			FRACTION font_x, font_y;
			FRACTION x, y;
		} leader;
		struct {
			SFTFont font;
			FRACTION font_x, font_y;
			FRACTION x, y;
		} down;
		struct {
			SFTFont font;
			FRACTION font_x, font_y;
			FRACTION x, y;
		} total;
	} scoreboard;
	struct mersenne_rng *rng;
} ffl;

struct attract_control
{
	LOGICAL playing;
	PSI_CONTROL pc;
	struct ffmpeg_file *file;
	LOGICAL attract;
};

PRELOAD( InitInterfaces )
{
	ffl.pii = GetImageInterface();
	ffl.pdi = GetDisplayInterface();
	ffl.rng = init_genrand( timeGetTime() );
}

static void AddRules( PCONFIG_HANDLER pch );
static void InitKeys( void );


static struct prize_grid_data * GetGridPrize( void )
{
	if( ffl.prizes.grid_prizes_used >= ffl.prizes.grid_prizes_avail )
	{
		struct prize_grid_data *tmp = NewArray( struct prize_grid_data, ffl.prizes.grid_prizes_avail + 16 );
		if( ffl.prizes.grid )
		{
			MemCpy( tmp, ffl.prizes.grid, sizeof( struct prize_grid_data ) * ffl.prizes.grid_prizes_avail );
			Release( ffl.prizes.grid );
		}
		ffl.prizes.grid = tmp;
		ffl.prizes.grid_prizes_avail += 16;
	}
	return ffl.prizes.grid + ffl.prizes.grid_prizes_used++;
}

static void DeductPrizeLine( int n )
{
	TEXTCHAR buf[12];
	ffl.prizes.lines[n].count--;
	ffl.prizes.total_games--;
	snprintf( buf, 12, "%d", n + 1 );
	SACK_WriteProfileInt( "Prizes/Lines", buf, ffl.prizes.lines[n].count );
	SACK_WriteProfileInt( "Prizes", "Games", ffl.prizes.total_games );
}

static void ChoosePrize( void )
{
	if( !ffl.prize_shuffle )
		ffl.prize_shuffle = CreateBinaryTree();
	else
		ResetBinaryTree( ffl.prize_shuffle );

	// reset down count
	ffl.current_down = 0;
	ffl.scoreboard.tick_draw = 0;

	if( ffl.flags.prize_line_mode )
	{
		_32 rand = genrand_int32( ffl.rng );
		_32 value = ( (_64)ffl.prizes.total_lines * (_64)rand ) / 0xFFFFFFFFU;
		int n;
		for( n = 0; n < ffl.prizes.number_lines; n++ )
		{
			if( value < ffl.prizes.lines[n].count )
			{
				int down;
				for( down = 0; down < ffl.prizes.lines[n].picks; down++ )
				{
					rand = genrand_int32( ffl.rng );
					AddBinaryNode( ffl.prize_shuffle, (POINTER)ffl.prizes.lines[n].payouts[down], (PTRSZVAL)rand );
				}
				for( down = 0; down < 4; down++ )
				{
					if( !down )
						ffl.prizes.prize_line.value[down] = (_32)GetLeastNode( ffl.prize_shuffle );
					else
						ffl.prizes.prize_line.value[down] = (_32)GetGreaterNode( ffl.prize_shuffle );
				}
				DeductPrizeLine( n );
				break;
			}
			else
				value -= ffl.prizes.lines[n].count;
		}
	}
	else
	{
		_32 rand;
		int square;
		int g = 0;
		int c = 0;
		for( square = 0; square < 32; square++ )
		{
			rand = genrand_int32( ffl.rng );
			AddBinaryNode( ffl.prize_shuffle, (POINTER)ffl.prizes.grid[g].value, (PTRSZVAL)rand );
			c++;
			if( c == ffl.prizes.grid[g].count )
			{
				c = 0;
				g++;
			}
		}
		for( square = 0; square < 32; square++ )
		{
			if( !square )
				ffl.prizes.grid_prizes[square].value = (_32)GetLeastNode( ffl.prize_shuffle );
			else
				ffl.prizes.grid_prizes[square].value = (_32)GetGreaterNode( ffl.prize_shuffle );
		}
	}
}


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

static PTRSZVAL CPROC SetStartCharacter( PTRSZVAL psv, arg_list args )
{
	PARAM( args, char*, data );
	ffl.card_begin_char = data[0];
	return psv;
}

static PTRSZVAL CPROC SetEndCharacter( PTRSZVAL psv, arg_list args )
{
	PARAM( args, char*, data );
	ffl.card_end_char = data[0];
	return psv;
}

static PTRSZVAL CPROC SetAttract( PTRSZVAL psv, arg_list args )
{
	PARAM( args, CTEXTSTR, data );
	ffl.attract = StrDup( data );
	return psv;
}

static PTRSZVAL CPROC SetGameForeground( PTRSZVAL psv, arg_list args )
{
	PARAM( args, CTEXTSTR, data );
	ffl.grid.foreground = StrDup( data );
	return psv;
}


static PTRSZVAL CPROC SetHelmetSound( PTRSZVAL psv, arg_list args )
{
	PARAM( args, CTEXTSTR, data );
	ffl.helmet_sound = StrDup( data );
	return psv;
}

static PTRSZVAL CPROC SetGameBackground( PTRSZVAL psv, arg_list args )
{
	PARAM( args, CTEXTSTR, data );
	ffl.grid.background = StrDup( data );
	return psv;
}

static PTRSZVAL CPROC SetGameOffset( PTRSZVAL psv, arg_list args )
{
	PARAM( args, FRACTION, x );
	PARAM( args, FRACTION, y );
	ffl.grid.offset_x = (x);
	ffl.grid.offset_y = (y);
	return psv;
}

static PTRSZVAL CPROC SetGameSize( PTRSZVAL psv, arg_list args )
{
	PARAM( args, FRACTION, x );
	PARAM( args, FRACTION, y );
	ffl.grid.cell_width = (x);
	ffl.grid.cell_height = (y);
	return psv;
}

static PTRSZVAL CPROC SetDownIntro( PTRSZVAL psv, arg_list args )
{
	PARAM( args, S_64, down );
	PARAM( args, CTEXTSTR, data );
	ffl.downs[down-1] = StrDup( data );
	return psv;
}
static PTRSZVAL CPROC SetTeamHelmet( PTRSZVAL psv, arg_list args )
{
	PARAM( args, S_64, down );
	PARAM( args, CTEXTSTR, data );
	ffl.helmets[down-1] = StrDup( data );
	return psv;
}

static PTRSZVAL CPROC SetTeamHelmetSticker( PTRSZVAL psv, arg_list args )
{
	PARAM( args, S_64, down );
	PARAM( args, CTEXTSTR, data );
	ffl.helmet_sticker_name[down-1] = StrDup( data );
	return psv;
}

static PTRSZVAL CPROC SetSwipeEnable( PTRSZVAL psv, arg_list args )
{
	PARAM( args, S_64, data );
	ffl.enable_code = data;
	return psv;
}

static PTRSZVAL CPROC SetValueImage( PTRSZVAL psv, arg_list args )
{
	PARAM( args, S_64, data );
	PARAM( args, CTEXTSTR, filename );
	return psv;
}

static PTRSZVAL CPROC SetUsePrizeLines( PTRSZVAL psv, arg_list args )
{
	PARAM( args, S_64, data );
	ffl.flags.prize_line_mode = data;
	return psv;
}
static PTRSZVAL CPROC SetGameCount( PTRSZVAL psv, arg_list args )
{
	PARAM( args, S_64, data );
	ffl.prizes.total_games = data;
	return psv;
}


static void GetPrizeLine( int line )
{
	if( ffl.prizes.avail_lines < line )
	{
		struct prize_line_data *tmp = NewArray( struct prize_line_data, ffl.prizes.avail_lines+ 16 );
		if( ffl.prizes.lines )
		{
			MemCpy( tmp, ffl.prizes.lines, sizeof( struct prize_line_data ) * ffl.prizes.avail_lines );
			MemSet( tmp + ffl.prizes.avail_lines, 0, sizeof( struct prize_line_data ) * 16 );
			Release( ffl.prizes.lines );
		}
		ffl.prizes.lines = tmp;
		ffl.prizes.avail_lines += 16;
	}
}

static PTRSZVAL CPROC SetPrizeLineCount( PTRSZVAL psv, arg_list args )
{
	PARAM( args, S_64, line );
	PARAM( args, S_64, count );
	TEXTCHAR buf[12];
	snprintf( buf, 12, WIDE("%")_64fs, line );
	GetPrizeLine( line );
	ffl.prizes.lines[line-1].count = SACK_GetProfileIntEx( WIDE("Prizes/Lines"), buf, count, TRUE );
	return psv;
}
static PTRSZVAL CPROC SetPrizeLinePicks( PTRSZVAL psv, arg_list args )
{
	PARAM( args, S_64, line );
	PARAM( args, S_64, picks );
	GetPrizeLine( line );
	ffl.prizes.lines[line-1].picks = picks;
	ffl.prizes.lines[line-1].payouts = NewArray( int, picks );
	return psv;
}
static PTRSZVAL CPROC SetPrizeLinePickValue( PTRSZVAL psv, arg_list args )
{
	PARAM( args, S_64, line );
	PARAM( args, S_64, pick );
	PARAM( args, S_64, value );
	GetPrizeLine( line );
	ffl.prizes.lines[line-1].payouts[pick] = value;
	return psv;
}

static PTRSZVAL CPROC SetGridPrize( PTRSZVAL psv, arg_list args )
{
	PARAM( args, S_64, count );
	PARAM( args, S_64, value );
	struct prize_grid_data *prize = GetGridPrize( );
	prize->value = value;
	prize->count = count;
	return psv;
}


static PTRSZVAL CPROC 	SetScoreboardMovie ( PTRSZVAL psv, arg_list args )
{
	PARAM( args, CTEXTSTR, name );	ffl.scoreboard.movie_name= StrDup( name );	return psv;
}
	static PTRSZVAL CPROC SetScoreboardStatic( PTRSZVAL psv, arg_list args )
{
	PARAM( args, CTEXTSTR, name );	ffl.scoreboard.static_image = LoadImageFile( name );	return psv;
}
	static PTRSZVAL CPROC SetScoreboardFont( PTRSZVAL psv, arg_list args )
{
	PARAM( args, CTEXTSTR, name );	ffl.scoreboard.fontname = StrDup( name );	return psv;
}
	static PTRSZVAL CPROC SetScoreLeaderPosition( PTRSZVAL psv, arg_list args )
{
	PARAM( args, FRACTION, x);	PARAM( args, FRACTION, y);	ffl.scoreboard.leader.x = x;	ffl.scoreboard.leader.y = y;	return psv;
}
	static PTRSZVAL CPROC SetScoreDownPosition( PTRSZVAL psv, arg_list args )
{
	PARAM( args, FRACTION, x);	PARAM( args, FRACTION, y);	ffl.scoreboard.down.x = x;	ffl.scoreboard.down.y = y;	return psv;
}
	static PTRSZVAL CPROC SetScoreTotalPosition( PTRSZVAL psv, arg_list args )
{
	PARAM( args, FRACTION, x);	PARAM( args, FRACTION, y);	ffl.scoreboard.total.x = x;	ffl.scoreboard.total.y = y;	return psv;
}

	static PTRSZVAL CPROC SetScoreLeaderFontSize( PTRSZVAL psv, arg_list args )
{
	PARAM( args, FRACTION, x);	PARAM( args, FRACTION, y);	ffl.scoreboard.leader.font_x = x;	ffl.scoreboard.leader.font_y = y;	return psv;
}
	static PTRSZVAL CPROC SetScoreDownFontSize( PTRSZVAL psv, arg_list args )
{
	PARAM( args, FRACTION, x);	PARAM( args, FRACTION, y);	ffl.scoreboard.down.font_x = x;	ffl.scoreboard.down.font_y = y;	return psv;
}
	static PTRSZVAL CPROC SetScoreTotalFontSize( PTRSZVAL psv, arg_list args )
{
	PARAM( args, FRACTION, x);	PARAM( args, FRACTION, y);	ffl.scoreboard.total.font_x = x;	ffl.scoreboard.total.font_y = y;	return psv;
}
	static PTRSZVAL CPROC SetScoreDrawDelay( PTRSZVAL psv, arg_list args )
{
	PARAM( args, S_64, count );	ffl.scoreboard.delay_draw = count;	return psv;
}

	static PTRSZVAL CPROC SetScoreOutputDoneDelay( PTRSZVAL psv, arg_list args )
{
	PARAM( args, S_64, count );	ffl.scoreboard.delay_finish = count;	return psv;
}


static void AddRules( PCONFIG_HANDLER pch )
{
	AddConfigurationMethod( pch, WIDE("config=%B"), ProcessConfig );
	AddConfigurationMethod( pch, WIDE("Attract=%m"), SetAttract );
	AddConfigurationMethod( pch, WIDE("Down %i=%m"), SetDownIntro );
	AddConfigurationMethod( pch, WIDE("Grid Background=%m"), SetGameBackground );
	AddConfigurationMethod( pch, WIDE("Grid Foreground=%m"), SetGameForeground );
	AddConfigurationMethod( pch, WIDE("Team %i Helmet=%m"), SetTeamHelmet );
	AddConfigurationMethod( pch, WIDE("Team %i Helmet Sticker=%m"), SetTeamHelmetSticker );
	AddConfigurationMethod( pch, WIDE("Grid offset XY=%q %q"), SetGameOffset );
	AddConfigurationMethod( pch, WIDE("Grid Cell Size=%q %q"), SetGameSize );
	AddConfigurationMethod( pch, WIDE("Helmet Sound=%m"), SetHelmetSound );
	AddConfigurationMethod( pch, WIDE("card swipe enable=%i"), SetSwipeEnable );
	AddConfigurationMethod( pch, WIDE("card start character=%w"), SetStartCharacter );
	AddConfigurationMethod( pch, WIDE("card end character=%w"), SetEndCharacter );

	AddConfigurationMethod( pch, WIDE("Prize Image fot value %i=%m"), SetValueImage);
	AddConfigurationMethod( pch, WIDE("Use Grid Prize Lines=%b"), SetUsePrizeLines );
	AddConfigurationMethod( pch, WIDE("Total Games=%i"), SetGameCount );
	AddConfigurationMethod( pch, WIDE("Prize Line %i count = %i"), SetPrizeLineCount );
	AddConfigurationMethod( pch, WIDE("Prize Line %i picks = %i"), SetPrizeLinePicks );
	AddConfigurationMethod( pch, WIDE("Prize Line %i down %i = %i"), SetPrizeLinePickValue );
	AddConfigurationMethod( pch, WIDE("Grid Prize= %i,%i"), SetGridPrize );

	AddConfigurationMethod( pch, WIDE("Scoreboard movie=%m"), SetScoreboardMovie );	AddConfigurationMethod( pch, WIDE("Scoreboard static=%m"), SetScoreboardStatic );	AddConfigurationMethod( pch, WIDE("Scoreboard font=%m"), SetScoreboardFont );	AddConfigurationMethod( pch, WIDE("Scoreboard leader position=%q %q"), SetScoreLeaderPosition );	AddConfigurationMethod( pch, WIDE("Scoreboard down position=%q %q"), SetScoreDownPosition );	AddConfigurationMethod( pch, WIDE("Scoreboard total position=%q %q"), SetScoreTotalPosition );	AddConfigurationMethod( pch, WIDE("Scoreboard leader text size=%q %q"), SetScoreLeaderFontSize );	AddConfigurationMethod( pch, WIDE("Scoreboard down text size=%q %q"), SetScoreDownFontSize );	AddConfigurationMethod( pch, WIDE("Scoreboard total text size=%q %q"), SetScoreTotalFontSize );	AddConfigurationMethod( pch, WIDE("Scoreboard Start time=%i"), SetScoreDrawDelay );	AddConfigurationMethod( pch, WIDE("Scoreboard End time=%i"), SetScoreOutputDoneDelay );

}

static void ReadConfigFile( CTEXTSTR filename )
{
	PCONFIG_HANDLER pch = CreateConfigurationHandler();
	AddRules( pch );
	ProcessConfigurationFile( pch, filename, 0 );
	DestroyConfigurationHandler( pch );
}

static void OnFinishInit( WIDE( "Fantasy Football" ) )( PSI_CONTROL canvas )
{
	// final init.. count how many prizes...
	int n;
	for( n = 0; n < ffl.prizes.number_lines; n++ )
	{
		ffl.prizes.total_lines += ffl.prizes.lines[n].count;
	}
}

static void OnLoadCommon( WIDE( "Fantasy Football" ) )( PCONFIG_HANDLER pch )
{
	ReadConfigFile( "fantasy_football_game.config" );
}



static void EndScoreboard( PTRSZVAL psv )
{
	// do nothing...
	ffl.scoreboard.tick_draw = 0; 
	/*
	struct attract_control *ac = (struct attract_control *)psv;
	if( ac->file )
	{
		ffl.attract_mode = ac->attract;
		ffmpeg_SeekFile( ac->file, 0 );
		ffmpeg_PlayFile( ac->file );
	}
	*/
}

static void RestartAttract( PTRSZVAL psv )
{
	struct attract_control *ac = (struct attract_control *)psv;
	if( ac->file )
	{
		ffl.attract_mode = ac->attract;
		ffmpeg_SeekFile( ac->file, 0 );
		ffmpeg_PlayFile( ac->file );
	}
}

// helmet animation ended
static void EndGridCell( PTRSZVAL psv )
{
	struct game_cell_control *gcc = (struct game_cell_control *)psv;
	gcc->playing = FALSE;
	SmudgeCommon( gcc->pc ); // one more draw to make sure movie part is cleared
}

static void EndGridCellSound( PTRSZVAL psv )
{
	//struct game_cell_control *gcc = (struct game_cell_control *)psv;
	//gcc->playing = FALSE;
	//SmudgeCommon( gcc->pc ); // one more draw to make sure movie part is cleared
}

static int OnCreateCommon( WIDE( "FF_Attract" ) )( PSI_CONTROL pc )
{
	return 1;
}

static void OnHideCommon( WIDE("FF_Attract") )( PSI_CONTROL pc )
{
	ValidatedControlData( struct attract_control **, FF_Attract.TypeID, ppac, pc );
	struct attract_control *ac = (*ppac);
	ac->playing = FALSE;
	if( ac->attract )
	{
		ffl.attract_mode = 0;
		ChoosePrize();
	}
	ffmpeg_PauseFile( ac->file );
}

static void OneShotDelay( PTRSZVAL psv )
{
	RestartAttract( psv );
}

static void OnRevealCommon( WIDE("FF_Attract") )( PSI_CONTROL pc )
{
	ValidatedControlData( struct attract_control **, FF_Attract.TypeID, ppac, pc );
	struct attract_control *ac = (*ppac);
	if( !ac->playing )
	{
		ac->playing = TRUE;
	//	AddTimerEx( 250, 0, OneShotDelay, (PTRSZVAL)ac );
	}
	RestartAttract( (PTRSZVAL)ac );
}


static int OnCreateCommon( WIDE( "FF_Grid" ) )( PSI_CONTROL pc )
{
	return 1;
}

static int OnDrawCommon( WIDE( "FF_Grid" ) )( PSI_CONTROL pc )
{
	Image surface = GetControlSurface( pc );
	BlotScaledImage( surface, ffl.grid.control.background );
	BlotScaledImageAlpha( surface, ffl.grid.control.foreground, ALPHA_TRANSPARENT );
	return 1;
}

static int OnCreateCommon( WIDE( "FF_Grid_Cell" ) )( PSI_CONTROL pc )
{
	return 1;
}

static int OnMouseCommon( WIDE( "FF_Grid_Cell") )( PSI_CONTROL pc, S_32 x, S_32 y, _32 b )
{
	if( b )
	{
		ValidatedControlData( struct game_cell_control **, FF_GridCell.TypeID, ppgcc, pc );
		if( !ppgcc[0]->playing && !ppgcc[0]->Prize )
		{
			ffmpeg_SeekFile( ppgcc[0]->file, 0 );
			ffmpeg_PlayFile( ppgcc[0]->file );
			ffmpeg_SeekFile( ppgcc[0]->sound_file, 0 );
			ffmpeg_PlayFile( ppgcc[0]->sound_file );
			ppgcc[0]->playing = TRUE;
		}
	}
	return 1;
}

static void OnHideCommon( WIDE("FF_Grid_Cell") )( PSI_CONTROL pc )
{
	ValidatedControlData( struct game_cell_control **, FF_GridCell.TypeID, ppgcc, pc );
	struct game_cell_control *gcc = (*ppgcc);
	if( gcc->playing )
	{
		gcc->playing = FALSE;
		ffmpeg_PauseFile( gcc->file );
	}
}

static void DrawCellBackground( PSI_CONTROL pc )
{
	ValidatedControlData( struct game_cell_control **, FF_GridCell.TypeID, ppgcc, pc );
	Image surface = GetControlSurface( pc );
	BlotScaledImageSized( surface, ffl.grid.control.background
		, 0, 0
		, surface->width, surface->height 
		, ppgcc[0]->bx, ppgcc[0]->by
		, ppgcc[0]->bw, ppgcc[0]->bh );
}

static void DrawCellForeground( PSI_CONTROL pc )
{
	ValidatedControlData( struct game_cell_control **, FF_GridCell.TypeID, ppgcc, pc );
	Image surface = GetControlSurface( pc );
	BlotScaledImageSizedEx( surface, ffl.grid.control.foreground
		, 0, 0
		, surface->width, surface->height 
		, ppgcc[0]->fx, ppgcc[0]->fy
		, ppgcc[0]->fw, ppgcc[0]->fh, ALPHA_TRANSPARENT, BLOT_COPY );
}

static void DrawCellPrize( PSI_CONTROL pc )
{
	ValidatedControlData( struct game_cell_control **, FF_GridCell.TypeID, ppgcc, pc );
	Image surface = GetControlSurface( pc );
	BlotScaledImageSizedEx( surface, ffl.grid.control.foreground
		, 0, 0
		, surface->width, surface->height 
		, ppgcc[0]->fx, ppgcc[0]->fy
		, ppgcc[0]->fw, ppgcc[0]->fh, ALPHA_TRANSPARENT, BLOT_COPY );
}

static int OnDrawCommon( WIDE( "FF_Grid_Cell" ) )( PSI_CONTROL pc )
{
	ValidatedControlData( struct game_cell_control **, FF_GridCell.TypeID, ppgcc, pc );
	if( ppgcc[0] )
	{
		DrawCellBackground( pc );
		if( ppgcc[0]->Prize )
		{
			Image surface = GetControlSurface( pc );
			BlotScaledImageAlpha( surface, ppgcc[0]->Prize, ALPHA_TRANSPARENT );
		}
		else
		{
			Image surface = GetControlSurface( pc );
			BlotScaledImageAlpha( surface, ppgcc[0]->sticker, ALPHA_TRANSPARENT );
		}
		DrawCellForeground( pc );
	}
	return 1;
}


static int OnCreateCommon( WIDE( "FF_Scoreboard" ) )( PSI_CONTROL pc )
{
	return 1;
}

static void DrawScoreText( PSI_CONTROL pc )
{
	if( ffl.scoreboard.
}


static int OnDrawCommon( WIDE( "FF_Scoreboard" ) )( PSI_CONTROL pc )
{
	BlotScaledImage( GetControlSurface( pc ), ffl.scoreboard.static_image );
	if( ffl.scoreboard.tick_draw || ffl.scoreboard.tick_finish )
		DrawScoreText( pc );
	return 1;
}

static void OnHideCommon( WIDE("FF_Scoreboard") )( PSI_CONTROL pc )
{
	ValidatedControlData( struct attract_control **, FF_Attract.TypeID, ppac, pc );
	struct attract_control *ac = (*ppac);
	ac->playing = FALSE;
	ffmpeg_PauseFile( ac->file );
}

static void NextPage( PTRSZVAL psv )
{
	struct attract_control *ac = (struct attract_control *)psv;
	ShellSetCurrentPage( GetCommonParent( ac->pc ), "next" );
}



static PTRSZVAL OnCreateControl(WIDE( "Fantasy Football/Attract" ))(PSI_CONTROL parent,S_32 x,S_32 y,_32 w,_32 h)
{
	struct attract_control *ac;
	ac = New( struct attract_control );

	ac->playing = FALSE;
	ac->attract = TRUE;
	ac->pc = MakeNamedControl( parent, WIDE("FF_Attract"), x, y, w, h, 0 );
	{
		ValidatedControlData( struct attract_control **, FF_Attract.TypeID, ppac, ac->pc );
		(*ppac) = ac;
	}
	ac->file = ffmpeg_LoadFile( ffl.attract
								, NULL, 0
								, ac->pc
								, NULL, 0
								, RestartAttract, (PTRSZVAL)ac
								, NULL );
	return (PTRSZVAL)ac;
}

static PSI_CONTROL OnGetControl( WIDE("Fantasy Football/Attract") )( PTRSZVAL psv )
{
	struct attract_control *ac = (struct attract_control *)psv;
	return ac->pc;
}

static PTRSZVAL OnCreateControl(WIDE( "Fantasy Football/Game Grid" ))(PSI_CONTROL parent,S_32 x,S_32 y,_32 w,_32 h)
{
	struct game_grid_control *ac;
	ac = &ffl.grid.control;

	ac->pc = MakeNamedControl( parent, WIDE("FF_Grid"), x, y, w, h, 0 );
	ac->background = LoadImageFile( ffl.grid.background );
	ac->foreground = LoadImageFile( ffl.grid.foreground );
	InitKeys();
	{
		ValidatedControlData( struct game_grid_control **, FF_Grid.TypeID, ppac, ac->pc );
		(*ppac) = ac;
	}
	{
		int r, c;
		for( r = 0; r < 4; r++ )
			for( c = 0; c < 8; c++ )
			{
				FRACTION tmp1;
				FRACTION tmp2;
				FRACTION tmp3;
				int cx, cy, cw, ch;

				tmp1 = ffl.grid.offset_x;
				AddFractions( &tmp1, ScaleFraction( &tmp3, c, &ffl.grid.cell_width ) );
				tmp2 = ffl.grid.offset_x;
				AddFractions( &tmp2, ScaleFraction( &tmp3, c+1, &ffl.grid.cell_width ) );
				SubtractFractions( &tmp2, &tmp1 );

				ffl.grid.cell[r*8+c].bx = ScaleValue( &tmp1, ac->background->width );
				ffl.grid.cell[r*8+c].fx = ScaleValue( &tmp1, ac->foreground->width );
				cx = ScaleValue( &tmp1, w ); 
				ffl.grid.cell[r*8+c].bw = ScaleValue( &tmp2, ac->background->width );
				ffl.grid.cell[r*8+c].fw = ScaleValue( &tmp2, ac->foreground->width );
				cw = ScaleValue( &tmp2, w );

				tmp1 = ffl.grid.offset_y;
				AddFractions( &tmp1, ScaleFraction( &tmp2, r, &ffl.grid.cell_height ) );
				tmp2 = ffl.grid.offset_y;
				AddFractions( &tmp2, ScaleFraction( &tmp3, r+1, &ffl.grid.cell_height ) );
				SubtractFractions( &tmp2, &tmp1 );

				ffl.grid.cell[r*8+c].by = ScaleValue( &tmp1, ac->background->height );
				ffl.grid.cell[r*8+c].fy = ScaleValue( &tmp1, ac->foreground->height );
				cy = ScaleValue( &tmp1, h ); 
				ffl.grid.cell[r*8+c].bh = ScaleValue( &tmp2, ac->background->height );
				ffl.grid.cell[r*8+c].fh = ScaleValue( &tmp2, ac->foreground->height );
				ch = ScaleValue( &tmp2, h );
				lprintf( "cell %d is %d,%d %dx%d   %d,%d  %dx%d   %d,%d  %dx%d"
					, r * 8 + c
					, cx, cy, cw, ch
					, ffl.grid.cell[r*8+c].bx
					, ffl.grid.cell[r*8+c].by
					, ffl.grid.cell[r*8+c].bw
					, ffl.grid.cell[r*8+c].bh
					, ffl.grid.cell[r*8+c].fx
					, ffl.grid.cell[r*8+c].fy
					, ffl.grid.cell[r*8+c].fw
					, ffl.grid.cell[r*8+c].fh );
				ffl.grid.cell[r*8+c].pc = MakeNamedControl( ac->pc, WIDE("FF_Grid_Cell"), cx, cy, cw, ch, 0 );
				{
					ValidatedControlData( struct game_cell_control **, FF_GridCell.TypeID, ppgcc, ffl.grid.cell[r*8+c].pc );
					(*ppgcc) = ffl.grid.cell + (r*8+c);
				}
				ffl.grid.cell[r*8+c].sticker = LoadImageFile( ffl.helmet_sticker_name[r*8+c] );

				ffl.grid.cell[r*8+c].file = ffmpeg_LoadFile( ffl.helmets[r*8+c]
								, NULL, 0
								, ffl.grid.cell[r*8+c].pc
								, NULL, 0
								, EndGridCell, (PTRSZVAL)&ffl.grid.cell[r*8+c]
								, NULL );
				ffl.grid.cell[r*8+c].sound_file = ffmpeg_LoadFile( ffl.helmet_sound
								, NULL, 0
								, NULL
								, NULL, 0
								, NULL, 0
								, NULL );

				ffmpeg_SetPrePostDraw( ffl.grid.cell[r*8+c].file, DrawCellBackground, NULL/*DrawCellForeground*/ );

			}
	}
	return (PTRSZVAL)ac;
}

static PSI_CONTROL OnGetControl( WIDE("Fantasy Football/Game Grid") )( PTRSZVAL psv )
{
	struct game_grid_control *ggc = (struct game_grid_control *)psv;
	return ggc->pc;
}


static PTRSZVAL OnCreateControl(WIDE( "Fantasy Football/1st Down" ))(PSI_CONTROL parent,S_32 x,S_32 y,_32 w,_32 h)
{
	struct attract_control *ac;
	ac = New( struct attract_control );

	ac->playing = FALSE;
	ac->attract = FALSE;
	ac->pc = MakeNamedControl( parent, WIDE("FF_Attract"), x, y, w, h, 0 );
	{
		ValidatedControlData( struct attract_control **, FF_Attract.TypeID, ppac, ac->pc );
		(*ppac) = ac;
	}
	ac->file = ffmpeg_LoadFile( ffl.downs[0]
								, NULL, 0
								, ac->pc
								, NULL, 0
								, NextPage, (PTRSZVAL)ac
								, NULL );
	return (PTRSZVAL)ac;
}

static PSI_CONTROL OnGetControl( WIDE("Fantasy Football/1st Down") )( PTRSZVAL psv )
{
	struct attract_control *ac = (struct attract_control *)psv;
	return ac->pc;
}

static PTRSZVAL OnCreateControl(WIDE( "Fantasy Football/2nd Down" ))(PSI_CONTROL parent,S_32 x,S_32 y,_32 w,_32 h)
{
	struct attract_control *ac;
	ac = New( struct attract_control );

	ac->playing = FALSE;
	ac->attract = FALSE;
	ac->pc = MakeNamedControl( parent, WIDE("FF_Attract"), x, y, w, h, 0 );
	{
		ValidatedControlData( struct attract_control **, FF_Attract.TypeID, ppac, ac->pc );
		(*ppac) = ac;
	}
	ac->file = ffmpeg_LoadFile( ffl.downs[1]
								, NULL, 0
								, ac->pc
								, NULL, 0
								, NextPage, (PTRSZVAL)ac
								, NULL );
	return (PTRSZVAL)ac;
}

static PSI_CONTROL OnGetControl( WIDE("Fantasy Football/2nd Down") )( PTRSZVAL psv )
{
	struct attract_control *ac = (struct attract_control *)psv;
	return ac->pc;
}

static PTRSZVAL OnCreateControl(WIDE( "Fantasy Football/3rd Down" ))(PSI_CONTROL parent,S_32 x,S_32 y,_32 w,_32 h)
{
	struct attract_control *ac;
	ac = New( struct attract_control );

	ac->playing = FALSE;
	ac->attract = FALSE;
	ac->pc = MakeNamedControl( parent, WIDE("FF_Attract"), x, y, w, h, 0 );
	{
		ValidatedControlData( struct attract_control **, FF_Attract.TypeID, ppac, ac->pc );
		(*ppac) = ac;
	}
	ac->file = ffmpeg_LoadFile( ffl.downs[2]
								, NULL, 0
								, ac->pc
								, NULL, 0
								, NextPage, (PTRSZVAL)ac
								, NULL );
	return (PTRSZVAL)ac;
}

static PSI_CONTROL OnGetControl( WIDE("Fantasy Football/3rd Down") )( PTRSZVAL psv )
{
	struct attract_control *ac = (struct attract_control *)psv;
	return ac->pc;
}

static PTRSZVAL OnCreateControl(WIDE( "Fantasy Football/4th Down" ))(PSI_CONTROL parent,S_32 x,S_32 y,_32 w,_32 h)
{
	struct attract_control *ac;
	ac = New( struct attract_control );

	ac->playing = FALSE;
	ac->attract = FALSE;
	ac->pc = MakeNamedControl( parent, WIDE("FF_Attract"), x, y, w, h, 0 );
	{
		ValidatedControlData( struct attract_control **, FF_Attract.TypeID, ppac, ac->pc );
		(*ppac) = ac;
	}
	ac->file = ffmpeg_LoadFile( ffl.downs[3]
								, NULL, 0
								, ac->pc
								, NULL, 0
								, NextPage, (PTRSZVAL)ac
								, NULL );
	return (PTRSZVAL)ac;
}

static PSI_CONTROL OnGetControl( WIDE("Fantasy Football/4th Down") )( PTRSZVAL psv )
{
	struct attract_control *ac = (struct attract_control *)psv;
	return ac->pc;
}

static void DrawScoreText( PSI_CONTROL pc )
{

}

static PTRSZVAL OnCreateControl(WIDE( "Fantasy Football/Scoreboard" ))(PSI_CONTROL parent,S_32 x,S_32 y,_32 w,_32 h)
{
	struct attract_control *ac;
	ac = New( struct attract_control );

	ac->playing = FALSE;
	ac->attract = TRUE;
	ac->pc = MakeNamedControl( parent, WIDE("FF_Attract"), x, y, w, h, 0 );
	{
		ValidatedControlData( struct attract_control **, FF_Attract.TypeID, ppac, ac->pc );
		(*ppac) = ac;
	}
	ac->file = ffmpeg_LoadFile( ffl.scoreboard.movie_name
								, NULL, 0
								, ac->pc
								, NULL, 0
								, EndScoreboard, (PTRSZVAL)ac
								, NULL );
	ffmpeg_SetPrePostDraw( ac->file, NULL, DrawScoreText );
	return (PTRSZVAL)ac;
}

static PSI_CONTROL OnGetControl( WIDE("Fantasy Football/Scoreboard") )( PTRSZVAL psv )
{
	struct attract_control *ac = (struct attract_control *)psv;
	return ac->pc;
}




static LOGICAL CPROC PressSomeKey( PTRSZVAL psv, _32 key_code )
{
	static _32 _tick, tick;
	static int reset = 0;
	static int reset2 = 0;
	static int reset3 = 0;
	static int reset4 = 0;
	//TEXTCHAR key = GetKeyText( key_code );

	tick = timeGetTime();
	//lprintf( "got key %08x  (%d,%c)  %d ", key_code, key, key, tick - _tick );
	if( !_tick || ( _tick < ( tick - 2000 ) ) )
	{
		//lprintf( "late enough" );
		reset4 = 0;
		reset3 = 0;
		reset2 = 0;
		reset = 0;
		//ffl.begin_card = 0;
		ffl.number_collector = 0;
	}
	_tick = tick;

	{
		//lprintf( "continue sequence... begin new collections" );
		if( psv >= 0 && psv <= 9 )
		{
			// reset to new value
			ffl.number_collector = ( ffl.number_collector * 10 ) + psv;
			//ffl.value_collector[ffl.value_collect_index++] = key;
			//ffl.value_collector[ffl.value_collect_index] = 0;
			//lprintf( "new value %d (%s)", ffl.number_collector, ffl.value_collector );
			if( ffl.attract_mode )
			{
				if( ffl.number_collector == ffl.enable_code )
				{
					ffl.number_collector = 0;
					ShellSetCurrentPage( GetCommonParent( ffl.grid.control.pc ), "1st down" );
				}

			}
			else
			{
				if( ffl.number_collector == ffl.enable_code )
				{
					ShellSetCurrentPage( GetCommonParent( ffl.grid.control.pc ), "first" );
					ffl.number_collector = 0;
				}
			}
		}
		/*
		else if( key == ffl.card_begin_char )
		{
			//lprintf( "Begin swipe..." );
			if( !ffl.attract_mode )
			{
				ffl.begin_card = 1;
				ffl.number_collector = 0;
				ffl.value_collect_index = 0;
				//ffl.value_collector[ffl.value_collect_index] = 0;
			}
		}
		else if( key == ffl.card_end_char ) // '?'
		{
			//lprintf( "end card with (%s)", ffl.value_collector );
			if( !ffl.attract_mode )
			{
				//PickPrize();
			}
		}
		*/
	}
	return TRUE;
}

void InitKeys( void )
{
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

	BindEventToKey( NULL, KEY_PAD_0, 0, PressSomeKey, (PTRSZVAL)0 );
	BindEventToKey( NULL, KEY_PAD_1, 0, PressSomeKey, (PTRSZVAL)1 );
	BindEventToKey( NULL, KEY_PAD_2, 0, PressSomeKey, (PTRSZVAL)2 );
	BindEventToKey( NULL, KEY_PAD_3, 0, PressSomeKey, (PTRSZVAL)3 );
	BindEventToKey( NULL, KEY_PAD_4, 0, PressSomeKey, (PTRSZVAL)4 );
	BindEventToKey( NULL, KEY_PAD_5, 0, PressSomeKey, (PTRSZVAL)5 );
	BindEventToKey( NULL, KEY_PAD_6, 0, PressSomeKey, (PTRSZVAL)6 );
	BindEventToKey( NULL, KEY_PAD_7, 0, PressSomeKey, (PTRSZVAL)7 );
	BindEventToKey( NULL, KEY_PAD_8, 0, PressSomeKey, (PTRSZVAL)8 );
	BindEventToKey( NULL, KEY_PAD_9, 0, PressSomeKey, (PTRSZVAL)9 );

	BindEventToKey( NULL, KEY_INSERT, 0, PressSomeKey, (PTRSZVAL)0 );
	BindEventToKey( NULL, KEY_END, 0, PressSomeKey, (PTRSZVAL)1 );
	BindEventToKey( NULL, KEY_DOWN, 0, PressSomeKey, (PTRSZVAL)2 );
	BindEventToKey( NULL, KEY_PGDN, 0, PressSomeKey, (PTRSZVAL)3 );
	BindEventToKey( NULL, KEY_LEFT, 0, PressSomeKey, (PTRSZVAL)4 );
	BindEventToKey( NULL, KEY_CENTER, 0, PressSomeKey, (PTRSZVAL)5 );
	BindEventToKey( NULL, KEY_RIGHT, 0, PressSomeKey, (PTRSZVAL)6 );
	BindEventToKey( NULL, KEY_HOME, 0, PressSomeKey, (PTRSZVAL)7 );
	BindEventToKey( NULL, KEY_UP, 0, PressSomeKey, (PTRSZVAL)8 );
	BindEventToKey( NULL, KEY_PGUP, 0, PressSomeKey, (PTRSZVAL)9 );

	BindEventToKey( NULL, KEY_5, KEY_MOD_SHIFT, PressSomeKey, (PTRSZVAL)10 );

	BindEventToKey( NULL, KEY_SEMICOLON, 0, PressSomeKey, (PTRSZVAL)10 );
	BindEventToKey( NULL, KEY_SLASH, KEY_MOD_SHIFT, PressSomeKey, (PTRSZVAL)11 );
}
