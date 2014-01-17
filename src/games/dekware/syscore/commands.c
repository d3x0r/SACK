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
   STORY,     // show introduction story....
   PROMPT,    // displays the current prompt....
   EXIT,
   CHANGEDIR, // change current working directory....
   SCRIPT,    // call run external command script [file]
   PARSE,     // just run a file (param1) through the parser
   COMMAND,   // open a device as command channel of sentience...
   FILTER,    // apply filters to datapaths... 
   OptionDevice,  // set options on a datapath(filter)
   CMD_GETWORD,   // get next word from current file being parsed...
   CMD_GETLINE,   // get until next line terminator (unwraps quotes?)
   CMD_GETPARTIAL,// get any partial expression from the parser... clears it.
   CMD_ENDPARSE,  // Stop parsing this file...
   CMD_ENDCOMMAND,// Stop parsing this file...
//   Filter, // /filter command/data devname device options
//   UnFilter, // unfilter <devname> - removes a filter

   //FORMATTED, // data input is not to be run through burst... merely gatherline...
   WAKE,      // wake an object to sentient mode....
   SUSPEND,   // stop an aware process from executing temporily
   RESUME,    // start a process which has been suspended...
   DELAY,     // pause a macro for ### milliseconds
   GetDelay,  // useful is the macro was resumed from a delay...
   KILL,      // put an aware object to sleep (permanently)

   ECHO,      // output to command data path...
   FECHO,     // formatted echo.. rare - but does not have linefeed prefix...
   CMD_PROCESS,// process <...> as if entered as a command 
   CMD_RUN,    // Run a macro to completion before handling command input
   DECLARE,    // create a variable type reference
   COLLAPSE,   // make data in a variable atomic
   BINARY,     // create a variable without substitution applied
   ALLOCATE,   // allocate a binary storage buffer of size...
   UNDEFINE,   // delete a variable...
 VARS,       // dump what current variables are present...
 VVARS,      // dump volatile variables (these are getting numerous)
   TELL,       // add input to a sentient object
   REPLY,      // uses the name of the object which told this to perform the action...
 SendToObject, // <to object> <data (inbound channel)>
 WriteToObject, // <to object> <data (outbound channel)>
   MONITOR,    // observe a previously aware object
 RELAY,      // auto relay command input to data output while data open
 PAGE, // generate a page break....

   CMD_INPUT,     // get next command input into a variable... whole line...

   //------------- object definition/declaration
   CREATE,     // [objectname]
   CreateRegisteredObject, // <object type> <object names....>
   _SHADOW,    // create a shadow of an object... within self...
   BECOME,     // leave your body and become another object.
   _DUPLICATE,  // object in hand.... [objectname]
   DESTROY,    // object in hand ... [objectname]
   DESCRIBE,   // attach a description for LOOKat(???)
 RENAME,     // change an object's name...
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
   CMD_STOP,   // forces current macro to end...
   VAR_PUSH,   // push a variable onto the current var stack...
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
   DIVIDE,    // var amount (must have both)
   BOUND,     // define boundry of a variable (upper, lower, equate )
   LALIGN,
   RALIGN,
   NOALIGN,
   UCASE,
   LCASE,
   PCASE,

   //------------- object manipulations
   JOIN,    // attach self to object
   ATTACH,  // put hand against [object]
   DETACH,  // take two items apart...
   ENTER,
   LEAVE,

   //------------- 
   DROP,  // put hand in LOCATION   - returns what what stored
   STORE, // put hand in pocket     - returns what was stored
   GRAB,  // get from pocket [object] - returns object
   LOOKFOR,    // find an entity named <thing>

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
   CMD_MACRO,      // begin recording a macro [macroname]
   CMD_ENDMACRO,   // stop recording macro [macroname]
   CMD_LIST,       // list the commands in a macro - NOT recorded.
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
#define DEFCMD( name,desc ) { DEFTEXT(WIDE(#name)),DEFTEXT(WIDE("Nexus Core")),0,(sizeof(WIDE(#name))/sizeof(TEXTCHAR))-1,DEFTEXT(desc),name }
 // the function to call and the name interpreted do not match...
