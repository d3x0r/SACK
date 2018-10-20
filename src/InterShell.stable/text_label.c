
//#define PAPER_SUPPORT_LIBRARY
#include <stdhdrs.h>
#include <filesys.h>
#include "intershell_local.h"
#include "intershell_registry.h"
#include "resource.h"

// this module is built-in to the core..
#include "menu_real_button.h"

//#include "papers.h"
//#include "users.h"

#include "fonts.h"

INTERSHELL_NAMESPACE

#define TEXT_LABEL_NAME WIDE("Text Label")



struct page_label {
	PSI_CONTROL control;
	PSI_CONTROL canvas;
	PMENU_BUTTON button;  // to the shell, everything is a button
	struct {
		BIT_FIELD bCenter : 1;
		BIT_FIELD bRight : 1;
		/* these will be fun to implement... */
		BIT_FIELD bVertical : 1;
		BIT_FIELD bInverted : 1;
		BIT_FIELD bScroll : 1;
		BIT_FIELD bShadow : 1;  // user draw?  maybe I should override the draw proc?, take the caption text to the config file?
	} flags;
	CDATA color;
	CDATA color2;
	CDATA back_color;
	SFTFont *font;
  	SFTFont *new_font; // temporary variable until changes are okayed
  	CTEXTSTR fontname; // used to communicate with font preset subsystem
	//POINTER fontdata;
  	//uint32_t fontdatalen;
	PPAGE_DATA page;
	//TEXTCHAR *label_text; // allowed to override the real title...
	int offset;
	int min_offset;
	int max_offset;
	// need to figure out how to maintain this information
	CTEXTSTR preset_name;
	TEXTSTR last_text; // the last value set as the label content.
};

enum { BTN_VARNAME = 1322
	  , BTN_VARVAL };

PRIORITY_PRELOAD( AliasPageTitle, DEFAULT_PRELOAD_PRIORITY-3 )
{
	EasyRegisterResource( WIDE( "intershell/text" ), BTN_VARNAME, EDIT_FIELD_NAME );
	EasyRegisterResource( WIDE( "intershell/text" ), BTN_VARVAL, EDIT_FIELD_NAME );
}

typedef struct variable_tag VARIABLE;
struct variable_tag
{
	TEXTCHAR *name;
#define BITFIELD uint32_t
	struct {
		BITFIELD bString : 1;
		BITFIELD bInt : 1;
		BITFIELD bProc : 1;
		BITFIELD bProcEx : 1;
		BITFIELD bProcControlEx : 1;
		BITFIELD bConstString : 1;
		BITFIELD bValueString : 1;
		BITFIELD bParameterString : 1;
	} flags;
	union {
		CTEXTSTR *string_value;
		CTEXTSTR string_const_value;
		uint32_t *int_value;
		label_gettextproc proc;
		label_gettextproc_ex proc_ex;
		label_gettextproc_control proc_control_ex;
		label_value_proc value_proc;
		label_value_proc_parameter value_param_proc;
	} data;
	uintptr_t psvUserData; // passed to data.proc_ex
	TEXTCHAR tmpval[32];
	PLIST references; // PPAGE_LABELs which reference this variable...
	PLIST button_refs;
};

static struct label_local
{
	PLIST labels;
	PLIST bad_button_variable; // check these controls that have bad variables.
	PLIST bad_label_variables; // check these controls that have bad variables.

} l;

/*
#define NUM_VARIABLES ( sizeof( variables ) / sizeof( variables[0] ) )
VARIABLE variables[] = { { "Current User", .flags={1,0},.data={.string_value=&paper_global_data.pCurrentUserName } }
							  , { "Current Session", .flags={0,1},.data={.int_value=&paper_global_data.current_session } }
							  , { "Current Page", .flags={.bProc=1},.data={.proc=GetPageTitle } }
							  , { "Current Selected User Name", .flags={.bString=1},.data={.string_value=&user_local.name } }
							  , { "Current Selected User StaffID", .flags={.bString=1},.data={.string_value=&user_local.staff_id} }
//							  , { "Current Current User Total", .flags={.bProc=1},.data={.proc=GetPageTitle } }

};
*/
static PLIST extern_variables;


#undef CreateLabelVariableEx
PVARIABLE CreateLabelVariableEx( CTEXTSTR name, enum label_variable_types type, CPOINTER data, uintptr_t psv )
{
	if( name && name[0] )
	{
		PVARIABLE newvar = New( VARIABLE );
		MemSet( newvar, 0, sizeof( *newvar ) );
		newvar->name = StrDup( name );
		switch( type )
		{
		case LABEL_TYPE_PROC_PARAMETER:
			newvar->flags.bParameterString = 1;
			newvar->data.value_param_proc = (label_value_proc_parameter)data;
			break;
		case LABEL_TYPE_VALUE_STRING:
			newvar->flags.bValueString = 1;
			newvar->data.value_proc = (label_value_proc)data;
			newvar->psvUserData = psv;
			break;
		case LABEL_TYPE_STRING:
			newvar->flags.bString = 1;
			newvar->data.string_value = (CTEXTSTR*)data;
			break;
		case LABEL_TYPE_CONST_STRING:
			newvar->flags.bConstString = 1;
			newvar->data.string_const_value = StrDup((TEXTSTR)data);
			break;
		case LABEL_TYPE_INT:
			newvar->flags.bInt = 1;
			newvar->data.int_value = (uint32_t*)data;
			break;
		case LABEL_TYPE_PROC:
			newvar->flags.bProc = 1;
			newvar->data.proc = (label_gettextproc)data;
			break;
		case LABEL_TYPE_PROC_EX:
			newvar->flags.bProcEx = 1;
			newvar->data.proc_ex = (label_gettextproc_ex)data;
			newvar->psvUserData = psv;
			break;
		}
		AddLink( &extern_variables, newvar );
		{
			PLIST to_update = NULL;
			INDEX idx;
			PMENU_BUTTON button;
			LIST_FORALL( l.bad_button_variable, idx, PMENU_BUTTON, button )
			{
				//lprintf( "Had a bad variable...(button) and refreshing that will give us new text?!" );
				SetLink( &l.bad_button_variable, idx, NULL );
				AddLink( &to_update, button );
			}
			{
				PPAGE_LABEL label;
				INDEX idx;
				LIST_FORALL( l.bad_label_variables, idx, PPAGE_LABEL, label )
				{
					//lprintf( "Had a bad variable...(label) and refreshing that will give us new text?!" );
					SetLink( &l.bad_label_variables, idx, NULL );
					AddLink( &to_update, label->button );
				}
			}
			LIST_FORALL( to_update, idx, PMENU_BUTTON, button )
			{
				UpdateButton( button );
			}
			DeleteList( &to_update );

		}
		return newvar;
	}
	return NULL;
}

