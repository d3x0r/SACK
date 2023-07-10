#ifndef CORE_SOURCE
#define CORE_SOURCE
#endif

#define DO_LOGGING
#include <logging.h>
#include <stdhdrs.h>
//#include "types.h"

#include <sharemem.h>
#include <string.h>
#include <stdlib.h> // exit()

#include <stdio.h>

#include "input.h"
#include "space.h"

//--------------------------------------------------------------------------


	//------------- system commands
 FunctionProto  VERSION,
	PLUGIN,
	UNLOAD,
	STORY,	  // show introduction story....
	PROMPT,	 // displays the current prompt....
	EXIT,
	CHANGEDIR, // change current working directory....
	SCRIPT,	 // call run external command script [file]
	PARSE,	  // just run a file (param1) through the parser
	COMMAND,	// open a device as command channel of sentience...
	FILTER,	 // apply filters to datapaths... 
	OptionDevice,  // set options on a datapath(filter)
	CMD_GETWORD,	// get next word from current file being parsed...
	CMD_GETLINE,	// get until next line terminator (unwraps quotes?)
	CMD_GETPARTIAL,// get any partial expression from the parser... clears it.
	CMD_ENDPARSE,  // Stop parsing this file...
	CMD_ENDCOMMAND,// Stop parsing this file...
//	Filter, // /filter command/data devname device options
//	UnFilter, // unfilter <devname> - removes a filter

	//FORMATTED, // data input is not to be run through burst... merely gatherline...
	WAKE,		// wake an object to sentient mode....
	SUSPEND,	// stop an aware process from executing temporily
	RESUME,	 // start a process which has been suspended...
	DELAY,	  // pause a macro for ### milliseconds
	GetDelay,  // useful is the macro was resumed from a delay...
	KILL,		// put an aware object to sleep (permanently)

	ECHO,		// output to command data path...
	FECHO,	  // formatted echo.. rare - but does not have linefeed prefix...
	CMD_PROCESS,// process <...> as if entered as a command 
	CMD_RUN,	 // Run a macro to completion before handling command input
	DECLARE,	 // create a variable type reference
	COLLAPSE,	// make data in a variable atomic
	BINARY,	  // create a variable without substitution applied
	ALLOCATE,	// allocate a binary storage buffer of size...
	UNDEFINE,	// delete a variable...
	VARS,		 // dump what current variables are present...
	VVARS,		// dump volatile variables (these are getting numerous)
	TELL,		 // add input to a sentient object
	REPLY,		// uses the name of the object which told this to perform the action...
	SendToObject, // <to object> <data (inbound channel)>
	WriteToObject, // <to object> <data (outbound channel)>
	MONITOR,	 // observe a previously aware object
	RELAY,		// auto relay command input to data output while data open
	PAGE, // generate a page break....
	COUNT, // count the number of things matching name

	CMD_INPUT,	  // get next command input into a variable... whole line...

	//------------- object definition/declaration
	CREATE,	  // [objectname]
	CreateRegisteredObject, // <object type> <object names....>
	_SHADOW,	 // create a shadow of an object... within self...
	BECOME,	  // leave your body and become another object.
	_DUPLICATE,  // object in hand.... [objectname]
	DESTROY,	 // object in hand ... [objectname]
	DESCRIBE,	// attach a description for LOOKat(???)
 RENAME,	  // change an object's name...
 ListExtensions,

	//------------- logicals
	// actually these may/should be implemented by state tables... Condition/Action
	CMD_IF,
	CMD_ELSE,
	CMD_ENDIF,
	CMD_WHILE,
	CMD_UNTIL,
 CMD_LABEL,  // takes a parameter...
 ForEach,
 StepEach,
	CMD_GOTO,
	CMD_RETURN,
	CMD_COMPARE,
	CMD_STOP,	// forces current macro to end...
	VAR_PUSH,	// push a variable onto the current var stack...
	VAR_POP,
	VAR_HEAD,
	VAR_TAIL,
	CMD_BURST, // feed data through the parser... introduce result into Data...
	CMD_WAIT,
	CMD_RESULT, // set result from macro....
	CMD_GETRESULT, // get the value of a result into a variable...
	DefineOnBehavior, // define action for an object
	
	// ------------ math type operations
	INCREMENT, // var amount (1 default)
	DECREMENT, // var amount (1 default)
	MULTIPLY,  // var amount (must have both)
	DIVIDE,	 // var amount (must have both)
	BOUND,	  // define boundry of a variable (upper, lower, equate )
	LALIGN,
	RALIGN,
	NOALIGN,
	UCASE,
	LCASE,
	PCASE,

	//------------- object manipulations
	JOIN,	 // attach self to object
	ATTACH,  // put hand against [object]
	DETACH,  // take two items apart...
	ENTER,
	LEAVE,

	//------------- 
	DROP,  // put hand in LOCATION	- returns what what stored
	STORE, // put hand in pocket	  - returns what was stored
	GRAB,  // get from pocket [object] - returns object
	LOOKFOR,	 // find an entity named <thing>

	//------------- view commands
	HELP,
	METHODS,
	INVENTORY,
	LOOK,
	MAP,

	//-------------- object file operations
	SAVE,
	//LOAD,

	//-------------- macro commands
	CMD_MACRO,		// begin recording a macro [macroname]
	CMD_ENDMACRO,	// stop recording macro [macroname]
	CMD_LIST,		 // list the commands in a macro - NOT recorded.
	CMD_SAVE,

	//-------------- DIAGNOSTIC COMMANDS
	DUMP,
	DUMPVAR,
	MEMORY,
	MEMDUMP,
	PRIORITY,
	CMD_BREAK,  // causes a DebugBreak();
	CMD_TRACE;  // starts/stops trace of macro commands (print as execute)

#define NUM_COMMANDS (sizeof(commands)/sizeof(command_entry))
 // NAME used is name of interpreted command and also name of function
 // to call taking paramters( PSENTIENT, PTEXT )
#define DEFCMD( name,desc ) { DEFTEXT(#name),DEFTEXT("Nexus Core"),0,(sizeof(#name)/sizeof(TEXTCHAR))-1,DEFTEXT(desc),name }
 // the function to call and the name interpreted do not match...
#define DEFCMDEX( name,desc,func) { DEFTEXT(#name),DEFTEXT("Nexus Core"),0,(sizeof(#name)/sizeof(TEXTCHAR))-1,DEFTEXT(desc),func }

 command_entry commands[]={ DEFCMDEX(?,"",HELP )
								  , DEFCMDEX(MACRO, "Create new macro.", CMD_MACRO)
								  , DEFCMDEX(ENDMACRO, "End Macro creation...", CMD_ENDMACRO )
								  , DEFCMDEX(LIST, "List a macro definition.", CMD_LIST )
								  , DEFCMDEX(GETWORD, "Get Next parsed element from file", CMD_GETWORD )
								  , DEFCMDEX(GETLINE, "Get Next parsed line from file", CMD_GETLINE )
								  , DEFCMDEX(GETPARTIAL, "Gets any parital expression from the parser.", CMD_GETPARTIAL )
								  , DEFCMDEX(ENDPARSE, "Stop (close) file being parsed.", CMD_ENDPARSE )
								  , DEFCMDEX(CLOSE, "Stop (close) file being parsed.", CMD_ENDPARSE )
								  , DEFCMDEX(ENDCOMMAND, "Stop (close) command processing", CMD_ENDCOMMAND )
								  , DEFCMDEX(IF, "Comparison operator <complex>", CMD_IF )
								  , DEFCMDEX(ELSE, "Begin here if previous IF result false", CMD_ELSE )
								  , DEFCMDEX(ENDIF, "End of IF expression", CMD_ENDIF )
								  , DEFCMDEX(LABEL, "Define a goto name in macro", CMD_LABEL )
								  , DEFCMDEX(GOTO, "Goto a label in a macro", CMD_GOTO )
								  , DEFCMDEX(EXECUTE, "Perform parameters as a command", CMD_PROCESS )
								  , DEFCMDEX(RUN, "Run a macro to completion", CMD_RUN )
								  , DEFCMDEX(COMPARE, "Comparison... sets fail/success", CMD_COMPARE )
								  , DEFCMDEX(RETURN, "End and return to prior macro", CMD_RETURN )
								  , DEFCMDEX(PUSH, "Add variable to end of variable", VAR_PUSH )
								  , DEFCMDEX(POP, "Take the first thing from src put in dest", VAR_POP )
								  , DEFCMDEX(HEAD, "Get Variable from beginning of variable", VAR_HEAD )
								  , DEFCMDEX(TAIL, "Get variable from end of variable", VAR_TAIL )
								  , DEFCMDEX(BREAK, "!!Generate debug breakpoint!!", CMD_BREAK )
								  , DEFCMDEX(DEBUG, "Start displaying executed commands", CMD_TRACE )
								  , DEFCMDEX(STOP, "Tell object to end current macro", CMD_STOP )
								  , DEFCMDEX(BURST, "Parse data... into variable", CMD_BURST )
								  , DEFCMDEX(INPUT, "Get Next command input line into variable", CMD_INPUT )
								  , DEFCMDEX(SHADOW, "Create a shadow of an object...", _SHADOW )
								  , DEFCMDEX(WAIT, "Wait for data to be available...", CMD_WAIT )
								  , DEFCMDEX(RESULT, "Set return result from macro", CMD_RESULT )
								  , DEFCMDEX(GETRESULT, "Get result from a macro", CMD_GETRESULT )
								  , DEFCMDEX(ON, "Define an action for an object", DefineOnBehavior )
								  , DEFCMDEX(MAKE, "Create an object from an archtype", CreateRegisteredObject )
								  , DEFCMDEX(SENDTO, "Send data directly to an object's input datapath", SendToObject )
								  , DEFCMDEX(WRITETO, "Send data directly to an object's output datapath", WriteToObject )
		 // if the same variable name is used within a foreach
		 //  foreach in item
		 //	 /%item/foreach in item
		 //
		 // /foreach on %me content
		 //	 /foreach near %me content
		 //	 /step next
		 // /step content
		 //
		 // /step is list goto
		 //	but I guess I need to remember the
		 //	stack of macrosteps that last had a loop
		 // /step next (off end of file will not goto, but will continue...
		 // /step first will reset the current position and should be reserved for exclusive
		 //	 advanced programming efforts...
		 // /step current - could also be used to go back to the beginnigng with the current element...
								  , DEFCMDEX(FOREACH, "[variable_list_name,on,in,around,near,exit(attached to room),visible] variable_name", ForEach )
								  , DEFCMDEX(STEP, "[next(default),prior,first,last] variable_name", StepEach )
								  , DEFCMDEX( EXTENSIONS, "List registered extension types", ListExtensions )
								  , DEFCMDEX(GETDELAY, "Get last delay time, or if resumed delay left", GetDelay )
								  , DEFCMDEX(OPTION, "Sets options for a datapath/filter.", OptionDevice )
								  , DEFCMDEX(CD, "Set Current Directory", CHANGEDIR )
								  , DEFCMDEX(DUPLICATE, "copy an object and all it contains", _DUPLICATE )
								  , DEFCMDEX(UNDECLARE, "Delete a variable", UNDEFINE )
								  , DEFCMD( BECOME, "Become an object." )
								  , DEFCMD( COUNT, "Get the number of objects by name." )
								  , DEFCMD(ATTACH, "Attach object in your hand to another." ) //ATTACH)
								  , DEFCMD(CHANGEDIR, "Set Current Directory" ) // CHANGEDIR )
								  , DEFCMD(CREATE, "Make something" ) //CREATE)
								  , DEFCMD(DESCRIBE, "Add a description to an object." ) //DESCRIBE)
								  , DEFCMD(DESTROY, "Destroy something" ) //DESTROY)
								  , DEFCMD(ECHO, "send output to command data channel..." )
								  , DEFCMD(PAGE, "echo a page break on the command path..." )
								  , DEFCMD(FECHO, "send output to command data channel..." ) // FECHO )
								  , DEFCMD(DETACH, "take apart an object" ) // DETACH )
								  , DEFCMD(DROP, "Drop the object in your hand" ) //DROP)
								  , DEFCMD(DUMP, "Display information about an object/macro/variable" ) // DUMP )
								  , DEFCMD(DUMPVAR, "Display information about a variable" ) // DUMPVAR )
								  , DEFCMD(ENTER, "Go into an object." ) //ENTER)
								  , DEFCMD(EXIT, "leave this mess" ) //EXIT)
								  , DEFCMD(GRAB, "Put object in your hand." ) //GRAB)
								  , DEFCMD(HELP, "this list :) " ) //HELP)
								  , DEFCMD(METHODS, "List plugin methods on this object" ) //METHODS)
								  , DEFCMD(INVENTORY,"What you are holding." ) //INVENTORY)
								  , DEFCMD(LEAVE, "Go out of an object." ) //LEAVE)
								  // , DEFCMD(LOAD, "Load objects from a file to current location." ) //LOAD)
								  , DEFCMD(LOOK, "Look at an object or the room." ) //LOOK)
								  , DEFCMD(MAP, "Show a map of all cookies." ) //MAP)
								  , DEFCMD(SAVE, "Save all objects to a file." ) //SAVE)
								  , DEFCMD(SCRIPT, "Start parsing an external file as commands." ) // SCRIPT)
								  , DEFCMD(STORE, "Pockets the object in your hand." ) //STORE)
								  , DEFCMD(PARSE, "BEGIN Parse file through first level parser." ) // PARSE )
								  , DEFCMDEX(OPEN, "Open a data channel device.", PARSE )
								  , DEFCMD(MEMORY, "Show Memory Statistics." ) // MEMORY )
								  , DEFCMD(MEMDUMP, "Dump current allocation table." ) // MEMDUMP )
								  , DEFCMD(DECLARE, "Define a variable" ) // DECLARE )
								  , DEFCMDEX(SET, "Define a variable", DECLARE )
								  , DEFCMD(COLLAPSE, "Collapse value of a variable to an atom" ) // COLLAPSE )
								  , DEFCMD(BINARY, "Define a variable no substitution" ) // BINARY )
								  , DEFCMD(ALLOCATE, "Allocate a sized binary variable" ) // ALLOCATE )
								  , DEFCMD(VARS, "List vars in object/macro" ) // VARS )
								  , DEFCMD( VVARS, "List Volatile Variables" )
								  , DEFCMD(TELL, "Tell an aware object to perform a command" ) // TELL )
								  , DEFCMD(WAKE, "Give an object a mind" ) // WAKE )
								  , DEFCMD(KILL, "Put object to sleep" ) // KILL )
								  , DEFCMD(PROMPT, "Issue current command prompt" ) // PROMPT )
								  , DEFCMD(STORY, "Show Introductory story..." ) // STORY )
								  , DEFCMD(SUSPEND, "Pause object macro processing" ) // SUSPEND )
								  , DEFCMD(RESUME, "Resume a suspended macro" ) // RESUME )
								  , DEFCMD(DELAY, "Wait for N milliseconds" ) // DELAY )
								  , DEFCMD(MONITOR, "Watch an object which is sentient" ) // MONITOR )
								  , DEFCMD(REPLY, "Reply to object last telling" ) // REPLY )
								  , DEFCMD(RENAME, "Rename an object" ) // RENAME )
								  , DEFCMD(PLUGIN, "Load plugin file named...." ) // PLUGIN )
								  , DEFCMD(UNLOAD, "Unload module which registered name" ) // UNLOAD )
								  , DEFCMD(JOIN, "Attach self to specified object" ) // JOIN )
								  , DEFCMD(COMMAND, "open a device as command channel of sentience..." ) // COMMAND )
								  , DEFCMD(INCREMENT, "Increment a variable by amount" )//, INCREMENT )
								  , DEFCMD(DECREMENT, "Decrement a variable by amount" )//, DECREMENT )
								  , DEFCMD(MULTIPLY, "Multiply a variable by amount" )//, MULTIPLY )
								  , DEFCMD(DIVIDE, "Divide a variable by amount" )//, DIVIDE )
								  , DEFCMD(LALIGN, "Change variable alignment to left..." )//, LALIGN )
								  , DEFCMD(RALIGN, "Change variable alignment to right..." )//, RALIGN )
								  , DEFCMD(NOALIGN, "Change variable alignment to none..." )//, NOALIGN )
								  , DEFCMD(UCASE, "Upper case a variable" )//, UCASE )
								  , DEFCMD(LCASE, "Lower case a variable" )//, LCASE )
								  , DEFCMD(PCASE, "Proper caes a variable (first letter upper)" )//, PCASE )
								  , DEFCMD(BOUND, "Set a variable's boundry condition (lower, uppwer)" )//, BOUND )
								  , DEFCMD(VERSION, "Get current program version into a variable(name)" )//, VERSION )
								  , DEFCMD(RELAY, "Auto relay command input to data output" )//, RELAY )
								  , DEFCMD(FILTER, "Add/modify filters on datapaths." )// , FILTER )
								  , DEFCMD(LOOKFOR, "Finds an object, sets result to entity." )// , LOOKFOR )
								  , DEFCMD(PRIORITY, "Set the priority of the dekware process." )//), PRIORITY )
};