#define DEFCMDEX( name,desc,func) { DEFTEXT(WIDE(#name)),DEFTEXT(WIDE("Nexus Core")),0,(sizeof(WIDE(#name))/sizeof(TEXTCHAR))-1,DEFTEXT(desc),func }

 command_entry commands[]={ DEFCMDEX(?,WIDE(""),HELP )
                          , DEFCMDEX(MACRO, WIDE("Create new macro."), CMD_MACRO)
                          , DEFCMDEX(ENDMACRO, WIDE("End Macro creation..."), CMD_ENDMACRO )
                          , DEFCMDEX(LIST, WIDE("List a macro definition."), CMD_LIST )
                          , DEFCMDEX(GETWORD, WIDE("Get Next parsed element from file"), CMD_GETWORD )
                          , DEFCMDEX(GETLINE, WIDE("Get Next parsed line from file"), CMD_GETLINE )
                          , DEFCMDEX(GETPARTIAL, WIDE("Gets any parital expression from the parser."), CMD_GETPARTIAL )
                          , DEFCMDEX(ENDPARSE, WIDE("Stop (close) file being parsed."), CMD_ENDPARSE )
                          , DEFCMDEX(CLOSE, WIDE("Stop (close) file being parsed."), CMD_ENDPARSE )
                          , DEFCMDEX(ENDCOMMAND, WIDE("Stop (close) command processing"), CMD_ENDCOMMAND )
                          , DEFCMDEX(IF, WIDE("Comparison operator <complex>"), CMD_IF )
                          , DEFCMDEX(ELSE, WIDE("Begin here if previous IF result false"), CMD_ELSE )
                          , DEFCMDEX(ENDIF, WIDE("End of IF expression"), CMD_ENDIF )
                          , DEFCMDEX(LABEL, WIDE("Define a goto name in macro"), CMD_LABEL )
                          , DEFCMDEX(GOTO, WIDE("Goto a label in a macro"), CMD_GOTO )
                          , DEFCMDEX(EXECUTE, WIDE("Perform parameters as a command"), CMD_PROCESS )
                          , DEFCMDEX(RUN, WIDE("Run a macro to completion"), CMD_RUN )
                          , DEFCMDEX(COMPARE, WIDE("Comparison... sets fail/success"), CMD_COMPARE )
                          , DEFCMDEX(RETURN, WIDE("End and return to prior macro"), CMD_RETURN )
                          , DEFCMDEX(PUSH, WIDE("Add variable to end of variable"), VAR_PUSH )
                          , DEFCMDEX(POP, WIDE("Take the first thing from src put in dest"), VAR_POP )
                          , DEFCMDEX(HEAD, WIDE("Get Variable from beginning of variable"), VAR_HEAD )
                          , DEFCMDEX(TAIL, WIDE("Get variable from end of variable"), VAR_TAIL )
                          , DEFCMDEX(BREAK, WIDE("!!Generate debug breakpoint!!"), CMD_BREAK )
                          , DEFCMDEX(DEBUG, WIDE("Start displaying executed commands"), CMD_TRACE )
                          , DEFCMDEX(STOP, WIDE("Tell object to end current macro"), CMD_STOP )
                          , DEFCMDEX(BURST, WIDE("Parse data... into variable"), CMD_BURST )
                          , DEFCMDEX(INPUT, WIDE("Get Next command input line into variable"), CMD_INPUT )
                          , DEFCMDEX(SHADOW, WIDE("Create a shadow of an object..."), _SHADOW )
                          , DEFCMDEX(WAIT, WIDE("Wait for data to be available..."), CMD_WAIT )
                          , DEFCMDEX(RESULT, WIDE("Set return result from macro"), CMD_RESULT )
                          , DEFCMDEX(GETRESULT, WIDE("Get result from a macro"), CMD_GETRESULT )
                          , DEFCMDEX(ON, WIDE("Define an action for an object"), DefineOnBehavior )
                          , DEFCMDEX(MAKE, WIDE("Create an object from an archtype"), CreateRegisteredObject )
								  , DEFCMDEX(SENDTO, WIDE("Send data directly to an object's input datapath"), SendToObject )
								  , DEFCMDEX(WRITETO, WIDE("Send data directly to an object's output datapath"), WriteToObject )
		 // if the same variable name is used within a foreach
		 //  foreach in item
		 //    /%item/foreach in item
		 //
		 // /foreach on %me content
		 //    /foreach near %me content
		 //    /step next
		 // /step content
       //
		 // /step is list goto
		 //   but I guess I need to remember the
		 //   stack of macrosteps that last had a loop
		 // /step next (off end of file will not goto, but will continue...
		 // /step first will reset the current position and should be reserved for exclusive
		 //    advanced programming efforts...
       // /step current - could also be used to go back to the beginnigng with the current element...
								  , DEFCMDEX(FOREACH, WIDE("[variable_list_name,on,in,around,near,exit(attached to room),visible] variable_name"), ForEach )
								  , DEFCMDEX(STEP, WIDE("[next(default),prior,first,last] variable_name"), StepEach )
								  , DEFCMDEX( EXTENSIONS, WIDE("List registered extension types"), ListExtensions )
                          , DEFCMDEX(GETDELAY, WIDE("Get last delay time, or if resumed delay left"), GetDelay )
                          , DEFCMDEX(OPTION, WIDE("Sets options for a datapath/filter."), OptionDevice )
                          , DEFCMDEX(CD, WIDE("Set Current Directory"), CHANGEDIR )
                          , DEFCMDEX(DUPLICATE, WIDE("copy an object and all it contains"), _DUPLICATE )
                          , DEFCMDEX(UNDECLARE, WIDE("Delete a variable"), UNDEFINE )
                          , DEFCMD( BECOME, WIDE("Become an object.") )
                          , DEFCMD(ATTACH, WIDE("Attach object in your hand to another.") ) //ATTACH)
                          , DEFCMD(CHANGEDIR, WIDE("Set Current Directory") ) // CHANGEDIR )
                          , DEFCMD(CREATE, WIDE("Make something") ) //CREATE)
                          , DEFCMD(DESCRIBE, WIDE("Add a description to an object.") ) //DESCRIBE)
                          , DEFCMD(DESTROY, WIDE("Destroy something") ) //DESTROY)
                          , DEFCMD(ECHO, WIDE("send output to command data channel...") )
                          , DEFCMD(PAGE, WIDE("echo a page break on the command path...") )
                          , DEFCMD(FECHO, WIDE("send output to command data channel...") ) // FECHO )
                          , DEFCMD(DETACH, WIDE("take apart an object") ) // DETACH )
                          , DEFCMD(DROP, WIDE("Drop the object in your hand") ) //DROP)
                          , DEFCMD(DUMP, WIDE("Display information about an object/macro/variable") ) // DUMP )
                          , DEFCMD(DUMPVAR, WIDE("Display information about a variable") ) // DUMPVAR )
                          , DEFCMD(ENTER, WIDE("Go into an object.") ) //ENTER)
                          , DEFCMD(EXIT, WIDE("leave this mess") ) //EXIT)
                          , DEFCMD(GRAB, WIDE("Put object in your hand.") ) //GRAB)
                          , DEFCMD(HELP, WIDE("this list :) ") ) //HELP)
                          , DEFCMD(METHODS, WIDE("List plugin methods on this object") ) //METHODS)
                          , DEFCMD(INVENTORY,WIDE("What you are holding.") ) //INVENTORY)
                          , DEFCMD(LEAVE, WIDE("Go out of an object.") ) //LEAVE)
                          // , DEFCMD(LOAD, WIDE("Load objects from a file to current location.") ) //LOAD)
                          , DEFCMD(LOOK, WIDE("Look at an object or the room.") ) //LOOK)
                          , DEFCMD(MAP, WIDE("Show a map of all cookies.") ) //MAP)
                          , DEFCMD(SAVE, WIDE("Save all objects to a file.") ) //SAVE)
                          , DEFCMD(SCRIPT, WIDE("Start parsing an external file as commands.") ) // SCRIPT)
                          , DEFCMD(STORE, WIDE("Pockets the object in your hand.") ) //STORE)
                          , DEFCMD(PARSE, WIDE("BEGIN Parse file through first level parser.") ) // PARSE )
                          , DEFCMDEX(OPEN, WIDE("Open a data channel device."), PARSE )
                          , DEFCMD(MEMORY, WIDE("Show Memory Statistics.") ) // MEMORY )
                          , DEFCMD(MEMDUMP, WIDE("Dump current allocation table.") ) // MEMDUMP )
                          , DEFCMD(DECLARE, WIDE("Define a variable") ) // DECLARE )
                          , DEFCMDEX(SET, WIDE("Define a variable"), DECLARE )
                          , DEFCMD(COLLAPSE, WIDE("Collapse value of a variable to an atom") ) // COLLAPSE )
                          , DEFCMD(BINARY, WIDE("Define a variable no substitution") ) // BINARY )
                          , DEFCMD(ALLOCATE, WIDE("Allocate a sized binary variable") ) // ALLOCATE )
								  , DEFCMD(VARS, WIDE("List vars in object/macro") ) // VARS )
								  , DEFCMD( VVARS, WIDE("List Volatile Variables") )
                          , DEFCMD(TELL, WIDE("Tell an aware object to perform a command") ) // TELL )
                          , DEFCMD(WAKE, WIDE("Give an object a mind") ) // WAKE )
                          , DEFCMD(KILL, WIDE("Put object to sleep") ) // KILL )
                          , DEFCMD(PROMPT, WIDE("Issue current command prompt") ) // PROMPT )
                          , DEFCMD(STORY, WIDE("Show Introductory story...") ) // STORY )
                          , DEFCMD(SUSPEND, WIDE("Pause object macro processing") ) // SUSPEND )
                          , DEFCMD(RESUME, WIDE("Resume a suspended macro") ) // RESUME )
                          , DEFCMD(DELAY, WIDE("Wait for N milliseconds") ) // DELAY )
                          , DEFCMD(MONITOR, WIDE("Watch an object which is sentient") ) // MONITOR )
                          , DEFCMD(REPLY, WIDE("Reply to object last telling") ) // REPLY )
                          , DEFCMD(RENAME, WIDE("Rename an object") ) // RENAME )
                          , DEFCMD(PLUGIN, WIDE("Load plugin file named....") ) // PLUGIN )
                          , DEFCMD(UNLOAD, WIDE("Unload module which registered name") ) // UNLOAD )
                          , DEFCMD(JOIN, WIDE("Attach self to specified object") ) // JOIN )
                          , DEFCMD(COMMAND, WIDE("open a device as command channel of sentience...") ) // COMMAND )
                          , DEFCMD(INCREMENT, WIDE("Increment a variable by amount") )//, INCREMENT )
                          , DEFCMD(DECREMENT, WIDE("Decrement a variable by amount") )//, DECREMENT )
                          , DEFCMD(MULTIPLY, WIDE("Multiply a variable by amount") )//, MULTIPLY )
                          , DEFCMD(DIVIDE, WIDE("Divide a variable by amount") )//, DIVIDE )
                          , DEFCMD(LALIGN, WIDE("Change variable alignment to left...") )//, LALIGN )
                          , DEFCMD(RALIGN, WIDE("Change variable alignment to right...") )//, RALIGN )
                          , DEFCMD(NOALIGN, WIDE("Change variable alignment to none...") )//, NOALIGN )
                          , DEFCMD(UCASE, WIDE("Upper case a variable") )//, UCASE )
                          , DEFCMD(LCASE, WIDE("Lower case a variable") )//, LCASE )
                          , DEFCMD(PCASE, WIDE("Proper caes a variable (first letter upper)") )//, PCASE )
                          , DEFCMD(BOUND, WIDE("Set a variable's boundry condition (lower, uppwer)") )//, BOUND )
                          , DEFCMD(VERSION, WIDE("Get current program version into a variable(name)") )//, VERSION )
                          , DEFCMD(RELAY, WIDE("Auto relay command input to data output") )//, RELAY )
                          , DEFCMD(FILTER, WIDE("Add/modify filters on datapaths.") )// , FILTER )
                          , DEFCMD(LOOKFOR, WIDE("Finds an object, sets result to entity.") )// , LOOKFOR )
                          , DEFCMD(PRIORITY, WIDE("Set the priority of the dekware process.") )//), PRIORITY )
};