PVARIABLE CreateLabelVariable( CTEXTSTR name, enum label_variable_types type, CPOINTER data )
{
	if( type == LABEL_TYPE_PROC_EX )
	{
		xlprintf( LOG_ALWAYS )( WIDE( "Cannot Register an EX Proc tyep label with CreateLabelVariable!" ) );
		DebugBreak();
	}
	return CreateLabelVariableEx( name, type, data, 0 );
}

CTEXTSTR InterShell_GetLabelText( PPAGE_LABEL label, CTEXTSTR variable )
{
	return InterShell_GetControlLabelText( NULL, label, variable );
}

// somehow, variables need to get udpate events...
// which can then update the text labels referencing said variable.

//OnUpdateVariable( WIDE("Current User Total") )( TEXTCHAR *variable )
//{
//}

void CPROC ScrollingLabelUpdate( uintptr_t psv )
{
	PPAGE_LABEL label;
	INDEX idx;
	LIST_FORALL( l.labels, idx, PPAGE_LABEL, label )
	{
		if( label->flags.bScroll )
		{
			label->offset -= 1;
			if( !SetControlTextOffset( label->control, label->offset ) )
			{
	            label->offset = label->max_offset;
				SetControlTextOffset( label->control, label->offset );
			}
		}
	}
}

int GetHighlight( uintptr_t psv, PMENU_BUTTON button )
{
	if( InterShell_GetButtonHighlight( button ) )
		return 1;
	return 0;
}


static CTEXTSTR TestParam1( int64_t arg )
{
	static TEXTCHAR val[32];
	snprintf( val, 32, WIDE("%lld"), arg * 2 );
	return val;
}

static CTEXTSTR TestParam2( int64_t arg )
{
	static TEXTCHAR val[32];
	snprintf( val, 32, WIDE("%lld"), arg * 3 );
	return val;
}

static CTEXTSTR GetPageTitle( uintptr_t psv, PMENU_BUTTON control )
{
	PPAGE_DATA page = ShellGetCurrentPage( InterShell_GetButtonCanvas( control ) );
	return page->title?page->title:WIDE( "DEFAULT PAGE" );
}

PRELOAD( PreconfigureVariables )
{
	CreateLabelVariableEx( WIDE( "Current Page" ), LABEL_TYPE_PROC_CONTROL_EX, (POINTER)GetPageTitle, 0 );
	CreateLabelVariable( WIDE( "Highlight State" ), LABEL_TYPE_VALUE_STRING, (POINTER)GetHighlight );
	AddTimer( 50, ScrollingLabelUpdate, 0 );
}


static PVARIABLE FindVariableByName( CTEXTSTR variable )
{
	PVARIABLE var;
	INDEX nVar;
	if( !variable )
		return NULL; // can't have a NULL variable.
	LIST_FORALL( extern_variables, nVar, PVARIABLE, var )
	{
		//for( nVar = 0; nVar < NUM_VARIABLES; nVar++ )
		if( var->name )
		{
			size_t varnamelen = strlen( var->name );
			// if it's a tag-type declaration, don't compare the close character length
			// there is in-between data.
			if( var->flags.bParameterString && var->name[0] == '<' )
				varnamelen--;
			if( StrCaseCmpEx( variable, var->name, varnamelen ) == 0 )
			{
				if( var->flags.bParameterString && variable[varnamelen] != ':' )
				{
					// matched, but maybe it's a similar variable
					continue;
				}
				break;
			}
		}
	}
	return var;
}

static CTEXTSTR HandleValueProc( CTEXTSTR *pvariable, PVARIABLE var, TEXTCHAR *output, int output_len, int *pnOutput, PMENU_BUTTON button )
{
	int nOutput = (*pnOutput);
	CTEXTSTR variable = (*pvariable);
	CTEXTSTR values = variable + strlen( var->name ) + 1; //varnamelen;
	if( values[0] == ':' )
	{
		int option;
		TEXTSTR token;
		TEXTSTR tmp = StrDup( values + 1 );
		TEXTSTR value = tmp;
		PLIST opts = NULL;

		while( token = (TEXTSTR)StrChr( value, ',' ) )
		{
			// this is the final spot.
			token[0] = 0;
			AddLink( &opts, value );
         // if the next token is end of optoins... end.
			value = token + 1;
			if( token[1] == ';' )
				break;
		}
		if( !token )
		{
			// ended without an ender...
			AddLink( &opts, value );
			value += StrLen( value );
		}
		option = var->data.value_proc( var->psvUserData, button );
		nOutput += snprintf( output + nOutput, output_len - nOutput - 1
								 , WIDE("%s"), (char*)GetLink( &opts, option ) );
		//lprintf( WIDE("Before adjustment : [%s]"), variable );
		variable = variable + ( value - tmp ) + 2;
		(*pvariable) = variable;
		(*pnOutput) = nOutput;
		//lprintf( WIDE("AFter adjustment : [%s]"), variable );
		Release( tmp );
	}
	return output;
}

