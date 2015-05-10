//#define NO_DB_TEST
#define USE_RENDER_INTERFACE ffl.pdi
#define USE_IMAGE_INTERFACE ffl.pii
#include <stdhdrs.h>
#include <network.h>
#include <filemon.h>
#include <pssql.h>
#include <sack_system.h>
#include <render.h>
#include <keybrd.h>
#include <sexpat/expat.h>
#include "../../contrib/genx/genx.h"
//#include <shlwapi.h>
//#include <shellapi.h>
#include <ShlObj.h>

static CTEXTSTR create_tracking = 
	"create table ctc_player_tracking "
	"(plrid int(11)"
	", system_name varchar(128)"
	", player_name varchar(128)"
	", prize_id varchar(12)"
	", prize_name varchar(64)"
	", swipe_at_literal varchar(64)"
	", swipe_at date"
	", INDEX plrkey (plrid)"
	", INDEX prizekey (prize_id)"
	", INDEX syskey(system_name))";

struct keycode
{
	_32 key;
	_32 key2;
};

struct loaded_player
{
	TEXTSTR timestamp;
	TEXTSTR cardNum;
	TEXTSTR playerName;
	TEXTSTR prizeNum;
	TEXTSTR prizeDesc;
};

struct loaded_prize
{
	TEXTSTR remaining;
	TEXTSTR value;
	TEXTSTR desc;
};

enum loaded_player_state
{
	lps_nostate,
	lps_date,
	lps_cardNum,
	lps_playerName,
	lps_prizeNum,
	lps_prizeDesc,
	lps_remaining,
	lps_value,
	lps_desc,
};
struct xml_userdata {
	XML_Parser xp;
	LOGICAL get_card;
	LOGICAL get_date;
	LOGICAL got_player;
	LOGICAL this_player; // state whether this player card is the one we want.
	int swiped_today;
	int skip_entries;
	int entries;
	TEXTSTR card;
	int cardlen;
	SYSTEMTIME now;
	SYSTEMTIME swipe_at;

	enum loaded_player_state load_state;
	struct loaded_player *loading;
	struct loaded_prize *loading_prize;

};

static struct fantasy_football_local
{
	PMONITOR file_monitor;
	PODBC user_tracking;
	CTEXTSTR sysname;
	TEXTCHAR card_end_char;
	TEXTCHAR card_begin_char;
	_64 enable_code;

	LOGICAL attract_mode;
	_32 number_collector;
	TEXTCHAR value_collector[256];
	int value_collect_index;
	TEXTCHAR name_collector[256];
	int name_collect_index;
	int collect_name;
	int collect_card;
	int card_collected;
	PRENDER_INTERFACE pdi;
	PIMAGE_INTERFACE pii;


	PRENDERER r;
	Image banner_image;
	PRENDERER r_standby;
	Image standby_image;
	_32 timer;  // not zero if banner is showing.
	_32 timer_standby;
	TEXTCHAR path[MAX_PATH];
	PTASK_INFO task;
	TEXTSTR player_id;
	TEXTSTR player_name;
	int allow_swipe;
	int in_replay;
	PLIST xml_players;
	struct {
		genxWriter w;
		genxElement game;
		genxElement prizeGroups;
		genxElement prizeGroup;
		genxElement prizes;
		genxElement prize;
		genxElement remaining;
		genxElement value;
		genxElement desc;
		genxElement extraPrizes;
		genxElement playerLog;
		genxElement player;
		genxElement timestamp;
		genxElement cardNum;
		genxElement playerName;
		genxElement prizeNum;
		genxElement prizeDesc;
	} writer;
	PLIST loaded_prizes;
} ffl;

#ifndef INPUT_MOUSE
#  define INPUT_MOUSE 0
#  define INPUT_KEYBOARD 1
#  define INPUT_HARDWARE 2
#endif

/*
 typedef struct tagMOUSEINPUT {
  LONG      dx;
  LONG      dy;
  DWORD     mouseData;
  DWORD     dwFlags;
  DWORD     time;
  ULONG_PTR dwExtraInfo;
  } MOUSEINPUT, *PMOUSEINPUT;

 typedef struct tagKEYBDINPUT {
  WORD      wVk;
  WORD      wScan;
  DWORD     dwFlags;
  DWORD     time;
  ULONG_PTR dwExtraInfo;
} KEYBDINPUT, *PKEYBDINPUT;

 typedef struct tagINPUT {
  DWORD type;
  union {
    MOUSEINPUT    mi;
    KEYBDINPUT    ki;
    HARDWAREINPUT hi;
  };
} INPUT, *PINPUT;
*/

static void InitGenX( void )
{
	genxStatus status;
	ffl.writer.w = genxNew(NULL,NULL,NULL);
#define DeclElem( name )  	ffl.writer.name = genxDeclareElement( ffl.writer.w, NULL, (constUtf8)#name, &status );
	DeclElem( game );
	DeclElem( prizeGroups );
	DeclElem( prizeGroup );
	DeclElem( prizes );
	DeclElem( prize );
	DeclElem( remaining );
	DeclElem( value );
	DeclElem( desc );
	DeclElem( extraPrizes );
	DeclElem( playerLog );
	DeclElem( player );
	DeclElem( timestamp );
	DeclElem( cardNum );
	DeclElem( playerName );
	DeclElem( prizeNum );
	DeclElem( prizeDesc );
}

