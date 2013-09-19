#define DEFINES_DEKWARE_INTERFACE

#include <stdhdrs.h>
#include "plugin.h"

#include <controls.h>

#define MAIN_SOURCE
#include "global.h"

FunctionProto AddControl
				  , Result
				  , DoHideCommon
				  , DoShowCommon
				  , DoEditCommon
				  , DoSaveCommon
	;

/*
command_entry dialog_commands[] = { { DEFTEXT( WIDE("display") ), 4, 7, DEFTEXT( WIDE("Show the frame") ), DoShowCommon }
											 , { DEFTEXT( WIDE("hide") ), 4, 4, DEFTEXT( WIDE("Hide the frame") ), DoHideCommon }
											 , { DEFTEXT( WIDE("edit") ), 4, 4, DEFTEXT( WIDE("Edit the frame") ), DoEditCommon }
											 , { DEFTEXT( WIDE("save") ), 4, 4, DEFTEXT( WIDE("save the frame as a file") ), DoSaveCommon }
};
*/

static int ObjectMethod( WIDE("psi_control"), WIDE("save"), WIDE("Save Control (frame)") )
//int DoSaveCommon
( PSENTIENT ps, PTEXT params )
{
	PTEXT filename = GetFileName( ps, &params );
	if( filename )
	{
		PCOMMON_TRACKER pct = (PCOMMON_TRACKER)GetLink( &ps->Current->pPlugin, g.iCommon );
      PCOMMON pc = pct->control.pc;
		if( pc )
		{
			if( SaveXMLFrame( pc, GetText( filename ) ) )
			{
				if( ps->CurrentMacro )
               ps->CurrentMacro->state.flags.bSuccess = 1;
			}
			else
				if( !ps->CurrentMacro )
				{
               S_MSG( ps, WIDE("Failed to save control.") );
				}
		}
	}
	else
	{
		if( !ps->CurrentMacro )
		{
         S_MSG( ps, WIDE("Must supply a filename to write control to.") );
		}
	}
   return 0;
}
//--------------------------------------------------------------------------

static int ObjectMethod( WIDE("psi_control"), WIDE("edit"), WIDE("Edit Control (frame)") )
//int DoEditCommon
( PSENTIENT ps, PTEXT params )
{
	PCOMMON_TRACKER pct = (PCOMMON_TRACKER)GetLink( &ps->Current->pPlugin, g.iCommon );
	PCOMMON pc = pct->control.pc;
	if( pc )
	{
		TEXTCHAR *option = GetText( GetParam( ps, &params ) );
		if( !option )
			option = WIDE("0");
		EditFrame( GetFrame( pc ), atoi( option ) );
	}
   return 1;
}

//--------------------------------------------------------------------------
static int ObjectMethod( WIDE("psi_control"), WIDE("hide"), WIDE("Hide Control (frame)") )
//int DoHideCommon
	( PSENTIENT ps, PTEXT params )
{
	PCOMMON_TRACKER pct = (PCOMMON_TRACKER)GetLink( &ps->Current->pPlugin, g.iCommon );
	PCOMMON pc = pct->control.pc;
	if( pc )
	{
      HideCommon( pc );
	}
   return 1;
}

//--------------------------------------------------------------------------

static int ObjectMethod( WIDE("psi_control"), WIDE("show"), WIDE("Show Control (frame)") )
//int DoShowCommon
( PSENTIENT ps, PTEXT params )
{
	PCOMMON_TRACKER pct = (PCOMMON_TRACKER)GetLink( &ps->Current->pPlugin, g.iCommon );
	PCOMMON pc = pct->control.pc;
	if( pc )
	{
      DisplayFrame( pc );
	}
   return 1;
}

//--------------------------------------------------------------------------

void DestroyAControl( PENTITY pe )
{
	//PCOMMON pc = GetLink( &pe->pPlugin, g.iCommon );
	PCOMMON_TRACKER pComTrack = (PCOMMON_TRACKER)GetLink( &pe->pPlugin, g.iCommon );
	if( pComTrack )
	{
      DeleteLink( &g.pMyFrames, pe );
		SetLink( &pe->pPlugin, g.iCommon, NULL );
		if( pComTrack->flags.created_internally )
		{
			if( !pComTrack->flags.menu )
				DestroyCommon( &pComTrack->control.pc );
			else
				DestroyPopup( pComTrack->control.menu );
		}
		Release( pComTrack );
	}
}

//--------------------------------------------------------------------------