static CTEXTSTR HandleParameterProc( CTEXTSTR *pvariable, PVARIABLE var, TEXTCHAR *output, int output_len, int *pnOutput, PMENU_BUTTON button )
{
	int nOutput = (*pnOutput);
	CTEXTSTR variable = (*pvariable);
	TEXTCHAR end_string;
	int offset;
	CTEXTSTR values;// = variable + strlen( var->name ) + 1; //varnamelen;
	if( var->name[0] == '<' )
	{
		offset = 1;
		end_string = '>';
		values = variable + strlen( var->name ); // assume it's tag delimited, and paramter is inside tag.
	}
	else
	{
		offset = 0;
		end_string = ';';
		values = variable + strlen( var->name ) + 1; //varnamelen;
	}
	if( values[0] == ':' )
	{
		CTEXTSTR value_end = StrChr( values, end_string );
		int64_t param = IntCreateFromText( values + 1 );
		CTEXTSTR result = var->data.value_param_proc( param );

		nOutput += snprintf( output + nOutput, output_len - nOutput - 1
								 , WIDE("%s"), result );

		variable = variable + ( value_end - values + 1 - offset );  // one for the leading %, one for the end?
		(*pvariable) = variable;
		(*pnOutput) = nOutput;
	}
	return output;
}


CTEXTSTR InterShell_GetControlLabelText( PMENU_BUTTON button, PPAGE_LABEL label, CTEXTSTR variable )
{
	static TEXTCHAR output[256];
	static int buffer_len = 256;
	int nOutput = 0;
	while( variable && variable[0] )
	{
		if( variable[0] == '%' )
		{
			if( variable[1] == '%' )
			{
				if( nOutput < buffer_len )
				{
					output[nOutput] = variable[0];
					nOutput++;
				}
				variable++; // skip over the extra percent... (fall through to bottom which wil increment over first '%')
			}
			else
			{
				PVARIABLE var = FindVariableByName( variable + 1);
				if( var )
				{
					if( var->flags.bParameterString )
					{
						HandleParameterProc( &variable, var, output, sizeof( output ), &nOutput, button );
					}
					else if( var->flags.bValueString )
					{
						HandleValueProc( &variable, var, output, sizeof( output ), &nOutput, button );
					}
					else if( var->flags.bString )
					{
						if( *var->data.string_value )
							nOutput += snprintf( output + nOutput, sizeof( output ) - (nOutput - 1)*sizeof(TEXTCHAR)
													 , WIDE("%s"), (*var->data.string_value) );
					}
					else if( var->flags.bConstString )
					{
					nOutput += snprintf( output + nOutput, sizeof( output ) - (nOutput - 1)*sizeof(TEXTCHAR)
											 , WIDE("%s"), (var->data.string_const_value?var->data.string_const_value:WIDE("")) );
					}
					else if( var->flags.bInt )
					{
						nOutput += snprintf( output + nOutput, sizeof( output ) - (nOutput - 1)*sizeof(TEXTCHAR)
												 , WIDE("%ld"), (*var->data.int_value) );
					}
					else if( var->flags.bProc )
					{
						//lprintf( "Calling external function to get value..." );
						nOutput += snprintf( output + nOutput, sizeof( output ) - (nOutput - 1)*sizeof(TEXTCHAR)
												 , WIDE("%s"), var->data.proc() );
						//lprintf( "New output is [%s]", output );
					}
					else if( var->flags.bProcControlEx )
					{
						//lprintf( "Calling external function to get value..." );
						nOutput += snprintf( output + nOutput, sizeof( output ) - (nOutput - 1)*sizeof(TEXTCHAR)
												 , WIDE("%s"), var->data.proc_control_ex(var->psvUserData,button) );
						//lprintf( "New output is [%s]", output );
					}
					else if( var->flags.bProcEx )
					{
						//lprintf( "Calling external extended function to get value..." );
						nOutput += snprintf( output + nOutput, sizeof( output ) - (nOutput - 1)*sizeof(TEXTCHAR)
												 , WIDE("%s"), var->data.proc_ex( var->psvUserData ) );
						//lprintf( "New output is [%s]", output );
					}
					else
					{
						nOutput += snprintf( output + nOutput, sizeof( output ) - (nOutput - 1)*sizeof(TEXTCHAR)
												 , WIDE("%%%s"), var->name );
					}
					if( label )
					{
						if( FindLink( &var->references, label ) == INVALID_INDEX )
							AddLink( &var->references, label );
					}
					else if( button )
					{
						if( FindLink( &var->button_refs, button ) == INVALID_INDEX )
							AddLink( &var->button_refs, button );
					}
					variable += strlen( var->name ); //varnamelen;
				}
				else
				{
					CTEXTSTR env=StrChr( variable + 1, '%' );
					if( env )
					{
						CTEXTSTR env_var;
						TEXTSTR tmp = NewArray( TEXTCHAR, env - variable );
						StrCpyEx( tmp, variable+1, (env-variable) );
						//lprintf( WIDE("failed var try %s"), tmp );
#ifdef HAVE_ENVIRONMENT
#ifdef __ANDROID__
						if( StrCaseCmp( tmp, "hostname" ) == 0 ||
							StrCaseCmp( tmp, "computername" ) == 0 )
							env_var = GetSystemName();
                  else
#endif
							env_var = OSALOT_GetEnvironmentVariable( tmp );
						lprintf( "test %s=%s", tmp, env_var );
						if( env_var )
						{
							nOutput += snprintf( output + nOutput, sizeof( output ) - (nOutput - 1)*sizeof(TEXTCHAR)
													 , WIDE( "%s" ), env_var );
							variable += (env-variable);
						}
						else
#endif
						{
#ifdef OUTPUT_BAD_VARIABLES
							nOutput += snprintf( output + nOutput, sizeof( output ) - (nOutput - 1)*sizeof(TEXTCHAR)
													 , WIDE("[bad variable]") );
#endif
						}
					}
					else
					{
						if( button )
						{
							if( FindLink( &l.bad_button_variable, button ) == INVALID_INDEX )
								AddLink( &l.bad_button_variable, button );
						}
						if( label )
						{
							if( FindLink( &l.bad_label_variables, label ) == INVALID_INDEX )
								AddLink( &l.bad_label_variables, label );
						}

#ifdef OUTPUT_BAD_VARIABLES
						nOutput += snprintf( output + nOutput, sizeof( output ) - (nOutput - 1)*sizeof(TEXTCHAR)
												 , WIDE("[bad variable]") );
#endif
					}
					var = (PVARIABLE)1;
				}
				if( !var )
				{
					static int n;
					n++;
					//lprintf( "is a button? might be badvar? %p %p", button, label );
					if( button )
					{
						if( FindLink( &l.bad_button_variable, button ) == INVALID_INDEX )
						{
							AddLink( &l.bad_button_variable, button );
						}
					}
					if( label )
					{
						if( FindLink( &l.bad_label_variables, label ) == INVALID_INDEX )
						{
							AddLink( &l.bad_label_variables, label );
						}
					}
#ifdef OUTPUT_BAD_VARIABLES
					nOutput += snprintf( output + nOutput, sizeof( output ) - (nOutput - 1)*sizeof(TEXTCHAR)
											 , WIDE("[bad variable(%d)]"), n );
#endif
				}
			}
		}
		else
		{
			if( nOutput < 255 )
			{
				output[nOutput] = variable[0];
				nOutput++;
			}
		}
		variable++;
	}
	if( nOutput < buffer_len )
		output[nOutput] = 0;
	else
		output[buffer_len-1] = 0; // make sure it's nul terminated
	return output;
}