int gbTrace = TRUE;

//--------------------------------------------------------------------------
#ifndef WIN32
int stricmp(const TEXTCHAR * dst, const TEXTCHAR * src) /*FOLD00*/

{
		  int f,l;

				do {
					 if ( ((f = (TEXTCHAR)(*(dst++))) >= 'A') && (f <= 'Z') )
						  f -= ('A' - 'a');

					 if ( ((l = (TEXTCHAR)(*(src++))) >= 'A') && (l <= 'Z') )
						  l -= ('A' - 'a');
				} while ( f && (f == l) );

		  return(f - l);
}

int strnicmp ( CTEXTSTR first, CTEXTSTR last, size_t count ) /*FOLD00*/
{
		  int f,l;

		  if ( count ) { /*FOLD01*/
				do { /*FOLD02*/
					 if ( ((f = (TEXTCHAR)(*(first++))) >= 'A') && /*FOLD03*/
							 (f <= 'Z') )
						  f -= 'A' - 'a';
					 if ( ((l = (TEXTCHAR)(*(last++))) >= 'A') && /*fold03*/
							 (l <= 'Z') )
						  l -= 'A' - 'a';

				} while ( --count && f && (f == l) ); /*FOLD02*/
				return( f - l );
		  } /*FOLD01*/

		  return( 0 );
} /*FOLD00*/
#endif

int FindDiff(const TEXTCHAR * dst, const TEXTCHAR * src)

{
		  int f,l;
		  // first charcter different to start.
		  int n = 1;
				do {
					 if ( ((f = (TEXTCHAR)(*(dst++))) >= 'A') && (f <= 'Z') )
						  f -= ('A' - 'a');
					 if( !f )
						 return n;
					 if ( ((l = (TEXTCHAR)(*(src++))) >= 'A') && (l <= 'Z') )
						 l -= ('A' - 'a');
					 if( !l )
						 return n;
					 if( f == l )
						 n++;
					 else
						 break;
				} while ( 1 );
		  return n;
}

//--------------------------------------------------------------------------

CORE_PROC( void, WriteCommand )( PLINKQUEUE *Output
										 , PTEXT name
										  , int significant
										 , PTEXT description
										 )
{
	PVARTEXT vt = VarTextCreate();
	vtprintf( vt,"[%.*s]%-*s - %s"
			  , significant
			  , GetText(name)
			  , 10 - significant
			  , GetText( name ) + significant
			  , GetText( description ) );
	EnqueLink( Output, VarTextGet( vt ) );
	VarTextDestroy( &vt );
}



CORE_PROC( void, WriteCommandList2 )( PLINKQUEUE *Output, CTEXTSTR root
								 , PTEXT pMatch )
{
	PVARTEXT vt = VarTextCreate();
	//INDEX count;
	PCLASSROOT current = NULL;
	PCLASSROOT _current;
	CTEXTSTR name;
	CTEXTSTR prior_name = NULL;
	CTEXTSTR _prior_name = NULL;
	PCLASSROOT troot = GetClassRoot( root );
	for( name = GetFirstRegisteredName( (CTEXTSTR)troot, &current );
		 name;
		  name = GetNextRegisteredName( &current ) )
	{
		size_t diff;
		//if( commands[count].maxlen < 0 )  // skip disabled commands
		//	continue;
		if( pMatch )
			if( TextLike( pMatch, name ) )
				continue;
		if( prior_name )
		{
			diff = FindDiff( prior_name, name );
			if( _prior_name )
			{
				int diff2 = FindDiff( _prior_name, prior_name );
				if( diff2 > diff )
					diff = diff2;
			}

			{
				size_t len_name = StrLen( prior_name );
				if( diff > len_name )
					diff = len_name;
			}

			{						
				CTEXTSTR desc;
				desc = GetRegisteredValue( (CTEXTSTR)_current, "Description" );
				vtprintf( vt,"[%.*s]%-*s - %s"
						  , diff
						  , prior_name
						  , 10-diff
						  , prior_name+diff
						  , desc?desc:"" );
				EnqueLink( Output, VarTextGet( vt ) );
			}
		}
		_current = GetCurrentRegisteredTree( &current );
		_prior_name = prior_name;
		prior_name = name;
	}
	if( _prior_name && prior_name )
	{
		int diff = FindDiff( _prior_name, prior_name );

		PCLASSROOT tmp;
		CTEXTSTR desc;
		tmp = GetClassRootEx( (PCLASSROOT)root, prior_name );
		desc = GetRegisteredValue( (CTEXTSTR)tmp, "Description" );
		vtprintf( vt,"[%.*s]%-*s - %s"
				  , diff
				  , prior_name
				  , 10-diff
				  , prior_name+diff
				  , desc?desc:"" );
		EnqueLink( Output, VarTextGet( vt ) );
	}
	VarTextDestroy( &vt );
}

CORE_PROC( void, WriteCommandList )( PLINKQUEUE *Output, command_entry *commands /*FOLD00*/
								 , INDEX nCommands
								 , PTEXT pMatch )
{


	INDEX count;
	for (count=0;count<nCommands;count++)
	{
		if( commands[count].maxlen < 0 )  // skip disabled commands
			continue; 
		if( pMatch )
			if( LikeText( (PTEXT)&commands[count].name, pMatch ) )
				continue;
		if( commands[count].description.data.size )
		{
			WriteCommand( Output, (PTEXT)&commands[count].name
							, commands[count].significant
							, (PTEXT)&commands[count].description );
		}
	}
}

//--------------------------------------------------------------------------

CORE_PROC( void, WriteOptionList )( PLINKQUEUE *Output, option_entry *commands
								 , INDEX nCommands
								 , PTEXT pMatch )
{
	INDEX count;
	for (count=0;count<nCommands;count++)
	{
		if( commands[count].maxlen < 0 )  // skip disabled commands
			continue; 
		if( pMatch )
			if( LikeText( (PTEXT)&commands[count].name, pMatch ) )
				continue;
		if( commands[count].description.data.size )
		{
			WriteCommand( Output, (PTEXT)&commands[count].name
							, commands[count].significant
							, (PTEXT)&commands[count].description );
		}
	}
}

//--------------------------------------------------------------------------

void RegisterCommands(CTEXTSTR device, command_entry *cmds, INDEX nCommands)
{
	INDEX i;
	//int32_t l;
	static int lock;
	//command_entry temp;
	while( lock )
		Relinquish();
	lock++;
	if( !cmds )
	{
		cmds = commands;
		nCommands = NUM_COMMANDS;
	}


	// bubble sort the command list...
	for( i = 0; i < nCommands-1; i++ )
	{
		TEXTCHAR tmp[256];
		TEXTCHAR tmp2[256];
		CTEXTSTR name;
		snprintf( tmp, sizeof( tmp ), "Dekware/commands%s%s/%s"
				  , device?"/":""
				  , device?device:""
				  , name = GetText( (PTEXT)&cmds[i].name ) );
		snprintf( tmp2, sizeof( tmp2 ), "Dekware/commands%s%s"
				  , device?"/":""
				  , device?device:"" );
		if( CheckClassRoot( tmp ) )
		{
			lprintf( "%s already registered", tmp );
			continue;
		}
		//lprintf( "regsiter %s", tmp );
		SimpleRegisterMethod( tmp2, cmds[i].function
								  , "int", name, "(PSENTIENT,PTEXT)" );
		RegisterValue( tmp, "Description", GetText( (PTEXT)&cmds[i].description ) );
		RegisterValue( tmp, "Command Class", GetText( (PTEXT)&cmds[i].classname ) );
	}
	lock--;
}

