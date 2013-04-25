
#include <stdhdrs.h>
#include <system.h>
#include <sqlgetoption.h>
#include <timers.h>
#include "../../InterShell/widgets/include/banner.h"

// fifth
// fourth
// third
// second
// first

static CTEXTSTR ords[] = { "Zeroth", "First", "Second", "Third", "Fourth", "Fifth", "Sixth", "Seventh", "Eighth", "Ninth", "Tenth" };

static struct {
	PLIST players;
   PLIST total_players;
	int nPlayers;

	CTEXTSTR local_banner_args;
	CTEXTSTR local_banner_noprompt_args;
	CTEXTSTR local_banner_final_args;
	CTEXTSTR local_banner_path;
	CTEXTSTR remote_banner_args;
	CTEXTSTR remote_banner_final_args;
	CTEXTSTR remote_banner_path;
   TEXTSTR remote_kill;
	CTEXTSTR launchpad_args;
	CTEXTSTR launchpad_path;
	struct {
		BIT_FIELD bResultYes : 1;
		BIT_FIELD bResult : 1;
		BIT_FIELD bShowLocal : 1;
		BIT_FIELD bWaitForRemote : 1;
		BIT_FIELD bExit : 1;
		BIT_FIELD bDone : 1;
		BIT_FIELD bRemoteKillRequired : 1;
	} flags;
	PTASK_INFO local_task;
	PTASK_INFO remote_task;
	PTASK_INFO remote_task_end; // pskill task on remote
	PTHREAD wait;
	PTHREAD wait_show_local;
   int first_place;
   int max_places;
	int claimed;
   PLIST claims;
} l;


void ReadPlayerFile( CTEXTSTR name )
{
	if( 1 )
	{
      TEXTCHAR buf[80];
		FILE *file = fopen( name, "rt" );
		if( file )
		{
			while( fgets( buf, sizeof( buf ), file ) )
			{
            TEXTCHAR *tmp;
				int n = strlen( buf );
				if( n && buf[n-1] == '\n' )
					buf[n-1] = 0;
				tmp = (TEXTCHAR*)StrChr( buf, '@' );
				if( tmp )
				{
					SetLink( &l.total_players, l.nPlayers, StrDup( tmp+1 ) );
					tmp[0] = 0;
				}
				AddLink( &l.players, StrDup( buf ) );
            l.nPlayers++;
			}
			fclose( file );
		}
		else
         lprintf( "failed to open %s", name );
	}
	else
	{
	}
}

static void CPROC LocalOutput( PTRSZVAL psv, PTASK_INFO task, CTEXTSTR buffer, size_t size )
{
   lprintf( "Task %p", task );
	if( StrCmpEx( buffer, "~CONNECT OK", 11 )== 0 )
	{
		l.flags.bShowLocal = 1;
		WakeThread( l.wait_show_local );
	}
	lprintf( "banner output: %s", buffer );
}

static void CPROC RemoteKillEnd( PTRSZVAL psv, PTASK_INFO task )
{
   l.remote_task_end = NULL;
}

static void CloseRemote( void )
{
   if( l.flags.bRemoteKillRequired )
	{
		TEXTSTR *pArgs;
		int nArgs;
		ParseIntoArgs( l.remote_kill, &nArgs, &pArgs );
		l.remote_task_end = LaunchPeerProgram( pArgs[0], NULL, (PCTEXTSTR)pArgs, LocalOutput, RemoteKillEnd, 0 );
	}
	TerminateProgram( l.remote_task );
}

static void CPROC LocalEnd( PTRSZVAL psv, PTASK_INFO task )
{
	_32 result = GetTaskExitCode( task );
	if( result == 1 )
		l.flags.bResultYes = 1;
	if( result == 2 )
	{
		l.flags.bExit = 1;
	}
	if( result == 3 )
	{
		l.flags.bResultYes = 1;
      l.flags.bDone = 1;
	}
	l.flags.bResult = 1;
	lprintf( "result yes is %d %d", l.flags.bResultYes, result );
	l.local_task = NULL;
   CloseRemote();
   WakeThread( l.wait );
}

PTEXT my_burst( PTEXT text )
{
   // just break on spaces and tabs.
   return TextParse( text, ",", " \t", 1, 1 DBG_SRC );
}