CTEXTSTR InterShell_TranslateLabelTextEx( PMENU_BUTTON button, PPAGE_LABEL label, TEXTSTR output, int buffer_len, CTEXTSTR variable )
{
	int nOutput = 0;
	while( variable && variable[0] )
	{
		if( variable[0] == '%' )
		{
			if( variable[1] == '%' )
			{
				if( nOutput < buffer_len )
				{
					output[nOutput] = variable[0];
					nOutput++;
				}
				variable++; // skip over the extra percent... (fall through to bottom which wil increment over first '%')
			}
			else
			{
				PVARIABLE var = FindVariableByName( variable + 1 );
				//for( nVar = 0; nVar < NUM_VARIABLES; nVar++ )
				if( var )
				{
					if( var->flags.bParameterString )
					{
						HandleParameterProc( &variable, var, output, buffer_len, &nOutput, button );
					}
					else if( var->flags.bValueString )
					{
						HandleValueProc( &variable, var, output, buffer_len, &nOutput, button );
					}
					else if( var->flags.bString )
					{
						nOutput += snprintf( output + nOutput, buffer_len - (nOutput - 1)*sizeof(TEXTCHAR)
												 , WIDE("%s"), (*var->data.string_value) );
					}
					else if( var->flags.bConstString )
					{
						nOutput += snprintf( output + nOutput, buffer_len - (nOutput - 1)*sizeof(TEXTCHAR)
												 , WIDE("%s"), (var->data.string_const_value?var->data.string_const_value:WIDE("")) );
					}
					else if( var->flags.bInt )
					{
						nOutput += snprintf( output + nOutput, buffer_len - (nOutput - 1)*sizeof(TEXTCHAR)
												 , WIDE("%ld"), (*var->data.int_value) );
					}
					else if( var->flags.bProc )
					{
						//lprintf( "Calling external function to get value..." );
						nOutput += snprintf( output + nOutput, buffer_len - (nOutput - 1)*sizeof(TEXTCHAR)
												 , WIDE("%s"), var->data.proc() );
					//lprintf( "New output is [%s]", output );
					}
					else
					{
						nOutput += snprintf( output + nOutput, buffer_len - (nOutput - 1)*sizeof(TEXTCHAR)
												 , WIDE("%%%s"), var->name );
					}
					if( label )
					{
						if( FindLink( &var->references, label ) == INVALID_INDEX )
							AddLink( &var->references, label );
					}
					else if( button )
					{
						if( FindLink( &var->button_refs, button ) == INVALID_INDEX )
							AddLink( &var->button_refs, button );
					}
					variable += strlen( var->name ); //varnamelen;
				}
				else
				{
					CTEXTSTR env=StrChr( variable + 1, '%' );
					if( env )
					{
						CTEXTSTR env_var;
						TEXTSTR tmp = NewArray( TEXTCHAR, env - variable );
						StrCpyEx( tmp, variable+1, (env-variable) );
						//lprintf( WIDE("failed var try %s"), tmp );
#ifdef HAVE_ENVIRONMENT
						env_var = OSALOT_GetEnvironmentVariable( tmp );
						if( env_var )
						{
							nOutput += snprintf( output + nOutput, buffer_len - (nOutput - 1)*sizeof(TEXTCHAR)
													 , WIDE( "%s" ), env_var );
							variable += (env-variable);
						}
						else
#endif
						{
#ifdef OUTPUT_BAD_VARIABLES
							nOutput += snprintf( output + nOutput, buffer_len - (nOutput - 1)*sizeof(TEXTCHAR)
													 , WIDE("[bad variable]") );
#endif
						}
					}
					else
					{
						if( button )
						{
							if( FindLink( &l.bad_button_variable, button ) == INVALID_INDEX )
								AddLink( &l.bad_button_variable, button );
						}
						if( label )
						{
							if( FindLink( &l.bad_label_variables, label ) == INVALID_INDEX )
								AddLink( &l.bad_label_variables, label );
						}

#ifdef OUTPUT_BAD_VARIABLES
						nOutput += snprintf( output + nOutput, buffer_len - (nOutput - 1)*sizeof(TEXTCHAR)
												 , WIDE("[bad variable]") );
#endif
					}
				}
			}
		}
		else
		{
			if( nOutput < 255 )
			{
				output[nOutput] = variable[0];
				nOutput++;
			}
		}
		// otherwise it's ended.
		if( variable[0] )
			variable++;
	}
	if( nOutput < buffer_len )
	{
		output[nOutput] = 0;
	}
	else
		output[buffer_len-1] = 0; // make sure it's nul terminated.
	return output;
}

