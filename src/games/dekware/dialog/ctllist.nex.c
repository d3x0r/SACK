#include <stdhdrs.h>
// ooOo fancy text parsing... good thing
// we get fed fancy text segments :)
#include <configscript.h> // sack
#include <plugin.h> // dekware
#include <controls.h> // psi

#include "global.h" // common for this plugin

// this is a variable in dialog.nex

void CPROC DoListItemOpened( uintptr_t psvUser, PSI_CONTROL pc, PLISTITEM hli, LOGICAL bOpened )
{
	PENTITY pe = (PENTITY)psvUser;
   InvokeBehavior( "open", pe, pe->pControlledBy, NULL );
}

void CPROC DoDoubleClicker( uintptr_t psvUser, PSI_CONTROL pc, PLISTITEM hli )
{
	PENTITY pe = (PENTITY)psvUser;
   InvokeBehavior( "double", pe, pe->pControlledBy, NULL );
}

void CPROC DoSelectionChanged ( uintptr_t psvUser, PSI_CONTROL pc, PLISTITEM hli )
{
	PENTITY pe = (PENTITY)psvUser;
   InvokeBehavior( "select", pe, pe->pControlledBy, NULL );
}

static int ObjectMethod( "psi_listbox", "add", "Add an item to a list" )
//static int CPROC AddItem
( PSENTIENT ps, PENTITY pe, PTEXT parameters )
{
	PSI_CONTROL_TRACKER pct = (PSI_CONTROL_TRACKER)GetLink( &ps->Current->pPlugin, g.iCommon );
	PSI_CONTROL pc = pct->control.pc;
   // macro duplicate so subsitutions happen...
	PTEXT line = BuildLine( parameters );
	AddListItem( pc, GetText( line ) );
	LineRelease( line );
   return 0;
}

static int ObjectMethod( "psi_listbox", "tree", "Add an item to a list" )
//static int CPROC SetTree
( PSENTIENT ps, PENTITY pe, PTEXT parameters )
{
	PTEXT temp = GetParam( ps, &parameters );
	PSI_CONTROL_TRACKER pct = (PSI_CONTROL_TRACKER)GetLink( &ps->Current->pPlugin, g.iCommon );
	PSI_CONTROL pc = pct->control.pc;
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
command_entry methods[] = { { DEFTEXT( "add" ), 1, 3, DEFTEXT( "Add an item to a list." ), AddItem }
								  , { DEFTEXT( "tree" ), 1, 4, DEFTEXT( "Set listbox treemode." ), SetTree }
								  //, { DEFTEXT(),
						 };
#endif
//-- draw will require a command interface to the image library
//-- are NOT ready for this.
//void CPROC ButtonDraw(

static int OnCreateObject( "psi_listbox", "a PSI List Control" )( PSENTIENT ps, PENTITY pe, PTEXT params )
{
   return 0;
}

static void InitControlObject( PENTITY pe, PSI_CONTROL pc )
{
	if( !pe )
      return;
	AddBehavior( pe, "select", "an item has been selected." );
	AddBehavior( pe, "double", "double click on an item has been done." );
	AddBehavior( pe, "open", "An item in a tree list has been expanded." );
//	AddMethod( pe, methods + 0 );
//   AddMethod( pe, methods + 1 );
//AddVolatileVariable( pe, normal_button_vars+2, (uintptr_t)pc );
	SetListItemOpenHandler( pc, DoListItemOpened, (uintptr_t)pe );
	SetDoubleClickHandler( pc, DoDoubleClicker, (uintptr_t)pe );
   SetSelChangeHandler( pc, DoSelectionChanged, (uintptr_t)pe );
}

int CPROC CustomInitListbox( PSI_CONTROL pc )
{
	InitControlObject( CommonInitControl( pc ), pc );
   return 1;
}

PRELOAD( LstRegisterExtraInits )
{
	SimpleRegisterMethod( "psi/control/" LISTBOX_CONTROL_NAME "/rtti/extra init"
							  , CustomInitListbox, "int", "extra init", "(PSI_CONTROL)" );
}

