//#define DEKWARE_PLUGIN
#define DEFINE_DEFAULT_IMAGE_INTERFACE
#include <sack_system.h>
#include <deadstart.h>
#include <psi/console.h>
#include <filesys.h>
#include <sqlgetoption.h>
#include "../widgets/include/banner.h"

#define DEFINES_INTERSHELL_INTERFACE
#define USES_INTERSHELL_INTERFACE
#include "../intershell_export.h"
#include "../intershell_registry.h"

typedef struct file_info *PFILE_INFO;
struct file_info
{
	DeclareLink( struct file_info );
	struct {
		BIT_FIELD bUsed : 1;
	} flags;
	CTEXTSTR name; // strdup'd name
};

struct path_info
{
	CTEXTSTR property;
	CTEXTSTR base_path;
};

struct property_button {
	PMENU_BUTTON button;
	int ID;
};

static struct my_vidplayer_local
{
	PLIST consoles;
	PLIST lists;
	PTHREAD waiting;
	PTHREAD waiting2;
	PTHREAD workthread;
	PTHREAD workthread2;
	PMENU_BUTTON button; // button which triggered the event.
	PMENU_BUTTON button2; // button which triggered the event.
	int begin_update;
	int update_done;
	int begin_test;
	int test_done;
	PFILE_INFO files;
	PFILE_INFO old_files;
	PLIST base_video_paths;
	CTEXTSTR base_video_path;
	CTEXTSTR base_video_play_path;
	PFILE_INFO current_file;
	PLISTITEM current_item;
	PLIST original_order;
	int current_property; // just an initializer counter for select property buttons.
	struct property_button * prior_property;
}l;

void CPROC TaskDone( PTRSZVAL psv, PTASK_INFO Task )
{
	{
		INDEX idx;
		PSI_CONTROL console;
		LIST_FORALL( l.consoles, idx, PSI_CONTROL, console )
			pcprintf( console, "---- TASK DONE-----" );
	}
	lprintf( "---- TASK DONE-----" );
}
void CPROC GetOutput( PTRSZVAL psv, PTASK_INFO task, CTEXTSTR buffer, _32 size )
{
	{
		INDEX idx;
		PSI_CONTROL console;
		LIST_FORALL( l.consoles, idx, PSI_CONTROL, console )
			pcprintf( console, "%*.*s", size, size, buffer );
	}
	LogBinary( buffer, size );
}

#ifdef DEKWARE_PLUGIN
static int HandleCommand( "VidPlayContinue", "Tell the video player to continue..." )( PSENTIENT ps, PTEXT params )
{
	l.update_done = 1;
	WakeThread( l.waiting );
	return 0;
}

static int HandleCommand( "VidTestContinue", "Tell the video player to continue..." )( PSENTIENT ps, PTEXT params )
{
	l.test_done = 1;
	WakeThread( l.waiting );
	return 0;
}

void LaunchProcess()
{
	if( !l.base_video_path )
		return;
	{
		TEXTCHAR buf[256];
		snprintf( buf, sizeof( buf ), "%s/PlayList.m3u", l.base_video_path );
		{
			FILE *file;
			file = fopen( buf, "wt" );
			if( file )
			{
				PFILE_INFO info;
				for( info = l.files; info; info = NextLink( info ) )
				{
					if( info->flags.bUsed )
						fprintf( file, "%s/%s\n", l.base_video_play_path, info->name );
				}
				fclose( file );
			}
		}
	}
	{
		TEXTCHAR buf[256];
		snprintf( buf, sizeof( buf ), "%s/PlayTest.m3u", l.base_video_path );
		{
			FILE *file;
			file = fopen( buf, "wt" );
			if( file )
			{
				PFILE_INFO info;
				for( info = l.files; info; info = NextLink( info ) )
				{
					if( info->flags.bUsed )
						fprintf( file, "%s/%s\n", l.base_video_path, info->name );
				}
				fclose( file );
			}
		}
	}

	DoCommandf( PLAYER, "/scr test3.scr" );
	l.waiting = MakeThread();
	while( !l.update_done )
	{
		WakeableSleep( 1000 );
	}
	l.update_done = 0;
	//LaunchPeerProgram( "test.bat", NULL, NULL, GetOutput, TaskDone, 0 );
}