CTEXTSTR InterShell_TranslateLabelText( PPAGE_LABEL label, TEXTSTR output, int buffer_len, CTEXTSTR variable )
{
	return InterShell_TranslateLabelTextEx( NULL, label, output, buffer_len, variable );
}


void LabelVariableChanged( PVARIABLE variable) // update any labels which are using this variable.
{
	PVARIABLE var;
	INDEX nVar;
	LIST_FORALL( extern_variables, nVar, PVARIABLE, var )
	{
		if( ( !variable ) || ( var == variable ) )
		{
			INDEX idx;
			PPAGE_LABEL label;
			PMENU_BUTTON button;
			LIST_FORALL( var->references, idx, PPAGE_LABEL, label )
			{
				CTEXTSTR tmp;
				tmp = InterShell_GetControlLabelText( label->button, label, label->button->text );
				//lprintf("Got one.");
				if( label->last_text && StrCmp( label->last_text, tmp ) == 0 )
				{
					continue;
				}
				else
				{
					Release( label->last_text );
					label->last_text = StrDup( tmp );
				}
				//lprintf( "Query show... update caption" );
				SetControlText( label->control, tmp );
				GetControlTextOffsetMinMax( label->control, &label->min_offset, &label->max_offset );
				//SmudgeCommon( label->control );
			}
			LIST_FORALL( var->button_refs, idx, PMENU_BUTTON, button )
			{
				UpdateButton( button );
			}
		}
	}
}

void LabelVariablesChanged( PLIST variables) // update any labels which are using this variable.
{
	PVARIABLE var;
	INDEX idx;
	LIST_FORALL( variables, idx, PVARIABLE, var )
		LabelVariableChanged( var );
}


static void OnDestroyControl( TEXT_LABEL_NAME )( uintptr_t psv )
{
	PPAGE_LABEL title = (PPAGE_LABEL)psv;
	PVARIABLE var;
	INDEX nVar;
	LIST_FORALL( extern_variables, nVar, PVARIABLE, var )
	{
		DeleteLink( &var->references, (POINTER)psv );
	}
	DeleteLink( &l.labels, title );
	DestroyCommon( &title->control );
	Release( title );

}

static uintptr_t OnCreateControl( TEXT_LABEL_NAME )( PSI_CONTROL frame, int32_t x, int32_t y, uint32_t w, uint32_t h )
{
	PPAGE_LABEL title = New( struct page_label );
	MemSet( title, 0, sizeof( *title ) );
	title->canvas = frame;
	title->page = ShellGetCurrentPage( frame );
	title->button = InterShell_GetCurrentlyCreatingButton();
	title->control = MakeControl( frame, STATIC_TEXT, x, y, w, h, -1 );
	SetCommonBorder( title->control, BORDER_FIXED|BORDER_NONE );
	//SetControlAlignment( title->control, 2 );
	SetTextControlColors( title->control, BASE_COLOR_WHITE, 0 );
	SetCommonTransparent( title->control, TRUE );
	AddLink( &l.labels, title );
	return (uintptr_t)title;
}

static PSI_CONTROL OnGetControl( TEXT_LABEL_NAME )( uintptr_t psv )
//PSI_CONTROL CPROC GetTitleControl( uintptr_t psv )
{
	PPAGE_LABEL title = (PPAGE_LABEL)psv;
	if( title )
		return title->control;
	else
		DebugBreak();
	return NULL;
}

static void OnSaveControl( TEXT_LABEL_NAME )( FILE *file,uintptr_t psv )
{
	PPAGE_LABEL title = (PPAGE_LABEL)psv;
	sack_fprintf( file, WIDE("%scolor=%s\n"), InterShell_GetSaveIndent(), FormatColor( title->color ) );
	sack_fprintf( file, WIDE("%sbackground color=%s\n"), InterShell_GetSaveIndent(), FormatColor( title->back_color ) );
	sack_fprintf( file, WIDE( "%salign center?%s\n" ), InterShell_GetSaveIndent(), title->flags.bCenter?WIDE( "on" ):WIDE( "off" ) );
	sack_fprintf( file, WIDE( "%salign right?%s\n" ), InterShell_GetSaveIndent(), title->flags.bRight?WIDE( "on" ):WIDE( "off" ) );
	sack_fprintf( file, WIDE( "%salign scroll?%s\n" ), InterShell_GetSaveIndent(), title->flags.bScroll?WIDE( "on" ):WIDE( "off" ) );
	sack_fprintf( file, WIDE( "%salign vertical?%s\n" ), InterShell_GetSaveIndent(), title->flags.bVertical?WIDE( "on" ):WIDE( "off" ) );
	sack_fprintf( file, WIDE( "%salign inverted?%s\n" ), InterShell_GetSaveIndent(), title->flags.bInverted?WIDE( "on" ):WIDE( "off" ) );
	if( title->preset_name )
	{
		sack_fprintf( file, WIDE("%sfont name=%s\n"), InterShell_GetSaveIndent(),title->preset_name );
	}
	if( title->button->text )
		sack_fprintf( file, WIDE("%slabel=%s\n"), InterShell_GetSaveIndent(), EscapeMenuString( title->button->text ) );
}