void RegisterOptions(CTEXTSTR device, option_entry *cmds, INDEX nCommands)
{
	INDEX i;
	static int lock;
	while( lock )
		Relinquish();
	lock++;
	if( !cmds )
	{
		return;
	}


	for( i = 0; i < nCommands-1; i++ )
	{
		TEXTCHAR tmp[256];
		TEXTCHAR tmp2[256];
		CTEXTSTR name;
		snprintf( tmp, sizeof( tmp ), "Dekware/devices%s%s/options/%s"
				  , device?"/":""
				  , device?device:""
				  , name = GetText( (PTEXT)&cmds[i].name ) );
		snprintf( tmp2, sizeof( tmp2 ), "Dekware/devices%s%s/options"
				  , device?"/":""
				  , device?device:"" );
		if( CheckClassRoot( tmp ) )
		{
			lprintf( "%s already registered", tmp );
			continue;
		}
		//lprintf( "regsiter %s", tmp );
		SimpleRegisterMethod( tmp2, cmds[i].function
								  , "int", name, "(PDATAPATH,PSENTIENT,PTEXT)" );
		RegisterValue( tmp, "Description", GetText( (PTEXT)&cmds[i].description ) );
		//RegisterIntValue( tmp, "option_entry", (uintptr_t)(cmds+i) );
		//RegisterIntValue( tmp, "significant", strlen( name ) );
		//RegisterIntValue( tmp, "max_length", strlen( name ) );
	}
	lock--;
}

//--------------------------------------------------------------------------

//-----------------------------------------------------------
// Error Handling routines for command processing
//-----------------------------------------------------------
//--------------------------------------------------------------------------

void LackingParam(PLINKQUEUE *Output, INDEX command) // and command table... /*FOLD00*/
{
	PTEXT pOut;
	DECLTEXT( msg, "was expecting more parameters." );
	pOut = SegAppend( SegCreateIndirect( (PTEXT)&commands[command].name )
						 , SegCreateIndirect( (PTEXT)&msg ) );
	EnqueLink( Output, pOut );
}

//--------------------------------------------------------------------------

PTEXT Help( PSENTIENT ps, PTEXT pCommand ) /*FOLD00*/
{
	PLINKQUEUE *Output = &ps->Command->Output;
	PENTITY pEnt = ps->Current;
	uint16_t count;
	PMACRO pm;
	{
		DECLTEXT( leader, " --- Builtin Commands ---" );
		EnqueLink( Output, &leader );
	// this command may pause waiting for input...
	// we have a limited number of threads for sentients...
	// if 16 people are in help... well we need to have
	// nActiveSentients... then if it is greater than
	// nAvailSentients... echo that help is temporarily
	// unavailable...
		WriteCommandList2( Output, "dekware/commands", pCommand );
	}
	// list macros of current object
	if( !pCommand )
	{
		PVARTEXT vt = VarTextCreate();
		S_MSG( ps, " --- Macros ---" );
		LIST_FORALL( pEnt->pMacros, count, PMACRO, pm )
		{
			vtprintf( vt, "[%s] - %s"
						, GetText( GetName( pm ) )
						, GetText( GetDescription( pm ) ) );
			EnqueLink( Output, VarTextGet( vt ) );
		}
		VarTextDestroy( &vt );
	}
//	if( !pCommand )
	{
		//S_MSG( ps, " --- Plugins ---" );
		//PrintRegisteredRoutines( Output, ps, pCommand );
	}
	if( !pCommand )
	{
		S_MSG( ps, " --- Devices ---" );
		PrintRegisteredDevices( Output );
	}
	if( !pCommand )
	{
		DECLTEXT( PluginSep, " - parameter to help will show only matching commands" );
		EnqueLink( Output, &PluginSep );
	}
	return NULL;
}

//--------------------------------------------------------------------------

PTEXT Methods( PSENTIENT ps ) /*FOLD00*/
{
	PLINKQUEUE *Output = &ps->Command->Output;
	//PENTITY pEnt = ps->Current;
	uint16_t count;
	int bMethod = FALSE;
	{
		PVARTEXT vt = VarTextCreate();
		PENTITY pe = ps->Current;
		DECLTEXT( msg, " --- Object Methods ---" );
		PCLASSROOT pme;
		//command_entry *pme;

		LIST_FORALL( pe->pMethods, count, PCLASSROOT, pme )
		{
			CTEXTSTR name;
			CTEXTSTR desc;
			PCLASSROOT tmp;
			PCLASSROOT root = GetClassRootEx( pme, "methods" );
			for( name = GetFirstRegisteredName( (CTEXTSTR)root, &tmp );
				 name;
				  name = GetNextRegisteredName( &tmp ) )
			{
				if( !bMethod )
				{
					EnqueLink( Output, &msg );
					bMethod = TRUE;
				}
				desc = GetRegisteredValue( (CTEXTSTR)(GetCurrentRegisteredTree(&tmp)), "Description" );
				vtprintf( vt, "[%s] - %s"
						  , name
						  , desc
						  );
				EnqueLink( Output, VarTextGet( vt ) );
			}
		}
		VarTextDestroy( &vt );
	}
	return NULL;
}

//--------------------------------------------------------------------------

TEXTCHAR Months[13][10] = { ""
						, "January"
						, "February"
						, "March"
						, "April"
						, "May"
						, "June"
						, "July"
						, "August"
						, "September"
						, "October"
						, "November"
						, "December" };
TEXTCHAR Days[7][10] = {"Sunday", "Monday", "Tuesday", "Wednesday"
					, "Thursday", "Friday", "Saturday" };

DECLTEXT( timenow, "00/00/0000 00:00:00						" );

PTEXT GetTime( void ) /*FOLD00*/
{
//	PTEXT pTime;
#ifdef WIN32
	SYSTEMTIME st;
//	pTime = SegCreate( 38 );
	GetLocalTime( &st );
	/*
	n = sprintf( pTime->data.data, "%s, %s %d, %d, %02d:%02d:%02d",
							Days[st.wDayOfWeek], Months[st.wMonth],
							st.wDay, st.wYear
							, st.wHour, st.wMinute, st.wSecond );
	*/
#if defined( _MSC_VER ) && defined( __cplusplus_cli )
#define snprintf _snprintf
#endif
	timenow.data.size = snprintf( timenow.data.data, sizeof( timenow.data.data ), "%02d/%02d/%d %02d:%02d:%02d",
							st.wMonth, st.wDay, st.wYear
							, st.wHour, st.wMinute, st.wSecond );
	return (PTEXT)&timenow;
#else
	//struct timeval tv;
	struct tm *timething;
	time_t timevalnow;
	time(&timevalnow);
	timething = localtime( &timevalnow );
	strftime( timenow.data.data
				, sizeof( timenow.data.data )
				, "%m/%d/%Y %H:%M:%S"
				, timething );
	return (PTEXT)&timenow;
#endif
}

//------------------------------------------------------------------------

PTEXT GetShortTime( void ) /*FOLD00*/
{
//	PTEXT pTime;
#ifdef WIN32
	SYSTEMTIME st;
	int n;
//	pTime = SegCreate( 38 );
	GetLocalTime( &st );
	/*
	n = sprintf( pTime->data.data, "%s, %s %d, %d, %02d:%02d:%02d",
							Days[st.wDayOfWeek], Months[st.wMonth],
							st.wDay, st.wYear
							, st.wHour, st.wMinute, st.wSecond );
	*/

	n = snprintf( timenow.data.data, sizeof( timenow.data.data ), "%02d:%02d:%02d",
							st.wHour, st.wMinute, st.wSecond );
	if( n > (signed)timenow.data.size )
		DebugBreak();
	timenow.data.size = n;
	return (PTEXT)&timenow;
#else
	//struct timeval tv;
	struct tm *timething;
	time_t timevalnow;
	time(&timevalnow);
	timething = localtime( &timevalnow );
	strftime( timenow.data.data
				, sizeof( timenow.data.data )
				, "%H:%M:%S"
				, timething );
	return (PTEXT)&timenow;
#endif
}

//------------------------------------------------------------------------
/*

Order of operation for substitution...
first check the symbol for TF_PAREN
	if success - Macro duplicate subst indirect...

*/

//------------------------------------------------------------------------

CORE_PROC( PTEXT, MakeNumberText )( size_t val ) /*FOLD00*/
{
	 PVARTEXT vt = VarTextCreate();
	 PTEXT pVal;
	 vtprintf( vt, "%d", val );
	 pVal = VarTextGet( vt );
	 VarTextDestroy( &vt );
	 return pVal;
}

//----------------------------------------------------------------------

PTEXT MakeTempNumber( size_t val ) /*FOLD00*/
{
	 PTEXT var = MakeNumberText(val);
	 var->flags |= TF_TEMP;
	 return var;
}

//----------------------------------------------------------------------

CORE_PROC( PTEXT, GetListVariable )( PLIST pList, CTEXTSTR text ) /*FOLD00*/
{
	INDEX idx;
	PTEXT pVar;
	LIST_FORALL( pList, idx, PTEXT, pVar )
	{
		if( TextIs( pVar, text ) )
			return NEXTLINE( pVar );
	}
	return NULL;
}

//------------------------------------------------------------------------

CORE_PROC( PTEXT, GetVolatileVariable )( PENTITY pEnt, CTEXTSTR pNamed )
{
	INDEX idx;
	volatile_variable_entry *pvve = NULL;
	// look for it in registered variable extensions (methods, but as vars)
	LIST_FORALL( pEnt->pVariables, idx, volatile_variable_entry*, pvve )
	{
		if( StrCmp( pvve->pName, pNamed ) == 0 )
		{
			if( pvve->get )
				return pvve->get( pEnt, &pvve->pLastValue );
			return NULL;
		}
	}
	return NULL;
}
//------------------------------------------------------------------------

CORE_PROC( PTEXT, SetVolatileVariable )( PENTITY pEnt, CTEXTSTR pNamed, PTEXT newval )
{
	INDEX idx;
	volatile_variable_entry *pvve = NULL;
	// look for it in registered variable extensions (methods, but as vars)
	LIST_FORALL( pEnt->pVariables, idx, volatile_variable_entry*, pvve )
	{  
		if( StrCmp( pvve->pName, pNamed ) == 0 )
		{
			if( pvve->set )
				return pvve->set( pEnt, newval );
			return NULL;
		}
	}
	return NULL;
}
//------------------------------------------------------------------------

static PTEXT ObjectVolatileVariableGet( "core object", "me", "my name" )( PENTITY pe, PTEXT *lastvalue )
{
	return GetName( pe );
}

static PTEXT ObjectVolatileVariableGet( "core object", "room", "my room's name" )( PENTITY pe, PTEXT *lastvalue )
{
	*lastvalue = TextDuplicate( GetName( FindContainer( pe ) ), FALSE );
	return *lastvalue;
}

static PTEXT ObjectVolatileVariableGet( "core object", "now", "the current time" )( PENTITY pe, PTEXT *lastvalue )
{
	*lastvalue = GetShortTime();
	return *lastvalue;
}

static PTEXT ObjectVolatileVariableGet( "core object", "time", "the current time" )( PENTITY pe, PTEXT *lastvalue )
{
	*lastvalue = GetTime();
	return *lastvalue;
}

static PTEXT ObjectVolatileVariableGet( "core object", "blank", "a space character(segment)" )( PENTITY pe, PTEXT *lastvalue )
{
	DECLTEXT( blank, " ");
	if( !*lastvalue )
		*lastvalue = (PTEXT)&blank;
	return *lastvalue;
}

static PTEXT ObjectVolatileVariableGet( "core object", "EOL", "end of line character(segment)" )( PENTITY pe, PTEXT *lastvalue )
{
	DECLTEXT( eol, "");
	if( !*lastvalue )
		*lastvalue = (PTEXT)&eol;
	return *lastvalue;
}