void LaunchProcess2()
{
	if( !l.base_video_path )
		return;

	DoCommandf( PLAYER, "/decl base_path %s", l.base_video_path );
	DoCommandf( PLAYER, "/decl dest_path %s", l.base_video_play_path );
	DoCommandf( PLAYER, "/scr test4.scr" );
	l.waiting = MakeThread();
	while( !l.test_done )
	{
		WakeableSleep( 1000 );
	}
	l.test_done = 0;
	//LaunchPeerProgram( "test.bat", NULL, NULL, GetOutput, TaskDone, 0 );
}
#endif

PTRSZVAL CPROC Thread( PTHREAD thread )
{
	l.workthread = thread;
	while( 1 )
	{
		WakeableSleep( 10000 );
		if( l.begin_update )
		{
			PMENU_BUTTON b = l.button;
			l.begin_update = 0;
			InterShell_SetButtonHighlight( b, TRUE );
			//LaunchProcess();
			InterShell_SetButtonHighlight( b, FALSE );
		}
	}
	return 0;
}

PTRSZVAL CPROC Thread2( PTHREAD thread )
{
	l.workthread2 = thread;
	while( 1 )
	{
		WakeableSleep( 10000 );
		if( l.begin_test )
		{
			PMENU_BUTTON b = l.button2;
			l.begin_test = 0;
			InterShell_SetButtonHighlight( b, TRUE );
#ifdef DEKWARE_LUGIN
			LaunchProcess2();
#endif
			InterShell_SetButtonHighlight( b, FALSE );
		}
	}
	return 0;
}

void ReloadPlaylist( void )
{
	if( l.base_video_path )
	{
		TEXTCHAR buf[256];
		snprintf( buf, sizeof( buf ), "%s/PlayList.m3u", l.base_video_path );
		{
			FILE *file;
			file = fopen( buf, "rt" );
			if( file )
			{
				while( fgets( buf, sizeof( buf ), file ) )
				{
					CTEXTSTR name = pathrchr( buf );
					if( buf[strlen(buf)-1] == '\n' )
						buf[strlen(buf)-1] = 0;
					if( !name )
						name = buf;
					else
						name++;
					AddLink( &l.original_order, StrDup( name ) );
				}
				fclose( file );
			}
		}
	}
}


PRELOAD( BeginVideoPlayer )
{
	ThreadTo( Thread, 0 );
	ThreadTo( Thread2, 0 );
	{
		char buffer[256];
		//l.base_video_path = StrDup( buffer );

		{
			int n;
			for( n = 0; ;n++ )
			{
				char prop_path[256];
				char prop_name[256];
				char property[64];
				snprintf( property, sizeof( property ), "Property %d", n );
				SACK_GetPrivateProfileString( property, "base path to video files", "", prop_path, sizeof( prop_path ), "vplayer.ini" );
				if( prop_path[0] == 0 )
					break;
				SACK_GetPrivateProfileString( property, "Property Name", "", prop_name, sizeof( prop_name ), "vplayer.ini" );
				{
					struct path_info *prop = New( struct path_info );
					prop->property = StrDup( prop_name );
					prop->base_path = StrDup( prop_path );
					SetLink( &l.base_video_paths, n, prop );
					snprintf( property, sizeof( property ), "<Property %d>", n );
					CreateLabelVariable( property, LABEL_TYPE_STRING, &prop->property );
				}
			}
		}

		SACK_GetPrivateProfileString( "Video Ad Player", "base path to play video files (on vserver)", "/storage", buffer, sizeof( buffer ), "vplayer.ini" );
		l.base_video_play_path = StrDup( buffer );
	}
	ReloadPlaylist();
}

static PSI_CONTROL OnGetControl( "Video Player/Console" )( PTRSZVAL psv )
{
	return (PSI_CONTROL)psv;
}

static PTRSZVAL OnCreateControl( "Video Player/Console" )( PSI_CONTROL parent, S_32 x, S_32 y, _32 w, _32 h )
{
	PSI_CONTROL pc = MakeNamedControl( parent, "PSI Console", x, y, w, h, -1 );
	AddLink( &l.consoles, pc );
	return (PTRSZVAL)pc;
}