static int CPROC CreatePopupThing( PSENTIENT ps, PENTITY peNew, PTEXT params )
{
	PMENU menu = CreatePopup();
	PCOMMON_TRACKER pComTrack = New( COMMON_TRACKER );
	pComTrack->control.menu = menu;
	pComTrack->flags.created_internally = 1;
   pComTrack->flags.menu = 1;
	SetLink( &ps->Current->pPlugin, g.iCommon, (POINTER)pComTrack );
	// add some methods to add menu items, and to
	// issue the popup dialog....
	// also these popup objects need to be able to be given
   // to things like listboxes for contect menus on items.
	UnlockAwareness( CreateAwareness( peNew ) );

   return 0;
}

//--------------------------------------------------------------------------

int IsOneOfMyFrames( PENTITY pe )
{
	PENTITY peCheck;
	INDEX idx;
	LIST_FORALL( g.pMyFrames, idx, PENTITY, peCheck )
	{
		if( pe == peCheck )
         return TRUE;
	}
   return FALSE;
}

static PTEXT CPROC SetCaption( PTRSZVAL psv
							  , struct entity_tag *pe
							  , PTEXT newvalue )
{
	{
		PCOMMON_TRACKER pct = (PCOMMON_TRACKER)psv;//GetLink( &pe->pPlugin, g.iCommon );
      PCOMMON pc = pct->control.pc;
		if( pc )
		{
			PTEXT line = BuildLine( newvalue );
			SetCommonText( pc, GetText( line ) );
         LineRelease( line );
		}
	}
   return NULL;
}

static void DumpMacro( PVARTEXT vt, PMACRO pm, TEXTCHAR *type )
{
	PTEXT pt;
   INDEX idx;
	vtprintf( vt, WIDE("/%s \"%s\""), type, GetText( GetName(pm) ) );
	{
		PTEXT param;
		param = pm->pArgs;
		while( param )
		{
			vtprintf( vt, WIDE(" %s"), GetText( param ) );
			param = NEXTLINE( param );
		}
	}
   vtprintf( vt, WIDE(";") );
	LIST_FORALL( pm->pCommands, idx, PTEXT, pt )
	{
		PTEXT x;
		x = BuildLine( pt );
		vtprintf( vt, WIDE("%s;"), GetText( x ) );
		LineRelease( x );
	}
}

//--------------------------------------------------------------------------

static void DumpMacros( PVARTEXT vt, PENTITY pe )
{
	INDEX idx;
	PMACRO pm;
	PMACRO behavior;
	LIST_FORALL( pe->pGlobalBehaviors, idx, PMACRO, behavior )
	{
		DumpMacro( vt, behavior, WIDE("on") );
	}
	LIST_FORALL( pe->pBehaviors, idx, PMACRO, behavior )
	{
		DumpMacro( vt, behavior, WIDE("on") );
	}
	LIST_FORALL( pe->pMacros, idx, PMACRO, pm )
		DumpMacro( vt, pm, WIDE("macro") );
}

//--------------------------------------------------------------------------

 PENTITY GetOneOfMyFrames( PCOMMON pc )
{
	PENTITY peCheck;
	INDEX idx;
	LIST_FORALL( g.pMyFrames, idx, PENTITY, peCheck )
	{
		// this can accidentally create a list which will create a memory leak.
		if( peCheck->pPlugin )
		{
			PCOMMON_TRACKER pctCheck = (PCOMMON_TRACKER)GetLink( &peCheck->pPlugin, g.iCommon );
         // and 099.99% of the time it won't be... fuhk
			//lprintf( WIDE("this list may be orphaned... %p"), pctCheck );
			if( pctCheck )
			{
				PCOMMON pcCheck = pctCheck->control.pc;
				if( pc == pcCheck )
				{
					return peCheck;
				}
			}
		}
	}
   lprintf( WIDE("Failed to find contorl... entity this time...") );
   return NULL;
}

//--------------------------------------------------------------------------

int CPROC SaveCommonMacroData( PCOMMON pc, PVARTEXT pvt )
{
	PCOMMON pcFrame = GetFrame( pc );
	PENTITY peFrame = (PENTITY)GetOneOfMyFrames( pcFrame );
	if( peFrame && IsOneOfMyFrames( peFrame ) )
	{
		// gather the script components attached on behaviors
		DumpMacros( pvt, (PENTITY)GetOneOfMyFrames( pc ) );
      return 1;
	}
   return 0;
}

#if 0
static volatile_variable_entry common_vars[] = { { DEFTEXT(WIDE("caption"))
																 , NULL
																 , SetCaption }
															  , { DEFTEXT( WIDE("x") )
																 , NULL, NULL }
															  , { DEFTEXT( WIDE("y") )
																 , NULL, NULL }
															  , { DEFTEXT( WIDE("width") )
																 , NULL, NULL }
															  , { DEFTEXT( WIDE("height") )
																 , NULL, NULL }
};
#endif