static PTEXT ObjectVolatileVariableGet( "core object", "cmdline", "end of line character(segment)" )( PENTITY pe, PTEXT *lastvalue )
{
	extern PTEXT global_command_line;
	if( !*lastvalue )
	{
		*lastvalue = SegCreateIndirect( global_command_line );
	}
	return *lastvalue;
}

static PTEXT ObjectVolatileVariableGet( "core object", "program_path", "the path to the .exe that ran this" )( PENTITY pe, PTEXT *lastvalue )
{
	extern CTEXTSTR load_path;
	if( !*lastvalue )
		*lastvalue = SegCreateFromText( load_path );
	return *lastvalue;
}

static PTEXT ObjectVolatileVariableGet( "core object", "core_path", "the path to dekware.core" )( PENTITY pe, PTEXT *lastvalue )
{
	extern CTEXTSTR core_load_path;
	if( !*lastvalue )
		*lastvalue = SegCreateFromText( core_load_path );
	return *lastvalue;
}

static PTEXT ObjectVolatileVariableGet( "core object", "caller", "end of line character(segment)" )( PENTITY pe, PTEXT *lastvalue )
{
	PSENTIENT ps = pe->pControlledBy;
	if( *lastvalue )
		LineRelease( *lastvalue );
	if( ps && ps->CurrentMacro )
	{
		*lastvalue = SegCreateIndirect( ps->CurrentMacro->pInvokedBy?GetName ( ps->CurrentMacro->pInvokedBy->Current ):NULL );
	}
	else
		*lastvalue = NULL;
	return *lastvalue;
}

static PTEXT ObjectVolatileVariableGet( "core object", "actor", "end of line character(segment)" )( PENTITY pe, PTEXT *lastvalue )
{
	PSENTIENT ps = pe->pControlledBy;
	if( *lastvalue )
		LineRelease( *lastvalue );
	if( ps )
	{
		(*lastvalue) = SegCreateIndirect( (PTEXT)ps->pInactedBy );
		(*lastvalue)->flags |= TF_ENTITY;
	}
	else
		(*lastvalue) = NULL;
	return *lastvalue;
}


PTEXT GetVariable( PSENTIENT ps, CTEXTSTR varname )
{
	PENTITY pEnt = ps->Current;
	{
		PTEXT pText = GetVolatileVariable( pEnt, varname );
		if( pText )
			return pText;
	}
	{
		PTEXT c;
		if( ( c = GetListVariable( pEnt->pVars, varname ) ) )
		{
			return c;
		}
	}
	return NULL;
}

static PTEXT LookupMacroVariable( CTEXTSTR ptext, PMACROSTATE pms )
{
	PTEXT c;
	PTEXT pReturn;
	// if skipped a % retain all further...
	// also cannot perform substition....
	if( !strcmp( ptext, "..." ) ) // may test for 'elispes'?
	{
		int32_t i;
		//lprintf( "Gathering trailing macro args into one line..."  );
		if( pms->pMacro->nArgs < 0 )
		{
			//lprintf( "Skipping some arguments to get to ... " );
			i = -pms->pMacro->nArgs;
			pReturn = pms->pArgs;
			while( i && --i && pReturn )
			{
				//lprintf( "Skipped %s", GetText( pReturn ) );
				pReturn = NEXTLINE( pReturn );
			}
		}
		else
			lprintf( "Macro has positive argument count - there is no ... param" );
		//lprintf( "Okay at this point pReturn should be start of macro extra parms" );
		if( !(pReturn->flags & TF_INDIRECT) )
		{
			PTEXT pWrapper = SegCreateIndirect( pReturn );
			lprintf( "Wrapping pReturn in a single seg wrapper" );
			SegAppend( SegBreak( pReturn ), pWrapper );
			pWrapper->flags |= TF_DEEP;
			pReturn = pWrapper;
		}
		//else // normal condition.
		//	lprintf( "Result is already in a DEEP indirect? " );
		return pReturn;
	}

	{
		// parameter substition for macros...
		// wow even supports ancient syntax of %1 %2 %3 ....
		int n = atoi( ptext );
#ifdef DEBUG_TOKEN_SUBST
		lprintf( "int of %s is %d", ptext, n );
#endif
		if( !n )
		{
			LIST_FORALL( pms->pVars, n, PTEXT, c )
			{
#ifdef DEBUG_TOKEN_SUBST
				lprintf( "is var %s == %s",GetText( c ), ptext );
#endif
				if( TextIs( c, ptext ) )
				{
					return NEXTLINE(c);
				}
			}
			// if variable was found - it is returned...

			// test versus prior macro names....
			// count segments which are names...
			// segments are parallel aligned with macros from command...

			// n == 0 at this point cause this param was not numeric...
			n = 0;
			c = pms->pMacro->pArgs;
			while( c )
			{
#ifdef DEBUG_TOKEN_SUBST
				lprintf( "is arg %s == %s",GetText( c ), ptext );
#endif
				if( TextIs( c, ptext ) )
					break;
				n++;
				c = NEXTLINE(c);
			}
			if( !c )
				return NULL;
		}
		else
			n--;

		c = pms->pArgs; // current args to this macro.
#ifdef DEBUG_TOKEN_SUBST
		lprintf( "so... %d %p", n, c );
#endif
		while( n && c )
		{
			c = NEXTLINE(c);
			n--;
		}
		pReturn = c;
#ifdef DEBUG_TOKEN_SUBST
		lprintf( "result param is %s", GetText( pReturn ) );
#endif
	}
	return pReturn;
}

//------------------------------------------------------------------------

LOGICAL IsVariableBreak( PTEXT token )
{
	if( !token || HAS_WHITESPACE( token ) )
		return TRUE;
	if( GetTextSize( token ) == 1 )
	{
		TEXTCHAR *text = GetText( token );
		if( text[0] == '[' || text[0] == ']' || text[0] == '(' || text[0] == ')' )
			return FALSE;
		if( StrChr( "!@#$%^&*,.<>/?\\|{}-=_+~`", text[0] ) )
			return TRUE;
	}
	return FALSE;
}

//------------------------------------------------------------------------
// return FALSE if it's an object result
// in some cases, the result is an entity WITH a varname (varname != NULL )
// sometimes it's just to find the entity, and (varname is NULL)
// tokens has to be updated if more than varname is used
PENTITY ResolveEntity( PSENTIENT ps_out, PENTITY focus, enum FindWhere type, PTEXT *tokens, LOGICAL bKeepVarName )
{
	PTEXT septoken;
	PTEXT original_token;
	PTEXT next_token;
	PTEXT name_token = NULL;
	PENTITY result = NULL;
	int64_t long_count = 1;
	while( 1 )
	{
		original_token = (*tokens );
		septoken = GetParam( ps_out, tokens );//*varname;
		if( !septoken )
			return focus;
		//ExtraParse( &septoken );

		if( GetTextSize( septoken ) == 1 && GetText( septoken )[0] == '(' )
		{
			PENTITY tmp;
			//PTEXT real_token = SubstTokenEx( ps_out, &tmp_token, FALSE, FALSE, focus );
			tmp = ResolveEntity( ps_out, focus, type, tokens, FALSE );

			if( GetTextSize( *tokens ) == 1 && GetText( *tokens )[0] == ')' )
			{
				// success
				focus = tmp;
				GetParam( ps_out, tokens ); // already know the content, just step token
				if( IsVariableBreak( *tokens ) )
					break;
				else
					continue;
			}
			else
			{
				S_MSG( ps_out, "Failed to find close paran phrase" );
				return NULL;
			}
		}
		else
		{
			PTEXT next_original_token = (*tokens);
			next_token = GetParam( ps_out, tokens );//NEXTLINE( septoken );
			if( next_token && GetTextSize( next_token ) == 1 && GetText( next_token )[0] == '[' )
			{
				PTEXT indexer;
				indexer = GetParam( ps_out, tokens );
				//PTEXT indexer = SubstTokenEx( ps_out, &phrase, FALSE, FALSE, focus );
				if( GetTextSize( *tokens ) == 1 && GetText( *tokens )[0] == ']' )
				{
					if( IsIntNumber( indexer, &long_count ) )
					{
						name_token = septoken;
						//(*varname) = NEXTLINE( phrase );
						GetParam( ps_out, tokens );
					}
					else
					{
						S_MSG( ps_out, "Indexer phrase did not result in an integer" );
						lprintf( "Indexer phrase did not result in an integer" );
					}
				}
			}
			else
			{
				// no extra syntax, word is taken as name; restore tokens
				if( !bKeepVarName )
				{
					name_token = septoken;
					(*tokens) = next_original_token;
				}
				else
				{
					if( IsVariableBreak( next_token ) )
					{
						(*tokens) = original_token;
						return focus;
					}
				}
			}
		}
		if( name_token )
		{
			PENTITY discovered;
			enum FindWhere findtype;
			size_t count = long_count;

			if( bKeepVarName )
			{
			}

			{
				discovered = (PENTITY)DoFindThing( focus, type, &findtype, &count, GetText( name_token ) );
				if( discovered )
				{
					focus = discovered;
					name_token = NULL;
					if( !bKeepVarName )
					{
						break;
					}
					continue;
				}
				else
					break;
			}
		}
		else
		{
			(*tokens) = original_token;
			break;
		}
	}
	return focus;
}


//------------------------------------------------------------------------

#define STEP_TOKEN() do { pPrior = pReturn; \
	pReturn = *token;  \
	ptext = GetText( pReturn ); textlen = GetTextSize( pReturn );  \
	*token = NEXTLINE( *token ); } while(0)

#define RESET_TOKEN() do { pPrior = pReturn; \
	pReturn = *token;	 \
	ptext = GetText( pReturn ); textlen = GetTextSize( pReturn );  \
	} while(0)

#define BEGIN_STEP_TOKEN() {  pReturn = *token;  \
	pPrior = NULL;			  \
	ptext = GetText( pReturn ); textlen = GetTextSize( pReturn );	  \
	*token = NEXTLINE( *token ); }