static void OnKeyPressEvent( "Video Player/Upload" )( PTRSZVAL psv )
{
	l.begin_update = 1;
	l.button = (PMENU_BUTTON)psv;
	WakeThread( l.workthread );
}

static PTRSZVAL OnCreateMenuButton( "Video Player/Upload" )( PMENU_BUTTON button )
{
	InterShell_SetButtonStyle( button, "bicolor square" );
	InterShell_SetButtonColors( button, BASE_COLOR_BLACK, BASE_COLOR_CYAN, BASE_COLOR_BLACK, COLOR_IGNORE );
	return (PTRSZVAL)button;
}

static void OnKeyPressEvent( "Video Player/Test" )( PTRSZVAL psv )
{
	l.begin_test = 1;
	l.button2 = (PMENU_BUTTON)psv;
	WakeThread( l.workthread2 );
}

static PTRSZVAL OnCreateMenuButton( "Video Player/Test" )( PMENU_BUTTON button )
{
	InterShell_SetButtonStyle( button, "bicolor square" );
	InterShell_SetButtonText( button, "Test_Playlist" );
	InterShell_SetButtonColors( button, BASE_COLOR_BLACK, BASE_COLOR_CYAN, BASE_COLOR_BLACK, COLOR_IGNORE );
	return (PTRSZVAL)button;
}


void CPROC AddFile( PTRSZVAL psv, CTEXTSTR name, int flags )
{
	if( ( StrCaseCmp( name, "PlayList.m3u" ) == 0 )
	  ||( StrCaseCmp( name, "PlayTest.m3u" ) == 0 )
	  ||( StrCaseCmp( name, "PlayTest.m3u~" ) == 0 )
	  ||( StrCaseCmp( name, "PlayList.m3u~" ) == 0 ) )
		return;
	else
	{
		PFILE_INFO file = New( struct file_info );
		file->flags.bUsed = 1;
		file->name = StrDup( name );
		file->me = NULL;
		file->next = NULL;
		lprintf( "adding file %s", name );
		LinkLast( l.files, PFILE_INFO, file );
		{
			CTEXTSTR ext = StrRChr( name, '.' );
			if( ext && StrCaseCmp( ext, ".m3u" ) == 0 )
				file->flags.bUsed = 0;
			{
				PFILE_INFO info;
				for( info = l.old_files; info; info = NextLink( info ) )
				{
					if( StrCmp( info->name, file->name ) == 0 )
					{
						file->flags.bUsed = info->flags.bUsed;
						break;
					}
				}
			}
		}
	}
}

void DoScanFiles( void )
{
	void *info = NULL;
	if( !l.base_video_path )
	{
		return;
	}
	l.old_files = l.files;
	if( l.old_files )
		l.old_files->me = &l.old_files;
	l.files = NULL;

	while( ScanFiles( l.base_video_path, "*", &info, AddFile, SFF_NAMEONLY, 0 ) );

	if( l.old_files ) // just to clarify that this is not part of the while loop.
	{
		PFILE_INFO tmp2 = l.files;
		PFILE_INFO ztmp;
		PFILE_INFO info;
		PFILE_INFO next;
		tmp2->me = &tmp2;
		l.files = NULL; // grabbed all files from this list....
		for( info = l.old_files; info; info = NextLink( info ) )
		{
			for( ztmp = tmp2; ztmp; ztmp = NextLink( ztmp ) )
			{
				if( StrCaseCmp( ztmp->name, info->name ) == 0 )
				{
					ztmp->flags.bUsed = info->flags.bUsed;
					UnlinkThing( ztmp );
					// don't use a variable called'tmp' in this macro.
					LinkLast( l.files, PFILE_INFO, ztmp );
					break;
				}
			}
		}
		for( info = l.old_files; next = NextLink( info ), info; info = next )
		{
			Release( (POINTER)info->name );
			Release( info );
		}
		l.old_files = NULL;
	}
	else if( l.files )
	{
		// this is the first load of the list... and, we should order appropriately.
		PFILE_INFO tmp = l.files;
		INDEX idx;
		CTEXTSTR name;
		tmp->me = &tmp;
		l.files = NULL; // grabbed all files from this list....
		LIST_FORALL( l.original_order, idx, CTEXTSTR, name )
		{
			PFILE_INFO test;
			for( test = tmp; test; test = NextLink( test ) )
			{
				lprintf( "Is %s==%s", name, test->name );
				if( StrCaseCmp( name, test->name ) == 0 )
				{
					test->flags.bUsed = TRUE;
					UnlinkThing( test );
					LinkLast( l.files, PFILE_INFO, test );
					break;
				}
			}
		}
		{
			PFILE_INFO test;
			for( test = tmp; test; test = NextLink( test ) )
			{
				test->flags.bUsed = 0;
			}
		}
		// then add anything else.
		while( tmp )
		{
			PFILE_INFO add = tmp;
			UnlinkThing( add );
			LinkLast( l.files, PFILE_INFO, add );
		}
	}
}