static void WriteXML( void )
{
	TEXTCHAR tmpname[256];
	FILE *file;
	if( !ffl.writer.w )
		InitGenX();

	snprintf( tmpname, 256, "%s/Crack the Code Game State.xml", ffl.path );
	file = fopen( tmpname, "wt" );
	genxStartDocFile( ffl.writer.w, file );
	genxPI( ffl.writer.w, "xml", "version=\"1.0\" encoding=\"utf-8\"" );
	//genxAddText(ffl.writer.w, (constUtf8)"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" );

	genxStartElement( ffl.writer.game );
	genxAddText(ffl.writer.w, (constUtf8)"\n  ");
	genxStartElement( ffl.writer.prizeGroups );
	genxAddText(ffl.writer.w, (constUtf8)"\n    ");
	genxStartElement( ffl.writer.prizeGroup );
	genxAddText(ffl.writer.w, (constUtf8)"\n      ");
	genxStartElement( ffl.writer.prizes );
	{
		INDEX idx;
		struct loaded_prize *prize;
		LIST_FORALL( ffl.loaded_prizes, idx, struct loaded_prize *, prize )
		{
			genxAddText(ffl.writer.w, (constUtf8)"\n        ");
			genxStartElement( ffl.writer.prize );
			genxAddText(ffl.writer.w, (constUtf8)"\n          ");
			genxStartElement( ffl.writer.remaining );
			genxAddText(ffl.writer.w, (constUtf8)prize->remaining);
			genxEndElement( ffl.writer.w );
			genxAddText(ffl.writer.w, (constUtf8)"\n          ");
			genxStartElement( ffl.writer.value );
			genxAddText(ffl.writer.w, (constUtf8)prize->value);
			genxEndElement( ffl.writer.w );
			genxAddText(ffl.writer.w, (constUtf8)"\n          ");

			genxStartElement( ffl.writer.desc );
			//genxAddText(ffl.writer.w, (constUtf8));
			genxEndElement( ffl.writer.w );
			genxAddText(ffl.writer.w, (constUtf8)"\n        ");
			genxEndElement( ffl.writer.w );
			Release( prize->remaining );
			Release( prize->value );
			if( prize->desc )
				Release( prize->desc );
			Release( prize );
		}
		DeleteList( &ffl.loaded_prizes );
	}
	genxAddText(ffl.writer.w, (constUtf8)"\n      ");
	genxEndElement( ffl.writer.w );
	genxAddText(ffl.writer.w, (constUtf8)"\n    ");
	genxEndElement( ffl.writer.w );
	genxAddText(ffl.writer.w, (constUtf8)"\n  ");
	genxEndElement( ffl.writer.w );
	genxAddText(ffl.writer.w, (constUtf8)"\n  ");

	genxStartElement( ffl.writer.extraPrizes );
	genxAddText(ffl.writer.w, (constUtf8)"0");
	genxEndElement( ffl.writer.w );
	genxAddText(ffl.writer.w, (constUtf8)"\n  ");

	genxStartElement( ffl.writer.playerLog );

	{
		INDEX idx;
		LOGICAL big_prize_claimed = FALSE;
		struct loaded_player *player;
		LIST_FORALL( ffl.xml_players, idx, struct loaded_player *, player )
		{
			if( big_prize_claimed 
				|| StrCmp( player->prizeNum, "1000000" ) != 0 )
			{
				Release( player->cardNum );
				Release( player->playerName );
				Release( player->prizeDesc );
				Release( player->prizeNum );
				Release( player->timestamp );
				Release( player );
				continue;
			}
			big_prize_claimed = TRUE;
			genxAddText(ffl.writer.w, (constUtf8)"\n    ");

			genxStartElement( ffl.writer.player );
			genxAddText(ffl.writer.w, (constUtf8)"\n      ");

			genxStartElement( ffl.writer.timestamp );
			genxAddText(ffl.writer.w, (constUtf8)player->timestamp);
			genxEndElement( ffl.writer.w );
			genxAddText(ffl.writer.w, (constUtf8)"\n      ");

			genxStartElement( ffl.writer.cardNum );
			genxAddText(ffl.writer.w, (constUtf8)player->cardNum);
			genxEndElement( ffl.writer.w );
			genxAddText(ffl.writer.w, (constUtf8)"\n      ");

			genxStartElement( ffl.writer.playerName );
			genxAddText(ffl.writer.w, (constUtf8)player->playerName);
			genxEndElement( ffl.writer.w );
			genxAddText(ffl.writer.w, (constUtf8)"\n      ");

			genxStartElement( ffl.writer.prizeNum );
			genxAddText(ffl.writer.w, (constUtf8)player->prizeNum);
			genxEndElement( ffl.writer.w );
			genxAddText(ffl.writer.w, (constUtf8)"\n      ");

			genxStartElement( ffl.writer.prizeDesc );
			genxAddText(ffl.writer.w, (constUtf8)player->prizeDesc);
			genxEndElement( ffl.writer.w );
			genxAddText(ffl.writer.w, (constUtf8)"\n    ");

			genxEndElement( ffl.writer.w );
			Release( player->cardNum );
			Release( player->playerName );
			Release( player->prizeDesc );
			Release( player->prizeNum );
			Release( player->timestamp );
			Release( player );
		}
		DeleteList( &ffl.xml_players );
	}
	genxAddText(ffl.writer.w, (constUtf8)"\n  ");
	genxEndElement( ffl.writer.w );
	genxAddText(ffl.writer.w, (constUtf8)"\n");
	genxEndElement( ffl.writer.w );

	genxEndDocument( ffl.writer.w );

	fclose( file );
}