CORE_PROC( PTEXT, SubstTokenEx )( PSENTIENT ps, PTEXT *token, int IsVar, int IsLen, PENTITY pe ) /*FOLD00*/
{
	PTEXT c;
	PMACROSTATE pms;
	PENTITY pEnt;
	TEXTCHAR *ptext;
	size_t textlen;
	PTEXT pReturn;
	PTEXT original_token;
	PTEXT pPrior;

	//int IsVariable = FALSE;
	int IsVarLen  = FALSE;
	TEXTCHAR const_var;

	if( !token || !*token )
	{
		 return NULL;
	}

	if( ps )
	{
		pEnt = ps->Current; // default entity to check...
		pms = ps->CurrentMacro;
		//lprintf( "pms is now something %p %p", pms, pms?pms->pVars:0 );
	}
	else
	{
		pEnt = pe;
		//lprintf( "PMS is NULL" );
		pms = NULL; // no known macro state?
	}
	original_token = (*token);
	BEGIN_STEP_TOKEN();
	//lprintf( "Begin Step Token: %s (%s)", ptext, GetText( *token ) );
	if( ( !pEnt && !pms )
		|| ( pReturn->flags & ( TF_BINARY | TF_SENTIENT | TF_ENTITY ) ) )
		return pReturn; // return this token... 

	const_var = 0;
	if( !IsVar && !IsLen )
	{
		if( ( IsVar || ptext[0] == '%' ) && ( textlen == 1 ) )
		{
			const_var = '%';
			IsVarLen = FALSE;
			if( *token )
			{
				original_token = (*token);
				STEP_TOKEN();
			}
			else
				return pReturn; // last thing on the line was this '%'
		}
		else if( ( ptext[0] == '#' ) && ( textlen == 1 ) )
		{
			const_var = '#';
			IsVarLen = TRUE;
			if( *token )
			{
				original_token = (*token);
				STEP_TOKEN();
			}
			else
				return pReturn; // last thing on the line was this '#'
		}
		else
		{
			return pReturn; // isn't a variable at all....
		}

		// check for double var escapes '%%' and '##'
		// if the token after a '%' is the same,
		// align the return to the same left as the prior,
		// and return the symbol.
		if( ptext[0] == const_var ) // double % strip one off return direct.
		{
			pReturn->format.position.offset.spaces = pPrior->format.position.offset.spaces;
			return pReturn; // return now....
		}
	}
	else
	{
		if( IsLen )
			IsVarLen = TRUE;
		else
			return pReturn;
	}
#ifdef DEBUG_TOKEN_SUBST
	lprintf( "Token resolved as a variable... further processing." );
#endif
	// at this point - we have established that the next thing
	// may be a variable name to locate
	//	variable in local space, macro param space, and object var space
	// may also be a combination obj.var or (obj)var

	// parenthesis has the advantage of being able to subsittute
	// chained commands (apple)(core)(seed)blah
	//ExtraParse( &pReturn, token );

	{
		(*token) = original_token;
		pEnt = ResolveEntity( ps, pEnt, FIND_VISIBLE, token, TRUE );
		BEGIN_STEP_TOKEN();
		
		if( pEnt == ps->Current )
		{
			// macro state only applies for the object of the sentience.
			// IF next token is not a '.' and this token != '(' and next token != '['
			if( pms && ptext )
			{
				PTEXT pMacroReturn = LookupMacroVariable( ptext, pms );
				if( pMacroReturn )
				{
					if( IsVarLen )
						MakeTempNumber( LineLength( pMacroReturn ) );
#ifdef DEBUG_TOKEN_SUBST
					lprintf( "Returning %s", GetText( pMacroReturn ) );
#endif
					return pMacroReturn; // this variable comes from this variable
				}
			}
		}
	}

	{ /*FOLD00*/
		if( pEnt )
		{
			PTEXT pText = GetVolatileVariable( pEnt, ptext );
			if( pText )
			{
				if( IsVarLen ) 
					return MakeTempNumber( LineLength( pText ) );
				return pText;
			}
		}

		{
			if( ( c = GetListVariable( pEnt->pVars, ptext ) ) )
			{
				//lprintf( "Found global var %s", GetText( c ) );
				if( IsVarLen )
					return MakeTempNumber( LineLength( c ) );
				else
					return c;
			}
			// otherwise - unknown location - or unkonw variable..
			if( !ps->CurrentMacro )
			{
				//S_MSG( ps, "Parameter named %s was not found."
				//	  , GetText(pReturn) );
			}
			if( IsVarLen )
				if( pReturn->flags & TF_INDIRECT )
					return MakeTempNumber( LineLength( GetIndirect( pReturn ) ) );
				else
					return MakeTempNumber( GetTextSize( pReturn ) );
		}
	}
	return pReturn;
}



//--------------------------------------------------------------------------
CORE_PROC( PTEXT, GetParam )( PSENTIENT ps, PTEXT *from )
{
	if( !from || !*from) return NULL;

	// skip end of lines to getparam....
	while( from &&
			 *from &&
			 !( (*(int*)&(*from)->flags ) & IS_DATA_FLAGS ) &&
			 !(*from)->data.size ) // blah...
		*from = NEXTLINE( *from );
	return SubstToken( ps, from, FALSE, FALSE );
}

//--------------------------------------------------------------------------

int32_t CountArguments( PSENTIENT ps, PTEXT args ) /*FOLD00*/
{
	int32_t idx;
	PTEXT pSubst;
	idx = 0;
	while( ( pSubst = SubstToken( ps, &args, FALSE, FALSE ) ) )
	{
		if( pSubst->flags & TF_INDIRECT )
		{
			//xlprintf(LOG_NOISE+1)( "Found indirect arguemtn count it from %d", idx );
			idx += CountArguments( ps, GetIndirect( pSubst ) );
			//xlprintf(LOG_NOISE+1)( "Found indirect arguemtn count it to %d", idx );
		}
		else
		{
			idx++;
			if( TextIs( pSubst, "..." ) )
				return -idx;
		}
		if( pSubst->flags & TF_TEMP )
			LineRelease( pSubst );
	}
	//xlprintf(LOG_NOISE+1)( "total args is %d", idx );
	return (int32_t)idx;
}

//--------------------------------------------------------------------------

void DestroyMacro( PENTITY pe, PMACRO pm ) /*FOLD00*/
{
	PTEXT temp;
	uintptr_t idx;
	if( !pm )
		return;
	// if the macro is in use (running) just mark that we wish to delete.
	if( pm->flags.un.macro.bUsed )
	{
		xlprintf(LOG_NOISE)( "Macro is in use - mark delete, don't do it now." );
		pm->flags.un.macro.bDelete = TRUE;
		return;
	}
	xlprintf(LOG_NOISE+1)( "Destroying macro %s", GetText( pm->pName ) );
	if( --pm->Used )
	{
		xlprintf(LOG_NOISE+1)(" Macro usage count is now %d", pm->Used );
		return;
	}
	xlprintf(LOG_NOISE+1)(" Macro usage count is now %d", pm->Used );

	// find the macro within this entity
	if( pe )
	{
		idx = (uintptr_t)DoFindThing( pe, FIND_MACRO_INDEX, NULL, NULL, GetText( pm->pName ) );
		if( idx != INVALID_INDEX )
				SetLink( &pe->pMacros, idx, NULL );
		// it might be a behavior macro...
	}
	else
	{
		// can't use pe if it's not set...
		//DECLTEXT( msg, "Could not locate the macro specified" );
		//EnqueLink( &pe->pControlledBy->Command->Output, &msg );
	}
	if( pe )
	{
		PCLASSROOT current = NULL;
		CTEXTSTR name2;
		CTEXTSTR root;
		INDEX idx;

		CTEXTSTR extension_name;
		LIST_FORALL( global.ExtensionNames, idx, CTEXTSTR, extension_name )
		{
			if( GetLink( &pe->pPlugin, idx ) )
			{
				PVARTEXT pvt = VarTextCreate();
				vtprintf( pvt, "dekware/objects/%s/macro/destroy", extension_name );
				for( name2 = GetFirstRegisteredName( root = GetText( VarTextPeek( pvt ) ), &current );
				 	name2;
					name2 = GetNextRegisteredName( &current ) )
				{
					MacroCreateFunction f = GetRegisteredProcedure2( root, void, name2, (PENTITY,PMACRO) );
					if( f )
					{
						f( pe, pm );
					}
				}
				VarTextDestroy( &pvt );
			}
		}
	}
	LineRelease( pm->pArgs );
	LineRelease( pm->pName );
	LineRelease( pm->pDescription );
	LIST_FORALL( pm->pCommands, idx, PTEXT, temp )
	{
		//{
		//	PTEXT x = BuildLine( temp );
		//	lprintf( "DestroyMacro: %s", GetText( x ) );
		//	LineRelease( x );
		//}
		LineRelease( temp );
	}
	DeleteList( &pm->pCommands );
	Release( pm );
}
 /*FOLD00*/
//--------------------------------------------------------------------------
CORE_PROC( PTEXT, MacroDuplicateExx )( PSENTIENT ps /*FOLD00*/
												 , PTEXT pText
												 , int bKeepEOL
												 , int bSubst
												 , PTEXT pArgs DBG_PASS )
#define DBG_LOCAL DBG_SRC
{
	PTEXT pNew, pSubst;
	INDEX len;
	int first = TRUE;
	int hadargs = (pArgs!=NULL);
	pNew = NULL;
	while( pText && ( ( len = GetTextSize( pText ) ) || bKeepEOL ) )
	{
		int spaces;
		pSubst = NULL;
		spaces = pText->format.position.offset.spaces;
		if( hadargs && !pArgs )
		{
			lprintf( "Ran out of arguments to match, still have text:%s", GetText( pText ) );
		}
		if( pText->flags & TF_BINARY )
		{
			pText = NEXTLINE( pText );
			continue;
		}
		if( pArgs && strcmp( GetText( pArgs ), "..." ) == 0 )
		{
			//lprintf( "Argname is ... therefore grab rest of line and be done." );
			pSubst = SegCreateIndirect( MacroDuplicateExx( ps
																	  , pText
																	  , bKeepEOL
																	  , bSubst
																		, NULL DBG_LOCAL ) );

			pSubst->flags = pText->flags | TF_INDIRECT | TF_DEEP;
			pText = NULL; // done. have to be.
		}
		else if( pText &&
			  pText->flags & TF_INDIRECT )
		{
			pSubst = SegCreateIndirect( MacroDuplicateExx( ps
																	  , GetIndirect( pText )
																	  , bKeepEOL
																	  , bSubst
																	  , NULL DBG_LOCAL ) );
			pSubst->flags = pText->flags;
			pText = NEXTLINE( pText );
		}
		if( pSubst )
		{
			if( first )
				pSubst->format.position.offset.spaces = 0;
			else
				pSubst->format.position.offset.spaces = spaces;
			pSubst->flags |= TF_DEEP;
			pNew = SegAppend( pNew, pSubst );
			first = FALSE;
			pArgs = NEXTLINE( pArgs );
			continue;
		}
		if( ps && len && bSubst )
		{
			// SubstToken is responsible for token advance...
			pSubst = SubstToken( ps, &pText, FALSE, FALSE );
		}
		else
		{
			pSubst = pText;
			pText = NEXTLINE( pText );
		}

		if( pSubst )
		{
			// pSubst was a variable reference, and this indirection
			// points to the actual content...
			if( pSubst->flags & TF_TEMP )
			{
				pSubst->format.position.offset.spaces = spaces;
				pNew = SegAppend( pNew, pSubst );
			}
			else
			{
				if( pSubst->flags & TF_INDIRECT )
				{
					PTEXT pInd;
					pNew = SegAppend( pNew,
											pInd = SegCreateIndirect(
													TextDuplicateEx( GetIndirect( pSubst ),
																		  FALSE
																		 DBG_LOCAL ) ) ); //
					pInd->format.position.offset.spaces = spaces;
					pInd->flags |= TF_DEEP;
				}
				else
				{
					PTEXT pDup;
					pNew = SegAppend( pNew, pDup = SegDuplicateEx( pSubst DBG_RELAY ) );
					pDup->format.position.offset.spaces = spaces;
				}
			}
			if( first )
			{
				pNew->format.position.offset.spaces = 0;
				first = FALSE;
			}
		}
		pArgs = NEXTLINE( pArgs );
	}
	SetStart( pNew );

	// show what the result of this is - then check value in addmacrocommand
	//{
	//	PTEXT x = BuildLine( pNew );
	//	_lprintf( DBG_AVAILABLE, "Macroduplicate result:%p %s" DBG_RELAY, pNew, GetText( x ) );
	//	LineRelease( x );
	//}
	return pNew;
}

//--------------------------------------------------------------------------


//--------------------------------------------------------------------------
PTEXT DeeplyBurst( PTEXT pText ) /*FOLD00*/
{
	PTEXT p, start;
	start = p = pText;
	while( p )
	{
		if( p->flags & IS_DATA_FLAGS )
		{
			PTEXT pNew, pBurst;
			if( !(p->flags & TF_INDIRECT) ) // assume we've already done this
			{
				pNew = SegCreateIndirect( NULL );
				pNew->flags |= p->flags & IS_DATA_FLAGS;
				SegSubst( p, pNew );
				pBurst = burst( p );
				SetIndirect( pNew, (uintptr_t)DeeplyBurst( pBurst ) );
				pNew->flags |= TF_DEEP;
				LineRelease( p );
				if( start == p )
					start = pNew;
				p = pNew;
			}
		}
		p = NEXTLINE( p );
	}
	return start;
}
//--------------------------------------------------------------------------
// ps passed for substitution
// pe passed for destination object to receive the variable
// temp is a single text token with the name of the variable
// parameters is a unsubstituted command line....