CTEXTSTR GetFileString( PFILE_INFO info )
{
	static TEXTCHAR buf[256];
	snprintf( buf, sizeof( buf ), "[%s]\t%s", info->flags.bUsed?"PLAY":"	 ", info->name );
	return buf;
}

void RefillListbox( PSI_CONTROL list )
{
	EnableCommonUpdates( list, FALSE );
	ResetList( list );
	if( !l.base_video_path )
	{
		AddListItem( list, "Please select a location" );
	}
	else
	{
		PFILE_INFO file;
		for( file = l.files; file; file = NextLink( file ) )
		{
			PLISTITEM item = SetItemData( AddListItem( list, GetFileString( file ) ), (PTRSZVAL)file );
			lprintf( "Added file %s", GetFileString( file ) );
			if( file == l.current_file )
				SetSelectedItem( list, item );
		}
	}
	EnableCommonUpdates( list, TRUE );
	SmudgeCommon( list );

}


static void OnShowControl( "Video PLayer/File List" )( PTRSZVAL psv )
{
	PSI_CONTROL list = (PSI_CONTROL)psv;
	DoScanFiles();
	l.current_item = NULL;
	l.current_file = NULL;
	RefillListbox( list );
}

static void OnSelectListboxItem( "Video Player/File List", ".." )( PTRSZVAL psv, PLISTITEM pli )
{
	l.current_item = pli;
	l.current_file = (PFILE_INFO)GetItemData( l.current_item );
}

static PTRSZVAL OnCreateListbox( "Video Player/File List" )( PSI_CONTROL list )
{
	static int stops[2] = {0, 48};
	SetListBoxTabStops( list, 2, stops );
	AddLink( &l.lists, list );
	return (PTRSZVAL)list;
}

static void OnDestroyControl( "Video Player/File List" )( PTRSZVAL psv )
{
	DeleteLink( &l.lists, (POINTER)psv );
}

static void OnKeyPressEvent( "Video Player/Move File Up" )( PTRSZVAL psv )
{
	if( l.current_item )
	{
		PTRSZVAL psv = GetItemData( l.current_item );
		PFILE_INFO info = (PFILE_INFO)psv;
		PFILE_INFO next = (PFILE_INFO)info->me;
		// if the beginning of the list..
		if( next == (PFILE_INFO)&l.files )
		{
			return;
		}
		UnlinkThing( info );
		LinkThingBefore( next, info );
		{
			INDEX idx;
			PSI_CONTROL list;
			LIST_FORALL( l.lists, idx, PSI_CONTROL, list )
			{
				RefillListbox( list );
			}
		}
	}
}

static PTRSZVAL OnCreateMenuButton( "Video Player/Move File Up" )( PMENU_BUTTON button )
{
	InterShell_SetButtonStyle( button, "bicolor square" );
	InterShell_SetButtonText( button, "File_Up" );
	InterShell_SetButtonColors( button, BASE_COLOR_WHITE, BASE_COLOR_BLUE, COLOR_IGNORE, COLOR_IGNORE );
	return (PTRSZVAL)button;
}