static uintptr_t CPROC SetTitleLabel( uintptr_t psv, arg_list args )
{
	PPAGE_LABEL title = (PPAGE_LABEL)psv;
	PARAM( args, TEXTCHAR *, label );
	InterShell_SetButtonText( title->button, label );
	return psv;
}

static uintptr_t CPROC SetTitleColor( uintptr_t psv, arg_list args )
{
	PPAGE_LABEL title = (PPAGE_LABEL)psv;
	PARAM( args, CDATA, color );
	title->color = color;
	return psv;
}

static uintptr_t CPROC SetTitleBackColor( uintptr_t psv, arg_list args )
{
	PPAGE_LABEL title = (PPAGE_LABEL)psv;
	PARAM( args, CDATA, color );
	title->back_color = color;
	return psv;
}

static uintptr_t CPROC SetTitleCenter( uintptr_t psv, arg_list args )
{
	PPAGE_LABEL title = (PPAGE_LABEL)psv;
	PARAM( args, LOGICAL, center );
	title->flags.bCenter = center;
	return psv;
}

static uintptr_t CPROC SetTitleTextShadow( uintptr_t psv, arg_list args )
{
	PPAGE_LABEL title = (PPAGE_LABEL)psv;
	PARAM( args, LOGICAL, center );
	title->flags.bShadow = center;
	return psv;
}

static uintptr_t CPROC SetTitleVertical( uintptr_t psv, arg_list args )
{
	PPAGE_LABEL title = (PPAGE_LABEL)psv;
	PARAM( args, LOGICAL, vertical );
	title->flags.bVertical = vertical;
	return psv;
}

static uintptr_t CPROC SetTitleInverted( uintptr_t psv, arg_list args )
{
	PPAGE_LABEL title = (PPAGE_LABEL)psv;
	PARAM( args, LOGICAL, inverted );
	title->flags.bInverted = inverted;
	return psv;
}

static uintptr_t CPROC SetTitleRight( uintptr_t psv, arg_list args )
{
	PPAGE_LABEL title = (PPAGE_LABEL)psv;
	PARAM( args, LOGICAL, right );
	title->flags.bRight = right;
	return psv;
}

static uintptr_t CPROC SetTitleScrollRightLeft( uintptr_t psv, arg_list args )
{
	PPAGE_LABEL title = (PPAGE_LABEL)psv;
	PARAM( args, LOGICAL, scroll );
	title->flags.bScroll = scroll;
	return psv;
}

static uintptr_t CPROC SetTitleFontByName( uintptr_t psv, arg_list args )
{
	PPAGE_LABEL title = (PPAGE_LABEL)psv;
	PARAM( args, TEXTCHAR *, name );
	title->font = UseACanvasFont( title->canvas, name );
	title->preset_name = StrDup( name );
	return psv;
}

static void OnLoadControl( TEXT_LABEL_NAME )( PCONFIG_HANDLER pch, uintptr_t psv )
{
	AddConfigurationMethod( pch, WIDE("color=%c"), SetTitleColor );
	AddConfigurationMethod( pch, WIDE("background color=%c"), SetTitleBackColor );
	AddConfigurationMethod( pch, WIDE("font name=%m"), SetTitleFontByName );
	AddConfigurationMethod( pch, WIDE("label=%m"), SetTitleLabel );
	AddConfigurationMethod( pch, WIDE("align center?%b"), SetTitleCenter );
	AddConfigurationMethod( pch, WIDE("align scroll?%b"), SetTitleScrollRightLeft );
	AddConfigurationMethod( pch, WIDE("align right?%b"), SetTitleRight );
	AddConfigurationMethod( pch, WIDE("align vertical?%b"), SetTitleVertical );
	AddConfigurationMethod( pch, WIDE("align vertical?%b"), SetTitleTextShadow );
	AddConfigurationMethod( pch, WIDE("align inverted?%b"), SetTitleInverted );
}

static LOGICAL OnQueryShowControl( TEXT_LABEL_NAME )( uintptr_t psv )
{
	return TRUE;
}

static void OnShowControl( TEXT_LABEL_NAME )( uintptr_t psv )
{
	PPAGE_LABEL title = (PPAGE_LABEL)psv;
	SetCommonFont( title->control, (title->font?(*title->font):NULL) );
	SetTextControlColors( title->control, title->color, title->back_color );
	//lprintf( "On show... update caption" );
	SetControlText( title->control, InterShell_GetControlLabelText( title->button, title, title->button->text ) );
	GetControlTextOffsetMinMax( title->control, &title->min_offset, &title->max_offset );
	if( title->flags.bCenter )
		SetControlAlignment( title->control, TEXT_CENTER );
	else if( title->flags.bRight )
		SetControlAlignment( title->control, TEXT_RIGHT );
	else
		SetControlAlignment( title->control, TEXT_NORMAL );

	//SmudgeCommon( title->control );
}

static void CPROC PickLabelFont( uintptr_t psv, PSI_CONTROL pc )
{
	PPAGE_LABEL page_label = (PPAGE_LABEL)psv;
	SFTFont *font = SelectACanvasFont( InterShell_GetButtonCanvas( page_label->button )
									, GetFrame( pc )
									, &page_label->preset_name
									);
	if( font )
	{
		page_label->new_font = font;
	}
}


void CPROC VariableChanged( uintptr_t psv, PSI_CONTROL pc, PLISTITEM pli )
{
	PSI_CONTROL pc_text = (PSI_CONTROL)psv;
	if( pc_text )
	{
		TEXTCHAR buffer[256];
		GetItemText( pli, sizeof(buffer)-1, buffer+1 );
		buffer[0] = '%';
		TypeIntoEditControl( pc_text, buffer );
	}

}