CORE_PROC( void, AddVariableExxx )( PSENTIENT ps, PENTITY pe /*FOLD00*/
						, PTEXT pName, PTEXT parameters, int bBinary, int bForceEnt
							 , int bNoSubst
						  DBG_PASS )
{
	PLIST *ppList;
	//PTEXT temp;
	PSENTIENT psEnt;
	PTEXT pInd;
	int pass = 0;
	if( !pName || !GetText( pName ) )
		return;
	{
		if( !pe )
			pe = ps->Current;
		{
			INDEX idx;
			volatile_variable_entry *pvve = NULL;
			// look for it in registered variable extensions (methods, but as vars)
			LIST_FORALL( pe->pVariables, idx, volatile_variable_entry*, pvve )
			{  
				if( StrCmp( pvve->pName, GetText( pName ) ) == 0 )
					break;
			}
			if( pvve )
			{
				PTEXT pSubstituted = MacroDuplicateEx( ps, parameters, FALSE, TRUE );
				if( pvve->set )
					pvve->set( pe, pSubstituted );
				return;
			}
		}

	}
	psEnt = pe->pControlledBy;
	while( pass < 2 )
	{
		if( psEnt &&
			 psEnt == ps &&
			 psEnt->CurrentMacro &&
			 !pass &&
			 !bForceEnt )
		{
			ppList = &psEnt->CurrentMacro->pVars;
			pass = 1;
		}
		else
		{
			ppList = &pe->pVars;
			pass = 2;
		}

		{
			INDEX idx;
			LOGICAL bFound = FALSE;
			PTEXT var;
			LIST_FORALL(*ppList,idx,PTEXT, var)
			{
				if( SameText( var, pName ) == 0 )
				{
					PTEXT pNext;
					pNext = NEXTLINE( var );
					LineRelease( GetIndirect( pNext ) );
					SetIndirect( pNext, (uintptr_t)NULL );
					pInd = pNext;
					bFound = TRUE;
					break;
				}
			}
			if( !bFound ) // new varible in this context...
			{
				if( pass == 2 )
				{
					if( psEnt == ps &&
						 psEnt &&
						 psEnt->CurrentMacro &&
						 !bForceEnt )
						ppList = &psEnt->CurrentMacro->pVars;
					else
						ppList = &pe->pVars;
					if( pName->flags & TF_INDIRECT )
					{
						var = BuildLine( pName );
					}
					else
						var = SegDuplicateEx( pName  DBG_RELAY );
					AddLink( ppList, var );
					SegAppend( var, pInd = SegCreateIndirect(NULL) );
				}
				else
					continue;
			}

			{
				// SegAppend( var, pInd = SegCreateIndirect(NULL) );
				if( bBinary )
					// can't do substitution on binarys....
					SetIndirect( pInd, TextDuplicateEx( parameters, FALSE DBG_RELAY ) );
				else
				{
					parameters = DeeplyBurst( parameters );
					if( !ps && psEnt )
					{
						if( bNoSubst )
							SetIndirect( pInd, TextDuplicateEx( parameters, FALSE  DBG_RELAY ) );
						else
							SetIndirect( pInd, MacroDuplicateEx( psEnt, parameters, TRUE, TRUE ) );
					}
					else if( ps )
					{
						if( bNoSubst )
							SetIndirect( pInd, TextDuplicateEx( parameters, FALSE  DBG_RELAY ) );
						else
							SetIndirect( pInd, MacroDuplicateEx( ps, parameters, TRUE, TRUE ) );
					}
					else // !ps && !psEnt
						SetIndirect( pInd, TextDuplicateEx( parameters, FALSE  DBG_RELAY ) );
				}
				pInd->flags|= TF_DEEP; //need depth when object deletes...
				if( bBinary )
					pInd->flags |= TF_BINARY;
				else
					pInd->flags &= ~TF_BINARY;
				break;
			}
		}
	}
}

//----------------------------------------------------------------------

CORE_PROC( PTEXT, GetFileName )( PSENTIENT ps, PTEXT *parameters ) /*FOLD00*/
{
	PTEXT seg;
	PTEXT line = NULL;
	PTEXT result;
	while( ( seg = GetParam( ps, parameters ) ) )
	{
		line = SegAppend( line, ( seg->flags & TF_INDIRECT )?TextDuplicate( seg, FALSE ):SegDuplicate( seg ) );
	}
	if( line )
	{
		line->flags &= ~(TF_SQUOTE|TF_QUOTE);
		line->format.position.offset.spaces = 0;

		result = BuildLine( line );
		LineRelease( line );
		//lprintf( "GetFileName = %s", GetText( result ) );
	}
	else
		result = NULL;
	return result;
}

//--------------------------------------------------------------------------
int RELAY( PSENTIENT ps, PTEXT parameters ) /*fold00*/
{
	ps->flags.bRelay = !ps->flags.bRelay;
	return 0;
}

//--------------------------------------------------------------------------
int HELP( PSENTIENT ps, PTEXT parameters ) /*fold00*/
{
	PTEXT temp;
	if( parameters )
		while( ( temp = GetParam( ps, &parameters ) ) )
			Help( ps, temp );
	else
		Help( ps, NULL );
	return FALSE;
}
//--------------------------------------------------------------------------
int METHODS( PSENTIENT ps, PTEXT parameters ) /*fold00*/
{
	Methods( ps );
	return FALSE;
}

int COUNT( PSENTIENT ps, PTEXT parameters )
{

	return 0;
}
//--------------------------------------------------------------------------
int UNIMPLEMENTED( PSENTIENT ps, PTEXT parameters ) /*FOLD00*/
{
	 PTEXT temp;
	 PVARTEXT vt;
	 vt = VarTextCreate();
	 vtprintf( vt, "Command \"%s\" is unimplemented.",
				 GetText(PRIORLINE(parameters) ) );
	 EnqueLink( &ps->Command->Output, VarTextGet( vt ) );
	 while( ( temp = GetParam( ps, &parameters ) ) )
	 {
		  vtprintf( vt, "	 Parameter: \"%s\"", GetText(temp) );
		  EnqueLink( &ps->Command->Output, VarTextGet( vt ) );
	 }
	 VarTextDestroy( &vt );
	 return FALSE;
}

//--------------------------------------------------------------------------

int SCRIPT( PSENTIENT ps, PTEXT parameters )  // can actually be done IN script... /*FOLD00*/
{
	PMACROSTATE pms;
	PTEXT temp;
	pms = InvokeMacro( ps
					, global.script
					, temp = SegCreateIndirect( GetFileName( ps, &parameters ) ) 
					);
	temp->flags |= TF_DEEP;
	pms->state.flags.forced_run = 1;
	return 0;
}

//--------------------------------------------------------------------------

CORE_PROC( PMACRO, GetMacro )( PENTITY pe, CTEXTSTR pNamed ) /*FOLD00*/
{
	PMACRO match;
	uint16_t idx;
	PLIST pMacroList = pe->pMacros;
	/* check local objects for the command. */
	LIST_FORALL( pMacroList, idx, PMACRO, match )
	{
		if( NameIs( match, pNamed ) )
		{
			return match;
		}
	}
	return NULL;
}

//--------------------------------------------------------------------------

int CanProcess( PSENTIENT ps, Function function ) /*FOLD00*/
{
	//lprintf( "f is %p  (%p or %p?)", function, CMD_ENDMACRO, CMD_MACRO );
	if( function == CMD_ENDMACRO ||
		  function == CMD_MACRO ) // always can execute end...
		return TRUE;

	if( ps->pRecord )
	{
		//lprintf( "Recording... no process." );
		return FALSE;
	}

	if( !ps->CurrentMacro ) // then always process...
	{
		//lprintf( "not running in a macro, process." );
		return TRUE;
	}

	// label search pre-empts all others...
	if( ps->CurrentMacro->state.flags.bFindLabel )
	{
		if( !function )
		{
			lprintf( "not a command, skip." );
			return FALSE;
		}
		if( function == CMD_LABEL )
			return TRUE;  // can do this statement....
		return FALSE;
	}

	if( ps->CurrentMacro->state.flags.bFindEndIf ||
		 ps->CurrentMacro->state.flags.bFindElse )
	{
		if( !function )  // skip any macro command or send....
			return FALSE;

		if( function == CMD_IF )
		{
			// just count it for this test....
			ps->CurrentMacro->state.flags.data.levels++;
			return FALSE;
		}

		if( function == CMD_ELSE )
		{
			// is an else of an inner IF...
			if( ps->CurrentMacro->state.flags.data.levels > 0 )
				return FALSE;  // not this else...
			// if we weren't looking for an else we were looking for endif...
			if( !ps->CurrentMacro->state.flags.bFindElse )
				return FALSE;
			return TRUE;
		}

		if( function == CMD_ENDIF )
		{
			if( ps->CurrentMacro->state.flags.data.levels > 0 )
			{
				ps->CurrentMacro->state.flags.data.levels--;
				return FALSE;
			}
			return TRUE;  // okay to go now...
		}
		return FALSE;
	}
	return TRUE; // shouldn't be a problem to do this statement.
}

//--------------------------------------------------------------------------

void EnqueCommandProcess( PTEXT pName, PLINKQUEUE *ppOutput, PTEXT pCommand ) /*FOLD00*/
{
	PTEXT pOut, pLeader;
	pLeader = SegAppend( SegCreateIndirect( pName )
							 , SegCreateFromText( " Processing:" ) );
	SegAppend( pLeader, SegCreateIndirect( pCommand ) );
	pOut = BuildLine( pLeader );
	//lprintf( "%s", GetText( pOut ) );
	EnqueLink( ppOutput,  pOut );
	LineRelease( pLeader );
}

//--------------------------------------------------------------------------

void EnqueBareCommandProcess( PTEXT pName, PLINKQUEUE *ppOutput, PTEXT pCommand ) /*FOLD00*/
{
	PVARTEXT vt;
	PTEXT pOut, pLeader;
	vt = VarTextCreate();
	vtprintf( vt, "%s Command(%p):", GetText( pName ), pCommand );
	pLeader = VarTextGet( vt );
	VarTextDestroy( &vt );
	SegAppend( pLeader, SegCreateIndirect( pCommand ) );
	pOut = BuildLine( pLeader );
	Log( GetText( pOut ) );
	LineRelease( pLeader );
	LineRelease( pOut );
}

//--------------------------------------------------------------------------

void EnqueCommandRecord( PSENTIENT ps, PTEXT pName, PLINKQUEUE *ppOutput, PTEXT pCommand ) /*FOLD00*/
{
	PVARTEXT pvt = VarTextCreate();
	PTEXT pOut, pLeader;
	if( !ps->pRecord )
	{
		vtprintf( pvt, "%s Recording(unknown) :"
					);

	}
	else
		vtprintf( pvt, "%s Recording(%s) :"
				  , GetText( pName )
				  , GetText( GetName( ps->pRecord ) ) );
	pLeader = VarTextGet( pvt );
	VarTextDestroy( &pvt );
	SegAppend( pLeader, SegCreateIndirect( pCommand ) );
	pOut = BuildLine( pLeader );
	lprintf( "%s", GetText( pOut ) );
	EnqueLink( ppOutput,  pOut );
	LineRelease( pLeader );
}

//--------------------------------------------------------------------------
CORE_PROC( void, AddMacroCommand )( PMACRO pMacro, PTEXT Params )
{
	if( !pMacro )
	{
		if( Params )
			LineRelease( Params );
		return;
	}
	if( Params )
	{
		// this indicates where the error is... between this and macro duplicate result...
		//_lprintf( 0, "Adding a macro command to %s(%d)=%p"
		//		  , GetText( GetName( pMacro ) )
		//		  , (pMacro)->nCommands, Params );
		pMacro->nCommands++;
		AddLink( &pMacro->pCommands, Params );
	}
}
//--------------------------------------------------------------------------

CORE_PROC( PMACRO, LocateMacro )( PENTITY pe, CTEXTSTR name ) /*FOLD00*/
{
	//CTEXTSTR data;
	//data = GetText( Command );
	PMACRO match;

	match = GetMacro( pe, name );
	if( !match )
	{
		// if no macro found, check for macro in room...
		match = GetMacro( FindContainer( pe ), name );
	}
	return match;
}

//--------------------------------------------------------------------------