static void OnKeyPressEvent( "Video Player/Move File Down" )( PTRSZVAL psv )
{
	if( l.current_item )
	{
		PTRSZVAL psv = GetItemData( l.current_item );
		PFILE_INFO info = (PFILE_INFO)psv;
		PFILE_INFO next = NextLink( info );
		// if end of list..
		if( !next )
			return;
		UnlinkThing( info );
		LinkThingAfter( next, info );
		{
			INDEX idx;
			PSI_CONTROL list;
			LIST_FORALL( l.lists, idx, PSI_CONTROL, list )
			{
				RefillListbox( list );
			}
		}
	}
}

static PTRSZVAL OnCreateMenuButton( "Video Player/Move File Down" )( PMENU_BUTTON button )
{
	InterShell_SetButtonStyle( button, "bicolor square" );
	InterShell_SetButtonText( button, "File_Down" );
	InterShell_SetButtonColors( button, BASE_COLOR_WHITE, BASE_COLOR_BLUE, COLOR_IGNORE, COLOR_IGNORE );
	return (PTRSZVAL)button;
}



static void OnKeyPressEvent( "Video Player/Toggle File Play" )( PTRSZVAL psv )
{
	if( l.current_item )
	{
		PTRSZVAL psv = GetItemData( l.current_item );
		PFILE_INFO info = (PFILE_INFO)psv;
		info->flags.bUsed = !info->flags.bUsed;
		SetItemText( l.current_item, GetFileString( info ) );
	}
}

static PTRSZVAL OnCreateMenuButton( "Video Player/Toggle File Play" )( PMENU_BUTTON button )
{
	InterShell_SetButtonStyle( button, "bicolor square" );
	InterShell_SetButtonText( button, "Toggle_Play" );
	InterShell_SetButtonColors( button, BASE_COLOR_WHITE, BASE_COLOR_GREEN, COLOR_IGNORE, COLOR_IGNORE );
	return (PTRSZVAL)button;
}


static void OnKeyPressEvent( "Video Player/Delete File" )( PTRSZVAL psv )
{
	if( l.current_item )
	{
		PTRSZVAL psv = GetItemData( l.current_item );
		PFILE_INFO info = (PFILE_INFO)psv;
		{
			char buffer[256];
			snprintf( buffer, sizeof( buffer ), "%s/%s", l.base_video_path, info->name );
			unlink( buffer );
		}
		UnlinkThing( info );
		Release( (POINTER)info->name );
		Release( info );
		l.current_item = NULL;
		l.current_file = NULL;
		{
			INDEX idx;
			PSI_CONTROL list;
			LIST_FORALL( l.lists, idx, PSI_CONTROL, list )
			{
				RefillListbox( list );
			}
		}
	}
}

static PTRSZVAL OnCreateMenuButton( "Video Player/Delete File" )( PMENU_BUTTON button )
{
	InterShell_SetButtonStyle( button, "bicolor square" );
	InterShell_SetButtonText( button, "Toggle_Play" );
	InterShell_SetButtonColors( button, BASE_COLOR_WHITE, BASE_COLOR_GREEN, COLOR_IGNORE, COLOR_IGNORE );
	return (PTRSZVAL)button;
}



static void OnKeyPressEvent( "Video Player/Select Property" )( PTRSZVAL psv )
{
	//if( l.current_item )
	{
		struct property_button *button = (struct property_button*)psv;
		struct path_info *path = (struct path_info *)GetLink( &l.base_video_paths, button->ID );
		PFILE_INFO info;
		PFILE_INFO next;
		l.base_video_path = path->base_path;
		for( info = l.files; next = NextLink( info ), info; info = next )
		{
			Release( (POINTER)info->name );
			Release( info );
		}
		l.files = NULL;
		ReloadPlaylist();
		//ShellSetCurrentPage( InterShell_GetCanvas( "here" );
		if( l.prior_property )
		{
			InterShell_SetButtonHighlight( l.prior_property->button, FALSE );
		}
		InterShell_SetButtonHighlight( button->button, TRUE );
		l.prior_property = button;
	}
}

static void OnShowControl( "Video Player/Select Property" )( PTRSZVAL psv )
{
	struct property_button *button = (struct property_button*)psv;
	char varname[64];
	snprintf( varname, sizeof( varname ), "Select_%%<Property %d>", button->ID );
	InterShell_SetButtonText( button->button, varname );
}