int gbTrace = FALSE;

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
   vtprintf( vt,WIDE("[%.*s]%-*s - %s")
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
      //   continue;
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
				desc = GetRegisteredValue( (CTEXTSTR)_current, WIDE("Description") );
				vtprintf( vt,WIDE("[%.*s]%-*s - %s")
						  , diff
						  , prior_name
						  , 10-diff
						  , prior_name+diff
						  , desc?desc:WIDE("") );
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
		desc = GetRegisteredValue( (CTEXTSTR)tmp, WIDE("Description") );
		vtprintf( vt,WIDE("[%.*s]%-*s - %s")
				  , diff
				  , prior_name
				  , 10-diff
				  , prior_name+diff
				  , desc?desc:WIDE("") );
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
	//S_32 l;
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
		snprintf( tmp, sizeof( tmp ), WIDE("Dekware/commands%s%s/%s")
				  , device?WIDE("/"):WIDE("")
				  , device?device:WIDE("")
				  , name = GetText( (PTEXT)&cmds[i].name ) );
		snprintf( tmp2, sizeof( tmp2 ), WIDE("Dekware/commands%s%s")
				  , device?WIDE("/"):WIDE("")
				  , device?device:WIDE("") );
		if( CheckClassRoot( tmp ) )
		{
			lprintf( WIDE("%s already registered"), tmp );
			continue;
		}
		//lprintf( WIDE("regsiter %s"), tmp );
		SimpleRegisterMethod( tmp2, cmds[i].function
								  , WIDE("int"), name, WIDE("(PSENTIENT,PTEXT)") );
		RegisterValue( tmp, WIDE("Description"), GetText( (PTEXT)&cmds[i].description ) );
		RegisterValue( tmp, WIDE("Command Class"), GetText( (PTEXT)&cmds[i].classname ) );
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
		snprintf( tmp, sizeof( tmp ), WIDE("Dekware/options%s%s/%s")
				  , device?WIDE("/"):WIDE("")
				  , device?device:WIDE("")
				  , name = GetText( (PTEXT)&cmds[i].name ) );
		snprintf( tmp2, sizeof( tmp2 ), WIDE("Dekware/options%s%s")
				  , device?WIDE("/"):WIDE("")
				  , device?device:WIDE("") );
		if( CheckClassRoot( tmp ) )
		{
			lprintf( WIDE("%s already registered"), tmp );
			continue;
		}
		//lprintf( WIDE("regsiter %s"), tmp );
		SimpleRegisterMethod( tmp2, cmds[i].function
								  , WIDE("int"), name, WIDE("(PDATAPATH,PSENTIENT,PTEXT)") );
		RegisterValue( tmp, WIDE("Description"), GetText( (PTEXT)&cmds[i].description ) );
		//RegisterIntValue( tmp, WIDE("option_entry"), (PTRSZVAL)(cmds+i) );
		//RegisterIntValue( tmp, WIDE("significant"), strlen( name ) );
		//RegisterIntValue( tmp, WIDE("max_length"), strlen( name ) );
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
   DECLTEXT( msg, WIDE("was expecting more parameters.") );
   pOut = SegAppend( SegCreateIndirect( (PTEXT)&commands[command].name )
                   , SegCreateIndirect( (PTEXT)&msg ) );
   EnqueLink( Output, pOut );
}

//--------------------------------------------------------------------------