static void GenerateInput( int key1, int key2 )
{
	INPUT *inputs = NewArray( INPUT, 4 );
	UINT result;
	inputs[0].type = INPUT_KEYBOARD;
	inputs[1].type = INPUT_KEYBOARD;
	inputs[2].type = INPUT_KEYBOARD;
	inputs[3].type = INPUT_KEYBOARD;

   if( key2 && key1 != key2 )
	{
		inputs[0].ki.wVk = key2;  // VK_ ;
		inputs[0].ki.wScan = 0;  // scancode of key...
		inputs[0].ki.dwFlags = 0; // KEYEVENTF_UNICODE, KEYEVENTF_SCANCODE, KEYEVENTF_KEYUP, KEYEVENTF_EXTENDEDKEY
		inputs[0].ki.time = 0; // event timestamp in milliseconds... if 0, system fills it in.
		inputs[0].ki.dwExtraInfo = 0;
		inputs[1].ki.wVk = key1;  // VK_ ;
		inputs[1].ki.wScan = 0;  // scancode of key...
		inputs[1].ki.dwFlags = 0; // KEYEVENTF_UNICODE, KEYEVENTF_SCANCODE, KEYEVENTF_KEYUP, KEYEVENTF_EXTENDEDKEY
		inputs[1].ki.time = 0; // event timestamp in milliseconds... if 0, system fills it in.
		inputs[1].ki.dwExtraInfo = 0;
		inputs[2].ki.wVk = key1; // VK_ ;
		inputs[2].ki.wScan = 0;  // scancode of key...
		inputs[2].ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_UNICODE, KEYEVENTF_SCANCODE, KEYEVENTF_KEYUP, KEYEVENTF_EXTENDEDKEY
		inputs[2].ki.time = 0; // event timestamp in milliseconds... if 0, system fills it in.
		inputs[2].ki.dwExtraInfo = 0;
		inputs[3].ki.wVk = key2;  // VK_ ;
		inputs[3].ki.wScan = 0;  // scancode of key...
		inputs[3].ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_UNICODE, KEYEVENTF_SCANCODE, KEYEVENTF_KEYUP, KEYEVENTF_EXTENDEDKEY
		inputs[3].ki.time = 0; // event timestamp in milliseconds... if 0, system fills it in.
		inputs[3].ki.dwExtraInfo = 0;
      result = 4;
	}
	else
	{
		inputs[0].ki.wVk = key1;  // VK_ ;
		inputs[0].ki.wScan = 0;  // scancode of key...
		inputs[0].ki.dwFlags = 0; // KEYEVENTF_UNICODE, KEYEVENTF_SCANCODE, KEYEVENTF_KEYUP, KEYEVENTF_EXTENDEDKEY
		inputs[0].ki.time = 0; // event timestamp in milliseconds... if 0, system fills it in.
		inputs[0].ki.dwExtraInfo = 0;
		inputs[1].ki.wVk = key1;  // VK_ ;
		inputs[1].ki.wScan = 0;  // scancode of key...
		inputs[1].ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_UNICODE, KEYEVENTF_SCANCODE, KEYEVENTF_KEYUP, KEYEVENTF_EXTENDEDKEY
		inputs[1].ki.time = 0; // event timestamp in milliseconds... if 0, system fills it in.
		inputs[1].ki.dwExtraInfo = 0;
      result = 2;
	}

	result = SendInput( result, inputs, sizeof( INPUT ) );

	if( result == 0 )
	{
      // input is blocked somehow.
	}


   Deallocate( PINPUT, inputs );
}

void ReleaseShift( int key1 )
{
	INPUT inputs[1];
	UINT result;
	inputs[0].type = INPUT_KEYBOARD;

		inputs[0].ki.wVk = key1;  // VK_ ;
		inputs[0].ki.wScan = 0;  // scancode of key...
		inputs[0].ki.dwFlags = KEYEVENTF_KEYUP|(key1==VK_RSHIFT?KEYEVENTF_EXTENDEDKEY:0); // KEYEVENTF_UNICODE, KEYEVENTF_SCANCODE, KEYEVENTF_KEYUP, KEYEVENTF_EXTENDEDKEY
		inputs[0].ki.time = 0; // event timestamp in milliseconds... if 0, system fills it in.
		inputs[0].ki.dwExtraInfo = 0;

	result = SendInput( 1, inputs, sizeof( INPUT ) );

	if( result == 0 )
	{
      // input is blocked somehow.
	}

}

void PressShift( int key1 )
{
	INPUT inputs[1];
	UINT result;
	inputs[0].type = INPUT_KEYBOARD;

		inputs[0].ki.wVk = key1;  // VK_ ;
		inputs[0].ki.wScan = 0;  // scancode of key...
		inputs[0].ki.dwFlags = (key1==VK_RSHIFT?KEYEVENTF_EXTENDEDKEY:0); // KEYEVENTF_UNICODE, KEYEVENTF_SCANCODE, KEYEVENTF_KEYUP, KEYEVENTF_EXTENDEDKEY
		inputs[0].ki.time = 0; // event timestamp in milliseconds... if 0, system fills it in.
		inputs[0].ki.dwExtraInfo = 0;

	result = SendInput( 1, inputs, sizeof( INPUT ) );

	if( result == 0 )
	{
      // input is blocked somehow.
	}

}

PTRSZVAL CPROC GenerateApplicationStrokes( PTHREAD thread )
{
	int n;
	lprintf( "Short pause.." );
	if( CanSwipe() )
	{
		Sleep( 100 );
		lprintf( "generate percent" );
		GenerateInput( KEY_2, KEY_SHIFT );
		//GenerateInput( KEY_5, KEY_SHIFT );
		lprintf( "player name is : %s", ffl.player_name );
		if( ffl.player_name )
			for( n = 0; ffl.player_name[n]; n++ )
			{
				lprintf( "character is %c", ffl.player_name[n] );
				if( ffl.player_name[n] >= 'A' && ffl.player_name[n] <= 'Z' )
					GenerateInput( ffl.player_name[n], KEY_SHIFT );
				else if( ffl.player_name[n] >= 'a' && ffl.player_name[n] <= 'z' )
					GenerateInput( ffl.player_name[n] - ( 'a' - 'A' ), 0 );
				else
					GenerateInput( ffl.player_name[n], 0 );
			}
		lprintf( "generate caret" );
		GenerateInput( KEY_6, KEY_SHIFT );
		if( ffl.player_id )
			for( n = 0; ffl.player_id[n]; n++ )
			{
				lprintf( "character is %c", ffl.player_id[n] );
				GenerateInput( ffl.player_id[n], 0 );
			}
		//GenerateInput( KEY_SLASH, KEY_SHIFT );
		GenerateInput( KEY_3, KEY_SHIFT );
	}
	ffl.in_replay = 0;
	ffl.value_collect_index = 0;
	ffl.card_collected = 0;
	return 0;
}