static PTRSZVAL OnCreateMenuButton( "Video Player/Select Property" )( PMENU_BUTTON button )
{
	struct property_button *prop_button = New(struct property_button);
	prop_button->button = button;
	prop_button->ID = l.current_property++;
	InterShell_SetButtonStyle( button, "bicolor square" );
	InterShell_SetButtonColors( button, BASE_COLOR_WHITE, BASE_COLOR_GREEN, COLOR_IGNORE, COLOR_IGNORE );
	return (PTRSZVAL)(prop_button);
}

static PTRSZVAL OnConfigureControl( "Video Player/Select Property" )( PTRSZVAL psv, PSI_CONTROL parent )
{

	return psv;
}

static void OnSaveControl( "Video Player/Select Property")( FILE *file, PTRSZVAL psv )
{
	struct property_button *button = (struct property_button*)psv;
	fprintf( file, "Property ID=%d\n", button->ID );
}

PTRSZVAL CPROC ReloadID( PTRSZVAL psv, arg_list args )
{
	PARAM( args, int, id );
	struct property_button *button = (struct property_button*)psv;
	button->ID = id;
	return psv;
}

static void OnLoadControl( "Video PLayer/Select Property")( PCONFIG_HANDLER pch, PTRSZVAL psv )
{
	AddConfigurationMethod( pch, "Property ID=%i", ReloadID );
}


static void copy( CTEXTSTR src, char *dst )
{
	static _8 buffer[4096];
	FILE *in, *out;
	_64 filetime;
	_64 filetime_dest;

	filetime = GetFileWriteTime( src );
	filetime_dest = GetFileWriteTime( dst );

	if( filetime <= filetime_dest )
		return;
	in = fopen( src, WIDE("rb") );
	if( in )
		out = fopen( dst, WIDE("wb") );
	else
		out = NULL;
	if( in && out )
	{
		int len;
		while( len = fread( buffer, 1, sizeof( buffer ), in ) )
			fwrite( buffer, 1, len, out );
	}
	if( in )
		fclose( in );
	if( out )
		fclose( out );
	SetFileWriteTime( dst, filetime );
}

static void CPROC AcceptFile( PSI_CONTROL pc, CTEXTSTR file, S_32 x, S_32 y )
{
	if( !l.base_video_path )
	{
		Banner2Message( "No current location specified...." );
	}
	else
	{
		TEXTCHAR dest_name[256];
		CTEXTSTR fname = pathrchr( file );
		if( !fname )
			fname = file;
		else
			fname++;
		snprintf( dest_name, sizeof( dest_name ), "%s/%s", l.base_video_path, fname );
		copy( file, dest_name );
	}
	// otherwise we'll only get the files we just had.
	{
		PFILE_INFO info;
		PFILE_INFO next;
		for( info = l.files; next = NextLink( info ), info; info = next )
		{
			Release( (POINTER)info->name );
			Release( info );
		}
	}
	l.files = NULL;
	DoScanFiles();
	l.current_item = NULL;
	l.current_file = NULL;
	{
		INDEX idx;
		PSI_CONTROL list;
		LIST_FORALL( l.lists, idx, PSI_CONTROL, list )
		{
			RefillListbox( list );
		}
	}
}

static PTRSZVAL OnCreateMenuButton( "Video Player/Accept Files" )( PMENU_BUTTON button )
{
	AddCommonAcceptDroppedFiles( InterShell_GetButtonControl( button ), AcceptFile );
	InterShell_SetButtonStyle( button, "bicolor square" );
	InterShell_SetButtonText( button, "Toggle_Play" );
	InterShell_SetButtonColors( button, BASE_COLOR_WHITE, BASE_COLOR_GREEN, COLOR_IGNORE, COLOR_IGNORE );
	return (PTRSZVAL)button;
}



#if defined( __CMAKE_VERSION__ ) && ( __CMAKE_VERSION__ < 2081003 )
// cmake + watcom link failure fix
PUBLIC( void, ExportThis )( void )
{
}
#endif