PMACROSTATE InvokeMacroEx( PSENTIENT ps, PMACRO pMacro, PTEXT pArgs, void (CPROC*StopEvent)(uintptr_t psvUser, PMACROSTATE pms ), uintptr_t psv ) /*FOLD00*/
{
	MACROSTATE MacState;
	// Begin Macro
	MacState.nCommand = 0;
	// this needs to duplicate pMacro -nArgs then
	// append an indirect to the remainder of the line...
	MacState.pArgs = pArgs;
	MacState.pVars = NULL;
	pMacro->flags.un.macro.bUsed = TRUE;
	MacState.pMacro = pMacro;
	MemSet( &MacState.state.flags, 0, sizeof( MacState.state.flags ) );
	//MacState.state.flags. = 0;
	MacState.state.flags.macro_suspend = 0;
	MacState.peInvokedOn = ps->Current;
	MacState.state.flags.data.levels = 0; // overflow
	MacState.pdsForEachState = CreateDataStack( sizeof( FOREACH_STATE ) );
	MacState.pInvokedBy = ps->pToldBy; // might have multiple leaders...
	MacState.MacroEnd = NULL;
	MacState.StopEvent = StopEvent;
	MacState.psv_StopEvent = psv;
	PushData( &ps->MacroStack, &MacState ); // changes address of all data content...
	return (PMACROSTATE)PeekData( &ps->MacroStack );
}

#undef InvokeMacro
CORE_PROC( PMACROSTATE, InvokeMacro )( PSENTIENT ps, PMACRO pMacro, PTEXT pArgs )
{
	return InvokeMacroEx( ps, pMacro, pArgs, NULL, 0 );
}
//--------------------------------------------------------------------------

int SendLiteral( PSENTIENT ps, PTEXT *RealCommand, PTEXT Command, PTEXT EndLine )
{
	if( CanProcess( ps, NULL ) )
	{
		if( ps->Data )
		{
			PTEXT pSend;
			// must restore EOL to send to data channel...
			if( gbTrace )
			{
				EnqueCommandProcess( GetName( ps->Current ), &ps->Command->Output, *RealCommand );
				if( ps->Command->Write )
					ps->Command->Write( ps->Command );
			}

			if( EndLine )
			{
				// this really looks wrong - someday figure this out.
				*RealCommand = SegAppend( *RealCommand, EndLine );
				// Need to set command if there was a blank line
				// cause way above we strip off the newline of the command
				// and here we had to put it back.  BUT - if there was
				// data like say '.send this' then command is sitting after
				// the . and needs to remain where it was... otherwise
				// it resets to the start and sends the . also...
				// wonder if this fixes !s
				if( !Command )
					Command = *RealCommand;
			}
			pSend = MacroDuplicateEx( ps, Command, TRUE, TRUE );
			EnqueLink( &ps->Data->Output, pSend );
			return FALSE;
		}
	}
	if( EndLine )
		SegAppend( *RealCommand, EndLine );
	return FALSE; // no effect...
}

int Process_Command(PSENTIENT ps, PTEXT *RealCommand) /*FOLD00*/
{
	int32_t idx = 0;
	TEXTCHAR *data;
	size_t sig;
	uint32_t slash_count, syscommand;
	PTEXT Command = *RealCommand;
	PTEXT EndLine = NULL;

	if( !Command )
		return 0;
	if( !ps->Command )
		return 0; // command queue is no longer valid...

	if( global.flags.bLogAllCommands || gbTrace )
		EnqueBareCommandProcess( GetName( ps->Current )
									  , &ps->Command->Output
									  , *RealCommand );

	{ // remove carriage return from command... ( I think this is somewhere else too... )
		PTEXT pReturn;
		pReturn = Command;
		while( pReturn )
		{
			// first ,
			PTEXT pNext;
			pNext = NEXTLINE( pReturn );

			// it's a null line segment, it's a line segment stream termination, and it's not blank
			// because it's really encoding, or indirect.
			if( !(pReturn->flags & (TF_SENTIENT|TF_ENTITY|IS_DATA_FLAGS) )
				&& !GetTextSize( pReturn ) )
			{
				EndLine = SegGrab( pReturn );
				// if the very first segment is blank, then there's no command
				// and we skip command processing and send a blank line directly to output.
				if( pReturn == Command )
				{
					Command = NULL;
					*RealCommand = NULL;
					idx = -1;
					//Log( "Sending blank line?!" );
					return SendLiteral( ps, RealCommand, Command, EndLine );
				}
			}
			if( EndLine && pNext )
			{
				Log( "Command input contained EOL within the line... FAULT" );
				DebugBreak();
			}
			pReturn = pNext;
		}
	}


	slash_count = 0;
	syscommand = 0;
	ps->pToldBy = NULL; // not sourced from anyone...

	// from other command procesor commands, we may know that the command
	// was given from myself or another sentient... it will be prefixed to the command
	while( Command && Command->flags&TF_SENTIENT )
	{
		// used to be GetIndirect - but that returns NULL now -
		// for purposes of TextDuplicate.
		ps->pToldBy = (PSENTIENT)Command->data.size;
		Command = NEXTLINE( Command );
	}

	if( !Command )
	{
		DECLTEXT( msg, "null command told to me..." );
		EnqueLink( &ps->Command->Output, &msg );
		return SendLiteral( ps, RealCommand, Command, EndLine );
	}


	// if recording, append this command literally into a macro.
	// macro duplicate does one level of substitiuion on the command
	// (nessecitating double %% notation of variable references.
	if( ps->pRecord )
	{
		if( gbTrace  ||
			( ps->CurrentMacro &&
			  ps->CurrentMacro->state.flags.bTrace ) )
		{
			// this should be prefixed... RECORDING....
			EnqueCommandRecord( ps, GetName( ps->Current ), &ps->Command->Output, *RealCommand );
			if( ps->Command->Write ) // flush output...
				ps->Command->Write( ps->Command );
		}
		AddMacroCommand( ps->pRecord
							, MacroDuplicateEx( ps, Command, TRUE, FALSE ));
		// just append everythign, and get out of here... no reason to process it (unless it's endmac)
		idx = -3;
	}


Recheck:
	if( !Command ||
		  !ps ||
		  !ps->Current) // if somehow it's a disembodied sentience...
	{
		return SendLiteral( ps, RealCommand, Command, EndLine );
	}

	data = GetText( Command );

	// preferred punctuation for transmit literal
	if( data[0] == '.' )
	{
		//Log( "Consider send to datapath" );
		if( CanProcess( ps, NULL ) )
		{
			if( GetTextSize( Command ) == 1 )
			{
				//Log( "Step to next token..." );
				Command = NEXTLINE( Command );// skip period....
			}
			else
			{
				PTEXT period;
				//Log( "Do split on line..." );
				period = SegAppend( SegCreateFromText( "." )
										  , SegCreateFromText( data+1 ) );
				period->format.position.offset.spaces = Command->format.position.offset.spaces;
				period->flags = Command->flags & (IS_DATA_FLAGS);
				LineRelease( SegSubst( Command, period ) );
				*RealCommand = period;
				Command = NEXTLINE( period );
			}
			//Log( "Send to datapath" );
			return SendLiteral( ps, RealCommand, Command, EndLine );
		}
		else
		{
			if( EndLine )
				SegAppend( *RealCommand, EndLine );
			return FALSE;
		}
	}

	// escape into system process...
	if( !ps->pRecord && data[0] == '!' )
	{
		PTEXT evalcommand;
		PDATAPATH pdp;
		DECLTEXT( dev, "system" );
		DECLTEXT( devname, "system" );
		DECLTEXT( dev2, "ansi" );
		DECLTEXT( dev2opt, "inbound newline" );
		DECLTEXT( dev2name, "system parse" );
		PTEXT exclam;
		if( data[1] )
		{
			//Log( "Do split on line..." );
			exclam = SegAppend( SegCreateFromText( "!" )
				, SegCreateFromText( data + 1 ) );
			exclam->format.position.offset.spaces = Command->format.position.offset.spaces;
			exclam->flags = Command->flags & (IS_DATA_FLAGS);
			LineRelease( SegSubst( Command, exclam ) );
			*RealCommand = exclam;
		}
		else
			exclam = Command;
		Command = NEXTLINE( exclam );
		evalcommand = MacroDuplicateEx( ps, Command, FALSE, TRUE );
		if( ( pdp = OpenDevice( &ps->Data, ps, (PTEXT)&dev, evalcommand ) ) )
		{
			//PDATAPATH pdp2a;
			PDATAPATH pdp2;
			PTEXT tmp;
			pdp->pName = (PTEXT)&devname;
			//pdp2 = OpenDevice( &ps->Data, ps, (PTEXT)&devdbg, tmp = SegAppend( SegCreateFromText( "outbound" ), SegCreateFromText( "log" ) ) );
			//if( pdp2) pdp2->pName = (PTEXT)&devdbgname;
			//LineRelease( tmp );
			//pdp2a = OpenDevice( &ps->Data, ps, (PTEXT)&dev2a, NULL );
			//if( pdp2a) pdp2a->pName = (PTEXT)&devname2a;
			tmp = burst( (PTEXT)&dev2opt );
			pdp2 = OpenDevice( &ps->Data, ps, (PTEXT)&dev2, tmp );
			if( pdp2) pdp2->pName = (PTEXT)&dev2name;
			LineRelease( tmp );
			//pdp2 = OpenDevice( &ps->Data, ps, (PTEXT)&dev3, (PTEXT)&dev3opt );
			//if( pdp2) pdp2->pName = (PTEXT)&dev3name;
			//pdp2 = OpenDevice( &ps->Data, ps, (PTEXT)&devdbg, tmp = SegAppend( SegCreateFromText( "inbound" ), SegCreateFromText( "log" ) ) );
			//if( pdp2) pdp2->pName = (PTEXT)&devdbgname;
			//LineRelease( tmp );

			if( ps->CurrentMacro )
				ps->CurrentMacro->state.flags.bSuccess = TRUE;
		}
		LineRelease( evalcommand );
		if( EndLine )
			SegAppend( *RealCommand, EndLine );
		return TRUE;
	}

	// valid command - other pucntuation may have occured...
	sig = GetTextSize(Command); // remove NULL - assume full word parse.
	if( data[0] == '/' )
	{
		if( !(slash_count++) )
		{	
			Command = NEXTLINE( Command );
			goto Recheck;
		}
		else
		{
			// slash slash ...
			if( CanProcess( ps, NULL ) )
			{
				extern void Tell( PSENTIENT ps // source
									 , PSENTIENT pEnt  // destination
									 , PTEXT pWhat	// command parameter string...
									 , int bSubst );
				if( gbTrace )
				{
					// this should be prefixed... RECORDING....
					EnqueCommandProcess( GetName( ps->Current ), &ps->Command->Output, *RealCommand );
					if( ps->Command->Write )
						ps->Command->Write( ps->Command );
				}
				if( ps->pLastTell )
				{
					if( !ps->pLastTell->pControlledBy )
					{
						// directly dispatch the command using my own sentience
						// on the new object...
						PENTITY peMe = ps->Current;
						int status;
						ps->Current = ps->pLastTell;
						ps->pLastTell->pControlledBy = ps;
						lprintf( "Executing command on entity without awareness using my own sentience." );
						status = Process_Command( ps, RealCommand );
						ps->pLastTell->pControlledBy = NULL;
						ps->Current = peMe;
						return status;
					}
					else
					{
						ps->flags.waiting_for_someone = 1;
						Tell( ps, ps->pLastTell->pControlledBy, Command, FALSE );
					}
				}
				idx = -3; // fail all further processing on this here...
			}
			else
			{
				if( EndLine )
					SegAppend( *RealCommand, EndLine );
				return TRUE; // stored into macro...
			}
		}
	}
	else
	{
		if( slash_count || ps->CurrentMacro )
		{
			int result;
			INDEX i;
			PENTITY pe = ps->Current;
			PCLASSROOT pce;

			if( result = RoutineRegistered( ps, Command ) )
			{
				//lprintf( "Executed by registered command..." );
				if( EndLine )
					SegAppend( *RealCommand, EndLine );
				return TRUE;
			}
			if( result == -1 )
			{
				// no command processing can happen.
				if( EndLine )
					SegAppend( *RealCommand, EndLine );
				return FALSE;
			}

			// didn't find command... else it was not able to process...
			// and if a normal command can't - a method definatly cannot process??!!

			LIST_FORALL( pe->pMethods, i, PCLASSROOT, pce )
			{
				PCLASSROOT root = GetClassRootEx( pce, "methods" );
				CTEXTSTR name;
				PCLASSROOT current;
				for( name = GetFirstRegisteredName( (CTEXTSTR)root, &current );
					 name;
					  name = GetNextRegisteredName( &current ) )
				{
					if( StrCaseCmp( GetText( Command ), name ) == 0 ) // command was on THIS object...
					{
						ObjectFunction f = GetRegisteredProcedure2( root, int, name, (PSENTIENT,PENTITY,PTEXT) );
						//if( CanProcess( ps, f ) )
						{
							if( gbTrace ||
								( ps->CurrentMacro &&
								 ps->CurrentMacro->state.flags.bTrace ) )
							{
								EnqueCommandProcess( GetName( ps->Current ), &ps->Command->Output, *RealCommand );
								if( ps->Command->Write )
									ps->Command->Write( ps->Command );
							}
							if( ps->CurrentMacro )
							{
								ps->CurrentMacro->state.flags.bSuccess = FALSE;
							}
							if( f )
								f( ps, pe, NEXTLINE( Command ) );

							// here we found a method, ran it, and return.
							if( EndLine )
								SegAppend( *RealCommand, EndLine );
							return TRUE;
						}
					}
				}
			}
		}
		else // not in a macro, and no slashes... not really a command.
			idx = -2; // skip OVER macro processing...
	}


	{
		PMACRO match;
		/* check local objects for the command. */

		if( idx > -2 )
		{
			if( !CanProcess( ps, NULL ) )
			{
				if( EndLine )
					SegAppend( *RealCommand, EndLine );
				return FALSE;
			}

			if( ( match = LocateMacro( ps->Current, GetText(Command) ) ) ||
					 ( match = LocateMacro( ps->Current->pWithin, GetText(Command) ) ) ||
				 ( match = LocateMacro( global.THE_VOID, GetText(Command) ) ) )
			{
				PTEXT argline;
				PTEXT pArgs;
				if( gbTrace ||
					 ( ps->CurrentMacro &&
						  ps->CurrentMacro->state.flags.bTrace ) )
				 {
					EnqueCommandProcess( GetName( ps->Current ), &ps->Command->Output, *RealCommand );
					if( ps->Command->Write )
						ps->Command->Write( ps->Command );
				}
				argline = NEXTLINE(Command);
				if( match->nArgs > 0 )
				{
					int i;
					argline = Command;
					if( argline->flags & TF_INDIRECT )
					{
						argline = GetIndirect( argline );
						if( NEXTLINE( argline ) )
							argline = NEXTLINE( argline );
					}
					else
						argline = NEXTLINE( argline );
					if( ( ( i = CountArguments( ps, argline ) ) < match->nArgs )
					  || ( i < 0 && ( -i < match->nArgs ) ) )
					{
						PVARTEXT vt;
						PTEXT t;
						vt = VarTextCreate( );
						vtprintf( vt, "Macro requires: " );
						t = match->pArgs;
						while( t )
						{
							vtprintf( vt, "%s", GetText( t ) );
							t = NEXTLINE( t );
							if( t )
								vtprintf( vt, ", " );
						}
						vtprintf( vt, "." );
						EnqueLink( &ps->Command->Output, VarTextGet( vt ) );
						VarTextDestroy( &vt );
						return FALSE; // return...
					}	
					if( ( i > match->nArgs ) || ( -i > match->nArgs ) )
					{
						DECLTEXT( msg, "Extra parameters passed to macro. Continuing but ignoring extra." );
						EnqueLink( &ps->Command->Output, &msg );
					}
				}
				pArgs = MacroDuplicateExx( ps, argline, TRUE, TRUE, match->pArgs DBG_SRC);
				InvokeMacroEx( ps, match, pArgs, NULL, 0 );
				return TRUE;
			}


			// try and send this as a command to an object, so locate visible objects that might
			// have this name...
			{
				PENTITY pe;
				if( CanProcess( ps, NULL ) )
				{
					if( ( pe = (PENTITY)FindThing( ps, &Command, ps->Current, FIND_VISIBLE, NULL ) ) )
					{
						if( !pe->pControlledBy )
						{
							// directly dispatch the command using my own sentience
							// on the new object...
							PENTITY peMe = ps->Current;
							int status;
							ps->Current = pe;
							pe->pControlledBy = ps;
							status = Process_Command( ps, &Command );
							pe->pControlledBy = NULL;
							ps->Current = peMe;
							return status;
						}
						else //if( pe->pControlledBy )
						{
							extern void Tell( PSENTIENT ps // source
										, PSENTIENT pEnt  // destination
										, PTEXT pWhat // command parameter string...
										, int bSubst
										); 
							if( gbTrace ||
										 ( ps->CurrentMacro &&
										 ps->CurrentMacro->state.flags.bTrace ) )
										EnqueCommandProcess( GetName( ps->Current ), &ps->Command->Output, Command );
									ps->flags.waiting_for_someone = 1;
							Tell( ps, pe->pControlledBy, Command, FALSE );
						}
						if( EndLine )
							SegAppend( *RealCommand, EndLine );
						return TRUE;
					}
				}
			}
			// if macro was not found, check for registered plugin
			// command...  okay then we check registered routines...
			// putting emphasis on namespace as such...
			//
			//  macro name
			//  object name [ /macro name?]
			//  registered command object
			//	  - by internal methods
			//	  - within the procreg tree
		}
		// default action if line was not a command or macro or plugin command
		if( idx > -3 )
		{
			return SendLiteral( ps, RealCommand, Command, EndLine );
		}
	}
	if( EndLine )
		SegAppend( *RealCommand, EndLine );
	return TRUE;
}