void CPROC FillVariableList( PSI_CONTROL frame )
{
	PSI_CONTROL list = GetControl( frame, LST_VARIABLES );
	if( list )
	{
		PVARIABLE var;
		INDEX i;
		LIST_FORALL( extern_variables, i, PVARIABLE, var )
			//for( i = 0; i < NUM_VARIABLES; i++ )
		{
			AddListItem( list, var->name );
		}
		SetSelChangeHandler( list, VariableChanged, (uintptr_t)GetControl( frame, TXT_CONTROL_TEXT ) );
	}
}

static uintptr_t OnEditControl( TEXT_LABEL_NAME )( uintptr_t psv, PSI_CONTROL parent_frame )
{
	PPAGE_LABEL page_label = (PPAGE_LABEL)psv;
	if( page_label )
	{
		PSI_CONTROL frame;
		int okay = 0;
		int done = 0;
		// psv may be passed as NULL, and therefore there was no task assicated with this
		// button before.... the button is blank, and this is an initial creation of a button of this type.
		// basically this should call (psv=CreatePaper(button)) to create a blank button, and then launch
		// the config, and return the button created.
		//PPAPER_INFO issue = button->paper;
		frame = LoadXMLFrame( WIDE("page_label_property.isframe") );
		if( frame )
		{
			//could figure out a way to register methods under
			// the filename of the property thing being loaded
			// for future use...
			{ // init frame which was loaded..
				SetCommonButtons( frame, &done, &okay );
				page_label->new_font = NULL;
				//EnableColorWellPick( SetColorWell( GetControl( frame, CLR_BACKGROUND), page_label->button->color ), TRUE );
				EnableColorWellPick( SetColorWell( GetControl( frame, CLR_TEXT_COLOR), page_label->color ), TRUE );
				EnableColorWellPick( SetColorWell( GetControl( frame, CLR_BACKGROUND), page_label->back_color ), TRUE );
				//EnableColorWellPick( SetColorWell( GetControl( frame, CLR_RING_BACKGROUND), page_label->button->secondary_color ), TRUE );
				SetCheckState( GetControl( frame, CHECKBOX_LABEL_SHADOW ), page_label->flags.bShadow );
				SetCheckState( GetControl( frame, CHECKBOX_LABEL_CENTER ), page_label->flags.bCenter );
				SetCheckState( GetControl( frame, CHECKBOX_LABEL_SCROLL ), page_label->flags.bScroll );
				SetCheckState( GetControl( frame, CHECKBOX_LABEL_RIGHT ), page_label->flags.bRight );
				SetButtonPushMethod( GetControl( frame, BTN_PICKFONT ), PickLabelFont, psv );
				{
					TEXTCHAR buffer[256];
#define BUFSIZE (sizeof(buffer)/sizeof(buffer[0]))
					int o = 0;
					CTEXTSTR string = page_label->button->text;
					int n;
					if( string ) for( n = 0; ( o < (sizeof(buffer)/sizeof(buffer[0])) ) && string[n]; n++ )
					{
						switch( string[n] )
						{
						case '\n':
							buffer[o++] = '\\';
							buffer[o++] = 'n';
							break;
						case '\t':
							buffer[o++] = '\\';
							buffer[o++] = 't';
							break;
						default:
							buffer[o++] = string[n];
						}
					}
					if( o < (BUFSIZE-1) )
						buffer[o] = 0;
					else
						buffer[BUFSIZE-1] = 0;
					SetControlText( GetControl( frame, TXT_CONTROL_TEXT ), buffer );
				}
				{
					PSI_CONTROL list = GetControl( frame, LST_VARIABLES );
					if( list )
					{
						PVARIABLE var;
						INDEX i;
						LIST_FORALL( extern_variables, i, PVARIABLE, var )
						//for( i = 0; i < NUM_VARIABLES; i++ )
						{
							AddListItem( list, var->name );
						}
						SetSelChangeHandler( list, VariableChanged, (uintptr_t)GetControl( frame, TXT_CONTROL_TEXT ) );
					}
				}
			}
			DisplayFrameOver( frame, parent_frame );

			EditFrame( frame, TRUE );
			//edit frame must be done after the frame has a physical surface...
			// it's the surface itself that allows the editing...
			CommonWait( frame );
			if( okay )
			{
				// blah get the resuslts...
				if( page_label->new_font )
					page_label->font = page_label->new_font;
				page_label->color = GetColorFromWell( GetControl( frame, CLR_TEXT_COLOR ) );
				page_label->back_color = GetColorFromWell( GetControl( frame, CLR_BACKGROUND ) );
				page_label->flags.bShadow = GetCheckState( GetControl( frame, CHECKBOX_LABEL_SHADOW ) );
				page_label->flags.bCenter = GetCheckState( GetControl( frame, CHECKBOX_LABEL_CENTER ) );
				if( !page_label->flags.bCenter )
					page_label->flags.bRight = GetCheckState( GetControl( frame, CHECKBOX_LABEL_RIGHT ) );
				else
					page_label->flags.bRight = 0;
				page_label->flags.bScroll = GetCheckState( GetControl( frame, CHECKBOX_LABEL_SCROLL ) );

				{
					TEXTCHAR buffer[256];
					int i,o;
					GetControlText( GetControl( frame, TXT_CONTROL_TEXT ), buffer, sizeof( buffer ) );
					for( i = o = 0; buffer[i]; i++,o++ )
					{
						if( buffer[o] == '\\' )
						{
							switch( buffer[o+1] )
							{
							case 'n':
								i++;
								buffer[o] = '\n';
								break;
							case 't':
								i++;
								buffer[o] = '\t';
								break;
							}
						}
					}
					if( page_label->button->text )
						Release( page_label->button->text );
					page_label->button->text = StrDup( buffer );
				}
			}
			DestroyFrame( &frame );
		}
	}
	return psv;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//  Begin button method that can set variable text...
//  useful for setting in macros to indicate current task mode? or perhaps
//  status messages to indicate what needs to be done now?
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct button_set_text {
	TEXTSTR varname;
	TEXTSTR newval;
};
typedef struct button_set_text SETVAR;
typedef struct button_set_text *PSETVAR;

void SetVariable( CTEXTSTR name, CTEXTSTR value )
{
	PVARIABLE pVar = FindVariableByName( name );
	if( !pVar )
		pVar = CreateLabelVariableEx( name, LABEL_TYPE_CONST_STRING, value, 0 );
	else if( pVar->flags.bConstString )
	{
		/* this isn't const data... or better not be... */
		Release( (POINTER)pVar->data.string_const_value );
		if( value )
			pVar->data.string_const_value = StrDup( value );
		else
			pVar->data.string_const_value = NULL;
		LabelVariableChanged( pVar );
	}
	else
		lprintf( WIDE( "Attempt to set a variable that is not direct text, cannot override routines...name:%s newval:%s" )
				 , name, value );
}

static void OnKeyPressEvent( WIDE( "InterShell/Set Variable" ) )( uintptr_t psv )
{
	PSETVAR pSetVar = (PSETVAR)psv;
	SetVariable( pSetVar->varname, pSetVar->newval );

	//return 1;
}

static uintptr_t OnCreateMenuButton( WIDE( "InterShell/Set Variable" ) )( PMENU_BUTTON button )
{
	PSETVAR pSetVar = New( SETVAR );
	pSetVar->varname = NULL;
	pSetVar->newval = NULL;
	InterShell_SetButtonStyle( button, WIDE( "bicolor square" ) );

	return (uintptr_t)pSetVar;
}

static uintptr_t OnConfigureControl( WIDE( "InterShell/Set Variable" ) )( uintptr_t psv, PSI_CONTROL parent )
{
	PSETVAR pSetVar = (PSETVAR)psv;
	PSI_CONTROL frame;
	frame = LoadXMLFrameOver( parent, WIDE( "configure_text_setvar_button.isframe" ) );
	if( frame )
	{
		int okay = 0;
		int done = 0;
		SetControlText( GetControl( frame, BTN_VARNAME ), pSetVar->varname );
		SetControlText( GetControl( frame, BTN_VARVAL ), pSetVar->newval );
		FillVariableList( frame );
		SetCommonButtons( frame, &done, &okay );
		DisplayFrameOver( frame, parent );
		CommonWait( frame );
		if( okay )
		{
			TEXTCHAR buffer[256];
			GetControlText( GetControl( frame, BTN_VARNAME ), buffer, sizeof( buffer )  );
			if( ( pSetVar->varname && strcmp( pSetVar->varname, buffer ) ) || ( !pSetVar->varname && buffer[0] ) )
			{
				Release( pSetVar->varname );
				pSetVar->varname = StrDup( buffer );
			}
			GetControlText( GetControl( frame, BTN_VARVAL ), buffer, sizeof( buffer ) );
			if( ( pSetVar->newval && strcmp( pSetVar->newval, buffer ) ) || ( !pSetVar->newval && buffer[0] ) )
			{
				Release( pSetVar->newval );
				pSetVar->newval = StrDup( buffer );
			}
		}
		{
			PVARIABLE pVar = FindVariableByName( pSetVar->varname );
			if( !pVar )
				pVar = CreateLabelVariableEx( pSetVar->varname, LABEL_TYPE_CONST_STRING, pSetVar->newval, 0 );
		}
		DestroyFrame( &frame );
	}
	return psv;
}

static void OnSaveControl( WIDE( "InterShell/Set Variable" ) )( FILE *file, uintptr_t psv )
{
	PSETVAR pSetVar = (PSETVAR)psv;
	sack_fprintf( file, WIDE( "set variable text name=%s\n" ), EscapeMenuString( pSetVar->varname ) );
	sack_fprintf( file, WIDE( "set variable text value=%s\n" ), EscapeMenuString( pSetVar->newval ) );
}

static void OnCloneControl( WIDE( "InterShell/Set Variable" ) )( uintptr_t psvNew, uintptr_t psvOld )
{
	PSETVAR pSetVarNew = (PSETVAR)psvNew;
	PSETVAR pSetVarOld = (PSETVAR)psvOld;
	pSetVarNew->varname = StrDup( pSetVarOld->varname );
	pSetVarNew->newval = StrDup( pSetVarOld->newval );
}

static uintptr_t CPROC SetVariableVariableName( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, name );
	PSETVAR pSetVar = (PSETVAR)psv;
	pSetVar->varname = StrDup( name );
	{
		PVARIABLE pVar = FindVariableByName( pSetVar->varname );
		if( !pVar )
			pVar = CreateLabelVariableEx( pSetVar->varname, LABEL_TYPE_CONST_STRING, NULL, 0 );
 	}
	return psv;
}

static uintptr_t CPROC SetVariableVariableValue( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, name );
	PSETVAR pSetVar = (PSETVAR)psv;
	pSetVar->newval = StrDup( name );
	return psv;
}

static void OnLoadControl( WIDE( "InterShell/Set Variable" ) )( PCONFIG_HANDLER pch, uintptr_t psv )
{
	AddConfigurationMethod( pch,  WIDE( "set variable text name=%m" ), SetVariableVariableName );
	AddConfigurationMethod( pch,  WIDE( "set variable text value=%m" ), SetVariableVariableValue );
}

#undef SetTextLabelOptions
void SetTextLabelOptions( PMENU_BUTTON label, LOGICAL center, LOGICAL right, LOGICAL scroll, LOGICAL shadow )
{
	PPAGE_LABEL page_label = (PPAGE_LABEL)InterShell_GetButtonExtension( label );
	if( page_label )
	{
		page_label->flags.bShadow = shadow;
		page_label->flags.bCenter = center;
		if( !page_label->flags.bCenter )
			page_label->flags.bRight = right;
		else
			page_label->flags.bRight = 0;
		page_label->flags.bScroll = scroll;
	}
}

INTERSHELL_NAMESPACE_END