PTEXT Help( PSENTIENT ps, PTEXT pCommand ) /*FOLD00*/
{
   PLINKQUEUE *Output = &ps->Command->Output;
   PENTITY pEnt = ps->Current;
   _16 count;
   PMACRO pm;
   {
      DECLTEXT( leader, WIDE(" --- Builtin Commands ---") );
      EnqueLink( Output, &leader );
   // this command may pause waiting for input...
   // we have a limited number of threads for sentients...
   // if 16 people are in help... well we need to have
   // nActiveSentients... then if it is greater than
   // nAvailSentients... echo that help is temporarily
   // unavailable...
      WriteCommandList2( Output, WIDE("dekware/commands"), pCommand );
   }
   // list macros of current object
   if( !pCommand )
   {
      PVARTEXT vt = VarTextCreate();
		S_MSG( ps, WIDE(" --- Macros ---") );
      LIST_FORALL( pEnt->pMacros, count, PMACRO, pm )
      {
         vtprintf( vt, WIDE("[%s] - %s")
                  , GetText( GetName( pm ) )
                  , GetText( GetDescription( pm ) ) );
         EnqueLink( Output, VarTextGet( vt ) );
      }
      VarTextDestroy( &vt );
   }
//   if( !pCommand )
   {
		//S_MSG( ps, WIDE(" --- Plugins ---") );
      //PrintRegisteredRoutines( Output, ps, pCommand );
	}
   if( !pCommand )
   {
      S_MSG( ps, WIDE(" --- Devices ---") );
      PrintRegisteredDevices( Output );
   }
   if( !pCommand )
   {
      DECLTEXT( PluginSep, WIDE(" - parameter to help will show only matching commands") );
      EnqueLink( Output, &PluginSep );
   }
   return NULL;
}

//--------------------------------------------------------------------------