static void DoGenerateApplicationStrokes( void )
{
	ffl.in_replay = 1;
	ThreadTo( GenerateApplicationStrokes, 0 );
}


static LOGICAL SetP( TEXTSTR *p, const XML_Char **atts )
{
	if( p[0] )
		Deallocate( TEXTSTR, p[0] );
	if( p[1] )
		Deallocate( TEXTSTR, p[1] );
	if( atts[0] )
	{
		p[0] = DupCStr( atts[0] );
		p[1] = DupCStr( atts[1] );
		return TRUE;
	}
	else
		return FALSE;
}


void XMLCALL start_tags( void *UserData
							  , const XML_Char *name
							  , const XML_Char **atts )
{
	struct xml_userdata *userdata = (struct xml_userdata *)UserData;
	TEXTSTR p[2];
	p[0] = NULL;
	p[1] = NULL;
	//lprintf( WIDE("begin a tag %s with..."), name );
	if( StrCmp( name, "player" ) == 0 )
	{
		struct loaded_player *load = New( struct loaded_player );
		AddLink( &ffl.xml_players, load );
		MemSet( load, NULL, sizeof( struct loaded_player ) );
		userdata->loading = load;
	}
	if( StrCmp( name, "cardNum" ) == 0 )
	{
		userdata->load_state = lps_cardNum;
		userdata->get_card = TRUE;
	}
	if( StrCmp( name, "timestamp" ) == 0 )
	{
		userdata->load_state = lps_date;
		userdata->get_date = TRUE;
	}
	if( StrCmp( name, "playerName" ) == 0 )
	{
		userdata->load_state = lps_playerName;
	}
	if( StrCmp( name, "prizeNum" ) == 0 )
	{
		userdata->load_state = lps_prizeNum;
	}
	if( StrCmp( name, "prizeDesc" ) == 0 )
	{
		userdata->load_state = lps_prizeDesc;
	}

	if( StrCmp( name, "remaining" ) == 0 )
	{
		userdata->load_state = lps_remaining;
	}
	if( StrCmp( name, "value" ) == 0 )
	{
		userdata->load_state = lps_value;
	}
	if( StrCmp( name, "desc" ) == 0 )
	{
		userdata->load_state = lps_desc;
	}

	if( StrCmp( name, "prize" ) == 0 )
	{
		struct loaded_prize *load = New( struct loaded_prize );
		AddLink( &ffl.loaded_prizes, load );
		MemSet( load, NULL, sizeof( struct loaded_prize ) );

		userdata->loading_prize = load;
	}

	/*
	while( SetP( p, atts ) )
	{
		
		lprintf( WIDE("begin a attrib %s=%s with..."), p[0], p[1] );
	}
	*/
}

void XMLCALL end_tags( void *UserData
							, const XML_Char *name )
{
	struct xml_userdata *userdata = (struct xml_userdata *)UserData;
	if( StrCmp( name, "player" ) == 0 )
	{
		userdata->entries++;
		if( userdata->entries > userdata->skip_entries )
		{
			SQLCommandf( ffl.user_tracking
				, "insert into ctc_player_tracking (plrid,system_name,swipe_at,prize_id,prize_name,player_name,swipe_at_literal) values ('%*.*s','%s','%04d%02d%02d','%s','%s','%s','%s')"
				, userdata->cardlen
				, userdata->cardlen
				, userdata->card
				, ffl.sysname
				, userdata->swipe_at.wYear
				, userdata->swipe_at.wMonth
				, userdata->swipe_at.wDay 
				, userdata->loading->prizeNum
				, userdata->loading->prizeDesc
				, userdata->loading->playerName
				, userdata->loading->timestamp
				);
		}
	}
}