void PromptAndYesNo( CTEXTSTR name, CTEXTSTR total_players )
{
	PVARTEXT pvt_local = VarTextCreate();
	PVARTEXT pvt_remote = VarTextCreate();
	PTEXT tmp;
	TEXTSTR *pArgs;
   int nArgs;

	PTEXT burst_name = my_burst( tmp = SegCreateFromText( name ) );
   LineRelease( tmp );

   l.flags.bResultYes = 0;
	l.flags.bResult = 0;
   l.flags.bShowLocal = 0;
	l.wait_show_local = MakeThread();

	vtprintf( pvt_local, "%s %s ", l.local_banner_path, l.local_banner_args );
	vtprintf( pvt_remote, "%s %s %s %s ", l.launchpad_path, l.launchpad_args, l.remote_banner_path, l.remote_banner_args );

   if( l.nPlayers > 1 )
		vtprintf( pvt_local, "\"%s Place\" ", ords[l.first_place + l.claimed+1] );

	for( tmp = burst_name; tmp; tmp = NEXTLINE( tmp ) )
	{
		if( StrCmp( GetText( tmp ), "," ) == 0 )
         break;
		vtprintf( pvt_local, "\"%s\" ", GetText( tmp ) );
		vtprintf( pvt_remote, "\"%s\" ", GetText( tmp ) );
	}

   if( total_players )
		vtprintf( pvt_local, "\"Players : %s\" ", total_players );

	ParseIntoArgs( GetText( VarTextPeek( pvt_remote ) ), &nArgs, &pArgs );
	lprintf( "staring remote." );
	l.remote_task = LaunchPeerProgram( l.launchpad_path, NULL, (PCTEXTSTR)pArgs, LocalOutput, NULL, 0 );
	lprintf( "send start remote? %p", l.remote_task );


   if( l.flags.bWaitForRemote )
		while( !l.flags.bShowLocal )
		{
			WakeableSleep( 10000 );
			if( !l.flags.bShowLocal )
            break;
		}

	ParseIntoArgs( GetText( VarTextPeek( pvt_local ) ), &nArgs, &pArgs );
	l.local_task = LaunchPeerProgram( l.local_banner_path, NULL, (PCTEXTSTR)pArgs, LocalOutput, LocalEnd, 0 );



   VarTextDestroy( &pvt_local );
   VarTextDestroy( &pvt_remote );
}

void PromptLocalYesNo( CTEXTSTR name, CTEXTSTR total_players )
{
	PVARTEXT pvt_local = VarTextCreate();
	PTEXT tmp;
	TEXTSTR *pArgs;
   int nArgs;

	PTEXT burst_name = my_burst( tmp = SegCreateFromText( name ) );
   LineRelease( tmp );

   l.flags.bResultYes = 0;
	l.flags.bResult = 0;
   l.flags.bShowLocal = 0;
	l.wait_show_local = MakeThread();

	vtprintf( pvt_local, "%s %s ", l.local_banner_path, l.local_banner_args );

	vtprintf( pvt_local, "%s", name );

   if( total_players )
		vtprintf( pvt_local, "\"Players : %s\" ", total_players );

	ParseIntoArgs( GetText( VarTextPeek( pvt_local ) ), &nArgs, &pArgs );
	l.local_task = LaunchPeerProgram( l.local_banner_path, NULL, (PCTEXTSTR)pArgs, LocalOutput, LocalEnd, 0 );

   VarTextDestroy( &pvt_local );
}

void PromptLocal( CTEXTSTR name )
{
	PVARTEXT pvt_local = VarTextCreate();
	PTEXT tmp;
	TEXTSTR *pArgs;
   int nArgs;

	PTEXT burst_name = my_burst( tmp = SegCreateFromText( name ) );
   LineRelease( tmp );

   l.flags.bResultYes = 0;
	l.flags.bResult = 0;
   l.flags.bShowLocal = 0;
	l.wait_show_local = MakeThread();

	vtprintf( pvt_local, "%s %s ", l.local_banner_path, l.local_banner_noprompt_args );

	vtprintf( pvt_local, "%s", name );

	ParseIntoArgs( GetText( VarTextPeek( pvt_local ) ), &nArgs, &pArgs );
	l.local_task = LaunchPeerProgram( l.local_banner_path, NULL, (PCTEXTSTR)pArgs, LocalOutput, LocalEnd, 0 );

   VarTextDestroy( &pvt_local );
}

void PromptResult( CTEXTSTR name )
{
	PVARTEXT pvt_local = VarTextCreate();
	PVARTEXT pvt_remote = VarTextCreate();
	PTEXT tmp;
	TEXTSTR *pArgs;
   int nArgs;

	PTEXT burst_name = my_burst( tmp = SegCreateFromText( name ) );
   LineRelease( tmp );

   l.flags.bResultYes = 0;
   l.flags.bResult = 0;

	vtprintf( pvt_local, "%s %s ", l.local_banner_path, l.local_banner_args );

   for( tmp = burst_name; tmp; tmp = NEXTLINE( tmp ) )
		vtprintf( pvt_local, "\"%s\" ", GetText( tmp ) );

   ParseIntoArgs( GetText( VarTextPeek( pvt_local ) ), &nArgs, &pArgs );
	l.local_task = LaunchPeerProgram( l.local_banner_path, NULL, (PCTEXTSTR)pArgs, LocalOutput, LocalEnd, 0 );

	vtprintf( pvt_remote, "%s %s %s %s ", l.launchpad_path, l.launchpad_args, l.remote_banner_path, l.remote_banner_args );

	for( tmp = burst_name; tmp; tmp = NEXTLINE( tmp ) )
		vtprintf( pvt_remote, "\"%s\" ", GetText( tmp ) );

   ParseIntoArgs( GetText( VarTextPeek( pvt_remote ) ), &nArgs, &pArgs );
	l.remote_task = LaunchPeerProgram( l.launchpad_path, NULL, (PCTEXTSTR)pArgs, LocalOutput, NULL, 0 );

   VarTextDestroy( &pvt_local );
   VarTextDestroy( &pvt_remote );
}