PTEXT Methods( PSENTIENT ps ) /*FOLD00*/
{
	PLINKQUEUE *Output = &ps->Command->Output;
	//PENTITY pEnt = ps->Current;
	_16 count;
	int bMethod = FALSE;
	{
		PVARTEXT vt = VarTextCreate();
		PENTITY pe = ps->Current;
		DECLTEXT( msg, WIDE(" --- Object Methods ---") );
		PCLASSROOT pme;
		//command_entry *pme;

		LIST_FORALL( pe->pMethods, count, PCLASSROOT, pme )
		{
			CTEXTSTR name;
			CTEXTSTR desc;
			PCLASSROOT tmp;
			PCLASSROOT root = GetClassRootEx( pme, WIDE("methods") );
			for( name = GetFirstRegisteredName( (CTEXTSTR)root, &tmp );
				 name;
				  name = GetNextRegisteredName( &tmp ) )
			{
				if( !bMethod )
				{
					EnqueLink( Output, &msg );
					bMethod = TRUE;
				}
				desc = GetRegisteredValue( (CTEXTSTR)(GetCurrentRegisteredTree(&tmp)), WIDE("Description") );
				vtprintf( vt, WIDE("[%s] - %s")
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

TEXTCHAR Months[13][10] = { WIDE("")
                  , WIDE("January")
                  , WIDE("February")
                  , WIDE("March")
                  , WIDE("April")
                  , WIDE("May")
                  , WIDE("June")
                  , WIDE("July")
                  , WIDE("August")
                  , WIDE("September")
                  , WIDE("October")
                  , WIDE("November")
                  , WIDE("December") };
TEXTCHAR Days[7][10] = {WIDE("Sunday"), WIDE("Monday"), WIDE("Tuesday"), WIDE("Wednesday")
               , WIDE("Thursday"), WIDE("Friday"), WIDE("Saturday") };

DECLTEXT( timenow, WIDE("00/00/0000 00:00:00                  ") );

PTEXT GetTime( void ) /*FOLD00*/
{
//   PTEXT pTime;
#ifdef WIN32
   SYSTEMTIME st;
//   pTime = SegCreate( 38 );
   GetLocalTime( &st );
   /*
   n = sprintf( pTime->data.data, WIDE("%s, %s %d, %d, %02d:%02d:%02d"),
                     Days[st.wDayOfWeek], Months[st.wMonth],
                     st.wDay, st.wYear
                     , st.wHour, st.wMinute, st.wSecond );
   */
#if defined( _MSC_VER ) && defined( __cplusplus_cli )
#define snprintf _snprintf
#endif
   timenow.data.size = snprintf( timenow.data.data, sizeof( timenow.data.data ), WIDE("%02d/%02d/%d %02d:%02d:%02d"),
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
            , WIDE("%m/%d/%Y %H:%M:%S")
            , timething );
   return (PTEXT)&timenow;
#endif
}

//------------------------------------------------------------------------

PTEXT GetShortTime( void ) /*FOLD00*/
{
//   PTEXT pTime;
#ifdef WIN32
   SYSTEMTIME st;
   int n;
//   pTime = SegCreate( 38 );
   GetLocalTime( &st );
   /*
   n = sprintf( pTime->data.data, WIDE("%s, %s %d, %d, %02d:%02d:%02d"),
                     Days[st.wDayOfWeek], Months[st.wMonth],
                     st.wDay, st.wYear
                     , st.wHour, st.wMinute, st.wSecond );
   */

   n = snprintf( timenow.data.data, sizeof( timenow.data.data ), WIDE("%02d:%02d:%02d"),
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
            , WIDE("%H:%M:%S")
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
    vtprintf( vt, WIDE("%d"), val );
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

static PTEXT ObjectVolatileVariableGet( WIDE("core object"), WIDE("me"), WIDE("my name") )( PENTITY pe, PTEXT *lastvalue )
{
	return GetName( pe );
}

static PTEXT ObjectVolatileVariableGet( WIDE("core object"), WIDE("room"), WIDE("my room's name") )( PENTITY pe, PTEXT *lastvalue )
{
	*lastvalue = TextDuplicate( GetName( FindContainer( pe ) ), FALSE );
	return *lastvalue;
}

static PTEXT ObjectVolatileVariableGet( WIDE("core object"), WIDE("now"), WIDE("the current time") )( PENTITY pe, PTEXT *lastvalue )
{
	*lastvalue = GetShortTime();
	return *lastvalue;
}

static PTEXT ObjectVolatileVariableGet( WIDE("core object"), WIDE("time"), WIDE("the current time") )( PENTITY pe, PTEXT *lastvalue )
{
	*lastvalue = GetTime();
	return *lastvalue;
}

static PTEXT ObjectVolatileVariableGet( WIDE("core object"), WIDE("blank"), WIDE("a space character(segment)") )( PENTITY pe, PTEXT *lastvalue )
{
	DECLTEXT( blank, WIDE(" "));
	if( !*lastvalue )
		*lastvalue = (PTEXT)&blank;
	return *lastvalue;
}

static PTEXT ObjectVolatileVariableGet( WIDE("core object"), WIDE("EOL"), WIDE("end of line character(segment)") )( PENTITY pe, PTEXT *lastvalue )
{
	DECLTEXT( eol, WIDE(""));
	if( !*lastvalue )
		*lastvalue = (PTEXT)&eol;
	return *lastvalue;
}

static PTEXT ObjectVolatileVariableGet( WIDE("core object"), WIDE("cmdline"), WIDE("end of line character(segment)") )( PENTITY pe, PTEXT *lastvalue )
{
	extern PTEXT global_command_line;
	if( !*lastvalue )
	{
		*lastvalue = SegCreateIndirect( global_command_line );
	}
	return *lastvalue;
}

static PTEXT ObjectVolatileVariableGet( WIDE("core object"), WIDE("program_path"), WIDE("the path to the .exe that ran this") )( PENTITY pe, PTEXT *lastvalue )
{
	extern CTEXTSTR load_path;
	if( !*lastvalue )
		*lastvalue = SegCreateFromText( load_path );
	return *lastvalue;
}

static PTEXT ObjectVolatileVariableGet( WIDE("core object"), WIDE("core_path"), WIDE("the path to dekware.core") )( PENTITY pe, PTEXT *lastvalue )
{
	extern CTEXTSTR core_load_path;
	if( !*lastvalue )
		*lastvalue = SegCreateFromText( core_load_path );
	return *lastvalue;
}

static PTEXT ObjectVolatileVariableGet( WIDE("core object"), WIDE("caller"), WIDE("end of line character(segment)") )( PENTITY pe, PTEXT *lastvalue )
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

static PTEXT ObjectVolatileVariableGet( WIDE("core object"), WIDE("actor"), WIDE("end of line character(segment)") )( PENTITY pe, PTEXT *lastvalue )
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

//------------------------------------------------------------------------
#define STEP_TOKEN() do { pPrior = pReturn; \
   pReturn = *token;  \
   ptext = GetText( pReturn ); textlen = GetTextSize( pReturn );  \
   *token = NEXTLINE( *token ); } while(0)

#define RESET_TOKEN() do { pPrior = pReturn; \
   pReturn = *token;    \
   ptext = GetText( pReturn ); textlen = GetTextSize( pReturn );  \
   } while(0)

#define BEGIN_STEP_TOKEN() {  pReturn = *token;  \
   pPrior = NULL;           \
   ptext = GetText( pReturn ); textlen = GetTextSize( pReturn );     \
   *token = NEXTLINE( *token ); }

CORE_PROC( PTEXT, SubstTokenEx )( PSENTIENT ps, PTEXT *token, int IsVar, int IsLen, PENTITY pe ) /*FOLD00*/
{
	PTEXT c;
	_32 n;
	PMACROSTATE pms;
	PENTITY pEnt;
	TEXTCHAR *ptext;
	size_t textlen;
	PTEXT pReturn;
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
	}
	else
	{
		pEnt = pe;
		pms = NULL; // no known macro state?
	}

   BEGIN_STEP_TOKEN();

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
   }
   // at this point - we have established that the next thing
   // may be a variable name to locate
   //   variable in local space, macro param space, and object var space
   // may also be a combination obj.var or (obj)var

	// parenthesis has the advantage of being able to subsittute
	// chained commands (apple)(core)(seed)blah
   if( GetTextSize( pReturn ) > 1 )
	{
		// check for dots in a single token...
		TEXTCHAR *dot;
		TEXTCHAR *start;
		PTEXT pTmpReturn = pReturn;
		while( pTmpReturn && ( dot = strchr( (start = GetText( pTmpReturn )), '.' ) ) )
		{
			if( dot == ptext )
			{
				// check for elipses
				if( dot[1] != '.' && dot[1] )
					pTmpReturn = NEXTLINE( SegSplit( &pTmpReturn, 1 ) );
				else
					break;
			}
			else if( pReturn->flags & TF_STATIC )
			{
				pTmpReturn = TextDuplicate( pReturn, FALSE );
				// swaps it out of the segment, text needs to be udpated.
				LineRelease( SegSubst( pReturn, pTmpReturn ) );
				pReturn = pTmpReturn;
				ptext = GetText( pReturn );
			}
			else
			{
				PTEXT tmp = pTmpReturn;
				int bFixToken;
				SegSplit( &pTmpReturn, dot - start );
				if( tmp == pReturn )
				{
					pReturn = pTmpReturn;
					ptext = GetText( pReturn );
					bFixToken = 1;
				}
				else
					bFixToken = 0;
				pTmpReturn = NEXTLINE( pTmpReturn );
				pTmpReturn = NEXTLINE( SegSplit( &pTmpReturn, 1 ) );
				if( bFixToken )
					*token = NEXTLINE( pReturn );

			}
		}
	}

	{

		int bLoop/* = 0*/;
		PTEXT pNext/* = NEXTLINE( *token )*/;
		PTEXT pAfterNext/* = NEXTLINE( pNext )*/;
		//int bDot/* = ( GetTextSize( pNext ) == 1 ) && GetText( pNext ) == '.'*/;
		//if( bDot )
		//{
		//	if( !HAS_WHITESPACE( pAfterNext ) )
      //      bLoop = 1;
		//}
#define UGLY_LOOP_TEST_FOR_PERIOD_SEPARATOR() 		(bLoop=((pNext = NEXTLINE( *token )),(pAfterNext = NEXTLINE( pNext )), \
		((( GetTextSize( *token ) == 1 ) && (GetText( *token )[0] == '.'))                                           \
		?(!HAS_WHITESPACE(pNext)?(1):(0)):(0))))

		while( UGLY_LOOP_TEST_FOR_PERIOD_SEPARATOR() || ( ptext[0] == '(' ) )
		{
			size_t count;
			S_64 long_count = 1;
			// resolve an object's name...
			PTEXT pObjName;
			lprintf( WIDE("Token : %s text %s"), GetText( *token ), ptext );
			if( !bLoop )
			{
				//STEP_TOKEN();
            // if within parens, substitute that expression for object name
				pObjName = SubstToken( ps, token, FALSE, FALSE );
			}
			else
			{
				pObjName = pReturn;
			}
			// make sure we only take int numbers
			if( IsIntNumber( *token, &long_count ) )
			{
				RESET_TOKEN(); // isint number updates token to be after the number (if is number)

				// next thing needs to be a '.' and the thing after is the object
				// this is the count...
				//count = IntNumber( pNumber );
				//STEP_TOKEN();
				if( ptext[0] != '.' )
				{
               S_MSG( ps, WIDE("Dot not found after object count reference...") );
				}
				else           	
				{                 	
					STEP_TOKEN();
					pObjName = pReturn;
				}
				lprintf( WIDE("Object name after count of %d is %s"), count, ptext );
			}
			lprintf( WIDE("Text is %s token %s"), ptext, GetText( *token ) );
			if( ptext[0] == '[' )
			{
				PTEXT _token = *token;
				PTEXT pCount = SubstToken( ps, token, FALSE, FALSE );
				S_64 iNumber;
				if( !IsIntNumber( pCount, &iNumber ) )
				{
					S_MSG( ps, WIDE("Array reference is not a number: %s in %s"), GetText( pCount ), GetText( pObjName ) );
					*token = _token;
				}
				else
				{
					//BEGIN_STEP_TOKEN();
				}
				{
					if( ptext[0] != ']' )
					{
						S_MSG( ps, WIDE("Count reference is nto closed with a ']' instead it is: '%s'"), ptext );
					}
					else
						STEP_TOKEN();
				}
			}
			lprintf( WIDE("Looking for object %s"), GetText( pObjName ) );
			if( !pObjName )
			{
				lprintf( WIDE("Badly formed variable reference, open paren, no close ...") );
				return NULL;
			}
			{
				// clean code using %(%me) to work correctly
				// %me should be the name reference of the current
				// object.
				// %me.x would not work?
				// %(%me)x would...
				// %deck.hand.1.show
				// /deck.1.hand.show
				// ---- double diminsioned arrays...
				// /deck.1.2.3.hand.show
				//   each thing contains a number of other things
				//

				if( GetName( ps->Current ) == pObjName )
					pEnt = ps->Current;
				else
				{
					int findtype;
					// hmm I have a feeling that I'll need to lock this after I
					// find it - cause it might go away while I'm referencing it :(
					count = long_count;
					pEnt = (PENTITY)DoFindThing( pEnt, FIND_VISIBLE, &findtype, &count, GetText( pObjName ) );
				}
				if( !pEnt )
				{
					S_MSG( ps, WIDE("Cannot see entity %s to get varible from..."), GetText( pObjName ) );
					return NULL;
				}
			}
			// step over the name we just referenced, (which sets the current
			// actively referenced entity)
			STEP_TOKEN(); // and step again to get the var name after...
			// get the current token, presumably a ')' or '.', and set return past it...
			// if we entered because of bLoop being set - it was a period following
			// not a parenthesis
			if( !bLoop && ptext[0] != ')' )
			{
				S_MSG( ps, WIDE("improper parenthation... ") );
				return NULL;
			}
			// step over the closing paren... or onto next word after .
			STEP_TOKEN();
		}
   }
	{ /*FOLD00*/
      {
         PTEXT pText = GetVolatileVariable( pEnt, ptext );
         if( pText )
         {
            if( IsVarLen ) 
               MakeTempNumber( LineLength( pText ) );
            return pText;
         }
      }
      if( !pms )
      {
check_global_vars:
         if( ( c = GetListVariable( pEnt->pVars, ptext ) ) )
			{
            //lprintf( WIDE("Found global var %s"), GetText( c ) );
            if( IsVarLen )
               return MakeTempNumber( LineLength( c ) );
            else
               return c;
         }
			// otherwise - unknown location - or unkonw variable..
		if( !ps->CurrentMacro )
		{
			S_MSG( ps, WIDE("Parameter named %s was not found.")
					  , GetText(pReturn) );
		}
         if( IsVarLen )
            if( pReturn->flags & TF_INDIRECT )
               return MakeTempNumber( LineLength( GetIndirect( pReturn ) ) );
            else
               return MakeTempNumber( GetTextSize( pReturn ) );
         else
            return pReturn;
      }
      else
      {
         // if skipped a % retain all further...
         // also cannot perform substition....
         if( !strcmp( ptext, WIDE("...") ) ) // may test for 'elispes'?
         {
            S_32 i;
            //lprintf( WIDE("Gathering trailing macro args into one line...")  );
            if( pms->pMacro->nArgs < 0 )
            {
               //lprintf( WIDE("Skipping some arguments to get to ... ") );
               i = -pms->pMacro->nArgs;
               pReturn = pms->pArgs;
               while( i && --i && pReturn )
               {
                  //lprintf( WIDE("Skipped %s"), GetText( pReturn ) );
                  pReturn = NEXTLINE( pReturn );
               }
            }
            else
               lprintf( WIDE("Macro has positive argument count - there is no ... param") );
            //lprintf( WIDE("Okay at this point pReturn should be start of macro extra parms") );
            if( !(pReturn->flags & TF_INDIRECT) )
            {
               PTEXT pWrapper = SegCreateIndirect( pReturn );
               lprintf( WIDE("Wrapping pReturn in a single seg wrapper") );
               SegAppend( SegBreak( pReturn ), pWrapper );
               pWrapper->flags |= TF_DEEP;
               pReturn = pWrapper;
            }
            //else // normal condition.
            //   lprintf( WIDE("Result is already in a DEEP indirect? ") );
            return pReturn;
         }

         {
            // parameter substition for macros...
            // wow even supports ancient syntax of %1 %2 %3 ....
            n = atoi( ptext );
            if( !n )
            {
               LIST_FORALL( pms->pVars, n, PTEXT, c )
               {
                  if( TextIs( c, ptext ) )
                  {
                     if( IsVarLen )
                        return MakeTempNumber( LineLength( NEXTLINE(c) ) );
                     else
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
                  if( TextIs( c, ptext ) )
                     break;
                  n++;
                  c = NEXTLINE(c);
               }
               if( !c ) // didn't find the variable local, or as param...
                  goto check_global_vars;
            }
            else
               n--;

            c = pms->pArgs; // current args to this macro.

            while( n && c )
            {
               c = NEXTLINE(c);
               n--;
            }

            pReturn = c;

            if( !c ) // ran out of passed arguments to this variable...
            {
               DECLTEXT( msg, WIDE("Macro needs argument: ") );
               PTEXT pMsg;
               pMsg = SegAppend( SegCreateIndirect( (PTEXT)&msg ), SegCreateFromText( ptext ) );
               EnqueLink( &ps->Command->Output, pMsg );
               return NULL;
            }
         }
      }
   }

   return pReturn;
}

//--------------------------------------------------------------------------
CORE_PROC( PTEXT, GetEntityParam )( PENTITY pe, PTEXT *from ) /*FOLD00*/
{
   if( !from || !*from) return NULL;

   // skip end of lines to getparam....
   while( from &&
          *from &&
          !( (*(int*)&(*from)->flags ) & IS_DATA_FLAGS ) &&
          !(*from)->data.size ) // blah...
      *from = NEXTLINE( *from );
   return SubstTokenEx( NULL, from, FALSE, FALSE, pe );
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

S_32 CountArguments( PSENTIENT ps, PTEXT args ) /*FOLD00*/
{
   S_32 idx;
   PTEXT pSubst;
	idx = 0;
   while( ( pSubst = SubstToken( ps, &args, FALSE, FALSE ) ) )
   {
      if( pSubst->flags & TF_INDIRECT )
      {
			//xlprintf(LOG_NOISE+1)( WIDE("Found indirect arguemtn count it from %d"), idx );
         idx += CountArguments( ps, GetIndirect( pSubst ) );
			//xlprintf(LOG_NOISE+1)( WIDE("Found indirect arguemtn count it to %d"), idx );
      }
      else
      {
         idx++;
         if( TextIs( pSubst, WIDE("...") ) )
            return -idx;
      }
      if( pSubst->flags & TF_TEMP )
         LineRelease( pSubst );
   }
   //xlprintf(LOG_NOISE+1)( WIDE("total args is %d"), idx );
   return (S_32)idx;
}

//--------------------------------------------------------------------------

void DestroyMacro( PENTITY pe, PMACRO pm ) /*FOLD00*/
{
	PTEXT temp;
	PTRSZVAL idx;
	if( !pm )
		return;
	// if the macro is in use (running) just mark that we wish to delete.
	if( pm->flags.un.macro.bUsed )
	{
		xlprintf(LOG_NOISE)( WIDE("Macro is in use - mark delete, don't do it now.") );
		pm->flags.un.macro.bDelete = TRUE;
		return;
	}
	xlprintf(LOG_NOISE+1)( WIDE("Destroying macro %s"), GetText( pm->pName ) );
	if( --pm->Used )
	{
		xlprintf(LOG_NOISE+1)(WIDE(" Macro usage count is now %d"), pm->Used );
		return;
	}
	xlprintf(LOG_NOISE+1)(WIDE(" Macro usage count is now %d"), pm->Used );

	// find the macro within this entity
	if( pe )
	{
		idx = (PTRSZVAL)DoFindThing( pe, FIND_MACRO_INDEX, NULL, NULL, GetText( pm->pName ) );
		if( idx != INVALID_INDEX )
      		SetLink( &pe->pMacros, idx, NULL );
		// it might be a behavior macro...
   }
   else
   {
      // can't use pe if it's not set...
      //DECLTEXT( msg, WIDE("Could not locate the macro specified") );
      //EnqueLink( &pe->pControlledBy->Command->Output, &msg );
   }
   xlprintf(LOG_NOISE+1)( WIDE("Destroying macro %s"), GetText( pm->pName ) );
   LineRelease( pm->pArgs );
   LineRelease( pm->pName );
   LineRelease( pm->pDescription );
   LIST_FORALL( pm->pCommands, idx, PTEXT, temp )
	{
		//{
		//   PTEXT x = BuildLine( temp );
		//	lprintf( WIDE("DestroyMacro: %s"), GetText( x ) );
		//   LineRelease( x );
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
         lprintf( WIDE("Ran out of arguments to match, still have text:%s"), GetText( pText ) );
      }
      if( pText->flags & TF_BINARY )
      {
         pText = NEXTLINE( pText );
         continue;
      }
      if( pArgs && strcmp( GetText( pArgs ), WIDE("...") ) == 0 )
      {
         //lprintf( WIDE("Argname is ... therefore grab rest of line and be done.") );
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
   //   PTEXT x = BuildLine( pNew );
	//	_lprintf( DBG_AVAILABLE, WIDE("Macroduplicate result:%p %s") DBG_RELAY, pNew, GetText( x ) );
   //   LineRelease( x );
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
            SetIndirect( pNew, (PTRSZVAL)DeeplyBurst( pBurst ) );
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
               SetIndirect( pNext, (PTRSZVAL)NULL );
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
      //lprintf( WIDE("GetFileName = %s"), GetText( result ) );
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
//--------------------------------------------------------------------------
int UNIMPLEMENTED( PSENTIENT ps, PTEXT parameters ) /*FOLD00*/
{
    PTEXT temp;
    PVARTEXT vt;
    vt = VarTextCreate();
    vtprintf( vt, WIDE("Command \"%s\" is unimplemented."),
             GetText(PRIORLINE(parameters) ) );
    EnqueLink( &ps->Command->Output, VarTextGet( vt ) );
    while( ( temp = GetParam( ps, &parameters ) ) )
    {
        vtprintf( vt, WIDE("    Parameter: \"%s\""), GetText(temp) );
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

CORE_PROC( PMACRO, GetMacro )( PENTITY pe, TEXTCHAR *pNamed ) /*FOLD00*/
{
   PMACRO match;
   _16 idx;
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
	//lprintf( WIDE("f is %p  (%p or %p?)"), function, CMD_ENDMACRO, CMD_MACRO );
	if( function == CMD_ENDMACRO ||
        function == CMD_MACRO ) // always can execute end...
		return TRUE;

	if( ps->pRecord )
	{
		//lprintf( WIDE("Recording... no process.") );
		return FALSE;
	}

	if( !ps->CurrentMacro ) // then always process...
	{
		//lprintf( WIDE("not running in a macro, process.") );
		return TRUE;
	}

	// label search pre-empts all others...
	if( ps->CurrentMacro->state.flags.bFindLabel )
	{
		if( !function )
		{
			lprintf( WIDE("not a command, skip.") );
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
                      , SegCreateFromText( WIDE(" Processing:") ) );
   SegAppend( pLeader, SegCreateIndirect( pCommand ) );
   pOut = BuildLine( pLeader );
   //Log1( WIDE("%s"), GetText( pOut ) );
   EnqueLink( ppOutput,  pOut );
   LineRelease( pLeader );
}

//--------------------------------------------------------------------------

void EnqueBareCommandProcess( PTEXT pName, PLINKQUEUE *ppOutput, PTEXT pCommand ) /*FOLD00*/
{
   PVARTEXT vt;
   PTEXT pOut, pLeader;
   vt = VarTextCreate();
   vtprintf( vt, WIDE("%s Command(%p):"), GetText( pName ), pCommand );
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
		vtprintf( pvt, WIDE("%s Recording(unknown) :")
				   );

	}
	else
		vtprintf( pvt, WIDE("%s Recording(%s) :")
				  , GetText( pName )
				  , GetText( GetName( ps->pRecord ) ) );
	pLeader = VarTextGet( pvt );
	VarTextDestroy( &pvt );
	SegAppend( pLeader, SegCreateIndirect( pCommand ) );
	pOut = BuildLine( pLeader );
	Log1( WIDE("%s"), GetText( pOut ) );
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
		//_lprintf( 0, WIDE("Adding a macro command to %s(%d)=%p")
		//		  , GetText( GetName( pMacro ) )
		//		  , (pMacro)->nCommands, Params );
		pMacro->nCommands++;
		AddLink( &pMacro->pCommands, Params );
	}
}
//--------------------------------------------------------------------------

CORE_PROC( PMACRO, LocateMacro )( PENTITY pe, TEXTCHAR *name ) /*FOLD00*/
{
   //TEXTCHAR *data;
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

PMACROSTATE InvokeMacroEx( PSENTIENT ps, PMACRO pMacro, PTEXT pArgs, void (CPROC*StopEvent)(PTRSZVAL psvUser, PMACROSTATE pms ), PTRSZVAL psv ) /*FOLD00*/
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
   S_32 idx = 0;
   TEXTCHAR *data;
   size_t sig;
   _32 slash_count, syscommand;
   PTEXT Command = *RealCommand;
   PTEXT EndLine = NULL;

   if( !Command )
      return 0;
   if( !ps->Command )
      return 0; // command queue is no longer valid...

	if( global.flags.bLogAllCommands )
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
					//Log( WIDE("Sending blank line?!") );
					return SendLiteral( ps, RealCommand, Command, EndLine );
				}
			}
			if( EndLine && pNext )
			{
				Log( WIDE("Command input contained EOL within the line... FAULT") );
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
		DECLTEXT( msg, WIDE("null command told to me...") );
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
		//Log( WIDE("Consider send to datapath") );
		if( CanProcess( ps, NULL ) )
		{
			if( GetTextSize( Command ) == 1 )
			{
				//Log( WIDE("Step to next token...") );
				Command = NEXTLINE( Command );// skip period....
			}
			else
			{
				PTEXT period;
				//Log( WIDE("Do split on line...") );
				period = SegAppend( SegCreateFromText( WIDE(".") )
                                , SegCreateFromText( data+1 ) );
				period->format.position.offset.spaces = Command->format.position.offset.spaces;
				period->flags = Command->flags & (IS_DATA_FLAGS);
				LineRelease( SegSubst( Command, period ) );
				*RealCommand = period;
				Command = NEXTLINE( period );
			}
			//Log( WIDE("Send to datapath") );
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
		DECLTEXT( dev, WIDE("system") );
		DECLTEXT( devname, WIDE("system") );
		DECLTEXT( dev2, WIDE("ansi") );
		DECLTEXT( dev2opt, WIDE("inbound newline") );
		DECLTEXT( dev2name, WIDE("system parse") );
		//DECLTEXT( dev3, WIDE("splice") );
		//DECLTEXT( dev3name, WIDE("system splice") );
		//DECLTEXT( dev3opt, WIDE("inbound") );
		//DECLTEXT( devdbg, WIDE("binary") );
		//DECLTEXT( devdbgname, WIDE("Binary Logger") );
		if( data[1] )
		{
			PTEXT exclam;
			//Log( WIDE("Do split on line...") );
	         exclam = SegAppend( SegCreateFromText( WIDE("!") )
							, SegCreateFromText( data+1 ) );
			exclam->format.position.offset.spaces = Command->format.position.offset.spaces;
			exclam->flags = Command->flags & (IS_DATA_FLAGS);
			LineRelease( SegSubst( Command, exclam ) );
			*RealCommand = exclam;
			Command = NEXTLINE( exclam );
		}
		evalcommand = MacroDuplicateEx( ps, Command, FALSE, TRUE );
		if( ( pdp = OpenDevice( &ps->Data, ps, (PTEXT)&dev, evalcommand ) ) )
		{
			//PDATAPATH pdp2a;
			PDATAPATH pdp2;
			PTEXT tmp;
			pdp->pName = (PTEXT)&devname;
			//pdp2 = OpenDevice( &ps->Data, ps, (PTEXT)&devdbg, tmp = SegAppend( SegCreateFromText( WIDE("outbound") ), SegCreateFromText( WIDE("log") ) ) );
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
			//pdp2 = OpenDevice( &ps->Data, ps, (PTEXT)&devdbg, tmp = SegAppend( SegCreateFromText( WIDE("inbound") ), SegCreateFromText( WIDE("log") ) ) );
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
									 , PTEXT pWhat   // command parameter string...
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
						lprintf( WIDE("Executing command on entity without awareness using my own sentience.") );
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
				//lprintf( WIDE("Executed by registered command...") );
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
				PCLASSROOT root = GetClassRootEx( pce, WIDE("methods") );
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
						vtprintf( vt, WIDE("Macro requires: ") );
						t = match->pArgs;
						while( t )
						{
							vtprintf( vt, WIDE("%s"), GetText( t ) );
							t = NEXTLINE( t );
							if( t )
								vtprintf( vt, WIDE(", ") );
						}
						vtprintf( vt, WIDE(".") );
						EnqueLink( &ps->Command->Output, VarTextGet( vt ) );
						VarTextDestroy( &vt );
						return FALSE; // return...
					}	
					if( ( i > match->nArgs ) || ( -i > match->nArgs ) )
					{
						DECLTEXT( msg, WIDE("Extra parameters passed to macro. Continuing but ignoring extra.") );
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
			//     - by internal methods
			//     - within the procreg tree
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
					vtprintf( pvt, WIDE("dekware/objects/%s/macro/create"), extension_name );
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
      DECLTEXT( msg, WIDE("Macro name overlaps internal command.") );
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
		lprintf(WIDE(" Macro %s usage count is now %d"), GetText( pm->pName ), pm->Used );
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

CORE_PROC( void, QueueCommand )( PSENTIENT ps, TEXTCHAR *Command )
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
   DECLTEXT( nulprompt, WIDE("") );
   PTEXT text;
   PTEXT pPrompt;
   if( ps->flags.no_prompt )
      return;
   if( gbTrace )
   {
      //DECLTEXT( msg, WIDE("Issuing prompt...") );
      //EnqueLink( &ps->Command->Output, (PTEXT)&msg );
   }
   pPrompt = GetListVariable( ps->Current->pVars, WIDE("prompt") );
   pPrompt = GetIndirect( pPrompt );
   text = (PTEXT)&nulprompt;
   if( ps->pRecord && pPrompt )
   {
      DECLTEXT( colon, WIDE(":") );
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
      DECLTEXT( colon, WIDE(":") );
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

static int HandleCommand( WIDE("debug"),WIDE("dumpnames"), WIDE("Call DumpRegisteredNames()") )( PSENTIENT ps, PTEXT params )
{
	DumpRegisteredNames();
	return 0;
}