//--------------------------------------------------------------------------
PENTITY CommonInitControl( PCOMMON pc )
{
	PENTITY peNew = NULL;
   EnterCriticalSec( &g.csCreating );
	{
		PCOMMON_TRACKER pComTrack = New( COMMON_TRACKER );
		PCOMMON pcFrame = GetParentControl( pc );
		pComTrack->control.pc = pc;
		pComTrack->flags.created_internally = 0;
		pComTrack->flags.menu = 0;
		if( g.peCreating )
		{
			pComTrack->flags.created_internally = 1;
         peNew = g.peCreating;
			g.peCreating = NULL;
			SetLink( &peNew->pPlugin, g.iCommon, (POINTER)pComTrack );
		}
		else
		{
			PENTITY peFrame = GetOneOfMyFrames( pc );
			if( !peFrame )
			{
				// need to validate that this IS a PE frame and not
				// something like the pointer to the object which handles
				// locking for common buttons...
				PTEXT name = SegCreateFromText( GetControlTypeName( pc ) );
				while( pcFrame && !( peFrame = (PENTITY)GetOneOfMyFrames( pcFrame ) ) )
				{
					pcFrame = GetParentControl( pcFrame );
				}
				peNew = CreateEntityIn( peFrame, name );
				if( peNew )
				{
					SetLink( &peNew->pPlugin, g.iCommon, (POINTER)pComTrack );
					AddLink( &peNew->pDestroy, DestroyAControl );

					// AddMethod( peNew, dialog_commands );
					// AddMethod( peNew, dialog_commands+1 );
					// AddMethod( peNew, dialog_commands+2 );

					AddLink( &g.pMyFrames, peNew );
				}
				else
               lprintf( WIDE("Failed to create entity for control...") );
			}
         // this needs to be done so that buttons may perform 'on' commands.
			//UnlockAwareness( CreateAwareness( peNew ) );
			LeaveCriticalSec( &g.csCreating );
			return NULL;
		}
	}
	LeaveCriticalSec( &g.csCreating );
   return peNew;
}


static int OnCreateObject( WIDE("psi_control"), WIDE("generic control...") )
//static int CPROC AddAControl
( PSENTIENT ps, PENTITY peNew, PTEXT params )
{
	PENTITY peFrame = FindContainer( peNew );
	PCOMMON_TRACKER pct = (PCOMMON_TRACKER)GetLink( &peFrame->pPlugin, g.iCommon );
	PCOMMON pc;
	TEXTCHAR *control_type;
	if( pct )
	{
		pc = pct->control.pc;
	}
	else
      pc = NULL;
	//if( pc )
	//{
		//lprintf( WIDE("hmm... create a child of the frame... but within or without?") );
      // abort!
   //   return 0;
	//}
	//else
	{
		control_type = GetText( GetParam( ps, &params ) );
		if( !control_type )
		{
			int first = 1;
			CTEXTSTR name;
			PCLASSROOT data = NULL;
			PVARTEXT pvt = VarTextCreate();
			vtprintf( pvt, WIDE("Must specify type of control to make: ") );
			for( name = GetFirstRegisteredName( WIDE("psi/control"), &data );
				 name;
				  name = GetNextRegisteredName( &data ) )
			{
				int n;
				for( n = 0; name[n]; n++ )
					if( name[n] < '0' || name[n] > '9' )
						break;
				if( name[n] )
				{
					vtprintf( pvt, WIDE("%s%s"), first?WIDE(""):WIDE(", "), name );
               first = 0;
				}
			}
			EnqueLink( &ps->Command->Output, VarTextGet( pvt ) );
			VarTextDestroy( &pvt );
         return 1;
		}

	}
	{
		TEXTCHAR *caption = GetText( GetParam( ps, &params ) );
		if( caption )
		{
			const TEXTCHAR *px = GetText( GetParam( ps, &params ) )
		, *py = GetText( GetParam( ps, &params ) )
		, *pw = GetText( GetParam( ps, &params ) )
		, *ph = GetText( GetParam( ps, &params ) );
			if( !px ) px = WIDE("120");
			if( !py ) py = WIDE("50");
			if( !pw ) pw = WIDE("320");
			if( !ph ) ph = WIDE("250");
			{
				PCOMMON pNewControl;
				EnterCriticalSec( &g.csCreating );
            g.peCreating = peNew;
				pNewControl = MakeNamedCaptionedControl( pc, control_type
																 , atoi( px ), atoi( py )
																 , atoi( pw ), atoi( ph )
																 , 0
																 , caption
																	);
				// named control have invoked WIDE("extra init") method, which
				// if all goes well we should be able to discover our parent
				//SetCommonUserData( pNewControl, (PTRSZVAL)peNew );
				AddLink( &g.pMyFrames, peNew );
				if( g.peCreating )
				{
					lprintf( WIDE("An extra init function did not clear g.peCreating.!") );
					DebugBreak();
					g.peCreating = NULL;
				}
				LeaveCriticalSec( &g.csCreating );
#if 0
				AddVolatileVariable( peNew, common_vars, (PTRSZVAL)pNewControl );
				AddVolatileVariable( peNew, common_vars+1, (PTRSZVAL)pNewControl );
				AddVolatileVariable( peNew, common_vars+2, (PTRSZVAL)pNewControl );
				AddVolatileVariable( peNew, common_vars+3, (PTRSZVAL)pNewControl );
				AddVolatileVariable( peNew, common_vars+4, (PTRSZVAL)pNewControl );
				AddMethod( peNew, dialog_commands );
				AddMethod( peNew, dialog_commands+1 );
				AddMethod( peNew, dialog_commands+2 );
            if( !pc )
					AddMethod( peNew, dialog_commands+3 );
#endif

				AddLink( &peNew->pDestroy, DestroyAControl );
				// add custom methods here?
            // add a init method?

				// these don't really need awareness?
				// they do need a method of invoking ON events...
				// maybe I can summon a parent awareness to do it's bidding...
				UnlockAwareness( CreateAwareness( peNew ) );
				// allow control to specify /on methods
				// or query the control registration tree for possibilities...
				// on click...

            return 0;
			}
		}
		else
		{
			DECLTEXT( msg, WIDE("Must supply a caption for the frame.") );
         EnqueLink( &ps->Command->Output, &msg );
		}
	}
	return 1; // abort creation for now.
}