void PromptAll( void )
{
	INDEX idx;
	CTEXTSTR name;
	CTEXTSTR extra;
	PVARTEXT pvt_local = VarTextCreate();
	PVARTEXT pvt_remote = VarTextCreate();
	TEXTSTR *pArgs;
	int nArgs;
	SYSTEMTIME st;
	GetLocalTime( &st );


	vtprintf( pvt_local, "%s %s ", l.local_banner_path, l.local_banner_final_args );
	vtprintf( pvt_remote, "%s %s %s %s ", l.launchpad_path, l.launchpad_args, l.remote_banner_path, l.remote_banner_final_args );

	LIST_FORALL( l.claims, idx, CTEXTSTR, name )
	{
		extra = StrChr( name, ',' );
		if( extra )
		{
			DoSQLCommandf( "insert into drawing (place,name,bingoday,whenstamp,account_number,card_number,session) values (%d,'%*.*s',%04d%02d%02d,%04d%02d%02d%02d%02d%02d,'%s','%s',current_session())"
							 , idx + 1
							 , (extra-name), (extra-name)
							 , name
							 , st.wYear, st.wMonth, st.wDay
							 , st.wYear, st.wMonth, st.wDay
							 , st.wHour, st.wMinute, st.wSecond
							 , extra+1
							 , extra+1
							 );
			if( l.nPlayers > 1 )
			{
				vtprintf( pvt_local, "\"%d. %*.*s\" ", idx + 1, (extra-name), (extra-name), name );
				vtprintf( pvt_remote, "\"%d. %*.*s\" ", idx + 1, (extra-name), (extra-name), name );
			}
			else
			{
				vtprintf( pvt_local, "\"%*.*s\" ", (extra-name), (extra-name), name );
				vtprintf( pvt_remote, "\"%*.*s\" ", (extra-name), (extra-name), name );
			}
		}
		else
		{
			DoSQLCommandf( "insert into drawing (place,name,bingoday,whenstamp,session) values (%d,'%*.*s',%04d%02d%02d,%04d%02d%02d%02d%02d%02d,current_session())"
                      , idx + 1
							 , (extra-name), (extra-name), name
							 , st.wYear, st.wMonth, st.wDay
							 , st.wYear, st.wMonth, st.wDay
							 , st.wHour, st.wMinute, st.wSecond
                       );
			if( l.nPlayers > 1 )
			{
				vtprintf( pvt_local, "\"%d. %s\" ", idx + 1, name );
				vtprintf( pvt_remote, "\"%d.%s\" ", idx + 1, name );
			}
			else
			{
				vtprintf( pvt_local, "\"%s\" ", name );
				vtprintf( pvt_remote, "\"%s\" ", name );
			}
		}
	}

   ParseIntoArgs( GetText( VarTextPeek( pvt_remote ) ), &nArgs, &pArgs );
	l.remote_task = LaunchPeerProgram( l.launchpad_path, NULL, (PCTEXTSTR)pArgs, LocalOutput, NULL, 0 );
   ParseIntoArgs( GetText( VarTextPeek( pvt_local ) ), &nArgs, &pArgs );
	//l.local_task = LaunchPeerProgram( l.local_banner_path, NULL, (PCTEXTSTR)pArgs, LocalOutput, LocalEnd, 0 );

	VarTextDestroy( &pvt_local );
	VarTextDestroy( &pvt_remote );
	PromptLocal( "\"Touch Anywhere\" \"to continue\"" );
	while( l.local_task )
	{
		WakeableSleep( 20000 );
	}
	CloseRemote();

}

CTEXTSTR DrawCreate = "CREATE TABLE if not exists `drawing` (                               "
					 "  `drawing_id` int(10) unsigned NOT NULL auto_increment,"
					 "  `first_name` varchar(45) default NULL,                       "
					 "  `last_name` varchar(45) default NULL,                        "
					 "  `account_number` varchar(25) default NULL,              "
					 "  `bingoday` date default NULL,                                "
					 "  `session` int default '0',                                "
					 "  `card_number` varchar(45) default NULL,                      "
					 "  `whenstamp` datetime default NULL,                           "
					 "  `name` varchar(80) default NULL,                             "
					 "  `modified_when` timestamp NOT NULL default CURRENT_TIMESTAMP,"
					 "  `place` int(10) unsigned NOT NULL,                   "
					 "  PRIMARY KEY  USING BTREE (`drawing_id`)              "
					 ") ";