CTEXTSTR months[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

void handle_data (void *userData,
                                                  const XML_Char *s,
                                                  int len)
{
	struct xml_userdata *userdata = (struct xml_userdata *)userData;
	TEXTCHAR tmp[256];
	if( s[0] == '\n' && len == 1 )
		return;
	snprintf( tmp, 256, "%*.*s", len, len, s );
	switch( userdata->load_state )
	{
	case lps_date:
		userdata->loading->timestamp = StrDup( tmp );
		break;
	case lps_playerName:
		userdata->loading->playerName = StrDup( tmp );
		break;
	case lps_cardNum:
		userdata->loading->cardNum = StrDup( tmp );
		break;
	case lps_prizeDesc:
		userdata->loading->prizeDesc = StrDup( tmp );
		break;
	case lps_prizeNum:
		userdata->loading->prizeNum = StrDup( tmp );
		break;

	case lps_remaining:
		userdata->loading_prize->remaining = StrDup( tmp );
		break;
	case lps_value:
		userdata->loading_prize->value = StrDup( tmp );
		break;
	case lps_desc:
		userdata->loading_prize->desc = StrDup( tmp );
		break;
	}
	userdata->load_state = lps_nostate;

	if( userdata->get_card )
	{
		userdata->get_card = FALSE;

		if( userdata->card )
			Release( userdata->card );
		userdata->card = StrDup( s );
		userdata->cardlen = len;

		if( StrCmpEx( s, ffl.player_id, len ) == 0 )
		{
			userdata->got_player = TRUE;
			userdata->this_player = TRUE;
		}
		else
			userdata->this_player = FALSE;
	}
	if( userdata->get_date )
	{
		int n;
		CTEXTSTR parse = s;
		int month, year;
		int day, hr, mn, sc;
		// Mon Apr 6 17:00:24 GMT-0700 2015
		userdata->get_date = 0;
		parse += 4;
		for( n = 0; n < 12; n++ )
			if( StrCaseCmpEx( parse, months[n], 3 ) == 0 )
			{
				month = n + 1;
				break;
			}
		parse += 4;
		sscanf( parse, "%d %d:%d:%d", &day, &hr, &mn, &sc );
		{
			CTEXTSTR tmp;
			tmp = strchr( parse, ' ' );
			if( tmp )
				parse = tmp + 1;
			tmp = strchr( parse, ' ' );
			if( tmp )
				parse = tmp + 1;
			tmp = strchr( parse, ' ' );
			if( tmp )
				parse = tmp + 1;
			sscanf( parse, "%d", &year );
		}
		userdata->swipe_at.wYear = year;
		userdata->swipe_at.wMonth = month;
		userdata->swipe_at.wDay = day;

		if( userdata->now.wYear == year 
			&& userdata->now.wMonth == month 
			&& userdata->now.wDay == day )
		{
			userdata->swiped_today++;
		}
	}
	//lprintf( "data is %s", s );
	//lprintf( "limited data is %*.*s", len, len, s );
}
//-------------------------------------------------------------------------
static struct {
	CTEXTSTR pFile;
	_32 nLine;
} current_loading;
#if defined( _DEBUG ) || defined( _DEBUG_INFO )
void * MyAllocate( size_t s ) { return AllocateEx( s, current_loading.pFile, current_loading.nLine ); }
#else
void * MyAllocate( size_t s ) { return AllocateEx( s ); }
#endif
void *MyReallocate( void *p, size_t s ) { return Reallocate( p, s ); }
void MyRelease( void *p ) { Release( p ); }

static XML_Memory_Handling_Suite XML_memhandler;// = { MyAllocate, MyReallocate, MyRelease };
//-------------------------------------------------------------------------

void ParseXML( POINTER buffer, size_t size, int *first_swipe, int *today_swipes, LOGICAL bQuery )
{
	POINTER xml_buffer;
	struct xml_userdata userdata;
	//lprintf( WIDE("Beginning parse frame...") );
	XML_memhandler.malloc_fcn = MyAllocate;
	XML_memhandler.realloc_fcn = MyReallocate;
	XML_memhandler.free_fcn = MyRelease;

	userdata.xp = XML_ParserCreate_MM( NULL, &XML_memhandler, NULL );
	userdata.got_player = FALSE;
	userdata.get_card = FALSE;
	userdata.get_date = FALSE;
	userdata.swiped_today = 0;
	userdata.entries = 0;
	userdata.card = NULL;
	// don't skip entries on a system anymore...
	// each XML will be read entirely.
	if( 0 )
	{
		CTEXTSTR *result = NULL;
		if( SQLRecordQueryf( ffl.user_tracking, NULL, &result, NULL
						, "select count(*) from ctc_player_tracking where system_name='%s'", ffl.sysname )
			&& result )
		{
			int n = result[0]?atoi( result[0] ):0;
			userdata.skip_entries = n;
		}
		SQLEndQuery( ffl.user_tracking );
	}


	GetLocalTime( &userdata.now );

	XML_SetElementHandler( userdata.xp, start_tags, end_tags );
	XML_SetUserData( userdata.xp, &userdata );
	XML_SetCharacterDataHandler( userdata.xp, handle_data );
	xml_buffer = XML_GetBuffer( userdata.xp, size );
	MemCpy( xml_buffer, buffer, size );
	if( XML_ParseBuffer( userdata.xp, size, TRUE ) == XML_STATUS_ERROR )
	{
		lprintf( WIDE( "Error in XML parse %d  at line %")_size_f WIDE("(%")_size_f WIDE(")" ), XML_GetErrorCode( userdata.xp ),XML_GetCurrentLineNumber( userdata.xp ), XML_GetCurrentColumnNumber( userdata.xp ) );
	}
	XML_ParserFree( userdata.xp );
	userdata.xp = 0;

	if( bQuery )
	{
		CTEXTSTR *results = NULL;
		if( SQLRecordQueryf( ffl.user_tracking, NULL, &results, NULL
			, "select count(*) from ctc_player_tracking where plrid='%s'"
			, ffl.player_id ) 
			&& results )
		{
			if( results[0] && results[0][0] != '0' )
				(*first_swipe) = 0;
			else
				(*first_swipe) = 1;
		}
		SQLEndQuery( ffl.user_tracking );

		if( SQLRecordQueryf( ffl.user_tracking, NULL, &results, NULL
			, "select plrid,player_name,prize_id,prize_name,swipe_at_literal from ctc_player_tracking where prize_id='1000000'"
			) 
			&& results )
		{
			struct loaded_player *player = New( struct loaded_player );
			player->cardNum = StrDup( results[0] );
			player->playerName = StrDup( results[1] );
			player->prizeNum = StrDup( results[2] );
			player->prizeDesc = StrDup( results[3] );
			player->timestamp = StrDup( results[4] );
			AddLink( &ffl.xml_players, player );
		}
		SQLEndQuery( ffl.user_tracking );
		if( SQLRecordQueryf( ffl.user_tracking, NULL, &results, NULL
			, "select count(*) from ctc_player_tracking where plrid='%s' and swipe_at=%04d%02d%02d"
			, ffl.player_id
			, userdata.now.wYear
			, userdata.now.wMonth
			, userdata.now.wDay
			) 
			&& results )
		{
			if( results[0] )
				(*today_swipes) = atoi( results[0] );
			else
				(*today_swipes) = 0;
		}
		SQLEndQuery( ffl.user_tracking );
	}
	//lprintf( WIDE("Parse done...") );
	//return l.frame;
}

static void ReadXML( int *first, int *today, LOGICAL bQuery )
{
	POINTER buffer;
	PTRSZVAL size;
	TEXTSTR delete_filename = NULL;
	TEXTCHAR filename[MAX_PATH]; // assume this is the name until later
	//SHGetFolderPath( NULL, CSIDL_DESKTOP, NULL, SHGFP_TYPE_CURRENT, path );

	snprintf( filename, MAX_PATH, "%s/Crack the Code Game State.xml", ffl.path );
	// enter critical section!
	size = 0;
	{
		FILE *file_read = sack_fopen( 0, filename, "rt" );
		if( file_read )
		{
			int zz;
			sack_fseek( file_read, 0, SEEK_END );
			zz = ftell( file_read );
			sack_fseek( file_read, 0, SEEK_SET );

			size = zz;
			buffer = Allocate( zz );
			sack_fread( buffer, 1, zz, file_read );
			sack_fclose( file_read );
			//lprintf( WIDE( "loaded font blob %s %d %p" ), file, zz, buffer );
		}
	}

	if( buffer && size )
	{
		ParseXML( buffer, size, first, today, bQuery );
		Release( buffer );
	}

}

static void CPROC redraw( PTRSZVAL psvUser, PRENDERER self )
{
	Image out = GetDisplayImage( ffl.r );
	//lprintf( "Draw image" );
	BlotScaledImage( out, ffl.banner_image );
	UpdateDisplay( ffl.r );
}

static void CPROC redraw_standby( PTRSZVAL psvUser, PRENDERER self )
{
	Image out = GetDisplayImage( ffl.r_standby );
	//lprintf( "Draw image" );
	BlotScaledImage( out, ffl.standby_image );
	UpdateDisplay( ffl.r_standby );
}

static void CPROC hide_display( PTRSZVAL psv )
{
	lprintf( "hide" );
	HideDisplay( ffl.r );
	RemoveTimer( ffl.timer );
	ffl.timer = 0;
}

static void BannerNoSwipes( void )
{
	lprintf( "restore" );
	RestoreDisplay( ffl.r );
	Redraw( ffl.r );
	//{
	//	Image out = GetDisplayImage( ffl.r );
	//	BlotScaledImage( out, ffl.banner_image );
	//}
	ffl.timer = AddTimerEx( 3000, 0, hide_display, 0 );
	//while( ffl.timer )
	//	WakeableSleep( 100 );
}

static void CPROC StandbyDelay( PTRSZVAL psv )
{
	lprintf( "restore standby" );
	RestoreDisplay( ffl.r_standby );
	Redraw( ffl.r_standby );
	ffl.timer_standby = 0;
}

static void HideStandby( void )
{
	HideDisplay( ffl.r_standby );
	if( ffl.timer_standby )
	{
		RemoveTimer( ffl.timer_standby );
		ffl.timer_standby = 0;
	}
}

static void BannerStandby( void )
{
	lprintf( "need standby.." );
	if( !ffl.timer_standby )
	{
		//lprintf( "add timer to standby 500" );
		//ffl.timer_standby = AddTimerEx( 10, 0, StandbyDelay, 0 );
		StandbyDelay( 0 );
	}
}

static LOGICAL InAttract( void )
{
	FILE *file;
	TEXTCHAR filename[MAX_PATH]; // assume this is the name until later
	//SHGetFolderPath( NULL, CSIDL_DESKTOP, NULL, SHGFP_TYPE_CURRENT, path );
	snprintf( filename, MAX_PATH, "%s/Crack the Code Attract Check.txt", ffl.path );
	file = sack_fopen( 0, filename, "rt" );
	if( file )
	{
		TEXTCHAR buf[10];
		if( sack_fgets( buf, 10, file ) )
		{
			if( buf[0] == '0' )
			{
				sack_fclose( file );
				return FALSE;
			}
		}
		sack_fclose( file );
	}
	return TRUE;
}

static int CanSwipe( void )
{
	TEXTCHAR **results = NULL;
	int available_count = 0;
	S_32 slot_total = 0;
	S_32 table_total = 0;
	int first_swipe = 0;
	int today_swipe = 0;
	BannerStandby();
	//WakeableSleep( 3000 );
	if( SQLRecordQueryf( NULL, NULL, &results, NULL
						, "select PTNID,FIRSTNAME,LASTNAME from EMS.VW_AGAMING_PATRON where CARDID='%s' AND CARDSTATID=1"
						, ffl.value_collector ) 
		&& results 
		&& results[0]
		)
	{
		// should be 0 or 1....
		if( ffl.player_id )
		{
			Release( ffl.player_id );
		}
		ffl.player_id = StrDup( results[0] );
		if( ffl.player_name )
			Release( ffl.player_name );
		{
			static TEXTCHAR tmpbuf[256];
			snprintf( tmpbuf, 256, "%s %s", results[1], results[2] );
			ffl.player_name = StrDup( tmpbuf );
		}
	}
	else
	{
		ffl.player_id = "12345678";
		ffl.player_name = "No Player Found";
#ifdef NO_DB_TEST
	if( ffl.timer_standby )
		RemoveTimer( ffl.timer_standby );
	else
		HideStandby();

		SQLEndQuery( NULL );
		return 1;
#endif
	}
	SQLEndQuery( NULL );

	ReadXML( &first_swipe, &today_swipe, TRUE );
	WriteXML();

	if( first_swipe )
	{
		if( SQLRecordQueryf( NULL, NULL, &results, NULL
							, "select count(*) from EMS.VW_AGAMING_PATRON where CARDID='%s' AND SIGN_UP_DATE>='30-Apr-2015' AND CARDSTATID=1"
							, ffl.value_collector ) 
			&& results 
			&& results[0]
		  )
		{
			// should be 0 or 1....
			if( results[0][0] != '0' )
				available_count = 1;
		}
		SQLEndQuery( NULL );
	}

	if( SQLRecordQueryf( NULL, NULL, &results, NULL
	                   , "select sum(CREDIT_POINT) from EMS.VW_AGAMING_PATRON_SLOT_RATING where PTNID='%s'" 
					      //" AND TO_CHAR(DATE_TIME_OUT,'dd-MON-YYYY')=TO_CHAR(sysdate,'dd-MON-YYYY')"
						  , ffl.player_id )
	    && results && results[0] )
	{
		slot_total = atoi( results[0] );
	}
	SQLEndQuery( NULL );

	if( SQLRecordQueryf( NULL, NULL, &results, NULL
	                   , "select sum(THEO_WIN) from EMS.VW_AGAMING_PATRON_TABLE_RATING where PTNID='%s'" 
					      //" AND TO_CHAR(DATE_TIME_OUT,'dd-MON-YYYY')=TO_CHAR(sysdate,'dd-MON-YYYY')"
						  , ffl.player_id )
	    && results && results[0] )
	{
		table_total = atoi( results[0] );
	}
	SQLEndQuery( NULL );

	if( ffl.timer_standby )
		RemoveTimer( ffl.timer_standby );
	else
		HideStandby();

	if( ( slot_total ) >= 150 )
		available_count += 3;
	else if( ( slot_total ) >= 75 )
		available_count += 2;
	else if( ( slot_total ) >= 25 )
		available_count += 1;

	if( ( table_total ) >= 90 )
		available_count += 3;
	else if( ( table_total ) >= 45 )
		available_count += 2;
	else if( ( table_total ) >= 15 )
		available_count += 1;

	if( available_count > 3 )
		available_count = 3;
	available_count -= today_swipe;
	lprintf( "available count will be %d", available_count );
	if( available_count <= 0 )
	{
		BannerNoSwipes();
		return 0;
	}
	//?select * from EMS.VW_AGAMING_PATRON_SLOT_RATING where TO_CHAR(DATE_TIME_OUT,'dd-MON-YYYY')=TO_CHAR(sysdate,'dd-MON-YYYY')
	return available_count > 0;
}

static LOGICAL CPROC PressSomeKey( PTRSZVAL psv, _32 key_code )
{
	TEXTCHAR key;
	static int last_press_used;
	static int last_shift;
	static int keys_down;
	static int last_key;
	static int last_key_down;
	int can_swipe;

	if( IsKeyPressed( key_code ) )
	{
		last_key_down = TRUE;
		keys_down++;
	}
	else
	{
		last_key_down = FALSE;
		keys_down--;
		if( keys_down < 0 )
			keys_down = 0;
	}
	if( last_key_down && KEY_CODE( key_code ) == last_key )
	{

	}

	lprintf( "Keys pressed = %d %d %d %d %d %d %d"
		, keys_down, ffl.allow_swipe, !ffl.in_replay
		, ffl.card_collected, ffl.value_collect_index, !ffl.collect_card, !ffl.collect_name );
	can_swipe = TRUE;
	if( //ffl.allow_swipe 
		//&&
		!ffl.in_replay 
		&& ffl.card_collected 
		&& ffl.value_collect_index 
		&& !ffl.collect_card 
		&& !ffl.collect_name 
		&& !keys_down
		&& !ffl.timer 
		)
	{
		ffl.in_replay = 1;
		ThreadTo( GenerateApplicationStrokes, 0 );
		//DoGenerateApplicationStrokes();
		//ffl.value_collect_index = 0;
		//ffl.card_collected = 0;
	}

	if( InAttract() || ffl.in_replay || ffl.timer )
	{
		lprintf( "in attract or in replay; or has banner up" );
		return last_press_used = FALSE;
	}

	if( KEY_CODE( key_code ) == VK_LSHIFT )
		last_shift = VK_LSHIFT;
	if( KEY_CODE( key_code ) == VK_RSHIFT )
		last_shift = VK_RSHIFT;
	if( IsKeyPressed( key_code ) )
	{
		key = GetKeyText( key_code );

		lprintf( "key is %c(%d)(%x) %s", key, key, key, (KEY_MOD( key_code ) & KEY_MOD_SHIFT)?"shift":"noshft" );
		if( key == '%' || key == '^' )
		{
			return last_press_used = TRUE;
		}
		else if( key == ';' )
		{
			ffl.value_collect_index = 0;
			ffl.value_collector[0] = 0;
			ffl.collect_card = TRUE;
			return last_press_used = TRUE; // key used.
		}
		else if( key == '?' )
		{
			lprintf( "terminator for number %d", ffl.value_collect_index );
			if( ffl.value_collect_index )
			{
				ffl.card_collected = 1;
				ffl.collect_card = FALSE;
			}
			//else
				return last_press_used = TRUE; // 
		}
		else
		{
			if( ffl.collect_name )
			{
				lprintf( "collecting name..." );
				ffl.name_collector[ffl.name_collect_index++] = key;
				ffl.name_collector[ffl.name_collect_index] = 0;
				return last_press_used = TRUE; // key used.
			}
			else if( ffl.collect_card )
			{
				lprintf( "collecting card..." );
				ffl.value_collector[ffl.value_collect_index++] = key;
				ffl.value_collector[ffl.value_collect_index] = 0;
				return last_press_used = TRUE; // key used.
			}
			lprintf( "not collecting name or card...so pass the key" );
			return last_press_used = FALSE;
		}
	}
	{
		int r = last_press_used;
		last_press_used = FALSE;
		lprintf( "last key %d", r );
		return r;  // didn't actually use the key....
	}
}

void InitKeys( void )
{
	int n;
	for( n = 0; n < 256; n++ )
	{
		if( n == 0xA4 || n == 0xA5 || n == 0xA2 || n == 0xA3 )
			continue;
		BindEventToKey( NULL, n, KEY_MOD_ALL_CHANGES, PressSomeKey, (PTRSZVAL)0 );
		BindEventToKey( NULL, n, KEY_MOD_ALL_CHANGES|KEY_MOD_SHIFT, PressSomeKey, (PTRSZVAL)0 );
		//BindEventToKey( NULL, n, KEY_MOD_ALL_CHANGES, PressSomeKey, (PTRSZVAL)0 );
		//BindEventToKey( NULL, n, KEY_MOD_ALL_CHANGES|KEY_MOD_SHIFT, PressSomeKey, (PTRSZVAL)0 );
	}
}

static void CPROC TaskEnded( PTRSZVAL psv, PTASK_INFO task )
{
	ffl.task = NULL;
	exit(0);
}

ATEXIT( KillGame )
{
	if( ffl.task )
		TerminateProgram( ffl.task );
}


static int CPROC ChandleHandler(PTRSZVAL psv
											 , CTEXTSTR filepath
											 , int bDeleted)
{
	
	lprintf( "Change on %s %s", filepath, bDeleted?"deleted":"" );
	if( StrStr( filepath, "State.xml" ) )
	{
		lprintf( "match..." );
		//if( bDeleted )
		//	SQLCommandf( ffl.user_tracking, "delete from ctc_player_tracking where system_name='%s'", ffl.sysname );
		//else
		{
			int a = 0;
			int b = 0;
			ReadXML( &a, &b, FALSE );
		}
	}
	return TRUE;
}

SaneWinMain( argc, argv )
{
	ffl.pdi = GetDisplayInterface();
	ffl.pii = GetImageInterface();
#ifndef NO_DB_TEST
	if( !SQLCommandf( NULL, "SELECT 1 from EMS.VW_AGAMING_PATRON" ) )  // open oracle connection
	{
		MessageBox( NULL, "Failed to connect to oracle database", "Setup Error?", MB_OK );
		return 0;
	}
#endif
	ffl.user_tracking = SQLGetODBC( "MySQL" );
	
	NetworkStart();
	ffl.sysname = GetSystemName();
	{
		PTABLE table = GetFieldsInSQL( create_tracking, FALSE );
		CheckODBCTable( ffl.user_tracking, table, CTO_MERGE );
		DestroySQLTable( table );
		//SQLCommandf( ffl.user_tracking, "create table if not exists ctc_player_tracking (plrid int(11), system_name varchar(128), swipe_at date, INDEX plrkey (plrid), INDEX syskey(system_name))" );
	}

	SHGetFolderPath( NULL, CSIDL_DESKTOP, NULL, SHGFP_TYPE_CURRENT, ffl.path );
	lprintf( "Open monitor on %s", ffl.path );

	{
		int first_swipe, today_swipe;
		ReadXML( &first_swipe, &today_swipe, TRUE );
		WriteXML();
	}

	ffl.file_monitor = MonitorFiles( ffl.path, 500 );
	SetFileLogging( ffl.file_monitor, TRUE );
	AddFileChangeCallback( ffl.file_monitor, NULL, ChandleHandler, 0 );

	if( !IsPath( "../CrackTheCode" ) )
	{
		MessageBox( NULL, "Failed to locate game", "Installation error", MB_OK );
		return 0;
	}
	{
		TEXTSTR args[2];
		args[0] = "../CrackTheCode/CrackTheCode.exe";
		args[1] = NULL;
		ffl.task = LaunchPeerProgram( args[0], ffl.path, args, NULL, TaskEnded, 0 );
	}
	ffl.banner_image = LoadImageFile( "noswipes.jpg" );
	ffl.standby_image = LoadImageFile( "standby.jpg" );
	ffl.card_begin_char = '%';
	ffl.card_end_char = '?';
	
	{
		_32 w, h;
		GetDisplaySize( &w, &h );
		//ffl.r = OpenDisplaySizedAt( DISPLAY_ATTRIBUTE_NO_AUTO_FOCUS| DISPLAY_ATTRIBUTE_CHILD, w, h, 0, 0 );
		ffl.r = OpenDisplaySizedAt( DISPLAY_ATTRIBUTE_NO_AUTO_FOCUS| DISPLAY_ATTRIBUTE_CHILD|DISPLAY_ATTRIBUTE_TOPMOST, w, h, 0, 0 );
		MakeTopmost( ffl.r );
		SetRedrawHandler( ffl.r, redraw, 0 );	
		ffl.r_standby = OpenDisplaySizedAt( DISPLAY_ATTRIBUTE_NO_AUTO_FOCUS| DISPLAY_ATTRIBUTE_CHILD|DISPLAY_ATTRIBUTE_TOPMOST, w, h, 0, 0 );
		MakeTopmost( ffl.r_standby );
		SetRedrawHandler( ffl.r_standby, redraw_standby, 0 );	
	}
	InitKeys();
	while( 1 )
		WakeableSleep( 10000 );
	return 0;
}
EndSaneWinMain()