//--------------------------------------------------------------------------

PMACRO CreateMacro( PENTITY pEnt, PLINKQUEUE *ppOutput, PTEXT name ) /*FOLD00*/
{
	PMACRO pMacro;

	if( !pEnt || !GetRoutineRegistered( NULL, name ) )
	{
		if( pEnt )
			pMacro = GetMacro( pEnt, GetText(name) );
		else
			pMacro = NULL;
			
		if( pMacro )
			DestroyMacro( pEnt, pMacro );

		pMacro = New( MACRO );
		MemSet( pMacro, 0, sizeof( MACRO ) );
		pMacro->Used = 1; // used once.
		pMacro->flags.bMacro = TRUE; 
		pMacro->pName = name;
		pMacro->pDescription = NULL;
		pMacro->pCommands = NULL;

		{
			PCLASSROOT current = NULL;
			CTEXTSTR name2;
			CTEXTSTR root;
			INDEX idx;

			CTEXTSTR extension_name;
			LIST_FORALL( global.ExtensionNames, idx, CTEXTSTR, extension_name )
			{
				if( GetLink( &pEnt->pPlugin, idx ) )
				{
					PVARTEXT pvt = VarTextCreate();
					vtprintf( pvt, "dekware/objects/%s/macro/create", extension_name );
					for( name2 = GetFirstRegisteredName( root = GetText( VarTextPeek( pvt ) ), &current );
				 		name2;
					  name2 = GetNextRegisteredName( &current ) )
					{
						MacroCreateFunction f = GetRegisteredProcedure2( root, void, name2, (PENTITY,PMACRO) );
						if( f )
						{
							f( pEnt, pMacro );
						}
					}
					VarTextDestroy( &pvt );
				}
			}
		}

		if( pEnt )
		{
			AddLink( &pEnt->pMacros, pMacro );
		}
		return pMacro;
	}
	else
	{
		DECLTEXT( msg, "Macro name overlaps internal command." );
		EnqueLink( ppOutput, &msg );
	}

	return NULL;
}

//--------------------------------------------------------------------------

PMACRO DuplicateMacro( PMACRO pm ) /*FOLD00*/
{
	if( pm )
	{
		pm->Used++;
		lprintf(" Macro %s usage count is now %d", GetText( pm->pName ), pm->Used );
	}
	return pm;
	/*
	PMACRO pmNew;
	PTEXT pLine;
	INDEX idx;
	// this will ALWAYS result in a macro...
	pmNew = CreateMacro( NULL, NULL, LineDuplicate( pm->pName ) );
	if( pmNew )
	{
		LineDuplicate( pm->pDescription );
		pmNew->pDescription = pm->pDescription;
		pmNew->nArgs = pm->nArgs;
		LineDuplicate( pm->pArgs );
		pmNew->pArgs = pm->pArgs;
		LIST_FORALL( pm->pCommands, idx, PTEXT, pLine )
		{
			LineDuplicate( pLine );
			AddMacroCommand( pmNew, pLine );
		}
	}
	return pmNew;
	*/
}

//---------------------------------------------------------------------------

CORE_PROC( void, QueueCommand )( PSENTIENT ps, CTEXTSTR Command )
{
	if( ps && Command )
	{
		PTEXT pc, pb;
		pc = SegCreateFromText( Command );
		pb = burst( pc );
		EnqueLink( &ps->Command->Input, pb );
		LineRelease( pc );
	}
}

//--------------------------------------------------------------------------

CORE_PROC( void, prompt )( PSENTIENT ps ) /*FOLD00*/
{
	DECLTEXT( nulprompt, "" );
	PTEXT text;
	PTEXT pPrompt;
	if( ps->flags.no_prompt )
		return;
	if( gbTrace )
	{
		//DECLTEXT( msg, "Issuing prompt..." );
		//EnqueLink( &ps->Command->Output, (PTEXT)&msg );
	}
	pPrompt = GetListVariable( ps->Current->pVars, "prompt" );
	pPrompt = GetIndirect( pPrompt );
	text = (PTEXT)&nulprompt;
	if( ps->pRecord && pPrompt )
	{
		DECLTEXT( colon, ":" );
		PTEXT pRec;
		if( ( pPrompt->flags & IS_DATA_FLAGS ) || GetTextSize( pPrompt ) )
			text = MacroDuplicateEx( ps, pPrompt, FALSE, TRUE );
		pRec = SegDuplicate( GetName( (PENTITY)ps->pRecord ) );
		pRec->flags |= TF_PAREN;
		// prec will have no spaces?
		SegAppend( text, pRec );
		// will have no spaces on colon constant...
		SegAppend( text, SegDuplicate( (PTEXT)&colon ) );
	}
	else if( ps->pRecord )
	{
		DECLTEXT( colon, ":" );
		PTEXT pRec;
		text = pRec = SegDuplicate( GetName( (PENTITY)ps->pRecord ) );
		pRec->flags |= TF_PAREN;
		SegAppend( pRec, SegDuplicate( (PTEXT)&colon ) );
	}
	else if( pPrompt )
	{
		PTEXT line;
		text = MacroDuplicateEx( ps, pPrompt, FALSE, TRUE );
		line = BuildLine( text );
		LineRelease( text );
		text = line;
	}
	else
		text->flags |= TF_NORETURN;
	{
		PTEXT p;
		p = text;
		while( p )
		{
			p->flags |= TF_PROMPT;
			p = NEXTLINE(p);
		}
	}

	if( text )
	{
		EnqueLink( &ps->Command->Output, text );
	}
	return;
}

CORE_PROC( void, vExecute )( PSENTIENT ps, TEXTCHAR *cmd, va_list args )
{
	PVARTEXT pvt;
	PTEXT cmdt, temp;
	if( !ps )
		ps = global.PLAYER;
	pvt = VarTextCreate();
	vvtprintf( pvt, cmd, args );
	cmdt = burst( temp = VarTextGet( pvt ) );
	LineRelease( temp );
	EnqueLink( &ps->Command->Input, cmdt );
	VarTextDestroy( &pvt );
}

CORE_PROC( void, Execute )( PSENTIENT ps, TEXTCHAR *cmd, ... )
{
	va_list args;
	va_start( args, cmd );
	vExecute( ps, cmd, args );
}

CORE_PROC( LOGICAL, vProcess )( PSENTIENT ps, TEXTCHAR *cmd, va_list args )
{
	// this will wait until the command completes
	// the command is a macro, all relatvent commands
	// will be executed until the macro completes.
	return FALSE; // this should be ps.macrostate.bSuccess
}

CORE_PROC( LOGICAL, Process )( PSENTIENT ps, TEXTCHAR *cmd, ... )
{
	va_list args;
	va_start( args, cmd );
	return vProcess( ps, cmd, args );
}

static int HandleCommand( "debug","dumpnames", "Call DumpRegisteredNames()" )( PSENTIENT ps, PTEXT params )
{
	DumpRegisteredNames();
	return 0;
}


