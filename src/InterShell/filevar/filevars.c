#include <stdhdrs.h>
#include <system.h> // GetProgramName()
#include <sqlgetoption.h>
#define USES_INTERSHELL_INTERFACE
#define DEFINES_INTERSHELL_INTERFACE
#include "../intershell_export.h"
#include "../intershell_registry.h"

enum filevar_resources{
	LISTBOX_VARIABLES = 1000,
	BTN_ADD_VARIABLE,
	BTN_DEL_VARIABLE,
	EDIT_VARNAME,
   EDIT_FILENAME,
};

struct variable_tracker
{
	TEXTSTR var_content;
	PVARIABLE variable;
   TEXTSTR var_name;
};

struct input_file
{
	struct {
		BIT_FIELD bDeleted : 1;
	} flags;
	CTEXTSTR varname;
	CTEXTSTR filename;
   PLIST vars;
};

struct local
{
	struct {
		BIT_FIELD bLog : 1;
	} flags;
	PLIST files;
   _32 timer;
#define l local_filevar_data
} l;

PRELOAD( InitFileVars )
{
   l.flags.bLog = SACK_GetProfileInt( GetProgramName(), WIDE("filevars/Log File Variables"), 0 );
	EasyRegisterResource( WIDE("InterShell/File Vars"), LISTBOX_VARIABLES, LISTBOX_CONTROL_NAME );
	EasyRegisterResource( WIDE("InterShell/File Vars"), BTN_ADD_VARIABLE, NORMAL_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/File Vars"), BTN_DEL_VARIABLE, NORMAL_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/File Vars"), EDIT_VARNAME, EDIT_FIELD_NAME );
	EasyRegisterResource( WIDE("InterShell/File Vars"), EDIT_FILENAME, EDIT_FIELD_NAME );
}

static void CPROC CheckFiles( PTRSZVAL psv )
{
	INDEX idx;
	struct input_file *file;
	if( l.flags.bLog )
		lprintf( WIDE("Check Files...") );
	LIST_FORALL( l.files, idx, struct input_file*, file )
	{
		FILE *input;
		static TEXTCHAR buf[256];
      if( l.flags.bLog )
			lprintf( WIDE("check %s"), file->filename );
		input = sack_fopen( 0, file->filename, WIDE("rt") );
		if( input )
		{
         INDEX var_idx = 0;
			while( fgets( buf, sizeof( buf ), input ) )
			{
            size_t end;
				struct variable_tracker *var =(struct variable_tracker*)GetLink( &file->vars, var_idx );
				//lprintf( WIDE("buf .. %s"), buf );
				while( ( end = strlen(buf) ) && ( buf[end-1] == '\n' ) )
					buf[end-1] = 0;
				if( l.flags.bLog )
					lprintf( WIDE("Content %p %s"), var, buf );
				if( !var )
				{
					TEXTCHAR tmp_name[128];
					snprintf( tmp_name, sizeof( tmp_name ), WIDE("<File %s.%d>"), file->varname, var_idx+1 );
					var = New( struct variable_tracker );
					var->var_content = StrDup( buf );
               var->var_name = StrDup( tmp_name );
					lprintf( WIDE("Newvar %s=%s"), tmp_name, buf );
					var->variable = CreateLabelVariable( tmp_name, LABEL_TYPE_STRING, &var->var_content );
               SetLink( &file->vars, var_idx, var );
				}
				else
				{
					if( StrCmp( var->var_content, buf ) )
					{
						Release( var->var_content );
						var->var_content = StrDup( buf );
						lprintf( WIDE("Change var %s=%s"), var->var_name, buf );
 						LabelVariableChanged( var->variable );
					}
				}

            var_idx++;
			}
         sack_fclose( input );
		}
	}
}

