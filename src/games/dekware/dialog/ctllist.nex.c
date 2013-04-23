#include <stdhdrs.h>
// ooOo fancy text parsing... good thing
// we get fed fancy text segments :)
#include <configscript.h> // sack
#include <plugin.h> // dekware
#include <controls.h> // psi

#include "global.h" // common for this plugin

// this is a variable in dialog.nex

void CPROC DoListItemOpened( PTRSZVAL psvUser, PCONTROL pc, PLISTITEM hli, LOGICAL bOpened )
{
	PENTITY pe = (PENTITY)psvUser;
   InvokeBehavior( WIDE("open"), pe, pe->pControlledBy, NULL );
}

void CPROC DoDoubleClicker( PTRSZVAL psvUser, PCONTROL pc, PLISTITEM hli )
{
	PENTITY pe = (PENTITY)psvUser;
   InvokeBehavior( WIDE("double"), pe, pe->pControlledBy, NULL );
}

void CPROC DoSelectionChanged ( PTRSZVAL psvUser, PCOMMON pc, PLISTITEM hli )
{
	PENTITY pe = (PENTITY)psvUser;
   InvokeBehavior( WIDE("select"), pe, pe->pControlledBy, NULL );
}

static int ObjectMethod( WIDE("psi_listbox"), WIDE("add"), WIDE("Add an item to a list") )
//static int CPROC AddItem
( PSENTIENT ps, PTEXT parameters )
{
	PCOMMON_TRACKER pct = (PCOMMON_TRACKER)GetLink( &ps->Current->pPlugin, g.iCommon );
	PCOMMON pc = pct->control.pc;
   // macro duplicate so subsitutions happen...
	PTEXT line = BuildLine( parameters );
	AddListItem( pc, GetText( line ) );
	LineRelease( line );
   return 0;
}

static int ObjectMethod( WIDE("psi_listbox"), WIDE("tree"), WIDE("Add an item to a list") )
//static int CPROC SetTree
( PSENTIENT ps, PTEXT parameters )
{
	PTEXT temp = GetParam( ps, &parameters );
	PCOMMON_TRACKER pct = (PCOMMON_TRACKER)GetLink( &ps->Current->pPlugin, g.iCommon );
	PCOMMON pc = pct->control.pc;
	if( temp )
	{
      LOGICAL yesno;
		GetBooleanVar( &temp, &yesno );
      SetListboxIsTree( pc, yesno );

	}
	else
	{
      SetListboxIsTree( pc, FALSE );
	}
   return 0;
}

#if 0
command_entry methods[] = { { DEFTEXT( WIDE("add") ), 1, 3, DEFTEXT( WIDE("Add an item to a list.") ), AddItem }
								  , { DEFTEXT( WIDE("tree") ), 1, 4, DEFTEXT( WIDE("Set listbox treemode.") ), SetTree }
								  //, { DEFTEXT(),
						 };
#endif
//-- draw will require a command interface to the image library
//-- are NOT ready for this.
//void CPROC ButtonDraw(

static int OnCreateObject( WIDE("psi_listbox"), WIDE("a PSI List Control") )( PSENTIENT ps, PENTITY pe, PTEXT params )
{
   return 0;
}

static void InitControlObject( PENTITY pe, PCOMMON pc )
{
	if( !pe )
      return;
	AddBehavior( pe, WIDE("select"), WIDE("an item has been selected.") );
	AddBehavior( pe, WIDE("double"), WIDE("double click on an item has been done.") );
	AddBehavior( pe, WIDE("open"), WIDE("An item in a tree list has been expanded.") );
//	AddMethod( pe, methods + 0 );
//   AddMethod( pe, methods + 1 );
//AddVolatileVariable( pe, normal_button_vars+2, (PTRSZVAL)pc );
	SetListItemOpenHandler( pc, DoListItemOpened, (PTRSZVAL)pe );
	SetDoubleClickHandler( pc, DoDoubleClicker, (PTRSZVAL)pe );
   SetSelChangeHandler( pc, DoSelectionChanged, (PTRSZVAL)pe );
}

int CPROC CustomInitListbox( PCOMMON pc )
{
	InitControlObject( CommonInitControl( pc ), pc );
   return 1;
}

PRELOAD( LstRegisterExtraInits )
{
	SimpleRegisterMethod( WIDE("psi/control/") LISTBOX_CONTROL_NAME WIDE("/rtti/extra init")
							  , CustomInitListbox, WIDE("int"), WIDE("extra init"), WIDE("(PCOMMON)") );
   DumpRegisteredNamesFrom( (PCLASSROOT)WIDE("psi/control") LISTBOX_CONTROL_NAME );
}