void Init( void )
{
   TEXTCHAR filenamebuf[256];
   TEXTCHAR buf[256];
   PTABLE pTable = GetFieldsInSQL( DrawCreate, 0 );
	CheckODBCTable( NULL, pTable, CTO_MERGE );
	DestroySQLTable( pTable );
   /*
	DoSQLCommand( DrawCreate );
   */
	SACK_GetProfileString( "Top Player Picker", "player source pathname", "\\\\172.17.2.200\\c\\players.txt", filenamebuf, sizeof( filenamebuf ) );

	ReadPlayerFile( filenamebuf );

   if( SACK_GetProfileInt( "Top Player Picker", "select places claimed (resume from picked)", 0 ) )
	{
      CTEXTSTR *results;
		if( DoSQLRecordQueryf( NULL, &results, NULL, "select max(place) from drawing where place>0 and bingoday=%s and session=current_session()"
									,GetSQLOffsetDate( NULL, "Bingoday", 500 ) ) && results )
		{
			l.first_place = results[0]?atoi( results[0] ):0;
		}
	}

	SACK_GetProfileString( "Top Player Picker", "Local Banner program path", "banner_command", buf, sizeof( buf ) );
	l.local_banner_path = StrDup( buf );
   SACK_GetProfileString( "Top Player Picker", "Local Banner Arguments", "-okcancel -yesno -lines 5 -cols 10", buf, sizeof( buf ) );
	l.local_banner_args = StrDup( buf );
   SACK_GetProfileString( "Top Player Picker", "Local Banner No Prompt Arguments", "-lines 5 -cols 10", buf, sizeof( buf ) );
	l.local_banner_noprompt_args = StrDup( buf );
   SACK_GetProfileString( "Top Player Picker", "Local Banner Final Arguments", "-okcancel -yesno -lines 10 -cols 20", buf, sizeof( buf ) );
	l.local_banner_final_args = StrDup( buf );


	SACK_GetProfileString( "Top Player Picker", "Launchcmd Command Path", "launch_command.exe", buf, sizeof( buf ) );
   l.launchpad_path = StrDup( buf );
	SACK_GetProfileString( "Top Player Picker", "Launchcmd Command args", "-h -l -s 172.17.255.255 -c flashboard", buf, sizeof( buf ) );
   l.launchpad_args = StrDup( buf );


	SACK_GetProfileString( "Top Player Picker", "Remote Banner program path", "banner_command", buf, sizeof( buf ) );
	l.remote_banner_path = StrDup( buf );
	SACK_GetProfileString( "Top Player Picker", "Remote Banner Args", "", buf, sizeof( buf ) );
	l.remote_banner_args = StrDup( buf );

	l.flags.bRemoteKillRequired = SACK_GetProfileInt( "Top Player Picker", "Remote Kill Required", 1 );
	if( l.flags.bRemoteKillRequired )
	{
		SACK_GetProfileString( "Top Player Picker", "Remote Kill Program Command", "launchcmd -s 172.17.255.255 -c flashboard c:\\tools\\pskill.exe banner_command.exe", buf, sizeof( buf ) );
		l.remote_kill = StrDup( buf );
	}

	SACK_GetProfileString( "Top Player Picker", "Remote Banner Final Args", "-lines 7 -cols 20", buf, sizeof( buf ) );
	l.remote_banner_final_args = StrDup( buf );

   l.flags.bWaitForRemote = SACK_GetProfileIntEx( "Top Player Picker", "Wait for remote task start", 1, TRUE );
   l.max_places = SACK_GetProfileInt( "Top Player Picker", "Max Places", 5 );
}

void ProcessList( void )
{
	CTEXTSTR name;
	INDEX idx;
	l.wait = MakeThread();
	LIST_FORALL( l.players, idx, CTEXTSTR, name )
	{
		PromptAndYesNo( name, GetLink( &l.total_players, idx ) );
		while( l.local_task )
		{
			WakeableSleep( 20000 );
		}
		if( l.flags.bResultYes )
		{
			AddLink( &l.claims, StrDup( name ) );
			l.claimed++;
		}
		if( (l.first_place + l.claimed ) == l.max_places )
			break;
		if( l.flags.bExit )
			break;
	}
	if( l.claimed == 0 )
      l.flags.bExit = 1;
	if( !l.flags.bExit )
	{
		PromptAll();
	}

}

int main( void )
{
	Init();
	ProcessList();
   return 0;
}