static PTRSZVAL CPROC AddInputFile( PTRSZVAL psv, arg_list args )
{
   PARAM( args, CTEXTSTR, var_name );
	PARAM( args, CTEXTSTR, file_name );

	struct input_file *file;
	file = New( struct input_file );
   file->flags.bDeleted = 0;
	file->varname = StrDup( var_name );
	file->filename = StrDup( file_name );
	file->vars = NULL;
   AddLink( &l.files, file );

	if( !l.timer )
	{
		l.timer = AddTimer( 5000, CheckFiles, 0 );
	}
	RescheduleTimer( l.timer );
   CheckFiles( 0 );

   return psv;
}

OnLoadCommon( WIDE("File Variables") )( PCONFIG_HANDLER pch )
{
   AddConfigurationMethod( pch, WIDE( "file variable input \"%m\" \"%m\"" ), AddInputFile );
}

OnSaveCommon( WIDE("File Variables") )( FILE *output )
{
	INDEX idx;
	struct input_file *file;
	LIST_FORALL( l.files, idx, struct input_file*, file )
	{
		if( !file->flags.bDeleted )
			fprintf( output, WIDE("file variable input \"%s\" \"%s\"\n"), file->varname, EscapeMenuString( file->filename ) );
	}
}

static void CPROC AddVariable( PTRSZVAL psv, PSI_CONTROL pc )
{
	TEXTCHAR varname[256];
	TEXTCHAR filename[256];
	GetControlText( GetNearControl( pc, EDIT_VARNAME ), varname, sizeof( varname ) );
	GetControlText( GetNearControl( pc, EDIT_FILENAME ), filename, sizeof( filename ) );
	{
		PSI_CONTROL listbox = GetNearControl( pc, LISTBOX_VARIABLES );
		INDEX idx;
		struct input_file *file;
		LIST_FORALL( l.files, idx, struct input_file*, file )
		{
			if( StrCaseCmp( file->varname, varname )== 0 )
				break;
		}

		if( !file )
		{
			file = New( struct input_file );
			file->flags.bDeleted = 0;
			file->varname = StrDup( varname );
			file->filename = StrDup( filename );
			file->vars = NULL;
			AddLink( &l.files, file );
			{
				TEXTCHAR tmp[80];
				snprintf( tmp, sizeof( tmp ), WIDE("%s from file %s"), file->varname, file->filename );
				SetItemData( AddListItem( listbox, tmp ), (PTRSZVAL)file );
			}
		}
	}
}

static void CPROC DelVariable( PTRSZVAL psv, PSI_CONTROL pc )
{
	PSI_CONTROL listbox = GetNearControl( pc, LISTBOX_VARIABLES );
	PLISTITEM pli = GetSelectedItem( listbox );
	struct input_file *file = (struct input_file *)GetItemData( pli );
	if( file )
	{
      file->flags.bDeleted = 1;
      DeleteListItem( listbox, pli );
	}
}

OnGlobalPropertyEdit( WIDE("File Variables") )( PSI_CONTROL parent )
{
	PSI_CONTROL frame = LoadXMLFrameOver( parent, WIDE("CommonFileVariableProperties.isFrame") );
	if( frame )
	{
		int okay = 0;
		int done = 0;
		SetCommonButtons( frame, &done, &okay );

		{
         PSI_CONTROL listbox = GetControl( frame, LISTBOX_VARIABLES );
			INDEX idx;
			struct input_file *file;
			LIST_FORALL( l.files, idx, struct input_file*, file )
			{
				TEXTCHAR tmp[80];
				if( !file->flags.bDeleted )
				{
					snprintf( tmp, sizeof( tmp ), WIDE("%s from file %s"), file->varname, file->filename );
					SetItemData( AddListItem( listbox, tmp ), (PTRSZVAL)file );
				}
			}
			SetButtonPushMethod( GetControl( frame, BTN_ADD_VARIABLE), AddVariable, 0 );
			SetButtonPushMethod( GetControl( frame, BTN_DEL_VARIABLE ), DelVariable, 0 );
		}

		DisplayFrameOver( frame, parent );
		CommonWait( frame );
		if( okay )
		{
		}
      DestroyFrame( &frame );
	}
}

#if ( __WATCOMC__ < 1291 )
PUBLIC( void, ExportThis )( void )
{
}
#endif