int CPROC CustomFrameInit( PCOMMON pc )
{
	CommonInitControl( pc );
   return 1;
}

//--------------------------------------------------------------------------
static int CPROC CustomDefaultInit( PCOMMON pc )
{
	CommonInitControl( pc );
   return 1;
}

//--------------------------------------------------------------------------
static int CPROC CustomDefaultDestroy( PCOMMON pc )
{
	// registered as destroy of control...
   // predates On___ behaviors.
	PENTITY pe = GetOneOfMyFrames( pc );
	if( pe )
	{
		// in the course of destruction, DestroyCallbacks dispatched
		// which removes this from my list of things now.
      // since multiple paths of destroyentity might happen...
		DestroyEntity( pe );
	}
   return 1;
}

//--------------------------------------------------------------------------

PRELOAD( RegisterExtraInits )
{
	TEXTCHAR rootname[128];
   InitializeCriticalSec( &g.csCreating );
}


PUBLIC( TEXTCHAR *, RegisterRoutines )( void )
{
   g.iCommon = RegisterExtension( WIDE("PSI Control") );
	////RegisterObject( WIDE("Frame"), WIDE("Allows interface to Panther's Slick Interface dialogs"), InitFrame );
	//RegisterObject( WIDE("Control"), WIDE("Allows interface to Panther's Slick Interface dialogs"), AddAControl );
	RegisterObject( WIDE("Menu"), WIDE("A popup menu selector"), CreatePopupThing );

   // this registers a default, if the control itself does not specify...
	SimpleRegisterMethod( WIDE("psi/control/rtti/extra init")
							  , CustomDefaultInit, WIDE("int"), WIDE("dekware common init"), WIDE("(PCOMMON)") );
	SimpleRegisterMethod( WIDE("psi/control/rtti/extra destroy")
							  , CustomDefaultDestroy, WIDE("int"), WIDE("dekware common destroy"), WIDE("(PCOMMON)") );

	SimpleRegisterMethod( WIDE("psi/control/") CONTROL_FRAME_NAME  WIDE("/rtti/extra init")
							  , CustomFrameInit, WIDE("int"), WIDE("extra init"), WIDE("(PCOMMON)") );
   //DumpRegisteredNames();
	return DekVersion;
}

//--------------------------------------------------------------------------

PUBLIC( void, EditControlBehaviors)( PSI_CONTROL pc )
{
   // interesting wonder what this does...
}

//--------------------------------------------------------------------------

PUBLIC( void, UnloadPlugin )( void ) // this routine is called when /unload is invoked
{
	UnregisterObject( WIDE("Control") );
}

//--------------------------------------------------------------------------
// $Log: dialog.c,v $
// Revision 1.6  2005/02/22 12:28:31  d3x0r
// Final bit of sweeping changes to use CPROC instead of undefined proc call type
//
// Revision 1.5  2005/01/17 09:01:13  d3x0r
// checkpoint ...
//
// Revision 1.4  2004/12/16 10:01:10  d3x0r
// Continue work on PSI plugin module for dekware, which is now within realm of grasp... need a couple enum methods
//
// Revision 1.3  2004/09/27 16:06:47  d3x0r
// Checkpoint - all seems well.
//
// Revision 1.2  2003/01/13 08:47:27  panther
// *** empty log message ***
//
// Revision 1.1  2002/08/04 01:34:43  panther
// Initial commit.
//
//
